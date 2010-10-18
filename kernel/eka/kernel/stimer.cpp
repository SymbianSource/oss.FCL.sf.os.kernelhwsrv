// Copyright (c) 1994-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\stimer.cpp
// Note that timer handles are always thread relative
// 
//

#include <kernel/kern_priv.h>
#include "execs.h"

_LIT(KTimerThreadName,"TimerThread");
_LIT(KLitTimerMutex,"TimerMutex");

DMutex* TTickQ::Mutex;

const TInt KTimerThreadPriority=27;

const TInt KDaysFrom0ADTo2000AD=730497;		// See US_TIME.CPP to verify this
const TInt KSecondsPerDay=86400;

const TInt KMaxSkippedTicks=65536;
const TInt KMsDeltaLimit=2000000;

#define __CHECK_LAST_DELTA	if (TUint32(iLastDelta)>TUint32(KMaxSkippedTicks)) {NKern::Lock(); *(TInt*)0xfeedface=iLastDelta;}


/** Gets the address of the tick timer DFC queue.

	@return The tick timer DFC queue.
*/
EXPORT_C TDfcQue* Kern::TimerDfcQ()
	{
	return K::TimerDfcQ;
	}

TInt K::StartTickQueue()
	{
	TInt r=Kern::MutexCreate(TTickQ::Mutex, KLitTimerMutex, KMutexOrdTimer);
	if (r!=KErrNone)
		return r;
	r=Kern::DfcQCreate(K::TimerDfcQ,KTimerThreadPriority,&KTimerThreadName);
	if (r!=KErrNone)
		return r;
	DThread* pT = _LOFF(K::TimerDfcQ->iThread, DThread, iNThread);
	pT->iFlags |= KThreadFlagSystemPermanent;
	K::SecondQ=new TSecondQ;
	if (!K::SecondQ)
		return KErrNoMemory;
	K::InactivityQ=new TInactivityQ;
	if (!K::InactivityQ)
		return KErrNoMemory;
	r=TTickQ::Init();
	if (r!=KErrNone)
		return r;
	return KErrNone;
	}

void AbortTimers(TBool aAbortAbsolute)
	{
	DObjectCon& timers=*K::Containers[ETimer];
	DObjectCon& threads=*K::Containers[EThread];
	timers.Wait();
	TInt c=timers.Count();
	TInt i;
	for (i=0; i<c; i++)
		{
		DTimer* pT=(DTimer*)timers[i];
		pT->Abort(aAbortAbsolute);
		}
	timers.Signal();
	threads.Wait();
	c=threads.Count();
	for (i=0; i<c; i++)
		{
		DThread* pT=(DThread*)threads[i];
		pT->AbortTimer(aAbortAbsolute);
		}
	threads.Signal();
	}

/********************************************
 * Tick-based relative timer queue
 ********************************************/
void msCallBack(TAny* aPtr)
	{
//	__KTRACE_OPT(KTIMING,Kern::Printf("!"));
	TTickQ& q=*(TTickQ*)aPtr;
	q.iTickDfc.Add();
	}

void tickDfc(TAny* aPtr)
	{
	TTickQ& q=*(TTickQ*)aPtr;
	__ASSERT_DEBUG(q.iMsTimer.i_NTimer_iState==NTimer::EIdle, *(TInt*)0xdfc01bad=0);
	__ASSERT_DEBUG(!q.iTickDfc.Queued(), *(TInt*)0xdfc02bad=0);
	__ASSERT_DEBUG(NTickCount()-q.iMsTimer.iTriggerTime<0x80000000u, *(TInt*)0xdfc03bad=0);
	q.Tick();
	}

TTickQ::TTickQ()
	:	iLastDelta(0), iLastTicks(0), iRtc(0), iInTick(0),
		iRounding(0),
		iPrevRounding(0),
		iLastMs(0),
		iTickDfc(tickDfc,this,1),
		iMsTimer(msCallBack,this)
	{
	// Careful with the constants here or GCC will get it wrong
	K::Year2000InSeconds=Int64(KDaysFrom0ADTo2000AD)*Int64(KSecondsPerDay);
	}

#ifdef _DEBUG
void TTickQ::Check()
	{
	__ASSERT_DEBUG(iMsTimer.i_NTimer_iState==NTimer::EIdle, *(TInt*)0xdfc03bad=0);
	__ASSERT_DEBUG(!iTickDfc.Queued(), *(TInt*)0xdfc04bad=0);
	}
#endif

TInt TTickQ::Init()
	{
	TTickQ* pQ=new TTickQ;
	if (!pQ)
		return KErrNoMemory;
	K::TickQ=pQ;
	pQ->iTickDfc.SetDfcQ(K::TimerDfcQ);
	pQ->iMsTickPeriod=NTickPeriod();
	_LOFF(K::TimerDfcQ->iThread,DThread,iNThread)->iDebugMask=0x80000000;

	// These lines are tick-period dependent
	// Assume we can always set the tick period to 64Hz
	pQ->iTickPeriod=15625;
	pQ->iNominalTickPeriod=15625;
	pQ->iTicksPerSecond=64;
	K::SecondQ->iTicksPerDay=86400*64;

	TInt s2000=P::InitSystemTime();
	__KTRACE_OPT(KBOOT,Kern::Printf("s2000=%d",s2000));
	pQ->iMsTimer.iTriggerTime=NTickCount();
	pQ->iLastMs=pQ->iMsTimer.iTriggerTime;
	TUint temp = 0;
	K::SetSystemTime(s2000,0,temp,ETimeSetTime|ETimeSetAllowTimeReversal);
//	Int64 now=s2000;
//	now*=1000000;
//	now+=K::Year2000;
//	now/=pQ->iNominalTickPeriod;
//	pQ->iRtc=now;

//	TInt seconds=s2000%86400;
//	__KTRACE_OPT(KBOOT,Kern::Printf("seconds=%d",seconds));
//	now+=(86400-seconds)*pQ->iTicksPerSecond;
//	K::SecondQ->iMidnight=now;
	
	pQ->iMsTimer.i_NTimer_iUserFlags = ETrue;	// used as start flag
	TTickQ::Wait();
	K::SecondQ->Tick();		// start the second queue
	K::TickQ->Tick();		// start the tick queue
	TTickQ::Signal();

	return KErrNone;
	}

void TTickQ::Wait()
	{
	if (Mutex)
		Kern::MutexWait(*Mutex);
	}

void TTickQ::Signal()
	{
	if (Mutex)
		Kern::MutexSignal(*Mutex);
	}

void TTickQ::Tick()
	{
	Wait();
	__KTRACE_OPT(KTIMING,Kern::Printf("Tick!: l%d e%d",iLastDelta,IsEmpty()));

	// Make sure this DFC hasn't been queued again - this can happen if someone calls Add()
	// between this DFC being dispatched and executing the Wait() above; priority inheritance
	// can make this happen more often than you might expect.
	if (iTickDfc.Queued())
		iTickDfc.Cancel();

	if (K::SecondQ->iWakeUpDfc.Queued())
		{
		Signal();
		return;
		}
	iInTick=ETrue;
	Update();
	if (!IsEmpty() && CountDown(iLastDelta)) // Anything ready to be completed ?
		{
		__KTRACE_OPT(KTIMING,Kern::Printf("Tick!: f%d",FirstDelta()));
		TTickLink* pT;
		while (!IsEmpty() && (pT=(TTickLink*)RemoveFirst())!=NULL)
			{
			pT->iNext=NULL;					// In case Complete() calls TTickLink::Cancel()
			(*pT->iCallBack)(pT->iPtr);		// Dispatch the timer completion
#ifdef _DEBUG
			Check();
#endif
			if (pT->iNext==NULL && pT->iPeriod!=0)		// Periodic timer
				Add(pT,pT->iPeriod);
#ifdef _DEBUG
			Check();
#endif
			}
		}
	StartTimer();
	iInTick=EFalse;
	Signal();
	}

