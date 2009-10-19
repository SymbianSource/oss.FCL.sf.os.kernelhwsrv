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
// e32\include\memmodel\epoc\moving\memmodel.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __MEMMODEL_H__
#define __MEMMODEL_H__
#include <memmodel/epoc/mmubase/mmubase.h>

/********************************************
 * Deterministic Scheduler Implementation
 ********************************************/

/**
@internalComponent
*/
#define TheCurrentAddressSpace			((DMemModelProcess*&)TheScheduler.iAddressSpace)

/**
@internalComponent
*/
#define TheCurrentVMProcess				((DMemModelProcess*&)TheScheduler.iExtras[0])

/**
@internalComponent
*/
#define TheCurrentDataSectionProcess	((DMemModelProcess*&)TheScheduler.iExtras[1])

/**
@internalComponent
*/
#define TheCompleteDataSectionProcess	((DMemModelProcess*&)TheScheduler.iExtras[2])

/**
@internalComponent
*/
#define FlushProgress					((TInt&)TheScheduler.iExtras[3])


/********************************************
 * Process Control Block
 ********************************************/
/**
@internalComponent
*/
const TInt KMaxChunksInProcess=16;

class DMemModelChunk;
/**
@internalComponent
*/
class DMemModelProcess : public DEpocProcess
	{
public:
	void Destruct();
public:
	virtual TInt DoCreate(TBool aKernelProcess, TProcessCreateInfo& aInfo);
	virtual TInt NewChunk(DChunk*& aChunk, SChunkCreateInfo& anInfo, TLinAddr& aRunAddr);
	virtual TInt AddChunk(DChunk* aChunk,TBool isReadOnly);
	virtual TInt NewShPool(DShPool*& aPool, TShPoolCreateInfo& aInfo);
	virtual TInt CreateDataBssStackArea(TProcessCreateInfo& aInfo);
	virtual TInt MapCodeSeg(DCodeSeg* aCodeSeg);
	virtual void UnmapCodeSeg(DCodeSeg* aCodeSeg);
	virtual void RemoveDllData();
	virtual void FinalRelease();
public:
	virtual TInt GetNewChunk(DMemModelChunk*& aChunk, SChunkCreateInfo& anInfo)=0;
	virtual TInt AddFixedAccessChunk(DMemModelChunk *aChunk)=0;
	virtual TInt RemoveFixedAccessChunk(DMemModelChunk *aChunk)=0;
	virtual void CheckForFixedAccess()=0;
	virtual void DoAttributeChange()=0;
public:
	TInt AddChunk(DMemModelChunk* aChunk, TLinAddr& aDataSectionBase, TBool isReadOnly);
	TInt AllocateDataSectionBase(DMemModelChunk& aChunk, TUint& aBase);
	TUint8* DataSectionBase(DMemModelChunk* aChunk);
	void RemoveChunk(DMemModelChunk *aChunk);
	void DoRemoveChunk(TInt aIndex);
	TInt ChunkIndex(DMemModelChunk* aChunk,TInt& aPos);
	TInt CreateDllDataChunk();
	void FreeDllDataChunk();
	TInt CommitDllData(TLinAddr aBase, TInt aSize);
	void DecommitDllData(TLinAddr aBase, TInt aSize);
public:
	enum TMemModelProcessAttributes
		{
		EFixedAddress=1,
		EMoving=0x40000000,
		EVariableAccess=0x20000000,
//		EMovingCode=0x10000000,		NEVER USED
		EVariableCode=0x04000000,
		EMMProcessAttributesMask = EFixedAddress|EMoving|EVariableAccess|EVariableCode,
		};

	struct SChunkInfo
		{
		TLinAddr iDataSectionBase;
		DMemModelChunk *iChunk;
		TInt16 iAccessCount;
		TInt16 isReadOnly;
		};

	TInt iNumChunks;
	SChunkInfo iChunks[KMaxChunksInProcess];
	TInt iNumMovingChunks;
	TInt iNumNonFixedAccessChunks;
	TInt iNumNonFixedAccessCodeChunks;
	DMemModelChunk* iDllDataChunk;
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
		EFixedAccess	=0x80000000,
		EFixedAddress	=0x40000000,
		EPrivate		=0x10000000,
		ECode			=0x08000000,

		EMMChunkAttributesMask = EFixedAccess | EFixedAddress | EPrivate | ECode,
		};

	enum TChunkState {ENotRunning=0, ERunningRO=1, ERunningRW=2};
public:
	DMemModelChunk();
	void Destruct();
