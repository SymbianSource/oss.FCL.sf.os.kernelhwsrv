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
// e32\include\memmodel\epoc\multiple\arm\arm_mem.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalComponent
*/

#ifndef __ARM_MEM_H__
#define __ARM_MEM_H__
#include <mmboot.h>

#if defined	(__CPU_USE_SHARED_MEMORY)
#define SHARABLE_PDE	KArmV6PdeSectionS
#define SHARABLE_PTE	KArmV6PteS
#else
#define SHARABLE_PDE	0
#define SHARABLE_PTE	0
#endif

/** Section entry in the 1st level (aka page directory) descriptor*/
#define SECTION_PDE(perm, attr, domain, execute, global)		\
						(	(((perm)&3)<<10)|(((perm)&4)<<13)	|\
							(((attr)&3)<<2)|(((attr)&28)<<10)	|\
							((domain)<<5)						|\
							((execute)?0:KArmV6PdeSectionXN)	|\
							((global)?0:KArmV6PdeSectionNG)		|\
							(SHARABLE_PDE)						|\
							KArmV6PdeSection )

/** Page table entry in the 1st level (aka page directory) descriptor*/
#define PT_PDE(domain)		(KArmV6PdePageTable|((domain)<<5))

/** Large page (64K) entry in the 2nd level (aka page table) descriptor*/
#define LP_PTE(perm, attr, execute, global)						\
						(	(((perm)&3)<<4)|(((perm)&4)<<7)		|\
							(((attr)&3)<<2)|(((attr)&28)<<10)	|\
							((execute)?0:KArmV6PteLargeXN)		|\
							((global)?0:KArmV6PteNG)			|\
							(SHARABLE_PTE)						|\
							KArmV6PteLargePage )

/** Small page (4K) entry in the 2nd level (aka page table) descriptor*/
#define SP_PTE(perm, attr, execute, global)						\
						(	(((perm)&3)<<4)|(((perm)&4)<<7)		|\
							(((attr)&3)<<2)|(((attr)&28)<<4)	|\
							((execute)?0:KArmV6PteSmallXN)		|\
							((global)?0:KArmV6PteNG)			|\
							(SHARABLE_PTE)						|\
							KArmV6PteSmallPage )

#define	SECTION_PDE_FROM_PDEPTE(pde, pte)						\
						(	(((pte)<< 6)&0x0003fc00)			|\
							(((pde)    )&0x000003e0)			|\
							(((pte)<< 4)&0x00000010)			|\
							(((pte)    )&0x0000000c)			|\
							KArmV6PdeSection )

#define LP_PTE_FROM_SP_PTE(pte)									\
						(	(((pte)<<15)&0x00008000)			|\
							(((pte)<< 6)&0x00007000)			|\
							(((pte)    )&0x00000e3c)			|\
							KArmV6PteLargePage )

#define SP_PTE_FROM_LP_PTE(pte)									\
						(	(((pte)>>15)&0x00000001)			|\
							(((pte)>> 6)&0x000001c0)			|\
							(((pte)    )&0x00000e3c)			|\
							KArmV6PteSmallPage )

#define SP_PTE_PERM_SET(pte, perm)								\
						(	((pte)&~KArmV6PtePermMask)			|\
							(((perm)&3)<<4)|(((perm)&4)<<7) )

#define SP_PTE_PERM_GET(pte)									\
						(	(((pte)>> 4)&0x00000003)			|\
							(((pte)>> 7)&0x00000004) )

TPde* const InitPageDirectory = ((TPde*)KPageDirectoryBase);	// initial page directory

inline TPde* PageDirectory(TInt aOsAsid)
	{
	return (TPde*)(KPageDirectoryBase+(aOsAsid<<KPageDirectoryShift));
	}

class DArmPlatThread : public DMemModelThread
	{
public:
	~DArmPlatThread();
	virtual TInt SetupContext(SThreadCreateInfo& anInfo);
	virtual TInt Context(TDes8& aDes);
	virtual void DoExit2();
public:
	friend class Monitor;
	};

class DArmPlatProcess : public DMemModelProcess
	{
public:
	DArmPlatProcess();
	~DArmPlatProcess();
public:
	virtual TInt GetNewChunk(DMemModelChunk*& aChunk, SChunkCreateInfo& aInfo);
	virtual TInt GetNewThread(DThread*& aThread, SThreadCreateInfo& aInfo);
private:
	friend class Monitor;
	};

class DArmPlatChunk : public DMemModelChunk
	{
public:
	DArmPlatChunk();
	~DArmPlatChunk();
	TInt Create(SChunkCreateInfo& aInfo);
	virtual TInt SetupPermissions();
	TInt SetAttributes(SChunkCreateInfo& aInfo);
public:
	friend class Monitor;
	};

