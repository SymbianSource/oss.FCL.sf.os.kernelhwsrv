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

#define i_NTimer_iState			i8888.iHState1
#define i_NTimer_iCompleteInDfc	i8888.iHState2

const TInt KTimerQDfcPriority=6;

GLDEF_D NTimerQ TheTimerQ;

extern "C" void send_irq_ipi(TSubScheduler*, TInt);

#ifndef __MSTIM_MACHINE_CODED__
#ifdef _DEBUG
#define __DEBUG_CALLBACK(n)	{if (iDebugFn) (*iDebugFn)(iDebugPtr,n);}
#else
#define __DEBUG_CALLBACK(n)
#endif

/** Construct a nanokernel timer tied to a specified thread or group
	

	@param	aTied Pointer to the thread/group to which the timer should be tied
	@param	aFunction Pointer to the function to call on timer expiry
	@param	aPtr Parameter to pass to the expiry handler
	
	@pre	Any context

	@publishedPartner
	@prototype
 */
EXPORT_C NTimer::NTimer(NSchedulable* aTied, NTimerFn aFunction, TAny* aPtr)
	{
	iPtr = aPtr;
	iFn = aFunction;
	iHType = EEventHandlerNTimer;
//	i8888.iHState1 = EIdle;		done by NEventHandler constructor
	if (aTied)
		{
		SetTied(aTied);
		}
	}


/** Construct a nanokernel timer which mutates into and runs as a DFC on expiry
	The DFC queue is not specified at object construction time, but must be set
	using NTimer::SetDfcQ() before the timer is used.

	@param	aFunction	Pointer to the function to call on timer expiry
	@param	aPtr		Parameter to pass to the expiry handler
	@param	aPriority	Priority of DFC within the queue (0 to 7, where 7 is highest)
	
	@pre	Any context

	@publishedPartner
	@prototype
 */
EXPORT_C NTimer::NTimer(TDfcFn aFunction, TAny* aPtr, TInt aPriority)
	{
	iPtr = aPtr;
	iFn = aFunction;
	iTied = 0;
	iHType = (TUint8)aPriority;
//	i8888.iHState0 = 0;			done by NEventHandler constructor
//	i8888.iHState1 = EIdle;		done by NEventHandler constructor
//	i8888.iHState2 = 0;			done by NEventHandler constructor
	}


/** Construct a nanokernel timer which mutates into and runs as a DFC on expiry

	@param	aFunction	Pointer to the function to call on timer expiry
	@param	aPtr		Parameter to pass to the expiry handler
	@param	aDfcQ		Pointer to DFC queue which this timer should use
	@param	aPriority	Priority of DFC within the queue (0 to 7, where 7 is highest)
	
	@pre	Any context

	@publishedPartner
	@prototype
 */
EXPORT_C NTimer::NTimer(TDfcFn aFunction, TAny* aPtr, TDfcQue* aDfcQ, TInt aPriority)
	{
	iPtr = aPtr;
	iFn = aFunction;
	iDfcQ = aDfcQ;
	iHType = (TUint8)aPriority;
//	i8888.iHState0 = 0;			done by NEventHandler constructor
//	i8888.iHState1 = EIdle;		done by NEventHandler constructor
//	i8888.iHState2 = 0;			done by NEventHandler constructor
	}


/** Set the DFC queue to be used by an NTimer constructed using a TDfcFn

	@param	aDfcQ		Pointer to DFC queue which this timer should use

	@pre	Timer cannot be in use
	@pre	Any context

	@publishedPartner
	@prototype
 */
EXPORT_C void NTimer::SetDfcQ(TDfcQue* aDfcQ)
	{
	__NK_ASSERT_ALWAYS(aDfcQ!=0);
	__NK_ASSERT_ALWAYS(iHType < KNumDfcPriorities);
	__NK_ASSERT_ALWAYS(i8816.iHState16==EIdle);
	iDfcQ = aDfcQ;
	}


