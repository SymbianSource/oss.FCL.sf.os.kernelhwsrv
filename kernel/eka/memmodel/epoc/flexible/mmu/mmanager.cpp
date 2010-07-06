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

#include <plat_priv.h>
#include "cache_maintenance.h"
#include "decompress.h"	// include for the generic BytePairDecompress().
#include "mm.h"
#include "mmu.h"
#include "mpager.h"
#include "mmanager.h"
#include "mmapping.h"
#include "mobject.h"
#include "mcleanup.h"


//
// DMemoryManager
//

TInt DMemoryManager::New(DMemoryObject*& aMemory, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags)
	{
	DMemoryObject* memory;
	if(aSizeInPages&(KChunkMask>>KPageShift))
		memory = DFineMemory::New(this,aSizeInPages,aAttributes,aCreateFlags);
	else
		memory = DCoarseMemory::New(this,aSizeInPages,aAttributes,aCreateFlags);
	aMemory = memory;
	if(!memory)
		return KErrNoMemory;
	return KErrNone;
	}


TInt DMemoryManager::Alloc(DMemoryObject* /*aMemory*/, TUint /*aIndex*/, TUint /*aCount*/)
	{
	return KErrNotSupported;
	}


TInt DMemoryManager::AllocContiguous(DMemoryObject* /*aMemory*/, TUint /*aIndex*/, TUint /*aCount*/, TUint /*aAlign*/, TPhysAddr& /*aPhysAddr*/)
	{
	return KErrNotSupported;
	}


void DMemoryManager::Free(DMemoryObject* /*aMemory*/, TUint /*aIndex*/, TUint /*aCount*/)
	{
	}


TInt DMemoryManager::Wipe(DMemoryObject* /*aMemory*/)
	{
	return KErrNotSupported;
	}


TInt DMemoryManager::AddPages(DMemoryObject* /*aMemory*/, TUint /*aIndex*/, TUint /*aCount*/, const TPhysAddr* /*aPages*/)
	{
	return KErrNotSupported;
	}


TInt DMemoryManager::AddContiguous(DMemoryObject* /*aMemory*/, TUint /*aIndex*/, TUint /*aCount*/, TPhysAddr /*aPhysAddr*/)
	{
	return KErrNotSupported;
	}


TInt DMemoryManager::RemovePages(DMemoryObject* /*aMemory*/, TUint /*aIndex*/, TUint /*aCount*/, TPhysAddr* /*aPages*/)
	{
	return KErrNotSupported;
	}


TInt DMemoryManager::AllowDiscard(DMemoryObject* /*aMemory*/, TUint /*aIndex*/, TUint /*aCount*/)
	{
	return KErrNotSupported;
	}


TInt DMemoryManager::DisallowDiscard(DMemoryObject* /*aMemory*/, TUint /*aIndex*/, TUint /*aCount*/)
	{
	return KErrNotSupported;
	}


TInt DMemoryManager::StealPage(DMemoryObject* /*aMemory*/, SPageInfo* /*aPageInfo*/)
	{
	return KErrNotSupported;
	}


TInt DMemoryManager::RestrictPage(DMemoryObject* /*aMemory*/, SPageInfo* /*aPageInfo*/, TRestrictPagesType /*aRestriction*/)
	{
	return KErrNotSupported;
	}


void DMemoryManager::CleanPages(TUint aPageCount, SPageInfo** aPageInfos, TBool /*aBackground*/)
	{
	for (TUint i = 0 ; i < aPageCount ; ++i)
		__NK_ASSERT_DEBUG(!aPageInfos[i]->IsDirty());
	}


TInt DMemoryManager::HandleFault(	DMemoryObject* aMemory, TUint aIndex, DMemoryMapping* aMapping,
									TUint aMapInstanceCount, TUint aAccessPermissions)
	{
	(void)aMemory;
	(void)aIndex;
	(void)aMapping;
	(void)aMapInstanceCount;
	(void)aAccessPermissions;
//	Kern::Printf("DMemoryManager::HandlePageFault(0x%08x,0x%x,0x%08x,%d)",aMemory,aIndex,aMapping,aAccessPermissions);
	return KErrAbort;
	}


TInt DMemoryManager::MovePage(	DMemoryObject* aMemory, SPageInfo* aOldPageInfo, 
								TPhysAddr& aNewPage, TUint aBlockZoneId, TBool aBlockRest)
	{
	MmuLock::Unlock();
	return KErrNotSupported;
	}


TInt DMemoryManager::MoveAndAllocPage(DMemoryObject*, SPageInfo*, TZonePageType)
	{
	__NK_ASSERT_DEBUG(0);	// This should only be invoked on managers that can move or discard pages.
	return KErrNotSupported;
	}


TZonePageType DMemoryManager::PageType()
	{// This should not be invoked on memory managers that do not use the methods
	// AllocPages() and FreePages().
	__NK_ASSERT_DEBUG(0);
	return EPageFixed;
	}

static TMemoryCleanup Cleanup;

DMemoryObject* DMemoryManager::iCleanupHead = 0;
TSpinLock DMemoryManager::iCleanupLock(TSpinLock::EOrderGenericIrqHigh3);

void DMemoryManager::CleanupFunction(TAny*)
	{
	for(;;)
		{
		__SPIN_LOCK_IRQ(iCleanupLock);

		// get an object from queue...
		DMemoryObject* memory = iCleanupHead;
		if(!memory)
			{
			// none left, so end...
			__SPIN_UNLOCK_IRQ(iCleanupLock);
			return;
			}

		if(memory->iCleanupFlags&ECleanupDecommitted)
			{
			// object requires cleanup of decommitted pages...
			memory->iCleanupFlags &= ~ECleanupDecommitted;
			__SPIN_UNLOCK_IRQ(iCleanupLock);
			memory->iManager->DoCleanupDecommitted(memory);
			}
		else
			{
			// object has no more cleanup operations to perform,
			// so remove it from the cleanup queue...
			__NK_ASSERT_DEBUG(memory->iCleanupFlags==ECleanupIsQueued); // no operations left, just flag to say its in the cleanup queue
			memory->iCleanupFlags &= ~ECleanupIsQueued;
			iCleanupHead = memory->iCleanupNext;
			memory->iCleanupNext = NULL;
			__SPIN_UNLOCK_IRQ(iCleanupLock);

			// close reference which was added when object was queued...
			memory->Close();
			}
		}
	}


void DMemoryManager::QueueCleanup(DMemoryObject* aMemory, TCleanupOperationFlag aCleanupOp)
	{
	// add new cleanup operation...
	__SPIN_LOCK_IRQ(iCleanupLock);
	TUint32 oldFlags = aMemory->iCleanupFlags;
	aMemory->iCleanupFlags = oldFlags|aCleanupOp|ECleanupIsQueued;
	__SPIN_UNLOCK_IRQ(iCleanupLock);

	// if cleanup was already requested...
	if(oldFlags)
		return; // nothing more to do

	// increase reference count...
	aMemory->Open();

	// add object to cleanup queue...
	__SPIN_LOCK_IRQ(iCleanupLock);
	aMemory->iCleanupNext = iCleanupHead;
	iCleanupHead = aMemory;
	__SPIN_UNLOCK_IRQ(iCleanupLock);

	// queue cleanup function to run...
	Cleanup.Add((TMemoryCleanupCallback)CleanupFunction,0);
	}


void DMemoryManager::DoCleanupDecommitted(DMemoryObject* aMemory)
	{
	TRACE2(("DMemoryManager::DoCleanupDecommitted(0x%08x)",aMemory));
	__NK_ASSERT_DEBUG(0);
	}


void DMemoryManager::ReAllocDecommitted(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	// make iterator for region...
	RPageArray::TIter pageIter;
	aMemory->iPages.FindStart(aIndex,aCount,pageIter);

	for(;;)
		{
		// find some pages...
		RPageArray::TIter pageList;
		TUint n = pageIter.Find(pageList);
		if(!n)
			break;

		// check each existing page...
		RamAllocLock::Lock();
		TPhysAddr* pages;
		while(pageList.Pages(pages))
			{
			TPhysAddr page = *pages;
			if(RPageArray::State(page)==RPageArray::EDecommitted)
				{
				// decommitted pages need re-initialising...
				TPhysAddr pagePhys = page&~KPageMask;
				*pages = pagePhys|RPageArray::ECommitted;
				TheMmu.PagesAllocated(&pagePhys,1,aMemory->RamAllocFlags(),true);
				}
			pageList.Skip(1);
			}
		RamAllocLock::Unlock();

		// move on...
		pageIter.FindRelease(n);
		}

	aMemory->iPages.FindEnd(aIndex,aCount);
	}