class ArmMmu : public Mmu
	{
public:
	// overriding MmuBase
	virtual void Init1();
//	virtual void Init2();
	virtual void DoInit2();
	virtual TBool PteIsPresent(TPte aPte);
	virtual TPhysAddr PtePhysAddr(TPte aPte, TInt aPteIndex);
	virtual TPhysAddr PdePhysAddr(TLinAddr aAddr);
//	virtual void SetupInitialPageInfo(SPageInfo* aPageInfo, TLinAddr aChunkAddr, TInt aPdeIndex);
//	virtual void SetupInitialPageTableInfo(TInt aId, TLinAddr aChunkAddr, TInt aNumPtes);
//	virtual void AssignPageTable(TInt aId, TInt aUsage, TAny* aObject, TLinAddr aAddr, TPde aPdePerm);
//	virtual TInt UnassignPageTable(TLinAddr aAddr);
	virtual void BootstrapPageTable(TInt aXptId, TPhysAddr aXptPhys, TInt aId, TPhysAddr aPhysAddr);
	virtual void FixupXPageTable(TInt aId, TLinAddr aTempMap, TPhysAddr aOld, TPhysAddr aNew);
//	virtual TInt PageTableId(TLinAddr aAddr);
	virtual TInt BootPageTableId(TLinAddr aAddr, TPhysAddr& aPtPhys);
	virtual void ClearPageTable(TInt aId, TInt aFirstIndex=0);
//	virtual TPhysAddr LinearToPhysical(TLinAddr aAddr);
	virtual void MapRamPages(TInt aId, SPageInfo::TType aType, TAny* aPtr, TUint32 aOffset, const TPhysAddr* aPageList, TInt aNumPages, TPte aPtePerm);
	virtual void MapPhysicalPages(TInt aId, SPageInfo::TType aType, TAny* aPtr, TUint32 aOffset, TPhysAddr aPhysAddr, TInt aNumPages, TPte aPtePerm);
	virtual void RemapPage(TInt aId, TUint32 aAddr, TPhysAddr aOldAddr, TPhysAddr aNewAddr, TPte aPtePerm, DProcess* aProcess);
	virtual TInt UnmapPages(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TBool aSetPagesFree, TInt& aNumPtes, TInt& aNumFree, DProcess* aProcess);
	virtual TInt UnmapUnownedPages(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TLinAddr* aLAPageList, TInt& aNumPtes, TInt& aNumFree, DProcess* aProcess);
	virtual void ClearRamDrive(TLinAddr aStart);
	virtual TInt PdePtePermissions(TUint& aMapAttr, TPde& aPde, TPte& aPte);
	virtual void Map(TLinAddr aLinAddr, TPhysAddr aPhysAddr, TInt aSize, TPde aPdePerm, TPte aPtePerm, TInt aMapShift);
	virtual void Unmap(TLinAddr aLinAddr, TInt aSize);
	virtual void InitShadowPageTable(TInt aId, TLinAddr aRomAddr, TPhysAddr aOrigPhys);
	virtual void InitShadowPage(TPhysAddr aShadowPhys, TLinAddr aRomAddr);
	virtual void DoUnmapShadowPage(TInt aId, TLinAddr aRomAddr, TPhysAddr aOrigPhys);
	virtual TInt UnassignShadowPageTable(TLinAddr aRomAddr, TPhysAddr aOrigPhys);
	virtual void DoFreezeShadowPage(TInt aId, TLinAddr aRomAddr);
	virtual void FlushShadow(TLinAddr aRomAddr);
	virtual void AssignShadowPageTable(TInt aId, TLinAddr aRomAddr);
	virtual void ClearPages(TInt aNumPages, TPhysAddr* aPageList, TUint8 aClearByte);
	virtual void Pagify(TInt aId, TLinAddr aLinAddr);
#if defined(__CPU_MEMORY_TYPE_REMAPPING)	// arm1176, arm11mcore, armv7, ...
	virtual TInt CopyToShadowMemory(TLinAddr aDest, TLinAddr aSrc, TUint32 aLength);
#endif
	virtual TPte PtePermissions(TChunkType aChunkType);
	virtual TInt RamDefragFault(TAny* aExceptionInfo);
	virtual void DisablePageModification(DMemModelChunk* aChunk, TInt aOffset);

	// overriding Mmu
	virtual TInt NewPageDirectory(TInt aOsAsid, TBool aSeparateGlobal, TPhysAddr& aPhysAddr, TInt& aNumPages);
	virtual void InitPageDirectory(TInt aOsAsid, TBool aGlobal);
	virtual TInt PageTableId(TLinAddr aAddr, TInt aOsAsid);
	virtual TPhysAddr LinearToPhysical(TLinAddr aAddr, TInt aOsAsid);
//	virtual TPhysAddr LinearToPhysical(TLinAddr aAddr, TInt aOsAsid, TInt& aPerm, TInt& aAttr);
	virtual TInt LinearToPhysical(TLinAddr aAddr, TInt aSize, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList, TInt aOsAsid);
	virtual TInt PreparePagesForDMA(TLinAddr aAddr, TInt aSize, TInt aOsAsid, TPhysAddr* aPhysicalPageList);
	virtual TInt ReleasePagesFromDMA(TPhysAddr* aPhysicalPageList, TInt aPageCount);
	virtual void DoAssignPageTable(TInt aId, TLinAddr aAddr, TPde aPdePerm, const TAny* aOsAsids);
	virtual void RemapPageTableSingle(TPhysAddr aOld, TPhysAddr aNew, TLinAddr aAddr, TInt aOsAsid);
	virtual void RemapPageTableMultiple(TPhysAddr aOld, TPhysAddr aNew, TLinAddr aAddr, const TAny* aOsAsids);
	virtual void RemapPageTableGlobal(TPhysAddr aOld, TPhysAddr aNew, TLinAddr aAddr);
	virtual void RemapPageTableAliases(TPhysAddr aOld, TPhysAddr aNew);
	virtual void DoUnassignPageTable(TLinAddr aAddr, const TAny* aOsAsids);
	virtual TPde PdePermissions(TChunkType aChunkType, TBool aRO);
	virtual void ApplyTopLevelPermissions(TLinAddr aAddr, TInt aOsAsid, TInt aNumPdes, TPde aPdePerm);
	virtual void ApplyPagePermissions(TInt aId, TInt aPageOffset, TInt aNumPages, TPte aPtePerm);
	virtual void GenericFlush(TUint32 aMask);
	virtual TLinAddr MapTemp(TPhysAddr aPage,TLinAddr aLinAddr, TInt aPages=1);
	virtual TLinAddr MapTemp(TPhysAddr aPage,TLinAddr aLinAddr,TInt aPages, TMemoryType aMemType);
	virtual TLinAddr MapSecondTemp(TPhysAddr aPage,TLinAddr aLinAddr, TInt aPages=1);
	virtual void UnmapTemp();
	virtual void UnmapSecondTemp();
	virtual TBool ValidateLocalIpcAddress(TLinAddr aAddr,TInt aSize,TBool aWrite);
	virtual TInt UnlockRamCachePages(TLinAddr aLinAddr, TInt aNumPages, DProcess* aProcess);
	virtual TInt LockRamCachePages(TLinAddr aLinAddr, TInt aNumPages, DProcess* aProcess);
	virtual void MapVirtual(TInt aId, TInt aNumPages);
	virtual TInt UnmapVirtual(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TBool aSetPagesFree, TInt& aNumPtes, TInt& aNumFree, DProcess* aProcess);
	virtual TInt UnmapUnownedVirtual(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TLinAddr* aLAPageList, TInt& aNumPtes, TInt& aNumFree, DProcess* aProcess);
	virtual void RemapPageByAsid(TBitMapAllocator* aOsAsids, TLinAddr aLinAddr, TPhysAddr aOldAddr, TPhysAddr aNewAddr, TPte aPtePerm);
	virtual void CacheMaintenanceOnDecommit(const TPhysAddr* aPhysAddr, TInt aPageCount);
	virtual void CacheMaintenanceOnDecommit(const TPhysAddr aPhysAddr);
	virtual void CacheMaintenanceOnPreserve(const TPhysAddr* aPhysAddr, TInt aPageCount, TUint iMapAttr);
	virtual void CacheMaintenanceOnPreserve(const TPhysAddr aPhysAddr, TUint iMapAttr); 
	virtual void CacheMaintenanceOnPreserve(TPhysAddr aPhysAddr, TInt aSize, TLinAddr aLinAddr, TUint iMapAttr);
	static void UnlockAlias();
	static void LockAlias();
private:
	TInt iTempMapColor;
	TInt iSecondTempMapColor;
public:
	friend class TScheduler;
	friend class Monitor;
	};

GLREF_D ArmMmu TheMmu;

#if !defined(__CPU_ARM1136__) || !defined(__CPU_ARM1136_ERRATUM_399234_FIXED)
/**
Write-Through cache mode is not used by Kernel internally. Also, any request
(by device drivers) for Write-Through memory will be downgraded to buffered&non-cached memory.
*/
#define __CPU_WriteThroughDisabled
#endif

#endif	// __ARM_MEM_H__
