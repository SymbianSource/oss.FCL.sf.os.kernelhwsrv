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
// f32test\server\t_rcount.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>

#include "../server/t_server.h"

GLDEF_D RTest test(_L("T_RCOUNT"));
const TInt KHeapSize=0x2000;

LOCAL_C void Test1()
//
// Test resource counting success
//
	{

	test.Start(_L("Test resource count success"));
	RFs TheFs;
	TheFs.Connect();
	RFile file;
	TheFs.ResourceCountMarkStart();
	TInt count=TheFs.ResourceCount();
	test(count==0);
	TFileName name=_L("Z:\\Test\\T_FILE.CPP");
	name[0] = gExeFileName[0];
	test.Printf(_L("%S\n"),&name);
	TInt r=file.Open(TheFs,name,EFileRead);
	test_KErrNone(r);
	count=TheFs.ResourceCount();
	test(count==1);
	file.Close();
	count=TheFs.ResourceCount();
	test(count==0);
	TheFs.ResourceCountMarkEnd();
	TheFs.Close();
	}

LOCAL_C TInt TestPanic(TAny*)
//
// The Test thread
//
	{

	User::SetJustInTime(EFalse);
	RFs fs;
	TInt r=fs.Connect();
	test_KErrNone(r);
	RFile file;
	fs.ResourceCountMarkStart();
	TFileName name=_L("Z:\\Test\\T_FILE.CPP");
	name[0] = gExeFileName[0];

	r=file.Open(fs,name,EFileRead);
	test_KErrNone(r);
	TInt count=fs.ResourceCount();
	test(count==1);
	fs.ResourceCountMarkEnd(); // MarkEnd without close
	fs.Close();
	return KErrNone;
	}

