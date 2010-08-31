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
// e32\kernel\skernel.cpp
// 
//

#include <kernel/kern_priv.h>
#include "execs.h"

#define iMState		iWaitLink.iSpare1		// Allow a sensible name to be used for iMState

#ifdef BTRACE_SYMBIAN_KERNEL_SYNC
#define	BTRACE_KS(sub,obj)					{BTraceContext4(BTrace::ESymbianKernelSync, (sub), (obj));}
#define	COND_BTRACE_KS(cond,sub,obj)		if (cond) {BTraceContext4(BTrace::ESymbianKernelSync, (sub), (obj));}
#define	BTRACE_KS2(sub,obj1,obj2)			{BTraceContext8(BTrace::ESymbianKernelSync, (sub), (obj1), (obj2));}
#define	COND_BTRACE_KS2(cond,sub,obj1,obj2)	if (cond) {BTraceContext4(BTrace::ESymbianKernelSync, (sub), (obj1), (obj2));}
#define	BTRACE_KSC(sub)						{TKName n; Name(n); BTraceContextN(BTrace::ESymbianKernelSync, (sub), this, iOwner, n.Ptr(), n.Size());}
#define	COND_BTRACE_KSC(cond,sub)			if (cond) {TKName n; Name(n); BTraceContextN(BTrace::ESymbianKernelSync, (sub), this, iOwner, n.Ptr(), n.Size());}
#else
#define	BTRACE_KS(sub,obj)
#define	COND_BTRACE_KS(cond,sub,obj)
#define	BTRACE_KS2(sub,obj1,obj2)
#define	COND_BTRACE_KS2(cond,sub,obj1,obj2)
#define	BTRACE_KSC(sub)
#define	COND_BTRACE_KSC(cond,sub)
#endif


/********************************************
 * Semaphore
 ********************************************/

// Enter and return with system unlocked.
DSemaphore::~DSemaphore()
	{
	NKern::LockSystem();
	Reset();
	BTRACE_KS(BTrace::ESemaphoreDestroy, this);
	NKern::UnlockSystem();
	}

// Enter and return with system unlocked.
TInt DSemaphore::Create(DObject* aOwner, const TDesC* aName, TInt aInitialCount, TBool aVisible)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("DSemaphore::Create owner %O, name %S, init count=%d, visible=%d",aOwner,aName,aInitialCount,aVisible));
	if (aInitialCount<0)
		return KErrArgument;
	SetOwner(aOwner);
	TInt r=KErrNone;
	if (aName && aName->Length())
		{
		r=SetName(aName);
		if (r!=KErrNone)
			return r;
		}
	iCount=aInitialCount;
	r = iWaitQ.Construct();
	if (r==KErrNone && aVisible)
		r=K::AddObject(this,ESemaphore);
	COND_BTRACE_KSC(r==KErrNone, BTrace::ESemaphoreCreate);
	return r;
	}

// Wait for semaphore with timeout
// Enter with system locked, return with system unlocked.
// If aNTicks==0, wait forever
// If aNTicks==-1, poll (don't block)
// If aNTicks>0, timeout is aNTicks nanokernel ticks
TInt DSemaphore::Wait(TInt aNTicks)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Semaphore %O Wait %d Timeout %d",this,iCount,aNTicks));
	__ASSERT_DEBUG(TheCurrentThread->iMState==DThread::EReady,K::Fault(K::ESemWaitBadState));
	__ASSERT_DEBUG(!TheCurrentThread->iWaitObj,K::Fault(K::ESemWaitBadWaitObj));
	TInt r=KErrNone;
	if (iResetting)
		r=KErrGeneral;
	else if (--iCount<0)
		{
		if (aNTicks >= 0)
			{
			DThread* pC=TheCurrentThread;
			pC->iMState=DThread::EWaitSemaphore;
			pC->iWaitObj=this;
			iWaitQ.Add(pC);
			BTRACE_KS(BTrace::ESemaphoreBlock, this);
			r=NKern::Block(aNTicks,NKern::ERelease,SYSTEM_LOCK);
			__ASSERT_DEBUG(pC->iMState==DThread::EReady,K::Fault(K::ESemWaitBadState));
			COND_BTRACE_KS(r==KErrNone, BTrace::ESemaphoreAcquire, this);
			}
		else
			{
			++iCount;
			NKern::UnlockSystem();
			r = KErrTimedOut;	// couldn't acquire semaphore immediately, so fail
			}
		return r;
		}
#ifdef BTRACE_SYMBIAN_KERNEL_SYNC
	else
		BTRACE_KS(BTrace::ESemaphoreAcquire, this);
#endif
	NKern::UnlockSystem();
	return r;
	}

// Enter with system locked, return with system unlocked.
void DSemaphore::Signal()
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Semaphore %O Signal %d",this,iCount));
	__ASSERT_DEBUG(TheCurrentThread->iMState==DThread::EReady,K::Fault(K::ESemSignalBadState));
	COND_BTRACE_KS(!iResetting, BTrace::ESemaphoreRelease, this);
	if (!iResetting && ++iCount<=0)
		{
		DThread* pT=iWaitQ.First();
		iWaitQ.Remove(pT);
		pT->iMState=DThread::EReady;
		pT->iWaitObj=NULL;
#if defined(_DEBUG) && !defined(__SMP__)
		// For crazy scheduler: if next thread is same priority as current, let it run
		// Check before releasing it in case it preempts us and exits
		TBool yield = EFalse;
		if (TheSuperPage().KernelConfigFlags() & EKernelConfigCrazyScheduling
				&& NCurrentThread()->iPriority == pT->iNThread.iPriority)
			yield = ETrue;
#endif
		NKern::ThreadRelease(&pT->iNThread,0,SYSTEM_LOCK);
#if defined(_DEBUG) && !defined(__SMP__)
		// Actually do the yield
		if (yield)
			NKern::YieldTimeslice();
#endif
		return;
		}
	NKern::UnlockSystem();
	}

// Enter and return with system locked.
void DSemaphore::SignalN(TInt aCount)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Semaphore %O SignalN(%d) %d",this,aCount,iCount));
	__ASSERT_DEBUG(TheCurrentThread->iMState==DThread::EReady,K::Fault(K::ESemSignalBadState));
	if (iResetting)
		return;
	if (iCount<0 && aCount>0)
		{
		while(aCount--)
			{
			BTRACE_KS(BTrace::ESemaphoreRelease, this);
			if (++iCount<=0)
				{
				DThread* pT=iWaitQ.First();
				iWaitQ.Remove(pT);
				pT->iMState=DThread::EReady;
				pT->iWaitObj=NULL;
#if defined(_DEBUG) && !defined(__SMP__)
				// For crazy scheduler: if next thread is same priority as current, let it run
				// Check before releasing it in case it preempts us and exits
				TBool yield = EFalse;
				if (TheSuperPage().KernelConfigFlags() & EKernelConfigCrazyScheduling
						&& NCurrentThread()->iPriority == pT->iNThread.iPriority)
					yield = ETrue;
#endif
				NKern::ThreadRelease(&pT->iNThread,0,SYSTEM_LOCK);
#if defined(_DEBUG) && !defined(__SMP__)
				// Actually do the yield
				if (yield)
					NKern::YieldTimeslice();
#endif
				NKern::LockSystem();
				if (iResetting)
					return;
				}
			else
				{
				iCount+=aCount;
				break;
				}
			}
		}
	else if (aCount>0)
		iCount+=aCount;
	}

// Enter and return with system locked.
void DSemaphore::Reset()
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Semaphore %O Reset %d",this,iCount));
	if (iResetting)
		return;
	iResetting = TRUE;

	// We release the waiting threads before the suspended threads.
	// Other code relies on this.
	while(iCount<0)
		{
		iCount++;
		DThread* pT=iWaitQ.First();
		iWaitQ.Remove(pT);
		pT->iMState=DThread::EReady;
		pT->iWaitObj=NULL;
		NKern::ThreadRelease(&pT->iNThread,KErrGeneral,SYSTEM_LOCK);
		NKern::LockSystem();
		}
	while (!iSuspendedQ.IsEmpty())
		{
		DThread* pT=_LOFF(iSuspendedQ.First()->Deque(),DThread,iWaitLink);
		pT->iMState=DThread::EReady;
		pT->iWaitObj=NULL;
		NKern::ThreadRelease(&pT->iNThread,KErrGeneral,SYSTEM_LOCK);
		NKern::LockSystem();
		}
	iResetting = FALSE;
	iCount=0;
	}

// Enter and return with system locked.
void DSemaphore::WaitCancel(DThread* aThread)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Semaphore %O WaitCancel(%O) %d",this,aThread,iCount));
	iWaitQ.Remove(aThread);
	++iCount;
	}

// Enter and return with system locked.
void DSemaphore::WaitCancelSuspended(DThread* aThread)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Semaphore %O WaitCanSusp(%O) %d",this,aThread,iCount));
	aThread->iWaitLink.Deque();
	}

// Enter and return with system locked.
void DSemaphore::SuspendWaitingThread(DThread* aThread)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Semaphore %O SuspWait(%O) %d",this,aThread,iCount));
	++iCount;
	iWaitQ.Remove(aThread);
	iSuspendedQ.Add(&aThread->iWaitLink);	// OK if resetting since suspended queue is processed after wait queue
	aThread->iMState=DThread::EWaitSemaphoreSuspended;
	}

// Enter and return with system locked.
void DSemaphore::ResumeWaitingThread(DThread* aThread)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Semaphore %O ResumeWait(%O) %d",this,aThread,iCount));
	aThread->iWaitLink.Deque();
	if (!iResetting && --iCount<0)
		{
		iWaitQ.Add(aThread);
		aThread->iMState=DThread::EWaitSemaphore;
		}
	else
		{
		aThread->iMState=DThread::EReady;
		aThread->iWaitObj=NULL;
		NKern::ThreadRelease(&aThread->iNThread,iResetting?KErrGeneral:KErrNone);
		}
	}

// Enter and return with system locked.
void DSemaphore::ChangeWaitingThreadPriority(DThread* aThread, TInt aNewPriority)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Semaphore %O ChangeWaitPri(%O,%d)",this,aThread,aNewPriority));
	iWaitQ.ChangePriority(aThread,aNewPriority);
	}

