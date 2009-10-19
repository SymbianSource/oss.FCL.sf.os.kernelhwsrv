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
// e32\memmodel\epoc\multiple\mmonitor.cpp
// Kernel crash debugger - machine independent portion
// 
//

#include <kernel/monitor.h>
#include "memmodel.h"

void Monitor::DumpMemModelProcessData(DProcess* aProcess)
	{
	DMemModelProcess* pP=(DMemModelProcess*)aProcess;
	Printf("OS ASID=%d, LPD=%08x, GPD=%08x\r\n",pP->iOsAsid,pP->iLocalPageDir,pP->iGlobalPageDir);
	Printf("ChunkCount=%d ChunkAlloc=%d\r\n",pP->iChunkCount,pP->iChunkAlloc);
	TInt i;
	for (i=0; i<pP->iChunkCount; i++)
		{
		DMemModelProcess::SChunkInfo& ci=pP->iChunks[i];
		Printf("%d: Chunk %08x, access count %d\r\n",i,ci.iChunk,ci.iAccessCount);
		}
	}

void Monitor::DumpChunkData(DChunk* aChunk)
	{
	DMemModelChunk* pC=(DMemModelChunk*)aChunk;
	Printf("Owning Process %08x OS ASIDS %08x\r\n",pC->iOwningProcess,pC->iOsAsids);
	Printf("Size %x, MaxSize %x, Base %08x\r\n",pC->iSize,pC->iMaxSize,pC->iBase);
	Printf("Attrib %x, StartPos %x\r\n",pC->iAttributes,pC->iStartPos);
	Printf("Type %d\r\n",pC->iChunkType);
	Printf("PTE: %08x, PDE: %08x\r\n", pC->iPtePermissions,pC->iPdePermissions);
	Printf("PageTables=%08x, PageBitMap=%08x, PermPgBitMap=%08x\r\n",pC->iPageTables,pC->iPageBitMap,pC->iPermanentPageBitMap);
	DumpCpuChunkData(pC);
	if (pC->iKernelMirror)
		{
		Printf("iKernelMirror=%08x\r\n", pC->iKernelMirror);
		DumpObjectData((DMonObject*)pC->iKernelMirror, EChunk);
		}
	}

void Monitor::MMProcessInfoCommand()
	{
	TScheduler* pS=TScheduler::Ptr();
#ifdef __SMP__
	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		{
		DMonObject* pA = (DMonObject*)pS->iSub[i]->iAddressSpace;
		Printf("CPU%d: TheCurrentAddressSpace=%08x\r\n",i,pA);
		}
#else
	DMonObject* pA=(DMonObject*)pS->iAddressSpace;
	Printf("TheCurrentAddressSpace=%08x\r\n",pA);
#endif
	}
	
void Monitor::MDisplayCodeSeg(DCodeSeg* aSeg)
	{
	DMemModelCodeSeg* seg = (DMemModelCodeSeg*) aSeg;
	DMemModelCodeSegMemory* mem = seg->Memory();
	Printf("   iPageCount: %x\r\n",mem ? mem->iPageCount : 0);
	Printf("   iPages: %08x (%08x)\r\n",mem ? mem->iPages : 0, mem ? *mem->iPages : 0);
	Printf("   iCodeAllocBase: %08x\r\n",seg->iCodeAllocBase);
	Printf("   iDataAllocBase: %08x\r\n",seg->iDataAllocBase);
	Printf("   iKernelData: %08x\r\n",seg->iKernelData);
	Printf("   iOsAsids: %08x\r\n",mem ? mem->iOsAsids : 0);
	Printf("   iCreator: %08x\r\n",mem ? mem->iCreator : 0);
	}

extern TInt MapProcess(DMemModelProcess* aProcess, TBool aForce);

EXPORT_C TUint Monitor::MapAndLocateUserStack(DThread* aThread)
	{
	DMemModelProcess* pP=(DMemModelProcess*)aThread->iOwningProcess;
	TInt r = SwitchAddressSpace(pP, EFalse);
	if (r!=KErrNone)
		return 0;
	return aThread->iUserStackRunAddress;
	}

EXPORT_C TInt Monitor::SwitchAddressSpace(DProcess* aProcess, TBool aForce)
	{
	return MapProcess((DMemModelProcess*)aProcess, aForce);
	}
