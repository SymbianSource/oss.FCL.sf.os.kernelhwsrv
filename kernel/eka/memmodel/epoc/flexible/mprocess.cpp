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
//

#include <memmodel.h>
#include "mmu/mm.h"
#include "mmu/maddrcont.h"
#include "mmboot.h"
#include <kernel/cache.h>
#include "execs.h"

#define iMState		iWaitLink.iSpare1

NFastMutex TheSharedChunkLock;

#ifndef _DEBUG
const TInt KChunkGranularity = 4; // amount to grow SChunkInfo list by
const TInt KMaxChunkInfosInOneGo = 100; // max number of SChunkInfo objects to copy with System Lock held
#else // if debug...
const TInt KChunkGranularity = 1;
const TInt KMaxChunkInfosInOneGo = 1;
#endif



/********************************************
 * Process
 ********************************************/

DMemModelProcess::~DMemModelProcess()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelProcess destruct"));
	Destruct();
	}


void DMemModelProcess::Destruct()
	{
	__ASSERT_ALWAYS(!iOsAsidRefCount, MM::Panic(MM::EProcessDestructOsAsidRemaining));
	__ASSERT_ALWAYS(!iChunkCount, MM::Panic(MM::EProcessDestructChunksRemaining));
	Kern::Free(iChunks);
	__ASSERT_ALWAYS(!iSharedChunks || iSharedChunks->Count()==0, MM::Panic(MM::EProcessDestructChunksRemaining));
	delete iSharedChunks;

	DProcess::Destruct();
	}


TInt DMemModelProcess::TryOpenOsAsid()
	{
	if (__e32_atomic_tas_ord32(&iOsAsidRefCount, 1, 1, 0))
		{
		return iOsAsid;
		}
	return KErrDied;
	}


void DMemModelProcess::CloseOsAsid()
	{
	if (__e32_atomic_tas_ord32(&iOsAsidRefCount, 1, -1, 0) == 1)
		{// Last reference has been closed so free the asid.
		MM::AddressSpaceFree(iOsAsid);
		}
	}


void DMemModelProcess::AsyncCloseOsAsid()
	{
	if (__e32_atomic_tas_ord32(&iOsAsidRefCount, 1, -1, 0) == 1)
		{// Last reference has been closed so free the asid asynchronusly.
		MM::AsyncAddressSpaceFree(iOsAsid);
		}
	}


TInt DMemModelProcess::NewChunk(DChunk*& aChunk, SChunkCreateInfo& aInfo, TLinAddr& aRunAddr)
	{
	aChunk=NULL;

	DMemModelChunk* pC=new DMemModelChunk;
	if (!pC)
		return KErrNoMemory;

	TChunkType type = aInfo.iType;
	pC->iChunkType=type;
	TInt r=pC->SetAttributes(aInfo);
	if (r!=KErrNone)
		{
		pC->Close(NULL);
		return r;
		}

	pC->iOwningProcess=(pC->iAttributes&DMemModelChunk::EPublic)?NULL:this;
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
		r = AddChunk(pC, EFalse);
		}
	if (r==KErrNone)
		{
		if(pC->iKernelMapping)
			aRunAddr = (TLinAddr)MM::MappingBase(pC->iKernelMapping);
		pC->iDestroyedDfc = aInfo.iDestroyedDfc;
		aChunk=(DChunk*)pC;
		}
	else
		pC->Close(NULL);	// NULL since chunk can't have been added to process
	return r;
	}


/**
Determine whether this process should be data paged.

@param aInfo	A reference to the create info for this process.
 */
TInt DMemModelProcess::SetPaging(const TProcessCreateInfo& aInfo)
	{
	TUint pagedFlags = aInfo.iFlags & TProcessCreateInfo::EDataPagingMask;
	// If KImageDataPaged and KImageDataUnpaged flags present then corrupt
	// Check this first to ensure that it is always verified.
	if (pagedFlags == TProcessCreateInfo::EDataPagingMask)
		{
		return KErrCorrupt;
		}

	if (aInfo.iAttr & ECodeSegAttKernel ||
		!(K::MemModelAttributes & EMemModelAttrDataPaging))
		{// Kernel process shouldn't be data paged or no data paging device installed.
		return KErrNone;
		}

	TUint dataPolicy = TheSuperPage().KernelConfigFlags() & EKernelConfigDataPagingPolicyMask;
	if (dataPolicy == EKernelConfigDataPagingPolicyAlwaysPage)
		{
		iAttributes |= EDataPaged;
		return KErrNone;
		}
	if (dataPolicy == EKernelConfigDataPagingPolicyNoPaging)
		{// No paging allowed so just return.
		return KErrNone;
		}
	if (pagedFlags == TProcessCreateInfo::EDataPaged)
		{
		iAttributes |= EDataPaged;
		return KErrNone;
		}
	if (pagedFlags == TProcessCreateInfo::EDataUnpaged)
		{// No paging set so just return.
		return KErrNone;
		}
	// Neither paged nor unpaged set so use default paging policy.
	// dataPolicy must be EKernelConfigDataPagingPolicyDefaultUnpaged or 
	// EKernelConfigDataPagingPolicyDefaultPaged.
	__NK_ASSERT_DEBUG(pagedFlags == TProcessCreateInfo::EDataPagingUnspecified);
	__NK_ASSERT_DEBUG(	dataPolicy == EKernelConfigDataPagingPolicyDefaultPaged ||
						dataPolicy == EKernelConfigDataPagingPolicyDefaultUnpaged);
	if (dataPolicy == EKernelConfigDataPagingPolicyDefaultPaged)
		{
		iAttributes |= EDataPaged;
		}
	return KErrNone;
	}


