// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// f32\sfile\sf_prel.cpp
// 
//

#include "sf_std.h"
#include "u32std.h"
#include <e32hal.h>
#include "sf_cache_man.h"

_LIT(KLitInitCompleteThread,"InitCompleteThread");

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
_LIT(KCorruptFileNamesList,"CorruptFileNames.lst");
_LIT(KSysData,"\\sys\\data\\");
_LIT(KSystemData,"\\system\\data\\");
/**
ASCII text file reader constructor
This code is more or less lifted directly from estart.cpp
*/
TText8FileReader::TText8FileReader()
	{

	iBufPos=-1;
	iFileDataBuf=NULL;
	iFileSize=0;	
	}

/**
ASCII text file reader destructor
*/	
TText8FileReader::~TText8FileReader()
	{
	
	delete[] iFileDataBuf;
	}	

/**	
Supply an ASCII text file for the file reader. 
This function reads the entire contents of this file into a buffer, converting to
unicode / folding each character. Subsequent parsing of the file data is all done from this
buffer.
@param aFile The file to be read. Must already be opened for read access in EFileStreamText mode.  
@return	KErrNone if no error.
*/
TInt TText8FileReader::Set(RFile& aFile)
	{
	
	iFile=aFile;
	iBuf.Zero();
	iBufPos=0;
	
	// Read the size of the file
	TInt r=iFile.Size(iFileSize);
	if (r!=KErrNone || !iFileSize)
		return(KErrGeneral);
	
	// Allocate a buffer to read in the 
	iFileDataBuf=new TText[iFileSize+1];	// +1 in case need to NULL terminate the end of the last string
	if (iFileDataBuf==NULL)
		return(KErrNoMemory);

	// Read the entire contents of the file into the buffer
	TPtr fdata(NULL,0);	
	TInt pos=0;
	r=iFile.Seek(ESeekStart,pos);
	while (pos<iFileSize)
		{
		if ((r=iFile.Read(iBuf))!=KErrNone)
			return(r);
		fdata.Set((iFileDataBuf+pos),0,iBuf.Length());	
		fdata.Copy(iBuf);
		fdata.Fold();
		pos+=iBuf.Length();	
		}
	return(KErrNone);	
	}
	
/**
Return the next record from the text file.
@param aPtr A TPtr which is setup with the address and length of the next record, referencing
the data in the reader's buffer.
@return	KErrNone if a record is successfully loaded into the buffer, KErrEof if the end of the
file is encountered, KErrGeneral if a file hasn't been set.
*/
TInt TText8FileReader::Read(TPtr& aPtr)
	{
	
	// Check that Set() has been called.
	if (iBufPos<0)
		return(KErrGeneral);
		
	// Check if we have run into the end of the file	
	TInt bufRemainder=(iFileSize-iBufPos);
	if (!bufRemainder)
		return(KErrEof);
		
	// Setup the descriptor passed with the next record - don't include the record terminator
	// The line terminators are CR + LF for DOS
	// whereas only LF for Unix line endings
	aPtr.Set((iFileDataBuf+iBufPos),bufRemainder,bufRemainder);
	TInt len=aPtr.Locate('\n');
	if (len != KErrNotFound)
		{
		iBufPos += len;
	    // Check for DOS line ending to support both DOS and Unix formats
	    if ((len != 0) && (iFileDataBuf[iBufPos-1] == '\r'))
			{
			len--;
			}
		aPtr.SetLength(len);
		}
	else
		iBufPos=iFileSize;
	
	// Point iBufPos to the next non-empty line
	while (iBufPos<iFileSize && (iFileDataBuf[iBufPos]=='\n' || iFileDataBuf[iBufPos]=='\r'))
		iBufPos++;
	return(KErrNone);
	}

/**
Parse a record and split it into filename, error code & use once flag
@param aLine, this a a record from read from the file
@param aName, this is the parsed filename component of the record
@param aReturnCode, the error code that will be returned when an attempt is made to open the file
@param aUseOnce, flag which states whether the error should persist over more than one open attempt
@return error code, if the record is malformed return KErrCorrupt (ironic, what?) else KErrNone
*/
TInt ParseCorruptNamesRecord(TPtr aLine, TPtr& aName, TInt& aReturnCode, TBool&  aUseOnce)
	{
	aName.Set(NULL,0,0);
	// Remove leading spaces
	aLine.TrimLeft();
	TPtr remainderPtr(aLine);

	// collect pathname
	TInt lenFileName=remainderPtr.Locate(',');
	if(lenFileName<1)
		return KErrCorrupt;
	TPtr pathNamePtr(remainderPtr.LeftTPtr(lenFileName));
	// Remove trailing spaces
	pathNamePtr.TrimRight();
	// Parse the pathname to see if it could be valid, do not allow wild cards
	TParse fileParse;
	TInt r=fileParse.SetNoWild(pathNamePtr,NULL,NULL);
	if(r!=KErrNone)
		return KErrCorrupt;

	// move over delimiter to collect the user supplied return code
	// see first if sufficient length in the record
	TInt lenRemainder=remainderPtr.Length();
	if(lenRemainder<lenFileName+2)
		return KErrCorrupt;

	remainderPtr.Set(remainderPtr.MidTPtr(lenFileName+1));
	remainderPtr.TrimLeft();
	TInt lenReturnCode=remainderPtr.Locate(',');
	if(lenReturnCode<1)
		return KErrCorrupt;

	TPtr returnCodePtr(remainderPtr.LeftTPtr(lenReturnCode));
	TLex lexLine(returnCodePtr);
	TInt returnCode;
	if(lexLine.Val(returnCode)!=KErrNone)
		return KErrCorrupt;

	lenRemainder=remainderPtr.Length();
	if(lenRemainder<lenReturnCode+2)
		return KErrCorrupt;

	// collect the useOnce flag value
	remainderPtr.Set(remainderPtr.MidTPtr(lenReturnCode+1));
	remainderPtr.Trim();
	TBool useOnce;
	// This must either be "EVERY" or "ONCE"
	if (remainderPtr.MatchF(_L("EVERY"))!=KErrNotFound)
		useOnce=EFalse;
	else if(remainderPtr.MatchF(_L("ONCE"))!=KErrNotFound)
		useOnce=ETrue;
	else
		return KErrCorrupt;

	// Everything has worked out, so set up output args

	aName.Set(pathNamePtr);
	aReturnCode=returnCode;
	aUseOnce=useOnce;
	return KErrNone;
	}

