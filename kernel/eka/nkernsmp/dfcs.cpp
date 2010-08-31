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
// e32\nkernsmp\dfcs.cpp
// DFCs
// 
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

// TDfc member data
#define __INCLUDE_TDFC_DEFINES__

#include "nk_priv.h"

extern "C" void send_self_resched_ipi();

/** Construct an IDFC

	@param aFunction = function to call
	@param aPtr = parameter to be passed to function
 */
EXPORT_C TDfc::TDfc(TDfcFn aFunction, TAny* aPtr)
	{
	iPtr = aPtr;
	iFn = aFunction;
	iTied = 0;
	iHType = EEventHandlerIDFC;
	i8888.iHState0 = 0;
	i8888.iHState1 = 0;
	i8888.iHState2 = 0;
	iTiedLink.iNext = 0;
	}


/** Construct an IDFC tied to a thread or group

	@param aTied = pointer to thread or group to which IDFC should be tied
	@param aFunction = function to call
	@param aPtr = parameter to be passed to function

	@pre Call in thread context, interrupts enabled
 */
EXPORT_C TDfc::TDfc(NSchedulable* aTied, TDfcFn aFunction, TAny* aPtr)
	{
	iPtr = aPtr;
	iFn = aFunction;
	iTied = 0;
	iHType = EEventHandlerIDFC;
	i8888.iHState0 = 0;
	i8888.iHState1 = 0;
	i8888.iHState2 = 0;
	iTiedLink.iNext = 0;
	if (aTied)
		{
		SetTied(aTied);
		}
	}


/** Construct a DFC without specifying a DFC queue.
	The DFC queue must be set before the DFC may be queued.

	@param aFunction = function to call
	@param aPtr = parameter to be passed to function
	@param aPriority = priority of DFC within the queue (0 to 7, where 7 is highest)
 */
EXPORT_C TDfc::TDfc(TDfcFn aFunction, TAny* aPtr, TInt aPriority)
	{
	__NK_ASSERT_DEBUG((TUint)aPriority<(TUint)KNumDfcPriorities);
	iPtr = aPtr;
	iFn = aFunction;
	iTied = 0;
	iHType = TUint8(aPriority);
	i8888.iHState0 = 0;
	i8888.iHState1 = 0;
	i8888.iHState2 = 0;
	iTiedLink.iNext = 0;
	}


/** Construct a DFC specifying a DFC queue.

	@param aFunction = function to call
	@param aPtr = parameter to be passed to function
	@param aDfcQ = pointer to DFC queue which this DFC should use
	@param aPriority = priority of DFC within the queue (0-7)
 */
EXPORT_C TDfc::TDfc(TDfcFn aFunction, TAny* aPtr, TDfcQue* aDfcQ, TInt aPriority)
	{
	__NK_ASSERT_DEBUG((TUint)aPriority<(TUint)KNumDfcPriorities);
	iPtr = aPtr;
	iFn = aFunction;
	iDfcQ = aDfcQ;
	iHType = TUint8(aPriority);
	i8888.iHState0 = 0;
	i8888.iHState1 = 0;
	i8888.iHState2 = 0;
	iTiedLink.iNext = 0;
	}


/** Tie an IDFC to a thread or group

	@param	aTied = pointer to thread or group to which IDFC should be tied
	@return	KErrNone if successful
	@return	KErrDied if thread has exited or group has been destroyed.

	@pre Call in thread context, interrupts enabled
	@pre Must be IDFC not DFC
	@pre IDFC must not be queued or running
	@pre IDFC must not already be tied
 */
EXPORT_C TInt TDfc::SetTied(NSchedulable* aTied)
	{
	__NK_ASSERT_ALWAYS(IsIDFC() && i8816.iHState16==0);
	__NK_ASSERT_ALWAYS(aTied && !iTied);
	NKern::Lock();
	TInt r = aTied->AddTiedEvent(this);
	__NK_ASSERT_ALWAYS(r==KErrNone || r==KErrDied);
	NKern::Unlock();
	return r;
	}


/** Destroy a DFC or IDFC

	@pre Call from thread context with interrupts and preemption enabled
	@pre Calling thread holds no fast mutex
	@pre Calling thread in critical section
 */