TInt DMemModelProcess::DoCreate(TBool aKernelProcess, TProcessCreateInfo& aInfo)
	{
	// Required so we can detect whether a process has been created and added 
	// to its object container by checking for iContainerID!=EProcess.
	__ASSERT_COMPILE(EProcess != 0);
	__KTRACE_OPT(KPROC,Kern::Printf(">DMemModelProcess::DoCreate %O",this));
	TInt r=KErrNone;

	if (aKernelProcess)
		{
		iAttributes |= ESupervisor;
		iOsAsid = KKernelOsAsid;
		}
	else
		{
		r = MM::AddressSpaceAlloc(iPageDir);
		if (r>=0)
			{
			iOsAsid = r;
			r = KErrNone;
			}
		}
	if (r == KErrNone)
		{// Add this process's own reference to its os asid.
		__e32_atomic_store_ord32(&iOsAsidRefCount, 1);
		}

#ifdef BTRACE_FLEXIBLE_MEM_MODEL
	BTrace8(BTrace::EFlexibleMemModel,BTrace::EAddressSpaceId,this,iOsAsid);
#endif

	__KTRACE_OPT(KPROC,Kern::Printf("OS ASID=%d, PD=%08x",iOsAsid,iPageDir));
	__KTRACE_OPT(KPROC,Kern::Printf("<DMemModelProcess::DoCreate %d",r));
	return r;
	}

TInt DMemModelProcess::CreateDataBssStackArea(TProcessCreateInfo& aInfo)
	{
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess::CreateDataBssStackArea %O",this));
	TInt r = KErrNone;
	TInt dataBssSize = MM::RoundToPageSize(aInfo.iTotalDataSize);
	if(dataBssSize)
		{
		DMemoryObject* memory;
		TMemoryObjectType memoryType = iAttributes&EDataPaged ? EMemoryObjectPaged : EMemoryObjectMovable;
		r = MM::MemoryNew(memory,memoryType,MM::BytesToPages(dataBssSize));
		if(r==KErrNone)
			{
			r = MM::MemoryAlloc(memory,0,MM::BytesToPages(dataBssSize));
			if(r==KErrNone)
				{
				r = MM::MappingNew(iDataBssMapping,memory,EUserReadWrite,OsAsid());
				}
			if(r!=KErrNone)
				MM::MemoryDestroy(memory);
			else
				{
				iDataBssRunAddress = MM::MappingBase(iDataBssMapping);
#ifdef BTRACE_FLEXIBLE_MEM_MODEL
				BTrace8(BTrace::EFlexibleMemModel,BTrace::EMemoryObjectIsProcessStaticData,memory,this);
#endif
				}
			}
		}
	__KTRACE_OPT(KPROC,Kern::Printf("DataBssSize=%x, ",dataBssSize));

	return r;
	}


TInt DMemModelProcess::AttachExistingCodeSeg(TProcessCreateInfo& aInfo)
	{
	TInt r = DEpocProcess::AttachExistingCodeSeg(aInfo);
	if(r==KErrNone)
		{
		// allocate virtual memory for the EXEs codeseg...
		DMemModelCodeSeg* seg = (DMemModelCodeSeg*)iTempCodeSeg;
		if(seg->iAttr&ECodeSegAttAddrNotUnique)
			{
			TUint codeSize = seg->iSize;
			TLinAddr codeAddr = seg->RamInfo().iCodeRunAddr;
			TBool isDemandPaged = seg->iAttr&ECodeSegAttCodePaged;
			// Allocate virtual memory for the code seg using the os asid.
			// No need to open a reference on os asid as process not fully 
			// created yet so it can't die and free the os asid.
			r = MM::VirtualAlloc(OsAsid(),codeAddr,codeSize,isDemandPaged);
			if(r==KErrNone)
				{
				iCodeVirtualAllocSize = codeSize;
				iCodeVirtualAllocAddress = codeAddr;
				}
			}
		}

	return r;
	}


