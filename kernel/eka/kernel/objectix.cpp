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
// e32\kernel\objectix.cpp
// 
//

/**
 @file
 @internalTechnology
*/

#include <kernel/kern_priv.h>
#include "dobject.h"

#ifdef __VC32__
#pragma warning(disable: 4127) // disabling warning "conditional expression is constant"
#endif

#undef asserta
#undef assertd
#undef assertt

#define asserta(x) do { if (!(x)) { __crash(); } } while(0)

#ifdef DOBJECT_TEST_CODE
#define assertt(x)	asserta(x)
#else
#define assertt(x)
#endif

#ifdef _DEBUG
#define __DEBUG_TRANSFER(s,d)			((RHeap::SDebugCell*)d)[-1].nestingLevel = ((RHeap::SDebugCell*)s)[-1].nestingLevel
#define assertd(x)	asserta(x)
#else
#define __DEBUG_TRANSFER(s,d)
#define assertd(x)
#endif

volatile TUint32 RObjectIx::NextInstance = 0x00040000u;


void RObjectIx::Wait()
	{
	Kern::MutexWait(*HandleMutex);
	} // RObjectIx::Wait


void RObjectIx::Signal()
	{
	Kern::MutexSignal(*HandleMutex);
	} // RObjectIx::Signal


RObjectIx::RObjectIx()
  : iRWL(TSpinLock::EOrderGenericIrqLow3)
	{
	iAllocated=0;
	iCount=0;
	iActiveCount=0;
	iReservedFree=0;
	iReservedTotal=0;
	iReservedFreeHWM=0;
	iSlots=NULL;
	iAmortize=0;
	iState=ENormal;
	iModCount=255; // iModCount=255 means iModList not in use
	iModListShift=0;
	iSpare1=0;
	
	TInt i;
	for (i=-ENumFreeQ; i<0; ++i)
		{
		Empty(i);
		}
	} // RObjectIx::RObjectIx


/**	Destroys a kernel object index.

	Any objects in the index are closed.
	
	@param aPtr  Passed as the parameter to Close() for each object.
	@pre	Calling thread must be in a critical section.
	@pre	No fast mutex can be held.
	@pre	Call in a thread context.
 */
TInt RObjectIx::Close(TAny* aPtr)
	{
	Wait();
	AcquireWriteLock();
	SSlot* slots = iSlots;
	TInt count = iCount;
	iSlots = NULL;
	iCount = 0;	// stops any future handle lookups succeeding
	iState = (TUint8)ETerminated;	// stops any future add/remove/reserve etc.
	iAllocated = 0;
	iModCount = 255;
	ReleaseWriteLock();
	Signal();
	TInt i=-1;
	while(++i<count)
		{
		DObject* pO = Occupant(slots+i);
		if (pO)
			{
			pO->Close(aPtr);
			}
		}
	Kern::Free(slots);	// free memory
	
	return KErrNone;
	} // RObjectIx::Close


/* Look up a handle in the object index
@param aHandle looks like this:
	Bits 0-14	index
	Bit 15		no-close flag (ignored here)
	Bits 16-29	instance value
	Bit 30		thread local flag (ignored here)
	Bit 31		special handle flag (should be 0 here)
@param aUniqueID type of object required (enum value + 1), 0 for any
@param aAttr receives attributes of handle
@return pointer to the object if handle valid, 0 if not
@pre Interrupts enabled
*/
// Note: The aAttr was changed to a pointer to avoid the amibiguous overload of
//       the RObjectIx::At(TInt aHandle, TInt aUniqueID, TUint32 aRequiredAttr)
//       version.
DObject* RObjectIx::At(TInt aHandle, TInt aUniqueID, TUint32* aAttr)
	{
	TInt ix = HandleIndex(aHandle);
	TInt in = HandleInstance(aHandle);

	if (aAttr != NULL)
		{
		*aAttr = 0;
		}

	DObject* pO = 0;
	AcquireReadLock();
	if (ix<iCount)
		{
		SSlot* slot = iSlots + ix;
		TUint32 attr = slot->iUsed.iAttr;
		TUint32 objr = slot->iUsed.iObjR;
		if (objr>=EObjROccupied && ((attr&0x3FFF)==(TUint)in))
			{
			// slot occupied and instance value matches
			if (!aUniqueID || ((attr>>14)&0x3F) == (TUint)aUniqueID)
				{
				// object of correct type (if type specified)
				if (aAttr != NULL)
					{
					*aAttr = (objr<<31)|(attr>>20);
					}

#ifdef __HANDLES_USE_RW_SPIN_LOCK__
// If handle lookup protected only by RW spin lock, must take a reference on the object
				DObject* obj = (DObject*)(objr & EObjRObjMask);
				if (obj->Open()==KErrNone)
					{
					pO = obj;
					}
#else
				pO = (DObject*)(objr & EObjRObjMask);
#endif
				}
			}
		}
	ReleaseReadLock();

	return pO;
	} // RObjectIx::At


/* Look up a handle in the object index
@param aHandle looks like this:
	Bits 0-14	index
	Bit 15		no-close flag (ignored here)
	Bits 16-29	instance value
	Bit 30		thread local flag (ignored here)
	Bit 31		special handle flag (should be 0 here)
@param aUniqueID type of object required (enum value + 1), 0 for any
@param aRequiredAttr bitmask of attributes which handle should have
@return pointer to the object if handle valid, 0 if not
@pre Interrupts enabled
*/
DObject* RObjectIx::At(TInt aHandle, TInt aUniqueID, TUint32 aRequiredAttr)
	{
	TInt ix = HandleIndex(aHandle);
	TInt in = HandleInstance(aHandle);
	DObject* pO = 0;
	AcquireReadLock();
	if (ix<iCount)
		{
		SSlot* slot = iSlots + ix;
		TUint32 attr = slot->iUsed.iAttr;
		TUint32 objr = slot->iUsed.iObjR;
		if (objr>=EObjROccupied && ((attr&0x3FFF)==(TUint)in))
			{
			// slot occupied and instance value matches
			if (!aUniqueID || ((attr>>14)&0x3F) == (TUint)aUniqueID)
				{
				// object of correct type (if type specified)
				TUint32 xattr = (objr<<31)|(attr>>20);
				if (!(aRequiredAttr &~ xattr))
					{
					// all required attributes present
#ifdef __HANDLES_USE_RW_SPIN_LOCK__
// If handle lookup protected only by RW spin lock, must take a reference on the object
					DObject* obj = (DObject*)(objr & EObjRObjMask);
					if (obj->Open()==KErrNone)
						{
						pO = obj;
						}
#else
					pO = (DObject*)(objr & EObjRObjMask);
#endif
					}
				}
			}
		}
	ReleaseReadLock();

	return pO;
	} // RObjectIx::At


