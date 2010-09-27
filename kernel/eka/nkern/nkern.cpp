// Copyright (c) 1998-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkern\nkern.cpp
// 
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

#include "nk_priv.h"

/******************************************************************************
 * Fast mutex
 ******************************************************************************/

/** Checks if the current thread holds this fast mutex

	@return TRUE if the current thread holds this fast mutex
	@return FALSE if not
*/
EXPORT_C TBool NFastMutex::HeldByCurrentThread()
	{
	return iHoldingThread == NCurrentThread();
	}

/** Find the fast mutex held by the current thread

	@return a pointer to the fast mutex held by the current thread
	@return NULL if the current thread does not hold a fast mutex
*/
EXPORT_C NFastMutex* NKern::HeldFastMutex()
	{
	return TheScheduler.iCurrentThread->iHeldFastMutex;
	}


#ifndef __FAST_MUTEX_MACHINE_CODED__
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
	__KTRACE_OPT(KNKERN,DEBUGPRINT("FMWait %M",this));
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED_ONCE|MASK_NO_FAST_MUTEX,"NFastMutex::Wait");			
	NThreadBase* pC=TheScheduler.iCurrentThread;
	if (iHoldingThread)
		{
		iWaiting=1;
		pC->iWaitFastMutex=this;
		__KTRACE_OPT(KNKERN,DEBUGPRINT("FMWait: YieldTo %T",iHoldingThread));
		TheScheduler.YieldTo(iHoldingThread);	// returns with kernel unlocked, interrupts disabled
		TheScheduler.iKernCSLocked = 1;	// relock kernel
		NKern::EnableAllInterrupts();
		pC->iWaitFastMutex=NULL;
		}
	pC->iHeldFastMutex=this;		// automatically puts thread into critical section
#ifdef BTRACE_FAST_MUTEX
	BTraceContext4(BTrace::EFastMutex,BTrace::EFastMutexWait,this);
#endif
	iHoldingThread=pC;
	}


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
	__KTRACE_OPT(KNKERN,DEBUGPRINT("FMSignal %M",this));
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED,"NFastMutex::Signal");			
	NThreadBase* pC=TheScheduler.iCurrentThread;
	__ASSERT_WITH_MESSAGE_DEBUG(pC->iHeldFastMutex==this,"The calling thread holds the mutex","NFastMutex::Signal");
#ifdef BTRACE_FAST_MUTEX
	BTraceContext4(BTrace::EFastMutex,BTrace::EFastMutexSignal,this);
#endif
	iHoldingThread=NULL;
	pC->iHeldFastMutex=NULL;
	TBool w=iWaiting;
	iWaiting=0;
	if (w)
		{
		RescheduleNeeded();
		if (pC->iCsFunction && !pC->iCsCount)
			pC->DoCsFunction();
		}
	}


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
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"NKern::FMWait");
	NKern::Lock();
	aMutex->Wait();
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
	NKern::Lock();
	aMutex->Signal();
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
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"NKern::LockSystem");
	NKern::Lock();
	TheScheduler.iLock.Wait();
	NKern::Unlock();
	}


/** Releases the System Lock.

	@pre System lock must be held.

	@see NKern::LockSystem()
	@see NKern::FMSignal()
*/
EXPORT_C void NKern::UnlockSystem()
	{
	NKern::Lock();
	TheScheduler.iLock.Signal();
	NKern::Unlock();
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"NKern::UnlockSystem");
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
	__ASSERT_WITH_MESSAGE_DEBUG(aM->HeldByCurrentThread(),"The calling thread holds the mutex","NKern::FMFlash");
	TBool w = aM->iWaiting;
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
		BTraceContext4(BTrace::EFastMutex,BTrace::EFastMutexFlash,aM);
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
	if(!aThread)
		aThread = TheScheduler.iCurrentThread;
	if(iOwningThread && iOwningThread!=aThread)
		{
		__NK_ASSERT_ALWAYS(!iCount);	// Can't change owner if iCount!=0
		}
	iOwningThread = aThread;
	}


