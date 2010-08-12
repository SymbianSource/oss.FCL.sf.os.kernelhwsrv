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

#include "memmodel.h"
#include "mm.h"
#include "mmu.h"

#include "mpager.h"
#include "mrom.h"
#include "mobject.h"
#include "mmapping.h"
#include "maddressspace.h"
#include "mmanager.h"
#include "mptalloc.h"
#include "mpagearray.h"
#include "mswap.h"
#include "mthrash.h"
#include "mpagecleaner.h"

#include "cache_maintenance.inl"


const TUint16 KDefaultYoungOldRatio = 3;
const TUint16 KDefaultMinPages = 256;
const TUint16 KDefaultOldOldestRatio = 3;

const TUint KMinOldPages = 1;

/*	On a 32 bit system without PAE can't have more than 2^(32-KPageShift) pages.
 *	Subtract 1 so it doesn't overflow when converted to bytes.
*/
const TUint	KAbsoluteMaxPageCount = (1u<<(32-KPageShift))-1u;

/**
Default limit for the maximum number of oldest pages.

If the data paging device sets iPreferredWriteShift, then this is increased if necessary to allow
that many pages to be present.

This limit exists to make our live list implementation a closer approximation to LRU, and to bound
the time taken by SelectSequentialPagesToClean(), which is called with the MmuLock held.
*/
const TUint KDefaultMaxOldestPages = 32;

static DMutex* ThePageCleaningLock = NULL;

DPager ThePager;


DPager::DPager() :
	iMinimumPageCount(0),
	iMaximumPageCount(0),
	iYoungOldRatio(0),
	iYoungCount(0),
	iOldCount(0),
	iOldestCleanCount(0),
	iMaxOldestPages(KDefaultMaxOldestPages),
	iNumberOfFreePages(0),
	iReservePageCount(0),
	iMinimumPageLimit(0),
	iPagesToClean(1)
#ifdef __DEMAND_PAGING_BENCHMARKS__
	, iBenchmarkLock(TSpinLock::EOrderGenericIrqHigh3)
#endif	  
	{
	}


void DPager::InitCache()
	{
	//
	// This routine doesn't acquire any mutexes because it should be called before the system
	// is fully up and running. I.e. called before another thread can preempt this.
	//
	TRACEB(("DPager::InitCache()"));
	// If any pages have been reserved then they will have already been allocated and 
	// therefore should be counted as part of iMinimumPageCount.
	__NK_ASSERT_DEBUG(iReservePageCount == iMinimumPageCount);
	__NK_ASSERT_DEBUG(!CacheInitialised());

#if defined(__CPU_ARM)

/** Minimum number of young pages the demand paging live list may have.
	Need at least 4 mapped pages to guarantee to be able to execute all ARM instructions,
	plus enough pages for 4 page tables to map those pages, plus enough pages for the
	page table info structures of those page tables.
	(Worst case is a Thumb-2 STM instruction with both instruction and data straddling chunk
	boundaries.)
*/
	iMinYoungPages = 4											// pages
					+(4+KPtClusterSize-1)/KPtClusterSize		// page table pages
					+(4+KPageTableInfosPerPage-1)/KPageTableInfosPerPage;	// page table info pages

#elif defined(__CPU_X86)

/*	Need at least 6 mapped pages to guarantee to be able to execute all X86 instructions,
	plus enough pages for 6 page tables to map those pages, plus enough pages for the
	page table info structures of those page tables.
	(Worst case is (?) a MOV [X],[Y] instruction with instruction, 'X' and 'Y' all
	straddling chunk boundaries.)
*/
	iMinYoungPages = 6											// pages
					+(6+KPtClusterSize-1)/KPtClusterSize		// page table pages
					+(6+KPageTableInfosPerPage-1)/KPageTableInfosPerPage;	// page table info pages

#else
#error Unknown CPU
#endif

#ifdef __SMP__
	// Adjust min page count so that all CPUs are guaranteed to make progress.
	TInt numberOfCpus = NKern::NumberOfCpus();
	iMinYoungPages *= numberOfCpus;
#endif

	// A minimum young/old ratio of 1 means that we need at least twice iMinYoungPages pages...
	iAbsoluteMinPageCount = 2*iMinYoungPages;

	__NK_ASSERT_DEBUG(KMinOldPages<=iAbsoluteMinPageCount/2);

	// Read any paging config data.
	SDemandPagingConfig config = TheRomHeader().iDemandPagingConfig;

	// Set the list ratios...
	iYoungOldRatio = KDefaultYoungOldRatio;
	if(config.iYoungOldRatio)
		iYoungOldRatio = config.iYoungOldRatio;
	iOldOldestRatio = KDefaultOldOldestRatio;
	if(config.iSpare[2])
		iOldOldestRatio = config.iSpare[2];

	// Set the minimum page counts...
	iMinimumPageLimit = iMinYoungPages * (1 + iYoungOldRatio) / iYoungOldRatio
									   + DPageReadRequest::ReservedPagesRequired();
	
	if(iMinimumPageLimit < iAbsoluteMinPageCount)
		iMinimumPageLimit = iAbsoluteMinPageCount;

	if (K::MemModelAttributes & (EMemModelAttrRomPaging | EMemModelAttrCodePaging | EMemModelAttrDataPaging))
	    iMinimumPageCount = KDefaultMinPages; 
	else
		{// No paging is enabled so set the minimum cache size to the minimum
		// allowable with the current young old ratio.
	    iMinimumPageCount = iMinYoungPages * (iYoungOldRatio + 1);
		}

	if(config.iMinPages)
		iMinimumPageCount = config.iMinPages;
	if(iMinimumPageCount < iAbsoluteMinPageCount)
		iMinimumPageCount = iAbsoluteMinPageCount;
	if (iMinimumPageLimit + iReservePageCount > iMinimumPageCount)
		iMinimumPageCount = iMinimumPageLimit + iReservePageCount;

	iInitMinimumPageCount = iMinimumPageCount;

	// Set the maximum page counts...
	iMaximumPageCount = KMaxTInt;
	if(config.iMaxPages)
		iMaximumPageCount = config.iMaxPages;
	if (iMaximumPageCount > KAbsoluteMaxPageCount)
		iMaximumPageCount = KAbsoluteMaxPageCount;
	iInitMaximumPageCount = iMaximumPageCount;

	TRACEB(("DPager::InitCache() live list min=%d max=%d ratio=%d",iMinimumPageCount,iMaximumPageCount,iYoungOldRatio));

	// Verify the page counts are valid.
	__NK_ASSERT_ALWAYS(iMaximumPageCount >= iMinimumPageCount);
	TUint minOldAndOldest = iMinimumPageCount / (1 + iYoungOldRatio);
	__NK_ASSERT_ALWAYS(minOldAndOldest >= KMinOldPages);
	__NK_ASSERT_ALWAYS(iMinimumPageCount >= minOldAndOldest);

	// Need at least iMinYoungPages pages mapped to execute worst case CPU instruction
	TUint minYoung = iMinimumPageCount - minOldAndOldest;
	__NK_ASSERT_ALWAYS(minYoung >= iMinYoungPages);

	// Verify that the young old ratio can be met even when there is only the 
	// minimum number of old pages.
	TInt ratioLimit = (iMinimumPageCount-KMinOldPages)/KMinOldPages;
	__NK_ASSERT_ALWAYS(iYoungOldRatio <= ratioLimit);

	// There should always be enough old pages to allow the oldest lists ratio.
	TUint oldestCount = minOldAndOldest / (1 + iOldOldestRatio);
	__NK_ASSERT_ALWAYS(oldestCount);

	iNumberOfFreePages = 0;
	iNumberOfDirtyPages = 0;

	// Allocate RAM pages and put them all on the old list.
	// Reserved pages have already been allocated and already placed on the
	// old list so don't allocate them again.
	RamAllocLock::Lock();
	iYoungCount = 0;
	iOldCount = 0;
	iOldestDirtyCount = 0;
	__NK_ASSERT_DEBUG(iOldestCleanCount == iReservePageCount);
	Mmu& m = TheMmu;
	for(TUint i = iReservePageCount; i < iMinimumPageCount; i++)
		{
		// Allocate a single page
		TPhysAddr pagePhys;
		TInt r = m.AllocRam(&pagePhys, 1, 
							(Mmu::TRamAllocFlags)(EMemAttNormalCached|Mmu::EAllocNoWipe|Mmu::EAllocNoPagerReclaim), 
							EPageDiscard);
		__NK_ASSERT_ALWAYS(r == KErrNone);
		MmuLock::Lock();
		AddAsFreePage(SPageInfo::FromPhysAddr(pagePhys));
		MmuLock::Unlock();
		}
	RamAllocLock::Unlock();

	__NK_ASSERT_DEBUG(CacheInitialised());
	TRACEB(("DPager::InitCache() end with young=%d old=%d oldClean=%d oldDirty=%d min=%d free=%d max=%d",iYoungCount,iOldCount,iOldestCleanCount,iOldestDirtyCount,iMinimumPageCount,iNumberOfFreePages,iMaximumPageCount));
	}


#ifdef _DEBUG
#ifdef FMM_PAGER_CHECK_LISTS
TBool CheckList(SDblQueLink* aHead, TUint aCount)
	{
	SDblQueLink* link = aHead;
	while(aCount--)
		{
		link = link->iNext;
		if(link == aHead)
			return EFalse;
		}
	link = link->iNext;
	if(link != aHead)
		return EFalse;
	return ETrue;
	}
#endif // #ifdef FMM_PAGER_CHECK_LISTS

TBool DPager::CheckLists()
	{
#ifdef FMM_PAGER_CHECK_LISTS
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	if (!CheckList(&iOldList.iA, iOldCount))
		return EFalse;
	if (!CheckList(&iYoungList.iA, iYoungCount))
		return EFalse;
	if (!CheckList(&iOldestCleanList.iA, iOldestCleanCount))
		return EFalse;
	if (!CheckList(&iOldestDirtyList.iA, iOldestDirtyCount))
		return EFalse;
	TRACEP(("DP: y=%d o=%d oc=%d od=%d f=%d", iYoungCount, iOldCount, 
			iOldestCleanCount, iOldestDirtyCount, iNumberOfFreePages));
	TraceCounts();
#endif // #ifdef FMM_PAGER_CHECK_LISTS
	return true;
	}

void DPager::TraceCounts()
	{
	TRACEP(("DP: y=%d o=%d oc=%d od=%d f=%d min=%d max=%d ml=%d res=%d",
		iYoungCount, iOldCount, iOldestCleanCount, iOldestDirtyCount, 
		iNumberOfFreePages, iMinimumPageCount, iMaximumPageCount,
		iMinimumPageLimit, iReservePageCount));
	}
#endif //#ifdef _DEBUG


TBool DPager::HaveTooManyPages()
	{
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	return iMinimumPageCount+iNumberOfFreePages > iMaximumPageCount;
	}


TBool DPager::HaveMaximumPages()
	{
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	return iMinimumPageCount+iNumberOfFreePages >= iMaximumPageCount;
	}


void DPager::AddAsYoungestPage(SPageInfo* aPageInfo)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(CheckLists());
	__NK_ASSERT_DEBUG(aPageInfo->PagedState()==SPageInfo::EUnpaged);

	aPageInfo->SetPagedState(SPageInfo::EPagedYoung);
	iYoungList.AddHead(&aPageInfo->iLink);
	++iYoungCount;
	}


void DPager::AddAsFreePage(SPageInfo* aPageInfo)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(CheckLists());

	__NK_ASSERT_DEBUG(aPageInfo->PagedState()==SPageInfo::EUnpaged);
	TheMmu.PageFreed(aPageInfo);
	__NK_ASSERT_DEBUG(aPageInfo->PagedState()==SPageInfo::EUnpaged);

	// add as oldest page...
	aPageInfo->SetPagedState(SPageInfo::EPagedOldestClean);
	iOldestCleanList.Add(&aPageInfo->iLink);
	++iOldestCleanCount;

	Event(EEventPageInFree,aPageInfo);
	}


TInt DPager::PageFreed(SPageInfo* aPageInfo)
	{
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(CheckLists());

	switch(aPageInfo->PagedState())
		{
	case SPageInfo::EUnpaged:
		return KErrNotFound;

	case SPageInfo::EPagedYoung:
		__NK_ASSERT_DEBUG(iYoungCount);
		aPageInfo->iLink.Deque();
		--iYoungCount;
		break;

	case SPageInfo::EPagedOld:
		__NK_ASSERT_DEBUG(iOldCount);
		aPageInfo->iLink.Deque();
		--iOldCount;
		break;

	case SPageInfo::EPagedOldestClean:
		__NK_ASSERT_DEBUG(iOldestCleanCount);
		aPageInfo->iLink.Deque();
		--iOldestCleanCount;
		break;

	case SPageInfo::EPagedOldestDirty:
		__NK_ASSERT_DEBUG(iOldestDirtyCount);
		aPageInfo->iLink.Deque();
		--iOldestDirtyCount;
		break;

	case SPageInfo::EPagedPinned:
		// this can occur if a pinned mapping is being unmapped when memory is decommitted.
		// the decommit will have succeeded because the the mapping no longer vetoes this,
		// however the unpinning hasn't yet got around to changing the page state.
		// When the state change happens the page will be put back on the live list so
		// we don't have to do anything now...
		return KErrNone;

	case SPageInfo::EPagedPinnedMoved:
		// This page was pinned when it was moved but it has not been returned 
		// to the free pool yet so make sure it is...
		aPageInfo->SetPagedState(SPageInfo::EUnpaged);	// Must be unpaged before returned to free pool.
		return KErrCompletion;

	default:
		__NK_ASSERT_DEBUG(0);
		return KErrNotFound;
		}

	// Update the dirty page count as required...
	if (aPageInfo->IsDirty())
		{
		aPageInfo->SetReadOnly();
		SetClean(*aPageInfo);
		}

	if (iNumberOfFreePages > 0)
		{// The paging cache is not at the minimum size so safe to let the 
		// ram allocator free this page.
		iNumberOfFreePages--;
		aPageInfo->SetPagedState(SPageInfo::EUnpaged);
		return KErrCompletion;
		}
	// Need to hold onto this page as have reached the page cache limit.
	// add as oldest page...
	aPageInfo->SetPagedState(SPageInfo::EPagedOldestClean);
	iOldestCleanList.Add(&aPageInfo->iLink);
	++iOldestCleanCount;

	return KErrNone;
	}


