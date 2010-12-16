// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkernsmp\nkerns.cpp
// 
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

#include <e32cmn.h>
#include <e32cmn_private.h>
#include "nk_priv.h"

extern "C" void ExcFault(TAny*);

/******************************************************************************
 * Fast mutex
 ******************************************************************************/
/** Create a fast mutex

	@publishedPartner
	@released
*/
EXPORT_C NFastMutex::NFastMutex()
	: iHoldingThread(0), iMutexLock(TSpinLock::EOrderFastMutex)
	{
	}

/******************************************************************************
 * NSchedulable
 ******************************************************************************/
NSchedulable::NSchedulable()
	:	iSSpinLock(TSpinLock::EOrderThread)
	{
	iPriority = 0;
	iReady = 0;
	iCurrent = 0;
	iLastCpu = 0;
	iNSchedulableSpare1 = 0;
	iPauseCount = 0;
	iSuspended = 0;
	iNSchedulableSpare2 = 0;
	iCpuChange = 0;
	iStopping = 0;
	iFreezeCpu = 0;
	iParent = (NSchedulable*)0xdeadbeef;
	iCpuAffinity = 0;
	new (i_IDfcMem) TDfc(&DeferredReadyIDfcFn, this);
	iEventState = 0;
	iTotalCpuTime64 = 0;
	}

/******************************************************************************
 * NThreadGroup
 ******************************************************************************/
NThreadGroup::NThreadGroup()
	{
	iParent = 0;
	iThreadCount = 0;
	new (&iSSpinLock) TSpinLock(TSpinLock::EOrderThreadGroup);
	}

/** Create a thread group

	@publishedPartner
	@prototype
*/
EXPORT_C TInt NKern::GroupCreate(NThreadGroup* aGroup, SNThreadGroupCreateInfo& aInfo)
	{
	new (aGroup) NThreadGroup();
	aGroup->iCpuAffinity = aInfo.iCpuAffinity;
	return KErrNone;
	}


/** Destroy a thread group

	@pre Call in thread context, interrupts enabled, preemption enabled
	@pre No fast mutex held
	@pre Calling thread in critical section
	@pre All threads have left the group

	@publishedPartner
	@prototype
*/
EXPORT_C void NKern::GroupDestroy(NThreadGroup* aGroup)
	{
	NKern::ThreadEnterCS();
	aGroup->DetachTiedEvents();
	NKern::ThreadLeaveCS();
	}


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

NThreadWaitState::NThreadWaitState()
	:	iTimer(&TimerExpired, this)
	{
	iWtSt64 = 0;
	iTimer.iTriggerTime = 0;
	iTimer.iNTimerSpare1 = 0;
	}

NThreadBase::NThreadBase()
	:	iRequestSemaphore(), iWaitState()
	{
	iParent = this;
	iWaitLink.iPriority = 0;
	iBasePri = 0;
	iMutexPri = 0;
	i_NThread_Initial = 0;
	iLinkedObjType = EWaitNone;
	i_ThrdAttr = 0;
	iNThreadBaseSpare10 = 0;
	iFastMutexDefer = 0;
	iRequestSemaphore.iOwningThread = (NThreadBase*)this;
	iTime = 0;
	iTimeslice = 0;
	iSavedSP = 0;
	iAddressSpace = 0;
	iHeldFastMutex = 0;
	iUserModeCallbacks = 0;
	iLinkedObj = 0;
	iNewParent = 0;
	iFastExecTable = 0;
	iSlowExecTable = 0;
	iCsCount = 0;
	iCsFunction = 0;
	iHandlers = 0;
	iSuspendCount = 0;
	iStackBase = 0;
	iStackSize = 0;
	iExtraContext = 0;
	iExtraContextSize = 0;
	iNThreadBaseSpare6 = 0;
	iNThreadBaseSpare7 = 0;
	iNThreadBaseSpare8 = 0;
	iNThreadBaseSpare9 = 0;

	// KILL
	iTag = 0;
	iVemsData = 0;
	}

TInt NThreadBase::Create(SNThreadCreateInfo& aInfo, TBool aInitial)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT(">NThreadBase::Create %08x(%08x,%d)", this, &aInfo, aInitial));
	if (aInfo.iPriority<0 || aInfo.iPriority>63)
		return KErrArgument;
	if (aInfo.iPriority==0 && !aInitial)
		return KErrArgument;
