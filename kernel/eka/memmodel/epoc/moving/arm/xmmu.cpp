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
// e32\memmodel\epoc\moving\arm\xmmu.cpp
// 
//

#include "arm_mem.h"
#include <mmubase.inl>
#include <ramcache.h>
#include <demand_paging.h>
#include "execs.h"
#include <defrag.h>
#include "cache_maintenance.h"


extern void FlushTLBs();

#if defined(__CPU_SA1__)
const TPde KRomSectionPermissions	=	SECTION_PDE(KArmV45PermRORO, KArmV45MemAttWB, EDomainClient);
const TPde KShadowPdePerm			=	PT_PDE(EDomainClient);
const TPte KPtPtePerm				=	SP_PTE(KArmV45PermRWNO, KArmV45MemAttBuf);	// page tables not cached
const TPte KRomPtePermissions		=	SP_PTE(KArmV45PermRORO, KArmV45MemAttWB);	// ROM is cached, read-only for everyone
const TPte KShadowPtePerm			=	SP_PTE(KArmV45PermRWRO, KArmV45MemAttWB);	// shadowed ROM is cached, supervisor writeable

#elif defined(__CPU_ARM710T__) || defined(__CPU_ARM720T__)
const TPde KRomSectionPermissions	=	SECTION_PDE(KArmV45PermRORO, KArmV45MemAttWB, EDomainClient);
const TPde KShadowPdePerm			=	PT_PDE(EDomainClient);
const TPte KPtPtePerm				=	SP_PTE(KArmV45PermRWNO, KArmV45MemAttWB);	// page tables cached (write-through)
const TPte KRomPtePermissions		=	SP_PTE(KArmV45PermRORO, KArmV45MemAttWB);	// ROM is cached, read-only for everyone
const TPte KShadowPtePerm			=	SP_PTE(KArmV45PermRWRO, KArmV45MemAttWB);	// shadowed ROM is cached, supervisor writeable

#elif defined(__CPU_ARM920T__) || defined(__CPU_ARM925T__) || defined(__CPU_ARM926J__)
const TPde KRomSectionPermissions	=	SECTION_PDE(KArmV45PermRORO, KArmV45MemAttWT, EDomainClient);
const TPde KShadowPdePerm			=	PT_PDE(EDomainClient);
const TPte KPtPtePerm				=	SP_PTE(KArmV45PermRWNO, KArmV45MemAttWT);	// page tables cached write through
const TPte KRomPtePermissions		=	SP_PTE(KArmV45PermRORO, KArmV45MemAttWT);	// ROM is cached, read-only for everyone
const TPte KShadowPtePerm			=	SP_PTE(KArmV45PermRWRO, KArmV45MemAttWT);	// shadowed ROM is cached, supervisor writeable

#elif defined(__CPU_XSCALE__)
	#ifdef __CPU_XSCALE_MANZANO__
const TPde KRomSectionPermissions	=	SECTION_PDE(KArmV45PermRORO, KXScaleMemAttWTRA_WBWA, EDomainClient);
const TPde KShadowPdePerm			=	PT_PDE(EDomainClient);
const TPte KPtPtePerm				=	SP_PTE(KArmV45PermRWNO, KXScaleMemAttWTRA_WBWA);	// page tables write-through cached
const TPte KRomPtePermissions		=	SP_PTE(KArmV45PermRORO, KXScaleMemAttWTRA_WBWA);	// ROM is cached, read-only for everyone
const TPte KShadowPtePerm			=	SP_PTE(KArmV45PermRWRO, KXScaleMemAttWTRA_WBWA);	// shadowed ROM is cached, supervisor writeable
	#else
const TPde KRomSectionPermissions	=	SECTION_PDE(KArmV45PermRORO, KXScaleMemAttWTRA, EDomainClient);
const TPde KShadowPdePerm			=	PT_PDE(EDomainClient);
const TPte KPtPtePerm				=	SP_PTE(KArmV45PermRWNO, KXScaleMemAttWTRA);	// page tables write-through cached
const TPte KRomPtePermissions		=	SP_PTE(KArmV45PermRORO, KXScaleMemAttWTRA);	// ROM is cached, read-only for everyone
const TPte KShadowPtePerm			=	SP_PTE(KArmV45PermRWRO, KXScaleMemAttWTRA);	// shadowed ROM is cached, supervisor writeable
	#endif
#endif

const TPte KPtInfoPtePerm = KPtPtePerm;
const TPde KPtPdePerm = PT_PDE(EDomainClient);

// Permissions for each chunk type
enum TPTEProperties
	{
	ESupRo	=	SP_PTE(KArmV45PermRORO, KDefaultCaching),
	ESupRw	=	SP_PTE(KArmV45PermRWNO, KDefaultCaching),
	EUserRo	=	SP_PTE(KArmV45PermRWRO, KDefaultCaching),
	EUserRw	=	SP_PTE(KArmV45PermRWRW, KDefaultCaching)
	};

LOCAL_D const TPde ChunkPdePermissions[ENumChunkTypes] =
	{
	PT_PDE(EDomainClient),		// EKernelData
	PT_PDE(EDomainClient),		// EKernelStack
	PT_PDE(EDomainClient),		// EKernelCode
	PT_PDE(EDomainClient),		// EDll
	PT_PDE(EDomainClient),		// EUserCode - user/ro & sup/rw everywhere
	PT_PDE(EDomainClient),		// ERamDrive - sup/rw accessed by domain change

	// user data or self modifying code is sup/rw, user no access at home. It's user/rw & sup/rw when running
	// note ARM MMU architecture prevents implementation of user read-only data
	PT_PDE(EDomainClient),		// EUserData
	PT_PDE(EDomainClient),		// EDllData
	PT_PDE(EDomainClient),		// EUserSelfModCode
	PT_PDE(EDomainClient),		// ESharedKernelSingle
	PT_PDE(EDomainClient),		// ESharedKernelMultiple
	PT_PDE(EDomainClient),		// ESharedIo
	PT_PDE(EDomainClient),		// ESharedKernelMirror (unused in this memory model)
	PT_PDE(EDomainClient),		// EKernelMessage
	};

const TPde KUserDataRunningPermissions = PT_PDE(EDomainVarUserRun);

LOCAL_D const TPte ChunkPtePermissions[ENumChunkTypes] =
	{
	ESupRw,					// EKernelData
	ESupRw,					// EKernelStack
	ESupRw,					// EKernelCode
	EUserRo,				// EDll
	EUserRo,				// EUserCode
	ESupRw,					// ERamDrive
	ESupRw,					// EUserData
	ESupRw,					// EDllData
	ESupRw,					// EUserSelfModCode
	ESupRw,					// ESharedKernelSingle
	ESupRw,					// ESharedKernelMultiple
	ESupRw,					// ESharedIo
	ESupRw,					// ESharedKernelMirror (unused in this memory model)
	ESupRw,					// EKernelMessage
	};

const TPte KUserCodeLoadPte = (TPte)EUserRo;
const TPte KKernelCodeRunPte = (TPte)ESupRw;

// Inline functions for simple transformations
inline TLinAddr PageTableLinAddr(TInt aId)
	{
	return KPageTableBase + (aId<<KPageTableShift);
	}

inline TPte* PageTable(TInt aId)
	{
	return (TPte*)(KPageTableBase+(aId<<KPageTableShift));
	}

inline TPde* PageDirectoryEntry(TLinAddr aLinAddr)
	{
	return PageDirectory + (aLinAddr>>KChunkShift);
	}

inline TBool IsPageTable(TPde aPde)
	{
	return ((aPde&KPdeTypeMask)==KArmV45PdePageTable);
	}

inline TBool IsSectionDescriptor(TPde aPde)
	{
	return ((aPde&KPdeTypeMask)==KArmV45PdeSection);
	}

inline TBool IsPresent(TPte aPte)
	{
	return (aPte&KPtePresentMask);
	}

inline TPhysAddr PageTablePhysAddr(TPde aPde)
	{
	return aPde & KPdePageTableAddrMask;
	}

inline TPhysAddr PhysAddrFromSectionDescriptor(TPde aPde)
	{
	return aPde & KPdeSectionAddrMask;
	}

extern void InvalidateTLBForPage(TLinAddr /*aLinAddr*/);

void Mmu::SetupInitialPageInfo(SPageInfo* aPageInfo, TLinAddr aChunkAddr, TInt aPdeIndex)
	{
	__ASSERT_ALWAYS(aChunkAddr==0 || aChunkAddr>=KRamDriveEndAddress, Panic(EBadInitialPageAddr));
	TLinAddr addr = aChunkAddr + (aPdeIndex<<KPageShift);
	if (aPageInfo->Type()!=SPageInfo::EUnused)
		return;	// already set (page table)
	if (addr == KPageTableInfoBase)
		{
		aPageInfo->SetPtInfo(0);
		aPageInfo->Lock();
		}
	else if (addr>=KPageDirectoryBase && addr<(KPageDirectoryBase+KPageDirectorySize))
		{
		aPageInfo->SetPageDir(0,aPdeIndex);
		aPageInfo->Lock();
		}
	else
		aPageInfo->SetFixed();
	}

void Mmu::SetupInitialPageTableInfo(TInt aId, TLinAddr aChunkAddr, TInt aNumPtes)
	{
	__ASSERT_ALWAYS(aChunkAddr==0 || aChunkAddr>=KRamDriveEndAddress, Panic(EBadInitialPageAddr));
	SPageTableInfo& pti=PtInfo(aId);
	pti.iCount=aNumPtes;
	pti.SetGlobal(aChunkAddr>>KChunkShift);
	}

TInt Mmu::GetPageTableId(TLinAddr aAddr)
	{
	TInt id=-1;
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::PageTableId(%08x)",aAddr));
	TInt pdeIndex=aAddr>>KChunkShift;
	TPde pde = PageDirectory[pdeIndex];
	if (IsPageTable(pde))
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
	TInt pdeIndex=aAddr>>KChunkShift;
	TPde pde = PageDirectory[pdeIndex];
	if (IsPageTable(pde))
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
	return aPte & KPtePresentMask;
	}

TPhysAddr ArmMmu::PtePhysAddr(TPte aPte, TInt aPteIndex)
	{
	TUint pte_type = aPte & KPteTypeMask;
	if (pte_type == KArmV45PteLargePage)
		return (aPte & KPteLargePageAddrMask) + (TPhysAddr(aPteIndex << KPageShift) & KLargePageMask);
	else if (pte_type != 0)
		return aPte & KPteSmallPageAddrMask;
	return KPhysAddrInvalid;
	}

TPhysAddr ArmMmu::PdePhysAddr(TLinAddr aAddr)
	{
	TPde pde = PageDirectory[aAddr>>KChunkShift];
	if (IsSectionDescriptor(pde))
		return PhysAddrFromSectionDescriptor(pde);
	return KPhysAddrInvalid;
	}