/** Tie a nanokernel timer to a thread or group

	@param	aTied = pointer to thread or group to which IDFC should be tied
	@return	KErrNone if successful
	@return	KErrDied if thread has exited or group has been destroyed.

	@pre Call in thread context, interrupts enabled
	@pre Timer must not be queued or running
	@pre Timer must not already be tied
	@pre Must not be a mutating timer (constructed with TDfcFn)

	@publishedPartner
	@prototype
 */
EXPORT_C TInt NTimer::SetTied(NSchedulable* aTied)
	{
	__NK_ASSERT_ALWAYS(!IsMutating());
	__NK_ASSERT_ALWAYS(i8888.iHState1 == EIdle);
	__NK_ASSERT_ALWAYS(aTied && !iTied);
	NKern::Lock();
	TInt r = aTied->AddTiedEvent(this);
	__NK_ASSERT_ALWAYS(r==KErrNone || r==KErrDied);
	NKern::Unlock();
	return r;
	}


/** Destroy a nanokernel timer

	@pre Call in thread context, interrupts enabled, preemption enabled
	@pre Calling thread in critical section
	@pre No fast mutex held

	@publishedPartner
	@prototype
 */
EXPORT_C NTimer::~NTimer()
	{
	if (!IsMutating() && iTied)
		{
		NKern::Lock();
		// remove from tied thread/group
		NEventHandler::TiedLock.LockOnly();
		NSchedulable* tied = iTied;
		DoCancel(ECancelDestroy);
		if (tied)	// might have been dequeued by thread/group termination
			{
			tied->AcqSLock();
			if (iTiedLink.iNext)
				{
				iTiedLink.Deque();
				iTiedLink.iNext = 0;
				}
			iTied = 0;
			tied->RelSLock();
			}
		NEventHandler::TiedLock.UnlockOnly();
		NKern::Unlock();
		}
	else if (IsMutating() && iDfcQ)
		DoCancelMutating(ECancelDestroy);
	else
		DoCancel(ECancelDestroy);
	}


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
	For normal timers (constructed with NTimerFn) the expiry handler will be
	called in either ISR context or in the context of the nanokernel timer
	thread (DfcThread1). For mutating timers (constructed with TDfcFn) the
	expiry handler is called in the context of the thread running the relevant
	TDfcQue.

    Note that NKern::TimerTicks() can be used to convert milliseconds to ticks.

	@param	aTime Timeout in nanokernel ticks
	@param	aDfc TRUE if DFC callback required, FALSE if ISR callback required.
			Note that this parameter is ignored for mutating timers.
	
	@return	KErrNone if no error
	@return	KErrInUse if timer is already active.
	@return	KErrDied if tied thread/group has exited
	
	@pre	Any context
	
	@see    NKern::TimerTicks()
 */
EXPORT_C TInt NTimer::OneShot(TInt aTime, TBool aDfc)
	{
	__NK_ASSERT_DEBUG(aTime>=0);
	/** iFn could be set to NULL after NTimer::OneShot(TInt, TDfc&) call.
	Call-back mechanism cannot be changed in the life time of a timer. */
	__NK_ASSERT_DEBUG(iFn!=NULL);

	TInt irq = TheTimerQ.iTimerSpinLock.LockIrqSave();
	if (!IsValid())
		{
		TheTimerQ.iTimerSpinLock.UnlockIrqRestore(irq);
		return KErrDied;
		}
	TUint16 state = i8816.iHState16;
	if (IsNormal())
		state &= 0xFF;
	else
		aDfc = FALSE;	// mutating timers start as ISR completion
	if (state!=EIdle)
		{
		TheTimerQ.iTimerSpinLock.UnlockIrqRestore(irq);
		return KErrInUse;
		}
	mb();	// ensure that if we observe an idle state all accesses to the NTimer have also been observed
	i_NTimer_iCompleteInDfc=TUint8(aDfc?1:0);
	iTriggerTime=TheTimerQ.iMsCount+(TUint32)aTime;
	TheTimerQ.Add(this);
	TheTimerQ.iTimerSpinLock.UnlockIrqRestore(irq);
	return KErrNone;
	}


