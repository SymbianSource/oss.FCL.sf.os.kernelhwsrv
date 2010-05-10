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

#ifndef MMANAGER_H
#define MMANAGER_H

#include "mpagearray.h"


class DMemoryObject;
class DMemoryMappingBase;
class DPageReadRequest;
class DPageWriteRequest;

/**
An abstract interface for performing operations on a memory object, such as allocating
or freeing memory.

There are different concrete implementations of this class for memory being managed in
different ways, e.g. demand paged versus unpaged memory.

Any particular instance of a manager will only support a subset of the methods provided.
The default implementations of these in this base class return KErrErrNotSupported.
*/
class DMemoryManager : public DBase
	{
public:
	/**
	Create a new memory object for use with this manager.

	@param[out]	aMemory	On success this is set to the address of the created memory object.
	@param aSizeInPages	Size of the memory object, in number of pages.
	@param aAttributes	Bitmask of values from enum #TMemoryAttributes.
	@param aCreateFlags	Bitmask of option flags from enum #TMemoryCreateFlags.

	@return KErrNone if successful, otherwise one of the system wide error codes.
	*/
	virtual TInt New(DMemoryObject*& aMemory, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags);

	/**
	Remove all memory from a memory object and close a reference on it.

	If there are no longer any mappings to the memory, or other references,
	this will result in the memory object being destroyed. However whilst
	other references exist, cleanup of memory and other resources may be
	delayed indefinitely.

	@param aMemory		A memory object associated with this manager.
	*/
	virtual void Destruct(DMemoryObject* aMemory) =0;

	/**
	Allocate memory resources for a specified region of a memory object.

	Depending on the manager, this may involve allocating physical RAM pages or
	reserving space in a backing store.

	@param aMemory		A memory object associated with this manager.
	@param aIndex		Page index for the start of the region.
	@param aCount		Number of pages in the region.

	@return KErrNone if successful,
			KErrAlreadyExists if any part of the region was already allocated,
			KErrNotSupported if the manager doesn't support this function,
			otherwise one of the system wide error codes.
	*/
	virtual TInt Alloc(DMemoryObject* aMemory, TUint aIndex, TUint aCount);

	/**
	Allocate physically contiguous RAM for a specified region of a memory object.

	Important note, this function can unexpectedly fail with KErrAlreadyExists
	if any part of the the region previously contained allocated memory which
	had been freed but which was pinned. It is therefore not suitable for general
	use.

	@param aMemory			A memory object associated with this manager.
	@param aIndex			Page index for the start of the region.
	@param aCount			Number of pages in the region.
	@param aAlign			Log2 of the alignment (in bytes) that the address of the
							allocated physical RAM must have.
	@param[out] aPhysAddr	On success, this is set to the start address of the
							allocated physical RAM.

	@return KErrNone if successful,
			KErrAlreadyExists if any part of the region was already allocated,
			KErrNotSupported if the manager doesn't support this function,
			otherwise one of the system wide error codes.
	*/
	virtual TInt AllocContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TUint aAlign, TPhysAddr& aPhysAddr);

	/**
	Free any memory resources used for a specified region of a memory object.
	This is the inverse operation to #Alloc and #AllocContiguous.

	Depending on the manager, this may involve freeing space in a backing store
	as well as freeing any currently committed RAM.

	If part of the memory is currently pinned then resource freeing may be delayed
	indefinitely.

	@param aMemory		A memory object associated with this manager.
	@param aIndex		Page index for the start of the region.
	@param aCount		Number of pages in the region.
	*/
	virtual void Free(DMemoryObject* aMemory, TUint aIndex, TUint aCount);

	/**
	Wipe the entire memory contents of the memory object so they are in the
	same state as if the memory had been newly allocated with #Alloc.

	This doesn't allocate any new memory, just fills the existing contents
	with the appropriate wipe byte which is used with the memory object.

	@param aMemory	A memory object associated with this manager.

	@see EMemoryCreateUseCustomWipeByte
	@see EMemoryCreateNoWipe

	@return KErrNone, if successful;
			otherwise KErrNotSupported, to indicate the memory object doesn't support this operation.
	*/
	virtual TInt Wipe(DMemoryObject* aMemory);

	/**
	Add a specified set of physical memory pages to a region of a memory object.

	@param aMemory		A memory object associated with this manager.
	@param aIndex		Page index for the start of the region.
	@param aCount		Number of pages in the region.
	@param aPages		Pointer to array of pages to add. This must contain \a aCount
						number of physical page addresses which are each page aligned.

	@return KErrNone if successful,
			KErrAlreadyExists if any part of the region was already allocated,
			KErrNotSupported if the manager doesn't support this function,
			otherwise one of the system wide error codes.
	*/
	virtual TInt AddPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, const TPhysAddr* aPages);

	/**
	Add a contiguous range of physical memory pages to a region of a memory object.

	@param aMemory		A memory object associated with this manager.
	@param aIndex		Page index for the start of the region.
	@param aCount		Number of pages in the region.
	@param aPhysAddr	The page aligned start address of the pages to be added.

	@return KErrNone if successful,
			KErrAlreadyExists if any part of the region was already allocated,
			KErrNotSupported if the manager doesn't support this function,
			otherwise one of the system wide error codes.
	*/
	virtual TInt AddContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr aPhysAddr);

	/**
	Remove the memory pages from a specified region of a memory object.
	This is the inverse operation to #AddPages and #AddContiguous.

	@param aMemory		A memory object associated with this manager.
	@param aIndex		Page index for the start of the region.
	@param aCount		Number of pages in the region.
	@param[out] aPages	Pointer to an array of physical addresses which has a
						length of \a aCount. The contents of this will be set to
						the addresses of the pages which were removed by this
						function. The number of valid entries in this array is
						given by the return value of this function.
						aPages may be the null-pointer, to indicate that these
						page addresses aren't required by the caller.

	@return The number of pages successfully removed from the memory object.
			This gives the number of valid entries in the array \a aPages and is
			less-than or equal to \a aCount.
	*/
	virtual TInt RemovePages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr* aPages);

	/**
	Mark the specified region of a memory object as discardable.
	The system may remove discardable pages from the memory object at any time,
	to be reused for other purposes.

	@param aMemory		A memory object associated with this manager.
	@param aIndex		Page index for the start of the region.
	@param aCount		Number of pages in the region.

	@return KErrNone if successful,
			KErrNotSupported if the manager doesn't support this function,
			otherwise one of the system wide error codes.
	*/
	virtual TInt AllowDiscard(DMemoryObject* aMemory, TUint aIndex, TUint aCount);

	/**
	Mark the specified region of a memory object as no longer discardable.
	This undoes the operation of #AllowDiscard.

	If any pages in the region are no longer present, then the operation will
	fail with KErrNotFound. In this case, the state of the pages in the region
	is indeterminate; they may be either still discardable, not discardable, or
	not present.

	@param aMemory		A memory object associated with this manager.
	@param aIndex		Page index for the start of the region.
	@param aCount		Number of pages in the region.

	@return KErrNone if successful,
			KErrNotFound if any page in the region was no longer present,
			KErrNotSupported if the manager doesn't support this function,
			otherwise one of the system wide error codes.
	*/
	virtual TInt DisallowDiscard(DMemoryObject* aMemory, TUint aIndex, TUint aCount);

	/**
	Remove a page of RAM from a memory object.

	This is only intended for use by #DPager::StealPage when it removes a page
	from the demand paging live list.

	@param aMemory		A memory object associated with this manager.
	@param aPageInfo	The page information structure of the page to be stolen.
						This must be owned by \a aMemory.

	@return KErrNone if successful,
			KErrInUse if the page became pinned or was subject to a page fault,
			KErrNotSupported if the manager doesn't support this function,
			otherwise one of the system wide error codes.

 	@pre RamAllocLock held.
	@pre If the page is dirty the PageCleaning lock must be held.
	@pre MmuLock held.
	*/
	virtual TInt StealPage(DMemoryObject* aMemory, SPageInfo* aPageInfo);

	/**
	Restrict the access permissions for a page of RAM.

	This is only intended for use by #DPager::RestrictPage when it restricts
	access to a page in the demand paging live list.

	@param aMemory		A memory object associated with this manager.
	@param aPageInfo	The page information structure of the page to be restricted.
						This must be owned by \a aMemory.
	@param aRestriction	The restriction type to apply.

	@return KErrNone if successful,
			KErrInUse if the page state changed, e.g. became pinned or was subject to a page fault,
			KErrNotSupported if the manager doesn't support this function,
			otherwise one of the system wide error codes.
	*/
	virtual TInt RestrictPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TRestrictPagesType aRestriction);

	/**
	Clean multiple pages of RAM by saving any modifications to it out to backing store.

	This function must only be called when there are no writable MMU mappings of the pages.

	The function takes an array of SPageInfo pointers indicating the pages to clean.  On return, the
	elements of this array are unchanged if the page was successfully cleaned, or set to NULL if
	cleaning was abandoned (by the page being written to, for example).

	The pages passed must be sequential in their page colour (index & KPageColourMask).

	Those pages that are successfully cleaned are marked as clean using SPageInfo::SetClean.

	This is intended for use by #StealPage and #CleanSomePages.

	@param aPageCount		Number of pages to clean.
	@param aPageInfos		Pointer to an array of aPageCount page info pointers.
	@param aBackground      Whether the activity should be ignored when determining whether the
	                        paging device is busy.

	@pre MmuLock held
	@pre PageCleaning lock held
	@pre The memory page must not have any writeable MMU mappings.
	@post MmuLock held (but may have been released by this function)
	*/
	virtual void CleanPages(TUint aPageCount, SPageInfo** aPageInfos, TBool aBackground);

	/**
	Process a page fault in memory associated with this manager.

	This is only intended for use by #DPager::HandlePageFault.

	@param aMemory				A memory object associated with this manager whose memory was
								accessed by the page fault.
	@param aIndex				Page index, within the memory, at which the page fault occurred.
	@param aMapping				The memory mapping in which the page fault occurred.
	@param aMapInstanceCount	The instance count of the mapping that took the page fault.
	@param aAccessPermissions	Flags from enum #TMappingPermissions indicating the memory
								access permissions used by the page fault. E.g. the #EReadWrite
								flag will be set for a write access.

	@return KErrNone if the fault was handled successfully and the running program should
			restart at the faulting instruction.
			Otherwise, one of the system wide error codes indicating that the program generating
			the page fault attempted an invalid memory access.
	*/
	virtual TInt HandleFault(	DMemoryObject* aMemory, TUint aIndex, DMemoryMapping* aMapping, 
								TUint aMapInstanceCount, TUint aAccessPermissions);

	/**
	Pin the region of a memory object covered by a specified memory mapping.

	This function should ensure that the memory pages covered by the mapping are
	present in the memory object and will not be removed again until an #Unpin
	operation is issued. Additionally, for demand paged memory, the pages
	must be mapped into \a aMapping using DMemoryMappingBase::PageIn.

	This function is only intended to be called via DMemoryMappingBase::DoPin
	which is itself called from DMemoryMappingBase::Attach.

	@param aMemory	A memory object associated with this manager.
	@param aMapping	A mapping with the DMemoryMappingBase::EPinned attribute which
					has been attached to \a aMemory.
	@param aPinArgs	The resources to use for pinning. This must have sufficient replacement
					pages allocated to pin every page the mapping covers. Also, the
					value of \a aPinArgs.iReadOnly must be set to correspond to the
					mappings access permissions.

	@return KErrNone if successful,
			KErrNotFound if any part of the memory to be pinned was not present,
			otherwise one of the system wide error codes.
	*/
	virtual TInt Pin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs) =0;

	/**
	Unpin the region of a memory object covered by a specified memory mapping.

	This reverses the action of #Pin and is only intended to be called from
	DMemoryMappingBase::Detach.

	Note, pinning is a reference counting operation, therefore pages must stay
	resident in memory (pinned) until the number of unpinning operations affecting
	them equals the number of pinning operations.

	@param aMemory	A memory object associated with this manager.
	@param aMapping	A mapping with the DMemoryMappingBase::EPinned attribute which
					has been attached to \a aMemory.
	@param aPinArgs	The resources used for pinning. The replacement pages allocated
					to this will be increased for each page which was became completely
					unpinned.
	*/
	virtual void Unpin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs) =0;

	/**
	Attempt to move the page specified to a new physical location.  The new physical
	location for the page to be moved to is allocated by this method.  However,
	aBlockZoneId and aBlockRest can be used to control which RAM zone the new
	location is in.

	@param aMemory		The memory object that owns the page.
	@param aOldPageInfo	The page info for the physical page to move.
	@param aNewPage 	On success this will hold the physical address of the new 
						location for the page.
	@param aBlockZoneId The ID of a RAM zone not to allocate the new page into.
	@param aBlockRest 	When set to ETrue the search for a new page will stop if it 
						ever needs to look at aBlockZoneId.
	@return KErrNone on success, KErrInUse if the page couldn't be moved, 
			or KErrNoMemory if it wasn't possible to allocate a new page.
	*/
	virtual TInt MovePage(DMemoryObject* aMemory, SPageInfo* aOldPageInfo, TPhysAddr& aNewPage, TUint aBlockZoneId, TBool aBlockRest);

	/**
	Move the page specified to a new physical location and mark the page as 
	allocated as type aPageType.
	
	@param aMemory		The memory object that owns the page.
	@param aPageInfo	The page info for the physical page to move.
	@param aPageType	The type of the page to allocate into the orignal physical 
						location of the page to move.
	@return KErrNone on success, KErrInUse if the page couldn't be moved, 
			or KErrNoMemory if it wasn't possible to allocate a new page.
	*/
	virtual TInt MoveAndAllocPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TZonePageType aPageType);

	/**
	Return the TZonePageType of the pages that the memory manager can allocate and free.
	*/
	virtual TZonePageType PageType();

	/**
	Bitmask representing the different cleanup operations which can be performed
	on a memory object. The operations are queued with #QueueCleanup and
	pending operations are stored in each memory object's DMemoryObject::iCleanupFlags.
	*/
	enum TCleanupOperationFlag
		{
		/**
		Execute #DoCleanupDecommitted.
		*/
		ECleanupDecommitted	= 1<<0,

		/**
		Internal flag which indicates that cleanup has been queued.
		*/
		ECleanupIsQueued	= 1u<<31
		};

	/**
	Queue a cleanup operation to run for a specified memory object.

	The operations are run from #CleanupFunction which is called
	from a #TMemoryCleanup callback.

	@param aMemory		The memory object.
	@param aCleanupOp	The operation to perform.
	*/
	static void QueueCleanup(DMemoryObject* aMemory, TCleanupOperationFlag aCleanupOp);