EXPORT_C TDfc::~TDfc()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TDfc::~TDfc");
	NKern::Lock();
	NEventHandler::TiedLock.LockOnly();
	NSchedulable* tied = iTied;
	if (IsDFC() || (IsIDFC() && !tied))
		{
		Cancel();
		iHType = (TUint8)EEventHandlerDummy;
		}
	if (IsIDFC())
		{
		__NK_ASSERT_ALWAYS(tied!=0);
		tied->AcqSLock();
		if (iTiedLink.iNext)
			{
			iTiedLink.Deque();
			iTiedLink.iNext = 0;
			}
		tied->RelSLock();
		Cancel();
		iHType = (TUint8)EEventHandlerDummy;
		iTied = 0;
		}
	NEventHandler::TiedLock.UnlockOnly();
	NKern::Unlock();
	}


/** Construct a DFC queue
	Kern::DfcQInit() should be called on the new DFC queue before it can be used.
 */
EXPORT_C TDfcQue::TDfcQue()
	: iThread(NULL)
	{}



/** Queue an IDFC or a DFC from an ISR

	This function is the only way to queue an IDFC and is the only way to queue
	a DFC from an ISR. To queue a DFC from an IDFC or a thread either Enque()
	or DoEnque() should be used.

	This function does nothing if the IDFC/DFC is already queued.

	@pre Call only from ISR, IDFC or thread with preemption disabled.
	@pre Do not call from thread with preemption enabled.
	@return	TRUE if DFC was actually queued by this call
			FALSE if DFC was already queued on entry so this call did nothing

	@see TDfc::DoEnque()
	@see TDfc::Enque()
 */
EXPORT_C TBool TDfc::Add()
	{
	__ASSERT_DEBUG(NKern::CurrentContext()!=NKern::EThread || NKern::KernelLocked(), *(int*)0xdfcadd01=0);
	__ASSERT_DEBUG(IsIDFC() || (IsDFC() && iDfcQ), *(int*)0xdfcadd03=0);
//	__ASSERT_WITH_MESSAGE_DEBUG(  NKern::CurrentContext()!=NKern::EThread  ||  NKern::KernelLocked(),"Do not call from thread with preemption enabled","TDfc::Add");
//	__ASSERT_WITH_MESSAGE_DEBUG(  IsIDFC() || (IsDFC() && iDfcQ), "DFC queue not set", "TDfc::Add");
#ifdef __WINS__
	__NK_ASSERT_ALWAYS(Interrupt.InInterrupt() || NKern::KernelLocked());
#endif
	TInt irq = NKern::DisableAllInterrupts();
	TSubScheduler& ss = SubScheduler();
	TUint32 orig = 0xFF00;

	// Transition the state to 'on normal IDFC queue'
	// 0000->008n
	// 00Cn->00En
	// All other states unchanged
	// Return original state
	if (IsValid())	// don't add if tied and tied thread/group is being/has been destroyed
		orig = AddStateChange();
	if (orig==0)
		{
		// wasn't already queued
		i8888.iHState0 = 0;	// BeginTiedEvent() not done
		ss.iDfcs.Add(this);
		ss.iDfcPendingFlag = 1;
#ifdef _DEBUG
		TUint32 st8 = DFC_STATE(this) & 0xFF;
		if (st8 != (0x80|ss.iCpuNum))
			__crash();
#endif
		}
	NKern::RestoreInterrupts(irq);
	return (orig==0 || (orig&0xFFE0)==0x00C0);
	}


/** Queue an IDFC or a DFC from any context

	This function is identical to TDfc::Add() but no checks are performed for correct usage,
	and it contains no instrumentation code.

	@return	TRUE if DFC was actually queued by this call
			FALSE if DFC was already queued on entry so this call did nothing

	@see TDfc::DoEnque()
	@see TDfc::Enque()
	@see TDfc::Add()
 */
