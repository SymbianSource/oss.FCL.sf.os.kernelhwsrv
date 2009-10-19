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
// eka\include\memmodel\epoc\multiple\x86\x86_mem.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalComponent
*/

#ifndef __X86_MEM_H__
#define __X86_MEM_H__
#include <mmboot.h>

TPde* const InitPageDirectory = ((TPde*)KPageDirectoryBase);	// initial page directory

inline TPde* PageDirectory(TInt aOsAsid)
	{
	return (TPde*)(KPageDirectoryBase+(aOsAsid<<KPageTableShift));
	}

typedef TInt (__fastcall *TMagicExcHandler)(TX86ExcInfo*);
class DX86PlatThread : public DMemModelThread
	{
public:
	DX86PlatThread();
	~DX86PlatThread();
	virtual TInt SetupContext(SThreadCreateInfo& aInfo);
	virtual TInt Context(TDes8& aDes);
	virtual void DoExit2();
public:
	TMagicExcHandler iMagicExcHandler;
public:
	friend class Monitor;
	};

class DX86PlatProcess : public DMemModelProcess
	{
public:
	DX86PlatProcess();
	~DX86PlatProcess();
public:
	virtual TInt GetNewChunk(DMemModelChunk*& aChunk, SChunkCreateInfo& aInfo);
	virtual TInt GetNewThread(DThread*& aThread, SThreadCreateInfo& aInfo);
private:
	friend class Monitor;
	};

class DX86PlatChunk : public DMemModelChunk
	{
public:
	DX86PlatChunk();
	~DX86PlatChunk();
	TInt Create(SChunkCreateInfo& aInfo);
	virtual TInt SetupPermissions();
	TInt SetAttributes(SChunkCreateInfo& aInfo);
public:
	friend class Monitor;
	};

class X86Mmu : public Mmu
	{
public:
	// overriding MmuBase
	virtual void Init1();
//	virtual void Init2();
	virtual void DoInit2();
	virtual THwChunkAddressAllocator* MappingRegion(TUint aMapAttr);
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
	virtual TInt CopyToShadowMemory(TLinAddr aDest, TLinAddr aSrc, TUint32 aLength);
	virtual void DoFreezeShadowPage(TInt aId, TLinAddr aRomAddr);
	virtual void FlushShadow(TLinAddr aRomAddr);
	virtual void AssignShadowPageTable(TInt aId, TLinAddr aRomAddr);
	virtual void ClearPages(TInt aNumPages, TPhysAddr* aPageList, TUint8 aClearByte);
	virtual void Pagify(TInt aId, TLinAddr aLinAddr);
	virtual TPte PtePermissions(TChunkType aChunkType);
	virtual TInt RamDefragFault(TAny* aExceptionInfo);
	virtual void DisablePageModification(DMemModelChunk* aChunk, TInt aOffset);

	// overriding Mmu
	virtual TInt NewPageDirectory(TInt aOsAsid, TBool aSeparateGlobal, TPhysAddr& aPhysAddr, TInt& aNumPages);
	virtual void InitPageDirectory(TInt aOsAsid, TBool aGlobal);
	virtual TInt PageTableId(TLinAddr aAddr, TInt aOsAsid);
	virtual TPhysAddr LinearToPhysical(TLinAddr aAddr, TInt aOsAsid);
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
	virtual TLinAddr MapTemp(TPhysAddr aPage,TLinAddr aLinAddr,TInt aPages=1);
	virtual TLinAddr MapSecondTemp(TPhysAddr aPage,TLinAddr aLinAddr,TInt aPages=1);
	virtual TLinAddr MapTemp(TPhysAddr aPage,TLinAddr aLinAddr,TInt aPages, TMemoryType aMemType);

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
	
public:
	THwChunkAddressAllocator* iUserHwChunkAllocator;	// address allocator for HW chunks in user global section
	TPte iPteGlobal;	// =0x100 on processors which support global pages, 0 on processors which don't
public:
	friend class TScheduler;
	friend class Monitor;
	};

GLREF_D X86Mmu TheMmu;

#define	__DRAIN_WRITE_BUFFER

#endif	// __X86_MEM_H__
