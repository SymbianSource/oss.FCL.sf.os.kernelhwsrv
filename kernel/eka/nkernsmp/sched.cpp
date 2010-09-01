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
// e32\nkernsmp\sched.cpp
// 
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

// TDfc member data
#define __INCLUDE_TDFC_DEFINES__

#include "nk_priv.h"
#include <nk_irq.h>

TSpinLock	NEventHandler::TiedLock(TSpinLock::EOrderEventHandlerTied);

/******************************************************************************
 * TScheduler
 ******************************************************************************/

// TScheduler resides in .bss so other fields are zero-initialised
TScheduler::TScheduler()
	:	iActiveCpus1(1),	// only boot CPU for now
		iActiveCpus2(1),	// only boot CPU for now
		iIdleSpinLock(TSpinLock::EOrderIdleDFCList),
		iCpusNotIdle(1)		// only boot CPU for now
	{
	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		{
		TSubScheduler* s = TheSubSchedulers + i;
		iSub[i] = s;
		s->iScheduler = this;
		s->iCpuNum = TUint32(i);
		s->iCpuMask = 1u<<i;
		}
	}


/** Return a pointer to the scheduler
	Intended for use by the crash debugger, not for general device driver use.

	@return	Pointer to the scheduler object
	@internalTechnology
 */
EXPORT_C TScheduler* TScheduler::Ptr()
	{
	return &TheScheduler;
	}


/******************************************************************************
 * TSubScheduler
 ******************************************************************************/

// TSubScheduler resides in .bss so other fields are zero-initialised
TSubScheduler::TSubScheduler()
	:	TPriListBase(KNumPriorities),
		iExIDfcLock(TSpinLock::EOrderExIDfcQ),
		iReadyListLock(TSpinLock::EOrderReadyList),
		iKernLockCount(1),
		iEventHandlerLock(TSpinLock::EOrderEventHandlerList)
	{
	}


/******************************************************************************
 * NSchedulable
 ******************************************************************************/
void NSchedulable::AcqSLock()
	{
	iSSpinLock.LockOnly();
	if (iParent!=this && iParent)
		iParent->AcqSLock();
	}

void NSchedulable::RelSLock()
	{
	if (iParent!=this && iParent)
		iParent->RelSLock();
	iSSpinLock.UnlockOnly();
	}

void NSchedulable::LAcqSLock()
	{
	NKern::Lock();
	AcqSLock();
	}

void NSchedulable::RelSLockU()
	{
	RelSLock();
	NKern::Unlock();
	}

void NSchedulable::UnPauseT()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR,"NSchedulable::UnPauseT");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T nUnPauseT",this));
	__NK_ASSERT_DEBUG(iPauseCount);
	if (--iPauseCount || iReady || iSuspended || (iParent && ((NThread*)this)->iWaitState.ThreadIsBlocked()))
		return;
	ReadyT(EUnPause);
	}

void NSchedulable::DeferredReadyIDfcFn(TAny* aPtr)
	{
	NSchedulable* a = (NSchedulable*)aPtr;
	a->AcqSLock();
	TUint32 evs = __e32_atomic_and_acq32(&a->iEventState, ~EDeferredReady);
	if (evs & EDeferredReady)
		{
		if (a->iParent)
			{
			// thread
			a->UnPauseT();
			}
		else
			{
			// thread group
			NThreadGroup* g = (NThreadGroup*)a;
			__KTRACE_OPT(KNKERN,DEBUGPRINT("%G nDeferredReady",g));
			__NK_ASSERT_DEBUG(g->iPauseCount);
			if (--g->iPauseCount && g->iNThreadList.NonEmpty())
				g->ReadyT(EUnPause);
			}
		}
	a->RelSLock();
	}

