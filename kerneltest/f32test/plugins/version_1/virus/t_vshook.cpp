// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\virus\t_vshook.cpp
// 
//

#include "t_vshook.h"
#include <f32pluginutils.h>


_LIT(KVirusScannerName, "This is a test virus scanner");


/**
Leaving New function for the plugin
@internalComponent
*/
CTestVirusHook* CTestVirusHook::NewL()
	{
	return new(ELeave) CTestVirusHook;
	}


/**
Constructor for the plugin
@internalComponent
*/
CTestVirusHook::CTestVirusHook()
	{
//	RDebug::Print(_L("EMaxClientOperations %d, size of CFsPlugin %d, iSignatureOffset %d"), EMaxClientOperations, sizeof(CFsPlugin), _FOFF(CTestVirusHook, iSignature));
	ASSERT(iSignature == 0);
	iSignature = 0x1234;
	}


/**
The destructor for the test virus scanner hook.  This would
not be a part of a normal virus scanner implementation as
normal virus scanners cannot be unloaded - it must be 
provided in the test virus scanner server so that it can
be tested with the F32 test suite.
@internalComponent
*/
CTestVirusHook::~CTestVirusHook()
	{
	iFs.Close();

	for (TInt i = 0; i < iSignaturesLoaded; i++)
		{
		if (iKnownSignatures[i])
			{
			delete iKnownSignatures[i];
			}
		}
	ASSERT(iSignature == 0x1234);
	}

/**
Initialise the virus scanner.
Reads the virus definition file and then waits for
notification that files containing a virus have been
detected.
@internalComponent
*/
void CTestVirusHook::InitialiseL()
	{
	User::LeaveIfError(RegisterIntercept(EFsFileOpen,			EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSubClose,		EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileRename,			EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsRename,				EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDelete,				EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsReplace,			EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsReadFileSection,	EPreIntercept));

	ReadVirusDefinitionFile();
	}

/**
Read the virus definition file C:\virusdef.txt and parse
its contents.  Any virus definitions found in that file
are added to the KnownSignatures array so they can be
used by the virus scanning hook.

@internalComponent

@return KErrNone if the file was read and parsed
successfuly.
*/
TInt CTestVirusHook::ReadVirusDefinitionFile()
	{
	TInt r = iFs.Connect();
	if (r != KErrNone)
		return r;
	
	r = iFs.SetNotifyChange(EFalse);	// Disable change notifications
	if (r != KErrNone)
		return r;
	
	//Open the virus definition file
	RFile vsDefFile;
	r = vsDefFile.Open(iFs, _L("C:\\virusdef.txt"), EFileShareAny);
	if (r != KErrNone)
		return r;
	
	TInt fileSize=0;
	r = vsDefFile.Size(fileSize);
	if (r != KErrNone)
		{
		vsDefFile.Close();
		return r;
		}

	HBufC8* defBuf=NULL;
	
	TRAP(r,defBuf = HBufC8::NewL(fileSize));
	if (r != KErrNone)
		{
		vsDefFile.Close();
		return r;
		}

	TPtr8 ptr(defBuf->Des());
	r = vsDefFile.Read(ptr);
	if (r != KErrNone)
		{
		delete defBuf;
		vsDefFile.Close();
		return r;
		}

	//Now parse the definition file, putting the definitions into the hook's
	//array of known virus signatures.
	TInt bytesParsed     = 0;
	TInt stringBeginPos  = 0;
	TInt stringEndPos    = 0;
	TInt stringLength    = 0;
	HBufC8* signatureBuf = NULL;
	while (bytesParsed < fileSize)
		{
		ptr.Set(defBuf->Des());
		ptr.Set(&ptr[bytesParsed], fileSize-bytesParsed, fileSize-bytesParsed);
		stringBeginPos = ptr.MatchF(_L8(":*;*"));

		if (stringBeginPos < 0)
			{
			break;
			}

		stringBeginPos += 1; //:
		stringBeginPos += bytesParsed;
		ptr.Set(defBuf->Des());
		ptr.Set(&ptr[stringBeginPos], fileSize-stringBeginPos, fileSize-stringBeginPos);
		stringEndPos = ptr.MatchF(_L8("*;*"));

		if (stringEndPos < 0)
			{
			break;
			}

		stringEndPos += bytesParsed;
		stringLength = stringEndPos - stringBeginPos + 1;

		ptr.Set(defBuf->Des());
		TRAP(r,signatureBuf = HBufC8::NewL(stringLength));
		
		TPtrC8 actualSig(ptr.Mid(stringBeginPos, stringLength));
		
		TPtr8 ptr2(signatureBuf->Des());
		ptr2.Append(actualSig);
		iKnownSignatures[iSignaturesLoaded] = signatureBuf;
		iSignaturesLoaded++;

		bytesParsed += 1; //:
		bytesParsed += stringLength;
		bytesParsed += 1; //;
		}

	//Cleanup
	delete defBuf;
	vsDefFile.Close();

	return r;
	}

