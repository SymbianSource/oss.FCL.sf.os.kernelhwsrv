// Copyright (c) 1995-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\prime\t_timer.cpp
// Overview:
// Time and Timer tests.
// API Information:
// RTimer, TTime
// Details:
// - Test relative timers using the RTimer::After() method. Verify 
// results are as expected.
// - Set the date and time using TTime::HomeTime() and verify results
// are as expected.
// - Test absolute timers using RTimer::At() method. Verify results
// are as expected.
// - Test the timer is ok if its thread terminates.
// - Test synchronising time via the RTimer::Lock() method. Verify 
// results are as expected.
// - Test locked timers abort when the system time changes.
// - Test User::ResetInactivityTime() results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
//
//

// the following was used to help debug emulator implemenation of user mode callbacks
//#define REQUEST_STATUS_POLL_SOAK_TEST  

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32hal.h>
#include <e32panic.h>
#include <hal.h>
#include <e32power.h>
#include <e32math.h>

LOCAL_D RTest test(_L("T_TIMER"));
TInt MachineUid;

TInt AfterNegative(TAny*)
	{
	RTimer t;
	TInt r=t.CreateLocal();
	test(r==KErrNone);
	TRequestStatus s;
	t.After(s,-1);
	return KErrNone;
	}

TInt AfterTwice(TAny*)
	{
	RTimer t;
	TInt r=t.CreateLocal();
	test(r==KErrNone);
	TRequestStatus s;
	t.After(s,1000000);
	test(s==KRequestPending);
	t.After(s,1000000);
	return KErrNone;
	}

void PrintTime()
	{
	TTime now;
	now.HomeTime();
	TDateTime dt(now.DateTime());
	test.Printf(_L("Time: %02d:%02d:%02d:%06d\n"),dt.Hour(),dt.Minute(),dt.Second(),dt.MicroSecond());
	}

TBool RequestIsComplete(TRequestStatus& s)
	{
	return s != KRequestPending;
	}


LOCAL_C void testRel()
//
// Test relative timers.
//
	{
	test.Start(_L("After 0"));
	RTimer t;
	TInt r=t.CreateLocal();
	test(r==KErrNone);
	TRequestStatus s;
	t.After(s,0);
	test(s==KRequestPending || s==KErrNone);
	User::WaitForRequest(s);
	test(s==KErrNone);

	test.Next(_L("After 1 tenth"));
	t.After(s,100000);
#ifdef __WINS__
	// On WINS we can't guarantee thread scheduling so timer may already have
	// completed before we get to test the status. Therefore, allow KErrNone.
	test(s==KRequestPending || s==KErrNone);
	if(s==KErrNone)
		test.Printf(_L("NOTE: completed 'early'"));
#else
	test(s==KRequestPending);
#endif
	User::WaitForRequest(s);
	test(s==KErrNone);

	test.Next(_L("After -1 millionth"));
	RThread thread;
	r=thread.Create(_L("After -1"),AfterNegative,KDefaultStackSize,NULL,&thread);
	test(r==KErrNone);
	thread.Logon(s);
	test(s==KRequestPending);
	TBool justInTime=User::JustInTime();
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(s);
	test(s==ERTimerAfterTimeNegative);
	test(thread.ExitCategory()==_L("USER"));
	test(thread.ExitReason()==ERTimerAfterTimeNegative);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
	User::SetJustInTime(justInTime);

	test.Next(_L("After 1 second"));
	t.After(s,1000000);
	test(s==KRequestPending);
	User::WaitForRequest(s);
	test(s==KErrNone);

	test.Next(_L("After 1 second polling"));
	t.After(s,1000000);
	test(s==KRequestPending);
	// Have to be careful the compiler doesn't optimise this away
	while(!RequestIsComplete(s))
		; // poll
	test(s==KErrNone);
	User::WaitForRequest(s);

	test.Next(_L("Cancel"));
	t.After(s,1000000);
	test(s==KRequestPending);
	t.Cancel();
	User::WaitForRequest(s);
	test(s==KErrCancel);
	t.Close();

	test.Next(_L("Request twice"));
	r=thread.Create(_L("After twice"),AfterTwice,KDefaultStackSize,NULL,&thread);
	test(r==KErrNone);
	thread.Logon(s);
	test(s==KRequestPending);
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(s);
	test(s==ETimerAlreadyPending);
	test(thread.ExitCategory()==_L("KERN-EXEC"));
	test(thread.ExitReason()==ETimerAlreadyPending);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
	User::SetJustInTime(justInTime);

	test.End();
	}

