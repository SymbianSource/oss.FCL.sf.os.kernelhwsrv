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
// e32\nkern\nkerns.cpp
// 
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

#include <e32cmn.h>
#include <e32cmn_private.h>
#include "nk_priv.h"

extern "C" void ExcFault(TAny*);

/******************************************************************************
 * Thread
 ******************************************************************************/

void InvalidExec()
	{
	FAULT();
	}

static const SFastExecTable DefaultFastExecTable={0,{0}};
static const SSlowExecTable DefaultSlowExecTable={0,(TLinAddr)InvalidExec,0,{{0,0}}};

const SNThreadHandlers NThread_Default_Handlers =
	{
	NTHREAD_DEFAULT_EXIT_HANDLER,
	NTHREAD_DEFAULT_STATE_HANDLER,
	NTHREAD_DEFAULT_EXCEPTION_HANDLER,
	NTHREAD_DEFAULT_TIMEOUT_HANDLER
	};

/** Create a fast mutex

	@publishedPartner
	@released
*/
EXPORT_C NFastMutex::NFastMutex()
	: iHoldingThread(0), iWaiting(0)
	{
	}

/** Create a spin lock

	@internalComponent
*/
EXPORT_C TSpinLock::TSpinLock(TUint)
	: iLock(0)
	{
	}

/** Create a R/W spin lock

	@internalComponent
*/
EXPORT_C TRWSpinLock::TRWSpinLock(TUint)
	: iLock(0)
	{
	}

NThreadBase::NThreadBase()
	{
	// from TPriListLink
	iPriority = 0;
	iSpare1 = 0;
	iSpare2 = 0;
	iSpare3 = 0;

	iRequestSemaphore.iOwningThread=(NThreadBase*)this;
	new (&iTimer) NTimer(TimerExpired,this);
	iRequestSemaphore.iOwningThread = this;

	iHeldFastMutex = 0;
	iWaitFastMutex = 0;
	iAddressSpace = 0;
	iTime = 0;
	iTimeslice = 0;
	iWaitObj = 0;
	iSuspendCount = 0;
	iCsCount = 0;
	iCsFunction = 0;
	iReturnValue = 0;	
	iStackBase = 0;
	iStackSize = 0;
	iHandlers = 0;
	iFastExecTable = 0;
	iSlowExecTable = 0;
	iSavedSP = 0;
	iExtraContext = 0;
	iExtraContextSize = 0;
	iLastStartTime = 0;
	iTotalCpuTime = 0;
	iTag = 0;
	iVemsData = 0;
	iUserModeCallbacks = 0;
	iSpare7 = 0;
	iSpare8 = 0;
	}

TInt NThreadBase::Create(SNThreadCreateInfo& aInfo, TBool aInitial)
	{
	if (aInfo.iPriority<0 || aInfo.iPriority>63)
		return KErrArgument;
	if (aInfo.iPriority==0 && !aInitial)
		return KErrArgument;
	new (this) NThreadBase;
	iStackBase=(TLinAddr)aInfo.iStackBase;
	iStackSize=aInfo.iStackSize;
	iTimeslice=(aInfo.iTimeslice>0)?aInfo.iTimeslice:-1;
	iTime=iTimeslice;
#ifdef _DEBUG
	// When the crazy scheduler is active, refuse to set any priority higher than 1
	if (KCrazySchedulerEnabled())
		iPriority=TUint8(Min(1,aInfo.iPriority));
	else
#endif
		{
		iPriority=TUint8(aInfo.iPriority);
		}
	iHandlers = aInfo.iHandlers ? aInfo.iHandlers : &NThread_Default_Handlers;
	iFastExecTable=aInfo.iFastExecTable?aInfo.iFastExecTable:&DefaultFastExecTable;
	iSlowExecTable=(aInfo.iSlowExecTable?aInfo.iSlowExecTable:&DefaultSlowExecTable)->iEntries;
	iSpare2=(TUint8)aInfo.iAttributes;		// iSpare2 is NThread attributes
	if (aInitial)
		{
		iNState=EReady;
		iSuspendCount=0;
		TheScheduler.Add(this);
		TheScheduler.iCurrentThread=this;
		TheScheduler.iKernCSLocked=0;		// now that current thread is defined
		}
	else
		{
		iNState=ESuspended;
		iSuspendCount=-1;
		}
	return KErrNone;
	}