void TTickQ::Update()
	{
	TInt irq = __SPIN_LOCK_IRQSAVE(TheTimerQ.iTimerSpinLock);
	iLastMs=iMsTimer.iTriggerTime;
	iLastTicks+=iLastDelta;
	iRtc+=iLastDelta;
	__SPIN_UNLOCK_IRQRESTORE(TheTimerQ.iTimerSpinLock,irq);
	iPrevRounding=iRounding;
	}

void TTickQ::StartTimer()
	{
	TInt delta=KMaxTInt;
	if (!IsEmpty())
		delta=FirstDelta();
	if (delta>KMaxSkippedTicks)			// so that microseconds don't overflow
		delta=KMaxSkippedTicks;
	TInt delta_us=delta*iTickPeriod;	// number of microseconds to next timer expiry
	__KTRACE_OPT(KTIMING,Kern::Printf("Tick:delta_us=%d, prdg=%d",delta_us,iPrevRounding));
	delta_us-=iPrevRounding;			// subtract rounding error on last completed ms timer
	TInt msp=iMsTickPeriod;
	TInt delta_ms=(delta_us+(msp>>1))/msp;	// round to milliseconds
	iRounding=msp*delta_ms-delta_us;	// save rounding error on this timer
	iLastDelta=delta;					// save number of ticks
	__CHECK_LAST_DELTA;
	__ASSERT_DEBUG(!iTickDfc.Queued(), *(TInt*)0xdfc00bad=0);
	TInt r=KErrNone;
	if (iMsTimer.i_NTimer_iUserFlags)
		{
		iMsTimer.i_NTimer_iUserFlags = EFalse;
		iMsTimer.OneShot(delta_ms);		// start timer for the first time
		}
	else
		r=iMsTimer.Again(delta_ms);	// start timer
	__KTRACE_OPT(KTIMING,Kern::Printf("Tick:delta=%d, r=%d, lastdelta=%d, rdg=%d",delta_ms,r,iLastDelta,iRounding));
	if (r!=KErrNone)
		{
		__ASSERT_ALWAYS(r==KErrArgument, *(TInt*)0xbad0beef=r);
		// requested time has already passed so manually requeue the DFC
		iMsTimer.iTriggerTime+=delta_ms;
		iTickDfc.Enque();
		}
	}

// Wait on mutex before calling this
void TTickQ::Add(TTickLink* aLink, TInt aPeriod)
	{
	__KTRACE_OPT(KTIMING,Kern::Printf("Tick:Add p%d i%d ld%d",aPeriod, iInTick, iLastDelta));
	SDeltaQue::Add(aLink,aPeriod);
	if (!iInTick && aPeriod<iLastDelta)
		{
		// need to reset millisecond timer
		if (iMsTimer.Cancel())
			{
			// the timer was actually cancelled - so it hadn't expired
			iMsTimer.iTriggerTime=iLastMs;
			StartTimer();
			}
		}
	}

void TTickQ::Synchronise()
//
// Update everything as if a tick had just occurred
// Call with system unlocked and timer mutex held
//
	{
	TInt ms_delta=(TInt)(NTickCount()-iLastMs-1);
	__ASSERT_ALWAYS(ms_delta<KMsDeltaLimit,K::Fault(K::ESynchroniseMsDeltaTooBig));
	TInt msp=iMsTickPeriod;
	TInt us_delta=ms_delta*msp+iPrevRounding;
	TInt tick_delta=us_delta/iTickPeriod;
	if (tick_delta>iLastDelta)
		tick_delta=iLastDelta;
	us_delta=tick_delta*iTickPeriod-iPrevRounding;
	ms_delta=(us_delta+(msp>>1))/msp;
	iPrevRounding=msp*ms_delta-us_delta;
	iLastDelta-=tick_delta;
	TInt irq = __SPIN_LOCK_IRQSAVE(TheTimerQ.iTimerSpinLock);
	iRtc+=tick_delta;
	iLastTicks+=tick_delta;
	iLastMs+=ms_delta;
	__SPIN_UNLOCK_IRQRESTORE(TheTimerQ.iTimerSpinLock,irq);
	__CHECK_LAST_DELTA;
	if (!IsEmpty())
		{
		CountDown(tick_delta);
		__ASSERT_ALWAYS(FirstDelta()>=0, *(TInt*)0xfacefeed=FirstDelta());
		}
	}


/**
Rounds up microseconds into the number of Kernel Ticks(1/64Hz). 
@param a	if >=0, the number of microseconds;
			if < 0, ABS(a) is the number of Kernel Ticks.
@par aAdjust if true, the elapsed time from the last Kernel Tick is also taken into account.
*/
TInt MicroSecondsToTicks(TInt a, TBool aAdjust)
	{
	TUint32 p=K::TickQ->iTickPeriod;

	if (a<0)
		{
		TUint32 b=(TUint32)(-a);
		if (aAdjust)
   			b+=((NTickCount()-K::TickQ->iLastMs-1)* K::TickQ->iMsTickPeriod + p - 1)/p;
		if (b>(TUint32)KMaxTInt) b=KMaxTInt;
		return b;
		}
	if (a==0)
		a=1;
	TUint32 b=(TUint32)a;
	if (aAdjust)
		b += (TInt)(NTickCount()-K::TickQ->iLastMs-1)*K::TickQ->iMsTickPeriod;
	return (TInt)((b+p-1)/p);
	}



/**
Constructor for a tick timer.

@pre Calling thread must be in a critical section.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C TTickLink::TTickLink()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC|MASK_CRITICAL,"TTickLink::TTickLink");
	iNext=NULL;
	iLastLock=-1;
	}

/** @internalComponent */
void TTickLink::DoCancel()
	{
	iPeriod=0;
	if (iNext)
		{
		K::TickQ->Remove(this);
		iNext=NULL;
		}
	}




