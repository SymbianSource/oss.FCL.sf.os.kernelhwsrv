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
// e32\memmodel\epoc\multiple\x86\xmmu.cpp
// 
//

#include <x86_mem.h>
#include <mmubase.inl>
#include <ramcache.h>
#include "execs.h"
#include <defrag.h>

extern "C" void DoTotalInvalidateTLB();

// Constants for X86 MMU
const TUint32 KPdePtePresent=0x01;
const TUint32 KPdePteWrite=0x02;
const TUint32 KPdePteUser=0x04;
const TUint32 KPdePteWriteThrough=0x08;
const TUint32 KPdePteUncached=0x10;
const TUint32 KPdePteAccessed=0x20;
const TUint32 KPdePteDirty=0x40;
const TUint32 KPdeLargePage=0x80;						// Pentium and above, not 486
const TUint32 KPdePteGlobal=0x100;						// P6 and above, not 486 or Pentium
const TUint32 KPdePtePhysAddrMask=0xfffff000u;
const TUint32 KPdeLargePagePhysAddrMask=0xffc00000u;	// Pentium and above, not 486

const TPde KPdPdePerm=KPdePtePresent|KPdePteWrite;
const TPte KPdPtePerm=KPdePtePresent|KPdePteWrite;
const TPde KPtPdePerm=KPdePtePresent|KPdePteWrite;
const TPte KPtPtePerm=KPdePtePresent|KPdePteWrite;
const TPde KPtInfoPdePerm=KPdePtePresent|KPdePteWrite;
const TPte KPtInfoPtePerm=KPdePtePresent|KPdePteWrite;
const TPde KRomPdePerm=KPdePtePresent|KPdePteWrite|KPdePteUser;
const TPte KRomPtePerm=KPdePtePresent|KPdePteUser;
const TPde KShadowPdePerm=KPdePtePresent|KPdePteWrite|KPdePteUser;
const TPte KShadowPtePerm=KPdePtePresent|KPdePteWrite|KPdePteUser;	// unfortunately there's no RWRO

// Permissions for each chunk type

const TPde KStandardPtePerm=KPdePtePresent|KPdePteWrite|KPdePteUser;
const TPte KPdePermNONO=KPdePtePresent|KPdePteWrite|KPdePteUser;
const TPte KPdePermRONO=KPdePtePresent;
const TPte KPdePermRORO=KPdePtePresent|KPdePteUser;
const TPte KPdePermRWNO=KPdePtePresent|KPdePteWrite;
const TPte KPdePermRWRW=KPdePtePresent|KPdePteWrite|KPdePteUser;

LOCAL_D const TPte ChunkPtePermissions[ENumChunkTypes] =
	{
	KStandardPtePerm|KPdePteGlobal,		// EKernelData
	KStandardPtePerm|KPdePteGlobal,		// EKernelStack
	KPdePermRWNO|KPdePteGlobal,			// EKernelCode - loading
	KPdePermRWNO,						// EDll (used for global code) - loading
	KPdePermRORO,						// EUserCode
	KStandardPtePerm,					// ERamDrive
	KStandardPtePerm,					// EUserData
	KStandardPtePerm,					// EDllData
	KStandardPtePerm,					// EUserSelfModCode
	KStandardPtePerm,					// ESharedKernelSingle
	KStandardPtePerm,					// ESharedKernelMultiple
	KStandardPtePerm,					// ESharedIo
	KStandardPtePerm|KPdePteGlobal,		// ESharedKernelMirror
	KStandardPtePerm|KPdePteGlobal,		// EKernelMessage
	};

LOCAL_D const TPde ChunkPdePermissions[ENumChunkTypes] =
	{
	KPdePermRWNO,			// EKernelData
	KPdePermRWNO,			// EKernelStack
	KPdePermRWNO,			// EKernelCode
	KPdePermRWRW,			// EDll
	KPdePermRWRW,			// EUserCode
	KPdePermRWRW,			// ERamDrive
	KPdePermRWRW,			// EUserData
	KPdePermRWRW,			// EDllData
	KPdePermRWRW,			// EUserSelfModCode
	KPdePermRWRW,			// ESharedKernelSingle
	KPdePermRWRW,			// ESharedKernelMultiple
	KPdePermRWRW,			// ESharedIo
	KPdePermRWNO,			// ESharedKernelMirror
	KPdePermRWNO,			// EKernelMessage
	};

#if defined(KMMU)
extern "C" void __DebugMsgFlushTLB()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("FlushTLB"));
	}

extern "C" void __DebugMsgLocalFlushTLB()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("FlushTLB"));
	}

extern "C" void __DebugMsgTotalFlushTLB()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("TotalFlushTLB"));
	}

extern "C" void __DebugMsgINVLPG(int a)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("INVLPG(%08x)",a));
	}
#endif

// Inline functions for simple transformations
inline TLinAddr PageTableLinAddr(TInt aId)
	{
	return (KPageTableBase+(aId<<KPageTableShift));
	}

inline TPte* PageTable(TInt aId)
	{
	return (TPte*)(KPageTableBase+(aId<<KPageTableShift));
	}

inline TLinAddr PageDirectoryLinAddr(TInt aOsAsid)
	{
	return (KPageDirectoryBase+(aOsAsid<<KPageTableShift));
	}

extern "C" {

void __fastcall DoInvalidateTLBForPage(TLinAddr /*aLinAddr*/);
void DoInvalidateTLB();
void DoLocalInvalidateTLB();

}


#ifdef __SMP__

TSpinLock ShadowSpinLock(TSpinLock::EOrderGenericPreHigh0);	// Used when stopping other CPUs

class TTLBIPI : public TGenericIPI
	{
public:
	TTLBIPI();

	static void InvalidateForPagesIsr(TGenericIPI*);
	static void LocalInvalidateIsr(TGenericIPI*);
	static void TotalInvalidateIsr(TGenericIPI*);
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
	__KTRACE_OPT(KMMU2,Kern::Printf("TLBLocInv"));
	DoLocalInvalidateTLB();
	}

void TTLBIPI::TotalInvalidateIsr(TGenericIPI*)
	{
	__KTRACE_OPT(KMMU2,Kern::Printf("TLBTotInv"));
	DoTotalInvalidateTLB();
	}

void TTLBIPI::InvalidateIsr(TGenericIPI*)
	{
	__KTRACE_OPT(KMMU2,Kern::Printf("TLBInv"));
	DoInvalidateTLB();
	}

