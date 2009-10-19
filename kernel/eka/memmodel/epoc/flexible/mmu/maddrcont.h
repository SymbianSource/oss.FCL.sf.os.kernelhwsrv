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

#ifndef MADDRCONT_H
#define MADDRCONT_H

/**
A container for storing objects keyed on a virtual address.
*/
class RAddressedContainer
	{
public:
	/**
	@param aReadLock	Fast mutex used to synchronise read operations,
					 	i.e. the Find functions. This may be the null pointer to
						indicate that extra locking is required.
	@param aWriteLock	Reference to the mutex used to synchronise write operations,
					 	i.e. #Add and Remove functions. This mutex if not used by
						the RAddressedContainer class but it is used for asserting
						correct preconditions in debug builds
	*/
	RAddressedContainer(NFastMutex* aReadLock, DMutex*& aWriteLock);

	~RAddressedContainer();

	/**
	Add an object to the container.

	@param aAddress The address key for the object.
	@param aObject  Pointer to the object to add. This may not be the null pointer.

	@pre The write lock must be held.
	*/
	TInt Add(TLinAddr aAddress, TAny* aObject);

	/**
	Remove an object from the container.

	@param aAddress The address key for the object.

	@return The pointer of the object removed, or the null pointer if there
			was none with the specified address key.

	@pre The write lock must be held.
	*/
	TAny* Remove(TLinAddr aAddress);

	/**
	Find an object in the container.

	@param aAddress The address key for the object.

	@return The pointer of the object found, or the null pointer if there
			was none with the specified address key.

	@pre The read lock must be held, or if there is no read lock, the write lock must be held.
	*/
	TAny* Find(TLinAddr aAddress);

	/**
	Find the object in the container which has the highest value
	address key less-than-or-equal to the specified address.

	@param aAddress The address key to search on.

	@param[out] aOffset Reference to an value which will be set to the difference
						between \a aAddress and the address key of the found object.

	@return The pointer of the object found, or the null pointer if there
			was none matching the search criteria.

	@pre The read lock must be held, or if there is no read lock, the write lock must be held.
	*/
	TAny* Find(TLinAddr aAddress, TUint& aOffset);

	/** Acquire the read lock. */
	FORCE_INLINE void ReadLock()
		{
		if(iReadLock)
			NKern::FMWait(iReadLock);
		}

	/** Release the read lock. */
	FORCE_INLINE void ReadUnlock()
		{
		if(iReadLock)
			NKern::FMSignal(iReadLock);
		}

	/** Flash (release and re-acquire) the read lock. */
	FORCE_INLINE void ReadFlash()
		{
		if(iReadLock)
			NKern::FMFlash(iReadLock);
		}

	/** The number of objects in the container. */
	FORCE_INLINE TUint Count()
		{
		return iCount;
		}
private:
	TUint FindIndex(TLinAddr aAddress);
	TUint CalculateGrow();
	TUint CalculateShrink(TUint aCount);
	TBool CheckWriteLock();

	class TEntry
		{
	public:
		TLinAddr	iAddress;
		TAny*		iObject;
		};
private:
	TUint iMaxCount;
	TUint iCount;
	TEntry* iList;
	NFastMutex* iReadLock;
	DMutex*& iWriteLock;
	};

#endif // MADDRCONT_H
