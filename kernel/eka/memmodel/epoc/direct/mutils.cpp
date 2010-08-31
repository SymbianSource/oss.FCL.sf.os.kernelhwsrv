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
// eka\memmodel\epoc\direct\mutils.cpp
// 
//

#include <memmodel.h>
#include "execs.h"
#include "cache_maintenance.h"
#include <kernel/cache.h>

#ifdef BTRACE_KERNEL_MEMORY
TInt   Epoc::DriverAllocdPhysRam = 0;
#endif

void RHeapK::Mutate(TInt aOffset, TInt aMaxLength)
//
// Used by the kernel to mutate a fixed heap into a chunk heap.
//
	{
	(void)aOffset;
	(void)aMaxLength;
	}

void MM::Panic(MM::TMemModelPanic aPanic)
	{
	Kern::Fault("MemModel", aPanic);
	}

TInt M::PageSizeInBytes()
	{
	return MM::RamBlockSize;
	}

EXPORT_C TUint32 Kern::RoundToPageSize(TUint32 aSize)
	{
	return MM::RoundToBlockSize(aSize);
	}

EXPORT_C TUint32 Kern::RoundToChunkSize(TUint32 aSize)
	{
	return MM::RoundToBlockSize(aSize);
	}


/**
Allows the variant/BSP to specify the details of the RAM zones.
This should to be invoked by the variant in its implementation of
the pure virtual function Asic::Init1().

There are some limitations to the how RAM zones can be specified:
- Each RAM zone's address space must be distinct and not overlap with any 
other RAM zone's address space
- Each RAM zone's address space must have a size that is multiples of the 
ASIC's MMU small page size and be aligned to the ASIC's MMU small page size, 
usually 4KB on ARM MMUs.
- When taken together all of the RAM zones must cover the whole of the physical RAM
address space as specified by the bootstrap in the SuperPage members iTotalRamSize
and iRamBootData;.
- There can be no more than KMaxRamZones RAM zones specified by the base port

Note the verification of the RAM zone data is not performed here but by the ram 
allocator later in the boot up sequence.  This is because it is only possible to
verify the zone data once the physical RAM configuration has been read from 
the super page.  Any verification errors will result in a "RAM-ALLOC" panic 
faulting the kernel during initialisation.

@param aZones Pointer to an array of SRamZone structs containing each zone's details
The end of the array is specified by an element with iSize==0.  The array must 
remain in memory at least until the kernel has successfully booted.

@param aCallback Pointer to call back function that kernel may invoke to request
one of the opeartions specified from enum TRamZoneOp is performed

@return KErrNone if successful, otherwise one of the system wide error codes
*/
EXPORT_C TInt Epoc::SetRamZoneConfig(const SRamZone* /*aZones*/, TRamZoneCallback /*aCallback*/)
	{// RAM zones not supported for this memory model
	return KErrNotSupported;
	}


/**
Gets the current count of a paricular RAM zone's free and allocated pages.

@param aId The ID of the RAM zone to enquire about
@param aPageData If successful, on return this will contain the page counts

@return KErrNone if successful, KErrArgument if a RAM zone of aId is not found or
one of the system wide error codes 
*/
EXPORT_C TInt Epoc::GetRamZonePageCount(TUint /*aId*/, SRamZonePageCount& /*aPageData*/)
	{// RAM zones not supported for this memory model
	return KErrNotSupported;
	}

