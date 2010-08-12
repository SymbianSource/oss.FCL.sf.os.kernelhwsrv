// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <plat_priv.h>
#include "cache_maintenance.h"
#include "mm.h"
#include "mmu.h"
#include "mmanager.h"
#include "mobject.h"
#include "mpager.h"
#include "mcodepaging.h"

/**
Manager for memory objects containing demand paged executable code.
This is the memory used by DCodeSegMemory object to store the contents of RAM loaded
EXEs and DLLs which are to be demand paged.

This memory has associated information, supplied by the Loader, which enables
the executable's code to be located in the file system and its contents
relocated and fixed-up when demand loaded.

@see DPagedCodeInfo
@see MM::PagedCodeNew
*/
class DCodePagedMemoryManager : public DPagedMemoryManager
	{
private:
	// from DMemoryManager...
	virtual TInt New(DMemoryObject*& aMemory, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags);
	virtual void Destruct(DMemoryObject* aMemory);
	virtual void Free(DMemoryObject* aMemory, TUint aIndex, TUint aCount);
	virtual TInt CleanPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TPhysAddr*& aPageArrayEntry);

	// from DPagedMemoryManager...
	virtual void Init3();
	virtual TInt InstallPagingDevice(DPagingDevice* aDevice);
	virtual TInt AcquirePageReadRequest(DPageReadRequest*& aRequest, DMemoryObject* aMemory, TUint aIndex, TUint aCount);
	virtual TInt ReadPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr* aPages, DPageReadRequest* aRequest);
	virtual TBool IsAllocated(DMemoryObject* aMemory, TUint aIndex, TUint aCount);

private:
	/**
	Array of paging devices used for each media drive.
	This is a initialised by #InstallPagingDevice.
	Drives without paging devices have the null pointer in their entry.
	*/
	DPagingDevice* iDevice[KMaxLocalDrives];

public:
	/**
	The single instance of this manager class.
	*/
	static DCodePagedMemoryManager TheManager;

	friend DPagingDevice* CodePagingDevice(TInt aDriveNum);
	};


/**
Reference counted object containing a #TPagedCodeInfo.
This is a structure containing the information about a demand paged code segment
which is required to load and fixup its code section.

An instance of this object is created for each memory object being managed by
#DCodePagedMemoryManager, and a pointer to it is stored in the memory object's
DMemoryObject::iManagerData member.

@see TPagedCodeInfo
@see MM::PagedCodeLoaded
*/
class DPagedCodeInfo : public DReferenceCountedObject
	{
public:
	/**
	Return a reference to the embedded #TPagedCodeInfo.
	*/
	inline TPagedCodeInfo& Info()
		{ return iInfo; }
private:
	/**
	@copybrief TPagedCodeInfo
	*/
	TPagedCodeInfo iInfo;
	};


DCodePagedMemoryManager DCodePagedMemoryManager::TheManager;
DPagedMemoryManager* TheCodePagedMemoryManager = &DCodePagedMemoryManager::TheManager;


DPagingDevice* CodePagingDevice(TInt aDriveNum)
	{
	__NK_ASSERT_DEBUG(aDriveNum<KMaxLocalDrives);
	return DCodePagedMemoryManager::TheManager.iDevice[aDriveNum];
	}


void DCodePagedMemoryManager::Init3()
	{
	TRACEB(("DCodePagedMemoryManager::Init3()"));
	}


TInt DCodePagedMemoryManager::InstallPagingDevice(DPagingDevice* aDevice)
	{
	TRACEB(("DCodePagedMemoryManager::InstallPagingDevice(0x%08x)",aDevice));

	TUint codePolicy = TheSuperPage().KernelConfigFlags() & EKernelConfigCodePagingPolicyMask;
	TRACEB(("Code Paging Policy = %d", codePolicy >> EKernelConfigCodePagingPolicyShift));
	if(codePolicy == EKernelConfigCodePagingPolicyNoPaging)
		{
		// no paging allowed so end now...
		return KErrNone;
		}
	
	TInt i;
	for(i=0; i<KMaxLocalDrives; ++i)
		if(aDevice->iDrivesSupported&(1<<i))
			{
			TRACEB(("DCodePagedMemoryManager::InstallPagingDevice drive=%d",i));
			TAny* null = 0;
			if(aDevice->iType & DPagingDevice::EMediaExtension)
				__e32_atomic_store_ord_ptr(&iDevice[i], null);
			if(!__e32_atomic_cas_ord_ptr(&iDevice[i], &null, aDevice)) // set iDevice[i]=aDevice if it was originally 0
				{
				// paging device already registered...
				TRACEB(("DCodePagedMemoryManager::InstallPagingDevice returns ALREADY EXISTS!"));
				return KErrAlreadyExists;
				}
			// flag code paging is supported...
			__e32_atomic_ior_ord32(&K::MemModelAttributes, (TUint32)EMemModelAttrCodePaging);
			}

	return KErrNone;
	}


