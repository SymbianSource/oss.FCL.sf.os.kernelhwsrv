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
// e32\memmodel\epoc\moving\mprocess.cpp
// 
//

#include "memmodel.h"
#include "cache_maintenance.h"
#include "mmboot.h"

#define iMState		iWaitLink.iSpare1

_LIT(KDollarDat,"$DAT");
_LIT(KLitDllDollarData,"DLL$DATA");

/********************************************
 * Process
 ********************************************/
void DMemModelProcess::Destruct()
	{
	NKern::LockSystem();
	if (this==TheCurrentAddressSpace)
		TheCurrentAddressSpace=NULL;
	if (this==TheCurrentVMProcess)
		TheCurrentVMProcess=NULL;
	if (this==TheCurrentDataSectionProcess)
		TheCurrentDataSectionProcess=NULL;
	if (this==TheCompleteDataSectionProcess)
		TheCompleteDataSectionProcess=NULL;
	NKern::UnlockSystem();
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
	if (aInfo.iForceFixed || iAttributes & DMemModelProcess::EFixedAddress)
		pC->iAttributes |= DMemModelChunk::EFixedAddress;
	if (!aInfo.iGlobal && (iAttributes & DMemModelProcess::EPrivate)!=0)
		pC->iAttributes |= DMemModelChunk::EPrivate;
	if (pC->iChunkType==EDll || pC->iChunkType==EUserCode || pC->iChunkType==EUserSelfModCode || pC->iChunkType==EKernelCode)
		pC->iAttributes |= (DMemModelChunk::EFixedAddress|DMemModelChunk::ECode);
	pC->iOwningProcess=(aInfo.iGlobal)?NULL:this;
	r=pC->Create(aInfo);
	if (r==KErrNone && (aInfo.iOperations & SChunkCreateInfo::EAdjust))
		{
		if (aInfo.iRunAddress!=0)
			pC->SetFixedAddress(aInfo.iRunAddress,aInfo.iPreallocated);
		if (aInfo.iPreallocated==0)
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
		if (r==KErrNone && pC->iHomeRegionBase==0 && (pC->iAttributes&DMemModelChunk::EFixedAddress)!=0)
			{
			r=pC->Reserve(0);
			aRunAddr=(TLinAddr)pC->Base();
			}
		}
	if (r==KErrNone && (aInfo.iOperations & SChunkCreateInfo::EAdd))
		{
		if (pC->iAttributes & DMemModelChunk::ECode)
			Mmu::Get().SyncCodeMappings();
		if (pC->iChunkType!=EUserCode)
			{
			r=WaitProcessLock();
			if (r==KErrNone)
				{
				r=AddChunk(pC,aRunAddr,EFalse);
				SignalProcessLock();
				}
			}
		else
			aRunAddr=(TLinAddr)pC->Base();	// code chunks always fixed address
		}
	if (r==KErrNone)
		{
		pC->iDestroyedDfc = aInfo.iDestroyedDfc;
		aChunk=(DChunk*)pC;
		}
	else
		pC->Close(NULL);	// NULL since chunk can't have been added to process
	return r;
	}

TInt DMemModelProcess::DoCreate(TBool aKernelProcess, TProcessCreateInfo& aInfo)
	{
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess::DoCreate %O",this));

	if (aKernelProcess)
		iAttributes=ESupervisor|EFixedAddress|EPrivate;
	else if (aInfo.iAttr & ECodeSegAttFixed)
		iAttributes=EFixedAddress|EPrivate;
	else
		iAttributes=0;
	if ((iAttributes & ESupervisor)==0 && (iAttributes & EFixedAddress)!=0)
		{
		CheckForFixedAccess();
		}
	return KErrNone;
	}

TInt DMemModelProcess::CreateDataBssStackArea(TProcessCreateInfo& aInfo)
	{
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess::CreateDataBssStackArea %O",this));
	TInt dataBssSize=Mmu::RoundToPageSize(aInfo.iTotalDataSize);
	TInt maxSize=dataBssSize+PP::MaxStackSpacePerProcess;
	TBool fixed=(iAttributes & EFixedAddress);

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
	if (fixed && dataBssSize!=0 && aInfo.iCodeLoadAddress)
		{
		const TRomImageHeader& rih=*(const TRomImageHeader*)aInfo.iCodeLoadAddress;
		cinfo.iRunAddress=rih.iDataBssLinearBase;
		}
	else
		cinfo.iRunAddress=0;
	TInt r=NewChunk((DChunk*&)iDataBssStackChunk,cinfo,iDataBssRunAddress);
	return r;
	}

