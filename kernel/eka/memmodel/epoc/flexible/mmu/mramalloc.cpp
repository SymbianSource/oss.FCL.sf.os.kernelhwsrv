// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Symbian Foundation License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.symbianfoundation.org/legal/sfl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//

#include <plat_priv.h>
#include "mramalloc.h"

struct SGroup
	{
	TBitMapAllocator* iBma;
	TPhysAddr iPhysBase;
	TInt iNumBase;
	TUint8 iPwrBlock;
	};

class DRamAllocator : public DRamAllocatorBase
	{
public:
	virtual TInt Create(const SRamInfo& aInfo, const SRamBank* aPowerBanks);
	virtual TInt MarkPageAllocated(TPhysAddr aAddr);
	virtual TInt FreeRamPage(TPhysAddr aAddr);
	virtual void FreeRamPages(TPhysAddr* aPageList, TInt aNumPages);
	virtual TInt AllocRamPages(TPhysAddr* aPageList, TInt aNumPages);
	virtual TInt AllocContiguousRam(TInt aNumPages, TPhysAddr& aPhysAddr, TInt aAlign=0);
	virtual TInt SetPhysicalRamState(TPhysAddr aBase, TInt aSize, TBool aState);
	virtual TUint TotalPhysicalRamPages() {return iTotalRamPages;};
#ifdef KMMU
	void DebugDump();
#endif
private:
	SGroup* GetGroupAndOffset(TPhysAddr aAddr, TInt& aOffset);
	void MarkPagesAllocated(TInt aPageNum, TInt aCount);
	TInt FindContiguousRam(TInt aNumPages, TInt aAlignWrtPage, TUint8* aPermute, TInt& aPageNum);
private:
	enum TPanic
		{
		// don't use 0
		EDoNotUse=0,
		EBytesFromStartInvalid,
		EBytesFromEndInvalid,
		EAreasTooSmall,
		ETooManyPowerBlocks,
		EInvalidPowerBlocks,
		EDoMarkPagesAllocated1,
		EAllocRamPagesInconsistent,
		};
private:
	TInt iTotalRamPages;
	TInt iNumGroups;				// group corresponds to BMA
	TInt iAreaSize;					// size of an area in bytes
	TInt iAreaShift;				// log2(areasize)
	TUint32 iAreaMask;				// iAreaSize-1
	TInt iAreaPages;				// size of an area in pages
	TInt iAreaPageShift;			// log2(areapages)
	TUint32 iAreaPageMask;			// iAreaPages-1
	TInt iNumAreas;					// number of areas recognised (size of iPhysAddrLUT)
	SGroup* iGroups;				// per-group info
	TUint8* iGroupPowerOrder;		// table of indices into above tables in power block order
	TUint8* iPhysAddrLUT;			// table of indices indexed by (physaddr-physaddrbase)>>areashift
	TUint8* iPageNumLUT;			// table of indices indexed by pagenum>>areapageshift
	TPhysAddr iPhysAddrBase;		// lowest valid physical address
	TPhysAddr iPhysAddrTop;			// highest valid physical address+1
	};

DRamAllocatorBase* DRamAllocatorBase::New()
	{
	return new DRamAllocator;
	}

DRamAllocatorBase* DRamAllocatorBase::New(const SRamInfo& aInfo, TInt aPageShift, const SRamBank* aPowerBanks)
	{
	DRamAllocatorBase* pA=New();
	if (!pA)
		Panic(KErrNoMemory);
	pA->iPageShift=aPageShift;
	pA->iPageSize=1<<aPageShift;
	TInt r=pA->Create(aInfo,aPowerBanks);
	if (r!=KErrNone)
		Panic(r);
	return pA;
	}

void DRamAllocatorBase::Panic(TInt aPanic)
	{
	Kern::Fault("RAM-ALLOC", aPanic);
	}

#ifdef KMMU
void HexDump32(const TAny* a, TInt n, const char* s)
	{
	const TUint32* p=(const TUint32*)a;
	Kern::Printf(s);
	TInt i=0;
	while(n)
		{
		TBuf8<80> b;
		b.AppendNumFixedWidth(i,EHex,4);
		b.Append(':');
		TInt m=Min(n,4);
		n-=m;
		i+=m;
		while(m--)
			{
			b.Append(' ');
			b.AppendNumFixedWidth(*p++,EHex,8);
			}
		Kern::Printf("%S",&b);
		}
	}

