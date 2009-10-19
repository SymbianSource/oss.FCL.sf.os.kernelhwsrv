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
// e32\nkern\dfcs.cpp
// DFCs
// 
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

// TDfc member data
#define __INCLUDE_TDFC_DEFINES__

#include "nk_priv.h"


/** Construct an IDFC

	@param aFunction = function to call
	@param aPtr = parameter to be passed to function
 */
EXPORT_C TDfc::TDfc(TDfcFn aFunction, TAny* aPtr)
	: iPtr(aPtr), iFunction(aFunction), iDfcQ(NULL)
	{
	iPriority=0xff;
	iSpare1=0;
	iOnFinalQ=FALSE;
	iQueued=FALSE;
	}


/** Construct a DFC without specifying a DFC queue.
	The DFC queue must be set before the DFC may be queued.

	@param aFunction = function to call
	@param aPtr = parameter to be passed to function
	@param aPriority = priority of DFC within the queue (0 to 7, where 7 is highest)
 */
EXPORT_C TDfc::TDfc(TDfcFn aFunction, TAny* aPtr, TInt aPriority)
	: iPtr(aPtr), iFunction(aFunction), iDfcQ(NULL)
	{
	__NK_ASSERT_DEBUG((TUint)aPriority<(TUint)KNumDfcPriorities);
	iPriority=TUint8(aPriority);
	iSpare1=0;
	iOnFinalQ=FALSE;
	iQueued=FALSE;
	}


/** Construct a DFC specifying a DFC queue.

	@param aFunction = function to call
	@param aPtr = parameter to be passed to function
	@param aDfcQ = pointer to DFC queue which this DFC should use
	@param aPriority = priority of DFC within the queue (0-7)
 */
EXPORT_C TDfc::TDfc(TDfcFn aFunction, TAny* aPtr, TDfcQue* aDfcQ, TInt aPriority)
	: iPtr(aPtr), iFunction(aFunction), iDfcQ(aDfcQ)
	{
	__NK_ASSERT_DEBUG((TUint)aPriority<(TUint)KNumDfcPriorities);
	iPriority=TUint8(aPriority);
	iSpare1=0;
	iOnFinalQ=FALSE;
	iQueued=FALSE;
	}


/** Construct a DFC queue
	Kern::DfcQInit() should be called on the new DFC queue before it can be used.
 */
EXPORT_C TDfcQue::TDfcQue()
	: iThread(NULL)
	{}


#ifndef __DFC_MACHINE_CODED__

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
	__ASSERT_WITH_MESSAGE_DEBUG(  NKern::CurrentContext()!=NKern::EThread  ||  TheScheduler.iKernCSLocked,"Do not call from thread with preemption enabled","TDfc::Add");
	__ASSERT_WITH_MESSAGE_DEBUG(  IsIDFC() || iDfcQ != NULL, "DFC queue not set", "TDfc::Add");
#ifdef __WINS__
	__NK_ASSERT_ALWAYS(Interrupt.InInterrupt() || TheScheduler.iKernCSLocked);
#endif
	return RawAdd();
	}


/** Queue an IDFC or a DFC from an ISR

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
	TInt irq=NKern::DisableAllInterrupts();

	// make sure DFC not already queued
	TBool ok = !TestAndSetQueued();
	if (ok)
		{
		TheScheduler.iDfcs.Add(this);
		TheScheduler.iDfcPendingFlag=1;
		}

	NKern::RestoreInterrupts(irq);
	return ok;
	}


/** Queue a DFC (not an IDFC) from an IDFC or thread with preemption disabled.

	This function is the preferred way to queue a DFC from an IDFC. It should not
	be used to queue an IDFC - use TDfc::Add() for this.

	This function does nothing if the DFC is already queued.

	@return	TRUE if DFC was actually queued by this call
			FALSE if DFC was already queued on entry so this call did nothing
	@pre Call only from IDFC or thread with preemption disabled.
	@pre Do not call from ISR or thread with preemption enabled.

	@see TDfc::Add()
	@see TDfc::Enque()
 */
EXPORT_C TBool TDfc::DoEnque()
	{
	__ASSERT_WITH_MESSAGE_DEBUG(  (NKern::CurrentContext()==NKern::EIDFC )||( NKern::CurrentContext()==NKern::EThread  &&  TheScheduler.iKernCSLocked),"Do not call from ISR or thread with preemption enabled","TDfc::DoEnque");
	__NK_ASSERT_DEBUG(!IsIDFC());
	__ASSERT_WITH_MESSAGE_DEBUG(  iDfcQ, "DFC queue not set", "TDfc::Add");

	// Check not already queued and then mark queued to prevent ISRs touching this DFC
	TBool ok = !TestAndSetQueued();
	if (ok)
		DoEnqueFinal();
	return ok;
	}

void TDfc::DoEnqueFinal()
//
// Add a DFC to its final queue. Assumes DFC not currently queued.
// Enter and return with kernel locked.
//
	{
	iOnFinalQ=TRUE;
	iDfcQ->Add(this);
	NThreadBase* pT=iDfcQ->iThread;
	if (pT->iNState==NThreadBase::EWaitDfc)
		pT->CheckSuspendThenReady();
	}