TInt NSchedulable::AddTiedEvent(NEventHandler* aEvent)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T AddEv %08x",this,aEvent));
	TInt r = KErrGeneral;
	NEventHandler::TiedLock.LockOnly();
	AcqSLock();
	if (iStopping)
		r = KErrDied;
	else if (!aEvent->iTied)
		{
		aEvent->iTied = this;
		iEvents.Add(&aEvent->iTiedLink);
		r = KErrNone;
		}
	RelSLock();
	NEventHandler::TiedLock.UnlockOnly();
	return r;
	}

void ipi_dummy(TGenericIPI*)
	{
	}

/** Detach and cancel any tied events attached to this thread/group

Call in a thread context with interrupts and preemption enabled.
Calling thread in critical section.

@internalComponent
*/
void NSchedulable::DetachTiedEvents()
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T DetTiedEv",this));
	NKern::Lock();
	NEventHandler::TiedLock.LockOnly();
	AcqSLock();
	iStopping = TRUE;
	if (!iParent)
		{
		// can't destroy a group until all threads have detached from it
		NThreadGroup* g = (NThreadGroup*)this;
		__NK_ASSERT_ALWAYS(g->iThreadCount==0 && g->iNThreadList.IsEmpty());
		}
	RelSLock();
	NEventHandler::TiedLock.UnlockOnly();

	// send IPI to all processors to synchronise
	// after this, any tied IDFCs can only proceed to completion
	// they can't be queued again
	TGenericIPI ipi;
	ipi.QueueAllOther(&ipi_dummy);
	NKern::Unlock();
	ipi.WaitCompletion();

	FOREVER
		{
		NKern::Lock();
		NEventHandler::TiedLock.LockOnly();
		AcqSLock();
		NEventHandler* h = 0;
		TInt type = -1;
		if (!iEvents.IsEmpty())
			{
			h = _LOFF(iEvents.First()->Deque(), NEventHandler, iTiedLink);
			h->iTiedLink.iNext = 0;
			type = h->iHType;
			}
		RelSLock();
		if (type == NEventHandler::EEventHandlerNTimer)
			{
			// everything's easy for a timer since we can just cancel it here
			NTimer* tmr = (NTimer*)h;
			tmr->DoCancel(NTimer::ECancelDestroy);
			tmr->iTied = 0;
			}
		else if (type == NEventHandler::EEventHandlerIDFC)
			{
			// can just cancel the IDFC with TiedLock held
			// EndTiedEvent() may be delayed, but we wait for that further down
			// iTied will have been captured before the IDFC state is reset
			// Cancel() waits for the state to be reset
			TDfc* d = (TDfc*)h;
			d->Cancel();
			d->iHType = (TUint8)NEventHandler::EEventHandlerDummy;
			d->iTied = 0;
			}
		NEventHandler::TiedLock.UnlockOnly();
		NKern::Unlock();
		if (!h)
			break;
		switch (type)
			{
			case NEventHandler::EEventHandlerIrq:
				{
				NIrqHandler* pH = (NIrqHandler*)h;
				// pH can't have been freed since we dequeued it but left iTied set
				pH->Unbind(pH->iHandle, this);
				break;
				}
			case NEventHandler::EEventHandlerNTimer:
			case NEventHandler::EEventHandlerIDFC:
			case NEventHandler::EEventHandlerDummy:
				// nothing left to do
				break;
			default:
				__NK_ASSERT_ALWAYS(0);
				break;
			}
		}

	// Wait for any remaining tied event handlers to complete
	while (iEventState & EEventCountMask)
		{
		__chill();
		}
	}

/******************************************************************************
 * NThreadGroup
 ******************************************************************************/


/******************************************************************************
 * NThreadBase
 ******************************************************************************/

/** Makes a nanothread ready.
	
	For use by RTOS personality layers.

	@pre	Kernel must be locked.
	@pre	Call either in a thread or an IDFC context.
	@pre	The thread being made ready must not be explicitly suspended
	
	@post	Kernel is locked.
 */
void NSchedulable::ReadyT(TUint aMode)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR,"NSchedulable::ReadyT");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T nReadyT(%x)",this,aMode));
	NThreadBase* t = (NThreadBase*)this;