/**
@internalComponent
*/
TInt CTestVirusHook::DoRequestL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNotSupported;

	TInt function = aRequest.Function();
	
	iDrvNumber = aRequest.DriveNumber();

	switch(function)
		{
		case EFsFileOpen:
			err = VsFileOpen(aRequest);
			break;

		case EFsFileSubClose:
			VsFileClose(aRequest);
			break;

		case EFsFileRename:
		case EFsRename:
		case EFsReplace:
			err = VsFileRename(aRequest);
			break;

		case EFsDelete:
			err = VsFileDelete(aRequest);
			break;

		case EFsReadFileSection:
			err = VsReadFileSection(aRequest);
			break;

		default:
			break;
		}

	return err;
	}


/**
@internalComponent
*/
TInt CTestVirusHook::VsFileOpen(TFsPluginRequest& aRequest)
	{
	TFileName fileName;
	TInt err = ValidateRequest(aRequest, fileName);
	if (err == KErrNone)
		{
		err = ScanFile(fileName);
		if (err != KErrNone)
			{
			// Clean the infected file
			CleanFile(fileName, EFileOpen);
			}
		}	

	return err;
	}

/**
@internalComponent
*/
void CTestVirusHook::VsFileClose(TFsPluginRequest& aRequest)
	{
	TFileName fileName;
	TInt err = GetName(&aRequest, fileName);
	if(err == KErrNone)
		{
		err = ScanFile(fileName);
		if (err != KErrNone)
			{
			// Clean the infected file
			CleanFile(fileName, EFileClose);
			}
		}
	}

/**
@internalComponent
*/
TInt CTestVirusHook::VsFileRename(TFsPluginRequest& aRequest)
	{

	TInt err = VsDirRename(aRequest);
	if(err != KErrAccessDenied)
		{
		TFileName fileName;
		err = ValidateRequest(aRequest, fileName);
		if (err == KErrNone)
			{
			err = ScanFile(fileName);
			if (err != KErrNone)
				{
				// Clean the infected file
				CleanFile(fileName, EFileRename);
				}
			}
		}

	return err;
	}

/**
@internalComponent
*/
TInt CTestVirusHook::VsDirRename(TFsPluginRequest& aRequest)
	{

	TFileName fileName;
	TInt err = GetName(&aRequest, fileName);
	if(err != KErrNone)
		return(err);
	
	err = fileName.Find(_L("\\system\\lib\\"));
	if (err != KErrNotFound)
		{
		//Don't allow sys\bin to be ever renamed
		return KErrAccessDenied;
		}
	err = fileName.Find(_L("\\sys\\bin\\"));
	if (err != KErrNotFound)
		{
		//Don't allow sys\bin to be ever renamed
		return KErrAccessDenied;
		}

	return err;
	}

