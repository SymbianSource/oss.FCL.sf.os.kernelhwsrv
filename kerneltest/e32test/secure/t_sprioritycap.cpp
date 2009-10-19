// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\secure\t_sprioritycap.cpp
// Overview:
// Test the platform security aspects of the RThread class as affected by the process priority
// specified in the MMP file, mainly capping of higher thread priorities without ProtServ.
// API Information:
// Process priorities windowserver, fileserver, supervisor and realtimeserver set with
//

// # 'epocprocesspriority' keyword in MMP files
// # 'priority' keyword in OBEY (OBY/IBY) files
// Details:
// - Tests that the desired thread prioritisation results are obtained for process priorities
// SystemServer and RealTimeServer (established by separate MMP files):
// # without ECapabilityProtServ - priorities capped to SystemServer/More.
// # with ECapabilityProtServ - higher, "real-time" priorities obtainable.
// - Tests effect of reduction of SystemServer/More from nanothread priority 24 to 23,
// i.e. same priority as AbsoluteHigh
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
//

#include <e32test.h>

LOCAL_D RTest test(_L("T_SPRIORITYCAP"));
RMutex SyncMutex;
TInt threadMutexAcquireOrder; // where 132 = thread 1 acquires mutex, then thread 3 then thread 2
_LIT(KTestPanicCategory,"TEST PANIC");

class RTestThread : public RThread
	{
public:
	void Create(TThreadFunction aFunction,TAny* aArg=0);
	};

void RTestThread::Create(TThreadFunction aFunction,TAny* aThreadNumber)
	{
	TInt threadNumber = reinterpret_cast<TInt>(aThreadNumber);
	ASSERT((threadNumber > 0) && (threadNumber < 10));
	TBuf<20> threadName = _L("TestThread_");
	threadName.AppendNum(threadNumber);
	TInt r=RThread::Create(threadName,aFunction,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,aThreadNumber);
	test(r==KErrNone);
	}

TInt TestThreadWaitMutex(TAny* aThreadNumber)
	{
	TInt threadNumber = reinterpret_cast<TInt>(aThreadNumber);
	ASSERT((threadNumber > 0) && (threadNumber < 10));
	RThread thisThread;
	thisThread.Rendezvous(KErrNone);
	SyncMutex.Wait();
	threadMutexAcquireOrder = threadMutexAcquireOrder*10 + threadNumber;
	SyncMutex.Signal();
	return KErrNone;
	}

// Create three threads with priority 1, 2 and 1, each waiting on a mutex that is already held by the
// main thread. Signal the mutex from the main thread and return the order the threads acquire it
// as an integer where 132 = thread 1 acquires mutex, then thread 3 then thread 2.
//
// In order to work this test requires the three threads to wait on the mutex in the order listed.
// This cannot be guaranteed, but the following points make it a near-certainty:
// (1) This main thread runs at lowest priority
// (2) The three threads initially resume at higher, decreasing priorities.
// (3) We Rendezvous() with the thread just before it waits on the mutex
// (4) Wait some time before creating next thread
// (5) Actual priorities are set after all test threads are waiting
TInt TestThreadMutexAcquireOrder(TThreadPriority aPriorityThread1, TThreadPriority aPriorityThread2)
	{
	RTestThread thread1;
	RTestThread thread2;
	RTestThread thread3;
	TRequestStatus logonStatus1;
	TRequestStatus logonStatus2;
	TRequestStatus logonStatus3;
	TRequestStatus rendezvousStatus;

	threadMutexAcquireOrder = 0; // global variable to hold order in which threads obtain mutex

	RThread thisThread;
	thisThread.SetPriority(EPriorityAbsoluteVeryLow);
	
	// create the SyncMutex global variable and hold it initially
	if(SyncMutex.CreateLocal()!=KErrNone)
		User::Invariant();
	SyncMutex.Wait();

	thread1.Create(TestThreadWaitMutex, reinterpret_cast<TAny*>(1));
	thread1.Logon(logonStatus1);
	thread1.SetPriority(EPriorityAbsoluteHigh);
	thread1.Rendezvous(rendezvousStatus);
	thread1.Resume();
	User::WaitForRequest(rendezvousStatus);
	User::After(500000);

	thread2.Create(TestThreadWaitMutex, reinterpret_cast<TAny*>(2));
	thread2.Logon(logonStatus2);
	thread2.SetPriority(EPriorityAbsoluteForeground);
	thread2.Rendezvous(rendezvousStatus);
	thread2.Resume();
	User::WaitForRequest(rendezvousStatus);
	User::After(500000);

	thread3.Create(TestThreadWaitMutex, reinterpret_cast<TAny*>(3));
	thread3.Logon(logonStatus3);
	thread3.SetPriority(EPriorityAbsoluteBackground);
	thread3.Rendezvous(rendezvousStatus);
	thread3.Resume();
	User::WaitForRequest(rendezvousStatus);
	User::After(500000);

	thread1.SetPriority(aPriorityThread1);
	thread2.SetPriority(aPriorityThread2);
	thread3.SetPriority(aPriorityThread1);

	SyncMutex.Signal();

	User::WaitForRequest(logonStatus1);
	User::WaitForRequest(logonStatus2);
	User::WaitForRequest(logonStatus3);
	test(thread1.ExitType()==EExitKill);
	test(logonStatus1==KErrNone);
	test(thread2.ExitType()==EExitKill);
	test(logonStatus2==KErrNone);
	test(thread3.ExitType()==EExitKill);
	test(logonStatus3==KErrNone);

	thread1.Close();
	thread2.Close();
	thread3.Close();
	SyncMutex.Close();

	return threadMutexAcquireOrder;
	}