protected:

	/**
	Unmap and free the RAM pages used for a specified region of a memory object.

	Successfully freed pages are returned to the system's free RAM pool.
	However, pinned pages are placed in the decommitted state
	(RPageArray::EDecommitted) and remain allocated to the memory object.
	These decommitted pages may ultimately get freed by #FreeDecommitted
	or they may become re-allocated with #ReAllocDecommitted.

	@param aMemory		A memory object associated with this manager.
	@param aIndex		Page index for the start of the region.
	@param aCount		Number of pages in the region.
	*/
	static void DoFree(DMemoryObject* aMemory, TUint aIndex, TUint aCount);

	/**
	Re-allocate all decommitted but still present RAM pages in a region of a memory object.

	When pinned memory is freed from a memory object it is placed in the
	RPageArray::EDecommitted state. This function is called by #Alloc to place
	all such memory back into the committed state as if it had been newly allocated.

	@param aMemory		A memory object associated with this manager.
	@param aIndex		Page index for the start of the region.
	@param aCount		Number of pages in the region.
	*/
	static void ReAllocDecommitted(DMemoryObject* aMemory, TUint aIndex, TUint aCount);

	/**
	Attempt to free all decommitted but still present RAM pages in a region of a memory object.

	When pinned memory is freed from a memory object it is placed in the
	RPageArray::EDecommitted state. This function is called by #DoCleanupDecommitted
	to attempt to fully free this memory if it is no longer pinned.

	@param aMemory		A memory object associated with this manager.
	@param aIndex		Page index for the start of the region.
	@param aCount		Number of pages in the region.
	*/
	static void FreeDecommitted(DMemoryObject* aMemory, TUint aIndex, TUint aCount);

	/**
	Implementation factor for #FreeDecommitted and #DoFree.
	*/
	static TInt FreePages(DMemoryObject* aMemory, RPageArray::TIter aPageList);

	/**
	Cleanup any decommitted memory associated with a memory object.

	This is called for memory objects which were queued for cleanup (#QueueCleanup)
	with the #ECleanupDecommitted operation. A manager class must override this method
	if it triggers this cleanup type - the default implementation faults in debug builds.

	@param aMemory The memory object requiring cleanup.
	*/
	virtual void DoCleanupDecommitted(DMemoryObject* aMemory);

