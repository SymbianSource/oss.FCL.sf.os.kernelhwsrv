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

#ifndef MM_H
#define MM_H

#include <mmtypes.h>
#include <platform.h>


class DMemoryObject;
class DMemoryMapping;
class DMemModelThread;
class DMemModelProcess;
class DPhysicalPinMapping;

/**
Memory object types for MM::MemoryNew which indicates how the
contents of a memory object are to be managed.
*/
enum TMemoryObjectType
	{
	/**
	Memory object for containing memory mapped hardware devices or special
	purpose memory for which the physical addresses are fixed. The contents of
	these type of objects are manipulated with the functions:
	- MM::MemoryAddPages
	- MM::MemoryAddContiguous
	- MM::MemoryRemovePages
	*/
	EMemoryObjectHardware				= 0,

	/**
	Memory object containing normal program memory (RAM) which is allocated from a
	system wide pool. The contents of these type of objects are manipulated with
	the functions:
	- MM::MemoryAlloc
	- MM::MemoryAllocContiguous
	- MM::MemoryFree
	*/
	EMemoryObjectUnpaged				= 1,

	/**
	Memory object containing normal program memory (RAM) which is allocated from a
	system wide pool. This is the same basic management as EMemoryObjectUnpaged however
	RAM defragmentation activity may substituted physical RAM pages with others and
	this process may cause transient page faults which make this memory not suitable
	for most kernel-side usage.

	The contents of these type of objects are manipulated with the functions:
	- MM::MemoryAlloc
	- MM::MemoryAllocContiguous
	- MM::MemoryFree
	*/
	EMemoryObjectMovable				= 2,

	/**
	Memory object containing normal program memory (RAM) which is demand paged
	from a backing store. The contents of these type of objects are manipulated
	with the functions.
	- MM::MemoryAlloc
	- MM::MemoryFree
	*/
	EMemoryObjectPaged					= 3,

	/**
	Memory object containing normal program memory (RAM) in the same way as
	EMemoryObjectMovable but with the additional option of marking pages as
	'discardable'. Discardable pages may be reclaimed (remove) by the system at
	any time, this state is controlled using the functions:
	- MM::MemoryAlloc
	- MM::MemoryAllocContiguous
	- MM::MemoryFree
	- MM::MemoryAllowDiscard
	- MM::MemoryDisallowDiscard
	*/
	EMemoryObjectDiscardable			= 4
	};


/**
Bitmask of flags to specify options to MM:MemoryNew.
*/
enum TMemoryCreateFlags
	{
	/**
	Default value which has all flags false.
	*/
	EMemoryCreateDefault				= 0,

	/**
	Memory allocated for the memory object contents does not need wiping.
	IMPORTANT, memory is normally wiped for security purposes, this attribute
	should only be used when the old contents of any memory allocated can not
	be read by any process without TCB capability.
	*/
	EMemoryCreateNoWipe					= 1<<0,

	/**
	Use the custom wipe byte value when wiping memory, rather than the default value.
	@see EMemoryCreateUseCustomWipeByte
	*/
	EMemoryCreateUseCustomWipeByte		= 1<<1,

	/**
	Pre-create all resources the memory object needs so that operations on it
	can not fail due to low memory conditions. This excludes any explicit
	allocation of memory pages for use as the objects contents.
	*/
	EMemoryCreateReserveAllResources	= 1<<2,

	/**
	Memory object contents are not allowed to be pinned.
	*/
	EMemoryCreateDisallowPinning		= 1<<3,

	/**
	Memory object contents are read-only. Mappings with write permissions are
	not allowed.
	*/
	EMemoryCreateReadOnly				= 1<<4,

	/**
	Memory object contents may be executed. Mappings with execute permissions
	are allowed.
	*/
	EMemoryCreateAllowExecution			= 1<<5,

	/**
	Bit position for the least significant bit of an 8 bit value to use for
	wiping newly allocated memory.
	@see EMemoryCreateUseCustomWipeByte
	@see EMemoryCreateNoWipe
	*/
	EMemoryCreateWipeByteShift			= 8,

	// for selected internal use only...

	/**
	The TMemoryObjectType specified is actually a pointer to the DMemoryManager to use.
	*/
	EMemoryCreateCustomManager			= 1<<30,

	/**
	Memory object contents are to be demand paged. 
	*/
	EMemoryCreateDemandPaged			= 1U<<31
	};


