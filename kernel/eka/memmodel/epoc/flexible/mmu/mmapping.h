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

#ifndef MMAPPING_H
#define MMAPPING_H

#include "mrefcntobj.h"
#include "mmappinglist.h"
#include "mpagearray.h"



/**
Base class for memory mappings.

This provides the methods for linking a mapping to a memory object
as well as the interface for updating the MMU page tables associated
with a mapping when the memory state changes.
*/
class DMemoryMappingBase : public DReferenceCountedObject
	{
private:
	/**
	Memory object to which this mapping is currently attached.
	Updates to the are protected by the MmuLock.
	*/
	DMemoryObject*	 iMemory;

public:
	/**
	Link used to maintain list of mappings attached to a memory object.
	*/
	TMappingListLink iLink;

	/**
	Offset, in page units, within the memory object's memory for start of this mapping.
	*/
	TUint			 iStartIndex;

	/**
	Size of this mapping, in page units.
	*/
	TUint			 iSizeInPages;

private:
	/**
	Instance count which is incremented every time a mapping is attached to a memory object.
	When code is manipulating mappings, the instance count is used to detect that a
	mapping has been reused and that the operation it is performing is no long needed.
	*/
	TUint			 iMapInstanceCount;

public:

	/**
	Bit flags stored in #Flags giving various state and attributes of the mapping.
	*/
	enum TFlags
		{
		/**
		Flag set during object construction to indicate that this mapping is of
		class #DCoarseMapping.
		*/
		ECoarseMapping			= 1<<0,

		/**
		Flag set during object construction to indicate that this mapping will pin
		any memory pages it maps. This may not be used with coarse memory mappings.
		*/
		EPinned					= 1<<1,

		/**
		Pages have already been reserved for pinning, so when this mapping is attached
		to a memory object no additional pages need to be reserved. Pre-reserving pages
		is used to prevent the possibility of failing to pin due to an out of memory
		condition. It is essential that the users of these mappings ensure that there
		are enough reserved pages in the paging pool to meet the maximum mapping size
		used.
		*/
		EPinningPagesReserved	= 1<<2,

		/**
		Pages have been successfully pinned by this mapping. This is set after demand
		paged memory has been succeeded pinned and is used to indicate that the pages
		need unpinning again when the mapping is later unmapped.
		*/
		EPagesPinned			= 1<<3,

		/**
		Flag set during object construction to indicate that MMU page tables are to
		be permanently allocated for use by this mapping. Normally, page tables are
		allocated as needed to map memory which can result in out-of-memory errors
		when mapping memory pages.
		*/
		EPermanentPageTables	= 1<<4,

		/**
		Permanent page tables have been successfully been allocated for this mapping.
		This flag is used to track allocation so they can be released when the mapping
		is destroyed.
		*/
		EPageTablesAllocated	= 1<<5,

		/**
		For pinned mappings (EPinned) this flag is set whenever the mapping prevents
		any pages of memory from being fully decommitted from a memory object. When a
		mapping is finally unmapped from the memory object this flag is checked, and,
		if set, further cleanup of the decommitted pages triggered.
		*/
		EPageUnmapVetoed		= 1<<6,

		/**
		Mapping is being, or has been, detached from a memory object.
		When set, operations on the mapping should act as though the mapping is no
		longer attached to a memory object. Specifically, no further pages of memory
		should be mapped into this mapping.

		This flag is only set when the MmuLock is held.
		*/
		EDetaching				= 1<<7,

		/**
		This mapping is a physical pinning mapping.  The pages it pins
		cannot be paged out or moved.

		This flag is set when DPhysicalPinMapping objects are created.
		*/
		EPhysicalPinningMapping = 1<<8,

		/**
		Flag set during object construction to indicate that this mapping is of
		class #DLargeMapping.

		Note that #DLargeMapping is derived from #DCoarseMapping, therefore presence of this flag
		implies presence of #ECoarseMapping as well.
		*/
		ELargeMapping			= 1<<9,
		};

	/**
	Bitmask of values from enum #TPteType which will be used to calculate
	the correct attributes for any page table entries this mapping uses.
	*/
	FORCE_INLINE TUint8& PteType()
		{ return iLink.iSpare1; }

	/**
	Bitmask of values from enum #TFlags.
	The flags 16 bits and are stored in iLink.iSpare2 and iLink.iSpare3.
	*/
	FORCE_INLINE TUint16& Flags()
		{ return (TUint16&)iLink.iSpare2; }

public:
	/**
	Return the memory object to which this mapping is currently attached.

	@pre MmuLock is held. (If aNoCheck==false)
	*/
	FORCE_INLINE DMemoryObject* Memory(TBool aNoCheck=false)
		{
		if(!aNoCheck)
			__NK_ASSERT_DEBUG(MmuLock::IsHeld());
		return iMemory;
		}

	/**
	Return true if the mapping is currently attached to a memory object.
	*/
	FORCE_INLINE TBool IsAttached()
		{ return iLink.IsLinked(); }

	/**
	Return true if the mapping is being, or has been, detached from a memory object.
	The mapping may or may not still be attached to a memory object, i.e. #IsAttached
	is indeterminate.
	*/
	FORCE_INLINE TBool BeingDetached()
		{ return Flags()&EDetaching; }

	/**
	Return the mapping instance count.
	@see #iMapInstanceCount.
	*/
	FORCE_INLINE TUint MapInstanceCount()
		{ return iMapInstanceCount; }

	/**
	Return true if this mapping provides read only access to memory.
	*/
	FORCE_INLINE TBool IsReadOnly()
		{ return !(PteType()&EPteTypeWritable); }

#ifdef MMU_SUPPORTS_EXECUTE_NEVER
	/**
	Return true if this mapping provides access to memory which allows
	code to be executed from it.
	*/
	FORCE_INLINE TBool IsExecutable()
		{ return (PteType()&EPteTypeExecutable); }
#endif

	/**
	Return true if this is a coarse mapping, in other words it is an instance of #DCoarseMapping or
	#DLargeMapping.
	*/
	FORCE_INLINE TBool IsCoarse()
		{ return Flags()&ECoarseMapping; }

	/**
	Return true if this mapping is a large mapping, in other words an instance of #DLargeMapping.

	Note that all large mappings are also coarse mappings.
	*/
	FORCE_INLINE TBool IsLarge()
		{ return Flags()&ELargeMapping; }

	/**
	Return true if this mapping pins the memory it maps.
	*/
	FORCE_INLINE TBool IsPinned()
		{ return Flags()&EPinned; }

	/**
	Return true if this mapping physically pins the memory it maps.
	*/
	FORCE_INLINE TBool IsPhysicalPinning()
		{ return Flags()&EPhysicalPinningMapping; }

	/**
	Return true if this mapping has beed successfully attached to a memory object, pinning its pages.
	*/
	FORCE_INLINE TBool PagesPinned()
		{ return Flags()&EPagesPinned; }		
		
	/**
	Return the access permissions which this mapping uses to maps memory.
	*/
	FORCE_INLINE TMappingPermissions Permissions()
		{ return Mmu::PermissionsFromPteType(PteType()); }

	/**
	Link this mapping to a memory object.

	This is called by the memory object during processing of #Attach.

	@param aMemory		The memory object the mapping is being attached to.
	@param aMappingList	The list to add this mapping to.

	@pre MmuLock is held.
	@pre Mapping list lock is held.
	*/
	void LinkToMemory(DMemoryObject* aMemory, TMappingList& aMappingList);

	/**
	Unlink this mapping from the memory object it was previously linked to with
	#LinkToMemory.

	This is called by the memory object during processing of #Detach.

	@param aMappingList	The list that the mapping appears on.
	*/
	void UnlinkFromMemory(TMappingList& aMappingList);

	/**
	Get the physical address(es) for a region of pages in this mapping.

	@param aIndex			Page index, within the mapping, for start of the region.
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

	@pre This mapping must have been attached to a memory object with #Pin.
	*/
	TInt PhysAddr(TUint aIndex, TUint aCount, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList);

protected:
	/**
	@param aType Initial value for #Flags.
	*/
	DMemoryMappingBase(TUint aType);

	/**
	Attach this mapping to a memory object so that it maps a specified region of its memory.

	@param aMemory	The memory object.
	@param aIndex	The page index of the first page of memory to be mapped by the mapping.
	@param aCount	The number of pages of memory to be mapped by the mapping.

	@return KErrNone if successful, otherwise one of the system wide error codes.
	*/
	TInt Attach(DMemoryObject* aMemory, TUint aIndex, TUint aCount);

	/**
	Remove this mapping from the memory object it was previously attached to by #Attach.
	*/
	void Detach();

public:
	/**
	Update the page table entries corresponding to this mapping to add entries for
	a specified set of memory pages.

	This method is called by DMemoryObject::MapPages to update each mapping attached
	to a memory object whenever new pages of memory are added. However, it won't be
	called for any mapping with the #EPinned attribute as such mappings are unchanging.

	@param aPages				An RPageArray::TIter which refers to a range of pages
								in a memory object. This has been clipped to fit within
								the range of pages mapped by this mapping.
								Only array entries which have state RPageArray::ECommitted
								should be mapped into the mapping's page tables.

	@param aMapInstanceCount	The instance of this mapping which is to be updated.
								Whenever this no longer matches the current #MapInstanceCount
								the function must not update any more of the mapping's
								page table entries, (but must still return KErrNone).

	@return KErrNone if successful, otherwise one of the system wide error codes.	
	*/
	virtual TInt MapPages(RPageArray::TIter aPages, TUint aMapInstanceCount) =0;

	/**
	Update the page table entries corresponding to this mapping to remove entries for
	a specified set of memory pages.

	This method is called by DMemoryObject::UnmapPages to update each mapping attached
	to a memory object whenever pages of memory are removed.

	@param aPages				An RPageArray::TIter which refers to a range of pages
								in a memory object. This has been clipped to fit within
								the range of pages mapped by this mapping.
								Only array entries which return true for
								RPageArray::TargetStateIsDecommitted should be unmapped
								from the mapping's page tables.

	@param aMapInstanceCount	The instance of this mapping which is to be updated.
								Whenever this no longer matches the current #MapInstanceCount
								the function must not update any more of the mapping's
								page table entries.
	*/
	virtual void UnmapPages(RPageArray::TIter aPages, TUint aMapInstanceCount) =0;

	/**
	Update the page table entry corresponding to this mapping to update an entry for a specified
	page that has just been moved or shadowed.

	@param aPages				The page array entry of the page in a memory object. 
								Only array entries which have a target state of 
								RPageArray::ECommitted should be mapped into the 
								mapping's page tables.

	@param aIndex				The index of the page in the memory object.

	@param aMapInstanceCount	The instance of this mapping which is to be updated.
								Whenever this no longer matches the current #MapInstanceCount
								the function must not update any more of the mapping's
								page table entries, (but must still return KErrNone).

	@param	aInvalidateTLB		Set to ETrue when the TLB entries associated with this page
								should be invalidated.  This must be done when there is 
								already a valid pte for this page, i.e. if the page is still 
								mapped.
	*/
	virtual void RemapPage(TPhysAddr& aPageArray, TUint aIndex, TUint aMapInstanceCount, TBool aInvalidateTLB)=0;

	/**
	Update the page table entries corresponding to this mapping to apply access restrictions
	to a specified set of memory pages.

	This method is called by DMemoryObject::RestrictPages to update each mapping attached
	to a memory object whenever pages of memory are restricted.

	@param aPages				An RPageArray::TIter which refers to a range of pages
								in a memory object. This has been clipped to fit within
								the range of pages mapped by this mapping.
								Only array entries which return true for
								RPageArray::TargetStateIsDecommitted should be unmapped
								from the mapping's page tables.

	@param aMapInstanceCount	The instance of this mapping which is to be updated.
								Whenever this no longer matches the current #MapInstanceCount
								the function must not update any more of the mapping's
								page table entries.
	*/
	virtual void RestrictPagesNA(RPageArray::TIter aPages, TUint aMapInstanceCount) =0;

	/**
	Update the page table entries corresponding to this mapping to add entries for
	a specified set of demand paged memory pages following a 'page in' or memory
	pinning operation.

	@param aPages				An RPageArray::TIter which refers to a range of pages
								in a memory object. This will be within the range of pages
								mapped by this mapping.
								Only array entries which have state RPageArray::ECommitted
								should be mapped into the mapping's page tables.

	@param aPinArgs				The resources required to pin any page tables the mapping uses.
								Page table must be pinned if \a aPinArgs.iPinnedPageTables is
								not the null pointer, in which case this the virtual address
								of the pinned must be stored in the array this points to.
								\a aPinArgs.iReadOnly is true if write access permissions
								are not needed.

	@return KErrNone if successful, otherwise one of the system wide error codes.	
	*/
	virtual TInt PageIn(RPageArray::TIter aPages, TPinArgs& aPinArgs, TUint aMapInstanceCount) =0;


	/**
	Update the page table entry corresponding to this mapping to add an entry for
	a specified page which is in the process of being moved.

	@param aPageArrayPtr		The page array entry for the page to be mapped which must be
								within this mapping range of pages.
								Only array entries which have a target state of
								RPageArray::ECommitted should be mapped into the mapping's 
								page tables.

	@param	aIndex				The index of the page.

	@return ETrue if successful, EFalse otherwise.
	*/
	virtual TBool MovingPageIn(TPhysAddr& aPageArrayPtr, TUint aIndex)=0;


	/**
	In debug builds, dump information about this mapping to the kernel trace port.
	*/
	virtual void Dump();

private:
	/**
	Update this mapping's MMU data structures to map all pages of memory
	currently committed to the memory object (#iMemory) in the region covered
	by this mapping.

	This method is called by #Attach after the mapping has been linked
	into the memory object.

	@return KErrNone if successful, otherwise one of the system wide error codes.
	*/
	virtual TInt DoMap() =0;

	/**
	Update this mapping's MMU data structures to unmap all pages of memory.

	This method is called by #Detach before the mapping has been unlinked
	from the memory object but after the #EDetaching flag has been set.
	*/
	virtual void DoUnmap() =0;

protected:
	/**
	For pinned mapping, this virtual method is called by #Attach in order to pin
	pages of memory if required. This is called after the mapping has been linked
	into the memory object but before #DoMap.

	The default implementation of this method simply calls DMemoryManager::Pin.

	@param aPinArgs	The resources to use for pinning. This has sufficient replacement
					pages allocated to pin every page the mapping covers, and the
					value of \a aPinArgs.iReadOnly has been set to correspond to the
					mappings access permissions.

	@return KErrNone if successful, otherwise one of the system wide error codes.
	*/
	virtual TInt DoPin(TPinArgs& aPinArgs);

	/**
	For pinned mapping, this virtual method is called by #Detach in order to unpin
	pages of memory if required. This is called before the mapping has been unlinked
	from the memory object but after #DoUnmap.

	The default implementation of this method simply calls DMemoryManager::Unpin.

	@param aPinArgs	The resources used for pinning. The replacement pages allocated
					to this will be increased for each page which was became completely
					unpinned.
	*/
	virtual void DoUnpin(TPinArgs& aPinArgs);
	};



