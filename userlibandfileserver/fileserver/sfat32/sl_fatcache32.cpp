// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfat32\sl_fatcache32.cpp
// 
//

#include "sl_std.h"
#include "sl_fatcache32.h"

/**
    @file
    Various FAT32 caches implementation
*/



//#################################################################################################################################
//# CFat32LruCache implementation
//#################################################################################################################################

//-----------------------------------------------------------------------------
CFat32LruCache::CFat32LruCache()
               :CFatPagedCacheBase(), iPageList(_FOFF(CFat32LruCachePage, iLink))
    {
    }

//-----------------------------------------------------------------------------
/**
    FAT32 LRU cache factory function.
    @param  aOwner              pointer to the owning FAT mount
    @param  aMaxMemSize         maximal size of the memory the cache can use, bytes
    @param  aRdGranularityLog2  Log2(read granularity)
    @param  aWrGranularityLog2  Log2(write granularity)

    @return pointer to the constructed object.
*/
CFat32LruCache* CFat32LruCache::NewL(CFatMountCB* aOwner, TUint32 aMaxMemSize, TUint32 aRdGranularityLog2, TUint32 aWrGranularityLog2)
    {
    __PRINT(_L("#-CFat32LruCache::NewL()"));
    CFat32LruCache* pSelf = NULL;
    pSelf = new (ELeave) CFat32LruCache;

    CleanupStack::PushL(pSelf);
    pSelf->InitialiseL(aOwner, aMaxMemSize, aRdGranularityLog2, aWrGranularityLog2);
    CleanupStack::Pop();
    
    return pSelf;
    }

//-----------------------------------------------------------------------------
/** 
    @return pointer to the CFatBitCache interface. 
*/
CFatBitCache* CFat32LruCache::BitCacheInterface() 
    {
    return iBitCache;
    }  

//-----------------------------------------------------------------------------

/**
    FAT32 LRU cache initialisation.

    @param  aOwner              pointer to the owning FAT mount
    @param  aMaxMemSize         maximal size of the memory the cache can use, bytes
    @param  aRdGranularityLog2  Log2(read granularity)
    @param  aWrGranularityLog2  Log2(write granularity)

    @return pointer to the constructed object.
*/
void  CFat32LruCache::InitialiseL(CFatMountCB* aOwner, TUint32 aMaxMemSize, TUint32 aRdGranularityLog2, TUint32 aWrGranularityLog2)
    {
    const TUint32 KReadGranularity = Pow2(aRdGranularityLog2);
    const TUint32 KWriteGranularity = Pow2(aWrGranularityLog2);

    __PRINT3(_L("#-CFat32LruCache::InitialiseL MaxMem:%u, RdGr:%d, WrGr:%d"),aMaxMemSize, KReadGranularity, KWriteGranularity);
    (void)KReadGranularity;
    (void)KWriteGranularity;


    const TBool bParamsValid = (aRdGranularityLog2 >= aWrGranularityLog2) && (aWrGranularityLog2 >= KDefSectorSzLog2) && (aMaxMemSize > KReadGranularity);
    __ASSERT_ALWAYS(bParamsValid, Fault(EFatCache_BadGranularity));  
   
    CFatPagedCacheBase::InitialiseL(aOwner);
    
    ASSERT(FatType() == EFat32);
    
    //-- according to the FAT32 specs, FAT32 min size is 65526 entries or 262104 bytes.
    //-- It's possible to incorrectly format a small volume to FAT32, it shall be accessible read-only.
    if(aMaxMemSize > FatSize()) 
        {//-- strange situation, memory allocated for LRU cache is enough to cache whole FAT32
        __PRINT(_L("#-CFat32LruCache::InitialiseL warning: LRU cache becomes fixed! (too much memory allowed)"));
        aMaxMemSize = FatSize();
        }

    //-- LRU cache page size is (2^aRdGranularityLog2) bytes and consists of 2^(aRdGranularityLog2-aWrGranularity) sectors.
    iPageSizeLog2 = aRdGranularityLog2;
    iSectorSizeLog2 = aWrGranularityLog2; //-- Log2(number of sectors in cache page)

    iMaxPages = aMaxMemSize / PageSize(); //-- maximal number of cache pages we can allocate
    iNumPagesAllocated = 0;
    
    __ASSERT_ALWAYS((iMaxPages > 1 && SectorsInPage() < KMaxSectorsInPage), Fault(EFatCache_BadGranularity));

    //-- obtain maximal number of entries in the table
    if(aOwner->UsableClusters() < 1)
        {
        ASSERT(0);
        User::Leave(KErrCorrupt);
        }

    //-- create FAT bit supercache if it is enabled in config
    ASSERT(!iBitCache);
    if(aOwner->FatConfig().FAT32_UseBitSupercache())
        {
        iBitCache = CFatBitCache::New(*this);
        }
    else
        {
        __PRINT(_L("#++ !! Fat Bit Supercache is disabled in config !!"));
        }

    }

