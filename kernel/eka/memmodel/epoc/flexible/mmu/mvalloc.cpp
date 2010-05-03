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

#include <plat_priv.h>
#include "mm.h"
#include "mmu.h"
#include "mvalloc.h"
#include "maddrcont.h"


/**
Log2 of the minimum granularity and alignment of virtual address allocation.
Must be greater than or equal to #KPageShift+#KPageColourShift.
*/
const TUint KVirtualAllocShift = KPageShift+KPageColourShift;

/**
Log2 of the size of the region covered by a single 'slab' of virtual addresses.
Must be greater than or equal to KChunkShift.
*/
const TUint KVirtualAllocSlabShift = KChunkShift;

/**
Size, in bytes, of the size of the region covered by a single 'slab' of virtual addresses.
*/
const TUint KVirtualAllocSlabSize = 1<<KVirtualAllocSlabShift;

const TUint KVirtualAllocSlabMask = KVirtualAllocSlabSize-1;

__ASSERT_COMPILE(KVirtualAllocShift>=KPageShift+KPageColourShift);
__ASSERT_COMPILE(KVirtualAllocSlabShift>=TUint(KChunkShift));


#if defined(__GCCXML__)
FORCE_INLINE TUint CountLeadingZeroes(TUint32 /*aValue*/)
	{
	// empty
	return 0;
	}

#elif defined(__MARM__)

#ifdef __ARMCC__
FORCE_INLINE TUint CountLeadingZeroes(TUint32 aValue)
	{
	#if __ARMCC_VERSION < 310000
		TUint r;
		asm("clz r,aValue");
		return r;
	#else
		// Inline assembler is deprecated in RVCT 3.1 so we use an intrinsic.
		return __clz(aValue);
	#endif
	}
#endif // __ARMCC__

#ifdef __MARM_ARM4__
__declspec(naked) static TUint CountLeadingZeroes(TUint32)
	{
	CLZ(0,0);
	__JUMP(,lr);
	}

#elif defined(__GNUC__)
FORCE_INLINE TUint CountLeadingZeroes(TUint32 aValue)
	{
	TUint r;
	asm("clz %0,%1" : "=r"(r) : "r"(aValue));
	return r;
	}
#endif //  __GNUC__

#else // !__MARM__

inline TUint CountLeadingZeroes(TUint32 aValue)
	{
	if(!aValue)
		return 32;
	TUint count = 31;
	if(aValue>=(1<<16))
		{
		count -= 16;
		aValue >>= 16;
		}
	if(aValue>=(1<<8))
		{
		count -= 8;
		aValue >>= 8;
		}
	if(aValue>=(1<<4))
		{
		count -= 4;
		aValue >>= 4;
		}
	if(aValue>=(1<<2))
		{
		count -= 2;
		aValue >>= 2;
		}
	count -= aValue>>1;
	return count;
	}

#endif // __MARM__



//
// TLogAllocator
//

/**
Bitmap allocator for allocating regions which have size and alignment which
are a power-of-two.
*/
class TLogAllocator
	{
public:
	TLogAllocator();

	/**
	Find and allocate a free region in the bitmap.

	@param aSizeShift	Log2 of the number of bits to allocate.

	@return If successful, the index of the first bit allocated.
			Otherwise, -1.
	*/
	TInt Alloc(TUint aSizeShift);

	/**
	Allocate a specific region of bits.

	@param aIndex		The index of the first bit to allocated.
						Must be a integer multiple of 2^aSizeShift.
	@param aSizeShift	Log2 of the number of bits to allocate.

	@return KErrNone, if successful;
			KErrAlreadyExists, if any part of the region was already allocated.
	*/
	TInt Alloc(TUint aIndex, TUint aSizeShift);

	/**
	Free a specific region of bits.

	@param aIndex		The index of the first bit to free.
						Must be a integer multiple of 2^aSizeShift.

	@param aSizeShift	Log2 of the number of bits to free.

	@return True, if the slab no longer has any bits allocated.
	*/
	TBool Free(TUint aIndex, TUint aSizeShift);
private:
	enum
		{
		ENumBits = 1<<(KVirtualAllocSlabShift-KVirtualAllocShift),
		ENumWords = (ENumBits+31)/32
		};

	/**
	Number of bits which have been allocated.
	*/
	TUint iAllocCount;

	/**
	Bitmap where a bit set to one indicates 'free' and a bit cleared to zero
	indicates 'allocated'. The most significant bit in each word has the lowest
	index value. E.g.
	- Index 0 is bit 31 of iBits[0]
	- Index 31 is bit 0 of iBits[0]
	- Index 32 is bit 31 of iBits[1]
	*/
	TUint32 iBits[ENumWords];
	};


