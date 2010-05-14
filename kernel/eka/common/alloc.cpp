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
// kernel\eka\common\alloc.cpp
// 
//

#include "common.h"
#include <e32atomics.h>


#ifndef __KERNEL_MODE__
/**
Opens this heap for shared access.

Opening the heap increases the heap's access count by one.

@return KErrNone if successful;
        KErrGeneral, if the original valeu of the access count
        was not positive.
*/
EXPORT_C TInt RAllocator::Open()
	{
	TInt c = __e32_atomic_tas_ord32(&iAccessCount, 1, 1, 0);
	return (c>0) ? KErrNone : KErrGeneral;
	}




/**
Closes this shared heap.

Closing the heap decreases the heap's access count by one.

@panic USER 57 if the access count has already reached zero.
*/
EXPORT_C void RAllocator::Close()
	{
	TInt count = __e32_atomic_tas_ord32(&iAccessCount, 1, -1, 0);
	__ASSERT_ALWAYS(count>0, Panic(EAllocatorClosedTooManyTimes));
	if (count==1)
		DoClose();
	}



/**
@internalComponent
*/
EXPORT_C void RAllocator::DoClose()
	{
	__ASSERT_ALWAYS(TUint32(iHandleCount)<=TUint32(EMaxHandles), Panic(EAllocatorBadHandleCount));
	TInt handle[EMaxHandles];
	TInt c = iHandleCount;
	wordmove(handle, iHandles, c*sizeof(TInt));
	memclr(iHandles, c*sizeof(TInt));
	TInt* pH = handle;
	TInt* pE = pH + c;
	while (pH<pE)
		{
		RHandleBase h;
		h.SetHandle(*pH++);
		h.Close();
		}
	}




/**
Allocates a cell of specified size from the heap, and clears
it to binary zeroes.

If there is insufficient memory available on the heap from which to
allocate a cell of the required size, the function returns NULL.

The resulting size of the allocated cell may be rounded up to a value greater 
than aSize, but is guaranteed to be not less than aSize.

@param aSize The size of the cell to be allocated from the current thread's 
             heap.
             
@return A pointer to the allocated cell. NULL, if there is insufficient memory 
        available.
        
@panic USER 47  if the maximum unsigned value of aSize is greater
                than or equal to KMaxTInt/2. For example,
                calling Alloc(-1) raises this panic.
*/
EXPORT_C TAny* RAllocator::AllocZ(TInt aSize)
	{
	TAny* p = Alloc(aSize);
	if (p)
		Mem::FillZ(p, aSize);
	return p;
	}




/**
Allocates a cell of specified size from the heap, clears it to
binary zeroes, and leaves if there is insufficient memory in the heap.

The resulting size of the allocated cell may be rounded up to a value greater 
than aSize, but is guaranteed to be not less than aSize.

@param aSize The size of the cell to be allocated from the heap.

@return A pointer to the allocated cell.
 
@panic USER 47  if the maximum unsigned value of aSize is greater
                than or equal to KMaxTInt/2. For example,
                calling Alloc(-1) raises this panic.
*/
EXPORT_C TAny* RAllocator::AllocZL(TInt aSize)
	{
	TAny* p = AllocL(aSize);
	Mem::FillZ(p, aSize);
	return p;
	}




/**
Allocates a cell of specified size from the heap, and leaves 
if there is insufficient memory in the heap.

The resulting size of the allocated cell may be rounded up to a value greater 
than aSize, but is guaranteed to be not less than aSize.

@param aSize The size of the cell to be allocated from the heap.

@return A pointer to the allocated cell.

@panic USER 47  if the maximum unsigned value of aSize is greater
                than or equal to KMaxTInt/2. For example,
                calling Alloc(-1) raises this panic.
*/
EXPORT_C TAny* RAllocator::AllocL(TInt aSize)
	{
	TAny* p = Alloc(aSize);
	if (!p)
		User::LeaveNoMemory();
	return p;
	}