#ifdef REQUEST_STATUS_POLL_SOAK_TEST

static volatile TBool PollTestRunning;

LOCAL_C TInt PollThread(TAny* aArg)
	{
	const TInt KMaxTimers = 1000;

	TInt threadIndex = (TInt)aArg;
	TInt64 seed = 5511498647534504549 + RThread().Id();
	RTimer timers[KMaxTimers];
	TRequestStatus statuses[KMaxTimers];

	TInt i;
	for (i = 0 ; i < KMaxTimers ; ++i)
		{
		test_KErrNone(timers[i].CreateLocal());
		statuses[i] = 1;
		}

	TInt totalComplete = 0;
	TInt totalWaiting = 0;

	while(PollTestRunning)
		{
		for (i = 0 ; i < KMaxTimers ; ++i)
			{
			switch(statuses[i].Int())
				{
				case KRequestPending:
					// do nothing
					++totalWaiting;
					break;

				case KErrNone:
					User::WaitForRequest(statuses[i]);
					++totalComplete;
					// fall through

				case 1:
					{
					TInt after = ((TUint)Math::Rand(seed) >> 28) + 1;
					timers[i].HighRes(statuses[i], after);
					}
					break;

				default:
					return statuses[i].Int();
				}
			}
		}

	for (i = 0 ; i < KMaxTimers ; ++i)
		{
		User::WaitForRequest(statuses[i]);
		if (statuses[i].Int() != KErrNone)
			return statuses[i].Int();
		timers[i].Close();
		}

	RDebug::Printf("%d: %d %d\n", threadIndex, totalComplete, totalWaiting);
	return KErrNone;
	}

LOCAL_C void testPoll()
	{
	const TInt KMaxThreads = 10;
	const TInt KSecondsToTest = 60;

	RThread threads[KMaxThreads];
	TRequestStatus statuses[KMaxThreads];

	test.Start(_L("Test polling"));

	PollTestRunning = ETrue;

	TInt i;
	for (i = 0 ; i < KMaxThreads ; ++i)
		{
		test_KErrNone(threads[i].Create(KNullDesC, PollThread, 0x1000, NULL, (TAny*)i));
		threads[i].Logon(statuses[i]);
		threads[i].Resume();
		}

	User::After(KSecondsToTest * 1000 * 1000);

	PollTestRunning = EFalse;

	for (i = 0 ; i < KMaxThreads ; ++i)
		{
		User::WaitForRequest(statuses[i]);
		test_KErrNone(statuses[i].Int());
		test_Equal(EExitKill, threads[i].ExitType());
		threads[i].Close();
		}

	test.End();
	}

#endif


LOCAL_C void testHomeTime()
//
// Test HomeTime.
//
	{
    TTime t1, t2;
    t1.HomeTime();
    for (TInt x=0;x<100;x++)
        {
        do
            {
            t2.HomeTime();
            }
        while (t2==t1);

		if (t2 <= t1 && t1.MicroSecondsFrom(t2) > TTimeIntervalMicroSeconds(1000)) // HomeTime() only operates at ms precision
			{
			test.Printf(_L("Time comparison failed\r\n"));
			test.Printf(_L("Before: 0x%lx\r\n"), t1.Int64());
			test.Printf(_L("After:  0x%lx\r\n"), t2.Int64());
			test(t2>t1);
			}

        t1=t2;
        }
    }

TInt AtTwice(TAny*)
	{
	RTimer t;
	TInt r=t.CreateLocal();
	test(r==KErrNone);
	TRequestStatus s;
	TTime time;
	time.UniversalTime();
	t.AtUTC(s,time+TTimeIntervalSeconds(1));
	test(s==KRequestPending);
	t.AtUTC(s,time+TTimeIntervalSeconds(2));
	return KErrNone;
	}

