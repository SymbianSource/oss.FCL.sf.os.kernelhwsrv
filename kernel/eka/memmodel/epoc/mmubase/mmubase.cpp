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
// e32\memmodel\epoc\mmubase\mmubase.cpp
// 
//

#include <memmodel/epoc/mmubase/mmubase.h>
#include <mmubase.inl>
#include <ramcache.h>
#include <demand_paging.h>
#include "cache_maintenance.h"
#include "highrestimer.h"
#include <defrag.h>
#include <ramalloc.h>


__ASSERT_COMPILE(sizeof(SPageInfo)==(1<<KPageInfoShift));

_LIT(KLitRamAlloc,"RamAlloc");
_LIT(KLitHwChunk,"HwChunk");


DMutex* MmuBase::HwChunkMutex;
DMutex* MmuBase::RamAllocatorMutex;
#ifdef BTRACE_KERNEL_MEMORY
TInt   Epoc::DriverAllocdPhysRam = 0;
TInt   Epoc::KernelMiscPages = 0;
#endif

/******************************************************************************
 * Code common to all MMU memory models
 ******************************************************************************/

const TInt KFreePagesStepSize=16;

void MmuBase::Panic(TPanic aPanic)
	{
	Kern::Fault("MMUBASE",aPanic);
	}

void SPageInfo::Lock()
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED,"SPageInfo::Lock");
	++iLockCount;
	if(!iLockCount)
		MmuBase::Panic(MmuBase::EPageLockedTooManyTimes);
	}

TInt SPageInfo::Unlock()
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED,"SPageInfo::Unlock");
	if(!iLockCount)
		MmuBase::Panic(MmuBase::EPageUnlockedTooManyTimes);
	return --iLockCount;
	}

#ifdef _DEBUG
void SPageInfo::Set(TType aType, TAny* aOwner, TUint32 aOffset)
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED,"SPageInfo::Set");
	(TUint16&)iType = aType; // also sets iState to EStateNormal
	
	iOwner = aOwner;
	iOffset = aOffset;
	iModifier = 0;
	}

void SPageInfo::Change(TType aType,TState aState)
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED,"SPageInfo::Change");
	iType = aType;
	iState = aState;
	iModifier = 0;
	}

void SPageInfo::SetState(TState aState)
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED,"SPageInfo::SetState");
	iState = aState;
	iModifier = 0;
	}

void SPageInfo::SetModifier(TAny* aModifier)
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED,"SPageInfo::SetModifier");
	iModifier = aModifier;
	}

TInt SPageInfo::CheckModified(TAny* aModifier)
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED,"SPageInfo::CheckModified");
	return iModifier!=aModifier;
	}

void SPageInfo::SetZone(TUint8 aZoneIndex)
	{
	__ASSERT_ALWAYS(K::Initialising,Kern::Fault("SPageInfo::SetZone",0));
	iZone = aZoneIndex;
	}


#endif

MmuBase::MmuBase()
	: iRamCache(NULL), iDefrag(NULL)
	{
	}

TUint32 MmuBase::RoundToPageSize(TUint32 aSize)
	{
	return (aSize+KPageMask)&~KPageMask;
	}

TUint32 MmuBase::RoundToChunkSize(TUint32 aSize)
	{
	TUint32 mask=TheMmu->iChunkMask;
	return (aSize+mask)&~mask;
	}

TInt MmuBase::RoundUpRangeToPageSize(TUint32& aBase, TUint32& aSize)
	{
	TUint32 mask=KPageMask;
	TUint32 shift=KPageShift;
	TUint32 offset=aBase&mask;
	aBase&=~mask;
	aSize=(aSize+offset+mask)&~mask;
	return TInt(aSize>>shift);
	}

void MmuBase::Wait()
	{
	Kern::MutexWait(*RamAllocatorMutex);
	if (RamAllocatorMutex->iHoldCount==1)
		{
		MmuBase& m=*TheMmu;
		m.iInitialFreeMemory=Kern::FreeRamInBytes();
		m.iAllocFailed=EFalse;
		}
	}

void MmuBase::Signal()
	{
	if (RamAllocatorMutex->iHoldCount>1)
		{
		Kern::MutexSignal(*RamAllocatorMutex);
		return;
		}
	MmuBase& m=*TheMmu;
	TInt initial=m.iInitialFreeMemory;
	TBool failed=m.iAllocFailed;
	TInt final=Kern::FreeRamInBytes();
	Kern::MutexSignal(*RamAllocatorMutex);
	K::CheckFreeMemoryLevel(initial,final,failed);
	}

void MmuBase::WaitHwChunk()
	{
	Kern::MutexWait(*HwChunkMutex);
	}

void MmuBase::SignalHwChunk()
	{
	Kern::MutexSignal(*HwChunkMutex);
	}


void MmuBase::MapRamPage(TLinAddr aAddr, TPhysAddr aPage, TPte aPtePerm)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MmuBase::MapRamPage %08x@%08x perm %08x", aPage, aAddr, aPtePerm));
	TInt ptid=PageTableId(aAddr);
	NKern::LockSystem();
	MapRamPages(ptid,SPageInfo::EInvalid,0,aAddr,&aPage,1,aPtePerm);
	NKern::UnlockSystem();
	}

//
// Unmap and free pages from a global area
//
void MmuBase::UnmapAndFree(TLinAddr aAddr, TInt aNumPages)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MmuBase::UnmapAndFree(%08x,%d)",aAddr,aNumPages));
	while(aNumPages)
		{
		TInt pt_np=(iChunkSize-(aAddr&iChunkMask))>>iPageShift;
		TInt np=Min(aNumPages,pt_np);
		aNumPages-=np;
		TInt id=PageTableId(aAddr);
		if (id>=0)
			{
			while(np)
				{
				TInt np2=Min(np,KFreePagesStepSize);
				TPhysAddr phys[KFreePagesStepSize];
				TInt nptes;
				TInt nfree;
				NKern::LockSystem();
				UnmapPages(id,aAddr,np2,phys,true,nptes,nfree,NULL);
				NKern::UnlockSystem();
				if (nfree)
					{
					if (iDecommitThreshold)
						CacheMaintenanceOnDecommit(phys, nfree);
					iRamPageAllocator->FreeRamPages(phys,nfree,EPageFixed);
					}
				np-=np2;
				aAddr+=(np2<<iPageShift);
				}
			}
		else
			{
			aAddr+=(np<<iPageShift);
			}
		}
	}

void MmuBase::FreePages(TPhysAddr* aPageList, TInt aCount, TZonePageType aPageType)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MmuBase::FreePages(%08x,%d)",aPageList,aCount));
	if (!aCount)
		return;
	TBool sync_decommit = (TUint(aCount)<iDecommitThreshold);
	TPhysAddr* ppa=aPageList;
	TPhysAddr* ppaE=ppa+aCount;
	NKern::LockSystem();
	while (ppa<ppaE)
		{
		TPhysAddr pa=*ppa++;
		SPageInfo* pi=SPageInfo::SafeFromPhysAddr(pa);
		if (pi)
			{
			pi->SetUnused();
			if (pi->LockCount())
				ppa[-1]=KPhysAddrInvalid;	// don't free page if it's locked down
			else if (sync_decommit)
				{
				NKern::UnlockSystem();
				CacheMaintenanceOnDecommit(pa);
				NKern::LockSystem();
				}
			}
		if (!sync_decommit)
			NKern::FlashSystem();
		}
	NKern::UnlockSystem();
	if (iDecommitThreshold && !sync_decommit)
		CacheMaintenance::SyncPhysicalCache_All();
	iRamPageAllocator->FreeRamPages(aPageList,aCount, aPageType);
	}

TInt MmuBase::InitPageTableInfo(TInt aId)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MmuBase::InitPageTableInfo(%x)",aId));
	TInt ptb=aId>>iPtBlockShift;
	if (++iPtBlockCount[ptb]==1)
		{
		// expand page table info array
		TPhysAddr pagePhys;
		if (AllocRamPages(&pagePhys,1, EPageFixed)!=KErrNone)
			{
			__KTRACE_OPT(KMMU,Kern::Printf("Unable to allocate page"));
			iPtBlockCount[ptb]=0;
			iAllocFailed=ETrue;
			return KErrNoMemory;
			}
#ifdef BTRACE_KERNEL_MEMORY
		BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryMiscAlloc, 1<<KPageShift);
		++Epoc::KernelMiscPages;
#endif
		TLinAddr pil=PtInfoBlockLinAddr(ptb);
		NKern::LockSystem();
		SPageInfo::FromPhysAddr(pagePhys)->SetPtInfo(ptb);
		NKern::UnlockSystem();
		MapRamPage(pil, pagePhys, iPtInfoPtePerm);
		memclr((TAny*)pil, iPageSize);
		}
	return KErrNone;
	}

TInt MmuBase::DoAllocPageTable(TPhysAddr& aPhysAddr)
//
// Allocate a new page table but don't map it.
// Return page table id and page number/phys address of new page if any.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MmuBase::DoAllocPageTable()"));
#ifdef _DEBUG
	if(K::CheckForSimulatedAllocFail())
		return KErrNoMemory;
#endif
	TInt id=iPageTableAllocator?iPageTableAllocator->Alloc():-1;
	if (id<0)
		{
		// need to allocate a new page
		if (AllocRamPages(&aPhysAddr,1, EPageFixed)!=KErrNone)
			{
			__KTRACE_OPT(KMMU,Kern::Printf("Unable to allocate page"));
			iAllocFailed=ETrue;
			return KErrNoMemory;
			}

		// allocate an ID for the new page
		id=iPageTableLinearAllocator->Alloc();
		if (id>=0)
			{
			id<<=iPtClusterShift;
			__KTRACE_OPT(KMMU,Kern::Printf("Allocated ID %04x",id));
			}
		if (id<0 || InitPageTableInfo(id)!=KErrNone)
			{
			__KTRACE_OPT(KMMU,Kern::Printf("Unable to allocate page table info"));
			iPageTableLinearAllocator->Free(id>>iPtClusterShift);
			if (iDecommitThreshold)
				CacheMaintenanceOnDecommit(aPhysAddr);

			iRamPageAllocator->FreeRamPage(aPhysAddr, EPageFixed);
			iAllocFailed=ETrue;
			return KErrNoMemory;
			}

		// Set up page info for new page
		NKern::LockSystem();
		SPageInfo::FromPhysAddr(aPhysAddr)->SetPageTable(id>>iPtClusterShift);
		NKern::UnlockSystem();
#ifdef BTRACE_KERNEL_MEMORY
		BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryMiscAlloc, 1<<KPageShift);
		++Epoc::KernelMiscPages;
#endif
		// mark all subpages other than first as free for use as page tables
		if (iPtClusterSize>1)
			iPageTableAllocator->Free(id+1,iPtClusterSize-1);
		}
	else
		aPhysAddr=KPhysAddrInvalid;

	__KTRACE_OPT(KMMU,Kern::Printf("DoAllocPageTable returns %d (%08x)",id,aPhysAddr));
	PtInfo(id).SetUnused();
	return id;
	}

TInt MmuBase::MapPageTable(TInt aId, TPhysAddr aPhysAddr, TBool aAllowExpand)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MmuBase::MapPageTable(%d,%08x)",aId,aPhysAddr));
	TLinAddr ptLin=PageTableLinAddr(aId);
	TInt ptg=aId>>iPtGroupShift;
	if (++iPtGroupCount[ptg]==1)
		{
		// need to allocate a new page table
		__ASSERT_ALWAYS(aAllowExpand, Panic(EMapPageTableBadExpand));
		TPhysAddr xptPhys;
		TInt xptid=DoAllocPageTable(xptPhys);
		if (xptid<0)
			{
			__KTRACE_OPT(KMMU,Kern::Printf("Unable to allocate extra page table"));
			iPtGroupCount[ptg]=0;
			return KErrNoMemory;
			}
		if (xptPhys==KPhysAddrInvalid)
			xptPhys=aPhysAddr + ((xptid-aId)<<iPageTableShift);
		BootstrapPageTable(xptid, xptPhys, aId, aPhysAddr);	// initialise XPT and map it
		}
	else
		MapRamPage(ptLin, aPhysAddr, iPtPtePerm);
	return KErrNone;
	}

TInt MmuBase::AllocPageTable()
//
// Allocate a new page table, mapped at the correct linear address.
// Clear all entries to Not Present. Return page table id.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MmuBase::AllocPageTable()"));
	__ASSERT_MUTEX(MmuBase::RamAllocatorMutex);

	TPhysAddr ptPhys;
	TInt id=DoAllocPageTable(ptPhys);
	if (id<0)
		return KErrNoMemory;
	if (ptPhys!=KPhysAddrInvalid)
		{
		TInt r=MapPageTable(id,ptPhys);
		if (r!=KErrNone)
			{
			DoFreePageTable(id);
			SPageInfo* pi=SPageInfo::FromPhysAddr(ptPhys);
			NKern::LockSystem();
			pi->SetUnused();
			NKern::UnlockSystem();
			if (iDecommitThreshold)
				CacheMaintenanceOnDecommit(ptPhys);

			iRamPageAllocator->FreeRamPage(ptPhys, EPageFixed);
			return r;
			}
		}
	ClearPageTable(id);
	__KTRACE_OPT(KMMU,Kern::Printf("AllocPageTable returns %d",id));
	return id;
	}

TBool MmuBase::DoFreePageTable(TInt aId)
//
// Free an empty page table. We assume that all pages mapped by the page table have
// already been unmapped and freed.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MmuBase::DoFreePageTable(%d)",aId));
	SPageTableInfo& s=PtInfo(aId);
	__NK_ASSERT_DEBUG(!s.iCount); // shouldn't have any pages mapped
	s.SetUnused();

	TInt id=aId &~ iPtClusterMask;
	if (iPageTableAllocator)
		{
		iPageTableAllocator->Free(aId);
		if (iPageTableAllocator->NotFree(id,iPtClusterSize))
			{
			// some subpages still in use
			return ETrue;
			}
		__KTRACE_OPT(KMMU,Kern::Printf("Freeing whole page, id=%d",id));
		// whole page is now free
		// remove it from the page table allocator
		iPageTableAllocator->Alloc(id,iPtClusterSize);
		}

	TInt ptb=aId>>iPtBlockShift;
	if (--iPtBlockCount[ptb]==0)
		{
		// shrink page table info array
		TLinAddr pil=PtInfoBlockLinAddr(ptb);
		UnmapAndFree(pil,1);	// remove PTE, null page info, free page
#ifdef BTRACE_KERNEL_MEMORY
		BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryMiscFree, 1<<KPageShift);
		--Epoc::KernelMiscPages;
#endif
		}

	// free the page table linear address
	iPageTableLinearAllocator->Free(id>>iPtClusterShift);
	return EFalse;
	}

void MmuBase::FreePageTable(TInt aId)
//
// Free an empty page table. We assume that all pages mapped by the page table have
// already been unmapped and freed.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MmuBase::FreePageTable(%d)",aId));
	if (DoFreePageTable(aId))
		return;

	TInt id=aId &~ iPtClusterMask;

	// calculate linear address of page
	TLinAddr ptLin=PageTableLinAddr(id);
	__KTRACE_OPT(KMMU,Kern::Printf("Page lin %08x",ptLin));

	// unmap and free the page
	UnmapAndFree(ptLin,1);
#ifdef BTRACE_KERNEL_MEMORY
	BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryMiscFree, 1<<KPageShift);
	--Epoc::KernelMiscPages;
#endif

	TInt ptg=aId>>iPtGroupShift;
	--iPtGroupCount[ptg];
	// don't shrink the page table mapping for now
	}

TInt MmuBase::AllocPhysicalRam(TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::AllocPhysicalRam() size=%x align=%d",aSize,aAlign));
	TInt r=AllocContiguousRam(aSize, aPhysAddr, aAlign);
	if (r!=KErrNone)
		{
		iAllocFailed=ETrue;
		return r;
		}
	TInt n=TInt(TUint32(aSize+iPageMask)>>iPageShift);
	SPageInfo* pI=SPageInfo::FromPhysAddr(aPhysAddr);
	SPageInfo* pE=pI+n;
	for (; pI<pE; ++pI)
		{
		NKern::LockSystem();
		__NK_ASSERT_DEBUG(pI->Type()==SPageInfo::EUnused);
		pI->Lock();
		NKern::UnlockSystem();
		}
	return KErrNone;
	}

/** Attempt to allocate a contiguous block of RAM from the specified zone.

@param aZoneIdList	An array of the IDs of the RAM zones to allocate from.
@param aZoneIdCount	The number of RAM zone IDs listed in aZoneIdList.
@param aSize 		The number of contiguous bytes to allocate
@param aPhysAddr 	The physical address of the start of the contiguous block of 
					memory allocated
@param aAlign		Required alignment
@return KErrNone on success, KErrArgument if zone doesn't exist or aSize is larger than the
size of the RAM zone or KErrNoMemory when the RAM zone is too full.
*/
TInt MmuBase::ZoneAllocPhysicalRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::ZoneAllocPhysicalRam() size=0x%x align=%d", aSize, aAlign));
	TInt r = ZoneAllocContiguousRam(aZoneIdList, aZoneIdCount, aSize, aPhysAddr, aAlign);
	if (r!=KErrNone)
		{
		iAllocFailed=ETrue;
		return r;
		}
	TInt n=TInt(TUint32(aSize+iPageMask)>>iPageShift);
	SPageInfo* pI=SPageInfo::FromPhysAddr(aPhysAddr);
	SPageInfo* pE=pI+n;
	for (; pI<pE; ++pI)
		{
		NKern::LockSystem();
		__NK_ASSERT_DEBUG(pI->Type()==SPageInfo::EUnused);
		pI->Lock();
		NKern::UnlockSystem();
		}
	return KErrNone;
	}


/** Attempt to allocate discontiguous RAM pages.

@param aNumPages	The number of pages to allocate.
@param aPageList 	Pointer to an array where each element will be the physical 
					address of each page allocated.
@return KErrNone on success, KErrNoMemory otherwise
*/
TInt MmuBase::AllocPhysicalRam(TInt aNumPages, TPhysAddr* aPageList)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::AllocPhysicalRam() numpages=%x", aNumPages));
	TInt r = AllocRamPages(aPageList, aNumPages, EPageFixed);
	if (r!=KErrNone)
		{
		iAllocFailed=ETrue;
		return r;
		}
	TPhysAddr* pageEnd = aPageList + aNumPages;
	for (TPhysAddr* page = aPageList; page < pageEnd; page++)
		{
		SPageInfo* pageInfo = SPageInfo::FromPhysAddr(*page);
		NKern::LockSystem();
		__NK_ASSERT_DEBUG(pageInfo->Type() == SPageInfo::EUnused);
		pageInfo->Lock();
		NKern::UnlockSystem();
		}
	return KErrNone;
	}


/** Attempt to allocate discontiguous RAM pages from the specified RAM zones.

@param aZoneIdList	An array of the IDs of the RAM zones to allocate from.
@param aZoneIdCount	The number of RAM zone IDs listed in aZoneIdList.
@param aNumPages	The number of pages to allocate.
@param aPageList 	Pointer to an array where each element will be the physical 
					address of each page allocated.
@return KErrNone on success, KErrArgument if zone doesn't exist or aNumPages is 
larger than the total number of pages in the RAM zone or KErrNoMemory when the RAM 
zone is too full.
*/
TInt MmuBase::ZoneAllocPhysicalRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aNumPages, TPhysAddr* aPageList)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::ZoneAllocPhysicalRam() numpages 0x%x zones 0x%x", aNumPages, aZoneIdCount));
	TInt r = ZoneAllocRamPages(aZoneIdList, aZoneIdCount, aPageList, aNumPages, EPageFixed);
	if (r!=KErrNone)
		{
		iAllocFailed=ETrue;
		return r;
		}

	TPhysAddr* pageEnd = aPageList + aNumPages;
	for (TPhysAddr* page = aPageList; page < pageEnd; page++)
		{
		SPageInfo* pageInfo = SPageInfo::FromPhysAddr(*page);
		NKern::LockSystem();
		__NK_ASSERT_DEBUG(pageInfo->Type() == SPageInfo::EUnused);
		pageInfo->Lock();
		NKern::UnlockSystem();
		}
	return KErrNone;
	}


TInt MmuBase::FreePhysicalRam(TPhysAddr aPhysAddr, TInt aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::FreePhysicalRam(%08x,%x)",aPhysAddr,aSize));

	TInt n=TInt(TUint32(aSize+iPageMask)>>iPageShift);
	SPageInfo* pI=SPageInfo::FromPhysAddr(aPhysAddr);
	SPageInfo* pE=pI+n;
	for (; pI<pE; ++pI)
		{
		NKern::LockSystem();
		__ASSERT_ALWAYS(pI->Type()==SPageInfo::EUnused && pI->Unlock()==0, Panic(EBadFreePhysicalRam));
		NKern::UnlockSystem();
		}
	TInt r=iRamPageAllocator->FreePhysicalRam(aPhysAddr, aSize);
	return r;
	}

/** Free discontiguous RAM pages that were previously allocated using discontiguous
overload of MmuBase::AllocPhysicalRam() or MmuBase::ZoneAllocPhysicalRam().

Specifying one of the following may cause the system to panic: 
a) an invalid physical RAM address.
b) valid physical RAM addresses where some had not been previously allocated.
c) an adrress not aligned to a page boundary.

@param aNumPages	Number of pages to free
@param aPageList	Array of the physical address of each page to free

@return KErrNone if the operation was successful.
		
*/
TInt MmuBase::FreePhysicalRam(TInt aNumPages, TPhysAddr* aPageList)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::FreePhysicalRam(%08x,%08x)", aNumPages, aPageList));
	
	TPhysAddr* pageEnd = aPageList + aNumPages;
	TInt r = KErrNone;

	for (TPhysAddr* page = aPageList; page < pageEnd && r == KErrNone; page++)
		{
		SPageInfo* pageInfo = SPageInfo::FromPhysAddr(*page);
		NKern::LockSystem();
		__ASSERT_ALWAYS(pageInfo->Type()==SPageInfo::EUnused && pageInfo->Unlock()==0, Panic(EBadFreePhysicalRam));
		NKern::UnlockSystem();
		
		// Free the page
		r = iRamPageAllocator->FreePhysicalRam(*page, KPageSize);
		}
	return r;
	}


TInt MmuBase::FreeRamZone(TUint aZoneId, TPhysAddr& aZoneBase, TUint& aZoneBytes)
	{
	TUint zonePages;
	TInt r = iRamPageAllocator->GetZoneAddress(aZoneId, aZoneBase, zonePages);
	if (r != KErrNone)
		return r;
	aZoneBytes = zonePages << KPageShift;
	return MmuBase::FreePhysicalRam(aZoneBase, aZoneBytes);
	}


TInt MmuBase::ClaimPhysicalRam(TPhysAddr aPhysAddr, TInt aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::ClaimPhysicalRam(%08x,%x)",aPhysAddr,aSize));
	TUint32 pa=aPhysAddr;
	TUint32 size=aSize;
	TInt n=RoundUpRangeToPageSize(pa,size);
	TInt r=iRamPageAllocator->ClaimPhysicalRam(pa, size);
	if (r==KErrNone)
		{
		SPageInfo* pI=SPageInfo::FromPhysAddr(pa);
		SPageInfo* pE=pI+n;
		for (; pI<pE; ++pI)
			{
			NKern::LockSystem();
			__NK_ASSERT_DEBUG(pI->Type()==SPageInfo::EUnused && pI->LockCount()==0);
			pI->Lock();
			NKern::UnlockSystem();
			}
		}
	return r;
	}

/** 
Allocate a set of discontiguous RAM pages from the specified zone.

@param aZoneIdList	The array of IDs of the RAM zones to allocate from.
@param aZoneIdCount	The number of RAM zone IDs in aZoneIdList.
@param aPageList 	Preallocated array of TPhysAddr elements that will receive the
physical address of each page allocated.
@param aNumPages 	The number of pages to allocate.
@param aPageType 	The type of the pages being allocated.

@return KErrNone on success, KErrArgument if a zone of aZoneIdList doesn't exist, 
KErrNoMemory if there aren't enough free pages in the zone
*/
TInt MmuBase::ZoneAllocRamPages(TUint* aZoneIdList, TUint aZoneIdCount, TPhysAddr* aPageList, TInt aNumPages, TZonePageType aPageType)
	{
#ifdef _DEBUG
	if(K::CheckForSimulatedAllocFail())
		return KErrNoMemory;
#endif
	__NK_ASSERT_DEBUG(aPageType == EPageFixed);

	return iRamPageAllocator->ZoneAllocRamPages(aZoneIdList, aZoneIdCount, aPageList, aNumPages, aPageType);
	}