public:
	virtual TInt Close(TAny* aPtr);
	virtual TInt DoCreate(SChunkCreateInfo& anInfo);
	virtual TInt Adjust(TInt aNewSize);
	virtual TInt AdjustDoubleEnded(TInt aBottom, TInt aTop);
	virtual TInt CheckAccess();
	virtual TInt Commit(TInt anOffset, TInt aSize, TCommitType aCommitType=DChunk::ECommitDiscontiguous, TUint32* aExtraArg=0);
	virtual TInt Allocate(TInt aSize, TInt aGuard=0, TInt aAlign=0);
	TInt Decommit(TInt anOffset, TInt aSize);
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
	TInt FindFree(TInt aSize, TInt aGuard, TInt aAlign);
	void SetFixedAddress(TLinAddr anAddr, TInt anInitialSize);
	TInt Reserve(TInt anInitialSize);
	TUint32 ApplyTopLevelPermissions(TChunkState aChunkState);
	TUint32 MoveToRunAddress(TLinAddr aLinearAddr,TChunkState aChunkState);
	TUint32 MoveToHomeSection();
	TZonePageType GetPageType();
protected:
	TInt DoCommit(TInt aOffset, TInt aSize, TCommitType aCommitType=DChunk::ECommitDiscontiguous, TUint32* aExtraArg=0);
	void DoDecommit(TInt aOffset, TInt aSize, TDecommitType aDecommitType=EDecommitNormal);
private:
	void ClaimInitialPages();
	TInt ExpandHomeRegion(TInt anOffset, TInt aSize);
	TLinAddr AllocateHomeAddress(TInt aSize);
	void DeallocateHomeAddress();
	TLinAddr ReallocateHomeAddress(TInt aNewSize);
	TInt DoAllocate(TInt aSize, TInt aGuard, TInt aAlign, TBool aCommit);
private:
	virtual TInt SetupPermissions()=0;
	virtual void AddPde(TInt anOffset)=0;
	virtual void RemovePde(TInt anOffset)=0;
	virtual void MoveHomePdes(TLinAddr anOldAddr, TLinAddr aNewAddr)=0;
	virtual void MoveCurrentPdes(TLinAddr anOldAddr, TLinAddr aNewAddr)=0;
public:
	TChunkState iChunkState;
	TPte iPtePermissions;
	TPde iPdePermissions[3];	// indexed by iChunkState
	TInt iHomeRegionOffset;
	TInt iHomeRegionSize;
	TLinAddr iHomeRegionBase;
	TLinAddr iHomeBase;
	TInt iNumPdes;
	TPde* iPdes;
	TPde* iHomePdes;
	TUint32* iPdeBitMap;
	TBitMapAllocator* iPageBitMap;
	TBitMapAllocator* iPermanentPageBitMap;
public:
	friend class Monitor;
	};


/********************************************
 * Code segment
 ********************************************/

class DMemModelCodeSegMemory : public DMmuCodeSegMemory
	{
public:
	DMemModelCodeSegMemory(DEpocCodeSeg* aCodeSeg);
	~DMemModelCodeSegMemory();
	TInt Create(TCodeSegCreateInfo& aInfo);
	TInt Loaded(TCodeSegCreateInfo& aInfo);
	void Destroy();
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
	inline DMemModelCodeSegMemory* Memory()
		{ return (DMemModelCodeSegMemory*)iMemory; }
public:
	TInt iCodeAllocBase;
	TInt iDataAllocBase;
	TAny* iKernelData;			// only for kernel modules
	};


/********************************************
 * MMU stuff
 ********************************************/

typedef void (*TCopyPageFn)(TLinAddr aDest, TLinAddr aSrc);

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
						EFlushDPermChg=0x20000000,		// =DProcess::EVariableAccess
						EFlushDMove=0x40000000,			// =DProcess::EMoving
						EFlushIPermChg=0x04000000,		// =DProcess::EVariableCode
						EFlushIMove=0x10000000,			// =DProcess::EMovingCode
						EFlushInheritMask=EFlushDPermChg|EFlushDMove|EFlushIPermChg|EFlushIMove,
						};

	enum TPanic
		{
		EBadInitialPageAddr,
		EAssignPageTableInvalidUsage,
		EUserCodeAllocatorCreateFailed,
		EDllDataAllocatorCreateFailed,
		ERomUserDataAddressInvalid,
		ERomUserDataSizeInvalid,
		ERomLinearAddressInvalid,
		ERemapPageFailed,
		ERemapPageTableFailed,
		ENoCopyPageFunction,
		EFixupXPTFailed,
		ECacheMaintenance,
		EDefragDisablePageFailed,
		EDefragFaultWhilstFMHeld,
		EDefragKernelChunkNoPageTable,
		};