/********************************************
 * Mutex
 ********************************************/
inline TThreadMutexCleanup::TThreadMutexCleanup(DMutex* aMutex)
	: iMutex(aMutex)
	{}

// Enter and return with system locked.
void TThreadMutexCleanup::Cleanup()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("TThreadCleanup::Cleanup Free mutex %O",iMutex));
	if (iMutex->iResetting)
		{
		iMutex->iCleanup.Remove();
		iMutex->iCleanup.iThread=NULL;
#ifdef _DEBUG
		iMutex->iOrderLink.Deque();
#endif
		}
	else
		{
		iMutex->iHoldCount=1;
		iMutex->Signal();
		NKern::LockSystem();
		}
	}


DMutex::DMutex()
	: iCleanup(this)
	{}

// Enter and return with system unlocked.
DMutex::~DMutex()
	{
	NKern::LockSystem();
	Reset();
	BTRACE_KS(BTrace::EMutexDestroy, this);
	NKern::UnlockSystem();
	}

// Enter and return with system unlocked.
TInt DMutex::Create(DObject* aOwner, const TDesC* aName, TBool aVisible, TUint aOrder)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("DMutex::Create owner %O, name %S, visible=%d, order=%02x",aOwner,aName,aVisible,aOrder));
	iOrder = (TUint8)aOrder;
	SetOwner(aOwner);
	TInt r=KErrNone;
	if (aName && aName->Length())
		{
		r=SetName(aName);
		if (r!=KErrNone)
			return r;
		}
	r = iWaitQ.Construct();
	if (r==KErrNone && aVisible)
		r=K::AddObject(this,EMutex);
	COND_BTRACE_KSC(r==KErrNone, BTrace::EMutexCreate);
	return r;
	}
#ifdef _DEBUG
extern const SNThreadHandlers EpocThreadHandlers;
#endif

// Wait for mutex with timeout
// Enter with system locked, return with system unlocked.
// If aNTicks==0, wait forever
// If aNTicks==-1, poll (don't block)
// If aNTicks>0, timeout is aNTicks nanokernel ticks
TInt DMutex::Wait(TInt aNTicks)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Mutex %O Wait(%d) hold %O hldc=%d wtc=%d",this,aNTicks,iCleanup.iThread,iHoldCount,iWaitCount));
	__ASSERT_SYSTEM_LOCK;
	__ASSERT_DEBUG(NCurrentThread()->iHandlers==&EpocThreadHandlers, K::Fault(K::EMutexWaitNotDThread));
	DThread* pC=TheCurrentThread;
	__ASSERT_DEBUG(pC->iMState==DThread::EReady,K::Fault(K::EMutexWaitBadState));
	__ASSERT_DEBUG(!pC->iWaitObj,K::Fault(K::EMutexWaitBadWaitObj));
#ifdef _DEBUG
	SDblQue& ml = pC->iMutexList;
	DMutex* m = ml.IsEmpty() ? NULL : _LOFF(ml.First(), DMutex, iOrderLink);
	TUint last_mutex_order = m ? m->iOrder : KMutexOrdNone;
	if (iCleanup.iThread!=pC && iOrder<KMutexOrdUser && iOrder>=last_mutex_order)
		{
		__KTRACE_OPT(KPANIC,Kern::Printf("Mutex ordering violation: holding mutex %O (%08x) order %d, trying to acquire mutex %O (%08x) order %d",m,m,last_mutex_order,this,this,iOrder));
		K::Fault(K::EMutexOrderingViolation);
		}
#endif
	while(!iResetting && iCleanup.iThread)
		{
		if (iCleanup.iThread==pC)
			{
			++iHoldCount;
			BTRACE_KS(BTrace::EMutexAcquire, this);
			return KErrNone;
			}
		if (aNTicks<0)
			return KErrTimedOut;	// poll mode - can't get mutex immediately so fail
		K::PINestLevel=0;
		pC->iMState=DThread::EWaitMutex;
		pC->iWaitObj=this;
		++iWaitCount;
		iWaitQ.Add(pC);
		TInt p=pC->iWaitLink.iPriority;
		if (p>iCleanup.iPriority)
			iCleanup.ChangePriority(p);
		BTRACE_KS(BTrace::EMutexBlock, this);

		// If the thread is woken up normally as a result of the mutex being released
		// this function will KErrNone and this thread will have been placed in
		// EHoldMutexPending state. If the thread is killed while waiting this function
		// will not return (since the exit handler will run instead).
		// If the mutex is reset (or deleted) while the thread is waiting this function
		// will return KErrGeneral and the thread will have been placed into EReady
		// state. If however the mutex is reset (or deleted) while the thread is in
		// EHoldMutexPending state, having already been woken up normally as a result
		// of the mutex being released, this function will return KErrNone (since the
		// return value is set at the point where the thread is released from its wait
		// condition). However we can still detect this situation since the thread will
		// have been placed into the EReady state when the mutex was reset.
		TInt r=NKern::Block(aNTicks, NKern::ERelease|NKern::EClaim|NKern::EObstruct, SYSTEM_LOCK);
		if (r==KErrNone && pC->iMState==DThread::EReady)
			r = KErrGeneral;	// mutex has been reset
		if (r!=KErrNone)		// if we get an error here...
			{
			__ASSERT_DEBUG(pC->iMState==DThread::EReady,K::Fault(K::EMutexWaitBadState));
			return r;		// ...bail out now - this mutex may no longer exist
			}
		pC->iMState=DThread::EReady;
		pC->iWaitObj=NULL;
		pC->iWaitLink.Deque();		// remove thread from iPendingQ
		}
	if (iResetting)
		return KErrGeneral;
	BTRACE_KS(BTrace::EMutexAcquire, this);
	iHoldCount=1;
	iCleanup.iPriority=TUint8(HighestWaitingPriority());
	pC->AddCleanup(&iCleanup);
#ifdef _DEBUG
	ml.AddHead(&iOrderLink);
#endif
	return KErrNone;
	}

// Enter with system locked, return with system unlocked.
void DMutex::Signal()
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Mutex %O Signal hold=%O hldc=%d wtc=%d",this,iCleanup.iThread,iHoldCount,iWaitCount));
	__ASSERT_SYSTEM_LOCK;
	DThread* pC=TheCurrentThread;
	__ASSERT_DEBUG(iCleanup.iThread==pC,K::Fault(K::EMutexSignalWrongThread));
	__ASSERT_DEBUG(pC->iMState==DThread::EReady,K::Fault(K::EMutexSignalBadState));
	COND_BTRACE_KS(!iResetting, BTrace::EMutexRelease, this);
	if (!iResetting && --iHoldCount==0)
		{
		if (iWaitQ.NonEmpty())
			{
			// Wake up the next waiting thread.
			// We MUST do this before reliquishing our inherited priority.
			// We won't thrash on the system lock because inheritance ensures our priority is not
			// lower than the waiting thread's and the scheduler will not round-robin a thread which
			// holds a fast mutex (the system lock in this case).
			WakeUpNextThread();
			}
		else
			{
			__ASSERT_DEBUG(iCleanup.iPriority==0,Kern::Fault("MutSigBadClnPri",iCleanup.iPriority));
			}
		iCleanup.iThread=NULL;
#ifdef _DEBUG
		iOrderLink.Deque();
#endif
		pC->iCleanupQ.Remove(&iCleanup);	// remove cleanup item but don't set priority yet
		if (iCleanup.iPriority!=0)			// if cleanup item had nonzero priority may need to revert our priority
			{
			// Revert this thread's priority and release the system lock without thrashing.
			// Relies on the fact that our MState is READY here.
			pC->RevertPriority();
			return;
			}
		}
	NKern::UnlockSystem();
	}

// Enter and return with system locked.
void DMutex::Reset()
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Mutex %O Reset hold=%O hldc=%d wtc=%d",this,iCleanup.iThread,iHoldCount,iWaitCount));
	__ASSERT_SYSTEM_LOCK;
	if (iResetting)
		return;
	K::PINestLevel=0;
	iResetting = TRUE;

	// We release the pending threads first, then waiting threads, then suspended threads.
	// Other code relies on this.
	while (!iPendingQ.IsEmpty())
		{
		DThread* pT=_LOFF(iPendingQ.First()->Deque(),DThread,iWaitLink);
		pT->iMState=DThread::EReady;
		pT->iWaitObj=NULL;
		NKern::FlashSystem();
		}
	while (iWaitQ.NonEmpty())
		{
		DThread* pT=iWaitQ.First();
		iWaitQ.Remove(pT);
		pT->iMState=DThread::EReady;
		pT->iWaitObj=NULL;
		NKern::ThreadRelease(&pT->iNThread,KErrGeneral,SYSTEM_LOCK);
		NKern::LockSystem();
		}
	while (!iSuspendedQ.IsEmpty())
		{
		DThread* pT=_LOFF(iSuspendedQ.First()->Deque(),DThread,iWaitLink);
		pT->iMState=DThread::EReady;
		pT->iWaitObj=NULL;
		NKern::ThreadRelease(&pT->iNThread,KErrGeneral,SYSTEM_LOCK);
		NKern::LockSystem();
		}
	if (iCleanup.iThread)
		{
		iCleanup.Remove();
		iCleanup.iThread=NULL;
#ifdef _DEBUG
		iOrderLink.Deque();
#endif
		}
	iCleanup.iPriority=0;
	iHoldCount=0;
	iWaitCount=0;
	iResetting = FALSE;
	}

// Enter and return with system locked.
void DMutex::WaitCancel(DThread* aThread)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Mutex %O WaitCancel(%O)",this,aThread));
	iWaitQ.Remove(aThread);
	--iWaitCount;
	K::PINestLevel=0;
	TInt p=HighestWaitingPriority();
	iCleanup.ChangePriority(p);
	}

// Enter and return with system locked.
void DMutex::WaitCancelSuspended(DThread* aThread)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Mutex %O WaitCanSusp(%O)",this,aThread));
	aThread->iWaitLink.Deque();
	--iWaitCount;
	}

