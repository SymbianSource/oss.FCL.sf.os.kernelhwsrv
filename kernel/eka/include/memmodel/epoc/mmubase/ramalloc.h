// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\memmodel\epoc\mmubase\ramalloc.h
// 
//

/**
 @file
 @internalComponent
*/

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

/** The number of types of pages that can't be moved or discarded
*/
const TInt KPageImmovable = EPageMovable;

/** The lowest enum from TZonePageType that DRamAllocater will allocate
*/
const TUint KPageTypeAllocBase = EPageFixed;

/** Element 0 of SZone::iBma is the bit map of all allocated pages 
regardless of type.
*/
const TUint KBmaAllPages = 0;

/** The maximum number of freeable contiguous pages that can be found by
DRamAllocator::FindFreeableContiguousPages and RamCacheBase::AllocFreeContiguousPages.
*/
const TUint KMaxFreeableContiguousPages = 16;


/** Structure to store the information on a zone.
*/
struct SZone
	{
	TBitMapAllocator* iBma[EPageTypes];	/**< Pointers to bit map allocators for each type of page*/
	TPhysAddr iPhysBase;	/**< physical base address of this zone*/
	TPhysAddr iPhysEnd;		/**< physical end address of this zone*/
	TUint32 iPhysPages;		/**< the total no. of pages that are in this zone*/
	TUint32 iAllocPages[EPageTypes]; /**< number of pages allocated of each type*/
	TUint32 iFreePages;		/**< number of pages free*/
	SDblQueLink iPrefLink;	/**< The link of this zone into the preference ordered list*/
	TUint iPrefRank;		/**< The rank of this zone in the preference ordered list.*/
	TUint iId;				/**< the ID of the zone*/
	TUint iFlags;			/**< bit flags for this zone, all flags masked KRamZoneFlagInvalid are 
								 for use by the kernel only*/
	TUint8 iPref;			/**< the preference of the zone, lower preference zones are used first*/
	};

/** The different stages of a general defragmentation*/
enum TGenDefragStage
	{
	EGenDefragStage0,	/**< 	This stage should discard any pages needed to
								allow the required movable pages to fit into the
								RAM zones to be in use after the general defrag.*/
	EGenDefragStage1,	/**< 	This stage should reduce the number of RAM
								zones in use to the minmum required.*/
	EGenDefragStage2,	/**<	This stage clears space in most preferable RAM 
								zones for fixed page allocations by placing movable 
								and discardable pages in the least preferable RAM 
								zones in use.*/
	EGenDefragStageEnd,
	};

// forward declare
struct SPageInfo;

class DRamAllocator : public DBase
	{
private:
	/** Used with NextAllocZone to specify which order to run through the zones
	*/
	enum TZoneSearchState
		{
		EZoneSearchPref,
		EZoneSearchAddr,
		EZoneSearchEnd,
		};

	enum TPanic
		{
		EDoNotUse=0,
		EDoMarkPagesAllocated1=1,
		EAllocRamPagesInconsistent=2,
		EZonesTooNumerousOrFew=3,	/**<There are too many or too few RAM zones*/
		EZonesNotDistinct=4,		/**<Some of the RAM zones overlap*/
		EZonesIncomplete=5,			/**<The RAM zones don't cover all the RAM, either because the zones 
										are too small or at least one zone is mapped to an incorrect address*/
		EZonesIDInvalid=6,			/**<KRamZoneInvalidId can't be used for any zone's ID*/
		EZonesIDNotUnique=7,		/**<At least two zones share the same ID*/
		EZonesAlignment=8,			/**<Zones are not aligned to page boundaries*/
		EZonesCountErr=9,			/**<The count of free and alloc'd zone pages is corrupted*/
		EZonesCallbackErr=10,		/**<Unexpected error when zone call back invoked*/
		EZonesFlagsInvalid=11,		/**<Attempt to create a zone with flags set to invalid values*/
		ECreateNoMemory=12,			/**<Not enough free RAM to create a DRamAllocator object*/
		ECreateInvalidReserveBank=13,/**<A specified reserve bank could not be reserved*/
		ECreateInvalidRamBanks=14,	/**<The specified RAM banks are invalid*/
		EFreeingLockedPage=15, 		/**< A locked page was requested for freeing*/
		};

public:

	static DRamAllocator* New(const SRamInfo& aInfo, const SRamZone* aZoneInfo, TRamZoneCallback aZoneCallback);
	static void Panic(TPanic aPanic);
	TUint TotalPhysicalRamPages();
	TInt FreeRamInBytes();
	TUint FreeRamInPages();
	TInt ClaimPhysicalRam(TPhysAddr aBase, TInt aSize);
	TInt FreePhysicalRam(TPhysAddr aBase, TInt aSize);
	void Create(const SRamInfo& aInfo, const SRamZone* aZones, TRamZoneCallback aZoneCallback);
	TInt MarkPageAllocated(TPhysAddr aAddr, TZonePageType aType);
	void MarkPagesAllocated(TPhysAddr aAddr, TInt aCount, TZonePageType aType);
	TInt FreeRamPage(TPhysAddr aAddr, TZonePageType aType);
	void FreeRamPages(TPhysAddr* aPageList, TInt aNumPages, TZonePageType aType);
	TInt AllocRamPages(TPhysAddr* aPageList, TInt aNumPages, TZonePageType aType, TUint aBlockedZoneId=KRamZoneInvalidId, TBool aBlockRest=EFalse);
	TInt ZoneAllocRamPages(TUint* aZoneIdList, TUint aZoneIdCount, TPhysAddr* aPageList, TInt aNumPages, TZonePageType aType);
	TInt AllocContiguousRam(TUint aNumPages, TPhysAddr& aPhysAddr, TInt aAlign=0);
#if !defined(__MEMMODEL_MULTIPLE__) && !defined(__MEMMODEL_MOVING__)
	TUint BlockContiguousRegion(TPhysAddr aAddrBase, TUint aNumPages);
	void UnblockSetAllocRuns(TUint& aOffset1, TUint& aOffset2, TUint aRunLength1, TUint aRunLength2, TUint& aAllocLength, TUint& aAllocStart);
	void UnblockContiguousRegion(TPhysAddr aAddrBase, TUint aNumPages);
	TBool ClearContiguousRegion(TPhysAddr aAddrBase, TPhysAddr aZoneBase, TUint aNumPages, TInt& aOffsetm, TUint aUnreservedPages);
	TUint CountPagesInRun(TPhysAddr aAddrBase, TPhysAddr aAddrEndPage, TZonePageType aType);
#endif
	TInt ZoneAllocContiguousRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign);
