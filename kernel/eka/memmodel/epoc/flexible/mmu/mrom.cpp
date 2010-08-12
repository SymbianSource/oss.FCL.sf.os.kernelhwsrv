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
#include <kernel/cache.h>
#include "mm.h"
#include "mmu.h"
#include "mrom.h"
#include "mpager.h"
#include "mmanager.h"
#include "mobject.h"
#include "mmapping.h"
#include "maddrcont.h"
#include "mptalloc.h"
#include "mlargemappings.h"

#include "cache_maintenance.inl"


/**
Class representing the resources allocated for a ROM shadow page.

A shadow page is a page of RAM which is mapped by the MMU to replace
a prior existing page at a particular virtual address.
*/
class DShadowPage : public DVirtualPinMapping
	{
public:
	/**
	Create a new #DShadowPage to shadow a specified memory page.

	On success, #iOriginalPage holds the physical address of the original page
	and #iNewPage the physical address of the newly allocated RAM page; the
	contents of this are a copy of the original.

	No MMU entries for the shadow page are changed - it is the responsibility
	of the caller to handle this. However, the new #DShadowPage object will
	have pinned the page table used by \a aMapping which maps the page being
	shadowed, prevent demand paging from discarding any modifications made to
	this.

	@param aMemory		The memory object whose memory is to be shadowed.
	@param aIndex		Page index, within the memory, of the page to shadow.
	@param aMapping		A memory mapping which currently maps the page to be
						shadowed.

	@return The newly created DShadowPage or the null pointer if there was
			insufficient memory.
	*/
	static DShadowPage* New(DMemoryObject* aMemory, TUint aIndex, DMemoryMappingBase* aMapping);

	/**
	Free the allocated shadow page (#iNewPage) and unpin any pages table which
	was pinned, then free this shadow page object.

	The called of this function must ensure that all references to the shadow
	RAM page have been removed from any MMU mappings.
	*/
	void Destroy();

private:
	DShadowPage();
	~DShadowPage();

	/**
	Second phase constructor. For arguments, see #New.
	*/
	TInt Construct(DMemoryObject* aMemory, TUint aIndex, DMemoryMappingBase* aMapping);

public:
	/**
	The physical address of the original page being shadowed.
	*/
	TPhysAddr iOriginalPage;

	/**
	The physical address of the allocated shadow page.
	*/
	TPhysAddr iNewPage;
	};


/**
Specialised manager for the memory object representing the system ROM.
This handles demand paging of the ROM contents if it is not stored in a memory
device capable of execute-in-place random access. E.g. when stored in NAND
flash.
*/
class DRomMemoryManager : public DPagedMemoryManager
	{
public:
	DRomMemoryManager();

	/**
	Allocate a shadow page for the specified ROM address.

	Shadow pages are pages of RAM which are mapped by the MMU so that 
	they replace the original ROM memory. The contents of a shadow page
	are initially the same as the ROM they replace, but may be modified with
	#CopyToShadowMemory.

	@param aRomAddr	An virtual address which lies within the ROM.

	@return KErrNone if successful,
			KErrAlreadyExists if the specified address already has a show page,
			otherwise one of the system wide error codes.
	*/
	TInt AllocShadowPage(TLinAddr aRomAddr);

	/**
	Free a shadow page previously allocated with #AllocShadowPage.

	The original ROM memory page is again mapped at the specified address.

	@param aRomAddr	An virtual address which lies within the ROM.

	@return KErrNone if successful,
			otherwise one of the system wide error codes.
	*/
	TInt FreeShadowPage(TLinAddr aRomAddr);

	/**
	Copy data into a shadow page, modifying its contents.

	@param aDst		An virtual address which lies within the ROM for which a shadow
					page has previously been allocated with #AllocShadowPage.
	@param aSrc		The start address of the data to copy to \a aDst.
	@param aSize	The size, in bytes, of the data to copy.

	@return KErrNone if successful,
			KErrNotFound if the specified address didn't have a shadow page,
			otherwise one of the system wide error codes.
	*/
	TInt CopyToShadowMemory(TLinAddr aDst, TLinAddr aSrc, TUint32 aSize);

protected:

	// from DPagedMemoryManager...
	virtual TInt PageInPinnedDone(DMemoryObject* aMemory, TUint aIndex, SPageInfo* aPageInfo, TPhysAddr* aPageArrayEntry, TPinArgs& aPinArgs);

private:
	// from DMemoryManager...
	virtual void Destruct(DMemoryObject* aMemory);
	virtual TInt HandleFault(	DMemoryObject* aMemory, TUint aIndex, DMemoryMapping* aMapping, 
								TUint aMapInstanceCount, TUint aAccessPermissions);
	virtual TInt Pin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs);
	virtual void Unpin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs);

	// methods inherited from DPagedMemoryManager

	/**
	@copydoc DPagedMemoryManager::Init3
	This acts as a second phase constructor for the manager which
	creates the memory objects and mappings to represent the ROM.
	*/
	virtual void Init3();

	virtual TInt InstallPagingDevice(DPagingDevice* aDevice);
	virtual TInt AcquirePageReadRequest(DPageReadRequest*& aRequest, DMemoryObject* aMemory, TUint aIndex, TUint aCount);
	virtual TInt ReadPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr* aPages, DPageReadRequest* aRequest);
	virtual TBool IsAllocated(DMemoryObject* aMemory, TUint aIndex, TUint aCount);
	virtual void DoUnpin(DMemoryObject* aMemory, TUint aIndex, TUint aCount, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs);

	/**
	Acquire the mutex used to protect shadow page allocation.
	*/
	void ShadowLock();

	/**
	Release the mutex used to protect shadow page allocation.
	*/
	void ShadowUnlock();

