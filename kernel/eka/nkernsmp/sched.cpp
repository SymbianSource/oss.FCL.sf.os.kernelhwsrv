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


const TUint8 KClassFromPriority[KNumPriorities] =
	{
	0,	0,	0,	0,	0,	0,	0,	0,				// priorities 0-7
	0,	0,	0,	0,	1,	1,	1,	1,				// priorities 8-15
	2,	2,	2,	2,	2,	2,	2,	2,				// priorities 16-23
	2,	2,	2,	3,	3,	3,	3,	3,				// priorities 24-31
	3,	3,	3,	3,	3,	3,	3,	3,				// priorities 32-39
	3,	3,	3,	3,	3,	3,	3,	3,				// priorities 40-47
	3,	3,	3,	3,	3,	3,	3,	3,				// priorities 48-55
	3,	3,	3,	3,	3,	3,	3,	3				// priorities 56-63
	};


/******************************************************************************
 * TScheduler
 ******************************************************************************/

// TScheduler resides in .bss so other fields are zero-initialised
TScheduler::TScheduler()
	:	iThreadAcceptCpus(1),	// only boot CPU for now
		iIpiAcceptCpus(1),		// only boot CPU for now
		iGenIPILock(TSpinLock::EOrderGenericIPIList),
		iIdleBalanceLock(TSpinLock::EOrderEnumerate),
		iIdleSpinLock(TSpinLock::EOrderIdleDFCList),
		iCpusNotIdle(1),	// only boot CPU for now
		iEnumerateLock(TSpinLock::EOrderEnumerate),
		iBalanceListLock(TSpinLock::EOrderReadyList),
		iBalanceTimer(&BalanceTimerExpired, this, 1),
		iCCSyncIDFC(&CCSyncDone, 0),
		iCCReactivateDfc(&CCReactivateDfcFn, this, 3),
		iCCRequestLevel(1),		// only boot CPU for now
		iCCRequestDfc(&CCRequestDfcFn, this, 2),
		iCCPowerDownDfc(&CCIndirectPowerDown, this, 0),
		iCCIpiReactIDFC(&CCIpiReactivateFn, this),
		iFreqChgDfc(&DoFrequencyChanged, this, 6)
	{
	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		{
		TSubScheduler* s = TheSubSchedulers + i;
		iSub[i] = s;
		s->iScheduler = this;
		s->iCpuNum = TUint32(i);
		s->iCpuMask = 1u<<i;
		s->iLbCounter = TUint8(NSchedulable::ELbState_PerCpu + i);
		}
	iLbCounter = (TUint8)NSchedulable::ELbState_Global;
	iNeedBal = 1;	// stop anyone trying to kick rebalancer before it has been created
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
	:	iExIDfcLock(TSpinLock::EOrderExIDfcQ),
		iReadyListLock(TSpinLock::EOrderReadyList),
		iKernLockCount(1),
		iEventHandlerLock(TSpinLock::EOrderEventHandlerList)
	{
	}

void TSubScheduler::SSAddEntry(NSchedulable* aEntry)
	{
	if (aEntry->iParent!=aEntry || !((NThreadBase*)aEntry)->i_NThread_Initial)
		{
		TInt c = KClassFromPriority[aEntry->iPriority];
		++iPriClassThreadCount[c];
		++iRdyThreadCount;
		}
	iSSList.Add(aEntry);
	}

void TSubScheduler::SSAddEntryHead(NSchedulable* aEntry)
	{
	if (aEntry->iParent!=aEntry || !((NThreadBase*)aEntry)->i_NThread_Initial)
		{
		TInt c = KClassFromPriority[aEntry->iPriority];
		++iPriClassThreadCount[c];
		++iRdyThreadCount;
		}
	iSSList.AddHead(aEntry);
	}

void TSubScheduler::SSRemoveEntry(NSchedulable* aEntry)
	{
	if (aEntry->iParent!=aEntry || !((NThreadBase*)aEntry)->i_NThread_Initial)
		{
		TInt c = KClassFromPriority[aEntry->iPriority];
		--iPriClassThreadCount[c];
		--iRdyThreadCount;
		}
	iSSList.Remove(aEntry);
	}

void TSubScheduler::SSChgEntryP(NSchedulable* aEntry, TInt aNewPriority)
	{
	if (aEntry->iParent!=aEntry || !((NThreadBase*)aEntry)->i_NThread_Initial)
		{
		TInt c0 = KClassFromPriority[aEntry->iPriority];
		TInt c1 = KClassFromPriority[aNewPriority];
		if (c0 != c1)
			{
			--iPriClassThreadCount[c0];
			++iPriClassThreadCount[c1];
			}
		}
	iSSList.ChangePriority(aEntry, aNewPriority);
	}


/******************************************************************************
 * NSchedulable
 ******************************************************************************/
TUint32 NSchedulable::PreprocessCpuAffinity(TUint32 aAffinity)
	{
	if (!(aAffinity & NTHREADBASE_CPU_AFFINITY_MASK))
		return aAffinity;
	TUint32 x = aAffinity & ~NTHREADBASE_CPU_AFFINITY_MASK;
	if (x & (x-1))
		return aAffinity;
	return __e32_find_ls1_32(x);
	}

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


/** Return the total CPU time so far used by the specified thread.

	@return The total CPU time in units of 1/NKern::CpuTimeMeasFreq().
*/
EXPORT_C TUint64 NKern::ThreadCpuTime(NThread* aThread)
	{
	NSchedulable::SCpuStats stats;
	NKern::Lock();
	aThread->GetCpuStats(NSchedulable::E_RunTime, stats);
	NKern::Unlock();
	return stats.iRunTime;
	}

void NSchedulable::GetCpuStats(TUint aMask, NSchedulable::SCpuStats& aOut)
	{
	AcqSLock();
	GetCpuStatsT(aMask, aOut);
	RelSLock();
	}

void NSchedulable::GetCpuStatsT(TUint aMask, NSchedulable::SCpuStats& aOut)
	{
	TSubScheduler* ss = 0;
	NThread* t = 0;
	TBool initial = FALSE;
	if (!IsGroup())
		t = (NThread*)this;
	if (t && t->i_NThread_Initial)
		ss = &TheSubSchedulers[iLastCpu], initial = TRUE;
	else if (iReady)
		{
		if (IsGroup())
			ss = &TheSubSchedulers[iReady & NSchedulable::EReadyCpuMask];
		else if (iParent->iReady)
			ss = &TheSubSchedulers[iParent->iReady & NSchedulable::EReadyCpuMask];
		}
	if (ss)
		ss->iReadyListLock.LockOnly();
	TUint64 now = NKern::Timestamp();
	if (aMask & (E_RunTime|E_RunTimeDelta))
		{
		aOut.iRunTime = iTotalCpuTime.i64;
		if (iCurrent || (initial && !ss->iCurrentThread))
			aOut.iRunTime += (now - ss->iLastTimestamp.i64);
		if (aMask & E_RunTimeDelta)
			{
			aOut.iRunTimeDelta = aOut.iRunTime - iSavedCpuTime.i64;
			iSavedCpuTime.i64 = aOut.iRunTime;
			}
		}
	if (aMask & (E_ActiveTime|E_ActiveTimeDelta))
		{
		aOut.iActiveTime = iTotalActiveTime.i64;
		if (iActiveState)
			aOut.iActiveTime += (now - iLastActivationTime.i64);
		if (aMask & E_ActiveTimeDelta)
			{
			aOut.iActiveTimeDelta = aOut.iActiveTime - iSavedActiveTime.i64;
			iSavedActiveTime.i64 = aOut.iActiveTime;
			}
		}
	if (aMask & E_LastRunTime)
		{
		if (iCurrent)
			aOut.iLastRunTime = 0;
		else
			aOut.iLastRunTime = now - iLastRunTime.i64;
		}
	if (aMask & E_LastActiveTime)
		{
		if (iActiveState)
			aOut.iLastActiveTime = 0;
		else
			aOut.iLastActiveTime = now - iLastRunTime.i64;
		}
	if (ss)
		ss->iReadyListLock.UnlockOnly();
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
	if (iParent && !iActiveState)
		{
		iActiveState=1;
		iLastActivationTime.i64 = NKern::Timestamp();
		if (iParent!=this && ++iParent->iActiveState==1)
			iParent->iLastActivationTime.i64 = iLastActivationTime.i64;
		}
#ifdef _DEBUG
	if (!iParent)
		t = (NThreadBase*)0xface0fff;
#endif
	__NK_ASSERT_DEBUG(!iReady && (!iParent || (!t->iWaitState.iWtC.iWtStFlags && !t->iSuspended)));
	TSubScheduler& ss0 = SubScheduler();
	TScheduler& s = TheScheduler;
	TBool reactivate = FALSE;
	TBool no_ipi = FALSE;
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
				{
				ss.SSChgEntryP(tg, iPriority);
				}
			if (iPriority>hp || (iPriority==hp && ss.iCurrentThread && ss.iCurrentThread->iTime==0))
				{
				if (&ss == &ss0)
					RescheduleNeeded();					// reschedule on this processor
				else
					ss0.iReschedIPIs |= ss.iCpuMask;	// will kick the other CPU when this CPU reenables preemption
				}
			if ((aMode & ENewTimeslice) && t->iTime==0 && (iNext!=this || ss.EntryAtPriority(iPriority)) )
				t->iTime = t->iTimeslice;
			ss.iReadyListLock.UnlockOnly();

			ss0.iMadeReadyCounter++;
			return;
			}
		tg->iNThreadList.Add(this);
		tg->iPriority = iPriority;	// first in group
		g = tg;						// fall through to add group to subscheduler
		}
	TInt priClass = -1;
	TInt cpu = -1;
	TUint32 active = TheScheduler.iThreadAcceptCpus;
	if (g!=t || !t->i_NThread_Initial)
		priClass = KClassFromPriority[g->iPriority];
	if (g->iForcedCpu)
		{
		cpu = iForcedCpu & EReadyCpuMask;	// handles core cycling case (No.1 below)
		if (active & (1u<<cpu))
			goto cpu_ok;
		else
			goto single_cpu_reactivate;
		}
	if (aMode & EUnPause)
		{
		cpu = (g->iEventState & EThreadCpuMask)>>EThreadCpuShift;
		if (CheckCpuAgainstAffinity(cpu, g->iCpuAffinity))
			goto cpu_ok;
		cpu = -1;
		}
	if (g->iFreezeCpu)
		{
		cpu = g->iLastCpu;
		goto cpu_ok;
		}
	if (!(g->iCpuAffinity & NTHREADBASE_CPU_AFFINITY_MASK))
		{
		cpu = g->iCpuAffinity;
		if (!(active & (1u<<cpu)))
			goto single_cpu_reactivate;
		goto cpu_ok;
		}
	if ((aMode & EPreferSameCpu) && CheckCpuAgainstAffinity(ss0.iCpuNum, g->iCpuAffinity, active))
		cpu = ss0.iCpuNum;
	else if (iTransientCpu && CheckCpuAgainstAffinity(iTransientCpu & EReadyCpuMask, g->iCpuAffinity))
		cpu = iTransientCpu & EReadyCpuMask;
	else if (iPreferredCpu && CheckCpuAgainstAffinity(iPreferredCpu & EReadyCpuMask, g->iCpuAffinity, active))
		cpu = iPreferredCpu & EReadyCpuMask;
	if (cpu < 0)
		{
		// pick a cpu
		TUint32 m = g->iCpuAffinity & active;
		TInt lastCpu = g->iLastCpu;
		TInt i = lastCpu;
		TInt lcp = KMaxTInt;
		TInt lco = KMaxTInt;
		TInt cpunp = -1;
		TInt idle_cpu = -1;
		do	{
			if (m & (1u<<i))
				{
				TSubScheduler& ss = *s.iSub[i];
				TInt nInC = ss.iPriClassThreadCount[priClass];
				if (nInC < lco)
					lco=nInC, cpunp=i;
				TInt hp = ss.HighestPriority();
				if (idle_cpu<0 && hp<=0)
					idle_cpu = i;
				if (hp < iPriority)
					{
					if (i == lastCpu)
						{
						cpu = i;
						if (hp <= 0)
							break;
						lcp = -1;
						}
					if (nInC < lcp)
						lcp=nInC, cpu=i;
					}
				}
			if (++i == s.iNumCpus)
				i = 0;
			} while (i != lastCpu);
		if (idle_cpu>=0 && cpu!=idle_cpu)
			cpu = idle_cpu;
		else if (cpu<0)
			cpu = cpunp;
		}
	if (cpu<0)
		{
single_cpu_reactivate:
		/*	CORE_CONTROL
			Might have no CPU at this point due to all CPUs specified by
			iCpuAffinity being off or in the process of shutting down.
			There are three possibilities:
			1.	This thread is 'core cycling'. In that case it will be
				allowed to move to a 'shutting down' CPU. The CPU will
				not be permitted to shut down entirely until all core cycling
				has completed. This is already handled above.
			2.	There are one or more CPUs which this thread could run on which
				are shutting down. In that case, pick one, abort the shutdown
				process and put this thread on it.
			3.	All CPUs which this thread can run on are off. In that case,
				assign the thread to one of them and initiate power up of that core.
		*/
		TUint32 affm = AffinityToMask(g->iCpuAffinity);
		TInt irq = s.iGenIPILock.LockIrqSave();
		if (cpu < 0)
			{
			if (affm & s.iCCReactivateCpus)
				cpu = __e32_find_ls1_32(affm & s.iCCReactivateCpus);
			else if (affm & s.iIpiAcceptCpus)
				cpu = __e32_find_ls1_32(affm & s.iIpiAcceptCpus);
			else
				cpu = __e32_find_ls1_32(affm), no_ipi = TRUE;
			}
		TUint32 cm = 1u<<cpu;
		if (!((s.iCCReactivateCpus|s.iThreadAcceptCpus) & cm))
			{
			s.iCCReactivateCpus |= (1u<<cpu);
			reactivate = TRUE;
			}
		s.iGenIPILock.UnlockIrqRestore(irq);
		}
