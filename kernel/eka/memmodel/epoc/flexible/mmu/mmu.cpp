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
//

#include "memmodel.h"
#include "kernel/cache_maintenance.inl"
#include <kernel/cache.h>
#include <ramalloc.h>
#include <defrag.h>
#include "mm.h"
#include "mmu.h"
#include "mpager.h"
#include "mmapping.h"
#include "mobject.h"
#include "mmanager.h"
#include "mpagearray.h"


//
// SPageInfo
//

// check enough space for page infos...
__ASSERT_COMPILE((KPageInfoLinearEnd-KPageInfoLinearBase)/sizeof(SPageInfo)==(1<<(32-KPageShift)));

// check KPageInfoShift...
__ASSERT_COMPILE(sizeof(SPageInfo)==(1<<KPageInfoShift));


SPageInfo* SPageInfo::SafeFromPhysAddr(TPhysAddr aAddress)
	{
	__NK_ASSERT_DEBUG((aAddress&KPageMask)==0);
	TUint index = aAddress>>(KPageShift+KPageShift-KPageInfoShift);
	TUint flags = ((TUint8*)KPageInfoMap)[index>>3];
	TUint mask = 1<<(index&7);
	if(!(flags&mask))
		return 0; // no SPageInfo for aAddress
	SPageInfo* info = FromPhysAddr(aAddress);
	if(info->iType==SPageInfo::EInvalid)
		return 0;
	return info;
	}


#ifdef _DEBUG

void SPageInfo::CheckAccess(const char* aMessage, TUint aFlags)
	{
	if(K::Initialising || NKern::Crashed())
		return;

	if((aFlags&ECheckNotAllocated) && (iType!=EUnknown))
		{
		Kern::Printf("SPageInfo[0x%08x]::CheckAccess failed, PhysAddr()=0x%08x, iType==%d : %s",this,PhysAddr(),iType,aMessage);
		__NK_ASSERT_DEBUG(0);
		goto fail;
		}

	if((aFlags&ECheckNotUnused) && (iType==EUnused))
		{
		Kern::Printf("SPageInfo[0x%08x]::CheckAccess failed, PhysAddr()=0x%08x, iType==%d : %s",this,PhysAddr(),iType,aMessage);
		__NK_ASSERT_DEBUG(0);
		goto fail;
		}

	if((aFlags&ECheckUnused) && (iType!=EUnused))
		{
		Kern::Printf("SPageInfo[0x%08x]::CheckAccess failed, PhysAddr()=0x%08x, iType==%d : %s",this,PhysAddr(),iType,aMessage);
		__NK_ASSERT_DEBUG(0);
		goto fail;
		}

	if((aFlags&ECheckNotPaged) && (iPagedState!=EUnpaged))
		{
		Kern::Printf("SPageInfo[0x%08x]::CheckAccess failed, PhysAddr()=0x%08x, iPagedState=%d : %s",this,PhysAddr(),iPagedState,aMessage);
		__NK_ASSERT_DEBUG(0);
		goto fail;
		}

	if((aFlags&ECheckRamAllocLock) && !RamAllocLock::IsHeld())
		{
		Kern::Printf("SPageInfo[0x%08x]::CheckAccess failed, PhysAddr()=0x%08x, iType==%d : %s",this,PhysAddr(),iType,aMessage);
		__NK_ASSERT_DEBUG(0);
		goto fail;
		}

	if((aFlags&ENoCheckMmuLock) || MmuLock::IsHeld())
		return;
fail:
	Kern::Printf("SPageInfo[0x%08x]::CheckAccess failed, PhysAddr()=0x%08x : %s",this,PhysAddr(),aMessage);
	Mmu::Panic(Mmu::EUnsafePageInfoAccess);
	}


void SPageInfo::Dump()
	{
	Kern::Printf("SPageInfo for page %x = %d,%d,%02x,0x%08x,0x%x,%d",PhysAddr(),iType,iPagedState,iFlags,iOwner,iIndex,iPinCount);
	}

#endif



//
// SPageTableInfo
//

// check enough space for page table infos...
__ASSERT_COMPILE((KPageTableInfoEnd-KPageTableInfoBase)/sizeof(SPageTableInfo)
					>=(KPageTableEnd-KPageTableBase)/KPageTableSize);

// check KPtBlockShift...
__ASSERT_COMPILE((sizeof(SPageTableInfo)<<KPtBlockShift)==KPageSize);


#ifdef _DEBUG

TBool SPageTableInfo::CheckPageCount()
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	TPte* pt = PageTable();
	TUint realCount = 0;
	do if(*pt++) ++realCount;
	while(TLinAddr(pt)&(KPageTableMask/sizeof(TPte)*sizeof(TPte)));
	if(iPageCount==realCount)
		return true;
	Kern::Printf("CheckPageCount Failed: pt=0x%08x count=%d realCount=%d",TLinAddr(pt)-KPageTableSize,iPageCount,realCount);
	return false;
	}


void SPageTableInfo::CheckChangeUse(const char* aName)
	{
	if(K::Initialising)
		return;
	if(PageTablesLockIsHeld() && MmuLock::IsHeld())
		return;
	Kern::Printf("SPageTableInfo::CheckChangeUse failed : %s",aName);
	Mmu::Panic(Mmu::EUnsafePageTableInfoAccess);
	}


void SPageTableInfo::CheckCheckUse(const char* aName)
	{
	if(K::Initialising)
		return;
	if(PageTablesLockIsHeld() || MmuLock::IsHeld())
		return;
	Kern::Printf("SPageTableInfo::CheckCheckUse failed : %s",aName);
	Mmu::Panic(Mmu::EUnsafePageTableInfoAccess);
	}


void SPageTableInfo::CheckAccess(const char* aName)
	{
	if(K::Initialising)
		return;
	if(MmuLock::IsHeld())
		return;
	Kern::Printf("SPageTableInfo::CheckAccess failed : %s",aName);
	Mmu::Panic(Mmu::EUnsafePageTableInfoAccess);
	}


void SPageTableInfo::CheckInit(const char* aName)
	{
	if(K::Initialising)
		return;
	if(PageTablesLockIsHeld() && iType==EUnused)
		return;
	Kern::Printf("SPageTableInfo::CheckInit failed : %s",aName);
	Mmu::Panic(Mmu::EUnsafePageTableInfoAccess);
	}

#endif



//
// RamAllocLock
//

_LIT(KLitRamAlloc,"RamAlloc");
_LIT(KLitPhysMemSync,"PhysMemSync");

void RamAllocLock::Lock()
	{
	Mmu& m = TheMmu;
	Kern::MutexWait(*m.iRamAllocatorMutex);
	if(!m.iRamAllocLockCount++)
		{
		// first lock, so setup memory fail data...
		m.iRamAllocFailed = EFalse;
		__NK_ASSERT_DEBUG(m.iRamAllocInitialFreePages==m.FreeRamInPages()); // free RAM shouldn't have changed whilst lock was held
		}
	}


void RamAllocLock::Unlock()
	{
	Mmu& m = TheMmu;
	if(--m.iRamAllocLockCount)
		{
		Kern::MutexSignal(*m.iRamAllocatorMutex);
		return;
		}
	TBool failed = m.iRamAllocFailed;
	TUint initial = m.iRamAllocInitialFreePages;
	TUint final = m.FreeRamInPages();
	m.iRamAllocInitialFreePages = final; // new baseline value
	TUint changes = K::CheckFreeMemoryLevel(initial*KPageSize,final*KPageSize,failed);
	if(changes)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("RamAllocLock::Unlock() changes=%x",changes));
		}
	Kern::MutexSignal(*m.iRamAllocatorMutex);
	}


TBool RamAllocLock::Flash()
	{
	Unlock();
	Lock();
	return true; // lock was released
	}


TBool RamAllocLock::IsHeld()
	{
	Mmu& m = TheMmu;
	return m.iRamAllocatorMutex->iCleanup.iThread == &Kern::CurrentThread() && m.iRamAllocLockCount;
	}



//
// MmuLock
//

#ifdef _DEBUG
TUint MmuLock::UnlockGuardNest =0;
TUint MmuLock::UnlockGuardFail =0;
#endif

NFastMutex MmuLock::iLock;

void MmuLock::Lock()
	{
	NKern::FMWait(&iLock);
	}

void MmuLock::Unlock()
	{
	UnlockGuardCheck();
	NKern::FMSignal(&iLock);
	}

TBool MmuLock::Flash()
	{
	UnlockGuardCheck();
	return NKern::FMFlash(&iLock);
	}

TBool MmuLock::IsHeld()
	{
	NFastMutex& m = iLock;
	return m.HeldByCurrentThread();
	}



//
// Initialisation
//

Mmu TheMmu;

void Mmu::Init1Common()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("Mmu::Init1Common"));

	// Mmu data
	TUint pteType = PteType(ESupervisorReadWrite,true);
	iTempPteCached = BlankPte((TMemoryAttributes)(EMemoryAttributeNormalCached|EMemoryAttributeDefaultShareable),pteType);
	iTempPteUncached = BlankPte((TMemoryAttributes)(EMemoryAttributeNormalUncached|EMemoryAttributeDefaultShareable),pteType);
	iTempPteCacheMaintenance = BlankPte((TMemoryAttributes)(CacheMaintenance::TemporaryMapping()|EMemoryAttributeDefaultShareable),pteType);
	
	// other
	PP::MaxUserThreadStack=0x14000;			// 80K - STDLIB asks for 64K for PosixServer!!!!
	PP::UserThreadStackGuard=0x2000;		// 8K
	PP::MaxStackSpacePerProcess=0x200000;	// 2Mb
	K::SupervisorThreadStackSize=0x1000;	// 4K
	PP::SupervisorThreadStackGuard=0x1000;	// 4K
	K::MachineConfig=(TMachineConfig*)KMachineConfigLinAddr;
	PP::RamDriveStartAddress=0;
	PP::RamDriveRange=0;
	PP::RamDriveMaxSize=0x20000000;	// 512MB, probably will be reduced later
	K::MemModelAttributes=EMemModelTypeFlexible|EMemModelAttrNonExProt|EMemModelAttrKernProt|EMemModelAttrWriteProt|
						EMemModelAttrVA|EMemModelAttrProcessProt|EMemModelAttrSameVA|EMemModelAttrSvKernProt|
						EMemModelAttrIPCKernProt|EMemModelAttrRamCodeProt;
	}


#ifdef FMM_VERIFY_RAM
// Attempt to write to each unused RAM page and verify the contents.
void Mmu::VerifyRam()
	{
	Kern::Printf("Mmu::VerifyRam() pass 1");
	RamAllocLock::Lock();

	TPhysAddr p = 0;
	do
		{
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(p);
		if(pi)
			{
			Kern::Printf("%08x %d",p,pi->Type());
			if(pi->Type()==SPageInfo::EUnused)
				{
				volatile TPhysAddr* b = (volatile TPhysAddr*)MapTemp(p,0);
				b[0] = p;
				b[1] = ~p;
				__NK_ASSERT_DEBUG(b[0]==p);
				__NK_ASSERT_DEBUG(b[1]==~p);
				UnmapTemp();
				}
			}
		p += KPageSize;
		}
	while(p);

	TBool fail = false;
	Kern::Printf("Mmu::VerifyRam() pass 2");
	do
		{
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(p);
		if(pi)
			{
			if(pi->Type()==SPageInfo::EUnused)
				{
				volatile TPhysAddr* b = (volatile TPhysAddr*)MapTemp(p,0);
				if(b[0]!=p || b[1]!=~p)
					{
					fail = true;
					Kern::Printf("%08x FAILED %x %x",b[0],b[1]);
					}
				UnmapTemp();
				}
			}
		p += KPageSize;
		}
	while(p);

	__NK_ASSERT_DEBUG(!fail);
	RamAllocLock::Unlock();
	}
#endif


