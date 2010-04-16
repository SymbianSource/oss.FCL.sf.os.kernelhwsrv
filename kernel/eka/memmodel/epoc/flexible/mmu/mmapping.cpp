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

#include <plat_priv.h>
#include "mm.h"
#include "mmu.h"
#include "mmapping.h"
#include "mobject.h"
#include "maddressspace.h"
#include "mptalloc.h"
#include "mmanager.h" // needed for DMemoryManager::Pin/Unpin, not nice, but no obvious way to break dependency
#include "cache_maintenance.inl"

//
// DMemoryMapping
//

DMemoryMapping::DMemoryMapping(TUint aType)
	: DMemoryMappingBase(aType)
	{
	}


TInt DMemoryMapping::Construct(TMemoryAttributes aAttributes, TMappingCreateFlags aFlags, TInt aOsAsid, TLinAddr aAddr, TUint aSize, TLinAddr aColourOffset)
	{
	TRACE(("DMemoryMapping[0x%08x]::Construct(0x%x,0x%x,%d,0x%08x,0x%08x,0x%08x)",this,(TUint32&)aAttributes,aFlags,aOsAsid,aAddr,aSize,aColourOffset));

	// setup PDE values...
	iBlankPde = Mmu::BlankPde(aAttributes);

	// setup flags...
	if(aFlags&EMappingCreateReserveAllResources)
		Flags() |= EPermanentPageTables;

	// allocate virtual memory...
	TInt r = AllocateVirtualMemory(aFlags,aOsAsid,aAddr,aSize,aColourOffset);
	if(r==KErrNone)
		{
		// add to address space...
		TLinAddr addr = iAllocatedLinAddrAndOsAsid&~KPageMask;
		TInt osAsid = iAllocatedLinAddrAndOsAsid&KPageMask;
		r = AddressSpace[osAsid]->AddMapping(addr,this);
		if(r!=KErrNone)
			FreeVirtualMemory();
		}

	return r;
	}


DMemoryMapping::~DMemoryMapping()
	{
	TRACE(("DMemoryMapping[0x%08x]::~DMemoryMapping()",this));
	Destruct();
	}


void DMemoryMapping::Destruct()
	{
	__NK_ASSERT_DEBUG(!IsAttached());

	// remove from address space...
	TLinAddr addr = iAllocatedLinAddrAndOsAsid&~KPageMask;
	TInt osAsid = iAllocatedLinAddrAndOsAsid&KPageMask;
	TAny* removed = AddressSpace[osAsid]->RemoveMapping(addr);
	if(removed)
		__NK_ASSERT_DEBUG(removed==this);

	FreeVirtualMemory();
	}


void DMemoryMapping::BTraceCreate()
	{
	MmuLock::Lock();
	TUint32 data[4] = { iStartIndex, iSizeInPages, OsAsid(), Base() };
	BTraceContextN(BTrace::EFlexibleMemModel,BTrace::EMemoryMappingCreate,this,Memory(),data,sizeof(data));
	MmuLock::Unlock();
	}


TInt DMemoryMapping::Map(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TMappingPermissions aPermissions)
	{
	TRACE(("DMemoryMapping[0x%08x]::Map(0x%08x,0x%x,0x%x,0x%08x)",this,aMemory,aIndex,aCount,aPermissions));
	__NK_ASSERT_DEBUG(!IsAttached());

	// check reserved resources are compatible (memory objects with reserved resources 
	// don't expect to have to allocate memory when mapping new pages),,,
	if(aMemory->iFlags&DMemoryObject::EReserveResources && !(Flags()&EPermanentPageTables))
		return KErrArgument;

	// check arguments for coarse mappings...
	if(IsCoarse())
		{
		if(!aMemory->IsCoarse())
			return KErrArgument;
		if((aCount|aIndex)&(KChunkMask>>KPageShift))
			return KErrArgument;
		}

	TLinAddr base = iAllocatedLinAddrAndOsAsid & ~KPageMask;
	TLinAddr top = base + (aCount << KPageShift);

	// check user/supervisor memory partitioning...
	if (aPermissions & EUser)
		{
		if (base > KUserMemoryLimit || top > KUserMemoryLimit)
			return KErrAccessDenied;
		}
	else
		{
		if (base < KUserMemoryLimit || top < KUserMemoryLimit)
			return KErrAccessDenied;
		}

	// check that mapping doesn't straddle KUserMemoryLimit or KGlobalMemoryBase ...
	__NK_ASSERT_DEBUG((base < KUserMemoryLimit) == (top <= KUserMemoryLimit));
	__NK_ASSERT_DEBUG((base < KGlobalMemoryBase) == (top <= KGlobalMemoryBase));

	// check that only global memory is mapped into the kernel process
	TBool global = base >= KGlobalMemoryBase;
	__NK_ASSERT_DEBUG(global || (iAllocatedLinAddrAndOsAsid & KPageMask) != KKernelOsAsid);

	// setup attributes...
	PteType() =	Mmu::PteType(aPermissions,global);
	iBlankPte = Mmu::BlankPte(aMemory->Attributes(),PteType());

	// setup base address... 
	TUint colourOffset = ((aIndex&KPageColourMask)<<KPageShift);
	if(colourOffset+aCount*KPageSize > iAllocatedSize)
		return KErrTooBig;
	__NK_ASSERT_DEBUG(!iLinAddrAndOsAsid || ((iLinAddrAndOsAsid^iAllocatedLinAddrAndOsAsid)&~(KPageColourMask<<KPageShift))==0); // new, OR, only differ in page colour
	iLinAddrAndOsAsid = iAllocatedLinAddrAndOsAsid+colourOffset;

	// attach to memory object...
	TInt r = Attach(aMemory,aIndex,aCount);

	// cleanup if error...
	if(r!=KErrNone)
		iLinAddrAndOsAsid = 0;

	return r;
	}


void DMemoryMapping::Unmap()
	{
	Detach();
	// we can't clear iLinAddrAndOsAsid here because this may be needed by other code,
	// e.g. DFineMapping::MapPages/UnmapPages/RestrictPages/PageIn
	}


TInt DMemoryMapping::AllocateVirtualMemory(TMappingCreateFlags aFlags, TInt aOsAsid, TLinAddr aAddr, TUint aSize, TLinAddr aColourOffset)
	{
	TRACE(("DMemoryMapping[0x%08x]::AllocateVirtualMemory(0x%x,%d,0x%08x,0x%08x,0x%08x)",this,aFlags,aOsAsid,aAddr,aSize,aColourOffset));
	__NK_ASSERT_DEBUG((aAddr&KPageMask)==0);
	__NK_ASSERT_DEBUG(!iAllocatedLinAddrAndOsAsid);
	__NK_ASSERT_DEBUG(!iAllocatedSize);

	// setup PDE type...
	TUint pdeType = 0;
	if(aFlags&EMappingCreateCommonVirtual)
		pdeType |= EVirtualSlabTypeCommonVirtual;
	if(aFlags&EMappingCreateDemandPaged)
		pdeType |= EVirtualSlabTypeDemandPaged;

	TInt r;
	TUint colourOffset = aColourOffset&(KPageColourMask<<KPageShift);
	TLinAddr addr;
	TUint size;
	if(aFlags&(EMappingCreateFixedVirtual|EMappingCreateAdoptVirtual))
		{
		// just use the supplied virtual address...
		__NK_ASSERT_ALWAYS(aAddr);
		__NK_ASSERT_ALWAYS(colourOffset==0);
		__NK_ASSERT_DEBUG((aFlags&EMappingCreateAdoptVirtual)==0 || AddressSpace[aOsAsid]->CheckPdeType(aAddr,aSize,pdeType));
		addr = aAddr;
		size = aSize;
		r = KErrNone;
		}
	else
		{
		if(aFlags&(EMappingCreateExactVirtual|EMappingCreateCommonVirtual))
			{
			__NK_ASSERT_ALWAYS(aAddr); // address must be specified
			}
		else
			{
			__NK_ASSERT_ALWAYS(!aAddr); // address shouldn't have been specified
			}

		// adjust for colour...
		TUint allocSize = aSize+colourOffset;
		TUint allocAddr = aAddr;
		if(allocAddr)
			{
			allocAddr -= colourOffset;
			if(allocAddr&(KPageColourMask<<KPageShift))
				return KErrArgument; // wrong colour
			}

		// allocate virtual addresses...
		if(aFlags&EMappingCreateUserGlobalVirtual)
			{
			if(aOsAsid!=(TInt)KKernelOsAsid)
				return KErrArgument;
			r = DAddressSpace::AllocateUserGlobalVirtualMemory(addr,size,allocAddr,allocSize,pdeType);
			}
		else
			r = AddressSpace[aOsAsid]->AllocateVirtualMemory(addr,size,allocAddr,allocSize,pdeType);
		}

	if(r==KErrNone)
		{
		iAllocatedLinAddrAndOsAsid = addr|aOsAsid;
		iAllocatedSize = size;
		}

	TRACE(("DMemoryMapping[0x%08x]::AllocateVirtualMemory returns %d address=0x%08x",this,r,addr));
	return r;
	}