void TDfcQue::ThreadFunction(TAny* aDfcQ)
	{
	TDfcQue& q=*(TDfcQue*)aDfcQ;
	NThreadBase* pC=TheScheduler.iCurrentThread;
	FOREVER
		{
		NKern::Lock();
		if (q.IsEmpty())
			{
			pC->iNState=NThreadBase::EWaitDfc;
			TheScheduler.Remove(pC);
			RescheduleNeeded();
			NKern::Unlock();
			}
		else
			{
			TDfc& d=*q.First();
			q.Remove(&d);
			d.iOnFinalQ=FALSE;
			d.iQueued=FALSE;
			NKern::Unlock();
			(*d.iFunction)(d.iPtr);
			}
		}
	}


/** Cancels an IDFC or DFC.

	This function does nothing if the IDFC or DFC is not queued.

	@return	TRUE	if the DFC was actually dequeued by this call. In that case
					it is guaranteed that the DFC will not execute until it is
					queued again.
			FALSE	if the DFC was not queued on entry to the call, or was in
					the process of being executed or cancelled. In this case
					it is possible that the DFC executes after this call
					returns.

	@post	However in either case it is safe to delete the DFC object on
			return from this call provided only that the DFC function does not
			refer to the DFC object itself.

	@pre IDFC or thread context. Do not call from ISRs.

	@pre If the DFC function accesses the DFC object itself, the user must ensure that
	     Cancel() cannot be called while the DFC function is running.
 */
EXPORT_C TBool TDfc::Cancel()
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR,"TDfc::Cancel");
	NKern::Lock();
	TBool ret = iQueued;
	if (iQueued)	// ISRs can't affect this test since they can't de-queue a DFC or IDFC
		{
		if (!iOnFinalQ)	// OK to check this with interrupts enabled since interrupts can't change it
			{
			// Must disable interrupts to protect the pending queue
			TInt irq=NKern::DisableAllInterrupts();
			SDblQueLink::Deque();
			NKern::RestoreInterrupts(irq);
			}
		else
			{
			// Final queues can't be modified by interrupts
			iDfcQ->Remove(this);
			iOnFinalQ=FALSE;
			}
		iQueued=FALSE;	// must be done last
		}
	NKern::Unlock();
	return ret;
	}
#endif

/** Queues a DFC (not an IDFC) from a thread.

	Does nothing if DFC is already queued.

    NOTE: Although this can be called in an IDFC context, it is more efficient to call
    DoEnque() in this case.
    
	@return	TRUE if DFC was actually queued by this call
			FALSE if DFC was already queued on entry so this call did nothing
    @pre    Call either in a thread or an IDFC context.
	@pre	Do not call from an ISR.
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
	if (IsIDFC())
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
	TInt irq=NKern::DisableAllInterrupts();

	// make sure DFC not already queued
	TBool ok = !TestAndSetQueued();
	if (ok)
		TheScheduler.iIdleDfcs.Add(this);

	NKern::RestoreInterrupts(irq);
	return ok;
	}


TUint32 NKern::IdleGenerationCount()
	{
	return TheScheduler.iIdleGenerationCount;
	}


void NKern::Idle()
	{
	TInt irq = NKern::DisableAllInterrupts();
#ifdef _DEBUG
	if (!TheScheduler.iIdleDfcs.IsEmpty() && TheScheduler.iDelayedQ.IsEmpty())
#else
	if (!TheScheduler.iIdleDfcs.IsEmpty())
#endif
		{
		++TheScheduler.iIdleGenerationCount;
		TheScheduler.iDfcs.MoveFrom(&TheScheduler.iIdleDfcs);
		TheScheduler.iDfcPendingFlag=1;
		NKern::RestoreInterrupts(irq);
		return;
		}
	NKern::RestoreInterrupts(irq);
	NKIdle(0);
	}


/******************************************************************************
 * Scheduler IDFC/DFC Processing
 ******************************************************************************/

#ifndef __SCHEDULER_MACHINE_CODED__
void TScheduler::QueueDfcs()
//
// Enter with interrupts off and kernel locked
// Leave with interrupts off and kernel locked
//
	{
	iInIDFC = TRUE;
	BTrace0(BTrace::ECpuUsage,BTrace::EIDFCStart);
	FOREVER
		{
		// remove from pending queue with interrupts disabled
		TDfc* d=(TDfc*)iDfcs.GetFirst();
		if (!d)
			break;
		NKern::EnableAllInterrupts();
		if (d->IsIDFC())
			{
			d->iQueued=FALSE;
			(*d->iFunction)(d->iPtr);
			}
		else
			d->DoEnqueFinal();
		NKern::DisableAllInterrupts();
		}
	iDfcPendingFlag = FALSE;
	BTrace0(BTrace::ECpuUsage,BTrace::EIDFCEnd);
	iInIDFC = FALSE;
	}
#endif


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
