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

#include "mobject.h"
#include "mmapping.h"
#include "mptalloc.h"
#include "mmanager.h"
#include "cache_maintenance.inl"

const TUint KMaxMappingsInOneGo = KMaxPageInfoUpdatesInOneGo; // must be power-of-2



//
// MemoryObjectLock
//

/**
The mutex pool used to assign locks to memory objects.
@see #MemoryObjectLock.
*/
DMutexPool MemoryObjectMutexPool;

void MemoryObjectLock::Lock(DMemoryObject* aMemory)
	{
	TRACE2(("MemoryObjectLock::Lock(0x%08x) try",aMemory));
	MemoryObjectMutexPool.Wait(aMemory->iLock);
	TRACE2(("MemoryObjectLock::Lock(0x%08x) acquired",aMemory));
	}

void MemoryObjectLock::Unlock(DMemoryObject* aMemory)
	{
	TRACE2(("MemoryObjectLock::Unlock(0x%08x)",aMemory));
	MemoryObjectMutexPool.Signal(aMemory->iLock);
	}

TBool MemoryObjectLock::IsHeld(DMemoryObject* aMemory)
	{
	return MemoryObjectMutexPool.IsHeld(aMemory->iLock);
	}



//
// DMemoryObject
//

DMemoryObject::DMemoryObject(DMemoryManager* aManager, TUint aFlags, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags)
	: iManager(aManager), iFlags(aFlags), iAttributes(Mmu::CanonicalMemoryAttributes(aAttributes)),
	  iSizeInPages(aSizeInPages)
	{
	__ASSERT_COMPILE(EMemoryAttributeMask<0x100); // make sure aAttributes fits into a TUint8

	TMemoryType type = (TMemoryType)(aAttributes&EMemoryAttributeTypeMask);
	iRamAllocFlags = type;
	if(aCreateFlags&EMemoryCreateNoWipe)
		iRamAllocFlags |= Mmu::EAllocNoWipe;
	else if(aCreateFlags&EMemoryCreateUseCustomWipeByte)
		{
		TUint8 wipeByte = (aCreateFlags>>EMemoryCreateWipeByteShift)&0xff;
		iRamAllocFlags |= wipeByte<<Mmu::EAllocWipeByteShift;
		iRamAllocFlags |= Mmu::EAllocUseCustomWipeByte;
		}

	if(aCreateFlags&EMemoryCreateDemandPaged)
		{
		iFlags |= EDemandPaged;
		iRamAllocFlags |= Mmu::EAllocNoPagerReclaim;
		}
	if(aCreateFlags&EMemoryCreateReserveAllResources)
		iFlags |= EReserveResources;
	if(aCreateFlags&EMemoryCreateDisallowPinning)
		iFlags |= EDenyPinning;
	if(aCreateFlags&EMemoryCreateReadOnly)
		iFlags |= EDenyWriteMappings;
	if(!(aCreateFlags&EMemoryCreateAllowExecution))
		iFlags |= EDenyExecuteMappings;
	}


TInt DMemoryObject::Construct()
	{
	TBool preAllocateMemory = iFlags&(EReserveResources|EDemandPaged);
	TInt r = iPages.Construct(iSizeInPages,preAllocateMemory);
	return r;
	}


DMemoryObject::~DMemoryObject()
	{
	TRACE(("DMemoryObject[0x%08x]::~DMemoryObject()",this));
	__NK_ASSERT_DEBUG(iMappings.IsEmpty());
	}


TBool DMemoryObject::CheckRegion(TUint aIndex, TUint aCount)
	{
	TUint end = aIndex+aCount;
	return end>=aIndex && end<=iSizeInPages;
	}


void DMemoryObject::ClipRegion(TUint& aIndex, TUint& aCount)
	{
	TUint end = aIndex+aCount;
	if(end<aIndex) // overflow?
		end = ~0u;
	if(end>iSizeInPages)
		end = iSizeInPages;
	if(aIndex>=end)
		aIndex = end;
	aCount = end-aIndex;
	}


void DMemoryObject::SetLock(DMutex* aLock)
	{
	__NK_ASSERT_DEBUG(!iLock);
	iLock = aLock;
	TRACE(("MemoryObject[0x%08x]::SetLock(0x%08x) \"%O\"",this,aLock,aLock));
	}


DMemoryMapping* DMemoryObject::CreateMapping(TUint, TUint)
	{
	return new DFineMapping();
	}


TInt DMemoryObject::MapPages(RPageArray::TIter aPages)
	{
	TRACE2(("DMemoryObject[0x%08x]::MapPages(?) index=0x%x count=0x%x",this,aPages.Index(),aPages.Count()));

	TUint offset = aPages.Index();
	TUint offsetEnd = aPages.IndexEnd();
	TInt r = KErrNone;

	iMappings.Lock();
	TMappingListIter iter;
	DMemoryMappingBase* mapping = iter.Start(iMappings);
	while(mapping)
		{
		if(mapping->IsPinned())
			{
			// pinned mappings don't change, so nothing to do...
			iMappings.Unlock();
			}
		else
			{
			// get region where pages overlap the mapping...
			TUint start = mapping->iStartIndex;
			TUint end = start+mapping->iSizeInPages;
			if(start<offset)
				start = offset;
			if(end>offsetEnd)
				end = offsetEnd;
			if(start>=end)
				{
				// the mapping doesn't contain the pages...
				iMappings.Unlock();
				}
			else
				{
				// map pages in the mapping...
				mapping->Open();
				TUint mapInstanceCount = mapping->MapInstanceCount();
				iMappings.Unlock();
				r = mapping->MapPages(aPages.Slice(start,end),mapInstanceCount);
				mapping->AsyncClose();
				if(r!=KErrNone)
					{
					iMappings.Lock();
					break;
					}
				}
			}
		iMappings.Lock();
		mapping = iter.Next();
		}
	iter.Finish();
	iMappings.Unlock();

	return r;
	}


void DMemoryObject::RemapPage(TPhysAddr& aPageArray, TUint aIndex, TBool aInvalidateTLB)
	{
	TRACE2(("DMemoryObject[0x%08x]::RemapPage(0x%x,%d,%d)",this,aPageArray,aIndex,aInvalidateTLB));

	iMappings.RemapPage(aPageArray, aIndex, aInvalidateTLB);

#ifdef COARSE_GRAINED_TLB_MAINTENANCE
	if (aInvalidateTLB)
		InvalidateTLB();
#endif
	}


void DMemoryObject::UnmapPages(RPageArray::TIter aPages, TBool aDecommitting)
	{
	TRACE2(("DMemoryObject[0x%08x]::UnmapPages(?,%d) index=0x%x count=0x%x",this,(bool)aDecommitting,aPages.Index(),aPages.Count()));

	TUint offset = aPages.Index();
	TUint offsetEnd = aPages.IndexEnd();
	if(offset==offsetEnd)
		return;

	iMappings.Lock();
	TMappingListIter iter;
	DMemoryMappingBase* mapping = iter.Start(iMappings);
	while(mapping)
		{
		// get region where pages overlap the mapping...
		TUint start = mapping->iStartIndex;
		TUint end = start+mapping->iSizeInPages;
		if(start<offset)
			start = offset;
		if(end>offsetEnd)
			end = offsetEnd;
		if(start>=end)
			{
			// the mapping doesn't contain the pages...
			iMappings.Unlock();
			}
		else
			{
			RPageArray::TIter pages = aPages.Slice(start,end);
			if(mapping->IsPinned())
				{
				// pinned mappings veto page unmapping...
				if(aDecommitting)
					__e32_atomic_ior_ord8(&mapping->Flags(), (TUint8)DMemoryMapping::EPageUnmapVetoed);
				iMappings.Unlock();
				TRACE2(("DFineMemoryMapping[0x%08x] veto UnmapPages, index=0x%x count=0x%x",mapping,pages.Index(),pages.Count()));
				pages.VetoUnmap();
				}
			else
				{
				// unmap pages in the mapping...
				mapping->Open();
				TUint mapInstanceCount = mapping->MapInstanceCount();
				iMappings.Unlock();
				mapping->UnmapPages(pages,mapInstanceCount);
				mapping->AsyncClose();
				}
			}
		iMappings.Lock();
		mapping = iter.Next();
		}
	iter.Finish();
	iMappings.Unlock();

#ifdef COARSE_GRAINED_TLB_MAINTENANCE
	InvalidateTLB();
#endif
	}


