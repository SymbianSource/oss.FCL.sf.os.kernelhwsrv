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
// e32test\secure\t_sthread.cpp
// Overview:
// Test the platform security aspects of the RThread class
// API Information:
// RThread
// Details:
// - Test renaming the current thread and renaming a thread from 
// another thread. Verify results are as expected.
// - Test resuming a thread from a different process and from the
// process that created the thread. Verify results are as expected.
// - Verify that other processes can not suspend a thread and that the
// creating process can.
// - Perform a variety of tests on killing, terminating and panicking
// a thread. Verify results are as expected.
// - Test setting thread priority in a variety of ways, verify results
// are as expected. Includes ensuring real-time priorities are
// unavailable to processes without capability ProtServ.
// - Test RequestComplete and RequestSignal on a thread in a variety 
// of ways, verify results are as expected.
// - Test SetProcessPriority on a thread in a variety of ways, verify 
// results are as expected.
// - Test the Heap, ExceptionHandler, SetExceptionHandler, 
// ModifyExceptionMask, RaiseException, IsExceptionHandled, 
// SetProtected and SetSystem methods. Verify results as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("T_STHREAD"));

_LIT(KSyncMutex,"T_STHREAD-sync-mutex");
RMutex SyncMutex;

void Wait()
	{
	RMutex syncMutex;
	if(syncMutex.OpenGlobal(KSyncMutex)!=KErrNone)
		User::Invariant();
	syncMutex.Wait();
	syncMutex.Signal();
	syncMutex.Close();
	}

enum TTestProcessFunctions
	{
	ETestProcessResume,
	ETestProcessSuspend,
	ETestProcessKill,
	ETestProcessTerminate,
	ETestProcessPanic,
	ETestProcessRequestComplete,
	ETestProcessRequestSignal,
	ETestProcessPriorityControlOff,
	ETestProcessPriorityControlOn,
	ETestProcessSetPriority,
	ETestProcessSetPrioritiesWithoutProtServ,
	ETestProcessSetPrioritiesWithProtServ
	};

#include "testprocess.h"

_LIT(KTestPanicCategory,"TEST PANIC");
_LIT(KTestThreadName,"TestName");


class RTestThread : public RThread
	{
public:
	void Create(TThreadFunction aFunction,TAny* aArg=0);
	};

volatile TInt TestThreadCount = 0;

TInt TestThreadNull(TAny*)
	{
	++TestThreadCount;
	Wait();
	return KErrNone;
	}

void RTestThread::Create(TThreadFunction aFunction,TAny* aArg)
	{
	TInt r=RThread::Create(_L("TestThread"),aFunction,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,aArg);
	test(r==KErrNone);
	}


// these priorities are available to any process
void TestSetNormalApplicationPriorities(RThread& aThread)
	{
	TThreadPriority priority = aThread.Priority(); // save priority to restore before return
	aThread.SetPriority(EPriorityAbsoluteVeryLow);
	test(aThread.Priority()==EPriorityAbsoluteVeryLow);
	aThread.SetPriority(EPriorityAbsoluteLowNormal);
	test(aThread.Priority()==EPriorityAbsoluteLowNormal);
	aThread.SetPriority(EPriorityAbsoluteLow);
	test(aThread.Priority()==EPriorityAbsoluteLow);
	aThread.SetPriority(EPriorityAbsoluteBackgroundNormal);
	test(aThread.Priority()==EPriorityAbsoluteBackgroundNormal);
	aThread.SetPriority(EPriorityAbsoluteBackground);
	test(aThread.Priority()==EPriorityAbsoluteBackground);
	aThread.SetPriority(EPriorityAbsoluteForegroundNormal);
	test(aThread.Priority()==EPriorityAbsoluteForegroundNormal);
	aThread.SetPriority(EPriorityAbsoluteForeground);
	test(aThread.Priority()==EPriorityAbsoluteForeground);
	aThread.SetPriority(EPriorityAbsoluteHighNormal);
	test(aThread.Priority()==EPriorityAbsoluteHighNormal);
	aThread.SetPriority(EPriorityAbsoluteHigh);
	test(aThread.Priority()==EPriorityAbsoluteHigh);
	aThread.SetPriority(priority);
	}

