// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file
 @internalComponent
*/

#ifndef __MMU_H__
#define __MMU_H__

#include "mm.h"
#include "mmboot.h"
#include <mmtypes.h>
#include <kern_priv.h>


class DCoarseMemory;
class DMemoryObject;
class DMemoryMapping;

/**
A page information structure giving the current use and state for a
RAM page being managed by the kernel.

Any modification to the contents of any SPageInfo structure requires the
#MmuLock to be held. The exceptions to this is when a page is unused (#Type()==#EUnused),
in this case only the #RamAllocLock is required to use #SetAllocated(), #SetUncached(),
and #CacheInvalidateCounter().

These structures are stored in an array at the virtual address #KPageInfoLinearBase
which is indexed by the physical address of the page they are associated with, divided
by #KPageSize. The memory for this array is allocated by the bootstrap and it has
unallocated regions where no memory is required to store SPageInfo structures.
These unallocated memory regions are indicated by zeros in the bitmap stored at
#KPageInfoMap.
*/
struct SPageInfo
	{
	/**
	Enumeration for the usage of a RAM page. This is stored in #iType.
	*/
	enum TType
		{
		/**
		No physical RAM exists for this page.

		This represents memory which doesn't exist or is not part of the physical
		address range being managed by the kernel.
		*/
		EInvalid,

		/**
		RAM fixed at boot time.

		This is for memory which was allocated by the bootstrap and which
		the kernel does not actively manage.
		*/
		EFixed,

		/**
		Page is unused.

		The page is either free memory in Mmu::iRamPageAllocator or the demand
		paging 'live' list.

		To change from or to this type the #RamAllocLock must be held.
		*/
		EUnused,

		/**
		Page is in an indeterminate state.

		A page is placed into this state by Mmu::PagesAllocated when it is
		allocated (ceases to be #EUnused). Once the page has been assigned to 
		its new use its type will be updated.
		*/
		EUnknown,

		/**
		Page was allocated with Mmu::AllocPhysicalRam, Mmu::ClaimPhysicalRam
		or is part of a reserved RAM bank set at system boot.
		*/
		EPhysAlloc,

		/**
		Page is owned by a memory object.

		#iOwner will point to the owning memory object and #iIndex will
		be the page index into its memory for this page.
		*/
		EManaged,

		/**
		Page is being used as a shadow page.

		@see DShadowPage.
		*/
		EShadow
		};


	/**
	Flags stored in #iFlags.

	The least significant bits of these flags are used for the #TMemoryAttributes
	value for the page.
	*/
	enum TFlags
		{
		// lower bits hold TMemoryAttribute value for this page

		/**
		Flag set to indicate that the page has writable mappings.
		(This is to facilitate demand paged memory.)
		*/
		EWritable			= 1<<(EMemoryAttributeShift),

		/**
		Flag set to indicate that the memory page contents may be different
		to those previously saved to backing store (contents are 'dirty').
		This is set whenever a page gains a writeable mapping and only every
		cleared once a demand paging memory manager 'cleans' the page.
		*/
		EDirty				= 1<<(EMemoryAttributeShift+1)
		};


	/**
	State for the page when being used to contain demand paged content.
	*/
	enum TPagedState
		{
		/**
		Page is not being managed for demand paging purposes, or has been transiently
		removed from the demand paging live list.
		*/
		EUnpaged 			= 0x0,

		/**
		Page is in the live list as a young page.
		*/
		EPagedYoung 		= 0x1,

		/**
		Page is in the live list as an old page.
		*/
		EPagedOld 			= 0x2,

		/**
		Page was pinned but it has been moved but not yet freed.
		*/
		EPagedPinnedMoved	= 0x3,

		/**
		Page has been removed from live list to prevent contents being paged-out.
		*/
		// NOTE - This must be the same value as EStatePagedLocked as defined in mmubase.h
		EPagedPinned 		= 0x4,

		/**
		Page is in the live list as one of oldest pages that is clean.
		*/
		EPagedOldestClean	= 0x5,

		/**
		Page is in the live list as one of oldest pages that is dirty.
		*/
		EPagedOldestDirty 	= 0x6	
		};


	/**
	Additional flags, stored in #iFlags2.
	*/
	enum TFlags2
		{
		/**
		When #iPagedState==#EPagedPinned this indicates the page is a 'reserved' page
		and is does not increase free page count when returned to the live list.
		*/
		EPinnedReserve	= 1<<0,
		};

private:
	/**
	Value from enum #TType, returned by #Type().
	*/
	TUint8 iType;

	/**
	Bitmask of values from #TFlags, returned by #Flags().
	*/
	TUint8 iFlags;

	/**
	Value from enum #TPagedState, returned by #PagedState().
	*/
	TUint8 iPagedState;

	/**
	Bitmask of values from #TFlags2.
	*/
	TUint8 iFlags2;

	union
		{
		/**
		The memory object which owns this page.
		Always set for #EManaged pages and can be set for #PhysAlloc pages.
		*/
		DMemoryObject* iOwner;

		/**
		A pointer to the SPageInfo of the page that is being shadowed.
		For use with #EShadow pages only.
		*/
		SPageInfo* iOriginalPageInfo;
		};

	/**
	The index for this page within within the owning object's (#iOwner) memory.
	*/
	TUint32 iIndex;

	/**
	Pointer identifying the current modifier of the page. See #SetModifier.
	*/
	TAny* iModifier;

	/**
	Storage location for data specific to the memory manager object handling this page.
	See #SetPagingManagerData.
	*/
	TUint32 iPagingManagerData;

	/**
	Union of values which vary depending of the current value of #iType.
	*/
	union
		{
		/**
		When #iType==#EPhysAlloc, this stores a count of the number of memory objects
		this page has been added to.
		*/
		TUint32 iUseCount;

		/**
		When #iType==#EUnused, this stores the value of Mmu::iCacheInvalidateCounter
		at the time the page was freed. This is used for some cache maintenance optimisations.
		*/
		TUint32 iCacheInvalidateCounter;

		/**
		When #iType==#EManaged, this holds the count of the number of times the page was pinned.
		This will only be non-zero for demand paged memory.
		*/
		TUint32 iPinCount;
		};

public:
	/**
	Used for placing page into linked lists. E.g. the various demand paging live lists.
	*/
	SDblQueLink iLink;

public:
	/**
	Return the SPageInfo for a given page of physical RAM.
	*/
	static SPageInfo* FromPhysAddr(TPhysAddr aAddress);

	/**
	Return physical address of the RAM page which this SPageInfo object is associated.
	If the address has no SPageInfo, then a null pointer is returned.
	*/
	static SPageInfo* SafeFromPhysAddr(TPhysAddr aAddress);

	/**
	Return physical address of the RAM page which this SPageInfo object is associated.
	*/
	FORCE_INLINE TPhysAddr PhysAddr();

	/**
	Return a SPageInfo by conversion from the address of its embedded link member #iLink.
	*/
	FORCE_INLINE static SPageInfo* FromLink(SDblQueLink* aLink)
		{
		return _LOFF(aLink, SPageInfo, iLink);
		}

	//
	// Getters...
	//

	/**
	Return the current #TType value stored in #iType.
	@pre #MmuLock held.
	*/
	FORCE_INLINE TType Type()
		{
		CheckAccess("Type");
		return (TType)iType;
		}

	/**
	Return the current value of #iFlags.
	@pre #MmuLock held (if \a aNoCheck false).
	*/
	FORCE_INLINE TUint Flags(TBool aNoCheck=false)
		{
		if(!aNoCheck)
			CheckAccess("Flags");
		return iFlags;
		}

	/**
	Return the current value of #iPagedState.
	@pre #MmuLock held.
	*/
	FORCE_INLINE TPagedState PagedState()
		{
		CheckAccess("PagedState");
		return (TPagedState)iPagedState;
		}

	/**
	Return the current value of #iOwner.
	@pre #MmuLock held.
	*/
	FORCE_INLINE DMemoryObject* Owner()
		{
		CheckAccess("Owner");
		return iOwner;
		}

	/**
	Return the current value of #iIndex.
	@pre #MmuLock held (if \a aNoCheck false).
	*/
	FORCE_INLINE TUint32 Index(TBool aNoCheck=false)
		{
		if(!aNoCheck)
			CheckAccess("Index");
		return iIndex;
		}

	/**
	Return the current value of #iModifier.
	@pre #MmuLock held (if \a aNoCheck false).
	*/
	FORCE_INLINE TAny* Modifier()
		{
		CheckAccess("Modifier");
		return iModifier;
		}


	//
	// Setters..
	//

	/**
	Set this page as type #EFixed.
	This is only used during boot by Mmu::Init2Common.
	*/
	inline void SetFixed(TUint32 aIndex=0)
		{
		CheckAccess("SetFixed");
		Set(EFixed,0,aIndex);
		}

	/**
	Set this page as type #EUnused.

	@pre #MmuLock held.
	@pre #RamAllocLock held if previous page type != #EUnknown.

	@post #iModifier==0 to indicate that page usage has changed.
	*/
	inline void SetUnused()
		{
		CheckAccess("SetUnused",ECheckNotUnused|((iType!=EUnknown)?(TInt)ECheckRamAllocLock:0));
		iType = EUnused;
		iModifier = 0;
		// do not modify iFlags or iIndex in this function because page allocating cache cleaning operations rely on using this value
		}

	/**
	Set this page as type #EUnknown.
	This is only used by Mmu::PagesAllocated.

	@pre #RamAllocLock held.

	@post #iModifier==0 to indicate that page usage has changed.
	*/
	inline void SetAllocated()
		{
		CheckAccess("SetAllocated",ECheckUnused|ECheckRamAllocLock|ENoCheckMmuLock);
		iType = EUnknown;
		iModifier = 0;
		// do not modify iFlags or iIndex in this function because cache cleaning operations rely on using this value
		}

	/**
	Set this page as type #EPhysAlloc.
	@param aOwner	 Optional value for #iOwner.
	@param aIndex	 Optional value for #iIndex.

	@pre #MmuLock held.

	@post #iModifier==0 to indicate that page usage has changed.
	*/
	inline void SetPhysAlloc(DMemoryObject* aOwner=0, TUint32 aIndex=0)
		{
		CheckAccess("SetPhysAlloc");
		Set(EPhysAlloc,aOwner,aIndex);
		iUseCount = 0;
		}

	/**
	Set this page as type #EManaged.

	@param aOwner	Value for #iOwner.
	@param aIndex 	Value for #iIndex.
	@param aFlags 	Value for #iFlags (aOwner->PageInfoFlags()).

	@pre #MmuLock held.

	@post #iModifier==0 to indicate that page usage has changed.
	*/
	inline void SetManaged(DMemoryObject* aOwner, TUint32 aIndex, TUint8 aFlags)
		{
		CheckAccess("SetManaged");
		Set(EManaged,aOwner,aIndex);
		iFlags = aFlags;
		iPinCount = 0;
		}

	/**
	Set this page as type #EShadow.

	This is for use by #DShadowPage.

	@param aIndex 	Value for #iIndex.
	@param aFlags 	Value for #iFlags.

	@pre #MmuLock held.

	@post #iModifier==0 to indicate that page usage has changed.
	*/
	inline void SetShadow(TUint32 aIndex, TUint8 aFlags)
		{
		CheckAccess("SetShadow");
		Set(EShadow,0,aIndex);
		iFlags = aFlags;
		}

	/**
	Store a pointer to the SPageInfo of the page that this page is shadowing.

	@param aOrigPageInfo	Pointer to the SPageInfo that this page is shadowing

	@pre #MmuLock held.
	*/
	inline void SetOriginalPage(SPageInfo* aOrigPageInfo)
		{
		CheckAccess("SetOriginalPage");
		__NK_ASSERT_DEBUG(iType == EShadow);
		__NK_ASSERT_DEBUG(!iOriginalPageInfo);
		iOriginalPageInfo = aOrigPageInfo;
		}

	/**
	Returns a pointer to the SPageInfo of the page that this page is shadowing.

	@return	A pointer to the SPageInfo that this page is shadowing

	@pre #MmuLock held.
	*/
	inline SPageInfo* GetOriginalPage()
		{
		CheckAccess("GetOriginalPage");
		__NK_ASSERT_DEBUG(iType == EShadow);
		__NK_ASSERT_DEBUG(iOriginalPageInfo);
		return iOriginalPageInfo;
		}


private:
	/** Internal implementation factor for methods which set page type. */
	FORCE_INLINE void Set(TType aType, DMemoryObject* aOwner, TUint32 aIndex)
		{
		CheckAccess("Set",ECheckNotAllocated|ECheckNotPaged);
		(TUint32&)iType = aType; // also clears iFlags, iFlags2 and iPagedState
		iOwner = aOwner;
		iIndex = aIndex;
		iModifier = 0;
		}

public:


	//
	//
	//

	/**
	Set #iFlags to indicate that the contents of this page have been removed from
	any caches.

	@pre #MmuLock held if #iType!=#EUnused, #RamAllocLock held if #iType==#EUnused.
	*/
	FORCE_INLINE void SetUncached()
		{
		CheckAccess("SetUncached",iType==EUnused ? ECheckRamAllocLock|ENoCheckMmuLock : 0);
		__NK_ASSERT_DEBUG(iType==EUnused || (iType==EPhysAlloc && iUseCount==0));
		iFlags = EMemAttNormalUncached;
		}

	/**
	Set memory attributes and colour for a page of type #EPhysAlloc.
	
	This is set the first time a page of type #EPhysAlloc is added to a memory
	object with DMemoryManager::AddPages or DMemoryManager::AddContiguous.
	The set values are used to check constraints are met if the page is
	also added to other memory objects.

	@param aIndex	The page index within a memory object at which this page
					has been added. This is stored in #iIndex and used to determine
					the page's 'colour'.
	@param aFlags 	Value for #iFlags. This sets the memory attributes for the page.

	@post #iModifier==0 to indicate that page usage has changed.
	*/
	inline void SetMapped(TUint32 aIndex, TUint aFlags)
		{
		CheckAccess("SetMapped");
		__NK_ASSERT_DEBUG(iType==EPhysAlloc);
		__NK_ASSERT_DEBUG(iUseCount==0); // check page not already added to an object
		iIndex = aIndex;
		iFlags = aFlags;
		iModifier = 0;
		}

	/**
	Set #iPagedState

	@pre #MmuLock held.

	@post #iModifier==0 to indicate that page state has changed.
	*/
	FORCE_INLINE void SetPagedState(TPagedState aPagedState)
		{
		CheckAccess("SetPagedState");
		__NK_ASSERT_DEBUG(aPagedState==iPagedState || iPagedState!=EPagedPinned || iPinCount==0); // make sure don't set an unpinned state if iPinCount!=0
		iPagedState = aPagedState;
		iModifier = 0;
		}

	/**
	Mark this page as an oldest old page.

	This does not mark the object as modified as conceptually it's still an oldest page.  This means
	that if a page goes from young -> old -> oldest the second transition will not interrupt the
	page restriction that happens on the first.

	@pre #MmuLock held.
	*/
	FORCE_INLINE void SetOldestPage(TPagedState aPagedState)
		{
		CheckAccess("SetPagedState");
		__NK_ASSERT_DEBUG(iPagedState==EPagedOld);
		__NK_ASSERT_DEBUG(aPagedState==EPagedOldestClean || aPagedState==EPagedOldestDirty);
		iPagedState = aPagedState;
		}

	/**
	Set the page's #iModifier value.

	#iModifier is cleared to zero whenever the usage or paging state of the page
	changes. So if a thread sets this to a suitable unique value (e.g. the address
	of a local variable) then it may perform a long running operation on the page
	and later check with #CheckModified that no other thread has changed the page
	state or used SetModifier in the intervening time.
	Example.

	@code
	TInt anyLocalVariable; // arbitrary local variable

	MmuLock::Lock();
	SPageInfo* thePageInfo = GetAPage();
	thePageInfo->SetModifier(&anyLocalVariable); // use &anyLocalVariable as value unique to this thread 
	MmuLock::Unlock();

	DoOperation(thePageInfo);

	MmuLock::Lock();
	TInt r;
	if(!thePageInfo->CheckModified(&anyLocalVariable));
		{
		// nobody else touched the page...
		OperationSucceeded(thePageInfo);
		r = KErrNone;
		}
	else
		{
		// somebody else changed our page...
		OperationInterrupted(thePageInfo);
		r = KErrAbort;
		}
	MmuLock::Unlock();

	return r;
	@endcode

	@pre #MmuLock held.
	*/
	FORCE_INLINE void SetModifier(TAny* aModifier)
		{
		CheckAccess("SetModifier");
		iModifier = aModifier;
		}

	/**
	Return true if the #iModifier value does not match a specified value.

	@param aModifier	A 'modifier' value previously set with #SetModifier.

	@pre #MmuLock held.

	@see SetModifier.
	*/
	FORCE_INLINE TBool CheckModified(TAny* aModifier)
		{
		CheckAccess("CheckModified");
		return iModifier!=aModifier;
		}

	/**
	Flag this page as having Page Table Entries which give writeable access permissions.
	This sets flags #EWritable and #EDirty.

	@pre #MmuLock held.
	*/
	FORCE_INLINE void SetWritable()
		{
		CheckAccess("SetWritable");
		// This should only be invoked on paged pages.
		__NK_ASSERT_DEBUG(PagedState() != EUnpaged);
		iFlags |= EWritable;
		SetDirty();
		}

	/**
	Flag this page as having no longer having any Page Table Entries which give writeable
	access permissions.
	This clears the flag #EWritable.

	@pre #MmuLock held.
	*/
	FORCE_INLINE void SetReadOnly()
		{
		CheckAccess("SetReadOnly");
		iFlags &= ~EWritable;
		}

	/**
	Returns true if #SetWritable has been called without a subsequent #SetReadOnly.
	This returns the flag #EWritable.

	@pre #MmuLock held.
	*/
	FORCE_INLINE TBool IsWritable()
		{
		CheckAccess("IsWritable");
		return iFlags&EWritable;
		}

	/**
	Flag this page as 'dirty', indicating that its contents may no longer match those saved
	to a backing store. This sets the flag #EDirty.

	This is used in the management of demand paged memory.

	@pre #MmuLock held.
	*/
	FORCE_INLINE void SetDirty()
		{
		CheckAccess("SetDirty");
		__NK_ASSERT_DEBUG(IsWritable());
		iFlags |= EDirty;
		}

	/**
	Flag this page as 'clean', indicating that its contents now match those saved
	to a backing store. This clears the flag #EDirty.

	This is used in the management of demand paged memory.

	@pre #MmuLock held.
	*/
	FORCE_INLINE void SetClean()
		{
		CheckAccess("SetClean");
		__NK_ASSERT_DEBUG(!IsWritable());
		iFlags &= ~EDirty;
		}

	/**
	Return the  #EDirty flag. See #SetDirty and #SetClean.

	This is used in the management of demand paged memory.

	@pre #MmuLock held.
	*/
	FORCE_INLINE TBool IsDirty()
		{
		CheckAccess("IsDirty");
		return iFlags&EDirty;
		}


	//
	// Type specific...
	//

	/**
	Set #iCacheInvalidateCounter to the specified value.

	@pre #MmuLock held.
	@pre #iType==#EUnused.
	*/
	void SetCacheInvalidateCounter(TUint32 aCacheInvalidateCounter)
		{
		CheckAccess("SetCacheInvalidateCounter");
		__NK_ASSERT_DEBUG(iType==EUnused);
		iCacheInvalidateCounter = aCacheInvalidateCounter;
		}

	/**
	Return #iCacheInvalidateCounter.

	@pre #MmuLock held.
	@pre #iType==#EUnused.
	*/
	TUint32 CacheInvalidateCounter()
		{
		CheckAccess("CacheInvalidateCounter",ECheckRamAllocLock|ENoCheckMmuLock);
		__NK_ASSERT_DEBUG(iType==EUnused);
		return iCacheInvalidateCounter;
		}

	/**
	Increment #iUseCount to indicate that the page has been added to a memory object.

	@return New value of #iUseCount.

	@pre #MmuLock held.
	@pre #iType==#EPhysAlloc.
	*/
	TUint32 IncUseCount()
		{
		CheckAccess("IncUseCount");
		__NK_ASSERT_DEBUG(iType==EPhysAlloc);
		return ++iUseCount;
		}

	/**
	Decrement #iUseCount to indicate that the page has been removed from a memory object.

	@return New value of #iUseCount.

	@pre #MmuLock held.
	@pre #iType==#EPhysAlloc.
	*/
	TUint32 DecUseCount()
		{
		CheckAccess("DecUseCount");
		__NK_ASSERT_DEBUG(iType==EPhysAlloc);
		__NK_ASSERT_DEBUG(iUseCount);
		return --iUseCount;
		}

	/**
	Return #iUseCount, this indicates the number of times the page has been added to memory object(s).

	@return #iUseCount.

	@pre #MmuLock held.
	@pre #iType==#EPhysAlloc.
	*/
	TUint32 UseCount()
		{
		CheckAccess("UseCount");
		__NK_ASSERT_DEBUG(iType==EPhysAlloc);
		return iUseCount;
		}

	/**
	Increment #iPinCount to indicate that a mapping has pinned this page.
	This is only done for demand paged memory; unpaged memory does not have
	#iPinCount updated when it is pinned.

	@return New value of #iPinCount.

	@pre #MmuLock held.
	@pre #iType==#EManaged.
	*/
	TUint32 IncPinCount()
		{
		CheckAccess("IncPinCount");
		__NK_ASSERT_DEBUG(iType==EManaged);
		return ++iPinCount;
		}

	/**
	Decrement #iPinCount to indicate that a mapping which was pinning this page has been removed.
	This is only done for demand paged memory; unpaged memory does not have
	#iPinCount updated when it is unpinned.

	@return New value of #iPinCount.

	@pre #MmuLock held.
	@pre #iType==#EManaged.
	*/
	TUint32 DecPinCount()
		{
		CheckAccess("DecPinCount");
		__NK_ASSERT_DEBUG(iType==EManaged);
		__NK_ASSERT_DEBUG(iPinCount);
		return --iPinCount;
		}

	/**
	Clear #iPinCount to zero as this page is no longer being used for the 
	pinned page.
	This is only done for demand paged memory; unpaged memory does not have
	#iPinCount set.

	@pre #MmuLock held.
	@pre #iType==#EManaged.
	*/
	void ClearPinCount()
		{
		CheckAccess("ClearPinCount");
		__NK_ASSERT_DEBUG(iType==EManaged);
		__NK_ASSERT_DEBUG(iPinCount);
		iPinCount = 0;
		}

	/**
	Return #iPinCount which indicates the number of mappings that have pinned this page.
	This is only valid for demand paged memory; unpaged memory does not have
	#iPinCount updated when it is pinned.

	@return #iPinCount.

	@pre #MmuLock held.
	@pre #iType==#EManaged.
	*/
	TUint32 PinCount()
		{
		CheckAccess("PinCount");
		__NK_ASSERT_DEBUG(iType==EManaged);
		return iPinCount;
		}

	/**
	Set the #EPinnedReserve flag.
	@pre #MmuLock held.
	@see EPinnedReserve.
	*/
	void SetPinnedReserve()
		{
		CheckAccess("SetPinnedReserve");
		iFlags2 |= EPinnedReserve;
		}

	/**
	Clear the #EPinnedReserve flag.
	@pre #MmuLock held.
	@see EPinnedReserve.
	*/
	TBool ClearPinnedReserve()
		{
		CheckAccess("ClearPinnedReserve");
		TUint oldFlags2 = iFlags2;
		iFlags2 = oldFlags2&~EPinnedReserve;
		return oldFlags2&EPinnedReserve;
		}

	/**
	Set #iPagingManagerData to the specified value.
	@pre #MmuLock held.
	@pre #iType==#EManaged.
	*/
	void SetPagingManagerData(TUint32 aPagingManagerData)
		{
		CheckAccess("SetPagingManagerData");
		__NK_ASSERT_DEBUG(iType==EManaged);
		iPagingManagerData = aPagingManagerData;
		}

	/**
	Return #iPagingManagerData.
	@pre #MmuLock held.
	@pre #iType==#EManaged.
	*/
	TUint32 PagingManagerData()
		{
		CheckAccess("PagingManagerData");
		__NK_ASSERT_DEBUG(iType==EManaged);
		return iPagingManagerData;
		}

	//
	// Debug...
	//

private:
	enum TChecks
		{
		ECheckNotAllocated	= 1<<0,
		ECheckNotUnused		= 1<<1,
		ECheckUnused		= 1<<2,
		ECheckNotPaged		= 1<<3,
		ECheckRamAllocLock	= 1<<4,
		ENoCheckMmuLock		= 1<<5
		};
#ifdef _DEBUG
	void CheckAccess(const char* aMessage, TUint aFlags=0);
#else
	FORCE_INLINE void CheckAccess(const char* /*aMessage*/, TUint /*aFlags*/=0)
		{}
#endif

public:
#ifdef _DEBUG
	/**
	Debug function which outputs the contents of this object to the kernel debug port.
	*/
	void Dump();
#else
	FORCE_INLINE void Dump()
		{}
#endif
	};