/**
Adds this periodic tick timer to the tick timer queue.

On expiry, the timer is put back onto the tick timer queue.

@param aPeriod   The timer interval in microseconds.
@param aCallBack The callback function that is called every time
                 this periodic timer expires.
@param aPtr      An argument that is passed to the callback function.

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C void TTickLink::Periodic(TInt aPeriod, TTickCallBack aCallBack, TAny* aPtr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TTickLink::Periodic");
	TTickQ::Wait();
	DoCancel();
	iPeriod=MicroSecondsToTicks(aPeriod, EFalse);
	iCallBack=aCallBack;
	iPtr=aPtr;
	K::TickQ->Add(this,MicroSecondsToTicks(aPeriod, ETrue));
	TTickQ::Signal();
	}




/**
Adds this one-off tick timer to the tick timer queue.

@param aTime     The timer interval in microseconds.
@param aCallBack The callback function that is called when this timer expires.
@param aPtr      An argument that is passed to the callback function.

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C void TTickLink::OneShot(TInt aTime, TTickCallBack aCallBack, TAny* aPtr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TTickLink::OneShot");
	TTickQ::Wait();
	DoCancel();
	iCallBack=aCallBack;
	iPtr=aPtr;
	K::TickQ->Add(this,MicroSecondsToTicks(aTime, ETrue));
	TTickQ::Signal();
	}




/**
Cancels this tick timer.

This timer object is removed from the tick timer queue.

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C void TTickLink::Cancel()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TTickLink::Cancel");
	TTickQ::Wait();
	DoCancel();
	TTickQ::Signal();
	}




/**
Adds this tick timer to the tick timer queue.

@param aTicks    The timer interval in ticks. This is expected to be
                 the tick count for the next lock.
@param aCallBack The callback function that is called when this timer expires.
@param aPtr      An argument that is passed to the callback function.

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C void TTickLink::Lock(TInt aTicks, TTickCallBack aCallBack, TAny* aPtr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TTickLink::Lock");
	TTickQ::Wait();
	DoCancel();
	iCallBack=aCallBack;
	iPtr=aPtr;
	K::TickQ->Add(this, aTicks);
	TTickQ::Signal();
	}




/**
	Get the tick count for the next lock.
	THIS FUNCTION ASSUMES 64 TICKS PER SECOND

	@return KErrGeneral with aTickCount = number of ticks until ETwelveOClock if the
			requested lock is more than 1 second from the last completed lock.
			KErrNone with aTickCount = number of ticks until the next aMark otherwise.
	@pre	Wait on TimerMutex before calling this.
	
	@internalComponent
*/
TInt TTickLink::GetNextLock(TTimerLockSpec aLock, TInt& aTickCount) const
	{
	__ASSERT_WITH_MESSAGE_MUTEX(TTickQ::Mutex,"Wait on TimerMutex before calling this","TTickLink::GetNextLock");
	TTickQ& tq=*K::TickQ;
	tq.Synchronise();
	TUint odd_ticks=TUint(tq.iRtc)&63;
	TUint lock_ticks=((aLock+1)*64+6)/12;
	Int64 second=tq.iRtc-odd_ticks;
	Int64 next_lock=second+lock_ticks;
	if (next_lock<=tq.iRtc)
		next_lock+=64;
	if (next_lock-iLastLock>64)
		{
		// Lock on ETwelveOClock
		aTickCount=64-odd_ticks;
		__KTRACE_OPT(KTIMING,Kern::Printf("GS%x",aTickCount));
		return KErrGeneral;
		}
	// Lock on aMark
	aTickCount=TInt(next_lock-tq.iRtc);
	__KTRACE_OPT(KTIMING,Kern::Printf("GL%x",aTickCount));
	return KErrNone;
	}

/********************************************
 * Absolute timer queue
 ********************************************/
void TSecondQ::TickCallBack(TAny* aPtr)
	{
	((TSecondQ*)aPtr)->Tick();
	}

void TSecondQ::WakeUpDfc(TAny* aPtr)
	{
	TSecondQ& sq=*(TSecondQ*)aPtr;
	TTickQ& tq=*K::TickQ;
	TTickQ::Wait();
	__ASSERT_ALWAYS(tq.iInTick,K::Fault(K::ETickQNotLocked));
	sq.iTimer.DoCancel();	// make sure second queue timer is stopped
	sq.Tick();				// call back any timers which have already expired and restart second queue
	if (!tq.IsEmpty() && tq.FirstDelta()<=0) // Anything ready to be completed now?
		tq.Tick();				// restart tick queue
	else
		{
		tq.StartTimer();		// restart tick queue
		tq.iInTick=EFalse;
		}
	TTickQ::Signal();
	}

TSecondQ::TSecondQ()
:	iInTick(0), iWakeUpDfc(WakeUpDfc,this,K::TimerDfcQ,2)
	{
	iTimer.iPtr=this;
	iTimer.iCallBack=TickCallBack;
	}

void TSecondQ::Tick()
	{
	// Called in tick timer callback, so mutex already held
	// RTC value already incremented
	Int64 rtc=K::TickQ->iRtc;
	iInTick=ETrue;
	if (rtc==iMidnight)
		{
		iMidnight+=iTicksPerDay;
		Kern::AsyncNotifyChanges(EChangesMidnightCrossover);
		}
	while (!IsEmpty())
		{
		TSecondLink* pS=(TSecondLink*)First();
		if (pS->iTime>rtc)
			break;
		if (K::PowerModel)
			K::PowerModel->AbsoluteTimerExpired();
		pS->Deque();
		pS->iNext=NULL;
		(*pS->iCallBack)(pS->iPtr);
		}
	StartTimer();
	iInTick=EFalse;
	}

void TSecondQ::StartTimer()
	{
	TSecondLink* pS=(TSecondLink*)First();
	Int64 rtc=K::TickQ->iRtc;
	TInt delta=(TInt)(iMidnight-rtc);		// ticks before midnight
	if (pS!=&iA)
		{
		Int64 delta64=pS->iTime-rtc;		// ticks before next timer
		if (delta64<delta)
			delta=(TInt)delta64;
		}
	iNextTrigger=rtc+delta;
	K::TickQ->Add(&iTimer, delta);
	}

// Wait on mutex before calling this
void TSecondQ::Add(TSecondLink* aLink)
	{
	Int64 time=aLink->iTime;
	TSecondLink* pS=(TSecondLink*)First();
	SDblQueLink* anchor=&iA;
	while(pS!=anchor && pS->iTime<=time)
		pS=(TSecondLink*)pS->iNext;
	aLink->InsertBefore(pS);
	if (!iInTick && time<iNextTrigger)
		{
		iTimer.DoCancel();
		StartTimer();
		}
	}

TInt TSecondQ::FirstDelta()
	{
	if (IsEmpty())
		return KMaxTInt;
	TSecondLink* pS=(TSecondLink*)First();
	TTimeK first=pS->iTime*K::TickQ->iNominalTickPeriod;
	TTimeK now=Kern::SystemTime();
	Int64 delta=first-now;
	delta/=1000000;
	if (delta>KMaxTInt)
		return KMaxTInt;
	return (TInt)delta;
	}

TTimeK TSecondQ::WakeupTime()
//
// Called machine goes to standby
// Timer mutex is already held
//
	{
	if (IsEmpty())
		return 0;
	TSecondLink* pS=(TSecondLink*)First();
	return pS->iTime*K::TickQ->iNominalTickPeriod;
	}

void TSecondQ::WakeUp()
//
// Called after machine wakes up
// Timer mutex is already held
//
	{
    TInt secs;
	AbortTimers(EFalse);							// abort any locked timers
	TInt r=A::SystemTimeInSecondsFrom2000(secs);	// get hardware RTC value
	TUint changes = 0;
	if (r==KErrNone)
		{
		// Apply nonsecure offset if secure clock in use
		if (K::SecureClockStatus == ESecureClockOk)		
			{
			secs += K::NonSecureOffsetSeconds;
			}
		K::TickQ->Synchronise();
		r=K::SetSystemTime(secs,0,changes,ETimeSetTime);		// update K::SecondQ->iRtc, don't allow it to go backwards
		__KTRACE_OPT(KPOWER,Kern::Printf("new time=%d, r=%d",secs,r));
		}
	K::InactivityQ->Reset();
	if (r<0)
		return;									// time has not changed

	if (!K::TickQ->iInTick)						// if iInTick is already set, wake up DFC is already queued
		{
		K::TickQ->iInTick=ETrue;				// stop anyone else restarting timer queues
		iWakeUpDfc.Enque();						// this will restart the timer queues
		}
	if (changes & EChangesMidnightCrossover)
		Kern::AsyncNotifyChanges(EChangesMidnightCrossover);
	}