void HexDump8(const TAny* a, TInt n, const char* s)
	{
	const TUint8* p=(const TUint8*)a;
	Kern::Printf(s);
	TInt i=0;
	while(n)
		{
		TBuf8<80> b;
		b.AppendNumFixedWidth(i,EHex,4);
		b.Append(':');
		TInt m=Min(n,16);
		n-=m;
		i+=m;
		while(m--)
			{
			b.Append(' ');
			b.AppendNumFixedWidth(*p++,EHex,2);
			}
		Kern::Printf("%S",&b);
		}
	}

void DRamAllocator::DebugDump()
	{
	Kern::Printf("PageSize=%08x PageShift=%d",iPageSize,iPageShift);
	Kern::Printf("AreaSize=%08x AreaShift=%d AreaMask=%08x",iAreaSize,iAreaShift,iAreaMask);
	Kern::Printf("AreaPages=%08x AreaPageShift=%d AreaPageMask=%08x",iAreaPages,iAreaPageShift,iAreaPageMask);
	Kern::Printf("Total Pages=%08x Total Free=%08x",iTotalRamPages,iTotalFreeRamPages);
	Kern::Printf("Number of areas=%08x, number of groups=%08x",iNumAreas,iNumGroups);
	Kern::Printf("Number of power blocks=%d, PowerState=%08x",iNumPowerBlocks,iPowerState);
	Kern::Printf("PhysAddrBase=%08x, PhysAddrTop=%08x",iPhysAddrBase,iPhysAddrTop);

	TInt i;
	Kern::Printf("Group Info:");
	for (i=0; i<iNumGroups; ++i)
		{
		SGroup& g=iGroups[i];
		TBitMapAllocator& b=*g.iBma;
		Kern::Printf("%02x: Avail %08x Size %08x Phys %08x Num %08x Pwr %02x",i,b.iAvail,b.iSize,
										g.iPhysBase,g.iNumBase,g.iPwrBlock);
		}
	if (iGroupPowerOrder)
		HexDump8(iGroupPowerOrder,iNumGroups,"GroupPowerOrder:");
	HexDump8(iPhysAddrLUT,iNumAreas,"PhysAddrLUT:");
	HexDump8(iPageNumLUT,iTotalRamPages>>iAreaPageShift,"PageNumLUT:");
	HexDump32(iPowerBlockPages,iNumPowerBlocks,"PowerBlockPages:");
	}
#endif

TInt CountBanks(const SRamBank* aBankList)
	{
	TInt banks=0;
	for (; aBankList->iSize; ++banks, ++aBankList);
	return banks;
	}

TInt CalcAreaShift(const SRamBank* aBankList)
	{
	TUint32 mask=0;
	for (; aBankList->iSize; ++aBankList)
		{
		TUint32 base=aBankList->iBase;
		TUint32 end=base+aBankList->iSize-1;
		__KTRACE_OPT(KBOOT,Kern::Printf("Base=%08x End=%08x",base,end));
		mask|=base;
		mask|=~end;
		}
	return __e32_find_ls1_32(mask);
	}

TUint32 TotalBankSize(const SRamBank* aBankList)
	{
	TUint32 size=0;
	for (; aBankList->iSize; ++aBankList)
		size+=aBankList->iSize;
	return size;
	}

