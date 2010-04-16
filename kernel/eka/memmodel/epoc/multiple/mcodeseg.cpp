// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\epoc\multiple\mcodeseg.cpp
// 
//

#include "memmodel.h"
#include <mmubase.inl>
#include "cache_maintenance.h"
#include <demand_paging.h>

DCodeSeg* M::NewCodeSeg(TCodeSegCreateInfo&)
//
// Create a new instance of this class.
//
	{

	__KTRACE_OPT(KDLL,Kern::Printf("M::NewCodeSeg"));
	return new DMemModelCodeSeg;
	}

//
// DMemModelCodeSegMemory
//

DEpocCodeSegMemory* DEpocCodeSegMemory::New(DEpocCodeSeg* aCodeSeg)
	{
	return new DMemModelCodeSegMemory(aCodeSeg);
	}
	

DMemModelCodeSegMemory::DMemModelCodeSegMemory(DEpocCodeSeg* aCodeSeg)
	: DMmuCodeSegMemory(aCodeSeg)
	{
	}


TInt DMemModelCodeSegMemory::Create(TCodeSegCreateInfo& aInfo)
	{
	TInt r = DMmuCodeSegMemory::Create(aInfo);
	if(r!=KErrNone)
		return r;

	Mmu& m=Mmu::Get();

	iOsAsids = TBitMapAllocator::New(m.iNumOsAsids, EFalse);
	if(!iOsAsids)
		return KErrNoMemory;

	TInt totalPages = iPageCount+iDataPageCount;
	iPages = (TPhysAddr*)Kern::Alloc(totalPages*sizeof(TPhysAddr));
	if(!iPages)
		return KErrNoMemory;
	TInt i;
	for (i=0; i<totalPages; ++i)
		iPages[i] = KPhysAddrInvalid;

	MmuBase::Wait();

	// allocate RAM pages...
	__KTRACE_OPT(KDLL,Kern::Printf("Alloc DLL pages %x,%x", iPageCount,iDataPageCount));
	TInt startPage = iIsDemandPaged ? iPageCount : 0; 	// if demand paged, skip pages for code
	TInt endPage = iPageCount+iDataPageCount;
	r=m.AllocRamPages(iPages+startPage, endPage-startPage, EPageMovable);

	// initialise SPageInfo objects for allocated pages...
	if (r==KErrNone)
		{
		NKern::LockSystem();
		for (i=startPage; i<endPage; ++i)
			{
			SPageInfo* info = SPageInfo::FromPhysAddr(iPages[i]);
			info->SetCodeSegMemory(this,i);
			if((i&15)==15)
				NKern::FlashSystem();
			}
		NKern::UnlockSystem();
		}

	MmuBase::Signal();

	if (r!=KErrNone)
		return r;

#ifdef BTRACE_CODESEGS
	BTrace8(BTrace::ECodeSegs,BTrace::ECodeSegMemoryAllocated,iCodeSeg,iPageCount<<m.iPageShift);
#endif

	DCodeSeg::Wait();

	TInt code_alloc=((totalPages<<m.iPageShift)+m.iAliasMask)>>m.iAliasShift;
	r=MM::UserCodeAllocator->AllocConsecutive(code_alloc, ETrue);
	if (r<0)
		r = KErrNoMemory;
	else
		{
		MM::UserCodeAllocator->Alloc(r, code_alloc);
		iCodeAllocBase=r;
		iRamInfo.iCodeRunAddr=m.iUserCodeBase+(r<<m.iAliasShift);
		iRamInfo.iCodeLoadAddr=iRamInfo.iCodeRunAddr;
		if (iRamInfo.iDataSize)
			{
			if(iDataPageCount)
				iRamInfo.iDataLoadAddr=iRamInfo.iCodeLoadAddr+Mmu::RoundToPageSize(iRamInfo.iCodeSize);
			else
				iRamInfo.iDataLoadAddr=iRamInfo.iCodeLoadAddr+iRamInfo.iCodeSize;
			}

		DMemModelProcess* pP=(DMemModelProcess*)TheCurrentThread->iOwningProcess;
		r=pP->MapUserRamCode(this, ETrue);
		if (r==KErrNone)
			iCreator=pP;
		}

	DCodeSeg::Signal();
	return r;
	}