void DMemoryMapping::FreeVirtualMemory()
	{
	if(!iAllocatedSize)
		return; // no virtual memory to free

	TRACE(("DMemoryMapping[0x%08x]::FreeVirtualMemory()",this));

	iLinAddrAndOsAsid = 0;

	TLinAddr addr = iAllocatedLinAddrAndOsAsid&~KPageMask;
	TInt osAsid = iAllocatedLinAddrAndOsAsid&KPageMask;
	AddressSpace[osAsid]->FreeVirtualMemory(addr,iAllocatedSize);
	iAllocatedLinAddrAndOsAsid = 0;
	iAllocatedSize = 0;
	}



//
// DCoarseMapping
//

DCoarseMapping::DCoarseMapping()
	: DMemoryMapping(ECoarseMapping)
	{
	}


DCoarseMapping::DCoarseMapping(TUint aFlags)
	: DMemoryMapping(ECoarseMapping|aFlags)
	{
	}


DCoarseMapping::~DCoarseMapping()
	{
	}


TInt DCoarseMapping::DoMap()
	{
	TRACE(("DCoarseMapping[0x%08x]::DoMap()", this));
	__NK_ASSERT_DEBUG(((iStartIndex|iSizeInPages)&(KChunkMask>>KPageShift))==0); // be extra paranoid about alignment

	MmuLock::Lock();
	TPde* pPde = Mmu::PageDirectoryEntry(OsAsid(),Base());
	DCoarseMemory* memory = (DCoarseMemory*)Memory(true); // safe because we're called from code which has added mapping to memory
	
	TUint flash = 0;
	TUint chunk = iStartIndex >> KPagesInPDEShift;
	TUint endChunk = (iStartIndex + iSizeInPages) >> KPagesInPDEShift;
	TBool sectionMappingsBroken = EFalse;
	
	while(chunk < endChunk)
		{
		MmuLock::Flash(flash,KMaxPdesInOneGo*2);
		TPte* pt = memory->GetPageTable(PteType(), chunk);
		if(!pt)
			{
			TRACE2(("!PDE %x=%x (was %x)",pPde,KPdeUnallocatedEntry,*pPde));
			__NK_ASSERT_DEBUG(*pPde==KPdeUnallocatedEntry);
			}
		else
			{
			TPde pde = Mmu::PageTablePhysAddr(pt)|iBlankPde;
#ifdef	__USER_MEMORY_GUARDS_ENABLED__
			if (IsUserMapping())
				pde = PDE_IN_DOMAIN(pde, USER_MEMORY_DOMAIN);
#endif
			TRACE2(("!PDE %x=%x (was %x)",pPde,pde,*pPde));
			if (Mmu::PdeMapsSection(*pPde))
				{
				// break previous section mapping...
				__NK_ASSERT_DEBUG(*pPde==Mmu::PageToSectionEntry(pt[0],iBlankPde));
				sectionMappingsBroken = ETrue;
				}
			else
				__NK_ASSERT_DEBUG(*pPde==KPdeUnallocatedEntry || ((*pPde^pde)&~KPdeMatchMask)==0); 
			*pPde = pde;
			SinglePdeUpdated(pPde);
			flash += 3; // increase flash rate because we've done quite a bit more work
			}
		++pPde;
		++chunk;
		}
	MmuLock::Unlock();

	if (sectionMappingsBroken)
		{
		// We must invalidate the TLB since we broke section mappings created by the bootstrap.
		// Since this will only ever happen on boot, we just invalidate the entire TLB for this
		// process.
		InvalidateTLBForAsid(OsAsid());
		}

	return KErrNone;
	}


void DCoarseMapping::DoUnmap()
	{
	TRACE(("DCoarseMapping[0x%08x]::DoUnmap()", this));
	MmuLock::Lock();
	TPde* pPde = Mmu::PageDirectoryEntry(OsAsid(),Base());
	TPde* pPdeEnd = pPde+(iSizeInPages>>(KChunkShift-KPageShift));
	TUint flash = 0;
	do
		{
		MmuLock::Flash(flash,KMaxPdesInOneGo);
		TPde pde = KPdeUnallocatedEntry;
		TRACE2(("!PDE %x=%x",pPde,pde));
		*pPde = pde;
		SinglePdeUpdated(pPde);
		++pPde;
		}
	while(pPde<pPdeEnd);
	MmuLock::Unlock();

	InvalidateTLBForAsid(OsAsid());
	}


TInt DCoarseMapping::MapPages(RPageArray::TIter aPages, TUint aMapInstanceCount)
	{
	// shouldn't ever be called because coarse mappings don't have their own page tables...
	__NK_ASSERT_DEBUG(0);
	return KErrNotSupported;
	}


void DCoarseMapping::UnmapPages(RPageArray::TIter aPages, TUint aMapInstanceCount)
	{
	// shouldn't ever be called because coarse mappings don't have their own page tables...
	__NK_ASSERT_DEBUG(0);
	}

void DCoarseMapping::RemapPage(TPhysAddr& aPageArray, TUint aIndex, TUint aMapInstanceCount, TBool aInvalidateTLB)
	{
	// shouldn't ever be called because coarse mappings don't have their own page tables...
	__NK_ASSERT_DEBUG(0);
	}

void DCoarseMapping::RestrictPagesNA(RPageArray::TIter aPages, TUint aMapInstanceCount)
	{
	// shouldn't ever be called because coarse mappings don't have their own page tables...
	__NK_ASSERT_DEBUG(0);
	}


TInt DCoarseMapping::PageIn(RPageArray::TIter aPages, TPinArgs& aPinArgs, TUint aMapInstanceCount)
	{
	MmuLock::Lock();

	if(!IsAttached())
		{
		MmuLock::Unlock();
		return KErrNotFound;
		}

	DCoarseMemory* memory = (DCoarseMemory*)Memory(true); // safe because we've checked mapping IsAttached
	return memory->PageIn(this, aPages, aPinArgs, aMapInstanceCount);
	}


TBool DCoarseMapping::MovingPageIn(TPhysAddr& aPageArrayPtr, TUint aIndex)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(IsAttached());

	DCoarseMemory* memory = (DCoarseMemory*)Memory(true); // safe because we've checked mapping IsAttached
	TBool success = memory->MovingPageIn(this, aPageArrayPtr, aIndex);
	if (success)
		{
		TLinAddr addr = Base() + (aIndex - iStartIndex) * KPageSize;
		InvalidateTLBForPage(addr);
		}
	return success;
	}


