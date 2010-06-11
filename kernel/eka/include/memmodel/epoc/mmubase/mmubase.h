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
// e32\include\memmodel\epoc\mmubase\mmubase.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __MMUBASE_H__
#define __MMUBASE_H__
#include <plat_priv.h>
#include <memmodel/epoc/mmubase/kblockmap.h>


/******************************************************************************
 * Definitions common to all MMU memory models
 ******************************************************************************/

/**
@internalComponent
*/
struct SPageInfo
	{
	enum TType
		{
		EInvalid=0,			// No physical RAM exists for this page
		EFixed=1,			// RAM fixed at boot time, 
		EUnused=2,			// Page is unused
		EChunk=3,			// iOwner=DChunk*			iOffset=index into chunk
		ECodeSegMemory=4,	// iOwner=DCodeSegMemory*	iOffset=index into CodeSeg memory (Multiple Memory Model only)
//		EHwChunk=5,			// Not used
		EPageTable=6,		// iOwner=0					iOffset=index into KPageTableBase
		EPageDir=7,			// iOwner=ASID				iOffset=index into Page Directory
		EPtInfo=8,			// iOwner=0					iOffset=index into KPageTableInfoBase
		EShadow=9,			// iOwner=phys ROM page		iOffset=index into ROM
		EPagedROM=10,		// iOwner=0,				iOffset=index into ROM
		EPagedCode=11,		// iOwner=DCodeSegMemory*	iOffset=index into code chunk (not offset into CodeSeg!)
		EPagedData=12,		// NOT YET SUPPORTED
		EPagedCache=13,		// iOwner=DChunk*			iOffset=index into chunk 
		EPagedFree=14,		// In demand paging 'live list' but not used for any purpose
		};

	enum TState
		{
		EStateNormal = 0,		// no special state
		EStatePagedYoung = 1,	// demand paged and is on the young list
		EStatePagedOld = 2,		// demand paged and is on the old list
		EStatePagedDead = 3,	// demand paged and is currently being modified
		EStatePagedLocked = 4	// demand paged but is temporarily not being demand paged
		};

	inline TType Type()
		{
		return (TType)iType;
		}
	inline TState State()
		{
		return (TState)iState;
		}
	inline TAny* Owner()
		{
		return iOwner;
		}
	inline TUint32 Offset()
		{
		return iOffset;
		}
	inline TInt LockCount()
		{
		return iLockCount;
		}

	/** Return the index of the zone the page is in
	*/
	inline TUint8 Zone()
		{
		return iZone;
		}

#ifdef _DEBUG
	void Set(TType aType, TAny* aOwner, TUint32 aOffset);
	void Change(TType aType,TState aState);
	void SetState(TState aState);
	void SetModifier(TAny* aModifier);
	TInt CheckModified(TAny* aModifier);
	void SetZone(TUint8 aZoneIndex);
#else
	inline void Set(TType aType, TAny* aOwner, TUint32 aOffset)
		{
		(TUint16&)iType = aType; // also sets iState to EStateNormal
		iOwner = aOwner;
		iOffset = aOffset;
		iModifier = 0;
		}
	inline void Change(TType aType,TState aState)
		{
		iType = aType;
		iState = aState;
		iModifier = 0;
		}
	inline void SetState(TState aState)
		{
		iState = aState;
		iModifier = 0;
		}
	inline void SetModifier(TAny* aModifier)
		{
		iModifier = aModifier;
		}
	inline TInt CheckModified(TAny* aModifier)
		{
		return iModifier!=aModifier;
		}
	
	inline void SetZone(TUint8 aZoneIndex)
		{
		iZone = aZoneIndex;
		}
#endif // !_DEBUG
	void Lock();
	TInt Unlock();

	inline void SetFixed()
		{
		Set(EFixed,0,0);
		iLockCount = 1;
		}
	inline void SetUnused()
		{
		__NK_ASSERT_DEBUG(0 == LockCount());
		(TUint16&)iType = EUnused; // also sets iState to zero
		iModifier = 0;
		// do not modify iOffset in this function because cache cleaning operations
		// rely on using this value
		}
	inline void SetChunk(TAny* aChunk, TUint32 aOffset)
		{
		Set(EChunk,aChunk,aOffset);
		}
	inline void SetCodeSegMemory(TAny* aCodeSegMemory,TUint32 aOffset)
		{
		Set(ECodeSegMemory,aCodeSegMemory,aOffset);
		}
//	inline void SetHwChunk(TAny* aChunk, TUint32 aOffset)
//		{
//		Set(EHwChunk,aChunk,aOffset);
//		}
	inline void SetPageTable(TUint32 aId)
		{
		Set(EPageTable,0,aId);
		}
	inline void SetPageDir(TUint32 aOsAsid, TInt aOffset)
		{
		Set(EPageDir,(TAny*)aOsAsid,aOffset);
		}
	inline void SetPtInfo(TUint32 aOffset)
		{
		Set(EPtInfo,0,aOffset);
		}
	inline void SetShadow(TPhysAddr aOrigPhys, TUint32 aOffset)
		{
		Set(EShadow,(TAny*)aOrigPhys,aOffset);
		}
	inline void SetPagedROM(TUint32 aOffset)
		{
		Set(EPagedROM,0,aOffset);
		}
	inline void SetPagedCode(TAny* aCodeSegMemory, TUint32 aOffset)
		{
		Set(EPagedCode,aCodeSegMemory,aOffset);
		}

	inline static SPageInfo* FromLink(SDblQueLink* aLink)
		{ return (SPageInfo*)((TInt)aLink-_FOFF(SPageInfo,iLink)); }

	inline TUint& PagedLock()
		{ return (TUint&)iLink.iPrev; }

	/**
	Return the SPageInfo for a given page of physical RAM.
	*/
	inline static SPageInfo* FromPhysAddr(TPhysAddr aAddress);

	/**
	Return physical address of the RAM page which this SPageInfo object is associated.
	If the address has no SPageInfo, then a null pointer is returned.
	*/
	static SPageInfo* SafeFromPhysAddr(TPhysAddr aAddress);

	/**
	Return physical address of the RAM page which this SPageInfo object is associated.
	*/
	inline TPhysAddr PhysAddr();

private:
	TUint8 iType;			// enum TType
	TUint8 iState;			// enum TState
	TUint8 iZone;			// The index of the zone the page is in, for use by DRamAllocator
	TUint8 iSpare1;
	TAny* iOwner;			// owning object 
	TUint32 iOffset;		// page offset withing owning object
	TAny* iModifier;		// pointer to object currently manipulating page
	TUint32 iLockCount;		// non-zero if page acquired by code outside of the kernel
	TUint32 iSpare2;
public:
	SDblQueLink iLink;		// used for placing page into linked lists
	};

