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
#include "mm.h"
#include "mmu.h"

#include "mmanager.h"
#include "mobject.h"
#include "mmapping.h"
#include "mpager.h"
#include "mswap.h"


/**
Manages the swap via the data paging device.
*/
class DSwapManager
	{
public:

	enum TSwapFlags
		{
		EAllocated		= 1 << 0,
		EUninitialised	= 1 << 1,
		ESaved			= 1 << 2,
		ESwapFlagsMask 	= 0x7,

		ESwapIndexShift = 3,
		ESwapIndexMask = 0xffffffff << ESwapIndexShift,
		};

	TInt Create(DPagingDevice* aDevice);

	TInt ReserveSwap(DMemoryObject* aMemory, TUint aStartIndex, TUint aPageCount);
	TInt UnreserveSwap(DMemoryObject* aMemory, TUint aStartIndex, TUint aPageCount);
	TBool IsReserved(DMemoryObject* aMemory, TUint aStartIndex, TUint aPageCount);

	TInt ReadSwapPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TLinAddr aLinAddr, DPageReadRequest* aRequest, TPhysAddr* aPhysAddrs);
	TInt WriteSwapPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TLinAddr aLinAddr, DPageWriteRequest* aRequest);
	void DoDeleteNotify(TUint aSwapData);

	void GetSwapInfo(SVMSwapInfo& aInfoOut);
	TInt SetSwapThresholds(const SVMSwapThresholds& aThresholds);
	void CheckSwapThresholds(TUint aInitial, TUint aFinal);
	
protected:
	DPagingDevice* iDevice;
	TBitMapAllocator* iBitMap;
	TUint iBitMapFree;
	TUint iAllocOffset;
 	TUint iSwapThesholdLow;
 	TUint iSwapThesholdGood;
	TThreadMessage iDelNotifyMsg;
	};


/**
Manager for demand paged memory objects which contain writeable data.
The contents of the memory are written to a backing store whenever its
pages are 'paged out'.

@see DSwapManager
*/
class DDataPagedMemoryManager : public DPagedMemoryManager
	{
private:
	// from DMemoryManager...
	virtual TInt Alloc(DMemoryObject* aMemory, TUint aIndex, TUint aCount);
	virtual void Free(DMemoryObject* aMemory, TUint aIndex, TUint aCount);
	virtual TInt Wipe(DMemoryObject* aMemory);
	virtual TInt CleanPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TPhysAddr*& aPageArrayEntry);

	// Methods inherited from DPagedMemoryManager
	virtual void Init3();
	virtual TInt InstallPagingDevice(DPagingDevice* aDevice);
	virtual TInt AcquirePageReadRequest(DPageReadRequest*& aRequest, DMemoryObject* aMemory, TUint aIndex, TUint aCount);
	virtual TInt AcquirePageWriteRequest(DPageWriteRequest*& aRequest, DMemoryObject* aMemory, TUint aIndex, TUint aCount);
	virtual TInt ReadPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr* aPages, DPageReadRequest* aRequest);
	virtual TInt WritePages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr* aPages, DPageWriteRequest* aRequest);
	virtual TBool IsAllocated(DMemoryObject* aMemory, TUint aIndex, TUint aCount);

public:
	void GetSwapInfo(SVMSwapInfo& aInfoOut);
	TInt SetSwapThresholds(const SVMSwapThresholds& aThresholds);

private:
	/**
	The paging device used for accessing the backing store.
	This is set by #InstallPagingDevice.
	*/
	DPagingDevice* iDevice;

	/**
	The instance of #DSwapManager being used by this manager.
	*/
	DSwapManager* iSwapManager;

public:
	/**
	The single instance of this manager class.
	*/
	static DDataPagedMemoryManager TheManager;
	};


DDataPagedMemoryManager DDataPagedMemoryManager::TheManager;
DPagedMemoryManager* TheDataPagedMemoryManager = &DDataPagedMemoryManager::TheManager;