void DMemoryManager::FreeDecommitted(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	TRACE2(("DMemoryManager::FreeDecommitted(0x%08x,0x%x,0x%x)",aMemory, aIndex, aCount));
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	// make iterator for region...
	RPageArray::TIter pageIter;
	aMemory->iPages.FindStart(aIndex,aCount,pageIter);

	for(;;)
		{
		// find some pages...
		RPageArray::TIter pageList;
		TUint findCount = pageIter.Find(pageList);
		if(!findCount)
			break;

		// search for decommitted pages...
		RamAllocLock::Lock();
		TPhysAddr* pages;
		TUint numPages;
		while((numPages=pageList.Pages(pages))!=0)
			{
			TUint n=0;
			if(RPageArray::State(pages[n])!=RPageArray::EDecommitted)
				{
				// skip pages which aren't EDecommitted...
				while(++n<numPages && RPageArray::State(pages[n])!=RPageArray::EDecommitted)
					{}
				}
			else
				{
				// find range of pages which are EDecommitted...
				while(++n<numPages && RPageArray::State(pages[n])==RPageArray::EDecommitted)
					{}
				RPageArray::TIter decommittedList(pageList.Left(n));

				// free pages...
				TUint freedCount = FreePages(aMemory,decommittedList);
				(void)freedCount;
				TRACE2(("DMemoryManager::FreeDecommitted(0x%08x) freed %d in 0x%x..0x%x",aMemory,freedCount,decommittedList.Index(),decommittedList.IndexEnd()));
				}
			pageList.Skip(n);
			}
		RamAllocLock::Unlock();

		// move on...
		pageIter.FindRelease(findCount);
		}

	aMemory->iPages.FindEnd(aIndex,aCount);
	}


void DMemoryManager::DoFree(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	TRACE2(("DMemoryManager::DoFree(0x%08x,0x%x,0x%x)",aMemory, aIndex, aCount));
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	RPageArray::TIter pageIter;
	aMemory->iPages.FindStart(aIndex,aCount,pageIter);

	for(;;)
		{
		// find some pages...
		RPageArray::TIter pageList;
		TUint n = pageIter.RemoveFind(pageList);
		if(!n)
			break;

		// free pages...
		FreePages(aMemory,pageList);

		// move on...
		pageIter.FindRelease(n);
		}

	aMemory->iPages.FindEnd(aIndex,aCount);
	}


TInt DMemoryManager::FreePages(DMemoryObject* aMemory, RPageArray::TIter aPageList)
	{
	// unmap the pages...
	aMemory->UnmapPages(aPageList,true);

	RamAllocLock::Lock();

	// remove and free pages...
	Mmu& m = TheMmu;
	TUint count = 0;
	TPhysAddr pages[KMaxPagesInOneGo];
	TUint n;
	while((n=aPageList.Remove(KMaxPagesInOneGo,pages))!=0)
		{
		count += n;
		m.FreeRam(pages, n, aMemory->iManager->PageType());
		}

	RamAllocLock::Unlock();

	return count;
	}



/**
Manager for memory objects containing normal unpaged program memory (RAM) which
is allocated from a system wide pool. The physical pages allocated to this
memory are fixed until explicitly freed.

This is normally used for kernel memory and any other situation where it
is not permissible for memory accesses to generate page faults of any kind.
*/
class DUnpagedMemoryManager : public DMemoryManager
	{
public:
	// from DMemoryManager...
	virtual void Destruct(DMemoryObject* aMemory);
	virtual TInt Alloc(DMemoryObject* aMemory, TUint aIndex, TUint aCount);
	virtual TInt AllocContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TUint aAlign, TPhysAddr& aPhysAddr);
	virtual void Free(DMemoryObject* aMemory, TUint aIndex, TUint aCount);
	virtual TInt Pin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs);
	virtual void Unpin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs);
	virtual TInt Wipe(DMemoryObject* aMemory);
	virtual TZonePageType PageType();

private:
	// from DMemoryManager...
	virtual void DoCleanupDecommitted(DMemoryObject* aMemory);

	/**
	Implementation factor for implementation of #Alloc.
	*/
	static TInt AllocPages(DMemoryObject* aMemory, RPageArray::TIter aPageList);

	/**
	Implementation factor for implementation of #AllocContiguous.
	*/
	static TInt AllocContiguousPages(DMemoryObject* aMemory, RPageArray::TIter aPageList, TUint aAlign, TPhysAddr& aPhysAddr);

	/**
	Implementation factor for implementation of #Wipe.
	*/
	static void WipePages(DMemoryObject* aMemory, RPageArray::TIter aPageList);

public:
	/**
	The single instance of this manager class.
	*/
	static DUnpagedMemoryManager TheManager;
	};


DUnpagedMemoryManager DUnpagedMemoryManager::TheManager;
DMemoryManager* TheUnpagedMemoryManager = &DUnpagedMemoryManager::TheManager;


void DUnpagedMemoryManager::Destruct(DMemoryObject* aMemory)
	{
	MemoryObjectLock::Lock(aMemory);
	Free(aMemory,0,aMemory->iSizeInPages);
	MemoryObjectLock::Unlock(aMemory);
	aMemory->Close();
	}


TInt DUnpagedMemoryManager::Alloc(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	TRACE2(("DUnpagedMemoryManager::Alloc(0x%08x,0x%x,0x%x)",aMemory, aIndex, aCount));
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	// re-initialise any decommitted pages which we may still own because they were pinned...
	ReAllocDecommitted(aMemory,aIndex,aCount);

	// check and allocate page array entries...
	RPageArray::TIter pageList;
	TInt r = aMemory->iPages.AddStart(aIndex,aCount,pageList,true);
	if(r!=KErrNone)
		return r;

	// allocate RAM and add it to page array...
	r = AllocPages(aMemory,pageList);

	// map pages...
	if(r==KErrNone)
		r = aMemory->MapPages(pageList);

	// release page array entries...
	aMemory->iPages.AddEnd(aIndex,aCount);

	// revert if error...
	if(r!=KErrNone)
		Free(aMemory,aIndex,aCount);

	return r;
	}


TInt DUnpagedMemoryManager::AllocContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TUint aAlign, TPhysAddr& aPhysAddr)
	{
	TRACE2(("DUnpagedMemoryManager::AllocContiguous(0x%08x,0x%x,0x%x,%d,?)",aMemory, aIndex, aCount, aAlign));
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	// set invalid memory in case of error...
	aPhysAddr = KPhysAddrInvalid;

	// check and allocate page array entries...
	RPageArray::TIter pageList;
	TInt r = aMemory->iPages.AddStart(aIndex,aCount,pageList);
	if(r!=KErrNone)
		return r;

	// allocate memory...
	TPhysAddr physAddr;
	r = AllocContiguousPages(aMemory, pageList, aAlign, physAddr);

	// map memory...
	if(r==KErrNone)
		{
		r = aMemory->MapPages(pageList);
		if(r==KErrNone)
			aPhysAddr = physAddr;
		}

	// release page array entries...
	aMemory->iPages.AddEnd(aIndex,aCount);

	// revert if error...
	if(r!=KErrNone)
		Free(aMemory,aIndex,aCount);

	return r;
	}


void DUnpagedMemoryManager::Free(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	DoFree(aMemory,aIndex,aCount);
	}


TInt DUnpagedMemoryManager::AllocPages(DMemoryObject* aMemory, RPageArray::TIter aPageList)
	{
	TInt r = KErrNone;
	RamAllocLock::Lock();

	Mmu& m = TheMmu;
	for(;;)
		{
		// find entries in page array to allocate...
		RPageArray::TIter allocList;
		TUint n = aPageList.AddFind(allocList);
		if(!n)
			break;

		do
			{
			// allocate ram...
			TPhysAddr pages[KMaxPagesInOneGo];
			if(n>KMaxPagesInOneGo)
				n = KMaxPagesInOneGo;
			r = m.AllocRam(pages, n, aMemory->RamAllocFlags(), aMemory->iManager->PageType());
			if(r!=KErrNone)
				goto done;

			// assign pages to memory object...
			{
			TUint index = allocList.Index();
			TUint flags = aMemory->PageInfoFlags();
			TUint i=0;
			MmuLock::Lock();
			do
				{
				SPageInfo* pi = SPageInfo::FromPhysAddr(pages[i]);
				pi->SetManaged(aMemory,index+i,flags);
				}
			while(++i<n);
			MmuLock::Unlock();
			}

			// add pages to page array...
			allocList.Add(n,pages);
			}
		while((n=allocList.Count())!=0);
		}
done:
	RamAllocLock::Unlock();
	return r;
	}


