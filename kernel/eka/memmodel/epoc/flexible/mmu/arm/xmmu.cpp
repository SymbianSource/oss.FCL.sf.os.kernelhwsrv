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

#include "arm_mem.h"
#include "mm.h"
#include "mmu.h"
#include "mpager.h"

#include "cache_maintenance.inl"
#include "execs.h"


#ifdef BROADCAST_TLB_MAINTENANCE
class TTLBIPI : public TGenericIPI
	{
public:
	TTLBIPI();
	static void InvalidateIsr(TGenericIPI*);
	static void WaitAndInvalidateIsr(TGenericIPI*);
	void AddArg(TLinAddr aArg);
public:
	volatile TInt	iFlag;
	TLinAddr		iArg;
	};

TTLBIPI::TTLBIPI()
	:	iFlag(0), iArg(0)
	{
	}

void TTLBIPI::InvalidateIsr(TGenericIPI* aPtr)
	{
	TRACE2(("TLBInv"));
	TTLBIPI& a = *(TTLBIPI*)aPtr;
	TLinAddr arg = a.iArg;
	if (arg==0)
		LocalInvalidateTLB();
	else if (arg<256)
		LocalInvalidateTLBForAsid(arg);
	else
		LocalInvalidateTLBForPage(arg);
	}

void TTLBIPI::WaitAndInvalidateIsr(TGenericIPI* aPtr)
	{
	TRACE2(("TLBWtInv"));
	TTLBIPI& a = *(TTLBIPI*)aPtr;
	while (!a.iFlag)
		{ __chill(); }
	InvalidateIsr(aPtr);
	}

void TTLBIPI::AddArg(TLinAddr aArg)
	{
	iArg = aArg;
	NKern::Lock();
	InvalidateIsr(this);
	QueueAllOther(&InvalidateIsr);
	NKern::Unlock();
	WaitCompletion();
	}

void BroadcastInvalidateTLB(TLinAddr aLinAddrAndAsid)
	{
	TTLBIPI ipi;
	ipi.AddArg(aLinAddrAndAsid);
	}
#endif	// BROADCAST_TLB_MAINTENANCE

//
// Functions for class Mmu
//

/**
Return the physical address of the memory mapped by a Page Table Entry (PTE).

@param aPte			The value contained in the PTE.
@param aPteIndex	The index of the PTE within its page table.
*/
TPhysAddr Mmu::PtePhysAddr(TPte aPte, TUint aPteIndex)
	{
	if(aPte&KArmV6PteSmallPage)
		return aPte & KPteSmallPageAddrMask;
	if(aPte&KArmV6PteLargePage)
		return (aPte & KPteLargePageAddrMask) + (TPhysAddr(aPteIndex << KPageShift) & KLargePageMask);
	return KPhysAddrInvalid;
	}


/**
Return the virtual address of the page table referenced by the given
Page Directory Entry (PDE) \a aPde. If the PDE doesn't refer to a
page table then the null-pointer is returned.

If the page table was not one allocated by the kernel then the
results are unpredictable and may cause a system fault.

@pre #MmuLock held.
*/
TPte* Mmu::PageTableFromPde(TPde aPde)
	{
	if((aPde&KPdePresentMask)==KArmV6PdePageTable)
		{
		SPageInfo* pi = SPageInfo::FromPhysAddr(aPde);
		return (TPte*)(KPageTableBase+(pi->Index()<<KPageShift)+(aPde&(KPageMask&~KPageTableMask)));
		}
	return 0;
	}


