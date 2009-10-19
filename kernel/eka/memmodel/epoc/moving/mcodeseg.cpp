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
// e32\memmodel\epoc\moving\mcodeseg.cpp
// 
//

#include "memmodel.h"
#include "cache_maintenance.h"
#include <demand_paging.h>

_LIT(KLitUserCode,"USER$CODE");
_LIT(KLitKernCode,"KERN$CODE");

TInt MM::CreateCodeChunk(TBool aKernel)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("MM::CreateCodeChunk %d",aKernel));
	Mmu& m = Mmu::Get();
	SChunkCreateInfo c;
	c.iGlobal = ETrue;
	c.iAtt = TChunkCreate::EDisconnected;
	c.iForceFixed = EFalse;
	c.iOperations = 0;
	c.iPreallocated = 0;
	c.iOwner = NULL;

	if (aKernel)
		{
		c.iRunAddress = m.iKernelCodeBase;
		c.iType = EKernelCode;
		c.iMaxSize = m.iMaxKernelCodeSize;
		c.iName.Set(KLitKernCode);
		}
	else
		{
		c.iRunAddress = m.iUserCodeBase;
		c.iType = EDll;
		c.iMaxSize = m.iMaxUserCodeSize;
		c.iName.Set(KLitUserCode);
		}

	TLinAddr runAddr;
	DMemModelChunk* pC = NULL;
	TInt r=K::TheKernelProcess->NewChunk((DChunk*&)pC, c, runAddr);
	if (r==KErrNone)
		{
		pC->SetFixedAddress(c.iRunAddress, 0);
		if (aKernel)
			MM::KernelCodeChunk = pC;
		else
			MM::UserCodeChunk = pC;
		}
	return r;
	}

DCodeSeg* M::NewCodeSeg(TCodeSegCreateInfo&)
//
// Create a new instance of this class.
//
	{

	__KTRACE_OPT(KDLL,Kern::Printf("M::NewCodeSeg"));
	return new DMemModelCodeSeg;
	}


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

	TInt codeSize = iPageCount<<m.iPageShift;
	TInt dataSize = iDataPageCount<<m.iPageShift;
	TInt totalSize = codeSize+dataSize;

	DCodeSeg::Wait();
	
	DChunk::TCommitType commitType = iIsDemandPaged ? DChunk::ECommitVirtual : DChunk::ECommitDiscontiguous;
	r=MM::UserCodeChunk->FindFree(totalSize, 0, 0);
	if (r<0)
		{
		r = KErrNoMemory;
		goto exit;
		}
	iCodeAllocBase = r;
	r = KErrNone;
	
	r=MM::UserCodeChunk->Commit(iCodeAllocBase, codeSize, commitType);
	if (r<0)
		{
		r = KErrNoMemory;
		goto exit;
		}
	
	iRamInfo.iCodeRunAddr = ((TLinAddr)MM::UserCodeChunk->Base())+iCodeAllocBase;
	iRamInfo.iCodeLoadAddr = iRamInfo.iCodeRunAddr;

	if (iRamInfo.iDataSize)
		{
		if(iDataPageCount)
			iRamInfo.iDataLoadAddr = iRamInfo.iCodeLoadAddr+codeSize;
		else
			iRamInfo.iDataLoadAddr = iRamInfo.iCodeLoadAddr+iRamInfo.iCodeSize;
		}
	
	if (!iIsDemandPaged)
		{
		TInt loadedSize = iRamInfo.iCodeSize+iRamInfo.iDataSize;
		memset((TAny*)(iRamInfo.iCodeLoadAddr+loadedSize), 0x03, totalSize-loadedSize);
		}
	else
		{
		if(dataSize)
			{
			r=MM::UserCodeChunk->Commit(iCodeAllocBase+codeSize, dataSize, DChunk::ECommitDiscontiguous);
			if (r<0)
				goto exit;
			memset((TAny*)(iRamInfo.iCodeLoadAddr+codeSize+iRamInfo.iDataSize), 0x03, dataSize-iRamInfo.iDataSize);
			}
		}

exit:
	DCodeSeg::Signal();
	return r;
	}

	
