// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\epoc\multiple\arm\xmmu.cpp
// 
//

#include "arm_mem.h"
#include <mmubase.inl>
#include <ramcache.h>
#include <demand_paging.h>
#include "execs.h"
#include <defrag.h>
#include "cache_maintenance.inl"

#undef __MMU_MACHINE_CODED__

// SECTION_PDE(perm, attr, domain, execute, global)
// PT_PDE(domain)
// LP_PTE(perm, attr, execute, global)
// SP_PTE(perm, attr, execute, global)

const TInt KPageColourShift=2;
const TInt KPageColourCount=(1<<KPageColourShift);
const TInt KPageColourMask=KPageColourCount-1;


const TPde KPdPdePerm=PT_PDE(0);
const TPde KPtPdePerm=PT_PDE(0);
const TPde KShadowPdePerm=PT_PDE(0);

#if defined(__CPU_MEMORY_TYPE_REMAPPING)
// ARM1176, ARM11MPCore, ARMv7 and later
// __CPU_MEMORY_TYPE_REMAPPING means that only three bits (TEX0:C:B) in page table define
// memory attributes. Kernel runs with a limited set of memory types: stronlgy ordered,
// device, normal un-cached & and normal WBWA. Due to lack of write through mode, page tables are
// write-back which means that cache has to be cleaned on every page/directory table update.
const TPte KPdPtePerm=				SP_PTE(KArmV6PermRWNO, EMemAttNormalCached, 0, 1);
const TPte KPtPtePerm=				SP_PTE(KArmV6PermRWNO, EMemAttNormalCached, 0, 1);
const TPte KPtInfoPtePerm=			SP_PTE(KArmV6PermRWNO, EMemAttNormalCached, 0, 1);
const TPte KRomPtePerm=				SP_PTE(KArmV6PermRORO, EMemAttNormalCached, 1, 1);
const TPte KShadowPtePerm=			SP_PTE(KArmV6PermRORO, EMemAttNormalCached, 1, 1);
const TPde KRomSectionPermissions=	SECTION_PDE(KArmV6PermRORO, EMemAttNormalCached, 0, 1, 1);
const TPte KUserCodeLoadPte=		SP_PTE(KArmV6PermRWNO, EMemAttNormalCached, 1, 0);
const TPte KUserCodeRunPte=			SP_PTE(KArmV6PermRORO, EMemAttNormalCached, 1, 0);
const TPte KGlobalCodeRunPte=		SP_PTE(KArmV6PermRORO, EMemAttNormalCached, 1, 1);
const TPte KKernelCodeRunPte=		SP_PTE(KArmV6PermRONO, EMemAttNormalCached, 1, 1);

const TInt KNormalUncachedAttr = EMemAttNormalUncached;
const TInt KNormalCachedAttr = EMemAttNormalCached;

#else

//ARM1136 
const TPte KPtInfoPtePerm=SP_PTE(KArmV6PermRWNO, KArmV6MemAttWBWAWBWA, 0, 1);
#if defined (__CPU_WriteThroughDisabled)
const TPte KPdPtePerm=SP_PTE(KArmV6PermRWNO, KArmV6MemAttWBWAWBWA, 0, 1);
const TPte KPtPtePerm=SP_PTE(KArmV6PermRWNO, KArmV6MemAttWBWAWBWA, 0, 1);
const TPte KRomPtePerm=SP_PTE(KArmV6PermRORO, KArmV6MemAttWBWAWBWA, 1, 1);
const TPte KShadowPtePerm=SP_PTE(KArmV6PermRWRO, KArmV6MemAttWBWAWBWA, 1, 1);
const TPde KRomSectionPermissions	=	SECTION_PDE(KArmV6PermRORO, KArmV6MemAttWBWAWBWA, 0, 1, 1);
const TPte KUserCodeLoadPte=SP_PTE(KArmV6PermRWNO, KArmV6MemAttWBWAWBWA, 1, 0);
const TPte KUserCodeRunPte=SP_PTE(KArmV6PermRWRO, KArmV6MemAttWBWAWBWA, 1, 0);
const TPte KGlobalCodeRunPte=SP_PTE(KArmV6PermRWRO, KArmV6MemAttWBWAWBWA, 1, 1);
const TInt KKernelCodeRunPteAttr = KArmV6MemAttWBWAWBWA;
#else
const TPte KPdPtePerm=SP_PTE(KArmV6PermRWNO, KArmV6MemAttWBRAWTRA, 0, 1);
const TPte KPtPtePerm=SP_PTE(KArmV6PermRWNO, KArmV6MemAttWBRAWTRA, 0, 1);
const TPte KRomPtePerm=SP_PTE(KArmV6PermRORO, KArmV6MemAttWTRAWTRA, 1, 1);
const TPte KShadowPtePerm=SP_PTE(KArmV6PermRWRO, KArmV6MemAttWTRAWTRA, 1, 1);
const TPde KRomSectionPermissions	=	SECTION_PDE(KArmV6PermRORO, KArmV6MemAttWTRAWTRA, 0, 1, 1);
const TPte KUserCodeLoadPte=SP_PTE(KArmV6PermRWNO, KArmV6MemAttWTRAWTRA, 1, 0);
const TPte KUserCodeRunPte=SP_PTE(KArmV6PermRWRO, KArmV6MemAttWTRAWTRA, 1, 0);
const TPte KGlobalCodeRunPte=SP_PTE(KArmV6PermRWRO, KArmV6MemAttWTRAWTRA, 1, 1);
const TInt KKernelCodeRunPteAttr = KArmV6MemAttWTRAWTRA;
#endif


#if defined(__CPU_ARM1136_ERRATUM_353494_FIXED)
const TInt KKernelCodeRunPtePerm = KArmV6PermRONO;
#else
const TInt KKernelCodeRunPtePerm = KArmV6PermRORO;
#endif
const TPte KKernelCodeRunPte=SP_PTE(KKernelCodeRunPtePerm, KKernelCodeRunPteAttr, 1, 1);

const TInt KNormalUncachedAttr = KArmV6MemAttNCNC;
const TInt KNormalCachedAttr = KArmV6MemAttWBWAWBWA;

#endif


extern void __FlushBtb();

#if defined(__CPU_ARM1136__) && !defined(__CPU_ARM1136_ERRATUM_353494_FIXED)
extern void remove_and_invalidate_page(TPte* aPte, TLinAddr aAddr, TInt aAsid);
extern void remove_and_invalidate_section(TPde* aPde, TLinAddr aAddr, TInt aAsid);
#endif


LOCAL_D const TPte ChunkPtePermissions[ENumChunkTypes] =
	{
#if defined(__CPU_MEMORY_TYPE_REMAPPING)
// ARM1176, ARM11 mcore, ARMv7 and later
	SP_PTE(KArmV6PermRWNO, EMemAttNormalCached, 0, 1),		// EKernelData
	SP_PTE(KArmV6PermRWNO, EMemAttNormalCached, 0, 1),		// EKernelStack
	SP_PTE(KArmV6PermRWNO, EMemAttNormalCached, 1, 1),		// EKernelCode - loading
	SP_PTE(KArmV6PermRWNO, EMemAttNormalCached, 1, 1),		// EDll (used for global code) - loading
	SP_PTE(KArmV6PermRORO, EMemAttNormalCached, 1, 0),		// EUserCode - run
	SP_PTE(KArmV6PermRWRW, EMemAttNormalCached, 0, 1),		// ERamDrive
	SP_PTE(KArmV6PermRWRW, EMemAttNormalCached, 0, 0),		// EUserData
	SP_PTE(KArmV6PermRWRW, EMemAttNormalCached, 0, 0),		// EDllData
	SP_PTE(KArmV6PermRWRW, EMemAttNormalCached, 1, 0),		// EUserSelfModCode
	SP_PTE(KArmV6PermRWRW, EMemAttNormalCached, 0, 0),		// ESharedKernelSingle
	SP_PTE(KArmV6PermRWRW, EMemAttNormalCached, 0, 0),		// ESharedKernelMultiple
	SP_PTE(KArmV6PermRWRW, EMemAttNormalCached, 0, 0),		// ESharedIo
	SP_PTE(KArmV6PermRWNO, EMemAttNormalCached, 0, 1),		// ESharedKernelMirror
	SP_PTE(KArmV6PermRWNO, EMemAttNormalCached, 0, 1),		// EKernelMessage
#else
	SP_PTE(KArmV6PermRWNO, KArmV6MemAttWBWAWBWA, 0, 1),		// EKernelData
	SP_PTE(KArmV6PermRWNO, KArmV6MemAttWBWAWBWA, 0, 1),		// EKernelStack
#if defined (__CPU_WriteThroughDisabled)
	SP_PTE(KArmV6PermRWNO, KArmV6MemAttWBWAWBWA, 1, 1),		// EKernelCode - loading
	SP_PTE(KArmV6PermRWNO, KArmV6MemAttWBWAWBWA, 1, 1),		// EDll (used for global code) - loading
	SP_PTE(KArmV6PermRWRO, KArmV6MemAttWBWAWBWA, 1, 0),		// EUserCode - run
#else
	SP_PTE(KArmV6PermRWNO, KArmV6MemAttWTRAWTRA, 1, 1),		// EKernelCode - loading
	SP_PTE(KArmV6PermRWNO, KArmV6MemAttWTRAWTRA, 1, 1),		// EDll (used for global code) - loading
	SP_PTE(KArmV6PermRWRO, KArmV6MemAttWTRAWTRA, 1, 0),		// EUserCode - run
#endif
	SP_PTE(KArmV6PermRWRW, KArmV6MemAttWBWAWBWA, 0, 1),		// ERamDrive
	SP_PTE(KArmV6PermRWRW, KArmV6MemAttWBWAWBWA, 0, 0),		// EUserData
	SP_PTE(KArmV6PermRWRW, KArmV6MemAttWBWAWBWA, 0, 0),		// EDllData
	SP_PTE(KArmV6PermRWRW, KArmV6MemAttWBWAWBWA, 1, 0),		// EUserSelfModCode
	SP_PTE(KArmV6PermRWRW, KArmV6MemAttWBWAWBWA, 0, 0),		// ESharedKernelSingle
	SP_PTE(KArmV6PermRWRW, KArmV6MemAttWBWAWBWA, 0, 0),		// ESharedKernelMultiple
	SP_PTE(KArmV6PermRWRW, KArmV6MemAttWBWAWBWA, 0, 0),		// ESharedIo
	SP_PTE(KArmV6PermRWNO, KArmV6MemAttWBWAWBWA, 0, 1),		// ESharedKernelMirror
	SP_PTE(KArmV6PermRWNO, KArmV6MemAttWBWAWBWA, 0, 1),		// EKernelMessage
#endif
	};

// The domain for each chunk is selected according to its type.
// The RamDrive lives in a separate domain, to minimise the risk
// of accidental access and corruption. User chunks may also be
// located in a separate domain (15) in DEBUG builds.
LOCAL_D const TPde ChunkPdePermissions[ENumChunkTypes] =
	{
	PT_PDE(0),						// EKernelData
	PT_PDE(0),						// EKernelStack
	PT_PDE(0),						// EKernelCode
	PT_PDE(0),						// EDll
	PT_PDE(USER_MEMORY_DOMAIN),		// EUserCode
	PT_PDE(1),						// ERamDrive
	PT_PDE(USER_MEMORY_DOMAIN),		// EUserData
	PT_PDE(USER_MEMORY_DOMAIN),		// EDllData
	PT_PDE(USER_MEMORY_DOMAIN),		// EUserSelfModCode
	PT_PDE(USER_MEMORY_DOMAIN),		// ESharedKernelSingle
	PT_PDE(USER_MEMORY_DOMAIN),		// ESharedKernelMultiple
	PT_PDE(0),						// ESharedIo
	PT_PDE(0),						// ESharedKernelMirror
	PT_PDE(0),						// EKernelMessage
	};

// Inline functions for simple transformations
inline TLinAddr PageTableLinAddr(TInt aId)
	{
	return (KPageTableBase+(aId<<KPageTableShift));
	}

inline TPte* PageTable(TInt aId)
	{
	return (TPte*)(KPageTableBase+(aId<<KPageTableShift));
	}

inline TPte* PageTableEntry(TInt aId, TLinAddr aAddress)
	{
	return PageTable(aId) + ((aAddress >> KPageShift) & (KChunkMask >> KPageShift));
	}

inline TLinAddr PageDirectoryLinAddr(TInt aOsAsid)
	{
	return (KPageDirectoryBase+(aOsAsid<<KPageDirectoryShift));
	}

inline TPde* PageDirectoryEntry(TInt aOsAsid, TLinAddr aAddress)
	{
	return PageDirectory(aOsAsid) + (aAddress >> KChunkShift);
	}

extern void InvalidateTLBForPage(TLinAddr /*aLinAddr*/, TInt /*aAsid*/);
extern void FlushTLBs();
extern TUint32 TTCR();

TPte* SafePageTableFromPde(TPde aPde)
	{
	if((aPde&KPdeTypeMask)==KArmV6PdePageTable)
		{
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(aPde);
		if(pi)
			{
			TInt id = (pi->Offset()<<KPtClusterShift) | ((aPde>>KPageTableShift)&KPtClusterMask);
			return PageTable(id);
			}
		}
	return 0;
	}

TPte* SafePtePtrFromLinAddr(TLinAddr aAddress, TInt aOsAsid=0)
	{
	if ((TInt)(aAddress>>KChunkShift)>=(TheMmu.iLocalPdSize>>2))
		aOsAsid = 0;
	TPde pde = PageDirectory(aOsAsid)[aAddress>>KChunkShift];
	TPte* pt = SafePageTableFromPde(pde);
	if(pt)
		pt += (aAddress>>KPageShift)&(KChunkMask>>KPageShift);
	return pt;
	}

#ifndef _DEBUG
// inline in UREL builds...
#ifdef __ARMCC__
	__forceinline /* RVCT ignores normal inline qualifier :-( */
#else
	inline
#endif
#endif
TPte* PtePtrFromLinAddr(TLinAddr aAddress, TInt aOsAsid=0)
	{
	// this function only works for process local memory addresses, or for kernel memory (asid==0).
	__NK_ASSERT_DEBUG(aOsAsid==0 || (TInt)(aAddress>>KChunkShift)<(TheMmu.iLocalPdSize>>2));
	TPde pde = PageDirectory(aOsAsid)[aAddress>>KChunkShift];
	SPageInfo* pi = SPageInfo::FromPhysAddr(pde);
	TInt id = (pi->Offset()<<KPtClusterShift) | ((pde>>KPageTableShift)&KPtClusterMask);
	TPte* pt = PageTable(id);
	pt += (aAddress>>KPageShift)&(KChunkMask>>KPageShift);
	return pt;
	}


TInt ArmMmu::LinearToPhysical(TLinAddr aLinAddr, TInt aSize, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList, TInt aOsAsid)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::LinearToPhysical %08x+%08x, asid=%d",aLinAddr,aSize,aOsAsid));
	TPhysAddr physStart = ArmMmu::LinearToPhysical(aLinAddr,aOsAsid);
	TPhysAddr nextPhys = physStart&~KPageMask;

	TUint32* pageList = aPhysicalPageList;

	TInt pageIndex = aLinAddr>>KPageShift;
	TInt pagesLeft = ((aLinAddr+aSize-1)>>KPageShift)+1 - pageIndex;
	TInt pdeIndex = aLinAddr>>KChunkShift;
	TPde* pdePtr = (pdeIndex<(iLocalPdSize>>2) || (iAsidInfo[aOsAsid]&1))
					? PageDirectory(aOsAsid)
					: ::InitPageDirectory;
	pdePtr += pdeIndex;
	while(pagesLeft)
		{
		pageIndex &= KChunkMask>>KPageShift;
		TInt pagesLeftInChunk = (1<<(KChunkShift-KPageShift))-pageIndex;
		if(pagesLeftInChunk>pagesLeft)
			pagesLeftInChunk = pagesLeft;
		pagesLeft -= pagesLeftInChunk;

		TPhysAddr phys;
		TPde pde = *pdePtr++;
		TUint pdeType = pde&KPdeTypeMask;
		if(pdeType==KArmV6PdeSection)
			{
			phys = (pde & KPdeSectionAddrMask) + (pageIndex*KPageSize);
			__KTRACE_OPT(KMMU2,Kern::Printf("ArmMmu::LinearToPhysical Section phys=%8x",phys));
			TInt n=pagesLeftInChunk;
			phys==nextPhys ? nextPhys+=n*KPageSize : nextPhys=KPhysAddrInvalid;
			if(pageList)
				{
				TUint32* pageEnd = pageList+n;
				do
					{
					*pageList++ = phys;
					phys+=KPageSize;
					}
				while(pageList<pageEnd);
				}
			}
		else
			{
			TPte* pt = SafePageTableFromPde(pde);
			if(!pt)
				{
				__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::LinearToPhysical missing page table: PDE=%8x",pde));
				return KErrNotFound;
				}
			pt += pageIndex;
			for(;;)
				{
				TPte pte = *pt++;
				TUint pte_type = pte & KPteTypeMask;
				if (pte_type >= KArmV6PteSmallPage)
					{
					phys = (pte & KPteSmallPageAddrMask);
					__KTRACE_OPT(KMMU2,Kern::Printf("ArmMmu::LinearToPhysical Small Page phys=%8x",phys));
					phys==nextPhys ? nextPhys+=KPageSize : nextPhys=KPhysAddrInvalid;
					if(pageList)
						*pageList++ = phys;
					if(--pagesLeftInChunk)
						continue;
					break;
					}
				if (pte_type == KArmV6PteLargePage)
					{
					--pt; // back up ptr
					TUint pageOffset = ((TUint)pt>>2)&(KLargeSmallPageRatio-1);
					phys = (pte & KPteLargePageAddrMask) + pageOffset*KPageSize;
					__KTRACE_OPT(KMMU2,Kern::Printf("ArmMmu::LinearToPhysical Large Page phys=%8x",phys));
					TInt n=KLargeSmallPageRatio-pageOffset;
					if(n>pagesLeftInChunk)
						n = pagesLeftInChunk;
					phys==nextPhys ? nextPhys+=n*KPageSize : nextPhys=KPhysAddrInvalid;
					if(pageList)
						{
						TUint32* pageEnd = pageList+n;
						do
							{
							*pageList++ = phys;
							phys+=KPageSize;
							}
						while(pageList<pageEnd);
						}
					pt += n;
					if(pagesLeftInChunk-=n)
						continue;
					break;
					}
				__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::LinearToPhysical bad PTE %8x",pte));
				return KErrNotFound;
				}
			}
		if(!pageList && nextPhys==KPhysAddrInvalid)
			{
			__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::LinearToPhysical not contiguous"));
			return KErrNotFound;
			}
		pageIndex = 0;
		}

	if(nextPhys==KPhysAddrInvalid)
		{
		// Memory is discontiguous...
		aPhysicalAddress = KPhysAddrInvalid;
		return 1;
		}
	else
		{
		// Memory is contiguous...
		aPhysicalAddress = physStart;
		return KErrNone;
		}
	}


TInt ArmMmu::PreparePagesForDMA(TLinAddr aLinAddr, TInt aSize, TInt aOsAsid, TPhysAddr* aPhysicalPageList)
//Returns the list of physical pages belonging to the specified memory space.
//Checks these pages belong to a chunk marked as being trusted. 
//Locks these pages so they can not be moved by e.g. ram defragmentation.
	{
	SPageInfo* pi = NULL;
	DChunk* chunk = NULL;
	TInt err = KErrNone;

	__NK_ASSERT_DEBUG(MM::MaxPagesInOneGo == 32);	// Needs to be a power of 2.
	TUint flashMask = MM::MaxPagesInOneGo - 1;
	
	__KTRACE_OPT(KMMU2,Kern::Printf("ArmMmu::PreparePagesForDMA %08x+%08x, asid=%d",aLinAddr,aSize,aOsAsid));

	TUint32* pageList = aPhysicalPageList;
	TInt pagesInList = 0;				//The number of pages we put in the list so far
	
	TInt pageIndex = (aLinAddr & KChunkMask) >> KPageShift;	// Index of the page within the section
	TInt pagesLeft = ((aLinAddr & KPageMask) + aSize + KPageMask) >> KPageShift;

	TInt pdeIndex = aLinAddr>>KChunkShift;


	MmuBase::Wait(); 	// RamAlloc Mutex for accessing page/directory tables.
	NKern::LockSystem();// SystemlLock for accessing SPageInfo objects.

	// Get the page directory entry that maps aLinAddr.
	// If the address is in the global region check whether this asid maps
	// global pdes (i.e. the LSB of iAsidInfo is set), if not find the pde from 
	// the kernel's initial page directory.
	TPde* pdePtr = (pdeIndex<(iLocalPdSize>>2) || (iAsidInfo[aOsAsid]&1)) ? PageDirectory(aOsAsid) : ::InitPageDirectory;
	pdePtr += pdeIndex;//This points to the first pde 

	while(pagesLeft)
		{
		TInt pagesLeftInChunk = (1<<(KChunkShift-KPageShift))-pageIndex;
		if(pagesLeftInChunk>pagesLeft)
			pagesLeftInChunk = pagesLeft;
		
		pagesLeft -= pagesLeftInChunk;

		TPte* pPte = SafePageTableFromPde(*pdePtr++);
		if(!pPte) 
			{// Cannot get page table. 
			err = KErrNotFound; 
			goto fail; 
			}
		
		pPte += pageIndex;

		for(;pagesLeftInChunk--;)
			{// This pte must be of type ArmV6 small page, the pde type will 
			// have already been checked by SafePageTableFromPde().
			__NK_ASSERT_DEBUG((*pPte & KArmV6PteTypeMask) >= KArmV6PteSmallPage);
			TPhysAddr phys = (*pPte++ & KPteSmallPageAddrMask);
			pi =  SPageInfo::SafeFromPhysAddr(phys);
			if(!pi)	
				{// Invalid address
				err = KErrNotFound; 
				goto fail; 
				}
			
			__KTRACE_OPT(KMMU2,Kern::Printf("PageInfo: PA:%x T:%x S:%x O:%x C:%x",phys, pi->Type(), pi->State(), pi->Owner(), pi->LockCount()));
			if (chunk == NULL)
				{//This is the first page. Check 'trusted' bit.
				if (pi->Type()!= SPageInfo::EChunk)
					{// The first page does not belong to a chunk.
					err = KErrAccessDenied;
					goto fail;
					}

				chunk = (DChunk*)pi->Owner();
				if ((chunk == NULL) || ((chunk->iAttributes & DChunk::ETrustedChunk) == 0))
					{// Not a trusted chunk
					err = KErrAccessDenied;
					goto fail;
					}
				}
			pi->Lock();

			*pageList++ = phys;

			if(!(++pagesInList & flashMask))
				{
				NKern::FlashSystem();
				}
			}
		pageIndex = 0;
		}

	if (pi->Type() != SPageInfo::EChunk)
		{// The last page does not belong to a chunk.
		err = KErrAccessDenied;
		goto fail;
		}

	if (chunk && (chunk != (DChunk*)pi->Owner()))
		{//The first & the last page do not belong to the same chunk.
		err = KErrArgument;
		goto fail;
		}

	NKern::UnlockSystem();
	MmuBase::Signal();
	return KErrNone;

fail:
	__KTRACE_OPT(KMMU2,Kern::Printf("ArmMmu::PreparePagesForDMA failed"));
	NKern::UnlockSystem();
	MmuBase::Signal();
	ReleasePagesFromDMA(aPhysicalPageList, pagesInList);
	return err;
	}