/**
Allocates a cell of specified size from the heap, and,
if successful, places a pointer to the cell onto the cleanup stack.

The function leaves if there is insufficient memory in the heap.

The resulting size of the allocated cell may be rounded up to a value greater 
than aSize, but is guaranteed to be not less than aSize.

@param aSize The size of the cell to be allocated from the heap.
             
@return A pointer to the allocated cell.

@panic USER 47  if the maximum unsigned value of aSize is greater
                than or equal to KMaxTInt/2. For example,
                calling Alloc(-1) raises this panic.
*/
EXPORT_C TAny* RAllocator::AllocLC(TInt aSize)
	{
	TAny* p = AllocL(aSize);
	CleanupStack::PushL(p);
	return p;
	}




/**
Frees the specified cell, returns it to the heap, and resets 
the pointer to NULL.

@param aCell A reference to a pointer to a valid cell to be freed. If NULL 
             this function call will be ignored.
             
@panic USER 42  if aCell is not NULL and does not point to a valid cell.             
*/
EXPORT_C void RAllocator::FreeZ(TAny*& aCell)
	{
	Free(aCell);
	aCell = NULL;
	}




/**
Increases or decreases the size of an existing cell, and leaves 
if there is insufficient memory in the heap.

If the cell is being decreased in size, then it is guaranteed not to move,
and the function returns the pointer originally passed in aCell. Note that the
length of the cell will be the same if the difference between the old size
and the new size is smaller than the minimum cell size.

If the cell is being increased in size, i.e. aSize is bigger than its
current size, then the function tries to grow the cell in place.
If successful, then the function returns the pointer originally
passed in aCell. If unsuccessful, then:

1. if the cell cannot be moved, i.e. aMode has the ENeverMove bit set, then
   the function leaves.
2. if the cell can be moved, i.e. aMode does not have the ENeverMove bit set,
   then the function tries to allocate a new replacement cell, and, if
   successful, returns a pointer to the new cell; if unsuccessful, it
   leaves.

Note that in debug mode, the function leaves if the cell cannot be grown
in place, regardless of whether the ENeverMove bit is set.

If the reallocated cell is at a different location from the original cell, then
the content of the original cell is copied to the reallocated cell.

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
        pointer supplied through aCell.

@panic USER 42, if aCell is not NULL, and does not point to a valid cell.
@panic USER 47, if the maximum unsigned value of aSize is greater
                than or equal to KMaxTInt/2. For example,
                calling ReAlloc(someptr,-1) raises this panic.

@see RAllocator::TReAllocMode
*/
EXPORT_C TAny* RAllocator::ReAllocL(TAny* aCell, TInt aSize, TInt aMode)
	{
	TAny* p = ReAlloc(aCell, aSize, aMode);
	if (!p)
		User::LeaveNoMemory();
	return p;
	}




/**
Gets the total number of cells allocated on the heap.

@return The number of cells allocated on the heap.
*/
EXPORT_C TInt RAllocator::Count() const
	{
	TInt totalAllocSize; 
	return ((RAllocator*)this)->AllocSize(totalAllocSize);
	}




/**
Gets the the total number of cells allocated, and the number of free cells, 
on the heap.

@param aFreeCount On return, contains the number of free cells 
                  on the heap.

@return The number of cells allocated on the heap.
*/
EXPORT_C TInt RAllocator::Count(TInt& aFreeCount) const
	{
	return ((RAllocator*)this)->DebugFunction(ECount, &aFreeCount);
	}
#endif




/**
Checks the validity of the heap.

The function walks through the list of allocated cells and the list of
free cells checking that this heap is consistent and complete.

@panic USER 47 if any corruption is found, specifically	a bad allocated
               heap cell size.
@panic USER 48 if any corruption is found, specifically a bad allocated
               heap cell address.
@panic USER 49 if any corruption is found, specifically a bad free heap
               cell address.
*/
UEXPORT_C void RAllocator::Check() const
	{
	((RAllocator*)this)->DebugFunction(ECheck);
	}




