// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\sthread.cpp
// 
//

#include <kernel/kern_priv.h>
#include "execs.h"
#include <kernel/emi.h>

#define iMState				iWaitLink.iSpare1		// Allow a sensible name to be used for iMState
#define iExiting			iWaitLink.iSpare2		// Allow a sensible name to be used for iExiting
#define iWaitListReserved	iWaitLink.iSpare3		// Allow a sensible name to be used for iWaitListReserved
#define __CHECK_PRIORITIES(t)	__ASSERT_DEBUG((t)->iWaitLink.iPriority==(t)->iNThread.i_NThread_BasePri,Kern::Fault("WaitLinkPriT",__LINE__))

_LIT(KLitKill,"Kill");
_LIT(KLitTerminate,"Terminate");

extern const SNThreadHandlers EpocThreadHandlers =
	{
	&DThread::EpocThreadExitHandler,
	NTHREAD_DEFAULT_STATE_HANDLER,
	&Exc::Dispatch,
	&DThread::EpocThreadTimeoutHandler
	};

/********************************************
 * Thread cleanup entries
 ********************************************/
EXPORT_C TThreadCleanup::TThreadCleanup()
	: iThread(NULL)
	{
	}

// Enter and return with system locked.
void TThreadCleanup::ChangePriority(TInt aNewPriority)
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("TThreadCleanup::ChangePriority %d to %d, thread %O nest %d",iPriority,aNewPriority,iThread,K::PINestLevel));
	if (aNewPriority != iPriority)
		{
		if (iThread)
			{
			iThread->iCleanupQ.ChangePriority(this,aNewPriority);
			if (++K::PINestLevel<KMaxPriorityInheritanceNesting)
				iThread->SetRequiredPriority();
			}
		else
			iPriority=TUint8(aNewPriority);
		}
	}

// Enter and return with system locked.
/**
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.
	@pre System must be locked
	@pre Call in a thread context.
*/
EXPORT_C void TThreadCleanup::Remove()
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED|MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,
		"TThreadCleanup::Remove");
//	__KTRACE_OPT(KTHREAD,Kern::Printf("TThreadCleanup::Remove priority %d, thread %O",iPriority,iThread));
	iThread->iCleanupQ.Remove(this);
	if (iPriority>=iThread->iNThread.i_NThread_BasePri)	// can't affect priority if lower than current
		iThread->SetRequiredPriority();
	}

/********************************************
 * Thread
 ********************************************/

#ifdef KTHREAD
void DThread::DefaultUnknownStateHandler(DThread* aThread, TInt anOperation, TInt aParameter)
#else
void DThread::DefaultUnknownStateHandler(DThread* aThread, TInt, TInt)
#endif
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("UNKNOWN THREAD MSTATE!! thread %O Mstate=%d, operation=%d, parameter=%d",
		aThread,aThread->iMState,anOperation,aParameter));
	Kern::Fault("THREAD STATE",aThread->iMState);
	}

DThread::DThread()
	:	iTls(KTlsArrayGranularity,_FOFF(STls,iHandle)),
		iDebugMask(0xffffffff), iKillDfc(NULL,this,K::SvMsgQ,2),
		iUnknownStateHandler(DefaultUnknownStateHandler),
		iExitType((TUint8)EExitPending)
#ifdef __SMP__
		, iSMPSafeCallback(SMPSafeCallback)
#endif
	{
	iKernMsg.iSem.iOwningThread=&iNThread;
	iMState=ECreated;
	iExiting=EFalse;
	}

// Enter and return with system unlocked.
void DThread::Destruct()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DThread::Destruct %O",this));
	__NK_ASSERT_DEBUG(((TUint)iNThread.iUserModeCallbacks & ~3) == NULL);
	iTls.Close();
	if (iSyncMsgPtr)
		{
		// The sync message might still be outstanding; in particular it may be
		// on the kernel server's list of messages to be cleaned up on behalf of
		// dead clients. So we only release it here if it's free; otherwise, we
		// mutate the type, so that cleanup will return it to the Global pool.
		NKern::LockSystem();
		if (iSyncMsgPtr->IsFree())
			{
			NKern::UnlockSystem();
			__KTRACE_OPT(KSERVER,Kern::Printf("DThread::Destruct(%08X) releasing sync message at %08X", this, iSyncMsgPtr));
			iSyncMsgPtr->ReleaseMessagePool(RMessageK::ESync, 1);
			}
		else
			{
			__KTRACE_OPT(KSERVER,Kern::Printf("DThread::Destruct(%08X) mutating sync message at %08X", this, iSyncMsgPtr));
			iSyncMsgPtr->iMsgType = RMessageK::EGlobal;
			NKern::UnlockSystem();
			}
		iSyncMsgPtr = NULL;
		}
	FreeSupervisorStack();
	iHandles.Close(iOwningProcess);
	Kern::SafeClose(iExtTempObj,NULL);
	if (iNThread.iExtraContextSize > 0)
		Kern::Free(iNThread.iExtraContext);
	}

#if defined(__EPOC32__) && defined(KTHREAD)
void CheckSupervisorStackUsage(DThread* aT)
	{
	const TUint32* p = (const TUint32*)aT->iSupervisorStack;
	TUint32 used = 0;
	TUint32 free = 0;
	if (p)
		{
		const TUint32* pE = p + (aT->iSupervisorStackSize / sizeof(TUint32));
		while(p<pE && *p==0xeeeeeeeeu)
			++p;
		used = (pE - p) * sizeof(TUint32);
		free = TUint32(aT->iSupervisorStackSize) - used;
		}
	Kern::Printf("Thread %O used %d bytes of kernel stack (%d bytes free)", aT, used, free);
	}
#endif

// Enter and return with system unlocked.
void DThread::Release()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("Thread %O Release()",this));
	if (iWaitListReserved)
		{
		iWaitListReserved = 0;
		TThreadWaitList::ThreadDestroyed();
		}
	iTls.Close();
	__KTRACE_OPT(KTHREAD,Kern::Printf("Close temporary object %O",iTempObj));
	Kern::SafeClose(iTempObj,NULL);
	__KTRACE_OPT(KTHREAD,Kern::Printf("Decrement temporary message %08x",iTempMsg));
	if (iTempMsg)
		{
		NKern::LockSystem();
		iTempMsg->CloseRef();
		NKern::UnlockSystem();
		}
	iTempMsg=NULL;
	__KTRACE_OPT(KTHREAD,Kern::Printf("Free temporary allocation %08x",iTempAlloc));
	Kern::Free(iTempAlloc);
	iTempAlloc=NULL;
	
	__KTRACE_OPT(KTHREAD,Kern::Printf("Removing closing libraries"));
	if (!iClosingLibs.IsEmpty())
		RemoveClosingLibs();

	__KTRACE_OPT(KTHREAD, Kern::Printf("Unmapping code segs delayed by User::Leave() (if any)"));
	if (iLeaveDepth)
		CleanupLeave(iLeaveDepth);
	
	__KTRACE_OPT(KTHREAD,Kern::Printf("Deleting handles..."));
	iHandles.Close(iOwningProcess);
	__KTRACE_OPT(KTHREAD,Kern::Printf("Deleted handles"));

	// deallocate the user stack if any
	if (iThreadType==EThreadUser)
		FreeUserStack();

	// cancel the timer
	__KTRACE_OPT(KTHREAD,Kern::Printf("Cancelling timer"));
	iTimer.Cancel(NULL);

	__KTRACE_OPT(KTHREAD,Kern::Printf("Freeing supervisor-mode stack"));
#if defined(__EPOC32__) && defined(KTHREAD)
	__KTRACE_OPT(KTHREAD,CheckSupervisorStackUsage(this));
#endif
	FreeSupervisorStack();
#ifdef BTRACE_THREAD_IDENTIFICATION
	BTrace12(BTrace::EThreadIdentification,BTrace::EThreadDestroy,&iNThread,iOwningProcess,iId);
#endif
	__DEBUG_EVENT(EEventRemoveThread, this);
	__KTRACE_OPT(KTHREAD,Kern::Printf("Closing thread"));
	Close(iOwningProcess);					// close the thread
	}

// Enter and return with system locked.
/**
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.
	@pre System must be locked
	@pre Call in a thread context.
*/
EXPORT_C void DThread::AddCleanup(TThreadCleanup* aCleanup)
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED|MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,
		"DThread::AddCleanup");
	aCleanup->iThread=this;
	iCleanupQ.Add(aCleanup);
	if (aCleanup->iPriority>iNThread.i_NThread_BasePri)	// can't affect priority unless it's higher than current
		SetActualPriority(aCleanup->iPriority);	// this cleanup item must be the highest priority
	}

// Enter and return with system locked.
void DThread::Suspend(TInt aCount)
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("Suspend thread %O, count %d",this,aCount));

	TBool r=NKern::ThreadSuspend(&iNThread,aCount);
	if (!r)
		return;		// suspend was deferred (always so if self-suspend)

	// suspend has taken effect - fix up wait queues
	K::PINestLevel=0;
	switch (iMState)
		{
		case ECreated:
		case EDead:
		case EReady:
		case EWaitSemaphoreSuspended:
		case EWaitMutexSuspended:
		case EWaitCondVarSuspended:
			break;
		case EWaitSemaphore:
			((DSemaphore*)iWaitObj)->SuspendWaitingThread(this);
			break;
		case EWaitMutex:
			((DMutex*)iWaitObj)->SuspendWaitingThread(this);
			break;
		case EHoldMutexPending:
			((DMutex*)iWaitObj)->SuspendPendingThread(this);
			break;
		case EWaitCondVar:
			((DCondVar*)iWaitObj)->SuspendWaitingThread(this);
			break;
		default:
			UnknownState(ESuspend,0);
			break;
		}
	}

// Enter and return with system locked.
void DThread::Resume()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("Resume thread %O",this));

	TBool r=NKern::ThreadResume(&iNThread);
	if (!r)
		return;				// just cancelled deferred suspends

	// resume has taken effect - fix up wait queues
	K::PINestLevel=0;
	switch (iMState)
		{
		case ECreated:
		case EDead:
		case EReady:
		case EWaitSemaphore:
		case EWaitMutex:
		case EHoldMutexPending:
		case EWaitCondVar:
			break;
		case EWaitSemaphoreSuspended:
			((DSemaphore*)iWaitObj)->ResumeWaitingThread(this);
			break;
		case EWaitMutexSuspended:
			((DMutex*)iWaitObj)->ResumeWaitingThread(this);
			break;
		case EWaitCondVarSuspended:
			((DCondVar*)iWaitObj)->ResumeWaitingThread(this);
			break;
		default:
			UnknownState(EResume,0);
			break;
		}
	}