TInt DMemModelProcess::AddChunk(DChunk* aChunk,TBool isReadOnly)
	{
	DMemModelChunk* pC=(DMemModelChunk*)aChunk;
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

void FlushBeforeChunkMove(DMemModelChunk* aChunk)
	{
	Mmu& m = Mmu::Get();
	TUint32 ff=Mmu::EFlushDMove|Mmu::EFlushDPermChg;
	if (aChunk->iAttributes & DMemModelChunk::ECode)		// assumption here that code chunks don't move
		ff |= Mmu::EFlushIPermChg;
	m.GenericFlush(ff);
	}

TInt DMemModelProcess::AddChunk(DMemModelChunk* aChunk, TLinAddr& aDataSectionBase, TBool isReadOnly)
	{
	//
	// Must hold the process $LOCK mutex before calling this
	//
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess::AddChunk %08x to %08x (for first time)",aChunk,this));
	TInt r=AllocateDataSectionBase(*((DMemModelChunk*)aChunk),(TUint&)aDataSectionBase);
	if(r!=KErrNone)
		return r;

	if (iNumChunks==KMaxChunksInProcess)
		return KErrOverflow;		// too many chunks in the process

	SChunkInfo *pC=iChunks;
	SChunkInfo *pE=pC+iNumChunks-1;
	NKern::LockSystem();
	while(pE>=pC && TUint(pE->iDataSectionBase)>TUint(aDataSectionBase))
		{
		pE[1]=pE[0];
		pE--;
		}
	pC=pE+1;
	pC->iDataSectionBase=aDataSectionBase;
	pC->isReadOnly=isReadOnly;
	pC->iAccessCount=1;
	pC->iChunk=aChunk;
	iNumChunks++;

	if(!(iAttributes&ESupervisor))
		{
		TInt attribs=aChunk->iAttributes;
		if (!(attribs&DMemModelChunk::EFixedAddress))
			{
			iNumMovingChunks++;
			iAttributes |= EMoving;
			}

		if (attribs&DMemModelChunk::EFixedAccess)
			{
			NKern::UnlockSystem();
			AddFixedAccessChunk(aChunk);
			goto done;	// FINISHED
			}

		iAttributes |= EVariableAccess;
		if (attribs & DMemModelChunk::ECode)
			{
			iNumNonFixedAccessCodeChunks++;
			iAttributes |= EVariableCode;
			}
		if (++iNumNonFixedAccessChunks==1)
			{
			NKern::UnlockSystem();
			DoAttributeChange();	// change process from fixed to variable access
			NKern::LockSystem();
			}

		if (this!=TheCurrentThread->iOwningProcess)
			{
			// Adding chunk to another process
			if (this==TheCurrentDataSectionProcess && !(attribs&DMemModelChunk::EFixedAddress))
				TheCompleteDataSectionProcess=NULL;	// just set partial state change flag and leave chunk alone
			if (this==TheCurrentAddressSpace)
				TheCurrentAddressSpace=NULL;
			NKern::UnlockSystem();
			goto done;	// FINISHED
			}

		// Adding chunk to currently active user process
		{
		TheCurrentAddressSpace=NULL;
		Mmu& m = Mmu::Get();
		TUint32 ff=0;	// flush flags
		DMemModelChunk::TChunkState state=isReadOnly?DMemModelChunk::ERunningRO:DMemModelChunk::ERunningRW;
		if (attribs&DMemModelChunk::EFixedAddress)
			{
			// Fixed address chunk, just change permissions
			ff|=aChunk->ApplyTopLevelPermissions(state);
			}
		else if (this==TheCurrentDataSectionProcess)
			{
			// Moving chunk.
			// This process is already in the data section, so just move the chunk down.
			// Must do flushing first
			TheCompleteDataSectionProcess=NULL;
			FlushBeforeChunkMove(aChunk);
			aChunk->MoveToRunAddress(aDataSectionBase,state);	// idempotent
			TheCompleteDataSectionProcess=this;
			}
		else if (iNumMovingChunks==1)
			{
			// The first moving chunk being added to a process with the data section occupied by another process.
			// This is the problematic case - we must displace the other process from the data section.
			// However we must allow preemption after each chunk is moved. Note that if a reschedule does
			// occur the necessary chunk moves will have been done by the scheduler, so we can finish
			// immediately.
			// Must do cache flushing first
			m.GenericFlush(Mmu::EFlushDMove);
			if (TheCurrentDataSectionProcess)
				{
				if (TheCurrentDataSectionProcess->iAttributes & EVariableCode)
					ff |= Mmu::EFlushIPermChg;
				SChunkInfo* pOtherProcChunks=TheCurrentDataSectionProcess->iChunks;
				SChunkInfo* pEndOtherProcChunks=pOtherProcChunks+TheCurrentDataSectionProcess->iNumChunks;
				NKern::FlashSystem();
				// if a reschedule occurs, TheCompleteDataSectionProcess will become equal to this
				while (TheCompleteDataSectionProcess!=this && pOtherProcChunks<pEndOtherProcChunks)
					{
					DMemModelChunk *pChunk=pOtherProcChunks->iChunk;
					pChunk->MoveToHomeSection();
					++pOtherProcChunks;
					TheCompleteDataSectionProcess=NULL;
					NKern::FlashSystem();
					}
				}
			if (TheCompleteDataSectionProcess!=this)
				{
				if (attribs & DMemModelChunk::ECode)
					ff |= Mmu::EFlushIPermChg;
				aChunk->MoveToRunAddress(aDataSectionBase,state);
				TheCurrentDataSectionProcess=this;
				TheCompleteDataSectionProcess=this;
				}
			}
		TheCurrentAddressSpace=this;
		TheCurrentVMProcess=this;
		if (ff)
			m.GenericFlush(ff);
		}
	}
	NKern::UnlockSystem();
done:
	__KTRACE_OPT(KPROC,Kern::Printf("Added array entry for %x",aDataSectionBase));
	__KTRACE_OPT(KPROC,Kern::Printf("Chunks maxsize %x",pC->iChunk->MaxSize()));
	__DEBUG_EVENT(EEventUpdateProcess, this);
	return KErrNone;
	}

TInt DMemModelProcess::AllocateDataSectionBase(DMemModelChunk& aChunk, TUint& aBase)
	{
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess::AllocateDataSectionBase"));
	aBase=0;
	if ((aChunk.iAttributes & DMemModelChunk::EPrivate) && this!=aChunk.iOwningProcess)
		return KErrAccessDenied;
	if (aChunk.iAttributes & DMemModelChunk::EFixedAddress)
		{
		aBase=aChunk.iHomeRegionBase;
		return KErrNone;
		}
	Mmu& m = Mmu::Get();
	TLinAddr base=0;
	TLinAddr maxBase=0;
	switch (aChunk.iChunkType)
		{
	case EUserData:
		base=m.iDataSectionBase;
		maxBase=m.iDllDataBase;
		break;
	case EUserCode:
	case EUserSelfModCode:
		MM::Panic(MM::EUserCodeNotFixed);
		break;
	case EDllData:
		aBase=m.iDllDataBase;
		return KErrNone;
	default:
		__KTRACE_OPT(KPANIC,Kern::Printf("DMemModelProcess::AllocateDataSectionBase BadChunkType %d",aChunk.iChunkType));
		return KErrAccessDenied;
		}

	TLinAddr lastBase=base;
	SChunkInfo *pS=iChunks;
	SChunkInfo *pE=pS+iNumChunks;
	while (pS<pE)
		{
		TLinAddr thisBase=pS->iDataSectionBase;
		__KTRACE_OPT(KPROC,Kern::Printf("Chunk already at %x",thisBase));
		if (thisBase>=maxBase)
			break;
		if (thisBase>=base) // Within the range we are allocating
			{
			TInt gap=thisBase-lastBase;
			if (gap>=aChunk.MaxSize())
				break;
			lastBase=thisBase+pS->iChunk->MaxSize();
			}
		pS++;
		}
	if (lastBase+aChunk.MaxSize()>maxBase)
		{
		__KTRACE_OPT(KPROC,Kern::Printf("ERROR - none allocated, out of memory"));
		return KErrNoMemory;
		}
	aBase=lastBase;
	__KTRACE_OPT(KPROC,Kern::Printf("User allocated %x",aBase));
	return KErrNone;
	}

TUint8* DMemModelProcess::DataSectionBase(DMemModelChunk* aChunk)
	{
	// this can't be called after $LOCK is deleted
	Kern::MutexWait(*iProcessLock);
	TInt pos=0;
	TInt r=ChunkIndex(aChunk,pos);
	if (r==0) // Found the chunk
		{
		TUint8* answer=((TUint8*)iChunks[pos].iDataSectionBase);
		__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess::DataSectionBase %x",answer));
		Kern::MutexSignal(*iProcessLock);
		return answer;
		}
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess::DataSectionBase chunk %08x not present in %08x",aChunk,this));
	Kern::MutexSignal(*iProcessLock);
	return(NULL);
	}

void DMemModelProcess::DoRemoveChunk(TInt aIndex)
	{
	// Must be called with process $LOCK mutex held
	__DEBUG_EVENT(EEventUpdateProcess, this);
	DMemModelChunk* chunk = iChunks[aIndex].iChunk;
	Mmu& m = Mmu::Get();
	NKern::LockSystem();
	TInt attribs=chunk->iAttributes;
	__KTRACE_OPT(KPROC,Kern::Printf("Removing Chunk attribs=%08x, Process attribs=%08x",attribs,iAttributes));
	if (!(attribs&DMemModelChunk::EFixedAccess))
		{
		// Must leave chunk in process chunk list until we have flushed the cache if necessary
		if (this==TheCurrentVMProcess && (attribs&DMemModelChunk::EFixedAddress))
			{
			TUint32 ff=chunk->ApplyTopLevelPermissions(DMemModelChunk::ENotRunning);
			m.GenericFlush(ff);
			// the system must now remain locked until the chunk is removed from the process chunk list
			}
		if (this==TheCurrentDataSectionProcess && !(attribs&DMemModelChunk::EFixedAddress))
			{
			// must do cache flush first
			FlushBeforeChunkMove(chunk);	// preemptible, but on return cache is free of chunk data
			chunk->MoveToHomeSection();
			// the system must now remain locked until the chunk is removed from the process chunk list
			}
		}

	// Remove the chunk from the process chunk list
	SChunkInfo *pD=iChunks+aIndex;
	SChunkInfo *pS=iChunks+aIndex+1;
	SChunkInfo *pE=iChunks+iNumChunks;
	while(pS<pE)
		*pD++=*pS++;
	iNumChunks--;

	// Update the process attribute flags
	if (!(attribs&DMemModelChunk::EFixedAddress))
		{
		if (--iNumMovingChunks==0)
			iAttributes &= ~EMoving;
		}
	if (!(attribs&DMemModelChunk::EFixedAccess))
		{
		if ((attribs&DMemModelChunk::ECode) && --iNumNonFixedAccessCodeChunks==0)
				iAttributes &= ~EVariableCode;
		if (this==TheCurrentDataSectionProcess && !(iAttributes&EMoving))
			{
			TheCurrentDataSectionProcess=NULL;
			TheCompleteDataSectionProcess=NULL;
			}
		if (--iNumNonFixedAccessChunks==0)
			{
			iAttributes &= ~EVariableAccess;
			if (this==TheCurrentVMProcess)
				{
				TheCurrentVMProcess=NULL;
				TheCurrentAddressSpace=NULL;
				}
			NKern::UnlockSystem();
			DoAttributeChange();	// change process from variable to fixed access
			}
		else
			NKern::UnlockSystem();
		}
	else
		{
		NKern::UnlockSystem();
		RemoveFixedAccessChunk(chunk);
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
	while(iNumChunks)
		DoRemoveChunk(0);
	}

void DMemModelProcess::RemoveChunk(DMemModelChunk *aChunk)
	{
	// note that this can't be called after the process $LOCK mutex has been deleted
	// since it can only be called by a thread in this process doing a handle close or
	// dying, or by the process handles array being deleted due to the process dying,
	// all of which happen before $LOCK is deleted.
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess::RemoveChunk %08x from %08x",aChunk,this));
	Kern::MutexWait(*iProcessLock);
	TInt pos=0;
	TInt r=ChunkIndex(aChunk,pos);
	__KTRACE_OPT(KPROC,if(r) Kern::Printf("Chunk lookup failed with %d",r));
	if (r==0) // Found the chunk
		{
		__KTRACE_OPT(KPROC,Kern::Printf("Chunk access count %d",iChunks[pos].iAccessCount));
		if (--iChunks[pos].iAccessCount==0)
			DoRemoveChunk(pos);
		}
	Kern::MutexSignal(*iProcessLock);
	}

TInt DMemModelProcess::ChunkIndex(DMemModelChunk* aChunk,TInt& aPos)
	{
	if (aChunk==NULL)
		return(KErrNotFound);
	TInt i=0;
	SChunkInfo *pC=iChunks;
	SChunkInfo *pE=pC+iNumChunks;
	while(pC<pE && (pC->iChunk!=aChunk))
		{
		pC++;
		i++;
		}
	if (pC==pE)
		return KErrNotFound;
	aPos=i;
	return KErrNone;
	}

void DMemModelProcess::RemoveDllData()
//
// Call with CodeSegLock held
//
	{
	Kern::SafeClose((DObject*&)iDllDataChunk, this);
	}

TInt DMemModelProcess::CreateDllDataChunk()
//
// Call with CodeSegLock held
//
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelProcess %O CreateDllDataChunk",this));
	Mmu& m = Mmu::Get();
	SChunkCreateInfo c;
	c.iGlobal=EFalse;
	c.iAtt=TChunkCreate::EDisconnected;
	c.iForceFixed=EFalse;
	c.iOperations=SChunkCreateInfo::EAdjust|SChunkCreateInfo::EAdd;
	c.iRunAddress=0;
	c.iPreallocated=0;
	c.iType=EDllData;
	c.iMaxSize=(iAttributes&EFixedAddress) ? 1 : m.iMaxDllDataSize;	// minimal size for fixed processes
	c.iName.Set(KLitDllDollarData);
	c.iOwner=this;
	c.iInitialBottom=0;
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
		Mmu& m = Mmu::Get();
		TLinAddr dll_data_base=(iAttributes & EFixedAddress) ? (TLinAddr)iDllDataChunk->Base()
														: TLinAddr(m.iDllDataBase);
		TInt offset=aBase-dll_data_base;
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
	Mmu& m = Mmu::Get();
	TLinAddr dll_data_base=(iAttributes & EFixedAddress) ? (TLinAddr)iDllDataChunk->Base()
													: TLinAddr(m.iDllDataBase);
	TInt offset=aBase-dll_data_base;
	TInt r=iDllDataChunk->Decommit(offset, aSize);
	__ASSERT_ALWAYS(r==KErrNone,MM::Panic(MM::EDecommitInvalidDllDataAddress));
	if (iDllDataChunk->iSize==0)
		FreeDllDataChunk();
	}

TInt DMemModelProcess::MapCodeSeg(DCodeSeg* aSeg)
	{
	DMemModelCodeSeg& seg=*(DMemModelCodeSeg*)aSeg;
	__KTRACE_OPT(KDLL,Kern::Printf("Process %O MapCodeSeg %C", this, aSeg));
	TBool kernel_only=( (seg.iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttKernel );
	if (kernel_only && !(iAttributes&ESupervisor))
		return KErrNotSupported;
	if (seg.iAttr&ECodeSegAttKernel || seg.iDataAllocBase==-1)
		return KErrNone;	// no extra mappings needed for kernel code or code with fixed data address
	TInt r=KErrNone;
	if (seg.IsDll())
		{
		TInt total_data_size;
		TLinAddr data_base;
		seg.GetDataSizeAndBase(total_data_size, data_base);
		if (r==KErrNone && total_data_size)
			{
			TInt size=Mmu::RoundToPageSize(total_data_size);
			r=CommitDllData(data_base, size);
			}
		}
	return r;
	}

void DMemModelProcess::UnmapCodeSeg(DCodeSeg* aSeg)
	{
	DMemModelCodeSeg& seg=*(DMemModelCodeSeg*)aSeg;
	__KTRACE_OPT(KDLL,Kern::Printf("Process %O UnmapCodeSeg %C", this, aSeg));
	if (seg.iAttr&ECodeSegAttKernel || seg.iDataAllocBase==-1)
		return;	// no extra mappings needed for kernel code or code with fixed data address
	if (seg.IsDll())
		{
		TInt total_data_size;
		TLinAddr data_base;
		seg.GetDataSizeAndBase(total_data_size, data_base);
		if (total_data_size)
			DecommitDllData(data_base, Mmu::RoundToPageSize(total_data_size));
		}
	}

TInt DMemModelProcess::NewShPool(DShPool*& /* aPool */, TShPoolCreateInfo& /* aInfo */)
	{
	return KErrNotSupported;
	}

TInt DThread::RawRead(const TAny* aSrc, TAny* aDest, TInt aLength, TInt aFlags, TIpcExcTrap* aExcTrap)
//
// Read from the thread's process.
// aSrc is run address of memory to read. The memory is in aThread's address space.
// aDest is the address of destination. The memory is in the current process's address space.
// aExcTrap, exception trap object to be updated if the actual memory access is performed on another memory area. It happens 
//           when  reading is performed in chunks or if home adress is read instead of the provided run address.
// Enter and return with system locked.
	{
	const TUint8* pS=(const TUint8*)aSrc;
	TUint8* pD=(TUint8*)aDest;
	const TUint8* pC=NULL;
	TBool check=ETrue;
	TBool suspect=EFalse;
	DThread* pT=TheCurrentThread;
	while (aLength)
		{
		if (check)
			{
			suspect=((aFlags & KCheckLocalAddress) && !MM::CurrentAddress(pT,pD,aLength,ETrue));
			if (iMState==EDead)
				return KErrDied;
			pC=(const TUint8*)MM::CurrentAddress(this,pS,aLength,EFalse);
			__KTRACE_OPT(KTHREAD2,Kern::Printf("DThread::RawRead %08x<-[%08x::%08x]%08x+%x",pD,this,pS,pC,aLength));
			if (!pC)
				return KErrBadDescriptor;
			}
		TInt len=Min(aLength,K::MaxMemCopyInOneGo);
		if (aExcTrap)
			{
			aExcTrap->iSize = (len + 2*(sizeof(TInt32)-1));//+6 is for the worst case. We do not have to be precise here.
			aExcTrap->iRemoteBase = (TLinAddr)pC & ~(sizeof(TInt32)-1);		
			if (aExcTrap->iLocalBase)
				aExcTrap->iLocalBase = (TLinAddr)pD & ~(sizeof(TInt32)-1);
			__KTRACE_OPT(KTHREAD2,Kern::Printf("DThread::RawRead exc. update: %08x %08x %08x",aExcTrap->iLocalBase,aExcTrap->iRemoteBase,aExcTrap->iSize));
			}

#ifdef __DEMAND_PAGING__
		XTRAP_PAGING_START(check);
		CHECK_PAGING_SAFE;
#endif

		suspect?(void)umemput(pD,pC,len):(void)memcpy(pD,pC,len);

#ifdef __DEMAND_PAGING__
		XTRAP_PAGING_END;
		if(check<0)
			return check; // paging error caused by bad client (I.e. 'this' thread was bad)
		if(check)
			{
			__KTRACE_OPT(KTHREAD2,Kern::Printf("DThread::RawRead paging trap, suspect %d, dest %08x, source %08x, length %d\n", suspect, pD, pC, len));
			continue;
			}
#endif

		pD+=len;
		pS+=len;
		pC+=len;
		aLength-=len;
		if (aLength)
			check=NKern::FlashSystem();
		}
	return KErrNone;
	}

TInt DThread::RawWrite(const TAny* aDest, const TAny* aSrc, TInt aLength, TInt aFlags, DThread* aOriginatingThread, TIpcExcTrap* aExcTrap)
//
// Write to the thread's process.
// aDest is run address of memory to write. It resides in this thread's address space.
// aSrc is address of the source buffer. It resides in the current process's address space.
// aOriginatingThread is the thread on behalf of which this operation is performed (eg client of device driver).
// Enter and return with system locked
// aExcTrap, exception trap object to be updated if the actual memory access is performed on another memory area. It happens 
//           when  reading is performed in chunks or if home adress is read instead of the provided run address.
//
	{
	TUint8* pD=(TUint8*)aDest;
	const TUint8* pS=(const TUint8*)aSrc;
	TUint8* pC=NULL;
	TBool check=ETrue;
	TBool suspect=EFalse;
	DThread* pT=TheCurrentThread;
	DThread* pO=aOriginatingThread;
	if (!pO)
		pO=pT;
	DProcess* pF=K::TheFileServerProcess;
	TBool special=(iOwningProcess==pF && pO->iOwningProcess==pF);
	while (aLength)
		{
		if (check)
			{
			suspect=((aFlags & KCheckLocalAddress) && !MM::CurrentAddress(pT,pS,aLength,EFalse));
			if (iMState==EDead)
				return KErrDied;
			pC=(TUint8*)MM::CurrentAddress(this,pD,aLength,ETrue);
			__KTRACE_OPT(KTHREAD2,Kern::Printf("DThread::RawRead [%08x::%08x]%08x<-%08x+%x",this,pD,pC,pS,aLength));
			if (!pC)
				{
				if (special)
					pC=pD;
				else
					return KErrBadDescriptor;
				}
			}
		TInt len=Min(aLength,K::MaxMemCopyInOneGo);
		if (aExcTrap)
			{
			aExcTrap->iSize = (len + 2*(sizeof(TInt32)-1));//+6 is for the worst case. We do not have to be precise here.
			aExcTrap->iRemoteBase = (TLinAddr)pC & ~(sizeof(TInt32)-1);	
			if (aExcTrap->iLocalBase)
				aExcTrap->iLocalBase = (TLinAddr)pS & ~(sizeof(TInt32)-1);
			__KTRACE_OPT(KTHREAD2,Kern::Printf("DThread::RawWrite exc. update %08x %08x %08x",aExcTrap->iLocalBase,aExcTrap->iRemoteBase,aExcTrap->iSize));
			}

#ifdef __DEMAND_PAGING__
		XTRAP_PAGING_START(check);
		// Must check that it is safe to page, unless we are reading from unpaged ROM in which case
		// we allow it.  umemget does this anyway, so we just need to check if suspect is not set.
		if (!suspect)
			{
			CHECK_PAGING_SAFE_RANGE((TLinAddr)aSrc, aLength);
			CHECK_DATA_PAGING_SAFE_RANGE((TLinAddr)aDest, aLength);
			}
#endif

		suspect?(void)umemget(pC,pS,len):(void)memcpy(pC,pS,len);

#ifdef __DEMAND_PAGING__
		XTRAP_PAGING_END
		if(check<0)
			return check; // paging error caused by bad client (I.e. 'this' thread was bad)
		if(check)
			{
			__KTRACE_OPT(KTHREAD2,Kern::Printf("DThread::RawWrite paging trap, suspect %d, dest %08x, src %08x, length %d\n", suspect, pC, pD, len));
			continue;
			}
#endif

		pD+=len;
		pS+=len;
		pC+=len;
		aLength-=len;
		if (aLength)
			check=NKern::FlashSystem();
		}
	return KErrNone;
	}

#ifdef __DEBUGGER_SUPPORT__

TInt CodeModifier::SafeWriteCode(DProcess* aProcess, TLinAddr aAddress, TInt aSize, TUint aValue, void* aOldValue)
	{
	//Set exception handler. Make sure the boundaries cover the worst case (aSize = 4)
	TIpcExcTrap xt;
	xt.iLocalBase=0;
	xt.iRemoteBase=(TLinAddr)aAddress&~3; //word aligned.
	xt.iSize=sizeof(TInt);
	xt.iDir=1;
	NKern::LockSystem();
	TInt r=xt.Trap(NULL);
	if (r==0)
		{
		r = WriteCode(aAddress, aSize, aValue, aOldValue);
		xt.UnTrap();
		}
	NKern::UnlockSystem();
	return r;	
	}

TInt CodeModifier::WriteCode(TLinAddr aAddress, TInt aSize, TUint aValue, void* aOldValue)
	{
	TUint userChunkBase = (TUint)MM::UserCodeChunk->Base();
	TRomHeader romHeader = Epoc::RomHeader();

	if (!((aAddress >= romHeader.iRomBase ) && (aAddress < (romHeader.iRomBase + romHeader.iUncompressedSize))))  //if not in ROM
		if ( (aAddress<userChunkBase) || (aAddress) > (userChunkBase+MM::UserCodeChunk->MaxSize()) ) //and not in non-XIP code
			return KErrBadDescriptor;

	// if page was moved by defrag there may be a cache line with the
	// wrong, old physical address, so we must invalidate this first.
	InternalCache::Invalidate(KCacheSelectD, (TLinAddr)aAddress, 4);

	//Copy data and clean/invalidate caches with interrupts disabled.
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

TInt DThread::ReadAndParseDesHeader(const TAny* aSrc, TDesHeader& aDest)
//
// Read the header of a remote descriptor.
// Enter and return with system locked
//
	{
	TInt r=KErrBadDescriptor;
	DThread* thread = TheCurrentThread;
	TRawDesHeader& header = (TRawDesHeader&)aDest;

#ifdef __DEMAND_PAGING__
retry:
	TInt pagingFault;
	XTRAP_PAGING_START(pagingFault);
	CHECK_PAGING_SAFE;
	thread->iIpcClient = this;
#endif

	const TUint32* pS=(const TUint32*)MM::CurrentAddress(this,aSrc,sizeof(TDesC8),EFalse);
	if (pS && KErrNone==Kern::SafeRead(pS,&header[0],sizeof(TUint32)))
		{
		TInt type=header[0]>>KShiftDesType8;
		static const TUint8 LengthLookup[16]={4,8,12,8,12,0,0,0,0,0,0,0,0,0,0,0};
		TInt len=LengthLookup[type];
		if(len>(TInt)sizeof(TUint32))
			{
			if(KErrNone==Kern::SafeRead(pS+1,&header[1],len-sizeof(TUint32)))
				r = type;
			// else, bad descriptor
			}
		else if(len)
			r = type;
		// else, bad descriptor
		}

#ifdef __DEMAND_PAGING__
	thread->iIpcClient = NULL;
	XTRAP_PAGING_END;
	if(pagingFault<0)
		return pagingFault; // paging error caused by bad client (I.e. 'this' thread was bad)
	if(pagingFault)
		goto retry;
#endif
	
	return (r < 0) ? r : K::ParseDesHeader(aSrc, header, aDest);
	}

DMemModelChunk* ChunkFromAddress(DThread* aThread, const TAny* aAddress)
	{
	DMemModelProcess* pP = (DMemModelProcess*)aThread->iOwningProcess;
	DMemModelProcess::SChunkInfo* pS=pP->iChunks;
	DMemModelProcess::SChunkInfo* pC=pS+pP->iNumChunks;
	while(--pC>=pS && TUint(pC->iDataSectionBase)>TUint(aAddress)) {};
	if(pC<pS)
		return 0;
	return pC->iChunk;
	}

/**
	Open a shared chunk in which a remote address range is located.
*/
DChunk* DThread::OpenSharedChunk(const TAny* aAddress, TBool aWrite, TInt& aOffset)
	{
	NKern::LockSystem();
	
	DMemModelProcess* pP = (DMemModelProcess*)iOwningProcess;
	DMemModelProcess::SChunkInfo* pS=pP->iChunks;
	DMemModelProcess::SChunkInfo* pC=pS+pP->iNumChunks;
	while(--pC>=pS && TUint(pC->iDataSectionBase)>TUint(aAddress)) {};
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
	if ((iOwningProcess->iAttributes & DMemModelProcess::EFixedAddress )==0)
		return KErrNotSupported;
	Mmu& m=(Mmu&)*MmuBase::TheMmu;
	return m.PreparePagesForDMA((TLinAddr)aLinAddr, aSize, aPhysicalPageList);
	}

TInt DThread::ReleaseMemoryFromDMA(const TAny* aLinAddr, TInt aSize, TPhysAddr* aPhysicalPageList)
	{
	if ((iOwningProcess->iAttributes & DMemModelProcess::EFixedAddress )==0)
		return KErrNotSupported;
	TInt pageCount = (((TInt)aLinAddr & KPageMask) + aSize + KPageMask) >> KPageShift;
	Mmu& m=(Mmu&)*MmuBase::TheMmu;
	return m.ReleasePagesFromDMA(aPhysicalPageList, pageCount);
	}