//-----------------------------------------------------------------------------
/**
    Close the cache and deallocate its memory.
    @param  aDiscardDirtyData if ETrue, will ignore dirty data. If EFalse, will panic on atempt to close dirty cache.  
*/
void CFat32LruCache::Close(TBool aDiscardDirtyData)
    {
    __PRINT1(_L("#-CFat32LruCache::Close(%d)"), aDiscardDirtyData);
    
    //-- delete FAT bit supercache if present
    delete iBitCache;
    iBitCache=NULL;


    //-- delete existing cache pages
    TPageIterator itr(iPageList);

    for(;;)
        {
        CFat32LruCachePage* pPage = itr++;
        if(!pPage) 
            break;

        pPage->iLink.Deque(); //-- remove page from the list
        
        if(pPage->IsDirty())
            {//-- trying to destroy the cache that has dirty pages
            __PRINT1(_L("#-CFat32LruCache::Close() The page is dirty! Start idx:%d"), pPage->StartFatIndex());
            if(!aDiscardDirtyData)
                {
                Fault(EFatCache_DiscardingDirtyData);  
                }
            //-- ignore this fact if requested.
            }

        delete pPage;
        --iNumPagesAllocated;
        }

    SetDirty(EFalse);
    ASSERT(!iNumPagesAllocated);
    }


//-----------------------------------------------------------------------------
/**
    Tries to read FAT entry from the cache. If the entry at aFatIndex is not cached, does nothing and returns EFalse.
    If finds the cache page that contains entry at index "aFatIndex", reads it and returns ETrue.

    @param  aFatIndex  FAT entry index within FAT table
    @param  aFatEntry  on success it will contain FAT entry value
    @return ETrue if the entry has been read
            EFalse if index aFatIndex isn't cached
*/
TBool CFat32LruCache::ReadCachedEntryL(TUint32 aFatIndex, TFat32Entry& aResult)
    {
    //-- iterate through LRU list looking if the entry is cached.
    TPageIterator itr(iPageList);

    for(;;)
        {
        CFat32LruCachePage* pPage = itr++;
        if(!pPage) 
            break;

        if(pPage->ReadCachedEntryL(aFatIndex, aResult))
            {//-- found entry in some cache page. Make this page LRU
            if(!iPageList.IsFirst(pPage))
                {
                pPage->iLink.Deque();
                iPageList.AddFirst(*pPage);
                }
            return ETrue; 
            }
        }

    return EFalse; //-- the entry is not cached
    }

//-----------------------------------------------------------------------------
/**
    Tries to write FAT entry to the cache. If the entry at aFatIndex is not cached, does nothing and returns EFalse.
    If finds the cache page that contains entry at index "aFatIndex", overwrites it and returns ETrue

    @param  aFatIndex  FAT entry index within FAT table
    @param  aFatEntry  new FAT entry value
    @return ETrue if the entry has been overwritten
            EFalse if index aFatIndex isn't cached
*/
TBool CFat32LruCache::WriteCachedEntryL(TUint32 aFatIndex, TFat32Entry aFatEntry)
    {
    //-- iterate through LRU list looking if the entry is cached.
    TPageIterator itr(iPageList);

    for(;;)
        {
        CFat32LruCachePage* pPage = itr++;
        if(!pPage) 
            break;

        if(pPage->WriteCachedEntryL(aFatIndex, aFatEntry))
            {//-- the entry was cached and modified now. Make this page LRU
            if(!iPageList.IsFirst(pPage))
                {
                pPage->iLink.Deque();
                iPageList.AddFirst(*pPage);
                }
            return ETrue; 
            }
        }
    
    return EFalse; //-- the entry is not cached
    }

//-----------------------------------------------------------------------------
/**
    Get a spare page. This function can either allocate a page if memory limit isn't reached yet
    or find the least recently used (in the end of the LRU list) and evict it.
    
    @return pointer to the cache page to use, it will be insertet to the beginning of the LRU list
*/
CFat32LruCachePage* CFat32LruCache::DoGetSpareCachePageL()
    {
    CFat32LruCachePage* pPage=NULL;

    if(iNumPagesAllocated < iMaxPages)
        {//-- we still can allocate a page
    
        pPage = CFat32LruCachePage::NewL(*this);
        ++iNumPagesAllocated;
        iPageList.AddFirst(*pPage); //-- insert the page into the beginning of LRU list
        return pPage;
        }

    //-- all pages are already allocated, evict the last recently used and remove it from the list
    pPage = iPageList.Last();   //-- least recently used page, last in the list
    pPage->iLink.Deque();       //-- remove it from the LRU list
    iPageList.AddFirst(*pPage); //-- insert the page into the beginning of LRU list

    //__PRINT1(_L("#-CFat32LruCache::DoGetSpareCachePageL() page @FAT idx:%d evicted"), pPage->StartFatIndex());
    
    //-- flush the page, writing its data to all copies of FAT, to FAT1, then to FAT2 etc.
    ASSERT(NumFATs() >0);
    if(pPage->IsDirty())
        {
        //-- write page data to all copies of FAT
        for(iCurrentFatNo=0; iCurrentFatNo < NumFATs(); ++iCurrentFatNo)
            {
            const TBool keepDirty = iCurrentFatNo < (NumFATs()-1);
            pPage->FlushL(keepDirty);
            }
    
        iCurrentFatNo = KInvalidFatNo;
        }

    
    return pPage;
    }



//-----------------------------------------------------------------------------
/**
    Read FAT entry from the cache. 

    @param  aIndex FAT entry index to read
    @return FAT entry value at the index "aIndex"
*/
TUint32 CFat32LruCache::ReadEntryL(TUint32 aIndex)
    {
//    __PRINT1(_L("#-CFat32LruCache::ReadEntryL() FAT idx:%d"), aIndex);

    ASSERT(FatIndexValid(aIndex));

    //-- firstly try to locate required entry in cache
    TFat32Entry entry;
    if(ReadCachedEntryL(aIndex, entry))
        return entry; //-- the requested entry found in cache

    //-- No luck, get a spare cache page (it will be inserted to the head of the LRU list)
    CFat32LruCachePage* pPage = DoGetSpareCachePageL();
    ASSERT(pPage); 

    entry = pPage->ReadFromMediaL(aIndex); //-- read whole FAT page from the media

    return entry;
    }