TPte* DCoarseMapping::FindPageTable(TLinAddr aLinAddr, TUint aMemoryIndex)
	{
	TRACE(("DCoarseMapping::FindPageTable(0x%x, %d)", aLinAddr, aMemoryIndex));
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(IsAttached());
	DCoarseMemory* memory = (DCoarseMemory*)Memory(true); // safe because we've checked mapping IsAttached
	return memory->FindPageTable(this, aLinAddr, aMemoryIndex);
	}



//
// DFineMapping
//

DFineMapping::DFineMapping()
	: DMemoryMapping(0)
	{
	}


DFineMapping::~DFineMapping()
	{
	TRACE(("DFineMapping[0x%08x]::~DFineMapping()",this));
	FreePermanentPageTables();
	}

#ifdef _DEBUG
void DFineMapping::ValidatePageTable(TPte* aPt, TLinAddr aAddr)
	{
	if(aPt)
		{
		// check page table is correct...
		SPageTableInfo* pti = SPageTableInfo::FromPtPtr(aPt);
		__NK_ASSERT_DEBUG(pti->CheckFine(aAddr&~KChunkMask,OsAsid()));
		DMemoryObject* memory = Memory();
		if(memory)
			{
			if(memory->IsDemandPaged() && !IsPinned() && !(Flags()&EPageTablesAllocated))
				__NK_ASSERT_DEBUG(pti->IsDemandPaged());
			else
				__NK_ASSERT_DEBUG(!pti->IsDemandPaged());
			}
		}
	}
#endif

TPte* DFineMapping::GetPageTable(TLinAddr aAddr)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	// get address of PDE which refers to the page table...
	TPde* pPde = Mmu::PageDirectoryEntry(OsAsid(),aAddr);

	// get page table...
	TPte* pt = Mmu::PageTableFromPde(*pPde);
#ifdef _DEBUG
	ValidatePageTable(pt, aAddr);
#endif
	return pt;
	}


TPte* DFineMapping::GetOrAllocatePageTable(TLinAddr aAddr)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	// get address of PDE which refers to the page table...
	TPde* pPde = Mmu::PageDirectoryEntry(OsAsid(),aAddr);

	// get page table...
	TPte* pt = Mmu::PageTableFromPde(*pPde);
	if(!pt)
		{
		pt = AllocatePageTable(aAddr,pPde);
#ifdef _DEBUG
		ValidatePageTable(pt, aAddr);
#endif
		}

	return pt;
	}


TPte* DFineMapping::GetOrAllocatePageTable(TLinAddr aAddr, TPinArgs& aPinArgs)
	{
	__NK_ASSERT_DEBUG(aPinArgs.iPinnedPageTables);

	if(!aPinArgs.HaveSufficientPages(KNumPagesToPinOnePageTable))
		return 0;

	TPte* pinnedPt = 0;
	for(;;)
		{
		TPte* pt = GetOrAllocatePageTable(aAddr);

		if(pinnedPt && pinnedPt!=pt)
			{
			// previously pinned page table not needed...
			::PageTables.UnpinPageTable(pinnedPt,aPinArgs);

			// make sure we have memory for next pin attempt...
			MmuLock::Unlock();
			aPinArgs.AllocReplacementPages(KNumPagesToPinOnePageTable);
			if(!aPinArgs.HaveSufficientPages(KNumPagesToPinOnePageTable)) // if out of memory...
				{
				// make sure we free any unneeded page table we allocated...
				if(pt)
					FreePageTable(Mmu::PageDirectoryEntry(OsAsid(),aAddr));
				MmuLock::Lock();
				return 0;
				}
			MmuLock::Lock();
			}

		if(!pt)
			return 0; // out of memory

		if(pt==pinnedPt)
			{
			// we got a page table and it was pinned...
			*aPinArgs.iPinnedPageTables++ = pt;
			++aPinArgs.iNumPinnedPageTables;
			return pt;
			}

		// don't pin page table if it's not paged (e.g. unpaged part of ROM)...
		SPageTableInfo* pti = SPageTableInfo::FromPtPtr(pt);
		if(!pti->IsDemandPaged())
			return pt;

		// pin the page table...
		if (::PageTables.PinPageTable(pt,aPinArgs) != KErrNone)
			{// Couldn't pin the page table...
			MmuLock::Unlock();
			// make sure we free any unneeded page table we allocated...
			FreePageTable(Mmu::PageDirectoryEntry(OsAsid(),aAddr));
			MmuLock::Lock();
			return 0;
			}

		pinnedPt = pt;
		}
	}


TInt DFineMapping::AllocateVirtualMemory(TMappingCreateFlags aFlags, TInt aOsAsid, TLinAddr aAddr, TUint aSize, TLinAddr aColourOffset)
	{
	TInt r = DMemoryMapping::AllocateVirtualMemory(aFlags,aOsAsid,aAddr,aSize,aColourOffset);
	if(r==KErrNone && (Flags()&EPermanentPageTables))
		{
		r = AllocatePermanentPageTables();
		if(r!=KErrNone)
			FreeVirtualMemory();
		}
	return r;
	}


void DFineMapping::FreeVirtualMemory()
	{
	FreePermanentPageTables();
	DMemoryMapping::FreeVirtualMemory();
	}


TPte* DFineMapping::AllocatePageTable(TLinAddr aAddr, TPde* aPdeAddress, TBool aPermanent)
	{
	TRACE2(("DFineMapping[0x%08x]::AllocatePageTable(0x%08x,0x%08x,%d)",this,aAddr,aPdeAddress,aPermanent));

	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	for(;;)
		{
		// mapping is going, so we don't need a page table any more...
		if(BeingDetached())
			return 0;

		// get paged state...
		TBool demandPaged = false;
		if(!aPermanent)
			{
			DMemoryObject* memory = Memory();
			__NK_ASSERT_DEBUG(memory); // can't be NULL because not BeingDetached()
			demandPaged = memory->IsDemandPaged();
			}

		// get page table...
		TPte* pt = Mmu::PageTableFromPde(*aPdeAddress);
		if(pt!=0)
			{
			// we have a page table...
			__NK_ASSERT_DEBUG(SPageTableInfo::FromPtPtr(pt)->CheckFine(aAddr&~KChunkMask,iAllocatedLinAddrAndOsAsid&KPageMask));
			if(aPermanent)
				{
				__NK_ASSERT_DEBUG(BeingDetached()==false);
				__NK_ASSERT_ALWAYS(!demandPaged);
				SPageTableInfo* pti = SPageTableInfo::FromPtPtr(pt);
				pti->IncPermanenceCount();
				}
			return pt;
			}

		// allocate a new page table...
		MmuLock::Unlock();
		::PageTables.Lock();
		TPte* newPt = ::PageTables.Alloc(demandPaged);
		if(!newPt)
			{
			// out of memory...
			::PageTables.Unlock();
			MmuLock::Lock();
			return 0;
			}

		// check if new page table is still needed...
		MmuLock::Lock();
		pt = Mmu::PageTableFromPde(*aPdeAddress);
		if(pt)
			{
			// someone else has already allocated a page table,
			// so free the one we just allocated and try again...
			MmuLock::Unlock();
			::PageTables.Free(newPt);
			}
		else if(BeingDetached())
			{
			// mapping is going, so we don't need a page table any more...
			MmuLock::Unlock();
			::PageTables.Free(newPt);
			::PageTables.Unlock();
			MmuLock::Lock();
			return 0;
			}
		else
			{
			// setup new page table...
			SPageTableInfo* pti = SPageTableInfo::FromPtPtr(newPt);
			pti->SetFine(aAddr&~KChunkMask,iAllocatedLinAddrAndOsAsid&KPageMask);

			TPde pde = Mmu::PageTablePhysAddr(newPt)|iBlankPde;
#ifdef	__USER_MEMORY_GUARDS_ENABLED__
			if (IsUserMapping())
				pde = PDE_IN_DOMAIN(pde, USER_MEMORY_DOMAIN);
#endif
			TRACE2(("!PDE %x=%x",aPdeAddress,pde));
			__NK_ASSERT_DEBUG(((*aPdeAddress^pde)&~KPdeMatchMask)==0 || *aPdeAddress==KPdeUnallocatedEntry);
			*aPdeAddress = pde;
			SinglePdeUpdated(aPdeAddress);

			MmuLock::Unlock();
			}

		// loop back and recheck...
		::PageTables.Unlock();
		MmuLock::Lock();
		}
	}


