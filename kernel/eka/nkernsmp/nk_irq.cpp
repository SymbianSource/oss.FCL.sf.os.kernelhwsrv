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
// e32\nkernsmp\nk_irq.cpp
// 
//

/**
 @file
 @internalTechnology
*/

#include <e32cmn.h>
#include <e32cmn_private.h>
#include "nk_priv.h"
#include <nk_irq.h>

NIrq		Irq[NK_MAX_IRQS];
NIrqHandler	Handlers[NK_MAX_IRQ_HANDLERS];
NIrqHandler* NIrqHandler::FirstFree;

extern "C" void send_irq_ipi(TSubScheduler*, TInt);

void StepCookie(volatile TUint16& p, TInt n)
	{
	TUint32 x = p<<17;
	while(n--)
		{
		TUint32 y = x;
		x<<=1;
		y^=x;
		x |= ((y>>31)<<17);
		}
	p = (TUint16)(x>>17);
	}

NIrq::NIrq()
	:	iNIrqLock(TSpinLock::EOrderNIrq)
	{
	iIState = EWait;
	iEventsPending = 0;
	iEnabledEvents = 0;
	iHwId = 0;
	iX = 0;
	}

TInt NIrq::BindRaw(NIsr aIsr, TAny* aPtr)
	{
	// Call only from thread context
	TInt r = KErrNone;
	Wait();
	iNIrqLock.LockOnly();
	if (iStaticFlags & EShared)
		{
		r = KErrAccessDenied;
		goto error;
		}
	if ( (iIState & ERaw) || !iHandlers.IsEmpty())
		{
		r = KErrInUse;
		goto error;
		}
	iHandlers.iA.iNext = (SDblQueLink*)aIsr;
	iHandlers.iA.iPrev = (SDblQueLink*)aPtr;
	__e32_atomic_ior_rel32(&iIState, ERaw);
error:
	iNIrqLock.UnlockOnly();
	Done();
	return r;
	}

TInt NIrq::UnbindRaw()
	{
	// Call only from thread context
	TInt r = DisableRaw(TRUE);
	if (r != KErrNone)
		return r;
	Wait();
	iNIrqLock.LockOnly();
	if (iIState & ERaw)
		{
		iHandlers.iA.iNext = 0;
		iHandlers.iA.iPrev = 0;
		++iGeneration;	// release anyone still waiting in Disable()
		__e32_atomic_and_rel32(&iIState, ~(ERaw|EUnbind));
		}
	iNIrqLock.UnlockOnly();
	Done();
	return r;
	}

TInt NIrq::DisableRaw(TBool aUnbind)
	{
	TBool wait = FALSE;
	TInt r = KErrNone;
	TInt irq = __SPIN_LOCK_IRQSAVE(iNIrqLock);
	if (!(iIState & ERaw))
		r = KErrGeneral;
	else
		{
		wait = TRUE;
		if (aUnbind)
			__e32_atomic_ior_acq32(&iIState, EUnbind);
		if (!(iEnabledEvents & 1))
			{
			iEnabledEvents |= 1;
			HwDisable();
//			wait = TRUE;
			}
		}
	__SPIN_UNLOCK_IRQRESTORE(iNIrqLock,irq);
	TInt c = NKern::CurrentContext();
	if (wait && c!=NKern::EInterrupt)
		{
		// wait for currently running handler to finish or interrupt to be reenabled
		if (c==NKern::EThread)
			NKern::ThreadEnterCS();
		HwWaitCpus();	// ensure other CPUs have had a chance to accept any outstanding interrupts
		TUint32 g = iGeneration;
		while ( ((iIState >> 16) || HwPending()) && (iGeneration == g))
			{
			__chill();
			}
		if (c==NKern::EThread)
			NKern::ThreadLeaveCS();
		}
	return r;
	}