/******************************************************************************
Per-page table info

Page count	0-256
Usage		unused
			chunk		ptr (26 bits) offset (12 bits)
			HW chunk	ptr (26 bits) offset (12 bits)
			global		offset (12 bits)
			shadow page	offset (12 bits)


*******************************************************************************/
/**
@internalComponent
*/
struct SPageTableInfo
	{
	enum TAttribs
		{
		EUnused=0,
		EChunk=1,
//		EHwChunk=2,
		EGlobal=3,
		EShadow=4,
		};

	enum {EAttShift=6, EAttMask=0x3f};

	inline TInt Attribs()
		{return iAttPtr&EAttMask;}
	inline TInt Count()
		{return iCount;}
	inline TUint32 Offset()
		{return iOffset;}
	inline TUint32 Ptr()
		{return iAttPtr>>EAttShift;}

	inline void SetUnused()
		{iCount=0; iOffset=0; iAttPtr=0;}
	inline void SetChunk(TUint32 aChunk, TUint32 aOffset)
		{iOffset=aOffset; iAttPtr=(aChunk<<EAttShift)|EChunk;}
//	inline void SetHwChunk(TUint32 aChunk, TUint32 aOffset)
//		{iOffset=aOffset; iAttPtr=(aChunk<<EAttShift)|EHwChunk;}
	inline void SetGlobal(TUint32 aOffset)
		{iOffset=aOffset; iAttPtr=EGlobal;}
	inline void SetShadow(TUint32 aOffset)
		{iCount=0; iOffset=aOffset; iAttPtr=EShadow;}

	TUint16 iCount;
	TUint16 iOffset;
	TUint32 iAttPtr;
	};