TInt ArmMmu::ReleasePagesFromDMA(TPhysAddr* aPhysicalPageList, TInt aPageCount)
// Unlocks physical pages.
// @param aPhysicalPageList - points to the list of physical pages that should be released.
// @param aPageCount		- the number of physical pages in the list.
	{
	NKern::LockSystem();
	__KTRACE_OPT(KMMU2,Kern::Printf("ArmMmu::ReleasePagesFromDMA count:%d",aPageCount));

	TUint flashMask = MM::MaxPagesInOneGo - 1;
	while (aPageCount--)
		{
		SPageInfo* pi =  SPageInfo::SafeFromPhysAddr(*aPhysicalPageList++);
		if(!pi)
			{
			NKern::UnlockSystem();
			return KErrArgument;
			}
		__KTRACE_OPT(KMMU2,Kern::Printf("PageInfo: T:%x S:%x O:%x C:%x",pi->Type(), pi->State(), pi->Owner(), pi->LockCount()));
		pi->Unlock();

		if(!(aPageCount & flashMask))
			{
			NKern::FlashSystem();
			}
		}
	NKern::UnlockSystem();
	return KErrNone;
	}


TPhysAddr ArmMmu::LinearToPhysical(TLinAddr aLinAddr, TInt aOsAsid)
//
// Find the physical address corresponding to a given linear address in a specified OS
// address space. Call with system locked.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::LinearToPhysical(%08x,%d)",aLinAddr,aOsAsid));
	TInt pdeIndex=aLinAddr>>KChunkShift;
	TPde pde = (pdeIndex<(iLocalPdSize>>2) || (iAsidInfo[aOsAsid]&1)) ? PageDirectory(aOsAsid)[pdeIndex] : ::InitPageDirectory[pdeIndex];
	TPhysAddr pa=KPhysAddrInvalid;
	if ((pde&KPdePresentMask)==KArmV6PdePageTable)
		{
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pde);
		if (pi)
			{
			TInt id = (pi->Offset()<<KPtClusterShift) | ((pde>>KPageTableShift)&KPtClusterMask);
			TPte* pPte=PageTable(id);
			TPte pte=pPte[(aLinAddr&KChunkMask)>>KPageShift];
			if (pte & KArmV6PteSmallPage)
				{
				pa=(pte&KPteSmallPageAddrMask)+(aLinAddr&~KPteSmallPageAddrMask);
				__KTRACE_OPT(KMMU,Kern::Printf("Mapped with small page - returning %08x",pa));
				}
			else if ((pte & KArmV6PteTypeMask) == KArmV6PteLargePage)
				{
				pa=(pte&KPteLargePageAddrMask)+(aLinAddr&~KPteLargePageAddrMask);
				__KTRACE_OPT(KMMU,Kern::Printf("Mapped with large page - returning %08x",pa));
				}
			}
		}
	else if ((pde&KPdePresentMask)==KArmV6PdeSection)
		{
		pa=(pde&KPdeSectionAddrMask)|(aLinAddr&~KPdeSectionAddrMask);
		__KTRACE_OPT(KMMU,Kern::Printf("Mapped with section - returning %08x",pa));
		}
	return pa;
	}

// permission table indexed by XN:APX:AP1:AP0
static const TInt PermissionLookup[16]=
	{													//XN:APX:AP1:AP0
	0,													//0   0   0   0  no access
	EMapAttrWriteSup|EMapAttrReadSup|EMapAttrExecSup,	//0   0   0   1  RW sup			execute
	EMapAttrWriteSup|EMapAttrReadUser|EMapAttrExecUser,	//0   0   1   0  supRW usrR		execute
	EMapAttrWriteUser|EMapAttrReadUser|EMapAttrExecUser,//0   0   1   1  supRW usrRW	execute
	0,													//0   1   0   0  reserved
	EMapAttrReadSup|EMapAttrExecSup,					//0   1   0   1  supR			execute
	EMapAttrReadUser|EMapAttrExecUser,					//0   1   1   0  supR usrR		execute
	0,													//0   1   1   1  reserved
	0,													//1   0   0   0  no access
	EMapAttrWriteSup|EMapAttrReadSup,					//1   0   0   1  RW sup
	EMapAttrWriteSup|EMapAttrReadUser,					//1   0   1   0  supRW usrR
	EMapAttrWriteUser|EMapAttrReadUser,					//1   0   1   1  supRW usrRW
	0,													//1   1   0   0  reserved
	EMapAttrReadSup,									//1   1   0   1  supR
	EMapAttrReadUser,									//1   1   1   0  supR usrR
	EMapAttrReadUser,									//1   1   1   1  supR usrR
	};

TInt ArmMmu::PageTableId(TLinAddr aAddr, TInt aOsAsid)
	{
	TInt id=-1;
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::PageTableId(%08x,%d)",aAddr,aOsAsid));
	TInt pdeIndex=aAddr>>KChunkShift;
	TPde pde = (pdeIndex<(iLocalPdSize>>2) || (iAsidInfo[aOsAsid]&1)) ? PageDirectory(aOsAsid)[pdeIndex] : ::InitPageDirectory[pdeIndex];
	if ((pde&KArmV6PdeTypeMask)==KArmV6PdePageTable)
		{
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pde);
		if (pi)
			id = (pi->Offset()<<KPtClusterShift) | ((pde>>KPageTableShift)&KPtClusterMask);
		}
	__KTRACE_OPT(KMMU,Kern::Printf("ID=%d",id));
	return id;
	}

// Used only during boot for recovery of RAM drive
TInt ArmMmu::BootPageTableId(TLinAddr aAddr, TPhysAddr& aPtPhys)
	{
	TInt id=KErrNotFound;
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu:BootPageTableId(%08x,&)",aAddr));
	TPde* kpd=(TPde*)KPageDirectoryBase;	// kernel page directory
	TInt pdeIndex=aAddr>>KChunkShift;
	TPde pde = kpd[pdeIndex];
	if ((pde & KArmV6PdeTypeMask) == KArmV6PdePageTable)
		{
		aPtPhys = pde & KPdePageTableAddrMask;
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pde);
		if (pi)
			{
			SPageInfo::TType type = pi->Type();
			if (type == SPageInfo::EPageTable)
				id = (pi->Offset()<<KPtClusterShift) | ((pde>>KPageTableShift)&KPtClusterMask);
			else if (type == SPageInfo::EUnused)
				id = KErrUnknown;
			}
		}
	__KTRACE_OPT(KMMU,Kern::Printf("ID=%d",id));
	return id;
	}

TBool ArmMmu::PteIsPresent(TPte aPte)
	{
	return aPte & KArmV6PteTypeMask;
	}

TPhysAddr ArmMmu::PtePhysAddr(TPte aPte, TInt aPteIndex)
	{
	TUint32 pte_type = aPte & KArmV6PteTypeMask;
	if (pte_type == KArmV6PteLargePage)
		return (aPte & KPteLargePageAddrMask) + (TPhysAddr(aPteIndex << KPageShift) & KLargePageMask);
	else if (pte_type != 0)
		return aPte & KPteSmallPageAddrMask;
	return KPhysAddrInvalid;
	}

TPhysAddr ArmMmu::PdePhysAddr(TLinAddr aAddr)
	{
	TPde* kpd = (TPde*)KPageDirectoryBase;	// kernel page directory
	TPde pde = kpd[aAddr>>KChunkShift];
	if ((pde & KPdePresentMask) == KArmV6PdeSection)
		return pde & KPdeSectionAddrMask;
	return KPhysAddrInvalid;
	}

void ArmMmu::Init1()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("ArmMmu::Init1"));

	// MmuBase data
	iPageSize=KPageSize;
	iPageMask=KPageMask;
	iPageShift=KPageShift;
	iChunkSize=KChunkSize;
	iChunkMask=KChunkMask;
	iChunkShift=KChunkShift;
	iPageTableSize=KPageTableSize;
	iPageTableMask=KPageTableMask;
	iPageTableShift=KPageTableShift;
	iPtClusterSize=KPtClusterSize;
	iPtClusterMask=KPtClusterMask;
	iPtClusterShift=KPtClusterShift;
	iPtBlockSize=KPtBlockSize;
	iPtBlockMask=KPtBlockMask;
	iPtBlockShift=KPtBlockShift;
	iPtGroupSize=KChunkSize/KPageTableSize;
	iPtGroupMask=iPtGroupSize-1;
	iPtGroupShift=iChunkShift-iPageTableShift;
	//TInt* iPtBlockCount;		// dynamically allocated - Init2
	//TInt* iPtGroupCount;		// dynamically allocated - Init2
	iPtInfo=(SPageTableInfo*)KPageTableInfoBase;
	iPageTableLinBase=KPageTableBase;
	//iRamPageAllocator;		// dynamically allocated - Init2
	//iAsyncFreeList;			// dynamically allocated - Init2
	//iPageTableAllocator;		// dynamically allocated - Init2
	//iPageTableLinearAllocator;// dynamically allocated - Init2
	iPtInfoPtePerm=KPtInfoPtePerm;
	iPtPtePerm=KPtPtePerm;
	iPtPdePerm=KPtPdePerm;
	iUserCodeLoadPtePerm=KUserCodeLoadPte;
	iKernelCodePtePerm=KKernelCodeRunPte;
	iTempAddr=KTempAddr;
	iSecondTempAddr=KSecondTempAddr;
	iMapSizes=KPageSize|KLargePageSize|KChunkSize;
	iRomLinearBase = ::RomHeaderAddress;
	iRomLinearEnd = KRomLinearEnd;
	iShadowPtePerm = KShadowPtePerm;
	iShadowPdePerm = KShadowPdePerm;

	// Mmu data
	TInt total_ram=TheSuperPage().iTotalRamSize;

	// Large or small configuration?
	// This is determined by the bootstrap based on RAM size
	TUint32 ttcr=TTCR();
	__NK_ASSERT_ALWAYS(ttcr==1 || ttcr==2);
	TBool large = (ttcr==1);

	// calculate cache colouring...
	TInt iColourCount = 0;
	TInt dColourCount = 0;
	TUint32 ctr = InternalCache::TypeRegister();
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("CacheTypeRegister = %08x",ctr));
#ifdef __CPU_ARMV6
	__NK_ASSERT_ALWAYS((ctr>>29)==0);	// check ARMv6 format
	if(ctr&0x800)
		iColourCount = 4;
	if(ctr&0x800000)
		dColourCount = 4;
#else
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("CacheTypeRegister = %08x",ctr));
	__NK_ASSERT_ALWAYS((ctr>>29)==4);	// check ARMv7 format
	TUint l1ip = (ctr>>14)&3;			// L1 instruction cache indexing and tagging policy
	__NK_ASSERT_ALWAYS(l1ip>=2);		// check I cache is physically tagged

	TUint32 clidr = InternalCache::LevelIDRegister();
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("CacheLevelIDRegister = %08x",clidr));
	TUint l1type = clidr&7;
	if(l1type)
		{
		if(l1type==2 || l1type==3 || l1type==4)
			{
			// we have an L1 data cache...
			TUint32 csir = InternalCache::SizeIdRegister(0,0);
			TUint sets = ((csir>>13)&0x7fff)+1;
			TUint ways = ((csir>>3)&0x3ff);
			ways+=1;
			TUint lineSizeShift = (csir&7)+4;
			// assume L1 data cache is VIPT and alias checks broken and so we need data cache colouring...
			dColourCount = (sets<<lineSizeShift)>>KPageShift;
			if(l1type==4) // unified cache, so set instruction cache colour as well...
				iColourCount = (sets<<lineSizeShift)>>KPageShift;
			__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("L1DCache = 0x%x,0x%x,%d colourCount=%d",sets,ways,lineSizeShift,(sets<<lineSizeShift)>>KPageShift));
			}

		if(l1type==1 || l1type==3)
			{
			// we have a separate L1 instruction cache...
			TUint32 csir = InternalCache::SizeIdRegister(1,0);
			TUint sets = ((csir>>13)&0x7fff)+1;
			TUint ways = ((csir>>3)&0x3ff);
			ways+=1;
			TUint lineSizeShift = (csir&7)+4;
			iColourCount = (sets<<lineSizeShift)>>KPageShift;
			__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("L1ICache = 0x%x,0x%x,%d colourCount=%d",sets,ways,lineSizeShift,(sets<<lineSizeShift)>>KPageShift));
			}
		}
	if(l1ip==3)
		{
		// PIPT cache, so no colouring restrictions...
		__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("L1ICache is PIPT"));
		iColourCount = 0;
		}
	else
		{
		// VIPT cache...
		__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("L1ICache is VIPT"));
		}
#endif
	TUint colourShift = 0;
	for(TUint colourCount=Max(iColourCount,dColourCount); colourCount!=0; colourCount>>=1)
		++colourShift;
	iAliasSize=KPageSize<<colourShift;
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("iAliasSize=0x%x",iAliasSize));
	iAliasMask=iAliasSize-1;
	iAliasShift=KPageShift+colourShift;

	iDecommitThreshold = CacheMaintenance::SyncAllPerformanceThresholdPages();

	iNumOsAsids=KArmV6NumAsids;
	iNumGlobalPageDirs=1;
	//iOsAsidAllocator;			// dynamically allocated - Init2
	iGlobalPdSize=KPageDirectorySize;
	iGlobalPdShift=KPageDirectoryShift;
	iAsidGroupSize=KChunkSize/KPageDirectorySize;
	iAsidGroupMask=iAsidGroupSize-1;
	iAsidGroupShift=KChunkShift-KPageDirectoryShift;
	iUserLocalBase=KUserLocalDataBase;
	iAsidInfo=(TUint32*)KAsidInfoBase;
	iPdeBase=KPageDirectoryBase;
	iPdPtePerm=KPdPtePerm;
	iPdPdePerm=KPdPdePerm;
	iRamDriveMask=0x00f00000;
	iGlobalCodePtePerm=KGlobalCodeRunPte;
#if defined(__CPU_MEMORY_TYPE_REMAPPING)
	iCacheMaintenanceTempMapAttr = CacheMaintenance::TemporaryMapping();
#else
	switch(CacheMaintenance::TemporaryMapping())
		{
		case EMemAttNormalUncached:
			iCacheMaintenanceTempMapAttr = KArmV6MemAttNCNC;
			break;
		case EMemAttNormalCached:
			iCacheMaintenanceTempMapAttr = KArmV6MemAttWBWAWBWA;
			break;
		default:
			Panic(ETempMappingFailed);
		}
#endif	
	iMaxDllDataSize=Min(total_ram/2, 0x08000000);				// phys RAM/2 up to 128Mb
	iMaxDllDataSize=(iMaxDllDataSize+iChunkMask)&~iChunkMask;	// round up to chunk size
	iMaxUserCodeSize=Min(total_ram, 0x10000000);				// phys RAM up to 256Mb
	iMaxUserCodeSize=(iMaxUserCodeSize+iChunkMask)&~iChunkMask;	// round up to chunk size
	if (large)
		{
		iLocalPdSize=KPageDirectorySize/2;
		iLocalPdShift=KPageDirectoryShift-1;
		iUserSharedBase=KUserSharedDataBase2GB;
		iUserLocalEnd=iUserSharedBase-iMaxDllDataSize;
		iUserSharedEnd=KUserSharedDataEnd2GB-iMaxUserCodeSize;
		iDllDataBase=iUserLocalEnd;
		iUserCodeBase=iUserSharedEnd;
		}
	else
		{
		iLocalPdSize=KPageDirectorySize/4;
		iLocalPdShift=KPageDirectoryShift-2;
		iUserSharedBase=KUserSharedDataBase1GB;
		iUserLocalEnd=iUserSharedBase;
		iDllDataBase=KUserSharedDataEnd1GB-iMaxDllDataSize;
		iUserCodeBase=iDllDataBase-iMaxUserCodeSize;
		iUserSharedEnd=iUserCodeBase;
		}
	__KTRACE_OPT(KMMU,Kern::Printf("LPD size %08x GPD size %08x Alias size %08x",
													iLocalPdSize, iGlobalPdSize, iAliasSize));
	__KTRACE_OPT(KMMU,Kern::Printf("ULB %08x ULE %08x USB %08x USE %08x",iUserLocalBase,iUserLocalEnd,
																			iUserSharedBase,iUserSharedEnd));
	__KTRACE_OPT(KMMU,Kern::Printf("DDB %08x UCB %08x",iDllDataBase,iUserCodeBase));

	// ArmMmu data

	// other
	PP::MaxUserThreadStack=0x14000;			// 80K - STDLIB asks for 64K for PosixServer!!!!
	PP::UserThreadStackGuard=0x2000;		// 8K
	PP::MaxStackSpacePerProcess=0x200000;	// 2Mb
	K::SupervisorThreadStackSize=0x1000;	// 4K
	PP::SupervisorThreadStackGuard=0x1000;	// 4K
	K::MachineConfig=(TMachineConfig*)KMachineConfigLinAddr;
	PP::RamDriveStartAddress=KRamDriveStartAddress;
	PP::RamDriveRange=KRamDriveMaxSize;
	PP::RamDriveMaxSize=KRamDriveMaxSize;	// may be reduced later
	K::MemModelAttributes=EMemModelTypeMultiple|EMemModelAttrNonExProt|EMemModelAttrKernProt|EMemModelAttrWriteProt|
						EMemModelAttrVA|EMemModelAttrProcessProt|EMemModelAttrSameVA|EMemModelAttrSvKernProt|
						EMemModelAttrIPCKernProt|EMemModelAttrRamCodeProt;

	Arm::DefaultDomainAccess=KDefaultDomainAccess;

	Mmu::Init1();
	}

void ArmMmu::DoInit2()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("ArmMmu::DoInit2"));
	iTempPte=PageTable(PageTableId(iTempAddr,0))+((iTempAddr&KChunkMask)>>KPageShift);
	iSecondTempPte=PageTable(PageTableId(iSecondTempAddr,0))+((iSecondTempAddr&KChunkMask)>>KPageShift);
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("iTempAddr=%08x, iTempPte=%08x, iSecondTempAddr=%08x, iSecondTempPte=%08x",
			iTempAddr, iTempPte, iSecondTempAddr, iSecondTempPte));
	CreateKernelSection(KKernelSectionEnd, iAliasShift);
	CreateUserGlobalSection(KUserGlobalDataBase, KUserGlobalDataEnd);
	Mmu::DoInit2();
	}

#ifndef __MMU_MACHINE_CODED__
void ArmMmu::MapRamPages(TInt aId, SPageInfo::TType aType, TAny* aPtr, TUint32 aOffset, const TPhysAddr* aPageList, TInt aNumPages, TPte aPtePerm)
//
// Map a list of physical RAM pages into a specified page table with specified PTE permissions.
// Update the page information array.
// Call this with the system locked.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::MapRamPages() id=%d type=%d ptr=%08x off=%08x n=%d perm=%08x",
			aId, aType, aPtr, aOffset, aNumPages, aPtePerm));

	SPageTableInfo& ptinfo=iPtInfo[aId];
	ptinfo.iCount+=aNumPages;
	aOffset>>=KPageShift;
	TInt ptOffset=aOffset & KPagesInPDEMask;				// entry number in page table
	TPte* pPte=PageTable(aId)+ptOffset;						// address of first PTE

	TLinAddr firstPte = (TLinAddr)pPte; //Will need this to clean page table changes in cache.

	while(aNumPages--)
		{
		TPhysAddr pa = *aPageList++;
		if(pa==KPhysAddrInvalid)
			{
			++pPte;
			__NK_ASSERT_DEBUG(aType==SPageInfo::EInvalid);
			continue;
			}
		*pPte++ =  pa | aPtePerm;					// insert PTE
		__KTRACE_OPT(KMMU,Kern::Printf("Writing PTE %08x to %08x",pPte[-1],pPte-1));
		if (aType!=SPageInfo::EInvalid)
			{
			SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pa);
			if(pi)
				{
				pi->Set(aType,aPtr,aOffset);
				__KTRACE_OPT(KMMU,Kern::Printf("I: %d %08x %08x",aType,aPtr,aOffset));
				++aOffset;	// increment offset for next page
				}
			}
		}
	CacheMaintenance::MultiplePtesUpdated(firstPte, (TUint)pPte-firstPte);
	}

void ArmMmu::MapPhysicalPages(TInt aId, SPageInfo::TType aType, TAny* aPtr, TUint32 aOffset, TPhysAddr aPhysAddr, TInt aNumPages, TPte aPtePerm)
//
// Map consecutive physical pages into a specified page table with specified PTE permissions.
// Update the page information array if RAM pages are being mapped.
// Call this with the system locked.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::MapPhysicalPages() id=%d type=%d ptr=%08x off=%08x phys=%08x n=%d perm=%08x",
			aId, aType, aPtr, aOffset, aPhysAddr, aNumPages, aPtePerm));
	SPageTableInfo& ptinfo=iPtInfo[aId];
	ptinfo.iCount+=aNumPages;
	aOffset>>=KPageShift;
	TInt ptOffset=aOffset & KPagesInPDEMask;				// entry number in page table
	TPte* pPte=(TPte*)(PageTableLinAddr(aId))+ptOffset;		// address of first PTE

	TLinAddr firstPte = (TLinAddr)pPte; //Will need this to clean page table changes in cache

	SPageInfo* pi;
	if(aType==SPageInfo::EInvalid)
		pi = NULL;
	else
		pi = SPageInfo::SafeFromPhysAddr(aPhysAddr);
	while(aNumPages--)
		{
		*pPte++ = aPhysAddr|aPtePerm;						// insert PTE
		aPhysAddr+=KPageSize;
		__KTRACE_OPT(KMMU,Kern::Printf("Writing PTE %08x to %08x",pPte[-1],pPte-1));
		if (pi)
			{
			pi->Set(aType,aPtr,aOffset);
			__KTRACE_OPT(KMMU,Kern::Printf("I: %d %08x %08x",aType,aPtr,aOffset));
			++aOffset;	// increment offset for next page
			++pi;
			}
		}

	CacheMaintenance::MultiplePtesUpdated(firstPte, (TUint)pPte-firstPte);
	}