/**
Create a swap manager.

@param	aDevice	The demand paging device for access to the swap.
*/
TInt DSwapManager::Create(DPagingDevice* aDevice)
	{
	__ASSERT_COMPILE(!(ESwapIndexMask & ESwapFlagsMask));
	__NK_ASSERT_DEBUG(iDevice == NULL);
	iDevice = aDevice;

	// Create the structures required to track the swap usage.
	TUint swapPages = (iDevice->iSwapSize << iDevice->iReadUnitShift) >> KPageShift;
	// Can't have more swap pages than we can map.
	__NK_ASSERT_DEBUG(swapPages<=DMemoryObject::KMaxPagingManagerData);
	__NK_ASSERT_DEBUG(swapPages<=(KMaxTUint>>ESwapIndexShift));

	if ((TheMmu.TotalPhysicalRamPages() << 2) < swapPages)
		{// The swap is limited to a maximum of 4 times the amount of RAM.
		return KErrTooBig;
		}

	iBitMap = TBitMapAllocator::New(swapPages, ETrue);
	if (iBitMap == NULL)
		{// Not enough RAM to keep track of the swap.
		return KErrNoMemory;
		}
	iBitMapFree = swapPages;
	iAllocOffset = 0;
	return KErrNone;
	}


/**
Reserve some swap pages for the requested region of the memory object

@param aMemory		The memory object to reserve pages for.
@param aStartIndex	The page index in the memory object of the start of the region.
@param aPageCount	The number of pages to reserve.

@return KErrNone on success, KErrNoMemory if not enough swap space available.
@pre aMemory's lock is held.
@post aMemory's lock is held.
*/
TInt DSwapManager::ReserveSwap(DMemoryObject* aMemory, TUint aStartIndex, TUint aPageCount)
	{
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());

	const TUint indexEnd = aStartIndex + aPageCount;
	TUint index = aStartIndex;

#ifdef _DEBUG
	for (; index < indexEnd; index++)
		{// This page shouldn't already be in use.
		MmuLock::Lock();
		__NK_ASSERT_DEBUG(!(aMemory->PagingManagerData(index) & ESwapFlagsMask));
		MmuLock::Unlock();
		}
#endif

	if (iBitMapFree < aPageCount)
		{
		Kern::AsyncNotifyChanges(EChangesOutOfMemory);
		return KErrNoMemory;
		}
	// Reserve the required swap space and mark each page as allocated and uninitialised.
	TUint initFree = iBitMapFree;
	iBitMapFree -= aPageCount;
	for (index = aStartIndex; index < indexEnd; index++)
		{		
		// Grab MmuLock to stop manager data being accessed.
		MmuLock::Lock();
		TUint swapData = aMemory->PagingManagerData(index);
		__NK_ASSERT_DEBUG(!(swapData & EAllocated));
		swapData = EAllocated | EUninitialised;
		aMemory->SetPagingManagerData(index, swapData);
		MmuLock::Unlock();
		}

	CheckSwapThresholds(initFree, iBitMapFree);		
	return KErrNone;
	}


/**
Unreserve swap pages for the requested region of the memory object.

@param aMemory		The memory object to unreserve pages for.
@param aStartIndex	The page index in the memory object of the start of the region.
@param aPageCount	The number of pages to unreserve.

@return The number of pages freed.
@pre aMemory's lock is held.
@post aMemory's lock is held.
*/
TInt DSwapManager::UnreserveSwap(DMemoryObject* aMemory, TUint aStartIndex, TUint aPageCount)
	{
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());

	TUint initFree = iBitMapFree;
	TUint freedPages = 0;
	const TUint indexEnd = aStartIndex + aPageCount;
	for (TUint index = aStartIndex; index < indexEnd; index++)
		{
		// Grab MmuLock to stop manager data being accessed.
		MmuLock::Lock();
		TUint swapData = aMemory->PagingManagerData(index);
		TUint swapIndex = swapData >> ESwapIndexShift;
		TBool notifyDelete = EFalse;
		if (swapData & EAllocated)
			{
			if (swapData & ESaved)
				{
				notifyDelete = ETrue;
				iBitMap->Free(swapIndex);
				}
			freedPages++;
			aMemory->SetPagingManagerData(index, 0);
			}
#ifdef _DEBUG
		else
			__NK_ASSERT_DEBUG(swapData == 0);
#endif

		MmuLock::Unlock();

		if (notifyDelete)
			DoDeleteNotify(swapIndex);
		}
	iBitMapFree += freedPages;
	CheckSwapThresholds(initFree, iBitMapFree);	
	return freedPages;
	}


