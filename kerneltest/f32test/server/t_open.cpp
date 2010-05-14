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
// f32test\server\t_open.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include "t_server.h"

GLDEF_D RTest test(_L("T_OPEN"));

LOCAL_D TFileName gBatchFile;
LOCAL_D TBool gRunByBatch=EFalse;

TFileName filename1=_L("Z:\\TEST\\T_FSRV.CPP");
TFileName filename2=_L("Z:\\TEST\\T_FILE.CPP");
TFileName dirname1=_L("Z:\\TEST\\*.XDE");


LOCAL_C void Test0()
//
// Scan for open files - no sessions
//
	{

	test.Next(_L("Scan for open files with no sessions open"));
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
		test.Printf(_L("%d) EntryName = %S\n"),count,&entry.iName);
		}
	test.Printf(_L("Test will fail unless files are closed.\n"));
	test(0);
	//test.Printf(_L("Press any key ...\n"));
	//test.Getch();
	}

LOCAL_C void Test1()
//
// Test OpenFileScan
//
	{

	test.Next(_L("Scan for open files - one session only"));

	RFile file1,file2,file3;
	
	TInt r=file1.Open(TheFs,filename1,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	
	r=file2.Open(TheFs,filename2,EFileRead);
	test_KErrNone(r);

	r=file3.Open(TheFs,filename1,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	
	CFileList* list=NULL;
	TOpenFileScan fileScan(TheFs);
	TRAP(r,fileScan.NextL(list));
	test_KErrNone(r);

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
	
	r=file1.Open(fs2,filename1,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	
	r=file2.Open(fs2,filename2,EFileRead);
	test_KErrNone(r);
	
	r=file3.Open(fs2,filename1,EFileRead|EFileShareReadersOnly);
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
	
	r=file1.Open(fs2,filename1,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	
	r=file2.Open(fs2,filename2,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	
	r=file3.Open(fs2,filename1,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	
	r=file1.Open(fs4,filename1,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	
	r=file2.Open(fs4,filename2,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	
	r=file3.Open(fs4,filename1,EFileRead|EFileShareReadersOnly);
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
	TInt r=fs1.Connect();
	test_KErrNone(r);
	r=fs2.Connect();
	test_KErrNone(r);
	r=fs3.Connect();
	test_KErrNone(r);
	r=fs4.Connect();
	test_KErrNone(r);

	RDir dir1,dir2,dir3,dir4;
	r=dir1.Open(TheFs,dirname1,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir2.Open(TheFs,dirname1,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir3.Open(TheFs,dirname1,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir4.Open(TheFs,dirname1,KEntryAttMaskSupported);
	test_KErrNone(r);

	RFile file1,file2,file3;
	r=file1.Open(fs2,filename1,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	r=file2.Open(fs2,filename2,EFileRead);
	test_KErrNone(r);
	r=file3.Open(fs2,filename1,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	
	RDir dir5,dir6,dir7,dir8;
	r=dir5.Open(fs4,dirname1,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir6.Open(fs4,dirname1,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir7.Open(fs4,dirname1,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir8.Open(fs4,dirname1,KEntryAttMaskSupported);
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
	TInt r=file1.Open(TheFs,filename1,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	r=file2.Open(TheFs,filename2,EFileRead);
	test_KErrNone(r);
	r=file3.Open(TheFs,filename1,EFileRead|EFileShareReadersOnly);
	test_KErrNone(r);
	
	RDir dir1,dir2,dir3,dir4;
	r=dir1.Open(TheFs,dirname1,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir2.Open(TheFs,dirname1,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir3.Open(TheFs,dirname1,KEntryAttMaskSupported);
	test_KErrNone(r);
	r=dir4.Open(TheFs,dirname1,KEntryAttMaskSupported);
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


NONSHARABLE_STRUCT(TThreadData)
//
// Encapsulates the data required by the worker thread.
//
	{
	// Thread identifier for debug output
	TInt            iNumber;

	// ID of the thread that started the worker thread, and the
	// worker thread itself
	TThreadId       iMain;
	TThreadId       iWorker;

	// Request status object of the parent thread, used for signalling
	TRequestStatus* iStatus;

	// Name of the file the parent thread requires this thread to open
	TFileName       iFilename;

	// Number of files opened by the thread;
	TInt            iNumFiles;
	};


LOCAL_C TInt WorkerThread(TAny* aParameter)
//
// This function is designed to run as a separate thread in order to verify the
// fix for DEF062875.
//
// When the thread is started it opens the file specified in the startup 
// parameter, signals the main thread and then suspends. Once the main thread
// has completed its checking the worker thread is resumed and allowed to run
// to completion.
//
// @param aParameter Thread specific data supplied by the main thread when the
//                   worker thread is started. The data may be accessed by 
//                   casting this pointer to a TThreadData*
//
	{
	// Can't use our global "test" object here
	RTest myTest(_L("Worker thread"));

	
	// Extract the parameters that this thread will need to use
	TThreadData* threadData = (TThreadData*)aParameter;


	RThread current;
	TThreadId currentId = current.Id(); 

	
	myTest.Printf(_L("WORK%d: Worker thread %d started\n"), threadData->iNumber, threadData->iNumber);
	myTest.Printf(_L("WORK%d:   File:   %S\n"), threadData->iNumber, &threadData->iFilename);
	myTest.Printf(_L("WORK%d:   Thread: %d\n"), threadData->iNumber, (TUint)currentId);
	myTest.Printf(_L("WORK%d:   Parent: %d\n"), threadData->iNumber, (TUint)threadData->iMain);


	// Open the file specified by the parameter passed to us from the main 
	// thread
	RFs myFs;
	myFs.Connect();
	RFile file;
	User::LeaveIfError(file.Open(myFs, threadData->iFilename, EFileRead | EFileShareReadersOnly));

	// Signal the parent thread to continue then wait
	myTest.Printf(_L("WORK%d: Signalling parent thread\n"), threadData->iNumber);
	RThread parent;
	User::LeaveIfError(parent.Open(threadData->iMain));
	parent.RequestComplete(threadData->iStatus, KErrNone);

	
	myTest.Printf(_L("WORK%d: Waiting for parent thread to restart us\n"), threadData->iNumber);
	current.Suspend();


	// Tidy up
	myTest.Printf(_L("WORK%d: Closing file\n"), threadData->iNumber);
	file.Close();

	
	return KErrNone;
	}


LOCAL_C void TestDEF062875()
//
// Verify that TOpenFileScan::ThreadId() returns the ID of the thread that
// opened the file.
//
// The object of the exercise here is to create several worker threads, each
// one will open a file, signal the main thread and then suspend. Once all
// the worker threads have suspended the main thread then uses 
// TOpenFileScan::NextL() to verify that the thread IDs correspond to the
// worker threads that opened each file and not that of the main thread.
//
// The worker threads are then restarted and allowed to terminate naturally by
// running to completion
//
	{
	test.Start(_L("Test TOpenFileScan::ThreadId()"));
	
	const TInt KHeapSize  = 32768;
	
	RThread        thread1;
	RThread        thread2;
	
	TRequestStatus status1;
	TRequestStatus status2;

	TThreadId id = RThread().Id();
	
	TThreadData threadData[3];
	
	threadData[0].iNumber   = 0;
	threadData[0].iMain     = id;
	threadData[0].iWorker   = id;
	threadData[0].iStatus   = 0;
	threadData[0].iFilename = filename1;
	threadData[0].iNumFiles = 2;

	threadData[1].iNumber   = 1;
	threadData[1].iMain     = id;
	threadData[1].iStatus   = &status1;
	threadData[1].iFilename = filename1;
	threadData[1].iNumFiles = 1;

	threadData[2].iNumber   = 2;
	threadData[2].iMain     = id;
	threadData[2].iStatus   = &status2;
	threadData[2].iFilename = filename2;
	threadData[2].iNumFiles = 1;

	TInt numThreads = sizeof(threadData)/sizeof(threadData[0]);


	// Open the files in the MAIN thread.
	RFile file1;
	User::LeaveIfError(file1.Open(TheFs, filename1, EFileRead | EFileShareReadersOnly));

	RFile file2;
	User::LeaveIfError(file2.Open(TheFs, filename2, EFileRead | EFileShareReadersOnly));


	// Create the first worker thread
	test.Printf(_L("MAIN: Creating worker threads\n"));
	thread1.Create(_L("WorkerThread1"), WorkerThread, KDefaultStackSize, KHeapSize, KHeapSize, &threadData[1]);
	threadData[1].iWorker = thread1.Id();
	
	// Start it and wait for it to suspend
	thread1.Logon(status1);
	thread1.Resume();
	test.Printf(_L("MAIN: Waiting for worker thread 1\n"));
	User::WaitForRequest(status1);


	// Create the second worker thread
	thread2.Create(_L("WorkerThread2"), WorkerThread, KDefaultStackSize, KHeapSize, KHeapSize, &threadData[2]);
	threadData[2].iWorker = thread2.Id();
	
	
	// Start it and wait for it to suspend
	thread2.Logon(status2);
	thread2.Resume();
	test.Printf(_L("MAIN: Waiting for worker thread 2\n"));
	User::WaitForRequest(status2);


	// Obtain a list of open files. At this point we should have a single open
	// file, as opened by our worker thread. The thread ID reported by
	// TOpenFileScan should be that of our worker thread rather than the main
	// thread.
	test.Printf(_L("MAIN: Verifying thread ID of open file(s)\n"));
	CFileList* list;
	TOpenFileScan fileScan(TheFs);

	
	TInt count = 0;
	FOREVER
		{
		fileScan.NextL(list);


		// The NULL list indicates we've run out of sessions.
		if(!list)
			{
			break;
			}


		TThreadId threadId=fileScan.ThreadId();
		TThreadData* data = 0;
		for (count = 0; count < numThreads; count++)
			{
			if (threadId == threadData[count].iWorker)
				{
				data = &threadData[count];
				break;
				}
			}


		if (data)
			{
			test.Next(_L("Check number of open files..."));
			test.Printf(_L("MAIN: Number of open files: %d (expecting %d)\n"), list->Count(), data->iNumFiles);
			test(list->Count() == threadData[count].iNumFiles);

			
			test.Next(_L("Check TThreadIds..."));
			test.Printf(_L("MAIN: Main thread ID  : %d\n"), (TUint)data->iMain);
			test.Printf(_L("MAIN: Worker thread ID: %d\n"), (TUint)data->iWorker);
			test.Printf(_L("MAIN: File thread ID  : %d\n"), (TUint)threadId);

			
			TInt loop = 0;
			for (loop = 0; loop < list->Count(); loop++)
				{
				const TEntry& theEntry = (*list)[loop];
				test.Printf(_L("  "));
				test.Printf(theEntry.iName);
				test.Printf(_L("\n"));
				}
			}
		else
			{
			test.Printf(_L("Ignored thread %d\n"), (TUint)threadId);
			}

			
		delete list;
		list = 0;

		
		test.Printf(_L("\n"));
		}


	// Signal the two worker threads to tidy up and run to normal termination
	test.Printf(_L("MAIN: Signalling worker thread 1\n"));
	thread1.Logon(status1);
	thread1.Resume();
	User::WaitForRequest(status1);

	test.Printf(_L("MAIN: Signalling worker thread 2\n"));
	thread2.Logon(status2);
	thread2.Resume();
	User::WaitForRequest(status2);


	// Tidy up and finish
	test.Printf(_L("MAIN: Closing worker thread 1\n"));
	thread1.Close();

	test.Printf(_L("MAIN: Closing worker thread 2\n"));
	thread2.Close();

	file1.Close();
	file2.Close();
	
	test.End();
	}


GLDEF_C void CallTestsL()
//
// Call tests that may leave
//
	{
	filename1[0] = gExeFileName[0];
	filename2[0] = gExeFileName[0];
	dirname1[0] = gExeFileName[0];
	Test0();
	Test1();
	Test2();
	Test3();
	Test4();
	Test5();

	TestDEF062875();
	}