void ArmMmu::MapVirtual(TInt aId, TInt aNumPages)
//
// Called in place of MapRamPages or MapPhysicalPages to update mmu data structures when committing
// virtual address space to a chunk.  No pages are mapped.
// Call this with the system locked.
//
	{
	SPageTableInfo& ptinfo=iPtInfo[aId];
	ptinfo.iCount+=aNumPages;
	}

void ArmMmu::RemapPage(TInt aId, TUint32 aAddr, TPhysAddr aOldAddr, TPhysAddr aNewAddr, TPte aPtePerm, DProcess* aProcess)
//
// Replace the mapping at address aAddr in page table aId.
// Update the page information array for both the old and new pages.
// Return physical address of old page if it is now ready to be freed.
// Call this with the system locked.
// May be called with interrupts disabled, do not enable/disable them.
//
	{
	TInt ptOffset=(aAddr&KChunkMask)>>KPageShift;			// entry number in page table
	TPte* pPte=PageTable(aId)+ptOffset;						// address of PTE
	TPte pte=*pPte;
	TInt asid=aProcess ? ((DMemModelProcess*)aProcess)->iOsAsid :
						 (aAddr<KRomLinearBase ? (TInt)UNKNOWN_MAPPING : (TInt)KERNEL_MAPPING );
	
	if (pte & KArmV6PteSmallPage)
		{
		__ASSERT_ALWAYS((pte & KPteSmallPageAddrMask) == aOldAddr, Panic(ERemapPageFailed));
		SPageInfo* oldpi = SPageInfo::FromPhysAddr(aOldAddr);
		__ASSERT_DEBUG(oldpi->LockCount()==0,Panic(ERemapPageFailed));

		// remap page
		*pPte = aNewAddr | aPtePerm;					// overwrite PTE
		CacheMaintenance::SinglePteUpdated((TLinAddr)pPte);
		InvalidateTLBForPage(aAddr,asid);	// flush TLB entry
		
		// update new pageinfo, clear old
		SPageInfo* pi = SPageInfo::FromPhysAddr(aNewAddr);
		pi->Set(oldpi->Type(),oldpi->Owner(),oldpi->Offset());
		oldpi->SetUnused();
		}
	else
		{
		Panic(ERemapPageFailed);
		}
	}

void ArmMmu::RemapPageByAsid(TBitMapAllocator* aOsAsids, TLinAddr aLinAddr, TPhysAddr aOldAddr, TPhysAddr aNewAddr, TPte aPtePerm)
//
// Replace the mapping at address aLinAddr in the relevant page table for all
// ASIDs specified in aOsAsids, but only if the currently mapped address is
// aOldAddr.
// Update the page information array for both the old and new pages.
// Call this with the system unlocked.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::RemapPageByAsid() linaddr=%08x oldaddr=%08x newaddr=%08x perm=%08x", aLinAddr, aOldAddr, aNewAddr, aPtePerm));

	TInt asid = -1;
	TInt lastAsid = KArmV6NumAsids - 1;
	TUint32* ptr = aOsAsids->iMap;
	NKern::LockSystem();
	do
		{
		TUint32 bits = *ptr++;
		do
			{
			++asid;
			if(bits & 0x80000000u)
				{
				// mapped in this address space, so update PTE...
				TPte* pPte = PtePtrFromLinAddr(aLinAddr, asid);
				TPte pte = *pPte;
				if ((pte&~KPageMask) == aOldAddr)
					{
					*pPte = aNewAddr | aPtePerm;
					__KTRACE_OPT(KMMU,Kern::Printf("Writing PTE %08x to %08x in asid %d",*pPte,pPte,asid));
					CacheMaintenance::SinglePteUpdated((TLinAddr)pPte);
					InvalidateTLBForPage(aLinAddr,asid);	// flush TLB entry
					}
				}
			}
		while(bits<<=1);
		NKern::FlashSystem();
		asid |= 31;
		}
	while(asid<lastAsid);

	// copy pageinfo attributes and mark old page unused
	SPageInfo* oldpi = SPageInfo::FromPhysAddr(aOldAddr);
	SPageInfo::FromPhysAddr(aNewAddr)->Set(oldpi->Type(),oldpi->Owner(),oldpi->Offset());
	oldpi->SetUnused();

	NKern::UnlockSystem();
	}

TInt ArmMmu::UnmapPages(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TBool aSetPagesFree, TInt& aNumPtes, TInt& aNumFree, DProcess* aProcess)
//
// Unmap a specified area at address aAddr in page table aId. Place physical addresses of unmapped
// pages into aPageList, and count of unmapped pages into aNumPtes.
// Return number of pages still mapped using this page table.
// Call this with the system locked.
// On multiple memory model, do not call this method with aSetPagesFree false. Call UnmapUnownedPages instead.
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::UnmapPages() id=%d addr=%08x n=%d pl=%08x set-free=%d",aId,aAddr,aNumPages,aPageList,aSetPagesFree));
	TInt ptOffset=(aAddr&KChunkMask)>>KPageShift;			// entry number in page table
	TPte* pPte=PageTable(aId)+ptOffset;						// address of first PTE
	TInt np=0;
	TInt nf=0;
	TUint32 ng=0;
	TInt asid=aProcess ? ((DMemModelProcess*)aProcess)->iOsAsid :
	                     (aAddr<KRomLinearBase ? (TInt)UNKNOWN_MAPPING : (TInt)KERNEL_MAPPING );

	
	while(aNumPages--)
		{
		TPte pte=*pPte;						// get original PTE
#if defined(__CPU_ARM1136__) && !defined(__CPU_ARM1136_ERRATUM_353494_FIXED)
		remove_and_invalidate_page(pPte, aAddr, asid);
		++pPte;
#else
		*pPte++=0;							// clear PTE
#endif
		
		// We count all unmapped pages in np, including demand paged 'old' pages - but we don't pass
		// these to PageUnmapped, as the page doesn't become free until it's unmapped from all
		// processes		
		if (pte != KPteNotPresentEntry)
			++np;
		
		if (pte & KArmV6PteSmallPage)
			{
			ng |= pte;
#if !defined(__CPU_ARM1136__) || defined(__CPU_ARM1136_ERRATUM_353494_FIXED)
			// Remove_and_invalidate_page will sort out cache and TLB. 
			// When __CPU_ARM1136_ERRATUM_353494_FIXED, we have to do it here.
			CacheMaintenance::SinglePteUpdated((TLinAddr)(pPte-1));
			if (asid >= 0) //otherwise, KUnmapPagesTLBFlushDeferred will be returned.
				InvalidateTLBForPage(aAddr,asid);	// flush any corresponding TLB entry
#endif
			TPhysAddr pa=pte & KPteSmallPageAddrMask;	// physical address of unmapped page
			if (aSetPagesFree)
				{
				SPageInfo* pi = SPageInfo::FromPhysAddr(pa);
				if(iRamCache->PageUnmapped(pi))
					{
					pi->SetUnused();					// mark page as unused
					if (pi->LockCount()==0)
						{
						*aPageList++=pa;			// store in page list
						++nf;						// count free pages
						}
					}
				}
			else
				*aPageList++=pa;				// store in page list
			}
		aAddr+=KPageSize;
		}

	aNumPtes=np;
	aNumFree=nf;
	SPageTableInfo& ptinfo=iPtInfo[aId];
	TInt r=(ptinfo.iCount-=np);
	if (asid<0)
		r|=KUnmapPagesTLBFlushDeferred;

	
	#if defined(__CPU_ARM1136__) && !defined(__CPU_ARM1136_ERRATUM_353494_FIXED)
	__FlushBtb();
	#endif

	__KTRACE_OPT(KMMU,Kern::Printf("Unmapped %d; Freed: %d; Return %08x",np,nf,r));
	return r;								// return number of pages remaining in this page table
	}

TInt ArmMmu::UnmapVirtual(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TBool aSetPagesFree, TInt& aNumPtes, TInt& aNumFree, DProcess* aProcess)
//
// Unmap a specified area at address aAddr in page table aId. Place physical addresses of unmapped
// pages into aPageList, and count of unmapped pages into aNumPtes.
// Adjust the page table reference count as if aNumPages pages were unmapped.
// Return number of pages still mapped using this page table.
// Call this with the system locked.
// On multiple memory model, do not call this method with aSetPagesFree false. Call UnmapUnownedVirtual instead.
//
	{
	SPageTableInfo& ptinfo=iPtInfo[aId];
	TInt newCount = ptinfo.iCount - aNumPages;
	UnmapPages(aId, aAddr, aNumPages, aPageList, aSetPagesFree, aNumPtes, aNumFree, aProcess);
	ptinfo.iCount = newCount;
	aNumPtes = aNumPages;
	return newCount;
	}

TInt ArmMmu::UnmapUnownedPages(TInt aId, TUint32 aAddr, TInt aNumPages,
		TPhysAddr* aPageList, TLinAddr* aLAPageList,TInt& aNumPtes, TInt& aNumFree, DProcess* aProcess)
/*
 * Unmaps specified area at address aAddr in page table aId.
 * Places physical addresses of not-demaned-paged unmapped pages into aPageList.
 * Corresponding linear addresses are placed into aLAPageList.
 * 'Old' demand-paged pages (holds invalid PE entry with physucal address) are neither unmapped nor
 * encountered in aPageList but are still counted in aNumPtes.
 * 
 * This method should be called to decommit physical memory not owned by the chunk. As we do not know
 * the origin of such memory, PtInfo could be invalid (or does't exist) so cache maintenance may not be
 * able to obtain mapping colour. For that reason, this also returns former linear address of each page 
 * in aPageList.   
 *   
 * @pre All pages are mapped within a single page table identified by aId.
 * @pre On entry, system locked is held and is not released during the execution.
 *
 * @arg aId             Id of the page table that maps tha pages.
 * @arg aAddr           Linear address of the start of the area.
 * @arg aNumPages       The number of pages to unmap.
 * @arg aProcess        The owning process of the mamory area to unmap.
 * @arg aPageList       On  exit, holds the list of unmapped pages.
 * @arg aLAPageList     On  exit, holds the list of linear addresses of unmapped pages.
 * @arg aNumFree        On exit, holds the number of pages in aPageList.
 * @arg aNumPtes        On exit, holds the number of unmapped pages. This includes demand-paged 'old'
 *                      pages (with invalid page table entry still holding the address of physical page.)
 *                      
 * @return              The number of pages still mapped using this page table. It is orred by
 *                      KUnmapPagesTLBFlushDeferred if TLB flush is not executed - which requires 
 *                      the caller to do global TLB flush.
 */ 
    {
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::UnmapUnownedPages() id=%d addr=%08x n=%d pl=%08x",aId,aAddr,aNumPages,aPageList));
	TInt ptOffset=(aAddr&KChunkMask)>>KPageShift;			// entry number in page table
	TPte* pPte=PageTable(aId)+ptOffset;						// address of first PTE
	TInt np=0;
	TInt nf=0;
	TUint32 ng=0;
	TInt asid=aProcess ? ((DMemModelProcess*)aProcess)->iOsAsid :
	                     (aAddr<KRomLinearBase ? (TInt)UNKNOWN_MAPPING : (TInt)KERNEL_MAPPING );

	while(aNumPages--)
		{
		TPte pte=*pPte;						// get original PTE
#if defined(__CPU_ARM1136__) && !defined(__CPU_ARM1136_ERRATUM_353494_FIXED)
		remove_and_invalidate_page(pPte, aAddr, asid);
		++pPte;
#else
		*pPte++=0;							// clear PTE
#endif
		
		// We count all unmapped pages in np, including demand paged 'old' pages - but we don't pass
		// these to PageUnmapped, as the page doesn't become free until it's unmapped from all
		// processes		
		if (pte != KPteNotPresentEntry)
			++np;
		
		if (pte & KArmV6PteSmallPage)
			{
			ng |= pte;
#if !defined(__CPU_ARM1136__) || defined(__CPU_ARM1136_ERRATUM_353494_FIXED)
			// Remove_and_invalidate_page will sort out cache and TLB. 
			// When __CPU_ARM1136_ERRATUM_353494_FIXED, we have to do it here.
			CacheMaintenance::SinglePteUpdated((TLinAddr)(pPte-1));
			if (asid >= 0) //otherwise, KUnmapPagesTLBFlushDeferred will be returned.
				InvalidateTLBForPage(aAddr,asid);	// flush any corresponding TLB entry
#endif
			TPhysAddr pa=pte & KPteSmallPageAddrMask;	// physical address of unmapped page
	        ++nf;
	        *aPageList++=pa;				// store physical aaddress in page list
	        *aLAPageList++=aAddr;			// store linear address in page list
			}
		aAddr+=KPageSize;
		}

	aNumPtes=np;
	aNumFree=nf;
	SPageTableInfo& ptinfo=iPtInfo[aId];
	TInt r=(ptinfo.iCount-=np);
	if (asid<0)
		r|=KUnmapPagesTLBFlushDeferred;

	
	#if defined(__CPU_ARM1136__) && !defined(__CPU_ARM1136_ERRATUM_353494_FIXED)
	__FlushBtb();
	#endif

	__KTRACE_OPT(KMMU,Kern::Printf("Unmapped %d; Freed: %d; Return %08x",np,nf,r));
	return r;								// return number of pages remaining in this page table
	}


TInt ArmMmu::UnmapUnownedVirtual(TInt aId, TUint32 aAddr, TInt aNumPages,
		TPhysAddr* aPageList, TLinAddr* aLAPageList,TInt& aNumPtes, TInt& aNumFree, DProcess* aProcess)
//
// Unmap a specified area at address aAddr in page table aId. Place physical addresses of unmapped
// pages into aPageList, and count of unmapped pages into aNumPtes.
// Adjust the page table reference count as if aNumPages pages were unmapped.
// Return number of pages still mapped using this page table.
// Call this with the system locked.
//
	{
	SPageTableInfo& ptinfo=iPtInfo[aId];
	TInt newCount = ptinfo.iCount - aNumPages;
	UnmapUnownedPages(aId, aAddr, aNumPages, aPageList,  aLAPageList, aNumPtes,  aNumFree,  aProcess);
	ptinfo.iCount = newCount;
	aNumPtes = aNumPages;	
	return newCount;
	}

void ArmMmu::DoAssignPageTable(TInt aId, TLinAddr aAddr, TPde aPdePerm, const TAny* aOsAsids)
//
// Assign an allocated page table to map a given linear address with specified permissions.
// This should be called with the system unlocked and the MMU mutex held.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::DoAssignPageTable %d to %08x perm %08x asid %08x",aId,aAddr,aPdePerm,aOsAsids));
	TLinAddr ptLin=PageTableLinAddr(aId);
	TPhysAddr ptPhys=LinearToPhysical(ptLin,0);
	TInt pdeIndex=TInt(aAddr>>KChunkShift);
	TBool gpd=(pdeIndex>=(iLocalPdSize>>2));
	TInt os_asid=(TInt)aOsAsids;
	if (TUint32(os_asid)<TUint32(iNumOsAsids))
		{
		// single OS ASID
		TPde* pageDir=PageDirectory(os_asid);
		NKern::LockSystem();
		pageDir[pdeIndex]=ptPhys|aPdePerm;	// will blow up here if address is in global region aOsAsid doesn't have a global PD
		CacheMaintenance::SinglePteUpdated((TLinAddr)(pageDir+pdeIndex));
		NKern::UnlockSystem();
				
		__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x",ptPhys|aPdePerm,pageDir+pdeIndex));
		}
	else if (os_asid==-1 && gpd)
		{
		// all OS ASIDs, address in global region
		TInt num_os_asids=iNumGlobalPageDirs;
		const TBitMapAllocator& b=*(const TBitMapAllocator*)iOsAsidAllocator;
		for (os_asid=0; num_os_asids; ++os_asid)
			{
			if (!b.NotAllocated(os_asid,1) && (iAsidInfo[os_asid]&1))
				{
				// this OS ASID exists and has a global page directory
				TPde* pageDir=PageDirectory(os_asid);
				NKern::LockSystem();
				pageDir[pdeIndex]=ptPhys|aPdePerm;
				CacheMaintenance::SinglePteUpdated((TLinAddr)(pageDir+pdeIndex));
				NKern::UnlockSystem();

				__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x",ptPhys|aPdePerm,pageDir+pdeIndex));
				--num_os_asids;
				}
			}
		}
	else
		{
		// selection of OS ASIDs or all OS ASIDs
		const TBitMapAllocator* pB=(const TBitMapAllocator*)aOsAsids;
		if (os_asid==-1)
			pB=iOsAsidAllocator;	// 0's in positions which exist
		TInt num_os_asids=pB->iSize-pB->iAvail;
		for (os_asid=0; num_os_asids; ++os_asid)
			{
			if (pB->NotAllocated(os_asid,1))
				continue;			// os_asid is not needed
			TPde* pageDir=PageDirectory(os_asid);
			NKern::LockSystem();
			pageDir[pdeIndex]=ptPhys|aPdePerm;
			CacheMaintenance::SinglePteUpdated((TLinAddr)(pageDir+pdeIndex));
			NKern::UnlockSystem();

			__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x",ptPhys|aPdePerm,pageDir+pdeIndex));
			--num_os_asids;
			}
		}
	}

void ArmMmu::RemapPageTableSingle(TPhysAddr aOld, TPhysAddr aNew, TLinAddr aAddr, TInt aOsAsid)
//
// Replace a single page table mapping the specified linear address.
// This should be called with the system locked and the MMU mutex held.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::RemapPageTableSingle %08x to %08x at %08x asid %d",aOld,aNew,aAddr,aOsAsid));
	TPde* pageDir=PageDirectory(aOsAsid);
	TInt pdeIndex=TInt(aAddr>>KChunkShift);
	TPde pde=pageDir[pdeIndex];
	__ASSERT_ALWAYS((pde & KPdePageTableAddrMask) == aOld, Panic(ERemapPageTableFailed));
	TPde newPde=aNew|(pde&~KPdePageTableAddrMask);
	pageDir[pdeIndex]=newPde;	// will blow up here if address is in global region aOsAsid doesn't have a global PD
	CacheMaintenance::SinglePteUpdated((TLinAddr)(pageDir+pdeIndex));
				
	__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x",newPde,pageDir+pdeIndex));
	}

void ArmMmu::RemapPageTableGlobal(TPhysAddr aOld, TPhysAddr aNew, TLinAddr aAddr)
//
// Replace a global page table mapping the specified linear address.
// This should be called with the system locked and the MMU mutex held.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::RemapPageTableGlobal %08x to %08x at %08x",aOld,aNew,aAddr));
	TInt pdeIndex=TInt(aAddr>>KChunkShift);
	TInt num_os_asids=iNumGlobalPageDirs;
	const TBitMapAllocator& b=*(const TBitMapAllocator*)iOsAsidAllocator;
	for (TInt os_asid=0; num_os_asids; ++os_asid)
		{
		if (!b.NotAllocated(os_asid,1) && (iAsidInfo[os_asid]&1))
			{
			// this OS ASID exists and has a global page directory
			TPde* pageDir=PageDirectory(os_asid);
			TPde pde=pageDir[pdeIndex];
			if ((pde & KPdePageTableAddrMask) == aOld)
				{
				TPde newPde=aNew|(pde&~KPdePageTableAddrMask);
				pageDir[pdeIndex]=newPde;
				CacheMaintenance::SinglePteUpdated((TLinAddr)(pageDir+pdeIndex));

				__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x",newPde,pageDir+pdeIndex));
				}
			--num_os_asids;
			}
		if ((os_asid&31)==31)
			NKern::FlashSystem();
		}
	}

void ArmMmu::RemapPageTableMultiple(TPhysAddr aOld, TPhysAddr aNew, TLinAddr aAddr, const TAny* aOsAsids)
//
// Replace multiple page table mappings of the specified linear address.
// This should be called with the system locked and the MMU mutex held.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::RemapPageTableMultiple %08x to %08x at %08x asids %08x",aOld,aNew,aAddr,aOsAsids));
	TInt pdeIndex=TInt(aAddr>>KChunkShift);
	const TBitMapAllocator* pB=(const TBitMapAllocator*)aOsAsids;
	if ((TInt)aOsAsids==-1)
		pB=iOsAsidAllocator;	// 0's in positions which exist
	
	TInt asid = -1;
	TInt lastAsid = KArmV6NumAsids - 1;
	const TUint32* ptr = pB->iMap;
	do
		{
		TUint32 bits = *ptr++;
		do
			{
			++asid;
			if ((bits & 0x80000000u) == 0)
				{
				// mapped in this address space - bitmap is inverted
				TPde* pageDir=PageDirectory(asid);
				TPde pde=pageDir[pdeIndex];
				if ((pde & KPdePageTableAddrMask) == aOld)
					{
					TPde newPde=aNew|(pde&~KPdePageTableAddrMask);
					pageDir[pdeIndex]=newPde;
					CacheMaintenance::SinglePteUpdated((TLinAddr)(pageDir+pdeIndex));

					__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x",newPde,pageDir+pdeIndex));
					}
				}
			}
		while(bits<<=1);
		NKern::FlashSystem();
		asid |= 31;
		}
	while(asid<lastAsid);
	}

void ArmMmu::RemapPageTableAliases(TPhysAddr aOld, TPhysAddr aNew)
//
// Replace aliases of the specified page table.
// This should be called with the system locked and the MMU mutex held.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::RemapPageTableAliases %08x to %08x",aOld,aNew));
	SDblQue checkedList;
	SDblQueLink* next;

	while(!iAliasList.IsEmpty())
		{
		next = iAliasList.First()->Deque();
		checkedList.Add(next);
		DMemModelThread* thread = (DMemModelThread*)((TInt)next-_FOFF(DMemModelThread,iAliasLink));
		TPde pde = thread->iAliasPde;
		if ((pde & ~KPageMask) == aOld)
			{
			// a page table in this page is being aliased by the thread, so update it...
			thread->iAliasPde = (pde & KPageMask) | aNew;
			}
		NKern::FlashSystem();
		}

	// copy checkedList back to iAliasList
	iAliasList.MoveFrom(&checkedList);
	}