/**
Base class for memory mappings which map memory contents into a address space.

This provides methods for allocating virtual memory and holds the attributes needed
for MMU page table entries.
*/
class DMemoryMapping : public DMemoryMappingBase
	{
protected:
	/**
	The page directory entry (PDE) value for use when mapping this mapping's page tables.
	This value has the physical address component being zero, so a page table's physical
	address can be simply ORed in.

	This could potentially be removed (see DMemoryMapping::PdeType()).
	*/
	TPde			iBlankPde;

	/**
	The page table entry (PTE) value for use when mapping pages into this mapping.
	This value has the physical address component being zero, so a page's physical
	address can be simply ORed in.
	*/
	TPte			iBlankPte;

	/**
	Start of the virtual address region allocated for use by this mapping
	ORed with the OS ASID of the address space this lies in.

	Note, the address at which memory is mapped (#iLinAddrAndOsAsid) may be different
	to this allocated address due to page colouring restrictions.

	@see iAllocatedSize
	*/
	TLinAddr		iAllocatedLinAddrAndOsAsid;

	/**
	Size of virtual address region memory allocated for use by this mapping.

	@see iAllocatedLinAddrAndOsAsid
	*/
	TUint			iAllocatedSize;

private:
	/**
	Start of the virtual address region that this mapping is currently
	mapping memory at, ORed with the OS ASID of the address space this lies in.

	This value is set by #Map which is called from #Attach when the mapping
	is attached to a memory object. The address used may be different to
	#iAllocatedLinAddrAndOsAsid due to page colouring restrictions.

	The size of the region mapped is #iSizeInPages.

	Note, access to this value is through #Base() and #OsAsid().
	*/
	TLinAddr		iLinAddrAndOsAsid;

public:
	/**
	Second phase constructor.

	The main function of this is to allocate a virtual address region for the mapping
	and to add it to an address space.

	@param aAttributes		The attributes of the memory which this mapping is intended to map.
							This is only needed to setup #PdeType which is required for correct
							virtual address allocation so in practice the only relevant attribute
							is to set EMemoryAttributeUseECC if required, else use
							EMemoryAttributeStandard.

	@param aFlags			A combination of the options from enum TMappingCreateFlags.

	@param aOsAsid			The OS ASID of the address space the mapping is to be added to.

	@param aAddr			The virtual address to use for the mapping, or zero if this is
							to be allocated by this function.

	@param aSize			The maximum size of memory, in bytes, this mapping will be used to
							map. This determines the size of the virtual address region the
							mapping will use.

	@param aColourOffset	The byte offset within a memory object's memory which this mapping
							is to start. This is used to adjust virtual memory allocation to
							meet page colouring restrictions. If this value is not known leave
							this argument unspecified; however, it must be specified if \a aAddr
							is specified.

	@return KErrNone if successful, otherwise one of the system wide error codes.	
	*/
	TInt Construct(TMemoryAttributes aAttributes, TMappingCreateFlags aFlags, TInt aOsAsid, TLinAddr aAddr, TUint aSize, TLinAddr aColourOffset=~(TLinAddr)0);

	/**
	Add this mapping to a memory object so that it maps a specified region of its memory.

	Most of the action of this method is performed by #Attach.

	@param aMemory		The memory object.
	@param aIndex		The page index of the first page of memory to be mapped by the mapping.
	@param aCount		The number of pages of memory to be mapped by the mapping.
	@param aPermissions	The memory access permissions to apply to the mapping.

	@return KErrNone if successful, otherwise one of the system wide error codes.
	*/
	TInt Map(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TMappingPermissions aPermissions);

	/**
	Remove this mapping from the memory object it was previously added to by #Map.

	Most of the action of this method is performed by #Detach.
	*/
	void Unmap();

	/**
	Return the OS ASID for the address space that this mapping is currently mapping memory in.
	*/
	FORCE_INLINE TInt OsAsid()
		{
		__NK_ASSERT_DEBUG(iLinAddrAndOsAsid); // check mapping has been added to an address space
		return iLinAddrAndOsAsid&KPageMask;
		}

	/**
	Return starting virtual address that this mapping is currently mapping memory at.
	The size of the region mapped is #iSizeInPages.
	*/
	FORCE_INLINE TLinAddr Base()
		{
		__NK_ASSERT_DEBUG(iLinAddrAndOsAsid); // check mapping has been added to an address space
		return iLinAddrAndOsAsid&~KPageMask;
		}

	/**
	Return #Base()|#OsAsid()
	*/
	FORCE_INLINE TLinAddr LinAddrAndOsAsid()
		{
		__NK_ASSERT_DEBUG(iLinAddrAndOsAsid); // check mapping has been added to an address space
		return iLinAddrAndOsAsid;
		}

	FORCE_INLINE TBool IsUserMapping()
		{
		// Note: must be usable before the mapping has been added to an address space
		return (PteType() & (EPteTypeUserAccess|EPteTypeGlobal)) == EPteTypeUserAccess;
		}

	/**
	Return #iBlankPde.
	*/
	FORCE_INLINE TPde BlankPde()
		{
		return iBlankPde;
		}

	/**
	Emit BTrace traces identifying this mappings virtual address usage.
	*/
	void BTraceCreate();

	/**
	In debug builds, dump information about this mapping to the kernel trace port.
	*/
	virtual void Dump();

	/**
	Function to return a page table pointer for the specified linear address and
	index to this mapping.

	This is called by #Epoc::MovePhysicalPage when moving page table or page table info pages.
	
	@param aLinAddr		The linear address to find the page table entry for.
	@param aMemoryIndex	The memory object index of the page to find the page 
						table entry for.
	
	@return A pointer to the page table entry, if the page table entry couldn't 
			be found this will be NULL
	*/
	virtual TPte* FindPageTable(TLinAddr aLinAddr, TUint aMemoryIndex)=0;

protected:
	/**
	@param aType Initial value for #Flags.
	*/
	DMemoryMapping(TUint aType);

	/**
	This destructor removes the mapping from any address space it was added to and
	frees any virtual addresses allocated to it.
	*/
	~DMemoryMapping();

	/**
	Free any resources owned by this mapping, i.e. allow Construct() to be used
	on this mapping at a new address etc.
	*/
	void Destruct();

	/**
	Allocatate virtual addresses for this mapping to use.
	This is called from #Construct and the arguments to this function are the same.

	On success, iAllocatedLinAddrAndOsAsid and iAllocatedSize will be initialised.
	*/
	virtual TInt AllocateVirtualMemory(TMappingCreateFlags aFlags, TInt aOsAsid, TLinAddr aAddr, TUint aSize, TLinAddr aColourOffset);

	/**
	Free the virtual addresses allocated to this mapping with AllocateVirtualMemory.
	*/
	virtual void FreeVirtualMemory();
	};