void NThread_Default_State_Handler(NThread* __DEBUG_ONLY(aThread), TInt __DEBUG_ONLY(aOperation), TInt __DEBUG_ONLY(aParameter))
	{
	__KTRACE_OPT(KPANIC,DEBUGPRINT("Unknown NState %d: thread %T op %08x par %08x",aThread,aThread->iNState,aOperation,aParameter));
	FAULT();
	}

void NThread_Default_Exception_Handler(TAny* aContext, NThread*)
	{
	ExcFault(aContext);
	}


/** Create a nanothread.

	This function is intended to be used by the EPOC kernel and by personality
	layers. A nanothread may not use most of the functions available to normal
	Symbian OS threads. Use Kern::ThreadCreate() to create a Symbian OS thread.

	@param aThread Pointer to control block for thread to create.
	@param aInfo Information needed for creating the thread.

	@see SNThreadCreateInfo
	@see Kern::ThreadCreate

	@pre	Call in a thread context.
	@pre	Interrupts must be enabled.
	@pre	Kernel must be unlocked.
 */
EXPORT_C TInt NKern::ThreadCreate(NThread* aThread, SNThreadCreateInfo& aInfo)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::ThreadCreate");
	return aThread->Create(aInfo,FALSE);
	}

// User-mode callbacks

TUserModeCallback::TUserModeCallback(TUserModeCallbackFunc aFunc) :
	iNext(KUserModeCallbackUnqueued),
	iFunc(aFunc)
	{
	}

TUserModeCallback::~TUserModeCallback()
	{
	__NK_ASSERT_DEBUG(iNext == KUserModeCallbackUnqueued);
	}

TInt NKern::QueueUserModeCallback(NThreadBase* aThread, TUserModeCallback* aCallback)
	{
	if (aCallback->iNext != KUserModeCallbackUnqueued)
		return KErrInUse;
	TInt r = KErrDied;
	NKern::Lock();
	TUserModeCallback* listHead = aThread->iUserModeCallbacks;
	if (((TLinAddr)listHead & 3) == 0)
		{
		aCallback->iNext = listHead;
		aThread->iUserModeCallbacks = aCallback;
		r = KErrNone;
		}
	NKern::Unlock();
	return r;
	}

// Called with interrupts disabled
// The vast majority of times this is called with zero or one callback pending
void NThreadBase::CallUserModeCallbacks()
	{
	while (iUserModeCallbacks != NULL)
		{		
		// Remove first callback
		TUserModeCallback* callback = iUserModeCallbacks;
		iUserModeCallbacks = callback->iNext;

		// Enter critical section to ensure callback is called
		NKern::ThreadEnterCS();
		
		// Re-enable interrupts and call callback
		NKern::EnableAllInterrupts();
		callback->iNext = KUserModeCallbackUnqueued;
		callback->iFunc(callback, EUserModeCallbackRun);

		// Leave critical section: thread may die at this point
		NKern::ThreadLeaveCS();
		
		NKern::DisableAllInterrupts();
		}
	}

void NKern::CancelUserModeCallbacks()
	{
	// Call any queued callbacks with the EUserModeCallbackCancel reason code, in the current
	// thread.

	NThreadBase* thread = NCurrentThread();
	NKern::Lock();	
	TUserModeCallback* listHead = thread->iUserModeCallbacks;
	thread->iUserModeCallbacks = NULL;
	NKern::Unlock();
	
	while (listHead != NULL)
		{
		TUserModeCallback* callback = listHead;
		listHead = listHead->iNext;
		callback->iNext = KUserModeCallbackUnqueued;
		callback->iFunc(callback, EUserModeCallbackCancel);
		}
	}

void NKern::MoveUserModeCallbacks(NThreadBase* aDestThread, NThreadBase* aSrcThread)
	{
	// Move all queued user-mode callbacks from the source thread to the destination thread, and
	// prevent any more from being queued.  Used by the kernel thread code so that callbacks get
	// cancelled in another thread if the thread they were originally queued on dies.

	NKern::Lock();	
	TUserModeCallback* sourceListStart = aSrcThread->iUserModeCallbacks;
	aSrcThread->iUserModeCallbacks = (TUserModeCallback*)1;
	NKern::Unlock();
	__NK_ASSERT_DEBUG(((TUint)sourceListStart & 3) == 0);  // check this only gets called once per thread

	if (sourceListStart == NULL)
		return;

	TUserModeCallback* sourceListEnd = sourceListStart;
	while (sourceListEnd->iNext != NULL)
		sourceListEnd = sourceListEnd->iNext;

	NKern::Lock();
	TUserModeCallback* destListStart = aDestThread->iUserModeCallbacks;
	__NK_ASSERT_DEBUG(((TUint)destListStart & 3) == 0);
	sourceListEnd->iNext = destListStart;
	aDestThread->iUserModeCallbacks = sourceListStart;
	NKern::Unlock();
	}

