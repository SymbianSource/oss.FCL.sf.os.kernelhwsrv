// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "memmodel.h"
#include "mm.h"
#include "mmu.h"
#include "mpager.h"
#include "mrom.h"

/**	Returns the amount of free RAM currently available.

@return The number of bytes of free RAM currently available.
@pre	any context
 */
EXPORT_C TInt Kern::FreeRamInBytes()
	{
	TUint numPages = TheMmu.FreeRamInPages();
	// hack, clip free RAM to fit into a signed integer...
	if(numPages>(KMaxTInt>>KPageShift))
		return KMaxTInt;
	return numPages*KPageSize;
	}


/**	Rounds up the argument to the size of a MMU page.

	To find out the size of a MMU page:
	@code
	size = Kern::RoundToPageSize(1);
	@endcode

	@param aSize Value to round up
	@pre any context
 */
EXPORT_C TUint32 Kern::RoundToPageSize(TUint32 aSize)
	{
	return (aSize+KPageMask)&~KPageMask;
	}


/**	Rounds up the argument to the amount of memory mapped by a MMU page 
	directory entry.

	Chunks occupy one or more consecutive page directory entries (PDE) and
	therefore the amount of linear and physical memory allocated to a chunk is
	always a multiple of the amount of memory mapped by a page directory entry.
 */
EXPORT_C TUint32 Kern::RoundToChunkSize(TUint32 aSize)
	{
	return (aSize+KChunkMask)&~KChunkMask;
	}


//
// Epoc class
// 
#ifdef BTRACE_KERNEL_MEMORY
TInt   Epoc::DriverAllocdPhysRam = 0;
TInt   Epoc::KernelMiscPages = 0;
#endif


/**
Allows the variant to specify the details of the RAM zones. This should be invoked 
by the variant in its implementation of the pure virtual function Asic::Init1().

There are some limitations to how the RAM zones can be specified:
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
the super page. Any verification errors result in a "RAM-ALLOC" panic 
faulting the kernel during initialisation.

@param aZones Pointer to an array of SRamZone structs containing the details for all 
the zones. The end of the array is specified by an element with an iSize of zero. The array must 
remain in memory at least until the kernel has successfully booted.

@param aCallback Pointer to a call back function that the kernel may invoke to request 
one of the operations specified by TRamZoneOp.

@return KErrNone if successful, otherwise one of the system wide error codes

@see TRamZoneOp
@see SRamZone
@see TRamZoneCallback
*/
EXPORT_C TInt Epoc::SetRamZoneConfig(const SRamZone* aZones, TRamZoneCallback aCallback)
	{
	TRamZoneCallback dummy;
	// Ensure this is only called once and only while we are initialising the kernel
	if (!K::Initialising || TheMmu.RamZoneConfig(dummy) != NULL)
		{// fault kernel, won't return
		K::Fault(K::EBadSetRamZoneConfig);
		}

	if (NULL == aZones)
		{
		return KErrArgument;
		}
	TheMmu.SetRamZoneConfig(aZones, aCallback);
	return KErrNone;
	}


/**
Modify the specified RAM zone's flags.

This allows the BSP or device driver to configure which type of pages, if any,
can be allocated into a RAM zone by the system.

Note: updating a RAM zone's flags can result in
	1 - memory allocations failing despite there being enough free RAM in the system.
	2 - the methods TRamDefragRequest::EmptyRamZone(), TRamDefragRequest::ClaimRamZone()
	or TRamDefragRequest::DefragRam() never succeeding.

The flag masks KRamZoneFlagDiscardOnly, KRamZoneFlagMovAndDisOnly and KRamZoneFlagNoAlloc
are intended to be used with this method.

@param aId			The ID of the RAM zone to modify.
@param aClearMask	The bit mask to clear, each flag of which must already be set on the RAM zone.
@param aSetMask		The bit mask to set.

@return KErrNone on success, KErrArgument if the RAM zone of aId not found or if 
aSetMask contains invalid flag bits.

@see TRamDefragRequest::EmptyRamZone()
@see TRamDefragRequest::ClaimRamZone()
@see TRamDefragRequest::DefragRam()

@see KRamZoneFlagDiscardOnly
@see KRamZoneFlagMovAndDisOnly
@see KRamZoneFlagNoAlloc
*/
EXPORT_C TInt Epoc::ModifyRamZoneFlags(TUint aId, TUint aClearMask, TUint aSetMask)
	{
	RamAllocLock::Lock();
	TInt r = TheMmu.ModifyRamZoneFlags(aId, aClearMask, aSetMask);
	RamAllocLock::Unlock();
	return r;
	}