/**
Perform the action of #PageTableFromPde but without the possibility of
a system fault caused the page table not being one allocated by the kernel.

@pre #MmuLock held.
*/
TPte* Mmu::SafePageTableFromPde(TPde aPde)
	{
	if((aPde&KPdeTypeMask)==KArmV6PdePageTable)
		{
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(aPde&~KPageMask);
		if(pi)
			return (TPte*)(KPageTableBase+(pi->Index()<<KPageShift)+(aPde&(KPageMask&~KPageTableMask)));
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
		return aPde&KPdeSectionAddrMask;
	return KPhysAddrInvalid;
	}


/**
Return a pointer to the Page Table Entry (PTE) which maps the
virtual address \a aAddress in the address space \a aOsAsid.

If no page table exists or it was not one allocated by the kernel
then the results are unpredictable and may cause a system fault.

@pre #MmuLock held.
*/
TPte* Mmu::PtePtrFromLinAddr(TLinAddr aAddress, TInt aOsAsid)
	{
	TPde pde = PageDirectory(aOsAsid)[aAddress>>KChunkShift];
	SPageInfo* pi = SPageInfo::FromPhysAddr(pde);
	TPte* pt = (TPte*)(KPageTableBase+(pi->Index()<<KPageShift)+(pde&(KPageMask&~KPageTableMask)));
	pt += (aAddress>>KPageShift)&(KChunkMask>>KPageShift);
	return pt;
	}


/**
Perform the action of #PtePtrFromLinAddr but without the possibility
of a system fault. If the page table is not present or not one
allocated by the kernel then the null-pointer is returned.

@pre #MmuLock held.
*/
TPte* Mmu::SafePtePtrFromLinAddr(TLinAddr aAddress, TInt aOsAsid)
	{
	TPde pde = PageDirectory(aOsAsid)[aAddress>>KChunkShift];
	TPte* pt = SafePageTableFromPde(pde);
	if(pt)
		pt += (aAddress>>KPageShift)&(KChunkMask>>KPageShift);
	return pt;
	}


/**
Return the physical address for the page table whose virtual
address is \a aPt.

If the page table was not one allocated by the kernel then the
results are unpredictable and may cause a system fault.

@pre #MmuLock held.
*/
TPhysAddr Mmu::PageTablePhysAddr(TPte* aPt)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld() || PageTablesLockIsHeld());

	TInt pdeIndex = ((TLinAddr)aPt)>>KChunkShift;
	TPde pde = PageDirectory(KKernelOsAsid)[pdeIndex];
	__NK_ASSERT_DEBUG((pde&KPdePresentMask)==KArmV6PdePageTable);

	SPageInfo* pi = SPageInfo::FromPhysAddr(pde);
	TPte* pPte = (TPte*)(KPageTableBase+(pi->Index(true)<<KPageShift)+(pde&(KPageMask&~KPageTableMask)));
	TPte pte = pPte[(((TLinAddr)aPt)&KChunkMask)>>KPageShift];
	__NK_ASSERT_DEBUG(pte & KArmV6PteSmallPage);

	return (pte&KPteSmallPageAddrMask)|(((TLinAddr)aPt)&(KPageMask&~KPageTableMask));
	}


/**
Perform a page table walk to return the physical address of
the memory mapped at virtual address \a aLinAddr in the
address space \a aOsAsid.

If the page table used was not one allocated by the kernel
then the results are unpredictable and may cause a system fault.

Use of this function should be avoided, use instead Mmu::LinearToPhysical
which contains debug assertions for its preconditions.

@pre #MmuLock held.
*/
TPhysAddr Mmu::UncheckedLinearToPhysical(TLinAddr aLinAddr, TInt aOsAsid)
	{
	TRACE2(("Mmu::UncheckedLinearToPhysical(%08x,%d)",aLinAddr,aOsAsid));
	TInt pdeIndex = aLinAddr>>KChunkShift;
	TPde pde = PageDirectory(aOsAsid)[pdeIndex];
	if ((pde&KPdePresentMask)==KArmV6PdePageTable)
		{
		SPageInfo* pi = SPageInfo::FromPhysAddr(pde);
		TPte* pPte = (TPte*)(KPageTableBase+(pi->Index(true)<<KPageShift)+(pde&(KPageMask&~KPageTableMask)));
		TPte pte = pPte[(aLinAddr&KChunkMask)>>KPageShift];
		if (pte & KArmV6PteSmallPage)
			{
			TPhysAddr pa=(pte&KPteSmallPageAddrMask)|(aLinAddr&~KPteSmallPageAddrMask);
			__KTRACE_OPT(KMMU,Kern::Printf("Mapped with small page - returning %08x",pa));
			return pa;
			}
		else if (pte & KArmV6PteLargePage)
			{
			TPhysAddr pa=(pte&KPteLargePageAddrMask)|(aLinAddr&~KPteLargePageAddrMask);
			__KTRACE_OPT(KMMU,Kern::Printf("Mapped with large page - returning %08x",pa));
			return pa;
			}
		}
	else if ((pde&KPdePresentMask)==KArmV6PdeSection)
		{
		TPhysAddr pa=(pde&KPdeSectionAddrMask)|(aLinAddr&~KPdeSectionAddrMask);
		__KTRACE_OPT(KMMU,Kern::Printf("Mapped with section - returning %08x",pa));
		return pa;
		}
	return KPhysAddrInvalid;
	}