//-----------------------------------------------------------------------------
/**
    Write FAT entry to the cache. 
    Appropriate FAT cache sector will be marked as "dirty" and will be eventually flushed to the media.

    @param  aIndex FAT entry index
    @param  aEntry FAT entry value
*/
void CFat32LruCache::WriteEntryL(TUint32 aIndex, TUint32 aEntry)
    {
    //__PRINT2(_L("#-CFat32LruCache::WriteEntryL() FAT idx:%d, val:%d"), aIndex, aEntry);

    ASSERT(FatIndexValid(aIndex));

    SetDirty(ETrue);

    //-- 1. try to locate entry in the cache and overwrite it there if it is cached
    if(WriteCachedEntryL(aIndex, aEntry))
        return; //-- the entry in cache altered
    
    //-- 2. the entry isn't cached; find a spare cache page (it will be inserted to the head of the LRU list)
    CFat32LruCachePage* pPage = DoGetSpareCachePageL();
    ASSERT(pPage); 
    
    pPage->ReadFromMediaL(aIndex); //-- read whole FAT page from the media


    //-- 3. overwrite entry in cache
    TBool bRes = pPage->WriteCachedEntryL(aIndex, aEntry);
    ASSERT(bRes);
    (void)bRes;
    }

//-----------------------------------------------------------------------------
/**
    A debug method that asserts that the cache is really clean
*/
void CFat32LruCache::AssertCacheReallyClean() 
    {
#ifdef _DEBUG 
    
    TPageIterator itr(iPageList);
    for(;;)
        {//-- iterate through LRU list flushing pages into the current copy of FAT
        CFat32LruCachePage* pPage = itr++;
        
        if(!pPage) 
            break;

        if(pPage->IsDirty())
            {
            __PRINT(_L("#-CFat32LruCache::AssertCacheReallyClean()"));
            ASSERT(0);
            }
        }

#endif   
    }

//-----------------------------------------------------------------------------
/**
    Flushes all dirty data to the media.
*/
void CFat32LruCache::FlushL()
    {
    if(!IsDirty())
        {
        AssertCacheReallyClean();
        return;
        }

    //-- flush dirty data to all copies of FAT
    //-- all dirty pages will be written firstly to FAT1, then all of them will be written to FAT2 etc.
    for(iCurrentFatNo=0; iCurrentFatNo < NumFATs(); ++iCurrentFatNo)
        {
        TPageIterator itr(iPageList);
        for(;;)
            {//-- iterate through LRU list flushing pages into the current copy of FAT
            CFat32LruCachePage* pPage = itr++;
            if(!pPage) 
                break;

            //-- we need to keep page dirty until it is flushed to the last copy of FAT table
            const TBool keepDirty = iCurrentFatNo < (NumFATs() - 1);
            pPage->FlushL(keepDirty);
            }
        }
   
    iCurrentFatNo = KInvalidFatNo;
   
    SetDirty(EFalse);
    }

//-----------------------------------------------------------------------------

/**
    Invalidate whole cache. All pages will be marked as invalid and will be re-read from the media on first access to them.
    @return always KErrNone
*/
TInt CFat32LruCache::Invalidate()
    {
    __PRINT(_L("#-CFat32LruCache::Invalidate()"));
    const TBool bIgnoreDirtyData = CheckInvalidatingDirtyCache();

    //-- iterate through LRU list marking every page as invalid
    TPageIterator itr(iPageList);
    for(;;)
        {
        CFat32LruCachePage* pPage = itr++;
        if(!pPage) 
            break;

        pPage->Invalidate(bIgnoreDirtyData);
        }

    SetDirty(EFalse);

    return KErrNone;
    }


//-----------------------------------------------------------------------------

/**
    Invalidate FAT cache pages that contain FAT32 entries from aStartIndex to (aStartIndex+aNumEntries)
    These pages will be marked as invalid and will be re-read from the media on first access to them.
    
    @param  aStartIndex FAT start index of the region being invalidated
    @param  aNumEntries number of entries to invalidate
    @return always KErrNone
*/
TInt CFat32LruCache::InvalidateRegion(TUint32 aStartIndex, TUint32 aNumEntries)
    {
    __PRINT2(_L("#-CFat32LruCache::InvalidateRegion() startIndex:%d, entries:%d"),aStartIndex, aNumEntries);
    ASSERT(FatIndexValid(aStartIndex));
    ASSERT(FatIndexValid(aStartIndex+aNumEntries-1));


    if(!aNumEntries)
        {
        ASSERT(0);
        return KErrNone;
        }

    const TBool bIgnoreDirtyData = CheckInvalidatingDirtyCache();
    const TUint KEntriesInPage = Pow2(PageSizeLog2() - KFat32EntrySzLog2);
    const TUint KLastIndex = aStartIndex+aNumEntries;

    TBool bCacheIsStillDirty = EFalse; //-- ETrue if the cache is still dirty after invalidating its region
    
    for(TUint currIndex = aStartIndex; currIndex < KLastIndex; currIndex+=KEntriesInPage)
        {
        TPageIterator itr(iPageList);
        for(;;)
            {//-- iterate through all pages, invalidating required
            CFat32LruCachePage* pPage = itr++;
            if(!pPage) 
                break;

            if(pPage->IsEntryCached(currIndex))
                {
                pPage->Invalidate(bIgnoreDirtyData); 
                }
            else if(pPage->IsDirty()) //-- invalid page can't be ditry.
                {
                bCacheIsStillDirty = ETrue; //-- we have at least 1 dirty page
                }
            }
        }

    SetDirty(bCacheIsStillDirty);
    
    return KErrNone;
}

