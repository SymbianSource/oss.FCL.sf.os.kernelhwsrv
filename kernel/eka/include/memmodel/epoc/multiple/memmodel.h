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
// e32\include\memmodel\epoc\multiple\memmodel.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __MEMMODEL_H__
#define __MEMMODEL_H__
#include <memmodel/epoc/mmubase/mmubase.h>

#ifdef __SMP__
// SubScheduler fields for each processor
#define	i_AliasLinAddr			iSubSchedScratch[0]
#define	i_AliasPdePtr			iSubSchedScratch[1]
#endif

/********************************************
 * Dynamic Branch Predictor Support
 ********************************************/

/**
@internalComponent
*/
#ifdef __SMP__
//#define LastUserSelfMod                   ((DProcess*&)SubScheduler().iExtras[0])
#else
#define LastUserSelfMod                   ((DProcess*&)TheScheduler.iExtras[0])
#endif

/********************************************
 * RAM Defrag Page Table Moving Support
 ********************************************/

#ifdef __SMP__
/**
@internalComponent
*/
#define AliasRemapOld					((TPhysAddr&)TheScheduler.iSchedScratch[1])

/**
@internalComponent
*/
#define AliasRemapNew					((TPhysAddr&)TheScheduler.iSchedScratch[2])

#else

/**
@internalComponent
*/
#define AliasRemapOld					((TPhysAddr&)TheScheduler.iExtras[1])

/**
@internalComponent
*/
#define AliasRemapNew					((TPhysAddr&)TheScheduler.iExtras[2])
#endif

/********************************************
 * Thread Control Block
 ********************************************/

class DMemModelProcess;

/**
@internalComponent
*/
class DMemModelThread : public DThread
	{
public:
	TInt Alias(TLinAddr aAddr, DMemModelProcess* aProcess, TInt aSize, TInt aPerm, TLinAddr& aAliasAddr, TInt& aAliasSize);
	void RemoveAlias();
	virtual void DoExit1();
	static void RestoreAddressSpace();
public:
	TLinAddr iAliasLinAddr;	// linear address to access aliased memory (0 means no alias is present).
	TPde* iAliasPdePtr;		// Address of PDE which has been modified to make aliased memory accessible.
	TPde iAliasPde;			// PDE to store at iAliasPdePtr.
	TInt iAliasOsAsid;		// asid for the process whoes memory is aliased.
	SDblQueLink iAliasLink;	// link to make TheMmu.iAliasList.
	TLinAddr iAliasTarget;	// linear address of the memory which has been aliased
#ifdef __SMP__
	TInt iCpuRestoreCookie;
#endif
	};


/********************************************
 * Process Control Block
 ********************************************/

class DMemModelChunk;
class DMemModelCodeSegMemory;

/**
@internalComponent
*/
class DMemModelProcess : public DEpocProcess
	{
public:
	void Destruct();
public:
	virtual TInt DoCreate(TBool aKernelProcess, TProcessCreateInfo& aInfo);
	virtual TInt NewChunk(DChunk*& aChunk, SChunkCreateInfo& aInfo, TLinAddr& aRunAddr);
	virtual TInt AddChunk(DChunk* aChunk,TBool isReadOnly);
	virtual TInt NewShPool(DShPool*& aPool, TShPoolCreateInfo& aInfo);
	virtual TInt CreateDataBssStackArea(TProcessCreateInfo& aInfo);
	virtual TInt MapCodeSeg(DCodeSeg* aCodeSeg);
	virtual void UnmapCodeSeg(DCodeSeg* aCodeSeg);
	virtual void RemoveDllData();
	virtual void FinalRelease();
public:
	virtual TInt GetNewChunk(DMemModelChunk*& aChunk, SChunkCreateInfo& aInfo)=0;
public:
	TInt AddChunk(DMemModelChunk* aChunk, TLinAddr& aDataSectionBase, TBool isReadOnly);
	TInt AllocateDataSectionBase(DMemModelChunk& aChunk, TUint& aBase);
	TUint8* DataSectionBase(DMemModelChunk* aChunk);
	void RemoveChunk(DMemModelChunk *aChunk);
	void DoRemoveChunk(TInt aIndex);
	TInt ChunkIndex(DMemModelChunk* aChunk,TInt& aPos);
	TInt CreateCodeChunk();
	void FreeCodeChunk();
	TInt CreateDllDataChunk();
	void FreeDllDataChunk();
	TInt CommitDllData(TLinAddr aBase, TInt aSize);
	void DecommitDllData(TLinAddr aBase, TInt aSize);
	TInt MapUserRamCode(DMemModelCodeSegMemory* aMemory, TBool aLoading);
	void UnmapUserRamCode(DMemModelCodeSegMemory* aMemory, TBool aLoading);
public:
	enum TMemModelProcessAttributes
		{
		ESeparateGlobalSpace=0x40000000,
		EMMProcessAttributesMask = ESeparateGlobalSpace,
		};

	struct SChunkInfo
		{
		DMemModelChunk* iChunk;
		TInt16 iAccessCount;
		TInt16 isReadOnly;
		};

	TInt iChunkCount;
	TInt iChunkAlloc;
	SChunkInfo* iChunks;
	TLinearSection* iLocalSection;
	TInt iOsAsid;
	TPhysAddr iLocalPageDir;
	TPhysAddr iGlobalPageDir;
	TUint32 iAddressCheckMaskR;
	TUint32 iAddressCheckMaskW;
	DMemModelChunk* iCodeChunk;
	DMemModelChunk* iDllDataChunk;
	TInt iSelfModChunks;
public:
	friend class Monitor;
	};