#ifndef __FAST_SEM_MACHINE_CODED__
/** Waits on a fast semaphore.

    Decrements the signal count for the semaphore and
	removes the calling thread from the ready-list if the sempahore becomes
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
	NThreadBase* pC=TheScheduler.iCurrentThread;
	__ASSERT_WITH_MESSAGE_ALWAYS(pC==iOwningThread,"The calling thread must own the semaphore","NFastSemaphore::Wait");
	if (--iCount<0)
		{
		pC->iNState=NThread::EWaitFastSemaphore;
		pC->iWaitObj=this;
		TheScheduler.Remove(pC);
		RescheduleNeeded();
		}
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
	if (++iCount<=0)
		{
		iOwningThread->iWaitObj=NULL;
		iOwningThread->CheckSuspendThenReady();
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
	if (aCount>0 && iCount<0)
		{
		iOwningThread->iWaitObj=NULL;
		iOwningThread->CheckSuspendThenReady();
		}
	iCount+=aCount;
	}


/** Resets a fast semaphore.

	@pre Kernel must be locked.
	@pre Call either in a thread or an IDFC context.
	
	@post Kernel is locked.

	@internalComponent	
 */
EXPORT_C void NFastSemaphore::Reset()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR,"NFastSemaphore::Reset");			
	if (iCount<0)
		{
		iOwningThread->iWaitObj=NULL;
		iOwningThread->CheckSuspendThenReady();
		}
	iCount=0;
	}


/** Cancels a wait on a fast semaphore.

	@pre Kernel must be locked.
	@pre Call either in a thread or an IDFC context.
	
	@post Kernel is locked.

	@internalComponent	
 */
void NFastSemaphore::WaitCancel()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR,"NFastSemaphore::WaitCancel");			
	iCount=0;
	iOwningThread->iWaitObj=NULL;
	iOwningThread->CheckSuspendThenReady();
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
	NThreadBase* pC=TheScheduler.iCurrentThread;
	NKern::Lock();
	pC->iRequestSemaphore.Wait();
	NKern::Unlock();
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"NKern::WaitForAnyRequest");
	}
#endif


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

/** Waits on a fast semaphore.

    Decrements the signal count for the semaphore
	and waits for a signal if the sempahore becomes unsignalled. Only the
	thread that owns a fast	semaphore can wait on it.

	@param aSem The semaphore to wait on.
	
	@pre The calling thread must own the semaphore.
	@pre No fast mutex can be held.
	
	@see NFastSemaphore::Wait()
*/
EXPORT_C void NKern::FSWait(NFastSemaphore* aSem)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::FSWait %m",aSem));
	NKern::Lock();
	aSem->Wait();
	NKern::Unlock();
	}


/** Signals a fast semaphore.

    Increments the signal count of a fast semaphore
	by one and releases any	waiting thread if the semphore becomes signalled.
	
	@param aSem The semaphore to signal.

	@see NKern::FSWait()

	@pre Interrupts must be enabled.
	@pre Do not call from an ISR
 */
EXPORT_C void NKern::FSSignal(NFastSemaphore* aSem)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR,"NKern::FSSignal(NFastSemaphore*)");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::FSSignal %m",aSem));
	NKern::Lock();
	aSem->Signal();
	NKern::Unlock();
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR,"NKern::FSSignal(NFastSemaphore*)");
	}


/** Atomically signals a fast semaphore and releases a fast mutex.

	Rescheduling only occurs after both synchronisation operations are complete.
	
	@param aSem The semaphore to signal.
	@param aMutex The mutex to release. If NULL, the System Lock is released

	@pre The calling thread must hold the mutex.
	
	@see NKern::FMSignal()
 */
EXPORT_C void NKern::FSSignal(NFastSemaphore* aSem, NFastMutex* aMutex)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR,"NKern::FSSignal(NFastSemaphore*, NFastMutex*)");
	if (!aMutex)
		aMutex=&TheScheduler.iLock;
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::FSSignal %m +FM %M",aSem,aMutex));
	NKern::Lock();
	aSem->Signal();
	aMutex->Signal();
	NKern::Unlock();
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR,"NKern::FSSignal(NFastSemaphore*, NFastMutex*)");
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
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::FSSignalN %m %d",aSem,aCount));
	NKern::Lock();
	aSem->SignalN(aCount);
	NKern::Unlock();
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR,"NKern::FSSignalN(NFastSemaphore*, TInt)");
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
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR,"NKern::FSSignalN(NFastSemaphore*, TInt, NFastMutex*)");
	if (!aMutex)
		aMutex=&TheScheduler.iLock;
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::FSSignalN %m %d + FM %M",aSem,aCount,aMutex));
	NKern::Lock();
	aSem->SignalN(aCount);
	aMutex->Signal();
	NKern::Unlock();
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR,"NKern::FSSignalN(NFastSemaphore*, TInt, NFastMutex*)");
	}


