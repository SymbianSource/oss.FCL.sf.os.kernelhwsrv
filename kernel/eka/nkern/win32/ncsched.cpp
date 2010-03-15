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
// e32\nkern\win32\ncsched.cpp
// 
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

#include <e32cmn.h>
#include <e32cmn_private.h>
#include "nk_priv.h"

#ifdef __EMI_SUPPORT__
extern void EMI_AddTaskSwitchEvent(TAny* aPrevious, TAny* aNext);
extern void EMI_CheckDfcTag(TAny* aNext);
#endif
typedef void (*ProcessHandler)(TAny* aAddressSpace);

static DWORD TlsIndex = TLS_OUT_OF_INDEXES;

static NThreadBase* SelectThread(TScheduler& aS)
//
// Select the next thread to run.
// This is the heart of the rescheduling algorithm.
//
	{
	NThreadBase* t = static_cast<NThreadBase*>(aS.First());
	__NK_ASSERT_DEBUG(t);
#ifdef _DEBUG
	if (t->iHeldFastMutex)
		{
		__KTRACE_OPT(KSCHED2,DEBUGPRINT("Resched init->%T, Holding %M",t,t->iHeldFastMutex));
		}
	else
		{
		__KTRACE_OPT(KSCHED2,DEBUGPRINT("Resched init->%T",t));
		}
#endif
	if (t->iTime == 0 && !t->Alone())
		{
		// round robin
		// get here if thread's timeslice has expired and there is another
		// thread ready at the same priority
		if (t->iHeldFastMutex)
			{
			// round-robin deferred due to fast mutex held
			t->iHeldFastMutex->iWaiting = 1;
			return t;
			}
		t->iTime = t->iTimeslice;		// reset old thread time slice
		t = static_cast<NThreadBase*>(t->iNext);					// next thread
		aS.iQueue[t->iPriority] = t;		// make it first in list
		__KTRACE_OPT(KSCHED2,DEBUGPRINT("RoundRobin->%T",t));
		}
	if (t->iHeldFastMutex)
		{
		if (t->iHeldFastMutex == &aS.iLock)
			{
			// thread holds system lock: use it
			return t;
			}
		if ((t->i_ThrdAttr & KThreadAttImplicitSystemLock) != 0 && aS.iLock.iHoldingThread)
			t->iHeldFastMutex->iWaiting = 1;
		__NK_ASSERT_DEBUG((t->i_ThrdAttr & KThreadAttAddressSpace) == 0);
/*
		Check for an address space change. Not implemented for Win32, but useful as
		documentaiton of the algorithm.

		if ((t->i_ThrdAttr & KThreadAttAddressSpace) != 0 && t->iAddressSpace != aS.iAddressSpace)
			t->iHeldFastMutex->iWaiting = 1;
*/
		}
	else if (t->iWaitFastMutex && t->iWaitFastMutex->iHoldingThread)
		{
		__KTRACE_OPT(KSCHED2,DEBUGPRINT("Resched inter->%T, Blocked on %M",t->iWaitFastMutex->iHoldingThread,t->iWaitFastMutex));
		t = t->iWaitFastMutex->iHoldingThread;
		}
	else if (t->i_ThrdAttr & KThreadAttImplicitSystemLock)
		{
		// implicit system lock required
		if (aS.iLock.iHoldingThread)
			{
			// system lock held, switch to that thread
			t = aS.iLock.iHoldingThread;
			__KTRACE_OPT(KSCHED2,DEBUGPRINT("Resched inter->%T (IMP SYS)",t));
			t->iHeldFastMutex->iWaiting = 1;	// aS.iLock.iWaiting = 1;
			return t;
			}
		__NK_ASSERT_DEBUG((t->i_ThrdAttr & KThreadAttAddressSpace) == 0);
/*
		Check for an address space change. Not implemented for Win32, but useful as
		documentaiton of the algorithm.

		if ((t->i_ThrdAttr & KThreadAttAddressSpace) != 0 || t->iAddressSpace != aS.iAddressSpace)
			{
			// what do we do now?
			__NK_ASSERT_DEBUG(FALSE);
			}
*/
		}
	return t;
	}

// from NThread
#undef i_ThrdAttr