void TTLBIPI::WaitAndInvalidateIsr(TGenericIPI* aTLBIPI)
	{
	__KTRACE_OPT(KMMU2,Kern::Printf("TLBWtInv"));
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
		__KTRACE_OPT(KMMU2,Kern::Printf("TLBInv %08x", a.iAddr[i]));
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

void TotalInvalidateTLB()
	{
	TTLBIPI ipi;
	NKern::Lock();
	DoTotalInvalidateTLB();
	ipi.QueueAllOther(&TTLBIPI::TotalInvalidateIsr);
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

#else
#define	InvalidateTLBForPage(a)		DoInvalidateTLBForPage(a)
#define	LocalInvalidateTLB()		DoLocalInvalidateTLB()
#define	TotalInvalidateTLB()		TotalInvalidateTLB()
#define	InvalidateTLB()				DoInvalidateTLB()
#endif


TPte* SafePageTableFromPde(TPde aPde)
	{
	if (aPde&KPdePtePresent)
		{
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(aPde);
		if (pi)
			{
			TInt id=pi->Offset();	// assumes page table size = page size
			return PageTable(id);
			}
		}
	return 0;
	}

TPte* SafePtePtrFromLinAddr(TLinAddr aAddress, TInt aOsAsid=0)
	{
	TPde pde = PageDirectory(aOsAsid)[aAddress>>KChunkShift];
	TPte* pt = SafePageTableFromPde(pde);
	if(pt)
		pt += (aAddress>>KPageShift)&(KChunkMask>>KPageShift);
	return pt;
	}

TPte* PtePtrFromLinAddr(TLinAddr aAddress, TInt aOsAsid=0)
	{
	TPde pde = PageDirectory(aOsAsid)[aAddress>>KChunkShift];
	SPageInfo* pi = SPageInfo::FromPhysAddr(pde);
	TInt id = (pi->Offset()<<KPtClusterShift) | ((pde>>KPageTableShift)&KPtClusterMask);
	TPte* pt = PageTable(id);
	pt += (aAddress>>KPageShift)&(KChunkMask>>KPageShift);
	return pt;
	}

TInt X86Mmu::LinearToPhysical(TLinAddr aAddr, TInt aSize, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList, TInt aOsAsid)
	{
	TPhysAddr physStart = LinearToPhysical(aAddr,aOsAsid);

	TInt pageShift = iPageShift;
	TUint32 page = aAddr>>pageShift<<pageShift;
	TUint32 lastPage = (aAddr+aSize-1)>>pageShift<<pageShift;
	TUint32* pageList = aPhysicalPageList;
	TUint32 nextPhys = LinearToPhysical(page,aOsAsid);
	TUint32 pageSize = 1<<pageShift;
	while(page<=lastPage)
		{
		TPhysAddr phys = LinearToPhysical(page,aOsAsid);
		if(pageList)
			*pageList++ = phys;
		if(phys!=nextPhys)
			nextPhys = KPhysAddrInvalid;
		else
			nextPhys += pageSize;
		page += pageSize;
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
	return KErrNone;
	}

TPhysAddr X86Mmu::LinearToPhysical(TLinAddr aLinAddr, TInt aOsAsid)
//
// Find the physical address corresponding to a given linear address in a specified OS
// address space. Call with system locked.
//
	{
	__KTRACE_OPT(KMMU2,Kern::Printf("X86Mmu::LinearToPhysical(%08x,%d)",aLinAddr,aOsAsid));
	TInt pdeIndex=aLinAddr>>KChunkShift;
	TPde pde=PageDirectory(aOsAsid)[pdeIndex];
	TPhysAddr pa=KPhysAddrInvalid;
	if (pde & KPdePtePresent)
		{
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pde);
		if (pi)
			{
			TInt id=pi->Offset();	// assumes page table size = page size
			TPte* pPte=PageTable(id);
			TPte pte=pPte[(aLinAddr&KChunkMask)>>KPageShift];
			if (pte & KPdePtePresent)
				{
				pa=(pte&KPdePtePhysAddrMask)+(aLinAddr&KPageMask);
				__KTRACE_OPT(KMMU2,Kern::Printf("Mapped with page table - returning %08x",pa));
				}
			}
		}
	return pa;
	}


TInt X86Mmu::PreparePagesForDMA(TLinAddr /*aLinAddr*/, TInt /*aSize*/, TInt /*aOsAsid*/, TPhysAddr* /*aPhysicalPageList*/)
	{
	return KErrNotSupported;
	}

TInt X86Mmu::ReleasePagesFromDMA(TPhysAddr* /*aPhysicalPageList*/, TInt /*aPageCount*/)
	{
	return KErrNotSupported;
	}

static const TInt PermissionLookup[8]=
	{
	0,
	EMapAttrReadSup|EMapAttrExecSup,
	0,
	EMapAttrWriteSup|EMapAttrReadSup|EMapAttrExecSup,
	0,
	EMapAttrReadUser|EMapAttrExecUser,
	0,
	EMapAttrWriteUser|EMapAttrReadUser|EMapAttrExecUser
	};

TInt X86Mmu::PageTableId(TLinAddr aAddr, TInt aOsAsid)
	{
	TInt id=-1;
	__KTRACE_OPT(KMMU,Kern::Printf("X86Mmu::PageTableId(%08x,%d)",aAddr,aOsAsid));
	TInt pdeIndex=aAddr>>KChunkShift;
	TPde pde=PageDirectory(aOsAsid)[pdeIndex];
	if (pde & KPdePtePresent)
		{
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pde);
		if (pi)
			id=pi->Offset();	// assumes page table size = page size
		}
	__KTRACE_OPT(KMMU,Kern::Printf("ID=%d",id));
	return id;
	}

// Used only during boot for recovery of RAM drive
TInt X86Mmu::BootPageTableId(TLinAddr aAddr, TPhysAddr& aPtPhys)
	{
	TInt id=KErrNotFound;
	__KTRACE_OPT(KMMU,Kern::Printf("X86Mmu:BootPageTableId(%08x,&)",aAddr));
	TPde* kpd=(TPde*)KPageDirectoryBase;	// kernel page directory
	TInt pdeIndex=aAddr>>KChunkShift;
	TPde pde = kpd[pdeIndex];
	if (pde & KPdePtePresent)
		{
		aPtPhys = pde & KPdePtePhysAddrMask;
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pde);
		if (pi)
			{
			SPageInfo::TType type = pi->Type();
			if (type == SPageInfo::EPageTable)
				id=pi->Offset();	// assumes page table size = page size
			else if (type == SPageInfo::EUnused)
				id = KErrUnknown;
			}
		}
	__KTRACE_OPT(KMMU,Kern::Printf("ID=%d",id));
	return id;
	}

TBool X86Mmu::PteIsPresent(TPte aPte)
	{
	return aPte & KPdePtePresent;
	}

TPhysAddr X86Mmu::PtePhysAddr(TPte aPte, TInt /*aPteIndex*/)
	{
	return aPte & KPdePtePhysAddrMask;
	}

TPhysAddr X86Mmu::PdePhysAddr(TLinAddr aAddr)
	{
	TPde* kpd = (TPde*)KPageDirectoryBase;	// kernel page directory
	TPde pde = kpd[aAddr>>KChunkShift];
	if (pde & (KPdePtePresent|KPdeLargePage) == (KPdePtePresent|KPdeLargePage))
		return pde & KPdeLargePagePhysAddrMask;
	return KPhysAddrInvalid;
	}

void X86Mmu::Init1()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("X86Mmu::Init1"));

	TUint pge = TheSuperPage().iCpuId & EX86Feat_PGE;
	iPteGlobal = pge ? KPdePteGlobal : 0;	
	X86_UseGlobalPTEs = pge!=0;

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
	iPtInfoPtePerm=KPtInfoPtePerm|iPteGlobal;
	iPtPtePerm=KPtPtePerm|iPteGlobal;
	iPtPdePerm=KPtPdePerm;
	iUserCodeLoadPtePerm=KPdePermRWNO;
	iKernelCodePtePerm=KPdePermRONO|iPteGlobal;
	iTempAddr=KTempAddr;
	iSecondTempAddr=KSecondTempAddr;

	TUint pse = TheSuperPage().iCpuId & EX86Feat_PSE;
	iMapSizes = pse ? KPageSize|KChunkSize : KPageSize;

	iDecommitThreshold=0;		// no cache consistency issues on decommit
	iRomLinearBase = ::RomHeaderAddress;
	iRomLinearEnd = KRomLinearEnd;
	iShadowPtePerm = KShadowPtePerm;
	iShadowPdePerm = KShadowPdePerm;

	// Mmu data
	TInt total_ram=TheSuperPage().iTotalRamSize;

	iNumOsAsids=1024;
	iNumGlobalPageDirs=1;
	//iOsAsidAllocator;			// dynamically allocated - Init2
	iGlobalPdSize=KPageTableSize;
	iGlobalPdShift=KPageTableShift;
	iLocalPdSize=0;
	iLocalPdShift=0;
	iAsidGroupSize=KChunkSize/KPageTableSize;
	iAsidGroupMask=iAsidGroupSize-1;
	iAsidGroupShift=iChunkShift-iGlobalPdShift;
	iAliasSize=KPageSize;
	iAliasMask=KPageMask;
	iAliasShift=KPageShift;
	iUserLocalBase=KUserLocalDataBase;
	iUserSharedBase=KUserSharedDataBase;
	iAsidInfo=(TUint32*)KAsidInfoBase;
	iPdeBase=KPageDirectoryBase;
	iPdPtePerm=KPdPtePerm|iPteGlobal;
	iPdPdePerm=KPdPdePerm;
	iRamDriveMask=0x00f00000;
	iGlobalCodePtePerm=KPdePermRORO|iPteGlobal;

	iMaxDllDataSize=Min(total_ram/2, 0x08000000);				// phys RAM/2 up to 128Mb
	iMaxDllDataSize=(iMaxDllDataSize+iChunkMask)&~iChunkMask;	// round up to chunk size
	iMaxUserCodeSize=Min(total_ram, 0x10000000);				// phys RAM up to 256Mb
	iMaxUserCodeSize=(iMaxUserCodeSize+iChunkMask)&~iChunkMask;	// round up to chunk size
	iUserLocalEnd=iUserSharedBase-iMaxDllDataSize;
	iUserSharedEnd=KUserSharedDataEnd-iMaxUserCodeSize;
	iDllDataBase=iUserLocalEnd;
	iUserCodeBase=iUserSharedEnd;
	__KTRACE_OPT(KMMU,Kern::Printf("ULB %08x ULE %08x USB %08x USE %08x",iUserLocalBase,iUserLocalEnd,
																			iUserSharedBase,iUserSharedEnd));
	__KTRACE_OPT(KMMU,Kern::Printf("DDB %08x UCB %08x",iDllDataBase,iUserCodeBase));

	// X86Mmu data

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

	Mmu::Init1();
	}

