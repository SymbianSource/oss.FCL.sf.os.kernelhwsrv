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
#include <emulator.h>

#ifdef	__EMI_SUPPORT__
extern void EMI_AddTaskSwitchEvent(TAny* aPrevious, TAny* aNext);
extern void EMI_CheckDfcTag(TAny* aNext);
#endif
typedef void (*ProcessHandler)(TAny* aAddressSpace);

static NThreadBase* SelectThread(TScheduler& aS)
//
// Select the next thread to run.
// This is the heart of the rescheduling algorithm.
// This should be essentially the same as the EPOC32 version!
//
	{
	NThreadBase* t = static_cast<NThreadBase*>(aS.First());

#ifdef	_DEBUG
	__NK_ASSERT_DEBUG(t);
	if (t->iHeldFastMutex)
		{
		__KTRACE_OPT(KSCHED2, DEBUGPRINT("Resched init->%T, Holding %M", t, t->iHeldFastMutex));
		}
	else
		{
		__KTRACE_OPT(KSCHED2, DEBUGPRINT("Resched init->%T", t));
		}
#endif	// _DEBUG

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
		__KTRACE_OPT(KSCHED2, DEBUGPRINT("RoundRobin->%T", t));
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
		__KTRACE_OPT(KSCHED2, DEBUGPRINT("Resched inter->%T, Blocked on %M", t->iWaitFastMutex->iHoldingThread, t->iWaitFastMutex));
		t = t->iWaitFastMutex->iHoldingThread;
		}
	else if (t->i_ThrdAttr & KThreadAttImplicitSystemLock)
		{
		// implicit system lock required
		if (aS.iLock.iHoldingThread)
			{
			// system lock held, switch to that thread
			t = aS.iLock.iHoldingThread;
			__KTRACE_OPT(KSCHED2, DEBUGPRINT("Resched inter->%T (IMP SYS)", t));
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

// From here on it's all emulator (i.e. Win32) specific; there isn't any EPOC32 equivalent for most of it.
//
// The emulator uses one Win32 thread for each Symbian thread; these are the ones scheduled by the Symbian
// nanokernel in the algorithm above. Only one such thread will be running at a time; the others will be
// waiting on their individual scheduler locks, thus simulating a single-threaded architecture.
//
// In addition, there are some more Win32 threads used to handle timers, interrupts and the like. These
// are not under control of the Symbian scheduler. They are given higher priority than the Symbian threads,
// so they can run preemptively under control of the Win32 scheduler. However, they must call functions
// from the Win32Interrupt class before using any Symbian OS calls, so that the current Symbian thread can
// be suspended during the 'virtual interrupt'.

static DWORD TlsIndex = TLS_OUT_OF_INDEXES;

void SchedulerInit(NThread& aInit)
//
// Initialise the win32 nKern scheduler
//
	{
	DWORD procaffin, sysaffin;
	if (GetProcessAffinityMask(GetCurrentProcess(), &procaffin, &sysaffin))
		{
		DWORD cpu;
		switch (Win32SingleCpu)
			{
		default:
			// bind the emulator to a nominated CPU on the host PC
			cpu = (1 << Win32SingleCpu);
			if (!(sysaffin & cpu))
				cpu = procaffin;	// CPU selection invalid
			break;

		case NThread::ECpuSingle:
			// bind the emulator to a single CPU on the host PC, pick one
			cpu = procaffin ^ (procaffin & (procaffin - 1));
			break;

		case NThread::ECpuAll:
			// run the emulator on all CPUs on the host PC
			cpu = sysaffin;
			break;
			}

		SetProcessAffinityMask(GetCurrentProcess(), cpu);
		}

	// identify whether we can use the atomic SignalObjectAndWait API in Win32 for rescheduling
	Win32AtomicSOAW = (SignalObjectAndWait(aInit.iScheduleLock, aInit.iScheduleLock, INFINITE, FALSE) == WAIT_OBJECT_0);

	// allocate the TLS used for thread identification, and set it for the init thread
	TlsIndex = TlsAlloc();
	__NK_ASSERT_ALWAYS(TlsIndex != TLS_OUT_OF_INDEXES);
	SchedulerRegister(aInit);

	Win32FindNonPreemptibleFunctions();
	Interrupt.Init();
	}

void SchedulerRegister(NThread& aSelf)
	{
	TlsSetValue(TlsIndex, &aSelf);
	}

inline NThread* RunningThread()
// Returns the NThread actually running
	{
	if (TlsIndex == TLS_OUT_OF_INDEXES)
		return NULL;				// not yet initialised
	else
		return static_cast<NThread*>(TlsGetValue(TlsIndex));
	}

inline TBool IsScheduledThread()
// True if the NThread actually running is the scheduled one (not an interrupt thread or similar)
	{
	return RunningThread() == TheScheduler.iCurrentThread;
	}

inline NThread& CheckedCurrentThread()
// Returns the NThread actually running, checking that it's the scheduled one (not an interrupt thread or similar)
	{
	NThread* t = RunningThread();
	__NK_ASSERT_ALWAYS(t == TheScheduler.iCurrentThread);
	return *t;
	}

static void ThreadExit(NThread& aCurrent, NThread& aNext)
//
// The final context switch of a thread.
// Wake up the next thread and then destroy this one's Win32 resources.
//
// Return without terminating if we need to immediately reschedule again
// because we had to unlock the kernel but there are DFCs pending.
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

#ifdef	MONITOR_THREAD_CPU_TIME
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
#endif	// MONITOR_THREAD_CPU_TIME

static void SwitchThreads(NThread& aCurrent, NThread& aNext)
//
// The fundamental context switch - wake up the next thread and wait for reschedule
// trivially is aNext.WakeUp(), Wait(aCurrent.iScheduleLock), but we may be able to
// optimise the signal-and-wait
//
	{
	__NK_ASSERT_ALWAYS(InterruptsStatus(ETrue));
	UpdateThreadCpuTime(aCurrent, aNext);

	if (aCurrent.iNState == NThread::EDead)
		{
		ThreadExit(aCurrent, aNext);
		// Yes, this is reachable!
		}
	else if (Win32AtomicSOAW && aNext.iWakeup == NThread::ERelease)
		{
		// special case optimization for normally scheduled threads using atomic Win32 primitive
		TheScheduler.iCurrentThread = &aNext;
		CheckedSignalObjectAndWait(aNext.iScheduleLock, aCurrent.iScheduleLock);
		}
	else if (aNext.WakeUp())
		{
		// We didn't wake the target thread; instead we need to re-reschedule in this thread
		__NK_ASSERT_ALWAYS(InterruptsStatus(EFalse));
		return;
		}
	else
		{
		// Target thread woken, now wait to be rescheduled
		CheckedWaitForSingleObject(aCurrent.iScheduleLock);
		}

	__NK_ASSERT_ALWAYS(InterruptsStatus(ETrue));
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

		// Exit from this loop when further rescheduling is no longer needed
		if (!TheScheduler.iRescheduleNeededFlag)
			break;

		// Choose the next thread to run, using the Symbian scheduler
		TheScheduler.iRescheduleNeededFlag = FALSE;
		NKern::EnableAllInterrupts();
		NThread* t = static_cast<NThread*>(SelectThread(TheScheduler));
		__KTRACE_OPT(KSCHED, DEBUGPRINT("Reschedule->%T (%08x%08x)", t, TheScheduler.iPresent[1], TheScheduler.iPresent[0]));

#ifdef	__EMI_SUPPORT__
		EMI_AddTaskSwitchEvent(&me, t);
		EMI_CheckDfcTag(t);
#endif
#ifdef	BTRACE_CPU_USAGE
		if (TheScheduler.iCpuUsageFilter)
			TheScheduler.iBTraceHandler(BTRACE_HEADER_C(4, BTrace::ECpuUsage, BTrace::ENewThreadContext), 0, (TUint32)t, 0, 0, 0, 0, 0);
#endif

		// SwitchThreads() can return immediately, if it turns out that another reschedule is
		// necessary; otherwise, this thread will be descheduled in favour of the one selected
		// above, and SwitchThreads() will only return when this thread is next selected
		SwitchThreads(me, *t);

		// When we start again, we should check for being forced to exit; otherwise go round the
		// loop again to see whether another reschedule is called for (e.g. if there are new DFCs).
		NThread::TDivert divertToExit = me.iDivertFn;
		me.iDivertFn = NULL;
		if (divertToExit)
			divertToExit();
		}

	// interrupts are disabled, the kernel is still locked
	if (TheScheduler.iProcessHandler)
		(*ProcessHandler(TheScheduler.iProcessHandler))(me.iAddressSpace);		// thread will need to have its static data updated

	__NK_ASSERT_ALWAYS(TheScheduler.iKernCSLocked == 1);
	TheScheduler.iKernCSLocked = 0;
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

TBool NThread::WakeUp()
//
// Wake up the thread. What to do depends on whether it was preempted or voluntarily
// rescheduled.
//
// On entry, the kernel is locked, and interrupts may be enabled or disabled.
//
// The return value is TRUE if the caller should immediately reschedule again because we
// needed to unlock the kernel in order to resume the thread but there were DFCs pending.
// In this case, the thread is not woken, the kernel remains locked, and the return is
// made with interrupts disabled (whether or not they were on entry).
//
// Otherise, the target thread is woken up (in any of several different ways), and the
// the return value is FALSE. In that case the interrupt status is unchanged; and the
// kernel may or not still be locked.
//
	{
	__NK_ASSERT_ALWAYS(TheScheduler.iKernCSLocked > 0);
	__NK_ASSERT_ALWAYS(RunningThread() != this);		// Can't wake self!

	switch (iWakeup)
		{
	default:
		FAULT();

	case EIdle:
		// The thread is waiting on its scheduler lock, in Idle()
		__NK_ASSERT_ALWAYS(TheScheduler.iCurrentThread == this);
		CheckedSetEvent(iScheduleLock);
		break;

	case ERelease:
		// The thread is waiting on its scheduler lock
		TheScheduler.iCurrentThread = this;
		CheckedSetEvent(iScheduleLock);
		break;

	case EResumeLocked:
		// The thread is Win32 suspended and must be resumed.
		//
		// A newly created thread does not need the kernel unlocked so we can
		// just resume it; OTOH it will need to have its static data updated ...
		//
		__KTRACE_OPT(KSCHED, DEBUGPRINT("Win32ResumeLocked->%T", this));
		iWakeup = ERelease;
		TheScheduler.iCurrentThread = this;
		if (TheScheduler.iProcessHandler)
			(*ProcessHandler(TheScheduler.iProcessHandler))(iAddressSpace);
		CheckedResumeThread(iWinThread);
		break;

	case EResumeDiverted:
		// The thread is Win32 suspended and must be resumed.
		//
		// It does not need the kernel unlocked, but does have a diversion pending. We
		// know it's safe to divert the thread here because we called IsSafeToPreempt()
		// when we suspended it - otherwise the diversion could get lost.
		//
		__KTRACE_OPT(KSCHED, DEBUGPRINT("Win32Resume->%T (Resuming diverted thread)", this));
		iWakeup = ERelease;
		TheScheduler.iCurrentThread = this;
		ApplyDiversion();
		CheckedResumeThread(iWinThread, ETrue);
		break;

	case EResume:
		// The thread is Win32 suspended and must be resumed.
		//
		// The complication here is that we have to unlock the kernel on behalf of the
		// pre-empted thread. Before doing so, we have to check whether there are DFCs
		// or a reschedule pending; if so, we don't unlock the kernel or wake the target
		// thread, but instead return TRUE, so that our caller (usually SwitchThreads()
		// above) knows to return and go round the TScheduler::Reschedule() loop again.
		//
		TInt irq = NKern::DisableAllInterrupts();
		if (TheScheduler.iRescheduleNeededFlag || TheScheduler.iDfcPendingFlag)
			{
			__KTRACE_OPT(KSCHED, DEBUGPRINT("Win32Resume->%T preempted", this));
			TheScheduler.iRescheduleNeededFlag = TRUE;	// ensure we do the reschedule
			return TRUE;
			}

		// Otherwise we mark the thread as not-preempted, unlock the kernel, restore
		// interrupts, and resume the thread.
		__KTRACE_OPT(KSCHED, DEBUGPRINT("Win32Resume->%T", this));
		iWakeup = ERelease;
		TheScheduler.iCurrentThread = this;
		if (TheScheduler.iProcessHandler)
			(*ProcessHandler(TheScheduler.iProcessHandler))(iAddressSpace); // threads resumed after interrupt or locks need to have static data updated
		TheScheduler.iKernCSLocked = 0;

		// If there are callbacks waiting, and the thread is in user mode, divert it to
		// pick up its callbacks (we know this is safe because we called IsSafeToPreempt()
		// when we suspended it - otherwise the diversion could get lost.
		if (iUserModeCallbacks != NULL && !iInKernel)
			{
			TheScheduler.iKernCSLocked = 1;					// prevent further pre-emption
			ApplyDiversion();
			}

		// If pre-emption occurs before the thread is resumed, it is the new thread that
		// is pre-empted, not the running thread, so we are guaranteed to be able to call
		// ResumeThread. If pre-emption occurs, and we are rescheduled to run before that
		// occurs, we will once again be running with the kernel locked and the other
		// thread will have been re-suspended by Win32: so all is well.
		//
		NKern::RestoreInterrupts(irq);
		CheckedResumeThread(iWinThread);
		break;
		}

	return FALSE;
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
	CheckedWaitForSingleObject(me.iScheduleLock);

	// something happened, and we've been prodded by an interrupt
	// the kernel was locked by the interrupt, and now reschedule
	me.iWakeup = ERelease;
	TScheduler::Reschedule();
	NKern::EnableAllInterrupts();
	}


void EnterKernel(TBool aDiversion)
	{
	NThread& t = CheckedCurrentThread();
	volatile TInt& inKernel = t.iInKernel;
	__NK_ASSERT_DEBUG(inKernel >= 0);

	// This code has to be re-entrant, because a thread that's in the process
	// of entering the kernel may be preempted; then if it isn't yet marked
	// as 'in the kernel' it can be diverted through EnterKernel()/LeaveKernel()
	// in order to execute user-mode callbacks.  However this is all in the
	// same thread context, so it doesn't need any special synchronisation.
	// The moment of 'entering' the kernel is deemed to occur when the new value
	// of iInKernel is written back to the NThread object.
	if (inKernel++ == 0)
		{
		// preamble when coming from userspace
		__NK_ASSERT_ALWAYS(InterruptsStatus(ETrue));
		__NK_ASSERT_ALWAYS(t.iHeldFastMutex == 0);
		if (aDiversion)
			{
			// Forced entry, to make thread exit or run user-mode callbacks
			// If exiting, iCsCount will have been set to 1 to prevent preemption
			// Otherwise it must be 0, as in the non-diversion case
			__NK_ASSERT_ALWAYS(t.iCsCount <= 1);
			__NK_ASSERT_ALWAYS(TheScheduler.iKernCSLocked == 1);
			}
		else
			{
			__NK_ASSERT_ALWAYS(t.iCsCount == 0);
			__NK_ASSERT_ALWAYS(TheScheduler.iKernCSLocked == 0);
			}
		}
	}

void LeaveKernel()
	{
	NThread& t = CheckedCurrentThread();
	volatile TInt& inKernel = t.iInKernel;
	__NK_ASSERT_DEBUG(inKernel > 0);

	// This code has to be re-entrant, because a thread that's in the process
	// of leaving the kernel may be preempted; then if it isn't still marked
	// as 'in the kernel' it can be diverted through EnterKernel()/LeaveKernel()
	// in order to execute user-mode callbacks.  However this is all in the
	// same thread context, so it doesn't need any special synchronisation.
	// The moment of 'leaving' the kernel is deemed to occur when the new value
	// of iInKernel is written back to the NThread object.
	if (inKernel == 1)
		{
		// postamble when about to return to userspace
		__NK_ASSERT_ALWAYS(t.iCsCount == 0);
		__NK_ASSERT_ALWAYS(t.iHeldFastMutex == 0);
		__NK_ASSERT_ALWAYS(TheScheduler.iKernCSLocked == 0);
		NKern::DisableAllInterrupts();
		t.CallUserModeCallbacks();
		NKern::EnableAllInterrupts();
		}

	inKernel -= 1;
	}

/**	Locks the kernel and returns a pointer to the current thread
	Increments iKernCSLocked, thereby deferring IDFCs and preemption.

    @pre    Call either in a thread or an IDFC context.
    @pre    Do not call from an ISR.
	@pre	Do not call from bare Win32 threads.
 */
EXPORT_C NThread* NKern::LockC()
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR, "NKern::LockC");
	__ASSERT_WITH_MESSAGE_ALWAYS(IsScheduledThread(), "Do not call from bare Win32 threads", "NKern::LockC");	// check that we are a scheduled thread
	++TheScheduler.iKernCSLocked;
	return (NThread*)TheScheduler.iCurrentThread;
	}