TInt AtAfter(TAny*)
	{
	RTimer t;
	TInt r=t.CreateLocal();
	test(r==KErrNone);
	TRequestStatus s;
	TTime time;
	time.UniversalTime();
	t.AtUTC(s,time+TTimeIntervalSeconds(1));
	test(s==KRequestPending);
	t.After(s,1000000);
	return KErrNone;
	}

TInt AfterAt(TAny*)
	{
	RTimer t;
	TInt r=t.CreateLocal();
	test(r==KErrNone);
	TRequestStatus s;
	TTime time;
	time.UniversalTime();
	t.After(s,1000000);
	test(s==KRequestPending);
	t.AtUTC(s,time+TTimeIntervalSeconds(2));
	return KErrNone;
	}


LOCAL_C void testAbs()
//
// Test absolute timers.
//
	{
	test.Start(_L("Now -1"));
	RTimer t;
	TInt r=t.CreateLocal();
	test(r==KErrNone);
	TRequestStatus s;
	TTime time;
	time.UniversalTime();
	t.AtUTC(s,time+TTimeIntervalSeconds(-2));
	test(s==KErrUnderflow);  // =KRequestPending
	User::WaitForRequest(s);
	test(s==KErrUnderflow);

	TTime time2;
	test.Next(_L("Synchronise to clock"));
	time.UniversalTime();
    TDateTime dateTime=time.DateTime();
	dateTime.SetMicroSecond(0);
    time=dateTime;
 	time+=TTimeIntervalSeconds(2);
	t.AtUTC(s,time);
	User::WaitForRequest(s);

	test.Next(_L("Now +1"));
	time += TTimeIntervalSeconds(1);
	t.AtUTC(s,time);
	test(s==KRequestPending);
	User::WaitForRequest(s);
	time2.UniversalTime();
	test(s==KErrNone);
	TTimeIntervalMicroSeconds delay=time2.MicroSecondsFrom(time);
	// Test we are in the same second as the requested time...
	test(delay>=TTimeIntervalMicroSeconds(0));
	test(delay<TTimeIntervalMicroSeconds(1000000));

	test.Next(_L("Now +3"));
	time += TTimeIntervalSeconds(3);
	t.AtUTC(s,time);
	test(s==KRequestPending);
	User::WaitForRequest(s);
	time2.UniversalTime();
	test(s==KErrNone);
	delay=time2.MicroSecondsFrom(time);
	// Test we are in the same second as the requested time...
	test(delay>=TTimeIntervalMicroSeconds(0));
	test(delay<TTimeIntervalMicroSeconds(1000000));

	test.Next(_L("UTC vs local"));
	TTimeIntervalSeconds savedOffset = User::UTCOffset();
	User::SetUTCOffset(3600);

	time.HomeTime();
	time += TTimeIntervalSeconds(1);
	t.At(s,time);
	test(s==KRequestPending);
	User::WaitForRequest(s);
	time2.HomeTime();
	test(s==KErrNone);
	delay=time2.MicroSecondsFrom(time);
	// Test we are in the same second as the requested time...
	test(delay>=TTimeIntervalMicroSeconds(0));
	test(delay<TTimeIntervalMicroSeconds(1000000));

	time.UniversalTime();
	time += TTimeIntervalSeconds(1);
	t.AtUTC(s,time);
	test(s==KRequestPending);
	User::WaitForRequest(s);
	time2.UniversalTime();
	test(s==KErrNone);
	delay=time2.MicroSecondsFrom(time);
	// Test we are in the same second as the requested time...
	test(delay>=TTimeIntervalMicroSeconds(0));
	test(delay<TTimeIntervalMicroSeconds(1000000));

	User::SetUTCOffset(savedOffset);	

	test.Next(_L("Cancel"));
	time.UniversalTime();
	t.AtUTC(s,time+TTimeIntervalSeconds(10));
	test(s==KRequestPending);
	t.Cancel();
	User::WaitForRequest(s);
	test(s==KErrCancel);
	t.Close();						

	test.Next(_L("Request twice"));
	RThread thread;
	r=thread.Create(_L("At twice"),AtTwice,KDefaultStackSize,NULL,&thread);
	test(r==KErrNone);
	thread.Logon(s);
	test(s==KRequestPending);
	TBool justInTime=User::JustInTime();
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(s);
	User::SetJustInTime(justInTime);
	test(s==ETimerAlreadyPending);
	test(thread.ExitCategory()==_L("KERN-EXEC"));
	test(thread.ExitReason()==ETimerAlreadyPending);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);

	r=thread.Create(_L("At After"),AtAfter,KDefaultStackSize,NULL,&thread);
	test(r==KErrNone);
	thread.Logon(s);
	test(s==KRequestPending);
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(s);
	User::SetJustInTime(justInTime);
	test(s==ETimerAlreadyPending);
	test(thread.ExitCategory()==_L("KERN-EXEC"));
	test(thread.ExitReason()==ETimerAlreadyPending);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);

	r=thread.Create(_L("After At"),AfterAt,KDefaultStackSize,NULL,&thread);
	test(r==KErrNone);
	thread.Logon(s);
	test(s==KRequestPending);
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(s);
	User::SetJustInTime(justInTime);
	test(s==ETimerAlreadyPending);
	test(thread.ExitCategory()==_L("KERN-EXEC"));
	test(thread.ExitReason()==ETimerAlreadyPending);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);

	test.End();
	}