TInt DUnpagedMemoryManager::AllocContiguousPages(DMemoryObject* aMemory, RPageArray::TIter aPageList, TUint aAlign, TPhysAddr& aPhysAddr)
	{
	TUint size = aPageList.Count();
	RamAllocLock::Lock();

	// allocate memory...
	Mmu& m = TheMmu;
	TPhysAddr physAddr;
	TInt r = m.AllocContiguousRam(physAddr, size, aAlign, aMemory->RamAllocFlags());
	if(r==KErrNone)
		{
		// assign pages to memory object...
		TUint index = aPageList.Index();
		TUint flags = aMemory->PageInfoFlags();
		SPageInfo* pi = SPageInfo::FromPhysAddr(physAddr);
		SPageInfo* piEnd = pi+size;
		TUint flash = 0;
		MmuLock::Lock();
		while(pi<piEnd)
			{
			MmuLock::Flash(flash,KMaxPageInfoUpdatesInOneGo);
			pi->SetManaged(aMemory,index++,flags);
			++pi;
			}
		MmuLock::Unlock();

		// add pages to page array...
		aPageList.AddContiguous(size,physAddr);

		// set result...
		aPhysAddr = physAddr;
		}

	RamAllocLock::Unlock();
	return r;
	}


TInt DUnpagedMemoryManager::Pin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs)
	{
	RPageArray::TIter pageList;
	aMemory->iPages.FindStart(aMapping->iStartIndex,aMapping->iSizeInPages,pageList);

	MmuLock::Lock();

	TUint n;
	TPhysAddr* pages;
	TUint flash = 0;
	while((n=pageList.Pages(pages,KMaxPageInfoUpdatesInOneGo))!=0)
		{
		TPhysAddr* p = pages;
		TPhysAddr* pEnd = p+n;
		do
			{
			TPhysAddr page = *p++;
			if(RPageArray::TargetStateIsDecommitted(page))
				goto stop; // page is being decommitted, so can't pin it
			}
		while(p!=pEnd);
		pageList.Skip(n);
		flash += n;
		MmuLock::Flash(flash,KMaxPageInfoUpdatesInOneGo);
		}
stop:
	MmuLock::Unlock();

	aMemory->iPages.FindEnd(aMapping->iStartIndex,aMapping->iSizeInPages);

	return pageList.Count() ? KErrNotFound : KErrNone;
	}


void DUnpagedMemoryManager::Unpin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs)
	{
	}


void DUnpagedMemoryManager::DoCleanupDecommitted(DMemoryObject* aMemory)
	{
	MemoryObjectLock::Lock(aMemory);
	FreeDecommitted(aMemory,0,aMemory->iSizeInPages);
	MemoryObjectLock::Unlock(aMemory);
	}


TInt DUnpagedMemoryManager::Wipe(DMemoryObject* aMemory)
	{
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	// make iterator for region...
	RPageArray::TIter pageIter;
	aMemory->iPages.FindStart(0,aMemory->iSizeInPages,pageIter);

	for(;;)
		{
		// find some pages...
		RPageArray::TIter pageList;
		TUint n = pageIter.Find(pageList);
		if(!n)
			break;

		// wipe some pages...
		WipePages(aMemory,pageList);

		// move on...
		pageIter.FindRelease(n);
		}

	aMemory->iPages.FindEnd(0,aMemory->iSizeInPages);

	return KErrNone;
	}


void DUnpagedMemoryManager::WipePages(DMemoryObject* aMemory, RPageArray::TIter aPageList)
	{
	TUint index = aPageList.Index();
	TUint count = aPageList.Count();
	TRACE(("DUnpagedMemoryManager::WipePages(0x%08x,0x%x,0x%x)",aMemory,index,count));

	__NK_ASSERT_ALWAYS(!aMemory->IsReadOnly()); // trap wiping read-only memory

	RamAllocLock::Lock();

	while(count)
		{
		// get some physical page addresses...
		TPhysAddr pages[KMaxPagesInOneGo];
		TPhysAddr physAddr;
		TUint n = count;
		if(n>KMaxPagesInOneGo)
			n = KMaxPagesInOneGo;
		TInt r = aMemory->iPages.PhysAddr(index,n,physAddr,pages);
		__NK_ASSERT_ALWAYS(r>=0); // caller should have ensured all pages are present

		// wipe some pages...
		TPhysAddr* pagesToWipe = r!=0 ? pages : (TPhysAddr*)((TLinAddr)physAddr|1);
		TheMmu.PagesAllocated(pagesToWipe,n,aMemory->RamAllocFlags(),true);

		// move on...
		index += n;
		count -= n;
		}

	RamAllocLock::Unlock();
	}


TZonePageType DUnpagedMemoryManager::PageType()
	{// Unpaged memory cannot be moved or discarded therefore it is fixed.
	return EPageFixed;
	}


/**
Manager for memory objects containing normal unpaged RAM, as
#DUnpagedMemoryManager, but which may be 'moved' by RAM
defragmentation. I.e. have the physical pages used to store its content
substituted for others.

Such memory may cause transient page faults if it is accessed whilst its
contents are being moved, this makes it unsuitable for most kernel-side
usage. This is the memory management scheme normally used for unpaged user
memory.
*/
class DMovableMemoryManager : public DUnpagedMemoryManager
	{
public:
	// from DMemoryManager...
	virtual TInt MovePage(DMemoryObject* aMemory, SPageInfo* aOldPageInfo, TPhysAddr& aNewPage, TUint aBlockZoneId, TBool aBlockRest);
	virtual TInt MoveAndAllocPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TZonePageType aPageType);
	virtual TInt HandleFault(	DMemoryObject* aMemory, TUint aIndex, DMemoryMapping* aMapping, 
								TUint aMapInstanceCount, TUint aAccessPermissions);
	virtual TZonePageType PageType();
public:
	/**
	The single instance of this manager class.
	*/
	static DMovableMemoryManager TheManager;
	};


DMovableMemoryManager DMovableMemoryManager::TheManager;
DMemoryManager* TheMovableMemoryManager = &DMovableMemoryManager::TheManager;


