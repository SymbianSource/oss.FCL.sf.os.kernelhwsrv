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
// e32\common\heap.cpp
// 
//

#include "common.h"
#ifdef __KERNEL_MODE__
#include <kernel/kern_priv.h>
#endif

#ifdef _DEBUG
#define __SIMULATE_ALLOC_FAIL(s)	if (CheckForSimulatedAllocFail()) {s}
#define	__CHECK_CELL(p)				CheckCell(p)
#define	__ZAP_CELL(p)				memset( ((TUint8*)p) + RHeap::EAllocCellSize, 0xde, p->len - RHeap::EAllocCellSize)
#define __DEBUG_SAVE(p)				TInt dbgNestLevel = ((SDebugCell*)p)->nestingLevel
#define __DEBUG_RESTORE(p)			((SDebugCell*)(((TUint8*)p)-EAllocCellSize))->nestingLevel = dbgNestLevel
#else
#define __SIMULATE_ALLOC_FAIL(s)
#define	__CHECK_CELL(p)
#define	__ZAP_CELL(p)
#define __DEBUG_SAVE(p)
#define __DEBUG_RESTORE(p)
#endif

#define __NEXT_CELL(p)				((SCell*)(((TUint8*)p)+p->len))

#define __POWER_OF_2(x)				((TUint32)((x)^((x)-1))>=(TUint32)(x))

#define __MEMORY_MONITOR_CHECK_CELL(p) \
					{ \
					TLinAddr m = TLinAddr(iAlign-1); \
					SCell* c = (SCell*)(((TUint8*)p)-EAllocCellSize); \
					if((c->len & m) || (c->len<iMinCell) || ((TUint8*)c<iBase) || ((TUint8*)__NEXT_CELL(c)>iTop)) \
						BTraceContext12(BTrace::EHeap, BTrace::EHeapCorruption, (TUint32)this, (TUint32)p, (TUint32)c->len-EAllocCellSize); \
					}
					
/**
@SYMPatchable
@publishedPartner
@released

Defines the minimum cell size of  a heap.

The constant can be changed at ROM build time using patchdata OBY keyword.
*/
#ifdef __X86GCC__	// For X86GCC we dont use the proper data import attribute
#undef IMPORT_D		// since the constant is not really imported. GCC doesn't 
#define IMPORT_D	// allow imports from self.
#endif
IMPORT_D extern const TInt KHeapMinCellSize;

/**
@SYMPatchable
@publishedPartner
@released

This constant defines the ratio that determines the amount of hysteresis between heap growing and heap
shrinking.
It is a 32-bit fixed point number where the radix point is defined to be
between bits 7 and 8 (where the LSB is bit 0) i.e. using standard notation, a Q8 or a fx24.8
fixed point number.  For example, for a ratio of 2.0, set KHeapShrinkHysRatio=0x200.

The heap shrinking hysteresis value is calculated to be:
@code
KHeapShrinkHysRatio*(iGrowBy>>8)
@endcode
where iGrowBy is a page aligned value set by the argument, aGrowBy, to the RHeap constructor.
The default hysteresis value is iGrowBy bytes i.e. KHeapShrinkHysRatio=2.0.

Memory usage may be improved by reducing the heap shrinking hysteresis
by setting 1.0 < KHeapShrinkHysRatio < 2.0.  Heap shrinking hysteresis is disabled/removed
when KHeapShrinkHysRatio <= 1.0.

The constant can be changed at ROM build time using patchdata OBY keyword.
*/
IMPORT_D extern const TInt KHeapShrinkHysRatio;

#pragma warning( disable : 4705 )	// statement has no effect
UEXPORT_C RHeap::RHeap(TInt aMaxLength, TInt aAlign, TBool aSingleThread)
/**
@internalComponent
*/
//
// Constructor for fixed size heap
//
	:	iMinLength(aMaxLength), iMaxLength(aMaxLength), iOffset(0), iGrowBy(0), iChunkHandle(0),
		iNestingLevel(0), iAllocCount(0), iFailType(ENone), iTestData(NULL)
	{
	iAlign = aAlign ? aAlign : ECellAlignment;
	iPageSize = 0;
	iFlags = aSingleThread ? (ESingleThreaded|EFixedSize) : EFixedSize;
	Initialise();
	}
#pragma warning( default : 4705 )




UEXPORT_C RHeap::RHeap(TInt aChunkHandle, TInt aOffset, TInt aMinLength, TInt aMaxLength, TInt aGrowBy, TInt aAlign, TBool aSingleThread)
/**
@internalComponent
*/
//
// Constructor for chunk heaps.
//
	:	iOffset(aOffset), iChunkHandle(aChunkHandle),
		iNestingLevel(0), iAllocCount(0), iFailType(ENone), iTestData(NULL)
	{
	TInt sz = iBase - ((TUint8*)this - iOffset);
	GET_PAGE_SIZE(iPageSize);
	__ASSERT_ALWAYS(iOffset>=0, HEAP_PANIC(ETHeapNewBadOffset));
	iMinLength = Max(aMinLength, sz + EAllocCellSize);
	iMinLength = _ALIGN_UP(iMinLength, iPageSize);
	iMaxLength = Max(aMaxLength, iMinLength);
	iMaxLength = _ALIGN_UP(iMaxLength, iPageSize);
	iGrowBy = _ALIGN_UP(aGrowBy, iPageSize);
	iFlags = aSingleThread ? ESingleThreaded : 0;
	iAlign = aAlign ? aAlign : ECellAlignment;
	Initialise();
	}




UEXPORT_C TAny* RHeap::operator new(TUint aSize, TAny* aBase) __NO_THROW
/**
@internalComponent
*/
	{
	__ASSERT_ALWAYS(aSize>=sizeof(RHeap), HEAP_PANIC(ETHeapNewBadSize));
	RHeap* h = (RHeap*)aBase;
	h->iAlign = 0x80000000;	// garbage value
	h->iBase = ((TUint8*)aBase) + aSize;
	return aBase;
	}

void RHeap::Initialise()
//
// Initialise the heap.
//
	{

	__ASSERT_ALWAYS((TUint32)iAlign>=sizeof(TAny*) && __POWER_OF_2(iAlign), HEAP_PANIC(ETHeapNewBadAlignment));
	iCellCount = 0;
	iTotalAllocSize = 0;
	iBase = (TUint8*)Align(iBase + EAllocCellSize);
	iBase -= EAllocCellSize;
	TInt b = iBase - ((TUint8*)this - iOffset);
	TInt len = _ALIGN_DOWN(iMinLength - b, iAlign);
	iTop = iBase + len;
	iMinLength = iTop - ((TUint8*)this - iOffset);
	iMinCell = Align(KHeapMinCellSize + Max((TInt)EAllocCellSize, (TInt)EFreeCellSize));
#ifdef _DEBUG
	memset(iBase, 0xa5, len);
#endif
	SCell* pM=(SCell*)iBase; // First free cell
	iFree.next=pM; // Free list points to first free cell
	iFree.len=0; // Stop free from joining this with a free block
	pM->next=NULL; // Terminate the free list
	pM->len=len; // Set the size of the free cell
	}

#ifdef _DEBUG
void RHeap::CheckCell(const SCell* aCell) const
	{
	TLinAddr m = TLinAddr(iAlign - 1);

	__ASSERT_DEBUG(!(aCell->len & m), HEAP_PANIC(ETHeapBadCellAddress));
	__ASSERT_DEBUG(aCell->len >= iMinCell, HEAP_PANIC(ETHeapBadCellAddress));
	__ASSERT_DEBUG((TUint8*)aCell>=iBase, HEAP_PANIC(ETHeapBadCellAddress));
	__ASSERT_DEBUG((TUint8*)__NEXT_CELL(aCell)<=iTop, HEAP_PANIC(ETHeapBadCellAddress));
	}
