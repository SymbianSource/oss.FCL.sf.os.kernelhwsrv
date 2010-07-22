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

#ifndef MPAGEARRAY_H
#define MPAGEARRAY_H

#include "mmu.h"

const TUint KPageArraySegmentShift = 4;

/**
Number of entries in each segment RPageArray::TSegment.
*/
const TUint KPageArraySegmentSize = (1<<KPageArraySegmentShift);

const TUint KPageArraySegmentMask = KPageArraySegmentSize-1;

/**
Bit position in RPageArray::TSegment::iCount for the least significant bit
of the 'AllocCount'. I.e. the number of entries which are not empty.
*/
const TUint KPageArraySegmentAllocCountShift = 31-KPageArraySegmentShift;

const TUint KPageArraySegmentLockCountMask = (1<<KPageArraySegmentAllocCountShift)-1;

/**
Array which contains the physical addresses of all the pages contained in a DMemoryObject.
This is a sparse array, therefore memory storage may not exist for unallocated pages entries.
Where storage does exists for unallocated entries, a state value of ENotPresent indicates this.
For allocated entries, the redundant least significant bits of each entry contain flags and state
from from enum TFlags and TState.

To add pages to the array:

@code
	RPageArray::TIter iter;
	array.AddStart(index,count,iter);
		RPageArray::TIter pageList;
		while(n = iter.AddFind(pageList))
			{
			pageList.Add(n,pages);
			// or pageList.AddContiguous
			}
	array.AddEnd(index,count);
@endcode


To remove pages from the array:

@code
	RPageArray::TIter iter;
	array.FindStart(index,count,iter);
		RPageArray::TIter pageList;
		while(n = iter.RemoveFind(pageList))
			{
			pageList.Remove(n,pages);
			iter.FindRelease(n);
			}
	array.FindEnd(index,count);
@endcode

Mutual exclusion must be used to ensure that only a single Add or Remove operation is in
progress at any time.


To query the contents of the array:

@code
	RPageArray::TIter iter;
	array.FindStart(index,count,iter);
		RPageArray::TIter pageList;
		while(n=iter.Find(pageList));
			{
			TPhysAddr* pages;
			while(n = pageList.Pages(pages,max))
				{
				// do something with pages
				pageList.Skip(n);
				}
			iter.FindRelease(n);
			}
	array.FindEnd(index,count);
@endcode

*/
class RPageArray
	{
public:
	class TSegment;
	class TIter;

	/**
	States for pages stored in the array. These are stored in least significant part of each entry.
	*/
	enum TState
		{
		ENotPresent			= 0x000, ///< No page present.
		EDecommitted		= 0x001, ///< Paged Decommitted, but is pinned
		EDecommitting		= 0x002, ///< Page is in the process of being decommitted
		EStealing			= 0x003, ///< Page is in the process of being stolen
		ERestrictingNA		= 0x004, ///< Page is in the process of having no-access restrictions applied
		EMoving				= 0x005, ///< Page is in the process of being moved to another physical page
		ECommitted			= 0x006, ///< Page is committed

		EStateShift			= 3,	 ///< Number of bits needed to store state values.
		EStateMask			= (1<<EStateShift)-1, ///< Mask for state values

		EEmptyEntry			= ENotPresent ///< Value of an empty array entry
		};

	/**
	Flags stored in array entries in addition to the state.
	*/
	enum TFlags
		{
		EUnmapVetoed		= 1<<EStateShift, ///< A Steal or Decommit operation on the page has been vetoed

		EFlagsShift			= 1,	 ///< Number of bits needed to store flags.
		EFlagsMask			= ((1<<EFlagsShift)-1)<<EStateShift ///< Mask for flags values
		};

	/**
	Return true if the array entry \a aPage is currently being decommitted.
	*/
	static FORCE_INLINE TBool TargetStateIsDecommitted(TPhysAddr aPage)
		{
		return State(aPage)<=EStealing;
		}

	/**
	Return true if the array entry \a aPage is currently committed and may be being moved.
	*/
	static FORCE_INLINE TBool TargetStateIsCommitted(TPhysAddr aPage)
		{
		__ASSERT_COMPILE(RPageArray::EMoving == RPageArray::ECommitted - 1);
		return State(aPage)>=EMoving;
		}

	/**
	Return true if the array entry \a aPage is not present.
	*/
	static FORCE_INLINE TBool IsPresent(TPhysAddr aPage)
		{
		return State(aPage)!=ENotPresent;
		}

	/**
	Return the TState value in the array entry \a aPage.
	*/
	static FORCE_INLINE TState State(TPhysAddr aPage)
		{
		return (TState)(aPage&EStateMask);
		}

	/**
	Update the physical address in the array entry \a aEntry.
	@param aEntry		A reference to the entry to update.
	@param aPhysAddr	The new physical address.
	*/
	static FORCE_INLINE void PageMoveNewAddr(TPhysAddr& aEntry, TPhysAddr aPhysAddr)
		{
		__NK_ASSERT_DEBUG(!(aPhysAddr & EStateMask));
		__NK_ASSERT_DEBUG(State(aEntry) == EMoving);
		aEntry = (aEntry & EStateMask) | aPhysAddr;
		}

	static void Init2A();
	static void Init2B(DMutex* aLock);

	RPageArray();
	~RPageArray();

	/**
	Second stage constructor for the array.

	@param aMaxPages			The maximum number of entries to be stored in the array.
	@param aPreallocateMemory	If true, then all the memory required to store the array
								entries is allocated immediately - rather than on demand
								as entries are added.
	*/
	TInt Construct(TUint aMaxPages, TBool aPreallocateMemory=EFalse);

	/**
	Allocate all memory required to store array entries.
	This is only for use during system boot.
	*/
	TInt PreallocateMemory();

	/**
	Ensures the memory to store a region of array entries is allocated and locked.

	@param aIndex	Start index of region.
	@param aCount	Number of pages in region.

	@see RPageArray::Free()
	*/
	TInt Alloc(TUint aIndex, TUint aCount);

	/**
	Revert the action of #Alloc by unlocking the memory used for a region of array entries.
	Note, calling #Free for any entry more times than #Alloc was used will have
	unpredictable results.

	@param aIndex	Start index of region.
	@param aCount	Number of pages in region.
	*/
	void Free(TUint aIndex, TUint aCount);

	/**
	Prepare to add (commit) pages to a region in this array.
	This ensures the memory to store the entries is allocated and locked. It also,
	optionally and by default, check that these entries are empty.

	@param aIndex			Start index of region.
	@param aCount			Number of pages in region.
	@param[out] aIter		An iterator which covers the specified region.
	@param aAllowExisting	True if the region may contain non-empty entries.
							False to assert entries are empty.

	@see RPageArray::AddEnd()
	*/
	TInt AddStart(TUint aIndex, TUint aCount, TIter& aIter, TBool aAllowExisting=EFalse);

	/**
	End an 'add' operation started with #AddStart.
	This must be called to unlock any page array memory which #AddStart locked.

	@param aIndex	Start index of region. Must be same value as corresponding call to #AddStart.
	@param aCount	Number of pages in region. Must be same value as corresponding call to #AddStart.
	*/
	void AddEnd(TUint aIndex, TUint aCount);

	/**
	Prepare to search a region in this array.

	@param aIndex			Start index of region.
	@param aCount			Number of pages in region.
	@param[out] aIter		An iterator which covers the specified region.

	@see RPageArray::AddEnd()
	*/
	void FindStart(TUint aIndex, TUint aCount, TIter& aIter);

	/**
	End a find operation started with #FindStart.

	@param aIndex	Start index of region. Must be same value as corresponding call to #FindStart.
	@param aCount	Number of pages in region. Must be same value as corresponding call to #FindStart.
	*/
	void FindEnd(TUint aIndex, TUint aCount);

	/**
	Prepare to add (commit) a single page to this array.
	This ensures the memory to store the entry is allocated and locked.

	@param aIndex			Index of entry.
	@param[out] aPageList	An iterator represents the single array entry.

	@return Pointer to the array entry,
			or the null pointer if memory allocation failed.

	@see RPageArray::AddPage()
	@see RPageArray::AddPageEnd()
	*/
	TPhysAddr* AddPageStart(TUint aIndex, TIter& aPageList);

	/**
	Add (commit) a single page to the array.

	@param aPageEntry	The address of the array entry as returned by AddPageStart.
	@param aPage		The physical address of the page being added.
	*/
	static void AddPage(TPhysAddr* aPageEntry, TPhysAddr aPage);

	/**
	End an 'add' operation started with #AddPageStart.
	This must be called to unlock any page array memory which #AddPageStart locked.

	@param aIndex	Index of entry. Must be same value as corresponding call to #AddPageStart.
	@param aDelta	1 (one), if the array entry was changed from ENotPresent state (e.g. AddPage called),
					zero otherwise.
	*/
	void AddPageEnd(TUint aIndex, TInt aDelta);

	/**
	Prepare to remove (decommit) a single page from this array.

	This function is similar to TIter::RemoveFind and updates the array entry and
	memory locking in the same way.

	@param aIndex			Index of entry.
	@param[out] aPageList	An iterator representing the single array entry.
							Not set if this method returns the null pointer.

	@return Pointer to the array entry,
			or the null pointer if entry does not need decommitting.

	@see RPageArray::RemovePage()
	@see RPageArray::RemovePageEnd()
	*/
	TPhysAddr* RemovePageStart(TUint aIndex, TIter& aPageList);

	/**
	Remove a single page from the array.

	This function is similar to TIter::Remove and updates the array entry in the same way.

	@param aPageEntry	The address of the array entry as returned by RemovePageStart.

	@return The physical address of the page which was removed,
			or KPhysAddrInvalid if no page was removed.
	*/
	static TPhysAddr RemovePage(TPhysAddr* aPageEntry);

	/**
	End an 'remove' operation started with #RemovePageStart.
	This must be called to unlock any page array memory which #RemovePageStart locked.

	@param aIndex	Index of entry. Must be same value as corresponding call to #RemovePageStart.
	@param aDelta	1 (one), if the array entry was set ENotPresent state (RemovePage succeeded),
					zero otherwise.
	*/
	void RemovePageEnd(TUint aIndex, TInt aDelta);

	/**
	Prepare to restrict access to a single page in this array.

	If the page entry state indicates that the page is already more restricted
	than being requested, then the function returns the null pointer and does nothing.

	If the page does need its access restricting then its entry in the array is set to
	ERestrictingNA and the memory for the entry is locked.

	@param aIndex			Index of entry.
	@param[out] aPageList	An iterator representing the single array entry.
							Not set if this method returns the null pointer.

	@return Pointer to the array entry,
			or the null pointer if entry does not need it's access restricting further.

	@see RPageArray::RestrictPageNAEnd()
	*/
	TPhysAddr* RestrictPageNAStart(TUint aIndex, TIter& aPageList);

	/**
	End an 'restrict' operation started with #RestrictPageStart.
	This must be called to unlock any page array memory which #RestrictPageStart locked.

	@param aIndex	Index of entry. Must be same value as corresponding call to #RestrictPageStart.
	*/
	void RestrictPageNAEnd(TUint aIndex);

	/**
	Prepare to steal a single page from this array.

	The memory for the entry is locked and if the page entry is is in one of the committed
	states then it is changed to state EStealing.

	@param aIndex			Index of entry.
	@param[out] aPageList	An iterator representing the single array entry.
							Not set if this method returns the null pointer.

	@return Pointer to the array entry.

	@see RPageArray::StealPageEnd()
	*/
	TPhysAddr* StealPageStart(TUint aIndex, TIter& aPageList);

	/**
	End an 'steal' operation started with #StealPageStart.
	This must be called to unlock any page array memory which #StealPageStart locked.

	@param aIndex	Index of entry. Must be same value as corresponding call to #StealPageStart.
	@param aDelta	1 (one), if the array entry was set ENotPresent state (the page was stolen),
					zero otherwise.
	*/
	void StealPageEnd(TUint aIndex, TInt aDelta);

	/**
	Prepare to move a page in this array by changing its state to EMoving.

	Note - the memory entry isn't locked as the RamAllocLock mutex must be held 
	through out	the page moving process and therefore the page cannot be removed.

	@param aIndex			The index of the entry to be moved.
	@param[out] aPageList	An iterator representing the single array entry.
							Not set if this method returns the null pointer.

	@return Pointer to the array entry, NULL if the page cannot be moved.
	
	@see RPageArray::MovePageEnd()
	*/
	TPhysAddr* MovePageStart(TUint aIndex, TIter& aPageList);


	/**
	Page moving has ended so set the page back to committed if no other 
	operation has occurred/is occurring.

	@param aEntry		A reference to the entry to update.
	@param aIndex		The index of the page that was moved.
	*/
	void MovePageEnd(TPhysAddr& aEntry, TUint aIndex);


	/**
	Return the array entry for index \a aIndex.
	*/
	TPhysAddr Page(TUint aIndex);


	/**
	Return a pointer to the array entry for index \a aIndex.

	@return Pointer to the array entry, NULL if the page cannot found.
	*/
	TPhysAddr* PageEntry(TUint aIndex);

	/**
	Return the physical address of the page at index \a aIndex, or KPhysAddrInvalid if none present.
	*/
	TPhysAddr PhysAddr(TUint aIndex);

	/**
	Get the physical address for the pages stored in the specified region in the array.

	@param aIndex					Start index of region.
	@param aCount					Number of pages in region.
	@param[out] aPhysicalAddress	If all pages are physically contiguous this is set to the start address,
									otherwise this is set to KPhysAddrInvalid.
	@param[out] aPhysicalPageList	Pointer to array of \a aCount physical addresses which
									will be filled with the physical addressed of each page in the region.

	@return	0 (zero) if all pages in region are physically contiguous;
			1 (one) if pages are not physically contiguous;
			KErrNotFound, if any page in the region is not present.		
	*/
	TInt PhysAddr(TUint aIndex, TUint aCount, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList);

	enum
		{
		/**
		Maximum number of bits which can be stored in an array entry by SetPagingManagerData.
		*/
		KPagingManagerDataBits		= 32-(EFlagsShift+EStateShift),
		};

	enum
		{
		/**
		Maximum value which can be stored in an array entry by SetPagingManagerData.
		*/
		KMaxPagingManagerData		= (1u<<KPagingManagerDataBits)-1u
		};

	/**
	Write \a aValue to the paging manager data for index \a aIndex.
	The value must not exceed KMaxPagingManagerData.

	This value is stored in the page array entry, if it's state is ENotPresent;
	otherwise it is stored in the SPageInfo object for the page in the array entry.
	*/
	void SetPagingManagerData(TUint aIndex, TUint aValue);

	/**
	Return the paging manager data for index \a aIndex.
	@see RPageArray::SetPagingManagerData()
	*/
	TUint PagingManagerData(TUint aIndex);


private:
	/**
	Unlock the memory used for a region of array entries.

	@param aSegments	Copy of RPageArray::iSegments from the array.
	@param aIndex		Start index of region.
	@param aCount		Number of pages in region.
	*/
	static void Release(TSegment** aSegments, TUint aIndex, TUint aCount);

	/**
	Unlocking the memory used for a single array entry.
	This also updates

	@param aIndex The index of the array entry.
	@param aDelta The change in the 'present' state for the entry. This is
				  1 if the entry was added (state changed from ENotPresent),
				  -1 if the entry was removed (state changed to ENotPresent),
				  0 otherwise.
	*/
	void ReleasePage(TUint aIndex, TInt aDelta);

	/**
	Return the array segment in at \a aSegmentEntry, allocating a new one to this if
	none previously existed. Return the null pointer in no segment could be allocated
	(out of memory).

	The returned segment is locked (TSegment::Lock) \a aLockCount times; this normally
	represents the number of entries in the segment which are to be accesses.
	*/
	TSegment* GetOrAllocateSegment(TSegment** aSegmentEntry, TUint aLockCount);
private:
	TUint8 iPreallocatedMemory; ///< Set true, if this array was constructed with pre-allocated memory. See #Construct.
	TUint iNumSegments;			///< The number of segments in array iSegments.
	TSegment** iSegments;		///< Array of TSegment objects allocated for this array. May contain null pointers.

public:
	/**
	Class for iterating through and manipulating a section of entries in an RPageArray.
	*/
	class TIter
		{
	public:
		/**
		Find the next region of empty entries.

		@param[out] aPageList	The found region.
								The #Add or #AddContiguous method is normally subsequently used on this.

		@return The number of pages in the found region. Zero indicating no more empty entries were found.

		@post This iterator is updated start immediately after the found region returned in \a aPageList.
		*/
		TUint AddFind(TIter& aPageList);

		/**
		Add pages to the array, setting each entry state as ECommitted.

		@param aCount		The number of pages to add.
		@param aPages		Pointer to list of \a aCount physical page addresses to add.
		*/
		void Add(TUint aCount, const TPhysAddr* aPages);

		/**
		Add contiguous pages to the array, setting each entry state as ECommitted.

		@param aCount		The number of pages to add.
		@param aPhysAddr	The physical address of the first page to add.
		*/
		void AddContiguous(TUint aCount, TPhysAddr aPhysAddr);

		/**
		Update iterator and array state as if pages had been added with #Add.
		This is used after array entries have been directly manipulated rather
		than being updated through #Add.

		@param aCount	The number of pages to move this iterator on by.
		@param aChanged	The number of new entries which have been added to the array.
		*/
		void Added(TUint aCount, TUint aChanged);

		/**
		Find the next region of non-empty entries and lock the memory used to store these.

		@param[out] aPageList	The found region.
								The #Pages method is normally subsequently used on this.

		@return The number of pages in the found region. Zero indicating no more empty entries were found.

		@post This iterator is updated to start at the first found entry.

		@see RPageArray::FindRelease()
		*/
		TUint Find(TIter& aPageList);

		/**
		Unlock the page array memory locked by #Find or #RemoveFind and move this iterator
		past this region in preparation for a subsequent find operation.

		@param aCount	The number of pages returned by the corresponding find function.

		@post This iterator is updated to start immediately after the region
			  returned by the corresponding find function.
		*/
		void FindRelease(TUint aCount);

		/**
		Find the next region of entries to be removed (decommitted).
		The entries found are those which are neither empty nor in state EDecommitted.
		They are updated to state EDecommitting and the memory used to store these entries
		is locked. To unlock the memory and continue searching FindRelease

		@param[out] aPageList	The found region.
								The #Remove method is normally subsequently used on this.

		@return The number of pages in the found region. Zero indicating no more empty entries were found.

		@post This iterator is updated to start at the first found entry.

		@see RPageArray::FindRelease()
		@see RPageArray::Remove()
		*/
		TUint RemoveFind(TIter& aPageList);

		/**
		Remove pages from the array.

		For each entry found to be in the EDecommitting state (as set by #RemoveFind)
		the page address in the entry is appended to the supplied array (\a aPages) and
		the entry set to EEmptyEntry. However, if the array entry has the EUnmapVetoed flag set
		then instead the entry state is set to EDecommitted and the page address is not appended
		to \a aPages.

		@param aMaxCount	The maximum number of pages to remove.
		@param[out] aPages	Pointer to array of \a aMaxCount physical addresses which
							will be set to the physical addresses of the pages removed.

		@return The number of pages removed from the array and stored at \a aPages.

		@post This iterator is updated start immediately after the last removed entry.
		*/
		TUint Remove(TUint aMaxCount, TPhysAddr* aPages);

		/**
		Return a pointer to the array entries represented by this iterator.

		As array entries may not be stored contiguously in memory this method returns the
		number of valid entries.

		This method should only be used for array entries which have had their
		memory locked or for which there are other guarantees that the memory is present.

		@param[out] aStart	Set to the address of the first array entry.
		@param aMaxCount	The maximum count this function should return.

		@return The number of array entries starting at \a aStart which are valid.
		*/
		TUint Pages(TPhysAddr*& aStart, TUint aMaxCount=~0u);

		/**
		Move this iterator on by \a aCount pages.
		*/
		void Skip(TUint aCount);

		/**
		Prevent pages in the region covered by this iterator from having their
		access restricted. This is achieved by returning any entries currently
		in the specified 'being restricted' state to be fully committed again (state ECommitted).

		@param aPageMoving ETrue to veto pages being restricted for page moving (EMoving). Set to EFalse otherwise.
		*/
		void VetoRestrict(TBool aPageMoving);

		/**
		Prevent pages in the region covered by this iterator from being removed from the array.
		This is achieved by setting the EUnmapVetoed flag for all entries with a current state
		indicating they are being decommitted, c.f. TargetStateIsDecommitted.
		*/
		void VetoUnmap();

		/**
		Default constructor which does not initialise members.
		*/
		TIter();

		/**
		Return a new iterator which represents the array region [aIndex..aEndIndex).
		The new region is asserted to be within that specified by this iterator.
		*/
		TIter Slice(TUint aIndex, TUint aEndIndex);

		/**
		Return a new iterator which represents the first \a aCount pages of this one.
		*/
		TIter Left(TUint aCount);

		/**
		Return the start index of the region being represented by this iterator.
		*/
		FORCE_INLINE TUint Index() const
			{ return iIndex; }

		/**
		Return the index immediately after the being represented by this iterator.
		*/
		FORCE_INLINE TUint IndexEnd() const
			{ return iEndIndex; }

		/**
		Return the number of entries in the region represented by this iterator.
		*/
		FORCE_INLINE TUint Count() const
			{ return iEndIndex-iIndex; }

	private:
		TIter(TSegment** aSegments, TUint aIndex, TUint aEndIndex);
		void Set(TSegment** aSegments, TUint aIndex, TUint aEndIndex);
	private:
		TSegment** iSegments;	///< Copy of RPageArray::iSegments of the array being represented by this iterator.
		TUint iIndex;			///< Start index of the array region being represented by this iterator.
		TUint iEndIndex;		///< The index immediately after the array region being represented by this iterator.

		friend class RPageArray;
		};

	/**
	Class representing the memory storage for a 'segment' of entries in an RPageArray.
	Each segment contains storage for #KPageArraySegmentSize entries and the number
	of these which are not #EEmptyEntry are counted by the 'alloc count'.
	Each segment also has a 'lock count' which acts as a reference count preventing
	the segment from being deleted whilst it is being manipulated.
	Both of these counts are combined in #iCounts.
	*/
	class TSegment
		{
	private:
		/**
		Return a newly allocated segment or the null pointer if out-ot-memory.
		*/
		static TSegment* New();

		/**
		Delete \a aSegment and return the null pointer.
		*/
		static TSegment* Delete(TSegment* aSegment);

		/**
		Lock this segment \a aCount times. This prevents the segment being deleted.
		*/
		void Lock(TUint aCount=1);

		/**
		Unlock \a aSegment \a aCount times.
		If the lock count reaches zero and the segment has no allocated entries
		then it is deleted and \a aSegment set to the null pointer.
		*/
		static TBool Unlock(TSegment*& aSegment, TUint aCount=1);

		/**
		Adjust the allocation count for this segment by \a aDelta.
		The allocation count keeps count of the number of entries which are not #EEmptyEntry.
		*/
		void AdjustAllocCount(TInt aDelta);

		/**
		Debug function which outputs the contents of this segment to the kernel debug port.
		*/
		void Dump();
	private:
		/**
		Storage for each array entry.
		*/
		TPhysAddr iPages[KPageArraySegmentSize];

		/**
		Two count values are stored in this member.
		Bits 0..KPageArraySegmentAllocCountShift-1 is the 'lock count' modified by the
		Lock and Unlock methods.
		Bits KPageArraySegmentAllocCountShift..31 is the 'alloc count' modified by the
		AdjustAllocCount method.
		When both counts are zero, this segment is empty and not being used,
		and can therefore be deleted.
		Note, the alloc count is only valid when the lock count is zero, i.e.
		after all users have finished updating this segment.
		*/
		TUint iCounts;

		friend class RPageArray;
		friend class TIter;
		};

	friend class RPageArray::TSegment;
	friend class RPageArray::TIter;
	};



