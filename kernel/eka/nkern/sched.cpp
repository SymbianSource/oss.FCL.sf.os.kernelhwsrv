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
// e32\nkern\sched.cpp
// 
//

// TDfc member data
#define __INCLUDE_TDFC_DEFINES__
#define __INCLUDE_NTHREADBASE_DEFINES__

#include "nk_priv.h"

void ResumeDelayedThreads(TAny*)
	{
	TScheduler& s=TheScheduler;
	while (!s.iDelayedQ.IsEmpty())
		{
		NThreadBase& t = *(NThreadBase*)s.iDelayedQ.First()->Deque();
		t.i_ThrdAttr &= ~KThreadAttDelayed;
		if (t.iSuspendCount==0)
			t.DoReady();
		else
			t.iNState=NThreadBase::ESuspended;
		}
	}

// Don't need to zero initialise anything here since
// TheScheduler resides in .bss
TScheduler::TScheduler()
	:	TPriListBase(KNumPriorities), iKernCSLocked(1), iDelayDfc(ResumeDelayedThreads, NULL)
	{
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


#ifndef __MSTIM_MACHINE_CODED__
// Enter and leave with interrupts off
void TScheduler::TimesliceTick()
	{
	NThreadBase* pC=iCurrentThread;
	if (pC->iTime>0 && --pC->iTime==0)
		RescheduleNeeded();
	}
#endif

#ifndef __SCHEDULER_MACHINE_CODED__
void TScheduler::Remove(NThreadBase* aThread)
	{
	__NK_ASSERT_DEBUG(!aThread->iHeldFastMutex);	// can't block while holding fast mutex
	iMadeUnReadyCounter++;
	aThread->iTime=aThread->iTimeslice;		// thread has blocked so it gets a fresh timeslice for next time
	TPriListBase::Remove(aThread);
	}
#endif

void TScheduler::RotateReadyList(TInt p)
//
// rotate the ready list for priority p
//
	{
	__NK_ASSERT_DEBUG(p>=0 && p<KNumPriorities);
	SDblQueLink* pQ=iQueue[p];
	if (pQ)
		{
		SDblQueLink* pN=pQ->iNext;
		if (pN!=pQ)
			{
			NThread* pT=(NThread*)pQ;
			pT->iTime=pT->iTimeslice;
			iQueue[p]=pN;
			if (pQ==iCurrentThread)
				RescheduleNeeded();
			}
		}
	}

/**	Converts a time interval in microseconds to thread timeslice ticks

	@param aMicroseconds time interval in microseconds.
	@return Number of thread timeslice ticks.  Non-integral results are rounded up.

 	@pre aMicroseconds should be nonnegative
	@pre any context
 */
EXPORT_C TInt NKern::TimesliceTicks(TUint32 aMicroseconds)
	{
	TUint32 msp = TheTimerQ.iTickPeriod;
	TUint32 ticks = (TUint32(aMicroseconds) + msp - 1) / msp;
	return ticks;
	}


/** Return the total CPU time so far used by the specified thread.

	@return The total CPU time in units of 1/NKern::CpuTimeMeasFreq().
*/
EXPORT_C TUint64 NKern::ThreadCpuTime(NThread* aThread)
	{
#ifdef MONITOR_THREAD_CPU_TIME
	NKern::Lock();
	TUint64 t = aThread->iTotalCpuTime;
	if (aThread == TheScheduler.iCurrentThread)
		t += TUint64(NKern::FastCounter() - aThread->iLastStartTime);
	NKern::Unlock();
	return t;
#else
	return 0;
#endif
	}
