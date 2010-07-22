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
#include "mpager.h"

#include "mmanager.h"
#include "mmapping.h"
#include "mobject.h"

#include "mptalloc.h"
#include "cache_maintenance.inl"

/**
@class PageTableAllocator
@details

NOTES

Page tables are mapped into a sparse array in the virtual address range
#KPageTableBase..#KPageTableEnd. For each present page table there is a
corresponding #SPageTableInfo object mapped from #KPageTableInfoBase upwards.

Page tables for demand paged content are kept separate from other page tables,
this enables the memory for these to be freed when the page tables no longer map
any memory i.e. when it has all been paged-out. Pages with these 'paged' page
tables are stored in the demand paging live list, so it participates in the page
aging process.

The 'unpaged' page tables are allocated from the bottom of the array upwards,
via TPtPageAllocator::iLowerAllocator; the 'paged' page tables are allocated
from the top of the array downwards, via TPtPageAllocator::iUpperAllocator.
These two regions are prevented from overlapping, or from coming close enough
together so that the #SPageTableInfo struct for paged and unpaged page tables
lie in the same page. This means that the SPageTableInfo memory for paged page
tables can be discarded when it's page tables are discarded.

Memory for page tables and page table info objects is managed by
#ThePageTableMemoryManager. When allocating memory for demand paged use, this
uses memory from #ThePager which will reclaim paged memory if necessary.
Providing the live list always has #DPager::iMinYoungPages, this guarantees that
handling page faults can never fail by running out of memory.
*/


PageTableAllocator PageTables;



TBool PageTablesLockIsHeld()
	{
	return ::PageTables.LockIsHeld();
	}


/**
Minimum number of page tables to keep in reserve.
*/
const TUint KNumReservedPageTables = 0; // none needed - page tables for mapping page tables and infos are permanently allocated


/**
Manager for the memory object used to store all the MMU page tables.
*/
class DPageTableMemoryManager : public DMemoryManager
	{
public:
	/**
	Not implemented - page table memory is never destroyed.
	*/
	virtual void Destruct(DMemoryObject* aMemory)
		{}

	virtual TInt StealPage(DMemoryObject* aMemory, SPageInfo* aPageInfo)
		{ return PageTables.StealPage(aPageInfo); }

	/**
	Does nothing, returns KErrNone.
	The RAM containing page tables does not need access restrictions applied for demand paging
	purposes. Page table life-time is implicitly managed through the pages it maps.
	*/
	virtual TInt RestrictPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TRestrictPagesType aRestriction)
		{ return KErrNone; }

	/**
	Does nothing, returns KErrNone.
	The contents of page tables never need saving as their contents are dynamically generated.
	*/
	virtual TInt CleanPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TPhysAddr*& aPageArrayEntry)
		{ return KErrNone; }

	/**
	Not implemented, returns KErrNotSupported.
	*/
	virtual TInt Pin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs)
		{ return KErrNotSupported; }

	/**
	Not implemented.
	*/
	virtual void Unpin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs)
		{ }


	virtual TInt MovePage(	DMemoryObject* aMemory, SPageInfo* aOldPageInfo, 
							TPhysAddr& aNewPage, TUint aBlockZoneId, TBool aBlockRest);

	virtual TInt MoveAndAllocPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TZonePageType aType);
public:
	/**
	Allocate a page of RAM for storing page tables in.

	@param aMemory		A memory object associated with this manager.
	@param aIndex		Page index, within the memory, to allocate the page at.
	@param aDemandPaged	True if the memory is to be used for page tables mapping
						demand paged content.

	@return KErrNone if successful, otherwise one of the system wide error codes.
	*/
	TInt Alloc(DMemoryObject* aMemory, TUint aIndex, TBool aDemandPaged);

	/**
	Allocate a page of RAM being used for storing page tables in.

	@param aMemory		A memory object associated with this manager.
	@param aIndex		Page index, within the memory, to free the page from.
	@param aDemandPaged	True if the memory is being used for page tables mapping
						demand paged content.

	@return KErrNone if successful, otherwise one of the system wide error codes.
	*/
	TInt Free(DMemoryObject* aMemory, TUint aIndex, TBool aDemandPaged);
	};

/**
The single instance of the #DPageTableMemoryManager class.
*/
DPageTableMemoryManager ThePageTableMemoryManager;


TInt DPageTableMemoryManager::Alloc(DMemoryObject* aMemory, TUint aIndex, TBool aDemandPaged)
	{
	TRACE2(("DPageTableMemoryManager::Alloc(0x%08x,0x%x,%d)",aMemory, aIndex, aDemandPaged));
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	// allocate page array entry...
	RPageArray::TIter pageList;
	TPhysAddr* p = aMemory->iPages.AddPageStart(aIndex,pageList);
	if(!p)
		return KErrNoMemory;

	// allocate RAM...
	RamAllocLock::Lock();
	TPhysAddr pagePhys;
	TInt r;
	if(aDemandPaged)
		{
		r = ThePager.PageInAllocPages(&pagePhys,1,aMemory->RamAllocFlags());
		__NK_ASSERT_DEBUG(r!=KErrNoMemory);
		}
	else
		{// Allocate fixed paged as page tables aren't movable.
		r = TheMmu.AllocRam(&pagePhys, 1, aMemory->RamAllocFlags(), EPageFixed);

#ifdef BTRACE_KERNEL_MEMORY
		if (r == KErrNone)
			{
			BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryMiscAlloc, KPageSize);
			++Epoc::KernelMiscPages;
			}
#endif
		}
	RamAllocLock::Unlock();

	TUint usedNew = 0;
	if(r==KErrNone)
		{
		// add RAM to page array...
		MmuLock::Lock();
		if(aDemandPaged)
			ThePager.Event(DPager::EEventPagePageTableAlloc,SPageInfo::FromPhysAddr(pagePhys));
		SPageInfo* pi = SPageInfo::FromPhysAddr(pagePhys);
		pi->SetManaged(aMemory,aIndex,aMemory->PageInfoFlags());
		RPageArray::AddPage(p,pagePhys);
		MmuLock::Unlock();
		usedNew = 1;

		// map page...
		r = aMemory->MapPages(pageList);
		}

	// release page array entry...
	aMemory->iPages.AddPageEnd(aIndex,usedNew);

	// revert if error...
	if(r!=KErrNone)
		Free(aMemory,aIndex,aDemandPaged);

	return r;
	}


