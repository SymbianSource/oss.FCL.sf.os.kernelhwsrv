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
//

#include <x86_mem.h>
#include "cache_maintenance.inl"
#include "execs.h"
#include "mm.h"
#include "mmu.h"
#include "mpager.h"
#include "mpdalloc.h"


TPte PteGlobal;	// =0x100 on processors which support global pages, 0 on processors which don't

#if defined(KMMU)
extern "C" void __DebugMsgFlushTLB()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("FlushTLB"));
	}

extern "C" void __DebugMsgLocalFlushTLB()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("FlushTLB"));
	}

extern "C" void __DebugMsgINVLPG(int a)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("INVLPG(%08x)",a));
	}
#endif



extern void DoLocalInvalidateTLB();


#ifndef __SMP__


FORCE_INLINE void LocalInvalidateTLB()
	{
	DoLocalInvalidateTLB();
	}


#else // __SMP__


const TInt KMaxPages = 1;

class TTLBIPI : public TGenericIPI
	{
public:
	TTLBIPI();
	static void InvalidateForPagesIsr(TGenericIPI*);
	static void LocalInvalidateIsr(TGenericIPI*);
	static void InvalidateIsr(TGenericIPI*);
	static void WaitAndInvalidateIsr(TGenericIPI*);
	void AddAddress(TLinAddr aAddr);
	void InvalidateList();
public:
	volatile TInt	iFlag;
	TInt			iCount;
	TLinAddr		iAddr[KMaxPages];
	};

TTLBIPI::TTLBIPI()
	:	iFlag(0), iCount(0)
	{
	}

void TTLBIPI::LocalInvalidateIsr(TGenericIPI*)
	{
	TRACE2(("TLBLocInv"));
	DoLocalInvalidateTLB();
	}

void TTLBIPI::InvalidateIsr(TGenericIPI*)
	{
	TRACE2(("TLBInv"));
	DoInvalidateTLB();
	}

void TTLBIPI::WaitAndInvalidateIsr(TGenericIPI* aTLBIPI)
	{
	TRACE2(("TLBWtInv"));
	TTLBIPI& a = *(TTLBIPI*)aTLBIPI;
	while (!a.iFlag)
		{}
	if (a.iCount == 1)
		DoInvalidateTLBForPage(a.iAddr[0]);
	else
		DoInvalidateTLB();
	}

void TTLBIPI::InvalidateForPagesIsr(TGenericIPI* aTLBIPI)
	{
	TTLBIPI& a = *(TTLBIPI*)aTLBIPI;
	TInt i;
	for (i=0; i<a.iCount; ++i)
		{
		TRACE2(("TLBInv %08x", a.iAddr[i]));
		DoInvalidateTLBForPage(a.iAddr[i]);
		}
	}

void TTLBIPI::AddAddress(TLinAddr aAddr)
	{
	iAddr[iCount] = aAddr;
	if (++iCount == KMaxPages)
		InvalidateList();
	}

void TTLBIPI::InvalidateList()
	{
	NKern::Lock();
	InvalidateForPagesIsr(this);
	QueueAllOther(&InvalidateForPagesIsr);
	NKern::Unlock();
	WaitCompletion();
	iCount = 0;
	}

void LocalInvalidateTLB()
	{
	TTLBIPI ipi;
	NKern::Lock();
	DoLocalInvalidateTLB();
	ipi.QueueAllOther(&TTLBIPI::LocalInvalidateIsr);
	NKern::Unlock();
	ipi.WaitCompletion();
	}

void InvalidateTLB()
	{
	TTLBIPI ipi;
	NKern::Lock();
	DoInvalidateTLB();
	ipi.QueueAllOther(&TTLBIPI::InvalidateIsr);
	NKern::Unlock();
	ipi.WaitCompletion();
	}

void InvalidateTLBForPage(TLinAddr aAddr)
	{
	TTLBIPI ipi;
	ipi.AddAddress(aAddr);
	ipi.InvalidateList();
	}


#endif // __SMP__


void InvalidateTLBForAsid(TUint aAsid)
	{
	if(aAsid==KKernelOsAsid)
		InvalidateTLB();
	else
		LocalInvalidateTLB();
	}


void SinglePdeUpdated(TPde* aPde)
	{
	CacheMaintenance::SinglePdeUpdated((TLinAddr)aPde);
	PageDirectories.GlobalPdeChanged(aPde);
	}


//
// Functions for class Mmu
//