extern TBool IsPageTableUnpagedRemoveAllowed(SPageInfo* aPageInfo);

void DPager::RemovePage(SPageInfo* aPageInfo)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(CheckLists());

	switch(aPageInfo->PagedState())
		{
	case SPageInfo::EPagedYoung:
		__NK_ASSERT_DEBUG(iYoungCount);
		aPageInfo->iLink.Deque();
		--iYoungCount;
		break;

	case SPageInfo::EPagedOld:
		__NK_ASSERT_DEBUG(iOldCount);
		aPageInfo->iLink.Deque();
		--iOldCount;
		break;

	case SPageInfo::EPagedOldestClean:
		__NK_ASSERT_DEBUG(iOldestCleanCount);
		aPageInfo->iLink.Deque();
		--iOldestCleanCount;
		break;

	case SPageInfo::EPagedOldestDirty:
		__NK_ASSERT_DEBUG(iOldestDirtyCount);
		aPageInfo->iLink.Deque();
		--iOldestDirtyCount;
		break;

	case SPageInfo::EPagedPinned:
		__NK_ASSERT_DEBUG(0);
	case SPageInfo::EUnpaged:
#ifdef _DEBUG
		if (!IsPageTableUnpagedRemoveAllowed(aPageInfo))
			__NK_ASSERT_DEBUG(0);
#endif
		break;
	default:
		__NK_ASSERT_DEBUG(0);
		return;
		}

	aPageInfo->SetPagedState(SPageInfo::EUnpaged);
	}


void DPager::ReplacePage(SPageInfo& aOldPageInfo, SPageInfo& aNewPageInfo)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(CheckLists());

	__NK_ASSERT_DEBUG(aOldPageInfo.PagedState() == aNewPageInfo.PagedState());
	switch(aOldPageInfo.PagedState())
		{
		case SPageInfo::EPagedYoung:
		case SPageInfo::EPagedOld:
		case SPageInfo::EPagedOldestClean:
		case SPageInfo::EPagedOldestDirty:
			{// Update the list links to point to the new page.
			SDblQueLink* prevLink = aOldPageInfo.iLink.iPrev;
#ifdef _DEBUG
			SDblQueLink* nextLink = aOldPageInfo.iLink.iNext;
			__NK_ASSERT_DEBUG(prevLink == aOldPageInfo.iLink.iPrev);
			__NK_ASSERT_DEBUG(prevLink->iNext == &aOldPageInfo.iLink);
			__NK_ASSERT_DEBUG(nextLink == aOldPageInfo.iLink.iNext);
			__NK_ASSERT_DEBUG(nextLink->iPrev == &aOldPageInfo.iLink);
#endif
			aOldPageInfo.iLink.Deque();
			aNewPageInfo.iLink.InsertAfter(prevLink);
			aOldPageInfo.SetPagedState(SPageInfo::EUnpaged);
#ifdef _DEBUG
			__NK_ASSERT_DEBUG(prevLink == aNewPageInfo.iLink.iPrev);
			__NK_ASSERT_DEBUG(prevLink->iNext == &aNewPageInfo.iLink);
			__NK_ASSERT_DEBUG(nextLink == aNewPageInfo.iLink.iNext);
			__NK_ASSERT_DEBUG(nextLink->iPrev == &aNewPageInfo.iLink);
#endif
			}
			break;
		case SPageInfo::EPagedPinned:
			// Mark the page as 'pinned moved' so that when the page moving invokes 
			// Mmu::FreeRam() it returns this page to the free pool.
			aOldPageInfo.ClearPinCount();
			aOldPageInfo.SetPagedState(SPageInfo::EPagedPinnedMoved);
			break;
		case SPageInfo::EPagedPinnedMoved:
			// Shouldn't happen as the ram alloc mutex will be held for the 
			// entire time the page's is paged state == EPagedPinnedMoved.
		case SPageInfo::EUnpaged:
			// Shouldn't happen as we only move pinned memory and unpinning will 
			// atomically add the page to the live list and it can't be removed 
			// from the live list without the ram alloc mutex.
			__NK_ASSERT_DEBUG(0);
			break;
		}	
	}


SPageInfo* DPager::StealOrAllocPage(TBool aAllowAlloc, Mmu::TRamAllocFlags aAllocFlags)
	{
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	// The PageCleaningLock may or may not be held to start with
	TBool pageCleaningLockAcquired = EFalse;

	SDblQueLink* link;
	SPageInfo* pageInfo ;
	
restart:

	// if there is a free page in the live list then use that (it will be at the end)...
	if (iOldestCleanCount)
		{
		__NK_ASSERT_DEBUG(!iOldestCleanList.IsEmpty());
		link = iOldestCleanList.Last();
		pageInfo = SPageInfo::FromLink(link);
		if(pageInfo->Type()==SPageInfo::EUnused)
			goto try_steal_from_page_info;
		}
	
	// maybe try getting a free page from the system pool...
	if (aAllowAlloc && !HaveMaximumPages())
		{
		MmuLock::Unlock();
		pageInfo = GetPageFromSystem(aAllocFlags);
		MmuLock::Lock();
		if (pageInfo)
			goto exit;
		}
	
	// try stealing the oldest clean page on the  live list if there is one...
	if (iOldestCleanCount)
		{
		__NK_ASSERT_DEBUG(!iOldestCleanList.IsEmpty());
		link = iOldestCleanList.Last();
		goto try_steal_from_link;
		}

	// no clean oldest pages, see if we can clean multiple dirty pages in one go...
	if (iOldestDirtyCount > 1 && iPagesToClean > 1)
		{
		__NK_ASSERT_DEBUG(!iOldestDirtyList.IsEmpty());

		// check if we hold page cleaning lock
		TBool needPageCleaningLock = !PageCleaningLock::IsHeld();
		if (needPageCleaningLock)
			{
			// temporarily release ram alloc mutex and acquire page cleaning mutex
			MmuLock::Unlock();
			RamAllocLock::Unlock();
			PageCleaningLock::Lock();
			MmuLock::Lock();
			}

		// there may be clean pages now if we've waited on the page cleaning mutex, if so don't
		// bother cleaning but just restart
		if (iOldestCleanCount == 0 && iOldestDirtyCount >= 1)
			CleanSomePages(EFalse);

		if (needPageCleaningLock)
			{
			// release page cleaning mutex and re-aquire ram alloc mutex
			MmuLock::Unlock();
			PageCleaningLock::Unlock();			
			RamAllocLock::Lock();
			MmuLock::Lock();
			}

		// if there are now some clean pages we restart so as to take one of them
		if (iOldestCleanCount > 0)
			goto restart;
		}

	// otherwise just try to steal the oldest page...
	if (iOldestDirtyCount)
		{
		__NK_ASSERT_DEBUG(!iOldestDirtyList.IsEmpty());
		link = iOldestDirtyList.Last();
		}
	else if (iOldCount)
		{
		__NK_ASSERT_DEBUG(!iOldList.IsEmpty());
		link = iOldList.Last();
		}
	else
		{
		__NK_ASSERT_DEBUG(iYoungCount);
		__NK_ASSERT_ALWAYS(!iYoungList.IsEmpty());
		link = iYoungList.Last();
		}

try_steal_from_link:

	// lookup page info
	__NK_ASSERT_DEBUG(link);
	pageInfo = SPageInfo::FromLink(link);
	
try_steal_from_page_info:
	
	// if the page is dirty and we don't hold the page cleaning mutex then we have to wait on it,
	// and restart - we clean with the ram alloc mutex held in this case
	if (pageInfo->IsDirty() && !PageCleaningLock::IsHeld())
		{		
		MmuLock::Unlock();
		PageCleaningLock::Lock();
		MmuLock::Lock();
		pageCleaningLockAcquired = ETrue;
		goto restart;
		}
	
	// try to steal it from owning object...
	if (StealPage(pageInfo) != KErrNone)
		goto restart;
	
	BalanceAges();
	
exit:
	if (pageCleaningLockAcquired)
		{		
		MmuLock::Unlock();
		PageCleaningLock::Unlock();
		MmuLock::Lock();
		}

	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	
	return pageInfo;
	}


template <class T, TUint maxObjects> class TSequentialColourSelector
	{
public:
	enum
		{
		KMaxSearchLength = _ALIGN_UP(maxObjects, KPageColourCount)
		};
	
	FORCE_INLINE TSequentialColourSelector(TUint aTargetLength)
		{
		memclr(this, sizeof(*this));
		__NK_ASSERT_DEBUG(aTargetLength <= maxObjects);
		iTargetLength = aTargetLength;
		iSearchLength = _ALIGN_UP(aTargetLength, KPageColourCount);
		}

	FORCE_INLINE TBool FoundLongestSequence()
		{
		return iLongestLength >= iTargetLength;
		}

	FORCE_INLINE void AddCandidate(T* aObject, TUint aColour)
		{
		// allocate objects to slots based on colour
		for (TUint i = aColour ; i < iSearchLength ; i += KPageColourCount)
			{
			if (!iSlot[i])
				{
				iSlot[i] = aObject;
				iSeqLength[i] = i == 0 ? 1 : iSeqLength[i - 1] + 1;
				TUint j = i + 1;
				while(j < iSearchLength && iSeqLength[j])
					iSeqLength[j++] += iSeqLength[i];
				TUint currentLength = iSeqLength[j - 1];
				if (currentLength > iLongestLength)
					{
					iLongestLength = currentLength;
					iLongestStart = j - currentLength;
					}
				break;
				}
			}
		}

	FORCE_INLINE TUint FindLongestRun(T** aObjectsOut)
		{
		if (iLongestLength == 0)
			return 0;

		if (iLongestLength < iTargetLength && iSlot[0] && iSlot[iSearchLength - 1])
			{
			// check possibility of wrapping

			TInt i = 1;
			while (iSlot[i]) ++i;  // find first hole
			TUint wrappedLength = iSeqLength[iSearchLength - 1] + iSeqLength[i - 1];
			if (wrappedLength > iLongestLength)
				{
				iLongestLength = wrappedLength;
				iLongestStart = iSearchLength - iSeqLength[iSearchLength - 1];
				}
			}		

		iLongestLength = MinU(iLongestLength, iTargetLength);

		__NK_ASSERT_DEBUG(iLongestStart < iSearchLength);
		__NK_ASSERT_DEBUG(iLongestStart + iLongestLength < 2 * iSearchLength);

		TUint len = MinU(iLongestLength, iSearchLength - iLongestStart);
		wordmove(aObjectsOut, &iSlot[iLongestStart], len * sizeof(T*));
		wordmove(aObjectsOut + len, &iSlot[0], (iLongestLength - len) * sizeof(T*));
		
		return iLongestLength;
		}

private:
	TUint iTargetLength;
	TUint iSearchLength;
	TUint iLongestStart;
	TUint iLongestLength;
	T* iSlot[KMaxSearchLength];
	TUint8 iSeqLength[KMaxSearchLength];
	};


TInt DPager::SelectSequentialPagesToClean(SPageInfo** aPageInfosOut)
	{
	// select up to iPagesToClean oldest dirty pages with sequential page colours
	
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	TSequentialColourSelector<SPageInfo, KMaxPagesToClean> selector(iPagesToClean);

	SDblQueLink* link = iOldestDirtyList.Last();
	while (link != &iOldestDirtyList.iA)
		{
		SPageInfo* pi = SPageInfo::FromLink(link);
		if (!pi->IsWritable())  
			{
			// the page may be in the process of being restricted, stolen or decommitted, but don't
			// check for this as it will occur infrequently and will be detected by CheckModified
			// anyway
			TInt colour = pi->Index() & KPageColourMask;
			selector.AddCandidate(pi, colour);
			if (selector.FoundLongestSequence())
				break;
			}
		link = link->iPrev;
		}
	
	return selector.FindLongestRun(aPageInfosOut);
	}


TInt DPager::SelectOldestPagesToClean(SPageInfo** aPageInfosOut)
	{
	// select up to iPagesToClean oldest dirty pages
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	TUint pageCount = 0;
	SDblQueLink* link = iOldestDirtyList.Last();
	while (link != &iOldestDirtyList.iA && pageCount < iPagesToClean)
		{
		SPageInfo* pi = SPageInfo::FromLink(link);
		if (!pi->IsWritable())
			{
			// the page may be in the process of being restricted, stolen or decommitted, but don't
			// check for this as it will occur infrequently and will be detected by CheckModified
			// anyway
			aPageInfosOut[pageCount++] = pi;
			}
		link = link->iPrev;
		}
	return pageCount;
	}


TInt DPager::CleanSomePages(TBool aBackground)
	{
	TRACE(("DPager::CleanSomePages"));
	
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(PageCleaningLock::IsHeld());
	// ram alloc lock may or may not be held

	SPageInfo* pageInfos[KMaxPagesToClean];
	TInt pageCount;
	if (iCleanInSequence)
		pageCount = SelectSequentialPagesToClean(&pageInfos[0]);
	else
		pageCount = SelectOldestPagesToClean(&pageInfos[0]);
	
	if (pageCount == 0)
		{
		TRACE2(("DPager::CleanSomePages no pages to clean", pageCount));
		TRACE2(("  page counts %d, %d, %d, %d",
				iYoungCount, iOldCount, iOldestCleanCount, iOldestDirtyCount));
		return 0;
		}
	
	TheDataPagedMemoryManager->CleanPages(pageCount, pageInfos, aBackground);

	for (TInt i = 0 ; i < pageCount ; ++i)
		{
		SPageInfo* pi = pageInfos[i];
		if (pi)
			{
			__NK_ASSERT_DEBUG(pi->PagedState() == SPageInfo::EPagedOldestDirty && iOldestDirtyCount);
			__NK_ASSERT_DEBUG(!pi->IsDirty() && !pi->IsWritable());
		
			pi->iLink.Deque();
			iOldestCleanList.AddHead(&pi->iLink);
			--iOldestDirtyCount;
			++iOldestCleanCount;
			pi->SetPagedState(SPageInfo::EPagedOldestClean);
			}
		}

	TRACE2(("DPager::CleanSomePages cleaned %d pages", pageCount));

	return pageCount;
	}