//
// RPageArray::TIter
//

FORCE_INLINE RPageArray::TIter::TIter()
	{
#ifdef _DEBUG
	iSegments = 0;
	iIndex = 0;
	iEndIndex = ~0u;
#endif
	}

FORCE_INLINE RPageArray::TIter::TIter(RPageArray::TSegment** aSegments, TUint aIndex, TUint aEndIndex)
	: iSegments(aSegments), iIndex(aIndex), iEndIndex(aEndIndex)
	{
	__NK_ASSERT_DEBUG(iEndIndex>=aIndex);
	}

FORCE_INLINE RPageArray::TIter RPageArray::TIter::Slice(TUint aIndex, TUint aEndIndex)
	{
	__NK_ASSERT_DEBUG(aEndIndex>=aIndex);
	__NK_ASSERT_DEBUG(aIndex>=iIndex);
	__NK_ASSERT_DEBUG(aEndIndex<=iEndIndex);
	return TIter(iSegments,aIndex,aEndIndex);
	}

FORCE_INLINE RPageArray::TIter RPageArray::TIter::Left(TUint aCount)
	{
	__NK_ASSERT_DEBUG(aCount<=Count());
	return TIter(iSegments,iIndex,iIndex+aCount);
	}

FORCE_INLINE void RPageArray::TIter::Skip(TUint aCount)
	{
	__NK_ASSERT_DEBUG(iIndex+aCount>=iIndex);
	__NK_ASSERT_DEBUG(iIndex+aCount<=iEndIndex);
	iIndex += aCount;
	}


