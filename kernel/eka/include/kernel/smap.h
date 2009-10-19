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
// e32\include\kernel\smap.h

#ifndef SMAP_H
#define SMAP_H

class DMutex;

/**
	@file
	@internalComponent
	@prototype
*/

/**
   SMap uses a binary search to perform quick lookups (finds) whilst Add and
   Remove and Iterating is happening concurrently. This container is optimised
   for fast lookups.

   Add, Remove and Iterator check that the caller has the heavy wait Mutex lock
   held.

   Find methods use the fast lock whilst performing look ups.
 */

class SMap
	{
public:
	SMap(NFastMutex* aFastLock, DMutex* aSlowLock);
	~SMap();

	TInt Add(TUint aKey, TAny* aObject);
	TAny* Remove(TUint aKey);
	TAny* Find(TUint aKey);

	FORCE_INLINE TUint Count()
		{
		return iCount;
		}

	class TEntry
		{
	public:
		TUint	iKey;
		TAny*	iObj;
		};

	/**
	   An iterator index.
	*/

	class TIterator
		{
	public:
		FORCE_INLINE TIterator(SMap& aMap) : iMap(aMap), iPos(0)
			{
			}

		void Reset();

		TEntry* Next();

	private:
		SMap& iMap;
		TUint iPos;
		TInt iSpare1;
		};
private:
	FORCE_INLINE void FastLock()
		{
	    NKern::FMWait(iFastLock);
		}

	FORCE_INLINE void FastUnlock()
		{
	    NKern::FMSignal(iFastLock);
		}

	TUint FindIndex(TUint aKey);

private:
	TUint iCount;
	TEntry* iList;
	NFastMutex* iFastLock;
	DMutex* iSlowLock;
	TInt iSpare1;

	friend class TIterator;
	};



#endif // SMAP_H