TInt DMovableMemoryManager::MovePage(	DMemoryObject* aMemory, SPageInfo* aOldPageInfo, 
										TPhysAddr& aNewPage, TUint aBlockZoneId, TBool aBlockRest)
	{
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	TInt r = KErrInUse;

	TUint index = aOldPageInfo->Index();
	TRACE(	("DMovableMemoryManager::MovePage(0x%08x,0x%08x,?,0x%08x,%d) index=0x%x",
			aMemory,aOldPageInfo,aBlockZoneId,aBlockRest,index));
	__NK_ASSERT_DEBUG(aMemory==aOldPageInfo->Owner());

	// Mark the page as being moved and get a pointer to the page array entry.
	RPageArray::TIter pageIter;
	TPhysAddr* const movingPageArrayPtr = aMemory->iPages.MovePageStart(index, pageIter);
	if (!movingPageArrayPtr)
		{// Can't move the page another operation is being performed on it.
		MmuLock::Unlock();
		return r;
		}
	__NK_ASSERT_DEBUG(RPageArray::IsPresent(*movingPageArrayPtr));
	TPhysAddr oldPageEntry = *movingPageArrayPtr;
	TPhysAddr oldPage = oldPageEntry & ~KPageMask;
#ifdef _DEBUG
	if (oldPage != aOldPageInfo->PhysAddr())
		{// The address of page array entry and the page info should match except
		// when the page is being shadowed.
		__NK_ASSERT_DEBUG(SPageInfo::FromPhysAddr(oldPage)->Type() == SPageInfo::EShadow);
		}
#endif
	// Set the modifier so we can detect if the page state is updated.
	aOldPageInfo->SetModifier(&pageIter);

	MmuLock::Unlock();

	// Allocate the new page to move to, ensuring that we use the page type of the
	// manager assigned to this page.
	TPhysAddr newPage;
	Mmu& m = TheMmu;
	TInt allocRet = m.AllocRam(&newPage, 1, aMemory->RamAllocFlags(), aMemory->iManager->PageType(), 
						aBlockZoneId, aBlockRest);
	if (allocRet != KErrNone)
		{
		MmuLock::Lock();
		aMemory->iPages.MovePageEnd(*movingPageArrayPtr, index);
		MmuLock::Unlock();
		return allocRet;
		}

	__NK_ASSERT_DEBUG((newPage & KPageMask) == 0);
	__NK_ASSERT_DEBUG(newPage != oldPage);

	MmuLock::Lock();
	if (aOldPageInfo->CheckModified(&pageIter) ||
		oldPageEntry != *movingPageArrayPtr)
		{// The page was modified or freed.
		aMemory->iPages.MovePageEnd(*movingPageArrayPtr, index);
		MmuLock::Unlock();
		m.FreeRam(&newPage, 1, aMemory->iManager->PageType());
		return r;
		}
	MmuLock::Unlock();

	// This page's contents may be changed so restrict the page to no access 
	// so we can detect any access to it while we are moving it.
	// Read only memory objects don't need to be restricted but we still need
	// to discover any physically pinned mappings.
	TBool pageRestrictedNA = !aMemory->IsReadOnly();
	TRestrictPagesType restrictType = 	pageRestrictedNA ? 
										ERestrictPagesNoAccessForMoving :
										ERestrictPagesForMovingFlag;

	// This will clear the memory objects mapping added flag so we can detect any new mappings.
	aMemory->RestrictPages(pageIter, restrictType);

	const TUint KOldMappingSlot = 0;
	const TUint KNewMappingSlot = 1;
	const TAny* tmpPtrOld = NULL;
	TAny* tmpPtrNew;
	// Verify that page restricting wasn't interrupted, if it was then the page 
	// can't be moved so remap it.
	// If the page array entry (*movingPageArrayPtr) has been modified then a pinning 
	// veto'd the preparation.
	MmuLock::Lock();
	if (aOldPageInfo->CheckModified(&pageIter) ||
		oldPageEntry != *movingPageArrayPtr)
		{// Page is pinned or has been modified by another operation.
		MmuLock::Unlock();
		TheMmu.FreeRam(&newPage, 1, aMemory->iManager->PageType());
		goto remap;
		}

	MmuLock::Unlock();
	// Copy the contents of the page using some temporary mappings.
	tmpPtrOld = (TAny*)TheMmu.MapTemp(oldPage, index, KOldMappingSlot);
	tmpPtrNew = (TAny*)TheMmu.MapTemp(newPage, index, KNewMappingSlot);
	pagecpy(tmpPtrNew, tmpPtrOld);

	// Unmap and perform cache maintenance if the memory object is executable.
	// Must do cache maintenance before we add any new mappings to the new page 
	// to ensure that any old instruction cache entries for the new page aren't 
	// picked up by any remapped executable mappings.
	if (aMemory->IsExecutable())
		CacheMaintenance::CodeChanged((TLinAddr)tmpPtrNew, KPageSize);
	TheMmu.UnmapTemp(KNewMappingSlot);
#ifndef _DEBUG
	TheMmu.UnmapTemp(KOldMappingSlot);
#endif
	
	MmuLock::Lock();
	if (!aOldPageInfo->CheckModified(&pageIter) &&
		oldPageEntry == *movingPageArrayPtr &&
		!aMemory->MappingAddedFlag())
		{
		// The page has been copied without anyone modifying it so set the page 
		// array entry to new physical address and map the page.
		RPageArray::PageMoveNewAddr(*movingPageArrayPtr, newPage);

		// Copy across the page info data from the old page to the new.
		SPageInfo& newPageInfo = *SPageInfo::FromPhysAddr(newPage);
		newPageInfo = *aOldPageInfo;
		if (aMemory->IsDemandPaged())
			{// Let the pager deal with the live list links for this page if required.
			ThePager.ReplacePage(*aOldPageInfo, newPageInfo);
			}

		MmuLock::Unlock();
		r = KErrNone;
		aNewPage = newPage;
		}
	else
		{
		MmuLock::Unlock();
		TheMmu.FreeRam(&newPage, 1, aMemory->iManager->PageType());
		}
remap:
	// Remap all mappings to the new physical address if the move was successful or
	// back to the old page if the move failed.
	// Invalidate the TLB for the page if old mappings still exist or new
	// mappings were added but will be removed as the page can't be moved.
	TBool invalidateTLB = !pageRestrictedNA || r != KErrNone;
	aMemory->RemapPage(*movingPageArrayPtr, index, invalidateTLB);

	if (r == KErrNone)
		{// Must wait until here as read only memory objects' mappings aren't 
		// all guaranteed to point to the new page until after RemapPage().
		TheMmu.FreeRam(&oldPage, 1, aMemory->iManager->PageType());
#ifdef _DEBUG
		// For testing purposes clear the old page to help detect any 
		// erroneous mappings to the old page.  
		memclr((TAny*)tmpPtrOld, KPageSize);
		}
	TheMmu.UnmapTemp(KOldMappingSlot);	// Will invalidate the TLB entry for the mapping.
#else
		}
#endif
	// indicate we've stopped moving memory now...
	MmuLock::Lock();
	aMemory->iPages.MovePageEnd(*movingPageArrayPtr, index);
	MmuLock::Unlock();

	return r;
	}


TInt DMovableMemoryManager::MoveAndAllocPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TZonePageType aPageType)
	{
	TPhysAddr newPage;
	TInt r = MovePage(aMemory, aPageInfo, newPage, KRamZoneInvalidId, EFalse);
	if (r == KErrNone)
		{
		TheMmu.MarkPageAllocated(aPageInfo->PhysAddr(), aPageType);
		}
	return r;
	}


TInt DMovableMemoryManager::HandleFault(DMemoryObject* aMemory, TUint aIndex, DMemoryMapping* aMapping, 
										TUint aMapInstanceCount, TUint aAccessPermissions)
	{
	TInt r = KErrNotFound;
	SPageInfo* pageInfo;
	MmuLock::Lock();
	__UNLOCK_GUARD_START(MmuLock);
	TPhysAddr* const pageEntry = aMemory->iPages.PageEntry(aIndex);
	if (!pageEntry || !RPageArray::IsPresent(*pageEntry) || 
		aMapInstanceCount != aMapping->MapInstanceCount() || aMapping->BeingDetached())
		{// The page isn't present or has been unmapped so invalid access.
		goto exit;
		}

	if (aMapping->MovingPageIn(*pageEntry, aIndex))
		{// The page was has been paged in as it was still mapped.
		pageInfo = SPageInfo::FromPhysAddr(*pageEntry & ~KPageMask);
		pageInfo->SetModifier(0); // Signal to MovePage() that the page has been paged in.
		r = KErrNone;
		}

exit:
	__UNLOCK_GUARD_END(MmuLock);
	MmuLock::Unlock();
	return r;
	}


TZonePageType DMovableMemoryManager::PageType()
	{// Movable memory object pages are movable.
	return EPageMovable;
	}


/**
Manager for memory objects containing normal unpaged RAM, which
as well as being 'movable', like #DMovableMemoryManager,
may also have regions marked as 'discardable'.  Discardable pages may be
reclaimed (removed) by the system at any time; this state is controlled using
the functions #AllowDiscard and #DisallowDiscard.
<P>
This is used for the memory containing file system caches. Discardable memory
is managed using similar 
*/
class DDiscardableMemoryManager : public DMovableMemoryManager
	{
public:
	// from DMemoryManager...
	virtual TInt AllowDiscard(DMemoryObject* aMemory, TUint aIndex, TUint aCount);
	virtual TInt DisallowDiscard(DMemoryObject* aMemory, TUint aIndex, TUint aCount);
	virtual TInt StealPage(DMemoryObject* aMemory, SPageInfo* aPageInfo);
	virtual TInt RestrictPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TRestrictPagesType aRestriction);
	virtual TZonePageType PageType();
public:
	/**
	The single instance of this manager class.
	*/
	static DDiscardableMemoryManager TheManager;
	};


DDiscardableMemoryManager DDiscardableMemoryManager::TheManager;
DMemoryManager* TheDiscardableMemoryManager = &DDiscardableMemoryManager::TheManager;


TInt DDiscardableMemoryManager::AllowDiscard(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	TRACE2(("DDiscardableMemoryManager::AllowDiscard(0x%08x,0x%x,0x%x)",aMemory, aIndex, aCount));
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	// make iterator for region...
	RPageArray::TIter pageIter;
	aMemory->iPages.FindStart(aIndex,aCount,pageIter);

	for(;;)
		{
		// find some pages...
		RPageArray::TIter pageList;
		TUint nFound = pageIter.Find(pageList);
		if(!nFound)
			break;

		// donate pages...
		TUint n;
		TPhysAddr* pages;
		while((n=pageList.Pages(pages,KMaxPagesInOneGo))!=0)
			{
			pageList.Skip(n);
			ThePager.DonatePages(n,pages);
			}

		// move on...
		pageIter.FindRelease(nFound);
		}

	// done...
	aMemory->iPages.FindEnd(aIndex,aCount);

	return KErrNone;
	}


TInt DDiscardableMemoryManager::DisallowDiscard(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	TRACE2(("DDiscardableMemoryManager::DisallowDiscard(0x%08x,0x%x,0x%x)",aMemory, aIndex, aCount));
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	TInt r = KErrNone;

	// get pages...
	RPageArray::TIter pageIter;
	aMemory->iPages.FindStart(aIndex,aCount,pageIter);

	RPageArray::TIter pageList;
	TUint numPages = pageIter.Find(pageList);

	if(numPages!=aCount)
		{
		// not all pages are present...
		r = KErrNotFound;
		}
	else
		{
		TUint n;
		TPhysAddr* pages;
		while((n=pageList.Pages(pages,KMaxPagesInOneGo))!=0)
			{
			pageList.Skip(n);
			r = ThePager.ReclaimPages(n,pages);
			if(r!=KErrNone)
				break;
			}
		}

	// done with pages...
	if(numPages)
		pageIter.FindRelease(numPages);
	aMemory->iPages.FindEnd(aIndex,aCount);

	return r;
	}


