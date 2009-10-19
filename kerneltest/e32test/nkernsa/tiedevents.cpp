// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\nkernsa\tiedevents.cpp
// 
//

#include <nktest/nkutils.h>

//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-tiedevents-2448
//! @SYMTestType				UT
//! @SYMTestCaseDesc			Verifying tied events
//! @SYMPREQ					PREQ2094
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1. 	TiedEventTest: run a reader thread (or several in a group) accessing a
//! 		common data block. A timer, IDFC, or interrupt handler writes to the
//!			data block concurrently - first incrementing all data from 0 to 1 then
//!			decrementing it again.
//! 		
//! @SYMTestExpectedResults
//! 	1.	When the timer/IDFC/interrupt is tied to the thread/group, then the
//!			execution of the event should prevent the thread/group from running on
//!			any processor, and thus readers should never be able to observe the
//!			data being 1. When the event is not tied, the reader should observe
//!			the data as being 1 at least some of the time.
//---------------------------------------------------------------------------------------------------------------------

#ifdef __SMP__

extern "C" void HijackSystemTimer(NSchedulable* aTieTo);

const TInt FlagCount = 2048;
const TInt LoopCount = 100;

volatile TUint32 Flags[FlagCount];
volatile TBool Done;
volatile TUint32 FlagsSet;
NTimer* Timer;
TDfc* IDfc;
NThreadGroup TG;
NFastSemaphore* DoneSem;

enum TTiedMode
	{
	ETimer,
	EInterrupt,
	EIDFC
	};

// Used directly as the IDFC, also called by the timer function
void FiddleFlags(TAny*)
	{
	TInt i;
	for (i=0; i<FlagCount; ++i)
		__e32_atomic_add_ord32(&Flags[i], 1);
	for (i=0; i<FlagCount; ++i)
		__e32_atomic_add_ord32(&Flags[i], (TUint32)-1);
	}

// Used for timer/interrupt cases as the timer function
void TimerFn(TAny*)
	{
	FiddleFlags(NULL);
	if (!__e32_atomic_load_acq32(&Done))
		Timer->OneShot(10);
	}

// Used for IDFC case as the timer function
void IDfcQFn(TAny*)
	{
	IDfc->Add();
	if (!__e32_atomic_load_acq32(&Done))
		Timer->OneShot(10);
	}

// Test thread, just looks for flags being set
void TiedEventThread(TAny*)
	{
	TInt cpu, i, j;
	TUint32 set=0;

	for_each_cpu(cpu)
		{
		NKern::ThreadSetCpuAffinity(NKern::CurrentThread(), cpu);

		for (i=0; i<LoopCount; ++i)
			{
			for (j=0; j<FlagCount; ++j)
				if (__e32_atomic_load_acq32(&Flags[j]))
					++set;
			}
		}

	__e32_atomic_add_ord32(&FlagsSet, set);

	NKern::FSSignal(DoneSem);
	NKern::WaitForAnyRequest();
	}

void TiedEventTest(TBool aTied, TInt aThreads, TTiedMode aMode)
	{
	TEST_PRINT3("TiedEventTest aTied=%d aThreads=%d aMode=%d", aTied, aThreads, aMode);

	// Set up shared parameters
	memclr((void*)&Flags,sizeof(Flags));
	Done = EFalse;
	FlagsSet = 0;

	// Create test threads to check data
	NFastSemaphore exitSem(0);
	NFastSemaphore doneSem(0);
	DoneSem = &doneSem;
	char name[5]={0x54, 0x45, 0x54, 0x31, 0};
	TInt i;
	NSchedulable* tieTo = NULL;
	NThread* t[16];
	NThreadGroup* group = NULL;
	if (aThreads == 1)
		{
		t[0] = CreateUnresumedThreadSignalOnExit(name, TiedEventThread, 10, NULL, 0, KSmallTimeslice, &exitSem, KCpuAffinityAny);
		if (aTied)
			tieTo = t[0];
		}
	else
		{
		group = &TG;
		if (aTied)
			tieTo = group;
		SNThreadGroupCreateInfo ginfo;
		ginfo.iCpuAffinity = KCpuAffinityAny;
		TInt r = NKern::GroupCreate(group, ginfo);
		TEST_RESULT(r==KErrNone, "Failed creating group");
		for (i=0; i<aThreads; ++i)
			{
			t[i] = CreateUnresumedThreadSignalOnExit(name, TiedEventThread, 10, NULL, 0, KSmallTimeslice, &exitSem, KCpuAffinityAny, group);
			++name[3];
			}
		}

#ifndef __X86__
	// Tie the system timer interrupt to the thread if we're testing interrupts
	// This means the timer function should always be exclusive with the theads
	// even though it's not tied itself.
	if (aMode == EInterrupt && tieTo)
		HijackSystemTimer(tieTo);
#endif

	// Create the IDFC
	NSchedulable* tieDFC = aMode == EIDFC ? tieTo : NULL;
	TDfc idfc(tieDFC, FiddleFlags, NULL);
	IDfc = &idfc;

	// Create and start NTimer
	// If we're testing timers it will be tied itself
	// If we're testing interrupts it will not be tied itself but will still run
	// exclusively because the interrupt is tied
	// If we're testing IDFCs it's just used to repeatedly queue the IDFC and
	// where the timer itself runs is irrelevant.
	NSchedulable* tieTimer = aMode == ETimer ? tieTo : NULL;
	NTimerFn timerfn = aMode == EIDFC ? IDfcQFn : TimerFn;
	NTimer timer(tieTimer, timerfn, NULL);
	Timer = &timer;
	timer.OneShot(10);

	// Resume threads
	for (i=0; i<aThreads; ++i)
		NKern::ThreadResume(t[i]);

	// Wait for threads to be done
	for (i=0; i<aThreads; ++i)
		NKern::FSWait(&doneSem);

	// Tell timer to stop requeueing itself
	__e32_atomic_store_rel32(&Done, ETrue);
	NKern::Sleep(100);

#ifndef __X86__
	// Restart the normal system timer if we're testing interrupts
	// as otherwise it will get unbound when the thing it's tied to
	// dies.
	if (aMode == EInterrupt && tieTo)
		HijackSystemTimer(NULL);
#endif

	// Clean up threads/group
	for (i=0; i<aThreads; ++i)
		{
		NKern::ThreadRequestSignal(t[i]);
		NKern::FSWait(&exitSem);
		}
	if (group)
		NKern::GroupDestroy(group);

	// Check that the flag was ok
	TEST_PRINT1("Flag was set %d times", FlagsSet);
	if (aTied)
		TEST_RESULT(FlagsSet == 0, "Flag was set, shouldn't be");
	else
		TEST_RESULT(FlagsSet > 0, "Flag wasn't set, test broken?");
	}
#endif

void TestTiedEvents()
	{
#ifdef __SMP__
	TiedEventTest(EFalse, 1, ETimer);
	TiedEventTest(ETrue, 1, ETimer);
	TiedEventTest(EFalse, 2, ETimer);
	TiedEventTest(ETrue, 2, ETimer);

#ifndef __X86__
	TiedEventTest(EFalse, 1, EInterrupt);
	TiedEventTest(ETrue, 1, EInterrupt);
	TiedEventTest(EFalse, 2, EInterrupt);
	TiedEventTest(ETrue, 2, EInterrupt);
#endif

	TiedEventTest(EFalse, 1, EIDFC);
	TiedEventTest(ETrue, 1, EIDFC);
	TiedEventTest(EFalse, 2, EIDFC);
	TiedEventTest(ETrue, 2, EIDFC);
#endif
	}