EXPORT_C TBool TDfc::RawAdd()
	{
	TInt irq = NKern::DisableAllInterrupts();
	TSubScheduler& ss = SubScheduler();
	TUint32 orig = 0xFF00;
	if (IsValid())	// don't add if tied and tied thread/group is being/has been destroyed
		orig = AddStateChange();
	if (orig==0)
		{
		// wasn't already queued
		i8888.iHState0 = 0;	// BeginTiedEvent() not done
		ss.iDfcs.Add(this);
		ss.iDfcPendingFlag = 1;
		send_self_resched_ipi();	// ensure current CPU runs the DFC
#ifdef _DEBUG
		TUint32 st8 = DFC_STATE(this) & 0xFF;
		if (st8 != (0x80|ss.iCpuNum))
			__crash();
#endif
		// FIXME: Need to wait to ensure IRQ is active before reenabling interrupts
		}
	NKern::RestoreInterrupts(irq);
	return (orig==0 || (orig&0xFFE0)==0x00C0);
	}


/** Queue a DFC (not an IDFC) from an IDFC or thread with preemption disabled.

	This function is the preferred way to queue a DFC from an IDFC. It should not
	be used to queue an IDFC - use TDfc::Add() for this.

	This function does nothing if the DFC is already queued.

	@pre Call only from IDFC or thread with preemption disabled.
	@pre Do not call from ISR or thread with preemption enabled.
	@return	TRUE if DFC was actually queued by this call
			FALSE if DFC was already queued on entry so this call did nothing

	@see TDfc::Add()
	@see TDfc::Enque()
 */
EXPORT_C TBool TDfc::DoEnque()
	{
	__ASSERT_WITH_MESSAGE_DEBUG(  (NKern::CurrentContext()==NKern::EIDFC )||( NKern::CurrentContext()==NKern::EThread && NKern::KernelLocked()),"Do not call from ISR or thread with preemption enabled","TDfc::DoEnque");
	__NK_ASSERT_DEBUG(IsDFC());
	__ASSERT_WITH_MESSAGE_DEBUG(iDfcQ, "DFC queue not set", "TDfc::DoEnque");

	// Check not already queued and then mark queued to prevent ISRs touching this DFC
	TDfcQue* q = iDfcQ;
	NThreadBase* t = q->iThread;
	t->AcqSLock();	// also protects DFC queue
	TUint16 expect = 0;
	TBool ok = __e32_atomic_cas_acq16(&iDfcState, &expect, 1);
	if (ok)
		{
		// wasn't already queued, now marked as on final queue, which means
		// attempts to cancel will block on the thread spin lock
		TUint present = q->iPresent[0];
		q->Add((TPriListLink*)this);
		if (!present)
			t->iWaitState.UnBlockT(NThreadBase::EWaitDfc, q, KErrNone);
		}
	t->RelSLock();	// also protects DFC queue
	return ok;
	}

void TDfcQue::ThreadFunction(TAny* aDfcQ)
	{
	TDfcQue& q = *(TDfcQue*)aDfcQ;
	NThreadBase* t = NKern::CurrentThread();
	FOREVER
		{
		NKern::Lock();
		t->AcqSLock();	// also protects DFC queue
		if (q.IsEmpty())
			{
			t->iWaitState.SetUpWait(NThreadBase::EWaitDfc, 0, &q);
			RescheduleNeeded();
			t->RelSLock();	// also protects DFC queue
			NKern::Unlock();
			}
		else
			{
			TDfc* d = q.First();
			q.Remove((TPriListLink*)d);
			TDfcFn f = d->iFn;
			TAny* p = d->iPtr;
			d->ResetState();
			t->RelSLock();	// also protects DFC queue
			NKern::Unlock();
			(*f)(p);
			}
		}
	}



void TCancelIPI::Send(TDfc* aDfc, TInt aCpu)
	{
	iDfc = aDfc;
	Queue(&Isr, 1u<<aCpu);
	}

void TCancelIPI::Isr(TGenericIPI* aIPI)
	{
	TCancelIPI* p = (TCancelIPI*)aIPI;
	TDfc* d = p->iDfc;
	if (d->iNext)
		{
		// QueueDfcs() hasn't dequeued it yet
		// just dequeue it here and reset the state - QueueDfcs() will never see it
		// Note that this means we have to release the tied thread/group if necessary
		// BeginTiedEvent() has occurred if iHState0 is set and it's actually an IDFC not an NTimer
		NSchedulable* tied = (d->iHType==NEventHandler::EEventHandlerIDFC && d->i8888.iHState0) ? d->iTied : 0;
		d->Deque();
		d->ResetState();
		if (tied)
			tied->EndTiedEvent();
		}
	else
		{
		// QueueDfcs() has already dequeued it
		// state transition:
		//		XXYY->XX00
		//		XX00->0000
		// QueueDfcs() will take care of the tied thread/group
		d->CancelFinalStateChange();
		}
	}