TBool DPager::HasPagesToClean()
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	return iOldestDirtyCount > 0;
	}


TInt DPager::RestrictPage(SPageInfo* aPageInfo, TRestrictPagesType aRestriction)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	TInt r;
	if(aPageInfo->Type()==SPageInfo::EUnused)
		{
		// page was unused, so nothing to do...
		r = KErrNone;
		}
	else
		{
		// get memory object which owns the page...
		__NK_ASSERT_DEBUG(aPageInfo->Type()==SPageInfo::EManaged);
		DMemoryObject* memory = aPageInfo->Owner();
		memory->Open();

		// try restricting access to page...
		r = memory->iManager->RestrictPage(memory,aPageInfo,aRestriction);
		__NK_ASSERT_DEBUG(r!=KErrNotSupported);

		// close memory object...
		MmuLock::Unlock();
		memory->AsyncClose();
		MmuLock::Lock();
		}

	return r;
	}


TInt DPager::StealPage(SPageInfo* aPageInfo)
	{
	TRACE(("DPager::StealPage(0x%08x)",aPageInfo));
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
 
	__UNLOCK_GUARD_START(MmuLock);
	RemovePage(aPageInfo);

	TInt r;
	if(aPageInfo->Type()==SPageInfo::EUnused)
		{
		// page was unused, so nothing to do...
		r = KErrNone;
		__UNLOCK_GUARD_END(MmuLock);
		MmuLock::Unlock();
		}
	else
		{
		// get memory object which owns the page...
		__NK_ASSERT_DEBUG(aPageInfo->Type()==SPageInfo::EManaged);
		DMemoryObject* memory = aPageInfo->Owner();
		memory->Open();

		// try and steal page from memory object...
		__UNLOCK_GUARD_END(MmuLock); // StealPage must be called without releasing the MmuLock
		r = memory->iManager->StealPage(memory,aPageInfo);
		__NK_ASSERT_DEBUG(r!=KErrNotSupported);

		// close memory object...
		MmuLock::Unlock();
		memory->AsyncClose();
		}

	MmuLock::Lock();

	if(r==KErrNone)
		Event(EEventPageOut,aPageInfo);

	TRACE(("DPager::StealPage returns %d",r));
	return r;
	}


TInt DPager::DiscardAndAllocPage(SPageInfo* aPageInfo, TZonePageType aPageType)
	{
	TInt r = DiscardPage(aPageInfo, KRamZoneInvalidId, M::EMoveDisMoveDirty);
	if (r == KErrNone)
		{
		TheMmu.MarkPageAllocated(aPageInfo->PhysAddr(), aPageType);
		}
	return r;
	}


static TBool DiscardCanStealPage(SPageInfo* aOldPageInfo, TBool aMoveDirty)
	{
 	// If the page is pinned or if the page is dirty and a general defrag is being performed then
	// don't attempt to steal it
	return aOldPageInfo->Type() == SPageInfo::EUnused ||
		(aOldPageInfo->PagedState() != SPageInfo::EPagedPinned && (!aMoveDirty || !aOldPageInfo->IsDirty()));	
	}


TInt DPager::DiscardPage(SPageInfo* aOldPageInfo, TUint aBlockZoneId, TUint aMoveDisFlags)
	{
	// todo: assert MmuLock not released
	
	TRACE(("> DPager::DiscardPage %08x", aOldPageInfo));
	
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	TBool moveDirty = (aMoveDisFlags & M::EMoveDisMoveDirty) != 0;
	TBool blockRest = (aMoveDisFlags & M::EMoveDisBlockRest) != 0;

	if (!DiscardCanStealPage(aOldPageInfo, moveDirty))
		{
		// Page must be managed if it is pinned or dirty.
		__NK_ASSERT_DEBUG(aOldPageInfo->Type()==SPageInfo::EManaged);
		// The page is pinned or is dirty and this is a general defrag so move the page.
		DMemoryObject* memory = aOldPageInfo->Owner();
		memory->Open();
		TPhysAddr newAddr;
		TRACE2(("DPager::DiscardPage delegating pinned/dirty page to manager"));
		TInt r = memory->iManager->MovePage(memory, aOldPageInfo, newAddr, aBlockZoneId, blockRest);
		TRACE(("< DPager::DiscardPage %d", r));
		memory->AsyncClose();
		return r;
		}

	TInt r = KErrNone;
	SPageInfo* newPageInfo = NULL;
	TBool havePageCleaningLock = EFalse;

	TBool needNewPage;
	TBool needPageCleaningLock;
	while(needNewPage = (iNumberOfFreePages == 0 && newPageInfo == NULL),
		  needPageCleaningLock = (aOldPageInfo->IsDirty() && !havePageCleaningLock),
		  needNewPage || needPageCleaningLock)
		{
		MmuLock::Unlock();

		if (needNewPage)
			{
			// Allocate a new page for the live list as it has reached its minimum size.
			TUint flags = EMemAttNormalCached | Mmu::EAllocNoWipe;
			newPageInfo = GetPageFromSystem((Mmu::TRamAllocFlags)flags, aBlockZoneId, blockRest);
			if (!newPageInfo)
				{
				TRACE(("< DPager::DiscardPage KErrNoMemory"));
				r = KErrNoMemory;
				MmuLock::Lock();
				break;
				}
			}

		if (needPageCleaningLock)
			{
			// Acquire the page cleaning mutex so StealPage can clean it
			PageCleaningLock::Lock();
			havePageCleaningLock = ETrue;
			}

		// Re-acquire the mmulock and re-check that the page is not pinned or dirty.
		MmuLock::Lock();
		if (!DiscardCanStealPage(aOldPageInfo, moveDirty))
			{
			// Page is now pinned or dirty so give up as it is in use.
			r = KErrInUse;
			break;
			}
		}

	if (r == KErrNone)
		{
		// Attempt to steal the page
		r = StealPage(aOldPageInfo);  // temporarily releases MmuLock if page is dirty
		}
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	if (r == KErrCompletion)
		{// This was a page table that has been freed but added to the 
		// live list as a free page.  Remove from live list and continue.
		__NK_ASSERT_DEBUG(!aOldPageInfo->IsDirty());
		RemovePage(aOldPageInfo);
		r = KErrNone;
		}

	if (r == KErrNone && iNumberOfFreePages == 0)
		{
		if (newPageInfo)
			{
			// Add a new page to the live list if we have one as discarding the old page will reduce
			// the live list below the minimum.
			AddAsFreePage(newPageInfo);
			newPageInfo = NULL;
			}
		else
			{
			// Otherwise the live list shrank when page was being cleaned so have to give up
			AddAsFreePage(aOldPageInfo);
			BalanceAges();                  // temporarily releases MmuLock
			r = KErrInUse;
			}
		}

	if (r == KErrNone)
		{
		// We've successfully discarded the page and ensured the live list is large enough, so
		// return it to the free pool.
		ReturnPageToSystem(*aOldPageInfo);  // temporarily releases MmuLock
		BalanceAges();                      // temporarily releases MmuLock
		}

	if (newPageInfo)
		{
		// New page not required so just return it to the system.  This is safe as
		// iNumberOfFreePages will have this page counted but as it is not on the live list noone
		// else can touch it.
		if (iNumberOfFreePages == 0)
			AddAsFreePage(newPageInfo);
		else
			ReturnPageToSystem(*newPageInfo);   // temporarily releases MmuLock
		}	
	MmuLock::Unlock();

	if (havePageCleaningLock)
		{
		// Release the page cleaning mutex
		PageCleaningLock::Unlock();
		}

	TRACE(("< DPager::DiscardPage returns %d", r));
	return r;	
	}


TBool DPager::TryGrowLiveList()
	{
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	MmuLock::Unlock();
	SPageInfo* sparePage = GetPageFromSystem((Mmu::TRamAllocFlags)(EMemAttNormalCached|Mmu::EAllocNoWipe));
	MmuLock::Lock();

	if(!sparePage)
		return false;

	// add page to live list...
	AddAsFreePage(sparePage);
	return true;
	}


SPageInfo* DPager::GetPageFromSystem(Mmu::TRamAllocFlags aAllocFlags, TUint aBlockZoneId, TBool aBlockRest)
	{
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());

	TPhysAddr pagePhys;
	TInt r = TheMmu.AllocRam(&pagePhys, 1, 
							(Mmu::TRamAllocFlags)(aAllocFlags|Mmu::EAllocNoPagerReclaim), 
							EPageDiscard, aBlockZoneId, aBlockRest);
	if(r!=KErrNone)
		return NULL;

	MmuLock::Lock();
	++iNumberOfFreePages;
	MmuLock::Unlock();

	return SPageInfo::FromPhysAddr(pagePhys);
	}


TBool DPager::TryReturnOldestPageToSystem()
	{
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(iNumberOfFreePages>0);

	SPageInfo* pageInfo = StealOrAllocPage(EFalse, (Mmu::TRamAllocFlags)0);
	
	// StealOrAllocPage may have released the MmuLock, so check there are still enough pages
	// to remove one from the live list
	if (iNumberOfFreePages>0)
		{
		ReturnPageToSystem(*pageInfo);
		return ETrue;
		}
	else
		{
		AddAsFreePage(pageInfo);
		return EFalse;
		}
	}


TUint DPager::AllowAddFreePages(SPageInfo*& aPageInfo, TUint aNumPages)
	{
	if (iMinimumPageCount + iNumberOfFreePages == iMaximumPageCount)
		{// The paging cache is already at the maximum size so steal a page
		// so it can be returned to the system if required.
		aPageInfo = StealOrAllocPage(EFalse, (Mmu::TRamAllocFlags)0);
		__NK_ASSERT_DEBUG(aPageInfo->PagedState() == SPageInfo::EUnpaged);
		return 1;
		}
	// The paging cache is not at its maximum so determine how many can be added to
	// the paging cache without it growing past its maximum.
	aPageInfo = NULL;
	__NK_ASSERT_DEBUG(iMinimumPageCount + iNumberOfFreePages < iMaximumPageCount);
	if (iMinimumPageCount + iNumberOfFreePages + aNumPages > iMaximumPageCount)
		{
		return iMaximumPageCount - (iMinimumPageCount + iNumberOfFreePages);
		}
	else
		return aNumPages;
	}


void DPager::AllowAddFreePage(SPageInfo*& aPageInfo)
	{
	if (iMinimumPageCount + iNumberOfFreePages == iMaximumPageCount)
		{// The paging cache is already at the maximum size so steal a page
		// so it can be returned to the system if required.
		aPageInfo = StealOrAllocPage(EFalse, (Mmu::TRamAllocFlags)0);
		__NK_ASSERT_DEBUG(aPageInfo->PagedState() == SPageInfo::EUnpaged);
		return;
		}
	aPageInfo = NULL;
	}


void DPager::ReturnPageToSystem(SPageInfo& aPageInfo)
	{
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	// should be unpaged at this point, otherwise Mmu::FreeRam will just give it back to us
	__NK_ASSERT_DEBUG(aPageInfo.PagedState() == SPageInfo::EUnpaged);

	__NK_ASSERT_DEBUG(iNumberOfFreePages>0);
	--iNumberOfFreePages;

	// The page must be unpaged, otherwise it wasn't successfully removed 
	// from the live list.
	__NK_ASSERT_DEBUG(aPageInfo.PagedState() == SPageInfo::EUnpaged);
	MmuLock::Unlock();

	TPhysAddr pagePhys = aPageInfo.PhysAddr();
	TheMmu.FreeRam(&pagePhys, 1, EPageDiscard);

	MmuLock::Lock();
	}


SPageInfo* DPager::PageInAllocPage(Mmu::TRamAllocFlags aAllocFlags)
	{
	// ram alloc mutex may or may not be held
	__NK_ASSERT_DEBUG(!MmuLock::IsHeld());
	
	RamAllocLock::Lock();
	
	MmuLock::Lock();	
	SPageInfo* pageInfo = StealOrAllocPage(ETrue, aAllocFlags);
	TBool wasAllocated = pageInfo->Type() == SPageInfo::EUnknown;
	MmuLock::Unlock();

	if (!wasAllocated)
		{
		// make page state same as a freshly allocated page...
		TPhysAddr pagePhys = pageInfo->PhysAddr();
		TheMmu.PagesAllocated(&pagePhys,1,aAllocFlags);
		}

	RamAllocLock::Unlock();

	return pageInfo;
	}


TBool DPager::GetFreePages(TInt aNumPages)
	{
	TRACE(("DPager::GetFreePages(%d)",aNumPages));

	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());

	MmuLock::Lock();
	while(aNumPages>0 && (TInt)NumberOfFreePages()>=aNumPages)
		{
		if (TryReturnOldestPageToSystem())
			--aNumPages;
		}
	MmuLock::Unlock();

	TRACE(("DPager::GetFreePages returns %d",!aNumPages));
	return !aNumPages;
	}