void DFineMapping::FreePageTable(TPde* aPdeAddress)
	{
	TRACE2(("DFineMapping[0x%08x]::FreePageTable(0x%08x)",this,aPdeAddress));

	// get page table lock...
	::PageTables.Lock();
	MmuLock::Lock();

	// find page table...
	TPte* pt = Mmu::PageTableFromPde(*aPdeAddress);
	if(pt)
		{
		SPageTableInfo* pti = SPageTableInfo::FromPtPtr(pt);
		if(pti->PageCount() || pti->PermanenceCount())
			{
			// page table still in use, so don't free it...
			pt = 0;
			}
		else
			{
			// page table not used, so unmap it...
			TPde pde = KPdeUnallocatedEntry;
			TRACE2(("!PDE %x=%x",aPdeAddress,pde));
			*aPdeAddress = pde;
			SinglePdeUpdated(aPdeAddress);
			}
		}

	MmuLock::Unlock();
	if(pt)
		::PageTables.Free(pt);
	::PageTables.Unlock();
	}


void DFineMapping::RemapPage(TPhysAddr& aPageArray, TUint aIndex, TUint aMapInstanceCount, TBool aInvalidateTLB)
	{
	TRACE2(("DFineMemoryMapping[0x%08x]::RemapPage(0x%x,0x%x,%d,%d)",this,aPageArray,aIndex,aMapInstanceCount,aInvalidateTLB));

	__NK_ASSERT_DEBUG(aIndex >= iStartIndex);
	__NK_ASSERT_DEBUG(aIndex < iStartIndex + iSizeInPages);

	TLinAddr addr = Base() + ((aIndex - iStartIndex) << KPageShift);
	TUint pteIndex = (addr >> KPageShift) & (KChunkMask >> KPageShift);

	// get address of page table...
	MmuLock::Lock();
	TPte* pPte = GetPageTable(addr);

	// check the page is still mapped and mapping isn't being detached 
	// or hasn't been reused for another purpose...
	if(!pPte || BeingDetached() || aMapInstanceCount != MapInstanceCount())
		{
		// can't map pages to this mapping any more so just exit.
		MmuLock::Unlock();
		return;
		}

	// remap the page...
	pPte += pteIndex;
	Mmu::RemapPage(pPte, aPageArray, iBlankPte);
	MmuLock::Unlock();

#ifndef COARSE_GRAINED_TLB_MAINTENANCE
	// clean TLB...
	if (aInvalidateTLB)
		{
		InvalidateTLBForPage(addr + OsAsid());
		}
#endif
	}


TInt DFineMapping::MapPages(RPageArray::TIter aPages, TUint aMapInstanceCount)
	{
	TRACE2(("DFineMapping[0x%08x]::MapPages(?,%d) index=0x%x count=0x%x",this,aMapInstanceCount,aPages.Index(),aPages.Count()));

	__NK_ASSERT_DEBUG(aPages.Count());
	__NK_ASSERT_DEBUG(aPages.Index()>=iStartIndex);
	__NK_ASSERT_DEBUG(aPages.IndexEnd()-iStartIndex<=iSizeInPages);

	TLinAddr addr = Base()+(aPages.Index()-iStartIndex)*KPageSize;
	for(;;)
		{
		TUint pteIndex = (addr>>KPageShift)&(KChunkMask>>KPageShift);

		// calculate max number of pages to do...
		TUint n = (KChunkSize>>KPageShift)-pteIndex; // pages left in page table
		if(n>KMaxPagesInOneGo)
			n = KMaxPagesInOneGo;

		// get some pages...
		TPhysAddr* pages;
		n = aPages.Pages(pages,n);
		if(!n)
			break;

		// get address of page table...
		MmuLock::Lock();
		TPte* pPte = GetOrAllocatePageTable(addr);

		// check mapping isn't being unmapped, or been reused for another purpose...
		if(BeingDetached() || aMapInstanceCount!=MapInstanceCount())
			{
			// can't map pages to this mapping any more, so free any page table
			// we just got (if it's not used)...
			if(!pPte)
				MmuLock::Unlock();
			else
				{
				SPageTableInfo* pti = SPageTableInfo::FromPtPtr(pPte);
				TBool keepPt = pti->PermanenceCount() ||  pti->PageCount();
				MmuLock::Unlock();
				if(!keepPt)
					FreePageTable(Mmu::PageDirectoryEntry(OsAsid(),addr));
				}
			// then end...
			return KErrNone;
			}

		// check for OOM...
		if(!pPte)
			{
			MmuLock::Unlock();
			return KErrNoMemory;
			}

		// map some pages...
		pPte += pteIndex;
		TBool keepPt = Mmu::MapPages(pPte, n, pages, iBlankPte);
		MmuLock::Unlock();

		// free page table if no longer needed...
		if(!keepPt)
			FreePageTable(Mmu::PageDirectoryEntry(OsAsid(),addr));

		// move on...
		aPages.Skip(n);
		addr += n*KPageSize;
		}

	return KErrNone;
	}


void DFineMapping::UnmapPages(RPageArray::TIter aPages, TUint aMapInstanceCount)
	{
	TRACE2(("DFineMapping[0x%08x]::UnmapPages(?,%d) index=0x%x count=0x%x",this,aMapInstanceCount,aPages.Index(),aPages.Count()));

	__NK_ASSERT_DEBUG(aPages.Count());

	TLinAddr addr = Base()+(aPages.Index()-iStartIndex)*KPageSize;
#ifndef COARSE_GRAINED_TLB_MAINTENANCE
	TLinAddr startAddr = addr;
#endif
	for(;;)
		{
		TUint pteIndex = (addr>>KPageShift)&(KChunkMask>>KPageShift);

		// calculate max number of pages to do...
		TUint n = (KChunkSize>>KPageShift)-pteIndex; // pages left in page table
		if(n>KMaxPagesInOneGo)
			n = KMaxPagesInOneGo;

		// get some pages...
		TPhysAddr* pages;
		n = aPages.Pages(pages,n);
		if(!n)
			break;

		MmuLock::Lock();

		// check that mapping hasn't been reused for another purpose...
		if(aMapInstanceCount!=MapInstanceCount())
			{
			MmuLock::Unlock();
			break;
			}

		// get address of PTE for pages...
		TPde* pPde = Mmu::PageDirectoryEntry(OsAsid(),addr);
		TPte* pPte = Mmu::PageTableFromPde(*pPde);
		if(pPte)
			{
			// unmap some pages...
			pPte += pteIndex;
			TBool keepPt = Mmu::UnmapPages(pPte,n,pages);
			MmuLock::Unlock();

			// free page table if no longer needed...
			if(!keepPt)
				FreePageTable(pPde);
			}
		else
			{
			// no page table found...
			MmuLock::Unlock();
			}

		// move on...
		aPages.Skip(n);
		addr += n*KPageSize;
		}

#ifndef COARSE_GRAINED_TLB_MAINTENANCE
	// clean TLB...
	TLinAddr endAddr = addr;
	addr = startAddr+OsAsid();
	do InvalidateTLBForPage(addr);
	while((addr+=KPageSize)<endAddr);
#endif
	}