TInt DRamAllocator::Create(const SRamInfo& a, const SRamBank* aP)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DRamAllocator::Create"));

	TInt num_boot_banks=CountBanks(a.iBanks);
	TUint32 total_ram_size=TotalBankSize(a.iBanks);
	__KTRACE_OPT(KMMU,Kern::Printf("#banks from bootstrap=%d",num_boot_banks));
	__KTRACE_OPT(KMMU,Kern::Printf("Total size=%08x",total_ram_size));
	iTotalRamPages=total_ram_size>>iPageShift;
	iTotalFreeRamPages=iTotalRamPages;
	__KTRACE_OPT(KMMU,Kern::Printf("Total size=%08x, total pages=%08x",total_ram_size,iTotalRamPages));

	__KTRACE_OPT(KMMU,Kern::Printf("Calculate area shift from bootstrap blocks"));
	iAreaShift=CalcAreaShift(a.iBanks);
	__KTRACE_OPT(KMMU,Kern::Printf("iAreaShift=%d",iAreaShift));
	iNumPowerBlocks=1;
	if (aP)
		{
		iNumPowerBlocks=CountBanks(aP);
		__KTRACE_OPT(KMMU,Kern::Printf("iNumPowerBlocks=%d",iNumPowerBlocks));
		if (iNumPowerBlocks>32)
			return ETooManyPowerBlocks;
		__KTRACE_OPT(KMMU,Kern::Printf("Calculate area shift from power blocks"));
		TInt as=CalcAreaShift(aP);
		__KTRACE_OPT(KMMU,Kern::Printf("area shift=%d",as));
		if (as<iAreaShift)
			iAreaShift=as;
		}
	if (iAreaShift<16 || iAreaShift<iPageShift)
		return EAreasTooSmall;
	iAreaSize=1<<iAreaShift;
	iAreaMask=iAreaSize-1;
	iAreaPageShift=iAreaShift-iPageShift;
	iAreaPages=1<<iAreaPageShift;
	iAreaPageMask=iAreaPages-1;
	__KTRACE_OPT(KMMU,Kern::Printf("iAreaShift=%d",iAreaShift));
	
	iPhysAddrBase=a.iBanks[0].iBase;
	const SRamBank& last_boot_bank=a.iBanks[num_boot_banks-1];
	iPhysAddrTop=last_boot_bank.iBase+last_boot_bank.iSize;
	__KTRACE_OPT(KMMU,Kern::Printf("PA base=%08x, PA top=%08x",iPhysAddrBase,iPhysAddrTop));
	iNumAreas=(iPhysAddrTop-iPhysAddrBase)>>iAreaShift;
	__KTRACE_OPT(KMMU,Kern::Printf("iNumAreas=%08x",iNumAreas));

	iPhysAddrLUT=(TUint8*)Kern::Alloc(iNumAreas);
	if (!iPhysAddrLUT)
		return KErrNoMemory;
	iPageNumLUT=(TUint8*)Kern::Alloc(iNumAreas);	// overallocate temporarily
	if (!iPageNumLUT)
		return KErrNoMemory;
	iPowerBlockPages=(TInt*)Kern::AllocZ(iNumPowerBlocks*sizeof(TInt));
	if (!iPowerBlockPages)
		return KErrNoMemory;

	// coalesce contiguous boot banks
	SRamBank* phys_banks = (SRamBank*)Kern::Alloc(num_boot_banks*sizeof(SRamBank));
	if (!phys_banks)
		return KErrNoMemory;
	SRamBank* pD=phys_banks;
	const SRamBank* pBoot=a.iBanks;
	const SRamBank* pE=pBoot+num_boot_banks;
	TPhysAddr base=0;
	TPhysAddr end=0;
	for (; pBoot<=pE; ++pBoot)
		{
		if (pBoot==pE || pBoot->iBase!=end)
			{
			if (end)
				{
				pD->iBase=base;
				pD->iSize=end-base;
				++pD;
				__KTRACE_OPT(KMMU,Kern::Printf("Coalesced bank: %08x-%08x",base,end));
				}
			if (pBoot<pE)
				{
				base=pBoot->iBase;
				end=base+pBoot->iSize;
				}
			}
		else
			end+=pBoot->iSize;
		}
	SRamBank* pPhysEnd=pD;
	__KTRACE_OPT(KMMU,Kern::Printf("#Coalesced banks: %d",pD-phys_banks));

	// work out groups
	TInt start_area;
	TInt num_areas;
	TInt phys_bank;
	TInt pwr_bank;
	memset(iPhysAddrLUT,0xff,iNumAreas);
	pD=phys_banks;
	for (; pD<pPhysEnd; ++pD)
		{
		start_area=(pD->iBase-iPhysAddrBase)>>iAreaShift;
		num_areas=pD->iSize>>iAreaShift;
		phys_bank=pD-phys_banks;
		memset(iPhysAddrLUT+start_area, phys_bank, num_areas);
		}

	if (aP)
		{
		memset(iPageNumLUT,0xff,iNumAreas);
		const SRamBank* pB=aP;
		const SRamBank* pPwrEnd=aP+iNumPowerBlocks;
		for (; pB<pPwrEnd; ++pB)
			{
			start_area=(Max(pB->iBase,iPhysAddrBase)-iPhysAddrBase)>>iAreaShift;
			num_areas=(TInt)Min(TUint32(pB->iSize)>>iAreaShift, TUint32(iNumAreas-start_area));
			pwr_bank=pB-aP;
			memset(iPageNumLUT+start_area, pwr_bank, num_areas);
			}
		}
	Kern::Free(phys_banks);

	phys_bank=0xff;
	pwr_bank=-1;
	TInt area;
	iNumGroups=0;
	for (area=0; area<=iNumAreas; ++area)
		{
		TInt pb=(area<iNumAreas)?iPhysAddrLUT[area]:0xff;
		TInt pwb=aP?((area<iNumAreas)?iPageNumLUT[area]:0xff):-1;
		__KTRACE_OPT(KMMU,Kern::Printf("Area %04x (%08x) : pb=%02x pwb=%02x",area,iPhysAddrBase+(area<<iAreaShift),pb,pwb));
		if (pb!=phys_bank || pwb!=pwr_bank)
			{
			if (pb!=0xff && pwb==0xff)
				return EInvalidPowerBlocks;
			if (phys_bank!=0xff)
				++iNumGroups;
			phys_bank=pb;
			if (aP)
				pwr_bank=pwb;
			}
		}
	__KTRACE_OPT(KMMU,Kern::Printf("iNumGroups=%d",iNumGroups));
	iGroups=(SGroup*)Kern::Alloc(iNumGroups*sizeof(SGroup));
	if (!iGroups)
		return KErrNoMemory;
	if (aP)
		{
		iGroupPowerOrder = (TUint8*)Kern::Alloc(iNumGroups);
		if (!iGroupPowerOrder)
			return KErrNoMemory;
		}
	start_area=0;
	phys_bank=0xff;
	pwr_bank=0;
	TInt group=0;
	TInt page_number=0;
	for (area=0; area<=iNumAreas; ++area)
		{
		TInt pb=(area<iNumAreas)?iPhysAddrLUT[area]:0xff;
		TInt pwb=aP?((area<iNumAreas)?iPageNumLUT[area]:0xff):0;
		if (pb!=phys_bank || (aP && pwb!=pwr_bank))
			{
			TInt group_num_areas=area-start_area;
			if (phys_bank!=0xff)
				{
				SGroup& g=iGroups[group];
				TInt group_num_pages=TUint32(group_num_areas)<<iAreaPageShift;
				g.iBma=TBitMapAllocator::New(group_num_pages, ETrue);
				if (!g.iBma)
					return KErrNoMemory;
				g.iPhysBase=(TPhysAddr(start_area)<<iAreaShift)+iPhysAddrBase;
				g.iNumBase=page_number;
				memset(iPhysAddrLUT+start_area, group, group_num_areas);
				memset(iPageNumLUT+(page_number>>iAreaPageShift), group, group_num_areas);
				page_number+=group_num_pages;
				g.iPwrBlock=pwr_bank;
				__KTRACE_OPT(KMMU,Kern::Printf("Group %d: PhysBase=%08x NumBase=%08x PwrBlock=%02x NumPages=%08x",
													group, g.iPhysBase, g.iNumBase, g.iPwrBlock, group_num_pages));
				++group;
				}
			start_area=area;
			phys_bank=pb;
			if (aP)
				pwr_bank=pwb;
			}
		}

	// shrink iPageNumLUT to correct size
	iPageNumLUT=(TUint8*)Kern::ReAlloc(iPageNumLUT, iTotalRamPages>>iAreaPageShift);

	if (aP)
		{
		// work out power block ordering of groups
		TBool identity=ETrue;
		TInt last_pwb=-1;
		group=0;
		while(group<iNumGroups)
			{
			TInt first_pwb=256;
			TInt i=0;
			TInt j=0;
			for (; i<iNumGroups; ++i)
				{
				TInt gpwb=iGroups[i].iPwrBlock;
				if (gpwb>last_pwb && gpwb<first_pwb)
					{
					j=i;
					first_pwb=gpwb;
					}
				}
			do	{
				if (j!=group)
					identity=EFalse;
				iGroupPowerOrder[group++]=j++;
				} while (j<iNumGroups && iGroups[j].iPwrBlock==first_pwb);
			last_pwb=first_pwb;
			}
		if (identity)
			{
			// power order and physical address order coincide so no need to keep iGroupPowerOrder
			Kern::Free(iGroupPowerOrder);
			iGroupPowerOrder=NULL;
			}
		}

	// Now mark any reserved regions as allocated
	const SRamBank* pB = pE + 1;	// first reserved block specifier
	for (; pB->iSize; ++pB)
		{
		__KTRACE_OPT(KMMU, Kern::Printf("Reserve physical block %08x+%x", pB->iBase, pB->iSize));
		TInt r = SetPhysicalRamState(pB->iBase, pB->iSize, EFalse);
		__KTRACE_OPT(KMMU, Kern::Printf("Reserve returns %d", r));
		if (r!=KErrNone)
			return r;
		}

	__KTRACE_OPT(KMMU,DebugDump());
	return KErrNone;
	}