/** Starts a nanokernel timer in one-shot mode with callback in dfc thread that provided DFC belongs to.
	
	Queues the timer to expire in the specified number of nanokernel ticks. The
	actual wait time will be at least that much and may be up to one tick more.
	On expiry aDfc will be queued in ISR context.

    Note that NKern::TimerTicks() can be used to convert milliseconds to ticks.

	@param	aTime Timeout in nanokernel ticks
	@param	aDfc - Dfc to be queued when the timer expires.
	
	@return	KErrNone if no error
	@return	KErrInUse if timer is already active.
	@return	KErrDied if tied thread/group has exited
	
	@pre	Any context
	@pre	Must not be a mutating timer (constructed with TDfcFn)
	
	@see    NKern::TimerTicks()
 */
EXPORT_C TInt NTimer::OneShot(TInt aTime, TDfc& aDfc)
	{
	__NK_ASSERT_DEBUG(!IsMutating());
	__NK_ASSERT_DEBUG(aTime>=0);
	TInt irq = TheTimerQ.iTimerSpinLock.LockIrqSave();
	if (iHType != EEventHandlerNTimer)
		{
		TheTimerQ.iTimerSpinLock.UnlockIrqRestore(irq);
		return KErrDied;
		}
	if (i_NTimer_iState!=EIdle)
		{
		TheTimerQ.iTimerSpinLock.UnlockIrqRestore(irq);
		return KErrInUse;
		}
	mb();	// ensure that if we observe an idle state all accesses to the NTimer have also been observed
	i_NTimer_iCompleteInDfc = 0;
	iFn = NULL;
	iPtr = (TAny*) &aDfc;
	iTriggerTime=TheTimerQ.iMsCount+(TUint32)aTime;
	TheTimerQ.Add(this);
	TheTimerQ.iTimerSpinLock.UnlockIrqRestore(irq);
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

	@return	KErrNone if no error
	@return	KErrInUse if timer is already active;
	@return	KErrArgument if the requested expiry time is in the past.
	@return	KErrDied if tied thread/group has exited
	        
	@pre	Any context
 */
EXPORT_C TInt NTimer::Again(TInt aTime)
//
// Wait aTime from last trigger time - used for periodic timers
//
	{
	TInt irq = TheTimerQ.iTimerSpinLock.LockIrqSave();
	if (!IsValid())
		{
		TheTimerQ.iTimerSpinLock.UnlockIrqRestore(irq);
		return KErrDied;
		}
	TUint16 state = i8816.iHState16;
	if (IsNormal())
		state &= 0xFF;
	if (state!=EIdle)
		{
		TheTimerQ.iTimerSpinLock.UnlockIrqRestore(irq);
		return KErrInUse;
		}
	mb();	// ensure that if we observe an idle state all accesses to the NTimer have also been observed
	TUint32 nextTick=TheTimerQ.iMsCount;
	TUint32 trigger=iTriggerTime+(TUint32)aTime;
	TUint32 d=trigger-nextTick;
	if (d>=0x80000000)
		{
		TheTimerQ.iTimerSpinLock.UnlockIrqRestore(irq);
		return KErrArgument;		// requested time is in the past
		}
	iTriggerTime=trigger;
	TheTimerQ.Add(this);
	TheTimerQ.iTimerSpinLock.UnlockIrqRestore(irq);
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

	@pre	Any context for a non-mutating NTimer (constructed with NTimerFn)
	@pre	For mutating NTimer (constructed with TDfcFn), IDFC or thread context only.
	@return	TRUE if timer was actually cancelled
	@return	FALSE if timer was not cancelled - this could be because it was not
				active or because its expiry handler was already running on
				another CPU or in the timer DFC.
 */
EXPORT_C TBool NTimer::Cancel()
	{
	if (IsMutating() && iDfcQ)
		return DoCancelMutating(0);
	return DoCancel(0)!=EIdle;
	}

void NTimer::DoCancel0(TUint aState)
	{
	if (aState>ETransferring && aState<=EFinal)	// idle or transferring timers are not on a queue
		Deque();
	switch (aState)
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
		case EHolding:		// just deque
		case EOrdered:		// just deque
			break;
		default:
			__NK_ASSERT_ALWAYS(0);
		}
	}

TUint NTimer::DoCancel(TUint aFlags)
	{
	NSchedulable* tied = 0;
	TInt irq = NKern::DisableAllInterrupts();
	TheTimerQ.iTimerSpinLock.LockOnly();
	TUint state = i_NTimer_iState;
	mb();
	if (IsNormal() && state>=EEventQ)
		{
		// It's on a CPU's event handler queue
		TInt cpu = state - EEventQ;
		if (cpu < TheScheduler.iNumCpus)
			{
			TSubScheduler* ss = TheSubSchedulers + cpu;
			ss->iEventHandlerLock.LockOnly();
			state = i_NTimer_iState;
			if (state != EIdle)
				{
				Deque();	// we got to it first
				tied = iTied;
				i_NTimer_iState = EIdle;
				}
			ss->iEventHandlerLock.UnlockOnly();
			goto end;
			}
		}
	DoCancel0(state);
	if (IsMutating())
		i8816.iHState16 = 0;
	else
		i_NTimer_iState=EIdle;
end:
	if (aFlags & ECancelDestroy)
		iHType = EEventHandlerDummy;
	TheTimerQ.iTimerSpinLock.UnlockOnly();
	if (tied)
		tied->EndTiedEvent();	// FIXME - Could be called in thread context
	NKern::RestoreInterrupts(irq);
	return state;
	}

TBool NTimer::DoCancelMutating(TUint aFlags)
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR,"NTimer::Cancel (mutating NTimer)");
	TSubScheduler& ss0 = SubScheduler();
	TBool wait = FALSE;
	TInt cpu = -1;
	TBool result = TRUE;
	TDfc* d = (TDfc*)this;
	NKern::Lock();
	TDfcQue* q = iDfcQ;
	NThreadBase* t = q->iThread;
	t->AcqSLock();
	TheTimerQ.iTimerSpinLock.LockIrq();

	// 0000->0000, XX00->ZZ00, xxYY->zzYY
	TUint state = d->CancelInitialStateChange();
	if (state & 0xFF00)
		{
		// someone else cancelling at the same time - just wait for them to finish
		// they can only be waiting for the cancel IPI
		result = FALSE;
		wait = TRUE;
		goto end;
		}
	if (state == 0)	// timer was not active
		{
		result = FALSE;
		goto end;
		}
	if (state>=ETransferring && state<=EFinal)
		{
		DoCancel0(state);
		// cancel is complete
		goto reset;
		}
	if (state==1)
		{
		// on DFC final queue
		q->Remove((TPriListLink*)this);
		goto reset;
		}
	// must be on IDFC queue - need to send cancel IPI
	__NK_ASSERT_ALWAYS((state>>5)==4);
	cpu = state & 0x1f;
	if (TUint(cpu) == ss0.iCpuNum)
		{
		// it's on this CPU's IDFC queue so just dequeue it and finish
		Deque();
		cpu = -1;
reset:
		d->ResetState();	// release semantics
		}