TPte* SafePageTableFromPde(TPde aPde)
	{
	if((aPde&KPdeTypeMask)==KArmV45PdePageTable)
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

TPte* SafePtePtrFromLinAddr(TLinAddr aAddress)
	{
	TPde pde = PageDirectory[aAddress>>KChunkShift];
	TPte* pt = SafePageTableFromPde(pde);
	if(pt)
		pt += (aAddress>>KPageShift)&(KChunkMask>>KPageShift);
	return pt;
	}

#ifdef __ARMCC__
	__forceinline /* RVCT ignores normal inline qualifier :-( */
#else
	inline
#endif
TPte* PtePtrFromLinAddr(TLinAddr aAddress)
	{
	TPde pde = PageDirectory[aAddress>>KChunkShift];
	SPageInfo* pi = SPageInfo::FromPhysAddr(pde);
	TInt id = (pi->Offset()<<KPtClusterShift) | ((pde>>KPageTableShift)&KPtClusterMask);
	TPte* pt = PageTable(id);
	pt += (aAddress>>KPageShift)&(KChunkMask>>KPageShift);
	return pt;
	}


TInt ArmMmu::LinearToPhysical(TLinAddr aLinAddr, TInt aSize, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList)
	{
	TPhysAddr physStart = ArmMmu::LinearToPhysical(aLinAddr);
	TPhysAddr nextPhys = physStart&~KPageMask;

	TUint32* pageList = aPhysicalPageList;

	TInt pageIndex = aLinAddr>>KPageShift;
	TInt pagesLeft = ((aLinAddr+aSize-1)>>KPageShift)+1 - pageIndex;
	TPde* pdePtr = &PageDirectory[aLinAddr>>KChunkShift];

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
		if(pdeType==KArmV45PdeSection)
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
				if (pte_type >= KArmV45PteSmallPage)
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
				if (pte_type == KArmV45PteLargePage)
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

TPhysAddr ArmMmu::LinearToPhysical(TLinAddr aLinAddr)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::LinearToPhysical(%08x)",aLinAddr));
	TPhysAddr phys = KPhysAddrInvalid;
	TPde pde = PageDirectory[aLinAddr>>KChunkShift];
	if (IsPageTable(pde))
		{
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pde);
		if (pi)
			{
			TInt id = (pi->Offset()<<KPtClusterShift) | ((pde>>KPageTableShift)&KPtClusterMask);
			TInt pteIndex = (aLinAddr & KChunkMask)>>KPageShift;
			TPte pte = PageTable(id)[pteIndex];
			TUint pte_type = pte & KPteTypeMask;
			if (pte_type == KArmV45PteLargePage)
				{
				phys = (pte & KPteLargePageAddrMask) + (aLinAddr & KLargePageMask);
				__KTRACE_OPT(KMMU,Kern::Printf("Mapped with 64K page - returning %08x", phys));
				}
			else if (pte_type != 0)
				{
				phys = (pte & KPteSmallPageAddrMask) + (aLinAddr & KPageMask);
				__KTRACE_OPT(KMMU,Kern::Printf("Mapped with 4K page - returning %08x", phys));
				}
			}
		}
	else if (IsSectionDescriptor(pde))
		{
		phys = (pde & KPdeSectionAddrMask) + (aLinAddr & KChunkMask);
		__KTRACE_OPT(KMMU,Kern::Printf("Mapped with section - returning %08x", phys));
		}
	else
		{
		__KTRACE_OPT(KMMU,Kern::Printf("Address invalid"));
		}
	return phys;
	}


TInt ArmMmu::PreparePagesForDMA(TLinAddr aLinAddr, TInt aSize, TPhysAddr* aPhysicalPageList)
//Returns the list of physical pages belonging to the specified memory space.
//Checks these pages belong to a chunk marked as being trusted. 
//Locks these pages so they can not be moved by e.g. ram defragmentation.
	{
	SPageInfo* pi = NULL;
	DChunk* chunk = NULL;
	TInt err = KErrNone;

	__NK_ASSERT_DEBUG(MM::MaxPagesInOneGo == 32);	// Needs to be a power of 2.
	TUint flashMask = MM::MaxPagesInOneGo - 1;
	
	__KTRACE_OPT(KMMU2,Kern::Printf("ArmMmu::PreparePagesForDMA %08x+%08x, asid=%d",aLinAddr,aSize));

	TUint32* pageList = aPhysicalPageList;
	TInt pagesInList = 0;				//The number of pages we put in the list so far
	
	TInt pageIndex = (aLinAddr & KChunkMask) >> KPageShift;	// Index of the page within the section
	TInt pagesLeft = ((aLinAddr & KPageMask) + aSize + KPageMask) >> KPageShift;

	MmuBase::Wait(); 	// RamAlloc Mutex for accessing page/directory tables.
	NKern::LockSystem();// SystemlLock for accessing SPageInfo objects.
	
	TPde* pdePtr = PageDirectory + (aLinAddr>>KChunkShift);
	
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
			{
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
	iTempAddr=KTempAddr;
	iSecondTempAddr=KSecondTempAddr;
	iMapSizes=KPageSize|KLargePageSize|KChunkSize;
	iRomLinearBase = ::RomHeaderAddress;
	iRomLinearEnd = KRomLinearEnd;
	iShadowPtePerm = KShadowPtePerm;
	iShadowPdePerm = KShadowPdePerm;

	// Mmu data
	TInt total_ram=TheSuperPage().iTotalRamSize;

#if defined(__HAS_EXTERNAL_CACHE__) 
	//L2 cache on ARMv5 is always in write-back mode => must be always purged
	iDecommitThreshold = CacheMaintenance::SyncAllPerformanceThresholdPages();
#else
	iDecommitThreshold = 0; ///no cache consistency issues on decommit
#endif

	iDataSectionBase = KDataSectionBase;
	iDataSectionEnd = KDataSectionEnd;
	iMaxDllDataSize=Min(total_ram/2, 0x08000000);					// phys RAM/2 up to 128Mb
	iMaxDllDataSize=(iMaxDllDataSize+iChunkMask)&~iChunkMask;		// round up to chunk size
	iMaxUserCodeSize=Min(total_ram, 0x10000000);					// phys RAM up to 256Mb
	iMaxUserCodeSize=(iMaxUserCodeSize+iChunkMask)&~iChunkMask;		// round up to chunk size
	iMaxKernelCodeSize=Min(total_ram/2, 0x04000000);				// phys RAM/2 up to 64Mb
	iMaxKernelCodeSize=(iMaxKernelCodeSize+iChunkMask)&~iChunkMask;	// round up to chunk size
	iPdeBase=KPageDirectoryBase;
	iUserCodeLoadPtePerm=KUserCodeLoadPte;
	iKernelCodePtePerm=KKernelCodeRunPte;
	iDllDataBase = KDataSectionEnd - iMaxDllDataSize;
	iUserCodeBase = KPageInfoLinearBase - iMaxUserCodeSize;
	iKernelCodeBase = iUserCodeBase - iMaxKernelCodeSize;

	__KTRACE_OPT(KMMU,Kern::Printf("DDS %08x UCS %08x KCS %08x", iMaxDllDataSize, iMaxUserCodeSize, iMaxKernelCodeSize));
	__KTRACE_OPT(KMMU,Kern::Printf("DDB %08x KCB %08x UCB %08x RLB %08x", iDllDataBase, iKernelCodeBase, iUserCodeBase, iRomLinearBase));

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

	__KTRACE_OPT(KBOOT,Kern::Printf("K::MaxMemCopyInOneGo=0x%x",K::MaxMemCopyInOneGo));
	K::MemModelAttributes=EMemModelTypeMoving|EMemModelAttrNonExProt|EMemModelAttrKernProt|EMemModelAttrWriteProt|
						EMemModelAttrVA|EMemModelAttrProcessProt|EMemModelAttrSameVA|EMemModelAttrSupportFixed|
						EMemModelAttrSvKernProt|EMemModelAttrIPCKernProt;

	Arm::DefaultDomainAccess=KDefaultDomainAccess;

	// Domains 0-3 are preallocated
	// 0=Variable user running, 1=Client, 2=Page tables, 3=RAM drive
	Domains=(~(0xffffffffu<<ENumDomains))&0xfffffff0u;

	iMaxPageTables = 1<<(32-KChunkShift);		// possibly reduced when RAM size known

	Mmu::Init1();
	}

void ArmMmu::DoInit2()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("ArmMmu::DoInit2"));
	iTempPte=PageTable(GetPageTableId(iTempAddr))+((iTempAddr&KChunkMask)>>KPageShift);
	iSecondTempPte=PageTable(GetPageTableId(iSecondTempAddr))+((iSecondTempAddr&KChunkMask)>>KPageShift);
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("iTempAddr=%08x, iTempPte=%08x, iSecondTempAddr=%08x, iSecondTempPte=%08x", iTempAddr, iTempPte, iSecondTempAddr, iSecondTempPte));
	CreateKernelSection(iKernelCodeBase, KPageShift);
	iHomePdeMap=(TUint32*)Kern::AllocZ(-KSuperPageLinAddr>>KChunkShift<<2);
	iHomePdeMap=(TUint32*)((TUint32)iHomePdeMap-(KSuperPageLinAddr>>KChunkShift<<2)); //adjust the pointer so it's indexed by address>>20
#if defined(__CPU_WRITE_BACK_CACHE)
#if defined(__CPU_HAS_SINGLE_ENTRY_DCACHE_FLUSH)
	if (InternalCache::Info[KCacheInfoD].iLineLength == 32)
		iCopyPageFn = &::CopyPageForRemap32;
	else if (InternalCache::Info[KCacheInfoD].iLineLength == 16)
		iCopyPageFn = &::CopyPageForRemap16;
	else
		Panic(ENoCopyPageFunction);		
#else
#error Write-back cache without single entry dcache flush is not supported
#endif
#else // !__CPU_HAS_WRITE_BACK_CACHE
	iCopyPageFn = &::CopyPageForRemapWT;