/******************************************************************************
Bitmap Allocators

PageTableAllocator				free page tables within allocated pages
PageTableLinearAllocator		free linear addresses for page tables
ASIDAllocator					free process slots

Page directory linear address = PageDirBase + (ASID<<PageDirSizeShift)
Page table linear address = PageTableBase + (PTID<<PageTableSizeShift)

Terminology

  Page table cluster = no. of page tables in one page
  Page table block = no. of SPageTableInfo structures in one page
  Page table group = no. of page tables mapped with a single page table

  Local = specific to process
  Shared = subset of processes but not necessarily all
  Global = all processes
*******************************************************************************/

/********************************************
 * Address range allocator
 ********************************************/
/**
@internalComponent
*/
class TLinearSection
	{
public:
	static TLinearSection* New(TLinAddr aBase, TLinAddr aEnd);
public:
	TLinAddr iBase;
	TLinAddr iEnd;
	TBitMapAllocator iAllocator;	// bitmap of used PDE positions
	};

/******************************************************************************
 * Base class for MMU stuff
 ******************************************************************************/

/**
@internalComponent
*/
const TPhysAddr KRomPhysAddrInvalid=0xFFFFFFFFu;

/**
@internalComponent
*/
const TUint16 KPageTableNotPresentId=0xFFFF;

/**
@internalComponent
*/
const TInt KUnmapPagesTLBFlushDeferred=0x80000000;

/**
@internalComponent
*/
const TInt KUnmapPagesCountMask=0xffff;

/**
@internalComponent
*/
const TInt KMaxPages = 32;

/**
@internalComponent
*/
typedef TUint32 TPde;

/**
@internalComponent
*/
typedef TUint32 TPte;

class THwChunkAddressAllocator;
class RamCacheBase;
class Defrag;
class DRamAllocator;
class DMemModelCodeSegMemory;
class DMemModelChunk;

