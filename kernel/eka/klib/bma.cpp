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
// e32\klib\bma.cpp
// This file is directly included in the test harness t_tbma
// 
//

#include <kernel/kbma.h>

#ifdef TBMA_TEST_CODE

#ifdef __MARM__
#define __TBMA_MACHINE_CODED__
#endif

#include <e32std.h>
#include <e32std_private.h>
#include <e32atomics.h>

#define __ALLOC(x)		User::Alloc(x)

void TBmaFault(TInt aLine)
	{
	User::Panic(_L("TBMA"),aLine);
	}

#else

#include <kernel/kern_priv.h>

#define __ALLOC(x)		Kern::Alloc(x)

void TBmaFault(TInt aLine)
	{
	Kern::Fault("TBMA",aLine);
	}

#endif

#define TBMA_FAULT()	TBmaFault(__LINE__)

/**	Creates a new TBitMapAllocator object.

	@param	aSize The number of bit positions required, must be >0.
	@param	aState	TRUE if all bit positions initially free
					FALSE if all bit positions initially allocated.
	
	@return	Pointer to new object, NULL if out of memory.

    @pre    Calling thread must be in a critical section.
    @pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Interrupts must be enabled.
	@pre	Kernel must be unlocked.
 */
EXPORT_C TBitMapAllocator* TBitMapAllocator::New(TInt aSize, TBool aState)
	{
	#ifndef TBMA_TEST_CODE
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TBitMapAllocator::New");	
	#endif
	TInt nmapw=(aSize+31)>>5;
	TInt memsz=sizeof(TBitMapAllocator)+(nmapw-1)*sizeof(TUint32);
	TBitMapAllocator* pA=(TBitMapAllocator*)__ALLOC(memsz);
	if (pA)
		new(pA) TBitMapAllocator(aSize, aState);
	return pA;
	}