void Mmu::Init2Common()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("Mmu::Init2Common"));

	// create allocator...
	const SRamInfo& info = *(const SRamInfo*)TheSuperPage().iRamBootData;
	iRamPageAllocator = DRamAllocator::New(info, iRamZones, iRamZoneCallback);

	// initialise all pages in banks as unused...
	const SRamBank* bank = info.iBanks;
	while(bank->iSize)
		{
		TUint32 base = bank->iBase;
		TUint32 size = bank->iSize;
		__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("Found RAM bank 0x%08x size %d",base,size));
		if(base+size<=base || ((base|size)&KPageMask))
			Panic(EInvalidRamBankAtBoot);

		SPageInfo* pi = SPageInfo::FromPhysAddr(base);
		SPageInfo* piEnd = pi+(size>>KPageShift);
		while(pi<piEnd)
			(pi++)->SetUnused();
		++bank;
		}
	// step over the last bank to get to the reserved banks.
	++bank;
	// mark any reserved regions as allocated...
	while(bank->iSize)
		{
		TUint32 base = bank->iBase;
		TUint32 size = bank->iSize;
		__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("Found reserved bank 0x%08x size %d",base,size));
		if(base+size<=base || ((base|size)&KPageMask))
			Panic(EInvalidReservedBankAtBoot);

		SPageInfo* pi = SPageInfo::FromPhysAddr(base);
		SPageInfo* piEnd = pi+(size>>KPageShift);
		while(pi<piEnd)
			(pi++)->SetPhysAlloc();
		++bank;
		}

	// Clear the inital (and only so far) page table info page so all unused
	// page tables infos will be marked as unused.
	__ASSERT_COMPILE(SPageTableInfo::EUnused == 0);
	memclr((TAny*)KPageTableInfoBase, KPageSize);

	// look for page tables - assume first page table maps page tables
	TPte* pPte = (TPte*)KPageTableBase;
	TInt i;
	for(i=0; i<KChunkSize/KPageSize; ++i)
		{
		TPte pte = *pPte++;
		if(pte==KPteUnallocatedEntry)	// after boot, page tables are contiguous
			break;
		TPhysAddr ptpgPhys = Mmu::PtePhysAddr(pte,i);
		__KTRACE_OPT(KBOOT,Kern::Printf("Page Table Group %08x -> Phys %08x", KPageTableBase+i*KPageSize, ptpgPhys));
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(ptpgPhys);
		__ASSERT_ALWAYS(pi, Panic(EInvalidPageTableAtBoot));
		pi->SetFixed(i); // this also sets the SPageInfo::iOffset so that linear-to-physical works
		}

	// look for mapped pages
	TPde* pd = Mmu::PageDirectory(KKernelOsAsid);
	for(i=0; i<(1<<(32-KChunkShift)); ++i)
		{
		TPde pde = pd[i];
		if(pde==KPdeUnallocatedEntry)
			continue;
		TPhysAddr pdePhys = Mmu::PdePhysAddr(pde);
		TPte* pt = 0;
		if(pdePhys!=KPhysAddrInvalid)
			{
			__KTRACE_OPT(KBOOT,Kern::Printf("Addr %08x -> Whole PDE Phys %08x", i<<KChunkShift, pdePhys));
			}
		else
			{
			pt = Mmu::PageTableFromPde(pde);
			__KTRACE_OPT(KBOOT,Kern::Printf("Addr %08x -> page table %08x", i<<KChunkShift, pt));
			__ASSERT_ALWAYS(pt,Panic(EInvalidPdeAtBoot)); // bad PDE
			}

		TInt j;
		TInt np = 0;
		for(j=0; j<KChunkSize/KPageSize; ++j)
			{
			TBool present = ETrue;	// all pages present if whole PDE mapping
			TPte pte = 0;
			if(pt)
				{
				pte = pt[j];
				present = pte!=KPteUnallocatedEntry;
				}
			if(present)
				{
				++np;
				TPhysAddr pa = pt ? Mmu::PtePhysAddr(pte,j) : (pdePhys + (j<<KPageShift));
				SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pa);
				__KTRACE_OPT(KBOOT,Kern::Printf("Addr: %08x PA=%08x",
													(i<<KChunkShift)+(j<<KPageShift), pa));
				if(pi)	// ignore non-RAM mappings
					{
					TInt r = iRamPageAllocator->MarkPageAllocated(pa, EPageFixed);
					// allow KErrAlreadyExists since it's possible that a page is doubly mapped
					__ASSERT_ALWAYS(r==KErrNone || r==KErrAlreadyExists, Panic(EBadMappedPageAfterBoot));
					if(pi->Type()==SPageInfo::EUnused)
						pi->SetFixed();
#ifdef BTRACE_KERNEL_MEMORY
					if(r == KErrNone)
						++Epoc::KernelMiscPages;
#endif
					}
				}
			}
		__KTRACE_OPT(KBOOT,Kern::Printf("Addr: %08x #PTEs=%d",(i<<KChunkShift),np));
		if(pt)
			{
			SPageTableInfo* pti = SPageTableInfo::FromPtPtr(pt);
			pti->Boot(np);
			}
		}

	TInt r = K::MutexCreate(iRamAllocatorMutex, KLitRamAlloc, NULL, EFalse, KMutexOrdRamAlloc);
	if(r!=KErrNone)
		Panic(ERamAllocMutexCreateFailed);
	iRamAllocLockCount = 0;
	iRamAllocInitialFreePages = FreeRamInPages();

	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("Mmu::DoInit2"));

	for(i=0; i<KNumTempMappingSlots; ++i)
		iTempMap[i].Alloc(1);

	iPhysMemSyncTemp.Alloc(1);
	r = K::MutexCreate(iPhysMemSyncMutex, KLitPhysMemSync, NULL, EFalse, KMutexOrdSyncPhysMem);
	if(r!=KErrNone)
		Panic(EPhysMemSyncMutexCreateFailed);

#ifdef FMM_VERIFY_RAM
	VerifyRam();
#endif
	}


void Mmu::Init2FinalCommon()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("Mmu::Init2FinalCommon"));
	// Reduce free memory to <2GB...
	while(FreeRamInPages()>=0x80000000/KPageSize)
		{
		TPhysAddr dummyPage;
		TInt r = iRamPageAllocator->AllocRamPages(&dummyPage,1, EPageFixed);
		__NK_ASSERT_ALWAYS(r==KErrNone);
		}
	// Reduce total RAM to <2GB...
	if(TheSuperPage().iTotalRamSize<0)
		TheSuperPage().iTotalRamSize = 0x80000000-KPageSize;

	// Save current free RAM size - there can never be more free RAM than this
	TUint maxFreePages = FreeRamInPages();
	K::MaxFreeRam = maxFreePages*KPageSize;
	if(maxFreePages < (TUint(PP::RamDriveMaxSize)>>KPageShift))
		PP::RamDriveMaxSize = maxFreePages*KPageSize;

	// update this to stop assert triggering in RamAllocLock::Lock()
	iRamAllocInitialFreePages = maxFreePages;

	// Get the allocator to signal to the variant which RAM zones are in use so far
	iRamPageAllocator->InitialCallback();
	}

 
void Mmu::Init3()
	{
	iDefrag = new Defrag;
	if (!iDefrag)
		Panic(EDefragAllocFailed);
	iDefrag->Init3(TheMmu.iRamPageAllocator);
	}


void Mmu::BTracePrime(TUint aCategory)
	{
	(void)aCategory;

#ifdef BTRACE_RAM_ALLOCATOR
	// Must check for -1 as that is the default value of aCategory for
	// BTrace::Prime() which is intended to prime all categories that are 
	// currently enabled via a single invocation of BTrace::Prime().
	if(aCategory==BTrace::ERamAllocator || (TInt)aCategory == -1)
		{
		NKern::ThreadEnterCS();
		RamAllocLock::Lock();
		iRamPageAllocator->DoBTracePrime();
		RamAllocLock::Unlock();
		NKern::ThreadLeaveCS();
		}
#endif
	}


//
// Utils
//

void Mmu::Panic(TPanic aPanic)
	{
	Kern::Fault("MMU",aPanic);
	}


TUint Mmu::FreeRamInPages()
	{
	return iRamPageAllocator->FreeRamInPages()+ThePager.NumberOfFreePages();
	}


TUint Mmu::TotalPhysicalRamPages()
	{
	return iRamPageAllocator->TotalPhysicalRamPages();
	}


const SRamZone* Mmu::RamZoneConfig(TRamZoneCallback& aCallback) const
	{
	aCallback = iRamZoneCallback;
	return iRamZones;
	}


void Mmu::SetRamZoneConfig(const SRamZone* aZones, TRamZoneCallback aCallback)
	{
	iRamZones = aZones;
	iRamZoneCallback = aCallback;
	}


TInt Mmu::ModifyRamZoneFlags(TUint aId, TUint aClearMask, TUint aSetMask)
	{
	return iRamPageAllocator->ModifyZoneFlags(aId, aClearMask, aSetMask);
	}


TInt Mmu::GetRamZonePageCount(TUint aId, SRamZonePageCount& aPageData)
	{
	return iRamPageAllocator->GetZonePageCount(aId, aPageData);
	}


TInt Mmu::ZoneAllocPhysicalRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aBytes, TPhysAddr& aPhysAddr, TInt aAlign)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::ZoneAllocPhysicalRam(?,%d,%d,?,%d)", aZoneIdCount, aBytes, aPhysAddr, aAlign));
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());

	TInt r = iRamPageAllocator->ZoneAllocContiguousRam(aZoneIdList, aZoneIdCount, aBytes, aPhysAddr, aAlign);
	if(r!=KErrNone)
		iRamAllocFailed = ETrue;
	else
		{
		TUint pages = MM::RoundToPageCount(aBytes);
		AllocatedPhysicalRam(aPhysAddr, pages,  (Mmu::TRamAllocFlags)EMemAttStronglyOrdered);
		}
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::ZoneAllocPhysicalRam returns %d and aPhysAddr=0x%08x",r,aPhysAddr));
	return r;
	}


TInt Mmu::ZoneAllocPhysicalRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aNumPages, TPhysAddr* aPageList)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::ZoneAllocPhysicalRam(?,%d,%d,?)", aZoneIdCount, aNumPages));
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());

	TInt r = iRamPageAllocator->ZoneAllocRamPages(aZoneIdList, aZoneIdCount, aPageList, aNumPages, EPageFixed);
	if(r!=KErrNone)
		iRamAllocFailed = ETrue;
	else
		{
		PagesAllocated(aPageList, aNumPages, (Mmu::TRamAllocFlags)EMemAttStronglyOrdered);

		// update page infos...
		SetAllocPhysRam(aPageList, aNumPages);
		}
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::ZoneAllocPhysicalRam returns %d",r));
	return r;
	}


TInt Mmu::RamHalFunction(TInt aFunction, TAny* a1, TAny* a2)
	{
	// This function should only be registered with hal and therefore can only 
	// be invoked after the ram allocator has been created.
	__NK_ASSERT_DEBUG(iRamPageAllocator);
	return iRamPageAllocator->HalFunction(aFunction, a1, a2);
	}


void Mmu::ChangePageType(SPageInfo* aPageInfo, TZonePageType aOldPageType, TZonePageType aNewPageType)
	{
	iRamPageAllocator->ChangePageType(aPageInfo, aOldPageType, aNewPageType);
	}