end:
	if (aFlags & ECancelDestroy)
		iHType = EEventHandlerDummy;
	TheTimerQ.iTimerSpinLock.UnlockIrq();
	t->RelSLock();
	if (cpu>=0)
		{
		TCancelIPI ipi;
		ipi.Send(d, cpu);
		ipi.WaitCompletion();
		wait = TRUE;
		}
	if (wait)
		{
		TUint n = 0x01000000;
		while ((i8816.iHState16>>8) & ss0.iCpuMask)
			{
			__chill();
			if (!--n)
				__crash();
			}
		}
	NKern::Unlock();
	return result;
	}
#endif


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
	:	iDfc(NTimerQ::DfcFn,this,NULL,KTimerQDfcPriority),
		iDfcCompleteCount(1),
		iTimerSpinLock(TSpinLock::EOrderNTimerQ)
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
	NThreadBase* t = aDfcQ->iThread;
	t->iRebalanceAttr = 1;
	}

#ifndef __MSTIM_MACHINE_CODED__
void NTimerQ::Add(NTimer* aTimer)
//
//	Internal function to add a timer to the queue.
//	Enter and return with timer queue spin lock held.
//
	{
	TInt t=TInt(aTimer->iTriggerTime-iMsCount);
	if (t<ENumTimerQueues)
		AddFinal(aTimer);
	else
		{
		// >=32ms to expiry, so put on holding queue
		aTimer->i_NTimer_iState=NTimer::EHolding;
		iHoldingQ.Add(aTimer);
		}
	}