TInt TestThreadSetPriority(TAny* aArg)
	{
	RThread thisThread;
	thisThread.SetPriority((TThreadPriority)(reinterpret_cast<TInt>(aArg)));
	return KErrNone;
	}

void TestSetPriorityPanic(TThreadPriority aPriority)
	{
	RTestThread thread;
	TRequestStatus status;
	thread.Create(TestThreadSetPriority, reinterpret_cast<TAny*>(aPriority));
	thread.Logon(status);
	thread.Resume();
	User::WaitForRequest(status);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitCategory()==_L("KERN-EXEC"));
	test(thread.ExitReason()==46);
	CLOSE_AND_WAIT(thread);
	}

void TestSetPrioritySuccess(TThreadPriority aPriority)
	{
	RTestThread thread;
	TRequestStatus status;
	thread.Create(TestThreadSetPriority, reinterpret_cast<TAny*>(aPriority));
	thread.Logon(status);
	thread.Resume();
	User::WaitForRequest(status);
	test(thread.Priority()==aPriority);
	test(thread.ExitCategory()==_L("Kill"));
	test(thread.ExitReason()==0);
	CLOSE_AND_WAIT(thread);
	}

TInt DoTestProcess(TInt aTestNum,TInt aArg1,TInt aArg2)
	{
	RThread thread;
	TInt r;

	switch(aTestNum)
		{

	case ETestProcessResume:
		{
		r = thread.Open(aArg1);
		if(r==KErrNone)
			thread.Resume(); // Should panic us
		return r;
		}

	case ETestProcessSuspend:
		{
		r = thread.Open(aArg1);
		if(r==KErrNone)
			thread.Suspend(); // Should panic us
		return r;
		}

	case ETestProcessKill:
		{
		r = thread.Open(aArg1);
		if(r==KErrNone)
			thread.Kill(999); // Should panic us
		return r;
		}

	case ETestProcessTerminate:
		{
		r = thread.Open(aArg1);
		if(r==KErrNone)
			thread.Terminate(999); // Should panic us
		return r;
		}

	case ETestProcessPanic:
		{
		r = thread.Open(aArg1);
		if(r==KErrNone)
			thread.Panic(KTestPanicCategory,999); // Should panic us
		return r;
		}

	case ETestProcessSetPriority:
		{
		r = thread.Open(aArg1);
		if(r==KErrNone)
			thread.SetPriority((TThreadPriority)aArg2);
		return r;
		}

	case ETestProcessRequestComplete:
		{
		r = thread.Open(aArg1);
		if(r==KErrNone)
			{
			// use a local request status because Thread::RequestComplete is
			// implemented to write to it in our context
			TRequestStatus myStatus;
			TRequestStatus* status = &myStatus;
			thread.RequestComplete(status,KErrNone);
			}
		return r;
		}

	case ETestProcessRequestSignal:
		{
		r = thread.Open(aArg1);
		if(r==KErrNone)
			thread.RequestSignal();
		return r;
		}

	case ETestProcessPriorityControlOn:
		User::SetPriorityControl(ETrue);
		// fall through...
	case ETestProcessPriorityControlOff:
		RProcess::Rendezvous(RThread().Id());
		Wait();
		return KErrNone;

	case ETestProcessSetPrioritiesWithoutProtServ:
		{
		RThread thread;
		TestSetNormalApplicationPriorities(thread);
		TestSetPriorityPanic(EPriorityAbsoluteRealTime1);
		TestSetPriorityPanic(EPriorityAbsoluteRealTime2);
		TestSetPriorityPanic(EPriorityAbsoluteRealTime3);
		TestSetPriorityPanic(EPriorityAbsoluteRealTime4);
		TestSetPriorityPanic(EPriorityAbsoluteRealTime5);
		TestSetPriorityPanic(EPriorityAbsoluteRealTime6);
		TestSetPriorityPanic(EPriorityAbsoluteRealTime7);
		TestSetPriorityPanic(EPriorityAbsoluteRealTime8);	
		return KErrNone;
		}
		
	case ETestProcessSetPrioritiesWithProtServ:
		{
		RThread thread;
		TestSetNormalApplicationPriorities(thread);
		TestSetPrioritySuccess(EPriorityAbsoluteRealTime1);
		TestSetPrioritySuccess(EPriorityAbsoluteRealTime2);
		TestSetPrioritySuccess(EPriorityAbsoluteRealTime3);
		TestSetPrioritySuccess(EPriorityAbsoluteRealTime4);
		TestSetPrioritySuccess(EPriorityAbsoluteRealTime5);
		TestSetPrioritySuccess(EPriorityAbsoluteRealTime6);
		TestSetPrioritySuccess(EPriorityAbsoluteRealTime7);
		TestSetPrioritySuccess(EPriorityAbsoluteRealTime8);	
		return KErrNone;
		}

	default:
		User::Panic(_L("T_STHREAD"),1);
		}

	return KErrNone;
	}