private:
	/**
	The ROM paging device which was passed to #InstallPagingDevice.
	*/
	DPagingDevice* iDevice;

	/**
	The memory object containing the ROM.
	*/
	DMemoryObject* iRomMemory;

	/**
	The memory mapping which maps the ROM into a global visible virtual address.
	*/
	DMemoryMapping* iRomMapping;

	/**
	The virtual address for the start of the ROM in the global memory region.
	*/
	TLinAddr iBase;

	/**
	The size, in bytes, of the ROM image.
	This may not be an exact multiple of a page size.
	*/
	TUint iSize;

	/**
	The size, in pages, of the ROM image.
	*/
	TUint iSizeInPages;

	/**
	The offset from the ROM start, in bytes, for the region of the
	ROM which is demand paged.
	*/
	TUint iPagedStart;

	/**
	The size, in bytes, for the region of the ROM which is demand paged.
	*/
	TUint iPagedSize;

	/**
	The address within the ROM for the ROM page index.
	@see TRomHeader::iRomPageIndex.
	*/
	SRomPageInfo* iRomPageIndex;

	/**
	The mutex used to protect shadow page allocation.
	*/
	DMutex* iShadowLock;

	/**
	Container for all allocated DShadowPage objects.
	*/
	RAddressedContainer iShadowPages;

#ifdef __SUPPORT_DEMAND_PAGING_EMULATION__
	TInt iOriginalRomPageCount;
	TPhysAddr* iOriginalRomPages;
	friend void RomOriginalPages(TPhysAddr*& aPages, TUint& aPageCount);
#endif

	friend TBool IsUnpagedRom(TLinAddr aBase, TUint aSize);

public:
	/**
	The single instance of this manager class.
	*/
	static DRomMemoryManager TheManager;
	};


DRomMemoryManager DRomMemoryManager::TheManager;
DPagedMemoryManager* TheRomMemoryManager = &DRomMemoryManager::TheManager;


const TInt KMutexOrdRomMemory = KMutexOrdPageIn+1;


#ifdef __SUPPORT_DEMAND_PAGING_EMULATION__
/**
For use by the emulated paging device to get the location and size of the ROM.

@param aPages		A reference to store a pointer to an array of the physical addresses of each ROM page.
@param aPageCount	A reference to store the number of rom pages.
*/
void RomOriginalPages(TPhysAddr*& aPages, TUint& aPageCount)
	{
	aPages = DRomMemoryManager::TheManager.iOriginalRomPages;
	aPageCount = DRomMemoryManager::TheManager.iOriginalRomPageCount;
	}

#endif


TBool IsUnpagedRom(TLinAddr aBase, TUint aSize)
	{
	TUint offset = aBase-DRomMemoryManager::TheManager.iBase;
	TUint limit = DRomMemoryManager::TheManager.iPagedStart;
	if(offset>=limit)
		return false;
	offset += aSize;
	if(offset>limit || offset<aSize)
		return false;
	return true;
	}


