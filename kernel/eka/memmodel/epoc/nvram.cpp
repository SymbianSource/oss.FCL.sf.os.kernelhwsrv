// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\epoc\nvram.cpp
// 
//

#include "plat_priv.h"

_LIT(KLitMachineConfigMutex,"MCConfMutex");
_LIT(KLitRamDriveMutex,"RamDriveMutex");
_LIT(KLitTheRamDriveChunk,"TheRamDriveChunk");

void K::InitNvRam()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("InitNvRam"));
	TInt r=K::MutexCreate(K::MachineConfigMutex, KLitMachineConfigMutex, NULL, EFalse, KMutexOrdMachineConfig);
	if (r!=KErrNone)
		K::Fault(K::EMachineConfigMutexCreateFailed);
	if (K::ColdStart)
		{
		TheSuperPage().iRamDriveSize=0;
		TheMachineConfig().iLogSize=0;
		TheMachineConfig().iLogMaxSize=0;
		}
#ifdef __MEMMODEL_FLEXIBLE__
	TheSuperPage().iRamDriveSize=0;
#endif
	SChunkCreateInfo c;
	TInt ramDriveSize=TheSuperPage().iRamDriveSize;
	c.iGlobal=EFalse;
	c.iAtt=TChunkCreate::ENormal;
	c.iForceFixed=EFalse;
#ifndef __MEMMODEL_FLEXIBLE__
	c.iOperations=SChunkCreateInfo::EAdjust|SChunkCreateInfo::EAdd;
#else
	c.iOperations=SChunkCreateInfo::EAdjust;
#endif
	c.iRunAddress=PP::RamDriveStartAddress;
	c.iPreallocated=ramDriveSize;
	c.iType=ERamDrive;
	c.iMaxSize=PP::RamDriveMaxSize;
	c.iInitialBottom=0;
	c.iInitialTop=0;
	c.iName.Set(KLitTheRamDriveChunk);
	c.iOwner=K::TheKernelProcess;
	TLinAddr runAddr;
	r=K::TheKernelProcess->NewChunk((DChunk*&)PP::TheRamDriveChunk,c,runAddr);
	if (r!=KErrNone)
		K::Fault(K::ERamDriveChunkCreateFailed);
	__KTRACE_OPT(KBOOT,Kern::Printf("Ram Drive size = %08x", ramDriveSize));
	r=TInternalRamDrive::Create();
	if (r!=KErrNone)
		K::Fault(K::ERamDriveInitFailed);

	__KTRACE_OPT(KBOOT,Kern::Printf("K::InitNvRam() completed"));
	}

TInt TInternalRamDrive::Create()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("TInternalRamDrive::Create()"));

	// create the RAM drive mutex
	TInt r=K::MutexCreate((DMutex*&)Mutex, KLitRamDriveMutex, NULL, EFalse, KMutexOrdRamDrive);
	if (r!=KErrNone)
		return r;
	__KTRACE_OPT(KBOOT,Kern::Printf("RAM drive mutex created at %08x",Mutex));
	return KErrNone;
	}

#ifndef __MEMMODEL_FLEXIBLE__
EXPORT_C TLinAddr TInternalRamDrive::Base()
//
// Return the Internal Ram Drive base address
//
	{
	return (TLinAddr)PP::TheRamDriveChunk->Base(&Kern::CurrentProcess());
	}
#endif

EXPORT_C TInt TInternalRamDrive::Size()
//
// Return the Internal Ram Drive size
//
	{
	return TheSuperPage().iRamDriveSize;
	}

EXPORT_C TInt TInternalRamDrive::Adjust(TInt aNewSize)
//
// Adjust the size of the internal ram drive
//
	{
	// If we are shrinking the drive, change the size now in case the
	// machine is reset half way through the chunk adjust
	if (aNewSize<0)
		return KErrArgument;
	if (aNewSize<TheSuperPage().iRamDriveSize)
		{
		TheSuperPage().iRamDriveSize=aNewSize;
		return PP::TheRamDriveChunk->Adjust(aNewSize);
		}

	// If we are growing the drive, change the size after the adjustment is complete
	// If a reset occurs in the middle of the adjust, the ram drive will be
	// restored to its original state before the adjustment.
	else if (aNewSize>TheSuperPage().iRamDriveSize)
		{
		if (aNewSize>PP::RamDriveMaxSize)
			return KErrDiskFull;
		TInt r=PP::TheRamDriveChunk->Adjust(aNewSize);
		if (r==KErrNoMemory)
			return(KErrDiskFull);
		else if(r==KErrNone)
			TheSuperPage().iRamDriveSize=aNewSize;
		return(r);
		}
	return KErrNone;
	}

EXPORT_C void TInternalRamDrive::Wait()
	{
	Kern::MutexWait(*Mutex);
	UNLOCK_USER_MEMORY();
	}

EXPORT_C void TInternalRamDrive::Signal()
	{
	LOCK_USER_MEMORY();
	Kern::MutexSignal(*Mutex);
	}
