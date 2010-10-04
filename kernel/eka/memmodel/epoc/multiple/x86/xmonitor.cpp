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
// e32\memmodel\epoc\multiple\x86\xmonitor.cpp
// Kernel crash debugger - ARM specific portion
// 
//

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


// From xmmu.cpp:
const TUint32 KPdePteAccessed=0x20;
const TUint32 KPdePteDirty=0x40;

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
	TUint a = (TUint)aProcess;
	DMonObject* o = (DMonObject*)aProcess;
	if (a<KPrimaryIOBase || a>=KKernelSectionEnd || (a&3))
		return KErrArgument;
	if (o->Type()!=EProcess)
		return KErrArgument;

	const TPde* kpd=(const TPde*)KPageDirectoryBase;
	const TPde* ppd=(const TPde*)(KPageDirectoryBase+(aProcess->iOsAsid<<KPageTableShift));
	if (!PdesEqual(kpd, ppd, KRomLinearBase, KUserGlobalDataEnd)		||	// ROM + user global
		!PdesEqual(kpd, ppd, KRamDriveEndAddress, KIPCAlias)			||	// kernel mappings other than IPC aliases
		!PdesEqual(kpd, ppd, KIPCAlias+KIPCAliasAreaSize, 0x00000000u)		// kernel mappings other than IPC aliases
		)			// kernel mappings
		{
		if (!aForce)
			return KErrCorrupt;
		wordmove((TAny*)(ppd+(KRomLinearBase>>KChunkShift)), (TAny*)(kpd+(KRomLinearBase>>KChunkShift)), ((KUserGlobalDataEnd-KRomLinearBase)>>KChunkShift)*sizeof(TPde));
		wordmove((TAny*)(ppd+(KRamDriveEndAddress>>KChunkShift)), (TAny*)(kpd+(KRamDriveEndAddress>>KChunkShift)), ((0-KRamDriveEndAddress)>>KChunkShift)*sizeof(TPde));
		}
	SetPageDir(aProcess);
	return KErrNone;
	}