void TestThreadForPlatformSecurityTrap(TThreadFunction aFunction)
	{
	TBool jit = User::JustInTime();
	TRequestStatus logonStatus;
	RTestThread thread;
	thread.Create(aFunction,(TAny*)(TUint)RThread().Id());
	thread.Logon(logonStatus);
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(logonStatus);
	User::SetJustInTime(jit);
	test(thread.ExitType()==EExitPanic);
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(thread);
	}


void TestProcessForPlatformSecurityTrap(TTestProcessFunctions aFunction)
	{
	TRequestStatus logonStatus2;
	RTestProcess process;
	process.Create(~0u,aFunction,RThread().Id(),EPriorityAbsoluteLow);
	process.Logon(logonStatus2);
	process.Resume();
	User::WaitForRequest(logonStatus2);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus2==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);
	}


void TestRename()
	{
	TName name;

	test.Start(_L("Renaming the current thread"));
	name = RThread().Name();
	name.SetLength(KTestThreadName().Length());
	test(name.CompareF(KTestThreadName)!=0);
	User::RenameThread(KTestThreadName);
	name = RThread().Name();
	name.SetLength(KTestThreadName().Length());
	test(name.CompareF(KTestThreadName)==0);


	test.End();
	}



void TestResume()
	{
	RTestProcess process;
	RTestThread thread;
	TRequestStatus logonStatus;
	TInt testCount = TestThreadCount;

	test.Start(_L("Try to get another process to resume one we've created"));
	thread.Create(TestThreadNull);
	process.Create(~0u,ETestProcessResume,thread.Id());
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	User::After(1000000); // Give time for thread to run (if it had been resumed)...
	test(TestThreadCount==testCount); // it shouldn't have, so count will be unchanged.

	test.Next(_L("Test resuming a thread we've created"));
	thread.Logon(logonStatus);
	test(logonStatus==KRequestPending);
	thread.Resume();
	User::WaitForRequest(logonStatus);
	test(logonStatus==KErrNone);
	test(TestThreadCount==testCount+1); // Thread should have run and incremented the count
	CLOSE_AND_WAIT(thread);
	CLOSE_AND_WAIT(process);

	test.End();
	}



TInt TestThreadCounting(TAny*)
	{
	RThread().SetPriority(EPriorityAbsoluteVeryLow);
	for(;;)
		++TestThreadCount;
	}

TBool IsTestThreadRunning()
	{
	 // Thread should have been busy incrementing the count if it is running
	TInt testCount = TestThreadCount;
	User::After(100000);
	if(testCount!=TestThreadCount)
		return ETrue;
	User::After(1000000);
	return testCount!=TestThreadCount;
	}

void TestSuspend()
	{
	RTestProcess process;
	RTestThread thread;
	TRequestStatus logonStatus;

	test.Start(_L("Creating a never ending thread..."));
	thread.Create(TestThreadCounting);
	thread.Resume();
	test(IsTestThreadRunning());  // Thread should still be running

	test.Next(_L("Checking other process can't supspend it"));
	process.Create(~0u,ETestProcessSuspend,thread.Id());
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	test(IsTestThreadRunning());  // Thread should still be running

	test.Next(_L("Test suspending a thread in same process"));
	thread.Logon(logonStatus);
	thread.Suspend();
	test(!IsTestThreadRunning()); // Thread should have stopped...
	test(logonStatus==KRequestPending); // but not have died
	thread.LogonCancel(logonStatus);
	User::WaitForRequest(logonStatus);
	
	test.Next(_L("Kill thread"));
	thread.Kill(0);
	CLOSE_AND_WAIT(thread);
	CLOSE_AND_WAIT(process);

	test.End();
	}