TInt NIrq::EnableRaw()
	{
	TInt r = KErrNone;
	TInt irq = __SPIN_LOCK_IRQSAVE(iNIrqLock);
	if (!(iIState & ERaw))
		r = KErrGeneral;
	else if (iIState & EUnbind)
		r = KErrNotReady;
	else if (iEnabledEvents & 1)
		{
		iEnabledEvents = 0;
		HwEnable();
		++iGeneration;
		}
	__SPIN_UNLOCK_IRQRESTORE(iNIrqLock,irq);
	return r;
	}

TInt NIrq::Bind(NIrqHandler* aH)
	{
	// Call only from thread context
	TInt r = KErrInUse;
	Wait();
	if (!(iIState & ERaw))
		{
		r = KErrNone;
		TBool empty = iHandlers.IsEmpty();
		TBool shared = iStaticFlags & EShared;
		TBool exclusive = iIState & NIrqHandler::EExclusive;
		if (!empty)
			{
			if (!shared || exclusive)
				{
				r = KErrAccessDenied;
				goto error;
				}
			NIrqHandler* h = _LOFF(iHandlers.First(), NIrqHandler, iIrqLink);
			if (h->iHState & NIrqHandler::EExclusive)
				{
				r = KErrAccessDenied;
				goto error;
				}
			}
		aH->iIrq = this;
		iHandlers.Add(&aH->iIrqLink);
		}
error:
	Done();
	return r;
	}

void NIrq::HwIsr()
	{
	TRACE_IRQ12(16, this, iVector, iIState);
	TBool eoi_done = FALSE;
	TUint32 rcf0 = EnterIsr();		// for initial run count
	TUint32 rcf1 = iIState;			// might have changed while we were waiting in EnterIsr()
	if (rcf1 & ERaw)
		{
		if (!(rcf1 & EUnbind))
			{
			NIsr f = (NIsr)iHandlers.iA.iNext;
			TAny* p = iHandlers.iA.iPrev;
			(*f)(p);
			}
		HwEoi();
		IsrDone();
		return;
		}
	if (rcf0 >> 16)
		{
		HwEoi();
		return;
		}
	if (!(iStaticFlags & ELevel))
		{
		eoi_done = TRUE;
		HwEoi();
		}
	do	{
		// Handler list can't be touched now
		SDblQueLink* anchor = &iHandlers.iA;
		SDblQueLink* p = anchor->iNext;
		while (p != anchor)
			{
			NIrqHandler* h = _LOFF(p, NIrqHandler, iIrqLink);
			h->Activate(1);
			p = p->iNext;
			}
		if (!eoi_done)
			{
			eoi_done = TRUE;
			HwEoi();
			}
		if ((iStaticFlags & ELevel) && iEventsPending)
			{
			// For a level triggered interrupt make sure interrupt is disabled until
			// all pending event handlers have run, to avoid a continuous interrupt.
			TInt irq = __SPIN_LOCK_IRQSAVE(iNIrqLock);
			if (iEventsPending)
				{
				iEnabledEvents |= 1;
				HwDisable();
				}
			__SPIN_UNLOCK_IRQRESTORE(iNIrqLock,irq);
			}
		} while (IsrDone());
	}

void NIrqHandler::Activate(TInt aCount)
	{
	TUint32 orig = DoActivate(aCount);
	TRACE_IRQ12(17, this, orig, aCount);
	if (orig & (EDisable|EUnbind|EActive))
		return;	// disabled or already active
	NSchedulable* tied = iTied;
	if (tied)
		{
		// we need to enforce mutual exclusion between the event handler
		// and the tied thread or thread group, so the event handler must
		// run on the CPU to which the thread or group is currently attached
		// once the event has been attached to that CPU, the thread/group
		// can't be migrated until the event handler completes.
		// need a pending event count for the tied thread/group
		// so we know when the thread/group can be migrated
		TInt tied_cpu = tied->BeginTiedEvent();
		TInt this_cpu = NKern::CurrentCpu();
		if (tied_cpu != this_cpu)
			{
			__e32_atomic_add_acq32(&iIrq->iEventsPending, 1);
			TheSubSchedulers[tied_cpu].QueueEventAndKick(this);
			// FIXME: move IRQ over to tied CPU if this is the only handler for that IRQ
			//			what to do about shared IRQs?
			return;
			}
		}
	// event can run on this CPU so run it now
	if (aCount)
		{
		orig = EventBegin();
		TRACE_IRQ8(18, this, orig);
		(*iFn)(iPtr);
		orig = EventDone();
		TRACE_IRQ8(19, this, orig);
		if (!(orig & EActive))
			{
			if (tied)
				tied->EndTiedEvent();
			return;	// that was last occurrence or event now disabled
			}
		}
	__e32_atomic_add_ord32(&iIrq->iEventsPending, 1);
//	add event to this cpu
	SubScheduler().QueueEventAndKick(this);
	}