/**	Locks the kernel.

	Increments iKernCSLocked, thereby deferring IDFCs and preemption.

    @pre    Call either in a thread or an IDFC context.
    @pre    Do not call from an ISR.
	@pre	Do not call from bare Win32 threads.
 */
EXPORT_C void NKern::Lock()
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR, "NKern::Lock");
	__ASSERT_WITH_MESSAGE_ALWAYS(IsScheduledThread(), "Do not call from bare Win32 threads", "NKern::Lock");	// check that we are a scheduled thread
	++TheScheduler.iKernCSLocked;
	}

/**	Unlocks the kernel.

	Decrements iKernCSLocked; if it would become zero and IDFCs or a reschedule are
	pending, calls the scheduler to process them.

    @pre    Call either in a thread or an IDFC context.
    @pre    Do not call from an ISR.
	@pre	Do not call from bare Win32 threads.
 */
EXPORT_C void NKern::Unlock()
	{
	// check that the caller is the scheduled thread
	__ASSERT_WITH_MESSAGE_DEBUG(IsScheduledThread(), "Do not call from bare Win32 threads", "NKern::Unlock");
	CHECK_PRECONDITIONS(MASK_NOT_ISR, "NKern::Unlock");
	__NK_ASSERT_ALWAYS(TheScheduler.iKernCSLocked > 0);	// Can't unlock if it isn't locked!

	// Rather than decrementing the lock before testing the flags, and then
	// re-incrementing it in order to call Reschedule() -- which would
	// leave a window for preemption -- we can test the flags first, and then
	// see whether the lock count is 1 ...
	if ((TheScheduler.iRescheduleNeededFlag || TheScheduler.iDfcPendingFlag) &&
			TheScheduler.iKernCSLocked == 1)
		{
		// Reschedule() returns with the kernel unlocked, but interrupts disabled
		TScheduler::Reschedule();
		NKern::EnableAllInterrupts();
		}
	else
		{
		// All other cases - just decrement the lock count
		TheScheduler.iKernCSLocked -= 1;
		}
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
	// check that the caller is the scheduled thread
	__ASSERT_WITH_MESSAGE_DEBUG(IsScheduledThread(), "Do not call from bare Win32 threads", "NKern::PreemptionPoint");
	CHECK_PRECONDITIONS(MASK_NOT_ISR, "NKern::PreemptionPoint");

	if ((TheScheduler.iRescheduleNeededFlag || TheScheduler.iDfcPendingFlag) &&
			TheScheduler.iKernCSLocked == 1)
		{
		// Reschedule() returns with the kernel unlocked, but interrupts disabled
		TScheduler::Reschedule();
		TheScheduler.iKernCSLocked = 1;
		NKern::EnableAllInterrupts();
		return TRUE;
		}

	return FALSE;
	}