/* Look up a handle in the object index
@param aHandle looks like this:
	Bits 0-14	index
	Bit 15		no-close flag (ignored here)
	Bits 16-29	instance value
	Bit 30		thread local flag (ignored here)
	Bit 31		special handle flag (should be 0 here)
@param aUniqueID type of object required (enum value + 1), 0 for any
@return pointer to the object if handle valid, 0 if not
@pre Interrupts enabled
*/
DObject* RObjectIx::At(TInt aHandle, TInt aUniqueID)
	{
	return At(aHandle, aUniqueID, (TUint32)0);
	} // RObjectIx::At


/* Look up a handle in the object index
@param aHandle looks like this:
	Bits 0-14	index
	Bit 15		no-close flag (ignored here)
	Bits 16-29	instance value
	Bit 30		thread local flag (ignored here)
	Bit 31		special handle flag (should be 0 here)
@return pointer to the object if handle valid, 0 if not
@pre Interrupts enabled
*/
DObject* RObjectIx::At(TInt aHandle)
	{
	return At(aHandle, 0, (TUint32)0);
	} // RObjectIx::At


TUint32 RObjectIx::GetNextInstanceValue()
	{
	TUint32 x = NextInstance;
	TUint32 y;
	do	{
		y = x >> 1;
		y ^= (x>>11);
		y ^= (x>>12);
		y ^= (x>>13);
		y &= 0x00040000u;
		y |= (x<<1);
		} while(!__e32_atomic_cas_rlx32(&NextInstance, &x, y));
	return ((y>>18) * 10133u) & 0x3FFFu;
	} // RObjectIx::GetNextInstanceValue


void RObjectIx::Empty(TInt aQueue)
	{
	assertd(TUint(aQueue+ENumFreeQ)<TUint(ENumFreeQ));
	SSlotQLink* s = Link(aQueue);
	s->iNext = (TInt16)aQueue;
	s->iPrev = (TInt16)aQueue;
	} // RObjectIx::Empty


// requires write lock
RObjectIx::SSlot* RObjectIx::Dequeue(TInt aSlotIndex)
	{
	assertd(TUint(aSlotIndex)<TUint(iAllocated));
	SSlot* s = iSlots + aSlotIndex;

	Link(s->iFree.iPrev)->iNext = TInt16(s->iFree.iNext);
	Link(s->iFree.iNext)->iPrev = TInt16(s->iFree.iPrev);
	if (iModCount<255)
		{
		MarkModified(aSlotIndex);
		if (s->iFree.iPrev >= 0)
			{
			MarkModified(s->iFree.iPrev);
			}
		if (s->iFree.iNext >= 0)
			{
			MarkModified(s->iFree.iNext);
			}
		}

	return s;
	} // RObjectIx::Dequeue


// requires write lock
void RObjectIx::AddBefore(TInt aBase, TInt aSlotIndex)
	{
	assertd(TUint(aSlotIndex)<TUint(iAllocated));
	SSlotQLink* base = Link(aBase);
	SSlotQLink* slot = Link(aSlotIndex);
	TInt prevIndex = base->iPrev;
	SSlotQLink* prev = Link(prevIndex);

	slot->iNext = TInt16(aBase);
	slot->iPrev = TInt16(prevIndex);
	prev->iNext = TInt16(aSlotIndex);
	base->iPrev = TInt16(aSlotIndex);

	if (iModCount<255)
		{
		if (aBase>=0)
			{
			MarkModified(aBase);
			}
		if (aSlotIndex>=0)
			{
			MarkModified(aSlotIndex);
			}
		if (prevIndex>=0)
			{
			MarkModified(prevIndex);
			}
		}
	} // RObjectIx::AddBefore


// requires write lock
void RObjectIx::AddAfter(TInt aBase, TInt aSlotIndex)
	{
	assertd(TUint(aSlotIndex)<TUint(iAllocated));

	SSlotQLink* base = Link(aBase);
	SSlotQLink* slot = Link(aSlotIndex);
	TInt nextIndex = base->iNext;
	SSlotQLink* next = Link(nextIndex);

	slot->iNext = TInt16(nextIndex);
	slot->iPrev = TInt16(aBase);
	next->iPrev = TInt16(aSlotIndex);
	base->iNext = TInt16(aSlotIndex);

	if (iModCount<255)
		{
		if (aBase>=0)
			{
			MarkModified(aBase);
			}
		if (aSlotIndex>=0)
			{
			MarkModified(aSlotIndex);
			}
		if (nextIndex>=0)
			{
			MarkModified(nextIndex);
			}
		}
	} // RObjectIx::AddAfter


// requires write lock
inline void RObjectIx::AddHead(TInt aQueue, TInt aSlotIndex)
	{
	assertd(TUint(aSlotIndex)<TUint(iAllocated));
	assertd(TUint(aQueue+ENumFreeQ)<TUint(ENumFreeQ));

	AddAfter(aQueue, aSlotIndex);
	} // RObjectIx::AddHead


// requires write lock
inline void RObjectIx::AddTail(TInt aQueue, TInt aSlotIndex)
	{
	assertd(TUint(aSlotIndex)<TUint(iAllocated));
	assertd(TUint(aQueue+ENumFreeQ)<TUint(ENumFreeQ));

	AddBefore(aQueue, aSlotIndex);
	} // RObjectIx::AddTail


// Take all entries from aSrcQ and append them to aDestQ (i.e. add to end)
// Empty aSrcQ
// requires write lock
void RObjectIx::AppendList(TInt aSrcQ, TInt aDestQ)
	{
	assertd(TUint(aSrcQ+ENumFreeQ)<TUint(ENumFreeQ));
	assertd(TUint(aDestQ+ENumFreeQ)<TUint(ENumFreeQ));

	SSlotQLink* sq = Link(aSrcQ);
	SSlotQLink* dq = Link(aDestQ);

	if (sq->iNext >= 0)	// if source queue already empty, nothing to do
		{
		SSlotQLink* oldLast = Link(dq->iPrev);
		SSlotQLink* firstNew = Link(sq->iNext);
		SSlotQLink* newLast = Link(sq->iPrev);
		oldLast->iNext = (TInt16)sq->iNext;
		firstNew->iPrev = (TInt16)dq->iPrev;
		dq->iPrev = (TInt16)sq->iPrev;
		newLast->iNext = (TInt16)aDestQ;
		sq->iNext = (TInt16)aSrcQ;
		sq->iPrev = (TInt16)aSrcQ;
		}
	} // RObjectIx::AppendList