TInt MmuBase::AllocRamPages(TPhysAddr* aPageList, TInt aNumPages, TZonePageType aPageType, TUint aBlockedZoneId, TBool aBlockRest)
	{
#ifdef _DEBUG
	if(K::CheckForSimulatedAllocFail())
		return KErrNoMemory;
#endif
	TInt missing = iRamPageAllocator->AllocRamPages(aPageList, aNumPages, aPageType, aBlockedZoneId, aBlockRest);

	// If missing some pages, ask the RAM cache to donate some of its pages.
	// Don't ask it for discardable pages as those are intended for itself.
	if(missing && aPageType != EPageDiscard && iRamCache->GetFreePages(missing))
		missing = iRamPageAllocator->AllocRamPages(aPageList, aNumPages, aPageType, aBlockedZoneId, aBlockRest);
	return missing ? KErrNoMemory : KErrNone;
	}


TInt MmuBase::AllocContiguousRam(TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign)
	{
#ifdef _DEBUG
	if(K::CheckForSimulatedAllocFail())
		return KErrNoMemory;
#endif
	TUint contigPages = (aSize + KPageSize - 1) >> KPageShift;
	TInt r = iRamPageAllocator->AllocContiguousRam(contigPages, aPhysAddr, aAlign);
	if (r == KErrNoMemory && contigPages > KMaxFreeableContiguousPages)
		{// Allocation failed but as this is a large allocation flush the RAM cache 
		// and reattempt the allocation as large allocation wouldn't discard pages.
		iRamCache->FlushAll();
		r = iRamPageAllocator->AllocContiguousRam(contigPages, aPhysAddr, aAlign);
		}
	return r;
	}


/**
Allocate contiguous RAM from the specified RAM zones.
@param aZoneIdList	An array of IDs of the RAM zones to allocate from
@param aZoneIdCount	The number of IDs listed in aZoneIdList
@param aSize		The number of bytes to allocate
@param aPhysAddr 	Will receive the physical base address of the allocated RAM
@param aAlign 		The log base 2 alginment required
*/
TInt MmuBase::ZoneAllocContiguousRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign)
	{
#ifdef _DEBUG
	if(K::CheckForSimulatedAllocFail())
		return KErrNoMemory;
#endif
	return iRamPageAllocator->ZoneAllocContiguousRam(aZoneIdList, aZoneIdCount, aSize, aPhysAddr, aAlign);
	}

SPageInfo* SPageInfo::SafeFromPhysAddr(TPhysAddr aAddress)
	{
	TUint index = aAddress>>(KPageShift+KPageShift-KPageInfoShift);
	TUint flags = ((TUint8*)KPageInfoMap)[index>>3];
	TUint mask = 1<<(index&7);
	if(!(flags&mask))
		return 0; // no SPageInfo for aAddress
	SPageInfo* info = FromPhysAddr(aAddress);
	if(info->Type()==SPageInfo::EInvalid)
		return 0;
	return info;
	}

/** HAL Function wrapper for the RAM allocator.
 */

TInt RamHalFunction(TAny*, TInt aFunction, TAny* a1, TAny* a2)
	{
	DRamAllocator *pRamAlloc = MmuBase::TheMmu->iRamPageAllocator;
	
	if (pRamAlloc)
		return pRamAlloc->HalFunction(aFunction, a1, a2);
	return KErrNotSupported;
	}


/******************************************************************************
 * Initialisation
 ******************************************************************************/

void MmuBase::Init1()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("MmuBase::Init1"));
	iInitialFreeMemory=0;
	iAllocFailed=EFalse;
	}

void MmuBase::Init2()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("MmuBase::Init2"));
	TInt total_ram=TheSuperPage().iTotalRamSize;
	TInt total_ram_pages=total_ram>>iPageShift;
	iNumPages = total_ram_pages;
	const SRamInfo& info=*(const SRamInfo*)TheSuperPage().iRamBootData;
	iRamPageAllocator=DRamAllocator::New(info, RamZoneConfig, RamZoneCallback);

	TInt max_pt=total_ram>>iPageTableShift;
	if (max_pt<iMaxPageTables)
		iMaxPageTables=max_pt;
	iMaxPageTables &= ~iPtClusterMask;
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("iMaxPageTables=%d",iMaxPageTables));
	TInt max_ptpg=iMaxPageTables>>iPtClusterShift;
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("max_ptpg=%d",max_ptpg));
	iPageTableLinearAllocator=TBitMapAllocator::New(max_ptpg,ETrue);
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("iPageTableLinearAllocator=%08x",iPageTableLinearAllocator));
	__ASSERT_ALWAYS(iPageTableLinearAllocator,Panic(EPtLinAllocCreateFailed));
	if (iPtClusterShift)	// if more than one page table per page
		{
		iPageTableAllocator=TBitMapAllocator::New(iMaxPageTables,EFalse);
		__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("iPageTableAllocator=%08x",iPageTableAllocator));
		__ASSERT_ALWAYS(iPageTableAllocator,Panic(EPtAllocCreateFailed));
		}
	TInt max_ptb=(iMaxPageTables+iPtBlockMask)>>iPtBlockShift;
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("max_ptb=%d",max_ptb));
	iPtBlockCount=(TInt*)Kern::AllocZ(max_ptb*sizeof(TInt));
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("iPtBlockCount=%08x",iPtBlockCount));
	__ASSERT_ALWAYS(iPtBlockCount,Panic(EPtBlockCountCreateFailed));
	TInt max_ptg=(iMaxPageTables+iPtGroupMask)>>iPtGroupShift;
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("ptg_shift=%d, max_ptg=%d",iPtGroupShift,max_ptg));
	iPtGroupCount=(TInt*)Kern::AllocZ(max_ptg*sizeof(TInt));
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("iPtGroupCount=%08x",iPtGroupCount));
	__ASSERT_ALWAYS(iPtGroupCount,Panic(EPtGroupCountCreateFailed));


	// Clear the inital (and only so far) page table info page so all unused
	// page tables will be marked as unused.
	memclr((TAny*)KPageTableInfoBase, KPageSize);

	// look for page tables - assume first page table (id=0) maps page tables
	TPte* pPte=(TPte*)iPageTableLinBase;
	TInt i;
	for (i=0; i<iChunkSize/iPageSize; ++i)
		{
		TPte pte=*pPte++;
		if (!PteIsPresent(pte))	// after boot, page tables are contiguous
			break;
		iPageTableLinearAllocator->Alloc(i,1);
		TPhysAddr ptpgPhys=PtePhysAddr(pte, i);
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(ptpgPhys);
		__ASSERT_ALWAYS(pi, Panic(EInvalidPageTableAtBoot));
		pi->SetPageTable(i);
		pi->Lock();
		TInt id=i<<iPtClusterShift;
		TInt ptb=id>>iPtBlockShift;
		++iPtBlockCount[ptb];
		TInt ptg=id>>iPtGroupShift;
		++iPtGroupCount[ptg];
		}

	// look for mapped pages
	TInt npdes=1<<(32-iChunkShift);
	TInt npt=0;
	for (i=0; i<npdes; ++i)
		{
		TLinAddr cAddr=TLinAddr(i<<iChunkShift);
		if (cAddr>=PP::RamDriveStartAddress && TUint32(cAddr-PP::RamDriveStartAddress)<TUint32(PP::RamDriveRange))
			continue;	// leave RAM drive for now
		TInt ptid=PageTableId(cAddr);
		TPhysAddr pdePhys = PdePhysAddr(cAddr);	// check for whole PDE mapping
		pPte = NULL;
		if (ptid>=0)
			{
			++npt;
			__KTRACE_OPT(KMMU,Kern::Printf("Addr %08x -> page table %d", cAddr, ptid));
			pPte=(TPte*)PageTableLinAddr(ptid);
			}
#ifdef KMMU
		if (pdePhys != KPhysAddrInvalid)
			{
			__KTRACE_OPT(KMMU,Kern::Printf("Addr %08x -> Whole PDE Phys %08x", cAddr, pdePhys));
			}
#endif
		if (ptid>=0 || pdePhys != KPhysAddrInvalid)
			{
			TInt j;
			TInt np=0;
			for (j=0; j<iChunkSize/iPageSize; ++j)
				{
				TBool present = ETrue;	// all pages present if whole PDE mapping
				TPte pte = 0;
				if (pPte)
					{
					pte = pPte[j];
					present = PteIsPresent(pte);
					}
				if (present)
					{
					++np;
					TPhysAddr pa = pPte ? PtePhysAddr(pte, j) : (pdePhys + (j<<iPageShift));
					SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pa);
					__KTRACE_OPT(KMMU,Kern::Printf("Addr: %08x PA=%08x",
														cAddr+(j<<iPageShift), pa));
					if (pi)	// ignore non-RAM mappings
						{//these pages will never be freed and can't be moved
						TInt r = iRamPageAllocator->MarkPageAllocated(pa, EPageFixed);
						// allow KErrAlreadyExists since it's possible that a page is doubly mapped
						__ASSERT_ALWAYS(r==KErrNone || r==KErrAlreadyExists, Panic(EBadMappedPageAfterBoot));
						SetupInitialPageInfo(pi,cAddr,j);
#ifdef BTRACE_KERNEL_MEMORY
						if(r==KErrNone)
							++Epoc::KernelMiscPages;
#endif
						}
					}
				}
			__KTRACE_OPT(KMMU,Kern::Printf("Addr: %08x #PTEs=%d",cAddr,np));
			if (ptid>=0)
				SetupInitialPageTableInfo(ptid,cAddr,np);
			}
		}

	TInt oddpt=npt & iPtClusterMask;
	if (oddpt)
		oddpt=iPtClusterSize-oddpt;
	__KTRACE_OPT(KBOOT,Kern::Printf("Total page tables %d, left over subpages %d",npt,oddpt));
	if (oddpt)
		iPageTableAllocator->Free(npt,oddpt);

	DoInit2();

	// Save current free RAM size - there can never be more free RAM than this
	TInt max_free = Kern::FreeRamInBytes();
	K::MaxFreeRam = max_free;
	if (max_free < PP::RamDriveMaxSize)
		PP::RamDriveMaxSize = max_free;

	if (K::ColdStart)
		ClearRamDrive(PP::RamDriveStartAddress);
	else
		RecoverRamDrive();

	TInt r=K::MutexCreate((DMutex*&)RamAllocatorMutex, KLitRamAlloc, NULL, EFalse, KMutexOrdRamAlloc);
	if (r!=KErrNone)
		Panic(ERamAllocMutexCreateFailed);
	r=K::MutexCreate((DMutex*&)HwChunkMutex, KLitHwChunk, NULL, EFalse, KMutexOrdHwChunk);
	if (r!=KErrNone)
		Panic(EHwChunkMutexCreateFailed);
	
#ifdef __DEMAND_PAGING__
	if (DemandPaging::RomPagingRequested() || DemandPaging::CodePagingRequested())
		iRamCache = DemandPaging::New();
	else
		iRamCache = new RamCache;
#else
	iRamCache = new RamCache;
#endif
	if (!iRamCache)
		Panic(ERamCacheAllocFailed);
	iRamCache->Init2();
	RamCacheBase::TheRamCache = iRamCache;

	// Get the allocator to signal to the variant which RAM zones are in use so far
	iRamPageAllocator->InitialCallback();
	}

void MmuBase::Init3()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("MmuBase::Init3"));

	// Initialise demand paging
#ifdef __DEMAND_PAGING__
	M::DemandPagingInit();
#endif

	// Register a HAL Function for the Ram allocator.
	TInt r = Kern::AddHalEntry(EHalGroupRam, RamHalFunction, 0);
	__NK_ASSERT_ALWAYS(r==KErrNone);

	//
	// Perform the intialisation for page moving and RAM defrag object.
	//

	// allocate a page to use as an alt stack
	MmuBase::Wait();
	TPhysAddr stackpage;
	r = AllocPhysicalRam(KPageSize, stackpage);
	MmuBase::Signal();
	if (r!=KErrNone)
		Panic(EDefragStackAllocFailed);

	// map it at a predetermined address
	TInt ptid = PageTableId(KDefragAltStackAddr);
	TPte perm = PtePermissions(EKernelStack);
	NKern::LockSystem();
	MapRamPages(ptid, SPageInfo::EFixed, NULL, KDefragAltStackAddr, &stackpage, 1, perm);
	NKern::UnlockSystem();
	iAltStackBase = KDefragAltStackAddr + KPageSize;

	__KTRACE_OPT(KMMU,Kern::Printf("Allocated defrag alt stack page at %08x, mapped to %08x, base is now %08x", stackpage, KDefragAltStackAddr, iAltStackBase));

	// Create the actual defrag object and initialise it.
	iDefrag = new Defrag;
	if (!iDefrag)
		Panic(EDefragAllocFailed);
	iDefrag->Init3(iRamPageAllocator);
	}

void MmuBase::CreateKernelSection(TLinAddr aEnd, TInt aHwChunkAlign)
	{
	TLinAddr base=(TLinAddr)TheRomHeader().iKernelLimit;
	iKernelSection=TLinearSection::New(base, aEnd);
	__ASSERT_ALWAYS(iKernelSection!=NULL, Panic(ECreateKernelSectionFailed));
	iHwChunkAllocator=THwChunkAddressAllocator::New(aHwChunkAlign, iKernelSection);
	__ASSERT_ALWAYS(iHwChunkAllocator!=NULL, Panic(ECreateHwChunkAllocFailed));
	}

// Recover RAM drive contents after a reset
TInt MmuBase::RecoverRamDrive()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MmuBase::RecoverRamDrive()"));
	TLinAddr ptlin;
	TLinAddr chunk = PP::RamDriveStartAddress;
	TLinAddr end = chunk + (TLinAddr)PP::RamDriveRange;
	TInt size = 0;
	TInt limit = RoundToPageSize(TheSuperPage().iRamDriveSize);
	for( ; chunk<end; chunk+=iChunkSize)
		{
		if (size==limit)		// have reached end of ram drive
			break;
		TPhysAddr ptphys = 0;
		TInt ptid = BootPageTableId(chunk, ptphys);	// ret KErrNotFound if PDE not present, KErrUnknown if present but as yet unknown page table
		__KTRACE_OPT(KMMU,Kern::Printf("Addr %08x: PTID=%d PTPHYS=%08x", chunk, ptid, ptphys));
		if (ptid==KErrNotFound)
			break;		// no page table so stop here and clear to end of range
		TPhysAddr ptpgphys = ptphys & ~iPageMask;
		TInt r = iRamPageAllocator->MarkPageAllocated(ptpgphys, EPageMovable);
		__KTRACE_OPT(KMMU,Kern::Printf("MPA: r=%d",r));
		if (r==KErrArgument)
			break;		// page table address was invalid - stop here and clear to end of range
		if (r==KErrNone)
			{
			// this page was currently unallocated
			if (ptid>=0)
				break;	// ID has been allocated - bad news - bail here
			ptid = iPageTableLinearAllocator->Alloc();
			__ASSERT_ALWAYS(ptid>=0, Panic(ERecoverRamDriveAllocPTIDFailed));
			SPageInfo* pi = SPageInfo::SafeFromPhysAddr(ptpgphys);
			__ASSERT_ALWAYS(pi, Panic(ERecoverRamDriveBadPageTable));
			pi->SetPageTable(ptid);	// id = cluster number here
			ptid <<= iPtClusterShift;
			MapPageTable(ptid, ptpgphys, EFalse);
			if (iPageTableAllocator)
				iPageTableAllocator->Free(ptid, iPtClusterSize);
			ptid |= ((ptphys>>iPageTableShift)&iPtClusterMask);
			ptlin = PageTableLinAddr(ptid);
			__KTRACE_OPT(KMMU,Kern::Printf("Page table ID %d lin %08x", ptid, ptlin));
			if (iPageTableAllocator)
				iPageTableAllocator->Alloc(ptid, 1);
			}
		else
			{
			// this page was already allocated
			if (ptid<0)
				break;	// ID not allocated - bad news - bail here
			ptlin = PageTableLinAddr(ptid);
			__KTRACE_OPT(KMMU,Kern::Printf("Page table lin %08x", ptlin));
			if (iPageTableAllocator)
				iPageTableAllocator->Alloc(ptid, 1);
			}
		TInt pte_index;
		TBool chunk_inc = 0;
		TPte* page_table = (TPte*)ptlin;
		for (pte_index=0; pte_index<(iChunkSize>>iPageSize); ++pte_index)
			{
			if (size==limit)		// have reached end of ram drive
				break;
			TPte pte = page_table[pte_index];
			if (PteIsPresent(pte))
				{
				TPhysAddr pa=PtePhysAddr(pte, pte_index);
				SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pa);
				if (!pi)
					break;
				TInt r = iRamPageAllocator->MarkPageAllocated(pa, EPageMovable);
				__ASSERT_ALWAYS(r==KErrNone, Panic(ERecoverRamDriveBadPage));
				size+=iPageSize;
				chunk_inc = iChunkSize;
				}
			}
		if (pte_index < (iChunkSize>>iPageSize) )
			{
			// if we recovered pages in this page table, leave it in place
			chunk += chunk_inc;

			// clear from here on
			ClearPageTable(ptid, pte_index);
			break;
			}
		}
	if (chunk < end)
		ClearRamDrive(chunk);
	__KTRACE_OPT(KMMU,Kern::Printf("Recovered RAM drive size %08x",size));
	if (size<TheSuperPage().iRamDriveSize)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("Truncating RAM drive from %08x to %08x", TheSuperPage().iRamDriveSize, size));
		TheSuperPage().iRamDriveSize=size;
		}
	return KErrNone;
	}

TInt MmuBase::AllocShadowPage(TLinAddr aRomAddr)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MmuBase:AllocShadowPage(%08x)", aRomAddr));
	aRomAddr &= ~iPageMask;
	TPhysAddr orig_phys = KPhysAddrInvalid;
	if (aRomAddr>=iRomLinearBase && aRomAddr<=(iRomLinearEnd-iPageSize))
		orig_phys = LinearToPhysical(aRomAddr);
	__KTRACE_OPT(KMMU,Kern::Printf("OrigPhys = %08x",orig_phys));
	if (orig_phys == KPhysAddrInvalid)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("Invalid ROM address"));
		return KErrArgument;
		}
	SPageInfo* pi = SPageInfo::SafeFromPhysAddr(orig_phys);
	if (pi && pi->Type()==SPageInfo::EShadow)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("ROM address already shadowed"));
		return KErrAlreadyExists;
		}
	TInt ptid = PageTableId(aRomAddr);
	__KTRACE_OPT(KMMU, Kern::Printf("Shadow PTID %d", ptid));
	TInt newptid = -1;
	if (ptid<0)
		{
		newptid = AllocPageTable();
		__KTRACE_OPT(KMMU, Kern::Printf("New shadow PTID %d", newptid));
		if (newptid<0)
			return KErrNoMemory;
		ptid = newptid;
		PtInfo(ptid).SetShadow( (aRomAddr-iRomLinearBase)>>iChunkShift );
		InitShadowPageTable(ptid, aRomAddr, orig_phys);
		}
	TPhysAddr shadow_phys;

	if (AllocRamPages(&shadow_phys, 1, EPageFixed) != KErrNone)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("Unable to allocate page"));
		iAllocFailed=ETrue;
		if (newptid>=0)
			{
			FreePageTable(newptid);
			}
		return KErrNoMemory;
		}
#ifdef BTRACE_KERNEL_MEMORY
	BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryMiscAlloc, 1<<KPageShift);
	++Epoc::KernelMiscPages;
#endif
	InitShadowPage(shadow_phys, aRomAddr);	// copy original ROM contents
	NKern::LockSystem();
	Pagify(ptid, aRomAddr);
	MapRamPages(ptid, SPageInfo::EShadow, (TAny*)orig_phys, (aRomAddr-iRomLinearBase), &shadow_phys, 1, iShadowPtePerm);
	NKern::UnlockSystem();
	if (newptid>=0)
		{
		NKern::LockSystem();
		AssignShadowPageTable(newptid, aRomAddr);
		NKern::UnlockSystem();
		}
	FlushShadow(aRomAddr);
	__KTRACE_OPT(KMMU,Kern::Printf("AllocShadowPage successful"));
	return KErrNone;
	}

TInt MmuBase::FreeShadowPage(TLinAddr aRomAddr)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MmuBase:FreeShadowPage(%08x)", aRomAddr));
	aRomAddr &= ~iPageMask;
	TPhysAddr shadow_phys = KPhysAddrInvalid;
	if (aRomAddr>=iRomLinearBase || aRomAddr<=(iRomLinearEnd-iPageSize))
		shadow_phys = LinearToPhysical(aRomAddr);
	__KTRACE_OPT(KMMU,Kern::Printf("ShadowPhys = %08x",shadow_phys));
	if (shadow_phys == KPhysAddrInvalid)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("Invalid ROM address"));
		return KErrArgument;
		}
	TInt ptid = PageTableId(aRomAddr);
	SPageInfo* pi = SPageInfo::SafeFromPhysAddr(shadow_phys);
	if (ptid<0 || !pi || pi->Type()!=SPageInfo::EShadow)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("No shadow page at this address"));
		return KErrGeneral;
		}
	TPhysAddr orig_phys = (TPhysAddr)pi->Owner();
	DoUnmapShadowPage(ptid, aRomAddr, orig_phys);
	SPageTableInfo& pti = PtInfo(ptid);
	if (pti.Attribs()==SPageTableInfo::EShadow && --pti.iCount==0)
		{
		TInt r = UnassignShadowPageTable(aRomAddr, orig_phys);
		if (r==KErrNone)
			FreePageTable(ptid);
		else
			pti.SetGlobal(aRomAddr>>iChunkShift);
		}

	FreePages(&shadow_phys, 1, EPageFixed);
	__KTRACE_OPT(KMMU,Kern::Printf("FreeShadowPage successful"));
#ifdef BTRACE_KERNEL_MEMORY
	BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryMiscFree, 1<<KPageShift);
	--Epoc::KernelMiscPages;
#endif
	return KErrNone;
	}

TInt MmuBase::FreezeShadowPage(TLinAddr aRomAddr)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MmuBase:FreezeShadowPage(%08x)", aRomAddr));
	aRomAddr &= ~iPageMask;
	TPhysAddr shadow_phys = KPhysAddrInvalid;
	if (aRomAddr>=iRomLinearBase || aRomAddr<=(iRomLinearEnd-iPageSize))
		shadow_phys = LinearToPhysical(aRomAddr);
	__KTRACE_OPT(KMMU,Kern::Printf("ShadowPhys = %08x",shadow_phys));
	if (shadow_phys == KPhysAddrInvalid)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("Invalid ROM address"));
		return KErrArgument;
		}
	TInt ptid = PageTableId(aRomAddr);
	SPageInfo* pi = SPageInfo::SafeFromPhysAddr(shadow_phys);
	if (ptid<0 || pi==0)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("No shadow page at this address"));
		return KErrGeneral;
		}
	DoFreezeShadowPage(ptid, aRomAddr);
	__KTRACE_OPT(KMMU,Kern::Printf("FreezeShadowPage successful"));
	return KErrNone;
	}

TInt MmuBase::CopyToShadowMemory(TLinAddr aDest, TLinAddr aSrc, TUint32 aLength)
	{
	memcpy ((TAny*)aDest, (const TAny*)aSrc, aLength);
	return KErrNone;
	}

void M::BTracePrime(TUint aCategory)
	{
	(void)aCategory;

#ifdef BTRACE_KERNEL_MEMORY
	// Must check for -1 as that is the default value of aCategory for
	// BTrace::Prime() which is intended to prime all categories that are 
	// currently enabled via a single invocation of BTrace::Prime().
	if(aCategory==BTrace::EKernelMemory || (TInt)aCategory == -1)
		{
		NKern::ThreadEnterCS();
		Mmu::Wait();
		BTrace4(BTrace::EKernelMemory,BTrace::EKernelMemoryInitialFree,TheSuperPage().iTotalRamSize);
		BTrace4(BTrace::EKernelMemory,BTrace::EKernelMemoryCurrentFree,Kern::FreeRamInBytes());
		BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryMiscAlloc, Epoc::KernelMiscPages<<KPageShift);
		#ifdef __DEMAND_PAGING__
		if (DemandPaging::ThePager) 
			BTrace4(BTrace::EKernelMemory,BTrace::EKernelMemoryDemandPagingCache,DemandPaging::ThePager->iMinimumPageCount << KPageShift);
		#endif
		BTrace8(BTrace::EKernelMemory,BTrace::EKernelMemoryDrvPhysAlloc, Epoc::DriverAllocdPhysRam, -1);
		Mmu::Signal();
		NKern::ThreadLeaveCS();
		}
#endif

#ifdef BTRACE_RAM_ALLOCATOR
	// Must check for -1 as that is the default value of aCategory for
	// BTrace::Prime() which is intended to prime all categories that are 
	// currently enabled via a single invocation of BTrace::Prime().
	if(aCategory==BTrace::ERamAllocator || (TInt)aCategory == -1)
		{
		NKern::ThreadEnterCS();
		Mmu::Wait();
		Mmu::Get().iRamPageAllocator->DoBTracePrime();
		Mmu::Signal();
		NKern::ThreadLeaveCS();
		}