/********************************************
 * Chunk Control Block
 ********************************************/

/**
@internalComponent
*/
class DMemModelChunk : public DChunk
	{
public:
	/**
	@see DChunk::TChunkAttributes for generic attribute flags
	*/
	enum TMemModelChunkAttributes
		{
		EPrivate			=0x80000000,
		ECode				=0x40000000,
		EAddressAllocDown	=0x20000000,

		EAddressRangeMask	=0x0f000000,
		EAddressRangeShift	=24,
		EAddressLocal		=0x00000000,
		EAddressShared		=0x01000000,
		EAddressUserGlobal	=0x02000000,
		EAddressKernel		=0x03000000,
		EAddressFixed		=0x04000000,

		EMapTypeMask		=0x00c00000,
		EMapTypeLocal		=0x00000000,
		EMapTypeGlobal		=0x00400000,
		EMapTypeShared		=0x00800000,

		EMMChunkAttributesMask = EPrivate|ECode|EAddressAllocDown|EAddressRangeMask|EMapTypeMask,
		};
	
public:
	DMemModelChunk();
	void Destruct();
public:
	virtual TInt Close(TAny* aPtr);
	virtual TInt DoCreate(SChunkCreateInfo& aInfo);
	virtual TInt Adjust(TInt aNewSize);
	virtual TInt AdjustDoubleEnded(TInt aBottom, TInt aTop);
	virtual TInt CheckAccess();
	virtual TInt Commit(TInt aOffset, TInt aSize, TCommitType aCommitType=DChunk::ECommitDiscontiguous, TUint32* aExtraArg=0);
	virtual TInt Allocate(TInt aSize, TInt aGuard=0, TInt aAlign=0);
	virtual TInt Decommit(TInt aOffset, TInt aSize);
	virtual TInt Lock(TInt anOffset, TInt aSize);
	virtual TInt Unlock(TInt anOffset, TInt aSize);
	virtual TInt Address(TInt aOffset, TInt aSize, TLinAddr& aKernelAddress);
	virtual TInt PhysicalAddress(TInt aOffset, TInt aSize, TLinAddr& aKernelAddress, TUint32& aPhysicalAddress, TUint32* aPhysicalPageList=NULL);
	virtual void BTracePrime(TInt aCategory);
	virtual void Substitute(TInt aOffset, TPhysAddr aOldAddr, TPhysAddr aNewAddr);
	virtual TUint8* Base(DProcess* aProcess);
	inline TUint8* Base() const { return DChunk::Base(); }
public:
	TInt Decommit(TInt aOffset, TInt aSize, TDecommitType aDecommitType);
	void ClaimInitialPages();
	void SetFixedAddress(TLinAddr aAddr, TInt aInitialSize);
	TInt Reserve(TInt aInitialSize);
	TInt DoCommit(TInt aOffset, TInt aSize, TCommitType aCommitType=DChunk::ECommitDiscontiguous, TUint32* aExtraArg=0);
	void DoDecommit(TInt aOffset, TInt aSize, TDecommitType aDecommitType=EDecommitNormal);
	TInt AllocateAddress();
	void ApplyPermissions(TInt aOffset, TInt aSize, TPte aPtePerm);
	TLinearSection* LinearSection();
	TZonePageType GetPageType();

public:
	virtual TInt SetupPermissions()=0;

public:
	TBitMapAllocator* iOsAsids;			// NULL for local or fully global else list of OS ASIDs
	TPte iPtePermissions;
	TPde iPdePermissions;
	TUint16* iPageTables;
	TBitMapAllocator* iPageBitMap;		// NULL if not disconnected chunk
	TBitMapAllocator* iPermanentPageBitMap;
	DMemModelChunk* iKernelMirror;
public:
	friend class Monitor;
	};