void DMemModelCodeSegMemory::Substitute(TInt aOffset, TPhysAddr aOld, TPhysAddr aNew)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelCodeSegMemory::Substitute %x %08x %08x",aOffset,aOld,aNew));
	Mmu& m=Mmu::Get();

	if (iPages[aOffset>>KPageShift] != aOld)
		MM::Panic(MM::ECodeSegRemapWrongPage);
	
	iPages[aOffset>>KPageShift] = aNew;
	m.RemapPageByAsid(iOsAsids, iRamInfo.iCodeRunAddr+aOffset, aOld, aNew, m.PtePermissions(EUserCode));
	}


TInt DMemModelCodeSegMemory::Loaded(TCodeSegCreateInfo& aInfo)
	{
	__NK_ASSERT_DEBUG(iPages);

	TInt r = DMmuCodeSegMemory::Loaded(aInfo);
	if(r!=KErrNone)
		return r;

	Mmu& m=Mmu::Get();

	if(!iIsDemandPaged)
		{
		UNLOCK_USER_MEMORY();
		CacheMaintenance::CodeChanged(iRamInfo.iCodeLoadAddr, iRamInfo.iCodeSize);
		LOCK_USER_MEMORY();
		}
	else
		{
		// apply code fixups to pages which have already been loaded...
		TInt pageShift = m.iPageShift;
		for (TInt i = 0 ; i < iPageCount ; ++i)
			{
			if (iPages[i] != KPhysAddrInvalid)
				{
				r = ApplyCodeFixupsOnLoad((TUint32*)(iRamInfo.iCodeLoadAddr+(i<<pageShift)),iRamInfo.iCodeRunAddr+(i<<pageShift));
				if(r!=KErrNone)
					return r;
				}
			}

		// copy export directory (this will now have fixups applied)...
		TInt exportDirSize = iRamInfo.iExportDirCount * sizeof(TLinAddr);
		if (exportDirSize > 0 || (exportDirSize == 0 && (iCodeSeg->iAttr & ECodeSegAttNmdExpData)) )
			{
			exportDirSize += sizeof(TLinAddr);
			TLinAddr expDirRunAddr = iRamInfo.iExportDir - sizeof(TLinAddr);
			if (expDirRunAddr < iRamInfo.iCodeRunAddr ||
				expDirRunAddr + exportDirSize > iRamInfo.iCodeRunAddr + iRamInfo.iCodeSize)
				{// Invalid export section.
				return KErrCorrupt;
				}
			TLinAddr* expDir = (TLinAddr*)Kern::Alloc(exportDirSize);
			if (!expDir)
				return KErrNoMemory;
			iCopyOfExportDir = expDir;
			UNLOCK_USER_MEMORY();
			memcpy(expDir, (TAny*)expDirRunAddr, exportDirSize);
			LOCK_USER_MEMORY();
			}
		}

	// unmap code from loading process...
	DMemModelProcess* pP=(DMemModelProcess*)TheCurrentThread->iOwningProcess;
	__ASSERT_ALWAYS(iCreator==pP, MM::Panic(MM::ECodeSegLoadedNotCreator));
	pP->UnmapUserRamCode(this, ETrue);
	iCreator=NULL;

	// discard any temporary pages used to store loaded data section...
	if(iDataPageCount)
		{
		MmuBase::Wait();
		TPhysAddr* pages = iPages+iPageCount;
		m.FreePages(pages,iDataPageCount, EPageMovable);
		for (TInt i = 0 ; i < iDataPageCount ; ++i)
			pages[i] = KPhysAddrInvalid;
		MmuBase::Signal();

		// see if we can free any virtual address space now we don't need any for loading data
		TInt data_start = ((iPageCount << m.iPageShift) + m.iAliasMask) >> m.iAliasShift;
		TInt data_end = (((iPageCount + iDataPageCount) << m.iPageShift) + m.iAliasMask) >> m.iAliasShift;
		if (data_end != data_start)
			{
			DCodeSeg::Wait();
			MM::UserCodeAllocator->Free(iCodeAllocBase + data_start, data_end - data_start);
			DCodeSeg::Signal();
			}
		
		iDataPageCount = 0;
		//Reduce the size of the DCodeSeg now the data section has been moved 
		iCodeSeg->iSize = iPageCount << m.iPageShift;
		}

	return KErrNone;
	}