SGroup* DRamAllocator::GetGroupAndOffset(TPhysAddr aAddr, TInt& aOffset)
	{
	if (aAddr<iPhysAddrBase || aAddr>=iPhysAddrTop)
		return NULL;
	TInt area=TInt((aAddr-iPhysAddrBase)>>iAreaShift);
	TInt group=iPhysAddrLUT[area];
	if (group==0xff)
		return NULL;
	SGroup& g=iGroups[group];
	aOffset=(aAddr-g.iPhysBase)>>iPageShift;
	return &g;
	}

void DRamAllocator::MarkPagesAllocated(TInt aPageNum, TInt aCount)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DRamAllocator::MarkPagesAllocated(%x+%x)",aPageNum,aCount));
	if ((TUint32(aPageNum)>=TUint32(iTotalRamPages)) || (TUint32(aCount)>TUint32(iTotalRamPages-aPageNum)))
		Panic(EDoMarkPagesAllocated1);
	TInt area=aPageNum>>iAreaPageShift;
	SGroup* pG=iGroups+iPageNumLUT[area];
	iTotalFreeRamPages-=aCount;
	while(aCount)
		{
		TInt gpnb=pG->iNumBase;
		TBitMapAllocator& bma=*pG->iBma;
		TInt gsz=bma.iSize;
		TInt ix=aPageNum-gpnb;
		TInt count=Min(gsz-ix,aCount);
		bma.Alloc(ix,count);
		TInt pwb=pG->iPwrBlock;
		iPowerBlockPages[pwb]+=count;
		iPowerState|=(1u<<pwb);
		aCount-=count;
		aPageNum+=count;
		++pG;
		}
	}