TInt DMemModelProcess::AddChunk(DChunk* aChunk, TBool aIsReadOnly)
	{
	DMemModelChunk* pC=(DMemModelChunk*)aChunk;
	if(pC->iOwningProcess && this!=pC->iOwningProcess)
		return KErrAccessDenied;

	TInt r = WaitProcessLock();
	if(r==KErrNone)
		{
		TInt i = ChunkIndex(pC);
		if(i>=0) // Found the chunk in this process, just up its count
			{
			iChunks[i].iAccessCount++;
			__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess::AddChunk %08x to %08x (Access count incremented to %d)",aChunk,this,iChunks[i].iAccessCount));
			SignalProcessLock();
			return KErrNone;
			}
		r = DoAddChunk(pC,aIsReadOnly);
		SignalProcessLock();
		}
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess::AddChunk returns %d",r));
	return r;
	}


void M::FsRegisterThread()
	{
	TInternalRamDrive::Unlock();
	}


void ExecHandler::UnlockRamDrive()
	{
	}


EXPORT_C TLinAddr TInternalRamDrive::Base()
	{
	DMemModelChunk* pC=(DMemModelChunk*)PP::TheRamDriveChunk;
	DMemModelProcess* pP=(DMemModelProcess*)TheCurrentThread->iOwningProcess;
	NKern::LockSystem();
	TLinAddr addr = (TLinAddr)pC->Base(pP);
	NKern::UnlockSystem();
	if(!addr)
		{
		Unlock();
		NKern::LockSystem();
		addr = (TLinAddr)pC->Base(pP);
		NKern::UnlockSystem();
		}
	return addr;
	}


EXPORT_C void TInternalRamDrive::Unlock()
	{
	DMemModelChunk* pC=(DMemModelChunk*)PP::TheRamDriveChunk;
	DMemModelProcess* pP=(DMemModelProcess*)TheCurrentThread->iOwningProcess;
	
	TInt r = pP->WaitProcessLock();
	if(r==KErrNone)
		if(pP->ChunkIndex(pC)==KErrNotFound)
			r = pP->DoAddChunk(pC,EFalse);
	__ASSERT_ALWAYS(r==KErrNone, MM::Panic(MM::EFsRegisterThread));
	pP->SignalProcessLock();
	}


EXPORT_C void TInternalRamDrive::Lock()
	{
	}


TInt DMemModelProcess::DoAddChunk(DMemModelChunk* aChunk, TBool aIsReadOnly)
	{
	//
	// Must hold the process $LOCK mutex before calling this.
	// As the process lock is held it is safe to access iOsAsid without a reference.
	//

	__NK_ASSERT_DEBUG(ChunkIndex(aChunk)==KErrNotFound); // shouldn't be adding a chunk which is already added

	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess::DoAddChunk %O to %O",aChunk,this));

	// create mapping for chunk...
	DMemoryMapping* mapping;
	TMappingPermissions perm = MM::MappingPermissions
		(
		iOsAsid!=(TInt)KKernelOsAsid,	// user?
		aIsReadOnly==false, // write?
		aChunk->iAttributes&DMemModelChunk::ECode // execute?
		);
	TInt r;
	if(aChunk->iFixedBase) // HACK, kernel chunk has a fixed iBase
		r = MM::MappingNew(mapping,aChunk->iMemoryObject,perm,iOsAsid,EMappingCreateExactVirtual,(TLinAddr)aChunk->iFixedBase);
	else
		r = MM::MappingNew(mapping,aChunk->iMemoryObject,perm,iOsAsid);
	if(r!=KErrNone)
		return r;
	if(iOsAsid==0)
		aChunk->iKernelMapping = mapping;
	TLinAddr base = MM::MappingBase(mapping);

	// expand chunk info memory if required...
	if(iChunkCount==iChunkAlloc)
		{
		TInt newAlloc = iChunkAlloc+KChunkGranularity;
		r = Kern::SafeReAlloc((TAny*&)iChunks,iChunkAlloc*sizeof(SChunkInfo),newAlloc*sizeof(SChunkInfo));
		if(r!=KErrNone)
			{
			MM::MappingDestroy(mapping);
			return r;
			}
		iChunkAlloc = newAlloc;
		}

	// insert new chunk info...
	TUint i = ChunkInsertIndex(aChunk);
	SChunkInfo* info = iChunks+i;
	SChunkInfo* infoEnd = iChunks+iChunkCount;
	NKern::LockSystem();
	++iChunkCount;
	for(;;)
		{
		// make space for new chunk info by shuffling along
		// existing infos KMaxChunkInfosInOneGo at a time...
		SChunkInfo* infoPtr = infoEnd-KMaxChunkInfosInOneGo;
		if(infoPtr<info)
			infoPtr = info;
		memmove(infoPtr+1,infoPtr,(TLinAddr)infoEnd-(TLinAddr)infoPtr);
		infoEnd = infoPtr;
		if(infoEnd<=info)
			break;
		NKern::FlashSystem();
		}
	info->iChunk = aChunk;
	info->iMapping = mapping;
	info->iAccessCount = 1;
	info->iIsReadOnly = aIsReadOnly;
	NKern::UnlockSystem();

	// add chunk to list of Shared Chunks...
	if(aChunk->iChunkType==ESharedKernelSingle || aChunk->iChunkType==ESharedKernelMultiple)
		{
		if(!iSharedChunks)
			iSharedChunks = new RAddressedContainer(&TheSharedChunkLock,iProcessLock);
		if(!iSharedChunks)
			r = KErrNoMemory;
		else
			r = iSharedChunks->Add(base,aChunk);
		if(r!=KErrNone)
			{
			DoRemoveChunk(i);
			return r;
			}
		}

	// done OK...
	__DEBUG_EVENT(EEventUpdateProcess, this);
	return KErrNone;
	}


