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
// e32test\nkernsa\rwspinlock.cpp

//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-rwspinlock-2442
//! @SYMTestType				UT
//! @SYMTestCaseDesc			Verifying the nkern SpinLock
//! @SYMPREQ					PREQ2094
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1. 	RWParallelTest: run a number of reader and writer threads accessing a
//! 		common data block. Each writer completely rewrites the block over and
//! 		over, and the readers verify that the data is consistent.
//! 	2. 	RWOrderingTest: run a number of reader and writer threads which spin-
//! 		wait while holding the spinlock. Each works out the maximum time it
//!         had to wait to acquire the spinlock.
//! 		
//! 
//! @SYMTestExpectedResults
//! 	1.	Properties checked:
//! 		1) readers never see a partial write transaction
//! 		2) the number of writers active is never greater than 1
//! 		3) the number of readers active while a writer is active is 0
//! 		4) more than one reader ran concurrently
//! 	
//! 	2. Properties checked:
//! 		5) Threads acquire the spinlock in the order which they asked for it
//!     	   i.e. neither reader nor writer priority but FIFO
//---------------------------------------------------------------------------------------------------------------------

#include <nktest/nkutils.h>

#ifdef __SMP__

// cheap and cheerful, no side effects please
#define MIN(a, b) ((a)<(b)?(a):(b))

// The spinlock, used throughout
TRWSpinLock RW(TSpinLock::EOrderNone);


///////////////////////////////////////////////
// First test: RWParallelTest
//

// Number of words in the data block
#define BLOCK_SIZE 1024
// Number of write transactions to execute in total (across all writers)
#define WRITE_GOAL 100000

// The data block, the first entry is used as the seed value and is just
// incremented by one each time.
TUint32 Array[BLOCK_SIZE];
// The number of readers currently holding the lock
TUint32 Readers = 0;
// The number of writers currently holding the lock
TUint32 Writers = 0;
// The maximum number of readers that were seen holding the lock at once
TUint32 HighReaders = 0;

void RWParallelReadThread(TAny*)
	{
	// high_r is the maximum number of readers seen by this particular reader
	TUint32 c, r, high_r = 0;
	TBool failed;
	do
		{
		failed = EFalse;

		// Take read lock and update reader count
		RW.LockIrqR();
		__e32_atomic_add_ord32(&Readers, 1);

		// Check property 1
		c = Array[0];
		if (!verify_block_no_trace(Array, BLOCK_SIZE))
			failed = ETrue;

		// Update reader count and release read lock
		r = __e32_atomic_add_ord32(&Readers, (TUint32)-1);
		RW.UnlockIrqR();

		TEST_RESULT(!failed, "Array data inconsistent");

		// Update local high reader count
		if (r > high_r)
			high_r = r;
		}
	while (c < WRITE_GOAL);

	// Update HighReaders if our high reader count is greater
	TUint32 global_high = __e32_atomic_load_acq32(&HighReaders);
	do
		{
		if (global_high >= high_r)
			break;
		}
	while (!__e32_atomic_cas_ord32(&HighReaders, &global_high, high_r));
	}

void RWParallelWriteThread(TAny*)
	{
	TUint32 c, r, w;
	do
		{
		// Take write lock and update writer count
		RW.LockIrqW();
		w = __e32_atomic_add_ord32(&Writers, 1);

		// Get reader count
		r = __e32_atomic_load_acq32(&Readers);

		// Increment seed and recalculate array data
		c = ++Array[0];
		setup_block(Array, BLOCK_SIZE);

		// Update writer count and release write lock
		__e32_atomic_add_ord32(&Writers, (TUint32)-1);
		RW.UnlockIrqW();

		// Check properties 2 and 3
		TEST_RESULT(w == 0, "Multiple writers active");
		TEST_RESULT(r == 0, "Reader active while writing");
		}
	while (c < WRITE_GOAL);
	}

void RWParallelTest()
	{
	TEST_PRINT("Testing read consistency during parallel accesses");

	NFastSemaphore exitSem(0);

	// Set up the block for the initial seed of 0
	setup_block(Array, BLOCK_SIZE);

	// Spawn three readers and a writer for each processor, all equal priority
	TInt cpu;
	TInt threads = 0;
	for_each_cpu(cpu)
		{
		CreateThreadSignalOnExit("RWParallelTestR1", &RWParallelReadThread, 10, NULL, 0, KSmallTimeslice, &exitSem, cpu);
		CreateThreadSignalOnExit("RWParallelTestR2", &RWParallelReadThread, 10, NULL, 0, KSmallTimeslice, &exitSem, cpu);
		CreateThreadSignalOnExit("RWParallelTestR3", &RWParallelReadThread, 10, NULL, 0, KSmallTimeslice, &exitSem, cpu);
		CreateThreadSignalOnExit("RWParallelTestW", &RWParallelWriteThread, 10, NULL, 0, KSmallTimeslice, &exitSem, cpu);
		threads += 4;
		}

	// Wait for all threads to terminate
	while (threads--)
		NKern::FSWait(&exitSem);

	// Check property 4
	TUint r = __e32_atomic_load_acq32(&HighReaders);
	TEST_RESULT(r > 1, "Didn't see concurrent reads");

	TEST_PRINT1("Done, max concurrent readers was %d", r);
	}