TInt LockTwice(TAny*)
	{
	RTimer t;
	test(t.CreateLocal()==KErrNone);
	TRequestStatus stat;
	t.Lock(stat, ETwelveOClock);
	User::WaitForRequest(stat);
	test(stat==KErrGeneral);
	t.Lock(stat, ETwelveOClock);
	t.Lock(stat, ETwelveOClock);
	return KErrNone;
	}


LOCAL_C void testLock()
//
// Test locked timers
//
	{
	test.Start(_L("Test synchronise to ETwelveOClock"));
	RTimer t;
	TTime time,time2;
	test(t.CreateLocal()==KErrNone);
	TRequestStatus stat;
	t.Lock(stat, ETwelveOClock);
	User::WaitForRequest(stat);
	test(stat==KErrGeneral);
	time.UniversalTime();
	t.Lock(stat, ETwelveOClock);
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	test(User::LockPeriod()==ETwelveOClock);
	time2.UniversalTime();
	test(time<time2);
	User::After(500000);
	test.Next(_L("Test sync to EOneOClock for 4 seconds"));
	t.Lock(stat, EOneOClock);
	User::WaitForRequest(stat);
	test(stat==KErrGeneral);
	time.UniversalTime();
	TInt i;
	for (i=0; i<5; i++)
		{
		t.Lock(stat, EOneOClock);
		User::WaitForRequest(stat);
		test(stat==KErrNone);
		test.Printf(_L("."));
		}
	time2.UniversalTime();
	TTimeIntervalSeconds ti;
	test(time2.SecondsFrom(time, ti)==KErrNone);
	test(ti>=TTimeIntervalSeconds(4));
	test.Printf(_L("\n"));
	test.Next(_L("Test sync to every half second, from EFourOClock for 5 seconds"));
	t.Lock(stat, ETwelveOClock);
	User::WaitForRequest(stat);
	for (i=0; i<5; i++)
		{
		t.Lock(stat, EFourOClock);
		User::WaitForRequest(stat);
		test(stat==KErrNone);
		test.Printf(_L("."));
		t.Lock(stat, ETenOClock);
		User::WaitForRequest(stat);
		test(stat==KErrNone);
		test.Printf(_L(","));
		}
	test.Printf(_L("\n"));
	test.Next(_L("Test KErrGeneral after delay"));
	User::After(1000000);
	t.Lock(stat,EThreeOClock);
	User::WaitForRequest(stat);
	test(stat==KErrGeneral);
	test.Next(_L("Test cancel, and re-request immediately"));
	User::After(1000000);
	t.Lock(stat, ETwelveOClock);
	User::WaitForRequest(stat);
	test(stat==KErrGeneral);
	t.Lock(stat, EElevenOClock);
	t.Cancel();
	User::WaitForRequest(stat);
	test(stat==KErrCancel);
	t.Lock(stat, EElevenOClock);
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	test.Next(_L("Test complete a request at 1, then cancel a request for 11, and re-request at 3 gives KErrGeneral"));
	User::After(1000000);
	t.Lock(stat, ETwelveOClock);
	User::WaitForRequest(stat);
	test(stat==KErrGeneral);
	t.Lock(stat,EOneOClock);
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	t.Lock(stat,EElevenOClock);
	User::After(400000); // ensure EThreeOClock is in the past
	t.Cancel();
	User::WaitForRequest(stat);
	test(stat==KErrCancel);
	t.Lock(stat,EThreeOClock);
	User::WaitForRequest(stat);
	// EThreeOClock should be more than one second away from the previous timer expiration
	test(stat==KErrGeneral);

	test.Next(_L("Lock twice"));
	RThread thread;
	TInt r=thread.Create(_L("Lock twice"),LockTwice,KDefaultStackSize,NULL,&thread);
	test(r==KErrNone);
	thread.Logon(stat);
	test(stat==KRequestPending);
	TBool justInTime=User::JustInTime();
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(stat);
	User::SetJustInTime(justInTime);
	test(stat==ETimerAlreadyPending);
	test(thread.ExitCategory()==_L("KERN-EXEC"));
	test(thread.ExitReason()==ETimerAlreadyPending);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);