TBool NThread::WakeUp()
//
// Wake up the thread. What to do depends on whether we were preempted or voluntarily
// rescheduled.
//
// Return TRUE if we need to immediately reschedule again because we had to unlock
// the kernel but there are DFCs pending. In this case, the thread does not wake up.
//
// NB. kernel is locked
//
	{
	switch (iWakeup)
		{
	default:
		FAULT();
	case EIdle:
		__NK_ASSERT_ALWAYS(TheScheduler.iCurrentThread == this);
		__NK_ASSERT_ALWAYS(SetEvent(iScheduleLock));
		break;
	case ERelease:
		TheScheduler.iCurrentThread = this;
		__NK_ASSERT_ALWAYS(SetEvent(iScheduleLock));
		break;
	case EResumeLocked:
		// The thread is Win32 suspended and must be resumed.
		//
		// A newly created thread does not need the kernel unlocked so we can
		// just resume the suspended thread
		//
		__KTRACE_OPT(KSCHED,DEBUGPRINT("Win32Resume->%T",this));
		iWakeup = ERelease;
		TheScheduler.iCurrentThread = this;
		if (TheScheduler.iProcessHandler)
			(*ProcessHandler(TheScheduler.iProcessHandler))(iAddressSpace); // new thread will need to have its static data updated
		__NK_ASSERT_ALWAYS(TInt(ResumeThread(iWinThread)) > 0);	// check thread was previously suspended
		break;
	case EResumeDiverted:
		// The thread is Win32 suspended and must be resumed.
		//
		// The thread needs to be diverted, and does not need the kernel
		// unlocked.
		//
		// It's safe the divert the thread here because we called
		// IsSafeToPreempt() when we suspended it - otherwise the diversion
		// could get lost.
		//
		__KTRACE_OPT(KSCHED,DEBUGPRINT("Win32Resume->%T (Resuming diverted thread)",this));
		iWakeup = ERelease;
		ApplyDiversion();
		TheScheduler.iCurrentThread = this;
		__NK_ASSERT_ALWAYS(TInt(ResumeThread(iWinThread)) == 1);
		break;
	case EResume:
		// The thread is Win32 suspended and must be resumed.
		//
		// the complication here is that we have to unlock the kernel on behalf of the
		// pre-empted thread. This means that we have to check to see if there are more DFCs
		// pending or a reschedule required, as we unlock the kernel. That check is
		// carried out with interrupts disabled.
		//
		// If so, we go back around the loop in this thread context
		//
		// Otherwise, we unlock the kernel (having marked us as not-preempted),
		// enable interrupts and then resume the thread. If pre-emption occurs before the thread
		// is resumed, it is the new thread that is pre-empted, not the running thread, so we are guaranteed
		// to be able to call ResumeThread. If pre-emption occurs, and we are rescheduled to run before
		// that occurs, we will once again be running with the kernel locked and the other thread will
		// have been re-suspended by Win32: so all is well.
		//		
		{
		__KTRACE_OPT(KSCHED,DEBUGPRINT("Win32Resume->%T",this));
		TInt irq = NKern::DisableAllInterrupts();
		if (TheScheduler.iDfcPendingFlag || TheScheduler.iRescheduleNeededFlag)
			{
			// we were interrrupted... back to the top
			TheScheduler.iRescheduleNeededFlag = TRUE;	// ensure we do the reschedule
			return TRUE;
			}
		iWakeup = ERelease;
		TheScheduler.iCurrentThread = this;
		if (TheScheduler.iProcessHandler)
			(*ProcessHandler(TheScheduler.iProcessHandler))(iAddressSpace); // threads resumed after interrupt or locks need to have static data updated

		if (iInKernel == 0 && iUserModeCallbacks != NULL)
			ApplyDiversion();
		else 
			TheScheduler.iKernCSLocked = 0;		// have to unlock the kernel on behalf of the new thread
		
		TheScheduler.iCurrentThread = this;
		NKern::RestoreInterrupts(irq);
		__NK_ASSERT_ALWAYS(TInt(ResumeThread(iWinThread)) > 0);	// check thread was previously suspended
		}
		break;
		}
	return FALSE;
	}

static void ThreadExit(NThread& aCurrent, NThread& aNext)
//
// The final context switch of a thread.
// Wake up the next thread and then destroy this one's Win32 resources.
//
// Return without terminating if we need to immediately reschedule again because
// we had to unlock the kernel but there are DFCs pending.
//
	{
	// the thread is dead
	// extract win32 handles from dying NThread object before rescheduling
	HANDLE sl = aCurrent.iScheduleLock;
	HANDLE th = aCurrent.iWinThread;

	// wake up the next thread
	if (aNext.WakeUp())
		return;			// need to re-reschedule in this thread

	// we are now a vanilla win32 thread, nKern no longer knows about us
	// release resources and exit cleanly
	CloseHandle(sl);
	CloseHandle(th);
	ExitThread(0);		// does not return
	}

#ifdef MONITOR_THREAD_CPU_TIME
static inline void UpdateThreadCpuTime(NThread& aCurrent, NThread& aNext)
	{	
	TUint32 timestamp = NKern::FastCounter();
	if (aCurrent.iLastStartTime)
		aCurrent.iTotalCpuTime += timestamp - aCurrent.iLastStartTime;
	aNext.iLastStartTime = timestamp;
	}
#else
static inline void UpdateThreadCpuTime(NThread& /*aCurrent*/, NThread& /*aNext*/)
	{	
	}
#endif