/**
A memory mapping to map a 'chunk' aligned region of a DCoarseMemory object into
an address space. A 'chunk' is the size of memory mapped by a whole MMU page table
and is #KChunkSize bytes.

These mappings make use of page tables owned by a DCoarseMemory and when
they are attached to a memory object they are linked into
DCoarseMemory::DPageTables::iMappings not DCoarseMemory::iMappings.
*/
class DCoarseMapping : public DMemoryMapping
	{
public:
	DCoarseMapping();
	~DCoarseMapping();

protected:
	DCoarseMapping(TUint aFlags);
	
protected:
	// from DMemoryMappingBase...
	virtual TInt MapPages(RPageArray::TIter aPages, TUint aMapInstanceCount); ///< Not implemented. Faults in debug builds.
	virtual void UnmapPages(RPageArray::TIter aPages, TUint aMapInstanceCount); ///< Not implemented. Faults in debug builds.
	virtual void RemapPage(TPhysAddr& aPageArray, TUint aIndex, TUint aMapInstanceCount, TBool aInvalidateTLB); ///< Not implemented. Faults in debug builds.
	virtual void RestrictPagesNA(RPageArray::TIter aPages, TUint aMapInstanceCount); ///< Not implemented. Faults in debug builds.
	virtual TInt PageIn(RPageArray::TIter aPages, TPinArgs& aPinArgs, TUint aMapInstanceCount);
	virtual TBool MovingPageIn(TPhysAddr& aPageArrayPtr, TUint aIndex);
	virtual TInt DoMap();
	virtual void DoUnmap();
	
	// from DMemoryMapping...
	virtual TPte* FindPageTable(TLinAddr aLinAddr, TUint aMemoryIndex);
	};