extern TUint32 TTCR();
extern TUint32 CPUID(TInt /*aRegNum*/);


void Mmu::Init1()
	{
	TRACEB(("Mmu::Init1"));

	// check page local/global page directory split is correct...
	__NK_ASSERT_ALWAYS(TTCR()==1);

	// check cache type is supported and consistent with compile time macros...
	TInt iColourCount = 0;
	TInt dColourCount = 0;
	TUint32 ctr = InternalCache::TypeRegister();
	TRACEB(("CacheTypeRegister = %08x",ctr));
#ifdef __CPU_ARMV6
	__NK_ASSERT_ALWAYS((ctr>>29)==0);	// check ARMv6 format
	if(ctr&0x800)
		iColourCount = 4;
	if(ctr&0x800000)
		dColourCount = 4;
#else
	__NK_ASSERT_ALWAYS((ctr>>29)==4);	// check ARMv7 format
	TUint l1ip = (ctr>>14)&3;			// L1 instruction cache indexing and tagging policy
	__NK_ASSERT_ALWAYS(l1ip>=2);		// check I cache is physically tagged

	TUint32 clidr = InternalCache::LevelIDRegister();
	TRACEB(("CacheLevelIDRegister = %08x",clidr));
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
			TRACEB(("L1DCache = 0x%x,0x%x,%d colourCount=%d",sets,ways,lineSizeShift,(sets<<lineSizeShift)>>KPageShift));
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
			TRACEB(("L1ICache = 0x%x,0x%x,%d colourCount=%d",sets,ways,lineSizeShift,(sets<<lineSizeShift)>>KPageShift));
			}
		}
	if(l1ip==3)
		{
		// PIPT cache, so no colouring restrictions...
		TRACEB(("L1ICache is PIPT"));
		iColourCount = 0;
		}
	else
		{
		// VIPT cache...
		TRACEB(("L1ICache is VIPT"));
		}
#endif
	TRACEB(("page colouring counts I=%d, D=%d",iColourCount,dColourCount));
	__NK_ASSERT_ALWAYS(iColourCount<=KPageColourCount);
	__NK_ASSERT_ALWAYS(dColourCount<=KPageColourCount);
	#ifndef __CPU_I_CACHE_HAS_COLOUR
	__NK_ASSERT_ALWAYS(iColourCount==0);
	#endif
	#ifndef __CPU_D_CACHE_HAS_COLOUR
	__NK_ASSERT_ALWAYS(dColourCount==0);
	#endif
	#ifndef __CPU_CACHE_HAS_COLOUR
	__NK_ASSERT_ALWAYS(iColourCount==0);
	__NK_ASSERT_ALWAYS(dColourCount==0);
	#endif

	// check MMU attributes match our assumptions...
	if(((CPUID(-1)>>16)&0xf)==0xf) // if have new CPUID format....
		{
		TUint mmfr1 = CPUID(5);
		TRACEB(("mmfr1 = %08x",mmfr1));
		#ifdef __CPU_NEEDS_BTAC_FLUSH_AFTER_ASID_CHANGE
			__NK_ASSERT_ALWAYS(((mmfr1>>28)&0xf)==1); // Branch Predictor needs invalidating after ASID change
		#else
			__NK_ASSERT_ALWAYS(((mmfr1>>28)&0xf)>=2); // Branch Predictor doesn't needs invalidating after ASID change
		#endif

		TUint mmfr2 = CPUID(6);
		TRACEB(("mmfr2 = %08x",mmfr2));
		__NK_ASSERT_ALWAYS(((mmfr2>>20)&0xf)>=2); // check Mem Barrier instructions are supported in CP15

		TUint mmfr3 = CPUID(7);
		TRACEB(("mmfr3 = %08x",mmfr3));
		(void)mmfr3;

		#if defined(__SMP__) && !defined(__CPU_ARM11MP__)
			__NK_ASSERT_ALWAYS(((mmfr3>>12)&0xf)>=2); // check Maintenance Broadcast is for all cache and TLB operations
		#endif	
		#ifdef __CPU_SUPPORTS_PAGE_TABLE_WALK_TO_L1_CACHE
			__NK_ASSERT_ALWAYS(((mmfr3>>20)&0xf)>=1); // check Coherent Walk for page tables
		#endif	
		}

	Arm::DefaultDomainAccess = KDefaultDomainAccess;