// Enter and return with system locked.
void DMutex::SuspendWaitingThread(DThread* aThread)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Mutex %O SuspWait(%O)",this,aThread));
	iWaitQ.Remove(aThread);
	iSuspendedQ.Add(&aThread->iWaitLink);	// OK if resetting since suspended queue is processed after wait queue
	aThread->iMState=DThread::EWaitMutexSuspended;
	K::PINestLevel=0;
	TInt p=HighestWaitingPriority();
	iCleanup.ChangePriority(p);
	}

// Enter and return with system locked.
void DMutex::ResumeWaitingThread(DThread* aThread)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Mutex %O ResumeWait(%O)",this,aThread));
	aThread->iWaitLink.Deque();
	if (!iResetting)
		{
		if (iCleanup.iThread)
			{
			// mutex is held, so put this one back on wait queue
			aThread->iMState=DThread::EWaitMutex;
			iWaitQ.Add(aThread);
			K::PINestLevel=0;
			TInt p=aThread->iWaitLink.iPriority;
			if (p>iCleanup.iPriority)
				iCleanup.ChangePriority(p);
			return;
			}
		aThread->iMState=DThread::EHoldMutexPending;
		iPendingQ.Add(&aThread->iWaitLink);
		--iWaitCount;
		NKern::ThreadRelease(&aThread->iNThread,0);
		}
	else
		{
		// don't want to put it on the wait queue
		aThread->iMState=DThread::EReady;
		aThread->iWaitObj=NULL;
		NKern::ThreadRelease(&aThread->iNThread,KErrGeneral);
		}
	}

// Enter and return with system locked.
void DMutex::ChangeWaitingThreadPriority(DThread* aThread, TInt aNewPriority)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Mutex %O ChangeWaitPri(%O,%d)",this,aThread,aNewPriority));
	if (!iCleanup.iThread && aNewPriority>aThread->iWaitLink.iPriority && !iResetting)
		{
		// if the mutex is currently free and the thread's priority is being increased, wake up the thread
		__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Mutex %O wake up %O wtc=%d",this,aThread,iWaitCount));
		iWaitQ.Remove(aThread);
		aThread->iWaitLink.iPriority=(TUint8)aNewPriority;
		iPendingQ.Add(&aThread->iWaitLink);
		aThread->iMState=DThread::EHoldMutexPending;
		--iWaitCount;
		NKern::ThreadRelease(&aThread->iNThread,0);	// unfortunately this may well thrash but this should be a rare case
		}
	else
		{
		iWaitQ.ChangePriority(aThread,aNewPriority);
		iCleanup.ChangePriority(iWaitQ.HighestPriority());
		}
	}

// Enter and return with system locked.
void DMutex::ChangePendingThreadPriority(DThread* aThread, TInt aNewPriority)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Mutex %O ChangePendPri(%O,%d)",this,aThread,aNewPriority));
	if (!iCleanup.iThread && aNewPriority<aThread->iWaitLink.iPriority && !iResetting)
		{
		if (aNewPriority<HighestWaitingPriority())
			{
			// wake up the next thread
			WakeUpNextThread();
			}
		}
	aThread->iWaitLink.iPriority=(TUint8)aNewPriority;
	}

// Enter and return with system locked.
void DMutex::WakeUpNextThread()
	{
	// wake up the next thread
	DThread* pT=iWaitQ.First();
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Mutex %O wake up %O wtc=%d",this,pT,iWaitCount));
	iWaitQ.Remove(pT);
	iPendingQ.Add(&pT->iWaitLink);
	pT->iMState=DThread::EHoldMutexPending;
	--iWaitCount;
	NKern::ThreadRelease(&pT->iNThread,0);
	// If next thread is same priority as current, let it have a go with the mutex
	// Safe to inspect pT here as it can't have run yet, we've still got the system lock
	if (NCurrentThread()->iPriority == pT->iNThread.iPriority)
		NKern::YieldTimeslice();
	}

// Called when a thread which was about to claim the mutex is suspended
// Enter and return with system locked.
#ifdef KSEMAPHORE
void DMutex::SuspendPendingThread(DThread* aThread)
#else
void DMutex::SuspendPendingThread(DThread*)
#endif
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Mutex %O SuspendP(%O)",this,aThread));
	if (!iResetting && !iCleanup.iThread && iWaitQ.NonEmpty())
		WakeUpNextThread();
	}

// Called when a pending thread exits
// Enter and return with system locked.
void DMutex::RemovePendingThread(DThread* aThread)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Mutex %O RemoveP(%O)",this,aThread));
	aThread->iWaitLink.Deque();
	if (!iResetting && !iCleanup.iThread && iWaitQ.NonEmpty())
		WakeUpNextThread();
	}

TInt DMutex::HighestWaitingPriority()
	{
	if (iWaitQ.NonEmpty())
		return iWaitQ.HighestPriority();
	return 0;
	}

/********************************************
 * Condition Variable
 ********************************************/
DCondVar::DCondVar()
	{
	}

DCondVar::~DCondVar()
	{
	NKern::LockSystem();
	Reset();
	BTRACE_KS(BTrace::ECondVarDestroy, this);
	NKern::UnlockSystem();
	}

// Enter and return with system unlocked.
TInt DCondVar::Create(DObject* aOwner, const TDesC* aName, TBool aVisible)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("DCondVar::Create owner %O, name %S, visible=%d",aOwner,aName,aVisible));
	SetOwner(aOwner);
	TInt r=KErrNone;
	if (aName && aName->Length())
		{
		r=SetName(aName);
		if (r!=KErrNone)
			return r;
		}
	r = iWaitQ.Construct();
	if (r==KErrNone && aVisible)
		r=K::AddObject(this,ECondVar);
	COND_BTRACE_KSC(r==KErrNone, BTrace::ECondVarCreate);
	return r;
	}

void DCondVar::WaitCancel(DThread* aThread)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("CondVar %O WaitCancel(%O)", this, aThread));
	iWaitQ.Remove(aThread);
	if (--iWaitCount == 0)
		iMutex = NULL;
	}

void DCondVar::WaitCancelSuspended(DThread* aThread)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("CondVar %O WaitCanSusp(%O)", this, aThread));
	aThread->iWaitLink.Deque();
	if (--iWaitCount == 0)
		iMutex = NULL;
	}

void DCondVar::SuspendWaitingThread(DThread* aThread)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("CondVar %O SuspendWait(%O)", this, aThread));
	iWaitQ.Remove(aThread);
	iSuspendedQ.Add(&aThread->iWaitLink);	// OK if resetting since suspended queue is processed after wait queue
	aThread->iMState = DThread::EWaitCondVarSuspended;
	}

void DCondVar::ResumeWaitingThread(DThread* aThread)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("CondVar %O ResumeWait(%O)", this, aThread));
	aThread->iWaitLink.Deque();
	aThread->iMState = DThread::EWaitCondVar;
	UnBlockThread(aThread, EFalse);
	}

void DCondVar::ChangeWaitingThreadPriority(DThread* aThread, TInt aNewPriority)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("CondVar %O ChangeWaitPri(%O,%d)", this, aThread, aNewPriority));
	if (aNewPriority>aThread->iWaitLink.iPriority && !iResetting)
		{
		__KTRACE_OPT(KSEMAPHORE,Kern::Printf("CV %O wake up %O", this, aThread));
		iWaitQ.Remove(aThread);
		aThread->iWaitLink.iPriority = (TUint8)aNewPriority;
		UnBlockThread(aThread, EFalse);
		}
	else
		{
		iWaitQ.ChangePriority(aThread, aNewPriority);
		}
	}

TInt DCondVar::Wait(DMutex* aMutex, TInt aTimeout)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("CondVar %O Wait (M=%O, tmout=%d)", this, aMutex, aTimeout));
	__ASSERT_SYSTEM_LOCK;
	DMutex& m = *aMutex;
	DThread* pC=TheCurrentThread;
	__ASSERT_DEBUG(pC->iMState==DThread::EReady, K::Fault(K::ECondVarWaitBadState1));
	if (iResetting)
		return KErrGeneral;
	if (aMutex->iCleanup.iThread != pC)
		K::PanicCurrentThread(ECondVarWaitMutexNotLocked);
	if (iMutex && iMutex!=aMutex)
		return KErrInUse;
	if (!iMutex)
		iMutex = aMutex;

	// set the current thread M-State to wait-for-condition-variable
	pC->iMState = DThread::EWaitCondVar;
	pC->iWaitObj = this;
	iWaitQ.Add(pC);
	++iWaitCount;

	// unlock the associated mutex
	TBool unlock = ETrue;
	m.iHoldCount = 0;
	if (m.iWaitQ.NonEmpty())
		{
		// Wake up the next waiting thread.
		// We MUST do this before reliquishing our inherited priority.
		// We won't thrash on the system lock because inheritance ensures our priority is not
		// lower than the waiting thread's and the scheduler will not round-robin a thread which
		// holds a fast mutex (the system lock in this case).
		m.WakeUpNextThread();
		}
	else
		{
		__ASSERT_DEBUG(m.iCleanup.iPriority==0,Kern::Fault("MutSigBadClnPri",m.iCleanup.iPriority));
		}
	m.iCleanup.iThread=NULL;
#ifdef _DEBUG
	m.iOrderLink.Deque();