/**
A memory mapping to map a page aligned region of a memory object into
an address space. The may be used with any memory object: DFineMemory or DCoarseMemory.
*/
class DFineMapping : public DMemoryMapping
	{
public:
	DFineMapping();
	~DFineMapping();

private:
	// from DMemoryMappingBase...
	virtual TInt MapPages(RPageArray::TIter aPages, TUint aMapInstanceCount);
	virtual void UnmapPages(RPageArray::TIter aPages, TUint aMapInstanceCount);
	virtual void RemapPage(TPhysAddr& aPageArray, TUint aIndex, TUint aMapInstanceCount, TBool aInvalidateTLB);
	virtual void RestrictPagesNA(RPageArray::TIter aPages, TUint aMapInstanceCount);
	virtual TInt PageIn(RPageArray::TIter aPages, TPinArgs& aPinArgs, TUint aMapInstanceCount);
	virtual TBool MovingPageIn(TPhysAddr& aPageArrayPtr, TUint aIndex);
	virtual TInt DoMap();
	virtual void DoUnmap();

	// from DMemoryMapping...

	/**
	Allocatate virtual addresses for this mapping to use.

	In addition to performing the action of DMemoryMapping::AllocateVirtualMemory
	this will also allocate all permanent page tables for the mapping if it has attribute
	#EPermanentPageTables.
	*/
	virtual TInt AllocateVirtualMemory(TMappingCreateFlags aFlags, TInt aOsAsid, TLinAddr aAddr, TUint aSize, TLinAddr aColourOffset);

	/**
	Free the virtual addresses and permanent page tables allocated to this mapping with
	AllocateVirtualMemory.
	*/
	virtual void FreeVirtualMemory();

	virtual TPte* FindPageTable(TLinAddr aLinAddr, TUint aMemoryIndex);

	// new...

	/**
	Allocate all the page tables required for this mapping. This is called by
	AllocateVirtualMemory if the #EPermanentPageTables attribute is set.

	Each page table for the virtual address region used by the mapping is
	allocated if not already present. The permanence count of any page table
	(SPageTableInfo::iPermanenceCount) is then incremented so that it is not
	freed even when it no longer maps any pages.

	If successful, the #EPageTablesAllocated flag in #Flags will be set.

	@return KErrNone if successful, otherwise one of the system wide error codes.
	*/
	TInt AllocatePermanentPageTables();

	/**
	Free all permanent page tables allocated to this mapping.

	This reverses the action of #AllocatePermanentPageTables by decrementing
	the permanence count for each page table and freeing it if is no longer in use.
	*/
	void FreePermanentPageTables();

	/**
	Free a range of permanent page tables.

	This is an implementation factor for FreePermanentPageTables and
	AllocatePermanentPageTables. It decrements the permanence count
	for each page table and frees it if is no longer in use

	@param aFirstPde	The address of the page directory entry which refers to
						the first page table to be freed.
	@param aLastPde		The address of the page directory entry which refers to
						the last page table to be freed.
	*/
	void FreePermanentPageTables(TPde* aFirstPde, TPde* aLastPde);

#ifdef _DEBUG
	/**
	Validate the contents of the page table are valid.

	@param aPt	The page table to validate.
	*/
	void ValidatePageTable(TPte* aPt, TLinAddr aAddr);
#endif

	/**
	Get the page table being used to map a specified virtual address if it exists.

	@param aAddr	A virtual address in the region allocated to this mapping.

	@return The virtual address of the page table mapping \a aAddr,
			or the null pointer if one wasn't found.
	*/
	TPte* GetPageTable(TLinAddr aAddr);

	/**
	Get the page table being used to map a specified virtual address; allocating
	a new one if it didn't previously exist.

	@param aAddr	A virtual address in the region allocated to this mapping.

	@return The virtual address of the page table mapping \a aAddr,
			or the null pointer if one wasn't found and couldn't be allocated.
	*/
	TPte* GetOrAllocatePageTable(TLinAddr aAddr);

	/**
	Get and pin the page table being used to map a specified virtual address;
	allocating a new one if it didn't previously exist.

	@param aAddr	A virtual address in the region allocated to this mapping.
	@param aPinArgs	The resources required to pin the page table.
					On success, the page table will have been appended to
					\a aPinArgs.iPinnedPageTables.

	@return The virtual address of the page table mapping \a aAddr,
			or the null pointer if one wasn't found and couldn't be allocated.
	*/
	TPte* GetOrAllocatePageTable(TLinAddr aAddr, TPinArgs& aPinArgs);

	/**
	Allocate a single page table.

	@param aAddr		The virtual address the page table will be used to map.
	@param aPdeAddress	Address of the page directory entry which is to map
						the newly allocated page table.
	@param aPermanent	True, if the page table's permanence count is to be incremented.

	@return The virtual address of the page table if it was successfully allocated,
			otherwise the null pointer.
	*/
	TPte* AllocatePageTable(TLinAddr aAddr, TPde* aPdeAddress, TBool aPermanent=false);

	/**
	Free a single page table if it is unused.

	@param aPdeAddress	Address of the page directory entry (PDE) which maps the page table.
						If the page table is freed, this PDE will be set to an 'unallocated' value.
	*/
	void FreePageTable(TPde* aPdeAddress);
	};


