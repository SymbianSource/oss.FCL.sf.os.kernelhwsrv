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
// e32\nkernsmp\x86\ncsched.cpp
// 
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

#include <x86.h>
#include <apic.h>


// Called by a thread which has been forced to exit
// Kernel locked on entry
extern "C" void __fastcall do_forced_exit(NThreadBase* aT)
	{
	__NK_ASSERT_ALWAYS(aT->iFastMutexDefer != 1);
	aT->iFastMutexDefer = 0;
	aT->Exit();
	}

extern "C" NThreadBase* __fastcall select_next_thread(TSubScheduler* aS)
	{
	return aS->SelectNextThread();
	}

extern "C" void __fastcall queue_dfcs(TSubScheduler* aS)
	{
	aS->QueueDfcs();
	}

extern "C" void NewThreadTrace(NThread* a)
	{
	__ACQUIRE_BTRACE_LOCK();
	BTraceData.iHandler(BTRACE_HEADER_C(4,BTrace::ECpuUsage,BTrace::ENewThreadContext),0,(TUint32)a,0,0,0,0,0);
	__RELEASE_BTRACE_LOCK();
	}

extern "C" void __fastcall send_ipi(TUint32);
extern "C" void __fastcall do_send_resched_ipis(TUint32);

extern "C" void send_resched_ipi(TInt aCpu)
	{
	TSubScheduler& ss = TheSubSchedulers[aCpu];
	__KTRACE_OPT(KSCHED2,DEBUGPRINT("@%d",aCpu));
	send_ipi(ss.iSSX.iAPICID);
	}

#ifndef __USE_LOGICAL_DEST_MODE__
extern "C" void __fastcall do_send_resched_ipis(TUint32 aMask)
	{
	TInt i=0;
	for (; aMask; aMask>>=1, ++i)
		if (aMask&1)
			send_resched_ipi(i);
	}
#endif

extern "C" void send_resched_ipis(TUint32 aMask)
	{
	__KTRACE_OPT(KSCHED2,DEBUGPRINT("@%02x",aMask));
	TScheduler& s = TheScheduler;
	if (aMask &~ s.iThreadAcceptCpus)
		aMask = s.ReschedInactiveCpus(aMask);
	if (aMask)
		do_send_resched_ipis(aMask);
	}

extern "C" void send_resched_ipi_and_wait(TInt aCpu)
	{
	TSubScheduler& ss = TheSubSchedulers[aCpu];
	__KTRACE_OPT(KSCHED2,DEBUGPRINT("@@%d",aCpu));
	volatile TUint32& irqc = ss.iSSX.iIrqCount;
	volatile TInt& irqn = ss.iSSX.iIrqNestCount;
	TUint32 irqc0 = irqc;
	mb();
	send_ipi(ss.iSSX.iAPICID);
	mb();
	while (!ss.iRescheduleNeededFlag || (irqn<0 && irqc==irqc0))
		{
		__chill();
		}
	mb();	// guaranteed to observe final thread state after this
	}

void TSubScheduler::SaveTimesliceTimer(NThreadBase* aT)
	{
	if (aT->iTime>0 && !aT->i_NThread_Initial)
		{
		TUint32 x = read_apic_reg(CURRCNT);
		iSSX.iTimerFreqRI.iI.Mult(x);
		aT->iTime = (TInt)x;
		}
	write_apic_reg(INITCNT, 0);
	}


/*	Update aOld's execution time and set up the timer for aNew
	Update this CPU's timestamp value

	if (!aOld) aOld=iInitialThread
	if (!aNew) aNew=iInitialThread
	if new thread has a timeslice, start the timeslice timer
	update the last reschedule time
	update the run time for the old thread
	update the reschedule count for the new thread and the current CPU
 */
void TSubScheduler::UpdateThreadTimes(NThreadBase* aOld, NThreadBase* aNew)
	{
	if (!aOld)
		aOld = iInitialThread;
	if (!aNew)
		aNew = iInitialThread;
	if (aNew->iTime>0)
		{
		TUint32 x = (TUint32)aNew->iTime;
		iSSX.iTimerFreqRI.iR.Mult(x);
		write_apic_reg(INITCNT, x);
		}
	if (aNew!=aOld)
		{
		TUint64 now = NKern::Timestamp();
		TUint64 delta = now - iLastTimestamp.i64;
		iLastTimestamp.i64 = now;
		aOld->iLastRunTime.i64 = now;
		aOld->iTotalCpuTime.i64 += delta;
		++iReschedCount.i64;
		++aNew->iRunCount.i64;
		if (!aOld->iActiveState)
			aOld->iTotalActiveTime.i64 += (now - aOld->iLastActivationTime.i64);
		NSchedulable* parent = aOld->iParent;
		if (parent != aOld)
			{
			parent->iLastRunTime.i64 = now;
			if (!parent->iActiveState)
				parent->iTotalActiveTime.i64 += (now - parent->iLastActivationTime.i64);
			if (parent != aNew->iParent)
				parent->iTotalCpuTime.i64 += (now - parent->iLastStartTime.i64);
			}
		NSchedulable* np = aNew->iParent;
		if (np!=aNew && np!=parent)
			np->iLastStartTime.i64 = now;
		}
	}


