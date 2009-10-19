// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\epoc\multiple\mprocess.cpp
// 
//

#include "memmodel.h"
#include "mmboot.h"
#include "cache_maintenance.h"
#include <demand_paging.h>

#define iMState		iWaitLink.iSpare1

// just for convenience...
#define KAmSelfMod	(DMemModelChunk::ECode | DMemModelChunk::EAddressLocal)

_LIT(KDollarDat,"$DAT");
_LIT(KLitDollarCode,"$CODE");
_LIT(KLitDllDollarData,"DLL$DATA");

#ifdef __CPU_HAS_BTB
extern void __FlushBtb();
#endif

const TInt KChunkGranularity=4;

/********************************************
 * Process
 ********************************************/
void DMemModelProcess::Destruct()
	{
	__ASSERT_ALWAYS(!iChunkCount && !iCodeChunk && !iDllDataChunk, MM::Panic(MM::EProcessDestructChunksRemaining));
	Kern::Free(iChunks);
	Kern::Free(iLocalSection);
	if (iOsAsid)
		{
		Mmu& m=Mmu::Get();
		MmuBase::Wait();
		m.FreeOsAsid(iOsAsid);
		iOsAsid=0;
		MmuBase::Signal();
#ifndef __SMP__
		LastUserSelfMod=0;  // must force a BTB flush when next selfmod chunk switched in
#endif
		}
#ifdef __CPU_HAS_BTB
	__FlushBtb();
#endif
	DProcess::Destruct();
	}

TInt DMemModelProcess::NewChunk(DChunk*& aChunk, SChunkCreateInfo& aInfo, TLinAddr& aRunAddr)
	{
	aChunk=NULL;
	DMemModelChunk* pC=NULL;
	TInt r=GetNewChunk(pC,aInfo);
	if (r!=KErrNone)
		{
		if (pC)
			pC->Close(NULL);
		return r;
		}
	TInt mapType=pC->iAttributes & DMemModelChunk::EMapTypeMask;
	pC->iOwningProcess=(mapType==DMemModelChunk::EMapTypeLocal)?this:NULL;
#ifdef __CPU_HAS_BTB
	if ((pC->iAttributes & KAmSelfMod) == KAmSelfMod)  // it's a potentially overlapping self-mod
		{
		iSelfModChunks++;
#ifndef __SMP__
		LastUserSelfMod = this;  // we become the last selfmodding process
#endif
		__FlushBtb();		// we need to do this, as there may be bad branches already in the btb
		}
#endif
	r=pC->Create(aInfo);
	if (r==KErrNone && (aInfo.iOperations & SChunkCreateInfo::EAdjust))
		{
		if (aInfo.iRunAddress!=0)
			pC->SetFixedAddress(aInfo.iRunAddress,aInfo.iPreallocated);
		if (aInfo.iPreallocated==0 && aInfo.iInitialTop!=0)
			{
			if (pC->iAttributes & DChunk::EDisconnected)
				{
				r=pC->Commit(aInfo.iInitialBottom,aInfo.iInitialTop-aInfo.iInitialBottom);
				}
			else if (pC->iAttributes & DChunk::EDoubleEnded)
				{
				r=pC->AdjustDoubleEnded(aInfo.iInitialBottom,aInfo.iInitialTop);
				}
			else
				{
				r=pC->Adjust(aInfo.iInitialTop);
				}
			}
		}
	if (r==KErrNone && (aInfo.iOperations & SChunkCreateInfo::EAdd))
		{
//			if (pC->iAttributes & DMemModelChunk::ECode)
//				MM::TheMmu->SyncCodeMappings();
		if (mapType!=DMemModelChunk::EMapTypeGlobal)
			{
			r=WaitProcessLock();
			if (r==KErrNone)
				{
				r=AddChunk(pC,aRunAddr,EFalse);
				SignalProcessLock();
				}
			}
		else
			aRunAddr=(TLinAddr)pC->Base();
		}
	if (r==KErrNone)
		{
		if(r==KErrNone)
			if(pC->iKernelMirror)
				aRunAddr = (TLinAddr)pC->iKernelMirror->Base();
		pC->iDestroyedDfc = aInfo.iDestroyedDfc;
		aChunk=(DChunk*)pC;
		}
	else
		pC->Close(NULL);	// NULL since chunk can't have been added to process
	return r;
	}

