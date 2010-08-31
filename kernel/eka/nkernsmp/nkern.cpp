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
// e32\nkernsmp\nkern.cpp
// 
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

#include "nk_priv.h"

/******************************************************************************
 * Fast mutex
 ******************************************************************************/

/** Acquires the fast mutex.

    This will block until the mutex is available, and causes
	the thread to enter an implicit critical section until the mutex is released.

	Generally threads would use NKern::FMWait() which manipulates the kernel lock
	for you.
	
	@pre Kernel must be locked, with lock count 1.
	@pre The calling thread holds no fast mutexes.
	
	@post Kernel is locked, with lock count 1.
	@post The calling thread holds the mutex.
	
	@see NFastMutex::Signal()
	@see NKern::FMWait()
*/
EXPORT_C void NFastMutex::Wait()
	{
	NThreadBase* pC = NCurrentThreadL();
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED_ONCE|MASK_NO_FAST_MUTEX,"NFastMutex::Wait");

	pC->iHeldFastMutex = this;		// to handle kill/suspend between here and setting iHeldFastMutex
	DoWaitL();
	}

void NFastMutex::DoWaitL()
	{
	NThreadBase* pC = NCurrentThreadL();
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T FMWait %M",pC,this));
	TBool waited = FALSE;				// set if we actually had to wait
	iMutexLock.LockOnly();			// acquire mutex spin lock
	__e32_atomic_ior_rlx_ptr(&iHoldingThread, 1);		// set contention flag to make sure any other thread must acquire the mutex spin lock
	pC->AcqSLock();
	FOREVER
		{
		if (pC->iFastMutexDefer == 1)
			--pC->iParent->iFreezeCpu;
		pC->iFastMutexDefer = 0;
		NThreadBase* pH = (NThreadBase*)(TLinAddr(iHoldingThread) &~ 1);
		if (!pH)
			{
			// mutex is free
			TInt wp = iWaitQ.HighestPriority();		// -1 if no other thread wants the mutex

			// don't grab mutex if we have been suspended/killed/migrated by the previous holding thread
			if (!pC->iSuspended && pC->iCsFunction!=NThreadBase::ECSDivertPending && (!pC->iParent->iCpuChange || pC->iParent->iFreezeCpu))
				{
				TInt p = pC->iPriority;
				if (p>wp || (p==wp && waited))
					{
					// if we are highest priority waiting thread or equal and we have waited then grab the mutex
					// don't just grab it if we are equal priority and someone else was already waiting
					// set contention flag if other threads waiting or if current thread has a round robin outstanding
					pC->iMutexPri = (TUint8)(wp>=0 ? wp : 0);	// pC's actual priority doesn't change since p>=wp
					iHoldingThread = (wp>=0 || TUint32(pC->iTime)==0x80000000u) ? (NThreadBase*)(TLinAddr(pC)|1) : pC;
					__KTRACE_OPT(KNKERN,DEBUGPRINT("%T got mutex %M CF=%d WP=%d",TLinAddr(iHoldingThread)&~1,this,TLinAddr(iHoldingThread)&1,wp));
					pC->RelSLock();
					iMutexLock.UnlockOnly();
#ifdef BTRACE_FAST_MUTEX
					BTraceContext4(BTrace::EFastMutex, BTrace::EFastMutexWait, this);
#endif
					return;
					}
				}
			}
		pC->iFastMutexDefer = 2;	// signal to scheduler to allow ctxsw without incrementing iParent->iFreezeCpu
		if (!pC->iSuspended && pC->iCsFunction!=NThreadBase::ECSDivertPending && (!pC->iParent->iCpuChange || pC->iParent->iFreezeCpu))
			{
			// this forces priority changes to wait for the mutex lock
			pC->iLinkedObjType = NThreadBase::EWaitFastMutex;
			pC->iLinkedObj = this;
			pC->iWaitState.SetUpWait(NThreadBase::EWaitFastMutex, NThreadWaitState::EWtStObstructed, this);
			pC->iWaitLink.iPriority = pC->iPriority;
			if (waited)
				iWaitQ.AddHead(&pC->iWaitLink);	// we were next at this priority
			else
				iWaitQ.Add(&pC->iWaitLink);
			pC->RelSLock();
			if (pH)
				pH->SetMutexPriority(this);
do_pause:
			iMutexLock.UnlockOnly();
			RescheduleNeeded();
#ifdef BTRACE_FAST_MUTEX
			BTraceContext4(BTrace::EFastMutex, BTrace::EFastMutexBlock, this);
#endif
			NKern::PreemptionPoint();	// we block here until the mutex is released and we are 'nominated' for it or we are suspended/killed
			iMutexLock.LockOnly();
			pC->AcqSLock();
			if (pC->iPauseCount || pC->iSuspended || pC->iCsFunction==NThreadBase::ECSDivertPending || (pC->iParent->iCpuChange && !pC->iParent->iFreezeCpu))
				{
				pC->RelSLock();
				goto do_pause;			// let pause/suspend/kill take effect
				}
			// if thread was suspended it will have been removed from the wait queue
			if (!pC->iLinkedObj)
				goto thread_suspended;
			iWaitQ.Remove(&pC->iWaitLink);	// take ourselves off the wait/contend queue while we try to grab the mutex
			pC->iWaitLink.iNext = 0;
			pC->iLinkedObj = 0;
			pC->iLinkedObjType = NThreadBase::EWaitNone;
			waited = TRUE;
			// if we are suspended or killed, we loop round again and do the 'else' clause next time
			}
		else
			{
			pC->RelSLock();
			if (pC->iSuspended || pC->iCsFunction==NThreadBase::ECSDivertPending)
				{
				// wake up next thread to take this one's place
				if (!pH && !iWaitQ.IsEmpty())
					{
					NThreadBase* pT = _LOFF(iWaitQ.First(), NThreadBase, iWaitLink);
					pT->AcqSLock();
					// if thread is still blocked on this fast mutex, release it but leave it on the wait queue
					// NOTE: it can't be suspended
					pT->iWaitState.UnBlockT(NThreadBase::EWaitFastMutex, this, KErrNone);
					pT->RelSLock();
					}
				}
			iMutexLock.UnlockOnly();
			NKern::PreemptionPoint();	// thread suspends/dies/migrates here
			iMutexLock.LockOnly();
			pC->AcqSLock();
thread_suspended:
			waited = FALSE;
			// set contention flag to make sure any other thread must acquire the mutex spin lock
			// need to do it again since mutex may have been released while thread was suspended
			__e32_atomic_ior_rlx_ptr(&iHoldingThread, 1);
			}
		}
	}


#ifndef __FAST_MUTEX_MACHINE_CODED__
/** Releases a previously acquired fast mutex.
	
	Generally, threads would use NKern::FMSignal() which manipulates the kernel lock
	for you.
	
	@pre The calling thread holds the mutex.
	@pre Kernel must be locked.
	
	@post Kernel is locked.
	
	@see NFastMutex::Wait()
	@see NKern::FMSignal()
*/
EXPORT_C void NFastMutex::Signal()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED,"NFastMutex::Signal");
#ifdef BTRACE_FAST_MUTEX
	BTraceContext4(BTrace::EFastMutex, BTrace::EFastMutexSignal, this);
#endif
	NThreadBase* pC = NCurrentThreadL();
	((volatile TUint32&)pC->iHeldFastMutex) |= 1;	// flag to indicate about to release mutex

	if (__e32_atomic_cas_rel_ptr(&iHoldingThread, &pC, 0))
		{
		// tricky if suspend/kill here
		// suspend/kill should check flag set above and aMutex->iHoldingThread
		// if bit 0 of iHeldFastMutex set and iHoldingThread==pC then set iHeldFastMutex=0 and proceed

		// no-one else was waiting for the mutex - simple
		pC->iHeldFastMutex = 0;
		return;
		}

	// there was contention so do it the hard way
	DoSignalL();
	}
#endif

void NFastMutex::DoSignalL()
	{
	NThreadBase* pC = NCurrentThreadL();
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T FMSignal %M",pC,this));
	__ASSERT_WITH_MESSAGE_DEBUG(HeldByCurrentThread(),"The calling thread holds the mutex","NFastMutex::Signal");

	iMutexLock.LockOnly();
	if (!iWaitQ.IsEmpty())
		{
		NThreadBase* pT = _LOFF(iWaitQ.First(), NThreadBase, iWaitLink);
		pT->AcqSLock();

		// if thread is still blocked on this fast mutex, release it but leave it on the wait queue
		// NOTE: it can't be suspended
		pT->iWaitState.UnBlockT(NThreadBase::EWaitFastMutex, this, KErrNone);
		pT->RelSLock();
		iHoldingThread = (NThreadBase*)1;	// mark mutex as released but contended
		}
	else
		iHoldingThread = 0;	// mark mutex as released and uncontended
	__KTRACE_OPT(KNKERN,DEBUGPRINT("SiHT=%d",iHoldingThread));
	pC->AcqSLock();
	pC->iHeldFastMutex = 0;
	iMutexLock.UnlockOnly();
	pC->iMutexPri = 0;
	if (pC->iPriority != pC->iBasePri)
		{
		// lose any inherited priority
		pC->LoseInheritedPriorityT();
		}
	if (TUint32(pC->iTime)==0x80000000u)
		{
		pC->iTime = 0;
		RescheduleNeeded();	// handle deferred timeslicing
		__KTRACE_OPT(KNKERN,DEBUGPRINT("DTS %T",pC));
		}
	if (pC->iFastMutexDefer)
		{
		pC->iFastMutexDefer = 0;
		--pC->iParent->iFreezeCpu;
		}
	if (pC->iParent->iCpuChange && !pC->iParent->iFreezeCpu)
		RescheduleNeeded();	// need to migrate to another CPU
	if (!pC->iCsCount && pC->iCsFunction)
		pC->DoCsFunctionT();
	pC->RelSLock();
	}


/** Checks if the current thread holds this fast mutex

	@return TRUE if the current thread holds this fast mutex
	@return FALSE if not
	@pre	Call in thread context.
*/
EXPORT_C TBool NFastMutex::HeldByCurrentThread()
	{
	return (TLinAddr(iHoldingThread)&~1) == (TLinAddr)NKern::CurrentThread();
	}


/** Returns the fast mutex held by the calling thread, if any.

	@return	If the calling thread currently holds a fast mutex, this function
			returns a pointer to it; otherwise it returns NULL.
	@pre	Call in thread context.
*/
EXPORT_C NFastMutex* NKern::HeldFastMutex()
	{
	NThreadBase* t = NKern::CurrentThread();
	NFastMutex* m = (NFastMutex*)(TLinAddr(t->iHeldFastMutex)&~3);
	return (m && m->HeldByCurrentThread()) ? m : 0;
	}

	
#ifndef __FAST_MUTEX_MACHINE_CODED__
/** Acquires a fast mutex.

    This will block until the mutex is available, and causes
	the thread to enter an implicit critical section until the mutex is released.

	@param aMutex The fast mutex to acquire.
	
	@post The calling thread holds the mutex.
	
	@see NFastMutex::Wait()
	@see NKern::FMSignal()

	@pre No fast mutex can be held.
	@pre Call in a thread context.
	@pre Kernel must be unlocked
	@pre interrupts enabled

*/
EXPORT_C void NKern::FMWait(NFastMutex* aMutex)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NFMW %M", aMutex));
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"NKern::FMWait");
	NThreadBase* pC = NKern::CurrentThread();

	// If the reschedule IPI from an external suspend or kill occurs after this
	// point the initiating CPU must observe the write to iHeldFastMutex before
	// the cas operation.
	pC->iHeldFastMutex = aMutex;	// kill/suspend after this point should set mutex contention flag
	NThreadBase* expect = 0;
	if (__e32_atomic_cas_acq_ptr(&aMutex->iHoldingThread, &expect, pC))
		{
		// mutex was free and we have just claimed it - simple
#ifdef BTRACE_FAST_MUTEX
		BTraceContext4(BTrace::EFastMutex, BTrace::EFastMutexWait, aMutex);
#endif
		return;
		}

	// care required if suspend/kill here

	// there is contention so do it the hard way
	NKern::Lock();
	aMutex->DoWaitL();
	NKern::Unlock();
	}


