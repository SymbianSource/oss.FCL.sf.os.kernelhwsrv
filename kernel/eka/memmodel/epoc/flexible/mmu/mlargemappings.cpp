// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include "mlargemappings.h"
#include "cache_maintenance.inl"


//
// DLargeMappedMemory
//


DLargeMappedMemory::DLargeMappedMemory(DMemoryManager* aManager, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags)
	: DCoarseMemory(aManager, aSizeInPages, aAttributes, aCreateFlags)
	{
	}


DLargeMappedMemory* DLargeMappedMemory::New(DMemoryManager* aManager, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags)
	{
	TRACE(("DLargeMappedMemory::New()"));
	TUint chunkCount = (aSizeInPages + KPagesInPDE - 1) >> KPagesInPDEShift;
	TUint wordCount = (chunkCount  + 31) >> 5;
	TUint size = sizeof(DLargeMappedMemory) + sizeof(TUint) * (wordCount - 1);
	DLargeMappedMemory* self = (DLargeMappedMemory*)Kern::AllocZ(size);
	if(self)
		{
		new (self) DLargeMappedMemory(aManager, aSizeInPages, aAttributes, aCreateFlags);
		if(self->Construct()!=KErrNone)
			{
			self->Close();
			self = NULL;
			}
		}
	TRACE(("DLargeMappedMemory::New() returns 0x%08x", self));
	return self;
	}


DLargeMappedMemory::~DLargeMappedMemory()
	{
	TRACE2(("DLargeMappedMemory[0x%08x]::~DLargeMappedMemory()",this));
	}


DMemoryMapping* DLargeMappedMemory::CreateMapping(TUint aIndex, TUint aCount)
	{
	TRACE(("DLargeMappedMemory[0x%08x]::CreateMapping()",this));	
	if (((aIndex|aCount)&(KChunkMask>>KPageShift))==0)
		return new DLargeMapping();
	else
		return new DFineMapping();
	}


TInt DLargeMappedMemory::ClaimInitialPages(TLinAddr aBase,
										   TUint aSize,
										   TMappingPermissions aPermissions,
										   TBool aAllowGaps,
										   TBool aAllowNonRamPages)
	{
	TRACE(("DLargeMappedMemory[0x%08x]::ClaimInitialPages(0x%08x,0x%08x,0x%08x,%d,%d)",
		   this,aBase,aSize,aPermissions,aAllowGaps,aAllowNonRamPages));
	TInt r = DCoarseMemory::ClaimInitialPages(aBase,aSize,aPermissions,aAllowGaps,
											  aAllowNonRamPages);
	if (r != KErrNone)
		return r;

	// set initial contiguous state by checking which pages were section mapped by the bootstrap
	MmuLock::Lock();
	TPde* pPde = Mmu::PageDirectoryEntry(KKernelOsAsid,aBase);	
	TUint endChunk = aSize >> KChunkShift;
	for (TUint chunk = 0 ; chunk < endChunk ; ++chunk)
		{	  
		SetChunkContiguous(chunk, Mmu::PdeMapsSection(*pPde++));
		TRACE(("  chunk %d contiguous state is %d", chunk, IsChunkContiguous(chunk)));
		}
	MmuLock::Unlock();
	
	return KErrNone;
	}


TInt DLargeMappedMemory::MapPages(RPageArray::TIter aPages)
	{
	TRACE2(("DLargeMappedMemory[0x%08x]::MapPages(?) index=0x%x count=0x%x",this,aPages.Index(),aPages.Count()));

	// for now: assert pages do not overlapped a contiguous area
	// todo: update contiguous state, update page tables and call MapPages on large mappings
#ifdef _DEBUG
	for (TUint index = aPages.Index() ; index < aPages.IndexEnd() ; index += KPagesInPDE)
		{
		MmuLock::Lock();
		__NK_ASSERT_DEBUG(!IsChunkContiguous(index >> KPagesInPDEShift));
		MmuLock::Unlock();
		}
#endif

	// map pages in all page tables and fine mappings
	return DCoarseMemory::MapPages(aPages);
	}


void DLargeMappedMemory::RemapPage(TPhysAddr& aPageArray, TUint aIndex, TBool aInvalidateTLB)
	{
	TRACE2(("DLargeMappedMemory[0x%08x]::RemapPage() index=0x%x",this, aIndex));

	// update contiguous state...
	// todo: for now we will assume that remapping a page makes it non-contiguous
	MmuLock::Lock();
	SetChunkContiguous(aIndex >> KPagesInPDEShift, EFalse);
	MmuLock::Unlock();

	// remap pages in all page tables and call RemapPage on large mappings...
	MmuLock::Lock();
	TUint pteType = 0;
	do
		{
		DPageTables* tables = iPageTables[pteType];
		if(tables)
			{
			tables->Open();
			MmuLock::Unlock();
			tables->RemapPage(aPageArray, aIndex, aInvalidateTLB);
			tables->iMappings.RemapPage(aPageArray, aIndex, aInvalidateTLB);
			tables->AsyncClose();
			MmuLock::Lock();
			}
		}
	while(++pteType<ENumPteTypes);
	MmuLock::Unlock();

	// remap page in all fine mappings...
	DMemoryObject::RemapPage(aPageArray, aIndex, aInvalidateTLB);
	}