/**
Modify the specified RAM zone's flags.

This allows the BSP or device driver to configure which type of pages, if any,
can be allocated into a RAM zone by the system.

Note updating a RAM zone's flags can result in
	1 - memory allocations failing despite there being enough free RAM in the system.
	2 - the methods TRamDefragRequest::EmptyRamZone(), TRamDefragRequest::ClaimRamZone()
	or TRamDefragRequest::DefragRam() never succeeding.

The flag masks KRamZoneFlagDiscardOnly, KRamZoneFlagMovAndDisOnly and KRamZoneFlagNoAlloc
are intended to be used with this method.


@param aId			The ID of the RAM zone to modify.
@param aClearFlags	The bit flags to clear, each of which must already be set on the RAM zone.
@param aSetFlags	The bit flags to set.

@return KErrNone on success, KErrArgument if the RAM zone of aId not found
or if any of aClearFlags are not already set.
*/
EXPORT_C TInt Epoc::ModifyRamZoneFlags(TUint /*aId*/, TUint /*aClearMask*/, TUint /*aSetMask*/)
	{// RAM zone not supported for this memory model
	return KErrNotSupported;
	}

/**
	@pre	Call in a thread context.
	@pre	Interrupts must be enabled.
	@pre	Kernel must be unlocked.
	@pre    No fast mutex can be held.
	@pre	Calling thread must be in a critical section.
 */
EXPORT_C TInt Epoc::AllocShadowPage(TLinAddr aRomAddr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::AllocShadowPage");
	return KErrNotSupported;
	}

/**
	@pre	Call in a thread context.
	@pre	Interrupts must be enabled.
	@pre	Kernel must be unlocked.
	@pre    No fast mutex can be held.
	@pre	Calling thread must be in a critical section.
 */
EXPORT_C TInt Epoc::FreeShadowPage(TLinAddr aRomAddr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::FreeShadowPage");
	return KErrNotSupported;
	}

/**
@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
*/
EXPORT_C TInt Epoc::CopyToShadowMemory(TLinAddr /*aDest*/, TLinAddr /*aSrc*/, TUint32 /*aLength*/)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::CopyToShadowPage");
	return KErrNotSupported;
	}

/**
	@pre	Call in a thread context.
	@pre	Interrupts must be enabled.
	@pre	Kernel must be unlocked.
	@pre    No fast mutex can be held.
	@pre	Calling thread must be in a critical section.
 */
EXPORT_C TInt Epoc::FreezeShadowPage(TLinAddr aRomAddr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::FreezeShadowPage");
	return KErrNotSupported;
	}

/**
	@pre	Call in a thread context.
	@pre	Interrupts must be enabled.
	@pre	Kernel must be unlocked.
	@pre    No fast mutex can be held.
	@pre	Calling thread must be in a critical section.
 */
EXPORT_C TInt Epoc::AllocPhysicalRam(TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::AllocPhysicalRam");
	MM::WaitRamAlloc();
	TLinAddr lin;
	TInt r=MM::AllocContiguousRegion(lin, aSize, aAlign);
	if (r!=KErrNone)
		MM::AllocFailed=ETrue;
	else
		{
		aPhysAddr = LinearToPhysical(lin);
#if defined(__CPU_HAS_CACHE) && !defined(__CPU_X86)
		CacheMaintenance::MemoryToReuse(lin, aSize);
#endif
#ifdef BTRACE_KERNEL_MEMORY
		TUint size = Kern::RoundToPageSize(aSize);
		BTrace8(BTrace::EKernelMemory, BTrace::EKernelMemoryDrvPhysAlloc, size, aPhysAddr);
		Epoc::DriverAllocdPhysRam += size;
#endif
		}
	MM::SignalRamAlloc();
	return r;
	}

/**
	@pre	Call in a thread context.
	@pre	Interrupts must be enabled.
	@pre	Kernel must be unlocked.
	@pre    No fast mutex can be held.
	@pre	Calling thread must be in a critical section.
 */
EXPORT_C TInt Epoc::FreePhysicalRam(TPhysAddr aPhysAddr, TInt aSize)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::FreePhysicalRam");
	MM::WaitRamAlloc();
#ifndef __CPU_HAS_MMU
	MM::FreeRegion(aPhysAddr, aSize);