/**
Attributes that the memory in a memory object has.

These govern how the MMU and caching systems treat the memory. The following
terms have meanings as specified in the ARM Architecture Reference Manual - see
the section 'Memory types and attributes and the Memory order model'.

- Memory types 'normal', 'device' and 'strongly-ordered'.
- 'Shareable' attribute.
*/
enum TMemoryAttributes
	{
	// memory types (copy of TMemoryType)...

	EMemoryAttributeStronglyOrdered 	= EMemAttStronglyOrdered,
	EMemoryAttributeDevice 				= EMemAttDevice,
	EMemoryAttributeNormalUncached 		= EMemAttNormalUncached,
	EMemoryAttributeNormalCached 		= EMemAttNormalCached,
	EMemoryAttributeKernelInternal4 	= EMemAttKernelInternal4,
	EMemoryAttributePlatformSpecific5 	= EMemAttPlatformSpecific5,
	EMemoryAttributePlatformSpecific6	= EMemAttPlatformSpecific6,
	EMemoryAttributePlatformSpecific7	= EMemAttPlatformSpecific7,

	/**
	Bitmask to extract TMemoryType value from this enum. E.g.
	@code
	TMemoryAttributes attr;
	TMemoryType type = (TMemoryType)(attr&EMemoryAttributeTypeMask);
	@endcode
	*/
	EMemoryAttributeTypeMask			= KMemoryTypeMask,

	/**
	Set if memory is Shareable.
	*/
	EMemoryAttributeShareable			= 0x08,

	/**
	Legacy (and little-used/unused?) ARM attribute.
	*/
	EMemoryAttributeUseECC				= 0x10,


	/**
	Number of bits required to store memory attribute value.
	@internalComponent
	*/
	EMemoryAttributeShift				= 5,

	/**
	Bitmask for all significant attribute bits.
	@internalComponent
	*/
	EMemoryAttributeMask				= (1<<EMemoryAttributeShift)-1,

	// pseudo attributes...

	/**
	Indicates the Shareable attribute should be the default value for the system.
	See macro __CPU_USE_SHARED_MEMORY
	*/
	EMemoryAttributeDefaultShareable	= 0x80000000,

	// popular combinations...

	/**
	Normal program memory for use by software.
	*/
	EMemoryAttributeStandard			= EMemoryAttributeNormalCached|EMemoryAttributeDefaultShareable
	};


/**
Access permissions applied to Memory Mappings.
*/
enum TMappingPermissions
	{
	EUser	    = 1<<0, ///< Unprivileged (user mode) access allowed.
	EReadWrite  = 1<<1, ///< Memory contents may be modified
	EExecute	= 1<<2, ///< Memory contents may be executed as code.

	// popular combinations...
	EUserReadOnly = EUser,
	EUserReadWrite = EUser|EReadWrite,
	EUserExecute = EUser|EExecute,
	ESupervisorReadOnly = 0,
	ESupervisorReadWrite = EReadWrite,
	ESupervisorExecute = EExecute
	};


/**
Bitmask of flags to specify options to MM::MappingNew.
*/
enum TMappingCreateFlags
	{
	/**
	Default value which has all flags false.
	*/
	EMappingCreateDefault				= 0,

	/**
	Allocate the specified virtual address.
	*/
	EMappingCreateExactVirtual			= 1<<0,

	/**
	Pre-create all resources (like page tables) that the memory mapping
	needs so that operations on it can not fail due to low memory conditions.
	*/
	EMappingCreateReserveAllResources	= 1<<1,

	// for selected internal use only...

	/**
	Flag memory as being demand paged.
	Exclusive with EMappingCreatePinning and EMappingCreateReserveAllResources.
	@internalTechnology
	*/
	EMappingCreateDemandPaged			= 1<<27,

	/**
	Flag memory as requiring a common address across all address spaces, also
	implies EMappingCreateExactVirtual.
	@internalTechnology
	*/
	EMappingCreateCommonVirtual			= 1<<28,

	/**
	Allocate virtual address in the global region which it to be used for
	user-mode access. (KGlobalMemoryBase<=address<KUserMemoryLimit).
	@internalTechnology
	*/
	EMappingCreateUserGlobalVirtual		= 1<<29,

	/**
	Don't allocate any virtual memory in the address space, use the specified
	address and assume ownership of this.
	@internalTechnology
	*/
	EMappingCreateAdoptVirtual			= 1<<30,

	/**
	Don't allocate any virtual memory in the address space, just used the
	specified address.
	@internalTechnology
	*/
	EMappingCreateFixedVirtual	= 1U<<31
	};


class DMemModelChunk;
class TPagedCodeInfo;