#endif
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
	while(aNumPages--)
		{
		TPhysAddr pa = *aPageList++;
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
	__DRAIN_WRITE_BUFFER;
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
	TPte* pPte=PageTable(aId)+ptOffset;						// address of first PTE
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
	__DRAIN_WRITE_BUFFER;
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

void ArmMmu::RemapPage(TInt aId, TUint32 aAddr, TPhysAddr aOldAddr, TPhysAddr aNewAddr, TPte aPtePerm, DProcess* /*aProcess*/)
//
// Replace the mapping at address aAddr in page table aId.
// Update the page information array for both the old and new pages.
// Call this with the system locked.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::RemapPage() id=%d addr=%08x old=%08x new=%08x perm=%08x", aId, aAddr, aOldAddr, aNewAddr, aPtePerm));

	TInt ptOffset=(aAddr&KChunkMask)>>KPageShift;			// entry number in page table
	TPte* pPte=PageTable(aId)+ptOffset;						// address of PTE
	TPte pte=*pPte;

	TUint pageType = (pte & KPteTypeMask);
	if (pageType == KArmPteSmallPage || pageType == 0)
		{
		__ASSERT_ALWAYS((pte & KPteSmallPageAddrMask) == aOldAddr || pte==KPteNotPresentEntry, Panic(ERemapPageFailed));
		SPageInfo* oldpi = SPageInfo::FromPhysAddr(aOldAddr);
		__ASSERT_DEBUG(oldpi->LockCount()==0,Panic(ERemapPageFailed));

		// remap page
		*pPte = aNewAddr | aPtePerm;					// overwrite PTE
		__KTRACE_OPT(KMMU,Kern::Printf("Writing PTE %08x to %08x",*pPte,pPte));
		__DRAIN_WRITE_BUFFER;
		InvalidateTLBForPage(aAddr);		// flush any corresponding TLB entry

		// update new pageinfo, clear old
		SPageInfo* pi = SPageInfo::FromPhysAddr(aNewAddr);
		pi->Set(oldpi->Type(),oldpi->Owner(),oldpi->Offset());
		oldpi->SetUnused();
		}
	else
		{
		__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::RemapPage() called on a non-4K page!"));
		Panic(ERemapPageFailed);
		}
	}

void ArmMmu::RemapKernelPage(TInt aId, TLinAddr aSrc, TLinAddr aDest, TPhysAddr aNewPhys, TPte aPtePerm)
//
// Replace the mapping at address aAddr in page table aId.
// Called with the system locked.
// MUST NOT INVOKE ANY TRACING - or do anything else that might touch the kernel heap
// We are depending on this not reintroducing any of the cache lines we previously
// invalidated.
//
	{
	TInt ptOffset=(aSrc&KChunkMask)>>KPageShift;			// entry number in page table
	TPte* pPte=PageTable(aId)+ptOffset;						// address of PTE

	TInt irq = NKern::DisableAllInterrupts();
	CopyPageForRemap(aDest, aSrc);
	*pPte = aNewPhys | aPtePerm;					// overwrite PTE
	__DRAIN_WRITE_BUFFER;
	InvalidateTLBForPage(aSrc);		// flush any corresponding TLB entry
	NKern::RestoreInterrupts(irq);
	}

TInt ArmMmu::UnmapPages(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TBool aSetPagesFree, TInt& aNumPtes, TInt& aNumFree, DProcess*)
//
// Unmap a specified area at address aAddr in page table aId. Place physical addresses of unmapped
// pages into aPageList, and count of unmapped pages into aNumPtes.
// Return number of pages still mapped using this page table.
// Call this with the system locked.
// @param aId 			Identifies Page Table to unmap PTEs(Page Table Entries) from.
// @param aAddr Base 	Base Virtual Address of the region to unmap. It (indirectly) specifies the first PTE in this Page Table to unmap.
// @param aNumPages 	The number of consecutive PTEs to unmap.
// @param aPageList 	Points to pre-allocated array. On return, it is filled in with the list of physical addresses of the unmapped 4K
//						memory blocks.
// @param aSetPagesFree	If true, pages a placed in the free state and only mapped pages are added
//						to aPageList.
// @param aNumPtes		On return, indicates how many PTEs are unmapped.
// @param aNumFree		On return, holds the number are freed 4K memory blocks. Not updated if aSetPagesFree is false.
// @return 				The number of PTEs still mapped in this Page Table (aId).
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::UnmapPages() id=%d addr=%08x n=%d pl=%08x set-free=%d",aId,aAddr,aNumPages,aPageList,aSetPagesFree));
	TInt ptOffset=(aAddr&KChunkMask)>>KPageShift;			// entry number in page table
	TPte* pPte=PageTable(aId)+ptOffset;						// address of first PTE
	TInt np=0;
	TInt nf=0;
	while(aNumPages--)
		{
		TPte pte=*pPte;							// get original PTE
		*pPte++=0;								// clear PTE
		TUint pageType = (pte & KPteTypeMask);
		if (pageType == KArmPteSmallPage)
			InvalidateTLBForPage(aAddr);		// flush any corresponding TLB entry
		if (pageType == KArmPteSmallPage || (pageType == 0 && pte != KPteNotPresentEntry))
			{
			++np;								// count unmapped pages
			TPhysAddr pa=pte & KPteSmallPageAddrMask;	// physical address of unmapped page
			if (aSetPagesFree)
				{
				SPageInfo* pi = SPageInfo::FromPhysAddr(pa);
				__NK_ASSERT_DEBUG(pageType == KArmPteSmallPage ||
								  (pi->Type()==SPageInfo::EPagedCode && pi->State()==SPageInfo::EStatePagedOld));
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
	__DRAIN_WRITE_BUFFER;
	__KTRACE_OPT(KMMU,Kern::Printf("Unmapped %d; Freed: %d; Return %08x",np,nf,r));
	return r;								// return number of pages remaining in this page table
	}
#endif

TInt ArmMmu::UnmapVirtual(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TBool aSetPagesFree, TInt& aNumPtes, TInt& aNumFree, DProcess* aProcess)
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
	UnmapPages(aId, aAddr, aNumPages, aPageList, aSetPagesFree, aNumPtes, aNumFree, aProcess);
	ptinfo.iCount = newCount;
	aNumPtes = aNumPages;
	return newCount;
	}
   

#ifndef __MMU_MACHINE_CODED__
void ArmMmu::DoAssignPageTable(TInt aId, TLinAddr aAddr, TPde aPdePerm)
//
// Assign an allocated page table to map a given linear address with specified permissions.
// This should be called with the system locked and the MMU mutex held.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::DoAssignPageTable %d to %08x perm %08x",aId,aAddr,aPdePerm));
	TLinAddr ptLin=PageTableLinAddr(aId);
	TPhysAddr ptPhys=LinearToPhysical(ptLin);
	TInt pdeIndex=TInt(aAddr>>KChunkShift);
	PageDirectory[pdeIndex]=ptPhys|aPdePerm;
	__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x", ptPhys|aPdePerm, PageDirectory+pdeIndex));
	__DRAIN_WRITE_BUFFER;
	}

void ArmMmu::RemapPageTable(TPhysAddr aOld, TPhysAddr aNew, TLinAddr aAddr)
//
// Replace a page table mapping the specified linear address.
// This should be called with the system locked and the MMU mutex held.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::RemapPageTable %08x to %08x at %08x",aOld,aNew,aAddr));
	TInt pdeIndex=TInt(aAddr>>KChunkShift);
	TPde pde=PageDirectory[pdeIndex];
	__ASSERT_ALWAYS((pde & KPdePageTableAddrMask) == aOld, Panic(ERemapPageTableFailed));
	TPde newPde=aNew|(pde&~KPdePageTableAddrMask);
	PageDirectory[pdeIndex]=newPde;
	__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x", newPde, PageDirectory+pdeIndex));
	__DRAIN_WRITE_BUFFER;
	}

void ArmMmu::DoUnassignPageTable(TLinAddr aAddr)
//
// Unassign a now-empty page table currently mapping the specified linear address.
// We assume that TLB and/or cache flushing has been done when any RAM pages were unmapped.
// This should be called with the system locked and the MMU mutex held.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::DoUnassignPageTable at %08x",aAddr));
	TInt pdeIndex=TInt(aAddr>>KChunkShift);
	PageDirectory[pdeIndex]=0;
	__KTRACE_OPT(KMMU,Kern::Printf("Clearing PDE at %08x", PageDirectory+pdeIndex));
	__DRAIN_WRITE_BUFFER;
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
	*iTempPte = pa | SP_PTE(KArmV45PermRWNO, KMemAttNC);
	__DRAIN_WRITE_BUFFER;

	// clear XPT
	TPte* xpt=(TPte*)(iTempAddr+(aXptPhys&KPageMask));
	memclr(xpt, KPageTableSize);

	// must in fact have aXptPhys and aPhysAddr in same physical page
	__ASSERT_ALWAYS( TUint32(aXptPhys^aPhysAddr)<TUint32(KPageSize), MM::Panic(MM::EBootstrapPageTableBadAddr));

	// so only need one mapping
	xpt[(aXptId>>KPtClusterShift)&KPagesInPDEMask] = pa | KPtPtePerm;

	// remove temporary mapping
	*iTempPte=0;
	__DRAIN_WRITE_BUFFER;
	InvalidateTLBForPage(iTempAddr);

	// initialise PtInfo...
	TLinAddr xptAddr = PageTableLinAddr(aXptId);
	iPtInfo[aXptId].SetGlobal(xptAddr>>KChunkShift);

	// map xpt...
	TInt pdeIndex=TInt(xptAddr>>KChunkShift);
	NKern::LockSystem();
	PageDirectory[pdeIndex]=aXptPhys|KPtPdePerm;
	__DRAIN_WRITE_BUFFER;
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

	// invalidate the TLB entry for the self-mapping page table
	// the PDE has not yet been changed, but since we hold the
	// system lock, nothing should bring this back into the TLB.
	InvalidateTLBForPage(PageTableLinAddr(aId));
	}

// Set up a page table (specified by aId) to map a 1Mb section of ROM containing aRomAddr
// using ROM at aOrigPhys.
void ArmMmu::InitShadowPageTable(TInt aId, TLinAddr aRomAddr, TPhysAddr aOrigPhys)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("ArmMmu:InitShadowPageTable id=%04x aRomAddr=%08x aOrigPhys=%08x",
		aId, aRomAddr, aOrigPhys));
	TPte* ppte = PageTable(aId);
	TPte* ppte_End = ppte + KChunkSize/KPageSize;
	TPhysAddr phys = aOrigPhys - (aRomAddr & KChunkMask);
	for (; ppte<ppte_End; ++ppte, phys+=KPageSize)
		*ppte = phys | KRomPtePermissions;
	__DRAIN_WRITE_BUFFER;
	}

// Copy the contents of ROM at aRomAddr to a shadow page at physical address aShadowPhys
void ArmMmu::InitShadowPage(TPhysAddr aShadowPhys, TLinAddr aRomAddr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("ArmMmu:InitShadowPage aShadowPhys=%08x aRomAddr=%08x",
		aShadowPhys, aRomAddr));

	// put in a temporary mapping for aShadowPhys
	// make it noncacheable
	*iTempPte = aShadowPhys | SP_PTE(KArmV45PermRWNO, KMemAttNC);
	__DRAIN_WRITE_BUFFER;

	// copy contents of ROM
	wordmove( (TAny*)iTempAddr, (const TAny*)aRomAddr, KPageSize );
	__DRAIN_WRITE_BUFFER;	// make sure contents are written to memory

	// remove temporary mapping
	*iTempPte=0;
	__DRAIN_WRITE_BUFFER;
	InvalidateTLBForPage(iTempAddr);
	}