void DThread::ForceResume()
//
// Resume the thread regardless of nested suspensions
// Enter and return with system locked.
//
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("ForceResume thread %O",this));

	TBool r=NKern::ThreadForceResume(&iNThread);
	if (!r)
		return;				// just cancelled deferred suspends

	// resume has taken effect - fix up wait queues
	K::PINestLevel=0;
	switch (iMState)
		{
		case ECreated:
		case EDead:
		case EReady:
		case EWaitSemaphore:
		case EWaitMutex:
		case EHoldMutexPending:
		case EWaitCondVar:
			break;
		case EWaitSemaphoreSuspended:
			((DSemaphore*)iWaitObj)->ResumeWaitingThread(this);
			break;
		case EWaitMutexSuspended:
			((DMutex*)iWaitObj)->ResumeWaitingThread(this);
			break;
		case EWaitCondVarSuspended:
			((DCondVar*)iWaitObj)->ResumeWaitingThread(this);
			break;
		default:
			UnknownState(EResume,0);
			break;
		}
	}

// Cancel the thread's timer
// Enter and return with system locked.
EXPORT_C void DThread::CancelTimer()
	{
	iTimer.Cancel(NULL);
	iTimer.iState = (TUint8)TTimer::EIdle;
	}

// Release the thread's wait on any sync object (not explicit suspend)
// Enter and return with system locked.
/**
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.
	@pre System must be locked
	@pre Call in a thread context.
*/
EXPORT_C TInt DThread::ReleaseWait(TInt aReturnCode)
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED|MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,
		"DThread::ReleaseWait");	
	__KTRACE_OPT(KTHREAD,Kern::Printf("Thread %O ReleaseWait() retcode=%d",this,aReturnCode));
	switch(iMState)
		{
		case ECreated:
		case EDead:
		case EReady:
			return KErrGeneral;
		case EWaitSemaphore:
			((DSemaphore*)iWaitObj)->WaitCancel(this);
			break;
		case EWaitSemaphoreSuspended:
			((DSemaphore*)iWaitObj)->WaitCancelSuspended(this);
			break;
		case EWaitMutex:
			((DMutex*)iWaitObj)->WaitCancel(this);
			break;
		case EWaitMutexSuspended:
			((DMutex*)iWaitObj)->WaitCancelSuspended(this);
			break;
		case EHoldMutexPending:
			((DMutex*)iWaitObj)->RemovePendingThread(this);
			break;
		case EWaitCondVar:
			((DCondVar*)iWaitObj)->WaitCancel(this);
			break;
		case EWaitCondVarSuspended:
			((DCondVar*)iWaitObj)->WaitCancelSuspended(this);
			break;
		default:
			UnknownState(EReleaseWait,aReturnCode);
			break;
		}
	iWaitObj=NULL;
	iMState=EReady;
	NKern::ThreadRelease(&iNThread,aReturnCode);
	return KErrNone;
	}

void DThread::EpocThreadTimeoutHandler(NThread* aThread, TInt aOp)
	{
	if (aOp == NThreadBase::ETimeoutPreamble)
		{
		NKern::LockSystem();
		return;
		}
	if (aOp == NThreadBase::ETimeoutPostamble)
		{
		DThread* pT = _LOFF(aThread, DThread, iNThread);
		pT->ReleaseWait(KErrTimedOut);
		}
	NKern::UnlockSystem();
	}

#ifdef SYMBIAN_CURB_SYSTEMSERVER_PRIORITIES
const TUint8 KPrioritySystemServerMore = 23;
#else
const TUint8 KPrioritySystemServerMore = 24;
#endif // SYMBIAN_CURB_SYSTEMSERVER_PRIORITIES
const TInt KMaxPriorityWithoutProtServ = KPrioritySystemServerMore;

// Mapping table for thread+process priority to thread absolute priority
LOCAL_D const TUint8 ThreadPriorityTable[64] =
	{
//                  Idle MuchLess Less Normal                       More MuchMore RealTime
/*Low*/               1,     1,     2,     3,                         4,     5,      22,       0,
/*Background*/        3,     5,     6,     7,                         8,     9,      22,       0,
/*Foreground*/        3,     10,   11,    12,                        13,    14,      22,       0,
/*High*/              3,     17,   18,    19,                        20,    22,      23,       0,
/*SystemServer1*/     9,     15,   16,    21, KPrioritySystemServerMore,    25,      28,       0,
/*SystemServer2*/     9,     15,   16,    21, KPrioritySystemServerMore,    25,      28,       0,
/*SystemServer3*/     9,     15,   16,    21, KPrioritySystemServerMore,    25,      28,       0,
/*RealTimeServer*/   18,     26,   27,    28,                        29,    30,      31,       0
	};

TInt DThread::CalcDefaultThreadPriority()
	{
	TInt r;
	TInt tp=iThreadPriority;
	if (tp>=0)
		r=(tp<KNumPriorities)?tp:KNumPriorities-1;	// absolute thread priorities
	else
		{
		tp+=8;
		if (tp<0)
			tp=0;
		TInt pp=iOwningProcess->iPriority;	// process priority in range 0 to 7
		TInt i=(pp<<3)+tp;
		r=ThreadPriorityTable[i];	// map thread+process priority to actual priority
		if ((r > KMaxPriorityWithoutProtServ) &&
			!(this->HasCapability(ECapabilityProtServ,__PLATSEC_DIAGNOSTIC_STRING("Warning: thread priority capped to below realtime range"))))
			r = KMaxPriorityWithoutProtServ;
		}
	__KTRACE_OPT(KTHREAD,Kern::Printf("CalcDefaultThreadPriority tp %d pp %d abs %d",iThreadPriority,iOwningProcess->iPriority,r));
	return r;
	}

// Enter and return with system locked.
/**
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.
	@pre System must be locked
	@pre Call in a thread context.
*/
EXPORT_C void DThread::SetThreadPriority(TInt aThreadPriority)
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED|MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,
		"DThread::SetThreadPriority");
	__KTRACE_OPT(KTHREAD,Kern::Printf("DThread %O SetThreadPriority %d",this,aThreadPriority));
	iThreadPriority=aThreadPriority;
	TInt def=CalcDefaultThreadPriority();
	SetDefaultPriority(def);
	}

// Enter and return with system locked.
void DThread::SetDefaultPriority(TInt aDefaultPriority)
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("Thread %O SetDefaultPriority %d",this,aDefaultPriority));
#ifdef BTRACE_THREAD_PRIORITY
	BTrace12(BTrace::EThreadPriority,BTrace::EDThreadPriority,&this->iNThread,iThreadPriority,aDefaultPriority);
#endif
#if defined(_DEBUG) && !defined(__SMP__)
	// Cap priorities at 1 if we're doing crazy scheduling
	if (TheSuperPage().KernelConfigFlags() & EKernelConfigCrazyScheduling && aDefaultPriority > 1)
		aDefaultPriority = 1;
#endif
	iDefaultPriority=aDefaultPriority;
	NKern::ThreadSetNominalPriority(&iNThread, aDefaultPriority);
	K::PINestLevel=0;
	SetRequiredPriority();
	}

// Enter and return with system locked.
void DThread::SetRequiredPriority()
	{
	TInt p=iDefaultPriority;
	TInt c=iCleanupQ.HighestPriority();
	__KTRACE_OPT(KTHREAD,Kern::Printf("Thread %O SetRequiredPriority def %d cleanup %d nest %d",
								this,p,c,K::PINestLevel));
	if (c>p)
		p=c;
	if (p!=iNThread.i_NThread_BasePri)
		SetActualPriority(p);
	}

// Enter and return with system locked.
void DThread::SetActualPriority(TInt anActualPriority)
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("Thread %O MState %d SetActualPriority %d",this,iMState,anActualPriority));
	TInt newp=anActualPriority;
	__ASSERT_DEBUG(newp>=0 && newp<KNumPriorities, K::Fault(K::EBadThreadPriority));
	NKern::ThreadSetPriority(&iNThread,newp);
	switch(iMState)
		{
		case ECreated:
		case EWaitSemaphoreSuspended:
		case EWaitMutexSuspended:
		case EWaitCondVarSuspended:
		case EReady:
		case EDead:
			iWaitLink.iPriority=TUint8(newp);
			break;
		case EWaitSemaphore:
			((DSemaphore*)iWaitObj)->ChangeWaitingThreadPriority(this,newp);
			break;
		case EWaitMutex:
			((DMutex*)iWaitObj)->ChangeWaitingThreadPriority(this,newp);
			break;
		case EHoldMutexPending:
			((DMutex*)iWaitObj)->ChangePendingThreadPriority(this,newp);
			break;
		case EWaitCondVar:
			((DCondVar*)iWaitObj)->ChangeWaitingThreadPriority(this,newp);
			break;
		default:
			UnknownState(EChangePriority,newp);
			break;
		}
	__CHECK_PRIORITIES(this);
	}

// Enter with system locked, return with system unlocked.
// Assumes thread MState is READY.
void DThread::RevertPriority()
	{
	TInt p=iDefaultPriority;
	TInt c=iCleanupQ.HighestPriority();
	__KTRACE_OPT(KTHREAD,Kern::Printf("Thread %O RevertPriority def %d cleanup %d",
								this,iDefaultPriority,c));
	if (c>p)
		p=c;
	if (p!=iNThread.i_NThread_BasePri)
		{
		iWaitLink.iPriority=TUint8(p);
		NKern::ThreadSetPriority(&iNThread,p,SYSTEM_LOCK);	// anti-thrash
		}
	else
		NKern::UnlockSystem();
	}