const TInt KPageInfosPerPageShift = KPageShift-KPageInfoShift;
const TInt KPageInfosPerPage = 1<<KPageInfosPerPageShift;
const TInt KNumPageInfoPagesShift = 32-KPageShift-KPageInfosPerPageShift;
const TInt KNumPageInfoPages = 1<<KNumPageInfoPagesShift;

FORCE_INLINE SPageInfo* SPageInfo::FromPhysAddr(TPhysAddr aAddress)
	{
	return ((SPageInfo*)KPageInfoLinearBase)+(aAddress>>KPageShift);
	}

FORCE_INLINE TPhysAddr SPageInfo::PhysAddr()
	{
	return ((TPhysAddr)this)<<KPageInfosPerPageShift;
	}



/**
A page table information structure giving the current use and state for a
page table.
*/
struct SPageTableInfo
	{
public:

	/**
	Enumeration for the usage of a page table. This is stored in #iType.
	*/
	enum TType
		{
		/**
		Page table is unused (implementation assumes this enumeration == 0).
		@see #iUnused and #SPageTableInfo::TUnused.
		*/
		EUnused=0,

		/**
		Page table has undetermined use.
		(Either created by the bootstrap or is newly allocated but not yet assigned.)
		*/
		EUnknown=1,

		/**
		Page table is being used by a coarse memory object.
		@see #iCoarse and #SPageTableInfo::TCoarse.
		*/
		ECoarseMapping=2,

		/**
		Page table is being used for fine mappings.
		@see #iFine and #SPageTableInfo::TFine.
		*/
		EFineMapping=3
		};

private:

	/**
	Flags stored in #iFlags.
	*/
	enum TFlags
		{
		/**
		Page table if for mapping demand paged content.
		*/
		EDemandPaged		= 	1<<0,
		/**
		Page table is in Page Table Allocator's cleanup list
		(only set for first page table in a RAM page)
		*/
		EOnCleanupList		= 	1<<1,
		/**
		The page table cluster that this page table info refers to is currently allocated.
		*/
		EPtClusterAllocated 	=	1<<2
		};

	/**
	Value from enum #TType.
	*/
	TUint8 iType;				

	/**
	Bitmask of values from #TFlags.
	*/
	TUint8 iFlags;

	/**
	Spare member used for padding.
	*/
	TUint16 iSpare2;

	/**
	Number of pages currently mapped by this page table.
	Normally, when #iPageCount==0 and #iPermanenceCount==0, the page table is freed.
	*/
	TUint16 iPageCount;

	/**
	Count for the number of uses of this page table which require it to be permanently allocated;
	even when it maps no pages (#iPageCount==0).
	*/
	TUint16 iPermanenceCount;

	/**
	Information about a page table when #iType==#EUnused.
	*/
	struct TUnused
		{
		/**
		Cast this object to a SDblQueLink reference.
		This is used for placing unused SPageTableInfo objects into free lists.
		*/
		FORCE_INLINE SDblQueLink& Link()
			{ return *(SDblQueLink*)this; }
	private:
		SDblQueLink* iNext;	///< Next free page table
		SDblQueLink* iPrev;	///< Previous free page table
		};

	/**
	Information about a page table when #iType==#ECoarseMapping.
	*/
	struct TCoarse
		{
		/**
		Memory object which owns this page table.
		*/
		DCoarseMemory*	iMemoryObject;

		/**
		The index of the page table, i.e. the offset, in 'chunks',
		into the object's memory that the page table is being used to map.
		*/
		TUint16			iChunkIndex;

		/**
		The #TPteType the page table is being used for.
		*/
		TUint8			iPteType;
		};

	/**
	Information about a page table when #iType==#EFineMapping.
	*/
	struct TFine
		{
		/**
		Start of the virtual address region that this page table is currently
		mapping memory at, ORed with the OS ASID of the address space this lies in.
		*/
		TLinAddr		iLinAddrAndOsAsid;
		};

	/**
	Union of type specific info.
	*/
	union
		{
		TUnused	iUnused; ///< Information about a page table when #iType==#EUnused.
		TCoarse	iCoarse; ///< Information about a page table when #iType==#ECoarseMapping.
		TFine	iFine;   ///< Information about a page table when #iType==#EFineMapping.
		};

public:
	/**
	Return the SPageTableInfo for the page table in which a given PTE lies.
	*/
	static SPageTableInfo* FromPtPtr(TPte* aPtPte);

	/**
	Return the page table with which this SPageTableInfo is associated.
	*/
	TPte* PageTable();

	/**
	Used at boot time to initialise page tables which were allocated by the bootstrap. 

	@param aCount	The number of pages being mapped by this page table.
	*/
	FORCE_INLINE void Boot(TUint aCount)
		{
		CheckInit("Boot");
		iPageCount = aCount;
		iPermanenceCount = 1; // assume page table shouldn't be freed
		iType = EUnknown;
		iFlags = EPtClusterAllocated;
		}

	/**
	Initialise a page table after it has had memory allocated for it.

	@param aDemandPaged	True if this page table has been allocated for use with
						demand paged memory.
	*/
	FORCE_INLINE void New(TBool aDemandPaged)
		{
		iType = EUnused;
		iFlags = EPtClusterAllocated | (aDemandPaged ? EDemandPaged : 0);
		}

	/**
	Return true if the page table cluster that this page table info refers to has
	been previously allocated.
	*/
	FORCE_INLINE TBool IsPtClusterAllocated()
		{
		return iFlags & EPtClusterAllocated;
		}

	/**
	The page table cluster that this page table info refers to has been freed.
	*/
	FORCE_INLINE void PtClusterFreed()
		{
		__NK_ASSERT_DEBUG(IsPtClusterAllocated());
		iFlags &= ~EPtClusterAllocated;
		}

	/**
	The page table cluster that this page table info refers to has been allocated.
	*/
	FORCE_INLINE void PtClusterAlloc()
		{
		__NK_ASSERT_DEBUG(!IsPtClusterAllocated());
		iFlags |= EPtClusterAllocated;
		}

	/**
	Initialilse a page table to type #EUnknown after it has been newly allocated.

	@pre #PageTablesLockIsHeld.
	*/
	FORCE_INLINE void Init()
		{
		__NK_ASSERT_DEBUG(IsPtClusterAllocated());
		CheckInit("Init");
		iPageCount = 0;
		iPermanenceCount = 0;
		iType = EUnknown;
		}

	/**
	Increment #iPageCount to account for newly mapped pages.

	@param aStep	Amount to add to #iPageCount. Default is one.

	@return New value of #iPageCount.

	@pre #MmuLock held.
	*/
	FORCE_INLINE TUint IncPageCount(TUint aStep=1)
		{
		CheckAccess("IncPageCount");
		TUint count = iPageCount; // compiler handles half-word values stupidly, so give it a hand
		count += aStep;
		iPageCount = count;
		return count;
		}

	/**
	Decrement #iPageCount to account for removed pages.

	@param aStep	Amount to subtract from #iPageCount. Default is one.

	@return New value of #iPageCount.

	@pre #MmuLock held.
	*/
	FORCE_INLINE TUint DecPageCount(TUint aStep=1)
		{
		CheckAccess("DecPageCount");
		TUint count = iPageCount; // compiler handles half-word values stupidly, so give it a hand
		count -= aStep;
		iPageCount = count;
		return count;
		}

	/**
	Return #iPageCount.
	@pre #MmuLock held.
	*/
	FORCE_INLINE TUint PageCount()
		{
		CheckAccess("PageCount");
		return iPageCount;
		}

	/**
	Increment #iPermanenceCount to indicate a new use of this page table which
	requires it to be permanently allocated.

	@return New value of #iPermanenceCount.

	@pre #MmuLock held.
	*/
	FORCE_INLINE TUint IncPermanenceCount()
		{
		CheckAccess("IncPermanenceCount");
		TUint count = iPermanenceCount; // compiler handles half-word values stupidly, so give it a hand
		++count;
		iPermanenceCount = count;
		return count;
		}

	/**
	Decrement #iPermanenceCount to indicate the removal of a use added by #IncPermanenceCount.

	@return New value of #iPermanenceCount.

	@pre #MmuLock held.
	*/
	FORCE_INLINE TUint DecPermanenceCount()
		{
		CheckAccess("DecPermanenceCount");
		TUint count = iPermanenceCount; // compiler handles half-word values stupidly, so give it a hand
		__NK_ASSERT_DEBUG(count);
		--count;
		iPermanenceCount = count;
		return count;
		}

	/**
	Return #iPermanenceCount.

	@pre #MmuLock held.
	*/
	FORCE_INLINE TUint PermanenceCount()
		{
		CheckAccess("PermanenceCount");
		return iPermanenceCount;
		}

	/**
	Set page table to the #EUnused state.
	This is only intended for use by #PageTableAllocator.

	@pre #MmuLock held and #PageTablesLockIsHeld.
	*/
	FORCE_INLINE void SetUnused()
		{
		CheckChangeUse("SetUnused");
		iType = EUnused;
		}

	/**
	Return true if the page table is in the #EUnused state.
	This is only intended for use by #PageTableAllocator.

	@pre #MmuLock held or #PageTablesLockIsHeld.
	*/
	FORCE_INLINE TBool IsUnused()
		{
		CheckCheckUse("IsUnused");
		return iType==EUnused;
		}

	/**
	Set page table as being used by a coarse memory object.

	@param aMemory		Memory object which owns this page table.
	@param aChunkIndex	The index of the page table, i.e. the offset, in 'chunks',
						into the object's memory that the page table is being used to map.
	@param aPteType		The #TPteType the page table is being used for.

	@pre #MmuLock held and #PageTablesLockIsHeld.

	@see TCoarse.
	*/
	inline void SetCoarse(DCoarseMemory* aMemory, TUint aChunkIndex, TUint aPteType)
		{
		CheckChangeUse("SetCoarse");
		iPageCount = 0;
		iPermanenceCount = 0;
		iType = ECoarseMapping;
		iCoarse.iMemoryObject = aMemory;
		iCoarse.iChunkIndex = aChunkIndex;
		iCoarse.iPteType = aPteType;
		}

	/**
	Return true if this page table is currently being used by a coarse memory object
	matching the specified arguments.
	For arguments, see #SetCoarse.

	@pre #MmuLock held or #PageTablesLockIsHeld.
	*/
	inline TBool CheckCoarse(DCoarseMemory* aMemory, TUint aChunkIndex, TUint aPteType)
		{
		CheckCheckUse("CheckCoarse");
		return iType==ECoarseMapping
			&& iCoarse.iMemoryObject==aMemory
			&& iCoarse.iChunkIndex==aChunkIndex
			&& iCoarse.iPteType==aPteType;
		}

	/**
	Set page table as being used for fine mappings.

	@param aLinAddr	Start of the virtual address region that the page table is
					mapping memory at.
	@param aOsAsid	The OS ASID of the address space which \a aLinAddr lies in.

	@pre #MmuLock held and #PageTablesLockIsHeld.
	*/
	inline void SetFine(TLinAddr aLinAddr, TUint aOsAsid)
		{
		CheckChangeUse("SetFine");
		__NK_ASSERT_DEBUG((aLinAddr&KPageMask)==0);
		iPageCount = 0;
		iPermanenceCount = 0;
		iType = EFineMapping;
		iFine.iLinAddrAndOsAsid = aLinAddr|aOsAsid;
		}

	/**
	Return true if this page table is currently being used for fine mappings
	matching the specified arguments.
	For arguments, see #SetFine.

	@pre #MmuLock held or #PageTablesLockIsHeld.
	*/
	inline TBool CheckFine(TLinAddr aLinAddr, TUint aOsAsid)
		{
		CheckCheckUse("CheckFine");
		__NK_ASSERT_DEBUG((aLinAddr&KPageMask)==0);
		return iType==EFineMapping
			&& iFine.iLinAddrAndOsAsid==(aLinAddr|aOsAsid);
		}

	/**
	Set a previously unknown page table as now being used for fine mappings.
	This is used during the boot process by DFineMemory::ClaimInitialPages
	to initialise the state of a page table allocated by the bootstrap.

	@param aLinAddr	Start of the virtual address region that the page table is
					mapping memory at.
	@param aOsAsid	The OS ASID of the address space which \a aLinAddr lies in.
					(This should be KKernelOsAsid.)

	@pre #MmuLock held and #PageTablesLockIsHeld.
	*/
	inline TBool ClaimFine(TLinAddr aLinAddr, TUint aOsAsid)
		{
		CheckChangeUse("ClaimFine");
		__NK_ASSERT_DEBUG((aLinAddr&KPageMask)==0);
		if(iType==EFineMapping)
			return CheckFine(aLinAddr,aOsAsid);
		if(iType!=EUnknown)
			return false;
		iType = EFineMapping;
		iFine.iLinAddrAndOsAsid = aLinAddr|aOsAsid;
		return true;
		}

	/**
	Return true if page table was allocated for use with demand paged memory.
	*/
	FORCE_INLINE TBool IsDemandPaged()
		{
		return iFlags&EDemandPaged;
		}

#ifdef _DEBUG
	/**
	Debug check returning true if the value of #iPageCount is consistent with
	the PTEs in this page table.

	@pre #MmuLock held.
	*/
	TBool CheckPageCount();
#endif

	/**
	Return a reference to an embedded SDblQueLink which is used for placing this
	SPageTableInfo objects into free lists.
	@pre #PageTablesLockIsHeld.
	@pre #iType==#EUnused.
	*/
	inline SDblQueLink& FreeLink()
		{
		__NK_ASSERT_DEBUG(IsUnused());
		return iUnused.Link();
		}

	/**
	Return a pointer to a SPageTableInfo by conversion from the address
	of its embedded link as returned by #FreeLink.
	*/
	FORCE_INLINE static SPageTableInfo* FromFreeLink(SDblQueLink* aLink)
		{
		return _LOFF(aLink, SPageTableInfo, iUnused);
		}

	/**
	Return the SPageTableInfo for the first page table in the same
	physical ram page as the page table for this SPageTableInfo.
	*/
	FORCE_INLINE SPageTableInfo* FirstInPage()
		{
		return (SPageTableInfo*)(TLinAddr(this)&~(KPtClusterMask*sizeof(SPageTableInfo)));
		}

	/**
	Return the SPageTableInfo for the last page table in the same
	physical ram page as the page table for this SPageTableInfo.
	*/
	FORCE_INLINE SPageTableInfo* LastInPage()
		{
		return (SPageTableInfo*)(TLinAddr(this)|(KPtClusterMask*sizeof(SPageTableInfo)));
		}

	/**
	Return true if the page table for this SPageTableInfo is
	the first page table in the physical page it occupies.
	*/
	FORCE_INLINE TBool IsFirstInPage()
		{
		return (TLinAddr(this)&(KPtClusterMask*sizeof(SPageTableInfo)))==0;
		}

	/**
	Return true if this page table has been added to the cleanup list with
	#AddToCleanupList.
	Must only be used for page tables which return true for #IsFirstInPage.

	@pre #PageTablesLockIsHeld.
	*/
	FORCE_INLINE TBool IsOnCleanupList()
		{
		__NK_ASSERT_DEBUG(IsFirstInPage());
		return iFlags&EOnCleanupList;
		}

	/**
	Add the RAM page containing this page table to the specified cleanup list.
	Must only be used for page tables which return true for #IsFirstInPage.

	@pre #PageTablesLockIsHeld.
	*/
	FORCE_INLINE void AddToCleanupList(SDblQue& aCleanupList)
		{
		__NK_ASSERT_DEBUG(IsUnused());
		__NK_ASSERT_DEBUG(IsFirstInPage());
		__NK_ASSERT_DEBUG(!IsOnCleanupList());
		aCleanupList.Add(&FreeLink());
		iFlags |= EOnCleanupList;
		}

	/**
	Remove the RAM page containing this page table from a cleanup list it
	was added to with aCleanupList.
	Must only be used for page tables which return true for #IsFirstInPage.

	@pre #PageTablesLockIsHeld.
	*/
	FORCE_INLINE void RemoveFromCleanupList()
		{
		__NK_ASSERT_DEBUG(IsUnused());
		__NK_ASSERT_DEBUG(IsFirstInPage());
		__NK_ASSERT_DEBUG(IsOnCleanupList());
		iFlags &= ~EOnCleanupList;
		FreeLink().Deque();
		}

	/**
	Remove this page table from its owner and free it.
	This is only used with page tables which map demand paged memory
	and is intended for use in implementing #DPageTableMemoryManager.

	@return KErrNone if successful,
			otherwise one of the system wide error codes.

	@pre #MmuLock held and #PageTablesLockIsHeld.
	*/
	TInt ForcedFree();

private:

#ifdef _DEBUG
	void CheckChangeUse(const char* aName);
	void CheckCheckUse(const char* aName);
	void CheckAccess(const char* aName);
	void CheckInit(const char* aName);
#else
	FORCE_INLINE void CheckChangeUse(const char* /*aName*/)
		{}
	FORCE_INLINE void CheckCheckUse(const char* /*aName*/)
		{}
	FORCE_INLINE void CheckAccess(const char* /*aName*/)
		{}
	FORCE_INLINE void CheckInit(const char* /*aName*/)
		{}
#endif
	};