/******************************************************************************
 * Thread
 ******************************************************************************/

#ifndef __SCHEDULER_MACHINE_CODED__
/** Makes a nanothread ready provided that it is not explicitly suspended.
	
	For use by RTOS personality layers.

	@pre	Kernel must be locked.
	@pre	Call either in a thread or an IDFC context.

	@post	Kernel is locked.
 */
EXPORT_C void NThreadBase::CheckSuspendThenReady()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR,"NThreadBase::CheckSuspendThenReady");	
	if (iSuspendCount==0)
		Ready();
	else
		iNState=ESuspended;
	}

/** Makes a nanothread ready.
	
	For use by RTOS personality layers.

	@pre	Kernel must be locked.
	@pre	Call either in a thread or an IDFC context.
	@pre	The thread being made ready must not be explicitly suspended
	
	@post	Kernel is locked.
 */
EXPORT_C void NThreadBase::Ready()
	{
#ifdef _DEBUG
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR,"NThreadBase::Ready");	
	__ASSERT_WITH_MESSAGE_DEBUG(iSuspendCount==0,"The thread being made ready must not be explicitly suspended","NThreadBase::Ready");

	if (DEBUGNUM(KCRAZYSCHEDDELAY) && iPriority && TheTimerQ.iMsCount)
		{
		// Delay this thread, unless it's already on the delayed queue
		if ((i_ThrdAttr & KThreadAttDelayed) == 0)
			{
			i_ThrdAttr |= KThreadAttDelayed;
			TheScheduler.iDelayedQ.Add(this);
			}
		}
	else
		{
		// Delayed scheduler off
		// or idle thread, or the tick hasn't started yet
		DoReady();
		}
#else
	DoReady();
#endif
	}

void NThreadBase::DoReady()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR,"NThreadBase::DoReady");	
	__ASSERT_WITH_MESSAGE_DEBUG(iSuspendCount==0,"The thread being made ready must not be explicitly suspended","NThreadBase::DoReady");

	TScheduler& s=TheScheduler;
	TInt p=iPriority;
//	__KTRACE_OPT(KSCHED,Kern::Printf("Ready(%O), priority %d status %d",this,p,iStatus));
	if (iNState==EDead)
		return;
	s.Add(this);
	iNState=EReady;
	if (!(s>p))	// s>p <=> highest ready priority > our priority so no preemption
		{
		// if no other thread at this priority or first thread at this priority has used its timeslice, reschedule
		// note iNext points to first thread at this priority since we got added to the end
		if (iNext==this || ((NThreadBase*)iNext)->iTime==0)
			RescheduleNeeded();
		}
	}
#endif