void DPager::DonatePages(TUint aCount, TPhysAddr* aPages)
	{
	TRACE(("DPager::DonatePages(%d,?)",aCount));
	__ASSERT_CRITICAL;
	RamAllocLock::Lock();
	MmuLock::Lock();

	TPhysAddr* end = aPages+aCount;
	while(aPages<end)
		{
		// Steal a page from the paging cache in case we need to return one to the system.
		// This may release the ram alloc lock.
		SPageInfo* pageInfo;
		AllowAddFreePage(pageInfo);

		TPhysAddr pagePhys = *aPages++;
		if(RPageArray::State(pagePhys)!=RPageArray::ECommitted)
			{
			if (pageInfo)
				AddAsFreePage(pageInfo);
			continue; // page is not present
			}

#ifdef _DEBUG
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pagePhys&~KPageMask);
		__NK_ASSERT_DEBUG(pi);
#else
		SPageInfo* pi = SPageInfo::FromPhysAddr(pagePhys);
#endif
		switch(pi->PagedState())
			{
		case SPageInfo::EUnpaged:
			// Change the type of this page to discardable and 
			// then add it to live list.
			// Only the DDiscardableMemoryManager should be invoking this and
			// its pages will be movable before they are donated.
			__NK_ASSERT_DEBUG(pi->Owner()->iManager->PageType() == EPageMovable);
			TheMmu.ChangePageType(pi, EPageMovable, EPageDiscard);
			break;

		case SPageInfo::EPagedYoung:
		case SPageInfo::EPagedOld:
		case SPageInfo::EPagedOldestDirty:
		case SPageInfo::EPagedOldestClean:
			if (pageInfo)
				AddAsFreePage(pageInfo);
			continue; // discard already been allowed

		case SPageInfo::EPagedPinned:
			__NK_ASSERT_DEBUG(0);
		default:
			__NK_ASSERT_DEBUG(0);
			if (pageInfo)
				AddAsFreePage(pageInfo);
			continue;
			}

		// put page on live list and free the stolen page...
		AddAsYoungestPage(pi);
		++iNumberOfFreePages;
		if (pageInfo)
			ReturnPageToSystem(*pageInfo);
		Event(EEventPageDonate,pi);

		// re-balance live list...
		BalanceAges();
		}

	__NK_ASSERT_DEBUG((iMinimumPageCount + iNumberOfFreePages) <= iMaximumPageCount);
	MmuLock::Unlock();
	RamAllocLock::Unlock();
	}


TInt DPager::ReclaimPages(TUint aCount, TPhysAddr* aPages)
	{
	TRACE(("DPager::ReclaimPages(%d,?)",aCount));
	__ASSERT_CRITICAL;
	RamAllocLock::Lock();
	MmuLock::Lock();

	TInt r = KErrNone;
	TPhysAddr* end = aPages+aCount;
	while(aPages<end)
		{
		TPhysAddr pagePhys = *aPages++;
		TBool changeType = EFalse;

		if(RPageArray::State(pagePhys)!=RPageArray::ECommitted)
			{
			r = KErrNotFound; // too late, page has gone
			continue;
			}

#ifdef _DEBUG
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pagePhys&~KPageMask);
		__NK_ASSERT_DEBUG(pi);
#else
		SPageInfo* pi = SPageInfo::FromPhysAddr(pagePhys);
#endif
		switch(pi->PagedState())
			{
		case SPageInfo::EUnpaged:
			continue; // discard already been disallowed

		case SPageInfo::EPagedYoung:
		case SPageInfo::EPagedOld:
		case SPageInfo::EPagedOldestClean:
		case SPageInfo::EPagedOldestDirty:
			changeType = ETrue;
			break; // remove from live list

		case SPageInfo::EPagedPinned:
			__NK_ASSERT_DEBUG(0);
		default:
			__NK_ASSERT_DEBUG(0);
			break;
			}

		// check paging list has enough pages before we remove one...
		if(!iNumberOfFreePages)
			{
			// need more pages so get a page from the system...
			if(!TryGrowLiveList())
				{
				// out of memory...
				r = KErrNoMemory;
				break;
				}
			// retry the page reclaim...
			--aPages;
			continue;
			}

		if (changeType)
			{// Change the type of this page to movable, wait until any retries
			// have been attempted as we can't change a page's type twice.
			// Only the DDiscardableMemoryManager should be invoking this and
			// its pages should be movable once they are reclaimed.
			__NK_ASSERT_DEBUG(pi->Owner()->iManager->PageType() == EPageMovable);
			TheMmu.ChangePageType(pi, EPageDiscard, EPageMovable);
			}

		// remove page from paging list...
		__NK_ASSERT_DEBUG(iNumberOfFreePages>0);
		--iNumberOfFreePages;
		RemovePage(pi);

		Event(EEventPageReclaim,pi);

		// re-balance live list...
		BalanceAges();
		}

	// we may have added a spare free page to the live list without removing one,
	// this could cause us to have too many pages, so deal with this...

	// If there are too many pages they should all be unused free pages otherwise 
	// the ram alloc lock may be released by RemoveExcessPages().
	__NK_ASSERT_DEBUG(	!HaveTooManyPages() ||
						(iMinimumPageCount + iNumberOfFreePages - iMaximumPageCount
						<= iOldestCleanCount));
	RemoveExcessPages();

	__NK_ASSERT_DEBUG((iMinimumPageCount + iNumberOfFreePages) <= iMaximumPageCount);
	MmuLock::Unlock();
	RamAllocLock::Unlock();
	return r;
	}


TInt VMHalFunction(TAny*, TInt aFunction, TAny* a1, TAny* a2);

void DPager::Init3()
	{
	TRACEB(("DPager::Init3()"));
	TheRomMemoryManager->Init3();
	TheDataPagedMemoryManager->Init3();
	TheCodePagedMemoryManager->Init3();
	TInt r = Kern::AddHalEntry(EHalGroupVM, VMHalFunction, 0);
	__NK_ASSERT_ALWAYS(r==KErrNone);
	PageCleaningLock::Init();
#ifdef __DEMAND_PAGING_BENCHMARKS__
	for (TInt i = 0 ; i < EMaxPagingBm ; ++i)
		ResetBenchmarkData((TPagingBenchmark)i);
#endif
	}


void DPager::Fault(TFault aFault)
	{
	Kern::Fault("DPager",aFault);
	}


void DPager::BalanceAges()
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	TBool retry;
	do
		{
		retry = EFalse;
		TBool restrictPage = EFalse;
		SPageInfo* pageInfo = NULL;
		TUint oldestCount = iOldestCleanCount + iOldestDirtyCount;
		if((iOldCount + oldestCount) * iYoungOldRatio < iYoungCount)
			{
			// Need more old pages so make one young page into an old page...
			__NK_ASSERT_DEBUG(!iYoungList.IsEmpty());
			__NK_ASSERT_DEBUG(iYoungCount);
			SDblQueLink* link = iYoungList.Last()->Deque();
			--iYoungCount;

			pageInfo = SPageInfo::FromLink(link);
			pageInfo->SetPagedState(SPageInfo::EPagedOld);

			iOldList.AddHead(link);
			++iOldCount;

			Event(EEventPageAged,pageInfo);
			// Delay restricting the page until it is safe to release the MmuLock.
			restrictPage = ETrue;
			}

		// Check we have enough oldest pages.
		if (oldestCount < iMaxOldestPages &&
			oldestCount * iOldOldestRatio < iOldCount)
			{
			__NK_ASSERT_DEBUG(!iOldList.IsEmpty());
			__NK_ASSERT_DEBUG(iOldCount);
			SDblQueLink* link = iOldList.Last()->Deque();
			--iOldCount;

			SPageInfo* oldestPageInfo = SPageInfo::FromLink(link);
			if (oldestPageInfo->IsDirty())
				{
				oldestPageInfo->SetOldestPage(SPageInfo::EPagedOldestDirty);
				iOldestDirtyList.AddHead(link);
				++iOldestDirtyCount;
				PageCleaner::NotifyPagesToClean();
				Event(EEventPageAgedDirty,oldestPageInfo);
				}
			else
				{
				oldestPageInfo->SetOldestPage(SPageInfo::EPagedOldestClean);
				iOldestCleanList.AddHead(link);
				++iOldestCleanCount;
				Event(EEventPageAgedClean,oldestPageInfo);
				}
			}

		if (restrictPage)
			{
			// Make the recently aged old page inaccessible.  This is done last as it will release
			// the MmuLock and therefore the page counts may otherwise change.
			TInt r = RestrictPage(pageInfo,ERestrictPagesNoAccessForOldPage);

			if (r == KErrInUse)
				{
				SPageInfo::TPagedState state = pageInfo->PagedState();
				if (state == SPageInfo::EPagedOld ||
					state == SPageInfo::EPagedOldestClean ||
					state == SPageInfo::EPagedOldestDirty)
					{
					// The restrict operation failed, but the page was left in an old state.  This
					// can happen when:
					//  
					//  - pages are in the process of being pinned - the mapping will veto the
					//    restriction
					//  - pages are rejuvenated and then become quickly become old again
					//  
					// In the second instance the page will be needlessly rejuvenated because we
					// can't tell that it has actually been restricted by another thread
					RemovePage(pageInfo);
					AddAsYoungestPage(pageInfo);
					retry = ETrue;
					}
				}
			}
		}
	while (retry);
	}


void DPager::RemoveExcessPages()
	{
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	while(HaveTooManyPages())
		TryReturnOldestPageToSystem();
	}


void DPager::RejuvenatePageTable(TPte* aPt)
	{
	SPageInfo* pi = SPageInfo::FromPhysAddr(Mmu::PageTablePhysAddr(aPt));

	SPageTableInfo* pti = SPageTableInfo::FromPtPtr(aPt);
	if(!pti->IsDemandPaged())
		{
		__NK_ASSERT_DEBUG(pi->PagedState()==SPageInfo::EUnpaged);
		return;
		}

	TRACE2(("DP: %O Rejuvenate PT 0x%08x 0x%08x",TheCurrentThread,pi->PhysAddr(),aPt));
	switch(pi->PagedState())
		{
	case SPageInfo::EPagedYoung:
	case SPageInfo::EPagedOld:
	case SPageInfo::EPagedOldestClean:
	case SPageInfo::EPagedOldestDirty:
		RemovePage(pi);
		AddAsYoungestPage(pi);
		BalanceAges();
		break;

	case SPageInfo::EUnpaged:
		AddAsYoungestPage(pi);
		BalanceAges();
		break;

	case SPageInfo::EPagedPinned:
		break;

	default:
		__NK_ASSERT_DEBUG(0);
		break;
		}
	}


TInt DPager::PteAndInfoFromLinAddr(	TInt aOsAsid, TLinAddr aAddress, DMemoryMappingBase* aMapping, 
									TUint aMapInstanceCount, TPte*& aPte, SPageInfo*& aPageInfo)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());	

	// Verify the mapping is still mapped and has not been reused.
	if (aMapInstanceCount != aMapping->MapInstanceCount() || aMapping->BeingDetached())
		return KErrAbort;

	aPte = Mmu::SafePtePtrFromLinAddr(aAddress,aOsAsid);
	if(!aPte)
		return KErrNotFound;

	TPte pte = *aPte;
	if(pte==KPteUnallocatedEntry)
		return KErrNotFound;

	SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pte & ~KPageMask);
	if(!pi)
		return KErrNotFound;
	aPageInfo = pi;

	return KErrNone;
	}


TInt DPager::TryRejuvenate(	TInt aOsAsid, TLinAddr aAddress, TUint aAccessPermissions, TLinAddr aPc,
							DMemoryMappingBase* aMapping, TUint aMapInstanceCount, DThread* aThread, 
							TAny* aExceptionInfo)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	START_PAGING_BENCHMARK;

	SPageInfo* pi;
	TPte* pPte;
	TPte pte;
	TInt r = PteAndInfoFromLinAddr(aOsAsid, aAddress, aMapping, aMapInstanceCount, pPte, pi);
	if (r != KErrNone)
		{
		if (aThread->IsRealtime())
			{// This thread is real time so it shouldn't be accessing paged out paged memory
			// unless there is a paging trap.
			MmuLock::Unlock();
			// Ensure that we abort when the thread is not allowed to access paged out pages.
			if (CheckRealtimeThreadFault(aThread, aExceptionInfo) != KErrNone)
				r = KErrAbort;
			MmuLock::Lock();
			}
		return r;
		}
	pte = *pPte;
	SPageInfo::TType type = pi->Type();
	SPageInfo::TPagedState state = pi->PagedState();

	if (aThread->IsRealtime() && 
		state != SPageInfo::EPagedPinned && 
		state != SPageInfo::EPagedPinnedMoved)
		{// This thread is real time so it shouldn't be accessing unpinned paged memory
		// unless there is a paging trap.
		MmuLock::Unlock();
		r = CheckRealtimeThreadFault(aThread, aExceptionInfo);
		MmuLock::Lock();
		if (r != KErrNone)
			return r;
		// We had to release the MmuLock have to reverify the status of the page and mappings.
		r = PteAndInfoFromLinAddr(aOsAsid, aAddress, aMapping, aMapInstanceCount, pPte, pi);
		if (r != KErrNone)
			return r;
		pte = *pPte;
		type = pi->Type();
		state = pi->PagedState();
		}

	if (type != SPageInfo::EManaged)
		return KErrNotFound;

	if(state==SPageInfo::EUnpaged)
		return KErrNotFound;

	DMemoryObject* memory = pi->Owner();
	TUint index = pi->Index();

	TPhysAddr page = memory->iPages.Page(index);
	if(!RPageArray::IsPresent(page))
		return KErrNotFound;

	TPhysAddr physAddr = pi->PhysAddr();
	if ((page^physAddr) >= (TPhysAddr)KPageSize)
		{// Page array entry should contain same physical address as PTE unless the 
		// page has or is being moved and this mapping accessed the page.
		// Get the page info for the page that we should be using.
		physAddr = page & ~KPageMask;
		pi = SPageInfo::SafeFromPhysAddr(physAddr);
		if(!pi)
			return KErrNotFound;

		type = pi->Type();
		if (type!=SPageInfo::EManaged)
			return KErrNotFound;

		state = pi->PagedState();
		if(state==SPageInfo::EUnpaged)
			return KErrNotFound;

		memory = pi->Owner();
		index = pi->Index();

		// Update pte to point to the correct physical address for this memory object's page.
		pte = (pte & KPageMask) | physAddr;
		}

	if(aAccessPermissions&EReadWrite)
		{// The mapping that took the fault permits writes and is still attached 
		// to the memory object therefore the object can't be read only.
		__NK_ASSERT_DEBUG(!memory->IsReadOnly());
		SetWritable(*pi);
		}

	pte = Mmu::MakePteAccessible(pte,aAccessPermissions&EReadWrite);
	TRACE2(("!PTE %x=%x",pPte,pte));
	*pPte = pte;
	CacheMaintenance::SinglePteUpdated((TLinAddr)pPte);
	InvalidateTLBForPage((aAddress&~KPageMask)|aOsAsid);

	Event(EEventPageRejuvenate,pi,aPc,aAddress,aAccessPermissions);

	TBool balance = false;
	if(	state==SPageInfo::EPagedYoung || state==SPageInfo::EPagedOld || 
		state==SPageInfo::EPagedOldestClean || state==SPageInfo::EPagedOldestDirty)
		{
		RemovePage(pi);
		AddAsYoungestPage(pi);
		// delay BalanceAges because we don't want to release MmuLock until after
		// RejuvenatePageTable has chance to look at the page table page...
		balance = true;
		}
	else
		{// Clear the modifier so that if this page is being moved then this 
		// access is detected. For non-pinned pages the modifier is cleared 
		// by RemovePage().
		__NK_ASSERT_DEBUG(state==SPageInfo::EPagedPinned);
		pi->SetModifier(0);
		}

	RejuvenatePageTable(pPte);

	if(balance)
		BalanceAges();

	END_PAGING_BENCHMARK(EPagingBmRejuvenate);
	return KErrNone;
	}