TPhysAddr Mmu::PtePhysAddr(TPte aPte, TUint /*aPteIndex*/)
	{
	if(aPte&KPdePtePresent)
		return aPte & KPdePtePhysAddrMask;
	return KPhysAddrInvalid;
	}


TPte* Mmu::PageTableFromPde(TPde aPde)
	{
	if((aPde&(KPdeLargePage|KPdePtePresent)) == KPdePtePresent)
		{
		SPageInfo* pi = SPageInfo::FromPhysAddr(aPde);
		TInt id = (pi->Index()<<KPtClusterShift) | ((aPde>>KPageTableShift)&KPtClusterMask);
		return (TPte*)(KPageTableBase+(id<<KPageTableShift));
		}
	return 0;
	}


TPte* Mmu::SafePageTableFromPde(TPde aPde)
	{
	if((aPde&(KPdeLargePage|KPdePtePresent)) == KPdePtePresent)
		{
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(aPde&~KPageMask);
		if(pi)
			{
			TInt id = (pi->Index()<<KPtClusterShift) | ((aPde>>KPageTableShift)&KPtClusterMask);
			return (TPte*)(KPageTableBase+(id<<KPageTableShift));
			}
		}
	return 0;
	}


/**
Return the base phsical address of the section table referenced by the given
Page Directory Entry (PDE) \a aPde. If the PDE doesn't refer to a
section then KPhysAddrInvalid is returned.

@pre #MmuLock held.
*/
TPhysAddr Mmu::SectionBaseFromPde(TPde aPde)
	{
	if(PdeMapsSection(aPde))
	   return aPde&KPdeLargePagePhysAddrMask;
	return KPhysAddrInvalid;
	}


TPte* Mmu::PtePtrFromLinAddr(TLinAddr aAddress, TInt aOsAsid)
	{
	TPde pde = PageDirectory(aOsAsid)[aAddress>>KChunkShift];
	SPageInfo* pi = SPageInfo::FromPhysAddr(pde);
	TPte* pt = (TPte*)(KPageTableBase+(pi->Index()<<KPageShift)+(pde&(KPageMask&~KPageTableMask)));
	pt += (aAddress>>KPageShift)&(KChunkMask>>KPageShift);
	return pt;
	}


TPte* Mmu::SafePtePtrFromLinAddr(TLinAddr aAddress, TInt aOsAsid)
	{
	TPde pde = PageDirectory(aOsAsid)[aAddress>>KChunkShift];
	TPte* pt = SafePageTableFromPde(pde);
	if(pt)
		pt += (aAddress>>KPageShift)&(KChunkMask>>KPageShift);
	return pt;
	}


TPhysAddr Mmu::PageTablePhysAddr(TPte* aPt)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld() || PageTablesLockIsHeld());

	TInt pdeIndex = ((TLinAddr)aPt)>>KChunkShift;
	TPde pde = PageDirectory(KKernelOsAsid)[pdeIndex];
	__NK_ASSERT_DEBUG((pde&(KPdePtePresent|KPdeLargePage))==KPdePtePresent);

	SPageInfo* pi = SPageInfo::FromPhysAddr(pde);
	TPte* pPte = (TPte*)(KPageTableBase+(pi->Index(true)<<KPageShift));
	TPte pte = pPte[(((TLinAddr)aPt)&KChunkMask)>>KPageShift];
	__NK_ASSERT_DEBUG(pte & KPdePtePresent);

	return pte&KPdePtePhysAddrMask;
	}


TPhysAddr Mmu::UncheckedLinearToPhysical(TLinAddr aLinAddr, TInt aOsAsid)
	{
	TRACE2(("Mmu::UncheckedLinearToPhysical(%08x,%d)",aLinAddr,aOsAsid));
	TInt pdeIndex = aLinAddr>>KChunkShift;
	TPde pde = PageDirectory(aOsAsid)[pdeIndex];
	TPhysAddr pa=KPhysAddrInvalid;
	if (pde & KPdePtePresent)
		{
		if(pde&KPdeLargePage)
			{
			pa=(pde&KPdeLargePagePhysAddrMask)+(aLinAddr&~KPdeLargePagePhysAddrMask);
			__KTRACE_OPT(KMMU,Kern::Printf("Mapped with large table - returning %08x",pa));
			}
		else
			{
			SPageInfo* pi = SPageInfo::FromPhysAddr(pde);
			TInt id = (pi->Index(true)<<KPtClusterShift) | ((pde>>KPageTableShift)&KPtClusterMask);
			TPte* pPte = (TPte*)(KPageTableBase+(id<<KPageTableShift));
			TPte pte = pPte[(aLinAddr&KChunkMask)>>KPageShift];
			if (pte & KPdePtePresent)
				{
				pa=(pte&KPdePtePhysAddrMask)+(aLinAddr&KPageMask);
				__KTRACE_OPT(KMMU,Kern::Printf("Mapped with page table - returning %08x",pa));
				}
			}
		}
	return pa;
	}


