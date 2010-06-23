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

/**
 @file
 @internalComponent
*/

#ifndef MPTALLOC_H
#define MPTALLOC_H

#include "mcleanup.h"

/**
Number of #SPageTableInfo structures which will fit into a page of RAM.
*/
const TUint KPageTableInfosPerPage = KPageSize/sizeof(SPageTableInfo);

/**
Number of pages of page tables which correspond to a page of page infos.
*/
const TUint KPageTableGroupSize = KPageTableInfosPerPage/KPtClusterSize;

/**
Max number of RAM pages which can be used for page tables.
*/
const TUint KMaxPageTablePages = (KPageTableEnd-KPageTableBase)/KPageSize;


/**
The maximum number of pages required to pin a single page table.
*/
const TUint KNumPagesToPinOnePageTable = 2; // 1 page table page + 1 page table info page

/**
The minimum number of unpinned paged page table pages required so a page fault 
can't fail to allocate a page table.
*/
const TUint KMinUnpinnedPagedPtPages = KMaxCpus;


/**
Class for allocating MMU page tables.
*/
class PageTableAllocator
	{
public:
	void Init2(DMutex* aLock);
	void Init2B();

	/**
	Allocate a page table.

	@param aDemandPaged	True if the page table will be used to map demand paged memory;
						false, otherwise.

	@return Virtual address of the allocated page table,
			or the null-pointer if there was insufficient memory.

	@pre #PageTablesLockIsHeld, i.e. current thread has called #Lock()
	*/
	TPte* Alloc(TBool aDemandPaged);

	/**
	Free a page table previously aquired with #Alloc.

	@param aPageTable	Virtual address of the page table,

	@pre #PageTablesLockIsHeld, i.e. current thread has called #Lock()
	*/
	void Free(TPte* aPageTable);

	/**
	Acquire the mutex used to protect page table allocation.
	*/
	void Lock();

	/**
	Release the mutex used to protect page table allocation.
	*/
	void Unlock();

	/**
	Return true if the current thread has acquired the mutex used to protect page table allocation.
	I.e. has called #Lock().
	*/
	TBool LockIsHeld();

	/**
	Steal a RAM page which is currently being used to store demand paged page tables
	or page table infos.

	This removes the page tables contained in the RAM from any objects which own
	them and then returns the RAM to the demand paging system as a free page. This
	will not ever return KErrNone indicating that the page has been successfully
	stolen.

	This is only intended for use by DPageTableMemoryManager::StealPage.

	@param aPageInfo	The page information structure of the page to be stolen.

	@return KErrCompletion to indicate that the page was stolen but has been
			returned to the demand paging live list as a free page.
			Otherwise, KErrInUse if the page was not able to be freed.

	@pre #PageTablesLockIsHeld, i.e. current thread has called #Lock()
	@pre MmuLock held.
	*/
	TInt StealPage(SPageInfo* aPageInfo);

	/**
	Discards a page of page tables or page table infos but only if it is demand paged.
	
	This will only be invoked on page table info pages or pinned paged tables 
	as they aren't on the live list and so M::MovePage() will not know the pager 
	can discard them.

	@param aMemory		This should always be the page table info memory object.
	@param aOldPageInfo	The page info of the page to discard.
	@param aBlockZoneId	The ID of any RAM zone not to be allocated into.
	@param aBlockRest	ETrue when any allocations should stop if blocked zone hit.

	@return KErrNone if the page could be successfully discarded and its RAM page freed.
	*/
	TInt MovePage(	DMemoryObject* aMemory, SPageInfo* aOldPageInfo, 
						TUint aBlockZoneId, TBool aBlockRest);

	/**
	Attempts to discard a page of page tables using #MovePage.  If the discard was 
	succesful it then allocates the page with type aPageType.
	
	@param aMemory		This should always be the page table info memory object.
	@param aPageInfo	The page info of the page to discard.

	@return KErrNone if the page could be successfully discarded and its RAM page allocated.
	*/
	TInt MoveAndAllocPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TZonePageType aPageType);

