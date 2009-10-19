// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\memmodel\epoc\mmubase\ramcache.h
// 
//

#ifndef RAMCACHE_H
#define RAMCACHE_H

#ifndef __MEMMODEL_FLEXIBLE__
#include <memmodel/epoc/mmubase/mmubase.h>
#else
class MmuBase;
#endif

/**
Base class for the ram caching or demand paging implementation.

This provides an interface between class Mmu and the implementation
of any form of dynamic use of the system's free memory, e.g. demand paging.
The chief functionality of this interface is for transferring ownership of
physical pages of RAM.

@internalComponent
*/
class RamCacheBase
	{
public:
	/**
	Constructor
	*/
	RamCacheBase();

	/**
	Intialisation called during MmuBase:Init2.
	*/
	virtual void Init2();

	/**
	Initialisation called from M::DemandPagingInit.
	*/
	virtual TInt Init3()=0;

	/**
	Remove RAM pages from the cache and return them to the system's free pool.
	(Free them.)

	This is called by MmuBase when it requires more free RAM to meet an
	allocation request.

	@param	aNumPages The number of pages to free up.
	@return	True if all pages could be freed, false otherwise
	@pre	RamAlloc mutex held.
	*/
	virtual TBool GetFreePages(TInt aNumPages)=0;

	/**
	Give a RAM page to the cache system for managing.
	This page of RAM may be reused for any purpose.
	If the page has already been donated then no action is taken.

	@param aPageInfo The page info for the donated page.

	@see ReclaimRamCachePage.

	@pre System Lock held
	@post System Lock left unchanged.
	*/
	virtual void DonateRamCachePage(SPageInfo* aPageInfo)=0;

	/**
	Attempt to reclaim a RAM page given to the cache system with #DonateRamCachePage.
	If the RAM page has not been reused for other purposes then the page is
	removed from the cache system's management.
	If the page has not previousely been donated then no action is taken.

	@param aPageInfo The page info for the page to reclaim.

	@return True if page successfuly reclaimed, false otherwise.

	@pre System Lock held
	@post System Lock left unchanged.
	*/
	virtual TBool ReclaimRamCachePage(SPageInfo* aPageInfo)=0;

	/**
	Called by MMU class when a page is unmapped from a chunk.

	@param aPageInfo The page info for the page being unmapped.

	@return True is cache system doesn't claim ownership of this page, false if it does.
	*/
	virtual TBool PageUnmapped(SPageInfo* aPageInfo)=0;

	/**
	Check whether the specified page can be discarded by the RAM cache.

	@param aPageInfo The page info of the page being queried.
	@return ETrue when the page can be discarded, EFalse otherwise.
	@pre System lock held.
	@post System lock held.
	*/
	virtual TBool IsPageDiscardable(SPageInfo& aPageInfo) = 0;

	/**
	Discard the specified page.
	Should only be called on a page if a previous call to IsPageDiscardable()
	returned ETrue and the system lock hasn't been released between the calls.
	If necessary a new page may be allocated to replace the one being
	discarded, however the new page should not be allocated into the RAM
	zone of ID==aBlockedZoneId.
	
	@param aPageInfo The page info of the page to be discarded
	@param aBlockZoneId The ID of the RAM zone that shouldn't be allocated into.
	@param aBlockRest Set to ETrue to stop allocation as soon as aBlockedZoneId is reached 
	in preference ordering.  EFalse otherwise.
	@return ETrue if the page could be discarded, EFalse otherwise.
	
	@pre System lock held.
	@post System lock held.
	*/
	virtual TBool DoDiscardPage(SPageInfo& aPageInfo, TUint aBlockedZoneId, TBool aBlockRest) = 0;


	/**
	First stage in discarding a list of pages.

	Must ensure that the pages will still be discardable even if system lock 
	is released after this method has completed.
	To be used in conjunction with RamCacheBase::DoDiscardPages1().

	@param aPageList A NULL terminated list of the pages to be discarded
	@return KErrNone on success.

	@pre System lock held
	@post System lock held
	*/
	virtual TInt DoDiscardPages0(SPageInfo** aPageList) = 0;


	/**
	Final stage in discarding a list of page
	Finish discarding the pages previously removed by RamCacheBase::DoDiscardPages0().

	@param aPageList A NULL terminated list of the pages to be discarded
	@return KErrNone on success.

	@pre System lock held
	@post System lock held
	*/
	virtual TInt DoDiscardPages1(SPageInfo** aPageList) = 0;


	/**
	Flush (unmap) all the free memory which is currently cached.
	*/
	virtual void FlushAll() = 0;

	/**
	Attempt to allocate the required contiguous region of pages, freeing
	RAM cache pages if required.

	@param	aNumPages The number of pages to free up.
	@param	aAlign The alignment of the region to free-up, (a power-of-2).
	@param aBlockZoneId The ID of a zone that shouldn't be allocated into, when set to 
	KRamZoneInvalidId it will have no effect.
	@param aBlockRest Set to ETrue to stop allocation as soon as aBlockedZoneId is reached 
	in preference ordering.  EFalse otherwise.

	@return	KErrNone on success, KErrNoMemory otherwise
	@pre	RamAlloc mutex held.
	*/
	TInt AllocFreeContiguousPages(TInt aNumPages, TInt aAlign, TZonePageType aType, TPhysAddr& aPhysAddr, TUint aBlockedZoneId, TBool aBlockRest);

	/**
	Return the maximum number of RAM pages which could be obtained with #GetFreePages.
	This value is used in the calculation of the 'free' RAM in the system.

	@return Number of free RAM pages.
	*/
	inline TInt NumberOfFreePages() { return iNumberOfFreePages; }

	/**
	Put a page back on the system's free pool.

	@pre RamAlloc mutex held.
	*/
	void ReturnToSystem(SPageInfo* aPageInfo);

	/**
	Get a RAM page from the system's free pool.
	
	@param aBlockZoneId The ID of a zone that shouldn't be allocated into, when set to 
	KRamZoneInvalidId it will have no effect.
	@param aBlockRest Set to ETrue to stop allocation as soon as aBlockedZoneId is reached 
	in preference ordering.  EFalse otherwise.

 	@pre	RamAlloc mutex held.

	@return The page or NULL if no page is available.
	*/
	SPageInfo* GetPageFromSystem(TUint aBlockedZone=KRamZoneInvalidId, TBool aBlockRest=EFalse);



public:
	MmuBase* iMmu;				/**< Copy of MmuBase::TheMmu */
	TInt iNumberOfFreePages;	/**< Number of pages which could be freed by GetFreePages.
									 This value is protected by the RamAlloc mutex. */
	static RamCacheBase* TheRamCache;
	};


class RamCache : public RamCacheBase
	{
public:
	// from RamCacheBase
	virtual void Init2();
	virtual TInt Init3();
	virtual TBool GetFreePages(TInt aNumPages);
	virtual void DonateRamCachePage(SPageInfo* aPageInfo);
	virtual TBool ReclaimRamCachePage(SPageInfo* aPageInfo);
	virtual TBool PageUnmapped(SPageInfo* aPageInfo);
	virtual TBool IsPageDiscardable(SPageInfo& aPageInfo);
	virtual TBool DoDiscardPage(SPageInfo& aPageInfo, TUint aBlockZoneId, TBool aBlockRest);
	virtual TInt DoDiscardPages0(SPageInfo** aPageList);
	virtual TInt DoDiscardPages1(SPageInfo** aPageList);
	virtual void FlushAll();
	// new
	virtual void SetFree(SPageInfo* aPageInfo);
	enum TFault
		{
		EUnexpectedPageType = 3,	/**< A page in the live page list had an unexpected type (SPageInfo::Attribs) */
		};
	static void Panic(TFault aFault);
	void RemovePage(SPageInfo& aPageInfo);
private:
	SDblQue iPageList;
	};

#endif