TInt DPager::PageInAllocPages(TPhysAddr* aPages, TUint aCount, Mmu::TRamAllocFlags aAllocFlags)
	{
	TUint n = 0;
	while(n<aCount)
		{
		SPageInfo* pi = PageInAllocPage(aAllocFlags);
		if(!pi)
			goto fail;
		aPages[n++] = pi->PhysAddr();
		}
	return KErrNone;
fail:
	PageInFreePages(aPages,n);
	return KErrNoMemory;
	}


void DPager::PageInFreePages(TPhysAddr* aPages, TUint aCount)
	{
	while(aCount--)
		{
		MmuLock::Lock();
		SPageInfo* pi = SPageInfo::FromPhysAddr(aPages[aCount]);
		switch(pi->PagedState())
			{
		case SPageInfo::EPagedYoung:
		case SPageInfo::EPagedOld:
		case SPageInfo::EPagedOldestClean:
		case SPageInfo::EPagedOldestDirty:
			RemovePage(pi);
			// fall through...
		case SPageInfo::EUnpaged:
			AddAsFreePage(pi);
			break;

		case SPageInfo::EPagedPinned:
			__NK_ASSERT_DEBUG(0);
			break;
		default:
			__NK_ASSERT_DEBUG(0);
			break;
			}
		MmuLock::Unlock();
		}
	}


void DPager::PagedInUnneeded(SPageInfo* aPageInfo)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	Event(EEventPageInUnneeded,aPageInfo);
	AddAsFreePage(aPageInfo);
	}


void DPager::PagedIn(SPageInfo* aPageInfo)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	switch(aPageInfo->PagedState())
		{
	case SPageInfo::EPagedYoung:
	case SPageInfo::EPagedOld:
	case SPageInfo::EPagedOldestClean:
	case SPageInfo::EPagedOldestDirty:
		RemovePage(aPageInfo);
		AddAsYoungestPage(aPageInfo);
		BalanceAges();
		break;

	case SPageInfo::EUnpaged:
		AddAsYoungestPage(aPageInfo);
		BalanceAges();
		break;

	case SPageInfo::EPagedPinned:
		// Clear the modifier so that if this page is being moved then this 
		// access is detected. For non-pinned pages the modifier is cleared by RemovePage().
		aPageInfo->SetModifier(0);
		break;

	default:
		__NK_ASSERT_DEBUG(0);
		break;
		}
	}


void DPager::PagedInPinned(SPageInfo* aPageInfo, TPinArgs& aPinArgs)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	Pin(aPageInfo,aPinArgs);
	}


void DPager::Pin(SPageInfo* aPageInfo, TPinArgs& aPinArgs)
	{
	TRACE(("DPager::Pin %08x", aPageInfo->PhysAddr()));
	
	__ASSERT_CRITICAL;
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aPinArgs.HaveSufficientPages(1));

	aPageInfo->IncPinCount();
	Event(EEventPagePin,aPageInfo);

	// remove page from live list...
	switch(aPageInfo->PagedState())
		{
	case SPageInfo::EPagedYoung:
		__NK_ASSERT_DEBUG(iYoungCount);
		aPageInfo->iLink.Deque();
		--iYoungCount;
		__NK_ASSERT_DEBUG(aPageInfo->PinCount()==1);
		break;

	case SPageInfo::EPagedOld:
		__NK_ASSERT_DEBUG(iOldCount);
		aPageInfo->iLink.Deque();
		--iOldCount;
		__NK_ASSERT_DEBUG(aPageInfo->PinCount()==1);
		break;

	case SPageInfo::EPagedOldestClean:
		__NK_ASSERT_DEBUG(iOldestCleanCount);
		aPageInfo->iLink.Deque();
		--iOldestCleanCount;
		__NK_ASSERT_DEBUG(aPageInfo->PinCount()==1);
		break;

	case SPageInfo::EPagedOldestDirty:
		__NK_ASSERT_DEBUG(iOldestDirtyCount);
		aPageInfo->iLink.Deque();
		--iOldestDirtyCount;
		__NK_ASSERT_DEBUG(aPageInfo->PinCount()==1);
		break;

	case SPageInfo::EPagedPinned:
		// nothing more to do...
		__NK_ASSERT_DEBUG(aPageInfo->PinCount()>1);
		return;

	case SPageInfo::EUnpaged:
		__NK_ASSERT_DEBUG(aPageInfo->PinCount()==1);
		TRACE2(("DPager::PinPage page was unpaged"));
		// This could be a page in the process of being stolen.
		// Could also be page for storing page table infos, which aren't necessarily
		// on the live list.
		break;

	default:
		__NK_ASSERT_DEBUG(0);
		return;
		}

	// page has now been removed from the live list and is pinned...
	aPageInfo->SetPagedState(SPageInfo::EPagedPinned);

	if(aPinArgs.iReplacementPages==TPinArgs::EUseReserveForPinReplacementPages)
		{
		// pinned paged counts as coming from reserve pool...
		aPageInfo->SetPinnedReserve();
		}
	else
		{
		// we used up a replacement page...
		--aPinArgs.iReplacementPages;
		}

	BalanceAges();
	}


void DPager::Unpin(SPageInfo* aPageInfo, TPinArgs& aPinArgs)
	{
	__ASSERT_CRITICAL;
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aPageInfo->PagedState()==SPageInfo::EPagedPinned);
	__NK_ASSERT_DEBUG(aPageInfo->PinCount()>0);

	TUint pinCount = aPageInfo->DecPinCount();
	Event(EEventPageUnpin,aPageInfo);

	if(pinCount)
		return;

	aPageInfo->SetPagedState(SPageInfo::EUnpaged);

	if(!aPageInfo->ClearPinnedReserve())
		{
		// was not a pinned reserve page, so we how have a spare replacement page,
		// which can be used again or freed later ...
		__NK_ASSERT_DEBUG(aPinArgs.iReplacementPages!=TPinArgs::EUseReserveForPinReplacementPages);
		++aPinArgs.iReplacementPages;
		}

	AddAsYoungestPage(aPageInfo);
	BalanceAges();
	}


TInt TPinArgs::AllocReplacementPages(TUint aNumPages)
	{
	if(iUseReserve)
		{
		__NK_ASSERT_DEBUG(iReplacementPages==0 || iReplacementPages==EUseReserveForPinReplacementPages);
		iReplacementPages = EUseReserveForPinReplacementPages;
		}
	else
		{
		if(aNumPages>iReplacementPages)
			{
			if(!ThePager.AllocPinReplacementPages(aNumPages-iReplacementPages))
				return KErrNoMemory;
			iReplacementPages = aNumPages;
			}
		}
	return KErrNone;
	}


void TPinArgs::FreeReplacementPages()
	{
	if(iReplacementPages!=0 && iReplacementPages!=EUseReserveForPinReplacementPages)
		ThePager.FreePinReplacementPages(iReplacementPages);
	iReplacementPages = 0;
	}


TBool DPager::AllocPinReplacementPages(TUint aNumPages)
	{
	TRACE2(("DPager::AllocPinReplacementPages(0x%x)",aNumPages));
	__ASSERT_CRITICAL;
	RamAllocLock::Lock();
	MmuLock::Lock();

	TBool ok = false;
	do
		{
		if(iNumberOfFreePages>=aNumPages)
			{
			iNumberOfFreePages -= aNumPages;
			ok = true;
			break;
			}
		}
	while(TryGrowLiveList());

	if (!ok)
		{// Failed to allocate enough pages so free any excess..

		// If there are too many pages they should all be unused free pages otherwise 
		// the ram alloc lock may be released by RemoveExcessPages().
		__NK_ASSERT_DEBUG(	!HaveTooManyPages() ||
							(iMinimumPageCount + iNumberOfFreePages - iMaximumPageCount
							<= iOldestCleanCount));
		RemoveExcessPages();
		}
	__NK_ASSERT_DEBUG((iMinimumPageCount + iNumberOfFreePages) <= iMaximumPageCount);
	MmuLock::Unlock();
	RamAllocLock::Unlock();
	return ok;
	}


void DPager::FreePinReplacementPages(TUint aNumPages)
	{
	TRACE2(("DPager::FreePinReplacementPage(0x%x)",aNumPages));
	__ASSERT_CRITICAL;

	RamAllocLock::Lock();
	MmuLock::Lock();

	while (aNumPages)
		{
		SPageInfo* pageInfo;
		// This may release the ram alloc lock but it will flash the mmulock
		// if not all pages could be added in one go, i.e. freePages != aNumPages.
		TUint freePages = AllowAddFreePages(pageInfo, aNumPages);
		iNumberOfFreePages += freePages;
		aNumPages -= freePages;
		if (pageInfo)
			ReturnPageToSystem(*pageInfo);
		}

	__NK_ASSERT_DEBUG((iMinimumPageCount + iNumberOfFreePages) <= iMaximumPageCount);
	MmuLock::Unlock();
	RamAllocLock::Unlock();
	}


TBool DPager::ReservePage()
	{
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__ASSERT_CRITICAL;
	__NK_ASSERT_DEBUG(iMinimumPageCount >= iMinimumPageLimit+iReservePageCount);
	while(iMinimumPageCount==iMinimumPageLimit+iReservePageCount && iNumberOfFreePages==0)
		{
		if(!TryGrowLiveList())
			return false;
		}
	if(iMinimumPageCount==iMinimumPageLimit+iReservePageCount)
		{
		++iMinimumPageCount;
		--iNumberOfFreePages;
		if(iMinimumPageCount>iMaximumPageCount)
			iMaximumPageCount = iMinimumPageCount;
		}
	++iReservePageCount;
	__NK_ASSERT_DEBUG(iMinimumPageCount >= iMinimumPageLimit+iReservePageCount);
	__NK_ASSERT_DEBUG(iMinimumPageCount+iNumberOfFreePages <= iMaximumPageCount);
	return ETrue;
	}


TBool DPager::ReservePages(TUint aRequiredCount, TUint& aCount)
	{
	__ASSERT_CRITICAL;

	RamAllocLock::Lock();
	MmuLock::Lock();
	while(aCount<aRequiredCount)
		{
		if(!ReservePage())
			break;
		++aCount;
		MmuLock::Flash();
		}
	TBool enoughPages = aCount==aRequiredCount;
	MmuLock::Unlock();
	RamAllocLock::Unlock();

	if(!enoughPages)
		UnreservePages(aCount);

	return enoughPages;
	}


void DPager::UnreservePages(TUint& aCount)
	{
	MmuLock::Lock();
	iReservePageCount -= aCount;
	aCount = 0;
	MmuLock::Unlock();
	}


DThread* DPager::ResponsibleThread(DThread* aThread, TAny* aExceptionInfo)
	{
	DThread* client = aThread->iIpcClient;

	// If iIpcClient is set then we are accessing the address space of a remote thread.  If we are
	// in an IPC trap, this will contain information the local and remote addresses being accessed.
	// If this is not set then we assume than any fault must be the fault of a bad remote address.
	TIpcExcTrap* ipcTrap = (TIpcExcTrap*)aThread->iExcTrap;
	if (ipcTrap && !ipcTrap->IsTIpcExcTrap())
		ipcTrap = 0;
	if (client &&
		(!ipcTrap || ipcTrap->ExcLocation(aThread, aExceptionInfo) == TIpcExcTrap::EExcRemote))
		return client;
	else
		return NULL;
	}


TInt DPager::CheckRealtimeThreadFault(DThread* aThread, TAny* aExceptionInfo)
	{
	// realtime threads shouldn't take paging faults...
	DThread* thread = ResponsibleThread(aThread, aExceptionInfo);

	const char* message = thread ?
		"Access to Paged Memory (by other thread)" : "Access to Paged Memory";

	// kill respsonsible thread...
	if(K::IllegalFunctionForRealtimeThread(thread, message))
		{
		// if we are killing the current thread and we are in a critical section, then the above
		// kill will be deferred and we will continue executing. We will handle this by returning an
		// error which means that the thread will take an exception (which hopefully is XTRAPed!)

		// treat memory access as bad...
		return KErrAbort;
		}
	else
		{
		// thread is in 'warning only' state so allow paging...
		return KErrNone;
		}
	}


void DPager::KillResponsibleThread(TPagingErrorContext aContext, TInt aErrorCode,
								   TAny* aExceptionInfo)
	{
	const char* message = NULL;
	switch (aContext)
		{
		case EPagingErrorContextRomRead:
			message = "PAGED-ROM-READ";
			break;
		case EPagingErrorContextRomDecompress:
			message = "PAGED-ROM-COMP";
			break;
		case EPagingErrorContextCodeRead:
			message = "PAGED-CODE-READ";
			break;
		case EPagingErrorContextCodeDecompress:
			message = "PAGED-CODE-COMP";
			break;
		case EPagingErrorContextDataRead:
			message = "PAGED-DATA-READ";
			break;
		case EPagingErrorContextDataWrite:
			message = "PAGED-DATA-WRITE";
			break;
		default:
			message = "PAGED-UNKNOWN";
			break;
		}

	TPtrC8 category((const unsigned char*)message);
	DThread* thread = ResponsibleThread(TheCurrentThread, aExceptionInfo);
	if (thread)
		{
		NKern::LockSystem();
		thread->Die(EExitPanic, aErrorCode,  category);
		}
	else
		{
		TheCurrentThread->SetExitInfo(EExitPanic, aErrorCode, category);
		NKern::DeferredExit();
		}
	}


