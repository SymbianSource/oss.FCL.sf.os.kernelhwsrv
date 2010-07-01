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
// f32test\server\t_alert.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32math.h>
#include <e32svr.h>
#include "t_server.h"

GLDEF_D RTest test(_L("T_ALERT"));

LOCAL_D TInt gFileWrites;
LOCAL_D TInt gFileReads;
LOCAL_D TInt gFileCreates;
LOCAL_D TInt gFileDeletes;
LOCAL_D TInt gNotifies;
LOCAL_D TInt gNotifyCancels;
LOCAL_D TInt gMediaChanges;

LOCAL_D const TInt gMaxIteration=100;
LOCAL_D const TInt KHeapSize=0x4000;
LOCAL_D const TInt gMaxDelay=100;
LOCAL_D const TInt gMaxMediaChangeInterval=1000;
LOCAL_D const TInt gUpdateInterval=10000;
LOCAL_D const TInt gMaxFiles=4;
LOCAL_D const TInt gMaxTempSize=1024;
LOCAL_D const TFileName gPathThread1=_L("C:\\F32-TST\\TALERT\\");

// Test2
LOCAL_D const TInt gHeartBeatInterval=10000000;
LOCAL_D TInt gThreadTick;
LOCAL_D TInt gFileCreateFail;

#if defined(__WINS__)
LOCAL_D const TFileName gPathThread0=_L("X:\\F32-TST\\TALERT\\");
LOCAL_D const TFileName gPathThread2=_L("X:\\F32-TST\\TALERT\\");
#else
LOCAL_D TFileName gPathThread0=_L("?:\\F32-TST\\TALERT\\");
LOCAL_D TFileName gPathThread2=_L("?:\\F32-TST\\TALERT\\");
#endif

LOCAL_C void WaitForMedia()
//
// Wait until the media change is serviced
//
	{
	FOREVER
		{
		TInt r=TheFs.MkDir(gPathThread0);
		if (r==KErrNone || r==KErrAlreadyExists)
			break;
		}
	}

LOCAL_C TInt FileAccess(TAny* aPathPtr)
//
// Do lots of file access - ignore all errors
//
	{

	TFileName file[gMaxFiles];
	TFileName sessionPath=*(TDesC*)aPathPtr;
	HBufC8* tempPtr=HBufC8::New(gMaxTempSize);
	HBufC* tempPtrx=HBufC::New(gMaxTempSize);
	TPtr8 temp(tempPtr->Des());
	TPtr tempx(tempPtrx->Des());

	RFs fs;
	fs.Connect();
	fs.SetSessionPath(sessionPath);

	FOREVER
		{
		switch(Math::Rand(gSeed)%4)
			{
		case 0:
			{
			TInt fileNum=Math::Rand(gSeed)%gMaxFiles;
			if (file[fileNum].Length()==0)
				break;
			RFile f;
			TInt r=f.Open(fs,file[fileNum],EFileRead|EFileWrite);
			if (r!=KErrNone)
				break;
			CreateLongName(tempx,gSeed,Math::Rand(gSeed)%gMaxTempSize);
			temp.Copy(tempx);
			r=f.Write(temp);
			if (r==KErrNone)
				gFileWrites++;
			f.Close();
			break;
			}
		case 1:
			{
			TInt fileNum=Math::Rand(gSeed)%gMaxFiles;
			if (file[fileNum].Length()==0)
				break;
			RFile f;
			TInt r=f.Open(fs,file[fileNum],EFileRead|EFileWrite);
			if (r!=KErrNone)
				break;
			r=f.Read(temp);
			if (r==KErrNone)
				gFileReads++;
			f.Close();
			break;
			}
		case 2:
			{
			TInt fileNum=Math::Rand(gSeed)%gMaxFiles;
			if (file[fileNum].Length()!=0)
				break;
			RFile f;
			TInt r=f.Temp(fs,sessionPath,file[fileNum],EFileRead|EFileWrite);
			if (r==KErrNone)
				gFileCreates++;
			f.Close();
			break;
			}
		case 3:
			{
			TInt fileNum=Math::Rand(gSeed)%gMaxFiles;
			if (file[fileNum].Length()==0)
				break;
			TInt r=fs.Delete(file[fileNum]);
			if (r==KErrNone)
				{
				file[fileNum].SetLength(0);
				gFileDeletes++;
				}
			break;
			}
		default:
			break;
			}
		}
	
	//delete tempPtr;
	//delete tempPtrx;
	//return(KErrNone);
	}


LOCAL_C void StartThread0()
//
// Start a thread that reads and writes to D:
//
	{
	
	RThread clientThread;
	TInt r=clientThread.Create(_L("TALERT_Thread0"),FileAccess,0x4000,KHeapSize,KHeapSize,(TAny*)&gPathThread0,EOwnerThread);
	test_KErrNone(r);
	clientThread.Resume();
	clientThread.Close();
	}