#ifdef __SMP__
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

DMemoryObject* ExceptionStacks;

void Mmu::Init2Final()
	{
	TRACEB(("Mmu::Init2Final"));

	Init2FinalCommon();

	// initialise memory object for exception stacks...
	TMappingCreateFlags mapFlags = (TMappingCreateFlags)(EMappingCreateFixedVirtual|EMappingCreateReserveAllResources);
	TMemoryAttributes memAttr = EMemoryAttributeStandard;
	TUint size = 4*2*KPageSize; // 4 exception stacks each of one guard page and one mapped page
	size |= 1; // lower bit of size is set if region to be claimed contains gaps
	TInt r = MM::InitFixedKernelMemory(ExceptionStacks, KExcptStacksLinearBase, KExcptStacksLinearEnd, size, EMemoryObjectUnpaged, EMemoryCreateNoWipe, memAttr, mapFlags);
	__NK_ASSERT_ALWAYS(r==KErrNone);
	}


/**
Return the page directory entry (PDE) value to use for when mapping page tables intended
to map memory with the given attributes.
The returned value has the physical address component being zero, so a page table's physical
address can be simply ORed in.
*/
TPde Mmu::BlankPde(TMemoryAttributes aAttributes)
	{
	TPde pde = KArmV6PdePageTable;
	if(aAttributes&EMemoryAttributeUseECC)
		pde |= 1<<9;

	TRACE2(("Mmu::BlankPde(%x) returns 0x%x",aAttributes,pde));
	return pde;
	}


/**
Return the page directory entry (PDE) value to use for when creating a section mapping for memory
with the given attributes and #TPteType.
The returned value has the physical address component being zero, so the section's physical address
can be simply ORed in.
*/
TPde Mmu::BlankSectionPde(TMemoryAttributes aAttributes, TUint aPteType)
	{
	// reuse existing functions rather than duplicating the logic
	TPde pde = BlankPde(aAttributes);
	TPte pte = BlankPte(aAttributes, aPteType);
	return PageToSectionEntry(pte, pde);
	}


/**
Return the page table entry (PTE) to use when mapping memory pages
with the given attributes and #TPteType.
This value has the physical address component being zero, so a page's physical
address can be simply ORed in.
*/