#ifdef _DEBUG
	if (!iParent)
		t = (NThreadBase*)0xface0fff;
#endif
	__NK_ASSERT_DEBUG(!iReady && (!iParent || (!t->iWaitState.iWtC.iWtStFlags && !t->iPauseCount && !t->iSuspended)));
	TSubScheduler& ss0 = SubScheduler();
	NSchedulable* g = this;
	if (iParent != this && iParent)
		{
		NThreadGroup* tg = (NThreadGroup*)iParent;
		iReady = EReadyGroup;
		if (tg->iReady)
			{
			// extra thread added to group - change priority if necessary
			tg->iNThreadList.Add(this);
			TInt gp = tg->iPriority;
			TSubScheduler& ss = TheSubSchedulers[tg->iReady & EReadyCpuMask];
			ss.iReadyListLock.LockOnly();
			TInt hp = ss.HighestPriority();
			if (iPriority>gp)
				ss.ChangePriority(tg, iPriority);
			if (iPriority>hp || (iPriority==hp && ss.iCurrentThread && ss.iCurrentThread->iTime==0))
				{
				if (&ss == &ss0)
					RescheduleNeeded();					// reschedule on this processor
				else
					ss0.iReschedIPIs |= ss.iCpuMask;	// will kick the other CPU when this CPU reenables preemption
				}
			if ((aMode & ENewTimeslice) && t->iTime==0 && (iNext!=this || ss.iQueue[iPriority]))
				t->iTime = t->iTimeslice;
			ss.iReadyListLock.UnlockOnly();
			return;
			}
		tg->iNThreadList.Add(this);
		tg->iPriority = iPriority;	// first in group
		g = tg;						// fall through to add group to subscheduler
		}
	TInt cpu = -1;
	if (aMode & EUnPause)
		{
		cpu = (g->iEventState & EThreadCpuMask)>>EThreadCpuShift;
		if (CheckCpuAgainstAffinity(cpu, g->iCpuAffinity))
			goto cpu_ok;
		}
	else if (g->iFreezeCpu)
		{
		cpu = g->iLastCpu;
		if (!CheckCpuAgainstAffinity(cpu, g->iCpuAffinity))
			g->iCpuChange = TRUE;
		}
	else if (!(g->iCpuAffinity & NTHREADBASE_CPU_AFFINITY_MASK))
		cpu = g->iCpuAffinity;
	else if ((aMode & EPreferSameCpu) && (g->iCpuAffinity & ss0.iCpuMask))
		cpu = ss0.iCpuNum;
	if (cpu < 0)
		{
		// pick a cpu
		TScheduler& s = TheScheduler;
		TUint32 m = g->iCpuAffinity & s.iActiveCpus1;
		TInt i;
		TInt lowest_p = KMaxTInt;
		for (i=0; i<s.iNumCpus; ++i)
			{
			TSubScheduler& ss = *s.iSub[i];
			if (!(m & ss.iCpuMask))
				continue;
			TInt hp = ss.HighestPriority();
			if (hp < lowest_p)
				{
				lowest_p = hp;
				cpu = i;
				continue;
				}
			if (hp > lowest_p)
				continue;
			if (cpu>=0 && g->iLastCpu!=i)
				continue;
			lowest_p = hp;
			cpu = i;
			}
		}