#endif
	}


/******************************************************************************
 * Code common to all virtual memory models
 ******************************************************************************/

void RHeapK::Mutate(TInt aOffset, TInt aMaxLength)
//
// Used by the kernel to mutate a fixed heap into a chunk heap.
//
	{
	iMinLength += aOffset;
	iMaxLength = aMaxLength + aOffset;
	iOffset = aOffset;
	iChunkHandle = (TInt)K::HeapInfo.iChunk;
	iPageSize = M::PageSizeInBytes();
	iGrowBy = iPageSize;
	iFlags = 0;
	}

TInt M::PageSizeInBytes()
	{
	return KPageSize;
	}

TInt MmuBase::FreeRamInBytes()
	{
	TInt free = iRamPageAllocator->FreeRamInBytes();
	if(iRamCache)
		free += iRamCache->NumberOfFreePages()<<iPageShift;
	return free;
	}

/**	Returns the amount of free RAM currently available.

@return The number of bytes of free RAM currently available.
@pre	any context
 */
EXPORT_C TInt Kern::FreeRamInBytes()
	{
	return MmuBase::TheMmu->FreeRamInBytes();
	}


/**	Rounds up the argument to the size of a MMU page.

	To find out the size of a MMU page:
	@code
	size = Kern::RoundToPageSize(1);
	@endcode

	@param aSize Value to round up
	@pre any context
 */
EXPORT_C TUint32 Kern::RoundToPageSize(TUint32 aSize)
	{
	return MmuBase::RoundToPageSize(aSize);
	}


/**	Rounds up the argument to the amount of memory mapped by a MMU page 
	directory entry.

	Chunks occupy one or more consecutive page directory entries (PDE) and
	therefore the amount of linear and physical memory allocated to a chunk is
	always a multiple of the amount of memory mapped by a page directory entry.
 */
EXPORT_C TUint32 Kern::RoundToChunkSize(TUint32 aSize)
	{
	return MmuBase::RoundToChunkSize(aSize);
	}


/**
Allows the variant to specify the details of the RAM zones. This should be invoked 
by the variant in its implementation of the pure virtual function Asic::Init1().

There are some limitations to how the RAM zones can be specified:
- Each RAM zone's address space must be distinct and not overlap with any 
other RAM zone's address space
- Each RAM zone's address space must have a size that is multiples of the 
ASIC's MMU small page size and be aligned to the ASIC's MMU small page size, 
usually 4KB on ARM MMUs.
- When taken together all of the RAM zones must cover the whole of the physical RAM
address space as specified by the bootstrap in the SuperPage members iTotalRamSize
and iRamBootData;.
- There can be no more than KMaxRamZones RAM zones specified by the base port

Note the verification of the RAM zone data is not performed here but by the ram 
allocator later in the boot up sequence.  This is because it is only possible to
verify the zone data once the physical RAM configuration has been read from 
the super page. Any verification errors result in a "RAM-ALLOC" panic 
faulting the kernel during initialisation.

@param aZones Pointer to an array of SRamZone structs containing the details for all 
the zones. The end of the array is specified by an element with an iSize of zero. The array must 
remain in memory at least until the kernel has successfully booted.

@param aCallback Pointer to a call back function that the kernel may invoke to request 
one of the operations specified by TRamZoneOp.

@return KErrNone if successful, otherwise one of the system wide error codes

@see TRamZoneOp
@see SRamZone
@see TRamZoneCallback
*/
EXPORT_C TInt Epoc::SetRamZoneConfig(const SRamZone* aZones, TRamZoneCallback aCallback)
	{
	// Ensure this is only called once and only while we are initialising the kernel
	if (!K::Initialising || MmuBase::RamZoneConfig != NULL)
		{// fault kernel, won't return
		K::Fault(K::EBadSetRamZoneConfig);
		}

	if (NULL == aZones)
		{
		return KErrArgument;
		}
	MmuBase::RamZoneConfig=aZones;
	MmuBase::RamZoneCallback=aCallback;
	return KErrNone;
	}


/**
Modify the specified RAM zone's flags.

This allows the BSP or device driver to configure which type of pages, if any,
can be allocated into a RAM zone by the system.

Note: updating a RAM zone's flags can result in
	1 - memory allocations failing despite there being enough free RAM in the system.
	2 - the methods TRamDefragRequest::EmptyRamZone(), TRamDefragRequest::ClaimRamZone()
	or TRamDefragRequest::DefragRam() never succeeding.

The flag masks KRamZoneFlagDiscardOnly, KRamZoneFlagMovAndDisOnly and KRamZoneFlagNoAlloc
are intended to be used with this method.

@param aId			The ID of the RAM zone to modify.
@param aClearMask	The bit mask to clear, each flag of which must already be set on the RAM zone.
@param aSetMask		The bit mask to set.

@return KErrNone on success, KErrArgument if the RAM zone of aId not found or if 
aSetMask contains invalid flag bits.

@see TRamDefragRequest::EmptyRamZone()
@see TRamDefragRequest::ClaimRamZone()
@see TRamDefragRequest::DefragRam()

@see KRamZoneFlagDiscardOnly
@see KRamZoneFlagMovAndDisOnly
@see KRamZoneFlagNoAlloc
*/
EXPORT_C TInt Epoc::ModifyRamZoneFlags(TUint aId, TUint aClearMask, TUint aSetMask)
	{
	MmuBase& m = *MmuBase::TheMmu;
	MmuBase::Wait();

	TInt ret = m.ModifyRamZoneFlags(aId, aClearMask, aSetMask);

	MmuBase::Signal();
	return ret;
	}

TInt MmuBase::ModifyRamZoneFlags(TUint aId, TUint aClearMask, TUint aSetMask)
	{
	return iRamPageAllocator->ModifyZoneFlags(aId, aClearMask, aSetMask);
	}


/**
Gets the current count of a particular RAM zone's pages by type.

@param aId The ID of the RAM zone to enquire about
@param aPageData If successful, on return this contains the page count

@return KErrNone if successful, KErrArgument if a RAM zone of aId is not found or
one of the system wide error codes 

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.

@see SRamZonePageCount
*/
EXPORT_C TInt Epoc::GetRamZonePageCount(TUint aId, SRamZonePageCount& aPageData)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::GetRamZonePageCount");

	MmuBase& m = *MmuBase::TheMmu;
	MmuBase::Wait(); // Gets RAM alloc mutex

	TInt r = m.GetRamZonePageCount(aId, aPageData);

	MmuBase::Signal();	

	return r;
	}

TInt MmuBase::GetRamZonePageCount(TUint aId, SRamZonePageCount& aPageData)
	{
	return iRamPageAllocator->GetZonePageCount(aId, aPageData);
	}

/**
Replace a page of the system's execute-in-place (XIP) ROM image with a page of
RAM having the same contents. This RAM can subsequently be written to in order
to apply patches to the XIP ROM or to insert software breakpoints for debugging
purposes.
Call Epoc::FreeShadowPage() when you wish to revert to the original ROM page.

@param	aRomAddr	The virtual address of the ROM page to be replaced.
@return	KErrNone if the operation completed successfully.
		KErrArgument if the specified address is not a valid XIP ROM address.
		KErrNoMemory if the operation failed due to insufficient free RAM.
		KErrAlreadyExists if the XIP ROM page at the specified address has
			already been shadowed by a RAM page.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
*/
EXPORT_C TInt Epoc::AllocShadowPage(TLinAddr aRomAddr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::AllocShadowPage");

	TInt r;
	r=M::LockRegion(aRomAddr,1);
	if(r!=KErrNone && r!=KErrNotFound)
		return r;
	MmuBase& m=*MmuBase::TheMmu;
	MmuBase::Wait();
	r=m.AllocShadowPage(aRomAddr);
	MmuBase::Signal();
	if(r!=KErrNone)
		M::UnlockRegion(aRomAddr,1);
	return r;
	}

/**
Copies data into shadow memory. Source data is presumed to be in Kernel memory.

@param	aSrc	Data to copy from.
@param	aDest	Address to copy into.
@param	aLength	Number of bytes to copy. Maximum of 32 bytes of data can be copied.
.
@return	KErrNone 		if the operation completed successfully.
		KErrArgument 	if any part of destination region is not shadow page or
						if aLength is greater then 32 bytes.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
*/
EXPORT_C TInt Epoc::CopyToShadowMemory(TLinAddr aDest, TLinAddr aSrc, TUint32 aLength)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::CopyToShadowMemory");

	if (aLength>32)
		return KErrArgument;
	MmuBase& m=*MmuBase::TheMmu;
	// This is a simple copy operation except on platforms with __CPU_MEMORY_TYPE_REMAPPING defined,
	// where shadow page is read-only and it has to be remapped before it is written into.
	return m.CopyToShadowMemory(aDest, aSrc, aLength);
	}
/**
Revert an XIP ROM address which has previously been shadowed to the original
page of ROM.

@param	aRomAddr	The virtual address of the ROM page to be reverted.
@return	KErrNone if the operation completed successfully.
		KErrArgument if the specified address is not a valid XIP ROM address.
		KErrGeneral if the specified address has not previously been shadowed
			using Epoc::AllocShadowPage().

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
*/
EXPORT_C TInt Epoc::FreeShadowPage(TLinAddr aRomAddr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::FreeShadowPage");
	MmuBase& m=*MmuBase::TheMmu;
	MmuBase::Wait();
	TInt r=m.FreeShadowPage(aRomAddr);
	MmuBase::Signal();
	if(r==KErrNone)
		M::UnlockRegion(aRomAddr,1);
	return r;
	}


/**
Change the permissions on an XIP ROM address which has previously been shadowed
by a RAM page so that the RAM page may no longer be written to.

Note: Shadow page on the latest platforms (that use the reduced set of access permissions:
arm11mpcore, arm1176, cortex) is implemented with read only permissions. Therefore, calling
this function in not necessary, as shadow page is already created as 'frozen'.

@param	aRomAddr	The virtual address of the shadow RAM page to be frozen.
@return	KErrNone if the operation completed successfully.
		KErrArgument if the specified address is not a valid XIP ROM address.
		KErrGeneral if the specified address has not previously been shadowed
			using Epoc::AllocShadowPage().

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
*/
EXPORT_C TInt Epoc::FreezeShadowPage(TLinAddr aRomAddr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::FreezeShadowPage");
	MmuBase& m=*MmuBase::TheMmu;
	MmuBase::Wait();
	TInt r=m.FreezeShadowPage(aRomAddr);
	MmuBase::Signal();
	return r;
	}


/**
Allocate a block of physically contiguous RAM with a physical address aligned
to a specified power of 2 boundary.
When the RAM is no longer required it should be freed using
Epoc::FreePhysicalRam()

@param	aSize		The size in bytes of the required block. The specified size
					is rounded up to the page size, since only whole pages of
					physical RAM can be allocated.
@param	aPhysAddr	Receives the physical address of the base of the block on
					successful allocation.
@param	aAlign		Specifies the number of least significant bits of the
					physical address which are required to be zero. If a value
					less than log2(page size) is specified, page alignment is
					assumed. Pass 0 for aAlign if there are no special alignment
					constraints (other than page alignment).
@return	KErrNone if the allocation was successful.
		KErrNoMemory if a sufficiently large physically contiguous block of free
		RAM	with the specified alignment could not be found.
@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::AllocPhysicalRam(TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::AllocPhysicalRam");
	MmuBase& m=*MmuBase::TheMmu;
	MmuBase::Wait();
	TInt r=m.AllocPhysicalRam(aSize,aPhysAddr,aAlign);
	if (r == KErrNone)
		{
		// For the sake of platform security we have to clear the memory. E.g. the driver
		// could assign it to a chunk visible to user side.
		m.ClearPages(Kern::RoundToPageSize(aSize)>>m.iPageShift, (TPhysAddr*)(aPhysAddr|1));
#ifdef BTRACE_KERNEL_MEMORY
		TUint size = Kern::RoundToPageSize(aSize);
		BTrace8(BTrace::EKernelMemory, BTrace::EKernelMemoryDrvPhysAlloc, size, aPhysAddr);
		Epoc::DriverAllocdPhysRam += size;
#endif
		}
	MmuBase::Signal();
	return r;
	}

/**
Allocate a block of physically contiguous RAM with a physical address aligned
to a specified power of 2 boundary from the specified zone.
When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

Note that this method only repsects the KRamZoneFlagNoAlloc flag and will always attempt
to allocate regardless of whether the other flags are set for the specified RAM zones 
or not.

When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

@param 	aZoneId		The ID of the zone to attempt to allocate from.
@param	aSize		The size in bytes of the required block. The specified size
					is rounded up to the page size, since only whole pages of
					physical RAM can be allocated.
@param	aPhysAddr	Receives the physical address of the base of the block on
					successful allocation.
@param	aAlign		Specifies the number of least significant bits of the
					physical address which are required to be zero. If a value
					less than log2(page size) is specified, page alignment is
					assumed. Pass 0 for aAlign if there are no special alignment
					constraints (other than page alignment).
@return	KErrNone if the allocation was successful.
		KErrNoMemory if a sufficiently large physically contiguous block of free
		RAM	with the specified alignment could not be found within the specified 
		zone.
		KErrArgument if a RAM zone of the specified ID can't be found or if the
		RAM zone has a total number of physical pages which is less than those 
		requested for the allocation.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::ZoneAllocPhysicalRam(TUint aZoneId, TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign)
	{
	return ZoneAllocPhysicalRam(&aZoneId, 1, aSize, aPhysAddr, aAlign);
	}


/**
Allocate a block of physically contiguous RAM with a physical address aligned
to a specified power of 2 boundary from the specified RAM zones.
When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

RAM will be allocated into the RAM zones in the order they are specified in the 
aZoneIdList parameter. If the contiguous allocations are intended to span RAM zones 
when required then aZoneIdList should be listed with the RAM zones in ascending 
physical address order.

Note that this method only repsects the KRamZoneFlagNoAlloc flag and will always attempt
to allocate regardless of whether the other flags are set for the specified RAM zones 
or not.

When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

@param 	aZoneIdList	A pointer to an array of RAM zone IDs of the RAM zones to 
					attempt to allocate from.
@param 	aZoneIdCount The number of RAM zone IDs contained in aZoneIdList.
@param	aSize		The size in bytes of the required block. The specified size
					is rounded up to the page size, since only whole pages of
					physical RAM can be allocated.
@param	aPhysAddr	Receives the physical address of the base of the block on
					successful allocation.
@param	aAlign		Specifies the number of least significant bits of the
					physical address which are required to be zero. If a value
					less than log2(page size) is specified, page alignment is
					assumed. Pass 0 for aAlign if there are no special alignment
					constraints (other than page alignment).
@return	KErrNone if the allocation was successful.
		KErrNoMemory if a sufficiently large physically contiguous block of free
		RAM	with the specified alignment could not be found within the specified 
		zone.
		KErrArgument if a RAM zone of a specified ID can't be found or if the
		RAM zones have a total number of physical pages which is less than those 
		requested for the allocation.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::ZoneAllocPhysicalRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::ZoneAllocPhysicalRam");
	MmuBase& m=*MmuBase::TheMmu;
	MmuBase::Wait();
	TInt r = m.ZoneAllocPhysicalRam(aZoneIdList, aZoneIdCount, aSize, aPhysAddr, aAlign);
	if (r == KErrNone)
		{
		// For the sake of platform security we have to clear the memory. E.g. the driver
		// could assign it to a chunk visible to user side.
		m.ClearPages(Kern::RoundToPageSize(aSize)>>m.iPageShift, (TPhysAddr*)(aPhysAddr|1));
#ifdef BTRACE_KERNEL_MEMORY
		TUint size = Kern::RoundToPageSize(aSize);
		BTrace8(BTrace::EKernelMemory, BTrace::EKernelMemoryDrvPhysAlloc, size, aPhysAddr);
		Epoc::DriverAllocdPhysRam += size;
#endif
		}
	MmuBase::Signal();
	return r;
	}


/**
Attempt to allocate discontiguous RAM pages.

When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

@param	aNumPages	The number of discontiguous pages required to be allocated
@param	aPageList	This should be a pointer to a previously allocated array of
					aNumPages TPhysAddr elements.  On a succesful allocation it 
					will receive the physical addresses of each page allocated.

@return	KErrNone if the allocation was successful.
		KErrNoMemory if the requested number of pages can't be allocated

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::AllocPhysicalRam(TInt aNumPages, TPhysAddr* aPageList)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "Epoc::AllocPhysicalRam");
	MmuBase& m = *MmuBase::TheMmu;
	MmuBase::Wait();
	TInt r = m.AllocPhysicalRam(aNumPages, aPageList);
	if (r == KErrNone)
		{
		// For the sake of platform security we have to clear the memory. E.g. the driver
		// could assign it to a chunk visible to user side.
		m.ClearPages(aNumPages, aPageList);

#ifdef BTRACE_KERNEL_MEMORY
		if (BTrace::CheckFilter(BTrace::EKernelMemory))
			{// Only loop round each page if EKernelMemory tracing is enabled
			TPhysAddr* pAddr = aPageList;
			TPhysAddr* pAddrEnd = aPageList + aNumPages;
			while (pAddr < pAddrEnd)
				{
				BTrace8(BTrace::EKernelMemory, BTrace::EKernelMemoryDrvPhysAlloc, KPageSize, *pAddr++);
				Epoc::DriverAllocdPhysRam += KPageSize;
				}
			}
#endif
		}
	MmuBase::Signal();
	return r;
	}


/**
Attempt to allocate discontiguous RAM pages from the specified zone.

Note that this method only repsects the KRamZoneFlagNoAlloc flag and will always attempt
to allocate regardless of whether the other flags are set for the specified RAM zones 
or not.

When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

@param 	aZoneId		The ID of the zone to attempt to allocate from.
@param	aNumPages	The number of discontiguous pages required to be allocated 
					from the specified zone.
@param	aPageList	This should be a pointer to a previously allocated array of
					aNumPages TPhysAddr elements.  On a succesful 
					allocation it will receive the physical addresses of each 
					page allocated.
@return	KErrNone if the allocation was successful.
		KErrNoMemory if the requested number of pages can't be allocated from the 
		specified zone.
		KErrArgument if a RAM zone of the specified ID can't be found or if the
		RAM zone has a total number of physical pages which is less than those 
		requested for the allocation.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::ZoneAllocPhysicalRam(TUint aZoneId, TInt aNumPages, TPhysAddr* aPageList)
	{
	return ZoneAllocPhysicalRam(&aZoneId, 1, aNumPages, aPageList);
	}


/**
Attempt to allocate discontiguous RAM pages from the specified RAM zones.
The RAM pages will be allocated into the RAM zones in the order that they are specified 
in the aZoneIdList parameter, the RAM zone preferences will be ignored.

Note that this method only repsects the KRamZoneFlagNoAlloc flag and will always attempt
to allocate regardless of whether the other flags are set for the specified RAM zones 
or not.

When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

@param 	aZoneIdList	A pointer to an array of RAM zone IDs of the RAM zones to 
					attempt to allocate from.
@param	aZoneIdCount The number of RAM zone IDs pointed to by aZoneIdList.
@param	aNumPages	The number of discontiguous pages required to be allocated 
					from the specified zone.
@param	aPageList	This should be a pointer to a previously allocated array of
					aNumPages TPhysAddr elements.  On a succesful 
					allocation it will receive the physical addresses of each 
					page allocated.
@return	KErrNone if the allocation was successful.
		KErrNoMemory if the requested number of pages can't be allocated from the 
		specified zone.
		KErrArgument if a RAM zone of a specified ID can't be found or if the
		RAM zones have a total number of physical pages which is less than those 
		requested for the allocation.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::ZoneAllocPhysicalRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aNumPages, TPhysAddr* aPageList)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "Epoc::ZoneAllocPhysicalRam");
	MmuBase& m = *MmuBase::TheMmu;
	MmuBase::Wait();
	TInt r = m.ZoneAllocPhysicalRam(aZoneIdList, aZoneIdCount, aNumPages, aPageList);
	if (r == KErrNone)
		{
		// For the sake of platform security we have to clear the memory. E.g. the driver
		// could assign it to a chunk visible to user side.
		m.ClearPages(aNumPages, aPageList);

#ifdef BTRACE_KERNEL_MEMORY
		if (BTrace::CheckFilter(BTrace::EKernelMemory))
			{// Only loop round each page if EKernelMemory tracing is enabled
			TPhysAddr* pAddr = aPageList;
			TPhysAddr* pAddrEnd = aPageList + aNumPages;
			while (pAddr < pAddrEnd)
				{
				BTrace8(BTrace::EKernelMemory, BTrace::EKernelMemoryDrvPhysAlloc, KPageSize, *pAddr++);
				Epoc::DriverAllocdPhysRam += KPageSize;
				}
			}
#endif
		}
	MmuBase::Signal();
	return r;
	}

/**
Free a previously-allocated block of physically contiguous RAM.

Specifying one of the following may cause the system to panic: 
a) an invalid physical RAM address.
b) valid physical RAM addresses where some had not been previously allocated.
c) an adrress not aligned to a page boundary.

@param	aPhysAddr	The physical address of the base of the block to be freed.
					This must be the address returned by a previous call to
					Epoc::AllocPhysicalRam(), Epoc::ZoneAllocPhysicalRam(), 
					Epoc::ClaimPhysicalRam() or Epoc::ClaimRamZone().
@param	aSize		The size in bytes of the required block. The specified size
					is rounded up to the page size, since only whole pages of
					physical RAM can be allocated.
@return	KErrNone if the operation was successful.



@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::FreePhysicalRam(TPhysAddr aPhysAddr, TInt aSize)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::FreePhysicalRam");
	MmuBase& m=*MmuBase::TheMmu;
	MmuBase::Wait();
	TInt r=m.FreePhysicalRam(aPhysAddr,aSize);
#ifdef BTRACE_KERNEL_MEMORY
	if (r == KErrNone)
		{
		TUint size = Kern::RoundToPageSize(aSize);
		BTrace8(BTrace::EKernelMemory, BTrace::EKernelMemoryDrvPhysFree, size, aPhysAddr);
		Epoc::DriverAllocdPhysRam -= size;
		}
#endif
	MmuBase::Signal();
	return r;
	}


/**
Free a number of physical RAM pages that were previously allocated using
Epoc::AllocPhysicalRam() or Epoc::ZoneAllocPhysicalRam().

Specifying one of the following may cause the system to panic: 
a) an invalid physical RAM address.
b) valid physical RAM addresses where some had not been previously allocated.
c) an adrress not aligned to a page boundary.

@param	aNumPages	The number of pages to be freed.
@param	aPhysAddr	An array of aNumPages TPhysAddr elements.  Where each element
					should contain the physical address of each page to be freed.
					This must be the same set of addresses as those returned by a 
					previous call to Epoc::AllocPhysicalRam() or 
					Epoc::ZoneAllocPhysicalRam().
@return	KErrNone if the operation was successful.
  
@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
		
*/
EXPORT_C TInt Epoc::FreePhysicalRam(TInt aNumPages, TPhysAddr* aPageList)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::FreePhysicalRam");
	MmuBase& m=*MmuBase::TheMmu;
	MmuBase::Wait();
	TInt r=m.FreePhysicalRam(aNumPages, aPageList);
#ifdef BTRACE_KERNEL_MEMORY
	if (r == KErrNone && BTrace::CheckFilter(BTrace::EKernelMemory))
		{// Only loop round each page if EKernelMemory tracing is enabled
		TPhysAddr* pAddr = aPageList;
		TPhysAddr* pAddrEnd = aPageList + aNumPages;
		while (pAddr < pAddrEnd)
			{
			BTrace8(BTrace::EKernelMemory, BTrace::EKernelMemoryDrvPhysFree, KPageSize, *pAddr++);
			Epoc::DriverAllocdPhysRam -= KPageSize;
			}
		}
#endif
	MmuBase::Signal();
	return r;
	}


