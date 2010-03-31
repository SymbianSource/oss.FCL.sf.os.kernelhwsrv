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
// Classes which implement a circular doubly linked list of DMemoryMappingBase objects.
// 
//

/**
 @file
 @internalComponent
*/

#ifndef MMAPPINGLIST_H
#define MMAPPINGLIST_H

#include <kernel.h>
#include <nk_priv.h>

class DMemoryMappingBase;



//
// TMappingListBase
//

/**
Base class implementing basic list insertion and removal functionality for memory mapping lists.
*/
class TMappingListBase
	{
#ifdef _DEBUG
public:
	/**
	Debug check returning true if iSpinLock is held by the current thread.
	In reality this just checks that interrupts are currently disabled as spinlocks
	do not support identifying the thread which holds it.
	*/
	FORCE_INLINE TBool LockIsHeld()
		{
		return InterruptsStatus(false);
		}
#endif

protected:
	/** Class type id of this object, this is stored in iType. */
	enum TType
		{
		EHead = 1,	///< This object is a TMappingList object
		ELink,		///< This object is a TMappingListLink object
		EIter		///< This object is a TMappingListIter object
		};

	FORCE_INLINE TMappingListBase(TType aType)
		: iType(aType)
		{
		KillLink();
		}

	/** Remove this link from the list it is in. */
	FORCE_INLINE void Remove()
		{
		__NK_ASSERT_DEBUG(!IsLinkDead());
		TMappingListBase* next = iNext;
		TMappingListBase* prev = iPrev;
		next->iPrev=prev;
		prev->iNext=next;
		KillLink();
		}

	/** Insert this link after \a aPrev */
	FORCE_INLINE void InsertAfter(TMappingListBase* aPrev)
		{
		__NK_ASSERT_DEBUG(IsLinkDead());
		TMappingListBase* next = aPrev->iNext;
		iPrev = aPrev;
		iNext = next;
		next->iPrev = this;
		aPrev->iNext = this;
		}

	/** Insert this link before \a aNext */
	FORCE_INLINE void InsertBefore(TMappingListBase* aNext)
		{
		__NK_ASSERT_DEBUG(IsLinkDead());
		TMappingListBase* prev = aNext->iPrev;
		iNext = aNext;
		iPrev = prev;
		aNext->iPrev = this;
		prev->iNext = this;
		}

	/**
	In debug builds, munge the iNext and iPrev link values so that if
	they are used an exception is likely.
	*/
	FORCE_INLINE void KillLink()
		{
		#ifdef _DEBUG
			iNext = (TMappingListBase*)KILL_LINK_VALUE;
			iPrev = (TMappingListBase*)KILL_LINK_VALUE;
		#endif
		}

#ifdef _DEBUG

	/** Debug check returning true if this link is 'dead', i.e. been killed with KillLink. */
	FORCE_INLINE TBool IsLinkDead()
		{
		// we don't check iNext here because that may have been set to null by
		// TMappingListLink as a non-debug indicator of a link not in a list.
		return iPrev==(TMappingListBase*)KILL_LINK_VALUE;
		}
#endif

protected:
	TMappingListBase* iNext;	///< Next item in list.
	TMappingListBase* iPrev;	///< Previous item in list.
	TUint8 iType; 				///< Value from enum TType which identifies this object's derived type.
public:
	TUint8 iSpare1;				///< Unused storage available for any purpose.
	TUint8 iSpare2;				///< Unused storage available for any purpose.
	TUint8 iSpare3;				///< Unused storage available for any purpose.

protected:
	static TSpinLock iSpinLock;	///< Global spinlock used for manipulating all lists.

	friend class TMappingListIter;
	};


//
// TMappingList
//

/**
Head of a circular doubly linked list of DMemoryMappingBase objects.
*/
class TMappingList : public TMappingListBase
	{
public:
	/** Construct an empty list. */
	FORCE_INLINE TMappingList()
		: TMappingListBase(EHead)
		{
		iNext = this;
		iPrev = this;
		}

	/** Destructor which asserts in debug builds that this list is empty. */
	FORCE_INLINE ~TMappingList()
		{
		__NK_ASSERT_DEBUG(IsEmpty());
		}

	/**
	Add \a aMapping to the end of this list.
	@pre The list must be locked with #Lock.
	*/
	void Add(DMemoryMappingBase* aMapping);

	/**
	Remove \a aMapping from this list.
	@pre The list must be locked with #Lock.
	*/
	void Remove(DMemoryMappingBase* aMapping);

	/**
	Lock the list to prevent other threads changing it.
	This is a spinlock so all operations with the lock held must be very sort.
	@see Unlock
	*/
	FORCE_INLINE void Lock()
		{
		__NK_ASSERT_DEBUG(!LockIsHeld());
		__SPIN_LOCK_IRQ(iSpinLock);
		}

	/**
	Reverse the action of #Lock.
	*/
	FORCE_INLINE void Unlock()
		{
		__NK_ASSERT_DEBUG(LockIsHeld());
		__SPIN_UNLOCK_IRQ(iSpinLock);
		}

	/** Return true if this list is empty */
	FORCE_INLINE TBool IsEmpty() const
		{
		return iNext==(TMappingListBase*)this;
		}

	// Utility methods to call a method on every element of a list

	/**
	Update the page table entry for a specified page in all mappings in the list that contain it.

	@param aPageArray			The page array entry of the page in a memory object. 
								Only array entries which have a target state of 
								RPageArray::ECommitted should be mapped into the 
								mapping's page tables.
	@param aIndex				The index of the page in the memory object.
	@param aInvalidateTLB		Set to ETrue when the TLB entries associated with this page
								should be invalidated.  This must be done when there is 
								already a valid pte for this page, i.e. if the page is still 
								mapped.
	@see #DMemoryMappingBase::RemapPage
	*/
	void RemapPage(TPhysAddr& aPageArray, TUint aIndex, TBool aInvalidateTLB);	
	};



