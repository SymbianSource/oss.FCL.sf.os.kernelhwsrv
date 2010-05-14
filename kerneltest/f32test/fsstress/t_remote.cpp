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
// f32test\fsstress\t_remote.cpp
// 
//

#if !defined(__F32FILE_H__)
#include <f32file.h>
#endif
#if !defined(__E32TEST_H__)
#define	__E32TEST_EXTENSION__
#include <e32test.h>
#endif
#if !defined(__T_STD_H__)
#include "t_std.h"
#endif
#if !defined(__T_REMFSY_H__)
#include "t_remfsy.h"
#endif

GLDEF_D RTest test(_L("T_REMOTE - DEFAULT DRIVE"));
GLDEF_D RTest testx(_L("X Drive (WINS) <-> D Drive (MARM)"));
GLDEF_D RTest testy(_L("Y Drive (WINS) <-> C Drive (MARM)"));
GLDEF_D RTest testq(_L("REMOTE Q Drive"));

LOCAL_D TFileName gBatchFile;
LOCAL_D TBool gRunByBatch=EFalse;

GLDEF_D TFileName gExeFileName(RProcess().FileName());

/*
INFORMATION - What this test is all about

T_REMOTE tests the asynchronous remote file system implementation, introduced in
F32 release 110 and refined in release 112.  The test sets up and mounts four drives,
the default drive (usually C on WINS and MARM), a remote drive (Q) and two others, on
MARM these are C and D, on WINS these are X and Y.  The remote filesystem is the only
system treated asynchronously by F32.  

The test sets up a thread for each drive and runs a number of tests on each drive.
In the remote filesystem case, a dummy filesystem has been implemented, this is built
as T_REMFSY.fsy.  The tests are designed to stress a number of fileserver API
functions.  The test MultipleSessions() is adapted from another F32 test T_SESSION.
It sets up an array of fileserver connections and then uses each connection to run
a number of tests - alternating between connections to stress F32 as it does so.  The
test should therefore be testing multiple fileserver connections on multiple drives -
the remote filesystem running concurrently with the other drives.  There should be
no failure or confusion between fileserver sessions.  Each drive has a separate test
console which should complete successfully.

NOTE: TO RUN THIS CARD SUCCESSFULLY ON MARM A CF CARD IS REQUIRED

*/

LOCAL_C void Test0(RTest& aTest)
//
//	Scan for open files - no sessions
//	
	{
	aTest.Next(_L("Scan for open files with no sessions open"));
	CFileList* list;
	TOpenFileScan fileScan(TheFs);
	fileScan.NextL(list);
	if (list==NULL)
		return;
	TInt count=list->Count();
	if (count==1)
		{
		gRunByBatch=ETrue;
		gBatchFile=(*list)[0].iName;
		delete list;
		fileScan.NextL(list);
		if (list==NULL)
			return;
		count=list->Count();
		}
	while (count--)
		{
		TEntry entry=(*list)[count];
		aTest.Printf(_L("%d) EntryName = %S\n"),count,&entry.iName);
		}
	//aTest.Printf(_L("Test will fail unless files are closed.\n"));
	//aTest.Printf(_L("Press any key ...\n"));
	//aTest.Getch();
	}