static void SwitchThreads(NThread& aCurrent, NThread& aNext)
//
// The fundamental context switch - wake up the next thread and wait for reschedule
// trivially is aNext.WakeUp(), Wait(aCurrent.iScheduleLock), but we may be able to
// optimise the signal-and-wait
//
	{
	UpdateThreadCpuTime(aCurrent, aNext);
	if (aCurrent.iNState == NThread::EDead)
		ThreadExit(aCurrent, aNext);
	else if (Win32AtomicSOAW && aNext.iWakeup==NThread::ERelease)
		{
		// special case optimization for normally blocked threads using atomic Win32 primitive
		TheScheduler.iCurrentThread = &aNext;
		DWORD result=SignalObjectAndWait(aNext.iScheduleLock,aCurrent.iScheduleLock, INFINITE, FALSE);
		if (result != WAIT_OBJECT_0)
			{
			__NK_ASSERT_ALWAYS(result == 0xFFFFFFFF);
			KPrintf("SignalObjectAndWait() failed with %d (%T->%T)",GetLastError(),&aCurrent,&aNext);
			FAULT();
			}
		}
	else
		{
		if (aNext.WakeUp())
			return;			// need to re-reschedule in this thread
		__NK_ASSERT_ALWAYS(WaitForSingleObject(aCurrent.iScheduleLock, INFINITE) == WAIT_OBJECT_0);
		}
	}

void TScheduler::YieldTo(NThreadBase*)
//
// Directed context switch to the nominated thread.
// Enter with kernel locked, exit with kernel unlocked but interrupts disabled.
//
	{
	RescheduleNeeded();
	TScheduler::Reschedule();
	}

void TScheduler::Reschedule()
//
// Enter with kernel locked, exit with kernel unlocked, interrupts disabled.
// If the thread is dead do not return, but terminate the thread.
//
	{
	__NK_ASSERT_ALWAYS(TheScheduler.iKernCSLocked == 1);
	NThread& me = *static_cast<NThread*>(TheScheduler.iCurrentThread);
	for (;;)
		{
		NKern::DisableAllInterrupts();
		if (TheScheduler.iDfcPendingFlag)
			TheScheduler.QueueDfcs();
		if (!TheScheduler.iRescheduleNeededFlag)
			break;
		NKern::EnableAllInterrupts();
		TheScheduler.iRescheduleNeededFlag = FALSE;
		NThread* t = static_cast<NThread*>(SelectThread(TheScheduler));
		__KTRACE_OPT(KSCHED,DEBUGPRINT("Reschedule->%T (%08x%08x)",t,TheScheduler.iPresent[1],TheScheduler.iPresent[0]));
#ifdef __EMI_SUPPORT__
		EMI_AddTaskSwitchEvent(&me,t);
		EMI_CheckDfcTag(t);
#endif
#ifdef BTRACE_CPU_USAGE
		if(TheScheduler.iCpuUsageFilter)
			TheScheduler.iBTraceHandler(BTRACE_HEADER_C(4,BTrace::ECpuUsage,BTrace::ENewThreadContext),0,(TUint32)t,0,0,0,0,0);
#endif
		SwitchThreads(me, *t);

		// we have just been scheduled to run... check for diversion/new Dfcs
		NThread::TDivert divert = me.iDivert;
		if (divert)
			{
			// diversion (e.g. force exit)
			me.iDivert = NULL;
			divert();						// does not return
			}
		}
	if (TheScheduler.iProcessHandler)
		(*ProcessHandler(TheScheduler.iProcessHandler))(me.iAddressSpace);
	// interrrupts are disabled, the kernel is still locked
	TheScheduler.iKernCSLocked = 0;
	}

/**	Put the emulator into 'idle'.
	This is called by the idle thread when there is nothing else to do.

	@internalTechnology
 */
EXPORT_C void NThread::Idle()
//
// Rather than spin, we go to sleep on the schedule lock. Preemption detects
// this state (Win32Idling) and pokes the event rather than diverting the thread.
//
// enter and exit with kernel locked
//
	{
	NThread& me = *static_cast<NThread*>(TheScheduler.iCurrentThread);
	me.iWakeup = EIdle;
	__NK_ASSERT_ALWAYS(WaitForSingleObject(me.iScheduleLock, INFINITE) == WAIT_OBJECT_0);
	// something happened, and we've been prodded by an interrupt
	// the kernel was locked by the interrupt, and now reschedule
	me.iWakeup = ERelease;
	TScheduler::Reschedule();
	NKern::EnableAllInterrupts();
	}