TPte Mmu::BlankPte(TMemoryAttributes aAttributes, TUint aPteType)
	{
	TUint attr = CanonicalMemoryAttributes(aAttributes);

	// common PTE setup...
	TPte pte = KArmV6PteSmallPage|KArmV6PteAP0;
	if(aPteType&EPteTypeUserAccess)
		pte |= KArmV6PteAP1;					// AP1 = user access
	if((aPteType&EPteTypeWritable)==false)
		pte |= KArmV6PteAP2;					// AP2 = !writable
	if(attr&EMemoryAttributeShareable)
		pte |= KArmV6PteS;
	if((aPteType&EPteTypeGlobal)==false)
		pte |= KArmV6PteNG;
	if((aPteType&EPteTypeExecutable)==false)
		pte |= KArmV6PteSmallXN;

	#if defined(__CPU_MEMORY_TYPE_REMAPPING)

		// other PTE bits...
		if(pte&KArmV6PteSmallXN)
			pte |= KArmV6PteSmallTEX1;	// TEX1 is a copy of the XN

		// process memory type...
		TUint type = attr&EMemoryAttributeTypeMask;
		pte |= ((type&3)<<2) | ((type&4)<<4);

	#else

		// other PTE bits...
		if((pte&(KArmV6PteAP2|KArmV6PteAP1))==(KArmV6PteAP2|KArmV6PteAP1))
			pte &= ~KArmV6PteAP0;		// clear AP0 if user r/o

		// process memory type...
		TUint texcb;
		switch((TMemoryType)(attr&EMemoryAttributeTypeMask))
			{
		case EMemAttStronglyOrdered:
			texcb = KArmV6MemAttSO;
			break;
		case EMemAttDevice:
			if(attr&EMemoryAttributeShareable)
				texcb = KArmV6MemAttSD;
			else
				texcb = KArmV6MemAttSD;	// should be KArmV6MemAttNSD? (but this made H4 go bang)
			break;
		case EMemAttNormalUncached:
			texcb = KArmV6MemAttNCNC;
			break;
		case EMemAttNormalCached:
			texcb = KArmV6MemAttWBWAWBWA;
			break;
		case EMemAttKernelInternal4:
		case EMemAttPlatformSpecific5:
        case EMemAttPlatformSpecific6:
        case EMemAttPlatformSpecific7:
            {
			TUint32 cachingAttr = InternalCache::TypeToCachingAttributes((TMemoryType)(attr&EMemoryAttributeTypeMask));
		    switch (cachingAttr)
		        {
		        case EMapAttrFullyBlocking:
		            texcb = KArmV6MemAttSO;
		            break;
		        case EMapAttrBufferedNC:
	                texcb = KArmV6MemAttSD;
		            break;
		        default:
		            {
		            //attr describes normal mapping
		            //set texcb to b1BBAA where AA is internal and BB is external caching
		            // TYPE       AA/BB
		            // uncached   0
                    // WBWA       1
		            // WTRA       2
		            // WBRA       3
		            texcb = 0x10;
                    switch (cachingAttr&EMapAttrL1CacheMask)
                        {
                        case EMapAttrL1Uncached:  break;
                        #if defined(__CPU_ARM1136_ERRATUM_399234_FIXED)
                        case EMapAttrCachedWTRA:  texcb |= 2;break; // It is OK to use WT memory
                        #else
                        case EMapAttrCachedWTRA:;break; // Erratum not fixed. Use uncached memory instead
                        #endif
                        case EMapAttrCachedWBRA:  texcb |= 3; break;
                        default: texcb |= 1;//fully cached (WBWA)
                        }
                    switch (cachingAttr&EMapAttrL2CacheMask)
                        {
                        case EMapAttrL2Uncached:  break;
                        case EMapAttrL2CachedWTRA:  texcb |= 8;break;
                        case EMapAttrL2CachedWBRA:  texcb |= 0xc; break;
                        default: texcb |= 4;//fully cached (WBWA)
                        }
		            }
		        }
            }
            break;
        default:
		    __NK_ASSERT_ALWAYS(0);		// undefined memory type
			texcb = KArmV6MemAttSO;
			break;
			}
		pte |= ((texcb&0x1c)<<4) | ((texcb&0x03)<<2);

	#endif

	TRACE2(("Mmu::BlankPte(%x,%x) returns 0x%x",aAttributes,aPteType,pte));
	return pte;
	}


/**
Calculate PDE and PTE which represent a page table mapping for an existing
section mapping.

@param[in] aPde The PDE for the existing section mapping.
@param[out] aPde A PDE for a page table mapping, with physical address == 0.

@return The PTE value for the first entry in the page table.
*/
TPte Mmu::SectionToPageEntry(TPde& aPde)
	{
	TPde pde = aPde;

	// calculate new PTE...
	TPte pte = pde&0xc; // copy CB bits
	if(pde&KArmV6PdeSectionXN)
		pte |= KArmV6PteSmallXN; // copy XN bit
	pte |= (pde&(0xff<<10))>>6; // copy NG, S, APX, TEX, AP bits
	pte |= KArmV6PteSmallPage;

	// calculate new PDE...
	pde &= 0x3e0;	// keep IMP and DOMAIN
	pde |= KArmV6PdePageTable;

	aPde = pde;
	return pte;
	}


/**
Calculate a PDE entry which represents a section mapping for an existing
page table mapping.

@pre The existing page table contains mappings for a chunk sized and
	 aligned contiguous region.

@param aPte A PTE from the existing page table.
@param aPde The existing PDE for the page table mappings.
			(Physical address portion is ignored.)

@return A PDE entry value for a section mapping.
*/
TPde Mmu::PageToSectionEntry(TPte aPte, TPde aPde)
	{
	TPde pde = aPde&0x3e0;	// keep IMP and DOMAIN
	pde |= aPte&(KPdeSectionAddrMask|0xc); // copy address and CB bits
	if(aPte&KArmV6PteSmallXN)
		pde |= KArmV6PdeSectionXN; // copy XN bit
	pde |= (aPte&(0xff<<4))<<6;  // copy NG, S, APX, TEX, AP bits
	pde |= KArmV6PdeSection;
	return pde;
	}