/** Releases a previously acquired fast mutex.
	
	@param aMutex The fast mutex to release.
	
	@pre The calling thread holds the mutex.
	
	@see NFastMutex::Signal()
	@see NKern::FMWait()
*/
EXPORT_C void NKern::FMSignal(NFastMutex* aMutex)
	{
	NThreadBase* pC = NKern::CurrentThread();
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NFMS %M", aMutex));
#ifdef BTRACE_FAST_MUTEX
	BTraceContext4(BTrace::EFastMutex, BTrace::EFastMutexSignal, aMutex);
#endif
	((volatile TUint32&)pC->iHeldFastMutex) |= 1;	// flag to indicate about to release mutex

	if (__e32_atomic_cas_rel_ptr(&aMutex->iHoldingThread, &pC, 0))
		{
		// no-one else was waiting for the mutex and we have just released it

		// tricky if suspend/kill here
		// suspend/kill should check flag set above and aMutex->iHoldingThread
		// if bit 0 of iHeldFastMutex set and iHoldingThread==pC then set iHeldFastMutex=0 and proceed

		// If the reschedule IPI from an external suspend or kill occurs after this
		// point the initiating CPU must observe the write to iHeldFastMutex after
		// the cas operation.
		pC->iHeldFastMutex = 0;
		return;
		}

	// there was contention so do it the hard way
	NKern::Lock();
	aMutex->DoSignalL();
	NKern::Unlock();
	}

/** Acquires the System Lock.

    This will block until the mutex is available, and causes
	the thread to enter an implicit critical section until the mutex is released.

	@post System lock is held.

	@see NKern::UnlockSystem()
	@see NKern::FMWait()

	@pre No fast mutex can be held.
	@pre Call in a thread context.
	@pre Kernel must be unlocked
	@pre interrupts enabled

*/
EXPORT_C void NKern::LockSystem()
	{
	NKern::FMWait(&TheScheduler.iLock);
	}


/** Releases the System Lock.

	@pre System lock must be held.

	@see NKern::LockSystem()
	@see NKern::FMSignal()
*/
EXPORT_C void NKern::UnlockSystem()
	{
	NKern::FMSignal(&TheScheduler.iLock);
	}


/** Temporarily releases a fast mutex if there is contention.

    If there is another thread attempting to acquire the mutex, the calling
	thread releases the mutex and then acquires it again.
	
	This is more efficient than the equivalent code:
	
	@code
	NKern::FMSignal();
	NKern::FMWait();
	@endcode

	@return	TRUE if the mutex was relinquished, FALSE if not.

	@pre	The mutex must be held.

	@post	The mutex is held.
*/
EXPORT_C TBool NKern::FMFlash(NFastMutex* aM)
	{
	NThreadBase* pC = NKern::CurrentThread();
	__ASSERT_WITH_MESSAGE_DEBUG(aM->HeldByCurrentThread(),"The calling thread holds the mutex","NKern::FMFlash");
	TBool w = (pC->iMutexPri >= pC->iBasePri);	// a thread of greater or equal priority is waiting
	if (w)
		{
		NKern::Lock();
		aM->Signal();
		NKern::PreemptionPoint();
		aM->Wait();
		NKern::Unlock();
		}
#ifdef BTRACE_FAST_MUTEX
	else
		{
		BTraceContext4(BTrace::EFastMutex, BTrace::EFastMutexFlash, aM);
		}
#endif
	return w;
	}


/** Temporarily releases the System Lock if there is contention.

    If there
	is another thread attempting to acquire the System lock, the calling
	thread releases the mutex and then acquires it again.
	
	This is more efficient than the equivalent code:
	
	@code
	NKern::UnlockSystem();
	NKern::LockSystem();
	@endcode

	Note that this can only allow higher priority threads to use the System
	lock as lower priority cannot cause contention on a fast mutex.

	@return	TRUE if the system lock was relinquished, FALSE if not.

	@pre	System lock must be held.

	@post	System lock is held.

	@see NKern::LockSystem()
	@see NKern::UnlockSystem()
*/
EXPORT_C TBool NKern::FlashSystem()
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED,"NKern::FlashSystem");
	return NKern::FMFlash(&TheScheduler.iLock);
	}
#endif

/******************************************************************************
 * Fast semaphore
 ******************************************************************************/

/** Sets the owner of a fast semaphore.

	@param aThread The thread to own this semaphore. If aThread==0, then the
					owner is set to the current thread.

	@pre Kernel must be locked.
	@pre If changing ownership form one thread to another, the there must be no
		 pending signals or waits.
	@pre Call either in a thread or an IDFC context.
	
	@post Kernel is locked.
*/
EXPORT_C void NFastSemaphore::SetOwner(NThreadBase* aThread)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR,"NFastSemaphore::SetOwner");		
	if (!aThread)
		aThread = NCurrentThreadL();
	if (iOwningThread && iOwningThread!=aThread)
		{
		__NK_ASSERT_ALWAYS(!iCount);	// Can't change owner if iCount!=0
		}
	iOwningThread = aThread;
	}


#ifndef __FAST_SEM_MACHINE_CODED__
/** Waits on a fast semaphore.

    Decrements the signal count for the semaphore and
	removes the calling thread from the ready-list if the semaphore becomes
	unsignalled. Only the thread that owns a fast semaphore can wait on it.
	
	Note that this function does not block, it merely updates the NThread state,
	rescheduling will only occur when the kernel is unlocked. Generally threads
	would use NKern::FSWait() which manipulates the kernel lock for you.

	@pre The calling thread must own the semaphore.
	@pre No fast mutex can be held.
	@pre Kernel must be locked.
	
	@post Kernel is locked.
	
	@see NFastSemaphore::Signal()
	@see NKern::FSWait()
	@see NKern::Unlock()
 */
EXPORT_C void NFastSemaphore::Wait()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NO_FAST_MUTEX,"NFastSemaphore::Wait");
	NThreadBase* pC = NCurrentThreadL();
	__ASSERT_WITH_MESSAGE_ALWAYS(pC==iOwningThread,"The calling thread must own the semaphore","NFastSemaphore::Wait");
	pC->iWaitState.SetUpWait(NThreadBase::EWaitFastSemaphore, 0, this);
	if (Dec(pC))						// full barrier
		pC->iWaitState.CancelWait();	// don't have to wait
	else
		RescheduleNeeded();				// have to wait
	}


/** Signals a fast semaphore.

    Increments the signal count of a fast semaphore by
	one and releases any waiting thread if the semphore becomes signalled.
	
	Note that a reschedule will not occur before this function returns, this will
	only take place when the kernel is unlocked. Generally threads
	would use NKern::FSSignal() which manipulates the kernel lock for you.
	
	@pre Kernel must be locked.
	@pre Call either in a thread or an IDFC context.
	
	@post Kernel is locked.
	
	@see NFastSemaphore::Wait()
	@see NKern::FSSignal()
	@see NKern::Unlock()
 */
EXPORT_C void NFastSemaphore::Signal()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR,"NFastSemaphore::Signal");			
	NThreadBase* t = Inc(1);	// release semantics
	if (t)
		{
		t->AcqSLock();
		t->iWaitState.UnBlockT(NThreadBase::EWaitFastSemaphore, this, KErrNone);
		t->RelSLock();
		}
	}


/** Signals a fast semaphore multiple times.

	@pre Kernel must be locked.
	@pre Call either in a thread or an IDFC context.
	
	@post Kernel is locked.

	@internalComponent	
 */
EXPORT_C void NFastSemaphore::SignalN(TInt aCount)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR,"NFastSemaphore::SignalN");			
	__NK_ASSERT_DEBUG(aCount>=0);
	if (aCount > 0)
		{
		NThreadBase* t = Inc(aCount);
		if (t)
			{
			t->AcqSLock();
			t->iWaitState.UnBlockT(NThreadBase::EWaitFastSemaphore, this, KErrNone);
			t->RelSLock();
			}
		}
	}


/** Cancels a wait on a fast semaphore.

	@pre Kernel must be locked.
	@pre Call either in a thread or an IDFC context.
	
	@post Kernel is locked.

	@internalComponent	
 */
void NFastSemaphore::WaitCancel()
	{
	Inc(1);
	}


/** Waits for a signal on the current thread's I/O semaphore.

	@pre No fast mutex can be held.
	@pre Call in a thread context.
	@pre Kernel must be unlocked
	@pre interrupts enabled
 */
EXPORT_C void NKern::WaitForAnyRequest()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"NKern::WaitForAnyRequest");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("WfAR"));
	NThreadBase* t = NKern::LockC();
	NFastSemaphore* s = &t->iRequestSemaphore;
	t->iWaitState.SetUpWait(NThreadBase::EWaitFastSemaphore, 0, s);
	if (s->Dec(t))					// fully ordered semantics
		t->iWaitState.CancelWait();	// don't have to wait
	else
		RescheduleNeeded();			// have to wait
	NKern::Unlock();
	}
#endif


/** Resets a fast semaphore.

	@pre Kernel must be locked.
	@pre Call either in a thread or an IDFC context.
	
	@post Kernel is locked.

	@internalComponent	
 */
EXPORT_C void NFastSemaphore::Reset()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR,"NFastSemaphore::Reset");
	NThreadBase* t = DoReset();
	if (t)
		{
		t->AcqSLock();
		t->iWaitState.UnBlockT(NThreadBase::EWaitFastSemaphore, this, KErrNone);
		t->RelSLock();
		}
	}


/** Sets the owner of a fast semaphore.

	@param aSem The semaphore to change ownership off.
	@param aThread The thread to own this semaphore. If aThread==0, then the
					owner is set to the current thread.

	@pre If changing ownership form one thread to another, the there must be no
		 pending signals or waits.
*/
EXPORT_C void NKern::FSSetOwner(NFastSemaphore* aSem,NThreadBase* aThread)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::FSSetOwner %m %T",aSem,aThread));
	NKern::Lock();
	aSem->SetOwner(aThread);
	NKern::Unlock();
	}

#ifndef __FAST_SEM_MACHINE_CODED__
/** Waits on a fast semaphore.

    Decrements the signal count for the semaphore
	and waits for a signal if the semaphore becomes unsignalled. Only the
	thread that owns a fast	semaphore can wait on it.

	@param aSem The semaphore to wait on.
	
	@pre The calling thread must own the semaphore.
	@pre No fast mutex can be held.
	
	@see NFastSemaphore::Wait()
*/
EXPORT_C void NKern::FSWait(NFastSemaphore* aSem)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NFSW %m",aSem));
	NKern::Lock();
	aSem->Wait();
	NKern::Unlock();
	}


/** Signals a fast semaphore.

    Increments the signal count of a fast semaphore
	by one and releases any	waiting thread if the semaphore becomes signalled.
	
	@param aSem The semaphore to signal.

	@see NKern::FSWait()

	@pre Interrupts must be enabled.
	@pre Do not call from an ISR
 */
EXPORT_C void NKern::FSSignal(NFastSemaphore* aSem)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR,"NKern::FSSignal(NFastSemaphore*)");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NFSS %m",aSem));
	NKern::Lock();
	aSem->Signal();
	NKern::Unlock();
	}


/** Signals a fast semaphore multiple times.

    Increments the signal count of a
	fast semaphore by aCount and releases any waiting thread if the semphore
	becomes signalled.
	
	@param aSem The semaphore to signal.
	@param aCount The number of times to signal the semaphore.

	@see NKern::FSWait()

	@pre Interrupts must be enabled.
	@pre Do not call from an ISR
 */
EXPORT_C void NKern::FSSignalN(NFastSemaphore* aSem, TInt aCount)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR,"NKern::FSSignalN(NFastSemaphore*, TInt)");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NFSSN %m %d",aSem,aCount));
	__NK_ASSERT_DEBUG(aCount>=0);
	if (aCount == 0)
		return;
	NKern::Lock();
	aSem->SignalN(aCount);
	NKern::Unlock();
	}


/** Signals the request semaphore of a nanothread.

	This function is intended to be used by the EPOC layer and personality
	layers.  Device drivers should use Kern::RequestComplete instead.

	@param aThread Nanothread to signal. Must be non NULL.

	@see Kern::RequestComplete()

	@pre Interrupts must be enabled.
	@pre Do not call from an ISR
 */
EXPORT_C void NKern::ThreadRequestSignal(NThread* aThread)
	{
	NKern::FSSignal(&aThread->iRequestSemaphore);
	}


/** Signals the request semaphore of a nanothread several times.

	This function is intended to be used by the EPOC layer and personality
	layers.  Device drivers should use Kern::RequestComplete instead.

	@param aThread Nanothread to signal.  If NULL, the current thread is signaled.
	@param aCount Number of times the request semaphore must be signaled.
	
	@pre aCount >= 0

	@see Kern::RequestComplete()
 */
