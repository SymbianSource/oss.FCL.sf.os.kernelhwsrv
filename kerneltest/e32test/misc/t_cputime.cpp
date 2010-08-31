// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\t_cputime.cpp
// Tests User::FastCounter() and RThread::GetCpuTime()
// Note: This test only works on the emulator when run in textshell mode.  The
// reason for this is that is assumes that it will be able to use 100% of CPU
// time, but when techview is starting up there are many other threads consuming
// CPU time.
// 
//

#include <e32test.h>
#include <e32svr.h>
#include <u32hal.h>
#include <hal.h>
#ifdef __WINS__
#include <e32wins.h>
#endif

RTest test(_L("T_CPUTIME"));

_LIT(KUp, "up");
_LIT(KDown, "down");

const TInt KLongWait  = 3000000;  // 3 seconds
const TInt KShortWait =  100000;  // 0.1 seconds
const TInt KTolerance =    1000;  // 1 ms
const TInt numCpus = UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0);

#define FailIfError(EXPR) \
	{ \
	TInt aErr = (EXPR); \
	if (aErr != KErrNone) \
		{ \
		test.Printf(_L("Return code == %d\n"), aErr); \
		test(EFalse); \
		} \
	}

class TThreadParam
	{
public:
	TInt iCpu;
	RSemaphore iSem;
	};

TBool GetCpuTimeIsSupported()
	{
	RThread thread;
	TTimeIntervalMicroSeconds time;
	TInt err = thread.GetCpuTime(time);
	test(err == KErrNone || err == KErrNotSupported);
	return err == KErrNone;
	}

TInt SetCpuAffinity(TInt aCore)
    {
    TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, (TAny *)aCore, 0);
    test(r==KErrNone);  
    return r;
    }


//! @SYMTestCaseID t_cputime_0
//! @SYMTestType CT
//! @SYMTestCaseDesc Fast counter tests
//! @SYMREQ CR RFID-66JJKX
//! @SYMTestActions Compares the high res timer against the nanokernel microsecond tick
//! @SYMTestExpectedResults The differnce measured should be < 1%
//! @SYMTestPriority High
//! @SYMTestStatus Defined
void TestFastCounter()
	{
	test.Start(_L("Comparing NTickCount with FastCounter"));

	TInt tickPeriod = 0;
	FailIfError(HAL::Get(HAL::ENanoTickPeriod, tickPeriod));
	test.Printf(_L("  tick period == %d\n"), tickPeriod);
	
	TInt countFreq = 0;
	FailIfError(HAL::Get(HAL::EFastCounterFrequency, countFreq));
	test.Printf(_L("  count freq == %d\n"), countFreq);

	TBool fcCountsUp = 0;
	FailIfError(HAL::Get(HAL::EFastCounterCountsUp, fcCountsUp));
	test.Printf(_L("  count dir == %S\n"), fcCountsUp ? &KUp : &KDown);

	TUint startTick = User::NTickCount();
	TUint startCount = User::FastCounter();

	User::After(KLongWait);

	TUint endTick = User::NTickCount();
	TUint endCount = User::FastCounter();

	TInt tickDiff = endTick - startTick;
	TInt countDiff = fcCountsUp ? (endCount - startCount) : (startCount - endCount);

	test.Printf(_L("  tick difference == %d\n"), tickDiff);
	test.Printf(_L("  fast count difference == %d\n"), countDiff);

	TInt elapsedTickUs = tickDiff * tickPeriod;
	TInt elapsedCountUs = (TInt)(((TInt64)1000000 * countDiff) / countFreq);

	test.Printf(_L("  tick time == %d\n"), elapsedTickUs);
	test.Printf(_L("  count time == %d\n"), elapsedCountUs);

	TReal diff = (100.0 * Abs(elapsedCountUs - elapsedTickUs)) / elapsedTickUs;

	test.Printf(_L("  %% difference == %f\n"), diff);
	test(diff < 1.0);	
	test.End();
	}