cpu_ok:
	__NK_ASSERT_ALWAYS(cpu>=0);
	if (g->TiedEventReadyInterlock(cpu))
		{
		__KTRACE_OPT(KSCHED2,DEBUGPRINT("ReadyT->CPU %dD",cpu));
		++g->iPauseCount;
//		((TDfc*)g->i_IDfcMem)->Add();
		return;
		}
	__KTRACE_OPT(KSCHED2,DEBUGPRINT("ReadyT->CPU %d",cpu));
	TSubScheduler& ss = TheSubSchedulers[cpu];
	ss.iReadyListLock.LockOnly();
	TInt hp = ss.HighestPriority();
	if (g->iPriority>hp || (g->iPriority==hp && ss.iCurrentThread && ss.iCurrentThread->iTime==0))
		{
		if (&ss == &ss0)
			RescheduleNeeded();					// reschedule on this processor
		else
			ss0.iReschedIPIs |= ss.iCpuMask;	// will kick the other CPU when this CPU reenables preemption
		}
	ss.Add(g);
	g->iReady = TUint8(cpu | EReadyOffset);
	if ((aMode & ENewTimeslice) && iParent && t->iTime==0 && g->iNext!=g)
		t->iTime = t->iTimeslice;
	ss.iReadyListLock.UnlockOnly();
	}