void DMemoryObject::RestrictPages(RPageArray::TIter aPages, TRestrictPagesType aRestriction)
	{
	TRACE2(("DMemoryObject[0x%08x]::RestrictPages(?,%d) index=0x%x count=0x%x",this,aRestriction,aPages.Index(),aPages.Count()));

	TUint offset = aPages.Index();
	TUint offsetEnd = aPages.IndexEnd();
	if(offset==offsetEnd)
		return;

	iMappings.Lock();
	TMappingListIter iter;
	DMemoryMappingBase* mapping = iter.Start(iMappings);
	while(mapping)
		{
		// get region where pages overlap the mapping...
		TUint start = mapping->iStartIndex;
		TUint end = start+mapping->iSizeInPages;
		if(start<offset)
			start = offset;
		if(end>offsetEnd)
			end = offsetEnd;
		if(start>=end)
			{
			// the mapping doesn't contain the pages...
			iMappings.Unlock();
			}
		else
			{
			RPageArray::TIter pages = aPages.Slice(start,end);
			if(mapping->IsPhysicalPinning() ||
				(!(aRestriction & ERestrictPagesForMovingFlag) && mapping->IsPinned()))
				{
				// Pinned mappings veto page restrictions except for page moving 
				// where only physically pinned mappings block page moving.
				iMappings.Unlock();
				TRACE2(("DFineMemoryMapping[0x%08x] veto RestrictPages, index=0x%x count=0x%x",mapping,pages.Index(),pages.Count()));
				pages.VetoRestrict(aRestriction & ERestrictPagesForMovingFlag);
				// Mappings lock required for iter.Finish() as iter will be removed from the mappings list.
				iMappings.Lock();
				break;
				}
			else
				{
				// pages not pinned so do they need restricting...
				if(aRestriction == ERestrictPagesForMovingFlag)
					{
					// nothing to do when just checking for pinned mappings for 
					// page moving purposes and not restricting to NA.
					iMappings.Unlock();
					}
				else
					{
					// restrict pages in the mapping...
					mapping->Open();
					TUint mapInstanceCount = mapping->MapInstanceCount();
					iMappings.Unlock();
					mapping->RestrictPagesNA(pages, mapInstanceCount);
					mapping->AsyncClose();
					}
				}
			}
		iMappings.Lock();
		mapping = iter.Next();
		}

	if(aRestriction & ERestrictPagesForMovingFlag)
		{// Clear the mappings addded flag so page moving can detect whether any 
		// new mappings have been added
		ClearMappingAddedFlag();
		}

	iter.Finish();
	iMappings.Unlock();

	#ifdef COARSE_GRAINED_TLB_MAINTENANCE
	// Writable memory objects will have been restricted no access so invalidate TLB.
	if (aRestriction != ERestrictPagesForMovingFlag)
		InvalidateTLB();
	#endif
	}


TInt DMemoryObject::CheckNewMapping(DMemoryMappingBase* aMapping)
	{
	if(iFlags&EDenyPinning && aMapping->IsPinned())
		return KErrAccessDenied;
	if(iFlags&EDenyMappings)
		return KErrAccessDenied;
	if(iFlags&EDenyWriteMappings && !aMapping->IsReadOnly())
		return KErrAccessDenied;
#ifdef MMU_SUPPORTS_EXECUTE_NEVER
	if((iFlags&EDenyExecuteMappings) && aMapping->IsExecutable())
		return KErrAccessDenied;
#endif
	return KErrNone;
	}


TInt DMemoryObject::AddMapping(DMemoryMappingBase* aMapping)
	{
	__NK_ASSERT_DEBUG(!aMapping->IsCoarse());

	// check mapping allowed...
	MmuLock::Lock();
	iMappings.Lock();

	TInt r = CheckNewMapping(aMapping);
	if(r == KErrNone)
		{
		Open();
		aMapping->LinkToMemory(this, iMappings);
		}

	iMappings.Unlock();
	MmuLock::Unlock();

	TRACE(("DMemoryObject[0x%08x]::AddMapping(0x%08x)  returns %d", this, aMapping, r));

	return r;
	}


void DMemoryObject::RemoveMapping(DMemoryMappingBase* aMapping)
	{
	aMapping->UnlinkFromMemory(iMappings);
	Close();
	}


TInt DMemoryObject::SetReadOnly()
	{
	TRACE(("DMemoryObject[0x%08x]::SetReadOnly()",this));
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(this));

	TInt r = KErrNone;
	iMappings.Lock();
	if (iFlags & EDenyWriteMappings)
		{// The object is already read only.
		iMappings.Unlock();
		return KErrNone;
		}		

	TMappingListIter iter;
	DMemoryMappingBase* mapping = iter.Start(iMappings);
	while(mapping)
		{
		if (!mapping->IsReadOnly())
			{
			r = KErrInUse;
			goto exit;
			}
		// This will flash iMappings.Lock to stop it being held too long.
		// This is safe as new mappings will be added to the end of the list so we
		// won't miss them.
		mapping = iter.Next();
		}
	// Block any writable mapping from being added to this memory object.
	// Use atomic operation as iMappings.Lock protects EDenyWriteMappings
	// but not the whole word.
	__e32_atomic_ior_ord8(&iFlags, (TUint8)EDenyWriteMappings);

exit:
	iter.Finish();
	iMappings.Unlock();
	return r;
	}


void DMemoryObject::DenyMappings()
	{
	TRACE(("DMemoryObject[0x%08x]::LockMappings()",this));
	MmuLock::Lock();
	// Use atomic operation as MmuLock protects EDenyMappings
	// but not the whole word.
	__e32_atomic_ior_ord8(&iFlags, (TUint8)EDenyMappings);
	MmuLock::Unlock();
	}


TInt DMemoryObject::PhysAddr(TUint aIndex, TUint aCount, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList)
	{
	TRACE2(("DMemoryObject[0x%08x]::PhysAddr(0x%x,0x%x,?,?)",this,aIndex,aCount));
	TInt r = iPages.PhysAddr(aIndex,aCount,aPhysicalAddress,aPhysicalPageList);
	TRACE2(("DMemoryObject[0x%08x]::PhysAddr(0x%x,0x%x,?,?) returns %d aPhysicalAddress=0x%08x",this,aIndex,aCount,r,aPhysicalAddress));
	return r;
	}


void DMemoryObject::BTraceCreate()
	{
	BTraceContext8(BTrace::EFlexibleMemModel,BTrace::EMemoryObjectCreate,this,iSizeInPages);
	}