private:
	/**
	Callback function which executes all pending operations queued with #QueueCleanup.
	*/
	static void CleanupFunction(TAny*);

	/**
	Head of a singly linked list of memory objects which require a cleanup operation.
	Each object in the list is linked using its DMemoryObject::iCleanupNext member.
	@see #QueueCleanup
	*/
	static DMemoryObject* iCleanupHead;

	/**
	Spinlock used during memory cleanup operations to protect object list (#iCleanupHead
	and DMemoryObject::iCleanupNext) and operation flags (DMemoryObject::iCleanupFlags).
	*/
	static TSpinLock iCleanupLock;
	};



/**
The bass class for memory managers implementing the different forms of demand paging.
The provides the common functions required to 'page in' and 'page out' memory.
*/
class DPagedMemoryManager : public DMemoryManager
	{
public:
	// from DMemoryManager...
	virtual TInt New(DMemoryObject*& aMemory, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags);
	virtual void Destruct(DMemoryObject* aMemory);
	virtual TInt MovePage(DMemoryObject* aMemory, SPageInfo* aOldPageInfo, TPhysAddr& aNewPage, TUint aBlockZoneId, TBool aBlockRest);
	virtual TInt StealPage(DMemoryObject* aMemory, SPageInfo* aPageInfo);
	virtual TInt RestrictPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TRestrictPagesType aRestriction);
	virtual TInt HandleFault(	DMemoryObject* aMemory, TUint aIndex, DMemoryMapping* aMapping, 
								TUint aMapInstanceCount, TUint aAccessPermissions);
	virtual TInt Pin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs);
	virtual void Unpin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs);

	/**
	Called by DPager::Init3 during third stage initialisation.
	*/
	virtual void Init3() = 0;

	/**
	Called by Kern::InstallPagingDevice to notify memory managers when a paging device
	is installed. A manager requires use of these paging devices to access storage media
	where demand paged content is stored.
	*/
	virtual TInt InstallPagingDevice(DPagingDevice* aDevice) = 0;