void DFineMapping::RestrictPagesNA(RPageArray::TIter aPages, TUint aMapInstanceCount)
	{
	TRACE2(("DFineMapping[0x%08x]::RestrictPages(?,%d) index=0x%x count=0x%x",this,aMapInstanceCount,aPages.Index(),aPages.Count()));

	__NK_ASSERT_DEBUG(aPages.Count());

	TLinAddr addr = Base()+(aPages.Index()-iStartIndex)*KPageSize;
#ifndef COARSE_GRAINED_TLB_MAINTENANCE
	TLinAddr startAddr = addr;
#endif
	for(;;)
		{
		TUint pteIndex = (addr>>KPageShift)&(KChunkMask>>KPageShift);

		// calculate max number of pages to do...
		TUint n = (KChunkSize>>KPageShift)-pteIndex; // pages left in page table
		if(n>KMaxPagesInOneGo)
			n = KMaxPagesInOneGo;

		// get some pages...
		TPhysAddr* pages;
		n = aPages.Pages(pages,n);
		if(!n)
			break;

		MmuLock::Lock();

		// check that mapping hasn't been reused for another purpose...
		if(aMapInstanceCount!=MapInstanceCount())
			{
			MmuLock::Unlock();
			break;
			}

		// get address of PTE for pages...
		TPde* pPde = Mmu::PageDirectoryEntry(OsAsid(),addr);
		TPte* pPte = Mmu::PageTableFromPde(*pPde);
		if(pPte)
			{
			// restrict some pages...
			pPte += pteIndex;
			Mmu::RestrictPagesNA(pPte,n,pages);
			}
		MmuLock::Unlock();

		// move on...
		aPages.Skip(n);
		addr += n*KPageSize;
		}

#ifndef COARSE_GRAINED_TLB_MAINTENANCE
	// clean TLB...
	TLinAddr endAddr = addr;
	addr = startAddr+OsAsid();
	do InvalidateTLBForPage(addr);
	while((addr+=KPageSize)<endAddr);
#endif
	}


TInt DFineMapping::PageIn(RPageArray::TIter aPages, TPinArgs& aPinArgs, TUint aMapInstanceCount)
	{
	TRACE2(("DFineMapping[0x%08x]::PageIn(?,?,%d) index=0x%x count=0x%x",this,aMapInstanceCount,aPages.Index(),aPages.Count()));

	__NK_ASSERT_DEBUG(aPages.Count());
	__NK_ASSERT_DEBUG(aPages.Index()>=iStartIndex);
	__NK_ASSERT_DEBUG(aPages.IndexEnd()-iStartIndex<=iSizeInPages);

	TInt r = KErrNone;

	TLinAddr addr = Base()+(aPages.Index()-iStartIndex)*KPageSize;
#ifndef COARSE_GRAINED_TLB_MAINTENANCE
	TLinAddr startAddr = addr;
#endif
	TBool pinPageTable = aPinArgs.iPinnedPageTables!=0; // check if we need to pin the first page table
	for(;;)
		{
		TUint pteIndex = (addr>>KPageShift)&(KChunkMask>>KPageShift);
		if(pteIndex==0)
			pinPageTable = aPinArgs.iPinnedPageTables!=0;	// started a new page table, check if we need to pin it

		// calculate max number of pages to do...
		TUint n = (KChunkSize>>KPageShift)-pteIndex; // pages left in page table
		if(n>KMaxPagesInOneGo)
			n = KMaxPagesInOneGo;

		// get some pages...
		TPhysAddr* pages;
		n = aPages.Pages(pages,n);
		if(!n)
			break;

		// make sure we have memory to pin the page table if required...
		if(pinPageTable)
			aPinArgs.AllocReplacementPages(KNumPagesToPinOnePageTable);

		// get address of page table...
		MmuLock::Lock();
		TPte* pPte;
		if(pinPageTable)
			pPte = GetOrAllocatePageTable(addr,aPinArgs);
		else
			pPte = GetOrAllocatePageTable(addr);

		// check mapping isn't being unmapped or hasn't been reused...
		if(BeingDetached() || aMapInstanceCount != MapInstanceCount())
			{
			// can't map pages to this mapping any more, so free any page table
			// we just got (if it's not used)...
			if(!pPte)
				MmuLock::Unlock();
			else
				{
				SPageTableInfo* pti = SPageTableInfo::FromPtPtr(pPte);
				TBool keepPt = pti->PermanenceCount() ||  pti->PageCount();
				MmuLock::Unlock();
				if(!keepPt)
					FreePageTable(Mmu::PageDirectoryEntry(OsAsid(),addr));
				}
			// then end...
			r = KErrNotFound;
			break;
			}

		// check for OOM...
		if(!pPte)
			{
			MmuLock::Unlock();
			r = KErrNoMemory;
			break;
			}

		// map some pages...
		pPte += pteIndex;
		TPte blankPte = iBlankPte;
		if(aPinArgs.iReadOnly)
			blankPte = Mmu::MakePteInaccessible(blankPte,true);
		TBool keepPt = Mmu::PageInPages(pPte, n, pages, blankPte);
		MmuLock::Unlock();

		// free page table if no longer needed...
		if(!keepPt)
			FreePageTable(Mmu::PageDirectoryEntry(OsAsid(),addr));

		// move on...
		aPages.Skip(n);
		addr += n*KPageSize;
		pinPageTable = false;
		}

#ifndef COARSE_GRAINED_TLB_MAINTENANCE
	// clean TLB...
	TLinAddr endAddr = addr;
	addr = startAddr+OsAsid();
	do InvalidateTLBForPage(addr);
	while((addr+=KPageSize)<endAddr);
#endif
	return r;
	}


TBool DFineMapping::MovingPageIn(TPhysAddr& aPageArrayPtr, TUint aIndex)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(IsAttached());
	__NK_ASSERT_DEBUG(!BeingDetached());

	TLinAddr addr = Base() + (aIndex - iStartIndex) * KPageSize;
	TUint pteIndex = (addr >> KPageShift) & (KChunkMask >> KPageShift);

	// get address of page table...
	TPte* pPte = GetPageTable(addr);
	
	// Check the page is still mapped.
	if (!pPte)
		return EFalse;

	// map some pages...
	pPte += pteIndex;
	Mmu::RemapPage(pPte, aPageArrayPtr, iBlankPte);
	InvalidateTLBForPage(addr);
	return ETrue;
	}


TInt DFineMapping::DoMap()
	{
	TRACE(("DFineMapping[0x%08x]::DoMap()", this));
	DMemoryObject* memory = Memory(true); // safe because we're called from code which has added mapping to memory
	if(memory->IsDemandPaged())
		{
		// do nothing, allow pages to be mapped on demand...
		return KErrNone;
		}

	RPageArray::TIter pageIter;
	memory->iPages.FindStart(iStartIndex,iSizeInPages,pageIter);

	// map pages...
	TInt r = KErrNone;
	for(;;)
		{
		// find some pages...
		RPageArray::TIter pageList;
		TUint n = pageIter.Find(pageList);
		if(!n)
			break; // done

		// map some pages...
		r = MapPages(pageList,MapInstanceCount());

		// done with pages...
		pageIter.FindRelease(n);

		if(r!=KErrNone)
			break;
		}

	memory->iPages.FindEnd(iStartIndex,iSizeInPages);
	return r;
	}