TInt DMemModelProcess::DoCreate(TBool aKernelProcess, TProcessCreateInfo& aInfo)
	{
	__KTRACE_OPT(KPROC,Kern::Printf(">DMemModelProcess::DoCreate %O",this));

	Mmu& m=Mmu::Get();
	TInt r=KErrNone;

	iSelfModChunks=0;  // we don't have any yet.

	if (aKernelProcess)
		{
		iAttributes |= ESupervisor;
		//iOsAsid=0;
//		Leave these till Mmu::Init2
//		if (m.iLocalPdSize)
//			iLocalPageDir=m.LinearToPhysical(TLinAddr(m.LocalPageDir(0)));
//		iGlobalPageDir=m.LinearToPhysical(TLinAddr(m.GlobalPageDir(0)));
		m.iAsidInfo[0]=((TUint32)this)|1;
		iAddressCheckMaskR=0xffffffff;
		iAddressCheckMaskW=0xffffffff;
		}
	else
		{
		MmuBase::Wait();
		r=m.NewOsAsid(EFalse);
		if (r>=0)
			{
			iOsAsid=r;
			if (m.iLocalPdSize)
				iLocalPageDir=m.LinearToPhysical(TLinAddr(m.LocalPageDir(r)));
			else
				iGlobalPageDir=m.LinearToPhysical(TLinAddr(m.GlobalPageDir(r)));
			m.iAsidInfo[r] |= (TUint32)this;
			r=KErrNone;
			}
		MmuBase::Signal();
		if (r==KErrNone && 0==(iLocalSection=TLinearSection::New(m.iUserLocalBase, m.iUserLocalEnd)) )
			r=KErrNoMemory;
		}

	__KTRACE_OPT(KPROC,Kern::Printf("OS ASID=%d, LPD=%08x, GPD=%08x, ASID info=%08x",iOsAsid,iLocalPageDir,
											iGlobalPageDir,m.iAsidInfo[iOsAsid]));
	__KTRACE_OPT(KPROC,Kern::Printf("<DMemModelProcess::DoCreate %d",r));
	return r;
	}

TInt DMemModelProcess::CreateDataBssStackArea(TProcessCreateInfo& aInfo)
	{
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess::CreateDataBssStackArea %O",this));
	Mmu& m=Mmu::Get();
	TInt dataBssSize=Mmu::RoundToPageSize(aInfo.iTotalDataSize);
	TInt maxSize=dataBssSize+PP::MaxStackSpacePerProcess;
	TLinAddr dataRunAddress=m.iUserLocalBase;
	iDataBssRunAddress=dataRunAddress;

	__KTRACE_OPT(KPROC,Kern::Printf("DataBssSize=%x, chunk max size %x",dataBssSize,maxSize));

	SChunkCreateInfo cinfo;
	cinfo.iGlobal=EFalse;
	cinfo.iAtt=TChunkCreate::EDisconnected;
	cinfo.iForceFixed=EFalse;
	cinfo.iOperations=SChunkCreateInfo::EAdjust|SChunkCreateInfo::EAdd;
	cinfo.iType=EUserData;
	cinfo.iMaxSize=maxSize;
	cinfo.iInitialBottom=0;
	cinfo.iInitialTop=dataBssSize;
	cinfo.iPreallocated=0;
	cinfo.iName.Set(KDollarDat);
	cinfo.iOwner=this;
	cinfo.iRunAddress=0;
	TLinAddr cb;
	TInt r=NewChunk((DChunk*&)iDataBssStackChunk,cinfo,cb);
	return r;
	}

TInt DMemModelProcess::AddChunk(DChunk* aChunk, TBool isReadOnly)
	{
	DMemModelChunk* pC=(DMemModelChunk*)aChunk;
	if ((pC->iAttributes & DMemModelChunk::EPrivate) && this!=pC->iOwningProcess)
		return KErrAccessDenied;
	TInt r=WaitProcessLock();
	if (r==KErrNone)
		{
		TInt pos=0;
		r=ChunkIndex(pC,pos);
		TLinAddr dataSectionBase=0;
		if (r==0) // Found the chunk in this process, just up its count
			{
			iChunks[pos].iAccessCount++;
			__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess::AddChunk %08x to %08x (Access count incremented to %d)",aChunk,this,iChunks[pos].iAccessCount));
			SignalProcessLock();
			return KErrNone;
			}
		r=AddChunk(pC,dataSectionBase,isReadOnly);
		SignalProcessLock();
		}
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess::AddChunk returns %d",r));
	return r;
	}

void M::FsRegisterThread()
	{
	DMemModelChunk* pC=(DMemModelChunk*)PP::TheRamDriveChunk;
	TInt mapType=pC->iAttributes & DMemModelChunk::EMapTypeMask;
	if (mapType!=DMemModelChunk::EMapTypeLocal)
		{
		DMemModelProcess* pP=(DMemModelProcess*)TheCurrentThread->iOwningProcess;
		TLinAddr dataSectionBase;
		TInt r=pP->WaitProcessLock();
		if (r==KErrNone)
			r=pP->AddChunk(pC,dataSectionBase,EFalse);
		__ASSERT_ALWAYS(r==KErrNone, MM::Panic(MM::EFsRegisterThread));
		pP->SignalProcessLock();
		}
	}