/**
@internalComponent
*/
class MmuBase
	{
public:
	enum TPanic
		{
		EAsyncFreePageStillInUse=0,
		EPtLinAllocCreateFailed=1,
		EPtAllocCreateFailed=2,
		EPageInfoCreateFailed=3,
		EAsyncFreeListCreateFailed=4,
		EPtBlockCountCreateFailed=5,
		EPtGroupCountCreateFailed=6,
		EInvalidPageTableAtBoot=7,
		ERamAllocMutexCreateFailed=8,
		EHwChunkMutexCreateFailed=9,
		ECreateKernelSectionFailed=10,
		ECreateHwChunkAllocFailed=11,
		EFreeHwChunkAddrInvalid=12,
		EFreeHwChunkIndexInvalid=13,
		EBadMappedPageAfterBoot=14,
		ERecoverRamDriveAllocPTIDFailed=15,
		EMapPageTableBadExpand=16,
		ERecoverRamDriveBadPageTable=17,
		ERecoverRamDriveBadPage=18,
		EBadFreePhysicalRam=19,
		EPageLockedTooManyTimes=20,
		EPageUnlockedTooManyTimes=21,
		EPageInfoSetWhenNotUnused=22,
		ERamCacheAllocFailed=23,
		EDefragAllocFailed=24,
		EDefragUnknownPageType=25,
		EDefragUnknownPageTableType=27,
		EDefragUnknownChunkType=28,
		EDefragStackAllocFailed=29,
		EDefragKernelChunkNoPageTable=30,
		EDefragProcessWrongPageDir=31,
		};
public:
	MmuBase();

	// non virtual
	TInt AllocPageTable();
	TInt DoAllocPageTable(TPhysAddr& aPhysAddr);
	TInt InitPageTableInfo(TInt aId);
	TInt MapPageTable(TInt aId, TPhysAddr aPhysAddr, TBool aAllowExpand=ETrue);
	void FreePageTable(TInt aId);
	TBool DoFreePageTable(TInt aId);
	TInt AllocPhysicalRam(TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign=0);
	TInt ZoneAllocPhysicalRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign=0);
	TInt AllocPhysicalRam(TInt aNumPages, TPhysAddr* aPageList);
	TInt ZoneAllocPhysicalRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aNumPages, TPhysAddr* aPageList);
	TInt FreePhysicalRam(TPhysAddr aPhysAddr, TInt aSize);
	TInt FreePhysicalRam(TInt aNumPages, TPhysAddr* aPageList);
	TInt FreeRamZone(TUint aZoneId, TPhysAddr& aZoneBase, TUint& aZoneBytes);
	TInt ClaimPhysicalRam(TPhysAddr aPhysAddr, TInt aSize);
	TInt GetPageTableId(TPhysAddr aPtPhys);
	void MapRamPage(TLinAddr aAddr, TPhysAddr aPage, TPte aPtePerm);
	void UnmapAndFree(TLinAddr aAddr, TInt aNumPages);
	void FreePages(TPhysAddr* aPageList, TInt aCount, TZonePageType aPageType);
	void CreateKernelSection(TLinAddr aEnd, TInt aHwChunkAlign);
	TInt AllocateAllPageTables(TLinAddr aLinAddr, TInt aSize, TPde aPdePerm, TInt aMapShift, SPageTableInfo::TAttribs aAttrib);
	TInt AllocShadowPage(TLinAddr aRomAddr);
	TInt FreeShadowPage(TLinAddr aRomAddr);
	TInt FreezeShadowPage(TLinAddr aRomAddr);
	TInt FreeRamInBytes();
	TInt GetRamZonePageCount(TUint aId, SRamZonePageCount& aPageData);
	TInt ModifyRamZoneFlags(TUint aId, TUint aClearMask, TUint aSetMask);

	// RAM allocator and defrag interfaces.
	void RamAllocLock();
	void RamAllocUnlock();
	TUint NumberOfFreeDpPages();
	TInt MovePage(TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest);
	TInt DiscardPage(TPhysAddr aAddr, TUint aBlockZoneId, TBool aBlockRest);

	// virtual
	virtual void Init1();
	virtual void Init2();
	virtual void Init3();
	virtual THwChunkAddressAllocator* MappingRegion(TUint aMapAttr);
	virtual TInt RecoverRamDrive();
	virtual TInt CopyToShadowMemory(TLinAddr aDest, TLinAddr aSrc, TUint32 aLength);
	
	// cpu dependent page moving method - cutils.cia
	TInt MoveKernelStackPage(DChunk* aChunk, TUint32 aOffset, TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest);

	// pure virtual
	virtual void DoInit2()=0;
	virtual TBool PteIsPresent(TPte aPte)=0;
	virtual TPhysAddr PtePhysAddr(TPte aPte, TInt aPteIndex)=0;
	virtual TPhysAddr PdePhysAddr(TLinAddr aAddr)=0;
	virtual void SetupInitialPageInfo(SPageInfo* aPageInfo, TLinAddr aChunkAddr, TInt aPdeIndex)=0;
	virtual void SetupInitialPageTableInfo(TInt aId, TLinAddr aChunkAddr, TInt aNumPtes)=0;
	virtual void AssignPageTable(TInt aId, TInt aUsage, TAny* aObject, TLinAddr aAddr, TPde aPdePerm)=0;
	virtual TInt UnassignPageTable(TLinAddr aAddr)=0;
	virtual void BootstrapPageTable(TInt aXptId, TPhysAddr aXptPhys, TInt aId, TPhysAddr aPhysAddr)=0;
	virtual void FixupXPageTable(TInt aId, TLinAddr aTempMap, TPhysAddr aOld, TPhysAddr aNew)=0;
	virtual TInt PageTableId(TLinAddr aAddr)=0;
	virtual TInt BootPageTableId(TLinAddr aAddr, TPhysAddr& aPtPhys)=0;
	virtual void ClearPageTable(TInt aId, TInt aFirstIndex=0)=0;
	virtual TPhysAddr LinearToPhysical(TLinAddr aAddr)=0;
	virtual TInt LinearToPhysical(TLinAddr aAddr, TInt aSize, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList=NULL)=0;
	virtual void MapRamPages(TInt aId, SPageInfo::TType aType, TAny* aPtr, TUint32 aOffset, const TPhysAddr* aPageList, TInt aNumPages, TPte aPtePerm)=0;
	virtual void MapPhysicalPages(TInt aId, SPageInfo::TType aType, TAny* aPtr, TUint32 aOffset, TPhysAddr aPhysAddr, TInt aNumPages, TPte aPtePerm)=0;
	virtual void RemapPage(TInt aId, TUint32 aAddr, TPhysAddr aOldAddr, TPhysAddr aNewAddr, TPte aPtePerm, DProcess* aProcess)=0;
	virtual TInt UnmapPages(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TBool aSetPagesFree, TInt& aNumPtes, TInt& aNumFree, DProcess* aProcess)=0;
	virtual void ClearRamDrive(TLinAddr aStart)=0;
	virtual TInt PdePtePermissions(TUint& aMapAttr, TPde& aPde, TPte& aPte)=0;
	virtual void Map(TLinAddr aLinAddr, TPhysAddr aPhysAddr, TInt aSize, TPde aPdePerm, TPte aPtePerm, TInt aMapShift)=0;
	virtual void Unmap(TLinAddr aLinAddr, TInt aSize)=0;
	virtual void InitShadowPageTable(TInt aId, TLinAddr aRomAddr, TPhysAddr aOrigPhys)=0;
	virtual void InitShadowPage(TPhysAddr aShadowPhys, TLinAddr aRomAddr)=0;
	virtual void DoUnmapShadowPage(TInt aId, TLinAddr aRomAddr, TPhysAddr aOrigPhys)=0;
	virtual TInt UnassignShadowPageTable(TLinAddr aRomAddr, TPhysAddr aOrigPhys)=0;
	virtual void DoFreezeShadowPage(TInt aId, TLinAddr aRomAddr)=0;
	virtual void FlushShadow(TLinAddr aRomAddr)=0;
	virtual void AssignShadowPageTable(TInt aId, TLinAddr aRomAddr)=0;
	virtual void ClearPages(TInt aNumPages, TPhysAddr* aPageList, TUint8 aClearByte = KChunkClearByteDefault)=0;
	virtual void Pagify(TInt aId, TLinAddr aLinAddr)=0;
	virtual void CacheMaintenanceOnDecommit(const TPhysAddr* aPhysAdr, TInt aPageCount)=0;
	virtual void CacheMaintenanceOnDecommit(const TPhysAddr aPhysAdr)=0;
	virtual void CacheMaintenanceOnPreserve(const TPhysAddr* aPhysAdr, TInt aPageCount, TUint aMapAttr)=0;
	virtual void CacheMaintenanceOnPreserve(const TPhysAddr aPhysAdr, TUint aMapAttr)=0;
	virtual void CacheMaintenanceOnPreserve(TPhysAddr aPhysAddr, TInt aSize, TLinAddr aLinAddr, TUint iMapAttr)=0;

	// memory model dependent page moving methods - mdefrag.cpp
	virtual TInt MoveCodeSegMemoryPage(DMemModelCodeSegMemory* aCodeSegMemory, TUint32 aOffset, TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest)=0;
	virtual TInt MoveCodeChunkPage(DChunk* aChunk, TUint32 aOffset, TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest)=0;
	virtual TInt MoveDataChunkPage(DChunk* aChunk, TUint32 aOffset, TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest)=0;

	// cpu and memory model dependent page moving methods - xmmu.cpp
	virtual TInt RamDefragFault(TAny* aExceptionInfo)=0;
	virtual void DisablePageModification(DMemModelChunk* aChunk, TInt aOffset)=0;
	virtual TPte PtePermissions(TChunkType aChunkType)=0;