#ifdef _DEBUG
	/**
	Debug function for use by DPager::RemovePage() to allow it to remove
	pages with paged state == EUnpaged.

	A page table page may be stolen when it is unpaged as it has been
	allocated via DMemoryMapping::AllocatePageTable() but not yet rejuvenated
	by Mmu::PageInPages() as the MmuLock is released between these stages.

	A page table info page is never added to the live list so it will always
	be unpaged but it can be stolen so allow it to be removed.

	@param	aPageInfo	The page info of the page.

	@return ETrue if the page is a page table info page, EFalse otherwise.
	*/
	TBool IsPageTableUnpagedRemoveAllowed(SPageInfo* aPageInfo);
#endif

	/**
	Pin the RAM page containing a page table, as well as the RAM page
	containing its #SPageTableInfo structure.

	@param aPageTable	Virtual address of the page table,
	@param aPinArgs		The resources to use for pinning. This must have
						at least #KNumPagesToPinOnePageTable replacement
						pages available.
	*/
	TInt PinPageTable(TPte* aPageTable, TPinArgs& aPinArgs);

	/**
	Unpin the RAM page containing a page table, as well as the RAM page
	containing its #SPageTableInfo structure.
	This reverses the action of #PinPageTable.

	@param aPageTable	Virtual address of the page table,
	@param aPinArgs		The resources to use for pinning. The replacement
						pages count in this will be incremented for each
						completely unpinned, e.g. those which can be reused
						as new replacement pages or freed.
	*/
	void UnpinPageTable(TPte* aPageTable, TPinArgs& aPinArgs);