/**
Allocate a specific block of physically contiguous RAM, specified by physical
base address and size.
If and when the RAM is no longer required it should be freed using
Epoc::FreePhysicalRam()

@param	aPhysAddr	The physical address of the base of the required block.
@param	aSize		The size in bytes of the required block. The specified size
					is rounded up to the page size, since only whole pages of
					physical RAM can be allocated.
@return	KErrNone if the operation was successful.
		KErrArgument if the range of physical addresses specified included some
					which are not valid physical RAM addresses.
		KErrInUse	if the range of physical addresses specified are all valid
					physical RAM addresses but some of them have already been
					allocated for other purposes.
@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::ClaimPhysicalRam(TPhysAddr aPhysAddr, TInt aSize)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::ClaimPhysicalRam");
	MmuBase& m=*MmuBase::TheMmu;
	MmuBase::Wait();
	TInt r=m.ClaimPhysicalRam(aPhysAddr,aSize);
#ifdef BTRACE_KERNEL_MEMORY
	if(r==KErrNone)
		{
		TUint32 pa=aPhysAddr;
		TUint32 size=aSize;
		m.RoundUpRangeToPageSize(pa,size);
		BTrace8(BTrace::EKernelMemory, BTrace::EKernelMemoryDrvPhysAlloc, size, pa);
		Epoc::DriverAllocdPhysRam += size;
		}
#endif
	MmuBase::Signal();
	return r;
	}


/**
Free a RAM zone which was previously allocated by one of these methods:
Epoc::AllocPhysicalRam(), Epoc::ZoneAllocPhysicalRam() or 
TRamDefragRequest::ClaimRamZone().

All of the pages in the RAM zone must be allocated and only via one of the methods 
listed above, otherwise a system panic will occur.

@param	aZoneId			The ID of the RAM zone to free.
@return	KErrNone 		If the operation was successful.
		KErrArgument 	If a RAM zone with ID aZoneId was not found.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::FreeRamZone(TUint aZoneId)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::FreeRamZone");
	MmuBase& m = *MmuBase::TheMmu;
	MmuBase::Wait();
	TPhysAddr zoneBase;
	TUint zoneBytes;
	TInt r = m.FreeRamZone(aZoneId, zoneBase, zoneBytes);
#ifdef BTRACE_KERNEL_MEMORY
	if (r == KErrNone)
		{
		BTrace8(BTrace::EKernelMemory, BTrace::EKernelMemoryDrvPhysFree, zoneBytes, zoneBase);
		Epoc::DriverAllocdPhysRam -= zoneBytes;
		}
#endif
	MmuBase::Signal();
	return r;
	}


/**
Translate a virtual address to the corresponding physical address.

@param	aLinAddr	The virtual address to be translated.
@return	The physical address corresponding to the given virtual address, or
		KPhysAddrInvalid if the specified virtual address is unmapped.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre Call in a thread context.
@pre Can be used in a device driver.
@pre Hold system lock if there is any possibility that the virtual address is
		unmapped, may become unmapped, or may be remapped during the operation.
	This will potentially be the case unless the virtual address refers to a
	hardware chunk or shared chunk under the control of the driver calling this
	function.
*/
EXPORT_C TPhysAddr Epoc::LinearToPhysical(TLinAddr aLinAddr)
	{
//	This precondition is violated by various parts of the system under some conditions,
//	e.g. when __FLUSH_PT_INTO_RAM__ is defined. This function might also be called by
//	a higher-level RTOS for which these conditions are meaningless. Thus, it's been
//	disabled for now.
//	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,"Epoc::LinearToPhysical");
	MmuBase& m=*MmuBase::TheMmu;
	TPhysAddr pa=m.LinearToPhysical(aLinAddr);
	return pa;
	}


EXPORT_C TInt TInternalRamDrive::MaxSize()
	{
	return TheSuperPage().iRamDriveSize+Kern::FreeRamInBytes();
	}


/******************************************************************************
 * Address allocator
 ******************************************************************************/
TLinearSection* TLinearSection::New(TLinAddr aBase, TLinAddr aEnd)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("TLinearSection::New(%08x,%08x)", aBase, aEnd));
	MmuBase& m=*MmuBase::TheMmu;
	TUint npdes=(aEnd-aBase)>>m.iChunkShift;
	TInt nmapw=(npdes+31)>>5;
	TInt memsz=sizeof(TLinearSection)+(nmapw-1)*sizeof(TUint32);
	TLinearSection* p=(TLinearSection*)Kern::Alloc(memsz);
	if (p)
		{
		new(&p->iAllocator) TBitMapAllocator(npdes, ETrue);
		p->iBase=aBase;
		p->iEnd=aEnd;
		}
	__KTRACE_OPT(KMMU,Kern::Printf("TLinearSection at %08x", p));
	return p;
	}

/******************************************************************************
 * Address allocator for HW chunks
 ******************************************************************************/
THwChunkPageTable::THwChunkPageTable(TInt aIndex, TInt aSize, TPde aPdePerm)
	:	THwChunkRegion(aIndex, 0, aPdePerm),
		iAllocator(aSize, ETrue)
	{
	}

THwChunkPageTable* THwChunkPageTable::New(TInt aIndex, TPde aPdePerm)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("THwChunkPageTable::New(%03x,%08x)",aIndex,aPdePerm));
	MmuBase& m=*MmuBase::TheMmu;
	TInt pdepages=m.iChunkSize>>m.iPageShift;
	TInt nmapw=(pdepages+31)>>5;
	TInt memsz=sizeof(THwChunkPageTable)+(nmapw-1)*sizeof(TUint32);
	THwChunkPageTable* p=(THwChunkPageTable*)Kern::Alloc(memsz);
	if (p)
		new (p) THwChunkPageTable(aIndex, pdepages, aPdePerm);
	__KTRACE_OPT(KMMU, Kern::Printf("THwChunkPageTable at %08x",p));
	return p;
	}

THwChunkAddressAllocator::THwChunkAddressAllocator()
	{
	}

THwChunkAddressAllocator* THwChunkAddressAllocator::New(TInt aAlign, TLinearSection* aSection)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("THwChunkAddressAllocator::New(%d,%08x)",aAlign,aSection));
	THwChunkAddressAllocator* p=new THwChunkAddressAllocator;
	if (p)
		{
		p->iAlign=aAlign;
		p->iSection=aSection;
		}
	__KTRACE_OPT(KMMU, Kern::Printf("THwChunkAddressAllocator at %08x",p));
	return p;
	}

THwChunkRegion* THwChunkAddressAllocator::NewRegion(TInt aIndex, TInt aSize, TPde aPdePerm)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("THwChAA::NewRegion(index=%x, size=%x, pde=%08x)",aIndex,aSize,aPdePerm));
	THwChunkRegion* p=new THwChunkRegion(aIndex, aSize, aPdePerm);
	if (p)
		{
		TInt r=InsertInOrder(p, Order);
		__KTRACE_OPT(KMMU, Kern::Printf("p=%08x, insert ret %d",p,r));
		if (r<0)
			delete p, p=NULL;
		}
	__KTRACE_OPT(KMMU, Kern::Printf("THwChAA::NewRegion ret %08x)",p));
	return p;
	}

THwChunkPageTable* THwChunkAddressAllocator::NewPageTable(TInt aIndex, TPde aPdePerm, TInt aInitB, TInt aInitC)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("THwChAA::NewPageTable(index=%x, pde=%08x, iB=%d, iC=%d)",aIndex,aPdePerm,aInitB,aInitC));
	THwChunkPageTable* p=THwChunkPageTable::New(aIndex, aPdePerm);
	if (p)
		{
		TInt r=InsertInOrder(p, Order);
		__KTRACE_OPT(KMMU, Kern::Printf("p=%08x, insert ret %d",p,r));
		if (r<0)
			delete p, p=NULL;
		else
			p->iAllocator.Alloc(aInitB, aInitC);
		}
	__KTRACE_OPT(KMMU, Kern::Printf("THwChAA::NewPageTable ret %08x)",p));
	return p;
	}

TLinAddr THwChunkAddressAllocator::SearchExisting(TInt aNumPages, TInt aPageAlign, TInt aPageOffset, TPde aPdePerm)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("THwChAA::SrchEx np=%03x align=%d offset=%03x pdeperm=%08x",
				aNumPages, aPageAlign, aPageOffset, aPdePerm));
	TInt c=Count();
	if (c==0)
		return 0;	// don't try to access [0] if array empty!
	THwChunkPageTable** pp=(THwChunkPageTable**)&(*this)[0];
	THwChunkPageTable** ppE=pp+c;
	while(pp<ppE)
		{
		THwChunkPageTable* p=*pp++;
		if (p->iRegionSize!=0 || p->iPdePerm!=aPdePerm)
			continue;	// if not page table or PDE permissions wrong, we can't use it
		TInt r=p->iAllocator.AllocAligned(aNumPages, aPageAlign, -aPageOffset, EFalse);
		__KTRACE_OPT(KMMU, Kern::Printf("r=%d", r));
		if (r<0)
			continue;	// not enough space in this page table
		
		// got enough space in existing page table, so use it
		p->iAllocator.Alloc(r, aNumPages);
		MmuBase& m=*MmuBase::TheMmu;
		TLinAddr a = iSection->iBase + (TLinAddr(p->iIndex)<<m.iChunkShift) + (r<<m.iPageShift);
		__KTRACE_OPT(KMMU, Kern::Printf("THwChAA::SrchEx OK, returning %08x", a));
		return a;
		}
	__KTRACE_OPT(KMMU, Kern::Printf("THwChAA::SrchEx not found"));
	return 0;
	}

TLinAddr THwChunkAddressAllocator::Alloc(TInt aSize, TInt aAlign, TInt aOffset, TPde aPdePerm)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("THwChAA::Alloc size=%08x align=%d offset=%08x pdeperm=%08x",
				aSize, aAlign, aOffset, aPdePerm));
	MmuBase& m=*MmuBase::TheMmu;
	TInt npages=(aSize+m.iPageMask)>>m.iPageShift;
	TInt align=Max(aAlign,iAlign);
	if (align>m.iChunkShift)
		return 0;
	TInt aligns=1<<align;
	TInt alignm=aligns-1;
	TInt offset=(aOffset&alignm)>>m.iPageShift;
	TInt pdepages=m.iChunkSize>>m.iPageShift;
	TInt pdepageshift=m.iChunkShift-m.iPageShift;
	MmuBase::WaitHwChunk();
	if (npages<pdepages)
		{
		// for small regions, first try to share an existing page table
		TLinAddr a=SearchExisting(npages, align-m.iPageShift, offset, aPdePerm);
		if (a)
			{
			MmuBase::SignalHwChunk();
			return a;
			}
		}

	// large region or no free space in existing page tables - allocate whole PDEs
	TInt npdes=(npages+offset+pdepages-1)>>pdepageshift;
	__KTRACE_OPT(KMMU, Kern::Printf("Allocate %d PDEs", npdes));
	MmuBase::Wait();
	TInt ix=iSection->iAllocator.AllocConsecutive(npdes, EFalse);
	if (ix>=0)
		iSection->iAllocator.Alloc(ix, npdes);
	MmuBase::Signal();
	TLinAddr a=0;
	if (ix>=0)
		a = iSection->iBase + (TLinAddr(ix)<<m.iChunkShift) + (TLinAddr(offset)<<m.iPageShift);

	// Create bitmaps for each page table and placeholders for section blocks.
	// We only create a bitmap for the first and last PDE and then only if they are not
	// fully occupied by this request
	THwChunkPageTable* first=NULL;
	THwChunkRegion* middle=NULL;
	TInt remain=npages;
	TInt nix=ix;
	if (a && (offset || npages<pdepages))
		{
		// first PDE is bitmap
		TInt first_count = Min(remain, pdepages-offset);
		first=NewPageTable(nix, aPdePerm, offset, first_count);
		++nix;
		remain -= first_count;
		if (!first)
			a=0;
		}
	if (a && remain>=pdepages)
		{
		// next need whole-PDE-block placeholder
		TInt whole_pdes=remain>>pdepageshift;
		middle=NewRegion(nix, whole_pdes, aPdePerm);
		nix+=whole_pdes;
		remain-=(whole_pdes<<pdepageshift);
		if (!middle)
			a=0;
		}
	if (a && remain)
		{
		// need final bitmap section
		if (!NewPageTable(nix, aPdePerm, 0, remain))
			a=0;
		}
	if (!a)
		{
		// alloc failed somewhere - free anything we did create
		if (middle)
			Discard(middle);
		if (first)
			Discard(first);
		if (ix>=0)
			{
			MmuBase::Wait();
			iSection->iAllocator.Free(ix, npdes);
			MmuBase::Signal();
			}
		}
	MmuBase::SignalHwChunk();
	__KTRACE_OPT(KMMU, Kern::Printf("THwChAA::Alloc returns %08x", a));
	return a;
	}

void THwChunkAddressAllocator::Discard(THwChunkRegion* aRegion)
	{
	// remove a region from the array and destroy it
	TInt r=FindInOrder(aRegion, Order);
	if (r>=0)
		Remove(r);
	Kern::Free(aRegion);
	}

TInt THwChunkAddressAllocator::Order(const THwChunkRegion& a1, const THwChunkRegion& a2)
	{
	// order two regions by address
	return a1.iIndex-a2.iIndex;
	}

THwChunkRegion* THwChunkAddressAllocator::Free(TLinAddr aAddr, TInt aSize)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("THwChAA::Free addr=%08x size=%08x", aAddr, aSize));
	__ASSERT_ALWAYS(aAddr>=iSection->iBase && (aAddr+aSize)<=iSection->iEnd,
										MmuBase::Panic(MmuBase::EFreeHwChunkAddrInvalid));
	THwChunkRegion* list=NULL;
	MmuBase& m=*MmuBase::TheMmu;
	TInt ix=(aAddr - iSection->iBase)>>m.iChunkShift;
	TInt remain=(aSize+m.iPageMask)>>m.iPageShift;
	TInt pdepageshift=m.iChunkShift-m.iPageShift;
	TInt offset=(aAddr&m.iChunkMask)>>m.iPageShift;
	THwChunkRegion find(ix, 0, 0);
	MmuBase::WaitHwChunk();
	TInt r=FindInOrder(&find, Order);
	__ASSERT_ALWAYS(r>=0, MmuBase::Panic(MmuBase::EFreeHwChunkAddrInvalid));
	while (remain)
		{
		THwChunkPageTable* p=(THwChunkPageTable*)(*this)[r];
		__ASSERT_ALWAYS(p->iIndex==ix, MmuBase::Panic(MmuBase::EFreeHwChunkIndexInvalid));
		if (p->iRegionSize)
			{
			// multiple-whole-PDE region
			TInt rsz=p->iRegionSize;
			remain-=(rsz<<pdepageshift);
			Remove(r);	// r now indexes following array entry
			ix+=rsz;
			}
		else
			{
			// bitmap region
			TInt n=Min(remain, (1<<pdepageshift)-offset);
			p->iAllocator.Free(offset, n);
			remain-=n;
			++ix;
			if (p->iAllocator.iAvail < p->iAllocator.iSize)
				{
				// bitmap still in use
				offset=0;
				++r;	// r indexes following array entry
				continue;
				}
			Remove(r);	// r now indexes following array entry
			}
		offset=0;
		p->iNext=list;
		list=p;			// chain free region descriptors together
		}
	MmuBase::SignalHwChunk();
	__KTRACE_OPT(KMMU, Kern::Printf("THwChAA::Free returns %08x", list));
	return list;
	}

/********************************************
 * Hardware chunk abstraction
 ********************************************/
THwChunkAddressAllocator* MmuBase::MappingRegion(TUint)
	{
	return iHwChunkAllocator;
	}

TInt MmuBase::AllocateAllPageTables(TLinAddr aLinAddr, TInt aSize, TPde aPdePerm, TInt aMapShift, SPageTableInfo::TAttribs aAttrib)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("AllocateAllPageTables lin=%08x, size=%x, pde=%08x, mapshift=%d attribs=%d",
																aLinAddr, aSize, aPdePerm, aMapShift, aAttrib));
	TInt offset=aLinAddr&iChunkMask;
	TInt remain=aSize;
	TLinAddr a=aLinAddr&~iChunkMask;
	TInt newpts=0;
	for (; remain>0; a+=iChunkSize)
		{
		// don't need page table if a whole PDE mapping is permitted here
		if (aMapShift<iChunkShift || offset || remain<iChunkSize)
			{
			// need to check for a page table at a
			TInt id=PageTableId(a);
			if (id<0)
				{
				// no page table - must allocate one
				id = AllocPageTable();
				if (id<0)
					break;
				// got page table, assign it
				// AssignPageTable(TInt aId, TInt aUsage, TAny* aObject, TLinAddr aAddr, TPde aPdePerm)
				AssignPageTable(id, aAttrib, NULL, a, aPdePerm);
				++newpts;
				}
			}
		remain -= (iChunkSize-offset);
		offset=0;
		}
	if (remain<=0)
		return KErrNone;	// completed OK

	// ran out of memory somewhere - free page tables which were allocated
	for (; newpts; --newpts)
		{
		a-=iChunkSize;
		TInt id=UnassignPageTable(a);
		FreePageTable(id);
		}
	return KErrNoMemory;
	}


/**
Create a hardware chunk object mapping a specified block of physical addresses
with specified access permissions and cache policy.

When the mapping is no longer required, close the chunk using chunk->Close(0);
Note that closing a chunk does not free any RAM pages which were mapped by the
chunk - these must be freed separately using Epoc::FreePhysicalRam().

@param	aChunk	Upon successful completion this parameter receives a pointer to
				the newly created chunk. Upon unsuccessful completion it is
				written with a NULL pointer. The virtual address of the mapping
				can subsequently be discovered using the LinearAddress()
				function on the chunk.
@param	aAddr	The base address of the physical region to be mapped. This will
				be rounded down to a multiple of the hardware page size before
				being used.
@param	aSize	The size of the physical address region to be mapped. This will
				be rounded up to a multiple of the hardware page size before
				being used; the rounding is such that the entire range from
				aAddr to aAddr+aSize-1 inclusive is mapped. For example if
				aAddr=0xB0001FFF, aSize=2 and the hardware page size is 4KB, an
				8KB range of physical addresses from 0xB0001000 to 0xB0002FFF
				inclusive will be mapped.
@param	aMapAttr Mapping attributes required for the mapping. This is formed
				by ORing together values from the TMappingAttributes enumeration
				to specify the access permissions and caching policy.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
@see TMappingAttributes
*/
EXPORT_C TInt DPlatChunkHw::New(DPlatChunkHw*& aChunk, TPhysAddr aAddr, TInt aSize, TUint aMapAttr)
	{
	if (aAddr == KPhysAddrInvalid)
		return KErrNotSupported;
	return DoNew(aChunk, aAddr, aSize, aMapAttr);
	}

TInt DPlatChunkHw::DoNew(DPlatChunkHw*& aChunk, TPhysAddr aAddr, TInt aSize, TUint aMapAttr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DPlatChunkHw::New");
	__KTRACE_OPT(KMMU,Kern::Printf("DPlatChunkHw::New phys=%08x, size=%x, attribs=%x",aAddr,aSize,aMapAttr));
	if (aSize<=0)
		return KErrArgument;
	MmuBase& m=*MmuBase::TheMmu;
	aChunk=NULL;
	TPhysAddr pa=aAddr!=KPhysAddrInvalid ? aAddr&~m.iPageMask : 0;
	TInt size=((aAddr+aSize+m.iPageMask)&~m.iPageMask)-pa;
	__KTRACE_OPT(KMMU,Kern::Printf("Rounded %08x+%x", pa, size));
	DMemModelChunkHw* pC=new DMemModelChunkHw;
	if (!pC)
		return KErrNoMemory;
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunkHw created at %08x",pC));
	pC->iPhysAddr=aAddr;
	pC->iSize=size;
	TUint mapattr=aMapAttr;
	TPde pdePerm=0;
	TPte ptePerm=0;
	TInt r=m.PdePtePermissions(mapattr, pdePerm, ptePerm);
	if (r==KErrNone)
		{
		pC->iAllocator=m.MappingRegion(mapattr);
		pC->iAttribs=mapattr;	// save actual mapping attributes
		r=pC->AllocateLinearAddress(pdePerm);
		if (r>=0)
			{
			TInt map_shift=r;
			MmuBase::Wait();
			r=m.AllocateAllPageTables(pC->iLinAddr, size, pdePerm, map_shift, SPageTableInfo::EGlobal);
			if (r==KErrNone && aAddr!=KPhysAddrInvalid)
				m.Map(pC->iLinAddr, pa, size, pdePerm, ptePerm, map_shift);
			MmuBase::Signal();
			}
		}
	if (r==KErrNone)
		aChunk=pC;
	else
		pC->Close(NULL);
	return r;
	}

TInt DMemModelChunkHw::AllocateLinearAddress(TPde aPdePerm)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("DMemModelChunkHw::AllocateLinearAddress(%08x)", aPdePerm));
	__KTRACE_OPT(KMMU, Kern::Printf("iAllocator=%08x iPhysAddr=%08x iSize=%08x", iAllocator, iPhysAddr, iSize));
	MmuBase& m=*MmuBase::TheMmu;
	TInt map_shift = (iPhysAddr<0xffffffffu) ? 30 : m.iPageShift;
	for (; map_shift>=m.iPageShift; --map_shift)
		{
		TUint32 map_size = 1<<map_shift;
		TUint32 map_mask = map_size-1;
		if (!(m.iMapSizes & map_size))
			continue;	// map_size is not supported on this hardware
		TPhysAddr base = (iPhysAddr+map_mask) &~ map_mask;	// base rounded up
		TPhysAddr end = (iPhysAddr+iSize)&~map_mask;		// end rounded down
		if ((base-end)<0x80000000u && map_shift>m.iPageShift)
			continue;	// region not big enough to use this mapping size
		__KTRACE_OPT(KMMU, Kern::Printf("Try map size %08x", map_size));
		iLinAddr=iAllocator->Alloc(iSize, map_shift, iPhysAddr, aPdePerm);
		if (iLinAddr)
			break;		// done
		}
	TInt r=iLinAddr ? map_shift : KErrNoMemory;
	__KTRACE_OPT(KMMU, Kern::Printf("iLinAddr=%08x, returning %d", iLinAddr, r));
	return r;
	}

void DMemModelChunkHw::DeallocateLinearAddress()
	{
	__KTRACE_OPT(KMMU, Kern::Printf("DMemModelChunkHw::DeallocateLinearAddress %O", this));
	MmuBase& m=*MmuBase::TheMmu;
	MmuBase::WaitHwChunk();
	THwChunkRegion* rgn=iAllocator->Free(iLinAddr, iSize);
	iLinAddr=0;
	MmuBase::SignalHwChunk();
	TLinAddr base = iAllocator->iSection->iBase;
	TBitMapAllocator& section_allocator = iAllocator->iSection->iAllocator;
	while (rgn)
		{
		MmuBase::Wait();
		if (rgn->iRegionSize)
			{
			// free address range
			__KTRACE_OPT(KMMU, Kern::Printf("Freeing range %03x+%03x", rgn->iIndex, rgn->iRegionSize));
			section_allocator.Free(rgn->iIndex, rgn->iRegionSize);
			
			// Though this is large region, it still can be made up of page tables (not sections).
			// Check each chunk and remove tables in neccessary
			TInt i = 0;
			TLinAddr a = base + (TLinAddr(rgn->iIndex)<<m.iChunkShift);
			for (; i<rgn->iRegionSize ; i++,a+=m.iChunkSize)
				{
				TInt id = m.UnassignPageTable(a);
				if (id>=0)
					m.FreePageTable(id);
				}
			}
		else
			{
			// free address and page table if it exists
			__KTRACE_OPT(KMMU, Kern::Printf("Freeing index %03x", rgn->iIndex));
			section_allocator.Free(rgn->iIndex);
			TLinAddr a = base + (TLinAddr(rgn->iIndex)<<m.iChunkShift);
			TInt id = m.UnassignPageTable(a);
			if (id>=0)
				m.FreePageTable(id);
			}
		MmuBase::Signal();
		THwChunkRegion* free=rgn;
		rgn=rgn->iNext;
		Kern::Free(free);
		}
	}


//
// RamCacheBase
//


RamCacheBase* RamCacheBase::TheRamCache = NULL;


RamCacheBase::RamCacheBase()
	{
	}


void RamCacheBase::Init2()
	{
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf(">RamCacheBase::Init2"));
	iMmu = MmuBase::TheMmu;
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("<RamCacheBase::Init2"));
	}


void RamCacheBase::ReturnToSystem(SPageInfo* aPageInfo)
	{
	__ASSERT_MUTEX(MmuBase::RamAllocatorMutex);
	__ASSERT_SYSTEM_LOCK;
	aPageInfo->SetUnused();
	--iNumberOfFreePages;
	__NK_ASSERT_DEBUG(iNumberOfFreePages>=0);
	// Release system lock before using the RAM allocator.
	NKern::UnlockSystem();
	iMmu->iRamPageAllocator->FreeRamPage(aPageInfo->PhysAddr(), EPageDiscard);
	NKern::LockSystem();
	}


SPageInfo* RamCacheBase::GetPageFromSystem(TUint aBlockedZoneId, TBool aBlockRest)
	{
	__ASSERT_MUTEX(MmuBase::RamAllocatorMutex);
	SPageInfo* pageInfo;
	TPhysAddr pagePhys;
	TInt r = iMmu->iRamPageAllocator->AllocRamPages(&pagePhys,1, EPageDiscard, aBlockedZoneId, aBlockRest);
	if(r==KErrNone)
		{
		NKern::LockSystem();
		pageInfo = SPageInfo::FromPhysAddr(pagePhys);
		pageInfo->Change(SPageInfo::EPagedFree,SPageInfo::EStatePagedDead);
		++iNumberOfFreePages;
		NKern::UnlockSystem();
		}
	else
		pageInfo = NULL;
	return pageInfo;
	}