void NThreadBase::DoCsFunction()
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NThreadBase::DoCsFunction %T %d",this,iCsFunction));
	TInt f=iCsFunction;
	iCsFunction=0;
	if (f>0)
		{
		// suspend this thread f times
		Suspend(f);
		return;
		}
	if (f==ECSExitPending)
		{
		// We need to exit now
		Exit();	// this won't return
		}
	UnknownState(ELeaveCS,f);	// call into RTOS personality
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
	// If thread is executing a critical section, we must defer the suspend
	if (iNState==EDead)
		return FALSE;		// already dead so suspension is a no-op
	if (iCsCount || iHeldFastMutex)
		{
		__KTRACE_OPT(KNKERN,DEBUGPRINT("NThreadBase::Suspend %T (CSF %d) %d",this,iCsFunction,aCount));
		if (iCsFunction>=0)			// -ve means thread is about to exit
			{
			iCsFunction+=aCount;	// so thread will suspend itself when it leaves the critical section
			if (iHeldFastMutex && iCsCount==0)
				iHeldFastMutex->iWaiting=1;
			}
		return FALSE;
		}

	// thread not in critical section, so suspend it
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NThreadBase::Suspend %T (NState %d) %d",this,iNState,aCount));
	switch (iNState)
		{
		case EReady:
			TheScheduler.Remove(this);
			RescheduleNeeded();
			iNState=ESuspended;
		case EWaitFastSemaphore:
		case EWaitDfc:
		case ESleep:
		case EBlocked:
		case ESuspended:
			break;
		default:
			UnknownState(ESuspend,aCount);
			break;
		}
	TInt old_suspend=iSuspendCount;
	iSuspendCount-=aCount;
	return (old_suspend==0);	// return TRUE if thread has changed from not-suspended to suspended.
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
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NThreadBase::Resume %T, state %d CSC %d CSF %d",this,iNState,iCsCount,iCsFunction));
	if (iNState==EDead)
		return FALSE;

	// If thread is in critical section, just cancel deferred suspends
	if (iCsCount || iHeldFastMutex)
		{
		if (iCsFunction>0)
			--iCsFunction;	// one less deferred suspension
		return FALSE;
		}
	if (iSuspendCount<0 && ++iSuspendCount==0)
		{
		switch (iNState)
			{
			case ESuspended:
				Ready();
			case EReady:
			case EWaitFastSemaphore:
			case EWaitDfc:
			case ESleep:
			case EBlocked:
				break;
			default:
				UnknownState(EResume,0);
				break;
			}
		return TRUE;	// thread has changed from suspended to not-suspended
		}
	return FALSE;	// still suspended or not initially suspended so no higher level action required
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
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NThreadBase::ForceResume %T, state %d CSC %d CSF %d",this,iNState,iCsCount,iCsFunction));
	if (iNState==EDead)
		return FALSE;

	// If thread is in critical section, just cancel deferred suspends
	if (iCsCount || iHeldFastMutex)
		{
		if (iCsFunction>0)
			iCsFunction=0;	// cancel all deferred suspensions
		return FALSE;
		}
	if (iSuspendCount<0)
		{
		iSuspendCount=0;
		switch (iNState)
			{
			case ESuspended:
				Ready();
			case EReady:
			case EWaitFastSemaphore:
			case EWaitDfc:
			case ESleep:
			case EBlocked:
			case EDead:
				break;
			default:
				UnknownState(EForceResume,0);
				break;
			}
		}
	return TRUE;
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
EXPORT_C void NThreadBase::Release(TInt aReturnCode)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR,"NThreadBase::Release");		
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NThreadBase::Release %T, state %d retcode %d",this,iNState,aReturnCode));
	switch(iNState)
		{
		case EDead:
			return;
		case EReady:
		case ESuspended:
			// don't release explicit suspensions
			break;
		case EWaitFastSemaphore:
			if (aReturnCode<0 && iWaitObj)
				((NFastSemaphore*)iWaitObj)->WaitCancel();
			break;
		case ESleep:
		case EBlocked:
		case EWaitDfc:
			CheckSuspendThenReady();
			break;
		default:
			UnknownState(ERelease,aReturnCode);
			break;
		}
	if (iTimer.iUserFlags)
		{
		if (iTimer.iState == NTimer::EIdle)
			{
			// Potential race condition - timer must have completed but expiry
			// handler has not yet run. Signal to the handler that it should do
			// nothing by flipping the bottom bit of iTimer.iPtr
			// This condition cannot possibly recur until the expiry handler has
			// run since all expiry handlers run in DfcThread1.
			TLinAddr& x = *(TLinAddr*)&iTimer.iPtr;
			x ^= 1;
			}
		iTimer.Cancel();
		iTimer.iUserFlags = FALSE;
		}
	iWaitObj=NULL;
	iReturnValue=aReturnCode;
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