/**	Return the current processor context type
	(thread, IDFC, interrupt or escaped thread)

	@return	A value from NKern::TContext enumeration (including EEscaped)
	@pre	Any context

	@see	NKern::TContext
 */
EXPORT_C TInt NKern::CurrentContext()
	{
	NThread* t = RunningThread();

	if (!t)
		return NKern::EInterrupt;

	if (TheScheduler.iInIDFC)
		return NKern::EIDFC;

	if (t->iWakeup == NThread::EEscaped)
		return NKern::EEscaped;

	__NK_ASSERT_ALWAYS(NKern::Crashed() || t == TheScheduler.iCurrentThread);
	return NKern::EThread;
	}


/**	Disable normal 'interrupts'.

	@param	aLevel Ignored
    @pre    Call in a Symbian (thread, IDFC, ISR) context.
	@pre	Do not call from bare Win32 threads.
	@return	Cookie to be passed into RestoreInterrupts()
 */
EXPORT_C TInt NKern::DisableInterrupts(TInt /*aLevel*/)
	{
	return Interrupt.MaskInterrupts(EFalse);
	}

/** Restore interrupt mask to state preceding a DisableInterrupts() call

	@param	aLevel Cookie returned by Disable(All)Interrupts()
    @pre    Call in a Symbian (thread, IDFC, ISR) context.
	@pre	Do not call from bare Win32 threads.
 */