void DLargeMappedMemory::UnmapPages(RPageArray::TIter aPages, TBool aDecommitting)
	{
	TRACE2(("DLargeMappedMemory[0x%08x]::UnmapPages(?,%d) index=0x%x count=0x%x",this,(bool)aDecommitting,aPages.Index(),aPages.Count()));

	// for now: assert pages do not overlapped a contiguous area
	// todo: update contiguous state, update page tables and call UnmapPages on large mappings
#ifdef _DEBUG
	for (TUint index = aPages.Index() ; index < aPages.IndexEnd() ; index += KPagesInPDE)
		{
		MmuLock::Lock();
		__NK_ASSERT_DEBUG(!IsChunkContiguous(index >> KPagesInPDEShift));
		MmuLock::Unlock();
		}
#endif

	// unmap pages in all page tables and fine mappings
	DCoarseMemory::UnmapPages(aPages, aDecommitting);
	}


void DLargeMappedMemory::RestrictPages(RPageArray::TIter aPages, TRestrictPagesType aRestriction)
	{
	TRACE2(("DLargeMappedMemory[0x%08x]::RestrictPages(?,%d) index=0x%x count=0x%x",this,aRestriction,aPages.Index(),aPages.Count()));

	// assert pages do not overlapped a contiguous area...
#ifdef _DEBUG
	for (TUint index = aPages.Index() ; index < aPages.IndexEnd() ; index += KPagesInPDE)
		{
		MmuLock::Lock();
		__NK_ASSERT_DEBUG(!IsChunkContiguous(index >> KPagesInPDEShift));
		MmuLock::Unlock();
		}
#endif

	DCoarseMemory::RestrictPages(aPages, aRestriction);
	}


TBool DLargeMappedMemory::IsChunkContiguous(TInt aChunkIndex)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	TUint index = aChunkIndex >> 5;
	TUint mask = 1 << (aChunkIndex & 31);
	return (iContiguousState[index] & mask) != 0;
	}


void DLargeMappedMemory::SetChunkContiguous(TInt aChunkIndex, TBool aIsContiguous)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	TUint index = aChunkIndex >> 5;
	TUint mask = 1 << (aChunkIndex & 31);
	iContiguousState[index] = (iContiguousState[index] & ~mask) | (aIsContiguous ? mask : 0);
	}


//
// DLargeMapping
//


DLargeMapping::DLargeMapping() : DCoarseMapping(ELargeMapping)
	{
	}


TInt DLargeMapping::DoMap()
	{
	TRACE(("DLargeMapping[0x%08x]::DoMap()", this));
	__NK_ASSERT_DEBUG(((iStartIndex|iSizeInPages)&(KChunkMask>>KPageShift))==0); // be extra paranoid about alignment

	MmuLock::Lock();

	TPde* pPde = Mmu::PageDirectoryEntry(OsAsid(),Base());
	DLargeMappedMemory* memory = (DLargeMappedMemory*)Memory(ETrue); // safe because we're called from code which has added mapping to memory
	
	TUint flash = 0;
	TUint chunk = iStartIndex >> KPagesInPDEShift;
	TUint endChunk = (iStartIndex + iSizeInPages) >> KPagesInPDEShift;
	
	while(chunk < endChunk)
		{
		MmuLock::Flash(flash,KMaxPdesInOneGo*2);
		TPde pde = KPdeUnallocatedEntry;
		TPte* pt = memory->GetPageTable(PteType(), chunk);
		if (memory->IsChunkContiguous(chunk))
			pde = Mmu::PageToSectionEntry(pt[0],iBlankPde); // todo: use get phys addr?
		else if (pt)
			pde = Mmu::PageTablePhysAddr(pt)|iBlankPde;

		if (pde == KPdeUnallocatedEntry)
			{
			TRACE2(("!PDE %x=%x (was %x)",pPde,KPdeUnallocatedEntry,*pPde));
			__NK_ASSERT_DEBUG(*pPde==KPdeUnallocatedEntry);
			}
		else
			{
			TRACE2(("!PDE %x=%x (was %x)",pPde,pde,*pPde));
			__NK_ASSERT_DEBUG(*pPde==KPdeUnallocatedEntry || ((*pPde^pde)&~KPdeMatchMask)==0);
			*pPde = pde;
			SinglePdeUpdated(pPde);
			flash += 3; // increase flash rate because we've done quite a bit more work
			}

		++pPde;
		++chunk;
		}
	MmuLock::Unlock();

	return KErrNone;
	}