/**	Finds a set of consecutive bit positions with specified alignment, with
	support for chaining multiple allocators.
	
	Note that this function does not mark the positions as allocated.

	In first fit mode:
	1. Any initial run is added to the carry in
	2. If all bits free, if BMA size+carry<=request length return 0 and leave carry alone
		else add size to carry and return KErrOverflow
	3. If request satisfied by initial run + carry return 0 and leave carry alone
	4. If request satisfied by an intermediate or final run, return start pos of run and set carry=0
	5. Otherwise carry = length of any final run and return KErrNotFound

	With a single allocator set aCarry (and usually aBase) to zero and ignore
	aRunLength. The return value then indicates the required position (after
	being aligned up as necessary) or KErrNotFound.

	With multiple allocators, this function should be called on each allocator in
	increasing logical bit number order. The carry should be set to zero initially
	and if there is a gap in the logical bit number between two allocators, otherwise
	it should be left alone. The first call which returns a nonnegative value indicates
	success and the required logical bit position is given by aligning up
		logical bit number of first bit of allocator + return value - carry

	In best fit mode:
	1. Any initial run is added to the carry in
	2. If all bits free, add bma length to carry and return KErrOverflow
	3. If any run including initial+carry but not final has length >= request length
		return start pos and length of smallest such, also set carry = length of final run
		unless exact match found, when carry is either unchanged or set to 0
	4. If only final run large enough, return KErrNotFound and set carry = length of final run
		carry=0 if no final run

	Here is an example of how to use this for multiple allocators:
	@code
	// aLength = run length required, aAlign = alignment constraint
	TInt bmalen=0;
	TInt carry=0;
	TInt minrun=KMaxTInt;					// this will track the length of the shortest useful run
	TInt minrunpos=KErrNotFound;			// this will track the start position of the shortest useful run
	TUint32 alignsize=1<<aAlign;
	TUint32 alignmask=alignsize-1;
	TBitMapAllocator** ppA=iBmaList;		// pointer to list of TBitMapAllocator*
	TBitMapAllocator** pE=ppA+iNumBmas;		// pointer to end of list
	TInt* pB=iBaseList;						// pointer to list of initial logical bit numbers
	TInt base=*pB;
	for (; ppA<pE; ++ppA, ++pB)
		{
		TBitMapAllocator* pA=*ppA;
		if (*pB!=base+bmalen)
			{
			// this BMA is not contiguous with previous one
			// check final run of previous BMA
			if (carry<minrun)
				{
				TInt fpos=base+bmalen-carry;
				TInt lost=((fpos+base+alignmask)&~alignmask)-base-fpos;
				if (carry-lost>=aLength)
					{
					minrun=carry;
					minrunpos=fpos;
					}
				}
			carry=0;
			}
		base=*pB;
		bmalen=pA->iSize;
		TInt l=KMaxTInt;
		TInt oldc=carry;	// need to save this for the case where the best run is the initial one
		TInt r=pA->AllocAligned(aLength,aAlign,base,ETrue,carry,l);
		if (r>=0)
			{
			// check shortest run in this BMA
			if (l<minrun)
				{
				minrun=l;
				minrunpos=r ? (base+r) : (base-oldc);
				if (minrun==aLength)
					break;				// exact match so finish
				}
			}
		}
	// check final run of last BMA (unless exact match already found)
	if (ppA==pE && carry<minrun)
		{
		TInt fpos=base+bmalen-carry;
		TInt lost=((fpos+alignmask)&~alignmask)-fpos;
		if (carry-lost>=aLength)
			{
			minrun=carry;
			minrunpos=fpos;
			}
		}
	result = (minrunpos<0) ? minrunpos : ((minrunpos+alignmask)&~alignmask);

	@endcode


	@param	aLength		number of consecutive bit positions to allocate.
	@param	aAlign		logarithm to base 2 of the alignment required.
	@param	aBase		the alignment of the first bit of this allocator - only significant modulo 2^aAlign.
	@param	aBestFit	TRUE for best fit allocation strategy, FALSE for first fit.
	@param	aCarry		carry in/carry out.
	@param	aRunLength	Holds best run length found so far.  This will be set to KMaxTInt when no
						suitable run length has been found.  In best fit mode aCarry should also be
						checked as aRunLength will not be set if aCarry is the only suitable run length
						found.
	
	@return	Start position, if a suitable run was found;
	        KErrNotFound, if no suitable run was found;
	        KErrOverflow, if all positions free and best fit mode, or if all positions free 
			in first fit mode and length requested > number of positions available.

	@see	TBitMapAllocator::AllocConsecutive(TInt aLength, TBool aBestFit)
	@see	TBitMapAllocator::AllocAligned(TInt aLength, TInt aAlign, TInt aBase, TBool aBestFit)
 */
EXPORT_C TInt TBitMapAllocator::AllocAligned(TInt aLength, TInt aAlign, TInt aBase, TBool aBestFit, TInt& aCarry, TInt& aRunLength) const
	{
	return AllocAligned(aLength, aAlign, aBase, aBestFit, aCarry, aRunLength, 0);
	}


/**	Allocates the next available bit position starting from the specified offset.

	Note - If no free bit positions can be found after aOffset this method will
	wrap around and continue the search from the start of the bit map.

	@param aOffset	The offset from the start of the bit map.
	@return	The number of the bit position allocated, -1 if all positions are occupied.
 */
EXPORT_C TInt TBitMapAllocator::AllocFrom(TUint aOffset)
	{
	__ASSERT_ALWAYS(aOffset < (TUint)iSize, TBMA_FAULT());

	if (!iAvail)
		return -1;
	--iAvail;
	const TUint32* pEnd = iMap + ((iSize+31)>>5);
	TUint32* pW = iMap + (aOffset >> 5);
	// Only check the bits after aOffset in this word.
	TUint wordMask = 0xffffffffu >> (aOffset & 0x1f);
	#ifdef _DEBUG
	if(!((aOffset&0x1f)==0 || (wordMask&0x80000000u)==0)) // check compiler has done unsigned >>
		TBMA_FAULT();
	#endif
	TUint word = *pW & wordMask;
	// No free bit positions in this word so search through the rest of the words.
	while (!word)
		{
		++pW;
		if (pW >= pEnd)
			pW = iMap;
		word = *pW;
		}
	TInt n = __e32_find_ms1_32(word);
	*pW &= ~(1 << n);
	n = (31 - n) + ((pW - iMap) << 5);
	return n;
	}