public:
	static TUint32 RoundToPageSize(TUint32 aSize);
	static TUint32 RoundToChunkSize(TUint32 aSize);
	static TInt RoundUpRangeToPageSize(TUint32& aBase, TUint32& aSize);
	static void Wait();
	static void Signal();
	static void WaitHwChunk();
	static void SignalHwChunk();
	static void Panic(TPanic aPanic);
public:
	inline TLinAddr PageTableLinAddr(TInt aId)
		{return iPageTableLinBase+(aId<<iPageTableShift);}
	inline SPageTableInfo& PtInfo(TInt aId)
		{return iPtInfo[aId];}

	inline TLinAddr PtInfoBlockLinAddr(TInt aBlock)
		{return (TLinAddr)iPtInfo+(aBlock<<iPageShift);}

	/** Get the page table info block number from a page table ID
	@param aId The ID of the page table.
	@return The page table info block
	*/
	inline TInt PtInfoBlock(TInt aId)
		{return aId >> iPtBlockShift;}

	/**
	@return The page table entry for the page table info pages.
	*/
	inline TPte PtInfoPtePerm()
		{return iPtInfoPtePerm;}
	
public:
	TInt AllocRamPages(TPhysAddr* aPageList, TInt aNumPages, TZonePageType aPageType, TUint aBlockedZoneId=KRamZoneInvalidId, TBool aBlockRest=EFalse);
	TInt ZoneAllocRamPages(TUint* aZoneIdList, TUint aZoneIdCount, TPhysAddr* aPageList, TInt aNumPages, TZonePageType aPageType);
	TInt AllocContiguousRam(TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign);
	TInt ZoneAllocContiguousRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign);