cpu_ok:
	__NK_ASSERT_ALWAYS(cpu>=0);
	if (g->iFreezeCpu && !CheckCpuAgainstAffinity(cpu, g->iCpuAffinity))
		g->iCpuChange = TRUE;
	if (g->TiedEventReadyInterlock(cpu))
		{
		__KTRACE_OPT(KSCHED2,DEBUGPRINT("ReadyT->CPU %dD",cpu));
		++g->iPauseCount;
		}
	else
		{
		__KTRACE_OPT(KSCHED2,DEBUGPRINT("ReadyT->CPU %d",cpu));
		TSubScheduler& ss = TheSubSchedulers[cpu];
		ss.iReadyListLock.LockOnly();
		TInt hp = ss.HighestPriority();
		if (g->iPriority>hp || (g->iPriority==hp && ss.iCurrentThread && ss.iCurrentThread->iTime==0))
			{
			if (&ss == &ss0)
				RescheduleNeeded();					// reschedule on this processor
			else if (!no_ipi)
				ss0.iReschedIPIs |= ss.iCpuMask;	// will kick the other CPU when this CPU reenables preemption
			}
		ss.SSAddEntry(g);
		g->iReady = TUint8(cpu | EReadyOffset);
		if ((aMode & ENewTimeslice) && iParent && t->iTime==0 && g->iNext!=g)
			t->iTime = t->iTimeslice;
		if (!g->iLbLink.iNext && !(g->iParent && t->i_NThread_Initial))
			{
			ss.iLbQ.Add(&g->iLbLink);
			g->iLbState = ss.iLbCounter;
			if (!s.iNeedBal && (!g->iParent || !(t->iRebalanceAttr & 1)))
				{
				s.iNeedBal = 1;
				reactivate = TRUE;
				}
			}
		if (g->iForcedCpu == g->iReady)
			{
			g->iLastCpu = (TUint8)cpu;
			g->iForcedCpu = 0;	// iForcedCpu has done its job - iFreezeCpu will keep the thread on the right CPU
			}
		ss.iReadyListLock.UnlockOnly();
		ss0.iMadeReadyCounter++;
		}
	if (reactivate)
		s.iCCReactivateDfc.Add();
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
			ot->iParent->iTransientCpu = 0;
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
		if (wtst==KErrDied || ot->iSuspended || (!(ot->iWaitState.iWtC.iWtStFlags & NThreadWaitState::EWtStObstructed) && wtst>=0) )
			{
			ot->iActiveState = 0;
			ot->iParent->iTransientCpu = 0;
			if (ot->iParent != ot)
				--ot->iParent->iActiveState;
			}
		ot->UnReadyT();
		if (ot->iNewParent)
			{
			ot->iParent = ot->iNewParent, ++((NThreadGroup*)ot->iParent)->iThreadCount;
			wmb();	// must make sure iParent is updated before iNewParent is cleared
			ot->iNewParent = 0;
			if (ot->iActiveState && ++ot->iParent->iActiveState==1)
				ot->iParent->iLastActivationTime.i64 = NKern::Timestamp();
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
		TUint64 now = NKern::Timestamp();
		if (!ot->iParent->iCurrent)
			ot->iParent->iLastStartTime.i64 = now;
		if (++ot->iParent->iActiveState==1)
			ot->iParent->iLastActivationTime.i64 = now;
		}
	else if (ot->iParent->iCpuChange)
		{
		if (ot->iForcedCpu)
			migrate = TRUE;
		else if (!ot->iParent->iFreezeCpu)
			{
			if (ot->iParent->ShouldMigrate(iCpuNum))
				{
				if (ot->iParent==ot)
					{
					if (!fmd_done)
						fmd_res = ot->CheckFastMutexDefer(), fmd_done = TRUE;
					if (!fmd_res)
						migrate = TRUE;
					}
				else
					gmigrate = TRUE;
				}
			else
				{
				ot->iCpuChange = FALSE;
				ot->iParent->iCpuChange = FALSE;
				}
			}
		if (migrate)
			{
			__KTRACE_OPT(KSCHED2,DEBUGPRINT("Rschd<-%T A:%08x",ot,ot->iParent->iCpuAffinity));
			ot->UnReadyT();
			ot->iCpuChange = FALSE;
			}
		else if (gmigrate)
			{
			__KTRACE_OPT(KSCHED2,DEBUGPRINT("Rschd<-%T GA:%08x",ot,ot->iParent->iCpuAffinity));
			SSRemoveEntry(ot->iParent);
			ot->iParent->iReady = 0;
			ot->iCpuChange = FALSE;
			ot->iParent->iCpuChange = FALSE;
			}
		}
