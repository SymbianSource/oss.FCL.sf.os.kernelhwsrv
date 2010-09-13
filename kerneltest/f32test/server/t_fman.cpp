// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#define __E32TEST_EXTENSION__

#include <f32file.h>
#include <e32test.h>
#include <hal.h>
#include "t_server.h"
#include "t_chlffs.h"

GLDEF_D RTest test(_L("T_FMAN"));

LOCAL_D CFileMan* gFileMan=NULL;
LOCAL_D TBool gAsynch=EFalse;
LOCAL_D TRequestStatus gStat;
LOCAL_D TBool testingInvalidPathLengths;
LOCAL_D TChar gSecDrive;        // a second drive for inter-drive tests
LOCAL_D TBool gSecDriveReady;

class CFileManObserver : public CBase, public MFileManObserver
	{
public:
	CFileManObserver(CFileMan* aFileMan);
	TControl NotifyFileManEnded();
private:
	CFileMan* iFileMan;
	};

LOCAL_D CFileManObserver* gObserver;

CFileManObserver::CFileManObserver(CFileMan* aFileMan)
//
// Constructor
//
	{
	__DECLARE_NAME(_S("CFileManObserver"));
	iFileMan=aFileMan;
	}

MFileManObserver::TControl CFileManObserver::NotifyFileManEnded()
//
// Called back after each FMan tick
//
	{
    (void) MFileManObserver::NotifyFileManEnded();
	TInt lastError=iFileMan->GetLastError();
	if (lastError!=KErrNone && lastError!=KErrBadName)
		{
		TFileName fileName=iFileMan->CurrentEntry().iName;
		if (gAsynch==EFalse)
			test.Printf(_L("CurrentEntry is %S\n"),&fileName);
		test_Equal(KErrAlreadyExists, lastError);
		test_Value(KErrNotFound, fileName.MatchF(_L("PIPE1.PLP"))!=KErrNotFound || fileName.MatchF(_L("FOUR"))!=KErrNotFound || fileName.MatchF(_L("File*.TXT"))!=KErrNotFound  || fileName.MatchF(_L("ah"))!=KErrNotFound || fileName.MatchF(_L("a"))!=KErrNotFound);
		}
	return(MFileManObserver::EContinue);
	}

LOCAL_C TBool GetSecondDrive(TChar& aDrive) // Get the drive that is ready.
    {
    TDriveList list;
    TheFs.DriveList(list);
    if (list[EDriveD] != 0)
        {
        aDrive = 'D';
        return ETrue;
        }
    
    TInt drv;
    // In minibsp rom(sirocco) there is no D drive
    for(drv = EDriveE; drv < EDriveZ; drv++)
        {
        if (list[drv] != 0) 
            {
            aDrive = 'A' + drv;
            return ETrue;
            }
        }
    aDrive = '?';
    return EFalse;
    }

LOCAL_C void WaitForSuccess()
//
// Wait for gStat to complete with KErrNone
//
	{
	User::WaitForRequest(gStat);
	test(gStat==KErrNone);
	}

LOCAL_C void WaitForResult(TInt aResult)
//
// Wait for gStat to complete with aResult
//
	{
	User::WaitForRequest(gStat);
	test_Value(aResult, gStat==aResult);
	}

LOCAL_C void TestResult(TInt aReturnVal, TInt aExpectedAsynchReturnStatus=KErrNone, TInt aExpectedSynchReturn=KErrNone)
//
// Test the result, wait for an asynchronous call
//
	{
	if (!gAsynch)
		{
		test_Equal(aExpectedAsynchReturnStatus, aReturnVal);
		}
	else
		{
		test_Equal(aExpectedSynchReturn, aReturnVal);
		WaitForResult(aExpectedAsynchReturnStatus);
		}
	}

LOCAL_C void RmDir(const TDesC& aDirName)
//
// Remove a directory
//
	{
	gFileMan->Attribs(aDirName, 0, KEntryAttReadOnly, 0, CFileMan::ERecurse);
	TInt r=gFileMan->RmDir(aDirName);
	test_Value(r, r==KErrNone || r==KErrNotFound || r==KErrPathNotFound);
	}

LOCAL_C void Compare(const TDesC& aDir1,const TDesC& aDir2)
//
// Test that the contents of two directories are identical
//
	{
	CDirScan* scanDir1=CDirScan::NewL(TheFs);
	scanDir1->SetScanDataL(aDir1,KEntryAttMaskSupported,ESortByName);
	CDirScan* scanDir2=CDirScan::NewL(TheFs);
	scanDir2->SetScanDataL(aDir2,KEntryAttMaskSupported,ESortByName);

	FOREVER
		{
		CDir* entryList1;
		CDir* entryList2;

		scanDir1->NextL(entryList1);
		scanDir2->NextL(entryList2);

		if (entryList1==NULL || entryList2==NULL)
			{
			test(entryList1==NULL && entryList2==NULL);
			break;
			}

		TFileName abbPath1=scanDir1->AbbreviatedPath();
		TFileName abbPath2=scanDir2->AbbreviatedPath();
		test(abbPath1==abbPath2);

		TInt count1=entryList1->Count();
		TInt count2=entryList2->Count();
		test(count1==count2);

		while(count1--)
			{
			TEntry entry1=(*entryList1)[count1];
			TEntry entry2=(*entryList2)[count1];
			test(entry1.iName==entry2.iName);
			test(entry1.iAtt==entry2.iAtt);
			}

		delete entryList1;
		delete entryList2;
		}

	delete scanDir1;
	delete scanDir2;
	}

LOCAL_C void SetupDirectories(TBool aCreateFiles, TFileName* aDestOtherDrive)
//
// Set up a directory structure and files to test copying/moving across drives
//
	{
	TInt err = KErrNone;

	TFileName sourceName		= _L("\\F32-TST\\TFMAN\\source\\");
	TFileName sourceNameSubDir	= _L("\\F32-TST\\TFMAN\\source\\subdir\\");
	TFileName sourceCompare		= _L("\\F32-TST\\TFMAN\\compare\\");	
	TFileName sourceCompareSubDir		= _L("\\F32-TST\\TFMAN\\compare\\subdir\\");	
	TFileName destSameDrive		= _L("\\F32-TST\\TFMAN\\dest\\");	// Target destination on the same drive

	if(aDestOtherDrive)
		{
#if !defined(__WINS__)
		*aDestOtherDrive = gSessionPath[0] == 'C' ? _L("D:\\F32-TST\\TFMAN\\dest\\") : _L("C:\\F32-TST\\TFMAN\\dest\\");
        (*aDestOtherDrive)[0] = (TText) gSecDrive;
#else
		*aDestOtherDrive = gSessionPath[0] == 'C' ? _L("Y:\\F32-TST\\TFMAN\\dest\\") : _L("C:\\F32-TST\\TFMAN\\dest\\");
#endif
		err = TheFs.MkDirAll(*aDestOtherDrive);
		test_Value(err, err == KErrNone || err == KErrAlreadyExists);
		}

	err = TheFs.MkDirAll(sourceName);
	test_Value(err, err == KErrNone || err == KErrAlreadyExists);

	err = TheFs.MkDirAll(sourceCompare);
	test_Value(err, err == KErrNone || err == KErrAlreadyExists);

	err = TheFs.MkDirAll(destSameDrive);
	test_Value(err, err == KErrNone || err == KErrAlreadyExists);

	if(aCreateFiles)
		{
		err = TheFs.MkDirAll(sourceNameSubDir);
		test_Value(err, err == KErrNone || err == KErrAlreadyExists);

		err = TheFs.MkDirAll(sourceCompareSubDir);
		test_Value(err, err == KErrNone || err == KErrAlreadyExists);
		
		for(TInt i=0; i<5; i++)
			{
			// Create a test file to be copied
			TFileName name = sourceName;
			name.Append(_L("File"));
			name.AppendNum(i);
			name.Append(_L(".TXT"));

			RFile file;
			err = file.Create(TheFs,name,EFileRead|EFileWrite);
			test_Value(err, err == KErrNone || err == KErrAlreadyExists);
			file.Close();

			// ...and another to compare against
			name = sourceCompare;
			name.Append(_L("File"));
			name.AppendNum(i);
			name.Append(_L(".TXT"));

			err = file.Create(TheFs,name,EFileRead|EFileWrite);
			test_Value(err, err == KErrNone || err == KErrAlreadyExists);
			file.Close();
			}
		}
	}

TBool CheckIfShortPathsAreSupported()
	{
	TBool ret = EFalse;
	TBuf<1+8+3+1+4> buf;
	_LIT(KTestFile, "\\longname1\\file");
	RmDir(_L("\\longname1\\"));
	MakeFile(KTestFile);
	TInt err = TheFs.GetShortName(_L("\\longname1\\"), buf);	
	if(err == KErrNone)
		{
		buf.Insert(0, _L("\\"));
		buf.Append(_L("\\file"));
		err = TheFs.Delete(buf);
		test_KErrNone(err);
  		ret = ETrue;
		}
	RmDir(_L("\\longname1\\"));
	return ret;
	}
	
