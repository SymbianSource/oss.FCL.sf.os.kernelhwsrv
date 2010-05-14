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
// e32\memmodel\epoc\direct\minit.cpp
// 
//

#include <memmodel.h>
#include "cache_maintenance.h"

_LIT(KLitRamAlloc,"RamAlloc");


void M::Init1()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("M::Init1"));
	MM::UserDataSectionBase=TheRomHeader().iUserDataAddress;
	MM::UserRomDataSectionEnd=TheRomHeader().iUserDataAddress+TheRomHeader().iTotalUserDataSize;
	MM::UserDataSectionEnd = TheSuperPage().iRamBase + TheSuperPage().iTotalRamSize;

	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("MM::UserDataSectionBase=%08x",MM::UserDataSectionBase));
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("MM::UserRomDataSectionEnd=%08x",MM::UserRomDataSectionEnd));
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("MM::UserDataSectionEnd=%08x",MM::UserDataSectionEnd));

	// Memory model dependent CPU stuff
	MM::Init1();

	// Set up cache info
	CacheMaintenance::Init1();
	}

void M::Init2()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("M::Init2"));
	TInt userRam=MM::UserDataSectionEnd-MM::UserDataSectionBase;
	TInt blocks=userRam>>MM::RamBlockShift;
	MM::RamAllocator=TBitMapAllocator::New(blocks,ETrue);
	if (!MM::RamAllocator)
		MM::Panic(MM::ERamAllocCreateFailed);
	TInt used_blocks=(MM::UserRomDataSectionEnd-MM::UserDataSectionBase)>>MM::RamBlockShift;
	if (used_blocks)
		MM::RamAllocator->Alloc(0,used_blocks);
	__KTRACE_OPT(KBOOT,Kern::Printf("%d blocks, %d used",blocks,used_blocks));
	TInt r=K::MutexCreate((DMutex*&)MM::RamAllocatorMutex, KLitRamAlloc, NULL, EFalse, KMutexOrdRamAlloc);
	if (r!=KErrNone)
		MM::Panic(MM::ERamAllocMutexCreateFailed);
	const SRamBank* banks = (const SRamBank*)TheSuperPage().iRamBootData;
	TInt nBanks = 0;
	TInt maxBankBlocks = 0;
	TInt bnum = 0;
	const SRamBank* pB = banks;
	for (; pB->iSize; ++nBanks, ++pB)
		{
		TInt nblocks = pB->iSize >> MM::RamBlockShift;
		TInt abnum = bnum &~ 31;
		TInt a_end = (bnum + nblocks + 31)&~31;
		TInt a_count = a_end - abnum;
		if (a_count > maxBankBlocks)
			maxBankBlocks = a_count;
		bnum += (pB->iSize >> MM::RamBlockShift);
		}
	__KTRACE_OPT(KBOOT,Kern::Printf("%d banks, max bank blocks %08x", nBanks, maxBankBlocks));
	if (nBanks>1)
		{
		MM::SecondaryAllocator = TBitMapAllocator::New(maxBankBlocks, ETrue);
		if (!MM::SecondaryAllocator)
			MM::Panic(MM::ESecAllocCreateFailed);
		}
	}

void M::Init3()
	{
	// Third phase MMU initialisation
	}

void M::Init4()
    {
    // Fourth phase MMU initialisation - Not required on this memory model.
    }

TInt M::InitSvHeapChunk(DChunk* aChunk, TInt aSize)
	{
	DMemModelChunk* pC=(DMemModelChunk*)aChunk;
	TLinAddr base = TheRomHeader().iKernDataAddress;
	K::HeapInfo.iChunk = aChunk;
	K::HeapInfo.iBase = (TUint8*)base;
	K::HeapInfo.iMaxSize = pC->MaxSize();
	pC->SetFixedAddress(base, aSize);
	__KTRACE_OPT(KBOOT,Kern::Printf("Created SvHeap chunk, addr %08X, init size %08X max size %08X",pC->Base(),aSize,pC->MaxSize()));
	return KErrNone;
	}

TInt M::InitSvStackChunk()
	{
	return KErrNone;
	}