// Take all entries from aSrcQ and prepend them to aDestQ (i.e. add to front)
// Empty aSrcQ
// requires write lock
void RObjectIx::PrependList(TInt aSrcQ, TInt aDestQ)
	{
	assertd(TUint(aSrcQ+ENumFreeQ)<TUint(ENumFreeQ));
	assertd(TUint(aDestQ+ENumFreeQ)<TUint(ENumFreeQ));

	SSlotQLink* sq = Link(aSrcQ);
	SSlotQLink* dq = Link(aDestQ);

	if (sq->iNext >= 0)	// if source queue already empty, nothing to do
		{
		SSlotQLink* oldFirst = Link(dq->iNext);
		SSlotQLink* lastNew = Link(sq->iPrev);
		SSlotQLink* newFirst = Link(sq->iNext);

		oldFirst->iPrev = (TInt16)sq->iPrev;
		lastNew->iNext = (TInt16)dq->iNext;
		dq->iNext = (TInt16)sq->iNext;
		newFirst->iPrev = (TInt16)aDestQ;
		sq->iNext = (TInt16)aSrcQ;
		sq->iPrev = (TInt16)aSrcQ;
		}
	} // RObjectIx::PrependList


void RObjectIx::MarkModified(TInt aSlotIndex)
	{
	if (iModCount < EModCount)
		{
		iModList.iIndex[iModCount++] = (TInt16)aSlotIndex;
		return;
		}
	
	if (iModCount==EModCount)
		{
		// switch to bitmap
		SModList modList(iModList);
		memclr(&iModList, sizeof(iModList));
		TInt i;

		for (i=0; i<EModCount; ++i)
			{
			TInt ix = modList.iIndex[i];
			ix >>= iModListShift;
			iModList.iBitMap[ix>>5] |= (1u<<(ix&31));
			}
		iModCount = TUint8(EModCount+1);
		}

	aSlotIndex >>= iModListShift;
	iModList.iBitMap[aSlotIndex>>5] |= (1u<<(aSlotIndex & 31));
	} // RObjectIx::MarkModified


/**
 *  Add a handle to the index
 *  Use a free or reserved slot, don't grow the array
 *  
 *  @param	aObj	Object to make a handle to
 *  @param	aAttr	Attributes for handle
 *  				If RObjectIx::EReserved is set, use a reserved slot else use a free slot
 *  @param	aSlot	If non-null, receives data to be written to array
 *  @return	The handle value (always positive) if operation successful
 *  		KErrNoMemory if no free or reserved slot was available
 *  @pre Interrupts enabled
 */
TInt RObjectIx::DoAdd(DObject* aObj, TUint32 aAttr, SSlot* aSlot)
	{
	TBool rsvd = aAttr & EReserved;
	TInt fqn = rsvd ? EQRsvd : EQFree;				// free queue number
	SSlot* slot = aSlot;
	SSlotQLink* anchor = Link(fqn);
	TUint32 in = GetNextInstanceValue();
	TUint32 ot = aObj->UniqueID() & 0x3F;
	TInt r = KErrNoMemory;

	AcquireWriteLock();
	if (iState==ETerminated)
		{
		ReleaseWriteLock();
		return KErrDied;
		}

	TInt firstfree = anchor->iNext;
	if (firstfree < 0)
		{
		++anchor, ++fqn, firstfree = anchor->iNext;	// if main queue empty try alternate queue
		}

	if (firstfree >= 0)
		{
		// a slot is available
		// dequeue it from the free/reserved list
		slot = Dequeue(firstfree);

		// update the HWM if necessary
		if (firstfree >= iCount)
			{
			iCount = firstfree + 1;
			}
		if ((iState==ETidying || iState==EFindingLast) && firstfree>=iModList.iMonitor.iBoundary)
			{
			iModList.iMonitor.iBoundary = firstfree;
			}
		else if (iState==ECounting && aObj==iModList.iMonitor.iObj && firstfree<iModList.iMonitor.iBoundary)
			{
			++iModList.iMonitor.iResult;
			}
		else if (iState==ESearching && aObj==iModList.iMonitor.iObj && firstfree<iModList.iMonitor.iBoundary)
			{
			iModList.iMonitor.iBoundary = firstfree;
			}

		// update the active count
		++iActiveCount;

		// update the free reserved count if necessary
		if (rsvd)
			{
			--iReservedFree;
			}

		// synthesise the handle
		r = MakeHandle(firstfree, in);
		}

	if (slot)
		{
		// populate the slot or the temporary storage
		slot->iUsed.iAttr = in | (ot<<14) | (aAttr<<20);
		slot->iUsed.iObjR = TUint32(aObj) | (aAttr>>31);
		}
	ReleaseWriteLock();

	return r;
	} // RObjectIx::DoAdd


/**
 *  Remove a handle from the index
 *  Mark the slot free or reserved but don't shrink the array
 *  
 *  @param	aHandle	The handle to remove
 *  @param	aObj	Receives a pointer to the DObject referred to by the handle
 *  @param	aAttr	Receives the attributes of the removed handle
 *  				If RObjectIx::EReserved is set, it was a reserved handle
 *  @return	0 if the handle was removed successfully
 *  		1 if the handle was removed successfully and the amortize count changed from 1 to 0
 *  		KErrBadHandle if the handle did not exist
 *  @pre Interrupts enabled
 */
TInt RObjectIx::DoRemove(TInt aHandle, DObject*& aObj, TUint32& aAttr)
	{
	TInt ix = HandleIndex(aHandle);
	TInt in = HandleInstance(aHandle);
	SSlot* slot = 0;
	TUint32 attr = 0;
	TUint32 objr = 0;

	AcquireWriteLock();
	if (iState==ETerminated)
		{
		ReleaseWriteLock();
		return KErrDied;
		}

	if (ix < iCount)
		{
		slot = iSlots + ix;
		attr = slot->iUsed.iAttr;
		objr = slot->iUsed.iObjR;
		}

	if (objr<EObjROccupied || ((attr&0x3FFF)!=(TUint)in))	// slot empty or instance value doesn't match
		{
		ReleaseWriteLock();
		aObj = 0;
		aAttr = 0;
		return KErrBadHandle;
		}

	aObj = (DObject*)(objr & EObjRObjMask);
	TUint32 xattr = (objr<<31)|(attr>>20);
	aAttr = xattr;

	slot->iFree.iRsvd &= EObjRRsvd;		// mark slot as free

	--iActiveCount; // one less valid handle

	// help someone searching the whole index if necessary
	if (iState==ECounting && aObj==iModList.iMonitor.iObj && ix<iModList.iMonitor.iBoundary)
		{
		--iModList.iMonitor.iResult;
		}

	// update the free reserved count if necessary
	if (xattr & EReserved)
		{
		++iReservedFree;
		// if TidyAndCompact() is running concurrently, we should only update
		// iReservedFreeHWM if the freed slot has already been scanned
		if (iState != ETidying || ix < iModList.iMonitor.iResult)
			{
			if (ix >= iReservedFreeHWM)
				{
				iReservedFreeHWM = ix + 1;
				assertd(iReservedFreeHWM <= iCount);
				}
			}
		// put the slot at the front of the corresponding free queue
		AddHead(EQRsvd, ix);
		ReleaseWriteLock();
		return KErrNone;
		}

	// put the slot at the front of the corresponding free queue
	AddHead(EQFree, ix);

	// Reduce amortize count if necessary.
	// Signal compaction if amortize count reaches zero or index is entirely
	// empty (i.e. no occupied or reserved entries), but not if we just freed
	// a reserved entry.
	TInt r = 0;
	if (iAmortize>0)
		{
		if (iAmortize>1)	// don't want to tidy/compact on freeing a reserved handle
			{
			--iAmortize;
			}

		if (iAmortize==0 || (iActiveCount==0 && iReservedTotal==0))
			{
			r = 1;
			}
		}
	ReleaseWriteLock();

	return r;
	} // RObjectIx::DoRemove