#endif

UEXPORT_C RHeap::SCell* RHeap::GetAddress(const TAny* aCell) const
//
// As much as possible, check a cell address and backspace it
// to point at the cell header.
//
	{

	TLinAddr m = TLinAddr(iAlign - 1);
	__ASSERT_ALWAYS(!(TLinAddr(aCell)&m), HEAP_PANIC(ETHeapBadCellAddress));

	SCell* pC = (SCell*)(((TUint8*)aCell)-EAllocCellSize);
	__CHECK_CELL(pC);

	return pC;
	}




UEXPORT_C TInt RHeap::AllocLen(const TAny* aCell) const
/**
Gets the length of the available space in the specified allocated cell.

@param aCell A pointer to the allocated cell.

@return The length of the available space in the allocated cell.

@panic USER 42 if aCell does not point to  a valid cell.
*/
	{

	SCell* pC = GetAddress(aCell);
	return pC->len - EAllocCellSize;
	}





#if !defined(__HEAP_MACHINE_CODED__) || defined(_DEBUG)
RHeap::SCell* RHeap::DoAlloc(TInt aSize, SCell*& aLastFree)
//
// Allocate without growing. aSize includes cell header and alignment.
// Lock already held.
//
	{
	SCell* pP = &iFree;
	SCell* pC = pP->next;
	for (; pC; pP=pC, pC=pC->next) // Scan the free list
		{
		__CHECK_CELL(pC);
		SCell* pE;
		if (pC->len >= aSize)				// Block size bigger than request
			{
			if (pC->len - aSize < iMinCell)	// Leftover must be large enough to hold an SCell
			   	{
			   	aSize = pC->len;			// It isn't, so take it all
			   	pE = pC->next;				// Set the next field
			   	}
			else
			   	{
			   	pE = (SCell*)(((TUint8*)pC)+aSize); // Take amount required
			   	pE->len = pC->len - aSize;	// Initialize new free cell
			   	pE->next = pC->next;
			   	}
			pP->next = pE;					// Update previous pointer
			pC->len = aSize;				// Set control size word
#if defined(_DEBUG)														
			((SDebugCell*)pC)->nestingLevel = iNestingLevel;
			((SDebugCell*)pC)->allocCount = ++iAllocCount;
#endif
			return pC;
			}
		}
	aLastFree = pP;
	return NULL;
	}
#endif




UEXPORT_C TAny* RHeap::Alloc(TInt aSize)
/**
Allocates a cell of the specified size from the heap.

If there is insufficient memory available on the heap from which to allocate
a cell of the required size, the function returns NULL.

The cell is aligned according to the alignment value specified at construction,
or the default alignment value, if an explict value was not specified.

The resulting size of the allocated cell may be rounded up to a
value greater than aSize, but is guaranteed to be not less than aSize.

@param aSize The 
size of the cell to be allocated from the heap

@return A pointer to the allocated cell. NULL if there is insufficient memory 
        available.
        
@panic USER 47 if the maximum unsigned value of aSize is greater than or equal
       to the value of KMaxTInt/2; for example, calling Alloc(-1) raises
       this panic.
       
@see KMaxTInt        
*/
	{

	__CHECK_THREAD_STATE;
	__ASSERT_ALWAYS((TUint)aSize<(KMaxTInt/2),HEAP_PANIC(ETHeapBadAllocatedCellSize));
	__SIMULATE_ALLOC_FAIL(return NULL;)
	
	TInt origSize = aSize;
	aSize = Max(Align(aSize + EAllocCellSize), iMinCell);
	SCell* pL = NULL;
	Lock();
	SCell* pC = (SCell*)DoAlloc(aSize, pL);
	if (!pC && !(iFlags & EFixedSize))
		{
		// try to grow chunk heap
		TInt r = TryToGrowHeap(aSize, pL);
		if (r==KErrNone)
			pC = DoAlloc(aSize, pL);
		}
	if (pC)
		++iCellCount, iTotalAllocSize += (pC->len - EAllocCellSize);
	Unlock();
	if (pC)
		{
		TAny* result=((TUint8*)pC) + EAllocCellSize;
		if (iFlags & ETraceAllocs)
			{
			TUint32 traceData[2];
			traceData[0] = AllocLen(result);
			traceData[1] = origSize;
			BTraceContextN(BTrace::EHeap, BTrace::EHeapAlloc, (TUint32)this, (TUint32)result, traceData, sizeof(traceData));
			}
#ifdef __KERNEL_MODE__
		memclr(result, pC->len - EAllocCellSize);
#endif		
		return result;
		}
	if (iFlags & ETraceAllocs)						
			BTraceContext8(BTrace::EHeap, BTrace::EHeapAllocFail, (TUint32)this, (TUint32)origSize);
	return NULL;
	}




TInt RHeap::TryToGrowHeap(TInt aSize, SCell* aLastFree)
	{
	TBool at_end = IsLastCell(aLastFree);
	TInt extra = at_end ? aSize - aLastFree->len : aSize;
	extra = (extra + iGrowBy - 1) / iGrowBy;
	extra *= iGrowBy;
	TInt cur_len = _ALIGN_UP(iTop - ((TUint8*)this - iOffset), iPageSize);
	TInt new_len = cur_len + extra;
	TInt r = KErrNoMemory;
	if (new_len <= iMaxLength)
		{
		r = SetBrk(new_len);
		if (r == KErrNone)
			{
			if (at_end)
				aLastFree->len += extra;
			else
				{
				SCell* pC = (SCell*)iTop;
				pC->len = extra;
				pC->next = NULL;
				aLastFree->next = pC;
				}
			iTop += extra;
			}
		}
	return r;
	}




#ifndef __KERNEL_MODE__
EXPORT_C TInt RHeap::Compress()
/**
Compresses the heap.

The function frees excess committed space from the top 
of the heap. The size of the heap is never reduced below the minimum size 
specified during creation of the heap.

@return The space reclaimed. If no space can be reclaimed, then this value 
        is zero.
*/
	{

	if (iFlags & EFixedSize)
		return 0;
	TInt r = 0;
	Lock();
	SCell* pC = &iFree;
	for (; pC->next; pC=pC->next) {}
	if (pC!=&iFree)
		{
		__CHECK_CELL(pC);
		if (IsLastCell(pC))
			r = Reduce(pC);
		}
	Unlock();
	return r;
	}
#endif




#if !defined(__HEAP_MACHINE_CODED__) || defined(_DEBUG)
void RHeap::DoFree(SCell* pC)
	{
	__ZAP_CELL(pC);

	SCell* pP = &iFree;
	SCell* pE = pP->next;
	for (; pE && pE<pC; pP=pE, pE=pE->next) {}
	if (pE)			// Is there a following free cell?
		{
		SCell* pN = __NEXT_CELL(pC);
		__ASSERT_ALWAYS(pN<=pE, HEAP_PANIC(ETHeapFreeBadNextCell)); // Following cell overlaps
		if (pN==pE) // Is it adjacent
			{
			pC->len += pE->len; // Yes - coalesce adjacent free cells
			pC->next = pE->next;
			}
		else					// pN<pE, non-adjacent free cells
			pC->next = pE;		// Otherwise just point to it
		}
	else
		pC->next = NULL;		// No following free cell
	SCell* pN = __NEXT_CELL(pP);	// pN=pP=&iFree if no preceding free cell
	__ASSERT_ALWAYS(pN<=pC, HEAP_PANIC(ETHeapFreeBadPrevCell)); // Previous cell overlaps
	if (pN==pC) // Is it adjacent
		{
		pP->len += pC->len;		// Yes - coalesce adjacent free cells
		pP->next = pC->next;
		pC = pP;				// for size reduction check
		}
	else						// pN<pC, non-adjacent free cells
		pP->next = pC;			// point previous cell to the one being freed
	pN = __NEXT_CELL(pC);		// End of amalgamated free cell
	if ((TUint8*)pN==iTop && !(iFlags & EFixedSize) && 
		pC->len >= KHeapShrinkHysRatio*(iGrowBy>>8))
		Reduce(pC);
	}