void DFineMapping::DoUnmap()
	{
	TRACE2(("DFineMapping[0x%08x]::DoUnmap()",this));

	TLinAddr startAddr = Base();
	TUint count = iSizeInPages;
	TLinAddr addr = startAddr;
	TPde* pPde = Mmu::PageDirectoryEntry(OsAsid(),addr);

	for(;;)
		{
		TUint pteIndex = (addr>>KPageShift)&(KChunkMask>>KPageShift);

		// calculate number of pages to do...
		TUint n = (KChunkSize>>KPageShift)-pteIndex; // pages left in page table
		if(n>count)
			n = count;

		// get page table...
		MmuLock::Lock();
		TPte* pPte = Mmu::PageTableFromPde(*pPde);
		if(!pPte)
			{
			// no page table found, so nothing to do...
			MmuLock::Unlock();
			}
		else
			{
			// unmap some pages...
			pPte += pteIndex;
			if(n>KMaxPagesInOneGo)
				n = KMaxPagesInOneGo;
			TBool keepPt = Mmu::UnmapPages(pPte, n);
			MmuLock::Unlock();

			// free page table if no longer needed...
			if(!keepPt)
				FreePageTable(pPde);
			}

		// move on...
		addr += n*KPageSize;
		count -= n;
		if(!count)
			break;
		if(!(addr&KChunkMask))
			++pPde;
		}

#ifdef COARSE_GRAINED_TLB_MAINTENANCE
	InvalidateTLBForAsid(OsAsid());
#else
	// clean TLB...
	TLinAddr endAddr = addr;
	addr = LinAddrAndOsAsid();
	do InvalidateTLBForPage(addr);
	while((addr+=KPageSize)<endAddr);
#endif
	}


TInt DFineMapping::AllocatePermanentPageTables()
	{
	TRACE2(("DFineMapping[0x%08x]::AllocatePermanentPageTables()",this));
	__NK_ASSERT_DEBUG(((Flags()&EPageTablesAllocated)==0));
	__NK_ASSERT_DEBUG(iBlankPde);

	TLinAddr addr = iAllocatedLinAddrAndOsAsid&~KPageMask;
	TInt osAsid = iAllocatedLinAddrAndOsAsid&KPageMask;
	TPde* pStartPde = Mmu::PageDirectoryEntry(osAsid,addr);
	TPde* pEndPde = Mmu::PageDirectoryEntry(osAsid,addr+iAllocatedSize-1);
	TPde* pPde = pStartPde;

	while(pPde<=pEndPde)
		{
		MmuLock::Lock();
		TPte* pPte = AllocatePageTable(addr,pPde,true);
		if(!pPte)
			{
			// out of memory...
			MmuLock::Unlock();
			FreePermanentPageTables(pStartPde,pPde-1);
			return KErrNoMemory;
			}
		MmuLock::Unlock();

		addr += KChunkSize;
		++pPde;
		}

	TRACE2(("DFineMapping[0x%08x]::AllocatePermanentPageTables() done",this));
	Flags() |= DMemoryMapping::EPageTablesAllocated;
	return KErrNone;
	}


void DFineMapping::FreePermanentPageTables(TPde* aFirstPde, TPde* aLastPde)
	{
	Flags() &= ~DMemoryMapping::EPageTablesAllocated;

	MmuLock::Lock();

	TUint flash = 0;
	TPde* pPde = aFirstPde;
	while(pPde<=aLastPde)
		{
		TPte* pPte = Mmu::PageTableFromPde(*pPde);
		__NK_ASSERT_DEBUG(pPte);
		SPageTableInfo* pti = SPageTableInfo::FromPtPtr(pPte);
		if(pti->DecPermanenceCount() || pti->PageCount())
			{
			// still in use...
			MmuLock::Flash(flash,KMaxPageInfoUpdatesInOneGo*2);
			}
		else
			{
			// page table no longer used for anything...
			MmuLock::Unlock();
			FreePageTable(pPde);
			MmuLock::Lock();
			}

		++pPde;
		}

	MmuLock::Unlock();
	}


void DFineMapping::FreePermanentPageTables()
	{
	if((Flags()&EPageTablesAllocated)==0)
		return;

	TRACE2(("DFineMapping[0x%08x]::FreePermanentPageTables()",this));

	TLinAddr addr = iAllocatedLinAddrAndOsAsid&~KPageMask;
	TInt osAsid = iAllocatedLinAddrAndOsAsid&KPageMask;
	TPde* pPde = Mmu::PageDirectoryEntry(osAsid,addr);
	TPde* pEndPde = Mmu::PageDirectoryEntry(osAsid,addr+iAllocatedSize-1);
	FreePermanentPageTables(pPde,pEndPde);
	}


TPte* DFineMapping::FindPageTable(TLinAddr aLinAddr, TUint aMemoryIndex)
	{
	TRACE(("DFineMapping::FindPageTable(0x%x, %d)", aLinAddr, aMemoryIndex));
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(IsAttached());
	return GetPageTable(aLinAddr);
	}


//
// DKernelPinMapping
//
DKernelPinMapping::DKernelPinMapping()
	// : iReservePages(0)	// Allocated on the kernel heap so will already be 0.
	{
	Flags() |= EPhysicalPinningMapping | EPinned;
	}


TInt DKernelPinMapping::Construct(TUint aReserveMaxSize)
	{
	TInt r = KErrNone;
	if (aReserveMaxSize)
		{
		// Should not call Construct() on a mapping that has already reserved resources.
		__NK_ASSERT_DEBUG(!iReservePages);
		r = DFineMapping::Construct(EMemoryAttributeStandard, 
									EMappingCreateReserveAllResources, 
									KKernelOsAsid, 
									0, 
									aReserveMaxSize, 
									0);
		if (r == KErrNone)
			iReservePages = aReserveMaxSize >> KPageShift;
		}
	return r;
	}


TInt DKernelPinMapping::MapAndPin(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TMappingPermissions aPermissions)
	{
	if (IsAttached())
		{
		return KErrInUse;
		}

	if (!iReservePages)
		{
		TInt r = DFineMapping::Construct(	EMemoryAttributeStandard, 
											EMappingCreateDefault, 
											KKernelOsAsid, 
											0, 
											aCount << KPageShift, 
											0);
		if (r != KErrNone)
			return r;
		}
	// Map the memory, this will pin it first then map it.
	TInt r = DFineMapping::Map(aMemory, aIndex, aCount, aPermissions);

	if (r != KErrNone && !iReservePages)
		{// Reset this mapping object so it can be reused but has freed its address space.
		DMemoryMapping::Destruct();
		}
	return r;
	}


void DKernelPinMapping::UnmapAndUnpin()
	{
	DFineMapping::Unmap();
	if (!iReservePages)
		{// Reset this mapping object so it can be reused but has freed its address space.
		DMemoryMapping::Destruct();
		}
	}


//
// DPhysicalPinMapping
//

DPhysicalPinMapping::DPhysicalPinMapping()
	: DMemoryMappingBase(EPinned|EPhysicalPinningMapping)
	{
	}


TInt DPhysicalPinMapping::Pin(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TMappingPermissions aPermissions)
	{
	PteType() =	Mmu::PteType(aPermissions,true);
	return Attach(aMemory,aIndex,aCount);
	}


void DPhysicalPinMapping::Unpin()
	{
	Detach();
	}


TInt DPhysicalPinMapping::MapPages(RPageArray::TIter /*aPages*/, TUint /*aMapInstanceCount*/)
	{
	// shouldn't ever be called because these mappings are always pinned...
	__NK_ASSERT_DEBUG(0);
	return KErrNotSupported;
	}


void DPhysicalPinMapping::UnmapPages(RPageArray::TIter /*aPages*/, TUint /*aMapInstanceCount*/)
	{
	// nothing to do...
	}


void DPhysicalPinMapping::RemapPage(TPhysAddr& /*aPageArrayPtr*/, TUint /*aIndex*/, TUint /*aMapInstanceCount*/, TBool /*aInvalidateTLB*/)
	{
	// shouldn't ever be called because physically pinned mappings block page moving.
	__NK_ASSERT_DEBUG(0);
	}