#if !defined( __TBMA_MACHINE_CODED__) | defined(__EABI_CTORS__)
/**	Constructs a new TBitMapAllocator object.

	@param	aSize The number of bit positions required.
	@param	aState	TRUE if all bit positions initially free;
					FALSE if all bit positions initially allocated.
 */
EXPORT_C TBitMapAllocator::TBitMapAllocator(TInt aSize, TBool aState)
	{
	__ASSERT_ALWAYS(aSize>0, TBMA_FAULT());
	iSize=aSize;
	if (aState)
		{
		iCheckFirst=iMap;
		iAvail=aSize;
		TUint32* pW=iMap;
		for (; aSize>=32; aSize-=32)
			*pW++=0xffffffff;
		if (aSize)
			*pW=((0xffffffffu)<<(32-aSize));
		}
	else
		{
		TInt nmapw=(aSize+31)>>5;
		iAvail=0;
		iCheckFirst=iMap+nmapw-1;
		memclr(iMap, nmapw*sizeof(TUint32));
		}
	}
#endif


#if !defined( __TBMA_MACHINE_CODED__)
/**	Allocates the next available bit position.

	@return	Number of position allocated, -1 if all positions occupied.
 */
EXPORT_C TInt TBitMapAllocator::Alloc()
	{
	if (!iAvail)
		return -1;
	--iAvail;
	TUint32* pW=iCheckFirst;
	while (!*pW)
		++pW;
	iCheckFirst=pW;
	TInt n=__e32_find_ms1_32(*pW);
	*pW &= ~(1<<n);
	n=(31-n)+((pW-iMap)<<5);
	return n;
	}


/**	Frees the specified bit position.

	@param	aPos Number of bit position to be freed; must be currently allocated.
 */
EXPORT_C void TBitMapAllocator::Free(TInt aPos)
	{
	__ASSERT_ALWAYS(TUint(aPos)<TUint(iSize), TBMA_FAULT());
	TUint32* pW=iMap+(aPos>>5);
	TUint32 b=0x80000000u>>(aPos&31);
	__ASSERT_ALWAYS(!(*pW & b), TBMA_FAULT());
	*pW |= b;
	++iAvail;
	if (pW<iCheckFirst)
		iCheckFirst=pW;
	}


/**	Allocates a specific range of bit positions.

	The specified range must lie within the total range for this allocator and all
	the positions must currently be free.

	@param	aStart	First position to allocate.
	@param	aLength	Number of consecutive positions to allocate, must be >0.
 */
EXPORT_C void TBitMapAllocator::Alloc(TInt aStart, TInt aLength)
	{
	__ASSERT_ALWAYS(TUint(aStart)<TUint(iSize), TBMA_FAULT());
	__ASSERT_ALWAYS(TUint(aStart+aLength)>=TUint(aStart), TBMA_FAULT());
	__ASSERT_ALWAYS(TUint(aStart+aLength)<=TUint(iSize), TBMA_FAULT());
	TInt wix=aStart>>5;
	TInt sbit=aStart&31;
	TUint32* pW=iMap+wix;
	iAvail-=aLength;
	TInt ebit=sbit+aLength;
	if (ebit<32)
		{
		TUint32 b=(~(0xffffffffu>>aLength)>>sbit);
		TUint32 w=*pW;
		__ASSERT_ALWAYS((w|b)==w, TBMA_FAULT());
		*pW=w&~b;
		return;
		}
	TUint32 b=(0xffffffffu>>sbit);
	while (ebit>0)
		{
		TUint32 w=*pW;
		__ASSERT_ALWAYS((w|b)==w, TBMA_FAULT());
		*pW++=w&~b;
		b=0xffffffffu;
		ebit-=32;
		if (ebit<32)
			b=~(b>>ebit);
		}
	}


