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

#include <kernel/kern_priv.h>
#include "maddrcont.h"

RAddressedContainer::RAddressedContainer(NFastMutex* aReadLock, DMutex*& aWriteLock)
	: iMaxCount(0), iCount(0), iList(0), iReadLock(aReadLock), iWriteLock(aWriteLock)
	{}


RAddressedContainer::~RAddressedContainer()
	{
	Kern::Free(iList);
	}


const TUint MinCount = 8; // must be power of two

FORCE_INLINE TUint RAddressedContainer::CalculateGrow()
	{
	TUint newMax = iMaxCount;

	if(newMax<MinCount)
		newMax = MinCount;
	else
		newMax <<= 1;
	return newMax;
	}


TUint RAddressedContainer::CalculateShrink(TUint aCount)
	{
	TUint newMax = iMaxCount;
	if(!aCount)
		return 0; // empty container should have zero max size

#ifndef _DEBUG
	// we want at least 'MinCount' free slots after shrinking for hysteresis,
	// but not in UDEB builds because OOM testing will barf...
	aCount += MinCount;
#endif

	TUint tryMax = newMax;
	do
		{
		newMax = tryMax;
		tryMax >>= 1;
		}
	while(tryMax>=aCount);

	if(newMax<MinCount)
		newMax = MinCount;

	return newMax;
	}


TBool RAddressedContainer::CheckWriteLock()
	{
	if(K::Initialising)
		return true; // check disabled during boot as we are single threaded at that point
	if(!iWriteLock)
		return false;
	if((((TLinAddr)iWriteLock)&1))
		return true; // its a lock from DMutexPool, we can't check that, assume it's OK
	return iWriteLock->iCleanup.iThread==&Kern::CurrentThread();
	}


#ifdef _DEBUG
const TUint KMaxEntriesInOneGo = 4; // small number to improve test coverage
#else
const TUint KMaxEntriesInOneGo = 32; // to produce a similar worst case number of cache line accesses to the binary search
#endif

TInt RAddressedContainer::Add(TLinAddr aAddress, TAny* aObject)
	{
	__NK_ASSERT_DEBUG(aObject);
	__ASSERT_CRITICAL;
	__NK_ASSERT_DEBUG(CheckWriteLock());

#ifdef _DEBUG
	if(K::CheckForSimulatedAllocFail())
		{
		__KTRACE_OPT(KMMU,Kern::Printf("RAddressedContainer::Add returns simulated OOM %d",KErrNoMemory));
		return KErrNoMemory;
		}
#endif

	// find list insertion point...
	TUint i = FindIndex(aAddress);

	if(iCount<iMaxCount)
		{
		// insert new entry...
		ReadLock();

		// make room by shuffling entries up in the array KMaxEntriesInOneGo at a time...
		TEntry* entry = iList+i;
		TEntry* prev = iList+iCount;
		++iCount; // must do this before releasing read lock for the first time
		for(;;)
			{
			TEntry* next = prev-KMaxEntriesInOneGo;
			if(next<=entry)
				{
				// move the final remaining entries...
				wordmove(entry+1,entry,(TUintPtr)prev-(TUintPtr)entry);
				break;
				}
			wordmove(next+1,next,(TUintPtr)prev-(TUintPtr)next);
			prev = next;
			// flash the read lock to give readers a chance to look at the list...
			ReadFlash();
			// Note, readers may see a duplicate entry in the list at 'prev' but this
			// is OK as the Find functions will still work.
			}

		// copy in new entry...
		entry->iAddress = aAddress;
		entry->iObject = aObject;

		ReadUnlock();
		}
	else
		{
		// list memory needs expanding...
		TUint newMaxCount = CalculateGrow();

		// allocate new memory...
		TEntry* newList = (TEntry*)Kern::Alloc(sizeof(TEntry)*newMaxCount);
		if(!newList)
			return KErrNoMemory;
		#ifdef _DEBUG
			if(iList)
				K::Allocator->DebugFunction(RAllocator::ECopyDebugInfo, iList, newList);
		#endif
		iMaxCount = newMaxCount;

		// copy list to new memory, and insert new entry...
		wordmove(newList,iList,sizeof(TEntry)*i);
		TEntry* entry = newList+i;
		entry->iAddress = aAddress;
		entry->iObject = aObject;
		wordmove(entry+1,iList+i,sizeof(TEntry)*(iCount-i));

		// start using new list...
		TEntry* oldList = iList;
		ReadLock();
		iList = newList;
		++iCount;
		ReadUnlock();

		// free memory used for old list...
		Kern::Free(oldList);
		}

	return KErrNone;
	}