protected:

	/**
	Acquire a request object suitable for issuing to #ReadPages to
	obtain the data content of a specified region of a memory object.

	Once the request object is finished with it must be released (DPagingRequest::Release).

	Typically this function is implemented by calling DPagingRequestPool::AcquirePageReadRequest
	on the request pool of the paging device appropriate to the memory object.

	@param[out] aRequest	On success this is set to the address of the request object.
	@param aMemory			A memory object associated with this manager.
	@param aIndex			Page index for the start of the region.
	@param aCount			Number of pages in the region.

	@return KErrNone if successful,
			otherwise one of the system wide error codes.
	*/
	virtual TInt AcquirePageReadRequest(DPageReadRequest*& aRequest, DMemoryObject* aMemory, TUint aIndex, TUint aCount) = 0;

	/**
	Acquire a request object suitable for issuing to #WritePages to
	save the data content of a specified region of a memory object.

	Once the request object is finished with it must be released (DPagingRequest::Release).

	Typically this function is implemented by calling DPagingRequestPool::AcquirePageWriteRequest
	on the request pool of the paging device appropriate to the memory object.

	@param[out] aRequest	On success this is set to the address of the request object.
	@param aMemory			A memory object associated with this manager.
	@param aIndex			Page index for the start of the region.
	@param aCount			Number of pages in the region.

	@return KErrNone if successful,
			otherwise one of the system wide error codes.
	*/
	virtual TInt AcquirePageWriteRequest(DPageWriteRequest*& aRequest, DMemoryObject* aMemory, TUint aIndex, TUint aCount);

	/**
	Obtain the data content of a specified region of a memory object by reading it from
	storage media.

	The memory region must be the same as, or a subset of, the region used when obtaining
	the request object \a aRequest.

	@param aMemory	A memory object associated with this manager.
	@param aIndex	Page index for the start of the region.
	@param aCount	Number of pages in the region.
	@param aPages	Pointer to array of pages to read into. This must contain \a aCount
					number of physical page addresses which are each page aligned.
	@param aRequest	A request object previously obtained with #AcquirePageReadRequest.

	@return KErrNone if successful,
			otherwise one of the system wide error codes.
	*/
	virtual TInt ReadPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr* aPages, DPageReadRequest* aRequest) =0;

	/**
	Check if a region of a memory object has been allocated. E.g. that #Alloc
	has reserved backing store for the memory and this has has not yet been freed
	by #Free or #Destruct.

	@param aMemory	A memory object associated with this manager.
	@param aIndex	Page index for the start of the region.
	@param aCount	Number of pages in the region.

	@return True if the whole region has allocated storage, false otherwise.

	@pre #MmuLock held.
	@post #MmuLock held and must not have been released by this function.
	*/
	virtual TBool IsAllocated(DMemoryObject* aMemory, TUint aIndex, TUint aCount) =0;