EXPORT_C void NKern::RestoreInterrupts(TInt aLevel)
	{
	Interrupt.RestoreInterruptMask(aLevel);
	}

/**	Disable all maskable 'interrupts'.

    @pre    Call in a Symbian (thread, IDFC, ISR) context.
	@pre	Do not call from bare Win32 threads.
	@return	Cookie to be passed into RestoreInterrupts()
 */
EXPORT_C TInt NKern::DisableAllInterrupts()
	{
	return Interrupt.MaskInterrupts(EFalse);
	}

/**	Enable all maskable 'interrupts'

	@internalComponent
    @pre    Call in a Symbian (thread, IDFC, ISR) context.
	@pre	Do not call from bare Win32 threads.
 */
EXPORT_C void NKern::EnableAllInterrupts()
	{
	Interrupt.RestoreInterruptMask(0);
	}

/**	Mark the start of an 'interrupt' in the Win32 emulator.
	This must be called in interrupt threads before using any other kernel APIs,
	and should be paired with a call to EndOfInterrupt().

	@pre	Win32 'interrupt' thread context
 */
EXPORT_C void StartOfInterrupt()
	{
	// check that the caller is not a scheduled thread
	__ASSERT_WITH_MESSAGE_DEBUG(!IsScheduledThread(), "Win32 'interrupt' thread context", "StartOfInterrupt");
	Interrupt.BeginInterrupt();
	}