#endif
	pC->iCleanupQ.Remove(&m.iCleanup);	// remove cleanup item but don't set priority yet
	if (m.iCleanup.iPriority!=0)		// if cleanup item had nonzero priority may need to revert our priority
		{
		// Revert this thread's priority and release the system lock without thrashing.
		TInt p = pC->iDefaultPriority;
		TInt c = pC->iCleanupQ.HighestPriority();
		__KTRACE_OPT(KTHREAD,Kern::Printf("Thread %O CVRevertPriority def %d cleanup %d", pC, pC->iDefaultPriority, c));
		if (c>p)
			p=c;
		if (p != pC->iNThread.i_NThread_BasePri)
			{
			iWaitQ.ChangePriority(pC, p);
			NKern::ThreadSetPriority(&pC->iNThread, p, SYSTEM_LOCK);	// anti-thrash
			unlock = EFalse;
			}
		}
	if (unlock)
		NKern::UnlockSystem();

	// reacquire the system lock and check if we need to block
	NKern::LockSystem();
	switch (pC->iMState)
		{
		case DThread::EReady:				// condition variable deleted
			return KErrGeneral;
		case DThread::EHoldMutexPending:	// condition variable signalled, mutex free
		case DThread::EWaitMutex:			// condition variable signalled, now waiting for mutex
		case DThread::EWaitCondVar:			// still waiting
			break;
		case DThread::ECreated:
		case DThread::EDead:
		case DThread::EWaitSemaphore:
		case DThread::EWaitSemaphoreSuspended:
		case DThread::EWaitMutexSuspended:
		case DThread::EWaitCondVarSuspended:
		default:
			K::Fault(K::ECondVarWaitBadState2);
		}

	// block if necessary then reacquire the mutex
	TInt r = KErrNone;
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Mutex %O Wait hold %O hldc=%d wtc=%d", &m, m.iCleanup.iThread, m.iHoldCount, m.iWaitCount));
#ifdef _DEBUG
	SDblQue& ml = pC->iMutexList;
	DMutex* mm = ml.IsEmpty() ? NULL : _LOFF(ml.First(), DMutex, iOrderLink);
	TUint last_mutex_order = mm ? mm->iOrder : KMutexOrdNone;
	if (m.iOrder<KMutexOrdUser && m.iOrder>=last_mutex_order)
		K::Fault(K::EMutexOrderingViolation);
#endif
	while(m.iCleanup.iThread || pC->iMState==DThread::EWaitMutex || pC->iMState==DThread::EWaitCondVar)	// mutex can't be resetting since we have a handle on it
		{
		if (pC->iMState == DThread::EHoldMutexPending)
			pC->iWaitLink.Deque();
		if (pC->iMState!=DThread::EWaitCondVar && pC->iMState!=DThread::EWaitMutex)
			{
			K::PINestLevel = 0;
			pC->iMState = DThread::EWaitMutex;
			pC->iWaitObj = &m;
			++m.iWaitCount;
			m.iWaitQ.Add(pC);
			TInt p = pC->iWaitLink.iPriority;
			if (p>m.iCleanup.iPriority)
				m.iCleanup.ChangePriority(p);
			}
		TInt tmout = pC->iMState==DThread::EWaitCondVar ? aTimeout : 0;
		TUint mode = NKern::ERelease|NKern::EClaim;
		if (pC->iMState == DThread::EWaitMutex)
			mode |= NKern::EObstruct;
		BTRACE_KS2(BTrace::ECondVarBlock, this, &m);

		// The following possibilities exist here:
		// 1.	Normal operation: condition variable released and mutex is unlocked
		//		s=KErrNone, thread state EHoldMutexPending
		// 2.	Timeout while waiting for condition variable
		//		s=KErrTimedOut, thread state EReady
		// 3.	Condition variable reset while thread waiting for it
		//		s=KErrGeneral, thread state EReady
		// 4.	Mutex reset while thread waiting for it (after condition variable signalled)
		//		s=KErrGeneral, thread state EReady
		// 5.	Mutex reset while thread is in EHoldMutexPending state (after condition
		//		variable signalled and mutex unlocked)
		//		s=KErrNone, thread state EReady
		// 6.	Thread killed while waiting for mutex or condition variable
		//		Function doesn't return since exit handler runs instead.
		TInt s = NKern::Block(tmout, mode, SYSTEM_LOCK);
		if (s==KErrNone && pC->iMState==DThread::EReady)
			s = KErrGeneral;
		if (s!=KErrNone && s!=KErrTimedOut)	// if we get an error here...
			{
			__ASSERT_DEBUG(pC->iMState==DThread::EReady,K::Fault(K::EMutexWaitBadState));
			return s;		// ...bail out now - this condition variable may no longer exist
			}
		if (s==KErrTimedOut)
			r = s;
		BTRACE_KS2(BTrace::ECondVarWakeUp, this, &m);
		}
	__ASSERT_DEBUG(pC->iMState==DThread::EReady || pC->iMState==DThread::EHoldMutexPending,
																K::Fault(K::ECondVarWaitBadState3));
	if (pC->iMState == DThread::EHoldMutexPending)
		pC->iWaitLink.Deque();		// remove thread from iPendingQ
	pC->iMState = DThread::EReady;
	pC->iWaitObj = NULL;
	m.iHoldCount = 1;
	m.iCleanup.iPriority = TUint8(m.HighestWaitingPriority());
	pC->AddCleanup(&m.iCleanup);
#ifdef _DEBUG
	ml.AddHead(&m.iOrderLink);
#endif
	return r;
	}

void DCondVar::UnBlockThread(DThread* t, TBool aUnlock)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("CondVar %O UnBlockThread %O M:%d U:%d", this, t, t->iMState, aUnlock));
	if (iResetting)
		{
		t->iWaitObj = NULL;
		t->iMState = DThread::EReady;
#if defined(_DEBUG) && !defined(__SMP__)
		// For crazy scheduler: if next thread is same priority as current, let it run
		// Check before releasing it in case it preempts us and exits
		TBool yield = EFalse;
		if (TheSuperPage().KernelConfigFlags() & EKernelConfigCrazyScheduling
				&& NCurrentThread()->iPriority == t->iNThread.iPriority)
			yield = ETrue;
#endif
		if (aUnlock)
			NKern::ThreadRelease(&t->iNThread, KErrGeneral, SYSTEM_LOCK);
		else
			NKern::ThreadRelease(&t->iNThread, KErrGeneral);
#if defined(_DEBUG) && !defined(__SMP__)
		// Actually do the yield
		if (yield)
			NKern::YieldTimeslice();
#endif
		return;
		}
	t->iWaitObj = iMutex;
	if (t->iMState == DThread::EWaitCondVar)
		{
		if (iMutex->iCleanup.iThread)
			{
			__KTRACE_OPT(KSEMAPHORE,Kern::Printf("WaitThread %O -> EWaitMutex", t));
			t->iMState = DThread::EWaitMutex;
			iMutex->iWaitQ.Add(t);
			++iMutex->iWaitCount;
			K::PINestLevel = 0;
			TInt p = t->iWaitLink.iPriority;
			if (p > iMutex->iCleanup.iPriority)
				iMutex->iCleanup.ChangePriority(p);
			}
		else
			{
			__KTRACE_OPT(KSEMAPHORE,Kern::Printf("WaitThread %O -> EHoldMutexPending", t));
			t->iMState = DThread::EHoldMutexPending;
			iMutex->iPendingQ.Add(&t->iWaitLink);
			if (aUnlock)
				NKern::ThreadRelease(&t->iNThread, 0, SYSTEM_LOCK);
			else
				NKern::ThreadRelease(&t->iNThread, 0);
			aUnlock = EFalse;
			}
		}
	else if (t->iMState == DThread::EWaitCondVarSuspended)
		{
		__KTRACE_OPT(KSEMAPHORE,Kern::Printf("WaitThread %O -> EWaitMutexSusp", t));
		t->iMState = DThread::EWaitMutexSuspended;
		iMutex->iSuspendedQ.Add(&t->iWaitLink);
		++iMutex->iWaitCount;
		}
	else
		K::Fault(K::ECondVarUnBlockBadState);
	if (--iWaitCount == 0)
		iMutex = NULL;
	if (aUnlock)
		NKern::UnlockSystem();
	}

void DCondVar::Signal()
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("CondVar %O Signal", this));
	__ASSERT_SYSTEM_LOCK;
	BTRACE_KS2(BTrace::ECondVarSignal, this, iMutex);
	DThread* t = NULL;
	if (iWaitQ.NonEmpty())
		{
		t = iWaitQ.First();
		iWaitQ.Remove(t);
		}
	else if (!iSuspendedQ.IsEmpty())
		{
		t = _LOFF(iSuspendedQ.First()->Deque(), DThread, iWaitLink);
		}
	if (t)
		UnBlockThread(t, ETrue);
	else
		NKern::UnlockSystem();
	}

// On entry the specified mutex is held by the current thread
// Enter and return with system locked
void DCondVar::Broadcast(DMutex* m)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("CondVar %O Broadcast", this));
	__ASSERT_SYSTEM_LOCK;
	BTRACE_KS2(BTrace::ECondVarBroadcast, this, m);
	while (iMutex == m)
		{
		Signal();
		NKern::LockSystem();
		}
	}

void DCondVar::Reset()
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("CondVar %O Reset", this));
	__ASSERT_SYSTEM_LOCK;
	if (iResetting)
		return;
	iResetting = TRUE;

	// We release the waiting threads first, then suspended threads.
	// Other code relies on this.
	while (iWaitQ.NonEmpty())
		{
		DThread* pT=iWaitQ.First();
		iWaitQ.Remove(pT);
		pT->iMState=DThread::EReady;
		pT->iWaitObj=NULL;
		NKern::ThreadRelease(&pT->iNThread, KErrGeneral, SYSTEM_LOCK);
		NKern::LockSystem();
		}
	while (!iSuspendedQ.IsEmpty())
		{
		DThread* pT=_LOFF(iSuspendedQ.First()->Deque(),DThread,iWaitLink);
		pT->iMState=DThread::EReady;
		pT->iWaitObj=NULL;
		NKern::ThreadRelease(&pT->iNThread, KErrGeneral, SYSTEM_LOCK);
		NKern::LockSystem();
		}
	iMutex = NULL;
	iResetting = FALSE;
	}

TInt ExecHandler::CondVarWait(DCondVar* aCondVar, TInt aMutexHandle, TInt aTimeout)
	{
	if (aTimeout)
		{
		if (aTimeout<0)
			{
			NKern::UnlockSystem();
			return KErrArgument;
			}

		// Convert microseconds to NTimer ticks, rounding up
		TInt ntp = NKern::TickPeriod();
		aTimeout += ntp-1;
		aTimeout /= ntp;
		}
	DThread* t = TheCurrentThread;
	DMutex* m = (DMutex*)K::ObjectFromHandle(aMutexHandle, EMutex);
	m->CheckedOpen();
	t->iTempObj = m;
	TInt r = aCondVar->Wait(m, aTimeout);
	t->iTempObj = NULL;
	TInt c = m->Dec();
	if (c==1)
		{
		NKern::ThreadEnterCS();
		NKern::UnlockSystem();
		K::ObjDelete(m);
		NKern::ThreadLeaveCS();
		}
	else
		NKern::UnlockSystem();
	return r;
	}

