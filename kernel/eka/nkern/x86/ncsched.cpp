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
// e32\nkern\x86\ncsched.cpp
// 
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

#include <x86.h>

//#define __DEBUG_BAD_ADDR

#if defined(KSCHED)
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

extern "C" void DebugMsgFMSignal(int a)
	{
	__NK_ASSERT_DEBUG(TheScheduler.iCurrentThread->iHeldFastMutex==(NFastMutex*)a);
	__KTRACE_OPT(KNKERN,DEBUGPRINT("FMSignal %M",a));
	}

extern "C" void DebugMsgFMWait(int a)
	{
	__NK_ASSERT_DEBUG(!TheScheduler.iCurrentThread->iHeldFastMutex);
	__KTRACE_OPT(KNKERN,DEBUGPRINT("FMWait %M",a));
	}

extern "C" void DebugMsgFMWaitYield(int a)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("FMWait: YieldTo %T",a));
	}

extern "C" void DebugMsgNKFMSignal(int a)
	{
	__NK_ASSERT_DEBUG(TheScheduler.iCurrentThread->iHeldFastMutex==(NFastMutex*)a);
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKFMSignal %M",a));
	}

extern "C" void DebugMsgNKFMWait(int a)
	{
	__NK_ASSERT_DEBUG(!TheScheduler.iCurrentThread->iHeldFastMutex);
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKFMWait %M",a));
	}

extern "C" void DebugMsgNKFMWaitYield(int a)
	{
	__KTRACE_OPT(KNKERN,DEBUGPRINT("NKFMWait: YieldTo %T",a));
	}

#endif