void DMemModelCodeSegMemory::Destroy()
	{
	if(iCreator)
		iCreator->UnmapUserRamCode(this, ETrue);	// remove from creating process if not fully loaded
	}


DMemModelCodeSegMemory::~DMemModelCodeSegMemory()
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelCodeSegMemory::~DMemModelCodeSegMemory %x", this));
	__NK_ASSERT_DEBUG(iAccessCount==0);
	__NK_ASSERT_DEBUG(iOsAsids==0 || iOsAsids->Avail()==0); // check not mapped (inverted logic!)

	Mmu& m=Mmu::Get();

	if(iCodeAllocBase>=0)
		{
		// free allocated virtual memory space...
		TInt size = (iPageCount+iDataPageCount)<<KPageShift;
		TInt code_alloc=(size+m.iAliasMask)>>m.iAliasShift;
		DCodeSeg::Wait();
		MM::UserCodeAllocator->Free(iCodeAllocBase, code_alloc);
		DCodeSeg::Signal();
		}

	if(iPages)
		{
#ifdef __DEMAND_PAGING__
		if (iIsDemandPaged)
			{
			// Return any paged memory to the paging system
			MmuBase::Wait();
			NKern::LockSystem();
			DemandPaging& p = *DemandPaging::ThePager;
			for (TInt i = 0 ; i < iPageCount ; ++i)
				{
				if (iPages[i] != KPhysAddrInvalid)
					p.NotifyPageFree(iPages[i]);
				}
			NKern::UnlockSystem();
			MmuBase::Signal();
			
			Kern::Free(iCopyOfExportDir);
			iCopyOfExportDir = NULL;
			}
#endif
		MmuBase::Wait();
		m.FreePages(iPages,iPageCount+iDataPageCount, EPageMovable);
		MmuBase::Signal();
		Kern::Free(iPages);
		iPages = NULL;
#ifdef BTRACE_CODESEGS
		BTrace8(BTrace::ECodeSegs,BTrace::ECodeSegMemoryDeallocated,this,iPageCount<<m.iPageShift);
#endif
		}
	delete iOsAsids;
	}


DMemModelCodeSeg::DMemModelCodeSeg()
//
// Constructor
//
	:	iCodeAllocBase(-1),
		iDataAllocBase(-1)
	{
	}


DMemModelCodeSeg::~DMemModelCodeSeg()
//
// Destructor
//
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelCodeSeg::Destruct %C", this));
	Mmu& m=Mmu::Get();
	DCodeSeg::Wait();
	if (iCodeAllocBase>=0)
		{
		TBool kernel=( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttKernel );
		TBool global=( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttGlobal );
		TInt r=KErrNone;
		if (kernel)
			{
			DMemModelProcess& kproc=*(DMemModelProcess*)K::TheKernelProcess;
			r=kproc.iCodeChunk->Decommit(iCodeAllocBase, iSize);
			}
		else if (global)
			{
			r=m.iGlobalCode->Decommit(iCodeAllocBase, iSize);
			}
		__ASSERT_DEBUG(r==KErrNone, MM::Panic(MM::EDecommitFailed));
		r=r; // stop compiler warning
		}
	if(Memory())
		Memory()->Destroy();
	if (iDataAllocBase>=0 && !iXIP)
		{
		SRamCodeInfo& ri=RamInfo();
		TInt data_alloc=(ri.iDataSize+ri.iBssSize+m.iPageMask)>>m.iPageShift;
		MM::DllDataAllocator->Free(iDataAllocBase, data_alloc);
		}
	DCodeSeg::Signal();
	Kern::Free(iKernelData);
	DEpocCodeSeg::Destruct();
	}