TInt DPager::HandlePageFault(	TLinAddr aPc, TLinAddr aFaultAddress, TUint aFaultAsid, TUint aFaultIndex,
								TUint aAccessPermissions, DMemoryObject* aMemory, DMemoryMapping* aMapping,
								TUint aMapInstanceCount, DThread* aThread, TAny* aExceptionInfo)
	{
	MmuLock::Lock();
	TInt r = TryRejuvenate(	aFaultAsid, aFaultAddress, aAccessPermissions, aPc, aMapping, aMapInstanceCount,
							aThread, aExceptionInfo);
	if(r == KErrNone || r == KErrAbort)
		{
		MmuLock::Unlock();
		}
	else
		{
		// rejuvenate failed, call memory manager to page in memory...
		Event(EEventPageInStart, 0, aPc, aFaultAddress, aAccessPermissions);
		MmuLock::Unlock();
		TheThrashMonitor.NotifyStartPaging();

		DMemoryManager* manager = aMemory->iManager;
		r = manager->HandleFault(aMemory, aFaultIndex, aMapping, aMapInstanceCount, aAccessPermissions);

		TheThrashMonitor.NotifyEndPaging();

		// If the paging system encountered an error paging in the memory (as opposed to a thread
		// accessing non-existent memory), then panic the appropriate thread.  Unfortunately this
		// situation does occur as media such as eMMC wears out towards the end of its life. 
		if (r != KErrNone)
			{
			TPagingErrorContext context = ExtractErrorContext(r);
			if (context != EPagingErrorContextNone)
				KillResponsibleThread(context, ExtractErrorCode(r), aExceptionInfo);
			}
		}
	return r;
	}


TInt DPager::ResizeLiveList()
	{
	MmuLock::Lock();
	TUint min = iMinimumPageCount;
	TUint max = iMaximumPageCount;
	MmuLock::Unlock();
	return ResizeLiveList(min,max);
	}


TInt DPager::ResizeLiveList(TUint aMinimumPageCount, TUint aMaximumPageCount)
	{
	TRACE(("DPager::ResizeLiveList(%d,%d) current: %d %d %d %d, %d %d %d",
		   aMinimumPageCount,aMaximumPageCount,
		   iYoungCount,iOldCount,iOldestCleanCount,iOldestDirtyCount,
		   iMinimumPageCount,iNumberOfFreePages,iMaximumPageCount));
	__NK_ASSERT_DEBUG(CacheInitialised());

	if(!aMaximumPageCount)
		{
		aMinimumPageCount = iInitMinimumPageCount;
		aMaximumPageCount = iInitMaximumPageCount;
		}
	if (aMaximumPageCount > KAbsoluteMaxPageCount)
		aMaximumPageCount = KAbsoluteMaxPageCount;

	// Min must not be greater than max...
	if(aMinimumPageCount > aMaximumPageCount)
		return KErrArgument;
	
	NKern::ThreadEnterCS();
	RamAllocLock::Lock();

	// We must hold this otherwise StealOrAllocPage will release the RamAllocLock while waiting for
	// it.  Note this method is not used in producton, so it's ok to hold both locks for longer than
	// would otherwise happen.
	PageCleaningLock::Lock();  

	MmuLock::Lock();

	__NK_ASSERT_ALWAYS(iYoungOldRatio);

	// Make sure aMinimumPageCount is not less than absolute minimum we can cope with...
	iMinimumPageLimit = iMinYoungPages * (1 + iYoungOldRatio) / iYoungOldRatio
						+ DPageReadRequest::ReservedPagesRequired();
	if(iMinimumPageLimit < iAbsoluteMinPageCount)
		iMinimumPageLimit = iAbsoluteMinPageCount;
	if(aMinimumPageCount < iMinimumPageLimit + iReservePageCount)
		aMinimumPageCount = iMinimumPageLimit + iReservePageCount;
	if(aMaximumPageCount < aMinimumPageCount)
		aMaximumPageCount = aMinimumPageCount;

	// Increase iMaximumPageCount?
	if(aMaximumPageCount > iMaximumPageCount)
		iMaximumPageCount = aMaximumPageCount;

	// Reduce iMinimumPageCount?
	if(aMinimumPageCount < iMinimumPageCount)
		{
		iNumberOfFreePages += iMinimumPageCount - aMinimumPageCount;
		iMinimumPageCount = aMinimumPageCount;
		}

	// Increase iMinimumPageCount?
	TInt r = KErrNone;
	while(aMinimumPageCount > iMinimumPageCount)
		{
		TUint newMin = MinU(aMinimumPageCount, iMinimumPageCount + iNumberOfFreePages);
		
		if (newMin == iMinimumPageCount)
			{
			// have to add pages before we can increase minimum page count
			if(!TryGrowLiveList())
				{
				r = KErrNoMemory;
				break;
				}
			}
		else
			{
			iNumberOfFreePages -= newMin - iMinimumPageCount;
			iMinimumPageCount = newMin;
			}
		}

	// Reduce iMaximumPageCount?
	while(aMaximumPageCount < iMaximumPageCount)
		{
		TUint newMax = MaxU(aMaximumPageCount, iMinimumPageCount + iNumberOfFreePages);

		if (newMax == iMaximumPageCount)
			{
			// have to remove pages before we can reduce maximum page count
			TryReturnOldestPageToSystem();
			}
		else
			{
			iMaximumPageCount = newMax;
			}
		}
	
	TRACE(("DPager::ResizeLiveList end: %d %d %d %d, %d %d %d",
		   iYoungCount,iOldCount,iOldestCleanCount,iOldestDirtyCount,
		   iMinimumPageCount,iNumberOfFreePages,iMaximumPageCount));
	
	__NK_ASSERT_DEBUG((iMinimumPageCount + iNumberOfFreePages) <= iMaximumPageCount);

#ifdef BTRACE_KERNEL_MEMORY
	BTrace4(BTrace::EKernelMemory,BTrace::EKernelMemoryDemandPagingCache,iMinimumPageCount << KPageShift);
#endif

	MmuLock::Unlock();

	PageCleaningLock::Unlock();
	RamAllocLock::Unlock();
	NKern::ThreadLeaveCS();

	return r;
	}


TUint RequiredOldestPages(TUint aPagesToClean, TBool aCleanInSequence)
	{
	return aCleanInSequence ? aPagesToClean * 8 : aPagesToClean;
	}


void DPager::SetPagesToClean(TUint aPagesToClean)
	{
	TRACE(("WDP: Pager will attempt to clean %d pages", aPagesToClean));
	__NK_ASSERT_ALWAYS(aPagesToClean > 0 && aPagesToClean <= KMaxPagesToClean);
	MmuLock::Lock();
	iPagesToClean = aPagesToClean;
	iMaxOldestPages = MaxU(KDefaultMaxOldestPages,
						   RequiredOldestPages(iPagesToClean, iCleanInSequence));
	MmuLock::Unlock();
	TRACE(("WDP: Maximum %d oldest pages", iMaxOldestPages));	
	}


TUint DPager::PagesToClean()
	{
	return iPagesToClean;
	}


void DPager::SetCleanInSequence(TBool aCleanInSequence)
	{
	TRACE(("WDP: Sequential page colour set to %d", aCleanInSequence));
	MmuLock::Lock();
	iCleanInSequence = aCleanInSequence;
	iMaxOldestPages = MaxU(KDefaultMaxOldestPages,
						   RequiredOldestPages(iPagesToClean, iCleanInSequence));
	MmuLock::Unlock();	
	TRACE(("WDP: Maximum %d oldest pages", iMaxOldestPages));
	}


// WARNING THIS METHOD MAY HOLD THE RAM ALLOC LOCK FOR EXCESSIVE PERIODS.  DON'T USE THIS IN ANY PRODUCTION CODE.
void DPager::FlushAll()
	{
	NKern::ThreadEnterCS();
	RamAllocLock::Lock();
	PageCleaningLock::Lock();

	TRACE(("DPager::FlushAll() live list young=%d old=%d min=%d free=%d max=%d",iYoungCount,iOldCount,iMinimumPageCount,iNumberOfFreePages,iMaximumPageCount));

	// look at all RAM pages in the system, and unmap all those used for paging
	const TUint32* piMap = (TUint32*)KPageInfoMap;
	const TUint32* piMapEnd = piMap+(KNumPageInfoPages>>5);
	SPageInfo* pi = (SPageInfo*)KPageInfoLinearBase;
	MmuLock::Lock();
	do
		{
		SPageInfo* piNext = pi+(KPageInfosPerPage<<5);
		for(TUint32 piFlags=*piMap++; piFlags; piFlags>>=1)
			{
			if(!(piFlags&1))
				{
				pi += KPageInfosPerPage;
				continue;
				}
			SPageInfo* piEnd = pi+KPageInfosPerPage;
			do
				{
				SPageInfo::TPagedState state = pi->PagedState();
				if (state==SPageInfo::EPagedYoung || state==SPageInfo::EPagedOld ||
					state==SPageInfo::EPagedOldestClean || state==SPageInfo::EPagedOldestDirty)
					{
					if (pi->Type() != SPageInfo::EUnused)
						{
						TInt r = StealPage(pi);
						if(r==KErrNone)
							AddAsFreePage(pi);
						MmuLock::Flash();
						}
					}
				++pi;
				if(((TUint)pi&(0xf<<KPageInfoShift))==0)
					{
					MmuLock::Flash(); // every 16 page infos
					}
				}
			while(pi<piEnd);
			}
		pi = piNext;
		}
	while(piMap<piMapEnd);
	MmuLock::Unlock();
	PageCleaningLock::Unlock();

	// reduce live page list to a minimum
	while(GetFreePages(1)) {}; 

	TRACE(("DPager::FlushAll() end with young=%d old=%d min=%d free=%d max=%d",iYoungCount,iOldCount,iMinimumPageCount,iNumberOfFreePages,iMaximumPageCount));

	RamAllocLock::Unlock();
	NKern::ThreadLeaveCS();
	}


TInt DPager::FlushRegion(DMemModelProcess* aProcess, TLinAddr aStartAddress, TUint aSize)
	{
	if (aSize == 0)
		return KErrNone;

	// find mapping
	NKern::ThreadEnterCS();
	TUint offsetInMapping;
	TUint mapInstanceCount;
	DMemoryMapping* mapping = MM::FindMappingInProcess(aProcess, aStartAddress, aSize,
													   offsetInMapping, mapInstanceCount);
	if (!mapping)
		{
		NKern::ThreadLeaveCS();
		return KErrBadDescriptor;
		}

	// check whether memory is demand paged
	MmuLock::Lock();
	DMemoryObject* memory = mapping->Memory();
	if(mapInstanceCount != mapping->MapInstanceCount() || memory == NULL || !memory->IsDemandPaged())
		{
		MmuLock::Unlock();
		mapping->Close();
		NKern::ThreadLeaveCS();
		return KErrNone;
		}

	TRACE(("DPager::FlushRegion: %O %08x +%d", aProcess, aStartAddress, aSize));
	if (!K::Initialising)
		TRACE2(("  context %T %d", NCurrentThread(), NKern::CurrentContext()));

	// why did we not get assertion failures before I added this?
	__NK_ASSERT_DEBUG(!Kern::CurrentThread().IsRealtime());

	// acquire necessary locks
	MmuLock::Unlock();
	RamAllocLock::Lock();
	PageCleaningLock::Lock();
	MmuLock::Lock();

	// find region in memory object
	TUint startPage = (offsetInMapping >> KPageShift) + mapping->iStartIndex;
	TUint sizeInPages = ((aStartAddress & KPageMask) + aSize - 1) >> KPageShift;
	TUint endPage = startPage + sizeInPages;
	TRACE2(("DPager::FlushRegion: page range is %d to %d", startPage, endPage));
	
	// attempt to flush each page
	TUint index = startPage;
	while (mapping->MapInstanceCount() == mapInstanceCount &&
		   mapping->Memory() && index <= endPage)
		{
		TRACE2(("DPager::FlushRegion: flushing page %d", index));
		TPhysAddr physAddr = memory->iPages.PhysAddr(index);
		
		if (physAddr != KPhysAddrInvalid)
			{
			TRACE2(("DPager::FlushRegion: phys addr is %08x", physAddr));
			SPageInfo* pi = SPageInfo::SafeFromPhysAddr(physAddr);
			if (pi)
				{
				__NK_ASSERT_DEBUG(pi->Type() == SPageInfo::EManaged);
				SPageInfo::TPagedState state = pi->PagedState();
				if (state==SPageInfo::EPagedYoung || state==SPageInfo::EPagedOld ||
					state==SPageInfo::EPagedOldestClean || state==SPageInfo::EPagedOldestDirty)
					{
					TRACE2(("DPager::FlushRegion: attempt to steal page"));
					TInt r = StealPage(pi);
					if(r==KErrNone)
						{
						TRACE2(("DPager::FlushRegion: attempt to page out %08x", physAddr));
						AddAsFreePage(pi);
						TRACE2(("DPager::FlushRegion: paged out %08x", physAddr));
						}
					else
						TRACE2(("DPager::FlushRegion: page out %08x failed with %d", physAddr, r));
					}
				}
			}
		
		MmuLock::Flash();
		++index;
		}
	
	MmuLock::Unlock();
	PageCleaningLock::Unlock();
	RamAllocLock::Unlock();
	mapping->Close();
	NKern::ThreadLeaveCS();
	TRACE2(("DPager::FlushRegion: done"));
	return KErrNone;
	}


void DPager::GetLiveListInfo(SVMCacheInfo& aInfo)
	{
	MmuLock::Lock(); // ensure consistent set of values are read...
	aInfo.iMinSize = iMinimumPageCount<<KPageShift;
	aInfo.iMaxSize = iMaximumPageCount<<KPageShift;
	aInfo.iCurrentSize = (iMinimumPageCount+iNumberOfFreePages)<<KPageShift;
	aInfo.iMaxFreeSize = iNumberOfFreePages<<KPageShift;
	MmuLock::Unlock();
	}