void SchedulerInit(NThread& aInit)
//
// Initialise the win32 nKern scheduler
//
	{
	DWORD procaffin,sysaffin;
	if (GetProcessAffinityMask(GetCurrentProcess(),&procaffin,&sysaffin))
		{
		DWORD cpu;
		switch (Win32SingleCpu)
			{
		default:
			// bind the emulator to a nominated CPU on the host PC
			cpu = (1<<Win32SingleCpu);
			if (!(sysaffin & cpu))
				cpu = procaffin;	// CPU selection invalid
			break;
		case NThread::ECpuSingle:
			// bind the emulator to a single CPU on the host PC, pick one
			cpu = procaffin ^ (procaffin & (procaffin-1));
			break;
		case NThread::ECpuAll:
			// run the emulator on all CPUs on the host PC
			cpu=sysaffin;
			break;
			}
		SetProcessAffinityMask(GetCurrentProcess(), cpu);
		}
	// identify if we can use the atomic SignalObjectAndWait API in Win32 for rescheduling
	Win32AtomicSOAW = (SignalObjectAndWait(aInit.iScheduleLock, aInit.iScheduleLock, INFINITE, FALSE) == WAIT_OBJECT_0);
	//
	// allocate the TLS used for thread identification, and set it for the init thread
	TlsIndex = TlsAlloc();
	__NK_ASSERT_ALWAYS(TlsIndex != TLS_OUT_OF_INDEXES);
	SchedulerRegister(aInit);
	//
	Interrupt.Init();

	Win32FindNonPreemptibleFunctions();
	}

void SchedulerRegister(NThread& aSelf)
	{
	TlsSetValue(TlsIndex,&aSelf);
	}

NThread* SchedulerThread()
	{
	if (TlsIndex != TLS_OUT_OF_INDEXES)
		return static_cast<NThread*>(TlsGetValue(TlsIndex));
	else
		return NULL;  // not yet initialised
	}

inline TBool IsScheduledThread()
	{
	return SchedulerThread() == TheScheduler.iCurrentThread;
	}
	
NThread& CheckedCurrentThread()
	{
	NThread* t = SchedulerThread();
	__NK_ASSERT_ALWAYS(t == TheScheduler.iCurrentThread);
	return *t;
	}


/**	Disable normal 'interrupts'.

	@param	aLevel Ignored
	@return	Cookie to be passed into RestoreInterrupts()
 */
EXPORT_C TInt NKern::DisableInterrupts(TInt /*aLevel*/)
	{
	return Interrupt.Mask();
	}


/**	Disable all maskable 'interrupts'.

	@return	Cookie to be passed into RestoreInterrupts()
 */
EXPORT_C TInt NKern::DisableAllInterrupts()
	{
	return Interrupt.Mask();
	}


/**	Enable all maskable 'interrupts'

	@internalComponent
 */
EXPORT_C void NKern::EnableAllInterrupts()
	{
	Interrupt.Restore(0);
	}


/** Restore interrupt mask to state preceding a DisableInterrupts() call

	@param	aLevel Cookie returned by Disable(All)Interrupts()
 */
EXPORT_C void NKern::RestoreInterrupts(TInt aLevel)
	{
	Interrupt.Restore(aLevel);
	}


/**	Unlocks the kernel.

	Decrements iKernCSLocked; if it becomes zero and IDFCs or a reschedule are
	pending, calls the scheduler to process them.

    @pre    Call either in a thread or an IDFC context.
    @pre    Do not call from an ISR.
	@pre	Do not call from bare Win32 threads.
 */
EXPORT_C void NKern::Unlock()
//
// using this coding sequence it is possible to call Reschedule unnecessarily
// if we are preempted after testing the flags (lock is zero at this point).
// However, in the common case this is much faster because 'disabling interrupts'
// can be very expensive.
//
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR,"NKern::Unlock");	
	__ASSERT_WITH_MESSAGE_DEBUG(IsScheduledThread(),"Do not call from bare Win32 threads","NKern::Unlock");	// check that we are a scheduled thread
	__NK_ASSERT_ALWAYS(TheScheduler.iKernCSLocked > 0);	// Can't unlock if it isn't locked!
	if (--TheScheduler.iKernCSLocked == 0)
		{
		if (TheScheduler.iRescheduleNeededFlag || TheScheduler.iDfcPendingFlag)
			{
			TheScheduler.iKernCSLocked = 1;
			TScheduler::Reschedule();
			NKern::EnableAllInterrupts();
			}
		}
	}


/**	Locks the kernel.

	Increments iKernCSLocked, thereby deferring IDFCs and preemption.

    @pre    Call either in a thread or an IDFC context.
    @pre    Do not call from an ISR.
	@pre	Do not call from bare Win32 threads.
 */
EXPORT_C void NKern::Lock()
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR,"NKern::Lock");		
	__ASSERT_WITH_MESSAGE_ALWAYS(IsScheduledThread(),"Do not call from bare Win32 threads","NKern::Lock");	// check that we are a scheduled thread
	++TheScheduler.iKernCSLocked;
	}


/**	Locks the kernel and returns a pointer to the current thread
	Increments iKernCSLocked, thereby deferring IDFCs and preemption.

    @pre    Call either in a thread or an IDFC context.
    @pre    Do not call from an ISR.
	@pre	Do not call from bare Win32 threads.
 */