#endif




UEXPORT_C void RHeap::Free(TAny* aCell)
/**
Frees the specified cell and returns it to the heap.

@param aCell A pointer to a valid cell; this pointer can also be NULL,
             in which case the function does nothing and just returns.

@panic USER 42 if aCell points to an invalid cell.
*/
	{
	__CHECK_THREAD_STATE;
	if (!aCell)
		return;
	Lock();
	if (iFlags & EMonitorMemory)
		__MEMORY_MONITOR_CHECK_CELL(aCell);
	SCell* pC = GetAddress(aCell);
	--iCellCount;
	iTotalAllocSize -= (pC->len - EAllocCellSize);
	DoFree(pC);
	if (iFlags & ETraceAllocs)
		BTraceContext8(BTrace::EHeap, BTrace::EHeapFree, (TUint32)this, (TUint32)aCell);
	Unlock();
	}




TInt RHeap::Reduce(SCell* aCell)
	{
	TInt reduce=0;
	TInt offset=((TUint8*)aCell)-((TUint8*)this - iOffset);
	if (offset>=iMinLength)
		reduce = aCell->len;						// length of entire free cell
	else
		reduce = offset + aCell->len - iMinLength;	// length of free cell past minimum heap size
	reduce = _ALIGN_DOWN(reduce, iPageSize);		// round down to page multiple
	if (reduce<=0)
		return 0;									// can't reduce this heap
	TInt new_cell_len = aCell->len - reduce;		// length of last free cell after reduction
	if (new_cell_len == 0)
		{
		// the free cell can be entirely eliminated
		SCell* pP = &iFree;
		for (; pP->next!=aCell; pP=pP->next) {}
		pP->next = NULL;
		}
	else
		{
		if (new_cell_len < iMinCell)
			{
			// max reduction would leave a cell too small
			reduce -= iPageSize;
			new_cell_len += iPageSize;
			}
		aCell->len = new_cell_len;	// reduce the cell length
		}
	iTop -= reduce;
	TInt new_len = _ALIGN_UP(iTop - ((TUint8*)this - iOffset), iPageSize);
	TInt r = SetBrk(new_len);
	__ASSERT_ALWAYS(r==KErrNone, HEAP_PANIC(ETHeapReduceFailed));
	return reduce;
	}




#ifndef __KERNEL_MODE__
EXPORT_C void RHeap::Reset()
/**
Frees all allocated cells on this heap.
*/
	{

	Lock();
	if (!(iFlags & EFixedSize))
		{
		TInt r = SetBrk(iMinLength);
		__ASSERT_ALWAYS(r==KErrNone, HEAP_PANIC(ETHeapResetFailed));
		}
	Initialise();
	Unlock();
	}
#endif




inline void RHeap::FindFollowingFreeCell(SCell* aCell, SCell*& aPrev, SCell*& aNext)
//
// Find the free cell that immediately follows aCell, if one exists
// If found, aNext is set to point to it, else it is set to NULL.
// aPrev is set to the free cell before aCell or the dummy free cell where there are no free cells before aCell.
// Called with lock enabled.
//
	{
	aPrev = &iFree;
	aNext = aPrev->next;
	for (; aNext && aNext<aCell; aPrev=aNext, aNext=aNext->next) {}	
	
	if (aNext) // If there is a following free cell, check its directly after aCell.
		{
			SCell* pNextCell = __NEXT_CELL(aCell);			// end of this cell
			__ASSERT_ALWAYS(pNextCell<=aNext, (Unlock(), HEAP_PANIC(ETHeapReAllocBadNextCell)));	// Following free cell overlaps
			if (pNextCell!=aNext) 
				aNext=NULL;		
		}
	}




TInt RHeap::TryToGrowCell(SCell* aCell,SCell* aPrev, SCell* aNext, TInt aSize)
//
// Try to grow the heap cell 'aCell' in place, to size 'aSize'.
// Requires the free cell immediately after aCell (aNext), and the free cell prior to
// that (aPrev), to be provided.  (As found by FindFollowingFreeCell)
//

	{
	TInt extra = aSize - aCell->len;
	if (aNext && (aNext->len>=extra)) // Is there a following free cell big enough?
		{
		if (aNext->len - extra >= iMinCell)	// take part of free cell ?
			{
			SCell* pX = (SCell*)((TUint8*)aNext + extra);	// remainder of free cell
			pX->next = aNext->next;			// remainder->next = original free cell->next
			pX->len = aNext->len - extra;		// remainder length = original free cell length - extra
			aPrev->next = pX;					// put remainder into free chain
			}
		else
			{
			extra = aNext->len;					// Take whole free cell
			aPrev->next = aNext->next;			// remove from free chain
			}
#ifdef __KERNEL_MODE__
		memclr(((TUint8*)aCell) + aCell->len, extra);
#endif		
		aCell->len += extra;					// update reallocated cell length
		iTotalAllocSize += extra;
		return KErrNone;
		}
	return KErrGeneral;  // No space to grow cell
	}




// UEXPORT_C TAny* RHeap::ReAlloc(TAny* aCell, TInt aSize, TInt aMode)
/**
Increases or decreases the size of an existing cell in the heap.

If the cell is being decreased in size, then it is guaranteed not to move,
and the function returns the pointer originally passed in aCell. Note that the
length of the cell will be the same if the difference between the old size
and the new size is smaller than the minimum cell size.

If the cell is being increased in size, i.e. aSize is bigger than its
current size, then the function tries to grow the cell in place.
If successful, then the function returns the pointer originally
passed in aCell. If unsuccessful, then:

1. if the cell cannot be moved, i.e. aMode has the ENeverMove bit set, then
   the function returns NULL.
2. if the cell can be moved, i.e. aMode does not have the ENeverMove bit set,
   then the function tries to allocate a new replacement cell, and, if
   successful, returns a pointer to the new cell; if unsuccessful, it
   returns NULL.

Note that in debug mode, the function returns NULL if the cell cannot be grown
in place, regardless of whether the ENeverMove bit is set.

If the reallocated cell is at a different location from the original cell, then
the content of the original cell is copied to the reallocated cell.

If the supplied pointer, aCell is NULL, then the function attempts to allocate
a new cell, but only if the cell can be moved, i.e. aMode does not have
the ENeverMove bit set.

Note the following general points:

1. If reallocation fails, the content of the original cell is preserved.

2. The resulting size of the re-allocated cell may be rounded up to a value
   greater than aSize, but is guaranteed to be not less than aSize.
 
@param aCell A pointer to the cell to be reallocated. This may be NULL.

@param aSize The new size of the cell. This may be bigger or smaller than the
             size of the original cell.
             
@param aMode Flags controlling the reallocation. The only bit which has any
             effect on this function is that defined by the enumeration
             ENeverMove of the enum RAllocator::TReAllocMode.
             If this is set, then any successful reallocation guarantees not
             to have changed the start address of the cell.
             By default, this parameter is zero.

@return A pointer to the reallocated cell. This may be the same as the original
        pointer supplied through aCell. NULL if there is insufficient memory to
        reallocate the cell, or to grow it in place.

@panic USER 42, if aCell is not NULL, and does not point to a valid cell.
@panic USER 47, if the maximum unsigned value of aSize is greater
                than or equal to KMaxTInt/2. For example,
                calling ReAlloc(someptr,-1) raises this panic.

@see RAllocator::TReAllocMode
*/
UEXPORT_C TAny* RHeap::ReAlloc(TAny* aCell, TInt aSize, TInt aMode)
	{
	if (aCell && iFlags&EMonitorMemory)
		__MEMORY_MONITOR_CHECK_CELL(aCell);
	TAny* retval = ReAllocImpl(aCell, aSize, aMode);
	if (iFlags & ETraceAllocs)
		{
		if (retval)
			{
			TUint32 traceData[3];
			traceData[0] = AllocLen(retval);
			traceData[1] = aSize;
			traceData[2] = (TUint32)aCell;
			BTraceContextN(BTrace::EHeap, BTrace::EHeapReAlloc,(TUint32)this, (TUint32)retval,traceData, sizeof(traceData));
			}
		else
			BTraceContext12(BTrace::EHeap, BTrace::EHeapReAllocFail, (TUint32)this, (TUint32)aCell, (TUint32)aSize);
		}
	return retval;
	}