// Enter and return with system unlocked.
TInt DThread::DoCreate(SThreadCreateInfo& aInfo)
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("Thread %O DoCreate: Function %08x Ptr %08x",this,aInfo.iFunction,aInfo.iPtr));
	__KTRACE_OPT(KTHREAD,Kern::Printf("type %d, sup stack=%08x, sup stack size=%x",aInfo.iType,aInfo.iSupervisorStack,aInfo.iSupervisorStackSize));
	__KTRACE_OPT(KTHREAD,Kern::Printf("user stack=%08x size %x, init priority=%d",aInfo.iUserStack,aInfo.iUserStackSize,aInfo.iInitialThreadPriority));
	iSupervisorStackSize=aInfo.iSupervisorStackSize;
	if (iSupervisorStackSize==0)
		iSupervisorStackSize=K::SupervisorThreadStackSize;
	if (iThreadType==EThreadInitial || iThreadType==EThreadAPInitial || iThreadType==EThreadMinimalSupervisor)
		iSupervisorStack=aInfo.iSupervisorStack;
	TInt r=KErrNone;
	if (!iSupervisorStack)
		r=AllocateSupervisorStack();
	if (r!=KErrNone)
		return r;
	iSyncMsgPtr = RMessageK::ClaimMessagePool(RMessageK::ESync, 1, NULL);
	__KTRACE_OPT(KSERVER,Kern::Printf("DThread::DoCreate(%08X) claimed sync message at %08X", this, iSyncMsgPtr));
	if (K::MsgInfo.iChunk && !iSyncMsgPtr)
		return KErrNoMemory;
	SNThreadCreateInfo ni;
	ni.iFunction=EpocThreadFunction;
	ni.iPriority=1;	// Overridden by nkern for initial thread(s)
#if defined(_DEBUG) && !defined(__SMP__)
	// When doing crazy scheduling, all threads get just 1-tick timeslices
	if (TheSuperPage().KernelConfigFlags() & EKernelConfigCrazyScheduling)
		ni.iTimeslice = (iThreadType==EThreadMinimalSupervisor || iThreadType==EThreadAPInitial || iThreadType==EThreadInitial) ? -1 : 1;
	else
#endif
		{
		ni.iTimeslice =	(	iThreadType==EThreadMinimalSupervisor
						||	iThreadType==EThreadAPInitial
						||	iThreadType==EThreadInitial
						) ?	-1
						  :	NKern::TimesliceTicks(EDefaultUserTimeSliceMs*1000);
		}
	ni.iAttributes=0;		// overridden if necessary by memory model
	ni.iHandlers = &EpocThreadHandlers;
	ni.iFastExecTable=(const SFastExecTable*)EpocFastExecTable;
	ni.iSlowExecTable=(const SSlowExecTable*)EpocSlowExecTable;
	ni.iParameterBlock=(const TUint32*)&aInfo;
	ni.iParameterBlockSize=aInfo.iTotalSize;
	ni.iStackBase=iSupervisorStack;
	ni.iStackSize=iSupervisorStackSize;
#ifdef __SMP__
	TUint32 config = TheSuperPage().KernelConfigFlags();
	ni.iGroup = 0;
	ni.iCpuAffinity = KCpuAffinityAny;
	if (iThreadType==EThreadUser)
		{
		// user thread
		if ((config & EKernelConfigSMPUnsafeCPU0) && iOwningProcess->iSMPUnsafeCount)
			{
			ni.iCpuAffinity = 0; // compatibility mode
			}
		else
			{
			if ((config & EKernelConfigSMPUnsafeCompat) && iOwningProcess->iSMPUnsafeCount)
				ni.iGroup = iOwningProcess->iSMPUnsafeGroup;
			}
		}
	else
		{
		if (config & EKernelConfigSMPLockKernelThreadsCore0) 
			{
			// kernel thread
			ni.iCpuAffinity = 0;
			}
		}
#endif
	if (iThreadType!=EThreadInitial)
		{
		if (iSupervisorStack)
			memset(iSupervisorStack,0xee,iSupervisorStackSize);
		if (iThreadType != EThreadAPInitial)
			{
			TAny* ec = iNThread.iExtraContext;
			TInt ecs = iNThread.iExtraContextSize;
			r=NKern::ThreadCreate(&iNThread,ni);
			if (r!=KErrNone)
				return r;
			iNThread.SetExtraContext(ec, ecs);
			NKern::LockSystem();
			SetThreadPriority(aInfo.iInitialThreadPriority);
			NKern::UnlockSystem();
			}
		}
	else
		iMState=EReady;
	r=SetupContext(aInfo);
	if (r==KErrNone)
		r=iTimer.Create();
	return r;
	}

void svThreadKill(TAny* aPtr)
	{
	((DThread*)aPtr)->SvKill();
	}

// Enter and return with system unlocked.
TDfc* DThread::EpocThreadExitHandler(NThread* aThread)
	{
	DThread* pT=_LOFF(aThread,DThread,iNThread);
	pT->Exit();
	pT->iKillDfc.SetFunction(svThreadKill);
	return &pT->iKillDfc;	// NKERN will queue this before terminating this thread
	}

#if defined(__SMP__) && defined(KTIMING)
TUint64 tix2us(TUint64 aTicks, TUint32 aFreq)
	{
	TUint64 e6(1000000);
	aTicks *= e6;
	aTicks += TUint64(aFreq>>1);
	aTicks /= TUint64(aFreq);
	TUint64 us = aTicks % e6;
	TUint64 sec = aTicks / e6;
	return (sec<<32)|us;
	}
#endif

#if defined(__SMP__) && defined(KTIMING)
void TraceStatsOnThreadExit(DThread* aT)
	{
	TUint64 rc = aT->iNThread.iRunCount.i64;
	NSchedulable::SCpuStats stats;
	NKern::Lock();
	aT->iNThread.GetCpuStats(NSchedulable::E_RunTime|NSchedulable::E_ActiveTime, stats);
	NKern::Unlock();
	TUint64 cputime = stats.iRunTime;
	TUint64 acttime = stats.iActiveTime;
	TUint32 f = NKern::CpuTimeMeasFreq();
	TUint64 avgcpu = rc ? cputime / rc : 0;
	TUint64 ratio = (acttime*100)/cputime;
	TUint64 cpud = tix2us(cputime, f);
	TUint64 actd = tix2us(acttime, f);
	TUint64 avgd = tix2us(avgcpu, f);
	Kern::Printf("Thread %O RC=%u CPU=%u.%06us ACT=%u.%06us AVG=%u.%06us RATIO=%d%%",
					aT, TUint32(rc),
					I64HIGH(cpud), I64LOW(cpud),
					I64HIGH(actd), I64LOW(actd),
					I64HIGH(avgd), I64LOW(avgd),
					TUint32(ratio));
	}
#endif

void DThread::Exit()
//
// This function runs in the context of the exiting thread
// Enter and leave with system unlocked
//
	{
#if defined(__SMP__) && defined(KTIMING)
	if (KDebugNum(KTIMING))
		TraceStatsOnThreadExit(this);
#endif
#ifdef KPANIC
	if (iExitType==EExitPanic)
		{
		__KTRACE_OPT2(KPANIC,KSCHED,Kern::Printf("Thread %O Panic %S %d",this,&iExitCategory,iExitReason));
		}
	else if (iExitType==EExitTerminate)
		{
		__KTRACE_OPT2(KPANIC,KSCHED,Kern::Printf("Thread %O Terminated %d",this,iExitReason));
		}
	else if (iExitType==EExitKill && iExitReason!=KErrNone)
		{
		__KTRACE_OPT2(KPANIC,KSCHED,Kern::Printf("Thread %O Killed %d",this,iExitReason));
		}
#endif
	if (iExitType!=EExitKill && (iFlags & (KThreadFlagSystemPermanent|KThreadFlagSystemCritical)))
		K::Fault(K::ESystemThreadPanic);
	if (iFlags & KThreadFlagSystemPermanent)
		K::Fault(K::EPermanentThreadExit);
	if (this==K::EventThread)
		K::Fault(K::EThrdEventHookDied);

	// call crash debugger after application panic if KALLTHREADSSYSTEM bit is set
	if (iExitType==EExitPanic && KDebugNum(KALLTHREADSSYSTEM))
		K::Fault(K::ESystemThreadPanic);

#if defined(_DEBUG) && !defined(__SMP__)
	// Delay cleanup if we're using the delayed scheduler
	if (KDebugNum(KCRAZYSCHEDDELAY))
		NKern::Sleep(1);
#endif
	
	// Clear any paging exc trap as if exiting this thread causes any page faults
	// the thread must not resume execution from the trap handler.
	iPagingExcTrap = 0;

	NKern::LockSystem();
	ReleaseWait(KErrDied);
	iExiting=ETrue;				// used to make logons complete early
	NKern::UnlockSystem();
	
	// regularize the thread state before main exit processing
	DoExit1();

#ifdef __EMI_SUPPORT__
	EMI::CallExitHandler(this);
#endif

	// Hook into debugger if any.  Not conditioned by __DEBUGGER_SUPPORT__ so 
	// minimal post-mortem debugging is possible even without full debugger support.
	DKernelEventHandler::Dispatch(EEventKillThread, this, NULL);

	TBool kill_process = (iFlags&KThreadFlagProcessPermanent) || ((iFlags&KThreadFlagProcessCritical) && iExitType!=EExitKill);
	DProcess* pP=iOwningProcess;
	if (kill_process)
		pP->Die((TExitType)iExitType,iExitReason,iExitCategory);

	__KTRACE_OPT(KTHREAD,Kern::Printf("Cancelling kernel message"));
	iKernMsg.Cancel();

	NKern::LockSystem();

	if (iIpcCount)
		{
		iIpcCount |= 0x80000000u;
		Open();
		}

	// clean up - release held mutexes etc.
	while(iCleanupQ.NonEmpty())
		{
		K::PINestLevel=0;
		TThreadCleanup* pCln=iCleanupQ.First();
		pCln->Cleanup();	// this also removes the cleanup entry
		NKern::FlashSystem();
		}
	NKern::UnlockSystem();

	// complete target logons
	TLogon::CompleteAll(iTargetLogons, TLogon::ETarget, iExitReason);

	// remove any outstanding owned logons
	TLogon::CompleteAll(iOwnedLogons, TLogon::EOwned, KErrNone);

	// remove any outstanding miscellaneous notifiers
	KillMiscNotifiers();

	// close the heap originally used by this thread
	CloseCreatedHeap();

	// release any CPU resources (eg coprocessors)
	DoExit2();

//#ifdef KPANIC
#if 0
	extern void DumpMemory(const char* aTitle, TLinAddr aStart, TLinAddr aSize);

	if (KDebugNum(KPANIC) && (iExitType!=EExitKill || iExitReason!=KErrNone))
		{
		Kern::Printf("Thread %O iSavedSP=%08x", this, iNThread.iSavedSP);
		DumpMemory("Supervisor Stack", TLinAddr(iSupervisorStack), iSupervisorStackSize);
		if (iUserStackRunAddress)
			{
			DumpMemory("User Stack", iUserStackRunAddress, iUserStackSize);
			}
		}
#endif
	
	// stop the M-state machine
	NKern::LockSystem();
	iMState=EDead;
	NKern::UnlockSystem();
	}