EXPORT_C NThread* NKern::LockC()
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR,"NKern::Lock");		
	__ASSERT_WITH_MESSAGE_ALWAYS(IsScheduledThread(),"Do not call from bare Win32 threads","NKern::Lock");	// check that we are a scheduled thread
	++TheScheduler.iKernCSLocked;
	return (NThread*)TheScheduler.iCurrentThread;
	}


/**	Allows IDFCs and rescheduling if they are pending.

	If IDFCs or a reschedule are pending and iKernCSLocked is exactly equal to 1
	calls the scheduler to process the IDFCs and possibly reschedule.

	@return	Nonzero if a reschedule actually occurred, zero if not.
	
    @pre    Call either in a thread or an IDFC context.
    @pre    Do not call from an ISR.
	@pre	Do not call from bare Win32 threads.
 */
EXPORT_C TInt NKern::PreemptionPoint()
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR,"NKern::PreemptionPoint");		
	__ASSERT_WITH_MESSAGE_DEBUG(IsScheduledThread(),"Do not call from bare Win32 threads","NKern::PreemptionPoint");	// check that we are a scheduled thread
	if (TheScheduler.iKernCSLocked == 1 && 
		(TheScheduler.iRescheduleNeededFlag || TheScheduler.iDfcPendingFlag))
		{
		TScheduler::Reschedule();
		TheScheduler.iKernCSLocked = 1;
		NKern::EnableAllInterrupts();
		return TRUE;
		}
	return FALSE;
	}


/**	Mark the start of an 'interrupt' in the Win32 emulator.
	This must be called in interrupt threads before using any other kernel APIs,
	and should be paired with a call to EndOfInterrupt().

	@pre	Win32 'interrupt' thread context
 */
EXPORT_C void StartOfInterrupt()
	{
	__ASSERT_WITH_MESSAGE_DEBUG(!IsScheduledThread(),"Win32 'interrupt' thread context","StartOfInterrupt");	// check that we are a scheduled thread
	Interrupt.Begin();
	}


/**	Mark the end of an 'interrupt' in the Win32 emulator.
	This checks to see if we need to reschedule.

	@pre	Win32 'interrupt' thread context
 */
EXPORT_C void EndOfInterrupt()
	{
	__ASSERT_WITH_MESSAGE_DEBUG(!IsScheduledThread(),"Win32 'interrupt' thread context","EndOfInterrupt");	// check that we are a scheduled thread
	Interrupt.End();
	}


void Win32Interrupt::Init()
	{
	iQ=CreateSemaphoreA(NULL, 0, KMaxTInt, NULL);
	__NK_ASSERT_ALWAYS(iQ);
	//
	// create the NThread which exists solely to service reschedules for interrupts
	// this makes the End() much simpler as it merely needs to kick this thread
	SNThreadCreateInfo ni;
	memclr(&ni, sizeof(ni));
	ni.iFunction=&Reschedule;
	ni.iTimeslice=-1;
	ni.iPriority=1;
	NKern::ThreadCreate(&iScheduler, ni);
	NKern::Lock();
	TScheduler::YieldTo(&iScheduler);
	Restore(0);
	}

TInt Win32Interrupt::Mask()
	{
	if (!iQ)
		return 0;				// interrupt scheme not enabled yet
	DWORD id=GetCurrentThreadId();
	if (__e32_atomic_add_ord32(&iLock, 1))
		{
		if (id==iOwner)
			return iLevel++;
		__NK_ASSERT_ALWAYS(WaitForSingleObject(iQ,INFINITE) == WAIT_OBJECT_0);
		iRescheduleOnExit=IsScheduledThread() &&
				(TheScheduler.iRescheduleNeededFlag || TheScheduler.iDfcPendingFlag);
		}
	else
		iRescheduleOnExit=FALSE;
	__NK_ASSERT_ALWAYS(iOwner==0 && iLevel==0);
	iOwner=id;
	iLevel=1;
	return 0;
	}

void Win32Interrupt::Restore(TInt aLevel)
	{
	if (!iQ)
		return;				// interrupt scheme not enabled yet
	DWORD id=GetCurrentThreadId();
	for (;;)
		{
		__NK_ASSERT_ALWAYS(id == iOwner);
		TInt count = iLevel - aLevel;
		if (count <= 0)
			return;						// alredy restored to that level
		TBool reschedule = FALSE;
		iLevel = aLevel;		// update this value before releasing the lock
		if (aLevel == 0)
			{
			// we release the lock
			iOwner = 0;
			if (iRescheduleOnExit && TheScheduler.iKernCSLocked == 0)
				reschedule = TRUE;		// need to trigger reschedule on full release
			}
		// now release the lock
		if (__e32_atomic_add_ord32(&iLock, TUint32(-count)) == (TUint32)count)
			{	// fully released, check for reschedule
			if (!reschedule)
				return;
			}
		else
			{	// not fully released
			if (aLevel == 0)
				__NK_ASSERT_ALWAYS(ReleaseSemaphore(iQ,1,NULL));
			return;
			}
		// unlocked everything but a reschedule may be required
		TheScheduler.iKernCSLocked = 1;
		TScheduler::Reschedule();
		// return with the kernel unlocked, but interrupts disabled
		// instead of going recursive with a call to EnableAllInterrupts() we iterate
		aLevel=0;
		}
	}