/** Cancels an IDFC or DFC.

	This function does nothing if the IDFC or DFC is not queued.

	For any DFC or IDFC the following identity holds:
			Number of times Add() is called and returns TRUE
		+	Number of times DoEnque() is called and returns TRUE
		+	Number of times Enque() is called and returns TRUE
		+	Number of times QueueOnIdle() is called and returns TRUE
		=	Number of times Cancel() is called and returns TRUE
		+	Number of times the DFC/IDFC function executes

	@pre IDFC or thread context. Do not call from ISRs.

	@pre If the DFC function accesses the DFC object itself, the user must ensure that
	     Cancel() cannot be called while the DFC function is running.

	@return	TRUE	if the DFC was actually dequeued by this call - i.e. an
					instance of the DFC's execution has been prevented. It
					is still possible that a previous execution is still in
					progress.
			FALSE	if the DFC was not queued on entry to the call, or was in
					the process of being executed or cancelled. In this case
					it is possible that the DFC executes after this call
					returns.

	@post	However in either case it is safe to delete the DFC object on
			return from this call provided only that the DFC function does not
			refer to the DFC object itself.
 */
EXPORT_C TBool TDfc::Cancel()
	{
	enum TAction { EDeque=1, EReset=2, EIdleUnlock=4, ESendIPI=8, EWait=16 };

	CHECK_PRECONDITIONS(MASK_NOT_ISR|MASK_INTERRUPTS_ENABLED,"TDfc::Cancel");
	if (!iDfcState)
		return FALSE;
	TUint action = EIdleUnlock;
	TBool ret = FALSE;
	TInt cpu = -1;
	NSchedulable* tied = 0;
	TDfcQue* q = 0;
	NThreadBase* t = 0;
	NKern::Lock();
	TSubScheduler& ss0 = SubScheduler();
	if (IsDFC())
		q = iDfcQ, t = q->iThread, t->AcqSLock();
	TInt irq = NKern::DisableAllInterrupts();
	TheScheduler.iIdleSpinLock.LockOnly();

	// 0000->0000, XX00->ZZ00, xxYY->zzYY
	TUint state = CancelInitialStateChange();
	TUint stt = state >> 5;
	if (state & 0xFF00)
		{
		// someone else cancelling at the same time - just wait for them to finish
		action = EWait|EIdleUnlock;
		goto end;
		}
	if (state == 0)	// DFC not active
		goto end;

	// possible states here are 0001, 002g, 006m, 008m, 00Am, 00Cm, 00Em
	ret = (stt!=6);	// if running but not pending, Cancel() will not have prevented an execution
	if (state == TUint(TheScheduler.iIdleGeneration | 0x20))
		{
		// was on idle queue, BeginTiedEvent() isn't called until QueueDfcs() runs
		action = EDeque|EReset|EIdleUnlock;
		goto end;
		}
	if (state == 1)
		{
		// was on final queue, must be DFC not IDFC
		q->Remove((TPriListLink*)this);
		action = EReset|EIdleUnlock;
		goto end;
		}

	// possible states here are 002g (spilled), 006m, 008m, 00Am, 00Cm, 00Em
	// i.e. either on IDFC queue, ExIDFC queue or running
	// For IDFCs, tied thread/group is now in play.
	cpu = state & 0x1f;	// CPU it's on for states 006m, 008m, 00Am, 00Cm, 00Em
	if (stt==3 || stt==6 || stt==7)
		{
		// It's actually running - must be IDFC. A re-queue may also be pending.
		TheScheduler.iIdleSpinLock.UnlockOnly();
		TSubScheduler* ss = TheSubSchedulers + cpu;
		TDfc* expect = this;
		TBool done = __e32_atomic_cas_acq_ptr(&ss->iCurrentIDFC, &expect, 0);
		if (done)
			{
			// We cleared iCurrentIDFC so QueueDfcs() won't touch this again - we reset the state and finish up
			// We must also release the tied thread/group
			tied = iTied;
			action = EReset;
			goto end;
			}
		// QueueDfcs() got to iCurrentIDFC before we did, so we interlock with it
		// and we can leave the EndTiedEvent to it as well
		// State transition:
		//		XXAm->XX00, wait
		//		XX00->0000, don't wait
		TUint32 orig = CancelFinalStateChange() & 0xFF;
		__NK_ASSERT_ALWAYS(orig==0 || orig==state);
		action = orig ? EWait : 0;
		goto end;
		}

	// possible states here 002g (propagated), 008m, 00Am so it's either on the endogenous or exogenous IDFC queue
	if (stt==5)
		{
		// it's on the exogenous IDFC queue
		TheScheduler.iIdleSpinLock.UnlockOnly();
		TSubScheduler* ss = TheSubSchedulers + cpu;
		ss->iExIDfcLock.LockOnly();
		if (iNext)
			{
			// we got to it before QueueDfcs() on the other CPU so we can finish up here
			// QueueDfcs() will never see it again so we must release tied thread/group
			Deque();
			tied = iTied;
			ss->iExIDfcLock.UnlockOnly();
			action = EReset;
			goto end;
			}
		// QueueDfcs() on other CPU has already dequeued it - we must now interlock with RunIDFCStateChange()
		ss->iExIDfcLock.UnlockOnly();
		// State transition:
		//		XXAm->XX00, wait
		//		XX00->0000, don't wait
		// QueueDfcs() will take care of tied thread/group
		TUint32 orig = CancelFinalStateChange() & 0xFF;
		__NK_ASSERT_ALWAYS(orig==0 || orig==state);
		action = orig ? EWait : 0;
		goto end;
		}

	// possible states here 002g (propagated idle) or 008m (IDFC or DFC on endogenous DFC queue)
	if (stt==1)	// propagated idle
		cpu = TheScheduler.iIdleSpillCpu;

	// if it's on this CPU's IDFC queue we can just remove it and reset the state here
	// otherwise we send a cancel IPI to the CPU it's on
	// We are guaranteed to dequeue the DFC before it executes since the
	// QueueDfcs() on the target CPU will notice that a cancel is in progress and
	// so will not run the DFC even if it dequeues it.
	// QueueDfcs() takes care of the tied thread/group if it sees the DFC/IDFC again, otherwise
	// we must do it here.
	if (TUint(cpu) == ss0.iCpuNum)
		{
		if (IsIDFC())
			tied = iTied;
		action = EDeque|EReset|EIdleUnlock;
		}
	else
		action = EIdleUnlock|ESendIPI|EWait;

end:
	// Common exit point
	if (action & EDeque)
		Deque();
	if (action & EReset)
		{
		ResetState();
		}
	if (action & EIdleUnlock)
		TheScheduler.iIdleSpinLock.UnlockOnly();
	NKern::RestoreInterrupts(irq);
	if (t)
		t->RelSLock();

	// on another CPU's IDFC queue so send IPI to remove it
	if (action & ESendIPI)
		{
		TCancelIPI ipi;
		ipi.Send(this, cpu);
		ipi.WaitCompletion();
		tied = 0;
		}

	// wait for cancel to complete
	if (action & EWait)
		{
		TUint n = 0x01000000;
		while ((iDfcState>>8) & ss0.iCpuMask)
			{
			__chill();
			if (!--n)
				__crash();
			}
		}

	// release tied thread/group if waiting for IDFC to complete
	if (tied)
		tied->EndTiedEvent();
	NKern::Unlock();
	return ret;
	}