#else
	TInt bn = MM::BlockNumber(aPhysAddr);
	TInt bn0 = MM::BlockNumber(MM::UserDataSectionBase);
	TLinAddr lin = TLinAddr((bn - bn0)<<MM::RamBlockShift) + MM::UserDataSectionBase;
	MM::FreeRegion(lin, aSize);
#endif
#ifdef BTRACE_KERNEL_MEMORY
	TUint size = Kern::RoundToPageSize(aSize);
	BTrace8(BTrace::EKernelMemory, BTrace::EKernelMemoryDrvPhysFree, aPhysAddr, size);
	Epoc::DriverAllocdPhysRam -= size;
#endif
	MM::SignalRamAlloc();
	return KErrNone;
	}

/**
Allocate a block of physically contiguous RAM with a physical address aligned
to a specified power of 2 boundary from the specified zone.
When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

Note that this method only repsects the KRamZoneFlagNoAlloc flag and will always attempt
to allocate regardless of whether the other flags are set for the specified RAM zones 
or not.

When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

@param 	aZoneId		The ID of the zone to attempt to allocate from.
@param	aSize		The size in bytes of the required block. The specified size
					is rounded up to the page size, since only whole pages of
					physical RAM can be allocated.
@param	aPhysAddr	Receives the physical address of the base of the block on
					successful allocation.
@param	aAlign		Specifies the number of least significant bits of the
					physical address which are required to be zero. If a value
					less than log2(page size) is specified, page alignment is
					assumed. Pass 0 for aAlign if there are no special alignment
					constraints (other than page alignment).
@return	KErrNone if the allocation was successful.
		KErrNoMemory if a sufficiently large physically contiguous block of free
		RAM	with the specified alignment could not be found within the specified 
		zone.
		KErrArgument if a RAM zone of the specified ID can't be found or if the
		RAM zone has a total number of physical pages which is less than those 
		requested for the allocation.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::ZoneAllocPhysicalRam(TUint aZoneId, TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::ZoneAllocPhysicalRam");
	return KErrNotSupported;
	}


/**
Allocate a block of physically contiguous RAM with a physical address aligned
to a specified power of 2 boundary from the specified RAM zones.
When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

RAM will be allocated into the RAM zones in the order they are specified in the 
aZoneId parameter. If the contiguous allocations are intended to span RAM zones 
when required then aZoneId should be listed with the RAM zones in ascending 
physical address order.

Note that this method only repsects the KRamZoneFlagNoAlloc flag and will always attempt
to allocate regardless of whether the other flags are set for the specified RAM zones 
or not.

When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

@param 	aZoneIdList	A pointer to an array of RAM zone IDs of the RAM zones to 
					attempt to allocate from.
@param 	aZoneIdCount The number of RAM zone IDs contained in aZoneIdList.
@param	aSize		The size in bytes of the required block. The specified size
					is rounded up to the page size, since only whole pages of
					physical RAM can be allocated.
@param	aPhysAddr	Receives the physical address of the base of the block on
					successful allocation.
@param	aAlign		Specifies the number of least significant bits of the
					physical address which are required to be zero. If a value
					less than log2(page size) is specified, page alignment is
					assumed. Pass 0 for aAlign if there are no special alignment
					constraints (other than page alignment).
@return	KErrNone if the allocation was successful.
		KErrNoMemory if a sufficiently large physically contiguous block of free
		RAM	with the specified alignment could not be found within the specified 
		zone.
		KErrArgument if a RAM zone of a specified ID can't be found or if the
		RAM zones have a total number of physical pages which is less than those 
		requested for the allocation.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::ZoneAllocPhysicalRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::ZoneAllocPhysicalRam");
	return KErrNotSupported;
	}


/**
Attempt to allocate discontiguous RAM pages.

When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

@param	aNumPages	The number of discontiguous pages required to be allocated
@param	aPageList	This should be a pointer to a previously allocated array of
					aNumPages TPhysAddr elements.  On a succesful allocation it 
					will receive the physical addresses of each page allocated.

@return	KErrNone if the allocation was successful.
		KErrNoMemory if the requested number of pages can't be allocated

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::AllocPhysicalRam(TInt aNumPages, TPhysAddr* aPageList)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "Epoc::AllocPhysicalRam");
	return KErrNotSupported;
	}


/**
Attempt to allocate discontiguous RAM pages from the specified zone.

Note that this method only repsects the KRamZoneFlagNoAlloc flag and will always attempt
to allocate regardless of whether the other flags are set for the specified RAM zones 
or not.

When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

@param 	aZoneId		The ID of the zone to attempt to allocate from.
@param	aNumPages	The number of discontiguous pages required to be allocated 
					from the specified zone.
@param	aPageList	This should be a pointer to a previously allocated array of
					aNumPages TPhysAddr elements.  On a succesful 
					allocation it will receive the physical addresses of each 
					page allocated.
@return	KErrNone if the allocation was successful.
		KErrNoMemory if the requested number of pages can't be allocated from the 
		specified zone.
		KErrArgument if a RAM zone of the specified ID can't be found or if the
		RAM zone has a total number of physical pages which is less than those 
		requested for the allocation.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::ZoneAllocPhysicalRam(TUint aZoneId, TInt aNumPages, TPhysAddr* aPageList)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "Epoc::ZoneAllocPhysicalRam");
	return KErrNotSupported;
	}


/**
Attempt to allocate discontiguous RAM pages from the specified RAM zones.
The RAM pages will be allocated into the RAM zones in the order that they are specified 
in the aZoneId parameter, the RAM zone preferences will be ignored.

Note that this method only repsects the KRamZoneFlagNoAlloc flag and will always attempt
to allocate regardless of whether the other flags are set for the specified RAM zones 
or not.

When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

@param 	aZoneIdList	A pointer to an array of RAM zone IDs of the RAM zones to 
					attempt to allocate from.
@param	aZoneIdCount The number of RAM zone IDs pointed to by aZoneIdList.
@param	aNumPages	The number of discontiguous pages required to be allocated 
					from the specified zone.
@param	aPageList	This should be a pointer to a previously allocated array of
					aNumPages TPhysAddr elements.  On a succesful 
					allocation it will receive the physical addresses of each 
					page allocated.
@return	KErrNone if the allocation was successful.
		KErrNoMemory if the requested number of pages can't be allocated from the 
		specified zone.
		KErrArgument if a RAM zone of a specified ID can't be found or if the
		RAM zones have a total number of physical pages which is less than those 
		requested for the allocation.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::ZoneAllocPhysicalRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aNumPages, TPhysAddr* aPageList)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "Epoc::ZoneAllocPhysicalRam");
	return KErrNotSupported;
	}


/**
Free a number of physical RAM pages that were previously allocated using
Epoc::AllocPhysicalRam().

@param	aNumPages	The number of pages to be freed.
@param	aPhysAddr	An array of aNumPages TPhysAddr elements.  Where each element
					should contain the physical address of each page to be freed.
					This must be the same set of addresses as those returned by a 
					previous call to Epoc::AllocPhysicalRam() or 
					Epoc::ZoneAllocPhysicalRam().
@return	KErrNone if the operation was successful.
		KErrArgument if one or more of the physical addresses specified is not 
					a valid physical RAM address.
		KErrGeneral if the physical addresses specified are all valid
					physical RAM addresses but some of them had not
					been previously allocated.
@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::FreePhysicalRam(TInt aNumPages, TPhysAddr* aPageList)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::FreePhysicalRam");
	return KErrNotSupported;
	}


/**
Free a RAM zone which was previously allocated by one of these methods:
Epoc::AllocPhysicalRam(), Epoc::ZoneAllocPhysicalRam() or 
TRamDefragRequest::ClaimRamZone().

All of the pages in the RAM zone must be allocated and only via one of the methods 
listed above, otherwise a system panic will occur.

@param	aZoneId			The ID of the RAM zone to free.
@return	KErrNone 		If the operation was successful.
		KErrArgument 	If a RAM zone with ID aZoneId was not found.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::FreeRamZone(TUint aZoneId)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::FreeRamZone");
	return KErrNotSupported;
	}


/**
	@pre	Call in a thread context.
	@pre	Interrupts must be enabled.
	@pre	Kernel must be unlocked.
	@pre    No fast mutex can be held.
	@pre	Calling thread must be in a critical section.
 */