/**
Static interface to the Flexible Memory Model implementation
for use by the Symbian OS aware part of the memory model codebase.
*/
class MM
	{
public:
	//
	// Memory Object functions
	//

	/**
	Create a new memory object.

	A memory object is a sparse array of memory pages to which memory mappings
	may be attached. All pages in the array are managed using the same methods
	(see TMemoryObjectType) and have the same attributes (see TMemoryAttributes).

	On creation it contains no pages.

	@param[out] aMemory	Pointer reference which on success is set to the address
						of the created memory object.
	@param aType		Value from TMemoryObjectType which indicates how the
						contents of the memory object are to be managed.
	@param aPageCount	Size of the memory object, in number of pages.
	@param aCreateFlags	Bitmask of option flags from enum TMemoryCreateFlags.
	@param aAttributes	Bitmask of values from enum TMemoryAttributes.

	@return KErrNone, if successful;
			otherwise another of the system wide error codes.
	*/
	static TInt MemoryNew(DMemoryObject*& aMemory, TMemoryObjectType aType, TUint aPageCount, TMemoryCreateFlags aCreateFlags=EMemoryCreateDefault, TMemoryAttributes aAttributes=EMemoryAttributeStandard);

	/**
	Assign a mutex which will be used to serialise explicit modifications to the
	specified memory object.

	If a lock hasn't been specified for a particular object, it will make use of
	one allocated dynamically allocated from a shared pool; this mutex will be of 'order'
	#KMutexOrdMemoryObject.

	@see MemoryObjectLock.
	*/
	static void MemorySetLock(DMemoryObject* aMemory, DMutex* aLock);

	/**
	Wait on the specified memory object's lock mutex.
	*/
	static void MemoryLock(DMemoryObject* aMemory);

	/**
	Signal the specified memory object's lock mutex.
	*/
	static void MemoryUnlock(DMemoryObject* aMemory);

	/**
	Remove all memory from a memory object and close a reference on it.

	If there are no longer any mappings to the memory, or other references,
	this will result in the memory object being destroyed. However whilst
	other references exist, cleanup of memory and other resources may be
	delayed indefinitely.

	This function acquires and releases the memory objects lock.
	See MM::MemorySetLock.

	@param aMemory	Pointer reference to memory object to be destroyed.
					On return from the function this is set to the null-pointer.
					No action is performed if the reference was already the
					null-pointer on entry to this function.
	*/
	static void MemoryDestroy(DMemoryObject*& aMemory);

	/**
	Allocate memory for a specified region within a memory object.
	Memory is allocated as appropriate for the object type, e.g.
	For Unpaged objects, from the system RAM pool; for Paged objects, in the
	backing store.

	This function acquires and releases the memory objects lock.
	See MM::MemorySetLock.

	Supported for memory object types:
	- EMemoryObjectUnpaged
	- EMemoryObjectMovable
	- EMemoryObjectPaged
	- EMemoryObjectDiscardable

	@param aMemory	The memory object.
	@param aIndex	Page index for start of the region.
	@param aCount	Number of pages in the region.

	@return KErrNone, if successful;
			KErrAlreadyExists, if any part of the region was already allocated;
			KErrArgument, if the region exceeds the bounds of the memory object;
			KErrNotSupported, if the memory object doesn't support this operation;
			otherwise another of the system wide error codes.
	*/
	static TInt MemoryAlloc(DMemoryObject* aMemory, TUint aIndex, TUint aCount);

	/**
	Allocate memory for a specified region within a memory object.
	The allocated memory will have contiguous physical addresses.

	This function acquires and releases the memory objects lock.
	See MM::MemorySetLock.

	Important note, this function can unexpectedly fail with KErrAlreadyExists
	if any part of the the region previously contained allocated memory which
	had been freed but which was pinned. It is therefore not suitable for general
	use.

	Supported for memory object types:
	- EMemoryObjectUnpaged
	- EMemoryObjectDiscardable

	@param aMemory	The memory object.
	@param aIndex	Page index for start of the region.
	@param aCount	Number of pages in the region.
	@param aAlign	Log2 of the alignment (in bytes) that the address of the allocated physical RAM must have.
	@param[out] aPhysAddr	On success, this is set to the start address of the allocated physical RAM.

	@return KErrNone, if successful;
			KErrAlreadyExists, if any part of the region was already allocated;
			KErrArgument, if the region exceeds the bounds of the memory object;
			KErrNotSupported, if the memory object doesn't support this operation;
			otherwise another of the system wide error codes.
	*/
	static TInt MemoryAllocContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TUint aAlign, TPhysAddr& aPhysAddr);

	/**
	Free (unallocate) memory for a specified region within a memory object.
	This is the inverse operation to MemoryAlloc and MemoryAllocContiguous.

	This function acquires and releases the memory objects lock.
	See MM::MemorySetLock.

	Supported for memory object types:
	- EMemoryObjectUnpaged
	- EMemoryObjectMovable
	- EMemoryObjectPaged
	- EMemoryObjectDiscardable

	@param aMemory	The memory object.
	@param aIndex	Page index for start of the region.
	@param aCount	Number of pages in the region.
	*/
	static void MemoryFree(DMemoryObject* aMemory, TUint aIndex, TUint aCount);

	/**
	Add the specified pages to a region in a memory object.

	This function acquires and releases the memory objects lock.
	See MM::MemorySetLock.

	Supported for memory object types:
	- EMemoryObjectHardware

	@param aMemory	The memory object.
	@param aIndex	Page index for start of the region.
	@param aCount	Number of pages in the region.
	@param aPages	Pointer to array of pages to add. This must contain \a aCount
					number of physical page addresses which are page aligned.

	@return KErrNone, if successful;
			KErrAlreadyExists, if any part of the region already contains pages;
			KErrArgument, if the region exceeds the bounds of the memory object;
			KErrNotSupported, if the memory object doesn't support this operation;
			otherwise another of the system wide error codes.
	*/
	static TInt MemoryAddPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, const TPhysAddr* aPages);

	/**
	Add a contiguous range of pages to a region in a memory object.

	This function acquires and releases the memory objects lock.
	See MM::MemorySetLock.

	Supported for memory object types:
	- EMemoryObjectHardware

	@param aMemory		The memory object.
	@param aIndex		Page index for start of the region.
	@param aCount		Number of pages in the region.
	@param aPhysAddr	The page aligned start address of the pages to be added.

	@return KErrNone, if successful;
			KErrAlreadyExists, if any part of the region already contains pages;
			KErrArgument, if the region exceeds the bounds of the memory object;
			KErrNotSupported, if the memory object doesn't support this operation;
			otherwise another of the system wide error codes.
	*/
	static TInt MemoryAddContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr aPhysAddr);

	/**
	Remove pages from a region in a memory object.

	This is the inverse operation to MemoryAdd and MemoryAddContiguous.

	This function acquires and releases the memory objects lock.
	See MM::MemorySetLock.

	Supported for memory object types:
	- EMemoryObjectHardware

	@param aMemory		The memory object.
	@param aIndex		Page index for start of the region.
	@param aCount		Number of pages in the region.
	@param[out] aPages	Pointer to an array of physical addresses which has a
						length of \a aCount. The contents of this will be set to
						the addresses of the pages which were removed by this
						function. The number of valid entries in this array is
						given by the return value of this function.
						aPages may be the null-pointer, to indicate that these
						page addresses aren't required by the caller.

	@return The number of pages successfully removed from the memory object.
			This gives the number of valid entries in the array \a aPages and is
			less-than or equal to \a aCount.
	*/
	static TUint MemoryRemovePages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr* aPages);

	/**
	Mark a region in a memory object as discardable.

	The system may remove discardable pages from the memory object at any time,
	to be reused for other purposes.

	This function acquires and releases the memory objects lock.
	See MM::MemorySetLock.

	Supported for memory object types:
	- EMemoryObjectDiscardable

	@see MemoryDisallowDiscard

	@param aMemory	The memory object.
	@param aIndex	Page index for start of the region.
	@param aCount	Number of pages in the region.

	@return KErrNone, if successful;
			KErrNotSupported, if the memory object doesn't support this operation;
			otherwise another of the system wide error codes.

	*/
	static TInt MemoryAllowDiscard(DMemoryObject* aMemory, TUint aIndex, TUint aCount);

	/**
	Mark a region in a memory object as no longer discardable.
	This undoes the operation of MemoryAllowDiscard.

	If any pages in the region are no longer present, then the operation will
	fail with KErrNotFound. In this case, the state of the pages in the region
	is indeterminate; they may be either still discardable, not discardable, or
	not present.

	Supported for memory object types:
	- EMemoryObjectDiscardable

	@see MemoryAllowDiscard

	@param aMemory	The memory object.
	@param aIndex	Page index for start of the region.
	@param aCount	Number of pages in the region.

	@return KErrNone, if successful;
			KErrNotFound, if any page in the region was no longer present;
			KErrNotSupported, if the memory object doesn't support this operation;
			otherwise another of the system wide error codes.
	*/
	static TInt MemoryDisallowDiscard(DMemoryObject* aMemory, TUint aIndex, TUint aCount);

	/**
	This function only exists to support DMemModelChunk::PhysicalAddress and may be removed.
	DO NOT USE.
	*/
	static TInt MemoryPhysAddr(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList);

	/**
	Prime BTrace for a memory object - internal use only.
	*/
	static void MemoryBTracePrime(DMemoryObject* aMemory);

	/**
	Close a memory object.

	This should be called on the memory object returned by #MappingGetAndOpenMemory.
	*/
	static void MemoryClose(DMemoryObject* aMemory);

	/**
	Return true if aMemory has any mappings associated with it.
	*/
	static TBool MemoryIsNotMapped(DMemoryObject* aMemory);

	/**
	Wipe the entire memory contents of the memory object so they are in the
	same state as if the memory had been newly allocated with #MemoryAlloc.

	This is useful for situations where a memory object is being re-purposed and
	confidentiality requires that the old memory contents are not disclosed.
	For this reason, the function asserts that the memory object has no mappings.

	@see EMemoryCreateUseCustomWipeByte
	@see EMemoryCreateNoWipe

	@return KErrNone, if successful;
			otherwise KErrNotSupported, to indicate the memory object doesn't support this operation.
	*/
	static TInt MemoryWipe(DMemoryObject* aMemory);

	/**
	Set the memory object to be read only, i.e. to no longer allow writable mappings.

	NOTE - This can't be undone, page moving will break if a memory object is made 
	writable again.

	@param aMemory	The memory object to update.
	@return KErrNone on success, KErrInUse if the memory object has writable mappings
			KErrNotSupported if the objects manager doesn't support this.
	*/
	static TInt MemorySetReadOnly(DMemoryObject* aMemory);

	/**
	Pins physical memory associated with the memory object and returns the physical
	memory characteristics, e.g. address, map attributes and colour

	@param aMemory	        The memory object.
	@param aPinObject	    The physical pin mapping.
	@param aIndex	        Page index for start of the region.
	@param aCount   	    Number of pages in the region.
	@param aReadOnly   	    Indicates whether memory should be pinned as read only.
	@param aAddress[out]	The value is the physical address of the first page
						    in the region.
	@param aPages[out]      If not zero, this points to an array of TPhysAddr
	                        objects. On success, this array will be filled
						    with the addresses of the physical pages which
						    contain the specified region. If aPages is
						    zero, then the function will fail with
						    KErrNotFound if the specified region is not
						    physically contiguous.
	@param aMapAttr[out]    Memory attributes defined by TMappingAttributes2.
	@param aColour[out]     The mapping colour of the first physical page.


	@return The number of pages successfully removed from the memory object.
			This gives the number of entries in the array aPages and is
			less-than or equal to aCount.
	*/

	static TInt PinPhysicalMemory(DMemoryObject* aMemory, DPhysicalPinMapping* aPinObject, TUint aIndex,
								  TUint aCount, TBool aReadOnly, TPhysAddr& aAddress, TPhysAddr* aPages,
								  TUint32& aMapAttr, TUint& aColour);

	//
	// Memory Mapping functions
	//

	/**
	Create a new memory mapping for a specific memory object.

	A memory mapping represents the MMU resources required to make a region of a
	memory object accessible to software within a single address space (process).
	Each mapping has an associated set of access permissions (see
	#TMappingPermissions).

	@param[out] aMapping Pointer reference which on success is set to the address
						of the created memory mapping.
	@param aMemory		The memory object with which the mapping is associated.
	@param aPermissions	Bitmask of values from enum #TMappingPermissions which
						give the access permissions for this mapping.
	@param aOsAsid		The identifier for the address space in which this
						mapping is to appear.
	@param aFlags		Bitmask of option flags from enum #TMappingCreateFlags
	@param aAddr		Optional virtual address at which the memory mapped
						by this mapping is to appear within the specified
						address space.
	@param aIndex		Optional start page index within \a aMemory for this
						mapping.
	@param aCount		Optional page count for size of mapping. A value of
						the maximum integer indicates that the mapping is to
						extend to the end of the memory object.

	@return KErrNone, if successful;
			otherwise another of the system wide error codes.
	*/
	static TInt MappingNew(DMemoryMapping*& aMapping, DMemoryObject* aMemory, TMappingPermissions aPermissions, TInt aOsAsid, TMappingCreateFlags aFlags=EMappingCreateDefault, TLinAddr aAddr=0, TUint aIndex=0, TUint aCount=~0u);

	/**
	Create a new memory mapping with is not associated with any memory object.

	This mapping may be used and reused with different memory objects by using
	the #MappingMap and #MappingUnmap methods.

	@param[out] aMapping	Pointer reference which on success is set to the address
							of the created memory mapping.
	@param aCount			Page count for size of mapping.
	@param aOsAsid			The identifier for the address space in which this
							mapping is to appear.
	@param aFlags			Bitmask of option flags from enum #TMappingCreateFlags
	@param aAddr			Optional virtual address at which the mapping is to
							appear within the specified address space.
	@param aColourOffset	The byte offset within a memory object's memory which this mapping
							is to start. This is used to adjust virtual memory allocation to
							meet page colouring restrictions. If this value is not known leave
							this argument unspecified; however, it must be specified if \a aAddr
							is specified.

	@return KErrNone, if successful;
			otherwise another of the system wide error codes.
	*/
	static TInt MappingNew(DMemoryMapping*& aMapping, TUint aCount, TInt aOsAsid, TMappingCreateFlags aFlags=EMappingCreateDefault, TLinAddr aAddr=0, TLinAddr aColourOffset=~0);

	/**
	Apply a memory mapping to a memory object.

	@param aMapping		The memory mapping to be used to map \a aMemory.
	@param aPermissions	Bitmask of values from enum #TMappingPermissions which
						give the access permissions for this mapping.
	@param aMemory		The memory object with which the mapping is to be associated.
	@param aIndex		Optional start page index within aMemory for this
						mapping.
	@param aCount		Optional page count for size of mapping. A value of
						the maximum integer indicates that the mapping is to
						extend to the end of the memory object.

	@return KErrNone, if successful;
			otherwise another of the system wide error codes.
	*/
	static TInt MappingMap(DMemoryMapping* aMapping, TMappingPermissions aPermissions, DMemoryObject* aMemory, TUint aIndex=0, TUint aCount=~0u);

	/**
	Remove a mapping from the memory object with which it is associated.
	*/
	static void MappingUnmap(DMemoryMapping* aMapping);

	/**
	Remove a memory mapping from its associated address space and close a
	reference on it.

	If there are no longer any other references, this will result in the memory
	object being destroyed,

	@param aMapping	Pointer reference to memory mapping to be destroyed.
					On return from the function this is set to the null-pointer.
					No action is performed if the reference was already the
					null-pointer on entry to this function.
	*/
	static void MappingDestroy(DMemoryMapping*& aMapping);

	/**
	Remove a memory mapping from its associated address space and close a
	reference on it.

	The mapping is found based on the virtual address region within which it
	maps memory. The caller must that such a mapping exists and that it will not
	be unmapped by another thread.

	@param aAddr	Virtual address which lies within the region covered by the
					memory mapping.
	@param aOsAsid	Address space in which the mapping appears.
	*/
	static void MappingDestroy(TLinAddr aAddr, TInt aOsAsid);

	/**
	Perform the action of #MappingDestroy on a memory mapping then #MemoryDestroy
	on its associated memory object.

	This function acquires and releases the memory objects lock.
	See MM::MemorySetLock.

	NOTE - This should not be used on mappings that are reused as the mapping's 
	instance count is not checked.

	@param aMapping	Pointer reference to memory mapping to be destroyed.
					On return from the function this is set to the null-pointer.
					No action is performed if the reference was already the
					null-pointer on entry to this function.
	*/
	static void MappingAndMemoryDestroy(DMemoryMapping*& aMapping);

	/**
	Perform the action of #MappingDestroy on a memory mapping then #MemoryDestroy
	on its associated memory object.

	The mapping is found based on the virtual address region within which it
	maps memory. The caller must that such a mapping exists and that it will not
	be unmapped by another thread.

	This function acquires and releases the memory objects lock.
	See MM::MemorySetLock.

	NOTE - This should not be used on mappings that are reused as the mapping's 
	instance count is not checked.

	@param aAddr	Virtual address which lies within the region covered by the
					memory mapping.
	@param aOsAsid	Address space in which the mapping appears.
	*/
	static void MappingAndMemoryDestroy(TLinAddr aAddr, TInt aOsAsid);

	/**
	Return the virtual address at which the memory mapped by a memory mapping
	starts.

	@param aMapping	A memory mapping.

	@return The base address.
	*/
	static TLinAddr MappingBase(DMemoryMapping* aMapping);

	/**
	Return the OS Address Space ID (OS ASID) of the address space the mapping is within.

	@param aMapping	A memory mapping.

	@return OS Address Space ID (OS ASID).
	*/
	static TInt MappingOsAsid(DMemoryMapping* aMapping);

	/**
	Open the memory object mapped by a mapping and return it.

	The memory object can be closed again by calling #MemoryClose.

	NOTE - This should not be used on mappings that are reused as the mapping's 
	instance count is not checked.

	@param aMapping			A memory mapping.

	@return The memory object, or NULL.
	*/
	static DMemoryObject* MappingGetAndOpenMemory(DMemoryMapping* aMapping);

	/**
	Close a mapping

	@param aMapping	A memory mapping.
	*/
	static void MappingClose(DMemoryMapping* aMapping);

	/**
	Find and open the mapping that maps a virtual address in the address space of the specified
	thread.

	The caller must close the mapping when it has finished using it.

	@param aThread The thread whose address space is to be searched.
	@param aAddr The virtual address for which the mapping is to be found.
	@param aSize The size, in bytes, of the region at aAddr.
	@param aOffsetInMapping A reference which is set to the offset, in bytes, into the
							mapping of the start address.
	@param aInstanceCount	The instance count of the found mapping.

	@return The mapping, or NULL if no mapping was found.

	@pre Calling thread must be in a critical section.
	*/
	static DMemoryMapping* FindMappingInThread(	DMemModelThread* aThread, TLinAddr aAddr, TUint aSize, 
												TUint& aOffsetInMapping, TUint& aInstanceCount);

	/**
	Find and open the mapping that maps a virtual address in the address space of the specified
	process.

	The caller must close the mapping when it has finished using it.

	@param aProcess The process whose address space is to be searched.
	@param aAddr The virtual address for which the mapping is to be found.
	@param aSize The size, in bytes, of the region at aAddr.
	@param aOffsetInMapping A reference which is set to the offset, in bytes, into the
							mapping of the start address.
	@param aInstanceCount	The instance count of the found mapping.

	@return The mapping, or NULL if no mapping was found.

	@pre Calling thread must be in a critical section.
	*/
	static DMemoryMapping* FindMappingInProcess(DMemModelProcess* aProcess, TLinAddr aAddr, TUint aSize, 
												TUint& aOffsetInMapping, TUint& aInstanceCount);

	/**
	Find and open the mapping that maps a virtual address in the address space of the specified
	process.

	The caller must close the mapping when it has finished using it.  The caller must ensure that
	the process can't be destroyed while calling this method.

	@param aOsAsid The identifier for the address space in which the mapping appears.
	@param aAddr The virtual address for which the mapping is to be found.
	@param aSize The size, in bytes, of the region at aAddr.
	@param aOffsetInMapping A reference which is set to the offset, in bytes, into the
							mapping of the start address.
	@param aInstanceCount	The instance count of the found mapping.

	@return The mapping, or NULL if no mapping was found.

	@pre Calling thread must be in a critical section.
	*/
	static DMemoryMapping* FindMappingInAddressSpace(	TUint aOsAsid, TLinAddr aAddr, TUint aSize, 
														TUint& aOffsetInMapping, TUint& aInstanceCount);


	//
	// Conversion utilities
	//

	/**
	Round a size in bytes up to the next integer multiple of the page size.

	@param aSize A size in bytes.

	@return The rounded size.
	*/
	static TUint RoundToPageSize(TUint aSize);

	/**
	Round a size in bytes up to the next integer multiple of the page size
	then convert it into a page count by dividing it by the page size.

	@param aSize A size in bytes.

	@return The rounded page count.
	*/
	static TUint RoundToPageCount(TUint aSize);

	/**
	Round a bit shift value representing the log2 of a size in bytes, up to
	a shift value representing the log2 of a size in pages.

	@param aShift log2 of a size in bytes.

	@return aShift-KPageShift, or zero if aShift<=KPageShift.
	*/
	static TUint RoundToPageShift(TUint aShift);

	/**
	Convert a size in bytes to a size in pages.

	@param aBytes A size in bytes.

	@return aBytes/KPageSize.

	@panic MemModel #EBadBytesToPages if aBytes is not an integer multiple of
		   the page size.
	*/
	static TUint BytesToPages(TUint aBytes);

	/**
	Construct a #TMappingPermissions value base on individual permission flags.

	@param aUser	True to allow unprivileged (user mode) access.
	@param aWrite	True to allow memory contents to be modified.
	@param aExecute	True to allow memory contents to be executed as code.

	@return Permissions expressed as an #TMappingPermissions value.
	*/
	static TMappingPermissions MappingPermissions(TBool aUser, TBool aWrite, TBool aExecute);

	/**
	Extract the mapping permissions from a TMappingAttributes2
	(or TMappingAttributes) value.

	@param[out] aPermissions	If successful, mapping permissions extracted from aLegacyAttributes.
	@param aLegacyAttributes	A legacy combined permission/attribute value.

	@return KErrNone, if successful;
			otherwise another of the system wide error codes.
	*/
	static TInt MappingPermissions(TMappingPermissions& aPermissions, TMappingAttributes2 aLegacyAttributes);

	/**
	Extract the memory attributes from a TMappingAttributes2
	(or TMappingAttributes) value.

	@param[out] aAttributes		If successful, memory attributes extracted from \a aLegacyAttributes.
	@param aLegacyAttributes	A legacy combined permission/attribute value.

	@return KErrNone, if successful;
			otherwise another of the system wide error codes.
	*/
	static TInt MemoryAttributes(TMemoryAttributes& aAttributes, TMappingAttributes2 aLegacyAttributes);

	/**
	Construct a legacy combined permission/memory-type value based on a
	#TMemoryAttributes and #TMappingPermissions value.

	@param aAttributes	A memory attributes value.
	@param aPermissions	A mapping permissions value.

	@return A TMappingAttributes2 value combining \a aAttributes and \a aPermissions.
	*/
	static TMappingAttributes2 LegacyMappingAttributes(TMemoryAttributes aAttributes, TMappingPermissions aPermissions);

	//
	// Code paging
	//

	/**
	Create a new memory object which will contain demand paged code.

	@param[out] aMemory	Pointer reference which on success is set to the address
						of the created memory object.
	@param aPageCount	Size of the memory object, in number of pages.
	@param aInfo		Pointer reference which on success it set to a #TPagedCodeInfo
						object which should be initialised with information required
						for demand paging the code. (Call #PagedCodeLoaded when this is done.)

	@return KErrNone, if successful;
			otherwise another of the system wide error codes.
	*/
	static TInt PagedCodeNew(DMemoryObject*& aMemory, TUint aPageCount, TPagedCodeInfo*& aInfo);

	/**
	Call this to indicate that a memory object created with #PagedCodeNew has had its
	#TPagedCodeInfo object initialised.

	@param aMemory		The memory object.
	@param aLoadAddress	An address at which \a aMemory is mapped with write permissions.
	*/
	static void PagedCodeLoaded(DMemoryObject* aMemory, TLinAddr aLoadAddress);

	//
	// Misc
	//

	// Initialisation...
	static void Init1();
	static void Init2();
	static void Init3();
	static TInt InitFixedKernelMemory(DMemoryObject*& aMemory, TLinAddr aStart, TLinAddr aEnd, TUint aInitSize, TMemoryObjectType aType, TMemoryCreateFlags aMemoryCreateFlags, TMemoryAttributes aMemoryAttributes, TMappingCreateFlags aMappingCreateFlags);
	static TInt MemoryClaimInitialPages(DMemoryObject* aMemory, TLinAddr aBase, TUint aSize, TMappingPermissions aPermissions, TBool aAllowGaps=false, TBool aAllowNonRamPages=false);

	// IPC helpers...
	static void ValidateLocalIpcAddress(TLinAddr aAddr, TUint aSize, TBool aWrite);