/**
Constructor for a second timer.

@pre Calling thread must be in a critical section.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C TSecondLink::TSecondLink()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC|MASK_CRITICAL,"TSecondLink::TSecondLink");
	iNext=NULL;
	}




/**
Adds this timer to the second timer queue.

@param aUTCTime The absolute date and time when the timer is to expire, in UTC.
@param aCallBack The callback function that is called when this timer expires.
@param aPtr      An argument that is passed to the callback function.

@return KErrNone, if successful;
        KErrUnderflow, if the specified time is earlier than the system time.
        KErrOverflow, if the specified time is too big.

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C TInt TSecondLink::At(const TTimeK& aUTCTime, TSecondCallBack aCallBack, TAny* aPtr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TSecondLink::At");
	TTickQ::Wait();
	iTime=aUTCTime;
	iCallBack=aCallBack;
	iPtr=aPtr;
	if (iTime<=Kern::SystemTime())
		{
		TTickQ::Signal();
		return KErrUnderflow;
		}
	iTime=(iTime+999999)/1000000;		// seconds from 00:00:00 01-01-0000 UTC, rounded up
	Int64 y2k=Int64(KDaysFrom0ADTo2000AD)*Int64(KSecondsPerDay);
	Int64 delta2k=iTime-y2k;
	if (delta2k>KMaxTInt)
		{
		TTickQ::Signal();
		return KErrOverflow;
		}
	iTime*=K::TickQ->iTicksPerSecond;
	K::SecondQ->Add(this);
	TTickQ::Signal();
	return KErrNone;
	}




/**
Cancels this timer.

This timer object is removed from the second timer queue.

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C void TSecondLink::Cancel()
//
// Cancel a pending timer.
//
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TSecondLink::Cancel");
	TTickQ::Wait();
	if (iNext)
		{
		Deque();
		iNext=NULL;
		}
	TTickQ::Signal();
	}

/********************************************
 * Inactivity timer queue
 ********************************************/
/**
Constructor for an inactivity timer.

@pre Calling thread must be in a critical section.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C TInactivityLink::TInactivityLink()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC|MASK_CRITICAL,"TInactivityLink::TInactivityLink");
	iNext=NULL;
	}




/**
Adds this inactivity timer to the inactivity timer queue.

@param aSeconds  The period of inactivity needed to trigger this timer, in seconds.
@param aCallBack The callback function that is called when this timer expires.
@param aPtr      An argument that is passed to the callback function.

@return KErrNone, if successful;
        KErrArgument, if aSeconds is negative;
        KErrOverflow, if the number of ticks corresponding to aSeconds
        overflows a 32 bit signed integer.

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C TInt TInactivityLink::Start(TInt aSeconds, TInactivityCallBack aCallBack, TAny* aPtr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TInactivityLink::Start");
	// Time period can't be more than 2^31 ticks (just over 1 year at 64Hz)
	TTickQ& tq=*K::TickQ;
	TInactivityQ& iq=*K::InactivityQ;
	if (aSeconds<0)
		return KErrArgument;
	Int64 ticks(aSeconds);
	ticks*=Int64(tq.iTicksPerSecond);
	if (ticks>KMaxTInt)
		return KErrOverflow;
	TTickQ::Wait();
	if (iNext)
		{
		Deque();
		iNext=NULL;
		}
	iTime=(TUint32)ticks;
	iPtr=aPtr;
	iCallBack=aCallBack;
	TUint32 tc=Kern::TickCount();
	NKern::LockSystem();
	TUint32 lev = iq.iLastEventTime;
	SDblQue* pQ=&iq;
	if (tc-lev>=iTime)
		{
		pQ=&iq.iPending;
		if (pQ->IsEmpty())
			{
			// adding to empty pending queue, do with system lock held
			// to prevent race with AddEvent
			pQ->Add(this);
			pQ=NULL;
			}
		}
	NKern::UnlockSystem();
	if (pQ)
		{
		TInactivityLink* pL=(TInactivityLink*)pQ->First();
		while (pL!=&pQ->iA && pL->iTime<=iTime)
			pL=(TInactivityLink*)pL->iNext;
		InsertBefore(pL);
		if (pQ==&iq && iq.First()==this && !iq.iInTick)
			{
			// need to restart tick timer
			iq.iTimer.DoCancel();
			tq.Add(&iq.iTimer, iTime+lev-tq.iLastTicks);
			}
		}
	TTickQ::Signal();
	return KErrNone;
	}




/**
Cancels this inactivity timer.

This timer object is removed from the inactivity timer queue.

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C void TInactivityLink::Cancel()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TInactivityLink::Cancel");
	TTickQ::Wait();
	if (iNext)
		{
		NKern::LockSystem();
		Deque();
		NKern::UnlockSystem();
		iNext=NULL;
		}
	TTickQ::Signal();
	}

void TInactivityQ::TimerCallBack(TAny* aPtr)
	{
	((TInactivityQ*)aPtr)->Expired(ETrue);
	}

void TInactivityQ::EventDfcFn(TAny* aPtr)
	{
	((TInactivityQ*)aPtr)->EventDfc();
	}

TInactivityQ::TInactivityQ()
	:	iLastEventTime(0),
		iInTick(EFalse),
		iEventDfc(EventDfcFn,this,K::TimerDfcQ,1)
	{
	iTimer.iPtr=this;
	iTimer.iCallBack=TimerCallBack;
	}

void TInactivityQ::Reset()
	{
	TUint32 tc=Kern::TickCount();
	NKern::LockSystem();
	iLastEventTime=tc;
	if (!iPending.IsEmpty())
		iEventDfc.Enque(SYSTEM_LOCK);
	else
		NKern::UnlockSystem();
	}

TInt TInactivityQ::InactiveTime()
	{
	TUint32 lev=iLastEventTime;
	TUint32 tc=Kern::TickCount();
	tc-=lev;
	if (tc>0x80000000u)
		tc=0x80000000u;
	tc/=K::TickQ->iTicksPerSecond;
	return (TInt)tc;
	}

void TInactivityQ::Expired(TBool aTicksUpdated)
	{
	// called in tick timer call back, so timer mutex held
	TTickQ& tq=*K::TickQ;
	TUint32 tc;
	if (aTicksUpdated)
		tc=tq.iLastTicks-iLastEventTime;
	else
		tc=Kern::TickCount()-iLastEventTime;
	iInTick=ETrue;
	while (!IsEmpty())
		{
		TInactivityLink* pI=(TInactivityLink*)First();
		if (tc<pI->iTime)
			{
			tq.Add(&iTimer, pI->iTime-tc);
			break;
			}
		pI->Deque();
		pI->iNext=NULL;
		(*pI->iCallBack)(pI->iPtr);
		}
	iInTick=EFalse;
	}

void TInactivityQ::EventDfc()
	{
	// called after an event to transfer pending timers to active queue
	TTickQ::Wait();

	SDblQueLink* anchor=&iA;
	TInactivityLink* pActL=(TInactivityLink*)iA.iNext;	// first active timer
	TInactivityLink* pOldFirstActive=pActL;

	while (!iPending.IsEmpty())
		{
		TInactivityLink* pPendL=(TInactivityLink*)iPending.First();
		pPendL->Deque();
		TUint32 pending_time=pPendL->iTime;
		while (pActL!=anchor && pending_time>=pActL->iTime)
			pActL=(TInactivityLink*)pActL->iNext;	// loop until pActL expires after pPendL
		pPendL->InsertBefore(pActL);				// add pending one before first later one
		// leave pActL where it is - OK since pending Q is ordered
		}

	// prevent double calling of this DFC
	// pending queue can only become nonempty by adding a TInactivityLink
	// which would require TTickQ::Wait() to be called
	iEventDfc.Cancel();

	pActL=(TInactivityLink*)iA.iNext;	// first active timer
	if (pActL!=pOldFirstActive)
		{
		// need to restart tick timer
		iTimer.DoCancel();
		Expired(EFalse);
		}

	TTickQ::Signal();
	}

/********************************************
 * DTimer class
 ********************************************/
DTimer::DTimer()
	{}

DTimer::~DTimer()
	{
	// cancel timer before destroying
	Cancel();
	}