LOCAL_C void StartThread1()
//
// Start a thread that reads and writes to C:
//
	{
	
	RThread clientThread;
	TInt r=clientThread.Create(_L("TALERT_Thread1"),FileAccess,0x4000,KHeapSize,KHeapSize,(TAny*)&gPathThread1,EOwnerThread);
	test_KErrNone(r);
	clientThread.Resume();
	clientThread.Close();
	}

LOCAL_C void StartThread2()
//
// Start a thread that reads and writes to D:
//
	{
	
	RThread clientThread;
	TInt r=clientThread.Create(_L("TALERT_Thread2"),FileAccess,0x4000,KHeapSize,KHeapSize,(TAny*)&gPathThread2,EOwnerThread);
	test_KErrNone(r);
	clientThread.Resume();
	clientThread.Close();
	}


LOCAL_C TInt NotifyAccess(TAny*)
//
// Create, wait for and cancel notifiers.
//
	{

	RFs fs;
	fs.Connect();
	TRequestStatus status;

	FOREVER
		{
		switch(Math::Rand(gSeed)%4)
			{
		case 0:
			gNotifies++;
			fs.NotifyChange(ENotifyAll,status);
			User::WaitForRequest(status);
			break;
		case 1:
			gNotifies++;
			fs.NotifyChange(ENotifyEntry,status);
			User::WaitForRequest(status);
			break;
		case 2:
			gNotifyCancels++;
			fs.NotifyChange(ENotifyAll,status);
			User::After(Math::Rand(gSeed)%gMaxDelay);
			fs.NotifyChangeCancel();
			User::WaitForRequest(status);
			break;
		case 3:
			gNotifyCancels++;
			fs.NotifyChange(ENotifyEntry,status);
			User::After(Math::Rand(gSeed)%gMaxDelay);
			fs.NotifyChangeCancel();
			User::WaitForRequest(status);
			break;
			}
		}

	//return(KErrNone);
	}			

LOCAL_C void StartThread3()
//
// Start a thread that creates and waits/cancel RFs::Notifiers
//
	{
	
	RThread clientThread;
	TInt r=clientThread.Create(_L("TALERT_Thread3"),NotifyAccess,0x4000,KHeapSize,KHeapSize,NULL,EOwnerThread);
	test_KErrNone(r);
	clientThread.Resume();
	clientThread.Close();
	}

LOCAL_C TInt MediaChange(TAny*)
//
// Cause media changes a random intervals
//
	{
	gMediaChanges=0;
	FOREVER
		{
		TInt interval=Math::Rand(gSeed)%gMaxMediaChangeInterval;
		User::After(interval);
//		UserSvr::ForceRemountMedia(ERemovableMedia0); // Generate media change	
		++gMediaChanges;
		}

	//return(KErrNone);
	}

LOCAL_C void StartThread4()
//
// Start a thread that creates media changes
//
	{
	
	RThread clientThread;
	TInt r=clientThread.Create(_L("TALERT_Thread4"),MediaChange,0x4000,KHeapSize,KHeapSize,NULL,EOwnerThread);
	test_KErrNone(r);
	clientThread.SetPriority(EPriorityMore);
	clientThread.Resume();
	clientThread.Close();
	}

LOCAL_C void StartClock()
//
// Display ongoing reads/writes. Getch to exit
//
	{

	test.Console()->ClearScreen();
	FOREVER
		{
		User::After(gUpdateInterval);
		test.Console()->SetPos(0,5);
		test.Printf(_L("File writes  = %d   \n"),gFileWrites);
		test.Printf(_L("File reads   = %d   \n"),gFileReads);
		test.Printf(_L("File creates = %d   \n"),gFileCreates);
		test.Printf(_L("File deletes = %d   \n"),gFileDeletes);
		test.Printf(_L("File notifies = %d   \n"),gNotifies);
		test.Printf(_L("File notifies cancelled = %d   \n"),gNotifyCancels);
//		test.Printf(_L("Press any key to exit\n"));
//		TKeyCode keycode=test.Console()->KeyCode();
//		if (keycode!=EKeyNull)
//			break;
		if (gFileWrites>gMaxIteration)
			break;
		}
	}

LOCAL_C void KillThreads()
//
// Kill all threads
//
	{
	test.Printf(_L("+Kill threads"));
	RThread t;
	
	TInt r=t.Open(_L("TALERT_Thread0"),EOwnerThread);
	if(r==KErrNone)
	{
		t.Kill(KErrCancel);
		t.Close();
	}
	else 
		test_Value(r, r == KErrNotFound);

	r=t.Open(_L("TALERT_Thread1"),EOwnerThread);
	if(r==KErrNone)
	{
		t.Kill(KErrCancel);
		t.Close();
	}
	else 
		test_Value(r, r == KErrNotFound);

	r=t.Open(_L("TALERT_Thread2"),EOwnerThread);
	if(r==KErrNone)
	{
		t.Kill(KErrCancel);
		t.Close();
	}
	else 
		test_Value(r, r == KErrNotFound);

	r=t.Open(_L("TALERT_Thread3"),EOwnerThread);
	if(r==KErrNone)
	{
		t.Kill(KErrCancel);
		t.Close();
	}
	else 
		test_Value(r, r == KErrNotFound);
	
	r=t.Open(_L("TALERT_Thread4"),EOwnerThread);
	if(r==KErrNone)
	{
		t.Kill(KErrCancel);
		t.Close();
	}
	else 
		test_Value(r, r == KErrNotFound);
	
/*	TFindThread threadFinder(_L("TALERT_*"));
	FOREVER
		{
		TFullName threadName;
		TInt r=threadFinder.Next(threadName);
		test.Printf(_L("r=%d"),r);
		if (r==KErrNotFound)
			break;
		test_KErrNone(r);
		test.Printf(_L("Killing Thread %S\n"),&threadName);
		RThread t;
		r=t.Open(threadName,EOwnerThread);
		test_KErrNone(r);
		t.Kill(KErrCancel);
		t.Close();
		} */
	test.Printf(_L("-Kill threads"));
	}