void ExecHandler::CondVarSignal(DCondVar* aCondVar)
	{
	aCondVar->Signal();
	}

void ExecHandler::CondVarBroadcast(DCondVar* aCondVar)
	{
	TBool wm = EFalse;
	DMutex* m = aCondVar->iMutex;
	if (m)	// if no mutex, no-one is waiting so no-op
		{
		aCondVar->CheckedOpen();
		m->CheckedOpen();
		NKern::ThreadEnterCS();
		if (m->iCleanup.iThread != TheCurrentThread)
			{
			wm = ETrue;
			m->Wait();
			NKern::FlashSystem();
			}
		aCondVar->Broadcast(m);
		if (wm)
			m->Signal();
		else
			NKern::UnlockSystem();
		m->Close(NULL);
		aCondVar->Close(NULL);
		NKern::ThreadLeaveCS();
		}
	else
		NKern::UnlockSystem();
	}

TInt ExecHandler::CondVarCreate(const TDesC8* aName, TOwnerType aType)
	{
	TKName n;
	DObject* pO=NULL;
	const TDesC* pN=NULL;
	if (aName)
		{
		Kern::KUDesGet(n,*aName);
		pN=&n;
		__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Exec::CondVarCreate %S", pN));
		}
	else if (aType==EOwnerThread)
		pO=TheCurrentThread;
	else
		pO=TheCurrentThread->iOwningProcess;
	NKern::ThreadEnterCS();
	TInt r=KErrNoMemory;
	DCondVar* pV = new DCondVar;
	if (pV)
		{
		r = pV->Create(pO, pN, ETrue);
		if (r==KErrNone && aName)
			pV->SetProtection(n.Length()? DObject::EGlobal : DObject::EProtected);
		if (r==KErrNone)
			r = K::MakeHandle(aType, pV);
		if (r<KErrNone)
			pV->Close(NULL);
		}
	NKern::ThreadLeaveCS();
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Exec::CondVarCreate returns %d",r));
	return r;
	}

/********************************************
 * Chunk
 ********************************************/
DChunk::DChunk()
	{
	}

DChunk::~DChunk()
	{
	__COND_DEBUG_EVENT(iAttributes&EConstructed, EEventDeleteChunk, this);
	}


void DChunk::SetPaging(TUint /*aCreateAtt*/)
	{// Default implementation of virtual method that does nothing.
	}

TInt DChunk::Create(SChunkCreateInfo& aInfo)
	{
	SetOwner(aInfo.iOwner);
	TInt r=KErrNone;
	TBool named = (aInfo.iName.Ptr() && aInfo.iName.Length());
	if (named)
		{
		r=SetName(&aInfo.iName);
		if (r!=KErrNone)
			return r;
		}

	switch(aInfo.iType)
		{
		case ESharedKernelMultiple:
			SetProtection(DObject::EProtected);
			// fall through...
		case ESharedIo:
		case ESharedKernelSingle:
			// Shared kernel chunks can't be adjusted from user side
			iControllingOwner = K::TheKernelProcess->iId;
			iRestrictions = EChunkPreventAdjust;
			break;

		default:
			if(aInfo.iGlobal)
				SetProtection(named ? DObject::EGlobal : DObject::EProtected);
			iControllingOwner=TheCurrentThread->iOwningProcess->iId;
		}
	// Check if chunk is to own its memory
	if (aInfo.iAtt & TChunkCreate::EMemoryNotOwned)
		iAttributes |= EMemoryNotOwned;

	// Verify and save the mapping attributes.
	__ASSERT_COMPILE(DChunk::ENormal == 0);
	switch( aInfo.iAtt & TChunkCreate::EMappingMask)
		{
	case TChunkCreate::ENormal:
		break;
	case TChunkCreate::EDoubleEnded:
		iAttributes |= EDoubleEnded;
		break;
	case TChunkCreate::EDisconnected:
		iAttributes |= EDisconnected;
		break;
	case TChunkCreate::ECache:
		// Use TCB check to help keep cache chunks internal...
		if(!Kern::CurrentThreadHasCapability(ECapabilityTCB,__PLATSEC_DIAGNOSTIC_STRING("DChunk::Create")))
			return KErrPermissionDenied;
		iAttributes |= EDisconnected|ECache;
		break;
	default:
		return KErrArgument;
		}

	// Check if chunk is read-only
	if (aInfo.iAtt & TChunkCreate::EReadOnly)
		{
		iAttributes |= EReadOnly;
		iRestrictions |= EChunkPreventAdjust;
		}

	// Save the clear byte.
	iClearByte = aInfo.iClearByte;

	// Determine the data paging attributes.
	SetPaging(aInfo.iAtt);

	r=DoCreate(aInfo);
	if (r!=KErrNone)
		return r;

	r=K::AddObject(this,EChunk);
	if (r==KErrNone)
		{
		iAttributes|=EConstructed;
		__DEBUG_EVENT(EEventNewChunk, this);
		}
	return r;
	}

TInt DChunk::AddToProcess(DProcess* aProcess)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Adding chunk %O to process %O",this,aProcess));
	TBool readOnly = (iAttributes & EReadOnly) && (aProcess->iId != iControllingOwner);
	TInt r = aProcess->AddChunk(this, readOnly);
	if (r == KErrAccessDenied)
		{
		__KTRACE_OPT(KEXEC,Kern::Printf("Chunk is private - will not be mapped in to process"));
		r = KErrNone;
		}
	return r;
	}


void DChunk::BTracePrime(TInt aCategory)
	{
#ifdef BTRACE_CHUNKS
	if (aCategory == BTrace::EChunks || aCategory == -1)
		{
		TKName nameBuf;
		Name(nameBuf);
		BTraceN(BTrace::EChunks,BTrace::EChunkCreated,this,iMaxSize,nameBuf.Ptr(),nameBuf.Size());
		if(iOwningProcess)
			BTrace8(BTrace::EChunks,BTrace::EChunkOwner,this,iOwningProcess);
		BTrace12(BTrace::EChunks,BTrace::EChunkInfo,this,iChunkType,iAttributes);
		}
#endif
	}


SChunkCreateInfo::SChunkCreateInfo()
	{
	memset(this,0,sizeof(*this));
	iClearByte = KChunkClearByteDefault;
	}



/**
Create a chunk which can be shared between kernel and user code.

Once created, the chunk owns a region of linear address space of the size requested.
This region is empty (uncommitted) so before it can be used either RAM or I/O 
devices must be mapped into it. This is achieved with the Commit functions:

-  Kern::ChunkCommit()
-  Kern::ChunkCommitContiguous()
-  Kern::ChunkCommitPhysical()


@param aCreateInfo   A structure containing the required attributes of the chunk.
                     See TChunkCreateInfo.

@param aChunk        On return, this is set to point to the created chunk object.
                     This pointer is required as an argument for other functions
                     dealing with Shared Chunks.

@param aKernAddr     On return, this is set to the linear address in the kernel
                     process where the chunk's memory starts. This address should
                     only be used when executing kernel code; user code must not
                     use this address.

@param aMapAttr      On return, this is set to the mmu mapping attributes used for
                     the chunk. This is a value constructed from the bit masks in
                     the enumeration TMappingAttributes.
                     The typical use for this value is to use it as an argument to 
                     the Cache::SyncMemoryBeforeDmaWrite() and 
                     Cache::SyncMemoryBeforeDmaRead() methods.

@return KErrNone, if successful; otherwise one of the other system wide error codes.

@pre  Calling thread must be in a critical section.
@pre  No fast mutex can be held.
@pre  Call in a thread context.
@pre  Kernel must be unlocked.
@pre  interrupts enabled

@see TChunkCreateInfo
*/
EXPORT_C TInt Kern::ChunkCreate(const TChunkCreateInfo& aInfo, DChunk*& aChunk, TLinAddr& aKernAddr, TUint32& aMapAttr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::ChunkCreate");		
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkCreate type %d, maxSize %08x, mapAttr %08x", aInfo.iType, aInfo.iMaxSize, aInfo.iMapAttr));
	SChunkCreateInfo c;
	switch(aInfo.iType)
		{
	// Assert that chunk type enums are consistent between those used by TChunkCreateInfo and SChunkCreateInfo
	__ASSERT_COMPILE((TInt)TChunkCreateInfo::ESharedKernelSingle == (TInt)::ESharedKernelSingle);
	__ASSERT_COMPILE((TInt)TChunkCreateInfo::ESharedKernelMultiple == (TInt)::ESharedKernelMultiple);

	case TChunkCreateInfo::ESharedKernelSingle:
	case TChunkCreateInfo::ESharedKernelMultiple:
		c.iType = (TChunkType)aInfo.iType;
		c.iAtt = TChunkCreate::EDisconnected | (aInfo.iOwnsMemory? 0 : TChunkCreate::EMemoryNotOwned);
		c.iGlobal = ETrue;
		c.iForceFixed = ETrue;
		c.iMaxSize = aInfo.iMaxSize;
		c.iMapAttr = aInfo.iMapAttr;
		c.iOperations = SChunkCreateInfo::EAdjust; // To allocated virtual address space
		c.iDestroyedDfc = aInfo.iDestroyedDfc;
		break;

	default:
		return KErrArgument;
		}

	TInt r = K::TheKernelProcess->NewChunk(aChunk, c, aKernAddr);
	if(r==KErrNone)
		aMapAttr = aChunk->iMapAttr;
	else
		{
		if(aChunk)
			aChunk->Close(NULL), aChunk=0;				// can't have been added so NULL
		}

	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkCreate returns %d aChunk=%08x aKernAddr=%08x",r,aChunk,aKernAddr));
	return r;
	}