// Assign a shadow page table to replace a ROM section mapping
// Enter and return with system locked
void ArmMmu::AssignShadowPageTable(TInt aId, TLinAddr aRomAddr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("ArmMmu:AssignShadowPageTable aId=%04x aRomAddr=%08x",
		aId, aRomAddr));
	TLinAddr ptLin=PageTableLinAddr(aId);
	TPhysAddr ptPhys=LinearToPhysical(ptLin);
	TPde* ppde = PageDirectory + (aRomAddr>>KChunkShift);
	TPde newpde = ptPhys | KShadowPdePerm;
	__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x", newpde, ppde));
	TInt irq=NKern::DisableAllInterrupts();
	*ppde = newpde;		// map in the page table
	__DRAIN_WRITE_BUFFER;	// make sure new PDE written to main memory
	FlushTLBs();	// flush both TLBs (no need to flush cache yet)
	NKern::RestoreInterrupts(irq);
	}

void ArmMmu::DoUnmapShadowPage(TInt aId, TLinAddr aRomAddr, TPhysAddr aOrigPhys)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu:DoUnmapShadowPage, id=%04x lin=%08x origphys=%08x", aId, aRomAddr, aOrigPhys));
	TPte* ppte = PageTable(aId) + ((aRomAddr & KChunkMask)>>KPageShift);
	TPte newpte = aOrigPhys | KRomPtePermissions;
	__KTRACE_OPT(KMMU,Kern::Printf("Writing PTE %08x to %08x", newpte, ppte));
	TInt irq=NKern::DisableAllInterrupts();
	*ppte = newpte;
	__DRAIN_WRITE_BUFFER;
	InvalidateTLBForPage(aRomAddr);
	SyncCodeMappings();
	CacheMaintenance::CodeChanged(aRomAddr, KPageSize, CacheMaintenance::EMemoryRemap);
	CacheMaintenance::PageToReuse(aRomAddr, EMemAttNormalCached, KPhysAddrInvalid);
	NKern::RestoreInterrupts(irq);
	}

TInt ArmMmu::UnassignShadowPageTable(TLinAddr aRomAddr, TPhysAddr aOrigPhys)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu:UnassignShadowPageTable, lin=%08x origphys=%08x", aRomAddr, aOrigPhys));
	TPde* ppde = PageDirectory + (aRomAddr>>KChunkShift);
	TPde newpde = (aOrigPhys &~ KChunkMask) | KRomSectionPermissions;
	__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x", newpde, ppde));
	TInt irq=NKern::DisableAllInterrupts();
	*ppde = newpde;			// revert to section mapping
	__DRAIN_WRITE_BUFFER;	// make sure new PDE written to main memory
	FlushTLBs();			// flush both TLBs
	NKern::RestoreInterrupts(irq);
	return KErrNone;
	}

void ArmMmu::DoFreezeShadowPage(TInt aId, TLinAddr aRomAddr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("ArmMmu:DoFreezeShadowPage aId=%04x aRomAddr=%08x",
		aId, aRomAddr));
	TPte* ppte = PageTable(aId) + ((aRomAddr & KChunkMask)>>KPageShift);
	TPte newpte = (*ppte & KPteSmallPageAddrMask) | KRomPtePermissions;
	__KTRACE_OPT(KMMU,Kern::Printf("Writing PTE %08x to %08x", newpte, ppte));
	*ppte = newpte;
	__DRAIN_WRITE_BUFFER;
	InvalidateTLBForPage(aRomAddr);
	}

void ArmMmu::Pagify(TInt aId, TLinAddr aLinAddr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("ArmMmu:Pagify aId=%04x aLinAddr=%08x", aId, aLinAddr));
	
	TInt pteIndex = (aLinAddr & KChunkMask)>>KPageShift;
	TPte* pte = PageTable(aId);
	if ((pte[pteIndex] & KPteTypeMask) == KArmV45PteLargePage)
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
		FlushTLBs();
		}
	}

void ArmMmu::FlushShadow(TLinAddr aRomAddr)
	{
	CacheMaintenance::CodeChanged(aRomAddr, KPageSize, CacheMaintenance::EMemoryRemap);
	CacheMaintenance::PageToReuse(aRomAddr, EMemAttNormalCached, KPhysAddrInvalid);
	InvalidateTLBForPage(aRomAddr);		// remove all TLB references to original ROM page
	SyncCodeMappings();
	}


inline void ZeroPdes(TLinAddr aBase, TLinAddr aEnd)
	{
	memclr(PageDirectory+(aBase>>KChunkShift), ((aEnd-aBase)>>KChunkShift)*sizeof(TPde));
	}

void ArmMmu::ClearPageTable(TInt aId, TInt aFirstIndex)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::ClearPageTable(%d,%d)",aId,aFirstIndex));
	TPte* pte=PageTable(aId);
	memclr(pte+aFirstIndex, KPageTableSize-aFirstIndex*sizeof(TPte));
	__DRAIN_WRITE_BUFFER;
	}

void ArmMmu::ClearRamDrive(TLinAddr aStart)
	{
	// clear the page directory entries corresponding to the RAM drive
	ZeroPdes(aStart, KRamDriveEndAddress);
	__DRAIN_WRITE_BUFFER;
	}

void ArmMmu::ApplyTopLevelPermissions(TLinAddr aAddr, TUint aChunkSize, TPde aPdePerm)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ApplyTopLevelPermissions at %x",aAddr));
	TInt pdeIndex=aAddr>>KChunkShift;
	TInt numPdes=(aChunkSize+KChunkMask)>>KChunkShift;
	TPde* pPde=PageDirectory+pdeIndex;
	while(numPdes--)
		{
		*pPde=(*pPde)?((*pPde & KPdePageTableAddrMask)|aPdePerm):0;
		pPde++;
		}
	__DRAIN_WRITE_BUFFER;
	}

void ArmMmu::ApplyPagePermissions(TInt aId, TInt aPageOffset, TInt aNumPages, TPte aPtePerm)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::ApplyPagePermissions %04x:%03x+%03x perm %08x",
												aId, aPageOffset, aNumPages, aPtePerm));
	TPte* pPte=PageTable(aId)+aPageOffset;
	TPde* pPteEnd=pPte+aNumPages;
	NKern::LockSystem();
	for (; pPte<pPteEnd; ++pPte)
		{
		TPte pte=*pPte;
		if (pte)
			*pPte = (pte&KPteSmallPageAddrMask)|aPtePerm;
		}
	NKern::UnlockSystem();
	FlushTLBs();
	__DRAIN_WRITE_BUFFER;
	}

void ArmMmu::MoveChunk(TLinAddr aInitAddr, TUint aSize, TLinAddr aFinalAddr, TPde aPdePerm)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MoveChunk at %08x to %08x size %08x PdePerm %08x",
		aInitAddr, aFinalAddr, aSize, aPdePerm));
	TInt numPdes=(aSize+KChunkMask)>>KChunkShift;
	TInt iS=aInitAddr>>KChunkShift;
	TInt iD=aFinalAddr>>KChunkShift;
	TPde* pS=PageDirectory+iS;
	TPde* pD=PageDirectory+iD;
	while(numPdes--)
		{
		*pD++=(*pS)?((*pS & KPdePageTableAddrMask)|aPdePerm):0;
		*pS++=KPdeNotPresentEntry;
		}
	__DRAIN_WRITE_BUFFER;
	}

void ArmMmu::MoveChunk(TLinAddr aInitAddr, TLinAddr aFinalAddr, TInt aNumPdes)
//
// Move a block of PDEs without changing permissions. Must work with overlapping initial and final
// regions. Call this with kernel locked.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MoveChunk at %08x to %08x numPdes %d", aInitAddr, aFinalAddr, aNumPdes));
	if (aInitAddr==aFinalAddr || aNumPdes==0)
		return;
	TInt iS=aInitAddr>>KChunkShift;
	TInt iD=aFinalAddr>>KChunkShift;
	TBool forwardOverlap=(iS<iD && iD-iS<aNumPdes);
	TBool backwardOverlap=(iS>iD && iS-iD<aNumPdes);
	TInt iC=backwardOverlap?(iD+aNumPdes):iS;	// first index to clear
	TInt iZ=forwardOverlap?iD:(iS+aNumPdes);	// last index to clear + 1
	TPde* pS=PageDirectory+iS;
	TPde* pD=PageDirectory+iD;
	__KTRACE_OPT(KMMU,Kern::Printf("backwardOverlap=%d, forwardOverlap=%d",backwardOverlap,forwardOverlap));
	__KTRACE_OPT(KMMU,Kern::Printf("first clear %03x, last clear %03x",iC,iZ));
	wordmove(pD,pS,aNumPdes<<2);				// move PDEs
	pD=PageDirectory+iC;						// pointer to first PDE to clear
	iZ-=iC;										// number of PDEs to clear
	memclr(pD, iZ<<2);							// clear PDEs
	__DRAIN_WRITE_BUFFER;
	}

TPde ArmMmu::PdePermissions(TChunkType aChunkType, TInt aChunkState)
	{
	if ((aChunkType==EUserData || aChunkType==EDllData || aChunkType==EUserSelfModCode
		|| aChunkType==ESharedKernelSingle || aChunkType==ESharedKernelMultiple || aChunkType==ESharedIo)
		&& aChunkState!=0)
		return KUserDataRunningPermissions;
	return ChunkPdePermissions[aChunkType];
	}

TPte ArmMmu::PtePermissions(TChunkType aChunkType)
	{
	return ChunkPtePermissions[aChunkType];
	}

const TUint FBLK=(EMapAttrFullyBlocking>>12);
const TUint BUFC=(EMapAttrBufferedC>>12);
const TUint WTRA=(EMapAttrCachedWTRA>>12);
const TUint WBRA=(EMapAttrCachedWBRA>>12);

#if defined(__CPU_XSCALE__) || defined(__CPU_SA1__)
const TUint AWBR=(EMapAttrAltCacheWBRA>>12);
#endif

const TUint16 UNS=0xffffu;	// Unsupported attribute


#if defined(__CPU_ARM710T__) || defined(__CPU_ARM720T__)
// Original definition of C B
static const TUint16 CacheBuffAttributes[16]=
	{0x00,0x00,0x04,0x04,0x0C,0x0C,0x0C,0x0C, UNS, UNS, UNS, UNS, UNS, UNS, UNS,0x0C};
static const TUint8 CacheBuffActual[16]=
	{FBLK,FBLK,BUFC,BUFC,WTRA,WTRA,WTRA,WTRA,FBLK,FBLK,FBLK,FBLK,FBLK,FBLK,FBLK,WTRA};

#elif defined(__CPU_ARM920T__) || defined(__CPU_ARM925T__) || defined(__CPU_ARM926J__)
// Newer definition of C B
static const TUint16 CacheBuffAttributes[16]=
	{0x00,0x00,0x04,0x04,0x08,0x08,0x0C,0x0C, UNS, UNS, UNS, UNS, UNS, UNS, UNS,0x0C};