enum TTestProcessFunctions
	{
	ETestProcessThreadPrioritiesEqual,
	ETestProcessThreadPrioritiesHighLow
	};

#include "testprocess.h"

TInt DoTestProcess(TInt aTestNum,TInt aArg1,TInt aArg2)
	{
	RThread thread;

	switch(aTestNum)
		{

	case ETestProcessThreadPrioritiesEqual:
		{
		TInt acquireOrder = TestThreadMutexAcquireOrder((TThreadPriority)aArg1, (TThreadPriority)aArg2);
		if (acquireOrder != 123)
			{
			thread.Panic(KTestPanicCategory,999);
			}
		break;
		}

	case ETestProcessThreadPrioritiesHighLow:
		{
		TInt acquireOrder = TestThreadMutexAcquireOrder((TThreadPriority)aArg1, (TThreadPriority)aArg2);
		if (acquireOrder != 132)
			{
			thread.Panic(KTestPanicCategory,999);
			}
		break;
		}

	default:
		User::Panic(_L("T_SPRIORITYCAP"),1);
		}

	return KErrNone;
	}


//---------------------------------------------
//! @SYMTestCaseID KBASE-T_SPRIORITYCAP-0121
//! @SYMTestCaseDesc Check prioritisation of threads with ProtServ capability
//! @SYMTestType UT
//! @SYMREQ PREQ955
//! @SYMTestActions Create sets of three threads with various priorities and have them wait on
//!     a mutex. Signal the mutex to see whether the threads obtain it in priority or wait order.
//!     Note: 2 MMP files build test exe with RealTimeServer and WindowServer process priorities.
//!     Test creates copy of this executable with/without required capabilities.
//! @SYMTestExpectedResults All thread priorities are obtainable to processes with ProtServ, so
//!     confirm they are correctly mapped to absolute priorities, that MuchMore > More etc. and
//!     that SystemServer/More is correctly mapped for SYMBIAN_CURB_SYSTEMSERVER_PRIORITIES macro.
//! @SYMTestPriority Critical
//! @SYMTestStatus Implemented
//---------------------------------------------
void TestPriorityMappingWithProtServ()
	{
	const TUint32 capability = 1u<<ECapabilityProtServ; // only ProtServ capability
	RTestProcess process;

	TProcessPriority processPriority = process.Priority();
	// only call with the following process priorities
	ASSERT((processPriority == EPriorityWindowServer) || (processPriority == EPriorityFileServer)
		|| (processPriority == EPrioritySupervisor) || (processPriority == EPriorityRealTimeServer));

	test.Start(_L("Test EPriorityRealTime is greater than EPriorityMuchMore"));
	process.Create(capability,ETestProcessThreadPrioritiesHighLow,EPriorityRealTime,EPriorityMuchMore);
	process.Run();

	test.Next(_L("Test EPriorityMuchMore is greater than EPriorityMore"));
	process.Create(capability,ETestProcessThreadPrioritiesHighLow,EPriorityMuchMore,EPriorityMore);
	process.Run();

	test.Next(_L("Test EPriorityMore is greater than EPriorityNormal"));
	process.Create(capability,ETestProcessThreadPrioritiesHighLow,EPriorityMore,EPriorityNormal);
	process.Run();

	test.Next(_L("Test EPriorityNormal is greater than EPriorityLess"));
	process.Create(capability,ETestProcessThreadPrioritiesHighLow,EPriorityNormal,EPriorityLess);
	process.Run();

	test.Next(_L("Test EPriorityLess is greater than EPriorityMuchLess"));
	process.Create(capability,ETestProcessThreadPrioritiesHighLow,EPriorityLess,EPriorityMuchLess);
	process.Run();

	test.Next(_L("Test EPriorityMore versus independent capping priority"));
	process.Create(capability,
		(processPriority == EPriorityRealTimeServer) ? ETestProcessThreadPrioritiesHighLow : ETestProcessThreadPrioritiesEqual,
		EPriorityMore,
#ifdef SYMBIAN_CURB_SYSTEMSERVER_PRIORITIES
		EPriorityAbsoluteHigh
#else
		EPriorityAbsoluteRealTime1
#endif	
		);
	process.Run();

	test.End();
	}