NThread* TSubScheduler::SelectNextThread()
	{
	NThread* ot = iCurrentThread;
	NThread* t = 0;
	TBool migrate = FALSE;
	TBool gmigrate = FALSE;
	TBool fmd_done = FALSE;
	TBool fmd_res = FALSE;
	if (!ot)
		{
		iReadyListLock.LockOnly();
		iRescheduleNeededFlag = FALSE;
		goto no_ot;
		}
	ot->AcqSLock();
	if (ot->iNewParent)
		ot->iNewParent->AcqSLock();
	SaveTimesliceTimer(ot);	// remember how much of current thread's timeslice remains
	if (ot->iCsFunction==NThreadBase::ECSDivertPending && ot->iWaitState.iWtC.iWtStFlags)
		{
		// thread about to exit so cancel outstanding wait
		ot->DoReleaseT(KErrDied,0);
		}
	if (ot->iWaitState.iWtC.iWtStFlags==0)
		{
		// ASSUMPTION: If iNewParent set, ot can't hold a fast mutex (assertion in JoinGroup)
		TBool pfmd = (ot->iParent!=ot && !ot->iFastMutexDefer);
		if (ot->iTime==0 || pfmd)
			{
			// ot's timeslice has expired
			fmd_res = ot->CheckFastMutexDefer();
			fmd_done = TRUE;
			if (fmd_res)
				{
				if (ot->iTime == 0)
					ot->iTime = 0x80000000;	// mark deferred timeslice expiry
				if (pfmd)
					{
					ot->iFastMutexDefer = 1;
					++ot->iParent->iFreezeCpu;
					}
				}
			}
		}
	iReadyListLock.LockOnly();
	iRescheduleNeededFlag = FALSE;

	//	process outstanding suspend/kill/CPU change on ot

	__NK_ASSERT_DEBUG(!(ot->iWaitState.iWtC.iWtStFlags & NThreadWaitState::EWtStWaitActive));
	if (ot->iWaitState.iWtC.iWtStFlags || ot->iPauseCount || ot->iSuspended)
		{
		// ot is no longer ready to run
		__KTRACE_OPT(KSCHED2,DEBUGPRINT("Rschd<-%T WS: %02x %02x (%08x) P:%02x S:%1x", ot, 
							ot->iWaitState.iWtC.iWtStFlags, ot->iWaitState.iWtC.iWtObjType, ot->iWaitState.iWtC.iWtObj, ot->iPauseCount, ot->iSuspended));
		TInt wtst = ot->iWaitState.DoWait();
		if (wtst>=0 && wtst!=NThread::EWaitFastMutex)
			ot->iTime = ot->iTimeslice;
		ot->UnReadyT();
		if (ot->iNewParent)
			{
			ot->iParent = ot->iNewParent, ++((NThreadGroup*)ot->iParent)->iThreadCount;
			wmb();	// must make sure iParent is updated before iNewParent is cleared
			ot->iNewParent = 0;
			}
		ot->iCpuChange = FALSE;
		}
	else if (ot->iNewParent)
		{
		__NK_ASSERT_ALWAYS(ot->iParent==ot && !ot->iHeldFastMutex && !ot->iFreezeCpu);
		ot->UnReadyT();
		migrate = TRUE;
		ot->iParent = ot->iNewParent;
		ot->iCpuChange = FALSE;
		++((NThreadGroup*)ot->iParent)->iThreadCount;
		wmb();	// must make sure iParent is updated before iNewParent is cleared
		ot->iNewParent = 0;
		}
	else if (ot->iParent->iCpuChange && !ot->iParent->iFreezeCpu)
		{
		if (!CheckCpuAgainstAffinity(iCpuNum, ot->iParent->iCpuAffinity))
			{
			if (ot->iParent==ot)
				{
				if (!fmd_done)
					fmd_res = ot->CheckFastMutexDefer(), fmd_done = TRUE;
				if (!fmd_res)
					{
					__KTRACE_OPT(KSCHED2,DEBUGPRINT("Rschd<-%T A:%08x",ot,ot->iParent->iCpuAffinity));
					ot->UnReadyT();
					migrate = TRUE;
					ot->iCpuChange = FALSE;
					}
				}
			else
				{
				__KTRACE_OPT(KSCHED2,DEBUGPRINT("Rschd<-%T GA:%08x",ot,ot->iParent->iCpuAffinity));
				Remove(ot->iParent);
				ot->iParent->iReady = 0;
				gmigrate = TRUE;
				ot->iCpuChange = FALSE;
				ot->iParent->iCpuChange = FALSE;
				}
			}
		else
			{
			ot->iCpuChange = FALSE;
			ot->iParent->iCpuChange = FALSE;
			}
		}
no_ot:
	NSchedulable* g = (NSchedulable*)First();
	TBool rrcg = FALSE;
	if (g && g->IsGroup())
		{
		t = (NThread*)((NThreadGroup*)g)->iNThreadList.First();
		if (g->iNext!=g)
			rrcg = TRUE;
		}
	else
		t = (NThread*)g;
	TBool rrct = (t && t->iNext!=t);
	if (t && t->iTime==0 && (rrcg || rrct))
		{
		// candidate thread's timeslice has expired and there is another at the same priority
		if (t==ot)
			{
			if (ot->iParent!=ot)
				{
				((NThreadGroup*)ot->iParent)->iNThreadList.iQueue[ot->iPriority] = ot->iNext;
				iQueue[ot->iParent->iPriority] = ot->iParent->iNext;
				}
			else
				iQueue[ot->iPriority] = ot->iNext;
			ot->iTime = ot->iTimeslice;
			NSchedulable* g2 = (NSchedulable*)First();
			if (g2->IsGroup())
				t = (NThread*)((NThreadGroup*)g2)->iNThreadList.First();
			else
				t = (NThread*)g2;
			if (t->iTime==0)
				{
				// loop again since we need to lock t before round robining it
				__KTRACE_OPT(KSCHED2,DEBUGPRINT("Rschd<-%T RRL",ot));
				iRescheduleNeededFlag = TRUE;
				}
			else
				{
				__KTRACE_OPT(KSCHED2,DEBUGPRINT("Rschd<-%T RR",ot));
				}
/*			if (ot->iCpuAffinity & NTHREADBASE_CPU_AFFINITY_MASK)
				{
				ot->UnReadyT();
				migrate = TRUE;
				}
			else
				ot->iTime = ot->iTimeslice;
*/
			}
		else	// loop again since we need to lock t before round robining it
			{
			__KTRACE_OPT(KSCHED2,DEBUGPRINT("Rschd<-%T LL",ot));
			iRescheduleNeededFlag = TRUE;
			}
		}
	if (t != ot)
		{
		if (ot)
			{
			ot->iCurrent = 0;
			ot->iParent->iCurrent = 0;
			ot->CompleteContextSave();
			}
		if (t)
			{
			t->iLastCpu = iCpuNum;
			t->iParent->iLastCpu = iCpuNum;
			t->iCurrent = TUint8(iCpuNum | NSchedulable::EReadyOffset);
			t->iParent->iCurrent = t->iCurrent;
			}
		iCurrentThread = t;
		}
	UpdateThreadTimes(ot,t);		// update ot's run time and set up the timeslice timer for t
	iReadyListLock.UnlockOnly();
	if (migrate)
		ot->ReadyT(NThreadBase::ENewTimeslice);	// new timeslice if it's queued behind another thread at same priority
	if (gmigrate)
		ot->iParent->ReadyT(0);	// new timeslice if it's queued behind another thread at same priority
	if (ot)
		{
		ot->RelSLock();

		// DFC to signal thread is now dead
		if (ot->iWaitState.ThreadIsDead() && ot->iWaitState.iWtC.iKillDfc)
			ot->iWaitState.iWtC.iKillDfc->DoEnque();
		}
	__KTRACE_OPT(KSCHED,DEBUGPRINT("Rschd->%T",t));
	__NK_ASSERT_ALWAYS(!t || t->iParent);	// must be a thread not a group
	return t;	// could return NULL
	}