void Win32Interrupt::Begin()
	{
	Mask();
	__NK_ASSERT_ALWAYS(iInterrupted==0);	// check we haven't done this already
	__NK_ASSERT_ALWAYS(!IsScheduledThread());	// check that we aren't a scheduled thread
	NThread* pC;
	for (;;)
		{
		pC=static_cast<NThread*>(TheScheduler.iCurrentThread);
		DWORD r=SuspendThread(pC->iWinThread);
		if (pC == TheScheduler.iCurrentThread)
			{
			// there was no race while suspending the thread, so we can carry on
			__NK_ASSERT_ALWAYS(r != 0xffffffff);
			break;
			}
		// We suspended the thread while doing a context switch, resume it and try again
		if (r != 0xffffffff)
			__NK_ASSERT_ALWAYS(TInt(ResumeThread(pC->iWinThread)) > 0);	// check thread was previously suspended
		}
#ifdef BTRACE_CPU_USAGE
	BTrace0(BTrace::ECpuUsage,BTrace::EIrqStart);
#endif
	iInterrupted = pC;
	}

void Win32Interrupt::End()
	{
	__NK_ASSERT_ALWAYS(iOwner == GetCurrentThreadId());	// check we are the interrupting thread
	NThread* pC = iInterrupted;
	__NK_ASSERT_ALWAYS(pC==TheScheduler.iCurrentThread);
	iInterrupted = 0;
	if (iLock == 1 && TheScheduler.iKernCSLocked == 0 &&
		(TheScheduler.iRescheduleNeededFlag || TheScheduler.iDfcPendingFlag) &&
		pC->IsSafeToPreempt())
		{
		TheScheduler.iKernCSLocked = 1;		// prevent further pre-emption
		if (pC->iWakeup == NThread::EIdle)
			{
			// wake up the NULL thread, it will always reschedule immediately
			pC->WakeUp();
			}
		else
			{
			// pre-empt the current thread and poke the 'scheduler' thread
			__NK_ASSERT_ALWAYS(pC->iWakeup == NThread::ERelease);
			pC->iWakeup = NThread::EResume;
			UpdateThreadCpuTime(*pC, iScheduler);
			RescheduleNeeded();
			NKern::EnableAllInterrupts();
			iScheduler.WakeUp();
			return;
			}
		}
	else
		{
		// no thread reschedle, so emit trace...
#ifdef BTRACE_CPU_USAGE
		BTrace0(BTrace::ECpuUsage,BTrace::EIrqEnd);
#endif
		}

	if (((NThread*)pC)->iInKernel == 0 &&		// thread is running in user mode
		pC->iUserModeCallbacks != NULL && 		// and has callbacks queued
		TheScheduler.iKernCSLocked == 0 &&		// and is not currently processing a diversion
		pC->IsSafeToPreempt())					// and can be safely prempted at this point
		{
		TheScheduler.iKernCSLocked = 1;
		pC->ApplyDiversion();
		}
	NKern::EnableAllInterrupts();
	__NK_ASSERT_ALWAYS(TInt(ResumeThread(pC->iWinThread)) > 0);	// check thread was previously suspended
	}

void Win32Interrupt::Reschedule(TAny*)
//
// The entry-point for the interrupt-rescheduler thread.
//
// This spends its whole life going around the TScheduler::Reschedule() loop
// selecting another thread to run.
//
	{
	TheScheduler.iKernCSLocked = 1;
	RescheduleNeeded();
	TScheduler::Reschedule();
	FAULT();
	}

void Win32Interrupt::ForceReschedule()
	{
	RescheduleNeeded();
	iScheduler.WakeUp();
	}

void SchedulerEscape()
	{
	NThread& me=CheckedCurrentThread();
	EnterKernel();
	__NK_ASSERT_ALWAYS(TheScheduler.iKernCSLocked==0);	// Can't call Escape() with the Emulator/kernel already locked
	NKern::ThreadEnterCS();
	NKern::Lock();
	me.iNState=NThreadBase::EBlocked;
	TheScheduler.Remove(&me);
	me.iWakeup=NThread::EEscaped;
	SetThreadPriority(me.iWinThread,THREAD_PRIORITY_ABOVE_NORMAL);
	Interrupt.ForceReschedule();	// schedules some other thread so we can carry on outside the scheduler domain
	// this will change the value of iCurrentThread to ensure the 'escaped' invariants are set
	}

void ReenterDfc(TAny* aPtr)
	{
	NThread& me = *static_cast<NThread*>(aPtr);
	me.iWakeup = NThread::ERelease;
	me.CheckSuspendThenReady();
	}

