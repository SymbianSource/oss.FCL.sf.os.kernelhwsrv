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
// e32\memmodel\epoc\moving\mmonitor.cpp
// Kernel crash debugger - machine independent portion
// 
//

#include <kernel/monitor.h>
#include "memmodel.h"

void Monitor::DumpMemModelProcessData(DProcess* aProcess)
	{
	DMemModelProcess* pP=(DMemModelProcess*)aProcess;
	Printf("NumChunks=%d\r\n",pP->iNumChunks);
	TInt i;
	for (i=0; i<pP->iNumChunks; i++)
		{
		DMemModelProcess::SChunkInfo& ci=pP->iChunks[i];
		Printf("%d: Chunk %08x, run %08x, access count %d\r\n",i,ci.iChunk,ci.iDataSectionBase,ci.iAccessCount);
		}
	}

void Monitor::DumpChunkData(DChunk* aChunk)
	{
	DMemModelChunk* pC=(DMemModelChunk*)aChunk;
	Printf("Owning Process %08x\r\n",pC->iOwningProcess);
	Printf("Size %x, MaxSize %x, Base %08x\r\n",pC->iSize,pC->iMaxSize,pC->iBase);
	Printf("Attrib %x, StartPos %x\r\n",pC->iAttributes,pC->iStartPos);
	Printf("Type %d, State %d, Home Base %08x\r\n",pC->iChunkType,pC->iChunkState,pC->iHomeBase);
	Printf("Home Region Offset %08x\r\n",pC->iHomeRegionOffset);
	Printf("Home Region Base %08x\r\n",pC->iHomeRegionBase);
	Printf("Home Region Size %08x\r\n",pC->iHomeRegionSize);
	Printf("PTE: %08x, PDE: %08x %08x %08x\r\n", pC->iPtePermissions,
					pC->iPdePermissions[0],pC->iPdePermissions[1],pC->iPdePermissions[2]);
	Printf("NumPdes=%d, iPdes=%08x, iHomePdes=%08x\r\n",pC->iNumPdes,pC->iPdes,pC->iHomePdes);
	Printf("PdeBitMap=%08x, PageBitMap=%08x\r\n",
							pC->iPdeBitMap, pC->iPageBitMap);
	DumpCpuChunkData(pC);
	}

void Monitor::MMProcessInfoCommand()
	{
	TScheduler* pS=TScheduler::Ptr();
	DMonObject* pA=(DMonObject*)pS->iAddressSpace;
	DMonObject* pV=(DMonObject*)pS->iExtras[0];
	DMonObject* pD=(DMonObject*)pS->iExtras[1];
	DMonObject* pC=(DMonObject*)pS->iExtras[2];
	Printf("TheCurrentAddressSpace=%08x\r\n",pA);
	Printf("TheCurrentVMProcess=%08x\r\n",pV);
	Monitor::DumpObjectData(pV,EProcess);
	Printf("TheCurrentDataSectionProcess=%08x\r\n",pD);
	Printf("TheCompleteDataSectionProcess=%08x\r\n",pC);
	Monitor::DumpObjectData(pC,EProcess);
	}

void Monitor::MDisplayCodeSeg(DCodeSeg* aSeg)
	{
	DMemModelCodeSeg* seg = (DMemModelCodeSeg*) aSeg;
	Printf("   iCodeAllocBase: %08x\r\n",seg->iCodeAllocBase);
	Printf("   iDataAllocBase: %08x\r\n",seg->iDataAllocBase);
	Printf("   iKernelData: %08x\r\n",seg->iKernelData);
	}
	
EXPORT_C TUint Monitor::MapAndLocateUserStack(DThread* aThread)	
	{
 	TLinAddr stackBase=aThread->iUserStackRunAddress;
 	TLinAddr stackTop=stackBase+aThread->iUserStackSize;
	DMemModelProcess* pP=(DMemModelProcess*)aThread->iOwningProcess;
	if (!pP || ((DMonObject*)pP)->Type()!=EProcess)
		return 0;
	for(TInt i=0 ; i<pP->iNumChunks ; ++i)
		{
		DMemModelProcess::SChunkInfo& info=pP->iChunks[i];
		DMemModelChunk* pC=info.iChunk;
		if (!pC || ((DMonObject*)pC)->Type()!=EChunk)
			continue;
		if (stackBase >= info.iDataSectionBase  &&
			stackTop <= info.iDataSectionBase+pC->MaxSize())
			return (TUint)pC->iBase+(stackBase-info.iDataSectionBase);
		}
	return 0;
	}

EXPORT_C TInt Monitor::SwitchAddressSpace(DProcess*, TBool)
	{
	return KErrNone;
	}