TAny* RAddressedContainer::Remove(TLinAddr aAddress)
	{
	__ASSERT_CRITICAL;
	__NK_ASSERT_DEBUG(CheckWriteLock());

	// search for it...
	TUint i = FindIndex(aAddress);
	TEntry* entry = iList+i-1;

	if(!i || entry->iAddress!=aAddress)
		{
		// not found...
		return 0;
		}
	--i; // make 'i' the index of entry to remove

	// get object...
	TAny* result = entry->iObject;
	__NK_ASSERT_DEBUG(result);

	TUint newMaxCount = CalculateShrink(iCount-1);

	if(newMaxCount>=iMaxCount)
		{
remove_without_resize:
		// remove old entry...
		ReadLock();

		// shuffling entries down in the array KMaxEntriesInOneGo at a time...
		TEntry* prev = iList+i+1;
		TEntry* end = iList+iCount;
		for(;;)
			{
			TEntry* next = prev+KMaxEntriesInOneGo;
			if(next>=end)
				{
				// move the final remaining entries...
				wordmove(prev-1,prev,(TUintPtr)end-(TUintPtr)prev);
				break;
				}
			wordmove(prev-1,prev,(TUintPtr)next-(TUintPtr)prev);
			prev = next;
			// flash the read lock to give readers a chance to look at the list...
			ReadFlash();
			// Note, readers may see a duplicate entry at the end of the list but this
			// is OK as the Find functions will still work.
			}

		--iCount; // must do this after moving all the entries
		ReadUnlock();		
		}
	else
		{
		// list memory needs shrinking...

		// allocate new memory...
		TEntry* newList = 0;
		if(newMaxCount)
			{
			newList = (TEntry*)Kern::Alloc(sizeof(TEntry)*newMaxCount);
			if(!newList)
				goto remove_without_resize; // have no memory to shrink array
			#ifdef _DEBUG
				if(iList)
					K::Allocator->DebugFunction(RAllocator::ECopyDebugInfo, iList, newList);
			#endif
			}
		iMaxCount = newMaxCount;

		// copy list to new memory, deleting removed entry...
		wordmove(newList,iList,sizeof(TEntry)*i);
		wordmove(newList+i,iList+i+1,sizeof(TEntry)*(iCount-i-1));

		// start using new list...
		TEntry* oldList = iList;
		ReadLock();
		iList = newList;
		--iCount;
		ReadUnlock();

		// free memory used for old list...
		Kern::Free(oldList);
		}

	return result;
	}


TAny* RAddressedContainer::Find(TLinAddr aAddress)
	{
	if(iReadLock)
		__NK_ASSERT_DEBUG(iReadLock->HeldByCurrentThread());
	else
		__NK_ASSERT_DEBUG(CheckWriteLock());

	TUint i = FindIndex(aAddress);
	TEntry* entry = iList+i;
	if(i==0)
		return 0;
	--entry;

	if(aAddress!=entry->iAddress)
		return 0;

	TAny* result = entry->iObject;
	__NK_ASSERT_DEBUG(result);
	return result;
	}


TAny* RAddressedContainer::Find(TLinAddr aAddress, TUint& aOffset)
	{
	if(iReadLock)
		__NK_ASSERT_DEBUG(iReadLock->HeldByCurrentThread());
	else
		__NK_ASSERT_DEBUG(CheckWriteLock());

	TUint i = FindIndex(aAddress);
	TEntry* entry = iList+i;
	if(i==0)
		return 0;
	--entry;

	aOffset = aAddress-entry->iAddress;

	TAny* result = entry->iObject;
	__NK_ASSERT_DEBUG(result);
	return result;
	}


TUint RAddressedContainer::FindIndex(TLinAddr aAddress)
	{
	TEntry* list = iList;
#if 0
	// linear search from end...
	TUint count = iCount;
	while(count)
		{
		if(list[count-1].iAddress<=aAddress)
			break;
		--count;
		}
	return count;
#else
	// binary search...
	TUint l = 0;
	TUint r = iCount;
	TUint m;
	while(l<r)
		{
		m = (l+r)>>1;
		TUint32 x = list[m].iAddress;
		if(x<=aAddress)
			l = m+1;
		else
			r = m;
		}
	return r;
#endif
	}