//-----------------------------------------------------------------------------



/**
    Look for free FAT entry in the FAT sector that corresponds to the aFatEntryIndex.
    Search is performed in both directions, the right one has more priority (FAT cluster chain needs to grow right).
    See FindFreeEntryInCacheSector()
*/
TBool CFat32LruCache::FindFreeEntryInCacheSectorL(TUint32& aFatEntryIndex)
    {
    if(ReadEntryL(aFatEntryIndex) == KSpareCluster)
        return ETrue;
    
    //-- look for free FAT entries in the FAT cache sector corresponting to the aStartIndex.
    //-- use the same approach as in CFatTable::FindClosestFreeClusterL()
    const TUint32 coeff = SectorSizeLog2()-KFat32EntrySzLog2;
    const TUint32 numEntriesInSector = Pow2(coeff); //-- number of FAT32 entries in FAT cache sector

    TUint32 MinIdx = (aFatEntryIndex >> coeff) << coeff;
    TUint32 MaxIdx = MinIdx+numEntriesInSector-1;

    if(MinIdx == 0)
        {//-- correct values if this is the first FAT sector; FAT[0] & FAT[1] are reserved
        MinIdx += KFatFirstSearchCluster;
        }

    //-- actual number of usable FAT entries can be less than deducted from number of FAT sectors.
    MaxIdx = Min(MaxIdx, MaxFatEntries()-1);

    //-- look in both directions starting from the aFatEntryIndex
    //-- but in one FAT cache page sector only
    TBool canGoRight = ETrue;
    TBool canGoLeft = ETrue;
    
    TUint32 rightIdx=aFatEntryIndex;
    TUint32 leftIdx=aFatEntryIndex;

    for(TUint i=0; i<numEntriesInSector; ++i)
        {
        if(canGoRight)
            {
            if(rightIdx < MaxIdx)
                ++rightIdx;
            else
                canGoRight = EFalse;
            }

        if(canGoLeft)
            {
            if(leftIdx > MinIdx)
                --leftIdx;
            else        
                canGoLeft = EFalse;
            }

        if(!canGoRight && !canGoLeft)
            return EFalse;  //-- no free entries in this sector

        if(canGoRight && ReadEntryL(rightIdx) == KSpareCluster)
            {
            aFatEntryIndex = rightIdx;
            return ETrue;
            }

        if (canGoLeft && ReadEntryL(leftIdx) == KSpareCluster)
            {
            aFatEntryIndex = leftIdx;
            return ETrue;
            }
        }//for(TUint i=0; i<numEntriesInSector; ++i)

    return EFalse;
    }



//#################################################################################################################################
//  CFat32LruCachePage implementation
//#################################################################################################################################


CFat32LruCachePage::CFat32LruCachePage(CFatPagedCacheBase& aCache)
                   :CFatCachePageBase(aCache)
    {

    ASSERT(IsPowerOf2(EntriesInPage()));
    }


/**
    Factory function.
    @param aCache reference to the owning cache.
    @return pointer to the constructed object or NULL on error
*/
CFat32LruCachePage* CFat32LruCachePage::NewL(CFatPagedCacheBase& aCache)
    {

    CFat32LruCachePage* pSelf = NULL;
    pSelf = new (ELeave) CFat32LruCachePage(aCache);

    CleanupStack::PushL(pSelf);
    
    pSelf->iData.CreateMaxL(pSelf->PageSize()); //-- allocate memory for the page
   
    CleanupStack::Pop();

    return pSelf;
    }


//-----------------------------------------------------------------------------

/**
    Get a pointer to the FAT32 entry in the page buffer.
    The page 's data shall be valid and the entry shall belong to this page.
    
    @param aFatIndex absolute FAT index (from the FAT start) of the entry
    @return pointer to the FAT32 entry in the page buffer.
*/
TFat32Entry* CFat32LruCachePage::GetEntryPtr(TUint32 aFatIndex) const
    {

    ASSERT(IsValid() && IsEntryCached(aFatIndex));
    
    const TUint KEntryIndexInPage = aFatIndex & (EntriesInPage()-1); //-- number of entries in page is always a power of 2

    TFat32Entry* pEntry = ((TFat32Entry*)iData.Ptr()) + KEntryIndexInPage;
    return  pEntry;
    }

//-----------------------------------------------------------------------------

/**
    Read FAT32 entry from the cache.
    
    1. If the entry at aFatIndex doesn't belong to this page, returns EFalse
    2. If page's data are valid and the entry is cached just extracts data from the page buffer.
    3. If page's data are invalid but the entry's index belongs to this page, firstly reads data from the media and goto 2
    
    @param  aFatIndex entry's absolute FAT index (from the FAT start)
    @param  aResult on sucess there will be FAT32 entry value
    @return ETrue if the entry at aFatIndex belongs to this page (cached) and in this case aResult will contain this entry.
            EFalse if the entry isn't cached.
    
*/
TBool CFat32LruCachePage::ReadCachedEntryL(TUint32 aFatIndex, TUint32& aResult) 
    {
    if(!IsEntryCached(aFatIndex))
        return EFalse;  //-- the page doesn't contain required index
    
    if(IsValid())
        {//-- read entry directly from page buffer, the cached data are valid
        aResult = (*GetEntryPtr(aFatIndex)) & KFat32EntryMask;
        }
    else
        {//-- aFatIndex belongs to this page, but the page is invalid and needs to be read from the media
        __PRINT1(_L("#-CFat32LruCachePage::ReadCachedEntry(%d) The page is invalid, reading from the media"), aFatIndex);
        aResult = ReadFromMediaL(aFatIndex);
        }

    return ETrue;
    }