EXPORT_C void NKern::ThreadRequestSignal(NThread* aThread, TInt aCount)
	{
	__ASSERT_WITH_MESSAGE_DEBUG(aCount >= 0,"aCount >= 0","NKern::ThreadRequestSignal");
	if (!aThread)
		aThread = (NThread*)NKern::CurrentThread();
	NKern::FSSignalN(&aThread->iRequestSemaphore, aCount);
	}
#endif



/** Atomically signals a fast semaphore and releases a fast mutex.

	Rescheduling only occurs after both synchronisation operations are complete.
	
	@param aSem The semaphore to signal.
	@param aMutex The mutex to release. If NULL, the System Lock is released

	@pre The calling thread must hold the mutex.
	
	@see NKern::FMSignal()
 */
EXPORT_C void NKern::FSSignal(NFastSemaphore* aSem, NFastMutex* aMutex)
	{
	if (!aMutex)
		aMutex=&TheScheduler.iLock;
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NFSS %m +FM %M",aSem,aMutex));
	NKern::Lock();
	aSem->Signal();
	aMutex->Signal();
	NKern::Unlock();
	}


/** Atomically signals a fast semaphore multiple times and releases a fast mutex.

	Rescheduling only occurs after both synchronisation operations are complete.
	
	@param aSem The semaphore to signal.
	@param aCount The number of times to signal the semaphore.
	@param aMutex The mutex to release. If NULL, the System Lock is released.

	@pre The calling thread must hold the mutex.
	
	@see NKern::FMSignal()
 */
EXPORT_C void NKern::FSSignalN(NFastSemaphore* aSem, TInt aCount, NFastMutex* aMutex)
	{
	if (!aMutex)
		aMutex=&TheScheduler.iLock;
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NFSSN %m %d + FM %M",aSem,aCount,aMutex));
	NKern::Lock();
	aSem->SignalN(aCount);
	aMutex->Signal();
	NKern::Unlock();
	}


/******************************************************************************
 * Thread
 ******************************************************************************/

void NThreadBase::DoCsFunctionT()
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T nDoCsFuncT %d",this,iCsFunction));
	TInt f=iCsFunction;
	if (f==0)
		return;
	if (f>0)
		{
		// suspend this thread f times
		iCsFunction = 0;
		iSuspendCount += f;
		iSuspended = 1;
		RescheduleNeeded();
		return;
		}
	if (f==ECSExitPending || f==ECSDivertPending)
		{
		// We need to exit now
		RelSLock();
		Exit();	// this won't return
		}
//	UnknownState(ELeaveCS,f);	// call into RTOS personality
	__NK_ASSERT_ALWAYS(0);
	}

TBool NThreadBase::DoSuspendOrKillT(TInt aCount, TSubScheduler* aS)
	{
	TBool result = TRUE;
	if (aCount>=0)
		{
		if (iSuspended)
			result = FALSE;
		iSuspendCount+=aCount;
		iSuspended = 1;
		if (!iCurrent)
			{
			if (aS)
				UnReadyT();
			else if (iReady)
				{
				NThreadGroup* g = (NThreadGroup*)iParent;
				g->iNThreadList.Remove(this);
				}
			}
		if (this == NCurrentThreadL())
			RescheduleNeeded();
		if (aS)
			aS->iReadyListLock.UnlockOnly();
		}
	else
		{
		iCsFunction = ECSDivertPending;
		iSuspendCount = 0;
		iSuspended = 0;

		// If thread is killed before first resumption, set iACount=1
		__e32_atomic_tau_ord8(&iACount, 1, 0, 1);
		if (aS)
			aS->iReadyListLock.UnlockOnly();
		DoReleaseT(KErrDied,0);
		if (!iReady && !iPauseCount)
			ReadyT(0);
		}
	return result;
	}

// If aCount>=0 suspend the thread aCount times
// If aCount<0 kill the thread
TBool NThreadBase::SuspendOrKill(TInt aCount)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T nSuspendOrKill %d", this, aCount));
	if (aCount==0 || i_NThread_Initial)
		return FALSE;
	TBool result = FALSE;
	TBool concurrent = FALSE;
	TSubScheduler* ss = 0;
	AcqSLock();
	NFastMutex* wfm = 0;
	if (iLinkedObj && iLinkedObjType==EWaitFastMutex)
		wfm = (NFastMutex*)iLinkedObj;
	if (iCsFunction<0)
		goto done2;	// if already exiting ignore suspend or kill
	if (wfm)
		{
		// if thread is waiting on a fast mutex, need to acquire mutex lock
		++iPauseCount;
		RelSLock();
		wfm->iMutexLock.LockOnly();
		AcqSLock();
		UnPauseT();
		}
	if (iReady && iParent->iReady)
		{
		ss = TheSubSchedulers + (iParent->iReady & EReadyCpuMask);
		ss->iReadyListLock.LockOnly();
		}
	concurrent = (iCurrent && this!=NCurrentThreadL());
	if (iWaitState.ThreadIsDead())				// already dead so suspension/kill is a no-op
		goto done;
	if (concurrent)
		{
		// thread is actually running on another CPU
		// interrupt that CPU and wait for it to enter interrupt mode
		// this allows a snapshot of the thread state to be observed
		// in this state, the thread cannot enter or leave a critical section
		send_resched_ipi_and_wait(iLastCpu);
		}
	if (iCsCount)
		{
suspend_or_kill_in_cs:
		__KTRACE_OPT(KNKERN,DEBUGPRINT("n Suspend %T (CSF %d) %d",this,iCsFunction,aCount));
		if (aCount>0)				// -ve means thread is about to exit
			iCsFunction+=aCount;	// so thread will suspend itself when it leaves the critical section
		else
			iCsFunction = ECSExitPending;
		goto done;
		}
	// iCsCount==0 and it can't become nonzero until we release the thread spin lock
	// (since threads may not set iCsCount to a nonzero value with the kernel lock held)
	// Make sure the thread isn't actually about to exit by itself
	if (iCsFunction<0)
		goto done;	// if already exiting ignore suspend or kill
	if (wfm)
		{
		wfm->iWaitQ.Remove(&iWaitLink);	// take thread off the wait/contend queue
		iWaitLink.iNext = 0;
		iLinkedObj = 0;
		iLinkedObjType = EWaitNone;
		result = DoSuspendOrKillT(aCount, ss);
		if (aCount>0)
			DoReleaseT(KErrGeneral, 0);	// thread isn't blocked any more, just suspended
		RelSLock();

		// May need to adjust holding thread's inherited priority.
		// May need to wake up next thread to take this one's place.
		NThreadBase* pH = (NThreadBase*)(TLinAddr(wfm->iHoldingThread) &~ 1);
		if (pH)
			pH->SetMutexPriority(wfm);
		else if (!pH && !wfm->iWaitQ.IsEmpty())
			{
			NThreadBase* pT = _LOFF(wfm->iWaitQ.First(), NThreadBase, iWaitLink);
			pT->AcqSLock();
			pT->iWaitState.UnBlockT(NThreadBase::EWaitFastMutex, wfm, KErrNone);
			pT->RelSLock();
			}
		wfm->iMutexLock.UnlockOnly();
		return result;
		}
	if (CheckFastMutexDefer())
		goto suspend_or_kill_in_cs;

	// thread not in critical section, so suspend it
	result = DoSuspendOrKillT(aCount, ss);
	goto done2;

done:
	if (wfm)
		wfm->iMutexLock.UnlockOnly();
	if (ss)
		ss->iReadyListLock.UnlockOnly();
done2:
	RelSLock();

	return result;
	}


/** Suspends a nanothread the specified number of times.
	
	For use by RTOS personality layers.
	Do not use this function directly on a Symbian OS thread.
	Since the kernel is locked on entry, any reschedule will be deferred until
	it is unlocked.
	The suspension will be deferred if the target thread is currently in a
	critical section; in this case the suspension will take effect when it exits
	the critical section.
	The thread's unknown state handler will be invoked with function ESuspend and
	parameter aCount if the current NState is not recognised and it is not in a
	critical section.

	@param	aCount = the number of times to suspend.
	@return	TRUE, if the suspension has taken immediate effect;
			FALSE, if the thread is in a critical section or is already suspended.
	
	@pre	Kernel must be locked.
	@pre	Call in a thread context.
	
	@post	Kernel is locked.
 */
EXPORT_C TBool NThreadBase::Suspend(TInt aCount)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR|MASK_NOT_IDFC,"NThreadBase::Suspend");
	__NK_ASSERT_ALWAYS(aCount>=0);

	// If thread is executing a critical section, we must defer the suspend

	return SuspendOrKill(aCount);
	}


TBool NThreadBase::Resume(TBool aForce)
	{
	TBool result = FALSE;
	AcqSLock();
	if (iWaitState.ThreadIsDead() || iCsFunction<0)		// already dead or dying so resume is a no-op
		goto done;

	if (iCsFunction>0)
		{
		if (aForce)
			iCsFunction = 0;
		else
			--iCsFunction;
		}
	else if (iSuspendCount)
		{
		if (aForce)
			iSuspendCount = 0;
		else
			--iSuspendCount;
		if (!iSuspendCount)
			{
			result = TRUE;
			iSuspended = 0;

			// On first resumption set iACount=1
			// From then on the thread must be killed before being deleted
			__e32_atomic_tau_ord8(&iACount, 1, 0, 1);
			if (!iPauseCount && !iReady && !iWaitState.iWtC.iWtStFlags)
				ReadyT(0);
			}
		}

done:
	RelSLock();
	return result;
	}

/** Resumes a nanothread, cancelling one suspension.
	
	For use by RTOS personality layers.
	Do not use this function directly on a Symbian OS thread.
	Since the kernel is locked on entry, any reschedule will be deferred until
	it is unlocked.
	If the target thread is currently in a critical section this will simply
	cancel one deferred suspension.
	The thread's unknown state handler will be invoked with function EResume if
	the current NState is not recognised and it is not in a	critical section.

	@return	TRUE, if the resumption has taken immediate effect;
			FALSE, if the thread is in a critical section or is still suspended.
	
	@pre	Kernel must be locked.
	@pre	Call either in a thread or an IDFC context.
	
	@post	Kernel must be locked.
 */
EXPORT_C TBool NThreadBase::Resume()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR|MASK_NOT_IDFC,"NThreadBase::Resume");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T nRsm",this));

	return Resume(FALSE);
	}


/** Resumes a nanothread, cancelling all outstanding suspensions.
	
	For use by RTOS personality layers.
	Do not use this function directly on a Symbian OS thread.
	Since the kernel is locked on entry, any reschedule will be deferred until
	it is unlocked.
	If the target thread is currently in a critical section this will simply
	cancel all deferred suspensions.
	The thread's unknown state handler will be invoked with function EForceResume
	if the current NState is not recognised and it is not in a	critical section.

	@return	TRUE, if the resumption has taken immediate effect;
			FALSE, if the thread is in a critical section.

	@pre	Kernel must be locked.
	@pre	Call either in a thread or an IDFC context.

	@post	Kernel is locked.
 */
EXPORT_C TBool NThreadBase::ForceResume()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR,"NThreadBase::ForceResume");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T nFRsm",this));

	return Resume(TRUE);
	}


void NThreadBase::DoReleaseT(TInt aReturnCode, TUint aMode)
	{
	TAny* wobj = 0;
	TUint32 b = iWaitState.ReleaseT(wobj, aReturnCode);	// cancels timer if necessary

	// if wait pending or no wait, done
	// if wait in effect and nothing else stopping it, make thread ready
	// cancel any outstanding wait on fast semaphore if abnormal release
	// FIXME: Potential problems with abnormal release of generic wait objects
	if (aReturnCode<0 && ((b>>8)&0xff)==NThreadBase::EWaitFastSemaphore && wobj)
		((NFastSemaphore*)wobj)->WaitCancel();

	if ((b & NThreadWaitState::EWtStWaitActive) && !iPauseCount && !iSuspended)
		ReadyT(aMode);
	}

/** Releases a waiting nanokernel thread.

	For use by RTOS personality layers.
	Do not use this function directly on a Symbian OS thread.
	This function should make the thread ready (provided it is not explicitly
	suspended) and cancel any wait timeout. It should also remove it from any
	wait queues.
	If aReturnCode is nonnegative it indicates normal completion of the wait.
	If aReturnCode is negative it indicates early/abnormal completion of the
	wait and so any wait object should be reverted as if the wait had never
	occurred (eg semaphore count should be incremented as this thread has not
	actually acquired the semaphore).
	The thread's unknown state handler will be invoked with function ERelease
	and parameter aReturnCode if the current NState is not recognised.
	
	@param aReturnCode	The reason code for release.

	@pre	Kernel must be locked.
	@pre	Call either in a thread or an IDFC context.
	
	@post	Kernel is locked.
 */