void DMemModelProcess::DoRemoveChunk(TInt aIndex)
	{
	__DEBUG_EVENT(EEventUpdateProcess, this);

	DMemModelChunk* chunk = iChunks[aIndex].iChunk;
	DMemoryMapping* mapping = iChunks[aIndex].iMapping;

	if(chunk->iChunkType==ESharedKernelSingle || chunk->iChunkType==ESharedKernelMultiple)
		{
		// remove chunk from list of Shared Chunks...
		if(iSharedChunks)
			{
			iSharedChunks->Remove(MM::MappingBase(mapping));
#ifdef _DEBUG
			// delete iSharedChunks if it's empty, so memory leak test code passes...
			if(iSharedChunks->Count()==0)
				{
				NKern::FMWait(&TheSharedChunkLock);
				RAddressedContainer* s = iSharedChunks;
				iSharedChunks = 0;
				NKern::FMSignal(&TheSharedChunkLock);
				delete s;
				}
#endif
			}
		}

	// remove chunk from array...
	SChunkInfo* infoStart = iChunks+aIndex+1;
	SChunkInfo* infoEnd = iChunks+iChunkCount;
	NKern::LockSystem();
	for(;;)
		{
		// shuffle existing infos down KMaxChunkInfosInOneGo at a time...
		SChunkInfo* infoPtr = infoStart+KMaxChunkInfosInOneGo;
		if(infoPtr>infoEnd)
			infoPtr = infoEnd;
		memmove(infoStart-1,infoStart,(TLinAddr)infoPtr-(TLinAddr)infoStart);
		infoStart = infoPtr;
		if(infoStart>=infoEnd)
			break;
		NKern::FlashSystem();
		}
	--iChunkCount;
	NKern::UnlockSystem();

	if(mapping==chunk->iKernelMapping)
		chunk->iKernelMapping = 0;

	MM::MappingDestroy(mapping);
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
	// Destroy the remaining mappings and memory objects owned by this process
	MM::MappingAndMemoryDestroy(iDataBssMapping);
	if(iCodeVirtualAllocSize)
		MM::VirtualFree(iOsAsid,iCodeVirtualAllocAddress,iCodeVirtualAllocSize);

	// Close the original reference on the os asid.
	CloseOsAsid();
	}


void DMemModelProcess::RemoveChunk(DMemModelChunk *aChunk)
	{
	// note that this can't be called after the process $LOCK mutex has been deleted
	// since it can only be called by a thread in this process doing a handle close or
	// dying, or by the process handles array being deleted due to the process dying,
	// all of which happen before $LOCK is deleted.
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess %O RemoveChunk %O",this,aChunk));
	Kern::MutexWait(*iProcessLock);
	TInt i = ChunkIndex(aChunk);
	if(i>=0) // Found the chunk
		{
		__KTRACE_OPT(KPROC,Kern::Printf("Chunk access count %d",iChunks[i].iAccessCount));
		if(--iChunks[i].iAccessCount==0)
			{
			DoRemoveChunk(i);
			}
		}
	Kern::MutexSignal(*iProcessLock);
	}


TUint8* DMemModelChunk::Base(DProcess* aProcess)
	{
	DMemModelProcess* pP = (DMemModelProcess*)aProcess;
	DMemoryMapping* mapping = 0;

	if(iKernelMapping && pP==K::TheKernelProcess)
		{
		// shortcut for shared chunks...
		mapping = iKernelMapping;
		}
	else
		{
		// find chunk in process...
		TInt i = pP->ChunkIndex(this);
		if(i>=0)
			mapping = pP->iChunks[i].iMapping;
		}

	if(!mapping)
		return 0;

	return (TUint8*)MM::MappingBase(mapping);
	}


DChunk* DThread::OpenSharedChunk(const TAny* aAddress, TBool aWrite, TInt& aOffset)
	{
	DMemModelChunk* chunk = 0;

	NKern::FMWait(&TheSharedChunkLock);
	RAddressedContainer* list = ((DMemModelProcess*)iOwningProcess)->iSharedChunks;
	if(list)
		{
		// search list...
		TUint offset;
		chunk = (DMemModelChunk*)list->Find((TLinAddr)aAddress,offset);
		if(chunk && offset<TUint(chunk->iMaxSize) && chunk->Open()==KErrNone)
			aOffset = offset; // chunk found and opened successfully
		else
			chunk = 0; // failed
		}
	NKern::FMSignal(&TheSharedChunkLock);

	return chunk;
	}