/**
A mapping which maps any memory into the kernel address space and provides access to 
the physical address used by a memory object.

These mappings are always of the 'pinned' type to prevent the obtained physical addresses
from becoming invalid.
*/
class DKernelPinMapping : public DFineMapping
	{
public:
	DKernelPinMapping();
	TInt Construct(TUint aReserveSize);
	TInt MapAndPin(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TMappingPermissions aPermissions);
	void UnmapAndUnpin();

public:
	TInt iReservePages;		///< The number of pages this mapping is able to map with its reserved resources(page tables etc).
	};


/**
A mapping which provides access to the physical address used by a memory object
without mapping these at any virtual address accessible to software.

These mappings are always of the 'pinned' type to prevent the obtained physical addresses
from becoming invalid.
*/
class DPhysicalPinMapping : public DMemoryMappingBase
	{
public:
	DPhysicalPinMapping();

	/**
	Attach this mapping to a memory object so that it pins a specified region of its memory.

	Most of the action of this method is performed by #Attach.

	@param aMemory		The memory object.
	@param aIndex		The page index of the first page of memory to be pinned by the mapping.
	@param aCount		The number of pages of memory to be pinned by the mapping.
	@param aPermissions	The memory access permissions appropriate to the intended use
						of the physical addresses. E.g. if the memory contents will be
						changes, use EReadWrite. These permissions are used for error
						checking, e.g. detecting attempted writes to read-only memory.
						They are also used for optimising access to demand paged memory;
						which is more efficient if only read-only access is required.

	@return KErrNone if successful,
			KErrNotFound if any part of the memory to be pinned was not present,
			KErrNoMemory if there was insufficient memory,
			otherwise one of the system wide error codes.
	*/
	TInt Pin(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TMappingPermissions aPermissions);

	/**
	Remove this mapping from the memory object it was previously added to by #Pin.

	Most of the action of this method is performed by #Detach.
	*/
	virtual void Unpin();

private:
	// from DMemoryMappingBase...
	virtual TInt MapPages(RPageArray::TIter aPages, TUint aMapInstanceCount); ///< Not implemented. Faults in debug builds.
	virtual void UnmapPages(RPageArray::TIter aPages, TUint aMapInstanceCount); ///< Does nothing
	virtual void RemapPage(TPhysAddr& aPageArray, TUint aIndex, TUint aMapInstanceCount, TBool aInvalidateTLB); ///< Not implemented. Faults in debug builds.
	virtual void RestrictPagesNA(RPageArray::TIter aPages, TUint aMapInstanceCount); ///< Does nothing
	virtual TInt PageIn(RPageArray::TIter aPages, TPinArgs& aPinArgs, TUint aMapInstanceCount); ///< Does nothing
	virtual TBool MovingPageIn(TPhysAddr& aPageArrayPtr, TUint aIndex);///< Not implemented. Faults in debug builds.
	virtual TInt DoMap(); ///< Does nothing
	virtual void DoUnmap(); ///< Does nothing
	}; 



