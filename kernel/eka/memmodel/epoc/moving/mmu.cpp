// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\epoc\moving\mmu.cpp
// 
//

#include "memmodel.h"

/*******************************************************************************
 * "Independent" MMU code
 *******************************************************************************/

void Mmu::Panic(TPanic aPanic)
	{
	Kern::Fault("MMU",aPanic);
	}

void Mmu::Init1()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("Mmu::Init1"));
	__ASSERT_ALWAYS(TheRomHeader().iUserDataAddress==iDllDataBase+iMaxDllDataSize,Panic(ERomUserDataAddressInvalid));
	__ASSERT_ALWAYS((TheRomHeader().iTotalUserDataSize&iPageMask)==0,Panic(ERomUserDataSizeInvalid));
	__ASSERT_ALWAYS(::RomHeaderAddress==iRomLinearBase,Panic(ERomLinearAddressInvalid));
	MmuBase::Init1();
	}

void Mmu::DoInit2()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("Mmu::DoInit2"));
	MM::DllDataAllocator=TBitMapAllocator::New(iMaxDllDataSize>>iPageShift, ETrue);
	__ASSERT_ALWAYS(MM::DllDataAllocator,Panic(EDllDataAllocatorCreateFailed));
	TInt rom_dll_pages=TheRomHeader().iTotalUserDataSize>>iPageShift;
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("DllDataAllocator @ %08x, %d ROM DLL Data Pages", MM::DllDataAllocator, rom_dll_pages));
	if (rom_dll_pages)
		MM::DllDataAllocator->Alloc(0, rom_dll_pages);	// low bit numbers represent high addresses
	}

//#ifndef __MMU_MACHINE_CODED__
TInt Mmu::PageTableId(TLinAddr aAddr)
	{
	NKern::LockSystem();
	TInt id = GetPageTableId(aAddr);
	NKern::UnlockSystem();
	return id;
	}
//#endif

void Mmu::AssignPageTable(TInt aId, TInt aUsage, TAny* aObject, TLinAddr aAddr, TPde aPdePerm)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::AssignPageTable id=%d, u=%08x, obj=%08x, addr=%08x, perm=%08x",
					aId, aUsage, aObject, aAddr, aPdePerm));
	NKern::LockSystem();
	SPageTableInfo& pti=PtInfo(aId);
	switch (aUsage)
		{
//		case SPageTableInfo::EChunk:
//			{
//			DMemModelChunk* pC=(DMemModelChunk*)aObject;
//			TUint32 ccp=K::CompressKHeapPtr(pC);
//			TUint32 offset=(aAddr-TLinAddr(pC->iBase))>>iChunkShift;
//			pti.SetChunk(ccp,offset);
//			break;
//			}
//		case SPageTableInfo::EHwChunk:
//			break;
		case SPageTableInfo::EGlobal:
			pti.SetGlobal(aAddr>>iChunkShift);
			break;
		default:
			Panic(EAssignPageTableInvalidUsage);
		}
	DoAssignPageTable(aId, aAddr, aPdePerm);
	NKern::UnlockSystem();
	}

TInt Mmu::UnassignPageTable(TLinAddr aAddr)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::UnassignPageTable addr=%08x", aAddr));
	NKern::LockSystem();
	TInt id=GetPageTableId(aAddr);
	if (id>=0)
		DoUnassignPageTable(aAddr);
	NKern::UnlockSystem();
	return id;
	}