TInt ThreadFunction(TAny* aParam)
	{
	if (numCpus > 1)
		{
		TInt& core = (static_cast<TThreadParam*>(aParam))->iCpu;
		FailIfError(SetCpuAffinity(core));
		}

	RSemaphore& semaphore = (static_cast<TThreadParam*>(aParam))->iSem;
	semaphore.Wait();
	for (;;)
		{
		// Spin
		}
	}

void EnsureSystemIdle()
	{
	// This test assumes 100% cpu resource is available, so it can fail on
	// windows builds if something else is running in the background.  This
	// function attempts to wait for the system to become idle.
	
#ifdef __WINS__

	const TInt KMaxWait = 60 * 1000000;
	const TInt KSampleTime = 1 * 1000000;
	const TInt KWaitTime = 5 * 1000000;
	
	test.Start(_L("Waiting for system to become idle"));
	TInt totalTime = 0;
	TBool idle;
	do
		{
		test(totalTime < KMaxWait);
		
		TThreadParam threadParam;
		FailIfError((threadParam.iSem).CreateLocal(0));
		threadParam.iCpu = 1;

		RThread thread;
		FailIfError(thread.Create(_L("Thread"), ThreadFunction, 1024, NULL, &threadParam));		
		thread.SetPriority(EPriorityLess);
		thread.Resume();

		User::After(KShortWait); // Pause to allow thread setup

		(threadParam.iSem).Signal();		
		User::After(KSampleTime);
		thread.Suspend();

		TTimeIntervalMicroSeconds time;
		FailIfError(thread.GetCpuTime(time));
		TReal error = (100.0 * Abs(time.Int64() - KSampleTime)) / KSampleTime;
		test.Printf(_L("    time == %ld, error == %f%%\n"), time, error);

		idle = error < 2.0;		
		
		thread.Kill(KErrNone);
		TRequestStatus status;
		thread.Logon(status);
		User::WaitForRequest(status);
		test(status == KErrNone);
		CLOSE_AND_WAIT(thread);
		
		(threadParam.iSem).Close();

		if (!idle)
			User::After(KWaitTime);		// Allow system to finish whatever it's doing

		totalTime += KShortWait + KSampleTime + KWaitTime;
		}
	while(!idle);
	
	test.End();
	
#endif
	}