TLogAllocator::TLogAllocator()
	{
	iAllocCount = 0;
	memset(iBits,~0u,sizeof(iBits)); // unallocated bits are set to one
	}


TInt TLogAllocator::Alloc(TUint aSizeShift)
	{
	TUint size = 1<<aSizeShift;

	__NK_ASSERT_DEBUG(size<=ENumBits); // check in range

	TUint32* bits = iBits;
	TUint32* bitsEnd = bits+ENumWords;
	TUint32 b;
	switch(aSizeShift)
		{
	case 0: // find word with any unallocated bits...
		do
			{
			b = *bits++;
			if(b)
				goto small_found;
			}
		while(bits<bitsEnd);
		break;

	case 1: // find word with 2 adjacent unallocated bits...
		do
			{
			b = *bits++;
			b &= b<<1;
			b &= 0xaaaaaaaa;
			if(b)
				goto small_found;
			}
		while(bits<bitsEnd);
		break;

	case 2: // find word with 4 adjacent unallocated bits...
		do
			{
			b = *bits++;
			b &= b<<1;
			b &= b<<2;
			b &= 0x88888888;
			if(b)
				goto small_found;
			}
		while(bits<bitsEnd);
		break;

	case 3: // find word with 8 adjacent unallocated bits...
		do
			{
			b = *bits++;
			b &= b<<1;
			b &= b<<2;
			b &= b<<4;
			b &= 0x80808080;
			if(b)
				goto small_found;
			}
		while(bits<bitsEnd);
		break;

	case 4: // find word with 16 adjacent unallocated bits...
		do
			{
			b = *bits++;
			b &= b<<1;
			b &= b<<2;
			b &= b<<4;
			b &= b<<8;
			b &= 0x80008000;
			if(b)
				goto small_found;
			}
		while(bits<bitsEnd);
		break;

	case 5: // find word which is totally unallocated (has 32 bits free)...
		do
			{
			b = *bits++;
			if(b==0xffffffffu)
				goto big_found;
			}
		while(bits<bitsEnd);
		break;

	default: // find relevant number of words which are unallocated...
		{
		do
			{
			// AND words together...
			TUint32* end = (TUint32*)((TUint8*)bits+(size>>3));
			TUint32 b = 0xffffffffu;
			do b &= *bits++;
			while(bits<end);

			if(b==0xffffffffu)
				goto big_found; // all were free
			}
		while(bits<bitsEnd);
		break;
		}

		}
	__NK_ASSERT_DEBUG(bits==bitsEnd);
	return -1;

small_found:
	{
	// find first position in word which have free region (a bit set to one)...
	TUint offset = CountLeadingZeroes(b);

	// clear bits...
	TUint32 mask = 0xffffffffu;
	mask >>= size;
	mask = ~mask;
	mask >>= offset;
	*--bits &= ~mask;

	// calculate index for allocated region...
	TUint index = (bits-iBits)*32+offset;

	iAllocCount += size;
	return index;
	}

big_found:
	{
	// clear bits...
	TUint32* start = (TUint32*)((TUint8*)bits-(size>>3));
	do *--bits = 0;
	while(bits>start);

	// calculate index for allocated region...
	TUint index = (bits-iBits)*32;

	iAllocCount += size;
	return index;
	}

	}


