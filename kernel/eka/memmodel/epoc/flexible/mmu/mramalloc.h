// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Symbian Foundation License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.symbianfoundation.org/legal/sfl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//

#ifndef __RAMALLOC_H__
#define __RAMALLOC_H__
#include <kernel/klib.h>

// RAM information block passed from bootstrap to kernel.
// Consists of two consecutive lists of SRamBank structures, each terminated by
// a bank with iSize=0.
// The first list specifies all available blocks of RAM in the system.
// The second list specifies any blocks of RAM which are reserved and not
// available for general allocation. This should not include RAM mapped by the
// bootstrap - this will be discovered when the initial page mappings are
// analysed.
struct SRamInfo
	{
	SRamBank iBanks[1];				// extend for multiple banks
//	SRamBank iReservedBlocks[1];
	};

const TPhysAddr NULL_PAGE=0xffffffffu;
class DRamAllocatorBase : public DBase
	{
public:
	static DRamAllocatorBase* New(const SRamInfo& aInfo, TInt aPageShift, const SRamBank* aPowerBanks=NULL);
	static void Panic(TInt aPanic);
	FORCE_INLINE TInt FreeRamInPages()
		{return iTotalFreeRamPages;}
	FORCE_INLINE TInt ClaimPhysicalRam(TPhysAddr aBase, TInt aSize)
		{return SetPhysicalRamState(aBase,aSize,EFalse);}
	FORCE_INLINE TInt FreePhysicalRam(TPhysAddr aBase, TInt aSize)
		{return SetPhysicalRamState(aBase,aSize,ETrue);}
public:
	virtual TInt Create(const SRamInfo& aInfo, const SRamBank* aPowerBanks)=0;
	virtual TInt MarkPageAllocated(TPhysAddr aAddr)=0;
	virtual TInt FreeRamPage(TPhysAddr aAddr)=0;
	virtual void FreeRamPages(TPhysAddr* aPageList, TInt aNumPages)=0;
	virtual TInt AllocRamPages(TPhysAddr* aPageList, TInt aNumPages)=0;
	virtual TInt AllocContiguousRam(TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign=0)=0;
	virtual TInt SetPhysicalRamState(TPhysAddr aBase, TInt aSize, TBool aState)=0;
	virtual TUint TotalPhysicalRamPages()=0;
protected:
	static DRamAllocatorBase* New();
protected:
	TInt iPageSize;
	TInt iPageShift;
	TInt iTotalFreeRamPages;
	TInt iNumPowerBlocks;
	TInt* iPowerBlockPages;		// number of pages used in each power block
	TUint32 iPowerState;		// mask of currently used power blocks
	};

#endif