//-----------------------------------------------------------------------------

/**
    Read the FAT32 cache page from the media and return required FAT32 entry.    

    @param  aFatIndex entry's absolute FAT index (from the FAT start)
    @return entry value at aFatIndex.
*/
TUint32 CFat32LruCachePage::ReadFromMediaL(TUint32 aFatIndex)
    {
    //__PRINT1(_L("#-CFat32LruCachePage::ReadFromMediaL() FAT idx:%d"), aFatIndex);

    const TUint KFat32EntriesInPageLog2 = iCache.PageSizeLog2()-KFat32EntrySzLog2; //-- number of FAT32 entries in page is always a power of 2

    //-- find out index in FAT this page starts from
    iStartIndexInFAT = (aFatIndex >> KFat32EntriesInPageLog2) << KFat32EntriesInPageLog2;

    SetState(EInvalid); //-- mark the page as invalid just in case if the read fails.
    
    //-- read page from the media
    const TUint32 pageStartPos = iCache.FatStartPos() + (iStartIndexInFAT << KFat32EntrySzLog2);
    TInt nRes = iCache.ReadFatData(pageStartPos, iCache.PageSize(), iData);
    if(nRes != KErrNone)
        {
        __PRINT1(_L("#-CFat32LruCachePage::ReadFromMediaL() failed! code:%d"), nRes);
        User::Leave(nRes);
        }

    SetClean(); //-- mark this page as clean

    const TFat32Entry entry = (*GetEntryPtr(aFatIndex)) & KFat32EntryMask;

    return entry;
    }

//-----------------------------------------------------------------------------

/**
    Writes FAT cache page sector to the media (to all copies of the FAT)

    @param  aSector page sector number
*/
void CFat32LruCachePage::DoWriteSectorL(TUint32 aSector)
    {
    //__PRINT1(_L("#-CFat32LruCachePage::DoWriteContiguousSectorsL() startSec:%d"),aSector);

    ASSERT(aSector < iCache.SectorsInPage());

    const TUint CacheSecSzLog2=iCache.SectorSizeLog2();

    TInt offset = 0;
    if(iStartIndexInFAT == 0 && aSector == 0)
        {//-- this is the very beginning of FAT32. We must skip FAT[0] & FAT[1] entries and do not write them to media.    
        offset = KFatFirstSearchCluster << KFat32EntrySzLog2; 
        }    
    
    const TUint8* pData = iData.Ptr()+offset+(aSector << CacheSecSzLog2);
    
    TUint32 dataLen = (1 << CacheSecSzLog2) - offset;

    const TUint32 mediaPosStart = iCache.FatStartPos() + (iStartIndexInFAT << KFat32EntrySzLog2) + (aSector << CacheSecSzLog2) + offset; 
    const TUint32 mediaPosEnd = mediaPosStart + dataLen; 

    //-- check if we are going to write beyond FAT. It can happen if the write granularity is bigger that the sector size.
    const TUint32 posFatEnd = iCache.FatStartPos() + iCache.FatSize();
    if(mediaPosEnd > posFatEnd)
        {//-- correct the leength of the data to write.
        dataLen -= (mediaPosEnd-posFatEnd);
        }

    TPtrC8 ptrData(pData, dataLen); //-- source data descriptor 

    TInt nRes = iCache.WriteFatData(mediaPosStart, ptrData);
    
    if(nRes != KErrNone)
        {
        __PRINT1(_L("#-CFat32LruCachePage::DoWriteSectorsL() failed! code:%d"), nRes);
        User::Leave(nRes);
        }

    
    //-- if we have FAT bit supercache and it is in consistent state, check if the entry in this cache differs from the data in dirty FAT cache sector. 
    CFatBitCache *pFatBitCache = iCache.BitCacheInterface();
    if(pFatBitCache && pFatBitCache->UsableState())
        {
        //-- absolute FAT cache sector number corresponding aSector number in _this_ cache page
        const TUint32 absSectorNum = (iStartIndexInFAT >> (CacheSecSzLog2-KFat32EntrySzLog2)) + aSector; 

        if(pFatBitCache->FatSectorHasFreeEntry(absSectorNum))
            {   //-- it means that the corresponding FAT cache sector may or may not contain free FAT entry.
            //-- in this case we need to repopulate corresponding bit cache entry. 

            const TUint32 numEntries = dataLen >> KFat32EntrySzLog2; //-- amount of FAT entries in this sector
            const TFat32Entry*  pFat32Entry = (const TFat32Entry* )pData;
    
            TBool bHasFreeFatEntry = EFalse;

            for(TUint i=0; i<numEntries; ++i)
                {//-- look for free entries in this particular FAT cache sector.
                if(pFat32Entry[i] == KSpareCluster)
                    {
                    bHasFreeFatEntry = ETrue;
                    break;
                    }
                }
        
            if(!bHasFreeFatEntry)
                {   //-- FAT bit cache indicates that FAT sector absSectorNum has free entries, but it doesn't.
                //-- this is because we can only set "has free entry" flag in CAtaFatTable::WriteL().
                //-- correct FAT bit cache entry 
                pFatBitCache->SetFreeEntryInFatSector(absSectorNum, EFalse);

                //__PRINT2(_L("#++ :DoWriteSectorL() Fixed FAT bit cache BitVec[%d]=%d"), absSectorNum, pFatBitCache->FatSectorHasFreeEntry(absSectorNum));
                }

            }
        else //if(pBitCache->FatSectorHasFreeEntry(absSectorNum))
            {//-- don't need to do anything. The corresponding FAT cache sector never contained free FAT entry and
             //-- free FAT entry has never been written there in CAtaFatTable::WriteL().
            }

        }//if(pFatBitCache && pFatBitCache->UsableState())
 

    }