/**
Tranform the specified memory attributes into the canonical form relevant to
the platform the code is running on. This applies defaults and overrides to
the attributes to return what should be used with the MMU.
*/
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

#if defined(FAULTY_NONSHARED_DEVICE_MEMORY)
	if((attr&(EMemoryAttributeShareable|EMemoryAttributeTypeMask))==EMemoryAttributeDevice)
		{
		// make unshared device memory into shared strongly ordered memory...
		attr ^= EMemoryAttributeShareable;
		attr ^= EMemoryAttributeDevice^EMemoryAttributeStronglyOrdered;
		}
#endif

#if	defined(__SMP__) || defined(__CPU_FORCE_SHARED_MEMORY_IF_CACHED)
	TMemoryType type = (TMemoryType)(attr&KMemoryTypeMask);
	if(CacheMaintenance::IsCached(type))
		{
		// force cached memory to be shared memory on SMP systems...
		attr |= EMemoryAttributeShareable;
		}
#endif

	return (TMemoryAttributes)(attr&EMemoryAttributeMask);
	}

/**
Method called to initialise RAM pages when they are allocated for a new use.
This performs any cache synchronisation required to remove old entries
and also wipes the contents of the memory (if requested via \a aFlags).

@param aPageList	Pointer to a list of physical addresses for the RAM pages,
					or, if the least significant bit of this value is set, then
					the rest of the value is the physical address of a contiguous
					region of RAM pages being allocated.

@param aCount		The number of pages.

@param aFlags		A set of flag values from #TRamAllocFlags which indicate
					the memory type the pages will be used for and whether
					the contents should be wiped.

@param aReallocate	True, if the RAM pages have already been previously allocated
					and are being reinitilised e.g. by DMemoryManager::ReAllocDecommitted.
					False, to indicate that these pages have been newly allocated (are in
					the SPageInfo::EUnused state.)

@pre #RamAllocLock held.
*/
void Mmu::PagesAllocated(TPhysAddr* aPageList, TUint aCount, TRamAllocFlags aFlags, TBool aReallocate)
	{
	TRACE2(("Mmu::PagesAllocated(0x%08x,%d,0x%x,%d)",aPageList, aCount, aFlags, (bool)aReallocate));
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());

	TBool wipe = !(aFlags&EAllocNoWipe); // do we need to wipe page contents?
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
		TBool oldTypeNormal = CacheMaintenance::IsNormal(oldType);

		TRACE2(("Mmu::PagesAllocated page=0x%08x, oldType=%d, wipe=%d, colour=%d",pagePhys,oldType,wipe,pi->Index(true)&KPageColourMask));
		if(wipe || oldTypeNormal)
			{
			// work out temporary mapping values...
			TUint colour = pi->Index(true)&KPageColourMask;
			TLinAddr tempLinAddr = iTempMap[0].iLinAddr+colour*KPageSize;
			TPte* tempPte = iTempMap[0].iPtePtr+colour;

			if(oldTypeNormal)
				{
				// cache maintenance required. Prepare temporary mapping.
				*tempPte = pagePhys | iTempPteCacheMaintenance;
				CacheMaintenance::SinglePteUpdated((TLinAddr)tempPte);
				InvalidateTLBForPage(tempLinAddr|KKernelOsAsid);

				// will hold additional arguments in CacheMaintenance::PageToReuse call
				TInt pageToReuseMask = 0;
				
				// check if old and new mappings are the same. (Wiping needs temporary
				// mapping which may not be the same as the old and new mapping.)
				TMemoryType newType = (TMemoryType)(aFlags&KMemoryTypeMask); // memory type that pages will be used for
				if (!wipe && (newType ==oldType))
					pageToReuseMask |= CacheMaintenance::EOldAndNewMappingMatch;

				MmuLock::Lock();

				// decide wether to trigger maintenance of entire cache(s).
				if(CacheMaintenance::IsPageToReuseThresholdReached(iCacheInvalidatePageCount))
					{
					// enough pages to make it worth triggering maintenance of entire cache(s)
					pageToReuseMask |= CacheMaintenance::EThresholdReached;
					++iCacheInvalidateCounter;
					iCacheInvalidatePageCount = 0; // all pages will be partially synced 
					}
				
				if(CacheMaintenance::IsCached(oldType) && !aReallocate)
					{
					if(pi->CacheInvalidateCounter()==(TUint32)iCacheInvalidateCounter)
						{
						// one less unused page in the L1 cache...
						__NK_ASSERT_DEBUG(iCacheInvalidatePageCount);
						--iCacheInvalidatePageCount;
						}
					else
						{
						// our page has been already partially maintained in cache
						// by a previous PageToReuse call.
						pageToReuseMask |= CacheMaintenance::EPageHasBeenPartiallySynced;
						}
					}
				
				MmuLock::Unlock();
				
				TBool pageRemovedFromCache = CacheMaintenance::PageToReuse(tempLinAddr, oldType, pagePhys, pageToReuseMask);
				if(pageRemovedFromCache && !aReallocate)
					pi->SetUncached();
				}

			if(wipe)
				{
				//We need uncached normal temporary mapping to wipe. Change it if necessary.
				//or , in case of !oldTypeNormal it is not configured yet.
				if (!oldTypeNormal || (CacheMaintenance::TemporaryMapping()!=EMemAttNormalUncached))
					{
					*tempPte = pagePhys | iTempPteUncached;
					CacheMaintenance::SinglePteUpdated((TLinAddr)tempPte);
					InvalidateTLBForPage(tempLinAddr|KKernelOsAsid);
					}
				// wipe contents of memory...
				memset((TAny*)tempLinAddr, wipeByte, KPageSize);
				CacheMaintenance::PageToReuse(tempLinAddr, EMemAttNormalUncached, pagePhys);
				}

			// invalidate temporary mapping...
			*tempPte = KPteUnallocatedEntry;
			CacheMaintenance::SinglePteUpdated((TLinAddr)tempPte);
			InvalidateTLBForPage(tempLinAddr|KKernelOsAsid);
			}

		// indicate page has been allocated...
		if(!aReallocate)
			pi->SetAllocated();

		// loop round for next page...
		} // end of while(aCount--)
	}