TInt DRamAllocator::MarkPageAllocated(TPhysAddr aAddr)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DRamAllocator::MarkPageAllocated %08x",aAddr));
	TInt n;
	SGroup* g=GetGroupAndOffset(aAddr,n);
	if (!g)
		return KErrArgument;
	__KTRACE_OPT(KMMU2,Kern::Printf("Group %d index %04x",g-iGroups,n));
	TBitMapAllocator& bma=*g->iBma;
	if (bma.NotFree(n,1))
		{
		__KTRACE_OPT(KMMU,Kern::Printf("Page already allocated"));
		return KErrAlreadyExists;			// page is already allocated
		}
	bma.Alloc(n,1);
	--iTotalFreeRamPages;
	TInt pwb=g->iPwrBlock;
	if (++iPowerBlockPages[pwb]==1)
		iPowerState|=(1u<<pwb);
	__KTRACE_OPT(KMMU,Kern::Printf("Total free RAM pages now = %d",iTotalFreeRamPages));
	return KErrNone;
	}

TInt DRamAllocator::FreeRamPage(TPhysAddr aAddr)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("FreeRamPage %08x",aAddr));
	TInt n;
	SGroup* g=GetGroupAndOffset(aAddr,n);
	if (!g)
		return KErrArgument;
	__KTRACE_OPT(KMMU2,Kern::Printf("Group %d index %04x",g-iGroups,n));
	TBitMapAllocator& bma=*g->iBma;
	bma.Free(n);
	++iTotalFreeRamPages;
	TInt pwb=g->iPwrBlock;
	if (--iPowerBlockPages[pwb]==0)
		iPowerState&=~(1u<<pwb);
	return KErrNone;
	}