TUint DMemModelProcess::ChunkInsertIndex(DMemModelChunk* aChunk)
	{
	// need to hold iProcessLock or System Lock...
#ifdef _DEBUG
	if(K::Initialising==false && iProcessLock!=NULL && iProcessLock->iCleanup.iThread!=&Kern::CurrentThread())
		{
		// don't hold iProcessLock, so...
		__ASSERT_SYSTEM_LOCK;
		}
#endif

	// binary search...
	SChunkInfo* list = iChunks;
	TUint l = 0;
	TUint r = iChunkCount;
	TUint m;
	while(l<r)
		{
		m = (l+r)>>1;
		DChunk* x = list[m].iChunk;
		if(x<=aChunk)
			l = m+1;
		else
			r = m;
		}
	return r;
	}


TInt DMemModelProcess::ChunkIndex(DMemModelChunk* aChunk)
	{
	TUint i = ChunkInsertIndex(aChunk);
	if(i && iChunks[--i].iChunk==aChunk)
		return i;
	return KErrNotFound;
	}


TInt DMemModelProcess::MapCodeSeg(DCodeSeg* aSeg)
	{
	__ASSERT_CRITICAL;	// Must be in critical section so can't leak os asid references.
		
	DMemModelCodeSeg& seg=*(DMemModelCodeSeg*)aSeg;
	__KTRACE_OPT(KDLL,Kern::Printf("Process %O MapCodeSeg %C", this, aSeg));
	TBool kernel_only=( (seg.iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttKernel );
	TBool user_local=( (seg.iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == 0 );
	if (kernel_only && !(iAttributes&ESupervisor))
		return KErrNotSupported;
	if (seg.iAttr&ECodeSegAttKernel)
		return KErrNone;	// no extra mappings needed for kernel code

	// Attempt to open a reference on the os asid it is required so
	// MapUserRamCode() and CommitDllData() can use iOsAsid safely.
	TInt osAsid = TryOpenOsAsid();
	if (osAsid < 0)
		{// The process has died.
		return KErrDied;
		}

	TInt r=KErrNone;
	if (user_local)
		r=MapUserRamCode(seg.Memory());
	if (seg.IsDll())
		{
		TInt total_data_size;
		TLinAddr data_base;
		seg.GetDataSizeAndBase(total_data_size, data_base);
		if (r==KErrNone && total_data_size)
			{
			TInt size=MM::RoundToPageSize(total_data_size);
			r=CommitDllData(data_base, size, aSeg);
			if (r!=KErrNone && user_local)
				UnmapUserRamCode(seg.Memory());
			}
		}
	CloseOsAsid();

	return r;
	}


void DMemModelProcess::UnmapCodeSeg(DCodeSeg* aSeg)
	{
	__ASSERT_CRITICAL;	// Must be in critical section so can't leak os asid references.

	DMemModelCodeSeg& seg=*(DMemModelCodeSeg*)aSeg;
	__KTRACE_OPT(KDLL,Kern::Printf("Process %O UnmapCodeSeg %C", this, aSeg));
	if (seg.iAttr&ECodeSegAttKernel)
		return;	// no extra mappings needed for kernel code

	// Attempt to open a reference on the os asid it is required so
	// UnmapUserRamCode() and DecommitDllData() can use iOsAsid safely.
	TInt osAsid = TryOpenOsAsid();
	if (osAsid < 0)
		{// The process has died and it the process it will have cleaned up any code segs.
		return;
		}

	if (seg.IsDll())
		{
		TInt total_data_size;
		TLinAddr data_base;
		seg.GetDataSizeAndBase(total_data_size, data_base);
		if (total_data_size)
			DecommitDllData(data_base, MM::RoundToPageSize(total_data_size));
		}
	if (seg.Memory())
		UnmapUserRamCode(seg.Memory());

	CloseOsAsid();
	}

void DMemModelProcess::RemoveDllData()
//
// Call with CodeSegLock held
//
	{
	}


TInt DMemModelProcess::MapUserRamCode(DMemModelCodeSegMemory* aMemory)
	{
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess %O MapUserRamCode %C %d %d",
									this, aMemory->iCodeSeg, iOsAsid, aMemory->iPagedCodeInfo!=0));
	__ASSERT_MUTEX(DCodeSeg::CodeSegLock);

	TMappingCreateFlags createFlags = EMappingCreateExactVirtual;

	if(!(aMemory->iCodeSeg->iAttr&ECodeSegAttAddrNotUnique))
		{
		// codeseg memory address is globally unique, (common address across all processes)...
		FlagSet(createFlags,EMappingCreateCommonVirtual);
		}

	if(aMemory->iCodeSeg->IsExe())
		{
		// EXE codesegs have already had their virtual address allocated so we must adopt that...
		__NK_ASSERT_DEBUG(iCodeVirtualAllocSize);
		__NK_ASSERT_DEBUG(iCodeVirtualAllocAddress==aMemory->iRamInfo.iCodeRunAddr);
		iCodeVirtualAllocSize = 0;
		iCodeVirtualAllocAddress = 0;
		FlagSet(createFlags,EMappingCreateAdoptVirtual);
		}

	DMemoryMapping* mapping;
	return MM::MappingNew(mapping,aMemory->iCodeMemoryObject,EUserExecute,iOsAsid,createFlags,aMemory->iRamInfo.iCodeRunAddr);
	}