/**
Gets the current count of a particular RAM zone's pages by type.

@param aId The ID of the RAM zone to enquire about
@param aPageData If successful, on return this contains the page count

@return KErrNone if successful, KErrArgument if a RAM zone of aId is not found or
one of the system wide error codes 

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.

@see SRamZonePageCount
*/
EXPORT_C TInt Epoc::GetRamZonePageCount(TUint aId, SRamZonePageCount& aPageData)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::GetRamZonePageCount");
	RamAllocLock::Lock();
	TInt r = TheMmu.GetRamZonePageCount(aId, aPageData);
	RamAllocLock::Unlock();
	return r;
	}


/**
Allocate a block of physically contiguous RAM with a physical address aligned
to a specified power of 2 boundary.
When the RAM is no longer required it should be freed using
Epoc::FreePhysicalRam()

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
		RAM	with the specified alignment could not be found.
@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::AllocPhysicalRam(TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::AllocPhysicalRam");
	RamAllocLock::Lock();
	TInt r = TheMmu.AllocPhysicalRam
		(
		aPhysAddr,
		MM::RoundToPageCount(aSize),
		MM::RoundToPageShift(aAlign),
		(Mmu::TRamAllocFlags)EMemAttStronglyOrdered
		);
	RamAllocLock::Unlock();
	return r;
	}


/**
Allocate a block of physically contiguous RAM with a physical address aligned
to a specified power of 2 boundary from the specified zone.
When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

Note that this method only respects the KRamZoneFlagNoAlloc flag and will always attempt
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
	return ZoneAllocPhysicalRam(&aZoneId, 1, aSize, aPhysAddr, aAlign);
	}


/**
Allocate a block of physically contiguous RAM with a physical address aligned
to a specified power of 2 boundary from the specified RAM zones.
When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

RAM will be allocated into the RAM zones in the order they are specified in the 
aZoneIdList parameter. If the contiguous allocations are intended to span RAM zones 
when required then aZoneIdList should be listed with the RAM zones in ascending 
physical address order.

Note that this method only respects the KRamZoneFlagNoAlloc flag and will always attempt
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
	RamAllocLock::Lock();
	TInt r = TheMmu.ZoneAllocPhysicalRam(aZoneIdList, aZoneIdCount, aSize, aPhysAddr, aAlign);
	RamAllocLock::Unlock();
	return r;
	}


/**
Attempt to allocate discontiguous RAM pages.

When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

@param	aNumPages	The number of discontiguous pages required to be allocated
@param	aPageList	This should be a pointer to a previously allocated array of
					aNumPages TPhysAddr elements.  On a successful allocation it 
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
	RamAllocLock::Lock();
	TInt r = TheMmu.AllocPhysicalRam(aPageList,aNumPages,(Mmu::TRamAllocFlags)EMemAttStronglyOrdered);
	RamAllocLock::Unlock();
	return r;
	}


/**
Attempt to allocate discontiguous RAM pages from the specified zone.

Note that this method only respects the KRamZoneFlagNoAlloc flag and will always attempt
to allocate regardless of whether the other flags are set for the specified RAM zones 
or not.

When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

@param 	aZoneId		The ID of the zone to attempt to allocate from.
@param	aNumPages	The number of discontiguous pages required to be allocated 
					from the specified zone.
@param	aPageList	This should be a pointer to a previously allocated array of
					aNumPages TPhysAddr elements.  On a successful 
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
	return ZoneAllocPhysicalRam(&aZoneId, 1, aNumPages, aPageList);
	}


/**
Attempt to allocate discontiguous RAM pages from the specified RAM zones.
The RAM pages will be allocated into the RAM zones in the order that they are specified 
in the aZoneIdList parameter, the RAM zone preferences will be ignored.

Note that this method only respects the KRamZoneFlagNoAlloc flag and will always attempt
to allocate regardless of whether the other flags are set for the specified RAM zones 
or not.

When the RAM is no longer required it should be freed using Epoc::FreePhysicalRam().

@param 	aZoneIdList	A pointer to an array of RAM zone IDs of the RAM zones to 
					attempt to allocate from.
@param	aZoneIdCount The number of RAM zone IDs pointed to by aZoneIdList.
@param	aNumPages	The number of discontiguous pages required to be allocated 
					from the specified zone.
@param	aPageList	This should be a pointer to a previously allocated array of
					aNumPages TPhysAddr elements.  On a successful 
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
	RamAllocLock::Lock();
	TInt r = TheMmu.ZoneAllocPhysicalRam(aZoneIdList, aZoneIdCount, aNumPages, aPageList);
	RamAllocLock::Unlock();
	return r;
	}


/**
Free a previously-allocated block of physically contiguous RAM.

Specifying one of the following may cause the system to panic: 
a) an invalid physical RAM address.
b) valid physical RAM addresses where some had not been previously allocated.
c) an address not aligned to a page boundary.

@param	aPhysAddr	The physical address of the base of the block to be freed.
					This must be the address returned by a previous call to
					Epoc::AllocPhysicalRam(), Epoc::ZoneAllocPhysicalRam(), 
					Epoc::ClaimPhysicalRam() or Epoc::ClaimRamZone().
@param	aSize		The size in bytes of the required block. The specified size
					is rounded up to the page size, since only whole pages of
					physical RAM can be allocated.
@return	KErrNone if the operation was successful.



@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::FreePhysicalRam(TPhysAddr aPhysAddr, TInt aSize)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::FreePhysicalRam");
	RamAllocLock::Lock();
	TheMmu.FreePhysicalRam(aPhysAddr,MM::RoundToPageCount(aSize));
	RamAllocLock::Unlock();
	return KErrNone;
	}


/**
Free a number of physical RAM pages that were previously allocated using
Epoc::AllocPhysicalRam() or Epoc::ZoneAllocPhysicalRam().

Specifying one of the following may cause the system to panic: 
a) an invalid physical RAM address.
b) valid physical RAM addresses where some had not been previously allocated.
c) an address not aligned to a page boundary.

@param	aNumPages	The number of pages to be freed.
@param	aPageList	An array of aNumPages TPhysAddr elements.  Where each element
					should contain the physical address of each page to be freed.
					This must be the same set of addresses as those returned by a 
					previous call to Epoc::AllocPhysicalRam() or 
					Epoc::ZoneAllocPhysicalRam().
@return	KErrNone if the operation was successful.
  
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
	RamAllocLock::Lock();
	TheMmu.FreePhysicalRam(aPageList,aNumPages);
	RamAllocLock::Unlock();
	return KErrNone;
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
	RamAllocLock::Lock();
	TInt r = TheMmu.FreeRamZone(aZoneId);
	RamAllocLock::Unlock();
	return r;
	}


/**
Allocate a specific block of physically contiguous RAM, specified by physical
base address and size.
If and when the RAM is no longer required it should be freed using
Epoc::FreePhysicalRam()

@param	aPhysAddr	The physical address of the base of the required block.
@param	aSize		The size in bytes of the required block. The specified size
					is rounded up to the page size, since only whole pages of
					physical RAM can be allocated.
@return	KErrNone if the operation was successful.
		KErrArgument if the range of physical addresses specified included some
					which are not valid physical RAM addresses.
		KErrInUse	if the range of physical addresses specified are all valid
					physical RAM addresses but some of them have already been
					allocated for other purposes.
@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Epoc::ClaimPhysicalRam(TPhysAddr aPhysAddr, TInt aSize)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::ClaimPhysicalRam");
	RamAllocLock::Lock();
	TInt r = TheMmu.ClaimPhysicalRam
		(
		aPhysAddr,
		MM::RoundToPageCount(aSize),
		(Mmu::TRamAllocFlags)EMemAttStronglyOrdered
		);
	RamAllocLock::Unlock();
	return r;
	}


/**
Translate a virtual address to the corresponding physical address.

@param	aLinAddr	The virtual address to be translated.
@return	The physical address corresponding to the given virtual address, or
		KPhysAddrInvalid if the specified virtual address is unmapped.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TPhysAddr Epoc::LinearToPhysical(TLinAddr aLinAddr)
	{
//	This precondition is violated by various parts of the system under some conditions,
//	e.g. when __FLUSH_PT_INTO_RAM__ is defined. This function might also be called by
//	a higher-level RTOS for which these conditions are meaningless. Thus, it's been
//	disabled for now.
//	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,"Epoc::LinearToPhysical");

	// When called by a higher-level OS we may not be in a DThread context, so avoid looking up the
	// current process in the DThread for a global address
	TInt osAsid = KKernelOsAsid;
	if (aLinAddr < KGlobalMemoryBase)
		{
		// Get the os asid of current thread's process so no need to open a reference on it.
		DMemModelProcess* pP=(DMemModelProcess*)TheCurrentThread->iOwningProcess;
		osAsid = pP->OsAsid();
		}
	
#if 1
	return Mmu::UncheckedLinearToPhysical(aLinAddr, osAsid);
#else
	MmuLock::Lock();
	TPhysAddr addr =  Mmu::LinearToPhysical(aLinAddr, osAsid);
	MmuLock::Unlock();
	return addr;
#endif
	}


//
// Misc
//

EXPORT_C TInt TInternalRamDrive::MaxSize()
	{
	TUint maxPages = (TUint(TheSuperPage().iRamDriveSize)>>KPageShift)+TheMmu.FreeRamInPages(); // current size plus spare memory
	TUint maxPages2 = TUint(PP::RamDriveMaxSize)>>KPageShift;
	if(maxPages>maxPages2)
		maxPages = maxPages2;
	return maxPages*KPageSize;
	}


TInt M::PageSizeInBytes()
	{
	return KPageSize;
	}


void M::BTracePrime(TUint aCategory)
	{
	(void)aCategory;

#ifdef BTRACE_KERNEL_MEMORY
	// Must check for -1 as that is the default value of aCategory for
	// BTrace::Prime() which is intended to prime all categories that are 
	// currently enabled via a single invocation of BTrace::Prime().
	if(aCategory == BTrace::EKernelMemory || (TInt)aCategory == -1)
		{
		NKern::ThreadEnterCS();
		RamAllocLock::Lock();
		BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryInitialFree, TheSuperPage().iTotalRamSize);
		BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryCurrentFree, Kern::FreeRamInBytes());
		BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryMiscAlloc, Epoc::KernelMiscPages << KPageShift);
		BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryDemandPagingCache, ThePager.MinimumPageCount() << KPageShift);
		BTrace8(BTrace::EKernelMemory, BTrace::EKernelMemoryDrvPhysAlloc, Epoc::DriverAllocdPhysRam, -1);
		RamAllocLock::Unlock();
		NKern::ThreadLeaveCS();
		}
#endif
	TheMmu.BTracePrime(aCategory);
	}


//
// DPlatChunkHw
//

/**
Create a hardware chunk object, optionally mapping a specified block of physical
addresses with specified access permissions and cache policy.

When the mapping is no longer required, close the chunk using chunk->Close(0);
Note that closing a chunk does not free any RAM pages which were mapped by the
chunk - these must be freed separately using Epoc::FreePhysicalRam().

@param	aChunk	Upon successful completion this parameter receives a pointer to
				the newly created chunk. Upon unsuccessful completion it is
				written with a NULL pointer. The virtual address of the mapping
				can subsequently be discovered using the LinearAddress()
				function on the chunk.
@param	aAddr	The base address of the physical region to be mapped. This will
				be rounded down to a multiple of the hardware page size before
				being used.
@param	aSize	The size of the physical address region to be mapped. This will
				be rounded up to a multiple of the hardware page size before
				being used; the rounding is such that the entire range from
				aAddr to aAddr+aSize-1 inclusive is mapped. For example if
				aAddr=0xB0001FFF, aSize=2 and the hardware page size is 4KB, an
				8KB range of physical addresses from 0xB0001000 to 0xB0002FFF
				inclusive will be mapped.
@param	aMapAttr Mapping attributes required for the mapping. This is formed
				by ORing together values from the TMappingAttributes enumeration
				to specify the access permissions and caching policy.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
@see TMappingAttributes
*/
EXPORT_C TInt DPlatChunkHw::New(DPlatChunkHw*& aChunk, TPhysAddr aAddr, TInt aSize, TUint aMapAttr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DPlatChunkHw::New");
	__KTRACE_OPT(KMMU,Kern::Printf("DPlatChunkHw::New phys=%08x, size=%x, attribs=%x",aAddr,aSize,aMapAttr));

	aChunk = NULL;

	// check size...
	if(aSize<=0)
		return KErrArgument;
	TPhysAddr end = aAddr+aSize-1;
	if(end<aAddr) // overflow?
		return KErrArgument;
	aAddr &= ~KPageMask;
	TUint pageCount = (end>>KPageShift)-(aAddr>>KPageShift)+1;

	// check attributes...
	TMappingPermissions perm;
	TInt r = MM::MappingPermissions(perm,*(TMappingAttributes2*)&aMapAttr);
	if(r!=KErrNone)
		return r;
	TMemoryAttributes attr;
	r = MM::MemoryAttributes(attr,*(TMappingAttributes2*)&aMapAttr);
	if(r!=KErrNone)
		return r;

	// construct a hardware chunk...
	DMemModelChunkHw* pC = new DMemModelChunkHw;
	if(!pC)
		return KErrNoMemory;

	// set the executable flags based on the specified mapping permissions...
	TMemoryCreateFlags flags = EMemoryCreateDefault;
	if(perm&EExecute)
		flags = (TMemoryCreateFlags)(flags|EMemoryCreateAllowExecution);

	r = MM::MemoryNew(pC->iMemoryObject, EMemoryObjectHardware, pageCount, flags, attr);
	if(r==KErrNone)
		{
		r = MM::MemoryAddContiguous(pC->iMemoryObject,0,pageCount,aAddr);
		if(r==KErrNone)
			{
			r = MM::MappingNew(pC->iKernelMapping,pC->iMemoryObject,perm,KKernelOsAsid);
			if(r==KErrNone)
				{
				pC->iPhysAddr = aAddr;
				pC->iLinAddr = MM::MappingBase(pC->iKernelMapping);
				pC->iSize = pageCount<<KPageShift;
				const TMappingAttributes2& lma = MM::LegacyMappingAttributes(attr,perm); // not needed, but keep in case someone uses this internal member
				*(TMappingAttributes2*)&pC->iAttribs = lma;
				}
			}
		}

	if(r==KErrNone)
		aChunk = pC;
	else
		pC->Close(NULL);
	return r;
	}