/********************************************
 * Code segment
 ********************************************/

/**
@internalComponent
*/
class DMemModelCodeSegMemory : public DMmuCodeSegMemory
	{
public:
	DMemModelCodeSegMemory(DEpocCodeSeg* aCodeSeg);
	~DMemModelCodeSegMemory();
	TInt Create(TCodeSegCreateInfo& aInfo);
	TInt Loaded(TCodeSegCreateInfo& aInfo);
	void Substitute(TInt aOffset, TPhysAddr aOld, TPhysAddr aNew);
	void Destroy();
public:
	DMemModelProcess* iCreator;	// process loading this code segment

	TPhysAddr* iPages;			// list of physical pages (iPageCount+iDataPageCount)

	/**
	List of OS ASIDs this code segment is mapped into.
	Protected by RamAllocMutex and System Lock.
	*/
	TBitMapAllocator* iOsAsids;

	TLinAddr* iCopyOfExportDir;	// kernel side copy of export directory or NULL
	};

/**
@internalComponent
*/
class DMemModelCodeSeg: public DEpocCodeSeg
	{
public:
	DMemModelCodeSeg();
	virtual ~DMemModelCodeSeg();
	virtual TInt DoCreateRam(TCodeSegCreateInfo& aInfo, DProcess* aProcess);
	virtual TInt DoCreateXIP(DProcess* aProcess);
	virtual TInt Loaded(TCodeSegCreateInfo& aInfo);
	virtual void ReadExportDir(TUint32* aDest);
	virtual TBool FindCheck(DProcess* aProcess);
	virtual TBool OpenCheck(DProcess* aProcess);
	virtual void BTracePrime(TInt aCategory);
	inline DMemModelCodeSegMemory* Memory()
		{ return (DMemModelCodeSegMemory*)iMemory; }
	inline TPhysAddr* Pages()
		{ return iMemory!=0 ? Memory()->iPages : (TPhysAddr*)0; }
public:
	TInt iCodeAllocBase;
	TInt iDataAllocBase;
	TAny* iKernelData;			// only for kernel modules
	};


/********************************************
 * MMU stuff
 ********************************************/

/**
@internalComponent
Indicates that corresponding linear address applies to unknown address space.
Common for EMemTypeShared types of chunks with no owning process.
*/
#define	UNKNOWN_MAPPING	((TInt)-2)