/**
 * 	Adds a kernel object to an index and return a handle.
 * 	
 * 	@param	aObj	Pointer to the object to add
 * 	@param	aAttr	Attributes for handle
 * 					If RObjectIx::EReserved is set, a reserved slot will be used
 * 	
 * 	@return	Positive handle value if operation successful;
 * 			KErrNoMemory, if there was insufficient memory to expand the array
 * 				or if there was no reserved slot available.
 *			KErrDied if the index has been closed
 * 	
 * 	@pre	Calling thread must be in a critical section.
 * 	@pre	No fast mutex can be held.
 * 	@pre	Call in a thread context.
 */
TInt RObjectIx::Add(DObject* aObj, TUint32 aAttr)
	{
	TBool rsvd = aAttr & EReserved;
	TInt r = DoAdd(aObj, aAttr, 0);
	if (r>=0 || r==KErrDied || rsvd)
		return r;

	Wait();						// acquire heavyweight mutex
	SSlot slotData;
	r = DoAdd(aObj, aAttr, &slotData);	// try to add again
	if (r==KErrNoMemory)
		{
		r = Grow(0, &slotData);	// grow array and insert entry
		}
	Signal();
	return r;
	} // RObjectIx::Add


/**
 *  Increase the number of reserved slots
 *
 *  @param	aCount	Number of additional reserved slots to allocate
 *  @return	KErrNone if operation successful
 *  		KErrNoMemory if additional slots could not be allocated
 *  @pre	HandleMutex held
 */
TInt RObjectIx::ReserveSlots(TInt aCount)
	{
	__ASSERT_MUTEX(HandleMutex);

	TInt r = KErrNone;
	TInt remain = aCount;
	while (remain > 0)
		{
		TInt got = 0;
		AcquireWriteLock();
		TInt fqn = EQFree;				// free queue number
		SSlot* slot = 0;
		SSlotQLink* anchor = Link(fqn);
		TInt firstfree = anchor->iNext;
		if (firstfree < 0)
			{
			++anchor, ++fqn, firstfree = anchor->iNext;	// if main queue empty try alternate queue
			}
		
		if (firstfree >= 0)
			{
			// a slot is available
			// move it from the free to the reserved list
			slot = Dequeue(firstfree);
			slot->iFree.iRsvd = EObjRRsvd;
			AddHead(EQRsvd, firstfree);

			// update HWMs/counts if necessary
			// can't happen concurrently with TidyAndCompact()
			if (firstfree >= iCount)
				{
				iCount = firstfree + 1;
				}
			if (firstfree >= iReservedFreeHWM)
				{
				iReservedFreeHWM = firstfree + 1;
				assertd (iReservedFreeHWM <= iCount);
				}
			++iReservedFree;
			++iReservedTotal;
			got = 1;
			}
		ReleaseWriteLock();
		if (!got)
			{
			break;	// no more free slots to pinch
			}
		remain -= got;
		}
	if (remain == 0)
		{
		return r;	// got them all from the free list
		}

	// else try to expand the array to get the others
	r = Grow(remain, 0);
	if (r != KErrNone)
		{
		// out of memory - unreserve any slots we reserved beforehand
		if (aCount > remain)
			{
			UnReserveSlots(aCount-remain, EFalse);
			}
		}
	return r;
	} // RObjectIx::ReserveSlots


/**
 *  Unreserve a number of reserved slots
 *  
 *  @param	aCount	Number of reserved slots to unreserve
 *  @return	1 if the array should be compacted afterwards
 *  		0 if not
 *  @pre	HandleMutex held
 */
TInt RObjectIx::UnReserveSlots(TInt aCount, TBool aAmortize)
	{
	__ASSERT_MUTEX(HandleMutex);

	TInt r = 0;
	TInt remain = aCount;
	while (remain>0)
		{
		AcquireWriteLock();

		TInt pos = iReservedFreeHWM;

		// pos=0 if reserved slots have gone missing
		// pos<0 or pos>iCount -> corrupt
		asserta(pos>0 && pos<=iCount);

		TInt limit = pos<EMaxLockedIter ? 0 : pos-EMaxLockedIter;	// last pos to test before releasing spinlock
		--pos;

		SSlot* slot = iSlots + pos;
		for (; pos>=limit && !IsFreeReserved(slot); --pos, --slot)
			{
			}

		if (pos>=limit)
			{
			// slot is reserved and unoccupied so unreserve it
			// can't happen concurrently with TidyAndCompact()
			slot->iFree.iRsvd = 0;
			--iReservedFree;
			--iReservedTotal;
			Dequeue(pos);
			AddHead(EQAltFree, pos);	// put on front of alternate free queue
			iReservedFreeHWM = pos;
			assertd(iReservedFreeHWM <= iCount);
			--remain;
			}
		else
			{
			iReservedFreeHWM = pos + 1;	// haven't examined pos yet
			assertd(iReservedFreeHWM <= iCount);
			}
		ReleaseWriteLock();
		}
	AcquireWriteLock();
	if (aAmortize && iAmortize>0)
		{
		if (iAmortize <= aCount)
			{
			iAmortize = 0, r = 1;
			}
		else
			{
			iAmortize -= aCount;
			}
		}

	// move alternate free queue to front of normal free queue
	PrependList(EQAltFree, EQFree);
	ReleaseWriteLock();

	return r;
	} // RObjectIx::UnReserveSlots


/**
 *  Change the number of reserved slots
 *  
 *  @param	aCount	If positive, number of additional reserved slots to allocate
 *  				If negative, -number of reserved slots to unreserve
 *  @return	KErrNone if operation successful
 *  		KErrNoMemory if additional slots could not be allocated
 */
TInt RObjectIx::Reserve(TInt aCount)
	{
	if (aCount==0)
		{
		return KErrNone;
		}

	Wait();
	TInt r = KErrNone;

	if (iState==ETerminated)
		{
		r = KErrDied;
		}
	else if (aCount>0)
		{
		r = ReserveSlots(aCount);
		}
	else if (UnReserveSlots(-aCount, ETrue))
		{
		TidyAndCompact();
		}

	Signal();

	return r;
	} // RObjectIx::Reserve