void Mmu::Init1()
	{
	TRACEB(("Mmu::Init1"));

	TUint pge = TheSuperPage().iCpuId & EX86Feat_PGE;
	PteGlobal = pge ? KPdePteGlobal : 0;
	X86_UseGlobalPTEs = pge!=0;

#ifdef __SMP__
	ApTrampolinePage = KApTrampolinePageLin;

	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		{
		TSubScheduler& ss = TheSubSchedulers[i];
		TLinAddr a = KIPCAlias + (i<<KChunkShift);
		ss.i_AliasLinAddr = (TAny*)a;
		ss.i_AliasPdePtr = (TAny*)(KPageDirectoryBase + (a>>KChunkShift)*sizeof(TPde));
		}
#endif

	Init1Common();
	}

void Mmu::Init2()
	{
	TRACEB(("Mmu::Init2"));

	Init2Common();
	}

void Mmu::Init2Final()
	{
	TRACEB(("Mmu::Init2Final"));

	Init2FinalCommon();
	}


const TPde KPdeForBlankPageTable = KPdePtePresent|KPdePteWrite|KPdePteUser;

TPde Mmu::BlankPde(TMemoryAttributes aAttributes)
	{
	(void)aAttributes;
	TPde pde = KPdeForBlankPageTable;
	TRACE2(("Mmu::BlankPde(%x) returns 0x%x",aAttributes,pde));
	return pde;
	}


TPde Mmu::BlankSectionPde(TMemoryAttributes aAttributes, TUint aPteType)
	{
	return PageToSectionEntry(BlankPte(aAttributes, aPteType), KPdeForBlankPageTable);
	}


TPte Mmu::BlankPte(TMemoryAttributes aAttributes, TUint aPteType)
	{
	TPte pte = KPdePtePresent;
	if(aPteType&EPteTypeUserAccess)
		pte |= KPdePteUser;
	if(aPteType&EPteTypeWritable)
		pte |= KPdePteWrite;
	if(aPteType&EPteTypeGlobal)
		pte |= PteGlobal;

	switch((TMemoryType)(aAttributes&EMemoryAttributeTypeMask))
		{
	case EMemAttStronglyOrdered:
	case EMemAttDevice:
	case EMemAttNormalUncached:
		pte |= KPdePteUncached;
		break;
	case EMemAttNormalCached:
		break;
	default:
		__NK_ASSERT_ALWAYS(0);
		break;
		}

	TRACE2(("Mmu::BlankPte(%x,%x) returns 0x%x",aAttributes,aPteType,pte));
	return pte;
	}


TPte Mmu::SectionToPageEntry(TPde& aPde)
	{
	TPte pte = aPde&~(KPdePtePhysAddrMask|KPdeLargePage);
	aPde = KPdeForBlankPageTable;
	return pte;
	}


TPde Mmu::PageToSectionEntry(TPte aPte, TPde /*aPde*/)
	{
	TPte pde = aPte&~KPdeLargePagePhysAddrMask;
	pde |= KPdeLargePage;
	return pde;
	}


TMemoryAttributes Mmu::CanonicalMemoryAttributes(TMemoryAttributes aAttr)
	{
	TUint attr = aAttr;
	if(attr&EMemoryAttributeDefaultShareable)
		{
		// sharing not specified, use default...
#if defined	(__CPU_USE_SHARED_MEMORY)
		attr |= EMemoryAttributeShareable;
#else
		attr &= ~EMemoryAttributeShareable;
#endif
		}

	// remove invalid attributes...
	attr &= ~(EMemoryAttributeUseECC);

	return (TMemoryAttributes)(attr&EMemoryAttributeMask);
	}