/**	Frees a specific range of bit positions.

	The specified range must lie within the total range for this allocator and all
	the positions must currently be allocated.

	@param	aStart	First position to free.
	@param	aLength	Number of consecutive positions to free, must be >0.
 */
EXPORT_C void TBitMapAllocator::Free(TInt aStart, TInt aLength)
	{
	__ASSERT_ALWAYS(TUint(aStart)<TUint(iSize), TBMA_FAULT());
	__ASSERT_ALWAYS(TUint(aStart+aLength)>=TUint(aStart), TBMA_FAULT());
	__ASSERT_ALWAYS(TUint(aStart+aLength)<=TUint(iSize), TBMA_FAULT());
	TInt wix=aStart>>5;
	TInt sbit=aStart&31;
	TUint32* pW=iMap+wix;
	if (!iAvail || pW<iCheckFirst)
		iCheckFirst=pW;
	iAvail+=aLength;
	TInt ebit=sbit+aLength;
	if (ebit<32)
		{
		TUint32 b=(~(0xffffffffu>>aLength)>>sbit);
		TUint32 w=*pW;
		__ASSERT_ALWAYS((w&b)==0, TBMA_FAULT());
		*pW=w|b;
		return;
		}
	TUint32 b=(0xffffffffu>>sbit);
	while (ebit>0)
		{
		TUint32 w=*pW;
		__ASSERT_ALWAYS((w&b)==0, TBMA_FAULT());
		*pW++=w|b;
		b=0xffffffffu;
		ebit-=32;
		if (ebit<32)
			b=~(b>>ebit);
		}
	}


/**	Frees a specific range of bit positions.
	
	The specified range must lie within the total range for this allocator but it is
	not necessary that all the positions are currently allocated.

	@param	aStart	First position to free.
	@param	aLength	Number of consecutive positions to free, must be >0.
 */
EXPORT_C void TBitMapAllocator::SelectiveFree(TInt aStart, TInt aLength)
	{
	__ASSERT_ALWAYS(TUint(aStart)<TUint(iSize), TBMA_FAULT());
	__ASSERT_ALWAYS(TUint(aStart+aLength)>=TUint(aStart), TBMA_FAULT());
	__ASSERT_ALWAYS(TUint(aStart+aLength)<=TUint(iSize), TBMA_FAULT());
	TInt wix=aStart>>5;
	TInt sbit=aStart&31;
	TUint32* pW=iMap+wix;
	if (!iAvail || pW<iCheckFirst)
		iCheckFirst=pW;
	iAvail+=aLength;	// update free count assuming no positions already free
	TInt ebit=sbit+aLength;
	if (ebit<32)
		{
		TUint32 b=(~(0xffffffffu>>aLength)>>sbit);
		TUint32 w=*pW;
		*pW=w|b;		// mark all positions free
		iAvail-=__e32_bit_count_32(w&b);	// reduce free count by number of positions already free
		return;
		}
	TUint32 b=(0xffffffffu>>sbit);
	while (ebit>0)
		{
		TUint32 w=*pW;
		*pW++=w|b;		// mark all positions free
		iAvail-=__e32_bit_count_32(w&b);	// reduce free count by number of positions already free
		b=0xffffffffu;
		ebit-=32;
		if (ebit<32)
			b=~(b>>ebit);
		}
	}


/**	Allocates a specific range of bit positions.
	
	The specified range must lie within the total range for this allocator but it is
	not necessary that all the positions are currently free.

	@param	aStart	First position to allocate.
	@param	aLength	Number of consecutive positions to allocate, must be >0.
	@return The number of previously free positions that were allocated.
 */