//
// TMappingListLink
//

/**
A link in a circular doubly linked list of DMemoryMappingBase objects.
This object is the member DMemoryMappingBase::iLink.
*/
class TMappingListLink : public TMappingListBase
	{
public:
	/** Construct a link which is not part of any list. */
	FORCE_INLINE TMappingListLink()
		: TMappingListBase(ELink)
		{
		// iNext is the null pointer when link is not on list...
		iNext = 0;
		}

	/** Destructor which asserts in debug builds that this link isn't in a list. */
	FORCE_INLINE ~TMappingListLink()
		{
		__NK_ASSERT_DEBUG(!IsLinked());
		}

	/** Return true if link is in any list. */
	FORCE_INLINE TBool IsLinked()
		{
		return iNext!=0;
		}

private:
	/**
	Remove this link from the list it is in.
	This is for use by class TMappingList.
	*/
	FORCE_INLINE void Remove()
		{
		__NK_ASSERT_DEBUG(LockIsHeld());
		__NK_ASSERT_DEBUG(iNext);
		TMappingListBase::Remove();
		iNext = 0;
		}

	friend class TMappingList;
	};



//
// TMappingListIter
//

/**
An iterator for visiting all memory mappings in a list.

The iterator object itself is linked into the list so that mappings can safely
be removed and added to the list whilst it is being iterated. As new mappings
are only ever added to the end of the list, this ensures that once the iterator
reaches the end of the list it has found all mappings currently present.

As the spinlock used to guard lists disables interrupts it is very important
to only hold the lock for a very short and bounded time. Specifically, as the
Next() function doesn't necessarily release the lock, any loop iterating over
a list must take care to Unlock() then Lock() again sufficiently often to avoid
adversely impacting interrupt latency.

Example usage...

@code
	DMemoryObject* memory;
	memory->iMappings.Lock();
	TMappingListIter iter;
	DMemoryMappingBase* mapping = iter.Start(memory->iMappings);
	while(mapping)
		{
		mapping->Open();
		memory->iMappings.Unlock();
		// mapping may now be used without risk of it being deleted;
		// however, it may still be removed from the list and updated/reused
		// therefore caution must be exercised
		mapping->Close();
		memory->iMappings.Lock();
		mapping = iter.Next();
		}
	iter.Finish();
	memory->iMappings.Unlock();
@endcode
*/
class TMappingListIter : public TMappingListBase
	{
public:
	FORCE_INLINE TMappingListIter()
		: TMappingListBase(EIter)
		{
		}

	/** Destructor which asserts in debug builds that this iterator isn't attached to a list. */
	FORCE_INLINE ~TMappingListIter()
		{
		// check that this isn't still linked into a list...
		__NK_ASSERT_DEBUG(IsLinkDead());
		}

	/**
	Attach this iterator to list \a aList and return the first mapping in that list.
	The null pointer is returned if the list is empty.

	Must only be called on a newly constructed iterator object, or after #Finish
	has removed this iterator from a list.

	@pre The list must be locked with TMappingList::Lock
	*/
	DMemoryMappingBase* Start(TMappingList& aList);

	/**
	Return the next mapping in the list after the previous one returned by either
	#Start or #Next. The null pointer is returned if the list end has been reached.

	During its operation, this function may or may not have unlocked the list using
	TMappingList::Unlock. Therefore, when using Next() in a loop, care care must be
	taken to unlock the list sufficiently often to avoid adversely impacting interrupt
	latency.

	@pre The list must be locked with TMappingList::Lock.
	@pre The iterator must have been previously attached to a list with #Start and not
		 since been removed with #Finish.

	@post The list will be locked with TMappingList::Lock but this lock may have been
		  released for a period of time by the function during its operation.
	*/
	DMemoryMappingBase* Next();

	/**
	Remove this iterator from the list it has been attached to.

	@pre The list must be locked with TMappingList::Lock
	@pre The iterator must have been previously attached to a list with #Start and not
		 since been removed with #Finish.
	*/
	void Finish();

private:
	/** Implementation factor for #Start and #Next() */
	DMemoryMappingBase* Next(TMappingListBase* aPrev);
	};



#endif