/**
@internalComponent
Indicates that corresponding linear address applies to global address space.
*/
#define	GLOBAL_MAPPING	((const TAny*)-1)

	/**
@internalComponent
Indicates that corresponding linear address applies to kernel process (either global or Kernel's local space).
*/
#define	KERNEL_MAPPING	((TInt)0)
	
	
/**
@internalComponent
*/
class Mmu : public MmuBase
	{
public:
	enum TFlushFlags	{
						EFlushDTLB=0x01,
						EFlushDCache=0x02,
						EFlushITLB=0x04,
						EFlushICache=0x08,
						EFlushDDecommit=0x80000000,
						EFlushDPermChg=0x20000000,
						EFlushDMove=0x40000000,
						EFlushIPermChg=0x04000000,
						EFlushIMove=0x10000000,
						EFlushInheritMask=EFlushDPermChg|EFlushDMove|EFlushIPermChg|EFlushIMove,
						};

	enum TPanic
		{
		ELocalPageDirBadAsid,
		EGlobalPageDirBadAsid,
		EPDEBadAsid,
		EFreeOsAsidBadAsid,
		EOsAsidAllocCreateFailed,
		EBadInitialPageAddr,
		EAssignPageTableInvalidUsage,
		EUserCodeAllocatorCreateFailed,
		EDllDataAllocatorCreateFailed,
		ERomUserDataAddressInvalid,
		ERomUserDataSizeInvalid,
		ECreateSharedSectionFailed,
		ECreateUserGlobalSectionFailed,
		ERemapPageFailed,
		ERemapPageTableFailed,
		EFixupXPTFailed,
		ETempMappingFailed,		
		EDefragDisablePageFailed,
		EDefragFaultWhilstFMHeld,
		};

public:
	TPde* LocalPageDir(TInt aOsAsid);
	TPde* GlobalPageDir(TInt aOsAsid);
	TPde& PDE(TLinAddr aAddr, TInt aOsAsid);
	TInt NewOsAsid(TBool aSeparateGlobal);
	void FreeOsAsid(TInt aOsAsid);
	void CreateUserGlobalSection(TLinAddr aBase, TLinAddr aEnd);
	TInt CreateGlobalCodeChunk();

	// virtual - inherited/overridden from MmuBase
	virtual void Init1();
//	virtual void Init2();
	virtual void DoInit2();
//	virtual TBool PteIsPresent(TPte aPte)=0;
//	virtual TPhysAddr PtePhysAddr(TPte aPte, TInt aPteIndex)=0;
//	virtual TPhysAddr PdePhysAddr(TLinAddr aAddr)=0;
	virtual void SetupInitialPageInfo(SPageInfo* aPageInfo, TLinAddr aChunkAddr, TInt aPdeIndex);
	virtual void SetupInitialPageTableInfo(TInt aId, TLinAddr aChunkAddr, TInt aNumPtes);
	virtual void AssignPageTable(TInt aId, TInt aUsage, TAny* aObject, TLinAddr aAddr, TPde aPdePerm);
	virtual TInt UnassignPageTable(TLinAddr aAddr);
//	virtual void BootstrapPageTable(TInt aXptId, TPhysAddr aXptPhys, TInt aId, TPhysAddr aPhysAddr)=0;
	virtual TInt PageTableId(TLinAddr aAddr);
//	virtual TInt BootPageTableId(TLinAddr aAddr, TPhysAddr& aPtPhys)=0;
//	virtual void ClearPageTable(TInt aId, TInt aFirstIndex=0)=0;
	virtual TPhysAddr LinearToPhysical(TLinAddr aAddr);
	virtual TInt LinearToPhysical(TLinAddr aAddr, TInt aSize, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList=NULL);
//	virtual void MapRamPages(TInt aId, SPageInfo::TType aType, TAny* aPtr, TUint32 aOffset, const TPhysAddr* aPageList, TInt aNumPages, TPte aPtePerm)=0;
//	virtual void MapPhysicalPages(TInt aId, SPageInfo::TType aType, TAny* aPtr, TUint32 aOffset, TPhysAddr aPhysAddr, TInt aNumPages, TPte aPtePerm)=0;
//	virtual TInt UnmapPages(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TBool aSetPagesFree, TInt& aNumPtes, TInt& aNumFree, DProcess* aProcess)=0;
//	virtual void ClearRamDrive(TLinAddr aStart)=0;
//	virtual TInt PdePtePermissions(TUint& aMapAttr, TPde& aPde, TPte& aPte)=0;
//	virtual void Map(TLinAddr aLinAddr, TPhysAddr aPhysAddr, TInt aSize, TPde aPdePerm, TPte aPtePerm, TInt aMapShift)=0;
//	virtual void Unmap(TLinAddr aLinAddr, TInt aSize)=0;
//	virtual void InitShadowPageTable(TInt aId, TLinAddr aRomAddr, TPhysAddr aOrigPhys)=0;
//	virtual void InitShadowPage(TPhysAddr aShadowPhys, TLinAddr aRomAddr)=0;
//	virtual void DoUnmapShadowPage(TInt aId, TLinAddr aRomAddr, TPhysAddr aOrigPhys)=0;
//	virtual TInt UnassignShadowPageTable(TLinAddr aRomAddr, TPhysAddr aOrigPhys)=0;
//	virtual void DoFreezeShadowPage(TInt aId, TLinAddr aRomAddr)=0;
//	virtual void FlushShadow(TLinAddr aRomAddr)=0;
//	virtual void AssignShadowPageTable(TInt aId, TLinAddr aRomAddr)=0;
//	virtual void ClearPages(TInt aNumPages, TPhysAddr* aPageList)=0;
	virtual TPte PtePermissions(TChunkType aChunkType)=0;
	virtual TInt MoveKernelPage(DChunk* aChunk, TUint32 aOffset, TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest);
	virtual TInt MoveCodeSegMemoryPage(DMemModelCodeSegMemory* aCodeSegMemory, TUint32 aOffset, TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest);
	virtual TInt MoveCodeChunkPage(DChunk* aChunk, TUint32 aOffset, TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest);
	virtual TInt MoveDataChunkPage(DChunk* aChunk, TUint32 aOffset, TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest);

	// pure virtual - new in Mmu
	virtual TInt NewPageDirectory(TInt aOsAsid, TBool aSeparateGlobal, TPhysAddr& aPhysAddr, TInt& aNumPages)=0;
	virtual void InitPageDirectory(TInt aOsAsid, TBool aGlobal)=0;
	virtual TInt PageTableId(TLinAddr aAddr, TInt aOsAsid)=0;
	virtual TPhysAddr LinearToPhysical(TLinAddr aAddr, TInt aOsAsid)=0;
	virtual TInt LinearToPhysical(TLinAddr aAddr, TInt aSize, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList, TInt aOsAsid)=0;
	virtual TInt PreparePagesForDMA(TLinAddr aAddr, TInt aSize, TInt aOsAsid, TPhysAddr* aPhysicalPageList)=0;
	virtual TInt ReleasePagesFromDMA(TPhysAddr* aPhysicalPageList, TInt aPageCount)=0;
	virtual void DoAssignPageTable(TInt aId, TLinAddr aAddr, TPde aPdePerm, const TAny* aOsAsids)=0;
	virtual void RemapPageTableSingle(TPhysAddr aOld, TPhysAddr aNew, TLinAddr aAddr, TInt aOsAsid)=0;
	virtual void RemapPageTableMultiple(TPhysAddr aOld, TPhysAddr aNew, TLinAddr aAddr, const TAny* aOsAsids)=0;
	virtual void RemapPageTableGlobal(TPhysAddr aOld, TPhysAddr aNew, TLinAddr aAddr)=0;
	virtual void RemapPageTableAliases(TPhysAddr aOld, TPhysAddr aNew)=0;
	virtual void DoUnassignPageTable(TLinAddr aAddr, const TAny* aOsAsids)=0;
	virtual TPde PdePermissions(TChunkType aChunkType, TBool aRO)=0;
	virtual void ApplyTopLevelPermissions(TLinAddr aAddr, TInt aOsAsid, TInt aNumPdes, TPde aPdePerm)=0;
	virtual void ApplyPagePermissions(TInt aId, TInt aPageOffset, TInt aNumPages, TPte aPtePerm)=0;
	virtual void GenericFlush(TUint32 aMask)=0;
	virtual TLinAddr MapTemp(TPhysAddr aPage,TLinAddr aLinAddr, TInt aPages=1)=0;
	virtual TLinAddr MapTemp(TPhysAddr aPage,TLinAddr aLinAddr,TInt aPages, TMemoryType aMemType)=0;
	virtual TLinAddr MapSecondTemp(TPhysAddr aPage,TLinAddr aLinAddr, TInt aPages=1)=0;
	virtual void UnmapTemp()=0;
	virtual void UnmapSecondTemp()=0;
	virtual TBool ValidateLocalIpcAddress(TLinAddr aAddr,TInt aSize,TBool aWrite)=0;
	virtual TInt UnlockRamCachePages(TLinAddr aLinAddr, TInt aNumPages, DProcess* aProcess)=0;
	virtual TInt LockRamCachePages(TLinAddr aLinAddr, TInt aNumPages, DProcess* aProcess)=0;
	virtual void MapVirtual(TInt aId, TInt aNumPages)=0;
	virtual TInt UnmapUnownedPages(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TLinAddr* aLAPageList, TInt& aNumPtes, TInt& aNumFree, DProcess* aProcess)=0;
	virtual TInt UnmapVirtual(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TBool aSetPagesFree, TInt& aNumPtes, TInt& aNumFree, DProcess* aProcess)=0;
	virtual TInt UnmapUnownedVirtual(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TLinAddr* aLAPageList, TInt& aNumPtes, TInt& aNumFree, DProcess* aProcess)=0;
	virtual void RemapPageByAsid(TBitMapAllocator* aOsAsids, TLinAddr aLinAddr, TPhysAddr aOldAddr, TPhysAddr aNewAddr, TPte aPtePerm)=0;
	virtual void CacheMaintenanceOnDecommit(const TPhysAddr* aPhysAddr, TInt aPageCount)=0;
	virtual void CacheMaintenanceOnDecommit(const TPhysAddr aPhysAddr)=0; // Maintains physical (VIPT & PIPT) cache for pages to be reused. 
	virtual void CacheMaintenanceOnPreserve(const TPhysAddr* aPhysAddr, TInt aPageCount, TUint iMapAttr)=0;
	virtual void CacheMaintenanceOnPreserve(const TPhysAddr aPhysAddr, TUint iMapAttr)=0;
	virtual void CacheMaintenanceOnPreserve(TPhysAddr aPhysAddr, TInt aSize, TLinAddr aLinAddr, TUint iMapAttr)=0;
	
public:
	inline static Mmu& Get()
		{return *(Mmu*)TheMmu;}
	static void Panic(TPanic aPanic);
public:
	TInt iNumOsAsids;
	TInt iNumGlobalPageDirs;
	TBitMapAllocator* iOsAsidAllocator;
	TInt iGlobalPdSize;
	TInt iGlobalPdShift;
	TInt iLocalPdSize;
	TInt iLocalPdShift;
	TInt iAsidGroupSize;			// number of global page directories mapped by a page table
	TInt iAsidGroupMask;			// number of global page directories mapped by a page table - 1
	TInt iAsidGroupShift;			// log2(number of global page directories mapped by a page table)
	TInt iAliasSize;				// minimum allowed spacing between synonyms of any physical address
	TInt iAliasMask;
	TInt iAliasShift;
	TLinAddr iUserLocalBase;		// lowest local data address
	TLinAddr iUserLocalEnd;			// 1+highest local data address (lowest DLL data address)
	TLinAddr iUserSharedBase;		// lowest shared data address (1+highest DLL data address)
	TLinAddr iUserSharedEnd;		// 1+highest shared data address (=local PD size)
	TLinAddr iDllDataBase;
	TInt iMaxDllDataSize;
	TLinAddr iUserCodeBase;
	TInt iMaxUserCodeSize;
	TUint32* iAsidInfo;
	TLinAddr iPdeBase;
	TPte iPdPtePerm;
	TPde iPdPdePerm;
	TPte iUserCodeLoadPtePerm;
	TPte iKernelCodePtePerm;
	TPte iGlobalCodePtePerm;
	TUint32 iRamDriveMask;
	TLinearSection* iSharedSection;
	TLinearSection* iUserGlobalSection;
	DMemModelChunk* iGlobalCode;
	SDblQue iAliasList;
	TInt iTempMapCount;
	TInt iSecondTempMapCount;
	TPte* iSecondTempPte;		// second PTE used for temporary mappings
	TLinAddr iSecondTempAddr;	// address corresponding to iSecondTempPte
	TInt iCacheMaintenanceTempMapAttr;	// holds SP_PTE's attr. entry for cache maintenance
										// temporary mapping.
public:
	friend class Monitor;
	friend TPte& PageTableEntry(TLinAddr aLinAddr);
	};