no_ot:
	NSchedulable* g = (NSchedulable*)iSSList.First();
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

		iTimeSliceExpireCounter++; // update metric
		
		if (t==ot)
			{
			if (ot->iParent!=ot)
				{
				((NThreadGroup*)ot->iParent)->iNThreadList.iQueue[ot->iPriority] = ot->iNext;
				iSSList.iQueue[ot->iParent->iPriority] = ot->iParent->iNext;
				}
			else
				iSSList.iQueue[ot->iPriority] = ot->iNext;
			ot->iTime = ot->iTimeslice;
			NSchedulable* g2 = (NSchedulable*)iSSList.First();
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
		TBool dead = ot->iWaitState.ThreadIsDead();
		if (dead && ot->iLbLink.iNext)
			ot->LbUnlink();
		ot->RelSLock();

		// DFC to signal thread is now dead
		if (dead && ot->iWaitState.iWtC.iKillDfc && __e32_atomic_tau_ord8(&ot->iACount, 1, 0xff, 0)==1)
			{
			ot->RemoveFromEnumerateList();
			ot->iWaitState.iWtC.iKillDfc->DoEnque();
			}
		}
	if (iCCSyncPending)
		{
		iCCSyncPending = 0;
		iReschedIPIs |= 0x80000000u;		// update iCCSyncCpus when kernel is finally unlocked
		}
	__KTRACE_OPT(KSCHED,DEBUGPRINT("Rschd->%T",t));
	__NK_ASSERT_ALWAYS(!t || t->iParent);	// must be a thread not a group
	return t;	// could return NULL
	}