/** Queues a DFC (not an IDFC) from a thread.

	Does nothing if DFC is already queued.

    NOTE: Although this can be called in an IDFC context, it is more efficient to call
    DoEnque() in this case.
    
    @pre    Call either in a thread or an IDFC context.
	@pre	Do not call from an ISR.
	@return	TRUE if DFC was actually queued by this call
			FALSE if DFC was already queued on entry so this call did nothing
 */
EXPORT_C TBool TDfc::Enque()
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR,"TDfc::Enque()");		
	NKern::Lock();
	TBool ret = DoEnque();
	NKern::Unlock();
	return ret;
	}


/** Queue a DFC (not an IDFC) from a thread and also signals a fast mutex.

	The DFC is unaffected if it is already queued.

	The fast mutex is signalled before preemption is reenabled to avoid potential
	scheduler thrashing.

	@param	aMutex =	pointer to fast mutex to be signalled;
						NULL means system lock mutex.
	@return	TRUE if DFC was actually queued by this call
			FALSE if DFC was already queued on entry so this call did nothing
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked.
	@pre	Do not call from an ISR.
	@pre    Do not call from an IDFC.
 */
EXPORT_C TBool TDfc::Enque(NFastMutex* aMutex)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_NOT_ISR|MASK_NOT_IDFC,"TDfc::Enque(NFastMutex* aMutex)");		
	if (!aMutex)
		aMutex=&TheScheduler.iLock;
	NKern::Lock();
	TBool ret = DoEnque();
	aMutex->Signal();
	NKern::Unlock();
	return ret;
	}


