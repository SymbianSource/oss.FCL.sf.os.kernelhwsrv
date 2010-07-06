// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkernsmp\arm\ncsched.cpp
// 
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

#include <arm.h>
#include <arm_tmr.h>

extern "C" void NewThreadTrace(NThread* a)
	{
	__ACQUIRE_BTRACE_LOCK();
	BTraceData.iHandler(BTRACE_HEADER_C(4,BTrace::ECpuUsage,BTrace::ENewThreadContext),0,(TUint32)a,0,0,0,0,0);
	__RELEASE_BTRACE_LOCK();
	}

extern "C" void __DebugMsgSendReschedIPI(int a)
	{
	__KTRACE_OPT(KSCHED2,DEBUGPRINT("@%d",a));
	}

extern "C" void __DebugMsgSendReschedIPIs(int a)
	{
	__KTRACE_OPT(KSCHED2,DEBUGPRINT("@%02x",a));
	}

extern "C" void __DebugMsgSendReschedIPIAndWait(int a)
	{
	__KTRACE_OPT(KSCHED2,DEBUGPRINT("@@%d",a));
	}

#ifdef __FAST_SEM_MACHINE_CODED__
extern "C" void PanicFastSemaphoreWait()
	{
	FAULT();
	}
#endif

#ifdef BTRACE_CPU_USAGE
extern "C" void btrace_irq_entry(TInt aVector)
	{
	BTrace4(BTrace::ECpuUsage, BTrace::EIrqStart, aVector);
	}

extern "C" void btrace_fiq_entry()
	{
	BTrace0(BTrace::ECpuUsage, BTrace::EFiqStart);
	}

extern "C" void btrace_irq_exit()
	{
	BTrace0(BTrace::ECpuUsage, BTrace::EIrqEnd);
	}

extern "C" void btrace_fiq_exit()
	{
	BTrace0(BTrace::ECpuUsage, BTrace::EFiqEnd);
	}
#endif

extern "C" void __DebugMsgIrq(TUint a)
	{
	if ((a & 0x3ffu) >= 0x20u)
		return;
	__KTRACE_OPT(KSCHED2,DEBUGPRINT("!%x",a));
	}


#ifdef _DEBUG
extern "C" void __DebugMsgFSWait(NFastSemaphore* a)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NO_FAST_MUTEX,"NFastSemaphore::Wait");
	NThreadBase* pC = NCurrentThreadL();
	__ASSERT_WITH_MESSAGE_ALWAYS(pC==a->iOwningThread,"The calling thread must own the semaphore","NFastSemaphore::Wait");
	}

extern "C" void __DebugMsgNKFSWait(NFastSemaphore* a)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NFSW %m",a));
	NThreadBase* pC = NCurrentThread();
	__ASSERT_WITH_MESSAGE_ALWAYS(pC==a->iOwningThread,"The calling thread must own the semaphore","NKern::FSWait");
	}

extern "C" void __DebugMsgWFAR()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"NKern::WaitForAnyRequest");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("WfAR"));
	}

extern "C" void __DebugMsgFSSignal(NFastSemaphore* /*a*/)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR,"NFastSemaphore::Signal");
	}

extern "C" void __DebugMsgFSSignalN(NFastSemaphore* /*a*/, TInt aCount)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED|MASK_NOT_ISR,"NFastSemaphore::SignalN");			
	__NK_ASSERT_DEBUG(aCount>=0);
	}

extern "C" void __DebugMsgNKThreadRequestSignalN(NThread* /*aThread*/, TInt aCount)
	{
	__ASSERT_WITH_MESSAGE_DEBUG(aCount>=0, "aCount>=0", "NKern::ThreadRequestSignal");
	}

extern "C" void __DebugMsgNKThreadRequestSignal(NThread* /*aThread*/)
	{
	}

extern "C" void __DebugMsgNKFSSignal(NFastSemaphore* a)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR,"NKern::FSSignal(NFastSemaphore*)");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NFSS %m",a));
	}

extern "C" void __DebugMsgNKFSSignalN(NFastSemaphore* a, TInt aCount)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR,"NKern::FSSignalN(NFastSemaphore*, TInt)");
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NFSSN %m %d",a,aCount));
	__NK_ASSERT_DEBUG(aCount>=0);
	}

extern "C" void __DebugMsgFMSignal(NFastMutex* /*a*/)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED,"NFastMutex::Signal");
	}

extern "C" void __DebugMsgNKFMWait(NFastMutex* a)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NFMW %M", a));
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"NKern::FMWait");
	}

extern "C" void __DebugMsgNKFMSignal(NFastMutex* a)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NFMS %M", a));
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED | MASK_INTERRUPTS_ENABLED | MASK_NOT_ISR | MASK_NOT_IDFC, "NKern::FMSignal");
	__ASSERT_WITH_MESSAGE_DEBUG(a->HeldByCurrentThread(),"The calling thread holds the mutex","NKern::FMSignal");
	}

extern "C" void __DebugMsgNKFMFlash(NFastMutex* a)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED | MASK_INTERRUPTS_ENABLED | MASK_NOT_ISR | MASK_NOT_IDFC, "NKern::FMFlash");
	__ASSERT_WITH_MESSAGE_DEBUG(a->HeldByCurrentThread(),"The calling thread holds the mutex","NKern::FMFlash");
	}

#endif

#if !defined(__UTT_MACHINE_CODED__)
void TSubScheduler::UpdateThreadTimes(NThreadBase* aOld, NThreadBase* aNew)
	{
	/* If necessary update local timer frequency (DVFS) */
	SRatioInv* pNCF = iSSX.iNewCpuFreqRI;
	if (pNCF)
		{
		iSSX.iCpuFreqRI = *pNCF;
		__e32_atomic_store_rel_ptr(&iSSX.iNewCpuFreqRI, 0);
		}
	SRatioInv* pNTF = iSSX.iNewTimerFreqRI;
	if (pNTF)
		{
		iSSX.iTimerFreqRI = *pNTF;
		__e32_atomic_store_rel_ptr(&iSSX.iNewTimerFreqRI, 0);
		}
	if (!aOld)
		aOld = iInitialThread;
	if (!aNew)
		aNew = iInitialThread;
	if (aNew!=aOld || aNew->iTime<=0 || pNTF)
		{
		TUint32 tmrval = 0x7fffffffu;
		if (aNew->iTime > 0)
			{
			tmrval = aNew->iTime;	// this will have been computed based on the old timer frequency
			iSSX.iTimerFreqRI.iR.Mult(tmrval);
			}
//		iSSX.iLastTimerSet = tmrval;
		iSSX.iLocalTimerAddr->iTimerCount = tmrval;
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
#endif