#ifndef __SMP__
	static void IpcAliasPde(TPde*& aPdePtr, TUint aOsAsid);
#endif
	static void UserPermissionFault(TLinAddr aAddr, TBool aWrite);

	// Address space...
	static TInt AddressSpaceAlloc(TPhysAddr& aPageDirectory);
	static void AddressSpaceFree(TUint aOsAsid);
	static void AsyncAddressSpaceFree(TUint aOsAsid);
	static TInt VirtualAllocCommon(TLinAddr& aLinAddr, TUint aSize, TBool aDemandPaged);
	static void VirtualFreeCommon(TLinAddr aLinAddr, TUint aSize);
	static TInt VirtualAlloc(TInt aOsAsid, TLinAddr& aLinAddr, TUint aSize, TBool aDemandPaged);
	static void VirtualFree(TInt aOsAsid, TLinAddr aLinAddr, TUint aSize);

#ifdef _DEBUG
	/**
	Force a region of paged memory to be paged out.

	If the memory is not paged this call has no effect.

	@return KErrNone, or KErrBadDescriptor if a single mapping containing the region could not be
	        found.
	*/
	static TInt FlushRegion(DMemModelProcess*, TLinAddr aStartAddress, TUint aSize);
#endif

	/**
	Enumeration of panic values for category "MemModel".
	*/
	enum TMemModelPanic
		{
		EFsRegisterThread,
		EProcessDestructChunksRemaining,
		ECommitInvalidDllDataAddress,
		ECodeSegLoadedNotCreator,
		ETempMappingAlreadyInUse,
		EUnsupportedOperation,
		EBadBytesToPages,
		ECodeSegSetReadOnlyFailure,
		EProcessDestructOsAsidRemaining,
		};
	static void Panic(TMemModelPanic aPanic);
	};