/**	Mark the end of an 'interrupt' in the Win32 emulator.
	This checks to see if we need to reschedule.

	@pre	Win32 'interrupt' thread context
 */
EXPORT_C void EndOfInterrupt()
	{
	// check that the caller is not a scheduled thread
	__ASSERT_WITH_MESSAGE_DEBUG(!IsScheduledThread(), "Win32 'interrupt' thread context", "EndOfInterrupt");
	Interrupt.EndInterrupt();
	}


// The Win32Interrupt class manages virtual interrupts from Win32 event threads

void Win32Interrupt::Init()
	{
	InitializeCriticalSection(&iCS);
	iQ = CreateSemaphoreA(NULL, 0, KMaxTInt, NULL);
	__NK_ASSERT_ALWAYS(iQ);

	// create the NThread which exists solely to service reschedules for interrupts
	// this makes the End() much simpler as it merely needs to kick this thread
	SNThreadCreateInfo ni;
	memclr(&ni, sizeof(ni));
	ni.iFunction = &SchedulerThreadFunction;
	ni.iTimeslice = -1;
	ni.iPriority = 1;
	NKern::ThreadCreate(&iScheduler, ni);
	NKern::Lock();
	TScheduler::YieldTo(&iScheduler);
	RestoreInterruptMask(0);
	}