TInt DCodePagedMemoryManager::AcquirePageReadRequest(DPageReadRequest*& aRequest, DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	DPagingDevice* device = 0;
	MmuLock::Lock();
	DPagedCodeInfo* pagedCodeInfo = (DPagedCodeInfo*)aMemory->iManagerData;
	if(pagedCodeInfo)
		{
		TPagedCodeInfo& info = pagedCodeInfo->Info();
		device = iDevice[info.iCodeLocalDrive];
		}
	MmuLock::Unlock();

	if(!device)
		{
		aRequest = 0;
		return KErrNotFound;
		}

	aRequest = device->iRequestPool->AcquirePageReadRequest(aMemory,aIndex,aCount);
	return KErrNone;
	}


TInt DCodePagedMemoryManager::New(DMemoryObject*& aMemory, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags)
	{
	DPagedCodeInfo* pagedCodeInfo = new DPagedCodeInfo;
	if(!pagedCodeInfo)
		return KErrNoMemory;

	TInt r = DPagedMemoryManager::New(aMemory, aSizeInPages, aAttributes, aCreateFlags);
	if(r!=KErrNone)
		pagedCodeInfo->Close();
	else
		aMemory->iManagerData = pagedCodeInfo;

	return r;
	}


void DCodePagedMemoryManager::Destruct(DMemoryObject* aMemory)
	{
	MmuLock::Lock();
	DPagedCodeInfo* pagedCodeInfo = (DPagedCodeInfo*)aMemory->iManagerData; 
	aMemory->iManagerData = 0;
	MmuLock::Unlock();

	if(pagedCodeInfo)
		pagedCodeInfo->Close();

	// base call to free memory and close object...
	DPagedMemoryManager::Destruct(aMemory);
	}


void DCodePagedMemoryManager::Free(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	DoFree(aMemory,aIndex,aCount);
	}


TInt DCodePagedMemoryManager::CleanPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TPhysAddr*& aPageArrayEntry)
	{
	if(!aPageInfo->IsDirty())
		return KErrNone;

	// shouldn't be asked to clean a page which is writable...
	__NK_ASSERT_DEBUG(!aPageInfo->IsWritable());

	// Note, memory may have been modified by the CodeModifier class.

	// just mark page as clean as we don't try and preserve code modifications...
	ThePager.SetClean(*aPageInfo);

	return KErrNone;
	}


TInt ReadFunc(TAny* aArg1, TAny* aArg2, TLinAddr aBuffer, TInt aBlockNumber, TInt aBlockCount)
	{
	START_PAGING_BENCHMARK;
	TInt drive = (TInt)aArg1;
	TThreadMessage* msg = (TThreadMessage*)aArg2;
	DPagingDevice* device = CodePagingDevice(drive);
	TInt r = device->Read(msg, aBuffer, aBlockNumber, aBlockCount, drive);
	__NK_ASSERT_DEBUG(r!=KErrNoMemory); // not allowed to allocated memory, therefore can't fail with KErrNoMemory
	END_PAGING_BENCHMARK(EPagingBmReadMedia);
	return r;
	}


TInt DCodePagedMemoryManager::ReadPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr* aPages, DPageReadRequest* aRequest)
	{
	TRACE2(("DCodePagedMemoryManager::ReadPage(0x%08x,0x%08x,0x%08x,?,?)",aMemory,aIndex,aCount));

	__NK_ASSERT_DEBUG(aRequest->CheckUseContiguous(aMemory,aIndex,aCount));
	__ASSERT_CRITICAL;

	START_PAGING_BENCHMARK;

	MmuLock::Lock();
	DPagedCodeInfo* pagedCodeInfo = (DPagedCodeInfo*)aMemory->iManagerData;
	if(pagedCodeInfo)
		pagedCodeInfo->Open();
	MmuLock::Unlock();
	if(!pagedCodeInfo)
		return KErrNotFound;

	TPagedCodeInfo& info = pagedCodeInfo->Info();
	DPagingDevice& device = *iDevice[info.iCodeLocalDrive];

	TLinAddr linAddr = aRequest->MapPages(aIndex,aCount,aPages);
	TInt r = KErrNone;
	TThreadMessage message;

	if(!info.iCodeSize)
		{
		// no blockmap yet, use blank pages...
		memset((TAny*)linAddr, aCount*KPageSize, 0x03);
		CacheMaintenance::CodeChanged(linAddr, aCount*KPageSize);
		goto done;
		}

	for(; aCount; ++aIndex, --aCount, linAddr+=KPageSize)
		{
		// work out which bit of the file to read
		TInt codeOffset = aIndex<<KPageShift;
		TInt dataOffset;
		TInt dataSize;
		TInt decompressedSize = Min(KPageSize, info.iCodeSize-codeOffset);
		if(info.iCompressionType)
			{
			dataOffset = info.iCodePageOffsets[aIndex];
			dataSize = info.iCodePageOffsets[aIndex+1] - dataOffset;
			__KTRACE_OPT(KPAGING,Kern::Printf("  compressed, file offset == %x, size == %d", dataOffset, dataSize));
			}
		else
			{
			dataOffset = codeOffset + info.iCodeStartInFile;
			dataSize = Min(KPageSize, info.iBlockMap.DataLength()-dataOffset);
			__NK_ASSERT_DEBUG(dataSize==decompressedSize);
			__KTRACE_OPT(KPAGING,Kern::Printf("  uncompressed, file offset == %x, size == %d", dataOffset, dataSize));
			}

		TInt bufferStart = info.iBlockMap.Read(aRequest->Buffer(),
												dataOffset,
												dataSize,
												device.iReadUnitShift,
												ReadFunc,
												(TAny*)info.iCodeLocalDrive,
												(TAny*)&message);

		r = ThePager.EmbedErrorContext(EPagingErrorContextCodeRead, bufferStart); 
		if(r<0)
			break;

		TLinAddr data = aRequest->Buffer() + bufferStart;
		r = Decompress(info.iCompressionType, linAddr, decompressedSize, data, dataSize);
		if (r >= 0)
			r = (r == decompressedSize) ? KErrNone : KErrCorrupt;
		if(r != KErrNone)
			__KTRACE_OPT(KPANIC, Kern::Printf("DCodePagedMemoryManager::ReadPage: error decompressing page at %08x + %x: %d", dataOffset, dataSize, r));
		r = ThePager.EmbedErrorContext(EPagingErrorContextCodeDecompress, r); 			
		if(r!=KErrNone)
			break;

		if(decompressedSize<KPageSize)
			memset((TAny*)(linAddr+decompressedSize), KPageSize-decompressedSize, 0x03);
		if(info.iLoaded)
			info.ApplyFixups(linAddr, aIndex);
		}
done:
	aRequest->UnmapPages(true);

	pagedCodeInfo->AsyncClose();

	END_PAGING_BENCHMARK_N(EPagingBmReadCodePage, aCount);
	return r;
	}