TInt DMemModelCodeSeg::DoCreateRam(TCodeSegCreateInfo& aInfo, DProcess*)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelCodeSeg::DoCreateRam %C", this));
	TBool kernel=( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttKernel );
	TBool global=( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttGlobal );
	Mmu& m=Mmu::Get();
	SRamCodeInfo& ri=RamInfo();
	iSize = Mmu::RoundToPageSize(ri.iCodeSize+ri.iDataSize);
	if (iSize==0)
		return KErrCorrupt;
	TInt total_data_size=ri.iDataSize+ri.iBssSize;
	TInt r=KErrNone;
	if (kernel)
		{
		DMemModelProcess& kproc=*(DMemModelProcess*)K::TheKernelProcess;
		if (!kproc.iCodeChunk)
			r=kproc.CreateCodeChunk();
		if (r!=KErrNone)
			return r;
		r=kproc.iCodeChunk->Allocate(iSize, 0, m.iAliasShift);
		if (r<0)
			return r;
		iCodeAllocBase=r;
		ri.iCodeRunAddr=(TUint32)kproc.iCodeChunk->Base();
		ri.iCodeRunAddr+=r;
		ri.iCodeLoadAddr=ri.iCodeRunAddr;
		if (ri.iDataSize)
			ri.iDataLoadAddr=ri.iCodeLoadAddr+ri.iCodeSize;
		if (total_data_size)
			{
			iKernelData=Kern::Alloc(total_data_size);
			if (!iKernelData)
				return KErrNoMemory;
			ri.iDataRunAddr=(TLinAddr)iKernelData;
			}
		return KErrNone;
		}
	if (global)
		{
		if (!m.iGlobalCode)
			r=m.CreateGlobalCodeChunk();
		if (r==KErrNone)
			r=m.iGlobalCode->Allocate(iSize, 0, m.iAliasShift);
		if (r<0)
			return r;
		iCodeAllocBase=r;
		ri.iCodeRunAddr=(TUint32)m.iGlobalCode->Base();
		ri.iCodeRunAddr+=r;
		ri.iCodeLoadAddr=ri.iCodeRunAddr;
		ri.iDataLoadAddr=0;	// we don't allow static data in global code
		ri.iDataRunAddr=0;
		TInt loadSize = ri.iCodeSize+ri.iDataSize;
		memset((TAny*)(ri.iCodeRunAddr+loadSize), 0x03, iSize-loadSize);
		return KErrNone;
		}

	DCodeSeg::Wait();
	if (total_data_size && !IsExe())
		{
		TInt data_alloc=(total_data_size+m.iPageMask)>>m.iPageShift;
		__KTRACE_OPT(KDLL,Kern::Printf("Alloc DLL data %x", data_alloc));
		r=MM::DllDataAllocator->AllocConsecutive(data_alloc, ETrue);
		if (r<0)
			r = KErrNoMemory;
		else
			{
			MM::DllDataAllocator->Alloc(r, data_alloc);
			iDataAllocBase=r;
			ri.iDataRunAddr=m.iDllDataBase+m.iMaxDllDataSize-((r+data_alloc)<<m.iPageShift);
			r = KErrNone;
			}
		}
	DCodeSeg::Signal();

	if(r==KErrNone)
		r = Memory()->Create(aInfo);

	return r;
	}


TInt DMemModelCodeSeg::DoCreateXIP(DProcess* aProcess)
	{
//	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelCodeSeg::DoCreateXIP %C proc %O", this, aProcess));
	return KErrNone;
	}


