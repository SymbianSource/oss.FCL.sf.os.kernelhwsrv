// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkernsmp\x86\ncglob.cpp
// 
//

#include <x86.h>

//
extern "C" {
TLinAddr X86_IrqHandler;
SCpuIdleHandler CpuIdleHandler;
TUint32 X86_CPUID;
TBool X86_UseGlobalPTEs;
TUint64 DefaultCoprocessorState[64];
}

TUint32 X86::DefaultCR0;

TScheduler TheScheduler;
TSubScheduler TheSubSchedulers[KMaxCpus];
extern "C" {
TSubScheduler* SubSchedulerLookupTable[256];
}

#ifdef __USE_BTRACE_LOCK__
TSpinLock BTraceLock(TSpinLock::EOrderBTrace);
#endif

SBTraceData BTraceData = { {0}, 0, 0 };