template <class T>
FORCE_INLINE void FlagSet(T& a,const T b)
	{ a = (T)(a|b); }

template <class T>
FORCE_INLINE void FlagSet(T& a,const T b,const T c)
	{ a = (T)(a|b|c); }

template <class T>
FORCE_INLINE void FlagSet(T& a,const T b,const T c,const T d)
	{ a = (T)(a|b|c|d); }

template <class T>
FORCE_INLINE void FlagClear(T& a,const T b)
	{ a = (T)(a&~b); }

template <class T>
FORCE_INLINE void FlagClear(T& a,const T b,const T c)
	{ a = (T)(a&~b&~c); }

template <class T>
FORCE_INLINE void FlagClear(T& a,const T b,const T c,const T d)
	{ a = (T)(a&~b&~c&~d); }

/// Utility function to calculate the minimum of two unsigned integers
FORCE_INLINE TUint MinU(TUint a, TUint b)
	{ return a <= b ? a : b; }

/// Utility function to calculate the maximum of two unsigned integers
FORCE_INLINE TUint MaxU(TUint a, TUint b)
	{ return a >= b ? a : b; }


#include <memmodel/epoc/mmubase/kblockmap.h>

/**
Structure containing the information about a demand paged code segment
which is required to load and fixup its code section.
*/
class TPagedCodeInfo
	{
public:
	~TPagedCodeInfo();
	TInt ReadBlockMap(const TCodeSegCreateInfo& aInfo);
	TInt ReadFixupTables(const TCodeSegCreateInfo& aInfo);
	void ApplyFixups(TLinAddr aBuffer, TUint iIndex);
public:
	TBool iLoaded;						///< True once initial loading has finished and paging should start applying fixups.
	TUint iCodeRelocTableSize;			///< Size, in bytes, of #iCodeRelocTable.
	TUint8* iCodeRelocTable;			///< Code relocation information.
	TUint iImportFixupTableSize;		///< Size, in bytes, of #iImportFixupTable.
	TUint8* iImportFixupTable;			///< Import fixup information.
	TUint32 iCodeDelta;					///< Delta to apply to relocate words referring to the code section.
	TUint32 iDataDelta;					///< Delta to apply to relocate words referring to the data section.

	TUint iCodeSize;					///< Size, in bytes, of the code section.
	TUint32 iCompressionType;			///< Compression scheme in use, (KUidCompressionBytePair or KFormatNotCompressed).
	TInt32* iCodePageOffsets;			///< Array of compressed page offsets within the file.
	TInt iCodeLocalDrive;				///< Local drive number.
	TInt iCodeStartInFile;				///< Offset of (possibly compressed) code from start of file.
	TBlockMap iBlockMap;				///< Kernel-side representation of block map.
	};



