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

/**
 @file
 @internalComponent
*/

#ifndef MOBJECT_H
#define MOBJECT_H

#include "mrefcntobj.h"
#include "mmappinglist.h"
#include "mpagearray.h"

class DMemoryManager;
class DCoarseMapping;


/**
Base class for memory objects.

A memory object is a sparse array of memory pages (#iPages) to which memory
mappings may be attached (#iMappings). All pages in the array are managed
in the same way (#iManager).
*/
class DMemoryObject : public DReferenceCountedObject
	{
public:
	virtual ~DMemoryObject();

	/**
	Claim ownership of memory originally allocated by the bootstrap.

	This is used during system boot to initialise this object's memory to contain
	the pages which are already mapped at a given region of virtual addresses.

	For coarse memory objects, this function also takes ownership of the page tables
	being used to map the memory.

	@param aBase				Starting virtual address of memory to claim.

	@param aSize				Size, in bytes, of memory region to claim.

	@param aPermissions			The access permissions which the memory region
								is mapped. As well as being required for correct object
								initialisation this also enables the function to check
								that the bootstrap code mapped the memory in manner
								consistent with the flexible memory model implementation.

	@param aAllowGaps			True if the memory region may have gaps (unmapped pages) in it.
								If false, the function faults if the region is not fully
								populated with mapped pages.

	@param aAllowNonRamPages	True if the memory region contains memory pages which are
								not RAM pages known to the kernel. I.e. are not
								contained in the list of RAM banks supplied by the bootstrap.
								Any such memory cannot be never be subsequently freed from the
								memory object because the kernel can't handle this memory.

	@return KErrNone if successful, otherwise one of the system wide error codes.
	*/
	virtual TInt ClaimInitialPages(TLinAddr aBase, TUint aSize, TMappingPermissions aPermissions, TBool aAllowGaps=false, TBool aAllowNonRamPages=false) = 0;

	/**
	Update the page table entries for all attached mappings to add entries for
	a specified set of memory pages.

	This method is called by this object's manager whenever new pages of memory are added.

	@param aPages				An RPageArray::TIter which refers to a range of pages
								in this memory object.
								Only array entries which have state RPageArray::ECommitted
								should be mapped into a mapping's page tables.

	@return KErrNone if successful, otherwise one of the system wide error codes.
	*/
	virtual TInt MapPages(RPageArray::TIter aPages);

	/**
	Update the page table entries for all attached mappings to add new entry for
	a specified memory page.

	This method is called by this object's manager whenever the page is moved.

	@param aPageArray			The page array entry of the page in this memory object.
								Only array entries which have state RPageArray::ECommitted
								should be mapped into a mapping's page tables.

	@param aIndex				The index of the page in this memory object.

	@param	aInvalidateTLB		Set to ETrue when the TLB entries associated with this page
								should be invalidated.  This must be done when there is 
								already a valid pte for this page, i.e. if the page is still 
								mapped.

	@return KErrNone if successful, otherwise one of the system wide error codes.
	*/
	virtual void RemapPage(TPhysAddr& aPageArray, TUint aIndex, TBool aInvalidateTLB);

	/**
	Update the page table entries for all attached mappings to remove entries for
	a specified set of memory pages.

	This method is called this object's manager whenever pages of memory are removed.

	@param aPages				An RPageArray::TIter which refers to a range of pages
								in this memory object.
								Only array entries which return true for
								RPageArray::TargetStateIsDecommitted should be unmapped
								from a mapping's page tables.

	@param aDecommitting		True if memory is being permanently decommitted from
								the memory object. False if the memory pages are only
								temporarily being unmapped due to a demand paging 'page out'
								operation.
	*/
	virtual void UnmapPages(RPageArray::TIter aPages, TBool aDecommitting);

	/**
	Update the page table entries for all attached mappings to apply access restrictions
	to a specified set of memory pages.

	This method is called by this object's manager whenever pages of memory are restricted.

	@param aPages				An RPageArray::TIter which refers to a range of pages
								in this memory object.
								Only array entries which return true for
								RPageArray::TargetStateIsDecommitted should be unmapped
								from a mapping's page tables.

	@param aRestriction			A value from enum #TRestrictPagesType indicating the
								kind of restriction to apply.
	*/
	virtual void RestrictPages(RPageArray::TIter aPages, TRestrictPagesType aRestriction);

	/**
	Add a memory mapping to this memory object.

	This is only intended for use by #DMemoryMappingBase::Attach.

	After verifying that the mapping is permitted (using #CheckNewMapping)
	the mapping is linked into this objects list of mappings #iMappings.

	@param aMapping The mapping to add.

	@return KErrNone if successful,
			otherwise KErrAccessDenied to indicate that the mapping is not allowed.
	*/
	virtual TInt AddMapping(DMemoryMappingBase* aMapping);

	/**
	Remove a memory mapping from this memory object.

	This is only intended for use by #DMemoryMappingBase::Detach.

	This unlinks the mapping from the list of mappings #iMappings.

	@param aMapping The mapping to remove.
	*/
	virtual void RemoveMapping(DMemoryMappingBase* aMapping);

	/**
	Attempt to set the memory object read only.  This will only succeed if
	there are no writable mappings to the memory object.

	NOTE - This can't be undone, page moving will break if a memory object 
	is made writable again.

	@return KErrNone on success, KErrInUse if writable mappings are found.
	*/
	virtual TInt SetReadOnly();

	/**
	Create a mapping object to map this memory.

	This is only intended for use by #MM::MappingNew.
	
	The base class creates a #DFineMapping object.  It is overridden by #DCoarseMemory to create a
	#DCoarseMapping in appropriate circumstances.

	@param aIndex 		The index of the start of the mapping into this memory object.
	@oaram aCount		The size in pages of the mapping.
	*/
	virtual DMemoryMapping* CreateMapping(TUint aIndex, TUint aCount);

	/**
	Get the physical address(es) for a region of pages in this memory object.

	Depending on how the memory is being managed the physical addresses returned
	may become invalid due to various reasons, e.g.

	- memory being decommitted from the memory object
	- ram defragmentation moving the memory contents to a different physical page
	- paging out of demand paged memory

	This function should therefore only be used where it is know that these
	possibilities can't occur, e.g. this is used safely by DPhysicalPinMapping::PhysAddr.

	@param aIndex			Page index, within the memory, for the start of the region.
	@param aCount			Number of pages in the region.
	@param aPhysicalAddress	On success, this value is set to one of two values.
							If the specified region is physically contiguous,
							the value is the physical address of the first page
							in the region. If the region is discontiguous, the
							value is set to KPhysAddrInvalid.
	@param aPhysicalPageList If not zero, this points to an array of TPhysAddr
							objects. On success, this array will be filled
							with the addresses of the physical pages which
							contain the specified region. If aPageList is
							zero, then the function will fail with
							KErrNotFound if the specified region is not
							physically contiguous.

	@return 0 if successful and the whole region is physically contiguous.
			1 if successful but the region isn't physically contiguous.
			KErrNotFound, if any page in the region is not present,
			otherwise one of the system wide error codes.
	*/
	TInt PhysAddr(TUint aIndex, TUint aCount, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList);

	/**
	Check specified region lies entirely within this memory object, and that it
	is page aligned.
	*/
	TBool CheckRegion(TUint aIndex, TUint aCount);

	/**
	Clip the specified region to lie within the memory object,
	*/
	void ClipRegion(TUint& aIndex, TUint& aCount);

	/**
	Set the mutex used to lock operations on this memory object
	*/
	void SetLock(DMutex* aLock);

	/**
	Prevents any further mappings being added to this memory object.
	*/
	void DenyMappings();

	/**
	Return the memory attributes for this object's memory.
	*/
	FORCE_INLINE TMemoryAttributes Attributes()
		{
		return (TMemoryAttributes)iAttributes;
		}

	/**
	Value for initialising SPageInfo::iFlags when allocating pages.
	*/
	FORCE_INLINE TUint8 PageInfoFlags()
		{
		return iAttributes;
		}

	/**
	Value for #Mmu::TRamAllocFlags to use when allocating pages.
	*/
	FORCE_INLINE Mmu::TRamAllocFlags RamAllocFlags()
		{
		return (Mmu::TRamAllocFlags)iRamAllocFlags;
		}

	/**
	Return true if this object is an instance of #DCoarseMemory.
	*/
	FORCE_INLINE TBool IsCoarse()
		{
		return iFlags&ECoarseObject;
		}

	/**
	Return true if this object contains memory which is being demand paged.
	*/
	FORCE_INLINE TBool IsDemandPaged()
		{
		return iFlags&EDemandPaged;
		}

	/**
	Return true if writeable mappings of the memory are not allowed.
	*/
	FORCE_INLINE TBool IsReadOnly()
		{
		return iFlags&EDenyWriteMappings;
		}

	/**
	Return true if executable mappings allowed on this memory object.
	*/
	FORCE_INLINE TBool IsExecutable()
		{
		return !(iFlags&EDenyExecuteMappings);
		}

	/**
	Clear the flag that indicates that a mapping has been added.
	*/
	FORCE_INLINE void ClearMappingAddedFlag()
		{
		__NK_ASSERT_DEBUG(iMappings.LockIsHeld());
		__e32_atomic_and_ord8(&iFlags, (TUint8)~EMappingAdded);
		}

	/**
	Set the flag to indicate that a mapping has been added.
	*/
	FORCE_INLINE void SetMappingAddedFlag()
		{
		__NK_ASSERT_DEBUG(iMappings.LockIsHeld());
		__NK_ASSERT_DEBUG(MmuLock::IsHeld());
		__e32_atomic_ior_ord8(&iFlags, (TUint8)EMappingAdded);
		}

	/**
	Get the value of the mappings added flags

	@return ETrue if a mapping has been added, EFalse otherwise.
	*/
	FORCE_INLINE TBool MappingAddedFlag()
		{
		__NK_ASSERT_DEBUG(MmuLock::IsHeld());
		return iFlags & (TUint8)EMappingAdded;
		}

	enum
		{
		/**
		Maximum number of bits which can be stored in an array entry by SetPagingManagerData.
		*/
		KPagingManagerDataBits = RPageArray::KPagingManagerDataBits
		};

	enum
		{
		/**
		Maximum value which can be stored in an array entry by SetPagingManagerData.
		*/
		KMaxPagingManagerData = RPageArray::KMaxPagingManagerData
		};

	/**
	Write \a aValue to the paging manager data for page index \a aIndex.
	The value must not exceed KMaxPagingManagerData.
	This must only be used for demand paged memory objects.
	*/
	void SetPagingManagerData(TUint aIndex, TUint aValue);

	/**
	Return the paging manager data for page index \a aIndex.
	This must only be used for demand paged memory objects.
	@see SetPagingManagerData
	*/
	TUint PagingManagerData(TUint aIndex);

	/**
	Check that a given memory mapping is allowed to be attached to this memory object.

	@param aMapping	The mapping to check.

	@return KErrNone if successful,
			otherwise KErrAccessDenied to indicate that the mapping is not allowed.
	*/
	TInt CheckNewMapping(DMemoryMappingBase* aMapping);

	/**
	Emit BTrace traces identifying the initial attributes of this object.
	*/
	void BTraceCreate();

protected:
	/**
	@param aManager		The manager object for this memory.
	@param aFlags		Initial value for #iFlags.
	@param aSizeInPages	Size of the memory object, in number of pages.
	@param aAttributes	Bitmask of values from enum #TMemoryAttributes.
	@param aCreateFlags	Bitmask of option flags from enum #TMemoryCreateFlags.
	*/
	DMemoryObject(DMemoryManager* aManager, TUint aFlags, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags);

	/**
	Second phase constructor.

	@return KErrNone if successful, otherwise one of the system wide error codes.
	*/
	TInt Construct();

public:
	/**
	The manager of this memory object.
	*/
	DMemoryManager*	iManager;

	/**
	For use by this object's manager (iManager) to store any memory objects specific state
	it requires to keep track of.
	*/
	TAny*			iManagerData;

	/**
	For use by DMemoryManager::QueueCleanup to link objects which require a cleanup operation.
	Access to this is protected by #DMemoryManager::iCleanupLock.
	*/
	DMemoryObject*	iCleanupNext;

	/**
	For use by DMemoryManager::QueueCleanup to store flags representing each pending cleanup operation.
	Access to this is protected by #DMemoryManager::iCleanupLock.
	*/
	TUint32			iCleanupFlags;

	/**
	Bit flags stored in #iFlags giving various state and attributes of the object.
	*/
	enum TFlags
		{
		/**
		Flag set during object construction to indicate that this mapping is of
		class #DCoarseMemory.
		*/
		ECoarseObject			= 1<<0,

		/**
		Flag set during object construction to indicate that the memory for this object
		is being demand paged in some manner.
		*/
		EDemandPaged			= 1<<1,

		/**
		Flag set during object construction to indicate that all resources for this
		object are to be reserved during construction; excluding memory pages owned by
		object. Objects constructed in this way will not require additional memory
		allocation when committing memory to them (other than allocating the memory
		pages being committed.)
		*/
		EReserveResources		= 1<<2,

		/**
		Flag set during object construction to indicate that pinned memory mappings
		are not allowed to be attached to this object.
		*/
		EDenyPinning			= 1<<3,

		/**
		Flag set by DenyMappings to indicate that no additional memory mappings
		are allowed to be attached to this object.
		*/
		EDenyMappings			= 1<<4,

		/**
		Flag set during object construction, or by SetReadOnly, to indicate that
		the memory object is read-only and no writable mappings are allowed
		to be attached to this object.
		*/
		EDenyWriteMappings		= 1<<5,

		/**
		Flag set during object construction to indicate that executable memory mappings
		are not allowed to be attached to this object.
		This is mainly an optimisation to allow demand paging to avoid instruction cache
		maintenance operations during page fault handling.
		*/
		EDenyExecuteMappings	= 1<<6,

		/**
		Flag set whenever a new mapping is added to a memory object.
		The object's mappings lock and MmuLock protects this flag when it is set
		and the mappings lock protects when it is cleared.
		*/
		EMappingAdded			= 1<<7,
		};

	/**
	Bitmask of TFlags
	*/
	TUint8 iFlags;

	/**
	Value from TMemoryAttributes indicating type of memory in object.
	*/
	TUint8 iAttributes;

	/**
	#Mmu::TRamAllocFlags value to use when allocating RAM for this memory object.
	*/
	TUint16 iRamAllocFlags;

	/**
	List of mappings currently attached to this object.
	*/
	TMappingList iMappings;

	/**
	Size, in page units, of this memory object.
	*/
	TUint iSizeInPages;

	/**
	Lock currently being used to serialise explicit memory operations.
	This is assigned using #MemoryObjectLock.
	*/
	DMutex* iLock;

	/**
	The array of memory pages assigned to this memory object.
	*/
	RPageArray iPages;
	};