TInt TLogAllocator::Alloc(TUint aIndex, TUint aSizeShift)
	{
	TUint size = 1<<aSizeShift;

	__NK_ASSERT_DEBUG(aIndex+size>aIndex); // check overflow
	__NK_ASSERT_DEBUG(aIndex+size<=ENumBits); // check in range
	__NK_ASSERT_DEBUG(((aIndex>>aSizeShift)<<aSizeShift)==aIndex); // check alignment

	TUint32* bits = iBits+(aIndex>>5);
	if(size<32)
		{
		TUint32 mask = 0xffffffffu;
		mask >>= size;
		mask = ~mask;
		mask >>= aIndex&31;
		TUint32 b = *bits;
		if((b&mask)!=mask)
			return KErrAlreadyExists;
		*bits = b&~mask;
		}
	else
		{
		TUint32* start = bits;
		TUint32* end = bits+(size>>5);
		do if(*bits++!=0xffffffffu) return KErrAlreadyExists;
		while(bits<end);

		bits = start;
		do *bits++ = 0;
		while(bits<end);
		}

	iAllocCount += size;
	return KErrNone;
	}


TBool TLogAllocator::Free(TUint aIndex, TUint aSizeShift)
	{
	TUint size = 1<<aSizeShift;

	__NK_ASSERT_DEBUG(aIndex+size>aIndex); // check overflow
	__NK_ASSERT_DEBUG(aIndex+size<=ENumBits); // check in range
	__NK_ASSERT_DEBUG(((aIndex>>aSizeShift)<<aSizeShift)==aIndex); // check alignment

	TUint32* bits = iBits+(aIndex>>5);
	if(size<32)
		{
		TUint32 mask = 0xffffffffu;
		mask >>= size;
		mask = ~mask;
		mask >>= aIndex&31;
		TUint32 b = *bits;
		__NK_ASSERT_DEBUG((b&mask)==0); // check was allocated
		*bits = b|mask;
		}
	else
		{
		TUint wordCount = size>>5;
		do
			{
			__NK_ASSERT_DEBUG(bits[0]==0);
			*bits++ = 0xffffffffu;
			}
		while(--wordCount);
		}

	iAllocCount -= size;
	return !iAllocCount;
	}



//
// TVirtualSlab
//

/**
Class for allocating virtual addresses contained in a single 'slab'.
@see RVirtualAllocSlabSet.
*/
class TVirtualSlab
	{
public:
	/**
	@param aHead		The head of a linked list of slabs to which this one should be added.
	@param aBase		The starting virtual address of the region covered by this slab.
	@param aSlabType	The 'slab type'.
	*/
	TVirtualSlab(SDblQue& aHead, TUint aBase, TUint aSlabType);

	~TVirtualSlab();

	/**
	Find an allocate a free region of virtual addresses.

	@param aSizeShift	Log2 of the size, in bytes, of the region.

	@return If successful, the allocated virtual address.
			Otherwise, 0 (zero).
	*/
	TLinAddr Alloc(TUint aSizeShift);

	/**
	Allocate a specific region of virtual addresses.

	@param aAddr		The start address of the region.
						Must be a integer multiple of 2^aSizeShift.
	@param aSizeShift	Log2 of the size, in bytes, of the region.


	@return KErrNone, if successful;
			KErrAlreadyExists, if any part of the region was already allocated.
	*/
	TInt Alloc(TLinAddr aAddr, TUint aSizeShift);

	/**
	Free a specific region of virtual addresses.

	@param aAddr		The start address of the region.
						Must be a integer multiple of 2^aSizeShift.
	@param aSizeShift	Log2 of the size, in bytes, of the region.

	@return True, if the slab no longer has any addresses allocated.
	*/
	TBool Free(TLinAddr aAddr, TUint aSizeShift);

	/**
	Return the starting virtual address of the region covered by this slab.
	*/
	FORCE_INLINE TLinAddr Base() { return iBase; }

	/**
	Return this objects 'slab type'. 
	*/
	FORCE_INLINE TUint SlabType() { return iSlabType; }
private:
	/**
	Link object used to insert this slab into lists.
	*/
	SDblQueLink iLink;

	/**
	The starting virtual address of the region covered by this slab.
	*/
	TLinAddr iBase;

	/**
	This objects 'slab type'. 
	*/
	TUint8 iSlabType;

	/**
	Bitmap allocator used to allocated pages in this slab's virtual address region.
	*/
	TLogAllocator iAllocator;

	friend class RVirtualAllocSlabSet;
	};