//	if (aInfo.iCpu!=KCpuAny && aInfo.iCpu>=TheScheduler.iNumCpus)
//		return KErrArgument;
	iStackBase=(TLinAddr)aInfo.iStackBase;
	iStackSize=aInfo.iStackSize;
	iTimeslice=(aInfo.iTimeslice>0)?aInfo.iTimeslice:-1;
	iTime=iTimeslice;
	iPriority=TUint8(aInfo.iPriority);
	iBasePri=TUint8(aInfo.iPriority);
	iCpuAffinity = aInfo.iCpuAffinity;
	iHandlers = aInfo.iHandlers ? aInfo.iHandlers : &NThread_Default_Handlers;
	iFastExecTable=aInfo.iFastExecTable?aInfo.iFastExecTable:&DefaultFastExecTable;
	iSlowExecTable=(aInfo.iSlowExecTable?aInfo.iSlowExecTable:&DefaultSlowExecTable)->iEntries;
	i_ThrdAttr=(TUint8)aInfo.iAttributes;
	if (aInitial)
		{
		TSubScheduler& ss = SubScheduler();
		iLastCpu = (TUint8)ss.iCpuNum;
		iReady = (TUint8)(iLastCpu | EReadyOffset);
		iCurrent = iReady;
		iCpuAffinity = iLastCpu;
		iEventState = (iLastCpu<<EEventCpuShift) | (iLastCpu<<EThreadCpuShift);
		ss.Add(this);
		i_NThread_Initial = TRUE;
		ss.iInitialThread = (NThread*)this;
		NKern::Unlock();		// now that current thread is defined
		}
	else
		{
		iSuspendCount = 1;
		iSuspended = 1;
		TInt ecpu;
		if (iCpuAffinity & NTHREADBASE_CPU_AFFINITY_MASK)
			{
			ecpu = __e32_find_ls1_32(iCpuAffinity);
			if (ecpu >= TheScheduler.iNumCpus)
				ecpu = 0;	// FIXME: Inactive CPU?
			}
		else
			ecpu = iCpuAffinity;
		iEventState = (ecpu<<EEventCpuShift) | (ecpu<<EThreadCpuShift);
		if (aInfo.iGroup)
			{
			NKern::Lock();
			AcqSLock();
			aInfo.iGroup->AcqSLock();
			iParent = (NSchedulable*)aInfo.iGroup;
			++aInfo.iGroup->iThreadCount;
			iEventState |= EEventParent;
			RelSLock();
			NKern::Unlock();
			}
		}
	__KTRACE_OPT(KNKERN,DEBUGPRINT("<NThreadBase::Create OK"));
	return KErrNone;
	}

void NThread_Default_State_Handler(NThread* __DEBUG_ONLY(aThread), TInt __DEBUG_ONLY(aOperation), TInt __DEBUG_ONLY(aParameter))
	{
//	__KTRACE_OPT(KPANIC,DEBUGPRINT("Unknown NState %d: thread %T op %08x par %08x",aThread,aThread->iNState,aOperation,aParameter));
#ifdef _DEBUG
	DEBUGPRINT("UnknownState: thread %T op %08x par %08x",aThread,aOperation,aParameter);
#endif
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

TUserModeCallback::TUserModeCallback(TUserModeCallbackFunc aFunc)
	:	iNext(KUserModeCallbackUnqueued),
		iFunc(aFunc)
	{
	}

TUserModeCallback::~TUserModeCallback()
	{
	__NK_ASSERT_DEBUG(iNext == KUserModeCallbackUnqueued);
	}

void NKern::CancelUserModeCallbacks()
	{
	// Call any queued callbacks with the EUserModeCallbackCancel reason code, in the current
	// thread.

	TUserModeCallback* listHead =
		(TUserModeCallback*)__e32_atomic_swp_ord_ptr(&NCurrentThread()->iUserModeCallbacks, NULL);
	while (listHead)
		{
		TUserModeCallback* callback = listHead;
		listHead = listHead->iNext;
		callback->iNext = KUserModeCallbackUnqueued;
		__e32_memory_barrier();
		callback->iFunc(callback, EUserModeCallbackCancel);
		}
	}

void NKern::MoveUserModeCallbacks(NThreadBase* aDestThread, NThreadBase* aSrcThread)
	{
	// Move all queued user-mode callbacks from the source thread to the destination thread, and
	// prevent any more from being queued.  Used by the kernel thread code so that callbacks get
	// cancelled in another thread if the thread they were originally queued on dies.

	// Atomically remove list of callbacks and set pointer to 1
	// The latter ensures any subsequent attempts to add callbacks fail
	TUserModeCallback* sourceListStart =
		(TUserModeCallback*)__e32_atomic_swp_ord_ptr(&aSrcThread->iUserModeCallbacks, (TAny*)1);
	__NK_ASSERT_DEBUG(((TUint)sourceListStart & 3) == 0);  // check this only gets called once per thread

	if (sourceListStart == NULL)
		return;
	
	TUserModeCallback* sourceListEnd = sourceListStart;
	while (sourceListEnd->iNext != NULL)
		sourceListEnd = sourceListEnd->iNext;
	
	NKern::Lock();
	TUserModeCallback* destListStart = aDestThread->iUserModeCallbacks;
	do
		{
		__NK_ASSERT_DEBUG(((TUint)destListStart & 3) == 0);  // dest thread must not die
		sourceListEnd->iNext = destListStart;
		} while (!__e32_atomic_cas_ord_ptr(&aDestThread->iUserModeCallbacks, &destListStart, sourceListStart));
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

/** Get the current value of the high performance counter.

    If a high performance counter is not available, this uses the millisecond
    tick count instead.
*/
EXPORT_C TUint32 NKern::FastCounter()
	{
	return (TUint32)Timestamp();
	}


/** Get the frequency of counter queried by NKern::FastCounter().
*/
EXPORT_C TInt NKern::FastCounterFrequency()
	{
	return (TInt)TimestampFrequency();
	}


extern "C" {
TUint32 CrashState;
}

EXPORT_C TBool NKern::Crashed()
	{
	return CrashState!=0;
	}


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
		__ACQUIRE_BTRACE_LOCK();
		TBool r = traceData.iHandler(a0,0,aContext,a1,a2,a3,0,aPc);
		__RELEASE_BTRACE_LOCK();
		return r;
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

		__ACQUIRE_BTRACE_LOCK();
		TBool result = traceData.iHandler(a0+size,header2,aContext,aDataSize,a1,(TUint32)aData,traceId,aPc);
		__RELEASE_BTRACE_LOCK();
		if (!result)
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
#ifdef __USE_BTRACE_LOCK__
	return &BTraceLock;
#else
	return 0;
#endif
	}