TInt DMemModelProcess::AddChunk(DMemModelChunk* aChunk, TLinAddr& aDataSectionBase, TBool isReadOnly)
	{
	//
	// Must hold the process $LOCK mutex before calling this
	//
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess::AddChunk %O to %O",aChunk,this));
	SChunkInfo *pC=iChunks;
	SChunkInfo *pE=pC+iChunkCount-1;
	TLinAddr base=TLinAddr(aChunk->iBase);
	TInt i=0;

#ifdef __CPU_HAS_BTB
	if ((aChunk->iAttributes & KAmSelfMod)==KAmSelfMod)  // it's a potentially overlapping self-mod
		{
		iSelfModChunks++;
#ifndef __SMP__
		LastUserSelfMod = this;  // we become the last selfmodding process
#endif
		__FlushBtb();		// we need to do this, as there may be bad branches already in the btb
		}
#endif
	if (iChunkCount)
		{
		for (; pE>=pC && TLinAddr(pE->iChunk->iBase)>base; --pE);
		if (pE>=pC && TLinAddr(pE->iChunk->iBase)+pE->iChunk->iMaxSize>base)
			return KErrInUse;
		pC=pE+1;
		if (pC<iChunks+iChunkCount && base+aChunk->iMaxSize>TLinAddr(pC->iChunk->iBase))
			return KErrInUse;
		i=pC-iChunks;
		}
	if (iChunkCount==iChunkAlloc)
		{
		TInt newAlloc=iChunkAlloc+KChunkGranularity;
		TInt r=Kern::SafeReAlloc((TAny*&)iChunks,iChunkAlloc*sizeof(SChunkInfo),newAlloc*sizeof(SChunkInfo));
		if (r!=KErrNone)
			return r;
		pC=iChunks+i;
		iChunkAlloc=newAlloc;
		}
	memmove(pC+1,pC,(iChunkCount-i)*sizeof(SChunkInfo));
	++iChunkCount;
	pC->isReadOnly=isReadOnly;
	pC->iAccessCount=1;
	pC->iChunk=aChunk;
	aDataSectionBase=base;
	Mmu& m=Mmu::Get();
	if (aChunk->iOsAsids)
		{
		// only need to do address space manipulation for shared chunks
		MmuBase::Wait();
		aChunk->iOsAsids->Alloc(iOsAsid,1);
		TLinAddr a;
		TInt i=0;
		for (a=TLinAddr(aChunk->iBase); a<TLinAddr(aChunk->iBase)+aChunk->iMaxSize; a+=m.iChunkSize, ++i)
			{
			TInt ptid=aChunk->iPageTables[i];
			if (ptid!=0xffff)
				m.DoAssignPageTable(ptid,a,aChunk->iPdePermissions,(const TAny*)iOsAsid);
			}
		MmuBase::Signal();
		}
	if (aChunk->iChunkType==ERamDrive)
		{
		NKern::LockSystem();
		iAddressCheckMaskR |= m.iRamDriveMask;
		iAddressCheckMaskW |= m.iRamDriveMask;
		NKern::UnlockSystem();
		}
	__DEBUG_EVENT(EEventUpdateProcess, this);
	return KErrNone;
	}

void DMemModelProcess::DoRemoveChunk(TInt aIndex)
	{
	__DEBUG_EVENT(EEventUpdateProcess, this);
	DMemModelChunk* chunk = iChunks[aIndex].iChunk;
	memmove(iChunks+aIndex, iChunks+aIndex+1, (iChunkCount-aIndex-1)*sizeof(SChunkInfo));
	--iChunkCount;
	Mmu& m=Mmu::Get();
	if (chunk->iOsAsids)
		{
		// only need to do address space manipulation for shared chunks
		MmuBase::Wait();
		chunk->iOsAsids->Free(iOsAsid);
		TLinAddr a;
		for (a=TLinAddr(chunk->iBase); a<TLinAddr(chunk->iBase)+chunk->iMaxSize; a+=m.iChunkSize)
			m.DoUnassignPageTable(a,(const TAny*)iOsAsid);
		TUint32 mask=(chunk->iAttributes&DMemModelChunk::ECode)?Mmu::EFlushITLB:0;
		m.GenericFlush(mask|Mmu::EFlushDTLB);

		MmuBase::Signal();
		}
	if (chunk->iChunkType==ERamDrive)
		{
		NKern::LockSystem();
		iAddressCheckMaskR &= ~m.iRamDriveMask;
		iAddressCheckMaskW &= ~m.iRamDriveMask;
		NKern::UnlockSystem();
		}
	}

/**
Final chance for process to release resources during its death.

Called with process $LOCK mutex held (if it exists).
This mutex will not be released before it is deleted.
I.e. no other thread will ever hold the mutex again.
*/
void DMemModelProcess::FinalRelease()
	{
	// Clean up any left over chunks (such as SharedIo buffers)
	if(iProcessLock)
		while(iChunkCount)
			DoRemoveChunk(0);
	}

void DMemModelProcess::RemoveChunk(DMemModelChunk *aChunk)
	{
	// note that this can't be called after the process $LOCK mutex has been deleted
	// since it can only be called by a thread in this process doing a handle close or
	// dying, or by the process handles array being deleted due to the process dying,
	// all of which happen before $LOCK is deleted.
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess %O RemoveChunk %O",this,aChunk));
	Kern::MutexWait(*iProcessLock);
	TInt pos=0;
	TInt r=ChunkIndex(aChunk,pos);

	if (r==KErrNone) // Found the chunk
		{
		__KTRACE_OPT(KPROC,Kern::Printf("Chunk access count %d",iChunks[pos].iAccessCount));
		if (--iChunks[pos].iAccessCount==0)
			{
			DoRemoveChunk(pos);
#ifdef __CPU_HAS_BTB
			if ((aChunk->iAttributes & KAmSelfMod)==KAmSelfMod)  // was a self-mod code chunk
				if (iSelfModChunks)
					iSelfModChunks--;
#endif
			}
		}
	Kern::MutexSignal(*iProcessLock);
	}