TVirtualSlab::TVirtualSlab(SDblQue& aHead, TUint aBase, TUint aSlabType)
	: iBase(aBase),iSlabType(aSlabType)
	{
	TRACE2(("TVirtualSlab::TVirtualSlab(?,0x%08x,%d)",aBase, aSlabType));
	aHead.Add(&iLink);
	}


TVirtualSlab::~TVirtualSlab()
	{
	TRACE2(("TVirtualSlab::~TVirtualSlab base=0x%08x",iBase));
	iLink.Deque();
	}


TLinAddr TVirtualSlab::Alloc(TUint aSizeShift)
	{
	TRACE2(("TVirtualSlab::Alloc(%d)",aSizeShift));
	__NK_ASSERT_DEBUG(aSizeShift>=KVirtualAllocShift);
	aSizeShift -= KVirtualAllocShift;
	TInt index = iAllocator.Alloc(aSizeShift);
	TLinAddr addr = 0;
	if(index>=0)
		addr = iBase+(index<<KVirtualAllocShift);
	TRACE2(("TVirtualSlab::Alloc returns 0x%08x",addr));
	return addr;
	}


TInt TVirtualSlab::Alloc(TLinAddr aAddr, TUint aSizeShift)
	{
	TRACE2(("TVirtualSlab::Alloc(0x%08x,%d)",aAddr,aSizeShift));
	__NK_ASSERT_DEBUG(aSizeShift>=KVirtualAllocShift);
	aSizeShift -= KVirtualAllocShift;
	TUint index = (aAddr-iBase)>>KVirtualAllocShift;
	__NK_ASSERT_DEBUG(iBase+(index<<KVirtualAllocShift)==aAddr);
	TInt r = iAllocator.Alloc(index,aSizeShift);
	if(r<0)
		return r;
	TRACE2(("TVirtualSlab::Alloc returns 0x%08x",iBase+(r<<KVirtualAllocShift)));
	return r;
	}


TBool TVirtualSlab::Free(TLinAddr aAddr, TUint aSizeShift)
	{
	TRACE2(("TVirtualSlab::Free(0x%08x,%d)",aAddr,aSizeShift));
	__NK_ASSERT_DEBUG(aSizeShift>=KVirtualAllocShift);
	aSizeShift -= KVirtualAllocShift;
	TUint offset = aAddr-iBase;
	TUint index = offset>>KVirtualAllocShift;
	__NK_ASSERT_DEBUG((index<<KVirtualAllocShift)==offset);
	return iAllocator.Free(index,aSizeShift);
	}


//
// RVirtualAllocSet
//