const TInt KPageTableInfoShift = 4;
__ASSERT_COMPILE(sizeof(SPageTableInfo)==(1<<KPageTableInfoShift));

FORCE_INLINE SPageTableInfo* SPageTableInfo::FromPtPtr(TPte* aPtPte)
	{
	TUint id = ((TLinAddr)aPtPte-KPageTableBase)>>KPageTableShift;
	return (SPageTableInfo*)KPageTableInfoBase+id;
	}

FORCE_INLINE TPte* SPageTableInfo::PageTable()
	{
	return (TPte*)
		(KPageTableBase+
			(
			((TLinAddr)this-(TLinAddr)KPageTableInfoBase)
			<<(KPageTableShift-KPageTableInfoShift)
			)
		);
	}



/**
Class providing access to the mutex used to protect memory allocation operations;
this is the mutex Mmu::iRamAllocatorMutex.
In addition to providing locking, these functions monitor the system's free RAM
levels and call K::CheckFreeMemoryLevel to notify the system of changes.
*/
class RamAllocLock
	{
public:
	/**
	Acquire the lock.
	The lock may be acquired multiple times by a thread, and will remain locked
	until #Unlock has been used enough times to balance this.
	*/
	static void Lock();

	/**
	Release the lock.

	@pre The current thread has previously acquired the lock.
	*/
	static void Unlock();

	/**
	Allow another thread to acquire the lock.
	This is equivalent to #Unlock followed by #Lock, but optimised
	to only do this if there is another thread waiting on the lock.

	@return True if the lock was released by this function.

	@pre The current thread has previously acquired the lock.
	*/
	static TBool Flash();

	/**
	Return true if the current thread holds the lock.
	This is used for debug checks.
	*/
	static TBool IsHeld();
	};