void NSchedulable::LbUnlink()
	{
	if (iLbState & ELbState_PerCpu)
		{
		TSubScheduler* ss = &TheSubSchedulers[iLbState & ELbState_CpuMask];
		ss->iReadyListLock.LockOnly();
		if (iLbState == ss->iLbCounter)
			{
			iLbLink.Deque();
			iLbLink.iNext = 0;
			iLbState = ELbState_Inactive;
			}
		ss->iReadyListLock.UnlockOnly();
		}
	else if ((iLbState & ELbState_CpuMask) == ELbState_Global)
		{
		TScheduler& s = TheScheduler;
		s.iBalanceListLock.LockOnly();
		if (iLbState == s.iLbCounter)
			{
			iLbLink.Deque();
			iLbLink.iNext = 0;
			iLbState = ELbState_Inactive;
			}
		s.iBalanceListLock.UnlockOnly();
		}
	if (iLbState != ELbState_Inactive)
		{
		// load balancer is running so we can't dequeue the thread
		iLbState |= ELbState_ExtraRef;				// indicates extra ref has been taken
		__e32_atomic_tau_ord8(&iACount, 1, 1, 0);	// extra ref will be removed by load balancer
		}
	}

TBool NSchedulable::TakeRef()
	{
	return __e32_atomic_tau_ord8(&iACount, 1, 1, 0);
	}