/**
A mapping which pins memory in order to prevent demand paging related
page faults from occurring.
*/
class DVirtualPinMapping : public DPhysicalPinMapping
	{
public:
	DVirtualPinMapping();
	~DVirtualPinMapping();

	/**
	Create a new DVirtualPinMapping object suitable for pinning a specified number of pages.

	If no maximum is specified (\a aMaxCount==0) then this object may be used to pin
	any number of pages, however this will require dynamic allocation of storage for
	page table references.

	@param aMaxCount The maximum number of pages which can be pinned, or zero for no maximum.

	@return The newly created DVirtualPinMapping or the null pointer if there was
			insufficient memory.
	*/
	static DVirtualPinMapping* New(TUint aMaxCount);

	/**
	Attach this mapping to a memory object so that it pins a specified region of its memory.

	Additionally, pin the page tables in a specified mapping (\a aMapping) which
	are being used to map these pages.

	The result of this function is that access to the pinned memory through the virtual
	addresses used by \a aMapping will not generate any demand paging related page faults.

	@param aMemory		The memory object.
	@param aIndex		The page index of the first page of memory to be pinned by the mapping.
	@param aCount		The number of pages of memory to be pinned by the mapping.
	@param aPermissions	The memory access permissions appropriate to the intended use
						of the physical addresses. E.g. if the memory contents will be
						changes, use EReadWrite. These permissions are used for error
						checking, e.g. detecting attempted writes to read-only memory.
						They are also used for optimising access to demand paged memory;
						which is more efficient if only read-only access is required.
	@param aMapping		The mapping whose page tables are to be pinned. This must be
						currently mapping the specified region of memory pages.
	@param aMapInstanceCount	The instance count of the mapping who's page tables are to be pinned.

	@return KErrNone if successful,
			KErrNotFound if any part of the memory to be pinned was not present,
			KErrNoMemory if there was insufficient memory,
			otherwise one of the system wide error codes.
	*/
	TInt Pin(	DMemoryObject* aMemory, TUint aIndex, TUint aCount, TMappingPermissions aPermissions, 
				DMemoryMappingBase* aMapping, TUint aMapInstanceCount);

	/**
	Remove this mapping from the memory object it was previously added to by #Pin.
	This will unpin any memory pages and pages tables that were pinned.
	*/
	void Unpin();

	/**
	Return the maximum number of page tables which could be required to map
	\a aPageCount pages. This is used by various resource reserving calculations.
	*/
	static TUint MaxPageTables(TUint aPageCount);

	/**
	In debug builds, dump information about this mapping to the kernel trace port.
	*/
	virtual void Dump();

private:
	// from DMemoryMappingBase...
	virtual void RemapPage(TPhysAddr& aPageArray, TUint aIndex, TUint aMapInstanceCount, TBool aInvalidateTLB); ///< Does nothing.
	virtual TInt PageIn(RPageArray::TIter aPages, TPinArgs& aPinArgs, TUint aMapInstanceCount);
	virtual TBool MovingPageIn(TPhysAddr& aPageArrayPtr, TUint aIndex);///< Not implemented. Faults in debug builds.
	virtual TInt DoPin(TPinArgs& aPinArgs);
	virtual void DoUnpin(TPinArgs& aPinArgs);

private:
	/**
	Allocate memory to store pointers to all the page table which map
	\a aCount pages of memory. The pointer to the allocated memory
	is stored at iAllocatedPinnedPageTables.

	If iSmallPinnedPageTablesArray is large enough, this function doesn't
	allocate any memory.

	@return KErrNone if successful, otherwise KErrNoMemory.
	*/
	TInt AllocPageTableArray(TUint aCount);

	/**
	Delete iAllocatedPinnedPageTables.
	*/
	void FreePageTableArray();

	/**
	Return the address of the array storing pinned page tables.
	This is either iSmallPinnedPageTablesArray or iAllocatedPinnedPageTables.
	*/
	TPte** PageTableArray();

	/**
	Unpin all the page tables which have been pinned by this mapping.

	@param aPinArgs	The resources used for pinning. The replacement pages allocated
					to this will be increased for each page which was became completely
					unpinned.
	*/
	void UnpinPageTables(TPinArgs& aPinArgs);
private:
	/**
	Temporary store for the mapping passed to #Pin
	*/
	DMemoryMappingBase* iPinVirtualMapping;

	/**
	Temporary store for the mapping instance count passed to #Pin
	*/
	TUint iPinVirtualMapInstanceCount;

	/**
	The number of page tables which are currently being pinned by this mapping.
	This is the number of valid entries stored at PageTableArray.
	*/
	TUint iNumPinnedPageTables;

	/**
	The maximum number of pages which can be pinned by this mapping.
	If this is zero, there is no maximum.
	*/
	TUint iMaxCount;

	/**
	The memory allocated by this object for storing pointer to the page tables
	it has pinned.
	*/
	TPte** iAllocatedPinnedPageTables;

	enum
		{
		KSmallPinnedPageTableCount = 2 ///< Number of entries in iSmallPinnedPageTablesArray
		};

	/**
	A small array to use for storing pinned page tables.
	This is an optimisation used for the typical case of pinning a small number of pages
	to avoid dynamic allocation of memory.
	*/
	TPte* iSmallPinnedPageTablesArray[KSmallPinnedPageTableCount];
	}; 

#endif
