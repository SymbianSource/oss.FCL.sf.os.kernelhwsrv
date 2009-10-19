// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkern\arm\ncsched.cpp
// 
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

#include <arm.h>

#if defined(_DEBUG)
extern "C" void __DebugMsgWaitForAnyRequest()
	{
	__KTRACE_OPT(KEXEC,DEBUGPRINT("WfAR"));
	}

extern "C" void __DebugMsgResched(int a)
	{
	__KTRACE_OPT(KSCHED,DEBUGPRINT("Reschedule->%T",a));
	}

extern "C" void __DebugMsgInitSelection(int a)
	{
	NThread* pT=(NThread*)a;
	if (pT->iHeldFastMutex)
		{
		__KTRACE_OPT(KSCHED2,DEBUGPRINT("Resched init->%T, Holding %M",pT,pT->iHeldFastMutex));
		}
	else
		{
		__KTRACE_OPT(KSCHED2,DEBUGPRINT("Resched init->%T",pT));
		}
	}

extern "C" void __DebugMsgRR(int a)
	{
	NThread* pT=(NThread*)a;
	__KTRACE_OPT(KSCHED2,DEBUGPRINT("RoundRobin->%T",pT));
	}

extern "C" void __DebugMsgBlockedFM(int a)
	{
	NFastMutex* pM=(NFastMutex*)a;
	__KTRACE_OPT(KSCHED2,DEBUGPRINT("Resched inter->%T, Blocked on %M",pM->iHoldingThread,pM));
	}

extern "C" void __DebugMsgImpSysHeld(int a)
	{
	NThread* pT=(NThread*)a;
	__KTRACE_OPT(KSCHED2,DEBUGPRINT("Resched inter->%T (IMP SYS)",pT));
	}

extern "C" void __DebugMsgFMSignal(int a)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED,"NFastMutex::Signal");			
	__ASSERT_WITH_MESSAGE_DEBUG(TheScheduler.iCurrentThread->iHeldFastMutex==(NFastMutex*)a,"The calling thread doesn't hold the mutex","NFastMutex::Signal");	
	__KTRACE_OPT(KNKERN,DEBUGPRINT("FMSignal %M",a));
	}

extern "C" void __DebugMsgFMWait(int a)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED_ONCE,"NFastMutex::Wait()");
	__ASSERT_WITH_MESSAGE_DEBUG(!TheScheduler.iCurrentThread->iHeldFastMutex,"No fast mutex can be held.","NFastMutex::Wait");	
	__KTRACE_OPT(KNKERN,DEBUGPRINT("FMWait %M",a));
	}

extern "C" void __DebugMsgFMWaitYield(int a)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("FMWait: YieldTo %T",a));
	}

extern "C" void __DebugMsgNKFMSignal(int a)
	{
	__ASSERT_WITH_MESSAGE_DEBUG(TheScheduler.iCurrentThread->iHeldFastMutex==(NFastMutex*)a,"The calling thread doesn't hold the mutex","NKern::FMSignal");	
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKFMSignal %M",a));
	}

extern "C" void __DebugMsgNKFMWait(int a)
	{
	__ASSERT_WITH_MESSAGE_DEBUG(!TheScheduler.iCurrentThread->iHeldFastMutex,"No fast mutex can be held.","NKern::FMWait");	
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKFMWait %M",a));
	}

extern "C" void __DebugMsgNKFMWaitYield(int a)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKFMWait: YieldTo %T",a));
	}

extern "C" void __DebugMsgNKFMFlash(int a)
	{
	__ASSERT_WITH_MESSAGE_DEBUG(TheScheduler.iCurrentThread->iHeldFastMutex==(NFastMutex*)a,"The calling thread doesn't hold the mutex","NKern::FMFlash");	
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKFMFlash %M",a));
	}
#endif

#ifdef __FAST_SEM_MACHINE_CODED__
extern "C" void PanicFastSemaphoreWait()
	{
	FAULT();
	}
#endif