public:
	inline TPde& PDE(TLinAddr aAddr)
		{return *(((TPde*)iPdeBase)+(aAddr>>iChunkShift));}

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
//	virtual TPhysAddr LinearToPhysical(TLinAddr aAddr)=0;
//	virtual TInt LinearToPhysical(TLinAddr aAddr, TInt aSize, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList=NULL)=0;
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
	virtual void DoAssignPageTable(TInt aId, TLinAddr aAddr, TPde aPdePerm)=0;
	virtual void RemapPageTable(TPhysAddr aOld, TPhysAddr aNewId, TLinAddr aAddr)=0;
	virtual void DoUnassignPageTable(TLinAddr aAddr)=0;
	virtual void MoveChunk(TLinAddr anInitAddr, TUint aSize, TLinAddr aFinalAddr, TPde aPermissions)=0;
	virtual void MoveChunk(TLinAddr anInitAddr, TLinAddr aFinalAddr, TInt aNumPdes)=0;
	virtual void ApplyTopLevelPermissions(TLinAddr anAddr, TUint aChunkSize, TPde aPermissions)=0;
	virtual void ApplyPagePermissions(TInt aId, TInt aPageOffset, TInt aNumPages, TPte aPtePerm)=0;
	virtual void SyncCodeMappings()=0;
	virtual void GenericFlush(TUint32 aMask)=0;
	virtual TPde PdePermissions(TChunkType aChunkType, TInt aChunkState)=0;
	virtual TInt UnlockRamCachePages(TUint8* volatile & aBase, TInt aFirstPage, TInt aNumPages)=0;
	virtual TInt LockRamCachePages(TUint8* volatile & aBase, TInt aFirstPage, TInt aNumPages)=0;
	virtual void MapVirtual(TInt aId, TInt aNumPages)=0;
	virtual TInt UnmapVirtual(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TBool aSetPagesFree, TInt& aNumPtes, TInt& aNumFree, DProcess* aProcess)=0;
	virtual TLinAddr MapTemp(TPhysAddr aPage, TBool aCached)=0;
	virtual TLinAddr MapSecondTemp(TPhysAddr aPage, TBool aCached)=0;
	virtual void UnmapTemp()=0;
	virtual void UnmapSecondTemp()=0;
	virtual void RemapKernelPage(TInt aId, TLinAddr aSrc, TLinAddr aDest, TPhysAddr aNewPhys, TPte aPtePerm)=0;
	virtual TInt PreparePagesForDMA(TLinAddr aAddr, TInt aSize, TPhysAddr* aPhysicalPageList)=0;
	virtual TInt ReleasePagesFromDMA(TPhysAddr* aPhysicalPageList, TInt aPageCount)=0;
	
public:
	TInt GetPageTableId(TLinAddr aAddr);
public:
	inline static Mmu& Get()
		{return *(Mmu*)TheMmu;}
	inline void CopyPageForRemap(TLinAddr aDest, TLinAddr aSrc)
		{iCopyPageFn(aDest, aSrc);}
	static void Panic(TPanic aPanic);
public:
	TLinAddr iDataSectionBase;		// lowest data section address
	TLinAddr iDllDataBase;			// start of DLL static data area
	TLinAddr iDataSectionEnd;		// highest data section address + 1
	TInt iMaxDllDataSize;
	TLinAddr iUserCodeBase;
	TInt iMaxUserCodeSize;
	TLinAddr iKernelCodeBase;
	TInt iMaxKernelCodeSize;
	TLinAddr iPdeBase;
	TPte iUserCodeLoadPtePerm;
	TPte iKernelCodePtePerm;
	TUint32* iHomePdeMap;
	TCopyPageFn iCopyPageFn;
	TPte* iSecondTempPte;		// second PTE used for temporary mappings
	TLinAddr iSecondTempAddr;	// address corresponding to iSecondTempPte
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
		EUserCodeNotFixed=0,
		EClaimInitialPagesBadPageTable=1,
		EFreeInvalidDomain=2,
		EFreeDomainNotAllocated=3,
		EFixedChunkMoving=4,
		EChunkDecommitNoPageTable=5,
		ECommitInvalidDllDataAddress=6,
		EDecommitInvalidDllDataAddress=7,
		EPdeAlreadyInUse=8,
		EPteAlreadyInUse=9,
		EMmuMapNoPageTable=10,
		EUnmapBadAlignment=11,
		EBootstrapPageTableBadAddr=12,
		EAddFixedBadPerm=13,
		ERemoveFixedBadPerm=14,
		EUnexpectedPageType=15,
		EOperationNotImplemented=16,
		ECodeAddressOutOfRange=17,
		ETempMappingAlreadyInUse=18,
		EChunkRemapNoPageTable=19,
		EChunkRemapWrongPageTable=20,
		};

	static void Panic(TMemModelPanic aPanic);
public:
	static void Init1();
	static TAny* CurrentAddress(DThread* aThread, const TAny* aAddress, TInt aSize, TBool aWrite);
	static void StartCrashDebugger();
	static TInt CreateCodeChunk(TBool aKernel);
public:
	static TInt MaxPagesInOneGo;
	static DMemModelChunk* SvStackChunk;
	static DMemModelChunk* TheRamDriveChunk;
	static DMemModelChunk* UserCodeChunk;
	static DMemModelChunk* KernelCodeChunk;
	static TBitMapAllocator* DllDataAllocator;
	};

#endif