void ArmMmu::DoUnassignPageTable(TLinAddr aAddr, const TAny* aOsAsids)
//
// Unassign a now-empty page table currently mapping the specified linear address.
// We assume that TLB and/or cache flushing has been done when any RAM pages were unmapped.
// This should be called with the system unlocked and the MMU mutex held.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::DoUnassignPageTable at %08x a=%08x",aAddr,aOsAsids));
	TInt pdeIndex=TInt(aAddr>>KChunkShift);
	TBool gpd=(pdeIndex>=(iLocalPdSize>>2));
	TInt os_asid=(TInt)aOsAsids;
	TUint pde=0;

	SDblQue checkedList;
	SDblQueLink* next;

	if (TUint32(os_asid)<TUint32(iNumOsAsids))
		{
		// single OS ASID
		TPde* pageDir=PageDirectory(os_asid);
		NKern::LockSystem();
		pde = pageDir[pdeIndex];
		pageDir[pdeIndex]=0;
		CacheMaintenance::SinglePteUpdated((TLinAddr)(pageDir+pdeIndex));
		__KTRACE_OPT(KMMU,Kern::Printf("Clearing PDE at %08x",pageDir+pdeIndex));

		// remove any aliases of the page table...
		TUint ptId = pde>>KPageTableShift;
		while(!iAliasList.IsEmpty())
			{
			next = iAliasList.First()->Deque();
			checkedList.Add(next);
			DMemModelThread* thread = (DMemModelThread*)((TInt)next-_FOFF(DMemModelThread,iAliasLink));
			if(thread->iAliasOsAsid==os_asid && (thread->iAliasPde>>KPageTableShift)==ptId)
				{
				// the page table is being aliased by the thread, so remove it...
				thread->iAliasPde = 0;
				}
			NKern::FlashSystem();
			}
		}
	else if (os_asid==-1 && gpd)
		{
		// all OS ASIDs, address in global region
		TInt num_os_asids=iNumGlobalPageDirs;
		const TBitMapAllocator& b=*(const TBitMapAllocator*)iOsAsidAllocator;
		for (os_asid=0; num_os_asids; ++os_asid)
			{
			if (!b.NotAllocated(os_asid,1) && (iAsidInfo[os_asid]&1))
				{
				// this OS ASID exists and has a global page directory
				TPde* pageDir=PageDirectory(os_asid);
				NKern::LockSystem();
				pageDir[pdeIndex]=0;
				CacheMaintenance::SinglePteUpdated((TLinAddr)(pageDir+pdeIndex));
				NKern::UnlockSystem();
				
				__KTRACE_OPT(KMMU,Kern::Printf("Clearing PDE at %08x",pageDir+pdeIndex));
				--num_os_asids;
				}
			}
		// we don't need to look for aliases in this case, because these aren't
		// created for page tables in the global region.
		NKern::LockSystem();
		}
	else
		{
		// selection of OS ASIDs or all OS ASIDs
		const TBitMapAllocator* pB=(const TBitMapAllocator*)aOsAsids;
		if (os_asid==-1)
			pB=iOsAsidAllocator;	// 0's in positions which exist
		TInt num_os_asids=pB->iSize-pB->iAvail;
		for (os_asid=0; num_os_asids; ++os_asid)
			{
			if (pB->NotAllocated(os_asid,1))
				continue;			// os_asid is not needed
			TPde* pageDir=PageDirectory(os_asid);
			NKern::LockSystem();
			pde = pageDir[pdeIndex];
			pageDir[pdeIndex]=0;
			CacheMaintenance::SinglePteUpdated((TLinAddr)(pageDir+pdeIndex));
			NKern::UnlockSystem();
			
			__KTRACE_OPT(KMMU,Kern::Printf("Clearing PDE at %08x",pageDir+pdeIndex));
			--num_os_asids;
			}

		// remove any aliases of the page table...
		TUint ptId = pde>>KPageTableShift;
		NKern::LockSystem();
		while(!iAliasList.IsEmpty())
			{
			next = iAliasList.First()->Deque();
			checkedList.Add(next);
			DMemModelThread* thread = (DMemModelThread*)((TInt)next-_FOFF(DMemModelThread,iAliasLink));
			if((thread->iAliasPde>>KPageTableShift)==ptId && !pB->NotAllocated(thread->iAliasOsAsid,1))
				{
				// the page table is being aliased by the thread, so remove it...
				thread->iAliasPde = 0;
				}
			NKern::FlashSystem();
			}
		}

	// copy checkedList back to iAliasList
	iAliasList.MoveFrom(&checkedList);

	NKern::UnlockSystem();
	}
#endif

// Initialise page table at physical address aXptPhys to be used as page table aXptId
// to expand the virtual address range used for mapping page tables. Map the page table
// at aPhysAddr as page table aId using the expanded range.
// Assign aXptPhys to kernel's Page Directory.
// Called with system unlocked and MMU mutex held.
void ArmMmu::BootstrapPageTable(TInt aXptId, TPhysAddr aXptPhys, TInt aId, TPhysAddr aPhysAddr)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::BootstrapPageTable xptid=%04x, xptphys=%08x, id=%04x, phys=%08x",
						aXptId, aXptPhys, aId, aPhysAddr));
	
	// put in a temporary mapping for aXptPhys
	// make it noncacheable
	TPhysAddr pa=aXptPhys&~KPageMask;
	*iTempPte = pa | SP_PTE(KArmV6PermRWNO, KNormalUncachedAttr, 0, 1);
	CacheMaintenance::SinglePteUpdated((TLinAddr)iTempPte);
	
	// clear XPT
	TPte* xpt=(TPte*)(iTempAddr+(aXptPhys&KPageMask));
	memclr(xpt, KPageTableSize);

	// must in fact have aXptPhys and aPhysAddr in same physical page
	__ASSERT_ALWAYS( TUint32(aXptPhys^aPhysAddr)<TUint32(KPageSize), MM::Panic(MM::EBootstrapPageTableBadAddr));

	// so only need one mapping
	xpt[(aXptId>>KPtClusterShift)&KPagesInPDEMask] = pa | KPtPtePerm;
	CacheMaintenance::MultiplePtesUpdated((TLinAddr)xpt, KPageTableSize);

	// remove temporary mapping
	*iTempPte=0;
	CacheMaintenance::SinglePteUpdated((TLinAddr)iTempPte);
	
	InvalidateTLBForPage(iTempAddr, KERNEL_MAPPING);

	// initialise PtInfo...
	TLinAddr xptAddr = PageTableLinAddr(aXptId);
	iPtInfo[aXptId].SetGlobal(xptAddr>>KChunkShift);

	// map xpt...
	TInt pdeIndex=TInt(xptAddr>>KChunkShift);
	TPde* pageDir=PageDirectory(0);
	NKern::LockSystem();
	pageDir[pdeIndex]=aXptPhys|KPtPdePerm;
	CacheMaintenance::SinglePteUpdated((TLinAddr)(pageDir+pdeIndex));
	
	NKern::UnlockSystem();				
	}

// Edit the self-mapping entry in page table aId, mapped at aTempMap, to
// change the physical address from aOld to aNew. Used when moving page
// tables which were created by BootstrapPageTable.
// Called with system locked and MMU mutex held.
void ArmMmu::FixupXPageTable(TInt aId, TLinAddr aTempMap, TPhysAddr aOld, TPhysAddr aNew)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::FixupXPageTable id=%04x, tempmap=%08x, old=%08x, new=%08x",
						aId, aTempMap, aOld, aNew));
	
	// find correct page table inside the page
	TPte* xpt=(TPte*)(aTempMap + ((aId & KPtClusterMask) << KPageTableShift));
	// find the pte in that page table
	xpt += (aId>>KPtClusterShift)&KPagesInPDEMask;

	// switch the mapping
	__ASSERT_ALWAYS((*xpt&~KPageMask)==aOld, Panic(EFixupXPTFailed));
	*xpt = aNew | KPtPtePerm;
	// mapped with MapTemp, and thus not mapped as a PTE - have to do real cache clean.
	CacheMaintenance::SinglePteUpdated((TLinAddr)xpt);
	}

TInt ArmMmu::NewPageDirectory(TInt aOsAsid, TBool aSeparateGlobal, TPhysAddr& aPhysAddr, TInt& aNumPages)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::NewPageDirectory(%d,%d)",aOsAsid,aSeparateGlobal));
	TInt r=0;
	TInt nlocal=iLocalPdSize>>KPageShift;
	aNumPages=aSeparateGlobal ? KPageDirectorySize/KPageSize : nlocal;
	__KTRACE_OPT(KMMU,Kern::Printf("nlocal=%d, aNumPages=%d",nlocal,aNumPages));
	if (aNumPages>1)
		{
		TInt align=aSeparateGlobal ? KPageDirectoryShift : KPageDirectoryShift-1;
		r=AllocContiguousRam(aNumPages<<KPageShift, aPhysAddr, align);
		}
	else
		r=AllocRamPages(&aPhysAddr,1, EPageFixed);
	__KTRACE_OPT(KMMU,Kern::Printf("r=%d, phys=%08x",r,aPhysAddr));
	if (r!=KErrNone)
		return r;
#ifdef BTRACE_KERNEL_MEMORY
	BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryMiscAlloc, aNumPages<<KPageShift);
	Epoc::KernelMiscPages += aNumPages;
#endif
	SPageInfo* pi = SPageInfo::FromPhysAddr(aPhysAddr);
	NKern::LockSystem();
	TInt i;
	for (i=0; i<aNumPages; ++i)
		pi[i].SetPageDir(aOsAsid,i);
	NKern::UnlockSystem();
	return KErrNone;
	}

inline void CopyPdes(TPde* aDest, const TPde* aSrc, TLinAddr aBase, TLinAddr aEnd)
	{
	memcpy(aDest+(aBase>>KChunkShift), aSrc+(aBase>>KChunkShift), ((aEnd-aBase)>>KChunkShift)*sizeof(TPde));
	CacheMaintenance::MultiplePtesUpdated((TLinAddr)(aDest+(aBase>>KChunkShift)), ((aEnd-aBase)>>KChunkShift)*sizeof(TPde));
	}

inline void ZeroPdes(TPde* aDest, TLinAddr aBase, TLinAddr aEnd)
	{
	memclr(aDest+(aBase>>KChunkShift), ((aEnd-aBase)>>KChunkShift)*sizeof(TPde));
	CacheMaintenance::MultiplePtesUpdated((TLinAddr)(aDest+(aBase>>KChunkShift)), ((aEnd-aBase)>>KChunkShift)*sizeof(TPde));
	}

void ArmMmu::InitPageDirectory(TInt aOsAsid, TBool aSeparateGlobal)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::InitPageDirectory(%d,%d)",aOsAsid,aSeparateGlobal));
	TPde* newpd=PageDirectory(aOsAsid);	// new page directory
	memclr(newpd, iLocalPdSize);		// clear local page directory
	CacheMaintenance::MultiplePtesUpdated((TLinAddr)newpd, iLocalPdSize);
	if (aSeparateGlobal)
		{
		const TPde* kpd=(const TPde*)KPageDirectoryBase;	// kernel page directory
		if (iLocalPdSize==KPageSize)
			ZeroPdes(newpd, KUserSharedDataEnd1GB, KUserSharedDataEnd2GB);
		ZeroPdes(newpd, KRamDriveStartAddress, KRamDriveEndAddress);	// don't copy RAM drive
		CopyPdes(newpd, kpd, KRomLinearBase, KUserGlobalDataEnd);		// copy ROM + user global
		CopyPdes(newpd, kpd, KRamDriveEndAddress, 0x00000000);			// copy kernel mappings
		}
	}

void ArmMmu::ClearPageTable(TInt aId, TInt aFirstIndex)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::ClearPageTable(%d,%d)",aId,aFirstIndex));
	TPte* pte=PageTable(aId);
	memclr(pte+aFirstIndex, KPageTableSize-aFirstIndex*sizeof(TPte));
	CacheMaintenance::MultiplePtesUpdated((TLinAddr)(pte+aFirstIndex), KPageTableSize-aFirstIndex*sizeof(TPte));
	}

void ArmMmu::ApplyTopLevelPermissions(TLinAddr aAddr, TInt aOsAsid, TInt aNumPdes, TPde aPdePerm)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::ApplyTopLevelPermissions %04x:%08x->%08x count %d",
												aOsAsid, aAddr, aPdePerm, aNumPdes));
	TInt ix=aAddr>>KChunkShift;
	TPde* pPde=PageDirectory(aOsAsid)+ix;
	TLinAddr firstPde = (TLinAddr)pPde; //Will need this to clean page table memory region in cache

	TPde* pPdeEnd=pPde+aNumPdes;
	NKern::LockSystem();
	for (; pPde<pPdeEnd; ++pPde)
		{
		TPde pde=*pPde;
		if (pde)
			*pPde = (pde&KPdePageTableAddrMask)|aPdePerm;
		}
	CacheMaintenance::MultiplePtesUpdated(firstPde, aNumPdes*sizeof(TPde));
	FlushTLBs();
	NKern::UnlockSystem();
	}

void ArmMmu::ApplyPagePermissions(TInt aId, TInt aPageOffset, TInt aNumPages, TPte aPtePerm)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::ApplyPagePermissions %04x:%03x+%03x perm %08x",
												aId, aPageOffset, aNumPages, aPtePerm));
	TPte* pPte=PageTable(aId)+aPageOffset;
	TLinAddr firstPte = (TLinAddr)pPte; //Will need this to clean page table memory region in cache

	TPde* pPteEnd=pPte+aNumPages;
	NKern::LockSystem();
	for (; pPte<pPteEnd; ++pPte)
		{
		TPte pte=*pPte;
		if (pte)
			*pPte = (pte&KPteSmallPageAddrMask)|aPtePerm;
		}
	CacheMaintenance::MultiplePtesUpdated(firstPte, aNumPages*sizeof(TPte));
	FlushTLBs();
	NKern::UnlockSystem();
	}

void ArmMmu::ClearRamDrive(TLinAddr aStart)
	{
	// clear the page directory entries corresponding to the RAM drive
	TPde* kpd=(TPde*)KPageDirectoryBase;	// kernel page directory
	ZeroPdes(kpd, aStart, KRamDriveEndAddress);
	}

TPde ArmMmu::PdePermissions(TChunkType aChunkType, TBool aRO)
	{
//	if (aChunkType==EUserData && aRO)
//		return KPdePtePresent|KPdePteUser;
	return ChunkPdePermissions[aChunkType];
	}

TPte ArmMmu::PtePermissions(TChunkType aChunkType)
	{
	return ChunkPtePermissions[aChunkType];
	}

// Set up a page table (specified by aId) to map a 1Mb section of ROM containing aRomAddr
// using ROM at aOrigPhys.
void ArmMmu::InitShadowPageTable(TInt aId, TLinAddr aRomAddr, TPhysAddr aOrigPhys)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("ArmMmu:InitShadowPageTable id=%04x aRomAddr=%08x aOrigPhys=%08x",
		aId, aRomAddr, aOrigPhys));
	TPte* ppte = PageTable(aId);
	TLinAddr firstPte = (TLinAddr)ppte; //Will need this to clean page table memory region in cache

	TPte* ppte_End = ppte + KChunkSize/KPageSize;
	TPhysAddr phys = aOrigPhys - (aRomAddr & KChunkMask);
	for (; ppte<ppte_End; ++ppte, phys+=KPageSize)
		*ppte = phys | KRomPtePerm;
	CacheMaintenance::MultiplePtesUpdated(firstPte, sizeof(TPte)*KChunkSize/KPageSize);
	}

// Copy the contents of ROM at aRomAddr to a shadow page at physical address aShadowPhys
// It is assumed aShadowPage is not mapped, therefore any mapping colour is OK.
void ArmMmu::InitShadowPage(TPhysAddr aShadowPhys, TLinAddr aRomAddr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("ArmMmu:InitShadowPage aShadowPhys=%08x aRomAddr=%08x",
		aShadowPhys, aRomAddr));

	// put in a temporary mapping for aShadowPhys
	// make it noncacheable
	*iTempPte = aShadowPhys | SP_PTE(KArmV6PermRWNO, KNormalUncachedAttr, 0, 1);
	CacheMaintenance::SinglePteUpdated((TLinAddr)iTempPte);

	// copy contents of ROM
	wordmove( (TAny*)iTempAddr, (const TAny*)aRomAddr, KPageSize );
	//Temp address is uncached. No need to clean cache, just flush write buffer
	CacheMaintenance::MemoryToPreserveAndReuse((TLinAddr)iTempAddr, KPageSize, EMapAttrBufferedC);
	
	// remove temporary mapping
	*iTempPte=0;
	CacheMaintenance::SinglePteUpdated((TLinAddr)iTempPte);
	InvalidateTLBForPage(iTempAddr, KERNEL_MAPPING);
	}

// Assign a shadow page table to replace a ROM section mapping
// Enter and return with system locked
void ArmMmu::AssignShadowPageTable(TInt aId, TLinAddr aRomAddr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("ArmMmu:AssignShadowPageTable aId=%04x aRomAddr=%08x",
		aId, aRomAddr));
	TLinAddr ptLin=PageTableLinAddr(aId);
	TPhysAddr ptPhys=LinearToPhysical(ptLin, 0);
	TPde* ppde = ::InitPageDirectory + (aRomAddr>>KChunkShift);
	TPde newpde = ptPhys | KShadowPdePerm;
	__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x", newpde, ppde));
	TInt irq=NKern::DisableAllInterrupts();
	*ppde = newpde;		// map in the page table
	CacheMaintenance::SinglePteUpdated((TLinAddr)ppde);
	
	FlushTLBs();	// flush both TLBs (no need to flush cache yet)
	NKern::RestoreInterrupts(irq);
	}

void ArmMmu::DoUnmapShadowPage(TInt aId, TLinAddr aRomAddr, TPhysAddr aOrigPhys)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu:DoUnmapShadowPage, id=%04x lin=%08x origphys=%08x", aId, aRomAddr, aOrigPhys));
	TPte* ppte = PageTable(aId) + ((aRomAddr & KChunkMask)>>KPageShift);
	TPte newpte = aOrigPhys | KRomPtePerm;
	__KTRACE_OPT(KMMU,Kern::Printf("Writing PTE %08x to %08x", newpte, ppte));
	TInt irq=NKern::DisableAllInterrupts();
	*ppte = newpte;
	CacheMaintenance::SinglePteUpdated((TLinAddr)ppte);
	
	InvalidateTLBForPage(aRomAddr, KERNEL_MAPPING);
	#if defined(__CPU_ARM1136__) && !defined(__CPU_ARM1136_ERRATUM_353494_FIXED)
	__FlushBtb();
	#endif

	CacheMaintenance::CodeChanged(aRomAddr, KPageSize, CacheMaintenance::EMemoryRemap);
	CacheMaintenance::PageToReuse(aRomAddr, EMemAttNormalCached, KPhysAddrInvalid);
	NKern::RestoreInterrupts(irq);
	}

TInt ArmMmu::UnassignShadowPageTable(TLinAddr aRomAddr, TPhysAddr aOrigPhys)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu:UnassignShadowPageTable, lin=%08x origphys=%08x", aRomAddr, aOrigPhys));
	TPde* ppde = ::InitPageDirectory + (aRomAddr>>KChunkShift);
	TPde newpde = (aOrigPhys &~ KChunkMask) | KRomSectionPermissions;
	__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x", newpde, ppde));
	TInt irq=NKern::DisableAllInterrupts();
	*ppde = newpde;			// revert to section mapping
	CacheMaintenance::SinglePteUpdated((TLinAddr)ppde);
	
	FlushTLBs();			// flush both TLBs
	NKern::RestoreInterrupts(irq);
	return KErrNone;
	}


#if defined(__CPU_MEMORY_TYPE_REMAPPING)	// arm1176, arm11mcore, armv7, ...
/**
Shadow pages on platforms with remapping (mpcore, 1176, cortex...) are not writable.
This will map the region into writable memory first.
@pre No Fast Mutex held
*/
TInt ArmMmu::CopyToShadowMemory(TLinAddr aDest, TLinAddr aSrc, TUint32 aLength)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("ArmMmu:CopyToShadowMemory aDest=%08x aSrc=%08x aLength=%08x", aDest, aSrc, aLength));

	// Check that destination is ROM
	if (aDest<iRomLinearBase || (aDest+aLength) > iRomLinearEnd)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu:CopyToShadowMemory: Destination not entirely in ROM"));
		return KErrArgument;
		}
	// do operation with RamAlloc mutex held (to prevent shadow pages from being released from under us)
	MmuBase::Wait();


	TInt r = KErrNone;
	while (aLength)
		{
		// Calculate memory size to copy in this loop. A single page region will be copied per loop
		TInt copySize = Min(aLength, iPageSize - (aDest&iPageMask));

		// Get physical address
		TPhysAddr	physAddr = LinearToPhysical(aDest&~iPageMask, 0);
		if (KPhysAddrInvalid==physAddr)
			{
			r = KErrArgument;
			break;
			}
		
		//check whether it is shadowed rom
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(physAddr);
		if (pi==0 || pi->Type()!=SPageInfo::EShadow)
			{
			__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu:CopyToShadowMemory: No shadow page at this address"));
			r = KErrArgument;
			break;
			}

		//Temporarily map into writable memory and copy data. RamAllocator DMutex is required
		TLinAddr tempAddr = MapTemp (physAddr, aDest&~iPageMask);
		__KTRACE_OPT(KMMU, Kern::Printf("ArmMmu:CopyToShadowMemory Copy aDest=%08x aSrc=%08x aSize=%08x", tempAddr+(aDest&iPageMask), aSrc, copySize));
		memcpy ((TAny*)(tempAddr+(aDest&iPageMask)), (const TAny*)aSrc, copySize);  //Kernel-to-Kernel copy is presumed
		UnmapTemp();

		//Update variables for the next loop/page
		aDest+=copySize;
		aSrc+=copySize;
		aLength-=copySize;
		}
	MmuBase::Signal();
	return r;
	}
#endif