void X86Mmu::DoInit2()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("X86Mmu::DoInit2"));
	iTempPte=PageTable(PageTableId(iTempAddr,0))+((iTempAddr&KChunkMask)>>KPageShift);
	iSecondTempPte=PageTable(PageTableId(iSecondTempAddr,0))+((iSecondTempAddr&KChunkMask)>>KPageShift);
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("iTempAddr=%08x, iTempPte=%08x, iSecondTempAddr=%08x, iSecondTempPte=%08x",
			iTempAddr, iTempPte, iSecondTempAddr, iSecondTempPte));
	CreateKernelSection(KKernelSectionEnd, iAliasShift);
	CreateUserGlobalSection(KUserGlobalDataBase, KUserGlobalDataEnd);
	iUserHwChunkAllocator=THwChunkAddressAllocator::New(0, iUserGlobalSection);
	__ASSERT_ALWAYS(iUserHwChunkAllocator, Panic(ECreateUserGlobalSectionFailed));
	Mmu::DoInit2();
	}

#ifndef __MMU_MACHINE_CODED__
void X86Mmu::MapRamPages(TInt aId, SPageInfo::TType aType, TAny* aPtr, TUint32 aOffset, const TPhysAddr* aPageList, TInt aNumPages, TPte aPtePerm)
//
// Map a list of physical RAM pages into a specified page table with specified PTE permissions.
// Update the page information array.
// Call this with the system locked.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("X86Mmu::MapRamPages() id=%d type=%d ptr=%08x off=%08x n=%d perm=%08x",
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

void X86Mmu::MapPhysicalPages(TInt aId, SPageInfo::TType aType, TAny* aPtr, TUint32 aOffset, TPhysAddr aPhysAddr, TInt aNumPages, TPte aPtePerm)
//
// Map consecutive physical pages into a specified page table with specified PTE permissions.
// Update the page information array if RAM pages are being mapped.
// Call this with the system locked.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("X86Mmu::MapPhysicalPages() id=%d type=%d ptr=%08x off=%08x phys=%08x n=%d perm=%08x",
			aId, aType, aPtr, aOffset, aPhysAddr, aNumPages, aPtePerm));
	SPageTableInfo& ptinfo=iPtInfo[aId];
	ptinfo.iCount+=aNumPages;
	aOffset>>=KPageShift;
	TInt ptOffset=aOffset & KPagesInPDEMask;				// entry number in page table
	TPte* pPte=(TPte*)(PageTableLinAddr(aId))+ptOffset;		// address of first PTE
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
			++aOffset;										// increment offset for next page
			__KTRACE_OPT(KMMU,Kern::Printf("I: %d %08x %08x",aType,aPtr,aOffset));
			++pi;
			}
		}
	__DRAIN_WRITE_BUFFER;
	}

void X86Mmu::MapVirtual(TInt /*aId*/, TInt /*aNumPages*/)
//
// Used in the implementation of demand paging - not supported on x86
//
	{
	MM::Panic(MM::EOperationNotSupported);
	}

void X86Mmu::RemapPage(TInt /*aId*/, TUint32 /*aAddr*/, TPhysAddr /*aOldAddr*/, TPhysAddr /*aNewAddr*/, TPte /*aPtePerm*/, DProcess* /*aProcess*/)
	{
	MM::Panic(MM::EOperationNotSupported);
	}

void X86Mmu::RemapPageByAsid(TBitMapAllocator* /*aOsAsids*/, TLinAddr /*aLinAddr*/, TPhysAddr /*aOldAddr*/, TPhysAddr /*aNewAddr*/, TPte /*aPtePerm*/)
	{
	MM::Panic(MM::EOperationNotSupported);
	}

TInt X86Mmu::UnmapPages(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TBool aSetPagesFree, TInt& aNumPtes, TInt& aNumFree, DProcess*)
//
// Unmap a specified area at address aAddr in page table aId. Place physical addresses of unmapped
// pages into aPageList, and count of unmapped pages into aNumPtes.
// Return number of pages still mapped using this page table.
// Call this with the system locked.
	{
	__KTRACE_OPT(KMMU,Kern::Printf("X86Mmu::UnmapPages() id=%d off=%08x n=%d pl=%08x set-free=%08x",aId,aAddr,aNumPages,aPageList,aSetPagesFree));
	TInt ptOffset=(aAddr&KChunkMask)>>KPageShift;			// entry number in page table
	TPte* pPte=PageTable(aId)+ptOffset;						// address of first PTE
	TInt np=0;
	TInt nf=0;
#ifdef __SMP__
	TTLBIPI ipi;
#endif
	while(aNumPages--)
		{
		TPte pte=*pPte;						// get original PTE
		*pPte++=0;							// clear PTE
		if (pte & KPdePtePresent)
			{
#ifdef __SMP__
			ipi.AddAddress(aAddr);
#else
			InvalidateTLBForPage(aAddr);	// flush any corresponding TLB entry
#endif
			++np;							// count unmapped pages
			TPhysAddr pa=pte & KPdePtePhysAddrMask;	// physical address of unmapped page
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
#ifdef __SMP__
	ipi.InvalidateList();
#endif
	aNumPtes=np;
	aNumFree=nf;
	SPageTableInfo& ptinfo=iPtInfo[aId];
	TInt r=(ptinfo.iCount-=np);
	__DRAIN_WRITE_BUFFER;
	__KTRACE_OPT(KMMU,Kern::Printf("Pages recovered %d Pages remaining %d NF=%d",np,r,nf));
	return r;								// return number of pages remaining in this page table
	}

TInt X86Mmu::UnmapUnownedPages(TInt aId, TUint32 aAddr, TInt aNumPages, TPhysAddr* aPageList, TLinAddr* aLAPageList, TInt& aNumPtes, TInt& aNumFree, DProcess*)
//
// Unmap a specified area at address aAddr in page table aId. Place physical addresses of unmapped
// pages into aPageList, and count of unmapped pages into aNumPtes.
// Return number of pages still mapped using this page table.
// Call this with the system locked.
	{
	__KTRACE_OPT(KMMU,Kern::Printf("X86Mmu::UnmapPages() id=%d off=%08x n=%d pl=%08x",aId,aAddr,aNumPages,aPageList));
	TInt ptOffset=(aAddr&KChunkMask)>>KPageShift;			// entry number in page table
	TPte* pPte=PageTable(aId)+ptOffset;						// address of first PTE
	TInt np=0;
	TInt nf=0;
#ifdef __SMP__
	TTLBIPI ipi;
#endif
	while(aNumPages--)
		{
		TPte pte=*pPte;						// get original PTE
		*pPte++=0;							// clear PTE
		if (pte & KPdePtePresent)
			{
#ifdef __SMP__
			ipi.AddAddress(aAddr);
#else
			InvalidateTLBForPage(aAddr);	// flush any corresponding TLB entry
#endif
			++np;							// count unmapped pages
			TPhysAddr pa=pte & KPdePtePhysAddrMask;	// physical address of unmapped page

			nf++;
			*aPageList++=pa;				// store in page list
			*aLAPageList++ = aAddr;
			}
		aAddr+=KPageSize;
		}
#ifdef __SMP__
	ipi.InvalidateList();
#endif
	aNumPtes=np;
	aNumFree=nf;
	SPageTableInfo& ptinfo=iPtInfo[aId];
	TInt r=(ptinfo.iCount-=np);
	__DRAIN_WRITE_BUFFER;
	__KTRACE_OPT(KMMU,Kern::Printf("Pages recovered %d Pages remaining %d NF=%d",np,r,nf));
	return r;								// return number of pages remaining in this page table
	}

TInt X86Mmu::UnmapVirtual(TInt /*aId*/, TUint32 /*aAddr*/, TInt /*aNumPages*/, TPhysAddr* /*aPageList*/, TBool /*aSetPagesFree*/, TInt& /*aNumPtes*/, TInt& /*aNumFree*/, DProcess* /*aProcess*/)
//
// Used in the implementation of demand paging - not supported on x86
//
	{
	MM::Panic(MM::EOperationNotSupported);
	return 0; // keep compiler happy
	}

TInt X86Mmu::UnmapUnownedVirtual(TInt /*aId*/, TUint32 /*aAddr*/, TInt /*aNumPages*/, TPhysAddr* /*aPageList*/, TLinAddr*  /*aLALinAddr*/, TInt& /*aNumPtes*/, TInt& /*aNumFree*/, DProcess* /*aProcess*/)
//
// Used in the implementation of demand paging - not supported on x86
//
	{
	MM::Panic(MM::EOperationNotSupported);
	return 0; // keep compiler happy
	}

void X86Mmu::DoAssignPageTable(TInt aId, TLinAddr aAddr, TPde aPdePerm, const TAny* aOsAsids)
//
// Assign an allocated page table to map a given linear address with specified permissions.
// This should be called with the system unlocked and the MMU mutex held.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("X86Mmu::DoAssignPageTable %d to %08x perm %08x asid %08x",aId,aAddr,aPdePerm,aOsAsids));
	TLinAddr ptLin=PageTableLinAddr(aId);
	TPhysAddr ptPhys=LinearToPhysical(ptLin,0);
	TInt pdeIndex=TInt(aAddr>>KChunkShift);
	TInt os_asid=(TInt)aOsAsids;
	if (TUint32(os_asid)<TUint32(iNumOsAsids))
		{
		// single OS ASID
		TPde* pageDir=PageDirectory(os_asid);
		NKern::LockSystem();
		pageDir[pdeIndex]=ptPhys|aPdePerm;
		NKern::UnlockSystem();
		__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x",ptPhys|aPdePerm,pageDir+pdeIndex));
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
			NKern::UnlockSystem();
			__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x",ptPhys|aPdePerm,pageDir+pdeIndex));
			--num_os_asids;
			}
		}
	__DRAIN_WRITE_BUFFER;
	}