TInt RObjectIx::Remove(TInt aHandle, DObject*& aObj, TUint32& aAttr)
	{
	TInt r = DoRemove(aHandle, aObj, aAttr);
	if (r<=0)
		{
		return r;
		}

	Wait();
	if (iState==ETerminated)
		{
		r = KErrDied;
		}
	else
		{
		TidyAndCompact();
		r = KErrNone;
		}
	Signal();

	return r;
	} // RObjectIx::Remove


/**
 *  Grow the array either to add a new handle or to add some more
 *  reserved entries.
 *  
 *  @param	aReserve	Number of reserved slots to add
 *  @param	aSlotData	Pointer to data to be placed in first new slot
 *  @return	KErrNone if aSlotData==NULL and operation successful
 *  		A positive handle value if aSlotData!=NULL and operation successful
 *  		KErrNoMemory if insufficient memory available to expand the array or
 *  				if the array is already at the maximum possible size.
 *  @pre	HandleMutex held
 */
TInt RObjectIx::Grow(TInt aReserve, SSlot* aSlotData)
	{
	__ASSERT_MUTEX(HandleMutex);

	if (iAllocated == EMaxSlots)
		{
		return KErrNoMemory;
		}

	TInt normal = aSlotData ? 1 : 0;
	TInt newAlloc = iAllocated + aReserve + normal;
	if (newAlloc > EMaxSlots)
		{
		return KErrNoMemory;
		}
	if (newAlloc < EMinSlots)
		{
		newAlloc = EMinSlots;
		}
	--newAlloc;
	newAlloc |= (newAlloc>>1);
	newAlloc |= (newAlloc>>2);
	newAlloc |= (newAlloc>>4);
	newAlloc |= (newAlloc>>8);
	++newAlloc;	// round up newAlloc to next power of 2
	SSlot* newSlots = (SSlot*)Kern::Alloc(newAlloc * sizeof(SSlot));	// zero initialized memory
	if (!newSlots)
		{
		return KErrNoMemory;
		}
	TInt r = KErrNone;

	Empty(EQTempFree);	// new normal free slots will go here
	Empty(EQTempRsvd);	// new reserved slots will go here

	// initialize all the new slots
	SSlotQLink* tf = Link(EQTempFree);
	SSlotQLink* tr = Link(EQTempRsvd);
	TInt first = iAllocated;
	TInt last = first + aReserve - 1;
	TInt ix = first;
	if (aReserve>0)
		{
		for (; ix<=last; ++ix)
			{
			SSlot* slot = newSlots + ix;
			slot->iFree.iRsvd = EObjRRsvd;
			slot->iFree.iPrev = (TInt16)(ix-1);
			slot->iFree.iNext = (TInt16)(ix+1);
			}
		newSlots[first].iFree.iPrev = (TInt16)EQTempRsvd;
		newSlots[last].iFree.iNext = (TInt16)EQTempRsvd;
		tr->iNext = (TInt16)first;
		tr->iPrev = (TInt16)last;
		}
	if (normal)
		{
		SSlot* slot = newSlots + ix;
		*slot = *aSlotData;
		r = MakeHandle(ix, slot->iUsed.iAttr);
		++ix;
		}
	first = ix;
	last = newAlloc - 1;
	if (first<=last)
		{
		for (; ix<=last; ++ix)
			{
			SSlot* slot = newSlots + ix;
			slot->iFree.iRsvd = 0;
			slot->iFree.iPrev = (TInt16)(ix-1);
			slot->iFree.iNext = (TInt16)(ix+1);
			}
		newSlots[first].iFree.iPrev = (TInt16)EQTempFree;
		newSlots[last].iFree.iNext = (TInt16)EQTempFree;
		tf->iNext = (TInt16)first;
		tf->iPrev = (TInt16)last;
		}

	// copy the existing slot contents
	AcquireWriteLock();
	if (iAllocated==0)
		{
		goto first_alloc;	// no existing slots, so skip this section
		}
	__DEBUG_TRANSFER(iSlots, newSlots);		// so kernel heap checking doesn't barf on a realloc'd cell
	memclr(&iModList, sizeof(iModList));
	iState = (TUint8)ENormal;
	iModCount = 0;	// any concurrent modifications will now be recorded
	ReleaseWriteLock();
	wordmove(newSlots, iSlots, iAllocated*sizeof(SSlot));
	FOREVER
		{
		AcquireWriteLock();
		if (iModCount==0)
			{
			break;	// there were no concurrent modifications, so finish up
			}
		// get list of modifications and reset it
		TInt modCount = iModCount;
		SModList modList = iModList;
		memclr(&iModList, sizeof(iModList));
		iModCount = 0;	// any concurrent modifications will now be recorded
		ReleaseWriteLock();
		if (modCount <= EModCount)
			{
			// list of particular modified entries
			TInt i;
			for (i=0; i<modCount; ++i)
				{
				TInt ix = modList.iIndex[i];
				newSlots[ix] = iSlots[ix];
				}
			continue;
			}
		// bitmap of groups of modified entries
		TInt i;
		TInt nbits = 0;
		for (i=0; i<EBitMapSize/32; ++i)
			{
			nbits += __e32_bit_count_32(modList.iBitMap[i]);
			}
		while (nbits--)
			{
			for (i=0; modList.iBitMap[i]==0; ++i)
				{
				}
			TInt ix = __e32_find_ls1_32(modList.iBitMap[i]);
			modList.iBitMap[i] &= ~(1u<<ix);
			ix += (i<<5);
			// copy affected block of slots
			wordmove(newSlots + (ix<<iModListShift), iSlots + (ix<<iModListShift), sizeof(SSlot)<<iModListShift);
			}
		}
	// exit loop with write lock held, no outstanding modifications to copy
first_alloc:
	SSlot* oldSlots = iSlots;			// remember old array so it can be freed
	iSlots = newSlots;					// swing pointer to new larger array
	iCount = iAllocated + aReserve + normal;	// HWM = one above new reserved and/or occupied slots
	iActiveCount += normal;				// increment if we added a new occupied slot
	iReservedFree += aReserve;			// add new reserved slots
	iReservedTotal += aReserve;
	if (aReserve>0)
		{
		// can't happen concurrently with TidyAndCompact()
		iReservedFreeHWM = iAllocated + aReserve;
		assertd(iReservedFreeHWM <= iCount);
		}
	iAllocated = newAlloc;				// new allocated array size
	if (newAlloc > EBitMapSize)			// recalculate modification bitmap granularity
		{
		iModListShift = (TUint8)__e32_find_ms1_32(newAlloc / EBitMapSize);
		}
	else
		{
		iModListShift = 0;
		}
	iState = (TUint8)ENormal;
	iModCount = 255;					// stop tracking modifications
	AppendList(EQTempRsvd, EQRsvd);		// append new reserved slots to reserved list
	AppendList(EQTempFree, EQFree);		// append new free slots to free list
	if (iAmortize == 0)
		{
		iAmortize = iAllocated / 2;		// number of Remove() before we tidy/compact
		}
	ReleaseWriteLock();
	Kern::Free(oldSlots);				// free old array

	return r;			// return new handle or KErrNone
	} // RObjectIx::Grow