inline TAny* RHeap::ReAllocImpl(TAny* aCell, TInt aSize, TInt aMode)
	{
	__CHECK_THREAD_STATE;
	if (!aCell)
		return (aMode & ENeverMove) ? NULL : Alloc(aSize);
	__ASSERT_ALWAYS((TUint)aSize<(KMaxTInt/2),HEAP_PANIC(ETHeapBadAllocatedCellSize));
	Lock();
	SCell* pC = GetAddress(aCell);
	TInt old_len = pC->len;
	__DEBUG_SAVE(pC);
	aSize = Max(Align(aSize + EAllocCellSize), iMinCell);
	if (aSize > old_len)	// Trying to grow cell
		{
		__SIMULATE_ALLOC_FAIL({	Unlock(); return NULL;})			
		
		// Try to grow cell in place, without reallocation
		SCell* pPrev;
		SCell* pNext;
		FindFollowingFreeCell(pC,pPrev, pNext);
		TInt r = TryToGrowCell(pC, pPrev, pNext, aSize);
		
		if (r==KErrNone) 
			{
			Unlock();
			return aCell;
			}

		if (!(aMode & ENeverMove))
		// If moving allowed, try re-alloc. 
		// If we need to extend heap,and cell is at the end, try and grow in place
			{
			SCell* pLastFree;
			SCell* pNewCell = (SCell*)DoAlloc(aSize, pLastFree);
			if (!pNewCell && !(iFlags & EFixedSize))
			// if we need to extend the heap to alloc
				{
				if (IsLastCell(pC) || (pNext && IsLastCell(pNext)))
				// if last used Cell, try and extend heap and then cell 
					{
					TInt r = TryToGrowHeap(aSize - old_len, pLastFree);
					if (r==KErrNone)
						{
						r = TryToGrowCell(pC, pPrev, pPrev->next, aSize);
						Unlock();
						__ASSERT_DEBUG(r == KErrNone, HEAP_PANIC(ETHeapCellDidntGrow));						
						return aCell;
						}
					}
				else
				// try to grow chunk heap and Alloc on it
					{
					TInt r = TryToGrowHeap(aSize, pLastFree);
					if (r==KErrNone)
						pNewCell = DoAlloc(aSize, pLastFree);
					}
				}

			if (pNewCell)
			// if we created a new cell, adjust tellies, copy the contents and delete old cell.
				{
				iCellCount++;
				iTotalAllocSize += (pNewCell->len - EAllocCellSize);

				Unlock();
				TUint8* raw = ((TUint8*) pNewCell);
				
				memcpy(raw + EAllocCellSize, aCell, old_len - EAllocCellSize);
#ifdef __KERNEL_MODE__
				memclr(raw + old_len, pNewCell->len - old_len);
#endif		
				Free(aCell);
				__DEBUG_RESTORE(raw + EAllocCellSize);
				return raw + EAllocCellSize;
				}
			}
		else 
		// No moving, but still posible to extend the heap (if heap extendable)
			{
			if (!(iFlags & EFixedSize) && (IsLastCell(pC) || (pNext && IsLastCell(pNext))))
				{
				SCell* pLastFree = pNext ? pNext : pPrev;
				TInt r = TryToGrowHeap(aSize - old_len, pLastFree);
				if (r==KErrNone)
					{
					r = TryToGrowCell(pC, pPrev, pPrev->next, aSize);
					Unlock();
					__ASSERT_DEBUG(r==KErrNone, HEAP_PANIC(ETHeapCellDidntGrow));					
					return aCell;
					}
				}
			}			
		Unlock();
		return NULL;
		}
	if (old_len - aSize >= iMinCell)
		{
		// cell shrinking, remainder big enough to form a new free cell
		SCell* pX = (SCell*)((TUint8*)pC + aSize);	// pointer to new free cell
		pC->len = aSize;			// update cell size
		pX->len = old_len - aSize;	// size of remainder
		iTotalAllocSize -= pX->len;
		DoFree(pX);					// link new free cell into chain, shrink heap if necessary
		}
	Unlock();
	return aCell;
	}




#ifndef __KERNEL_MODE__

EXPORT_C TInt RHeap::Available(TInt& aBiggestBlock) const
/**
Gets the total free space currently available on the heap and the space 
available in the largest free block.

The space available represents the total space which can be allocated.

Note that compressing the heap may reduce the total free space available and 
the space available in the largest free block.

@param aBiggestBlock On return, contains the space available 
                     in the largest free block on the heap.
                     
@return The total free space currently available on the heap.
*/
	{

	TInt total = 0;
	TInt max = 0;
	Lock();
	SCell* pC = iFree.next;
	for (; pC; pC=pC->next)
		{
		TInt l = pC->len - EAllocCellSize;
		if (l > max)
			max = l;
		total += l;
		}
	Unlock();
	aBiggestBlock = max;
	return total;
	}




EXPORT_C TInt RHeap::AllocSize(TInt& aTotalAllocSize) const
/**
Gets the number of cells allocated on this heap, and the total space 
allocated to them.

@param aTotalAllocSize On return, contains the total space allocated
                       to the cells.

@return The number of cells allocated on this heap.
*/
	{
	Lock();
	TInt c = iCellCount;
	aTotalAllocSize = iTotalAllocSize;
	Unlock();
	return c;
	}




EXPORT_C RHeap* UserHeap::FixedHeap(TAny* aBase, TInt aMaxLength, TInt aAlign, TBool aSingleThread)
/**
Creates a fixed length heap at a specified location.

On successful return from this function, aMaxLength bytes are committed by the chunk.
The heap cannot be extended.

@param aBase         A pointer to the location where the heap is to be constructed.
@param aMaxLength    The length of the heap. If the supplied value is less
                     than KMinHeapSize, it is discarded and the value KMinHeapSize
                     is used instead.
@param aAlign        The alignment of heap cells.
@param aSingleThread Indicates whether single threaded or not.

@return A pointer to the new heap, or NULL if the heap could not be created.

@panic USER 56 if aMaxLength is negative.
@panic USER 172 if aAlign is not a power of 2 or is less than the size of a TAny*.
*/
//
// Force construction of the fixed memory.
//
	{

	__ASSERT_ALWAYS(aMaxLength>=0, ::Panic(ETHeapMaxLengthNegative));
	if (aMaxLength<KMinHeapSize)
		aMaxLength=KMinHeapSize;
	RHeap* h = new(aBase) RHeap(aMaxLength, aAlign, aSingleThread);
	if (!aSingleThread)
		{
		TInt r = h->iLock.CreateLocal();
		if (r!=KErrNone)
			return NULL;
		h->iHandles = (TInt*)&h->iLock;
		h->iHandleCount = 1;
		}
	return h;
	}