/**
Commit RAM to a shared chunk which was previously created with Kern::ChunkCreate().
The memory pages to commit are obtained from the system's free pool.

This method may only be used if the chunk was created with
TChunkCreateInfo::iOwnsMemory set to true.

@param aChunk   Pointer to the chunk.

@param aOffset  The offset (in bytes) from start of chunk, which indicates the start
                of the memory region to be committed. Must be a multiple of the MMU
                page size.

@param aSize    Number of bytes to commit. Must be a multiple of the MMU page size.

@return KErrNone, if successful; otherwise one of the other system wide error codes.

@pre  Calling thread must be in a critical section.
@pre  No fast mutex can be held.
@pre  Call in a thread context.
@pre  Kernel must be unlocked.
@pre  interrupts enabled

@post Calling thread is in a critical section.
*/
EXPORT_C TInt Kern::ChunkCommit(DChunk* aChunk, TInt aOffset, TInt aSize)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::ChunkCommit");		
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkCommit aChunk=%08x, aOffset=%08x, aSize=%08x", aChunk, aOffset, aSize));
	__ASSERT_DEBUG(aChunk->iChunkType == ESharedKernelSingle || aChunk->iChunkType == ESharedKernelMultiple, K::Fault(K::EChunkCommitBadType));
	TInt r = aChunk->Commit(aOffset, aSize);
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkCommit returns %d",r));
	return r;
	}


/**
Commit RAM to a shared chunk which was previously created with Kern::ChunkCreate().
The memory pages to commit are obtained from the system's free pool and will have
physically contiguous addresses.

This method may only be used if the chunk was created with
TChunkCreateInfo::iOwnsMemory set to true.

@param aChunk           Pointer to the chunk.

@param aOffset          The offset (in bytes) from start of chunk, which indicates
                        the start of the memory region to be committed. Must be a
                        multiple of the MMU page size.

@param aSize            Number of bytes to commit. Must be a multiple of the MMU
                        page size.

@param aPhysicalAddress On return, this is set to the physical address of the first
                        page of memory which was committed. I.e. the page at
                        aOffset.

@return KErrNone, if successful;
        KErrNoMemory, if there is insufficient free memory, or there is not a
                      contiguous region of the requested size;
        otherwise one of the other system wide error codes.

@pre  Calling thread must be in a critical section.
@pre  No fast mutex can be held.
@pre  Call in a thread context.
@pre  Kernel must be unlocked.
@pre  interrupts enabled

@post Calling thread is in a critical section.
*/
EXPORT_C TInt Kern::ChunkCommitContiguous(DChunk* aChunk, TInt aOffset, TInt aSize, TUint32& aPhysicalAddress)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::ChunkCommitContiguous");		
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkCommitContiguous aChunk=%08x, aOffset=%08x, aSize=%08x", aChunk, aOffset, aSize));
	__ASSERT_DEBUG(aChunk->iChunkType == ESharedKernelSingle || aChunk->iChunkType == ESharedKernelMultiple, K::Fault(K::EChunkCommitBadType));
	TInt r = aChunk->Commit(aOffset, aSize, DChunk::ECommitContiguous, &aPhysicalAddress);
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkCommitContiguous returns %d aPhysicalAddress=%08x",r,aPhysicalAddress));
	return r;
	}


/**
Commit memory to a shared chunk which was previously created with
Kern::ChunkCreate().

The physical region committed is that which starts at the supplied physical address.
Typically, this region either represents memory mapped i/o, or RAM which has been 
set aside for special use at system boot time.

This method may only be used if the chunk was created with
TChunkCreateInfo::iOwnsMemory set to false.

@param aChunk           Pointer to the chunk.

@param aOffset          The offset (in bytes) from start of chunk, which indicates
                        the start of the memory region to be committed. Must be a
                        multiple of the MMU page size.

@param aSize            Number of bytes to commit. Must be a multiple of the MMU
                        page size.

@param aPhysicalAddress The physical address of the memory to be commited to the
                        chunk. Must be a multiple of the MMU page size.

@return KErrNone, if successful; otherwise one of the other system wide error codes.

@pre  Calling thread must be in a critical section.
@pre  No fast mutex can be held.
@pre  Call in a thread context.
@pre  Kernel must be unlocked.
@pre  interrupts enabled

@post Calling thread is in a critical section.
*/
EXPORT_C TInt Kern::ChunkCommitPhysical(DChunk* aChunk, TInt aOffset, TInt aSize, TUint32 aPhysicalAddress)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::ChunkCommitPhysical(DChunk* aChunk, TInt aOffset, TInt aSize, TUint32 aPhysicalAddress)");		
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkCommitPhysical aChunk=%08x, aOffset=%08x, aSize=%08x aPhysicalAddress=%08x", aChunk, aOffset, aSize, aPhysicalAddress));
	__ASSERT_DEBUG(aChunk->iChunkType == ESharedKernelSingle || aChunk->iChunkType == ESharedKernelMultiple, K::Fault(K::EChunkCommitBadType));
	TInt r = aChunk->Commit(aOffset, aSize, DChunk::ECommitContiguousPhysical, (TUint32*)aPhysicalAddress);
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkCommit returns %d",r));
	return r;
	}


/**
Commit memory to a shared chunk which was previously created with
Kern::ChunkCreate().

The physical region committed is determined by the list of physical addresses
supplied to this function. Typically, this region either represents memory mapped 
i/o, or RAM which has been set aside for special use at system boot time.

This method may only be used if the chunk was created with
TChunkCreateInfo::iOwnsMemory set to false.

@param aChunk           Pointer to the chunk.

@param aOffset          The offset (in bytes) from start of chunk, which indicates
                        the start of the memory region to be committed. Must be a
                        multiple of the MMU page size.

@param aSize            Number of bytes to commit. Must be a multiple of the MMU
                        page size.

@param aPhysicalAddress A pointer to a list of physical addresses, one address for
                        each page of memory committed. Each physical address must be
                        a multiple of the MMU page size.

@return KErrNone, if successful; otherwise one of the other system wide error codes.

@pre  Calling thread must be in a critical section.
@pre  No fast mutex can be held.
@pre  Call in a thread context.
@pre  Kernel must be unlocked.
@pre  interrupts enabled

@post Calling thread is in a critical section.
*/
EXPORT_C TInt Kern::ChunkCommitPhysical(DChunk* aChunk, TInt aOffset, TInt aSize, const TUint32* aPhysicalAddressList)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::ChunkCommitPhysical(DChunk* aChunk, TInt aOffset, TInt aSize, const TUint32* aPhysicalAddressList)");		
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkCommitPhysical aChunk=%08x, aOffset=%08x, aSize=%08x aPhysicalAddressList=%08x", aChunk, aOffset, aSize, aPhysicalAddressList));
	__ASSERT_DEBUG(aChunk->iChunkType == ESharedKernelSingle || aChunk->iChunkType == ESharedKernelMultiple, K::Fault(K::EChunkCommitBadType));
	TInt r = aChunk->Commit(aOffset, aSize, DChunk::ECommitDiscontiguousPhysical, (TUint32*)aPhysicalAddressList);
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkCommit returns %d",r));
	return r;
	}


/**
Close a chunk created with Kern::ChunkCreate().

If the reference count of the chunk has gone to zero, any memory 
committed to the chunk will be decommited immediately but the chunk 
object will be deleted asynchronously.

@param aChunk  Pointer to the chunk.

@return True if the reference count of the chunk has gone to zero.
        False otherwise.

@pre  Calling thread must be in a critical section.
@pre  No fast mutex can be held.
@pre  Call in a thread context.
@pre  Kernel must be unlocked.
@pre  interrupts enabled
*/
EXPORT_C TBool Kern::ChunkClose(DChunk* aChunk)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkClose aChunk=%08x", aChunk));

	TBool r = (aChunk->Dec() == 1);
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkClose returns %d",r));
	if (r)
		{
		// Decommit all the memory from the chunk which is safe as no further
		// reference can be made to the chunk now its access count is 0. Decommit
		// is required to ensure any committed pages have their usage count
		// decremented immediately to allow any physically committed memory to
		// be freed after this method which may occur before aChunk is deleted.
		aChunk->Decommit(0, aChunk->MaxSize());
		aChunk->AsyncDelete();
		}
	return r;
	}


/**
Open a shared chunk in which the given address lies.

@param aThread          The thread in whose process the given address lies.
                        If aThread is zero, then the current thread is used.

@param aAddress         An address in the given threads process.

@param aWrite           A flag which is true if the chunk memory is intended to be
                        written to, false otherwise.

@param aOffset          On return, this is set to the offset within the chunk which
                        correspons to aAddress.

@return If the supplied address is within a shared chunk mapped into aThread's
        process, then the returned value is a pointer to this chunk.
        Otherwise zero is returned.

@pre  Calling thread must be in a critical section.
@pre  No fast mutex can be held.
@pre  Call in a thread context.
@pre  Kernel must be unlocked.
@pre  interrupts enabled

@post If a chunk pointer is returned, then the access count on this chunk has been
      incremented. I.e. Open() has been called on it.
*/
EXPORT_C DChunk* Kern::OpenSharedChunk(DThread* aThread,const TAny* aAddress, TBool aWrite, TInt& aOffset)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::OpenSharedChunk(DThread* aThread,const TAny* aAddress, TBool aWrite, TInt& aOffset)");		
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::OpenSharedChunk aThread=%08x, aAddress=%08x, aWrite=%d", aThread, aAddress, aWrite));	
	if(!aThread)
		aThread = &Kern::CurrentThread();
	DChunk* chunk = aThread->OpenSharedChunk(aAddress,aWrite,aOffset);
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::OpenSharedChunk returns %08x aOffset=%08x",chunk,aOffset));
	return chunk;
	}