TInt TestThreadKillSelf(TAny* )
	{
	RThread().Kill(999);
	return KErrGeneral;
	}

TInt TestThreadTerminateSelf(TAny*)
	{
	RThread().Terminate(999);
	return KErrGeneral;
	}

TInt TestThreadPanicSelf(TAny*)
	{
	RThread().Panic(KTestPanicCategory,999);
	return KErrGeneral;
	}

void TestKill()
	{
	RTestProcess process;
	RTestThread thread;
	TRequestStatus logonStatus;
	TRequestStatus logonStatus2;
	TBool jit = User::JustInTime();

	// Test RProcess::Kill()

	test.Start(_L("Test killing an un-resumed thread created by us"));
	thread.Create(TestThreadNull);
	thread.Logon(logonStatus);
	thread.Kill(999);
	User::WaitForRequest(logonStatus);
	test(thread.ExitType()==EExitKill);
	test(logonStatus==999);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Test killing a resumed thread created by us"));
	thread.Create(TestThreadNull);
	thread.Logon(logonStatus);
	SyncMutex.Wait();
	thread.Resume();
	thread.Kill(999);
	SyncMutex.Signal();
	User::WaitForRequest(logonStatus);
	test(thread.ExitType()==EExitKill);
	test(logonStatus==999);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Try killing un-resumed thread not created by self"));
	thread.Create(TestThreadNull);
	process.Create(~0u,ETestProcessKill,thread.Id());
	thread.Logon(logonStatus2);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	test(logonStatus2==KRequestPending); // the thread should still be alive
	thread.Resume();
	User::WaitForRequest(logonStatus2);
	test(logonStatus2==KErrNone);
	CLOSE_AND_WAIT(thread);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try killing resumed thread not created by self"));
	thread.Create(TestThreadNull);
	process.Create(~0u,ETestProcessKill,thread.Id());
	thread.Logon(logonStatus2);
	process.Logon(logonStatus);
	SyncMutex.Wait();
	thread.Resume();
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	test(logonStatus2==KRequestPending); // the thread should still be alive
	SyncMutex.Signal();
	User::WaitForRequest(logonStatus2);
	test(logonStatus2==KErrNone);
	CLOSE_AND_WAIT(thread);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test a thread killing itself"));
	thread.Create(TestThreadKillSelf);
	thread.Logon(logonStatus);
	thread.Resume();
	User::WaitForRequest(logonStatus);
	test(thread.ExitType()==EExitKill);
	test(logonStatus==999);
	CLOSE_AND_WAIT(thread);

	// Test RProcess::Teminate()

	test.Next(_L("Test terminating an un-resumed thread created by us"));
	thread.Create(TestThreadNull);
	thread.Logon(logonStatus);
	thread.Terminate(999);
	User::WaitForRequest(logonStatus);
	test(thread.ExitType()==EExitTerminate);
	test(logonStatus==999);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Test terminating a resumed thread created by us"));
	thread.Create(TestThreadNull);
	thread.Logon(logonStatus);
	SyncMutex.Wait();
	thread.Resume();
	thread.Terminate(999);
	SyncMutex.Signal();
	User::WaitForRequest(logonStatus);
	test(thread.ExitType()==EExitTerminate);
	test(logonStatus==999);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Try terminating un-resumed thread not created by self"));
	thread.Create(TestThreadNull);
	process.Create(~0u,ETestProcessTerminate,thread.Id());
	thread.Logon(logonStatus2);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	test(logonStatus2==KRequestPending); // the thread should still be alive
	thread.Resume();
	User::WaitForRequest(logonStatus2);
	test(logonStatus2==KErrNone);
	CLOSE_AND_WAIT(thread);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try terminating resumed thread not created by self"));
	thread.Create(TestThreadNull);
	process.Create(~0u,ETestProcessTerminate,thread.Id());
	thread.Logon(logonStatus2);
	process.Logon(logonStatus);
	SyncMutex.Wait();
	thread.Resume();
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	test(logonStatus2==KRequestPending); // the thread should still be alive
	SyncMutex.Signal();
	User::WaitForRequest(logonStatus2);
	test(logonStatus2==KErrNone);
	CLOSE_AND_WAIT(thread);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test a thread terminating itself"));
	thread.Create(TestThreadTerminateSelf);
	thread.Logon(logonStatus);
	thread.Resume();
	User::WaitForRequest(logonStatus);
	test(thread.ExitType()==EExitTerminate);
	test(logonStatus==999);
	CLOSE_AND_WAIT(thread);

	// Test RProcess::Panic()

	test.Next(_L("Test panicking an un-resumed thread created by us"));
	thread.Create(TestThreadNull);
	thread.Logon(logonStatus);
	User::SetJustInTime(EFalse);
	thread.Panic(KTestPanicCategory,999);
	User::WaitForRequest(logonStatus);
	User::SetJustInTime(jit);
	test(thread.ExitType()==EExitPanic);
	test(logonStatus==999);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Test panicking a resumed thread created by us"));
	thread.Create(TestThreadNull);
	thread.Logon(logonStatus);
	SyncMutex.Wait();
	thread.Resume();
	User::SetJustInTime(EFalse);
	thread.Panic(KTestPanicCategory,999);
	SyncMutex.Signal();
	User::WaitForRequest(logonStatus);
	User::SetJustInTime(jit);
	test(thread.ExitType()==EExitPanic);
	test(logonStatus==999);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Try panicking un-resumed thread not created by self"));
	thread.Create(TestThreadNull);
	process.Create(~0u,ETestProcessPanic,thread.Id());
	thread.Logon(logonStatus2);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	test(logonStatus2==KRequestPending); // the thread should still be alive
	thread.Resume();
	User::WaitForRequest(logonStatus2);
	test(logonStatus2==KErrNone);
	CLOSE_AND_WAIT(thread);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try panicking resumed thread not created by self"));
	thread.Create(TestThreadNull);
	process.Create(~0u,ETestProcessPanic,thread.Id());
	thread.Logon(logonStatus2);
	process.Logon(logonStatus);
	SyncMutex.Wait();
	thread.Resume();
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	test(logonStatus2==KRequestPending); // the thread should still be alive
	SyncMutex.Signal();
	User::WaitForRequest(logonStatus2);
	test(logonStatus2==KErrNone);
	CLOSE_AND_WAIT(thread);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test a thread panicking itself"));
	thread.Create(TestThreadPanicSelf);
	thread.Logon(logonStatus);
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(logonStatus);
	User::SetJustInTime(jit);
	test(thread.ExitType()==EExitPanic);
	test(logonStatus==999);
	CLOSE_AND_WAIT(thread);

	// 

	test.End();
	}