TBool NSchedulable::DropRef()
	{
	if (__e32_atomic_tau_ord8(&iACount, 1, 0xff, 0)!=1)
		return EFalse;
	TDfc* d = 0;
	AcqSLock();
	if (iParent)
		{
		// it's a thread
		NThreadBase* t = (NThreadBase*)this;
		if (t->iWaitState.ThreadIsDead() && t->iWaitState.iWtC.iKillDfc)
			d = t->iWaitState.iWtC.iKillDfc;
		RelSLock();
		t->RemoveFromEnumerateList();
		}
	else
		{
		NThreadGroup* g = (NThreadGroup*)this;
		d = g->iDestructionDfc;
		RelSLock();
		g->RemoveFromEnumerateList();
		}
	if (d)
		d->DoEnque();
	return ETrue;
	}

void NSchedulable::RemoveFromEnumerateList()
	{
	TScheduler& s = TheScheduler;
	s.iEnumerateLock.LockOnly();
	if (iEnumerateLink.Next())
		{
		iEnumerateLink.Deque();
		iEnumerateLink.SetNext(0);
		}
	s.iEnumerateLock.UnlockOnly();
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
				ss.SSRemoveEntry(&g);
				g.iReady = 0;
				g.iPriority = 0;
				}
			else
				{
				TInt np = l.HighestPriority();
				ss.SSChgEntryP(&g, np);
				}
			}
		}
	else
		{
		TSubScheduler& ss = TheSubSchedulers[iReady & EReadyCpuMask];
		ss.SSRemoveEntry(this);
		}
	iReady = 0;

	SubScheduler().iMadeUnReadyCounter++;
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
				ss->SSChgEntryP(tg, ngp);
			}
		}
	else
		ss->SSChgEntryP(this, newp);
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
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T nSetPri(%d) PBNM[%d,%d,%d,%d]",this,newp,iPriority,iBasePri,iNominalPri,iMutexPri));
	iBasePri = TUint8(newp);
	if (iMutexPri > newp)
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

