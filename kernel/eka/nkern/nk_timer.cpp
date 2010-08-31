// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkern\nk_timer.cpp
// Fast Millisecond Timer Implementation
// This file is just a template - you'd be mad not to machine code this
// 
//

#include "nk_priv.h"

const TInt KTimerQDfcPriority=6;

GLDEF_D NTimerQ TheTimerQ;

#ifndef __MSTIM_MACHINE_CODED__
#ifdef _DEBUG
#define __DEBUG_CALLBACK(n)	{if (iDebugFn) (*iDebugFn)(iDebugPtr,n);}
#else
#define __DEBUG_CALLBACK(n)
#endif


/** Starts a nanokernel timer in one-shot mode with ISR callback.
	
	Queues the timer to expire in the specified number of nanokernel ticks. The
	actual wait time will be at least that much and may be up to one tick more.
	The expiry handler will be called in ISR context.
	
	Note that NKern::TimerTicks() can be used to convert milliseconds to ticks.

	@param	aTime Timeout in nanokernel ticks
	
	@return	KErrNone if no error; KErrInUse if timer is already active.
	
	@pre	Any context
	
	@see    NKern::TimerTicks()
 */
EXPORT_C TInt NTimer::OneShot(TInt aTime)
	{
	return OneShot(aTime,FALSE);
	}


/** Starts a nanokernel timer in one-shot mode with ISR or DFC callback.
	
	Queues the timer to expire in the specified number of nanokernel ticks. The
	actual wait time will be at least that much and may be up to one tick more.
	The expiry handler will be called in either ISR context or in the context
	of the nanokernel timer thread (DfcThread1).

	Note that NKern::TimerTicks() can be used to convert milliseconds to ticks.

	Be aware that if you are being called back in DFC context then the DFC thread
	involved (DfcThread1) will be shared with both the kernel and other device
	drivers.  YOU MUST DO YOUR WORK AND RETURN AS QUICKLY AS POSSIBLE in order to
	not interfere with the internal workings of the kernel and with other device
	drivers.  If you need to do anything more complex than signalling a semaphore
	or similar short term processing then you may want to consider using your own
	DFC queue.  Failure to follow these rules WILL LEAD TO INSTABILITY OF THE PHONE.

	@param	aTime Timeout in nanokernel ticks
	@param	aDfc TRUE if DFC callback required, FALSE if ISR callback required.
	
	@return	KErrNone if no error; KErrInUse if timer is already active.
	
	@pre	Any context
	
	@see    NKern::TimerTicks()
 */
EXPORT_C TInt NTimer::OneShot(TInt aTime, TBool aDfc)
	{
	__NK_ASSERT_DEBUG(aTime>=0);

	/** iFunction could be set to NULL after NTimer::OneShot(TInt, TDfc&) call.
	Call-back mechanism cannot be changed in the life time of a timer. */
	__NK_ASSERT_DEBUG(iFunction!=NULL); 

	TInt irq=NKern::DisableAllInterrupts();
	if (iState!=EIdle)
		{
		NKern::RestoreInterrupts(irq);
		return KErrInUse;
		}
	iCompleteInDfc=TUint8(aDfc?1:0);
	iTriggerTime=TheTimerQ.iMsCount+(TUint32)aTime;
	TheTimerQ.Add(this);
	NKern::RestoreInterrupts(irq);
	return KErrNone;
	}

/** Starts a nanokernel timer in one-shot mode with callback in dfc thread that provided DFC belongs to.
	
	Queues the timer to expire in the specified number of nanokernel ticks. The
	actual wait time will be at least that much and may be up to one tick more.
	On expiry aDfc will be queued in ISR context.

    Note that NKern::TimerTicks() can be used to convert milliseconds to ticks.

	@param	aTime Timeout in nanokernel ticks
	@param	aDfc - Dfc to be queued when the timer expires.
	
	@return	KErrNone if no error; KErrInUse if timer is already active.
	
	@pre	Any context
	
	@see    NKern::TimerTicks()
 */