/********************************************
 * Functions/Data defined in memory model
 ********************************************/

/**
@internalComponent
*/
class MM
	{
public:
	enum TMemModelPanic
		{
		EChunkTransferBadOwner=0,
		EChunkDecommitNoPageTable=1,
		EChunkTransferAllocAddrFailed=2,
		EFsRegisterThread=3,
		EClaimInitialPagesBadPageTable=4,
		EChunkNotDisconnected1=5,
		EChunkNotDisconnected2=6,
		EChunkCommitNoPageTable=7,
		EProcessDestructChunksRemaining=8,
		ECommitInvalidDllDataAddress=9,
		EDecommitInvalidDllDataAddress=10,
		EChunkApplyPermissions1=11,
		EChunkApplyPermissions2=12,
		ECodeSegLoadedNotCreator=13,
		EChunkBadAddressRange=14,
		EPdeAlreadyInUse=15,
		EPteAlreadyInUse=16,
		EMmuMapNoPageTable=17,
		EUnmapBadAlignment=18,
		EBootstrapPageTableBadAddr=19,
		ETempMappingAlreadyInUse=20,
		EDecommitFailed=21,
		EPageTableNotFound=22,
		EUnexpectedPageType=23,
		EOperationNotSupported=24,
		EChunkRemapNoPageTable=25,
		EChunkRemapUnsupported=26,
		ECodeSegRemapWrongPage=27,
		EChunkRemapWrongPageTable=28,
		ETempMappingNoRoom=29,
		};

	static void Panic(TMemModelPanic aPanic);
public:
	static void Init1();
	static void StartCrashDebugger();
public:
	static TInt MaxPagesInOneGo;
	static DMemModelChunk* SvStackChunk;
	static DMemModelChunk* TheRamDriveChunk;
	static TBitMapAllocator* UserCodeAllocator;
	static TBitMapAllocator* DllDataAllocator;
	};

#endif