/**
Class used by #RVirtualAllocator for allocating virtual addresses which
have a size less than a 'chunk' (#KChunkSize).

This consists of a set of #TVirtualSlab objects.
*/
class RVirtualAllocSlabSet
	{
public:
	/**
	Create a new slab set for use with the specified allocator.

	@param aAllocator		The virtual address allocator which will use the slab set.
	@param aNumSlabTypes	The number of slab types this allocator will support.
	@param aWriteLock		Reference to the mutex which is being used to protect allocations
							with this object. This is only used for debug checks and may be
							a mutex assigned by #DMutexPool. In practice, this will usually be an
							address space lock DAddressSpace::iLock.

	@return The newly created #RVirtualAllocSlabSet or the null pointer if there was
			insufficient memory.
	*/
	static RVirtualAllocSlabSet* New(RVirtualAllocator* aAllocator, TUint aNumSlabTypes, DMutex*& aWriteLock);

	~RVirtualAllocSlabSet();

	/**
	Allocate a region of virtual addresses.

	@param[in,out] aAddr	On entry, if this is non-zero it represents
							the start address a specific region to allocate.
							On exit, this is set to the start address of the region allocated.
	@param aSizeShift		Log2 of the size, in bytes, of the region.
	@param aSlabType		The 'slab type' of the address to be allocated.		

	@return KErrNone, if successful;
			KErrAlreadyExists, if any part of the region was already allocated.

	@pre The write lock must be held. (The \a aWriteLock argument for the constructor
		 #RVirtualAllocSlabSet::RVirtualAllocSlabSet.)
	*/
	TInt Alloc(TLinAddr& aAddr, TUint aSizeShift, TUint aSlabType);

	/**
	Free a region of virtual addresses.

	@param aAddr			The start address of the region.
	@param aSizeShift		Log2 of the size, in bytes, of the region.

	@pre The write lock must be held. (The \a aWriteLock argument for the constructor
		 #RVirtualAllocSlabSet::RVirtualAllocSlabSet.)
	*/
	void Free(TLinAddr aAddr, TUint aSizeShift);

	/**
	Return true if the the address region specified by \a aAddr and \a aSizeShift was
	allocated by this allocator using the specified \a aSlabType.

	@pre The write lock must be held. (The \a aWriteLock argument for the constructor
		 #RVirtualAllocSlabSet::RVirtualAllocSlabSet.)
	*/
	TBool CheckSlabType(TLinAddr aAddr, TUint aSizeShift, TUint aSlabType);

private:
	/**
	Create a new slab (#TVirtualSlab) for use by this slab set.
	Newly allocated slabs are added to #iLists[\a aSlabType].

	The virtual address range used by the slab is obtained by
	by allocating a slab sized region from #iAllocator.

	@param aAddr		A virtual address which must be in the region to be covered by the slab.
	@param aSlabType	The 'slab type'.
	*/
	TVirtualSlab* NewSlab(TLinAddr aAddr, TUint aSlabType);

	/**
	Delete a slab created with #NewSlab.
	*/
	void DeleteSlab(TVirtualSlab* aSlab);

	/**
	Constructor, for arguments see #New.
	*/
	RVirtualAllocSlabSet(RVirtualAllocator* aAllocator, TUint aNumSlabTypes, DMutex*& aWriteLock);

private:
	/**
	The virtual allocator which is using this slab set.
	*/
	RVirtualAllocator* iAllocator;

	/**
	Container for all slabs owned by this slab set. This is keyed on the starting
	virtual address of the region each slab covers.

	Each slab in this container is also linked into the #iLists member appropriate
	to its slab type..
	*/
	RAddressedContainer iSlabs;

	/**
	The number of different 'slab types' this object can allocate addresses for.
	*/
	TUint iNumSlabTypes;

	/**
	An array of lists which each contain slabs of a single 'slab type'
	which this object has created. Slabs are linked by their TVirtualSlab::iLink 
	member.

	This may extend into memory beyond the end of this object and contains
	#iNumSlabTypes entries.

	Each slab in these lists is also contained in #iSlabs.
	*/
	SDblQue iLists[1];
	};


FORCE_INLINE RVirtualAllocSlabSet::RVirtualAllocSlabSet(RVirtualAllocator* aAllocator, TUint aNumSlabTypes, DMutex*& aWriteLock)
	: iAllocator(aAllocator), iSlabs(0,aWriteLock), iNumSlabTypes(aNumSlabTypes)
	{
	while(aNumSlabTypes--)
		new (&iLists+aNumSlabTypes) SDblQue;
	}