NIrqHandler::NIrqHandler()
	{
	iIrqLink.iNext = 0;
	iIrq = 0;
	iTied = 0;
	iHState = EDisable|EBind|ENotReady|EEventHandlerIrq;
	iFn = 0;
	iPtr = 0;
	memclr(iNIrqHandlerSpare, sizeof(iNIrqHandlerSpare));
	}

void NIrqHandler::Free()
	{
	NKern::Lock();
	NEventHandler::TiedLock.LockOnly();
	if (!iTied)	// Only free if iTied has been cleared
		{
		iIrqLink.iNext = FirstFree;
		FirstFree = this;
		}
	NEventHandler::TiedLock.UnlockOnly();
	NKern::Unlock();
	}

NIrqHandler* NIrqHandler::Alloc()
	{
	NKern::Lock();
	NEventHandler::TiedLock.LockOnly();
	NIrqHandler* p = FirstFree;
	if (p)
		FirstFree = (NIrqHandler*)p->iIrqLink.iNext;
	NEventHandler::TiedLock.UnlockOnly();
	NKern::Unlock();
	if (p)
		new (p) NIrqHandler();
	return p;
	}

TInt NIrqHandler::Enable(TInt aHandle)
	{
	// call from any context
	TBool reactivate = FALSE;
	TInt r = KErrNotReady;
	NIrq* pI = iIrq;
	if (!pI)
		return KErrNotReady;
	TInt irq = __SPIN_LOCK_IRQSAVE(pI->iNIrqLock);	// OK since NIrq's are never deleted
	if (iIrq==pI && TUint(aHandle)==iHandle)	// check handler not unbound
		{
		TUint32 orig = DoSetEnabled();	// clear EDisable and EBind provided neither EUnbind nor ENotReady set
		if (!(orig & (EUnbind|ENotReady)))
			{
			r = KErrNone;
			if (orig & EDisable)	// check not already enabled
				{
				++iGeneration;
				TUint32 n = pI->iEnabledEvents;
				pI->iEnabledEvents += 2;
				if (n==0)
					pI->HwEnable();	// enable HW interrupt if this is first handler to be enabled
				if ((orig >> 16) && !(orig & EActive))
					// replay remembered interrupt(s)
					reactivate = TRUE;
				}
			}
		}
	if (reactivate)
		{
		pI->iNIrqLock.UnlockOnly();
		Activate(0);
		pI->iNIrqLock.LockOnly();
		}
	__SPIN_UNLOCK_IRQRESTORE(pI->iNIrqLock,irq);
	return r;
	}

