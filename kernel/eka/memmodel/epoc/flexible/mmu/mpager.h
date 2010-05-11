// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef MPAGER_H
#define MPAGER_H

#include "mmu.h"
#include <kern_priv.h>

/**
Maximum number of pages to attempt to clean in one go.
*/
const TInt KMaxPagesToClean = 4;

struct SVMCacheInfo;
class DMemModelThread;
class DMemoryMappingBase;

class DPager
	{
public:
	DPager();
	void InitCache();
	void Init3();

	FORCE_INLINE TBool CacheInitialised()
		{
		return iYoungOldRatio && iMinimumPageLimit;
		}

	FORCE_INLINE TUint NumberOfFreePages()
		{
		return iNumberOfFreePages;
		}

	FORCE_INLINE TUint NumberOfDirtyPages()
		{
		TUint ret;
		MmuLock::Lock();
		ret = iNumberOfDirtyPages;
		MmuLock::Unlock();
		return ret;
		}

	FORCE_INLINE TUint MinimumPageCount()
		{
		return iMinimumPageCount;
		}
	
	FORCE_INLINE void SetWritable(SPageInfo& aPageInfo)
		{
		if (!aPageInfo.IsDirty())
			{// This is the first mapping to write to the page so increase the 
			// dirty page count.
			iNumberOfDirtyPages++;
			}
		aPageInfo.SetWritable();
		}
	
	FORCE_INLINE void SetClean(SPageInfo& aPageInfo)
		{
		__NK_ASSERT_DEBUG(iNumberOfDirtyPages);
		__NK_ASSERT_DEBUG(aPageInfo.IsDirty());
		aPageInfo.SetClean();
		iNumberOfDirtyPages--;
		}

	/**
	Remove RAM pages from the cache and return them to the system's free pool.
	(Free them.)

	This is called by class Mmu when it requires more free RAM to meet an
	allocation request.

	@param	aNumPages The number of pages to free up.
	@return	True if all pages could be freed, false otherwise
	@pre	RamAlloc mutex held.
	*/
	TBool GetFreePages(TInt aNumPages);


	/**
	Attempts to rejuvenate or page in the page to the mapping that took the page fault.

	@param aPc					Address of instruction causing the fault.
	@param aFaultAddress		Address of memory access which faulted.
	@param aFaultAsid			The asid of the faulting thread's process.
	@param aAccessPermissions	Bitmask of values from enum TAccessPermissions, which
								indicates the permissions required by faulting memory access.
	@param aMapInstanceCount	The instance count of the mapping when it took the page fault.
	@param aThread				The thread that took the page fault.
	@param aExceptionInfo		The processor specific exception info.
	
	@return KErrNone if the page is now accessable, otherwise one of the system wide error codes.
	*/
	TInt HandlePageFault(	TLinAddr aPc, TLinAddr aFaultAddress, TUint aFaultAsid, TUint aFaultIndex,
							TUint aAccessPermissions, DMemoryObject* aMemory, DMemoryMapping* aMapping,
							TUint aMapInstanceCount, DThread* aThread, TAny* aExceptionInfo);


	/**
	Fault enumeration
	*/
	enum TFault
		{
		};

	/**
	Fault the system.
	*/
	static void Fault(TFault aFault);

	/**
	Get state of live page list.
	*/
	void GetLiveListInfo(SVMCacheInfo& aInfo);

	/**
	Resize the live page list.
	*/
	TInt ResizeLiveList(TUint aMinimumPageCount, TUint aMaximumPageCount);

	/**
	Recalculate live list size.
	*/
	TInt ResizeLiveList();

	/**
	Flush (unmap) all memory which is demand paged.
	This reduces the live page list to a minimum.
	*/
	void FlushAll();

	/**
	Give pages to paging system for managing.
	*/
	void DonatePages(TUint aCount, TPhysAddr* aPages);

	/**
	Reclaim pages from paging system which were previously donated with DonatePages.

	@param aCount Number of pages.
	@param aPages Array of pages (as stored in an RPageArray).

	@return KErrNone if successful.
			KErrNoMemory if paging system doesn't have enough spare pages. This will leave some or all of the pages still managed by the pager.
			KErrNotFound if some of the pages were not actually being managed by the pager.
	*/
	TInt ReclaimPages(TUint aCount, TPhysAddr* aPages);

	/**
	Called by class Mmu whenever a page of RAM is freed. The page state will be EUnused.
	If the page was being used by the pager then this gives it the opportunity to update
	any internal state. If the pager wishes to retain ownership of the page the it must
	return the result KErrNone, any other value will cause the page to be returned to the
	systems free pool.
	*/
	TInt PageFreed(SPageInfo* aPageInfo);

	//
	// following public members for use by memory managers...
	//

	/**
	Allocate a number of RAM pages to store demand paged content.
	These pages are obtained from...

	1. An unused page in the live page list.
	2. The systems free pool.
	3. The oldest page from the live page list.
	*/
	TInt PageInAllocPages(TPhysAddr* aPages, TUint aCount, Mmu::TRamAllocFlags aAllocFlags);

	/**
	Free a number of RAM pages allocated by PageInAllocPages.
	*/
	void PageInFreePages(TPhysAddr* aPages, TUint aCount);

	/**
	Called to add a new page to the live list after a fault has occurred.

	@param aPageInfo		The page.

	@pre MmuLock held
	@post MmuLock held (but may have been released by this function)
	*/
	void PagedIn(SPageInfo* aPageInfo);

	/**
	@param aPageInfo		The page.
	@param aPinArgs			Owner of a replacement page which will be used to substitute for the pinned page.

	@pre MmuLock held
	@post MmuLock held (but may have been released by this function)
	*/
	void PagedInPinned(SPageInfo* aPageInfo, TPinArgs& aPinArgs);

	/**
	@pre MmuLock held
	@post MmuLock left unchanged.
	*/
	void PagedInUnneeded(SPageInfo* aPageInfo);

	/**
	@param aPageInfo		The page to unpin.
	@param aPinArgs			The resources used for pinning. The replacement pages allocated
							to this will be increased for each page which was became completely
							unpinned.

	@pre MmuLock held
	@post MmuLock held (but may have been released by this function)
	*/
	void Unpin(SPageInfo* aPageInfo, TPinArgs& aPinArgs);

	/**
	@param aPageInfo		The page to pin. Must be page being demand paged.
	@param aPinArgs			Owner of a replacement page which will be used to substitute for the pinned page.

	@pre MmuLock held
	@post MmuLock held (but may have been released by this function)
	*/
	void Pin(SPageInfo* aPageInfo, TPinArgs& aPinArgs);


	/**
	@pre MmuLock held
	@post MmuLock held (but may have been released by this function)
	*/
	void RejuvenatePageTable(TPte* aPt);

	/**
	*/
	TBool ReservePages(TUint aRequiredCount, TUint& aCount);

	/**
	*/
	void UnreservePages(TUint& aCount);

	/**
	Indicates whether there are any dirty pages available to be cleaned by #CleanSomePages.

	This is called by the page cleaner to work out whether it has any work to do.

	@return Whether there are any dirty pages in the oldest section of the live list.
	*/
	TBool HasPagesToClean();

	/**
	Attempt to clean one or more dirty pages in one go.

	Called by the page cleaner to clean pages and by PageInAllocPage when needs to steal a page from
	the live list, but the oldest clean list is empty.

	May or may not succeed in acually cleaning any pages.

	@param aBackground Whether the activity should be ignored when determining whether the paging
	                   device is busy.  This is used by the page cleaner.

	@return The number of pages this method attempted to clean.  If it returns zero, there were no
	        pages eligible to be cleaned.
	*/
	TInt CleanSomePages(TBool aBackground);

	/**
	Enumeration of instrumented paging events which only require the
	SPageInfo object as an argument. 
	*/
	enum TEventSimple
		{
		EEventPageInNew,
		EEventPageInAgain,
		EEventPageInUnneeded,
		EEventPageInFree,
		EEventPageOut,
		EEventPageAged,
		EEventPagePin,
		EEventPageUnpin,
		EEventPageDonate,
		EEventPageReclaim,
		EEventPageAgedClean,
		EEventPageAgedDirty,
		EEventPagePageTableAlloc
		};

	/**
	Signal the occurrence of an event of type TEventSimple.
	*/
	void Event(TEventSimple aEvent, SPageInfo* aPageInfo);

	/**
	Enumeration of instrumented paging events which require the faulting address
	and program counter as arguments. 
	*/
	enum TEventWithAddresses
		{
		EEventPageInStart,
		EEventPageRejuvenate
		};

	/**
	Signal the occurrence of an event of type TEventWithAddresses.
	*/
	void Event(TEventWithAddresses aEvent, SPageInfo* aPageInfo, TLinAddr aPc, TLinAddr aFaultAddress, TUint aAccessPermissions);

	/**
	Get the pager's event info data.
	*/
	void GetEventInfo(SVMEventInfo& aInfoOut);

	/**
	Reset the pager's event info data.
	*/
	void ResetEventInfo();

	/**
	Attempt to discard the specified page.
	
	@param aOldPageInfo	The page info of the page to discard.
	@param aBlockZoneId	The ID of the RAM zone not to allocate any required new page into.
	@param aBlockRest	Set to ETrue when we don't want the allocator to search for new pages if the RAM 
						zone with ID==aBlockZoneId is encountered, i.e. a general RAM defrag operation.
	*/
	TInt DiscardPage(SPageInfo* aOldPageInfo, TUint aBlockZoneId, TBool aBlockRest);

	/**
	Attempt to discard the specified page and then allocate a page of type aPageType
	in its place.
	
	@param aPageInfo	The page info of the page to discard.
	@param aPageType 	The new page type to allocate into aPageInfo's physical address.
	*/
	TInt DiscardAndAllocPage(SPageInfo* aPageInfo, TZonePageType aPageType);


	/**
	Update any live list links to replace the old page with the new page.
	This is to be used when a page has been moved.

	@param aOldPageInfo	The page info of the page to replace.
	@param aNewPageInfo	The page info of the page to be used instead of the old page.
	*/
	void ReplacePage(SPageInfo& aOldPageInfo, SPageInfo& aNewPageInfo);


	//
	// following public members for use by TPinArgs...
	//

	/**
	*/
	TBool AllocPinReplacementPages(TUint aNumPages);

	/**
	*/
	void FreePinReplacementPages(TUint aNumPages);

private:
	/**
	Add a page to the head of the live page list. I.e. make it the 'youngest' page.

	@pre MmuLock held
	@post MmuLock left unchanged.
	*/
	void AddAsYoungestPage(SPageInfo* aPageInfo);

	/**
	Mark a page as type EUnused and add it to the end of the live page list.
	I.e. make it the 'oldest' page, so that it is the first page to be reused.

	@pre MmuLock held
	@post MmuLock left unchanged.
	*/
	void AddAsFreePage(SPageInfo* aPageInfo);

	/**
	Remove a page from live page list.
	It paged state is set to EUnpaged.

	@pre MmuLock held
	@post MmuLock left unchanged.
	*/
	void RemovePage(SPageInfo* aPageInfo);

	/**
	Try to remove the oldest page from the live page list and perform #StealPage.

	@param aPageInfoOut Set to the SPageInfo pointer for the stolen page if any.
	
	@return KErrNone on success, KErrInUse if stealing failed or 1 to indicate the the oldest page
	was dirty and the PageCleaning mutex was not held.
	
	@pre MmuLock held
	@post MmuLock left unchanged.
	*/
	TInt TryStealOldestPage(SPageInfo*& aPageInfoOut);

	/**
	Remove the oldest page from the live page list and perform #StealPage.

	@pre MmuLock held
	@post MmuLock held (but may have been released by this function)
	*/
	SPageInfo* StealOldestPage();

	/**
	Steal a page from the memory object (if any) which is using the page.
	If successful the returned page will be in the EUnknown state and the
	cache state for the page is indeterminate. This is the same state as
	if the page had been allocated by Mmu::AllocRam.

	@pre RamAlloc mutex held
	@pre If the page is dirty the PageCleaning lock must be held.
	@pre MmuLock held
	@post MmuLock held (but may have been released by this function)
	*/
	TInt StealPage(SPageInfo* aPageInfo);

	/**
	Restrict the access permissions for a page.

	@param aPageInfo	The page.
	@param aRestriction	The restriction type to apply.
	*/
	TInt RestrictPage(SPageInfo* aPageInfo, TRestrictPagesType aRestriction);

	/**
	Get a RAM page from the system's free pool and add it to the live list as a free page.

	@return False if out of memory;
			true otherwise, though new free page may still have already been used.

	@pre MmuLock held
	@post MmuLock held (but may have been released by this function)
	*/
	TBool TryGrowLiveList();

	/**
	Get a RAM page from the system's free pool.

 	@pre RamAllocLock held.

	@return The page or NULL if no page is available.
	*/
	SPageInfo* GetPageFromSystem(Mmu::TRamAllocFlags aAllocFlags, TUint aBlockZoneId=KRamZoneInvalidId, TBool aBlockRest=EFalse);

	/**
	Put a page back on the system's free pool.

	@pre RamAllocLock held.
	*/
	void ReturnPageToSystem();

	/**
	Put a specific page back on the system's free pool.

	@pre RamAllocLock held.
	*/
	void ReturnPageToSystem(SPageInfo& aPageInfo);

	/**
	Allocate a RAM page to store demand paged content.
	This tries to obtain a RAM from the following places:
	1. An unused page in the live page list.
	2. The systems free pool.
	3. The oldest page from the live page list.
	*/
	SPageInfo* PageInAllocPage(Mmu::TRamAllocFlags aAllocFlags);

	/**
	Called by CleanSomePages() to determine which pages should be cleaned.

	This deals with the complexity of page colouring, which means that pages can only be mapped at
	certain locations.  When cleaning multiple pages at once we need to find a set of pages that we
	can map in memory sequentially.

	@pre MmuLock held
	
	@param aPageInfosOut Pointer to an array of SPageInfo pointers, which must be at least
	KMaxPagesToClean long.  This will be filled in to indicate the pages to clean.
	
	@return The numnber of pages to clean.
	*/
	TInt SelectPagesToClean(SPageInfo** aPageInfosOut);

	/**
	If the number of young pages exceeds that specified by iYoungOldRatio then a
	single page is made 'old'. Call this after adding a new 'young' page.

	@pre MmuLock held
	@post MmuLock held (but may have been released by this function)
	*/
	void BalanceAges();

	/**
	If HaveTooManyPages() then return them to the system.
	*/
	void RemoveExcessPages();

	/**
	@return True if pager has too many pages, false otherwise.
	*/
	TBool HaveTooManyPages();

	/**
	@return True if pager has its maximum number of pages, false otherwise.
	*/
	TBool HaveMaximumPages();

	/**
	Attempt to rejuvenate a page in which a page fault occurred.

	@param aOsAsid 				Address space ID in which fault occurred.
	@param aAddress				Address of memory access which faulted.
	@param aAccessPermissions 	Bitmask of values from enum TAccessPermissions, which
								indicates the permissions required by faulting memory access.
	@param aPc				  	Address of instruction causing the fault. (Used for tracing.)
	@param aMapping				The mapping that maps the page that took the fault.
	@param aMapInstanceCount	The instance count of the mappig when the page fault occurred.
	@param aThread				The thread that took the page fault.
	@param aExceptionInfo		The processor specific exception info.
	
	@return KErrNone if the page was remapped, KErrAbort if the mapping has be reused or detached,
	KErrNotFound if it may be possible to page in the page.
	*/	
	TInt TryRejuvenate(	TInt aOsAsid, TLinAddr aAddress, TUint aAccessPermissions, TLinAddr aPc,
						DMemoryMappingBase* aMapping, TUint aMapInstanceCount, DThread* aThread, 
						TAny* aExceptionInfo);

	/**
	Reserve one page for guaranteed locking use.
	Increments iReservePageCount if successful.

	@return True if operation was successful.
	*/
	TBool ReservePage();

	/**
	Called when a realtime thread takes a paging fault.
	Checks whether it's OK for the thread to take to fault.
	@return KErrNone if the paging fault should be further processed
	*/
	TInt CheckRealtimeThreadFault(DThread* aThread, TAny* aExceptionInfo);
	
	/**
	Attempt to find the page table entry and page info for a page in the specified mapping.

	@param aOsAsid				The OsAsid of the process that owns the mapping.
	@param aAddress				The linear address of the page.
	@param aMapping				The mapping that maps the linear address.
	@param aMapInstanceCount	The instance count of the mapping.
	@param[out] aPte			Will return a pointer to the page table entry for the page.
	@param[out] aPageInfo		Will return a pointer to the page info for the page.

	@return KErrNone on success, KErrAbort when the mapping is now invalid, KErrNotFound when
	the page table or page info can't be found.
	*/
	TInt PteAndInfoFromLinAddr(	TInt aOsAsid, TLinAddr aAddress, DMemoryMappingBase* aMapping, 
								TUint aMapInstanceCount, TPte*& aPte, SPageInfo*& aPageInfo);
#ifdef _DEBUG
	/**
	Check consistency of live list.
	*/
	TBool CheckLists();

	void TraceCounts();
#endif

private:
	TUint iMinYoungPages;		///< Minimum number of young pages in live list required for correct functioning.
	TUint iAbsoluteMinPageCount;///< Absolute minimum number of pages in live to meet algorithm constraints
private:
	TUint iMinimumPageCount;	/**< Minimum size for the live page list, including locked pages */
	TUint iMaximumPageCount;	/**< Maximum size for the live page list, including locked pages */
	TUint16 iYoungOldRatio;		/**< Ratio of young to old pages in the live page list */
	SDblQue iYoungList;			/**< Head of 'young' page list. */
	TUint iYoungCount;			/**< Number of young pages */
	SDblQue iOldList;			/**< Head of 'old' page list. */
	TUint iOldCount;			/**< Number of old pages */
	SDblQue iOldestCleanList;	/**< Head of 'oldestClean' page list. */
	TUint iOldestCleanCount;	/**< Number of 'oldestClean' pages */
	SDblQue iOldestDirtyList;	/**< Head of 'oldestDirty' page list. */
	TUint iOldestDirtyCount;	/**< Number of 'oldestDirty' pages */
	TUint16 iOldOldestRatio;	/**< Ratio of old pages to oldest to clean and dirty in the live page list*/
	TUint iNumberOfFreePages;
	TUint iNumberOfDirtyPages;	/**< The total number of dirty pages in the paging cache. Protected by MmuLock */
	TUint iInitMinimumPageCount;/**< Initial value for iMinimumPageCount */
	TUint iInitMaximumPageCount;/**< Initial value for iMaximumPageCount  */
	TUint iReservePageCount;	/**< Number of pages reserved for locking */
	TUint iMinimumPageLimit;	/**< Minimum size for iMinimumPageCount, not including locked pages.
								     iMinimumPageCount >= iMinimumPageLimit + iReservePageCount */
	SVMEventInfo iEventInfo;

#ifdef __DEMAND_PAGING_BENCHMARKS__
public:
	void RecordBenchmarkData(TPagingBenchmark aBm, TUint32 aStartTime, TUint32 aEndTime, TUint aCount);
	void ResetBenchmarkData(TPagingBenchmark aBm);
	void ReadBenchmarkData(TPagingBenchmark aBm, SPagingBenchmarkInfo& aDataOut);
	TSpinLock iBenchmarkLock;
	SPagingBenchmarkInfo iBenchmarkInfo[EMaxPagingBm];
#endif //__DEMAND_PAGING_BENCHMARKS__
	};