//---------------------------------------------
//! @SYMTestCaseID KBASE-T_STHREAD-0120
//! @SYMTestCaseDesc Set thread priority
//! @SYMTestType UT
//! @SYMREQ historical, enhanced under PREQ955
//! @SYMTestActions Test setting all thread priority values to threads in this process,
//!     and in another process, resumed and not.
//! @SYMTestExpectedResults Confirm can set and get "normal application" thread priorities
//!     for threads in this process, whether resumed or not. Confirm thread is panicked
//!     if attempts to set priority of thread in another process. Confirm can set and get
//!     "real-time" thread priorities if this process has ProtServ capability, and that
//!     calling thread is panicked if not.
//! @SYMTestPriority Critical
//! @SYMTestStatus Implemented
//---------------------------------------------
void TestSetPriority()
	{
	RTestThread thread;
	RTestProcess process;
	TRequestStatus logonStatus;
	TRequestStatus logonStatus2;

	test.Start(_L("Test changing our own threads priority"));
	TestSetNormalApplicationPriorities(thread);

	test.Next(_L("Test changing priority of un-resumed thread in our process"));
	thread.Create(TestThreadNull);
	thread.Logon(logonStatus);
	TestSetNormalApplicationPriorities(thread);

	test.Next(_L("Test changing priority of resumed thread in our process"));
	SyncMutex.Wait();
	thread.Resume();
	TestSetNormalApplicationPriorities(thread);
	SyncMutex.Signal();
	User::WaitForRequest(logonStatus);
	test(logonStatus==KErrNone);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Try changing priority of an un-resumed thread in other process"));
	thread.Create(TestThreadNull);
	thread.Logon(logonStatus);
	thread.SetPriority(EPriorityAbsoluteHigh);
	process.Create(~0u,ETestProcessSetPriority,thread.Id(),EPriorityAbsoluteLow);
	process.Logon(logonStatus2);
	process.Resume();
	User::WaitForRequest(logonStatus2);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus2==EPlatformSecurityTrap);
	test(thread.Priority()==EPriorityAbsoluteHigh); // Priority should be unaltered

	test.Next(_L("Try changing priority of a resumed thread in other process"));
	process.Create(~0u,ETestProcessSetPriority,thread.Id(),EPriorityAbsoluteLow);
	process.Logon(logonStatus2);
	SyncMutex.Wait();
	thread.Resume();
	process.Resume();
	User::WaitForRequest(logonStatus2);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus2==EPlatformSecurityTrap);
	test(thread.Priority()==EPriorityAbsoluteHigh); // Priority should be unaltered
	SyncMutex.Signal();
	User::WaitForRequest(logonStatus);
	test(logonStatus==KErrNone);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Test setting thread priorities without ECapabilityProtServ"));
	process.Create(~(1u<<ECapabilityProtServ), ETestProcessSetPrioritiesWithoutProtServ);
	process.Run();
	
	test.Next(_L("Test setting thread priorities with ECapabilityProtServ"));
	process.Create(1<<ECapabilityProtServ, ETestProcessSetPrioritiesWithProtServ);
	process.Run();

	test.End();
	}