TInt NIrqHandler::Disable(TBool aUnbind, TInt aHandle)
	{
	// call from any context
	NIrq* pI = iIrq;
	if (!pI)
		return KErrGeneral;
	TInt irq = __SPIN_LOCK_IRQSAVE(pI->iNIrqLock);	// OK since NIrq's are never deleted
	if (iIrq != pI || TUint(aHandle)!=iHandle)	// check handler not unbound
		{
		__SPIN_UNLOCK_IRQRESTORE(pI->iNIrqLock,irq);
		return KErrGeneral;
		}
	TInt r = aUnbind ? KErrGeneral : KErrNone;
	TUint32 f = aUnbind ? EUnbind|EDisable : EDisable;
	TUint32 orig = __e32_atomic_ior_acq32(&iHState, f);
	TUint32 g = iGeneration;
	if (!(orig & EDisable))	// check not already disabled
		{
		pI->iEnabledEvents -= 2;
		if (!pI->iEnabledEvents)
			pI->HwDisable();	// disable HW interrupt if no more enabled handlers
		}
	if (aUnbind && !(orig & EUnbind))
		{
		volatile TUint16& cookie = *(volatile TUint16*)(((TUint8*)&iHandle)+2);
		StepCookie(cookie, 1);
		r = KErrNone;
		}
	__SPIN_UNLOCK_IRQRESTORE(pI->iNIrqLock,irq);
	if (NKern::CurrentContext() != NKern::EInterrupt)
		{
		// wait for currently running handler to finish or interrupt to be reenabled
 		while ((iHState & EActive) && (iGeneration == g))
			{
			__chill();
			}
		}
	return r;
	}

TInt NIrqHandler::Unbind(TInt aId, NSchedulable* aTied)
	{
	TInt r = Disable(TRUE, aId);	// waits for any current activation of ISR to finish
	if (r==KErrNone || aTied)	// returns KErrGeneral if someone else already unbound this interrupt handler
		{
		// Possible race condition here between tied thread termination and interrupt unbind.
		// We need to be sure that the iTied field must be NULL before the tied thread/group
		// is destroyed.
		NKern::Lock();
		NEventHandler::TiedLock.LockOnly();	// this guarantees pH->iTied cannot change
		NSchedulable* t = iTied;
		if (t)
			{
			// We need to guarantee the object pointed to by t cannot be deleted until we
			// have finished with it.
			t->AcqSLock();
			if (iTiedLink.iNext)
				{
				iTiedLink.Deque();
				iTiedLink.iNext = 0;
				iTied = 0;
				}
			if (aTied && aTied==t)
				iTied = 0;
			t->RelSLock();
			}
		NEventHandler::TiedLock.UnlockOnly();
		NKern::Unlock();
		}
	if (r==KErrNone)
		{
		DoUnbind();
		Free();
		}
	return r;
	}

void NIrqHandler::DoUnbind()
	{
	// Call only from thread context
	NIrq* pI = iIrq;
	pI->Wait();
	iIrqLink.Deque();
	iIrq = 0;
	pI->Done();
	}

TInt TSubScheduler::QueueEvent(NEventHandler* aEvent)
	{
	TInt r = 0;
	TInt irq = __SPIN_LOCK_IRQSAVE(iEventHandlerLock);
	if (!(iScheduler->iIpiAcceptCpus & iCpuMask))
		r = EQueueEvent_WakeUp;
	else if (!iEventHandlersPending)
		r = EQueueEvent_Kick;
	iEventHandlersPending = TRUE;
	iEventHandlers.Add(aEvent);
	__SPIN_UNLOCK_IRQRESTORE(iEventHandlerLock,irq);
	return r;
	}

void TSubScheduler::QueueEventAndKick(NEventHandler* aEvent)
	{
	TInt kick = QueueEvent(aEvent);
	if (kick)
		{
		// extra barrier ?
		send_irq_ipi(this, kick);
		}
	}