/**
Constructor where minimum and maximum length of the heap can be defined.
It defaults the chunk heap to be created to have use a new local chunk, 
to have a grow by value of KMinHeapGrowBy, to be unaligned, not to be 
single threaded and not to have any mode flags set.

@param aMinLength    The minimum length of the heap to be created.
@param aMaxLength    The maximum length to which the heap to be created can grow.
                     If the supplied value is less than KMinHeapSize, then it
                     is discarded and the value KMinHeapSize used instead.
*/
EXPORT_C TChunkHeapCreateInfo::TChunkHeapCreateInfo(TInt aMinLength, TInt aMaxLength) :
	iVersionNumber(EVersion0), iMinLength(aMinLength), iMaxLength(aMaxLength),
	iAlign(0), iGrowBy(1), iSingleThread(EFalse), 
	iOffset(0), iPaging(EUnspecified), iMode(0), iName(NULL)
	{
	}


/**
Sets the chunk heap to create a new chunk with the specified name.

This overriddes any previous call to TChunkHeapCreateInfo::SetNewChunkHeap() or
TChunkHeapCreateInfo::SetExistingChunkHeap() for this TChunkHeapCreateInfo object.

@param aName	The name to be given to the chunk heap to be created
				If NULL, the function constructs a local chunk to host the heap.
				If not NULL, a pointer to a descriptor containing the name to be 
				assigned to the global chunk hosting the heap.
*/
EXPORT_C void TChunkHeapCreateInfo::SetCreateChunk(const TDesC* aName)
	{
	iName = (TDesC*)aName;
	iChunk.SetHandle(KNullHandle);
	}


/**
Sets the chunk heap to be created to use the chunk specified.

This overriddes any previous call to TChunkHeapCreateInfo::SetNewChunkHeap() or
TChunkHeapCreateInfo::SetExistingChunkHeap() for this TChunkHeapCreateInfo object.

@param aChunk	A handle to the chunk to use for the heap.
*/
EXPORT_C void TChunkHeapCreateInfo::SetUseChunk(const RChunk aChunk)
	{
	iName = NULL;
	iChunk = aChunk;
	}


/**
Creates a chunk heap of the type specified by the parameter aCreateInfo.

@param aCreateInfo	A reference to a TChunkHeapCreateInfo object specifying the
					type of chunk heap to create.

@return A pointer to the new heap or NULL if the heap could not be created.

@panic USER 41 if the heap's specified minimum length is greater than the specified maximum length.
@panic USER 55 if the heap's specified minimum length is negative.
@panic USER 172 if the heap's specified alignment is not a power of 2 or is less than the size of a TAny*.
*/
EXPORT_C RHeap* UserHeap::ChunkHeap(const TChunkHeapCreateInfo& aCreateInfo)
	{
	// aCreateInfo must have been configured to use a new chunk or an exiting chunk.
	__ASSERT_ALWAYS(!(aCreateInfo.iMode & (TUint32)~EChunkHeapMask), ::Panic(EHeapCreateInvalidMode));
	RHeap* h = NULL;

	if (aCreateInfo.iChunk.Handle() == KNullHandle)
		{// A new chunk is to be created for this heap.
		__ASSERT_ALWAYS(aCreateInfo.iMinLength >= 0, ::Panic(ETHeapMinLengthNegative));
		__ASSERT_ALWAYS(aCreateInfo.iMaxLength >= aCreateInfo.iMinLength, ::Panic(ETHeapCreateMaxLessThanMin));

		TInt maxLength = aCreateInfo.iMaxLength;
		if (maxLength < KMinHeapSize)
			maxLength = KMinHeapSize;

		TChunkCreateInfo chunkInfo;
		chunkInfo.SetNormal(0, maxLength);
		chunkInfo.SetOwner((aCreateInfo.iSingleThread)? EOwnerThread : EOwnerProcess);
		if (aCreateInfo.iName)
			chunkInfo.SetGlobal(*aCreateInfo.iName);
		// Set the paging attributes of the chunk.
		if (aCreateInfo.iPaging == TChunkHeapCreateInfo::EPaged)
			chunkInfo.SetPaging(TChunkCreateInfo::EPaged);
		if (aCreateInfo.iPaging == TChunkHeapCreateInfo::EUnpaged)
			chunkInfo.SetPaging(TChunkCreateInfo::EUnpaged);
		// Create the chunk.
		RChunk chunk;
		if (chunk.Create(chunkInfo) != KErrNone)
			return NULL;
		// Create the heap using the new chunk.
		TUint mode = aCreateInfo.iMode | EChunkHeapDuplicate;	// Must duplicate the handle.
		h = OffsetChunkHeap(chunk, aCreateInfo.iMinLength, aCreateInfo.iOffset,
							aCreateInfo.iGrowBy, maxLength, aCreateInfo.iAlign,
							aCreateInfo.iSingleThread, mode);
		chunk.Close();
		}
	else
		{
		h = OffsetChunkHeap(aCreateInfo.iChunk, aCreateInfo.iMinLength, aCreateInfo.iOffset,
							aCreateInfo.iGrowBy, aCreateInfo.iMaxLength, aCreateInfo.iAlign,
							aCreateInfo.iSingleThread, aCreateInfo.iMode);
		}
	return h;
	}


EXPORT_C RHeap* UserHeap::ChunkHeap(const TDesC* aName, TInt aMinLength, TInt aMaxLength, TInt aGrowBy, TInt aAlign, TBool aSingleThread)
/**
Creates a heap in a local or global chunk.

The chunk hosting the heap can be local or global.

A local chunk is one which is private to the process creating it and is not
intended for access by other user processes.
A global chunk is one which is visible to all processes.

The hosting chunk is local, if the pointer aName is NULL, otherwise
the hosting chunk is global and the descriptor *aName is assumed to contain
the name to be assigned to it.

Ownership of the host chunk is vested in the current process.

A minimum and a maximum size for the heap can be specified. On successful
return from this function, the size of the heap is at least aMinLength.
If subsequent requests for allocation of memory from the heap cannot be
satisfied by compressing the heap, the size of the heap is extended in
increments of aGrowBy until the request can be satisfied. Attempts to extend
the heap causes the size of the host chunk to be adjusted.

Note that the size of the heap cannot be adjusted by more than aMaxLength.

@param aName         If NULL, the function constructs a local chunk to host
                     the heap.
                     If not NULL, a pointer to a descriptor containing the name
                     to be assigned to the global chunk hosting the heap.
@param aMinLength    The minimum length of the heap.
@param aMaxLength    The maximum length to which the heap can grow.
                     If the supplied value is less than KMinHeapSize, then it
                     is discarded and the value KMinHeapSize used instead.
@param aGrowBy       The increments to the size of the host chunk. If a value is
                     not explicitly specified, the value KMinHeapGrowBy is taken
                     by default
@param aAlign        The alignment of heap cells.
@param aSingleThread Indicates whether single threaded or not.

@return A pointer to the new heap or NULL if the heap could not be created.

@panic USER 41 if aMinLength is greater than the supplied value of aMaxLength.
@panic USER 55 if aMinLength is negative.
@panic USER 172 if aAlign is not a power of 2 or is less than the size of a TAny*.
*/
//
// Allocate a Chunk of the requested size and force construction.
//
	{
	TChunkHeapCreateInfo createInfo(aMinLength, aMaxLength);
	createInfo.SetCreateChunk(aName);
	createInfo.SetGrowBy(aGrowBy);
	createInfo.SetAlignment(aAlign);
	createInfo.SetSingleThread(aSingleThread);
	return ChunkHeap(createInfo);
	}