EXPORT_C void NThreadBase::Release(TInt aReturnCode, TUint aMode)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR,"NThreadBase::Release");		
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T nRel %d",this,aReturnCode));
	AcqSLock();
	DoReleaseT(aReturnCode, aMode);
	RelSLock();
	}


/** Signals a nanokernel thread's request semaphore.

	This can also be used on Symbian OS threads.
	
	@pre	Kernel must be locked.
	@pre	Call either in a thread or an IDFC context.
	
	@post	Kernel is locked.
 */
EXPORT_C void NThreadBase::RequestSignal()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR,"NThreadBase::RequestSignal");		
	iRequestSemaphore.Signal();
	}


void exit_sync_fn(TAny* aDfc)
	{
	((TDfc*)aDfc)->Enque();
	}

void NThreadBase::Exit()
	{
	// The current thread is exiting
	// Enter with kernel locked, don't return
	__NK_ASSERT_DEBUG(this==NCurrentThreadL());

	OnExit();

	TInt threadCS = iCsCount;
	TInt kernCS = SubScheduler().iKernLockCount;
	iCsCount = 1;
	AcqSLock();
	iCsFunction = ECSExitInProgress;
	NFastMutex* m = NKern::HeldFastMutex();
	iHeldFastMutex = 0;
	RelSLock();
	NKern::Unlock();
	__KTRACE_OPT(KSCHED,DEBUGPRINT("Exit %T %u",this,NTickCount()));
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T nExit, CSC %d HeldFM %M KernCS %d",this,threadCS,iHeldFastMutex,kernCS));
	if (kernCS!=1)
		FAULT();
	if (m)
		FAULT();
	if (threadCS)
		FAULT();
	TDfc* pD = NULL;
	NThreadExitHandler xh = iHandlers->iExitHandler;
	if (xh)
		pD = (*xh)((NThread*)this);		// call exit handler

	// if CPU freeze still active, remove it
	NKern::EndFreezeCpu(0);

	// detach any tied events
	DetachTiedEvents();

	NKern::LeaveGroup();	// detach from group if exit handler didn't do it

	NKern::Lock();
#ifdef BTRACE_THREAD_IDENTIFICATION
	BTrace4(BTrace::EThreadIdentification,BTrace::ENanoThreadDestroy,this);
#endif
	__NK_ASSERT_ALWAYS(iCsFunction == ECSExitInProgress);
	iWaitState.SetDead(pD);	// doesn't return
	FAULT();
	}

/** Kills a nanokernel thread.

	For use by RTOS personality layers.
	Do not use this function directly on a Symbian OS thread.

	When acting on the calling thread, causes the calling thread to exit.

	When acting on another thread, causes that thread to exit unless it is
	currently in a critical section. In this case the thread is marked as
	"exit pending" and will exit as soon as it leaves the critical section.

	In either case the exiting thread first invokes its exit handler (if it
	exists). The handler runs with preemption enabled and with the thread in a
	critical section so that it may not be suspended or killed again. The
	handler may return a pointer to a TDfc, which will be enqueued just before
	the thread finally terminates (after the kernel has been relocked). This DFC
	will therefore execute once the NThread has been safely removed from the
	scheduler and is intended to be used to cleanup the NThread object and any
	associated personality layer resources.
	
	@pre	Kernel must be locked.
	@pre	Call in a thread context.
	@pre	If acting on calling thread, calling thread must not be in a
			critical section; if it is the kernel will fault. Also, the kernel
			must be locked exactly once (iKernCSLocked = 1).
	
	@post	Kernel is locked, if not acting on calling thread.
	@post	Does not return if it acts on the calling thread.
 */
EXPORT_C void NThreadBase::Kill()
	{
	// Kill a thread
	// Enter with kernel locked
	// Exit with kernel locked if not current thread, otherwise does not return
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED_ONCE|MASK_NOT_IDFC|MASK_NOT_ISR,"NThreadBase::Kill");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T nKill",this));
	OnKill(); // platform-specific hook
	NThreadBase* pC = NCurrentThreadL();
	if (this==pC)
		{
		if (iCsFunction==ECSExitInProgress)
			FAULT();
		Exit();				// this will not return
		}
	SuspendOrKill(-1);
	}


/** Change the CPU affinity of a thread

	@pre	Kernel must be locked.
	@pre	Call in a thread context.

	@param	The number of the CPU to which this thread should be locked, or
			KCpuAny if it should be able to run on any CPU.
	@return The previous affinity mask.
*/
TUint32 NSchedulable::SetCpuAffinityT(TUint32 aAffinity)
	{
	// check aAffinity is valid
	NThreadBase* t = 0;
	NThreadGroup* g = 0;
	NSchedulable* p = iParent;
	if (!p)
		g = (NThreadGroup*)this, p=g;
	else
		t = (NThreadBase*)this;
	if (iParent && iParent!=this)
		g = (NThreadGroup*)iParent;
	TUint32 old_aff = p->iCpuAffinity;
	TBool make_ready = FALSE;
	TSubScheduler* ss0 = &SubScheduler();
	TSubScheduler* ss = 0;
#ifdef KNKERN
	if (iParent)
		{
		__KTRACE_OPT(KNKERN,DEBUGPRINT("%T nSetCpu %08x->%08x, F:%d R:%02x PR:%02x",this,iParent->iCpuAffinity,aAffinity,iParent->iFreezeCpu,iReady,iParent->iReady));
		}
	else
		{
		__KTRACE_OPT(KNKERN,DEBUGPRINT("%G nSetCpu %08x->%08x, F:%d R:%02x",this,iCpuAffinity,aAffinity,iFreezeCpu,iReady));
		}
#endif
	if (t && t->i_NThread_Initial)
		goto done;	// can't change affinity of initial thread
	if (aAffinity == NTHREADBASE_CPU_AFFINITY_MASK)
		{
		p->iTransientCpu = 0;
		}
	else if ( (aAffinity & (KCpuAffinityPref|NTHREADBASE_CPU_AFFINITY_MASK)) == KCpuAffinityPref)
		{
		p->iTransientCpu = 0;
		p->iPreferredCpu = TUint8((aAffinity & (EReadyCpuMask|EReadyCpuSticky)) | EReadyOffset);
		}
	else if ( (aAffinity & (KCpuAffinityTransient|KCpuAffinityPref|NTHREADBASE_CPU_AFFINITY_MASK)) == KCpuAffinityTransient)
		{
		p->iTransientCpu = TUint8(aAffinity & EReadyCpuMask) | EReadyOffset;
		}
	else
		p->iCpuAffinity = NSchedulable::PreprocessCpuAffinity(aAffinity);		// set new affinity, might not take effect yet
	if (!p->iReady)
		goto done;	// thread/group not currently on a ready list so can just change affinity

	// Check if the thread needs to migrate or can stay where it is
	if (!p->ShouldMigrate(p->iReady & EReadyCpuMask))
		goto done;	// don't need to move thread, so just change affinity
	ss = TheSubSchedulers + (p->iReady & EReadyCpuMask);
	ss->iReadyListLock.LockOnly();
	if (p->iCurrent)
		{
		p->iCpuChange = TRUE;			// mark CPU change pending
		if (ss == ss0)
			RescheduleNeeded();
		else
			// kick other CPU now so migration happens before acquisition of fast mutex
			send_resched_ipi_and_wait(p->iReady & EReadyCpuMask);
		}
	else
		{
		// Note: Need to know here if any thread in group would return TRUE from CheckFastMutexDefer()
		// This is handled by the scheduler - when a thread belonging to a group is context switched
		// out while holding a fast mutex its iFastMutexDefer is set to 1 and the group's iFreezeCpu
		// is incremented.
		if (p->iFreezeCpu || (iParent==this && t->CheckFastMutexDefer()))
			p->iCpuChange = TRUE;	// CPU frozen or fast mutex held so just mark deferred CPU migration
		else
			{
			ss->SSRemoveEntry(p);
			p->iReady = 0;
			make_ready = TRUE;
			}
		}
	ss->iReadyListLock.UnlockOnly();
	if (make_ready)
		p->ReadyT(0);
done:
	return old_aff;
	}

/** Force the current thread onto a particular CPU

	@pre	Kernel must not be locked.
	@pre	Call in a thread context.
	@pre	Current thread must not be in a group
	@pre	Current thread must not hold a fast mutex
	@pre	Current thread must have an active CPU freeze
	@pre	Current thread must not be an initial thread

	@param	The number of the CPU to which this thread should be moved
*/
void NKern::JumpTo(TInt aCpu)
	{
	// check aAffinity is valid
	NThreadBase* t = NKern::CurrentThread();
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T NJumpTo %d", t, aCpu));
	if (NKern::HeldFastMutex())
		__crash();
	t->LAcqSLock();
	if (t->iParent!=t)
		__crash();
	if (!t->iFreezeCpu)
		__crash();
	if (t->i_NThread_Initial)
		__crash();
	if (TUint(aCpu) >= (TUint)NKern::NumberOfCpus())
		__crash();
	TUint8 fc = (TUint8)(aCpu | NSchedulable::EReadyOffset);
	if (t->iCurrent != fc)
		{
		t->iForcedCpu = fc;
		t->iCpuChange = TRUE;
		RescheduleNeeded();
		}
	t->RelSLockU();		// reschedules and jumps to new CPU
	}

TBool NSchedulable::ShouldMigrate(TInt aCpu)
	{
	// Check if the thread's current CPU is compatible with the new affinity
	TUint32 active = TheScheduler.iThreadAcceptCpus;

	// If it can't stay where it is, migrate
	if (!CheckCpuAgainstAffinity(aCpu, iCpuAffinity, active))
		return TRUE;

	TInt cpu = iTransientCpu ? iTransientCpu : iPreferredCpu;

	// No preferred or transient CPU, so can stay where it is
	if (!cpu)
		return FALSE;

	// If thread isn't on preferred CPU but could be, migrate
	cpu &= EReadyCpuMask;
	if (cpu!=aCpu && CheckCpuAgainstAffinity(cpu, iCpuAffinity, active))
		return TRUE;
	return FALSE;
	}


/******************************************************************************
 * Thread wait state
 ******************************************************************************/
#ifndef __NTHREAD_WAITSTATE_MACHINE_CODED__
void NThreadWaitState::SetUpWait(TUint aType, TUint aFlags, TAny* aWaitObj)
	{
	SetUpWait(aType, aFlags, aWaitObj, 0);
	}

void NThreadWaitState::SetUpWait(TUint aType, TUint aFlags, TAny* aWaitObj, TUint32 aTimeout)
	{
	aFlags &= EWtStObstructed;
	aFlags |= EWtStWaitPending;
	aType &= 0xff;
	TUint64 ws64 = (TUint32)aWaitObj;
	ws64 <<= 32;
	ws64 |= ((aType<<8)|aFlags);
	TUint64 oldws64 = __e32_atomic_swp_rlx64(&iWtSt64, ws64);
	if (I64LOW(oldws64)!=0)
		__crash();	// ??we were already waiting for something else??
	iTimer.iTriggerTime = aTimeout;
	}

void NThreadWaitState::CancelWait()
	{
	TUint64 oldws64 = __e32_atomic_swp_rlx64(&iWtSt64, 0);
	if (oldws64 & (EWtStDead|EWtStWaitActive))
		__crash();
	}

TInt NThreadWaitState::DoWait()
	{
	TUint64 oldws64 = iWtSt64;
	TUint64 ws64;
	TUint32 timeout = iTimer.iTriggerTime;
	TUint32 set = timeout ? (EWtStWaitActive|EWtStTimeout) : EWtStWaitActive;
	do	{
		TUint32 ws32 = I64LOW(oldws64);
		if (ws32 & EWtStDead)
			return KErrDied;
		if (!(ws32 & EWtStWaitPending))
			return KErrGeneral;
		ws64 = oldws64;
		ws64 &= ~TUint64(EWtStWaitPending);
		ws64 |= TUint64(set);
		} while(!__e32_atomic_cas_rlx64(&iWtSt64, &oldws64, ws64));
	if (timeout)
		{
		if (iTimer.OneShot(timeout, TRUE)!=KErrNone)
			__crash();
		++iTimer.iNTimerSpare1;
		}
	return TUint32(oldws64)>>8;
	}