void NTimerQ::AddFinal(NTimer* aTimer)
//
//	Internal function to add a timer to the corresponding final queue.
//	Enter and return with timer queue spin lock held.
//
	{
	TInt i=aTimer->iTriggerTime & ETimerQMask;
	SDblQue* pQ;
	if (aTimer->i_NTimer_iCompleteInDfc)
		pQ=&iTickQ[i].iDfcQ;
	else
		pQ=&iTickQ[i].iIntQ;
	iPresent |= (1<<i);
	aTimer->i_NTimer_iState=NTimer::EFinal;
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
	// First transfer entries on the Ordered queue to the Final queues
	FOREVER
		{
		iTimerSpinLock.LockIrq();
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
		iTimerSpinLock.UnlockIrq();
		__DEBUG_CALLBACK(0);
		}
	iTimerSpinLock.UnlockIrq();
	__DEBUG_CALLBACK(1);

	// Next transfer entries on the Holding queue to the Ordered queue or final queue
	FOREVER
		{
		iTimerSpinLock.LockIrq();
		if (iHoldingQ.IsEmpty())
			break;
		NTimer* pC=(NTimer*)iHoldingQ.First();
		pC->Deque();
		pC->i_NTimer_iState=NTimer::ETransferring;
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
				iTimerSpinLock.UnlockIrq();
				__DEBUG_CALLBACK(2);

				// we now need to walk ordered queue to find correct position for pC
				SDblQueLink* anchor=&iOrderedQ.iA;
				iCriticalCancelled=FALSE;
				iTimerSpinLock.LockIrq();
				NTimer* pN=(NTimer*)iOrderedQ.First();
				while (pN!=anchor && !iTransferringCancelled)
					{
					if ((pN->iTriggerTime-trigger)<0x80000000u)
						break;	// insert before pN
					pN->i_NTimer_iState=NTimer::ECritical;
					iTimerSpinLock.UnlockIrq();
					__DEBUG_CALLBACK(3);
					iTimerSpinLock.LockIrq();
					if (iCriticalCancelled)
						break;
					pN->i_NTimer_iState=NTimer::EOrdered;
					pN=(NTimer*)pN->iNext;
					}

				if (iTransferringCancelled)
					break;		// this one has been cancelled, go on to next one
				if (!iCriticalCancelled)
					{
					pC->InsertBefore(pN);
					pC->i_NTimer_iState=NTimer::EOrdered;
					break;		// done this one
					}
				}
			}
		iTimerSpinLock.UnlockIrq();
		__DEBUG_CALLBACK(4);
		}
	iTimerSpinLock.UnlockIrq();
	__DEBUG_CALLBACK(5);

	// Finally do call backs for timers which requested DFC callback
	FOREVER
		{
		iTimerSpinLock.LockIrq();
		if (iCompletedQ.IsEmpty())
			break;
		NTimer* pC=(NTimer*)iCompletedQ.First();
		pC->Deque();
		pC->i_NTimer_iState=NTimer::EIdle;
		TAny* p=pC->iPtr;
		NTimerFn f=pC->iFn;
		iTimerSpinLock.UnlockIrq();
		__DEBUG_CALLBACK(7);
		(*f)(p);
		}
	iTimerSpinLock.UnlockIrq();
	__e32_atomic_add_rel32(&iDfcCompleteCount, 2);
	}


/** Tick over the nanokernel timer queue.
	This function should be called by the base port in the system tick timer ISR.
	It should not be called at any other time.
	The value of 'this' to pass is the value returned by NTimerQ::TimerAddress().

	@see NTimerQ::TimerAddress()
 */
