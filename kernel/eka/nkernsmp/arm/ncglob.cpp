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
// e32\nkernsmp\arm\ncglob.cpp
// 
//

#include <arm.h>

extern "C" {
SArmInterruptInfo	ArmInterruptInfo;
}

#ifdef __CPU_HAS_VFP
NThread* Arm::VfpThread[KMaxCpus];
#endif
TUint32 Arm::DefaultDomainAccess;
TUint32 Arm::DefaultCoprocessorAccess;

TScheduler		TheScheduler;
TSubScheduler	TheSubSchedulers[KMaxCpus];
extern "C" {
//TSubScheduler*	SubSchedulerLookupTable[256];
TUint32 CrashStateOut;
SFullArmRegSet DefaultRegSet;
}

#ifdef __USE_BTRACE_LOCK__
TSpinLock BTraceLock(TSpinLock::EOrderBTrace);
#endif

SBTraceData BTraceData = { {0}, 0, 0 };