//
// RamCache
//


void RamCache::Init2()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf(">RamCache::Init2"));
	RamCacheBase::Init2();
	__KTRACE_OPT(KBOOT,Kern::Printf("<RamCache::Init2"));
	}


TInt RamCache::Init3()
	{
	return KErrNone;
	}

void RamCache::RemovePage(SPageInfo& aPageInfo)
	{
	__NK_ASSERT_DEBUG(aPageInfo.Type() == SPageInfo::EPagedCache);
	__NK_ASSERT_DEBUG(aPageInfo.State() == SPageInfo::EStatePagedYoung);
	aPageInfo.iLink.Deque();
	aPageInfo.SetState(SPageInfo::EStatePagedDead);
	}

TBool RamCache::GetFreePages(TInt aNumPages)
	{
	__KTRACE_OPT(KPAGING,Kern::Printf("DP: >GetFreePages %d",aNumPages));
	NKern::LockSystem();

	while(aNumPages>0 && NumberOfFreePages()>=aNumPages)
		{
		// steal a page from cache list and return it to the free pool...
		SPageInfo* pageInfo = SPageInfo::FromLink(iPageList.First()->Deque());
		pageInfo->SetState(SPageInfo::EStatePagedDead);
		SetFree(pageInfo);
		ReturnToSystem(pageInfo);
		--aNumPages;
		}

	NKern::UnlockSystem();
	__KTRACE_OPT(KPAGING,Kern::Printf("DP: <GetFreePages %d",!aNumPages));
	return !aNumPages;
	}


void RamCache::DonateRamCachePage(SPageInfo* aPageInfo)
	{
	SPageInfo::TType type = aPageInfo->Type();
	if(type==SPageInfo::EChunk)
		{
		// Must not donate locked page. An example is DMA transferred memory.
		__NK_ASSERT_DEBUG(!aPageInfo->LockCount());

		aPageInfo->Change(SPageInfo::EPagedCache,SPageInfo::EStatePagedYoung);
		iPageList.Add(&aPageInfo->iLink);
		++iNumberOfFreePages;
		// Update ram allocator counts as this page has changed its type
		DMemModelChunk* chunk = (DMemModelChunk*)aPageInfo->Owner();
		iMmu->iRamPageAllocator->ChangePageType(aPageInfo, chunk->GetPageType(), EPageDiscard);

#ifdef BTRACE_PAGING
		BTraceContext8(BTrace::EPaging, BTrace::EPagingChunkDonatePage, chunk, aPageInfo->Offset());
#endif
		return;
		}
	// allow already donated pages...
	__NK_ASSERT_DEBUG(type==SPageInfo::EPagedCache);
	}


TBool RamCache::ReclaimRamCachePage(SPageInfo* aPageInfo)
	{
	SPageInfo::TType type = aPageInfo->Type();
//	Kern::Printf("DemandPaging::ReclaimRamCachePage %x %d free=%d",aPageInfo,type,iNumberOfFreePages);

	if(type==SPageInfo::EChunk)
		return ETrue; // page already reclaimed

	__NK_ASSERT_DEBUG(type==SPageInfo::EPagedCache);
	__NK_ASSERT_DEBUG(aPageInfo->State()==SPageInfo::EStatePagedYoung);
	// Update ram allocator counts as this page has changed its type
	DMemModelChunk* chunk = (DMemModelChunk*)aPageInfo->Owner();
	iMmu->iRamPageAllocator->ChangePageType(aPageInfo, EPageDiscard, chunk->GetPageType());
	aPageInfo->iLink.Deque();
	--iNumberOfFreePages;
	aPageInfo->Change(SPageInfo::EChunk,SPageInfo::EStateNormal);

#ifdef BTRACE_PAGING
	BTraceContext8(BTrace::EPaging, BTrace::EPagingChunkReclaimPage, chunk, aPageInfo->Offset());
#endif
	return ETrue;
	}


/**
Discard the specified page.
Should only be called on a page if a previous call to IsPageDiscardable()
returned ETrue and the system lock hasn't been released between the calls.

@param aPageInfo The page info of the page to be discarded
@param aBlockedZoneId Not used by this overload.
@param aBlockRest Not used by this overload. 
@return ETrue if page succesfully discarded

@pre System lock held.
@post System lock held.
*/
TBool RamCache::DoDiscardPage(SPageInfo& aPageInfo, TUint aBlockedZoneId, TBool aBlockRest)
	{
	__NK_ASSERT_DEBUG(iNumberOfFreePages > 0);
	RemovePage(aPageInfo);
	SetFree(&aPageInfo);
	ReturnToSystem(&aPageInfo);
	return ETrue;
	}


/**
First stage in discarding a list of pages.

Must ensure that the pages will still be discardable even if system lock is released.
To be used in conjunction with RamCacheBase::DoDiscardPages1().

@param aPageList A NULL terminated list of the pages to be discarded
@return KErrNone on success.

@pre System lock held
@post System lock held
*/
TInt RamCache::DoDiscardPages0(SPageInfo** aPageList)
	{
	__ASSERT_SYSTEM_LOCK;

	SPageInfo* pageInfo;
	while((pageInfo = *aPageList++) != 0)
		{
		RemovePage(*pageInfo);
		}
	return KErrNone;
	}


/**
Final stage in discarding a list of page
Finish discarding the pages previously removed by RamCacheBase::DoDiscardPages0().
This overload doesn't actually need to do anything.

@param aPageList A NULL terminated list of the pages to be discarded
@return KErrNone on success.

@pre System lock held
@post System lock held
*/
TInt RamCache::DoDiscardPages1(SPageInfo** aPageList)
	{
	__ASSERT_SYSTEM_LOCK;
	SPageInfo* pageInfo;
	while((pageInfo = *aPageList++) != 0)
		{
		SetFree(pageInfo);
		ReturnToSystem(pageInfo);
		}
	return KErrNone;
	}


/**
Check whether the specified page can be discarded by the RAM cache.

@param aPageInfo The page info of the page being queried.
@return ETrue when the page can be discarded, EFalse otherwise.
@pre System lock held.
@post System lock held.
*/
TBool RamCache::IsPageDiscardable(SPageInfo& aPageInfo)
	{
	SPageInfo::TType type = aPageInfo.Type();
	SPageInfo::TState state = aPageInfo.State();
	return (type == SPageInfo::EPagedCache && state == SPageInfo::EStatePagedYoung);
	}


/**
@return ETrue when the unmapped page should be freed, EFalse otherwise
*/
TBool RamCache::PageUnmapped(SPageInfo* aPageInfo)
	{
	SPageInfo::TType type = aPageInfo->Type();
//	Kern::Printf("DemandPaging::PageUnmapped %x %d",aPageInfo,type);
	if(type!=SPageInfo::EPagedCache)
		return ETrue;
	SPageInfo::TState state = aPageInfo->State();
	if(state==SPageInfo::EStatePagedYoung)
		{
		// This page will be freed by DChunk::DoDecommit as it was originally 
		// allocated so update page counts in ram allocator
		DMemModelChunk* chunk = (DMemModelChunk*)aPageInfo->Owner();
		iMmu->iRamPageAllocator->ChangePageType(aPageInfo, EPageDiscard, chunk->GetPageType());
		aPageInfo->iLink.Deque();
		--iNumberOfFreePages;
		}
	return ETrue;
	}


void RamCache::Panic(TFault aFault)
	{
	Kern::Fault("RamCache",aFault);
	}

/**
Flush all cache pages.

@pre RAM allocator mutex held
@post RAM allocator mutex held
*/
void RamCache::FlushAll()
	{
	__ASSERT_MUTEX(MmuBase::RamAllocatorMutex);
#ifdef _DEBUG
	// Should always succeed
	__NK_ASSERT_DEBUG(GetFreePages(iNumberOfFreePages));
#else
	GetFreePages(iNumberOfFreePages);
#endif
	}


//
// Demand Paging
//

#ifdef __DEMAND_PAGING__

DemandPaging* DemandPaging::ThePager = 0;
TBool DemandPaging::PseudoRandInitialised = EFalse;
volatile TUint32 DemandPaging::PseudoRandSeed = 0;


void M::DemandPagingInit()
	{
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf(">M::DemandPagingInit"));
	TInt r = RamCacheBase::TheRamCache->Init3();
	if (r != KErrNone)
		DemandPaging::Panic(DemandPaging::EInitialiseFailed);	

	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("<M::DemandPagingInit"));
	}


TInt M::DemandPagingFault(TAny* aExceptionInfo)
	{
	DemandPaging* pager = DemandPaging::ThePager;
	if(pager)
		return pager->Fault(aExceptionInfo);
	return KErrAbort;
	}

#ifdef _DEBUG
extern "C" void ASMCheckPagingSafe(TLinAddr aPC, TLinAddr aLR, TLinAddr aStartAddres, TUint aLength)
	{
	if(M::CheckPagingSafe(EFalse, aStartAddres, aLength))
		return;
	Kern::Printf("ASM_ASSERT_PAGING_SAFE FAILED: pc=%x lr=%x",aPC,aLR);
	__NK_ASSERT_ALWAYS(0);
	}

extern "C" void ASMCheckDataPagingSafe(TLinAddr aPC, TLinAddr aLR, TLinAddr aStartAddres, TUint aLength)
	{
	if(M::CheckPagingSafe(ETrue, aStartAddres, aLength))
		return;
	__KTRACE_OPT(KDATAPAGEWARN,Kern::Printf("Data paging: ASM_ASSERT_DATA_PAGING_SAFE FAILED: pc=%x lr=%x",aPC,aLR));
	}
#endif


TBool M::CheckPagingSafe(TBool aDataPaging, TLinAddr aStartAddr, TUint aLength)
	{
	DemandPaging* pager = DemandPaging::ThePager;
	if(!pager || K::Initialising)
		return ETrue;
	
	NThread* nt = NCurrentThread();
	if(!nt)
		return ETrue; // We've not booted properly yet!

	if (!pager->NeedsMutexOrderCheck(aStartAddr, aLength))
		return ETrue;

	TBool dataPagingEnabled = EFalse; // data paging not supported on moving or multiple models

	DThread* thread = _LOFF(nt,DThread,iNThread);
	NFastMutex* fm = NKern::HeldFastMutex();
	if(fm)
		{
		if(!thread->iPagingExcTrap || fm!=&TheScheduler.iLock)
			{
			if (!aDataPaging)
				{
				__KTRACE_OPT2(KPAGING,KPANIC,Kern::Printf("DP: CheckPagingSafe FAILED - FM Held"));
				return EFalse;
				}
			else
				{
				__KTRACE_OPT(KDATAPAGEWARN, Kern::Printf("Data paging: CheckPagingSafe FAILED - FM Held"));
				return !dataPagingEnabled;
				}
			}
		}

	DMutex* m = pager->CheckMutexOrder();
	if (m)
		{
		if (!aDataPaging)
			{
			__KTRACE_OPT2(KPAGING,KPANIC,Kern::Printf("DP: Mutex Order Fault %O",m));
			return EFalse;
			}
		else
			{
			__KTRACE_OPT(KDATAPAGEWARN, Kern::Printf("Data paging: Mutex Order Fault %O",m));
			return !dataPagingEnabled;
			}
		}
	
	return ETrue;
	}


TInt M::LockRegion(TLinAddr aStart,TInt aSize)
	{
	DemandPaging* pager = DemandPaging::ThePager;
	if(pager)
		return pager->LockRegion(aStart,aSize,NULL);
	return KErrNone;
	}


TInt M::UnlockRegion(TLinAddr aStart,TInt aSize)
	{
	DemandPaging* pager = DemandPaging::ThePager;
	if(pager)
		return pager->UnlockRegion(aStart,aSize,NULL);
	return KErrNone;
	}

#else // !__DEMAND_PAGING__

TInt M::LockRegion(TLinAddr /*aStart*/,TInt /*aSize*/)
	{
	return KErrNone;
	}


TInt M::UnlockRegion(TLinAddr /*aStart*/,TInt /*aSize*/)
	{
	return KErrNone;
	}

#endif // __DEMAND_PAGING__




//
// DemandPaging
//

#ifdef __DEMAND_PAGING__


const TUint16 KDefaultYoungOldRatio = 3;
const TUint KDefaultMinPages = 256;
const TUint KDefaultMaxPages = KMaxTUint >> KPageShift;

/*	Need at least 4 mapped pages to guarentee to be able to execute all ARM instructions.
	(Worst case is a THUMB2 STM instruction with both instruction and data stradling page
	boundaries.)
*/
const TUint KMinYoungPages = 4;
const TUint KMinOldPages = 1;

/*	A minimum young/old ratio of 1 means that we need at least twice KMinYoungPages pages...
*/
const TUint KAbsoluteMinPageCount = 2*KMinYoungPages;

__ASSERT_COMPILE(KMinOldPages<=KAbsoluteMinPageCount/2);

class DMissingPagingDevice : public DPagingDevice
	{
	TInt Read(TThreadMessage* /*aReq*/,TLinAddr /*aBuffer*/,TUint /*aOffset*/,TUint /*aSize*/,TInt /*aDrvNumber*/)
		{ DemandPaging::Panic(DemandPaging::EDeviceMissing); return 0; }
	};


TBool DemandPaging::RomPagingRequested()
	{
	return TheRomHeader().iPageableRomSize != 0;
	}


TBool DemandPaging::CodePagingRequested()
	{
	return (TheSuperPage().KernelConfigFlags() & EKernelConfigCodePagingPolicyDefaultPaged) != EKernelConfigCodePagingPolicyNoPaging;
	}


DemandPaging::DemandPaging()
	{
	}


void DemandPaging::Init2()
	{
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf(">DemandPaging::Init2"));

	RamCacheBase::Init2();

	// initialise live list...
	SDemandPagingConfig config = TheRomHeader().iDemandPagingConfig;

	iMinimumPageCount = KDefaultMinPages;
	if(config.iMinPages)
		iMinimumPageCount = config.iMinPages;
	if(iMinimumPageCount<KAbsoluteMinPageCount)
		iMinimumPageCount = KAbsoluteMinPageCount;
	iInitMinimumPageCount = iMinimumPageCount;

	iMaximumPageCount = KDefaultMaxPages;
	if(config.iMaxPages)
		iMaximumPageCount = config.iMaxPages;
	iInitMaximumPageCount = iMaximumPageCount;

	iYoungOldRatio = KDefaultYoungOldRatio;
	if(config.iYoungOldRatio)
		iYoungOldRatio = config.iYoungOldRatio;
	TInt ratioLimit = (iMinimumPageCount-KMinOldPages)/KMinOldPages;
	if(iYoungOldRatio>ratioLimit)
		iYoungOldRatio = ratioLimit;

	iMinimumPageLimit = (KMinYoungPages * (1 + iYoungOldRatio)) / iYoungOldRatio;
	if(iMinimumPageLimit<KAbsoluteMinPageCount)
		iMinimumPageLimit = KAbsoluteMinPageCount;

	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf(">DemandPaging::InitialiseLiveList min=%d max=%d ratio=%d",iMinimumPageCount,iMaximumPageCount,iYoungOldRatio));

	if(iMaximumPageCount<iMinimumPageCount)
		Panic(EInitialiseBadArgs);

	//
	// This routine doesn't acuire any mutexes because it should be called before the system
	// is fully up and running. I.e. called before another thread can preempt this.
	//

	// Calculate page counts
	iOldCount = iMinimumPageCount/(1+iYoungOldRatio);
	if(iOldCount<KMinOldPages)
		Panic(EInitialiseBadArgs);
	iYoungCount = iMinimumPageCount-iOldCount;
	if(iYoungCount<KMinYoungPages)
		Panic(EInitialiseBadArgs); // Need at least 4 pages mapped to execute an ARM LDM instruction in THUMB2 mode
	iNumberOfFreePages = 0;

	// Allocate RAM pages and put them all on the old list
	iYoungCount = 0;
	iOldCount = 0;
	for(TUint i=0; i<iMinimumPageCount; i++)
		{
		// Allocate a single page
		TPhysAddr pagePhys;
		TInt r = iMmu->iRamPageAllocator->AllocRamPages(&pagePhys,1, EPageDiscard);
		if(r!=0)
			Panic(EInitialiseFailed);
		AddAsFreePage(SPageInfo::FromPhysAddr(pagePhys));
		}

	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("<DemandPaging::Init2"));
	}


TInt VMHalFunction(TAny*, TInt aFunction, TAny* a1, TAny* a2);

TInt DemandPaging::Init3()
	{
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf(">DemandPaging::Init3"));
	TInt r;

	// construct iBufferChunk
	iDeviceBufferSize = 2*KPageSize;
	TChunkCreateInfo info;
	info.iType = TChunkCreateInfo::ESharedKernelMultiple;
	info.iMaxSize = iDeviceBufferSize*KMaxPagingDevices;
	info.iMapAttr = EMapAttrCachedMax;
	info.iOwnsMemory = ETrue;
	TUint32 mapAttr;
	r = Kern::ChunkCreate(info,iDeviceBuffersChunk,iDeviceBuffers,mapAttr);
	if(r!=KErrNone)
		return r;

	// Install 'null' paging devices which panic if used...
	DMissingPagingDevice* missingPagingDevice = new DMissingPagingDevice;
	for(TInt i=0; i<KMaxPagingDevices; i++)
		{
		iPagingDevices[i].iInstalled = EFalse;
		iPagingDevices[i].iDevice = missingPagingDevice;
		}

	// Initialise ROM info...
	const TRomHeader& romHeader = TheRomHeader();
	iRomLinearBase = (TLinAddr)&romHeader;
	iRomSize = iMmu->RoundToPageSize(romHeader.iUncompressedSize);
	if(romHeader.iRomPageIndex)
		iRomPageIndex = (SRomPageInfo*)((TInt)&romHeader+romHeader.iRomPageIndex);

	TLinAddr pagedStart = romHeader.iPageableRomSize ? (TLinAddr)&romHeader+romHeader.iPageableRomStart : 0;
	if(pagedStart)
		{
		__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("ROM=%x+%x PagedStart=%x",iRomLinearBase,iRomSize,pagedStart));
		__NK_ASSERT_ALWAYS(TUint(pagedStart-iRomLinearBase)<TUint(iRomSize));
		iRomPagedLinearBase = pagedStart;
		iRomPagedSize = iRomLinearBase+iRomSize-pagedStart;
		__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("DemandPaging::Init3, ROM Paged start(0x%x), sixe(0x%x)",iRomPagedLinearBase,iRomPagedSize));

#ifdef __SUPPORT_DEMAND_PAGING_EMULATION__
		// Get physical addresses of ROM pages
		iOriginalRomPageCount = iMmu->RoundToPageSize(iRomSize)>>KPageShift;
		iOriginalRomPages = new TPhysAddr[iOriginalRomPageCount];
		__NK_ASSERT_ALWAYS(iOriginalRomPages);
		TPhysAddr romPhysAddress; 
		iMmu->LinearToPhysical(iRomLinearBase,iRomSize,romPhysAddress,iOriginalRomPages);
#endif
		}

	r = Kern::AddHalEntry(EHalGroupVM, VMHalFunction, 0);
	__NK_ASSERT_ALWAYS(r==KErrNone);

#ifdef __DEMAND_PAGING_BENCHMARKS__
	for (TInt i = 0 ; i < EMaxPagingBm ; ++i)
		ResetBenchmarkData((TPagingBenchmark)i);
#endif

	// Initialisation now complete
	ThePager = this;
	return KErrNone;
	}


DemandPaging::~DemandPaging()
	{
#ifdef __SUPPORT_DEMAND_PAGING_EMULATION__
	delete[] iOriginalRomPages;
#endif
	for (TUint i = 0 ; i < iPagingRequestCount ; ++i)
		delete iPagingRequests[i];
	}


TInt DemandPaging::InstallPagingDevice(DPagingDevice* aDevice)
	{
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf(">DemandPaging::InstallPagingDevice name='%s' type=%d",aDevice->iName,aDevice->iType));

	if(aDevice->iReadUnitShift>KPageShift)
		Panic(EInvalidPagingDevice);

	TInt i;
	TInt r = KErrNone;
	TBool createRequestObjects = EFalse;
	
	if ((aDevice->iType & DPagingDevice::ERom) && RomPagingRequested())
		{
		r = DoInstallPagingDevice(aDevice, 0);
		if (r != KErrNone)
			goto done;
		K::MemModelAttributes|=EMemModelAttrRomPaging;
		createRequestObjects = ETrue;
		}
	
	if ((aDevice->iType & DPagingDevice::ECode) && CodePagingRequested())
		{
		for (i = 0 ; i < KMaxLocalDrives ; ++i)
			{
			if (aDevice->iDrivesSupported & (1<<i))
				{
				r = DoInstallPagingDevice(aDevice, i + 1);
				if (r != KErrNone)
					goto done;
				}
			}
		K::MemModelAttributes|=EMemModelAttrCodePaging;
		createRequestObjects = ETrue;
		}

	if (createRequestObjects)
		{
		for (i = 0 ; i < KPagingRequestsPerDevice ; ++i)
			{
			r = CreateRequestObject();
			if (r != KErrNone)
				goto done;
			}
		}
	
done:	
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("<DemandPaging::InstallPagingDevice returns %d",r));
	return r;
	}

TInt DemandPaging::DoInstallPagingDevice(DPagingDevice* aDevice, TInt aId)
	{
	NKern::LockSystem();
	SPagingDevice* device = &iPagingDevices[aId];
	if((device->iInstalled) && !(aDevice->iType & DPagingDevice::EMediaExtension))
		{
		__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("**** Attempt to install more than one ROM paging device !!!!!!!! ****"));
		//Panic(EDeviceAlreadyExists);
		NKern::UnlockSystem();
		return KErrNone;
		}	
	
	aDevice->iDeviceId = aId;
	device->iDevice = aDevice;
	device->iInstalled = ETrue;
	NKern::UnlockSystem();
	
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("DemandPaging::InstallPagingDevice id=%d, device=%08x",aId,device));
	
	return KErrNone;
	}

DemandPaging::DPagingRequest::~DPagingRequest()
	{
	if (iMutex)
		iMutex->Close(NULL);
	}

TInt DemandPaging::CreateRequestObject()
	{
	_LIT(KLitPagingRequest,"PagingRequest-"); 

	TInt index;
	TInt id = (TInt)__e32_atomic_add_ord32(&iNextPagingRequestCount, 1);
	TLinAddr offset = id * iDeviceBufferSize;
	TUint32 physAddr = 0;
	TInt r = Kern::ChunkCommitContiguous(iDeviceBuffersChunk,offset,iDeviceBufferSize, physAddr);
	if(r != KErrNone)
		return r;

	DPagingRequest* req = new DPagingRequest();
	if (!req)
		return KErrNoMemory;

	req->iBuffer = iDeviceBuffers + offset;
	AllocLoadAddress(*req, id);
		
	TBuf<16> mutexName(KLitPagingRequest);
	mutexName.AppendNum(id);
	r = K::MutexCreate(req->iMutex, mutexName, NULL, EFalse, KMutexOrdPageIn);
	if (r!=KErrNone)
		goto done;

	// Ensure there are enough young pages to cope with new request object
	r = ResizeLiveList(iMinimumPageCount, iMaximumPageCount);
	if (r!=KErrNone)
		goto done;

	NKern::LockSystem();
	index = iPagingRequestCount++;
	__NK_ASSERT_ALWAYS(index < KMaxPagingRequests);
	iPagingRequests[index] = req;
	iFreeRequestPool.AddHead(req);
	NKern::UnlockSystem();

done:
	if (r != KErrNone)
		delete req;
	
	return r;
	}

DemandPaging::DPagingRequest* DemandPaging::AcquireRequestObject()
	{
	__ASSERT_SYSTEM_LOCK;	
	__NK_ASSERT_DEBUG(iPagingRequestCount > 0);
	
	DPagingRequest* req = NULL;

	// System lock used to serialise access to our data strucures as we have to hold it anyway when
	// we wait on the mutex

	req = (DPagingRequest*)iFreeRequestPool.GetFirst();
	if (req != NULL)
		__NK_ASSERT_DEBUG(req->iUsageCount == 0);
	else
		{
		// Pick a random request object to wait on
		TUint index = (FastPseudoRand() * TUint64(iPagingRequestCount)) >> 32;
		__NK_ASSERT_DEBUG(index < iPagingRequestCount);
		req = iPagingRequests[index];
		__NK_ASSERT_DEBUG(req->iUsageCount > 0);
		}
	
#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
	++iWaitingCount;
	if (iWaitingCount > iMaxWaitingCount)
		iMaxWaitingCount = iWaitingCount;
#endif

	++req->iUsageCount;
	TInt r = req->iMutex->Wait();
	__NK_ASSERT_ALWAYS(r == KErrNone);

#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
	--iWaitingCount;
	++iPagingCount;
	if (iPagingCount > iMaxPagingCount)
		iMaxPagingCount = iPagingCount;
#endif

	return req;
	}