EXPORT_C void NTimerQ::Tick()
	{
	TInt irq = iTimerSpinLock.LockIrqSave();
	TInt i = TInt(__e32_atomic_add_rlx64(&iMsCount64, 1)) & ETimerQMask;
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
			{
			doDfc=TRUE;				// if holding queue nonempty, queue DFC to sort
			}
		else if (!iOrderedQ.IsEmpty())
			{
			// if first ordered queue entry expires in <32ms, queue the DFC to transfer
			NTimer* pC=(NTimer*)iOrderedQ.First();
			TUint x = pC->iTriggerTime - iMsCount;
			if (x < (TUint)ENumTimerQueues)
				{
				doDfc=TRUE;
				}
			}
		}
	if (!pQ->iIntQ.IsEmpty())
		{
		// transfer ISR completions to a temporary queue
		// careful here - other CPUs could dequeue timers!
		SDblQue q(&pQ->iIntQ,0);
		for (; !q.IsEmpty(); iTimerSpinLock.LockIrqSave())
			{
			NTimer* pC=(NTimer*)q.First();
			pC->Deque();
			if (pC->IsMutating())
				{
				pC->AddAsDFC();			//mutate NTimer into TDfc and Add() it
				iTimerSpinLock.UnlockIrqRestore(irq);
				continue;
				}
			if (!pC->iFn)
				{
				pC->i_NTimer_iState=NTimer::EIdle;
				iTimerSpinLock.UnlockIrqRestore(irq);
				((TDfc*)(pC->iPtr))->Add();
				continue;
				}
			NSchedulable* tied = pC->iTied;
			if (tied)
				{
				TInt cpu = tied->BeginTiedEvent();
				if (cpu != NKern::CurrentCpu())
					{
					pC->i_NTimer_iState = TUint8(NTimer::EEventQ + cpu);
					TSubScheduler* ss = TheSubSchedulers + cpu;
					TInt kick = ss->QueueEvent(pC);
					iTimerSpinLock.UnlockIrqRestore(irq);
					if (kick)
						send_irq_ipi(ss, kick);
					continue;
					}
				}
			pC->i_NTimer_iState=NTimer::EIdle;
			TAny* p = pC->iPtr;
			NTimerFn f = pC->iFn;
			iTimerSpinLock.UnlockIrqRestore(irq);
			(*f)(p);
			if (tied)
				tied->EndTiedEvent();
			}
		}
	iTimerSpinLock.UnlockIrqRestore(irq);
	if (doDfc)
		iDfc.Add();
	}


/** Mutate an NTimer into a DFC and Add() it

If NTimer state is EFinal, change to DFC state 008n and add to endogenous IDFC
queue for this CPU.

Enter and return with IRQs disabled and timer spin lock held
No need to worry about Cancel()s since timer spin lock is held
Don't touch iHState0

@internalComponent
*/
void NTimer::AddAsDFC()
	{
	TSubScheduler& ss = SubScheduler();
	i8816.iHState16 = (TUint16)(0x80|ss.iCpuNum);
	ss.iDfcs.Add(this);
	ss.iDfcPendingFlag = 1;
	}


/** Check if a nanokernel timer is pending or not

	@return	TRUE if the timer is pending (OneShot() etc. would return KErrInUse)
	@return FALSE if the timer is idle (OneShot() etc. would succeed)
	@pre	Any context

	@publishedPartner
	@prototype
 */
EXPORT_C TBool NTimer::IsPending()
	{
	TUint16 state = i8816.iHState16;
	return state != EIdle;
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
	NTimerQ& m=TheTimerQ;
	TUint32 next=m.iMsCount;	// number of next tick
	TUint32 p=m.iPresent;
	TInt r=KMaxTInt;
	if (p)
		{
		// Final queues nonempty
		TInt nx=next&0x1f;				// number of next tick modulo 32
		p=(p>>nx)|(p<<(32-nx));			// rotate p right by nx (so lsb corresponds to next tick)
		r=__e32_find_ls1_32(p);	// find number of zeros before LS 1
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
	__e32_atomic_add_rlx64(&TheTimerQ.iMsCount64, TUint64(TUint32(aTicks)));
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