//-----------------------------------------------------------------------------
/**
    Write FAT32 entry at aFatIndex to the cache. Note that the data are not written to the media, only to the cache page.
    Corresponding page sector is marked as dirty and will be flushed on FlushL() call later.

    1. If the entry at aFatIndex doesn't belong to this page, returns EFalse
    2. If page's data are valid and the entry is cached, copies data to the page buffer and marks sector as dirty.
    3. If page's data are invalid but the entry's index belongs to this page, firstly reads data from the media and goto 2

    @param  aFatIndex entry's absolute FAT index (from the FAT start)
    @param  aFatEntry FAT32 entry value
    @return ETrue if the entry at aFatIndex belongs to this page (cached) and in this case aResult will contain this entry.
            EFalse if the entry isn't cached.

*/
TBool CFat32LruCachePage::WriteCachedEntryL(TUint32 aFatIndex, TUint32 aFatEntry)
    {

    if(!IsEntryCached(aFatIndex)) 
        return EFalse;  //-- the page doesn't contain required index
    
    if(!IsValid())
        {//-- we are trying to write data to the page that has invalid data. //-- read the data from the media first.
        ReadFromMediaL(aFatIndex);
        }
    
    //-- for FAT32 only low 28 bits are used, 4 high are reserved; preserve them
    TFat32Entry* pEntry = GetEntryPtr(aFatIndex);
    const TFat32Entry orgEntry = *pEntry;
    *pEntry = (orgEntry & ~KFat32EntryMask) | (aFatEntry & KFat32EntryMask);

    //-- mark corresponding sector of the cache page as dirty
    const TUint entryIndexInPage = aFatIndex & (EntriesInPage()-1); //-- number of entries in page is always a power of 2
    const TUint dirtySectorNum   = entryIndexInPage >> (iCache.SectorSizeLog2() - KFat32EntrySzLog2);

    ASSERT(dirtySectorNum < iCache.SectorsInPage());

    iDirtySectors.SetBit(dirtySectorNum);
    SetState(EDirty); //-- mark page as dirty.

    return ETrue;
    }



//#################################################################################################################################
//  CFatBitCache implementation
//#################################################################################################################################

//-- define this macro for extra debugging facilities for the CFatBitCache
//-- probably needs to be removed completely as soon as everything settles
//#define FAT_BIT_CACHE_DEBUG

//----------------------------------------------------------------------------- 

CFatBitCache::CFatBitCache(CFat32LruCache& aOnwerFatCache)
             :iOwnerFatCache(aOnwerFatCache)
    {
    SetState(EInvalid);
    DBG_STATEMENT(iPopulatingThreadId=0);
    }

CFatBitCache::~CFatBitCache()
    {
    Close();
    }

//----------------------------------------------------------------------------- 
/**
    FAT bit supercache factory method
    @return pointer to the created object or NULL if it coud not create or initialise it.
*/
CFatBitCache* CFatBitCache::New(CFat32LruCache& aOnwerFatCache)
    {
    __PRINT(_L("#++ CFatBitCache::New()"));
    
    CFatBitCache* pSelf = NULL;
    pSelf = new CFatBitCache(aOnwerFatCache);

    if(!pSelf)
        return NULL; //-- failed to create object

    TInt nRes = pSelf->Initialise();
    if(nRes != KErrNone)
        {//-- failed to initialise the object
        delete pSelf;
        pSelf = NULL;
        }

    return pSelf;
    }


//-----------------------------------------------------------------------------

/** 
    Initialisation.
    Note that this cache suports FAT32 only.
    @return KErrNone on success; otherwise standard error code.
*/
TInt CFatBitCache::Initialise()
    {
    __PRINT(_L("#++ CFatBitCache::Initialise()"));
    
    Close();
    
    //-- only FAT32 supported
    if(iOwnerFatCache.FatType() != EFat32)
        {
        ASSERT(0);
        Fault(EFatCache_BadFatType);
        }

    //-- create the bit vector. each bit position there represents one FAT cache sector (in FAT cache page terms, see FAT page structure)
    const TUint fatSize = iOwnerFatCache.FatSize(); //-- FAT size in bytes
    const TUint fatCacheSecSize = Pow2(iOwnerFatCache.SectorSizeLog2()); //-- FAT cache sector size
    const TUint maxFatUsableCacheSectors = (fatSize + (fatCacheSecSize-1)) >> iOwnerFatCache.SectorSizeLog2(); //-- maximal number of usable fat cache sectors in whole FAT table

    //-- create a bit vector
    __PRINT1(_L("#++ CFatBitCache::Initialise() FAT supercache bits:%u"), maxFatUsableCacheSectors);
    
    TInt nRes = iBitCache.Create(maxFatUsableCacheSectors);
    if(nRes != KErrNone)
        {
        __PRINT1(_L("#++ Failed to create a bit vector! code:%d"), nRes);
        return nRes;
        }
    
    //-- calculate the coefficient to be used to convet FAT index to FAT cache sector number (bit vector index).
    iFatIdxToSecCoeff = iOwnerFatCache.SectorSizeLog2()-KFat32EntrySzLog2;
    SetState(ENotPopulated);

    return KErrNone;    
    }