void NThreadBase::SetNominalPriority(TInt newp)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_IDFC|MASK_NOT_ISR,"NThreadBase::SetNominalPriority");
	AcqSLock();
	__KTRACE_OPT(KNKERN,DEBUGPRINT("%T nSetNPr(%d) PBNM[%d,%d,%d,%d]",this,newp,iPriority,iBasePri,iNominalPri,iMutexPri));
	iNominalPri = TUint8(newp);
	NominalPriorityChanged();
	RelSLock();
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
	ss->SSChgEntryP(g, newp);
out:
	ss->iReadyListLock.UnlockOnly();
	}


/******************************************************************************
 * Pull threads on idle
 ******************************************************************************/

const TInt KMaxTries = 4;

struct SIdlePullThread
	{
	SIdlePullThread();
	void Finish(TBool aDone);

	NSchedulable*	iS;
	TInt			iPri;
	NSchedulable*	iOld[KMaxCpus];
	};

SIdlePullThread::SIdlePullThread()
	{
	iS = 0;
	iPri = 0;
	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		iOld[i] = 0;
	}

void SIdlePullThread::Finish(TBool aComplete)
	{
	if (aComplete && iS)
		{
		iS->AcqSLock();
		iS->SetCpuAffinityT(NKern::CurrentCpu() | KCpuAffinityTransient);
		iS->RelSLock();
		}
	if (iS)
		iS->DropRef();
	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		if (iOld[i])
			iOld[i]->DropRef();
	}