/**
Method called to update the state of a RAM page when it is freed.
This sets the page state to SPageInfo::EUnused.

@param aPageInfo	The page information structure for the RAM page.

@pre #MmuLock held.
*/
void Mmu::PageFreed(SPageInfo* aPageInfo)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	if(aPageInfo->Type()==SPageInfo::EUnused)
		return;

	aPageInfo->SetUnused();

	TMemoryType type = (TMemoryType)(aPageInfo->Flags()&KMemoryTypeMask);
	if(CacheMaintenance::IsCached(type))
		{
		// another unused page with L1 cache entries...
		aPageInfo->SetCacheInvalidateCounter(iCacheInvalidateCounter);
		++iCacheInvalidatePageCount;
		}

	TRACE2(("Mmu::PageFreed page=0x%08x type=%d colour=%d",aPageInfo->PhysAddr(),aPageInfo->Flags()&KMemoryTypeMask,aPageInfo->Index()&KPageColourMask));
	}

/**
Remove the contents of RAM pages from any memory caches.

@param aPages		Pointer to a list of physical addresses for the RAM pages,
					or, if the least significant bit of this value is set, then
					the rest of the value is the physical address of a contiguous
					region of RAM pages.

@param aCount		The number of pages.

@param aAttributes	The memory attributes of the pages.

@param aColour 		The colour for the first page;
					consecutive pages will be coloured accordingly.
					Only #KPageColourShift least significant bits are used,
					therefore an index into a memory object's memory can be
					used for this value.
*/
void Mmu::CleanAndInvalidatePages(TPhysAddr* aPages, TUint aCount, TMemoryAttributes aAttributes, TUint aColour)
	{
	TMemoryType type = (TMemoryType)(aAttributes&EMemoryAttributeTypeMask);

	if(!CacheMaintenance::IsNormal(type))
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
		aColour &= KPageColourMask;
		TLinAddr tempLinAddr = iTempMap[0].iLinAddr+aColour*KPageSize;
		TPte* tempPte = iTempMap[0].iPtePtr+aColour;
		++aColour;

		// temporarily map page...
		*tempPte = pagePhys | iTempPteCacheMaintenance;
		CacheMaintenance::SinglePteUpdated((TLinAddr)tempPte);
		InvalidateTLBForPage(tempLinAddr|KKernelOsAsid);

		// preserve memory content and remove from cache...
		CacheMaintenance::PageToPreserveAndReuse(tempLinAddr, type, pagePhys);

		// invalidate temporary mapping...
		*tempPte = KPteUnallocatedEntry;
		CacheMaintenance::SinglePteUpdated((TLinAddr)tempPte);
		InvalidateTLBForPage(tempLinAddr|KKernelOsAsid);

		RamAllocLock::Flash();
		}
	RamAllocLock::Unlock();
	}