EXPORT_C TUint TBitMapAllocator::SelectiveAlloc(TInt aStart, TInt aLength)
	{
	__ASSERT_ALWAYS(TUint(aStart) < TUint(iSize), TBMA_FAULT());
	__ASSERT_ALWAYS(TUint(aStart + aLength) >= TUint(aStart), TBMA_FAULT());
	__ASSERT_ALWAYS(TUint(aStart + aLength) <= TUint(iSize), TBMA_FAULT());
	TInt wix = aStart >> 5;
	TInt sbit = aStart & 31;
	TUint32* pW = iMap + wix;
	iAvail -= aLength;	// update free count assuming no positions already allocated
	TInt ebit = sbit + aLength;
	if (ebit < 32)
		{
		TUint32 b = ((0xffffffffu >> aLength) >> sbit) | ~(0xffffffffu >> sbit);
		TUint32 w = *pW;
		*pW = w & b;	// mark all positions allocated
		TUint allocated = __e32_bit_count_32(~w & ~b);
		iAvail += allocated;	// increase free count by number of positions already allocated
		return aLength - allocated;
		}
	TUint32 b = ~(0xffffffffu >> sbit);
	while (ebit > 0)
		{
		TUint32 w = *pW;
		*pW++ = w & b;		// mark all positions allocated
		TUint allocated = __e32_bit_count_32(~w & ~b);
		iAvail += allocated;	// increase free count by number of positions already allocated
		aLength -= allocated;
		ebit -= 32;
		b = (ebit >= 32)? 0 : 0xffffffff >> ebit;
		}
	return aLength;
	}


/**	Tests whether a specific range of bit positions are all free.

	The specified range must lie within the total range for this allocator.

	@param	aStart	First position to check.
	@param	aLength	Number of consecutive positions to check, must be >0.
	
	@return	FALSE if all positions free, TRUE if at least one is occupied.
 */
EXPORT_C TBool TBitMapAllocator::NotFree(TInt aStart, TInt aLength) const
	{
	// Inverse logic - returns 0 if all positions free, nonzero otherwise
	__ASSERT_ALWAYS(TUint(aStart)<TUint(iSize), TBMA_FAULT());
	__ASSERT_ALWAYS(TUint(aStart+aLength)>=TUint(aStart), TBMA_FAULT());
	__ASSERT_ALWAYS(TUint(aStart+aLength)<=TUint(iSize), TBMA_FAULT());
	TInt wix=aStart>>5;
	TInt sbit=aStart&31;
	const TUint32* pW=iMap+wix;
	TInt ebit=sbit+aLength;
	if (ebit<32)
		{
		TUint32 b=(~(0xffffffffu>>aLength)>>sbit);
		return (*pW^b)&b;
		}
	TUint32 b=(0xffffffffu>>sbit);
	TUint32 r=0;
	while (ebit>0)
		{
		r|=((*pW++^b)&b);
		b=0xffffffffu;
		ebit-=32;
		if (ebit<32)
			b=~(b>>ebit);
		}
	return r;
	}


/**	Tests whether a specific range of bit positions are all occupied.

	The specified range must lie within the total range for this allocator.

	@param	aStart	First position to check.
	@param	aLength	Number of consecutive positions to check, must be >0.
	
	@return	FALSE if all positions occupied, TRUE if at least one is free.
 */
EXPORT_C TBool TBitMapAllocator::NotAllocated(TInt aStart, TInt aLength) const
	{
	// Inverse logic - returns 0 if all positions allocated, nonzero otherwise
	__ASSERT_ALWAYS(TUint(aStart)<TUint(iSize), TBMA_FAULT());
	__ASSERT_ALWAYS(TUint(aStart+aLength)>=TUint(aStart), TBMA_FAULT());
	__ASSERT_ALWAYS(TUint(aStart+aLength)<=TUint(iSize), TBMA_FAULT());
	TInt wix=aStart>>5;
	TInt sbit=aStart&31;
	const TUint32* pW=iMap+wix;
	TInt ebit=sbit+aLength;
	if (ebit<32)
		{
		TUint32 b=(~(0xffffffffu>>aLength)>>sbit);
		return *pW&b;
		}
	TUint32 b=(0xffffffffu>>sbit);
	TUint32 r=0;
	while (ebit>0)
		{
		r|=(*pW++&b);
		b=0xffffffffu;
		ebit-=32;
		if (ebit<32)
			b=~(b>>ebit);
		}
	return r;
	}


