// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\secure\t_rendezvous.cpp
// Overview:
// Test RThread and RProcess rendezvous
// API Information:
// RThread, RProcess
// Details:
// - Create a thread, request rendezvous, cancel rendezvous, request rendezvous
// again, resume thread and wait for rendezvous, verify results as expected.
// - Create a thread, request rendezvous, allow thread to die, verify results.
// - Create a thread, request rendezvous, allow thread to finish, verify results.
// - Create a process, request rendezvous, cancel rendezvous, request rendezvous
// again, resume process and wait for rendezvous, verify results as expected.
// - Create a process, request rendezvous, allow process to die, verify results.
// - Create a process, request rendezvous, allow process to finish, verify results.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("T_RENDEZVOUS"));

_LIT(KSyncMutext,"T_RENDEZVOUS-sync-mutex");
RMutex SyncMutex;

void Wait()
	{
	RMutex syncMutex;
	if(syncMutex.OpenGlobal(KSyncMutext)!=KErrNone)
		User::Invariant();
	syncMutex.Wait();
	syncMutex.Signal();
	syncMutex.Close();
	}

const TInt KRendezvousOk = 1;

TInt ThreadRendezvous(TAny *)
	{
	RThread::Rendezvous(KRendezvousOk);
	Wait();
	return KErrNone;
	}

TInt ThreadDie(TAny *)
	{
	User::Panic(_L("ThreadDie"),KErrDied);
	return KErrNone;
	}

TInt ThreadFinish(TAny *)
	{
	return KErrNone;
	}

void TestThreadRendezvous()
	{
	TRequestStatus rendezvousStatus;
	TRequestStatus rendezvousStatusCancel;
	TRequestStatus logonStatus;
	TInt r;
	RThread thread;

	//

	test.Start(_L("Create thread 'ThreadRendezvous'"));
	r=thread.Create(_L("ThreadRendezvous"),ThreadRendezvous,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,NULL);
	test(r==KErrNone);

	test.Next(_L("Request rendezvous"));
	thread.Rendezvous(rendezvousStatusCancel);
	test(rendezvousStatusCancel==KRequestPending);

	test.Next(_L("Cancel rendezvous request"));
	r = thread.RendezvousCancel(rendezvousStatusCancel);
	test(r==KErrNone);
	test(rendezvousStatusCancel==KErrNone);

	test.Next(_L("Request rendezvous again"));
	thread.Rendezvous(rendezvousStatus);
	test(rendezvousStatus==KRequestPending);

	test.Next(_L("Also logon to thread"));
	thread.Logon(logonStatus);
	test(logonStatus==KRequestPending);

	test.Next(_L("Resume thread and wait for rendezvous..."));
	SyncMutex.Wait();
	thread.Resume();
	User::WaitForRequest(rendezvousStatus,logonStatus);
	test(rendezvousStatus==KRendezvousOk);
	test(logonStatus==KRequestPending);
	test(rendezvousStatusCancel==KErrNone);

	test.Next(_L("Rendezvous OK, now wait for thread to end..."));
	SyncMutex.Signal();
	User::WaitForRequest(logonStatus);
	test(logonStatus==KErrNone);
	test(rendezvousStatusCancel==KErrNone);

	CLOSE_AND_WAIT(thread);

	//

	test.Next(_L("Create thread 'ThreadDie'"));
	r=thread.Create(_L("ThreadDie"),ThreadDie,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,NULL);
	test(r==KErrNone);

	test.Next(_L("Request rendezvous"));
	thread.Rendezvous(rendezvousStatus);
	test(rendezvousStatus==KRequestPending);

	test.Next(_L("Resume thread and wait for thread to die..."));
	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(rendezvousStatus);
	User::SetJustInTime(jit);
	test(rendezvousStatus==KErrDied);
	test(thread.ExitReason()==KErrDied);
	test(thread.ExitType()==EExitPanic);

	CLOSE_AND_WAIT(thread);

	//

	test.Next(_L("Create thread 'ThreadFinish'"));
	r=thread.Create(_L("ThreadFinish"),ThreadFinish,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,NULL);
	test(r==KErrNone);

	test.Next(_L("Request rendezvous"));
	thread.Rendezvous(rendezvousStatus);
	test(rendezvousStatus==KRequestPending);

	test.Next(_L("Resume thread and wait for thread to finish..."));
	thread.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==KErrNone);
	test(thread.ExitReason()==KErrNone);
	test(thread.ExitType()==EExitKill);

	CLOSE_AND_WAIT(thread);

	//

	test.End();
	}