/**
 *  Tidy up the index and shrink the array if possible
 *  
 *  Reorder the free list and reserved list so that the reserved list consists of
 *  the lowest index free slots and the free list is in ascending order of slot
 *  index (so the lowest index slots are used first). Move the HWM down as much
 *  as possible and, if it would reduce the size of the array by at least half,
 *  shrink the array.
 *  
 *  @pre	HandleMutex held
 */
void RObjectIx::TidyAndCompact()
	{
	__ASSERT_MUTEX(HandleMutex);

	if (!iAllocated || iState==ETerminated)
		{
		return;	// nothing to tidy or compact, or index dead
		}

	SSlotQLink* fq = Link(EQFree);
	SSlotQLink* afq = Link(EQAltFree);
	SSlotQLink* arq = Link(EQAltRsvd);

	// This function is the only one which adds items to the AltFree or AltRsvd
	// queues, so they should be empty at this point.
	asserta(afq->iNext == EQAltFree);
	asserta(afq->iPrev == EQAltFree);
	asserta(arq->iNext == EQAltRsvd);
	asserta(arq->iPrev == EQAltRsvd);

	AcquireWriteLock();
	if (iActiveCount==0 && iReservedTotal==0)
		{
		// no occupied or reserved slots, so just reset the index to the initial state
		SSlot* oldSlots = iSlots;
		iAllocated = 0;
		iCount = 0;
		iReservedFree = 0;
		iReservedFreeHWM = 0;
		iSlots = 0;
		iAmortize = 0;
		iState = (TUint8)ENormal;
		iModCount = 255;
		iModListShift = 0;
		iSpare1 = 0;
		memclr(&iModList, sizeof(iModList));
		TInt i;
		for (i=-ENumFreeQ; i<0; ++i)
			{
			Empty(i); // Empty lists
			}
		ReleaseWriteLock();
		Kern::Free(oldSlots);
		return;
		}
	iAmortize = -1;	// don't need to check for compaction
	iModList.iMonitor.iBoundary = -1;	// will change if any slots become occupied
	iModList.iMonitor.iResult = 0;		// tracks next position to be scanned (to update iReservedFreeHWM)
	iModCount = 255;
	iState = (TUint8)ETidying;

	iReservedFreeHWM = 0;	// will be updated if any more reserved slots are freed while we are tidying
							// only relied on by UnReserveSlots(), which can't happen since we have the HandleMutex locked
	ReleaseWriteLock();
	TInt pos = 0;
	TInt hwm = -1;			// tracks last occupied and/or reserved slot
	TInt rfhwm = 0;			// tracks last free reserved slot
	TInt rsvd_remain = iReservedTotal;	// this can't change since we have the HandleMutex locked
	FOREVER
		{
		TInt limit = pos + EMaxLockedIter;
		AcquireWriteLock();
		if (limit > iCount)
			{
			limit = iCount;
			}

		SSlot* slot = iSlots + pos;
		// skip occupied slots
		for (; pos<limit && Occupant(slot); ++pos, ++slot)
			{
			assertd(hwm < pos);
			hwm = pos;					// hwm tracks last occupied and/or reserved slot
			if (IsReserved(slot))
				{
				--rsvd_remain;			// count off reserved slots as we see them
				}
			}
		if (pos==iCount)
			{
			break;						// reached end of occupied and/or reserved entries
			}

		if (pos<limit)
			{
			// found unoccupied slot
			if (IsReserved(slot))
				{
				// Reserved, unoccupied slot.
				// First take it off the free reserved queue.
				Dequeue(pos);

				// Now see if there is anything on the AltFree queue.
				// If there is, it must be a free slot we have previously encountered and moved.
				// We interchange this slot with the first one on the AltFree queue
				// so that the reserved slots get moved as close as possible to the beginning
				// of the index - this makes it more likely that we can compact the index,
				// since we can't free the space used by reserved slots.
				TInt rsvpos = pos;
				TInt substitute = afq->iNext;
				if (substitute>=0)
					{
					assertd(substitute < pos);
					rsvpos = substitute;
					Dequeue(substitute);			// take substitute slot off AltFree queue
					SSlot* substSlot = iSlots + substitute;
					substSlot->iFree.iRsvd = EObjRRsvd;	// mark slot as reserved
					AddTail(EQAltRsvd, substitute);	// move substitute slot to alternate reserved queue
					slot->iFree.iRsvd = 0;			// mark original reserved slot as normal free
					AddTail(EQAltFree, pos);		// move this slot to the the alternate free queue
					}
				else
					{
					// nothing to substitute with so leave this slot as reserved
					AddTail(EQAltRsvd, pos);		// move it to the alternate reserved queue
					}
				if (rsvpos >= rfhwm)
					{
					rfhwm = rsvpos + 1;				// remember last free reserved slot
					assertd(rfhwm <= iCount);
					}
				if (rsvpos > hwm)
					{
					hwm = rsvpos;					// hwm tracks last occupied and/or reserved slot
					}

				--rsvd_remain;						// count off reserved slots as we see them
				++pos;
				}
			else
				{
				// unoccupied, non-reserved slot
				Dequeue(pos);
				AddTail(EQAltFree, pos);			// move it to the alternate free queue
				++pos;
				}
			}
		iModList.iMonitor.iResult = pos;	// tracks next position to be scanned (to update iReservedFreeHWM)
		ReleaseWriteLock();
		}

	//
	// At this point we have been through all the occupied and/or reserved entries
	// The AltRsvd queue should contain an ordered list of free reserved slots
	// The AltFree queue should contain an ordered list of free normal slots
	// If there has been no concurrent activity, iModList.iMonitor.iBoundary == -1,
	// the Rsvd queue is empty and the Free queue holds only the entries between
	// iCount and iAllocated which have never been used. Otherwise,
	// iModList.iMonitor.iBoundary holds the highest index allocated during the scan
	// and the Free and/or Rsvd queues hold any slots freed after they were scanned.
	// In either case we can set iCount to Max(hwm, iModList.iMonitor.iBoundary)+1.
	//
	asserta(rsvd_remain == 0);		// should have seen all reserved slots

	TInt oldCount = iCount;
	iCount = Max(hwm, iModList.iMonitor.iBoundary) + 1;
	asserta(iCount <= oldCount);

	// iReservedFreeHWM is set to the last free reserved slot we saw during the
	// scan or to the highest index one freed after we had scanned it
	if (rfhwm > iReservedFreeHWM)
		iReservedFreeHWM = rfhwm;
	asserta(iReservedFreeHWM <= iCount);

	//
	// Move all unoccupied entries back to their usual queues...
	//
	PrependList(EQAltRsvd, EQRsvd);

	// ok we want the random/newly freed entries at the head of the free queue,
	// followed by the sorted used entries
	// followed by the never used entries

	TInt split = Max(oldCount, (iModList.iMonitor.iBoundary + 1));

	if (split < iAllocated)
		{
		SSlot* firstUnused = iSlots + split;
		if (firstUnused->iFree.iPrev != (TInt16)EQFree)
			{
			// split, random entries will be at the head and unused entries at the end
			SSlotQLink* lastUsed = Link(firstUnused->iFree.iPrev);
			TInt lastUnused = fq->iPrev;

			fq->iPrev = firstUnused->iFree.iPrev;
			lastUsed->iNext = (TInt16)EQFree;

			// now added sorted used entries
			AppendList(EQAltFree, EQFree);

			// now added never used entries
			SSlotQLink* last = Link(fq->iPrev);

			last->iNext = (TInt16)split;
			firstUnused->iFree.iPrev = fq->iPrev;
			// note last unused should point to the QFree
			fq->iPrev = (TInt16)lastUnused;
			}
		else
			{
			// No entries freed before tidy, so put sorted 'used' entries at the head
			PrependList(EQAltFree, EQFree);
			}
		}
	else
		{
		// Free list only contains random entries
		// now added sorted used entries
		AppendList(EQAltFree, EQFree);
		}

	// Work out if we can shrink the array
	TInt newAlloc = Max(iCount, EMinSlots) - 1;
	newAlloc |= (newAlloc>>1);
	newAlloc |= (newAlloc>>2);
	newAlloc |= (newAlloc>>4);
	newAlloc |= (newAlloc>>8);
	++newAlloc;	// round up newAlloc to next power of 2

	if (newAlloc < iAllocated)
		{
		// We are going to shrink the array.
		// Cut out any slots which will disappear
		SSlot* last = iSlots + newAlloc;		// first slot to go

		if (last->iFree.iPrev != (TInt16)EQFree)
			{
			// there are other free slots before the last block, so free queue
			// will not be empty after shrink
			fq->iPrev = last->iFree.iPrev;
			last = iSlots + last->iFree.iPrev;	// last free slot after shrink
			last->iFree.iNext = (TInt16)EQFree;
			}
		else
			{
			// free queue is empty after shrinking
			fq->iNext = (TInt16)EQFree;
			fq->iPrev = (TInt16)EQFree;
			}

		iAllocated = newAlloc;
		if (newAlloc > EBitMapSize)			// recalculate modification bitmap granularity
			{
			iModListShift = (TUint8)__e32_find_ms1_32(newAlloc / EBitMapSize);
			}
		else
			{
			iModListShift = 0;
			}
		}
	else
		{
		newAlloc = -1;
		}

	// set the amortize value before next tidy/compaction
	iAmortize = iAllocated / 2;
	iState = (TUint8)ENormal;
	ReleaseWriteLock();

	// Now actually shrink the array. This relies on the current heap behaviour
	// when shrinking - the memory must not be moved.
	if (newAlloc>0)
		{
		Kern::ReAlloc(iSlots, iAllocated*sizeof(SSlot), RAllocator::ENeverMove);
		}

	} // RObjectIx::TidyAndCompact