TInt NThreadWaitState::UnBlockT(TUint aType, TAny* aWaitObj, TInt aReturnValue)
	{
	TUint64 exp = TUint32(aWaitObj);
	exp <<= 32;
	exp |= (aType<<8);
	TUint64 oldws64 = iWtSt64;
	TUint64 ws64;
	do	{
		if ((oldws64 ^ exp) < TUint64(EWtStDead))
			ws64 = TUint64(TUint32(aReturnValue))<<32;
		else
			ws64 = oldws64;
		} while(!__e32_atomic_cas_rel64(&iWtSt64, &oldws64, ws64));
	if ((oldws64 ^ exp) >= TUint64(EWtStDead))
		return KErrGeneral;	// not unblocked - no matching wait
	if (oldws64 & EWtStTimeout)
		CancelTimerT();
	if (oldws64 & EWtStWaitActive)
		{
		NThreadBase* t = Thread();
		if (!t->iPauseCount && !t->iSuspended)
			t->ReadyT(oldws64 & EWtStObstructed);
		}
	return KErrNone;
	}

TUint32 NThreadWaitState::ReleaseT(TAny*& aWaitObj, TInt aReturnValue)
	{
	TUint64 leave = EWtStDead;
	TUint64 set = TUint64(TUint32(aReturnValue))<<32;
	TUint64 ws64 = __e32_atomic_axo_ord64(&iWtSt64, leave, set);
	aWaitObj = (TAny*)I64HIGH(ws64);
	TUint32 ws32 = I64LOW(ws64);
	if (ws32 & EWtStTimeout)
		CancelTimerT();
	return ws32;
	}
#endif

void NThreadWaitState::SetDead(TDfc* aKillDfc)
	{
	TDfc syncDfc(&exit_sync_fn, aKillDfc, TheTimerQ.iDfc.iDfcQ, 0);
	NThreadBase* t = Thread();
	t->AcqSLock();
	iWtC.iWtStFlags = NThreadWaitState::EWtStDead;
	iWtC.iWtObjType = NThreadBase::EWaitNone;
	CancelTimerT();
	if (aKillDfc && iTimer.iNTimerSpare1)
		{
		// There is an outstanding timer expiry handler still running
		// so we must synchronise with DfcThread1.
		// Add a priority 0 DFC to DfcThread1 so this thread's exit DFC can
		// only run after the timer expiry handler has completed.
		aKillDfc = &syncDfc;
		}
	iWtC.iKillDfc = aKillDfc;
	RescheduleNeeded();
	t->RelSLock();
	NKern::Unlock();	// this won't return
	}

void NThreadWaitState::CancelTimerT()
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T nCancelTimerT ",Thread()));
	if (iTimer.Cancel())
		--iTimer.iNTimerSpare1;
	else
		{
		// Potential race condition - timer must have completed but expiry
		// handler has not yet run. Signal to the handler that it should do
		// nothing by flipping the bottom bit of iTimer.iPtr
		// This condition cannot possibly recur until the expiry handler has
		// run since all expiry handlers run in DfcThread1.
		volatile TLinAddr& x = *(volatile TLinAddr*)&iTimer.iPtr;
		x ^= 1;
		}
	}

// Timeout handler, called in DfcThread1
// NOTE: aPtr is sampled with the timer queue locked, so if Cancel() on the timer fails
// and iTimer.iPtr is then changed, aPtr here will differ from iTimer.iPtr.
// This fact is used here to detect expiry of cancelled timers.
void NThreadWaitState::TimerExpired(TAny* aPtr)
	{
	TLinAddr cookie = (TLinAddr)aPtr;
	NThreadWaitState* pW = (NThreadWaitState*)(cookie &~ 3);
	NThread* pT = (NThread*)pW->Thread();
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T nTmExp",pT));
	NThreadTimeoutHandler th = pT->iHandlers->iTimeoutHandler;
	pT->LAcqSLock();
	TUint flags = pW->iWtSt32[0];
	if (!(flags & EWtStWaitActive) || ((flags>>8)&0xff)!=NThreadBase::EWaitBlocked)
		th = 0;
	if (th)
		{
		// Use higher level timeout handler
		pT->RelSLockU();
		(*th)(pT, NThreadBase::ETimeoutPreamble);
		TInt param = NThreadBase::ETimeoutPostamble;
		pT->LAcqSLock();
		TLinAddr current_cookie = *(volatile TLinAddr*)&pW->iTimer.iPtr;
		if ((cookie ^ current_cookie) & 1)
			{
			// The timer was cancelled just after expiring but before this function
			// managed to acquire the thread spin lock, so it's spurious
			param = NThreadBase::ETimeoutSpurious;
			}
		pT->RelSLockU();
		(*th)(pT, param);
		pT->LAcqSLock();
		--pW->iTimer.iNTimerSpare1;	// note timer has expired
		pT->RelSLockU();
		return;
		}
	TLinAddr current_cookie = *(volatile TLinAddr*)&pW->iTimer.iPtr;
	if ((cookie ^ current_cookie) & 1)
		// The timer was cancelled just after expiring but before this function
		// managed to acquire the thread spin lock, so just return without doing anything.
		goto done;
	pT->DoReleaseT(KErrTimedOut,0);
done:
	pT->RelSLockU();
	}



/******************************************************************************
 * NKern:: static functions
 ******************************************************************************/

/** Suspends the execution of a thread.

	This function is intended to be used by the EPOC layer and personality layers.
	Do not use this function directly on a Symbian OS thread - use Kern::ThreadSuspend().

    If the thread is in a critical section or holds a fast mutex, the suspension will
    be deferred until the thread leaves the critical section or signals the fast mutex.
    Otherwise the thread will be suspended with immediate effect. If the thread it's
    running, the execution of the thread will be suspended and a reschedule will occur.

    @param aThread Thread to be suspended.
    @param aCount  Number of times to suspend this thread.
    
    @return TRUE, if the thread had changed the state from non-suspended to suspended;
	        FALSE, otherwise.
	     
	@see Kern::ThreadSuspend()
*/
EXPORT_C TBool NKern::ThreadSuspend(NThread* aThread, TInt aCount)
	{	
	NKern::Lock();
	TBool r=aThread->Suspend(aCount);
	NKern::Unlock();
	return r;
	}


/** Resumes the execution of a thread.

	This function is intended to be used by the EPOC layer and personality layers.
	Do not use this function directly on a Symbian OS thread - use Kern::ThreadResume().

    This function resumes the thread once. If the thread was suspended more than once
    the thread will remain suspended.
    If the thread is in a critical section, this function will decrease the number of
    deferred suspensions.

    @param aThread Thread to be resumed.
    
    @return TRUE, if the thread had changed the state from suspended to non-suspended;
            FALSE, otherwise.
            
	@see Kern::ThreadResume()
*/
EXPORT_C TBool NKern::ThreadResume(NThread* aThread)
	{	
	NKern::Lock();
	TBool r=aThread->Resume();
	NKern::Unlock();
	return r;
	}


/** Resumes the execution of a thread and signals a mutex.

	This function is intended to be used by the EPOC layer and personality layers.
	Do not use this function directly on a Symbian OS thread - use Kern::ThreadResume().

    This function resumes the thread once. If the thread was suspended more than once
    the thread will remain suspended.
    If the thread is in a critical section, this function will decrease the number of
    deferred suspensions.

    @param aThread Thread to be resumed.
    @param aMutex Mutex to be signalled. If NULL, the scheduler's mutex will be signalled.

    @return TRUE, if the thread had changed the state from suspended to non-suspended;
            FALSE, otherwise.
           
	@see Kern::ThreadResume()
*/
EXPORT_C TBool NKern::ThreadResume(NThread* aThread, NFastMutex* aMutex)
	{
	if (!aMutex)
		aMutex=&TheScheduler.iLock;
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T NRsm + FM %M",aThread,aMutex));
	NKern::Lock();
	TBool r=aThread->Resume();
	aMutex->Signal();
	NKern::Unlock();
	return r;
	}


/** Forces the execution of a thread to be resumed.

	This function is intended to be used by the EPOC layer and personality layers.
	Do not use this function directly on a Symbian OS thread - use Kern::ThreadResume().

    This function cancels all suspensions on a thread.

    @param aThread Thread to be resumed.
    
    @return TRUE, if the thread had changed the state from suspended to non-suspended;
            FALSE, otherwise.
            
	@see Kern::ThreadResume()
*/
EXPORT_C TBool NKern::ThreadForceResume(NThread* aThread)
	{	
	NKern::Lock();
	TBool r=aThread->ForceResume();
	NKern::Unlock();
	return r;
	}


/** Forces the execution of a thread to be resumed and signals a mutex.

	This function is intended to be used by the EPOC layer and personality layers.
	Do not use this function directly on a Symbian OS thread - use Kern::ThreadResume().

    This function cancels all suspensions on a thread.

    @param aThread Thread to be resumed.
    @param aMutex Mutex to be signalled. If NULL, the scheduler's mutex will be signalled.
    
    @return TRUE, if the thread had changed the state from suspended to non-suspended;
            FALSE, otherwise.
            
    @see Kern::ThreadResume()
*/
EXPORT_C TBool NKern::ThreadForceResume(NThread* aThread, NFastMutex* aMutex)
	{
	if (!aMutex)
		aMutex=&TheScheduler.iLock;
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T NFRsm + FM %M",aThread,aMutex));
	NKern::Lock();
	TBool r=aThread->ForceResume();
	aMutex->Signal();
	NKern::Unlock();
	return r;
	}


/** Awakens a nanothread.

	This function is used to implement synchronisation primitives in the EPOC
	kernel (e.g. DMutex and DSemaphore) and in personality layers.  It is not
	intended to be used directly by device drivers.

	If the nanothread is waiting on a fast semaphore, waiting for a DFC, or is
	blocked in a call to NKern::Block, it is awakened and put back on the ready
	list.  Otherwise, the thread state is unchanged.  In particular, nothing
	happens if the nanothread has been explicitly suspended.

	@param aThread Thread to release.
	@param aReturnValue Value returned by NKern::Block if the thread was blocked.

	@see NKern::Block()

	@pre Interrupts must be enabled.
	@pre Do not call from an ISR
 */
EXPORT_C void NKern::ThreadRelease(NThread* aThread, TInt aReturnValue)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR,"NKern::ThreadRelease(NThread*, TInt)");
	NKern::Lock();
	aThread->Release(aReturnValue,0);
	NKern::Unlock();
	}


/** Atomically awakens a nanothread and signals a fast mutex.

	This function is used to implement synchronisation primitives in the EPOC
	kernel (e.g. DMutex and DSemaphore) and in personality layers.  It is not
	intended to be used directly by device drivers.

	@param aThread Thread to release.
	@param aReturnValue Value returned by NKern::Block if the thread was blocked.
	@param aMutex Fast mutex to signal. If NULL, the system lock is signalled.

	@see NKern::ThreadRelease(NThread*, TInt)
	@see NKern::Block()

	@pre	Call in a thread context.
	@pre	Interrupts must be enabled.
	@pre	Kernel must be unlocked.
	@pre	Specified mutex must be held
 */
EXPORT_C void NKern::ThreadRelease(NThread* aThread, TInt aReturnValue, NFastMutex* aMutex)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::ThreadRelease(NThread*,TInt,NFastMutex*)");
	if (!aMutex)
		aMutex=&TheScheduler.iLock;
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T NRel ret %d + FM %M",aThread,aReturnValue,aMutex));
	NKern::Lock();
	aThread->Release(aReturnValue,0);
	aMutex->Signal();
	NKern::Unlock();
	}


/** Changes the priority of a thread.

	This function is intended to be used by the EPOC layer and personality layers.
	Do not use this function directly on a Symbian OS thread - use Kern::ThreadSetPriority().

    @param aThread Thread to receive the new priority.
    @param aPriority New priority for aThread.
    
	@see Kern::SetThreadPriority()
*/
EXPORT_C void NKern::ThreadSetPriority(NThread* aThread, TInt aPriority)
	{
	NKern::Lock();
	aThread->SetPriority(aPriority);
	NKern::Unlock();
	}


/** Changes the priority of a thread and signals a mutex.

	This function is intended to be used by the EPOC layer and personality layers.
	Do not use this function directly on a Symbian OS thread - use Kern::ThreadSetPriority().

    @param aThread Thread to receive the new priority.
    @param aPriority New priority for aThread.
    @param aMutex Mutex to be signalled. If NULL, the scheduler's mutex will be signalled.
        
	@see Kern::SetThreadPriority()
*/
EXPORT_C void NKern::ThreadSetPriority(NThread* aThread, TInt aPriority, NFastMutex* aMutex)
	{	
	if (!aMutex)
		aMutex=&TheScheduler.iLock;
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T NSPri->%d + FM %M",aThread,aPriority,aMutex));
	NKern::Lock();
	aThread->SetPriority(aPriority);
	aMutex->Signal();
	NKern::Unlock();
	}