TInt DDiscardableMemoryManager::StealPage(DMemoryObject* aMemory, SPageInfo* aPageInfo)
	{
	TRACE2(("DDiscardableMemoryManager::StealPage(0x%08x,0x%08x)",aMemory,aPageInfo));
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__UNLOCK_GUARD_START(MmuLock);

	// must always hold the PageCleaningLock if the page needs to be cleaned
	__NK_ASSERT_DEBUG(!aPageInfo->IsDirty() || PageCleaningLock::IsHeld());

	TUint index = aPageInfo->Index();
	TInt r;

	RPageArray::TIter pageList;
	TPhysAddr* p = aMemory->iPages.StealPageStart(index,pageList);
	__NK_ASSERT_DEBUG((*p&~KPageMask)==aPageInfo->PhysAddr()); // object should have our page

	aPageInfo->SetModifier(&pageList);

	__UNLOCK_GUARD_END(MmuLock);
	MmuLock::Unlock();

	// unmap the page...
	aMemory->UnmapPages(pageList,false);

	MmuLock::Lock();

	__NK_ASSERT_DEBUG((*p&~KPageMask)==aPageInfo->PhysAddr()); // object should still have our page because freeing a page requires the RamAllocLock, which we hold

	if(aPageInfo->CheckModified(&pageList))
		{
		// page state was changed, this can only happen if a page fault put this page
		// back into the committed state or if the page was pinned.
		// From either of these states it's possible to subsequently change
		// to any other state or use (so we can't assert anything here).
		r = KErrInUse;
		}
	else
		{
		// nobody else has modified page state, so we can...
		TPhysAddr page = *p;
		__NK_ASSERT_DEBUG(RPageArray::TargetStateIsDecommitted(page));
		if(page&RPageArray::EUnmapVetoed)
			{
			// operation was vetoed, which means page had a pinned mapping but the pin
			// operation hadn't got around to removing the page from the live list,
			// we need to restore correct state...
			if(RPageArray::State(page)==RPageArray::EStealing)
				*p = (page&~(RPageArray::EStateMask|RPageArray::EUnmapVetoed))|RPageArray::ECommitted;
			// else
			//	   leave page in state it was before we attempted to steal it

			// put page back on live list so it doesn't get lost.
			// We put it at the start as if it were recently accessed because being pinned
			// counts as an access and we can't put it anywhere else otherwise when
			// page stealing retries it may get this same page again, potentially causing
			// deadlock.
			__NK_ASSERT_DEBUG(aPageInfo->PagedState()==SPageInfo::EUnpaged); // no one else has changed page since we removed it in DPager::StealPage
			ThePager.PagedIn(aPageInfo);

			r = KErrInUse;
			}
		else
			{
			// page successfully unmapped...
			aPageInfo->SetReadOnly(); // page not mapped, so must be read-only

			// attempt to clean the page if it is dirty...
			if (aPageInfo->IsDirty())
				{
				//Kern::Printf("WDP: Cleaning single page in StealPage");
				aMemory->iManager->CleanPages(1, &aPageInfo, EFalse);
				}

			if(aPageInfo)
				{
				// page successfully stolen...
				__NK_ASSERT_DEBUG((*p^page)<(TUint)KPageSize); // sanity check, page should still be allocated to us
				__NK_ASSERT_DEBUG(aPageInfo->IsDirty()==false);
				__NK_ASSERT_DEBUG(aPageInfo->IsWritable()==false);

				TPhysAddr pagerInfo = aPageInfo->PagingManagerData();
				*p = pagerInfo;
				__NK_ASSERT_ALWAYS((pagerInfo&(RPageArray::EFlagsMask|RPageArray::EStateMask)) == RPageArray::ENotPresent);

				TheMmu.PageFreed(aPageInfo);
				r = KErrNone;
				}
			else
				r = KErrInUse;
			}
		}

	aMemory->iPages.StealPageEnd(index,r==KErrNone ? 1 : 0);

#ifdef _DEBUG
	if(r!=KErrNone)
		TRACE2(("DDiscardableMemoryManager::StealPage fail because preempted"));
#endif

	TRACE2(("DDiscardableMemoryManager::StealPage returns %d",r));
	return r;
	}


TInt DDiscardableMemoryManager::RestrictPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TRestrictPagesType aRestriction)
	{
	if(aRestriction==ERestrictPagesNoAccessForOldPage)
		{
		// Lie to pager when it sets an old page inaccessible as we don't want to rejunvanate
		// the page if it is accessed as RChunk::Lock() should be used to remove the page from 
		// the live list before accessing the page.
		return KErrNone;
		}
	return DMovableMemoryManager::RestrictPage(aMemory, aPageInfo, aRestriction);
	}


TZonePageType DDiscardableMemoryManager::PageType()
	{// Discardable memory objects page are movable unless they are donated to the pager.
	return EPageMovable;
	}



/**
Manager for memory objects containing memory mapped hardware devices or special
purpose memory for which the physical addresses are fixed.
*/
class DHardwareMemoryManager : public DMemoryManager
	{
public:
	// from DMemoryManager...
	virtual void Destruct(DMemoryObject* aMemory);
	virtual TInt AddPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, const TPhysAddr* aPages);
	virtual TInt AddContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr aPhysAddr);
	virtual TInt RemovePages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr* aPages);
	virtual TInt Pin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs);
	virtual void Unpin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs);

private:
	/**
	Update the page information structure for RAM added with #AddPages and #AddContiguous.

	This performs debug checks to ensure that any physical memory which is added to more than
	one memory object meets with the restriction imposed by the MMU and cache hardware.
	It also verifies that the RAM pages are of type SPageInfo::EPhysAlloc,
	i.e. were allocated with Epoc::AllocPhysicalRam or similar.

	This is only used when the physical addresses of the page being added to a memory
	object corresponds to RAM being managed by the kernel, i.e. physical addresses
	with an associated #SPageInfo structure.

	@param aMemory			A memory object associated with this manager.
	@param aIndex			Page index, within the memory, for the page.
	@param aPageInfo		The page information structure of the RAM page.

	@pre #MmuLock held.
	@post #MmuLock held.
	*/
	static void AssignPage(DMemoryObject* aMemory, TUint aIndex, SPageInfo* aPageInfo);

	/**
	Update the page information structure for RAM removed with #RemovePages.

	This is only used when the physical addresses of the page being removed from a memory
	object corresponds to RAM being managed by the kernel, i.e. physical addresses
	with an associated #SPageInfo structure.

	@param aMemory			A memory object associated with this manager.
	@param aIndex			Page index, within the memory, for the page.
	@param aPageInfo		The page information structure of the RAM page.

	@pre #MmuLock held.
	@post #MmuLock held.
	*/
	static void UnassignPage(DMemoryObject* aMemory, TUint aIndex, SPageInfo* aPageInfo);

public:
	/**
	The single instance of this manager class.
	*/
	static DHardwareMemoryManager TheManager;
	};


DHardwareMemoryManager DHardwareMemoryManager::TheManager;
DMemoryManager* TheHardwareMemoryManager = &DHardwareMemoryManager::TheManager;


void DHardwareMemoryManager::Destruct(DMemoryObject* aMemory)
	{
	MemoryObjectLock::Lock(aMemory);
	RemovePages(aMemory,0,aMemory->iSizeInPages,0);
	MemoryObjectLock::Unlock(aMemory);
	aMemory->Close();
	}


TInt DHardwareMemoryManager::AddPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, const TPhysAddr* aPages)
	{
	TRACE2(("DHardwareMemoryManager::AddPages(0x%08x,0x%x,0x%x,?)",aMemory, aIndex, aCount));
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	// validate arguments...
	const TPhysAddr* pages = aPages;
	const TPhysAddr* pagesEnd = aPages+aCount;
	TPhysAddr checkMask = 0;
	do checkMask |= *pages++;
	while(pages<pagesEnd);
	if(checkMask&KPageMask)
		return KErrArgument;

	// check and allocate page array entries...
	RPageArray::TIter pageIter;
	TInt r = aMemory->iPages.AddStart(aIndex,aCount,pageIter);
	if(r!=KErrNone)
		return r;

	// assign pages...
	pages = aPages;
	TUint index = aIndex;
	TUint flash = 0;
	MmuLock::Lock();
	do
		{
		MmuLock::Flash(flash,KMaxPageInfoUpdatesInOneGo/2); // flash twice as often because we're doing about twice the work as a simple page info update
		TPhysAddr pagePhys = *pages++;
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pagePhys);
		if(pi)
			AssignPage(aMemory,index,pi);
		++index;
		}
	while(pages<pagesEnd);
	MmuLock::Unlock();

	// map the pages...
	RPageArray::TIter pageList = pageIter;
	pageIter.Add(aCount,aPages);
	r = aMemory->MapPages(pageList);

	// release page array entries...
	aMemory->iPages.AddEnd(aIndex,aCount);

	// revert if error...
	if(r!=KErrNone)
		RemovePages(aMemory,aIndex,aCount,0);

	return r;
	}