/**	Allocates up to a specified number of available bit positions.

	The allocated positions are not required to bear any relationship to
	each other.
	If the number of free positions is less than the number requested,
	allocate all currently free positions.

	@param	aLength	Maximum number of positions to allocate.
	@param	aList	Pointer to memory area where allocated bit numbers should be stored.

	@return	The number of positions allocated.
 */
EXPORT_C TInt TBitMapAllocator::AllocList(TInt aLength, TInt* aList)
	{
	__ASSERT_ALWAYS(aLength>0, TBMA_FAULT());
	if (aLength>iAvail)
		aLength=iAvail;
	TInt c=aLength;
	while (c--)
		*aList++=Alloc();
	return aLength;
	}


/**	Finds a set of consecutive bit positions with specified alignment starting the 
	search from the specfied bit position offset, with support for chaining 
	multiple allocators.
	
	For further details see:
	TBitMapAllocator::AllocAligned(TInt aLength, TInt aAlign, TInt aBase, TBool aBestFit, TInt& aCarry, TInt& aRunLength)

	@param	aLength		number of consecutive bit positions to allocate.
	@param	aAlign		logarithm to base 2 of the alignment required.
	@param	aBase		the alignment of the first bit of this allocator - only significant modulo 2^aAlign.
	@param	aBestFit	TRUE for best fit allocation strategy, FALSE for first fit.
	@param	aCarry		carry in/carry out.
	@param	aRunLength	Holds best run length found so far.  This will be set to KMaxTInt when no
						suitable run length has been found.  In best fit mode aCarry should also be
						checked as aRunLength will not be set if aCarry is the only suitable run length
						found.
	@param	aOffset		The bit position to start the search from, set to 0 to search all bit positions.
						aOffset will be aligned so all bits before an aligned aOffset will be
						ignored.  This can only be non-zero if aCarry is zero as any carry in bits will be
						ignored if aOffset is non-zero.
	
	@return	Start position, if a suitable run was found;
	        KErrNotFound, if no suitable run was found;
	        KErrOverflow, if all positions free and best fit mode, or if all positions free 
			in first fit mode and length requested > number of positions available.

	@see TBitMapAllocator::AllocAligned(TInt aLength, TInt aAlign, TInt aBase, TBool aBestFit, TInt& aCarry, TInt& aRunLength)
 */