void NThreadBase::TimerExpired(TAny* aPtr)
	{
	TLinAddr cookie = (TLinAddr)aPtr;
	NThread* pT = (NThread*)(cookie &~ 3);
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NThreadBase::TimerExpired %T, state %d",pT,pT->iNState));
	NThreadTimeoutHandler th = pT->iHandlers->iTimeoutHandler;
	NKern::Lock();
	if (pT->iNState<ENumNStates && pT->iNState!=EBlocked)
		th = NULL;
	if (th)
		{
		// Use higher level timeout handler
		NKern::Unlock();
		(*th)(pT, ETimeoutPreamble);
		TInt param = ETimeoutPostamble;
		NKern::Lock();
		TLinAddr current_cookie = (TLinAddr)pT->iTimer.iPtr;
		if ((cookie ^ current_cookie) & 1)
			{
			// The timer was cancelled just after expiring but before this function
			// managed to call NKern::Lock(), so it's spurious
			param = ETimeoutSpurious;
			}
		else
			pT->iTimer.iUserFlags = FALSE;
		NKern::Unlock();
		(*th)(pT, param);
		return;
		}
	TLinAddr current_cookie = (TLinAddr)pT->iTimer.iPtr;
	if ((cookie ^ current_cookie) & 1)
		{
		// The timer was cancelled just after expiring but before this function
		// managed to call NKern::Lock(), so just return without doing anything.
		NKern::Unlock();
		return;
		}
	pT->iTimer.iUserFlags = FALSE;
	switch(pT->iNState)
		{
		case EDead:
		case EReady:
		case ESuspended:
			NKern::Unlock();
			return;
		case EWaitFastSemaphore:
			((NFastSemaphore*)pT->iWaitObj)->WaitCancel();
			break;
		case EBlocked:
		case ESleep:
		case EWaitDfc:
			pT->CheckSuspendThenReady();
			break;
		default:
			pT->UnknownState(ETimeout,0);
			break;
		}
	pT->iWaitObj=NULL;
	pT->iReturnValue=KErrTimedOut;
	NKern::Unlock();
	}


/** Changes the priority of a nanokernel thread.

	For use by RTOS personality layers.
	Do not use this function directly on a Symbian OS thread.

	The thread's unknown state handler will be invoked with function EChangePriority
	and parameter newp if the current NState is not recognised and the new priority
	is not equal to the original priority.
	
	@param	newp  The new nanokernel priority (0 <= newp < KNumPriorities).

	@pre	Kernel must be locked.
	@pre	Call in a thread context.
	
	@post	Kernel is locked.
 */
EXPORT_C void NThreadBase::SetPriority(TInt newp)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_IDFC|MASK_NOT_ISR,"NThreadBase::SetPriority");		
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NThreadBase::SetPriority %T %d->%d, state %d",this,iPriority,newp,iNState));
#ifdef _DEBUG
	// When the crazy scheduler is active, refuse to set any priority higher than 1
	if (KCrazySchedulerEnabled() && newp>1)
		newp=1;
#endif
	if (newp==iPriority)
		return;
#ifdef BTRACE_THREAD_PRIORITY
	BTrace8(BTrace::EThreadPriority,BTrace::ENThreadPriority,this,newp);
#endif
	switch(iNState)
		{
		case EReady:
			{
			TInt oldp=iPriority;
			TheScheduler.ChangePriority(this,newp);
			NThreadBase* pC=TheScheduler.iCurrentThread;
			if (this==pC)
				{
				if (newp<oldp && (TheScheduler>newp || !TPriListLink::Alone()))	// can't have scheduler<newp
					RescheduleNeeded();
				}
			else if (newp>oldp)
				{
				TInt cp=pC->iPriority;
				if (newp>cp)
					RescheduleNeeded();
				else if (newp==cp && pC->iTime==0)
					{
					if (pC->iHeldFastMutex)
						pC->iHeldFastMutex->iWaiting=1;	// don't round-robin now, wait until fast mutex released
					else
						RescheduleNeeded();
					}
				}
			break;
			}
		case ESuspended:
		case EWaitFastSemaphore:
		case EWaitDfc:
		case ESleep:
		case EBlocked:
		case EDead:
			iPriority=TUint8(newp);
			break;
		default:
			UnknownState(EChangePriority,newp);
			break;
		}
	}