void DPhysicalPinMapping::RestrictPagesNA(RPageArray::TIter /*aPages*/, TUint /*aMapInstanceCount*/)
	{
	// nothing to do...
	}


TInt DPhysicalPinMapping::PageIn(RPageArray::TIter /*aPages*/, TPinArgs& /*aPinArgs*/, TUint /*aMapInstanceCount*/)
	{
	// nothing to do...
	return KErrNone;
	}


TInt DPhysicalPinMapping::MovingPageIn(TPhysAddr& /*aPageArrayPtr*/, TUint /*aIndex*/)
	{
	// Should never be asked to page in a page that is being moved as physical 
	// pin mappings don't own any page tables.
	__NK_ASSERT_DEBUG(0);
	return KErrAbort;
	}

TInt DPhysicalPinMapping::DoMap()
	{
	// nothing to do...
	return KErrNone;
	}


void DPhysicalPinMapping::DoUnmap()
	{
	// nothing to do...
	}



//
// DVirtualPinMapping
//

DVirtualPinMapping::DVirtualPinMapping()
	: iMaxCount(0)
	{
	// Clear flag so it is possible to distingish between virtual and physical pin mappings.
	Flags() &= ~EPhysicalPinningMapping;
	}


DVirtualPinMapping::~DVirtualPinMapping()
	{
	TRACE(("DVirtualPinMapping[0x%08x]::~DVirtualPinMapping()",this));
	FreePageTableArray();
	}


DVirtualPinMapping* DVirtualPinMapping::New(TUint aMaxCount)
	{
	TRACE(("DVirtualPinMapping::New(0x%x)",aMaxCount));
	DVirtualPinMapping* self = new DVirtualPinMapping;
	if(self && aMaxCount)
		{
		// pages have been reserved for our use.

		// Create the array for storing pinned paged tables now, so we
		// don't risk out-of-memory errors trying to do so later...
		if(self->AllocPageTableArray(aMaxCount)!=KErrNone)
			{
			// failed, so cleanup...
			self->Close();
			self = 0;
			}
		else
			{
			// success, so remember the pages that have been reserved for us...
			self->iMaxCount = aMaxCount;
			self->Flags() |= EPinningPagesReserved;
			}
		}
	TRACE(("DVirtualPinMapping::New(0x%x) returns 0x%08x",aMaxCount,self));
	return self;
	}


TUint DVirtualPinMapping::MaxPageTables(TUint aPageCount)
	{
	return (aPageCount+2*KChunkSize/KPageSize-2)>>(KChunkShift-KPageShift);
	}


TInt DVirtualPinMapping::AllocPageTableArray(TUint aCount)
	{
	__NK_ASSERT_ALWAYS(iAllocatedPinnedPageTables==0);
	TUint maxPt	= MaxPageTables(aCount);
	if(maxPt>KSmallPinnedPageTableCount)
		{
		iAllocatedPinnedPageTables = new TPte*[maxPt];
		if(!iAllocatedPinnedPageTables)
			return KErrNoMemory;
		}
	return KErrNone;
	}


void DVirtualPinMapping::FreePageTableArray()
	{
	delete [] iAllocatedPinnedPageTables;
	iAllocatedPinnedPageTables = 0;
	}


TPte** DVirtualPinMapping::PageTableArray()
	{
	return iAllocatedPinnedPageTables ? iAllocatedPinnedPageTables : iSmallPinnedPageTablesArray;
	}


TInt DVirtualPinMapping::Pin(	DMemoryObject* aMemory, TUint aIndex, TUint aCount, TMappingPermissions aPermissions, 
								DMemoryMappingBase* aMapping, TUint aMappingInstanceCount)
	{
	// Virtual pinning ensures a page is always mapped to a particular virtual address
	// and therefore require a non-pinning mapping of the virtual address to pin.
	__NK_ASSERT_ALWAYS(aMapping && !aMapping->IsPinned());

	if(iMaxCount)
		{
		if(aCount>iMaxCount)
			return KErrArgument;
		}
	else
		{
		TInt r = AllocPageTableArray(aCount);
		if(r!=KErrNone)
			return r;
		}

	iPinVirtualMapping = aMapping;
	iPinVirtualMapInstanceCount = aMappingInstanceCount;
	TInt r = DPhysicalPinMapping::Pin(aMemory,aIndex,aCount,aPermissions);
	iPinVirtualMapping = 0;

	return r;
	}


void DVirtualPinMapping::Unpin()
	{
	Detach();
	}


void DVirtualPinMapping::UnpinPageTables(TPinArgs& aPinArgs)
	{
	TPte** pPt = PageTableArray();
	TPte** pPtEnd = pPt+iNumPinnedPageTables;

	MmuLock::Lock();
	while(pPt<pPtEnd)
		::PageTables.UnpinPageTable(*pPt++,aPinArgs);
	MmuLock::Unlock();
	iNumPinnedPageTables = 0;

	if(!iMaxCount)
		FreePageTableArray();
	}


void DVirtualPinMapping::RemapPage(TPhysAddr& /*aPageArrayPtr*/, TUint /*aIndex*/, TUint /*aMapInstanceCount*/, TBool /*aInvalidateTLB*/)
	{
	__NK_ASSERT_DEBUG(0);
	}


TInt DVirtualPinMapping::PageIn(RPageArray::TIter aPages, TPinArgs& aPinArgs, TUint aMapInstanceCount)
	{
	if(iPinVirtualMapping)
		return iPinVirtualMapping->PageIn(aPages, aPinArgs, iPinVirtualMapInstanceCount);
	return KErrNone;
	}


TInt DVirtualPinMapping::MovingPageIn(TPhysAddr& /*aPageArrayPtr*/, TUint /*aIndex*/)
	{
	// Should never be asked to page in a page that is being moved as virtual 
	// pin mappings don't own any page tables.
	__NK_ASSERT_DEBUG(0);
	return KErrAbort;
	}


TInt DVirtualPinMapping::DoPin(TPinArgs& aPinArgs)
	{
	// setup for page table pinning...
	aPinArgs.iPinnedPageTables = PageTableArray();

	// do pinning...
	TInt r = DPhysicalPinMapping::DoPin(aPinArgs);

	// save results...
	iNumPinnedPageTables = aPinArgs.iNumPinnedPageTables;
	__NK_ASSERT_DEBUG(iNumPinnedPageTables<=MaxPageTables(iSizeInPages));

	// cleanup if error...
	if(r!=KErrNone)
		UnpinPageTables(aPinArgs);

	return r;
	}


void DVirtualPinMapping::DoUnpin(TPinArgs& aPinArgs)
	{
	DPhysicalPinMapping::DoUnpin(aPinArgs);
	UnpinPageTables(aPinArgs);
	}



//
// DMemoryMappingBase
//


DMemoryMappingBase::DMemoryMappingBase(TUint aType)
	{
	Flags() = aType; // rest of members cleared by DBase
	}


