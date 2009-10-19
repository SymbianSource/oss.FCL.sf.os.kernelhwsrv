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

#ifndef MVALLOC_H
#define MVALLOC_H


class RVirtualAllocSlabSet;

/**
Allocator for virtual addresses.

The allocator has the concept of addresses having a 'slab type' (#TVirtualSlabType).
It ensures that addresses of different slab types will not overlap in the same 'chunk'
(the region covered by a single MMU page table).

Addresses will be allocated from lower address regions first, subject to slab type
and allocation algorithm constraints. See #RBackwardsVirtualAllocator.
*/
class RVirtualAllocator
	{
public:
	RVirtualAllocator();
	~RVirtualAllocator();

	/**
	Second phase constructor.

	@param aStart			The starting virtual address of the region to be covered
							by this allocator.
							Must be an integer multiple of #KVirtualAllocSlabSize.
	@param aEnd				The end virtual address (last valid address plus one) of the region
							to be covered by this allocator.
							Must be an integer multiple of #KVirtualAllocSlabSize.
	@param aNumSlabTypes	The number of different 'slab types' to be allocated.
							This will normally be #ENumVirtualAllocTypes.
	@param aWriteLock		Reference to the mutex which is being used to protect allocations
							with this object. This is only used for debug checks and may be
							a mutex assigned by #DMutexPool. In practice, this will usually be an
							address space lock DAddressSpace::iLock.

	@return KErrNone if successful, otherwise one of the system wide error codes.
	*/
	TInt Construct(TLinAddr aStart, TLinAddr aEnd, TUint aNumSlabTypes, DMutex*& aWriteLock);

	/**
	Allocate a region of virtual addresses.

	The returned region may have a start address and/or size which is different to
	those requested due to various alignment requirements in the implementation.
	However the returned region will always include all addresses requested.

	@param[out] aAddr			Returns the start address of the region which was allocated.
	@param[out] aSize			Returns the size, in bytes, of the region which was allocated.
								This will always be aligned to a multiple of the page colouring
								size: #KPageColourCount*#KPageSize.
	@param		aRequestedAddr	The requested start address of the region to allocate,
								or zero if no specific address is required.
	@param		aRequestedSize	The requested size, in bytes, of the region to allocate.
	@param		aSlabType		The 'slab type' of the address to be allocated.
								Addresses of different slab types will not overlap in the
								same 'chunk' (region covered by a single MMU page table).
								This value must be less than the \a aNumSlabTypes argument
								used in #Construct.

	@return KErrNone if successful.
			KErrAlreadyExists if a specific address was supplied and this was already
			allocated, or exists in a slab already used for a different slab type.
			Otherwise, one of the system wide error codes.

	@pre The write lock must be held. (See \a aWriteLock argument for #Construct.)
	*/
	TInt Alloc(TLinAddr& aAddr, TUint& aSize, TLinAddr aRequestedAddr, TUint aRequestedSize, TUint aSlabType);

	/**
	Free a virtual addresses region which was allocated with #Alloc.
	The region supplied to this function must either be one supplied to a
	previous call to #Alloc or be one returned by that function.

	@param aAddr	Start address of the region to be freed.
	@param aSize	Size, in bytes, of the region to be freed.

	@pre The write lock must be held. (See \a aWriteLock argument for #Construct.)
	*/
	void Free(TLinAddr aAddr, TUint aSize);

	/**
	Return true if the the address region specified by \a aAddr and \a aSize is
	entirely within the region of addresses covered by this allocator.
	*/
	TBool InRange(TLinAddr aAddr, TUint aSize);

	/**
	Return true if the the address region specified by \a aAddr and \a aSize was
	allocated by this allocator using the specified \a aSlabType.

	@pre The write lock must be held. (See \a aWriteLock argument for #Construct.)
	*/
	TBool CheckSlabType(TLinAddr aAddr, TUint aSize, TUint aSlabType);

private:
	/**
	If required, expand the region specified by \a aAddr and \a aSize 
	to meet size and alignment requirements of the allocator.
	This also returns log2 of the address alignment required.
	*/
	static TUint AdjustRegion(TLinAddr& aAddr, TUint& aSize);

protected:
	/**
	The starting virtual address of the region covered by this allocator.
	*/
	TLinAddr iBase;

	/**
	The size, in bytes, of the virtual address of the region covered by this allocator.
	*/
	TUint iSize;

private:
	/**
	Bitmap of used virtual address regions, each a 'slab' size (#KVirtualAllocSlabSize).
	*/
	TBitMapAllocator* iAllocator;

	/**
	Pointer to allocator object used for sizes less than #KVirtualAllocSlabSize.
	*/
	RVirtualAllocSlabSet* iSlabSet;
	};


inline TBool RVirtualAllocator::InRange(TLinAddr aAddr, TUint aSize)
	{
	aAddr -= iBase;
	return aAddr<iSize && aAddr+aSize>=aAddr && aAddr+aSize<=iSize;
	}



/**
Allocator for virtual addresses which is identical to #RVirtualAllocator
except that addresses will be allocated from higher address regions first.
(Subject to 'slab type' and allocation algorithm constraints).
*/
class RBackwardsVirtualAllocator : public RVirtualAllocator
	{
public:
	// overriding RVirtualAllocator...
	TInt Alloc(TLinAddr& aAddr, TUint& aSize, TLinAddr aRequestedAddr, TUint aRequestedSize, TUint aSlabType);
	void Free(TLinAddr aAddr, TUint aSize);
	};


/**
Enumeration of the different virtual address allocation types which may not
overlap in the same 'chunk' (region covered by a single MMU page table).

This includes all #TPdeType values, plus addition address types.
*/
enum TVirtualSlabType
	{
	/**
	Bit flag used to distinguish common virtual addresses allocated with
	DAddressSpace::AllocateUserCommonVirtualMemory.

	It is important that these addresses reside in their own slab type,
	otherwise normal local address allocation would tend to get allocated
	adjacent to them; clogging up the 'common' address region.
	*/
	EVirtualSlabTypeCommonVirtual		= ENumPdeTypes<<0,

	/**
	Bit flag used to distinguish virtual addresses allocated for use in
	mapping demand paged memory.

	This ensures that page tables used for demand paged memory are not
	used for other memory types and means they may be freed once the
	memory is paged out.
	*/
	EVirtualSlabTypeDemandPaged			= ENumPdeTypes<<1,

	/**
	Total number of different 'kinds' of virtual address which may need to be allocated.
	*/
	ENumVirtualAllocTypes				= ENumPdeTypes<<2
	};


#endif