enum TTestProcessFunctions
	{
	ETestProcessRendezvous,
	ETestProcessDie,
	ETestProcessFinish,
	};

#include "testprocess.h"

TInt DoTestProcess(TInt aTestNum,TInt aArg1,TInt aArg2)
	{
	(void)aArg1; // Keep compiler quiet about unused parameters
	(void)aArg2;

	switch(aTestNum)
		{
	case ETestProcessRendezvous:
		RProcess::Rendezvous(KRendezvousOk);
		Wait();
		return KErrNone;

	case ETestProcessDie:
		User::Panic(_L("ProcessDie"),KErrDied);
		return KErrNone;

	case ETestProcessFinish:
		return KErrNone;

	default:
		User::Panic(_L("T_RENDEZVOUS"),1);
		}
	return KErrNone;
	}

void TestProcessRendezvous()
	{
	TRequestStatus rendezvousStatus;
	TRequestStatus rendezvousStatusCancel;
	TRequestStatus logonStatus;
	TInt r;
	RTestProcess process;

	test.Start(_L("Create new process"));
	process.Create(ETestProcessRendezvous);

	test.Next(_L("Request rendezvous"));
	process.Rendezvous(rendezvousStatusCancel);
	test(rendezvousStatusCancel==KRequestPending);

	test.Next(_L("Cancel rendezvous request"));
	r = process.RendezvousCancel(rendezvousStatusCancel);
	test(r==KErrNone);
	test(rendezvousStatusCancel==KErrNone);

	test.Next(_L("Request rendezvous again"));
	process.Rendezvous(rendezvousStatus);
	test(rendezvousStatus==KRequestPending);

	test.Next(_L("Also logon to process"));
	process.Logon(logonStatus);
	test(logonStatus==KRequestPending);

	test.Next(_L("Resume process and wait for rendezvous..."));
	SyncMutex.Wait();
	process.Resume();
	User::WaitForRequest(rendezvousStatus,logonStatus);
	test(rendezvousStatus==KRendezvousOk);
	test(logonStatus==KRequestPending);
	test(rendezvousStatusCancel==KErrNone);

	test.Next(_L("Rendezvous OK, now wait for process to end..."));
	SyncMutex.Signal();
	User::WaitForRequest(logonStatus);
	test(logonStatus==KErrNone);
	test(rendezvousStatusCancel==KErrNone);

	CLOSE_AND_WAIT(process);

	//

	test.Next(_L("Create new process"));
	process.Create(ETestProcessDie);

	test.Next(_L("Request rendezvous"));
	process.Rendezvous(rendezvousStatus);
	test(rendezvousStatus==KRequestPending);

	test.Next(_L("Resume process and wait for process to die..."));
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==KErrDied);
	test(process.ExitReason()==KErrDied);
	test(process.ExitType()==EExitPanic);

	CLOSE_AND_WAIT(process);

	//

	test.Next(_L("Create new process"));
	process.Create(ETestProcessFinish);

	test.Next(_L("Request rendezvous"));
	process.Rendezvous(rendezvousStatus);
	test(rendezvousStatus==KRequestPending);

	test.Next(_L("Resume process and wait for process to finish..."));
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==KErrNone);
	test(process.ExitReason()==KErrNone);
	test(process.ExitType()==EExitKill);

	CLOSE_AND_WAIT(process);

	// 

	test.End();
	}



GLDEF_C TInt E32Main()
    {
	TBuf16<512> cmd;
	User::CommandLine(cmd);
	if(cmd.Length() && TChar(cmd[0]).IsDigit())
		{
		TInt function = -1;
		TInt arg1 = -1;
		TInt arg2 = -1;
		TLex lex(cmd);
		lex.Val(function);
		lex.SkipSpace();
		lex.Val(arg1);
		lex.SkipSpace();
		lex.Val(arg2);
		return DoTestProcess(function,arg1,arg2);
		}

	test.Title();
	test(SyncMutex.CreateGlobal(KSyncMutext)==KErrNone);

	test.Start(_L("Test Thread rendezvous"));
	TestThreadRendezvous();

	test.Next(_L("Test Process rendezvous"));
	TestProcessRendezvous();

	SyncMutex.Close();
	test.End();

	return(0);
    }