//-----------------------------------------------------------------------------
/**
    Closes the cache and deallocates bit vector memory.
*/
void CFatBitCache::Close()
    {
    __PRINT(_L("#++ CFatBitCache::Close()"));
    
    //-- this method must not be called during populating (optionally by another thread)
    ASSERT(State() != EPopulating);
    ASSERT(iPopulatingThreadId == 0);

    iBitCache.Close();
    SetState(EInvalid);
    }

//-----------------------------------------------------------------------------

/**
    Tell the cache that we are starting to populate it. 
    N.B. Start, Finish and populating methods shall be called from the same thread. 
    Only one thread can be populating the bit vector; 

    @return ETrue on success. Efalse means that the cache is in the invalid state for some reason.
*/
TBool CFatBitCache::StartPopulating()
    {
    __PRINT2(_L("#++ CFatBitCache::StartPopulating(), State:%d, ThreadId:%d"), State(), (TUint)RThread().Id());

    if(State() != ENotPopulated)
        {//-- wrong state
        ASSERT(0);
        return EFalse; 
        }
    
    ASSERT(iPopulatingThreadId == 0);

    iBitCache.Fill(0);
    SetState(EPopulating);
    
    //-- store the the ID of the thread that starts populating the cache; it'll be checked later during populating.
    DBG_STATEMENT(iPopulatingThreadId = RThread().Id());

    return ETrue;
    }

//-----------------------------------------------------------------------------

/**
    Tell the cache that we have finished to populate it. 

    @return ETrue on success. EFalse means that the cache is in the invalid state for some reason.
*/
TBool CFatBitCache::FinishPopulating(TBool aSuccess)
    {
    __PRINT2(_L("#++ CFatBitCache::PopulatingFinished(), ThreadId:%d, success:%d"), (TUint)RThread().Id(), aSuccess);

    if(State() != EPopulating)
        {//-- wrong state
        ASSERT(0);
        return EFalse; 
        }

    ASSERT(iPopulatingThreadId == RThread().Id()); //-- check that this method is called from the same thread that started populating
    DBG_STATEMENT(iPopulatingThreadId = 0); 
    
    if(aSuccess)
        SetState(EPopulated); //-- the cache is usable; populated OK
    else
        SetState(EInvalid);   //-- the cache isn't populated properly, make it not usable

    return ETrue;
    }

//-----------------------------------------------------------------------------
/**
    Tell FAT bit cache that there is a free entry at FAT aFatIndex.
    Only this method can be used to populate the bit array (in EPopulating state). 
    Other methods can't access bit array in EPopulating state i.e. it is safe to populate the cache
    from the thread other than FS drive thread (e.g within background FAT scan)

    @param  aFatIndex free FAT32 entry index
    @return ETrue on success. EFalse means that the cache is in the invalid state for some reason.
*/
TBool CFatBitCache::SetFreeFatEntry(TUint32 aFatIndex)
    {
    //__PRINT3(_L("#++  ReportFreeFatEntry: idx:%d, state:%s, tid:%d"), aFatIndex, State(), (TUint)RThread().Id());

    if(State() != EPopulating && State() != EPopulated)
        {//-- wrong state, this can happen if someone forcedly invalidated this cache during populating
        return EFalse; 
        }

#if defined _DEBUG && defined FAT_BIT_CACHE_DEBUG 
    //-- This leads to serious performance degradation, so be careful with it.
    if(State() == EPopulating)
        {//-- check that this method is called from the same thread that started populating
        if(iPopulatingThreadId != RThread().Id())
            {
            __PRINT3(_L("#++ !! ReportFreeFatEntry: Access from different thread!! idx:%d, state:%d, tid:%d"), aFatIndex, State(), (TUint)RThread().Id());
            }
        //ASSERT(iPopulatingThreadId == RThread().Id()); 
        }
#endif
    
    //-- set bit to '1' which indicates that the FAT cache sector corresponding to the aFatIndex has at least one free FAT entry
    const TUint32 bitNumber = FatIndexToCacheSectorNumber(aFatIndex); //-- index in the bit array corresponding FAT cache sector

#if defined _DEBUG && defined FAT_BIT_CACHE_DEBUG 
    //-- This leads to serious performance degradation, so be careful with it.
    TBool b = iBitCache[bitNumber];
    if(!b && State()==EPopulated)
        {//-- someone is reporting a free entry in the given cache sector.
        __PRINT1(_L("#++ CFatBitCache::ReportFreeFatEntry BitVec[%d]=1"), bitNumber);
        }
#endif


    iBitCache.SetBit(bitNumber);
    
    return ETrue;
    }