/** Changes the nominal priority of a thread.

	This function is intended to be used by the EPOC layer and personality layers.
	Do not use this function directly on a Symbian OS thread - use Kern::ThreadSetPriority().

    @param aThread Thread to receive the new priority.
    @param aPriority New inherited priority for aThread.
    
	@see Kern::SetThreadPriority()
*/
void NKern::ThreadSetNominalPriority(NThread* aThread, TInt aPriority)
	{
	NKern::Lock();
	aThread->SetNominalPriority(aPriority);
	NKern::Unlock();
	}


/** Atomically signals the request semaphore of a nanothread and a fast mutex.

	This function is intended to be used by the EPOC layer and personality
	layers.  Device drivers should use Kern::RequestComplete instead.

	@param aThread Nanothread to signal.  Must be non NULL.
	@param aMutex Fast mutex to signal.  If NULL, the system lock is signaled.

	@see Kern::RequestComplete()

	@pre	Call in a thread context.
	@pre	Interrupts must be enabled.
	@pre	Kernel must be unlocked.
	@pre	Specified mutex must be held
 */
EXPORT_C void NKern::ThreadRequestSignal(NThread* aThread, NFastMutex* aMutex)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::ThreadRequestSignal(NThread*,NFastMutex*)");
	if (!aMutex)
		aMutex = &TheScheduler.iLock;
	NKern::Lock();
	aThread->iRequestSemaphore.Signal();
	aMutex->Signal();
	NKern::Unlock();
	}


/**	Kills a nanothread.

	This function is intended to be used by the EPOC layer and personality layers.
	Do not use this function directly on a Symbian OS thread - use Kern::ThreadKill().

	This function does not return if the current thread is killed.  
	This function is asynchronous (i.e. the thread to kill may still be alive when the call returns).

	@param aThread Thread to kill.  Must be non NULL.

	@pre If acting on calling thread, calling thread must not be in a
			critical section
	@pre Thread must not already be exiting.

	@see Kern::ThreadKill()
 */
EXPORT_C void NKern::ThreadKill(NThread* aThread)
	{
	NKern::Lock();
	aThread->Kill();
	NKern::Unlock();
	}


/**	Atomically kills a nanothread and signals a fast mutex.

	This function is intended to be used by the EPOC layer and personality layers.
	Do not use this function directly on a Symbian OS thread - use Kern::ThreadKill().

	@param aThread Thread to kill.  Must be non NULL.
	@param aMutex Fast mutex to signal.  If NULL, the system lock is signalled.

	@pre	If acting on calling thread, calling thread must not be in a
			critical section
	@pre Thread must not already be exiting.

	@see NKern::ThreadKill(NThread*)
 */
EXPORT_C void NKern::ThreadKill(NThread* aThread, NFastMutex* aMutex)
	{
	if (!aMutex)
		aMutex = &TheScheduler.iLock;
	NThreadBase* pC = NKern::LockC();
	if (aThread==pC)
		{
		__NK_ASSERT_DEBUG(pC->iCsCount==0);	// Make sure thread isn't in critical section
		__NK_ASSERT_ALWAYS(aMutex->HeldByCurrentThread());
		pC->AcqSLock();
		aThread->iCsFunction = NThreadBase::ECSExitPending;
		pC->RelSLock();
		aMutex->iHoldingThread = (NThreadBase*)(TLinAddr(aThread) | 1);
		aMutex->Signal();	// this will make us exit
		FAULT();			// should never get here
		}
	else
		{
		aThread->Kill();
		aMutex->Signal();
		}
	NKern::Unlock();
	}


/** Enters thread critical section.

	This function can safely be used in device drivers.

    The current thread will enter its critical section. While in critical section
    the thread cannot be suspended or killed. Any suspension or kill will be deferred
    until the thread leaves the critical section.
    Some API explicitly require threads to be in critical section before calling that
    API.
    Only User threads need to call this function as the concept of thread critical
    section applies to User threads only.

	@pre	Call in a thread context.
	@pre	Kernel must be unlocked.
*/
EXPORT_C void NKern::ThreadEnterCS()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::ThreadEnterCS");
	NThreadBase* pC = NKern::CurrentThread();
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T NEntCS",pC));
	__NK_ASSERT_DEBUG(pC->iCsCount>=0);
	++pC->iCsCount;
	}

NThread* NKern::_ThreadEnterCS()
	{
	NThreadBase* pC = NKern::CurrentThread();
	__NK_ASSERT_DEBUG(pC->iCsCount>=0);
	++pC->iCsCount;
	return (NThread*)pC;
	}


/** Leaves thread critical section.

	This function can safely be used in device drivers.

    The current thread will leave its critical section. If the thread was suspended/killed
    while in critical section, the thread will be suspended/killed after leaving the
    critical section by calling this function.
    Only User threads need to call this function as the concept of thread critical
    section applies to User threads only.

	@pre	Call in a thread context.
	@pre	Kernel must be unlocked.
*/
EXPORT_C void NKern::ThreadLeaveCS()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::ThreadLeaveCS");
	NThreadBase* pC = NKern::LockC();
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T NLvCS",pC));
	pC->AcqSLock();
	__NK_ASSERT_DEBUG(pC->iCsCount>0);
	if (--pC->iCsCount==0 && pC->iCsFunction!=0)
		{
		NFastMutex* m = HeldFastMutex();
		if (m)
			m->iHoldingThread = (NThreadBase*)(TLinAddr(pC) | 1);
		else
			pC->DoCsFunctionT();
		}
	pC->RelSLock();
	NKern::Unlock();
	}

void NKern::_ThreadLeaveCS()
	{
	NThreadBase* pC = NKern::LockC();
	pC->AcqSLock();
	__NK_ASSERT_DEBUG(pC->iCsCount>0);
	if (--pC->iCsCount==0 && pC->iCsFunction!=0)
		{
		NFastMutex* m = HeldFastMutex();
		if (m)
			m->iHoldingThread = (NThreadBase*)(TLinAddr(pC) | 1);
		else
			pC->DoCsFunctionT();
		}
	pC->RelSLock();
	NKern::Unlock();
	}

/** Freeze the CPU of the current thread

	After this the current thread will not migrate to another processor

	@return	A cookie to be passed to NKern::EndFreezeCpu() to allow nesting
*/
EXPORT_C TInt NKern::FreezeCpu()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::FreezeCpu");
	NKern::Lock();
	TSubScheduler& ss = SubScheduler();
	NThreadBase* pC = ss.iCurrentThread;
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T NFrzCpu",pC));
	if (pC->iFreezeCpu)
		{
		NKern::Unlock();
		return 1;
		}
	pC->iFreezeCpu = 1;
	__e32_atomic_add_rlx32(&ss.iDeferShutdown, 1);
	if (pC->iParent != pC)
		{
		pC->AcqSLock();
		++pC->iParent->iFreezeCpu;
		pC->RelSLock();
		}
	NKern::Unlock();
	return 0;
	}


/** Unfreeze the current thread's CPU

	After this the current thread will again be eligible to migrate to another processor

	@param	aCookie the value returned by NKern::FreezeCpu()
*/
EXPORT_C void NKern::EndFreezeCpu(TInt aCookie)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::EndFreezeCpu");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T NEndFrz %d",NKern::CurrentThread(),aCookie));
	if (aCookie)
		return;
	NKern::Lock();
	TSubScheduler& ss = SubScheduler();
	NThreadBase* pC = ss.iCurrentThread;
	if (pC->iFreezeCpu)
		{
		pC->iFreezeCpu = 0;
		mb();
		if (pC->iParent != pC)
			{
			pC->AcqSLock();
			if (!--pC->iParent->iFreezeCpu && pC->iParent->iCpuChange)
				RescheduleNeeded();
			pC->RelSLock();
			}
		else if (pC->iCpuChange)		// deferred CPU change?
			RescheduleNeeded();
		__e32_atomic_add_rlx32(&ss.iDeferShutdown, TUint32(-1));
		}
	NKern::Unlock();
	}


/** Change the CPU affinity of a thread

	@pre	Call in a thread context.

	@param	The new CPU affinity mask
	@return The old affinity mask
 */
EXPORT_C TUint32 NKern::ThreadSetCpuAffinity(NThread* aThread, TUint32 aAffinity)
	{
	aThread->LAcqSLock();
	TUint32 r = aThread->SetCpuAffinityT(aAffinity);
	aThread->RelSLockU();
	return r;
	}


/** Modify a thread's timeslice

	@pre	Call in a thread context.

	@param	aTimeslice	The new timeslice value
 */
EXPORT_C void NKern::ThreadSetTimeslice(NThread* aThread, TInt aTimeslice)
	{
	NKern::Lock();
	aThread->AcqSLock();
	if (aThread->iTimeslice == aThread->iTime || aTimeslice<0)
		aThread->iTime = aTimeslice;
	aThread->iTimeslice = aTimeslice;
	aThread->RelSLock();
	NKern::Unlock();
	}


/** Blocks current nanothread.

	This function is used to implement synchronisation primitives in the EPOC
	layer and in personality layers.  It is not intended to be used directly by
	device drivers.  

	@param aTimeout If greater than 0, the nanothread will be blocked for at most
					aTimeout microseconds.
	@param aMode	Bitmask whose possible values are documented in TBlockMode.  
	@param aMutex	Fast mutex to operate on.  If NULL, the system lock is used.

	@see NKern::ThreadRelease()
	@see TBlockMode

	@pre	Call in a thread context.
	@pre	Interrupts must be enabled.
	@pre	Kernel must be unlocked.
	@pre	Specified mutex must be held
 */
EXPORT_C TInt NKern::Block(TUint32 aTimeout, TUint aMode, NFastMutex* aMutex)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::Block(TUint32,TUint,NFastMutex*)");
	if (!aMutex)
		aMutex = &TheScheduler.iLock;
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::Block time %d mode %d FM %M",aTimeout,aMode,aMutex));
	if (aMode & EEnterCS)
		NKern::_ThreadEnterCS();	// NOTE: MUST DO THIS BEFORE CALLING NKern::Lock()
	NThreadBase* pC = NKern::LockC();
	TUint flags = (aMode & NKern::EObstruct) ? NThreadWaitState::EWtStObstructed : 0;
	pC->iWaitState.SetUpWait(NThreadBase::EWaitBlocked, flags, 0, aTimeout);
	if (aMode & ERelease)
		aMutex->Signal();
	RescheduleNeeded();
	NKern::Unlock();	// thread blocks here
	TInt r = pC->iWaitState.iWtC.iRetVal;	// sample here since it will be overwritten if we block on the fast mutex
	if (aMode & EClaim)
		FMWait(aMutex);
	return r;
	}


/**
@pre	Call in a thread context.
@pre	Interrupts must be enabled.
@pre	Kernel must be unlocked.
@pre	No fast mutex can be held
*/
/** @see NKern::Block(TUint32, TUint, NFastMutex*) */
EXPORT_C TInt NKern::Block(TUint32 aTimeout, TUint aMode)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"NKern::Block(TUint32,TUint)");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::Block time %d mode %d",aTimeout,aMode));
	if (aMode & EEnterCS)
		NKern::_ThreadEnterCS();	// NOTE: MUST DO THIS BEFORE CALLING NKern::Lock()
	NThreadBase* pC = NKern::LockC();
	TUint flags = (aMode & NKern::EObstruct) ? NThreadWaitState::EWtStObstructed : 0;
	pC->iWaitState.SetUpWait(NThreadBase::EWaitBlocked, flags, 0, aTimeout);
	RescheduleNeeded();
	NKern::Unlock();	// thread blocks here
	return pC->iWaitState.iWtC.iRetVal;
	}