TUint DMemoryObject::PagingManagerData(TUint aIndex)
	{
	TRACE2(("DMemoryObject[0x%08x]::PagingManagerData(0x%x)",this,aIndex));
	__NK_ASSERT_DEBUG(IsDemandPaged());
	TUint value = iPages.PagingManagerData(aIndex);
	TRACE2(("DMemoryObject[0x%08x]::PagingManagerData(0x%x) returns 0x%x",this,aIndex,value));
	return value;
	}


void DMemoryObject::SetPagingManagerData(TUint aIndex, TUint aValue)
	{
	TRACE(("DMemoryObject[0x%08x]::SetPagingManagerData(0x%x,0x%08x)",this,aIndex,aValue));
	__NK_ASSERT_DEBUG(IsDemandPaged());
	iPages.SetPagingManagerData(aIndex, aValue);
	__NK_ASSERT_DEBUG(iPages.PagingManagerData(aIndex)==aValue);
	}



//
// DCoarseMemory::DPageTables
//

DCoarseMemory::DPageTables::DPageTables(DCoarseMemory* aMemory, TInt aNumPts, TUint aPteType)
	: iMemory(aMemory), iPteType(aPteType), iPermanenceCount(0), iNumPageTables(aNumPts)
	{
	aMemory->Open();
	iBlankPte = Mmu::BlankPte(aMemory->Attributes(),aPteType);
	}


DCoarseMemory::DPageTables* DCoarseMemory::DPageTables::New(DCoarseMemory* aMemory, TUint aNumPages, TUint aPteType)
	{
	TRACE2(("DCoarseMemory::DPageTables::New(0x%08x,0x%x,0x%08x)",aMemory, aNumPages, aPteType));
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));
	__NK_ASSERT_DEBUG((aNumPages&(KChunkMask>>KPageShift))==0);
	TUint numPts = aNumPages>>(KChunkShift-KPageShift);
	DPageTables* self = (DPageTables*)Kern::AllocZ(sizeof(DPageTables)+(numPts-1)*sizeof(TPte*));
	if(self)
		{
		new (self) DPageTables(aMemory,numPts,aPteType);
		// Add this page tables object to the memory object before we update any 
		// page table entries. To ensure that if any of aMemory's pages with 
		// corresponding page table entries in self are moved during Construct(), 
		// DCoarseMemory::RemapPage() will be able to find the page table entries 
		// to update via iPageTables.
		__NK_ASSERT_DEBUG(!aMemory->iPageTables[aPteType]);
		aMemory->iPageTables[aPteType] = self;
		TInt r = self->Construct();
		if(r!=KErrNone)
			{
			aMemory->iPageTables[aPteType] = 0;
			self->Close();
			self = 0;
			}
		}
	TRACE2(("DCoarseMemory::DPageTables::New(0x%08x,0x%x,0x%08x) returns 0x%08x",aMemory, aNumPages, aPteType, self));
	return self;
	}


TInt DCoarseMemory::DPageTables::Construct()
	{
	if(iMemory->IsDemandPaged())
		{
		// do nothing, allow pages to be mapped on demand...
		return KErrNone;
		}

	RPageArray::TIter pageIter;
	iMemory->iPages.FindStart(0,iMemory->iSizeInPages,pageIter);

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
		r = MapPages(pageList);

		// done with pages...
		pageIter.FindRelease(n);

		if(r!=KErrNone)
			break;
		}

	iMemory->iPages.FindEnd(0,iMemory->iSizeInPages);

	return r;
	}


void DCoarseMemory::DPageTables::Close()
	{
	__NK_ASSERT_DEBUG(CheckCloseIsSafe());
	MmuLock::Lock();
	if (__e32_atomic_tas_ord32(&iReferenceCount, 1, -1, 0) != 1)
		{
		MmuLock::Unlock();
		return;
		}
	DCoarseMemory* memory = iMemory;
	if(memory)
		{
		iMemory->iPageTables[iPteType] = 0;
		iMemory = 0;
		}
	MmuLock::Unlock();
	if(memory)
		memory->Close();
	delete this;
	}


void DCoarseMemory::DPageTables::AsyncClose()
	{
	__ASSERT_CRITICAL
#ifdef _DEBUG
	NFastMutex* fm = NKern::HeldFastMutex();
	if(fm)
		{
		Kern::Printf("DCoarseMemory::DPageTables::[0x%08x]::AsyncClose() fast mutex violation %M",this,fm);
		__NK_ASSERT_DEBUG(0);
		}
#endif

	MmuLock::Lock();
	if (__e32_atomic_tas_ord32(&iReferenceCount, 1, -1, 0) != 1)
		{
		MmuLock::Unlock();
		return;
		}
	DCoarseMemory* memory = iMemory;
	if(memory)
		{
		iMemory->iPageTables[iPteType] = 0;
		iMemory = 0;
		}
	MmuLock::Unlock();
	if(memory)
		memory->AsyncClose();
	AsyncDelete();
	}


DCoarseMemory::DPageTables::~DPageTables()
	{
	TRACE2(("DCoarseMemory::DPageTables[0x%08x]::~DPageTables()",this));
	__NK_ASSERT_DEBUG(!iMemory);
	__NK_ASSERT_DEBUG(iMappings.IsEmpty());
	TUint i=0;
	while(i<iNumPageTables)
		{
		TPte* pt = iTables[i];
		if(pt)
			{
			iTables[i] = 0;
			::PageTables.Lock();
			::PageTables.Free(pt);
			::PageTables.Unlock();
			}
		++i;
		}
	}


TPte* DCoarseMemory::DPageTables::GetOrAllocatePageTable(TUint aChunkIndex)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	// get page table...
	TPte* pt = GetPageTable(aChunkIndex);
	if(!pt)
		pt = AllocatePageTable(aChunkIndex, iMemory->IsDemandPaged());

	return pt;
	}


TPte* DCoarseMemory::DPageTables::GetOrAllocatePageTable(TUint aChunkIndex, TPinArgs& aPinArgs)
	{
	__NK_ASSERT_DEBUG(aPinArgs.iPinnedPageTables);

	if(!aPinArgs.HaveSufficientPages(KNumPagesToPinOnePageTable))
		return 0;

	TPte* pinnedPt = 0;
	for(;;)
		{
		TPte* pt = GetOrAllocatePageTable(aChunkIndex);

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
					FreePageTable(aChunkIndex);
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
			{
			// Couldn't pin the page table...
			MmuLock::Unlock();
			// make sure we free any unneeded page table we allocated...
			FreePageTable(aChunkIndex);
			MmuLock::Lock();
			return 0;
			}
		pinnedPt = pt;
		}
	}


TPte* DCoarseMemory::DPageTables::AllocatePageTable(TUint aChunkIndex, TBool aDemandPaged, TBool aPermanent)
	{
	TRACE2(("DCoarseMemory::DPageTables[0x%08x]::AllocatePageTable(0x%08x,%d,%d)",this,aChunkIndex,aDemandPaged,aPermanent));

	TPte* pt;
	do
		{
		// acquire page table lock...
		MmuLock::Unlock();
		::PageTables.Lock();

		// see if we still need to allocate a page table...
		pt = iTables[aChunkIndex];
		if(!pt)
			{
			// allocate page table...
			pt = ::PageTables.Alloc(aDemandPaged);
			if(!pt)
				{
				// out of memory...
				::PageTables.Unlock();
				MmuLock::Lock();
				return 0;
				}
			AssignPageTable(aChunkIndex,pt);
			}

		// release page table lock...
		::PageTables.Unlock();
		MmuLock::Lock();

		// check again...
		pt = iTables[aChunkIndex];
		}
	while(!pt);

	// we have a page table...
	if(aPermanent)
		{
		__NK_ASSERT_ALWAYS(!aDemandPaged);
		SPageTableInfo* pti = SPageTableInfo::FromPtPtr(pt);
		pti->IncPermanenceCount();
		}
	return pt;
	}


