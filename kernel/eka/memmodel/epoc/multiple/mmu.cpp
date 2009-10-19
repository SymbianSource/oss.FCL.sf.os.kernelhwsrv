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
// e32\memmodel\epoc\multiple\mmu.cpp
// 
//

#include "memmodel.h"
#include <ramalloc.h>

_LIT(KLitGlobalDollarCode,"GLOBAL$CODE");

/*******************************************************************************
 * "Independent" MMU code
 *******************************************************************************/

void Mmu::Panic(TPanic aPanic)
	{
	Kern::Fault("MMU",aPanic);
	}

TPde* Mmu::LocalPageDir(TInt aOsAsid)
	{
	__ASSERT_DEBUG(TUint32(aOsAsid)<TUint32(iNumOsAsids),Panic(ELocalPageDirBadAsid));
	return (TPde*)(iPdeBase+(aOsAsid<<iGlobalPdShift));
	}

TPde* Mmu::GlobalPageDir(TInt aOsAsid)
	{
	__ASSERT_DEBUG(TUint32(aOsAsid)<TUint32(iNumOsAsids),Panic(EGlobalPageDirBadAsid));
	if (iAsidInfo[aOsAsid]&1)
		return (TPde*)(iPdeBase+(aOsAsid<<iGlobalPdShift));
	return (TPde*)iPdeBase;
	}
/*
TPde& Mmu::PDE(TLinAddr aAddr, TInt aOsAsid)
	{
	__ASSERT_DEBUG(TUint32(aOsAsid)<TUint32(iNumOsAsids),Panic(EPDEBadAsid));
	TPde* p=(TPde*)(iPdeBase+(aOsAsid<<iGlobalPdShift));
	if (aAddr>=iUserSharedEnd && (iAsidInfo[aOsAsid]&1))
		p=(TPde*)iPdeBase;
	p+=(aAddr>>iChunkShift);
	__KTRACE_OPT(KMMU,Kern::Printf("PDE(%08x,%d) at %08x",aAddr,aOsAsid,p));
	return *p;
	}
*/
TInt Mmu::NewOsAsid(TBool aSeparateGlobal)
//
// Allocate a new OS ASID and page directory.
// Map the page directory at the expected linear address and initialise it.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::NewOsAsid(%x)",aSeparateGlobal));
	TInt os_asid=iOsAsidAllocator->Alloc();
	if (os_asid<0)
		return KErrNoMemory;
	TPhysAddr pdPhys;
	TInt pdPages=0;
	TInt r=NewPageDirectory(os_asid,aSeparateGlobal,pdPhys,pdPages);
	__KTRACE_OPT(KMMU,Kern::Printf("NewPageDirectory: %d %08x %d",r,pdPhys,pdPages));
	if (r!=KErrNone)
		{
		iOsAsidAllocator->Free(os_asid);
		return KErrNoMemory;
		}
	TBool global=(pdPages<<iPageShift==iGlobalPdSize)?1:0;
	TLinAddr pdLin=iPdeBase+(os_asid<<iGlobalPdShift);
	if (((os_asid & iAsidGroupMask)==0) && (!iOsAsidAllocator->NotFree(os_asid+1,iAsidGroupMask)) )
		{
		// expand page directory mapping
		TInt xptid=AllocPageTable();
		if (xptid<0)
			{
			iRamPageAllocator->FreePhysicalRam(pdPhys,pdPages<<iPageShift);
			iOsAsidAllocator->Free(os_asid);
			return KErrNoMemory;
			}
		AssignPageTable(xptid, SPageTableInfo::EGlobal, NULL, pdLin, iPdPdePerm);	// map XPT
		}
	TInt i;
	for (i=0; i<pdPages; ++i)
		MapRamPage(pdLin+(i<<iPageShift), pdPhys+(i<<iPageShift), iPdPtePerm);
	InitPageDirectory(os_asid, global);
	iNumGlobalPageDirs+=global;
	iAsidInfo[os_asid]=global;
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::NewOsAsid returns %d (%d)",os_asid,global));
	return os_asid;
	}

void Mmu::FreeOsAsid(TInt aOsAsid)
//
// Free an OS ASID and the corresponding page directory.
// Assumes any local PDEs have already been unmapped.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::FreeOsAsid(%d)",aOsAsid));
	__ASSERT_DEBUG(TUint32(aOsAsid)<TUint32(iNumOsAsids),Panic(EFreeOsAsidBadAsid));
	TBool global=iAsidInfo[aOsAsid]&1;
	iAsidInfo[aOsAsid]=0;
	iOsAsidAllocator->Free(aOsAsid);
	iNumGlobalPageDirs-=global;
	TLinAddr pdLin=iPdeBase+(aOsAsid<<iGlobalPdShift);
	TUint32 size=global?iGlobalPdSize:iLocalPdSize;
	UnmapAndFree(pdLin,size>>iPageShift);