void DPager::GetEventInfo(SVMEventInfo& aInfoOut)
	{
	MmuLock::Lock(); // ensure consistent set of values are read...
	aInfoOut = iEventInfo;
	MmuLock::Unlock();
	}


void DPager::ResetEventInfo()
	{
	MmuLock::Lock();
	memclr(&iEventInfo, sizeof(iEventInfo));
	MmuLock::Unlock();
	}


TInt TestPageState(TLinAddr aAddr)
	{
	DMemModelProcess* process = (DMemModelProcess*)TheCurrentThread->iOwningProcess;
	// Get the os asid of current thread's process so no need to open a reference on it.
	TInt osAsid = process->OsAsid();
	TPte* ptePtr = 0;
	TPte pte = 0;
	TInt r = 0;
	SPageInfo* pageInfo = NULL;

	NKern::ThreadEnterCS();

	TUint offsetInMapping;
	TUint mapInstanceCount;
	DMemoryMapping* mapping = MM::FindMappingInAddressSpace(osAsid, aAddr, 1, offsetInMapping, mapInstanceCount);

	MmuLock::Lock();

	if(mapping)
		{
		DMemoryObject* memory = mapping->Memory();
		if(mapInstanceCount == mapping->MapInstanceCount() && memory)
			{
			DMemoryManager* manager = memory->iManager;
			if(manager==TheCodePagedMemoryManager)
				r |= EPageStateInRamCode|EPageStatePaged;
			}
		}

	ptePtr = Mmu::SafePtePtrFromLinAddr(aAddr,osAsid);
	if (!ptePtr)
		goto done;
	pte = *ptePtr;
	if (pte == KPteUnallocatedEntry)
		goto done;		
	r |= EPageStatePtePresent;
	if (pte!=Mmu::MakePteInaccessible(pte,0))
		r |= EPageStatePteValid;
	
	pageInfo = SPageInfo::SafeFromPhysAddr(pte&~KPageMask);
	if(pageInfo)
		{
		r |= pageInfo->Type();
		r |= pageInfo->PagedState()<<8;
		}
done:
	MmuLock::Unlock();
	if(mapping)
		mapping->Close();
	NKern::ThreadLeaveCS();
	return r;
	}



TInt VMHalFunction(TAny*, TInt aFunction, TAny* a1, TAny* a2)
	{
	switch(aFunction)
		{
	case EVMHalFlushCache:
		if(!TheCurrentThread->HasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by VMHalFunction(EVMHalFlushCache)")))
			K::UnlockedPlatformSecurityPanic();
		ThePager.FlushAll();
		return KErrNone;

	case EVMHalSetCacheSize:
		{
		if(!TheCurrentThread->HasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by VMHalFunction(EVMHalSetCacheSize)")))
			K::UnlockedPlatformSecurityPanic();
		TUint min = TUint(a1)>>KPageShift;
		if(TUint(a1)&KPageMask)
			++min;
		TUint max = TUint(a2)>>KPageShift;
		if(TUint(a2)&KPageMask)
			++max;
		return ThePager.ResizeLiveList(min,max);
		}

	case EVMHalGetCacheSize:
		{
		SVMCacheInfo info;
		ThePager.GetLiveListInfo(info);
		kumemput32(a1,&info,sizeof(info));
		}
		return KErrNone;

	case EVMHalGetEventInfo:
		{
		SVMEventInfo info;
		ThePager.GetEventInfo(info);
		Kern::InfoCopy(*(TDes8*)a1,(TUint8*)&info,sizeof(info));
		}
		return KErrNone;

	case EVMHalResetEventInfo:
		ThePager.ResetEventInfo();
		return KErrNone;

#ifdef __SUPPORT_DEMAND_PAGING_EMULATION__
	case EVMHalGetOriginalRomPages:
		RomOriginalPages(*((TPhysAddr**)a1), *((TUint*)a2));
		return KErrNone;
#endif

	case EVMPageState:
		return TestPageState((TLinAddr)a1);

	case EVMHalGetSwapInfo:
		{
		if ((K::MemModelAttributes & EMemModelAttrDataPaging) == 0)
			return KErrNotSupported;
		SVMSwapInfo info;
		GetSwapInfo(info);
		kumemput32(a1,&info,sizeof(info));
		}
		return KErrNone;

	case EVMHalGetThrashLevel:
		return TheThrashMonitor.ThrashLevel();

	case EVMHalSetSwapThresholds:
		{
		if(!TheCurrentThread->HasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by VMHalFunction(EVMHalSetSwapThresholds)")))
			K::UnlockedPlatformSecurityPanic();
		if ((K::MemModelAttributes & EMemModelAttrDataPaging) == 0)
			return KErrNotSupported;
		SVMSwapThresholds thresholds;
		kumemget32(&thresholds,a1,sizeof(thresholds));
		return SetSwapThresholds(thresholds);
		}

	case EVMHalSetThrashThresholds:
		if(!TheCurrentThread->HasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by VMHalFunction(EVMHalSetThrashThresholds)")))
			K::UnlockedPlatformSecurityPanic();
		return TheThrashMonitor.SetThresholds((TUint)a1, (TUint)a2);

#ifdef __DEMAND_PAGING_BENCHMARKS__
	case EVMHalGetPagingBenchmark:
		{
		TUint index = (TInt) a1;
		if (index >= EMaxPagingBm)
			return KErrNotFound;
		SPagingBenchmarkInfo info;
		ThePager.ReadBenchmarkData((TPagingBenchmark)index, info);
		kumemput32(a2,&info,sizeof(info));
		}		
		return KErrNone;
		
	case EVMHalResetPagingBenchmark:
		{
		TUint index = (TInt) a1;
		if (index >= EMaxPagingBm)
			return KErrNotFound;
		ThePager.ResetBenchmarkData((TPagingBenchmark)index);
		}
		return KErrNone;
#endif

	case EVMHalGetPhysicalAccessSupported:
		if ((K::MemModelAttributes & EMemModelAttrDataPaging) == 0)
			return KErrNotSupported;
		return GetPhysicalAccessSupported();
		
	case EVMHalGetUsePhysicalAccess:
		if ((K::MemModelAttributes & EMemModelAttrDataPaging) == 0)
			return KErrNotSupported;
		return GetUsePhysicalAccess();

	case EVMHalSetUsePhysicalAccess:
		if(!TheCurrentThread->HasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by VMHalFunction(EVMHalSetUsePhysicalAccess)")))
			K::UnlockedPlatformSecurityPanic();
		if ((K::MemModelAttributes & EMemModelAttrDataPaging) == 0)
			return KErrNotSupported;
		if ((TUint)a1 > 1)
			return KErrArgument;
		SetUsePhysicalAccess((TBool)a1);
		return KErrNone;
		
	case EVMHalGetPreferredDataWriteSize:
		if ((K::MemModelAttributes & EMemModelAttrDataPaging) == 0)
			return KErrNotSupported;
		return GetPreferredDataWriteSize();
		
	case EVMHalGetDataWriteSize:
		if ((K::MemModelAttributes & EMemModelAttrDataPaging) == 0)
			return KErrNotSupported;
		return __e32_find_ms1_32(ThePager.PagesToClean());
		
	case EVMHalSetDataWriteSize:
		if(!TheCurrentThread->HasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by VMHalFunction(EVMHalSetDataWriteSize)")))
			K::UnlockedPlatformSecurityPanic();
		if ((K::MemModelAttributes & EMemModelAttrDataPaging) == 0)
			return KErrNotSupported;
		return SetDataWriteSize((TUint)a1);

#ifdef _DEBUG
	case EVMHalDebugSetFail:
		{
		TUint context = (TUint)a1;
		if (context >= EMaxPagingErrorContext)
			return KErrArgument;
		__e32_atomic_store_ord32(&(ThePager.iDebugFailContext), context);
		return KErrNone;
		}
#endif

	default:
		return KErrNotSupported;
		}
	}


#ifdef __DEMAND_PAGING_BENCHMARKS__

void DPager::ResetBenchmarkData(TPagingBenchmark aBm)
    {
    SPagingBenchmarkInfo& info = iBenchmarkInfo[aBm];
	__SPIN_LOCK_IRQ(iBenchmarkLock);
    info.iCount = 0;
    info.iTotalTime = 0;
    info.iMaxTime = 0;
    info.iMinTime = KMaxTInt;
	__SPIN_UNLOCK_IRQ(iBenchmarkLock);
    }
 
void DPager::RecordBenchmarkData(TPagingBenchmark aBm, TUint32 aStartTime, TUint32 aEndTime, TUint aCount)
    {
    SPagingBenchmarkInfo& info = iBenchmarkInfo[aBm];
#if !defined(HIGH_RES_TIMER) || defined(HIGH_RES_TIMER_COUNTS_UP)
    TInt64 elapsed = aEndTime - aStartTime;
#else
    TInt64 elapsed = aStartTime - aEndTime;
#endif
	__SPIN_LOCK_IRQ(iBenchmarkLock);
    info.iCount +=  aCount;
    info.iTotalTime += elapsed;
    if (elapsed > info.iMaxTime)
        info.iMaxTime = elapsed;
    if (elapsed < info.iMinTime)
        info.iMinTime = elapsed;
	__SPIN_UNLOCK_IRQ(iBenchmarkLock);
    }

void DPager::ReadBenchmarkData(TPagingBenchmark aBm, SPagingBenchmarkInfo& aDataOut)
	{
	__SPIN_LOCK_IRQ(iBenchmarkLock);
	aDataOut = iBenchmarkInfo[aBm];
	__SPIN_UNLOCK_IRQ(iBenchmarkLock);
	}

#endif //__DEMAND_PAGING_BENCHMARKS__


//
// Paging request management...
//

//
// DPagingRequestBase
//


TLinAddr DPagingRequestBase::MapPages(TUint aColour, TUint aCount, TPhysAddr* aPages)
	{
	__NK_ASSERT_DEBUG(iMutex->iCleanup.iThread == &Kern::CurrentThread());
	return iTempMapping.Map(aPages,aCount,aColour);
	}


void DPagingRequestBase::UnmapPages(TBool aIMBRequired)
	{
	__NK_ASSERT_DEBUG(iMutex->iCleanup.iThread == &Kern::CurrentThread());
	iTempMapping.Unmap(aIMBRequired);
	}


//
// DPageReadRequest
//


TInt DPageReadRequest::iAllocNext = 0;


TUint DPageReadRequest::ReservedPagesRequired()
	{
	return iAllocNext*EMaxPages;
	}


DPageReadRequest::DPageReadRequest(DPagingRequestPool::TGroup& aPoolGroup) :
	iPoolGroup(aPoolGroup)
	{
	iTempMapping.Alloc(EMaxPages);
	}


TInt DPageReadRequest::Construct()
	{
	// allocate id and mutex...
	TUint id = (TUint)__e32_atomic_add_ord32(&iAllocNext, 1);
	_LIT(KLitPagingRequest,"PageReadRequest-");
	TBuf<sizeof("PageReadRequest-")+10> mutexName(KLitPagingRequest);
	mutexName.AppendNum(id);
	TInt r = K::MutexCreate(iMutex, mutexName, NULL, EFalse, KMutexOrdPageIn);
	if(r!=KErrNone)
		return r;

	// create memory buffer...
	TUint bufferSize = EMaxPages+1;
	DMemoryObject* bufferMemory;
	r = MM::MemoryNew(bufferMemory,EMemoryObjectUnpaged,bufferSize,EMemoryCreateNoWipe);
	if(r!=KErrNone)
		return r;
	MM::MemorySetLock(bufferMemory,iMutex);
	TPhysAddr physAddr;
	r = MM::MemoryAllocContiguous(bufferMemory,0,bufferSize,0,physAddr);
	(void)physAddr;
	if(r!=KErrNone)
		return r;
	DMemoryMapping* bufferMapping;
	r = MM::MappingNew(bufferMapping,bufferMemory,ESupervisorReadWrite,KKernelOsAsid);
	if(r!=KErrNone)
		return r;
	iBuffer = MM::MappingBase(bufferMapping);

	return r;
	}


void DPageReadRequest::Release()
	{
	NKern::LockSystem();
	ResetUse();
	Signal();
	}


void DPageReadRequest::Wait()
	{
	__ASSERT_SYSTEM_LOCK;
	++iUsageCount;
	TInt r = iMutex->Wait();
	__NK_ASSERT_ALWAYS(r == KErrNone);
	}


void DPageReadRequest::Signal()
	{
	__ASSERT_SYSTEM_LOCK;
	__NK_ASSERT_DEBUG(iUsageCount > 0);
	if (--iUsageCount == 0)
		iPoolGroup.iFreeList.AddHead(&iLink);
	iMutex->Signal();
	}


void DPageReadRequest::SetUseContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	__ASSERT_SYSTEM_LOCK;
	__NK_ASSERT_DEBUG(aMemory != NULL && aCount <= EMaxPages);
	__NK_ASSERT_DEBUG(iMemory == NULL);
	iMemory = aMemory;
	iIndex = aIndex;
	iCount = aCount;
	}


void DPageReadRequest::ResetUse()
	{
	__ASSERT_SYSTEM_LOCK;
	__NK_ASSERT_DEBUG(iMemory != NULL);
	iMemory = NULL;
	}


 TBool DPageReadRequest::IsCollisionContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	__ASSERT_SYSTEM_LOCK;
	return iMemory == aMemory && aIndex < iIndex + iCount && aIndex + aCount > iIndex;
	}


TBool DPageReadRequest::CheckUseContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	return iMemory == aMemory && iIndex == aIndex && iCount == aCount;
	}


//
// DPageWriteRequest
//


DPageWriteRequest::DPageWriteRequest()
	{
	iMutex = ThePageCleaningLock;
	iTempMapping.Alloc(EMaxPages);
	}


void DPageWriteRequest::Release()
	{
	NKern::LockSystem();
	ResetUse();
	NKern::UnlockSystem();
	}


void DPageWriteRequest::SetUseDiscontiguous(DMemoryObject** aMemory, TUint* aIndex, TUint aCount)
	{
	__ASSERT_SYSTEM_LOCK;
	__NK_ASSERT_DEBUG(iUseRegionCount == 0);
	__NK_ASSERT_DEBUG(aCount > 0 && aCount <= EMaxPages);
	for (TUint i = 0 ; i < aCount ; ++i)
		{
		iUseRegionMemory[i] = aMemory[i];
		iUseRegionIndex[i] = aIndex[i];
		}
	iUseRegionCount = aCount;
	}


void DPageWriteRequest::ResetUse()
	{
	__ASSERT_SYSTEM_LOCK;
	__NK_ASSERT_DEBUG(iUseRegionCount > 0);
	iUseRegionCount = 0;
	}


TBool DPageWriteRequest::CheckUseContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	if (iUseRegionCount != aCount)
		return EFalse;
	for (TUint i = 0 ; i < iUseRegionCount ; ++i)
		{
		if (iUseRegionMemory[i] != aMemory || iUseRegionIndex[i] != aIndex + i)
			return EFalse;
		}
	return ETrue;
	}


 TBool DPageWriteRequest::IsCollisionContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	// note this could be optimised as most of the time we will be checking read/read collusions,
	// both of which will be contiguous
	__ASSERT_SYSTEM_LOCK;
	for (TUint i = 0 ; i < iUseRegionCount ; ++i)
		{
		if (iUseRegionMemory[i] == aMemory &&
			TUint(iUseRegionIndex[i] - aIndex) < aCount)
			return ETrue;
		}
	return EFalse;
	}

//
// DPagingRequestPool
//

DPagingRequestPool::DPagingRequestPool(TUint aNumPageReadRequest, TBool aWriteRequest)
	: iPageReadRequests(aNumPageReadRequest)
	{
	TUint i;
	for(i=0; i<aNumPageReadRequest; ++i)
		{
		DPageReadRequest* req = new DPageReadRequest(iPageReadRequests);
		__NK_ASSERT_ALWAYS(req);
		TInt r = req->Construct();
		__NK_ASSERT_ALWAYS(r==KErrNone);
		iPageReadRequests.iRequests[i] = req;
		iPageReadRequests.iFreeList.Add(&req->iLink);
		}

	if (aWriteRequest)
		{
		iPageWriteRequest = new DPageWriteRequest();
		__NK_ASSERT_ALWAYS(iPageWriteRequest);
		}
	}


DPagingRequestPool::~DPagingRequestPool()
	{
	__NK_ASSERT_ALWAYS(0); // deletion not implemented
	}


DPageReadRequest* DPagingRequestPool::AcquirePageReadRequest(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	NKern::LockSystem();

	DPageReadRequest* req;
	
	// check for collision with existing write
	if(iPageWriteRequest && iPageWriteRequest->IsCollisionContiguous(aMemory,aIndex,aCount))
		{
		NKern::UnlockSystem();
		PageCleaningLock::Lock();
		PageCleaningLock::Unlock();
		return 0; // caller expected to retry if needed
		}

	// get a request object to use...
	req = iPageReadRequests.GetRequest(aMemory,aIndex,aCount);

	// check no new read or write requests collide with us...
	if ((iPageWriteRequest && iPageWriteRequest->IsCollisionContiguous(aMemory,aIndex,aCount)) ||
		iPageReadRequests.FindCollisionContiguous(aMemory,aIndex,aCount))
		{
		// another operation is colliding with this region, give up and retry...
		req->Signal();
		return 0; // caller expected to retry if needed
		}

	// we have a request object which we can use...
	req->SetUseContiguous(aMemory,aIndex,aCount);

	NKern::UnlockSystem();
	return (DPageReadRequest*)req;
	}


DPageWriteRequest* DPagingRequestPool::AcquirePageWriteRequest(DMemoryObject** aMemory, TUint* aIndex, TUint aCount)
	{
	__NK_ASSERT_DEBUG(iPageWriteRequest);
	__NK_ASSERT_DEBUG(PageCleaningLock::IsHeld());

	NKern::LockSystem();

	// Collision with existing read requests is not possible here.  For a page to be read it must
	// not be present, and for it to be written it must be present and dirty.  There is no way for a
	// page to go between these states without an intervening read on an uninitialised (freshly
	// committed) page, which will wait on the first read request.  In other words something like
	// this:
	//
	//   read (blocks), decommit, re-commit, read (waits on mutex), write (now no pending reads!)
	//
	// Note that a read request can be outstanding and appear to collide with this write, but only
	// in the case when the thread making the read has blocked just after acquiring the request but
	// before it checks whether the read is still necessasry.  This makes it difficult to assert
	// that no collisions take place.
	
	iPageWriteRequest->SetUseDiscontiguous(aMemory,aIndex,aCount);
	NKern::UnlockSystem();
	
	return iPageWriteRequest;
	}


DPagingRequestPool::TGroup::TGroup(TUint aNumRequests)
	{
	iNumRequests = aNumRequests;
	iRequests = new DPageReadRequest*[aNumRequests];
	__NK_ASSERT_ALWAYS(iRequests);
	}


DPageReadRequest* DPagingRequestPool::TGroup::FindCollisionContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	__ASSERT_SYSTEM_LOCK;
	DPageReadRequest** ptr = iRequests;
	DPageReadRequest** ptrEnd = ptr+iNumRequests;
	while(ptr<ptrEnd)
		{
		DPageReadRequest* req = *ptr++;
		if(req->IsCollisionContiguous(aMemory,aIndex,aCount))
			return req;
		}
	return 0;
	}