TInt PagifyChunk(TLinAddr aAddress)
	{
	TRACE(("PagifyChunk(0x%08x)",aAddress));

	aAddress &= ~KChunkMask;
	TPde* pPde = Mmu::PageDirectoryEntry(KKernelOsAsid,aAddress);

retry:
	// check there is actually some memory mapped...
	TPde pde = *pPde;
	if(pde==KPdeUnallocatedEntry)
		{
		TRACE(("PagifyChunk returns %d",KErrNotFound));
		return KErrNotFound;
		}

	// end if memory is not a section mapping...
	TPhysAddr pdePhys = Mmu::PdePhysAddr(pde);
	if(pdePhys==KPhysAddrInvalid)
		{
		TRACE(("PagifyChunk returns %d",KErrAlreadyExists));
		return KErrAlreadyExists;
		}

	// get a new page table...
	::PageTables.Lock();
	TPte* pt = ::PageTables.Alloc(false);
	if(!pt)
		{
		TRACE(("PagifyChunk returns %d",KErrNoMemory));
		::PageTables.Unlock();
		return KErrNoMemory;
		}

	// fill page table so it maps the same physical addresses as the section mapping...
	TPte pte = Mmu::SectionToPageEntry(pde);
	pte |= pdePhys;
	TPte* pPte = pt;
	do
		{
		TRACE2(("!PTE %x=%x",pPte,pte));
		*pPte++ = pte;
		pte += KPageSize;
		}
	while(TLinAddr(pPte)&(KPageTableMask/sizeof(TPte)*sizeof(TPte)));
	CacheMaintenance::MultiplePtesUpdated((TLinAddr)pt,KPageTableSize);

	// check memory not changed...
	MmuLock::Lock();
	if(Mmu::PdePhysAddr(*pPde)!=pdePhys)
		{
		// pde was changed whilst we were creating a new page table, need to retry...
		MmuLock::Unlock();
		::PageTables.Free(pt);
		::PageTables.Unlock();
		goto retry;
		}

	// update page counts...
	SPageTableInfo* pti = SPageTableInfo::FromPtPtr(pt);
	TUint count = pti->IncPageCount(KPageTableSize/sizeof(TPte));
	(void)count;
	TRACE2(("pt %x page count=%d",pt,pti->PageCount()));
	__NK_ASSERT_DEBUG(pti->CheckPageCount());

	// swap pde entry to point to new page table...
	pde |= Mmu::PageTablePhysAddr(pt);
	TRACE2(("!PDE %x=%x",pPde,pde));
	*pPde = pde;
	SinglePdeUpdated(pPde);
	InvalidateTLB();

	// done...
	MmuLock::Unlock();
	::PageTables.Unlock();
	TRACE(("PagifyChunk returns %d",KErrNone));
	return KErrNone;
	}


void UnmapROM(TLinAddr aStart, TLinAddr aEnd)
	{
	TRACEB(("UnmapROM 0x%08x..0x%08x",aStart,aEnd));

	TLinAddr p = aStart;
	if(p>=aEnd)
		return;

	PagifyChunk(p);

	MmuLock::Lock(); // hold MmuLock for long time, shouldn't matter as this is only done during boot

	TPte* pPte = Mmu::PtePtrFromLinAddr(p,KKernelOsAsid);
	__NK_ASSERT_ALWAYS(pPte);
	while(p<aEnd && p&KChunkMask)
		{
		*pPte++ = KPteUnallocatedEntry;
		p += KPageSize;
		}

	if(p<aEnd)
		{
		TPde* pPde = Mmu::PageDirectoryEntry(KKernelOsAsid,p);
		while(p<aEnd)
			{
			*pPde++ = KPdeUnallocatedEntry;
			p += KChunkSize;
			}
		}

	MmuLock::Unlock();

	__NK_ASSERT_DEBUG(p==aEnd);
	}


DRomMemoryManager::DRomMemoryManager()
	: iShadowPages(0,iShadowLock)
	{
	}


void DRomMemoryManager::Init3()
	{
	// get ROM info...
	const TRomHeader& romHeader = TheRomHeader();
	iBase = (TLinAddr)&romHeader;
	iSize = romHeader.iUncompressedSize;
	iSizeInPages = MM::RoundToPageCount(iSize);
	TUint chunkSize = ((iSize+KChunkMask)&~KChunkMask);
	TUint committedSize = TheSuperPage().iTotalRomSize; // size of memory loaded by bootstrap
	TRACEB(("DRomMemoryManager::Init3 rom=0x%08x+0x%x",iBase,iSize));

	// get paged rom info...
	if(romHeader.iRomPageIndex)
		iRomPageIndex = (SRomPageInfo*)((TInt)&romHeader+romHeader.iRomPageIndex);
	iPagedSize = romHeader.iPageableRomSize;
	iPagedStart = iPagedSize ? romHeader.iPageableRomStart : 0;
	if(iPagedStart)
		{
		TRACEB(("DRomMemoryManager::Init3() paged=0x%08x+0x%x",(TLinAddr)&romHeader+iPagedStart,iPagedSize));
		__NK_ASSERT_ALWAYS(iPagedStart<iSize && iPagedStart+iPagedSize>iPagedStart && iPagedStart+iPagedSize<=iSize);

#ifdef __SUPPORT_DEMAND_PAGING_EMULATION__
		// get physical addresses of ROM pages...
		iOriginalRomPageCount = iSizeInPages;
		iOriginalRomPages = new TPhysAddr[iOriginalRomPageCount];
		__NK_ASSERT_ALWAYS(iOriginalRomPages);
		MmuLock::Lock(); // hold MmuLock for long time, shouldn't matter as this is only done during boot
		TInt i;
		for(i=0; i<iOriginalRomPageCount; i++)
			iOriginalRomPages[i] = Mmu::LinearToPhysical(iBase+i*KPageSize);
		MmuLock::Unlock();

		// unmap paged part of ROM as the bootstrap will have left it mapped.
		// See CFG_SupportEmulatedRomPaging in the bootstrap code.
		// todo: use FMM for this after memory object created
		UnmapROM(iBase+iPagedStart,iBase+chunkSize);
		committedSize = iPagedStart;
#endif
		}

	if(iPagedStart && committedSize!=iPagedStart)
		{
		// unmap any paged ROM which the bootstrap mapped...
		TRACEB(("DRomMemoryManager::Init3() unmapping unpaged ROM offsets 0x%x thru 0x%x",iPagedStart,committedSize));
		// todo: use FMM for this after memory object created
		UnmapROM(iBase+iPagedStart,iBase+committedSize);
		committedSize = iPagedStart;
		}

	// create memory object for ROM...
	TRACEB(("DRomMemoryManager::Init3() committed ROM memory 0x%x of 0x%x",committedSize,chunkSize));
	TMemoryCreateFlags flags = (TMemoryCreateFlags)(EMemoryCreateNoWipe | EMemoryCreateReadOnly | 
													EMemoryCreateDemandPaged | EMemoryCreateAllowExecution);
	iRomMemory = DLargeMappedMemory::New(&DRomMemoryManager::TheManager,chunkSize>>KPageShift,EMemoryAttributeStandard,flags);
	__NK_ASSERT_ALWAYS(iRomMemory);
	TInt r = MM::MemoryClaimInitialPages(iRomMemory,iBase,committedSize,EUserExecute,false,true);
	__NK_ASSERT_ALWAYS(r==KErrNone);
	r = iRomMemory->iPages.Alloc(committedSize>>KPageShift,(chunkSize-committedSize)>>KPageShift);
	__NK_ASSERT_ALWAYS(r==KErrNone);

	// create mapping for ROM...
	r = MM::MappingNew(iRomMapping, iRomMemory, EUserExecute, KKernelOsAsid, EMappingCreateExactVirtual, iBase);
	__NK_ASSERT_ALWAYS(r==KErrNone);
	__NK_ASSERT_ALWAYS(iRomMapping->IsLarge());

	// Set the paging device to be uninstalled, i.e. NULL.
	iDevice = NULL;

	_LIT(KRomMemoryLockName,"RomMemory");
	r = K::MutexCreate(iShadowLock, KRomMemoryLockName, NULL, EFalse, KMutexOrdRomMemory);
	__NK_ASSERT_ALWAYS(r==KErrNone);
	MM::MemorySetLock(iRomMemory,iShadowLock);
	}