extern "C" void run_event_handlers(TSubScheduler* aS)
	{
	while (aS->iEventHandlersPending)
		{
		TInt irq = __SPIN_LOCK_IRQSAVE(aS->iEventHandlerLock);
		if (aS->iEventHandlers.IsEmpty())
			{
			aS->iEventHandlersPending = FALSE;
			__SPIN_UNLOCK_IRQRESTORE(aS->iEventHandlerLock, irq);
			break;
			}
		NIrqHandler* h = (NIrqHandler*)aS->iEventHandlers.First()->Deque();
		if (aS->iEventHandlers.IsEmpty())
			aS->iEventHandlersPending = FALSE;
		TInt type = h->iHType;
		NSchedulable* tied = h->iTied;
		if (type == NEventHandler::EEventHandlerNTimer)
			{
			NEventFn f = h->iFn;
			TAny* p = h->iPtr;
			mb();	// make sure dequeue observed and iFn,iPtr,iTied sampled before state change observed
			h->i8888.iHState1 = NTimer::EIdle; // can't touch timer again after this
			__SPIN_UNLOCK_IRQRESTORE(aS->iEventHandlerLock, irq);
			(*f)(p);
			if (tied)
				tied->EndTiedEvent();
			continue;
			}
		__SPIN_UNLOCK_IRQRESTORE(aS->iEventHandlerLock, irq);
		TBool requeue = TRUE;
		switch (h->iHType)
			{
			case NEventHandler::EEventHandlerIrq:
				{
				TUint32 orig;
				// event can run on this CPU so run it now
				// if event tied, migration of tied thread/group will have been blocked
				orig = h->EventBegin();
				TRACE_IRQ8(20, h, orig);
				(*h->iFn)(h->iPtr);
				TRACE_IRQ4(21, h);
				if (!(h->iHState & NIrqHandler::ERunCountMask))	// if run count still nonzero, definitely still active
					{
					NIrq* pI = h->iIrq;
					irq = __SPIN_LOCK_IRQSAVE(pI->iNIrqLock);
					orig = h->EventDone();
					TRACE_IRQ8(22, h, orig);
					if (!(orig & NIrqHandler::EActive))
						{
						// handler is no longer active - can't touch it again
						// pI is OK since NIrq's are never deleted/reused
						requeue = FALSE;
						if (__e32_atomic_add_rel32(&pI->iEventsPending, TUint32(-1)) == 1)
							{
							if (pI->iEnabledEvents & 1)
								{
								pI->iEnabledEvents &= ~1;
								if (pI->iEnabledEvents)
									pI->HwEnable();
								}
							}
						}
					__SPIN_UNLOCK_IRQRESTORE(pI->iNIrqLock,irq);
					}
				break;
				}
			default:
				__KTRACE_OPT(KPANIC,DEBUGPRINT("h=%08x",h));
				__NK_ASSERT_ALWAYS(0);
			}
		if (tied && !requeue)
			{
			// If the tied thread/group has no more tied events outstanding
			// and has a migration pending, trigger the migration now.
			// Atomically change the tied_cpu to the target CPU here. An IDFC
			// can then effect the migration.
			// Note that the tied code can't run in parallel with us until
			// the tied_cpu is changed. However it could run as soon as the
			// tied_cpu is changed (e.g. if added to ready list after change)
			tied->EndTiedEvent();
			}
		if (requeue)
			{
			// still pending so put it back on the queue
			// leave interrupt disabled (if so) and migration of tied thread/group blocked
			aS->QueueEvent(h);
			}
		}
	}

/******************************************************************************
 * Public interrupt management functions
 ******************************************************************************/

void NKern::InterruptInit0()
	 {
	 TInt i;
	 TUint16 cookie = 1;
	 NIrqHandler::FirstFree = 0;
	 for (i=NK_MAX_IRQ_HANDLERS-1; i>=0; --i)
		 {
		 StepCookie(cookie, 61);
		 NIrqHandler* h = &::Handlers[i];
		__KTRACE_OPT(KBOOT,DEBUGPRINT("NIrqHandler[%d] at %08x", i, h));
		 h->iGeneration = 0;
		 h->iHandle = (cookie << 16) | i;
		 h->iIrqLink.iNext = NIrqHandler::FirstFree;
		 NIrqHandler::FirstFree = h;
		 }
	 NIrq::HwInit0();
	 }