void DThread::DoExit1()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("Thread %O DoExit1",this));
	}

/**	Terminates the execution of a thread

	It terminates the specified thread and it sets its exit info.
	This is an asynchronous operation.

	This method can only be used by a thread to kill itself, or to kill
	a user thread (iThreadType==EThreadUser). An attempt to kill a non-user
	thread which is not the currently running thread will cause the system to fault
	with KERN 94 (ENonUserThreadKilled).

	@pre System Locked
	@post System Un-locked
*/
void DThread::Die(TExitType aType, TInt aReason, const TDesC& aCategory)
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED,"DThread::Die");				
	__KTRACE_OPT(KTHREAD,Kern::Printf("Thread %O Die: %d,%d,%S",this,aType,aReason,&aCategory));
	SetExitInfo(aType,aReason,aCategory);

	// If necessary, decrement count of running user threads in this process.  We get here if the
	// thread exited without calling User::Exit (eg panic, termination).
	if (iUserThreadState == EUserThreadCreated || iUserThreadState == EUserThreadRunning)
		{
		__e32_atomic_tas_ord32(&iOwningProcess->iUserThreadsRunning, 1, -1, 0);
		iUserThreadState = EUserThreadExiting;
		}
	
	if (this==TheCurrentThread)
		{
		SetDefaultPriority(KDefaultExitPriority);
		NKern::ThreadKill(&iNThread,SYSTEM_LOCK);	// this will not return
		K::Fault(K::EDeadThreadRunning);
		}
	if(iThreadType!=EThreadUser)
		K::Fault(K::ENonUserThreadKilled);
	NKern::ThreadKill(&iNThread);
	SetDefaultPriority(KDefaultExitPriority);
	NKern::UnlockSystem();
	}

void DThread::SvKill()
	{
	//
	// This function runs in the kernel server context
	// Enter and leave with system unlocked
	//
	__KTRACE_OPT(KTHREAD,Kern::Printf("Thread %O SvKill()",this));

	// move any queued user-mode callbacks to this thread and cancel them
	NKern::MoveUserModeCallbacks(NCurrentThread(), &iNThread);
	NKern::CancelUserModeCallbacks();	

	DProcess* pP=iOwningProcess;
	
	// take this thread off the process thread list
	Kern::MutexWait(*pP->iProcessLock);
	if (iProcessLink.iNext)
		{
		iProcessLink.Deque();
		iProcessLink.iNext=NULL;
		}
	TBool all_threads_gone=pP->iThreadQ.IsEmpty();
	Kern::MutexSignal(*pP->iProcessLock);

	// if all threads have exited, complete process logons
	if (all_threads_gone)
		{
		NKern::LockSystem();
		if (pP->iExitType == EExitPending)
			{
			// process has exited by last thread exiting, take its exit reason
			pP->iExitType = (TUint8)iExitType;
			pP->iExitReason = iExitReason;
			pP->iExitCategory = iExitCategory;
			}
		NKern::UnlockSystem();
		__KTRACE_OPT(KPROC,Kern::Printf("Completing process logons for %O",pP));
		TLogon::CompleteAll(pP->iTargetLogons, TLogon::ETarget, pP->iExitReason);
		}

	// notify undertakers + change notifiers
	Kern::NotifyChanges(EChangesThreadDeath);
	Kern::NotifyThreadDeath(this);

	// close handles, free stacks, close thread
	Release();

	// if all threads in process now dead, clean up process
	if (all_threads_gone)
		pP->Release();
	}

void DThread::AbortTimer(TBool aAbortAbsolute)
	{
	TInt typeMask=aAbortAbsolute?(TTimer::ELocked|TTimer::EAbsolute):(TTimer::ELocked);
	iTimer.Abort(this,typeMask);
	}


void DThread::SetPaging(TUint& aCreateFlags)
	{// Default implementation that doesn't allow threads to be paged.
	// It is virtual so can be overridden on memory models that allow data paging.
	__ASSERT_COMPILE(EThreadCreateFlagPagingUnspec == 0);
	aCreateFlags &= ~EThreadCreateFlagPagingMask;
	}


TInt DThread::Create(SThreadCreateInfo& aInfo)
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DThread::Create %S owner %O size %03x", &aInfo.iName,
																iOwningProcess, aInfo.iTotalSize));

	if (aInfo.iTotalSize < (TInt)sizeof(SThreadCreateInfo))
		return KErrArgument;
	if (aInfo.iTotalSize > KMaxThreadCreateInfo)
		return KErrArgument;
	SetOwner(iOwningProcess);
	__KTRACE_OPT(KTHREAD,Kern::Printf("Owner set"));
	TInt r=KErrNone;
	if (aInfo.iName.Length()!=0)
		{
		SetProtection(DObject::EGlobal);
		r=SetName(&aInfo.iName);
		__KTRACE_OPT(KTHREAD,Kern::Printf("Name set, %d",r));
		if (r!=KErrNone)
			return r;
		}
	else
		SetProtection(DObject::EProtected);

	iId = K::NewId();
	iThreadPriority=aInfo.iInitialThreadPriority;
	iThreadType=(TUint8)aInfo.iType;

	// Determine the data paging properties for the thread and create user stack.
	if (iThreadType == EThreadUser)
		{
		// This is a user side thread so aInfo will be a SStdEpocThreadCreateInfo.
		__NK_ASSERT_DEBUG(aInfo.iTotalSize == sizeof(SStdEpocThreadCreateInfo));
		SStdEpocThreadCreateInfo& info = (SStdEpocThreadCreateInfo&)aInfo;
		SetPaging(info.iFlags);	// Will update info.iFlags to have paged or unpaged set.
		r = AllocateUserStack(info.iUserStackSize, info.iFlags & EThreadCreateFlagPaged);
		if (r != KErrNone)
			return r;
		}

	// inherit system critical + process critical from process (implements AllThreasdCritical)
	if (iThreadType == EThreadInitial || iThreadType == EThreadAPInitial)
		iFlags |= KThreadFlagSystemPermanent; // initial thread can't exit for any reason
	else if (iThreadType == EThreadUser)
		iFlags |= (iOwningProcess->iFlags & KThreadFlagProcessCritical);
	else
		iFlags |= KThreadFlagSystemCritical; // kernel threads can't panic

	// create platform-dependent stuff
	r=DoCreate(aInfo);
	if (r!=KErrNone)
		return r;
	r = TThreadWaitList::ThreadCreated();
	if (r!=KErrNone)
		return r;
	iWaitListReserved = 1;
	if (iThreadType!=EThreadInitial && iThreadType!=EThreadMinimalSupervisor)
		r=K::Containers[EThread]->Add(this);
	return r;
	}

TInt DThread::SetPriority(TThreadPriority aPriority)
	{
	TInt tp=0;
	switch(aPriority)
		{
		case EPriorityMuchLess: tp=EThrdPriorityMuchLess; break;
		case EPriorityLess: tp=EThrdPriorityLess; break;
		case EPriorityNormal: tp=EThrdPriorityNormal; break;
		case EPriorityMore: tp=EThrdPriorityMore; break;
		case EPriorityMuchMore: tp=EThrdPriorityMuchMore; break;
		case EPriorityRealTime: tp=EThrdPriorityRealTime; break;
		case EPriorityAbsoluteVeryLow: tp=EThrdPriorityAbsoluteVeryLow; break;
		case EPriorityAbsoluteLowNormal: tp=EThrdPriorityAbsoluteLowNormal; break;
		case EPriorityAbsoluteLow: tp=EThrdPriorityAbsoluteLow; break;
		case EPriorityAbsoluteBackgroundNormal: tp=EThrdPriorityAbsoluteBackgroundNormal; break;
		case EPriorityAbsoluteBackground: tp=EThrdPriorityAbsoluteBackground; break;
		case EPriorityAbsoluteForegroundNormal: tp=EThrdPriorityAbsoluteForegroundNormal; break;
		case EPriorityAbsoluteForeground: tp=EThrdPriorityAbsoluteForeground; break;
		case EPriorityAbsoluteHighNormal: tp=EThrdPriorityAbsoluteHighNormal; break;
		case EPriorityAbsoluteHigh: tp=EThrdPriorityAbsoluteHigh; break;
		case EPriorityAbsoluteRealTime1: tp=EThrdPriorityAbsoluteRealTime1; break;
		case EPriorityAbsoluteRealTime2: tp=EThrdPriorityAbsoluteRealTime2; break;
		case EPriorityAbsoluteRealTime3: tp=EThrdPriorityAbsoluteRealTime3; break;
		case EPriorityAbsoluteRealTime4: tp=EThrdPriorityAbsoluteRealTime4; break;
		case EPriorityAbsoluteRealTime5: tp=EThrdPriorityAbsoluteRealTime5; break;
		case EPriorityAbsoluteRealTime6: tp=EThrdPriorityAbsoluteRealTime6; break;
		case EPriorityAbsoluteRealTime7: tp=EThrdPriorityAbsoluteRealTime7; break;
		case EPriorityAbsoluteRealTime8: tp=EThrdPriorityAbsoluteRealTime8; break;
		default:
			K::PanicCurrentThread(EBadPriority);
		}
	SetThreadPriority(tp);
	return KErrNone;
	}

#ifndef __REQUEST_COMPLETE_MACHINE_CODED__

