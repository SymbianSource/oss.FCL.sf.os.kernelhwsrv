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

	/// The state of swap for a logical page in a memory object.
	///
	/// Note that this does not always correspond to the state of the page in RAM - for example a
	/// page can be dirty in RAM but blank in swap if it has never been written out.
	enum TSwapState
		{
		EStateUnreserved = 0,	///< swap space not yet reserved, or page is being decommitted
		EStateBlank      = 1,	///< swap page has never been written
		EStateWritten    = 2,	///< swap page has been written out at least once
		EStateWriting    = 3	///< swap page is in the process of being written out
		};
	
	enum
		{
		ESwapIndexShift = 2,
		ESwapStateMask 	= (1 << ESwapIndexShift) - 1,
		ESwapIndexMask  = 0xffffffff & ~ESwapStateMask
		};

public:
	TInt Create(DPagingDevice* aDevice);

	TInt ReserveSwap(DMemoryObject* aMemory, TUint aStartIndex, TUint aPageCount);
	TInt UnreserveSwap(DMemoryObject* aMemory, TUint aStartIndex, TUint aPageCount);
	TBool IsReserved(DMemoryObject* aMemory, TUint aStartIndex, TUint aPageCount);

	TInt ReadSwapPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TLinAddr aLinAddr, DPageReadRequest* aRequest, TPhysAddr* aPhysAddrs);
	TInt WriteSwapPages(DMemoryObject** aMemory, TUint* aIndex, TUint aCount, TLinAddr aLinAddr, TBool aBackground);

	void GetSwapInfo(SVMSwapInfo& aInfoOut);
	TInt SetSwapThresholds(const SVMSwapThresholds& aThresholds);

private:
	inline TSwapState SwapState(TUint aSwapData);
	inline TInt SwapIndex(TUint aSwapData);
	inline TUint SwapData(TSwapState aSwapState, TInt aSwapIndex);
	
	TInt AllocSwapIndex(TInt aCount);
	void FreeSwapIndex(TInt aSwapIndex);
	void CheckSwapThresholdsAndUnlock(TUint aInitial);
	
	void DoDeleteNotify(TUint aSwapIndex);
	TInt DoWriteSwapPages(DMemoryObject** aMemory, TUint* aIndex, TUint aCount, TLinAddr aLinAddr, TInt aSwapIndex, TBool aBackground);
	
private:
	DPagingDevice* iDevice;			///< Paging device used to read and write swap pages
	
	NFastMutex iSwapLock;			///< Fast mutex protecting access to all members below
	TUint iFreePageCount;			///< Number of swap pages that have not been reserved
	TBitMapAllocator* iBitMap;		///< Bitmap of swap pages that have been allocated
	TUint iAllocOffset;				///< Next offset to try when allocating a swap page
 	TUint iSwapThesholdLow;
 	TUint iSwapThesholdGood;
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
	virtual void CleanPages(TUint aPageCount, SPageInfo** aPageInfos, TBool aBackground);

	// Methods inherited from DPagedMemoryManager
	virtual void Init3();
	virtual TInt InstallPagingDevice(DPagingDevice* aDevice);
	virtual TInt AcquirePageReadRequest(DPageReadRequest*& aRequest, DMemoryObject* aMemory, TUint aIndex, TUint aCount);
	virtual TInt AcquirePageWriteRequest(DPageWriteRequest*& aRequest, DMemoryObject** aMemory, TUint* aIndex, TUint aCount);
	virtual TInt ReadPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr* aPages, DPageReadRequest* aRequest);
	virtual TBool IsAllocated(DMemoryObject* aMemory, TUint aIndex, TUint aCount);

public:
	void GetSwapInfo(SVMSwapInfo& aInfoOut);
	TInt SetSwapThresholds(const SVMSwapThresholds& aThresholds);

private:
	TInt WritePages(DMemoryObject** aMemory, TUint* aIndex, TPhysAddr* aPages, TUint aCount, DPageWriteRequest *aRequest, TBool aAnyExecutable, TBool aBackground);

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
	__ASSERT_COMPILE(!(ESwapIndexMask & ESwapStateMask));
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
	iFreePageCount = swapPages;
	iAllocOffset = 0;
	return KErrNone;
	}