RVirtualAllocSlabSet* RVirtualAllocSlabSet::New(RVirtualAllocator* aAllocator, TUint aNumSlabTypes, DMutex*& aWriteLock)
	{
	TUint size = sizeof(RVirtualAllocSlabSet) + sizeof(SDblQue) * (aNumSlabTypes - 1);
	RVirtualAllocSlabSet* set = (RVirtualAllocSlabSet*)Kern::AllocZ(size);
	if(set)
		new (set) RVirtualAllocSlabSet(aAllocator,aNumSlabTypes,aWriteLock);
	return set;
	}


RVirtualAllocSlabSet::~RVirtualAllocSlabSet()
	{
	__NK_ASSERT_DEBUG(iSlabs.Count()==0);
	}


TVirtualSlab* RVirtualAllocSlabSet::NewSlab(TLinAddr aAddr, TUint aSlabType)
	{
	TRACE2(("RVirtualAllocSlabSet::NewSlab(0x%08x,%d,%d)",aAddr,aSlabType));
	__NK_ASSERT_DEBUG(aSlabType<iNumSlabTypes);

	TVirtualSlab* slab = 0;
	TLinAddr base;
	TUint size;
	TInt r = iAllocator->Alloc(base,size,aAddr&~KVirtualAllocSlabMask,KVirtualAllocSlabSize,aSlabType);
	if(r==KErrNone)
		{
		slab = new TVirtualSlab(iLists[aSlabType],base,aSlabType);
		if(slab && iSlabs.Add(base,slab)!=KErrNone)
			{
			delete slab;
			slab = 0;
			}
		if(!slab)
			iAllocator->Free(base,KVirtualAllocSlabSize);
		}

	TRACE2(("RVirtualAllocSlabSet::NewSlab returns 0x%08x",slab));
	return slab;
	}


void RVirtualAllocSlabSet::DeleteSlab(TVirtualSlab* aSlab)
	{
	TLinAddr base = aSlab->Base();
#ifdef _DEBUG
	TAny* removedSlab = 
#endif
	iSlabs.Remove(base);
	__NK_ASSERT_DEBUG(removedSlab==aSlab);
	delete aSlab;
	iAllocator->Free(base,KVirtualAllocSlabSize);
	}


TInt RVirtualAllocSlabSet::Alloc(TLinAddr& aAddr, TUint aSizeShift, TUint aSlabType)
	{
	__NK_ASSERT_DEBUG(aSizeShift>=KVirtualAllocShift && aSizeShift<KVirtualAllocSlabShift);
	__NK_ASSERT_DEBUG(aSlabType<iNumSlabTypes);

	if(!aAddr)
		{
		SDblQueLink* head = &iLists[aSlabType].iA;
		SDblQueLink* link = head;
		while((link=link->iNext)!=head)
			{
			TVirtualSlab* slab = _LOFF(link,TVirtualSlab,iLink);
			TLinAddr addr = slab->Alloc(aSizeShift);
			if(addr)
				{
				aAddr = addr;
				return KErrNone;
				}
			}
		TVirtualSlab* slab = NewSlab(0,aSlabType);
		if(!slab)
			return KErrNoMemory;
		TLinAddr addr = slab->Alloc(aSizeShift);
		// Shouldn't ever fail as we've just allocated an empty slab and we can't 
		// attempt to allocate more than a whole slab.
		__NK_ASSERT_DEBUG(addr);
		aAddr = addr;
		return KErrNone;
		}

	TVirtualSlab* slab = (TVirtualSlab*)iSlabs.Find(aAddr&~KVirtualAllocSlabMask);
	if(!slab)
		{
		slab = NewSlab(aAddr,aSlabType);
		if(!slab)
			return KErrNoMemory;
		}
	else
		{
		if(slab->SlabType()!=aSlabType)
			return KErrAlreadyExists; // slab is of incompatible type
		}
	return slab->Alloc(aAddr,aSizeShift);
	}