EXPORT_C TInt Epoc::ClaimPhysicalRam(TPhysAddr aPhysAddr, TInt aSize)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::ClaimPhysicalRam");
	MM::WaitRamAlloc();
#ifndef __CPU_HAS_MMU
	TInt r=MM::ClaimRegion(aPhysAddr, aSize);
#else
	TInt bn = MM::BlockNumber(aPhysAddr);
	TInt bn0 = MM::BlockNumber(MM::UserDataSectionBase);
	TLinAddr lin = TLinAddr((bn - bn0)<<MM::RamBlockShift) + MM::UserDataSectionBase;
	TInt r=MM::ClaimRegion(lin, aSize);
#endif
	MM::SignalRamAlloc();
	return r;
	}

void ExecHandler::UnlockRamDrive()
	{
	}

EXPORT_C void TInternalRamDrive::Unlock()
	{
	}

EXPORT_C void TInternalRamDrive::Lock()
	{
	}

void MM::WaitRamAlloc()
	{
	Kern::MutexWait(*RamAllocatorMutex);
	if (RamAllocatorMutex->iHoldCount==1)
		{
		MM::InitialFreeMemory=Kern::FreeRamInBytes();
		MM::AllocFailed=EFalse;
		}
	}

void MM::SignalRamAlloc()
	{
	if (RamAllocatorMutex->iHoldCount>1)
		{
		Kern::MutexSignal(*RamAllocatorMutex);
		return;
		}
	TInt initial=MM::InitialFreeMemory;
	TBool failed=MM::AllocFailed;
	TInt final=Kern::FreeRamInBytes();
	Kern::MutexSignal(*RamAllocatorMutex);
	K::CheckFreeMemoryLevel(initial,final,failed);
	}