TBool DCodePagedMemoryManager::IsAllocated(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	// all pages allocated if memory not destroyed (iManagerData!=0)...
	return aMemory->iManagerData!=0;
	}


TInt MM::PagedCodeNew(DMemoryObject*& aMemory, TUint aPageCount, TPagedCodeInfo*& aInfo)
	{
	TRACE(("MM::PagedCodeNew(?,0x%08x,0x%08x)",aPageCount,aInfo));
	TMemoryCreateFlags createFlags = (TMemoryCreateFlags)(EMemoryCreateNoWipe | EMemoryCreateAllowExecution);
	TInt r = TheCodePagedMemoryManager->New(aMemory,aPageCount,EMemoryAttributeStandard,createFlags);
	if(r==KErrNone)
		aInfo = &((DPagedCodeInfo*)aMemory->iManagerData)->Info();
	TRACE(("MM::PagedCodeNew returns %d, aMemory=0x%08x",r,aMemory));
	return r;
	}


void MM::PagedCodeLoaded(DMemoryObject* aMemory, TLinAddr aLoadAddress)
	{
	TRACE(("MM::PagedCodeLoaded(0x%08x,0x%08x)",aMemory,aLoadAddress));

	TPagedCodeInfo& info = ((DPagedCodeInfo*)aMemory->iManagerData)->Info();

	// we need to apply fixups for all memory already paged in.
	// Note, if this memory is subsequently discarded it should not be paged-in again
	// until after this function has completed, because the Loader won't touch the memory
	// and it has not yet been mapped into any other process.

	// make iterator for memory...
	RPageArray::TIter pageIter;
	aMemory->iPages.FindStart(0,aMemory->iSizeInPages,pageIter);

	for(;;)
		{
		// find some pages...
		RPageArray::TIter pageList;
		TUint n = pageIter.Find(pageList);
		if(!n)
			break;

		// fix up each page found...
		UNLOCK_USER_MEMORY();
		do
			{
			TUint i = pageList.Index();
			TLinAddr a = aLoadAddress+i*KPageSize;
			info.ApplyFixups(a,i);
			CacheMaintenance::CodeChanged(a, KPageSize);
			// now we've finished updating the page, mark it as read only and 
			// clean as we don't need to save changes if it is stolen.
			MmuLock::Lock();
			TPhysAddr* pages;
			if(pageList.Pages(pages,1)==1)
				if(RPageArray::IsPresent(*pages))
					{// The loader page still has a writable mapping but it won't
					// touch the page again so this is safe.  No use restricting the 
					// page to be read only as if the loader did write to it again 
					// it would just be rejuvenated as writeable and made dirty.
					SPageInfo& pageInfo = *SPageInfo::FromPhysAddr(*pages);
					pageInfo.SetReadOnly();
					ThePager.SetClean(pageInfo);
					}
			MmuLock::Unlock();

			pageList.Skip(1);
			}
		while(pageList.Count());
		LOCK_USER_MEMORY();

		// move on...
		pageIter.FindRelease(n);
		}

	// done...
	aMemory->iPages.FindEnd(0,aMemory->iSizeInPages);
	info.iLoaded = true; // allow ReadPage to start applying fixups when handling page faults
	}