EXPORT_C RHeap* UserHeap::ChunkHeap(RChunk aChunk, TInt aMinLength, TInt aGrowBy, TInt aMaxLength, TInt aAlign, TBool aSingleThread, TUint32 aMode)
/**
Creates a heap in an existing chunk.

This function is intended to be used to create a heap in a user writable code
chunk as created by a call to RChunk::CreateLocalCode().
This type of heap can be used to hold code fragments from a JIT compiler.

The maximum length to which the heap can grow is the same as
the maximum size of the chunk.

@param aChunk        The chunk that will host the heap.
@param aMinLength    The minimum length of the heap.
@param aGrowBy       The increments to the size of the host chunk. 
@param aMaxLength    The maximum length to which the heap can grow.
@param aAlign        The alignment of heap cells.
@param aSingleThread Indicates whether single threaded or not.
@param aMode         Flags controlling the heap creation.  This should be set 
					 from one or more of the values in TChunkHeapCreateMode.
                     
@return A pointer to the new heap or NULL if the heap could not be created.

@panic USER 172 if aAlign is not a power of 2 or is less than the size of a TAny*.
*/
//
// Construct a heap in an already existing chunk
//
	{
	
	return OffsetChunkHeap(aChunk, aMinLength, 0, aGrowBy, aMaxLength, aAlign, aSingleThread, aMode);
	}




EXPORT_C RHeap* UserHeap::OffsetChunkHeap(RChunk aChunk, TInt aMinLength, TInt aOffset, TInt aGrowBy, TInt aMaxLength, TInt aAlign, TBool aSingleThread, TUint32 aMode)
/**
Creates a heap in an existing chunk, offset from the beginning of the chunk.

This function is intended to be used to create a heap where a fixed amount of
additional data must be stored at a known location. The additional data can be
placed at the base address of the chunk, allowing it to be located without
depending on the internals of the heap structure.

The maximum length to which the heap can grow is the maximum size of the chunk,
minus the offset.

@param aChunk        The chunk that will host the heap.
@param aMinLength    The minimum length of the heap.
@param aOffset       The offset from the start of the chunk, to the start of the heap.
@param aGrowBy       The increments to the size of the host chunk. 
@param aMaxLength    The maximum length to which the heap can grow.
@param aAlign        The alignment of heap cells.
@param aSingleThread Indicates whether single threaded or not.
@param aMode         Flags controlling the heap creation.  This should be set 
					 from one or more of the values in TChunkHeapCreateMode.
                     
@return A pointer to the new heap or NULL if the heap could not be created.

@panic USER 172 if aAlign is not a power of 2 or is less than the size of a TAny*.
*/
//
// Construct a heap in an already existing chunk
//
	{

	TInt page_size;
	UserHal::PageSizeInBytes(page_size);
	if (!aAlign)
		aAlign = RHeap::ECellAlignment;
	TInt maxLength = aChunk.MaxSize();
	TInt round_up = Max(aAlign, page_size);
	TInt min_cell = _ALIGN_UP(Max((TInt)RHeap::EAllocCellSize, (TInt)RHeap::EFreeCellSize), aAlign);
	aOffset = _ALIGN_UP(aOffset, 8);
	if (aMaxLength && aMaxLength+aOffset<maxLength)
		maxLength = _ALIGN_UP(aMaxLength+aOffset, round_up);
	__ASSERT_ALWAYS(aMinLength>=0, ::Panic(ETHeapMinLengthNegative));
	__ASSERT_ALWAYS(maxLength>=aMinLength, ::Panic(ETHeapCreateMaxLessThanMin));
	aMinLength = _ALIGN_UP(Max(aMinLength, (TInt)sizeof(RHeap) + min_cell) + aOffset, round_up);
	TInt r=aChunk.Adjust(aMinLength);
	if (r!=KErrNone)
		return NULL;

	RHeap* h = new (aChunk.Base() + aOffset) RHeap(aChunk.Handle(), aOffset, aMinLength, maxLength, aGrowBy, aAlign, aSingleThread);

	TBool duplicateLock = EFalse;
	if (!aSingleThread)
		{
		duplicateLock = aMode & EChunkHeapSwitchTo;
		if(h->iLock.CreateLocal(duplicateLock ? EOwnerThread : EOwnerProcess)!=KErrNone)
			{
			h->iChunkHandle = 0;
			return NULL;
			}
		}

	if (aMode & EChunkHeapSwitchTo)
		User::SwitchHeap(h);

	h->iHandles = &h->iChunkHandle;
	if (!aSingleThread)
		{
		// now change the thread-relative chunk/semaphore handles into process-relative handles
		h->iHandleCount = 2;
		if(duplicateLock)
			{
			RHandleBase s = h->iLock;
			r = h->iLock.Duplicate(RThread());
			s.Close();
			}
		if (r==KErrNone && (aMode & EChunkHeapDuplicate))
			{
			r = ((RChunk*)&h->iChunkHandle)->Duplicate(RThread());
			if (r!=KErrNone)
				h->iLock.Close(), h->iChunkHandle=0;
			}
		}
	else
		{
		h->iHandleCount = 1;
		if (aMode & EChunkHeapDuplicate)
			r = ((RChunk*)&h->iChunkHandle)->Duplicate(RThread(), EOwnerThread);
		}

	// return the heap address
	return (r==KErrNone) ? h : NULL;
	}



#define UserTestDebugMaskBit(bit) (TBool)(UserSvr::DebugMask(bit>>5) & (1<<(bit&31)))

_LIT(KLitDollarHeap,"$HEAP");
EXPORT_C TInt UserHeap::CreateThreadHeap(SStdEpocThreadCreateInfo& aInfo, RHeap*& aHeap, TInt aAlign, TBool aSingleThread)
/**
@internalComponent
*/
//
// Create a user-side heap
//
	{
	TInt page_size;
	UserHal::PageSizeInBytes(page_size);
	TInt minLength = _ALIGN_UP(aInfo.iHeapInitialSize, page_size);
	TInt maxLength = Max(aInfo.iHeapMaxSize, minLength);
	if (UserTestDebugMaskBit(96)) // 96 == KUSERHEAPTRACE in nk_trace.h
		aInfo.iFlags |= ETraceHeapAllocs;

	// Create the thread's heap chunk.
	RChunk c;
	TChunkCreateInfo createInfo;
	createInfo.SetThreadHeap(0, maxLength, KLitDollarHeap());	// Initialise with no memory committed.

	// Set the paging policy of the heap chunk based on the thread's paging policy.
	TUint pagingflags = aInfo.iFlags & EThreadCreateFlagPagingMask;
	switch (pagingflags)
		{
		case EThreadCreateFlagPaged:
			createInfo.SetPaging(TChunkCreateInfo::EPaged);
			break;
		case EThreadCreateFlagUnpaged:
			createInfo.SetPaging(TChunkCreateInfo::EUnpaged);
			break;
		case EThreadCreateFlagPagingUnspec:
			// Leave the chunk paging policy unspecified so the process's 
			// paging policy is used.
			break;
		}

	TInt r = c.Create(createInfo);
	if (r!=KErrNone)
		return r;

	aHeap = ChunkHeap(c, minLength, page_size, maxLength, aAlign, aSingleThread, EChunkHeapSwitchTo|EChunkHeapDuplicate);
	c.Close();
	if (!aHeap)
		return KErrNoMemory;
	if (aInfo.iFlags & ETraceHeapAllocs)
		{
		aHeap->iFlags |= RHeap::ETraceAllocs;
		BTraceContext8(BTrace::EHeap, BTrace::EHeapCreate,(TUint32)aHeap, RHeap::EAllocCellSize);
		TInt handle = aHeap->ChunkHandle();
		TInt chunkId = ((RHandleBase&)handle).BTraceId();
		BTraceContext8(BTrace::EHeap, BTrace::EHeapChunkCreate, (TUint32)aHeap, chunkId);
		}
	if (aInfo.iFlags & EMonitorHeapMemory)
		aHeap->iFlags |= RHeap::EMonitorMemory;
	return KErrNone;
	}