/**
Marks the start of heap cell checking for this heap.

If earlier calls to __DbgMarkStart() have been made, then this call
to __DbgMarkStart() marks the start of a new nested level of
heap cell checking.

Every call to __DbgMarkStart() should be matched by a later call
to __DbgMarkEnd() to verify that the number of heap cells allocated,
at the current nested level, is as expected. This expected number of heap cells
is passed to __DbgMarkEnd() as a parameter; however, the most common expected
number is zero, reflecting the fact that most developers check that all memory
allocated since a previous call to __DbgMarkStart() has been freed.

@see RAllocator::__DbgMarkEnd()
*/
UEXPORT_C void RAllocator::__DbgMarkStart()
	{
	DebugFunction(EMarkStart);
	}




/**
Marks the end of heap cell checking at the current nested level for this heap.

A call to this function should match an earlier call to __DbgMarkStart().
If there are more calls to this function than calls to __DbgMarkStart(),
then this function raises a USER 51 panic.

The function checks that the number of heap cells allocated, at the current
nested level, is aCount. The most common value for aCount is zero, reflecting
the fact that most developers check that all memory allocated since a previous
call to __DbgMarkStart() has been freed.

If the check fails, the function returns a pointer to the first orphaned
heap cell.

@param aCount The number of allocated heap cells expected.

@return A pointer to the first orphaned heap cell, if verification fails;
        zero otherwise.

@see RAllocator::__DbgMarkStart()
*/
UEXPORT_C TUint32 RAllocator::__DbgMarkEnd(TInt aCount)
	{
	return DebugFunction(EMarkEnd, (TAny*)aCount);
	}




/**
Checks the current number of allocated heap cells for this heap.

If aCountAll is true, the function checks that the total number of allocated
cells on this heap is the same as aCount. If aCountAll is false, then
the function checks that the number of allocated cells at the current nested
level is the same as aCount.

If checking fails, the function raises a panic;
information about the failure is put into the panic category;
this takes the form:

ALLOC COUNT\\rExpected aaa\\rAllocated bbb\\rLn: ccc ddddd

where

1. aaaa is the value aCount

2. bbbb is the number of allocated heap cells

3. ccc is a line number, copied from aLineNum

4. ddddd is a file name, copied from the descriptor aFileName

Note that the panic number is 1.

@param aCountAll If true, the function checks that the total number of
                 allocated cells on this heap is the same as aCount.
                 If false, the function checks that the number of allocated
                 cells at the current nested level is the same as aCount.
@param aCount    The expected number of allocated cells.
@param aFileName A filename; this is displayed as part of the panic
                 category if the check fails.
@param aLineNum  A line number; this is displayed as part of the panic category 
                 if the check fails.

@return KErrNone, if successful; otherwise one of the other system wide error codes.
*/
UEXPORT_C TInt RAllocator::__DbgMarkCheck(TBool aCountAll, TInt aCount, const TDesC8& aFileName, TInt aLineNum)
	{
	SCheckInfo info;
	info.iAll = aCountAll;
	info.iCount = aCount;
	info.iFileName = &aFileName;
	info.iLineNum = aLineNum;
	return DebugFunction(ECheck, &info);
	}




/**
Simulates a heap allocation failure for this heap.

The failure occurs on subsequent calls to new or any of the functions which
allocate memory from this heap.

The timing of the allocation failure depends on the type of 
allocation failure requested, i.e. on the value of aType.

The simulation of heap allocation failure is cancelled
if aType is given the value RAllocator::ENone.

Notes:

1. If the failure type is RAllocator::EFailNext, the next attempt to allocate from
   this heap fails; however, no further failures will occur.

2. For failure types RAllocator::EFailNext and RAllocator::ENone, set aRate to 1.

@param aType  An enumeration which indicates how to simulate heap
              allocation failure.
@param aRate The rate of failure; when aType is RAllocator::EDeterministic,
              heap allocation fails every aRate attempts
*/
UEXPORT_C void RAllocator::__DbgSetAllocFail(TAllocFail aType, TInt aRate)
	{
	DebugFunction(ESetFail, (TAny*)aType, (TAny*)aRate);
	}


/**
Obtains the current heap failure simulation type.

After calling __DbgSetAllocFail(), this function may be called to retrieve the
value set.  This is useful primarily for test code that doesn't know if a heap
has been set to fail and needs to check.

@return RAllocator::ENone if heap is not in failure simulation mode;
		Otherwise one of the other RAllocator::TAllocFail enumerations
*/
UEXPORT_C RAllocator::TAllocFail RAllocator::__DbgGetAllocFail()
	{
	return((TAllocFail) DebugFunction(EGetFail));
	}