void DThread::RequestComplete(TRequestStatus*& aStatus, TInt aReason)
//
// Signal this threads request semaphore.
// Enter with system locked, return with system unlocked.
//
	{

	__KTRACE_OPT(KDATAPAGEWARN,Kern::Printf("Data paging: Use of deprecated DThread::RequestComplete API by %O", TheCurrentThread));
	TRequestStatus *theStatus=aStatus;
	aStatus=NULL;	// to indicate that this request has been completed
	__KTRACE_OPT2(KTHREAD,KSEMAPHORE,Kern::Printf("DThread::RequestComplete %O %d->%08x",this,aReason,theStatus));
	
#ifndef __MEMMODEL_FLEXIBLE__

	TInt r = KErrDied;
	if (iMState!=EDead)
		{
		TIpcExcTrap xt;
		xt.iLocalBase=0;
		xt.iRemoteBase=(TLinAddr)theStatus;
		xt.iSize=sizeof(TInt);
		xt.iDir=1;
		r=xt.Trap(this);
		if (r==KErrNone)
			{
			// On some memory models(such as moving), RawRead may update the content of xt. It
			// happens if home address is accessed (instead of the provided run address) or if it
			// reads/writes in chunks.
			r=RawWrite(theStatus,&aReason,sizeof(TInt),0,this,&xt);
			xt.UnTrap();
			}
		}
	
	if (r == KErrNone)
		{
#ifdef BTRACE_REQUESTS
		BTraceContext12(BTrace::ERequests,BTrace::ERequestComplete,&this->iNThread,theStatus,aReason);
#endif
		NKern::ThreadRequestSignal(&iNThread, SYSTEM_LOCK);
		}
	else
		NKern::UnlockSystem();

#else

	NKern::UnlockSystem();
	if (iMState!=EDead && Kern::ThreadRawWrite(this,theStatus,&aReason,sizeof(TInt))==KErrNone)
		{
#ifdef BTRACE_REQUESTS
		BTraceContext12(BTrace::ERequests,BTrace::ERequestComplete,&this->iNThread,theStatus,aReason);
#endif
		NKern::ThreadRequestSignal(&iNThread);
		}

#endif
	}

#endif

#if !defined(__REQUEST_COMPLETE_MACHINE_CODED__) || defined(__MEMMODEL_FLEXIBLE__)

/**	Write back a completion code and signal a thread's request semaphore, indicating that an asynchronous request has completed.

	@param aThread Thread to be signaled
	@param aStatus A TRequestStatus instance that will receive the request status code.
				   It must reside in user side address space.
	@param aReason Request status code. KErrCancel indicates the request has been canceled.

	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
	
	@deprecated
*/
EXPORT_C void Kern::RequestComplete(DThread* aThread, TRequestStatus*& aStatus, TInt aReason)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::RequestComplete");
	__KTRACE_OPT(KDATAPAGEWARN,Kern::Printf("Data paging: Use of deprecated Kern::RequestComplete API by %O", TheCurrentThread));
	NKern::LockSystem();
	if (aStatus)
		aThread->RequestComplete(aStatus,aReason);
	else
		NKern::UnlockSystem();
	}

#endif

#ifndef __REQUEST_COMPLETE_MACHINE_CODED__

/**	Complete a request made by the current thread.

 	This writes the completion code and signals the current thread's request semaphore, indicating
 	that an asynchronous request has completed.

 	Note that this must only be called in the context of the thread that made the request.

	@param aStatus A TRequestStatus instance that will receive the request status code.
				   It must reside in the user side address space of the current thread.
	@param aReason Request status code. KErrCancel indicates the request has been canceled.

	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled

	@publishedPartner
	@released
*/
EXPORT_C void Kern::RequestComplete(TRequestStatus*& aStatus, TInt aReason)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED | MASK_INTERRUPTS_ENABLED | MASK_NOT_ISR | MASK_NOT_IDFC,"Kern::RequestComplete");
	TRequestStatus* status = (TRequestStatus*)__e32_atomic_swp_rel_ptr(&aStatus, 0);
	if (status && KUSafeWrite(status, &aReason, sizeof(aReason)) == NULL)
		NKern::ThreadRequestSignal(NKern::CurrentThread());
	}

#endif

/********************************************
 * User heap
 ********************************************/

class RUserAllocator : public RAllocator
	{
public:
	TBool Close();
	TInt* GetHandleList(TInt& aCount);
	};

// Decrement the object's reference count and return true if it reached zero
TBool RUserAllocator::Close()
	{
	return Kern::KUSafeDec(iAccessCount) == 1;
	}

TInt* RUserAllocator::GetHandleList(TInt& aCount)
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("RUserAllocator::GetHandleList() %08x", this));
	TInt* h[2] = {NULL, NULL};
	if (Close())
		kumemget32(h, &iHandleCount, sizeof(h));
	aCount = (TInt)h[0];
	return h[1];
	}

#ifdef __USERSIDE_THREAD_DATA__
void TLocalThreadData::Close()
	{
	RUserAllocator* tlsHeap;
	kumemget32(&tlsHeap, &iTlsHeap, sizeof(tlsHeap));
	if (tlsHeap)
		tlsHeap->Close();
	}
#endif

void DThread::CloseCreatedHeap()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DThread::CloseCreatedHeap() %O",this));
	TInt r;
#ifdef __USERSIDE_THREAD_DATA__
	if (iFlags & KThreadFlagLocalThreadDataValid)
		{
		TLocalThreadData* threadData = (TLocalThreadData*)(iUserStackRunAddress + iUserStackSize - KLocalThreadDataSize);
		XTRAP(r, XT_DEFAULT, threadData->Close());
		__KTRACE_OPT(KTHREAD, Kern::Printf("TLocalThreadData::Close() r=%d", r));
		if (r != KErrNone)
			return;
		}
#endif
	RUserAllocator* pA = (RUserAllocator*)__e32_atomic_swp_ord_ptr(&iCreatedAllocator, 0);
	if (!pA)
		return;
	TInt c = 0;
	TInt* h = NULL;
	XTRAP(r, XT_DEFAULT, h = pA->GetHandleList(c));
	__KTRACE_OPT(KTHREAD,Kern::Printf("RUserAllocator::SvClose() r=%d c=%d",r,c));
	if (r!=KErrNone || (TUint)c>(TUint)RAllocator::EMaxHandles)
		return;

	TInt handle[RAllocator::EMaxHandles];
	XTRAP(r, XT_DEFAULT, kumemget32(handle, h, c*sizeof(TInt)));
	__KTRACE_OPT(KTHREAD,Kern::Printf("RUserAllocator::SvClose() r=%d h=%08x",r,h));
	if (r!=KErrNone)
		return;
	h = handle;
	TInt* e = h + c;
	for (; h<e; ++h)
		{
		if (*h)
			HandleClose(*h);
		}
	}

TInt DThread::SetTls(TInt aHandle, TInt aDllUid, TAny* aPtr)
//
// Set the Thread Local Storage variable for a DLL.
//
	{
	STls tls;
	tls.iHandle = aHandle;
	tls.iDllUid = aDllUid;
	tls.iPtr = aPtr;
	TInt i;
	TInt r=iTls.FindInSignedKeyOrder(tls,i);
	if (r==KErrNone)
		{
		iTls[i] = tls;
		return KErrNone;
		}
	return iTls.Insert(tls,i);
	}

TAny* DThread::Tls(TInt aHandle, TInt aDllUid)
//
// Retrieve the Thread Local Storage variable for a DLL.
//
	{
	STls tls;
	tls.iHandle=aHandle;
	TInt r=iTls.FindInSignedKeyOrder(tls);
	if (r>=0 && iTls[r].iDllUid==aDllUid)
		return iTls[r].iPtr;
	return NULL;
	}

void DThread::FreeTls(TInt aHandle)
//
// Remove the Thread Local Storage variable for a DLL.
//
	{
	STls tls;
	tls.iHandle=aHandle;
	TInt r=iTls.FindInSignedKeyOrder(tls);
	if (r>=0)
		{
		iTls.Remove(r);
		iTls.Compress();
		}
	}

TInt DThread::Rename(const TDesC& aName)
	{
	TKName n;
	Name(n);				// get current name
	if (n.MatchF(aName)==0)
		return KErrNone;	// new name is the same so nothing to do
	K::Containers[EThread]->Wait();
	TInt r=K::Containers[EThread]->CheckUniqueFullName(this,aName);
	if (r==KErrNone)
		{
		__KTRACE_OPT(KTHREAD,Kern::Printf("DThread::Rename %O to %S",this,&aName));
		r=SetName(&aName);
#ifdef BTRACE_THREAD_IDENTIFICATION
		Name(n);
		BTraceN(BTrace::EThreadIdentification,BTrace::EThreadName,&iNThread,iOwningProcess,n.Ptr(),n.Size());
#endif
		}
	K::Containers[EThread]->Signal();
	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateThread, this);
	return(r);
	}


/**
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.
	@pre System must be locked
	@pre Call in a thread context.
*/
TInt DThread::GetDesInfo(const TAny* aDes, TInt& aLength, TInt& aMaxLength, TUint8*& aPtr, TBool aWriteable)
//
// Get remote descriptor info.
// Enter and leave with system locked
//
	{
#ifndef __MEMMODEL_FLEXIBLE__
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED|MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,
		"DThread::GetDesInfo");
#else
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,
		"DThread::GetDesInfo");
#endif
	TDesHeader d;
	TInt r=ReadAndParseDesHeader(aDes,d);
	if (r!=KErrNone)
		return r;
	aLength = d.Length();
	aPtr = (TUint8*)d.DataPtr();
	if (d.IsWriteable())
		aMaxLength = d.MaxLength();
	else
		{
		if (aWriteable)
			return KErrBadDescriptor;
		aMaxLength = 0;
		}			
	return KErrNone;
	}

/**
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.
	@pre System must be locked
	@pre Call in a thread context.
*/
TInt DThread::GetDesLength(const TAny* aPtr)
//
// Get the length of a descriptor.
// Enter and leave with system locked
//
	{
#ifndef __MEMMODEL_FLEXIBLE__
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED|MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,
		"DThread::GetDesLength");
#else
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,
		"DThread::GetDesLength");
#endif
	TDesHeader d;
	TInt r=ReadAndParseDesHeader(aPtr,d);
	if (r<0)
		return r;
	return d.Length();
	}

/**
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.
	@pre System must be locked
	@pre Call in a thread context.
*/
TInt DThread::GetDesMaxLength(const TAny* aPtr)
//
// Get the maximum length of a descriptor.
// Enter and leave with system locked
//
	{
#ifndef __MEMMODEL_FLEXIBLE__
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED|MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,
		"DThread::GetDesMaxLength");
#else
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,
		"DThread::GetDesMaxLength");
#endif
	TDesHeader d;
	TInt r=ReadAndParseDesHeader(aPtr,d);
	if (r!=KErrNone)
		return r;
	return d.MaxLength();
	}