TInt DPageTableMemoryManager::Free(DMemoryObject* aMemory, TUint aIndex, TBool aDemandPaged)
	{
	TRACE2(("DPageTableMemoryManager::Free(0x%08x,0x%x,%d)",aMemory, aIndex, aDemandPaged));
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	// find page array entry...
	RPageArray::TIter pageList;
	TPhysAddr* p = aMemory->iPages.RemovePageStart(aIndex,pageList);
	if(!p)
		return KErrNoMemory;

	// unmap page...
	aMemory->UnmapPages(pageList,true);

	RamAllocLock::Lock();

	// remove page...
	MmuLock::Lock();
	TPhysAddr pagePhys = RPageArray::RemovePage(p);
	MmuLock::Unlock();

	TInt r;
	if(pagePhys==KPhysAddrInvalid)
		{
		// no page removed...
		r = 0;
		}
	else
		{
		// free the removed page...
		if(aDemandPaged)
			ThePager.PageInFreePages(&pagePhys,1);
		else
			{
			TheMmu.FreeRam(&pagePhys, 1, EPageFixed);

#ifdef BTRACE_KERNEL_MEMORY
			BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryMiscFree, KPageSize);
			--Epoc::KernelMiscPages;
#endif
			}
		r = 1;
		}

	RamAllocLock::Unlock();

	// cleanup...
	aMemory->iPages.RemovePageEnd(aIndex,r);
	return r;
	}


TInt DPageTableMemoryManager::MovePage(	DMemoryObject* aMemory, SPageInfo* aOldPageInfo, 
										TPhysAddr& aNewPage, TUint aBlockZoneId, TBool aBlockRest)
	{
	// This could be a demand paged page table info which can be discarded 
	// but let the PageTableAllocator handle that.
	return ::PageTables.MovePage(aMemory, aOldPageInfo, aBlockZoneId, aBlockRest);
	}


TInt DPageTableMemoryManager::MoveAndAllocPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TZonePageType aPageType)
	{
	// This could be a demand paged page table info which can be discarded 
	// but let the PageTableAllocator handle that.
	return ::PageTables.MoveAndAllocPage(aMemory, aPageInfo, aPageType);
	}


//
// PageTableAllocator
//

void PageTableAllocator::Init2(DMutex* aLock)
	{
	TRACEB(("PageTableAllocator::Init2(0x%x)",aLock));
	iLock = aLock;

	__NK_ASSERT_DEBUG(iUnpagedAllocator.CheckFreeList());

	// scan for already allocated page tables
	// (assumes the first page table is used to map page tables)...
	SPageTableInfo* pti = (SPageTableInfo*)KPageTableInfoBase;
	TUint pages = 0;
	for(;;)
		{
		TPte pte = ((TPte*)KPageTableBase)[pages];
		if(pte==KPteUnallocatedEntry)
			break; // end (assumes no gaps in page table allocation) 

		// process free page tables in this page...
		TUint freeCount = 0;
		do
			{
			if(pti->IsUnused())
				{
				pti->PtClusterAlloc();
				iUnpagedAllocator.iFreeList.Add(&pti->FreeLink());
				++freeCount;
				}
#ifdef _DEBUG
			else
				__NK_ASSERT_DEBUG(pti->IsPtClusterAllocated());
#endif
			}
		while(!(++pti)->IsFirstInPage());
		iUnpagedAllocator.iFreeCount += freeCount;
		__NK_ASSERT_DEBUG(iUnpagedAllocator.CheckFreeList());
		TRACE2(("PT page 0x%08x has %d free tables",pti[-KPtClusterSize].PageTable(),freeCount));

		// count page, and move on to next one...
		++pages;
		__NK_ASSERT_DEBUG(pages<KChunkSize/KPageSize); // we've assumed less than one page table of page tables
		}

	// construct allocator for page table pages...
	iPtPageAllocator.Init2(pages);

	// initialise allocator page table infos...
	iPageTableGroupCounts[0] = pages;
	__NK_ASSERT_DEBUG(pages/KPageTableGroupSize==0); // we've assumed less than 1 page of page table infos

	// FOLLOWING CODE WILL USE THIS OBJECT TO ALLOCATE SOME PAGE TABLES,
	// SO ALLOCATOR MUST BE INITIALISED TO A FIT STATE BEFORE THIS POINT!

	// construct memory object for page tables...
	TMappingCreateFlags mapFlags = (TMappingCreateFlags)(EMappingCreateFixedVirtual|EMappingCreateReserveAllResources);
#if defined(__CPU_PAGE_TABLES_FULLY_CACHED)
	TMemoryAttributes memAttr = EMemoryAttributeStandard;
#else
	TMemoryAttributes memAttr = (TMemoryAttributes)(EMemoryAttributeNormalUncached|EMemoryAttributeDefaultShareable);
#endif
	TMemoryCreateFlags createFlags = (TMemoryCreateFlags)(EMemoryCreateNoWipe|EMemoryCreateCustomManager);
	TInt r = MM::InitFixedKernelMemory(iPageTableMemory, KPageTableBase, KPageTableEnd, pages<<KPageShift, (TMemoryObjectType)(T_UintPtr)&ThePageTableMemoryManager, createFlags, memAttr, mapFlags);
	__NK_ASSERT_ALWAYS(r==KErrNone);
	MM::MemorySetLock(iPageTableMemory,aLock);

	// construct memory object for page table infos...
	memAttr = EMemoryAttributeStandard;
	TUint size = pages*KPtClusterSize*sizeof(SPageTableInfo);
	size = (size+KPageMask)&~KPageMask;
	r = MM::InitFixedKernelMemory(iPageTableInfoMemory, KPageTableInfoBase, KPageTableInfoEnd, size, (TMemoryObjectType)(T_UintPtr)&ThePageTableMemoryManager, createFlags, memAttr, mapFlags);
	__NK_ASSERT_ALWAYS(r==KErrNone);
	MM::MemorySetLock(iPageTableInfoMemory,aLock);

	// make sure we have enough reserve page tables...
	Lock();
	iUnpagedAllocator.Init2(this,KNumReservedPageTables,false);
	iPagedAllocator.Init2(this,0,true);
	Unlock();

	TRACEB(("PageTableAllocator::Init2 done"));
	}