extern DPager ThePager;


#ifdef __DEMAND_PAGING_BENCHMARKS__

#define START_PAGING_BENCHMARK TUint32 _bmStart = NKern::FastCounter()
#define END_PAGING_BENCHMARK(bm) ThePager.RecordBenchmarkData(bm, _bmStart, NKern::FastCounter(), 1)
#define END_PAGING_BENCHMARK_N(bm, n) ThePager.RecordBenchmarkData(bm, _bmStart, NKern::FastCounter(), (n))

#else

#define START_PAGING_BENCHMARK
#define END_PAGING_BENCHMARK(bm)
#define END_PAGING_BENCHMARK_N(bm, n)
#endif // __DEMAND_PAGING_BENCHMARKS__


FORCE_INLINE void DPager::Event(TEventSimple aEvent, SPageInfo* aPageInfo)
	{
	switch(aEvent)
		{
	case EEventPageInNew:
		TRACEP(("DP: %O PageIn 0x%08x",TheCurrentThread,aPageInfo->PhysAddr()));
		#ifdef BTRACE_PAGING
			BTraceContext12(BTrace::EPaging,BTrace::EPagingPageIn,aPageInfo->PhysAddr(),aPageInfo->Owner(),aPageInfo->Index());
		#endif
		++iEventInfo.iPageInReadCount;
		break;

	case EEventPageInAgain:
		TRACEP(("DP: %O PageIn (again) 0x%08x",TheCurrentThread,aPageInfo->PhysAddr()));
		#ifdef BTRACE_PAGING
			BTraceContext4(BTrace::EPaging,BTrace::EPagingMapPage,aPageInfo->PhysAddr());
		#endif
		break;

	case EEventPageInUnneeded:
		TRACEP(("DP: %O PageIn (unneeded) 0x%08x",TheCurrentThread,aPageInfo->PhysAddr()));
		#ifdef BTRACE_PAGING
			BTraceContext0(BTrace::EPaging,BTrace::EPagingPageInUnneeded);
		#endif
		break;

	case EEventPageInFree:
		TRACEP(("DP: %O PageInFree 0x%08x",TheCurrentThread,aPageInfo->PhysAddr()));
		#ifdef BTRACE_PAGING
			BTraceContext4(BTrace::EPaging,BTrace::EPagingPageInFree,aPageInfo->PhysAddr());
		#endif
		break;

	case EEventPageOut:
		TRACEP(("DP: %O PageOut 0x%08x",TheCurrentThread,aPageInfo->PhysAddr()));
		#ifdef BTRACE_PAGING
			BTraceContext4(BTrace::EPaging,BTrace::EPagingPageOut,aPageInfo->PhysAddr());
		#endif
		break;

	case EEventPageAged:
		TRACEP(("DP: %O Aged 0x%08x",TheCurrentThread,aPageInfo->PhysAddr()));
		#ifdef BTRACE_PAGING_VERBOSE
			BTraceContext4(BTrace::EPaging,BTrace::EPagingAged,aPageInfo->PhysAddr());
		#endif
		break;

	case EEventPagePin:
		TRACEP(("DP: %O Pin 0x%08x count=%d",TheCurrentThread,aPageInfo->PhysAddr(),aPageInfo->PinCount()));
		#ifdef BTRACE_PAGING
			BTraceContext8(BTrace::EPaging,BTrace::EPagingPageLock,aPageInfo->PhysAddr(),aPageInfo->PinCount());
		#endif
		break;

	case EEventPageUnpin:
		TRACEP(("DP: %O Unpin 0x%08x count=%d",TheCurrentThread,aPageInfo->PhysAddr(),aPageInfo->PinCount()));
		#ifdef BTRACE_PAGING
			BTraceContext8(BTrace::EPaging,BTrace::EPagingPageUnlock,aPageInfo->PhysAddr(),aPageInfo->PinCount());
		#endif
		break;

	case EEventPageDonate:
		TRACEP(("DP: %O Donate 0x%08x",TheCurrentThread,aPageInfo->PhysAddr()));
		#ifdef BTRACE_PAGING
			BTraceContext12(BTrace::EPaging,BTrace::EPagingDonatePage,aPageInfo->PhysAddr(),aPageInfo->Owner(),aPageInfo->Index());
		#endif
		break;

	case EEventPageReclaim:
		TRACEP(("DP: %O Reclaim 0x%08x",TheCurrentThread,aPageInfo->PhysAddr()));
		#ifdef BTRACE_PAGING
			BTraceContext4(BTrace::EPaging,BTrace::EPagingReclaimPage,aPageInfo->PhysAddr());
		#endif
		break;

	case EEventPageAgedClean:
		TRACEP(("DP: %O AgedClean 0x%08x",TheCurrentThread,aPageInfo->PhysAddr()));
		#ifdef BTRACE_PAGING_VERBOSE
			BTraceContext4(BTrace::EPaging,BTrace::EPagingAgedClean,aPageInfo->PhysAddr());
		#endif
		break;

	case EEventPageAgedDirty:
		TRACEP(("DP: %O AgedDirty 0x%08x",TheCurrentThread,aPageInfo->PhysAddr()));
		#ifdef BTRACE_PAGING_VERBOSE
			BTraceContext4(BTrace::EPaging,BTrace::EPagingAgedDirty,aPageInfo->PhysAddr());
		#endif
		break;

	case EEventPagePageTableAlloc:
		TRACEP(("DP: %O PageTableAlloc 0x%08x",TheCurrentThread,aPageInfo->PhysAddr()));
		#ifdef BTRACE_PAGING
			BTraceContext4(BTrace::EPaging,BTrace::EPagingPageTableAlloc,aPageInfo->PhysAddr());
		#endif
		break;

	default:
		__NK_ASSERT_DEBUG(0);
		break;
		}
	}