TInt Mmu::HandlePageFault(TLinAddr aPc, TLinAddr aFaultAddress, TUint aAccessPermissions, TAny* aExceptionInfo)
	{
	TRACE(("Mmu::HandlePageFault(0x%08x,0x%08x,%d)",aPc,aFaultAddress,aAccessPermissions));

	DMemModelThread* thread = (DMemModelThread*)TheCurrentThread;
	// Get the os asid of the process taking the fault, no need to open a reference 
	// as it is the current thread's process so can't be freed.
	TUint faultOsAsid = ((DMemModelProcess*)thread->iNThread.iAddressSpace)->OsAsid();

	// check if any fast mutexes held...
	NFastMutex* fm = NKern::HeldFastMutex();
	TPagingExcTrap* trap = thread->iPagingExcTrap;
	if(fm)
		{
		// check there is an XTRAP_PAGING in effect...
		if(!trap)
			{
			// oops, kill system...
			__KTRACE_OPT2(KPAGING,KPANIC,Kern::Printf("Fault with FM Held! addr=0x%08x (%O pc=%x)",aFaultAddress,thread,aPc));
			Exc::Fault(aExceptionInfo);
			}

		// release the fast mutex...
		NKern::FMSignal(fm);
		}

	NKern::ThreadEnterCS();

	// work out address space for aFaultAddress...
	TUint osAsid = faultOsAsid;
	TLinAddr addr = aFaultAddress;
	if(thread->iAliasLinAddr && TUint(addr - thread->iAliasLinAddr) < TUint(KPageSize))
		{
		// Address in aliased memory...
		addr = (addr - thread->iAliasLinAddr) + thread->iAliasTarget;
		// Get the os asid of the process thread is aliasing, no need to open 
		// a reference on it as one was already opened when the alias was created.
		osAsid = thread->iAliasProcess->OsAsid();
		}
	else if(addr>=KGlobalMemoryBase)
		{
		// Address in global region, so look it up in kernel's address space...
		osAsid = KKernelOsAsid;
		}

	// NOTE, osAsid will remain valid for duration of this function because it is either
	// - The current thread's address space, which can't go away whilst the thread
	//   is running.
	// - The address space of another thread which we are aliasing memory from,
	//   and we would only do this if we have a reference on this other thread,
	//   which has a reference on it's process, which should own the address space!

#ifdef __BROADCAST_CACHE_MAINTENANCE__
	TInt aliasAsid = -1;
	if (thread->iAliasLinAddr)
		{
		// If an alias is in effect, the the thread will be locked to the current CPU,
		// but we need to be able to migrate between CPUs for cache maintainance.  This
		// must be dealt with by removing the alias and restoring it with a paging trap
		// handler.
		if(!trap)
			{
			// oops, kill system...
			__KTRACE_OPT2(KPAGING,KPANIC,Kern::Printf("Fault with thread locked to current CPU! addr=0x%08x (%O pc=%x)",aFaultAddress,thread,aPc));
			Exc::Fault(aExceptionInfo);
			}
		// Open a reference on the aliased process's os asid before removing the alias
		// so that the address space can't be freed while we try to access its members.
		aliasAsid = thread->iAliasProcess->TryOpenOsAsid();
		// This should never fail as until we remove the alias there will 
		// always be at least one reference on the os asid.
		__NK_ASSERT_DEBUG(aliasAsid >= 0);
		thread->RemoveAlias();
		}
#endif

	// find mapping...
	TUint offsetInMapping;
	TUint mapInstanceCount;
	DMemoryMapping* mapping = MM::FindMappingInAddressSpace(osAsid, addr, 1, offsetInMapping, mapInstanceCount);
//	TRACE(("%O mapping=0x%08x",TheCurrentThread,mapping));
	TInt r = KErrNotFound;

	if(mapping)
		{
		MmuLock::Lock();

		// check if we need to process page fault...
		if(!Mmu::CheckPteTypePermissions(mapping->PteType(),aAccessPermissions) ||
			mapInstanceCount != mapping->MapInstanceCount())
			{
			// Invalid access to the page.
			MmuLock::Unlock();
			r = KErrAbort;
			}
		else
			{
			// Should not be able to take a fault on a pinned mapping if accessing it 
			// with the correct permissions.
			__NK_ASSERT_DEBUG(!mapping->IsPinned());

			// we do need to handle fault so is this a demand paging or page moving fault
			DMemoryObject* memory = mapping->Memory();
			if(!memory)
				MmuLock::Unlock();
			else
				{
				TUint faultIndex = (offsetInMapping >> KPageShift) + mapping->iStartIndex;
				memory->Open();

				// This is safe as we have the instance count so can detect the mapping 
				// being reused and we have a reference to the memory object so it can't 
				// be deleted.
				MmuLock::Unlock();

				if(memory->IsDemandPaged())
					{
					// Let the pager handle the fault...
					r = ThePager.HandlePageFault(	aPc, aFaultAddress, faultOsAsid, faultIndex,
													aAccessPermissions, memory, mapping, mapInstanceCount,
													thread, aExceptionInfo);
					}
				else
					{// The page could be being moved so verify that with its manager.
					DMemoryManager* manager = memory->iManager;
					r = manager->HandleFault(memory, faultIndex, mapping, mapInstanceCount, aAccessPermissions);
					}
				if (r == KErrNone)
					{// alias PDE needs updating because page tables have changed...
					thread->RefreshAlias();
					}
				memory->Close();
				}
			}
		mapping->Close();
		}

	if (trap)
		{
		// restore address space (because the trap will bypass any code
		// which would have done this.)...
		DMemModelThread::RestoreAddressSpace();
		}

#ifdef __BROADCAST_CACHE_MAINTENANCE__
	// Close any reference on the aliased process's os asid before we leave the
	// critical section.
	if (aliasAsid >= 0)
		{
		thread->iAliasProcess->CloseOsAsid();
		}
#endif

	NKern::ThreadLeaveCS();  // thread will die now if CheckRealtimeThreadFault caused a panic

	// deal with XTRAP_PAGING...
	if(trap)
		{
		// re-acquire any fast mutex which was held before the page fault...
		if(fm)
			NKern::FMWait(fm);
		if (r == KErrNone)
			{
			trap->Exception(1); // return from exception trap with result '1' (value>0)
			// code doesn't continue beyond this point.
			__NK_ASSERT_DEBUG(0);
			}
		}

	return r;
	}


//
// Memory allocation
//

TInt Mmu::AllocRam(	TPhysAddr* aPages, TUint aCount, TRamAllocFlags aFlags, TZonePageType aZonePageType, 
					TUint aBlockZoneId, TBool aBlockRest)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::AllocRam(?,%d,%x)",aCount,aFlags));
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
#ifdef _DEBUG
	if(K::CheckForSimulatedAllocFail())
		{
		__KTRACE_OPT(KMMU,Kern::Printf("Mmu::AllocRam returns simulated OOM %d",KErrNoMemory));
		return KErrNoMemory;
		}
#endif
	TInt missing = iRamPageAllocator->AllocRamPages(aPages, aCount, aZonePageType, aBlockZoneId, aBlockRest);
	if(missing && !(aFlags&EAllocNoPagerReclaim))
		{
		// taking the page cleaning lock here prevents the pager releasing the ram alloc lock
		PageCleaningLock::Lock();  
		if (ThePager.GetFreePages(missing))
			missing = iRamPageAllocator->AllocRamPages(aPages, aCount, aZonePageType, aBlockZoneId, aBlockRest);
		PageCleaningLock::Unlock();  
		}
	TInt r = missing ? KErrNoMemory : KErrNone;
	if(r!=KErrNone)
		iRamAllocFailed = ETrue;
	else
		PagesAllocated(aPages,aCount,aFlags);
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::AllocRam returns %d",r));
	return r;
	}


/**
Mark a page as being allocated to a particular page type.

NOTE - This page should not be used until PagesAllocated() has been invoked on it.

@param aPhysAddr		The physical address of the page to mark as allocated.
@param aZonePageType	The type of the page to mark as allocated.
*/
void Mmu::MarkPageAllocated(TPhysAddr aPhysAddr, TZonePageType aZonePageType)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::MarkPageAllocated(0x%x, %d)", aPhysAddr, aZonePageType));
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	iRamPageAllocator->MarkPageAllocated(aPhysAddr, aZonePageType);
	}


void Mmu::FreeRam(TPhysAddr* aPages, TUint aCount, TZonePageType aZonePageType)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::FreeRam(?,%d)",aCount));
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());

	// update page infos...
	TPhysAddr* pages = aPages;
	TPhysAddr* pagesEnd = pages+aCount;
	TPhysAddr* pagesOut = aPages;
	MmuLock::Lock();
	TUint flash = 0;
	while(pages<pagesEnd)
		{
		MmuLock::Flash(flash,KMaxPageInfoUpdatesInOneGo/2);
		TPhysAddr pagePhys = *pages++;
		__NK_ASSERT_DEBUG(pagePhys!=KPhysAddrInvalid);
		SPageInfo* pi = SPageInfo::FromPhysAddr(pagePhys);
		PageFreed(pi);

		switch (ThePager.PageFreed(pi))
			{
			case KErrNone: 
				--aCount; // pager has dealt with this page, so one less for us
				break;
			case KErrCompletion:
				// This was a pager controlled page but it is no longer required.
				__NK_ASSERT_DEBUG(aZonePageType == EPageMovable || aZonePageType == EPageDiscard);
				__NK_ASSERT_DEBUG(pi->PagedState() == SPageInfo::EUnpaged);
				if (aZonePageType == EPageMovable)
					{// This page was donated to the pager so have to free it here
					// as aZonePageType is incorrect for this page but aPages may 
					// contain a mixture of movable and discardable pages.
					MmuLock::Unlock();
					iRamPageAllocator->FreeRamPages(&pagePhys, 1, EPageDiscard);
					aCount--; // We've freed this page here so one less to free later
					flash = 0;	// reset flash count as we released the mmulock.
					MmuLock::Lock();
					break;
					}
				// fall through..
			default:
				// Free this page..
				__NK_ASSERT_DEBUG(pi->PagedState() == SPageInfo::EUnpaged);
				*pagesOut++ = pagePhys; // store page address for freeing later
			}
		}
	MmuLock::Unlock();

	iRamPageAllocator->FreeRamPages(aPages, aCount, aZonePageType);
	}


TInt Mmu::AllocContiguousRam(TPhysAddr& aPhysAddr, TUint aCount, TUint aAlign, TRamAllocFlags aFlags)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::AllocContiguousRam(?,0x%x,%d,%x)",aCount,aAlign,aFlags));
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
#ifdef _DEBUG
	if(K::CheckForSimulatedAllocFail())
		{
		__KTRACE_OPT(KMMU,Kern::Printf("Mmu::AllocContiguousRam returns simulated OOM %d",KErrNoMemory));
		return KErrNoMemory;
		}
	// Only the pager sets EAllocNoPagerReclaim and it shouldn't allocate contiguous ram.
	__NK_ASSERT_DEBUG(!(aFlags&EAllocNoPagerReclaim));
#endif
	TInt r = iRamPageAllocator->AllocContiguousRam(aCount, aPhysAddr, aAlign+KPageShift);
	if(r!=KErrNone)
		iRamAllocFailed = ETrue;
	else
		PagesAllocated((TPhysAddr*)(aPhysAddr|1), aCount, aFlags);
	__KTRACE_OPT(KMMU,Kern::Printf("AllocContiguousRam returns %d and aPhysAddr=0x%08x",r,aPhysAddr));
	return r;
	}


void Mmu::FreeContiguousRam(TPhysAddr aPhysAddr, TUint aCount)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::FreeContiguousRam(0x%08x,0x%x)",aPhysAddr,aCount));
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG((aPhysAddr&KPageMask)==0);

	TUint pageCount = aCount;

	// update page infos...
	SPageInfo* pi = SPageInfo::FromPhysAddr(aPhysAddr);
	SPageInfo* piEnd = pi+pageCount;
	TUint flash = 0;
	MmuLock::Lock();
	while(pi<piEnd)
		{
		MmuLock::Flash(flash,KMaxPageInfoUpdatesInOneGo);
		PageFreed(pi++);
		}
	MmuLock::Unlock();

	// free pages...
	while(pageCount)
		{
		iRamPageAllocator->FreeRamPage(aPhysAddr, EPageFixed);
		aPhysAddr += KPageSize;
		--pageCount;
		}
	}


TInt Mmu::AllocPhysicalRam(TPhysAddr* aPages, TUint aCount, TRamAllocFlags aFlags)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::AllocPhysicalRam(?,%d,%x)",aCount,aFlags));
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	// Allocate fixed pages as physically allocated pages aren't movable or discardable.
	TInt r = AllocRam(aPages, aCount, aFlags, EPageFixed);
	if (r!=KErrNone)
		return r;

	// update page infos...
	SetAllocPhysRam(aPages, aCount);

	return KErrNone;
	}