TInt DRomMemoryManager::InstallPagingDevice(DPagingDevice* aDevice)
	{
	TRACEB(("DRomMemoryManager::InstallPagingDevice(0x%08x)",aDevice));

	if(!iPagedStart)
		{
		TRACEB(("ROM is not paged"));
		return KErrNone;
		}

	TAny* null = 0;
	if(aDevice->iType & DPagingDevice::EMediaExtension)
		__e32_atomic_store_ord_ptr(&iDevice, null);
	if(!__e32_atomic_cas_ord_ptr(&iDevice, &null, aDevice)) // set iDevice=aDevice if it was originally 0
		{
		// ROM paging device already registered...
		TRACEB(("DRomMemoryManager::InstallPagingDevice returns ALREADY EXISTS!"));
		return KErrAlreadyExists;
		}

	__e32_atomic_ior_ord32(&K::MemModelAttributes, (TUint32)EMemModelAttrRomPaging);

	return KErrNone;
	}


TInt DRomMemoryManager::AcquirePageReadRequest(DPageReadRequest*& aRequest, DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	aRequest = iDevice->iRequestPool->AcquirePageReadRequest(aMemory,aIndex,aCount);
	return KErrNone;
	}


void DRomMemoryManager::Destruct(DMemoryObject* aMemory)
	{
	__NK_ASSERT_DEBUG(0);
	}