FORCE_INLINE void DPager::Event(TEventWithAddresses aEvent, SPageInfo* aPageInfo, TLinAddr aPc, TLinAddr aFaultAddress, TUint aAccessPermissions)
	{
	switch(aEvent)
		{
	case EEventPageInStart:
		TRACEP(("DP: %O HandlePageFault 0x%08x 0x%08x %d",TheCurrentThread,aFaultAddress,aPc,aAccessPermissions));
		#ifdef BTRACE_PAGING
			BTraceContext12(BTrace::EPaging,BTrace::EPagingPageInBegin,aFaultAddress,aPc,aAccessPermissions);
		#endif
		++iEventInfo.iPageFaultCount;
		break;

	case EEventPageRejuvenate:
		TRACEP(("DP: %O Rejuvenate 0x%08x 0x%08x 0x%08x %d",TheCurrentThread,aPageInfo->PhysAddr(),aFaultAddress,aPc,aAccessPermissions));
		#ifdef BTRACE_PAGING
			BTraceContext12(BTrace::EPaging,BTrace::EPagingRejuvenate,aPageInfo->PhysAddr(),aFaultAddress,aPc);
		#endif
		++iEventInfo.iPageFaultCount;
		break;

	default:
		__NK_ASSERT_DEBUG(0);
		break;
		}
	}