void DCoarseMemory::DPageTables::AssignPageTable(TUint aChunkIndex, TPte* aPageTable)
	{
	__NK_ASSERT_DEBUG(PageTablesLockIsHeld());

	MmuLock::Lock();

	// get physical address of page table now, this can't change whilst we have the page table allocator mutex...
	TPhysAddr ptPhys = Mmu::PageTablePhysAddr(aPageTable);

	// update mappings with new page table...
	TUint offset = aChunkIndex<<(KChunkShift-KPageShift);
	iMappings.Lock();
	TMappingListIter iter;
	DMemoryMapping* mapping = (DMemoryMapping*)iter.Start(iMappings);
	TUint flash = 0;
	while(mapping)
		{
		TUint size = mapping->iSizeInPages;
		TUint start = offset-mapping->iStartIndex;
		if(start<size && !mapping->BeingDetached())
			{
			// page table is used by this mapping, so set PDE...
			TLinAddr linAddrAndOsAsid = mapping->LinAddrAndOsAsid()+start*KPageSize;
			TPde* pPde = Mmu::PageDirectoryEntry(linAddrAndOsAsid&KPageMask,linAddrAndOsAsid);
			TPde pde = ptPhys|mapping->BlankPde();
#ifdef	__USER_MEMORY_GUARDS_ENABLED__
			if (mapping->IsUserMapping())
				pde = PDE_IN_DOMAIN(pde, USER_MEMORY_DOMAIN);
#endif
			TRACE2(("!PDE %x=%x",pPde,pde));
			__NK_ASSERT_DEBUG(((*pPde^pde)&~KPdeMatchMask)==0 || *pPde==KPdeUnallocatedEntry);
			*pPde = pde;
			SinglePdeUpdated(pPde);

			++flash; // increase flash rate because we've done quite a bit more work
			}
		iMappings.Unlock();
		MmuLock::Flash(flash,KMaxMappingsInOneGo);
		iMappings.Lock();
		mapping = (DMemoryMapping*)iter.Next();
		}
	iter.Finish();
	iMappings.Unlock();

	// next, assign page table to us...
	// NOTE: Must happen before MmuLock is released after reaching the end of the mapping list
	// otherwise it would be possible for a new mapping to be added and mapped before we manage
	// to update iTables with the page table it should use.
	SPageTableInfo* pti = SPageTableInfo::FromPtPtr(aPageTable);
	pti->SetCoarse(iMemory,aChunkIndex,iPteType);
	__NK_ASSERT_DEBUG(!iTables[aChunkIndex]);
	iTables[aChunkIndex] = aPageTable; // new mappings can now see the page table

	MmuLock::Unlock();
	}


void DCoarseMemory::DPageTables::FreePageTable(TUint aChunkIndex)
	{
	TRACE2(("DCoarseMemory::DPageTables[0x%08x]::FreePageTable(0x%08x)",this,aChunkIndex));

	// acquire locks...
	::PageTables.Lock();
	MmuLock::Lock();

	// test if page table still needs freeing...
	TPte* pt = iTables[aChunkIndex];
	if(pt)
		{
		SPageTableInfo* pti = SPageTableInfo::FromPtPtr(pt);
		if(pti->PageCount()==0 && pti->PermanenceCount()==0)
			{
			// page table needs freeing...
			UnassignPageTable(aChunkIndex);
			MmuLock::Unlock();
			::PageTables.Free(pt);
			::PageTables.Unlock();
			return;
			}
		}

	// page table doesn't need freeing...
	MmuLock::Unlock();
	::PageTables.Unlock();
	return;
	}