/**
@internalComponent
*/
TInt CTestVirusHook::VsFileDelete(TFsPluginRequest& aRequest)
	{

	TFileName fileName;
	TInt err = ValidateRequest(aRequest, fileName);
	return err;
	}

/**
@internalComponent
*/
TInt CTestVirusHook::VsReadFileSection(TFsPluginRequest& aRequest)
	{

	// The t_virus test uses a filename clean.txt, a read length of 8 and a read position of 0.
	TFileName fileName;
	TInt len;
	TInt pos;

	// test getting name on read file section intercept
	TInt err = GetName(&aRequest, fileName);
	if(err != KErrNone)
		{
		return(err);
		}
	TParse parse;
	parse.Set(fileName,NULL,NULL);
	TPtrC name = parse.Name();
	if(name.CompareF(_L("clean"))!=0)
		{
		return(KErrGeneral);
		}
	TPtrC ext = parse.Ext();
	if(ext.CompareF(_L(".txt"))!=0)
		{
		return(KErrGeneral);
		}

	// test getting read length and required file position on read file section intercept
	err = GetFileAccessInfo(&aRequest, len, pos);
	if(err != KErrNone)
		{
		return(err);
		}
	if ((len != 8) || (pos != 0))
		{
		return(KErrGeneral);
		}
	
	return KErrNone;
	}


/**
@internalComponent
*/
TInt CTestVirusHook::VirusScannerName(TDes& aName)
	{
	aName = KVirusScannerName;
	return KErrNone;
	}

/**
Reads the contents of the file passed in and checks
whether it contains any of the specified virus
signatures

@return A value depending on whether a known virus is
found within the file.

@param aFile A CFileCB object which can be used to read the file.
@internalComponent
*/
TInt CTestVirusHook::ScanFile(const TDesC& aName)
	{

	TInt r    = KErrNone;
	TInt pos  = 0;
	TInt size = 0;

	// Rename the file	
	TPtrC tmpFile = _L("VS_RENAMED.VSH");
	TParse parse;
	parse.Set(aName, NULL, NULL);
	parse.Set(parse.DriveAndPath(), &tmpFile, NULL);

	r = iFs.Rename(aName, parse.FullName());
	if(r != KErrNone)
		return r;

	RFile file;
	r = file.Open(iFs, parse.FullName(), EFileRead);
	if(r == KErrNone)
		{
		r = file.Size(size);
		}

	//If the file size is 0, then the file
	//has just been created - so it can't contain
	//a virus.
	if(r != KErrNone || size == 0)
		{
		file.Close();

		// Rename the file back
		TInt err = iFs.Rename(parse.FullName(), aName);
		if(err != KErrNone)
			return err;

		return r;
		}

	do
		{
		r = file.Read(pos, iScanBuf);
		
		if (r != KErrNone)
			{
			break;
			}

		r = ScanBuffer();
		pos += iScanBuf.Length();
		} while ((r == KErrNotFound) && (iScanBuf.Length() == iScanBuf.MaxLength()));

	file.Close();

	// Rename the file back
	TInt err = iFs.Rename(parse.FullName(), aName);
	if(err != KErrNone)
		return err;

	if (r > 0)
		{
		//We've found an infection
		return KErrAccessDenied;
		}
	else
		{
		return KErrNone;
		}
	}

/**
Scans the internal buffer which has been loaded with fresh
file contents, to see if it contains any known virus
signatures.

@return A value depending on whether a known virus signature
is found within the buffer.

@internalComponent
*/
TInt CTestVirusHook::ScanBuffer()
	{
	//Look through the internal buffer for all known virus signatures.

	TInt r;
	for (TInt i = 0; i < iSignaturesLoaded; i++)
		{
		r = iScanBuf.Find(iKnownSignatures[i]->Des());

		if (r != KErrNotFound)
			{
			return r;
			}
		}
	return KErrNone;
	}