LOCAL_C void Test1()
//
// Create lots of threads and change notifiers
//
	{
	test.Next(_L("Create lots of threads and change notifiers"));
	TInt r=TheFs.MkDirAll(gPathThread1);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	r=TheFs.MkDir(gPathThread2);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);

	StartThread0(); // Read and write to D:
	StartThread1(); // Read and write to C:
	StartThread2(); // Read and write to D: (again)
	StartThread3(); // Set RFs::Notifiers
	StartThread4(); // Generate media changes
	StartClock(); // Time how long test has run
	KillThreads();
	}

LOCAL_C TInt ThreadHangTest(TAny*)
//
// Keep writing to the fileserver and setting gThreadTick
//
	{

	RFs fs;
	fs.Connect();
	fs.SetSessionPath(gPathThread0);

	FOREVER
		{
		gFileCreateFail++;
		gThreadTick=ETrue;
		RFile f;
		TInt r=f.Replace(fs,_L("TwiddleThumbs"),EFileRead|EFileWrite);
		if (r!=KErrNone)
			continue;
		gFileCreateFail=0;
		f.Close();
		gThreadTick=ETrue;
		fs.Delete(_L("TwiddleThumbs"));
		gThreadTick=ETrue;
		}
	
	//return(KErrNone);
	}

LOCAL_C void Test2()
//
// Create a hung server then kill the thread it wants to write to.
//
	{

	test.Next(_L("Create a hung server and kill the thread it is writing to"));
	TInt r=TheFs.MkDir(gPathThread0);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	StartThread4(); // Generate media changes

	RThread clientThread;
	r=clientThread.Create(_L("TALERT_ThreadHangTest"),ThreadHangTest,0x4000,KHeapSize,KHeapSize,NULL,EOwnerThread);
	test_KErrNone(r);
	TRequestStatus status;
	clientThread.Logon(status);
	clientThread.Resume();
	gThreadTick=ETrue;
	test.Next(_L("ThreadHangTest is running"));

	TInt count=0;
	FOREVER
		{
		test.Printf(_L("Thread tick = %d File create failures = %d \n"),count,gFileCreateFail);
		test.Printf(_L("Media changes = %d\n"),gMediaChanges);
		User::After(gHeartBeatInterval);
		if (gThreadTick==0)
			break;
		gThreadTick=0;
		count++;
		}

	test.Next(_L("Thread is hung"));
	clientThread.Kill(KErrCancel);
	clientThread.Close();
	User::WaitForRequest(status);
	KillThreads();
	User::After(1000000);
	
	test.Printf(_L("Press return to the clear notifier, then any key to continue test"));
	test.Getch();

	test.Next(_L("Test fileserver is still alive after thread is killed"));
	WaitForMedia();
	r=TheFs.MkDir(gPathThread0); // Check fileserver ok
	if(r!=KErrNone && r!=KErrAlreadyExists)
		test.Printf(_L("r=%d"),r);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	
	}		

GLDEF_C void CallTestsL()
//
// Do tests relative to the session path
//
	{
	
	RThread().SetPriority(EPriorityLess);
#if defined(__WINS__)
	if (gSessionPath[0]!='X')
		return;
#else
	// Test on two drives where possible. This should be C: together with 
	// the drive referenced by the default path. Where the default path is
	// C:, then only C: is used. The standard way to run the test is to 
	// set the default path to D:

    gPathThread0[0]=gSessionPath[0];
    gPathThread2[0]=gSessionPath[0];
#endif

	Test2();
	Test1();
//This test can leave the drive corrupt so a format is required 
#if defined(__WINS__)
	Format(EDriveX);
#else
	Format(EDriveD);
#endif
	//clean up the talert directory after the test completes
/*	CFileMan* FileMan=NULL;
	FileMan=CFileMan::NewL(TheFs);
	TInt r=FileMan->RmDir(_L("\\F32-TST\\TALERT\\"));
	test.Printf(_L("r=%d"),r);
	test_Value(r, r == KErrNone || r==KErrPathNotFound);
	delete FileMan;
*/
	}