void X86Mmu::RemapPageTableSingle(TPhysAddr aOld, TPhysAddr aNew, TLinAddr aAddr, TInt aOsAsid)
	{
	MM::Panic(MM::EOperationNotSupported);
	}

void X86Mmu::RemapPageTableGlobal(TPhysAddr aOld, TPhysAddr aNew, TLinAddr aAddr)
	{
	MM::Panic(MM::EOperationNotSupported);
	}

void X86Mmu::RemapPageTableMultiple(TPhysAddr aOld, TPhysAddr aNew, TLinAddr aAddr, const TAny* aOsAsids)
	{
	MM::Panic(MM::EOperationNotSupported);
	}

void X86Mmu::RemapPageTableAliases(TPhysAddr aOld, TPhysAddr aNew)
	{
	MM::Panic(MM::EOperationNotSupported);
	}

void X86Mmu::DoUnassignPageTable(TLinAddr aAddr, const TAny* aOsAsids)
//
// Unassign a now-empty page table currently mapping the specified linear address.
// We assume that TLB and/or cache flushing has been done when any RAM pages were unmapped.
// This should be called with the system unlocked and the MMU mutex held.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("X86Mmu::DoUnassignPageTable at %08x a=%08x",aAddr,aOsAsids));
	TInt pdeIndex=TInt(aAddr>>KChunkShift);
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
		__KTRACE_OPT(KMMU,Kern::Printf("Clearing PDE at %08x",pageDir+pdeIndex));

		// remove any aliases of the page table...
		TUint ptId = pde>>KPageTableShift;
		while(!iAliasList.IsEmpty())
			{
			next = iAliasList.First()->Deque();
			checkedList.Add(next);
			DMemModelThread* thread = _LOFF(next, DMemModelThread, iAliasLink);
			if(thread->iAliasOsAsid==os_asid && (thread->iAliasPde>>KPageTableShift)==ptId)
				{
				// the page table is being aliased by the thread, so remove it...
				thread->iAliasPde = 0;
				}
			NKern::FlashSystem();
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
			pde = pageDir[pdeIndex];
			pageDir[pdeIndex]=0;
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
			DMemModelThread* thread = _LOFF(next, DMemModelThread, iAliasLink);
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

	__DRAIN_WRITE_BUFFER; // because page tables have been updated
	}
#endif

// Initialise page table at physical address aXptPhys to be used as page table aXptId
// to expand the virtual address range used for mapping page tables. Map the page table
// at aPhysAddr as page table aId using the expanded range.
// Assign aXptPhys to kernel's Page Directory.
// Called with system unlocked and MMU mutex held.
void X86Mmu::BootstrapPageTable(TInt aXptId, TPhysAddr aXptPhys, TInt aId, TPhysAddr aPhysAddr)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("X86Mmu::BootstrapPageTable xptid=%04x, xptphys=%08x, id=%04x, phys=%08x",
						aXptId, aXptPhys, aId, aPhysAddr));
	
	// put in a temporary mapping for aXptPhys
	*iTempPte = aXptPhys | KPtPtePerm | iPteGlobal;
	__DRAIN_WRITE_BUFFER;

	// clear XPT
	TPte* xpt=(TPte*)iTempAddr;
	memclr(xpt, KPageSize);

	// map XPT
	xpt[aXptId & KPagesInPDEMask] = aXptPhys | KPtPtePerm | iPteGlobal;

	// map other page table
	xpt[aId & KPagesInPDEMask] = aPhysAddr | KPtPtePerm | iPteGlobal;

	// remove temporary mapping
	iTempPte=0;
	__DRAIN_WRITE_BUFFER;
	InvalidateTLBForPage(iTempAddr);

	// initialise PtInfo...
	TLinAddr xptAddr = PageTableLinAddr(aXptId);
	iPtInfo[aXptId].SetGlobal(xptAddr>>KChunkShift);

	// map xpt...
	TInt pdeIndex=TInt(xptAddr>>KChunkShift);
	TPde* pageDir=PageDirectory(0);
	NKern::LockSystem();
	pageDir[pdeIndex]=aXptPhys|KPtPdePerm;
	__DRAIN_WRITE_BUFFER;
	NKern::UnlockSystem();				
	}

void X86Mmu::FixupXPageTable(TInt aId, TLinAddr aTempMap, TPhysAddr aOld, TPhysAddr aNew)
	{
	MM::Panic(MM::EOperationNotSupported);
	}

TInt X86Mmu::NewPageDirectory(TInt aOsAsid, TBool aSeparateGlobal, TPhysAddr& aPhysAddr, TInt& aNumPages)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("X86Mmu::NewPageDirectory(%d,%d)",aOsAsid,aSeparateGlobal));
	TInt r=AllocRamPages(&aPhysAddr,1, EPageFixed);
	if (r!=KErrNone)
		return r;
#ifdef BTRACE_KERNEL_MEMORY
	BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryMiscAlloc, 1<<KPageShift);
	Epoc::KernelMiscPages += 1;
#endif
	SPageInfo* pi = SPageInfo::FromPhysAddr(aPhysAddr);
	NKern::LockSystem();
	pi->SetPageDir(aOsAsid,0);
	NKern::UnlockSystem();
	aNumPages=1;
	return KErrNone;
	}

inline void CopyPdes(TPde* aDest, const TPde* aSrc, TLinAddr aBase, TLinAddr aEnd)
	{
	memcpy(aDest+(aBase>>KChunkShift), aSrc+(aBase>>KChunkShift), ((aEnd-aBase)>>KChunkShift)*sizeof(TPde));
	}

inline void ZeroPdes(TPde* aDest, TLinAddr aBase, TLinAddr aEnd)
	{
	memclr(aDest+(aBase>>KChunkShift), ((aEnd-aBase)>>KChunkShift)*sizeof(TPde));
	}

