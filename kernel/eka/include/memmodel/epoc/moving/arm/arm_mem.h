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
// e32\include\memmodel\epoc\moving\arm\arm_mem.h
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

#undef __MMU_MACHINE_CODED__

//
// Permissions - 2 bit field
// They are the same for all platforms
//

const TInt KArmV45PermRORO=0;		// sup RO user RO
const TInt KArmV45PermRWNO=1;		// sup RW user no access
const TInt KArmV45PermRWRO=2;		// sup RW user RO
const TInt KArmV45PermRWRW=3;		// sup RW user RW

//
// Cache attributes - 2(ARM, SA1), 3(XScale) or 5(XScale-Manzano) bits
//

#ifndef __CPU_XSCALE__
const TInt KArmV45MemAttNC=0;		// not cached or buffered
const TInt KArmV45MemAttBuf=1;		// buffered, coalescing
const TInt KArmV45MemAttWT=2;		// write through cached (read allocate)
const TInt KArmV45MemAttWB=3;		// write back cached (read allocate)
#endif
#ifdef __CPU_SA1__
const TInt KSA1MemAttMiniCache=2;	// write back cached, mini cache
#endif
#ifdef __CPU_XSCALE__
	#ifdef __CPU_XSCALE_MANZANO__
const TInt KXScaleMemAttNCNB=0;		// not cached or buffered
const TInt KXScaleMemAttBufC=4;		// buffered, coalescing
const TInt KXScaleMemAttWTRA=0x12;	// write through cached, read allocate
const TInt KXScaleMemAttWBRA=0x13;	// write back cached, read allocate
const TInt KXScaleMemAttBufNC=5;	// buffered, noncoalescing
const TInt KXScaleMemAttOFF_WBWA=0x14;		//L1 Off   							L2:write back write allocate
const TInt KXScaleMemAttWTRA_WBWA=0x16;		//L1 write through read allocate   	L2:write back write allocate
const TInt KXScaleMemAttWBRA_WBWA=0x17;		//L1 write back read allocate 		L2:write back write allocate
const TInt KXScaleDefaultCaching=KXScaleMemAttWBRA_WBWA;
const TInt KMemAttTempDemandPaging = KXScaleMemAttOFF_WBWA;	//Cache attributes for tempotrary pages in demand paging
	#else
const TInt KXScaleMemAttNCNB=0;		// not cached or buffered
const TInt KXScaleMemAttBufC=1;		// buffered, coalescing
const TInt KXScaleMemAttWTRA=2;		// write through cached, read allocate
const TInt KXScaleMemAttWBRA=3;		// write back cached, read allocate
const TInt KXScaleMemAttBufNC=5;	// buffered, noncoalescing
const TInt KXScaleMemAttMiniCache=6;// use minicache
const TInt KXScaleMemAttWBWA=7;		// write back cached, write allocate
const TInt KXScaleDefaultCaching=KXScaleMemAttWBRA;
const TInt KMemAttTempDemandPaging = KXScaleMemAttNCNB;	//Cache attributes for tempotrary pages in demand paging
	#endif
#endif

#ifdef __CPU_XSCALE__
const TInt KMemAttNC = KXScaleMemAttNCNB;			//Tex::CB bits for neither cached nor buffered memory
const TInt KMemAttBuf = KXScaleMemAttBufC;			//Tex::CB bits for not cached but buffered/coalescing memory
const TInt KDefaultCaching = KXScaleDefaultCaching;	//The preferable caching
#else
const TInt KMemAttNC = KArmV45MemAttNC;				//Tex::CB bits for neither cached nor buffered memory
const TInt KMemAttBuf = KArmV45MemAttBuf;			//Tex::CB bits for not cached but buffered/coalescing memory
const TInt KDefaultCaching = KArmV45MemAttWB;		//The preferable caching
const TInt KMemAttTempDemandPaging = KMemAttNC;		//Cache attributes for tempotrary pages in demand paging
#endif

//
// Definition of the other bits in the 1st(PDE) and 2nd(PTE) level tables
//

const TUint32 KArmV45PdePageTable	=0x00000001;	// L1 descriptor is page table
const TUint32 KArmV45PdeSection		=0x00000002;	// L1 descriptor is section
#ifdef __CPU_XSCALE__
const TUint32 KXScalePdeECCEnable	=0x00000200;	// ECC enable (all L1 descriptors)
#endif
const TUint32 KArmV45PdePermMask	=0x00000c00;	// Section permission bits
const TUint32 KArmV45PdeAttMask		=0x0000000c;	// Section memory attribute bits
#ifdef __CPU_XSCALE__
const TUint32 KXScalePdeAttMask		=0x0000800c;	// Section memory attribute bits
#endif

const TUint32 KArmV45PteLargePage	=0x00000001;	// L2 descriptor is large page
const TUint32 KArmV45PteSmallPage	=0x00000002;	// L2 descriptor is small page
#ifdef __CPU_XSCALE__
const TUint32 KXScalePteSmallPageX	=0x00000003;	// L2 descriptor is XScale extended small page
const TUint32 KArmPteSmallPage		=KXScalePteSmallPageX;
#else
const TUint32 KArmPteSmallPage		=KArmV45PteSmallPage;
#endif