/**
Multiplier for number of request objects in pool per drive that supports paging.
*/
const TInt KPagingRequestsPerDevice = 2;


class DPoolPagingRequest;
class DPageReadRequest;
class DPageWriteRequest;

/**
A pool of paging requests for use by a single paging device.
*/
class DPagingRequestPool : public DBase
	{
public:
	DPagingRequestPool(TUint aNumPageReadRequest, TBool aWriteRequest);
	DPageReadRequest* AcquirePageReadRequest(DMemoryObject* aMemory, TUint aIndex, TUint aCount);
	DPageWriteRequest* AcquirePageWriteRequest(DMemoryObject** aMemory, TUint* aIndex, TUint aCount);
private:
	~DPagingRequestPool();
private:
	class TGroup
		{
	public:
		TGroup(TUint aNumRequests);
		DPoolPagingRequest* FindCollisionContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount);
		DPoolPagingRequest* GetRequest(DMemoryObject* aMemory, TUint aIndex, TUint aCount);
		void Signal(DPoolPagingRequest* aRequest);
	public:
		TUint iNumRequests;
		DPoolPagingRequest** iRequests;
		SDblQue iFreeList;
		};
	TGroup iPageReadRequests;
	DPageWriteRequest* iPageWriteRequest;

	friend class DPoolPagingRequest;
	friend class DPageReadRequest;
	friend class DPageWriteRequest;
	};