EXPORT_C TInt TBitMapAllocator::AllocAligned(TInt aLength, TInt aAlign, TInt aBase, TBool aBestFit, TInt& aCarry, TInt& aRunLength, TUint aOffset) const
	{
	TInt minrl=KMaxTInt;
	__ASSERT_ALWAYS(aLength>0, TBMA_FAULT());
	__ASSERT_ALWAYS(TUint(aAlign)<31, TBMA_FAULT());
	__ASSERT_ALWAYS(aOffset < (TUint)iSize, TBMA_FAULT());
	__ASSERT_ALWAYS(!aCarry || !aOffset, TBMA_FAULT());
	TUint32 alignsize=1<<aAlign;
	TUint32 alignmask=alignsize-1;
	aBase&=alignmask;
	if (iAvail==iSize)
		{
		// Align aOffset if it is set so we ignore all bits before the aligned offset.
		aOffset = (!aOffset)? aOffset : ((aOffset + aBase + alignmask) & ~alignmask) - aBase;
		TInt runLength = (aOffset < (TUint)iSize)? iSize - aOffset : 0;
		if (!aBestFit)
			{
			TInt alignedStartPos = ((aOffset - aCarry + aBase + alignmask) & ~alignmask) - aBase;
			TInt lost = alignedStartPos - (aOffset - aCarry);
			if (runLength + aCarry - lost >= aLength)
				{
				aRunLength = runLength;
				if (alignedStartPos >= 0)
					{
					aCarry=0;	// clear carry if not initial run
					}
				return (alignedStartPos >= 0)? alignedStartPos : 0; // return start pos of exact run
				}
			}
		if (aOffset)
			aCarry = runLength;
		else
			aCarry += iAvail;
		aRunLength = KMaxTInt;
		return KErrOverflow;
		}
	const TUint32* pW=aCarry?iMap:iCheckFirst;
	const TUint32* pE=iMap+((iSize+31)>>5);
	TInt n=((pW-iMap)<<5);
	TInt p=-1;
	TInt q=-aCarry;
	TUint32 s=aCarry?~0:0;	// 0 when searching for 1's, FFFFFFFF when searching for 0's

	TUint32 offsetMask = 0;
	if (aOffset)
		{// Start search from aOffset.  Only align aOffset if aOffset is to
		// be used otherwise the best fit mode may fail as aligning aOffset
		// may cause the search to skip past parts of the bit map.
		aOffset = ((aOffset + aBase + alignmask) & ~alignmask) - aBase;
		const TUint32* offsetWord = iMap + (aOffset >> 5);
		if (offsetWord >= pW)
			{
			pW = offsetWord;
			n = aOffset & 0xffffffe0;
			offsetMask = 0xffffffff >> (aOffset & 31);
			__ASSERT_ALWAYS(offsetMask, TBMA_FAULT());
			}
		}
	while (pW<pE)
		{
		TUint32 word = *pW++;
		if (offsetMask)
			{// Start search after bit aOffset.
			word &= offsetMask; // Mask off any bits before the aOffset
			offsetMask = 0;		// Reset so future iterations use whole of next word.
			}
		if (word==s)		// check if any of required bit present
			{
			n+=32;			// if not, step bit number on by 32
			continue;
			}
		TInt rl=-1;
		for (TUint32 b=0x80000000; b; ++n, b>>=1)
			{
			if ((word ^ s) & b)
				{
				if (s && n==iSize)
					break;	// reached end
				// bit found - invert search bit
				s=~s;
				if (s)
					q=n;	// 1 found so save position
				else
					{
					rl=n-q;	// 0 found, calculate run length of 1's
					TInt alignedStartPos = ((q + aBase + alignmask) & ~alignmask) - aBase;
					TInt lost = alignedStartPos - q;
					if (rl-lost>=aLength)
						{
						if (!aBestFit || rl==aLength)
							{
							// first fit or exact match - we're finished
							if (alignedStartPos >= 0)
								{
								aCarry=0;	// clear carry if not initial run
								}
							aRunLength=rl;
							return (alignedStartPos >= 0)? alignedStartPos : 0;
							}
						if (rl<minrl)
							{
							// best fit and this run is smallest so far, so record its position and length
							minrl=rl;
							p = (alignedStartPos >= 0)? alignedStartPos : 0;
							}
						}
					}
				}
			}
		}
	if (minrl!=aLength)
		{
		// exact match not found or first fit and no match found
		TInt rl=0;
		if (s)
			{
			// we were looking for 0, so this counts as a run
			// get run length
			rl=n-q;
			if (!aBestFit)
				{
				TInt alignedStartPos = ((q + aBase + alignmask) & ~alignmask) - aBase;
				TInt lost = alignedStartPos - q;
				if (rl-lost>=aLength)
					{// BMA is not totally empty so this can't be the initial run
					// and the final run.  Therefore the start pos must be within
					// this bma so clear carry and return start pos.
					aCarry=0;
					aRunLength=rl;
					return alignedStartPos;
					}
				}
			}
		aCarry=rl;	// set carry to length of final run or 0 if none
		}
	aRunLength=minrl;	// return best run length found
	return p;		// return start position of run or -1 if run not found
	}
