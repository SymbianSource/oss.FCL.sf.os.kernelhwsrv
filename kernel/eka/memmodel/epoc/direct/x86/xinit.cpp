// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\epoc\direct\x86\xinit.cpp
// 
//

#include <x86_mem.h>
#include <e32uid.h>
#include <kernel/cache.h>

void MM::Init1()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("MM::Init1()"));

	MM::RamBlockSize=0x1000;
	MM::RamBlockShift=12;

	PP::MaxUserThreadStack=0x14000;			// 80K - STDLIB asks for 64K for PosixServer!!!!
	PP::UserThreadStackGuard=0x2000;		// 8K
	PP::MaxStackSpacePerProcess=0x200000;	// 2Mb
	K::SupervisorThreadStackSize=0x1000;	// 4K
	PP::SupervisorThreadStackGuard=0x1000;	// 4K

	K::MachineConfig=(TMachineConfig*)(((TUint8*)SuperPageAddress)+0x800);	// HACK!!

	K::MaxMemCopyInOneGo = KDefaultMaxMemCopyInOneGo;

	PP::RamDriveStartAddress = TheSuperPage().iKernelLimit;
	PP::RamDriveMaxSize = TheRomHeader().iUserDataAddress - PP::RamDriveStartAddress;
	__KTRACE_OPT(KBOOT,Kern::Printf("RamDriveBase %08x RamDriveMaxSize %08x",PP::RamDriveStartAddress,PP::RamDriveMaxSize));
	__KTRACE_OPT(KBOOT,Kern::Printf("K::MaxMemCopyInOneGo=0x%x",K::MaxMemCopyInOneGo));

	K::MemModelAttributes=EMemModelTypeDirect|EMemModelAttrNonExProt|EMemModelAttrKernProt|EMemModelAttrWriteProt;
	}

#ifdef __CPU_HAS_CACHE
// Set up virtual addresses used for cache flushing if this is
// done by data read or line allocate
void M::SetupCacheFlushPtr(TInt aCache, SCacheInfo& aInfo)
	{
	}
#endif

#ifdef __SMP__
void M::GetAPBootInfo(TInt /*aCpu*/, volatile SAPBootInfo* /*aInfo*/)
	{
	}

void M::Init2AP()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("M::Init2AP()"));
	}
#endif