/**
Return true if the PageTableLock is held by the current thread.
This lock is the mutex used to protect page table allocation; it is acquired
with
@code
	::PageTables.Lock();
@endcode
and released with
@code
	::PageTables.Unlock();
@endcode
*/
TBool PageTablesLockIsHeld();



/**
Class providing access to the fast mutex used to protect various
low level memory operations.

This lock must only be held for a very short and bounded time.
*/
class MmuLock
	{
public:
	/**
	Acquire the lock.
	*/
	static void Lock();

	/**
	Release the lock.

	@pre The current thread has previously acquired the lock.
	*/
	static void Unlock();

	/**
	Allow another thread to acquire the lock.
	This is equivalent to #Unlock followed by #Lock, but optimised
	to only do this if there is another thread waiting on the lock.

	@return True if the lock was released by this function.

	@pre The current thread has previously acquired the lock.
	*/
	static TBool Flash();

	/**
	Return true if the current thread holds the lock.
	This is used for debug checks.
	*/
	static TBool IsHeld();

	/**
	Increment a counter and perform the action of #Flash() once a given threshold
	value is reached. After flashing the counter is reset.

	This is typically used in long running loops to periodically flash the lock
	and so avoid holding it for too long, e.g.

	@code
	MmuLock::Lock();
	TUint flash = 0;
	const TUint KMaxInterationsWithLock = 10;
	while(WorkToDo)
		{
		DoSomeWork();
		MmuLock::Flash(flash,KMaxInterationsWithLock); // flash every N loops
		}
	MmuLock::Unlock();
	@endcode

	@param aCounter			Reference to the counter.
	@param aFlashThreshold	Value \a aCounter must reach before flashing the lock.
	@param aStep			Value to add to \a aCounter.

	@return True if the lock was released by this function.

	@pre The current thread has previously acquired the lock.
	*/
	static FORCE_INLINE TBool Flash(TUint& aCounter, TUint aFlashThreshold, TUint aStep=1)
		{
		UnlockGuardCheck();
		if((aCounter+=aStep)<aFlashThreshold)
			return EFalse;
		aCounter -= aFlashThreshold;
		return MmuLock::Flash();
		}

	/**
	Begin a debug check to test that the MmuLock is not unlocked unexpectedly.

	This is used in situations where a series of operation must be performed
	atomically with the MmuLock held. It is usually used via the
	#__UNLOCK_GUARD_START macro, e.g.

	@code
	__UNLOCK_GUARD_START(MmuLock);
	SomeCode();
	SomeMoreCode();
	__UNLOCK_GUARD_END(MmuLock); // fault if MmuLock released by SomeCode or SomeMoreCode
	@endcode
	*/
	static FORCE_INLINE void UnlockGuardStart()
		{
#ifdef _DEBUG
		++UnlockGuardNest;
#endif
		}

	/**
	End a debug check testing that the MmuLock is not unlocked unexpectedly.
	This is usually used via the #__UNLOCK_GUARD_END which faults if true is returned.

	@see UnlockGuardStart

	@return EFalse if the MmuLock was released between a previous #UnlockGuardStart
			and the call this function.
	*/
	static FORCE_INLINE TBool UnlockGuardEnd()
		{
#ifdef _DEBUG
		__NK_ASSERT_DEBUG(UnlockGuardNest);
		--UnlockGuardNest;
		return UnlockGuardFail==0;
#else
		return ETrue;
#endif
		}

private:
	/**
	Exectued whenever the lock is released to check that
	#UnlockGuardStart and #UnlockGuardEnd are balanced.
	*/
	static FORCE_INLINE void UnlockGuardCheck()
		{
#ifdef _DEBUG
		if(UnlockGuardNest)
			UnlockGuardFail = ETrue;
#endif
		}

public:
	/** The lock */
	static NFastMutex iLock;

#ifdef _DEBUG
private:
	static TUint UnlockGuardNest;
	static TUint UnlockGuardFail;
#endif
	};