/**
Open a shared chunk using a given handle.

Typically, this handle would be a value supplied by a user application which 
obtained the value by using RChunk::Handle().

@param aThread          The thread in which the given handle is valid.

@param aChunkHandle     The handle value.

@param aWrite           A flag which is true if the chunk memory is intended to be
                        written to, false otherwise.

@return If the handle is a valid chunk handle, and it is of a shared chunk type,
        then the returned value is a pointer to this chunk.
        Otherwise zero is returned.

@pre  Calling thread must be in a critical section.
@pre  No fast mutex can be held.

@post If a chunk pointer is returned, then the access count on this chunk has been
      incremented. I.e. Open() has been called on it.
*/
EXPORT_C DChunk* Kern::OpenSharedChunk(DThread* aThread, TInt aChunkHandle, TBool aWrite)
	{
	CHECK_PRECONDITIONS(MASK_CRITICAL|MASK_NO_FAST_MUTEX,"Kern::OpenSharedChunk(DThread* aThread, TInt aChunkHandle, TBool aWrite)");		
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::OpenSharedChunk aThread=%08x, aHandle=%08x, aWrite=%d", aThread, aChunkHandle, aWrite));
	if(!aThread)
		aThread = &Kern::CurrentThread();
	NKern::LockSystem();
	DChunk* chunk = aThread->OpenSharedChunk(aChunkHandle,aWrite);
	NKern::UnlockSystem();
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::OpenSharedChunk returns %08x",chunk));
	return chunk;
	}


/**
@pre System lock must be held.
*/
DChunk* DThread::OpenSharedChunk(TInt aHandle, TBool /*aWrite*/)
	{
	DChunk* chunk = (DChunk*)ObjectFromHandle(aHandle,EChunk);
	if(	chunk 
		&& (chunk->iChunkType==ESharedKernelSingle || chunk->iChunkType==ESharedKernelMultiple)
		&& chunk->Open()==KErrNone)
			return chunk;
	return 0;
	}


/**
Get the linear address for a region in a shared chunk.

The chunk must be of a shared chunk type and the specified region must
contain committed memory.

@param aChunk           The chunk

@param aOffset          The start of the region as an offset in bytes from the
                        start of the chunk.

@param aSize            The size of the region in bytes.

@param aKernelAddress   On success, this value is set to the linear address in the
                        kernel process which coresponds to first byte in the
                        specified region.

@return KErrNone if successful.
        KErrAccessDenied if the chunk isn't a shared chunk type.
        KErrArgument if the region isn't within the chunk.
        KErrNotFound if the whole region doesn't contain comitted memory.

@pre  No fast mutex can be held.
*/
EXPORT_C TInt Kern::ChunkAddress(DChunk* aChunk, TInt aOffset, TInt aSize, TLinAddr& aKernelAddress)
	{
	CHECK_PRECONDITIONS(MASK_NO_FAST_MUTEX,"Kern::ChunkAddress");		
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkAddress aChunk=%08x, aOffset=%08x, aSize=%d", aChunk, aOffset, aSize));
	TInt r = aChunk->Address(aOffset,aSize,aKernelAddress);
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkAddress returns %d aKernelAddress=%08x",r,aKernelAddress));
	return r;
	}


/**
Get the linear address for the base of a shared chunk within a user process.
Note, this address may become invalid if the process closes then re-opens the chunk.

@param aChunk           The chunk

@param aThread			The thread in whose process the returned address lies.

@return the base address of the shared chunk in the specified user process.

@pre  No fast mutex can be held.
*/
EXPORT_C TUint8* Kern::ChunkUserBase(DChunk* aChunk, DThread* aThread)
	{
	CHECK_PRECONDITIONS(MASK_NO_FAST_MUTEX,"Kern::ChunkUserBase");
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkUserBase aChunk=%08x, aThread=%08x", aChunk, aThread));
	NKern::LockSystem();
	TUint8* r = aChunk->Base(aThread->iOwningProcess);
	NKern::UnlockSystem();
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkUserBase returns %08x", r));
	return r;
	}



/**
Get the physical address for a region in a shared chunk.

The chunk must be of a shared chunk type and the specified region must
contain committed memory.

@param aChunk           The chunk

@param aOffset          The start of the region as an offset in bytes from the
                        start of the chunk.

@param aSize            The size of the region in bytes.

@param aKernelAddress   On success, this value is set to the linear address in the
                        kernel process which coresponds to first byte in the
                        specified region.

@param aMapAttr         On success, this is set to the mmu mapping attributes used
                        for the chunk. This is a value constructed from the bit
                        masks in the enumeration TMappingAttributes.
                        The typical use for this value is to use it as an argument 
                        to the Cache::SyncMemoryBeforeDmaWrite() and 
                        Cache::SyncMemoryBeforeDmaRead() methods.

@param aPhysicalAddress On success, this value is set to one of two values.
						If the specified region is physically contiguous, the value
						is the physical address of the first byte in the region.
						If the region is discontiguous, the value is set to KPhysAddrInvalid.

@param aPageList        If not zero, this points to an array of TUint32
                        (or TPhysAddr) objects. The length of the array must be at
                        least (aSize + MMU_page_size-2)/MMU_page_size + 1,
                        where MMU_page_size = Kern::RoundToPageSize(1).
                        On success, this array will be filled with the addresses of
                        the physical pages which contain the specified region.
                        These addresses are the start of each page, (they are a
                        multiple of the physical page size), therefore the byte 
                        corresponding to aOffset is at physical address
                        aPageList[0]+aOffset%MMU_page_size.
						If aPageList is zero (the default), then the function will fail with
						KErrNotFound if the specified region is not physically contiguous.

@return 0 if successful and the whole region is physically contiguous.
        1 if successful but the region isn't physically contiguous.
        KErrAccessDenied if the chunk isn't a shared chunk type.
        KErrArgument if the region isn't within the chunk.
        KErrNotFound if the whole region doesn't contain comitted memory
					 or aPageList==0 and the specified region is not physically contiguous.

@pre  No fast mutex can be held.
*/
EXPORT_C TInt Kern::ChunkPhysicalAddress(DChunk* aChunk, TInt aOffset, TInt aSize, TLinAddr& aKernelAddress, TUint32& aMapAttr, TUint32& aPhysicalAddress, TUint32* aPageList)
	{
	CHECK_PRECONDITIONS(MASK_NO_FAST_MUTEX,"Kern::ChunkPhysicalAddress");		
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkPhysicalAddress aChunk=%08x, aOffset=%08x, aSize=%d", aChunk, aOffset, aSize));
	TInt r = aChunk->PhysicalAddress(aOffset,aSize,aKernelAddress,aPhysicalAddress,aPageList);
	if(r >= 0)	/* KErrNone or 1 (i.e. not phys contig) are both successful returns */
		aMapAttr = aChunk->iMapAttr;
	__KTRACE_OPT(KMMU,Kern::Printf("Kern::ChunkPhysicalAddress returns %d aKernelAddress=%08x aPhysicalAddress=%08x",r,aKernelAddress,aPhysicalAddress));
	return r;
	}

/**
Returns the list of physical addresses of the specified virtual memory region - if the whole region is safe
to DMA to/from. A region of virtual space is considered safe if it belongs to a chunk which is marked by
FileServer as being trusted to perform DMA to/from. File system must ensure physical pages are not
decomitted or unlocked for demand paging until DMA transfer is completed.
This will also lock the pages to prevent them of being moved by ram defrag.

@see UserSvr::RegisterTrustedChunk
@see Kern::ReleaseMemoryFromDMA

@param aThread          The thread in whose process the given address lies. If zero, the current thread is used.
@param aAddress         An address in the given thread's process.
@param aSize            The size of the region.
@param aPageList        Points to an array of TUint32 (or TPhysAddr) objects. The length of the array must
						be at least aSize/MMU_page_size+1, where MMU_page_size = Kern::RoundToPageSize(1).
                        On success, this array will be filled with the addresses of the physical pages
                        which contain the specified region. These addresses are the start of each page,
                        (they are a multiple of the physical page size), therefore the byte corresponding
                        to aOffset is at physical address aPageList[0]+aOffset%MMU_page_size.

@return KErrNone		 if successful.
        KErrAccessDenied if any part of region doesn't belong to "trusted" chunk.
        Other			 if memory region is invalid, or mapped in 1MB sections or large pages.

@pre	Calling thread must be in critical section.
@pre	No fast mutex held.

@internalComponent
*/
EXPORT_C TInt Kern::PrepareMemoryForDMA(DThread* aThread, TAny* aAddress, TInt aSize, TPhysAddr* aPageList)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::PrepareMemoryForDMA");
	__KTRACE_OPT(KMMU2,Kern::Printf("Kern::PrepareMemoryForDMA T:%x, A:%x, S:%x, Prt:%x",aThread, aAddress, aSize, aPageList));
	if(!aThread)
		aThread = &Kern::CurrentThread();
	return aThread->PrepareMemoryForDMA(aAddress, aSize, aPageList);
	}

/**
Unlocks the physical pages that have been locked by PrepareMemoryForDMA.
All input parameters are the same as those in PrepareMemoryForDMA.

@see Kern::PrepareMemoryForDMA

@param aThread          The thread in whose process the given address lies. If zero, the current thread is used.
@param aAddress         An address in the given thread's process.
@param aSize            The size of the region.
@param aPageList        Points to the list of pages returned by PrepareMemoryForDMA.

@return KErrNone		if successful.
        KErrArgument	if the list of physical pages is invalid.
        
@pre	Calling thread must be in critical section.
@pre	No fast mutex held.

@internalComponent
*/
EXPORT_C TInt Kern::ReleaseMemoryFromDMA(DThread* aThread, TAny* aAddress, TInt aSize, TPhysAddr* aPageList)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::ReleaseMemoryFromDMA");
	__KTRACE_OPT(KMMU2,Kern::Printf("Kern::ReleaseMemoryFromDMA T:%x, A:%x, S:%x, Prt:%x",aThread, aAddress, aSize, aPageList));
	if(!aThread)
		aThread = &Kern::CurrentThread();
	return aThread->ReleaseMemoryFromDMA(aAddress, aSize, aPageList);
	}

/**
Installs Trace Handler Hook.
@param aHandler Trace Handler Hook. Will be called on any debug log .It includes user-side
                (@see RDebug::Print), kernel (@see Kern::Printf) and PlatSec logging.
                Set to NULL to uninstall the hook.
@see TTraceHandler
@return Previous hook or NULL.
*/
EXPORT_C TTraceHandler Kern::SetTraceHandler(TTraceHandler aHandler)
	{
	return (TTraceHandler) SetHook(EHookTrace, (TKernelHookFn)aHandler, ETrue);
	}

/********************************************
 * Kernel event dispatcher
 ********************************************/

/** Returns whether the kernel has been built with __DEBUGGER_SUPPORT__ defined.  */