extern void UnlockIPCAlias();
extern void LockIPCAlias();


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
		if (!iAliasLinAddr)
			{// Close the new reference as RemoveAlias won't do as iAliasLinAddr is not set.
			aProcess->AsyncCloseOsAsid();	// Asynchronous close as this method should be quick.
			}
		MmuLock::Unlock();
		return KErrBadDescriptor; // prevent access to supervisor only memory
		}

	// Now we know all accesses to global memory are safe so check if aAddr is global.
	if(aAddr >= KGlobalMemoryBase)
		{
		// address is in global section, don't bother aliasing it...
		if (!iAliasLinAddr)
			{// Close the new reference as not required.
			aProcess->AsyncCloseOsAsid(); // Asynchronous close as this method should be quick.
			MmuLock::Unlock();
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
	pde = PDE_IN_DOMAIN(pde, KIPCAliasDomain);	// change domain for PDE
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
			UnlockIPCAlias();
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
		iAliasPdePtr = (TPde*)(TLinAddr(ss.i_AliasPdePtr) + (local_asid << KPageDirectoryShift));
#endif
		iAliasLinAddr = aliasAddr;
		*iAliasPdePtr = pde;
		SinglePdeUpdated(iAliasPdePtr);
		}

	TRACE2(("DMemModelThread::Alias() PDEntry=%x, iAliasLinAddr=%x",pde, aliasAddr));
	LocalInvalidateTLBForPage(aliasAddr | local_asid);
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
@post MmuLock released.
*/
void DMemModelThread::DoRemoveAlias(TLinAddr aAddr)
	{
	LockIPCAlias();
	iAliasLinAddr = 0;
	iAliasPde = KPdeUnallocatedEntry;
	*iAliasPdePtr = KPdeUnallocatedEntry;
	SinglePdeUpdated(iAliasPdePtr);
	__NK_ASSERT_DEBUG((aAddr&KPageMask)==0);
	// Invalidate the tlb using os asid, no need to open a reference as this
	// is the current thread's process os asid.
	LocalInvalidateTLBForPage(aAddr | ((DMemModelProcess*)iOwningProcess)->OsAsid());
	iAliasLink.Deque();
#ifdef __SMP__
	__NK_ASSERT_DEBUG(iCpuRestoreCookie>=0);
	NKern::EndFreezeCpu(iCpuRestoreCookie);
	iCpuRestoreCookie = -1;
#endif

	// Must close the os asid while holding MmuLock so we are in an implicit critical section.
	iAliasProcess->AsyncCloseOsAsid(); // Asynchronous close as this method should be quick.
	MmuLock::Unlock();
	}


TInt M::DemandPagingFault(TAny* aExceptionInfo)
	{
	TArmExcInfo& exc=*(TArmExcInfo*)aExceptionInfo;

	// permissions required by faulting memory access...
	TUint accessPermissions = EUser; // we only allow paging of user memory

	// get faulting address...
	TLinAddr faultAddress = exc.iFaultAddress;
	if(exc.iExcCode==EArmExceptionPrefetchAbort)
		{
		// fault trying to read code to execute...
		accessPermissions |= EExecute;
		}
	else if(exc.iExcCode!=EArmExceptionDataAbort)
		return KErrUnknown; // not prefetch or data abort

	// check fault type...
	if((exc.iFaultStatus&0x405) != 5 && (exc.iFaultStatus&0x40f) != 4)
		return KErrUnknown; // not translation, permission or instruction cache maintenance fault.

	// check access type...
	if(exc.iFaultStatus&(1<<11))
		accessPermissions |= EReadWrite;

	// let TheMmu handle the fault...
	return TheMmu.HandlePageFault(exc.iR15, faultAddress, accessPermissions, aExceptionInfo);
	}