private:
	/**
	Sub-allocator used for managing page tables of a given 'pagedness' (paged/not-paged).
	Each allocator maintains a list free page tables (#iFreeList) from which it can allocate.
	As well as a separate list of RAM pages which have no allocated page tables, #iCleanupList.
	Page tables in the RAM on #iCleanupList do not appear in #iFreeList.
	*/
	class TSubAllocator
		{
	public:
		void Init2(PageTableAllocator* iAllocator, TUint aReserveCount, TBool aDemandPaged);

		/**
		Allocate a page table from this sub-allocator.

		@return The #SPageTableInfo structure of the page table,
				or the null-pointer if none could be allocated.
		*/
		SPageTableInfo* Alloc();

		/**
		Free a page table back to this sub-allocator.

		@param aPageTableInfo	The #SPageTableInfo structure of the page table.
		*/
		void Free(SPageTableInfo* aPageTableInfo);

		/**
		Add a single page of page tables to this allocator for management.

		@param aPageTableInfo	The #SPageTableInfo structure of the first page table
								contained in the page.
		*/
		void AllocPage(SPageTableInfo* aPageTableInfo);

		/**
		Attempt to remove a single unused page of page tables from this allocator.

		@return The #SPageTableInfo structure of the first page table contained
				in the removed page. Or the null-pointer if there were no unused
				memory to be freed.
		*/
		SPageTableInfo* FreePage();

		/**
		Move a page of RAM containing page tables to #iCleanupList.
		All page tables in the page must be currently unused.

		@param aPageTableInfo	The #SPageTableInfo structure of the first page table
								contained in the page.
		*/
		void MoveToCleanup(SPageTableInfo* aPageTableInfo);

		/**
		Return true if there are whole RAM pages which can be freed from this
		sub-allocator without reducing #iFreeCount below #iReserveCount.
		*/
		TBool IsCleanupRequired();

		/**
		Debug check returning true if this objects lists are in a valid state.
		*/
		TBool CheckFreeList();
	public:
		SDblQue iFreeList;		///< List of unused page tables, linked by their SPageTableInfo::FreeLink.
		SDblQue iCleanupList;	///< List of unused pages, linked by the SPageTableInfo::FreeLink of the first page table in the page.
		TUint iFreeCount;		///< Total free page tables in pages on #iFreeList and #iCleanupList.
		TUint iReserveCount;	///< Minimum number of page tables to keep in reserve.
		TBool iDemandPaged;		///< True if this allocator id used for demand paged page tables.
		};

	/**
	Implementation of #Alloc.
	*/
	TPte* DoAlloc(TBool aDemandPaged);

	/**
	Implementation of #Free.
	*/
	void DoFree(TPte* aPageTable);

	/**
	Allocate resources for a pages worth of page tables owned by \a aSubAllocator.

	@return True if the resources were successfully allocated.

	@pre #PageTablesLockIsHeld, i.e. current thread has called #Lock()
	*/
	TBool AllocReserve(TSubAllocator& aSubAllocator);

	/**
	Free the resources taken up by a pages worth of unused page tables
	owned by \a aSubAllocator.

	@return True, if any resources were freed.
			False, if there are no more unused resources.

	@pre #PageTablesLockIsHeld, i.e. current thread has called #Lock()
	*/
	TBool FreeReserve(TSubAllocator& aSubAllocator);

	/**
	Steal a RAM page which is currently being used to store demand paged page 
	table infos.

	This removes all the page tables references by the page table infos contained in 
	the RAM from any objects which own them and then returns the RAM to the demand 
	paging system as a free page. This will not ever return KErrNone indicating that 
	the page has been successfully stolen.

	This is only intended for use by PageTableAllocator::StealPage.

	@param aPageInfo	The page information structure of the page to be stolen.

	@return KErrCompletion to indicate that the page was stolen but has been
			returned to the demand paging live list as a free page.
			Otherwise, KErrInUse if the page was not able to be freed.

	@pre #PageTablesLockIsHeld, i.e. current thread has called #Lock()
	@pre MmuLock held.
	*/
	TInt StealPageTableInfo(SPageInfo* aPageInfo);

	/**
	Free all unused resources taken up for page table management.
	*/
	void Cleanup();

	/**
	Trampoline function for use with iCleanup which redirects to #Cleanup().
	*/
	static void CleanupTrampoline(TAny* aSelf);