void PageTableAllocator::Init2B()
	{
	TRACEB(("PageTableAllocator::Init2B()"));
	TInt r = iPageTableMemory->iPages.PreallocateMemory();
	__NK_ASSERT_ALWAYS(r==KErrNone);
	r = iPageTableInfoMemory->iPages.PreallocateMemory();
	__NK_ASSERT_ALWAYS(r==KErrNone);
	TRACEB(("PageTableAllocator::Init2B done"));
	}


void PageTableAllocator::TSubAllocator::Init2(PageTableAllocator* aAllocator, TUint aReserveCount, TBool aDemandPaged)
	{
	iReserveCount = aReserveCount;
	iDemandPaged = aDemandPaged;
	while(iFreeCount < aReserveCount)
		__NK_ASSERT_ALWAYS(aAllocator->AllocReserve(*this));
	}


void PageTableAllocator::TPtPageAllocator::Init2(TUint aNumInitPages)
	{
	iLowerAllocator = TBitMapAllocator::New(KMaxPageTablePages,ETrue);
	__NK_ASSERT_ALWAYS(iLowerAllocator);
	iLowerAllocator->Alloc(0,aNumInitPages);
	iLowerWaterMark = aNumInitPages-1;

	iUpperAllocator = TBitMapAllocator::New(KMaxPageTablePages,ETrue);
	__NK_ASSERT_ALWAYS(iUpperAllocator);
	
	__ASSERT_COMPILE(KMaxPageTablePages > (TUint)KMinUnpinnedPagedPtPages);	// Unlikely to be untrue.
	iUpperWaterMark = KMaxPageTablePages - KMinUnpinnedPagedPtPages;
	iPinnedPageTablePages = 0;	// OK to clear this without MmuLock as only one thread running so far.
	}


static TUint32 RandomSeed = 33333;

TUint PageTableAllocator::TPtPageAllocator::RandomPagedPtPage()
	{
	__NK_ASSERT_DEBUG(PageTablesLockIsHeld());

	// Pick an allocated page at random, from iUpperWaterMark - KMaxPageTablePages.
	RandomSeed = RandomSeed * 69069 + 1; // next 'random' number
	TUint allocRange = KMaxPageTablePages - iUpperWaterMark - 1;
	TUint bit = (TUint64(RandomSeed) * TUint64(allocRange)) >> 32;

	// All page table pages should be allocated or we shouldn't be stealing one at random.
	__NK_ASSERT_DEBUG(iUpperAllocator->NotFree(bit, 1));

	return KMaxPageTablePages - 1 - bit;
	}


TInt PageTableAllocator::TPtPageAllocator::Alloc(TBool aDemandPaged)
	{
	__NK_ASSERT_DEBUG(PageTablesLockIsHeld());
	TUint pageIndex;
	if(aDemandPaged)
		{
		TInt bit = iUpperAllocator->Alloc();
		// There are always unpaged page tables so iUpperAllocator will always have 
		// at least one free bit.
		__NK_ASSERT_DEBUG(bit >= 0);

		pageIndex = KMaxPageTablePages - 1 - bit;

		if(pageIndex < iUpperWaterMark)
			{
			// new upper watermark...
			if((pageIndex & ~(KPageTableGroupSize - 1)) <= iLowerWaterMark)
				{
				// clashes with other bitmap allocator, so fail..
				iUpperAllocator->Free(bit);
				TRACE(("TPtPageAllocator::Alloc too low iUpperWaterMark %d ",iUpperWaterMark));
				return -1;
				}
			// Hold mmu lock so iUpperWaterMark isn't read by pinning before we've updated it.
			MmuLock::Lock();
			iUpperWaterMark = pageIndex;
			MmuLock::Unlock();
			TRACE(("TPtPageAllocator::Alloc new iUpperWaterMark=%d",pageIndex));
			}
		}
	else
		{
		TInt bit = iLowerAllocator->Alloc();
		if(bit < 0)
			return bit;
		pageIndex = bit;
		if(pageIndex > iLowerWaterMark)
			{// iLowerAllocator->Alloc() should only pick the next bit after iLowerWaterMark.
			__NK_ASSERT_DEBUG(pageIndex == iLowerWaterMark + 1);
			MmuLock::Lock();
			// new lower watermark...
			if(	pageIndex >= (iUpperWaterMark & ~(KPageTableGroupSize - 1)) ||
				AtPinnedPagedPtsLimit(iUpperWaterMark, pageIndex, iPinnedPageTablePages))
				{
				// clashes with other bitmap allocator or it would reduce the amount 
				// of available unpinned paged page tables too far, so fail..
				MmuLock::Unlock();
				iLowerAllocator->Free(bit);
				TRACE(("TPtPageAllocator::Alloc iLowerWaterMark=%d",iLowerWaterMark));
				return -1;
				}
			iLowerWaterMark = pageIndex;
			MmuLock::Unlock();
			TRACE(("TPtPageAllocator::Alloc new iLowerWaterMark=%d", iLowerWaterMark));
			}
		}
	return pageIndex;
	}


void PageTableAllocator::TPtPageAllocator::Free(TUint aPageIndex, TBool aDemandPaged)
	{
	__NK_ASSERT_DEBUG(PageTablesLockIsHeld());
	if(aDemandPaged)
		iUpperAllocator->Free(KMaxPageTablePages-1-aPageIndex);
	else
		iLowerAllocator->Free(aPageIndex);
	}


void PageTableAllocator::Lock()
	{
	Kern::MutexWait(*iLock);
	}


void PageTableAllocator::Unlock()
	{
	Kern::MutexSignal(*iLock);
	}


