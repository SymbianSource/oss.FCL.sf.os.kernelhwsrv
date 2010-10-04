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
// Kernel crash debugger - X86 specific portion
// 
//
#define INCLUDED_FROM_ASM	// Make DMemModelProcess::iOsAsid public, safe to 
							// do here as the system has crashed and the inline
							// DMemModelProcess::OsAsid() requires too much kernel knowlodge.

#include <kernel/monitor.h>
#include <x86_mem.h>

void SetPageDir(DMemModelProcess* aProcess);

void Monitor::CpuInit()
	{
	}

void Monitor::DumpCpuProcessData(DProcess* aProcess)
	{
	}

void Monitor::DumpCpuChunkData(DChunk* aChunk)
	{
	}

// Mask to ignore accessed and dirty bits
const TUint32 KPdeEqualMask=~(KPdePteAccessed|KPdePteDirty);

TBool PdesEqual(const TPde* aLeft, const TPde* aRight, TLinAddr aBase, TLinAddr aEnd)
	{
	for (TUint addr = aBase ; addr != aEnd ; addr += (1 << KChunkShift))
		{
		TUint i = addr >> KChunkShift;
		if ((aLeft[i] & KPdeEqualMask) != (aRight[i] & KPdeEqualMask))
			return EFalse;
		}
	return ETrue;
	}

TInt MapProcess(DMemModelProcess* aProcess, TBool aForce)
	{
	// Check top half of page dir is same as for the kernel
	DMonObject* o = (DMonObject*)aProcess;
	if (o->Type()!=EProcess)
		return KErrArgument;

	const TPde* kpd=(const TPde*)KPageDirectoryBase;
	const TPde* ppd=(const TPde*)(KPageDirectoryBase+(aProcess->iOsAsid<<KPageTableShift));

	// Check kernel mappings are the same except for IPC alias region
	if (!PdesEqual(kpd, ppd, KGlobalMemoryBase, KIPCAlias) || !PdesEqual(kpd, ppd, KIPCAlias+KIPCAliasAreaSize, 0x00000000))
		{
		if (!aForce)
			return KErrCorrupt;
		wordmove((TAny*)(ppd+(KGlobalMemoryBase>>KChunkShift)), (TAny*)(kpd+(KGlobalMemoryBase>>KChunkShift)), ((0-KGlobalMemoryBase)>>KChunkShift)*sizeof(TPde));
		}
	SetPageDir(aProcess);
	return KErrNone;
	}