/** Returns a pointer to the thread on which a DFC runs

	@return	If this is a DFC and the DFC queue has been set, a pointer to the
			thread which will run the DFC.
			NULL if this is an IDFC or the DFC queue has not been set.
 */
EXPORT_C NThreadBase* TDfc::Thread()
	{
	if (!IsDFC())
		return 0;
	return iDfcQ ? iDfcQ->iThread : 0;
	}


/******************************************************************************
 * Idle notification
 ******************************************************************************/

/** Register an IDFC or a DFC to be called when the system goes idle

	This function does nothing if the IDFC/DFC is already queued.

	@return	TRUE if DFC was actually queued by this call
			FALSE if DFC was already queued on entry so this call did nothing
 */
EXPORT_C TBool TDfc::QueueOnIdle()
	{
	TInt irq = TheScheduler.iIdleSpinLock.LockIrqSave();
	TUint32 orig = 0xFF00;

	// Transition the state to 'on normal idle queue'
	// 0000->002g
	// 00Cn->006n
	// All other states unchanged
	// Return original state
	if (IsValid())	// don't add if tied and tied thread/group is being/has been destroyed
		orig = QueueOnIdleStateChange();
	if (orig==0)
		{
		i8888.iHState0 = 0;	// BeginTiedEvent() not done
		TheScheduler.iIdleDfcs.Add(this);
		}

	TheScheduler.iIdleSpinLock.UnlockIrqRestore(irq);
	return (orig==0 || (orig&0xFFE0)==0x00C0);
	}


/******************************************************************************
 * Scheduler IDFC/DFC Processing
 ******************************************************************************/