/**
A memory object which has a size that is an exact multiple
multiple of the region covered by a whole MMU page table;
that is a 'chunk' size (#KChunkSize) bytes.

When used in conjunction with DCoarseMapping this object
allows RAM to be saved by sharing MMU page tables between multiple
different mappings of the memory.

Fine memory mappings (DFineMapping) may also be attached
to this memory object but these won't benefit from page table
sharing.
*/
class DCoarseMemory : public DMemoryObject
	{
public:
	// from DMemoryObject...
	virtual ~DCoarseMemory();
	virtual TInt ClaimInitialPages(TLinAddr aBase, TUint aSize, TMappingPermissions aPermissions, TBool aAllowGaps=false, TBool aAllowNonRamPages=false);
	virtual TInt MapPages(RPageArray::TIter aPages);
	virtual void RemapPage(TPhysAddr& aPageArray, TUint aIndex, TBool aInvalidateTLB);
	virtual void UnmapPages(RPageArray::TIter aPages, TBool aDecommitting);
	virtual void RestrictPages(RPageArray::TIter aPages, TRestrictPagesType aRestriction);
	virtual TInt AddMapping(DMemoryMappingBase* aMapping);
	virtual void RemoveMapping(DMemoryMappingBase* aMapping);
	virtual TInt SetReadOnly();
	virtual DMemoryMapping* CreateMapping(TUint aIndex, TUint aCount);
public:
	/**
	Create a new DCoarseMemory object.

	@param aManager		The manager object for this memory.
	@param aSizeInPages	Size of the memory object, in number of pages.
						(Must represent an exact 'chunk' size.)
	@param aAttributes	Bitmask of values from enum #TMemoryAttributes.
	@param aCreateFlags	Bitmask of option flags from enum #TMemoryCreateFlags.

	@return The newly created DCoarseMemory or the null pointer if there was
			insufficient memory.
	*/
	static DCoarseMemory* New(DMemoryManager* aManager, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags);

	/**
	Remove an mmu page table from this memory object's ownership.
	This is called when a RAM page containing the page table is paged out.
	This function delegates its action to DPageTables::StealPageTable.

	@param aChunkIndex	The index of the page table, i.e. the offset, in 'chunks',
						into the object's memory that the page table is being used to map.
						(The index into DPageTables::iTables.)
	@param aPteType		The #TPteType the page table is being used for.
						(The index into #iPageTables.)

	@pre #MmuLock is held.
	@pre #PageTablesLockIsHeld
	*/
	void StealPageTable(TUint aChunkIndex, TUint aPteType);

public:
	// Interface for DCoarseMapping
	
	/**
	Get the page table to use for mapping a specified chunk if it exists.

	@param aPteType		The #TPteType the page tables will be used for.
	@param aChunkIndex	The index of the chunk.

	@return The virtual address of the page table, or NULL.

	@pre #MmuLock is held.
	*/
	TPte* GetPageTable(TUint aPteType , TUint aChunkIndex);

	/**
	Update the page tables to add entries for a specified set of demand paged memory
	pages following a 'page in' or memory pinning operation.

	@param aMapping				The mapping the pages are being paged into.
	
	@param aPages				An RPageArray::TIter which refers to a range of pages
								in the memory object #iMemory.
								Only array entries which have state RPageArray::ECommitted
								should be mapped into the page tables.

	@param aPinArgs				The resources required to pin any page tables.
								Page table must be pinned if \a aPinArgs.iPinnedPageTables is
								not the null pointer, in which case this the virtual address
								of the pinned must be stored in the array this points to.
								\a aPinArgs.iReadOnly is true if write access permissions
								are not needed.

	@return KErrNone if successful, otherwise one of the system wide error codes.

	@pre #MmuLock is held.
	@post #MmuLock has been released.
	*/
	TInt PageIn(DCoarseMapping* aMapping, RPageArray::TIter aPages, TPinArgs& aPinArgs, TUint aMapInstanceCount);

	/**
	Update the page table entries to renable access to a specified memory page.

	This method is called by #DCoarseMapping::MovingPageIn

	@param aMapping				The mapping which maps the page.
	@param aPageArrayPtr		The page array entry of the page to map.
								Only array entries which have state RPageArray::ECommitted
								should be mapped into a mapping's page tables.

	@param aIndex				The index of the memory page.
	*/
	TBool MovingPageIn(DCoarseMapping* aMapping, TPhysAddr& aPageArrayPtr, TUint aIndex);

	/**
	Function to return a page table pointer for the specified linear address and
	index to this mapping.

	This method is called by #DCoarseMapping::FindPageTable.
	
	@param aLinAddr		The linear address to find the page table entry for.
	@param aMemoryIndex	The memory object index of the page to find the page 
						table entry for.
	
	@return A pointer to the page table entry, if the page table entry couldn't 
			be found this will be NULL
	*/
	TPte* FindPageTable(DCoarseMapping* aMapping, TLinAddr aLinAddr, TUint aMemoryIndex);
	
protected:
	/**
	For arguments, see #New.
	*/
	DCoarseMemory(DMemoryManager* aManager, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags);
	
public:
	/**
	The object which manages the page tables owned by a #DCoarseMemory object
	and used by #DCoarseMapping objects. Each DPageTables is used for mappings
	with a specific #TPteType e.g. set of memory access permissions, and all these
	#DCoarseMapping objects are linked into this object whenever they are
	attached to a DCoarseMemory.
	*/
	class DPageTables : public DReferenceCountedObject
		{
	public:
		/**
		Create a new DPageTables.

		This object is added to DCoarseMemory::iPageTables and all of
		the mmu page tables will be initialised to map the memory currently
		owned by the memory object (unless memory is demand paged).


		@param aMemory		The DCoarseMemory the new object is associated with.
		@param aNumPages	Size of the memory object, in number of pages.
							(Must represent an exact 'chunk' size.)
		@param aPteType		The #TPteType the page tables will be used for.

		@return The newly created DPageTables or the null pointer if there was
				insufficient memory.

		@pre The #MemoryObjectLock for the memory must be held by the current thread.
		*/
		static DPageTables* New(DCoarseMemory* aMemory, TUint aNumPages, TUint aPteType);

		virtual ~DPageTables();

		/**
		Update the page tables to add entries for a specified set of memory pages.

		This method is called by #DCoarseMemory::MapPages.

		@param aPages				An RPageArray::TIter which refers to a range of pages
									in the memory object #iMemory.
									Only array entries which have state RPageArray::ECommitted
									should be mapped into the page tables.

		@return KErrNone if successful, otherwise one of the system wide error codes.
		*/
		virtual TInt MapPages(RPageArray::TIter aPages);

		/**
		Update the page table entries for a specified memory page.

		This method is called by #DCoarseMemory::RemapPage

		@param aPageArray			The page array entry of the page in this memory object.
									Only array entries which have state RPageArray::ECommitted
									should be mapped into a mapping's page tables.

		@param aIndex				The index of the page in this memory object.

		@param	aInvalidateTLB		Set to ETrue when the TLB entries associated with this page
									should be invalidated.  This must be done when there is 
									already a valid pte for this page, i.e. if the page is still 
									mapped.
		*/
		virtual void RemapPage(TPhysAddr& aPageArray, TUint aIndex, TBool aInvalidateTLB);

		/**
		Update the page table entries to renable access to a specified memory page.

		This method is called by #DCoarseMemory::MovingPageIn

		@param aPageArrayPtr		The page array entry of the page to map.
									Only array entries which have state RPageArray::ECommitted
									should be mapped into a mapping's page tables.

		@param aIndex				The index of the memory page.
		*/
		virtual TBool MovingPageIn(TPhysAddr& aPageArrayPtr, TUint aIndex);


		/**
		Update the page tables to remove entries for a specified set of memory pages.

		This method is called by #DCoarseMemory::UnmapPages.

		@param aPages				An RPageArray::TIter which refers to a range of pages
									in the memory object #iMemory.
									Only array entries which return true for
									RPageArray::TargetStateIsDecommitted should be unmapped
									from the page tables.

		@param aDecommitting		True if memory is being permanently decommitted from
									the memory object. False if the memory pages are only
									temporarily being unmapped due to a demand paging 'page out'
									operation.
		*/
		virtual void UnmapPages(RPageArray::TIter aPages, TBool aDecommitting);

		/**
		Update the page tables to apply access restrictions to a specified set of memory pages.

		This method is called by #DCoarseMemory::RestrictPagesNA.

		@param aPages				An RPageArray::TIter which refers to a range of pages
									in the memory object #iMemory.
									Only array entries which return true for
									RPageArray::TargetStateIsDecommitted should be unmapped
									from the page tables.
		*/
		virtual void RestrictPagesNA(RPageArray::TIter aPages);

		/**
		Update the page tables to add entries for a specified set of demand paged memory
		pages following a 'page in' or memory pinning operation.

		@param aPages				An RPageArray::TIter which refers to a range of pages
									in the memory object #iMemory.
									Only array entries which have state RPageArray::ECommitted
									should be mapped into the page tables.

		@param aPinArgs				The resources required to pin any page tables.
									Page table must be pinned if \a aPinArgs.iPinnedPageTables is
									not the null pointer, in which case this the virtual address
									of the pinned must be stored in the array this points to.
									\a aPinArgs.iReadOnly is true if write access permissions
									are not needed.

		@param aMapping				The mapping that took the page fault or is being pinned.

		@param aMapInstanceCount	The instance count of the mapping.

		@return KErrNone if successful, otherwise one of the system wide error codes.
		*/
		virtual TInt PageIn(RPageArray::TIter aPages, TPinArgs& aPinArgs,
							DMemoryMappingBase* aMapping, TUint aMapInstanceCount);

		/**
		Flush the MMUs TLB entries associated with all attached memory mappings
		for a specified region of memory pages.

		This is used by UnmapPages and RestrictPages.

		@param aStartIndex	Page index, within the memory, for start of the region.
		@param aEndIndex	Page index, within the memory, for the first page after
							the end of the region.
		*/
		void FlushTLB(TUint aStartIndex, TUint aEndIndex);


		/**
		Get the page table being used for a specified chunk index if it exists.

		@param aChunkIndex	The index into #iTables of the page table.

		@return The virtual address of the page table,
				or the null pointer if one wasn't found.
		*/
		inline TPte* GetPageTable(TUint aChunkIndex)
			{
			__NK_ASSERT_DEBUG(MmuLock::IsHeld());
			return iTables[aChunkIndex];
			}

		/**
		Get the page table being used for a specified chunk index; allocating
		a new one if it didn't previously exist.

		@param aChunkIndex	The index into #iTables of the page table.

		@return The virtual address of the page table,
				or the null pointer if one wasn't found and couldn't be allocated.
		*/
		TPte* GetOrAllocatePageTable(TUint aChunkIndex);

		/**
		Get and pin the page table being for a specified chunk index; allocating
		a new one if it didn't previously exist.

		@param aChunkIndex	The index into #iTables of the page table.
		@param aPinArgs		The resources required to pin the page table.
							On success, the page table will have been appended to
							\a aPinArgs.iPinnedPageTables.

		@return The virtual address of the page table,
				or the null pointer if one wasn't found and couldn't be allocated.
		*/
		TPte* GetOrAllocatePageTable(TUint aChunkIndex, TPinArgs& aPinArgs);

		/**
		Allocate a single page table.

		@param aChunkIndex	The index into #iTables of the page table.
		@param aDemandPaged True if the page table is for mapping demand paged memory.  Most of the
		                    time this will be determined by the #EDemandPaged bit in #iFlags.
		@param aPermanent	True, if the page table's permanence count is to be incremented.

		@return The virtual address of the page table,
				or the null pointer if one wasn't found and couldn't be allocated.
		*/
		TPte* AllocatePageTable(TUint aChunkIndex, TBool aDemandPaged, TBool aPermanent=false);

		/**
		Free a single page table if it is unused.

		@param aChunkIndex	The index into #iTables of the page table.
		*/
		void FreePageTable(TUint aChunkIndex);

		/**
		Allocate all the mmu page tables for this object (iTables) and ensure that
		they are not freed even when they no longer map any pages.

		This method increments iPermanenceCount.

		This is called by DCoarseMemory::AddMapping when a memory mapping is
		added with the #DMemoryMappingBase::EPermanentPageTables attribute is set.
		This will also be true if the memory object has the #EReserveResources
		attribute.

		@pre The #MemoryObjectLock for the memory must be held by the current thread.

		@return KErrNone if successful, otherwise one of the system wide error codes.
		*/
		TInt AllocatePermanentPageTables();

		/**
		Reverses the action of #AllocatePermanentPageTables.

		This method decrements iPermanenceCount and if this reaches zero,
		the mmu page tables for this object are freed if the are no longer in use.
		*/
		void FreePermanentPageTables();

		/**
		This is called by DCoarseMemory::AddMapping when a coarse memory mapping is
		added.

		@param aMapping	The coarse memory mapping to add.

		@return KErrNone if successful, otherwise one of the system wide error codes.
		*/
		TInt AddMapping(DCoarseMapping* aMapping);

		/**
		This is called by DCoarseMemory::RemoveMapping when a coarse memory mapping is
		removed.

		@param aMapping	The coarse memory mapping to remove.
		*/
		void RemoveMapping(DCoarseMapping* aMapping);

		/**
		Overriding DReferenceCountedObject::Close.
		This removes the linkage with #iMemory if this object is deleted.
		*/
		void Close();

		/**
		Overriding DReferenceCountedObject::AsyncClose.
		This removes the linkage with #iMemory if this object is deleted.
		@pre No fast mutex must be held.  Unlike DReferenceCountedObject::AsyncClose().
		*/
		void AsyncClose();

		/**
		Remove an mmu page table from this object's ownership.
		This is called from DCoarseMemory::StealPageTable when a RAM page containing
		the page table is paged out.

		@param aChunkIndex	The index into #iTables of the page table.

		@pre #MmuLock is held.
		@pre #PageTablesLockIsHeld
		*/
		void StealPageTable(TUint aChunkIndex);
		
	protected:
		/**
		For arguments, see #New.
		*/
		DPageTables(DCoarseMemory* aMemory, TInt aNumPts, TUint aPteType);

		/**
		Second phase constructor.

		This initialises all of the mmu page tables to map the memory currently owned
		by the memory object (#iMemory).

		@return KErrNone if successful, otherwise one of the system wide error codes.
		*/
		TInt Construct();

	private:
		/**
		Reverses the action of #AllocatePermanentPageTables for a range of page tables.

		This is an implementation factor for #FreePermanentPageTables().

		@param aChunkIndex	The index into #iTables of the first page table.
		@param aChunkCount	The number of page tables.
		*/
		void FreePermanentPageTables(TUint aChunkIndex, TUint aChunkCount);

		/**
		Assign a newly allocated page table to this object.

		This adds the page table to the page directory entries associated with
		all mappings attached to this object.

		@param aChunkIndex	The index into #iTables of the page table.
		@param aPageTable	The page table.

		@pre #PageTablesLockIsHeld.
		*/
		void AssignPageTable(TUint aChunkIndex, TPte* aPageTable);

		/**
		Unassign a page table to this object.

		This removes the page table from the page directory entries associated with
		all mappings attached to this object.

		This is called by FreePageTable and StealPageTable.

		@param aChunkIndex	The index into #iTables of the page table.

		@pre #PageTablesLockIsHeld.
		*/
		void UnassignPageTable(TUint aChunkIndex);

	public:
		/**
		The coarse memory object which owns us.
		*/
		DCoarseMemory* iMemory;

		/**
		The #TPteType the page tables are being used for.
		(This object's index in #iMemory->iPageTables.)
		*/
		TUint iPteType;

		/**
		The list of coarse mappings attached to this object.
		These mappings use the mmu page tables owned by us.
		*/
		TMappingList iMappings;

		/**
		The page table entry (PTE) value for use when mapping pages into the page tables.
		This value has the physical address component being zero, so a page's physical
		address can be simply ORed in.
		*/
		TPte iBlankPte;

		/**
		Reference count for the number of times #AllocatePermanentPageTables
		has been called without #FreePermanentPageTables.
		*/
		TUint iPermanenceCount;

		/**
		Number of entries in #iTables.
		*/
		TUint iNumPageTables;

		/**
		Array of page tables owned by this object. This may extend into memory
		beyond the end of this object and contains #iNumPageTables entries.

		Each entry in the array corresponds to a #KChunkSize sized region of #iMemory.
		The null pointer indicating that no page table exists for the corresponding
		region.

		The contents of the array are protected by the PageTableAllocator lock AND #MmuLock
		*/
		TPte* iTables[1];
		};

private:
	/**
	Get or allocate the page tables container for a given PTE type

	@pre #MemoryObjectLock for this object must be held.
	*/
	DPageTables* GetOrAllocatePageTables(TUint aPteType);

protected:
	/**
	Array of #DPageTables objects owned by this memory object.
	Updates to this array require the #MmuLock.
	*/
	DPageTables* iPageTables[ENumPteTypes];

	friend class DCoarseMemory::DPageTables;  // for DPageTables::Close() / AsyncClose()
	};



/**
A memory object without the special case optimisations of DCoarseMemory.
*/
class DFineMemory : public DMemoryObject
	{
public:
	// from DMemoryObject...
	virtual ~DFineMemory();
	virtual TInt ClaimInitialPages(TLinAddr aBase, TUint aSize, TMappingPermissions aPermissions, TBool aAllowGaps=false, TBool aAllowNonRamPages=false);

public:
	/**
	Create a new DFineMemory object.

	@param aManager		The manager object for this memory.
	@param aSizeInPages	Size of the memory object, in number of pages.
	@param aAttributes	Bitmask of values from enum #TMemoryAttributes.
	@param aCreateFlags	Bitmask of option flags from enum #TMemoryCreateFlags.

	@return The newly created DFineMemory or the null pointer if there was
			insufficient memory.
	*/
	static DFineMemory* New(DMemoryManager* aManager, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags);

private:
	/**
	For arguments, see #New.
	*/
	DFineMemory(DMemoryManager* aManager, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags);
	};


#endif