/**
Reads from this thread's address space.
Enter and leave with system locked

@param aPtr Points to the source descriptor. It must reside in this thread's address space.
@param aDest Points to the destination buffer, which is in the current process's address space.
@param aMax Specifies maximum number of characters to read.
@param anOffset The offset in the source descriptor to copy from.
@param aMode if orred by KCheckLocalAddress, aDest memory in the current process will be accessed with user attributes.
@return nonnegative value indicates the number of characters transferred.
        KErrDied if this thread has died.
		KErrBadDescriptor if the attempt to read aPtr descriptor caused the exception.

@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre System must be locked
@pre Call in a thread context.
*/
TInt DThread::DesRead(const TAny* aPtr, TUint8* aDest, TInt aMax, TInt anOffset, TInt aMode)
	{
#ifndef __MEMMODEL_FLEXIBLE__
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED|MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,
		"DThread::DesRead");
#else
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,
		"DThread::DesRead");
#endif
	TDesHeader d;
	TInt r=ReadAndParseDesHeader(aPtr,d);
	if (r!=KErrNone)
		return r;
	return DoDesRead(d, aDest, aMax, anOffset, aMode);
	}

// Read descriptor from thread's address space, given the descriptor header
TInt DThread::DoDesRead(const TDesHeader& aDesInfo, TUint8* aDest, TInt aMax, TInt anOffset, TInt aMode)
	{
	if (anOffset<0 || aMax<0)
		return KErrArgument;
	TInt len=aDesInfo.Length();
	len-=anOffset;
	if (len<0)
		len=0;
	if (len>aMax)
		len=aMax;
	TInt l=len;
	if (aMode & KChunkShiftBy1)
		{
		l<<=1;
		anOffset<<=1;
		}
	const TUint8* pS = (TUint8*)aDesInfo.DataPtr() + anOffset;
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::FlashSystem();
#endif
	TIpcExcTrap xt;
	xt.iLocalBase=(TLinAddr)aDest;
	xt.iRemoteBase=(TLinAddr)pS;
	xt.iSize=l;
	xt.iDir=0;
	TInt r=xt.Trap(this);
	if (r==0)
		{
		//On some memory models(such as moving), RawRead may update the content of xt. It happens if home address
		// is accessed (instead of the provided run address) or if it reads in chunks.
		r=RawRead(pS,aDest,l,aMode&KCheckLocalAddress, &xt);
		xt.UnTrap();
		}
	return (r<0)?r:len;
	}

/**
Write to the thread's address space.
Enter and leave with system locked

@param aPtr points to descriptor to write to. It is in remote (this thread's) address space.
@param aSrc points to source buffer, which is in the current process's address space
@param aLength is number of characters to copy
@param anOffset The offset in aPtr descriptor where to start writing.
@param aOrigThread The thread on behalf of which this operation is performed (eg client of device driver). If NULL, current thread is assumed.
@return KErrDied if this thread has died.
		KErrBadDescriptor if the attempt to read aPtr descriptor caused the exception.
		KErrNone otherwise

@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre System must be locked
@pre Call in a thread context.
*/
TInt DThread::DesWrite(const TAny* aPtr, const TUint8* aSrc, TInt aLength, TInt anOffset, TInt aMode, DThread* anOriginatingThread)
	{
#ifndef __MEMMODEL_FLEXIBLE__
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED|MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,
		"DThread::DesWrite");
#else
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,
		"DThread::DesWrite");
#endif
	TDesHeader d;
	TInt r=ReadAndParseDesHeader(aPtr, d);
	if (r!=KErrNone)
		return r;
	r = DoDesWrite(aPtr, d, aSrc, aLength, anOffset, aMode, anOriginatingThread);
	return r < KErrNone ? r : KErrNone;
	}


// Write descriptor to thread's address space, given the descriptor header
// Return the new flags + length word, or one of the system-wide error codes  
TInt DThread::DoDesWrite(const TAny* aPtr, TDesHeader& aDesInfo, const TUint8* aSrc, TInt aLength, TInt anOffset, TInt aMode, DThread* anOriginatingThread)
	{
	if (anOffset<0 || aLength<0)
		return KErrArgument;
	if (!aDesInfo.IsWriteable())
		return KErrBadDescriptor;
	TUint maxLen=aDesInfo.MaxLength();
	TUint8* pT=(TUint8*)aDesInfo.DataPtr();	// remote descriptor Ptr()
	TUint finalLen=aLength+anOffset;
	if (finalLen>maxLen)
		{
		if (aMode & KTruncateToMaxLength)
			{
			aLength=maxLen-anOffset;
			finalLen=maxLen;
			if (aLength<0)
				return KErrOverflow;
			}
		else
			return KErrOverflow;
		}
	TInt type=aDesInfo.Type();
	TUint typelen=TUint(type)<<KShiftDesType | TUint(finalLen);
	TInt shift=(aMode&KChunkShiftBy1)?1:0;
	anOffset<<=shift;
	aLength<<=shift;
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::FlashSystem();
#endif
	TIpcExcTrap xt;
	xt.iDir=1;
	TInt r=xt.Trap(this);
	if (r==0)
		{
		xt.iLocalBase=(TLinAddr)aSrc;
		xt.iRemoteBase=(TLinAddr)(pT+anOffset);
		xt.iSize=aLength;
		if (aLength)
			{
			//On some memory models(such as moving), RawRead may update the content of xt. It happens if home address
			// is accessed (instead of the provided run address) or if it reads in chunks.
			r=RawWrite(pT+anOffset,aSrc,aLength,aMode&KCheckLocalAddress,anOriginatingThread, &xt);
			}
		if ((aMode & KDoNotUpdateDesLength) == 0)
			{
			if (r == KErrNone && type == EBufCPtr)
				{
				TUint8* pL = pT - sizeof(TDesC);
				xt.iLocalBase=0;
				xt.iRemoteBase=(TLinAddr)pL;
				xt.iSize=sizeof(finalLen);
				r=RawWrite(pL,&finalLen,sizeof(finalLen),0,anOriginatingThread, &xt);
				}
			xt.iLocalBase=0;
			xt.iRemoteBase=(TLinAddr)aPtr;
			xt.iSize=sizeof(finalLen);
			if (r==KErrNone)
				r=RawWrite(aPtr,&typelen,sizeof(typelen),0,anOriginatingThread, &xt);
			}
		xt.UnTrap();
		}
	return r == KErrNone ? typelen : r;
	}

TBool DThread::IsExceptionHandled(TExcType aType)
	{
	if (iExceptionHandler==NULL)
		return EFalse;
	if (iFlags&KThreadFlagLastChance)
		return EFalse;
	switch (aType)
		{
	case EExcGeneral:
	case EExcUserInterrupt:
		return iExceptionMask & KExceptionUserInterrupt;
	case EExcIntegerDivideByZero:
	case EExcIntegerOverflow:
		return iExceptionMask & KExceptionInteger;
	case EExcSingleStep:
	case EExcBreakPoint:
		return iExceptionMask & KExceptionDebug;
	case EExcBoundsCheck:
	case EExcInvalidOpCode:
	case EExcDoubleFault:
	case EExcStackFault:
	case EExcAccessViolation:
	case EExcPrivInstruction:
	case EExcAlignment:
	case EExcPageFault:
		return iExceptionMask & KExceptionFault;
	case EExcFloatDenormal:
	case EExcFloatDivideByZero:
	case EExcFloatInexactResult:
	case EExcFloatInvalidOperation:
	case EExcFloatOverflow:
	case EExcFloatStackCheck:
	case EExcFloatUnderflow:
		return iExceptionMask & KExceptionFpe;
	case EExcAbort:
		return iExceptionMask & KExceptionAbort;
	case EExcKill:
		return iExceptionMask & KExceptionKill;
	default:
		return EFalse;
		}
	}

/**
Terminates the current thread.

@param aReason Reason to be set in the current thread's exit information.

@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre Calling thread must not be in a critical section
@pre Can be used in a device driver.

@post It doesn't return.
*/
EXPORT_C void Kern::Exit(TInt aReason)
	{
//	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD|MASK_NO_CRITICAL,"Kern::Exit");
#ifdef _DEBUG
#if (defined (__KERNEL_APIS_CONTEXT_CHECKS_WARNING__)||defined (__KERNEL_APIS_CONTEXT_CHECKS_FAULT__))
	if (TheCurrentThread->iThreadType==EThreadUser)
		{
		CHECK_PRECONDITIONS(MASK_THREAD_STANDARD|MASK_NO_CRITICAL,"Kern::Exit");
		}
	else
		{
		CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::Exit");
		}
#endif
#endif

	NKern::LockSystem();
	TheCurrentThread->Die(EExitKill,aReason,KNullDesC);
	}

#ifdef KTHREAD
TInt DThread::RaiseException(TExcType aType)
#else
TInt DThread::RaiseException(TExcType)
#endif
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("Thread %O RaiseException(%d)",this,aType));
	if (this==TheCurrentThread)
		return KErrGeneral;
	else if (iThreadType!=EThreadUser)
		return KErrAccessDenied;
	return KErrNotSupported;
	}

void DThread::SetExitInfo(TExitType aType, TInt aReason, const TDesC& aCategory)
//
// Set a thread's exit info.
// Called with system locked.
//
	{
	if (iExitType==EExitPending)
		{
		if (iOwningProcess->iExitType!=EExitPending)
			{
			// process has already exited, so take its exit info.
			iExitType=iOwningProcess->iExitType;
			iExitReason=iOwningProcess->iExitReason;
			iExitCategory=iOwningProcess->iExitCategory;
			return;
			}
		iExitType=(TUint8)aType;
		iExitReason=aReason;
		if (iExitType==EExitKill)
			iExitCategory=KLitKill;
		else if (iExitType==EExitTerminate)
			iExitCategory=KLitTerminate;
		else if (aType==EExitPanic)
			iExitCategory=aCategory;
		}
	}

void DThread::CleanupLeave(TInt aDepth)
//
// Enter and return with system unlocked and calling thread in critical section.
//
	{
	__ASSERT_CRITICAL;
	__NK_ASSERT_DEBUG(iLeaveDepth);

	DProcess* pP = iOwningProcess;

	iLeaveDepth -= aDepth;

	if ((!iLeaveDepth) && (__e32_atomic_add_ord32(&pP->iThreadsLeaving, TUint32(-1)) == 1) && (!pP->iGarbageList.IsEmpty()))
		{
		DCodeSeg::Wait();

		// Avoid race condition where code segs are put onto garbage list by another thread leaving,
		// between the decrement of iThreadsLeaving above and aquiring the code seg mutex
		if (!pP->iThreadsLeaving)
			{
			while (!pP->iGarbageList.IsEmpty())
				{
				DLibrary* pL = _LOFF(pP->iGarbageList.First()->Deque(), DLibrary, iGbgLink);
				__NK_ASSERT_DEBUG(pL->iAccessCount);
				pL->iGbgLink.iNext = NULL;

				pL->ReallyRemoveFromProcess();
				pL->DObject::Close(NULL);
				}
			}

		DCodeSeg::Signal();
		}
	}