TInt DMemModelChunkHw::Close(TAny*)
	{
	__KTRACE_OPT2(KOBJECT,KMMU,Kern::Printf("DMemModelChunkHw::Close %d %O",AccessCount(),this));
	TInt r = Dec();
	if(r==1)
		{
		MM::MappingDestroy(iKernelMapping);
		MM::MemoryDestroy(iMemoryObject);
		DBase::Delete(this);
		}
	return r;
	}



//
// Demand Paging
//

#ifdef _DEBUG
extern "C" void ASMCheckPagingSafe(TLinAddr aPC, TLinAddr aLR, TLinAddr aStartAddres, TUint aLength)
	{
	if(M::CheckPagingSafe(EFalse, aStartAddres, aLength))
		return;
	Kern::Printf("ASM_ASSERT_PAGING_SAFE FAILED: pc=%x lr=%x",aPC,aLR);
	__NK_ASSERT_ALWAYS(0);
	}

extern "C" void ASMCheckDataPagingSafe(TLinAddr aPC, TLinAddr aLR, TLinAddr aStartAddres, TUint aLength)
	{
	if(M::CheckPagingSafe(ETrue, aStartAddres, aLength))
		return;
	__KTRACE_OPT(KDATAPAGEWARN,Kern::Printf("Data paging: ASM_ASSERT_DATA_PAGING_SAFE FAILED: pc=%x lr=%x",aPC,aLR));
	}
