// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\prime\t_kern.cpp
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("T_KERN"));
LOCAL_D TFindProcess fProc;
LOCAL_D TFindThread fThread;
LOCAL_D TFindSemaphore fSem;
LOCAL_D TFindMutex fMutex;
LOCAL_D TFindChunk fChunk;
LOCAL_D TFindLogicalDevice fLdd;
LOCAL_D TFindPhysicalDevice fPdd;
LOCAL_D TFindServer fServ;
LOCAL_D TFindLibrary fLib;

LOCAL_C void testFindHandles()
//
// Test the various find handles.
//
	{

	test.Start(_L("Processes"));
	TFullName n;
	while (fProc.Next(n)==KErrNone)
		test.Printf(_L("  %- 50s %d\n"),n.PtrZ(),fProc.Handle());
	test.Next(_L("Threads"));
	while (fThread.Next(n)==KErrNone)
		test.Printf(_L("  %- 50s %d\n"),n.PtrZ(),fThread.Handle());
	test.Next(_L("Semaphores"));
	while (fSem.Next(n)==KErrNone)
		test.Printf(_L("  %- 50s %d\n"),n.PtrZ(),fSem.Handle());
	test.Next(_L("Mutexes"));
	while (fMutex.Next(n)==KErrNone)
		test.Printf(_L("  %- 50s %d\n"),n.PtrZ(),fMutex.Handle());
	test.Next(_L("Chunks"));
	while (fChunk.Next(n)==KErrNone)
		test.Printf(_L("  %- 50s %d\n"),n.PtrZ(),fChunk.Handle());
	test.Next(_L("LogicalDevice"));
	while (fLdd.Next(n)==KErrNone)
		test.Printf(_L("  %- 50s %d\n"),n.PtrZ(),fLdd.Handle());
	test.Next(_L("PhysicalDevice"));
	while (fPdd.Next(n)==KErrNone)
		test.Printf(_L("  %- 50s %d\n"),n.PtrZ(),fPdd.Handle());
	test.Next(_L("Server"));
	while (fServ.Next(n)==KErrNone)
		test.Printf(_L("  %- 50s %d\n"),n.PtrZ(),fServ.Handle());
	test.Next(_L("Library"));
	while (fLib.Next(n)==KErrNone)
		test.Printf(_L("  %- 50s %d\n"),n.PtrZ(),fLib.Handle());
	test.End();
	}

LOCAL_C void testCurrentProcessAndThread()
//
// Test the handles for the current process and thread.
//
	{

	test.Start(_L("Process"));
	RProcess p;
	TFullName pFullName = p.FullName();
	test.Printf(_L("  %S\n"),&pFullName);
	TName pName = p.Name();
	test.Printf(_L("  %S\n"),&pName);
//
	test.Next(_L("Thread"));
	RThread t;
	TFullName tFullName = t.FullName();
	test.Printf(_L("  %S\n"),&tFullName);
	TName tName = t.Name();
	test.Printf(_L("  %S\n"),&tName);
//
	test.End();
	}

LOCAL_C TInt ThreadBeepFunction(TAny*)
//
// Thread which beeps and stops before the beep completes
//
	{

	User::Beep(440,1000000);
	return KErrNone;
	}

LOCAL_C void testBeep()
//
// Test the beep function.
//
	{

	test.Start(_L("220Hz dur=10"));
	User::Beep(220,1000000);
	User::After(2000000);
	test.Next(_L("330Hz dur=10"));
	User::Beep(330,1000000);
	User::After(2000000);
	test.Next(_L("440Hz dur=10"));
	User::Beep(440,1000000);
	User::After(2000000);
	test.Next(_L("880Hz dur=10"));
	User::Beep(880,1000000);
	User::After(2000000);
	test.Next(_L("1760Hz dur=10"));
	User::Beep(1760,1000000);
	User::After(2000000);

	test.Next(_L("Listen to check that the first beep is interrupted after 2 secs"));
	User::Beep(330,10000000);
	User::After(2000000);
	User::Beep(660,1000000);
	User::After(1000000);

	test.Next(_L("Listen to check that a negative value does not interrupt"));
	User::Beep(330,5000000);
	User::After(2000000);
	TInt r=User::Beep(660,-1000000);
	test.Next(_L("Check the 2nd beep returned KErrGeneral"));
	test(r==KErrGeneral);
	User::After(3000000);

	test.Next(_L("Beep survives when thread terminates"));
	RThread thread;
	thread.Create(_L("Beep Test Thread"),ThreadBeepFunction,0x1000,0x1000,0x1000,NULL);
	TRequestStatus stat;
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	RTimer timer;
	timer.CreateLocal();
	thread.Close();
	timer.After(stat,TTimeIntervalMicroSeconds32(2000000)); //2secs
	User::WaitForRequest(stat);
	CLOSE_AND_WAIT(thread);
	test.End();
	}

GLDEF_C TInt E32Main()
//
// Test the various kernel types.
//
    {

	test.Title();
//
	test.Start(_L("TFindHandles"));
	testFindHandles();
//
	test.Next(_L("Current process and thread"));
	testCurrentProcessAndThread();
//
	test.Next(_L("Test beeper"));
	testBeep();
//
	test.End();
	return(KErrNone);
    }