protected:
	/**
	Do the action of #Pin for a subregion of a memory mapping.
	This is an implementation factor used to implement #Pin.

	@param aMemory	A memory object associated with this manager.
	@param aIndex	Page index for the start of the region.
	@param aCount	Number of pages in the region.
	@param aMapping	A mapping with the DMemoryMappingBase::EPinned attribute which
					has been attached to \a aMemory.
	@param aPinArgs	The resources to use for pinning. This must have sufficient replacement
					pages allocated to pin every page the mapping covers, and the
					value of \a aPinArgs.iReadOnly must be set to correspond to the
					mappings access permissions.

	@return KErrNone if successful,
			KErrNotFound if any part of the memory to be pinned was not present,
			otherwise one of the system wide error codes.
	*/
	TInt DoPin(DMemoryObject* aMemory, TUint aIndex, TUint aCount, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs);

	/**
	Update a single page in a memory object during memory pinning handling.

	This includes assigning a new physical page of RAM to the memory object (optional)
	then updating the pages state to ensure that it is pinned i.e. doesn't take
	part in demand paging until unpinned again.

	This is called from #DoPin.

	@param aMemory			A memory object associated with this manager.
	@param aIndex			Page index, within the memory, for the page.
	@param aPageInfo		The page information structure of the new physical page
							to assign to the \a aMemory. If there was an existing
							assigned page this new page is freed back to the paging
							system. Specifying a new page is optional, use the null-pointer
							to indicate its absence.
	@param aPageArrayEntry	Reference to the page's page array entry in \a aMemory->iPages.
	@param aPinArgs			The resources to use for pinning. This must a replacement
							pages allocated.

	@return 1 (one) if the new page was assigned to the memory object.
	@return 0 (zero) if a page already existed at the specified index.
	@return KErrNotFound if there no page could be assigned, either because
			none was given, or because memory is in the process of being
			decommitted from the memory object.

	@pre #MmuLock held.
	*/
	virtual TInt PageInPinnedDone(DMemoryObject* aMemory, TUint aIndex, SPageInfo* aPageInfo, TPhysAddr* aPageArrayEntry, TPinArgs& aPinArgs);

	/**
	Implementation factor for #PageInDone and #PageInPinnedDone.

	@pre #MmuLock held.
	@post #MmuLock held and must not have been released by this function.
	*/
	TInt DoPageInDone(DMemoryObject* aMemory, TUint aIndex, SPageInfo*& aPageInfo, TPhysAddr* aPageArrayEntry, TBool aPinning);

	/**
	Do the action of #Unpin for a subregion of a memory mapping.
	This is an implementation factor used to implement #Unpin.

	Must only be used for memory pages already successfully pinned.

	@param aMemory	A memory object associated with this manager.
	@param aIndex	Page index for the start of the region.
	@param aCount	Number of pages in the region.
	@param aMapping	A mapping with the DMemoryMappingBase::EPinned attribute which
					has been attached to \a aMemory.
	@param aPinArgs	The resources used for pinning. The replacement pages allocated
					to this will be increased for each page which was became completely
					unpinned.
	*/
	virtual void DoUnpin(DMemoryObject* aMemory, TUint aIndex, TUint aCount, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs);

	// from DMemoryManager...
	virtual void DoCleanupDecommitted(DMemoryObject* aMemory);
	virtual TZonePageType PageType();

	/**
	Decompress demand paged data.

	The compression types supported are:
	- Byte-Pair, specified with a compression type of
	  SRomPageInfo::EBytePair or KUidCompressionBytePair.
	- No Compression, specified with a compression type of zero.

	@param aCompressionType 	The type of decompression to use.
	@param aDst					The destination address of the decompressed data.
	@param aDstBytes			The expected size of the data once it is decompressed.
	@param aSrc					The address for the data to decompress.
	@param aSrcBytes			The size of the data to decompress.

	@return The size of decompressed data in bytes or one of the system wide error codes.
	*/
	TInt Decompress(TUint32 aCompressionType, TLinAddr aDst, TUint aDstBytes, TLinAddr aSrc, TUint aSrcBytes);