TRequestStatus TestRequest;

TInt TestThreadRequestComplete(TAny* aArg)
	{
	RThread thread;
	TInt r = thread.Open((TInt)aArg);
	if(r==KErrNone)
		{
		TRequestStatus* status = &TestRequest;
		thread.RequestComplete(status,KErrNone);
		}
	return r;
	}

void TestRequestComplete()
	{
	RTestThread thread;
	RTestProcess process;
	TRequestStatus logonStatus;

	test.Start(_L("Test RequestComplete on thread in current process"));
	TestRequest = KRequestPending;
	thread.Create(TestThreadRequestComplete,(TAny*)(TUint)RThread().Id());
	thread.Logon(logonStatus);
	thread.Resume();
	User::WaitForRequest(TestRequest);
	test(TestRequest==KErrNone);
	User::WaitForRequest(logonStatus);
	test(logonStatus==KErrNone);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Test RequestComplete on with NULL request pointer"));
	test(RThread().RequestCount()==0); // No signals
	TRequestStatus* nullReq = 0;
	RThread().RequestComplete(nullReq,0);
	test(RThread().RequestCount()==0); // No signals

	test.Next(_L("Test RequestComplete on thread in different process"));
	TestRequest = KRequestPending;
	process.Create(~0u,ETestProcessRequestComplete,RThread().Id(),(TInt)&TestRequest);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	test(TestRequest==KRequestPending);
	CLOSE_AND_WAIT(process);

	test.End();
	}



TInt TestThreadRequestSignal(TAny* aArg)
	{
	RThread thread;
	TInt r = thread.Open((TInt)aArg);
	if(r==KErrNone)
		thread.RequestSignal();
	return r;
	}

void TestRequestSignal()
	{
	RTestThread thread;
	RTestProcess process;
	TRequestStatus logonStatus;
	TInt count;

	test.Start(_L("Test RequestSignal on thread in current process"));
	thread.Create(TestThreadRequestSignal,(TAny*)(TUint)RThread().Id());
	thread.Logon(logonStatus);
	count = RThread().RequestCount();
	test(count==0); // No signals yet
	thread.Resume();
	User::WaitForRequest(logonStatus);
	test(logonStatus==KErrNone);
	count = RThread().RequestCount();
	test(count==1); // We should have been signalled
	User::WaitForAnyRequest();	// eat signal
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Test RequestSignal on thread in different process"));
	process.Create(~0u,ETestProcessRequestSignal,RThread().Id(),0);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	count = RThread().RequestCount();
	test(count==0); // We shouldn't have been signalled
	CLOSE_AND_WAIT(process);

	test.End();
	}