void NThreadBase::UnReadyT()
	{
	if (iParent!=this)
		{
		NThreadGroup& g = *(NThreadGroup*)iParent;
		TPriListBase& l = g.iNThreadList;
		l.Remove(this);
		if (g.iReady)
			{
			TSubScheduler& ss = TheSubSchedulers[g.iReady & EReadyCpuMask];
			if (l.IsEmpty())
				{
//				__KTRACE_OPT(KNKERN,DEBUGPRINT("%T UnReadyT (G=%G-)",this,&g));
				ss.Remove(&g);
				g.iReady = 0;
				g.iPriority = 0;
				}
			else
				{
//				__KTRACE_OPT(KNKERN,DEBUGPRINT("%T UnReadyT (G=%G)",this,&g));
				ss.ChangePriority(&g, l.HighestPriority());
				}
			}
		}
	else
		{
//		__KTRACE_OPT(KNKERN,DEBUGPRINT("%T UnReadyT",this));
		TheSubSchedulers[iReady & EReadyCpuMask].Remove(this);
		}
	iReady = 0;
	}


void NThreadBase::ChangeReadyThreadPriority()
	{
	TInt newp = iMutexPri>iBasePri ? iMutexPri : iBasePri;
	TInt oldp = iPriority;
	TSubScheduler* ss0 = &SubScheduler();
	TSubScheduler* ss = 0;
	if (iParent->iReady)
		{
		ss = TheSubSchedulers + (iParent->iReady & EReadyCpuMask);
		ss->iReadyListLock.LockOnly();
		}
	TBool resched = FALSE;
	NSchedulable* g = iParent;
	if (g!=this)
		{
		NThreadGroup* tg = (NThreadGroup*)g;
		tg->iNThreadList.ChangePriority(this, newp);
		if (ss)
			{
			TInt ngp = tg->iNThreadList.HighestPriority();
			if (ngp!=tg->iPriority)
				ss->ChangePriority(tg, ngp);
			}
		}
	else
		ss->ChangePriority(this, newp);
	if (iCurrent)	// can't be current if parent not ready
		{
		TInt nhp = ss->HighestPriority();
		if (newp<oldp && (newp<nhp || (newp==nhp && iTime==0)))
			resched = TRUE;
		}
	else if (ss)
		{
		NThreadBase* ct = ss->iCurrentThread;
		TInt cp = ct ? ct->iPriority : -1;
		if (newp>cp || (newp==cp && ct->iTime==0))
			resched = TRUE;
		}
	if (resched)
		{
		if (ss == ss0)
			RescheduleNeeded();
		else
			ss0->iReschedIPIs |= ss->iCpuMask;	// will kick the other CPU when this CPU reenables preemption
		}
	if (ss)
		ss->iReadyListLock.UnlockOnly();
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
	AcqSLock();
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T nSetPri %d(%d)->%d(%d)",this,iPriority,iBasePri,newp,iMutexPri));
	iBasePri = TUint8(newp);
	if (iMutexPri > iBasePri)
		newp = iMutexPri;
	TInt oldp = iPriority;
	if (newp == oldp)
		{
		RelSLock();
		return;
		}
	NFastMutex* wfm = 0;
	if (iLinkedObj && iLinkedObjType==EWaitFastMutex)
		wfm = (NFastMutex*)iLinkedObj;
	if (wfm)
		{
		// if thread is attached to/waiting on a fast mutex, need to acquire mutex lock
		++iPauseCount;
		RelSLock();
		wfm->iMutexLock.LockOnly();
		AcqSLock();
		UnPauseT();
		wfm->iWaitQ.ChangePriority(&iWaitLink, newp);	// change position of this thread on mutex wait queue
		}
	if (iReady)
		{
		ChangeReadyThreadPriority();
		RelSLock();
		if (wfm && newp<=wfm->iWaitQ.HighestPriority())
			{
			// this thread was contending for the mutex but they may be other waiting threads
			// with higher or equal priority, so wake up the first thread on the list.
			NThreadBase* pT = _LOFF(wfm->iWaitQ.First(), NThreadBase, iWaitLink);
			pT->AcqSLock();

			// if thread is still blocked on this fast mutex, release it but leave it on the wait queue
			// NOTE: it can't be suspended
			pT->iWaitState.UnBlockT(NThreadBase::EWaitFastMutex, wfm, KErrNone);
			pT->RelSLock();
			}
		}
	else
		{
		iPriority = (TUint8)newp;
		if (wfm && newp>oldp)
			{
			NThreadBase* pT = _LOFF(wfm->iWaitQ.First(), NThreadBase, iWaitLink);	// highest priority waiting thread
			if (pT==this)
				{
				// this is now highest priority waiting thread so wake it up
				iWaitState.UnBlockT(NThreadBase::EWaitFastMutex, wfm, KErrNone);
				}
			}
		RelSLock();
		}
	if (wfm)
		{
		NThreadBase* t = (NThreadBase*)(TLinAddr(wfm->iHoldingThread)&~3);
		if (t)
			t->SetMutexPriority(wfm);
		wfm->iMutexLock.UnlockOnly();
		}
	}