#ifdef BTRACE_KERNEL_MEMORY
	BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryMiscFree, size);
	Epoc::KernelMiscPages -= size>>iPageShift;
#endif
	TInt asid_group=aOsAsid&~iAsidGroupMask;
	if (!iOsAsidAllocator->NotFree(asid_group,iAsidGroupSize))
		{
		// shrink page directory mapping
		TInt xptid=PageTableId(pdLin,0);
		DoUnassignPageTable(pdLin, GLOBAL_MAPPING);
		FreePageTable(xptid);
		}
	}

TPhysAddr Mmu::LinearToPhysical(TLinAddr aLinAddr)
//
// Find the physical address corresponding to a given linear address
// Call with system locked
//
	{
	DMemModelProcess* pP=(DMemModelProcess*)TheCurrentThread->iOwningProcess;
	return LinearToPhysical(aLinAddr, pP->iOsAsid);
	}

TInt Mmu::LinearToPhysical(TLinAddr aLinAddr, TInt aSize, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList)
	{
	DMemModelProcess* pP=(DMemModelProcess*)TheCurrentThread->iOwningProcess;
	return LinearToPhysical(aLinAddr, aSize, aPhysicalAddress, aPhysicalPageList, pP->iOsAsid);
	}

TInt Mmu::PageTableId(TLinAddr aAddr)
	{
	DMemModelProcess* pP=(DMemModelProcess*)TheCurrentThread->iOwningProcess;
	return PageTableId(aAddr, pP->iOsAsid);
	}

void Mmu::Init1()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("Mmu::Init1"));
	iMaxPageTables=65535;		// possibly reduced when RAM size known
	memclr(iAsidInfo, iNumOsAsids*sizeof(TUint32));
	MmuBase::Init1();
	}

void Mmu::CreateUserGlobalSection(TLinAddr aBase, TLinAddr aEnd)
	{
	iUserGlobalSection=TLinearSection::New(aBase, aEnd);
	__ASSERT_ALWAYS(iUserGlobalSection,Panic(ECreateUserGlobalSectionFailed));
	}

void Mmu::DoInit2()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("Mmu::DoInit2"));
	iSharedSection=TLinearSection::New(iUserSharedBase, iUserSharedEnd);
	__ASSERT_ALWAYS(iSharedSection,Panic(ECreateSharedSectionFailed));
	iOsAsidAllocator=TBitMapAllocator::New(iNumOsAsids,ETrue);
	__ASSERT_ALWAYS(iOsAsidAllocator,Panic(EOsAsidAllocCreateFailed));
	iOsAsidAllocator->Alloc(0,1);	// 0=kernel process
	DMemModelProcess* pP=(DMemModelProcess*)K::TheKernelProcess;
	if (iLocalPdSize)
		pP->iLocalPageDir=LinearToPhysical(TLinAddr(LocalPageDir(0)));
	pP->iGlobalPageDir=LinearToPhysical(TLinAddr(GlobalPageDir(0)));
	__KTRACE_OPT(KMMU,Kern::Printf("Kernel process: LPD=%08x GPD=%08x",pP->iLocalPageDir,pP->iGlobalPageDir));
	MM::UserCodeAllocator=TBitMapAllocator::New(iMaxUserCodeSize>>iAliasShift, ETrue);	// code is aligned to alias size
	__ASSERT_ALWAYS(MM::UserCodeAllocator,Panic(EUserCodeAllocatorCreateFailed));
	MM::DllDataAllocator=TBitMapAllocator::New(iMaxDllDataSize>>iPageShift, ETrue);
	__ASSERT_ALWAYS(MM::DllDataAllocator,Panic(EDllDataAllocatorCreateFailed));
	__ASSERT_ALWAYS(TheRomHeader().iUserDataAddress==iDllDataBase+iMaxDllDataSize,Panic(ERomUserDataAddressInvalid));
	__ASSERT_ALWAYS((TheRomHeader().iTotalUserDataSize&iPageMask)==0,Panic(ERomUserDataSizeInvalid));
	TInt rom_dll_pages=TheRomHeader().iTotalUserDataSize>>iPageShift;
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("UserCodeAllocator @ %08x DllDataAllocator @ %08x, %d ROM DLL Data Pages",
		MM::UserCodeAllocator, MM::DllDataAllocator, rom_dll_pages));
	if (rom_dll_pages)
		MM::DllDataAllocator->Alloc(0, rom_dll_pages);	// low bit numbers represent high addresses
	}