//! @SYMTestCaseID t_cputime_1
//! @SYMTestType CT
//! @SYMTestCaseDesc Thread CPU time tests
//! @SYMREQ CR RFID-66JJKX
//! @SYMTestActions Tests cpu time when a thread is put through the various states
//! @SYMTestExpectedResults Reported cpu time increses only when the thread is running
//! @SYMTestPriority High
//! @SYMTestStatus Defined
void TestThreadCpuTime()
	{
	test.Start(_L("CPU thread time unit tests"));

	TThreadParam threadParam;
	FailIfError((threadParam.iSem).CreateLocal(0));
	threadParam.iCpu = 0;				// Later tests will exercise other CPUs

	RThread thread;
	RUndertaker u;
	TInt h;
	TRequestStatus s;
	FailIfError(thread.Create(_L("Thread"), ThreadFunction, 1024, NULL, &threadParam));
	thread.SetPriority(EPriorityLess);
	FailIfError(u.Create());
	FailIfError(u.Logon(s,h));
	test(s==KRequestPending);

	TTimeIntervalMicroSeconds time, time2;
	TInt64 us;

	// Test cpu time is initially zero
	FailIfError(thread.GetCpuTime(time));
	test(time == 0);

	// Test cpu time is not increased while thread is waiting on semaphore
	thread.Resume();
	User::After(KShortWait);
	FailIfError(thread.GetCpuTime(time2));
	us = time2.Int64();
	test.Printf(_L("Time %dus\n"), us);
	test(us < KTolerance); // wait should happen in less than 1ms

	// Test cpu time increases when thread allowed to run
	// We want to allow 2% tolerance for the thread's CPU time, as there could be
	// something else running on the system during that time which would result lower CPU time than the
	// actual KShortPeriod or KLongPeriod wait time.
	// Also User::After(t) might return within the range of <t, t + 1000000/64 + 2*NanoKarnelTickPeriod>.
	// Given all that - we expect that the the cpu time should be within the range of:
	// <t - 0.02*t, t + 15625 + 2*NanoKernelTickPeriod>
	// or <0.98*t, t + 15625 + 2*NanoKernelTickPeriod>
	TInt user_after_tolerance = 0;
	HAL::Get(HAL::ENanoTickPeriod, user_after_tolerance);
	user_after_tolerance += user_after_tolerance + 15625;

	(threadParam.iSem).Signal();
	User::After(KShortWait);
	FailIfError(thread.GetCpuTime(time));
	us = time.Int64() - time2.Int64();
	test.Printf(_L("Time %dus\n"), us);
	test(100*us >= 98*KShortWait); // left limit
	test(us - KShortWait <= user_after_tolerance); // right limit

	FailIfError(thread.GetCpuTime(time));
	User::After(KLongWait);
	FailIfError(thread.GetCpuTime(time2));
	us = time2.Int64() - time.Int64();
	test.Printf(_L("Time %dus\n"), us);
	test(100*us >= 98*KLongWait); // left limit
	test(us - KLongWait <= user_after_tolerance); // right limit

	// Test not increased while suspended
	thread.Suspend();
	FailIfError(thread.GetCpuTime(time));
	User::After(KShortWait);
	FailIfError(thread.GetCpuTime(time2));
	test(time == time2);
	thread.Resume();

	// Test not increased while dead
	thread.Kill(KErrNone);
	User::WaitForRequest(s);	// wait on undertaker since that completes in supervisor thread
	FailIfError(thread.GetCpuTime(time));
	User::After(KShortWait);
	FailIfError(thread.GetCpuTime(time2));
	test(time == time2);

	RThread t;
	t.SetHandle(h);
	test(t.Id()==thread.Id());
	t.Close();
	u.Close();
	thread.Close();
	(threadParam.iSem).Close();
	test.End();
	}

//! @SYMTestCaseID t_cputime_2
//! @SYMTestType CT
//! @SYMTestCaseDesc Thread CPU time tests
//! @SYMREQ CR RFID-66JJKX
//! @SYMTestActions Tests cpu time when multiple threads are running
//! @SYMTestExpectedResults Total time is divided evenly among running threads
//! @SYMTestPriority High
//! @SYMTestStatus Defined