inline DSwapManager::TSwapState DSwapManager::SwapState(TUint aSwapData)
	{
	TSwapState state = (TSwapState)(aSwapData & ESwapStateMask);
	__NK_ASSERT_DEBUG(state >= EStateWritten || (aSwapData & ~ESwapStateMask) == 0);
	return state;
	}


inline TInt DSwapManager::SwapIndex(TUint aSwapData)
	{
	return aSwapData >> ESwapIndexShift;
	}


inline TUint DSwapManager::SwapData(TSwapState aSwapState, TInt aSwapIndex)
	{
	return (aSwapIndex << ESwapIndexShift) | aSwapState;
	}


/**
Allocate one or more page's worth of space within the swap area.

The location is represented by a page-based index into the swap area.

@param aCount The number of page's worth of space to allocate.

@return The swap index of the first location allocated.
*/
TInt DSwapManager::AllocSwapIndex(TInt aCount)
	{
	__NK_ASSERT_DEBUG(aCount > 0 && aCount <= KMaxPagesToClean);
	NKern::FMWait(&iSwapLock);

	// search for run of aCount from iAllocOffset to end
	TInt carry = 0;
	TInt l = KMaxTInt;
	TInt swapIndex = iBitMap->AllocAligned(aCount, 0, 0, EFalse, carry, l, iAllocOffset);

	// if search failed, retry from beginning
	if (swapIndex < 0)
		{
		iAllocOffset = 0;
		carry = 0;
		swapIndex = iBitMap->AllocAligned(aCount, 0, 0, EFalse, carry, l, iAllocOffset);
		}

	// if we found one then mark it as allocated and update iAllocOffset
	if (swapIndex >= 0)
		{
		__NK_ASSERT_DEBUG(swapIndex <= (iBitMap->iSize - aCount));
		iBitMap->Alloc(swapIndex, aCount);
		iAllocOffset = (swapIndex + aCount) % iBitMap->iSize;
		}
	
	NKern::FMSignal(&iSwapLock);
	__NK_ASSERT_DEBUG(swapIndex >= 0 || aCount > 1); // can't fail to allocate single page
	return swapIndex;
	}