static const TUint8 CacheBuffActual[16]=
	{FBLK,FBLK,BUFC,BUFC,WTRA,WTRA,WBRA,WBRA,FBLK,FBLK,FBLK,FBLK,FBLK,FBLK,FBLK,WBRA};

#elif defined(__CPU_SA1__)
// Special definition of C B
static const TUint16 CacheBuffAttributes[16]=
	{0x00,0x00,0x04,0x04,0x04,0x04,0x0C,0x0C,0x04,0x04,0x08,0x08, UNS, UNS, UNS,0x0C};
static const TUint8 CacheBuffActual[16]=
	{FBLK,FBLK,BUFC,BUFC,BUFC,BUFC,WBRA,WBRA,FBLK,FBLK,AWBR,AWBR,FBLK,FBLK,FBLK,WBRA};

#elif defined(__CPU_XSCALE__)
const TUint WBWA=(EMapAttrCachedWBWA>>12);
const TUint16 SPE=0xfffeu;	// Special processing required

#ifdef __CPU_XSCALE_MANZANO__
const TUint L1UN=(EMapAttrL1Uncached>>12);
const TUint BFNC=(EMapAttrBufferedNC>>12);

#ifdef __HAS_EXTERNAL_CACHE__
// ***MANZANO with L2 cache****** //
const TUint L2UN=(EMapAttrL2Uncached>>12);
const TUint MAXC=(EMapAttrL1CachedMax>>12);

//Specifies TEX::CB bits for different L1/L2 cache attributes
//  ...876543201
//  ...TEX..CB..
static const TUint16 CacheBuffAttributes[80]=
	{									// L1CACHE:
//  FBLK  BFNC  BUFC   L1UN   WTRA   WTWA   WBRA   WBWA  AWTR AWTW AWBR AWBT UNS UNS UNS MAX     L2CACHE:
	0x00, 0x44, 0x40,  0x40, 0x108, 0x108, 0x10c, 0x10c, SPE, SPE, SPE, SPE, UNS,UNS,UNS,0x10c,  //NC
	0x00, 0x44, 0x40,  0x40, 0x108, 0x108, 0x10c, 0x10c, SPE, SPE, SPE, SPE, UNS,UNS,UNS,0x10c,  //WTRA
	0x00, 0x44, 0x40,  0x40, 0x108, 0x108, 0x10c, 0x10c, SPE, SPE, SPE, SPE, UNS,UNS,UNS,0x10c,  //WTWA
	0x00, 0x44, 0x40, 0x140, 0x148, 0x148, 0x14c, 0x14c, SPE, SPE, SPE, SPE, UNS,UNS,UNS,0x14c,  //WBRA
	0x00, 0x44, 0x40, 0x140, 0x148, 0x148, 0x14c, 0x14c, SPE, SPE, SPE, SPE, UNS,UNS,UNS,0x14c,  //WBWA
   	};

extern TUint MiniCacheConfig();
//Converts page table attributes(TEX:CB) into appropriate cache attributes.
TInt CacheAttributesActual(TUint& cacheL1, TUint& cacheL2, TUint cbatt)
	{
	switch (cbatt)
		{
		case 0: 	cacheL1 = FBLK; cacheL2 = L2UN; return KErrNone;
		case 0x40: 	cacheL1 = L1UN; cacheL2 = L2UN; return KErrNone;
		case 0x44: 	cacheL1 = BFNC; cacheL2 = L2UN; return KErrNone;
		case 0x48: 	cacheL1 = MiniCacheConfig(); cacheL2 = L2UN; return KErrNone;
		case 0x108: cacheL1 = WTRA; cacheL2 = L2UN; return KErrNone;
		case 0x10c: cacheL1 = WBRA; cacheL2 = L2UN; return KErrNone;
		case 0x140: cacheL1 = L1UN; cacheL2 = WBWA; return KErrNone;
		case 0x148: cacheL1 = WTRA; cacheL2 = WBWA; return KErrNone;
		case 0x14c: cacheL1 = WBRA; cacheL2 = WBWA; return KErrNone;
		}
	return KErrNotSupported;
	}
#else //__HAS_EXTERNAL_CACHE__
// ***MANZANO without L2 cache****** //

static const TUint16 CacheBuffAttributes[16]=
//  FBLK BFNC BUFC L1UN WTRA  WTWA  WBRA   WBWA -----------AltCache--------  MAXC 
   {0x00,0x44,0x40,0x40,0x148,0x148,0x14C,0x14C,SPE,SPE,SPE,SPE,UNS,UNS,UNS,0x14C};
static const TUint8 CacheBuffActual[16]=
	{FBLK,BFNC,BUFC,BUFC,WTRA,WTRA,WBRA,WBRA,FBLK,FBLK,FBLK,FBLK,FBLK,FBLK,FBLK,WBRA};
#endif //__HAS_EXTERNAL_CACHE__

#else 
// ***XSCALE that is not MANZANO (no L2 cache)****** //

// X C B
static const TUint16 CacheBuffAttributes[16]=
	{0x00,0x44,0x04,0x04,0x08,0x08,0x0C,0x4C,SPE,SPE,SPE,SPE,UNS,UNS,UNS,0x4C};
static const TUint8 CacheBuffActual[16]=
	{FBLK,BFNC,BUFC,BUFC,WTRA,WTRA,WBRA,WBWA,FBLK,FBLK,FBLK,FBLK,FBLK,FBLK,FBLK,WBWA};
#endif

// ***Common code for all XSCALE cores*** //

extern TUint MiniCacheConfig();
void ProcessSpecialCacheAttr(TUint& cache, TUint& cbatt)
	{
	// If writeback requested, give writeback or writethrough
	// If writethrough requested, give writethrough or uncached
	// Give other allocation policy if necessary.
	TUint mccfg=MiniCacheConfig();
	__KTRACE_OPT(KMMU,Kern::Printf("MiniCacheConfig: %x",mccfg));

	if (cache<AWBR && mccfg>=AWBR)	// asked for WT but cache is set for WB
		{
		cache=BUFC;					// so give uncached, buffered, coalescing
		#if defined (__CPU_XSCALE_MANZANO__)
		cbatt=0x40;
		#else
		cbatt=0x04;
		#endif
		}
	else
		{
		cache=mccfg;	// give whatever minicache is configured for
		cbatt=0x48;		// minicache attributes
		}
	}
#endif

static const TUint8 ActualReadPrivilegeLevel[4]={4,1,4,4};		// RORO,RWNO,RWRO,RWRW
static const TUint8 ActualWritePrivilegeLevel[4]={0,1,1,4};	// RORO,RWNO,RWRO,RWRW

/** Calculates cb attributes for page table and sets actual cache attributes*/
TInt GetCacheAttr(TUint& cacheL1, TUint& cacheL2, TUint& cbatt)
	{
	TInt r = KErrNone;
	// Scale down L2 to 0-4 : NC, WTRA, WTWA, WBRA, WBWA
#if defined (__CPU_XSCALE_MANZANO__) && defined(__HAS_EXTERNAL_CACHE__)
	if      (cacheL2 == MAXC) cacheL2 = WBWA-3;			//	Scale down L2 cache attributes...
	else if (cacheL2 > WBWA)  return KErrNotSupported;	//	... to 0-4 for...
	else if (cacheL2 < WTRA)  cacheL2 = L2UN;			//	... L2UN to WBWA 
	else					  cacheL2-=3;				//
#else
	cacheL2 = 0; // Either no L2 cache or L2 cache attributes will be just a copy of L1 cache attributes.
#endif

	//Get cb page attributes. (On some platforms, tex bits are includded as well.)
	cbatt = CacheBuffAttributes[cacheL1 + (cacheL2<<4)];
	__KTRACE_OPT(KMMU,Kern::Printf("GetCacheAttr, table returned:%x",cbatt));

#if defined(__CPU_XSCALE__)
	//Check if altDCache/LLR cache attributes are defined
	if (cbatt == SPE)
		{
		cacheL2 = 0; //Not L2 cached in such case
		ProcessSpecialCacheAttr(cacheL1,cbatt);
		__KTRACE_OPT(KMMU,Kern::Printf("GetCacheAttr, spec case returned:%x",cbatt));
		}
#endif

	if(cbatt == UNS)
		return KErrNotSupported;
	
	//W Got CB page attributes. Now, find out what are the actual cache attributes.
#if defined(__CPU_XSCALE_MANZANO__) && defined(__HAS_EXTERNAL_CACHE__)
	r = CacheAttributesActual(cacheL1, cacheL2, cbatt);
#else
	cacheL1 = CacheBuffActual[cacheL1];
#if defined(__HAS_EXTERNAL_CACHE__)
	cacheL2 = cacheL1;
#else
	cacheL2 = 0;
#endif	
#endif
	return r;
	}