#endif	// __KERNEL_MODE__

void RHeap::WalkCheckCell(TAny* aPtr, TCellType aType, TAny* aCell, TInt aLen)
	{
	(void)aCell;
	SHeapCellInfo& info = *(SHeapCellInfo*)aPtr;
	switch(aType)
		{
		case EGoodAllocatedCell:
			{
			++info.iTotalAlloc;
			info.iTotalAllocSize += (aLen-EAllocCellSize);
#if defined(_DEBUG)
			RHeap& h = *info.iHeap;
			if ( ((SDebugCell*)aCell)->nestingLevel == h.iNestingLevel )
				{
				if (++info.iLevelAlloc==1)
					info.iStranded = (SDebugCell*)aCell;
#ifdef __KERNEL_MODE__
				if (KDebugNum(KSERVER) || KDebugNum(KTESTFAST))
					{
//				__KTRACE_OPT(KSERVER,Kern::Printf("LEAKED KERNEL HEAP CELL @ %08x : len=%d", aCell, aLen));
					Kern::Printf("LEAKED KERNEL HEAP CELL @ %08x : len=%d", aCell, aLen);
					TLinAddr base = ((TLinAddr)aCell)&~0x0f;
					TLinAddr end = ((TLinAddr)aCell)+(TLinAddr)aLen;
					while(base<end)
						{
						const TUint32* p = (const TUint32*)base;
						Kern::Printf("%08x: %08x %08x %08x %08x", p, p[0], p[1], p[2], p[3]);
						base += 16;
						}
					}
#endif
				}
#endif	
			break;
			}
		case EGoodFreeCell:
			++info.iTotalFree;
			break;
		case EBadAllocatedCellSize:
			HEAP_PANIC(ETHeapBadAllocatedCellSize);
		case EBadAllocatedCellAddress:
			HEAP_PANIC(ETHeapBadAllocatedCellAddress);
		case EBadFreeCellAddress:
			HEAP_PANIC(ETHeapBadFreeCellAddress);
		case EBadFreeCellSize:
			HEAP_PANIC(ETHeapBadFreeCellSize);
		default:
			HEAP_PANIC(ETHeapWalkBadCellType);
		}
	}

TInt RHeap::DoCountAllocFree(TInt& aFree)
	{
	SHeapCellInfo info;
	memclr(&info, sizeof(info));
	info.iHeap = this;
	Walk(&WalkCheckCell, &info);
	aFree = info.iTotalFree;
	return info.iTotalAlloc;
	}


UEXPORT_C TInt RHeap::DebugFunction(TInt aFunc, TAny* a1, TAny* a2)
/**
@internalComponent
*/
	{
	TInt r = KErrNone;
	switch(aFunc)
		{
		case RAllocator::ECount:
			r = DoCountAllocFree(*(TInt*)a1);
			break;
		case RAllocator::EMarkStart:
			__DEBUG_ONLY(DoMarkStart());
			break;
		case RAllocator::EMarkEnd:
			__DEBUG_ONLY( r = DoMarkEnd((TInt)a1) );
			break;
		case RAllocator::ECheck:
			r = DoCheckHeap((SCheckInfo*)a1);
			break;
		case RAllocator::ESetFail:
			__DEBUG_ONLY(DoSetAllocFail((TAllocFail)(TInt)a1, (TInt)a2));
			break;
		case RAllocator::ESetBurstFail:
#if _DEBUG
			{
			SRAllocatorBurstFail* fail = (SRAllocatorBurstFail*) a2;
			DoSetAllocFail((TAllocFail)(TInt)a1, fail->iRate, fail->iBurst);
			}
#endif
			break;

		case RAllocator::ECheckFailure:
				// iRand will be incremented for each EFailNext, EBurstFailNext,
				// EDeterministic and EBurstDeterministic failure.
				r = iRand;
				break;

		case RAllocator::ECopyDebugInfo:
			{
			TInt nestingLevel = ((SDebugCell*)a1)[-1].nestingLevel;
			((SDebugCell*)a2)[-1].nestingLevel = nestingLevel;
			break;
			}
		case RHeap::EWalk:
			Walk((TWalkFunc)a1, a2);
			break;
		default:
			return KErrNotSupported;
		}
	return r;
	}




void RHeap::Walk(TWalkFunc aFunc, TAny* aPtr)
//
// Walk the heap calling the info function.
//
	{

	Lock();
	SCell* pC = (SCell*)iBase;		// allocated cells
	SCell* pF = &iFree;				// free cells
	FOREVER
		{
		pF = pF->next;				// next free cell
		if (!pF)
			pF = (SCell*)iTop;		// to make size checking work
		else if ( (TUint8*)pF>=iTop || (pF->next && pF->next<=pF) )
			{
			if (iFlags & ETraceAllocs)
				BTraceContext12(BTrace::EHeap, BTrace::EHeapCorruption, (TUint32)this, (TUint32)pF+EFreeCellSize, 0);
			// free cell pointer off the end or going backwards
			Unlock();
			(*aFunc)(aPtr, EBadFreeCellAddress, pF, 0);
			return;
			}
		else
			{
			TInt l = pF->len;
			if (l<iMinCell || (l & (iAlign-1)))
				{
				if (iFlags & ETraceAllocs)
					BTraceContext12(BTrace::EHeap, BTrace::EHeapCorruption, (TUint32)this, (TUint32)pF+EFreeCellSize, l-EFreeCellSize);
				// free cell length invalid
				Unlock();
				(*aFunc)(aPtr, EBadFreeCellSize, pF, l);
				return;
				}
			}
		while (pC!=pF)				// walk allocated cells up to next free cell
			{
			TInt l = pC->len;
			if (l<iMinCell || (l & (iAlign-1)))
				{
				if (iFlags & ETraceAllocs)
					BTraceContext12(BTrace::EHeap, BTrace::EHeapCorruption, (TUint32)this, (TUint32)pC+EAllocCellSize, l-EAllocCellSize);
				// allocated cell length invalid
				Unlock();
				(*aFunc)(aPtr, EBadAllocatedCellSize, pC, l);
				return;
				}
			(*aFunc)(aPtr, EGoodAllocatedCell, pC, l);
			SCell* pN = __NEXT_CELL(pC);
			if (pN > pF)
				{
				if (iFlags & ETraceAllocs)
					BTraceContext12(BTrace::EHeap, BTrace::EHeapCorruption, (TUint32)this, (TUint32)pC+EAllocCellSize, l-EAllocCellSize);			
				// cell overlaps next free cell
				Unlock();
				(*aFunc)(aPtr, EBadAllocatedCellAddress, pC, l);
				return;
				}
			pC = pN;
			}
		if ((TUint8*)pF == iTop)
			break;		// reached end of heap
		pC = __NEXT_CELL(pF);	// step to next allocated cell
		(*aFunc)(aPtr, EGoodFreeCell, pF, pF->len);
		}
	Unlock();
	}

TInt RHeap::DoCheckHeap(SCheckInfo* aInfo)
	{
	(void)aInfo;
	SHeapCellInfo info;
	memclr(&info, sizeof(info));
	info.iHeap = this;
	Walk(&WalkCheckCell, &info);
#if defined(_DEBUG)
	if (!aInfo)
		return KErrNone;
	TInt expected = aInfo->iCount;
	TInt actual = aInfo->iAll ? info.iTotalAlloc : info.iLevelAlloc;
	if (actual!=expected && !iTestData)
		{
#ifdef __KERNEL_MODE__
		Kern::Fault("KERN-ALLOC COUNT", (expected<<16)|actual );
#else
		User::Panic(_L("ALLOC COUNT"), (expected<<16)|actual );
#endif
		}
#endif
	return KErrNone;
	}