static TUint32 RandomSeed = 33333;

DPageReadRequest* DPagingRequestPool::TGroup::GetRequest(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	__NK_ASSERT_DEBUG(iNumRequests > 0);

	// try using an existing request which collides with this region...
	DPageReadRequest* req  = FindCollisionContiguous(aMemory,aIndex,aCount);
	if(!req)
		{
		// use a free request...
		SDblQueLink* first = iFreeList.GetFirst();
		if(first)
			{
			// free requests aren't being used...
			req = _LOFF(first, DPageReadRequest, iLink);
			__NK_ASSERT_DEBUG(req->ThreadsWaiting() == 0);
			}
		else
			{
			// pick a random request...
			RandomSeed = RandomSeed*69069+1; // next 'random' number
			TUint index = (TUint64(RandomSeed) * TUint64(iNumRequests)) >> 32;
			req = iRequests[index];
			__NK_ASSERT_DEBUG(req->ThreadsWaiting() > 0); // we only pick random when none are free
			}
		}

	// wait for chosen request object...
	req->Wait();

	return req;
	}


/**
Register the specified paging device with the kernel.

@param aDevice	A pointer to the paging device to install

@return KErrNone on success
*/
EXPORT_C TInt Kern::InstallPagingDevice(DPagingDevice* aDevice)
	{
	TRACEB(("Kern::InstallPagingDevice(0x%08x) name='%s' type=%d",aDevice,aDevice->iName,aDevice->iType));

	__NK_ASSERT_DEBUG(!ThePager.CacheInitialised());
	__NK_ASSERT_ALWAYS(aDevice->iReadUnitShift <= KPageShift);

	TInt r = KErrNotSupported;	// Will return this if unsupported device type is installed

	// create the pools of page out and page in requests...
	const TBool writeReq = (aDevice->iType & DPagingDevice::EData) != 0;
	aDevice->iRequestPool = new DPagingRequestPool(KPagingRequestsPerDevice, writeReq);
	if(!aDevice->iRequestPool)
		{
		r = KErrNoMemory;
		goto exit;
		}

	if(aDevice->iType & DPagingDevice::ERom)
		{
		r = TheRomMemoryManager->InstallPagingDevice(aDevice);
		if(r!=KErrNone)
			goto exit;
		}

	if(aDevice->iType & DPagingDevice::ECode)
		{
		r = TheCodePagedMemoryManager->InstallPagingDevice(aDevice);
		if(r!=KErrNone)
			goto exit;
		}

	if(aDevice->iType & DPagingDevice::EData)
		{
		r = TheDataPagedMemoryManager->InstallPagingDevice(aDevice);
		if(r!=KErrNone)
			goto exit;
		}

 	if (K::MemModelAttributes & (EMemModelAttrRomPaging | EMemModelAttrCodePaging | EMemModelAttrDataPaging))
		TheThrashMonitor.Start();
	
 	if (K::MemModelAttributes & EMemModelAttrDataPaging)
		PageCleaner::Start();

exit:
	TRACEB(("Kern::InstallPagingDevice returns %d",r));
	return r;
	}



//
// DDemandPagingLock
//

EXPORT_C DDemandPagingLock::DDemandPagingLock()
	: iReservedPageCount(0), iLockedPageCount(0), iPinMapping(0)
	{
	}


EXPORT_C TInt DDemandPagingLock::Alloc(TInt aSize)
	{
	TRACEP(("DDemandPagingLock[0x%08x]::Alloc(0x%x)",this,aSize));
	iMaxPageCount = ((aSize-1+KPageMask)>>KPageShift)+1;

	TInt r = KErrNoMemory;

	NKern::ThreadEnterCS();

	TUint maxPt = DVirtualPinMapping::MaxPageTables(iMaxPageCount);
	// Note, we need to reserve whole pages even for page tables which are smaller
	// because pinning can remove the page from live list...
	TUint reserve = iMaxPageCount+maxPt*KNumPagesToPinOnePageTable;
	if(ThePager.ReservePages(reserve,(TUint&)iReservedPageCount))
		{
		iPinMapping = DVirtualPinMapping::New(iMaxPageCount);
		if(iPinMapping)
			r = KErrNone;
		else
			ThePager.UnreservePages((TUint&)iReservedPageCount);
		}

	NKern::ThreadLeaveCS();
	TRACEP(("DDemandPagingLock[0x%08x]::Alloc returns %d, iMaxPageCount=%d, iReservedPageCount=%d",this,r,iMaxPageCount,iReservedPageCount));
	return r;
	}


EXPORT_C void DDemandPagingLock::Free()
	{
	TRACEP(("DDemandPagingLock[0x%08x]::Free()"));
	Unlock();
	NKern::ThreadEnterCS();
	DVirtualPinMapping* pinMapping = (DVirtualPinMapping*)__e32_atomic_swp_ord_ptr(&iPinMapping, 0);
	if (pinMapping)
		pinMapping->Close();
	NKern::ThreadLeaveCS();
	ThePager.UnreservePages((TUint&)iReservedPageCount);
	}


EXPORT_C TInt DDemandPagingLock::Lock(DThread* aThread, TLinAddr aStart, TInt aSize)
	{
//	TRACEP(("DDemandPagingLock[0x%08x]::Lock(0x%08x,0x%08x,0x%08x)",this,aThread,aStart,aSize));
	__NK_ASSERT_ALWAYS(!iLockedPageCount); // lock already used

	// calculate the number of pages that need to be locked...
	TUint mask=KPageMask;
	TUint offset=aStart&mask;
	TInt numPages = (aSize+offset+mask)>>KPageShift;

	// Should never be asked to lock more pages than are allocated to this object.
	__NK_ASSERT_ALWAYS(numPages <= iMaxPageCount);

	NKern::ThreadEnterCS();

	// find mapping which covers the specified region...
	TUint offsetInMapping;
	TUint mapInstanceCount;
	DMemoryMapping* mapping = MM::FindMappingInThread((DMemModelThread*)aThread, aStart, aSize, offsetInMapping, mapInstanceCount);
	if(!mapping)
		{
		NKern::ThreadLeaveCS();
		return KErrBadDescriptor;
		}

	MmuLock::Lock(); 
	DMemoryObject* memory = mapping->Memory();
	if(mapInstanceCount != mapping->MapInstanceCount() || !memory)
		{// Mapping has been reused or no memory.
		MmuLock::Unlock();
		mapping->Close();
		NKern::ThreadLeaveCS();
		return KErrBadDescriptor;
		}

	if(!memory->IsDemandPaged())
		{
		// memory not demand paged, so we have nothing to do...
		MmuLock::Unlock();
		mapping->Close();
		NKern::ThreadLeaveCS();
		return KErrNone;
		}

	// Open a reference on the memory so it doesn't get deleted.
	memory->Open();
	MmuLock::Unlock();

	// pin memory...
	TUint index = (offsetInMapping>>KPageShift)+mapping->iStartIndex;
	TUint count = ((offsetInMapping&KPageMask)+aSize+KPageMask)>>KPageShift;
	TInt r = ((DVirtualPinMapping*)iPinMapping)->Pin(	memory,index,count,mapping->Permissions(),
														mapping, mapInstanceCount);

	if(r==KErrNotFound)
		{
		// some memory wasn't present, so treat this as an error...
		memory->Close();
		mapping->Close();
		NKern::ThreadLeaveCS();
		return KErrBadDescriptor;
		}

	// we can't fail to pin otherwise...
	__NK_ASSERT_DEBUG(r!=KErrNoMemory); // separate OOM assert to aid debugging
	__NK_ASSERT_ALWAYS(r==KErrNone);

	// indicate that we have actually pinned...
	__NK_ASSERT_DEBUG(iLockedPageCount==0);
	iLockedPageCount = count;

	// cleanup...
	memory->Close();
	mapping->Close();
	NKern::ThreadLeaveCS();

	return 1;
	}


EXPORT_C void DDemandPagingLock::DoUnlock()
	{
	NKern::ThreadEnterCS();
	((DVirtualPinMapping*)iPinMapping)->Unpin();
	__NK_ASSERT_DEBUG(iLockedPageCount);
	iLockedPageCount = 0;
	NKern::ThreadLeaveCS();
	}



//
// PageCleaningLock
//

_LIT(KLitPageCleaningLock,"PageCleaningLock");

void PageCleaningLock::Init()
	{
	__NK_ASSERT_DEBUG(!ThePageCleaningLock);
	TInt r = Kern::MutexCreate(ThePageCleaningLock, KLitPageCleaningLock, KMutexOrdPageOut);
	__NK_ASSERT_ALWAYS(r == KErrNone);
	}

void PageCleaningLock::Lock()
	{
	Kern::MutexWait(*ThePageCleaningLock);
	}


void PageCleaningLock::Unlock()
	{
	Kern::MutexSignal(*ThePageCleaningLock);
	}

TBool PageCleaningLock::IsHeld()
	{
	return ThePageCleaningLock->iCleanup.iThread == &Kern::CurrentThread();
	}