void DRamAllocator::FreeRamPages(TPhysAddr* aPageList, TInt aNumPages)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("FreeRamPages count=%08x",aNumPages));
	while(aNumPages--)
		{
		TPhysAddr first_pa=*aPageList++;
		if (first_pa==NULL_PAGE)
			continue;
		TInt ix;
		SGroup* g=GetGroupAndOffset(first_pa,ix);
		if (!g)
			continue;
		TBitMapAllocator& bma=*g->iBma;
		TInt gp_rem=bma.iSize-ix;
		__KTRACE_OPT(KMMU,Kern::Printf("1st PA=%08x Group %d index %04x",first_pa,g-iGroups,ix));
		TInt n=1;
		TPhysAddr pa=first_pa+iPageSize;
		while (--gp_rem && aNumPages && *aPageList==pa)
			{
			++n;
			--aNumPages;
			++aPageList;
			pa+=iPageSize;
			}
		__KTRACE_OPT(KMMU2,Kern::Printf("%d consecutive pages, gp_rem=%x, %d remaining pages",n,gp_rem,aNumPages));
		bma.Free(ix,n);
		iTotalFreeRamPages+=n;
		TInt pwb=g->iPwrBlock;
		if ((iPowerBlockPages[pwb]-=n)==0)
			iPowerState&=~(1u<<pwb);
		}
	}

/**
@return 0 on success, on failure, the number of extra pages required to fulfill the request
*/
TInt DRamAllocator::AllocRamPages(TPhysAddr* aPageList, TInt aNumPages)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("AllocRamPages %x",aNumPages));
	TInt numMissing = aNumPages-iTotalFreeRamPages;
	if (numMissing>0)
		return numMissing;
	iTotalFreeRamPages-=aNumPages;
	TInt gix;
	for (gix=0; aNumPages && gix<iNumGroups; ++gix)
		{
		TInt group=iGroupPowerOrder?iGroupPowerOrder[gix]:gix;
		SGroup& g=iGroups[group];
		TBitMapAllocator& bma=*g.iBma;
		TPhysAddr gpb=g.iPhysBase;
		TInt got=bma.AllocList(aNumPages, (TInt*)aPageList);
		if (got)
			{
			TInt pwb=g.iPwrBlock;
			TPhysAddr* pE=aPageList+got;
			while(aPageList<pE)
				{
				TInt ix=*aPageList;
				*aPageList++=gpb+(ix<<iPageShift);
				__KTRACE_OPT(KMMU,Kern::Printf("Got page @%08x",gpb+(ix<<iPageShift)));
				}
			aNumPages-=got;
			iPowerBlockPages[pwb]+=got;
			iPowerState |= (1u<<pwb);
			}
		}
	__ASSERT_ALWAYS(aNumPages==0, Panic(EAllocRamPagesInconsistent));
	return 0;
	}

TInt DRamAllocator::FindContiguousRam(TInt aNumPages, TInt aAlignWrtPage, TUint8* aPermute, TInt& aPageNum)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("FindContiguousRam np=%d align=%d",aNumPages,aAlignWrtPage));
	TUint32 alignsize=1u<<aAlignWrtPage;
	TUint32 alignmask=alignsize-1;
	__KTRACE_OPT(KMMU,Kern::Printf("alignsize=%08x alignmask=%08x",alignsize,alignmask));
	TInt base=KErrNotFound;
	TInt gplen=0;
	TInt carry=0;
	TInt gix;
	for (gix=0; gix<iNumGroups; ++gix)
		{
		TInt group=aPermute?aPermute[gix]:gix;
		SGroup& g=iGroups[group];
		TBitMapAllocator& bma=*g.iBma;
		TInt gpb=TInt(g.iPhysBase>>iPageShift);
		if (gpb!=base+gplen)
			{
			// this group is not contiguous with previous one
			carry=0;
			}
		base=gpb;
		gplen=bma.iSize;
		__KTRACE_OPT(KMMU,Kern::Printf("FCR: base=%08x gplen=%08x carry=%08x",base,gplen,carry));
		TInt l;
		TInt r=bma.AllocAligned(aNumPages, aAlignWrtPage, base, EFalse, carry, l);
		__KTRACE_OPT(KMMU,Kern::Printf("FCR: r=%08x",r));
		if (r>=0)
			{
			TInt p=(base+r-carry+alignmask)&~alignmask;
			aPageNum=g.iNumBase+p-base;
			return p;
			}
		}
	return KErrNotFound;
	}