TInt DTimer::Create(DThread *aThread)
//
// Create a Timer. Always owned by a thread.
//
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DTimer::Create thread %O",aThread));
	SetOwner(aThread);
	return iTimer.Create();
	}

TInt DTimer::RequestUserHandle(DThread* aThread, TOwnerType aType)
	{
	if (aThread!=Owner() || aType!=EOwnerThread)
		return KErrPermissionDenied;
	return KErrNone;
	}

void DTimer::TimerComplete(TAny* aPtr)
//
// Called when the relative timer completes.
//
	{
	DTimer* pT=FromPtr(aPtr);
	if (pT->iTimer.iState == TTimer::EWaiting)
		{
		Kern::QueueRequestComplete(pT->Owner(),pT->iTimer.iRequest,KErrNone);
		pT->iTimer.iState = (TUint8)TTimer::EIdle;
		}
	}

void DTimer::SecondComplete(TAny* aPtr)
//
// Called when the absolute timer completes.
//
	{
	DTimer* pT=FromPtr(aPtr);
	if (pT->iTimer.iState == TTimer::EWaiting)
		{
		Kern::QueueRequestComplete(pT->Owner(),pT->iTimer.iRequest,KErrNone);
		pT->iTimer.iState = (TUint8)TTimer::EIdle;
		}
	}

void DTimer::LockComplete(TAny* aPtr)
//
// Called when the locked timer completes.
//
	{
	DTimer* pT=(DTimer*)aPtr;
	if (pT->iTimer.iState == TTimer::EWaiting)
		{
		pT->iTimer.TickLink().iLastLock=K::TickQ->iRtc;
		__KTRACE_OPT(KTIMING,Kern::Printf("LC%lx",pT->iTimer.TickLink().iLastLock));
		Kern::QueueRequestComplete(pT->Owner(),pT->iTimer.iRequest,KErrNone);
		pT->iTimer.iState = (TUint8)TTimer::EIdle;
		}
	}

void DTimer::LockSynchronize(TAny* aPtr)
//
// Called when the locked timer synchronises
//
	{
	DTimer* pT=(DTimer*)aPtr;
	if (pT->iTimer.iState == TTimer::EWaiting)
		{
		pT->iTimer.TickLink().iLastLock=K::TickQ->iRtc;
		__KTRACE_OPT(KTIMING,Kern::Printf("LS%lx",pT->iTimer.TickLink().iLastLock));
		Kern::QueueRequestComplete(pT->Owner(),pT->iTimer.iRequest,KErrGeneral);
		pT->iTimer.iState = (TUint8)TTimer::EIdle;
		}
	}

void DTimer::MsComplete(TAny* aTimer)
	{
	// called in DFC, system unlocked
	DTimer* pT=FromPtr(aTimer);
	NKern::LockSystem();
	pT->iTimer.iState = (TUint8)TTimer::EIdle;
	Kern::QueueRequestComplete(pT->Owner(), pT->iTimer.iRequest, KErrNone);
	NKern::UnlockSystem();
	}

void DTimer::HighRes(TRequestStatus& aStatus, TInt aInterval)
	{
	// enter and return with system locked
	TInt r=iTimer.AfterHighRes(aInterval,MsComplete,aStatus);
	if (r!=KErrNone)
		K::PanicCurrentThread(ETimerAlreadyPending);
	}

void DTimer::AgainHighRes(TRequestStatus& aStatus, TInt aInterval)
	{
	// enter and return with system locked
	TInt r=iTimer.AgainHighRes(aInterval,MsComplete,aStatus);
	if (r==KErrInUse)
		K::PanicCurrentThread(ETimerAlreadyPending);
	else if (r!=KErrNone)
		{
		TRequestStatus* status = &aStatus;
		Kern::RequestComplete(status, r);
		}
	}

void DTimer::Cancel()
//
// Cancel an outstanding request.
// system unlocked here
//
	{
	iTimer.Cancel(Owner());
	}

void DTimer::Abort(TBool aAbortAbsolute)
//
// Abort an outstanding request.
// TimerMutex is already held here.
//
	{
	TInt typeMask=aAbortAbsolute?(TTimer::ELocked|TTimer::EAbsolute):(TTimer::ELocked);
	iTimer.Abort(Owner(),typeMask);
	}

TInt DTimer::After(TRequestStatus& aStatus, TInt anInterval)
//
// Request a relative time.
// System is unlocked here
//
	{
	return iTimer.After(anInterval,TimerComplete,aStatus);
	}

TInt DTimer::At(TRequestStatus& aStatus, const TTimeK& aTime)
//
// Request an absolute time.
//
	{
	return iTimer.At(aTime,SecondComplete,aStatus);
	}

TInt DTimer::Inactivity(TRequestStatus& aStatus, TInt aSeconds)
//
// Request an inactivity time.
//
	{
	return iTimer.Inactivity(aSeconds,TimerComplete,aStatus);
	}

TInt DTimer::Lock(TRequestStatus& aStatus, TTimerLockSpec aLock)
//
// Request a synchronisation lock to a fraction of a second.
//
	{
	TTickQ::Wait();
	NKern::LockSystem();
	if (iTimer.iState != TTimer::EIdle || iTimer.iRequest->SetStatus(&aStatus) != KErrNone)
		{
		NKern::UnlockSystem();
		TTickQ::Signal();
		return KErrInUse;
		}
	iTimer.iState = (TUint8)TTimer::EWaiting;
	if (iTimer.Type()!=TTimer::ELocked)
		iTimer.SetType(TTimer::ELocked);
	NKern::UnlockSystem();
	TInt ticks;
	TInt r=iTimer.TickLink().GetNextLock(aLock, ticks);
	if (r==KErrNone)
		iTimer.TickLink().Lock(ticks,LockComplete,this);
	else
		iTimer.TickLink().Lock(ticks,LockSynchronize,this);
	TTickQ::Signal();
	return KErrNone;
	}


/********************************************
 * Multipurpose timer
 ********************************************/

TInt TTimer::Create()
	{
	return Kern::CreateClientRequest(iRequest);
	}

TTimer::~TTimer()
	{
	Kern::DestroyClientRequest(iRequest);
	}

void TTimer::SetType(TTimerType aType)
	{
	switch (aType)
		{
		case ERelative:
		case ELocked:
			new (&TickLink()) TTickLink;
			break;
		case EAbsolute:
			new (&SecondLink()) TSecondLink;
			break;
		case EHighRes:
			break;
		case EInactivity:
			new (&Inact()) TInactivityLink;
			break;
		default:
			K::Fault(K::EBadTimerType);
		}
	iType=(TUint8)aType;
	}

TInt TTimer::AfterHighRes(TInt aInterval, NTimerFn aFunction, TRequestStatus& aStatus)
	{
	// enter and return with system locked
	if (iState!=EIdle || iRequest->SetStatus(&aStatus) != KErrNone)
		return KErrInUse;
	iState = (TUint8)EWaitHighRes;
	SetType(EHighRes);
	TInt msp=NTickPeriod();
	TInt t=(TInt)(((TUint)aInterval+msp-1)/msp);	// convert microseconds to milliseconds, rounding up
	new (&Ms()) NTimer(aFunction,this);
	Ms().OneShot(t,ETrue);			// start millisecond timer, complete in DFC
	return KErrNone;
	}

TInt TTimer::AgainHighRes(TInt aInterval, NTimerFn aFunction, TRequestStatus& aStatus)
	{
	// enter and return with system locked
	if (iState!=EIdle)
		return KErrInUse;
	if (iType!=EHighRes)
		return AfterHighRes(aInterval, aFunction, aStatus);
	if (iRequest->SetStatus(&aStatus) != KErrNone)
		return KErrInUse;
	iState = (TUint8)EWaitHighRes;
	TInt msp=NTickPeriod();
	TInt t;
	if (aInterval>=0)
		t = (TInt)(((TUint)aInterval+msp-1)/msp);	// convert microseconds to milliseconds, rounding up
	else
		t = aInterval/msp;		// convert microseconds to milliseconds, rounding up
	TInt r = Ms().Again(t);		// start millisecond timer, complete in DFC
	if (r != KErrNone)
		{
		iState=(TUint8)EIdle;
		iRequest->Reset();
		}
	return r;
	}