/**
Interface for accessing the lock mutex being used to serialise
explicit modifications to a specified memory object.

The lock mutex is either the one which was previously assigned with
DMemoryObject::SetLock. Or, if none was set, a dynamically assigned
mutex from #MemoryObjectMutexPool will be of 'order' #KMutexOrdMemoryObject.
*/
class MemoryObjectLock
	{
public:
	/**
	Acquire the lock for the specified memory object.
	If the object has no lock, one is assigned from #MemoryObjectMutexPool.
	*/
	static void Lock(DMemoryObject* aMemory);

	/**
	Release the lock for the specified memory object, which was acquired
	with #Lock. If the lock was one which was dynamically assigned, and there
	are no threads waiting for it, the the lock is unassigned from the memory
	object.
	*/
	static void Unlock(DMemoryObject* aMemory);

	/**
	Return true if the current thread holds lock for the specified memory object.
	This is used for debug checks.
	*/
	static TBool IsHeld(DMemoryObject* aMemory);
	};


#define __UNLOCK_GUARD_START(_l) __DEBUG_ONLY(_l::UnlockGuardStart())
#define __UNLOCK_GUARD_END(_l) __NK_ASSERT_DEBUG(_l::UnlockGuardEnd())


const TUint KMutexOrdAddresSpace = KMutexOrdKernelHeap + 2;
const TUint KMutexOrdMemoryObject = KMutexOrdKernelHeap + 1;
const TUint KMutexOrdMmuAlloc = KMutexOrdRamAlloc + 1;