/**
Open c:\CorruptFileNames.lst
File contains a set of text records
Each record has comma delimited fields and has the form:
 <filename>, <errorCode>, <[ONCE,EVERY]>
e.g.
c:\system\bin\bt.lst, -6,EVERY
the filename should be fully qualified,
errorCode should be a literal not a symbol
EVERY means that every attempt to open the file will be failed
ONCE means that only the first access to the file will be failed
*/

TInt FindHitListFile(RFs aFs)
	{
	TFindFile finder(aFs);
	TInt r=finder.FindByDir(KCorruptFileNamesList,_L("\\"));
	if(r!=KErrNone)
		{
		r=finder.FindByDir(KCorruptFileNamesList,KSysData);
		}

	if(r!=KErrNone)
		{
		r=finder.FindByDir(KCorruptFileNamesList,KSystemData);
		}

	if(r==KErrNone)
		{  // This is stored as a global because the name can be retrieved by a user api
		gCorruptFileNamesListFile=finder.File().Alloc();
		if(gCorruptFileNamesListFile==NULL)
			{
			r=KErrNoMemory;
			}
		}
	return r;
	}

void CreateCorruptFileNamesList()
	{
	RFs fs;
	
    TInt r = fs.Connect();
	if(r!=KErrNone)
		return;
	
    r=FindHitListFile(fs);
	if(r!=KErrNone)
		return;
	
	RFile f;
	r=f.Open(fs,*gCorruptFileNamesListFile,EFileShareExclusive|EFileStreamText|EFileRead);
	if(r!=KErrNone)
		{
		return;
		}
	// Create a corrupt filenames file reader and pass it the file object. This will also result in the copying of the contents of
	// the file into reader's buffer. Filename information read from the file will include pointers to text strings in
	// this buffer and so the reader object must not be deleted until processing is complete.
	__PRINT1(_L("@@@@ Using Corrupt names file %S"),gCorruptFileNamesListFile);

	TText8FileReader* namesFile=new TText8FileReader;	
	if (!namesFile)
		{
		f.Close();
		return;
		}

	r=namesFile->Set(f);
	f.Close();
	fs.Close();
	if(r==KErrNone)
		{
		// Parse each drive mapping record in turn, saving the information in an array of mapping structures - one for
		// each valid record.
		TPtr linePtr(NULL,0);
		while ((r=namesFile->Read(linePtr))==KErrNone)
			{		
			TInt returnCode;
			TBool useOnce;
			TPtr namePtr(NULL,0);
			if (ParseCorruptNamesRecord(linePtr,namePtr,returnCode,useOnce)==KErrNone)
				{
				// Valid corrupt filename record found
				TCorruptNameRec* newRec=new TCorruptNameRec;
				if(!newRec)
					{
					break;
					}
				if(newRec->Construct(&namePtr,returnCode,useOnce,gCorruptFileNameList)!=KErrNone)
					{
					delete newRec;
					newRec=NULL;
					break;
					}
	
				gCorruptFileNameList=newRec;
				}
			else
				{
				__PRINT1(_L("@@@@@ Bad Parse corrupt file name record %S\n"),&linePtr);
				}
			}
		}

	delete namesFile;
	}
#endif

TInt InitCompleteThread(TAny* aPtr)
	{
	__PRINT(KLitInitCompleteThread);
	RMessagePtr2 message=*(RMessagePtr2*)aPtr;
	RThread::Rendezvous(0);


	//
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	CreateCorruptFileNamesList();
#endif

	TInt r = KErrNone;
	message.Complete(r);
	return(KErrNone);
	}


TInt TFsStartupInitComplete::DoRequestL(CFsRequest *aRequest)
//
// do further initialisation once necessary startup init complete outside file server
//
	{
	if(StartupInitCompleted)
		return(KErrAlreadyExists);


	StartupInitCompleted=ETrue;
	RMessagePtr2 msg=aRequest->Message();
	RThread thread;
	TInt r=thread.Create(KLitInitCompleteThread,InitCompleteThread,KDefaultStackSize,NULL,&msg);
	if (KErrNone == r)
		{
		TRequestStatus s;
		thread.Rendezvous(s);
		if (s==KRequestPending)
			thread.Resume();
		else
			thread.Kill(0);
		thread.Close();
		User::WaitForRequest(s);
		r = s.Int();
		}
	return(KErrNone);
	}

TInt TFsStartupInitComplete::Initialise(CFsRequest* /*aRequest*/)
//
//
//
	{
	return KErrNone;
	}