#endif


/** Finds a set of consecutive free positions on a single bit map allocator.

	@param	aLength		number of consecutive bit positions to allocate.
	@param	aBestFit	TRUE for best fit allocation strategy, FALSE for first fit.
	
	@return	Start position, if a suitable run was found;
	        KErrNotFound, if no suitable run was found.
 */
EXPORT_C TInt TBitMapAllocator::AllocConsecutive(TInt aLength, TBool aBestFit) const
	{
	TInt carry=0;
	TInt l=KMaxTInt;
	TInt r=AllocAligned(aLength,0,0,aBestFit,carry,l);
	if (aBestFit)
		{
		// must check final run if any
		if (carry>=aLength && carry<l)
			r=iSize-carry;
		}
	if (r<0)
		r=KErrNotFound;
	return r;
	}


/** Finds a set of consecutive free positions on a single bit map allocator with
	specified alignment.

	@param	aLength		number of consecutive bit positions to allocate.
	@param	aAlign		logarithm to base 2 of the alignment required.
	@param	aBase		the alignment of the first bit of this allocator - only significant modulo 2^aAlign.
	@param	aBestFit	TRUE for best fit allocation strategy, FALSE for first fit.
	
	@return	Start position, if a suitable run was found;
	        KErrNotFound, if no suitable run was found.
 */
EXPORT_C TInt TBitMapAllocator::AllocAligned(TInt aLength, TInt aAlign, TInt aBase, TBool aBestFit) const
	{
	TInt carry=0;
	TInt l=KMaxTInt;
	TUint32 alignsize=1<<aAlign;
	TUint32 alignmask=alignsize-1;
	aBase&=alignmask;
	TInt r=AllocAligned(aLength,aAlign,aBase,aBestFit,carry,l);
	if (aBestFit)
		{
		// must check final run if any
		TInt fpos=iSize-carry;
		TInt lost=((fpos+aBase+alignmask)&~alignmask)-aBase-fpos;
		if (carry-lost>=aLength && carry<l)
			r=fpos+lost;
		}
	if (r<0)
		r=KErrNotFound;
	else
		r=((r+aBase+alignmask)&~alignmask)-aBase;
	return r;
	}


/** Copies a range from another allocator, mark remainder as occupied.

	Values of bit positions from aFirst to aFirst+aLen-1 inclusive in allocator
	aA are copied to bit positions in this allocator starting with aFirst mod 32.
	Remaining bit positions in this allocator are marked as occupied.

	@param	aA		Pointer to source allocator.
	@param	aFirst	Number in source allocator of first bit to copy.
	@param	aLen	Number of bits to copy from source allocator.
 */
EXPORT_C void TBitMapAllocator::CopyAlignedRange(const TBitMapAllocator* aA, TInt aFirst, TInt aLen)
	{
	const TUint32* srcptr = aA->iMap + (aFirst>>5);
	TInt last = aFirst + aLen - 1;
	TInt len = (((last+32)&~31)-(aFirst&~31))>>3;	// bytes
	__ASSERT_ALWAYS(len<=iSize, TBMA_FAULT());
	TInt remain = ((iSize+31)&~31)-(len<<3);
	wordmove(iMap, srcptr, len);
	memclr(iMap+(len>>2), remain>>3);
	TUint32* p = iMap;
	TUint32* pE = p + (len>>2);
	*p &= (0xffffffffu >> (aFirst&31));
	pE[-1] &= (0xffffffffu << (31-(last&31)));
	iCheckFirst = pE-1;
	iAvail = 0;
	for (; p<pE; ++p)
		{
		TUint32 x = *p;
		if (x)
			{
			if (p<iCheckFirst)
				iCheckFirst = p;
			iAvail += __e32_bit_count_32(x);
			}
		}
	}