/**
Determine whether the specified pages in the memory object have swap reserved for them.

@param aMemory		The memory object that owns the pages.
@param aStartIndex	The first index of the pages to check.
@param aPageCount	The number of pages to check.

@return ETrue if swap is reserved for all the pages, EFalse otherwise.
*/
TBool DSwapManager::IsReserved(DMemoryObject* aMemory, TUint aStartIndex, TUint aPageCount)
	{// MmuLock required to protect manager data.
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aStartIndex < aMemory->iSizeInPages);
	__NK_ASSERT_DEBUG(aStartIndex + aPageCount <= aMemory->iSizeInPages);

	const TUint indexEnd = aStartIndex + aPageCount;
	for (TUint index = aStartIndex; index < indexEnd; index++)
		{
		if (!(aMemory->PagingManagerData(index) & DSwapManager::EAllocated))
			{// This page is not allocated by swap manager.
			return EFalse;
			}
		}
	return ETrue;
	}


/**
Read from the swap the specified pages associated with the memory object.

@param aMemory 	The memory object to read the pages for
@param aIndex	The index of the first page within the memory object.
@param aCount	The number of pages to read.
@param aLinAddr	The address to copy the pages to.
@param aRequest	The request to use for the read.
@param aPhysAddrs	An array of the physical addresses for each page to read in.
*/
TInt DSwapManager::ReadSwapPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TLinAddr aLinAddr, DPageReadRequest* aRequest, TPhysAddr* aPhysAddrs)
	{
	TInt r = KErrNone;
	const TUint readUnitShift = iDevice->iReadUnitShift;
	TUint readSize = KPageSize >> readUnitShift;
	TThreadMessage* msg = const_cast<TThreadMessage*>(&aRequest->iMessage);

	// Determine the wipe byte values for uninitialised pages.
	TUint allocFlags = aMemory->RamAllocFlags();
	TBool wipePages = !(allocFlags & Mmu::EAllocNoWipe);
	TUint8 wipeByte = (allocFlags & Mmu::EAllocUseCustomWipeByte) ? (allocFlags >> Mmu::EAllocWipeByteShift) & 0xff : 0x03;

	const TUint indexEnd = aIndex + aCount;
	for (TUint index = aIndex; index < indexEnd; index++, aLinAddr += KPageSize, aPhysAddrs++)
		{
		START_PAGING_BENCHMARK;

		MmuLock::Lock();	// MmuLock required for atomic access to manager data.
		TUint swapData = aMemory->PagingManagerData(index);

		if (!(swapData & EAllocated))
			{// This page is not committed to the memory object
			MmuLock::Unlock();
			return KErrNotFound;			
			}
		if (swapData & EUninitialised)
			{// This page has not been written to yet so don't read from swap 
			// just wipe it if required.
			MmuLock::Unlock();
			if (wipePages)
				{
				memset((TAny*)aLinAddr, wipeByte, KPageSize);
				}
			}
		else
			{
			__NK_ASSERT_DEBUG(swapData & ESaved);
			TUint swapIndex = swapData >> ESwapIndexShift;
			// OK to release as if the object's data is decommitted the pager 
			// will check that data is still valid before mapping it.
			MmuLock::Unlock();
			TUint readStart = (swapIndex << KPageShift) >> readUnitShift;
			START_PAGING_BENCHMARK;
			r = iDevice->Read(msg, aLinAddr, readStart, readSize, DPagingDevice::EDriveDataPaging);
			if (r != KErrNone)
				__KTRACE_OPT(KPANIC, Kern::Printf("DSwapManager::ReadSwapPages: error reading media at %08x + %x: %d", readStart << readUnitShift, readSize << readUnitShift, r));				
			__NK_ASSERT_DEBUG(r!=KErrNoMemory); // not allowed to allocate memory, therefore can't fail with KErrNoMemory
			END_PAGING_BENCHMARK(EPagingBmReadDataMedia);
			// TODO: Work out what to do if page in fails, unmap all pages????
			__NK_ASSERT_ALWAYS(r == KErrNone);
			}
		END_PAGING_BENCHMARK(EPagingBmReadDataPage);
		}

	return r;
	}