void Win32Interrupt::BeginInterrupt()
	{
	__NK_ASSERT_ALWAYS(!IsScheduledThread());				// check that we aren't a scheduled thread
	MaskInterrupts(ETrue);									// suspend scheduled thread and set mask
#ifdef	BTRACE_CPU_USAGE
	BTrace0(BTrace::ECpuUsage, BTrace::EIrqStart);
#endif
	}

void Win32Interrupt::EndInterrupt()
	{
	NThread* pC = iInterrupted;
	iInterrupted = 0;
	__NK_ASSERT_ALWAYS(pC == TheScheduler.iCurrentThread);	// unchanged since BeginInterrupt()
	__NK_ASSERT_ALWAYS(!IsScheduledThread());				// check that we aren't a scheduled thread
	__NK_ASSERT_ALWAYS(iOwner == GetCurrentThreadId());		// check we are the interrupting thread
	__NK_ASSERT_ALWAYS(InterruptsStatus(EFalse));
	__NK_ASSERT_ALWAYS(iLevel == 1);						// DSG: is this correct?

	if (TheScheduler.iKernCSLocked)
		{
		// No rescheduling allowed; just resume the interrupted thread
		NKern::EnableAllInterrupts();
		CheckedResumeThread(pC->iWinThread);
		return;
		}

	__NK_ASSERT_ALWAYS(iLevel == 1);						// DSG: is this correct?
	__NK_ASSERT_ALWAYS(TheScheduler.iKernCSLocked == 0);

	TBool diversionUnsafe = EFalse;							// Optimistic assumption until checked
	if (TheScheduler.iRescheduleNeededFlag || TheScheduler.iDfcPendingFlag)
		{
		switch (pC->iWakeup)
			{
		default:
			FAULT();

		case NThread::EIdle:
			// wake up the Idle thread, it will always reschedule immediately
			TheScheduler.iKernCSLocked = 1;					// prevent further pre-emption
			if (pC->WakeUp())
				FAULT();									// this can't happen
			NKern::EnableAllInterrupts();
			CheckedResumeThread(pC->iWinThread);
			return;

		case NThread::ERelease:
			if (pC->IsSafeToPreempt())
				{
				// pre-empt the current thread and poke the 'scheduler' thread
				UpdateThreadCpuTime(*pC, iScheduler);
				pC->iWakeup = NThread::EResume;				// how to wake this thread later
				TheScheduler.iKernCSLocked = 1;				// prevent further pre-emption
				RescheduleNeeded();
				NKern::EnableAllInterrupts();
				if (iScheduler.WakeUp())
					FAULT();								// this can't happen
				return;
				}

			diversionUnsafe = ETrue;						// don't consider diverting
			break;
			}
		}

#ifdef	BTRACE_CPU_USAGE
	// no thread reschedle, so emit trace...
	BTrace0(BTrace::ECpuUsage, BTrace::EIrqEnd);
#endif

	// If there are callbacks waiting, and the thread is in user mode, and it's at a
	// point where it can safely be preempted, then divert it to pick up its callbacks
	if (pC->iUserModeCallbacks != NULL && !pC->iInKernel && !diversionUnsafe)
		if (pC->IsSafeToPreempt())
			{
			TheScheduler.iKernCSLocked = 1;
			pC->ApplyDiversion();
			}

	NKern::EnableAllInterrupts();
	CheckedResumeThread(pC->iWinThread);
	}