void DCoarseMemory::StealPageTable(TUint aChunkIndex, TUint aPteType)
	{
	__NK_ASSERT_DEBUG(PageTablesLockIsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(iPageTables[aPteType]);
	iPageTables[aPteType]->StealPageTable(aChunkIndex);
	}


void DCoarseMemory::DPageTables::StealPageTable(TUint aChunkIndex)
	{
	__NK_ASSERT_DEBUG(PageTablesLockIsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
#ifdef _DEBUG
	TPte* pt = iTables[aChunkIndex];
	__NK_ASSERT_DEBUG(pt);
	SPageTableInfo* pti = SPageTableInfo::FromPtPtr(pt);
	__NK_ASSERT_DEBUG(pti->PageCount()==0);
	__NK_ASSERT_DEBUG(pti->PermanenceCount()==0);
#endif
	UnassignPageTable(aChunkIndex);
	}


void DCoarseMemory::DPageTables::UnassignPageTable(TUint aChunkIndex)
	{
	__NK_ASSERT_DEBUG(PageTablesLockIsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

#ifdef _DEBUG
	TPhysAddr ptPhys = Mmu::PageTablePhysAddr(iTables[aChunkIndex]);
#endif

	// zero page table pointer immediately so new mappings or memory commits will be force to
	// create a new one (which will block until we've finished here because it also needs the
	// PageTablesLock...
	iTables[aChunkIndex] = 0; 

	// remove page table from mappings...
	TUint offset = aChunkIndex<<(KChunkShift-KPageShift);
	iMappings.Lock();
	TMappingListIter iter;
	DMemoryMapping* mapping = (DMemoryMapping*)iter.Start(iMappings);
	TUint flash = 0;
	while(mapping)
		{
		__NK_ASSERT_DEBUG(iTables[aChunkIndex]==0); // can't have been recreated because we hold PageTablesLock
		TUint size = mapping->iSizeInPages;
		TUint start = offset-mapping->iStartIndex;
		if(start<size)
			{
			// page table is used by this mapping, so clear PDE...
			TLinAddr linAddrAndOsAsid = mapping->LinAddrAndOsAsid()+start*KPageSize;
			TPde* pPde = Mmu::PageDirectoryEntry(linAddrAndOsAsid&KPageMask,linAddrAndOsAsid);
			TPde pde = KPdeUnallocatedEntry;
			TRACE2(("!PDE %x=%x",pPde,pde));
			__NK_ASSERT_DEBUG(*pPde==pde || (*pPde&~KPageTableMask)==ptPhys);
			*pPde = pde;
			SinglePdeUpdated(pPde);

			++flash; // increase flash rate because we've done quite a bit more work
			}
		iMappings.Unlock();
		MmuLock::Flash(flash,KMaxMappingsInOneGo);
		iMappings.Lock();
		mapping = (DMemoryMapping*)iter.Next();
		}
	iter.Finish();

	iMappings.Unlock();
	}


TInt DCoarseMemory::DPageTables::AllocatePermanentPageTables()
	{
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(iMemory));
	__NK_ASSERT_ALWAYS(!iMemory->IsDemandPaged());

	if(iPermanenceCount++)
		{
		// page tables already marked permanent, so end...
		return KErrNone;
		}

	// allocate all page tables...
	MmuLock::Lock();
	TUint flash = 0;
	TUint i;
	for(i=0; i<iNumPageTables; ++i)
		{
		TPte* pt = iTables[i];
		if(pt)
			{
			// already have page table...
			SPageTableInfo* pti = SPageTableInfo::FromPtPtr(pt);
			pti->IncPermanenceCount();
			MmuLock::Flash(flash,KMaxPageInfoUpdatesInOneGo);
			}
		else
			{
			// allocate new page table...
			pt = AllocatePageTable(i,EFalse,ETrue);
			if(!pt)
				{
				MmuLock::Unlock();
				--iPermanenceCount;
				FreePermanentPageTables(0,i);
				return KErrNoMemory;
				}
			}
		}
	MmuLock::Unlock();

	return KErrNone;
	}


void DCoarseMemory::DPageTables::FreePermanentPageTables(TUint aChunkIndex, TUint aChunkCount)
	{
	MmuLock::Lock();

	TUint flash = 0;
	TUint i;
	for(i=aChunkIndex; i<aChunkIndex+aChunkCount; ++i)
		{
		TPte* pt = iTables[i];
		SPageTableInfo* pti = SPageTableInfo::FromPtPtr(pt);
		if(pti->DecPermanenceCount() || pti->PageCount())
			{
			// still in use...
			MmuLock::Flash(flash,KMaxPageInfoUpdatesInOneGo);
			}
		else
			{
			// page table no longer used for anything...
			MmuLock::Unlock();
			FreePageTable(i);
			MmuLock::Lock();
			}
		}

	MmuLock::Unlock();
	}


void DCoarseMemory::DPageTables::FreePermanentPageTables()
	{
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(iMemory));

	if(--iPermanenceCount)
		{
		// page tables still permanent, so end...
		return;
		}

	FreePermanentPageTables(0,iNumPageTables);
	}


TInt DCoarseMemory::DPageTables::AddMapping(DCoarseMapping* aMapping)
	{
	TRACE(("DCoarseMemory::DPageTables[0x%08x]::AddMapping(0x%08x)",this,aMapping));
	__NK_ASSERT_DEBUG(aMapping->IsCoarse());
	Open();
	MmuLock::Lock();
	iMappings.Lock();
	aMapping->LinkToMemory(iMemory,iMappings);
	iMappings.Unlock();
	MmuLock::Unlock();
	return KErrNone;
	}


void DCoarseMemory::DPageTables::RemoveMapping(DCoarseMapping* aMapping)
	{
	aMapping->UnlinkFromMemory(iMappings);
	Close();
	}


void DCoarseMemory::DPageTables::RemapPage(TPhysAddr& aPageArray, TUint aIndex, TBool aInvalidateTLB)
	{
	TUint pteIndex = aIndex & (KChunkMask>>KPageShift);

	// get address of page table...
	MmuLock::Lock();
	TUint i = aIndex>>(KChunkShift-KPageShift);
	TPte* pPte = GetPageTable(i);

	if (!pPte)
		{// This page has been unmapped so just return.
		MmuLock::Unlock();
		return;
		}

	// remap the page...
	pPte += pteIndex;
	Mmu::RemapPage(pPte, aPageArray, iBlankPte);

	MmuLock::Unlock();
	
	if (aInvalidateTLB)
		FlushTLB(aIndex, aIndex + 1);
	}


TInt DCoarseMemory::DPageTables::MapPages(RPageArray::TIter aPages)
	{
	__NK_ASSERT_DEBUG(aPages.Count());

	for(;;)
		{
		TUint pteIndex = aPages.Index()&(KChunkMask>>KPageShift);

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
		TUint i = aPages.Index()>>(KChunkShift-KPageShift);
		TPte* pPte = GetOrAllocatePageTable(i);

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
			FreePageTable(i);

		// move on...
		aPages.Skip(n);
		}

	return KErrNone;
	}


void DCoarseMemory::DPageTables::UnmapPages(RPageArray::TIter aPages, TBool aDecommitting)
	{
	__NK_ASSERT_DEBUG(aPages.Count());

	TUint startIndex = aPages.Index();

	for(;;)
		{
		TUint pteIndex = aPages.Index()&(KChunkMask>>KPageShift);

		// calculate max number of pages to do...
		TUint n = (KChunkSize>>KPageShift)-pteIndex; // pages left in page table
		if(n>KMaxPagesInOneGo)
			n = KMaxPagesInOneGo;

		// get some pages...
		TPhysAddr* pages;
		n = aPages.Pages(pages,n);
		if(!n)
			break;

		// get address of PTE for pages...
		MmuLock::Lock();
		TUint i = aPages.Index()>>(KChunkShift-KPageShift);
		TPte* pPte = iTables[i];
		if(pPte)
			{
			// unmap some pages...
			pPte += pteIndex;
			TBool keepPt = Mmu::UnmapPages(pPte,n,pages);
			MmuLock::Unlock();

			// free page table if no longer needed...
			if(!keepPt)
				FreePageTable(i);
			}
		else
			{
			// no page table found...
			MmuLock::Unlock();
			}

		// move on...
		aPages.Skip(n);
		}

	FlushTLB(startIndex,aPages.IndexEnd());
	}


void DCoarseMemory::DPageTables::RestrictPagesNA(RPageArray::TIter aPages)
	{
	__NK_ASSERT_DEBUG(aPages.Count());

	TUint startIndex = aPages.Index();

	for(;;)
		{
		TUint pteIndex = aPages.Index()&(KChunkMask>>KPageShift);

		// calculate max number of pages to do...
		TUint n = (KChunkSize>>KPageShift)-pteIndex; // pages left in page table
		if(n>KMaxPagesInOneGo)
			n = KMaxPagesInOneGo;

		// get some pages...
		TPhysAddr* pages;
		n = aPages.Pages(pages,n);
		if(!n)
			break;

		// get address of PTE for pages...
		MmuLock::Lock();
		TUint i = aPages.Index()>>(KChunkShift-KPageShift);
		TPte* pPte = iTables[i];
		if(pPte)
			{
			// restrict some pages...
			pPte += pteIndex;
			Mmu::RestrictPagesNA(pPte,n,pages);
			}
		MmuLock::Unlock();

		// move on...
		aPages.Skip(n);
		}

	FlushTLB(startIndex,aPages.IndexEnd());
	}


TInt DCoarseMemory::DPageTables::PageIn(RPageArray::TIter aPages, TPinArgs& aPinArgs, 
										DMemoryMappingBase* aMapping, TUint aMapInstanceCount)
	{
	__NK_ASSERT_DEBUG(aPages.Count());

	TBool pinPageTable = aPinArgs.iPinnedPageTables!=0; // check if we need to pin the first page table
	for(;;)
		{
		TUint pteIndex = aPages.Index()&(KChunkMask>>KPageShift);
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
		TUint i = aPages.Index()>>(KChunkShift-KPageShift);
		TPte* pPte;
		if(pinPageTable)
			pPte = GetOrAllocatePageTable(i,aPinArgs);
		else
			pPte = GetOrAllocatePageTable(i);

		// check for OOM...
		if(!pPte)
			{
			MmuLock::Unlock();
			return KErrNoMemory;
			}

		if (aMapInstanceCount != aMapping->MapInstanceCount())
			{// The mapping that took the page fault has been reused.
			MmuLock::Unlock();
			FreePageTable(i);	// This will only free if this is the only pt referencer.
			return KErrNotFound;
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
			FreePageTable(i);

		// move on...
		aPages.Skip(n);
		pinPageTable = false;
		}

	return KErrNone;
	}


TBool DCoarseMemory::DPageTables::MovingPageIn(TPhysAddr& aPageArrayPtr, TUint aIndex)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	TUint pteIndex = aIndex & (KChunkMask >> KPageShift);

	// get address of page table...
	TUint i = aIndex >> (KChunkShift - KPageShift);
	TPte* pPte = GetPageTable(i);

	// Check the page is still mapped..
	if (!pPte)
		return EFalse;

	// map the page...
	pPte += pteIndex;
	Mmu::RemapPage(pPte, aPageArrayPtr, iBlankPte);
	return ETrue;
	}


void DCoarseMemory::DPageTables::FlushTLB(TUint aStartIndex, TUint aEndIndex)
	{
#ifndef COARSE_GRAINED_TLB_MAINTENANCE
	iMappings.Lock();
	TMappingListIter iter;
	DMemoryMapping* mapping = (DMemoryMapping*)iter.Start(iMappings);
	while(mapping)
		{
		// get region which overlaps the mapping...
		TUint start = mapping->iStartIndex;
		TUint end = start+mapping->iSizeInPages;
		if(start<aStartIndex)
			start = aStartIndex;
		if(end>aEndIndex)
			end = aEndIndex;
		if(start>=end)
			{
			// the mapping doesn't contain the pages...
			iMappings.Unlock();
			}
		else
			{
			// flush TLB for pages in the mapping...
			TUint size = end-start;
			start -= mapping->iStartIndex;
			TLinAddr addr = mapping->LinAddrAndOsAsid()+start*KPageSize;
			TLinAddr endAddr = addr+size*KPageSize;
			iMappings.Unlock();
			do
				{
				InvalidateTLBForPage(addr);
				}
			while((addr+=KPageSize)<endAddr);
			}
		iMappings.Lock();
		mapping = (DMemoryMapping*)iter.Next();
		}
	iter.Finish();
	iMappings.Unlock();
#endif
	}



//
// DCoarseMemory
//

DCoarseMemory::DCoarseMemory(DMemoryManager* aManager, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags)
	: DMemoryObject(aManager,ECoarseObject,aSizeInPages,aAttributes,aCreateFlags)
	{
	}


DCoarseMemory* DCoarseMemory::New(DMemoryManager* aManager, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags)
	{
	DCoarseMemory* self = new DCoarseMemory(aManager, aSizeInPages, aAttributes, aCreateFlags);
	if(self)
		{
		if(self->Construct()==KErrNone)
			return self;
		self->Close();
		}
	return 0;
	}


DCoarseMemory::~DCoarseMemory()
	{
	TRACE2(("DCoarseMemory[0x%08x]::~DCoarseMemory()",this));
#ifdef _DEBUG
	for(TUint i=0; i<ENumPteTypes; i++)
		{
		__NK_ASSERT_DEBUG(!iPageTables[i]);
		}
#endif
	}


DMemoryMapping* DCoarseMemory::CreateMapping(TUint aIndex, TUint aCount)
	{
	if (((aIndex|aCount)&(KChunkMask>>KPageShift))==0)
		return new DCoarseMapping();
	else
		return new DFineMapping();
	}


TInt DCoarseMemory::MapPages(RPageArray::TIter aPages)
	{
	TRACE2(("DCoarseMemory[0x%08x]::MapPages(?) index=0x%x count=0x%x",this,aPages.Index(),aPages.Count()));

	// map pages in all page tables for coarse mapping...
	MmuLock::Lock();
	TUint pteType = 0;
	do
		{
		DPageTables* tables = iPageTables[pteType];
		if(tables)
			{
			tables->Open();
			MmuLock::Unlock();
			TInt r = tables->MapPages(aPages);
			tables->AsyncClose();
			if(r!=KErrNone)
				return r;
			MmuLock::Lock();
			}
		}
	while(++pteType<ENumPteTypes);
	MmuLock::Unlock();

	// map page in all fine mappings...
	return DMemoryObject::MapPages(aPages);
	}


void DCoarseMemory::RemapPage(TPhysAddr& aPageArray, TUint aIndex, TBool aInvalidateTLB)
	{
	TRACE2(("DCoarseMemory[0x%08x]::RemapPage() index=0x%x",this, aIndex));

	// remap pages in all page tables for coarse mapping...
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
			tables->AsyncClose();
			MmuLock::Lock();
			}
		}
	while(++pteType<ENumPteTypes);
	MmuLock::Unlock();

	// remap page in all fine mappings...
	DMemoryObject::RemapPage(aPageArray, aIndex, aInvalidateTLB);
	}