#if !(defined(__EPOC32__) && defined(__X86__))
	TInt muid = 0;
	HAL::Get(HAL::EMachineUid, muid);
	if(muid==HAL::EMachineUid_OmapH2 || muid==HAL::EMachineUid_OmapH4 || muid==HAL::EMachineUid_OmapH6)
		{
		test.Next(_L("Test sequential locks fail over on/off"));
		RTimer tat;
		TRequestStatus sat;
		r=tat.CreateLocal();
		TTime now;
		now.UniversalTime();
		tat.AtUTC(sat, now+TTimeIntervalSeconds(10)); // turn on in 10 seconds
		test(sat==KRequestPending);
		t.Lock(stat, ETwelveOClock);
		User::WaitForRequest(stat);
		test(stat==KErrGeneral);
		t.Lock(stat, EElevenOClock);
		User::WaitForRequest(stat);
		PrintTime();
		// Go to standby 
		r = Power::EnableWakeupEvents(EPwStandby);
		test (r == KErrNone);
		r = Power::PowerDown();
		test (r == KErrNone);
		test(stat==KErrNone);
		PrintTime();
		t.Lock(stat, EElevenOClock);
		User::WaitForRequest(stat);
		test(stat==KErrGeneral);
		tat.Close();
		}
#endif

	t.Close();
	test.End();
	}