TInt DHardwareMemoryManager::AddContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr aPhysAddr)
	{
	TRACE2(("DHardwareMemoryManager::AddContiguous(0x%08x,0x%x,0x%x,0x%08x)",aMemory, aIndex, aCount, aPhysAddr));
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	// validate arguments...
	if(aPhysAddr&KPageMask)
		return KErrArgument;

	// check and allocate page array entries...
	RPageArray::TIter pageIter;
	TInt r = aMemory->iPages.AddStart(aIndex,aCount,pageIter);
	if(r!=KErrNone)
		return r;

	RPageArray::TIter pageList = pageIter;

	// assign pages...
	SPageInfo* piStart = SPageInfo::SafeFromPhysAddr(aPhysAddr);
	SPageInfo* piEnd = piStart+aCount;
	if(piStart)
		{
		SPageInfo* pi = piStart;
		TUint index = aIndex;
		TUint flash = 0;
		MmuLock::Lock();
		while(pi<piEnd)
			{
			MmuLock::Flash(flash,KMaxPageInfoUpdatesInOneGo/2); // flash twice as often because we're doing about twice the work as a simple page info update
			AssignPage(aMemory,index,pi);
			++index;
			++pi;
			}
		MmuLock::Unlock();
		}

	// map the pages...
	pageIter.AddContiguous(aCount,aPhysAddr);
	r = aMemory->MapPages(pageList);

	// release page array entries...
	aMemory->iPages.AddEnd(aIndex,aCount);

	// revert if error...
	if(r!=KErrNone)
		RemovePages(aMemory,aIndex,aCount,0);

	return r;
	}


TInt DHardwareMemoryManager::RemovePages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr* aPages)
	{
	TRACE2(("DHardwareMemoryManager::RemovePages(0x%08x,0x%x,0x%x,?)",aMemory, aIndex, aCount));
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	RPageArray::TIter pageIter;
	aMemory->iPages.FindStart(aIndex,aCount,pageIter);

	TUint numPages = 0;
	for(;;)
		{
		// find some pages...
		RPageArray::TIter pageList;
		TUint n = pageIter.RemoveFind(pageList);
		if(!n)
			break;

		// unmap some pages...
		aMemory->UnmapPages(pageList,true);

		// free pages...
		TPhysAddr pagePhys;
		while(pageList.Remove(1,&pagePhys))
			{
			if(aPages)
				*aPages++ = pagePhys;
			++numPages;

			__NK_ASSERT_DEBUG((pagePhys&KPageMask)==0);

			TUint index = pageList.Index()-1;
			SPageInfo* pi = SPageInfo::SafeFromPhysAddr(pagePhys);
			if(!pi)
				TheMmu.CleanAndInvalidatePages(&pagePhys,1,aMemory->Attributes(),index);
			else
				{
				MmuLock::Lock();
				UnassignPage(aMemory,index,pi);
				MmuLock::Unlock();
				}
			}

		// move on...
		pageIter.FindRelease(n);
		}

	aMemory->iPages.FindEnd(aIndex,aCount);

	return numPages;
	}


void DHardwareMemoryManager::AssignPage(DMemoryObject* aMemory, TUint aIndex, SPageInfo* aPageInfo)
	{
	TRACE2(("DHardwareMemoryManager::AssignPage(0x%08x,0x%x,phys=0x%08x)",aMemory, aIndex, aPageInfo->PhysAddr()));
 	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aPageInfo->Type()==SPageInfo::EPhysAlloc);
	TUint flags = aMemory->PageInfoFlags();
	if(aPageInfo->UseCount()==0)
		{
		// not mapped yet...
		aPageInfo->SetMapped(aIndex,flags);
		}
	else
		{
		// already mapped somewhere...
		TMemoryType type = (TMemoryType)(flags&KMemoryTypeMask);
		if(CacheMaintenance::IsCached(type))
			{
			// memory is cached at L1, check colour matches existing mapping...
			if( (aPageInfo->Index()^aIndex) & KPageColourMask )
				{
				#ifdef _DEBUG
					Kern::Printf("DHardwareMemoryManager::AssignPage BAD COLOUR");
					aPageInfo->Dump();
				#endif
				__NK_ASSERT_ALWAYS(0);
				}
			}
		// check memory type matches existing mapping...
		if( (aPageInfo->Flags()^flags) & EMemoryAttributeMask )
			{
			#ifdef _DEBUG
				Kern::Printf("DHardwareMemoryManager::AssignPage BAD MEMORY TYPE");
				aPageInfo->Dump();
			#endif
			__NK_ASSERT_ALWAYS(0);
			}
		}
	aPageInfo->IncUseCount();
	TRACE2(("DHardwareMemoryManager::AssignPage iUseCount=%d",aPageInfo->UseCount()));
	}


void DHardwareMemoryManager::UnassignPage(DMemoryObject* aMemory, TUint aIndex, SPageInfo* aPageInfo)
	{
	TRACE2(("DHardwareMemoryManager::UnassignPage(0x%08x,0x%x,phys=0x%08x)",aMemory, aIndex, aPageInfo->PhysAddr()));
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	TRACE2(("DHardwareMemoryManager::UnassignPage iUseCount=%d",aPageInfo->UseCount()));
	__NK_ASSERT_DEBUG(aPageInfo->UseCount());
	if(!aPageInfo->DecUseCount())
		{
		// page no longer being used by any memory object, make sure it's contents
		// are purged from the cache...
		TPhysAddr pagePhys = aPageInfo->PhysAddr();
		aPageInfo->SetModifier(&pagePhys);
		MmuLock::Unlock();
		TheMmu.CleanAndInvalidatePages(&pagePhys,1,aMemory->Attributes(),aIndex);
		MmuLock::Lock();
		if(!aPageInfo->CheckModified(&pagePhys)) // if page has not been reused...
			aPageInfo->SetUncached();			 //     we know the memory is not in the cache
		}
	}


TInt DHardwareMemoryManager::Pin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs)
	{
	return ((DUnpagedMemoryManager*)this)->DUnpagedMemoryManager::Pin(aMemory,aMapping,aPinArgs);
	}


void DHardwareMemoryManager::Unpin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs)
	{
	((DUnpagedMemoryManager*)this)->DUnpagedMemoryManager::Unpin(aMemory,aMapping,aPinArgs);
	}



//
// DPagedMemoryManager
//

TInt DPagedMemoryManager::New(DMemoryObject*& aMemory, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags)
	{
	return DMemoryManager::New(aMemory, aSizeInPages, aAttributes, (TMemoryCreateFlags)(aCreateFlags | EMemoryCreateDemandPaged));
	}


void DPagedMemoryManager::Destruct(DMemoryObject* aMemory)
	{
	((DUnpagedMemoryManager*)this)->DUnpagedMemoryManager::Destruct(aMemory);
	}


TInt DPagedMemoryManager::StealPage(DMemoryObject* aMemory, SPageInfo* aPageInfo)
	{
	return ((DDiscardableMemoryManager*)this)->DDiscardableMemoryManager::StealPage(aMemory,aPageInfo);
	}


TInt DPagedMemoryManager::MovePage(	DMemoryObject* aMemory, SPageInfo* aOldPageInfo, 
									TPhysAddr& aNewPage, TUint aBlockZoneId, TBool aBlockRest)
	{
	return TheMovableMemoryManager->MovePage(aMemory, aOldPageInfo, aNewPage, aBlockZoneId, aBlockRest);
	}