/**
Write the specified memory object's pages from the RAM into the swap.

@param	aMemory		The memory object who owns the pages.
@param	aIndex		The index within the memory object.
@param 	aCount		The number of pages to write out.
@param	aLinAddr	The location of the pages to write out.
@param	aRequest	The demand paging request to use.

*/
TInt DSwapManager::WriteSwapPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TLinAddr aLinAddr, DPageWriteRequest* aRequest)
	{// The RamAllocLock prevents the object's swap pages being reassigned.
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());

	// Write the page out to the swap.
	TInt r = KErrNone;
	const TUint readUnitShift = iDevice->iReadUnitShift;
	TUint writeSize = KPageSize >> readUnitShift;
	TThreadMessage* msg = const_cast<TThreadMessage*>(&aRequest->iMessage);

	const TUint indexEnd = aIndex + aCount;
	for (TUint index = aIndex; index < indexEnd; index++)
		{
		START_PAGING_BENCHMARK;

		MmuLock::Lock();
		TUint swapData = aMemory->PagingManagerData(index);
		// OK to release as ram alloc lock prevents manager data being updated.
		MmuLock::Unlock();
		if (!(swapData & EAllocated))
			{// This page is being decommited from aMemory so it is clean/unrequired.
			continue;
			}
		TInt swapIndex = swapData >> ESwapIndexShift;
		if (swapData & ESaved)
			{// An old version of this page has been saved to swap so free it now
			// as it will be out of date.
			iBitMap->Free(swapIndex);
			DoDeleteNotify(swapIndex);
			}
		// Get a new swap location for this page.
		swapIndex = iBitMap->AllocFrom(iAllocOffset);
		__NK_ASSERT_DEBUG(swapIndex != -1 && swapIndex < iBitMap->iSize);
		iAllocOffset = swapIndex + 1;
		if (iAllocOffset == (TUint)iBitMap->iSize)
			iAllocOffset = 0;

		TUint writeOffset = (swapIndex << KPageShift) >> readUnitShift;
		{
		START_PAGING_BENCHMARK;
		r = iDevice->Write(msg, aLinAddr, writeOffset, writeSize, EFalse);
		if (r != KErrNone)
			__KTRACE_OPT(KPANIC, Kern::Printf("DSwapManager::WriteSwapPages: error writing media at %08x + %x: %d", writeOffset << readUnitShift, writeSize << readUnitShift, r));				
		__NK_ASSERT_DEBUG(r!=KErrNoMemory); // not allowed to allocate memory, therefore can't fail with KErrNoMemory
		END_PAGING_BENCHMARK(EPagingBmWriteDataMedia);
		}
		// TODO: Work out what to do if page out fails.
		__NK_ASSERT_ALWAYS(r == KErrNone);
		MmuLock::Lock();
		// The swap data should not have been modified.
		__NK_ASSERT_DEBUG(swapData == aMemory->PagingManagerData(index));
		// Store the new swap location and mark the page as saved.
		swapData &= ~(EUninitialised | ESwapIndexMask);
		swapData |= (swapIndex << ESwapIndexShift) | ESaved;
		aMemory->SetPagingManagerData(index, swapData);
		MmuLock::Unlock();

		END_PAGING_BENCHMARK(EPagingBmWriteDataPage);
		}
	
	return r;
	}


/**
Notify the media driver that the page written to swap is no longer required.
*/
void DSwapManager::DoDeleteNotify(TUint aSwapIndex)
	{
	// Ram Alloc lock prevents the swap location being assigned to another page.
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());

#ifdef __PAGING_DELETE_NOTIFY_ENABLED
	const TUint readUnitShift = iDevice->iReadUnitShift;
	const TUint size = KPageSize >> readUnitShift;
	TUint offset = (aSwapIndex << KPageShift) >> readUnitShift;

	START_PAGING_BENCHMARK;
	// Ignore the return value as this is just an optimisation that is not supported on all media.
	(void)iDevice->DeleteNotify(&iDelNotifyMsg, offset, size);
	END_PAGING_BENCHMARK(EPagingBmDeleteNotifyDataPage);
#endif
	}


// Check swap thresholds and notify (see K::CheckFreeMemoryLevel)
void DSwapManager::CheckSwapThresholds(TUint aInitial, TUint aFinal)
	{
	TUint changes = 0;
	if (aFinal < iSwapThesholdLow && aInitial >= iSwapThesholdLow)
		changes |= (EChangesFreeMemory | EChangesLowMemory);
	if (aFinal >= iSwapThesholdGood && aInitial < iSwapThesholdGood)
		changes |= EChangesFreeMemory;
	if (changes)
		Kern::AsyncNotifyChanges(changes);
	}