TInt DMemModelProcess::ChunkIndex(DMemModelChunk* aChunk,TInt& aPos)
	{
	if (!aChunk)
		return KErrNotFound;
	SChunkInfo *pC=iChunks;
	SChunkInfo *pE=pC+iChunkCount;
	for (; pC<pE && pC->iChunk!=aChunk; ++pC);
	if (pC==pE)
		return KErrNotFound;
	aPos=pC-iChunks;
	return KErrNone;
	}

TInt DMemModelProcess::MapCodeSeg(DCodeSeg* aSeg)
	{
	DMemModelCodeSeg& seg=*(DMemModelCodeSeg*)aSeg;
	__KTRACE_OPT(KDLL,Kern::Printf("Process %O MapCodeSeg %C", this, aSeg));
	TBool kernel_only=( (seg.iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttKernel );
	if (kernel_only && !(iAttributes&ESupervisor))
		return KErrNotSupported;
	if (seg.iAttr&ECodeSegAttKernel)
		return KErrNone;	// no extra mappings needed for kernel code
	TInt r=KErrNone;
	if (seg.Pages())
		r=MapUserRamCode(seg.Memory(),EFalse);
	if (seg.IsDll())
		{
		TInt total_data_size;
		TLinAddr data_base;
		seg.GetDataSizeAndBase(total_data_size, data_base);
		if (r==KErrNone && total_data_size)
			{
			TInt size=Mmu::RoundToPageSize(total_data_size);
			r=CommitDllData(data_base, size);
			if (r!=KErrNone && seg.Pages())
				UnmapUserRamCode(seg.Memory(), EFalse);
			}
		}
	return r;
	}

void DMemModelProcess::UnmapCodeSeg(DCodeSeg* aSeg)
	{
	DMemModelCodeSeg& seg=*(DMemModelCodeSeg*)aSeg;
	__KTRACE_OPT(KDLL,Kern::Printf("Process %O UnmapCodeSeg %C", this, aSeg));
	if (seg.iAttr&ECodeSegAttKernel)
		return;	// no extra mappings needed for kernel code
	if (seg.IsDll())
		{
		TInt total_data_size;
		TLinAddr data_base;
		seg.GetDataSizeAndBase(total_data_size, data_base);
		if (total_data_size)
			DecommitDllData(data_base, Mmu::RoundToPageSize(total_data_size));
		}
	if (seg.Pages())
		UnmapUserRamCode(seg.Memory(), EFalse);
	}

void DMemModelProcess::RemoveDllData()
//
// Call with CodeSegLock held
//
	{
	}

TInt DMemModelProcess::CreateCodeChunk()
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelProcess %O CreateCodeChunk",this));
	TBool kernel=iAttributes&ESupervisor;
	Mmu& m=Mmu::Get();
	SChunkCreateInfo c;
	c.iGlobal=kernel;
	c.iAtt = TChunkCreate::EDisconnected | (kernel? 0 : TChunkCreate::EMemoryNotOwned);
	c.iForceFixed=EFalse;
	c.iOperations=SChunkCreateInfo::EAdjust|SChunkCreateInfo::EAdd;
	c.iRunAddress=kernel ? 0 : m.iUserCodeBase;
	c.iPreallocated=0;
	c.iType=kernel ? EKernelCode : EUserCode;
	c.iMaxSize=m.iMaxUserCodeSize;
	c.iName.Set(KLitDollarCode);
	c.iOwner=this;
	c.iInitialTop=0;
	TLinAddr runAddr;
	TInt r = NewChunk((DChunk*&)iCodeChunk,c,runAddr);
	return r;
	}

void DMemModelProcess::FreeCodeChunk()
	{
	iCodeChunk->Close(this);
	iCodeChunk=NULL;
	}