TInt Win32Interrupt::MaskInterrupts(TBool aPreempt)
	{
	if (!iQ)
		return 0;									// interrupt scheme not enabled yet

	EnterCriticalSection(&iCS); 					// Win32 critical section, not a Symbian one

	DWORD id = GetCurrentThreadId();
	if (iOwner == id)
		{
		// The easiest case: we already own the mask, so just increment the level.
		// The requirement for rescheduling on exit is unaffected.
		__NK_ASSERT_ALWAYS(!aPreempt);
		TInt r = iLevel++;
		LeaveCriticalSection(&iCS);
		return r;
		}

	if (!iOwner && !aPreempt)
		{
		// Another easy case; we've been called from a Symbian thread, and there's
		// no contention, so we can just take ownership of the interrupt mask. No
		// rescheduling is required on exit (but this may change) ...
		__NK_ASSERT_ALWAYS(iLevel == 0);
		TInt r = iLevel++;
		iOwner = id;
		iRescheduleOnExit = EFalse;
		LeaveCriticalSection(&iCS);
		return r;
		}

	if (iOwner)
		{
		// Someone else owns it; if we've been called from an interrupt thread,
		// this could be another interrupt thread or a Symbian thread. If we're
		// being called from a Symbian thread, the owner must be another Symbian
		// thread, because a Symbian thread can't preempt an interrupt thread.
		//
		// In either case, we can increment the count of waiters, then wait for the
		// curent holder to release it. Note that another (interrupt) thread could
		// also do this, and then the order in which they get to run is undefined.
		iWaiting += 1;

		do
			{
			__NK_ASSERT_ALWAYS(iWaiting > 0);
			LeaveCriticalSection(&iCS);
			CheckedWaitForSingleObject(iQ);
			EnterCriticalSection(&iCS);
			__NK_ASSERT_ALWAYS(iWaiting > 0);
			}
		while (iOwner);

		iWaiting -= 1;
		iRescheduleOnExit = IsScheduledThread() && (TheScheduler.iRescheduleNeededFlag || TheScheduler.iDfcPendingFlag);
		}

	// Nobody now controls the interrupt mask ...
	__NK_ASSERT_ALWAYS(iOwner == 0 && iLevel == 0);

	if (aPreempt)
		{
		// ... but in this case, we've been called from an interrupt thread and
		// a Symbian thread may still be running -- yes, even though all emulator
		// threads are normally bound to a single CPU!
		//
		// To ensure that such a thread doesn't see an inconsistent state, we
		// have to suspend it before we actually take ownership, as it could
		// examine the interrupt state at any time, without taking any locks.

		__NK_ASSERT_ALWAYS(iInterrupted == 0);		// we haven't done this already
		NThread* pC;
		for (;;)
			{
			pC = static_cast<NThread*>(TheScheduler.iCurrentThread);
			CheckedSuspendThread(pC->iWinThread);
			if (pC == TheScheduler.iCurrentThread)
				break;								// no change of thread, so ok to proceed

			// We suspended the thread while doing a (Symbian) context switch!
			// The scheduler state might be inconsistent if we left it like that,
			// so instead we'll resume it, then try again ...
			CheckedResumeThread(pC->iWinThread);
			}

		__NK_ASSERT_ALWAYS(iInterrupted == 0);
		iInterrupted = pC;
		}

	// Now we can assert ownership of the interrupt mask.
	__NK_ASSERT_ALWAYS(iOwner == 0 && iLevel == 0);
	TInt r = iLevel++;
	iOwner = id;
	LeaveCriticalSection(&iCS);
	return r;
	}

void Win32Interrupt::RestoreInterruptMask(TInt aLevel)
	{
	if (!iQ)
		return;										// interrupt scheme not enabled yet

	DWORD id = GetCurrentThreadId();
	EnterCriticalSection(&iCS); 					// Win32 critical section, not a Symbian one

	for (;;)
		{
		__NK_ASSERT_ALWAYS(id == iOwner);			// only the current owner may do this
		TInt count = iLevel - aLevel;
		if (count <= 0)
			break;									// already restored to that level

		iLevel = aLevel;							// update the recursion level first
		if (aLevel > 0)
			{
			// The easiest case: we're still holding ownership, so there's nothing to do
			break;
			}

		iOwner = 0;									// give up ownership
		if (iWaiting)
			{
			// Someone else is waiting for control of the interrupt mask.
			// They may preempt us as soon as we exit the critical section
			// (at the end of this function)
			CheckedReleaseSemaphore(iQ);
			break;
			}

		// Lock fully released, no-one waiting, so see whether we need to reschedule
		if (TheScheduler.iKernCSLocked || !iRescheduleOnExit)
			break;

		// Interrupt mask fully unlocked, but reschedule required ...
		TheScheduler.iKernCSLocked = 1;
		LeaveCriticalSection(&iCS);
		TScheduler::Reschedule();
		EnterCriticalSection(&iCS);

		// Note: TScheduler::Reschedule() above calls MaskInterrupts() -- which changes
		// the state of most of our member data. It returns with the kernel unlocked,
		// but interrupts still disabled. Hence we will have reacquired ownership of the
		// interrupt mask, and must release it again.  Instead of going recursive with a
		// call to EnableAllInterrupts() we iterate; we'll get out of this loop eventually,
		// because iRescheduleOnExit is updated by MaskInterrupts() ...
		aLevel = 0;
		}

	LeaveCriticalSection(&iCS);
	}

void Win32Interrupt::ForceReschedule()
	{
	RescheduleNeeded();
	if (iScheduler.WakeUp())
		FAULT();											// this can't happen
	}

void Win32Interrupt::SchedulerThreadFunction(TAny*)
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