void ArmMmu::DoFreezeShadowPage(TInt aId, TLinAddr aRomAddr)
	{
#if defined(__CPU_MEMORY_TYPE_REMAPPING) //arm1176, arm11mcore, armv7 and later
	__KTRACE_OPT(KMMU, Kern::Printf("ArmMmu:DoFreezeShadowPage not required with MEMORY_TYPE_REMAPPING"));
#else
	__KTRACE_OPT(KMMU, Kern::Printf("ArmMmu:DoFreezeShadowPage aId=%04x aRomAddr=%08x",
		aId, aRomAddr));
	TPte* ppte = PageTable(aId) + ((aRomAddr & KChunkMask)>>KPageShift);
	TPte newpte = (*ppte & KPteSmallPageAddrMask) | KRomPtePerm;
	__KTRACE_OPT(KMMU,Kern::Printf("Writing PTE %08x to %08x", newpte, ppte));
	*ppte = newpte;
	CacheMaintenance::SinglePteUpdated((TLinAddr)ppte);
	InvalidateTLBForPage(aRomAddr, KERNEL_MAPPING);
#endif	
	}

/** Replaces large page(64K) entry in page table with small page(4K) entries.*/
void ArmMmu::Pagify(TInt aId, TLinAddr aLinAddr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("ArmMmu:Pagify aId=%04x aLinAddr=%08x", aId, aLinAddr));
	
	TInt pteIndex = (aLinAddr & KChunkMask)>>KPageShift;
	TPte* pte = PageTable(aId);
	if ((pte[pteIndex] & KArmV6PteTypeMask) == KArmV6PteLargePage)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("Converting 64K page to 4K pages"));
		pteIndex &= ~0xf;
		TPte source = pte[pteIndex];
		source = (source & KPteLargePageAddrMask) | SP_PTE_FROM_LP_PTE(source);
		pte += pteIndex;
		for (TInt entry=0; entry<16; entry++)
			{
			pte[entry] = source | (entry<<12);
			}
		CacheMaintenance::MultiplePtesUpdated((TLinAddr)pte, 16*sizeof(TPte));
		FlushTLBs();
		}
	}

void ArmMmu::FlushShadow(TLinAddr aRomAddr)
	{
	CacheMaintenance::CodeChanged(aRomAddr, KPageSize, CacheMaintenance::EMemoryRemap);
	CacheMaintenance::PageToReuse(aRomAddr, EMemAttNormalCached, KPhysAddrInvalid);
	InvalidateTLBForPage(aRomAddr, KERNEL_MAPPING);		// remove all TLB references to original ROM page
	}


#if defined(__CPU_MEMORY_TYPE_REMAPPING) //arm1176, arm11mcore, armv7
/**
Calculates page directory/table entries for memory type described in aMapAttr.
Global, small page (4KB) mapping is assumed.
(All magic numbers come from ARM page table descriptions.)
@param aMapAttr On entry, holds description(memory type, access permisions,...) of the memory.
				It is made up of TMappingAttributes constants or TMappingAttributes2 object. If TMappingAttributes,
				may be altered 	on exit to hold the actual cache attributes & access permissions.
@param aPde		On exit, holds page-table-entry for the 1st level descriptor
				for given type of memory, with base address set to 0.
@param aPte		On exit, holds small-page-entry (4K) for the 2nd level descriptor
				for given type of memory, with base address set to 0.
@return KErrNotSupported 	If memory described in aMapAttr is not supported
		KErrNone			Otherwise
*/
TInt ArmMmu::PdePtePermissions(TUint& aMapAttr, TPde& aPde, TPte& aPte)
	{
	__KTRACE_OPT(KMMU,Kern::Printf(">ArmMmu::PdePtePermissions, mapattr=%08x",aMapAttr));

	TMappingAttributes2& memory = (TMappingAttributes2&)aMapAttr;

	if(memory.ObjectType2())
		{
//---------Memory described by TMappingAttributes2 object-----------------
		aPde = 	KArmV6PdePageTable	|
				(memory.Parity() ? KArmV6PdeECCEnable : 0);
#if defined(FAULTY_NONSHARED_DEVICE_MEMORY)
		if(!memory.Shared() && (memory.Type() == EMemAttDevice ))
		{
			aMapAttr ^= EMapAttrBufferedNC;
			aMapAttr |= EMapAttrFullyBlocking;
			// Clear EMemAttDevice
			aMapAttr ^= (EMemAttDevice << 26);
			aMapAttr |= (EMemAttStronglyOrdered << 26);
		}
#endif
		aPte =	KArmV6PteSmallPage										|
				KArmV6PteAP0											|	// AP0 bit always 1
				((memory.Type()&3)<<2) | ((memory.Type()&4)<<4)			|	// memory type
				(memory.Executable() ? 0			: KArmV6PteSmallXN)	|	// eXecuteNever bit
#if defined	(__CPU_USE_SHARED_MEMORY)
				KArmV6PteS 												|	// Memory is always shared.
#else
				(memory.Shared()	  ? KArmV6PteS	: 0) 				|	// Shared bit
#endif				
				(memory.Writable()	  ? 0			: KArmV6PteAPX)		|	// APX = !Writable
				(memory.UserAccess() ? KArmV6PteAP1: 0);					// AP1 = UserAccess
		// aMapAttr remains the same
		}
	else
		{
//---------Memory described by TMappingAttributes bitmask-----------------
#if defined(FAULTY_NONSHARED_DEVICE_MEMORY)
		if(((aMapAttr & EMapAttrL1CacheMask) == EMapAttrBufferedNC) && !(aMapAttr & EMapAttrShared))
		{
			// Clear EMapAttrBufferedNC attribute
			aMapAttr ^= EMapAttrBufferedNC;
			aMapAttr |= EMapAttrFullyBlocking;
		}
#endif
		//	1.	Calculate TEX0:C:B bits in page table and actual cache attributes.
		//		Only L1 cache attribute from aMapAttr matters. Outer (L2) cache policy will be the same as inner one.
		TUint l1cache=aMapAttr & EMapAttrL1CacheMask; // Inner cache attributes. May change to actual value.
		TUint l2cache;	// Will hold actual L2 cache attributes (in terms of TMappingAttributes constants)
		TUint tex0_c_b; // Will hold TEX[0]:C:B value in page table

		switch (l1cache)
			{
			case EMapAttrFullyBlocking:
				tex0_c_b = EMemAttStronglyOrdered;
				l2cache = EMapAttrL2Uncached;
				break;
			case EMapAttrBufferedNC:
				tex0_c_b = EMemAttDevice;
				l2cache = EMapAttrL2Uncached;
				break;
			case EMapAttrBufferedC:
			case EMapAttrL1Uncached:
			case EMapAttrCachedWTRA:
			case EMapAttrCachedWTWA:
				tex0_c_b = EMemAttNormalUncached;
				l1cache = EMapAttrBufferedC;
				l2cache = EMapAttrL2Uncached;
				break;
			case EMapAttrCachedWBRA:
			case EMapAttrCachedWBWA:
			case EMapAttrL1CachedMax:
				tex0_c_b = EMemAttNormalCached;
				l1cache = EMapAttrCachedWBWA;
				l2cache = EMapAttrL2CachedWBWA;
				break;
			default:
				return KErrNotSupported;
			}

		//	2.	Step 2 has been removed :)

		//	3.	Calculate access permissions (apx:ap bits in page table + eXecute it)
		TUint read=aMapAttr & EMapAttrReadMask;
		TUint write=(aMapAttr & EMapAttrWriteMask)>>4;
		TUint exec=(aMapAttr & EMapAttrExecMask)>>8;

		read|=exec; 		// User/Sup execute access requires User/Sup read access.
		if (exec) exec = 1; // There is a single eXecute bit in page table. Set to one if User or Sup exec is required.

		TUint apxap=0;
		if (write==0) 		// no write required
			{
			if 		(read>=4)	apxap=KArmV6PermRORO;		// user read required
			else if (read==1) 	apxap=KArmV6PermRONO;		// supervisor read required
			else 				return KErrNotSupported;	// no read required
			}
		else if (write<4)	// supervisor write required
			{
			if (read<4) 		apxap=KArmV6PermRWNO;		// user read not required
			else 				return KErrNotSupported;	// user read required 
			}
		else				// user & supervisor writes required
			{
			apxap=KArmV6PermRWRW;		
			}
	
		//	4.	Calculate page-table-entry for the 1st level (aka page directory) descriptor 
		aPde=((aMapAttr&EMapAttrUseECC)>>8)|KArmV6PdePageTable;

		//	5.	Calculate small-page-entry for the 2nd level (aka page table) descriptor 
		aPte=SP_PTE(apxap, tex0_c_b, exec, 1);	// always global
		if (aMapAttr&EMapAttrShared)
			aPte |= KArmV6PteS;
	
		//	6.	Fix aMapAttr to hold the actual values for access permission & cache attributes
		TUint xnapxap=((aPte<<3)&8)|((aPte>>7)&4)|((aPte>>4)&3);
		aMapAttr &= ~(EMapAttrAccessMask|EMapAttrL1CacheMask|EMapAttrL2CacheMask);
		aMapAttr |= PermissionLookup[xnapxap]; 	// Set actual access permissions
		aMapAttr |= l1cache;					// Set actual inner cache attributes
		aMapAttr |= l2cache;					// Set actual outer cache attributes
		}

	__KTRACE_OPT(KMMU,Kern::Printf("<ArmMmu::PdePtePermissions, mapattr=%08x, pde=%08x, pte=%08x", 	aMapAttr, aPde, aPte));
	return KErrNone;
	}

#else //ARMv6 (arm1136)

const TUint FBLK=(EMapAttrFullyBlocking>>12);
const TUint BFNC=(EMapAttrBufferedNC>>12);
//const TUint BUFC=(EMapAttrBufferedC>>12);
const TUint L1UN=(EMapAttrL1Uncached>>12);
const TUint WTRA=(EMapAttrCachedWTRA>>12);
//const TUint WTWA=(EMapAttrCachedWTWA>>12);
const TUint WBRA=(EMapAttrCachedWBRA>>12);
const TUint WBWA=(EMapAttrCachedWBWA>>12);
const TUint AWTR=(EMapAttrAltCacheWTRA>>12);
//const TUint AWTW=(EMapAttrAltCacheWTWA>>12);
//const TUint AWBR=(EMapAttrAltCacheWBRA>>12);
const TUint AWBW=(EMapAttrAltCacheWBWA>>12);
const TUint MAXC=(EMapAttrL1CachedMax>>12);

const TUint L2UN=(EMapAttrL2Uncached>>16);

const TUint8 UNS=0xffu;	// Unsupported attribute

//Maps L1 & L2 cache attributes into TEX[4:2]:CB[1:0]
//ARMv6 doesn't do WTWA so we use WTRA instead

#if !defined(__CPU_ARM1136_ERRATUM_399234_FIXED)
// L1 Write-Through mode is outlawed, L1WT acts as L1UN.
static const TUint8 CBTEX[40]=
	{            // L1CACHE:
//  FBLK  BFNC  BUFC  L1UN  WTRA  WTWA  WBRA  WBWA 	  L2CACHE:
	0x00, 0x01, 0x01, 0x04, 0x04, 0x04, 0x13, 0x11,	//NC
	0x00, 0x01, 0x01, 0x18, 0x18, 0x18, 0x1b, 0x19,	//WTRA
	0x00, 0x01, 0x01, 0x18, 0x18, 0x18, 0x1b, 0x19,	//WTWA
	0x00, 0x01, 0x01, 0x1c, 0x1c, 0x1c, 0x1f, 0x1d,	//WBRA
	0x00, 0x01, 0x01, 0x14, 0x14, 0x14, 0x17, 0x15	//WBWA
	};
#else
static const TUint8 CBTEX[40]=
	{            // L1CACHE:
//  FBLK  BFNC  BUFC  L1UN  WTRA  WTWA  WBRA  WBWA 	  L2CACHE:
	0x00, 0x01, 0x01, 0x04, 0x12, 0x12, 0x13, 0x11,	//NC
	0x00, 0x01, 0x01, 0x18, 0x02, 0x02, 0x1b, 0x19,	//WTRA
	0x00, 0x01, 0x01, 0x18, 0x02, 0x02, 0x1b, 0x19,	//WTWA
	0x00, 0x01, 0x01, 0x1c, 0x1e, 0x1e, 0x1f, 0x1d,	//WBRA
	0x00, 0x01, 0x01, 0x14, 0x16, 0x16, 0x17, 0x15	//WBWA
	};
#endif

//Maps TEX[4:2]:CB[1:0] value into L1 cache attributes
static const TUint8 L1Actual[32]=
	{
//CB 00		 01		 10		 11		//TEX
	FBLK,	BFNC,	WTRA,	WBRA,	//000
	L1UN,  	UNS,  	UNS, 	WBWA,	//001
	BFNC,	UNS,	UNS,  	UNS,	//010
	UNS,	UNS,	UNS,	UNS,	//011
	L1UN, 	WBWA, 	WTRA, 	WBRA,	//100
	L1UN, 	WBWA, 	WTRA, 	WBRA,	//101
	L1UN, 	WBWA, 	WTRA, 	WBRA,	//110
	L1UN, 	WBWA, 	WTRA, 	WBRA	//111
	};

//Maps TEX[4:2]:CB[1:0] value into L2 cache attributes
static const TUint8 L2Actual[32]=
	{
//CB 00		 01		 10		 11		//TEX
	L2UN,	L2UN,	WTRA,	WBRA,	//000
	L2UN,	UNS,	UNS,	WBWA,	//001
	L2UN,	UNS,	UNS,	UNS,	//010
	UNS,	UNS,	UNS,	UNS,	//011
	L2UN,	L2UN,	L2UN,	L2UN,	//100
	WBWA,	WBWA,	WBWA,	WBWA,	//101
	WTRA,	WTRA,	WTRA,	WTRA,	//110
	WBRA,	WBRA,	WBRA,	WBRA	//111
	};

TInt ArmMmu::PdePtePermissions(TUint& aMapAttr, TPde& aPde, TPte& aPte)
	{
	__KTRACE_OPT(KMMU,Kern::Printf(">ArmMmu::PdePtePermissions, mapattr=%08x",aMapAttr));

	TUint read=aMapAttr & EMapAttrReadMask;
	TUint write=(aMapAttr & EMapAttrWriteMask)>>4;
	TUint exec=(aMapAttr & EMapAttrExecMask)>>8;
	TUint l1cache=(aMapAttr & EMapAttrL1CacheMask)>>12;
	TUint l2cache=(aMapAttr & EMapAttrL2CacheMask)>>16;
	if (l1cache==MAXC) l1cache=WBRA;	// map max cache to WBRA
	if (l1cache>AWBW)
		return KErrNotSupported;		// undefined attribute
	if (l1cache>=AWTR) l1cache-=4;		// no alternate cache, so use normal cache
	if (l1cache<L1UN) l2cache=0;		// for blocking/device, don't cache L2
	if (l2cache==MAXC) l2cache=WBRA;	// map max cache to WBRA
	if (l2cache>WBWA)
		return KErrNotSupported;		// undefined attribute
	if (l2cache) l2cache-=(WTRA-1);		// l2cache now in range 0-4
	aPde=((aMapAttr&EMapAttrUseECC)>>8)|KArmV6PdePageTable;

#if defined(__CPU_ARM1136__) && !defined(__CPU_ARM1136_ERRATUM_353494_FIXED)
	// if broken 1136, can't have supervisor only code
	if (exec)
		exec = TUint(EMapAttrExecUser>>8);
#endif

	// if any execute access, must have read=execute
	if (exec)
		(void)(read>=exec || (read=exec)!=0), exec=1;

	// l1cache between 0 and 7, l2cache between 0 and 4; look up CBTEX
	TUint cbtex=CBTEX[(l2cache<<3)|l1cache];

	// work out apx:ap
	TUint apxap;
	if (write==0)
		apxap=(read>=4)?KArmV6PermRORO:(read?KArmV6PermRONO:KArmV6PermNONO);
	else if (write<4)
		apxap=(read>=4)?KArmV6PermRWRO:KArmV6PermRWNO;
	else
		apxap=KArmV6PermRWRW;
	TPte pte=SP_PTE(apxap, cbtex, exec, 1);	// always global
	if (aMapAttr&EMapAttrShared)
		pte |= KArmV6PteS;

	// Translate back to get actual map attributes
	TUint xnapxap=((pte<<3)&8)|((pte>>7)&4)|((pte>>4)&3);
	cbtex=((pte>>4)&0x1c)|((pte>>2)&3);  // = TEX[4:2]::CB[1:0]
	aMapAttr &= ~(EMapAttrAccessMask|EMapAttrL1CacheMask|EMapAttrL2CacheMask);
	aMapAttr |= PermissionLookup[xnapxap];
	aMapAttr |= (L1Actual[cbtex]<<12);
	aMapAttr |= (L2Actual[cbtex]<<16);
	aPte=pte;
	__KTRACE_OPT(KMMU,Kern::Printf("<ArmMmu::PdePtePermissions, mapattr=%08x, pde=%08x, pte=%08x",
								aMapAttr, aPde, aPte));
	return KErrNone;
	}
#endif

void ArmMmu::Map(TLinAddr aLinAddr, TPhysAddr aPhysAddr, TInt aSize, TPde aPdePerm, TPte aPtePerm, TInt aMapShift)
//
// Map a region of physical addresses aPhysAddr to aPhysAddr+aSize-1 to virtual address aLinAddr.
// Use permissions specified by aPdePerm and aPtePerm. Use mapping sizes up to and including (1<<aMapShift).
// Assume any page tables required are already assigned.
// aLinAddr, aPhysAddr, aSize must be page-aligned.
//
	{
	__KTRACE_OPT(KMMU, Kern::Printf("ArmMmu::Map lin=%08x phys=%08x size=%08x", aLinAddr, aPhysAddr, aSize));
	__KTRACE_OPT(KMMU, Kern::Printf("pde=%08x pte=%08x mapshift=%d", aPdePerm, aPtePerm, aMapShift));
	TPde pt_pde=aPdePerm;
	TPte sp_pte=aPtePerm;
	TPde section_pde=SECTION_PDE_FROM_PDEPTE(pt_pde, sp_pte);
	TPte lp_pte=LP_PTE_FROM_SP_PTE(sp_pte);
	TLinAddr la=aLinAddr;
	TPhysAddr pa=aPhysAddr;
	TInt remain=aSize;
	while (remain)
		{
		if (aMapShift>=KChunkShift && (la & KChunkMask)==0 && remain>=KChunkSize)
			{
			// use sections - ASSUMES ADDRESS IS IN GLOBAL REGION
			TInt npdes=remain>>KChunkShift;
			const TBitMapAllocator& b=*iOsAsidAllocator;
			TInt num_os_asids=iNumGlobalPageDirs;
			TInt os_asid=0;
			for (; num_os_asids; ++os_asid)
				{
				if (b.NotAllocated(os_asid,1) || (iAsidInfo[os_asid]&1)==0)
					continue;			// os_asid is not needed
				TPde* p_pde=PageDirectory(os_asid)+(la>>KChunkShift);
				TPde* p_pde_E=p_pde+npdes;
				TPde pde=pa|section_pde;
				TLinAddr firstPde = (TLinAddr)p_pde; //Will need this to clean page table memory region from cache

				NKern::LockSystem();
				for (; p_pde < p_pde_E; pde+=KChunkSize)
					{
					__ASSERT_DEBUG(*p_pde==0, MM::Panic(MM::EPdeAlreadyInUse));
					__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x", pde, p_pde));
					*p_pde++=pde;
					}
				CacheMaintenance::MultiplePtesUpdated(firstPde, (TUint)p_pde-firstPde);
				NKern::UnlockSystem();
				--num_os_asids;
				}
			npdes<<=KChunkShift;
			la+=npdes, pa+=npdes, remain-=npdes;
			continue;
			}
		TInt block_size = Min(remain, KChunkSize-(la&KChunkMask));
		TPte pa_mask=~KPageMask;
		TPte pte_perm=sp_pte;
		if (aMapShift>=KLargePageShift && block_size>=KLargePageSize)
			{
			if ((la & KLargePageMask)==0)
				{
				// use 64K large pages
				pa_mask=~KLargePageMask;
				pte_perm=lp_pte;
				}
			else
				block_size = Min(remain, KLargePageSize-(la&KLargePageMask));
			}
		block_size &= pa_mask;

		// use pages (large or small)
		TInt id=PageTableId(la, 0);
		__ASSERT_DEBUG(id>=0, MM::Panic(MM::EMmuMapNoPageTable));
		TPte* p_pte=PageTable(id)+((la&KChunkMask)>>KPageShift);
		TPte* p_pte_E=p_pte + (block_size>>KPageShift);
		SPageTableInfo& ptinfo=iPtInfo[id];
		TLinAddr firstPte = (TLinAddr)p_pte; //Will need this to clean page table memory region from cache
		
		NKern::LockSystem();
		for (; p_pte < p_pte_E; pa+=KPageSize)
			{
			__ASSERT_DEBUG(*p_pte==0, MM::Panic(MM::EPteAlreadyInUse));
			TPte pte = (pa & pa_mask) | pte_perm;
			__KTRACE_OPT(KMMU,Kern::Printf("Writing PTE %08x to %08x", pte, p_pte));
			*p_pte++=pte;
			++ptinfo.iCount;
			NKern::FlashSystem();
			}
		CacheMaintenance::MultiplePtesUpdated(firstPte, (TUint)p_pte-firstPte);
		NKern::UnlockSystem();
		la+=block_size, remain-=block_size;
		}
	}