void NThreadBase::Exit()
	{
	// The current thread is exiting
	// Enter with kernel locked, don't return
	__NK_ASSERT_DEBUG(this==TheScheduler.iCurrentThread);

	OnExit();

	TInt threadCS=iCsCount;
	TInt kernCS=TheScheduler.iKernCSLocked;
	iCsCount=1;
	iCsFunction=ECSExitInProgress;
	NKern::Unlock();
	__KTRACE_OPT(KSCHED,DEBUGPRINT("Exit %T %u",this,NTickCount()));
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NThreadBase::Exit %T, CSC %d HeldFM %M KernCS %d",this,threadCS,iHeldFastMutex,kernCS));
	if (kernCS!=1)
		FAULT();
	if (iHeldFastMutex)
		FAULT();
	if (threadCS)
		FAULT();
	TDfc* pD=NULL;
	NThreadExitHandler xh = iHandlers->iExitHandler;
	if (xh)
		pD=(*xh)((NThread*)this);		// call exit handler
	NKern::Lock();
	if (pD)
		pD->DoEnque();
	iNState=EDead;
	TheScheduler.Remove(this);
	RescheduleNeeded();
#ifdef BTRACE_THREAD_IDENTIFICATION
	BTrace4(BTrace::EThreadIdentification,BTrace::ENanoThreadDestroy,this);
#endif
	__NK_ASSERT_ALWAYS(iCsFunction == ECSExitInProgress);
	TScheduler::Reschedule();	// this won't return
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
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NThreadBase::Kill %T, state %d CSC %d HeldFM %M",this,iNState,iCsCount,iHeldFastMutex));
	OnKill(); // platform-specific hook
	NThreadBase* pC=TheScheduler.iCurrentThread;
	if (this==pC)
		{
		if (iCsFunction==ECSExitInProgress)
			FAULT();
		Exit();				// this will not return
		}
	if (iCsCount || iHeldFastMutex)
		{
		__KTRACE_OPT(KNKERN,DEBUGPRINT("NThreadBase::Kill %T deferred",this));
		if (iCsFunction<0)
			return;			// thread is already exiting
		iCsFunction=ECSExitPending;		// zap any suspensions pending
		if (iHeldFastMutex && iCsCount==0)
			iHeldFastMutex->iWaiting=1;
		return;
		}

	// thread is not in critical section
	// make the thread divert to Exit() when it next runs
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NThreadBase::Kill diverting %T",this));
	Release(KErrDied);		// cancel any waits on semaphores etc.
	ForceResume();			// release any suspensions
	iWaitFastMutex=NULL;	// if thread was waiting for a fast mutex it needn't bother
	iCsCount=1;				// stop anyone suspending the thread
	iCsFunction=ECSExitPending;
	ForceExit();			// get thread to call Exit when it is next scheduled
	}


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
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::ThreadResume %T + FM %M",aThread,aMutex));
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
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::ThreadForceResume %T + FM %M",aThread,aMutex));
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
	aThread->Release(aReturnValue);
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
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::ThreadRelease %T ret %d + FM %M",aThread,aReturnValue,aMutex));
	NKern::Lock();
	aThread->Release(aReturnValue);
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
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::ThreadSetPriority %T->%d + FM %M",aThread,aPriority,aMutex));
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
void NKern::ThreadSetNominalPriority(NThread* /*aThread*/, TInt /*aPriority*/)
	{
	}


#ifndef __SCHEDULER_MACHINE_CODED__

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
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR,"NKern::ThreadRequestSignal(NThread*)");
	NKern::Lock();
	aThread->iRequestSemaphore.Signal();
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
		aMutex=&TheScheduler.iLock;
	NKern::Lock();
	aThread->iRequestSemaphore.Signal();
	aMutex->Signal();
	NKern::Unlock();
	}
#endif


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
		aThread=(NThread*)TheScheduler.iCurrentThread;
	NKern::Lock();
	aThread->iRequestSemaphore.SignalN(aCount);
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
		aMutex=&TheScheduler.iLock;
	NThreadBase* pC=TheScheduler.iCurrentThread;
	NKern::Lock();
	if (aThread==pC)
		{
		__NK_ASSERT_DEBUG(pC->iCsCount==0);	// Make sure thread isn't in critical section
		aThread->iCsFunction=NThreadBase::ECSExitPending;
		aMutex->iWaiting=1;
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
	NThreadBase* pC=TheScheduler.iCurrentThread;
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::ThreadEnterCS %T",pC));
	__NK_ASSERT_DEBUG(pC->iCsCount>=0);
	++pC->iCsCount;
	}