void DMemModelProcess::UnmapUserRamCode(DMemModelCodeSegMemory* aMemory)
	{
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess %O UnmapUserRamCode %C %d %d",
									this, aMemory->iCodeSeg, iOsAsid, aMemory->iPagedCodeInfo!=0));

	__ASSERT_MUTEX(DCodeSeg::CodeSegLock);
	MM::MappingDestroy(aMemory->iRamInfo.iCodeRunAddr,iOsAsid);
	}


TInt DMemModelProcess::CommitDllData(TLinAddr aBase, TInt aSize, DCodeSeg* aCodeSeg)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelProcess %O CommitDllData %08x+%x",this,aBase,aSize));

	DMemoryObject* memory;
	TMemoryObjectType memoryType = aCodeSeg->iAttr&ECodeSegAttDataPaged ? EMemoryObjectPaged : EMemoryObjectMovable;
	TInt r = MM::MemoryNew(memory,memoryType,MM::BytesToPages(aSize));
	if(r==KErrNone)
		{
		r = MM::MemoryAlloc(memory,0,MM::BytesToPages(aSize));
		if(r==KErrNone)
			{
			DMemoryMapping* mapping;
			r = MM::MappingNew(mapping,memory,EUserReadWrite,iOsAsid,EMappingCreateCommonVirtual,aBase);
			}
		if(r!=KErrNone)
			MM::MemoryDestroy(memory);
		else
			{
#ifdef BTRACE_FLEXIBLE_MEM_MODEL
			BTrace12(BTrace::EFlexibleMemModel,BTrace::EMemoryObjectIsDllStaticData,memory,aCodeSeg,this);
#endif
			}
		
		}
	__KTRACE_OPT(KDLL,Kern::Printf("CommitDllData returns %d",r));
	return r;
	}


void DMemModelProcess::DecommitDllData(TLinAddr aBase, TInt aSize)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelProcess %O DecommitDllData %08x+%x",this,aBase,aSize));
	MM::MappingAndMemoryDestroy(aBase,iOsAsid);
	}

void DMemModelProcess::BTracePrime(TInt aCategory)
	{
	DProcess::BTracePrime(aCategory);

#ifdef BTRACE_FLEXIBLE_MEM_MODEL
	if (aCategory == BTrace::EFlexibleMemModel || aCategory == -1)
		{
		BTrace8(BTrace::EFlexibleMemModel,BTrace::EAddressSpaceId,this,iOsAsid);

		if (iDataBssMapping)
			{
			DMemoryObject* memory = MM::MappingGetAndOpenMemory(iDataBssMapping);
			if (memory)
				{
				MM::MemoryBTracePrime(memory);
				BTrace8(BTrace::EFlexibleMemModel,BTrace::EMemoryObjectIsProcessStaticData,memory,this);
				MM::MemoryClose(memory);
				}
			}
		
		// Trace memory objects for DLL static data
		SDblQue cs_list;
		DCodeSeg::UnmarkAll(DCodeSeg::EMarkListDeps|DCodeSeg::EMarkUnListDeps);
		TraverseCodeSegs(&cs_list, NULL, DCodeSeg::EMarkListDeps, 0);
		SDblQueLink* anchor=&cs_list.iA;
		SDblQueLink* pL=cs_list.First();
		for(; pL!=anchor; pL=pL->iNext)
			{
			DMemModelCodeSeg* seg = _LOFF(pL,DMemModelCodeSeg,iTempLink);
			if (seg->IsDll())
				{
				TInt total_data_size;
				TLinAddr data_base;
				seg->GetDataSizeAndBase(total_data_size, data_base);
				if (total_data_size)
					{
					TUint offset;
					// The instance count can be ignored as a dll data mapping is only ever 
					// used with a single memory object.
					TUint mappingInstanceCount;
					NKern::ThreadEnterCS();
					DMemoryMapping* mapping = MM::FindMappingInAddressSpace(iOsAsid, data_base, 0, offset, mappingInstanceCount);
					if (mapping)
						{
						DMemoryObject* memory = MM::MappingGetAndOpenMemory(mapping);
						if (memory)
							{
							MM::MemoryBTracePrime(memory);
							BTrace12(BTrace::EFlexibleMemModel,BTrace::EMemoryObjectIsDllStaticData,memory,seg,this);
							MM::MemoryClose(memory);
							}
						MM::MappingClose(mapping);
						}
					NKern::ThreadLeaveCS();
					}
				}
			}
		DCodeSeg::EmptyQueue(cs_list, 0);	// leave cs_list empty
		}
#endif
	}


