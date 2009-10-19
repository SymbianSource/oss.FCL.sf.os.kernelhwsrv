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
// e32test\prime\t_sem.cpp
// Overview:
// Test the RSemaphore type.
// API Information:
// RSemaphore
// Details:
// - Create two local semaphores, create a new thread, signal and wait on 
// the semaphores numerous time. Kill the thread and verify results.
// - Test creating a local semaphore with an initial count of -1. Verify 
// failure results are as expected.
// - Attempt to signal a semaphore without having done a create. Verify 
// failure results are as expected.
// - Attempt to signal a semaphore with signal count of -1. Verify failure
// results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32panic.h>

const TInt KMaxIterations=0x100;
const TInt KHeapSize=0x2000;

LOCAL_D RTest test(_L("T_SEM"));
LOCAL_D RSemaphore consumer;
LOCAL_D RSemaphore producer;

LOCAL_C TInt producerThreadEntryPoint(TAny* anArg)
//
// The entry point for the producer thread.
//
	{

	anArg=anArg;
	for (TInt i=0;i<KMaxIterations;i++)
		{
		consumer.Signal();
		producer.Wait();
		}
	return(KErrNone);
	}

enum {EFuncCreateNeg,EFuncSignalWithoutCreate,EFuncSignalNeg};

TInt CreateNegativeThread(TAny* aFunc)
	{
	RSemaphore sem;
	switch((TInt)aFunc)
		{
	case EFuncCreateNeg:
		sem.CreateLocal(-1);
		break;
	case EFuncSignalWithoutCreate:
		sem.Signal();
		break;
	case EFuncSignalNeg:
		sem.CreateLocal(1);
		sem.Signal(-1);
		break;
	default:
		test.Panic(_L("Hit default label"));
		}
	return KErrNone;
	}

GLDEF_C TInt E32Main()
//
// Test the RSemaphore type.
//
    {

	test.Title();
	test.Start(_L("Create semaphores"));
	TInt r=consumer.CreateLocal(1);
	test(r==KErrNone);
	r=producer.CreateLocal(0);
	test(r==KErrNone);
	test.Next(_L("Create thread"));
	RThread producerThread;
	r=producerThread.Create(_L("Producer"),producerThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	test.Next(_L("Logon thread"));
	TRequestStatus tStat;
	producerThread.Logon(tStat);
	test(tStat==KRequestPending);
	test.Next(_L("Resume thread"));
	producerThread.Resume();
	test(ETrue);
	test.Next(_L("Run semaphores"));
	for (TInt i=0;i<KMaxIterations;i++)
		{
		test.Printf(_L("\r%03d"),i);
		consumer.Wait();
		producer.Signal();
		}
	test.Printf(_L("\n"));
	test.Next(_L("Wait for thread to die"));
	User::WaitForRequest(tStat);
	test(r==KErrNone);
	test.Next(_L("Kill thread"));
	producerThread.Kill(0);
	TName c=producerThread.ExitCategory();
	TInt reason=producerThread.ExitReason();
	switch (producerThread.ExitType())
		{
	case EExitKill:
		test.Printf(_L("KILL: %d [%S]\n"),reason,&c);
		break;
	case EExitTerminate:
		test.Printf(_L("TERMINATE: %d [%S]\n"),reason,&c);
		break;
	case EExitPanic:
		test.Printf(_L("PANIC: %d [%S]\n"),reason,&c);
		break;
	default:
		test.Panic(_L("UNKNOWN: %d [%S]\n"),reason,&c);
		break;
		}
	test.Next(_L("Cleanup"));
	producerThread.Close();
	CLOSE_AND_WAIT(producer);
	CLOSE_AND_WAIT(consumer);
	
	test.Next(_L("Create(-1)"));
	RThread thread;
	r=thread.Create(_L("SemTests"),CreateNegativeThread,0x1000,NULL,(TAny*)EFuncCreateNeg);
	test(r==KErrNone);
	TRequestStatus stat;
	thread.Logon(stat);
	TBool justInTime=User::JustInTime();
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(stat);
	test(stat==ESemCreateCountNegative);
	User::SetJustInTime(justInTime);
	test(thread.ExitReason()==ESemCreateCountNegative);
	test(thread.ExitCategory()==_L("USER"));
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
	
	test.Next(_L("Signal without create"));
	r=thread.Create(_L("SemTests"),CreateNegativeThread,0x1000,NULL,(TAny*)EFuncSignalWithoutCreate);
	test(r==KErrNone);
	thread.Logon(stat);
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(stat);
	test(stat==EBadHandle);
	User::SetJustInTime(justInTime);
	test(thread.ExitReason()==EBadHandle);
	test(thread.ExitCategory()==_L("KERN-EXEC"));
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Signal(-1)"));
	r=thread.Create(_L("SemTests"),CreateNegativeThread,0x1000,NULL,(TAny*)EFuncSignalNeg);
	test(r==KErrNone);
	thread.Logon(stat);
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(stat);
	test(stat==ESemSignalCountNegative);
	User::SetJustInTime(justInTime);
	test(thread.ExitReason()==ESemSignalCountNegative);
	test(thread.ExitCategory()==_L("USER"));
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
	test.End();
	return(KErrNone);
    }