EXPORT_C TInt TInternalRamDrive::MaxSize()
	{
	return PP::RamDriveMaxSize;
	}

void M::FsRegisterThread()
	{
	}

void M::BTracePrime(TUint aCategory)
	{
	(void)aCategory;
#ifdef BTRACE_KERNEL_MEMORY	
	// Must check for -1 as that is the default value of aCategory for
	// BTrace::Prime() which is intended to prime all categories that are 
	// currently enabled via a single invocation of BTrace::Prime().
	if(aCategory==BTrace::EKernelMemory || (TInt)aCategory == -1)
		{
		BTrace4(BTrace::EKernelMemory,BTrace::EKernelMemoryInitialFree,TheSuperPage().iTotalRamSize);
		BTrace4(BTrace::EKernelMemory,BTrace::EKernelMemoryCurrentFree,Kern::FreeRamInBytes());
		BTrace8(BTrace::EKernelMemory,BTrace::EKernelMemoryDrvPhysAlloc, Epoc::DriverAllocdPhysRam, -1);
		}
#endif
	}

EXPORT_C DDemandPagingLock::DDemandPagingLock()
	: iLockedPageCount(0)
	{
	}

EXPORT_C TInt DDemandPagingLock::Alloc(TInt /*aSize*/)
	{
	return KErrNone;
	}

EXPORT_C TBool DDemandPagingLock::Lock(DThread* /*aThread*/, TLinAddr /*aStart*/, TInt /*aSize*/)
	{
	return EFalse;
	}

EXPORT_C void DDemandPagingLock::DoUnlock()
	{
	}

EXPORT_C void DDemandPagingLock::Free()
	{
	}