TInt DMemModelProcess::MapUserRamCode(DMemModelCodeSegMemory* aMemory, TBool aLoading)
	{
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess %O MapUserRamCode %C %d %d %d",
									this, aMemory->iCodeSeg, aLoading, iOsAsid, aMemory->iIsDemandPaged));
	__ASSERT_MUTEX(DCodeSeg::CodeSegLock);

	TInt r;

	if (!iCodeChunk)
		{
		r=CreateCodeChunk();
		__KTRACE_OPT(KPROC,Kern::Printf("CreateCodeChunk returns %d", r));
		if (r!=KErrNone)
			return r;
		}

	MmuBase::Wait();

	Mmu& m=Mmu::Get();
	TInt offset=aMemory->iRamInfo.iCodeRunAddr-TLinAddr(iCodeChunk->iBase);
	TInt codeSize = aMemory->iPageCount<<m.iPageShift;
	TBool paged = aMemory->iIsDemandPaged;
	DChunk::TCommitType commitType = paged ? DChunk::ECommitVirtual : DChunk::ECommitDiscontiguousPhysical;
	r=iCodeChunk->Commit(offset, codeSize, commitType, aMemory->iPages);
	__KTRACE_OPT(KPROC,Kern::Printf("Commit Pages returns %d", r));
	if(r==KErrNone)
		{
		if (aLoading && !paged)
			{
			iCodeChunk->ApplyPermissions(offset, codeSize, m.iUserCodeLoadPtePerm);
			UNLOCK_USER_MEMORY();
			memset((TAny*)(aMemory->iRamInfo.iCodeLoadAddr+aMemory->iRamInfo.iCodeSize+aMemory->iRamInfo.iDataSize), 0x03, codeSize-(aMemory->iRamInfo.iCodeSize+aMemory->iRamInfo.iDataSize));
			LOCK_USER_MEMORY();
			}
		if(aLoading && aMemory->iDataPageCount)
			{
			TInt dataSize = aMemory->iDataPageCount<<m.iPageShift;
			r=iCodeChunk->Commit(offset+codeSize, dataSize, DChunk::ECommitDiscontiguousPhysical, aMemory->iPages+aMemory->iPageCount);
			if(r==KErrNone)
				{
				iCodeChunk->ApplyPermissions(offset+codeSize, dataSize, m.iUserCodeLoadPtePerm);
				UNLOCK_USER_MEMORY();
				memset((TAny*)(aMemory->iRamInfo.iDataLoadAddr+aMemory->iRamInfo.iDataSize), 0x03, dataSize-aMemory->iRamInfo.iDataSize);
				LOCK_USER_MEMORY();
				}
			}
		if(r!=KErrNone)
			{
			// error, so decommit up code pages we had already committed...
			DChunk::TDecommitType decommitType = paged ? DChunk::EDecommitVirtual : DChunk::EDecommitNormal;
			iCodeChunk->Decommit(offset, codeSize, decommitType);
			}
		else
			{
			// indicate codeseg is now successfully mapped into the process...
			NKern::LockSystem();
			aMemory->iOsAsids->Free(iOsAsid);
			NKern::UnlockSystem();
			}
		}

	MmuBase::Signal();

	if(r!=KErrNone && iCodeChunk->iSize==0)
		FreeCodeChunk(); // cleanup any unused code chunk we would otherwise leave lying around

	return r;
	}

void DMemModelProcess::UnmapUserRamCode(DMemModelCodeSegMemory* aMemory, TBool aLoading)
	{
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess %O UnmapUserRamCode %C %d %d",
									this, aMemory->iCodeSeg, iOsAsid, aMemory->iIsDemandPaged != 0));

	__ASSERT_MUTEX(DCodeSeg::CodeSegLock);

	MmuBase::Wait();

	NKern::LockSystem();
	aMemory->iOsAsids->Alloc(iOsAsid, 1);
	NKern::UnlockSystem();

	Mmu& m=Mmu::Get();
	__NK_ASSERT_DEBUG(iCodeChunk);
	TInt offset=aMemory->iRamInfo.iCodeRunAddr-TLinAddr(iCodeChunk->iBase);
	TInt codeSize = aMemory->iPageCount<<m.iPageShift;
	TBool paged = aMemory->iIsDemandPaged;
	DChunk::TDecommitType decommitType = paged ? DChunk::EDecommitVirtual : DChunk::EDecommitNormal;
	TInt r=iCodeChunk->Decommit(offset, codeSize, decommitType);
	__ASSERT_DEBUG(r==KErrNone, MM::Panic(MM::EDecommitFailed));
	(void)r; //Supress the warning in urel build

	if(aLoading && aMemory->iDataPageCount)
		{
		// decommit pages used to store data section...
		TInt dataSize = aMemory->iDataPageCount<<m.iPageShift;
		r=iCodeChunk->Decommit(offset+codeSize, dataSize);
		__ASSERT_DEBUG(r==KErrNone, MM::Panic(MM::EDecommitFailed));
		(void)r; //Supress the warning in urel build
		}
	__NK_ASSERT_DEBUG(iCodeChunk->iSize >= 0);

	MmuBase::Signal();

	if (iCodeChunk->iSize==0)
		FreeCodeChunk();
	}

TInt DMemModelProcess::CreateDllDataChunk()
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelProcess %O CreateDllDataChunk",this));
	Mmu& m=Mmu::Get();
	SChunkCreateInfo c;
	c.iGlobal=EFalse;
	c.iAtt=TChunkCreate::EDisconnected;
	c.iForceFixed=EFalse;
	c.iOperations=SChunkCreateInfo::EAdjust|SChunkCreateInfo::EAdd;
	c.iRunAddress=m.iDllDataBase;
	c.iPreallocated=0;
	c.iType=EDllData;
	c.iMaxSize=m.iMaxDllDataSize;
	c.iName.Set(KLitDllDollarData);
	c.iOwner=this;
	c.iInitialTop=0;
	TLinAddr runAddr;
	return NewChunk((DChunk*&)iDllDataChunk,c,runAddr);
	}

void DMemModelProcess::FreeDllDataChunk()
	{
	iDllDataChunk->Close(this);
	iDllDataChunk=NULL;
	}