TBool DoTestThreadCpuTime2()  // Returns ETrue if test passed
	{
	test.Start(_L("Testing time shared between threads"));

	if (numCpus > 1)
		{
		test.Printf(_L("** SMP system detected - not testing time shared between threads until load balancing optimized **\n"));
		return ETrue;
		}

	const TInt KMaxThreads = 4;

	TThreadParam threadParam;
			
	RThread* threads = NULL;
	threads = new(ELeave) RThread[numCpus*KMaxThreads];
	FailIfError((threadParam.iSem).CreateLocal(0));

	TBool pass = ETrue;
	for (TInt numThreads = 1 ; pass && numThreads <= KMaxThreads ; ++numThreads)
		{
		test.Printf(_L("  testing with %d threads on each of %d CPUs:\n"), numThreads, numCpus);

		TInt i, j, k;
		for (i = 0 ; i < numThreads ; ++i)
			{
			for (j = 0 ; j < numCpus ; ++j)
				{
				TBuf<16> name;
				name.AppendFormat(_L("Thread%d%d"), i, j);
				threadParam.iCpu = j;
				k = i+j*KMaxThreads;
				FailIfError(threads[k].Create(name, ThreadFunction, 1024, NULL, &threadParam));
				threads[k].SetPriority(EPriorityLess);
				threads[k].Resume();
				}
			}

		User::After(KShortWait); // Pause to allow thread setup

		(threadParam.iSem).Signal(numThreads*numCpus);		
		User::After(KLongWait);
		for (i = 0 ; i < numThreads ; ++i)
			for (j = 0 ; j < numCpus ; ++j)
				threads[i+j*KMaxThreads].Suspend();

		TInt expected = KLongWait / numThreads;
		for (i = 0 ; i < numThreads ; ++i)
			{
			for (j = 0 ; j < numCpus ; ++j)
				{
				k = i+j*KMaxThreads;
				TTimeIntervalMicroSeconds time;
				FailIfError(threads[k].GetCpuTime(time));

				TReal error = (100.0 * Abs(time.Int64() - expected)) / expected;
			
				test.Printf(_L("    %d%d: time == %ld, error == %d%%\n"), i, j, time.Int64(), TInt(error));

				if (error >= 5.0)
					pass = EFalse;

				threads[k].Kill(KErrNone);
				TRequestStatus status;
				threads[k].Logon(status);
				User::WaitForRequest(status);
				test(status == KErrNone);
				CLOSE_AND_WAIT(threads[k]);
				}
			}
		}

	(threadParam.iSem).Close();
	test.End();

	return pass;
	}

void TestThreadCpuTime2()
	{
#ifdef __WINS__
	TBool pass = EFalse;
	for (TInt retry = 0 ; !pass && retry < 5 ; ++retry)
		{
		if (retry > 0)
			{
			test.Printf(_L("Test failed, retrying...\n"));
			EnsureSystemIdle();
			}
		pass = DoTestThreadCpuTime2();
		}
	test(pass);
#else
	test(DoTestThreadCpuTime2());
#endif
	}

TInt ThreadFunction2(TAny* aParam)
	{
	TTimeIntervalMicroSeconds& time = *(TTimeIntervalMicroSeconds*)aParam;	
	RThread thread;
	return thread.GetCpuTime(time);
	}

#ifdef __MARM__

void DoTestThreadCpuTime3(TAny* aParam, TExitType aExpectedExitType, TInt aExpectedExitReason)
	{
	RThread thread;
	FailIfError(thread.Create(_L("TestThread"), ThreadFunction2, 1024, NULL, aParam));
	thread.Resume();
	TRequestStatus status;
	thread.Logon(status);
	User::WaitForRequest(status);

	TExitCategoryName exitCat = thread.ExitCategory();
	test.Printf(_L("Thread exit with type == %d, reason == %d, cat == %S\n"),
				thread.ExitType(), thread.ExitReason(), &exitCat);
	
	test(thread.ExitType() == aExpectedExitType);
	test(thread.ExitReason() == aExpectedExitReason);
	CLOSE_AND_WAIT(thread);
	}

void TestThreadCpuTime3()
	{
	// Test kernel writes the return value back to user-space with the correct permissions
	TTimeIntervalMicroSeconds time;
	DoTestThreadCpuTime3(&time, 			EExitKill, 0);	// ok
	DoTestThreadCpuTime3((TAny*)0, 			EExitPanic, 3);	// null pointer
	DoTestThreadCpuTime3((TAny*)0x64000000, EExitPanic, 3);	// start of kernel data on moving memory model
	DoTestThreadCpuTime3((TAny*)0xc8000000, EExitPanic, 3);	// start of kernel data on moving multiple model
	}

#endif

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("T_CPUTIME"));

	if (numCpus > 1)
		FailIfError(SetCpuAffinity(0));

	TestFastCounter();
	if (GetCpuTimeIsSupported())
		{
		EnsureSystemIdle();
		TestThreadCpuTime();
		TestThreadCpuTime2();
#ifdef __MARM__
		TestThreadCpuTime3();
#endif
		}
	test.End();
	return 0;
	}