void TTimer::Cancel(DThread* aThread)
//
// Cancel an outstanding request.
// Enter and return with system unlocked
//
	{
	NKern::LockSystem();
	TTimerState s = (TTimerState)iState;
	if (s==EWaitHighRes)
		{
		Ms().Cancel();
		iState=(TUint8)EIdle;
		s=EIdle;
		if (aThread)
			Kern::QueueRequestComplete(aThread, iRequest,KErrCancel);
		NKern::UnlockSystem();
		}
	else
		NKern::UnlockSystem();
	if (s==EIdle)
		return;
	TTickQ::Wait();
	if (iState==EWaiting)
		{
		if (iType==EAbsolute)
			SecondLink().Cancel();
		else if (iType==EInactivity)
			Inact().Cancel();
		else
			TickLink().Cancel();
		if (aThread)
			Kern::QueueRequestComplete(aThread,iRequest,KErrCancel);
		iState=(TUint8)EIdle;
		}
	TTickQ::Signal();
	}

TInt TTimer::After(TInt anInterval, TTickCallBack aFunction, TRequestStatus& aStatus)
//
// Request a relative time.
// Enter and return with system unlocked
//
	{
	TTickQ::Wait();
	NKern::LockSystem();
	if (iState!=EIdle || iRequest->SetStatus(&aStatus) != KErrNone)
		{
		NKern::UnlockSystem();
		TTickQ::Signal();
		return KErrInUse;
		}
	iState=(TUint8)EWaiting;
	SetType(ERelative);
	NKern::UnlockSystem();
	TickLink().OneShot(anInterval,aFunction,this);
	TTickQ::Signal();
	return KErrNone;
	}

TInt TTimer::At(const TTimeK& aTime, TSecondCallBack aFunction, TRequestStatus& aStatus)
//
// Request an absolute time.
// Enter and return with system unlocked
//
	{
	TTickQ::Wait();
	NKern::LockSystem();
	if (iState!=EIdle || iRequest->SetStatus(&aStatus) != KErrNone)
		{
		NKern::UnlockSystem();
		TTickQ::Signal();
		return KErrInUse;
		}
	iState=(TUint8)EWaiting;
	SetType(EAbsolute);
	NKern::UnlockSystem();
	TInt r=SecondLink().At(aTime,aFunction,this);
	if (r!=KErrNone)
		{
		iState=(TUint8)EIdle;
		iRequest->Reset();
		}
	TTickQ::Signal();
	return r;
	}

TInt TTimer::Inactivity(TInt aSeconds, TInactivityCallBack aFunction, TRequestStatus& aStatus)
//
// Request an inactivity time.
// Enter and return with system unlocked
//
	{
	TTickQ::Wait();
	NKern::LockSystem();
	if (iState!=EIdle || iRequest->SetStatus(&aStatus) != KErrNone)
		{
		NKern::UnlockSystem();
		TTickQ::Signal();
		return KErrInUse;
		}
	iState=(TUint8)EWaiting;
	SetType(EInactivity);
	NKern::UnlockSystem();
	TInt r=Inact().Start(aSeconds,aFunction,this);
	if (r!=KErrNone)
		{
		iState=(TUint8)EIdle;
		iRequest->Reset();
		}
	TTickQ::Signal();
	return r;
	}

void TTimer::Abort(DThread* aThread, TInt aTypeMask)
//
// Abort an outstanding request.
// TimerMutex is already held here.
//
	{
	NKern::LockSystem();
	if (iState!=EWaiting || (iType&aTypeMask)==0)
		{
		NKern::UnlockSystem();
		return;
		}
	NKern::UnlockSystem();
	if (iType==EAbsolute)
		SecondLink().Cancel();
	else
		TickLink().Cancel();
	if (iType==ELocked)
		TickLink().iLastLock=-1;
	if (aThread)
		Kern::QueueRequestComplete(aThread,iRequest,KErrAbort);
	iState=(TUint8)EIdle;
	}


/********************************************
 * Timer utilities
 ********************************************/
TInt ExecHandler::TimerCreate()
//
// Create a Timer.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::TimerCreate"));
	NKern::ThreadEnterCS();
	DTimer *pT=new DTimer;
	TInt r=KErrNoMemory;
	if (pT)
		{
		r=pT->Create(TheCurrentThread);
		if (r==KErrNone)
			r=K::AddObject(pT,ETimer);
		if (r==KErrNone)
			r=K::MakeHandle(EOwnerThread,pT);
		}
	if (pT && r<KErrNone)
		pT->Close(NULL);
	NKern::ThreadLeaveCS();
	return r;
	}

void ExecHandler::TimerCancel(DTimer* aTimer)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::TimerCancel"));
	K::ThreadEnterCS();
	aTimer->Cancel();			// OK since timer handles always thread relative
	K::ThreadLeaveCS();
	}

void ExecHandler::TimerAfter(DTimer* aTimer, TRequestStatus& aStatus, TInt aTime)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::TimerAfter"));
	K::ThreadEnterCS();
	TInt r=aTimer->After(aStatus,aTime);	// OK since timer handles always thread relative
	K::ThreadLeaveCS();
	if (r!=KErrNone)
		K::PanicCurrentThread(ETimerAlreadyPending);
	}

void ExecHandler::TimerHighRes(DTimer* aTimer, TRequestStatus& aStatus, TInt aTime)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::TimerHighRes"));
	aTimer->HighRes(aStatus,aTime);
	}

void ExecHandler::TimerAgainHighRes(DTimer* aTimer, TRequestStatus& aStatus, TInt aTime)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::TimerAgainHighRes"));
	aTimer->AgainHighRes(aStatus,aTime);
	}

void ExecHandler::TimerAt(DTimer* aTimer, TRequestStatus& aStatus, TUint32 aTimeLo, TUint32 aTimeHi)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::TimerAt"));
	TTimeK time = MAKE_TINT64(aTimeHi,aTimeLo);
	K::ThreadEnterCS();
	TInt r=aTimer->At(aStatus,time);		// OK since timer handles always thread relative
	K::ThreadLeaveCS();
	if (r==KErrInUse)
		K::PanicCurrentThread(ETimerAlreadyPending);
	else if (r!=KErrNone)
		{
		TRequestStatus* status = &aStatus;
		Kern::RequestComplete(status, r);
		}
	}

void ExecHandler::TimerLock(DTimer* aTimer, TRequestStatus& aStatus, TTimerLockSpec aLock)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::TimerLock"));
	K::ThreadEnterCS();
	TInt r=aTimer->Lock(aStatus,aLock);		// OK since timer handles always thread relative
	K::ThreadLeaveCS();
	if (r!=KErrNone)
		K::PanicCurrentThread(ETimerAlreadyPending);
	}

void ExecHandler::TimerInactivity(DTimer* aTimer, TRequestStatus& aStatus, TInt aSeconds)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::TimerInactivity"));
	K::ThreadEnterCS();
	TInt r=aTimer->Inactivity(aStatus,aSeconds);	// OK since timer handles always thread relative
	K::ThreadLeaveCS();
	if (r==KErrInUse)
		K::PanicCurrentThread(ETimerAlreadyPending);
	else if (r!=KErrNone)
		{
		TRequestStatus* status = &aStatus;
		Kern::RequestComplete(status, r);
		}
	}