TBool PageTableAllocator::LockIsHeld()
	{
	return iLock->iCleanup.iThread == &Kern::CurrentThread();
	}


TBool PageTableAllocator::AllocReserve(TSubAllocator& aSubAllocator)
	{
	__NK_ASSERT_DEBUG(LockIsHeld());

	TBool demandPaged = aSubAllocator.iDemandPaged;

	// allocate page...
	TInt ptPageIndex = iPtPageAllocator.Alloc(demandPaged);
	if (ptPageIndex < 0) 
		{
		if (demandPaged)
			{
			TInt r;
			do
				{
				// Can't fail to find a demand paged page table, otherwise a page fault 
				// could fail with KErrNoMemory.  Therefore, keep attempting to steal a 
				// demand paged page table page until successful.
				TUint index = iPtPageAllocator.RandomPagedPtPage();
				MmuLock::Lock();
				TLinAddr pageTableLin = KPageTableBase + (index << (KPtClusterShift + KPageTableShift));
				TPhysAddr ptPhysAddr = Mmu::LinearToPhysical(pageTableLin);
				// Page tables must be allocated otherwise we shouldn't be stealing the page.
				__NK_ASSERT_DEBUG(ptPhysAddr != KPhysAddrInvalid);
				SPageInfo* ptSPageInfo = SPageInfo::FromPhysAddr(ptPhysAddr);
				r = StealPage(ptSPageInfo);
				MmuLock::Unlock();
				}
			while(r != KErrCompletion);
			// Retry the allocation now that we've stolen a page table page.
			ptPageIndex = iPtPageAllocator.Alloc(demandPaged);
			__NK_ASSERT_DEBUG(ptPageIndex >= 0);
			}		
		else
			{
			return EFalse;
			}
		}

	// commit memory for page...
	__NK_ASSERT_DEBUG(iPageTableMemory); // check we've initialised iPageTableMemory
	TInt r = ThePageTableMemoryManager.Alloc(iPageTableMemory,ptPageIndex,aSubAllocator.iDemandPaged);
	if(r==KErrNoMemory)
		{
		iPtPageAllocator.Free(ptPageIndex,aSubAllocator.iDemandPaged);
		return false;
		}
	__NK_ASSERT_DEBUG(r==KErrNone);

	// allocate page table info...
	TUint ptgIndex = ptPageIndex/KPageTableGroupSize;
	if(!iPageTableGroupCounts[ptgIndex])
		{
		__NK_ASSERT_DEBUG(iPageTableInfoMemory); // check we've initialised iPageTableInfoMemory
		r = ThePageTableMemoryManager.Alloc(iPageTableInfoMemory,ptgIndex,aSubAllocator.iDemandPaged);

		if(r==KErrNoMemory)
			{
			r = ThePageTableMemoryManager.Free(iPageTableMemory,ptPageIndex,aSubAllocator.iDemandPaged);
			__NK_ASSERT_DEBUG(r==1);
			iPtPageAllocator.Free(ptPageIndex,aSubAllocator.iDemandPaged);
			return false;
			}
		__NK_ASSERT_DEBUG(r==KErrNone);
		// For paged page tables set all the page table infos in this page as unused 
		// and their page table clusters as not allocated.
		if (aSubAllocator.iDemandPaged)
			{
			SPageTableInfo* ptiBase = (SPageTableInfo*)KPageTableInfoBase + (ptgIndex*KPageTableInfosPerPage);
			memclr(ptiBase, KPageSize);
			}
		}
	++iPageTableGroupCounts[ptgIndex];

	SPageTableInfo* pti = (SPageTableInfo*)KPageTableInfoBase+ptPageIndex*KPtClusterSize;
	aSubAllocator.AllocPage(pti);
	return true;
	}


void PageTableAllocator::TSubAllocator::AllocPage(SPageTableInfo* aPageTableInfo)
	{
	SPageTableInfo* pti = aPageTableInfo;
	__NK_ASSERT_DEBUG(pti->IsFirstInPage());

	TRACE2(("Alloc PT page (%d) 0x%08x",iDemandPaged,pti->PageTable()));

	// initialise page table infos...
	do pti->New(iDemandPaged);
	while(!(++pti)->IsFirstInPage());
	pti -= KPtClusterSize;

	// all page tables initially unused, so start them off on iCleanupList...
	pti->AddToCleanupList(iCleanupList);
	iFreeCount += KPtClusterSize;
	__NK_ASSERT_DEBUG(CheckFreeList());
	}


SPageTableInfo* PageTableAllocator::TSubAllocator::FreePage()
	{
	if(!IsCleanupRequired())
		return 0;

	// get a completely free page...
	SDblQueLink* link = iCleanupList.Last();
	__NK_ASSERT_DEBUG(link);
	SPageTableInfo* pti = SPageTableInfo::FromFreeLink(link);
	__NK_ASSERT_DEBUG(pti->IsFirstInPage());
	pti->RemoveFromCleanupList();
	iFreeCount -= KPtClusterSize;
	__NK_ASSERT_DEBUG(CheckFreeList());

	TRACE2(("Free PT page (%d) 0x%08x",iDemandPaged,pti->PageTable()));

	// Mark each page table info as no longer having its page table cluster allocated.
	do 
		{// make sure all page tables in page are unused...
		__NK_ASSERT_DEBUG(pti->IsUnused());
		pti->PtClusterFreed();
		}
	while(!(++pti)->IsFirstInPage());
	pti -= KPtClusterSize;

	return pti;
	}