/** Function only temporarily supported to aid migration to process emulation...
*/
TInt ExecHandler::ThreadAsProcess(DThread* aThread, TInt aLibrary)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("ExecHandler::ThreadAsProcess %O",aThread));
	if (aThread->iOwningProcess->iSecurityZone!=TheCurrentThread->iOwningProcess->iSecurityZone)
		K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Use of RThread::Create(const TDesC& aName,TThreadFunction aFunction,TInt aStackSize,TAny* aPtr,RLibrary* aLibrary,RHeap* aHeap, TInt aHeapMinSize,TInt aHeapMaxSize,TOwnerType aType)"));
	DObject* pL = K::ObjectFromHandle(aLibrary,ELibrary);
	pL->CheckedOpen();
	TInt r = aThread->Open();
	NKern::ThreadEnterCS();
	NKern::UnlockSystem();
	if(r==KErrNone)
		{
		TInt h;
		r = aThread->MakeHandleAndOpen(EOwnerThread,pL,h);
		aThread->Close(NULL);
		}
	pL->Close(NULL);
	NKern::ThreadLeaveCS();
	return r;
	}

/**
Sets the realtime state for the current thread.

@param aNewState The new realtime state for the thread.

@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre Interrupts enabled
@pre Can be used in a device driver.
*/
EXPORT_C void Kern::SetRealtimeState(TThreadRealtimeState aNewState)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::SetRealtimeState");
	TheCurrentThread->SetRealtimeState(aNewState);
	}

void DThread::SetRealtimeState(TThreadRealtimeState aState)
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("Thread %O SetRealtimeState %d", this, aState));
	const TUint32 clear = KThreadFlagRealtime | KThreadFlagRealtimeTest;
	TUint32 set = 0;
	switch (aState)
		{
		case ERealtimeStateOff:		set = 0;											break;
		case ERealtimeStateOn:		set = KThreadFlagRealtime;							break;
		case ERealtimeStateWarn:	set = KThreadFlagRealtime|KThreadFlagRealtimeTest;	break;

		default:
			K::PanicCurrentThread(EInvalidRealtimeState);
		}
	NKern::LockSystem();
	iFlags=(iFlags & ~clear) | set;	
	NKern::UnlockSystem();
	}

TBool DThread::IsRealtime()
	{
	if(iFlags&KThreadFlagRealtime)
		return ETrue;
	return EFalse;
	}

void K::CheckThreadNotRealtime(const char* aTraceMessage)
	{
	if(TheCurrentThread->IsRealtime())
		IllegalFunctionForRealtimeThread(NULL,aTraceMessage);
	}

/**
Panic the specified thread with EIllegalFunctionForRealtimeThread.
If the thread is the current one, and it is in a critical section, then the panic
will be deferred until the critical section has been left.
@return True if thread was/will be panicked. False if function should be made 'legal' again.
@pre No fast mutexes held
*/
TBool K::IllegalFunctionForRealtimeThread(DThread* aThread,const char* aTraceMessage)
	{
	DThread* pC=TheCurrentThread;
	if(!aThread)
		aThread = pC;

	// If current thread has RealtimeTest just emit a warning trace...
	if(pC->iFlags&KThreadFlagRealtimeTest)
		{
		__KTRACE_OPT(KREALTIME,Kern::Printf("*Realtime* WARNING - Illegal function for Thread %O - %s",aThread,aTraceMessage));
#ifndef _DEBUG
		(void)aTraceMessage; // stop unreferenced formal parameter warning
#endif
		return EFalse;
		}

	// Kill the thread...
	__KTRACE_OPT2(KREALTIME,KPANIC,Kern::Printf("*Realtime* ERROR - Illegal function for Thread %O - %s",aThread,aTraceMessage));
	NKern::LockSystem();
	if(aThread!=pC || aThread->iNThread.iCsCount==0)
		{
		aThread->Die(EExitPanic,EIllegalFunctionForRealtimeThread,KLitKernExec());
		}
	else
		{
		// Want to kill current thread but it is in a critical section, so poke the thread
		// with state so it panics when it leaves the critical section
		aThread->SetExitInfo(EExitPanic,EIllegalFunctionForRealtimeThread,KLitKernExec());
		NKern::DeferredExit();
		NKern::UnlockSystem();
		}
	return ETrue;
	}

void DThread::BTracePrime(TInt aCategory)
	{
#ifdef BTRACE_THREAD_IDENTIFICATION
	if(aCategory==BTrace::EThreadIdentification || aCategory==-1)
		{
		DProcess* process = iOwningProcess;
		TKName nameBuf;
		process->Name(nameBuf);
		BTraceN(BTrace::EThreadIdentification,BTrace::EProcessName,&iNThread,process,nameBuf.Ptr(),nameBuf.Size());
		Name(nameBuf);
		BTraceN(BTrace::EThreadIdentification,BTrace::EThreadName,&iNThread,process,nameBuf.Ptr(),nameBuf.Size());
		BTrace12(BTrace::EThreadIdentification,BTrace::EThreadId,&iNThread,process,iId);
		}
#endif
	}

/******************************************************************************
 * Miscellaneous notifications
 ******************************************************************************/
TMiscNotifierMgr::TMiscNotifierMgr()
	:	iCompletionDfc(&CompletionDfcFn, this, 1),
		iIdleNotifierQ(0),
		iIdleDfc(&IdleDfcFn, this, 1)
	{
	}

void TMiscNotifierMgr::Init2()
	{
	iCompletionDfc.SetDfcQ(K::SvMsgQ);
	iIdleDfc.SetDfcQ(K::SvMsgQ);
	iIdleNotifierQ = new SMiscNotifierQ;
	__NK_ASSERT_ALWAYS(iIdleNotifierQ);
	}

// runs in supervisor thread when completions are required
void TMiscNotifierMgr::CompletionDfcFn(TAny* aMgr)
	{
	__KTRACE_OPT(KOBJECT,Kern::Printf("TMiscNotifierMgr::CompletionDfcFn"));
	TMiscNotifierMgr& m = *(TMiscNotifierMgr*)aMgr;
	m.Lock();
	while (!m.iCompleted.IsEmpty())
		{
		TMiscNotifier* n = _LOFF(m.iCompleted.First()->Deque(), TMiscNotifier, iObjLink);
		n->iThreadLink.Deque();
		m.Unlock();
		__KTRACE_OPT(KOBJECT,Kern::Printf("Completing %08x thread %O 0->%08x", n, n->iThread, n->iStatus));
		DThread* t = n->iThread;
		Kern::QueueRequestComplete(t, n, KErrNone);
		t->Close(NULL);
		n->Close();
		m.Lock();
		}
	m.Unlock();
	}

// runs in supervisor thread when idle notification completions are required
void TMiscNotifierMgr::IdleDfcFn(TAny* aMgr)
	{
	TMiscNotifierMgr& m = *(TMiscNotifierMgr*)aMgr;
	m.CompleteNotifications(*m.iIdleNotifierQ);
	}

void TMiscNotifierMgr::CompleteNotifications(SDblQue& aQ)
	{
	__KTRACE_OPT(KOBJECT,Kern::Printf("TMiscNotifierMgr::CompleteNotifications(%08x)",&aQ));
	Lock();
	TBool queue_dfc = EFalse;
	if (!aQ.IsEmpty())
		{
		// remember if completed list was originally empty
		queue_dfc = iCompleted.IsEmpty();

		// move items on provided list onto completed list
		iCompleted.MoveFrom(&aQ);
		}
	Unlock();
	if (queue_dfc)
		iCompletionDfc.Enque();
	}


// Create and install a miscellanous notifier
// Return 1 if the list was originally empty
// Return 0 if success but list not originally empty
// Return <0 on error
// If aN is non-null on entry and an TMiscNotifier is required, *aN is used instead
// of allocating memory. aN is then set to NULL. Similarly for aQ.
// If aObj is TRUE, aPtr points to a DObject to which the notifier should be attached
// If aObj is FALSE, aPtr points to a SMiscNotifierQ pointer
TInt TMiscNotifierMgr::NewMiscNotifier(TRequestStatus* aStatus, TBool aObj, TAny* aPtr, TMiscNotifier*& aN, SMiscNotifierQ*& aQ)
	{
	TInt result = 0;
	DThread& t = *TheCurrentThread;
	DObject* pO = aObj ? (DObject*)aPtr : 0;
	SMiscNotifierQ** pQ = aObj ? 0 : (SMiscNotifierQ**)aPtr;
	TMiscNotifier* n = aN;
	if (!n)
		n = new TMiscNotifier;
	if (!n)
		return KErrNoMemory;
	__KTRACE_OPT(KEXEC,Kern::Printf("NewMiscNotifier at %08x, status=%08x, pO=%O, pQ=%08x",n,aStatus,pO,pQ));
	TInt r = n->SetStatus(aStatus);
	__NK_ASSERT_DEBUG(r == KErrNone);  // can't fail
	(void)r;
	n->iThread = &t;
	SMiscNotifierQ* newq = 0;
	SMiscNotifierQ* q = 0;
	TBool newq_reqd = pO ? (!pO->HasNotifierQ()) : (!*pQ);
	FOREVER
		{
		if (!newq && newq_reqd)
			{
			newq = aQ;
			if (!newq)
				newq = new SMiscNotifierQ;
			if (!newq)
				{
				if (n != aN)
					n->Close();
				return KErrNoMemory;
				}
			}
		Lock();
		q = pO ? (pO->NotifierQ()) : (*pQ);
		if (!q)
			{
			if (!newq)
				{
				Unlock();
				newq_reqd = ETrue;
				continue;
				}
			q = newq;
			if (q == aQ)
				aQ = 0;
			newq = 0;
			if (pO)
				pO->SetNotifierQ(q);
			else
				*pQ = q;
			}
		if (q->IsEmpty())
			result = 1;
		q->Add(&n->iObjLink);
		t.iMiscNotifiers.Add(&n->iThreadLink);
		Unlock();
		break;
		}
	if (newq && newq!=aQ)
		Kern::Free(newq);
	if (n == aN)
		aN = 0;
	t.Open();
	return result;
	}