void testChange()
//
// Bug HA-255
// Test locked timers abort when the system time changes
//
	{
    RTimer rr;
	TRequestStatus stat;
    rr.CreateLocal();
    rr.Lock(stat, ETwelveOClock);
    User::WaitForRequest(stat);
    test(stat==KErrGeneral);
    RTimer rrr;
    rrr.CreateLocal();
    rrr.After(stat, 1000000);
    User::WaitForRequest(stat);

	RTimer r;
	TRequestStatus sstat;
	TTime t;
	r.CreateLocal();
	r.Lock(stat,ETwelveOClock);
	rr.Lock(sstat,EOneOClock);
	User::WaitForRequest(stat);
	test(stat==KErrGeneral);
	User::WaitForRequest(sstat);
	test(sstat==KErrGeneral);
	r.Lock(stat,ETwelveOClock);
	rr.Lock(sstat,EOneOClock);
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	User::WaitForRequest(sstat);
	test(sstat==KErrNone);
	t.UniversalTime();
	r.Lock(stat,ETwelveOClock);
	rr.Lock(sstat,EOneOClock);
	TInt ret=User::SetUTCTime(t-TTimeIntervalSeconds(100));
	test(ret==KErrNone);
	t.UniversalTime();
	ret=User::SetUTCTime(t+TTimeIntervalSeconds(100));
	test(ret==KErrNone);
	User::WaitForRequest(stat);
	test(stat==KErrAbort);
	User::WaitForRequest(sstat);
	test(sstat==KErrAbort);

	// Check that changing the *secure* time *doesn't* abort a locked timer
	r.Lock(stat, ETwelveOClock);
	User::WaitForRequest(stat);		// stat will be KErrGeneral after abort above, but time will be TwelveOClock anyway
	t.UniversalTimeSecure();
	r.Lock(stat, EEightOClock);
	ret = User::SetUTCTimeSecure(t+TTimeIntervalSeconds(100));
	User::WaitForRequest(stat);		// this timer should complete at EightOClock with status KErrNone, *not* KErrAbort
	r.Lock(sstat, ETwelveOClock);
	User::WaitForRequest(sstat);	// this should complete one whole second after we read the secure time above
	User::SetUTCTimeSecure(t+TTimeIntervalSeconds(1));
	test(stat == KErrNone);
	test(sstat == KErrNone);
	if (ret != KErrNone)
		RDebug::Printf("WARNING: Secure clock change test skipped because secure time could not be changed!");

	r.Close();
	rr.Close();
	rrr.Close();
	}

void testInactivity()
	{
	test.Start(_L("Test User::ResetInactivityTime()"));
	RTimer t,t2;
	TRequestStatus stat,stat2;
	TTimeIntervalSeconds inact;
	t.CreateLocal();
	t2.CreateLocal();
	User::ResetInactivityTime();
	t.Inactivity(stat, 4);
	t2.Inactivity(stat2, 2);
	TTime now;
	now.UniversalTime();
	TInt r=User::SetUTCTime(now+TTimeIntervalDays(1));
	test(r==KErrNone);
	test(stat==KRequestPending);
	test(stat2==KRequestPending);
	r=User::SetUTCTime(now-TTimeIntervalDays(1));
	test(r==KErrNone);
	test(stat==KRequestPending);
	test(stat2==KRequestPending);
	r=User::SetUTCTime(now);
	test(r==KErrNone);
	test(stat==KRequestPending);
	test(stat2==KRequestPending);
	User::After(1000000);
	User::ResetInactivityTime();
	test(stat==KRequestPending);
	test(stat2==KRequestPending);
	User::After(3000000);
	User::ResetInactivityTime();
	test(stat==KRequestPending);
	test(stat2!=KRequestPending);
	User::After(2000000);
	User::ResetInactivityTime();
	test(stat==KRequestPending);
	User::After(2000000);
	User::ResetInactivityTime();
	test(stat==KRequestPending);
	User::After(5000000);
	test(stat!=KRequestPending);
	inact=User::InactivityTime();
	test.Printf(_L("User inactivity after 5 secs, reports %d secs\n"),inact.Int());
	test(inact > TTimeIntervalSeconds(3)); // test that inactivity lasted more than 3 seconds
	test(inact < TTimeIntervalSeconds(7)); // test that inactivity lasted less than 7 seconds
	test.End();
	}


GLDEF_C TInt E32Main()
//
// Test timers.
//
    {
	test.Title();
	TInt r=HAL::Get(HAL::EMachineUid,MachineUid);
	test(r==KErrNone);
	test.Start(_L("Testing relative timers"));
	testRel();

#ifdef REQUEST_STATUS_POLL_SOAK_TEST
	test.Next(_L("Testing polling"));
	testPoll();
#endif

    test.Next(_L("Testing HomeTime()"));
    testHomeTime();

	test.Next(_L("Testing absolute timers"));
	testAbs();

	test.Next(_L("Testing locked timers"));
	testLock();

	test.Next(_L("Testing changing time"));
	testChange();

	test.Next(_L("Testing inactivity timers"));
	testInactivity();

	test.End();
	return(KErrNone);
    }

