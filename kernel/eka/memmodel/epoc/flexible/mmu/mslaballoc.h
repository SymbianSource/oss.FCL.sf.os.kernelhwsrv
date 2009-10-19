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

#ifndef MSLABALLOC_H
#define MSLABALLOC_H

#include "mcleanup.h"

/**
Slab allocator.
*/
class RSlabAllocatorBase
	{
public:
	RSlabAllocatorBase(TBool aDelayedCleanup);
	~RSlabAllocatorBase();

	/**
	Construct a slab allocator.

	@param aMaxSlabs	Maximum number of slabs to use. (Number of bits in \a slabBits.)
	@param aObjectSize	Size of objects to allocate.
	*/
	TInt Construct(TUint aMaxSlabs, TUint aObjectSize);

	/**
	Construct a slab allocator using fixed virtual memory.

	@param aMaxSlabs	Maximum number of slabs to use. (Number of bits in \a slabBits.)
	@param aObjectSize	Size of objects to allocate.
	@param aBase		Virtual address for start of memory where slabs will be allocated.
						Zero indicates 'anywhere'.
	*/
	TInt Construct(TUint aMaxSlabs, TUint aObjectSize, TLinAddr aBase);

	/**
	Set the memory object to be used for the slab allocator.
	*/
	FORCE_INLINE void SetMemory(DMemoryObject* aMemory, TUint aReserveCount)
		{
		__NK_ASSERT_DEBUG(!iMemory);
		iMemory = aMemory;
		iReserveCount = aReserveCount;
		}

	/**
	Allocate an object.

	@return Allocated object, or the null pointer if there is insufficient memory to perform the allocation.

	@pre MmuLock held
	@post MmuLock held, but has always beet flashed
	*/
	TAny* Alloc();

	/**
	Free an object previously allocated with #Alloc.

	@param aObject The object.

	@pre MmuLock held
	@post MmuLock held, but has always beet flashed
	*/
	void Free(TAny* aObject);

protected:
	class TSlabHeader : public SDblQueLink
		{
	public:
		SDblQue iFreeList;		///< List of unallocated objects in list.
		TUint iAllocCount;		///< Number of objects allocated in this slab.
		TAny* iHighWaterMark;	///< End of initialise region in slab.
		};

private:
	TBool NewSlab();
	void InitSlab(TLinAddr aPage);
	void FreeSlab(TSlabHeader* aSlab);
	static void CleanupTrampoline(TAny* aSelf);
	void Cleanup();
#ifdef _DEBUG
	void CheckSlab(TSlabHeader* aSlab);
#endif

private:
	SDblQue iFreeList;			///< List of slabs which have unallocated objects in them.
	TUint iFreeCount;			///< Number of unallocated objects.
	TUint iReserveCount;		///< Number of unallocated objects to keep in reserve (to allow for recursion during allocation).
	TUint iObjectsPerSlab;		///< Number of objects in each slab.
	TUint iObjectSize;			///< Size, in bytes, of objects to be allocated.
	TSpinLock iSpinLock;		///< Spinlock which protects iFreeList, iFreeCount  and TSlabHeader contents.

	TMemoryCleanup iCleanup;	///< Used to queue Cleanup() if iDelayedCleanup is true.
	TBool iDelayedCleanup;		///< True, if Free() should not free empty slabs.

	TBool iAllocatingSlab;		///< True if a new slab page is being allocated.
	TBitMapAllocator* iSlabMap;	///< Bitmap of allocated slabs.
	DMemoryObject* iMemory;		///< The memory object used to store slabs.
	DMemoryMapping* iMapping;	///< The memory mapping used for slabs.
	TLinAddr iBase;				///< Address of first slab.
	};


/**
Template for a slab allocator which can allocate up to N objects of type T.
*/
template <class T, TUint N>
class RSlabAllocator : public RSlabAllocatorBase
	{
public:
	enum
		{
		EObjectSize = sizeof(T)>sizeof(SDblQueLink) ? sizeof(T) : sizeof(SDblQueLink),
		EObjectsPerSlab = (KPageSize-sizeof(TSlabHeader))/EObjectSize,
		EMaxSlabs = (N+EObjectsPerSlab-1)/EObjectsPerSlab
		};

	FORCE_INLINE RSlabAllocator()
		: RSlabAllocatorBase(EFalse)
		{
		__ASSERT_COMPILE(EObjectsPerSlab>0);
		}

	FORCE_INLINE TInt Construct()
		{
		return RSlabAllocatorBase::Construct(EMaxSlabs,EObjectSize);
		}

	/**
	Allocate an object.

	@return Allocated object, or the null pointer if there is insufficient memory to perform the allocation.
	*/
	FORCE_INLINE T* Alloc()
		{
		return (T*)RSlabAllocatorBase::Alloc();
		}

	/**
	Free an object previously allocated with #Alloc.

	@param aObject Object.
	*/
	FORCE_INLINE void Free(T* aObject)
		{
		RSlabAllocatorBase::Free(aObject);
		}
	};


/**
Template for a slab allocator for allocating objects of type T, using the
virtual address region [B..E)
*/
template <class T, TLinAddr B, TLinAddr E>
class RStaticSlabAllocator : public RSlabAllocatorBase
	{
public:
	enum
		{
		EObjectSize = sizeof(T)>sizeof(SDblQueLink) ? sizeof(T) : sizeof(SDblQueLink),
		EObjectsPerSlab = (KPageSize-sizeof(TSlabHeader))/EObjectSize,
		EMaxSlabs = (E-B)/KPageSize
		};

	FORCE_INLINE RStaticSlabAllocator()
		: RSlabAllocatorBase(ETrue)
		{
		__ASSERT_COMPILE(EObjectsPerSlab>0);
		}

	FORCE_INLINE TInt Construct()
		{
		return RSlabAllocatorBase::Construct(EMaxSlabs,EObjectSize,B);
		}

	/**
	Allocate an object.

	@return Allocated object, or the null pointer if there is insufficient memory to perform the allocation.
	*/
	FORCE_INLINE T* Alloc()
		{
		return (T*)RSlabAllocatorBase::Alloc();
		}

	/**
	Free an object previously allocated with #Alloc.

	@param aObject Object.
	*/
	FORCE_INLINE void Free(T* aObject)
		{
		RSlabAllocatorBase::Free(aObject);
		}
	};

#endif

