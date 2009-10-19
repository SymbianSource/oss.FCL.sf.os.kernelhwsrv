// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\nkernsa\arm\armutils.cpp
// 
//

#include <arm.h>
#include <nktest/nkutils.h>

const TUint32 KPageSize = 0x1000u;

extern "C" {

void thread_request_signal(NThread* aThread);

TUint64 fcf;
TUint32 nfcf;
TUint32 nfcfs;


TUint32 round_to_page(TUint32 x)
	{
	return (x + KPageSize - 1) &~ (KPageSize - 1);
	}


TLinAddr __initial_stack_base()
	{
	return __stack_pointer() & ~0xfff;
	}

TInt __initial_stack_size()
	{
	return 0x1000;
	}

TUint64 fast_counter_freq()
	{
	return fcf;
	}

TUint32 norm_fast_counter_freq()
	{
	return nfcf;
	}

void init_fast_counter()
	{
	NKern::Sleep(30);
	TUint64 initial = fast_counter();
	NKern::Sleep(1000);
	TUint64 final = fast_counter();
	fcf = final - initial;
	TUint64 f = fcf;
	nfcfs = 0;
	while (f > 2000000)
		f>>=1, ++nfcfs;
	nfcf = (TUint32)(fcf >> nfcfs);

	DEBUGPRINT("fcf=%lx",fcf);
	DEBUGPRINT("nfcf=%d",nfcf);
	}

TInt __microseconds_to_fast_counter(TInt us)
	{
	TUint64 x = TUint64(TUint32(us));
	x *= fcf;
	x += TUint64(500000);
	x /= TUint64(1000000);

	return (TInt)x;
	}

TInt __microseconds_to_norm_fast_counter(TInt us)
	{
	TUint64 x = TUint64(TUint32(us));
	x *= nfcf;
	x += TUint64(500000);
	x /= TUint64(1000000);

	return (TInt)x;
	}

void nfcfspin(TUint32 aTicks)
	{
	TUint64 ticks = aTicks;
	ticks <<= nfcfs;
	fcfspin(ticks);
	}

void fcfspin(TUint64 aTicks)
	{
	TUint64 t0 = fast_counter();
	TUint64 t1;
	do	{
		t1 = fast_counter();
		t1 -= t0;
		} while (t1<aTicks);
	}

extern TUint32 __cpsr();
void CheckPoint()
	{
	KPrintf("CPSR=%08x", __cpsr());
	}

void thread_request_signal(NThread* aThread)
	{
	NKern::ThreadRequestSignal(aThread);
	}
}

TAny* operator new(TUint aSize) __NO_THROW
//
// The global new operator.
//
	{

	return malloc(aSize);
	}

TAny* operator new[](TUint aSize) __NO_THROW
    {

    return malloc(aSize);
    }

void operator delete(TAny* aPtr) __NO_THROW
//
// The replacement delete operator.
//
	{

	free(aPtr);
	}

void operator delete[](TAny* aPtr) __NO_THROW
    {

	free(aPtr);
    }

#ifdef __ARMCC__
TAny* operator new(TUint aSize, const std::nothrow_t& aNoThrow) __NO_THROW
//
// The global new operator.
//
	{
	(void)aNoThrow;
	return malloc(aSize);
	}

TAny* operator new[](TUint aSize, const std::nothrow_t& aNoThrow) __NO_THROW
    {
    (void)aNoThrow;
    return malloc(aSize);
    }
#endif

#ifdef __SMP__
#include <arm_tmr.h>

void TickTimerFn(TAny*);
NFastSemaphore TTSem;

void TTIDfcFn(TAny*)
	{
	TTSem.Signal();
	}

TDfc TTIDfc(&TTIDfcFn,0);
NTimer TickTimer(&TickTimerFn,0);
volatile TInt State=0;
volatile TInt Cycle=0;
volatile TInt WC[1024];
volatile TInt WCI=0;

void TickTimerFn(TAny* aPtr)
	{
	ArmLocalTimer& T = LOCAL_TIMER;
	TInt c = (TInt)T.iWatchdogCount;
	WC[WCI++]=c;
	if (c<0)
		{
		if (++State==3)
			{
			if (++Cycle==3)
				{
				TTIDfc.Add();
				return;
				}
			T.iWatchdogCount = 1900000u;
			State = 0;
			}
		}
	TickTimer.Again(1);
	}

void DoWatchdogTimerTest()
	{
	NKern::FSSetOwner(&TTSem, NKern::CurrentThread());
	ArmLocalTimer& T = LOCAL_TIMER;
	T.iWatchdogLoad = KMaxTUint32;
	T.iWatchdogCount = 2097152;
	T.iWatchdogIntStatus = 1;
	__e32_io_completion_barrier();
	T.iWatchdogCtrl = 3;
	__e32_io_completion_barrier();
	TickTimer.OneShot(1);
	NKern::FSWait(&TTSem);
	TInt i;
	for (i=0; i<WCI; ++i)
		{
		DEBUGPRINT("%03d: %d", i, WC[i]);
		}
	}


#endif