//---------------------------------------------
//! @SYMTestCaseID KBASE-T_SPRIORITYCAP-0122
//! @SYMTestCaseDesc Check prioritisation of threads without ProtServ capability
//! @SYMTestType UT
//! @SYMREQ PREQ955
//! @SYMTestActions Create sets of three threads with various priorities and have them wait on
//!     a mutex. Signal the mutex to see whether the threads obtain it in priority or wait order.
//!     Note: 2 MMP files build test exe with RealTimeServer and WindowServer process priorities.
//!     Test creates copy of this executable with/without required capabilities.
//! @SYMTestExpectedResults Confirm thread priorities are capped at SystemServer/More without ProtServ,
//!     so many priority enumerations will map to the same absolute priority. Confirm that
//!     SystemServer/More is correctly mapped for SYMBIAN_CURB_SYSTEMSERVER_PRIORITIES macro.
//! @SYMTestPriority Critical
//! @SYMTestStatus Implemented
//---------------------------------------------
void TestPriorityMappingWithoutProtServ()
	{
	const TUint32 capability = ~(1u<<ECapabilityProtServ); // all capabilities except ProtServ
	RTestProcess process;

	TProcessPriority processPriority = process.Priority();
	// only call with the following process priorities
	ASSERT((processPriority == EPriorityWindowServer) || (processPriority == EPriorityFileServer)
		|| (processPriority == EPrioritySupervisor) || (processPriority == EPriorityRealTimeServer));

	test.Start(_L("Test EPriorityRealTime and EPriorityMuchMore are capped and equal"));
	process.Create(capability,ETestProcessThreadPrioritiesEqual,EPriorityRealTime,EPriorityMuchMore);
	process.Run();

	test.Next(_L("Test EPriorityMuchMore and EPriorityMore are capped and equal"));
	process.Create(capability,ETestProcessThreadPrioritiesEqual,EPriorityMuchMore,EPriorityMore);
	process.Run();

	if (processPriority == EPriorityRealTimeServer)
		{
		test.Next(_L("Test EPriorityMore and EPriorityMuchLess are capped and equal"));
		process.Create(capability,ETestProcessThreadPrioritiesEqual,EPriorityMore,EPriorityMuchLess);
		process.Run();

		test.Next(_L("Test EPriorityNormal and EPriorityMuchLess are capped and equal"));
		process.Create(capability,ETestProcessThreadPrioritiesEqual,EPriorityNormal,EPriorityMuchLess);
		process.Run();

		test.Next(_L("Test EPriorityLess and EPriorityMuchLess are capped and equal"));
		process.Create(capability,ETestProcessThreadPrioritiesEqual,EPriorityLess,EPriorityMuchLess);
		process.Run();
		}

	test.Next(_L("Test EPriorityMore versus EPriorityAbsoluteHigh"));
	process.Create(capability,
#ifdef SYMBIAN_CURB_SYSTEMSERVER_PRIORITIES
		ETestProcessThreadPrioritiesEqual,
#else
		ETestProcessThreadPrioritiesHighLow,
#endif	
		EPriorityMore,EPriorityAbsoluteHigh);
	process.Run();

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

	if(!PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement))
		{
		test.Start(_L("TESTS NOT RUN - EPlatSecEnforcement is OFF"));
		test.End();
		return 0;
		}

	test.Next(_L("Test thread priority mappings for processes with ECapabilityProtServ"));
	TestPriorityMappingWithProtServ();

	test.Start(_L("Test thread priority mappings for processes without ECapabilityProtServ"));
	TestPriorityMappingWithoutProtServ();

	test.End();

	return(0);
    }