void DSwapManager::GetSwapInfo(SVMSwapInfo& aInfoOut)
	{
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	aInfoOut.iSwapSize = iBitMap->iSize << KPageShift;
	aInfoOut.iSwapFree = iBitMapFree << KPageShift;
	}


TInt DSwapManager::SetSwapThresholds(const SVMSwapThresholds& aThresholds)
	{
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	if (aThresholds.iLowThreshold > aThresholds.iGoodThreshold)
		return KErrArgument;		
	TInt low = (aThresholds.iLowThreshold + KPageSize - 1) >> KPageShift;
	TInt good = (aThresholds.iGoodThreshold + KPageSize - 1) >> KPageShift;
	if (good > iBitMap->iSize)
		return KErrArgument;
	iSwapThesholdLow = low;
	iSwapThesholdGood = good;
	return KErrNone;
	}



TInt DDataPagedMemoryManager::InstallPagingDevice(DPagingDevice* aDevice)
	{
	TRACEB(("DDataPagedMemoryManager::InstallPagingDevice(0x%08x)",aDevice));

	TUint dataPolicy = TheSuperPage().KernelConfigFlags() & EKernelConfigDataPagingPolicyMask;
	TRACEB(("Data Paging Policy = %d", dataPolicy >> EKernelConfigDataPagingPolicyShift));
	if (dataPolicy == EKernelConfigDataPagingPolicyNoPaging)
		{// No paging allowed so don't register the device.
		return KErrNone;
		}

	// Store the device, blocking any other devices from installing.
	if (!NKern::CompareAndSwap((TAny*&)iDevice, (TAny*)NULL, (TAny*)aDevice))
		{// Data paging device already installed.
		__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("**** Attempt to install more than one data paging device !!!!!!!! ****"));
		return KErrAlreadyExists;
		}

	// Now we can determine the size of the swap, create the swap manager.
	iSwapManager = new DSwapManager;
	__NK_ASSERT_ALWAYS(iSwapManager);

	TInt r = iSwapManager->Create(iDevice);
	if (r != KErrNone)
		{// Couldn't create the swap manager.
		delete iSwapManager;
		iSwapManager = NULL;
		NKern::SafeSwap(NULL, (TAny*&)iDevice);
		return r;
		}
 	NKern::LockedSetClear(K::MemModelAttributes, 0, EMemModelAttrDataPaging);

	return r;
	}


TInt DDataPagedMemoryManager::AcquirePageReadRequest(DPageReadRequest*& aRequest, DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	aRequest = iDevice->iRequestPool->AcquirePageReadRequest(aMemory,aIndex,aCount);
	return KErrNone;
	}


TInt DDataPagedMemoryManager::AcquirePageWriteRequest(DPageWriteRequest*& aRequest, DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	aRequest = iDevice->iRequestPool->AcquirePageWriteRequest(aMemory,aIndex,aCount);
	return KErrNone;
	}


void DDataPagedMemoryManager::Init3()
	{
	}


TInt DDataPagedMemoryManager::Alloc(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	// re-initialise any decommitted pages which we may still own because they were pinned...
	ReAllocDecommitted(aMemory,aIndex,aCount);

	// Reserve the swap pages required.
	RamAllocLock::Lock();
	TInt r = iSwapManager->ReserveSwap(aMemory, aIndex, aCount);
	RamAllocLock::Unlock();

	return r;
	}


void DDataPagedMemoryManager::Free(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	TRACE2(("DDataPagedMemoryManager::Free(0x%08x,0x%x,0x%x)", aMemory, aIndex, aCount));
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	// Unreserve the swap pages associated with the memory object.  Do this before
	// removing the page array entries to prevent a page fault reallocating these pages.
	RamAllocLock::Lock();
	TInt freed = iSwapManager->UnreserveSwap(aMemory, aIndex, aCount);
	(void)freed;
	RamAllocLock::Unlock();

	DoFree(aMemory,aIndex,aCount);
	}


/**
@copydoc DMemoryManager::Wipe
@todo	Not yet implemented.
		Need to handle this smartly, e.g. throw RAM away and set to uninitialised 
*/
TInt DDataPagedMemoryManager::Wipe(DMemoryObject* aMemory)
	{
	__NK_ASSERT_ALWAYS(0); // not implemented yet

	return KErrNotSupported;
	}