TInt DRomMemoryManager::ReadPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr* aPages, DPageReadRequest* aRequest)
	{
	__NK_ASSERT_DEBUG(aRequest->CheckUseContiguous(aMemory,aIndex,aCount));
	__ASSERT_CRITICAL;

	TLinAddr linAddr = aRequest->MapPages(aIndex,aCount,aPages);
	TInt r = KErrNone;
	TThreadMessage message;

	const TInt readUnitShift = iDevice->iReadUnitShift;

	for(; aCount; ++aIndex, --aCount, linAddr+=KPageSize)
		{
		
		START_PAGING_BENCHMARK;
		if(!iRomPageIndex)
			{
			// ROM not broken into pages, so just read it in directly.
			// KPageShift > readUnitShift so page size is exact multiple of read 
			// units.  Therefore it is ok to just shift offset and KPageSize 
			// by readUnitShift.
			const TInt dataOffset = aIndex << KPageShift;
			START_PAGING_BENCHMARK;
			r = iDevice->Read(	&message, 
								linAddr, dataOffset >> readUnitShift, 
								KPageSize >> readUnitShift, DPagingDevice::EDriveRomPaging);
			__NK_ASSERT_DEBUG(r!=KErrNoMemory); // not allowed to allocated memory, therefore can't fail with KErrNoMemory
			END_PAGING_BENCHMARK(EPagingBmReadMedia);
			if (r != KErrNone)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("DRomMemoryManager::ReadPage: error reading media at %08x + %x: %d", dataOffset, KPageSize, r));
				}
			r = ThePager.EmbedErrorContext(EPagingErrorContextRomRead, r);
			}
		else
			{
			// Work out where data for page is located
			SRomPageInfo* romPageInfo = iRomPageIndex + aIndex;
			const TInt dataOffset = romPageInfo->iDataStart;
			const TInt dataSize = romPageInfo->iDataSize;
			if(!dataSize)
				{
				// empty page, fill it with 0xff...
				memset((TAny*)linAddr, 0xff, KPageSize);
				r = KErrNone;
				}
			else
				{
				__NK_ASSERT_ALWAYS(romPageInfo->iPagingAttributes & SRomPageInfo::EPageable);

				// Read data for page...
				const TLinAddr buffer = aRequest->Buffer();
				const TUint readStart = dataOffset >> readUnitShift;
				const TUint readSize = ((dataOffset + dataSize - 1) >> readUnitShift) - readStart + 1;
				__NK_ASSERT_DEBUG((readSize << readUnitShift) <= (DPageReadRequest::EMaxPages << KPageShift));
				START_PAGING_BENCHMARK;
				r = iDevice->Read(&message, buffer, readStart, readSize, DPagingDevice::EDriveRomPaging);
				__NK_ASSERT_DEBUG(r!=KErrNoMemory); // not allowed to allocated memory, therefore can't fail with KErrNoMemory
				END_PAGING_BENCHMARK(EPagingBmReadMedia);
				if(r!=KErrNone)
					__KTRACE_OPT(KPANIC, Kern::Printf("DRomMemoryManager::ReadPage: error reading media at %08x + %x: %d", dataOffset, dataSize, r));
				r = ThePager.EmbedErrorContext(EPagingErrorContextRomRead, r);
				if(r==KErrNone)
					{
					// Decompress data, remembering that the data to decompress may be offset from 
					// the start of the data just read in, due to reads having to be aligned by 
					// readUnitShift.
					const TLinAddr data = buffer + dataOffset - (readStart << readUnitShift);
					__ASSERT_COMPILE(SRomPageInfo::ENoCompression==0); // decompress assumes this
					r = Decompress(romPageInfo->iCompressionType, linAddr, KPageSize, data, dataSize);
					if (r >= 0)
						r = (r == KPageSize) ? KErrNone : KErrCorrupt;
					if (r != KErrNone)
						__KTRACE_OPT(KPANIC, Kern::Printf("DRomMemoryManager::ReadPage: error decompressing page at %08x + %x: %d", dataOffset, dataSize, r));
					__NK_ASSERT_DEBUG(r == KErrNone);
					r = ThePager.EmbedErrorContext(EPagingErrorContextRomDecompress, r);
					}
				}
			}
		END_PAGING_BENCHMARK(EPagingBmReadRomPage);

		if(r!=KErrNone)
			break;
		}

	aRequest->UnmapPages(true);
	
	return r;
	}


TBool DRomMemoryManager::IsAllocated(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	// all pages in the ROM memory object are always allocated...
	return true;
	}


TInt DRomMemoryManager::HandleFault(DMemoryObject* aMemory, TUint aIndex, DMemoryMapping* aMapping, 
									TUint aMapInstanceCount, TUint aAccessPermissions)
	{
	__NK_ASSERT_DEBUG(aMemory==iRomMemory);

	TUint offset = aIndex*KPageSize;
	if(offset<iPagedStart || offset>=iPagedStart+iPagedSize)
		return KErrAbort;

	return DPagedMemoryManager::HandleFault(aMemory, aIndex, aMapping, aMapInstanceCount, aAccessPermissions);
	}


TInt DRomMemoryManager::Pin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs)
	{
	TRACE(("DRomMemoryManager::Pin %08x %08x", aMemory, aMapping));
	TUint index = aMapping->iStartIndex;
	TUint endIndex = index+aMapping->iSizeInPages;
	if(endIndex>iSizeInPages)
		return KErrNotFound;

	TInt r = KErrNone;
	TUint pagedIndex = iPagedStart>>KPageShift;
	if(pagedIndex && pagedIndex<endIndex)
		{
		TUint start = index;
		if(start<pagedIndex)
			start = pagedIndex;
		r = DoPin(aMemory,start,endIndex-start,aMapping,aPinArgs);
		}

	return r;
	}


