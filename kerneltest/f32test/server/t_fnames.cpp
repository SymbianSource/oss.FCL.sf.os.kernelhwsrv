// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_fnames.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include "t_server.h"

GLDEF_D RTest test(_L("T_FNAMES"));

LOCAL_C void TestReturnValue(TInt aReturnValue,TInt anExpectedValue)
//
// Test the return value
//
	{

	if (aReturnValue==anExpectedValue)
		return;
	test.Printf(_L("ERROR: returnVal=%d, expectedVal=%d\n"),aReturnValue,anExpectedValue);
	test(0);
	//test.Printf(_L("Press any key to continue\n"));
	//test.Getch();
	}

LOCAL_C void DoTestName(const TDesC& aName,TInt anError)
//
// Check errors returned
//
	{

	TBuf<32> goodName=_L("\\GOODNAME.TXT");
	TBuf<32> badName=_L("\\< > : \" / |");
	TParse parser;
	TInt r=parser.Set(aName,NULL,NULL);
	test_Value(r, r == KErrNone || r==anError);
	goodName.Insert(0,parser.Drive());
	badName.Insert(0,parser.Drive());
	
	
	TBuf<KMaxFileName> name;	//	We need an 8-bit name to test RFs::GetLongName() in 
	name.Copy(aName);			//	both builds

	TFileName dummy;			//	To use as an argument to RFs::GetLongName() will store the 
	TBuf<12> dummyShortName;	//	To use as an argument to RFs::GetShortName()
	TUint dumVal;
	TTime dumTime;
	TBool dumAnswer;
	TEntry dumEntry;
	CDir* dumDir;
	TUidType dumUid;
	TInt dumInt=0;
	TFileName badPath=aName;
	badPath.Append(_L("\\"));

//	Test MKDIR with filename containing \000
	TBuf<32> emptyName;
	emptyName.Format(_L("\\%c\\"),0);
	emptyName.Insert(0,parser.Drive());
	r=TheFs.MkDir(emptyName);
	TestReturnValue(r, anError);
	emptyName.Format(_L("\\Big%cGreen\\"),0);
	emptyName.Insert(0,parser.Drive());
	r=TheFs.MkDir(emptyName);
	TestReturnValue(r, anError);
	r=TheFs.SetSessionPath(badPath);
	TestReturnValue(r,KErrBadName); // Do not check drives
	r=TheFs.MkDir(badPath);
	TestReturnValue(r,anError);
	r=TheFs.MkDirAll(badPath);
	TestReturnValue(r,anError);
	r=TheFs.RmDir(badPath);
	TestReturnValue(r,anError);
	r=TheFs.GetDir(aName,dumInt,dumInt,dumDir);
	TestReturnValue(r,anError);
	r=TheFs.GetDir(aName,dumInt,dumInt,dumDir,dumDir);
	TestReturnValue(r,anError);
	r=TheFs.GetDir(aName,dumUid,dumInt,dumDir);
	TestReturnValue(r,anError);
	r=TheFs.Delete(aName);
	TestReturnValue(r,anError);

	r=TheFs.Rename(aName,goodName);
	TestReturnValue(r,anError);
	r=TheFs.Rename(aName,badName);
	TestReturnValue(r,anError);
	r=TheFs.Rename(goodName,aName);
	TestReturnValue(r,anError);
	r=TheFs.Rename(badName,aName);
	TestReturnValue(r,anError);

	r=TheFs.Replace(aName,goodName);
	TestReturnValue(r,anError);
	r=TheFs.Replace(aName,badName);
	TestReturnValue(r,anError);
	r=TheFs.Replace(goodName,aName);
	TestReturnValue(r,anError);
	r=TheFs.Replace(badName,aName);
	TestReturnValue(r,anError);

	r=TheFs.Att(aName,dumVal);
	TestReturnValue(r,anError);
	r=TheFs.SetAtt(aName,dumInt,dumInt);
	TestReturnValue(r,anError);
	r=TheFs.Modified(aName,dumTime);
	TestReturnValue(r,anError);
	r=TheFs.SetModified(aName,dumTime);
	TestReturnValue(r,anError);
	r=TheFs.Entry(aName,dumEntry);
	TestReturnValue(r,anError);
	r=TheFs.SetEntry(aName,dumTime,dumInt,dumInt);
	TestReturnValue(r,anError);
	r=TheFs.IsFileOpen(aName,dumAnswer);
	TestReturnValue(r,anError);

	r=TheFs.GetShortName(aName,dummyShortName);
	TestReturnValue(r,anError);
	r=TheFs.GetLongName(name,dummy);
	TestReturnValue(r,anError);
	
	RFile f;
	r=f.Open(TheFs,aName,EFileWrite);
	TestReturnValue(r,anError);
	r=f.Create(TheFs,aName,EFileWrite);
	TestReturnValue(r,anError);
	r=f.Replace(TheFs,aName,EFileWrite);
	TestReturnValue(r,anError);
	RDir d;
	r=d.Open(TheFs,aName,KEntryAttNormal);
	TestReturnValue(r,anError);
	r=d.Open(TheFs,aName,dumUid);
	TestReturnValue(r,anError);
	}


//-------------------------------------------