/** Set the inherited priority of a nanokernel thread.

	@pre	Kernel must be locked.
	@pre	Call in a thread context.
	@pre	The thread holds a fast mutex
	
	@post	Kernel is locked.
 */
void NThreadBase::SetMutexPriority(NFastMutex* aM)
	{
	TInt newp = aM->iWaitQ.HighestPriority();
	if (newp<0)
		newp = 0;
	AcqSLock();
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T nSetMPri %d->%d Base %d (mutex %08x)",this,iMutexPri,newp,iBasePri,aM));
	iMutexPri = TUint8(newp);
	if (iMutexPri < iBasePri)
		newp = iBasePri;
	TInt oldp = iPriority;
	if (newp == oldp)
		{
		RelSLock();
		return;
		}
	if (iReady)
		ChangeReadyThreadPriority();
	else
		iPriority = (TUint8)newp;
	RelSLock();
	}


void NThreadBase::LoseInheritedPriorityT()
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T nLoseInhPri %d->%d",this,iPriority,iBasePri));
	TSubScheduler* ss = &SubScheduler();
	TInt newp = iBasePri;
	NSchedulable* g = iParent;
	ss->iReadyListLock.LockOnly();
	if (g!=this)
		{
		NThreadGroup* tg = (NThreadGroup*)g;
		tg->iNThreadList.ChangePriority(this, newp);
		TInt hp = tg->iNThreadList.HighestPriority();
		if (hp == tg->iPriority)
			{
			if (newp <= hp)
				RescheduleNeeded();
			goto out;
			}
		newp = hp;
		g = tg;
		}
	if (newp <= ss->HighestPriority())
		RescheduleNeeded();
	ss->ChangePriority(g, newp);
out:
	ss->iReadyListLock.UnlockOnly();
	}


