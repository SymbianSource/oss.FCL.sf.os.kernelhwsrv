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
// e32\memmodel\epoc\direct\mcodeseg.cpp
// 
//

#include <memmodel.h>
#include <kernel/cache.h>

DCodeSeg* M::NewCodeSeg(TCodeSegCreateInfo&)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("M::NewCodeSeg"));
	return new DMemModelCodeSeg;
	}


DEpocCodeSegMemory* DEpocCodeSegMemory::New(DEpocCodeSeg* aCodeSeg)
	{
	return new DMemModelCodeSegMemory(aCodeSeg);
	}

	
DMemModelCodeSegMemory::DMemModelCodeSegMemory(DEpocCodeSeg* aCodeSeg)
	: DEpocCodeSegMemory(aCodeSeg)
	{
	}


DMemModelCodeSeg::DMemModelCodeSeg()
	{
	}


DMemModelCodeSeg::~DMemModelCodeSeg()
//
// Destructor
//
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelCodeSeg::Destruct %C", this));
	if (!iXIP && iMemory)
		{
		SRamCodeInfo& ri=RamInfo();
		if (ri.iCodeLoadAddr)
			{
			TInt code_size=MM::RoundToBlockSize(ri.iCodeSize+ri.iDataSize);
			MM::FreeRegion(ri.iCodeLoadAddr, code_size);
			}
		if (iDataAlloc)
			{
			TInt data_size=MM::RoundToBlockSize(ri.iDataSize+ri.iBssSize);
			MM::FreeRegion(iDataAlloc, data_size);
			}
		}
	Kern::Free(iKernelData);
	DEpocCodeSeg::Destruct();
	}

TInt DMemModelCodeSeg::DoCreateRam(TCodeSegCreateInfo& aInfo, DProcess* aProcess)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelCodeSeg::DoCreateRam %C proc %O", this, aProcess));
	DMemModelProcess* p=(DMemModelProcess*)aProcess;
	TBool kernel=( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttKernel );
	SRamCodeInfo& ri=RamInfo();
	iSize = MM::RoundToBlockSize(ri.iCodeSize+ri.iDataSize);
	TInt total_data_size=ri.iDataSize+ri.iBssSize;
	TInt r=MM::AllocRegion(ri.iCodeLoadAddr, iSize);
	if (r!=KErrNone)
		return r;
	ri.iCodeRunAddr=ri.iCodeLoadAddr;
	if (ri.iDataSize)
		ri.iDataLoadAddr=ri.iCodeLoadAddr+ri.iCodeSize;
	if (kernel)
		{
		if (total_data_size)
			{
			iKernelData=Kern::Alloc(total_data_size);
			if (!iKernelData)
				return KErrNoMemory;
			ri.iDataRunAddr=(TLinAddr)iKernelData;
			}
		return KErrNone;
		}
	if (total_data_size && p)
		SetAttachProcess(p);
	if (total_data_size && !IsExe())
		{
		TInt data_alloc_size=MM::RoundToBlockSize(total_data_size);
		TInt r=MM::AllocRegion(iDataAlloc, data_alloc_size);
		if (r!=KErrNone)
			return r;
		ri.iDataRunAddr=iDataAlloc;
		}
	return r;
	}

TInt DMemModelCodeSeg::DoCreateXIP(DProcess* aProcess)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelCodeSeg::DoCreateXIP %C proc %O", this, aProcess));
	DMemModelProcess* p=(DMemModelProcess*)aProcess;
	TInt r=KErrNone;
	TBool kernel=( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttKernel );
	const TRomImageHeader& rih=RomInfo();
	if (!kernel && aProcess)
		{
		// XIP images with static data are specific to a single process
		if (rih.iFlags&KRomImageFlagDataPresent)
			SetAttachProcess(p);
		}
	return r;
	}

TInt DMemModelCodeSeg::Loaded(TCodeSegCreateInfo& aInfo)
	{
	if (!iXIP)
		{
		// Clean DCache for specified area, Invalidate ICache/BTB for specified area
		TLinAddr code_base=RamInfo().iCodeRunAddr;
		Cache::IMB_Range(code_base, RamInfo().iCodeSize);
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
	__ASSERT_DEBUG( (iAttachProcess || !(iMark & EMarkDataPresent)), MM::Panic(MM::ECodeSegCheckInconsistent));
	return ETrue;
	}