void RVirtualAllocSlabSet::Free(TLinAddr aAddr, TUint aSizeShift)
	{
	__NK_ASSERT_DEBUG(aSizeShift>=KVirtualAllocShift && aSizeShift<KVirtualAllocSlabShift);

	TVirtualSlab* slab = (TVirtualSlab*)iSlabs.Find(aAddr&~KVirtualAllocSlabMask);
	if(slab)
		if(slab->Free(aAddr,aSizeShift))
			DeleteSlab(slab);
	}


TBool RVirtualAllocSlabSet::CheckSlabType(TLinAddr aAddr, TUint aSizeShift, TUint aSlabType)
	{
	__NK_ASSERT_DEBUG(aSizeShift>=KVirtualAllocShift && aSizeShift<KVirtualAllocSlabShift);

	TVirtualSlab* slab = (TVirtualSlab*)iSlabs.Find(aAddr&~KVirtualAllocSlabMask);
	if(!slab)
		{
		TRACE2(("RVirtualAllocSlabSet::CheckSlabType returns No Slab"));
		return false;
		}

	if(slab->iSlabType!=aSlabType)
		{
		TRACE2(("RVirtualAllocSlabSet::CheckSlabType returns Wrong Type"));
		return false;
		}

	return true;
	}


//
// RVirtualAllocator
//

RVirtualAllocator::RVirtualAllocator()
	: iBase(0), iSize(0), iAllocator(0), iSlabSet(0)
	{}


RVirtualAllocator::~RVirtualAllocator()
	{
	__NK_ASSERT_DEBUG(iAllocator==0 || iAllocator->iAvail==iAllocator->iSize); // should be empty
	delete iAllocator;
	delete iSlabSet;
	}


TInt RVirtualAllocator::Construct(TLinAddr aStart, TLinAddr aEnd, TUint aNumSlabTypes, DMutex*& aWriteLock)
	{
	if((aStart|aEnd)&KVirtualAllocSlabMask)
		return KErrArgument; // region not aligned to KVirtualAllocSlabSize
	TUint bitSize = (aEnd-aStart)>>KVirtualAllocSlabShift;
	iAllocator = TBitMapAllocator::New(bitSize, ETrue);
	if(!iAllocator)
		return KErrNoMemory;
	iSlabSet = RVirtualAllocSlabSet::New(this,aNumSlabTypes,aWriteLock);
	if(!iSlabSet)
		return KErrNoMemory;
	iBase = aStart;
	iSize = aEnd-aStart;
	return KErrNone;
	}


TUint RVirtualAllocator::AdjustRegion(TLinAddr& aAddr, TUint& aSize)
	{
	TLinAddr first = aAddr;
	TLinAddr last = (aAddr+aSize-1);
	TLinAddr dif = first^last;
	TUint granularity = KVirtualAllocShift;
	while(dif>>granularity && ++granularity<KVirtualAllocSlabShift)
		{}
	first >>= granularity;
	last >>= granularity;
	aAddr = first<<granularity;
	aSize = (last-first+1)<<granularity;
	return granularity;
	}