void ArmMmu::Unmap(TLinAddr aLinAddr, TInt aSize)
//
// Remove all mappings in the specified range of addresses.
// Assumes there are only global mappings involved.
// Don't free page tables.
// aLinAddr, aSize must be page-aligned.
//
	{
	__KTRACE_OPT(KMMU, Kern::Printf("ArmMmu::Unmap lin=%08x size=%08x", aLinAddr, aSize));
	TLinAddr a=aLinAddr;
	TLinAddr end=a+aSize;
	__KTRACE_OPT(KMMU,Kern::Printf("a=%08x end=%08x",a,end));
	NKern::LockSystem();
	while(a!=end)
		{
		TInt pdeIndex=a>>KChunkShift;
		TLinAddr next=(pdeIndex<<KChunkShift)+KChunkSize;
		TInt to_do = Min(TInt(end-a), TInt(next-a))>>KPageShift;
		__KTRACE_OPT(KMMU,Kern::Printf("a=%08x next=%08x to_do=%d",a,next,to_do));
		TPde pde=::InitPageDirectory[pdeIndex];
		if ( (pde&KArmV6PdeTypeMask)==KArmV6PdeSection )
			{
			__ASSERT_DEBUG(!(a&KChunkMask), MM::Panic(MM::EUnmapBadAlignment));
#if defined(__CPU_ARM1136__) && !defined(__CPU_ARM1136_ERRATUM_353494_FIXED)
			remove_and_invalidate_section(::InitPageDirectory + pdeIndex, a, KERNEL_MAPPING);
#else
			::InitPageDirectory[pdeIndex]=0;
			CacheMaintenance::SinglePteUpdated(TLinAddr(::InitPageDirectory + pdeIndex));
			InvalidateTLBForPage(a, KERNEL_MAPPING);		// ASID irrelevant since global
#endif
			a=next;
			NKern::FlashSystem();
			continue;
			}
		TInt ptid=PageTableId(a,0);
		SPageTableInfo& ptinfo=iPtInfo[ptid];
		if (ptid>=0)
			{
			TPte* ppte=PageTable(ptid)+((a&KChunkMask)>>KPageShift);
			TPte* ppte_End=ppte+to_do;
			for (; ppte<ppte_End; ++ppte, a+=KPageSize)
				{
				if (*ppte & KArmV6PteSmallPage)
					{
					--ptinfo.iCount;
#if defined(__CPU_ARM1136__) && !defined(__CPU_ARM1136_ERRATUM_353494_FIXED)
					remove_and_invalidate_page(ppte, a, KERNEL_MAPPING);
#else
					*ppte=0;
					CacheMaintenance::SinglePteUpdated((TLinAddr)ppte);
					InvalidateTLBForPage(a, KERNEL_MAPPING);
#endif
					}
				else if ((*ppte & KArmV6PteTypeMask) == KArmV6PteLargePage)
					{
					__ASSERT_DEBUG(!(a&KLargePageMask), MM::Panic(MM::EUnmapBadAlignment));
					ptinfo.iCount-=KLargeSmallPageRatio;
#if defined(__CPU_ARM1136__) && !defined(__CPU_ARM1136_ERRATUM_353494_FIXED)
					remove_and_invalidate_page(ppte, a, KERNEL_MAPPING);
#else
					memclr(ppte, KLargeSmallPageRatio*sizeof(TPte));
					CacheMaintenance::MultiplePtesUpdated((TLinAddr)ppte, KLargeSmallPageRatio*sizeof(TPte));
					InvalidateTLBForPage(a, KERNEL_MAPPING);
#endif
					a+=(KLargePageSize-KPageSize);
					ppte+=(KLargeSmallPageRatio-1);
					}
				NKern::FlashSystem();
				}
			}
		else
			a += (to_do<<KPageShift);
		}
	NKern::UnlockSystem();
	#if defined(__CPU_ARM1136__) && !defined(__CPU_ARM1136_ERRATUM_353494_FIXED)
	__FlushBtb();
	#endif
	}


void ArmMmu::ClearPages(TInt aNumPages, TPhysAddr* aPageList, TUint8 aClearByte)
	{
	//map the pages at a temporary address, clear them and unmap
	__ASSERT_MUTEX(RamAllocatorMutex);
	while (--aNumPages >= 0)
		{
		TPhysAddr pa;
		if((TInt)aPageList&1)
			{
			pa = (TPhysAddr)aPageList&~1;
			*(TPhysAddr*)&aPageList += iPageSize;
			}
		else
			pa = *aPageList++;
		
		*iTempPte = pa | SP_PTE(KArmV6PermRWNO, KNormalUncachedAttr, 0, 1);
		CacheMaintenance::SinglePteUpdated((TLinAddr)iTempPte);
		InvalidateTLBForPage(iTempAddr, KERNEL_MAPPING);
		memset((TAny*)iTempAddr, aClearByte, iPageSize);
		// This temporary mapping is noncached => No need to flush cache here.
		// Still, we have to make sure that write buffer(s) are drained.
		CacheMaintenance::MemoryToPreserveAndReuse((TLinAddr)iTempAddr, iPageSize, EMapAttrBufferedC);
		}
	*iTempPte=0;
	CacheMaintenance::SinglePteUpdated((TLinAddr)iTempPte);
	InvalidateTLBForPage(iTempAddr, KERNEL_MAPPING);
	}


/**
Create a temporary mapping of one or more contiguous physical pages.
Fully cached memory attributes apply.
The RamAllocatorMutex must be held before this function is called and not released
until after UnmapTemp has been called.

@param aPage	The physical address of the pages to be mapped.
@param aLinAddr The linear address of any existing location where the page is mapped.
				If the page isn't already mapped elsewhere as a cachable page then
				this value irrelevent. (It is used for page colouring.)
@param aPages	Number of pages to map.

@return The linear address of where the pages have been mapped.
*/
TLinAddr ArmMmu::MapTemp(TPhysAddr aPage,TLinAddr aLinAddr,TInt aPages)
	{
	__ASSERT_MUTEX(RamAllocatorMutex);
	__ASSERT_DEBUG(!*iTempPte,MM::Panic(MM::ETempMappingAlreadyInUse));
	iTempMapColor = (aLinAddr>>KPageShift)&KPageColourMask;
	iTempMapCount = aPages;
	if (aPages==1)
		{
		iTempPte[iTempMapColor] = (aPage&~KPageMask) | SP_PTE(KArmV6PermRWNO, KNormalCachedAttr, 0, 1);
		CacheMaintenance::SinglePteUpdated((TLinAddr)(iTempPte+iTempMapColor));
		}
	else
		{
		__ASSERT_DEBUG(iTempMapColor+aPages<=KPageColourCount,MM::Panic(MM::ETempMappingNoRoom));
		for (TInt i=0; i<aPages; i++)
			iTempPte[iTempMapColor+i] = ((aPage&~KPageMask)+(i<<KPageShift)) | SP_PTE(KArmV6PermRWNO, KNormalCachedAttr, 0, 1);	
		CacheMaintenance::MultiplePtesUpdated((TLinAddr)(iTempPte+iTempMapColor), aPages*sizeof(TPte));
		}
	return iTempAddr+(iTempMapColor<<KPageShift);
	}

/**
Create a temporary mapping of one or more contiguous physical pages.
Memory attributes as specified by aMemType apply.
@See ArmMmu::MapTemp(TPhysAddr aPage,TLinAddr aLinAddr,TInt aPages) for other details.
*/
TLinAddr ArmMmu::MapTemp(TPhysAddr aPage,TLinAddr aLinAddr,TInt aPages, TMemoryType aMemType)
	{
	__ASSERT_MUTEX(RamAllocatorMutex);
	__ASSERT_DEBUG(!*iTempPte,MM::Panic(MM::ETempMappingAlreadyInUse));
	iTempMapColor = (aLinAddr>>KPageShift)&KPageColourMask;
	iTempMapCount = aPages;
	TUint pte = SP_PTE(KArmV6PermRWNO, aMemType, 0, 1);
	if (aPages==1)
		{
		iTempPte[iTempMapColor] = (aPage&~KPageMask) | SP_PTE(KArmV6PermRWNO, pte, 0, 1);
		CacheMaintenance::SinglePteUpdated((TLinAddr)(iTempPte+iTempMapColor));
		}
	else
		{
		__ASSERT_DEBUG(iTempMapColor+aPages<=KPageColourCount,MM::Panic(MM::ETempMappingNoRoom));
		for (TInt i=0; i<aPages; i++)
			iTempPte[iTempMapColor+i] = ((aPage&~KPageMask)+(i<<KPageShift)) | SP_PTE(KArmV6PermRWNO, pte, 0, 1);	
		CacheMaintenance::MultiplePtesUpdated((TLinAddr)(iTempPte+iTempMapColor), aPages*sizeof(TPte));
		}
	return iTempAddr+(iTempMapColor<<KPageShift);
	}

/**
Create a temporary mapping of one or more contiguous physical pages, distinct from
that created by MapTemp.
The RamAllocatorMutex must be held before this function is called and not released
until after UnmapSecondTemp has been called.

@param aPage	The physical address of the pages to be mapped.
@param aLinAddr The linear address of any existing location where the page is mapped.
				If the page isn't already mapped elsewhere as a cachable page then
				this value irrelevent. (It is used for page colouring.)
@param aPages	Number of pages to map.

@return The linear address of where the pages have been mapped.
*/
TLinAddr ArmMmu::MapSecondTemp(TPhysAddr aPage,TLinAddr aLinAddr,TInt aPages)
	{
	__ASSERT_MUTEX(RamAllocatorMutex);
	__ASSERT_DEBUG(!*iSecondTempPte,MM::Panic(MM::ETempMappingAlreadyInUse));
	iSecondTempMapColor = (aLinAddr>>KPageShift)&KPageColourMask;
	iSecondTempMapCount = aPages;
	if (aPages==1)
		{
		iSecondTempPte[iSecondTempMapColor] = (aPage&~KPageMask) | SP_PTE(KArmV6PermRWNO, KNormalCachedAttr, 0, 1);
		CacheMaintenance::SinglePteUpdated((TLinAddr)(iSecondTempPte+iSecondTempMapColor));
		}
	else
		{
		__ASSERT_DEBUG(iSecondTempMapColor+aPages<=KPageColourCount,MM::Panic(MM::ETempMappingNoRoom));
		for (TInt i=0; i<aPages; i++)
			iSecondTempPte[iSecondTempMapColor+i] = ((aPage&~KPageMask)+(i<<KPageShift)) | SP_PTE(KArmV6PermRWNO, KNormalCachedAttr, 0, 1);	
		CacheMaintenance::MultiplePtesUpdated((TLinAddr)(iSecondTempPte+iSecondTempMapColor), aPages*sizeof(TPte));
		}
	return iSecondTempAddr+(iSecondTempMapColor<<KPageShift);
	}

/**
Remove the temporary mapping created with MapTemp.
*/
void ArmMmu::UnmapTemp()
	{
	__ASSERT_MUTEX(RamAllocatorMutex);
	for (TInt i=0; i<iTempMapCount; i++)
		{
		iTempPte[iTempMapColor+i] = 0;
		CacheMaintenance::SinglePteUpdated((TLinAddr)(iTempPte+iTempMapColor+i));
		InvalidateTLBForPage(iTempAddr+((iTempMapColor+i)<<KPageShift), KERNEL_MAPPING);
		}
	}

/**
Remove the temporary mapping created with MapSecondTemp.
*/
void ArmMmu::UnmapSecondTemp()
	{
	__ASSERT_MUTEX(RamAllocatorMutex);
	for (TInt i=0; i<iSecondTempMapCount; i++)
		{
		iSecondTempPte[iSecondTempMapColor+i] = 0;
		CacheMaintenance::SinglePteUpdated((TLinAddr)(iSecondTempPte+iSecondTempMapColor+i));
		InvalidateTLBForPage(iSecondTempAddr+((iSecondTempMapColor+i)<<KPageShift), KERNEL_MAPPING);
		}
	}


TBool ArmMmu::ValidateLocalIpcAddress(TLinAddr aAddr,TInt aSize,TBool aWrite)
	{
	__NK_ASSERT_DEBUG(aSize<=KChunkSize);
	TLinAddr end = aAddr+aSize-1;
	if(end<aAddr)
		end = ~0u;

	if(TUint(aAddr^KIPCAlias)<TUint(KChunkSize) || TUint(end^KIPCAlias)<TUint(KChunkSize))
		{
		// local address is in alias region.
		// remove alias...
		NKern::LockSystem();
		((DMemModelThread*)TheCurrentThread)->RemoveAlias();
		NKern::UnlockSystem();
		// access memory, which will cause an exception...
		if(!(TUint(aAddr^KIPCAlias)<TUint(KChunkSize)))
			aAddr = end;
		InvalidateTLBForPage(aAddr,((DMemModelProcess*)TheCurrentThread->iOwningProcess)->iOsAsid);
		if(aWrite)
			*(volatile TUint8*)aAddr = 0;
		else
			aWrite = *(volatile TUint8*)aAddr;
		// can't get here
		__NK_ASSERT_DEBUG(0);
		}

	TUint32 local_mask;
	DMemModelProcess* process=(DMemModelProcess*)TheCurrentThread->iOwningProcess;
	if(aWrite)
		local_mask = process->iAddressCheckMaskW;
	else
		local_mask = process->iAddressCheckMaskR;
	TUint32 mask = 2<<(end>>27);
	mask -= 1<<(aAddr>>27);
	if((local_mask&mask)!=mask)
		return EFalse;

	if(!aWrite)
		return ETrue; // reads are ok

	// writes need further checking...
	TLinAddr userCodeStart = iUserCodeBase;
	TLinAddr userCodeEnd = userCodeStart+iMaxUserCodeSize;
	if(end>=userCodeStart && aAddr<userCodeEnd)
		return EFalse; // trying to write to user code area

	return ETrue;
	}

TInt DMemModelThread::Alias(TLinAddr aAddr, DMemModelProcess* aProcess, TInt aSize, TInt aPerm, TLinAddr& aAliasAddr, TInt& aAliasSize)
//
// Set up an alias mapping starting at address aAddr in specified process.
// Check permissions aPerm.
// Enter and return with system locked.
// Note: Alias is removed if an exception if trapped by DThread::IpcExcHandler.
//
	{
	__KTRACE_OPT(KMMU2,Kern::Printf("Thread %O Alias %08x+%x Process %O perm %x",this,aAddr,aSize,aProcess,aPerm));
	__ASSERT_SYSTEM_LOCK

	if(TUint(aAddr^KIPCAlias)<TUint(KChunkSize))
		return KErrBadDescriptor; // prevent access to alias region

	ArmMmu& m=::TheMmu;

	// check if memory is in region which is safe to access with supervisor permissions...
	TBool okForSupervisorAccess = aPerm&(EMapAttrReadSup|EMapAttrWriteSup) ? 1 : 0;
	if(!okForSupervisorAccess)
		{
		TInt shift = aAddr>>27;
		if(!(aPerm&EMapAttrWriteUser))
			{
			// reading with user permissions...
			okForSupervisorAccess = (aProcess->iAddressCheckMaskR>>shift)&1;
			}
		else
			{
			// writing with user permissions...
			okForSupervisorAccess = (aProcess->iAddressCheckMaskW>>shift)&1;
			if(okForSupervisorAccess)
				{
				// check for user code, because this is supervisor r/w and so
				// is not safe to write to access with supervisor permissions.
				if(TUint(aAddr-m.iUserCodeBase)<TUint(m.iMaxUserCodeSize))
					return KErrBadDescriptor; // prevent write to this...
				}
			}
		}

	TInt pdeIndex = aAddr>>KChunkShift;
	if(pdeIndex>=(m.iLocalPdSize>>2))
		{
		// address is in global section, don't bother aliasing it...
		if(iAliasLinAddr)
			RemoveAlias();
		aAliasAddr = aAddr;
		TInt maxSize = KChunkSize-(aAddr&KChunkMask);
		aAliasSize = aSize<maxSize ? aSize : maxSize;
		__KTRACE_OPT(KMMU2,Kern::Printf("DMemModelThread::Alias() abandoned as memory is globaly mapped"));
		return okForSupervisorAccess;
		}

	TInt asid = aProcess->iOsAsid;
	TPde* pd = PageDirectory(asid);
	TPde pde = pd[pdeIndex];
	if ((TPhysAddr)(pde&~KPageMask) == AliasRemapOld)
		pde = AliasRemapNew|(pde&KPageMask);
	pde = PDE_IN_DOMAIN(pde, KIPCAliasDomain);
	TLinAddr aliasAddr = KIPCAlias+(aAddr&(KChunkMask & ~KPageMask));
	if(pde==iAliasPde && iAliasLinAddr)
		{
		// pde already aliased, so just update linear address...
		iAliasLinAddr = aliasAddr;
		}
	else
		{
		// alias PDE changed...
		iAliasPde = pde;
		iAliasOsAsid = asid;
		if(!iAliasLinAddr)
			{
			ArmMmu::UnlockAlias();
			::TheMmu.iAliasList.Add(&iAliasLink); // add to list if not already aliased
			}
		iAliasLinAddr = aliasAddr;
		*iAliasPdePtr = pde;
		CacheMaintenance::SinglePteUpdated((TLinAddr)iAliasPdePtr);
		}

	__KTRACE_OPT(KMMU2,Kern::Printf("DMemModelThread::Alias() PDEntry=%x, iAliasLinAddr=%x",pde, aliasAddr));
	InvalidateTLBForPage(aliasAddr, ((DMemModelProcess*)iOwningProcess)->iOsAsid);
	TInt offset = aAddr&KPageMask;
	aAliasAddr = aliasAddr | offset;
	TInt maxSize = KPageSize - offset;
	aAliasSize = aSize<maxSize ? aSize : maxSize;
	iAliasTarget = aAddr & ~KPageMask;
	return okForSupervisorAccess;
	}

void DMemModelThread::RemoveAlias()
//
// Remove alias mapping (if present)
// Enter and return with system locked.
//
	{
	__KTRACE_OPT(KMMU2,Kern::Printf("Thread %O RemoveAlias", this));
	__ASSERT_SYSTEM_LOCK
	TLinAddr addr = iAliasLinAddr;
	if(addr)
		{
		ArmMmu::LockAlias();
		iAliasLinAddr = 0;
		iAliasPde = 0;
		*iAliasPdePtr = 0;
		CacheMaintenance::SinglePteUpdated((TLinAddr)iAliasPdePtr);
		InvalidateTLBForPage(addr, ((DMemModelProcess*)iOwningProcess)->iOsAsid);
		iAliasLink.Deque();
		}
	}

/*
 * Performs cache maintenance for physical page that is going to be reused.
 * Fully cached attributes are assumed. 
 */
void ArmMmu::CacheMaintenanceOnDecommit(TPhysAddr a)
	{
	// purge a single page from the cache following decommit
	ArmMmu& m=::TheMmu;
	TInt colour = SPageInfo::FromPhysAddr(a)->Offset()&KPageColourMask;
	TPte& pte=m.iTempPte[colour];
	TLinAddr va=m.iTempAddr+(colour<<KPageShift);
	pte=a|SP_PTE(KArmV6PermRWNO, iCacheMaintenanceTempMapAttr, 1, 1);
	CacheMaintenance::SinglePteUpdated((TLinAddr)&pte);

	CacheMaintenance::PageToReuse(va,EMemAttNormalCached, a);

	pte=0;
	CacheMaintenance::SinglePteUpdated((TLinAddr)&pte);
	InvalidateTLBForPage(va,KERNEL_MAPPING);
	}

void ArmMmu::CacheMaintenanceOnDecommit(const TPhysAddr* al, TInt n)
	{
	// purge a list of pages from the cache following decommit
	while (--n>=0)
		ArmMmu::CacheMaintenanceOnDecommit(*al++);
	}

/*
 * Performs cache maintenance to preserve physical page that is going to be reused. 
 */
void ArmMmu::CacheMaintenanceOnPreserve(TPhysAddr a, TUint aMapAttr)
	{
	// purge a single page from the cache following decommit
	ArmMmu& m=::TheMmu;
	TInt colour = SPageInfo::FromPhysAddr(a)->Offset()&KPageColourMask;
	TPte& pte=m.iTempPte[colour];
	TLinAddr va=m.iTempAddr+(colour<<KPageShift);
	pte=a|SP_PTE(KArmV6PermRWNO, iCacheMaintenanceTempMapAttr, 1, 1);
	CacheMaintenance::SinglePteUpdated((TLinAddr)&pte);

	CacheMaintenance::MemoryToPreserveAndReuse(va, KPageSize,aMapAttr);

	pte=0;
	CacheMaintenance::SinglePteUpdated((TLinAddr)&pte);
	InvalidateTLBForPage(va,KERNEL_MAPPING);
	}

void ArmMmu::CacheMaintenanceOnPreserve(const TPhysAddr* al, TInt n, TUint aMapAttr)
	{
	// purge a list of pages from the cache following decommit
	while (--n>=0)
		ArmMmu::CacheMaintenanceOnPreserve(*al++, aMapAttr);
	}

/*
 * Performs cache maintenance of physical memory that has been decommited and has to be preserved.
 * Call this method for physical pages with no page info updated (or no page info at all).
 * @arg aPhysAddr	The address of contiguous physical memory to be preserved.
 * @arg aSize		The size of the region
 * @arg aLinAddr 	Former linear address of the region. As said above, the physical memory is
 * 					already remapped from this linear address.
 * @arg aMapAttr 	Mapping attributes of the region when it was mapped in aLinAddr.
 * @pre MMU mutex is held.  
 */
void ArmMmu::CacheMaintenanceOnPreserve(TPhysAddr aPhysAddr, TInt aSize, TLinAddr aLinAddr, TUint aMapAttr )
	{
	__NK_ASSERT_DEBUG((aPhysAddr&KPageMask)==0);
	__NK_ASSERT_DEBUG((aSize&KPageMask)==0);
	__NK_ASSERT_DEBUG((aLinAddr&KPageMask)==0);

	TPhysAddr pa = aPhysAddr;
	TInt size = aSize;
	TInt colour = (aLinAddr>>KPageShift)&KPageColourMask;
	TPte* pte = &(iTempPte[colour]);
	while (size)
		{
		pte=&(iTempPte[colour]);
		TLinAddr va=iTempAddr+(colour<<KPageShift);
		*pte=pa|SP_PTE(KArmV6PermRWNO, iCacheMaintenanceTempMapAttr, 1, 1);
		CacheMaintenance::SinglePteUpdated((TLinAddr)pte);
		CacheMaintenance::MemoryToPreserveAndReuse(va, KPageSize,aMapAttr);

		*pte=0;
		CacheMaintenance::SinglePteUpdated((TLinAddr)pte);
		InvalidateTLBForPage(va,KERNEL_MAPPING);

		colour = (colour+1)&KPageColourMask;
		pa += KPageSize;
		size -=KPageSize;
		}
	}

