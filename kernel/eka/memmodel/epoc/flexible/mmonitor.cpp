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
// Kernel crash debugger - machine independent portion
// 
//
#define INCLUDED_FROM_ASM	// Make DMemModelProcess::iOsAsid public, safe to 
							// do here as the system has crashed and the inline
							// DMemModelProcess::OsAsid() requires too much kernel knowlodge.

#include <kernel/monitor.h>
#include <memmodel.h>

void Monitor::DumpMemModelProcessData(DProcess* aProcess)
	{
	DMemModelProcess* pP=(DMemModelProcess*)aProcess;
	// Read the os asid without opening a reference as after the crash this 
	// is the only running thread.
	Printf("OS ASID=%d, LPD=%08x\r\n",pP->iOsAsid,pP->iPageDir);
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
	Printf("Owning Process %08x OS ASIDS %08x\r\n",pC->iOwningProcess);
	Printf("Size %x, MaxSize %x\r\n",pC->iSize,pC->iMaxSize);
	Printf("Attrib %x, StartPos %x\r\n",pC->iAttributes,pC->iStartPos);
	Printf("Type %d\r\n",pC->iChunkType);
	Printf("PageBitMap=%08x\r\n",pC->iPageBitMap);
	DumpCpuChunkData(pC);
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
	Printf("   iKernelData: %08x\r\n",seg->iKernelData);
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