void Mmu::FreePhysicalRam(TPhysAddr* aPages, TUint aCount)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::FreePhysicalRam(?,%d)",aCount));
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());

	// update page infos...
	TPhysAddr* pages = aPages;
	TPhysAddr* pagesEnd = pages+aCount;
	MmuLock::Lock();
	TUint flash = 0;
	while(pages<pagesEnd)
		{
		MmuLock::Flash(flash,KMaxPageInfoUpdatesInOneGo/2);
		TPhysAddr pagePhys = *pages++;
		__NK_ASSERT_DEBUG(pagePhys!=KPhysAddrInvalid);
		SPageInfo* pi = SPageInfo::FromPhysAddr(pagePhys);
		__ASSERT_ALWAYS(pi->Type()==SPageInfo::EPhysAlloc, Panic(EBadFreePhysicalRam));
		__ASSERT_ALWAYS(!pi->UseCount(), Panic(EBadFreePhysicalRam));
		pi->SetUnused();
		}
	MmuLock::Unlock();

	iRamPageAllocator->FreeRamPages(aPages,aCount, EPageFixed);

#ifdef BTRACE_KERNEL_MEMORY
	if (BTrace::CheckFilter(BTrace::EKernelMemory))
		{// Only loop round each page if EKernelMemory tracing is enabled
		pages = aPages;
		pagesEnd = aPages + aCount;
		while (pages < pagesEnd)
			{
			BTrace8(BTrace::EKernelMemory, BTrace::EKernelMemoryDrvPhysFree, KPageSize, *pages++);
			Epoc::DriverAllocdPhysRam -= KPageSize;
			}
		}
#endif
	}


TInt Mmu::AllocPhysicalRam(TPhysAddr& aPhysAddr, TUint aCount, TUint aAlign, TRamAllocFlags aFlags)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::AllocPhysicalRam(?,0x%x,d,%x)",aCount,aAlign,aFlags));
	TInt r = AllocContiguousRam(aPhysAddr,aCount,aAlign,aFlags);
	if (r!=KErrNone)
		return r;

	// update page infos...
	SetAllocPhysRam(aPhysAddr, aCount);

	return KErrNone;
	}


void Mmu::FreePhysicalRam(TPhysAddr aPhysAddr, TUint aCount)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::FreePhysicalRam(0x%08x,0x%x)",aPhysAddr,aCount));

	// update page infos...
	SPageInfo* pi = SPageInfo::FromPhysAddr(aPhysAddr);
	SPageInfo* piEnd = pi+aCount;
	TUint flash = 0;
	MmuLock::Lock();
	while(pi<piEnd)
		{
		MmuLock::Flash(flash,KMaxPageInfoUpdatesInOneGo);
		__ASSERT_ALWAYS(pi->Type()==SPageInfo::EPhysAlloc, Panic(EBadFreePhysicalRam));
		__ASSERT_ALWAYS(!pi->UseCount(), Panic(EBadFreePhysicalRam));
		pi->SetUnused();
		++pi;
		}
	MmuLock::Unlock();

	TUint bytes = aCount << KPageShift;
	iRamPageAllocator->FreePhysicalRam(aPhysAddr, bytes);

#ifdef BTRACE_KERNEL_MEMORY
	BTrace8(BTrace::EKernelMemory, BTrace::EKernelMemoryDrvPhysFree, bytes, aPhysAddr);
	Epoc::DriverAllocdPhysRam -= bytes;
#endif
	}


TInt Mmu::FreeRamZone(TUint aZoneId)
	{
	TPhysAddr zoneBase;
	TUint zonePages;
	TInt r = iRamPageAllocator->GetZoneAddress(aZoneId, zoneBase, zonePages);
	if (r != KErrNone)
		return r;
	FreePhysicalRam(zoneBase, zonePages);
	return KErrNone;
	}


TInt Mmu::ClaimPhysicalRam(TPhysAddr aPhysAddr, TUint aCount, TRamAllocFlags aFlags)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::ClaimPhysicalRam(0x%08x,0x%x,0x%08x)",aPhysAddr,aCount,aFlags));
	aPhysAddr &= ~KPageMask;
	TInt r = iRamPageAllocator->ClaimPhysicalRam(aPhysAddr, aCount << KPageShift);
	if(r != KErrNone)
		return r;

	AllocatedPhysicalRam(aPhysAddr, aCount, aFlags);
	return KErrNone;
	}


void Mmu::AllocatedPhysicalRam(TPhysAddr aPhysAddr, TUint aCount, TRamAllocFlags aFlags)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::AllocatedPhysicalRam(0x%08x,0x%x,d,%x)",aPhysAddr,aCount,aFlags));

	PagesAllocated((TPhysAddr*)(aPhysAddr|1), aCount, aFlags);

	// update page infos...
	SetAllocPhysRam(aPhysAddr, aCount);
	}


void Mmu::SetAllocPhysRam(TPhysAddr aPhysAddr, TUint aCount)
	{
	SPageInfo* pi = SPageInfo::FromPhysAddr(aPhysAddr);
	SPageInfo* piEnd = pi+aCount;
	TUint flash = 0;
	MmuLock::Lock();
	while(pi<piEnd)
		{
		MmuLock::Flash(flash, KMaxPageInfoUpdatesInOneGo);
		pi->SetPhysAlloc();
		++pi;
		}
	MmuLock::Unlock();

#ifdef BTRACE_KERNEL_MEMORY
	TUint bytes = aCount << KPageShift;
	BTrace8(BTrace::EKernelMemory, BTrace::EKernelMemoryDrvPhysAlloc, bytes, aPhysAddr);
	Epoc::DriverAllocdPhysRam += bytes;
#endif
	}


void Mmu::SetAllocPhysRam(TPhysAddr* aPageList, TUint aNumPages)
	{
	TPhysAddr* page = aPageList;
	TPhysAddr* pageEnd = aPageList + aNumPages;
	TUint flash = 0;
	MmuLock::Lock();
	while (page < pageEnd)
		{
		MmuLock::Flash(flash, KMaxPageInfoUpdatesInOneGo / 2);
		TPhysAddr pagePhys = *page++;
		__NK_ASSERT_DEBUG(pagePhys != KPhysAddrInvalid);
		SPageInfo::FromPhysAddr(pagePhys)->SetPhysAlloc();
		}
	MmuLock::Unlock();

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


//
// Misc
//

#ifdef _DEBUG
/**
Perform a page table walk to return the physical address of
the memory mapped at virtual address \a aLinAddr in the
address space \a aOsAsid.

If the page table used was not one allocated by the kernel
then the results are unpredictable and may cause a system fault.

@pre #MmuLock held.
*/
TPhysAddr Mmu::LinearToPhysical(TLinAddr aLinAddr, TInt aOsAsid)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld() || K::Initialising);
	return UncheckedLinearToPhysical(aLinAddr,aOsAsid);
	}
#endif


/**
Next virtual address available for allocation by TTempMapping.
This is initialised to #KTempAddr and addresses may be allocated
until they reach #KTempAddrEnd.
*/
TLinAddr Mmu::TTempMapping::iNextLinAddr = KTempAddr;


/**
Allocate virtual address space required to map a given number of memory pages.

The actual size of allocated virtual allocated needs to accommodate \a aNumPages
number of pages of any colour. For example: if \a aNumPages == 4 and #KPageColourCount == 4,
then at least 7 pages are required. 

@param aNumPages	Maximum number of pages that can be mapped into this temporary mapping.

@pre Called in single threaded content (boot) only.

@pre #iNextLinAddr points to virtual page with zero colour.
@post #iNextLinAddr points to virtual page with zero colour.
*/
void Mmu::TTempMapping::Alloc(TUint aNumPages)
	{
	__NK_ASSERT_DEBUG(aNumPages<=(KTempAddrEnd-KTempAddr)/KPageSize);

	// This runs during the boot only (single threaded context) so the access to iNextLinAddr is not guarded by any mutex.
	TLinAddr tempAddr = iNextLinAddr;
	TUint numPages = (KPageColourMask+aNumPages+KPageColourMask)&~KPageColourMask;
	iNextLinAddr = tempAddr+numPages*KPageSize;

	__NK_ASSERT_ALWAYS(iNextLinAddr<=KTempAddrEnd);

	__NK_ASSERT_DEBUG(iSize==0);
	iLinAddr = tempAddr;
	MmuLock::Lock();
	iPtePtr = Mmu::PtePtrFromLinAddr(tempAddr,KKernelOsAsid);
	__NK_ASSERT_DEBUG(iPtePtr);
	MmuLock::Unlock();
	iBlankPte = TheMmu.iTempPteCached;
	iSize = aNumPages;
	iCount = 0;

	TRACEB(("Mmu::TTempMapping::Alloc(%d) iLinAddr=0x%08x, iPtePtr=0x%08x",aNumPages,iLinAddr,iPtePtr));
	}


/**
Map a single physical page into this temporary mapping.

Supervisor read/write access and EMemoryAttributeStandard memory attributes apply.

@param aPage		The physical page to map.
@param aColour 		The required colour for the mapping.

@return 			The linear address at which the page is mapped.
*/
TLinAddr Mmu::TTempMapping::Map(TPhysAddr aPage, TUint aColour)
	{
	__NK_ASSERT_DEBUG(iSize>=1);
	__NK_ASSERT_DEBUG(iCount==0);

	return Map(aPage, aColour, iBlankPte);
	}


/**
Map a single physical page into this temporary mapping using the given page table entry (PTE) value.

@param aPage		The physical page to map.
@param aColour 		The required colour for the mapping.
@param aBlankPte	The PTE value to use for mapping the page,
					with the physical address component equal to zero.

@return 			The linear address at which the page is mapped.
*/
TLinAddr Mmu::TTempMapping::Map(TPhysAddr aPage, TUint aColour, TPte aBlankPte)
	{
	__NK_ASSERT_DEBUG(iSize>=1);
	__NK_ASSERT_DEBUG(iCount==0);
	__NK_ASSERT_DEBUG(!(aBlankPte & ~KPageMask));

	TUint colour = aColour & KPageColourMask;
	TLinAddr addr = iLinAddr + (colour << KPageShift);
	TPte* pPte = iPtePtr + colour;
	iColour = colour;

	__ASSERT_DEBUG(*pPte == KPteUnallocatedEntry, MM::Panic(MM::ETempMappingAlreadyInUse));
	*pPte = (aPage & ~KPageMask) | aBlankPte;
	CacheMaintenance::SinglePteUpdated((TLinAddr)pPte);
	InvalidateTLBForPage(addr | KKernelOsAsid);

	iCount = 1;
	return addr;
	}


/**
Map a number of physical pages into this temporary mapping.

Supervisor read/write access and EMemoryAttributeStandard memory attributes apply.

@param aPages		The array of physical pages to map.
@param aCount		The number of pages to map.
@param aColour 		The required colour for the first page.
					Consecutive pages will be coloured accordingly.

@return 			The linear address at which the first page is mapped.
*/
TLinAddr Mmu::TTempMapping::Map(TPhysAddr* aPages, TUint aCount, TUint aColour)
	{
	__NK_ASSERT_DEBUG(iSize>=aCount);
	__NK_ASSERT_DEBUG(iCount==0);

	TUint colour = aColour&KPageColourMask;
	TLinAddr addr = iLinAddr+(colour<<KPageShift);
	TPte* pPte = iPtePtr+colour;
	iColour = colour;

	for(TUint i=0; i<aCount; ++i)
		{
		__ASSERT_DEBUG(pPte[i]==KPteUnallocatedEntry,MM::Panic(MM::ETempMappingAlreadyInUse));
		pPte[i] = (aPages[i]&~KPageMask) | iBlankPte;
		CacheMaintenance::SinglePteUpdated((TLinAddr)&pPte[i]);
		InvalidateTLBForPage((addr+i*KPageSize)|KKernelOsAsid);
		}

	iCount = aCount;
	return addr;
	}


/**
Unmap all pages from this temporary mapping.

@param aIMBRequired	True if IMB barrier is required prior unmapping.
*/
void Mmu::TTempMapping::Unmap(TBool aIMBRequired)
	{
	__NK_ASSERT_DEBUG(iSize>=1);
	if(aIMBRequired)
		CacheMaintenance::CodeChanged(iLinAddr+iColour*KPageSize,iCount*KPageSize);
	Unmap();
	}