/** Initialise the null thread
	@internalComponent
*/
void NKern::Init(NThread* aThread, SNThreadCreateInfo& aInfo)
	{
	aInfo.iFunction=NULL;			// irrelevant
	aInfo.iPriority=0;				// null thread has lowest priority
	aInfo.iTimeslice=0;				// null thread not timesliced
	aInfo.iAttributes=0;			// null thread does not require implicit locks
	aThread->Create(aInfo,TRUE);	// create the null thread
	}

extern "C" {
TUint32 CrashState;
}

EXPORT_C TBool NKern::Crashed()
	{
	return CrashState!=0;
	}


/** @internalTechnology */
EXPORT_C void NKern::RecordIntLatency(TInt /*aLatency*/, TInt /*aIntMask*/)
	{
	}


/** @internalTechnology */
EXPORT_C void NKern::RecordThreadLatency(TInt /*aLatency*/)
	{
	}

/********************************************
 * Deterministic Priority List Implementation
 ********************************************/


/** Construct a priority list with the specified number of priorities

	@param aNumPriorities The number of priorities (must be 1-64).
 */
EXPORT_C TPriListBase::TPriListBase(TInt aNumPriorities)
	{
	memclr(this, _FOFF(TPriListBase,iQueue[0])+aNumPriorities*sizeof(SDblQueLink*));
	}


/********************************************
 * Miscellaneous
 ********************************************/


/**	Returns number of nanokernel timer ticks since system started.
	@return tick count
	@pre any context
 */
EXPORT_C TUint32 NKern::TickCount()
	{
	return NTickCount();
	}


TUint32 BTrace::BigTraceId = 0;

TBool BTrace::DoOutBig(TUint32 a0, TUint32 a1, const TAny* aData, TInt aDataSize, TUint32 aContext, TUint32 aPc)
	{
	SBTraceData& traceData = BTraceData;

	// see if trace is small enough to fit in single record...
	if(TUint(aDataSize)<=TUint(KMaxBTraceDataArray+4))
		{
		a0 += aDataSize;
		TUint32 a2 = 0;
		TUint32 a3 = 0;
		if(aDataSize)
			{
			a2 = *((TUint32*&)aData)++; // first 4 bytes into a2
			if(aDataSize>=4 && aDataSize<=8)
				a3 = *(TUint32*)aData; // only 4 more bytes, so pass by value, not pointer
			else
				a3 = (TUint32)aData;
			}
		return traceData.iHandler(a0,0,aContext,a1,a2,a3,0,aPc);
		}

	// adjust for header2, extra, and size word...
	a0 |= BTrace::EHeader2Present<<(BTrace::EFlagsIndex*8)|BTrace::EExtraPresent<<(BTrace::EFlagsIndex*8);
	a0 += 12;

	TUint32 traceId = __e32_atomic_add_ord32(&BigTraceId, 1);
	TUint32 header2 = BTrace::EMultipartFirst;
	TInt offset = 0;
	do
		{
		TUint32 size = aDataSize-offset;
		if(size>KMaxBTraceDataArray)
			size = KMaxBTraceDataArray;
		else
			header2 = BTrace::EMultipartLast;
		if(size<=4)
			*(TUint32*)&aData = *(TUint32*)aData; // 4 bytes or less are passed by value, not pointer

		TBool result = traceData.iHandler(a0+size,header2,aContext,aDataSize,a1,(TUint32)aData,traceId,aPc);
		if(!result)
			return result;

		offset += size;
		*(TUint8**)&aData += size;

		header2 = BTrace::EMultipartMiddle;
		a1 = offset;
		}
	while(offset<aDataSize);

	return TRUE;
	}

EXPORT_C TSpinLock* BTrace::LockPtr()
	{
	return 0;
	}