/**
Free one page's worth of space within the swap area.

The index must have been previously allocated with AllocSwapIndex().
*/
void DSwapManager::FreeSwapIndex(TInt aSwapIndex)
	{
	__NK_ASSERT_DEBUG(aSwapIndex >= 0 && aSwapIndex < iBitMap->iSize);
	DoDeleteNotify(aSwapIndex);
	NKern::FMWait(&iSwapLock);
	iBitMap->Free(aSwapIndex);
	NKern::FMSignal(&iSwapLock);
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

	NKern::FMWait(&iSwapLock);
	TUint initFree = iFreePageCount;
	if (iFreePageCount < aPageCount)
		{
		NKern::FMSignal(&iSwapLock);
		Kern::AsyncNotifyChanges(EChangesOutOfMemory);
		return KErrNoMemory;
		}
	iFreePageCount -= aPageCount;
	CheckSwapThresholdsAndUnlock(initFree);		
	
	// Mark each page as allocated and uninitialised.
	const TUint indexEnd = aStartIndex + aPageCount;
	for (TUint index = aStartIndex; index < indexEnd; index++)
		{		
		// Grab MmuLock to stop manager data being accessed.
		MmuLock::Lock();
		__NK_ASSERT_DEBUG(SwapState(aMemory->PagingManagerData(index)) == EStateUnreserved);
		aMemory->SetPagingManagerData(index, EStateBlank);
		MmuLock::Unlock();
		}

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

	TUint freedPages = 0;
	const TUint indexEnd = aStartIndex + aPageCount;
	for (TUint index = aStartIndex; index < indexEnd; index++)
		{
		// Grab MmuLock to stop manager data being accessed.
		MmuLock::Lock();
		TUint swapData = aMemory->PagingManagerData(index);
		TSwapState state = SwapState(swapData);
		if (state != EStateUnreserved)
			{
			freedPages++;
			aMemory->SetPagingManagerData(index, EStateUnreserved);
			}
		MmuLock::Unlock();

		if (state == EStateWritten)
			FreeSwapIndex(SwapIndex(swapData));
		else if (state == EStateWriting)
			{
			// Wait for cleaning to finish before deallocating swap space
			PageCleaningLock::Lock();
			PageCleaningLock::Unlock();
			
#ifdef _DEBUG
			MmuLock::Lock();
			__NK_ASSERT_DEBUG(SwapState(aMemory->PagingManagerData(index)) == EStateUnreserved);
			MmuLock::Unlock();
#endif
			}
		}
	
	NKern::FMWait(&iSwapLock);
	TUint initFree = iFreePageCount;
	iFreePageCount += freedPages;
	CheckSwapThresholdsAndUnlock(initFree);	
	
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
		if (SwapState(aMemory->PagingManagerData(index)) == EStateUnreserved)
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
	__ASSERT_CRITICAL;
	
	TInt r = KErrNone;
	const TUint readUnitShift = iDevice->iReadUnitShift;
	TUint readSize = KPageSize >> readUnitShift;
	TThreadMessage message;

	const TUint indexEnd = aIndex + aCount;
	for (TUint index = aIndex; index < indexEnd; index++, aLinAddr += KPageSize, aPhysAddrs++)
		{
		START_PAGING_BENCHMARK;

		MmuLock::Lock();	// MmuLock required for atomic access to manager data.
		TUint swapData = aMemory->PagingManagerData(index);
		TSwapState state = SwapState(swapData);

		if (state == EStateUnreserved)
			{// This page is not committed to the memory object
			MmuLock::Unlock();
			return KErrNotFound;			
			}
		else if (state == EStateBlank)
			{// This page has not been written to yet so don't read from swap 
			// just wipe it if required.
			TUint allocFlags = aMemory->RamAllocFlags();
			MmuLock::Unlock();
			TBool wipePages = !(allocFlags & Mmu::EAllocNoWipe);
			if (wipePages)
				{
				TUint8 wipeByte = (allocFlags & Mmu::EAllocUseCustomWipeByte) ?
					(allocFlags >> Mmu::EAllocWipeByteShift) & 0xff :
					0x03;
				memset((TAny*)aLinAddr, wipeByte, KPageSize);
				}
			}
		else
			{
			// It is not possible to get here if the page is in state EStateWriting as if so it must
			// be present in RAM, and so will not need to be read in.
			__NK_ASSERT_DEBUG(state == EStateWritten);
			
			// OK to release as if the object's data is decommitted the pager 
			// will check that data is still valid before mapping it.
			MmuLock::Unlock();
			TUint readStart = (SwapIndex(swapData) << KPageShift) >> readUnitShift;
			START_PAGING_BENCHMARK;
			r = iDevice->Read(&message, aLinAddr, readStart, readSize, DPagingDevice::EDriveDataPaging);
			if (r != KErrNone)
				__KTRACE_OPT(KPANIC, Kern::Printf("DSwapManager::ReadSwapPages: error reading media at %08x + %x: %d", readStart << readUnitShift, readSize << readUnitShift, r));				
			__NK_ASSERT_DEBUG(r!=KErrNoMemory); // not allowed to allocate memory, therefore can't fail with KErrNoMemory
			END_PAGING_BENCHMARK(EPagingBmReadDataMedia);
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
@param  aBackground Whether this is being called in the background by the page cleaning thread
                    as opposed to on demand when a free page is required.

@pre Called with page cleaning lock held
*/
TInt DSwapManager::WriteSwapPages(DMemoryObject** aMemory, TUint* aIndex, TUint aCount, TLinAddr aLinAddr, TBool aBackground)
	{
	__ASSERT_CRITICAL;  // so we can pass the paging device a stack-allocated TThreadMessage
	__NK_ASSERT_DEBUG(PageCleaningLock::IsHeld());

	START_PAGING_BENCHMARK;
	
	TUint i;
	TUint swapData[KMaxPagesToClean + 1];
	 
	MmuLock::Lock();
	for (i = 0 ; i < aCount ; ++i)
		{
		swapData[i] = aMemory[i]->PagingManagerData(aIndex[i]);
		TSwapState s = SwapState(swapData[i]);
		// It's not possible to write a page while it's already being written, because we always hold
		// the PageCleaning mutex when we clean
		__NK_ASSERT_DEBUG(s == EStateUnreserved || s == EStateBlank || s == EStateWritten);
		if (s == EStateBlank || s == EStateWritten)
			aMemory[i]->SetPagingManagerData(aIndex[i], SwapData(EStateWriting, 0));
		}
	MmuLock::Unlock();

	// By the time we get here, some pages may have been decommitted, so write out only those runs
	// of pages which are still committed.

	TInt r = KErrNone;
	TInt startIndex = -1;
	swapData[aCount] = SwapData(EStateUnreserved, 0); // end of list marker
	for (i = 0 ; i < (aCount + 1) ; ++i)
		{
		if (SwapState(swapData[i]) != EStateUnreserved)
			{
			if (startIndex == -1)
				startIndex = i;

			// Free swap page corresponding to old version of the pages we are going to write
			if (SwapState(swapData[i]) == EStateWritten)
				FreeSwapIndex(SwapIndex(swapData[i]));
			}
		else
			{
			if (startIndex != -1)
				{
				// write pages from startIndex to i exclusive
				TInt count = i - startIndex;
				__NK_ASSERT_DEBUG(count > 0 && count <= KMaxPagesToClean);

				// Get a new swap location for these pages, writing them all together if possible
				TInt swapIndex = AllocSwapIndex(count);
				if (swapIndex >= 0)
					r = DoWriteSwapPages(&aMemory[startIndex], &aIndex[startIndex], count, aLinAddr + (startIndex << KPageShift), swapIndex, aBackground);
				else
					{
					// Otherwise, write them individually
					for (TUint j = startIndex ; j < i ; ++j)
						{
						swapIndex = AllocSwapIndex(1);
						__NK_ASSERT_DEBUG(swapIndex >= 0);
						r = DoWriteSwapPages(&aMemory[j], &aIndex[j], 1, aLinAddr + (j << KPageShift), swapIndex, aBackground);
						if (r != KErrNone)
							break;
						}
					}

				startIndex = -1;
				}
			}
		}
	
	END_PAGING_BENCHMARK_N(EPagingBmWriteDataPage, aCount);
	
	return r;
	}

TInt DSwapManager::DoWriteSwapPages(DMemoryObject** aMemory, TUint* aIndex, TUint aCount, TLinAddr aLinAddr, TInt aSwapIndex, TBool aBackground)
	{	
		
	const TUint readUnitShift = iDevice->iReadUnitShift;
	const TUint writeSize = aCount << (KPageShift - readUnitShift);
	const TUint writeOffset = aSwapIndex << (KPageShift - readUnitShift);
		
	TThreadMessage msg;
	START_PAGING_BENCHMARK;
	TInt r = iDevice->Write(&msg, aLinAddr, writeOffset, writeSize, aBackground);
	if (r != KErrNone)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("DSwapManager::WriteSwapPages: error writing media from %08x to %08x + %x: %d", aLinAddr, writeOffset << readUnitShift, writeSize << readUnitShift, r));
		}
	__NK_ASSERT_DEBUG(r!=KErrNoMemory); // not allowed to allocate memory, therefore can't fail with KErrNoMemory
	__NK_ASSERT_ALWAYS(r == KErrNone);
	END_PAGING_BENCHMARK(EPagingBmWriteDataMedia);

	TUint i;
	TUint swapData[KMaxPagesToClean];
	
	MmuLock::Lock();
	for (i = 0 ; i < aCount ; ++i)
		{
		// Re-check the swap state in case page was decommitted while we were writing
		swapData[i] = aMemory[i]->PagingManagerData(aIndex[i]);
		TSwapState s = SwapState(swapData[i]);
		__NK_ASSERT_DEBUG(s == EStateUnreserved || s == EStateWriting);
		if (s == EStateWriting)
			{
			// Store the new swap location and mark the page as saved.
			aMemory[i]->SetPagingManagerData(aIndex[i], SwapData(EStateWritten, aSwapIndex + i));
			}
		}
	MmuLock::Unlock();

	for (i = 0 ; i < aCount ; ++i)
		{
		TSwapState s = SwapState(swapData[i]);
		if (s == EStateUnreserved)
			{
			// The page was decommitted while we were cleaning it, so free the swap page we
			// allocated and continue, leaving this page in the unreserved state.
			FreeSwapIndex(aSwapIndex + i);
			}
		}

	return KErrNone;
	}
	

/**
Notify the media driver that the page written to swap is no longer required.
*/
void DSwapManager::DoDeleteNotify(TUint aSwapIndex)
	{
	__ASSERT_CRITICAL;  // so we can pass the paging device a stack-allocated TThreadMessage
#ifdef __PAGING_DELETE_NOTIFY_ENABLED
	const TUint readUnitShift = iDevice->iReadUnitShift;
	const TUint size = KPageSize >> readUnitShift;
	TUint offset = (aSwapIndex << KPageShift) >> readUnitShift;
	TThreadMessage msg;

	START_PAGING_BENCHMARK;
	// Ignore the return value as this is just an optimisation that is not supported on all media.
	(void)iDevice->DeleteNotify(&msg, offset, size);
	END_PAGING_BENCHMARK(EPagingBmDeleteNotifyDataPage);
#endif
	}


// Check swap thresholds and notify (see K::CheckFreeMemoryLevel)
void DSwapManager::CheckSwapThresholdsAndUnlock(TUint aInitial)
	{
	TUint changes = 0;
	if (iFreePageCount < iSwapThesholdLow && aInitial >= iSwapThesholdLow)
		changes |= (EChangesFreeMemory | EChangesLowMemory);
	if (iFreePageCount >= iSwapThesholdGood && aInitial < iSwapThesholdGood)
		changes |= EChangesFreeMemory;
	NKern::FMSignal(&iSwapLock);
	if (changes)
		Kern::AsyncNotifyChanges(changes);
	}


void DSwapManager::GetSwapInfo(SVMSwapInfo& aInfoOut)
	{
	aInfoOut.iSwapSize = iBitMap->iSize << KPageShift;
	NKern::FMWait(&iSwapLock);
	aInfoOut.iSwapFree = iFreePageCount << KPageShift;
	NKern::FMSignal(&iSwapLock);
	}


TInt DSwapManager::SetSwapThresholds(const SVMSwapThresholds& aThresholds)
	{
	if (aThresholds.iLowThreshold > aThresholds.iGoodThreshold)
		return KErrArgument;		
	TInt low = (aThresholds.iLowThreshold + KPageSize - 1) >> KPageShift;
	TInt good = (aThresholds.iGoodThreshold + KPageSize - 1) >> KPageShift;
	if (good > iBitMap->iSize)
		return KErrArgument;
	NKern::FMWait(&iSwapLock);
	iSwapThesholdLow = low;
	iSwapThesholdGood = good;
	NKern::FMSignal(&iSwapLock);
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
	// unless the device is a media extension device
	if(aDevice->iType & DPagingDevice::EMediaExtension)
		{
		delete iSwapManager;
		iSwapManager = NULL;
		TAny* null = 0;
		__e32_atomic_store_ord_ptr(&iDevice, null);
		}

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


TInt DDataPagedMemoryManager::AcquirePageWriteRequest(DPageWriteRequest*& aRequest, DMemoryObject** aMemory, TUint* aIndex, TUint aCount)
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
	return iSwapManager->ReserveSwap(aMemory, aIndex, aCount);
	}


void DDataPagedMemoryManager::Free(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	TRACE2(("DDataPagedMemoryManager::Free(0x%08x,0x%x,0x%x)", aMemory, aIndex, aCount));
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	// Unreserve the swap pages associated with the memory object.  Do this before
	// removing the page array entries to prevent a page fault reallocating these pages.
	TInt freed = iSwapManager->UnreserveSwap(aMemory, aIndex, aCount);
	(void)freed;

	DoFree(aMemory,aIndex,aCount);
	}


/**
@copydoc DMemoryManager::Wipe
*/
TInt DDataPagedMemoryManager::Wipe(DMemoryObject* aMemory)
	{
	// This is not implemented
	//
	// It's possible to implement this by throwing away all pages that are paged in and just setting
	// the backing store state to EStateBlank, however there are currently no use cases which
	// involve calling Wipe on paged memory.
	
	__NK_ASSERT_ALWAYS(0);

	return KErrNotSupported;
	}


TInt DDataPagedMemoryManager::ReadPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr* aPages, DPageReadRequest* aRequest)
	{
	__NK_ASSERT_DEBUG(aRequest->CheckUseContiguous(aMemory,aIndex,aCount));

	// Map pages temporarily so that we can copy into them.
	const TLinAddr linAddr = aRequest->MapPages(aIndex, aCount, aPages);

	TInt r = iSwapManager->ReadSwapPages(aMemory, aIndex, aCount, linAddr, aRequest, aPages);

	// The memory object allows executable mappings then need IMB.
	aRequest->UnmapPages(aMemory->IsExecutable());

	return r;
	}