void Mmu::PagesAllocated(TPhysAddr* aPageList, TUint aCount, TRamAllocFlags aFlags, TBool aReallocate)
	{
	TRACE2(("Mmu::PagesAllocated(0x%08x,%d,0x%x,%d)",aPageList, aCount, aFlags, (bool)aReallocate));
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());

	TBool wipe = !(aFlags&EAllocNoWipe); // do we need to wipe page contents?
	TMemoryType newType = (TMemoryType)(aFlags&KMemoryTypeMask); // memory type that pages will be used for
	TUint8 wipeByte = (aFlags&EAllocUseCustomWipeByte) ? (aFlags>>EAllocWipeByteShift)&0xff : 0x03; // value to wipe memory with

	// process each page in turn...
	while(aCount--)
		{
		// get physical address of next page...
		TPhysAddr pagePhys;
		if((TPhysAddr)aPageList&1)
			{
			// aPageList is actually the physical address to use...
			pagePhys = (TPhysAddr)aPageList&~1;
			*(TPhysAddr*)&aPageList += KPageSize;
			}
		else
			pagePhys = *aPageList++;
		__NK_ASSERT_DEBUG((pagePhys&KPageMask)==0);

		// get info about page...
		SPageInfo* pi = SPageInfo::FromPhysAddr(pagePhys);
		TMemoryType oldType = (TMemoryType)(pi->Flags(true)&KMemoryTypeMask);

		TRACE2(("Mmu::PagesAllocated page=0x%08x, oldType=%d, wipe=%d",pagePhys,oldType,wipe));
		if(wipe)
			{
			// work out temporary mapping values...
			TLinAddr tempLinAddr = iTempMap[0].iLinAddr;
			TPte* tempPte = iTempMap[0].iPtePtr;

			// temporarily map page...
			*tempPte = pagePhys | iTempPteCached;
			CacheMaintenance::SinglePteUpdated((TLinAddr)tempPte);
			InvalidateTLBForPage(tempLinAddr|KKernelOsAsid);

			// wipe contents of memory...
			memset((TAny*)tempLinAddr, wipeByte, KPageSize);
			__e32_io_completion_barrier();

			// invalidate temporary mapping...
			*tempPte = KPteUnallocatedEntry;
			CacheMaintenance::SinglePteUpdated((TLinAddr)tempPte);
			InvalidateTLBForPage(tempLinAddr|KKernelOsAsid);
			}

		// indicate page has been allocated...
		if(aReallocate==false)
			pi->SetAllocated();
		}
	}


void Mmu::PageFreed(SPageInfo* aPageInfo)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	if(aPageInfo->Type()==SPageInfo::EUnused)
		return;

	aPageInfo->SetUnused();

	TRACE2(("Mmu::PageFreed page=0x%08x type=%d colour=%d",aPageInfo->PhysAddr(),aPageInfo->Flags()&KMemoryTypeMask,aPageInfo->Index()&KPageColourMask));
	}


void Mmu::CleanAndInvalidatePages(TPhysAddr* aPages, TUint aCount, TMemoryAttributes aAttributes, TUint aColour)
	{
	TMemoryType type = (TMemoryType)(aAttributes&EMemoryAttributeTypeMask);
	if(!CacheMaintenance::IsCached(type))
		{
		TRACE2(("Mmu::CleanAndInvalidatePages - nothing to do"));
		return;
		}

	RamAllocLock::Lock();

	while(aCount--)
		{
		TPhysAddr pagePhys = *aPages++;
		TRACE2(("Mmu::CleanAndInvalidatePages 0x%08x",pagePhys));

		// work out temporary mapping values...
		TLinAddr tempLinAddr = iTempMap[0].iLinAddr;
		TPte* tempPte = iTempMap[0].iPtePtr;

		// temporarily map page...
		*tempPte = pagePhys | iTempPteCached;
		CacheMaintenance::SinglePteUpdated((TLinAddr)tempPte);
		InvalidateTLBForPage(tempLinAddr|KKernelOsAsid);

		// sort out cache for memory reuse...
		CacheMaintenance::PageToPreserveAndReuse(tempLinAddr, type, KPageSize);

		// invalidate temporary mapping...
		*tempPte = KPteUnallocatedEntry;
		CacheMaintenance::SinglePteUpdated((TLinAddr)tempPte);
		InvalidateTLBForPage(tempLinAddr|KKernelOsAsid);

		RamAllocLock::Flash();
		}
	RamAllocLock::Unlock();
	}


