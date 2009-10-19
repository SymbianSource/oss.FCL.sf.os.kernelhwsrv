// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkern\arm\ncglob.cpp
// 
//

#include <arm.h>

extern "C" {
SArmInterruptInfo ArmInterruptInfo;
}

#ifdef __CPU_HAS_VFP
NThread* Arm::VfpThread[1];
#endif
#ifdef __CPU_ARM_USE_DOMAINS
TUint32 Arm::DefaultDomainAccess;
#endif

#ifdef __CPU_HAS_COPROCESSOR_ACCESS_REG
TUint32 Arm::DefaultCoprocessorAccess;
#endif

TScheduler TheScheduler;

SBTraceData BTraceData = { {0}, 0, 0 };