TInt DDataPagedMemoryManager::WritePages(DMemoryObject** aMemory, TUint* aIndex, TPhysAddr* aPages, TUint aCount, DPageWriteRequest* aRequest, TBool aAnyExecutable, TBool aBackground)
	{
	// Map pages temporarily so that we can copy into them.
	const TLinAddr linAddr = aRequest->MapPages(aIndex[0], aCount, aPages);

	TInt r = iSwapManager->WriteSwapPages(aMemory, aIndex, aCount, linAddr, aBackground);

	// The memory object allows executable mappings then need IMB.
	aRequest->UnmapPages(aAnyExecutable);

	return r;
	}


void DDataPagedMemoryManager::CleanPages(TUint aPageCount, SPageInfo** aPageInfos, TBool aBackground)
	{
	__NK_ASSERT_DEBUG(PageCleaningLock::IsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aPageCount <= (TUint)KMaxPagesToClean);
	
	TUint i;
	DMemoryObject* memory[KMaxPagesToClean];
	TUint index[KMaxPagesToClean];
	TPhysAddr physAddr[KMaxPagesToClean];
	TBool anyExecutable = EFalse;
	
	for (i = 0 ; i < aPageCount ; ++i)
		{
		SPageInfo* pi = aPageInfos[i];

		__NK_ASSERT_DEBUG(!pi->IsWritable());
		__NK_ASSERT_DEBUG(pi->IsDirty());
		
		// mark page as being modified by us...
		pi->SetModifier(&memory[0]);
		
		// get info about page...
		memory[i] = pi->Owner();
		index[i] = pi->Index();
		physAddr[i] = pi->PhysAddr();
		anyExecutable = anyExecutable || memory[i]->IsExecutable();
		}

	MmuLock::Unlock();

	// get paging request object...
	DPageWriteRequest* req;
	TInt r = AcquirePageWriteRequest(req, memory, index, aPageCount);
	__NK_ASSERT_DEBUG(r==KErrNone && req);
	
	r = WritePages(memory, index, physAddr, aPageCount, req, anyExecutable, aBackground);
	__NK_ASSERT_DEBUG(r == KErrNone);  // this should never return an error

	req->Release();

	MmuLock::Lock();

	for (i = 0 ; i < aPageCount ; ++i)
		{
		SPageInfo* pi = aPageInfos[i];
		// check if page is clean...
		if(pi->CheckModified(&memory[0]) || pi->IsWritable())
			{
			// someone else modified the page, or it became writable, so mark as not cleaned
			aPageInfos[i] = NULL;
			}
		else
			{
			// page is now clean!
			ThePager.SetClean(*pi);
			}
		}
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
	iSwapManager->GetSwapInfo(aInfoOut);
	}


TInt DDataPagedMemoryManager::SetSwapThresholds(const SVMSwapThresholds& aThresholds)
	{
	return iSwapManager->SetSwapThresholds(aThresholds);
	}


void GetSwapInfo(SVMSwapInfo& aInfoOut)
	{
	((DDataPagedMemoryManager*)TheDataPagedMemoryManager)->GetSwapInfo(aInfoOut);
	}


TInt SetSwapThresholds(const SVMSwapThresholds& aThresholds)
	{
	return ((DDataPagedMemoryManager*)TheDataPagedMemoryManager)->SetSwapThresholds(aThresholds);
	}
  