void DCoarseMemory::UnmapPages(RPageArray::TIter aPages, TBool aDecommitting)
	{
	TRACE2(("DCoarseMemory[0x%08x]::UnmapPages(?,%d) index=0x%x count=0x%x",this,(bool)aDecommitting,aPages.Index(),aPages.Count()));

	if(!aPages.Count())
		return;

	// unmap pages from all page tables for coarse mapping...
	MmuLock::Lock();
	TUint pteType = 0;
	do
		{
		DPageTables* tables = iPageTables[pteType];
		if(tables)
			{
			tables->Open();
			MmuLock::Unlock();
			tables->UnmapPages(aPages,aDecommitting);
			tables->AsyncClose();
			MmuLock::Lock();
			}
		}
	while(++pteType<ENumPteTypes);
	MmuLock::Unlock();

	// unmap pages from all fine mappings...
	DMemoryObject::UnmapPages(aPages,aDecommitting);
	}


void DCoarseMemory::RestrictPages(RPageArray::TIter aPages, TRestrictPagesType aRestriction)
	{
	TRACE2(("DCoarseMemory[0x%08x]::RestrictPages(?,%d) index=0x%x count=0x%x",this,aRestriction,aPages.Index(),aPages.Count()));
	__ASSERT_COMPILE(ERestrictPagesForMovingFlag != ERestrictPagesNoAccessForMoving);

	if(!aPages.Count())
		return;

	if (aRestriction != ERestrictPagesForMovingFlag)
		{// restrict pages in all the page tables for the coarse mapping...
		MmuLock::Lock();
		TUint pteType = 0;
		do
			{
			DPageTables* tables = iPageTables[pteType];
			if(tables)
				{
				tables->Open();
				MmuLock::Unlock();
				tables->RestrictPagesNA(aPages);
				tables->AsyncClose();
				MmuLock::Lock();
				}
			}
		while(++pteType<ENumPteTypes);
		MmuLock::Unlock();
		}

	// restrict pages in all fine mappings, will also check for pinned mappings...
	DMemoryObject::RestrictPages(aPages,aRestriction);
	}


TPte* DCoarseMemory::GetPageTable(TUint aPteType, TUint aChunkIndex)
	{
	__NK_ASSERT_DEBUG(aChunkIndex < (iSizeInPages >> KPagesInPDEShift));
	return iPageTables[aPteType]->GetPageTable(aChunkIndex);
	}


TInt DCoarseMemory::PageIn(DCoarseMapping* aMapping, RPageArray::TIter aPages, TPinArgs& aPinArgs, TUint aMapInstanceCount)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	DPageTables* tables = iPageTables[aMapping->PteType()];
	tables->Open();

	MmuLock::Unlock();

#ifndef COARSE_GRAINED_TLB_MAINTENANCE
	TLinAddr startAddr = aMapping->Base()+(aPages.Index()-aMapping->iStartIndex)*KPageSize;
	TLinAddr endAddr = startAddr+aPages.Count()*KPageSize;