TInt DMemModelProcess::CommitDllData(TLinAddr aBase, TInt aSize)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelProcess %O CommitDllData %08x+%x",this,aBase,aSize));
	TInt r=KErrNone;
	if (!iDllDataChunk)
		r=CreateDllDataChunk();
	if (r==KErrNone)
		{
		TInt offset=aBase-(TLinAddr)iDllDataChunk->iBase;
		__ASSERT_ALWAYS(TUint32(offset)<TUint32(iDllDataChunk->iMaxSize),MM::Panic(MM::ECommitInvalidDllDataAddress));
		r=iDllDataChunk->Commit(offset, aSize);
		if (r!=KErrNone && iDllDataChunk->iSize==0)
			FreeDllDataChunk();
		}
	__KTRACE_OPT(KDLL,Kern::Printf("CommitDllData returns %d",r));
	return r;
	}

void DMemModelProcess::DecommitDllData(TLinAddr aBase, TInt aSize)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelProcess %O DecommitDllData %08x+%x",this,aBase,aSize));
	TInt offset=aBase-(TLinAddr)iDllDataChunk->iBase;
	TInt r=iDllDataChunk->Decommit(offset, aSize);
	__ASSERT_ALWAYS(r==KErrNone,MM::Panic(MM::EDecommitInvalidDllDataAddress));
	if (iDllDataChunk->iSize==0)
		FreeDllDataChunk();
	}

TInt DMemModelProcess::NewShPool(DShPool*& /* aPool */, TShPoolCreateInfo& /* aInfo */)
	{
	return KErrNotSupported;
	}


TInt DThread::RawRead(const TAny* aSrc, TAny* aDest, TInt aLength, TInt aFlags, TIpcExcTrap* /*aExcTrap*/)
//
// Read from the thread's process.
// Enter and return with system locked
// aSrc      Run address of memory to read
// aDest     Current address of destination
// aExcTrap  Exception trap object to be updated if the actual memory access is performed on other memory area then specified.
//           It happens when  reading is performed on un-aligned memory area.
//
	{
	Mmu& m=Mmu::Get();
	DMemModelThread& t=*(DMemModelThread*)TheCurrentThread;
	DMemModelProcess* pP=(DMemModelProcess*)iOwningProcess;
	TLinAddr src=(TLinAddr)aSrc;
	TLinAddr dest=(TLinAddr)aDest;
	TBool localIsSafe=ETrue;
	TInt result = KErrNone;

	while (aLength)
		{
		if (iMState==EDead)
			{
			result = KErrDied;
			break;
			}
		TLinAddr alias_src;
		TInt alias_size;
		TInt alias_result=t.Alias(src, pP, aLength, EMapAttrReadUser, alias_src, alias_size);
		if (alias_result<0)
			{
			result = KErrBadDescriptor;	// bad permissions
			break;
			}
		NKern::UnlockSystem();

		__KTRACE_OPT(KTHREAD2,Kern::Printf("DThread::RawRead %08x<-%08x+%x",dest,alias_src,alias_size));
		if(aFlags&KCheckLocalAddress)
			localIsSafe = m.ValidateLocalIpcAddress(dest,alias_size,ETrue);

		CHECK_PAGING_SAFE;

		COND_UNLOCK_USER_MEMORY(localIsSafe);

		if(alias_result)
			{
			// remote address is safe for direct access...
			if (localIsSafe)
				memcpy( (TAny*)dest, (const TAny*)alias_src, alias_size);
			else
				umemput( (TAny*)dest, (const TAny*)alias_src, alias_size);
			}
		else
			{
			// remote address is NOT safe for direct access, so use user permision checks when reading...
			if (localIsSafe)
				umemget( (TAny*)dest, (const TAny*)alias_src, alias_size);
			else
				uumemcpy( (TAny*)dest, (const TAny*)alias_src, alias_size);
			}

		LOCK_USER_MEMORY();

		src+=alias_size;
		dest+=alias_size;
		aLength-=alias_size;
		NKern::LockSystem();
		}
	t.RemoveAlias();
	return result;
	}