/**
Unmap all pages from this temporary mapping.
*/
void Mmu::TTempMapping::Unmap()
	{
	__NK_ASSERT_DEBUG(iSize>=1);

	TUint colour = iColour;
	TLinAddr addr = iLinAddr+(colour<<KPageShift);
	TPte* pPte = iPtePtr+colour;

	while(iCount)
		{
		*pPte = KPteUnallocatedEntry;
		CacheMaintenance::SinglePteUpdated((TLinAddr)pPte);
		InvalidateTLBForPage(addr|KKernelOsAsid);
		addr += KPageSize;
		++pPte;
		--iCount;
		}
	}

#ifdef __SMP__
/**
Dummy IPI to be invoked when a thread's alias pde members are updated remotely
by another thread.

@internalComponent
*/
class TAliasIPI : public TGenericIPI
	{
public:
	static void RefreshIsr(TGenericIPI*);
	void RefreshAlias();
	};


/**
Dummy isr method.
*/
void TAliasIPI::RefreshIsr(TGenericIPI*)
	{
	TRACE2(("TAliasIPI"));
	}


/**
Queue the dummy IPI on all other processors.  This ensures that DoProcessSwitch will
have completed updating iAliasPdePtr once this method returns.
*/
void TAliasIPI::RefreshAlias()
	{
	NKern::Lock();
	QueueAllOther(&RefreshIsr);
	NKern::Unlock();
	WaitCompletion();
	}


/** 
Perform a dummy ipi on all the other processors to ensure if any of them are 
executing DoProcessSwitch they will see the new value of iAliasPde before they 
update iAliasPdePtr or will finish updating iAliasPdePtr before we continue.  
This works as DoProcessSwitch() has interrupts disabled while reading iAliasPde 
and updating iAliasPdePtr.
*/
void BroadcastAliasRefresh()
	{
	TAliasIPI ipi;
	ipi.RefreshAlias();
	}
#endif //__SMP__

/**
Remove any thread IPC aliases which use the specified page table.
This is used by the page table allocator when a page table is freed.

@pre #PageTablesLockIsHeld
*/
void Mmu::RemoveAliasesForPageTable(TPhysAddr aPageTable)
	{
	__NK_ASSERT_DEBUG(PageTablesLockIsHeld());

	MmuLock::Lock();

	SDblQue checkedList;

	TUint ptId = aPageTable>>KPageTableShift;
	while(!iAliasList.IsEmpty())
		{
		SDblQueLink* next = iAliasList.First()->Deque();
		checkedList.Add(next);
		DMemModelThread* thread = (DMemModelThread*)((TInt)next-_FOFF(DMemModelThread,iAliasLink));
		if((thread->iAliasPde>>KPageTableShift)==ptId)
			{
			// the page table is being aliased by the thread, so remove it...
			TRACE2(("Thread %O RemoveAliasesForPageTable", this));
			thread->iAliasPde = KPdeUnallocatedEntry;
#ifdef __SMP__ // we need to also unmap the page table in case thread is running on another core...

			// Ensure other processors see the update to iAliasPde.
			BroadcastAliasRefresh();

			*thread->iAliasPdePtr = KPdeUnallocatedEntry;

			SinglePdeUpdated(thread->iAliasPdePtr);
			__NK_ASSERT_DEBUG((thread->iAliasLinAddr&KPageMask)==0);
			// Invalidate the tlb for the page using os asid of the process that created the alias
			// this is safe as the os asid will be valid as thread must be running otherwise the alias
			// would have been removed.
			InvalidateTLBForPage(thread->iAliasLinAddr | ((DMemModelProcess*)thread->iOwningProcess)->OsAsid());
			// note, race condition with 'thread' updating its iAliasLinAddr is
			// not a problem because 'thread' will not the be accessing the aliased
			// region and will take care of invalidating the TLB.
#endif
			}
		MmuLock::Flash();
		}

	// copy checkedList back to iAliasList
	iAliasList.MoveFrom(&checkedList);

	MmuLock::Unlock();
	}


void DMemModelThread::RefreshAlias()
	{
	if(iAliasLinAddr)
		{
		TRACE2(("Thread %O RefreshAlias", this));
		// Get the os asid, this is the current thread so no need to open a reference.
		TUint thisAsid = ((DMemModelProcess*)iOwningProcess)->OsAsid();
		MmuLock::Lock();
		TInt osAsid = iAliasProcess->OsAsid();
		TPde pde = *Mmu::PageDirectoryEntry(osAsid,iAliasTarget);
		iAliasPde = pde;
		*iAliasPdePtr = pde;
		SinglePdeUpdated(iAliasPdePtr);
		InvalidateTLBForPage(iAliasLinAddr|thisAsid);
		MmuLock::Unlock();
		}
	}



//
// Mapping/unmapping functions
//


/**
Modify page table entries (PTEs) so they map the given memory pages.
Entries are only updated if the current state of the corresponding page
is RPageArray::ECommitted.

@param aPtePtr		Pointer into a page table for the PTE of the first page.
@param aCount		The number of pages to modify.
@param aPages		Pointer to the entry for the first page in a memory object's #RPageArray.
					Each entry contains the physical address of a page together with its
					current state (RPageArray::TState).
@param aBlankPte	The value to use for each PTE, with the physical address component equal
					to zero.

@return False, if the page table no longer maps any entries and may be freed.
		True otherwise, to indicate that the page table is still needed.

@pre #MmuLock held.
@post #MmuLock held and has not been released by this function.
*/
TBool Mmu::MapPages(TPte* const aPtePtr, const TUint aCount, TPhysAddr* aPages, TPte aBlankPte)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aCount);
 	__NK_ASSERT_DEBUG(aBlankPte!=KPteUnallocatedEntry);

	TUint count = 0;
	if(aCount==1)
		{
		// get page to map...
		TPhysAddr pagePhys = *aPages;
		TPte pte = *aPtePtr;
		if(!RPageArray::TargetStateIsCommitted(pagePhys))
			goto done; // page no longer needs mapping

		// clear type flags...
		pagePhys &= ~KPageMask;
	
		// check nobody has already mapped the page...
		if(pte!=KPteUnallocatedEntry)
			{
			// already mapped...
#ifdef _DEBUG
			if((pte^pagePhys)>=TPte(KPageSize))
				{
				// but different!
				Kern::Printf("Mmu::MapPages already mapped %x->%x",pagePhys,pte);
				__NK_ASSERT_DEBUG(0);
				}
#endif
			return true; // return true to keep page table (it already had at least page mapped)
			}

		// map page...
		pte = pagePhys|aBlankPte;
		TRACE2(("!PTE %x=%x",aPtePtr,pte));
		*aPtePtr = pte;
		count = 1;

		// clean cache...
		CacheMaintenance::SinglePteUpdated((TLinAddr)aPtePtr);
		}
	else
		{
		// check we are only updating a single page table...
		__NK_ASSERT_DEBUG(((TLinAddr(aPtePtr)^TLinAddr(aPtePtr+aCount-1))>>KPageTableShift)==0);

		// map pages...
		TPte* pPte = aPtePtr;
		TPte* pPteEnd = aPtePtr+aCount;
		do
			{
			// map page...
			TPhysAddr pagePhys = *aPages++;
			TPte pte = *pPte++;
			if(RPageArray::TargetStateIsCommitted(pagePhys))
				{
				// clear type flags...
				pagePhys &= ~KPageMask;

				// page not being freed, so try and map it...
				if(pte!=KPteUnallocatedEntry)
					{
					// already mapped...
#ifdef _DEBUG
					if((pte^pagePhys)>=TPte(KPageSize))
						{
						// but different!
						Kern::Printf("Mmu::MapPages already mapped %x->%x",pagePhys,pte);
						__NK_ASSERT_DEBUG(0);
						}
#endif
					}
				else
					{
					// map page...
					pte = pagePhys|aBlankPte;
					TRACE2(("!PTE %x=%x",pPte-1,pte));
					pPte[-1] = pte;
					++count;
					}
				}
			}
		while(pPte!=pPteEnd);

		// clean cache...
		CacheMaintenance::MultiplePtesUpdated((TLinAddr)aPtePtr,(TLinAddr)pPte-(TLinAddr)aPtePtr);
		}

done:
	// update page counts...
	SPageTableInfo* pti = SPageTableInfo::FromPtPtr(aPtePtr);
	count = pti->IncPageCount(count);
	TRACE2(("pt %x page count=%d",TLinAddr(aPtePtr)&~KPageTableMask,pti->PageCount()));
	__NK_ASSERT_DEBUG(pti->CheckPageCount());

	// see if page table needs freeing...
	TUint keepPt = count | pti->PermanenceCount();

	__NK_ASSERT_DEBUG(!pti->IsDemandPaged()); // check not demand paged page table

	return keepPt;
	}


/**
Modify page table entries (PTEs) so they map a new page.
Entries are only updated if the current state of the corresponding page
is RPageArray::ECommitted or RPageArray::EMoving.

@param aPtePtr		Pointer into a page table for the PTE of the page.
@param aPage		Pointer to the entry for the page in a memory object's #RPageArray.
					The entry contains the physical address of a page together with its
					current state (RPageArray::TState).
@param aBlankPte	The value to use for each PTE, with the physical address component equal
					to zero.

@pre #MmuLock held.
@post #MmuLock held and has not been released by this function.
*/
void Mmu::RemapPage(TPte* const aPtePtr, TPhysAddr& aPage, TPte aBlankPte)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
 	__NK_ASSERT_DEBUG(aBlankPte!=KPteUnallocatedEntry);

	// get page to remap...
	TPhysAddr pagePhys = aPage;

	// Only remap the page if it is committed or it is being moved and
	// no other operation has been performed on the page.
	if(!RPageArray::TargetStateIsCommitted(pagePhys))
		return; // page no longer needs mapping

	// Only remap the page if it is currently mapped, i.e. doesn't have an unallocated pte.
	// This will only be true if a new mapping is being added but it hasn't yet updated 
	// all the ptes for the pages that it maps.
	TPte pte = *aPtePtr;
	if (pte == KPteUnallocatedEntry)
		return;

	// clear type flags...
	pagePhys &= ~KPageMask;

	// Get the SPageInfo of the page to map.  Allow pages without SPageInfos to
	// be mapped as when freeing a shadow page may need to remap an unpaged ROM 
	// page which won't have an SPageInfo.
	SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pagePhys);
	if (pi)
		{
		SPageInfo::TPagedState pagedState = pi->PagedState();
		if (pagedState != SPageInfo::EUnpaged)
			{
			// For paged pages only update the pte if the pte points to the wrong physical
			// address or the page is pinned.
			if (pagedState != SPageInfo::EPagedPinned)
				{
				if ((pte^pagePhys) < TPte(KPageSize))
					return;
				if (Mmu::IsPteInaccessible(pte))
					{
					// Updating this pte shouldn't be necessary but it stops random data 
					// corruption in stressed cases???
					Mmu::MakePteInaccessible(aBlankPte, EFalse);
					}
				else if (!pi->IsDirty())
					{
					// Ensure that the page is mapped as read only to prevent pages being writable 
					// without having been marked dirty.
					Mmu::MakePteInaccessible(aBlankPte, ETrue);
					}
				}
			else if (!pi->IsDirty())
				{
				// Ensure that the page is mapped as read only to prevent pages being writable 
				// without having been marked dirty.
				Mmu::MakePteInaccessible(aBlankPte, ETrue);
				}
			}
		}

	// Map the page in the page array entry as this is always the physical
	// page that the memory object's page should be mapped to.
	pte = pagePhys|aBlankPte;
	TRACE2(("!PTE %x=%x",aPtePtr,pte));
	*aPtePtr = pte;

	// clean cache...
	CacheMaintenance::SinglePteUpdated((TLinAddr)aPtePtr);
	}