void ExecHandler::NotifyOnIdle(TRequestStatus* aStatus)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("ExecHandler::NotifyOnIdle %08x",aStatus));
	TMiscNotifierMgr& m = K::TheMiscNotifierMgr;
	NKern::ThreadEnterCS();
	// FIXME: TIME ORDERING PROBLEM IF IDLE DFC IS CURRENTLY PENDING
	TMiscNotifier* n = 0;
	SMiscNotifierQ* q = 0;
	TInt r = m.NewMiscNotifier(aStatus, FALSE, &K::TheMiscNotifierMgr.iIdleNotifierQ, n, q);
	if (r<0)
		Kern::RequestComplete(aStatus, r);
	else if (r>0)
		m.iIdleDfc.QueueOnIdle();
	NKern::ThreadLeaveCS();
	}


// Find all this thread's miscellaneous notifiers with request status equal to aStatus
// Dequeue them from both the thread and the object they monitor and place them in
// an output doubly linked list.
// If aStatus==NULL, extract all this thread's miscellaneous notifiers
void DThread::ExtractMiscNotifiers(SDblQue& aQ, TRequestStatus* aStatus)
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("%T ExtractMiscNotifiers(%08x)", this, aStatus));
	TMiscNotifierMgr& m = K::TheMiscNotifierMgr;
	SDblQueLink* anchor = &iMiscNotifiers.iA;
	m.Lock();
	SDblQueLink* p = anchor->iNext;
	for (; p!=anchor; p=p->iNext)
		{
		TMiscNotifier* n = _LOFF(p, TMiscNotifier, iThreadLink);
		if (aStatus && n->StatusPtr() != aStatus)
			continue;
		__KTRACE_OPT(KTHREAD,Kern::Printf("Found %08x", n));
		p = p->iNext;
		n->iThreadLink.Deque();
		n->iObjLink.Deque();

		// no-one else can see this notifier now
		aQ.Add(&n->iThreadLink);
		}
	m.Unlock();
	}



void ExecHandler::CancelMiscNotifier(TRequestStatus* aStatus)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("ExecHandler::CancelMiscNotifier %08x",aStatus));
	DThread& t = *TheCurrentThread;
	SDblQue garbage;
	NKern::ThreadEnterCS();
	t.ExtractMiscNotifiers(garbage, aStatus);
	while (!garbage.IsEmpty())
		{
		TMiscNotifier* n = _LOFF(garbage.First()->Deque(), TMiscNotifier, iThreadLink);
		__KTRACE_OPT(KOBJECT,Kern::Printf("Completing %08x thread %O -3->%08x", n, n->iThread, n->iStatus));
		TRequestStatus* s = n->StatusPtr();
		n->Close();	// delete before complete in case someone's testing for kernel heap leaks
		Kern::RequestComplete(s, KErrCancel);
		t.Close(NULL);
		}
	NKern::ThreadLeaveCS();
	}


void ExecHandler::NotifyObjectDestruction(TInt aHandle, TRequestStatus* aStatus)
	{
	TMiscNotifierMgr& m = K::TheMiscNotifierMgr;
	DObject* pO=NULL;
	TInt r=K::OpenObjectFromHandle(aHandle,pO);
	if (r!=KErrNone)
		K::PanicKernExec(EBadHandle);
	__KTRACE_OPT(KEXEC,Kern::Printf("ExecHandler::NotifyObjectDestruction %O %08x",pO,aStatus));
	TMiscNotifier* n = 0;
	SMiscNotifierQ* q = 0;
	r = m.NewMiscNotifier(aStatus, TRUE, pO, n, q);
	pO->Close(NULL);
	if (r<0)
		Kern::RequestComplete(aStatus, r);
	NKern::ThreadLeaveCS();
	}

void DThread::KillMiscNotifiers()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("%T KillMiscNotifiers", this));
	SDblQue garbage;
	ExtractMiscNotifiers(garbage, 0);
	while (!garbage.IsEmpty())
		{
		TMiscNotifier* n = _LOFF(garbage.First()->Deque(), TMiscNotifier, iThreadLink);
		// don't bother completing since thread has terminated anyway
		__KTRACE_OPT(KOBJECT,Kern::Printf("Killing %08x", n));
		DThread* t = n->iThread;
		n->Close();
		t->Close(NULL);
		}
	}

void DThread::Rendezvous(TInt aReason)
//
// Enter and return with system unlocked and calling thread in critical section.
//
	{
	TLogon::CompleteAll(iTargetLogons, TLogon::ETargetRendezvous, aReason);
	}

TInt DThread::Logon(TRequestStatus* aStatus, TBool aRendezvous)
	{
	TInt r = KErrNoMemory;
	DThread* pC = TheCurrentThread;
	__KTRACE_OPT(KTHREAD, Kern::Printf("Thread %O Logon to thread %O, status at %08x rdv=%x",
				 pC, this, aStatus, aRendezvous));

	TLogon* pL = new TLogon;
	if (pL)
		{
		TUint32 type = TLogon::ETargetThread;
		if (aRendezvous)
			type |= TLogon::ERendezvous;
		r = pL->Attach(iTargetLogons, pC, this, aStatus, type);
		if (r != KErrNone)
			pL->Close();
		}

	__KTRACE_OPT(KTHREAD, Kern::Printf("DThread::Logon ret %d", r));
	return r;
	}

#ifdef __SMP__
void DThread::SMPSafeCallback(TAny* aThisPtr, TUserModeCallbackReason aReasonCode)
	{
	if (aReasonCode == EUserModeCallbackRun)
		{
		DThread* t = _LOFF(aThisPtr,DThread,iSMPSafeCallback);
		TUint32 config = TheSuperPage().KernelConfigFlags();
		if (config & EKernelConfigSMPUnsafeCPU0)
			{
			if (t->iOwningProcess->iSMPUnsafeCount)
				NKern::ThreadSetCpuAffinity(&t->iNThread, 0);
			else
				NKern::ThreadSetCpuAffinity(&t->iNThread, KCpuAffinityAny);
			}
		else if (config & EKernelConfigSMPUnsafeCompat)
			{
			if (t->iOwningProcess->iSMPUnsafeCount)
				NKern::JoinGroup(t->iOwningProcess->iSMPUnsafeGroup);
			else
				NKern::LeaveGroup();
			}
		}
	}
#endif

/********************************************
 * TLogon
 ********************************************/

//
// Pseudo-constructor for TLogon class. Fills in all the private members, and
// attaches the TLogon object to both the owner's and target's linked lists.
// Enter and return with no fast mutexes held and current thread in CS
//
TInt TLogon::Attach(SDblQue& aList, DThread* aOwner, DObject* aTarget,
					TRequestStatus* aStatus, TUint32 aType)
	{
	TInt r = KErrNone;
	Lock();
	if (aType & ETargetProcess)
		{
		DProcess* pP = (DProcess*)aTarget;
		if (pP->iAttributes & DProcess::EBeingLoaded)
			r = KErrAccessDenied;
		else if (pP->iExitType != EExitPending)
			r = KErrDied;
		}
	else
		{
		DThread* pT = (DThread*)aTarget;
		if (pT->iMState == DThread::ECreated)
			r = KErrAccessDenied;
		else if (pT->iExiting)
			r = KErrDied;
		}
	if (r == KErrNone)
		{
		iType = aType;
		r = SetStatus(aStatus);
		__NK_ASSERT_DEBUG(r == KErrNone);  // can't fail
		iOwningThread = aOwner;
		iTarget = aTarget;
		aOwner->Open();
		aTarget->Open();

		// Rendezvous entries are always kept ahead of non-rendezvous ones
		if (aType & ERendezvous)
			aList.AddHead(&iTargetLink);
		else
			aList.Add(&iTargetLink);
		aOwner->iOwnedLogons.Add(&iOwningThreadLink);
		}
	Unlock();
	return r;
	}

//
// Cancel a specific logon on the owning thread list aList
// The logon to be cancelled is identified by aTarget, aStatus, and aType
// Enter and return with no fast mutexes held and current thread in CS
//
TInt TLogon::Cancel(SDblQue& aList, DObject* aTarget, TRequestStatus* aStatus, TUint32 aType)
	{
	Lock();
	SDblQueLink* anchor = &aList.iA;
	SDblQueLink* pLink = aList.First();

	for ( ; pLink != anchor; pLink = pLink->iNext)
		{
		TLogon* pL = _LOFF(pLink, TLogon, iOwningThreadLink);
		if (pL->iType == aType && pL->iTarget == aTarget && pL->StatusPtr() == aStatus)
			{
			// found it
			pL->iOwningThreadLink.Deque();
			pL->iTargetLink.Deque();
			Unlock();

			// To avoid a race condition after dropping the lock, we must complete this
			// request BEFORE closing the referenced target object and owning thread ...
			Kern::QueueRequestComplete(pL->iOwningThread, pL, KErrNone);
			pL->iTarget->Close(NULL);
			pL->iOwningThread->Close(NULL);
			pL->Close();
			return KErrNone;
			}
		}

	// not found
	Unlock();
	return KErrGeneral;
	}

//
// Complete either all logons or only the rendezvous entries on the list aList with reason aReason
// Enter and return with no fast mutexes held and current thread in CS
//
void TLogon::CompleteAll(SDblQue& aList, TComplete aAction, TInt aReason)
	{
	TInt offset = (aAction == EOwned) ? _FOFF(TLogon, iOwningThreadLink) : _FOFF(TLogon, iTargetLink);

	FOREVER
		{
		Lock();
		if (aList.IsEmpty())
			break;

		SDblQueLink* pLink = aList.First();
		TLogon* pL = (TLogon*)((TUint8*)pLink-offset);

		// Rendezvous entries are always ahead of non-rendezvous ones, so stop
		// if we're only interested in rendezvous and we find a non-rendezvous
		if (aAction == ETargetRendezvous && !pL->IsRendezvous())
			break;

		pL->iOwningThreadLink.Deque();
		pL->iTargetLink.Deque();
		Unlock();
		__KTRACE_OPT(KTHREAD, Kern::Printf("Complete Logon to %O type %d",
					 pL->iOwningThread, pL->iType));

		// To avoid a race condition after dropping the lock, we must complete this
		// request BEFORE closing the referenced target object and owning thread ...
		Kern::QueueRequestComplete(pL->iOwningThread, pL, aReason);
		pL->iTarget->Close(NULL);
		pL->iOwningThread->Close(NULL);
		pL->Close();
		}

	Unlock();
	}