TInt DMemModelProcess::NewShPool(DShPool*& aPool, TShPoolCreateInfo& aInfo)
	{
	aPool = NULL;
	DMemModelShPool* pC = NULL;

	if (aInfo.iInfo.iFlags & TShPoolCreateInfo::EPageAlignedBuffer)
		{
		pC = new DMemModelAlignedShPool();
		}
	else
		{
		pC = new DMemModelNonAlignedShPool();
		}

	if (pC == NULL)
		{
		return KErrNoMemory;
		}

	TInt r = pC->Create(this, aInfo);

	if (r == KErrNone)
		{
		aPool = pC;
		}
	else
		{
		pC->Close(NULL);
		}

	return r;
	}


TInt DThread::RawRead(const TAny* aSrc, TAny* aDest, TInt aLength, TInt aFlags, TIpcExcTrap* aExcTrap)
//
// Read from the thread's process.
// aSrc      Run address of memory to read
// aDest     Current address of destination
// aExcTrap  Exception trap object to be updated if the actual memory access is performed on other memory area than specified.
//           It happens when  reading is performed on un-aligned memory area.
//
	{
	(void)aExcTrap;
	DMemModelThread& t=*(DMemModelThread*)TheCurrentThread;
	DMemModelProcess* pP=(DMemModelProcess*)iOwningProcess;
	TLinAddr src=(TLinAddr)aSrc;
	TLinAddr dest=(TLinAddr)aDest;
	TInt result = KErrNone;
	TBool have_taken_fault = EFalse;

	while (aLength)
		{
		if (iMState==EDead)
			{
			result = KErrDied;
			break;
			}
		TLinAddr alias_src;
		TUint alias_size;
		
#ifdef __BROADCAST_CACHE_MAINTENANCE__
		TInt pagingTrap;
		XTRAP_PAGING_START(pagingTrap);		
#endif

		TInt len = have_taken_fault ? Min(aLength, KPageSize - (src & KPageMask)) : aLength;
		TInt alias_result=t.Alias(src, pP, len, alias_src, alias_size);
		if (alias_result<0)
			{
			result = KErrBadDescriptor;	// bad permissions
			break;
			}
		
#ifdef __BROADCAST_CACHE_MAINTENANCE__
		// need to let the trap handler know where we are accessing in case we take a page fault
		// and the alias gets removed
		aExcTrap->iRemoteBase = alias_src;
		aExcTrap->iSize = alias_size;
#endif
			
		__KTRACE_OPT(KTHREAD2,Kern::Printf("DThread::RawRead %08x<-%08x+%x",dest,alias_src,alias_size));

		CHECK_PAGING_SAFE;

		if(aFlags&KCheckLocalAddress)
			MM::ValidateLocalIpcAddress(dest,alias_size,ETrue);
		UNLOCK_USER_MEMORY();
		memcpy( (TAny*)dest, (const TAny*)alias_src, alias_size);
		LOCK_USER_MEMORY();

		src+=alias_size;
		dest+=alias_size;
		aLength-=alias_size;

#ifdef __BROADCAST_CACHE_MAINTENANCE__
		XTRAP_PAGING_END;
		if(pagingTrap)
			have_taken_fault = ETrue;
#endif
		}
	t.RemoveAlias();
#ifdef __BROADCAST_CACHE_MAINTENANCE__
	t.iPagingExcTrap = NULL;  // in case we broke out of the loop and skipped XTRAP_PAGING_END
#endif

	return result;
	}