TBool PageTableAllocator::FreeReserve(TSubAllocator& aSubAllocator)
	{
	__NK_ASSERT_DEBUG(LockIsHeld());

	// get a page which needs freeing...
	SPageTableInfo* pti = aSubAllocator.FreePage();
	if(!pti)
		return false;

	// free the page...
	TUint ptPageIndex = ((TLinAddr)pti-KPageTableInfoBase)>>(KPageTableInfoShift+KPtClusterShift);
	iPtPageAllocator.Free(ptPageIndex,aSubAllocator.iDemandPaged);
	TInt r = ThePageTableMemoryManager.Free(iPageTableMemory,ptPageIndex,aSubAllocator.iDemandPaged);
	(void)r;
	__NK_ASSERT_DEBUG(r==1);

	// free page table info...
	TUint ptgIndex = ptPageIndex/KPageTableGroupSize;
	TUint groupCount = iPageTableGroupCounts[ptgIndex]; // compiler handles half-word values stupidly, so give it a hand
	--groupCount;
	iPageTableGroupCounts[ptgIndex] = groupCount;
	if(!groupCount)
		r = ThePageTableMemoryManager.Free(iPageTableInfoMemory,ptgIndex,aSubAllocator.iDemandPaged);

	return true;
	}


TPte* PageTableAllocator::Alloc(TBool aDemandPaged)
	{
	TRACE(("PageTableAllocator::Alloc(%d)",(bool)aDemandPaged));
	TPte* pt = DoAlloc(aDemandPaged);
	TRACE(("PageTableAllocator::Alloc() returns 0x%08x phys=0x%08x",pt,pt?Mmu::PageTablePhysAddr(pt):KPhysAddrInvalid));
	return pt;
	}


TPte* PageTableAllocator::DoAlloc(TBool aDemandPaged)
	{
	__NK_ASSERT_DEBUG(LockIsHeld());

#ifdef _DEBUG
	// simulated OOM, but not if demand paged as this can't fail under normal circumstances...
	if(!aDemandPaged)
		{
		RamAllocLock::Lock();
		TBool fail = K::CheckForSimulatedAllocFail();
		RamAllocLock::Unlock();
		if(fail)
			return 0;
		}
#endif

	TSubAllocator& allocator = aDemandPaged ? iPagedAllocator : iUnpagedAllocator;

	__NK_ASSERT_DEBUG(!iAllocating || !aDemandPaged); // can't recursively allocate demand paged tables

	__NK_ASSERT_DEBUG(iAllocating<=allocator.iReserveCount); // can't recursively allocate more than the reserve

	// keep up enough spare page tables...
	if(!iAllocating++) // if we haven't gone recursive...
		{
		// make sure we have a page table to allocate...
		while(allocator.iFreeCount<=allocator.iReserveCount)
			if(!AllocReserve(allocator))
				{
				--iAllocating;
				return 0; // out of memory
				}
		}
	else
		{
		TRACE(("PageTableAllocator::DoAlloc recurse=%d",iAllocating));
		}

	// allocate a page table...
	SPageTableInfo* pti = allocator.Alloc();

	// initialise page table info...
	pti->Init();

	// initialise page table...
	TPte* pt = pti->PageTable();
	memclr(pt,KPageTableSize);
	CacheMaintenance::MultiplePtesUpdated((TLinAddr)pt,KPageTableSize);

	// done...
	--iAllocating;
	return pt;
	}


SPageTableInfo* PageTableAllocator::TSubAllocator::Alloc()
	{
	__NK_ASSERT_DEBUG(PageTablesLockIsHeld());
	__NK_ASSERT_DEBUG(iFreeCount);
	__NK_ASSERT_DEBUG(CheckFreeList());

	// get next free page table...
	SDblQueLink* link = iFreeList.GetFirst();
	SPageTableInfo* pti; 
	if(link)
		pti = SPageTableInfo::FromFreeLink(link);
	else
		{
		// need to get one back from the cleanup list...
		link = iCleanupList.First();
		__NK_ASSERT_DEBUG(link); // we can't be out of page tables
		pti = SPageTableInfo::FromFreeLink(link);
		__NK_ASSERT_DEBUG(pti->IsFirstInPage());
		pti->RemoveFromCleanupList();

		// add other page tables in the page to the free list...
		SPageTableInfo* free = pti+1;
		while(!free->IsFirstInPage())
			{
			__NK_ASSERT_DEBUG(free->IsUnused());
			iFreeList.Add(&free->FreeLink());
			++free;
			}
		}

	// count page as allocated...
	--iFreeCount;
	__NK_ASSERT_DEBUG(pti->IsUnused());
	__NK_ASSERT_DEBUG(CheckFreeList());

	return pti;
	}


void PageTableAllocator::Free(TPte* aPageTable)
	{
	TRACE(("PageTableAllocator::Free(0x%08x)",aPageTable));
	DoFree(aPageTable);
	}


void PageTableAllocator::DoFree(TPte* aPageTable)
	{
	__NK_ASSERT_DEBUG(LockIsHeld());

	// make sure page table isn't being aliased...
	TPhysAddr pagePhys = Mmu::PageTablePhysAddr(aPageTable);
	__NK_ASSERT_DEBUG(pagePhys!=KPhysAddrInvalid);
	TheMmu.RemoveAliasesForPageTable(pagePhys);

	// free page table...
	SPageTableInfo* pti = SPageTableInfo::FromPtPtr(aPageTable);
	TSubAllocator& allocator = pti->IsDemandPaged() ? iPagedAllocator : iUnpagedAllocator;
	allocator.Free(pti);

	// check for surplus pages...
	if(allocator.IsCleanupRequired())
		{
		iCleanup.Add(CleanupTrampoline,this);
		}
	}