#endif


DMutex* CheckMutexOrder()
	{
#ifdef _DEBUG
	SDblQue& ml = TheCurrentThread->iMutexList;
	if(ml.IsEmpty())
		return NULL;
	DMutex* mm = _LOFF(ml.First(), DMutex, iOrderLink);
	if (KMutexOrdPageOut >= mm->iOrder)
		return mm;
#endif
	return NULL;
	}


TBool M::CheckPagingSafe(TBool aDataPaging, TLinAddr aStartAddr, TUint aLength)
	{
	if(K::Initialising)
		return ETrue;
	
	NThread* nt = NCurrentThread();
	if(!nt)
		return ETrue; // We've not booted properly yet!

	if(aStartAddr>=KUserMemoryLimit)
		return ETrue; // kernel memory can't be paged

	if(IsUnpagedRom(aStartAddr,aLength))
		return ETrue;

	TBool dataPagingEnabled = K::MemModelAttributes&EMemModelAttrDataPaging;

	DThread* thread = _LOFF(nt,DThread,iNThread);
	NFastMutex* fm = NKern::HeldFastMutex();
	if(fm)
		{
		if(!thread->iPagingExcTrap || fm!=&TheScheduler.iLock)
			{
			if (!aDataPaging)
				{
				__KTRACE_OPT2(KPAGING,KPANIC,Kern::Printf("DP: CheckPagingSafe FAILED - FM Held"));
				return EFalse;
				}
			else
				{
				__KTRACE_OPT(KDATAPAGEWARN, Kern::Printf("Data paging: CheckPagingSafe FAILED - FM Held"));
				return !dataPagingEnabled;
				}
			}
		}

	DMutex* m = CheckMutexOrder();
	if (m)
		{
		if (!aDataPaging)
			{
			__KTRACE_OPT2(KPAGING,KPANIC,Kern::Printf("DP: Mutex Order Fault %O",m));
			return EFalse;
			}
		else
			{
			__KTRACE_OPT(KDATAPAGEWARN, Kern::Printf("Data paging: Mutex Order Fault %O mem=%x+%x",m,aStartAddr,aLength));
			return !dataPagingEnabled;
			}
		}
	
	return ETrue;
	}