#ifdef _DEBUG
//#define FORCE_TRACE
//#define FORCE_TRACE2
//#define FORCE_TRACEB
//#define FORCE_TRACEP
#endif



#define TRACE_printf Kern::Printf

#define TRACE_ALWAYS(t) TRACE_printf t

#ifdef FORCE_TRACE
#define TRACE(t) TRACE_printf t
#else
#define TRACE(t) __KTRACE_OPT(KMMU2,TRACE_printf t)
#endif

#ifdef FORCE_TRACE2
#define TRACE2(t) TRACE_printf t
#else
#define TRACE2(t) __KTRACE_OPT(KMMU2,TRACE_printf t)
#endif

#ifdef FORCE_TRACEB
#define TRACEB(t) TRACE_printf t
#else
#define TRACEB(t) __KTRACE_OPT2(KMMU,KBOOT,TRACE_printf t)
#endif

#ifdef FORCE_TRACEP
#define TRACEP(t) TRACE_printf t
#else
#define TRACEP(t) __KTRACE_OPT(KPAGING,TRACE_printf t)
#endif


/**
The maximum number of consecutive updates to #SPageInfo structures which
should be executed without releasing the #MmuLock.

This value must be an integer power of two.
*/
const TUint KMaxPageInfoUpdatesInOneGo = 64;

/**
The maximum number of simple operations on memory page state which should
occur without releasing the #MmuLock. Examples of the operations are
read-modify-write of a Page Table Entry (PTE) or entries in a memory objects
RPageArray.

This value must be an integer power of two.
*/
const TUint KMaxPagesInOneGo = KMaxPageInfoUpdatesInOneGo/2;

/**
The maximum number of Page Directory Entries which should be updated
without releasing the #MmuLock.

This value must be an integer power of two.
*/
const TUint KMaxPdesInOneGo = KMaxPageInfoUpdatesInOneGo;


/********************************************
 * MMU stuff
 ********************************************/

class DRamAllocator;
class TPinArgs;
class Defrag;