public:
	TInt iPageSize;				// page size in bytes
	TInt iPageMask;				// page size - 1
	TInt iPageShift;			// log2(page size)
	TInt iChunkSize;			// PDE size in bytes
	TInt iChunkMask;			// PDE size - 1
	TInt iChunkShift;			// log2(PDE size)
	TInt iPageTableSize;		// 2nd level page table size in bytes
	TInt iPageTableMask;		// 2nd level page table size - 1
	TInt iPageTableShift;		// log2(2nd level page table size)
	TInt iPtClusterSize;		// number of page tables per page
	TInt iPtClusterMask;		// number of page tables per page - 1
	TInt iPtClusterShift;		// log2(number of page tables per page)
	TInt iPtBlockSize;			// number of SPageTableInfo per page
	TInt iPtBlockMask;			// number of SPageTableInfo per page - 1
	TInt iPtBlockShift;			// log2(number of SPageTableInfo per page)
	TInt iPtGroupSize;			// number of page tables mapped by a page table
	TInt iPtGroupMask;			// number of page tables mapped by a page table - 1
	TInt iPtGroupShift;			// log2(number of page tables mapped by a page table)
	TInt iMaxPageTables;		// maximum number of page tables (<65536)
	TInt* iPtBlockCount;		// number of page table pages in each block
	TInt* iPtGroupCount;		// number of page table pages in each group
	TInt iNumPages;				// Number of pages being managed
	SPageTableInfo* iPtInfo;	// per-page table information array
	TLinAddr iPageTableLinBase;	// base address of page tables
	DRamAllocator* iRamPageAllocator;
	TBitMapAllocator* iPageTableAllocator;	// NULL if page table size = page size
	TBitMapAllocator* iPageTableLinearAllocator;
	TInt iInitialFreeMemory;
	TBool iAllocFailed;
	TPte iPtInfoPtePerm;
	TPte iPtPtePerm;
	TPde iPtPdePerm;
	TPte* iTempPte;				// PTE used for temporary mappings
	TLinAddr iTempAddr;			// address corresponding to iTempPte
	TLinearSection* iKernelSection;		// bitmap used to allocate kernel section addresses
	THwChunkAddressAllocator* iHwChunkAllocator;	// address allocator for HW chunks in kernel section
	TUint32 iMapSizes;			// bit mask of supported mapping sizes
	TUint iDecommitThreshold;	// threshold for selective/global cache flush on decommit for VIPT caches
	TLinAddr iRomLinearBase;
	TLinAddr iRomLinearEnd;
	TPte iShadowPtePerm;
	TPde iShadowPdePerm;

	// Page moving and defrag fault handling members.
	TLinAddr iAltStackBase;
	TLinAddr iDisabledAddr;
	TInt iDisabledAddrAsid;
	TPte* iDisabledPte;
	TPte iDisabledOldVal;

	RamCacheBase* iRamCache;
	Defrag* iDefrag;

	static DMutex* HwChunkMutex;	// mutex protecting HW chunk address allocators
	static DMutex* RamAllocatorMutex;	// the main mutex protecting alloc/dealloc and most map/unmap
	static MmuBase* TheMmu;		// pointer to the single instance of this class
	static const SRamZone* RamZoneConfig; /**<Pointer to variant specified array containing details on RAM banks and their allocation preferences*/
	static TRamZoneCallback RamZoneCallback; /**<Pointer to callback function to be invoked when RAM power state changes*/

public:
	friend class Monitor;
	};


/******************************************************************************
 * Address allocator for HW chunks
 ******************************************************************************/

/**
@internalComponent
*/
class THwChunkRegion
	{
public:
	inline THwChunkRegion(TInt aIndex, TInt aSize, TPde aPdePerm)
				: iIndex((TUint16)aIndex), iRegionSize((TUint16)aSize), iPdePerm(aPdePerm)
				{}
public:
	TUint16 iIndex;				// index of base of this region in linear section
	TUint16 iRegionSize;		// number of PDEs covered; 0 means page table
	union
		{
		TPde iPdePerm;			// PDE permissions for this region
		THwChunkRegion* iNext;	// used during deallocation
		};
	};