TInt DMemModelThread::Alias(TLinAddr aAddr, DMemModelProcess* aProcess, TInt aSize, TLinAddr& aAliasAddr, TUint& aAliasSize)
//
// Set up an alias mapping starting at address aAddr in specified process.
// Note: Alias is removed if an exception is trapped by DThread::IpcExcHandler.
//
	{
	TRACE2(("Thread %O Alias %08x+%x Process %O",this,aAddr,aSize,aProcess));
	__NK_ASSERT_DEBUG(this==TheCurrentThread); // many bad things can happen if false
	// If there is an existing alias it should be on the same process otherwise
	// the os asid reference may be leaked.
	__NK_ASSERT_DEBUG(!iAliasLinAddr || aProcess == iAliasProcess);

	if(TUint(aAddr^KIPCAlias)<TUint(KIPCAliasAreaSize))
		return KErrBadDescriptor; // prevent access to alias region

#ifdef _DEBUG
	if (KDebugNum(KFORCEKUPAGEFAULTS))
		{
		TInt r = ThePager.FlushRegion(aProcess, aAddr, aSize);
		if (r != KErrNone)
			return r;
		}
#endif
	
	// Grab the mmu lock before opening a reference on os asid so that this thread 
	// is in an implicit critical section and therefore can't leak the reference by
	// dying before iAliasLinAddr is set.
	MmuLock::Lock();

	TInt osAsid;
	if (!iAliasLinAddr)
		{// There isn't any existing alias.
		// Open a reference on the aProcess's os asid so that it is not freed and/or reused
		// while we are aliasing an address belonging to it.
		osAsid = aProcess->TryOpenOsAsid();
		if (osAsid < 0)
			{// Couldn't open os asid so aProcess is no longer running.
			MmuLock::Unlock();
			return KErrBadDescriptor;
			}
		}
	else
		{
		// Just read the os asid of the process being aliased we already have a reference on it.
		osAsid = aProcess->OsAsid();
		}

	// Now we have the os asid check access to kernel memory.
	if(aAddr >= KUserMemoryLimit && osAsid != (TUint)KKernelOsAsid)
		{
		NKern::ThreadEnterCS();
		MmuLock::Unlock();
		if (!iAliasLinAddr)
			{// Close the new reference as RemoveAlias won't do as iAliasLinAddr is not set.
			aProcess->AsyncCloseOsAsid();	// Asynchronous close as this method should be quick.
			}
		NKern::ThreadLeaveCS();
		return KErrBadDescriptor; // prevent access to supervisor only memory
		}

	// Now we know all accesses to global memory are safe so check if aAddr is global.
	if(aAddr >= KGlobalMemoryBase)
		{
		// address is in global section, don't bother aliasing it...
		if (!iAliasLinAddr)
			{// Close the new reference as not required.
			NKern::ThreadEnterCS();
			MmuLock::Unlock();
			aProcess->AsyncCloseOsAsid();	// Asynchronous close as this method should be quick.
			NKern::ThreadLeaveCS();
			}
		else
			{// Remove the existing alias as it is not required.
			DoRemoveAlias(iAliasLinAddr);	// Releases mmulock.
			}
		aAliasAddr = aAddr;
		TInt maxSize = KChunkSize-(aAddr&KChunkMask);
		aAliasSize = aSize<maxSize ? aSize : maxSize;
		TRACE2(("DMemModelThread::Alias() abandoned as memory is globally mapped"));
		return KErrNone;
		}

	TPde* pd = Mmu::PageDirectory(osAsid);
	TInt pdeIndex = aAddr>>KChunkShift;
	TPde pde = pd[pdeIndex];
	// Get os asid, this is the current thread's process so no need for reference.
	TUint32 local_asid = ((DMemModelProcess*)iOwningProcess)->OsAsid();
#ifdef __SMP__
	TLinAddr aliasAddr;
#else
	TLinAddr aliasAddr = KIPCAlias+(aAddr&(KChunkMask & ~KPageMask));
#endif
	if(pde==iAliasPde && iAliasLinAddr)
		{
		// pde already aliased, so just update linear address...
#ifdef __SMP__
		__NK_ASSERT_DEBUG(iCpuRestoreCookie>=0);
		aliasAddr = iAliasLinAddr & ~KChunkMask;
		aliasAddr |= (aAddr & (KChunkMask & ~KPageMask));
#endif
		iAliasLinAddr = aliasAddr;
		}
	else
		{
		// alias PDE changed...
		if(!iAliasLinAddr)
			{
			TheMmu.iAliasList.Add(&iAliasLink); // add to list if not already aliased
#ifdef __SMP__
			__NK_ASSERT_DEBUG(iCpuRestoreCookie==-1);
			iCpuRestoreCookie = NKern::FreezeCpu();	// temporarily lock current thread to this processor
#endif
			}
		iAliasPde = pde;
		iAliasProcess = aProcess;
#ifdef __SMP__
		TSubScheduler& ss = SubScheduler();		// OK since we are locked to this CPU
		aliasAddr = TLinAddr(ss.i_AliasLinAddr) + (aAddr & (KChunkMask & ~KPageMask));
		iAliasPdePtr = (TPde*)(TLinAddr(ss.i_AliasPdePtr) + (local_asid << KPageTableShift));
#endif
		iAliasLinAddr = aliasAddr;
		*iAliasPdePtr = pde;
		}
	TRACE2(("DMemModelThread::Alias() PDEntry=%x, iAliasLinAddr=%x",pde, aliasAddr));
	LocalInvalidateTLBForPage(aliasAddr);
	TInt offset = aAddr&KPageMask;
	aAliasAddr = aliasAddr | offset;
	TInt maxSize = KPageSize - offset;
	aAliasSize = aSize<maxSize ? aSize : maxSize;
	iAliasTarget = aAddr & ~KPageMask;

	MmuLock::Unlock();

	return KErrNone;
	}