/**
Resources needed to service a paging request.
*/
class DPagingRequest : public SDblQueLink
	{
public:
	enum
		{
		EMaxPages = 4
		};
	DPagingRequest();
	TLinAddr MapPages(TUint aColour, TUint aCount, TPhysAddr* aPages);
	void UnmapPages(TBool aIMBRequired);
	void SetUseContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount);
	void SetUseDiscontiguous(DMemoryObject** aMemory, TUint* aIndex, TUint aCount);
	void ResetUse();
	TBool CheckUseContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount);
	TBool CheckUseDiscontiguous(DMemoryObject** aMemory, TUint* aIndex, TUint aCount);
	TBool IsCollisionContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount);
public:
	DMutex*			iMutex;		/**< A mutex for synchronisation and priority inheritance. */
protected:
	Mmu::TTempMapping	iTempMapping;
private:
	// used to identify memory request is used for...
	TUint			iUseRegionCount;
	DMemoryObject*	iUseRegionMemory[EMaxPages];
	TUint			iUseRegionIndex[EMaxPages];
	};


__ASSERT_COMPILE(DPagingRequest::EMaxPages >= KMaxPagesToClean);


/**
A paging request that is part of a pool of similar request objects.
*/
class DPoolPagingRequest : public DPagingRequest
	{
public:
	DPoolPagingRequest(DPagingRequestPool::TGroup& aPoolGroup);
 	void Release();
	void Wait();
	void Signal();
public:
	TInt			iUsageCount;	/**< How many threads are using or waiting for this object. */	
private:
	DPagingRequestPool::TGroup& iPoolGroup;
	};

	