EXPORT_C TBool DKernelEventHandler::DebugSupportEnabled()
	{
#ifdef __DEBUGGER_SUPPORT__
	return ETrue;
#else
	return EFalse;
#endif
	}


/** Constructs an event handler.
	The handler is not queued.
	@param aCb Pointer to C callback function called when an event occurs.
	@param aPrivateData Data to be passed to the callback function.

	@pre Calling thread must be in a critical section.
    @pre No fast mutex can be held.
	@pre Call in a thread context.
	@pre Kernel must be unlocked
	@pre interrupts enabled
 */

EXPORT_C DKernelEventHandler::DKernelEventHandler(TCallback aCb, TAny* aPrivateData)
	:	iAccessCount(1),
		iCb(aCb),
		iPrivateData(aPrivateData)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DKernelEventHandler::DKernelEventHandler");
	__ASSERT_DEBUG(aCb != NULL, K::Fault(K::EDebugEventHandlerBadCallBack));
	}

/** Adds handler in handler queue.
	@param aPolicy Selects where the handler should be inserted.
	@return standard error code

	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */

EXPORT_C TInt DKernelEventHandler::Add(TAddPolicy /*aPolicy*/)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"DKernelEventHandler::Add");
	NKern::LockSystem();
	if (!iLink.iNext)
		{
		HandlersQ.Add(&iLink);
		}
	NKern::UnlockSystem();
	return KErrNone;
	}

/** Decrements access count. 
	Removes from queue and asynchronously destruct it if access count reaches zero.
	@return original access count.

	@pre Calling thread must be in a critical section.
    @pre No fast mutex can be held.
	@pre Call in a thread context.
	@pre Kernel must be unlocked
	@pre interrupts enabled
 */

EXPORT_C TInt DKernelEventHandler::Close()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DKernelEventHandler::Close");
	NKern::LockSystem();
	TInt r=iAccessCount;
	if (r>0 && --iAccessCount==0)
		{
		if (iLink.iNext)
			{
			iLink.Deque();
			iLink.iNext=NULL;
			}
		}
	NKern::UnlockSystem();
	if (r==1)
		AsyncDelete();
	return r;
	}

/**	Sends event to all handlers in queue.
 	@internalTechnology
 */

TUint DKernelEventHandler::Dispatch(TKernelEvent aType, TAny* a1, TAny* a2)
	{
	TUint action = ERunNext;
	NKern::ThreadEnterCS();
	NKern::LockSystem();
	SDblQueLink* pE=&HandlersQ.iA;
	SDblQueLink* pL=pE->iNext;
	DKernelEventHandler* pD=NULL;
	while (pL!=pE)
		{
		DKernelEventHandler* pH=_LOFF(pL,DKernelEventHandler,iLink);
		++pH->iAccessCount;
		NKern::UnlockSystem();
		if (pD)
			{
			pD->AsyncDelete();
			pD=NULL;
			}
		action=pH->iCb(aType, a1, a2, pH->iPrivateData);
		NKern::LockSystem();
		SDblQueLink* pN=pL->iNext;
		if (--pH->iAccessCount==0)
			{
			pL->Deque();
			pL->iNext=NULL;
			pD=pH;
			}
		if (!(action & ERunNext))
			break;
		pL=pN;
		}
	NKern::UnlockSystem();
	if (pD)
		pD->AsyncDelete();
	NKern::ThreadLeaveCS();
	return action;
	}


/******************************************************************************
 * Memory saving fast deterministic thread wait queue
 ******************************************************************************/
TThreadWaitList::TList* TThreadWaitList::FirstFree;
TInt TThreadWaitList::NLists;
TInt TThreadWaitList::NWaitObj;
TInt TThreadWaitList::NThrd;

TThreadWaitList::~TThreadWaitList()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"~TThreadWaitList");
	__ASSERT_ALWAYS((iWaitPtr==EEmpty || iWaitPtr==EInitValue), K::Fault(K::EThreadWaitListDestroy));
	if (iWaitPtr==EEmpty)
		Down(EFalse);
	}

TInt TThreadWaitList::Construct()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TThreadWaitList::Construct");
	TInt r = Up(EFalse);
	if (r==KErrNone)
		iWaitPtr = EEmpty;
	return r;
	}

TInt TThreadWaitList::ThreadCreated()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TThreadWaitList::ThreadCreated");
	return Up(ETrue);
	}

void TThreadWaitList::ThreadDestroyed()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TThreadWaitList::ThreadDestroyed");
	Down(ETrue);
	}

TInt TThreadWaitList::Up(TBool aThread)
	{
	TList* l = 0;
	TInt r = 1;
	do	{
		NKern::LockSystem();
		__KTRACE_OPT(KSEMAPHORE,Kern::Printf(">TThreadWaitList::Up W=%d T=%d L=%d l=%08x", NWaitObj, NThrd, NLists, l));
		TInt nw=NWaitObj, nt=NThrd;
		aThread ? ++nt : ++nw;
		TInt needed = Min(nt/2, nw);
		if (needed<=NLists)
			goto done;
		else if (l)
			{
			++NLists;
			l->Next() = FirstFree;
			FirstFree = l;
			l = 0;
done:
			NThrd=nt, NWaitObj=nw;
			r = KErrNone;
			}
		__KTRACE_OPT(KSEMAPHORE,Kern::Printf("<TThreadWaitList::Up W=%d T=%d L=%d l=%08x", NWaitObj, NThrd, NLists, l));
		NKern::UnlockSystem();
		if (r!=KErrNone)
			{
			__ASSERT_ALWAYS(!l, K::Fault(K::EThreadWaitListUp));
			l = new TList;
			if (!l)
				r = KErrNoMemory;
			}
		} while (r>0);
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("<TThreadWaitList::Up l=%08x r=%d", l, r));
	delete l;
	return r;
	}

void TThreadWaitList::Down(TBool aThread)
	{
	TList* l = 0;
	NKern::LockSystem();
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf(">TThreadWaitList::Down W=%d T=%d L=%d", NWaitObj, NThrd, NLists));
	aThread ? --NThrd : --NWaitObj;
	TInt needed = Min(NThrd/2, NWaitObj);
	if (needed < NLists)
		{
		--NLists;
		l = FirstFree;
		FirstFree = l->Next();
		}
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("<TThreadWaitList::Down W=%d T=%d L=%d l=%08x", NWaitObj, NThrd, NLists, l));
	NKern::UnlockSystem();
	delete l;
	}

#ifndef __PRI_LIST_MACHINE_CODED__
DThread* TThreadWaitList::First() const
	{
	__DEBUG_ONLY(Check());
	if (iWaitPtr == EEmpty)
		return 0;
	if (!(iWaitPtr & EFlagList))
		return (DThread*)iWaitPtr;
	TList* l = (TList*)(iWaitPtr&~EFlagList);
	return _LOFF(l->First(),DThread,iWaitLink);
	}

TInt TThreadWaitList::HighestPriority() const
	{
	__DEBUG_ONLY(Check());
	if (iWaitPtr==EEmpty)
		return -1;
	if (!(iWaitPtr & EFlagList))
		return ((DThread*)iWaitPtr)->iWaitLink.iPriority;
	TList* l = (TList*)(iWaitPtr&~EFlagList);
	return l->HighestPriority();
	}

void TThreadWaitList::Add(DThread* aThread)
	{
	__DEBUG_ONLY(Check());
	if (iWaitPtr==EEmpty)
		{
		iWaitPtr = (TLinAddr)aThread;
		return;
		}
	TList* l = 0;
	if (iWaitPtr&EFlagList)
		l = (TList*)(iWaitPtr&~EFlagList);
	else
		{
		DThread* t0 = (DThread*)iWaitPtr;
		l = FirstFree;
		FirstFree = l->Next();
		l->Next() = 0;
		iWaitPtr = TLinAddr(l)|EFlagList;
		l->Add(&t0->iWaitLink);
		}
	l->Add(&aThread->iWaitLink);
	}

void TThreadWaitList::Remove(DThread* aThread)
	{
	__DEBUG_ONLY(Check());
	__ASSERT_DEBUG(iWaitPtr!=EEmpty && ((iWaitPtr&EFlagList)||(iWaitPtr==(TLinAddr)aThread)), K::Fault(K::EThreadWaitListRemove));
	if (!(iWaitPtr&EFlagList))
		{
		iWaitPtr = EEmpty;
		return;
		}
	TList* l = (TList*)(iWaitPtr&~EFlagList);
	l->Remove(&aThread->iWaitLink);
	TUint p0 = l->iPresent[0];
	TUint p1 = l->iPresent[1];
	if ((p0&&p1) || (p0&(p0-1)) || (p1&(p1-1)))
		return;
	TPriListLink* wl = l->First();
	__ASSERT_DEBUG(wl, K::Fault(K::EThreadWaitListRemove2));
	if (wl->iNext != wl)
		return;
	DThread* t = _LOFF(wl,DThread,iWaitLink);
	l->Remove(&t->iWaitLink);
	iWaitPtr = (TLinAddr)t;
	l->Next() = FirstFree;
	FirstFree = l;
	}

void TThreadWaitList::ChangePriority(DThread* aThread, TInt aNewPriority)
	{
	__DEBUG_ONLY(Check());
	__ASSERT_DEBUG(iWaitPtr!=EEmpty && ((iWaitPtr&EFlagList)||(iWaitPtr==(TLinAddr)aThread)), K::Fault(K::EThreadWaitListChangePriority));
	if (!(iWaitPtr & EFlagList))
		aThread->iWaitLink.iPriority = (TUint8)aNewPriority;
	else
		{
		TList* l = (TList*)(iWaitPtr&~EFlagList);
		l->ChangePriority(&aThread->iWaitLink, aNewPriority);
		}
	}
#endif

#ifdef _DEBUG
// Check the system lock is held and second phase construction has completed
// successfully
void TThreadWaitList::Check() const
	{
	__ASSERT_SYSTEM_LOCK;
	TUint32 mask = RHeap::ECellAlignment-1;
	TUint32 lowbits = iWaitPtr & mask;
	__ASSERT_ALWAYS(lowbits<=1, K::Fault(K::EThreadWaitListCheck));
	}
#endif