void DLargeMapping::RemapPage(TPhysAddr& aPageArray, TUint aIndex, TUint aMapInstanceCount, TBool aInvalidateTLB)
	{
	TRACE(("DLargeMapping[0x%08x]::RemapPage(%08x, %d, %d, %d)", this, aPageArray, aIndex, aMapInstanceCount, aInvalidateTLB));

	TInt chunkIndex = aIndex >> KPagesInPDEShift;

	MmuLock::Lock();
	DLargeMappedMemory* memory = (DLargeMappedMemory*)Memory(); // safe because we're called from code which has reference on tables, which has reference on memory
	TPte* pt = memory->GetPageTable(PteType(), chunkIndex);
	
	// check the page is still mapped and mapping isn't being detached 
	// or hasn't been reused for another purpose...
	if(!pt || BeingDetached() || aMapInstanceCount != MapInstanceCount())
		{
		// can't map pages to this mapping any more so just exit.
		TRACE(("  page no longer mapped"));
		MmuLock::Unlock();
		return;
		}
	
	TPde* pPde = Mmu::PageDirectoryEntry(OsAsid(),Base() + (chunkIndex << KChunkShift));
	TPde currentPde = *pPde;
	
	if (!memory->IsChunkContiguous(chunkIndex) && Mmu::PdeMapsSection(currentPde))
		{
		// break section mapping and replace with page table...
		TRACE2(("  breaking section mapping"));
		TPde pde = Mmu::PageTablePhysAddr(pt)|iBlankPde;
		TRACE2(("!PDE %x=%x (was %x)",pPde,pde,*pPde));
		// can't assert old value if the first page has been remapped
		__NK_ASSERT_DEBUG((aIndex & (KPagesInPDE - 1)) == 0 ||
						  *pPde == Mmu::PageToSectionEntry(pt[0],iBlankPde));
		*pPde = pde;
		SinglePdeUpdated(pPde);
		MmuLock::Unlock();
#ifndef COARSE_GRAINED_TLB_MAINTENANCE
		if (aInvalidateTLB) 
			{
			// invalidate chunk...
			TUint start = (chunkIndex << KPagesInPDEShift) - iStartIndex;
			TLinAddr addr = LinAddrAndOsAsid() + (start << KPageShift);
			TLinAddr endAddr = addr + KChunkSize;
			do InvalidateTLBForPage(addr);
			while((addr+=KPageSize)<endAddr);
			InvalidateTLBForPage(addr);
			}
#endif
		}
	else if (memory->IsChunkContiguous(chunkIndex) && Mmu::PdeMapsPageTable(currentPde))
		{
		// reform section mapping...
		TRACE2(("  reforming section mapping"));
		__NK_ASSERT_ALWAYS(0); // todo: not yet implemented
		}
	else
		{
		// remap already handled by page table update in DPageTables...
		MmuLock::Unlock();
#ifndef COARSE_GRAINED_TLB_MAINTENANCE
		if (aInvalidateTLB)
			{
			// invalidate page...
			TUint start = aIndex - iStartIndex;
			TLinAddr addr = LinAddrAndOsAsid() + (start << KPageShift);
			InvalidateTLBForPage(addr);
			}
#endif
		}
	
	}


TInt DLargeMapping::PageIn(RPageArray::TIter aPages, TPinArgs& aPinArgs, TUint aMapInstanceCount)
	{
	TRACE(("DLargeMapping[0x%08x]::PageIn(%d, %d, ?, %d)", this, aPages.Index(), aPages.Count(), aMapInstanceCount));
#ifdef _DEBUG
	// assert that we're not trying to page in any section mapped pages
	TUint startIndex = aPages.Index();
	TUint endIndex = startIndex + aPages.Count();
	for (TUint index = startIndex ; index < endIndex ; index += KPagesInPDE)
		{
		TLinAddr addr = Base() + ((index - iStartIndex) << KPageShift);
		TRACE2(("  checking page %d at %08x", index, addr));
		TPde* pPde = Mmu::PageDirectoryEntry(OsAsid(),addr);
		__NK_ASSERT_DEBUG(!Mmu::PdeMapsSection(*pPde));
		}
#endif	
	return DCoarseMapping::PageIn(aPages, aPinArgs, aMapInstanceCount);
	}


TBool DLargeMapping::MovingPageIn(TPhysAddr& aPageArrayPtr, TUint aIndex)
	{
	// this shouldn't ever be called as it's only used by ram defrag
	__NK_ASSERT_DEBUG(EFalse);
	return EFalse;
	}


TPte* DLargeMapping::FindPageTable(TLinAddr aLinAddr, TUint aMemoryIndex)
	{
	// this shouldn't ever be called as it's only used by ram defrag
	__NK_ASSERT_DEBUG(EFalse);
	return NULL;
	}