TInt ArmMmu::UnlockRamCachePages(TLinAddr aLinAddr, TInt aNumPages, DProcess* aProcess)
	{
	TInt asid = ((DMemModelProcess*)aProcess)->iOsAsid;
	TInt page = aLinAddr>>KPageShift;
	NKern::LockSystem();
	for(;;)
		{
		TPde* pd = PageDirectory(asid)+(page>>(KChunkShift-KPageShift));
		TPte* pt = SafePageTableFromPde(*pd++);
		TInt pteIndex = page&(KChunkMask>>KPageShift);
		if(!pt)
			{
			// whole page table has gone, so skip all pages in it...
			TInt pagesInPt = (KChunkSize>>KPageShift)-pteIndex;
			aNumPages -= pagesInPt;
			page += pagesInPt;
			if(aNumPages>0)
				continue;
			NKern::UnlockSystem();
			return KErrNone;
			}
		pt += pteIndex;
		do
			{
			TInt pagesInPt = (KChunkSize>>KPageShift)-pteIndex;
			if(pagesInPt>aNumPages)
				pagesInPt = aNumPages;
			if(pagesInPt>KMaxPages)
				pagesInPt = KMaxPages;

			aNumPages -= pagesInPt;
			page += pagesInPt;

			do
				{
				TPte pte = *pt++;
				if(pte) // pte may be null if page has already been unlocked and reclaimed by system
					iRamCache->DonateRamCachePage(SPageInfo::FromPhysAddr(pte));
				}
			while(--pagesInPt);

			if(!aNumPages)
				{
				NKern::UnlockSystem();
				return KErrNone;
				}

			pteIndex = page&(KChunkMask>>KPageShift);
			}
		while(!NKern::FlashSystem() && pteIndex);
		}
	}


TInt ArmMmu::LockRamCachePages(TLinAddr aLinAddr, TInt aNumPages, DProcess* aProcess)
	{
	TInt asid = ((DMemModelProcess*)aProcess)->iOsAsid;
	TInt page = aLinAddr>>KPageShift;
	NKern::LockSystem();
	for(;;)
		{
		TPde* pd = PageDirectory(asid)+(page>>(KChunkShift-KPageShift));
		TPte* pt = SafePageTableFromPde(*pd++);
		TInt pteIndex = page&(KChunkMask>>KPageShift);
		if(!pt)
			goto not_found;
		pt += pteIndex;
		do
			{
			TInt pagesInPt = (KChunkSize>>KPageShift)-pteIndex;
			if(pagesInPt>aNumPages)
				pagesInPt = aNumPages;
			if(pagesInPt>KMaxPages)
				pagesInPt = KMaxPages;

			aNumPages -= pagesInPt;
			page += pagesInPt;

			do
				{
				TPte pte = *pt++;
				if(pte==0)
					goto not_found;
				if(!iRamCache->ReclaimRamCachePage(SPageInfo::FromPhysAddr(pte)))
					goto not_found;
				}
			while(--pagesInPt);

			if(!aNumPages)
				{
				NKern::UnlockSystem();
				return KErrNone;
				}

			pteIndex = page&(KChunkMask>>KPageShift);
			}
		while(!NKern::FlashSystem() && pteIndex);
		}
not_found:
	NKern::UnlockSystem();
	return KErrNotFound;
	}


void RamCache::SetFree(SPageInfo* aPageInfo)
	{
	ArmMmu& m=::TheMmu;
	// Make a page free
	SPageInfo::TType type = aPageInfo->Type();
	if(type==SPageInfo::EPagedCache)
		{
		TInt offset = aPageInfo->Offset()<<KPageShift;
		DMemModelChunk* chunk = (DMemModelChunk*)aPageInfo->Owner();
		__NK_ASSERT_DEBUG(TUint(offset)<TUint(chunk->iMaxSize));
		TLinAddr lin = ((TLinAddr)chunk->iBase)+offset;
		TInt asid = ((DMemModelProcess*)chunk->iOwningProcess)->iOsAsid;
		TPte* pt = PtePtrFromLinAddr(lin,asid);
		TPhysAddr phys = (*pt)&~KPageMask;
		*pt = KPteNotPresentEntry;
		CacheMaintenance::SinglePteUpdated((TLinAddr)pt);
		InvalidateTLBForPage(lin,asid);
		m.CacheMaintenanceOnDecommit(phys);

		// actually decommit it from chunk...
		TInt ptid = ((TLinAddr)pt-KPageTableBase)>>KPageTableShift;
		SPageTableInfo& ptinfo=((ArmMmu*)iMmu)->iPtInfo[ptid];
		if(!--ptinfo.iCount)
			{
			chunk->iPageTables[offset>>KChunkShift] = 0xffff;
			NKern::UnlockSystem();
			((ArmMmu*)iMmu)->DoUnassignPageTable(lin, (TAny*)asid);
			((ArmMmu*)iMmu)->FreePageTable(ptid);
			NKern::LockSystem();
			}
		}
	else
		{
		__KTRACE_OPT2(KPAGING,KPANIC,Kern::Printf("DP: SetFree() with bad page type = %d",aPageInfo->Type()));
		Panic(EUnexpectedPageType);
		}
	}


//
// MemModelDemandPaging
//

class MemModelDemandPaging : public DemandPaging
	{
public:
	// From RamCacheBase
	virtual void Init2();
	virtual TInt Init3();
	virtual TBool PageUnmapped(SPageInfo* aPageInfo);
	// From DemandPaging
	virtual TInt Fault(TAny* aExceptionInfo);
	virtual void SetOld(SPageInfo* aPageInfo);
	virtual void SetFree(SPageInfo* aPageInfo);
	virtual void NotifyPageFree(TPhysAddr aPage);
	virtual TInt EnsurePagePresent(TLinAddr aPage, DProcess* aProcess);
	virtual TPhysAddr LinearToPhysical(TLinAddr aPage, DProcess* aProcess);
	virtual void AllocLoadAddress(DPagingRequest& aReq, TInt aDeviceId);
	virtual TInt PageState(TLinAddr aAddr);
	virtual TBool NeedsMutexOrderCheck(TLinAddr aStartAddr, TUint aLength);
	// New
	inline ArmMmu& Mmu() { return (ArmMmu&)*iMmu; }
	void InitRomPaging();
	void InitCodePaging();
	TInt HandleFault(TArmExcInfo& aExc, TLinAddr aFaultAddress, TInt aAsid);
	TInt PageIn(TLinAddr aAddress, TInt aAsid, DMemModelCodeSegMemory* aCodeSegMemory);
public:
	// use of the folowing members is protected by the system lock..
	TPte* iPurgePte;			// PTE used for temporary mappings during cache purge operations
	TLinAddr iPurgeAddr;		// address corresponding to iPurgePte
	};

extern void MakeGlobalPTEInaccessible(TPte* aPtePtr, TPte aNewPte, TLinAddr aLinAddr);
extern void MakePTEInaccessible(TPte* aPtePtr, TPte aNewPte, TLinAddr aLinAddr, TInt aAsid);

//
// MemModelDemandPaging
//


DemandPaging* DemandPaging::New()
	{
	return new MemModelDemandPaging();
	}


void MemModelDemandPaging::Init2()
	{
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf(">MemModelDemandPaging::Init2"));
	DemandPaging::Init2();

	iPurgeAddr = KDemandPagingTempAddr;
	iPurgePte = PtePtrFromLinAddr(iPurgeAddr);

	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("<MemModelDemandPaging::Init2"));
	}


void MemModelDemandPaging::AllocLoadAddress(DPagingRequest& aReq, TInt aReqId)
	{
	aReq.iLoadAddr = iTempPages + aReqId * KPageSize * KPageColourCount;
	aReq.iLoadPte = PtePtrFromLinAddr(aReq.iLoadAddr);
	}


TInt MemModelDemandPaging::Init3()
	{
	TInt r=DemandPaging::Init3();
	if(r!=KErrNone)
		return r;
	
	// Create a region for mapping pages during page in
	DPlatChunkHw* chunk;
	TInt chunkSize = (KMaxPagingDevices * KPagingRequestsPerDevice + 1) * KPageColourCount * KPageSize;
	DPlatChunkHw::DoNew(chunk, KPhysAddrInvalid, chunkSize, EMapAttrSupRw|EMapAttrFullyBlocking);
	if(!chunk)
		Panic(EInitialiseFailed);
	TInt colourMask = KPageColourMask << KPageShift;
	iTempPages = (chunk->iLinAddr + colourMask) & ~colourMask;

	if(RomPagingRequested())
		InitRomPaging();

	if (CodePagingRequested())
		InitCodePaging();

	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("<MemModelDemandPaging::Init3"));
	return KErrNone;
	}
	
void MemModelDemandPaging::InitRomPaging()
	{
	// Make page tables for demand paged part of ROM...
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("MemModelDemandPaging::Init3 making page tables for paged ROM"));
	TLinAddr lin = iRomPagedLinearBase&~KChunkMask; // first chunk with paged ROM in
	TLinAddr linEnd = iRomLinearBase+iRomSize;
	while(lin<linEnd)
		{
		// Get a Page Table
		TInt ptid = Mmu().PageTableId(lin,0);
		if(ptid<0)
			{
			MmuBase::Wait();
			ptid = Mmu().AllocPageTable();
			MmuBase::Signal();
			__NK_ASSERT_DEBUG(ptid>=0);
			Mmu().PtInfo(ptid).SetGlobal(lin >> KChunkShift);
			}

		// Get new page table addresses
		TPte* pt = PageTable(ptid);
		TPhysAddr ptPhys=Mmu().LinearToPhysical((TLinAddr)pt,0);

		// Pointer to page directory entry
		TPde* ppde = ::InitPageDirectory + (lin>>KChunkShift);

		// Fill in Page Table
		TPte* ptEnd = pt+(1<<(KChunkShift-KPageShift));
		pt += (lin&KChunkMask)>>KPageShift;
		TLinAddr firstPte = (TLinAddr)pt; // Will need this to clean page table memory region from cache

		do
			{
			if(lin<iRomPagedLinearBase)
				*pt++ = Mmu().LinearToPhysical(lin,0) | KRomPtePerm;
			else
				{
				MakeGlobalPTEInaccessible(pt, KPteNotPresentEntry, lin);
				++pt;
				}
			lin += KPageSize;
			}
		while(pt<ptEnd && lin<=linEnd);

		CacheMaintenance::MultiplePtesUpdated((TLinAddr)firstPte, (TUint)pt-firstPte);

		// Add new Page Table to the Page Directory
		TPde newpde = ptPhys | KShadowPdePerm;
		__KTRACE_OPT2(KPAGING,KMMU,Kern::Printf("Writing PDE %08x to %08x", newpde, ppde));
		TInt irq=NKern::DisableAllInterrupts();
		*ppde = newpde;
		CacheMaintenance::SinglePteUpdated((TLinAddr)ppde);
		FlushTLBs();
		NKern::RestoreInterrupts(irq);
		}
	}


void MemModelDemandPaging::InitCodePaging()
	{
	// Initialise code paging info
	iCodeLinearBase = Mmu().iUserCodeBase;
	iCodeSize = Mmu().iMaxUserCodeSize;
	}


/**
@return ETrue when the unmapped page should be freed, EFalse otherwise
*/
TBool MemModelDemandPaging::PageUnmapped(SPageInfo* aPageInfo)
	{
	SPageInfo::TType type = aPageInfo->Type();

	// Only have to deal with cache pages - pages containg code don't get returned to the system
	// when they are decommitted from an individual process, only when the code segment is destroyed	
	if(type!=SPageInfo::EPagedCache)
		{
		__NK_ASSERT_DEBUG(type!=SPageInfo::EPagedCode); // shouldn't happen
		__NK_ASSERT_DEBUG(type!=SPageInfo::EPagedData); // not supported yet
		return ETrue;
		}

	RemovePage(aPageInfo);
	AddAsFreePage(aPageInfo);
	// Return false to stop DMemModelChunk::DoDecommit from freeing this page
	return EFalse; 
	}


void DoSetCodeOld(SPageInfo* aPageInfo, DMemModelCodeSegMemory* aCodeSegMemory, TLinAddr aLinAddr)
	{
	NThread* currentThread = NKern::CurrentThread(); 
	aPageInfo->SetModifier(currentThread);
	// scan all address spaces...
	TInt asid = -1;
	TInt lastAsid = KArmV6NumAsids-1;
	TUint32* ptr = aCodeSegMemory->iOsAsids->iMap;
	do
		{
		TUint32 bits = *ptr++;
		do
			{
			++asid;
			if(bits&0x80000000u)
				{
				// codeseg is mapped in this address space, so update PTE...
				TPte* pt = PtePtrFromLinAddr(aLinAddr,asid);
				TPte pte = *pt;
				if(pte&KPtePresentMask)
					{
					__NK_ASSERT_DEBUG((pte&~KPageMask) == aPageInfo->PhysAddr());
					MakePTEInaccessible(pt, pte&~KPtePresentMask, aLinAddr, asid);
					}
				}
			}
		while(bits<<=1);
		if(NKern::FlashSystem() && aPageInfo->CheckModified(currentThread))
			return; // page was modified by another thread
		asid |= 31;
		}
	while(asid<lastAsid);
	}


void MemModelDemandPaging::SetOld(SPageInfo* aPageInfo)
	{
	__ASSERT_SYSTEM_LOCK;
	__NK_ASSERT_DEBUG(aPageInfo->State() == SPageInfo::EStatePagedOld);

	SPageInfo::TType type = aPageInfo->Type();

	if(type==SPageInfo::EPagedROM)
		{
		// get linear address of page...
		TInt offset = aPageInfo->Offset()<<KPageShift;
		__NK_ASSERT_DEBUG(TUint(offset)<iRomSize);

		// make page inaccessible...
		TLinAddr lin = iRomLinearBase+offset;
		TPte* pt = PtePtrFromLinAddr(lin);
		MakeGlobalPTEInaccessible(pt, *pt&~KPtePresentMask, lin);
		}
	else if(type==SPageInfo::EPagedCode)
		{
		START_PAGING_BENCHMARK;

		// get linear address of page...
		TInt offset = aPageInfo->Offset()<<KPageShift;
		__NK_ASSERT_DEBUG(TUint(offset)<iCodeSize);
		TLinAddr lin = iCodeLinearBase+offset;
			
		// get CodeSegMemory...
		DMemModelCodeSegMemory* codeSegMemory = (DMemModelCodeSegMemory*)aPageInfo->Owner();
		__NK_ASSERT_DEBUG(codeSegMemory && codeSegMemory->iPages && codeSegMemory->iIsDemandPaged);

#ifdef _DEBUG
		TInt pageNumber = (lin - codeSegMemory->iRamInfo.iCodeRunAddr) >> KPageShift;
		__NK_ASSERT_DEBUG(codeSegMemory->iPages[pageNumber] == aPageInfo->PhysAddr());
#endif

		// make page inaccessible...
		DoSetCodeOld(aPageInfo,codeSegMemory,lin);
		
		END_PAGING_BENCHMARK(this, EPagingBmSetCodePageOld);
		}
	else if(type==SPageInfo::EPagedCache)
		{
		// leave page accessible
		}
	else if(type!=SPageInfo::EPagedFree)
		{
		__KTRACE_OPT2(KPAGING,KPANIC,Kern::Printf("DP: SetOld() with bad page type = %d",aPageInfo->Type()));
		Panic(EUnexpectedPageType);
		}
	NKern::FlashSystem();
	}


void DoSetCodeFree(SPageInfo* aPageInfo, TPhysAddr aPhysAddr, DMemModelCodeSegMemory* aCodeSegMemory, TLinAddr aLinAddr)
	{
	NThread* currentThread = NKern::CurrentThread();
	aPageInfo->SetModifier(currentThread);
	// scan all address spaces...
	TInt asid = -1;
	TInt lastAsid = KArmV6NumAsids-1;
	TUint32* ptr = aCodeSegMemory->iOsAsids->iMap;
	do
		{
		TUint32 bits = *ptr++;
		do
			{
			++asid;
			if(bits&0x80000000u)
				{
				// codeseg is mapped in this address space, so update PTE...
				TPte* pt = PtePtrFromLinAddr(aLinAddr,asid);
				TPte pte = *pt;
				if (pte!=KPteNotPresentEntry && (pte&~KPageMask) == aPhysAddr)
					MakePTEInaccessible(pt, KPteNotPresentEntry, aLinAddr, asid);
				}
			}
		while(bits<<=1);
		if(NKern::FlashSystem())
			{
			// nobody else should modify page!
			__NK_ASSERT_DEBUG(!aPageInfo->CheckModified(currentThread));
			}
		asid |= 31;
		}
	while(asid<lastAsid);
	}


void MemModelDemandPaging::SetFree(SPageInfo* aPageInfo)
	{
	__ASSERT_SYSTEM_LOCK;
	__ASSERT_MUTEX(MmuBase::RamAllocatorMutex);
	__NK_ASSERT_DEBUG(aPageInfo->State() == SPageInfo::EStatePagedDead);
	if(aPageInfo->LockCount())
		Panic(ERamPageLocked);

	SPageInfo::TType type = aPageInfo->Type();
	TPhysAddr phys = aPageInfo->PhysAddr();

	if(type==SPageInfo::EPagedROM)
		{
		// get linear address of page...
		TInt offset = aPageInfo->Offset()<<KPageShift;
		__NK_ASSERT_DEBUG(TUint(offset)<iRomSize);
		TLinAddr lin = iRomLinearBase+offset;

		// unmap it...
		TPte* pt = PtePtrFromLinAddr(lin);
		MakeGlobalPTEInaccessible(pt, KPteNotPresentEntry, lin);

#ifdef BTRACE_PAGING
		BTraceContext8(BTrace::EPaging,BTrace::EPagingPageOutROM,phys,lin);
#endif
		}
	else if(type==SPageInfo::EPagedCode)
		{
		START_PAGING_BENCHMARK;
		
		// get linear address of page...
		TInt offset = aPageInfo->Offset()<<KPageShift;
		__NK_ASSERT_DEBUG(TUint(offset)<iCodeSize);
		TLinAddr lin = iCodeLinearBase+offset;

		// get CodeSegMemory...
		// NOTE, this cannot die because we hold the RamAlloc mutex, and the CodeSegMemory
		// destructor also needs this mutex to do it's cleanup...
		DMemModelCodeSegMemory* codeSegMemory = (DMemModelCodeSegMemory*)aPageInfo->Owner();
		__NK_ASSERT_DEBUG(codeSegMemory && codeSegMemory->iPages && codeSegMemory->iIsDemandPaged);

		// remove page from CodeSegMemory (must come before System Lock is released)...
		TInt pageNumber = (lin - codeSegMemory->iRamInfo.iCodeRunAddr) >> KPageShift;
		__NK_ASSERT_DEBUG(codeSegMemory->iPages[pageNumber] == aPageInfo->PhysAddr());
		codeSegMemory->iPages[pageNumber] = KPhysAddrInvalid;
		
		// unmap page from all processes it's mapped into...
		DoSetCodeFree(aPageInfo,phys,codeSegMemory,lin);

		END_PAGING_BENCHMARK(this, EPagingBmSetCodePageFree);
#ifdef BTRACE_PAGING
		BTraceContext8(BTrace::EPaging,BTrace::EPagingPageOutCode,phys,lin);
#endif
		}
	else if(type==SPageInfo::EPagedCache)
		{
		// get linear address of page...
		TInt offset = aPageInfo->Offset()<<KPageShift;
		DMemModelChunk* chunk = (DMemModelChunk*)aPageInfo->Owner();
		__NK_ASSERT_DEBUG(TUint(offset)<TUint(chunk->iMaxSize));
		TLinAddr lin = ((TLinAddr)chunk->iBase)+offset;

		// unmap it...
		TInt asid = ((DMemModelProcess*)chunk->iOwningProcess)->iOsAsid;
		TPte* pt = PtePtrFromLinAddr(lin,asid);
		*pt = KPteNotPresentEntry;
		CacheMaintenance::SinglePteUpdated((TLinAddr)pt);

		InvalidateTLBForPage(lin,asid);

		// actually decommit it from chunk...
		TInt ptid = ((TLinAddr)pt-KPageTableBase)>>KPageTableShift;
		SPageTableInfo& ptinfo=Mmu().iPtInfo[ptid];
		if(!--ptinfo.iCount)
			{
			chunk->iPageTables[offset>>KChunkShift] = 0xffff;
			NKern::UnlockSystem();
			Mmu().DoUnassignPageTable(lin, (TAny*)asid);
			Mmu().FreePageTable(ptid);
			NKern::LockSystem();
			}

#ifdef BTRACE_PAGING
		BTraceContext8(BTrace::EPaging,BTrace::EPagingPageOutCache,phys,lin);
#endif
		}
	else if(type==SPageInfo::EPagedFree)
		{
		// already free...
#ifdef BTRACE_PAGING
		BTraceContext4(BTrace::EPaging,BTrace::EPagingPageOutFree,phys);
#endif
		// fall through to cache purge code because cache may not have been
		// cleaned for this page if PageUnmapped called
		}
	else
		{
		__KTRACE_OPT2(KPAGING,KPANIC,Kern::Printf("DP: SetFree() with bad page type = %d",aPageInfo->Type()));
		Panic(EUnexpectedPageType);
		return;
		}

	NKern::UnlockSystem();

	// purge cache for page...
	TInt colour = aPageInfo->Offset()&KPageColourMask;
	TPte& pte=iPurgePte[colour];
	TLinAddr va=iPurgeAddr+(colour<<KPageShift);
	pte=phys|SP_PTE(KArmV6PermRWNO, TheMmu.iCacheMaintenanceTempMapAttr, 1, 1);
	CacheMaintenance::SinglePteUpdated((TLinAddr)&pte);

	CacheMaintenance::PageToReuse(va,EMemAttNormalCached, KPhysAddrInvalid);

	pte=0;
	CacheMaintenance::SinglePteUpdated((TLinAddr)&pte);
	InvalidateTLBForPage(va,KERNEL_MAPPING);

	NKern::LockSystem();
	}


void MemModelDemandPaging::NotifyPageFree(TPhysAddr aPage)
	{
	__KTRACE_OPT(KPAGING, Kern::Printf("MemModelDemandPaging::NotifyPageFree %08x", aPage));
	__ASSERT_SYSTEM_LOCK;

	SPageInfo* pageInfo = SPageInfo::FromPhysAddr(aPage);
	__ASSERT_DEBUG(pageInfo->Type()==SPageInfo::EPagedCode, MM::Panic(MM::EUnexpectedPageType));
	RemovePage(pageInfo);
	SetFree(pageInfo);
	AddAsFreePage(pageInfo);
	}