TInt DThread::RawWrite(const TAny* aDest, const TAny* aSrc, TInt aLength, TInt aFlags, DThread* /*anOriginatingThread*/, TIpcExcTrap* aExcTrap)
//
// Write to the thread's process.
// aDest               Run address of memory to write
// aSrc                Current address of destination
// aExcTrap            Exception trap object to be updated if the actual memory access is performed on other memory area then specified.
//                     It happens when reading is performed on un-aligned memory area.
//
	{
	(void)aExcTrap;
	DMemModelThread& t=*(DMemModelThread*)TheCurrentThread;
	DMemModelProcess* pP=(DMemModelProcess*)iOwningProcess;
	TLinAddr src=(TLinAddr)aSrc;
	TLinAddr dest=(TLinAddr)aDest;
	TInt result = KErrNone;
	TBool have_taken_fault = EFalse;

	while (aLength)
		{
		if (iMState==EDead)
			{
			result = KErrDied;
			break;
			}
		TLinAddr alias_dest;
		TUint alias_size;

#ifdef __BROADCAST_CACHE_MAINTENANCE__
		TInt pagingTrap;
		XTRAP_PAGING_START(pagingTrap);		
#endif
		
		TInt len = have_taken_fault ? Min(aLength, KPageSize - (dest & KPageMask)) : aLength;
		TInt alias_result=t.Alias(dest, pP, len, alias_dest, alias_size);
		if (alias_result<0)
			{
			result = KErrBadDescriptor;	// bad permissions
			break;
			}

#ifdef __BROADCAST_CACHE_MAINTENANCE__
		// need to let the trap handler know where we are accessing in case we take a page fault
		// and the alias gets removed
		aExcTrap->iRemoteBase = alias_dest;
		aExcTrap->iSize = alias_size;
#endif

		__KTRACE_OPT(KTHREAD2,Kern::Printf("DThread::RawWrite %08x+%x->%08x",src,alias_size,alias_dest));

		// Must check that it is safe to page, unless we are reading from unpaged ROM in which case
		// we allow it.
		CHECK_PAGING_SAFE_RANGE(src, aLength);
		CHECK_DATA_PAGING_SAFE_RANGE(dest, aLength);

		if(aFlags&KCheckLocalAddress)
			MM::ValidateLocalIpcAddress(src,alias_size,EFalse);
		UNLOCK_USER_MEMORY();
		memcpy( (TAny*)alias_dest, (const TAny*)src, alias_size);
		LOCK_USER_MEMORY();

		src+=alias_size;
		dest+=alias_size;
		aLength-=alias_size;

#ifdef __BROADCAST_CACHE_MAINTENANCE__
		XTRAP_PAGING_END;
		if(pagingTrap)
			have_taken_fault = ETrue;
#endif
 		}
	t.RemoveAlias();
#ifdef __BROADCAST_CACHE_MAINTENANCE__
	t.iPagingExcTrap = NULL;  // in case we broke out of the loop and skipped XTRAP_PAGING_END
#endif

	return result;
	}


#ifndef __MARM__

TInt DThread::ReadAndParseDesHeader(const TAny* aSrc, TDesHeader& aDest)
//
// Read the header of a remote descriptor.
//
	{
	static const TUint8 LengthLookup[16]={4,8,12,8,12,0,0,0,0,0,0,0,0,0,0,0};

	CHECK_PAGING_SAFE;
	
	DMemModelThread& t=*(DMemModelThread*)TheCurrentThread;
	DMemModelProcess* pP=(DMemModelProcess*)iOwningProcess;
	TLinAddr src=(TLinAddr)aSrc;

	__NK_ASSERT_DEBUG(t.iIpcClient==NULL);
	t.iIpcClient = this;
	
	TLinAddr pAlias;
	TUint8* pDest = (TUint8*)&aDest;
	TUint alias_size = 0;
	TInt length = 12;
	TInt type = KErrBadDescriptor;
	while (length > 0)
		{
#ifdef __BROADCAST_CACHE_MAINTENANCE__
		TInt pagingTrap;
		XTRAP_PAGING_START(pagingTrap);		
#endif
		
		if (alias_size == 0)  
			{
			// no alias present, so must create one here
			if (t.Alias(src, pP, length, pAlias, alias_size) != KErrNone)
				break;
			__NK_ASSERT_DEBUG(alias_size >= sizeof(TUint32));
			}

		// read either the first word, or as much as aliased of the remainder
		TInt l = length == 12 ? sizeof(TUint32) : Min(length, alias_size);
		if (Kern::SafeRead((TAny*)pAlias, (TAny*)pDest, l))
			break;  // exception reading from user space
		
		if (length == 12)  
			{
			// we have just read the first word, so decode the descriptor type
			type = *(TUint32*)pDest >> KShiftDesType8;
			length = LengthLookup[type];
			// invalid descriptor type will have length 0 which will get decrease by 'l' and
			// terminate the loop with length < 0
			}

		src += l;
		alias_size -= l;
		pAlias += l;
		pDest += l;
		length -= l;
		
#ifdef __BROADCAST_CACHE_MAINTENANCE__
		XTRAP_PAGING_END;
		if (pagingTrap)
			alias_size = 0;  // a page fault caused the alias to be removed
#endif
		}
	
	t.RemoveAlias();
	t.iIpcClient = NULL;
#ifdef __BROADCAST_CACHE_MAINTENANCE__
	t.iPagingExcTrap = NULL;  // in case we broke out of the loop and skipped XTRAP_PAGING_END
#endif
	return length == 0 ? K::ParseDesHeader(aSrc, (TRawDesHeader&)aDest, aDest) : KErrBadDescriptor;
	}


#endif


TInt DThread::PrepareMemoryForDMA(const TAny* aLinAddr, TInt aSize, TPhysAddr* aPhysicalPageList)
	{
	// not supported, new Physical Pinning APIs should be used for DMA
	return KErrNotSupported;
	}

TInt DThread::ReleaseMemoryFromDMA(const TAny* aLinAddr, TInt aSize, TPhysAddr* aPhysicalPageList)
	{
	// not supported, new Physical Pinning APIs should be used for DMA
	return KErrNotSupported;
	}