/**
A pool of DMutex objects that can be dynamically assigned for use.

This is intended as a memory saving alternative to having each object in a class of objects
owning their own mutex for locking purposes and provides a concurrency improvement over using
a single global lock.
*/
class DMutexPool : public DBase
	{
public:
	/**
	@param aCount	Number of mutexes to allocated in the pool.
					Must be smaller than #EMaxPoolSize.
	@param aName	Base name for mutexes. Each mutex in the pool has a number appended to this base name.
	@param aOrder	A value representing the order of the mutex with respect to deadlock prevention.
	*/
	TInt Create(TUint aCount, const TDesC* aName, TUint aOrder);

	~DMutexPool();

	/**
	Wait on the mutex specified by \a aMutexRef.

	If \a aMutexRef contains the null pointer then a member of this pool is selected and waited on,
	and \a aMutexRef is set to a cookie value identifying that mutex; this cookie has its least
	significant bit set to distinguish it from a normal DMutex pointer. Subsequent concurrent
	Wait operations will use the same pool mutex; possibly modifying the value of the cookie.
	*/
	void Wait(DMutex*& aMutexRef);

	/**
	Signal the mutex specified by \a aMutexRef.

	If mutex is identified as one from this pool then the value of \a aMutexRef will be modified
	and if there are no pending Wait operations it will be set to the null pointer to indicate
	that no mutex is assigned.
	*/
	void Signal(DMutex*& aMutexRef);

	/**
	Return true if the mutex specified by \a aMutexRef is held by the current thread.
	*/
	TBool IsHeld(DMutex*& aMutexRef);

	enum
		{
		/** Maximum number of mutexes allowed in a pool. */
		EMaxPoolSize = 64
		};

private:
	/** Structure for storing information about each member of the pool. */
	struct SMember
		{
		DMutex* iMutex;		///< The mutex
		TUint iUseCount;	///< Number of times this mutex has been used
		};
	TUint iCount;		///< Number of mutexes in pool. (Number of entries in iMembers).
	TUint iNext;		///< Index of next pool mutex to use.
	SMember* iMembers;	///< Info about each memory of pool.
	};


#endif