/**
Modify page table entries (PTEs) so they no longer map any memory pages.

@param aPtePtr		Pointer into a page table for the PTE of the first page.
@param aCount		The number of pages to modify.

@return False, if the page table no longer maps any entries and may be freed.
		True otherwise, to indicate that the page table is still needed.

@pre #MmuLock held.
@post #MmuLock held and has not been released by this function.
*/
TBool Mmu::UnmapPages(TPte* const aPtePtr, TUint aCount)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aCount);

	TUint count = 0;
	if(aCount==1)
		{
		if(*aPtePtr==KPteUnallocatedEntry)
			return true; // page already unmapped

		// unmap page...
		++count;
		TPte pte = KPteUnallocatedEntry;
		TRACE2(("!PTE %x=%x",aPtePtr,pte));
		*aPtePtr = pte;

		// clean cache...
		CacheMaintenance::SinglePteUpdated((TLinAddr)aPtePtr);
		}
	else
		{
		// check we are only updating a single page table...
		__NK_ASSERT_DEBUG(((TLinAddr(aPtePtr)^TLinAddr(aPtePtr+aCount-1))>>KPageTableShift)==0);

		// unmap pages...
		TPte* pPte = aPtePtr;
		TPte* pPteEnd = aPtePtr+aCount;
		do
			{
			if(*pPte!=KPteUnallocatedEntry)
				{
				// unmap page...
				++count;
				TPte pte = KPteUnallocatedEntry;
				TRACE2(("!PTE %x=%x",pPte,pte));
				*pPte = pte;
				}
			}
		while(++pPte<pPteEnd);

		if(!count)
			return true; // no PTEs changed, so nothing more to do

		// clean cache...
		CacheMaintenance::MultiplePtesUpdated((TLinAddr)aPtePtr,(TLinAddr)pPte-(TLinAddr)aPtePtr);
		}

	// update page table info...
	SPageTableInfo* pti = SPageTableInfo::FromPtPtr(aPtePtr);
	count = pti->DecPageCount(count);
	TRACE2(("pt %x page count=%d",TLinAddr(aPtePtr)&~KPageTableMask,count));
	__NK_ASSERT_DEBUG(pti->CheckPageCount());

	// see if page table needs freeing...
	TUint keepPt = count | pti->PermanenceCount();

	return keepPt;
	}


/**
Modify page table entries (PTEs) so they no longer map the given memory pages.
Entries are only updated if the current state of the corresponding page
is 'decommitted' i.e. RPageArray::TargetStateIsDecommitted returns true.

@param aPtePtr		Pointer into a page table for the PTE of the first page.
@param aCount		The number of pages to modify.
@param aPages		Pointer to the entry for the first page in a memory object's #RPageArray.
					Each entry contains the physical address of a page together with its
					current state (RPageArray::TState).

@return False, if the page table no longer maps any entries and may be freed.
		True otherwise, to indicate that the page table is still needed.

@pre #MmuLock held.
@post #MmuLock held and has not been released by this function.
*/
TBool Mmu::UnmapPages(TPte* const aPtePtr, TUint aCount, TPhysAddr* aPages)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aCount);

	TUint count = 0;
	if(aCount==1)
		{
		if(*aPtePtr==KPteUnallocatedEntry)
			return true; // page already unmapped

		if(!RPageArray::TargetStateIsDecommitted(*aPages))
			return true; // page has been reallocated

		// unmap page...
		++count;
		TPte pte = KPteUnallocatedEntry;
		TRACE2(("!PTE %x=%x",aPtePtr,pte));
		*aPtePtr = pte;

		// clean cache...
		CacheMaintenance::SinglePteUpdated((TLinAddr)aPtePtr);
		}
	else
		{
		// check we are only updating a single page table...
		__NK_ASSERT_DEBUG(((TLinAddr(aPtePtr)^TLinAddr(aPtePtr+aCount-1))>>KPageTableShift)==0);

		// unmap pages...
		TPte* pPte = aPtePtr;
		TPte* pPteEnd = aPtePtr+aCount;
		do
			{
			if(RPageArray::TargetStateIsDecommitted(*aPages++) && *pPte!=KPteUnallocatedEntry)
				{
				// unmap page...
				++count;
				TPte pte = KPteUnallocatedEntry;
				TRACE2(("!PTE %x=%x",pPte,pte));
				*pPte = pte;
				}
			}
		while(++pPte<pPteEnd);

		if(!count)
			return true; // no PTEs changed, so nothing more to do

		// clean cache...
		CacheMaintenance::MultiplePtesUpdated((TLinAddr)aPtePtr,(TLinAddr)pPte-(TLinAddr)aPtePtr);
		}

	// update page table info...
	SPageTableInfo* pti = SPageTableInfo::FromPtPtr(aPtePtr);
	count = pti->DecPageCount(count);
	TRACE2(("pt %x page count=%d",TLinAddr(aPtePtr)&~KPageTableMask,count));
	__NK_ASSERT_DEBUG(pti->CheckPageCount());

	// see if page table needs freeing...
	TUint keepPt = count | pti->PermanenceCount();

	return keepPt;
	}


/**
Modify page table entries (PTEs) so the given memory pages are not accessible.
Entries are only updated if the current state of the corresponding page
is RPageArray::ERestrictingNA.

@param aPtePtr		Pointer into a page table for the PTE of the first page.
@param aCount		The number of pages to modify.
@param aPages		Pointer to the entry for the first page in a memory object's #RPageArray.
					Each entry contains the physical address of a page together with its
					current state (RPageArray::TState).

@pre #MmuLock held.
@post #MmuLock held and has not been released by this function.
*/
void Mmu::RestrictPagesNA(TPte* const aPtePtr, TUint aCount, TPhysAddr* aPages)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aCount);

	if(aCount==1)
		{
		TPhysAddr page = *aPages;
		TPte pte = *aPtePtr;
		RPageArray::TState state = RPageArray::State(page);
		if(state != RPageArray::ERestrictingNA && state != RPageArray::EMoving)
			return; // page no longer needs restricting

		if(pte==KPteUnallocatedEntry)
			return; // page gone

		// restrict page...
		pte = Mmu::MakePteInaccessible(pte,false);
		TRACE2(("!PTE %x=%x",aPtePtr,pte));
		*aPtePtr = pte;

		// clean cache...
		CacheMaintenance::SinglePteUpdated((TLinAddr)aPtePtr);
		}
	else
		{
		// check we are only updating a single page table...
		__NK_ASSERT_DEBUG(((TLinAddr(aPtePtr)^TLinAddr(aPtePtr+aCount-1))>>KPageTableShift)==0);

		// restrict pages...
		TPte* pPte = aPtePtr;
		TPte* pPteEnd = aPtePtr+aCount;
		do
			{
			TPhysAddr page = *aPages++;
			TPte pte = *pPte++;
			if(RPageArray::State(page)==RPageArray::ERestrictingNA && pte!=KPteUnallocatedEntry)
				{
				pte = Mmu::MakePteInaccessible(pte,false);
				TRACE2(("!PTE %x=%x",pPte-1,pte));
				pPte[-1] = pte;
				}
			}
		while(pPte<pPteEnd);

		// clean cache...
		CacheMaintenance::MultiplePtesUpdated((TLinAddr)aPtePtr,(TLinAddr)pPte-(TLinAddr)aPtePtr);
		}
	}


/**
Modify page table entries (PTEs) so they map the given demand paged memory pages.

Entries are only updated if the current state of the corresponding page
is RPageArray::ECommitted.

This function is used for demand paged memory when handling a page fault or
memory pinning operation. It will widen the access permission of existing entries
if required to match \a aBlankPte and will 'rejuvenate' the page table.

@param aPtePtr		Pointer into a page table for the PTE of the first page.
@param aCount		The number of pages to modify.
@param aPages		Pointer to the entry for the first page in a memory object's #RPageArray.
					Each entry contains the physical address of a page together with its
					current state (RPageArray::TState).
@param aBlankPte	The value to use for each PTE, with the physical address component equal
					to zero.

@return False, if the page table no longer maps any entries and may be freed.
		True otherwise, to indicate that the page table is still needed.

@pre #MmuLock held.
@post MmuLock held (but may have been released by this function)
*/
TBool Mmu::PageInPages(TPte* const aPtePtr, const TUint aCount, TPhysAddr* aPages, TPte aBlankPte)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aCount);
	__NK_ASSERT_DEBUG(aBlankPte!=KPteUnallocatedEntry);

	TUint count = 0;

	if(aCount==1)
		{
		// get page to map...
		TPhysAddr page = *aPages;
		TPte pte = *aPtePtr;
		if(!RPageArray::TargetStateIsCommitted(page))
			goto done; // page no longer needs mapping

#ifdef _DEBUG
		if(pte!=KPteUnallocatedEntry)
			{
			if ((pte^page)>=TPte(KPageSize) && !Mmu::IsPteInaccessible(pte) &&
				!Mmu::IsPteReadOnly(pte))
				{
				// Page has been mapped before but the physical address is different
				// and the page hasn't been moved as it is not inaccessible.
				Kern::Printf("Mmu::PageInPages already mapped %x->%x",page,pte);
				__NK_ASSERT_DEBUG(0);
				}
			}
#endif
		if(!Mmu::IsPteMoreAccessible(aBlankPte,pte))
			{
			__NK_ASSERT_DEBUG((pte^page) < (TUint)KPageSize); // Must be the same physical addr.
			return true; // return true to keep page table (it already had at least page mapped)
			}

		// remap page with new increased permissions...
		if(pte==KPteUnallocatedEntry)
			count = 1; // we'll be adding a new pte entry, count it
		if(!Mmu::IsPteReadOnly(aBlankPte))
			ThePager.SetWritable(*SPageInfo::FromPhysAddr(page));
		pte = (page&~KPageMask)|aBlankPte;
		TRACE2(("!PTE %x=%x",aPtePtr,pte));
		*aPtePtr = pte;

		// clean cache...
		CacheMaintenance::SinglePteUpdated((TLinAddr)aPtePtr);
		}
	else
		{
		// check we are only updating a single page table...
		__NK_ASSERT_DEBUG(((TLinAddr(aPtePtr)^TLinAddr(aPtePtr+aCount-1))>>KPageTableShift)==0);

		// map pages...
		TPte* pPte = aPtePtr;
		TPte* pPteEnd = aPtePtr+aCount;
		do
			{
			// map page...
			TPhysAddr page = *aPages++;
			TPte pte = *pPte++;
			if(RPageArray::TargetStateIsCommitted(page))
				{
#ifdef _DEBUG
				if(pte!=KPteUnallocatedEntry)
					{
					if ((pte^page)>=TPte(KPageSize) && !Mmu::IsPteInaccessible(pte) &&
						!Mmu::IsPteReadOnly(pte))
						{
						// Page has been mapped before but the physical address is different
						// and the page hasn't been moved as it is not inaccessible.
						Kern::Printf("Mmu::PageInPages already mapped %x->%x",page,pte);
						__NK_ASSERT_DEBUG(0);
						}
					}
#endif
				if(Mmu::IsPteMoreAccessible(aBlankPte,pte))
					{
					// remap page with new increased permissions...
					if(pte==KPteUnallocatedEntry)
						++count; // we'll be adding a new pte entry, count it
					if(!Mmu::IsPteReadOnly(aBlankPte))
						ThePager.SetWritable(*SPageInfo::FromPhysAddr(page));
					pte = (page&~KPageMask)|aBlankPte;
					TRACE2(("!PTE %x=%x",pPte-1,pte));
					pPte[-1] = pte;
					}
				else
					__NK_ASSERT_DEBUG((pte^page) < (TUint)KPageSize); // Must be the same physical addr.	
				}
			}
		while(pPte!=pPteEnd);

		// clean cache...
		CacheMaintenance::MultiplePtesUpdated((TLinAddr)aPtePtr,(TLinAddr)pPte-(TLinAddr)aPtePtr);
		}