#ifdef _DEBUG
	void DebugDump();
#endif
#ifdef __VERIFY_LEASTMOVDIS
	void VerifyLeastPrefMovDis();
#endif
	TInt GetZonePageCount(TUint aId, SRamZonePageCount& aPageData);
	void ChangePageType(SPageInfo* aPageInfo, TZonePageType aOldType, TZonePageType aNewType);
#ifdef BTRACE_RAM_ALLOCATOR
	void DoBTracePrime(void);
#endif
	TInt GetZoneAddress(TUint aZoneId, TPhysAddr& aPhysBase, TUint& aNumPages);
	TInt HalFunction(TInt aFunction, TAny* a1, TAny* a2);
	TInt NextAllocatedPage(SZone* aZone, TUint& aOffset, TZonePageType aType) const;
	TInt NextAllocatedRun(SZone* aZone, TUint& aOffset, TUint aEndOffset, TZonePageType aType) const;
	TUint GenDefragFreePages(TZonePageType aType) const;
	SZone* GeneralDefragStart0(TGenDefragStage& aStage, TUint& aRequiredToBeDiscarded);
	SZone* GeneralDefragNextZone0();
	SZone* GeneralDefragStart1();
	SZone* GeneralDefragNextZone1();
	SZone* GeneralDefragStart2();
	SZone* GeneralDefragNextZone2();
	void GeneralDefragEnd();
	SZone* ZoneFromId(TUint aId) const;
	void ZoneClaimStart(SZone& aZone);
	void ZoneClaimEnd(SZone& aZone);
	TInt ModifyZoneFlags(TUint aId, TUint aClearFlags, TUint aSetFlags);
	void ZoneMark(SZone& aZone);
	TBool ZoneUnmark(SZone& aZone);
	void InitialCallback();
private:
	static DRamAllocator* New();
	SZone* GetZoneAndOffset(TPhysAddr aAddr, TInt& aOffset);
	inline void CountZones(const SRamZone* aZones);
	inline void SortRamZones(const SRamZone* aZones, TUint8* aZoneAddrOrder);
	void ZoneAllocPages(SZone* aZone, TUint32 aCount, TZonePageType aType);
	void ZoneFreePages(SZone* aZone, TUint32 aCount, TZonePageType aType);
	void ZoneClearPages(SZone& aZone, TUint aRequiredPages);
	TUint32 ZoneFindPages(TPhysAddr*& aPageList, SZone& aZone, TUint32 aNumPages, TZonePageType aType);
	TInt SetPhysicalRamState(TPhysAddr aBase, TInt aSize, TBool aState, TZonePageType aType);
	inline TUint InitSPageInfos(const SZone* aZone);
	TBool NextAllocZone(SZone*& aZone, TZoneSearchState& aState, TZonePageType aType, TUint aBlockedZoneId, TBool aBlockRest);
	TBool NoAllocOfPageType(SZone& aZone, TZonePageType aType) const;
private:
	TPhysAddr iPhysAddrBase;	// lowest valid physical address
	TPhysAddr iPhysAddrTop;		// highest valid physical address
	TUint iTotalFreeRamPages;
	TUint iTotalRamPages;

	SZone* iZones;				/**< per-zone info stored in ascending address order*/
	TUint32 iNumZones;			/**< The number of zones*/
	SDblQue iZonePrefList;		/**< Doubly linked list of zones in preference order*/
	SDblQueLink* iZoneLeastMovDis; 		/**< Link to the least preferable RAM zone that has discardable or movable pages*/
	TUint iZoneLeastMovDisRank; 		/**< Rank of the least preferable RAM zone that has discardable or movable pages*/
	TUint64 iZonePwrState;				/**< mask of currently used power blocks*/
	TBool iZoneCallbackInitSent;		/**< Set to ETrue once an ERamZoneOp_Init has been issued*/
	TRamZoneCallback iZonePowerFunc;	/**< Callback function to invoke when RAM zone power state changes*/
	TInt iZoneTmpAddrIndex;				/**< Used by NextAllocZone*/
	SDblQueLink* iZoneTmpPrefLink;		/**< Used by NextAllocZone*/
	SDblQueLink* iZoneGeneralPrefLink;	/**< Link to the current RAM zone being defragged*/
	SDblQueLink* iZoneGeneralTmpLink;	/**< Link to the current RAM zone being defragged*/
	TUint iZoneGeneralStage;			/**< The current stage of any general defrag operation*/
	TUint iContiguousReserved;			/**< The count of the number of separate contiguous allocations that have reserved pages*/
	};

#endif