/**
Places the current nanothread into a wait state on an externally
defined wait object.
	
For use by RTOS personality layers.
Do not use this function directly on a Symbian OS thread.

Since the kernel is locked on entry, any reschedule will be deferred until
it is unlocked. The thread should be added to any necessary wait queue after
a call to this function, since this function removes it from the ready list.
The thread's wait timer is started if aTimeout is nonzero.
The thread's NState and wait object are updated.

Call NThreadBase::Release() when the wait condition is resolved.

@param aTimeout The maximum time for which the thread should block, in nanokernel timer ticks.
                A zero value means wait forever.
                If the thread is still blocked when the timeout expires,
                then the timeout state handler will be called.
@param aState   The nanokernel thread state (N-State) value to be set.
                This state corresponds to the externally defined wait object.
                This value will be written into the member NThreadBase::iNState.
@param aWaitObj A pointer to an externally defined wait object.
                This value will be written into the member NThreadBase::iWaitObj.

@pre	Kernel must be locked.
@pre	Call in a thread context.

@post	Kernel is locked.

@see	NThreadBase::Release()
*/
EXPORT_C void NKern::NanoBlock(TUint32 aTimeout, TUint aState, TAny* aWaitObj)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::NanoBlock");		
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NanoBlock time %d state %d obj %08x", aTimeout, aState, aWaitObj));
	NThreadBase* pC = NCurrentThreadL();
	pC->iWaitState.SetUpWait(aState, aState>>8, aWaitObj, aTimeout);
	RescheduleNeeded();
	}




EXPORT_C void NKern::Sleep(TUint32 aTime)
/**
Puts the current nanothread to sleep for the specified duration.

It can be called from Symbian OS threads.

@param	aTime sleep time in nanokernel timer ticks.

@pre    No fast mutex can be held.
@pre    Kernel must be unlocked.
@pre	Call in a thread context.
@pre	Interrupts must be enabled.
*/
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"NKern::Sleep");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NSlp %d",aTime));
	NThreadBase* pC = NKern::LockC();
	pC->iWaitState.SetUpWait(NThreadBase::EWaitSleep, 0, 0, aTime);
	RescheduleNeeded();
	NKern::Unlock();
	}


/**	Terminates the current nanothread.

	Calls to this function never return.

	For use by RTOS personality layers.
	Do not use this function directly on a Symbian OS thread.

	@pre	Call in a thread context.
	@pre	Interrupts must be enabled.
	@pre	Kernel must be unlocked.	
 */
EXPORT_C void NKern::Exit()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::Exit");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NExit"));
	NKern::LockC()->Exit();		// this won't return
	FAULT();
	}


/**	Terminates the current nanothread at the next possible point.

	If the calling thread is not currently in a critical section and does not
	currently hold a fast mutex, it exits immediately and this function does
	not return. On the other hand if the thread is in a critical section or
	holds a fast mutex the thread continues executing but it will exit as soon
	as it leaves the critical section and/or releases the fast mutex.

	@pre	Call in a thread context.
	@pre	Interrupts must be enabled.
	@pre	Kernel must be unlocked.	
 */
EXPORT_C void NKern::DeferredExit()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::DeferredExit");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NDefExit"));
	NFastMutex* m = HeldFastMutex();
	NThreadBase* pC = NKern::LockC();
	if (!m && !pC->iCsCount)
		pC->Exit();			// this won't return
	pC->AcqSLock();
	if (pC->iCsFunction >= 0)	// don't touch it if we are already exiting
		pC->iCsFunction = NThreadBase::ECSExitPending;
	pC->RelSLock();
	if (m && !pC->iCsCount)
		m->iHoldingThread = (NThreadBase*)(TLinAddr(pC) | 1);
	NKern::Unlock();
	}


/** Prematurely terminates the current thread's timeslice

	@pre	Kernel must be unlocked.
	@pre	Call in a thread context.
	
	@post	Kernel is unlocked.
 */
EXPORT_C void NKern::YieldTimeslice()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::YieldTimeslice");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::YieldTimeslice"));
	NThreadBase* t = NKern::LockC();
	t->iTime = 0;
	mb();
	if (t->iNext!=t || t->iParent->iNext!=t->iParent)
		RescheduleNeeded();
	NKern::Unlock();
	}


/** Returns the number of CPUs available to Symbian OS

	@return the number of CPUs
	
	@pre Call in any context.
*/
EXPORT_C TInt NKern::NumberOfCpus()
	{
	return TheScheduler.iNumCpus;
	}


/** Rotates the specified CPU ready list for threads at the specified priority.
	
	For use by RTOS personality layers to allow external control of round-robin
	scheduling. Not intended for direct use by device drivers.

	@param	aPriority = priority at which threads should be rotated.
						-1 means use calling thread's priority.
	@param	aCpu		CPU to act on
	
	@pre	Kernel must be unlocked.
	@pre	Call in a thread context.
	
	@post	Kernel is unlocked.
 */

EXPORT_C void NKern::RotateReadyList(TInt aPriority, TInt aCpu)
	{
//	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::RotateReadyList");
//	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::RotateReadyList %d",aPriority));
//	if (aPriority<0 || aPriority>=KNumPriorities)
//		aPriority=NKern::CurrentThread()->iPriority;
//	NKern::Lock();
//	TheScheduler.RotateReadyList(aPriority);
//	NKern::Unlock();
	}


/** Rotates the ready list for threads at the specified priority.
	
	For use by RTOS personality layers to allow external control of round-robin
	scheduling. Not intended for direct use by device drivers.

	@param	aPriority = priority at which threads should be rotated.
						-1 means use calling thread's priority.
	
	@pre	Kernel must be unlocked.
	@pre	Call in a thread context.
	
	@post	Kernel is unlocked.
 */
EXPORT_C void NKern::RotateReadyList(TInt aPriority)
	{
	RotateReadyList(aPriority, -1);
	}


/** Returns a pointer to the thread group to which the current thread belongs,
	if any.	Returns NULL if current thread is a standalone thread.
	
	@pre	Call in a thread context.
 */
EXPORT_C NThreadGroup* NKern::CurrentGroup()
	{
	NThreadBase* pC = NKern::CurrentThread();
	return (pC->iParent == pC) ? (NThreadGroup*)0 : (NThreadGroup*)pC->iParent;
	}


/** Detaches the current thread from the group to which it currently belongs,
	if any.	Returns a pointer to the group (NULL if none).
		
	@pre	Call in a thread context.
	@pre	Interrupts enabled
	@pre	Kernel unlocked
 */
EXPORT_C NThreadGroup* NKern::LeaveGroup()
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR|MASK_NOT_IDFC|MASK_INTERRUPTS_ENABLED|MASK_KERNEL_UNLOCKED, "NKern::LeaveGroup");
	NKern::Lock();
	TSubScheduler& ss = SubScheduler();
	NThreadBase* pC = ss.iCurrentThread;
	pC->iNewParent = 0;	// cancel any pending Join
	NThreadGroup* g = (pC->iParent == pC) ? (NThreadGroup*)0 : (NThreadGroup*)pC->iParent;
	TBool make_group_ready = FALSE;
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NLeaveGroup %T (%G)",pC,g));
	if (g)
		{
		while (!pC->TiedEventLeaveInterlock())
			{
			TInt irq = NKern::DisableAllInterrupts();
			ss.QueueDfcs();
			NKern::RestoreInterrupts(irq);
			}
		pC->AcqSLock();
		ss.iReadyListLock.LockOnly();
		pC->UnReadyT();
		pC->iParent = pC;
		g->iCurrent = 0;	// since current thread is no longer in g
		TUint64 now = NKern::Timestamp();
		g->iLastRunTime.i64 = now;
		g->iTotalCpuTime.i64 += (now - g->iLastStartTime.i64);
		if (--g->iActiveState == 0)
			{
			// group no longer active
			g->iTotalActiveTime.i64 += (now - g->iLastActivationTime.i64);
			}
		ss.SSAddEntryHead(pC);
		pC->iReady = TUint8(ss.iCpuNum | NSchedulable::EReadyOffset);
		pC->iCpuAffinity = g->iCpuAffinity;	// keep same CPU affinity
		// if we're frozen, the group's freeze count was incremented
		if (pC->iFreezeCpu)
			--g->iFreezeCpu;
		// if we've been marked as deferring, the group's freeze count was incremented
		if (pC->iFastMutexDefer == 1)
			{
			--g->iFreezeCpu;
			pC->iFastMutexDefer = 0;
			}
		// if the group was waiting to change cpu then this thread needs to change still
		if (g->iCpuChange)
			{
			pC->iCpuChange = g->iCpuChange;
			RescheduleNeeded();
			if (!g->iFreezeCpu)
				{
				// we were the last thread in the group stopping it from moving
				// but there may be no other threads left after UnReadyT'ing this one
				g->iCpuChange = FALSE;
				if (g->iReady)
					{
					ss.SSRemoveEntry(g);
					g->iReady = 0;
					make_group_ready = TRUE;
					}
				}
			}
		ss.iReadyListLock.UnlockOnly();
		--g->iThreadCount;
		if (make_group_ready)
			g->ReadyT(0);
		g->RelSLock();		// since pC is no longer attached to g
		pC->RelSLock();
		}
	NKern::Unlock();
	return g;
	}


/** Adds the current thread to the specified group.
	
	@param	aGroup = pointer to group to join
	
	@pre	Call in a thread context, not in one of the idle threads.
	@pre	Interrupts enabled
	@pre	Kernel unlocked
	@pre	Thread does not hold a fast mutex
	@pre	Thread does not have a freeze on CPU migration
	@pre	Current thread is not already in a group
 */
EXPORT_C void NKern::JoinGroup(NThreadGroup* aGroup)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD, "NKern::JoinGroup");
	NKern::Lock();
	TSubScheduler& ss = SubScheduler();
	NThreadBase* pC = ss.iCurrentThread;
	__ASSERT_WITH_MESSAGE_DEBUG(pC->iParent==pC, "Thread not already in a group", "NKern::JoinGroup");
	__ASSERT_WITH_MESSAGE_DEBUG(!pC->iFreezeCpu, "No interdiction on CPU migration", "NKern::JoinGroup");
	__ASSERT_WITH_MESSAGE_DEBUG(!pC->i_NThread_Initial, "Not idle thread", "NKern::JoinGroup");
	__NK_ASSERT_ALWAYS(pC->iParent==pC && !pC->iFreezeCpu);
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NJoinGroup %T->%G",pC,aGroup));
	pC->AcqSLock();
	aGroup->AcqSLock();

	// Check if current CPU is compatible with group's affinity
	TBool migrate = !CheckCpuAgainstAffinity(ss.iCpuNum, aGroup->iCpuAffinity);
	if (!aGroup->iReady || aGroup->iReady==pC->iReady)
		{
		// group not ready or ready on this CPU
		if (!migrate)
			{
			ss.iReadyListLock.LockOnly();
			pC->UnReadyT();
			pC->iParent = aGroup;
			aGroup->iNThreadList.AddHead(pC);
			if (!aGroup->iReady)
				{
				aGroup->iPriority = pC->iPriority;
				ss.SSAddEntryHead(aGroup);
				aGroup->iReady = TUint8(ss.iCpuNum | NSchedulable::EReadyOffset);
				}
			else if (pC->iPriority > aGroup->iPriority)
				ss.SSChgEntryP(aGroup, pC->iPriority);
			pC->iReady = NSchedulable::EReadyGroup;
			aGroup->iCurrent = aGroup->iReady;
			ss.iReadyListLock.UnlockOnly();
			++aGroup->iThreadCount;
			TUint64 now = NKern::Timestamp();
			aGroup->iLastStartTime.i64 = now;
			if (++aGroup->iActiveState == 1)
				aGroup->iLastActivationTime.i64 = now;
			goto done;
			}
		}
	// this thread needs to migrate to another CPU
	pC->iNewParent = aGroup;
	RescheduleNeeded();

	// the following reschedule definitely joins the group even if the
	// thread's CPU affinity is incompatible with that of the group
	// (the thread's CPU affinity is subsequently determined by that of
	// the group)

done:
	if (pC->iParent != aGroup)
		aGroup->RelSLock();
	pC->RelSLock();
	while (!pC->TiedEventJoinInterlock())
		{
		TInt irq = NKern::DisableAllInterrupts();
		ss.QueueDfcs();
		NKern::RestoreInterrupts(irq);
		}
	NKern::Unlock();
	}


/******************************************************************************
 * Iterable Doubly Linked List
 ******************************************************************************/
TInt SIterDQIterator::Step(SIterDQLink*& aObj, TInt aMaxSteps)
	{
	if (aMaxSteps <= 0)
		aMaxSteps = KMaxCpus + 3;
	SIterDQLink* p = Next();
	SIterDQLink* q = p;
	__NK_ASSERT_DEBUG(p!=0);
	for(; p->IsIterator() && --aMaxSteps>0; p=p->Next())
		{}
	if (p->IsObject())
		{
		// found object
		Deque();
		InsertAfter(p);
		aObj = p;
		return KErrNone;
		}
	if (p->IsAnchor())
		{
		// reached end of list
		if (p != q)
			{
			Deque();
			InsertBefore(p);	// put at the end
			}
		aObj = 0;
		return KErrEof;
		}
	// Maximum allowed number of other iterators skipped
	Deque();
	InsertAfter(p);
	aObj = 0;
	return KErrGeneral;
	}