TInt MemModelDemandPaging::Fault(TAny* aExceptionInfo)
	{
	TArmExcInfo& exc=*(TArmExcInfo*)aExceptionInfo;

	// Get faulting address
	TLinAddr faultAddress = exc.iFaultAddress;
	if(exc.iExcCode==EArmExceptionDataAbort)
		{
		// Let writes take an exception rather than page in any memory...
		if(exc.iFaultStatus&(1<<11))
			return KErrUnknown;
		}
	else if (exc.iExcCode != EArmExceptionPrefetchAbort)
		return KErrUnknown; // Not prefetch or data abort
	
	// Only handle page translation faults
	if((exc.iFaultStatus & 0x40f) != 0x7)
		return KErrUnknown;

	DMemModelThread* thread = (DMemModelThread*)TheCurrentThread;

	// check which ragion fault occured in...
	TInt asid = 0; // asid != 0 => code paging fault
	if(TUint(faultAddress-iRomPagedLinearBase)<iRomPagedSize)
		{
		// in ROM
		}
	else if(TUint(faultAddress-iCodeLinearBase)<iCodeSize)
		{
		// in code
		asid = ((DMemModelProcess*)TheScheduler.iAddressSpace)->iOsAsid;
		}
	else if (thread->iAliasLinAddr && TUint(faultAddress - thread->iAliasLinAddr) < TUint(KPageSize))
		{
		// in aliased memory
		faultAddress = (faultAddress - thread->iAliasLinAddr) + thread->iAliasTarget;
		if(TUint(faultAddress-iCodeLinearBase)>=iCodeSize)
			return KErrUnknown; // not in alias of code
		asid = thread->iAliasOsAsid;
		__NK_ASSERT_DEBUG(asid != 0);
		}
	else
		return KErrUnknown; // Not in pageable region

	// Check if thread holds fast mutex and claim system lock
	NFastMutex* fm = NKern::HeldFastMutex();
	TPagingExcTrap* trap = thread->iPagingExcTrap;
	if(!fm)
		NKern::LockSystem();
	else
		{
		if(!trap || fm!=&TheScheduler.iLock)
			{
			__KTRACE_OPT2(KPAGING,KPANIC,Kern::Printf("DP: Fault with FM Held! %x (%O pc=%x)",faultAddress,&Kern::CurrentThread(),exc.iR15));
			Panic(EPageFaultWhilstFMHeld); // Not allowed to hold mutexes
			}
		// restore address space on multiple memory model (because the trap will
		// bypass any code which would have done this.)...
		DMemModelThread::RestoreAddressSpace();

		// Current thread already has the system lock...
		NKern::FlashSystem(); // Let someone else have a go with the system lock.
		}

	// System locked here

	TInt r = KErrNone;	
	if(thread->IsRealtime())
		r = CheckRealtimeThreadFault(thread, aExceptionInfo);
	if (r == KErrNone)
		r = HandleFault(exc, faultAddress, asid);
	
	// Restore system lock state
	if (fm != NKern::HeldFastMutex())
		{
		if (fm)
			NKern::LockSystem();
		else
			NKern::UnlockSystem();
		}
	
	// Deal with XTRAP_PAGING
	if(r == KErrNone && trap)
		{
		trap->Exception(1); // Return from exception trap with result '1' (value>0)
		// code doesn't continue beyond this point.
		}

	return r;
	}



TInt MemModelDemandPaging::HandleFault(TArmExcInfo& aExc, TLinAddr aFaultAddress, TInt aAsid)
	{
	++iEventInfo.iPageFaultCount;

	// get page table entry...
	TPte* pt = SafePtePtrFromLinAddr(aFaultAddress, aAsid);
	if(!pt)
		return KErrNotFound;
	TPte pte = *pt;

	// Do what is required to make page accessible...

	if(pte&KPtePresentMask)
		{
		// PTE is present, so assume it has already been dealt with
#ifdef BTRACE_PAGING
		BTraceContext12(BTrace::EPaging,BTrace::EPagingPageNop,pte&~KPageMask,aFaultAddress,aExc.iR15);
#endif
		return KErrNone;
		}

	if(pte!=KPteNotPresentEntry)
		{
		// PTE alread has a page
		SPageInfo* pageInfo = SPageInfo::FromPhysAddr(pte);
		if(pageInfo->State()==SPageInfo::EStatePagedDead)
			{
			// page currently being unmapped, so do that here...
			MakePTEInaccessible(pt, KPteNotPresentEntry, aFaultAddress, aAsid);
			}
		else
			{
			// page just needs making young again...
			*pt = TPte(pte|KArmV6PteSmallPage); // Update page table
			CacheMaintenance::SinglePteUpdated((TLinAddr)pt);
			Rejuvenate(pageInfo);
#ifdef BTRACE_PAGING
			BTraceContext12(BTrace::EPaging,BTrace::EPagingRejuvenate,pte&~KPageMask,aFaultAddress,aExc.iR15);
#endif
			return KErrNone;
			}
		}

	// PTE not present, so page it in...
	// check if fault in a CodeSeg...
	DMemModelCodeSegMemory* codeSegMemory = NULL;
	if (!aAsid)
		NKern::ThreadEnterCS();
	else
		{
		// find CodeSeg...
		DMemModelCodeSeg* codeSeg = (DMemModelCodeSeg*)DCodeSeg::CodeSegsByAddress.Find(aFaultAddress);
		if (!codeSeg)
			return KErrNotFound;
		codeSegMemory = codeSeg->Memory();
		if (codeSegMemory==0 || !codeSegMemory->iIsDemandPaged || codeSegMemory->iOsAsids->NotFree(aAsid, 1))
			return KErrNotFound;
	
		// check if it's paged in but not yet mapped into this process...			
		TInt pageNumber = (aFaultAddress - codeSegMemory->iRamInfo.iCodeRunAddr) >> KPageShift;
		TPhysAddr page = codeSegMemory->iPages[pageNumber];
		if (page != KPhysAddrInvalid)
			{
			// map it into this process...
			SPageInfo* pageInfo = SPageInfo::FromPhysAddr(page);
			__NK_ASSERT_DEBUG(pageInfo->State()!=SPageInfo::EStatePagedDead);
			*pt = page | (codeSegMemory->iCreator ? KUserCodeLoadPte : KUserCodeRunPte);
			CacheMaintenance::SinglePteUpdated((TLinAddr)pt);
			Rejuvenate(pageInfo);
#ifdef BTRACE_PAGING
			BTraceContext8(BTrace::EPaging,BTrace::EPagingMapCode,page,aFaultAddress);
#endif
			return KErrNone;
			}

		// open reference on CodeSegMemory
		NKern::ThreadEnterCS();
#ifdef _DEBUG
		TInt r = 
#endif
				 codeSegMemory->Open();
		__NK_ASSERT_DEBUG(r==KErrNone);
		NKern::FlashSystem();
		}		

#ifdef BTRACE_PAGING
	BTraceContext8(BTrace::EPaging,BTrace::EPagingPageInBegin,aFaultAddress,aExc.iR15);
#endif
	TInt r = PageIn(aFaultAddress, aAsid, codeSegMemory);

	NKern::UnlockSystem();

	if(codeSegMemory)
		codeSegMemory->Close();

	NKern::ThreadLeaveCS();
	
	return r;
	}


TInt MemModelDemandPaging::PageIn(TLinAddr aAddress, TInt aAsid, DMemModelCodeSegMemory* aCodeSegMemory)
	{
	// Get a request object - this may block until one is available
	DPagingRequest* req = AcquireRequestObject();
	
	// Get page table entry
	TPte* pt = SafePtePtrFromLinAddr(aAddress, aAsid);

	// Check page is still required...
	if(!pt || *pt!=KPteNotPresentEntry)
		{
#ifdef BTRACE_PAGING
		BTraceContext0(BTrace::EPaging,BTrace::EPagingPageInUnneeded);
#endif
		ReleaseRequestObject(req);
		return pt ? KErrNone : KErrNotFound;
		}

	++iEventInfo.iPageInReadCount;

	// Get a free page
	SPageInfo* pageInfo = AllocateNewPage();
	__NK_ASSERT_DEBUG(pageInfo);

	// Get physical address of free page
	TPhysAddr phys = pageInfo->PhysAddr();
	__NK_ASSERT_DEBUG(phys!=KPhysAddrInvalid);

	// Temporarily map free page
	TInt colour = (aAddress>>KPageShift)&KPageColourMask;
	__NK_ASSERT_DEBUG((req->iLoadAddr & (KPageColourMask << KPageShift)) == 0);
	req->iLoadAddr |= colour << KPageShift;
	TLinAddr loadAddr = req->iLoadAddr;
	pt = req->iLoadPte+colour;
//	*pt = phys | SP_PTE(KArmV6PermRWNO, KArmV6MemAttWTWAWTWA, 0, 1);
	*pt = phys | SP_PTE(KArmV6PermRWNO, KNormalUncachedAttr, 0, 1);
	CacheMaintenance::SinglePteUpdated((TLinAddr)pt);

	// Read page from backing store
	aAddress &= ~KPageMask;	
	NKern::UnlockSystem();

	TInt r;
	if (!aCodeSegMemory)
		r = ReadRomPage(req, aAddress);
	else
		{
		r = ReadCodePage(req, aCodeSegMemory, aAddress);
		if (r == KErrNone)
			aCodeSegMemory->ApplyCodeFixups((TUint32*)loadAddr, aAddress);
		}
	if(r!=KErrNone)
		Panic(EPageInFailed);
	
	// make caches consistant...
//	Cache::IMB_Range(loadAddr, KPageSize);
	*pt = phys | SP_PTE(KArmV6PermRWNO, KNormalCachedAttr, 0, 1);
	CacheMaintenance::SinglePteUpdated((TLinAddr)pt);
	InvalidateTLBForPage(loadAddr,KERNEL_MAPPING);
	CacheMaintenance::CodeChanged(loadAddr, KPageSize, CacheMaintenance::ECPUUncached);

	NKern::LockSystem();

	// Invalidate temporary mapping
	MakeGlobalPTEInaccessible(pt, KPteNotPresentEntry, loadAddr);

	// Release request object now we're finished with it
	req->iLoadAddr &= ~(KPageColourMask << KPageShift);
	ReleaseRequestObject(req);
	
	// Get page table entry
	pt = SafePtePtrFromLinAddr(aAddress, aAsid);

	// Check page still needs updating
	TBool notNeeded = pt==0 || *pt!=KPteNotPresentEntry;
	if(aCodeSegMemory)
		notNeeded |= aCodeSegMemory->iOsAsids->NotFree(aAsid, 1);
	if(notNeeded)
		{
		// We don't need the new page after all, so put it on the active list as a free page
		__KTRACE_OPT(KPAGING,Kern::Printf("DP: PageIn (New page not used)"));
#ifdef BTRACE_PAGING
		BTraceContext0(BTrace::EPaging,BTrace::EPagingPageInUnneeded);
#endif
		AddAsFreePage(pageInfo);
		return pt ? KErrNone : KErrNotFound;
		}

	// Update page info
	if (!aCodeSegMemory)
		pageInfo->SetPagedROM((aAddress-iRomLinearBase)>>KPageShift);
	else
		{
		// Check if page has been paged in and mapped into another process while we were waiting
		TInt pageNumber = (aAddress - aCodeSegMemory->iRamInfo.iCodeRunAddr) >> KPageShift;
		TPhysAddr page = aCodeSegMemory->iPages[pageNumber];
		if (page != KPhysAddrInvalid)
			{
			// don't need page we've just paged in...
			AddAsFreePage(pageInfo);

			// map existing page into this process...
			pageInfo = SPageInfo::FromPhysAddr(page);
			__NK_ASSERT_DEBUG(pageInfo->State()!=SPageInfo::EStatePagedDead);
			*pt = page | (aCodeSegMemory->iCreator ? KUserCodeLoadPte : KUserCodeRunPte);
			CacheMaintenance::SinglePteUpdated((TLinAddr)pt);
#ifdef BTRACE_PAGING
			BTraceContext0(BTrace::EPaging,BTrace::EPagingPageInUnneeded);
#endif
			Rejuvenate(pageInfo);
			return KErrNone;
			}
		aCodeSegMemory->iPages[pageNumber] = phys;
		
		pageInfo->SetPagedCode(aCodeSegMemory,(aAddress-Mmu().iUserCodeBase)>>KPageShift);
		}

	// Map page into final location	
	*pt = phys | (aCodeSegMemory ? (aCodeSegMemory->iCreator ? KUserCodeLoadPte : KUserCodeRunPte) : KRomPtePerm);
	CacheMaintenance::SinglePteUpdated((TLinAddr)pt);
#ifdef BTRACE_PAGING
	TInt subCat = aCodeSegMemory ? BTrace::EPagingPageInCode : BTrace::EPagingPageInROM;
	BTraceContext8(BTrace::EPaging,subCat,phys,aAddress);
#endif

	AddAsYoungest(pageInfo);
	BalanceAges();

	return KErrNone;
	}


inline TUint8 ReadByte(TLinAddr aAddress)
	{ return *(volatile TUint8*)aAddress; }


TInt MemModelDemandPaging::EnsurePagePresent(TLinAddr aPage, DProcess* aProcess)
	{
	TInt r = KErrBadDescriptor;
	XTRAPD(exc,XT_DEFAULT,
		if (!aProcess)
			{
			XTRAP_PAGING_RETRY(CHECK_PAGING_SAFE; ReadByte(aPage););
			r = KErrNone;
			}
		else
			{
			DMemModelThread& t=*(DMemModelThread*)TheCurrentThread;
		retry:
			TInt pagingFault;
			XTRAP_PAGING_START(pagingFault);
			CHECK_PAGING_SAFE;
			// make alias of page in this process
			TLinAddr alias_src;
			TInt alias_size;
			TInt aliasResult = t.Alias(aPage, (DMemModelProcess*)aProcess, 1, EMapAttrReadUser, alias_src, alias_size);
			if (aliasResult>=0)
				{
				// ensure page to be locked is mapped in, by reading from it...
				ReadByte(alias_src);
				r = KErrNone;
				}
			XTRAP_PAGING_END;
			t.RemoveAlias();
			if(pagingFault>0)
				goto retry;
			}
		); // end of XTRAPD
	if(exc)
		return KErrBadDescriptor;
	return r;
	}


TPhysAddr MemModelDemandPaging::LinearToPhysical(TLinAddr aPage, DProcess* aProcess)
	{
	TInt asid = 0;
	if (aProcess)
		asid = ((DMemModelProcess*)aProcess)->iOsAsid;
	return Mmu().LinearToPhysical(aPage, asid);
	}


TInt MemModelDemandPaging::PageState(TLinAddr aAddr)
	{
	DMemModelProcess* process = (DMemModelProcess*)TheCurrentThread->iOwningProcess;
	TInt asid = 0;
	TPte* ptePtr = 0;
	TPte pte = 0;
	TInt r = 0;
	SPageInfo* pageInfo = NULL;

	NKern::LockSystem();

	DMemModelCodeSegMemory* codeSegMemory = 0;
	if(TUint(aAddr-iRomPagedLinearBase)<iRomPagedSize)
		r |= EPageStateInRom;
	else if (TUint(aAddr-iCodeLinearBase)<iCodeSize)
		{
		DMemModelCodeSeg* codeSeg = (DMemModelCodeSeg*)DCodeSeg::CodeSegsByAddress.Find(aAddr);
		if(codeSeg)
			codeSegMemory = codeSeg->Memory();
		asid = process->iOsAsid;
		if (codeSegMemory && codeSegMemory->iOsAsids->NotAllocated(asid, 1))
			{
			r |= EPageStateInRamCode;
			if (codeSegMemory->iIsDemandPaged)
				r |= EPageStatePaged;
			}
		if(process->iCodeChunk)
			r |= EPageStateCodeChunkPresent;
		}

	ptePtr = SafePtePtrFromLinAddr(aAddr,asid);
	if (!ptePtr)
		goto done;
	r |= EPageStatePageTablePresent;
	pte = *ptePtr;
	if (pte == KPteNotPresentEntry)
		goto done;		
	r |= EPageStatePtePresent;
	if (pte & KPtePresentMask)
		r |= EPageStatePteValid;
	
	pageInfo = SPageInfo::FromPhysAddr(pte);
	r |= pageInfo->Type();
	r |= pageInfo->State()<<8;

	if (codeSegMemory && codeSegMemory->iPages)
		{
		TPhysAddr phys = pte & ~KPageMask;
		TInt pageNumber = (aAddr - codeSegMemory->iRamInfo.iCodeRunAddr) >> KPageShift;
		if (codeSegMemory->iPages[pageNumber] == phys)
			r |= EPageStatePhysAddrPresent;
		}

done:
	NKern::UnlockSystem();
	return r;
	}


TBool MemModelDemandPaging::NeedsMutexOrderCheck(TLinAddr aStartAddr, TUint aLength)
	{
	// Don't check mutex order for reads from global area, except for the paged part of rom
	TBool rangeInGlobalArea = aStartAddr >= KRomLinearBase;
	TBool rangeInPagedRom = iRomPagedLinearBase != 0 && aStartAddr < (iRomLinearBase + iRomSize) && (aStartAddr + aLength) > iRomPagedLinearBase;
	return !rangeInGlobalArea || rangeInPagedRom;
	}


EXPORT_C TBool DDemandPagingLock::Lock(DThread* aThread, TLinAddr aStart, TInt aSize)
	{
	MemModelDemandPaging* pager = (MemModelDemandPaging*)iThePager;
	if(pager)
		{
		ArmMmu& m = pager->Mmu();
		TLinAddr end = aStart+aSize;
		
		if ((aStart < TUint(pager->iRomPagedLinearBase+pager->iRomPagedSize) && end > pager->iRomPagedLinearBase) ||
			(aStart < TUint(m.iUserCodeBase + m.iMaxUserCodeSize) && end > m.iUserCodeBase))
			return pager->ReserveLock(aThread,aStart,aSize,*this);
		}
	return EFalse;
	}

void ArmMmu::DisablePageModification(DMemModelChunk* aChunk, TInt aOffset)
//
// Mark the page at aOffset in aChunk read-only to prevent it being
// modified while defrag is in progress. Save the required information
// to allow the fault handler to deal with this.
// Call this with the system unlocked.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::DisablePageModification() offset=%08x", aOffset));

	TInt ptid = aChunk->iPageTables[aOffset>>KChunkShift];
	if(ptid == 0xffff)
		Panic(EDefragDisablePageFailed);	

	NKern::LockSystem();
	TPte* pPte = PageTable(ptid) + ((aOffset&KChunkMask)>>KPageShift);
	TPte pte = *pPte;
	if ((pte & KArmV6PteSmallPage) != KArmV6PteSmallPage 
			|| SP_PTE_PERM_GET(pte) != (TUint)KArmV6PermRWRW)
		Panic(EDefragDisablePageFailed);

	iDisabledAddr = (TLinAddr)(aChunk->iBase) + aOffset;
	if (aChunk->iOwningProcess)
		iDisabledAddrAsid = ((DMemModelProcess*)(aChunk->iOwningProcess))->iOsAsid;
	else
		iDisabledAddrAsid = iDisabledAddr<KRomLinearBase ? UNKNOWN_MAPPING : KERNEL_MAPPING;
	iDisabledPte = pPte;
	iDisabledOldVal = pte;

	*pPte = SP_PTE_PERM_SET(pte, KArmV6PermRORO);
	CacheMaintenance::SinglePteUpdated((TLinAddr)pPte);
	InvalidateTLBForPage(iDisabledAddr, iDisabledAddrAsid);
	NKern::UnlockSystem();
	}

TInt ArmMmu::RamDefragFault(TAny* aExceptionInfo)
	{
	TArmExcInfo& exc=*(TArmExcInfo*)aExceptionInfo;

	// Get faulting address
	TLinAddr faultAddress;
	if(exc.iExcCode==EArmExceptionDataAbort)
		{
		faultAddress = exc.iFaultAddress;
		// Defrag can only cause writes to fault on multiple model
		if(!(exc.iFaultStatus&(1<<11)))
			return KErrUnknown;
		}
	else
		return KErrUnknown; // Not data abort

	// Only handle page permission faults
	if((exc.iFaultStatus & 0x40f) != 0xf)
		return KErrUnknown;

	DMemModelThread* thread = (DMemModelThread*)TheCurrentThread;
	TInt asid = ((DMemModelProcess*)TheScheduler.iAddressSpace)->iOsAsid;

	TBool aliased = EFalse;
	if (thread->iAliasLinAddr && TUint(faultAddress - thread->iAliasLinAddr) < TUint(KPageSize))
		{
		// in aliased memory
		aliased = ETrue;
		faultAddress = (faultAddress - thread->iAliasLinAddr) + thread->iAliasTarget;
		asid = thread->iAliasOsAsid;
		__NK_ASSERT_DEBUG(asid != 0);
		}

	// Take system lock if not already held
	NFastMutex* fm = NKern::HeldFastMutex();
	if(!fm)
		NKern::LockSystem();
	else if(fm!=&TheScheduler.iLock)
		{
		__KTRACE_OPT2(KMMU,KPANIC,Kern::Printf("Defrag: Fault with FM Held! %x (%O pc=%x)",faultAddress,&Kern::CurrentThread(),exc.iR15));
		Panic(EDefragFaultWhilstFMHeld); // Not allowed to hold mutexes
		}

	TInt r = KErrUnknown;

	// check if write access to the page has already been restored and retry if so
	TPte* pt = SafePtePtrFromLinAddr(faultAddress, asid);
	if(!pt)
		{
		r = KErrNotFound;
		goto leave;
		}
	if (SP_PTE_PERM_GET(*pt) == (TUint)KArmV6PermRWRW)
		{
		r = KErrNone;
		goto leave;
		}

	// check if the fault occurred in the page we are moving
	if (	   iDisabledPte
			&& TUint(faultAddress - iDisabledAddr) < TUint(KPageSize)
			&& (iDisabledAddrAsid < 0 || asid == iDisabledAddrAsid) )
		{
		// restore access to the page
		*iDisabledPte = iDisabledOldVal;
		CacheMaintenance::SinglePteUpdated((TLinAddr)iDisabledPte);
		InvalidateTLBForPage(iDisabledAddr, iDisabledAddrAsid);
		if (aliased)
			InvalidateTLBForPage(exc.iFaultAddress, ((DMemModelProcess*)TheScheduler.iAddressSpace)->iOsAsid);
		iDisabledAddr = 0;
		iDisabledAddrAsid = -1;
		iDisabledPte = NULL;
		iDisabledOldVal = 0;
		r = KErrNone;
		}

leave:
	// Restore system lock state
	if (!fm)
		NKern::UnlockSystem();
	
	return r;
	}