void DemandPaging::ReleaseRequestObject(DPagingRequest* aReq)
	{
	__ASSERT_SYSTEM_LOCK;

#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
	--iPagingCount;
#endif

	// If there are no threads waiting on the mutex then return it to the free pool
	__NK_ASSERT_DEBUG(aReq->iUsageCount > 0);
	if (--aReq->iUsageCount == 0)
		iFreeRequestPool.AddHead(aReq);

	aReq->iMutex->Signal();
	NKern::LockSystem();
	}

TInt DemandPaging::ReadRomPage(const DPagingRequest* aReq, TLinAddr aRomAddress)
	{
	START_PAGING_BENCHMARK;

	TInt pageSize = KPageSize;
	TInt dataOffset = aRomAddress-iRomLinearBase;
	TInt pageNumber = dataOffset>>KPageShift;
	TInt readUnitShift = RomPagingDevice().iDevice->iReadUnitShift;
	TInt r;
	if(!iRomPageIndex)
		{
		// ROM not broken into pages, so just read it in directly
		START_PAGING_BENCHMARK;
		r = RomPagingDevice().iDevice->Read(const_cast<TThreadMessage*>(&aReq->iMessage),aReq->iLoadAddr,dataOffset>>readUnitShift,pageSize>>readUnitShift,-1/*token for ROM paging*/);
		END_PAGING_BENCHMARK(DemandPaging::ThePager, EPagingBmReadMedia);
		}
	else
		{
		// Work out where data for page is located
		SRomPageInfo* romPageInfo = iRomPageIndex+pageNumber;
		dataOffset = romPageInfo->iDataStart;
		TInt dataSize = romPageInfo->iDataSize;
		if(!dataSize)
			{
			// empty page, fill it with 0xff...
			memset((void*)aReq->iLoadAddr,-1,pageSize);
			r = KErrNone;
			}
		else
			{
			__NK_ASSERT_ALWAYS(romPageInfo->iPagingAttributes&SRomPageInfo::EPageable);

			// Read data for page...
			TThreadMessage* msg= const_cast<TThreadMessage*>(&aReq->iMessage);
			TLinAddr buffer = aReq->iBuffer;
			TUint readStart = dataOffset>>readUnitShift;
			TUint readSize = ((dataOffset+dataSize-1)>>readUnitShift)-readStart+1;
			__NK_ASSERT_DEBUG((readSize<<readUnitShift)<=iDeviceBufferSize);
			START_PAGING_BENCHMARK;
			r = RomPagingDevice().iDevice->Read(msg,buffer,readStart,readSize,-1/*token for ROM paging*/);
			END_PAGING_BENCHMARK(DemandPaging::ThePager, EPagingBmReadMedia);
			if(r==KErrNone)
				{
				// Decompress data...
				TLinAddr data = buffer+dataOffset-(readStart<<readUnitShift);
				r = Decompress(romPageInfo->iCompressionType,aReq->iLoadAddr,data,dataSize);
				if(r>=0)
					{
					__NK_ASSERT_ALWAYS(r==pageSize);
					r = KErrNone;
					}
				}
			}
		}

	END_PAGING_BENCHMARK(this, EPagingBmReadRomPage);
	return r;
	}

TInt ReadFunc(TAny* aArg1, TAny* aArg2, TLinAddr aBuffer, TInt aBlockNumber, TInt aBlockCount)
	{
	START_PAGING_BENCHMARK;
	TInt drive = (TInt)aArg1;
	TThreadMessage* msg= (TThreadMessage*)aArg2;
	DemandPaging::SPagingDevice& device = DemandPaging::ThePager->CodePagingDevice(drive);
	TInt r = device.iDevice->Read(msg, aBuffer, aBlockNumber, aBlockCount, drive);
	END_PAGING_BENCHMARK(DemandPaging::ThePager, EPagingBmReadMedia);
	return r;
	}

TInt DemandPaging::ReadCodePage(const DPagingRequest* aReq, DMmuCodeSegMemory* aCodeSegMemory, TLinAddr aCodeAddress)
	{
	__KTRACE_OPT(KPAGING,Kern::Printf("ReadCodePage buffer = %08x, csm == %08x, addr == %08x", aReq->iLoadAddr, aCodeSegMemory, aCodeAddress));
	
	START_PAGING_BENCHMARK;

	// Get the paging device for this drive
	SPagingDevice& device = CodePagingDevice(aCodeSegMemory->iCodeLocalDrive);

	// Work out which bit of the file to read
	SRamCodeInfo& ri = aCodeSegMemory->iRamInfo;
	TInt codeOffset = aCodeAddress - ri.iCodeRunAddr;
	TInt pageNumber = codeOffset >> KPageShift;
	TBool compressed = aCodeSegMemory->iCompressionType != SRomPageInfo::ENoCompression;
	TInt dataOffset, dataSize;
	if (compressed)
		{
		dataOffset = aCodeSegMemory->iCodePageOffsets[pageNumber];
		dataSize = aCodeSegMemory->iCodePageOffsets[pageNumber + 1] - dataOffset;
		__KTRACE_OPT(KPAGING,Kern::Printf("  compressed, file offset == %x, size == %d", dataOffset, dataSize));
		}
	else
		{
		dataOffset = codeOffset + aCodeSegMemory->iCodeStartInFile;
		dataSize = Min(KPageSize, aCodeSegMemory->iBlockMap.DataLength() - dataOffset);
		__NK_ASSERT_DEBUG(dataSize >= 0);
		__KTRACE_OPT(KPAGING,Kern::Printf("  uncompressed, file offset == %x, size == %d", dataOffset, dataSize));
		}

	TInt bufferStart = aCodeSegMemory->iBlockMap.Read(aReq->iBuffer,
												dataOffset,
												dataSize,
												device.iDevice->iReadUnitShift,
												ReadFunc,
												(TAny*)aCodeSegMemory->iCodeLocalDrive,
												(TAny*)&aReq->iMessage);
	

	TInt r = KErrNone;
	if(bufferStart<0)
		{
		r = bufferStart; // return error
		__NK_ASSERT_DEBUG(0);
		}
	else
		{
		TLinAddr data = aReq->iBuffer + bufferStart;
		if (compressed)
			{
			TInt r = Decompress(aCodeSegMemory->iCompressionType, aReq->iLoadAddr, data, dataSize);
			if(r>=0)
				{
				dataSize = Min(KPageSize, ri.iCodeSize - codeOffset);
				if(r!=dataSize)
					{
					__NK_ASSERT_DEBUG(0);
					r = KErrCorrupt;
					}
				else
					r = KErrNone;
				}
			else
				{
				__NK_ASSERT_DEBUG(0);
				}
			}
		else
			{
			#ifdef BTRACE_PAGING_VERBOSE
			BTraceContext4(BTrace::EPaging,BTrace::EPagingDecompressStart,SRomPageInfo::ENoCompression);
			#endif
			memcpy((TAny*)aReq->iLoadAddr, (TAny*)data, dataSize);
			#ifdef BTRACE_PAGING_VERBOSE
			BTraceContext0(BTrace::EPaging,BTrace::EPagingDecompressEnd);
			#endif
			}
		}

	if(r==KErrNone)
		if (dataSize < KPageSize)
			memset((TAny*)(aReq->iLoadAddr + dataSize), KPageSize - dataSize, 0x03);

	END_PAGING_BENCHMARK(this, EPagingBmReadCodePage);
	
	return KErrNone;
	}


#include "decompress.h"

	
TInt DemandPaging::Decompress(TInt aCompressionType,TLinAddr aDst,TLinAddr aSrc,TUint aSrcSize)
	{
#ifdef BTRACE_PAGING_VERBOSE
	BTraceContext4(BTrace::EPaging,BTrace::EPagingDecompressStart,aCompressionType);
#endif
	TInt r;
	switch(aCompressionType)
		{
	case SRomPageInfo::ENoCompression:
		memcpy((void*)aDst,(void*)aSrc,aSrcSize);
		r = aSrcSize;
		break;

	case SRomPageInfo::EBytePair:
		{
		START_PAGING_BENCHMARK;
		TUint8* srcNext=0;
		r=BytePairDecompress((TUint8*)aDst,KPageSize,(TUint8*)aSrc,aSrcSize,srcNext);
		if (r == KErrNone)
			__NK_ASSERT_ALWAYS((TLinAddr)srcNext == aSrc + aSrcSize);
		END_PAGING_BENCHMARK(this, EPagingBmDecompress);
		}
		break;

	default:
		r = KErrNotSupported;
		break;
		}
#ifdef BTRACE_PAGING_VERBOSE
	BTraceContext0(BTrace::EPaging,BTrace::EPagingDecompressEnd);
#endif
	return r;
	}


void DemandPaging::BalanceAges()
	{
	if(iOldCount*iYoungOldRatio>=iYoungCount)
		return; // We have enough old pages

	// make one young page into an old page...

	__NK_ASSERT_DEBUG(!iYoungList.IsEmpty());
	__NK_ASSERT_DEBUG(iYoungCount);
	SDblQueLink* link = iYoungList.Last()->Deque();
	--iYoungCount;

	SPageInfo* pageInfo = SPageInfo::FromLink(link);
	pageInfo->SetState(SPageInfo::EStatePagedOld);

	iOldList.AddHead(link);
	++iOldCount;

	SetOld(pageInfo);

#ifdef BTRACE_PAGING_VERBOSE
	BTraceContext4(BTrace::EPaging,BTrace::EPagingAged,pageInfo->PhysAddr());
#endif
	}


void DemandPaging::AddAsYoungest(SPageInfo* aPageInfo)
	{
#ifdef _DEBUG
	SPageInfo::TType type = aPageInfo->Type();
	__NK_ASSERT_DEBUG(type==SPageInfo::EPagedROM || type==SPageInfo::EPagedCode || type==SPageInfo::EPagedData || type==SPageInfo::EPagedCache);
#endif
	aPageInfo->SetState(SPageInfo::EStatePagedYoung);
	iYoungList.AddHead(&aPageInfo->iLink);
	++iYoungCount;
	}


void DemandPaging::AddAsFreePage(SPageInfo* aPageInfo)
	{
#ifdef BTRACE_PAGING
	TPhysAddr phys = aPageInfo->PhysAddr();
	BTraceContext4(BTrace::EPaging,BTrace::EPagingPageInFree,phys);
#endif
	aPageInfo->Change(SPageInfo::EPagedFree,SPageInfo::EStatePagedOld);
	iOldList.Add(&aPageInfo->iLink);
	++iOldCount;
	}


void DemandPaging::RemovePage(SPageInfo* aPageInfo)
	{
	switch(aPageInfo->State())
		{
	case SPageInfo::EStatePagedYoung:
		__NK_ASSERT_DEBUG(iYoungCount);
		aPageInfo->iLink.Deque();
		--iYoungCount;
		break;

	case SPageInfo::EStatePagedOld:
		__NK_ASSERT_DEBUG(iOldCount);
		aPageInfo->iLink.Deque();
		--iOldCount;
		break;

	case SPageInfo::EStatePagedLocked:
		break;

	default:
		__NK_ASSERT_DEBUG(0);
		}
	aPageInfo->SetState(SPageInfo::EStatePagedDead);
	}


SPageInfo* DemandPaging::GetOldestPage()
	{
	// remove oldest from list...
	SDblQueLink* link;
	if(iOldCount)
		{
		__NK_ASSERT_DEBUG(!iOldList.IsEmpty());
		link = iOldList.Last()->Deque();
		--iOldCount;
		}
	else
		{
		__NK_ASSERT_DEBUG(iYoungCount);
		__NK_ASSERT_DEBUG(!iYoungList.IsEmpty());
		link = iYoungList.Last()->Deque();
		--iYoungCount;
		}
	SPageInfo* pageInfo = SPageInfo::FromLink(link);
	pageInfo->SetState(SPageInfo::EStatePagedDead);

	// put page in a free state...
	SetFree(pageInfo);
	pageInfo->Change(SPageInfo::EPagedFree,SPageInfo::EStatePagedDead);

	// keep live list balanced...
	BalanceAges();

	return pageInfo;
	}


TBool DemandPaging::GetFreePages(TInt aNumPages)
	{
	__KTRACE_OPT(KPAGING,Kern::Printf("DP: >GetFreePages %d",aNumPages));
	NKern::LockSystem();

	while(aNumPages>0 && NumberOfFreePages()>=aNumPages)
		{
		// steal a page from live page list and return it to the free pool...
		ReturnToSystem(GetOldestPage());
		--aNumPages;
		}

	NKern::UnlockSystem();
	__KTRACE_OPT(KPAGING,Kern::Printf("DP: <GetFreePages %d",!aNumPages));
	return !aNumPages;
	}


void DemandPaging::DonateRamCachePage(SPageInfo* aPageInfo)
	{
	__NK_ASSERT_DEBUG(iMinimumPageCount + iNumberOfFreePages <= iMaximumPageCount);
	SPageInfo::TType type = aPageInfo->Type();
	if(type==SPageInfo::EChunk)
		{
		// Must not donate locked page. An example is DMA transferred memory.
		__NK_ASSERT_DEBUG(!aPageInfo->LockCount());
		
		aPageInfo->Change(SPageInfo::EPagedCache,SPageInfo::EStatePagedYoung);

		// Update ram allocator counts as this page has changed its type
		DMemModelChunk* chunk = (DMemModelChunk*)aPageInfo->Owner();
		iMmu->iRamPageAllocator->ChangePageType(aPageInfo, chunk->GetPageType(), EPageDiscard);

		AddAsYoungest(aPageInfo);
		++iNumberOfFreePages;
		if (iMinimumPageCount + iNumberOfFreePages > iMaximumPageCount)
			ReturnToSystem(GetOldestPage());
		BalanceAges();
		return;
		}
	// allow already donated pages...
	__NK_ASSERT_DEBUG(type==SPageInfo::EPagedCache);
	}


TBool DemandPaging::ReclaimRamCachePage(SPageInfo* aPageInfo)
	{
	SPageInfo::TType type = aPageInfo->Type();
	if(type==SPageInfo::EChunk)
		return ETrue; // page already reclaimed

	__NK_ASSERT_DEBUG(type==SPageInfo::EPagedCache);

	if(!iNumberOfFreePages)
		return EFalse;
	--iNumberOfFreePages;

	RemovePage(aPageInfo);
	aPageInfo->Change(SPageInfo::EChunk,SPageInfo::EStateNormal);

	// Update ram allocator counts as this page has changed its type
	DMemModelChunk* chunk = (DMemModelChunk*)aPageInfo->Owner();
	iMmu->iRamPageAllocator->ChangePageType(aPageInfo, EPageDiscard, chunk->GetPageType());
	return ETrue;
	}


SPageInfo* DemandPaging::AllocateNewPage()
	{
	__ASSERT_SYSTEM_LOCK
	SPageInfo* pageInfo;

	NKern::UnlockSystem();
	MmuBase::Wait();
	NKern::LockSystem();

	// Try getting a free page from our active page list
	if(iOldCount)
		{
		pageInfo = SPageInfo::FromLink(iOldList.Last());
		if(pageInfo->Type()==SPageInfo::EPagedFree)
			{
			pageInfo = GetOldestPage();
			goto done;
			}
		}

	// Try getting a free page from the system pool
	if(iMinimumPageCount+iNumberOfFreePages<iMaximumPageCount)
		{
		NKern::UnlockSystem();
		pageInfo = GetPageFromSystem();
		NKern::LockSystem();
		if(pageInfo)
			goto done;
		}

	// As a last resort, steal one from our list of active pages
	pageInfo = GetOldestPage();

done:
	NKern::UnlockSystem();
	MmuBase::Signal();
	NKern::LockSystem();
	return pageInfo;
	}


void DemandPaging::Rejuvenate(SPageInfo* aPageInfo)
	{
	SPageInfo::TState state = aPageInfo->State();
	if(state==SPageInfo::EStatePagedOld)
		{
		// move page from old list to head of young list...
		__NK_ASSERT_DEBUG(iOldCount);
		aPageInfo->iLink.Deque();
		--iOldCount;
		AddAsYoungest(aPageInfo);
		BalanceAges();
		}
	else if(state==SPageInfo::EStatePagedYoung)
		{
		// page was already young, move it to the start of the list (make it the youngest)
		aPageInfo->iLink.Deque();
		iYoungList.AddHead(&aPageInfo->iLink);
		}
	else
		{
		// leave locked pages alone
		__NK_ASSERT_DEBUG(state==SPageInfo::EStatePagedLocked);
		}
	}


TInt DemandPaging::CheckRealtimeThreadFault(DThread* aThread, TAny* aContext)
	{
	TInt r = KErrNone;
	DThread* client = aThread->iIpcClient;
	
	// If iIpcClient is set then we are accessing the address space of a remote thread.  If we are
	// in an IPC trap, this will contain information the local and remte addresses being accessed.
	// If this is not set then we assume than any fault must be the fault of a bad remote address.
	TIpcExcTrap* ipcTrap = (TIpcExcTrap*)aThread->iExcTrap;
	if (ipcTrap && !ipcTrap->IsTIpcExcTrap())
		ipcTrap = 0;
	if (client && (!ipcTrap || ipcTrap->ExcLocation(aThread, aContext) == TIpcExcTrap::EExcRemote))
		{
		// Kill client thread...
		NKern::UnlockSystem();
		if(K::IllegalFunctionForRealtimeThread(client,"Access to Paged Memory (by other thread)"))
			{
			// Treat memory access as bad...
			r = KErrAbort;
			}
		// else thread is in 'warning only' state so allow paging
		}
	else
		{
		// Kill current thread...
		NKern::UnlockSystem();
		if(K::IllegalFunctionForRealtimeThread(NULL,"Access to Paged Memory"))
			{
			// If current thread is in critical section, then the above kill will be deferred
			// and we will continue executing. We will handle this by returning an error
			// which means that the thread will take an exception (which hopfully is XTRAPed!)
			r = KErrAbort;
			}
		// else thread is in 'warning only' state so allow paging
		}
	
	NKern::LockSystem();
	return r;
	}


TInt DemandPaging::ResizeLiveList(TUint aMinimumPageCount,TUint aMaximumPageCount)
	{
	if(!aMaximumPageCount)
		{
		aMinimumPageCount = iInitMinimumPageCount;
		aMaximumPageCount = iInitMaximumPageCount;
		}

	// Min must not be greater than max...
	if(aMinimumPageCount>aMaximumPageCount)
		return KErrArgument;

	NKern::ThreadEnterCS();
	MmuBase::Wait();

	NKern::LockSystem();

	// Make sure aMinimumPageCount is not less than absolute minimum we can cope with...
	iMinimumPageLimit = ((KMinYoungPages + iNextPagingRequestCount) * (1 + iYoungOldRatio)) / iYoungOldRatio;
	if(iMinimumPageLimit<KAbsoluteMinPageCount)
		iMinimumPageLimit = KAbsoluteMinPageCount;
	if(aMinimumPageCount<iMinimumPageLimit+iReservePageCount)
		aMinimumPageCount = iMinimumPageLimit+iReservePageCount;
	if(aMaximumPageCount<aMinimumPageCount)
		aMaximumPageCount=aMinimumPageCount;

	// Increase iMaximumPageCount?
	TInt extra = aMaximumPageCount-iMaximumPageCount;
	if(extra>0)
		iMaximumPageCount += extra;

	// Reduce iMinimumPageCount?
	TInt spare = iMinimumPageCount-aMinimumPageCount;
	if(spare>0)
		{
		iMinimumPageCount -= spare;
		iNumberOfFreePages += spare;
		}

	// Increase iMinimumPageCount?
	TInt r=KErrNone;
	while(aMinimumPageCount>iMinimumPageCount)
		{
		if(iNumberOfFreePages==0)	// Need more pages?
			{
			// get a page from the system
			NKern::UnlockSystem();
			SPageInfo* pageInfo = GetPageFromSystem();
			NKern::LockSystem();
			if(!pageInfo)
				{
				r=KErrNoMemory;
				break;
				}
			AddAsFreePage(pageInfo);
			}
		++iMinimumPageCount;
		--iNumberOfFreePages;
		NKern::FlashSystem();
		}

	// Reduce iMaximumPageCount?
	while(iMaximumPageCount>aMaximumPageCount)
		{
		if (iMinimumPageCount+iNumberOfFreePages==iMaximumPageCount)	// Need to free pages?
			{
			ReturnToSystem(GetOldestPage());
			}
		--iMaximumPageCount;
		NKern::FlashSystem();
		}

#ifdef BTRACE_KERNEL_MEMORY
	BTrace4(BTrace::EKernelMemory,BTrace::EKernelMemoryDemandPagingCache,ThePager->iMinimumPageCount << KPageShift);
#endif

	__NK_ASSERT_DEBUG(iMinimumPageCount + iNumberOfFreePages <= iMaximumPageCount);

	NKern::UnlockSystem();

	MmuBase::Signal();
	NKern::ThreadLeaveCS();

	return r;
	}