TInt ArmMmu::PdePtePermissions(TUint& aMapAttr, TPde& aPde, TPte& aPte)
	{
	__KTRACE_OPT(KMMU,Kern::Printf(">ArmMmu::PdePtePermissions, mapattr=%08x",aMapAttr));
	TUint read=aMapAttr & EMapAttrReadMask;
	TUint write=(aMapAttr & EMapAttrWriteMask)>>4;
	TUint exec=(aMapAttr & EMapAttrExecMask)>>8;

	// if execute access is greater than read, adjust read (since there are no separate execute permissions on ARM)
	if (exec>read)
		read=exec;
	TUint ap;
	if (write==0)
		{
		// read-only
		if (read>=4)
			ap=KArmV45PermRORO;			// user and supervisor read-only
		else
			ap=KArmV45PermRWNO;			// supervisor r/w user no access (since no RO/NO access is available)
		}
	else if (write<4)
		{
		// only supervisor can write
		if (read>=4)
			ap=KArmV45PermRWRO;			// supervisor r/w user r/o
		else
			ap=KArmV45PermRWNO;			// supervisor r/w user no access
		}
	else
		ap=KArmV45PermRWRW;				// supervisor r/w user r/w
	read=ActualReadPrivilegeLevel[ap];
	write=ActualWritePrivilegeLevel[ap];
#ifndef __CPU_USE_MMU_TEX_FIELD
	ap|=(ap<<2);
	ap|=(ap<<4);						// replicate permissions in all four subpages
#endif
	ap<<=4;								// shift access permissions into correct position for PTE
	ap|=KArmPteSmallPage;				// add in mandatory small page bits

	// Get cb atributes for the page table and the actual cache attributes
	TUint cbatt;
	TUint cacheL1=(aMapAttr & EMapAttrL1CacheMask)>>12;
	TUint cacheL2=(aMapAttr & EMapAttrL2CacheMask)>>16;
	TInt r = GetCacheAttr(cacheL1, cacheL2, cbatt);

	if (r==KErrNone)
		{
		aPde=PT_PDE(EDomainClient);
		aPte=ap|cbatt;
		aMapAttr=read|(write<<4)|(read<<8)|(cacheL1<<12)|(cacheL2<<16);
		}
	__KTRACE_OPT(KMMU,Kern::Printf("<ArmMmu::PdePtePermissions, r=%d, mapattr=%08x, pde=%08x, pte=%08x",
								r,aMapAttr,aPde,aPte));
	return r;
	}

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
			// use sections
			TInt npdes = remain>>KChunkShift;
			TPde* p_pde = PageDirectory + (la>>KChunkShift);
			TPde* p_pde_E = p_pde + npdes;
			TPde pde = pa|section_pde;
			NKern::LockSystem();
			for (; p_pde < p_pde_E; pde+=KChunkSize)
				{
				__ASSERT_DEBUG(*p_pde==0, MM::Panic(MM::EPdeAlreadyInUse));
				__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x", pde, p_pde));
				*p_pde++=pde;
				}
			NKern::UnlockSystem();
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
		TInt id = PageTableId(la);
		__ASSERT_DEBUG(id>=0, MM::Panic(MM::EMmuMapNoPageTable));
		TPte* p_pte = PageTable(id) + ((la&KChunkMask)>>KPageShift);
		TPte* p_pte_E = p_pte + (block_size>>KPageShift);
		SPageTableInfo& ptinfo = iPtInfo[id];
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
		TPde pde = PageDirectory[pdeIndex];
		if ( (pde&KPdePresentMask)==KArmV45PdeSection )
			{
			__ASSERT_DEBUG(!(a&KChunkMask), MM::Panic(MM::EUnmapBadAlignment));
			PageDirectory[pdeIndex]=0;
			InvalidateTLBForPage(a);
			a=next;
			NKern::FlashSystem();
			continue;
			}
		TInt ptid = GetPageTableId(a);
		SPageTableInfo& ptinfo=iPtInfo[ptid];
		if (ptid>=0)
			{
			TPte* ppte = PageTable(ptid) + ((a&KChunkMask)>>KPageShift);
			TPte* ppte_End = ppte + to_do;
			for (; ppte<ppte_End; ++ppte, a+=KPageSize)
				{
				TUint pte_type = *ppte & KPteTypeMask;
				if (pte_type && pte_type != KArmV45PteLargePage)
					{
					--ptinfo.iCount;
					*ppte=0;
					InvalidateTLBForPage(a);
					}
				else if (pte_type)
					{
					__ASSERT_DEBUG(!(a&KLargePageMask), MM::Panic(MM::EUnmapBadAlignment));
					ptinfo.iCount-=KLargeSmallPageRatio;
					memclr(ppte, KLargeSmallPageRatio*sizeof(TPte));
					InvalidateTLBForPage(a);
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
	}

TInt ArmMmu::AllocDomain()
	{
	NKern::FMWait(&DomainLock);
	TInt r=-1;
	if (Domains)
		{
		r=__e32_find_ls1_32(Domains);
		Domains &= ~(1<<r);
		}
	NKern::FMSignal(&DomainLock);
	return r;
	}

void ArmMmu::FreeDomain(TInt aDomain)
	{
	__ASSERT_ALWAYS(aDomain>=0 && aDomain<ENumDomains, MM::Panic(MM::EFreeInvalidDomain));
	TUint32 m=1<<aDomain;
	NKern::FMWait(&DomainLock);
	__ASSERT_ALWAYS(!(Domains&m), MM::Panic(MM::EFreeDomainNotAllocated));
	Domains|=m;
	NKern::FMSignal(&DomainLock);
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
		*iTempPte = pa | SP_PTE(KArmV45PermRWNO, KMemAttBuf);
		__DRAIN_WRITE_BUFFER;
		InvalidateTLBForPage(iTempAddr);
		memset((TAny*)iTempAddr, aClearByte, iPageSize);
		}
	*iTempPte=0;
	__DRAIN_WRITE_BUFFER;
	InvalidateTLBForPage(iTempAddr);
	}

TLinAddr DoMapTemp(TPhysAddr aPage, TBool aCached, TLinAddr aTempAddr, TPte* aTempPte)
	{
	__ASSERT_DEBUG(!*aTempPte,MM::Panic(MM::ETempMappingAlreadyInUse));
	*aTempPte = (aPage&~KPageMask) | SP_PTE(KArmV45PermRWNO, aCached?KDefaultCaching:KMemAttBuf);
	__DRAIN_WRITE_BUFFER;
	return aTempAddr;
	}

/**
Create a temporary mapping of a physical page.
The RamAllocatorMutex must be held before this function is called and not released
until after UnmapTemp has been called.

@param aPage	The physical address of the page to be mapped.
@param aCached	Whether to map the page cached or not.

@return The linear address of where the page has been mapped.
*/
TLinAddr ArmMmu::MapTemp(TPhysAddr aPage, TBool aCached)
	{
	__ASSERT_MUTEX(RamAllocatorMutex);
	return DoMapTemp(aPage, aCached, iTempAddr, iTempPte);
	}

/**
Create a temporary mapping of a physical page, distinct from that created by MapTemp.
The RamAllocatorMutex must be held before this function is called and not released
until after UnmapSecondTemp has been called.

@param aPage	The physical address of the page to be mapped.
@param aCached	Whether to map the page cached or not.

@return The linear address of where the page has been mapped.
*/
TLinAddr ArmMmu::MapSecondTemp(TPhysAddr aPage, TBool aCached)
	{
	__ASSERT_MUTEX(RamAllocatorMutex);
	return DoMapTemp(aPage, aCached, iSecondTempAddr, iSecondTempPte);
	}

void DoUnmapTemp(TLinAddr aTempAddr, TPte* aTempPte)
	{
	*aTempPte = 0;
	__DRAIN_WRITE_BUFFER;
	InvalidateTLBForPage(aTempAddr);
	}

/**
Remove the temporary mapping created with MapTemp.
*/
void ArmMmu::UnmapTemp()
	{
	__ASSERT_MUTEX(RamAllocatorMutex);
	DoUnmapTemp(iTempAddr, iTempPte);
	}

/**
Remove the temporary mapping created with MapSecondTemp.
*/
void ArmMmu::UnmapSecondTemp()
	{
	__ASSERT_MUTEX(RamAllocatorMutex);
	DoUnmapTemp(iSecondTempAddr, iSecondTempPte);
	}

/*
 * Performs cache maintenance on physical cache (VIPT & PIPT) for a page to be reused.
 */
void ArmMmu::CacheMaintenanceOnDecommit(TPhysAddr aAddr)
	{
	CacheMaintenance::PageToReusePhysicalCache(aAddr);
	}

void ArmMmu::CacheMaintenanceOnDecommit(const TPhysAddr* aAddr, TInt aCount)
	{
	while (--aCount>=0)
		ArmMmu::CacheMaintenanceOnDecommit(*aAddr++);
	}

void ArmMmu::CacheMaintenanceOnPreserve(TPhysAddr, TUint)
	{
	//Not required for moving memory model
	__ASSERT_ALWAYS(0, Panic(ECacheMaintenance));
	}

void ArmMmu::CacheMaintenanceOnPreserve(const TPhysAddr*, TInt, TUint)
	{
	//Not required for moving memory model
	__ASSERT_ALWAYS(0, Panic(ECacheMaintenance));
	}

void ArmMmu::CacheMaintenanceOnPreserve(TPhysAddr , TInt , TLinAddr , TUint )
	{
	//Not required for moving memory model
	__ASSERT_ALWAYS(0, Panic(ECacheMaintenance));
	}


TInt ArmMmu::UnlockRamCachePages(TUint8* volatile & aBase, TInt aStartPage, TInt aNumPages)
	{
	NKern::LockSystem();
	for(;;)
		{
		TInt page = ((TLinAddr)aBase>>KPageShift)+aStartPage;
		TPde* pd = PageDirectory+(page>>(KChunkShift-KPageShift));
		TPte* pt = SafePageTableFromPde(*pd++);
		TInt pteIndex = page&(KChunkMask>>KPageShift);
		if(!pt)
			{
			// whole page table has gone, so skip all pages in it...
			TInt pagesInPt = (KChunkSize>>KPageShift)-pteIndex;
			aNumPages -= pagesInPt;
			aStartPage += pagesInPt;
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
			aStartPage += pagesInPt;

			do
				{
				TPte pte = *pt++;
				if(pte!=KPteNotPresentEntry) // pte may be null if page has already been unlocked and reclaimed by system
					iRamCache->DonateRamCachePage(SPageInfo::FromPhysAddr(pte));
				}
			while(--pagesInPt);

			if(!aNumPages)
				{
				NKern::UnlockSystem();
				return KErrNone;
				}

			pteIndex = aStartPage&(KChunkMask>>KPageShift);
			}
		while(!NKern::FlashSystem() && pteIndex);
		}
	}


TInt ArmMmu::LockRamCachePages(TUint8* volatile & aBase, TInt aStartPage, TInt aNumPages)
	{
	NKern::LockSystem();
	for(;;)
		{
		TInt page = ((TLinAddr)aBase>>KPageShift)+aStartPage;
		TPde* pd = PageDirectory+(page>>(KChunkShift-KPageShift));
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
			aStartPage += pagesInPt;

			do
				{
				TPte pte = *pt++;
				if(pte==KPteNotPresentEntry)
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

			pteIndex = aStartPage&(KChunkMask>>KPageShift);
			}
		while(!NKern::FlashSystem() && pteIndex);
		}
not_found:
	NKern::UnlockSystem();
	return KErrNotFound;
	}


void RamCache::SetFree(SPageInfo* aPageInfo)
	{
	// Make a page free
	SPageInfo::TType type = aPageInfo->Type();
	if(type==SPageInfo::EPagedCache)
		{
		TInt offset = aPageInfo->Offset()<<KPageShift;
		DArmPlatChunk* chunk = (DArmPlatChunk*)aPageInfo->Owner();
		__NK_ASSERT_DEBUG(TUint(offset)<TUint(chunk->iMaxSize));
		TLinAddr lin = ((TLinAddr)chunk->iBase)+offset;
		TPte* pt = PtePtrFromLinAddr(lin);
		*pt = KPteNotPresentEntry;
		__DRAIN_WRITE_BUFFER;
		InvalidateTLBForPage(lin);
		((ArmMmu*)iMmu)->SyncCodeMappings();
		CacheMaintenance::PageToReuseVirtualCache(lin);
		// actually decommit it from chunk...
		TInt ptid = ((TLinAddr)pt-KPageTableBase)>>KPageTableShift;
		SPageTableInfo& ptinfo=((ArmMmu*)iMmu)->iPtInfo[ptid];
		if(!--ptinfo.iCount)
			{
			((ArmMmu*)iMmu)->DoUnassignPageTable(lin);
			chunk->RemovePde(offset);
			NKern::UnlockSystem();
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
	TInt HandleFault(TArmExcInfo& aExc, TLinAddr aFaultAddress, TBool aInRom);
	TInt PageIn(TLinAddr aAddress, DMemModelCodeSegMemory* aCodeSegMemory);
private:
	TLinAddr GetLinearAddress(SPageInfo* aPageInfo);
	};


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
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("<MemModelDemandPaging::Init2"));
	}


void MemModelDemandPaging::AllocLoadAddress(DPagingRequest& aReq, TInt aReqId)
	{
	aReq.iLoadAddr = iTempPages + aReqId * KPageSize;
	aReq.iLoadPte = PtePtrFromLinAddr(aReq.iLoadAddr);
	}


TInt MemModelDemandPaging::Init3()
	{
	TInt r=DemandPaging::Init3();
	if(r!=KErrNone)
		return r;

	// Create a region for mapping pages during page in
	DPlatChunkHw* chunk;
	TInt chunkSize = KMaxPagingDevices * KPagingRequestsPerDevice * KPageSize;
	DPlatChunkHw::DoNew(chunk, KPhysAddrInvalid, chunkSize, EMapAttrSupRw|EMapAttrFullyBlocking);
	if(!chunk)
		Panic(EInitialiseFailed);
	iTempPages = chunk->iLinAddr;

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
		TInt ptid = Mmu().PageTableId(lin);
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
		TPhysAddr ptPhys=Mmu().LinearToPhysical((TLinAddr)pt);

		// Pointer to page dirctory entry
		TPde* ppde = PageDirectory + (lin>>KChunkShift);

		// Fill in Page Table
		TPte* ptEnd = pt+(1<<(KChunkShift-KPageShift));
		pt += (lin&KChunkMask)>>KPageShift;
		do
			{
			if(lin<iRomPagedLinearBase)
				*pt++ = Mmu().LinearToPhysical(lin) | KRomPtePermissions;
			else
				*pt++ = KPteNotPresentEntry;
			lin += KPageSize;
			}
		while(pt<ptEnd && lin<=linEnd);
		__DRAIN_WRITE_BUFFER;

		// Add new Page Table to the Page Directory
		TPde newpde = ptPhys | KShadowPdePerm;
		__KTRACE_OPT2(KPAGING,KMMU,Kern::Printf("Writing PDE %08x to %08x", newpde, ppde));
		TInt irq=NKern::DisableAllInterrupts();
		*ppde = newpde;
		__DRAIN_WRITE_BUFFER;
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

	if(type!=SPageInfo::EPagedCache && type!=SPageInfo::EPagedCode)
		{
		__NK_ASSERT_DEBUG(type!=SPageInfo::EPagedData); // not supported yet
		return ETrue;
		}

	RemovePage(aPageInfo);
	AddAsFreePage(aPageInfo);
	// Return false to stop DMemModelChunk::DoDecommit from freeing this page
	return EFalse;
	}


TLinAddr MemModelDemandPaging::GetLinearAddress(SPageInfo* aPageInfo)
	{
	TInt offset = aPageInfo->Offset()<<KPageShift;
	SPageInfo::TType type = aPageInfo->Type();
	__NK_ASSERT_DEBUG(TUint(offset)<(type==SPageInfo::EPagedROM ? iRomSize : iCodeSize));
	TLinAddr base = type==SPageInfo::EPagedROM ? iRomLinearBase : iCodeLinearBase;
	return base + offset;
	}


void MemModelDemandPaging::SetOld(SPageInfo* aPageInfo)
	{
	__NK_ASSERT_DEBUG(aPageInfo->State() == SPageInfo::EStatePagedOld);
	SPageInfo::TType type = aPageInfo->Type();

	if(type==SPageInfo::EPagedROM || type==SPageInfo::EPagedCode)
		{
		START_PAGING_BENCHMARK;
		
		// get linear address of page...
		TLinAddr lin = GetLinearAddress(aPageInfo);

		// make page inaccessible...
		TPte* pt = PtePtrFromLinAddr(lin);
		*pt &= ~KPtePresentMask;
		__DRAIN_WRITE_BUFFER;
		InvalidateTLBForPage(lin);
		Mmu().SyncCodeMappings();

		if (type==SPageInfo::EPagedCode)
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


void MemModelDemandPaging::SetFree(SPageInfo* aPageInfo)
	{
	__ASSERT_SYSTEM_LOCK;
	__ASSERT_MUTEX(MmuBase::RamAllocatorMutex);
	__NK_ASSERT_DEBUG(aPageInfo->State() == SPageInfo::EStatePagedDead);
	if(aPageInfo->LockCount())
		Panic(ERamPageLocked);

	SPageInfo::TType type = aPageInfo->Type();

	if(type==SPageInfo::EPagedROM || type==SPageInfo::EPagedCode)
		{
		START_PAGING_BENCHMARK;
		
		// get linear address of page...
		TLinAddr lin = GetLinearAddress(aPageInfo);

		// unmap it...
		TPte* pt = PtePtrFromLinAddr(lin);
		*pt = KPteNotPresentEntry;
		__DRAIN_WRITE_BUFFER;
		InvalidateTLBForPage(lin);
		Mmu().SyncCodeMappings();

		if (type==SPageInfo::EPagedCode)
			END_PAGING_BENCHMARK(this, EPagingBmSetCodePageFree);
#ifdef BTRACE_PAGING
		TInt subCat = type==SPageInfo::EPagedCode ? BTrace::EPagingPageOutCode : BTrace::EPagingPageOutROM;
		TPhysAddr phys = aPageInfo->PhysAddr();
		BTraceContext8(BTrace::EPaging,subCat,phys,lin); 
#endif
		}
	else if(type==SPageInfo::EPagedCache)
		{
		// get linear address of page...
		TInt offset = aPageInfo->Offset()<<KPageShift;
		DArmPlatChunk* chunk = (DArmPlatChunk*)aPageInfo->Owner();
		__NK_ASSERT_DEBUG(TUint(offset)<TUint(chunk->iMaxSize));
		TLinAddr lin = ((TLinAddr)chunk->iBase)+offset;

		// unmap it...
		TPte* pt = PtePtrFromLinAddr(lin);
		*pt = KPteNotPresentEntry;
		__DRAIN_WRITE_BUFFER;
		InvalidateTLBForPage(lin);
		Mmu().SyncCodeMappings();
		NKern::UnlockSystem();
		CacheMaintenance::PageToReuseVirtualCache(lin);
		NKern::LockSystem();

		// actually decommit it from chunk...
		TInt ptid = ((TLinAddr)pt-KPageTableBase)>>KPageTableShift;
		SPageTableInfo& ptinfo=((ArmMmu*)iMmu)->iPtInfo[ptid];
		if(!--ptinfo.iCount)
			{
			((ArmMmu*)iMmu)->DoUnassignPageTable(lin);
			chunk->RemovePde(offset);
			NKern::UnlockSystem();
			((ArmMmu*)iMmu)->FreePageTable(ptid);
			NKern::LockSystem();
			}

#ifdef BTRACE_PAGING
		TPhysAddr phys = aPageInfo->PhysAddr();
		BTraceContext8(BTrace::EPaging,BTrace::EPagingPageOutCache,phys,lin);
#endif
		}
	else if(type==SPageInfo::EPagedFree)
		{
		// already free...
#ifdef BTRACE_PAGING
		TPhysAddr phys = aPageInfo->PhysAddr();
		BTraceContext4(BTrace::EPaging,BTrace::EPagingPageOutFree,phys);
#endif
		// external cache may not have been cleaned if PageUnmapped called
		CacheMaintenance::PageToReusePhysicalCache(aPageInfo->PhysAddr());
		}
	else
		{
		__KTRACE_OPT2(KPAGING,KPANIC,Kern::Printf("DP: SetFree() with bad page type = %d",aPageInfo->Type()));
		Panic(EUnexpectedPageType);
		}
	NKern::FlashSystem();
	}


void MemModelDemandPaging::NotifyPageFree(TPhysAddr aPage)
	{
	MM::Panic(MM::EOperationNotImplemented);
	}


/**
Return True if exception was caused by a memory write access.
This function can cause a paging exception!
*/
static TBool FaultDuringWrite(TArmExcInfo& aExc)
	{
	// We can't decode jazelle instruction to determine if they faulted during a read.
	// Therefore we will treat them as writes (which will panic the thread)...
	if(aExc.iCpsr&(1<<24))
		return ETrue; 

	if(aExc.iCpsr&(1<<5))
		{
		// thumb
		TUint32 op = *(TUint16*)aExc.iR15;
		switch((op>>13)&7)
			{
		case 2:
			if((op&0xfa00)==0x5000)
				return ETrue;			// STR (2) and STRB (2)
			if((op&0xfe00)==0x5200)
				return ETrue;			// STRH (2)
			return EFalse;
		case 3:
			return !(op&(1<<11));		// STR (1) and STRB (1)
		case 4:
			return !(op&(1<<11));		// STR (3) and STRH (1)
		case 5:
			return (op&0xfe00)==0xb400;	// PUSH
		case 6:
			return (op&0xf800)==0xc000; // STMIA
			}
		}
	else
		{
		// ARM
		TUint32 op = *(TUint32*)aExc.iR15;
		if(op<0xf0000000)
			{
			switch((op>>25)&7)
				{
			case 0:
				if((op&0xf0)==(0xb0))
					return !(op&(1<<20));		// load/store halfword
				else if((op&0x0e1000f0)==(0x000000f0))
					return ETrue;				// store double
				else if((op&0x0fb000f0) == 0x010000f0)
					return ETrue;				// swap instruction
				else if((op&0x0ff000f0) == 0x01800090)
					return ETrue;				// strex
				return EFalse;
			case 2:
				return !(op&(1<<20));			 // load/store immediate
			case 3:
				if(!(op&0x10))
					return !(op&(1<<20));		// load/store register offset
				return EFalse;
			case 4:
				return !(op&(1<<20));			// load/store multiple
			case 6:
				return !(op&(1<<20));			// coproc store 
				}
			}
		else
			{
			switch((op>>25)&7)
				{
			case 4:
				if((op&0xfe5f0f00)==(0xf84d0500))
					return ETrue;				// SRS instructions
				return EFalse;
			case 6:
				return !(op&(1<<20));			// coproc store (STC2)
				}
			}
		}
	return EFalse;
	}


TInt MemModelDemandPaging::Fault(TAny* aExceptionInfo)
	{
	TArmExcInfo& exc=*(TArmExcInfo*)aExceptionInfo;

	// Get faulting address
	TLinAddr faultAddress = exc.iFaultAddress;
	if(exc.iExcCode==EArmExceptionDataAbort)
		{
		// Only handle page translation faults
		if((exc.iFaultStatus&0xf)!=0x7)
			return KErrUnknown;
		// Let writes take an exception rather than page in any memory...
		if(FaultDuringWrite(exc))
			return KErrUnknown;
		}
	else if (exc.iExcCode != EArmExceptionPrefetchAbort)
		return KErrUnknown; // Not prefetch or data abort

	DThread* thread = TheCurrentThread;

	// check which ragion fault occured in...
	TBool inRom=ETrue;
	if(TUint(faultAddress-iRomPagedLinearBase)<iRomPagedSize)
		{
		// in ROM
		}
	else if(TUint(faultAddress-iCodeLinearBase)<iCodeSize)
		{
		// in code
		inRom=EFalse;
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

		// Current thread already has the system lock...
		NKern::FlashSystem(); // Let someone else have a go with the system lock.
		}

	// System locked here

	TInt r = KErrNone;	
	if(thread->IsRealtime())
		r = CheckRealtimeThreadFault(thread, aExceptionInfo);
	if (r == KErrNone)
		r = HandleFault(exc, faultAddress, inRom);
	
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


TInt MemModelDemandPaging::HandleFault(TArmExcInfo& aExc, TLinAddr aFaultAddress, TBool aInRom)
	{
	++iEventInfo.iPageFaultCount;

	// get page table entry...
	TPte* pt = SafePtePtrFromLinAddr(aFaultAddress);
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
			*pt = KPteNotPresentEntry; // Update page table
			__DRAIN_WRITE_BUFFER;
			}
		else
			{
			// page just needs making young again...
			*pt = TPte(pte|KArmPteSmallPage); // Update page table
			__DRAIN_WRITE_BUFFER;
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
	if (aInRom)
		NKern::ThreadEnterCS();
	else
		{
		// find CodeSeg...
		DMemModelCodeSeg* codeSeg = (DMemModelCodeSeg*)DCodeSeg::CodeSegsByAddress.Find(aFaultAddress);
		if (!codeSeg)
			return KErrNotFound;
		codeSegMemory = codeSeg->Memory();
		if (codeSegMemory==0 || !codeSegMemory->iIsDemandPaged)
			return KErrNotFound;
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
	
	TInt r = PageIn(aFaultAddress,codeSegMemory);

	NKern::UnlockSystem();

	if(codeSegMemory)
		codeSegMemory->Close();

	NKern::ThreadLeaveCS();
	
	return r;
	}


TInt MemModelDemandPaging::PageIn(TLinAddr aAddress, DMemModelCodeSegMemory* aCodeSegMemory)
	{
	// Get a request object - this may block until one is available
	DPagingRequest* req = AcquireRequestObject();
	
	// Get page table entry
	TPte* pt = SafePtePtrFromLinAddr(aAddress);

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
	TLinAddr loadAddr = req->iLoadAddr;
	pt = req->iLoadPte;
	*pt = phys | SP_PTE(KArmV45PermRWNO, KMemAttTempDemandPaging);
	__DRAIN_WRITE_BUFFER;

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

	// make caches consistant (uncached memory is used for page loading)
	__DRAIN_WRITE_BUFFER;
	NKern::LockSystem();

	// Invalidate temporary mapping
	*pt = KPteNotPresentEntry;
	__DRAIN_WRITE_BUFFER;
	InvalidateTLBForPage(loadAddr);

	ReleaseRequestObject(req);
	
	// Get page table entry
	pt = SafePtePtrFromLinAddr(aAddress);

	// Check page still needs updating
	TBool notNeeded = pt==0 || *pt!=KPteNotPresentEntry;
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
		pageInfo->SetPagedCode(aCodeSegMemory,(aAddress-Mmu().iUserCodeBase)>>KPageShift);

	// Map page into final location
	*pt = phys | (aCodeSegMemory ? KUserCodeLoadPte : KRomPtePermissions);
	__DRAIN_WRITE_BUFFER;
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
	XTRAPD(exc,XT_DEFAULT,XTRAP_PAGING_RETRY(CHECK_PAGING_SAFE; ReadByte(aPage);));
	return exc;
	}


TPhysAddr MemModelDemandPaging::LinearToPhysical(TLinAddr aPage, DProcess* aProcess)
	{
	return Mmu().LinearToPhysical(aPage);
	}


TInt MemModelDemandPaging::PageState(TLinAddr aAddr)
	{
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
		if (codeSegMemory)
			{
			r |= EPageStateInRamCode;
			if (codeSegMemory->iIsDemandPaged)
				r |= EPageStatePaged;
			}
		}

	ptePtr = SafePtePtrFromLinAddr(aAddr);
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

done:
	NKern::UnlockSystem();
	return r;
	}


TBool MemModelDemandPaging::NeedsMutexOrderCheck(TLinAddr aStartAddr, TUint aLength)
	{
	// Don't check mutex order for reads from unpaged rom, kernel data area and kernel stack chunk
	TLinAddr endAddr = aStartAddr + aLength;
	TLinAddr stackBase = (TLinAddr)MM::SvStackChunk->Base();
	TLinAddr stackEnd = stackBase + MM::SvStackChunk->iMaxSize;
	TLinAddr unpagedRomEnd = iRomPagedLinearBase ? iRomPagedLinearBase : iRomLinearBase + iRomSize;
	TBool rangeInUnpagedRom = aStartAddr >= iRomLinearBase && endAddr <= unpagedRomEnd;
	TBool rangeInKernelData = aStartAddr >= KKernelDataBase && endAddr <= KKernelDataEnd;
	TBool rangeInKernelStack = aStartAddr >= stackBase && endAddr <= stackEnd;
	return !rangeInUnpagedRom && !rangeInKernelData && !rangeInKernelStack;
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
// Mark the page at aOffset in aChunk inaccessible to prevent it being
// modified while defrag is in progress. Save the required information
// to allow the fault handler to deal with this.
// Flush the cache for the page so that it can be aliased elsewhere for
// copying.
// Call this with the system unlocked.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ArmMmu::DisablePageModification() offset=%08x", aOffset));

	// Acquire the system lock here for atomic access to aChunk->iBase as moving 
	// between the home and run addresses (a reschedule) may update aChunk->iBase.
	NKern::LockSystem();

	iDisabledAddr = (TLinAddr)(aChunk->iBase) + aOffset;
	TInt ptid=GetPageTableId(iDisabledAddr);
	if(ptid<0)
		Panic(EDefragDisablePageFailed);	

	TPte* pPte = PageTable(ptid) + ((aOffset&KChunkMask)>>KPageShift);
	TPte pte = *pPte;
	if ((pte & KPteTypeMask) != KArmPteSmallPage)
		Panic(EDefragDisablePageFailed);

	iDisabledPte = pPte;
	iDisabledOldVal = pte;

	*pPte = 0;
	__DRAIN_WRITE_BUFFER;
	InvalidateTLBForPage(iDisabledAddr);
	NKern::UnlockSystem();

	CacheMaintenance::PageToPreserveAndReuseVirtualCache(iDisabledAddr);
	__DRAIN_WRITE_BUFFER;
	}

TBool FaultStatusFromLinAddr(TLinAddr aAddr, TBool aKernel, TUint32& aFaultStatus)
	// Walk the page tables looking for the given linear address. If access
	// would've caused a fault, return ETrue and fill in aFaultStatus with a
	// FSR value. Otherwise, return EFalse. Assumes it was a read.
	{
	TPde pde = PageDirectory[aAddr>>KChunkShift];
	TPde pdetype = pde & KPdeTypeMask;
	if (pdetype == 0)
		{
		// section translation fault
		aFaultStatus = 0x5;
		return ETrue;
		}

	TPte pte=0;
	TInt domain = (pde >> 5) & 0xf;
	TUint32 dacr = Arm::Dacr();
	TInt domaccess = (dacr >> (domain<<1)) & 0x3;
	TInt ispage = (pdetype == KArmV45PdeSection) ? 0 : 0x2;

	if (ispage)
		{
		pte = *PtePtrFromLinAddr(aAddr);
		if ((pte & KPteTypeMask) == 0)
			{
			// page translation fault
			aFaultStatus = 0x7;
			return ETrue;
			}
		}

	if (domaccess == 0x3)
		{
		// manager access
		return EFalse;
		}
	if (domaccess == 0)
		{
		// domain fault
		aFaultStatus = 0x9 | ispage;
		return ETrue;
		}

	TInt perms;
	if (ispage)
		perms = (pte >> 4) & 0x3;
	else
		perms = (pde >> 10) & 0x3;
	
	if (aKernel || perms != 0x1)
		return EFalse;

	// permission fault
	aFaultStatus = 0xd | ispage;
	return ETrue;
	}

TInt ArmMmu::RamDefragFault(TAny* aExceptionInfo)
	{
	TArmExcInfo& exc=*(TArmExcInfo*)aExceptionInfo;

	// Get faulting address
	TLinAddr faultAddress;
	TBool prefetch=EFalse;
	if(exc.iExcCode==EArmExceptionDataAbort)
		{
		// Only handle page translation faults
		if((exc.iFaultStatus & 0xf) != 0x7)
			return KErrUnknown;
		faultAddress = exc.iFaultAddress;
		}
	else if(exc.iExcCode==EArmExceptionPrefetchAbort)
		{
		prefetch = ETrue;
		faultAddress = exc.iR15;
		}
	else
		return KErrUnknown; // Not data/prefetch abort

	TBool kernelmode = exc.iCpsr&EMaskMode != EUserMode;

	// Take system lock if not already held
	NFastMutex* fm = NKern::HeldFastMutex();
	if(!fm)
		NKern::LockSystem();
	else if(fm!=&TheScheduler.iLock)
		{
		__KTRACE_OPT2(KMMU,KPANIC,Kern::Printf("Defrag: Fault with FM Held! %x (%O pc=%x)",faultAddress,TheCurrentThread,exc.iR15));
		Panic(EDefragFaultWhilstFMHeld); // Not allowed to hold mutexes
		}

	TInt r = KErrUnknown;

	// check if the mapping of the page has already been restored and retry if so
	if (prefetch)
		{
		TUint32 fsr;
		if (!FaultStatusFromLinAddr(faultAddress, kernelmode, fsr))
			{
			r = KErrNone;
			goto leave;
			}
		}
	else
		{
		TPte* pt = SafePtePtrFromLinAddr(faultAddress);
		if(!pt)
			{
			r = KErrNotFound;
			goto leave;
			}
		if ((*pt & 0x3) != 0)
			{
			r = KErrNone;
			goto leave;
			}
		}

	// check if the fault occurred in the page we are moving
	if (iDisabledPte && TUint(faultAddress - iDisabledAddr) < TUint(KPageSize))
		{
		// restore access to the page
		*iDisabledPte = iDisabledOldVal;
		__DRAIN_WRITE_BUFFER;
		InvalidateTLBForPage(iDisabledAddr);
		iDisabledAddr = 0;
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