//-----------------------------------------------------------------------------
/**
    Forcedly mark a part of the FAT bit super cache as containing free clusters (or not).
 
    @param  aStartFatIndex  start FAT index of the range
    @param  aEndFatIndex    end FAT index of the range
    @param  aAsFree         if ETrue, the range will be marked as containing free clusters
*/
void CFatBitCache::MarkFatRange(TUint32 aStartFatIndex, TUint32 aEndFatIndex, TBool aAsFree)
    {
    __PRINT3(_L("#++ CFatBitCache::MarkFatRange(%d, %d, %d)"), aStartFatIndex, aEndFatIndex, aAsFree);

    ASSERT(State() == EPopulating || State() == EPopulated);

    const TUint32 bitNumberStart = FatIndexToCacheSectorNumber(aStartFatIndex);
    const TUint32 bitNumberEnd   = FatIndexToCacheSectorNumber(aEndFatIndex);

    iBitCache.Fill(bitNumberStart, bitNumberEnd, aAsFree);
    }


//-----------------------------------------------------------------------------
/**
    Try to locate closest to the aFatIndex free FAT entry in the FAT32 LRU cache.
    This is done by several steps:

    1. Try to find FAT cache sector containing free FAT entry (by using FAT sectors bitmap)
    2. locate free FAT entry within this sector.
    
    @param      aFatIndex  in: absolute FAT entry index that will be used to start search from (we need to find the closest free entry to it)
                           out: may contain FAT index of the located free entry.
                            
    @return     one of the completion codes: 
                KErrNone      free entry found and its index is in aFatIndex  
                KErrNotFound  FAT sector closest to the aFatIndex entry doesn't contain free FAT entries; the conflict is resolved, need to call this method again
                KErrEof       couldn't find any free sectors in FAT; need to fall back to the old search method
                KErrCorrupt   if the state of the cache is inconsistent
*/
TInt CFatBitCache::FindClosestFreeFatEntry(TUint32& aFatIndex)
    {
    const TUint32 startFatCacheSec = FatIndexToCacheSectorNumber(aFatIndex);
    
    //__PRINT2(_L("#++ CFatBitCache::FindClosestFreeFatEntry() start idx:%d, start cache sec:%d"), aFatIndex, startFatCacheSec);

    ASSERT(aFatIndex >= KFatFirstSearchCluster);
    if(!UsableState())
        {
        ASSERT(0);
        return KErrCorrupt;
        }

    TUint32 fatSeekCacheSec = startFatCacheSec; //-- FAT cache sector number that has free FAT entry, used for search . 
    TUint32 fatSeekIndex = aFatIndex;           //-- FAT index to start search with

    //-- 1. look if FAT sector that corresponds to the aStartFatIndex already has free entries.
    //-- 2. if not, try to locate closest FAT cache sector that has by searching a bit vector
    if(FatSectorHasFreeEntry(fatSeekCacheSec))
        {
        }
    else
        {//-- look in iBitCache for '1' entries nearest to the fatCacheSec, right side priority 

        if(!iBitCache.Find(fatSeekCacheSec, 1, RBitVector::ENearestR))
            {//-- strange situation, there are no '1' bits in whole vector, search failed
            __PRINT(_L("#++ CFatBitCache::FindClosestFreeFatEntry() bit vector search failed!"));
            return KErrEof;
            }
    
        //-- bit cache found FAT sector(fatSeekCacheSec) that may have free FAT entries
        //-- calculate FAT entry start index in this sector
        fatSeekIndex = Max(KFatFirstSearchCluster, CacheSectorNumberToFatIndex(fatSeekCacheSec));
        }

    //-- here we have absolute FAT cache sector number, which may contain at least one free FAT entty
    ASSERT(FatSectorHasFreeEntry(fatSeekCacheSec));

    //-- ask FAT cache to find the exact index of free FAT entry in this particular FAT cache sector
    TInt  nRes;
    TBool bFreeEntryFound=EFalse;

    TRAP(nRes, bFreeEntryFound = iOwnerFatCache.FindFreeEntryInCacheSectorL(fatSeekIndex));
    if(nRes != KErrNone)
        {//-- it's possible on media read error
        return KErrCorrupt;
        }

    if(bFreeEntryFound)
        {//-- found free entry at aNewFreeEntryIndex
        aFatIndex = fatSeekIndex;
        return KErrNone;   
        }

    //-- bit cache mismatch; its entry ('1') indicates that cache sector number fatCacheSec has free FAT entries,
    //-- while in reality it doesnt. We need to fix the bit cache.
    //__PRINT1(_L("#++ CFatBitCache::FindClosestFreeFatEntry  fixing cache conflict; BitVec[%d]=0"), fatSeekCacheSec);
    SetFreeEntryInFatSector(fatSeekCacheSec, EFalse);

    return KErrNotFound;
    }


//-----------------------------------------------------------------------------
/**
    Print out the contents of the object. This is a debug only method
*/
void CFatBitCache::Dump() const
{
#if defined _DEBUG && defined FAT_BIT_CACHE_DEBUG 

    const TUint32 vecSz = iBitCache.Size();
    __PRINT2(_L("#++ CFatBitCache::Dump(): state:%d, entries:%d"), State(), vecSz);


    TBuf<120> printBuf;
    const TUint KPrintEntries = 32;
    
    TUint i;
    printBuf.Append(_L("    "));
    for(i=0; i<KPrintEntries; ++i)
    {
        printBuf.AppendFormat(_L("%02d "),i);
    }
    
    __PRINT(printBuf);
    for(i=0; i<vecSz;)
    {
        printBuf.Format(_L("%03d: "), i);
        for(TInt j=0; j<KPrintEntries; ++j)
        {
            if(i >= vecSz)
                break;

            printBuf.AppendFormat(_L("% d  "), (iBitCache[i]!=0));
            ++i;
        }    
        __PRINT(printBuf);
        
    }
#endif
}









