void DMemModelThread::RemoveAlias()
//
// Remove alias mapping (if present)
//
	{
	TRACE2(("Thread %O RemoveAlias", this));
	__NK_ASSERT_DEBUG(this==TheCurrentThread); // many bad things can happen if false

	TLinAddr addr = iAliasLinAddr;
	if(addr)
		{
		MmuLock::Lock();

		DoRemoveAlias(addr);	// Unlocks mmulock.
		}
	}


/**
Remove the alias mapping.

@pre Mmulock held
*/
void DMemModelThread::DoRemoveAlias(TLinAddr aAddr)
	{
	iAliasLinAddr = 0;
	iAliasPde = KPdeUnallocatedEntry;
	*iAliasPdePtr = KPdeUnallocatedEntry;
	SinglePdeUpdated(iAliasPdePtr);
	__NK_ASSERT_DEBUG((aAddr&KPageMask)==0);
	LocalInvalidateTLBForPage(aAddr);
	iAliasLink.Deque();
#ifdef __SMP__
	__NK_ASSERT_DEBUG(iCpuRestoreCookie>=0);
	NKern::EndFreezeCpu(iCpuRestoreCookie);
	iCpuRestoreCookie = -1;
#endif

	// Must close the os asid while in critical section to prevent it being 
	// leaked.  However, we can't hold the mmu lock so we have to enter an 
	// explict crtical section. It is ok to release the mmu lock as the 
	// iAliasLinAddr and iAliasProcess members are only ever updated by the 
	// current thread.
	NKern::ThreadEnterCS();
	MmuLock::Unlock();
	iAliasProcess->AsyncCloseOsAsid();	// Asynchronous close as this method should be quick.
	NKern::ThreadLeaveCS();
	}


TInt M::DemandPagingFault(TAny* aExceptionInfo)
	{
	TX86ExcInfo& exc=*(TX86ExcInfo*)aExceptionInfo;
	if(exc.iExcId!=EX86VectorPageFault)
		return KErrAbort; // not a page fault

	/*
	Meanings of exc.iExcErrorCode when exception type is EX86VectorPageFault...

	Bit 0	0 The fault was caused by a non-present page.
			1 The fault was caused by a page-level protection violation.
	Bit 1	0 The access causing the fault was a read.
			1 The access causing the fault was a write.
	Bit 2	0 The access causing the fault originated when the processor was executing in supervisor mode.
			1 The access causing the fault originated when the processor was executing in user mode.   
	Bit 3	0 The fault was not caused by reserved bit violation.
			1 The fault was caused by reserved bits set to 1 in a page directory.
	Bit 4	0 The fault was not caused by an instruction fetch.
			1 The fault was caused by an instruction fetch.
	*/

	// check access type...
	TUint accessPermissions = EUser; // we only allow paging of user memory
	if(exc.iExcErrorCode&(1<<1))
		accessPermissions |= EReadWrite;

	// let TheMmu handle the fault...
	return TheMmu.HandlePageFault(exc.iEip, exc.iFaultAddress, accessPermissions, aExceptionInfo);
	}