TInt DRomMemoryManager::PageInPinnedDone(DMemoryObject* aMemory, TUint aIndex, SPageInfo* aPageInfo, TPhysAddr* aPageArrayEntry, TPinArgs& aPinArgs)
	{
	TRACE(("DRomMemoryManager::PageInPinnedDone %08x %d", aMemory, aIndex));
	
	// Only the paged part of rom should be pinned.
	__NK_ASSERT_DEBUG(aIndex >= iPagedStart >> KPageShift);

	TInt r = DoPageInDone(aMemory,aIndex,aPageInfo,aPageArrayEntry,true);

	// Rom page can't be decommitted so this must succeed.
	__NK_ASSERT_DEBUG(r >= 0);

	if (aPageInfo->Type() == SPageInfo::EShadow)
		{// The page is being shadowed so pin the original page.
		// This is safe as the original page was physically pinned when shadowed.
		__NK_ASSERT_DEBUG(RPageArray::IsPresent(*aPageArrayEntry));
		aPageInfo = aPageInfo->GetOriginalPage();
		}

	ThePager.PagedInPinned(aPageInfo,aPinArgs);

	// check page assigned correctly...
#ifdef _DEBUG
	if(RPageArray::IsPresent(*aPageArrayEntry))
		{
		SPageInfo* pi = SPageInfo::FromPhysAddr(*aPageArrayEntry);
		if (pi->Type() != SPageInfo::EShadow)
			{
			__NK_ASSERT_DEBUG(pi->Type() == SPageInfo::EManaged);
			__NK_ASSERT_DEBUG(pi->Owner()==aMemory);
			__NK_ASSERT_DEBUG(pi->Index()==aIndex);
			__NK_ASSERT_DEBUG(pi->PagedState()==SPageInfo::EPagedPinned);
			}
		}
#endif
	return r;
	}


void DRomMemoryManager::Unpin(DMemoryObject* aMemory, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs)
	{
	TRACE(("DRomMemoryManager::Unpin %08x %08x", aMemory, aMapping));
	
	__ASSERT_CRITICAL;
	TUint index = aMapping->iStartIndex;
	TUint endIndex = index+aMapping->iSizeInPages;
	__NK_ASSERT_DEBUG(endIndex<=iSizeInPages); // Pin() should have already ensured this

	TUint pagedIndex = iPagedStart>>KPageShift;
	if(pagedIndex && pagedIndex<endIndex)
		{
		TUint start = index;
		if(start<pagedIndex)
			start = pagedIndex;
		// unpin pages (but only if they were successfully pinned)...
		if(aMapping->Flags()&DMemoryMapping::EPagesPinned)
			DoUnpin(aMemory,start,endIndex-start,aMapping,aPinArgs);
		}

	__NK_ASSERT_DEBUG((aMapping->Flags()&DMemoryMapping::EPageUnmapVetoed)==0); // we shouldn't have tried to Free paged ROM
	}


void DRomMemoryManager::DoUnpin(DMemoryObject* aMemory, TUint aIndex, TUint aCount, DMemoryMappingBase* aMapping, TPinArgs& aPinArgs)
	{
	TRACE(("DRomMemoryManager::DoUnpin(0x%08x,0x%08x,0x%08x,0x%08x,?)",aMemory, aIndex, aCount, aMapping));

	// This should only be invoked on the paged part of rom.
	__NK_ASSERT_DEBUG(iPagedStart && aIndex >= (iPagedStart >> KPageShift));

	MmuLock::Lock();
	TUint endIndex = aIndex+aCount;
	for(TUint i = aIndex; i < endIndex; ++i)
		{
		TPhysAddr page = aMemory->iPages.Page(i);
		__NK_ASSERT_DEBUG(RPageArray::IsPresent(page));
		SPageInfo* pi = SPageInfo::FromPhysAddr(page);
		if(pi->Type() == SPageInfo::EShadow)
			{
			pi = pi->GetOriginalPage();
			}
		ThePager.Unpin(pi,aPinArgs);
		MmuLock::Flash();
		}

	MmuLock::Unlock();

	// clear EPagesPinned flag...
	__e32_atomic_and_ord8(&aMapping->Flags(), TUint8(~DMemoryMapping::EPagesPinned));
	}


void DRomMemoryManager::ShadowLock()
	{
	MM::MemoryLock(iRomMemory);
	}


void DRomMemoryManager::ShadowUnlock()
	{
	MM::MemoryUnlock(iRomMemory);
	}


TInt DRomMemoryManager::AllocShadowPage(TLinAddr aRomAddr)
	{
	TRACE(("DRomMemoryManager::AllocShadowPage %08x", aRomAddr));
	
	TUint index = (aRomAddr-iBase)>>KPageShift;
	if (index >= iSizeInPages)
		return KErrArgument;
	__NK_ASSERT_DEBUG(iRomMemory->CheckRegion(index,1));

	TInt r;

	ShadowLock();

	DShadowPage* shadow = (DShadowPage*)iShadowPages.Find(index);
	if(shadow)
		r = KErrAlreadyExists;
	else
		{
		shadow = DShadowPage::New(iRomMemory,index,iRomMapping);
		if(!shadow)
			r = KErrNoMemory;
		else
			{
			r = iShadowPages.Add(index,shadow);
			if(r!=KErrNone)
				{
				shadow->Destroy();
				}
			else
				{
				// Remap the shadowed rom page to the shadow page.  Update the 
				// page array entry for the page being shadowed, this ensures 
				// that any page moving attempts will remap the shadow page when
				// they realise that the page is physically pinned.
				MmuLock::Lock();
				TPhysAddr& pageEntry = *iRomMemory->iPages.PageEntry(index);
				TPhysAddr newPageAddr = shadow->iNewPage;
				pageEntry = (pageEntry & KPageMask) | newPageAddr;

				// Mark the SPageInfo of the shadow page with pointer to the original page's
				// SPageInfo, this is safe as we've physically pinned the original page
				// so it can't be freed or reused until this shadow page is destroyed.
				SPageInfo* origPi = SPageInfo::FromPhysAddr(shadow->iOriginalPage);
				SPageInfo* newPi = SPageInfo::FromPhysAddr(newPageAddr);
				newPi->SetOriginalPage(origPi);
				MmuLock::Unlock();

				iRomMemory->RemapPage(pageEntry, index, ETrue);
				}
			}
		}

	ShadowUnlock();

	return r;
	}