EXPORT_C TInt NKern::InterruptInit(TInt aId, TUint32 aFlags, TInt aVector, TUint32 aHwId, TAny* aExt)
	{
	__KTRACE_OPT(KBOOT,DEBUGPRINT("NKII: ID=%02x F=%08x V=%03x HWID=%08x X=%08x", aId, aFlags, aVector, aHwId, aExt));
	TRACE_IRQ12(0, (aId|(aVector<<16)), aFlags, aHwId);
	if (TUint(aId) >= TUint(NK_MAX_IRQS))
  		return KErrArgument;
	NIrq* pI = &Irq[aId];
	__KTRACE_OPT(KBOOT,DEBUGPRINT("NIrq[%02x] at %08x", aId, pI));
	TRACE_IRQ8(1, aId, pI);
	new (pI) NIrq;
	pI->iX = (NIrqX*)aExt;
	pI->iIndex = (TUint16)aId;
	pI->iHwId = aHwId;
	pI->iVector = aVector;
	pI->iStaticFlags = (TUint16)(aFlags & 0x13);
	if (aFlags & NKern::EIrqInit_Count)
		pI->iIState |= NIrq::ECount;
	pI->HwInit();
	__e32_atomic_and_rel32(&pI->iIState, ~NIrq::EWait);
	return KErrNone;
	}

EXPORT_C TInt NKern::InterruptBind(TInt aId, NIsr aIsr, TAny* aPtr, TUint32 aFlags, NSchedulable* aTied)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT(">NKIB: ID=%02x ISR=%08x(%08x) F=%08x T=%T", aId, aIsr, aPtr, aFlags, aTied));
	TRACE_IRQ12(2, aId, aIsr, aPtr);
	TRACE_IRQ12(3, aId, aFlags, aTied);
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"NKern::InterruptBind");
	if (TUint(aId) >= TUint(NK_MAX_IRQS))
		{
		TRACE_IRQ8(4, aId, KErrArgument);
		return KErrArgument;
		}
	NIrq* pI = &Irq[aId];
	NIrqHandler* pH = 0;
	NSchedulable* pT = 0;
	if (aFlags & NKern::EIrqBind_Tied)
		{
		if (!aTied)
			aTied = NKern::CurrentThread();
		pT = aTied;
		}
	TInt r = KErrNoMemory;
	TInt handle = 0;
	NKern::ThreadEnterCS();
	if (!(aFlags & NKern::EIrqBind_Raw))
		{
		pH = NIrqHandler::Alloc();
		if (!pH)
			goto out;
		pH->iFn = aIsr;
		pH->iPtr = aPtr;
		__e32_atomic_add_ord32(&pH->iGeneration, 1);
		if (aFlags & EIrqBind_Exclusive)
			pH->iHState |= NIrqHandler::EExclusive;
		if (aFlags & EIrqBind_Count)
			pH->iHState |= NIrqHandler::ECount;
		r = pI->Bind(pH);
		if (r==KErrNone)
			{
			handle = pH->iHandle;
			// We assume that aTied cannot disappear entirely before we return
			if (pT)
				{
				NKern::Lock();
				r = pT->AddTiedEvent(pH);
				NKern::Unlock();
				}
			if (r!=KErrNone)
				{
				// unbind
				pH->DoUnbind();
				}
			}
		if (r!=KErrNone)
			pH->Free();
		}
	else
		{
		if (aFlags & NKern::EIrqBind_Tied)
			r = KErrNotSupported;
		else
			r = pI->BindRaw(aIsr, aPtr);
		}
out:
	if (r==KErrNone)
		{
		// clear ENotReady so handler can be enabled
		__e32_atomic_and_rel32(&pH->iHState, ~NIrqHandler::ENotReady);
		r = handle;
		}
	NKern::ThreadLeaveCS();
	__KTRACE_OPT(KNKERN,DEBUGPRINT("<NKIB: %08x", r));
	TRACE_IRQ8(4, aId, r);
	return r;
	}