TInt DRamAllocator::AllocContiguousRam(TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("AllocContiguousRam size %08x align %d",aSize,aAlign));
	TInt npages=(aSize+iPageSize-1)>>iPageShift;
	TInt align_wrt_page=Max(aAlign-iPageShift,0);

	TInt pagenum;
	TInt found=FindContiguousRam(npages, align_wrt_page, iGroupPowerOrder, pagenum);
	if (found<0 && iGroupPowerOrder)
		found=FindContiguousRam(npages, align_wrt_page, NULL, pagenum);
	if (found<0)
		return KErrNoMemory;
	aPhysAddr=TPhysAddr(found)<<iPageShift;
	__KTRACE_OPT(KMMU,Kern::Printf("AllocContiguousRam returns %08x(%x)",aPhysAddr,pagenum));
	MarkPagesAllocated(pagenum, npages);
	return KErrNone;
	}

TInt DRamAllocator::SetPhysicalRamState(TPhysAddr aBase, TInt aSize, TBool aState)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("SetPhysicalRamState(%08x,%x,%d)",aBase,aSize,aState?1:0));
	TUint32 m=iPageSize-1;
	aSize+=(aBase&m);
	aBase&=~m;
	TInt npages=(aSize+m)>>iPageShift;
	__KTRACE_OPT(KMMU,Kern::Printf("Rounded base %08x npages=%x",aBase,npages));
	TInt ix0;
	SGroup* g0=GetGroupAndOffset(aBase,ix0);
	if (!g0)
		return KErrArgument;
	if ((TUint32)aSize>iPhysAddrTop-aBase)
		return KErrArgument;
	SGroup* g=g0;
	SGroup* gE=iGroups+iNumGroups;
	TPhysAddr base=aBase;
	TInt n=npages;
	TInt ix=ix0;
	TInt r=KErrNone;
	TInt c=-1;
	__KTRACE_OPT(KMMU2,Kern::Printf("Group %d index %x g=%08x gE=%08x n=%x base=%08x",g-iGroups,ix,g,gE,n,base));
	for (; n && g<gE && g->iPhysBase+(ix<<iPageShift)==base ; ++g, n-=c, ix=0, base+=(TPhysAddr(c)<<iPageShift))
		{
		TBitMapAllocator& bma=*g->iBma;
		TInt gp_rem=bma.iSize-ix;
		c=Min(n, gp_rem);
		__KTRACE_OPT(KMMU2,Kern::Printf("Group %d pages %x+%x base %08x",g-iGroups,ix,c,base));
		if(aState)
			{
			if(bma.NotAllocated(ix,c))
				r=KErrGeneral;
			}
		else
			{
			if(bma.NotFree(ix,c))
				r=KErrInUse;
			}
		}
	if (n)
		return KErrArgument;	// not all of the specified range exists
	if (r!=KErrNone)
		return r;				// some pages were already free/allocated
	iTotalFreeRamPages += (aState ? npages : -npages);
	for (g=g0, n=npages, ix=ix0; n; ++g, n-=c, ix=0)
		{
		TBitMapAllocator& bma=*g->iBma;
		TInt pwb=g->iPwrBlock;
		TInt& p=iPowerBlockPages[pwb];
		TUint32 pm=1u<<pwb;
		TInt gp_rem=bma.iSize-ix;
		c=Min(n, gp_rem);
		__KTRACE_OPT(KMMU2,Kern::Printf("Group %d pages %x+%x base %08x",g-iGroups,ix,c,base));
		aState ? (bma.Free(ix,c), (p||(iPowerState|=pm)), p+=c) : (bma.Alloc(ix,c), ((p-=c)||(iPowerState&=~pm)) );
		}
	return KErrNone;
	}