/**
 *  Counts the number of times an object appears in this index.
 *  
 *  @param	aObject	Object whose occurrences are to be counted.
 *  
 *  @return	Number of times aObject appears in the index.

 *  @pre	Calling thread must be in a critical section.
 *  @pre	No fast mutex can be held.
 *  @pre	Call in a thread context.
 *  @pre	DObject::HandleMutex held
 */
TInt RObjectIx::Count(DObject* aObject)
	{
	//Check preconditions(debug build only)
	__ASSERT_CRITICAL;
	__ASSERT_NO_FAST_MUTEX;
	__ASSERT_MUTEX(HandleMutex);

	if (iState==ETerminated)
		{
		return 0;
		}

	AcquireWriteLock();
	iModList.iMonitor.iBoundary = 0;	// count changes before this point
	iModList.iMonitor.iObj = aObject;	// count additions/removals of this object
	iModList.iMonitor.iResult = 0;		// initial count
	iState = (TUint8)ECounting;			// enable monitoring of additions/removals
	TInt pos = 0;
	while (pos<iCount && iActiveCount)	// stop if index empty
		{
		TInt limit = pos + EMaxLockedIter;
		if (limit>iCount)
			{
			limit = iCount;
			}
		
		while (pos<limit)
			{
			SSlot* slot = iSlots + pos;
			if (Occupant(slot) == aObject)
				{
				++iModList.iMonitor.iResult;
				}
			pos++;
			}
		iModList.iMonitor.iBoundary = pos;	// count additions/removals of aObject before this point
		ReleaseWriteLock();	// let other threads in
		AcquireWriteLock();
		}
	TInt result = iActiveCount ? iModList.iMonitor.iResult : 0;
	iState = (TUint8)ENormal;
	ReleaseWriteLock();

	return result;
	} // RObjectIx::Count


/**
 * 	Looks up an object in the index by object pointer.
 * 	
 * 	Returns a handle to the object.
 * 	
 * 	@param	aObj	Pointer to the object to look up.
 * 		
 * 	@return	Handle to object (always >0);
 * 			KErrNotFound, if object not present in index.
 * 	
 * 	@pre	Calling thread must be in a critical section.
 * 	@pre	No fast mutex can be held.
 * 	@pre	Call in a thread context.
 * 	@pre	DObject::HandleMutex held.
 */
TInt RObjectIx::At(DObject* aObj)
	{
	//Check preconditions(debug build only)
	__ASSERT_CRITICAL;
	__ASSERT_NO_FAST_MUTEX;
	__ASSERT_MUTEX(HandleMutex);

	if (iState==ETerminated)
		{
		return KErrNotFound;
		}

	TInt h = KErrNotFound;
	AcquireWriteLock();
	iState = (TUint8)ESearching;		// enable monitoring of new handles
	iModList.iMonitor.iObj = aObj;		// object to check for
	iModList.iMonitor.iBoundary = 0;	// will change if aObj is added to a slot before this point
	TInt pos = 0;
	while (pos<iCount && iActiveCount)	// stop if index empty
		{
		TInt limit = pos + EMaxLockedIter;
		if (limit>iCount)
			{
			limit = iCount;
			}
		while (pos<limit)
			{
			SSlot* slot = iSlots + pos;
			if (Occupant(slot) == aObj)
				{
				// found it, finish
				h = MakeHandle(pos, slot->iUsed.iAttr);
				break;
				}
			pos++;
			}
		if (h>0)
			{
			break;	// found it, finish
			}
		iModList.iMonitor.iBoundary = pos;	// will change if aObj is added to a slot already checked
		ReleaseWriteLock();	// let other threads in
		AcquireWriteLock();
		pos = iModList.iMonitor.iBoundary;	// next position to check
		}
	iState = (TUint8)ENormal;
	ReleaseWriteLock();
	return h;
	} // RObjectIx::At