/**
Interface to RAM allocation and MMU data structure manipulation.
*/
class Mmu
	{
public:
	enum TPanic
		{
		EInvalidRamBankAtBoot,
		EInvalidReservedBankAtBoot,
		EInvalidPageTableAtBoot,
		EInvalidPdeAtBoot,
		EBadMappedPageAfterBoot,
		ERamAllocMutexCreateFailed,
		EBadFreePhysicalRam,
		EUnsafePageInfoAccess,
		EUnsafePageTableInfoAccess,
		EPhysMemSyncMutexCreateFailed,
		EDefragAllocFailed
		};

	/**
	Attribute flags used when allocating RAM pages.
	See #AllocRam etc.

	The least significant bits of these flags are used for the #TMemoryType
	value for the memory.
	*/
	enum TRamAllocFlags
		{
		// lower bits hold TMemoryType

		/**
		If this flag is set, don't wipe the contents of the memory when allocated.
		By default, for security and confidentiality reasons, the memory is filled
		with a 'wipe' value to erase the previous contents.
		*/
		EAllocNoWipe			= 1<<(KMemoryTypeShift),

		/**
		If this flag is set, any memory wiping will fill memory with the byte
		value starting at bit position #EAllocWipeByteShift in these flags.
		*/
		EAllocUseCustomWipeByte	= 1<<(KMemoryTypeShift+1),

		/**
		If this flag is set, memory allocation won't attempt to reclaim pages
		from the demand paging system.
		This is used to prevent deadlock when the paging system itself attempts
		to allocate memory for itself.
		*/
		EAllocNoPagerReclaim	= 1<<(KMemoryTypeShift+2),

		/**
		@internal
		*/
		EAllocFlagLast,

		/*
		Bit position within these flags, for the least significant bit of the
		byte value used when #EAllocUseCustomWipeByte is set.
		*/
		EAllocWipeByteShift		= 8
		};

public:
	void Init1();
	void Init1Common();
	void Init2();
	void Init2Common();
	void Init2Final();
	void Init2FinalCommon();
	void Init3();

	void BTracePrime(TUint aCategory);

	static void Panic(TPanic aPanic);

	static TInt HandlePageFault(TLinAddr aPc, TLinAddr aFaultAddress, TUint aAccessPermissions, TAny* aExceptionInfo);

	TUint FreeRamInPages();
	TUint TotalPhysicalRamPages();

	TInt AllocRam(	TPhysAddr* aPages, TUint aCount, TRamAllocFlags aFlags, TZonePageType aZonePageType, 
					TUint aBlockZoneId=KRamZoneInvalidId, TBool aBlockRest=EFalse);
	void MarkPageAllocated(TPhysAddr aPhysAddr, TZonePageType aZonePageType);
	void FreeRam(TPhysAddr* aPages, TUint aCount, TZonePageType aZonePageType);
	TInt AllocContiguousRam(TPhysAddr& aPhysAddr, TUint aCount, TUint aAlign, TRamAllocFlags aFlags);
	void FreeContiguousRam(TPhysAddr aPhysAddr, TUint aCount);

	const SRamZone* RamZoneConfig(TRamZoneCallback& aCallback) const;
	void SetRamZoneConfig(const SRamZone* aZones, TRamZoneCallback aCallback);
	TInt ModifyRamZoneFlags(TUint aId, TUint aClearMask, TUint aSetMask);
	TInt GetRamZonePageCount(TUint aId, SRamZonePageCount& aPageData);
	TInt ZoneAllocPhysicalRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aBytes, TPhysAddr& aPhysAddr, TInt aAlign);
	TInt ZoneAllocPhysicalRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aNumPages, TPhysAddr* aPageList);
	TInt RamHalFunction(TInt aFunction, TAny* a1, TAny* a2);	
	void ChangePageType(SPageInfo* aPageInfo, TZonePageType aOldPageType, TZonePageType aNewPageType);
	TInt FreeRamZone(TUint aZoneId);

	TInt AllocPhysicalRam(TPhysAddr* aPages, TUint aCount, TRamAllocFlags aFlags);
	void FreePhysicalRam(TPhysAddr* aPages, TUint aCount);
	TInt AllocPhysicalRam(TPhysAddr& aPhysAddr, TUint aCount, TUint aAlign, TRamAllocFlags aFlags);
	void FreePhysicalRam(TPhysAddr aPhysAddr, TUint aCount);
	TInt ClaimPhysicalRam(TPhysAddr aPhysAddr, TUint aCount, TRamAllocFlags aFlags);
	void AllocatedPhysicalRam(TPhysAddr aPhysAddr, TUint aCount, TRamAllocFlags aFlags);
private:
	void SetAllocPhysRam(TPhysAddr aPhysAddr, TUint aCount);
	void SetAllocPhysRam(TPhysAddr* aPageList, TUint aNumPages);

public:
	TLinAddr MapTemp(TPhysAddr aPage, TUint aColour, TUint aSlot=0);
	void UnmapTemp(TUint aSlot=0);
	void RemoveAliasesForPageTable(TPhysAddr aPageTable);

	static TBool MapPages(TPte* const aPtePtr, const TUint aCount, TPhysAddr* aPages, TPte aBlankPte);
	static TBool UnmapPages(TPte* const aPtePtr, TUint aCount);
	static TBool UnmapPages(TPte* const aPtePtr, TUint aCount, TPhysAddr* aPages);
	static void RemapPage(TPte* const aPtePtr, TPhysAddr& aPage, TPte aBlankPte);
	static void RestrictPagesNA(TPte* const aPtePtr, TUint aCount, TPhysAddr* aPages);
	static TBool PageInPages(TPte* const aPtePtr, const TUint aCount, TPhysAddr* aPages, TPte aBlankPte);

	// implemented in CPU-specific code...
	static TUint PteType(TMappingPermissions aPermissions, TBool aGlobal);
	static TUint PdeType(TMemoryAttributes aAttributes);
	static TPte BlankPte(TMemoryAttributes aAttributes, TUint aPteType);
	static TPde BlankPde(TMemoryAttributes aAttributes);
	static TPde BlankSectionPde(TMemoryAttributes aAttributes, TUint aPteType);
	static TBool CheckPteTypePermissions(TUint aPteType, TUint aAccessPermissions);
	static TMappingPermissions PermissionsFromPteType(TUint aPteType);
	void PagesAllocated(TPhysAddr* aPageList, TUint aCount, TRamAllocFlags aFlags, TBool aReallocate=false);
	void PageFreed(SPageInfo* aPageInfo);
	void CleanAndInvalidatePages(TPhysAddr* aPages, TUint aCount, TMemoryAttributes aAttributes, TUint aColour);
public:
	// utils, implemented in CPU-specific code...
	static TPde* PageDirectory(TInt aOsAsid);
	static TPde* PageDirectoryEntry(TInt aOsAsid, TLinAddr aAddress);
	static TPhysAddr PdePhysAddr(TPde aPde);
	static TPhysAddr PtePhysAddr(TPte aPte, TUint aPteIndex);
	static TPte* PageTableFromPde(TPde aPde);
	static TPte* SafePageTableFromPde(TPde aPde);
	static TPhysAddr SectionBaseFromPde(TPde aPde);
	static TPte* PtePtrFromLinAddr(TLinAddr aAddress, TInt aOsAsid);
	static TPte* SafePtePtrFromLinAddr(TLinAddr aAddress, TInt aOsAsid);
	static TPhysAddr PageTablePhysAddr(TPte* aPt);
	static TPhysAddr LinearToPhysical(TLinAddr aAddr, TInt aOsAsid=KKernelOsAsid);
	static TPhysAddr UncheckedLinearToPhysical(TLinAddr aAddr, TInt aOsAsid);
	static TPte MakePteInaccessible(TPte aPte, TBool aReadOnly);
	static TPte MakePteAccessible(TPte aPte, TBool aWrite);
	static TBool IsPteReadOnly(TPte aPte);
	static TBool IsPteMoreAccessible(TPte aNewPte, TPte aOldPte);
	static TBool IsPteInaccessible(TPte aPte);
	static TBool PdeMapsPageTable(TPde aPde);
	static TBool PdeMapsSection(TPde aPde);

	void SyncPhysicalMemoryBeforeDmaWrite(TPhysAddr* aPages, TUint aColour, TUint aOffset, TUint aSize, TUint32 aMapAttr);
	void SyncPhysicalMemoryBeforeDmaRead (TPhysAddr* aPages, TUint aColour, TUint aOffset, TUint aSize, TUint32 aMapAttr);
	void SyncPhysicalMemoryAfterDmaRead  (TPhysAddr* aPages, TUint aColour, TUint aOffset, TUint aSize, TUint32 aMapAttr);

	static TPte SectionToPageEntry(TPde& aPde);
	static TPde PageToSectionEntry(TPte aPte, TPde aPde);
	static TMemoryAttributes CanonicalMemoryAttributes(TMemoryAttributes aAttr);

public:
	/**
	Class representing the resources and methods required to create temporary
	mappings of physical memory pages in order to make them accessible to
	software.
	These mare required by various memory model functions and are created only
	during system boot.
	*/
	class TTempMapping
		{
	public:
		void Alloc(TUint aNumPages);
		TLinAddr Map(TPhysAddr aPage, TUint aColour);
		TLinAddr Map(TPhysAddr aPage, TUint aColour, TPte aBlankPte);
		TLinAddr Map(TPhysAddr* aPages, TUint aCount, TUint aColour);
		void Unmap();
		void Unmap(TBool aIMBRequired);
		FORCE_INLINE TTempMapping()
			: iSize(0)
			{}
	public:
		TLinAddr iLinAddr;		///< Virtual address of the memory page mapped by #iPtePtr.
		TPte* iPtePtr;			///< Pointer to first PTE allocated to this object.
	private:
		TPte iBlankPte;			///< PTE value to use for mapping pages, with the physical address component equal to zero.
		TUint8 iSize;			///< Maximum number of pages which can be mapped in one go.
		TUint8 iCount;			///< Number of pages currently mapped.
		TUint8 iColour;			///< Colour of any pages mapped (acts as index from #iLinAddr and #iPtePtr).
		TUint8 iSpare1;
	private:
		static TLinAddr iNextLinAddr;
		};