#if defined(__CPU_ARM710T__) || defined(__CPU_ARM720T__)
const TPde KPdeExtraBits			=0x10;

#elif defined(__CPU_ARM920T__) || defined(__CPU_ARM925T__) || defined(__CPU_ARM926J__)
const TPde KPdeExtraBits			=0x10;

#elif defined(__CPU_SA1__)
const TPde KPdeExtraBits			=0x00;

#elif defined(__CPU_XSCALE__)
const TPde KPdeExtraBits			=0x00;
#endif

//
// PDE/PTE Macros
//


#ifdef __CPU_XSCALE_MANZANO__
#define SECTION_PDE(perm, attr, domain)							\
						(	(((perm)&3)<<10)					|\
							(((attr)&3)<<2)|(((attr)&0x1c)<<10)	|\
							((domain)<<5)						|\
							(KArmV45PdeSection|KPdeExtraBits) )
#else
#define SECTION_PDE(perm, attr, domain)							\
						(	(((perm)&3)<<10)					|\
							(((attr)&3)<<2)|(((attr)&4)<<10)	|\
							((domain)<<5)						|\
							(KArmV45PdeSection|KPdeExtraBits) )
#endif
#define PT_PDE(domain)		(KArmV45PdePageTable|KPdeExtraBits|((domain)<<5))

#define EXPAND_AP(perm)											\
						(	((perm)&0x30)|(((perm)&0x30)<<2)|(((perm)&0x30)<<4)|(((perm)&0x30)<<6) )

#define LP_PTE(perm, attr)										\
						(	EXPAND_AP(perm<<4)					|\
							(((attr)&3)<<2)|(((attr)&4)<<10)	|\
							KArmV45PteLargePage )

#ifdef __CPU_XSCALE__
	#ifdef __CPU_XSCALE_MANZANO__
#define SP_PTE(perm, attr)										\
						(	(((perm)&3)<<4)						|\
							(((attr)&3)<<2)|(((attr)&0x1c)<<4)		|\
							KXScalePteSmallPageX )
	#else
#define SP_PTE(perm, attr)										\
						(	(((perm)&3)<<4)						|\
							(((attr)&3)<<2)|(((attr)&4)<<4)		|\
							KXScalePteSmallPageX )
	#endif
#define	SECTION_PDE_FROM_PDEPTE(pde, pte)						\
						(	(((pte)<< 6)&0x00001c00)			|\
							(((pde)    )&0x000003e0)			|\
							(((pte)    )&0x0000000c)			|\
							(KArmV45PdeSection|KPdeExtraBits) )

#define LP_PTE_FROM_SP_PTE(pte)									\
						(	(((pte)    )&0x0000000c)			|\
							EXPAND_AP(pte)						|\
							(((pte)<< 6)&0x00001000)			|\
							KArmV45PteLargePage )

#define SP_PTE_FROM_LP_PTE(pte)									\
						(	(((pte)    )&0x0000003c)			|\
							(((pte)>> 6)&0x00000040)			|\
							KXScalePteSmallPageX )
							
#else
#define SP_PTE(perm, attr)										\
						(	EXPAND_AP(perm<<4)					|\
							(((attr)&3)<<2)						|\
							KArmV45PteSmallPage )

#define	SECTION_PDE_FROM_PDEPTE(pde, pte)						\
						(	(((pte)<< 6)&0x00000c00)			|\
							(((pde)    )&0x000003e0)			|\
							(((pte)    )&0x0000000c)			|\
							(KArmV45PdeSection|KPdeExtraBits) )

#define LP_PTE_FROM_SP_PTE(pte)									\
						(	(((pte)    )&0x00000ffc)			|\
							KArmV45PteLargePage )

#define SP_PTE_FROM_LP_PTE(pte)									\
						(	(((pte)    )&0x00000ffc)			|\
							KArmV45PteSmallPage )
#endif

TPde* const PageDirectory = ((TPde*)KPageDirectoryBase);


class DArmPlatThread : public DThread
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
	virtual TInt GetNewChunk(DMemModelChunk*& aChunk, SChunkCreateInfo& anInfo);
	virtual TInt GetNewThread(DThread*& aThread, SThreadCreateInfo& anInfo);
	virtual TInt AddFixedAccessChunk(DMemModelChunk* aChunk);
	virtual TInt RemoveFixedAccessChunk(DMemModelChunk* aChunk);
	virtual void CheckForFixedAccess();
	virtual void DoAttributeChange();
private:
	void AdjustDomainAccess(TArmReg aClearMask, TArmReg aSetMask);
	void AdjustThreadAttributes(TUint8 aClearMask, TUint8 aSetMask);
public:
	TInt iDomain;
	TArmReg iDacr;
public:
	friend class Monitor;
	};