/**
@internalComponent
*/
class THwChunkPageTable : public THwChunkRegion
	{
public:
	THwChunkPageTable(TInt aIndex, TInt aSize, TPde aPdePerm);
	static THwChunkPageTable* New(TInt aIndex, TPde aPdePerm);
public:
	TBitMapAllocator iAllocator;	// bitmap of used page positions
	};

/**
@internalComponent
*/
class THwChunkAddressAllocator : public RPointerArray<THwChunkRegion>
	{
public:
	static THwChunkAddressAllocator* New(TInt aAlign, TLinearSection* aSection);
	TLinAddr Alloc(TInt aSize, TInt aAlign, TInt aOffset, TPde aPdePerm);
	THwChunkRegion* Free(TLinAddr aAddr, TInt aSize);
public:
	THwChunkAddressAllocator();
	TLinAddr SearchExisting(TInt aNumPages, TInt aPageAlign, TInt aPageOffset, TPde aPdePerm);
	void Discard(THwChunkRegion* aRegion);
	static TInt Order(const THwChunkRegion& a1, const THwChunkRegion& a2);
	THwChunkRegion* NewRegion(TInt aIndex, TInt aSize, TPde aPdePerm);
	THwChunkPageTable* NewPageTable(TInt aIndex, TPde aPdePerm, TInt aInitB, TInt aInitC);
public:
	TInt iAlign;				// alignment required for allocated addresses
	TLinearSection* iSection;	// linear section in which allocation occurs
	};


/** Hardware chunk

@internalComponent
*/
class DMemModelChunkHw : public DPlatChunkHw
	{
public:
	virtual TInt Close(TAny* aPtr);
public:
	TInt AllocateLinearAddress(TPde aPdePerm);
	void DeallocateLinearAddress();
public:
	THwChunkAddressAllocator* iAllocator;
	};


/******************************************************************************
 * MMU-specifc code segment data
 ******************************************************************************/

/**
@internalComponent
*/
class DMmuCodeSegMemory : public DEpocCodeSegMemory
	{
public:
	DMmuCodeSegMemory(DEpocCodeSeg* aCodeSeg);
	~DMmuCodeSegMemory();
	virtual TInt Create(TCodeSegCreateInfo& aInfo);
	virtual TInt Loaded(TCodeSegCreateInfo& aInfo);

	/**
	Apply code relocations and import fixups to one page of code.
	@param aBuffer The buffer containg the code
	@param aCodeAddress The address the page will be mapped at
	*/
	void ApplyCodeFixups(TUint32* aBuffer, TLinAddr aCodeAddress);

	/**
	Apply code relocations and import fixups to one page of code.
	Called by DMemModelCodeSegMemory::Loaded to fixup pages which are already paged-in.

	@param aBuffer The buffer containg the code
	@param aCodeAddress The address the page will be mapped at
	*/
	TInt ApplyCodeFixupsOnLoad(TUint32* aBuffer, TLinAddr aCodeAddress);
private:
	TInt ReadBlockMap(const TCodeSegCreateInfo& aInfo);
	TInt ReadFixupTables(const TCodeSegCreateInfo& aInfo);
public:
	TBool iIsDemandPaged;

	TInt iPageCount;					// Number of pages used for code
	TInt iDataPageCount;				// Number of extra pages used to store data section
	TUint8* iCodeRelocTable;			// Code relocation information
	TInt iCodeRelocTableSize;			// Size of code relocation table in bytes
	TUint8* iImportFixupTable;			// Import fixup information
	TInt iImportFixupTableSize;			// Size of import fixup table in bytes
	TUint32 iCodeDelta;					// Code relocation delta
	TUint32 iDataDelta;					// Data relocation delta

	TInt iCompressionType;				// Compression scheme in use
	TInt32* iCodePageOffsets;			// Array of compressed page offsets within the file
	TInt iCodeLocalDrive;				// Local drive number
	TBlockMap iBlockMap;				// Kernel-side representation of block map
	TInt iCodeStartInFile;				// Offset of (possibly compressed) code from start of file

	TAny* iDataSectionMemory;			// pointer to saved copy of data section (when demand paging)

	TInt iCodeAllocBase;
	};

#endif
