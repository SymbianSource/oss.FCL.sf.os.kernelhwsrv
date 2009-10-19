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
// e32\memmodel\epoc\direct\mmonitor.cpp
// Kernel crash debugger - machine independent portion
// 
//

#include <kernel/monitor.h>
#include <memmodel.h>

void Monitor::DumpMemModelProcessData(DProcess* aProcess)
	{
	}

void Monitor::DumpChunkData(DChunk* aChunk)
	{
	DMemModelChunk* pC=(DMemModelChunk*)aChunk;
	Printf("Owning Process %08x\r\n",pC->iOwningProcess);
	Printf("Size %x, MaxSize %x, Base %08x\r\n",pC->iSize,pC->iMaxSize,pC->iBase);
	Printf("Attrib %x, StartPos %x\r\n",pC->iAttributes,pC->iStartPos);
	Printf("Type %d, Region Base %08x, RegionSize %08x\r\n",pC->iChunkType,pC->iRegionBase,pC->iRegionSize);
	}

void Monitor::MMProcessInfoCommand()
	{
	}

EXPORT_C TUint Monitor::MapAndLocateUserStack(DThread* aThread)
	{
 	return (TUint)aThread->iUserStackRunAddress;
	}
	
void Monitor::MDisplayCodeSeg(DCodeSeg* aSeg)
	{
	DMemModelCodeSeg* seg = (DMemModelCodeSeg*) aSeg;
	Printf("   iDataAlloc: %08x\r\n",seg->iDataAlloc);
	Printf("   iKernelData: %08x\r\n",seg->iKernelData);
	}

EXPORT_C TInt Monitor::SwitchAddressSpace(DProcess*, TBool)
	{
	return KErrNone;
	}
