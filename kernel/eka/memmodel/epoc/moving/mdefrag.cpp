// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\epoc\moving\mdefrag.cpp
// 
//

#include <memmodel.h>
#include <defrag.h>
#include "mmboot.h"
#include <mmubase.inl>
#include <ramalloc.h>
#include "cache_maintenance.h"

/*
 * Move a kernel page from aOld to aNew, updating the page table in aChunk.
 * Enter with system locked, exit with system unlocked (!!)
 * Must hold RAM alloc mutex.
 */
TInt Mmu::MoveKernelPage(DChunk* aChunk, TUint32 aOffset, TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::MoveKernelPage() off=%08x old=%08x",aOffset,aOld));
	Mmu& m=Mmu::Get();
	
	// Release the system lock - the kernel chunks can't ever be freed
	// and the ramalloc mutex protects us from decommit.
	NKern::UnlockSystem();

	DMemModelChunk* chunk = (DMemModelChunk*)aChunk;

	// Allocate new page, map it uncached but buffered, and find old mapping
	TPhysAddr newPage;
	if (m.AllocRamPages(&newPage, 1, EPageMovable, aBlockZoneId, aBlockRest) != KErrNone)
		return KErrNoMemory;
	TLinAddr vOld = (TLinAddr)chunk->iBase + aOffset;
	TLinAddr vNew = m.MapTemp(newPage, EFalse);
	
	// Find page table for mapping
	TInt ptid=m.GetPageTableId(vOld);
	if(ptid<0)
		Panic(EDefragKernelChunkNoPageTable);

	// With the system lock, ask Mmu to remap the page.
	// This will copy and remap it with interrupts disabled, while
	// avoiding touching any cache lines on the heap.
	NKern::LockSystem();
	m.RemapKernelPage(ptid, vOld, vNew, newPage, chunk->iPtePermissions);

	// update new pageinfo, clear old, then done with system lock.
	SPageInfo* oldpi = SPageInfo::FromPhysAddr(aOld);
	SPageInfo* pi = SPageInfo::FromPhysAddr(newPage);
	pi->Set(oldpi->Type(),oldpi->Owner(),oldpi->Offset());
	oldpi->SetUnused();
	NKern::UnlockSystem();

	// Remove temporary new page mapping
	m.UnmapTemp();

	// Remove old page from external cache - RemapKernelPage has already removed it from internal cache.
	CacheMaintenance::PageToReusePhysicalCache(aOld);

	// Free old page
#ifdef _DEBUG
	m.ClearPages(1, (TPhysAddr*)(aOld|1));
#endif
	m.iRamPageAllocator->FreeRamPage(aOld, EPageMovable);

	aNew = newPage;
	return KErrNone;
	}

/* 
 * These pages don't exist on moving memory model, no need to move them
 * but this function must exist to make the kernel link.
 */
TInt Mmu::MoveCodeSegMemoryPage(DMemModelCodeSegMemory* /*aCodeSegMemory*/, TUint32 /*aOffset*/, TPhysAddr /*aOld*/,
		TPhysAddr& /*aNew*/, TUint /*aBlockZoneId*/, TBool /*aBlockRest*/)
	{
	NKern::UnlockSystem();
	return KErrNotSupported;
	}

/*
 * Move a code chunk page from aOld to aNew, updating the page table in aChunk.
 * Enter with system locked, exit with system unlocked (!!)
 * Must hold RAM alloc mutex.
 */