EXPORT_C TInt Kern::InstallPagingDevice(DPagingDevice* aDevice)
	{
	return KErrNotSupported;
	}

// Dummy implementation of kernel pin APIs

class TVirtualPinObject
	{	
	};

TInt M::CreateVirtualPinObject(TVirtualPinObject*& aPinObject)
	{
	aPinObject = new TVirtualPinObject;
	return aPinObject != NULL ? KErrNone : KErrNoMemory;
	}

TInt M::PinVirtualMemory(TVirtualPinObject* aPinObject, TLinAddr, TUint, DThread*)
	{
	__ASSERT_DEBUG(aPinObject, K::Fault(K::EVirtualPinObjectBad));
	(void)aPinObject;
	return KErrNone;
	}

TInt M::CreateAndPinVirtualMemory(TVirtualPinObject*& aPinObject, TLinAddr, TUint)
	{
	aPinObject = 0;
	return KErrNone;
	}

void M::UnpinVirtualMemory(TVirtualPinObject* aPinObject)
	{
	__ASSERT_DEBUG(aPinObject, K::Fault(K::EVirtualPinObjectBad));
	(void)aPinObject;
	}

void M::DestroyVirtualPinObject(TVirtualPinObject*& aPinObject)
	{
	TVirtualPinObject* object = (TVirtualPinObject*)__e32_atomic_swp_ord_ptr(&aPinObject, 0);
	if (object)
		Kern::AsyncFree(object);
	}

TInt M::CreatePhysicalPinObject(TPhysicalPinObject*& aPinObject)
	{
	return KErrNotSupported;
	}

TInt M::PinPhysicalMemory(TPhysicalPinObject*, TLinAddr, TUint, TBool, TPhysAddr& , TPhysAddr*, TUint32&, TUint&, DThread*)
	{
	K::Fault(K::EPhysicalPinObjectBad);
	return KErrNone;
	}

void M::UnpinPhysicalMemory(TPhysicalPinObject* aPinObject)
	{
	K::Fault(K::EPhysicalPinObjectBad);
	}

void M::DestroyPhysicalPinObject(TPhysicalPinObject*& aPinObject)
	{
	K::Fault(K::EPhysicalPinObjectBad);
	}


//
// Kernel map and pin (Not supported on the direct memory models).
//

TInt M::CreateKernelMapObject(TKernelMapObject*&, TUint)
	{
	return KErrNotSupported;
	}


TInt M::MapAndPinMemory(TKernelMapObject*, DThread*, TLinAddr, TUint, TUint, TLinAddr&, TPhysAddr*)
	{
	return KErrNotSupported;
	}


void M::UnmapAndUnpinMemory(TKernelMapObject*)
	{
	}


void M::DestroyKernelMapObject(TKernelMapObject*&)
	{
	}


// Misc DPagingDevice methods

EXPORT_C NFastMutex* DPagingDevice::NotificationLock()
	{
	// use the system lock
	return &TheScheduler.iLock;
	}

EXPORT_C void DPagingDevice::NotifyIdle()
	{
	// Not used on this memory model
	}

EXPORT_C void DPagingDevice::NotifyBusy()
	{
	// Not used on this memory model
	}

EXPORT_C TInt Cache::SyncPhysicalMemoryBeforeDmaWrite(TPhysAddr* , TUint , TUint , TUint , TUint32 )
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncPhysicalMemoryBeforeDmaWrite");
	return KErrNotSupported;
	}

EXPORT_C TInt Cache::SyncPhysicalMemoryBeforeDmaRead(TPhysAddr* , TUint , TUint , TUint , TUint32 )
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncPhysicalMemoryBeforeDmaRead");
	return KErrNotSupported;
	}
EXPORT_C TInt Cache::SyncPhysicalMemoryAfterDmaRead(TPhysAddr* , TUint , TUint , TUint , TUint32 )
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncPhysicalMemoryAfterDmaRead");
	return KErrNotSupported;
	}