void X86Mmu::InitPageDirectory(TInt aOsAsid, TBool)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("X86Mmu::InitPageDirectory(%d)",aOsAsid));
	TPde* newpd=PageDirectory(aOsAsid);					// new page directory
	const TPde* kpd=(const TPde*)KPageDirectoryBase;	// kernel page directory
	ZeroPdes(newpd, 0x00000000, KUserSharedDataEnd);	// clear user mapping area
	ZeroPdes(newpd, KRamDriveStartAddress, KRamDriveEndAddress);	// don't copy RAM drive
	CopyPdes(newpd, kpd, KRomLinearBase, KUserGlobalDataEnd);		// copy ROM + user global
	CopyPdes(newpd, kpd, KRamDriveEndAddress, 0x00000000);			// copy kernel mappings
	__DRAIN_WRITE_BUFFER;
	}

void X86Mmu::ClearPageTable(TInt aId, TInt aFirstIndex)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("X86Mmu:ClearPageTable(%d,%d)",aId,aFirstIndex));
	TPte* pte=PageTable(aId);
	memclr(pte+aFirstIndex, KPageSize-aFirstIndex*sizeof(TPte));
	__DRAIN_WRITE_BUFFER;
	}

void X86Mmu::ApplyTopLevelPermissions(TLinAddr aAddr, TInt aOsAsid, TInt aNumPdes, TPde aPdePerm)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("X86Mmu::ApplyTopLevelPermissions %04x:%08x->%08x count %d",
												aOsAsid, aAddr, aPdePerm, aNumPdes));
	TInt ix=aAddr>>KChunkShift;
	TPde* pPde=PageDirectory(aOsAsid)+ix;
	TPde* pPdeEnd=pPde+aNumPdes;
	NKern::LockSystem();
	for (; pPde<pPdeEnd; ++pPde)
		{
		TPde pde=*pPde;
		if (pde)
			*pPde = (pde&KPdePtePhysAddrMask)|aPdePerm;
		}
	NKern::UnlockSystem();
	(aAddr>=KUserSharedDataEnd) ? InvalidateTLB() : LocalInvalidateTLB();
	__DRAIN_WRITE_BUFFER;
	}

void X86Mmu::ApplyPagePermissions(TInt aId, TInt aPageOffset, TInt aNumPages, TPte aPtePerm)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("X86Mmu::ApplyPagePermissions %04x:%03x+%03x perm %08x",
												aId, aPageOffset, aNumPages, aPtePerm));
	TPte* pPte=PageTable(aId)+aPageOffset;
	TPde* pPteEnd=pPte+aNumPages;
	TPte g=0;
	NKern::LockSystem();
	for (; pPte<pPteEnd; ++pPte)
		{
		TPte pte=*pPte;
		g |= pte;
		if (pte)
			*pPte = (pte&KPdePtePhysAddrMask)|aPtePerm;
		}
	NKern::UnlockSystem();
	(g & KPdePteGlobal) ? InvalidateTLB() : LocalInvalidateTLB();
	__DRAIN_WRITE_BUFFER;
	}


// Set up a page table (specified by aId) to map a 4Mb section of ROM containing aRomAddr
// using ROM at aOrigPhys.
void X86Mmu::InitShadowPageTable(TInt aId, TLinAddr aRomAddr, TPhysAddr aOrigPhys)
	{
	(void)aId, (void)aRomAddr, (void)aOrigPhys;
	FAULT();	// Never used
/*
	__KTRACE_OPT(KMMU, Kern::Printf("X86Mmu:InitShadowPageTable id=%04x aRomAddr=%08x aOrigPhys=%08x",
		aId, aRomAddr, aOrigPhys));
	TPte* ppte = PageTable(aId);
	TPte* ppte_End = ppte + KChunkSize/KPageSize;
	TPhysAddr phys = aOrigPhys - (aRomAddr & KChunkMask);
	for (; ppte<ppte_End; ++ppte, phys+=KPageSize)
		*ppte = phys | KRomPtePerm;
	__DRAIN_WRITE_BUFFER;
*/
	}

// Copy the contents of ROM at aRomAddr to a shadow page at physical address aShadowPhys
void X86Mmu::InitShadowPage(TPhysAddr aShadowPhys, TLinAddr aRomAddr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("X86Mmu:InitShadowPage aShadowPhys=%08x aRomAddr=%08x",
		aShadowPhys, aRomAddr));

	// put in a temporary mapping for aShadowPhys
	// make it noncacheable
	*iTempPte = aShadowPhys | KPtPtePerm | iPteGlobal;
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
void X86Mmu::AssignShadowPageTable(TInt aId, TLinAddr aRomAddr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("X86Mmu:AssignShadowPageTable aId=%04x aRomAddr=%08x",
		aId, aRomAddr));
	TLinAddr ptLin=PageTableLinAddr(aId);
	TPhysAddr ptPhys=LinearToPhysical(ptLin, 0);
	TPde* ppde = ::InitPageDirectory + (aRomAddr>>KChunkShift);
	TPde newpde = ptPhys | KShadowPdePerm;
	__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x", newpde, ppde));
#ifdef __SMP__
	TTLBIPI ipi;
	NKern::Lock();		// stop other processors passing this point
	ShadowSpinLock.LockOnly();
	ipi.QueueAllOther(&TTLBIPI::WaitAndInvalidateIsr);
	ipi.WaitEntry();	// wait for other processors to stop in the ISR
#endif
	TInt irq=NKern::DisableAllInterrupts();
	*ppde = newpde;		// map in the page table
	__DRAIN_WRITE_BUFFER;	// make sure new PDE written to main memory
	DoInvalidateTLB();	// completely flush TLB
	NKern::RestoreInterrupts(irq);
#ifdef __SMP__
	ipi.iFlag = 1;		// release other processors so they can flush their TLBs
	ipi.WaitCompletion();	// wait for other processors to flush their TLBs
	ShadowSpinLock.UnlockOnly();
	NKern::Unlock();
#endif
	}

void X86Mmu::DoUnmapShadowPage(TInt aId, TLinAddr aRomAddr, TPhysAddr aOrigPhys)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("X86Mmu:DoUnmapShadowPage, id=%04x lin=%08x origphys=%08x", aId, aRomAddr, aOrigPhys));
	TPte* ppte = PageTable(aId) + ((aRomAddr & KChunkMask)>>KPageShift);
	TPte newpte = aOrigPhys | KRomPtePerm;
	__KTRACE_OPT(KMMU,Kern::Printf("Writing PTE %08x to %08x", newpte, ppte));
#ifdef __SMP__
	TTLBIPI ipi;
	ipi.AddAddress(aRomAddr);
	NKern::Lock();		// stop other processors passing this point
	ShadowSpinLock.LockOnly();
	ipi.QueueAllOther(&TTLBIPI::WaitAndInvalidateIsr);
	ipi.WaitEntry();	// wait for other processors to stop
#endif
	TInt irq=NKern::DisableAllInterrupts();
	*ppte = newpte;
	__DRAIN_WRITE_BUFFER;
	DoInvalidateTLBForPage(aRomAddr);
	NKern::RestoreInterrupts(irq);
#ifdef __SMP__
	ipi.iFlag = 1;		// release other processors so they can flush their TLBs
	ipi.WaitCompletion();	// wait for other processors to flush their TLBs
	ShadowSpinLock.UnlockOnly();
	NKern::Unlock();
#endif
	}

TInt X86Mmu::UnassignShadowPageTable(TLinAddr /*aRomAddr*/, TPhysAddr /*aOrigPhys*/)
	{
	// not used since we use page mappings for the ROM
	return KErrGeneral;
	}