void TSubScheduler::QueueDfcs()
//
// Enter with interrupts off and kernel locked
// Leave with interrupts off and kernel locked
//
// In state descriptions:
//		XX=8 bits not all zero (bitmask representing cancelling CPUs)
//		xx=8 bits (bitmask representing cancelling CPUs)
//		YY=8 bits not all zero
//		ZZ=XX with an additional bit set corresponding to the current CPU
//		zz=xx with an additional bit set corresponding to the current CPU
//		n = current CPU number
//		m = another CPU number
//		g = idle generation number
	{
	__KTRACE_OPT(KSCHED2,DEBUGPRINT("^"));
	iInIDFC = TRUE;
	BTrace0(BTrace::ECpuUsage, BTrace::EIDFCStart);
	TDfc* d = 0;
	NSchedulable* tied = 0;
	FOREVER
		{
		NKern::DisableAllInterrupts();
		// remove from pending queue with interrupts disabled
		d = (TDfc*)iDfcs.GetFirst();
		if (d)
			{
			d->iNext = 0;
#ifdef _DEBUG
			TUint32 st8 = DFC_STATE(d) & 0xFF;
			if (st8 != TUint(0x80|iCpuNum) && st8 != TUint(0x21^TheScheduler.iIdleGeneration))
				__crash();
#endif
			if (d->IsDFC())	// also true for mutating NTimer
				{
				NKern::EnableAllInterrupts();
				TDfcQue* q = d->iDfcQ;
				NThreadBase* t = q->iThread;
				t->AcqSLock();	// also protects DFC queue

				// transition to 'final queue' state
				// 002g->0001, ok=TRUE
				// 008n->0001, ok=TRUE
				// XXYY->XX00, ok=FALSE
				// XX00->0000, ok=FALSE
				// other starting states invalid
				TUint32 orig = d->MoveToFinalQStateChange() >> 5;
				if (orig==1 || orig==4)
					{
					// wasn't being cancelled, now marked as on final queue, which means
					// attempts to cancel will block on the thread spin lock
					TUint present = q->iPresent[0];
					q->Add((TPriListLink*)d);
					if (!present)
						t->iWaitState.UnBlockT(NThreadBase::EWaitDfc, q, KErrNone);
					}
				t->RelSLock();	// also protects DFC queue
				continue;
				}
			// endogenous IDFC - could be tied in which case may need to be punted over to another CPU
			// can't be mutating NTimer since that would have gone into IsDFC() path
			tied = d->iTied;
			if (tied && !d->i8888.iHState0)	// if tied and BeginTiedEvent() not already done
				{
				d->i8888.iHState0 = 1;		// flag that BeginTiedEvent() done
				TInt cpu = tied->BeginTiedEvent();
				if (TUint(cpu) != iCpuNum)
					{
					// punt over to other CPU
					TBool kick = FALSE;
					TSubScheduler* ss = TheSubSchedulers + cpu;
					ss->iExIDfcLock.LockOnly();
					// transition state here to handle cancel
					// XXYY->XX00, ok=FALSE
					// XX00->0000, ok=FALSE
					// 008n->00Am, ok=TRUE
					// 002g->00Am, ok=TRUE
					// other starting states invalid
					TUint32 orig = d->TransferIDFCStateChange(cpu) >> 5;
					if (orig==1 || orig==4)
						{
						kick = !ss->iExIDfcPendingFlag;
						ss->iExIDfcPendingFlag = TRUE;
						ss->iExIDfcs.Add(d);
						}
					ss->iExIDfcLock.UnlockOnly();
					if (kick)
						{
						TScheduler& s = TheScheduler;
						TUint32 cpuMask = 1u<<cpu;
						if (!(s.iThreadAcceptCpus & cpuMask))	// deal with case where target CPU is shutting down or has already shut down
							{
							TInt irq = s.iGenIPILock.LockIrqSave();
							if (!(s.iIpiAcceptCpus & cpuMask))
								{
								s.iCCReactivateCpus |= cpuMask;
								kick = FALSE;
								}
							s.iGenIPILock.UnlockIrqRestore(irq);
							if (!kick)
								s.iCCReactivateDfc.DoEnque();	// arrange for target CPU to be powered on
							}
						}
					if (kick)
						send_resched_ipi(cpu);
					NKern::EnableAllInterrupts();	// let interrupts in
					if (orig >= 8)
						tied->EndTiedEvent();		// IDFC cancelled so release tied thread/group
					continue;
					}
				}
			}
		else
			{
			if (!iExIDfcPendingFlag)
				break;
			iExIDfcLock.LockOnly();
			d = (TDfc*)iExIDfcs.GetFirst();
			if (!d)
				{
				iExIDfcPendingFlag = 0;
				iExIDfcLock.UnlockOnly();
				break;
				}
			d->iNext = 0;
			tied = d->iTied;
			__NK_ASSERT_ALWAYS(d->IsIDFC() && tied);	// only tied IDFCs should get here
#ifdef _DEBUG
			TUint32 st8 = DFC_STATE(d) & 0xFF;
			if (st8 != (0xA0|iCpuNum))
				__crash();
#endif
			iExIDfcLock.UnlockOnly();
			}

		// endogenous or exogenous IDFC
		// if tied, we are on correct CPU
		TDfcFn f = d->iFn;
		TAny* p = d->iPtr;

		// If Cancel() finds the IDFC in the running state (00Cn or 00En) it will do the following
		// atomic { if (iCurrentIDFC==d) iCurrentIDFC=0; }
		// We must guarantee that the following access is observed before the state change in RunIDFCStateChange()
		// We assume the latter has full barrier semantics to guarantee this.
		iCurrentIDFC = d;

		// transition to running state
		// 002g->00Cn, ok=TRUE
		// 008n->00Cn, ok=TRUE
		// 00An->00Cn, ok=TRUE
		// XXYY->XX00, ok=FALSE
		// XX00->0000, ok=FALSE
		// other starting states invalid
		TUint32 orig = d->RunIDFCStateChange() >> 5;
		NKern::EnableAllInterrupts();
		if (orig==1 || orig==4 || orig==5)
			{
			(*f)(p);

			// transition to idle state or rerun if necessary
			// first swap iCurrentIDFC with 0 - if original value != d, don't touch d again, return 0xFFFFFFFF
			// 00Cn->0000
			// 00En->008n
			// 006n->006n
			// XXCn->XX00
			// XXEn->XX00
			// XX6n->XX00
			// other starting states invalid
			// return original state
			NKern::DisableAllInterrupts();
			TUint32 orig = d->EndIDFCStateChange(this);
			if ((orig>>5)==7)
				{
				iDfcs.Add(d);
#ifdef _DEBUG
				TUint32 st8 = DFC_STATE(d) & 0xFF;
				if (st8 != (0x80|iCpuNum))
					__crash();
#endif
				continue;
				}
			else if ((orig>>5)==3)
				{
				TheScheduler.iIdleSpinLock.LockOnly();
				// 006n->002g
				// XX6n->XX00
				orig = d->EndIDFCStateChange2();
				if ((orig>>5)==3)
					TheScheduler.iIdleDfcs.Add(d);
				TheScheduler.iIdleSpinLock.UnlockOnly();
				}
			NKern::EnableAllInterrupts();
			if (tied && orig<0x10000)
				tied->EndTiedEvent(); // if we set iCurrentIDFC back to 0, we release the tied thread/group
			}
		else
			{
			iCurrentIDFC = 0;
			if (tied)
				tied->EndTiedEvent();		// IDFC cancelled so release tied thread/group
			}
		}
	iDfcPendingFlag = 0;
	BTrace0(BTrace::ECpuUsage, BTrace::EIDFCEnd);
	iInIDFC = 0;
	__KTRACE_OPT(KSCHED2,DEBUGPRINT("~"));
	}