TInt DThread::RawWrite(const TAny* aDest, const TAny* aSrc, TInt aLength, TInt aFlags, DThread* anOriginatingThread, TIpcExcTrap* /*aExcTrap*/)
//
// Write to the thread's process.
// Enter and return with system locked
// aDest               Run address of memory to write
// aSrc                Current address of destination
// anOriginatingThread The thread on behalf of which this operation is performed (eg client of device driver).
// aExcTrap            Exception trap object to be updated if the actual memory access is performed on other memory area then specified.
//                     It happens when reading is performed on un-aligned memory area.
//
	{
	Mmu& m=Mmu::Get();
	DMemModelThread& t=*(DMemModelThread*)TheCurrentThread;
	DMemModelProcess* pP=(DMemModelProcess*)iOwningProcess;
	TLinAddr src=(TLinAddr)aSrc;
	TLinAddr dest=(TLinAddr)aDest;
	TBool localIsSafe=ETrue;
	DThread* pO=anOriginatingThread?anOriginatingThread:&t;
	DProcess* pF=K::TheFileServerProcess;
	TBool special=(iOwningProcess==pF && pO->iOwningProcess==pF);
	TUint32 perm=special ? EMapAttrWriteSup : EMapAttrWriteUser;
	TInt result = KErrNone;

	while (aLength)
		{
		if (iMState==EDead)
			{
			result = KErrDied;
			break;
			}
		TLinAddr alias_dest;
		TInt alias_size;
		TInt alias_result=t.Alias(dest, pP, aLength, perm, alias_dest, alias_size);
		if (alias_result<0)
			{
			result = KErrBadDescriptor;	// bad permissions
			break;
			}
		NKern::UnlockSystem();

		__KTRACE_OPT(KTHREAD2,Kern::Printf("DThread::RawWrite %08x+%x->%08x",src,alias_size,alias_dest));
		if(aFlags&KCheckLocalAddress)
			localIsSafe = m.ValidateLocalIpcAddress(src,alias_size,EFalse);

		// Must check that it is safe to page, unless we are reading from unpaged ROM in which case
		// we allow it.  umemget and uumemcpy do this anyway, so we just need to check if
		// localIsSafe is set.
		if (localIsSafe)
			{
			CHECK_PAGING_SAFE_RANGE(src, aLength);
			CHECK_DATA_PAGING_SAFE_RANGE(dest, aLength);
			}

		COND_UNLOCK_USER_MEMORY(localIsSafe);

		if(alias_result)
			{
			// remote address is safe for direct access...
			if (localIsSafe)
				memcpy( (TAny*)alias_dest, (const TAny*)src, alias_size);
			else
				umemget( (TAny*)alias_dest, (const TAny*)src, alias_size);
			}
		else
			{
			// remote address is NOT safe for direct access, so use user permision checks when writing...
			if (localIsSafe)
				umemput( (TAny*)alias_dest, (const TAny*)src, alias_size);
			else
				uumemcpy( (TAny*)alias_dest, (const TAny*)src, alias_size);
			}

		LOCK_USER_MEMORY();

		src+=alias_size;
		dest+=alias_size;
		aLength-=alias_size;
		NKern::LockSystem();
		}
	t.RemoveAlias();
	return result;
	}

#ifdef __DEBUGGER_SUPPORT__

/**
@pre Calling thread must be in critical section
@pre CodeSeg mutex held
*/
TInt CodeModifier::SafeWriteCode(DProcess* aProcess, TLinAddr aAddress, TInt aSize, TUint aValue, void* aOldValue)
	{
	Mmu& m=Mmu::Get();
	MmuBase::Wait();

	NKern::LockSystem();

	// Find physical address of the page, the breakpoint belongs to
	TPhysAddr physAddr = m.LinearToPhysical(aAddress,((DMemModelProcess*)aProcess)->iOsAsid);
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("CodeModifier::SafeWriteCode - PA:%x", physAddr));
	if (physAddr==KPhysAddrInvalid)
		{
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("CodeModifier::SafeWriteCode - invalid VA"));
		NKern::UnlockSystem();
		MmuBase::Signal();
		return KErrBadDescriptor;
		}

	// Temporarily map physical page
	TLinAddr tempAddr = m.MapTemp (physAddr&~m.iPageMask, aAddress);
	tempAddr |=  aAddress & m.iPageMask;
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("CodeModifier::SafeWriteCode - tempAddr:%x",tempAddr));

	//Set exception handler. Make sure the boundaries cover the worst case (aSize = 4)
	TIpcExcTrap xt;
	xt.iLocalBase=0;
	xt.iRemoteBase=(TLinAddr)tempAddr&~3; //word aligned.
	xt.iSize=sizeof(TInt);
	xt.iDir=1;

	TInt r=xt.Trap(NULL);
	if (r==0)
		{
		r = WriteCode(tempAddr, aSize, aValue, aOldValue);
		xt.UnTrap();
		}

	m.UnmapTemp();
	NKern::UnlockSystem();
	MmuBase::Signal();
	return r;	
	}

/**
@pre Calling thread must be in critical section
@pre CodeSeg mutex held
*/
TInt CodeModifier::WriteCode(TLinAddr aAddress, TInt aSize, TUint aValue, void* aOldValue)
	{
	// We do not want to be interrupted by e.g. ISR that will run altered code before IMB-Range.
	// Therefore, copy data and clean/invalidate caches with interrupts disabled.
	TInt irq=NKern::DisableAllInterrupts();
	switch(aSize)
		{
		case 1:
			*(TUint8*) aOldValue = *(TUint8*)aAddress;
			*(TUint8*) aAddress  = (TUint8)aValue;
			 break;
		case 2:
			*(TUint16*) aOldValue = *(TUint16*)aAddress;
			*(TUint16*) aAddress  = (TUint16)aValue;
			 break;
		default://It is 4 otherwise
			*(TUint32*) aOldValue = *(TUint32*)aAddress;
			*(TUint32*) aAddress  = (TUint32)aValue;
			 break;
		};
	CacheMaintenance::CodeChanged(aAddress, aSize, CacheMaintenance::ECodeModifier);
	NKern::RestoreInterrupts(irq);
	return KErrNone;
	}