TInt RVirtualAllocator::Alloc(TLinAddr& aAddr, TUint& aSize, TLinAddr aRequestedAddr, TUint aRequestedSize, TUint aSlabType)
	{
	TRACE2(("RVirtualAllocator::Alloc(?,?,0x%08x,0x%08x,%d)",aRequestedAddr,aRequestedSize,aSlabType));

	if(!aRequestedSize)
		{
		TRACE2(("RVirtualAllocator::Alloc zero size"));
		return KErrArgument;
		}

	aAddr = aRequestedAddr;
	aSize = aRequestedSize;
	TUint align = AdjustRegion(aAddr,aSize);
	TRACE2(("RVirtualAllocator::Alloc adjusted to 0x%08x+0x%08x, align=%d",aAddr,aSize,align));

	if(align<KVirtualAllocSlabShift)
		return iSlabSet->Alloc(aAddr,align,aSlabType);

	__NK_ASSERT_DEBUG(align==KVirtualAllocSlabShift);
	TUint size = aSize>>KVirtualAllocSlabShift;

	if(!aAddr)
		{
		TInt r = iAllocator->AllocConsecutive(size, EFalse);
		if(r>=0)
			{
			iAllocator->Alloc(r, size);
			aAddr = iBase+(r<<KVirtualAllocSlabShift);
			return KErrNone;
			}
		return KErrNoMemory;
		}

	// specific address requested...
	if(!InRange(aAddr,aSize))
		{
		TRACE2(("RVirtualAllocator::Alloc not in range"));
		return KErrArgument;
		}

	TUint offset = TUint(aAddr-iBase)>>KVirtualAllocSlabShift;
	if(!iAllocator->NotFree(offset,size))
		{
		iAllocator->Alloc(offset,size);
		return KErrNone;
		}
	else
		{
		TRACE2(("RVirtualAllocator::Alloc already allocated!"));
		return KErrAlreadyExists;
		}
	}


void RVirtualAllocator::Free(TLinAddr aAddr, TUint aSize)
	{
	if(!aSize)
		return;

	TRACE2(("RVirtualAllocator::Free(0x%08x,0x%08x)",aAddr,aSize));

	TUint align = AdjustRegion(aAddr,aSize);
	TRACE2(("RVirtualAllocator::Free adjusted to 0x%08x+0x%08x, align=%d",aAddr,aSize,align));

	if(!InRange(aAddr,aSize))
		{
		TRACE2(("RVirtualAllocator::Free invalid region"));
		__NK_ASSERT_ALWAYS(0);
		return; // invalid region
		}

	if(align<KVirtualAllocSlabShift)
		{
		iSlabSet->Free(aAddr,align);
		return;
		}

	__NK_ASSERT_DEBUG(align==KVirtualAllocSlabShift);
	TUint offset = (aAddr-iBase)>>KVirtualAllocSlabShift;
	TUint size = aSize>>KVirtualAllocSlabShift;
	iAllocator->Free(offset,size);
	}


TBool RVirtualAllocator::CheckSlabType(TLinAddr aAddr, TUint aSize, TUint aSlabType)
	{
	TRACE2(("RVirtualAllocator::CheckSlabType(0x%08x,0x%08x,%d)",aAddr,aSize,aSlabType));
	if(!aSize)
		return false;

	TUint align = AdjustRegion(aAddr,aSize);

	if(!InRange(aAddr,aSize))
		{
		TRACE2(("RVirtualAllocator::CheckSlabType not in range"));
		return false;
		}

	if(align<KVirtualAllocSlabShift)
		{
		return iSlabSet->CheckSlabType(aAddr,align,aSlabType);
		}
	else
		{
		return true;
		}
	}


//
// RBackwardsVirtualAllocator
//

TInt RBackwardsVirtualAllocator::Alloc(TLinAddr& aAddr, TUint& aSize, TLinAddr aRequestedAddr, TUint aRequestedSize, TUint aSlabType)
	{
	if(aRequestedAddr)
		aRequestedAddr = (iBase+iSize)-(aRequestedAddr+aRequestedSize-iBase);
	TInt r = RVirtualAllocator::Alloc(aAddr,aSize,aRequestedAddr,aRequestedSize,aSlabType);
	if(r==KErrNone)
		aAddr = (iBase+iSize)-(aAddr+aSize-iBase);
	return r;
	}


void RBackwardsVirtualAllocator::Free(TLinAddr aAddr, TUint aSize)
	{
	RVirtualAllocator::Free((iBase+iSize)-(aAddr+aSize-iBase),aSize);
	}