NThread* NKern::_ThreadEnterCS()
	{
	NThread* pC = (NThread*)TheScheduler.iCurrentThread;
	__NK_ASSERT_DEBUG(pC->iCsCount>=0);
	++pC->iCsCount;
	return pC;
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
	NThreadBase* pC=TheScheduler.iCurrentThread;
	NKern::Lock();
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::ThreadLeaveCS %T",pC));
	__NK_ASSERT_DEBUG(pC->iCsCount>0);
	if (--pC->iCsCount==0 && pC->iCsFunction!=0)
		{
		if (pC->iHeldFastMutex)
			pC->iHeldFastMutex->iWaiting=1;
		else
			pC->DoCsFunction();
		}
	NKern::Unlock();
	}

void NKern::_ThreadLeaveCS()
	{
	NThreadBase* pC=TheScheduler.iCurrentThread;
	NKern::Lock();
	__NK_ASSERT_DEBUG(pC->iCsCount>0);
	if (--pC->iCsCount==0 && pC->iCsFunction!=0)
		{
		if (pC->iHeldFastMutex)
			pC->iHeldFastMutex->iWaiting=1;
		else
			pC->DoCsFunction();
		}
	NKern::Unlock();
	}

/** Freeze the CPU of the current thread

	After this the current thread will not migrate to another processor

	On uniprocessor builds does nothing and returns 0

	@return	A cookie to be passed to NKern::EndFreezeCpu() to allow nesting
*/
EXPORT_C TInt NKern::FreezeCpu()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::FreezeCpu");
	return 0;
	}


/** Unfreeze the current thread's CPU

	After this the current thread will again be eligible to migrate to another processor

	On uniprocessor builds does nothing

	@param	aCookie the value returned by NKern::FreezeCpu()
*/
EXPORT_C void NKern::EndFreezeCpu(TInt /*aCookie*/)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::EndFreezeCpu");
	}


/** Change the CPU affinity of a thread

	On uniprocessor builds does nothing

	@pre	Call in a thread context.

	@param	The new CPU affinity mask
	@return The old affinity mask
 */
EXPORT_C TUint32 NKern::ThreadSetCpuAffinity(NThread*, TUint32)
	{
	return 0;	// lock to processor 0
	}


/** Modify a thread's timeslice

	@pre	Call in a thread context.

	@param	aTimeslice	The new timeslice value
 */
EXPORT_C void NKern::ThreadSetTimeslice(NThread* aThread, TInt aTimeslice)
	{
	NKern::Lock();
	if (aThread->iTimeslice == aThread->iTime || aTimeslice<0)
		aThread->iTime = aTimeslice;
	aThread->iTimeslice = aTimeslice;
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
		aMutex=&TheScheduler.iLock;
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::Block time %d mode %d FM %M",aTimeout,aMode,aMutex));
	NThreadBase* pC=TheScheduler.iCurrentThread;
	pC->iReturnValue=0;
	NKern::Lock();
	if (aMode & EEnterCS)
		++pC->iCsCount;
	if (aMode & ERelease)
		{
#ifdef BTRACE_FAST_MUTEX
		BTraceContext4(BTrace::EFastMutex,BTrace::EFastMutexSignal,aMutex);
#endif
		aMutex->iHoldingThread=NULL;
		TBool w=aMutex->iWaiting;
		aMutex->iWaiting=0;
		pC->iHeldFastMutex=NULL;
		if (w && !pC->iCsCount && pC->iCsFunction)
			pC->DoCsFunction();
		}
	RescheduleNeeded();
	if (aTimeout)
		{
		pC->iTimer.iUserFlags = TRUE;
		pC->iTimer.OneShot(aTimeout,TRUE);
		}
	if (pC->iNState==NThread::EReady)
		TheScheduler.Remove(pC);
	pC->iNState=NThread::EBlocked;
	NKern::Unlock();
	if (aMode & EClaim)
		FMWait(aMutex);
	return pC->iReturnValue;
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
	NThreadBase* pC=TheScheduler.iCurrentThread;
	pC->iReturnValue=0;
	NKern::Lock();
	if (aMode & EEnterCS)
		++pC->iCsCount;
	RescheduleNeeded();
	if (aTimeout)
		{
		pC->iTimer.iUserFlags = TRUE;
		pC->iTimer.OneShot(aTimeout,TRUE);
		}
	pC->iNState=NThread::EBlocked;
	TheScheduler.Remove(pC);
	NKern::Unlock();
	return pC->iReturnValue;
	}