private:
	/**
	Sub-allocator for allocating unpaged page tables.
	*/
	TSubAllocator iUnpagedAllocator;

	/**
	Sub-allocator for allocating demand paged page tables.
	*/
	TSubAllocator iPagedAllocator;

	/**
	Object used for queueing cleanup callbacks to #CleanupTrampoline.
	*/
	TMemoryCleanup iCleanup;

	/**
	Recursion count for #Alloc.
	*/
	TUint iAllocating;

	/**
	The mutex used to protect page table allocation.
	*/
	DMutex* iLock;

	/**
	The memory object used to store the memory containing page tables.
	*/
	DMemoryObject* iPageTableMemory;

	/**
	The memory object used to store #SPageTableInfo structures.
	*/
	DMemoryObject* iPageTableInfoMemory;

	/**
	Helper class for allocating page index values within #iPageTableMemory.
	This wraps up to two bitmap allocators, one each used for paged and unpaged
	page tables.

	Page indexes are allocated in a way which ensures that there will not be
	any SPageTableInfo structures for unpaged page tables in the same RAM page
	as the structures for paged page tables.
	*/
	class TPtPageAllocator
		{
	public:
		void Init2(TUint aNumInitPages);
		TInt Alloc(TBool aDemandPaged);
		void Free(TUint aPageIndex, TBool aDemandPaged);
		
		/**
		Determine if the page table info page is paged.
		
		@param aPageInfo Pointer to the SPageInfo of the page table info page.
		@return ETrue if the page table info page is paged, EFalse otherwise.
		@pre MmuLock is held.
		*/
		inline TBool IsDemandPagedPtInfo(SPageInfo* aPageInfo)
			{
			// Is the highest page table index this page table info page can reference 
			// allocated within the demand paged region of the page table address space.
			TUint groupIndex = aPageInfo->Index();
			return ((groupIndex+1) * KPageTableGroupSize)-1 >= iUpperWaterMark;
			}

		/**
		Determine if the page table page is paged.
		
		@param aPageInfo Pointer to the SPageInfo of the page table info page.
		@return ETrue if the page table page is paged, EFalse otherwise.
		@pre MmuLock is held.	
		*/
		inline TBool IsDemandPagedPt(SPageInfo* aPageInfo)
			{
			return aPageInfo->Index() >= iUpperWaterMark;
			}

		/**
		Get a random paged page table page.
		
		@return The index of a paged page table page.
		@pre All paged page table pages are allocated.
		@pre Page tables lock is held.
		*/
		TUint RandomPagedPtPage();

		/**
		Increase the count of pinned paged page table pages.
		
		@return KErrNone on success, KErrNoMemory if too many pages are already pinned.
		@pre MmuLock is held
		*/
		inline TInt PtPagePinCountInc()
			{
			if (AtPinnedPagedPtsLimit(iUpperWaterMark, iLowerWaterMark, iPinnedPageTablePages + 1))
				{
				return KErrNoMemory;
				}
			iPinnedPageTablePages++;
			return KErrNone;
			}

		/**
		Decrease the count of pinned paged page table pages.
		
		@pre MmuLock is held
		*/
		inline void PtPagePinCountDec()
			{
			__NK_ASSERT_DEBUG(iPinnedPageTablePages);	// Can't be zero.
			iPinnedPageTablePages--;
			}

	private:
		/**
		Check whether it is safe to pin a paged page table or reduce the amount of 
		virtual address space available to paged page tables.  By checking that we 
		either have spare virtual address space to increase the	amount of paged page 
		tables or that there are already enough unpinned paged page tables.
		
		@return ETrue if there isn't or EFalse if it is ok to pin more paged page
				tables or increase the number of unpaged page tables.
		*/
		TBool AtPinnedPagedPtsLimit(TUint aUpperWaterMark, TUint aLowerWaterMark, TUint aPinnedPtPages)
			{
			TUint adjustedUpperWaterMark = aUpperWaterMark & ~(KPageTableGroupSize - 1);
			TUint availPagedPtPages = KMaxPageTablePages - adjustedUpperWaterMark;
			TUint availUnpinnedPagedPtPages = availPagedPtPages - aPinnedPtPages;
			// This check is sufficient as we only increase the pinned paged page table 
			// pages or unpaged page table pages one at a time.
			return (aLowerWaterMark + 1 == adjustedUpperWaterMark && 
					availUnpinnedPagedPtPages < KMinUnpinnedPagedPtPages);
			}

	private:
		TBitMapAllocator* iLowerAllocator; ///< Allocator for unpaged page tables
		TUint iLowerWaterMark; ///< Highest page index allocated by iLowerAllocator
		TBitMapAllocator* iUpperAllocator; ///< Allocator for demand paged page tables
		TUint iUpperWaterMark; ///< Lowest page index allocated by iUpperAllocator
		TUint iPinnedPageTablePages; ///< The number of pinned paged page table pages.
		};

	/**
	Allocator for page indexes within #iPageTableMemory.
	*/
	TPtPageAllocator iPtPageAllocator;

	/**
	Array which contains usage for pages of #SPageTableInfo structures.
	When the count is zero, there are no structure in use in the corresponding
	page of memory in #iPageTableInfoMemory. Indicating that the memory may be
	freed.
	*/
	TUint16 iPageTableGroupCounts[KMaxPageTablePages/KPageTableGroupSize];

	friend class TSubAllocator;
	};


/**
The single instance of the #PageTableAllocator.
*/
extern PageTableAllocator PageTables;


#endif
