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
// e32\memmodel\epoc\direct\arm\xmonitor.cpp
// Kernel crash debugger - ARM specific portion
// 
//

#include <kernel/monitor.h>
#include "arm_mem.h"

EXPORT_C void Monitor::CpuInit()
	{
	}

void Monitor::DumpCpuProcessData(DProcess* /*aProcess*/)
	{
	}

void Monitor::DumpCpuChunkData(DChunk* /*aChunk*/)
	{
	}