EXPORT_C void NKern::NanoBlock(TUint32 aTimeout, TUint aState, TAny* aWaitObj)
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
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::NanoBlock");		
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::NanoBlock time %d state %d obj %08x", aTimeout, aState, aWaitObj));
	NThreadBase* pC=TheScheduler.iCurrentThread;
	if (aTimeout)
		{
		pC->iTimer.iUserFlags = TRUE;
		pC->iTimer.OneShot(aTimeout,TRUE);
		}
	pC->iNState = (TUint8)aState;
	pC->iWaitObj = aWaitObj;
	pC->iReturnValue = 0;
	TheScheduler.Remove(pC);
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
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::Sleep %d",aTime));
	NThreadBase* pC=TheScheduler.iCurrentThread;
	NKern::Lock();
	pC->iTimer.iUserFlags = TRUE;
	pC->iTimer.OneShot(aTime,TRUE);
	pC->iNState=NThread::ESleep;
	TheScheduler.Remove(pC);
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
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::Exit"));
	NThreadBase* pC=TheScheduler.iCurrentThread;
	NKern::Lock();
	pC->Exit();			// this won't return
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
	if (pC->iCsFunction >= 0)	// don't touch it if we are already exiting
		pC->iCsFunction = NThreadBase::ECSExitPending;
	if (m && !pC->iCsCount)
		m->iWaiting = TRUE;
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
	if (t->iNext != t)
		RescheduleNeeded();
	NKern::Unlock();
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
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::RotateReadyList");		
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKern::RotateReadyList %d",aPriority));
	if (aPriority<0 || aPriority>=KNumPriorities)
		aPriority=TheScheduler.iCurrentThread->iPriority;
	NKern::Lock();
	TheScheduler.RotateReadyList(aPriority);
	NKern::Unlock();
	}

/** Rotates the ready list for threads at the specified priority.
	
	For use by RTOS personality layers to allow external control of round-robin
	scheduling. Not intended for direct use by device drivers.

	@param	aPriority = priority at which threads should be rotated.
						-1 means use calling thread's priority.
	@param	aCpu = which CPU's ready list should be rotated
					ignored on UP systems.
	
	@pre	Kernel must be unlocked.
	@pre	Call in a thread context.
	
	@post	Kernel is unlocked.
 */
EXPORT_C void NKern::RotateReadyList(TInt aPriority, TInt /*aCpu*/)
	{
	RotateReadyList(aPriority);
	}


/** Returns the NThread control block for the currently scheduled thread.

    Note that this is the calling thread if called from a thread context, or the
	interrupted thread if called from an interrupt context.
	
	@return A pointer to the NThread for the currently scheduled thread.
	
	@pre Call in any context.
*/
EXPORT_C NThread* NKern::CurrentThread()
	{
	return (NThread*)TheScheduler.iCurrentThread;
	}


/** Returns the CPU number of the calling CPU.

	@return the CPU number of the calling CPU.
	
	@pre Call in any context.
*/
EXPORT_C TInt NKern::CurrentCpu()
	{
	return 0;
	}


/** Returns the number of CPUs available to Symbian OS

	@return the number of CPUs
	
	@pre Call in any context.
*/
EXPORT_C TInt NKern::NumberOfCpus()
	{
	return 1;
	}


/** Check if the kernel is locked the specified number of times.

	@param aCount	The number of times the kernel should be locked
					If zero, tests if it is locked at all
	@return TRUE if the tested condition is true.

	@internalTechnology
*/
EXPORT_C TBool NKern::KernelLocked(TInt aCount)
	{
	if (aCount)
		return TheScheduler.iKernCSLocked == aCount;
	return TheScheduler.iKernCSLocked!=0;
	}


/******************************************************************************
 * Priority lists
 ******************************************************************************/

#ifndef __PRI_LIST_MACHINE_CODED__
/** Returns the priority of the highest priority item present on a priority list.

	@return	The highest priority present or -1 if the list is empty.
 */
EXPORT_C TInt TPriListBase::HighestPriority()
	{
//	TUint64 present = MAKE_TUINT64(iPresent[1], iPresent[0]);
//	return __e32_find_ms1_64(present);
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


/** Adds an item to a priority list.

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