TInt VMHalFunction(TAny*, TInt aFunction, TAny* a1, TAny* a2)
	{
	DemandPaging* pager = DemandPaging::ThePager;
	switch(aFunction)
		{
	case EVMHalFlushCache:
		if(!TheCurrentThread->HasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by VMHalFunction(EVMHalFlushCache)")))
			K::UnlockedPlatformSecurityPanic();
		pager->FlushAll();
		return KErrNone;

	case EVMHalSetCacheSize:
		{
		if(!TheCurrentThread->HasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by VMHalFunction(EVMHalSetCacheSize)")))
			K::UnlockedPlatformSecurityPanic();
		TUint min = (TUint)a1>>KPageShift;
		if((TUint)a1&KPageMask)
			++min;
		TUint max = (TUint)a2>>KPageShift;
		if((TUint)a2&KPageMask)
			++max;
		return pager->ResizeLiveList(min,max);
		}

	case EVMHalGetCacheSize:
		{
		SVMCacheInfo info;
		NKern::LockSystem(); // lock system to ensure consistent set of values are read...
		info.iMinSize = pager->iMinimumPageCount<<KPageShift;
		info.iMaxSize = pager->iMaximumPageCount<<KPageShift;
		info.iCurrentSize = (pager->iMinimumPageCount+pager->iNumberOfFreePages)<<KPageShift;
		info.iMaxFreeSize = pager->iNumberOfFreePages<<KPageShift;
		NKern::UnlockSystem();
		kumemput32(a1,&info,sizeof(info));
		}
		return KErrNone;

	case EVMHalGetEventInfo:
		{
		SVMEventInfo info;
		NKern::LockSystem(); // lock system to ensure consistent set of values are read...
		info = pager->iEventInfo;
		NKern::UnlockSystem();
		Kern::InfoCopy(*(TDes8*)a1,(TUint8*)&info,sizeof(info));
		}
		return KErrNone;

	case EVMHalResetEventInfo:
		NKern::LockSystem();
		memclr(&pager->iEventInfo, sizeof(pager->iEventInfo));
		NKern::UnlockSystem();
		return KErrNone;

#ifdef __SUPPORT_DEMAND_PAGING_EMULATION__
	case EVMHalGetOriginalRomPages:
		*(TPhysAddr**)a1 = pager->iOriginalRomPages;
		*(TInt*)a2 = pager->iOriginalRomPageCount;
		return KErrNone;
#endif

	case EVMPageState:
		return pager->PageState((TLinAddr)a1);

#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
	case EVMHalGetConcurrencyInfo:
		{
		NKern::LockSystem();
		SPagingConcurrencyInfo info = { pager->iMaxWaitingCount, pager->iMaxPagingCount };
		NKern::UnlockSystem();
		kumemput32(a1,&info,sizeof(info));
		}
		return KErrNone;
		
	case EVMHalResetConcurrencyInfo:
		NKern::LockSystem();
		pager->iMaxWaitingCount = 0;
		pager->iMaxPagingCount = 0;
		NKern::UnlockSystem();
		return KErrNone;
#endif

#ifdef __DEMAND_PAGING_BENCHMARKS__
	case EVMHalGetPagingBenchmark:
		{
		TUint index = (TInt) a1;
		if (index >= EMaxPagingBm)
			return KErrNotFound;
		NKern::LockSystem();
		SPagingBenchmarkInfo info = pager->iBenchmarkInfo[index];
		NKern::UnlockSystem();
		kumemput32(a2,&info,sizeof(info));
		}		
		return KErrNone;
		
	case EVMHalResetPagingBenchmark:
		{
		TUint index = (TInt) a1;
		if (index >= EMaxPagingBm)
			return KErrNotFound;
		NKern::LockSystem();
		pager->ResetBenchmarkData((TPagingBenchmark)index);
		NKern::UnlockSystem();
		}
		return KErrNone;
#endif

	default:
		return KErrNotSupported;
		}
	}

void DemandPaging::Panic(TFault aFault)
	{
	Kern::Fault("DEMAND-PAGING",aFault);
	}


DMutex* DemandPaging::CheckMutexOrder()
	{
#ifdef _DEBUG
	SDblQue& ml = TheCurrentThread->iMutexList;
	if(ml.IsEmpty())
		return NULL;
	DMutex* mm = _LOFF(ml.First(), DMutex, iOrderLink);
	if (KMutexOrdPageIn >= mm->iOrder)
		return mm;
#endif
	return NULL;
	}


TBool DemandPaging::ReservePage()
	{
	__ASSERT_SYSTEM_LOCK;
	__ASSERT_CRITICAL;

	NKern::UnlockSystem();
	MmuBase::Wait();
	NKern::LockSystem();

	__NK_ASSERT_DEBUG(iMinimumPageCount >= iMinimumPageLimit + iReservePageCount);
	while (iMinimumPageCount == iMinimumPageLimit + iReservePageCount &&
		   iNumberOfFreePages == 0)
		{
		NKern::UnlockSystem();
		SPageInfo* pageInfo = GetPageFromSystem();
		if(!pageInfo)
			{
			MmuBase::Signal();
			NKern::LockSystem();
			return EFalse;
			}
		NKern::LockSystem();
		AddAsFreePage(pageInfo);
		}
	if (iMinimumPageCount == iMinimumPageLimit + iReservePageCount)
		{	
		++iMinimumPageCount;
		--iNumberOfFreePages;
		if (iMinimumPageCount > iMaximumPageCount)
			iMaximumPageCount = iMinimumPageCount;
		}
	++iReservePageCount;
	__NK_ASSERT_DEBUG(iMinimumPageCount >= iMinimumPageLimit + iReservePageCount);
	__NK_ASSERT_DEBUG(iMinimumPageCount + iNumberOfFreePages <= iMaximumPageCount);

	NKern::UnlockSystem();
	MmuBase::Signal();
	NKern::LockSystem();
	return ETrue;
	}


TInt DemandPaging::LockRegion(TLinAddr aStart,TInt aSize,DProcess* aProcess)
	{
	__KTRACE_OPT(KPAGING,Kern::Printf("DP: LockRegion(%08x,%x)",aStart,aSize));
	NKern::ThreadEnterCS();

	// calculate the number of pages required to lock aSize bytes
	TUint32 mask=KPageMask;
	TUint32 offset=aStart&mask;
	TInt numPages = (aSize+offset+mask)>>KPageShift;

	// Lock pages...
	TInt r=KErrNone;
	TLinAddr page = aStart;

	NKern::LockSystem();
	while(--numPages>=0)
		{
		if (!ReservePage())
			break;
		TPhysAddr phys;
		r = LockPage(page,aProcess,phys);
		NKern::FlashSystem();
		if(r!=KErrNone)
			break;
		page += KPageSize;
		}

	NKern::UnlockSystem();

	// If error, unlock whatever we managed to lock...
	if(r!=KErrNone)
		{
		while((page-=KPageSize)>=aStart)
			{
			NKern::LockSystem();
			UnlockPage(aStart,aProcess,KPhysAddrInvalid);
			--iReservePageCount;
			NKern::UnlockSystem();
			}
		}

	NKern::ThreadLeaveCS();
	__KTRACE_OPT(KPAGING,Kern::Printf("DP: LockRegion returns %d",r));
	return r;
	}


TInt DemandPaging::UnlockRegion(TLinAddr aStart,TInt aSize,DProcess* aProcess)
	{
	__KTRACE_OPT(KPAGING,Kern::Printf("DP: UnlockRegion(%08x,%x)",aStart,aSize));
	TUint32 mask=KPageMask;
	TUint32 offset=aStart&mask;
	TInt numPages = (aSize+offset+mask)>>KPageShift;
	NKern::LockSystem();
	__NK_ASSERT_DEBUG(iReservePageCount >= (TUint)numPages);
	while(--numPages>=0)
		{
		UnlockPage(aStart,aProcess,KPhysAddrInvalid);
		--iReservePageCount;		
		NKern::FlashSystem();
		aStart += KPageSize;
		}
	NKern::UnlockSystem();
	return KErrNone;
	}


void DemandPaging::FlushAll()
	{
	NKern::ThreadEnterCS();
	MmuBase::Wait();
	// look at all RAM pages in the system, and unmap all those used for paging
	const TUint32* piMap = (TUint32*)KPageInfoMap;
	const TUint32* piMapEnd = piMap+(KNumPageInfoPages>>5);
	SPageInfo* pi = (SPageInfo*)KPageInfoLinearBase;
	NKern::LockSystem();
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
				SPageInfo::TState state = pi->State();
				if(state==SPageInfo::EStatePagedYoung || state==SPageInfo::EStatePagedOld)
					{
					RemovePage(pi);
					SetFree(pi);
					AddAsFreePage(pi);
					NKern::FlashSystem();
					}
				++pi;
				const TUint KFlashCount = 64; // flash every 64 page infos (must be a power-of-2)
				__ASSERT_COMPILE((TUint)KPageInfosPerPage >= KFlashCount);
				if(((TUint)pi&((KFlashCount-1)<<KPageInfoShift))==0)
					NKern::FlashSystem();
				}
			while(pi<piEnd);
			}
		pi = piNext;
		}
	while(piMap<piMapEnd);
	NKern::UnlockSystem();

	// reduce live page list to a minimum
	while(GetFreePages(1)) {}; 

	MmuBase::Signal();
	NKern::ThreadLeaveCS();
	}


TInt DemandPaging::LockPage(TLinAddr aPage, DProcess *aProcess, TPhysAddr& aPhysAddr)
	{
	__KTRACE_OPT(KPAGING,Kern::Printf("DP: LockPage() %08x",aPage));
	__ASSERT_SYSTEM_LOCK

	aPhysAddr = KPhysAddrInvalid;

	TInt r = EnsurePagePresent(aPage,aProcess);
	if (r != KErrNone)
		return KErrArgument; // page doesn't exist

	// get info about page to be locked...
	TPhysAddr phys = LinearToPhysical(aPage,aProcess);
retry:
	__NK_ASSERT_DEBUG(phys!=KPhysAddrInvalid);

	SPageInfo* pageInfo = SPageInfo::SafeFromPhysAddr(phys);
	if(!pageInfo)
		return KErrNotFound;

	// lock it...
	SPageInfo::TType type = pageInfo->Type();
	if(type==SPageInfo::EShadow)
		{
		// get the page which is being shadowed and lock that
		phys = (TPhysAddr)pageInfo->Owner();
		goto retry;
		}

	switch(pageInfo->State())
		{
	case SPageInfo::EStatePagedLocked:
		// already locked, so just increment lock count...
		++pageInfo->PagedLock();
		break;

	case SPageInfo::EStatePagedYoung:
		{
		if(type!=SPageInfo::EPagedROM && type !=SPageInfo::EPagedCode)
			{
			// not implemented yet
			__NK_ASSERT_ALWAYS(0);
			}

		// remove page to be locked from live list...
		RemovePage(pageInfo);

		// change to locked state...
		pageInfo->SetState(SPageInfo::EStatePagedLocked);
		pageInfo->PagedLock() = 1; // Start with lock count of one

		// open reference on memory...
		if(type==SPageInfo::EPagedCode)
			{
			DMemModelCodeSegMemory* codeSegMemory = (DMemModelCodeSegMemory*)pageInfo->Owner();
			if(codeSegMemory->Open()!=KErrNone)
				{
				__NK_ASSERT_DEBUG(0);
				}
			}
		}
		
		break;

	case SPageInfo::EStatePagedOld:
		// can't happen because we forced the page to be accessible earlier
		__NK_ASSERT_ALWAYS(0);
		return KErrCorrupt;

	default:
		return KErrNotFound;
		}

	aPhysAddr = phys;

#ifdef BTRACE_PAGING
	BTraceContext8(BTrace::EPaging,BTrace::EPagingPageLock,phys,pageInfo->PagedLock());
#endif
	return KErrNone;
	}


TInt DemandPaging::UnlockPage(TLinAddr aPage, DProcess* aProcess, TPhysAddr aPhysAddr)
	{
	__KTRACE_OPT(KPAGING,Kern::Printf("DP: UnlockPage() %08x",aPage));
	__ASSERT_SYSTEM_LOCK;
	__ASSERT_CRITICAL;

	// Get info about page to be unlocked
	TPhysAddr phys = LinearToPhysical(aPage,aProcess);
	if(phys==KPhysAddrInvalid)
		{
		phys = aPhysAddr;
		if(phys==KPhysAddrInvalid)
			return KErrNotFound;
		}
retry:
	SPageInfo* pageInfo = SPageInfo::SafeFromPhysAddr(phys);
	if(!pageInfo)
		return KErrNotFound;

	SPageInfo::TType type = pageInfo->Type();
	if(type==SPageInfo::EShadow)
		{
		// Get the page which is being shadowed and unlock that
		phys = (TPhysAddr)pageInfo->Owner();
		goto retry;
		}

	__NK_ASSERT_DEBUG(phys==aPhysAddr || aPhysAddr==KPhysAddrInvalid);

	// Unlock it...
	switch(pageInfo->State())
		{
	case SPageInfo::EStatePagedLocked:
#ifdef BTRACE_PAGING
		BTraceContext8(BTrace::EPaging,BTrace::EPagingPageUnlock,phys,pageInfo->PagedLock());
#endif
		if(!(--pageInfo->PagedLock()))
			{
			// get pointer to memory...
			DMemModelCodeSegMemory* codeSegMemory = 0;
			if(type==SPageInfo::EPagedCode)
				codeSegMemory = (DMemModelCodeSegMemory*)pageInfo->Owner();

			// put page back on live list...
			AddAsYoungest(pageInfo);
			BalanceAges();

			// close reference on memory...
			if(codeSegMemory)
				{
				NKern::UnlockSystem();
				codeSegMemory->Close();
				NKern::LockSystem();
				}
			}
		break;

	default:
		return KErrNotFound;
		}

	return KErrNone;
	}



TInt DemandPaging::ReserveAlloc(TInt aSize, DDemandPagingLock& aLock)
	{
	__NK_ASSERT_DEBUG(aLock.iPages == NULL);
	
	// calculate the number of pages required to lock aSize bytes
	TInt numPages = ((aSize-1+KPageMask)>>KPageShift)+1;

	__KTRACE_OPT(KPAGING,Kern::Printf("DP: ReserveAlloc() pages %d",numPages));
	
	NKern::ThreadEnterCS();

	aLock.iPages = (TPhysAddr*)Kern::Alloc(numPages*sizeof(TPhysAddr));
	if(!aLock.iPages)
		{
		NKern::ThreadLeaveCS();
		return KErrNoMemory;
		}
	
	MmuBase::Wait();
	NKern::LockSystem();

	// reserve pages, adding more if necessary
	while (aLock.iReservedPageCount < numPages)
		{
		if (!ReservePage())
			break;
		++aLock.iReservedPageCount;
		}

	NKern::UnlockSystem();
	MmuBase::Signal();

	TBool enoughPages = aLock.iReservedPageCount == numPages;
	if(!enoughPages)
		ReserveFree(aLock);

	NKern::ThreadLeaveCS();
	return enoughPages ? KErrNone : KErrNoMemory;
	}



void DemandPaging::ReserveFree(DDemandPagingLock& aLock)
	{
	NKern::ThreadEnterCS();

	// make sure pages aren't still locked
	ReserveUnlock(aLock);

	NKern::LockSystem();
	__NK_ASSERT_DEBUG(iReservePageCount >= (TUint)aLock.iReservedPageCount);
	iReservePageCount -= aLock.iReservedPageCount;
	aLock.iReservedPageCount = 0;
	NKern::UnlockSystem();

	// free page array...
	Kern::Free(aLock.iPages);
	aLock.iPages = 0;

	NKern::ThreadLeaveCS();
	}



TBool DemandPaging::ReserveLock(DThread* aThread, TLinAddr aStart,TInt aSize, DDemandPagingLock& aLock)
	{
	if(aLock.iLockedPageCount)
		Panic(ELockTwice);

	// calculate the number of pages that need to be locked...
	TUint32 mask=KPageMask;
	TUint32 offset=aStart&mask;
	TInt numPages = (aSize+offset+mask)>>KPageShift;
	if(numPages>aLock.iReservedPageCount)
		Panic(ELockTooBig);

	NKern::LockSystem();

	// lock the pages
	TBool locked = EFalse; // becomes true if any pages were locked
	DProcess* process = aThread->iOwningProcess;
	TLinAddr page=aStart;
	TInt count=numPages;
	TPhysAddr* physPages = aLock.iPages;
	while(--count>=0)
		{
		if(LockPage(page,process,*physPages)==KErrNone)
			locked = ETrue;
		NKern::FlashSystem();
		page += KPageSize;
		++physPages;
		}

	// if any pages were locked, save the lock info...
	if(locked)
		{
		if(aLock.iLockedPageCount)
			Panic(ELockTwice);
		aLock.iLockedStart = aStart;
		aLock.iLockedPageCount = numPages;
		aLock.iProcess = process;
		aLock.iProcess->Open();
		}

	NKern::UnlockSystem();
	return locked;
	}



void DemandPaging::ReserveUnlock(DDemandPagingLock& aLock)
	{
	NKern::ThreadEnterCS();

	DProcess* process = NULL;
	NKern::LockSystem();
	TInt numPages = aLock.iLockedPageCount;
	TLinAddr page = aLock.iLockedStart;
	TPhysAddr* physPages = aLock.iPages;
	while(--numPages>=0)
		{
		UnlockPage(page, aLock.iProcess,*physPages);
		NKern::FlashSystem();
		page += KPageSize;
		++physPages;
		}
	process = aLock.iProcess;
	aLock.iProcess = NULL;
	aLock.iLockedPageCount = 0;
	NKern::UnlockSystem();
	if (process)
		process->Close(NULL);

	NKern::ThreadLeaveCS();
	}

/**
Check whether the specified page can be discarded by the RAM cache.

@param aPageInfo The page info of the page being queried.
@return ETrue when the page can be discarded, EFalse otherwise.
@pre System lock held.
@post System lock held.
*/
TBool DemandPaging::IsPageDiscardable(SPageInfo& aPageInfo)
	{
	 // on live list?
	SPageInfo::TState state = aPageInfo.State();
	return (state == SPageInfo::EStatePagedYoung || state == SPageInfo::EStatePagedOld);
	}


/**
Discard the specified page.
Should only be called on a page if a previous call to IsPageDiscardable()
returned ETrue and the system lock hasn't been released between the calls.

@param aPageInfo The page info of the page to be discarded
@param aBlockZoneId The ID of the RAM zone that shouldn't be allocated into.
@param aBlockRest Set to ETrue to stop allocation as soon as aBlockedZoneId is reached 
in preference ordering.  EFalse otherwise.
@return ETrue if the page could be discarded, EFalse otherwise.

@pre System lock held.
@post System lock held.
*/
TBool DemandPaging::DoDiscardPage(SPageInfo& aPageInfo, TUint aBlockedZoneId, TBool aBlockRest)
	{
	__ASSERT_SYSTEM_LOCK;
	// Ensure that we don't reduce the cache beyond its minimum.
	if (iNumberOfFreePages == 0)
		{
		NKern::UnlockSystem();
		SPageInfo* newPage = GetPageFromSystem(aBlockedZoneId, aBlockRest);
		NKern::LockSystem();
		if (newPage == NULL)
			{// couldn't allocate a new page
			return EFalse;
			}
		if (IsPageDiscardable(aPageInfo))
			{// page can still be discarded so use new page 
			// and discard old one
			AddAsFreePage(newPage);
			RemovePage(&aPageInfo);
			SetFree(&aPageInfo);
			ReturnToSystem(&aPageInfo);
			BalanceAges();
			return ETrue;
			}
		else
			{// page no longer discardable so no longer require new page
			ReturnToSystem(newPage);
			return EFalse;
			}
		}

	// Discard the page
	RemovePage(&aPageInfo);
	SetFree(&aPageInfo);
	ReturnToSystem(&aPageInfo);
	BalanceAges();
	
	return ETrue;
	}


/**
First stage in discarding a list of pages.

Must ensure that the pages will still be discardable even if system lock is released.
To be used in conjunction with RamCacheBase::DoDiscardPages1().

@param aPageList A NULL terminated list of the pages to be discarded
@return KErrNone on success.

@pre System lock held
@post System lock held
*/
TInt DemandPaging::DoDiscardPages0(SPageInfo** aPageList)
	{
	__ASSERT_SYSTEM_LOCK;

	SPageInfo* pageInfo;
	while((pageInfo = *aPageList++) != 0)
		{
		RemovePage(pageInfo);
		}
	return KErrNone;
	}


/**
Final stage in discarding a list of page
Finish discarding the pages previously removed by RamCacheBase::DoDiscardPages0().

@param aPageList A NULL terminated list of the pages to be discarded
@return KErrNone on success.

@pre System lock held
@post System lock held
*/
TInt DemandPaging::DoDiscardPages1(SPageInfo** aPageList)
	{
	__ASSERT_SYSTEM_LOCK;

	SPageInfo* pageInfo;
	while((pageInfo = *aPageList++)!=0)
		{
		SetFree(pageInfo);
		ReturnToSystem(pageInfo);
		BalanceAges();
		}
	return KErrNone;
	}


TBool DemandPaging::MayBePaged(TLinAddr aStartAddr, TUint aLength)
	{
	TLinAddr endAddr = aStartAddr + aLength;
	TBool rangeTouchesPagedRom =
		TUint(aStartAddr - iRomPagedLinearBase) < iRomSize  ||
		TUint(endAddr - iRomPagedLinearBase) < iRomSize;
	TBool rangeTouchesCodeArea =
		TUint(aStartAddr - iCodeLinearBase) < iCodeSize  ||
		TUint(endAddr - iCodeLinearBase) < iCodeSize;
	return rangeTouchesPagedRom || rangeTouchesCodeArea;
	}


#ifdef __DEMAND_PAGING_BENCHMARKS__

void DemandPaging::ResetBenchmarkData(TPagingBenchmark aBm)
	{
	SPagingBenchmarkInfo& info = iBenchmarkInfo[aBm];
	info.iCount = 0;
	info.iTotalTime = 0;
	info.iMaxTime = 0;
	info.iMinTime = KMaxTInt;
	}

void DemandPaging::RecordBenchmarkData(TPagingBenchmark aBm, TUint32 aStartTime, TUint32 aEndTime)
	{
	SPagingBenchmarkInfo& info = iBenchmarkInfo[aBm];
	++info.iCount;
#if !defined(HIGH_RES_TIMER) || defined(HIGH_RES_TIMER_COUNTS_UP)
	TInt64 elapsed = aEndTime - aStartTime;
#else
	TInt64 elapsed = aStartTime - aEndTime;
#endif
	info.iTotalTime += elapsed;
	if (elapsed > info.iMaxTime)
		info.iMaxTime = elapsed;
	if (elapsed < info.iMinTime)
		info.iMinTime = elapsed;
	}
	
#endif


//
// DDemandPagingLock
//

EXPORT_C DDemandPagingLock::DDemandPagingLock()
	: iThePager(DemandPaging::ThePager), iReservedPageCount(0), iLockedPageCount(0), iPages(0)
	{
	}


EXPORT_C TInt DDemandPagingLock::Alloc(TInt aSize)
	{	
	if (iThePager)
		return iThePager->ReserveAlloc(aSize,*this);
	else
		return KErrNone;
	}


EXPORT_C void DDemandPagingLock::DoUnlock()
	{
	if (iThePager)
		iThePager->ReserveUnlock(*this);
	}


EXPORT_C void DDemandPagingLock::Free()
	{
	if (iThePager)
		iThePager->ReserveFree(*this);
	}


EXPORT_C TInt Kern::InstallPagingDevice(DPagingDevice* aDevice)
	{
	if (DemandPaging::ThePager)
		return DemandPaging::ThePager->InstallPagingDevice(aDevice);
	else
		return KErrNotSupported;
	}


#else  // !__DEMAND_PAGING__

EXPORT_C DDemandPagingLock::DDemandPagingLock()
	: iLockedPageCount(0)
	{
	}

EXPORT_C TInt DDemandPagingLock::Alloc(TInt /*aSize*/)
	{
	return KErrNone;
	}

EXPORT_C TBool DDemandPagingLock::Lock(DThread* /*aThread*/, TLinAddr /*aStart*/, TInt /*aSize*/)
	{
	return EFalse;
	}

EXPORT_C void DDemandPagingLock::DoUnlock()
	{
	}

EXPORT_C void DDemandPagingLock::Free()
	{
	}

EXPORT_C TInt Kern::InstallPagingDevice(DPagingDevice* aDevice)
	{
	return KErrNotSupported;
	}

#endif // __DEMAND_PAGING__


DMmuCodeSegMemory::DMmuCodeSegMemory(DEpocCodeSeg* aCodeSeg)
	: DEpocCodeSegMemory(aCodeSeg), iCodeAllocBase(KMinTInt)
	{
	}

//#define __DUMP_BLOCKMAP_INFO
DMmuCodeSegMemory::~DMmuCodeSegMemory()
	{
#ifdef __DEMAND_PAGING__
	Kern::Free(iCodeRelocTable);
	Kern::Free(iCodePageOffsets);
	Kern::Free(iDataSectionMemory);
#endif
	}

#ifdef __DEMAND_PAGING__

/**
Read and process the block map and related data.
*/
TInt DMmuCodeSegMemory::ReadBlockMap(const TCodeSegCreateInfo& aInfo)
	{
	__KTRACE_OPT(KPAGING,Kern::Printf("DP: Reading block map for %C", iCodeSeg));

	if (aInfo.iCodeBlockMapEntriesSize <= 0)
		return KErrArgument;  // no block map provided
	
	// Get compression data
	switch (aInfo.iCompressionType)
		{
		case KFormatNotCompressed:
			iCompressionType = SRomPageInfo::ENoCompression;
			break;

		case KUidCompressionBytePair:
			{
			iCompressionType = SRomPageInfo::EBytePair;
			if (!aInfo.iCodePageOffsets)
				return KErrArgument;
			TInt size = sizeof(TInt32) * (iPageCount + 1);
			iCodePageOffsets = (TInt32*)Kern::Alloc(size);
			if (!iCodePageOffsets)
				return KErrNoMemory;
			kumemget32(iCodePageOffsets, aInfo.iCodePageOffsets, size);

#ifdef __DUMP_BLOCKMAP_INFO
			Kern::Printf("CodePageOffsets:");
			for (TInt i = 0 ; i < iPageCount + 1 ; ++i)
				Kern::Printf("  %08x", iCodePageOffsets[i]);
#endif

			TInt last = 0;
			for (TInt j = 0 ; j < iPageCount + 1 ; ++j)
				{
				if (iCodePageOffsets[j] < last ||
					iCodePageOffsets[j] > (aInfo.iCodeLengthInFile + aInfo.iCodeStartInFile))
					{
					__NK_ASSERT_DEBUG(0);
					return KErrCorrupt;
					}
				last = iCodePageOffsets[j];
				}
			}
			break;

		default:
			return KErrNotSupported;
		}		

	// Copy block map data itself...

#ifdef __DUMP_BLOCKMAP_INFO
	Kern::Printf("Original block map");
	Kern::Printf("  block granularity: %d", aInfo.iCodeBlockMapCommon.iBlockGranularity);
	Kern::Printf("  block start offset: %x", aInfo.iCodeBlockMapCommon.iBlockStartOffset);
	Kern::Printf("  start block address: %016lx", aInfo.iCodeBlockMapCommon.iStartBlockAddress);
	Kern::Printf("  local drive number: %d", aInfo.iCodeBlockMapCommon.iLocalDriveNumber);
	Kern::Printf("  entry size: %d", aInfo.iCodeBlockMapEntriesSize);
#endif

	// Find relevant paging device
	iCodeLocalDrive = aInfo.iCodeBlockMapCommon.iLocalDriveNumber;
	if (TUint(iCodeLocalDrive) >= (TUint)KMaxLocalDrives)
		{
		__KTRACE_OPT(KPAGING,Kern::Printf("Bad local drive number"));
		return KErrArgument;
		}
	DemandPaging* pager = DemandPaging::ThePager;
	
	if (!pager->CodePagingDevice(iCodeLocalDrive).iInstalled)
		{
		__KTRACE_OPT(KPAGING,Kern::Printf("No paging device installed for drive"));
		return KErrNotSupported;
		}
	DPagingDevice* device = pager->CodePagingDevice(iCodeLocalDrive).iDevice;

	// Set code start offset
	iCodeStartInFile = aInfo.iCodeStartInFile;
	if (iCodeStartInFile < 0)
		{
		__KTRACE_OPT(KPAGING,Kern::Printf("Bad code start offset"));
		return KErrArgument;
		}
	
	// Allocate buffer for block map and copy from user-side
	TBlockMapEntryBase* buffer = (TBlockMapEntryBase*)Kern::Alloc(aInfo.iCodeBlockMapEntriesSize);
	if (!buffer)
		return KErrNoMemory;
	kumemget32(buffer, aInfo.iCodeBlockMapEntries, aInfo.iCodeBlockMapEntriesSize);
	
#ifdef __DUMP_BLOCKMAP_INFO
	Kern::Printf("  entries:");
	for (TInt k = 0 ; k < aInfo.iCodeBlockMapEntriesSize / sizeof(TBlockMapEntryBase) ; ++k)
		Kern::Printf("    %d: %d blocks at %08x", k, buffer[k].iNumberOfBlocks, buffer[k].iStartBlock);
#endif

	// Initialise block map
	TInt r = iBlockMap.Initialise(aInfo.iCodeBlockMapCommon,
								  buffer,
								  aInfo.iCodeBlockMapEntriesSize,
								  device->iReadUnitShift,
								  iCodeStartInFile + aInfo.iCodeLengthInFile);
	if (r != KErrNone)
		{
		Kern::Free(buffer);
		return r;
		}

#if defined(__DUMP_BLOCKMAP_INFO) && defined(_DEBUG)
	iBlockMap.Dump();
#endif
	
	return KErrNone;
	}