TInt DMemModelCodeSeg::Loaded(TCodeSegCreateInfo& aInfo)
	{
	if(iXIP)
		return DEpocCodeSeg::Loaded(aInfo);

	TBool kernel=( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttKernel );
	TBool global=( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttGlobal );
	if (Pages())
		{
		TInt r = Memory()->Loaded(aInfo);
		if(r!=KErrNone)
			return r;
		}
	else if (kernel && iExeCodeSeg!=this)
		{
		Mmu& m=Mmu::Get();
		DMemModelProcess& kproc=*(DMemModelProcess*)K::TheKernelProcess;
		SRamCodeInfo& ri=RamInfo();

		// NOTE: Must do IMB before changing permissions since ARMv6 is very pedantic and
		// doesn't let you clean a cache line which is marked as read only.
		CacheMaintenance::CodeChanged(ri.iCodeRunAddr, ri.iCodeSize);

		TInt offset=ri.iCodeRunAddr-TLinAddr(kproc.iCodeChunk->iBase);
		kproc.iCodeChunk->ApplyPermissions(offset, iSize, m.iKernelCodePtePerm);
		}
	else if (global)
		{
		Mmu& m=Mmu::Get();
		SRamCodeInfo& ri=RamInfo();
		CacheMaintenance::CodeChanged(ri.iCodeRunAddr, ri.iCodeSize);
		TInt offset=ri.iCodeRunAddr-TLinAddr(m.iGlobalCode->iBase);
		m.iGlobalCode->ApplyPermissions(offset, iSize, m.iGlobalCodePtePerm);
		}
	return DEpocCodeSeg::Loaded(aInfo);
	}

void DMemModelCodeSeg::ReadExportDir(TUint32* aDest)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelCodeSeg::ReadExportDir %C %08x",this, aDest));

	if (!iXIP)
		{
		SRamCodeInfo& ri=RamInfo();
		TInt size=(ri.iExportDirCount+1)*sizeof(TLinAddr);

		if (Memory()->iCopyOfExportDir)
			{
			kumemput(aDest, Memory()->iCopyOfExportDir, size);
			return;
			}
		
		NKern::ThreadEnterCS();
		Mmu& m=Mmu::Get();
		TLinAddr src=ri.iExportDir-sizeof(TLinAddr);

		MmuBase::Wait();
		TInt offset=src-ri.iCodeRunAddr;
		TPhysAddr* physArray = Pages();
		TPhysAddr* ppa=physArray+(offset>>m.iPageShift);
		while(size)
			{
			TInt pageOffset = src&m.iPageMask;
			TInt l=Min(size, m.iPageSize-pageOffset);
			TLinAddr alias_src = m.MapTemp(*ppa++,src-pageOffset)+pageOffset;
			// Note, the following memory access isn't XTRAP'ed, because...
			// a) This function is only called by the loader thread, so even if
			//    exceptions were trapped the system is doomed anyway
			// b) Any exception will cause the crash debugger/logger to be called
			//    which will provide more information than if trapped exceptions
			//    and returned an error code.
			kumemput32(aDest, (const TAny*)alias_src, l);
			m.UnmapTemp();
			size-=l;
			src+=l;
			aDest+=l/sizeof(TUint32);
			}
		MmuBase::Signal();
		
		NKern::ThreadLeaveCS();
		}
	}

TBool DMemModelCodeSeg::OpenCheck(DProcess* aProcess)
	{
	return FindCheck(aProcess);
	}

TBool DMemModelCodeSeg::FindCheck(DProcess* aProcess)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("CSEG:%08x Compat? proc=%O",this,aProcess));
	if (aProcess)
		{
		DMemModelProcess& p=*(DMemModelProcess*)aProcess;
		DCodeSeg* pPSeg=p.CodeSeg();
		if (iAttachProcess && iAttachProcess!=aProcess)
			return EFalse;
		if (iExeCodeSeg && iExeCodeSeg!=pPSeg)
			return EFalse;
		}
	return ETrue;
	}


void DMemModelCodeSeg::BTracePrime(TInt aCategory)
	{
#ifdef BTRACE_CODESEGS
	if (aCategory == BTrace::ECodeSegs || aCategory == -1)
		{
		DCodeSeg::BTracePrime(aCategory);
		DMemModelCodeSegMemory* codeSegMemory = Memory();
		if(codeSegMemory && codeSegMemory->iPages && codeSegMemory->iPageCount)
			{
			BTrace8(BTrace::ECodeSegs,BTrace::ECodeSegMemoryAllocated,this,codeSegMemory->iPageCount<<Mmu::Get().iPageShift);
			}
		}
#endif
	}