TInt X86Mmu::CopyToShadowMemory(TLinAddr aDest, TLinAddr aSrc, TUint32 aLength)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("X86Mmu:CopyToShadowMemory aDest=%08x aSrc=%08x aLength=%08x", aDest, aSrc, aLength));

	// Check that destination is ROM
	if (aDest<iRomLinearBase || (aDest+aLength) > iRomLinearEnd)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("X86Mmu:CopyToShadowMemory: Destination not entirely in ROM"));
		return KErrArgument;
		}

	// do operation with RamAlloc mutex held (to prevent shadow pages from being released from under us)
	Kern::MutexWait(*RamAllocatorMutex);

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
			__KTRACE_OPT(KMMU,Kern::Printf("X86Mmu:CopyToShadowMemory: No shadow page at this address"));
			r = KErrArgument;
			break;
			}

		//Temporarily map into writable memory and copy data. RamAllocator DMutex is required
		TLinAddr tempAddr = MapTemp (physAddr, aDest&~iPageMask);
		__KTRACE_OPT(KMMU, Kern::Printf("X86Mmu:CopyToShadowMemory Copy aDest=%08x aSrc=%08x aSize=%08x", tempAddr+(aDest&iPageMask), aSrc, copySize));
		memcpy ((TAny*)(tempAddr+(aDest&iPageMask)), (const TAny*)aSrc, copySize);  //Kernel-to-Kernel copy is presumed
		UnmapTemp();

		//Update variables for the next loop/page
		aDest+=copySize;
		aSrc+=copySize;
		aLength-=copySize;
		}

	Kern::MutexSignal(*RamAllocatorMutex);
	return r;
	}

void X86Mmu::DoFreezeShadowPage(TInt aId, TLinAddr aRomAddr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("X86Mmu:DoFreezeShadowPage aId=%04x aRomAddr=%08x",
		aId, aRomAddr));
	TPte* ppte = PageTable(aId) + ((aRomAddr & KChunkMask)>>KPageShift);
	TPte newpte = (*ppte & KPdePtePhysAddrMask) | KRomPtePerm;
	__KTRACE_OPT(KMMU,Kern::Printf("Writing PTE %08x to %08x", newpte, ppte));
	*ppte = newpte;
	__DRAIN_WRITE_BUFFER;
	InvalidateTLBForPage(aRomAddr);
	}

void X86Mmu::FlushShadow(TLinAddr aRomAddr)
	{
#ifdef __SMP__
	TTLBIPI ipi;
	ipi.AddAddress(aRomAddr);
	NKern::Lock();		// stop other processors passing this point
	ShadowSpinLock.LockOnly();
	ipi.QueueAllOther(&TTLBIPI::WaitAndInvalidateIsr);
	ipi.WaitEntry();	// wait for other processors to stop
	DoInvalidateTLBForPage(aRomAddr);
	ipi.iFlag = 1;		// release other processors so they can flush their TLBs
	ipi.WaitCompletion();	// wait for other processors to flush their TLBs
	ShadowSpinLock.UnlockOnly();
	NKern::Unlock();
#else
	InvalidateTLBForPage(aRomAddr);		// remove all TLB references to original ROM page
#endif
	}

void X86Mmu::Pagify(TInt aId, TLinAddr aLinAddr)
	{
	// Nothing to do on x86
	}

void X86Mmu::ClearRamDrive(TLinAddr aStart)
	{
	// clear the page directory entries corresponding to the RAM drive
	TPde* kpd=(TPde*)KPageDirectoryBase;	// kernel page directory
	ZeroPdes(kpd, aStart, KRamDriveEndAddress);
	__DRAIN_WRITE_BUFFER;
	}

// Generic cache/TLB flush function.
// Which things are flushed is determined by aMask.
void X86Mmu::GenericFlush(TUint32 aMask)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("GenericFlush %x",aMask));
	if (aMask&(EFlushDPermChg|EFlushIPermChg))
		InvalidateTLB();
	}

TPde X86Mmu::PdePermissions(TChunkType aChunkType, TBool aRO)
	{
	if (aChunkType==EUserData && aRO)
		return KPdePtePresent|KPdePteUser;
	return ChunkPdePermissions[aChunkType];
	}

TPte X86Mmu::PtePermissions(TChunkType aChunkType)
	{
	TPte pte=ChunkPtePermissions[aChunkType];
	return (pte&~KPdePteGlobal)|(pte&iPteGlobal);
	}

const TUint FBLK=(EMapAttrFullyBlocking>>12);
const TUint BFNC=(EMapAttrBufferedNC>>12);
const TUint BUFC=(EMapAttrBufferedC>>12);
const TUint L1UN=(EMapAttrL1Uncached>>12);
const TUint WTRA=(EMapAttrCachedWTRA>>12);
const TUint WTWA=(EMapAttrCachedWTWA>>12);
const TUint WBRA=(EMapAttrCachedWBRA>>12);
const TUint WBWA=(EMapAttrCachedWBWA>>12);
const TUint AWTR=(EMapAttrAltCacheWTRA>>12);
const TUint AWTW=(EMapAttrAltCacheWTWA>>12);
const TUint AWBR=(EMapAttrAltCacheWBRA>>12);
const TUint AWBW=(EMapAttrAltCacheWBWA>>12);

const TUint16 UNS=0xffffu;	// Unsupported attribute
const TUint16 SPE=0xfffeu;	// Special processing required

static const TUint16 CacheBuffAttributes[16]=
	{0x10,0x10,0x10,0x10,0x08,0x08,0x00,0x00, UNS, UNS, UNS, UNS, UNS, UNS, UNS,0x00};
static const TUint8 CacheBuffActual[16]=
	{FBLK,FBLK,FBLK,FBLK,WTRA,WTRA,WBWA,WBWA,FBLK,FBLK,FBLK,FBLK,FBLK,FBLK,FBLK,WBWA};

static const TUint8 ActualReadPrivilegeLevel[4]={1,1,4,4};	// RONO,RWNO,RORO,RWRW
static const TUint8 ActualWritePrivilegeLevel[4]={0,1,0,4};	// RONO,RWNO,RORO,RWRW

TInt X86Mmu::PdePtePermissions(TUint& aMapAttr, TPde& aPde, TPte& aPte)
	{
	__KTRACE_OPT(KMMU,Kern::Printf(">X86Mmu::PdePtePermissions, mapattr=%08x",aMapAttr));
	TUint read=aMapAttr & EMapAttrReadMask;
	TUint write=(aMapAttr & EMapAttrWriteMask)>>4;
	TUint exec=(aMapAttr & EMapAttrExecMask)>>8;
	TUint cache=(aMapAttr & EMapAttrL1CacheMask)>>12;
	TPte pte;
	// ignore L2 cache attributes for now - downgrade to L2 uncached

	// if execute access is greater than read, adjust read (since there are no separate execute permissions on X86)
	if (exec>read)
		read=exec;
	pte=0;
	if (write==0)
		{
		// read-only
		if (read>=4)
			pte=KPdePermRORO;			// user and supervisor read-only
		else
			pte=KPdePermRONO;			// supervisor r/o user no access
		}
	else if (write<4)
		{
		// only supervisor can write
		if (read>=4)
			pte=KPdePermRWRW;			// full access since no RWRO
		else
			pte=KPdePermRWNO;			// sup rw user no access
		}
	else
		pte=KPdePermRWRW;				// sup rw user rw
	read=ActualReadPrivilegeLevel[pte>>1];
	write=ActualWritePrivilegeLevel[pte>>1];
	TUint cbatt=CacheBuffAttributes[cache];
	TInt r=KErrNone;
	if (cbatt==UNS)
		r=KErrNotSupported;
	if (r==KErrNone)
		{
		cache=CacheBuffActual[cache];
		aPde=KPdePtePresent|KPdePteWrite|KPdePteUser;
		aPte=pte|cbatt|iPteGlobal;		// HW chunks can always be global
		aMapAttr=read|(write<<4)|(read<<8)|(cache<<12);
		}
	__KTRACE_OPT(KMMU,Kern::Printf("<X86Mmu::PdePtePermissions, r=%d, mapattr=%08x, pde=%08x, pte=%08x",
								r,aMapAttr,aPde,aPte));
	return r;
	}

THwChunkAddressAllocator* X86Mmu::MappingRegion(TUint aMapAttr)
	{
	TUint read=aMapAttr & EMapAttrReadMask;
	TUint write=(aMapAttr & EMapAttrWriteMask)>>4;
	TUint exec=(aMapAttr & EMapAttrExecMask)>>8;
	if (read>=4 || write>=4 || exec>=4)
		return iUserHwChunkAllocator;	// if any access in user mode, must put it in user global section
	return iHwChunkAllocator;
	}