LOCAL_C void Test1()	
//
//	This test is only called by default drive
//	Test OpenFileScan			
//
	{
	test.Next(_L("Scan for open files - one session only"));

	RFile file1,file2,file3;
	TFileName fn;
	fn = _L("Z:\\TEST\\T_FSRV.CPP");
	fn[0] = gExeFileName[0];
	TInt r=file1.Open(TheFs,fn,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	fn = _L("Z:\\TEST\\T_FILE.CPP");
	fn[0] = gExeFileName[0];
	r=file2.Open(TheFs,fn,EFileRead);
	test_KErrNone(r);
	fn = _L("Z:\\TEST\\T_FSRV.CPP");
	fn[0] = gExeFileName[0];
	r=file3.Open(TheFs,fn,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	
	CFileList* list;
	TOpenFileScan fileScan(TheFs);
	fileScan.NextL(list);

	if (gRunByBatch)
		{
		test(list!=NULL);
		test(list->Count()==1);
		TEntry entry=(*list)[0];
		test(entry.iName.FindF(_L(".BAT"))>=0);
		delete list;
		fileScan.NextL(list);
		}

	test(list!=NULL);
	TInt count=list->Count();
	test(count==3);
	TEntry entry=(*list)[0];
	test(entry.iName.FindF(_L("T_FSRV.CPP"))>=0);
	entry=(*list)[1];
	test(entry.iName.FindF(_L("T_FILE.CPP"))>=0);
	entry=(*list)[2];
	test(entry.iName.FindF(_L("T_FSRV.CPP"))>=0);
	TThreadId threadId=fileScan.ThreadId();
	RThread current;
	TThreadId currentId=current.Id();
	test(threadId==currentId);
	delete list;

	fileScan.NextL(list);
	test(list==NULL);

	file1.Close();
	file2.Close();
	file3.Close();
	}

LOCAL_C void Test2()
//
// Test openfilescan - empty, full, empty.
//
	{
	test.Next(_L("Scan for open files - empty sessions"));

	RFs fs1,fs2,fs3,fs4;
	TInt r=fs1.Connect();
	test_KErrNone(r);
	r=fs2.Connect();
	test_KErrNone(r);
	r=fs3.Connect();
	test_KErrNone(r);
	r=fs4.Connect();
	test_KErrNone(r);

	RFile file1,file2,file3;
	TFileName fn;
	fn = _L("Z:\\TEST\\T_FSRV.CPP");
	fn[0] = gExeFileName[0];
	r=file1.Open(fs2,fn,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	fn = _L("Z:\\TEST\\T_FILE.CPP");
	fn[0] = gExeFileName[0];
	r=file2.Open(fs2,fn,EFileRead);
	test_KErrNone(r);
	fn = _L("Z:\\TEST\\T_FSRV.CPP");
	fn[0] = gExeFileName[0];
	r=file3.Open(fs2,fn,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	
	CFileList* list;
	TOpenFileScan fileScan(TheFs);
	fileScan.NextL(list);

	if (gRunByBatch)
		{
		test(list!=NULL);
		test(list->Count()==1);
		TEntry entry=(*list)[0];
		test(entry.iName.FindF(_L(".BAT"))>=0);
		delete list;
		fileScan.NextL(list);
		}

	test(list!=NULL);
	TInt count=list->Count();
	test(count==3);
	TEntry entry=(*list)[0];
	test(entry.iName.FindF(_L("T_FSRV.CPP"))>=0);
	entry=(*list)[1];
	test(entry.iName.FindF(_L("T_FILE.CPP"))>=0);
	entry=(*list)[2];
	test(entry.iName.FindF(_L("T_FSRV.CPP"))>=0);
	TThreadId threadId=fileScan.ThreadId();
	RThread current;
	TThreadId currentId=current.Id();
	test(threadId==currentId);
	delete list;

	fileScan.NextL(list);
	test(list==NULL);

	file1.Close();
	file2.Close();
	file3.Close();
	fs1.Close();
	fs2.Close();
	fs3.Close();
	fs4.Close();
	}

LOCAL_C void Test3()
//
// Test openfilescan - empty, full, empty full
//
	{
	test.Next(_L("Scan for open files - multiple sessions"));

	RFs fs1,fs2,fs3,fs4;
	TInt r=fs1.Connect();
	test_KErrNone(r);
	r=fs2.Connect();
	test_KErrNone(r);
	r=fs3.Connect();
	test_KErrNone(r);
	r=fs4.Connect();
	test_KErrNone(r);

	RFile file1,file2,file3;
	TFileName fn;
	fn = _L("Z:\\TEST\\T_FSRV.CPP");
	fn[0] = gExeFileName[0];
	r=file1.Open(fs2,fn,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	fn = _L("Z:\\TEST\\T_FILE.CPP");
	fn[0] = gExeFileName[0];
	r=file2.Open(fs2,fn,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	fn = _L("Z:\\TEST\\T_FSRV.CPP");
	fn[0] = gExeFileName[0];
	r=file3.Open(fs2,fn,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	
	r=file1.Open(fs4,fn,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	fn = _L("Z:\\TEST\\T_FILE.CPP");
	fn[0] = gExeFileName[0];
	r=file2.Open(fs4,fn,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	fn = _L("Z:\\TEST\\T_FSRV.CPP");
	fn[0] = gExeFileName[0];
	r=file3.Open(fs4,fn,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	
	CFileList* list;
	TOpenFileScan fileScan(TheFs);
	fileScan.NextL(list);

	if (gRunByBatch)
		{
		test(list!=NULL);
		test(list->Count()==1);
		TEntry entry=(*list)[0];
		test(entry.iName.FindF(_L(".BAT"))>=0);
		delete list;
		fileScan.NextL(list);
		}

	test(list!=NULL);
	TInt count=list->Count();
	test(count==3);
	TEntry entry=(*list)[0];
	test(entry.iName.FindF(_L("T_FSRV.CPP"))>=0);
	entry=(*list)[1];
	test(entry.iName.FindF(_L("T_FILE.CPP"))>=0);
	entry=(*list)[2];
	test(entry.iName.FindF(_L("T_FSRV.CPP"))>=0);
	TThreadId threadId=fileScan.ThreadId();
	RThread current;
	TThreadId currentId=current.Id();
	test(threadId==currentId);
	delete list;

	fileScan.NextL(list);
	test(list!=NULL);
	count=list->Count();
	test(count==3);
	entry=(*list)[0];
	test(entry.iName.FindF(_L("T_FSRV.CPP"))>=0);
	entry=(*list)[1];
	test(entry.iName.FindF(_L("T_FILE.CPP"))>=0);
	entry=(*list)[2];
	test(entry.iName.FindF(_L("T_FSRV.CPP"))>=0);
	threadId=fileScan.ThreadId();
	currentId=current.Id();
	test(threadId==currentId);
	delete list;

	fileScan.NextL(list);
	test(list==NULL);

	file1.Close();
	file2.Close();
	file3.Close();
	fs1.Close();
	fs2.Close();
	fs3.Close();
	fs4.Close();
	}

LOCAL_C void Test4()
//
// Test openfilescan - rdirs, empty, full, empty rdirs.
//
	{
	test.Next(_L("Scan for open files - check RDir sessions are ignored"));

	RFs fs1,fs2,fs3,fs4;
	TFileName fn;
	TInt r=fs1.Connect();
	test_KErrNone(r);
	r=fs2.Connect();
	test_KErrNone(r);
	r=fs3.Connect();
	test_KErrNone(r);
	r=fs4.Connect();
	test_KErrNone(r);

	RDir dir1,dir2,dir3,dir4;
	fn = _L("Z:\\TEST\\*.XDE");
	fn[0] = gExeFileName[0];
	r=dir1.Open(TheFs,fn,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir2.Open(TheFs,fn,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir3.Open(TheFs,fn,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir4.Open(TheFs,fn,KEntryAttMaskSupported);
	test_KErrNone(r);

	RFile file1,file2,file3;
	fn = _L("Z:\\TEST\\T_FSRV.CPP");
	fn[0] = gExeFileName[0];
	r=file1.Open(fs2,fn,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	fn = _L("Z:\\TEST\\T_FILE.CPP");
	fn[0] = gExeFileName[0];
	r=file2.Open(fs2,fn,EFileRead);
	test_KErrNone(r);
	fn = _L("Z:\\TEST\\T_FSRV.CPP");
	fn[0] = gExeFileName[0];
	r=file3.Open(fs2,fn,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	
	RDir dir5,dir6,dir7,dir8;
	fn = _L("Z:\\TEST\\*.XDE");
	fn[0] = gExeFileName[0];
	r=dir5.Open(fs4,fn,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir6.Open(fs4,fn,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir7.Open(fs4,fn,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir8.Open(fs4,fn,KEntryAttMaskSupported);
	test_KErrNone(r);

	CFileList* list;
	TOpenFileScan fileScan(TheFs);
	fileScan.NextL(list);

	if (gRunByBatch)
		{
		test(list!=NULL);
		test(list->Count()==1);
		TEntry entry=(*list)[0];
		test(entry.iName.FindF(_L(".BAT"))>=0);
		delete list;
		fileScan.NextL(list);
		}

	test(list!=NULL);
	TInt count=list->Count();
	test(count==3);
	TEntry entry=(*list)[0];
	test(entry.iName.FindF(_L("T_FSRV.CPP"))>=0);
	entry=(*list)[1];
	test(entry.iName.FindF(_L("T_FILE.CPP"))>=0);
	entry=(*list)[2];
	test(entry.iName.FindF(_L("T_FSRV.CPP"))>=0);
	TThreadId threadId=fileScan.ThreadId();
	RThread current;
	TThreadId currentId=current.Id();
	test(threadId==currentId);
	delete list;

	fileScan.NextL(list);
	test(list==NULL);

	file1.Close();
	file2.Close();
	file3.Close();
	dir1.Close();	dir2.Close();
	dir3.Close();	dir4.Close();
	dir5.Close();	dir6.Close();
	dir7.Close();	dir8.Close();
	fs1.Close();	fs2.Close();
	fs3.Close();	fs4.Close();
	}

LOCAL_C void Test5()
//
// Test OpenFileScan
//
	{

	test.Next(_L("Scan for open files - mixed RDirs and RFiles"));

	RFile file1,file2,file3;
	TFileName fn;
	fn = _L("Z:\\TEST\\T_FSRV.CPP");
	fn[0] = gExeFileName[0];
	TInt r=file1.Open(TheFs,fn,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	fn = _L("Z:\\TEST\\T_FILE.CPP");
	fn[0] = gExeFileName[0];
	r=file2.Open(TheFs,fn,EFileRead);
	test_KErrNone(r);
	fn = _L("Z:\\TEST\\T_FSRV.CPP");
	fn[0] = gExeFileName[0];
	r=file3.Open(TheFs,fn,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	
	RDir dir1,dir2,dir3,dir4;
	fn = _L("Z:\\TEST\\*.XDE");
	fn[0] = gExeFileName[0];
	r=dir1.Open(TheFs,fn,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir2.Open(TheFs,fn,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir3.Open(TheFs,fn,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir4.Open(TheFs,fn,KEntryAttMaskSupported);
	test_KErrNone(r);

	CFileList* list;
	TOpenFileScan fileScan(TheFs);
	fileScan.NextL(list);

	if (gRunByBatch)
		{
		test(list!=NULL);
		test(list->Count()==1);
		TEntry entry=(*list)[0];
		test(entry.iName.FindF(_L(".BAT"))>=0);
		delete list;
		fileScan.NextL(list);
		}

	test(list!=NULL);
	TInt count=list->Count();
	test(count==3);
	TEntry entry=(*list)[0];
	test(entry.iName.FindF(_L("T_FSRV.CPP"))>=0);
	entry=(*list)[1];
	test(entry.iName.FindF(_L("T_FILE.CPP"))>=0);
	entry=(*list)[2];
	test(entry.iName.FindF(_L("T_FSRV.CPP"))>=0);
	TThreadId threadId=fileScan.ThreadId();
	RThread current;
	TThreadId currentId=current.Id();
	test(threadId==currentId);
	delete list;

	fileScan.NextL(list);
	test(list==NULL);

	file1.Close();
	file2.Close();
	file3.Close();
	dir1.Close();
	dir2.Close();
	dir3.Close();
	dir4.Close();
	}



LOCAL_C void MultipleSessions(TInt aDrive, RTest& aTest)
//
//	Create an array of fileserver sessions
//	Create an array of TMultipleSessionTest objects 
//
	
	{
#if defined(UNICODE)
	const TInt maxNumberSessions=10;	
#else
	const TInt maxNumberSessions=20;
#endif

	RFs session[maxNumberSessions];
	TMultipleSessionTest testObject[maxNumberSessions];
	TInt i=0;
	TInt r;

	for (; i<maxNumberSessions; i++)
		{
		r=session[i].Connect();
		test_KErrNone(r);
		testObject[i].Initialise(session[i]);
		testObject[i].SetSessionPath(aDrive);
		testObject[i].RunTests(aTest);		//	Run the set of tests for each session
		}									//	Leave each session open
											

	for (i=0; i<(maxNumberSessions-1); i++)
		{
	//	Alternate tests between open sessions
		testObject[i].testSetVolume(aTest);
		testObject[i+1].testInitialisation(aTest);
		testObject[i].testSubst(aTest);
		testObject[i+1].testInitialisation(aTest);
		testObject[i].testInitialisation(aTest);
		testObject[i].testDriveList(aTest);
		testObject[i].MakeAndDeleteFiles(aTest);
	//	Close session[i] and check that session[i+1] is OK	
		session[i].Close();	
		testObject[i+1].testInitialisation(aTest);
		testObject[i+1].testSetVolume(aTest);
		testObject[i+1].testInitialisation(aTest);
		testObject[i+1].testSubst(aTest);
		testObject[i+1].testDriveList(aTest);
	//	Reconnect session[i]	
		r=session[i].Connect();
		test_KErrNone(r);
		testObject[i].Initialise(session[i]);
		testObject[i].SetSessionPath(aDrive);
		testObject[i].testSetVolume(aTest);
		testObject[i+1].testInitialisation(aTest);
		testObject[i].testSubst(aTest);
		testObject[i+1].testInitialisation(aTest);
		testObject[i].testInitialisation(aTest);
		testObject[i].testDriveList(aTest);
	//	Close session[i+1] and check that session[i] is OK
		session[i+1].Close();	
		testObject[i].testInitialisation(aTest);
		testObject[i].testSetVolume(aTest);
		testObject[i].testInitialisation(aTest);
		testObject[i].testSubst(aTest);
		testObject[i].testDriveList(aTest);
	//	Reconnect session[i+1]	
		r=session[i+1].Connect();
		test_KErrNone(r);
		testObject[i+1].Initialise(session[i+1]);
		testObject[i+1].SetSessionPath(aDrive);
		testObject[i].testSetVolume(aTest);
		testObject[i+1].testInitialisation(aTest);
		testObject[i].testSubst(aTest);
		testObject[i+1].testInitialisation(aTest);
	//	Close session[i] and check that session[i+1] is OK	
		session[i].Close();	
		testObject[i+1].testInitialisation(aTest);
		testObject[i+1].testSetVolume(aTest);
		testObject[i+1].testInitialisation(aTest);
		testObject[i+1].testSubst(aTest);
		testObject[i+1].testDriveList(aTest);
		
		if (i==maxNumberSessions-1)	//	Tidy up by closing remaining open session
			{
			session[i+1].Close();	
			}
		}
	}

GLDEF_C void CallTestsDefaultL(TInt aDrive)
//
// Call tests that may leave
//
	{
	Test0(test);
	Test1();		
	Test2();
	Test3();
	Test4();
	Test5();
	MultipleSessions(aDrive,test);
	}

GLDEF_C void CallTestsXL(TInt aDrive)
//
// Call tests for drive X
//
	{
	Test0(testx);

	RFile file1;
	TFileName fn;
	fn = _L("Z:\\TEST\\T_FSRV.CPP");
	fn[0] = gExeFileName[0];
	TInt r=file1.Open(TheFs,fn,EFileRead|EFileShareReadersOnly);
	testx(r==KErrNone);
	file1.Close();
	MultipleSessions(aDrive,testx);
	}

GLDEF_C void CallTestsYL(TInt aDrive)
//
// Call tests for drive Y
//
	{
	Test0(testy);

	RFile file1;
	TFileName fn;
	fn = _L("Z:\\TEST\\T_FSRV.CPP");
	fn[0] = gExeFileName[0];
	TInt r=file1.Open(TheFs,fn,EFileRead|EFileShareReadersOnly);
	testy(r==KErrNone);
	file1.Close();
	MultipleSessions(aDrive,testy);
	}

GLDEF_C void CallTestsQL(TInt aDrive)
//
// Call tests for remote drive
//
	{

	Test0(testq);
	
	testq.Printf(_L("This may take some time.  Please be patient...\n"));
	
	testq.Next(_L("Test remote drive with multiple sessions"));
	MultipleSessions(aDrive,testq);

	const TInt numberOfTests=10;
	
	TPtrC record[numberOfTests];
	
	TInt i=0;
	for (;i<numberOfTests;i++)
		{
		if (i%2)
			record[i].Set(_L("Hubble_Bubble"));
		else
			record[i].Set(_L("Toil_and_Trouble"));
		}
	
	testq.Next(_L("Create a file 'TEXTFILE.TXT' on the remote drive"));
	RFile f;
	TInt r=f.Replace(TheFs,_L("TEXTFILE.TXT"),0);
	testq(r==KErrNone);
	TFileText textFile;
	textFile.Set(f);
	
	testq.Next(_L("Write to 'TEXTFILE.TXT'"));
	
	for (i=0;i<numberOfTests;i++)
		{
		r=textFile.Write(record[i]);
		testq(r==KErrNone);
		testq.Printf(_L("Write %d completed OK\n"),i+1);
		}
	
	f.Close();

	RFile file1;
	r=file1.Open(TheFs,_L("Q:\\TEST\\T_FSRV.CPP"),EFileRead|EFileShareReadersOnly);
	testq(r==KErrNone);
	file1.Close();	

	}