TInt DRomMemoryManager::FreeShadowPage(TLinAddr aRomAddr)
	{
	TUint index = (aRomAddr-iBase)>>KPageShift;
	if(!iRomMemory->CheckRegion(index,1))
		return KErrArgument;

	TInt r;

	ShadowLock();

	DShadowPage* shadow = (DShadowPage*)iShadowPages.Remove(index);
	if(!shadow)
		{
		r = KErrNotFound;
		}
	else
		{
		// Remap the rom page and update the page array entry for the page
		// back to the original rom page.  This is safe as the page is physically 
		// pinned until shadow is destroyed.
		MmuLock::Lock();
		TPhysAddr& pageEntry = *iRomMemory->iPages.PageEntry(index);
		pageEntry = (pageEntry & KPageMask) | shadow->iOriginalPage;
		MmuLock::Unlock();

		iRomMemory->RemapPage(pageEntry, index, ETrue);
		
		shadow->Destroy();
		r = KErrNone;
		}

	ShadowUnlock();

	return r;
	}


TInt DRomMemoryManager::CopyToShadowMemory(TLinAddr aDst, TLinAddr aSrc, TUint32 aSize)
	{
	TRACE(("DRomMemoryManager::CopyToShadowMemory(0x%08x,0x%08x,0x%x)",aDst,aSrc,aSize));
	Mmu& m = TheMmu;
	TLinAddr offset = aDst-iBase;
	TLinAddr end = offset+aSize;
	if(end<offset || end>iSize)
		return KErrArgument;

	while(aSize)
		{
		TUint size = KPageSize-(offset&KPageMask); // bytes left in page at 'offset'
		if(size>aSize)
			size = aSize;

		TInt r;

		ShadowLock();

		DShadowPage* shadow = (DShadowPage*)iShadowPages.Find(offset>>KPageShift);
		if(!shadow)
			{
			r = KErrNotFound;
			}
		else
			{
			RamAllocLock::Lock();
			TLinAddr dst = m.MapTemp(shadow->iNewPage,offset>>KPageShift);
			dst += offset&KPageMask;
			memcpy((TAny*)dst,(TAny*)aSrc,size);
			m.UnmapTemp();
			RamAllocLock::Unlock();

			r = KErrNone;
			}

		ShadowUnlock();

		if(r!=KErrNone)
			return r;

		offset += size;
		aSrc += size;
		aSize -= size;
		}

	return KErrNone;
	}


//
// DShadowPage
//

DShadowPage* DShadowPage::New(DMemoryObject* aMemory, TUint aIndex, DMemoryMappingBase* aMapping)
	{
	TRACE(("DShadowPage::New(0x%08x,0x%x,0x%08x)",aMemory, aIndex, aMapping));
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	DShadowPage* self = new DShadowPage;
	if(self)
		if(self->Construct(aMemory,aIndex,aMapping)!=KErrNone)
			{
			self->Destroy();
			self = 0;
			}

	TRACE(("DShadowPage::New(0x%08x,0x%x,0x%08x) returns 0x%08x",aMemory, aIndex, aMapping, self));
	return self;
	}


DShadowPage::DShadowPage()
	: iOriginalPage(KPhysAddrInvalid), iNewPage(KPhysAddrInvalid)
	{
	// Set flag so that the rom page that is being shadowed can't be moved, 
	// otherwise iOriginalPage will become invalid if the page is moved.
	Flags() |= EPhysicalPinningMapping;
	}