void PageTableAllocator::TSubAllocator::Free(SPageTableInfo* aPageTableInfo)
	{
	__NK_ASSERT_DEBUG(PageTablesLockIsHeld());
	__NK_ASSERT_DEBUG(CheckFreeList());

	SPageTableInfo* pti = aPageTableInfo;

	// clear the page table info...
	MmuLock::Lock();
	__NK_ASSERT_DEBUG(!pti->PermanenceCount());
	pti->SetUnused();
	MmuLock::Unlock();

	// scan other page tables in same page...
	SPageTableInfo* first = pti->FirstInPage();
	SPageTableInfo* last = pti->LastInPage();
	SPageTableInfo* prev;
	SPageTableInfo* next;

	// try insert page table after previous free page table in same page...
	prev = pti;
	while(prev>first)
		{
		--prev;
		if(prev->IsUnused())
			{
			pti->FreeLink().InsertAfter(&prev->FreeLink());
			goto inserted;
			}
		}

	// try insert page table before next free page table in same page...
	next = pti;
	while(next<last)
		{
		++next;
		if(next->IsUnused())
			{
			pti->FreeLink().InsertBefore(&next->FreeLink());
			goto inserted;
			}
		}

	// only free page table in page, so link into start of free list...
	pti->FreeLink().InsertAfter(&iFreeList.iA);

inserted:
	++iFreeCount;
	__NK_ASSERT_DEBUG(CheckFreeList());

	// see if all page tables in page are empty...
	pti = first;
	do
		{
		if(!pti->IsUnused())
			return; // some page tables still in use, so end
		}
	while(!(++pti)->IsFirstInPage());
	pti -= KPtClusterSize;

	// check if page with page table in is pinned...
	MmuLock::Lock();
	TPte* pt = pti->PageTable();
	TPhysAddr pagePhys = Mmu::PageTablePhysAddr(pt);
	SPageInfo* pi = SPageInfo::FromPhysAddr(pagePhys);
	TBool pinned = pi->PagedState()==SPageInfo::EPagedPinned;
	MmuLock::Unlock();
	// Note, the pinned state can't change even though we've now released the MmuLock.
	// This is because all page tables in the page are unused and we don't pin unused
	// page tables. Also, the page table(s) can't become used again whilst this function
	// executes as we hold the page table allocator lock.
	if(pinned)
		{
		// return now and leave page table(s) in free list if their page is pinned...
		// Note, when page is unpinned it will end up in the paging live list and
		// eventually be reclaimed for other use (if the page tables in the page
		// don't get reallocated before then).
		__NK_ASSERT_DEBUG(pti->IsDemandPaged()); // only paged page tables should have been pinned
		return; 
		}

	// the page with our page table in it is no longer in use...
	MoveToCleanup(pti);
	}


void PageTableAllocator::TSubAllocator::MoveToCleanup(SPageTableInfo* aPageTableInfo)
	{
	__NK_ASSERT_DEBUG(PageTablesLockIsHeld());
	__NK_ASSERT_DEBUG(CheckFreeList());

	SPageTableInfo* pti = aPageTableInfo;
	__NK_ASSERT_DEBUG(pti->IsFirstInPage());

	TRACE2(("Cleanup PT page (%d) 0x%08x",iDemandPaged,pti->PageTable()));

	// make sure all page tables in page are unused...
#ifdef _DEBUG
	do __NK_ASSERT_DEBUG(pti->IsUnused());
	while(!(++pti)->IsFirstInPage());
	pti -= KPtClusterSize;
#endif

	// unlink all page tables in page...
	SDblQueLink* prev = pti->FreeLink().iPrev;
	SDblQueLink* next = pti->LastInPage()->FreeLink().iNext;
	prev->iNext = next;
	next->iPrev = prev;

	// add page tables to cleanup list...
	__NK_ASSERT_DEBUG(!pti->IsOnCleanupList());
	pti->AddToCleanupList(iCleanupList);
	__NK_ASSERT_DEBUG(CheckFreeList());
	}



TBool PageTableAllocator::TSubAllocator::IsCleanupRequired()
	{
	return iFreeCount>=iReserveCount+KPtClusterSize && !iCleanupList.IsEmpty();
	}


#ifdef _DEBUG

TBool PageTableAllocator::TSubAllocator::CheckFreeList()
	{
	TUint count = iFreeCount;

	// count page tables in iCleanupList...
	SDblQueLink* head = &iCleanupList.iA;
	SDblQueLink* link = head;
	for(;;)
		{
		link = link->iNext;
		if(link==head)
			break;
		SPageTableInfo* pti = SPageTableInfo::FromFreeLink(link);
		__NK_ASSERT_DEBUG(pti->IsFirstInPage());
		__NK_ASSERT_DEBUG(pti->IsOnCleanupList());
		if(count<(TUint)KPtClusterSize)
			return false;
		count -= KPtClusterSize;
		}

	// count page tables in iFreeList...	
	head = &iFreeList.iA;
	link = head;
	while(count)
		{
		link = link->iNext;
		if(link==head)
			return false;

		// check next free page table in page is linked in correct order...
		SPageTableInfo* pti = SPageTableInfo::FromFreeLink(link);
		SPageTableInfo* last = pti->LastInPage();
		SPageTableInfo* next = pti;
		while(next<last)
			{
			++next;
			if(next->IsUnused())
				{
				__NK_ASSERT_DEBUG(pti->FreeLink().iNext==&next->FreeLink());
				__NK_ASSERT_DEBUG(next->FreeLink().iPrev==&pti->FreeLink());
				break;
				}
			}

		--count;
		}

	return link->iNext==head;
	}

#endif



//
// Paged page table handling
//

