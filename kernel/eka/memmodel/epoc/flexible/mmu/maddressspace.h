// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
@file
@internalComponent
*/

#ifndef MADDRESSSPACE_H
#define MADDRESSSPACE_H

#include "mrefcntobj.h"
#include "maddrcont.h"
#include "mvalloc.h"



//
// DAddressSpace
//

/**
Class representing the virtual address space used by a single process.

Each address space has:
- An MMU page directory.
- An allocator for virtual addresses.
- A list of memory mappings currently mapped into it.

Most other APIs in the kernel which make use of address spaces don't use pointers to
the address space object itself, instead they use an OS Address Space ID (OS ASID).
This is an integer less than KNumOsAsids and can be used to index into the array of
address space objects - #AddressSpace.
*/
class DAddressSpace : public DReferenceCountedObject
	{
public:
	/**
	Initialiser called during stage 2 of system boot.
	This includes initialisation of the kernel's address space object.
	*/
	static void Init2();

	/**
	Create a new address space.

	This creates a DAddressSpace address space object and adds it to the array
	of all address space objects - #AddressSpace. The function returns the objects
	index into this array, this value is used as OS Address Space ID (OS ASID)
	in most kernel APIs which refer to address spaces.

	@param[out] aPageDirectory	Returns the physical address of the MMU page directory
								which was created for the new address space.

	@return On success, the OS ASID value for the new address space;
			otherwise one of the system wide error codes
	*/
	static TInt New(TPhysAddr& aPageDirectory);
public:
	DAddressSpace();
	~DAddressSpace();

	/**
	Allocate a region of virtual addresses within this address space.

	The returned region may have a start address and/or size which is different to
	those requested due to various alignment requirements in the implementation.
	However the returned region will always include all addresses requested.

	The range of virtual addresses available to a user-side address space is
	#KUserLocalDataBase through to #KUserLocalDataEnd.

	The range of virtual addresses available to the kernel's address space (#KKernelOsAsid)
	is #KKernelSectionBase though to #KKernelSectionEnd.

	@param[out] aAddr			Returns the start address of the region which was allocated.
								This will always be aligned to a multiple of the page colouring
								size: #KPageColourCount*#KPageSize.
	@param[out] aSize			Returns the size, in bytes, of the region which was allocated.
	@param		aRequestedAddr	The requested start address of the region to allocate,
								or zero if no specific address is required.
	@param		aRequestedSize	The requested size, in bytes, of the region to allocate.
	@param		aPdeType		A value from #TPdeType ORed with flags from #TVirtualSlabType.
								This is used to prevent incompatible memory uses (different
								\a aPdeType values) from being allocated virtual addresses
								which would share the same MMU page table.

	@return KErrNone if successful, otherwise one of the system wide error codes.
	*/
	TInt AllocateVirtualMemory(TLinAddr& aAddr, TUint& aSize, TLinAddr aRequestedAddr, TUint aRequestedSize, TUint aPdeType);

	/**
	Allocate a region of virtual addresses within the global address region, for use by
	user-side code. These global addresses are outside the range of addresses used
	by any individual address space, and so are globally unique. They lie in the range
	from #KGlobalMemoryBase through to #KUserMemoryLimit.

	The arguments for this function are of the same type and use as #AllocateVirtualMemory.
	*/
	static TInt AllocateUserGlobalVirtualMemory(TLinAddr& aAddr, TUint& aSize, TLinAddr aRequestedAddr, TUint aRequestedSize, TUint aPdeType);

	/**
	Free a virtual addresses region which was allocated with #AllocateVirtualMemory
	or #AllocateUserGlobalVirtualMemory. The region supplied to this function
	must either be one supplied to a previous call to AllocateVirtualMemory or
	be one returned by that function.

	@param aAddr	Start address of the region to be freed.
	@param aSize	Size, in bytes, of the region to be freed.
	*/
	void FreeVirtualMemory(TLinAddr aAddr, TUint aSize);

	/**
	Allocate a region of virtual addresses for use by memory which should appear at
	the same address in all user-side processes which map it. E.g. for position dependant
	memory like executable code.

	The region allocated lies in the range used by normal user-side address spaces
	but there is no guarantee that the addresses are not already in use by any of them,
	and there is nothing to prevent the allocated addresses from being returned by any
	future call to #AllocateVirtualMemory.

	However, as the 'common' addresses are allocated from the top of memory downwards
	and #AllocateVirtualMemory allocates upwards from the bottom of memory it XXX TODO

	The arguments for this function are of the same type and use as #AllocateVirtualMemory.
	*/
	static TInt AllocateUserCommonVirtualMemory(TLinAddr& aAddr, TUint& aSize, TLinAddr aRequestedAddr, TUint aRequestedSize, TUint aPdeType);

	/**
	Free a virtual addresses region which was allocated with #AllocateUserCommonVirtualMemory.
	The region supplied to this function must either be one supplied to a previous
	call to AllocateVirtualMemory or be one returned by that function.

	@param aAddr	Start address of the region to be freed.
	@param aSize	Size, in bytes, of the region to be freed.
	*/
	static void FreeUserCommonVirtualMemory(TLinAddr aAddr, TUint aSize);

	/**
	Add a memory mapping to this address space's list of mappings.

	This is intended only for use by #DMemoryMapping.

	@param aAddr	The address which is allocated to the mapping
					(DMemoryMapping::iAllocatedLinAddrAndOsAsid&~KPageMask).
	@param aMapping	The mapping to add.

	@return KErrNone if successful, otherwise one of the system wide error codes.
	*/
	TInt AddMapping(TLinAddr aAddr, DMemoryMapping* aMapping);

	/**
	Remove a memory mapping from this address space's list of mappings.

	This is intended only for use by #DMemoryMapping.

	@param aAddr	The address which was used to add the mapping with AddMapping.

	@return Pointer to the removed mapping or the null pointer if no mapping
			was found to match the specified address.
	*/
	DMemoryMapping* RemoveMapping(TLinAddr aAddr);

	/**
	Get a pointer to a memory mapping which was previously added to this
	address space's list of mappings. This function does not perform any additional
	reference counting or locking to prevent the returned pointer becoming invalid,
	therefore the caller must ensure that it is not possible for another thread to
	destroy the mapping.

	The typical use case for this function is where the owner of the mapping has
	not saved the pointer to the mapping object but has instead kept track of the
	virtual address it uses. This function can then be used to retrieve the pointer
	to the object again.

	If no mapping exists for the specified address the results are unpredictable.

	@param aAddr	The address which was used to add the mapping with AddMapping.

	@return Pointer to the mapping.
	*/
	DMemoryMapping* GetMapping(TLinAddr aAddr);

	/**
	Find the mapping within this address space which maps a specified region of
	addresses. A mapping is only found if all of the bytes in the specified region
	lie within it and it is current mapping a memory object (#DMemoryMapping::IsAttached
	returns true).

	The returned mapping will have had its reference count incremented and it is
	the callers responsibility to balance this with a #Close or #AsyncClose.

	@param		aAddr				The start address of the region.
	@param		aSize				The size, in bytes, of the region.
	@param[out] aOffsetInMapping	Returns the byte address offset within the found
									mapping which corresponds to \a aAddr.
	@param[out] aInstanceCount		The instance count of the found mapping.

	@return Pointer to the mapping which contains the specified region
			or the null pointer if no mapping was found.

	@post The returned mapping will have had its reference count incremented.
	*/
	DMemoryMapping* FindMapping(TLinAddr aAddr, TUint aSize, TUint& aOffsetInMapping, TUint& aInstanceCount);

	/**
	Check that the virtual addresses previously allocated with #AllocateVirtualMemory
	or #AllocateUserGlobalVirtualMemory are compatible with the specified 'pde type'.

	This is only intended for used by error checking in debug builds.

	@param aAddr	The start address of the region.
	@param aSize	The size, in bytes, of the region.
	@param aPdeType	See description in AllocateVirtualMemory.
	*/
	TBool CheckPdeType(TLinAddr aAddr, TUint aSize, TUint aPdeType);

private:
	/**
	Second phase constructor.

	@param aOsAsid	The OS ASID for the address space.
	@param aStart	The first virtual address available in the address space.
	@param aEnd		The last virtual address (plus one) available in the address space.

	@return KErrNone if successful, otherwise one of the system wide error codes.
	*/
	TInt Construct(TInt aOsAsid, TLinAddr aStart, TLinAddr aEnd);

	/**
	Wait on the mutex used to protect virtual address allocation and the
	addition/removal of memory mappings. This is only used internally by
	the DAddressSpace methods.
	*/
	void Lock();

	/**
	Reverse the action of #Lock.
	*/
	void Unlock();

	/**
	In debug builds, dump information about each mapping in the address to the
	kernel trace port.
	*/
	void Dump();

private:
	/**
	The OS ASID value for this address space.
	This is its index in the array #AddressSpace.
	*/
	TInt iOsAsid;

	/**
	Lock currently being used to protect virtual address allocation (#iVirtualAllocator)
	and changes to the mapping container (#iMappings).
	*/
	DMutex* iLock;

	/**
	Container containing all mappings added to this address space.
	*/
	RAddressedContainer iMappings;

	/**
	Allocator object for virtual addresses within the address space.
	*/
	RVirtualAllocator iVirtualAllocator;

private:
	/**
	Allocator for virtual addresses within the global address region for use by user-side code.

	This is used by #AllocateUserGlobalVirtualMemory and the lock mutex for the
	kernel's address space is used to protect access.
	*/
	static RVirtualAllocator UserGlobalVirtualAllocator;

	/**
	Allocator for virtual addresses for use by memory which should appear at
	the same address in all user-side processes which map it.

	This is used by AllocateUserCommonVirtualMemory and the lock mutex for the
	kernel's address space is used to protect access.
	*/
	static RBackwardsVirtualAllocator UserCommonVirtualAllocator;
	};


/**
Array of all address spaces. An OS Address Space ID (OS ASID) can be used to
index this array to find the corresponding DAddressSpace object.
*/
extern DAddressSpace* AddressSpace[];


#endif
