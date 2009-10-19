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
// e32\memmodel\epoc\moving\arm\xmonitor.cpp
// Kernel crash debugger - ARM specific portion
// 
//

#include <kernel/monitor.h>
#include "arm_mem.h"

extern void AccessAllAreas();

EXPORT_C void Monitor::CpuInit()
	{
	AccessAllAreas();
	}

void Monitor::DumpCpuProcessData(DProcess* aProcess)
	{
	DArmPlatProcess* pP=(DArmPlatProcess*)aProcess;
	Printf("Domain %d, DACR %08x\r\n",pP->iDomain,pP->iDacr);
	}

void Monitor::DumpCpuChunkData(DChunk* aChunk)
	{
	DArmPlatChunk* pC=(DArmPlatChunk*)aChunk;
	Printf("Domain %d\r\n",pC->iDomain);
	}