TInt DMemoryMappingBase::Attach(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	TRACE(("DMemoryMappingBase[0x%08x]::Attach(0x%08x,0x%x,0x%x)",this,aMemory,aIndex,aCount));
	__NK_ASSERT_DEBUG(!IsAttached());
	TInt r;

	if(++iMapInstanceCount>1)
		{// This mapping is being reused...

		// Non-pinned mappings can be reused however this is only exercised 
		// by aligned shared buffers whose memory is managed by the unpaged 
		// or hardware memory manager.  Reusing mappings to paged or movable 
		// memory hasn't tested and may need reusing mappings and its 
		// interactions with the fault handler, pinning etc to be tested.
		__NK_ASSERT_DEBUG(	IsPinned() ||
							aMemory->iManager == TheUnpagedMemoryManager || 
							aMemory->iManager == TheHardwareMemoryManager);

		// make sure new instance count is seen by other threads which may be operating
		// on old mapping instance (this will stop them changing the mapping any more)...
		MmuLock::Lock();
		MmuLock::Unlock();
		// clear unmapping flag from previous use...
		__e32_atomic_and_ord16(&Flags(), (TUint16)~(EDetaching|EPageUnmapVetoed));
		}

	__NK_ASSERT_DEBUG((Flags()&(EDetaching|EPageUnmapVetoed))==0);

	// set region being mapped...
	iStartIndex = aIndex;
	iSizeInPages = aCount;

	// reserve any pages required for pinning demand paged memory.
	// We must do this before we add the mapping to the memory object
	// because once that is done the pages we are mapping will be prevented
	// from being paged out. That could leave the paging system without
	// enough pages to correctly handle page faults...
	TPinArgs pinArgs;
	pinArgs.iReadOnly = IsReadOnly();
	if(IsPinned() && aMemory->IsDemandPaged())
		{
		pinArgs.iUseReserve = Flags()&EPinningPagesReserved;
		r = pinArgs.AllocReplacementPages(aCount);
		if(r!=KErrNone)
			return r;
		}

	// link into memory object...
	r = aMemory->AddMapping(this);
	if(r==KErrNone)
		{
		// pin pages if needed...
		if(IsPinned())
			r = DoPin(pinArgs);

		// add pages to this mapping...
		if(r==KErrNone)
			r = DoMap();

		// revert if error...
		if(r!=KErrNone)
			Detach();
		}

	// free any left over pinning pages...
	pinArgs.FreeReplacementPages();

	return r;
	}


void DMemoryMappingBase::Detach()
	{
	TRACE(("DMemoryMappingBase[0x%08x]::Detach()",this));
	__NK_ASSERT_DEBUG(IsAttached());

	// set EDetaching flag, which prevents anyone modifying pages in this
	// mapping, except to remove them...
	MmuLock::Lock();
	__e32_atomic_ior_ord16(&Flags(), (TUint16)EDetaching);
	MmuLock::Unlock();

	// remove all pages from this mapping...
	DoUnmap();

	// unpin pages if needed...
	TPinArgs pinArgs;
	if(IsPinned())
		DoUnpin(pinArgs);

	// unlink from memory object...
	iMemory->RemoveMapping(this);

	// free any spare pages produced by unpinning...
	pinArgs.FreeReplacementPages();
	}


TInt DMemoryMappingBase::DoPin(TPinArgs& aPinArgs)
	{
	DMemoryObject* memory = Memory(true); // safe because we're called from code which has added mapping to memory
	return memory->iManager->Pin(memory,this,aPinArgs);
	}


void DMemoryMappingBase::DoUnpin(TPinArgs& aPinArgs)
	{
	DMemoryObject* memory = Memory(true); // safe because we're called from code which will be removing this mapping from memory afterwards
	memory->iManager->Unpin(memory,this,aPinArgs);
	}


void DMemoryMappingBase::LinkToMemory(DMemoryObject* aMemory, TMappingList& aMappingList)
	{
	TRACE(("DMemoryMappingBase[0x%08x]::LinkToMemory(0x%08x,?)",this,aMemory));
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aMappingList.LockIsHeld());
	__NK_ASSERT_ALWAYS(!IsAttached());
	__NK_ASSERT_DEBUG(!BeingDetached());
	aMappingList.Add(this);
	iMemory = aMemory;
	iMemory->SetMappingAddedFlag();
	}


void DMemoryMappingBase::UnlinkFromMemory(TMappingList& aMappingList)
	{
	TRACE(("DMemoryMappingBase[0x%08x]::UnlinkMapping(?)",this));

	// unlink...
	MmuLock::Lock();
	aMappingList.Lock();
	__NK_ASSERT_DEBUG(IsAttached());
	__NK_ASSERT_DEBUG(BeingDetached());
	aMappingList.Remove(this);
	DMemoryObject* memory = iMemory;
	iMemory = 0;
	aMappingList.Unlock();
	MmuLock::Unlock();

	// if mapping had vetoed any page decommits...
	if(Flags()&DMemoryMapping::EPageUnmapVetoed)
		{
		// then queue cleanup of decommitted pages...
		memory->iManager->QueueCleanup(memory,DMemoryManager::ECleanupDecommitted);
		}
	}


TInt DMemoryMappingBase::PhysAddr(TUint aIndex, TUint aCount, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList)
	{
	__NK_ASSERT_ALWAYS(IsAttached() && IsPhysicalPinning());

	__NK_ASSERT_ALWAYS(TUint(aIndex+aCount)>aIndex && TUint(aIndex+aCount)<=iSizeInPages);
	aIndex += iStartIndex;

	DCoarseMemory* memory = (DCoarseMemory*)Memory(true); // safe because we should only be called whilst memory is Pinned
	TInt r = memory->PhysAddr(aIndex,aCount,aPhysicalAddress,aPhysicalPageList);
	if(r!=KErrNone)
		return r;

	if(memory->IsDemandPaged() && !IsReadOnly())
		{
		// the memory is demand paged and writeable so we need to mark it as dirty
		// as we have to assume that the memory will be modified via the physical
		// addresses we return...
		MmuLock::Lock();
		TPhysAddr* pages = aPhysicalPageList;
		TUint count = aCount;
		while(count)
			{
			SPageInfo* pi = SPageInfo::FromPhysAddr(*(pages++));
			pi->SetDirty();
			if((count&(KMaxPageInfoUpdatesInOneGo-1))==0)
				MmuLock::Flash(); // flash lock every KMaxPageInfoUpdatesInOneGo iterations of the loop
			--count;
			}
		MmuLock::Unlock();
		}

	return KErrNone;
	}



//
// Debug
//

void DMemoryMappingBase::Dump()
	{
#ifdef _DEBUG
	Kern::Printf("DMemoryMappingBase[0x%08x]::Dump()",this);
	Kern::Printf("  IsAttached() = %d",(bool)IsAttached());
	Kern::Printf("  iMemory = 0x%08x",iMemory);
	Kern::Printf("  iStartIndex = 0x%x",iStartIndex);
	Kern::Printf("  iSizeInPages = 0x%x",iSizeInPages);
	Kern::Printf("  Flags() = 0x%x",Flags());
	Kern::Printf("  PteType() = 0x%x",PteType());
#endif // _DEBUG
	}


void DMemoryMapping::Dump()
	{
#ifdef _DEBUG
	Kern::Printf("DMemoryMapping[0x%08x]::Dump()",this);
	Kern::Printf("  Base() = 0x08%x",iLinAddrAndOsAsid&~KPageMask);
	Kern::Printf("  OsAsid() = %d",iLinAddrAndOsAsid&KPageMask);
	Kern::Printf("  iBlankPde = 0x%08x",iBlankPde);
	Kern::Printf("  iBlankPte = 0x%08x",iBlankPte);
	Kern::Printf("  iAllocatedLinAddrAndOsAsid = 0x%08x",iAllocatedLinAddrAndOsAsid);
	Kern::Printf("  iAllocatedSize = 0x%x",iAllocatedSize);
	DMemoryMappingBase::Dump();
#endif // _DEBUG
	}


void DVirtualPinMapping::Dump()
	{
#ifdef _DEBUG
	Kern::Printf("DVirtualPinMapping[0x%08x]::Dump()",this);
	Kern::Printf("  iMaxCount = %d",iMaxCount);
	Kern::Printf("  iNumPinnedPageTables = %d",iNumPinnedPageTables);
	DMemoryMappingBase::Dump();
#endif // _DEBUG
	}