TInt DShadowPage::Construct(DMemoryObject* aMemory, TUint aIndex, DMemoryMappingBase* aMapping)
	{
	__NK_ASSERT_DEBUG(MemoryObjectLock::IsHeld(aMemory));

	// Pin the page.  It is ok to get the mapping instance count here without
	// MmuLock as there is only one permenant mapping used for the ROM.
	TInt r = Pin(aMemory,aIndex,1,EUserReadOnly,aMapping,aMapping->MapInstanceCount());
	if(r!=KErrNone)
		return r;

	r = PhysAddr(0,1,iOriginalPage,0);
	__NK_ASSERT_DEBUG(r>=0);
	if(r<0)
		return r;

	RamAllocLock::Lock();

	Mmu& m = TheMmu;
	// Allocate a page to shadow to allowing the allocation to steal pages from the paging cache.
	r = m.AllocRam(	&iNewPage, 
					1, 
					(Mmu::TRamAllocFlags)(aMemory->RamAllocFlags() & ~Mmu::EAllocNoPagerReclaim), 
					EPageFixed);
	if(r==KErrNone)
		{
		TLinAddr dst = m.MapTemp(iNewPage,aIndex,0);
		TLinAddr src = m.MapTemp(iOriginalPage,aIndex,1);
		pagecpy((TAny*)dst,(TAny*)src);
		CacheMaintenance::CodeChanged(dst,KPageSize); // IMB not needed, just clean to PoU (but we don't have a function to do that)

		m.UnmapTemp(0);
		m.UnmapTemp(1);
		MmuLock::Lock();
		SPageInfo::FromPhysAddr(iNewPage)->SetShadow(aIndex,aMemory->PageInfoFlags());
		MmuLock::Unlock();

#ifdef BTRACE_KERNEL_MEMORY
		BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryMiscAlloc, KPageSize);
		++Epoc::KernelMiscPages;
#endif
		}

	RamAllocLock::Unlock();

	if(r!=KErrNone)
		return r;

	return r;
	}


DShadowPage::~DShadowPage()
	{
	}


void DShadowPage::Destroy()
	{
	TRACE2(("DShadowPage[%x]::Destroy()",this));
	if(iNewPage!=KPhysAddrInvalid)
		{
		RamAllocLock::Lock();
		TheMmu.FreeRam(&iNewPage, 1, EPageFixed);

#ifdef BTRACE_KERNEL_MEMORY
		BTrace4(BTrace::EKernelMemory, BTrace::EKernelMemoryMiscFree, KPageSize);
		--Epoc::KernelMiscPages;
#endif
		RamAllocLock::Unlock();
		}
	if(IsAttached())
		Unpin();
	Close();
	}


/**
Replace a page of the system's execute-in-place (XIP) ROM image with a page of
RAM having the same contents. This RAM can subsequently be written to in order
to apply patches to the XIP ROM or to insert software breakpoints for debugging
purposes.
Call Epoc::FreeShadowPage() when you wish to revert to the original ROM page.

@param	aRomAddr	The virtual address of the ROM page to be replaced.
@return	KErrNone if the operation completed successfully.
		KErrArgument if the specified address is not a valid XIP ROM address.
		KErrNoMemory if the operation failed due to insufficient free RAM.
		KErrAlreadyExists if the XIP ROM page at the specified address has
			already been shadowed by a RAM page.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
*/
EXPORT_C TInt Epoc::AllocShadowPage(TLinAddr aRomAddr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::AllocShadowPage");
	return DRomMemoryManager::TheManager.AllocShadowPage(aRomAddr);
	}


/**
Copies data into shadow memory. Source data is presumed to be in Kernel memory.

@param	aSrc	Data to copy from.
@param	aDest	Address to copy into.
@param	aLength	Number of bytes to copy. Maximum of 32 bytes of data can be copied.

@return	KErrNone 		if the operation completed successfully.
		KErrArgument 	if any part of destination region is not shadow page or
						if aLength is greater then 32 bytes.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
*/
EXPORT_C TInt Epoc::CopyToShadowMemory(TLinAddr aDest, TLinAddr aSrc, TUint32 aLength)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::CopyToShadowMemory");
	return DRomMemoryManager::TheManager.CopyToShadowMemory(aDest,aSrc,aLength);
	}


/**
Revert an XIP ROM address which has previously been shadowed to the original
page of ROM.

@param	aRomAddr	The virtual address of the ROM page to be reverted.
@return	KErrNone if the operation completed successfully.
		KErrArgument if the specified address is not a valid XIP ROM address.
		KErrGeneral if the specified address has not previously been shadowed
			using Epoc::AllocShadowPage().

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
*/
EXPORT_C TInt Epoc::FreeShadowPage(TLinAddr aRomAddr)
	{
	return DRomMemoryManager::TheManager.FreeShadowPage(aRomAddr);
	}


/**
Change the permissions on an XIP ROM address which has previously been shadowed
by a RAM page so that the RAM page may no longer be written to.

Note: Shadow page on the latest platforms (that use the reduced set of access permissions:
arm11mpcore, arm1176, cortex) is implemented with read only permissions. Therefore, calling
this function in not necessary, as shadow page is already created as 'frozen'.

@param	aRomAddr	The virtual address of the shadow RAM page to be frozen.
@return	KErrNone if the operation completed successfully.
		KErrArgument if the specified address is not a valid XIP ROM address.
		KErrGeneral if the specified address has not previously been shadowed
			using Epoc::AllocShadowPage().

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
*/
EXPORT_C TInt Epoc::FreezeShadowPage(TLinAddr aRomAddr)
	{
	// Null operation for flexible memory model...
	return KErrNone;
	}