void X86Mmu::Map(TLinAddr aLinAddr, TPhysAddr aPhysAddr, TInt aSize, TPde aPdePerm, TPte aPtePerm, TInt aMapShift)
//
// Map a region of physical addresses aPhysAddr to aPhysAddr+aSize-1 to virtual address aLinAddr.
// Use permissions specified by aPdePerm and aPtePerm. Use mapping sizes up to and including (1<<aMapShift).
// Assume any page tables required are already assigned.
// aLinAddr, aPhysAddr, aSize must be page-aligned.
//
	{
	__KTRACE_OPT(KMMU, Kern::Printf("X86Mmu::Map lin=%08x phys=%08x size=%08x", aLinAddr, aPhysAddr, aSize));
	__KTRACE_OPT(KMMU, Kern::Printf("pde=%08x pte=%08x mapshift=%d", aPdePerm, aPtePerm, aMapShift));
	TPde lp_pde=aPtePerm|KPdeLargePage;
	TLinAddr la=aLinAddr;
	TPhysAddr pa=aPhysAddr;
	TInt remain=aSize;
	while (remain)
		{
		if (aMapShift>=KChunkShift && (la & KChunkMask)==0 && remain>=KChunkSize)
			{
			// use large pages
			TInt npdes=remain>>KChunkShift;
			const TBitMapAllocator& b=*iOsAsidAllocator;
			TInt num_os_asids=b.iSize-b.iAvail;
			TInt os_asid=0;
			for (; num_os_asids; ++os_asid)
				{
				if (b.NotAllocated(os_asid,1))
					continue;			// os_asid is not needed
				TPde* p_pde=PageDirectory(os_asid)+(la>>KChunkShift);
				TPde* p_pde_E=p_pde+npdes;
				TPde pde=pa|lp_pde;
				NKern::LockSystem();
				for (; p_pde < p_pde_E; pde+=KChunkSize)
					{
					__ASSERT_DEBUG(*p_pde==0, MM::Panic(MM::EPdeAlreadyInUse));
					__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x", pde, p_pde));
					*p_pde++=pde;
					}
				NKern::UnlockSystem();
				--num_os_asids;
				}
			npdes<<=KChunkShift;
			la+=npdes, pa+=npdes, remain-=npdes;
			continue;
			}
		// use normal pages
		TInt block_size = Min(remain, KChunkSize-(la&KChunkMask));
		TInt id=PageTableId(la, 0);
		__ASSERT_DEBUG(id>=0, MM::Panic(MM::EMmuMapNoPageTable));
		TPte* p_pte=PageTable(id)+((la&KChunkMask)>>KPageShift);
		TPte* p_pte_E = p_pte + (block_size>>KPageShift);
		TPte pte=pa|aPtePerm;
		SPageTableInfo& ptinfo=iPtInfo[id];
		NKern::LockSystem();
		for (; p_pte < p_pte_E; pte+=KPageSize)
			{
			__ASSERT_DEBUG(*p_pte==0, MM::Panic(MM::EPteAlreadyInUse));
			__KTRACE_OPT(KMMU,Kern::Printf("Writing PTE %08x to %08x", pte, p_pte));
			*p_pte++=pte;
			++ptinfo.iCount;
			NKern::FlashSystem();
			}
		NKern::UnlockSystem();
		la+=block_size, pa+=block_size, remain-=block_size;
		}
	}

void X86Mmu::Unmap(TLinAddr aLinAddr, TInt aSize)
//
// Remove all mappings in the specified range of addresses.
// Don't free page tables.
// aLinAddr, aSize must be page-aligned.
//
	{
	__KTRACE_OPT(KMMU, Kern::Printf("X86Mmu::Unmap lin=%08x size=%08x", aLinAddr, aSize));
#ifdef __SMP__
	TTLBIPI ipi;
#endif
	TLinAddr a=aLinAddr;
	TLinAddr end=a+aSize;
	__KTRACE_OPT(KMMU,Kern::Printf("a=%08x end=%08x",a,end));
	NKern::LockSystem();
	while(a!=end)
		{
		TInt pdeIndex=a>>KChunkShift;
		TLinAddr next=(pdeIndex<<KChunkShift)+KChunkSize;
		TInt to_do=Min(TInt(end-a), TInt(next-a))>>KPageShift;
		__KTRACE_OPT(KMMU,Kern::Printf("a=%08x next=%08x to_do=%d",a,next,to_do));
		TPde pde=::InitPageDirectory[pdeIndex];
		if ( (pde&(KPdePtePresent|KPdeLargePage))==(KPdePtePresent|KPdeLargePage) )
			{
			__ASSERT_DEBUG(!(a&KChunkMask), MM::Panic(MM::EUnmapBadAlignment));
			::InitPageDirectory[pdeIndex]=0;
#ifdef __SMP__
			ipi.AddAddress(a);
#else
			InvalidateTLBForPage(a);	// flush any corresponding TLB entry
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
				if (*ppte & KPdePtePresent)
					--ptinfo.iCount;
				*ppte=0;
#ifdef __SMP__
				ipi.AddAddress(a);
#else
				InvalidateTLBForPage(a);	// flush any corresponding TLB entry
#endif
				NKern::FlashSystem();
				}
			}
		else
			a += (to_do<<KPageShift);
		}
#ifdef __SMP__
	ipi.InvalidateList();
#endif
	NKern::UnlockSystem();
	}


void X86Mmu::ClearPages(TInt aNumPages, TPhysAddr* aPageList, TUint8 aClearByte)
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
		*iTempPte = pa | KPdePtePresent | KPdePteWrite | iPteGlobal;
		__DRAIN_WRITE_BUFFER;
		InvalidateTLBForPage(iTempAddr);
		memset((TAny*)iTempAddr, aClearByte, iPageSize);
		}
	*iTempPte=0;
	__DRAIN_WRITE_BUFFER;
	InvalidateTLBForPage(iTempAddr);
	}

TLinAddr X86Mmu::MapTemp(TPhysAddr aPage,TLinAddr /*aLinAddr*/,TInt aPages)
	{
	__ASSERT_MUTEX(RamAllocatorMutex);
	__ASSERT_DEBUG(!*iTempPte,MM::Panic(MM::ETempMappingAlreadyInUse));
	__ASSERT_DEBUG(aPages<=4,MM::Panic(MM::ETempMappingNoRoom));
	iTempMapCount = aPages;
	for (TInt i=0; i<aPages; i++)
		{
		iTempPte[i] = ((aPage&~KPageMask)+(i<<KPageShift)) | KPdePtePresent | KPdePteWrite | iPteGlobal
		__DRAIN_WRITE_BUFFER;
		InvalidateTLBForPage(iTempAddr+(i<<KPageShift));
		}
	return iTempAddr;
	}

TLinAddr X86Mmu::MapTemp(TPhysAddr aPage,TLinAddr aLinAddr,TInt aPages, TMemoryType)
	{
	return MapTemp(aPage, aLinAddr, aPages);
	}

TLinAddr X86Mmu::MapSecondTemp(TPhysAddr aPage,TLinAddr /*aLinAddr*/,TInt aPages)
	{
	__ASSERT_MUTEX(RamAllocatorMutex);
	__ASSERT_DEBUG(!*iSecondTempPte,MM::Panic(MM::ETempMappingAlreadyInUse));
	__ASSERT_DEBUG(aPages<=4,MM::Panic(MM::ETempMappingNoRoom));
	iSecondTempMapCount = aPages;
	for (TInt i=0; i<aPages; i++)
		{
		iSecondTempPte[i] = ((aPage&~KPageMask)+(i<<KPageShift)) | KPdePtePresent | KPdePteWrite | iPteGlobal
		__DRAIN_WRITE_BUFFER;
		InvalidateTLBForPage(iSecondTempAddr+(i<<KPageShift));
		}
	return iSecondTempAddr;
	}

void X86Mmu::UnmapTemp()
	{
	__ASSERT_MUTEX(RamAllocatorMutex);
	for (TInt i=0; i<iTempMapCount; i++)
		{
		iTempPte[i] = 0;
		__DRAIN_WRITE_BUFFER;
		InvalidateTLBForPage(iTempAddr+(i<<KPageShift));
		}
	}

void X86Mmu::UnmapSecondTemp()
	{
	__ASSERT_MUTEX(RamAllocatorMutex);
	for (TInt i=0; i<iSecondTempMapCount; i++)
		{
		iSecondTempPte[i] = 0;
		__DRAIN_WRITE_BUFFER;
		InvalidateTLBForPage(iSecondTempAddr+(i<<KPageShift));
		}
	}

void ExecHandler::UnlockRamDrive()
	{
	}