void SchedulerReenter()
	{
	NThread* me=SchedulerThread();
	__NK_ASSERT_ALWAYS(me);
	__NK_ASSERT_ALWAYS(me->iWakeup == NThread::EEscaped);
	TDfc idfc(&ReenterDfc, me);
	StartOfInterrupt();
	idfc.Add();
	EndOfInterrupt();
	SetThreadPriority(me->iWinThread,THREAD_PRIORITY_NORMAL);
	__NK_ASSERT_ALWAYS(WaitForSingleObject(me->iScheduleLock, INFINITE) == WAIT_OBJECT_0);
	// when released, the kernel is locked and handed over to us
	// need to complete the reschedule protocol in this thread now
	TScheduler::Reschedule();
	NKern::EnableAllInterrupts();
	NKern::ThreadLeaveCS();
	LeaveKernel();
	}


/**	Return the current processor context type
	(thread, IDFC, interrupt or escaped thread)

	@return	A value from NKern::TContext enumeration (including EEscaped)
	@pre	Any context

	@see	NKern::TContext
 */
EXPORT_C TInt NKern::CurrentContext()
	{
	NThread* t = SchedulerThread();
	if (!t)
		return NKern::EInterrupt;
	if (TheScheduler.iInIDFC)
		return NKern::EIDFC;
	if (t->iWakeup == NThread::EEscaped)
		return NKern::EEscaped;
	__NK_ASSERT_ALWAYS(NKern::Crashed() || t == TheScheduler.iCurrentThread);
	return NKern::EThread;
	}

//
// We use SuspendThread and ResumeThread to preempt threads.  This can cause
// deadlock if the thread is using windows synchronisation primitives (eg
// critical sections).  This isn't too much of a problem most of the time,
// because threads generally use the symbian environment rather than the native
// windows APIs.  However exceptions are an issue - they can happen at any time,
// and cause execution of native windows code over which we have no control.
//
// To work around this we examine the call stack to see if the thread is inside
// one of the windows exception handling functions.  If so, preemption is
// deferred.
//

#include <winnt.h>

// Uncomment the following line to turn on tracing when we examine the call stack
// #define DUMP_STACK_BACKTRACE

#ifdef DUMP_STACK_BACKTRACE

#include <psapi.h>

typedef BOOL (WINAPI GMIFunc)(HANDLE hProcess, HMODULE hModule, LPMODULEINFO lpmodinfo, DWORD cb);
typedef BOOL (WINAPI EPMFunc)(HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded);
typedef DWORD (WINAPI GMBNFunc)(HANDLE hProcess, HMODULE hModule, LPSTR lpBaseName, DWORD nSize);

void PrintAllModuleInfo()
	{
	HMODULE psapiLibrary = LoadLibraryA("psapi.dll");
	__NK_ASSERT_ALWAYS(psapiLibrary != NULL);

	EPMFunc* epmFunc = (EPMFunc*)GetProcAddress(psapiLibrary, "EnumProcessModules");
	__NK_ASSERT_ALWAYS(epmFunc != NULL);
	
	GMIFunc* gmiFunc = (GMIFunc*)GetProcAddress(psapiLibrary, "GetModuleInformation");
	__NK_ASSERT_ALWAYS(gmiFunc != NULL);
	
	GMBNFunc* gmbnFunc = (GMBNFunc*)GetProcAddress(psapiLibrary, "GetModuleBaseNameA");
	__NK_ASSERT_ALWAYS(gmbnFunc != NULL);

	const TInt maxModules = 256;
	HMODULE modules[maxModules];

	DWORD spaceNeeded;
	BOOL r = epmFunc(GetCurrentProcess(), modules, sizeof(HMODULE) * maxModules, &spaceNeeded);
	__NK_ASSERT_ALWAYS(r);
	__NK_ASSERT_ALWAYS(spaceNeeded <= sizeof(HMODULE) * maxModules);

	for (TUint i = 0 ; i < spaceNeeded / sizeof(HMODULE) ; ++i)
		{
		HMODULE library = modules[i];
		
		const TUint maxNameLen = 64;
		char name[maxNameLen];
		WORD len = gmbnFunc(GetCurrentProcess(), library, name, sizeof(name));
		__NK_ASSERT_ALWAYS(len > 0 && len < maxNameLen);
		
		MODULEINFO info;
		r = gmiFunc(GetCurrentProcess(), library, &info, sizeof(info));
		__NK_ASSERT_ALWAYS(r);
		
		DEBUGPRINT("Module %s found at %08x to %08x", name, (TUint)info.lpBaseOfDll, (TUint)info.lpBaseOfDll + info.SizeOfImage);
		}

	r = FreeLibrary(psapiLibrary);
	__NK_ASSERT_ALWAYS(r);
	}

#endif

const TInt KWin32NonPreemptibleFunctionCount = 2;

struct TWin32FunctionInfo
	{
	TUint iStartAddr;
	TUint iLength;
	};

static TWin32FunctionInfo Win32NonPreemptibleFunctions[KWin32NonPreemptibleFunctionCount];