/**
Read code relocation table and import fixup table from user side.
*/
TInt DMmuCodeSegMemory::ReadFixupTables(const TCodeSegCreateInfo& aInfo)
	{
	__KTRACE_OPT(KPAGING,Kern::Printf("DP: Reading fixup tables for %C", iCodeSeg));
	
	iCodeRelocTableSize = aInfo.iCodeRelocTableSize;
	iImportFixupTableSize = aInfo.iImportFixupTableSize;
	iCodeDelta = aInfo.iCodeDelta;
	iDataDelta = aInfo.iDataDelta;
	
	// round sizes to four-byte boundaris...
	TInt relocSize = (iCodeRelocTableSize + 3) & ~3;
	TInt fixupSize = (iImportFixupTableSize + 3) & ~3;

	// copy relocs and fixups...
	iCodeRelocTable = (TUint8*)Kern::Alloc(relocSize+fixupSize);
	if (!iCodeRelocTable)
		return KErrNoMemory;
	iImportFixupTable = iCodeRelocTable + relocSize;
	kumemget32(iCodeRelocTable, aInfo.iCodeRelocTable, relocSize);
	kumemget32(iImportFixupTable, aInfo.iImportFixupTable, fixupSize);
	
	return KErrNone;
	}

#endif


TInt DMmuCodeSegMemory::Create(TCodeSegCreateInfo& aInfo)
	{
	TInt r = KErrNone;	
	if (!aInfo.iUseCodePaging)
		iPageCount=(iRamInfo.iCodeSize+iRamInfo.iDataSize+KPageMask)>>KPageShift;
	else
		{
#ifdef __DEMAND_PAGING__
		iDataSectionMemory = Kern::Alloc(iRamInfo.iDataSize);
		if (!iDataSectionMemory)
			return KErrNoMemory;

		iPageCount=(iRamInfo.iCodeSize+KPageMask)>>KPageShift;
		iDataPageCount=(iRamInfo.iDataSize+KPageMask)>>KPageShift;

		r = ReadBlockMap(aInfo);
		if (r != KErrNone)
			return r;

		iIsDemandPaged = ETrue;
		iCodeSeg->iAttr |= ECodeSegAttCodePaged;
#endif
		}

	iCodeSeg->iSize = (iPageCount+iDataPageCount)<<KPageShift;
	return r;		
	}


TInt DMmuCodeSegMemory::Loaded(TCodeSegCreateInfo& aInfo)
	{
#ifdef __DEMAND_PAGING__
	if(iIsDemandPaged)
		{
		TInt r = ReadFixupTables(aInfo);
		if (r != KErrNone)
			return r;
		}
	TAny* dataSection = iDataSectionMemory;
	if(dataSection)
		{
		UNLOCK_USER_MEMORY();
		memcpy(dataSection,(TAny*)iRamInfo.iDataLoadAddr,iRamInfo.iDataSize);
		LOCK_USER_MEMORY();
		iRamInfo.iDataLoadAddr = (TLinAddr)dataSection;
		}
#endif
	return KErrNone;
	}


void DMmuCodeSegMemory::ApplyCodeFixups(TUint32* aBuffer, TLinAddr aDestAddress)
	{
	__NK_ASSERT_DEBUG(iRamInfo.iCodeRunAddr==iRamInfo.iCodeLoadAddr); // code doesn't work if this isn't true

	START_PAGING_BENCHMARK;
	
	TUint offset = aDestAddress - iRamInfo.iCodeRunAddr;
	__ASSERT_ALWAYS(offset < (TUint)(iRamInfo.iCodeSize + iRamInfo.iDataSize), K::Fault(K::ECodeSegBadFixupAddress));

	// Index tables are only valid for pages containg code
	if (offset >= (TUint)iRamInfo.iCodeSize)
		return;

	UNLOCK_USER_MEMORY();

	TInt page = offset >> KPageShift;

	// Relocate code
	
	if (iCodeRelocTableSize > 0)
		{
		TUint32* codeRelocTable32 = (TUint32*)iCodeRelocTable;
		TUint startOffset = codeRelocTable32[page];
		TUint endOffset = codeRelocTable32[page + 1];
		
		__KTRACE_OPT(KPAGING, Kern::Printf("Performing code relocation: start == %x, end == %x", startOffset, endOffset));
		__ASSERT_ALWAYS(startOffset <= endOffset && endOffset <= (TUint)iCodeRelocTableSize,
						K::Fault(K::ECodeSegBadFixupTables));
		
		TUint8* codeRelocTable8 = (TUint8*)codeRelocTable32;
		const TUint16* ptr = (const TUint16*)(codeRelocTable8 + startOffset);
		const TUint16* end = (const TUint16*)(codeRelocTable8 + endOffset);

		const TUint32 codeDelta = iCodeDelta;
		const TUint32 dataDelta = iDataDelta;

		while (ptr < end)
			{
			TUint16 entry = *ptr++;

			// address of word to fix up is sum of page start and 12-bit offset
			TUint32* addr = (TUint32*)((TUint8*)aBuffer + (entry & 0x0fff));
			
			TUint32 word = *addr;
#ifdef _DEBUG
			TInt type = entry & 0xf000;
			__NK_ASSERT_DEBUG(type == KTextRelocType || type == KDataRelocType);
#endif
			if (entry < KDataRelocType /* => type == KTextRelocType */)
				word += codeDelta;
			else
				word += dataDelta;
			*addr = word;
			}
		}
		
	// Fixup imports
			
	if (iImportFixupTableSize > 0)
		{
		TUint32* importFixupTable32 = (TUint32*)iImportFixupTable;
		TUint startOffset = importFixupTable32[page];
		TUint endOffset = importFixupTable32[page + 1];
		
		__KTRACE_OPT(KPAGING, Kern::Printf("Performing import fixup: start == %x, end == %x", startOffset, endOffset));
		__ASSERT_ALWAYS(startOffset <= endOffset && endOffset <= (TUint)iImportFixupTableSize,
						K::Fault(K::ECodeSegBadFixupTables));
		
		TUint8* importFixupTable8 = (TUint8*)importFixupTable32;
		const TUint16* ptr = (const TUint16*)(importFixupTable8 + startOffset);
		const TUint16* end = (const TUint16*)(importFixupTable8 + endOffset);

		while (ptr < end)
			{
			TUint16 offset = *ptr++;
		
			// get word to write into that address
			// (don't read as a single TUint32 because may not be word-aligned)
			TUint32 wordLow = *ptr++;
			TUint32 wordHigh = *ptr++;
			TUint32 word = (wordHigh << 16) | wordLow;

			__KTRACE_OPT(KPAGING, Kern::Printf("DP: Fixup %08x=%08x", iRamInfo.iCodeRunAddr+(page<<KPageShift)+offset, word));
			*(TUint32*)((TLinAddr)aBuffer+offset) = word;
			}
		}
	
	LOCK_USER_MEMORY();

	END_PAGING_BENCHMARK(DemandPaging::ThePager, EPagingBmFixupCodePage);
	}


TInt DMmuCodeSegMemory::ApplyCodeFixupsOnLoad(TUint32* aBuffer, TLinAddr aDestAddress)
	{
#ifdef __DEMAND_PAGING__
	TInt r=DemandPaging::ThePager->LockRegion((TLinAddr)aBuffer,KPageSize,&Kern::CurrentProcess());
	if(r!=KErrNone)
		return r;
#endif
	ApplyCodeFixups(aBuffer,aDestAddress);
	UNLOCK_USER_MEMORY();
	CacheMaintenance::CodeChanged((TLinAddr)aBuffer, KPageSize);
	LOCK_USER_MEMORY();
#ifdef __DEMAND_PAGING__
	DemandPaging::ThePager->UnlockRegion((TLinAddr)aBuffer,KPageSize,&Kern::CurrentProcess());
#endif
	return KErrNone;
	}


#ifdef __DEMAND_PAGING__

TInt M::CreateVirtualPinObject(TVirtualPinObject*& aPinObject)
	{
	aPinObject = (TVirtualPinObject*) new DDemandPagingLock;
	return aPinObject != NULL ? KErrNone : KErrNoMemory;
	}

TInt M::PinVirtualMemory(TVirtualPinObject* aPinObject, TLinAddr aStart, TUint aSize, DThread* aThread)
	{
	if (!DemandPaging::ThePager)
		return KErrNone;
	
	if (!DemandPaging::ThePager->MayBePaged(aStart, aSize))
		return KErrNone;

	DDemandPagingLock* lock = (DDemandPagingLock*)aPinObject;
	TInt r = lock->Alloc(aSize);
	if (r != KErrNone)
		return r;
	lock->Lock(aThread, aStart, aSize);
	return KErrNone;
	}

TInt M::CreateAndPinVirtualMemory(TVirtualPinObject*& aPinObject, TLinAddr aStart, TUint aSize)
	{
	aPinObject = 0;

	if (!DemandPaging::ThePager)
		return KErrNone;
	if (!DemandPaging::ThePager->MayBePaged(aStart, aSize))
		return KErrNone;

	TInt r = CreateVirtualPinObject(aPinObject);
	if (r != KErrNone)
		return r;

	DDemandPagingLock* lock = (DDemandPagingLock*)aPinObject;
	r = lock->Alloc(aSize);
	if (r != KErrNone)
		return r;
	lock->Lock(TheCurrentThread, aStart, aSize);
	return KErrNone;
	}

void M::UnpinVirtualMemory(TVirtualPinObject* aPinObject)
	{
	DDemandPagingLock* lock = (DDemandPagingLock*)aPinObject;
	if (lock)
		lock->Free();
	}
	
void M::DestroyVirtualPinObject(TVirtualPinObject*& aPinObject)
	{
	DDemandPagingLock* lock = (DDemandPagingLock*)__e32_atomic_swp_ord_ptr(&aPinObject, 0);
	if (lock)
		lock->AsyncDelete();
	}

#else

class TVirtualPinObject
	{	
	};

TInt M::CreateVirtualPinObject(TVirtualPinObject*& aPinObject)
	{
	aPinObject = new TVirtualPinObject;
	return aPinObject != NULL ? KErrNone : KErrNoMemory;
	}

TInt M::PinVirtualMemory(TVirtualPinObject* aPinObject, TLinAddr, TUint, DThread*)
	{
	__ASSERT_DEBUG(aPinObject, K::Fault(K::EVirtualPinObjectBad));
	(void)aPinObject;
	return KErrNone;
	}

TInt M::CreateAndPinVirtualMemory(TVirtualPinObject*& aPinObject, TLinAddr, TUint)
	{
	aPinObject = 0;
	return KErrNone;
	}

void M::UnpinVirtualMemory(TVirtualPinObject* aPinObject)
	{
	__ASSERT_DEBUG(aPinObject, K::Fault(K::EVirtualPinObjectBad));
	(void)aPinObject;
	}

void M::DestroyVirtualPinObject(TVirtualPinObject*& aPinObject)
	{
	TVirtualPinObject* object = (TVirtualPinObject*)__e32_atomic_swp_ord_ptr(&aPinObject, 0);
	if (object)
		Kern::AsyncFree(object);
	}

#endif

TInt M::CreatePhysicalPinObject(TPhysicalPinObject*& aPinObject)
	{
	return KErrNotSupported;
	}

TInt M::PinPhysicalMemory(TPhysicalPinObject*, TLinAddr, TUint, TBool, TUint32&, TUint32*, TUint32&, TUint&, DThread*)
	{
	K::Fault(K::EPhysicalPinObjectBad);
	return KErrNone;
	}

void M::UnpinPhysicalMemory(TPhysicalPinObject* aPinObject)
	{
	K::Fault(K::EPhysicalPinObjectBad);
	}

void M::DestroyPhysicalPinObject(TPhysicalPinObject*& aPinObject)
	{
	K::Fault(K::EPhysicalPinObjectBad);
	}


//
// Kernel map and pin (Not supported on the moving or multiple memory models).
//

TInt M::CreateKernelMapObject(TKernelMapObject*&, TUint)
	{
	return KErrNotSupported;
	}


TInt M::MapAndPinMemory(TKernelMapObject*, DThread*, TLinAddr, TUint, TUint, TLinAddr&, TPhysAddr*)
	{
	return KErrNotSupported;
	}


void M::UnmapAndUnpinMemory(TKernelMapObject*)
	{
	}


void M::DestroyKernelMapObject(TKernelMapObject*&)
	{
	}


// Misc DPagingDevice methods

EXPORT_C NFastMutex* DPagingDevice::NotificationLock()
	{
	// use the system lock
	return &TheScheduler.iLock;
	}

EXPORT_C void DPagingDevice::NotifyIdle()
	{
	// Not used on this memory model
	}

EXPORT_C void DPagingDevice::NotifyBusy()
	{
	// Not used on this memory model
	}

EXPORT_C TInt Cache::SyncPhysicalMemoryBeforeDmaWrite(TPhysAddr* , TUint , TUint , TUint , TUint32 )
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncPhysicalMemoryBeforeDmaWrite");
	return KErrNotSupported;
	}

EXPORT_C TInt Cache::SyncPhysicalMemoryBeforeDmaRead(TPhysAddr* , TUint , TUint , TUint , TUint32 )
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncPhysicalMemoryBeforeDmaRead");
	return KErrNotSupported;
	}
EXPORT_C TInt Cache::SyncPhysicalMemoryAfterDmaRead(TPhysAddr* , TUint , TUint , TUint , TUint32 )
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncPhysicalMemoryAfterDmaRead");
	return KErrNotSupported;
	}

//
//	Page moving methods
//

/*
 * Move a page from aOld to aNew safely, updating any references to the page
 * stored elsewhere (such as page table entries). The destination page must
 * already be allocated. If the move is successful, the source page will be
 * freed and returned to the allocator.
 *
 * @pre RAM alloc mutex must be held.
 * @pre Calling thread must be in a critical section.
 * @pre Interrupts must be enabled.
 * @pre Kernel must be unlocked.
 * @pre No fast mutex can be held.
 * @pre Call in a thread context.
 */
TInt MmuBase::MovePage(TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "Defrag::DoMovePage");
	__ASSERT_WITH_MESSAGE_MUTEX(MmuBase::RamAllocatorMutex, "Ram allocator mutex must be held", "Defrag::DoMovePage");
	__KTRACE_OPT(KMMU,Kern::Printf("MmuBase::MovePage() old=%08x",aOld));
	TInt r = KErrNotSupported;
#if defined(__CPU_X86) && defined(__MEMMODEL_MULTIPLE__)
	return r;
#endif
	aNew = KPhysAddrInvalid;
	NKern::LockSystem();
	SPageInfo* pi = SPageInfo::SafeFromPhysAddr(aOld);
	if (!pi)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("MmuBase::MovePage() fails: page has no PageInfo"));
		r = KErrArgument;
		goto fail;
		}
	if (pi->LockCount())
		{
		__KTRACE_OPT(KMMU,Kern::Printf("MmuBase::MovePage() fails: page is locked"));
		goto fail;
		}
	
	switch(pi->Type())
		{
	case SPageInfo::EUnused:
		// Nothing to do - we allow this, though, in case the caller wasn't
		// actually checking the free bitmap.
		r = KErrNotFound;
		__KTRACE_OPT(KMMU,Kern::Printf("MmuBase::MovePage(): page unused"));
		break;

	case SPageInfo::EChunk:
		{
		// It's a chunk - we need to investigate what it's used for.
		DChunk* chunk = (DChunk*)pi->Owner();
		TInt offset = pi->Offset()<<KPageShift;

		switch(chunk->iChunkType)
			{
		case EKernelData:
		case EKernelMessage:
			// The kernel data/bss/heap chunk pages are not moved as DMA may be accessing them.
			__KTRACE_OPT(KMMU, Kern::Printf("MmuBase::MovePage() fails: kernel data"));
			goto fail;

		case EKernelStack:
			// The kernel thread stack chunk.
			r = MoveKernelStackPage(chunk, offset, aOld, aNew, aBlockZoneId, aBlockRest);
			__KTRACE_OPT(KMMU,if (r!=KErrNone) Kern::Printf("MmuBase::MovePage() fails: k stack r%d",r));
			__NK_ASSERT_DEBUG(NKern::HeldFastMutex()==0);
 			goto released;

		case EKernelCode:
		case EDll:
			// The kernel code chunk, or a global user code chunk.
			r = MoveCodeChunkPage(chunk, offset, aOld, aNew, aBlockZoneId, aBlockRest);
			__KTRACE_OPT(KMMU,if (r!=KErrNone) Kern::Printf("MmuBase::MovePage() fails: code chk r%d",r));
			__NK_ASSERT_DEBUG(NKern::HeldFastMutex()==0);
			goto released;

		case ERamDrive:
		case EUserData:
		case EDllData:
		case EUserSelfModCode:
			// A data chunk of some description.
			r = MoveDataChunkPage(chunk, offset, aOld, aNew, aBlockZoneId, aBlockRest);
			__KTRACE_OPT(KMMU,if (r!=KErrNone) Kern::Printf("MmuBase::MovePage() fails: data chk r%d",r));
			__NK_ASSERT_DEBUG(NKern::HeldFastMutex()==0);
			goto released;

		case ESharedKernelSingle:
		case ESharedKernelMultiple:
		case ESharedIo:
		case ESharedKernelMirror:
			// These chunk types cannot be moved
			r = KErrNotSupported;
			__KTRACE_OPT(KMMU,if (r!=KErrNone) Kern::Printf("MmuBase::MovePage() fails: shared r%d",r));
			break;

		case EUserCode:
		default:
			// Unknown page type, or EUserCode.
			// EUserCode is not used in moving model, and on multiple model
			// it never owns any pages so shouldn't be found via SPageInfo
			__KTRACE_OPT(KMMU,Kern::Printf("Defrag::DoMovePage fails: unknown chunk type %d",chunk->iChunkType));
			Panic(EDefragUnknownChunkType);
			}
		}
		break;

	case SPageInfo::ECodeSegMemory:
		// It's a code segment memory section (multiple model only)
		r = MoveCodeSegMemoryPage((DMemModelCodeSegMemory*)pi->Owner(), pi->Offset()<<KPageShift, aOld, aNew, aBlockZoneId, aBlockRest);
		__KTRACE_OPT(KMMU,if (r!=KErrNone) Kern::Printf("MmuBase::MovePage() fails: codeseg r%d",r));
		__NK_ASSERT_DEBUG(NKern::HeldFastMutex()==0);
		goto released;

	case SPageInfo::EPagedROM:
	case SPageInfo::EPagedCode:
	case SPageInfo::EPagedData:
	case SPageInfo::EPagedCache:
	case SPageInfo::EPagedFree:
		{// DP or RamCache page so attempt to discard it. Added for testing purposes only
		//  In normal use ClearDiscardableFromZone will have already removed RAM cache pages
		r = KErrInUse;
		MmuBase& mmu = *MmuBase::TheMmu;
		RamCacheBase& ramCache = *(mmu.iRamCache);
		if (ramCache.IsPageDiscardable(*pi))
			{
			if (ramCache.DoDiscardPage(*pi, KRamZoneInvalidId, EFalse))
				{// Sucessfully discarded the page.
				r = KErrNone;
				}
			}
		__KTRACE_OPT(KMMU,if (r!=KErrNone) Kern::Printf("MmuBase::MovePage() fails: paged r%d",r));
		goto fail; // Goto fail to release the system lock.	
		}

		
	case SPageInfo::EPageTable:
	case SPageInfo::EPageDir:
	case SPageInfo::EPtInfo:
	case SPageInfo::EInvalid:
	case SPageInfo::EFixed:
	case SPageInfo::EShadow:
		// These page types cannot be moved (or don't need to be moved)
		r = KErrNotSupported;
		__KTRACE_OPT(KMMU,if (r!=KErrNone) Kern::Printf("MmuBase::MovePage() fails: PT etc r%d",r));
		break;

	default:
		// Unknown page type
		__KTRACE_OPT(KMMU,Kern::Printf("MmuBase::MovePage() fails: unknown page type %d",pi->Type()));
		Panic(EDefragUnknownPageType);
		}

fail:
	NKern::UnlockSystem();
released:
	__KTRACE_OPT(KMMU,Kern::Printf("MmuBase::MovePage() returns %d",r));
	return r;
	}


TInt MmuBase::DiscardPage(TPhysAddr aAddr, TUint aBlockZoneId, TBool aBlockRest)
	{
	TInt r = KErrInUse;
	NKern::LockSystem();
	SPageInfo* pageInfo = SPageInfo::SafeFromPhysAddr(aAddr);
	if (pageInfo != NULL)
		{// Allocatable page at this address so is it a discardable one?
		if (iRamCache->IsPageDiscardable(*pageInfo))
			{
			// Discard this page and return it to the ram allocator
			if (!iRamCache->DoDiscardPage(*pageInfo, aBlockZoneId, aBlockRest))
				{// Couldn't discard the page.
				if (aBlockRest)
					{
					__KTRACE_OPT(KMMU, Kern::Printf("ClearDiscardableFromZone: page discard fail addr %x", aAddr));
					NKern::UnlockSystem();
					return KErrNoMemory;
					}
				}
			else
				{// Page discarded successfully.
				r = KErrNone;
				}
			}
		}
	NKern::UnlockSystem();
	return r;
	}

TUint MmuBase::NumberOfFreeDpPages()
	{
	TUint free = 0;
	if(iRamCache)
		{
		free = iRamCache->NumberOfFreePages();
		}
	return free;
	}


EXPORT_C TInt Epoc::MovePhysicalPage(TPhysAddr aOld, TPhysAddr& aNew, TRamDefragPageToMove aPageToMove)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::MovePhysicalPage");
	__KTRACE_OPT(KMMU,Kern::Printf("Epoc::MovePhysicalPage() old=%08x pageToMove=%d",aOld,aPageToMove));

	switch(aPageToMove)
		{
		case ERamDefragPage_Physical:
			break;
		default:
			return KErrNotSupported;
		}

	MmuBase::Wait();
	TInt r=M::MovePage(aOld,aNew,KRamZoneInvalidId,0);
	if (r!=KErrNone)
		aNew = KPhysAddrInvalid;
	MmuBase::Signal();
	__KTRACE_OPT(KMMU,Kern::Printf("Epoc::MovePhysicalPage() returns %d",r));
	return r;
	}


TInt M::RamDefragFault(TAny* aExceptionInfo)
	{
	// If the mmu has been initialised then let it try processing the fault.
	if(MmuBase::TheMmu)
		return MmuBase::TheMmu->RamDefragFault(aExceptionInfo);
	return KErrAbort;
	}


void M::RamZoneClaimed(SZone* aZone)
	{
	// Lock each page.  OK to traverse SPageInfo array as we know no unknown
	// pages are in the zone.
	SPageInfo* pageInfo = SPageInfo::FromPhysAddr(aZone->iPhysBase);
	SPageInfo* pageInfoEnd = pageInfo + aZone->iPhysPages;
	for (; pageInfo < pageInfoEnd; ++pageInfo)
		{
		NKern::LockSystem();
		__NK_ASSERT_DEBUG(pageInfo->Type()==SPageInfo::EUnused);
		pageInfo->Lock();
		NKern::UnlockSystem();
		}
	// For the sake of platform security we have to clear the memory. E.g. the driver
	// could assign it to a chunk visible to user side.  Set LSB so ClearPages
	// knows this is a contiguous memory region.
	Mmu::Get().ClearPages(aZone->iPhysPages, (TPhysAddr*)(aZone->iPhysBase|1));
	}