EXPORT_C void TInternalRamDrive::Unlock()
	{
	}

EXPORT_C void TInternalRamDrive::Lock()
	{
	}

TBool X86Mmu::ValidateLocalIpcAddress(TLinAddr aAddr,TInt aSize,TBool aWrite)
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
		DoInvalidateTLBForPage(aAddr);	// only need to do this processor since alias range is owned by the thread
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
	TInt mask = 2<<(end>>27);
	mask -= 1<<(aAddr>>27);
	if((local_mask&mask)!=mask)
		return EFalse;

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
	__ASSERT_SYSTEM_LOCK;

	if(TUint(aAddr^KIPCAlias)<TUint(KIPCAliasAreaSize))
		return KErrBadDescriptor; // prevent access to alias region

	// check if memory is in region which is safe to access with supervisor permissions...
	TBool okForSupervisorAccess = aPerm&(EMapAttrReadSup|EMapAttrWriteSup) ? 1 : 0;
	if(!okForSupervisorAccess)
		{
		if(aAddr>=0xc0000000) // address in kernel area (top 1GB)?
			return KErrBadDescriptor; // don't have permission
		TUint32 local_mask;
		if(aPerm&EMapAttrWriteUser)
			local_mask = aProcess->iAddressCheckMaskW;
		else
			local_mask = aProcess->iAddressCheckMaskR;
		okForSupervisorAccess = (local_mask>>(aAddr>>27))&1;
		}

	if(aAddr>=KUserSharedDataEnd) // if address is in global section, don't bother aliasing it...
		{
		if(iAliasLinAddr)
			RemoveAlias();
		aAliasAddr = aAddr;
		TInt maxSize = KChunkSize-(aAddr&KChunkMask);
		aAliasSize = aSize<maxSize ? aSize : maxSize;
		return okForSupervisorAccess;
		}

	TInt asid = aProcess->iOsAsid;
	TPde* pd = PageDirectory(asid);
	TPde pde = pd[aAddr>>KChunkShift];
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
			::TheMmu.iAliasList.Add(&iAliasLink); // add to list if not already aliased
#ifdef __SMP__
			__NK_ASSERT_DEBUG(iCpuRestoreCookie==-1);
			iCpuRestoreCookie = NKern::FreezeCpu();	// temporarily lock current thread to this processor
#endif
			}
		iAliasPde = pde;
		iAliasOsAsid = asid;
#ifdef __SMP__
		TSubScheduler& ss = SubScheduler();	// OK since we are locked to this CPU
		aliasAddr = TLinAddr(ss.i_AliasLinAddr) + (aAddr & (KChunkMask & ~KPageMask));
		iAliasPdePtr = (TPde*)(TLinAddr(ss.i_AliasPdePtr) + (((DMemModelProcess*)iOwningProcess)->iOsAsid << KPageTableShift));
#endif
		iAliasLinAddr = aliasAddr;
		}
	__KTRACE_OPT(KMMU,Kern::Printf("Writing PDE %08x to %08x", pde, iAliasPdePtr));
		*iAliasPdePtr = pde;
		__DRAIN_WRITE_BUFFER;
	DoInvalidateTLBForPage(aliasAddr);	// only need to do this processor
	TInt offset = aAddr&KPageMask;
	aAliasAddr = aliasAddr | offset;
	TInt maxSize = KPageSize - offset;
	aAliasSize = aSize<maxSize ? aSize : maxSize;
	return okForSupervisorAccess;
	}

void DMemModelThread::RemoveAlias()
//
// Remove alias mapping (if present)
// Enter and return with system locked.
//
	{
	__KTRACE_OPT(KMMU2,Kern::Printf("Thread %O RemoveAlias", this));
	__ASSERT_SYSTEM_LOCK;
	TLinAddr addr = iAliasLinAddr;
	if(addr)
		{
		iAliasLinAddr = 0;
		iAliasPde = 0;
		__KTRACE_OPT(KMMU,Kern::Printf("Clearing PDE at %08x", iAliasPdePtr));
		*iAliasPdePtr = 0;
		__DRAIN_WRITE_BUFFER;
		DoInvalidateTLBForPage(addr);	// only need to do it for this processor
		iAliasLink.Deque();
#ifdef __SMP__
		__NK_ASSERT_DEBUG(iCpuRestoreCookie>=0);
		NKern::EndFreezeCpu(iCpuRestoreCookie);
		iCpuRestoreCookie = -1;
#endif
		}
	}

void X86Mmu::CacheMaintenanceOnDecommit(TPhysAddr)
	{
	// no cache operations required on freeing memory
	}

void X86Mmu::CacheMaintenanceOnDecommit(const TPhysAddr*, TInt)
	{
	// no cache operations required on freeing memory
	}

void X86Mmu::CacheMaintenanceOnPreserve(TPhysAddr, TUint)
	{
	// no cache operations required on freeing memory
	}

void X86Mmu::CacheMaintenanceOnPreserve(const TPhysAddr*, TInt, TUint)
	{
	// no cache operations required on freeing memory
	}

void X86Mmu::CacheMaintenanceOnPreserve(TPhysAddr , TInt , TLinAddr , TUint )
	{
	// no cache operations required on freeing memory
	}


TInt X86Mmu::UnlockRamCachePages(TLinAddr aLinAddr, TInt aNumPages, DProcess* aProcess)
	{
	TInt asid = ((DMemModelProcess*)aProcess)->iOsAsid;
	TInt page = aLinAddr>>KPageShift;
	NKern::LockSystem();
	for(;;)
		{
		TPde* pd = PageDirectory(asid)+(page>>(KChunkShift-KPageShift));
		TPte* pt = SafePageTableFromPde(*pd++);
		__NK_ASSERT_DEBUG(pt);
		TInt pteIndex = page&(KChunkMask>>KPageShift);
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


TInt X86Mmu::LockRamCachePages(TLinAddr aLinAddr, TInt aNumPages, DProcess* aProcess)
	{
	TInt asid = ((DMemModelProcess*)aProcess)->iOsAsid;
	TInt page = aLinAddr>>KPageShift;
	NKern::LockSystem();
	for(;;)
		{
		TPde* pd = PageDirectory(asid)+(page>>(KChunkShift-KPageShift));
		TPte* pt = SafePageTableFromPde(*pd++);
		__NK_ASSERT_DEBUG(pt);
		TInt pteIndex = page&(KChunkMask>>KPageShift);
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
	// Make a page free
	TInt type = aPageInfo->Type();
	if(type==SPageInfo::EPagedCache)
		{
		TInt offset = aPageInfo->Offset()<<KPageShift;
		DMemModelChunk* chunk = (DMemModelChunk*)aPageInfo->Owner();
		__NK_ASSERT_DEBUG(TUint(offset)<TUint(chunk->iSize));
		TLinAddr lin = ((TLinAddr)chunk->iBase)+offset;
		TInt asid = ((DMemModelProcess*)chunk->iOwningProcess)->iOsAsid;
		TPte* pt = PtePtrFromLinAddr(lin,asid);
		*pt = 0;
		InvalidateTLBForPage(lin);

		// actually decommit it from chunk...
		TInt ptid = ((TLinAddr)pt-KPageTableBase)>>KPageTableShift;
		SPageTableInfo& ptinfo=((X86Mmu*)iMmu)->iPtInfo[ptid];
		if(!--ptinfo.iCount)
			{
			chunk->iPageTables[offset>>KChunkShift] = 0xffff;
			NKern::UnlockSystem();
			((X86Mmu*)iMmu)->DoUnassignPageTable(lin, (TAny*)asid);
			((X86Mmu*)iMmu)->FreePageTable(ptid);
			NKern::LockSystem();
			}
		}
	else
		{
		__KTRACE_OPT2(KPAGING,KPANIC,Kern::Printf("DP: SetFree() with bad page type = %d",aPageInfo->Type()));
		Panic(EUnexpectedPageType);
		}
	}

// Not supported on x86 - no defrag yet
void X86Mmu::DisablePageModification(DMemModelChunk* aChunk, TInt aOffset)
	{
	MM::Panic(MM::EOperationNotSupported);
	}

TInt X86Mmu::RamDefragFault(TAny* aExceptionInfo)
	{
	MM::Panic(MM::EOperationNotSupported);
	return KErrAbort;
	}