LOCAL_C void TestDelete()
//
// Test files are deleted
//
	{
	test.Next(_L("Test delete - Set up files and start deleting"));

	MakeDir(_L("\\F32-TST\\TFMAN\\DELDIR\\DELTEST\\EMPTY\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE2.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE3.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\DELTEST\\FILE4.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\DELTEST\\EXE1.BIN"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\EXE2.BIN"));
	
	TInt r;
	// absolute path for code warrior two more than wins (\epoc32\winscw\c vs \epoc32\wins\c)
#if defined(__WINSCW__)
	_LIT(KLongName1,"\\TEST\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffd");
#else
	_LIT(KLongName1,"\\TEST\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa");
#endif

_LIT(KInvalidLongName,"\\TEST\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdffdsa23asdffdsa24asdffdsa25asdffdsa");
_LIT(KInvalidLongPath, "\\F32-TST\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\0495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\middle.gif");
	if (testingInvalidPathLengths)
//	Create a path of greater 256 characters by renaming a directory and check it can be
//	manipulated (tests fix to F32)		
		{
	//	One long directory name - makes paths invalid	
		MakeDir(_L("\\TEST\\LONG\\NAME\\ABCDE"));
		MakeDir(_L("\\TEST\\LONG\\NAME\\ABCDE\\DIRECTORY1DIRECTORY2DIRECTORY3DIRECTORY4\\"));
		MakeFile(_L("\\TEST\\LONG\\NAME\\ABCDE\\ELEPHANT01ELEPHANT02ELEPHANT03ELEPHANT04"));
		MakeFile(_L("\\TEST\\LONG\\NAME\\ABCDE\\FILEFILE01FILEFILE02FILEFILE03FILEFILE04"));
		TFileName name1(KLongName1);
		r=gFileMan->Rename(_L("\\TEST\\LONG"),name1,CFileMan::EOverWrite);
		test_KErrNone(r);
	//	Two long directory names - makes paths invalid
		MakeDir(_L("\\TEST\\LONG\\NAME\\FGHIJ"));
		MakeDir(_L("\\TEST\\LONG\\NAME\\FGHIJ\\DIRECTORY1DIRECTORY2DIRECTORY3DIRECTORY4\\"));
		MakeFile(_L("\\TEST\\LONG\\NAME\\FGHIJ\\ELEPHANT01ELEPHANT02ELEPHANT03ELEPHANT04"));
		MakeFile(_L("\\TEST\\LONG\\NAME\\FGHIJ\\FILEFILE01FILEFILE02FILEFILE03FILEFILE04"));
		
		// Testing invalid long file name (i.e. >256) 
		r=gFileMan->Rename(_L("\\TEST\\LONG"),KInvalidLongName,CFileMan::EOverWrite);
		test_Equal(KErrBadName, r);
		
		// Testing invalid long path (i.e. >256)
		r=gFileMan->Rename(_L("\\TEST\\LONG"),KInvalidLongPath,CFileMan::EOverWrite);
		test_Equal(KErrBadName, r);

		r=gFileMan->Rename(_L("\\TEST\\LONG"),_L("\\TEST\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20as\\fdsa21asdffds"),CFileMan::EOverWrite);
		test_KErrNone(r);
		}

	//testing invalid source path at the beginning:
	if (!gAsynch)
		{
		r=gFileMan->Delete(_L(":C\\F32-TST\\TFMAN\\DELDIR\\*.TXT"));
		}
	else
		{
		r=gFileMan->Delete(_L(":C\\F32-TST\\TFMAN\\DELDIR\\*.TXT"),0,gStat);
		}
	TestResult(r,KErrBadName,KErrBadName);

	//testing invalid source path at the middle:	
	if (!gAsynch)
		{
		r=gFileMan->Delete(_L("\\F32-TST\\TFMAN\\:DELDIR\\*.TXT"));
		}
	else
		{
		r=gFileMan->Delete(_L("\\F32-TST\\TFMAN\\:DELDIR\\*.TXT"),0,gStat);
		}
	TestResult(r,KErrBadName,KErrNone);

	//testing invalid source path at the end:	
	if (!gAsynch)
		{
		r=gFileMan->Delete(_L("\\F32-TST\\TFMAN\\DELDIR\\:*.TXT"));
		}
	else
		{
		r=gFileMan->Delete(_L("\\F32-TST\\TFMAN\\DELDIR\\:*.TXT"),0,gStat);
		}
	TestResult(r,KErrBadName,KErrNone);

	if (!gAsynch)
		{
		r=gFileMan->Delete(_L("\\F32-TST\\TFMAN\\DELDIR\\*.TXT"));
		TestResult(r);
		if (testingInvalidPathLengths)
			{
			TFileName name1(KLongName1);
			name1+=_L("\\NAME\\ABCDE\\*.*");
			r=gFileMan->Delete(name1);	
			test_KErrNone(r);

			r=gFileMan->Delete(_L("\\TEST\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20as\\fdsa21asdffds\\NAME\\FGHIJ\\*.*"));	
			test_KErrNone(r);
			}
		}
	else
		{
		gFileMan->Delete(_L("\\F32-TST\\TFMAN\\DELDIR\\*.TXT"),0,gStat);
		WaitForSuccess();
		if (testingInvalidPathLengths)
			{
			TFileName name1(KLongName1);
			name1+=_L("\\NAME\\ABCDE\\*.*");
			r=gFileMan->Delete(name1,0,gStat);	
			WaitForSuccess();
			test_KErrNone(r);
		
			r=gFileMan->Delete(_L("\\TEST\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20as\\fdsa21asdffds\\NAME\\FGHIJ\\*.*"),0,gStat);	
			WaitForSuccess();
			test_KErrNone(r);
			}
		}

	test.Next(_L("Check files are deleted"));
	RmDir(_L("\\F32-TST\\TFMAN\\After\\"));
	MakeDir(_L("\\F32-TST\\TFMAN\\AFTER\\DELDIR\\DELTEST\\EMPTY\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\DELDIR\\DELTEST\\EXE1.BIN"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\DELDIR\\DELTEST\\FILE4.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\DELDIR\\EXE2.BIN"));
	Compare(_L("\\F32-TST\\TFMAN\\DELDIR\\*"),_L("\\F32-TST\\TFMAN\\AFTER\\DELDIR\\*"));
	
	if (testingInvalidPathLengths)
		{
		r=gFileMan->RmDir(_L("\\TEST\\"));
		test_KErrNone(r);
		}

	/**
	Test wild card matching in short file names
	Note this test is only run on FAT file systems as 'short file names' are only
		supported by FAT.
	DEF130113: TTG:<Wild card characters cannot be handled in the short file names> 
	*/ 
	TInt theDrive; 
	r=TheFs.CharToDrive(gDriveToTest,theDrive);
	test_KErrNone(r);
    TFSName f;
	r = TheFs.FileSystemName(f, theDrive);
	test_Value(r, r == KErrNone || r == KErrNotFound);
    if (f.FindF(_L("Fat")) == 0 )
    	{
		test.Next(_L("Test wild card matching in short file names"));
    	MakeFile(_L("abcdefghi.txt"));
    	TInt err = gFileMan->Delete(_L("ABCDEF~*"));
    	test_KErrNone(err);
    	MakeFile(_L("abcdefghi.txt"));
    	err = gFileMan->Delete(_L("ABCDEF~*.TXT"));
    	test_KErrNone(err);
    	MakeFile(_L("abcdefghi.txt"));
    	err = gFileMan->Delete(_L("ABCDEF~*.?XT"));
    	test_KErrNone(err);
    	MakeFile(_L("abcdefghi.txt"));
    	err = gFileMan->Delete(_L("ABCDEF~1.*"));
    	test_KErrNone(err);
    	}
	}

LOCAL_C void TestCopy()
//
// Test copy
//
	{
	test.Next(_L("Test copy"));
	RmDir(_L("\\F32-TST\\TFMAN\\COPYDIR\\*"));

	MakeDir(_L("\\F32-TST\\TFMAN\\COPYDIR\\"));
	MakeDir(_L("\\F32-TST\\TFMAN\\DELDIR\\DELTEST\\EMPTY\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\NewDir\\ABC.DEF"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE2.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE3.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\DELTEST\\FILE4.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\DELTEST\\EXE1.BIN"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\EXE2.BIN"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\RUMBA1.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\RUMBA2.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\RUMBA3.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\RUMBA4.TXT"));

	test.Next(_L("Test copy files to the same directory"));
	TInt r;
	
	if (testingInvalidPathLengths)
		// Create a path of greater than 256 characters by renaming a directory and
		// check it can be manipulated (tests fix to F32)
		{
		MakeDir(_L("\\START\\LONG\\"));
		MakeDir(_L("\\FINISH\\"));
		MakeFile(_L("\\START\\LONG\\ABCDEFGH01ABCDEFGH01ABCDEFGH01ABCDEFGH01.txt"));
		MakeFile(_L("\\START\\LONG\\ELEPHANT01ELEPHANT02ELEPHANT03ELEPHANT04.txt"));
		MakeFile(_L("\\START\\LONG\\DINOSAUR01DINOSAUR02DINOSAUR03DINOSAUR04.txt"));
		MakeFile(_L("\\START\\LONG\\FILEFILE01FILEFILE02FILEFILE03FILEFILE04.txt"));
		r=gFileMan->Rename(_L("\\START\\LONG"),_L("\\START\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdff"),CFileMan::EOverWrite);
		test_KErrNone(r);
		MakeDir(_L("\\START\\ASDFFDSA\\"));
		}

	//testing invalid source path at the beginning:
	if (!gAsynch)
		{
		r=gFileMan->Copy(_L(":C:\\F32-TST\\TFMAN\\DELDIR\\file?.txt"),_L("\\F32-TST\\TFMAN\\DELDIR\\rumba?.txt"),0);
		}
	else
		{
		r=gFileMan->Copy(_L(":C:\\F32-TST\\TFMAN\\DELDIR\\file?.txt"),_L("\\F32-TST\\TFMAN\\DELDIR\\rumba?.txt"),0,gStat);
		}
	TestResult(r,KErrBadName,KErrBadName);
		
	//testing invalid target path at the beginning:  
		
	if (!gAsynch)
		{
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\file?.txt"),_L(":C:\\F32-TST\\TFMAN\\DELDIR\\rumba?.txt"),0);
		}
	else
		{
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\file?.txt"),_L(":C:\\F32-TST\\TFMAN\\DELDIR\\rumba?.txt"),0,gStat);
		}
	TestResult(r,KErrBadName,KErrBadName);
  
	//testing invalid source path at the middle:	
	if (!gAsynch)
		{
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\:DELDIR\\file?.txt"),_L("\\F32-TST\\TFMAN\\DELDIR\\rumba?.txt"),0);
		}
	else
		{
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\:DELDIR\\file?.txt"),_L("\\F32-TST\\TFMAN\\DELDIR\\rumba?.txt"),0,gStat);
		}
	TestResult(r,KErrBadName,KErrNone);
	
	//testing invalid target path at the middle:	
	if (!gAsynch)
		{
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\file?.txt"),_L("\\F32-TST\\TFMAN\\:DELDIR\\rumba?.txt"),0);
		}
	else
		{
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\file?.txt"),_L("\\F32-TST\\TFMAN\\:DELDIR\\rumba?.txt"),0,gStat);
		}
	TestResult(r,KErrBadName,KErrNone);

	//testing invalid source path at the end:	
	if (!gAsynch)
		{
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\:file?.txt"),_L("\\F32-TST\\TFMAN\\:DELDIR\\rumba?.txt"),0);
		}
	else
		{
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\:file?.txt"),_L("\\F32-TST\\TFMAN\\:DELDIR\\rumba?.txt"),0,gStat);
		}
	TestResult(r,KErrBadName,KErrNone);	
	
	//testing invalid target path at the end:	
	if (!gAsynch)
		{
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\file?.txt"),_L("\\F32-TST\\TFMAN\\:DELDIR\\:rumba?.txt"),0);
		}
	else
		{
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\file?.txt"),_L("\\F32-TST\\TFMAN\\:DELDIR\\:rumba?.txt"),0,gStat);
		}
	TestResult(r,KErrBadName,KErrNone);
		
	if (!gAsynch)
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\file?.txt"),_L("\\F32-TST\\TFMAN\\DELDIR\\rumba?.txt"),0);
	else
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\file?.txt"),_L("\\F32-TST\\TFMAN\\DELDIR\\rumba?.txt"),0,gStat);
	TestResult(r,KErrAlreadyExists);

	if (!gAsynch)
		r = gFileMan->Delete(_L("\\F32-TST\\TFMAN\\DELDIR\\rumba.txt"),0);
	else
		r = gFileMan->Delete(_L("\\F32-TST\\TFMAN\\DELDIR\\rumba.txt"),0,gStat);
	TestResult(r,KErrNone);

	if (!gAsynch)
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.txt"),_L("\\F32-TST\\TFMAN\\DELDIR\\file1.txt"),0);
	else
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.txt"),_L("\\F32-TST\\TFMAN\\DELDIR\\file1.txt"),0,gStat);
	TestResult(r,KErrAlreadyExists);

	if (!gAsynch)
		{
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\*.TXT"),_L("\\F32-TST\\TFMAN\\COPYDIR\\"));
		test_KErrNone(r);
		if (testingInvalidPathLengths)
			{
			test.Next(_L("Test invalid length paths"));
			r=gFileMan->Copy(_L("\\START\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdff\\*.*"),_L("\\FINISH\\"));
			test_KErrNone(r);
			r=gFileMan->RmDir(_L("\\START\\"));
			test_KErrNone(r);
			r=gFileMan->RmDir(_L("\\FINISH\\"));
			test_KErrNone(r);
			}
		}
	else
		{
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\*.TXT"),_L("\\F32-TST\\TFMAN\\COPYDIR\\"),0,gStat);
		WaitForSuccess();
		test_KErrNone(r);
		if (testingInvalidPathLengths)
			{
			test.Next(_L("Test invalid length paths (Asynch)"));
			r=gFileMan->Copy(_L("\\START\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdff\\*.*"),_L("\\FINISH\\"),0,gStat);
			WaitForSuccess();
			test_KErrNone(r);
			r=gFileMan->RmDir(_L("\\START\\"),gStat);
			WaitForSuccess();
			test_KErrNone(r);
			r=gFileMan->RmDir(_L("\\FINISH\\"),gStat);
			WaitForSuccess();
			test_KErrNone(r);
			}
		}
	
	if (!gAsynch)
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\NewDir\\*.*"),_L("\\F32-TST\\TFMAN\\COPYDIR\\"));
	else
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\NewDir\\*.*"),_L("\\F32-TST\\TFMAN\\COPYDIR\\"),0,gStat);
	TestResult(r);

	test.Next(_L("Check files have been copied"));
	RmDir(_L("\\F32-TST\\TFMAN\\after\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\FILE1.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\FILE2.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\FILE3.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\RUMBA1.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\RUMBA2.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\RUMBA3.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\RUMBA4.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\ABC.DEF"));
	Compare(_L("\\F32-TST\\TFMAN\\AFTER\\*"),_L("\\F32-TST\\TFMAN\\COPYDIR\\*"));

	TFileName fn = _L("Z:\\TEST\\T_FSRV.CPP");
	fn[0] = gExeFileName[0];
	if (!gAsynch)
		r=gFileMan->Copy(fn,_L("\\F32-TST\\TFMAN\\COPYDIR\\"));
	else
		r=gFileMan->Copy(fn,_L("\\F32-TST\\TFMAN\\COPYDIR\\"),0,gStat);
	TestResult(KErrNone);

	TEntry entry;
	r=TheFs.Entry(_L("\\F32-TST\\TFMAN\\COPYDIR\\T_FSRV.CPP"),entry);
	test_KErrNone(r);
	test(entry.iName.MatchF(_L("T_FSRV.CPP"))!=KErrNotFound);
#if defined (__WINS__)
	test_Equal(KEntryAttArchive | KEntryAttReadOnly, entry.iAtt);
#else
	if (!IsTestingLFFS())
		{
	    test_Equal(KEntryAttReadOnly, entry.iAtt);
		}
	else
		{
		test(entry.iAtt&KEntryAttReadOnly); // ???
		}
#endif
	r=TheFs.SetAtt(_L("\\F32-TST\\TFMAN\\COPYDIR\\T_FSRV.CPP"),0,KEntryAttReadOnly);
	test_KErrNone(r);

	r=gFileMan->Delete(_L("\\F32-TST\\TFMAN\\AFTER\\RUMBA?.TXT"));
	test_KErrNone(r);
	r=gFileMan->Delete(_L("\\F32-TST\\TFMAN\\DELDIR\\RUMBA?.TXT"));
	test_KErrNone(r);
	}

LOCAL_C void TestDEF121663_Setup(TFileName& aSrcPath)
	{
	RmDir(aSrcPath);
	MakeDir(aSrcPath);
	
	for(TInt index=0; index<10; index++)
		{
		TFileName fileName;
	    fileName.Copy(aSrcPath);
	    fileName.Append(_L("FILE_"));fileName.AppendNum(index);fileName.Append(_L(".TXT"));
		MakeFile(fileName, _L8("Some Data"));
		}
	}
	
LOCAL_C void TestDEF121663()
	{
	test.Next(_L("Test moving directory to its subdirectory (DEF121663)"));
	
	gFileMan->SetObserver(NULL);
	TInt err = 0;
	TFileName srcPath = _L("C:\\TestDEF121663\\");
		
	TestDEF121663_Setup(srcPath);
	if(!gAsynch)
		{
		err = gFileMan->Move(_L("C:\\TestDEF121663"),_L("C:\\TestDEF121663\\TestDEF121663"),CFileMan::EOverWrite);
		}
	else
		{
		err = gFileMan->Move(_L("C:\\TestDEF121663"),_L("C:\\TestDEF121663\\TestDEF121663"),CFileMan::EOverWrite, gStat);
		}
	TestResult(err,KErrInUse,KErrInUse);

	TestDEF121663_Setup(srcPath);
	if(!gAsynch)
		{
		err = gFileMan->Move(_L("C:\\TestDEF121663\\"),_L("C:\\TestDEF121663\\TestDEF121663\\"),CFileMan::EOverWrite);
		}
	else
		{
		err = gFileMan->Move(_L("C:\\TestDEF121663\\"),_L("C:\\TestDEF121663\\TestDEF121663\\"),CFileMan::EOverWrite, gStat);
		}
	TestResult(err,KErrPathNotFound);
	
	TestDEF121663_Setup(srcPath);
	if(!gAsynch)
		{
		err = gFileMan->Move(_L("C:\\TestDEF121663\\"),_L("C:\\TestDEF121663\\TestDEF121663"),CFileMan::EOverWrite);
		}
	else
		{
		err = gFileMan->Move(_L("C:\\TestDEF121663\\"),_L("C:\\TestDEF121663\\TestDEF121663"),CFileMan::EOverWrite, gStat);
		}
	TestResult(err,KErrPathNotFound);

	TestDEF121663_Setup(srcPath);
	if(!gAsynch)
		{
		err = gFileMan->Move(_L("C:\\TestDEF121663"),_L("C:\\TestDEF121663\\TestDEF121663\\"),CFileMan::ERecurse|CFileMan::EOverWrite);
		}
	else
		{
		err = gFileMan->Move(_L("C:\\TestDEF121663"),_L("C:\\TestDEF121663\\TestDEF121663\\"),CFileMan::ERecurse|CFileMan::EOverWrite, gStat);
		}
	TestResult(err,KErrInUse,KErrInUse);

	TestDEF121663_Setup(srcPath);
	if(!gAsynch)
		{
		err = gFileMan->Move(_L("C:\\TestDEF121663"),_L("C:\\TestDEF121663\\TestDEF121663"),CFileMan::ERecurse);
		}
	else
		{
		err = gFileMan->Move(_L("C:\\TestDEF121663"),_L("C:\\TestDEF121663\\TestDEF121663"),CFileMan::ERecurse, gStat);
		}
	TestResult(err,KErrInUse,KErrInUse);
	
	gFileMan->SetObserver(gObserver);
	// remove previous dirs
	RmDir(_L("C:\\TestDEF121663\\"));
	}

// Test moving directories where source and target have matching subdirectory structures
LOCAL_C void TestDEF123575()
	{
	test.Next(_L("Test moving directories with matching subdirectory structures (DEF123575)"));
	TFileName srcPath;
	TFileName destPath;
	TInt err;
	//setup the initial directory structure
	srcPath = _L("\\F32-TST\\DEF123575\\SRCDIR\\CommonDIR\\temp\\temp1.1\\");
	destPath = _L("\\F32-TST\\DEF123575\\DSTDIR\\CommonDIR\\temp\\temp1.1\\");
	MakeDir(srcPath);
	MakeDir(destPath);
	MakeFile(_L("\\F32-TST\\DEF123575\\SRCDIR\\CommonDIR\\temp\\temp1.1\\FILE1.TXT"));
	
	srcPath = _L("\\F32-TST\\DEF123575\\SRCDIR\\CommonDIR");
	destPath = _L("\\F32-TST\\DEF123575\\DSTDIR\\");
	if(!gAsynch)
		{
		err = gFileMan->Move(srcPath,destPath,CFileMan::EOverWrite);
		}
	else
		{
		err = gFileMan->Move(srcPath,destPath,CFileMan::EOverWrite, gStat);
		}
	TestResult(err,KErrNone,KErrNone);

	//test that source directory is empty after move
	MakeDir(_L("\\F32-TST\\DEF123575\\AFTER\\"));
	Compare(_L("\\F32-TST\\DEF123575\\SRCDIR\\*"),_L("\\F32-TST\\DEF123575\\AFTER\\*"));
	//test that the files have been moved to the destination directory
	MakeDir(_L("\\F32-TST\\DEF123575\\AFTER\\CommonDIR\\temp\\temp1.1\\"));
	MakeFile(_L("\\F32-TST\\DEF123575\\AFTER\\CommonDIR\\temp\\temp1.1\\FILE1.TXT"));
	Compare(_L("\\F32-TST\\DEF123575\\DSTDIR\\*"),_L("\\F32-TST\\DEF123575\\AFTER\\*"));
	
	//delete the entire directory structure
	RmDir(_L("\\F32-TST\\DEF123575\\*"));
	}

LOCAL_C void TestDEF125570()
	{
	test.Next(_L("Test move when trg has at least one of the src dirs (DEF125570)"));
	gFileMan->SetObserver(NULL);
	TInt err = KErrNone; 
	TFileName srcPath = _L("C:\\TestDEF125570\\src\\");
	TFileName trgPath = _L("C:\\TestDEF125570\\trg\\");

	// remove previous dirs
	RmDir(srcPath);
	RmDir(trgPath);

	//create src
	MakeDir(_L("C:\\TestDEF125570\\src\\DIR1\\"));
	MakeFile(_L("C:\\TestDEF125570\\src\\DIR1\\File1.txt"),_L8("FILE PATH : C:\\TestDEF125570\\SRC\\DIR1\\File1.txt"));
	MakeFile(_L("C:\\TestDEF125570\\src\\DIR1\\File2.txt"),_L8("FILE PATH : C:\\TestDEF125570\\SRC\\DIR1\\File2.txt"));
	MakeDir(_L("C:\\TestDEF125570\\src\\DIR1\\DIR11\\"));
	MakeFile(_L("C:\\TestDEF125570\\src\\DIR1\\DIR11\\File1.txt"),_L8("FILE PATH : C:\\TestDEF125570\\SRC\\DIR1\\DIR11\\File1.txt"));
	MakeFile(_L("C:\\TestDEF125570\\src\\DIR1\\DIR11\\File2.txt"),_L8("FILE PATH : C:\\TestDEF125570\\SRC\\DIR1\\DIR11\\File2.txt"));
	MakeDir(_L("C:\\TestDEF125570\\src\\DIR2\\"));
	MakeFile(_L("C:\\TestDEF125570\\src\\DIR2\\File1.txt"),_L8("FILE PATH : C:\\TestDEF125570\\SRC\\DIR2\\File1.txt"));
	MakeFile(_L("C:\\TestDEF125570\\src\\DIR2\\File2.txt"),_L8("FILE PATH : C:\\TestDEF125570\\SRC\\DIR2\\File2.txt"));
	MakeDir(_L("C:\\TestDEF125570\\src\\DIR2\\DIR12\\"));
	MakeFile(_L("C:\\TestDEF125570\\src\\DIR2\\DIR12\\File1.txt"),_L8("FILE PATH : C:\\TestDEF125570\\SRC\\DIR2\\DIR12\\File1.txt"));
	MakeFile(_L("C:\\TestDEF125570\\src\\DIR2\\DIR12\\File2.txt"),_L8("FILE PATH : C:\\TestDEF125570\\SRC\\DIR2\\DIR12\\File2.txt"));

	//trg has at least one of the src subfolders
	MakeDir(_L("C:\\TestDEF125570\\trg\\DIR2\\"));
	MakeFile(_L("C:\\TestDEF125570\\trg\\DIR2\\File1.txt"),_L8("FILE PATH : C:\\TestDEF125570\\TRG\\DIR2\\File1.txt"));
	MakeFile(_L("C:\\TestDEF125570\\trg\\DIR2\\File2.txt"),_L8("FILE PATH : C:\\TestDEF125570\\TRG\\DIR2\\File2.txt"));

	if(!gAsynch)
		err = gFileMan->Move(srcPath, trgPath, CFileMan::ERecurse|CFileMan::EOverWrite);
	else
		err = gFileMan->Move(srcPath, trgPath, CFileMan::ERecurse|CFileMan::EOverWrite, gStat);
	TestResult(err);
	gFileMan->SetObserver(gObserver);
	// remove previous dirs
	RmDir(_L("C:\\TestDEF125570\\"));
	}

LOCAL_C void TestDEF130404()
	{
	test.Printf(_L("Test move when the src doesn't fully exist (DEF130404)"));
	
	TInt r = 0;
	TFileName trgPath;
	trgPath.Format(_L("%c:\\TestDEF130404\\Trg\\"), (TUint8)gDriveToTest);
	TFileName srcPath;
	srcPath.Format(_L("C:\\TestDEF130404\\Src\\DIR1\\"), (TUint8)gDriveToTest);

	// clean up before testing
	RmDir(srcPath);
	RmDir(trgPath);
	
	MakeDir(srcPath);
	srcPath.Append(_L("NODIR\\*.*"));
	MakeDir(trgPath);
	
	if(!gAsynch)
		r = gFileMan->Move(srcPath, trgPath, 0);
	else
	 	r = gFileMan->Move(srcPath, trgPath, 0, gStat);
	TestResult(r,KErrPathNotFound);
	
	// clean up before leaving
	trgPath.Format(_L("%c:\\TestDEF130404\\"), (TUint8)gDriveToTest);
	RmDir(trgPath);
	RmDir(_L("C:\\TestDEF130404\\"));
	}


/**
This is to test that moving files to overwrite folders with the same names
and moving folders (directories) to overwrite files with the same names
across drives return proper error codes
*/
void TestPDEF137716()
	{
	// Do not run tests if we cannot move across different drives
	if (gSessionPath[0]=='C')
		return; 

	// Move FILE to overwrite FOLDER --------------------------------------------------------
	test.Next(_L("Test moving files to overwrite folders with the same names"));
	gFileMan->SetObserver(NULL);

	_LIT(KFixedTargetTestFolder,		"\\PDEF137716\\");
	_LIT(KFileToDirTargetCreatePath,	"\\PDEF137716\\FileToDir_Target\\ITEM\\");
	_LIT(KFileToDirTargetNameWild,		"\\PDEF137716\\FileToDir_Target\\");

	_LIT(KFixedSourceTestFolder,		"C:\\PDEF137716\\");
	_LIT(KFileToDirSourceName,			"C:\\PDEF137716\\FileToDir_Source\\ITEM");
	_LIT(KFileToDirSourceNameWild,		"C:\\PDEF137716\\FileToDir_Source\\");

	RmDir(KFixedTargetTestFolder);
	RmDir(KFixedSourceTestFolder);
	MakeDir(KFileToDirTargetCreatePath);
	MakeFile(KFileToDirSourceName);
	TInt err = KErrNone;

	if(!gAsynch)
		{
		err = gFileMan->Move(KFileToDirSourceName, KFileToDirTargetNameWild, 0);
		}
	else
		{
		err = gFileMan->Move(KFileToDirSourceName, KFileToDirTargetNameWild, 0, gStat);
		}
	TestResult(err,KErrAccessDenied);

	RmDir(KFixedTargetTestFolder);
	RmDir(KFixedSourceTestFolder);
	MakeDir(KFileToDirTargetCreatePath);
	MakeFile(KFileToDirSourceName);	
	if(!gAsynch)
		{
		err = gFileMan->Move(KFileToDirSourceName, KFileToDirTargetNameWild, CFileMan::EOverWrite);
		}
	else
		{
		err = gFileMan->Move(KFileToDirSourceName, KFileToDirTargetNameWild, CFileMan::EOverWrite, gStat);
		}
	TestResult(err,KErrAccessDenied);

	RmDir(KFixedTargetTestFolder);
	RmDir(KFixedSourceTestFolder);
	MakeDir(KFileToDirTargetCreatePath);
	MakeFile(KFileToDirSourceName);
	if(!gAsynch)
		{
		err = gFileMan->Move(KFileToDirSourceNameWild, KFileToDirTargetNameWild, 0);
		}
	else
		{
		err = gFileMan->Move(KFileToDirSourceNameWild, KFileToDirTargetNameWild, 0, gStat);
		}
	TestResult(err,KErrAccessDenied);

	RmDir(KFixedTargetTestFolder);
	RmDir(KFixedSourceTestFolder);
	MakeDir(KFileToDirTargetCreatePath);
	MakeFile(KFileToDirSourceName);
	if(!gAsynch)
		{
		err = gFileMan->Move(KFileToDirSourceNameWild, KFileToDirTargetNameWild, CFileMan::EOverWrite);
		}
	else
		{
		err = gFileMan->Move(KFileToDirSourceNameWild, KFileToDirTargetNameWild, CFileMan::EOverWrite, gStat);
		}
	TestResult(err,KErrAccessDenied);

	
	// Move FOLDER to overwrite FILE --------------------------------------------------------
	test.Next(_L("Test moving folders to overwrite files with the same names"));
	
	_LIT(KDirToFileTargetName,			"\\PDEF137716\\DirToFile_Target\\ITEM");
	_LIT(KDirToFileTargetNameWild,		"\\PDEF137716\\DirToFile_Target\\");
	
	_LIT(KDirToFileSourceName,			"C:\\PDEF137716\\DirToFile_Source\\ITEM");
	_LIT(KDirToFileSourceNameWild,		"C:\\PDEF137716\\DirToFile_Source\\");

	_LIT(KDirToFileSourceCreatePath,	"C:\\PDEF137716\\DirToFile_Source\\ITEM\\");

	RmDir(KFixedTargetTestFolder);
	RmDir(KFixedSourceTestFolder);
	MakeFile(KDirToFileTargetName);
	MakeDir(KDirToFileSourceCreatePath);
	if(!gAsynch)
		{
		err = gFileMan->Move(KDirToFileSourceName, KDirToFileTargetName, 0);
		}
	else
		{
		err = gFileMan->Move(KDirToFileSourceName, KDirToFileTargetName, 0, gStat);
		}
	TestResult(err,KErrAccessDenied,KErrAccessDenied);

	RmDir(KFixedTargetTestFolder);
	RmDir(KFixedSourceTestFolder);
	MakeFile(KDirToFileTargetName);
	MakeDir(KDirToFileSourceCreatePath);
	if(!gAsynch)
		{
		err = gFileMan->Move(KDirToFileSourceName, KDirToFileTargetName, CFileMan::EOverWrite);
		}
	else
		{
		err = gFileMan->Move(KDirToFileSourceName, KDirToFileTargetName, CFileMan::EOverWrite, gStat);
		}
	TestResult(err,KErrAccessDenied,KErrAccessDenied);

	RmDir(KFixedTargetTestFolder);
	RmDir(KFixedSourceTestFolder);
	MakeFile(KDirToFileTargetName);
	MakeDir(KDirToFileSourceCreatePath);
	if(!gAsynch)
		{
		err = gFileMan->Move(KDirToFileSourceName, KDirToFileTargetNameWild, 0);
		}
	else
		{
		err = gFileMan->Move(KDirToFileSourceName, KDirToFileTargetNameWild, 0, gStat);
		}
	TestResult(err,KErrAccessDenied,KErrAccessDenied);

	RmDir(KFixedTargetTestFolder);
	RmDir(KFixedSourceTestFolder);
	MakeFile(KDirToFileTargetName);
	MakeDir(KDirToFileSourceCreatePath);
	if(!gAsynch)
		{
		err = gFileMan->Move(KDirToFileSourceName, KDirToFileTargetNameWild, CFileMan::EOverWrite);
		}
	else
		{
		err = gFileMan->Move(KDirToFileSourceName, KDirToFileTargetNameWild, CFileMan::EOverWrite, gStat);
		}
	TestResult(err,KErrAccessDenied,KErrAccessDenied);

	RmDir(KFixedTargetTestFolder);
	RmDir(KFixedSourceTestFolder);
	MakeFile(KDirToFileTargetName);
	MakeDir(KDirToFileSourceCreatePath);
	err = gFileMan->Move(KDirToFileSourceNameWild, KDirToFileTargetNameWild, 0);
	if(!gAsynch)
		{
		err = gFileMan->Move(KDirToFileSourceNameWild, KDirToFileTargetNameWild, 0);
		}
	else
		{
		err = gFileMan->Move(KDirToFileSourceNameWild, KDirToFileTargetNameWild, 0, gStat);
		}
	TestResult(err,KErrNotFound);

	RmDir(KFixedTargetTestFolder);
	RmDir(KFixedSourceTestFolder);
	MakeFile(KDirToFileTargetName);
	MakeDir(KDirToFileSourceCreatePath);
	if(!gAsynch)
		{
		err = gFileMan->Move(KDirToFileSourceNameWild, KDirToFileTargetNameWild, CFileMan::EOverWrite);
		}
	else
		{
		err = gFileMan->Move(KDirToFileSourceNameWild, KDirToFileTargetNameWild, CFileMan::EOverWrite, gStat);
		}
	TestResult(err,KErrNotFound);

	RmDir(KFixedTargetTestFolder);
	RmDir(KFixedSourceTestFolder);
	gFileMan->SetObserver(gObserver);
	}

LOCAL_C void TestMove()
//
// Test Move
//
	{
	test.Next(_L("Test move"));
	RmDir(_L("\\F32-TST\\TFMAN\\MOVEDIR\\*"));

	MakeDir(_L("\\F32-TST\\TFMAN\\MOVEDIR\\"));
	MakeDir(_L("\\F32-TST\\TFMAN\\DELDIR\\DELTEST\\EMPTY\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE2.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE3.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\DELTEST\\FILE4.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\DELTEST\\EXE1.BIN"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\EXE2.BIN"));

	TInt r=KErrNone;

	if (testingInvalidPathLengths)
		//	Create a path of greater 256 characters by renaming a directory and check it can be
		//	manipulated (tests fix to F32)		
		{
		MakeDir(_L("\\START\\LONG\\"));
		MakeDir(_L("\\FINISH\\"));
		MakeFile(_L("\\START\\LONG\\ABCDEFGH01ABCDEFGH01ABCDEFGH01ABCDEFGH01.txt"));
		MakeFile(_L("\\START\\LONG\\ELEPHANT01ELEPHANT02ELEPHANT03ELEPHANT04.txt"));
		MakeFile(_L("\\START\\LONG\\DINOSAUR01DINOSAUR02DINOSAUR03DINOSAUR04.txt"));
		MakeFile(_L("\\START\\LONG\\FILEFILE01FILEFILE02FILEFILE03FILEFILE04.txt"));
		r=gFileMan->Rename(_L("\\START\\LONG"),_L("\\START\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdff"),CFileMan::EOverWrite);
		test_KErrNone(r);

		//	Two long directory names - makes paths invalid
		MakeDir(_L("\\TEST\\LONG\\NAME\\FGHIJ"));
		MakeDir(_L("\\TEST\\LONG\\NAME\\FGHIJ\\DIRECTORY1DIRECTORY2DIRECTORY3DIRECTORY4\\"));
		MakeFile(_L("\\TEST\\LONG\\NAME\\FGHIJ\\ELEPHANT01ELEPHANT02ELEPHANT03ELEPHANT04"));
		MakeFile(_L("\\TEST\\LONG\\NAME\\FGHIJ\\FILEFILE01FILEFILE02FILEFILE03FILEFILE04"));
		r=gFileMan->Rename(_L("\\TEST\\LONG"),_L("\\TEST\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20as\\fdsa21asdffds"),CFileMan::EOverWrite);
		test_KErrNone(r);
	
		MakeDir(_L("\\START\\ASDFFDSA\\"));
		}

	//testing invalid source path at the beginning:
	if (!gAsynch)
		{
		r=gFileMan->Move(_L(":C:\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"),_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"));
		}
	else
		{
		r=gFileMan->Move(_L(":C:\\F32-TST\\TFMAN\\DELDIR\\FILE?.TXT"),_L("\\F32-TST\\TFMAN\\DELDIR\\FILE*.TXT"),0,gStat);
		}
	TestResult(r,KErrBadName,KErrBadName);
		
	//testing invalid target path at the beginning:
	if (!gAsynch)
		{
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"),_L(":C:\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"));
		}
	else
		{
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE?.TXT"),_L(":C:\\F32-TST\\TFMAN\\DELDIR\\FILE*.TXT"),0,gStat);
		}
	TestResult(r,KErrBadName,KErrBadName);

	//testing invalid source path at the middle:	
	if (!gAsynch)
		{
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\:DELDIR\\FILE1.TXT"),_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"));
		}
	else
		{
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\:DELDIR\\FILE?.TXT"),_L("\\F32-TST\\TFMAN\\DELDIR\\FILE*.TXT"),0,gStat);
		}
	TestResult(r,KErrBadName,KErrNone);
	
	//testing invalid target path at the middle:	
	if (!gAsynch)
		{
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"),_L("\\F32-TST\\TFMAN\\:DELDIR\\FILE1.TXT"));
		}
	else
		{
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE?.TXT"),_L("\\F32-TST\\TFMAN\\:DELDIR\\FILE*.TXT"),0,gStat);
		}
	TestResult(r,KErrBadName,KErrNone);

	//testing invalid source path at the end:	
	if (!gAsynch)
		{
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\:FILE1.TXT"),_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"));
		}
	else
		{
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\:FILE?.TXT"),_L("\\F32-TST\\TFMAN\\DELDIR\\FILE*.TXT"),0,gStat);
		}
	TestResult(r,KErrBadName,KErrNone);
	
	//testing invalid target path at the end:	
	if (!gAsynch)
		{
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"),_L("\\F32-TST\\TFMAN\\DELDIR\\:FILE1.TXT"));
		}
	else
		{
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE?.TXT"),_L("\\F32-TST\\TFMAN\\DELDIR\\:FILE*.TXT"),0,gStat);
		}
	TestResult(r,KErrBadName,KErrNone);	
	

	if (!gAsynch)
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE?.TXT"),_L("\\F32-TST\\TFMAN\\DELDIR\\FILE*.TXT"));
	else
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE?.TXT"),_L("\\F32-TST\\TFMAN\\DELDIR\\FILE*.TXT"),0,gStat);
	TestResult(r,KErrNone);

	if ((!gAsynch)&&(testingInvalidPathLengths))
		{
		r=gFileMan->Move(_L("\\START\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdff\\*.*"),_L("\\FINISH\\"));
		test_KErrNone(r);
		
		r=gFileMan->Move(_L("\\TEST\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20as\\fdsa21asdffds\\*.*"),_L("\\FINISH\\"), CFileMan::EOverWrite | CFileMan::ERecurse);
		test_KErrNone(r);

		r=gFileMan->RmDir(_L("\\START\\"));
		test_KErrNone(r);
		r=gFileMan->RmDir(_L("\\FINISH\\"));
		test_KErrNone(r);
		}
	if ((gAsynch)&&(testingInvalidPathLengths))
		{
		r=gFileMan->Move(_L("\\START\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdff\\*.*"),_L("\\FINISH\\"),CFileMan::EOverWrite,gStat);
		User::WaitForRequest(gStat);
		test_KErrNone(r);
		r=gFileMan->Move(_L("\\TEST\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20as\\fdsa21asdffds\\*.*"),_L("\\FINISH\\"), CFileMan::EOverWrite | CFileMan::ERecurse,gStat);
		User::WaitForRequest(gStat);
		test_KErrNone(r);
		r=gFileMan->RmDir(_L("\\START\\"),gStat);
		WaitForSuccess();
		test_KErrNone(r);
		r=gFileMan->RmDir(_L("\\FINISH\\"),gStat);
		WaitForSuccess();
		test_KErrNone(r);
		}

	if (!gAsynch)
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"),_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"));
	else
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"),_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"),0,gStat);
	TestResult(r,KErrNone);

	if (!gAsynch)
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\*.TXT"),_L("\\F32-TST\\TFMAN\\MOVEDIR\\"));
	else
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\*.TXT"),_L("\\F32-TST\\TFMAN\\MOVEDIR\\"),0,gStat);
	TestResult(r);

	if (!gAsynch)
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\*.TXT"),_L("\\F32-TST\\TFMAN\\MoveDIR.\\FILE*.TXT"));
	else
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\*.TXT"),_L("\\F32-TST\\TFMAN\\MoveDIR\\FILE*.TXT"),0,gStat);
	TestResult(r,KErrNotFound);

	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"));
	if (!gAsynch)
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"),_L("\\F32-TST\\TFMAN\\MoveDIR\\FILE1.TXT"),0);
	else
		r=gFileMan->Move(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"),_L("\\F32-TST\\TFMAN\\MoveDIR\\FILE1.TXT"),0,gStat);
	TestResult(r,KErrAlreadyExists);
	r=TheFs.Delete(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"));
	test_KErrNone(r);

	test.Next(_L("Check files have been moved"));
	RmDir(_L("\\F32-TST\\TFMAN\\AFTER\\*"));
	MakeDir(_L("\\F32-TST\\TFMAN\\AFTER\\DELTEST\\EMPTY\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\DELTEST\\FILE4.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\DELTEST\\EXE1.BIN"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\EXE2.BIN"));
	Compare(_L("\\F32-TST\\TFMAN\\AFTER\\*"),_L("\\F32-TST\\TFMAN\\DELDIR\\*"));

	RmDir(_L("\\F32-TST\\TFMAN\\AFTER\\*"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\FILE1.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\FILE2.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\FILE3.TXT"));
	Compare(_L("\\F32-TST\\TFMAN\\AFTER\\*"),_L("\\F32-TST\\TFMAN\\MOVEDIR\\*"));

	if (testingInvalidPathLengths)
		{
		r=gFileMan->RmDir(_L("\\TEST\\"));
		test_KErrNone(r);
		}
	
	TestDEF121663(); // Test moving directory to its subdirectory
	TestDEF123575(); // Test moving directories where src and trg have matching subdirectory structures
	TestDEF125570(); // Test move when trg has at least one of the src dirs
	TestDEF130404(); // Test move when the src doesn't fully exist
	if (!IsTestingLFFS())
		TestPDEF137716(); // Test moving files to overwrite folders that have the same names
	}

LOCAL_C void TestSimultaneous()
//
// Create and run two CFileMen simultaneously
//
	{
	test.Next(_L("Test create and run two CFileMans simultaneously"));
	RmDir(_L("\\F32-TST\\TFMAN\\fman2\\"));

	MakeDir(_L("\\F32-TST\\TFMAN\\FMAN1\\"));
	MakeDir(_L("\\F32-TST\\TFMAN\\FMAN2\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\FMAN1\\ROD.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\FMAN1\\JANE.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\FMAN1\\FREDDY.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\FMAN2\\BORIS.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\FMAN2\\FREDRICK.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\FMAN2\\PETER.TXT"));

	CFileMan* fman=CFileMan::NewL(TheFs);
	TRequestStatus stat1;
	TInt r=fman->Delete(_L("\\F32-TST\\TFMAN\\FMAN1\\*.*"),0,stat1);
	test_KErrNone(r);
	r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\FMAN2\\*.TXT"),_L("\\F32-TST\\TFMAN\\FMAN2\\*.EXT"),0,gStat);
	test_KErrNone(r);
	FOREVER
		{
		if (stat1!=KRequestPending && gStat!=KRequestPending)
			break;
		User::WaitForAnyRequest();
		}
	test(stat1==KErrNone && gStat==KErrNone);
	delete fman;
	
	test.Next(_L("Check all files"));
	RmDir(_L("\\F32-TST\\TFMAN\\AFTER\\*"));

	MakeDir(_L("\\F32-TST\\TFMAN\\AFTER\\"));
	Compare(_L("\\F32-TST\\TFMAN\\After\\*"),_L("\\F32-TST\\TFMAN\\FMAN1\\*"));

	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\BORIS.EXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\FREDRICK.EXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\PETER.EXT"));
	Compare(_L("\\F32-TST\\TFMAN\\After\\*"),_L("\\F32-TST\\TFMAN\\FMAN2\\*"));
	}

// Test wildcards are replaced with letters from the matched file (CFileMan::CreateTargetNameFromSource)
LOCAL_C void TestDEF092084()
	{
	if(gAsynch)  
		{
		return;
		}
	test.Next(_L("Test wildcards are replaced with letters from the matched file (DEF092084)"));
	MakeDir(_L("\\DEF092084"));
	MakeFile(_L("\\DEF092084\\FILE1.TXT"));
	
	TInt r = gFileMan->Rename(_L("\\DEF092084\\*.TXT"),_L("\\DEF092084\\*.DDB"), CFileMan::EOverWrite);
	test_KErrNone(r); 
	CheckFileExists(_L("\\DEF092084\\FILE1.DDB"), KErrNone);
	
	r = gFileMan->Rename(_L("\\DEF092084\\?*.DD?"),_L("\\DEF092084\\?*.TXT"), CFileMan::EOverWrite);
	test_KErrNone(r); 
	CheckFileExists(_L("\\DEF092084\\FILE1.TXT"), KErrNone);
	
	RmDir(_L("\\DEF092084\\"));  
	}
  
//--------------------------------------------- 
//! @SYMTestCaseID			PBASE-T_FMAN-0542
//! @SYMTestType			UT 
//! @SYMREQ					INC109754
//! @SYMTestCaseDesc		1. Tests that CFileMan::Rename() does not incorrectly remove empty source directory
//! @SYMTestActions			Renames the only file from source directory to target directory, then check if 
//!							the empty source directory still exists.
//!							2. Tests the trailing backslash ("\") is interpreted to ("\*.*").
//! @SYMTestExpectedResults	The operation completes with error code KErrNone;
//!							The empty source directory still exists.
//! @SYMTestPriority		High
//! @SYMTestStatus			Implemented 
//--------------------------------------------- 	
void TestINC109754()
	{
	test.Next(_L("Test empty source directory should exist after contents being renamed (INC109754)"));
	TInt r = KErrNone;
	// Setting up comparing dir
	RmDir(		_L("\\F32-TST\\TFMAN\\INC109754_C\\"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC109754_C\\SRC\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC109754_C\\TRG\\FILE.TXT"));

	// Setting up testing dir
	RmDir(		_L("\\F32-TST\\TFMAN\\INC109754\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC109754\\SRC\\FILE.TXT"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC109754\\TRG\\"));

	// Test case 1: CFileMan::Rename(_L("C:\\SRC\\"), 	_L("C:\\TRG\\"));
	if (!gAsynch)
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\INC109754\\SRC\\"),_L("\\F32-TST\\TFMAN\\INC109754\\TRG\\"));
	else
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\INC109754\\SRC\\"),_L("\\F32-TST\\TFMAN\\INC109754\\TRG\\"),0,gStat);
	TestResult(r);
	Compare(_L("\\F32-TST\\TFMAN\\INC109754\\"), _L("\\F32-TST\\TFMAN\\INC109754_C\\"));
	
	// Setting up testing dir
	RmDir(		_L("\\F32-TST\\TFMAN\\INC109754\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC109754\\SRC\\FILE.TXT"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC109754\\TRG\\"));

	// Test case 2: CFileMan::Rename(_L("C:\\SRC\\*.*"), 	_L("C:\\TRG\\"));
	if (!gAsynch)
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\INC109754\\SRC\\*.*"),_L("\\F32-TST\\TFMAN\\INC109754\\TRG\\"));
	else
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\INC109754\\SRC\\*.*"),_L("\\F32-TST\\TFMAN\\INC109754\\TRG\\"),0,gStat);
	TestResult(r);
	Compare(_L("\\F32-TST\\TFMAN\\INC109754\\"), _L("\\F32-TST\\TFMAN\\INC109754_C\\"));
	}


/*
Test code for INC111038() and executed with Cache enabled and FS_NOT_RUGGED.
*/
LOCAL_C void TestINC111038()
	{
	TInt r;
	test.Next(_L("Test example of incorrect attribute flushing"));

	_LIT(KTestFile, "\\TESTFILE.TXT");
	
	test.Printf(_L("1: Create Test File\n"));
	RFile testFile;
	r = testFile.Create(TheFs, KTestFile, EFileRead | EFileWrite);
	test_KErrNone(r);

	test.Printf(_L("2: Populate testFile1 Data\n"));
	r = testFile.Write(_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
	test_KErrNone(r);

	test.Printf(_L("3: Get Initial Attributes\n"));
	TUint atts = 0;
	r = testFile.Att(atts);
	test_KErrNone(r);
	test.Printf(_L("   Attributes: %08x"), atts);

	test.Printf(_L("4: Set KEntryAttHidden Attribute\n"));
	r = testFile.SetAtt(KEntryAttHidden, 0);
	test_KErrNone(r);

	test.Printf(_L("5: Verify KEntryAttHidden Attribute is set for testFile1\n"));
	r = testFile.Att(atts);
	test_KErrNone(r);
	test(atts & KEntryAttHidden);

	test.Printf(_L("6: Read Data from beginning of file testFile1\n"));
	TBuf8<4> data;
	r = testFile.Read(0, data);
	test_KErrNone(r);

	test.Printf(_L("7: Close all the testFiles\n"));
	testFile.Close();
	
	test.Printf(_L("8: Verify KEntryAttHidden is present\n"));
	r = TheFs.Att(KTestFile, atts);
	test_KErrNone(r);
	test.Printf(_L("  Finally, attributes are : %08x\n"), atts);
	test(atts & KEntryAttHidden);
	
	test.Printf(_L("9: Delete Test File\n"));
	r = TheFs.Delete(KTestFile);
	test_Value(r, r == KErrNone || r == KErrNotFound);
	}
	
LOCAL_C void TestDEF113299()
	{
	test.Next(_L("Test invalid file rename (DEF113299)"));
	
	TInt err =0;
	TFileName srcFileName = _L("C:\\F32-TST\\TFMAN\\DEF113299\\src\\corner.html");
	TFileName trgFileName = _L("C:\\F32-TST\\TFMAN\\DEF113299\\src\\mi?d**dle.html");
	TFileName trgInvalidFileName = _L("C:\\F32-TST\\TFMAN\\DEF113299\\src\\mi?d**dle>.html"); // Invalid filename
	TFileName renamedFileName = _L("C:\\F32-TST\\TFMAN\\DEF113299\\src\\mirderdle.html");
	
	RmDir(_L("C:\\F32-TST\\TFMAN\\DEF113299\\"));
	MakeFile(srcFileName,_L8("Test Data"));
	
	// Renaming a file with invalid special characters should fail with error code KErrBadName(-28)
	if (!gAsynch)
		err = gFileMan->Rename(srcFileName,trgInvalidFileName);
	else
		err = gFileMan->Rename(srcFileName,trgInvalidFileName, 0, gStat);
	TestResult(err,KErrBadName);
		
	if(!gAsynch)
		err = gFileMan->Rename(srcFileName,trgFileName);
	else
		err = gFileMan->Rename(srcFileName,trgFileName, 0, gStat);
	TestResult(err,KErrNone);
		
	CheckFileExists(renamedFileName,KErrNone,ETrue);
	}
	
LOCAL_C void TestRename()
//
// Test rename with wildcards
//
	{
	test.Next(_L("Test rename with wildcards"));
	RmDir(_L("\\F32-TST\\TFMAN\\rename\\dest\\"));
	
	MakeDir(_L("\\F32-TST\\TFMAN\\RENAME\\DEST\\"));
	MakeDir(_L("\\F32-TST\\TFMAN\\RENAME\\SRC\\DirTest\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RENAME\\SRC\\abcDEF.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RENAME\\SRC\\abxx.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RENAME\\SRC\\HELLO.SPG"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RENAME\\SRC\\SHEET1.SPR"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RENAME\\SRC\\DirTest.TXT\\Unchanged.txt"));

	TInt r;

	if (testingInvalidPathLengths)
//	Create a path of greater 256 characters by renaming a directory and check it can be
//	manipulated (tests fix to F32)		
		{
		MakeDir(_L("\\LONGNAME\\"));
		MakeDir(_L("\\LONGNAME\\DIRECTORY1DIRECTORY2DIRECTORY3DIRECTORY4\\"));
		MakeFile(_L("\\LONGNAME\\ELEPHANT01ELEPHANT02ELEPHANT03ELEPHANT04.txt"));
		MakeFile(_L("\\LONGNAME\\DINOSAUR01DINOSAUR02DINOSAUR03DINOSAUR04.txt"));
		MakeFile(_L("\\LONGNAME\\FILEFILE01FILEFILE02FILEFILE03FILEFILE04.bin"));
		r=gFileMan->Rename(_L("\\LONGNAME"),_L("\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdff"),CFileMan::EOverWrite);
		test_KErrNone(r);

	//	Two long directory names - makes paths invalid
		MakeDir(_L("\\TEST\\LONG\\NAME\\FGHIJ"));
		MakeDir(_L("\\TEST\\LONG\\NAME\\FGHIJ\\DIRECTORY1DIRECTORY2DIRECTORY3DIRECTORY4\\"));
		MakeFile(_L("\\TEST\\LONG\\NAME\\FGHIJ\\ELEPHANT01ELEPHANT02ELEPHANT03ELEPHANT.txt"));
		MakeFile(_L("\\TEST\\LONG\\NAME\\FGHIJ\\FILEFILE01FILEFILE02FILEFILE03FILEFILE.txt"));
		r=gFileMan->Rename(_L("\\TEST\\LONG"),_L("\\TEST\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20as\\fdsa21asdffds"),CFileMan::EOverWrite);
		test_KErrNone(r);
		}

	//testing invalid source path at the beginning:
	if (!gAsynch)
		{
		r=gFileMan->Rename(_L("::C\\F32-TST\\TFMAN\\RENAME\\SRC\\*.TXT"),_L("\\F32-TST\\TFMAN\\RENAME\\DEST\\*.DDB"),CFileMan::EOverWrite);
		}
	else
		{
		r=gFileMan->Rename(_L("::C\\F32-TST\\TFMAN\\RENAME\\SRC\\*.TXT"),_L("\\F32-TST\\TFMAN\\RENAME\\DEST\\*.DDB"),CFileMan::EOverWrite,gStat);
		}
		TestResult(r,KErrBadName,KErrBadName);
		
	//testing invalid target path at the beginning:  
		
	if (!gAsynch)
		{
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\RENAME\\SRC\\*.TXT"),_L("::C\\F32-TST\\TFMAN\\RENAME\\DEST\\*.DDB"),CFileMan::EOverWrite);
		}
	else
		{
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\RENAME\\SRC\\*.TXT"),_L("::C\\F32-TST\\TFMAN\\RENAME\\DEST\\*.DDB"),CFileMan::EOverWrite,gStat);
		}
		TestResult(r,KErrBadName,KErrBadName);
	
	//testing invalid source path at the middle:	
	if (!gAsynch)
		{
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\:RENAME\\SRC\\*.TXT"),_L("\\F32-TST\\TFMAN\\RENAME\\DEST\\*.DDB"),CFileMan::EOverWrite);
		}
	else
		{
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\:RENAME\\SRC\\*.TXT"),_L("\\F32-TST\\TFMAN\\RENAME\\DEST\\*.DDB"),CFileMan::EOverWrite,gStat);
		}
		TestResult(r,KErrBadName,KErrNone);
	
	//testing invalid target path at the middle:	
	if (!gAsynch)
		{
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\RENAME\\SRC\\*.TXT"),_L("\\F32-TST\\TFMAN\\:RENAME\\DEST\\*.DDB"),CFileMan::EOverWrite);
		}
	else
		{
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\RENAME\\SRC\\*.TXT"),_L("\\F32-TST\\TFMAN\\:RENAME\\DEST\\*.DDB"),CFileMan::EOverWrite,gStat);
		}
		TestResult(r,KErrBadName,KErrNone);

		//testing invalid source path at the end:	
	if (!gAsynch)
		{
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\RENAME\\SRC\\:*.TXT"),_L("\\F32-TST\\TFMAN\\RENAME\\DEST\\*.DDB"),CFileMan::EOverWrite);
		}
	else
		{
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\RENAME\\SRC\\:*.TXT"),_L("\\F32-TST\\TFMAN\\RENAME\\DEST\\*.DDB"),CFileMan::EOverWrite,gStat);
		}
		TestResult(r,KErrBadName,KErrNone);	
	
	//testing invalid target path at the end:	
	if (!gAsynch)
		{
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\RENAME\\SRC\\*.TXT"),_L("\\F32-TST\\TFMAN\\RENAME\\DEST\\:*.DDB"),CFileMan::EOverWrite);
		}
	else
		{
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\RENAME\\SRC\\*.TXT"),_L("\\F32-TST\\TFMAN\\RENAME\\DEST\\:*.DDB"),CFileMan::EOverWrite,gStat);
		}
		TestResult(r,KErrBadName,KErrNone);
	
	if (!gAsynch)
		{
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\RENAME\\SRC\\*.TXT"),_L("\\F32-TST\\TFMAN\\RENAME\\DEST\\*.DDB"),CFileMan::EOverWrite);
		test_KErrNone(r);
		if (testingInvalidPathLengths)
			{
			r=gFileMan->Rename(_L("\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdff\\elephant01elephant02elephant03elephant04.txt"),_L("\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdff\\elephant01elephant02elephant03elephant04.bin"));
			test_Equal(KErrBadName, r);
			r=gFileMan->Rename(_L("\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdff"),_L("\\Shortened"),CFileMan::EOverWrite);
			test_KErrNone(r);
			r=gFileMan->Rename(_L("\\Shortened\\*.txt"),_L("\\Shortened\\*.cat"));
			test_KErrNone(r);
			r=gFileMan->RmDir(_L("\\Shortened\\"));
			test_KErrNone(r);

			r=gFileMan->Rename(_L("\\TEST\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20as\\fdsa21asdffds"),_L("\\TEST\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20as\\NotSoShortened"),CFileMan::EOverWrite);
			test_KErrNone(r);
			r=gFileMan->Rename(_L("\\TEST"),_L("\\OXO!"));
			test_KErrNone(r);
			r=gFileMan->Rename(_L("\\OXO!\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20as"),_L("\\OXO!\\Shorter"));
			test_KErrNone(r);
			r=gFileMan->Rename(_L("\\OXO!\\Shorter\\NotSoShortened\\NAME\\FGHIJ\\*.txt"),_L("\\TEST\\Shorter\\NotSoShortened\\NAME\\FGHIJ\\*.cat"));
			test_KErrNone(r);
			r=gFileMan->RmDir(_L("\\OXO!\\"));
			test_KErrNone(r);

			}
		}
	else
		{
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\RENAME\\SRC\\*.TXT"),_L("\\F32-TST\\TFMAN\\RENAME\\DEST\\*.DDB"),CFileMan::EOverWrite,gStat);
		WaitForSuccess();
		if (testingInvalidPathLengths)
			{
			r=gFileMan->Rename(_L("\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdff\\elephant01elephant02elephant03elephant04.txt"),_L("\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdff\\elephant01elephant02elephant03elephant04.bin"));
			test_Equal(KErrBadName, r);
			r=gFileMan->Rename(_L("\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdff"),_L("\\Shortened"),CFileMan::EOverWrite,gStat);
			WaitForSuccess();
			r=gFileMan->Rename(_L("\\Shortened\\*.txt"),_L("\\Shortened\\*.bin"),0,gStat);
			WaitForSuccess();
			r=gFileMan->RmDir(_L("\\Shortened\\"),gStat);
			WaitForSuccess();

			r=gFileMan->Rename(_L("\\TEST\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20as\\fdsa21asdffds"),_L("\\TEST\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20as\\NotSoShortened"),CFileMan::EOverWrite,gStat);
			WaitForSuccess();
			test_KErrNone(r);
			r=gFileMan->Rename(_L("\\TEST"),_L("\\OXO!"),CFileMan::EOverWrite,gStat);
			WaitForSuccess();
			test_KErrNone(r);
			r=gFileMan->Rename(_L("\\OXO!\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20as"),_L("\\OXO!\\Shorter"),CFileMan::EOverWrite,gStat);
			WaitForSuccess();
			test_KErrNone(r);
			r=gFileMan->Rename(_L("\\OXO!\\Shorter\\NotSoShortened\\NAME\\FGHIJ\\*.txt"),_L("\\TEST\\Shorter\\NotSoShortened\\NAME\\FGHIJ\\*.cat"),CFileMan::EOverWrite,gStat);
			WaitForSuccess();
			test_KErrNone(r);
			r=gFileMan->RmDir(_L("\\OXO!\\"));
			test_KErrNone(r);
			r=gFileMan->RmDir(_L("\\TEST\\"));
			test_KErrNone(r);
			}
		}
	RmDir(_L("\\F32-TST\\TFMAN\\after\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\abcDEF.DDB"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\abxx.DDB"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\DirTest.DDB\\Unchanged.txt"));
	Compare(_L("\\F32-TST\\TFMAN\\AFTER\\*"),_L("\\F32-TST\\TFMAN\\RENAME\\DEST\\*"));

	RmDir(_L("\\F32-TST\\TFMAN\\after\\"));
	MakeDir(_L("\\F32-TST\\TFMAN\\AFTER\\DirTest\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\HELLO.SPG"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\SHEET1.SPR"));
	Compare(_L("\\F32-TST\\TFMAN\\AFTER\\*"),_L("\\F32-TST\\TFMAN\\RENAME\\SRC\\*"));

	if (!gAsynch)
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\RENAME\\DEST\\*.DDB"),_L("\\F32-TST\\TFMAN\\RENAME\\DEST\\*.TXT"));
	else
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\RENAME\\DEST\\*.DDB"),_L("\\F32-TST\\TFMAN\\RENAME\\DEST\\*.TXT"),0,gStat);
	TestResult(r);

	RmDir(_L("\\F32-TST\\TFMAN\\after\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\abcDEF.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\abxx.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\DirTest.TXT\\Unchanged.txt"));
	Compare(_L("\\F32-TST\\TFMAN\\AFTER\\*"),_L("\\F32-TST\\TFMAN\\RENAME\\DEST\\*"));

	test.Next(_L("Test rename case of filenames"));
	MakeFile(_L("\\F32-TST\\TFMAN\\CASETEST\\FileName1"));

	if (!gAsynch)
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\CASETEST\\FileName1"),_L("\\F32-TST\\TFMAN\\CASETEST\\FILENAME1"));
	else
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\CASETEST\\FileName1"),_L("\\F32-TST\\TFMAN\\CASETEST\\FILENAME1"),0,gStat);
	TestResult(r);
	CheckFileExists(_L("\\F32-TST\\TFMAN\\CASETEST\\FileName1"),KErrNone,EFalse);
	CheckFileExists(_L("\\F32-TST\\TFMAN\\CASETEST\\FILENAME1"),KErrNone,ETrue);

	if (!gAsynch)
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\CASETEST\\FILENAME1"),_L("\\F32-TST\\TFMAN\\CASETEST\\FILENAME1"),CFileMan::EOverWrite);
	else
		r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\CASETEST\\FILENAME1"),_L("\\F32-TST\\TFMAN\\CASETEST\\FILENAME1"),CFileMan::EOverWrite,gStat);
	TestResult(r);
	CheckFileExists(_L("\\F32-TST\\TFMAN\\CASETEST\\FILENAME1"),KErrNone,ETrue);

	// Test behaviour for omitted parameters
	// For this, default should be session path
	TFileName sessionPath;
	TInt err=TheFs.SessionPath(sessionPath);
	test_KErrNone(err);

	SetupDirectories(ETrue, NULL);
	err=TheFs.SetSessionPath(_L("\\F32-TST\\TFMAN\\dest\\"));
	test_KErrNone(err);

	err = gFileMan->Rename(_L("\\F32-TST\\TFMAN\\source"), _L(""));
	test_KErrNone(err);
	Compare(_L("\\F32-TST\\TFMAN\\compare\\*"), _L("\\F32-TST\\TFMAN\\dest\\source\\*"));

	RmDir(_L("\\F32-TST\\TFMAN\\dest\\"));
	RmDir(_L("\\F32-TST\\TFMAN\\source\\"));
	SetupDirectories(ETrue, NULL);
	err=TheFs.SetSessionPath(_L("\\F32-TST\\TFMAN\\source\\"));
	test_KErrNone(err);

	err = gFileMan->Rename(_L(""), _L("\\F32-TST\\TFMAN\\dest\\"));
	test_KErrNone(err);
	Compare(_L("\\F32-TST\\TFMAN\\compare\\*"), _L("\\F32-TST\\TFMAN\\dest\\*"));
		
	err=TheFs.SetSessionPath(sessionPath);
	test_KErrNone(err);
	
	TestINC109754(); // Test empty source directory should exist after contents being renamed
	TestDEF092084(); // Test wildcards are replaced with letters from the matched file
	TestDEF113299(); // Test invalid file rename
	}

LOCAL_C void TestAttribs()
//
// Test attribs
//
	{
	test.Next(_L("Test set file attributes"));
	MakeFile(_L("\\F32-TST\\TFMAN\\ATTRIBS\\Attrib1.AT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\ATTRIBS\\Attrib2.at"));

	TUint legalAttMask=KEntryAttMaskSupported&~(KEntryAttDir|KEntryAttVolume);
	TUint setMask=KEntryAttReadOnly;
	TUint clearMask=KEntryAttHidden|KEntryAttArchive;
	if (!gAsynch)
		{
		TInt r=gFileMan->Attribs(_L("\\F32-TST\\TFMAN\\ATTRIBS\\AT*.AT"),setMask,clearMask,TTime(0));
		test_KErrNone(r);
		}
	else
		{
		TInt r=gFileMan->Attribs(_L("\\F32-TST\\TFMAN\\ATTRIBS\\AT*.AT"),setMask,clearMask,TTime(0),0,gStat);
		test_KErrNone(r);
		WaitForSuccess();
		}
	
	CDirScan* scan=CDirScan::NewL(TheFs);
	scan->SetScanDataL(_L("\\F32-TST\\TFMAN\\ATTRIBS\\*"),KEntryAttMaskSupported,ESortByName);
	CDir* entryList;
	scan->NextL(entryList);
	TInt count=entryList->Count();
	test_Equal(2, count);
	TEntry entry=(*entryList)[0];
	test(entry.iName.MatchF(_L("attrib1.AT"))!=KErrNotFound);
	test_Equal(KEntryAttReadOnly, entry.iAtt);
	entry=(*entryList)[1];
	test(entry.iName.MatchF(_L("attrib2.AT"))!=KErrNotFound);
	test_Equal(KEntryAttReadOnly, entry.iAtt);
	delete entryList;

	TDateTime dateTime(1990,ENovember,20,9,5,0,0);
	TTime time(dateTime); // FAT loses microseconds if try to convert HomeTime()

	if (!gAsynch)
		{
		TInt r=gFileMan->Attribs(_L("\\F32-TST\\TFMAN\\ATTRIBS\\AT*.AT"),0,legalAttMask,time);
		test_KErrNone(r);
		}
	else
		{
		TInt r=gFileMan->Attribs(_L("\\F32-TST\\TFMAN\\ATTRIBS\\AT*.AT"),0,legalAttMask,time,0,gStat);
		test_KErrNone(r);
		WaitForSuccess();
		}
	
	scan->SetScanDataL(_L("\\F32-TST\\TFMAN\\ATTRIBS\\*"),KEntryAttMaskSupported,ESortByName);
	scan->NextL(entryList);
	count=entryList->Count();
	test_Equal(2, count);
	entry=(*entryList)[0];
	test(entry.iName.MatchF(_L("attrib1.AT"))!=KErrNotFound);
	test_Equal(0, entry.iAtt);
	TDateTime dt=(entry.iModified).DateTime();
	test(dt.Year()==dateTime.Year());
	test(dt.Month()==dateTime.Month());
	test(dt.Day()==dateTime.Day());
	test(dt.Hour()==dateTime.Hour());
	test(dt.Minute()==dateTime.Minute());
	test(dt.Second()==dateTime.Second());
	test(entry.iModified==time);
	entry=(*entryList)[1];
	test(entry.iName.MatchF(_L("attrib2.AT"))!=KErrNotFound);
	test_Equal(0, entry.iAtt);
	test(entry.iModified==time);
	delete entryList;
	delete scan;
	
	TestINC111038(); // Test example of incorrect attribute flushing
	}
	
LOCAL_C void  TestINC091841()
	{
	if(gAsynch)  
		{
		return;
		}

	test.Next(_L("Test delete long fullnames (INC091841)"));
	MakeDir(_L("\\12345678\\Book\\12345678\\"));
	TFileName longname;
	longname.Copy(_L("\\12345678\\Book\\12345678\\12345678901234567890123456789012345678901234567890.x"));
	MakeFile(longname);
	TFileName oldname = longname;
	TInt ret = KErrNone;
	while(ret == KErrNone)
		{
		oldname = longname;
		longname.Append(_L("xxxxx"));
		ret = TheFs.Replace(oldname, longname);
		}
	if(oldname.Length() >= KMaxFileName-5) // if not, it means that we won't be calling ShrinkNames !!
		{
		TInt r = gFileMan->Rename(_L("\\12345678\\Book\\12345678"),_L("\\INC091841\\Book\\012-235-abcd"),0);
		test_KErrNone(r);  
		CDir* dir;
		r = TheFs.GetDir(_L("\\INC091841\\Book\\012-235-abcd\\"), KEntryAttNormal, ESortNone, dir);
		test_KErrNone(r);   
		r = KErrNotFound;
		TInt dirlen = sizeof("\\INC091841\\Book\\012-235-abcd\\");
		for(TInt i=0; r==KErrNotFound && i<dir->Count(); i++)
			{
			if((*dir)[i].iName.Length() + dirlen > oldname.Length())
				{
				r = KErrNone;
				}
			}
		delete dir;
		test_KErrNone(r);  
		r = gFileMan->RmDir(_L("\\INC091841\\"));
		test_KErrNone(r);  
		}
	RmDir(_L("\\12345678\\"));
	}

LOCAL_C void TestRmDir()
//
// Test rmdir function
//
	{
	test.Next(_L("Test rmdir function"));

	MakeDir(_L("\\F32-TST\\TFMAN\\RMDIR\\EMPTY\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RMDIR\\ALFRED.txt"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RMDIR\\RICHARD.txt"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RMDIR\\RMSUBDIR1\\RMSUBSUBDIR\\CHARLES.txt"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RMDIR\\RMSUBDIR2\\EDMUND.txt"));
	MakeDir(_L("\\F32-TST\\TFMAN\\TESTDIR\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\TESTDIR\\DEF123044.txt"));
	
	TInt r;
	
	if (testingInvalidPathLengths)
//	Create a path of greater 256 characters by renaming a directory and check it can be
//	manipulated (tests fix to F32)		
		{
		MakeDir(_L("\\LONGNAMETEST\\"));
		MakeDir(_L("\\LONGNAMETEST\\DIRECTORY1DIRECTORY2DIRECTORY3DIRECTORY4\\"));
		MakeFile(_L("\\LONGNAMETEST\\ELEPHANT01ELEPHANT02ELEPHANT03ELEPHANT04"));
		MakeFile(_L("\\LONGNAMETEST\\FILEFILE01FILEFILE02FILEFILE03FILEFILE04"));
		r=gFileMan->Rename(_L("\\LONGNAMETEST"),_L("\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdff"),CFileMan::EOverWrite);
		test_KErrNone(r);
		}

	//testing invalid source path at the beginning:
	if (!gAsynch)
		{
		r=gFileMan->RmDir(_L(":C:\\F32-TST\\TFMAN\\RMDIR\\*.AT"));
		}
	else
		{
		r=gFileMan->RmDir(_L(":C:\\F32-TST\\TFMAN\\RMDIR\\*.AT"),gStat);
		}
	TestResult(r,KErrBadName,KErrBadName);
	
	//testing invalid source path at the middle:	
	if (!gAsynch)
		{
		r=gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\:RMDIR\\*.AT"));
		}
	else
		{
		r=gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\:RMDIR\\*.AT"),gStat);
		}
	TestResult(r,KErrBadName,KErrNone);

	//testing invalid source path at the end:	
	if (!gAsynch)
		{
		r=gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\TESTDIR\\:DEF.txt"));
		}
	else
		{
		r=gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\TESTDIR\\:DEF.txt"),gStat);
		}
	TestResult(r,KErrNone,KErrNone);
	
	if (!gAsynch)
		{
		r=gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\RMDIR\\*.AT"));
		test_KErrNone(r);
			if (testingInvalidPathLengths)
			{
			r=gFileMan->RmDir(_L("\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdff\\"));
			test_KErrNone(r);
			}
		}
	else
		{
		r=gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\RMDIR\\*.AT"),gStat);
		test_KErrNone(r);
		WaitForSuccess();
		if (testingInvalidPathLengths)
			{
			r=gFileMan->RmDir(_L("\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdff\\"),gStat);
			test_KErrNone(r);
			WaitForSuccess();
			}
		}

	TEntry entry;
	r=TheFs.Entry(_L("\\F32-TST\\TFMAN\\RMDIR"),entry);
	test_Equal(KErrNotFound, r);

	MakeDir(_L("\\F32-TST\\TFMAN\\READONLY\\"));
	r=TheFs.SetAtt(_L("\\F32-TST\\TFMAN\\READONLY\\"),KEntryAttReadOnly,0);
	test_KErrNone(r);

	if (!gAsynch)
		{
		r=gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\READONLY\\"));
		test_Equal(KErrAccessDenied, r);
		}
	else
		{
		r=gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\READONLY\\"),gStat);
		test_KErrNone(r);
		WaitForResult(KErrAccessDenied);
		}

	r=TheFs.SetAtt(_L("\\F32-TST\\TFMAN\\READONLY\\"),0,KEntryAttReadOnly);
	test_KErrNone(r);

	r=gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\READONLY\\"));
	test_KErrNone(r);
	
	// Test behaviour for omitted parameters
	// For this, default should be session path
	TFileName sessionPath;
	r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);

	SetupDirectories(ETrue, NULL);
	r=TheFs.SetSessionPath(_L("\\F32-TST\\TFMAN\\source\\"));

	// Default removal of session path
	r=gFileMan->RmDir(_L(""));
	test_KErrNone(r);

	r=TheFs.SetSessionPath(sessionPath);
	
	r = gFileMan->Rename(_L("\\F32-TST\\TFMAN\\source\\subdir"), _L("\\F32-TST\\TFMAN\\source\\tofail"), CFileMan::ERecurse);
	test_Equal(KErrPathNotFound, r);
	
	r = gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\"));
	test_KErrNone(r);
	MakeDir(_L("\\F32-TST\\TFMAN\\"));
	
	if(testingInvalidPathLengths)
		{
		TestINC091841(); // Test delete long fullnames
		}

	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_FMAN-0316
	//! @SYMTestType			UT 
	//! @SYMREQ					DEF099820
	//! @SYMTestCaseDesc		Test that CFileMan::RmDir() works when deleting a directory containing open files.
	//! @SYMTestActions			Open a file within a directory and try to remove the directory.
	//! @SYMTestExpectedResults	The operation completes with the error code KErrInUse.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented 
	//--------------------------------------------- 	

	test.Next(_L("Test delete directory containing open files"));
	gFileMan->SetObserver(NULL);

	MakeDir(_L("\\F32-TST\\TFMAN\\OPENFILE\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\OPENFILE\\FILE.TXT"));

	RFile file;
	r = file.Open(TheFs,_L("\\F32-TST\\TFMAN\\OPENFILE\\FILE.TXT"), EFileRead | EFileShareExclusive);
	test_KErrNone(r);

	if (!gAsynch)
		{
		r=gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\OPENFILE\\"));
		test_Equal(KErrInUse, r);
		
		file.Close();
		
		r=gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\OPENFILE\\"));
		test_KErrNone(r);
		}
	else
		{
		r=gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\OPENFILE\\"), gStat);
		test_KErrNone(r);
		WaitForResult(KErrInUse);

		file.Close();

		r=gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\OPENFILE\\"), gStat);
		test_KErrNone(r);
		WaitForResult(KErrNone);
		}

	gFileMan->SetObserver(gObserver);
	}

LOCAL_C void TestRecursiveCopy()
//
// Test the recursive copy function
//
	{
	test.Next(_L("Test recursive copy"));
	RmDir(_L("\\F32-TST\\TFMAN\\COPYDIR\\"));

	MakeDir(_L("\\F32-TST\\TFMAN\\COPYDIR\\"));
	MakeDir(_L("\\F32-TST\\TFMAN\\DELDIR\\DELTEST\\EMPTY\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE2.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE3.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\DELTEST\\FILE4.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\DELTEST\\EXE1.BIN"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\EXE2.BIN"));

	TInt r;
	if (!gAsynch)
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\"),_L("\\F32-TST\\TFMAN\\COPYDIR"),CFileMan::ERecurse);
	else
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\"),_L("\\F32-TST\\TFMAN\\COPYDIR"),CFileMan::ERecurse,gStat);
	TestResult(r);

	Compare(_L("\\F32-TST\\TFMAN\\DELDIR\\*"),_L("\\F32-TST\\TFMAN\\COPYDIR\\*"));
	RmDir(_L("\\F32-TST\\TFMAN\\COPYDIR\\"));
	MakeDir(_L("\\F32-TST\\TFMAN\\COPYDIR\\"));

	if (!gAsynch)
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\*.BIN"),_L("\\F32-TST\\TFMAN\\COPYDIR\\*.EXT"),CFileMan::ERecurse);
	else
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\*.BIN"),_L("\\F32-TST\\TFMAN\\COPYDIR\\*.EXT"),CFileMan::ERecurse,gStat);
	TestResult(KErrNone);

	RmDir(_L("\\F32-TST\\TFMAN\\after\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\DELTEST\\EXE1.EXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\EXE2.EXT"));
	Compare(_L("\\F32-TST\\TFMAN\\AFTER\\*"),_L("\\F32-TST\\TFMAN\\COPYDIR\\*"));

	// Test behaviour for omitted parameters
	// For this, default should be session path
	TFileName sessionPath;
	r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);

	RmDir(_L("\\F32-TST\\TFMAN\\COPYDIR\\"));
	MakeDir(_L("\\F32-TST\\TFMAN\\COPYDIR\\"));
	r=TheFs.SetSessionPath(_L("\\F32-TST\\TFMAN\\COPYDIR\\"));

	// Default copy to session path
	if (!gAsynch)
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\"),_L(""),CFileMan::ERecurse);
	else
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\"),_L(""),CFileMan::ERecurse,gStat);
	TestResult(KErrNone);
	Compare(_L("\\F32-TST\\TFMAN\\DELDIR\\*"),_L("\\F32-TST\\TFMAN\\COPYDIR\\*"));

	RmDir(_L("\\F32-TST\\TFMAN\\COPYDIR\\"));
	MakeDir(_L("\\F32-TST\\TFMAN\\COPYDIR\\"));
	r=TheFs.SetSessionPath(_L("\\F32-TST\\TFMAN\\DELDIR\\"));

	// Default copy from session path
	if (!gAsynch)
		r=gFileMan->Copy(_L(""),_L("\\F32-TST\\TFMAN\\COPYDIR\\"),CFileMan::ERecurse);
	else
		r=gFileMan->Copy(_L(""),_L("\\F32-TST\\TFMAN\\COPYDIR\\"),CFileMan::ERecurse,gStat);
	TestResult(KErrNone);
	Compare(_L("\\F32-TST\\TFMAN\\DELDIR\\*"),_L("\\F32-TST\\TFMAN\\COPYDIR\\*"));

	RmDir(_L("\\F32-TST\\TFMAN\\COPYDIR\\"));
	RmDir(_L("\\F32-TST\\TFMAN\\DELDIR\\"));
	r=TheFs.SetSessionPath(sessionPath);
	test_KErrNone(r);
	}
	
LOCAL_C void TestRecursiveAttribs()
//
// Test set attribs recursively
//
	{
	test.Next(_L("Test recursive attribs"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RECATTRIBS\\Attrib1.AT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RECATTRIBS\\Attrib2.at"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RECATTRIBS\\SUBDIR\\ATFILE.TXT"));

	if (!gAsynch)
		{
		TInt r=gFileMan->Attribs(_L("\\F32-TST\\TFMAN\\RECATTRIBS\\"),KEntryAttReadOnly,0,TTime(0),CFileMan::ERecurse);
		test_KErrNone(r);
		}
	else
		{
		TInt r=gFileMan->Attribs(_L("\\F32-TST\\TFMAN\\RECATTRIBS\\"),KEntryAttReadOnly,0,TTime(0),CFileMan::ERecurse,gStat);
		test_KErrNone(r);
		WaitForSuccess();
		}

	CDir* entryList;
	CDirScan* scan=CDirScan::NewL(TheFs);
	scan->SetScanDataL(_L("\\F32-TST\\TFMAN\\RECATTRIBS\\*"),KEntryAttMaskSupported,ESortByName);
	scan->NextL(entryList);
	TInt count=entryList->Count();
	test_Equal(3, count);
	TEntry entry=(*entryList)[0];
	test(entry.iName.MatchF(_L("ATTRIB1.AT"))!=KErrNotFound);
	if (!IsTestingLFFS())
		{
		test_Equal((KEntryAttReadOnly|KEntryAttArchive), entry.iAtt);
		}
	else
		{
		test(entry.iAtt&KEntryAttReadOnly); // ???
		}
	entry=(*entryList)[1];
	test(entry.iName.MatchF(_L("ATTRIB2.AT"))!=KErrNotFound);
	if (!IsTestingLFFS())
		{
		test_Equal((KEntryAttReadOnly|KEntryAttArchive), entry.iAtt);
		}
	else
		{
		test(entry.iAtt&KEntryAttReadOnly); // ???
		}
	entry=(*entryList)[2];
	test(entry.iName.MatchF(_L("SUBDIR"))!=KErrNotFound);
	delete entryList;

	scan->NextL(entryList);
	count=entryList->Count();
	test_Equal(1, count);
	entry=(*entryList)[0];
	test(entry.iName.MatchF(_L("ATFILE.TXT"))!=KErrNotFound);
	if (!IsTestingLFFS())
		{
		test_Equal((KEntryAttReadOnly|KEntryAttArchive), entry.iAtt);
		}
	else
		{
		test(entry.iAtt&KEntryAttReadOnly); // ???
		}
	delete entryList;

	scan->NextL(entryList);
	test(entryList==NULL);

	if (!gAsynch)
		{
		TInt r=gFileMan->Attribs(_L("\\F32-TST\\TFMAN\\RECATTRIBS\\"),0,KEntryAttReadOnly|KEntryAttArchive,TTime(0),CFileMan::ERecurse);
		test_KErrNone(r);
		}
	else
		{
		TInt r=gFileMan->Attribs(_L("\\F32-TST\\TFMAN\\RECATTRIBS\\"),0,KEntryAttReadOnly|KEntryAttArchive,TTime(0),CFileMan::ERecurse,gStat);
		test_KErrNone(r);
		WaitForSuccess();
		}

	scan->SetScanDataL(_L("\\F32-TST\\TFMAN\\RECATTRIBS\\*"),KEntryAttMaskSupported,ESortByName);
	scan->NextL(entryList);
	count=entryList->Count();
	test_Equal(3, count);
	entry=(*entryList)[0];
	test(entry.iName.MatchF(_L("ATTRIB1.AT"))!=KErrNotFound);
	test_Equal(KEntryAttNormal, entry.iAtt);
	entry=(*entryList)[1];
	test(entry.iName.MatchF(_L("ATTRIB2.AT"))!=KErrNotFound);
	test_Equal(KEntryAttNormal, entry.iAtt);
	entry=(*entryList)[2];
	test(entry.iName.MatchF(_L("SUBDIR"))!=KErrNotFound);
	delete entryList;

	scan->NextL(entryList);
	count=entryList->Count();
	test_Equal(1, count);
	entry=(*entryList)[0];
	test(entry.iName.MatchF(_L("ATFILE.TXT"))!=KErrNotFound);
	test_Equal(KEntryAttNormal, entry.iAtt);
	delete entryList;

	scan->NextL(entryList);
	test(entryList==NULL);
	delete scan;
	}

LOCAL_C void TestRecursiveDelete()
//
// Test Recursive delete
//
	{
	test.Next(_L("Test recursive delete"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RECDELETE\\FULL\\GRAPE.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RECDELETE\\FULL\\GRAPE.PLP"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RECDELETE\\GRAPE.PLP"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RECDELETE\\FILE1.TXT"));

	if (!gAsynch)
		{
		TInt r=gFileMan->Delete(_L("\\F32-TST\\TFMAN\\RECDELETE\\*.PLP"),CFileMan::ERecurse);
		test_KErrNone(r);
		}
	else
		{
		TInt r=gFileMan->Delete(_L("\\F32-TST\\TFMAN\\RECDELETE\\*.PLP"),CFileMan::ERecurse,gStat);
		test_KErrNone(r);
		WaitForSuccess();
		}

	RmDir(_L("\\F32-TST\\TFMAN\\after\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\FULL\\GRAPE.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\FILE1.TXT"));
	Compare(_L("\\F32-TST\\TFMAN\\after\\*"),_L("\\F32-TST\\TFMAN\\RecDelete\\*"));
	}

LOCAL_C void TestINC108401()
  {
   	
   	test.Next(_L("Test synchronous and asynchronous move operations (INC108401)"));
   	TInt err = 0;
   	
	TFileName trgPath = _L("?:\\F32-TST\\");

	if (gSessionPath[0]!='D'&& gSessionPath[0]!='Y' && gSessionPath[0]!='I')
		{
#if !defined(__WINS__)
        if (!gSecDriveReady)
            {
            test.Printf(_L("Second drive not available for test, skip..."));
            return;
            }
		trgPath[0] = (TText) gSecDrive;
#else
		trgPath[0] = 'Y';
#endif
		}
	else
		return;

	TFileName trgDir = trgPath;
	trgDir.Append(_L("TFMAN\\INC108401\\dest\\"));
	
	// Moving files and dirs ACROSS DRIVE.
  	err = 0;
	MakeDir(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\subDirB\\subDirC\\"));
	MakeDir(trgDir);
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\file1"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\file1"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\file2"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\file2"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file1"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file1"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file2"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file2"));
	// Synchronously
	if (!gAsynch)
		err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC108401\\src"), trgDir, 0);
	else // Asynchronously
		err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC108401\\src"), trgDir, 0, gStat);
	test.Next(_L("Test INC108401 : ACROSS DRIVES with 0"));
	TestResult(err);
	// cleanup the current drive
	RmDir(trgPath);
	// remove the F32-TST dir on the C: drive
	RmDir(_L("\\F32-TST\\TFMAN\\"));
	
	err = 0;
	MakeDir(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\subDirB\\subDirC\\"));
	MakeDir(trgDir);
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\file1"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\file1"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\file2"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\file2"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file1"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file1"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file2"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file2"));
	// Synchronously
	if (!gAsynch)
		err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC108401\\src"), trgDir, CFileMan::EOverWrite);
	else // Asynchronously
		err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC108401\\src"), trgDir, CFileMan::EOverWrite, gStat);
	test.Next(_L("Test INC108401 : ACROSS DRIVES with CFileMan::EOverWrite"));
	TestResult(err);
	// cleanup the current drive
	RmDir(trgPath);
	// remove the F32-TST dir on the C: drive
	RmDir(_L("\\F32-TST\\"));
	
	err = 0;
	MakeDir(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\subDirB\\subDirC\\"));
	MakeDir(trgDir);
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\file1"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\file1"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\file2"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\file2"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file1"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file1"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file2"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file2"));
	// Synchronously
	if (!gAsynch)
		err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC108401\\src"), trgDir, CFileMan::ERecurse);
	else // Asynchronously
		err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC108401\\src"), trgDir, CFileMan::ERecurse, gStat);
	test.Next(_L("Test INC108401 : ACROSS DRIVES with CFileMan::ERecurse"));
	TestResult(err);
	// cleanup the current drive
	RmDir(trgPath);
	// remove the F32-TST dir on the C: drive
	RmDir(_L("\\F32-TST\\"));


	// Moving files and dirs on the SAME DRIVE.
	// case for gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC108401\\src"), _L("\\F32-TST\\TFMAN\\INC108401\\dest\\"), 0);
	MakeDir(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\subDirB\\subDirC\\"));
	MakeDir(_L("\\F32-TST\\TFMAN\\INC108401\\dest\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\file1"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\file1"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\file2"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\file2"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file1"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file1"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file2"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file2"));
	// Synchronously
	if (!gAsynch)	
		err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC108401\\src"), _L("\\F32-TST\\TFMAN\\INC108401\\dest\\"), 0);
	else // Asynchronously	
		err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC108401\\src"), _L("\\F32-TST\\TFMAN\\INC108401\\dest\\"), 0, gStat);
	test.Next(_L("Test INC108401 : SAME DRIVE with 0"));
	TestResult(err);
	// test_KErrNone(err);
	RmDir(_L("\\F32-TST\\TFMAN\\INC108401\\"));
	
	// case for gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC108401\\src"), _L("\\F32-TST\\TFMAN\\INC108401\\dest\\"), CFileMan::EOverWrite);
	MakeDir(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\subDirB\\subDirC\\"));
	MakeDir(_L("\\F32-TST\\TFMAN\\INC108401\\dest\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\file1"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\file1"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\file2"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\file2"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file1"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file1"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file2"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file2"));
	// Synchronously
	if (!gAsynch)	
		err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC108401\\src"), _L("\\F32-TST\\TFMAN\\INC108401\\dest\\"), CFileMan::EOverWrite);
	else // Asynchronously	
		err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC108401\\src"), _L("\\F32-TST\\TFMAN\\INC108401\\dest\\"), CFileMan::EOverWrite, gStat);
	test.Next(_L("Test INC108401 : SAME DRIVE with CFileMan::EOverWrite"));
	TestResult(err);
	// test_KErrNone(err);
	RmDir(_L("\\F32-TST\\TFMAN\\INC108401\\"));
	
	// case for gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC108401\\src"), _L("\\F32-TST\\TFMAN\\INC108401\\dest\\"), CFileMan::EOverWrite|CFileMan::ERecurse);
	MakeDir(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\subDirB\\subDirC\\"));
	MakeDir(_L("\\F32-TST\\TFMAN\\INC108401\\dest\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\file1"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\file1"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\file2"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\file2"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file1"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file1"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file2"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\file2"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\subDirB\\file1"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\subDirB\\file1"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\subDirB\\file1"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\subDirB\\file1"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\subDirB\\subDirC\\file1"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\subDirB\\subDirC\\file1"));
	MakeFile(_L("\\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\subDirB\\subDirC\\file1"),_L8("FILE PATH : \\F32-TST\\TFMAN\\INC108401\\src\\subDirA\\subDirB\\subDirC\\file1"));
	// Synchronously
	if (!gAsynch)	
		err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC108401\\src"), _L("\\F32-TST\\TFMAN\\INC108401\\dest\\"), CFileMan::ERecurse|CFileMan::EOverWrite);
	else // Asynchronously	
		err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC108401\\src"), _L("\\F32-TST\\TFMAN\\INC108401\\dest\\"), CFileMan::ERecurse|CFileMan::EOverWrite, gStat);
	test.Next(_L("Test INC108401 : SAME DRIVES with CFileMan::ERecurse|CFileMan::EOverWrite"));
	TestResult(err);
	// test_KErrNone(err);
	RmDir(_L("\\F32-TST\\TFMAN\\INC108401\\"));
	
	// cleanup for the current drive
	RmDir(trgPath);
	RmDir(_L("\\F32-TST\\"));

	test.Printf(_L("Test INC108401 : ends\n"));
  }

LOCAL_C void TestINC089638()
  {
	if(gAsynch)  
		{
		return;
		}
	
	test.Next(_L("Test all items removed from source directory after recursive moving (INC089638)"));
	RmDir(_L("\\INC089638\\source\\"));
	RmDir(_L("\\INC089638\\dest\\"));
	MakeFile(_L("\\INC089638\\source\\file1"));
	MakeFile(_L("\\INC089638\\source\\file2"));
	MakeFile(_L("\\INC089638\\source\\subdir1\\file3"));
	MakeFile(_L("\\INC089638\\source\\subdir1\\file4"));
	MakeFile(_L("\\INC089638\\source\\subdir2\\file5"));
	MakeFile(_L("\\INC089638\\source\\subdir2\\file6"));
	MakeDir(_L("\\INC089638\\dest\\"));
	test_KErrNone(TheFs.SetAtt(_L("\\INC089638\\source\\subdir1"), KEntryAttHidden, 0));
	test_KErrNone(TheFs.SetAtt(_L("\\INC089638\\source\\subdir2"), KEntryAttReadOnly, 0));
	
	TInt r = gFileMan->Move(_L("\\INC089638\\source\\"), _L("\\INC089638\\dest\\"), CFileMan::ERecurse);
	test_KErrNone(r);
	r = TheFs.RmDir(_L("\\INC089638\\source\\"));
	test_KErrNone(r);
	
	RmDir(_L("\\INC089638\\"));
  }

void TestINC101379()
	{
	test.Next(_L("Test moving of directory to its subdirectory recursively and not recursively (INC101379)"));
	TInt err;
	_LIT(KSourceDir,"\\INC101379\\dir\\");
	_LIT(KFile1, "\\INC101379\\dir\\file1.txt");
	_LIT(KFile2, "\\INC101379\\dir\\subdir\\file2.txt");
	_LIT(KFile3, "\\INC101379\\dir\\other\\file3.txt");
	MakeFile(KFile1, _L8("qwerty"));
	MakeFile(KFile2, _L8("abc"));
	MakeFile(KFile3, _L8("qwerty"));
	TFileName dest;
	dest.Copy(KSourceDir);
	dest.Append(_L("subdir"));
	gFileMan->SetObserver(NULL);
	if (!gAsynch)
		err = gFileMan->Move(KSourceDir, dest, CFileMan::ERecurse|CFileMan::EOverWrite);
	else
		err = gFileMan->Move(KSourceDir, dest, CFileMan::ERecurse|CFileMan::EOverWrite, gStat);
	test_Equal(KErrInUse, err); // Recursive move prohibited
	if (gAsynch)
		WaitForResult(KErrInUse);
	CheckFileContents(KFile1, _L8("qwerty"));
	CheckFileContents(KFile2, _L8("abc"));
	CheckFileContents(KFile3, _L8("qwerty"));

	if (!gAsynch)
		err = gFileMan->Move(KSourceDir, dest, CFileMan::EOverWrite);
	else
		err = gFileMan->Move(KSourceDir, dest, CFileMan::EOverWrite, gStat);
	TestResult(err, KErrNone); // Non-recursive move must be OK

	_LIT(KFile1Moved, "\\INC101379\\dir\\subdir\\file1.txt");
	CheckFileContents(KFile1Moved, _L8("qwerty"));
	CheckFileContents(KFile2, _L8("abc"));
	CheckFileContents(KFile3, _L8("qwerty"));
	gFileMan->SetObserver(gObserver);
	RmDir(KSourceDir);
	RmDir(_L("\\INC101379\\"));
	}
	
 void TestINC099600() // and INC101061
	{
	// Test move files from the internal drive to an external one (INC099600)
	// Test move files with system (KEntryAttSystem) or hidden (KEntryAttHidden) attributes (INC101061)
	test.Next(_L("Test move files from internal drive to external with system and hidden attributes"));
	_LIT(KDest,"C:\\DEST099600\\");
	TBuf<64> source;
	source.Format(_L("%c:\\INC099600\\"), (TUint) gDriveToTest);
	TBuf<64> src;
	TInt r;
	TBuf<64> dst;
	RmDir(source);
	RmDir(KDest);
	
	src = source;
	src.Append('a');
	MakeFile(src);
	TheFs.SetAtt(src, KEntryAttArchive, 0);
	src.Append('h');
	MakeFile(src);
	TheFs.SetAtt(src, KEntryAttArchive | KEntryAttHidden, 0);
	src.Append('s');
	MakeFile(src);
	TheFs.SetAtt(src, KEntryAttArchive | KEntryAttHidden | KEntryAttSystem, 0);
	src.Append('x');
	src.Append(KPathDelimiter);
	src.Append('a');
	MakeFile(src);
	TheFs.SetAtt(src, KEntryAttArchive, 0);
	src.Append('h');
	MakeFile(src);
	TheFs.SetAtt(src, KEntryAttArchive | KEntryAttHidden, 0);

	dst.Copy(KDest);
	dst.Append(_L("ahsx\\"));
	MakeDir(dst);

	TEntry entry;
	r = gFileMan->Move(src, KDest, 0); // ahsx\ah
	test_KErrNone(r);
	r = TheFs.Entry(src, entry);
	test_Equal(KErrNotFound, r);

	src.SetLength(src.Length()-1); // ahsx\a
	r = gFileMan->Move(src, KDest, 0);
	test_KErrNone(r);
	r = TheFs.Entry(src, entry);
	test_Equal(KErrNotFound, r);
	
	src.SetLength(src.Length()-3); // ahs
	r = gFileMan->Move(src, KDest, 0);
	test_KErrNone(r);
	r = TheFs.Entry(src, entry);
	test_Equal(KErrNotFound, r);
	
	src.SetLength(src.Length()-1); // ah
	r = gFileMan->Move(src, KDest, 0);
	test_Equal(KErrAlreadyExists, r);
	r = TheFs.Entry(src, entry);
	test_KErrNone(r);

	r = gFileMan->Move(src, KDest, CFileMan::EOverWrite); // ah
	test_KErrNone(r);
	r = TheFs.Entry(src, entry);
	test_Equal(KErrNotFound, r);

	src.SetLength(src.Length()-1); // a
	r = gFileMan->Move(src, KDest, 0);
	test_Equal(KErrAlreadyExists, r);
	r = TheFs.Entry(src, entry);
	test_KErrNone(r);

	r = gFileMan->Move(src, KDest, CFileMan::EOverWrite); // a
	test_KErrNone(r);
	r = TheFs.Entry(src, entry);
	test_Equal(KErrNotFound, r);

	RmDir(source);
	RmDir(KDest);
	}

void SetupDirectoriesForCase0520()
// Setup initial directory structure for test case PBASE-T_FMAN-0520
	{
	RmDir(		_L("\\F32-TST\\TFMAN\\INC106735\\"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735\\F_ROOT.TXT"), _L8("blahblahblah"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735\\SUB\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735\\SUB\\F_SUB.TXT"), _L8("blahblahblah"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735\\SUB01\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735\\SUB01\\F_SUB01.TXT"), _L8("blahblahblah"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735\\SUB01\\SUB02\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735\\SUB01\\SUB02\\F_SUB02.TXT"), _L8("blahblahblah"));
	}

void SetupDirectoriesForCase0520Compare1()
// Comparing directory structure for recursive Move() without wildcard
	{
	RmDir(		_L("\\F32-TST\\TFMAN\\INC106735_COM\\"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB\\F_ROOT.TXT"), _L8("blahblahblah"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB\\F_SUB.TXT"), _L8("blahblahblah"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB01\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB01\\F_SUB01.TXT"), _L8("blahblahblah"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB01\\SUB02\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB01\\SUB02\\F_SUB02.TXT"), _L8("blahblahblah"));
	}

void SetupDirectoriesForCase0520Compare2()
// Comparing directory structure for recursive Move() with wildcard
	{
	RmDir(		_L("\\F32-TST\\TFMAN\\INC106735_COM\\"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB\\F_ROOT.TXT"), _L8("blahblahblah"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB\\F_SUB.TXT"), _L8("blahblahblah"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB\\SUB01\\F_SUB01.TXT"), _L8("blahblahblah"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB\\SUB01\\SUB02\\F_SUB02.TXT"), _L8("blahblahblah"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB01\\"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB01\\SUB02\\"));
	}

void SetupDirectoriesForCase0520Compare3()
// Comparing directory structure for recursive Copy() without wildcard
	{
	RmDir(		_L("\\F32-TST\\TFMAN\\INC106735_COM\\"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\F_ROOT.TXT"), _L8("blahblahblah"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB\\F_ROOT.TXT"), _L8("blahblahblah"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB\\F_SUB.TXT"), _L8("blahblahblah"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB01\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB01\\F_SUB01.TXT"), _L8("blahblahblah"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB01\\SUB02\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB01\\SUB02\\F_SUB02.TXT"), _L8("blahblahblah"));
	}

void SetupDirectoriesForCase0520Compare4()
// Comparing directory structure for recursive Copy() with wildcard
	{
	RmDir(		_L("\\F32-TST\\TFMAN\\INC106735_COM\\"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\F_ROOT.TXT"), _L8("blahblahblah"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB\\F_ROOT.TXT"), _L8("blahblahblah"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB\\F_SUB.TXT"), _L8("blahblahblah"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB\\SUB01\\F_SUB01.TXT"), _L8("blahblahblah"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB\\SUB01\\SUB02\\F_SUB02.TXT"), _L8("blahblahblah"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB01\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB01\\F_SUB01.TXT"), _L8("blahblahblah"));
	MakeDir(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB01\\SUB02\\"));
	MakeFile(	_L("\\F32-TST\\TFMAN\\INC106735_COM\\SUB01\\SUB02\\F_SUB02.TXT"), _L8("blahblahblah"));
	}

LOCAL_C void TestRecursiveMove()
//
// Test recursive move
//
	{
	test.Next(_L("Test recursive move"));
	RmDir(_L("\\F32-TST\\TFMAN\\RecMove2\\"));

	MakeFile(_L("\\F32-TST\\TFMAN\\RECMOVE\\FULL\\FILE2.PLP"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RECMOVE\\FULL\\FILE3.PLP"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RECMOVE\\FULL\\GRAPE.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RECMOVE\\GRAPE.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\RECMOVE\\FILE1.PLP"));

	TInt err;
	if (!gAsynch)
		err=gFileMan->Move(_L("\\F32-TST\\TFMAN\\RECMOVE\\*.PLP"),_L("\\F32-TST\\TFMAN\\RECMOVE2\\"),CFileMan::ERecurse);
	else
		err=gFileMan->Move(_L("\\F32-TST\\TFMAN\\RECMOVE\\*.PLP"),_L("\\F32-TST\\TFMAN\\RECMOVE2\\"),CFileMan::ERecurse,gStat);
	TestResult(err);

	RmDir(_L("\\F32-TST\\TFMAN\\after\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\FULL\\FILE2.PLP"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\FULL\\FILE3.PLP"));
	MakeFile(_L("\\F32-TST\\TFMAN\\AFTER\\FILE1.PLP"));
	Compare(_L("\\F32-TST\\TFMAN\\after\\*"),_L("\\F32-TST\\TFMAN\\RecMOve2\\*"));

	//
	// Test moving empty directories (DEF073924)
	//
	test.Next(_L("Test moving empty directories"));

	SetupDirectories(EFalse, NULL);

	err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\source\\"), _L("\\F32-TST\\TFMAN\\dest\\"), CFileMan::ERecurse);
	test_Equal(KErrNotFound, err);	// Expected - directory is empty

	// Test that all directories are still present
	TEntry entry;
	err = TheFs.Entry(_L("\\F32-TST\\TFMAN\\source\\"), entry);
	test_KErrNone(err);
	err = TheFs.Entry(_L("\\F32-TST\\TFMAN\\dest\\"), entry);
	test_KErrNone(err);

	SetupDirectories(EFalse, NULL);

	err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\source"), _L("\\F32-TST\\TFMAN\\dest\\"), CFileMan::ERecurse);
	test_KErrNone(err);		// Expected - should move (or rename) directory

	// Test directory has been moved
	err = TheFs.Entry(_L("\\F32-TST\\TFMAN\\dest\\source\\"), entry);
	test_KErrNone(err);
	err = TheFs.Entry(_L("\\F32-TST\\TFMAN\\source\\"), entry);
	test_Equal(KErrNotFound, err);

	RmDir(_L("\\F32-TST\\TFMAN\\dest\\source\\"));

	//
	// Test moving when the source directory contains subdirectories (INC074828, INC078800)
	//
	test.Next(_L("Test moving a directory containing subdirectories"));

	SetupDirectories(ETrue, NULL);
	err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\source\\"), _L("\\F32-TST\\TFMAN\\dest\\"), CFileMan::ERecurse | CFileMan::EOverWrite);
	test_KErrNone(err);

	Compare(_L("\\F32-TST\\TFMAN\\compare\\*"), _L("\\F32-TST\\TFMAN\\dest\\*"));

	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_FMAN-0160
	//! @SYMTestType			UT 
	//! @SYMREQ					DEF087791
	//! @SYMTestCaseDesc		Test that CFileMan::Move() works when the destination paths does not exist.
	//! @SYMTestActions			Copy directory structures to a non-existant directory on the same drive.
	//! @SYMTestExpectedResults	Completes with no error, files are copied and intermediate directories are created.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented 
	//--------------------------------------------- 	
	test.Next(_L("Test moving when the target directory does not exist"));

	SetupDirectories(ETrue, NULL);
	
	RmDir(_L("\\F32-TST\\TFMAN\\dest\\"));

	err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\source\\"), _L("\\F32-TST\\TFMAN\\dest\\"), CFileMan::ERecurse);
	test_KErrNone(err);

	Compare(_L("\\F32-TST\\TFMAN\\compare\\*"), _L("\\F32-TST\\TFMAN\\dest\\*"));

	SetupDirectories(ETrue, NULL);
	
	RmDir(_L("\\F32-TST\\TFMAN\\dest\\"));

	err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\source\\"), _L("\\F32-TST\\TFMAN\\dest"), CFileMan::ERecurse);
	test_KErrNone(err);

	Compare(_L("\\F32-TST\\TFMAN\\compare\\*"), _L("\\F32-TST\\TFMAN\\dest\\*"));

	SetupDirectories(ETrue, NULL);
	
	RmDir(_L("\\F32-TST\\TFMAN\\dest\\"));

	err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\source"), _L("\\F32-TST\\TFMAN\\dest\\"), CFileMan::ERecurse);
	test_KErrNone(err);

	MakeDir(_L("\\F32-TST\\TFMAN\\compare\\subdir\\"));
	Compare(_L("\\F32-TST\\TFMAN\\compare\\*"), _L("\\F32-TST\\TFMAN\\dest\\source\\*"));
	RmDir(_L("\\F32-TST\\TFMAN\\compare\\subdir\\"));

	SetupDirectories(ETrue, NULL);

	RmDir(_L("\\F32-TST\\TFMAN\\dest\\"));

	err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\source\\File1.TXT"), _L("\\F32-TST\\TFMAN\\dest\\"), CFileMan::ERecurse);
	test_KErrNone(err);

	CheckFileExists(_L("\\F32-TST\\TFMAN\\source\\File1.TXT"), KErrNotFound, ETrue);
	CheckFileExists(_L("\\F32-TST\\TFMAN\\dest\\File1.TXT"),   KErrNone,     ETrue);

	RmDir(_L("\\F32-TST\\TFMAN\\dest\\"));
	RmDir(_L("\\F32-TST\\TFMAN\\source\\"));

	// Test behaviour for omitted parameters
	// For this, default should be session path
	TFileName sessionPath;
	err=TheFs.SessionPath(sessionPath);
	test_KErrNone(err);

	SetupDirectories(ETrue, NULL);
	err=TheFs.SetSessionPath(_L("\\F32-TST\\TFMAN\\dest\\"));
	test_KErrNone(err);

	err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\source"), _L(""), CFileMan::ERecurse);
	test_KErrNone(err);
	Compare(_L("\\F32-TST\\TFMAN\\compare\\*"), _L("\\F32-TST\\TFMAN\\dest\\source\\*"));

	RmDir(_L("\\F32-TST\\TFMAN\\dest\\"));
	RmDir(_L("\\F32-TST\\TFMAN\\source\\"));
	SetupDirectories(ETrue, NULL);
	err=TheFs.SetSessionPath(_L("\\F32-TST\\TFMAN\\source\\"));
	test_KErrNone(err);

	err = gFileMan->Move(_L(""), _L("\\F32-TST\\TFMAN\\dest\\"), CFileMan::ERecurse);
	test_KErrNone(err);
	Compare(_L("\\F32-TST\\TFMAN\\compare\\*"), _L("\\F32-TST\\TFMAN\\dest\\*"));
		
	err=TheFs.SetSessionPath(sessionPath);
	test_KErrNone(err);

	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_FMAN-0520
	//! @SYMTestType			UT 
	//! @SYMREQ					INC106735
	//! @SYMTestCaseDesc		Test that CFileMan::Move() (recursive mode) works properly when the destination
	//!							directory is sub-directory of the source directory. 
	//!							(e.g. "C:SRC\\*.TXT" -> "C:\\SRC\\SUB\\")
	//! @SYMTestActions			Move, copy files recursively from source directory to one of its sub-directory,
	//!							with or without wildcards applied.
	//! @SYMTestExpectedResults	Completes with no error, file(s) are moved or copied properly, and no redundant
	//!							movings or copyings are made for files in destination directory.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented 
	//--------------------------------------------- 	
	test.Next(_L("Test recursive moving and copying to sub-directories"));
	// Testing recursive Move() without wildcard
	SetupDirectoriesForCase0520();
	SetupDirectoriesForCase0520Compare1();
	if (!gAsynch)
		err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC106735\\F_ROOT.TXT"), _L("\\F32-TST\\TFMAN\\INC106735\\SUB\\*"), CFileMan::ERecurse);
	else
		err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC106735\\F_ROOT.TXT"), _L("\\F32-TST\\TFMAN\\INC106735\\SUB\\*"), CFileMan::ERecurse, gStat);
	TestResult(err, KErrNone);
	Compare(_L("\\F32-TST\\TFMAN\\INC106735\\"), _L("\\F32-TST\\TFMAN\\INC106735_COM\\"));

	// Testing recursive Move() with wildcard
	SetupDirectoriesForCase0520();
	SetupDirectoriesForCase0520Compare2();
	if (!gAsynch)
		err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC106735\\*.TXT"), _L("\\F32-TST\\TFMAN\\INC106735\\SUB\\"), CFileMan::ERecurse);
	else
		err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\INC106735\\*.TXT"), _L("\\F32-TST\\TFMAN\\INC106735\\SUB\\"), CFileMan::ERecurse, gStat);
	TestResult(err, KErrNone);
	Compare(_L("\\F32-TST\\TFMAN\\INC106735\\"), _L("\\F32-TST\\TFMAN\\INC106735_COM\\"));

	// Testing recursive Copy() without wildcard
	SetupDirectoriesForCase0520();
	SetupDirectoriesForCase0520Compare3();
	if (!gAsynch)
		err = gFileMan->Copy(_L("\\F32-TST\\TFMAN\\INC106735\\F_ROOT.TXT"), _L("\\F32-TST\\TFMAN\\INC106735\\SUB\\"), CFileMan::ERecurse);
	else
		err = gFileMan->Copy(_L("\\F32-TST\\TFMAN\\INC106735\\F_ROOT.TXT"), _L("\\F32-TST\\TFMAN\\INC106735\\SUB\\"), CFileMan::ERecurse, gStat);
	TestResult(err, KErrNone);
	Compare(_L("\\F32-TST\\TFMAN\\INC106735\\"), _L("\\F32-TST\\TFMAN\\INC106735_COM\\"));

	// Testing recursive Copy() with wildcard
	SetupDirectoriesForCase0520();
	SetupDirectoriesForCase0520Compare4();
	if (!gAsynch)
		err = gFileMan->Copy(_L("\\F32-TST\\TFMAN\\INC106735\\*.TXT"), _L("\\F32-TST\\TFMAN\\INC106735\\SUB\\"), CFileMan::ERecurse);
	else
		err = gFileMan->Copy(_L("\\F32-TST\\TFMAN\\INC106735\\*.TXT"), _L("\\F32-TST\\TFMAN\\INC106735\\SUB\\"), CFileMan::ERecurse, gStat);
	TestResult(err, KErrNone);
	Compare(_L("\\F32-TST\\TFMAN\\INC106735\\"), _L("\\F32-TST\\TFMAN\\INC106735_COM\\"));

	TestINC089638(); // Test all items removed from source directory after recursive moving
	TestINC101379(); // Test moving of directory to its subdirectory recursively and not recursively
	TestINC099600(); // and INC101061, Test move files from internal drive to external with system
					 // 			   and hidden attributes
	}


//
// A complex test directory structure...
//
LOCAL_D const TInt KNumFiles = 8;
LOCAL_D const TFileName complexFile[] = 
	{
	_L("\\F32-TST\\TFMAN\\complex\\dir1\\file1.txt"),
	_L("\\F32-TST\\TFMAN\\complex\\dir1\\file2.txt"),
	_L("\\F32-TST\\TFMAN\\complex\\dir1\\subdir1\\file3.txt"),
	_L("\\F32-TST\\TFMAN\\complex\\dir1\\subdir1\\file4.txt"),
	_L("\\F32-TST\\TFMAN\\complex\\dir12\\dir1\\file1.txt"),
	_L("\\F32-TST\\TFMAN\\complex\\dir12\\dir1\\file2.txt"),
	_L("\\F32-TST\\TFMAN\\complex\\dir12\\dir1\\subdir1\\file3.txt"),
	_L("\\F32-TST\\TFMAN\\complex\\dir12\\dir1\\subdir1\\file4.txt")
	};

//
// The expected result of moving the directory complex\\dir1 into complex\\dir2\\ *without* EOverWrite
//
LOCAL_D const TInt KNumFilesResult1 = 8;
LOCAL_D const TFileName complexResult1[] = 
	{
	_L("\\F32-TST\\TFMAN\\complex_result1\\dir1\\file1.txt"),
	_L("\\F32-TST\\TFMAN\\complex_result1\\dir1\\file2.txt"),
	_L("\\F32-TST\\TFMAN\\complex_result1\\dir1\\subdir1\\file3.txt"),
	_L("\\F32-TST\\TFMAN\\complex_result1\\dir1\\subdir1\\file4.txt"),
	_L("\\F32-TST\\TFMAN\\complex_result1\\dir12\\dir1\\file1.txt"),
	_L("\\F32-TST\\TFMAN\\complex_result1\\dir12\\dir1\\file2.txt"),
	_L("\\F32-TST\\TFMAN\\complex_result1\\dir12\\dir1\\subdir1\\file3.txt"),
	_L("\\F32-TST\\TFMAN\\complex_result1\\dir12\\dir1\\subdir1\\file4.txt")
	};

//
// The expected result of moving the directory complex\\dir1 into complex\\dir2\\ *with* EOverWrite
//
LOCAL_D const TInt KNumFilesResult2 = 4;
LOCAL_D const TFileName complexResult2[] = 
	{
	_L("\\F32-TST\\TFMAN\\complex_result2\\dir12\\dir1\\file1.txt"),
	_L("\\F32-TST\\TFMAN\\complex_result2\\dir12\\dir1\\file2.txt"),
	_L("\\F32-TST\\TFMAN\\complex_result2\\dir12\\dir1\\subdir1\\file3.txt"),
	_L("\\F32-TST\\TFMAN\\complex_result2\\dir12\\dir1\\subdir1\\file4.txt"),

	};


LOCAL_C void TestRecursiveMoveAcrossDrives()
//
// Test recursive move across drives
//
	{
	test.Next(_L("Test recursive move across drives"));

	TFileName trgDir   = _L("\\F32-TST\\TFMAN\\RECMOVE2\\");
	TFileName trgSpec  = _L("\\F32-TST\\TFMAN\\RECMOVE2\\*");

	if (gSessionPath[0]=='C')
		{
#if !defined(__WINS__)
        if (!gSecDriveReady)
            {
            test.Printf(_L("Second drive not available for test, skip..."));
            return;
            }
		trgDir	   = _L("?:\\F32-TST\\TFMAN\\RECMOVE2\\");
		trgSpec	   = _L("?:\\F32-TST\\TFMAN\\RECMOVE2\\*");     
        trgDir[0] = (TText) gSecDrive;;
        trgSpec[0] = (TText) gSecDrive;;
#else
		trgDir     = _L("Y:\\F32-TST\\TFMAN\\RECMOVE2\\");
		trgSpec    = _L("Y:\\F32-TST\\TFMAN\\RECMOVE2\\*");
#endif
		}

	RmDir(trgDir);

	MakeFile(_L("C:\\F32-TST\\TFMAN\\RECMOVE\\FULL\\FILE2.PLP"));
	MakeFile(_L("C:\\F32-TST\\TFMAN\\RECMOVE\\FULL\\FILE3.PLP"));
	MakeFile(_L("C:\\F32-TST\\TFMAN\\RECMOVE\\FULL\\GRAPE.TXT"));
	MakeFile(_L("C:\\F32-TST\\TFMAN\\RECMOVE\\GRAPE.TXT"));
	MakeFile(_L("C:\\F32-TST\\TFMAN\\RECMOVE\\FILE1.PLP"));

	TInt err;
	if (!gAsynch)
		err=gFileMan->Move(_L("C:\\F32-TST\\TFMAN\\RECMOVE\\*.PLP"),trgDir,CFileMan::ERecurse);
	else
		err=gFileMan->Move(_L("C:\\F32-TST\\TFMAN\\RECMOVE\\*.PLP"),trgDir,CFileMan::ERecurse,gStat);
	test.Printf(_L("TestRecursiveMoveAcrossDrives(),gFileMan->Move(),err=%d\n"),err);
	TestResult(err);

	RmDir(_L("C:\\F32-TST\\TFMAN\\after\\"));
	MakeFile(_L("C:\\F32-TST\\TFMAN\\AFTER\\FULL\\FILE2.PLP"));
	MakeFile(_L("C:\\F32-TST\\TFMAN\\AFTER\\FULL\\FILE3.PLP"));
	MakeFile(_L("C:\\F32-TST\\TFMAN\\AFTER\\FILE1.PLP"));
	Compare(_L("C:\\F32-TST\\TFMAN\\after\\*"),trgSpec);
	RmDir(_L("C:\\F32-TST\\TFMAN\\AFTER\\"));
	RmDir(_L("C:\\F32-TST\\TFMAN\\RECMOVE\\"));
	
    TFileName destOtherDrive;
	//
	// Test moving empty directories (DEF073924)
	//
	test.Next(_L("Test moving empty directories"));
	
	if (gSecDriveReady)
	    {
        SetupDirectories(EFalse, &destOtherDrive);
    
        err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\source\\"), destOtherDrive, CFileMan::ERecurse);
        test_Equal(KErrNotFound, err);	// Expected - directory is empty
    
        // Test that all directories are still present
        TEntry entry;
        err = TheFs.Entry(_L("\\F32-TST\\TFMAN\\source\\"), entry);
        test_KErrNone(err);
        err = TheFs.Entry(destOtherDrive, entry);
        test_KErrNone(err);
	    }
	else
	    {
        test.Printf(_L("Second drive not available for test, skip..."));
	    }
	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_FMAN-0571
	//! @SYMTestType			UT
	//! @SYMREQ					INC108401
	//! @SYMTestCaseDesc		This testcase tests the synchronous and asynchronous move operations 
	//!							exhaustively with flags set as 0, CFileMan::EOverWrite, CFileMan::ERecurse
	//!							on the SAME and ACROSS drives without trailing slash at the end of source
	//!							dir path.
	//! @SYMTestActions			1. Copy directory structures to another directory across drive.
	//! 						2. Copy directory structures to another directory across drive overwriting
	//!							   duplicate files.
	//! 						3. Copy directory structures to another directory across drive.
	//! 						4. Copy directory structures to another directory on same drive.
	//! 						5. Copy directory structures to another directory on same drive overwriting
	//!							   duplicate files.
	//! 						6. Copy directory structures to another directory on same drive.
	//! @SYMTestExpectedResults 1. Completes with no error, the last directory and its contents are moved
	//!							   from the src directory to the destination directory.
	//!							2. Completes with no error, the last directory and its contents are moved
	//!							   from the src directory to the destination directory, duplicate files are updated.
	//!							3. Completes with no error, the last directory and its contents are moved
	//!							   from the src directory to the destination directory.
	//!							4. Completes with no error, the last directory and its contents are moved
	//!							   from the src directory to the destination directory.
	//!							5. Completes with no error, the last directory and its contents are moved
	//!							   from the src directory to the destination directory, duplicate files are updated.
	//!							6. Completes with no error, the last directory and its contents are moved
	//!							   from the src directory to the destination directory.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented
	//--------------------------------------------- 	

	TestINC108401();

	//
	// Test moving when the source directory contains subdirectories (INC074828, INC078800)
	//
	test.Next(_L("Test moving a directory containing subdirectories"));

    if (gSecDriveReady)
        {
        SetupDirectories(ETrue, &destOtherDrive);
        err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\source\\"), destOtherDrive, CFileMan::ERecurse | CFileMan::EOverWrite);
        test_KErrNone(err);
        
        destOtherDrive.Append(_L("*"));
        Compare(_L("\\F32-TST\\TFMAN\\compare\\*"), destOtherDrive);
        }
    else
        {
        test.Printf(_L("Second drive not available for test, skip..."));
        }
	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_FMAN-0161
	//! @SYMTestType			UT 
	//! @SYMREQ					DEF087791
	//! @SYMTestCaseDesc		Test that CFileMan::Move() works when the destination paths does not exist.
	//! @SYMTestActions			Copy directory structures to a non-existant directory on a different drive.
	//! @SYMTestExpectedResults Completes with no error, files are copied and intermediate directories are created.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented 
	//--------------------------------------------- 	
	test.Next(_L("Test moving when the target directory does not exist"));
    
	if (gSecDriveReady)
        {
        SetupDirectories(ETrue, &destOtherDrive);
        
        RmDir(destOtherDrive);
    
        err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\source\\"), destOtherDrive, CFileMan::ERecurse);
        test_KErrNone(err);
    
        Compare(_L("\\F32-TST\\TFMAN\\compare\\*"), destOtherDrive);
    
        SetupDirectories(ETrue, &destOtherDrive);
        
        RmDir(destOtherDrive);
    
        err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\source"), destOtherDrive, CFileMan::ERecurse);
        test_KErrNone(err);
    
        MakeDir(_L("\\F32-TST\\TFMAN\\compare\\subdir\\"));
        destOtherDrive.Append(_L("source\\"));
        Compare(_L("\\F32-TST\\TFMAN\\compare\\*"), destOtherDrive);
        RmDir(_L("\\F32-TST\\TFMAN\\compare\\subdir\\"));
    
        SetupDirectories(ETrue, &destOtherDrive);
    
        RmDir(destOtherDrive);
    
        err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\source\\File1.TXT"), destOtherDrive, CFileMan::ERecurse);
        test_KErrNone(err);
    
        CheckFileExists(_L("\\F32-TST\\TFMAN\\source\\File1.TXT"), KErrNotFound, ETrue);
        destOtherDrive.Append(_L("File1.TXT"));
        CheckFileExists(destOtherDrive, KErrNone, ETrue);
    
        RmDir(destOtherDrive);
        RmDir(_L("\\F32-TST\\TFMAN\\source\\"));
        }
    else
        {
        test.Printf(_L("Second drive not available for test, skip..."));
        }

	//
	// Test recursive move of complex directory structure into itself (INC078759)
	//

	test.Next(_L("Test recursive move of complex directory structure"));

	// Set up the test directory
	TInt level = 0;
	for(level=0; level < KNumFiles; level++)
		{
		err = TheFs.MkDirAll(complexFile[level]);
		test_Value(err, err == KErrNone || err == KErrAlreadyExists);

		RFile file;
		err = file.Create(TheFs, complexFile[level], EFileRead | EFileWrite);
		test_Value(err, err == KErrNone || err == KErrAlreadyExists || err == KErrBadName);
		file.Close();
		}

	//
	// Move directory 'dir1' into 'dir12' *without* overwrite flag set
	//
	//  - This should fail, as 'dir12' already contains a directory called 'dir1'
	//  - No directories should be modified
	//

	err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\complex\\dir1"), _L("\\F32-TST\\TFMAN\\complex\\dir12\\"), CFileMan::ERecurse);
	test_Equal(KErrAlreadyExists, err);

	for(level=0; level < KNumFilesResult1; level++)
		{
		err = TheFs.MkDirAll(complexResult1[level]);
		test_Value(err, err == KErrNone || err == KErrAlreadyExists);

		RFile file;
		err = file.Create(TheFs, complexResult1[level], EFileRead | EFileWrite);
		test_Value(err, err == KErrNone || err == KErrAlreadyExists || err == KErrBadName);
		file.Close();
		}

	Compare(_L("\\F32-TST\\TFMAN\\complex_result1\\*"), _L("\\F32-TST\\TFMAN\\complex\\*"));

	//
	// Move directory 'dir1' into 'dir12' *with* overwrite flag set
	//
	err = gFileMan->Move(_L("\\F32-TST\\TFMAN\\complex\\dir1"), _L("\\F32-TST\\TFMAN\\complex\\dir12\\"), CFileMan::ERecurse | CFileMan::EOverWrite);
	test_KErrNone(err);

	for(level=0; level < KNumFilesResult2; level++)
		{
		err = TheFs.MkDirAll(complexResult2[level]);
		test_Value(err, err == KErrNone || err == KErrAlreadyExists);

		RFile file;
		err = file.Create(TheFs, complexResult2[level], EFileRead | EFileWrite);
		test_Value(err, err == KErrNone || err == KErrAlreadyExists || err == KErrBadName);
		file.Close();
		}

	Compare(_L("\\F32-TST\\TFMAN\\complex_result2\\*"), _L("\\F32-TST\\TFMAN\\complex\\*"));
	
	// ...tidy up files
	for(level=0; level < KNumFiles; level++)
		TheFs.Delete(complexFile[level]);
	for(level=0; level < KNumFilesResult1; level++)
		TheFs.Delete(complexResult1[level]);
	for(level=0; level < KNumFilesResult2; level++)
		TheFs.Delete(complexResult2[level]);

	// ...tidy up directories
	for(level=0; level < KNumFiles; level++)
		TheFs.RmDir(complexFile[level]);
	for(level=0; level < KNumFilesResult1; level++)
		TheFs.RmDir(complexResult1[level]);
	for(level=0; level < KNumFilesResult2; level++)
		TheFs.RmDir(complexResult2[level]);

	TheFs.RmDir(_L("C:\\F32-TST\\TFMAN\\"));
	TheFs.RmDir(_L("C:\\F32-TST\\"));		
	}

class CFileManCopyAllCancel : public CBase, public MFileManObserver
	{
public:
	CFileManCopyAllCancel(CFileMan* aFileMan);
	TControl NotifyFileManStarted();
	TControl NotifyFileManEnded();
	
private:
	CFileMan* iFileMan;
	};


CFileManCopyAllCancel::CFileManCopyAllCancel(CFileMan* aFileMan)
//
// Constructor
//
	{
	__DECLARE_NAME(_S("CFileManCopyAllCancel"));
	iFileMan=aFileMan;
	}

MFileManObserver::TControl CFileManCopyAllCancel::NotifyFileManStarted()
//
// Observer for TestCopyAllCancel tests
//
	{
	return(MFileManObserver::ECancel);
	}

MFileManObserver::TControl CFileManCopyAllCancel::NotifyFileManEnded()
//
// Observer for TestCopyAllCancel tests
//
	{
	return(MFileManObserver::EContinue);
	}
	

	
LOCAL_C void TestCopyAllCancel()
//
// Test copy (all cancel)
//
	{
	test.Next(_L("Test copy all cancel"));
	
	RmDir(_L("\\F32-TST\\TFMAN\\COPYDIR\\*"));
	CFileManCopyAllCancel* fManObserver=new(ELeave) CFileManCopyAllCancel(gFileMan);
	CleanupStack::PushL(fManObserver);
	gFileMan->SetObserver(fManObserver);
	
	MakeDir(_L("\\F32-TST\\TFMAN\\COPYDIR\\"));
	MakeDir(_L("\\F32-TST\\TFMAN\\DELDIR\\DELTEST\\EMPTY\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\NewDir\\ABC.DEF"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE2.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE3.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\DELTEST\\FILE4.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\DELTEST\\EXE1.BIN"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\EXE2.BIN"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\RUMBA1.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\RUMBA2.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\RUMBA3.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\DELDIR\\RUMBA4.TXT"));

	test.Next(_L("Test cancel copy all the files "));
	TInt r;
	
	if (!gAsynch)
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\file?.txt"),_L("\\F32-TST\\TFMAN\\DELDIR\\rumba?.txt"),0);
	else
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\file?.txt"),_L("\\F32-TST\\TFMAN\\DELDIR\\rumba?.txt"),0,gStat);
	TestResult(r,KErrCancel);
	
	if (!gAsynch)
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.txt"),_L("\\F32-TST\\TFMAN\\file1.txt"),0);
	else
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\DELDIR\\FILE1.txt"),_L("\\F32-TST\\TFMAN\\file1.txt"),0,gStat);
	TestResult(r,KErrCancel);
	gFileMan->SetObserver(gObserver);
	CleanupStack::PopAndDestroy();
	}

class CFileManObserverOverWrite : public CBase, public MFileManObserver
	{
public:
	CFileManObserverOverWrite(CFileMan* aFileMan);
	TControl NotifyFileManEnded();
private:
	CFileMan* iFileMan;
	};

CFileManObserverOverWrite::CFileManObserverOverWrite(CFileMan* aFileMan)
//
// Constructor
//
	{
	__DECLARE_NAME(_S("CFileManObserverOverWrite"));
	iFileMan=aFileMan;
	}


MFileManObserver::TControl CFileManObserverOverWrite::NotifyFileManEnded()
//
// Observer for testoverwrite tests
//
	{
	TInt lastError=iFileMan->GetLastError();
	if (lastError!=KErrNone)
		{
		test_Equal(KErrAlreadyExists, lastError);
		if (gAsynch==EFalse)
			{
			TFileName fileName=iFileMan->CurrentEntry().iName;
			test.Printf(_L("     %S already exists\n"),&fileName);
			}
		}
	return(MFileManObserver::EContinue);
	}

class CFileManObserverBytesCopied : public CBase, public MFileManObserver
    {
public:
    CFileManObserverBytesCopied(CFileMan* aFileMan);
    TControl NotifyFileManEnded();
    TControl NotifyFileManOperation();
    TInt iBytesToBeCopied;
private:
    CFileMan* iFileMan;
    TInt iBytesCopied;
    };

CFileManObserverBytesCopied::CFileManObserverBytesCopied(CFileMan* aFileMan)
//
// Constructor
//
    {
    __DECLARE_NAME(_S("CFileManObserverBytesCopied"));
    iFileMan=aFileMan;
    iBytesCopied=0;
    }

MFileManObserver::TControl CFileManObserverBytesCopied::NotifyFileManOperation()
//
// Observer for testBytesCopied tests
//
    {
    TFileName target;
    iFileMan->GetCurrentTarget(target);
    TInt match = target.MatchF(_L("?:\\bytesTransferred"));
    if(match != 0)
        {
        RDebug::Print(_L("CFileManObserverBytesCopied::NotifyFileManOperation - target %s, match %d"),target.PtrZ(),match);
        return MFileManObserver::EAbort;
        }
    
    iBytesCopied += iFileMan->BytesTransferredByCopyStep();
    return(MFileManObserver::EContinue);
    }

MFileManObserver::TControl CFileManObserverBytesCopied::NotifyFileManEnded()
//
// Observer for testBytesCopied  tests
//
    {
    if(iBytesCopied!=iBytesToBeCopied)
        return (MFileManObserver::EAbort);
    
    return(MFileManObserver::EContinue);
    }



LOCAL_C void TestOverWrite()
//
// Test overwrite for copy and rename
//
	{
	test.Next(_L("Test overwrite option"));
	RmDir(_L("\\F32-TST\\TFMAN\\OVERWRITE\\"));
	CFileManObserverOverWrite* fManObserver=new(ELeave) CFileManObserverOverWrite(gFileMan);
	CleanupStack::PushL(fManObserver);
	gFileMan->SetObserver(fManObserver);

	TBuf8<128> contentsFile1=_L8("Test file one contents");
	TBuf8<128> contentsFile2=_L8("Test file two contents");

	MakeFile(_L("\\F32-TST\\TFMAN\\OVERWRITE\\TRG\\FILE1.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\OVERWRITE\\TRG\\FILE2.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\OVERWRITE\\SRC\\FILE1.TXT"),contentsFile1);
	MakeFile(_L("\\F32-TST\\TFMAN\\OVERWRITE\\SRC\\FILE2.TXT"),contentsFile2);

	if (!gAsynch)
		{
		TInt r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\OVERWRITE\\SRC"),_L("\\F32-TST\\TFMAN\\OVERWRITE\\TRG"),0);
		test_Equal(KErrAlreadyExists, r);
		}
	else
		{
		TInt r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\OVERWRITE\\SRC"),_L("\\F32-TST\\TFMAN\\OVERWRITE\\TRG"),0,gStat);
		test_KErrNone(r);
		WaitForResult(KErrAlreadyExists);
		}

	RFile f;
	TInt r=f.Open(TheFs,_L("\\F32-TST\\TFMAN\\OVERWRITE\\TRG\\FILE1.TXT"),EFileRead);
	test_KErrNone(r);
	TBuf8<128> data;
	r=f.Read(data);
	test_KErrNone(r);
	test_Equal(0, data.Length());
	f.Close();

	if (!gAsynch)
		{
		TInt r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\OVERWRITE\\SRC"),_L("\\F32-TST\\TFMAN\\OVERWRITE\\TRG"),CFileMan::EOverWrite);
		test_KErrNone(r);
		}
	else
		{
		TInt r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\OVERWRITE\\SRC"),_L("\\F32-TST\\TFMAN\\OVERWRITE\\TRG"),CFileMan::EOverWrite,gStat);
		test_KErrNone(r);
		WaitForSuccess();
		}

	r=f.Open(TheFs,_L("\\F32-TST\\TFMAN\\OVERWRITE\\TRG\\FILE1.TXT"),EFileRead);
	test_KErrNone(r);
	r=f.Read(data);
	test_KErrNone(r);
	test(data==contentsFile1);
	f.Close();

	RmDir(_L("\\F32-TST\\TFMAN\\OVERWRITE\\TRG\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\OVERWRITE\\TRG\\FILE1.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\OVERWRITE\\TRG\\FILE2.TXT"));

	if (!gAsynch)
		{
	TInt r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\OVERWRITE\\SRC\\*"),_L("\\F32-TST\\TFMAN\\OVERWRITE\\TRG\\*"),0);
		test_Equal(KErrAlreadyExists, r);
		}
	else
		{
		TInt r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\OVERWRITE\\SRC\\*"),_L("\\F32-TST\\TFMAN\\OVERWRITE\\TRG\\*"),0,gStat);
		test_KErrNone(r);
		WaitForResult(KErrAlreadyExists);
		}

	r=f.Open(TheFs,_L("\\F32-TST\\TFMAN\\OVERWRITE\\TRG\\FILE2.TXT"),EFileRead);
	test_KErrNone(r);
	r=f.Read(data);
	test_KErrNone(r);
	test_Equal(0, data.Length());
	f.Close();

	if (!gAsynch)
		{
		TInt r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\OVERWRITE\\SRC\\*"),_L("\\F32-TST\\TFMAN\\OVERWRITE\\TRG\\*"),CFileMan::EOverWrite);
		test_KErrNone(r);
		}
	else
		{
		TInt r=gFileMan->Rename(_L("\\F32-TST\\TFMAN\\OVERWRITE\\SRC\\*"),_L("\\F32-TST\\TFMAN\\OVERWRITE\\TRG\\*"),CFileMan::EOverWrite,gStat);
		test_KErrNone(r);
		WaitForSuccess();
		}

	r=f.Open(TheFs,_L("\\F32-TST\\TFMAN\\OVERWRITE\\TRG\\FILE2.TXT"),EFileRead);
	test_KErrNone(r);
	r=f.Read(data);
	test_KErrNone(r);
	test(data==contentsFile2);
	f.Close();
	gFileMan->SetObserver(gObserver);
	CleanupStack::PopAndDestroy();
	}

LOCAL_C void TestErrorHandling()
//
// Test bad paths etc
//
	{
	test.Next(_L("Test error handling"));
	if (!gAsynch)
		{
		TInt r=gFileMan->Delete(_L("\\F32-TST\\TFMAN\\BADPATH\\*"));
		test_Equal(KErrPathNotFound, r);
		}
	else
		{
		TInt r=gFileMan->Delete(_L("\\F32-TST\\TFMAN\\BADPATH\\*"),0,gStat);
		test_KErrNone(r);
		WaitForResult(KErrPathNotFound);
		}

	CFileMan* fMan=CFileMan::NewL(TheFs);
	TInt r;
	{
	for(TInt drvNum=EDriveA; drvNum<=EDriveZ; ++drvNum)
		{
		TDriveInfo drvInfo;
		if(KErrNone==TheFs.Drive(drvInfo, drvNum) && drvInfo.iType==EMediaNotPresent)
			{
			// found a non-extant drive, test it...
			_LIT(KBad,"?:\\BADPATH\\*");
			TBuf<16> bad(KBad);
			bad[0] = TUint16('A'+drvNum);
			TInt r=fMan->Delete(bad);
			test_Equal(KErrNotReady, r);
			break;
			}
		}
	}
	delete fMan;

	MakeFile(_L("\\ONE\\TWO\\FILE1.TXT"));
	MakeFile(_L("\\ONE\\TWO\\FOUR"));
	test.Next(_L("Test cyclic copy"));
	if (!gAsynch)
		r=gFileMan->Copy(_L("\\ONE\\TWO\\*"),_L("\\ONE\\TWO\\THREE\\"),CFileMan::ERecurse);
	else
		r=gFileMan->Copy(_L("\\ONE\\TWO\\*"),_L("\\ONE\\TWO\\THREE\\"),CFileMan::ERecurse,gStat);
	test_Equal(KErrArgument, r);

	test.Next(_L("Test src name == trg name"));
	if (!gAsynch)
		{
		r=gFileMan->Copy(_L("\\ONE\\TWO\\FOUR"),_L("\\ONE\\TWO\\FOUR")); // default aSwitch=EOverWrite
		test_KErrNone(r);
		r=gFileMan->Copy(_L("\\ONE\\TWO\\FOUR"),_L("\\ONE\\TWO\\FOUR"), 0);
		test_Equal(KErrAlreadyExists, r);
		r=gFileMan->Copy(_L("\\ONE\\TWO\\FOUR"),_L("\\ONE\\TWO\\FOUR"),CFileMan::ERecurse);
		test_Equal(KErrAlreadyExists, r);
		r=gFileMan->Copy(_L("\\ONE\\TWO\\F*R"),_L("\\ONE\\TWO\\FOUR"),CFileMan::ERecurse);
		test_Equal(KErrAlreadyExists, r);
		}
	else
		{
		r=gFileMan->Copy(_L("\\ONE\\TWO\\*.TXT"),_L("\\ONE\\TWO\\*.TXT"),0,gStat);
		test_KErrNone(r);
		WaitForResult(KErrAlreadyExists);
		r=gFileMan->Copy(_L("\\ONE\\TWO\\FOUR"),_L("\\ONE\\TWO\\F*R"),CFileMan::ERecurse,gStat);
		test_KErrNone(r);
        WaitForResult(KErrNone);
		}
	RmDir(_L("\\ONE\\"));

	test.Next(_L("Test copy missing source and path"));
	if (!gAsynch)
		{
		r=gFileMan->Copy(_L("\\ONE\\TWO\\XXXYYY"),_L("\\ONE\\TWO\\asdfsadf"));
		test_Equal(KErrPathNotFound, r);
		r=gFileMan->Copy(_L("\\ONE\\TWO\\XXXYYY"),_L("\\ONE\\TWO\\asdfsadf"),CFileMan::ERecurse);
		test_Equal(KErrPathNotFound, r);
		}
	else
		{
		r=gFileMan->Copy(_L("\\ONE\\TWO\\*.TXT"),_L("\\ONE\\TWO\\*.YYY"),0,gStat);
		test_KErrNone(r);
		WaitForResult(KErrPathNotFound);
		r=gFileMan->Copy(_L("\\ONE\\TWO\\XXXYYY"),_L("\\ONE\\TWO\\asdfsadf"),CFileMan::ERecurse,gStat);
		test_KErrNone(r);
		WaitForResult(KErrPathNotFound);
		}
		
	test.Next(_L("Test copy missing source"));
	if (!gAsynch)
		{
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\*.LPQ"),_L("\\ONE\\TWO\\*.YYY"));
		test_Equal(KErrNotFound, r);
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\XXXYYY"),_L("\\ONE\\TWO\\asdfsadf"),CFileMan::ERecurse);
		test_Equal(KErrNotFound, r);
		}
	else
		{
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\XXXYYY"),_L("\\ONE\\TWO\\asdfsadf"),0,gStat);
		test_KErrNone(r);
		WaitForResult(KErrNotFound);
		r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\XXXYYY"),_L("\\ONE\\TWO\\asdfsadf"),CFileMan::ERecurse,gStat);
		test_KErrNone(r);
		WaitForResult(KErrNotFound);
		}
		
	RmDir(_L("\\EMPTYSRC\\"));
 	MakeDir(_L("\\EMPTYSRC\\"));
 	RmDir(_L("\\EMPTYTRG\\"));
 	MakeDir(_L("\\EMPTYTRG\\"));
 	test.Next(_L("Test copy empty source directory"));
 	if (!gAsynch)
 		{
 		r=gFileMan->Copy(_L("\\EMPTYSRC\\"),_L("\\EMPTYTRG\\"));
 		test_Equal(KErrNotFound, r);
 		r=gFileMan->Copy(_L("\\EMPTYSRC\\"),_L("\\EMPTYTRG\\"), CFileMan::ERecurse);
 		test_Equal(KErrNotFound, r);
 		r=gFileMan->Copy(_L("\\EMPTYSRC"),_L("\\EMPTYTRG\\"));
 		test_Equal(KErrNotFound, r);
 		r=gFileMan->Copy(_L("\\EMPTYSRC"),_L("\\EMPTYTRG\\"), CFileMan::ERecurse);
 		test_Equal(KErrNotFound, r);
 		}
 	else
 		{
 		r=gFileMan->Copy(_L("\\EMPTYSRC\\"),_L("\\EMPTYTRG\\"), 0, gStat);
 		test_KErrNone(r);
 		WaitForResult(KErrNotFound);
 		r=gFileMan->Copy(_L("\\EMPTYSRC\\"),_L("\\EMPTYTRG\\"), CFileMan::ERecurse, gStat);
 		test_KErrNone(r);
 		WaitForResult(KErrNotFound);
 		r=gFileMan->Copy(_L("\\EMPTYSRC"),_L("\\EMPTYTRG\\"), 0, gStat);
 		test_KErrNone(r);
 		WaitForResult(KErrNotFound);
 		r=gFileMan->Copy(_L("\\EMPTYSRC"),_L("\\EMPTYTRG\\"), CFileMan::ERecurse, gStat);
 		test_KErrNone(r);
 		WaitForResult(KErrNotFound);
 		}
 	RmDir(_L("\\EMPTYSRC\\"));
 	RmDir(_L("\\EMPTYTRG\\"));
	

	MakeFile(_L("Dummyfile"));
	test.Next(_L("Test illegal names"));
	r=gFileMan->Attribs(_L(":C:"),0,0,TTime(0),0);
	test_Equal(KErrBadName, r);
	r=gFileMan->Copy(_L(":C:"),_L("newname"),0);
	test_Equal(KErrBadName, r);
	r=gFileMan->Copy(_L("Dummyfile"),_L(":C:"),0);
	test_Equal(KErrBadName, r);
	r=gFileMan->Delete(_L(":C:"),0);
	test_Equal(KErrBadName, r);
	r=gFileMan->Move(_L(":C:"),_L("newname"),0);
	test_Equal(KErrBadName, r);
	r=gFileMan->Move(_L("dummyFile"),_L(":C:"),0);
	test_Equal(KErrBadName, r);
	r=gFileMan->Rename(_L(":C:"),_L("newname"),0);
	test_Equal(KErrBadName, r);
	r=gFileMan->Rename(_L("DummyFile"),_L(":C:"),0);
	test_Equal(KErrBadName, r);
	r=gFileMan->RmDir(_L("\\:C:\\"));
	test_Equal(KErrBadName, r);

	r=gFileMan->Attribs(_L("::C:"),0,0,TTime(0),0);
	test_Equal(KErrBadName, r);
	r=gFileMan->Copy(_L("::C:"),_L("newname"),0);
	test_Equal(KErrBadName, r);
	r=gFileMan->Copy(_L("Dummyfile"),_L("::C:"),0);
	test_Equal(KErrBadName, r);
	r=gFileMan->Delete(_L("::C:"),0);
	test_Equal(KErrBadName, r);
	r=gFileMan->Move(_L("::C:"),_L("newname"),0);
	test_Equal(KErrBadName, r);
	r=gFileMan->Move(_L("dummyFile"),_L("::C:"),0);
	test_Equal(KErrBadName, r);
	r=gFileMan->Rename(_L("::C:"),_L("newname"),0);
	test_Equal(KErrBadName, r);
	r=gFileMan->Rename(_L("DummyFile"),_L("::C:"),0);
	test_Equal(KErrBadName, r);
	r=gFileMan->RmDir(_L("::C:"));
	test_Equal(KErrBadName, r);
	r=TheFs.Delete(_L("DummyFile"));
	test_KErrNone(r);
	// test copying two files with identical names that do not exist
	_LIT(KNonExistent,"\\azzzz.txt");
	r=gFileMan->Copy(KNonExistent,KNonExistent,0);
	test_Equal(KErrNotFound, r);
	}

LOCAL_C void TestNameMangling()
//
// Synchronous test of name mangling
//
	{
	test.Next(_L("Test name mangling"));
	gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\NAMEMANGLER\\TRG\\"));
	MakeDir(_L("\\F32-TST\\TFMAN\\NAMEMANGLER\\TRG\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\NAMEMANGLER\\SRC\\abc.def"));
	MakeFile(_L("\\F32-TST\\TFMAN\\NAMEMANGLER\\SRC\\abcdefghijk.def"));
	MakeFile(_L("\\F32-TST\\TFMAN\\NAMEMANGLER\\SRC\\abc.defgh"));
	MakeFile(_L("\\F32-TST\\TFMAN\\NAMEMANGLER\\SRC\\abcdefghijk.defgh"));
	MakeFile(_L("\\F32-TST\\TFMAN\\NAMEMANGLER\\SRC\\zyx.abc.def"));
	MakeFile(_L("\\F32-TST\\TFMAN\\NAMEMANGLER\\SRC\\zyx.abcdefghijk.def"));
	MakeFile(_L("\\F32-TST\\TFMAN\\NAMEMANGLER\\SRC\\zyx.abc.defgh"));
	MakeFile(_L("\\F32-TST\\TFMAN\\NAMEMANGLER\\SRC\\zyx.abcdefghijk.defgh"));

	TInt r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\NAMEMANGLER\\SRC\\*.*"),_L("\\F32-TST\\TFMAN\\NAMEMANGLER\\TRG\\*.*"));
	test_KErrNone(r);
	Compare(_L("\\F32-TST\\TFMAN\\NAMEMANGLER\\SRC\\*"),_L("\\F32-TST\\TFMAN\\NAMEMANGLER\\TRG\\*"));
	r=gFileMan->Delete(_L("\\F32-TST\\TFMAN\\NAMEMANGLER\\TRG\\*.*"));
	test_KErrNone(r);
	}

LOCAL_C void TestLongNames()
//
// Synchronous test of name mangling
//
	{
#if defined(__WINS__)
	if (gSessionPath[0]=='C')
		return;
#endif

	gFileMan->SetObserver(NULL);
	
	// Format the current drive
	Format(CurrentDrive());
	// Create Session Path
	CreateTestDirectory(_L("\\F32-TST\\TFMAN\\"));
	
	// Create 2 root directories with very long names
	TInt longFileLength = KMaxFileName-4;
	test.Next(_L("Test methods on long names"));
	TFileName longFileNameA;
	longFileNameA.SetLength(longFileLength);
	longFileNameA.Fill('A',longFileLength);
	TFileName longRootDirNameA=_L("\\");
	longRootDirNameA+=longFileNameA;
	longRootDirNameA+=_L("\\");
	TInt r=TheFs.MkDir(longRootDirNameA);
	test_Value(r, r==KErrNone || r==KErrAlreadyExists);
	// Second folder
	TFileName longFileNameB;
	longFileNameB.SetLength(longFileLength);
	longFileNameB.Fill('B',longFileLength);
	TFileName longRootDirNameB=_L("\\");
	longRootDirNameB+=longFileNameB;
	longRootDirNameB+=_L("\\");
	r=TheFs.MkDir(longRootDirNameB);
	test_Value(r, r==KErrNone || r==KErrAlreadyExists);
	
	TInt longFilePtrLength = KMaxFileName-3; // We do not want the trailing backslash
	TPtrC ptrLongFileA(longRootDirNameA.Ptr(),longFilePtrLength);
	TPtrC ptrLongFileB(longRootDirNameB.Ptr(),longFilePtrLength);
	
	// TInt CFileMan::Move(const TDesC& anOld,const TDesC& aNew,TUint aSwitch=EOverWrite);
	// Tries to move a folder with a long name into another
	// This test will return KErrGeneral because the new path will exceed the maximum length
	// See KMaxFileName
	r=gFileMan->Move(ptrLongFileA,ptrLongFileB);
	test_Equal(KErrGeneral, r);
	
	r=gFileMan->RmDir(longRootDirNameA);
	test_KErrNone(r);
	r=gFileMan->Rename(ptrLongFileB,ptrLongFileA);
	test_KErrNone(r);
	r=gFileMan->RmDir(longRootDirNameB);
	test_Equal(KErrPathNotFound, r);
	r=gFileMan->RmDir(longRootDirNameA);
	test_KErrNone(r);

	TFileName longSubDirName=_L("\\Files\\");
	TPtrC longSubDirFileName(longFileNameA.Ptr(),longFilePtrLength-longSubDirName.Length());
	longSubDirName+=longSubDirFileName;
	longSubDirName+=_L("\\");
	r=TheFs.MkDirAll(longSubDirName);
	test_KErrNone(r);

	CDir* dirList;
	r=TheFs.GetDir(longSubDirName,KEntryAttMaskSupported,0,dirList);
	test_KErrNone(r);
	test_Equal(0, dirList->Count());
	delete dirList;

	TPtrC ptrLongSubDirSrc(longSubDirName.Ptr(),longSubDirName.Length()-1);
	TPtrC ptrLongSubDirTrg(longRootDirNameA.Ptr(),longRootDirNameA.Length()-1);
	r=gFileMan->Copy(ptrLongSubDirSrc,ptrLongSubDirTrg);
	test_KErrNone(r);
	r=TheFs.MkDir(longRootDirNameB);
	test_KErrNone(r);
	r=gFileMan->Move(ptrLongSubDirSrc,longRootDirNameB);
	test_Equal(KErrBadName, r);
	r=TheFs.RmDir(longRootDirNameB);
	test_KErrNone(r);
	test_KErrNone(TheFs.RmDir(longSubDirName));
	test_KErrNone(TheFs.RmDir(_L("\\Files\\")));
	gFileMan->SetObserver(gObserver);
	}

LOCAL_C void TestFileAttributes()
//
// Test file attributes are copied and new settings
//
	{
	test.Next(_L("Test file attributes are copied"));
	gFileMan->Delete(_L("\\F32-TST\\TFMAN\\FILEATT\\TRG\\*.*"));
	MakeDir(_L("\\F32-TST\\TFMAN\\FILEATT\\TRG\\"));
	MakeFile(_L("\\F32-TST\\TFMAN\\FILEATT\\SRC\\readonly.def"),KEntryAttReadOnly);
	MakeFile(_L("\\F32-TST\\TFMAN\\FILEATT\\SRC\\readonlyhidden.def"),KEntryAttReadOnly|KEntryAttHidden);
	MakeFile(_L("\\F32-TST\\TFMAN\\FILEATT\\SRC\\hiddensystem.def"),KEntryAttHidden|KEntryAttSystem);
	MakeFile(_L("\\F32-TST\\TFMAN\\FILEATT\\SRC\\systemarchive.def"),KEntryAttArchive|KEntryAttSystem);

	TInt r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\FILEATT\\SRC\\*.*"),_L("\\F32-TST\\TFMAN\\FILEATT\\TRG\\*.*"));
	test_KErrNone(r);
	Compare(_L("\\F32-TST\\TFMAN\\FILEATT\\SRC\\*.*"),_L("\\F32-TST\\TFMAN\\FILEATT\\TRG\\*"));
	r=gFileMan->Attribs(_L("\\F32-TST\\TFMAN\\FILEATT\\SRC\\*"),0,KEntryAttReadOnly,TTime(0));
	test_KErrNone(r);
	r=gFileMan->Attribs(_L("\\F32-TST\\TFMAN\\FILEATT\\TRG\\*"),0,KEntryAttReadOnly,TTime(0));
	test_KErrNone(r);
	r=gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\FILEATT\\"));
	test_KErrNone(r);
	}	

class CFileManObserverContinue : public CBase, public MFileManObserver
	{
public:
	CFileManObserverContinue(CFileMan* aFileMan);
	TControl NotifyFileManEnded();
private:
	CFileMan* iFileMan;
	};


CFileManObserverContinue::CFileManObserverContinue(CFileMan* aFileMan)
//
// Constructor
//
	{
	__DECLARE_NAME(_S("CFileManObserverOverWrite"));
	iFileMan=aFileMan;
	}


MFileManObserver::TControl CFileManObserverContinue::NotifyFileManEnded()
//
// Observer for testoverwrite tests
//
	{
	return(MFileManObserver::EContinue);
	}

LOCAL_C void TestCopyOpenFile()
//
// Copy a file while it is open
//
	{
	test.Next(_L("Test copying open files"));

	CFileManObserverContinue* fManObserver=new(ELeave) CFileManObserverContinue(gFileMan);
	gFileMan->SetObserver(fManObserver);

	TBuf<256> contents;
	TPtrC8 bufPtr;
	CreateLongName(contents,gSeed,256);
	bufPtr.Set((TUint8*)contents.Ptr(),contents.Size());

	MakeFile(_L("\\F32-TST\\TFMAN\\FILECOPY\\asdf.asdf"),bufPtr);
	RFile f;
	TInt r=f.Open(TheFs,_L("\\F32-TST\\TFMAN\\FILECOPY\\asdf.asdf"),EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\FILECOPY\\asdf.asdf"),_L("\\F32-TST\\TFMAN\\FILECOPY\\xxxx.xxxx"));
	test_KErrNone(r);
	f.Close();
	r=f.Open(TheFs,_L("\\F32-TST\\TFMAN\\FILECOPY\\xxxx.xxxx"),EFileRead);
	test_KErrNone(r);
	TBuf8<256*sizeof(TText)> temp;
	r=f.Read(temp);
	test_KErrNone(r);
	test(temp==bufPtr);
	r=f.Read(temp);
	test_KErrNone(r);
	test_Equal(0, temp.Length());
	f.Close();

	r=f.Open(TheFs,_L("\\F32-TST\\TFMAN\\FILECOPY\\asdf.asdf"),EFileRead);
	test_KErrNone(r);
	r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\FILECOPY\\asdf.asdf"),_L("\\F32-TST\\TFMAN\\FILECOPY\\xxxx.xxxx"));
	test_Equal(KErrInUse, r);
	f.Close();

	gFileMan->SetObserver(gObserver);
	r=gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\FILECOPY\\"));
	test_KErrNone(r);
	delete fManObserver;
	}

void TestINC101844()
	{
	test.Next(_L("Test move files and subdirs with different attributes (INC101844)"));
	_LIT(KDest,"C:\\DEST101844\\");
	TBuf<64> source;
	source.Format(_L("%c:\\INC101844\\"), (TUint) gDriveToTest);
	TBuf<64> src;
	RmDir(source);
	RmDir(KDest);
	MakeDir(KDest);
	TInt r;
	
	// Create files and subdirs with different attributes
	src = source;
	src.Append(_L("file1"));
	MakeFile(src, _L8("blah"));
	TheFs.SetAtt(src, KEntryAttReadOnly, 0);
	src = source;
	src.Append(_L("file2"));
	MakeFile(src, _L8("blah"));
	TheFs.SetAtt(src, KEntryAttHidden, 0);
	src = source;
	src.Append(_L("file3"));
	MakeFile(src, _L8("blah"));
	TheFs.SetAtt(src, KEntryAttSystem, 0);
	src = source;
	src.Append(_L("subdir1\\file4"));
	MakeFile(src, _L8("blah"));
	TheFs.SetAtt(src, KEntryAttArchive, 0);
	src = source;
	src.Append(_L("subdir1"));
	TheFs.SetAtt(src, KEntryAttSystem | KEntryAttHidden, KEntryAttArchive);

	// Move directory containing files and subdirs with different attributes
	r = gFileMan->Move(source, KDest, 0);
	test_KErrNone(r);
	
	// Check that the files and subdirs have moved and have the correct attributes
	TEntry entry;
	src = KDest;
	src.Append(_L("file1"));
	r = TheFs.Entry(src, entry);
	test_KErrNone(r);
	test(entry.iAtt&KEntryAttReadOnly);

	src = KDest;
	src.Append(_L("file2"));
	r = TheFs.Entry(src, entry);
	test_KErrNone(r);
	test(entry.iAtt&KEntryAttHidden);

	src = KDest;
	src.Append(_L("file3"));
	r = TheFs.Entry(src, entry);
	test_KErrNone(r);
	test(entry.iAtt&KEntryAttSystem);

	src = source;
	src.Append(_L("subdir1\\"));
	r = gFileMan->Move(src, source, 0);
	test_KErrNone(r);

	r = TheFs.RmDir(src);
	test_KErrNone(r);
	src = source;
	src.Append(_L("file4"));
	r = TheFs.Delete(src);
	test_KErrNone(r);
	r = TheFs.RmDir(source);
	test_KErrNone(r);
	RmDir(KDest);	
	}

LOCAL_C void TestMoveAcrossDrives()
//
// Move a file from C: to the target drive
//
	{
	test.Next(_L("Test move across drives"));

	TFileName trgDrive   = _L("\\");
	TFileName trgFile    = _L("\\Sketch");
	TFileName trgDir     = _L("\\DRIVEMOVE\\");
	TFileName trgDirFile = _L("\\DRIVEMOVE\\Sketch");

	if (gSessionPath[0]=='C')
		{
#if !defined(__WINS__)
        if (!gSecDriveReady)
            {
            test.Printf(_L("Second drive not available for test, skip..."));
            return;
            }
		trgDrive   = _L("?:\\");
		trgFile    = _L("?:\\Sketch");
		trgDir     = _L("?:\\DRIVEMOVE\\");
		trgDirFile = _L("?:\\DRIVEMOVE\\Sketch");
		trgDrive[0] = (TText) gSecDrive;;
        trgFile[0]  = (TText) gSecDrive;
        trgDir[0]   = (TText) gSecDrive;
        trgDirFile[0]  = (TText) gSecDrive;		
#else
		trgDrive   = _L("Y:\\");
		trgFile    = _L("Y:\\Sketch");
		trgDir     = _L("Y:\\DRIVEMOVE\\");
		trgDirFile = _L("Y:\\DRIVEMOVE\\Sketch");
#endif
		}

	RmDir(trgDir);
	RmDir(_L("C:\\F32-TST\\TFMAN\\DRIVEMOVE\\"));

	MakeFile(_L("C:\\F32-TST\\TFMAN\\DRIVEMOVE\\Sketch"));
		
	// Move Sketch from the source to target
	TInt r = gFileMan->Move(_L("C:\\F32-TST\\TFMAN\\DRIVEMOVE\\Sketch"),trgDrive);
	test.Printf(_L("TestMoveAcrossDrives(),gFileMan->Move(),r=%d\n"),r);
	// Check Sketch no longer exists on source drive
	CheckFileExists(_L("C:\\F32-TST\\TFMAN\\DRIVEMOVE\\Sketch"),KErrNotFound);
	// Check Sketch exists on target drive
	CheckFileExists(trgFile,KErrNone);

	MakeFile(_L("C:\\F32-TST\\TFMAN\\DRIVEMOVE\\Sketch"));
	// Move Directory DRIVEMOVE from the source to target
	r = gFileMan->Move(_L("C:\\F32-TST\\TFMAN\\DRIVEMOVE"),trgDrive);
	test.Printf(_L("TestMoveAcrossDrives(),gFileMan->Move(),r=%d\n"),r);
	// Check DRIVEMOVE no longer exists on source drive
	CheckFileExists(_L("C:\\F32-TST\\TFMAN\\DRIVEMOVE\\Sketch"),KErrPathNotFound);
	// Check Sketch exists on target drive
	CheckFileExists(trgDirFile,KErrNone);

	RmDir(trgDir);
	test_KErrNone(TheFs.Delete(trgFile));	

	TestINC101844(); // Test move files and subdirs with different attributes
	}

class CFileManObserverCopyAbort : public CBase, public MFileManObserver
	{
public:
	CFileManObserverCopyAbort(CFileMan* aFileMan);
	TControl NotifyFileManEnded();
	void SetAbortStep(TInt aAbortStep);
private:
	CFileMan* iFileMan;
	TInt iCurrentStep;
	};


CFileManObserverCopyAbort::CFileManObserverCopyAbort(CFileMan* aFileMan)
//
// Constructor
//
	: iFileMan(aFileMan),
	  iCurrentStep(0)
	{
	__DECLARE_NAME(_S("CFileManObserverCopyAbort"));
	}


void CFileManObserverCopyAbort::SetAbortStep(TInt aAbortStep)
//
// Set the step at which to cancel the operation
//
	{
	iCurrentStep = aAbortStep;
	}


MFileManObserver::TControl CFileManObserverCopyAbort::NotifyFileManEnded()
//
// Observer for testoverwrite tests
//
	{
	TInt lastError = iFileMan->GetLastError();
	test_KErrNone(lastError);

	TFileName srcfile;
	iFileMan->GetCurrentSource(srcfile);
	
	TInt action = iFileMan->CurrentAction();
	test_Value(action,  action == CFileMan::EMove   ||
                        action == CFileMan::EDelete ||
                        action == CFileMan::ERmDir);
		
	iCurrentStep--;
	return(iCurrentStep ? MFileManObserver::EContinue : MFileManObserver::EAbort);
	}

LOCAL_C void TestAbortedMoveAcrossDrives()
//
// Move a file from C: to D: or Y:, and test various cancel conditions
//
	{
	test.Next(_L("Test cancel move across drives"));

	const TInt KNumFiles = 5;

	TFileName trgDirRoot = _L("\\F32-TST\\TFMAN\\");
	TFileName trgDirFull = _L("\\F32-TST\\TFMAN\\CANCELMOVE\\");
	TFileName trgDirFile = _L("\\F32-TST\\TFMAN\\CANCELMOVE\\FILE");

	if (gSessionPath[0]=='C')
		{
#if !defined(__WINS__)
        if (!gSecDriveReady)
            {
            test.Printf(_L("Second drive not available for test, skip..."));
            return;
            }
		trgDirRoot = _L("?:\\F32-TST\\TFMAN\\");
		trgDirFull = _L("?:\\F32-TST\\TFMAN\\CANCELMOVE\\");
        trgDirFile = _L("?:\\F32-TST\\TFMAN\\CANCELMOVE\\FILE");
        trgDirRoot[0] = (TText) gSecDrive;
        trgDirFull[0] = (TText) gSecDrive;
        trgDirFile[0] = (TText) gSecDrive;
#else
		trgDirRoot = _L("Y:\\F32-TST\\TFMAN\\");
		trgDirFull = _L("Y:\\F32-TST\\TFMAN\\CANCELMOVE\\");
        trgDirFile = _L("Y:\\F32-TST\\TFMAN\\CANCELMOVE\\FILE");
#endif
		}

	gFileMan->RmDir(_L("C:\\F32-TST\\TFMAN\\CANCELMOVE\\"));

	CFileManObserverCopyAbort* fManObserver=new(ELeave) CFileManObserverCopyAbort(gFileMan);
	CleanupStack::PushL(fManObserver);

	// Check that source files exist when interrupting the copy step
	TInt step = 0;
	TInt i = 0;
	for(step = 1; step <= KNumFiles+1; ++step)
		{
		for (i = 0; i < KNumFiles; i++)
			{
			TFileName sourceFile =_L("C:\\F32-TST\\TFMAN\\CANCELMOVE\\FILE");
			sourceFile.AppendNum(i);
            sourceFile.Append(_L(".TXT"));
			MakeFile(sourceFile);
			}

		gFileMan->RmDir(trgDirFull);

		fManObserver->SetAbortStep(step);
		gFileMan->SetObserver(fManObserver);
	
		TInt r;
		if (!gAsynch)
			r=gFileMan->Move(_L("C:\\F32-TST\\TFMAN\\CANCELMOVE"),trgDirRoot, CFileMan::EOverWrite);
		else
			r=gFileMan->Move(_L("C:\\F32-TST\\TFMAN\\CANCELMOVE"),trgDirRoot, CFileMan::EOverWrite, gStat);
		
		test.Printf(_L("TestAbortedMoveAcrossDrives(),gFileMan->Move(),r=%d\n"),r);
		TestResult(r, (step <= KNumFiles) ? KErrCancel : KErrNone);

		gFileMan->SetObserver(NULL);
		
		// Check that the expected target files exist...
		CheckFileExists(trgDirFull, KErrNone, EFalse);
		for (i = 0; i < Min(step, KNumFiles); i++)
			{
			TFileName trgAfterFile = trgDirFile;
			trgAfterFile.AppendNum(i);
            trgAfterFile.Append(_L(".TXT"));
			CheckFileExists(trgAfterFile, KErrNone);
			}

		// Check that the expected source files still exist after the abort...
		CheckFileExists(_L("C:\\F32-TST\\TFMAN\\CANCELMOVE\\"), (step <= KNumFiles) ? KErrNone : KErrNotFound, EFalse);
		for (; i < KNumFiles; i++)
			{
			TFileName srcAfterFile =_L("C:\\F32-TST\\TFMAN\\CANCELMOVE\\FILE");
			srcAfterFile.AppendNum(i);
            srcAfterFile.Append(_L(".TXT"));
			CheckFileExists(srcAfterFile, KErrNone);
			}
		}
	
	gFileMan->SetObserver(NULL);
	CleanupStack::PopAndDestroy();
	
	RmDir(trgDirRoot); // "?:\\F32-TST\\TFMAN\\"
	TheFs.RmDir(trgDirRoot.Left(16));
	TheFs.RmDir(trgDirRoot.Left(10));
	TheFs.RmDir(_L("C:\\F32-TST\\TFMAN\\"));
	TheFs.RmDir(_L("C:\\F32-TST\\"));
	}


LOCAL_C void TestMoveEmptyDirectory()
//
//	"Try to move an empty directory C:\F32-TST\TFMAN\DRIVEMOVE\ to C:\"
//
	{
	test.Next(_L("Test move empty directory"));

#if !defined(__WINS__)
    if (!gSecDriveReady)
        {
        test.Printf(_L("Second drive not available for test, skip..."));
        return;
        }
	TFileName trgDrive=_L("?:\\");
	trgDrive[0] = (TText) gSecDrive;
#else
	if (gSessionPath[0]!='C')
		return;
	TFileName trgDrive=_L("C:\\");
#endif

	MakeDir(_L("C:\\F32-TST\\TFMAN\\DRIVEMOVE\\"));
	TInt r=gFileMan->Move(_L("C:\\F32-TST\\TFMAN\\DRIVEMOVE\\*"),trgDrive,CFileMan::ERecurse);
	test.Printf(_L("TestMoveEmptyDirectory(),gFileMan->Move(),r=%d\n"),r);
	test_Value(r, r == KErrNotFound);
	}

LOCAL_C void TestCopyAndRename()
//
// Rename while copying files and directories
//
	{
	test.Next(_L("Test rename while copying files and directories"));
	gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\CPMV"));
	MakeFile(_L("\\F32-TST\\TFMAN\\CPMV\\ONE\\ONE_1.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\CPMV\\ONE\\ONE_2.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\CPMV\\ONE\\ONE_3.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\CPMV\\ONE\\TWOTWO.TWO"));
	MakeFile(_L("\\F32-TST\\TFMAN\\CPMV\\ONE_4.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\CPMV\\ONE_5.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\CPMV\\TWO.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\CPMV\\TWO.GOD"));
	MakeFile(_L("\\F32-TST\\TFMAN\\CPMV\\TWO.BAD"));

	TInt r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\TWO.*"), _L("\\F32-TST\\TFMAN\\THREE.*"), CFileMan::ERecurse);
	test_KErrNone(r);

	CheckFileExists(_L("\\F32-TST\\TFMAN\\CPMV\\THREE.TXT"), KErrNone);
	CheckFileExists(_L("\\F32-TST\\TFMAN\\CPMV\\THREE.GOD"), KErrNone);
	CheckFileExists(_L("\\F32-TST\\TFMAN\\CPMV\\THREE.BAD"), KErrNone);
	r=gFileMan->Delete(_L("\\F32-TST\\TFMAN\\CPMV\\THREE.*"));
	test_KErrNone(r);

	MakeFile(_L("\\F32-TST\\TFMAN\\CPMV\\TWO2\\TWO__1.TXT"));
	MakeFile(_L("\\F32-TST\\TFMAN\\CPMV\\TWO2\\TWO__2.TXT"));

	// copy and rename dir
	r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\CPMV\\TWO2"), _L("\\F32-TST\\TFMAN\\CPMV\\THREE"), CFileMan::ERecurse);
	test_KErrNone(r);
	Compare(_L("\\F32-TST\\TFMAN\\CPMV\\TWO2\\*"), _L("\\F32-TST\\TFMAN\\CPMV\\THREE\\*"));

	// copy and move into another dir
	r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\CPMV\\TWO2"), _L("\\F32-TST\\TFMAN\\CPMV\\THREE\\TWO"), CFileMan::ERecurse);
	test_KErrNone(r);
	Compare(_L("\\F32-TST\\TFMAN\\CPMV\\TWO2\\*"), _L("\\F32-TST\\TFMAN\\CPMV\\THREE\\TWO\\*"));

	// copy and rename files and dirs in current dir
	r=gFileMan->Copy(_L("\\F32-TST\\TFMAN\\CPMV\\TWO*"), _L("\\F32-TST\\TFMAN\\CPMV\\THREE*"), CFileMan::ERecurse);
	test_KErrNone(r);
	//	Compare(_L("\\F32-TST\\TFMAN\\CPMV\\TWO2\\*"), _L("\\F32-TST\\TFMAN\\CPMV\\THREE2\\*"));

	CheckFileExists(_L("\\F32-TST\\TFMAN\\CPMV\\ONE\\THREEO.TWO"), KErrNone);
	CheckFileExists(_L("\\F32-TST\\TFMAN\\CPMV\\THREE\\TWO__1.TXT"), KErrNone);
	CheckFileExists(_L("\\F32-TST\\TFMAN\\CPMV\\THREE\\TWO__2.TXT"), KErrNone);
	CheckFileExists(_L("\\F32-TST\\TFMAN\\CPMV\\THREE.TXT"), KErrNone);
	CheckFileExists(_L("\\F32-TST\\TFMAN\\CPMV\\THREE.BAD"), KErrNone);
	CheckFileExists(_L("\\F32-TST\\TFMAN\\CPMV\\THREE.GOD"), KErrNone);
	CheckFileExists(_L("\\F32-TST\\TFMAN\\CPMV\\THREE\\THREE1.TXT"), KErrNone);
	CheckFileExists(_L("\\F32-TST\\TFMAN\\CPMV\\THREE\\THREE2.TXT"), KErrNone);
	
	gFileMan->RmDir(_L("\\F32-TST\\TFMAN\\CPMV\\"));
	}

void TestStackUsage(TInt aLevel, TThreadStackInfo& aStack)
	{
	// DEF104115
	_LIT(KDir, "\\DEF104115\\");
	_LIT(KFile1,   "\\DEF104115\\file1veryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverylong.txt");
	_LIT(KFile2,   "\\DEF104115\\SUBDIR\\longfile2.txt");
	
	if(aLevel==0)
		{
		test.Next(_L("Test stack usage"));
		RmDir(KDir);
		MakeFile(KFile1, _L8("123456789012345678901234567890"));
		MakeFile(KFile2);
		}
	TInt r = KErrNone;
	char* start = NULL;
	char* end = NULL;
	TInt available = 0;
	TInt stacksize = aStack.iBase - aStack.iLimit;
	start = (char*)aStack.iLimit;
	end = (char*)&stacksize;
	available = (TInt)end - (TInt)start;
#ifdef __X86__
	if(available > 6 * 1024) // X86 uses about twice as much stack as ARM in debug mode, so double the number.
#else
	if(available > 3 * 1024) // don't touch this constant ... fix CFileMan instead!!
#endif
		{
		TestStackUsage(aLevel+1, aStack);
		return;
		}
		
	test.Printf(_L("Level:%d Available:%d\n"), aLevel, available);
	
	gFileMan->SetObserver(NULL);
	// Attribs
	r = gFileMan->Attribs(KDir, 0, 0, 0, CFileMan::ERecurse);
	test_KErrNone(r);

	// Move
	r = gFileMan->Move(KFile1, KFile2, CFileMan::ERecurse);
	test_Equal(KErrAlreadyExists, r);

	r = gFileMan->Move(KFile1, KFile2, CFileMan::ERecurse | CFileMan::EOverWrite);
	test_KErrNone(r);

	// Copy
	r = gFileMan->Copy(KFile2, KFile1, CFileMan::ERecurse);
	test_KErrNone(r);

	// Rename
	r = gFileMan->Rename(KFile1, KFile2, 0);
	test_Equal(KErrAlreadyExists, r);

	r = gFileMan->Rename(KFile1, KFile2, CFileMan::EOverWrite);
	test_KErrNone(r);

	// Delete
	r = gFileMan->Delete(KFile2, CFileMan::ERecurse);
	test_KErrNone(r);

	// RmDir
	r = gFileMan->RmDir(KDir);
	test_KErrNone(r);
	
	gFileMan->SetObserver(gObserver);
	}

LOCAL_C void InitialiseL()
//
// Set up test variables
//
	{
	gFileMan=CFileMan::NewL(TheFs);
	gObserver=new(ELeave) CFileManObserver(gFileMan);
	gFileMan->SetObserver(gObserver);
	gSecDriveReady = GetSecondDrive(gSecDrive);
	}

LOCAL_C void Cleanup()
//
// Cleanup test variables
//
	{
	delete gFileMan;
	delete gObserver;
	}
	
LOCAL_C void SetupDirStructure(TFileName& aSrcPath,TFileName& aTrgPath)
	{
	TFileName tmpName = aSrcPath;
	
	// Create the TrgPath dirs
	MakeDir(aTrgPath);
	
	// Create the aSrcPath dirs
	tmpName = aSrcPath;
	tmpName.Append(_L("EmptyDir01\\"));
	MakeDir(tmpName);
	
	tmpName = aSrcPath;
	tmpName.Append(_L("EmptyDir02\\"));
	MakeDir(tmpName);
	
	tmpName = aSrcPath;
	tmpName.Append(_L("EmptyDir03\\"));
	MakeDir(tmpName);

	tmpName = aSrcPath;
	tmpName.Append(_L("FILE01.TXT"));
	MakeFile(tmpName);
	
	tmpName = aSrcPath;
	tmpName.Append(_L("FILE02.TXT"));
	MakeFile(tmpName);
	
	tmpName = aSrcPath;
	tmpName.Append(_L("FILE03.TXT"));
	MakeFile(tmpName);
	
	tmpName = aSrcPath;
	tmpName.Append(_L("SubDir01\\EmptyDir01\\"));
	MakeDir(tmpName);
	
	tmpName = aSrcPath;
	tmpName.Append(_L("SubDir02\\EmptyDir02\\"));
	MakeDir(tmpName);
	
	tmpName = aSrcPath;
	tmpName.Append(_L("SubDir03\\EmptyDir03\\"));
	MakeDir(tmpName);

	tmpName = aSrcPath;
	tmpName.Append(_L("SubDir01\\FILE01.TXT"));
	MakeFile(tmpName);
	
	tmpName = aSrcPath;
	tmpName.Append(_L("SubDir01\\FILE02.TXT"));
	MakeFile(tmpName);
	
	tmpName = aSrcPath;
	tmpName.Append(_L("SubDir01\\FILE03.TXT"));
	MakeFile(tmpName);
	
	tmpName = aSrcPath;
	tmpName.Append(_L("SubDir02\\FILE01.TXT"));
	MakeFile(tmpName);
	
	tmpName = aSrcPath;
	tmpName.Append(_L("SubDir02\\FILE02.TXT"));
	MakeFile(tmpName);
	
	tmpName = aSrcPath;
	tmpName.Append(_L("SubDir02\\FILE03.TXT"));
	MakeFile(tmpName);
	
	tmpName = aSrcPath;
	tmpName.Append(_L("SubDir03\\FILE01.TXT"));
	MakeFile(tmpName);
	
	tmpName = aSrcPath;
	tmpName.Append(_L("SubDir03\\FILE02.TXT"));
	MakeFile(tmpName);
	
	tmpName = aSrcPath;
	tmpName.Append(_L("SubDir03\\FILE03.TXT"));
	MakeFile(tmpName);
	}
	
LOCAL_C void TestPDEF112148()
	{
	test.Next(_L("Test recursive and non-recursive move across drives (PDEF112148)"));
	
	TInt err = 0;
	
	TFileName trgPath;
	trgPath.Format(_L("%c:\\F32-TST\\TFMAN\\PDEF112148\\dest\\"), (TUint8)gDriveToTest);
	TFileName srcPath = _L("C:\\F32-TST\\TFMAN\\PDEF112148\\src\\");
		
	// Non-Recursive move
	// clean up before testing
	RmDir(srcPath);
	RmDir(trgPath);
	// Setup the directory structure
	SetupDirStructure(srcPath, trgPath);
	if(!gAsynch)
		err = gFileMan->Move(srcPath, trgPath, 0);
	else
	 	err = gFileMan->Move(srcPath, trgPath, 0, gStat);
	TestResult(err);

	// Verify src contents after move operation
	CDir *dir = NULL;
	err = TheFs.GetDir(srcPath, KEntryAttMaskSupported, ESortNone, dir);
	test_Equal(6, dir->Count());
	delete dir;
	// Verify dest contents after move operation
	err = TheFs.GetDir(trgPath, KEntryAttMaskSupported, ESortNone, dir);
	test_Equal(3, dir->Count());
	delete dir;
	
	// Recursive move with "\\" at the end of srcPath
	// clean up before execution	
	RmDir(srcPath);
	RmDir(trgPath);
	// Setup the directory structure
	SetupDirStructure(srcPath, trgPath);
	if(!gAsynch)
		err = gFileMan->Move(srcPath, trgPath, CFileMan::ERecurse);
	else
		err = gFileMan->Move(srcPath, trgPath, CFileMan::ERecurse, gStat);
	TestResult(err);
	
	// Verify src has no content
	err = TheFs.GetDir(srcPath, KEntryAttMaskSupported, ESortNone, dir);
	test_Equal(0, dir->Count());
	delete dir;
	// Verify dest contents after move operation
	err = TheFs.GetDir(trgPath, KEntryAttMaskSupported, ESortNone, dir);
	test_Equal(9, dir->Count());
	delete dir;
	
	// Recursive move without "\\" at the end of srcPath
	// clean up before execution
	RmDir(srcPath);
	RmDir(trgPath);
	// Setup the directory structure
	SetupDirStructure(srcPath, trgPath);
	// Remove the "\\" at the end of srcPath for Recursive Move
	srcPath.Delete(srcPath.Length()-1,1);
	if(!gAsynch)
		err = gFileMan->Move(srcPath, trgPath, CFileMan::ERecurse);
	else
		err = gFileMan->Move(srcPath, trgPath, CFileMan::ERecurse, gStat);
	TestResult(err);
		
	// Add the "\\" at the end of srcPath for verification
	srcPath.Append(KPathDelimiter);
	// Verify src doesnt not exist
	err = gFileMan->RmDir(srcPath);
	test_Equal(KErrPathNotFound, err); // KErrPathNotFound expected as src has been moved to dest
	// Verify dest after move operation
	err = TheFs.GetDir(trgPath, KEntryAttMaskSupported, ESortNone, dir);
	test_Equal(1, dir->Count());
	delete dir;
		
	// clean up before leaving
	RmDir(srcPath);
	RmDir(trgPath);
	}
//---------------------------------------------
//! @SYMTestCaseID			PBASE-T_FMAN-2398
//! @SYMTestType			UT 
//! @SYMREQ					DEF130678
//! @SYMTestCaseDesc		Tests that CFileMan::Move itself does not leak any memory
//! @SYMTestActions			Move files and keep checking the memory usage for each operation.
//! @SYMTestExpectedResults	Test completes without any crash and hence without any memory leak.
//! @SYMTestPriority		High
//! @SYMTestStatus			Implemented
//---------------------------------------------
void TestDEF130678()
	{
	test.Next(_L("Test CFileMan::Move does not leak any memory"));
	_LIT(KFromFile,"C:\\TestDEF130678\\FROM\\from_");
	_LIT(KToFile,"C:\\TestDEF130678\\TO\\");

	TInt run;
	// choose a number that certainly tests all memory allocations.
	// it is used for memory allocation failure simulation.
	TInt maxRun = 50;
	// start OOM loop
	for(run=1; run<=maxRun; ++run)
		{
		TInt err = KErrNone;
		TFileName fromFile, toFile;

		fromFile.Append(KFromFile);
		fromFile.AppendNum(run);
		fromFile.Append(_L(".txt"));
		MakeFile(fromFile);
		
		MakeDir(_L("C:\\TestDEF130678\\FROM\\"));
		MakeDir(_L("C:\\TestDEF130678\\TO\\"));
		toFile.Append(KToFile);
		
		// Check the memory usage
		__UHEAP_MARK;
		// set to fail every run-th memory allocation
		__UHEAP_SETFAIL(RAllocator::EDeterministic, run);

		CFileMan* fileMan = NULL;
		TInt errTrap1 = KErrNone;
		TRAP(errTrap1,(fileMan=CFileMan::NewL(TheFs)));
		if (errTrap1 != KErrNone)
			{
			// reset the memory allocation failure and check for any leak
			__UHEAP_RESET;
			__UHEAP_MARKEND;
			continue;
			}

		CleanupStack::PushL(fileMan);
		TInt errTrap2 = KErrNone;
		TRAP(errTrap2,(err = fileMan->Move(fromFile,toFile)));
		if (errTrap2 != KErrNone || err != KErrNone)
			{
			CleanupStack::PopAndDestroy(fileMan);
			// reset the memory allocation failure and check for any leak
			__UHEAP_RESET;
			__UHEAP_MARKEND;
			continue;
			}
		CleanupStack::PopAndDestroy(fileMan);
		// reset the memory allocation failure and check for any leak
		__UHEAP_RESET;
		__UHEAP_MARKEND;
		} // End of OOM loop
		
	// cleanup
	RmDir(_L("C:\\TestDEF130678\\"));	
	}

void TestBytesTransferredByCopyStep()
    {
    //
    // Test BytesCopied
    //
    test.Next(_L("TestBytesTransferredByCopyStep"));
    (void)gFileMan->Delete(_L("\\bytesTransferred"));
    
    RFile tempFile;
    TFileName tempname;
    TInt r = tempFile.Temp(TheFs,_L("\\"),tempname,EFileWrite);
    test_KErrNone(r);
    r = tempFile.SetSize(50);
    test_KErrNone(r);
    tempFile.Flush();
    tempFile.Close();

    CFileManObserverBytesCopied* fManObserver=new(ELeave) CFileManObserverBytesCopied(gFileMan);
    CleanupStack::PushL(fManObserver);
    gFileMan->SetObserver(fManObserver);
    fManObserver->iBytesToBeCopied=50;
    
    if (!gAsynch)
        {
        r=gFileMan->Copy(tempname,_L("\\bytesTransferred"),CFileMan::EOverWrite);
        test_KErrNone(r);
        }
    else
        {
        TInt r=gFileMan->Copy(tempname,_L("\\bytesTransferred"),CFileMan::EOverWrite,gStat);
        test_KErrNone(r);
        WaitForSuccess();
        }
    
    (void)gFileMan->Delete(_L("\\bytesTransferred"));
    (void)TheFs.Delete(tempname);
    CleanupStack::PopAndDestroy();
    }

void TestGetMoreErrorInfo()
    {
    //
     // Test GetMoreErrorInfo
     //
     test.Next(_L("TestGetMoreErrorInfo"));

     CFileManObserver* fManObserver=new(ELeave) CFileManObserver(gFileMan);
     CleanupStack::PushL(fManObserver);
     gFileMan->SetObserver(fManObserver);
     
     if (!gAsynch)
         {
         TInt r=gFileMan->Copy(_L("\\SRC"),_L("\\TRG"),0);
         if(r!=KErrNone) //correct behaviour
             {
             TFileManError error = gFileMan->GetMoreInfoAboutError();
             test_Equal((TFileManError)ENoFilesProcessed,error);
             }
         else { test_Equal(!KErrNone,r); }
         }
     else
         {
         TInt r=gFileMan->Copy(_L("\\SRC"),_L("\\TRG"),0,gStat);
         if(r!=KErrNone) //correct behaviour
             {
             TFileManError error = gFileMan->GetMoreInfoAboutError();
             test_Equal((TFileManError)ENoFilesProcessed,error);
             }
         else { test_Equal(!KErrNone,r); }
         }
    CleanupStack::PopAndDestroy();
    }

GLDEF_C void CallTestsL()
//
// Do tests
//
	{
	// T_FMAN is independent of all tests so format first
#ifndef __WINS__
	Format(CurrentDrive());
#endif

	InitialiseL();
	CreateTestDirectory(_L("\\F32-TST\\TFMAN\\"));
//	May not be able to test handling of invalid path lengths because F32 fix 
//	to prevent paths >256 ever being created
	testingInvalidPathLengths = CheckIfShortPathsAreSupported();
	
	//-----------------------------------------------------------------------------------
	// Asynchronous tests
	//
	gAsynch=ETrue;
	test.Next(_L("Asynchronous tests ..."));
	TheFs.SetAllocFailure(gAllocFailOff);

	TInt uid;
	test_KErrNone(HAL::Get(HAL::EMachineUid,uid));
	TBool doTargetTests =  (!IsTestingLFFS() && 
							uid!=HAL::EMachineUid_Cogent && 
							uid!=HAL::EMachineUid_IQ80310 && 
							uid!=HAL::EMachineUid_X86PC);

	if (doTargetTests)
		{
		TestMoveAcrossDrives();
		TestRecursiveMoveAcrossDrives();
		TestMoveEmptyDirectory();
		TestAbortedMoveAcrossDrives();
		TestPDEF112148(); // Test recursive and non-recursive move across drives
		}

	TestOverWrite();
	TestRename();
	TestErrorHandling();

	TestRecursiveMove();
	TestRecursiveDelete();
	TestRecursiveAttribs();
	TestRecursiveCopy();
	
	TestRmDir();
	TestSimultaneous();
	TestAttribs();
	TestDelete();
	if (!IsTestingLFFS())
		TestCopy(); // ???
	TestMove();
	TestCopyAllCancel();

	//-----------------------------------------------------------------------------------
	// Synchronous tests
	//
	gAsynch=EFalse;
	test.Next(_L("Synchronous tests ..."));
	TheFs.SetAllocFailure(gAllocFailOn);

	if (doTargetTests)
		{
		TestMoveAcrossDrives();
		TestRecursiveMoveAcrossDrives();
		TestMoveEmptyDirectory();
		TestAbortedMoveAcrossDrives();
		TestPDEF112148(); // Test recursive and non-recursive move across drives
		}

	TestCopyOpenFile();
	if(testingInvalidPathLengths)
		TestLongNames();
	TestNameMangling();
	TestFileAttributes();
	TestOverWrite();
	TestRename();
	TestErrorHandling();
	TestRecursiveMove();
	TestRecursiveDelete();
	TestRecursiveAttribs();
	TestRecursiveCopy();
	TestRmDir();
	TestSimultaneous();
	TestAttribs();
	TestDelete();
	TestCopy();
	TestMove();
	TestCopyAndRename();
	TestCopyAllCancel();
	
	TestDEF130678(); // Test CFileMan::Move does not leak any memory
	TestBytesTransferredByCopyStep();
	TestGetMoreErrorInfo();
#ifndef __WINS__
	RThread t;
	TThreadStackInfo stack;
	test_KErrNone(t.StackInfo(stack));
	TestStackUsage(0, stack);
#endif

	
	Cleanup();
	DeleteTestDirectory();
	test_KErrNone(TheFs.RmDir(_L("\\F32-TST\\")));
	}