EXPORT_C TInt NTimer::OneShot(TInt aTime, TDfc& aDfc)
	{
	__NK_ASSERT_DEBUG(aTime>=0);
	TInt irq=NKern::DisableAllInterrupts();
	if (iState!=EIdle)
		{
		NKern::RestoreInterrupts(irq);
		return KErrInUse;
		}
	iCompleteInDfc = 0;
	iFunction = NULL;
	iPtr = (TAny*) &aDfc;
	iTriggerTime=TheTimerQ.iMsCount+(TUint32)aTime;
	TheTimerQ.Add(this);
	NKern::RestoreInterrupts(irq);
	return KErrNone;
	}


/** Starts a nanokernel timer in zero-drift periodic mode with ISR or DFC callback.

	Queues the timer to expire in the specified number of nanokernel ticks,
	measured from the time at which it last expired. This allows exact periodic
	timers to be implemented with no drift caused by delays in requeueing the
	timer.

	The expiry handler will be called in the same context as the previous timer
	expiry. Generally the way this is used is that NTimer::OneShot() is used to start 
	the first time interval and this specifies whether the callback is in ISR context 
	or in the context of the nanokernel timer thread (DfcThread1) or other Dfc thread.
	The expiry handler then uses NTimer::Again() to requeue the timer.

	@param	aTime Timeout in nanokernel ticks

	@return	KErrNone if no error; KErrInUse if timer is already active;
	        KErrArgument if the requested expiry time is in the past.
	        
	@pre	Any context
 */
EXPORT_C TInt NTimer::Again(TInt aTime)
//
// Wait aTime from last trigger time - used for periodic timers
//
	{
	TInt irq=NKern::DisableAllInterrupts();
	if (iState!=EIdle)
		{
		NKern::RestoreInterrupts(irq);
		return KErrInUse;
		}
	TUint32 nextTick=TheTimerQ.iMsCount;
	TUint32 trigger=iTriggerTime+(TUint32)aTime;
	TUint32 d=trigger-nextTick;
	if (d>=0x80000000)
		{
		NKern::RestoreInterrupts(irq);
		return KErrArgument;		// requested time is in the past
		}
	iTriggerTime=trigger;
	TheTimerQ.Add(this);
	NKern::RestoreInterrupts(irq);
	return KErrNone;
	}


/** Cancels a nanokernel timer.

	Removes this timer from the nanokernel timer queue. Does nothing if the
	timer is inactive or has already expired.
	Note that if the timer was queued and DFC callback requested it is possible
	for the expiry handler to run even after Cancel() has been called. This will
	occur in the case where DfcThread1 is preempted just before calling the
	expiry handler for this timer and the preempting thread/ISR/IDFC calls
	Cancel() on the timer.

	@pre	Any context
	@return	TRUE if timer was actually cancelled
	@return	FALSE if timer was not cancelled - this could be because it was not
				active or because its expiry handler was already running on
				another CPU or in the timer DFC.
 */
EXPORT_C TBool NTimer::Cancel()
	{
	TBool result = TRUE;
	TInt irq=NKern::DisableAllInterrupts();
	if (iState>ETransferring)	// idle or transferring timers are not on a queue
		Deque();
	switch (iState)
		{
		case ETransferring:	// signal DFC to abort this iteration
			TheTimerQ.iTransferringCancelled=TRUE;
			break;
		case ECritical:		// signal DFC to abort this iteration
			TheTimerQ.iCriticalCancelled=TRUE;
			break;
		case EFinal:
			{
			// Need to clear bit in iPresent if both final queues now empty
			// NOTE: Timer might actually be on the completed queue rather than the final queue
			//		 but the check is harmless in any case.
			TInt i=iTriggerTime & NTimerQ::ETimerQMask;
			NTimerQ::STimerQ& q=TheTimerQ.iTickQ[i];
			if (q.iIntQ.IsEmpty() && q.iDfcQ.IsEmpty())
				TheTimerQ.iPresent &= ~(1<<i);
			break;
			}
		case EIdle:			// nothing to do
			result = FALSE;
		case EHolding:		// just deque
		case EOrdered:		// just deque
			break;
		}
	iState=EIdle;
	NKern::RestoreInterrupts(irq);
	return result;
	}
#endif