TInt Mmu::MoveCodeChunkPage(DChunk* aChunk, TUint32 aOffset, TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::MoveCodeChunkPage() off=%08x old=%08x",aOffset,aOld));
	Mmu& m=Mmu::Get();

	// look up the code seg that corresponds to this page
	TLinAddr aLinearAddress = (TLinAddr)(aChunk->Base() + aOffset);
	DMemModelCodeSeg* codeseg = (DMemModelCodeSeg*)DCodeSeg::CodeSegsByAddress.Find(aLinearAddress);

	// if the code seg is not done loading yet, we can't move it the easy way
	// also, if it's being unloaded the codeseg will have gone.
	if (!codeseg || !(codeseg->iMark & DCodeSeg::EMarkLoaded))
		{
		NKern::UnlockSystem();
		return KErrInUse;
		}

	// Release system lock as page can't be decommitted while we hold ramalloc mutex
	NKern::UnlockSystem();

	// Allocate new page, map it uncached but buffered, and find old mapping
	TPhysAddr newPage;
	if (m.AllocRamPages(&newPage, 1, EPageMovable, aBlockZoneId, aBlockRest) != KErrNone)
		return KErrNoMemory;
	TLinAddr vOld = aLinearAddress;
	TLinAddr vNew = m.MapTemp(newPage, EFalse);
	
	// Copy the page and remap it
	pagecpy((TAny*)vNew, (TAny*)vOld);
	NKern::LockSystem();
	// Substitute drains the write buffer for us during the remap.
	aChunk->Substitute(aOffset, aOld, newPage);
	NKern::UnlockSystem();

	// Remove temporary new page mapping
	m.UnmapTemp();

	// Remove old page from physical cache
	CacheMaintenance::PageToReusePhysicalCache(aOld);

	// Free old page
#ifdef _DEBUG
	m.ClearPages(1, (TPhysAddr*)(aOld|1));
#endif
	m.iRamPageAllocator->FreeRamPage(aOld, EPageMovable);

	aNew = newPage;
	return KErrNone;
	}

/*
 * Move a data chunk page from aOld to aNew, updating the page table in aChunk.
 * Enter with system locked, exit with system unlocked (!!)
 * Must hold RAM alloc mutex.
 */
TInt Mmu::MoveDataChunkPage(DChunk* aChunk, TUint32 aOffset, TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::MoveDataChunkPage() off=%08x old=%08x",aOffset,aOld));
	Mmu& m=Mmu::Get();
	TInt r;

	// Release system lock as page can't be decommitted while we hold ramalloc mutex
	NKern::UnlockSystem();

	// Allocate new page, map it uncached but buffered
	TPhysAddr newPage;
	if (m.AllocRamPages(&newPage, 1, EPageMovable, aBlockZoneId, aBlockRest) != KErrNone)
		return KErrNoMemory;
	TLinAddr vNew = m.MapTemp(newPage, EFalse);

	// Mark the PTE as inaccessible to avoid the data being overwritten while we copy
	// This also takes care of the cache requirements to alias the page elsewhere,
	// since it can't be copied from an inaccessible PTE
	DisablePageModification((DMemModelChunk*)aChunk, aOffset);
	TLinAddr vOldAlias = m.MapSecondTemp(aOld, ETrue);

	// Copy the page's contents and remap its PTE
	pagecpy((TAny*)vNew, (TAny*)vOldAlias);
	NKern::LockSystem();
	if (iDisabledPte != NULL)
		{
		// Access wasn't reenabled, so we can continue
		aChunk->Substitute(aOffset, aOld, newPage);
		iDisabledAddr = 0;
		iDisabledPte = NULL;
		iDisabledOldVal = 0;
		r = KErrNone;
		}
	else
		r = KErrInUse;
	NKern::UnlockSystem();

	// Remove temporary page mappings
	CacheMaintenance::PageToReuseVirtualCache(vOldAlias);
	m.UnmapTemp();
	m.UnmapSecondTemp();

	if (r == KErrNone)
		{
		// Remove old page from physical cache - DisablePageModification removed it from L1 already
		CacheMaintenance::PageToReusePhysicalCache(aOld);
		}

	if (r == KErrNone)
		{
		// Free old page
#ifdef _DEBUG
		m.ClearPages(1, (TPhysAddr*)(aOld|1));
#endif
		m.iRamPageAllocator->FreeRamPage(aOld, EPageMovable);
		aNew = newPage;
		}
	else
		{
		// Free new page
		m.iRamPageAllocator->FreeRamPage(newPage, EPageMovable);
		}

	return r;
	}