private:
	/**
	Update a single page in a memory object during page fault handling.

	This includes assigning a new physical page of RAM to the memory object (optional)
	then updating the demand paging live list to reflect the fact that the memory page
	was newly accessed.

	This is called from #HandleFault.

	@param aMemory			A memory object associated with this manager.
	@param aIndex			Page index, within the memory, for the page.
	@param aPageInfo		The page information structure of the new physical page
							to assign to the \a aMemory. If there was an existing
							assigned page this new page is freed back to the paging
							system. Specifying a new page is optional, use the null-pointer
							to indicate its absence.
	@param aPageArrayEntry	Reference to the page's page array entry in \a aMemory->iPages.

	@return 1 (one) if the new page was assigned to the memory object.
	@return 0 (zero) if a page already existed at the specified index.
	@return	KErrNotFound if there no page could be assigned, either because
			none was given, or because memory is in the process of being
			decommitted from the memory object.

	@pre #MmuLock held.
	*/
	TInt PageInDone(DMemoryObject* aMemory, TUint aIndex, SPageInfo* aPageInfo, TPhysAddr* aPageArrayEntry);
	};



extern DMemoryManager* TheUnpagedMemoryManager;			///< The #DUnpagedMemoryManager
extern DMemoryManager* TheMovableMemoryManager;			///< The #DMovableMemoryManager
extern DMemoryManager* TheDiscardableMemoryManager;		///< The #DDiscardableMemoryManager
extern DMemoryManager* TheHardwareMemoryManager;		///< The #DHardwareMemoryManager
extern DPagedMemoryManager* TheRomMemoryManager;		///< The #DRomMemoryManager
extern DPagedMemoryManager* TheDataPagedMemoryManager;	///< The #DDataPagedMemoryManager
extern DPagedMemoryManager* TheCodePagedMemoryManager;	///< The #DCodePagedMemoryManager

#endif