class DArmPlatChunk : public DMemModelChunk
	{
public:
	DArmPlatChunk();
	~DArmPlatChunk();
	TInt Create(SChunkCreateInfo& anInfo);
	virtual TInt SetupPermissions();
	virtual void AddPde(TInt anOffset);
	virtual void RemovePde(TInt anOffset);
	virtual void MoveHomePdes(TLinAddr anOldAddr, TLinAddr aNewAddr);
	virtual void MoveCurrentPdes(TLinAddr anOldAddr, TLinAddr aNewAddr);
public:
	TInt iDomain;
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
	virtual TPhysAddr LinearToPhysical(TLinAddr aAddr);
	virtual TInt LinearToPhysical(TLinAddr aAddr, TInt aSize, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList=NULL);
	virtual TInt PreparePagesForDMA(TLinAddr aAddr, TInt aSize, TPhysAddr* aPhysicalPageList);
	virtual TInt ReleasePagesFromDMA(TPhysAddr* aPhysicalPageList, TInt aPageCount);

	virtual void MapRamPages(TInt aId, SPageInfo::TType aType, TAny* aPtr, TUint32 aOffset, const TPhysAddr* aPageList, TInt aNumPages, TPte aPtePerm);
	virtual void MapPhysicalPages(TInt aId, SPageInfo::TType aType, TAny* aPtr, TUint32 aOffset, TPhysAddr aPhysAddr, TInt aNumPages, TPte aPtePerm);
	virtual void RemapPage(TInt aId, TUint32 aAddr, TPhysAddr aOldAddr, TPhysAddr aNewAddr, TPte aPtePerm, DProcess* aProcess);
	virtual TInt UnmapPages(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TBool aSetPagesFree, TInt& aNumPtes, TInt& aNumFree, DProcess* aProcess);
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
	virtual TPte PtePermissions(TChunkType aChunkType);
	virtual TInt RamDefragFault(TAny* aExceptionInfo);
	virtual void DisablePageModification(DMemModelChunk* aChunk, TInt aOffset);

	// overriding Mmu
	virtual void DoAssignPageTable(TInt aId, TLinAddr aAddr, TPde aPdePerm);
	virtual void RemapPageTable(TPhysAddr aOld, TPhysAddr aNewId, TLinAddr aAddr);
	virtual void DoUnassignPageTable(TLinAddr aAddr);
	virtual void MoveChunk(TLinAddr anInitAddr, TUint aSize, TLinAddr aFinalAddr, TPde aPermissions);
	virtual void MoveChunk(TLinAddr anInitAddr, TLinAddr aFinalAddr, TInt aNumPdes);
	virtual void ApplyTopLevelPermissions(TLinAddr anAddr, TUint aChunkSize, TPde aPermissions);
	virtual void ApplyPagePermissions(TInt aId, TInt aPageOffset, TInt aNumPages, TPte aPtePerm);
	virtual void SyncCodeMappings();
	virtual void GenericFlush(TUint32 aMask);
	virtual TPde PdePermissions(TChunkType aChunkType, TInt aChunkState);
	virtual TInt UnlockRamCachePages(TUint8* volatile & aBase, TInt aStartPage, TInt aNumPages);
	virtual TInt LockRamCachePages(TUint8* volatile & aBase, TInt aStartPage, TInt aNumPages);
	virtual void MapVirtual(TInt aId, TInt aNumPages);
	virtual TInt UnmapVirtual(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TBool aSetPagesFree, TInt& aNumPtes, TInt& aNumFree, DProcess* aProcess);
	virtual TLinAddr MapTemp(TPhysAddr aPage, TBool aCached);
	virtual TLinAddr MapSecondTemp(TPhysAddr aPage, TBool aCached);
	virtual void UnmapTemp();
	virtual void UnmapSecondTemp();
	virtual void RemapKernelPage(TInt aId, TLinAddr aSrc, TLinAddr aDest, TPhysAddr aNewPhys, TPte aPtePerm);
	virtual void CacheMaintenanceOnDecommit(const TPhysAddr* aPhysAddr, TInt aPageCount);
	virtual void CacheMaintenanceOnDecommit(const TPhysAddr aPhysAddr);
	virtual void CacheMaintenanceOnPreserve(const TPhysAddr* aPhysAddr, TInt aPageCount, TUint iMapAttr);
	virtual void CacheMaintenanceOnPreserve(const TPhysAddr aPhysAddr, TUint iMapAttr); 
	virtual void CacheMaintenanceOnPreserve(TPhysAddr aPhysAddr, TInt aSize, TLinAddr aLinAddr, TUint iMapAttr);

public:
	static TInt AllocDomain();
	static void FreeDomain(TInt aDomain);
public:
	static NFastMutex DomainLock;
	static TUint32 Domains;

	friend class TScheduler;
	friend class Monitor;
	};

GLREF_D ArmMmu TheMmu;

void CopyPageForRemapWT(TLinAddr aDest, TLinAddr aSrc);
void CopyPageForRemap16(TLinAddr aDest, TLinAddr aSrc);
void CopyPageForRemap32(TLinAddr aDest, TLinAddr aSrc);

#ifdef __CPU_WRITE_BUFFER
#define	__DRAIN_WRITE_BUFFER	InternalCache::DrainBuffers()
#else
#define	__DRAIN_WRITE_BUFFER
#endif


#endif	// __ARM_MEM_H__