/**
Simulates a burst of heap allocation failures for this heap.

The failure occurs for aBurst allocations attempt via subsequent calls 
to new or any of the functions which allocate memory from this heap.

The timing of the allocation failure depends on the type of 
allocation failure requested, i.e. on the value of aType.

The simulation of heap allocation failure is cancelled
if aType is given the value RAllocator::ENone.

Notes:

1. If the failure type is RAllocator::EFailNext or RAllocator::EBurstFailNext, 
the next one or aBurst attempts to allocate from this heap will fail; 
however, no further failures will occur.

2. For failure types RAllocator::EFailNext and RAllocator::ENone, set aRate to 1.

@param aType  An enumeration which indicates how to simulate heap
              allocation failure.
@param aRate The rate of failure; when aType is RAllocator::EDeterministic,
              heap allocation fails every aRate attempts.
@param aBurst The number of consecutive heap allocations that will fail each
time the allocations should fail.

@see RAllocator::TAllocFail
*/
UEXPORT_C void RAllocator::__DbgSetBurstAllocFail(TAllocFail aType, TUint aRate, TUint aBurst)
	{
	SRAllocatorBurstFail burstFail;
	burstFail.iRate = aRate;
	burstFail.iBurst = aBurst;
	DebugFunction(ESetBurstFail, (TAny*)aType, (TAny*)&burstFail);
	}

/**
Returns the number of heap allocation failures the current debug allocator fail
function has caused so far.

This is intended to only be used with fail types RAllocator::EFailNext,
RAllocator::EBurstFailNext, RAllocator::EDeterministic and
RAllocator::EBurstDeterministic.  The return value is unreliable for 
all other fail types.

@return The number of heap allocation failures the current debug fail 
function has caused.

@see RAllocator::TAllocFail
*/
UEXPORT_C TUint RAllocator::__DbgCheckFailure()
	{
	return DebugFunction(ECheckFailure);
	}

/**
Gets the current size of the heap.

This is the total number of bytes committed by the host chunk, less the number
of bytes used by the heap's metadata (the internal structures used for keeping
track of allocated and free memory).

Size = (Rounded committed size - size of heap metadata).

@return The size of the heap, in bytes.
*/		
UEXPORT_C TInt RAllocator::Size() const	
	{
	return ((RAllocator*)this)->DebugFunction(EGetSize);
	}

/**
@return The maximum length to which the heap can grow.

@publishedAll
@released
*/
UEXPORT_C TInt RAllocator::MaxLength() const	
	{
	return ((RAllocator*)this)->DebugFunction(EGetMaxLength);
	}

/**
Gets a pointer to the start of the heap.

Note that this function exists mainly for compatibility reasons.  In a modern
heap implementation such as that present in Symbian it is not appropriate to
concern oneself with details such as the address of the start of the heap, as
it is not as meaningful as it was in older heap implementations.  In fact, the
"base" might not even be the base of the heap at all!

In other words, you can call this but it's not guaranteed to point to the start
of the heap (and in fact it never really was, even in legacy implementations).

@return A pointer to the base of the heap.  Maybe.
*/
UEXPORT_C TUint8* RAllocator::Base() const	
	{
	TUint8* base;
	((RAllocator*)this)->DebugFunction(EGetBase, &base);	
	return base;
	}

UEXPORT_C TInt RAllocator::Align(TInt a) const	
	{
	return ((RAllocator*)this)->DebugFunction(EAlignInteger, (TAny*)a);
	}

UEXPORT_C TAny* RAllocator::Align(TAny* a) const	
	{
	TAny* result;
	((RAllocator*)this)->DebugFunction(EAlignAddr, a, &result);
	return result;
	}
	
UEXPORT_C TInt RAllocator::Extension_(TUint, TAny*& a0, TAny*)
	{
	a0 = NULL;
	return KErrExtensionNotSupported;
	}