TInt DMemModelCodeSegMemory::Loaded(TCodeSegCreateInfo& aInfo)
	{
	TInt r = DMmuCodeSegMemory::Loaded(aInfo);
	if(r!=KErrNone)
		return r;

	Mmu& m=Mmu::Get();
	TInt pageShift = m.iPageShift;

	if(iIsDemandPaged)
		{
		// apply code fixups to pages which have already been loaded...
		TLinAddr loadAddr = iRamInfo.iCodeLoadAddr;
		TLinAddr loadAddrEnd = loadAddr+iRamInfo.iCodeSize;
		TLinAddr runAddr = iRamInfo.iCodeLoadAddr;
		TInt pageSize = 1<<pageShift;
		for(; loadAddr<loadAddrEnd; loadAddr+=pageSize,runAddr+=pageSize)
			{
			if(m.LinearToPhysical(loadAddr)!=KPhysAddrInvalid)
				{
				r = ApplyCodeFixupsOnLoad((TUint32*)loadAddr,runAddr);
				if(r!=KErrNone)
					return r;
				}
			}
		}
	CacheMaintenance::CodeChanged(iRamInfo.iCodeLoadAddr, iRamInfo.iCodeSize);

	// discard any temporary pages used to store loaded data section...
	if(iDataPageCount)
		{
		TInt codeSize = iPageCount<<pageShift;
		MM::UserCodeChunk->Decommit(iCodeAllocBase+codeSize, iDataPageCount<<pageShift);
		iDataPageCount = 0;
		//Reduce the size of the DCodeSeg now the data section has been moved
		iCodeSeg->iSize = codeSize;
		}

	return KErrNone;
	}

	
void DMemModelCodeSegMemory::Destroy()
	{
	if(iCodeAllocBase!=KMinTInt && iDataPageCount)
		{
		Mmu& m=Mmu::Get();
		TInt dataSize = iDataPageCount<<m.iPageShift;
		TInt codeSize = iPageCount<<m.iPageShift;
		if(dataSize)
			MM::UserCodeChunk->Decommit(iCodeAllocBase+codeSize, dataSize);
		}
	}


DMemModelCodeSegMemory::~DMemModelCodeSegMemory()
	{
	if(iCodeAllocBase!=KMinTInt)
		{
		Mmu& m=Mmu::Get();
		TInt codeSize = iPageCount<<m.iPageShift;
		DMemModelChunk::TDecommitType decommitType = iIsDemandPaged ? DChunk::EDecommitVirtual : DChunk::EDecommitNormal;
		MM::UserCodeChunk->Decommit(iCodeAllocBase, codeSize, decommitType);
		}
	}


DMemModelCodeSeg::DMemModelCodeSeg()
//
// Constructor
//
	:	iCodeAllocBase(KMinTInt),
		iDataAllocBase(KMinTInt)
	{
	}

DMemModelCodeSeg::~DMemModelCodeSeg()
//
// Destructor
//
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelCodeSeg::Destruct %C", this));
	Mmu& m=Mmu::Get();
	if (!iXIP && iMemory)
		{
		SRamCodeInfo& ri=RamInfo();
		DCodeSeg::Wait();
		if (iCodeAllocBase!=KMinTInt)
			{
			TBool kernel=( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttKernel );
			if (kernel)
				MM::KernelCodeChunk->Decommit(iCodeAllocBase, iSize);
			}
		Memory()->Destroy();
		TInt data_alloc=(ri.iDataSize+ri.iBssSize+m.iPageMask)>>m.iPageShift;
		if (iDataAllocBase>=0)
			{
			MM::DllDataAllocator->Free(iDataAllocBase, data_alloc);
			}
		else if (iDataAllocBase==-1)
			{
			DMemModelProcess* p=(DMemModelProcess*)iAttachProcess;
			if (p->iExitType==EExitPending)
				{
				DMemModelChunk& c=*p->iDllDataChunk;
				TInt offset=ri.iDataRunAddr-TLinAddr(c.Base());
				c.Decommit(offset, data_alloc<<m.iPageShift);
				if (c.iSize==0)
					p->FreeDllDataChunk();
				}
			}
		DCodeSeg::Signal();
		}
	Kern::Free(iKernelData);
	DEpocCodeSeg::Destruct();
	}