/********************************************
 * Miscellaneous
 ********************************************/
TInt ExecHandler::UserInactivityTime()
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::UserInactivityTime"));
	return K::InactivityQ->InactiveTime();
	}

void ExecHandler::ResetInactivityTime()
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ResetInactivityTime"));
	NKern::ThreadEnterCS();
	K::InactivityQ->Reset();
	NKern::ThreadLeaveCS();
	}

EXPORT_C TUint32 Kern::TickCount()
/**
	Returns the number of system ticks since boot.
*/
	{
	TTickQ& q=*K::TickQ;
	TInt irq = __SPIN_LOCK_IRQSAVE(TheTimerQ.iTimerSpinLock);
	TUint32 last_ticks=q.iLastTicks;
	TUint32 lastms=q.iLastMs;
	TUint32 ms=NTickCount();
	TUint32 p=(TUint32)q.iTickPeriod;
	__SPIN_UNLOCK_IRQRESTORE(TheTimerQ.iTimerSpinLock,irq);
	return last_ticks+((ms-lastms-1)*q.iMsTickPeriod)/p;
	}

TInt K::SecondsFrom2000(const TTimeK& aTime, TInt& aSeconds)
	{
	TInt64 diff=aTime/1000000; // convert microseconds to seconds
	diff-=K::Year2000InSeconds;
	if (diff>KMaxTInt || diff<KMinTInt)
		return KErrOverflow;
	aSeconds=(TInt)diff;
	return KErrNone;
	}

EXPORT_C TTimeK Kern::SystemTimeSecure()
/**
	Gets the current system time.

	@return The current time as the number of microseconds since midnight, January 1st, year 0 (nominal Gregorian).
*/
	{
	TTimeK r = Kern::SystemTime();
	if (K::SecureClockStatus==ESecureClockOk)
		{
		/*
		NB: We read the software clock and unapply the nonsecure offset, rather than
		reading the hardware rtc directly, for two reasons:
			1. The software clock ticks at 64Hz, whereas the variant API 
			   for reading the rtc is a measly 1Hz.
			2. Less code.
		*/
		TInt64 nonsecure_offset=K::NonSecureOffsetSeconds;
		nonsecure_offset *= 1000000;
		r -= nonsecure_offset;
		}
	else 
		{
		// Driver/variant code mustn't try to read the secure system time before the HAL has 
		// loaded (i.e. the nonsecure offset has been read from permanent storage). If you hit the 
		// assert below you need to change your code to use the nonsecure time API.
		__ASSERT_DEBUG(K::SecureClockStatus!=ESecureClockPresent, Kern::Printf("Secure clock read but no nonsecure offset is present"));
		}
	return r;
	}

EXPORT_C TTimeK Kern::SystemTime()
/**
	Gets the current nonsecure (user-settable) system time.

	@return The current time as the number of microseconds since midnight, January 1st, year 0 (nominal Gregorian).
*/
	{
	TTickQ& q=*K::TickQ;
	TInt irq = __SPIN_LOCK_IRQSAVE(TheTimerQ.iTimerSpinLock);
	Int64 rtc = q.iRtc;
	TUint32 lastms=q.iLastMs;
	TUint32 ms=NTickCount();
	__SPIN_UNLOCK_IRQRESTORE(TheTimerQ.iTimerSpinLock,irq);
	return rtc*q.iNominalTickPeriod+TInt(ms-lastms-1)*q.iMsTickPeriod;
	}


static TInt ExecTimeNowImpl(TTimeK& aUniversalTime, TInt& aUniversalTimeOffset, TBool aSecure)
{
	if (aSecure && K::SecureClockStatus != ESecureClockOk) 
		return KErrNoSecureTime;

	TInt off=K::HomeTimeOffsetSeconds;
	TTimeK time = aSecure ? Kern::SystemTimeSecure() : Kern::SystemTime();
	kumemput32(&aUniversalTime,&time,sizeof(time));
	kumemput32(&aUniversalTimeOffset,&off,sizeof(off));
	return KErrNone;
}

TInt ExecHandler::TimeNowSecure(TTimeK& aUniversalTime, TInt& aUniversalTimeOffset)
//
// Get the secure system time and universal time offset.
//
    {
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::TimeNowSecure"));
	return ExecTimeNowImpl(aUniversalTime, aUniversalTimeOffset, ETrue);
	}

TInt ExecHandler::TimeNow(TTimeK& aUniversalTime, TInt& aUniversalTimeOffset)
//
// Get the nonsecure system time and universal time offset.
//
    {
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::TimeNow"));
	return ExecTimeNowImpl(aUniversalTime, aUniversalTimeOffset, EFalse);
	}

TInt ExecHandler::UTCOffset()
//
// Get the universal time offset in seconds.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::UTCOffset"));
	return K::HomeTimeOffsetSeconds;
	}

// Call with system unlocked and timer mutex held
TInt K::SetSystemTime(TInt aSecondsFrom2000, TInt aUTCOffset, TUint& aChanges, TUint aMode)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("K::SetSystemTime %d %d %d",aSecondsFrom2000,aUTCOffset,aMode));

	TTickQ& tq=*K::TickQ;
	TSecondQ& sq=*(TSecondQ*)K::SecondQ;

	// get current midnight time and UTC offset
	Int64 mnt = sq.iMidnight;
	TInt old_offset = K::HomeTimeOffsetSeconds;

	// set up new offset and tweak midnight for offset change
	if (aMode & ETimeSetOffset)
		{
		aUTCOffset %= 86400;	// stop silly offsets crashing the system
		mnt += (old_offset - aUTCOffset) * tq.iTicksPerSecond;
		}
	else
		aUTCOffset = old_offset;

	// set up new time and check for time reversal
	Int64 c;
	if (aMode & ETimeSetTime)
		{
		c=aSecondsFrom2000;
		c+=K::Year2000InSeconds;
		c*=tq.iTicksPerSecond;
		if (c<tq.iRtc && (aMode & ETimeSetAllowTimeReversal) == 0)
			return KErrArgument;
		}
	else
		c = tq.iRtc;

	// check if we've crossed midnight	
	if (c>=mnt || c<mnt-sq.iTicksPerDay)
		aChanges |= EChangesMidnightCrossover;
		
	// work out local time and round to the next midnight, then back to utc
	mnt=(c + aUTCOffset*tq.iTicksPerSecond + sq.iTicksPerDay) / sq.iTicksPerDay * sq.iTicksPerDay - aUTCOffset*tq.iTicksPerSecond;
	
	// update time, next midnight, and offset atomically
	TInt irq = __SPIN_LOCK_IRQSAVE(TheTimerQ.iTimerSpinLock);
	tq.iRtc=c;
	sq.iMidnight=mnt;
	K::HomeTimeOffsetSeconds = aUTCOffset;
	__SPIN_UNLOCK_IRQRESTORE(TheTimerQ.iTimerSpinLock,irq);
	
	// Cancel timers and note time change
	if (aMode & ETimeSetTime)
		{
		tq.iMsTimer.Cancel();
		tq.iMsTimer.iTriggerTime=tq.iLastMs;
		tq.iTickDfc.Cancel();
		}
	if ((aMode & ETimeSetTime) || (old_offset != aUTCOffset && (aMode & ETimeSetNoTimeUpdate) == 0))
		{
		aChanges |= EChangesSystemTime;
		sq.iTimer.DoCancel();
		}
	
	return KErrNone;
	}