/** Check if a nanokernel timer is pending or not

	@return	TRUE if the timer is pending (OneShot() etc. would return KErrInUse)
	@return FALSE if the timer is idle (OneShot() etc. would succeed)
	@pre	Any context

	@publishedPartner
	@prototype
 */
EXPORT_C TBool NTimer::IsPending()
	{
	return iState != EIdle;
	}


/** Obtains the address of the nanokernel timer queue object.

	Not intended for general use. Intended only for base ports in order to get
	the address used to call NTimerQ::Tick() with.

	@return	The address of the nanokernel timer queue object
	@pre	Any context
 */
EXPORT_C TAny* NTimerQ::TimerAddress()
	{
	return &TheTimerQ;
	}

NTimerQ::NTimerQ()
	:	iDfc(NTimerQ::DfcFn,this,NULL,KTimerQDfcPriority)
	{
	// NOTE: All other members are initialised to zero since the single instance
	//		 of NTimerQ resides in .bss
	}

void NTimerQ::Init1(TInt aTickPeriod)
	{
	TheTimerQ.iTickPeriod=aTickPeriod;
	__KTRACE_OPT(KBOOT,DEBUGPRINT("NTimerQ::Init1 - period %d us",aTickPeriod));
	__KTRACE_OPT(KMEMTRACE, DEBUGPRINT("MT:P %d",aTickPeriod));
	}

void NTimerQ::Init3(TDfcQue* aDfcQ)
	{
	__KTRACE_OPT(KBOOT,DEBUGPRINT("NTimerQ::Init3 DFCQ at %08x",aDfcQ));
	TheTimerQ.iDfc.SetDfcQ(aDfcQ);
	}

#ifndef __MSTIM_MACHINE_CODED__
void NTimerQ::Add(NTimer* aTimer)
//
//	Internal function to add a timer to the queue.
//	Enter and return with all interrupts disabled.
//
	{
	TInt t=TInt(aTimer->iTriggerTime-iMsCount);
	if (t<ENumTimerQueues)
		AddFinal(aTimer);
	else
		{
		// >=32ms to expiry, so put on holding queue
		aTimer->iState=NTimer::EHolding;
		iHoldingQ.Add(aTimer);
		}
	}

void NTimerQ::AddFinal(NTimer* aTimer)
//
//	Internal function to add a timer to the corresponding final queue.
//	Enter and return with all interrupts disabled.
//
	{
	TInt i=aTimer->iTriggerTime & ETimerQMask;
	SDblQue* pQ;
	if (aTimer->iCompleteInDfc)
		pQ=&iTickQ[i].iDfcQ;
	else
		pQ=&iTickQ[i].iIntQ;
	iPresent |= (1<<i);
	aTimer->iState=NTimer::EFinal;
	pQ->Add(aTimer);
	}

void NTimerQ::DfcFn(TAny* aPtr)
	{
	((NTimerQ*)aPtr)->Dfc();
	}