LOCAL_C void Test2()
//
// Test resource count failure
//
	{

	test.Next(_L("Test resource count failure"));
	TRequestStatus stat;	
	RThread t;
	TInt r=t.Create(_L("TestPanicThread"),TestPanic,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test_KErrNone(r);
	t.Logon(stat);
	t.Resume();
	User::WaitForRequest(stat);	
	test(t.ExitReason()==CSession2::ESesFoundResCountHeaven);
	test(t.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(t);
	}


_LIT(KDirName1, "c:\\f32-tst");
_LIT(KFileName1, "c:\\crash001.txt");
_LIT(KFileName2, "c:\\crash002.txt");

LOCAL_C TInt SendBadHandle1(TAny *)
//
// Thread is panicked when server leaves when DoFsSubCloseL() calls CObjectIx::AtL().
// Avoid case where thread is panicked by CObjectIx::Remove() because
// ! (i<iHighWaterMark) closing the first file removes all of the CObjects.
//
	{
	RFs fs;
	test(fs.Connect() == KErrNone);

	RFile f1;
	test(f1.Replace(fs, KFileName1, EFileWrite) == KErrNone);
	RFile fc(f1);

	f1.Close();
	fc.Close();									// file server panics thread here

	test(EFalse);
	fs.Close();
	return KErrNone;							// shouldn't reach here
	}


LOCAL_C TInt SendBadHandle2(TAny *)
//
// Thread is panicked when server leaves when DoFsSubClose() calls CObjectIx::AtL().
// Avoid case where thread is panicked by CObjectIx() because ! pR->obj, with handle
// having been already removed.
//
	{
	RFs fs;
	test(fs.Connect() == KErrNone);

	RFile f1;
	test(f1.Replace(fs, KFileName1, EFileWrite) == KErrNone);
	RFile f2;
	test(f2.Replace(fs, KFileName2, EFileWrite) == KErrNone);

	RFile fc(f1);
	f1.Close();
	fc.Close();									// file server panics thread here

	test(EFalse);
	fs.Close();
	return KErrNone;							// shouldn't reach here
	}


LOCAL_C TInt SendBadHandle3(TAny*)
//
// Setup a bad RFile handle then try to write to the file, 
// the thread should get paniced by the file server
//
	{
	RFs f;
	TInt r = f.Connect();
	test_KErrNone(r);

	RFile a;
	r=a.Replace(f,KFileName1,EFileWrite);
	test_KErrNone(r);
	RFile b(a);
	a.Close();
	
	r=b.Write(_L8("testing testing"));
	
	//thread should get paniced here so should not reach this
	
	test(EFalse);
	f.Close();	

	return KErrNone;
	}


LOCAL_C TInt SendBadHandle4(TAny*)
//
// Setup a bad RDir handle then try to read the Directory, 
// the thread should get panniced by the server
//
	{
	RFs f;
	TInt r = f.Connect();
	test_KErrNone(r);

	RDir a;
	r=a.Open(f,KDirName1,KEntryAttNormal);
	test_KErrNone(r);
	RDir b(a);
	a.Close();
	
	TEntryArray dummyarray;
	r=b.Read(dummyarray);
	
	//thread should get paniced here so should not reach this
	
	test(EFalse);
	f.Close();	

	return KErrNone;
	}

LOCAL_C TInt SendBadHandle5(TAny*)
//
// Setup a bad RFormat handle then try to call next on the Format object, 
// the thread should get panniced by the server
//
	{

#if defined(__EPOC32__)
	_LIT(KDrive1, "C:\\");
#else
	_LIT(KDrive1, "X:\\");
#endif

	RFs f;
	TInt r = f.Connect();
	test_KErrNone(r);

	RFormat a;
	TInt count;
	r=a.Open(f,KDrive1,EQuickFormat,count);
	test_KErrNone(r);
	RFormat b(a);
	a.Close();
	
	r=b.Next(count);
	
	//thread should get paniced here so should not reach this
	
	test(EFalse);
	f.Close();	

	return KErrNone;
	}


LOCAL_C TInt SendBadHandle6(TAny*)
//
// Setup a bad RRawDisk handle then try to call read on the raw disk object, 
// the thread should get panniced by the server
//
	{

#if defined(__EPOC32__)
	const TInt KDrive1 = EDriveC;
#else
	const TInt KDrive1 = EDriveX;
#endif
	
	RFs f;
	TInt r = f.Connect();
	test_KErrNone(r);

	RRawDisk a;
    r=a.Open(f,KDrive1);

	test_KErrNone(r);
	RRawDisk b(a);
	a.Close();
	TBuf8<19> buffer;
	r=b.Read(0,buffer);
	
	//thread should get paniced here so should not reach this
	
	test(EFalse);
	f.Close();	

	return KErrNone;
	}





LOCAL_C void Test3()
//
//	Test how the file server copes with bad handles
//
	{
	test.Start(_L("RunTests()"));

	TRequestStatus rs;

	RThread t1;
	User::SetJustInTime(EFalse);
	test(t1.Create(_L("Handle1Test"), SendBadHandle1, KDefaultStackSize, KHeapSize, KHeapSize, NULL) == KErrNone);
	t1.Logon(rs);
	t1.Resume();
	User::WaitForRequest(rs);
	User::SetJustInTime(ETrue);
	test(t1.ExitType() == EExitPanic);
	test(t1.ExitReason() == KErrBadHandle);
	CLOSE_AND_WAIT(t1);

	RThread t2;
	User::SetJustInTime(EFalse);
	test(t2.Create(_L("Handle2Test"), SendBadHandle2, KDefaultStackSize, KHeapSize, KHeapSize, NULL) == KErrNone);
	t2.Logon(rs);
	t2.Resume();
	User::WaitForRequest(rs);
	User::SetJustInTime(ETrue);
	test(t2.ExitType() == EExitPanic);
	test(t2.ExitReason() == KErrBadHandle);
	CLOSE_AND_WAIT(t2);

	RThread t3;
	User::SetJustInTime(EFalse);
	test(t3.Create(_L("Handle2Test"), SendBadHandle3, KDefaultStackSize, KHeapSize, KHeapSize, NULL) == KErrNone);
	t3.Logon(rs);
	t3.Resume();
	User::WaitForRequest(rs);
	User::SetJustInTime(ETrue);
	test(t3.ExitType() == EExitPanic);
	test(t3.ExitReason() == KErrBadHandle);
	CLOSE_AND_WAIT(t3);

	RThread t4;
	User::SetJustInTime(EFalse);
	test(t4.Create(_L("Handle2Test"), SendBadHandle4, KDefaultStackSize, KHeapSize, KHeapSize, NULL) == KErrNone);
	t4.Logon(rs);
	t4.Resume();
	User::WaitForRequest(rs);
	User::SetJustInTime(ETrue);
	test(t4.ExitType() == EExitPanic);
	test(t4.ExitReason() == KErrBadHandle);
	CLOSE_AND_WAIT(t4);

	RThread t5;
	User::SetJustInTime(EFalse);
	test(t5.Create(_L("Handle2Test"), SendBadHandle5, KDefaultStackSize, KHeapSize, KHeapSize, NULL) == KErrNone);
	t5.Logon(rs);
	t5.Resume();
	User::WaitForRequest(rs);
	User::SetJustInTime(ETrue);
	test(t5.ExitType() == EExitPanic);
	test(t5.ExitReason() == KErrBadHandle);
	CLOSE_AND_WAIT(t5);

	RThread t6;
	User::SetJustInTime(EFalse);
	test(t6.Create(_L("Handle2Test"), SendBadHandle6, KDefaultStackSize, KHeapSize, KHeapSize, NULL) == KErrNone);
	t6.Logon(rs);
	t6.Resume();
	User::WaitForRequest(rs);
	User::SetJustInTime(ETrue);
	test(t6.ExitType() == EExitPanic);
	test(t6.ExitReason() == KErrBadHandle);
	CLOSE_AND_WAIT(t6);

	User::SetJustInTime(ETrue);
	test.End();
	
    }


LOCAL_C void DoTestsL()
//
// Call all tests
//
	{
	Test1();
	Test2();
	Test3();
	}

GLDEF_C void CallTestsL(void)
//
// Test resource counting
//
    {

	test.Title();
	test.Start(_L("Starting T_RCOUNT test"));

	DoTestsL();

	test.End();
	test.End();
	test.Close();

	return;
    }