#endif //__DEBUGGER_SUPPORT__


#ifdef __MARM__

// the body of ReadDesHeader is machine coded on ARM...
extern TInt ThreadDoReadAndParseDesHeader(DThread* aThread, const TAny* aSrc, TUint32* aDest);

TInt DThread::ReadAndParseDesHeader(const TAny* aSrc, TDesHeader& aDest)
//
// Read and parse the header of a remote descriptor.
// Enter and return with system locked
//
	{
	// todo: remove use of system lock from callers, when they have been un-exported from the kernel
	NKern::UnlockSystem();	
	TInt r = ThreadDoReadAndParseDesHeader(this,aSrc,(TUint32*)&aDest);
	NKern::LockSystem();
	return r;
	}


#else // !__MARM__


TInt DThread::ReadAndParseDesHeader(const TAny* aSrc, TDesHeader& aDest)
//
// Read and parse the header of a remote descriptor.
// Enter and return with system locked
//
	{
	static const TUint8 LengthLookup[16] = {4,8,12,8,12,0,0,0,0,0,0,0,0,0,0,0};

	DMemModelThread& t = *(DMemModelThread*)TheCurrentThread;
	TInt r = KErrBadDescriptor;

	CHECK_PAGING_SAFE;

	DMemModelProcess* pP = (DMemModelProcess*)iOwningProcess;
	TLinAddr src = (TLinAddr)aSrc;
	const TUint32* pAlias;
	TInt alias_size;
	TInt alias_result = t.Alias(src, pP, 12, EMapAttrReadUser, (TLinAddr&)pAlias, alias_size);
	if (alias_result<0)
		return KErrBadDescriptor;	// bad permissions
	NKern::UnlockSystem();
	t.iIpcClient = this;
	TUint32* dest = (TUint32*)&aDest;
	if (Kern::SafeRead(pAlias, dest, sizeof(TUint32)))
		goto fail;

	{
	TInt type=*dest>>KShiftDesType8;

	src += sizeof(TUint32);
	alias_size -=  sizeof(TUint32);
	++pAlias;
	++dest;

	TInt l=LengthLookup[type];
	if (l==0)
		goto fail;

	l -= sizeof(TUint32); // we've already read one word
	if (l>0 && alias_size)
		{
get_more:
		// more to go - get rest or as much as is currently aliased
		TInt ll = alias_size>=l ? l : alias_size;
		if(Kern::SafeRead(pAlias, dest, l))
			goto fail;
		l -= ll;
		src += TLinAddr(ll);
		dest = (TUint32*)(TLinAddr(dest) + TLinAddr(ll));
		}
	if (l>0)
		{
		// more to go - need to step alias on
		NKern::LockSystem();
		alias_result = t.Alias(src, pP, l, EMapAttrReadUser, (TLinAddr&)pAlias, alias_size);
		if (alias_result<0)
			goto fail_locked;
		NKern::UnlockSystem();
		goto get_more;
		}

	r = K::ParseDesHeader(aSrc, *(TRawDesHeader*)&aDest, aDest);
	}

fail:
	NKern::LockSystem();
fail_locked:
	t.RemoveAlias();
	t.iIpcClient = NULL;
	return r;
	}


#endif


DChunk* DThread::OpenSharedChunk(const TAny* aAddress, TBool aWrite, TInt& aOffset)
	{
	NKern::LockSystem();
	
	DMemModelProcess* pP = (DMemModelProcess*)iOwningProcess;
	DMemModelProcess::SChunkInfo* pS=pP->iChunks;
	DMemModelProcess::SChunkInfo* pC=pS+pP->iChunkCount;
	while(--pC>=pS && TUint(pC->iChunk->Base())>TUint(aAddress)) {};
	if(pC>=pS)
		{
		DMemModelChunk* chunk = pC->iChunk;
		if(chunk->iChunkType==ESharedKernelSingle || chunk->iChunkType==ESharedKernelMultiple)
			{
			TInt offset = (TInt)aAddress-(TInt)chunk->Base();
			if(TUint(offset)<TUint(chunk->iMaxSize) && chunk->Open()==KErrNone)
				{
				aOffset = offset;
				NKern::UnlockSystem();
				return chunk;
				}
			}
		}
	NKern::UnlockSystem();
	return 0;
	}

TInt DThread::PrepareMemoryForDMA(const TAny* aLinAddr, TInt aSize, TPhysAddr* aPhysicalPageList)
	{
	TInt asid = ((DMemModelProcess*)iOwningProcess)->iOsAsid;
	Mmu& m=(Mmu&)*MmuBase::TheMmu;
	return m.PreparePagesForDMA((TLinAddr)aLinAddr, aSize, asid, aPhysicalPageList);
	}

TInt DThread::ReleaseMemoryFromDMA(const TAny* aLinAddr, TInt aSize, TPhysAddr* aPhysicalPageList)
	{
	TInt pageCount = (((TInt)aLinAddr & KPageMask) + aSize + KPageMask) >> KPageShift;
	Mmu& m=(Mmu&)*MmuBase::TheMmu;
	return m.ReleasePagesFromDMA(aPhysicalPageList, pageCount);
	}