/******************************************************************************
 * Priority Lists
 ******************************************************************************/

#ifndef __PRI_LIST_MACHINE_CODED__
/** Returns the priority of the highest priority item present on a priority list.

	@return	The highest priority present or -1 if the list is empty.
 */
EXPORT_C TInt TPriListBase::HighestPriority()
	{
	return __e32_find_ms1_64(iPresent64);
	}


/** Finds the highest priority item present on a priority list.

	If multiple items at the same priority are present, return the first to be
	added in chronological order.

	@return	A pointer to the item or NULL if the list is empty.
 */
EXPORT_C TPriListLink* TPriListBase::First()
	{
	TInt p = HighestPriority();
	return p >=0 ? static_cast<TPriListLink*>(iQueue[p]) : NULL;
	}


/** Adds an item to a priority list at the tail of the queue for its priority.

	@param aLink A pointer to the item - must not be NULL.
 */
EXPORT_C void TPriListBase::Add(TPriListLink* aLink)
	{
	TInt p = aLink->iPriority;
	SDblQueLink* head = iQueue[p];
	if (head)
		{
		// already some at this priority
		aLink->InsertBefore(head);
		}
	else
		{
		// 'create' new list
		iQueue[p] = aLink;
		aLink->iNext = aLink->iPrev = aLink;
		iPresent[p>>5] |= 1u << (p & 0x1f);
		}
	}


/** Removes an item from a priority list.

	@param aLink A pointer to the item - must not be NULL.
 */
EXPORT_C void TPriListBase::Remove(TPriListLink* aLink)
	{
	if (!aLink->Alone())
		{
		// not the last on this list
		TInt p = aLink->iPriority;
		if (iQueue[p] == aLink)
			iQueue[p] = aLink->iNext;
		aLink->Deque();
		}
	else
		{
		TInt p = aLink->iPriority;
		iQueue[p] = 0;
		iPresent[p>>5] &= ~(1u << (p & 0x1f));
		KILL_LINK(aLink);
		}
	}


/** Changes the priority of an item on a priority list.

	@param	aLink A pointer to the item to act on - must not be NULL.
	@param	aNewPriority A new priority for the item.
 */
EXPORT_C void TPriListBase::ChangePriority(TPriListLink* aLink, TInt aNewPriority)
	{
	if (aLink->iPriority!=aNewPriority)
		{
		Remove(aLink);
		aLink->iPriority=TUint8(aNewPriority);
		Add(aLink);
		}
	}
#endif

/** Adds an item to a priority list at the head of the queue for its priority.

	@param aLink A pointer to the item - must not be NULL.
 */
EXPORT_C void TPriListBase::AddHead(TPriListLink* aLink)
	{
	TInt p = aLink->iPriority;
	SDblQueLink* head = iQueue[p];
	iQueue[p] = aLink;
	if (head)
		{
		// already some at this priority
		aLink->InsertBefore(head);
		}
	else
		{
		// 'create' new list
		aLink->iNext = aLink->iPrev = aLink;
		iPresent[p>>5] |= 1u << (p & 0x1f);
		}
	}


/******************************************************************************
 * Generic IPIs
 ******************************************************************************/

extern "C" {
extern void send_generic_ipis(TUint32);

void generic_ipi_isr(TSubScheduler* aS)
	{
	TScheduler& s = TheScheduler;
	TGenericIPI* ipi = aS->iNextIPI;
	if (!ipi)
		return;
	TUint32 m = aS->iCpuMask;
	SDblQueLink* anchor = &s.iGenIPIList.iA;
	while (ipi != anchor)
		{
		__e32_atomic_and_acq32(&ipi->iCpusIn, ~m);
		(*ipi->iFunc)(ipi);
		TInt irq = s.iGenIPILock.LockIrqSave();
		TGenericIPI* n = (TGenericIPI*)ipi->iNext;
		ipi->iCpusOut &= ~m;
		if (ipi->iCpusOut == 0)
			{
			ipi->Deque();
			mb();
			ipi->iNext = 0;
			}
		ipi = n;
		while (ipi!=anchor && !(ipi->iCpusIn & m))
			ipi = (TGenericIPI*)ipi->iNext;
		if (ipi == anchor)
			aS->iNextIPI = 0;
		s.iGenIPILock.UnlockIrqRestore(irq);
		}
	}
}

void TGenericIPI::Queue(TGenericIPIFn aFunc, TUint32 aCpuMask)
	{
	__KTRACE_OPT(KSCHED2,DEBUGPRINT("GenIPI F=%08x M=%08x", aFunc, aCpuMask));
	iFunc = aFunc;
	TScheduler& s = TheScheduler;
	TInt i;
	TUint32 ipis = 0;
	TInt irq = s.iGenIPILock.LockIrqSave();
	if (aCpuMask & 0x80000000u)
		{
		if (aCpuMask==0xffffffffu)
			aCpuMask = s.iIpiAcceptCpus;
		else if (aCpuMask==0xfffffffeu)
			aCpuMask = s.iIpiAcceptCpus &~ SubScheduler().iCpuMask;
		else
			aCpuMask = 0;
		}
	iCpusIn = aCpuMask;
	iCpusOut = aCpuMask;
	if (!aCpuMask)
		{
		s.iGenIPILock.UnlockIrqRestore(irq);
		iNext = 0;
		return;
		}
	s.iGenIPIList.Add(this);
	for (i=0; i<s.iNumCpus; ++i)
		{
		if (!(aCpuMask & (1<<i)))
			continue;
		TSubScheduler& ss = *s.iSub[i];
		if (!ss.iNextIPI)
			{
			ss.iNextIPI = this;
			ipis |= (1<<i);
			}
		}
	send_generic_ipis(ipis);
	s.iGenIPILock.UnlockIrqRestore(irq);
	__KTRACE_OPT(KSCHED2,DEBUGPRINT("GenIPI ipis=%08x", ipis));
	}

void TGenericIPI::QueueAll(TGenericIPIFn aFunc)
	{
	Queue(aFunc, 0xffffffffu);
	}

void TGenericIPI::QueueAllOther(TGenericIPIFn aFunc)
	{
	Queue(aFunc, 0xfffffffeu);
	}

// Call from thread or IDFC with interrupts enabled
void TGenericIPI::WaitEntry()
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR|MASK_INTERRUPTS_ENABLED,"TGenericIPI::WaitEntry");
	while (iCpusIn)
		{
		__chill();
		}
	mb();
	}

// Call from thread or IDFC with interrupts enabled
void TGenericIPI::WaitCompletion()
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR|MASK_INTERRUPTS_ENABLED,"TGenericIPI::WaitCompletion");
	volatile TInt* p = (volatile TInt*)&iNext;
	while (*p)
		{
		__chill();
		}
	mb();
	}

/**	Stop all other CPUs

Call with kernel unlocked, returns with kernel locked.
Returns mask of CPUs halted plus current CPU.
*/
TUint32 TStopIPI::StopCPUs()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"TStopIPI::StopCPUs()");
	TScheduler& s = TheScheduler;
	iFlag = 0;
	NKern::ThreadEnterCS();

	// Stop any cores powering up or down for now
	// A core already on the way down will stop just before the transition to SHUTDOWN_FINAL
	// A core already on the way up will carry on powering up
	TInt irq = s.iGenIPILock.LockIrqSave();
	++s.iCCDeferCount;	// stops bits in iIpiAcceptCpus being cleared, but doesn't stop them being set
						// but iIpiAcceptCpus | s.iCpusComingUp is constant
	TUint32 act2 = s.iIpiAcceptCpus;		// CPUs still accepting IPIs
	TUint32 cu = s.iCpusComingUp;			// CPUs powering up
	s.iGenIPILock.UnlockIrqRestore(irq);
	TUint32 cores = act2 | cu;
	if (cu)
		{
		// wait for CPUs coming up to start accepting IPIs
		while (cores & ~s.iIpiAcceptCpus)
			{
			__snooze();	// snooze until cores have come up
			}
		}
	NKern::Lock();
	QueueAllOther(&Isr);	// send IPIs to all other CPUs
	WaitEntry();			// wait for other CPUs to reach the ISR
	return cores;
	}


/**	Release the stopped CPUs

Call with kernel locked, returns with kernel unlocked.
*/
void TStopIPI::ReleaseCPUs()
	{
	__e32_atomic_store_rel32(&iFlag, 1);	// allow other CPUs to proceed
	WaitCompletion();		// wait for them to finish with this IPI
	NKern::Unlock();
	TheScheduler.CCUnDefer();
	NKern::ThreadLeaveCS();
	}

void TStopIPI::Isr(TGenericIPI* a)
	{
	TStopIPI* s = (TStopIPI*)a;
	while (!__e32_atomic_load_acq32(&s->iFlag))
		{
		__chill();
		}
	__e32_io_completion_barrier();
	}


/******************************************************************************
 * TCoreCycler - general method to execute something on all active cores
 ******************************************************************************/
TCoreCycler::TCoreCycler()
	{
	iCores = 0;
	iG = 0;
	}

void TCoreCycler::Init()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"TCoreCycler::Init()");
	TScheduler& s = TheScheduler;
	NKern::ThreadEnterCS();
	iG = NKern::LeaveGroup();
	NThread* t = NKern::CurrentThread();
	if (t->iCoreCycling)
		{
		__crash();
		}
	t->iCoreCycling = TRUE;

	// Stop any cores powering up or down for now
	// A core already on the way down will stop just before the transition to SHUTDOWN_FINAL
	// A core already on the way up will carry on powering up
	TInt irq = s.iGenIPILock.LockIrqSave();
	++s.iCCDeferCount;	// stops bits in iIpiAcceptCpus being cleared, but doesn't stop them being set
						// but iIpiAcceptCpus | s.iCpusComingUp is constant
	TUint32 act2 = s.iIpiAcceptCpus;		// CPUs still accepting IPIs
	TUint32 cu = s.iCpusComingUp;			// CPUs powering up
	TUint32 gd = s.iCpusGoingDown;			// CPUs no longer accepting IPIs on the way down
	s.iGenIPILock.UnlockIrqRestore(irq);
	if (gd)
		{
		// wait for CPUs going down to reach INACTIVE state
		TUint32 remain = gd;
		FOREVER
			{
			TInt i;
			for (i=0; i<KMaxCpus; ++i)
				{
				if (remain & (1u<<i))
					{
					// platform specific function returns TRUE when core has detached from SMP cluster
					if (s.iSub[i]->Detached())
						remain &= ~(1u<<i);	// core is now down
					}
				}
			if (!remain)
				break;		// all done
			else
				{
				__snooze();	// snooze until cores have gone down
				}
			}
		}
	iCores = act2 | cu;
	if (cu)
		{
		// wait for CPUs coming up to start accepting IPIs
		while (iCores & ~s.iIpiAcceptCpus)
			{
			__snooze();	// snooze until cores have come up
			}
		}
	iFrz = NKern::FreezeCpu();
	if (iFrz)
		__crash();	// already frozen so won't be able to migrate :-(
	iInitialCpu = NKern::CurrentCpu();
	iCurrentCpu = iInitialCpu;
	iRemain = iCores;
	}

TInt TCoreCycler::Next()
	{
	NThread* t = NKern::CurrentThread();
	if (iCores == 0)
		{
		Init();
		return KErrNone;
		}
	if (NKern::CurrentCpu() != iCurrentCpu)
		__crash();
	iRemain &= ~(1u<<iCurrentCpu);
	TInt nextCpu = iRemain ? __e32_find_ms1_32(iRemain) : iInitialCpu;
	if (nextCpu != iCurrentCpu)
		{
		NKern::JumpTo(nextCpu);
		iCurrentCpu = nextCpu;
		if (NKern::CurrentCpu() != iCurrentCpu)
			__crash();
		}
	if (iRemain)
		{
		return KErrNone;
		}
	NKern::EndFreezeCpu(iFrz);
	iCores = 0;
	TScheduler& s = TheScheduler;
	s.CCUnDefer();
	t->iCoreCycling = FALSE;
	if (iG)
		NKern::JoinGroup(iG);
	NKern::ThreadLeaveCS();
	return KErrEof;
	}