/******************************************************************************
 * Kernel-side asynchronous request DFCs
 ******************************************************************************/

EXPORT_C TAsyncRequest::TAsyncRequest(TDfcFn aFunction, TDfcQue* aDfcQ, TInt aPriority)
	: TDfc(aFunction, this, aDfcQ, aPriority), iCompletionObject(0), iCancel(0), iResult(0)
	{
	}


EXPORT_C void TAsyncRequest::Send(TDfc* aCompletionDfc)
	{
	__NK_ASSERT_DEBUG(!iCompletionObject);
	iCancel = EFalse;
	iCompletionObject = (TAny*)((TLinAddr)aCompletionDfc|1);
	TDfc::Enque();
	}


EXPORT_C void TAsyncRequest::Send(NFastSemaphore* aCompletionSemaphore)
	{
	__NK_ASSERT_DEBUG(!iCompletionObject);
	iCancel = EFalse;
	iCompletionObject = aCompletionSemaphore;
	TDfc::Enque();
	}


EXPORT_C TInt TAsyncRequest::SendReceive()
	{
	NFastSemaphore signal;
	NKern::FSSetOwner(&signal, 0);
	Send(&signal);
	NKern::FSWait(&signal);
	return iResult;
	}


EXPORT_C void TAsyncRequest::Cancel()
	{
	iCancel = ETrue;
	if(TDfc::Cancel())
		Complete(KErrCancel);
	}


EXPORT_C void TAsyncRequest::Complete(TInt aResult)
	{
	TLinAddr signal = (TLinAddr)__e32_atomic_swp_ord_ptr(&iCompletionObject, 0);
	if(signal)
		{
		iResult = aResult;
		if(signal&1)
			((TDfc*)(signal&~1))->Enque();
		else
			NKern::FSSignal((NFastSemaphore*)signal);
		}
	}