HMODULE GetFirstLoadedModuleHandleA(const char* aModuleName1, const char* aModuleName2)
	{
	HMODULE result = GetModuleHandleA(aModuleName1);
	return result ? result : GetModuleHandleA(aModuleName2);
	}

TWin32FunctionInfo Win32FindExportedFunction(const char* aFunctionName, ...)
	{
	const char *libname;
	HMODULE library = NULL;

	va_list arg;
	va_start(arg, aFunctionName);

	// Loop through arguments until we find a library we can get a handle to.  List of library names
	// is NULL-terminated.
	while ((libname = va_arg(arg, const char *)) != NULL)
		{
		library = GetModuleHandleA(libname);
		if (library != NULL)
			break;
		}

	va_end(arg);

	// Make sure we did get a valid library
	__NK_ASSERT_ALWAYS(library != NULL);
	
	// Find the start address of the function
	TUint start = (TUint)GetProcAddress(library, aFunctionName);
	__NK_ASSERT_ALWAYS(start != 0);

	// Now have to check all other exports to find the end of the function
	TUint end = 0xffffffff;
	TInt i = 1;
	for (;;)
		{
		TUint addr = (TUint)GetProcAddress(library, MAKEINTRESOURCEA(i));
		if (!addr)
			break;
		if (addr > start && addr < end)
			end = addr;
		++i;
		}
	__NK_ASSERT_ALWAYS(end != 0xffffffff);
	
	TWin32FunctionInfo result = { start, end - start };
	
#ifdef DUMP_STACK_BACKTRACE
	DEBUGPRINT("Function %s found at %08x to %08x", aFunctionName, start, end);
#endif
	
	return result;
	}

void Win32FindNonPreemptibleFunctions()
	{
#ifdef DUMP_STACK_BACKTRACE
	PrintAllModuleInfo();
#endif

	TUint i = 0;
	Win32NonPreemptibleFunctions[i++] = Win32FindExportedFunction("RaiseException", "kernelbase.dll", "kernel32.dll", NULL);
	Win32NonPreemptibleFunctions[i++] = Win32FindExportedFunction("KiUserExceptionDispatcher", "ntdll.dll", NULL);
	__NK_ASSERT_ALWAYS(i == KWin32NonPreemptibleFunctionCount);
	}
	
TBool Win32IsThreadInNonPreemptibleFunction(HANDLE aWinThread, TLinAddr aStackTop)
	{
	const TInt KMaxSearchDepth = 16;		 // 12 max observed while handling exceptions
	const TInt KMaxStackSize = 1024 * 1024;  // Default reserved stack size on windows
	const TInt KMaxFrameSize = 4096;

	CONTEXT c;
 	c.ContextFlags=CONTEXT_FULL;
	GetThreadContext(aWinThread, &c);

	TUint eip = c.Eip;
	TUint ebp = c.Ebp;
	TUint lastEbp = c.Esp;

	#ifdef DUMP_STACK_BACKTRACE
	DEBUGPRINT("Stack backtrace for thread %x", aWinThread);
	#endif	

	// Walk the call stack
	for (TInt i = 0 ; i < KMaxSearchDepth ; ++i)
		{
		#ifdef DUMP_STACK_BACKTRACE
		DEBUGPRINT("  %08x", eip);
		#endif
		
		for (TInt j = 0 ; j < KWin32NonPreemptibleFunctionCount ; ++j)
			{
			const TWin32FunctionInfo& info = Win32NonPreemptibleFunctions[j];
			if (TUint(eip - info.iStartAddr) < info.iLength)
				{
				__KTRACE_OPT(KSCHED, DEBUGPRINT("Thread is in non-preemptible function %d at frame %d: eip == %08x", j, i, eip));
				return TRUE;
				}
			}
		
		// Check frame pointer is valid before dereferencing it
		if (TUint(aStackTop - ebp) > KMaxStackSize || TUint(ebp - lastEbp) > KMaxFrameSize || ebp & 3)
			break;

		TUint* frame = (TUint*)ebp;
		lastEbp = ebp;
		ebp = frame[0];
		eip = frame[1];
		}
	
	return FALSE;
	}

TBool NThread::IsSafeToPreempt()
	{
	return !Win32IsThreadInNonPreemptibleFunction(iWinThread, iUserStackBase);
	}

void LeaveKernel()
	{
	TInt& k=CheckedCurrentThread().iInKernel;
	__NK_ASSERT_DEBUG(k>0);
	if (k==1)  // just about to leave kernel
		{
		NThread& t = CheckedCurrentThread();
		__NK_ASSERT_ALWAYS(t.iCsCount==0);
		__NK_ASSERT_ALWAYS(t.iHeldFastMutex==0);
		__NK_ASSERT_ALWAYS(TheScheduler.iKernCSLocked==0);
		NKern::DisableAllInterrupts();
		t.CallUserModeCallbacks();
		NKern::EnableAllInterrupts();
		}
	--k;
	}