void NTimerQ::Dfc()
//
// Do deferred timer queue processing and/or DFC completions
//
	{
	TInt irq;

	// First transfer entries on the Ordered queue to the Final queues
	FOREVER
		{
		irq=NKern::DisableAllInterrupts();
		if (iOrderedQ.IsEmpty())
			break;
		NTimer* pC=(NTimer*)iOrderedQ.First();
		TInt remain=pC->iTriggerTime-iMsCount;
		if (remain>=ENumTimerQueues)
			break;

		// If remaining time <32 ticks, add it to final queue;
		// also if remain < 0 we've 'missed it' so add to final queue.
		pC->Deque();
		AddFinal(pC);
		NKern::RestoreInterrupts(irq);
		__DEBUG_CALLBACK(0);
		}
	NKern::RestoreInterrupts(irq);
	__DEBUG_CALLBACK(1);

	// Next transfer entries on the Holding queue to the Ordered queue or final queue
	FOREVER
		{
		irq=NKern::DisableAllInterrupts();
		if (iHoldingQ.IsEmpty())
			break;
		NTimer* pC=(NTimer*)iHoldingQ.First();
		pC->Deque();
		pC->iState=NTimer::ETransferring;
		iTransferringCancelled=FALSE;
		TUint32 trigger=pC->iTriggerTime;
		if (TInt(trigger-iMsCount)<ENumTimerQueues)
			{
			// <32ms remaining so put it on final queue
			AddFinal(pC);
			}
		else
			{
			FOREVER
				{
				NKern::RestoreInterrupts(irq);
				__DEBUG_CALLBACK(2);

				// we now need to walk ordered queue to find correct position for pC
				SDblQueLink* anchor=&iOrderedQ.iA;
				iCriticalCancelled=FALSE;
				irq=NKern::DisableAllInterrupts();
				NTimer* pN=(NTimer*)iOrderedQ.First();
				while (pN!=anchor && !iTransferringCancelled)
					{
					if ((pN->iTriggerTime-trigger)<0x80000000u)
						break;	// insert before pN
					pN->iState=NTimer::ECritical;
					NKern::RestoreInterrupts(irq);
					__DEBUG_CALLBACK(3);
					irq=NKern::DisableAllInterrupts();
					if (iCriticalCancelled)
						break;
					pN->iState=NTimer::EOrdered;
					pN=(NTimer*)pN->iNext;
					}

				if (iTransferringCancelled)
					break;		// this one has been cancelled, go on to next one
				if (!iCriticalCancelled)
					{
					pC->InsertBefore(pN);
					pC->iState=NTimer::EOrdered;
					break;		// done this one
					}
				}
			}
		NKern::RestoreInterrupts(irq);
		__DEBUG_CALLBACK(4);
		}
	NKern::RestoreInterrupts(irq);
	__DEBUG_CALLBACK(5);

	// Finally do call backs for timers which requested DFC callback
	FOREVER
		{
		irq=NKern::DisableAllInterrupts();
		if (iCompletedQ.IsEmpty())
			break;
		NTimer* pC=(NTimer*)iCompletedQ.First();
		pC->Deque();
		pC->iState=NTimer::EIdle;
		TAny* p=pC->iPtr;
		NTimerFn f=pC->iFunction;
		NKern::RestoreInterrupts(irq);
		__DEBUG_CALLBACK(7);
		(*f)(p);
		}
	NKern::RestoreInterrupts(irq);
	}


/** Tick over the nanokernel timer queue.
	This function should be called by the base port in the system tick timer ISR.
	It should not be called at any other time.
	The value of 'this' to pass is the value returned by NTimerQ::TimerAddress().

	@see NTimerQ::TimerAddress()
 */
EXPORT_C void NTimerQ::Tick()
	{
#ifdef _DEBUG
	// If there are threads waiting to be released by the tick, enqueue the dfc
	if (!TheScheduler.iDelayedQ.IsEmpty())
		TheScheduler.iDelayDfc.Add();
#endif
	TheScheduler.TimesliceTick();
	TInt irq=NKern::DisableAllInterrupts();
	TInt i=iMsCount & ETimerQMask;
	iMsCount++;
	STimerQ* pQ=iTickQ+i;
	iPresent &= ~(1<<i);
	TBool doDfc=FALSE;
	if (!pQ->iDfcQ.IsEmpty())
		{
		// transfer DFC completions to completed queue and queue DFC
		iCompletedQ.MoveFrom(&pQ->iDfcQ);
		doDfc=TRUE;
		}
	if ((i&(ETimerQMask>>1))==0)
		{
		// Every 16 ticks we check if a DFC is required.
		// This allows a DFC latency of up to 16 ticks before timers are missed.
		if (!iHoldingQ.IsEmpty())
			doDfc=TRUE;				// if holding queue nonempty, queue DFC to sort
		else if (!iOrderedQ.IsEmpty())
			{
			// if first ordered queue entry expires in <32ms, queue the DFC to transfer
			NTimer* pC=(NTimer*)iOrderedQ.First();
#ifdef __EPOC32__
			__ASSERT_WITH_MESSAGE_DEBUG(iMsCount<=pC->iTriggerTime, "iMsCount has exceeded pC->iTriggerTime; function called later than expected ","NTimerQ::Tick()");
#endif
			if (TInt(pC->iTriggerTime-iMsCount)<ENumTimerQueues)
				doDfc=TRUE;
			}
		}
	if (!pQ->iIntQ.IsEmpty())
		{
		// transfer ISR completions to a temporary queue
		// careful here - higher priority interrupts could dequeue timers!
		SDblQue q(&pQ->iIntQ,0);
		while(!q.IsEmpty())
			{
			NTimer* pC=(NTimer*)q.First();
			pC->Deque();
			pC->iState=NTimer::EIdle;
			NKern::RestoreInterrupts(irq);
			if (pC->iFunction)
				(*pC->iFunction)(pC->iPtr);
			else
				((TDfc*)(pC->iPtr))->Add();
			irq=NKern::DisableAllInterrupts();
			}
		}
	NKern::RestoreInterrupts(irq);
	if (doDfc)
		iDfc.Add();
	}