/**
 *  Finds the object at a specific position in the index array.
 *  
 *  @param	aIndex	Index into array.
 *  	
 *  @return	Pointer to the object at that position (could be NULL).
 *  
 *  @pre	Call in a thread context. 
 *  @pre	System lock must be held.
 */
DObject* RObjectIx::operator[](TInt aIndex)
	{
	DObject* obj = 0;
	AcquireReadLock();
	asserta(TUint(aIndex)<TUint(iCount));
	SSlot* slot = iSlots + aIndex;
	obj = Occupant(slot);
	ReleaseReadLock();
	return obj;
	} // RObjectIx::operator[]


TInt RObjectIx::LastHandle()
//
// Return the last valid handle
// Must wait on HandleMutex before calling this.
//
	{
	//Check preconditions(debug build only)
	__ASSERT_CRITICAL;
	__ASSERT_NO_FAST_MUTEX;
	__ASSERT_MUTEX(HandleMutex);

	if (iState==ETerminated)
		{
		return 0;
		}

	TInt h = 0;
	AcquireWriteLock();
	iState = (TUint8)EFindingLast;	// enable monitoring of new handles
	iModList.iMonitor.iBoundary = iCount-1;	// will change if any slots past this point become occupied
	TInt pos = iCount;
	while (pos>=0 && iActiveCount)	// stop if index empty
		{
		TInt limit = pos - EMaxLockedIter;

		if (limit<0)
			{
			limit = 0;
			}

		while (--pos>=limit)
			{
			SSlot* slot = iSlots + pos;
			if (Occupant(slot))
				{
				// found it, finish
				h = MakeHandle(pos, slot->iUsed.iAttr);
				break;
				}
			}
		if (h)
			{
			break;	// found it, finish
			}
		iModList.iMonitor.iBoundary = pos - 1;	// will change if any slots already checked or after become occupied
		ReleaseWriteLock();	// let other threads in
		AcquireWriteLock();
		pos = iModList.iMonitor.iBoundary + 1;	// one after last used position or last checked position
		}
	iState = (TUint8)ENormal;
	ReleaseWriteLock();

	return h;
	} // RObjectIx::LastHandle


#ifdef DOBJECT_TEST_CODE
void RObjectIx::Check(RObjectIx::TValidateEntry aV)
	{
	Wait();
	AcquireWriteLock();

	TInt i;

	if (iAllocated==0)
		{
		assertt(iCount==0);
		assertt(iActiveCount==0);
		assertt(iReservedFree==0);
		assertt(iReservedTotal==0);
		assertt(iReservedFreeHWM==0);
		assertt(iSlots==0);
		assertt(iAmortize==0);
		assertt(iState==ENormal);
		assertt(iModCount==255);
		assertt(iModListShift==0);
		assertt(iSpare1==0);
		for (i=-ENumFreeQ; i<0; ++i)
			{
			SSlotQLink* l = Link(i);
			assertt(l->iNext == (TInt16)i);
			assertt(l->iPrev == (TInt16)i);
			}

		ReleaseWriteLock();
		Signal();

		return;
		}
	assertt((iAllocated & (iAllocated-1))==0);
	assertt(iSlots!=0);

	TInt hwm = 0;
	TInt active = 0;
	TInt rf = 0;
	TInt rt = 0;
	TInt rfhwm = 0;

	for (i=0; i<iAllocated; ++i)
		{
		SSlot* slot = iSlots + i;
		DObject* pO = Occupant(slot);
		TBool rsvd = IsReserved(slot);
		if (pO)
			{
			TUint attrW = slot->iUsed.iAttr;
			TUint in = attrW & 0x3fffu;
			assertt(in!=0);
			TUint objtype = (attrW>>14)&0x3fu;
			TUint attr = (attrW>>20);
			if (rsvd)
				{
				attr |= 0x80000000u;
				}
			assertt(objtype == pO->iContainerID);
			TInt h = MakeHandle(i, in);
			if (aV)
				{
				aV(i, h, in, objtype, attr, pO);
				}
			hwm = i+1;
			++active;
			if (rsvd)
				{
				++rt;
				}
			}
		else if (rsvd)
			{
			hwm = i+1;
			rfhwm = i+1;
			++rf;
			++rt;
			}
		}
	assertt(iCount >= 0);
	assertt(iCount <= iAllocated);
	assertt(iCount >= hwm);
	assertt(iActiveCount == active);
	assertt(iReservedFree == rf);
	assertt(iReservedTotal == rt);
	assertt(iReservedFreeHWM >= 0);
	assertt(iReservedFreeHWM <= iAllocated);
	assertt(iReservedFreeHWM >= rfhwm);
	assertt(iAmortize>=-1 && iAmortize<=iAllocated/2);
	assertt(iState==ENormal);
	assertt(iModCount==255);
	if (iAllocated <= EBitMapSize)
		{
		assertt(iModListShift==0);
		}
	else
		{
		assertt((EBitMapSize<<iModListShift)==iAllocated);
		}
	assertt(iSpare1==0);
	if (rf>0)
		{
		TInt prev = EQRsvd;
		SSlotQLink* p = Link(prev);
		for (i=0; i<=rf; ++i)
			{
			TInt n = p->iNext;
			p = Link(n);
			assertt(p->iPrev == prev);
			prev = n;
			}
		assertt(prev == EQRsvd);
		}
	if ((iAllocated - active) > rf)
		{
		TInt nf = iAllocated - active - rf;
		TInt never_used = iAllocated - iCount;
		TInt used = nf - never_used;
		TInt prev = EQFree;
		SSlotQLink* p = Link(prev);
		for (i=0; i<=nf; ++i)
			{
			TInt n = p->iNext;
			if (i>=used && i<nf)
				{
				assertt(n==iCount+i-used);
				}
			p = Link(n);
			assertt(p->iPrev == prev);
			prev = n;
			}
		assertt(prev == EQFree);
		}
	for (i=-ENumFreeQ; i<0; ++i)
		{
		if (i==EQRsvd && rf>0)
			{
			continue;
			}
		if (i==EQFree && (iAllocated-active)>rf)
			{
			continue;
			}
		SSlotQLink* l = Link(i);
		assertt(l->iNext == (TInt16)i);
		assertt(l->iPrev == (TInt16)i);
		}
	
	ReleaseWriteLock();
	Signal();
	}
#endif