TInt DDataPagedMemoryManager::ReadPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr* aPages, DPageReadRequest* aRequest)
	{
	__NK_ASSERT_DEBUG(aRequest->CheckUse(aMemory,aIndex,aCount));

	// Map pages temporarily so that we can copy into them.
	const TLinAddr linAddr = aRequest->MapPages(aIndex, aCount, aPages);

	TInt r = iSwapManager->ReadSwapPages(aMemory, aIndex, aCount, linAddr, aRequest, aPages);

	// The memory object allows executable mappings then need IMB.
	aRequest->UnmapPages(aMemory->IsExecutable());

	return r;
	}


TInt DDataPagedMemoryManager::WritePages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr* aPages, DPageWriteRequest* aRequest)
	{
	__NK_ASSERT_DEBUG(aRequest->CheckUse(aMemory,aIndex,aCount));

	// Map pages temporarily so that we can copy into them.
	const TLinAddr linAddr = aRequest->MapPages(aIndex, aCount, aPages);

	TInt r = iSwapManager->WriteSwapPages(aMemory, aIndex, aCount, linAddr, aRequest);

	// The memory object allows executable mappings then need IMB.
	aRequest->UnmapPages(aMemory->IsExecutable());

	return r;
	}


TInt DDataPagedMemoryManager::CleanPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TPhysAddr*& aPageArrayEntry)
	{
	if(aPageInfo->IsDirty()==false)
		return KErrNone;

	// shouldn't be asked to clean a page which is writable...
	__NK_ASSERT_DEBUG(aPageInfo->IsWritable()==false);

	// mark page as being modified by us...
	TUint modifierInstance; // dummy variable used only for it's storage address on the stack
	aPageInfo->SetModifier(&modifierInstance);

	// get info about page...
	TUint index = aPageInfo->Index();
	TPhysAddr physAddr = aPageInfo->PhysAddr();

	// Release the mmu lock while we write out the page.  This is safe as the 
	// RamAllocLock stops the physical address being freed from this object.
	MmuLock::Unlock();

	// get paging request object...
	DPageWriteRequest* req;
	TInt r = AcquirePageWriteRequest(req, aMemory, index, 1);
	__NK_ASSERT_DEBUG(r==KErrNone); // we should always get a write request because the previous function blocks until it gets one
	__NK_ASSERT_DEBUG(req); // we should always get a write request because the previous function blocks until it gets one

	r = WritePages(aMemory, index, 1, &physAddr, req);

	req->Release();

	MmuLock::Lock();

	if(r!=KErrNone)
		return r;

	// check if page is clean...
	if(aPageInfo->CheckModified(&modifierInstance) || aPageInfo->IsWritable())
		{
		// someone else modified the page, or it became writable, so fail...
		r = KErrInUse;
		}
	else
		{
		// page is now clean!
		ThePager.SetClean(*aPageInfo);
		}

	return r;
	}


TBool DDataPagedMemoryManager::IsAllocated(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{// MmuLock required to protect manager data.
	// DPagedMemoryManager::DoPageInDone() won't allow MmuLock to be released
	// so can only cope with a maximum of KMaxPagesInOneGo.
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aCount <= KMaxPagesInOneGo);

	return iSwapManager->IsReserved(aMemory, aIndex, aCount);
	}


void DDataPagedMemoryManager::GetSwapInfo(SVMSwapInfo& aInfoOut)
	{
	NKern::ThreadEnterCS();
	RamAllocLock::Lock();
	iSwapManager->GetSwapInfo(aInfoOut);
	RamAllocLock::Unlock();
	NKern::ThreadLeaveCS();
	}


TInt DDataPagedMemoryManager::SetSwapThresholds(const SVMSwapThresholds& aThresholds)
	{
	NKern::ThreadEnterCS();
	RamAllocLock::Lock();
	TInt r = iSwapManager->SetSwapThresholds(aThresholds);
	RamAllocLock::Unlock();
	NKern::ThreadLeaveCS();
	return r;
	}


void GetSwapInfo(SVMSwapInfo& aInfoOut)
	{
	((DDataPagedMemoryManager*)TheDataPagedMemoryManager)->GetSwapInfo(aInfoOut);
	}


TInt SetSwapThresholds(const SVMSwapThresholds& aThresholds)
	{
	return ((DDataPagedMemoryManager*)TheDataPagedMemoryManager)->SetSwapThresholds(aThresholds);
	}
  