#ifdef _DEBUG
void RHeap::DoMarkStart()
	{
	if (iNestingLevel==0)
		iAllocCount=0;
	iNestingLevel++;
	}

TUint32 RHeap::DoMarkEnd(TInt aExpected)
	{
	if (iNestingLevel==0)
		return 0;
	SHeapCellInfo info;
	SHeapCellInfo* p = iTestData ? (SHeapCellInfo*)iTestData : &info;
	memclr(p, sizeof(info));
	p->iHeap = this;
	Walk(&WalkCheckCell, p);
	if (p->iLevelAlloc != aExpected && !iTestData)
		return (TUint32)(p->iStranded + 1);
	if (--iNestingLevel == 0)
		iAllocCount = 0;
	return 0;
	}

void ResetAllocCellLevels(TAny* aPtr, RHeap::TCellType aType, TAny* aCell, TInt aLen)
	{
	(void)aPtr;
	(void)aLen;
	RHeap::SDebugCell* cell = (RHeap::SDebugCell*)aCell;
	if (aType == RHeap::EGoodAllocatedCell)
		{
		cell->nestingLevel = 0;
		}
	}

void RHeap::DoSetAllocFail(TAllocFail aType, TInt aRate)
	{// Default to a burst mode of 1, as aType may be a burst type.
	DoSetAllocFail(aType, aRate, 1);
	}

// Don't change as the ETHeapBadDebugFailParameter check below and the API 
// documentation rely on this being 16 for RHeap.
LOCAL_D const TInt KBurstFailRateShift = 16;
LOCAL_D const TInt KBurstFailRateMask = (1 << KBurstFailRateShift) - 1;

void RHeap::DoSetAllocFail(TAllocFail aType, TInt aRate, TUint aBurst)
	{
	if (aType==EReset)
		{
		// reset levels of all allocated cells to 0
		// this should prevent subsequent tests failing unnecessarily
		iFailed = EFalse;		// Reset for ECheckFailure relies on this.
		Walk(&ResetAllocCellLevels, NULL);
		// reset heap allocation mark as well
		iNestingLevel=0;
		iAllocCount=0;
		aType=ENone;
		}

	switch (aType)
		{
		case EBurstRandom:
		case EBurstTrueRandom:
		case EBurstDeterministic:
		case EBurstFailNext:
			// If the fail type is a burst type then iFailRate is split in 2:
			// the 16 lsbs are the fail rate and the 16 msbs are the burst length.
			if (TUint(aRate) > (TUint)KMaxTUint16 || aBurst > KMaxTUint16)
				HEAP_PANIC(ETHeapBadDebugFailParameter);

			iFailed = EFalse;
			iFailType = aType;
			iFailRate = (aRate == 0) ? 1 : aRate;
			iFailAllocCount = -iFailRate;
			iFailRate = iFailRate | (aBurst << KBurstFailRateShift);
			break;

		default:
			iFailed = EFalse;
			iFailType = aType;
			iFailRate = (aRate == 0) ? 1 : aRate; // A rate of <1 is meaningless
			iFailAllocCount = 0;
			break;
		}

	// Set up iRand for either:
	//		- random seed value, or
	//		- a count of the number of failures so far.
	iRand = 0;
#ifndef __KERNEL_MODE__
	switch (iFailType)
		{
		case ETrueRandom:
		case EBurstTrueRandom:
			{
			TTime time;
			time.HomeTime();
			TInt64 seed = time.Int64();
			iRand = Math::Rand(seed);
			break;
			}
		case ERandom:
		case EBurstRandom:
	        {
	        TInt64 seed = 12345;
			iRand = Math::Rand(seed);
			break;
	        }
		default:
			break;
		}
#endif
	}

TBool RHeap::CheckForSimulatedAllocFail()
//
// Check to see if the user has requested simulated alloc failure, and if so possibly 
// Return ETrue indicating a failure.
//
	{
	// For burst mode failures iFailRate is shared
	TUint16 rate  = (TUint16)(iFailRate &  KBurstFailRateMask);
	TUint16 burst = (TUint16)(iFailRate >> KBurstFailRateShift);
	TBool r = EFalse;
	switch (iFailType)
		{
#ifndef __KERNEL_MODE__
		case ERandom:
		case ETrueRandom:
			if (++iFailAllocCount>=iFailRate) 
				{	
				iFailAllocCount=0;
				if (!iFailed) // haven't failed yet after iFailRate allocations so fail now
					return(ETrue); 
				iFailed=EFalse;
				}
			else   
				{
				if (!iFailed)
					{
	                TInt64 seed=iRand;
					iRand=Math::Rand(seed);
					if (iRand%iFailRate==0)
						{
						iFailed=ETrue;
						return(ETrue);
						}
					}
				}
			break;

		case EBurstRandom:
		case EBurstTrueRandom:
			if (++iFailAllocCount < 0) 
				{
				// We haven't started failing yet so should we now?
				TInt64 seed = iRand;
				iRand = Math::Rand(seed);
				if (iRand % rate == 0)
					{// Fail now.  Reset iFailAllocCount so we fail burst times
					iFailAllocCount = 0;
					r = ETrue;
					}
				}
			else
				{
				if (iFailAllocCount < burst)
					{// Keep failing for burst times
					r = ETrue;
					}
				else
					{// We've now failed burst times so start again.
					iFailAllocCount = -(rate - 1);
					}
				}
			break;
#endif
		case EDeterministic:
			if (++iFailAllocCount%iFailRate==0)
				{
				r=ETrue;
				iRand++;	// Keep count of how many times we have failed
				}
			break;

		case EBurstDeterministic:
			// This will fail burst number of times, every rate attempts.
			if (++iFailAllocCount >= 0)
				{
				if (iFailAllocCount == burst - 1)
					{// This is the burst time we have failed so make it the last by
					// reseting counts so we next fail after rate attempts.
					iFailAllocCount = -rate;
					}
				r = ETrue;
				iRand++;	// Keep count of how many times we have failed
				}
			break;

		case EFailNext:
			if ((++iFailAllocCount%iFailRate)==0)
				{
				iFailType=ENone;
				r=ETrue;
				iRand++;	// Keep count of how many times we have failed
				}
			break;

		case EBurstFailNext:
			if (++iFailAllocCount >= 0)
				{
				if (iFailAllocCount == burst - 1)
					{// This is the burst time we have failed so make it the last.
					iFailType = ENone;
					}
				r = ETrue;
				iRand++;	// Keep count of how many times we have failed
				}
			break;
		default:
			break;
		}
	return r;
	}
#endif	// ifdef _DEBUG

UEXPORT_C TInt RHeap::Extension_(TUint aExtensionId, TAny*& a0, TAny* a1)
	{
	return RAllocator::Extension_(aExtensionId, a0, a1);
	}

#if defined(__HEAP_MACHINE_CODED__) && !defined(_DEBUG)
GLDEF_C void RHeap_PanicBadAllocatedCellSize()
	{
	HEAP_PANIC(ETHeapBadAllocatedCellSize);
	}

GLDEF_C void RHeap_PanicBadNextCell()
	{
	HEAP_PANIC(ETHeapFreeBadNextCell);
	}

GLDEF_C void RHeap_PanicBadPrevCell()
	{
	HEAP_PANIC(ETHeapFreeBadPrevCell);
	}

GLDEF_C void RHeap_PanicBadCellAddress()
	{
	HEAP_PANIC(ETHeapBadCellAddress);
	}
#endif