/**
Resources needed to service a page in request.
*/
class DPageReadRequest : public DPoolPagingRequest
	{
public:
	DPageReadRequest(DPagingRequestPool::TGroup& aPoolGroup);
	TInt Construct();
	static TUint ReservedPagesRequired();
private:
	~DPageReadRequest(); // can't delete
public:
	TLinAddr		iBuffer;	/**< A buffer to read compressed data into. Size is EMaxPages+1 pages.*/
private:
	DMemoryObject*	iMemory;
private:
	static TInt iAllocNext;
	};


FORCE_INLINE TUint DPageReadRequest::ReservedPagesRequired()
	{
	return iAllocNext*EMaxPages;
	}


/**
Resources needed to service a page out request.
*/
class DPageWriteRequest : public DPagingRequest
	{
public:
	DPageWriteRequest();
 	void Release();
private:
	~DPageWriteRequest(); // can't delete
	};


/**
Class providing access to the mutex used to protect page cleaning operations;
this is the mutex DPager::iPageCleaningLock.
*/
class PageCleaningLock
	{
public:
	/**
	Acquire the lock.
	The lock may be acquired multiple times by a thread, and will remain locked
	until #Unlock has been used enough times to balance this.
	*/
	static void Lock();

	/**
	Release the lock.

	@pre The current thread has previously acquired the lock.
	*/
	static void Unlock();

	/**
	Return true if the current thread holds the lock.
	This is used for debug checks.
	*/
	static TBool IsHeld();

	/**
	Create the lock.
	Called by DPager::Init3().
	*/
	static void Init();	
	};


#endif