TInt DPagedMemoryManager::RestrictPage(DMemoryObject* aMemory, SPageInfo* aPageInfo, TRestrictPagesType aRestriction)
	{
	TRACE2(("DPagedMemoryManager::RestrictPage(0x%08x,0x%08x,%d)",aMemory,aPageInfo,aRestriction));
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	TUint index = aPageInfo->Index();
	TInt r = KErrNotFound;

	TPhysAddr page;
	TPhysAddr originalPage;
	RPageArray::TIter pageList;
	TPhysAddr* p = aMemory->iPages.RestrictPageNAStart(index,pageList);
	if(!p)
		goto fail;
	originalPage = *p;
	__NK_ASSERT_DEBUG((originalPage&~KPageMask)==aPageInfo->PhysAddr());

	aPageInfo->SetModifier(&pageList);

	MmuLock::Unlock();

	// restrict page...
	aMemory->RestrictPages(pageList,aRestriction);

	MmuLock::Lock();

	page = *p;
	if(aPageInfo->CheckModified(&pageList) || page!=originalPage/*page state changed*/)
		r = KErrInUse;
	else
		{
		// nobody else has modified page state, so restrictions successfully applied...
		*p = (page&~RPageArray::EStateMask)|RPageArray::ECommitted; // restore state
		aPageInfo->SetReadOnly();
		r = KErrNone;
		}

	aMemory->iPages.RestrictPageNAEnd(index);

#ifdef _DEBUG
	if(r!=KErrNone)
		TRACE2(("DPagedMemoryManager::RestrictPage fail because preempted or vetoed"));
#endif

fail:
	TRACE2(("DPagedMemoryManager::RestrictPage returns %d",r));
	return r;
	}


TInt DPagedMemoryManager::HandleFault(	DMemoryObject* aMemory, TUint aIndex, DMemoryMapping* aMapping, 
										TUint aMapInstanceCount, TUint aAccessPermissions)
	{
	TPinArgs pinArgs;
	pinArgs.iReadOnly = !(aAccessPermissions&EReadWrite);

	TUint usedNew = 0;

	RPageArray::TIter pageList;
	TPhysAddr* p = aMemory->iPages.AddPageStart(aIndex,pageList);
	__NK_ASSERT_ALWAYS(p); // we should never run out of memory handling a paging fault

	TInt r = 1; // positive value to indicate nothing done

	// if memory object already has page, then we can use it...
	MmuLock::Lock();
	if(RPageArray::IsPresent(*p))
		{
		r = PageInDone(aMemory,aIndex,0,p);
		__NK_ASSERT_DEBUG(r<=0); // can't return >0 as we didn't supply a new page
		}
	MmuLock::Unlock();

	if(r>0)
		{
		// need to read page from backing store...

		// get paging request object...
		DPageReadRequest* req;
		do
			{
			r = AcquirePageReadRequest(req,aMemory,aIndex,1);
			__NK_ASSERT_DEBUG(r!=KErrNoMemory); // not allowed to allocated memory, therefore can't fail with KErrNoMemory
			if(r==KErrNone)
				{
				// if someone else has since read our page, then we can use it...
				MmuLock::Lock();
				r = 1;
				if(RPageArray::IsPresent(*p))
					{
					r = PageInDone(aMemory,aIndex,0,p);
					__NK_ASSERT_DEBUG(r<=0); // can't return >0 as we didn't supply a new page
					}
				MmuLock::Unlock();
				}
			}
		while(r>0 && !req); // while not paged in && don't have a request object

		if(r>0)
			{
			// still need to read page from backing store...

			// get RAM page...
			TPhysAddr pagePhys;
			r = ThePager.PageInAllocPages(&pagePhys,1,aMemory->RamAllocFlags());
			__NK_ASSERT_DEBUG(r!=KErrNoMemory);
			if(r==KErrNone)
				{
				// read data for page...
				r = ReadPages(aMemory,aIndex,1,&pagePhys,req);
				__NK_ASSERT_DEBUG(r!=KErrNoMemory); // not allowed to allocated memory, therefore can't fail with KErrNoMemory
				if(r!=KErrNone)
					{
					// error, so free unused pages...
					ThePager.PageInFreePages(&pagePhys,1);
					}
				else
					{
					// use new page...
					MmuLock::Lock();
					r = PageInDone(aMemory,aIndex,SPageInfo::FromPhysAddr(pagePhys),p);
					MmuLock::Unlock();
					if(r>0)
						{
						// new page actually used...
						r = KErrNone;
						usedNew = 1;
						}
					}
				}
			}

		// done with paging request object...
		if(req)
			req->Release();
		}

	// map page...
	if(r==KErrNone && aMapping)
		{
		r = aMapping->PageIn(pageList, pinArgs, aMapInstanceCount);
		__NK_ASSERT_ALWAYS(r!=KErrNoMemory); // we should never run out of memory handling a paging fault
		#ifdef COARSE_GRAINED_TLB_MAINTENANCE
		InvalidateTLB();
		#endif
		}

	// finished with this page...
	aMemory->iPages.AddPageEnd(aIndex,usedNew);

	__NK_ASSERT_ALWAYS(r!=KErrNoMemory); // we should never run out of memory handling a paging fault
	return r;
	}


TInt DPagedMemoryManager::Pin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs)
	{
	__ASSERT_CRITICAL;
	return DoPin(aMemory,aMapping->iStartIndex,aMapping->iSizeInPages,aMapping,aPinArgs);
	}


TInt DPagedMemoryManager::DoPin(DMemoryObject* aMemory, TUint aIndex, TUint aCount, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs)
	{
	TRACE(("DPagedMemoryManager::DoPin(0x%08x,0x%08x,0x%08x,0x%08x)",aMemory, aIndex, aCount, aMapping));
	__ASSERT_CRITICAL;
	__NK_ASSERT_DEBUG(aPinArgs.HaveSufficientPages(aCount));
	__NK_ASSERT_DEBUG(aMapping->IsPinned());
	__NK_ASSERT_DEBUG(!aMapping->PagesPinned());

	// check and allocate page array entries...
	RPageArray::TIter pageList;
	TInt r = aMemory->iPages.AddStart(aIndex,aCount,pageList,true);
	if(r!=KErrNone)
		return r;

	RPageArray::TIter pageIter = pageList;
	TUint n;
	TPhysAddr* pages;
	while((n=pageIter.Pages(pages,DPageReadRequest::EMaxPages))!=0)
		{
		MmuLock::Lock();

		if(RPageArray::IsPresent(*pages))
			{
			// pin page which is already committed to memory object...
			r = PageInPinnedDone(aMemory,pageIter.Index(),0,pages,aPinArgs);
			__NK_ASSERT_DEBUG(r<=0); // can't return >0 as we didn't supply a new page
			}
		else
			{
			// count consecutive pages which need to be read...
			TUint i;
			for(i=1; i<n; ++i)
				if(RPageArray::IsPresent(pages[i]))
					break;
			n = i;
			r = 1; // positive value to indicate nothing done
			}

		MmuLock::Unlock();

		if(r==KErrNone)
			{
			// successfully pinned one page, so move on to next one...
			pageIter.Skip(1);
			continue;
			}
		else if(r<0)
			{
			// error, so end...
			break;
			}

		// need to read pages from backing store...

		// get paging request object...
		DPageReadRequest* req;
		TUint i;
		do
			{
			i = 0;
			r = AcquirePageReadRequest(req,aMemory,pageIter.Index(),n);
			if(r==KErrNone)
				{
				// see if someone else has since read any of our pages...
				MmuLock::Lock();
				for(; i<n; ++i)
					if(RPageArray::IsPresent(pages[i]))
						break;
				MmuLock::Unlock();
				}
			}
		while(i==n && !req); // while still need all pages && don't have a request object

		// if don't need all pages any more...
		if(i!=n)
			{
			// retry loop...
			if(req)
				req->Release();
			continue;
			}

		// keep count of number of pages actually added to memory object...
		TUint usedNew = 0;

		// get RAM pages...
		TPhysAddr newPages[DPageReadRequest::EMaxPages];
		__NK_ASSERT_DEBUG(n<=DPageReadRequest::EMaxPages);
		r = ThePager.PageInAllocPages(newPages,n,aMemory->RamAllocFlags());
		if(r==KErrNone)
			{
			// read data for pages...
			r = ReadPages(aMemory,pageIter.Index(),n,newPages,req);
			if(r!=KErrNone)
				{
				// error, so free unused pages...
				ThePager.PageInFreePages(newPages,n);
				}
			else
				{
				// use new pages...
				for(i=0; i<n; ++i)
					{
					MmuLock::Lock();
					r = PageInPinnedDone(aMemory,
									pageIter.Index()+i,
									SPageInfo::FromPhysAddr(newPages[i]),
									pages+i,
									aPinArgs
									);
					MmuLock::Unlock();
					if(r>0)
						{
						// new page actually used...
						r = KErrNone;
						++usedNew;
						}
					if(r!=KErrNone)
						{
						// error, so free remaining unused pages...
						ThePager.PageInFreePages(newPages+(i+1),n-(i+1));
						// and update array for any pages already added...
						if(i)
							pageIter.Added(i,usedNew);
						break;
						}
					}
				}
			}

		// done with paging request object...
		if(req)
			req->Release();

		if(r!=KErrNone)
			break; // error, so give up

		// move on to next set of pages...
		pageIter.Added(n,usedNew);
		}

	// map pages...
	if(r==KErrNone)
		{// Page in the page with the pinning mapping, OK to get the instance count here 
		// without any locking as the pinned mapping can't be reused for another purpose 
		// during this method.
		r = aMapping->PageIn(pageList, aPinArgs, aMapping->MapInstanceCount());
		#ifdef COARSE_GRAINED_TLB_MAINTENANCE
		InvalidateTLB();
		#endif
		}

	// release page array entries...
	aMemory->iPages.AddEnd(aIndex,aCount);

	if(r==KErrNone)
		{
		// set EPagesPinned flag to indicate success...
		__NK_ASSERT_DEBUG((aMapping->Flags()&DMemoryMapping::EPagesPinned)==0);
		__e32_atomic_ior_ord8(&aMapping->Flags(), (TUint8)DMemoryMapping::EPagesPinned);
		}
	else
		{
		// cleanup on error...
		TUint pinnedCount = pageIter.Index()-aIndex; // number of pages actually pinned
		DoUnpin(aMemory,aIndex,pinnedCount,aMapping,aPinArgs);
		}

	return r;
	}