done:
	// update page counts...
	SPageTableInfo* pti = SPageTableInfo::FromPtPtr(aPtePtr);
	count = pti->IncPageCount(count);
	TRACE2(("pt %x page count=%d",TLinAddr(aPtePtr)&~KPageTableMask,pti->PageCount()));
	__NK_ASSERT_DEBUG(pti->CheckPageCount());

	// see if page table needs freeing...
	TUint keepPt = count | pti->PermanenceCount();

	// rejuvenate demand paged page tables...
	ThePager.RejuvenatePageTable(aPtePtr);

	return keepPt;
	}


//
// CodeModifier
//

#ifdef __DEBUGGER_SUPPORT__

void DoWriteCode(TUint32* aAddress, TUint32 aValue);

#ifdef __SMP__

extern "C" void __e32_instruction_barrier();

class TCodeModifierBroadcast : public TGenericIPI
	{
public:
	TCodeModifierBroadcast(TUint32* aAddress, TUint32 aValue);
	static void Isr(TGenericIPI*);
	void Go();
public:
	TUint32*		iAddress;
	TUint32			iValue;
	volatile TInt	iFlag;
	};

TCodeModifierBroadcast::TCodeModifierBroadcast(TUint32* aAddress, TUint32 aValue)
	:	iAddress(aAddress), iValue(aValue), iFlag(0)
	{
	}

void TCodeModifierBroadcast::Isr(TGenericIPI* aPtr)
	{
	TCodeModifierBroadcast& a = *(TCodeModifierBroadcast*)aPtr;
	while (!__e32_atomic_load_acq32(&a.iFlag))
		__chill();
#ifdef __BROADCAST_CACHE_MAINTENANCE__
	CacheMaintenance::CodeChanged((TLinAddr)a.iAddress, sizeof (TInt), CacheMaintenance::ECodeModifier);	// need to do separate Clean-D, Purge-I on each core
#else
	__e32_instruction_barrier();		// synchronize instruction execution
#endif
	}

void TCodeModifierBroadcast::Go()
	{
	NKern::Lock();
	QueueAllOther(&Isr);
	WaitEntry();					// wait for other cores to stop
	DoWriteCode(iAddress, iValue);
	iFlag = 1;
	__e32_instruction_barrier();	// synchronize instruction execution
	WaitCompletion();				// wait for other cores to resume
	NKern::Unlock();
	}
#endif

/**
@pre Calling thread must be in critical section
@pre CodeSeg mutex held
*/
TInt CodeModifier::SafeWriteCode(DProcess* aProcess, TLinAddr aAddress, TInt aSize, TUint aValue, void* aOldValue)
	{
	__ASSERT_CRITICAL;
	Mmu& m=TheMmu;
	RamAllocLock::Lock();
	MmuLock::Lock();
	__UNLOCK_GUARD_START(MmuLock);

	// Check aProcess is still alive by opening a reference on its os asid.
	TInt osAsid = ((DMemModelProcess*)aProcess)->TryOpenOsAsid();
	if (osAsid < 0)
		{
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("CodeModifier::SafeWriteCode - zombie process"));
		__UNLOCK_GUARD_END(MmuLock);
		MmuLock::Unlock();
		RamAllocLock::Unlock();
		return KErrBadDescriptor;
		}

	// Find physical address of the page, the breakpoint belongs to
	TPhysAddr physAddr = Mmu::LinearToPhysical(aAddress, osAsid);
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("CodeModifier::SafeWriteCode - PA:%x", physAddr));


	if (physAddr==KPhysAddrInvalid)
		{
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("CodeModifier::SafeWriteCode - invalid VA"));
		__UNLOCK_GUARD_END(MmuLock);
		MmuLock::Unlock();
		RamAllocLock::Unlock();
		// The os asid is no longer required.
		((DMemModelProcess*)aProcess)->CloseOsAsid();
		return KErrBadDescriptor;
		}

	// Temporary map physical page
	TLinAddr tempAddr = m.MapTemp(physAddr&~KPageMask, aAddress>>KPageShift);
	tempAddr |=  aAddress & KPageMask;
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("CodeModifier::SafeWriteCode - tempAddr:%x",tempAddr));

	TInt r = KErrBadDescriptor;
	TUint32* ptr = (TUint32*)(tempAddr&~3);
	TUint32 oldWord;

	if(Kern::SafeRead(ptr,&oldWord,sizeof(oldWord))==0 // safely read the original value...
		&& Kern::SafeWrite(ptr,&oldWord,sizeof(oldWord))==0 ) // and write it back
		{
		// We have successfully probed the memory by reading and writing to it
		// so we assume it is now safe to access without generating exceptions.
		// If this is wrong it will kill the system horribly.

		TUint32 newWord;
		TUint badAlign;
		TUint shift = (aAddress&3)*8;

		switch(aSize)
			{
		case 1: // 1 byte value
			badAlign = 0;
			*(TUint8*)aOldValue = oldWord>>shift;
			newWord = (oldWord&~(0xff<<shift)) | ((aValue&0xff)<<shift);
			break;

		case 2: // 2 byte value
			badAlign = tempAddr&1;
			if(!badAlign)
				*(TUint16*)aOldValue = oldWord>>shift;
			newWord = (oldWord&~(0xffff<<shift)) | ((aValue&0xffff)<<shift);
			break;

		default: // 4 byte value
			badAlign = tempAddr&3;
			if(!badAlign)
				*(TUint32*)aOldValue = oldWord;
			newWord = aValue;
			break;
			}

		if(!badAlign)
			{
			// write the new value...
#ifdef __SMP__
			TCodeModifierBroadcast b(ptr, newWord);
			b.Go();
#else
			DoWriteCode(ptr, newWord);
#endif
			r = KErrNone;
			}
		}

	__UNLOCK_GUARD_END(MmuLock);
	m.UnmapTemp();
	MmuLock::Unlock();
	RamAllocLock::Unlock();
	// The os asid is no longer required.
	((DMemModelProcess*)aProcess)->CloseOsAsid();
	return r;
	}

/**
@pre Calling thread must be in critical section
@pre CodeSeg mutex held
*/
void DoWriteCode(TUint32* aAddress, TUint32 aValue)
	{
	// We do not want to be interrupted by e.g. ISR that will run altered code before IMB-Range.
	// Therefore, copy data and clean/invalidate caches with interrupts disabled.
	TInt irq = NKern::DisableAllInterrupts();
	*aAddress = aValue;
	CacheMaintenance::CodeChanged((TLinAddr)aAddress, sizeof(TUint32), CacheMaintenance::ECodeModifier);
	NKern::RestoreInterrupts(irq);
	}

#endif //__DEBUGGER_SUPPORT__



//
// Virtual pinning
//

TInt M::CreateVirtualPinObject(TVirtualPinObject*& aPinObject)
	{
	aPinObject = (TVirtualPinObject*)new DVirtualPinMapping;
	return aPinObject != NULL ? KErrNone : KErrNoMemory;
	}

TInt M::PinVirtualMemory(TVirtualPinObject* aPinObject, TLinAddr aStart, TUint aSize, DThread* aThread)
	{
	__ASSERT_CRITICAL;
	TUint offsetInMapping;
	TUint mapInstanceCount;
	DMemoryMapping* mapping = MM::FindMappingInThread(	(DMemModelThread*)aThread, 
														aStart, 
														aSize, 
														offsetInMapping, 
														mapInstanceCount);
	TInt r = KErrBadDescriptor;
	if (mapping)
		{
		TInt count = ((aStart & KPageMask) + aSize + KPageMask) >> KPageShift;
		if(mapping->IsPinned())
			{
			// Mapping for specified virtual address is pinned so we don't need to
			// do anything. Also, we can't safely pin the memory in this case
			// anyway, as pinned mappings may move between memory objects
			r = KErrNone;
			}
		else
			{
			MmuLock::Lock();
			DMemoryObject* memory = mapping->Memory();
			if (mapInstanceCount != mapping->MapInstanceCount() || 
				!memory || !memory->IsDemandPaged())
				{
				// mapping has been reused, no memory, or it's not paged, so no need to pin...
				MmuLock::Unlock();
				r = KErrNone;
				}
			else
				{
				// paged memory needs pinning...
				// Open a reference on the memory so it doesn't get deleted.
				memory->Open();
				MmuLock::Unlock();

				TUint startInMemory = (offsetInMapping >> KPageShift) + mapping->iStartIndex;
				r = ((DVirtualPinMapping*)aPinObject)->Pin(	memory, startInMemory, count, mapping->Permissions(),
															mapping, mapInstanceCount);
				memory->Close();
				}
			}
		mapping->Close();
		}	
	return r;
	}

TInt M::CreateAndPinVirtualMemory(TVirtualPinObject*& aPinObject, TLinAddr aStart, TUint aSize)
	{
	__ASSERT_CRITICAL;
	aPinObject = 0;
	TUint offsetInMapping;
	TUint mapInstanceCount;
	DMemoryMapping* mapping = MM::FindMappingInThread(	(DMemModelThread*)&Kern::CurrentThread(), 
														aStart, 
														aSize, 
														offsetInMapping,
														mapInstanceCount);
	TInt r = KErrBadDescriptor;
	if (mapping)
		{
		TInt count = ((aStart & KPageMask) + aSize + KPageMask) >> KPageShift;
		if(mapping->IsPinned())
			{
			// Mapping for specified virtual address is pinned so we don't need to
			// do anything. Also, we can't safely pin the memory in this case
			// anyway, as pinned mappings may move between memory objects
			r = KErrNone;
			}
		else
			{
			MmuLock::Lock();
			DMemoryObject* memory = mapping->Memory();
			if (mapInstanceCount != mapping->MapInstanceCount() || 
				!memory || !memory->IsDemandPaged())
				{
				// mapping has been reused, no memory, or it's not paged, so no need to pin...
				MmuLock::Unlock();
				r = KErrNone;
				}
			else
				{// The memory is demand paged so create a pin object and pin it.
				// Open a reference on the memory so it doesn't get deleted.
				memory->Open();
				MmuLock::Unlock();
				r = CreateVirtualPinObject(aPinObject);
				if (r == KErrNone)
					{
					TUint startInMemory = (offsetInMapping >> KPageShift) + mapping->iStartIndex;
					r = ((DVirtualPinMapping*)aPinObject)->Pin(	memory, startInMemory, count, mapping->Permissions(), 
																mapping, mapInstanceCount);
					if (r != KErrNone)
						{// Failed to pin the memory so pin object is not required.
						DestroyVirtualPinObject(aPinObject);
						}
					}
				memory->Close();
				}
			}
		mapping->Close();
		}	
	return r;
	}

void M::UnpinVirtualMemory(TVirtualPinObject* aPinObject)
	{
	DVirtualPinMapping* mapping = (DVirtualPinMapping*)aPinObject;
	if (mapping->IsAttached())
		mapping->Unpin();
	}
	
void M::DestroyVirtualPinObject(TVirtualPinObject*& aPinObject)
	{
	DVirtualPinMapping* mapping = (DVirtualPinMapping*)__e32_atomic_swp_ord_ptr(&aPinObject, 0);
	if (mapping)
		{
		if (mapping->IsAttached())
			mapping->Unpin();
		mapping->AsyncClose();
		}
	}

//
// Physical pinning
//

TInt M::CreatePhysicalPinObject(TPhysicalPinObject*& aPinObject)
	{
	aPinObject = (TPhysicalPinObject*)new DPhysicalPinMapping;
	return aPinObject != NULL ? KErrNone : KErrNoMemory;
	}