TInt DMemModelCodeSeg::DoCreateRam(TCodeSegCreateInfo& aInfo, DProcess* aProcess)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelCodeSeg::DoCreateRam %C proc %O", this, aProcess));
	TBool kernel=( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttKernel );
	DMemModelProcess* p=(DMemModelProcess*)aProcess;
	Mmu& m=Mmu::Get();
	SRamCodeInfo& ri=RamInfo();
	iSize = Mmu::RoundToPageSize(ri.iCodeSize+ri.iDataSize);
	if (iSize==0)
		return KErrCorrupt;
	TInt total_data_size=ri.iDataSize+ri.iBssSize;
	TInt r=KErrNone;
	if (kernel)
		{
		r=MM::KernelCodeChunk->Allocate(iSize, 0, 0);
		if (r<0)
			return r;
		iCodeAllocBase=r;
		ri.iCodeRunAddr=(TUint32)MM::KernelCodeChunk->Base();
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

	r = Memory()->Create(aInfo);
	if(r!=KErrNone)
		return r;

	r=KErrNone;
	if (total_data_size && p && p->iAttributes&DMemModelProcess::EFixedAddress)
		{
		SetAttachProcess(p);
		}
	if (total_data_size && !IsExe())
		{
		TInt data_alloc_size=Mmu::RoundToPageSize(total_data_size);
		TInt data_alloc=data_alloc_size>>m.iPageShift;
		DCodeSeg::Wait();
		if (p)
			{
			if (p->iExitType!=EExitPending)
				return KErrDied;
			}
		if (iAttachProcess)
			{
			r=KErrNone;
			if (!p->iDllDataChunk)
				r=p->CreateDllDataChunk();
			if (r==KErrNone)
				{
				DMemModelChunk& c=*p->iDllDataChunk;
				r=c.Allocate(data_alloc_size, 0, 0);
				if (r>=0)
					{
					ri.iDataRunAddr=TLinAddr(c.Base())+r;
					iDataAllocBase=-1;
					r=KErrNone;
					}
				else
					{
					if (c.iSize==0)
						p->FreeDllDataChunk();
					r=KErrNoMemory;
					}
				}
			}
		else
			{
			r=MM::DllDataAllocator->AllocConsecutive(data_alloc, ETrue);
			if (r>=0)
				MM::DllDataAllocator->Alloc(r, data_alloc);
			if (r>=0)
				{
				iDataAllocBase=r;
				ri.iDataRunAddr=m.iDataSectionEnd-((r+data_alloc)<<m.iPageShift);
				r=KErrNone;
				}
			else
				r=KErrNoMemory;
			}
		DCodeSeg::Signal();
		}
	if(r!=KErrNone)
		return r;
	
	return r;
	}


TInt DMemModelCodeSeg::DoCreateXIP(DProcess* aProcess)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelCodeSeg::DoCreateXIP %C proc %O", this, aProcess));
	DMemModelProcess* p=(DMemModelProcess*)aProcess;
	TInt r=KErrNone;
	TBool kernel=( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttKernel );
	const TRomImageHeader& rih=RomInfo();
	TBool fixed=p && (p->iAttributes&DMemModelProcess::EFixedAddress);
	if (!kernel && fixed)
		{
		// XIP images with static data loaded into fixed processes are specific to a single process
		if (rih.iFlags&KRomImageFlagDataPresent)
			{
			if (rih.iTotalDataSize)
				{
				TLinAddr process_data_base=p->iDataBssRunAddress;
				TUint32 process_data_maxsize=p->iDataBssStackChunk->iMaxSize;
				if (rih.iDataBssLinearBase<process_data_base || 
					(rih.iDataBssLinearBase+rih.iTotalDataSize)>(process_data_base+process_data_maxsize))
					return KErrNotSupported;
				}
			SetAttachProcess(p);
			iDataAllocBase=-1;
			}
		}
	return r;
	}


TInt DMemModelCodeSeg::Loaded(TCodeSegCreateInfo& aInfo)
	{
	if (!iXIP)
		{
		TBool kernel=( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttKernel );
		if(kernel)
			{
			// Clean DCache for specified area, Invalidate ICache/BTB for specified area
			SRamCodeInfo& ri=RamInfo();
			CacheMaintenance::CodeChanged(ri.iCodeRunAddr, ri.iCodeSize);
			}
		else
			{
			TInt r = Memory()->Loaded(aInfo);
			if(r!=KErrNone)
				return r;
			}
		}
	return DEpocCodeSeg::Loaded(aInfo);
	}


void DMemModelCodeSeg::ReadExportDir(TUint32* aDest)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelCodeSeg::ReadExportDir %08x",aDest));
	if (!iXIP)
		{
		SRamCodeInfo& ri=RamInfo();
		TInt size=(ri.iExportDirCount+1)*sizeof(TLinAddr);
		kumemput32(aDest, (const TUint32*)(ri.iExportDir-sizeof(TUint32)), size);
		}
	}

TBool DMemModelCodeSeg::OpenCheck(DProcess* aProcess)
	{
	return FindCheck(aProcess);
	}

TBool DMemModelCodeSeg::FindCheck(DProcess* aProcess)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("CSEG:%08x Compat? proc=%O",this,aProcess));
	if (!aProcess)
		return !iAttachProcess;	// can't reuse same code segment for a new instance of the process
	DMemModelProcess& p=*(DMemModelProcess*)aProcess;
	DCodeSeg* pPSeg=p.CodeSeg();
	if (iAttachProcess && iAttachProcess!=aProcess)
		return EFalse;
	if (iExeCodeSeg && iExeCodeSeg!=pPSeg)
		return EFalse;
	if (!iAttachProcess && (iMark & EMarkDataPresent))
		{
		// code seg used for moving processes, data present
		if (p.iAttributes & DMemModelProcess::EFixedAddress)
			return EFalse;
		}
	return ETrue;
	}