TInt NIrq::FromHandle(TInt& aHandle, NIrq*& aIrq, NIrqHandler*& aHandler)
	{
	TRACE_IRQ4(5, aHandle);
	aIrq = 0;
	aHandler = 0;
	NIrqHandler* pH = 0;
	NIrqHandler* pH2 = 0;
	NIrq* pI = 0;
	SDblQueLink* anchor = 0;
	TUint32 i;
	TInt r = KErrArgument;
	if (aHandle & NKern::EIrqCookieMask)
		{
		i = aHandle & NKern::EIrqIndexMask;
		if (i>=NK_MAX_IRQ_HANDLERS)
			goto out;
		pH = &::Handlers[i];
		if (pH->iHandle != TUint(aHandle))
			goto out;
		aHandler = pH;
		aIrq = pH->iIrq;
		r = KErrNone;
		goto out;
		}
	if (TUint32(aHandle)>=NK_MAX_IRQS)
		goto out;
	pI = &::Irq[aHandle];
	if (pI->iIState & NIrq::ERaw)
		{
		aIrq = pI;
		r = KErrNone;
		goto out;
		}
	if (pI->iStaticFlags & NIrq::EShared)
		goto out;
	anchor = &pI->iHandlers.iA;
	pH = _LOFF(anchor->iNext, NIrqHandler, iIrqLink);
	i = pH - ::Handlers;
	if (i>=NK_MAX_IRQ_HANDLERS)
		goto out;
	pH2 = &::Handlers[i];
	if (pH2 != pH)
		goto out;
	if (pH->iIrq != pI || anchor->iPrev != anchor->iNext)
		goto out;
	aHandle = pH->iHandle;
	aHandler = pH;
	aIrq = pI;
	r = KErrNone;
out:
	TRACE_IRQ4(6, r);
	TRACE_IRQ12(7, aHandle, aIrq, aHandler);
	return r;
	}

EXPORT_C TInt NKern::InterruptUnbind(TInt aId)
	{
	TRACE_IRQ4(8, aId);
	__KTRACE_OPT(KNKERN,DEBUGPRINT(">NKIU: ID=%08x", aId));
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"NKern::InterruptUnbind");
	NIrq* pI;
	NIrqHandler* pH;
	TInt r = NIrq::FromHandle(aId, pI, pH);
	if (r!=KErrNone)
		return r;
	NKern::ThreadEnterCS();
	if (!pH)
		{
		// raw ISR
		r = pI->UnbindRaw();
		}
	else
		{
		r = pH->Unbind(aId, 0);
		}
	NKern::ThreadLeaveCS();
	TRACE_IRQ4(9, r);
	return r;
	}

EXPORT_C TInt NKern::InterruptEnable(TInt aId)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT(">NKIE: ID=%08x", aId));
	TRACE_IRQ4(10, aId);
	NIrq* pI;
	NIrqHandler* pH;
	TInt r = NIrq::FromHandle(aId, pI, pH);
	if (r==KErrNone)
		r = pH ? pH->Enable(aId) : pI->EnableRaw();
	TRACE_IRQ4(11, r);
	return r;
	}

EXPORT_C TInt NKern::InterruptDisable(TInt aId)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT(">NKID: ID=%08x", aId));
	TRACE_IRQ4(12, aId);
	NIrq* pI;
	NIrqHandler* pH;
	TInt r = NIrq::FromHandle(aId, pI, pH);
	if (r==KErrNone)
		r = pH ? pH->Disable(FALSE, aId) : pI->DisableRaw(FALSE);
	TRACE_IRQ4(13, r);
	return r;
	}

EXPORT_C TInt NKern::InterruptClear(TInt aId)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT(">NKIC: ID=%08x", aId));
	return KErrNotSupported;
	}

EXPORT_C TInt NKern::InterruptSetPriority(TInt aId, TInt aPri)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT(">NKIS: ID=%08x PRI=%08x", aId, aPri));
	return KErrNotSupported;
	}

EXPORT_C TInt NKern::InterruptSetCpuMask(TInt aId, TUint32 aMask)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT(">NKIM: ID=%08x M=%08x", aId, aMask));
	return KErrNotSupported;
	}

EXPORT_C void NKern::Interrupt(TInt aId)
	{
	__NK_ASSERT_ALWAYS(TUint(aId) < TUint(NK_MAX_IRQS));
	NIrq* pI = &Irq[aId];
	pI->HwIsr();
	}