TInt M::PinPhysicalMemory(TPhysicalPinObject* aPinObject, TLinAddr aStart, TUint aSize, TBool aReadOnly,
				TPhysAddr& aAddress, TPhysAddr* aPages, TUint32& aMapAttr, TUint& aColour, DThread* aThread)
	{
	__ASSERT_CRITICAL;
	TUint offsetInMapping;
	TUint mapInstanceCount;
	DMemoryMapping* mapping = MM::FindMappingInThread(	(DMemModelThread*)aThread, 
														aStart, 
														aSize, 
														offsetInMapping, 
														mapInstanceCount);
	TInt r = KErrBadDescriptor;
	if (mapping)
		{
		TInt count = ((aStart & KPageMask) + aSize + KPageMask) >> KPageShift;

		MmuLock::Lock();
		DMemoryObject* memory = mapping->Memory();
		if (mapInstanceCount == mapping->MapInstanceCount() && memory)
			{
			memory->Open();
			MmuLock::Unlock();

			TUint startInMemory = (offsetInMapping >> KPageShift) + mapping->iStartIndex;
			TMappingPermissions permissions = aReadOnly ? ESupervisorReadOnly : ESupervisorReadWrite;
			r = ((DPhysicalPinMapping*)aPinObject)->Pin(memory, startInMemory, count, permissions);
			if (r == KErrNone)
				{
				r = ((DPhysicalPinMapping*)aPinObject)->PhysAddr(0, count, aAddress, aPages);
				if (r>=KErrNone)
					{
					r = KErrNone; //Do not report discontiguous memory in return value.
					const TMappingAttributes2& mapAttr2 =
											MM::LegacyMappingAttributes(memory->Attributes(), mapping->Permissions());
					*(TMappingAttributes2*)&aMapAttr = mapAttr2;
					}
				else
					UnpinPhysicalMemory(aPinObject);
				}
			memory->Close();
			}
		else // mapping has been reused or no memory...
			{
			MmuLock::Unlock();
			}
		mapping->Close();
		}
	aColour = (aStart >>KPageShift) & KPageColourMask;
	return r;
	}

void M::UnpinPhysicalMemory(TPhysicalPinObject* aPinObject)
	{
	DPhysicalPinMapping* mapping = (DPhysicalPinMapping*)aPinObject;
	if (mapping->IsAttached())
		mapping->Unpin();
	}

void M::DestroyPhysicalPinObject(TPhysicalPinObject*& aPinObject)
	{
	DPhysicalPinMapping* mapping = (DPhysicalPinMapping*)__e32_atomic_swp_ord_ptr(&aPinObject, 0);
	if (mapping)
		{
		if (mapping->IsAttached())
			mapping->Unpin();
		mapping->AsyncClose();
		}
	}


//
// Kernel map and pin.
//

TInt M::CreateKernelMapObject(TKernelMapObject*& aMapObject, TUint aMaxReserveSize)
	{
	DKernelPinMapping*  pinObject = new DKernelPinMapping();
	aMapObject = (TKernelMapObject*) pinObject;
	if (pinObject == NULL)
		{
		return KErrNoMemory;
		}
	// Ensure we reserve enough bytes for all possible alignments of the start and 
	// end of the region to map.
	TUint reserveBytes = aMaxReserveSize? ((aMaxReserveSize + KPageMask) & ~KPageMask) + KPageSize : 0;
	TInt r = pinObject->Construct(reserveBytes);
	if (r != KErrNone)
		{// Failed so delete the kernel mapping object.
		pinObject->Close();
		aMapObject = NULL;
		}
	return r;
	}


TInt M::MapAndPinMemory(TKernelMapObject* aMapObject, DThread* aThread, TLinAddr aStart, 
						TUint aSize, TUint aMapAttributes, TLinAddr& aKernelAddr, TPhysAddr* aPages)
	{
	__ASSERT_CRITICAL;
	TUint offsetInMapping;
	TUint mapInstanceCount;
	DMemoryMapping* mapping = MM::FindMappingInThread(	(DMemModelThread*)aThread, 
														aStart, 
														aSize, 
														offsetInMapping, 
														mapInstanceCount);
	TInt r = KErrBadDescriptor;
	if (mapping)
		{
		DKernelPinMapping* kernelMap = (DKernelPinMapping*)aMapObject;
		TInt count = (((aStart + aSize + KPageMask) & ~KPageMask) - (aStart & ~KPageMask)) >> KPageShift;
		if (kernelMap->iReservePages && kernelMap->iReservePages < count)
			{
			mapping->Close();
			return KErrArgument;
			}

		MmuLock::Lock();
		DMemoryObject* memory = mapping->Memory();
		if (mapInstanceCount == mapping->MapInstanceCount() && memory)
			{
			memory->Open();
			MmuLock::Unlock();

			TUint startInMemory = (offsetInMapping >> KPageShift) + mapping->iStartIndex;
			TBool readOnly = aMapAttributes & Kern::EKernelMap_ReadOnly;
			TMappingPermissions permissions =  readOnly ? ESupervisorReadOnly : ESupervisorReadWrite;
			r = kernelMap->MapAndPin(memory, startInMemory, count, permissions);
			if (r == KErrNone)
				{
				__NK_ASSERT_DEBUG(!kernelMap->IsUserMapping());
				aKernelAddr = kernelMap->Base();
				TPhysAddr contigAddr;	// Ignore this value as aPages will be populated 
										// whether the memory is contiguous or not.
				r = kernelMap->PhysAddr(0, count, contigAddr, aPages);
				if (r>=KErrNone)
					{
					r = KErrNone; //Do not report discontiguous memory in return value.
					}
				else
					{
					UnmapAndUnpinMemory((TKernelMapObject*)kernelMap);
					}
				}
			memory->Close();
			}
		else // mapping has been reused or no memory...
			{
			MmuLock::Unlock();
			}
		mapping->Close();
		}
	return r;
	}


void M::UnmapAndUnpinMemory(TKernelMapObject* aMapObject)
	{
	DKernelPinMapping* mapping = (DKernelPinMapping*)aMapObject;
	if (mapping->IsAttached())
		mapping->UnmapAndUnpin();
	}


void M::DestroyKernelMapObject(TKernelMapObject*& aMapObject)
	{
	DKernelPinMapping* mapping = (DKernelPinMapping*)__e32_atomic_swp_ord_ptr(&aMapObject, 0);
	if (mapping)
		{
		if (mapping->IsAttached())
			mapping->UnmapAndUnpin();
		mapping->AsyncClose();
		}
	}


//
// Cache sync operations
//

//@pre	As for MASK_THREAD_STANDARD
void Mmu::SyncPhysicalMemoryBeforeDmaWrite(TPhysAddr* aPages, TUint aColour, TUint aOffset, TUint aSize, TUint32 aMapAttr)
	{
	//Jump over the pages we do not have to sync
	aPages += aOffset>>KPageShift;
	aOffset &=KPageMask;
	aColour  = (aColour + (aOffset>>KPageShift)) & KPageColourMask;

	//Calculate page table entry for the temporary mapping.
	TUint pteType = PteType(ESupervisorReadWrite,true);
	TMappingAttributes2 mapAttr2(aMapAttr);
	TPte pte = Mmu::BlankPte((TMemoryAttributes)mapAttr2.Type(), pteType);
	
	while (aSize) //A single pass of loop operates within page boundaries.
		{
		TUint sizeInLoopPass = Min (KPageSize, aOffset+aSize) - aOffset; //The size of the region in this pass.

		NKern::ThreadEnterCS();
		Kern::MutexWait(*iPhysMemSyncMutex);
		
		TLinAddr linAddr = iPhysMemSyncTemp.Map(*aPages, aColour, pte);
		CacheMaintenance::MakeCPUChangesVisible(linAddr+aOffset, sizeInLoopPass, aMapAttr, *aPages+aOffset);
		iPhysMemSyncTemp.Unmap();
		
		Kern::MutexSignal(*iPhysMemSyncMutex);
		NKern::ThreadLeaveCS();

		aSize-=sizeInLoopPass;  // Remaining bytes to sync
		aOffset=0;				// In all the pages after the first, sync will always start with zero offset.
		aPages++;	// Point to the next page
		aColour  = (aColour+1) & KPageColourMask;
		}
	}

//@pre	As for MASK_THREAD_STANDARD
void Mmu::SyncPhysicalMemoryBeforeDmaRead(TPhysAddr* aPages, TUint aColour, TUint aOffset, TUint aSize, TUint32 aMapAttr)
	{
	//Jump over the pages we do not have to sync
	aPages += aOffset>>KPageShift;
	aOffset &=KPageMask;
	aColour  = (aColour + (aOffset>>KPageShift)) & KPageColourMask;

	//Calculate page table entry for the temporary mapping.
	TUint pteType = PteType(ESupervisorReadWrite,true);
	TMappingAttributes2 mapAttr2(aMapAttr);
	TPte pte = Mmu::BlankPte((TMemoryAttributes)mapAttr2.Type(), pteType);
	
	while (aSize) //A single pass of loop operates within page boundaries.
		{
		TUint sizeInLoopPass = Min (KPageSize, aOffset+aSize) - aOffset; //The size of the region in this pass.

		NKern::ThreadEnterCS();
		Kern::MutexWait(*iPhysMemSyncMutex);
		
		TLinAddr linAddr = iPhysMemSyncTemp.Map(*aPages, aColour, pte);
		CacheMaintenance::PrepareMemoryForExternalWrites(linAddr+aOffset, sizeInLoopPass, aMapAttr, *aPages+aOffset);
		iPhysMemSyncTemp.Unmap();
		
		Kern::MutexSignal(*iPhysMemSyncMutex);
		NKern::ThreadLeaveCS();

		aSize-=sizeInLoopPass;  // Remaining bytes to sync
		aOffset=0;				// In all the pages after the first, sync will always start with zero offset.
		aPages++;	// Point to the next page
		aColour  = (aColour+1) & KPageColourMask;
		}
	}

//@pre	As for MASK_THREAD_STANDARD
void Mmu::SyncPhysicalMemoryAfterDmaRead(TPhysAddr* aPages, TUint aColour, TUint aOffset, TUint aSize, TUint32 aMapAttr)
	{
	//Jump over the pages we do not have to sync
	aPages += aOffset>>KPageShift;
	aOffset &=KPageMask;
	aColour  = (aColour + (aOffset>>KPageShift)) & KPageColourMask;

	//Calculate page table entry for the temporary mapping.
	TUint pteType = PteType(ESupervisorReadWrite,true);
	TMappingAttributes2 mapAttr2(aMapAttr);
	TPte pte = Mmu::BlankPte((TMemoryAttributes)mapAttr2.Type(), pteType);
	
	while (aSize) //A single pass of loop operates within page boundaries.
		{
		TUint sizeInLoopPass = Min (KPageSize, aOffset+aSize) - aOffset; //The size of the region in this pass.

		NKern::ThreadEnterCS();
		Kern::MutexWait(*iPhysMemSyncMutex);
		
		TLinAddr linAddr = iPhysMemSyncTemp.Map(*aPages, aColour, pte);
		CacheMaintenance::MakeExternalChangesVisible(linAddr+aOffset, sizeInLoopPass, aMapAttr, *aPages+aOffset);
		iPhysMemSyncTemp.Unmap();
		
		Kern::MutexSignal(*iPhysMemSyncMutex);
		NKern::ThreadLeaveCS();

		aSize-=sizeInLoopPass;  // Remaining bytes to sync
		aOffset=0;				// In all the pages after the first, sync will always start with zero offset.
		aPages++;	// Point to the next page
		aColour  = (aColour+1) & KPageColourMask;
		}
	}

EXPORT_C TInt Cache::SyncPhysicalMemoryBeforeDmaWrite(TPhysAddr* aPages, TUint aColour, TUint aOffset, TUint aSize, TUint32 aMapAttr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncPhysicalMemoryBeforeDmaWrite");
	TheMmu.SyncPhysicalMemoryBeforeDmaWrite(aPages, aColour, aOffset, aSize, aMapAttr);
	return KErrNone;
	}

EXPORT_C TInt Cache::SyncPhysicalMemoryBeforeDmaRead(TPhysAddr* aPages, TUint aColour, TUint aOffset, TUint aSize, TUint32 aMapAttr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncPhysicalMemoryBeforeDmaRead");
	TheMmu.SyncPhysicalMemoryBeforeDmaRead(aPages, aColour, aOffset, aSize, aMapAttr);
	return KErrNone;
	}

EXPORT_C TInt Cache::SyncPhysicalMemoryAfterDmaRead(TPhysAddr* aPages, TUint aColour, TUint aOffset, TUint aSize, TUint32 aMapAttr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncPhysicalMemoryAfterDmaRead");
	TheMmu.SyncPhysicalMemoryAfterDmaRead(aPages, aColour, aOffset, aSize, aMapAttr);
	return KErrNone;
	}