/** Return the number of ticks before the next nanokernel timer expiry.
	May on occasion return a pessimistic estimate (i.e. too low).
	Used by base port to disable the system tick interrupt when the system
	is idle.

	@return	The number of ticks before the next nanokernel timer expiry.
	
	@pre	Interrupts must be disabled.
	
	@post	Interrupts are disabled.
 */
EXPORT_C TInt NTimerQ::IdleTime()
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_DISABLED,"NTimerQ::IdleTime");	
#ifdef _DEBUG
	// If there are threads waiting to be released by the tick we can't idle
	if (!TheScheduler.iDelayedQ.IsEmpty())
		return 1;
#endif
	NTimerQ& m=TheTimerQ;
	TUint32 next=m.iMsCount;	// number of next tick
	TUint32 p=m.iPresent;
	TInt r=KMaxTInt;
	if (p)
		{
		// Final queues nonempty
		TInt nx=next&0x1f;				// number of next tick modulo 32
		p=(p>>nx)|(p<<(32-nx));			// rotate p right by nx (so lsb corresponds to next tick)
		r=__e32_find_ls1_32(p);			// find number of zeros before LS 1
		}
	if (!m.iHoldingQ.IsEmpty())
		{
		// Sort operation required - need to process next tick divisible by 16
		TInt nx=next&0x0f;				// number of next tick modulo 16
		TInt r2=nx?(16-nx):0;			// number of ticks before next divisible by 16
		if (r2<r)
			r=r2;
		}
	if (!m.iOrderedQ.IsEmpty())
		{
		// Timers present on ordered queue
		NTimer* pC=(NTimer*)m.iOrderedQ.First();
		TUint32 tt=pC->iTriggerTime;
		tt=(tt&~0x0f)-16;				// time at which transfer to final queue would occur
		TInt r3=(TInt)(tt-next);
		if (r3<r)
			r=r3;
		}
	return r;
	}
#endif


/** Advance the nanokernel timer queue by the specified number of ticks.
	It is assumed that no timers expire as a result of this.
	Used by base port when system comes out of idle mode after disabling the
	system tick interrupt to bring the timer queue up to date.

	@param	aTicks Number of ticks skipped due to tick suppression

	@pre	Interrupts must be disabled.

	@post	Interrupts are disabled.
 */
EXPORT_C void NTimerQ::Advance(TInt aTicks)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_DISABLED,"NTimerQ::Advance");	
	TheTimerQ.iMsCount+=(TUint32)aTicks;
	}


/**	Returns the period of the nanokernel timer.
	@return Period in microseconds
	@pre any context
	@see NTimer
 */
EXPORT_C TInt NKern::TickPeriod()
	{
	return TheTimerQ.iTickPeriod;
	}


/**	Converts a time interval to timer ticks.

	@param aMilliseconds time interval in milliseconds.
	@return Number of nanokernel timer ticks.  Non-integral results are rounded up.

 	@pre aMilliseconds should be <=2147483 to avoid integer overflow.
	@pre any context
 */
EXPORT_C TInt NKern::TimerTicks(TInt aMilliseconds)
	{
	__ASSERT_WITH_MESSAGE_DEBUG(aMilliseconds<=2147483,"aMilliseconds should be <=2147483","NKern::TimerTicks");
	TUint32 msp=TheTimerQ.iTickPeriod;
	if (msp==1000)	// will be true except on pathological hardware
		return aMilliseconds;
	TUint32 us=(TUint32)aMilliseconds*1000;
	return (us+msp-1)/msp;
	}

