// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\smap.cpp

#include <kernel/kern_priv.h>
#include "smap.h"


/**
   SMap uses a binary search to perform quick lookups (finds) whilst Add and
   Remove and Iterating is happening concurrently. This container is optimised
   for fast lookups.

   Add, Remove and Iterator check that the caller has the heavyweight (slow) mutex lock
   held.

   Find methods take the fast lock whilst performing look ups.
 */

SMap::SMap(NFastMutex* aFastLock, DMutex* aSlowLock)
	:  iCount(0), iList(0), iFastLock(aFastLock), iSlowLock(aSlowLock)
	{}

SMap::~SMap()
	{
	Kern::Free(iList);
	}

/**
	Add checks that the caller has the heavyweight mutex held, determines the
	entry point in the array for the new entry and performs a copy and swap
	with a newly-allocated array.
*/
TInt SMap::Add(TUint aKey, TAny* aObject)
	{
	__NK_ASSERT_DEBUG(aObject);
	__ASSERT_MUTEX(iSlowLock);

	__ASSERT_CRITICAL;

#ifdef _DEBUG
	if (K::CheckForSimulatedAllocFail())
		{
		__KTRACE_OPT(KMMU,Kern::Printf("SMap::Add returns simulated OOM %d",KErrNoMemory));
		return KErrNoMemory;
		}
#endif
	TEntry* newlist = (TEntry*)Kern::Alloc(sizeof(TEntry)*(iCount + 1));

	if (!newlist)
		return KErrNoMemory;

	// find list insertion point...
	TUint i = FindIndex(aKey);
	TEntry* entry = newlist + i;

	// make room...
	if (i)
		{
		TUint moveSize = i * sizeof(TEntry);
		wordmove(newlist, iList, moveSize);
		}

	TUint moveSize = (iCount - i) * sizeof(TEntry);
	wordmove(entry + 1, iList + i,moveSize);

#ifdef _DEBUG
	if (iList)
		K::Allocator->DebugFunction(RAllocator::ECopyDebugInfo, iList, newlist);
#endif

	// copy in new entry...
	entry->iKey = aKey;
	entry->iObj = aObject;
	TEntry* cleanup = iList;

	FastLock();
	// swap
	++iCount;
	iList = newlist;
	FastUnlock();

	Kern::Free(cleanup);
	return KErrNone;
	}

/**
   Remove checks that the caller has the heavyweight mutex held, determines the
   entry point in the array for the entry to be removed. It shuffles the entries
   left flashing the fast lock so as not to prevent 'finds' from being blocked.
*/
TAny* SMap::Remove(TUint aKey)
	{
	__ASSERT_CRITICAL;
	__ASSERT_MUTEX(iSlowLock);

	// search for it...
	TUint i = FindIndex(aKey);
	TEntry* entry = iList + i - 1;

	if (!i || entry->iKey != aKey)
		{
		// not found...
		return NULL;
		}

	// get objects...
	TAny* result = entry->iObj;
	__NK_ASSERT_DEBUG(result);

	FastLock();
	if (iCount > 1)
		{
		// the shuffle the entries a maximum of 4 at a time
		// in the existing memory flashing the lock, so as not to hold it too
		// long
		// Note Add and Remove cannot be performed while shuffling the
		// entries, but fast lookups can
		while (i < iCount)
			{
			TInt entries = Min((TInt)(iCount - i), 4);
			wordmove(&iList[i-1], &iList[i], entries * sizeof(TEntry));
			i += entries;
			NKern::FMFlash(iFastLock);
			}
		}

	--iCount;
	FastUnlock();
	return result;
	}

/**
   FindIndex returns the index into an array of entries for a given key, by
   performing a binary search.

   @returns    Zero if entry not found, otherwise returns 1 + index into
               the array of entries
*/
TUint SMap::FindIndex(TUint aKey)
	{
	TEntry* list = iList;

	TUint l = 0;
	TUint r = iCount;
	TUint m;

	while (l < r)
		{
		m = (l + r) >> 1;
		TUint32 x = list[m].iKey;
		if (x <= aKey)
			l = m + 1;
		else
			r = m;
		}

	return r;
	}

/**
   Find returns a pointer to the data (TAny) corresponding to the lookup key

   @returns    Returns NULL if entry not found, otherwise a pointer to the
               data corresponding to the key
*/
TAny* SMap::Find(TUint aKey)
	{
	__NK_ASSERT_DEBUG(iFastLock->HeldByCurrentThread());

	TUint i = FindIndex(aKey);

	if (i == 0)
		return NULL;

	TEntry* entry = iList + i;

	--entry;

	if (aKey != entry->iKey)
		return NULL;

	TAny* result = entry->iObj;
	__NK_ASSERT_DEBUG(result);
	return result;
	}

void SMap::TIterator::Reset()
	{
	iPos = 0;
	}

SMap::TEntry* SMap::TIterator::Next()
	{
	__ASSERT_MUTEX(iMap.iSlowLock);

	if (iPos < iMap.iCount)
		return &iMap.iList[iPos++];

	return NULL;
	}