TInt SPageTableInfo::ForcedFree()
	{
	__NK_ASSERT_DEBUG(PageTablesLockIsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(IsDemandPaged());

	TUint type = iType;

	if(type==EUnused)
		return KErrNone;

	__NK_ASSERT_DEBUG(iPermanenceCount==0);

	// clear all PTEs in page table...
	TPte* pt = PageTable();
	memclr(pt,KPageTableSize);
	__e32_memory_barrier(); // make sure all CPUs read zeros from pt so forcing a page-in (rather than a rejuvenate) if accessed
	iPageCount = 0;

	if(type==ECoarseMapping)
		{
		TRACE2(("SPageTableInfo::ForcedFree() coarse 0x%08x 0x%x %d",iCoarse.iMemoryObject,iCoarse.iChunkIndex,iCoarse.iPteType));
		// mustn't release MmuLock between clearing page table and calling this
		// (otherwise page table may get updated before its actually removed from
		// the memory object)...
		iCoarse.iMemoryObject->StealPageTable(iCoarse.iChunkIndex,iCoarse.iPteType);
		}
	else if(type==EFineMapping)
		{
		// need to remove page table from address spaces's page directory...
		TLinAddr addr = iFine.iLinAddrAndOsAsid;
		TUint osAsid = addr&KPageMask;
		TPde* pPde = Mmu::PageDirectoryEntry(osAsid,addr);

		TRACE2(("SPageTableInfo::ForcedFree() fine %d 0x%08x",osAsid,addr&~KPageMask));

		TPde pde = KPdeUnallocatedEntry;
		TRACE2(("!PDE %x=%x",pPde,pde));
		*pPde = pde;
		SinglePdeUpdated(pPde);
		}
	else
		{
		// invalid type...
		__NK_ASSERT_DEBUG(0);
		return KErrNotSupported;
		}

	MmuLock::Unlock();

	// make sure page table updates visible to MMU...
	CacheMaintenance::MultiplePtesUpdated((TLinAddr)pt,KPageTableSize);
	InvalidateTLB();

	// free the page table back to the allocator,
	// this will also remove any IPC alias using it...
	__NK_ASSERT_DEBUG(iPageCount==0); // should still be unused
	::PageTables.Free(pt);

	MmuLock::Lock();

	return KErrNone;
	}


TInt PageTableAllocator::StealPage(SPageInfo* aPageInfo)
	{
	TRACE2(("PageTableAllocator::StealPage(0x%08x)",aPageInfo));
	__NK_ASSERT_DEBUG(LockIsHeld()); // only works if PageTableAllocator lock is the RamAllocLock
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	if (aPageInfo->Owner() == iPageTableInfoMemory)
		return StealPageTableInfo(aPageInfo);

	__UNLOCK_GUARD_START(MmuLock);

	// This must be a page table page so steal it.
	__NK_ASSERT_ALWAYS(aPageInfo->Owner()==iPageTableMemory);
	TUint ptPageIndex = aPageInfo->Index();
	SPageTableInfo* pti = (SPageTableInfo*)KPageTableInfoBase+ptPageIndex*KPtClusterSize;

	aPageInfo->SetModifier(&pti);
	__UNLOCK_GUARD_END(MmuLock);

	// forcibly free each page table in the page...
	TInt r;
	do
		{// Check for pinning, ForcedFree() releases MmuLock so must check for
		// each page table.
		if (aPageInfo->PagedState() == SPageInfo::EPagedPinned)
			{// The page table page is pinned so can't steal it.
			r = KErrInUse;
			break;
			}
		r = pti->ForcedFree();
		if(r!=KErrNone)
			break;
		if(aPageInfo->CheckModified(&pti))
			{
			r = KErrInUse;
			break;
			}
		}
	while(!(++pti)->IsFirstInPage());
	pti -= KPtClusterSize; // restore pti back to first page table

	if(r==KErrNone)
		{
		MmuLock::Unlock();
		if(!pti->IsOnCleanupList())
			{
			// the page might not already be on the cleanup list in the case where
			// it was previously freed whilst it was pinned.
			// In this case, a later unpinning would have put it back into the paging live
			// list from where it is now subsequently being stolen...
			iPagedAllocator.MoveToCleanup(pti);
			}
		// free the page from allocator so it ends up back in the paging pool as a free page...
		while(FreeReserve(iPagedAllocator))
			{}
		// return an 'error' to indicate page has not been stolen.
		// We have however achieved the main aim of making the page 'free' and
		// it will be available if page stealing attempts to steal the page again...
		r = KErrCompletion;
		MmuLock::Lock();
		}

	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	TRACE2(("PageTableAllocator::StealPage returns %d",r));
	return r;
	}


TInt PageTableAllocator::StealPageTableInfo(SPageInfo* aPageInfo)
	{
	// Need to steal every page table for every page table info in this page.
	// This page can't be modified or removed as we hold the lock, however
	// the page table pages being freed may be rejuvenated and therefore their 
	// SPageInfos may be marked as modified.
	TInt r = KErrNone;
	TUint ptiOffset = aPageInfo->Index() * KPageTableInfosPerPage;
	SPageTableInfo* ptiBase = (SPageTableInfo*)KPageTableInfoBase + ptiOffset;
	SPageTableInfo* ptiEnd = ptiBase + KPageTableInfosPerPage;
	TUint flash = 0;
	for (SPageTableInfo* pti = ptiBase; pti < ptiEnd;)
		{// Free each page table cluster that is allocated.
		if (pti->IsPtClusterAllocated())
			{
			TPhysAddr ptPhysAddr = Mmu::LinearToPhysical((TLinAddr)pti->PageTable());
			SPageInfo* ptSPageInfo = SPageInfo::FromPhysAddr(ptPhysAddr);
			ptSPageInfo->SetModifier(&flash);
			do 
				{
				__NK_ASSERT_DEBUG(pti->IsPtClusterAllocated());
				if (aPageInfo->PagedState() == SPageInfo::EPagedPinned || 
					ptSPageInfo->PagedState() == SPageInfo::EPagedPinned)
					{// The page table or page table info is pinned so can't steal info page.
					r = KErrInUse;
					break;
					}
				r = pti->ForcedFree();
				if(r!=KErrNone)
					break;
				if(ptSPageInfo->CheckModified(&flash))
					{// The page table page has been rejunvenated so can't steal it.
					r = KErrInUse;
					break;
					}
				}
			while (!(++pti)->IsFirstInPage());
			if (r != KErrNone)
				break;
			SPageTableInfo* ptiTmp = pti - KPtClusterSize;
			MmuLock::Unlock();
			if(!ptiTmp->IsOnCleanupList())
				{
				// the page might not already be on the cleanup list in the case where
				// it was previously freed whilst it was pinned.
				// In this case, a later unpinning would have put it back into the paging live
				// list from where it is now subsequently being stolen...
				iPagedAllocator.MoveToCleanup(ptiTmp);
				}
			MmuLock::Lock();
			flash = 0;		// The MmuLock has been flashed at least once.
			}
		else
			{// Move onto the next cluster this page of page table infos refers to.
			__NK_ASSERT_DEBUG(pti->IsFirstInPage());
			pti += KPtClusterSize;
			MmuLock::Flash(flash,KMaxPageInfoUpdatesInOneGo);
			}
		}
	// free the pages discarded from allocator so they end up back in the paging pool as free pages...
	MmuLock::Unlock();
	while(FreeReserve(iPagedAllocator))
		{}
	if (r == KErrNone)
		r = KErrCompletion;	// The pager needs to remove the page from the live list.
	MmuLock::Lock();
	return r;
	}


TInt PageTableAllocator::MovePage(DMemoryObject* aMemory, SPageInfo* aOldPageInfo, 
									TUint aBlockZoneId, TBool aBlockRest)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	// We don't move page table or page table info pages, however, if this page 
	// is demand paged then we may be able to discard it.
	if (aOldPageInfo->Owner() == iPageTableInfoMemory)
		{
		if (!(iPtPageAllocator.IsDemandPagedPtInfo(aOldPageInfo)))
			{
			MmuLock::Unlock();
			return KErrNotSupported;
			}
		}
	else
		{
		__NK_ASSERT_DEBUG(aOldPageInfo->Owner() == iPageTableMemory);
		if (!(iPtPageAllocator.IsDemandPagedPt(aOldPageInfo)))
			{
			MmuLock::Unlock();
			return KErrNotSupported;
			}
		}
	if (aOldPageInfo->PagedState() == SPageInfo::EPagedPinned)
		{// The page is pinned so don't attempt to discard it as pinned pages 
		// can't be discarded.  Also, the pager will invoke this method again.
		MmuLock::Unlock();
		return KErrInUse;
		}
	// Let the pager discard the page as it controls the size of the live list.
	// If the size of the live list allows then eventually 
	// PageTableAllocator::StealPage() will be invoked on this page.
	TUint moveDisFlags = (aBlockRest)? M::EMoveDisBlockRest : 0;
	return ThePager.DiscardPage(aOldPageInfo, aBlockZoneId, moveDisFlags);
	}