/**
Validate that nobody is trying to touch the virus scanner files.

@internalComponent

@return A value depending on whethe the virus scanner files are
being fiddled with.

@param aDriveNum The drive number of the request which called into
the test virus scanning hook.

@param aName The full pathname of the file being accessed by the
request to the file server hook.
*/
TInt CTestVirusHook::ValidateRequest(TFsPluginRequest& aRequest, TFileName& aFileName)
	{
	TInt driveNumber = aRequest.DriveNumber();
	
	TInt err = GetName(&aRequest, aFileName);
	if(err != KErrNone)
		return(err);
	
	if (driveNumber == EDriveC)
		{
		TInt r = aFileName.Find(_L("\\virusdef.txt"));

		if (r != KErrNotFound)
			{
			//Do not allow the deletion of the virus definition file.
			return KErrAccessDenied;
			}

		r = aFileName.Find(_L("\\system\\libs\\t_vshook.pxt"));
		if (r != KErrNotFound)
			{
			//Do not allow the deletion of the this dll
			return KErrAccessDenied;
			}
		r = aFileName.Find(_L("\\sys\\bin\\t_vshook.pxt"));
		if (r != KErrNotFound)
			{
			//Do not allow the deletion of the this dll
			return KErrAccessDenied;
			}
		}
	return KErrNone;
	}

/**
Processes a message which describes the detection of an
infected file.  Appends the relevant text at the end of the
file to say that it has been "cleaned".  This allows the virus
test program to confirm that the test virus scanner is 
behaving as expected.

@internalComponent
@param aMessage The message to be processed.
*/
void CTestVirusHook::CleanFile(const TDesC& aName, TInt aOperation)
	{
	
	RFile infectedFile;
	TBool bChangedToRw=EFalse;
	TInt pos=0;

	TUint fileAtt;
	TInt r = iFs.Att(aName, fileAtt);
	if (r != KErrNone)
		{
		return;
		}

	if (fileAtt & KEntryAttReadOnly)
		{
		bChangedToRw = ETrue;
		r = iFs.SetAtt(aName, 0, KEntryAttReadOnly);
		}

	r = infectedFile.Open(iFs, aName, EFileShareAny | EFileWrite);

	if (r != KErrNone)
		{
		return;
		}

	//To show we've fixed the file, append "Infection deleted" to the end of it.
	infectedFile.Seek(ESeekEnd, pos);
	switch (aOperation)
		{
	case EFileOpen:
		infectedFile.Write(_L8("infection detected - file open\\n"));
		break;
	case EFileDelete:
		infectedFile.Write(_L8("infection detected - file delete\\n"));
		break;
	case EFileRename:
		infectedFile.Write(_L8("infection detected - file rename\\n"));
		break;
	case EFileClose:
		infectedFile.Write(_L8("infection detected - file close\\n"));
		break;
		}

	infectedFile.Close();

	if (bChangedToRw)
		{
		iFs.SetAtt(aName, KEntryAttReadOnly,0);
		}
	}

//factory functions

class CVsHookFactory : public CFsPluginFactory
	{
public:
	CVsHookFactory();
	virtual TInt Install();			
	virtual CFsPlugin* NewPluginL();
	virtual CFsPlugin* NewPluginConnL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CVsHookFactory::CVsHookFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
*/
TInt CVsHookFactory::Install()
	{
	iSupportedDrives = KPluginAutoAttach;

	_LIT(KVsHookName,"VsHook");
	return(SetName(&KVsHookName));
	}

/**
@internalComponent
*/
TInt CVsHookFactory::UniquePosition()
	{
	return(0x3EC);
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CVsHookFactory::NewPluginL()

	{
	return CTestVirusHook::NewL();
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CVsHookFactory::NewPluginConnL()

	{
	return CTestVirusHook::NewL();
	}

/**
Create a new Plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CVsHookFactory());
	}
}

