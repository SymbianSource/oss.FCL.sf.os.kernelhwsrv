// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\arm\cmonitor.cpp
// Kernel crash debugger - ARM specific portion
// 
//

#define __INCLUDE_REG_OFFSETS__  // for SP_R13U in nk_plat.h

#include <kernel/monitor.h>
#include "arm_mem.h"
#include "nk_plat.h"

GLDEF_D TUint32 MonitorStack[1024];

EXPORT_C void Monitor::DumpCpuThreadData(DThread* aThread)
	{
	}