//
// RPageArray
//

FORCE_INLINE void RPageArray::FindEnd(TUint /*aIndex*/, TUint /*aCount*/)
	{
	// nothing to do
	}

FORCE_INLINE void RPageArray::AddPageEnd(TUint aIndex, TInt aDelta)
	{
	MmuLock::Lock();
	ReleasePage(aIndex,aDelta);
	MmuLock::Unlock();
	}

FORCE_INLINE void RPageArray::AddPage(TPhysAddr* aPageEntry, TPhysAddr aPage)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG((aPage&KPageMask)==0);
	__NK_ASSERT_DEBUG(!RPageArray::IsPresent(*aPageEntry));
	*aPageEntry = aPage|RPageArray::ECommitted;
	}

FORCE_INLINE void RPageArray::RestrictPageNAEnd(TUint aIndex)
	{
	ReleasePage(aIndex,0);
	}

FORCE_INLINE void RPageArray::StealPageEnd(TUint aIndex, TInt aDelta)
	{
	ReleasePage(aIndex,-aDelta);
	}

FORCE_INLINE void RPageArray::RemovePageEnd(TUint aIndex, TInt aDelta)
	{
	MmuLock::Lock();
	ReleasePage(aIndex,-aDelta);
	MmuLock::Unlock();
	}

FORCE_INLINE void RPageArray::MovePageEnd(TPhysAddr& aEntry, TUint aIndex)
	{
	__NK_ASSERT_DEBUG(PageEntry(aIndex) == &aEntry);
	if (State(aEntry) == EMoving)
		aEntry = (aEntry & ~EStateMask) | ECommitted;
	ReleasePage(aIndex, 0);
	}

#endif