private:
	enum { KNumTempMappingSlots=2 };
	/**
	Temporary mappings used by various functions.
	Use of these is serialised by the #RamAllocLock.
	*/
	TTempMapping iTempMap[KNumTempMappingSlots];

	TTempMapping iPhysMemSyncTemp;	///< Temporary mapping used for physical memory sync.
	DMutex* 	 iPhysMemSyncMutex;	///< Mutex used to serialise use of #iPhysMemSyncTemp.

public:
	TPte iTempPteCached;			///< PTE value for cached temporary mappings
	TPte iTempPteUncached;			///< PTE value for uncached temporary mappings
	TPte iTempPteCacheMaintenance;	///< PTE value for temporary mapping of cache maintenance
private:
	DRamAllocator* iRamPageAllocator;			///< The RAM allocator used for managing free RAM pages.
	const SRamZone* iRamZones;					///< A pointer to the RAM zone configuration from the variant.
	TRamZoneCallback iRamZoneCallback;			///< Pointer to the RAM zone callback function.
	Defrag* iDefrag;							///< The RAM defrag class implementation.

	/**
	A counter incremented every time Mmu::PagesAllocated invalidates the L1 cache.
	This is used as part of a cache maintenance optimisation.
	*/
	TInt iCacheInvalidateCounter;

	/**
	Number of free RAM pages which are cached at L1 and have
	SPageInfo::CacheInvalidateCounter()==#iCacheInvalidateCounter.
	This is used as part of a cache maintenance optimisation.
	*/
	TInt iCacheInvalidatePageCount;

public:
	/**
	Linked list of threads which have an active IPC alias. I.e. have called
	DMemModelThread::Alias. Threads are linked by their DMemModelThread::iAliasLink member.
	Updates to this list are protected by the #MmuLock.
	*/
	SDblQue iAliasList;

	/**
	The mutex used to protect RAM allocation.
	This is the mutex #RamAllocLock operates on.
	*/
	DMutex* iRamAllocatorMutex;

private:
	/**
	Number of nested calls to RamAllocLock::Lock.
	*/
	TUint iRamAllocLockCount;

	/**
	Set by various memory allocation routines to indicate that a memory allocation
	has failed. This is used by #RamAllocLock in its management of out-of-memory
	notifications.
	*/
	TBool iRamAllocFailed;

	/**
	Saved value for #FreeRamInPages which is used by #RamAllocLock in its management
	of memory level change notifications.
	*/
	TUint iRamAllocInitialFreePages;

	friend class RamAllocLock;

#ifdef FMM_VERIFY_RAM
private:
	void VerifyRam();
#endif
	};

/**
The single instance of class #Mmu.
*/
extern Mmu TheMmu;


#ifndef _DEBUG
/**
Perform a page table walk to return the physical address of
the memory mapped at virtual address \a aLinAddr in the
address space \a aOsAsid.

If the page table used was not one allocated by the kernel
then the results are unpredictable and may cause a system fault.

@pre #MmuLock held.
*/
FORCE_INLINE TPhysAddr Mmu::LinearToPhysical(TLinAddr aAddr, TInt aOsAsid)
	{
	return Mmu::UncheckedLinearToPhysical(aAddr,aOsAsid);
	}
#endif


__ASSERT_COMPILE((Mmu::EAllocFlagLast>>Mmu::EAllocWipeByteShift)==0); // make sure flags don't run into wipe byte value


/**
Create a temporary mapping of a physical page.
The RamAllocatorMutex must be held before this function is called and not released
until after UnmapTemp has been called.

@param aPage	The physical address of the page to be mapped.
@param aColour	The 'colour' of the page if relevant.
@param aSlot	Slot number to use, must be less than Mmu::KNumTempMappingSlots.

@return The linear address of where the page has been mapped.
*/
FORCE_INLINE TLinAddr Mmu::MapTemp(TPhysAddr aPage, TUint aColour, TUint aSlot)
	{
//	Kern::Printf("Mmu::MapTemp(0x%08x,%d,%d)",aPage,aColour,aSlot);
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(aSlot<KNumTempMappingSlots);
	return iTempMap[aSlot].Map(aPage,aColour);
	}


/**
Remove the temporary mapping created with MapTemp.

@param aSlot	Slot number which was used when temp mapping was made.
*/
FORCE_INLINE void Mmu::UnmapTemp(TUint aSlot)
	{
//	Kern::Printf("Mmu::UnmapTemp(%d)",aSlot);
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(aSlot<KNumTempMappingSlots);
	iTempMap[aSlot].Unmap();
	}


/**
Class representing the resources and arguments needed for various
memory pinning operations.

The term 'replacement pages' in this documentation means excess
RAM pages which have been allocated to the demand paging pool so
that when a demand paged memory is pinned and removed the pool
does not become too small.

Relacement pages are allocated with #AllocReplacementPages and their
number remembered in #iReplacementPages. When a memory pinning operation
removes pages from the paging pool it will reduce #iReplacementPages
accordingly. At the end of the pinning operation, #FreeReplacementPages
is used to free any unused replacement pages.
*/
class TPinArgs
	{
public:
	/**
	Boolean value set to true if the requester of the pinning operation
	will only read from the pinned memory, not write to it.
	This is used as an optimisation to avoid unnecessarily marking
	demand paged memory as dirty.
	*/
	TBool iReadOnly;

	/**
	Boolean value set to true if sufficient replacement pages already exists
	in the demand paging pool and that #AllocReplacementPages does not need
	to actually allocated any.
	*/
	TBool iUseReserve;

	/**
	The number of replacement pages allocated to this object by #AllocReplacementPages.
	A value of #EUseReserveForPinReplacementPages indicates that #iUseReserve
	was true, and there is sufficient RAM already reserved for the operation
	being attempted.
	*/
	TUint iReplacementPages;

	/**
	The number of page tables which have been pinned during the course
	of an operation. This is the number of valid entries written to
	#iPinnedPageTables.
	*/
	TUint iNumPinnedPageTables;

	/**
	Pointer to the location to store the addresses of any page tables
	which have been pinned during the course of an operation. This is
	incremented as entries are added.

	The null-pointer indicates that page tables do not require pinning.
	*/
	TPte** iPinnedPageTables;

public:
	/**
	Construct an empty TPinArgs, one which owns no resources.
	*/
	inline TPinArgs()
		: iReadOnly(0), iUseReserve(0), iReplacementPages(0), iNumPinnedPageTables(0), iPinnedPageTables(0)
		{
		}

	/**
	Return true if this TPinArgs has at least \a aRequired number of
	replacement pages allocated.
	*/
	FORCE_INLINE TBool HaveSufficientPages(TUint aRequired)
		{
		return iReplacementPages>=aRequired; // Note, EUseReserveForPinReplacementPages will always return true.
		}

	/**
	Allocate replacement pages for this TPinArgs so that it has at least
	\a aNumPages.
	*/
	TInt AllocReplacementPages(TUint aNumPages);

	/**
	Free all replacement pages which this TPinArgs still owns.
	*/
	void FreeReplacementPages();

#ifdef _DEBUG
	~TPinArgs();
#endif

	/**
	Value used to indicate that replacement pages are to come
	from an already allocated reserve and don't need specially
	allocating.
	*/
	enum { EUseReserveForPinReplacementPages = 0xffffffffu };
	};


#ifdef _DEBUG
inline TPinArgs::~TPinArgs()
	{
	__NK_ASSERT_DEBUG(!iReplacementPages);
	}
#endif


/**
Enumeration used in various RestrictPages APIs to specify the type of restrictions to apply.
*/
enum TRestrictPagesType
	{
	/**
	Make all mappings of page not accessible.
	Pinned mappings will veto this operation.
	*/
	ERestrictPagesNoAccess			 = 1,

	/**
	Demand paged memory being made 'old'.
	Specific case of ERestrictPagesNoAccess.
	*/
	ERestrictPagesNoAccessForOldPage = ERestrictPagesNoAccess|0x80000000,

	/**
	For page moving pinned mappings always veto the moving operation.
	*/
	ERestrictPagesForMovingFlag  = 0x40000000,

	/**
	Movable memory being made no access whilst its being copied.
	Special case of ERestrictPagesNoAccess where pinned mappings always veto 
	this operation even if they are read-only mappings.
	*/
	ERestrictPagesNoAccessForMoving  = ERestrictPagesNoAccess|ERestrictPagesForMovingFlag,
	};

#include "xmmu.h"

#endif