//
// We need a global lock in the emulator to avoid scheduling reentrancy problems with the host
// in particular, some host API calls acquire host mutexes, preempting such services results
// in suspension of those threads which can cause deadlock if another thread requires that host
// mutex.
//
// Because thread dreaction and code loading also require the same underlying mutex (used
// by NT to protect DLL entrypoint calling), this would be rather complex with a fast mutex.
// For now, keep it simple and use the preemption lock. Note that this means that the
// MS timer DFC may be significantly delayed when loading large DLL trees, for example.
//

void SchedulerLock()
//
// Acquire the global lock. May be called before scheduler running, so handle that case
//
	{
	if (TheScheduler.iCurrentThread)
		{
		EnterKernel();
		NKern::Lock();
		}
	}

void SchedulerUnlock()
//
// Release the global lock. May be called before scheduler running, so handle that case
//
	{
	if (TheScheduler.iCurrentThread)
		{
		NKern::Unlock();
		LeaveKernel();
		}
	}


// This function allows a thread to escape from the Symbian scheduling domain to
// become an ordinary Win32 thread for a while, in cases where it is necessary
// to use Win32 APIs that are incompatible with the Symbian threading model.
// AFAICS this is not currently used!
void SchedulerEscape()
	{
	NThread& me = CheckedCurrentThread();
	EnterKernel();
	__NK_ASSERT_ALWAYS(TheScheduler.iKernCSLocked == 0);	// Can't call Escape() with the Emulator/kernel already locked
	NKern::ThreadEnterCS();
	NKern::Lock();
	me.iNState = NThreadBase::EBlocked;
	TheScheduler.Remove(&me);
	me.iWakeup = NThread::EEscaped;
	SetThreadPriority(me.iWinThread, THREAD_PRIORITY_ABOVE_NORMAL);
	Interrupt.ForceReschedule();
	// This schedules some other thread so we can carry on outside the scheduler domain.
	// It will change the value of iCurrentThread to ensure the 'escaped' invariants are set
	}

void ReenterDfc(TAny* aPtr)
	{
	NThread& me = *static_cast<NThread*>(aPtr);
	me.iWakeup = NThread::ERelease;
	me.CheckSuspendThenReady();
	}

void SchedulerReenter()
	{
	NThread* me = RunningThread();
	__NK_ASSERT_ALWAYS(me);
	__NK_ASSERT_ALWAYS(me->iWakeup == NThread::EEscaped);
	TDfc idfc(&ReenterDfc, me);
	StartOfInterrupt();
	idfc.Add();
	EndOfInterrupt();
	SetThreadPriority(me->iWinThread, THREAD_PRIORITY_NORMAL);
	CheckedWaitForSingleObject(me->iScheduleLock);
	// when released, the kernel is locked and handed over to us
	// need to complete the reschedule protocol in this thread now
	TScheduler::Reschedule();
	NKern::EnableAllInterrupts();
	NKern::ThreadLeaveCS();
	LeaveKernel();
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

#ifdef	DUMP_STACK_BACKTRACE

#include <psapi.h>

typedef BOOL (WINAPI GMIFunc)(HANDLE hProcess, HMODULE hModule, LPMODULEINFO lpmodinfo, DWORD cb);
typedef BOOL (WINAPI EPMFunc)(HANDLE hProcess, HMODULE* lphModule, DWORD cb, LPDWORD lpcbNeeded);
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

#endif	// DUMP_STACK_BACKTRACE

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
	va_list arg;
	va_start(arg, aFunctionName);
	HMODULE library = NULL;
	const char* libname;

	// Loop through arguments until we find a library we can get a handle to.  List of library names
	// is NULL-terminated.
	while ((libname = va_arg(arg, const char*)) != NULL)
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
	TUint end = ~0u;
	for (TInt i = 1; ; ++i)
		{
		TUint addr = (TUint)GetProcAddress(library, MAKEINTRESOURCEA(i));
		if (!addr)
			break;
		if (addr > start && addr < end)
			end = addr;
		}
	__NK_ASSERT_ALWAYS(end != ~0u);
	TWin32FunctionInfo result = { start, end - start };

#ifdef	DUMP_STACK_BACKTRACE
	DEBUGPRINT("Function %s found at %08x to %08x", aFunctionName, start, end);
#endif
	
	return result;
	}

void Win32FindNonPreemptibleFunctions()
	{
#ifdef	DUMP_STACK_BACKTRACE
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
	c.ContextFlags = CONTEXT_CONTROL;
	CheckedGetThreadContext(aWinThread, &c);
	TUint eip = c.Eip;
	TUint ebp = c.Ebp;
	TUint lastEbp = c.Esp;

#ifdef	DUMP_STACK_BACKTRACE
	DEBUGPRINT("Stack backtrace for thread %x", aWinThread);
#endif

	// Walk the call stack
	for (TInt i = 0 ; i < KMaxSearchDepth ; ++i)
		{
#ifdef	DUMP_STACK_BACKTRACE
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