void DPagedMemoryManager::Unpin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs)
	{
	__ASSERT_CRITICAL;
	// if mapping successfully pinned...
	if(aMapping->Flags()&DMemoryMapping::EPagesPinned)
		{
		// then undo pinning...
		DoUnpin(aMemory,aMapping->iStartIndex,aMapping->iSizeInPages,aMapping,aPinArgs);
		}
	}


void DPagedMemoryManager::DoUnpin(DMemoryObject* aMemory, TUint aIndex, TUint aCount, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs)
	{
	TRACE(("DPagedMemoryManager::DoUnpin(0x%08x,0x%08x,0x%08x,0x%08x,?)",aMemory, aIndex, aCount, aMapping));
	__ASSERT_CRITICAL;

	MmuLock::Lock();
	TUint endIndex = aIndex+aCount;
	for(TUint i=aIndex; i<endIndex; ++i)
		{
		TPhysAddr page = aMemory->iPages.Page(i);
		__NK_ASSERT_DEBUG(RPageArray::IsPresent(page));
		__NK_ASSERT_DEBUG(SPageInfo::SafeFromPhysAddr(page&~KPageMask));
		ThePager.Unpin(SPageInfo::FromPhysAddr(page),aPinArgs);
		MmuLock::Flash();
		}
	MmuLock::Unlock();

	// clear EPagesPinned flag...
	__e32_atomic_and_ord8(&aMapping->Flags(), TUint8(~DMemoryMapping::EPagesPinned));
	}


void DPagedMemoryManager::DoCleanupDecommitted(DMemoryObject* aMemory)
	{
	MemoryObjectLock::Lock(aMemory);
	FreeDecommitted(aMemory,0,aMemory->iSizeInPages);
	MemoryObjectLock::Unlock(aMemory);
	}


TInt DPagedMemoryManager::PageInDone(DMemoryObject* aMemory, TUint aIndex, SPageInfo* aPageInfo, TPhysAddr* aPageArrayEntry)
	{
	TInt r = DoPageInDone(aMemory,aIndex,aPageInfo,aPageArrayEntry,false);

	if(r>=0)
		ThePager.PagedIn(aPageInfo);

	// check page assigned correctly...
#ifdef _DEBUG
	if(RPageArray::IsPresent(*aPageArrayEntry))
		{
		SPageInfo* pi = SPageInfo::FromPhysAddr(*aPageArrayEntry);
		__NK_ASSERT_DEBUG(pi->Owner()==aMemory);
		__NK_ASSERT_DEBUG(pi->Index()==aIndex);
		}
#endif

	return r;
	}


TInt DPagedMemoryManager::PageInPinnedDone(DMemoryObject* aMemory, TUint aIndex, SPageInfo* aPageInfo, TPhysAddr* aPageArrayEntry, TPinArgs& aPinArgs)
	{
	TInt r = DoPageInDone(aMemory,aIndex,aPageInfo,aPageArrayEntry,true);

	if(r>=0)
		ThePager.PagedInPinned(aPageInfo,aPinArgs);

	// check page assigned correctly...
#ifdef _DEBUG
	if(RPageArray::IsPresent(*aPageArrayEntry))
		{
		SPageInfo* pi = SPageInfo::FromPhysAddr(*aPageArrayEntry);
		__NK_ASSERT_DEBUG(pi->Owner()==aMemory);
		__NK_ASSERT_DEBUG(pi->Index()==aIndex);
		if(r>=0)
			__NK_ASSERT_DEBUG(pi->PagedState()==SPageInfo::EPagedPinned);
		}
#endif

	return r;
	}


TInt DPagedMemoryManager::DoPageInDone(DMemoryObject* aMemory, TUint aIndex, SPageInfo*& aPageInfo, TPhysAddr* aPageArrayEntry, TBool aPinning)
	{
	TRACE(("DPagedMemoryManager::DoPageInDone(0x%08x,0x%08x,0x%08x,?,%d)",aMemory,aIndex,aPageInfo,aPinning));
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	__UNLOCK_GUARD_START(MmuLock);

	SPageInfo* pi = aPageInfo;

	if(!IsAllocated(aMemory,aIndex,1))
		{
		// memory has been decommitted from memory object...
		if(pi)
			ThePager.PagedInUnneeded(pi);
		__UNLOCK_GUARD_END(MmuLock);
		aPageInfo = 0;
		return KErrNotFound;
		}

	TPhysAddr oldPage = *aPageArrayEntry;
	TBool useNew = (bool)!RPageArray::IsPresent(oldPage);
	if(useNew)
		{
		if(!pi)
			{
			__UNLOCK_GUARD_END(MmuLock);
			// aPageInfo = 0; // this is already set to zero
			return KErrNotFound; // no new page to use
			}

		// assign page to memory object...
		pi->SetManaged(aMemory,aIndex,aMemory->PageInfoFlags());

		ThePager.Event(DPager::EEventPageInNew,pi);

		// save any paging manager data stored in page array before we overwrite it...
		pi->SetPagingManagerData(*aPageArrayEntry);
		}
	else
		{
		__NK_ASSERT_DEBUG(!pi); // should only have read new page if none present

		// discard new page...
		if(pi)
			ThePager.PagedInUnneeded(pi);

		// check existing page can be committed...
		if(RPageArray::State(oldPage)<=RPageArray::EDecommitting)
			{
			__UNLOCK_GUARD_END(MmuLock);
			aPageInfo = 0;
			return KErrNotFound;
			}

		// and use one we already have...
		SPageInfo* newPage = SPageInfo::FromPhysAddr(oldPage);

		if(!pi && !aPinning)
			ThePager.Event(DPager::EEventPageInAgain,newPage);
		
		pi = newPage;
		pi->SetModifier(0); // so observers see page state has changed
		}

	// set page array entry...
	TPhysAddr pagePhys = pi->PhysAddr();
	*aPageArrayEntry = pagePhys|RPageArray::ECommitted;

	// return the page we actually used...
	aPageInfo = pi;

	__UNLOCK_GUARD_END(MmuLock);
	return useNew;
	}


TInt DPagedMemoryManager::Decompress(TUint32 aCompressionType, TLinAddr aDst, TUint aDstBytes, TLinAddr aSrc, TUint aSrcBytes)
	{
#ifdef BTRACE_PAGING_VERBOSE
	BTraceContext4(BTrace::EPaging, BTrace::EPagingDecompressStart, aCompressionType);
#endif
	TInt r;
	switch(aCompressionType)
		{
	case 0:
		__NK_ASSERT_DEBUG(aSrcBytes == aDstBytes);
		memcpy((void*)aDst, (void*)aSrc, aSrcBytes);
		r = aSrcBytes;
		break;

	case SRomPageInfo::EBytePair:
	case KUidCompressionBytePair:
		{
		TUint8* srcNext = 0;
		START_PAGING_BENCHMARK;
		r = BytePairDecompress((TUint8*)aDst, aDstBytes, (TUint8*)aSrc, aSrcBytes, srcNext);
		END_PAGING_BENCHMARK(EPagingBmDecompress);
		if (r > 0)
			{
			// decompression successful so check srcNext points to the end of the compressed data...
			__NK_ASSERT_ALWAYS((TLinAddr)srcNext == aSrc + aSrcBytes);
			}
		}
		break;

	default:
		r = KErrNotSupported;
		break;
		}
#ifdef BTRACE_PAGING_VERBOSE
	BTraceContext0(BTrace::EPaging, BTrace::EPagingDecompressEnd);
#endif
	return r;
	}


TInt DPagedMemoryManager::AcquirePageWriteRequest(DPageWriteRequest*& aRequest, DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	__NK_ASSERT_ALWAYS(0);
	return KErrNotSupported;
	}
TZonePageType DPagedMemoryManager::PageType()
	{// Paged manager's pages should be discardable and will actaully be freed by 
	// the pager so this value won't be used.
	return EPageDiscard;
	}