/**
    Testing the case when the full length of the directory name is shorter than KMaxFileName but 
    on the emulator because of the conversion epoc "c:\something" to e.g. "d:\epoc32\release\winscw\c\something"
    it gets exactly KMaxFileName lengths, which causes +-1 syndrom problems.
*/
LOCAL_C void DoTestLongDirName1(void)
{
    RFs         rfs;
    RDir        rDir;

    CleanupClosePushL(rfs);    
    CleanupClosePushL(rDir);    
    
    test(rfs.Connect() == KErrNone);
    
    TPtrC dirName(_L("c:\\mainTestDir\\test000\\1a34567890123456789012345678901234567890 1234567890123456789012345678901234567890 1234567890123456789012345678901234567890 1234567890123456789012345678901234567890 1234567890123456789012345678901234567890 1234567890\\"));
    
    //-- TParse shall work correctly
    TParse parse;
    TInt err = parse.Set(dirName,NULL,NULL);
    test_KErrNone(err);
    
    //-- try to access a directory with a long name. This just shall not panic.
    //-- The return code can be any, on the emulator it's very likely to be KErrBadname
    err = rDir.Open(rfs,dirName,EFileStream|EFileWrite|EFileShareExclusive);

    CleanupStack::PopAndDestroy(2); // rfs, rDir
}

/**
    Testing the case of passing a directory name longer than KMaxFileName to the file server.
    KErrBadName shall be the result
*/
LOCAL_C void DoTestLongDirName2(void)
{
    RFs         rfs;
    TBool       bDirExisted=EFalse;

    CleanupClosePushL(rfs);    
    test(rfs.Connect() == KErrNone);
    
    //-- create a dir c:\a
    _LIT(dirName, "C:\\a\\");
    TInt err = rfs.MkDir(dirName);
    test_Value(err, err == KErrNone || err == KErrAlreadyExists);
    
    if(err == KErrAlreadyExists)
        bDirExisted = ETrue;
    
    //-- dir name longer than KMaxFileName
    _LIT(longDirName, "C:\\a\\longnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongnamelongname\\");
    //TInt nLen = dirName().Length();
    
    //-- try to create a directory with a very long name, checking that it doesn't get truncated to the "c:\a"
    err = rfs.MkDir(longDirName);
    test_Value(err, err == KErrBadName);

    //-- clean up, remove created directory, otherwise some ill-designed tests can fail
    if(!bDirExisted)
        rfs.RmDir(dirName);
    
    CleanupStack::PopAndDestroy(1); // rfs
}


//-------------------------------------------

GLDEF_C void CallTestsL()
//
// Check illegal chars
//
	{

	test.Next(_L("Check bad filenames return KErrBadName"));

	DoTestName(_L("\\Name(1/12/97)"),KErrBadName);
	DoTestName(_L("\\Na>me"),KErrBadName);
	DoTestName(_L("\\<Name(1/12/97)"),KErrBadName);
	DoTestName(_L("\\Name:"),KErrBadName);
	DoTestName(_L("\\Na\"me"),KErrBadName);
	DoTestName(_L("\\|"),KErrBadName);
	DoTestName(_L("\\    \\    "),KErrBadName);
	DoTestName(_L("\\:C:"),KErrBadName);

	test.Next(_L("Check bad paths return KErrBadName"));

	DoTestName(_L("\\asdf\\Name(1/12/97)\\asdf.txt"),KErrBadName);
	DoTestName(_L("\\asdf\\Na>me\\asdf.txt"),KErrBadName);
	DoTestName(_L("\\asdf\\<Name(1/12/97)\\asdf.txt"),KErrBadName);
	DoTestName(_L("\\asdf\\Name:\\asdf.txt"),KErrBadName);
	DoTestName(_L("\\asdf\\Na\"me\\asdf.txt"),KErrBadName);
	DoTestName(_L("\\asdf\\|\\asdf.txt"),KErrBadName);
	DoTestName(_L("\\asdf\\    \\asdf.txt"),KErrBadName);

	test.Next(_L("Check directory gets checked first"));
	
	TInt expectedError;
	expectedError=KErrNotReady;

	//The intention here is that even though these are bad file names, the
	//drive it's referencing is not being used.  Hence the first error caught
	//will be KErrNotReady, not KErrBadName.  If S becomes a valid drive letter
	//and you run this test on that drive, it'll fail with KErrBadName.  So
	//you'll need to move the drive letter to an unused one.
	DoTestName(_L("S:\\Name(1/12/97)"),expectedError);
	DoTestName(_L("S:\\Na>me"),expectedError);
	DoTestName(_L("S:\\<Name(1/12/97)"),expectedError);
	DoTestName(_L("S:\\Name:"),expectedError);
	DoTestName(_L("S:\\Na\"me"),expectedError);
	DoTestName(_L("S:\\|"),expectedError);
	DoTestName(_L("S:\\    \\>"),expectedError);
	DoTestName(_L("S:\\asdf\\Name(1/12/97)\\asdf.txt"),expectedError);
	DoTestName(_L("S:\\asdf\\Na>me\\asdf.txt"),expectedError);
	DoTestName(_L("S:\\asdf\\<Name(1/12/97)\\asdf.txt"),expectedError);
	DoTestName(_L("S:\\asdf\\Name:\\asdf.txt"),expectedError);
	DoTestName(_L("S:\\asdf\\Na\"me\\asdf.txt"),expectedError);
	DoTestName(_L("S:\\asdf\\|\\asdf.txt"),expectedError);
	DoTestName(_L("S:\\asdf\\    \\asdf.txt"),expectedError);

    test.Next(_L("Check access to the directory with a long name"));
    DoTestLongDirName1();
    
    test.Next(_L("Check creating a directory longer than 256 symbols"));
    DoTestLongDirName2();
    
    
	}