#endif

	TInt r = tables->PageIn(aPages, aPinArgs, aMapping, aMapInstanceCount);

	// clean TLB...
#ifdef COARSE_GRAINED_TLB_MAINTENANCE
	InvalidateTLBForAsid(aMapping->OsAsid());
#else
	TLinAddr addr = startAddr+aMapping->OsAsid();
	do InvalidateTLBForPage(addr);
	while((addr+=KPageSize)<endAddr);
#endif

	tables->AsyncClose();

	return r;
	}


TBool DCoarseMemory::MovingPageIn(DCoarseMapping* aMapping, TPhysAddr& aPageArrayPtr, TUint aIndex)
	{
	DCoarseMemory::DPageTables* tables = iPageTables[aMapping->PteType()];
	return tables->MovingPageIn(aPageArrayPtr, aIndex);
	}


TPte* DCoarseMemory::FindPageTable(DCoarseMapping* aMapping, TLinAddr aLinAddr, TUint aMemoryIndex)
	{
	DCoarseMemory::DPageTables* tables = iPageTables[aMapping->PteType()];

	// get address of page table...
	TUint i = aMemoryIndex >> (KChunkShift - KPageShift);	
	return tables->GetPageTable(i);
	}


TInt DCoarseMemory::ClaimInitialPages(TLinAddr aBase, TUint aSize, TMappingPermissions aPermissions, TBool aAllowGaps, TBool aAllowNonRamPages)
	{
	TRACE(("DCoarseMemory[0x%08x]::ClaimInitialPages(0x%08x,0x%08x,0x%08x,%d,%d)",this,aBase,aSize,aPermissions,aAllowGaps,aAllowNonRamPages));

	// validate arguments...
	if(aBase&KChunkMask || aBase<KGlobalMemoryBase)
		return KErrArgument;
	if(aSize&KPageMask || aSize>iSizeInPages*KPageSize)
		return KErrArgument;

	// get DPageTables object...
	TUint pteType = Mmu::PteType(aPermissions,true);
	MemoryObjectLock::Lock(this);
	DPageTables* tables = GetOrAllocatePageTables(pteType);
	MemoryObjectLock::Unlock(this);
	__NK_ASSERT_DEBUG(tables);

	// check and allocate page array entries...
	RPageArray::TIter pageIter;
	TInt r = iPages.AddStart(0,aSize>>KPageShift,pageIter);
	__NK_ASSERT_ALWAYS(r==KErrNone);

	// hold MmuLock for long time, shouldn't matter as this is only done during boot
	::PageTables.Lock();
	MmuLock::Lock();

	TPte blankPte = tables->iBlankPte;
	TPte** pPt = tables->iTables;
	TPde* pPde = Mmu::PageDirectoryEntry(KKernelOsAsid,aBase);
	TUint offset = 0;
	TUint size = aSize;
	while(size)
		{
		TPde pde = *pPde;
		TRACE(("DCoarseMemory::ClaimInitialPages: %08x: 0x%08x",aBase+offset,pde));
		
		TPte* pPte = NULL;
		SPageTableInfo* pti = NULL;

		if (Mmu::PdeMapsSection(pde))
			{
			TPhysAddr sectionBase = Mmu::SectionBaseFromPde(pde);
			TRACE(("  chunk is section mapped, base at %08x", sectionBase));
			__NK_ASSERT_DEBUG(sectionBase != KPhysAddrInvalid);

			TPde pde = sectionBase | Mmu::BlankSectionPde(Attributes(),pteType);
			__NK_ASSERT_DEBUG(((*pPde^pde)&~KPdeMatchMask)==0);			
			*pPde = pde;
			SinglePdeUpdated(pPde);
			InvalidateTLB();

			// We allocate and populate a page table for the section even though it won't be mapped
			// initially because the presense of the page table is used to check whether RAM is
			// mapped in a chunk, and because it makes it possible to break the section mapping
			// without allocating memory.  This may change in the future.

			// Note these page table are always unpaged here regardless of paged bit in iFlags
			// (e.g. ROM object is marked as paged despite initial pages being unpaged)
			pPte = tables->AllocatePageTable(offset >> KChunkShift, EFalse, EFalse);
			if (!pPte)
				{
				MmuLock::Unlock();
				return KErrNoMemory;
				}
			pti = SPageTableInfo::FromPtPtr(pPte);
			}
		else if (Mmu::PdeMapsPageTable(pde))
			{
			pPte = Mmu::PageTableFromPde(*pPde);
			TRACE(("  page table found at %08x", pPte));
			__NK_ASSERT_DEBUG(pPte);
			pti = SPageTableInfo::FromPtPtr(pPte);
			pti->SetCoarse(this,offset>>KChunkShift,pteType);
			}
		
		*pPt++ = pPte;
		++pPde;
		
		TUint numPages = 0;
		do
			{
			TPhysAddr pagePhys = Mmu::LinearToPhysical(aBase+offset);
			TPte pte;
			if(pagePhys==KPhysAddrInvalid)
				{
				if(size)
					{
					__NK_ASSERT_ALWAYS(aAllowGaps); // we have a gap, check this is allowed
					pageIter.Skip(1);
					}

				pte = KPteUnallocatedEntry;
				}
			else
				{
				__NK_ASSERT_ALWAYS(size); // pages can't be mapped above aSize

				pageIter.Add(1,&pagePhys);

				SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pagePhys);
				__NK_ASSERT_ALWAYS(pi || aAllowNonRamPages);
				if(pi)
					{
					__NK_ASSERT_ALWAYS(pi->Type()==SPageInfo::EFixed);
					pi->SetManaged(this,offset>>KPageShift,PageInfoFlags());
					}

				++numPages;
				pte = pagePhys|blankPte;
				}

			if(pPte)
				{
				TRACE2(("!PTE %x=%x (was %x)",pPte,pte,*pPte));
				__NK_ASSERT_DEBUG(((*pPte^pte)&~KPteMatchMask)==0 || *pPte==KPteUnallocatedEntry);
				*pPte = pte;
				CacheMaintenance::SinglePteUpdated((TLinAddr)pPte);
				++pPte;
				}

			offset += KPageSize;
			if(size)
				size -= KPageSize;
			}
		while(offset&(KChunkMask&~KPageMask));

		if(pti)
			{
			pti->IncPageCount(numPages);
			TRACE2(("pt %x page count=%d",TLinAddr(pPte)-KPageTableSize,numPages));
			__NK_ASSERT_DEBUG(pti->CheckPageCount());
			}
		}

	InvalidateTLBForAsid(KKernelOsAsid);

	MmuLock::Unlock();
	::PageTables.Unlock();

	// release page array entries...
	iPages.AddEnd(0,aSize>>KPageShift);

	return KErrNone;
	}


DCoarseMemory::DPageTables* DCoarseMemory::GetOrAllocatePageTables(TUint aPteType)
	{
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(this));
	
	MmuLock::Lock();
	DPageTables* tables = iPageTables[aPteType];
	if(tables)
		tables->Open();
	MmuLock::Unlock();

	if(!tables)
		{
		// allocate a new one if required...
		tables = DPageTables::New(this, iSizeInPages, aPteType);
		}		

	return tables;
	}