void TestSetProcessPriority()
	{
	RTestThread thread;
	RTestProcess process;
	TProcessPriority priority;
	TRequestStatus rendezvousStatus;
	TRequestStatus logonStatus;
	TInt r;

	test.Start(_L("Test changing our own process priority"));
	priority = process.Priority();
	thread.SetProcessPriority(EPriorityLow);
	test(process.Priority()==EPriorityLow);
	thread.SetProcessPriority(EPriorityBackground);
	test(process.Priority()==EPriorityBackground);
	thread.SetProcessPriority(EPriorityForeground);
	test(process.Priority()==EPriorityForeground);
	thread.SetProcessPriority(priority);

	test.Next(_L("Try changing other process's priority (no priority-control enabled)"));
	process.Create(~0u,ETestProcessPriorityControlOff);
	process.Rendezvous(rendezvousStatus);
	process.Logon(logonStatus);
	SyncMutex.Wait();
	process.Resume();
	User::WaitForRequest(rendezvousStatus); // Process has started
	r = thread.Open(rendezvousStatus.Int()); // Process returned Id of main thread as status value
	test(r==KErrNone);
	priority = process.Priority();
	thread.SetProcessPriority(EPriorityLow);
	test(process.Priority()==priority); // priority shouldn't have changed
	thread.SetProcessPriority(EPriorityBackground);
	test(process.Priority()==priority); // priority shouldn't have changed
	thread.SetProcessPriority(EPriorityForeground);
	test(process.Priority()==priority); // priority shouldn't have changed
	test(logonStatus==KRequestPending); // wait for process to end
	SyncMutex.Signal();
	User::WaitForRequest(logonStatus);
	CLOSE_AND_WAIT(thread);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try changing other process's priority (priority-control enabled)"));
	process.Create(~0u,ETestProcessPriorityControlOn);
	process.Rendezvous(rendezvousStatus);
	process.Logon(logonStatus);
	SyncMutex.Wait();
	process.Resume();
	User::WaitForRequest(rendezvousStatus); // Process has started
	r = thread.Open(rendezvousStatus.Int()); // Process returned Id of main thread as status value
	test(r==KErrNone);
	priority = process.Priority();
	thread.SetProcessPriority(EPriorityForeground);
	test(process.Priority()==EPriorityForeground);
	thread.SetProcessPriority(EPriorityBackground);
	test(process.Priority()==EPriorityBackground);
	thread.SetProcessPriority(EPriorityForeground);
	test(process.Priority()==EPriorityForeground);
	thread.SetProcessPriority(EPriorityLow);
	test(process.Priority()==EPriorityForeground); // should still be foreground priority
	thread.SetProcessPriority(priority);
	test(logonStatus==KRequestPending); // wait for process to end
	SyncMutex.Signal();
	User::WaitForRequest(logonStatus);
	CLOSE_AND_WAIT(thread);
	CLOSE_AND_WAIT(process);

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

	if((!PlatSec::ConfigSetting(PlatSec::EPlatSecProcessIsolation))||(!PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement)))
		{
		test.Start(_L("TESTS NOT RUN - PlatSecProcessIsolation is not enforced"));
		test.End();
		return 0;
		}

	test(SyncMutex.CreateGlobal(KSyncMutex)==KErrNone);
	
	test.Start(_L("Test Rename"));
	TestRename();

	test.Next(_L("Test Resume"));
	TestResume();

	test.Next(_L("Test Suspend"));
	TestSuspend();

	test.Next(_L("Test Kill, Panic and Teminate"));
	TestKill();

	test.Next(_L("Test SetPriority"));
	TestSetPriority();

	test.Next(_L("Test RequestComplete"));
	TestRequestComplete();

	test.Next(_L("Test RequestSignal"));
	TestRequestSignal();

	test.Next(_L("Test SetProcessPriority"));
	TestSetProcessPriority();


	SyncMutex.Close();
	test.End();

	return(0);
    }

