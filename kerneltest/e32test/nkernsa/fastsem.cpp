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
// e32test\nkernsa\fastsem.cpp
// 
//

#define __INCLUDE_NTHREADBASE_DEFINES__

#include <nktest/nkutils.h>

void CheckSemaphoreCount(NFastSemaphore* aS, TInt aExpected)
	{
	TInt w = 0;
	TInt r;
	do	{
		r = WaitWithTimeout(aS, KMinTimeout);
		TEST_RESULT1(r==KErrNone || r==KErrTimedOut, "Invalid return code %d", r);
		if (r == KErrNone)
			++w;
		} while(r == KErrNone);
	TEST_RESULT2(w==aExpected, "Signalled %d, Waited %d", aExpected, w);
	}

void FSTest1(TAny* a)
	{
	TInt n = (TInt)a;
	NFastSemaphore s(0);

	TInt i;
	for (i=0; i<n; ++i) NKern::FSSignal(&s);
	CheckSemaphoreCount(&s, n);
	NKern::FSSignalN(&s, n);
	CheckSemaphoreCount(&s, n);
	}

#define DO_FS_TEST1(n,a)	CreateThreadAndWaitForExit("FSTest1", &FSTest1, 12, (TAny*)(n), 0, -1, (a))

struct SFSTest2Info
	{
	NFastSemaphore iSem;
	volatile TInt iBlockCount;
	volatile TInt iSignals;
	volatile TInt iWaits;
	volatile TBool iStart;
	volatile TBool iStop;
	};

void FSTest2Signaller0(TAny* a)
	{
	SFSTest2Info& info = *(SFSTest2Info*)a;
	while (!info.iStart)
		{
		}
//	NThreadBase* t = info.iSem.iOwningThread;
	while (!info.iStop)
		{
		++info.iBlockCount;
		__e32_atomic_add_ord32(&info.iSignals, 1);
		NKern::FSSignal(&info.iSem);
		}
	TEST_PRINT1("Ran %d times", info.iBlockCount);
	}

#ifdef __SMP__
class NKTest
	{
public:
	static TBool ThreadIsBlocked(NThreadBase* aT)
		{ return aT->iWaitState.ThreadIsBlocked(); }
	};
#endif

void FSTest2Signaller(TAny* a)
	{
	SFSTest2Info& info = *(SFSTest2Info*)a;
	while (!info.iStart)
		{
		}
	NThreadBase* t = info.iSem.iOwningThread;
	TInt count0=0;
	TInt countneg=0;
	TInt blocked=0;
	TInt prev_block_count = info.iBlockCount;
	TInt tries = 1;
	TUint32 seed[2];
	seed[0] = NKern::CurrentCpu()+1;
	seed[1] = 0;
	while (!info.iStop)
		{
		TInt c = info.iSem.iCount;
		if (c>=1)
			continue;
		if (--tries==0)
			{
			TInt bc;
			do	{
				bc = info.iBlockCount;
				} while (bc<=prev_block_count);
			prev_block_count = bc;
			tries = random(seed) & 127;
			tries += 71;
			}
		TUint32 x = random(seed) & 63;
		while (x)
			--x;
		c = info.iSem.iCount;
		NKern::FSSignal(&info.iSem);
		__e32_atomic_add_ord32(&info.iSignals, 1);
		if (c==0) ++count0;
		if (c<0) ++countneg;
#ifdef __SMP__
		if (NKTest::ThreadIsBlocked(t)) ++blocked;
#else
		if (t->iNState == NThread::EWaitFastSemaphore) ++blocked;
#endif
		}
	TEST_PRINT1("Count =0 %d times", count0);
	TEST_PRINT1("Count <0 %d times", countneg);
	TEST_PRINT1("Blocked  %d times", blocked);
	}

void FSTest2(TAny* a)
	{
	SFSTest2Info& info = *(SFSTest2Info*)a;
	NFastSemaphore exitSem(0);
	NKern::FSSetOwner(&info.iSem, 0);
	info.iBlockCount = 0;
	info.iWaits = 0;
	info.iSignals = 0;
	info.iStart = FALSE;
	info.iStop = FALSE;
	TInt cpu;
	TInt threads = 0;
	TInt this_cpu = NKern::CurrentCpu();
	for_each_cpu(cpu)
		{
		if (cpu==this_cpu)
			CreateThreadSignalOnExit("FSTest2Sig0", &FSTest2Signaller0, 11, a, 0, KSmallTimeslice, &exitSem, cpu);
		else
			CreateThreadSignalOnExit("FSTest2Sig", &FSTest2Signaller, 12, a, 0, KSmallTimeslice, &exitSem, cpu);
		++threads;
		}

	info.iStart = TRUE;
	while(info.iWaits < 1048576)
		{
		NKern::FSWait(&info.iSem);
		++info.iWaits;
		}

	info.iStop = TRUE;
	while (threads--)
		NKern::FSWait(&exitSem);
	TEST_PRINT1("Leftover signals %d", info.iSignals-info.iWaits);
	TInt r;
	do	{
		r = WaitWithTimeout(&info.iSem, KMinTimeout);
		TEST_RESULT1(r==KErrNone || r==KErrTimedOut, "Invalid return code %d", r);
		if (r == KErrNone)
			++info.iWaits;
		} while(r == KErrNone);
	TEST_PRINT2("Signalled %d, Waited %d", info.iSignals, info.iWaits);
	TEST_RESULT(info.iWaits==info.iSignals, "MISMATCH!");
	}

void DoFsTest2()
	{
	SFSTest2Info info;
	CreateThreadAndWaitForExit("FSTest2", &FSTest2, 12, (TAny*)&info, 0, KSmallTimeslice, 0);
	}


void TestFastSemaphore()
	{
	TEST_PRINT("Testing Fast Semaphores...");

	TInt cpu;
	for_each_cpu(cpu)
		{
		DO_FS_TEST1(0,cpu);
		DO_FS_TEST1(1,cpu);
		DO_FS_TEST1(2,cpu);
		DO_FS_TEST1(13,cpu);
		}

	DoFsTest2();
	}