TInt PageTableAllocator::MoveAndAllocPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TZonePageType aPageType)
	{
	TInt r = MovePage(aMemory, aPageInfo, KRamZoneInvalidId, EFalse);
	if (r == KErrNone)
		{
		TheMmu.MarkPageAllocated(aPageInfo->PhysAddr(), aPageType);
		}
	return r;
	}


TInt PageTableAllocator::PinPageTable(TPte* aPageTable, TPinArgs& aPinArgs)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(SPageTableInfo::FromPtPtr(aPageTable)->IsDemandPaged());
	__NK_ASSERT_DEBUG(!SPageTableInfo::FromPtPtr(aPageTable)->IsUnused());
	__NK_ASSERT_DEBUG(aPinArgs.HaveSufficientPages(KNumPagesToPinOnePageTable));

	// pin page with page table in...
	TPhysAddr pagePhys = Mmu::PageTablePhysAddr(aPageTable);
	SPageInfo* pi = SPageInfo::FromPhysAddr(pagePhys);
	if (!pi->PinCount())
		{// Page is being pinned having previously been unpinned.
		TInt r = iPtPageAllocator.PtPagePinCountInc();
		if (r != KErrNone)
			return r;
		}
	ThePager.Pin(pi,aPinArgs);

	// pin page with page table info in...
	SPageTableInfo* pti = SPageTableInfo::FromPtPtr(aPageTable);
	pagePhys = Mmu::UncheckedLinearToPhysical((TLinAddr)pti,KKernelOsAsid);
	pi = SPageInfo::FromPhysAddr(pagePhys);
	ThePager.Pin(pi,aPinArgs);
	return KErrNone;
	}


void PageTableAllocator::UnpinPageTable(TPte* aPageTable, TPinArgs& aPinArgs)
	{
	// unpin page with page table info in...
	SPageTableInfo* pti = SPageTableInfo::FromPtPtr(aPageTable);
	TPhysAddr pagePhys = Mmu::UncheckedLinearToPhysical((TLinAddr)pti,KKernelOsAsid);
	SPageInfo* pi = SPageInfo::FromPhysAddr(pagePhys);
	ThePager.Unpin(pi,aPinArgs);

	// unpin page with page table in...
	pagePhys = Mmu::PageTablePhysAddr(aPageTable);
	pi = SPageInfo::FromPhysAddr(pagePhys);
	ThePager.Unpin(pi,aPinArgs);

	if (!pi->PinCount())
		{// This page table page is no longer pinned.
		iPtPageAllocator.PtPagePinCountDec();
		}
	}


#ifdef _DEBUG
TBool IsPageTableUnpagedRemoveAllowed(SPageInfo* aPageInfo)
		{ return ::PageTables.IsPageTableUnpagedRemoveAllowed(aPageInfo); }

TBool PageTableAllocator::IsPageTableUnpagedRemoveAllowed(SPageInfo* aPageInfo)
	{
	if (aPageInfo->Owner() == iPageTableInfoMemory)
		{// Page table info pages are never added to the live list but can be
		// stolen via DPager::StealPage()
		return ETrue;
		}

	if (aPageInfo->Owner() == iPageTableMemory)
		{// Page table pages are added to the live list but only after the page they 
		// map has been paged in. Therefore, a pde can reference a pte before it has been
		// added to the live list so allow this but for uninitialised page table pages only.
		TUint ptPageIndex = aPageInfo->Index();
		SPageTableInfo* pti = (SPageTableInfo*)KPageTableInfoBase+ptPageIndex*KPtClusterSize;
		do
			{
			if (!pti->IsUnused())
				{
				TPte* pte = pti->PageTable();
				TPte* pteEnd = pte + (KPageTableSize/sizeof(TPte));
				while (pte < pteEnd)
					if (*pte++ != KPteUnallocatedEntry)
						return EFalse;
				}
			}
		while(!(++pti)->IsFirstInPage());
		return ETrue;
		}
	return EFalse;
	}
#endif


//
// Cleanup
//

void PageTableAllocator::CleanupTrampoline(TAny* aSelf)
	{
	((PageTableAllocator*)aSelf)->Cleanup();
	}


void PageTableAllocator::Cleanup()
	{
	// free any surplus pages...
	Lock();
	while(FreeReserve(iPagedAllocator) || FreeReserve(iUnpagedAllocator))
		{}
	Unlock();
	}