void TSubScheduler::IdlePullSearch(SIdlePullThread& a, TSubScheduler* aDest)
	{
	NSchedulable* orig = a.iS;
	TInt dcpu = aDest->iCpuNum;
	volatile TUint32& flags = *(volatile TUint32*)&aDest->iRescheduleNeededFlag;
	iReadyListLock.LockOnly();
	if (iRdyThreadCount>1)	// if there's only 1 it'll be running so leave it alone
		{
		TUint64 pres = iSSList.iPresent64;
		TInt tries = iRdyThreadCount;
		if (tries > KMaxTries)
			tries = KMaxTries;
		NSchedulable* q = 0;
		NSchedulable* p = 0;
		TInt pri = -1;
		for (; tries>0 && !flags; --tries)
			{
			if (p)
				{
				p = (NSchedulable*)(p->iNext);
				if (p == q)
					pri = -1;
				}
			if (pri<0)
				{
				pri = __e32_find_ms1_64(pres);
				if (pri < 0)
					break;
				pres &= ~(TUint64(1)<<pri);
				q = (NSchedulable*)iSSList.iQueue[pri];
				p = q;
				}
			NThreadBase* t = 0;
			if (p->iParent)
				t = (NThreadBase*)p;
			if (p->iCurrent)
				continue;	// running on other CPU so leave it alone
			if (p->iFreezeCpu)
				continue;	// can't run on this CPU - frozen to current CPU
			if (t && t->iCoreCycling)
				continue;	// currently cycling through cores so leave alone
			if (t && t->iHeldFastMutex && t->iLinkedObjType==NThreadBase::EWaitNone)
				continue;	// can't run on this CPU - fast mutex held
			if (p->iCpuChange)
				continue;	// already being migrated so leave it alone
			if (!CheckCpuAgainstAffinity(dcpu, p->iCpuAffinity))
				continue;	// can't run on this CPU - hard affinity
			if (p->iPreferredCpu & NSchedulable::EReadyCpuSticky)
				continue;	// don't want to move it on idle, only on periodic balance
			if (pri > a.iPri)
				{
				if (p->TakeRef())
					{
					a.iS = p;
					a.iPri = pri;
					break;
					}
				}
			}
		}
	iReadyListLock.UnlockOnly();
	if (orig && orig!=a.iS)
		a.iOld[iCpuNum] = orig;
	}

void NKern::Idle()
	{
	TScheduler& s = TheScheduler;
	TSubScheduler& ss0 = SubScheduler();	// OK since idle thread locked to CPU
	ss0.iCurrentThread->iSavedSP = 0;		// will become nonzero if a reschedule occurs
	TUint32 m0 = ss0.iCpuMask;
	volatile TUint32& flags = *(volatile TUint32*)&ss0.iRescheduleNeededFlag;
	if (s.iThreadAcceptCpus & m0)			// if this CPU is shutting down, don't try to pull threads
		{
		SIdlePullThread ipt;
		NKern::Lock();
		s.iIdleBalanceLock.LockOnly();
		TUint32 active = s.iThreadAcceptCpus;
		TUint32 srchm = active &~ m0;
		if (srchm && srchm!=active)
			{
			TUint32 randomizer = *(volatile TUint32*)&s.iIdleBalanceLock;
			TInt nact = __e32_bit_count_32(srchm);
			while (srchm)
				{
				TUint32 srchm2 = srchm;
				if (nact > 1)
					{
					randomizer = 69069*randomizer+41;
					TUint32 lose = randomizer % TUint32(nact);
					for (; lose; --lose)
						srchm2 = srchm2 & (srchm2-1);
					}
				TInt cpu = __e32_find_ls1_32(srchm2);
				TSubScheduler* ss = &TheSubSchedulers[cpu];
				ss->IdlePullSearch(ipt, &ss0);
				if (flags)
					break;
				srchm &= ~(1u<<cpu);
				--nact;
				}
			}
		s.iIdleBalanceLock.UnlockOnly();
		ipt.Finish(!srchm);
		NKern::Unlock();
		}
	DoIdle();
	}