///////////////////////////////////////////////
// Second test: RWOrderingTest
//

// Number of times for each thread to try the lock
#define ORDERING_REPEATS 5000
// Time base for spinning
#define SPIN_BASE 100
// Time for read threads to spin (prime)
#define READ_SPIN 7
// Time for write threads to spin (different prime)
#define WRITE_SPIN 11
// Maximum write-thread wait seen
TUint32 MaxWriteWait;
// Maximum read-thread wait seen
TUint32 MaxReadWait;

void RWOrderingThread(TAny* aWrite)
	{
	NThreadBase* us = NKern::CurrentThread();
	TUint32 seed[2] = {(TUint32)us, 0};
	TUint32 c, maxdelay = 0;
	for (c = 0; c < ORDERING_REPEATS; ++c)
		{
		// Disable interrupts to stop preemption disrupting timing
		TInt irq = NKern::DisableAllInterrupts();

		// Time taking lock
		TUint32 before = norm_fast_counter();
		if (aWrite)
			RW.LockOnlyW();
		else
			RW.LockOnlyR();
		TUint32 after = norm_fast_counter();
		TUint32 delay = after - before;
		if (delay > maxdelay)
			maxdelay = delay;

		// Spin for a fixed amount of time
		nfcfspin(SPIN_BASE * (aWrite ? WRITE_SPIN : READ_SPIN));

		// Release lock
		if (aWrite)
			RW.UnlockOnlyW();
		else
			RW.UnlockOnlyR();

		// Reenable interrupts
		NKern::RestoreInterrupts(irq);

		// Sleep for a tick ~50% of the time to shuffle ordering
		if (random(seed) & 0x4000)
			NKern::Sleep(1);
		}

	// Update Max{Read,Write}Wait if ours is higher
	TUint32 global_high = __e32_atomic_load_acq32(aWrite ? &MaxWriteWait : &MaxReadWait);
	do
		{
		if (global_high >= maxdelay)
			break;
		}
	while (!__e32_atomic_cas_ord32(aWrite ? &MaxWriteWait : &MaxReadWait, &global_high, maxdelay));
	
	if (aWrite)
		TEST_PRINT1("Write max delay: %d", maxdelay);
	else
		TEST_PRINT1("Read max delay: %d", maxdelay);
	}

void RWOrderingTest()
	{
	TEST_PRINT("Testing lock acquisition ordering");

	NFastSemaphore exitSem(0);

	TInt cpus = NKern::NumberOfCpus();
	TInt writers, cpu;
	for (writers = 0; writers <= cpus; ++writers)
		{
		TInt readers = cpus - writers;

		// reset maximums
		__e32_atomic_store_rel32(&MaxWriteWait, 0);
		__e32_atomic_store_rel32(&MaxReadWait, 0);

		// start one thread on each cpu, according to readers/writers
		for (cpu = 0; cpu < writers; ++cpu)
			CreateThreadSignalOnExit("RWOrderingTestW", &RWOrderingThread, 10, (TAny*)ETrue, 0, KSmallTimeslice, &exitSem, cpu);
		for (       ; cpu < cpus; ++cpu)
			CreateThreadSignalOnExit("RWOrderingTestR", &RWOrderingThread, 10, (TAny*)EFalse, 0, KSmallTimeslice, &exitSem, cpu);

		// Wait for all threads to terminate
		while (cpu--)
			NKern::FSWait(&exitSem);

		// Get, round, and print maximum delays
		TUint32 w = __e32_atomic_load_acq32(&MaxWriteWait);
		TUint32 r = __e32_atomic_load_acq32(&MaxReadWait);
		w += (SPIN_BASE/2) - 1;
		r += (SPIN_BASE/2) - 1;
		w /= SPIN_BASE;
		r /= SPIN_BASE;
		TEST_PRINT4("%d writers, %d readers, max delay: write %d read %d", writers, readers, w, r);

		// Work out expected delays
		// For writers, we might have every other writer ahead of us, with the readers interleaved
		TUint32 we = ((writers-1) * WRITE_SPIN) + (MIN(readers  , writers) * READ_SPIN);
		// For readers, we might have every writer ahead of us, with the other readers interleaved
		TUint32 re = ((writers  ) * WRITE_SPIN) + (MIN(readers-1, writers) * READ_SPIN);

		// Compare
		if (writers)
			{
			TEST_PRINT1("Expected write %d", we);
			TEST_RESULT(w==we, "Write delay not expected time");
			}
		if (readers)
			{
			TEST_PRINT1("Expected read %d", re);
			TEST_RESULT(r==re, "Read delay not expected time");
			}
		}

	TEST_PRINT("Done");
	}


/////////////////////
// Run all tests
void TestRWSpinLock()
	{
	TEST_PRINT("Testing R/W Spinlocks...");

	RWParallelTest();
	RWOrderingTest();
	}

#else

void TestRWSpinLock()
	{
	TEST_PRINT("Skipping R/W Spinlock tests on uniproc");
	}

#endif