void Mmu::SetupInitialPageInfo(SPageInfo* aPageInfo, TLinAddr aChunkAddr, TInt aPdeIndex)
	{
	__ASSERT_ALWAYS(aChunkAddr>=iUserSharedEnd,Panic(EBadInitialPageAddr));
	TLinAddr addr=aChunkAddr+(aPdeIndex<<iPageShift);
	if (aPageInfo->Type()!=SPageInfo::EUnused)
		return;	// already set (page table)
	if (addr==(TLinAddr)iPtInfo)
		{
		aPageInfo->SetPtInfo(0);
		aPageInfo->Lock();
		}
	else if (addr>=iPdeBase && addr<iPdeBase+iGlobalPdSize)
		{
		aPageInfo->SetPageDir(0,aPdeIndex);
		aPageInfo->Lock();
		}
	else
		aPageInfo->SetFixed();
	}

void Mmu::SetupInitialPageTableInfo(TInt aId, TLinAddr aChunkAddr, TInt aNumPtes)
	{
	__ASSERT_ALWAYS(aChunkAddr>=iUserSharedEnd || aChunkAddr==0,Panic(EBadInitialPageAddr));
	SPageTableInfo& pti=PtInfo(aId);
	pti.iCount=aNumPtes;
	pti.SetGlobal(aChunkAddr>>iChunkShift);
	}

void Mmu::AssignPageTable(TInt aId, TInt aUsage, TAny* aObject, TLinAddr aAddr, TPde aPdePerm)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::AssignPageTable id=%d, u=%08x, obj=%08x, addr=%08x, perm=%08x",
					aId, aUsage, aObject, aAddr, aPdePerm));
	const TAny* asids=GLOBAL_MAPPING;
	SPageTableInfo& pti=PtInfo(aId);
	switch (aUsage)
		{
		case SPageTableInfo::EChunk:
			{
			DMemModelChunk* pC=(DMemModelChunk*)aObject;
			TUint32 ccp=K::CompressKHeapPtr(pC);
			TUint32 offset=(aAddr-TLinAddr(pC->iBase))>>iChunkShift;
			pti.SetChunk(ccp,offset);
			if (pC->iOsAsids)
				asids=pC->iOsAsids;
			else if (pC->iOwningProcess)
				asids=(const TAny*)((DMemModelProcess*)pC->iOwningProcess)->iOsAsid;
			break;
			}
//		case SPageTableInfo::EHwChunk:
//			break;
		case SPageTableInfo::EGlobal:
			pti.SetGlobal(aAddr>>iChunkShift);
			break;
		default:
			Panic(EAssignPageTableInvalidUsage);
		}
	DoAssignPageTable(aId, aAddr, aPdePerm, asids);
	}

TInt Mmu::UnassignPageTable(TLinAddr aAddr)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Mmu::UnassignPageTable addr=%08x", aAddr));
	TInt id=PageTableId(aAddr, 0);
	if (id>=0)
		DoUnassignPageTable(aAddr, GLOBAL_MAPPING);
	return id;
	}

TInt Mmu::CreateGlobalCodeChunk()
//
// Enter and return with neither system lock nor MMU mutex held
//
	{
	__KTRACE_OPT(KDLL,Kern::Printf("Mmu::CreateGlobalCodeChunk"));
	TInt maxsize=Min(TheSuperPage().iTotalRamSize/2, 0x01000000);
	SChunkCreateInfo c;
	c.iGlobal=ETrue;
	c.iAtt=TChunkCreate::EDisconnected;
	c.iForceFixed=EFalse;
	c.iOperations=SChunkCreateInfo::EAdjust|SChunkCreateInfo::EAdd;
	c.iRunAddress=0;
	c.iPreallocated=0;
	c.iType=EDll;
	c.iMaxSize=maxsize;
	c.iName.Set(KLitGlobalDollarCode);
	c.iOwner=NULL;
	c.iInitialBottom=0;
	c.iInitialTop=0;
	TLinAddr runAddr;
	return K::TheKernelProcess->NewChunk((DChunk*&)iGlobalCode,c,runAddr);
	}