TInt DCoarseMemory::AddMapping(DMemoryMappingBase* aMapping)
	{
	if(!aMapping->IsCoarse())
		{
		// not coarse mapping...
		return DMemoryObject::AddMapping(aMapping);
		}

	__NK_ASSERT_DEBUG(aMapping->IsPinned()==false); // coarse mappings can't pin

	// Check mapping allowed.  Must hold memory object lock to prevent changes 
	// to object's restrictions.
	MemoryObjectLock::Lock(this);
	TInt r = CheckNewMapping(aMapping);
	if(r!=KErrNone)
		{
		MemoryObjectLock::Unlock(this);
		return r;
		}

	// get DPageTable for mapping...
	DPageTables* tables = GetOrAllocatePageTables(aMapping->PteType());
	
	// Safe to release here as no restrictions to this type of mapping can be added as 
	// we now have an iPageTables entry for this type of mapping.
	MemoryObjectLock::Unlock(this);	
	if(!tables)
		return KErrNoMemory;

	// add mapping to DPageTable...
	r = tables->AddMapping((DCoarseMapping*)aMapping);
	if(r==KErrNone)
		{
		// allocate permanent page tables if required...
		if(aMapping->Flags()&DMemoryMapping::EPermanentPageTables)
			{
			MemoryObjectLock::Lock(this);
			r = tables->AllocatePermanentPageTables();
			MemoryObjectLock::Unlock(this);

			if(r==KErrNone)
				__e32_atomic_ior_ord8(&aMapping->Flags(), (TUint8)DMemoryMapping::EPageTablesAllocated);
			else
				tables->RemoveMapping((DCoarseMapping*)aMapping);
			}
		}

	tables->Close();

	return r;
	}


void DCoarseMemory::RemoveMapping(DMemoryMappingBase* aMapping)
	{
	if(!aMapping->IsCoarse())
		{
		// not coarse mapping...
		DMemoryObject::RemoveMapping(aMapping);
		return;
		}

	// need a temporary reference on self because we may be removing the last mapping
	// which will delete this...
	Open();

	// get DPageTable the mapping is attached to...
	DPageTables* tables = iPageTables[aMapping->PteType()];
	__NK_ASSERT_DEBUG(tables); // must exist because aMapping has a reference on it

	// free permanent page tables if required...
	if(aMapping->Flags()&DMemoryMapping::EPageTablesAllocated)
		{
		MemoryObjectLock::Lock(this);
		tables->FreePermanentPageTables();
		MemoryObjectLock::Unlock(this);
		}

	// remove mapping from page tables object...
	tables->RemoveMapping((DCoarseMapping*)aMapping);

	Close(); // may delete this memory object
	}


TInt DCoarseMemory::SetReadOnly()
	{
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(this));
	
	// Search for writable iPageTable entries.
	// We hold the MemoryObjectLock so iPageTable entries can't be added or removed.
	MmuLock::Lock();
	TUint pteType = 0;
	do
		{
		if((pteType & EPteTypeWritable) && iPageTables[pteType])
			{
			MmuLock::Unlock();
			return KErrInUse;
			}
		}
	while(++pteType < ENumPteTypes);
	MmuLock::Unlock();

	// unmap pages from all fine mappings...
	return DMemoryObject::SetReadOnly();
	}


//
// DFineMemory
//

DFineMemory::DFineMemory(DMemoryManager* aManager, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags)
	: DMemoryObject(aManager,0,aSizeInPages,aAttributes,aCreateFlags)
	{
	}


DFineMemory* DFineMemory::New(DMemoryManager* aManager, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags)
	{
	DFineMemory* self = new DFineMemory(aManager,aSizeInPages,aAttributes,aCreateFlags);
	if(self)
		{
		if(self->Construct()==KErrNone)
			return self;
		self->Close();
		}
	return 0;
	}


DFineMemory::~DFineMemory()
	{
	TRACE2(("DFineMemory[0x%08x]::~DFineMemory",this));
	}


TInt DFineMemory::ClaimInitialPages(TLinAddr aBase, TUint aSize, TMappingPermissions aPermissions, TBool aAllowGaps, TBool aAllowNonRamPages)
	{
	TRACE(("DFineMemory[0x%08x]::ClaimInitialPages(0x%08x,0x%08x,0x%08x,%d,%d)",this,aBase,aSize,aPermissions,aAllowGaps,aAllowNonRamPages));
	(void)aPermissions;

	// validate arguments...
	if(aBase&KPageMask || aBase<KGlobalMemoryBase)
		return KErrArgument;
	if(aSize&KPageMask || aSize>iSizeInPages*KPageSize)
		return KErrArgument;

#ifdef _DEBUG
	// calculate 'blankPte', the correct PTE value for pages in this memory object...
	TUint pteType = Mmu::PteType(aPermissions,true);
	TPte blankPte = Mmu::BlankPte(Attributes(),pteType);
#endif

	// get page table...
	TPde* pPde = Mmu::PageDirectoryEntry(KKernelOsAsid,aBase);
	TPte* pPte = Mmu::PageTableFromPde(*pPde);
	if(!pPte)
		return KErrNone; // no pages mapped

	// check and allocate page array entries...
	RPageArray::TIter pageIter;
	TInt r = iPages.AddStart(0,aSize>>KPageShift,pageIter);
	__NK_ASSERT_ALWAYS(r==KErrNone);

	// hold MmuLock for long time, shouldn't matter as this is only done during boot
	MmuLock::Lock();

	// setup page table for fine mappings...
	SPageTableInfo* pti = SPageTableInfo::FromPtPtr(pPte);
	__NK_ASSERT_DEBUG(pti->CheckPageCount());
	TBool pageTableOk = pti->ClaimFine(aBase&~KChunkMask,KKernelOsAsid);
	__NK_ASSERT_ALWAYS(pageTableOk);
	TRACE(("DFineMemory::ClaimInitialPages page table = 0x%08x",pPte));

	TUint pteIndex = (aBase>>KPageShift)&(KChunkMask>>KPageShift);
	TUint pageIndex = 0;
	TUint size = aSize;
	while(pageIndex<iSizeInPages)
		{
		TPhysAddr pagePhys = Mmu::PtePhysAddr(pPte[pteIndex],pteIndex);
		if(pagePhys==KPhysAddrInvalid)
			{
			if(size)
				{
				__NK_ASSERT_ALWAYS(aAllowGaps); // we have a gap, check this is allowed
				pageIter.Skip(1);
				}

			// check PTE is correct...
			__NK_ASSERT_DEBUG(pPte[pteIndex]==KPteUnallocatedEntry);
			}
		else
			{
			__NK_ASSERT_ALWAYS(size); // pages can't be mapped above aSize

			pageIter.Add(1,&pagePhys);

			SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pagePhys);

			if(!pi)
				__NK_ASSERT_ALWAYS(aAllowNonRamPages);
			else
				{
				__NK_ASSERT_ALWAYS(pi->Type()==SPageInfo::EFixed);
				pi->SetManaged(this,pageIndex,PageInfoFlags());
				}

#ifdef _DEBUG
			// check PTE is correct...
			TPte pte = pagePhys|blankPte;
			__NK_ASSERT_DEBUG(((pPte[pteIndex]^pte)&~KPteMatchMask)==0);
#endif
			}

		// move on to next page...
		++pteIndex;
		__NK_ASSERT_ALWAYS(pteIndex<(KChunkSize>>KPageShift));
		++pageIndex;
		if(size)
			size -= KPageSize;
		}

	MmuLock::Unlock();

	// release page array entries...
	iPages.AddEnd(0,aSize>>KPageShift);

	return KErrNone;
	}