TInt ExecHandler::SetUTCTimeAndOffset(const TTimeK& aHomeTime, TInt aOffsetInSeconds, TUint aMode, TUint aChanges)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SetUTCTimeAndOffset"));

	if(aChanges != EChangesLocale && !Kern::CurrentThreadHasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by User time set function")))
		K::UnlockedPlatformSecurityPanic();
	
	// If caller wants to set the secure (hardware) clock then check the process has permission to do that. Also
	// check that the nonsecure offset HAL attribute is present, or a secure time cannot be set.
	TUint flags = Kern::ETimeSet_SetHwRtc; // all user-mode clock setting APIs need to use this flag. It's only optional for drivers calling Kern::SetSystemTime
	if (aMode & ETimeSetSecure) 
		{
		if(!Kern::CurrentThreadHasCapability(ECapabilityTCB,__PLATSEC_DIAGNOSTIC_STRING("Checked by User time set function")))
			return KErrPermissionDenied;
		if (0 == (K::SecureClockStatus & ESecureClockOffsetPresent))
			return KErrNoSecureTime;
		aMode &= ~ETimeSetSecure;
		flags |= Kern::ETimeSet_Secure;
		}


	TTimeK hometime;
  	kumemget32(&hometime, &aHomeTime, sizeof(hometime));

	if (aMode & ETimeSetOffset)
		{
		SLocaleData& ld = TheMachineConfig().iLocale;
		NKern::LockSystem();
		ld.iUniversalTimeOffset = aOffsetInSeconds;
		ld.iDaylightSaving = EDstNone;
		NKern::ThreadEnterCS();
		NKern::UnlockSystem();
		}
	else
		NKern::ThreadEnterCS();
	TUint changes = 0;
	TInt r = K::SetSystemTimeAndOffset(hometime, aOffsetInSeconds, flags, changes, aMode);
	if (changes & EChangesSystemTime)
		{
		if (aMode & ETimeSetOffset || aChanges & EChangesLocale)
			changes |= EChangesLocale;
		__KTRACE_OPT(KEXEC,Kern::Printf("changes=%x",changes));
		Kern::NotifyChanges(changes);
		}
	else if(aChanges & EChangesLocale)
			Kern::NotifyChanges(EChangesLocale);
	NKern::ThreadLeaveCS();
	return r;
	}

/**
Updates the software RTC and possibly the hardware RTC.

@param aTime The new time to set, either universal or local
@param aMode	Flags as follows (from enum Kern::TTimeSetMode):
				ETimeSet_SetHwRtc - set HW as well as SW RTC
				ETimeSet_LocalTime - aTime is local time rather than UTC
				ETimeSet_SyncNotify - synchronously trigger change notifiers
				ETimeSet_AsyncNotify - asynchronously trigger change notifiers
				ETimeSet_Secure	- set the secure clock

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@return KErrNone, if successful;
        KErrOverflow, if aTime it outside the representable range, which is
		-2^31 to +2^31-1 seconds relative to 00:00:00 01-01-2000
*/
EXPORT_C TInt Kern::SetSystemTime(const TTimeK& aTime, TUint aMode)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::SetSystemTime");			
	TUint dummy = 0;
	return K::SetSystemTimeAndOffset(aTime, 0, aMode, dummy, ETimeSetTime);
	}


	
TInt K::SetSystemTimeAndOffset(const TTimeK& aTime, TInt aOffset, TUint aTimeSetMode, TUint& aChanges, TUint aMode)
	{
	TSecondQ& sq = *K::SecondQ;
	TTickQ& tq = *K::TickQ;
	TTickQ::Wait();
	tq.Synchronise();
	TInt s = 0;
	TInt old = 0;

	if (aMode & ETimeSetTime)
		{

		// Convert the new time parameter to UTC seconds
		Int64 oldrtc = tq.iRtc;
		TTimeK newTime = aTime;
		if ((aTimeSetMode & Kern::ETimeSet_LocalTime) || (aMode & ETimeSetLocalTime))
			{
			if (aMode & ETimeSetOffset)
				newTime -= TTimeK(aOffset)*1000000;
			else
				newTime -= TTimeK(K::HomeTimeOffsetSeconds)*1000000;
			}
		K::SecondsFrom2000(oldrtc*tq.iNominalTickPeriod, old);
	    TInt r = K::SecondsFrom2000(newTime, s);
		if (r!=KErrNone)
			{
			TTickQ::Signal();
			return r;
			}


		// Update secure clock
		if (aTimeSetMode & Kern::ETimeSet_Secure)
			{
			if (aTimeSetMode & Kern::ETimeSet_SetHwRtc) 
				{
				r = A::SetSystemTimeInSecondsFrom2000(s);		// Update hardware RTC whether or not secure clock OK (HAL setting present)
				if (r!=KErrNone)
					{
					TTickQ::Signal();
					return r;
					}
				}
			K::SecureClockStatus |= ESecureClockPresent;	// Flag that a secure time has/is going to be set.

			if (K::SecureClockStatus == ESecureClockOk)		// i.e. if Nonsecure offset present (if secure clock in use)
				{
				TInt64 tmp = old;
				tmp -= s;
				K::NonSecureOffsetSeconds = static_cast<TInt32>(tmp); // i.e. Nonsecure time remains unchanged
				s = old;											  // so the software RTC (set below) remains unchanged nonsecure time
				}
			}
		// Update nonsecure clock
		else 
			// If the secure clock is in working order (i.e. both a secure time source and the
			// nonsecure offset are present) then all we have to do is update the nonsecure 
			// offset. Otherwise (i.e. either secure clock attribute is missing) all we can do 
			// is update the hardware clock.
			if (K::SecureClockStatus == ESecureClockOk)
			{
				TInt64 tmp = K::NonSecureOffsetSeconds;
				tmp += s;
				tmp -= old;
				K::NonSecureOffsetSeconds = static_cast<TInt32>(tmp);
			}
			else
			{
				if (aTimeSetMode & Kern::ETimeSet_SetHwRtc)
					{
					r = A::SetSystemTimeInSecondsFrom2000(s);
					// Set SW RTC even if HW RTC is not supported 
					if ((r!=KErrNone) && (r != KErrNotSupported))
						{
						TTickQ::Signal();
						return r;
						}
					}
			}
		}

	// Update the software RTC and offset
	K::SetSystemTime(s, aOffset, aChanges, aMode|ETimeSetAllowTimeReversal);

	// abort locked and absolute timers if the nonsecure time changed
	if (aChanges & EChangesSystemTime && !(aTimeSetMode & Kern::ETimeSet_Secure))
		AbortTimers(ETrue);

	if (aMode & ETimeSetTime)
		{
		// queue wakeup DFC to restart the timer queues
		// if iInTick is already set, wake up DFC is already queued
		if (!tq.iInTick)
			{
			tq.iInTick = ETrue;
			sq.iWakeUpDfc.Enque();
			}

		// tell the power manager
		if (K::PowerModel)
			K::PowerModel->SystemTimeChanged(old, s);
		}
	else if (aChanges & EChangesSystemTime)
		sq.StartTimer();
	
	TTickQ::Signal();

	if (aChanges)
		{
		if (aTimeSetMode & Kern::ETimeSet_SyncNotify)
			Kern::NotifyChanges(aChanges);
		else if (aTimeSetMode & Kern::ETimeSet_AsyncNotify)
			Kern::AsyncNotifyChanges(aChanges);
		}
	return KErrNone;
	}


TTimerLockSpec ExecHandler::LockPeriod()
	{
	// NO ONE USES THIS, SO KILL IT
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::LockPeriod"));
	return ETwelveOClock;
	}

/** Returns the period of the Symbian OS tick timer.  

	@return Period in microseconds.
	
	@pre Call in any context.
	
	@see TTickLink
*/

EXPORT_C TInt Kern::TickPeriod()
	{
	return K::TickQ->iNominalTickPeriod;
	}
