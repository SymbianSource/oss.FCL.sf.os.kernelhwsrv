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
// f32\sfat32\inc\sl_facache32.h
// FAT32 various cache classes definition
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef SL_FAT_CACHE_32_H
#define SL_FAT_CACHE_32_H

#include "sl_fatcache.h"

class CFat32LruCachePage;


//---------------------------------------------------------------------------------------------------------------------------------

/**
    FAT32 LRU paged cache. Used for FAT32 only.

    Consists of LRU list of cache pages; Each page is logically divided to page sectors. The number of pages depends on the
    maximal amount of memory this cache allowed to use.  Usually, whole FAT32 can not be cached fully because of its large size.
    So, this type caches the most receinlly used areas of the FAT32. This is the wriite-back cache.
 
    Read granularity: One page, which size is 2^aRdGranularityLog2
    Write granularity: cache's page sector; its size is 2^aWrGranularityLog2
*/
class CFat32LruCache : public CFatPagedCacheBase
{
 public:

    static CFat32LruCache* NewL(CFatMountCB* aOwner, TUint32 aMaxMemSize, TUint32 aRdGranularityLog2, TUint32 aWrGranularityLog2);

    //-- overrides from base class
    virtual void Close(TBool aDiscardDirtyData);
    virtual void FlushL();

    virtual TUint32 ReadEntryL(TUint32 aIndex);
    virtual void WriteEntryL(TUint32 aIndex, TUint32 aEntry);

    virtual TInt Invalidate();
    virtual TInt InvalidateRegion(TUint32 aStartEntry, TUint32 aNumEntries);
    

    virtual CFatBitCache* BitCacheInterface();

 public:     
     TBool FindFreeEntryInCacheSectorL(TUint32& aFatEntryIndex);

 private:
    
    void InitialiseL(CFatMountCB* aOwner, TUint32 aFatSize, TUint32 aRdGranularityLog2, TUint32 aWrGranularityLog2);
    
    CFat32LruCache();
    CFat32LruCache(const CFat32LruCache&);
    CFat32LruCache& operator=(const CFat32LruCache&);
    
    TBool ReadCachedEntryL(TUint32 aFatIndex, TFat32Entry& aResult);
    TBool WriteCachedEntryL(TUint32 aFatIndex, TFat32Entry aFatEntry);

    CFat32LruCachePage* DoGetSpareCachePageL();

    void AssertCacheReallyClean() ;

 private:

    typedef TDblQue<CFat32LruCachePage> TPageList;
    typedef TDblQueIter<CFat32LruCachePage> TPageIterator;

    TUint     iNumPagesAllocated;   ///< number of pages currently allocated
    TUint     iMaxPages;            ///< maximal pages allowed to allocate
    TPageList iPageList;            ///< LRU list of cache pages.

    
    CFatBitCache *iBitCache; ///< pointer to the FAT bit supercache

};


//---------------------------------------------------------------------------------------------------------------------------------

/**
    FAT32 LRU cache page. Used only by CFat32LruCache. 
*/
class CFat32LruCachePage : public CFatCachePageBase
{
 public:

    static CFat32LruCachePage* NewL(CFatPagedCacheBase& aCache);

    //-- overrides
    virtual TBool ReadCachedEntryL (TUint32 aFatIndex, TUint32& aResult);
    virtual TBool WriteCachedEntryL(TUint32 aFatIndex, TUint32 aFatEntry); 
    virtual TUint32 ReadFromMediaL(TUint32 aFatIndex);
    //----

 private:
    CFat32LruCachePage(CFatPagedCacheBase& aCache);

    //-- outlaws here
    CFat32LruCachePage();
    CFat32LruCachePage(const CFat32LruCachePage&);
    CFat32LruCachePage& operator=(const CFat32LruCachePage&);

    inline TFat32Entry* GetEntryPtr(TUint32 aFatIndex) const;
    virtual void DoWriteSectorL(TUint32 aSector);

 private: 
    enum {KFat32EntryMask = 0x0FFFFFFF}; ///< FAT32 entry mask 

 public:
    TDblQueLink iLink; ///< list link object. See TPageList
};

//---------------------------------------------------------------------------------------------------------------------------------

/**
    FAT32 bit supercache. This is a special cache above the CFat32LruCache.
    Used for quick lookup for the free entries in FAT32 table without accessing media and thrashing FAT32 LRU cache.
    
    Logically consists of a bit vector, where each bit represents one FAT sector (one unit of read granularity within CFat32LruCachePage)
    for the _whole_ FAT table.
    
    If some bit in the vector is set to '1' it means that the corresponding FAT cache sector (not necessarily currently cached) _may_ have
    a at least one free FAT entry. If the bit is '0' it means that there is no free fat entries in this part of the FAT.

    The situation when bit set to '1' corresponds to the FAT cache sector without free entries is quite possible, but it is resolved by this cache.
    The situation when '0' bit corresponds to the fat sector that _does_ contain free entries is extremely unlikely and can be
    caused by direct raw writes to the FAT, for example. Nothing terribly wrong with this situation, the search for free entry will fall back
    to the old algorithm CFatTable::FindClosestFreeClusterL() .
    
    The information in this cache is also updated on flushing dirty sectros by  CFat32LruCachePage.


*/
class CFatBitCache : public CBase
{
 public:

    ~CFatBitCache();
 
    static CFatBitCache* New(CFat32LruCache& aOnwerFatCache);

    void Close();
   
    TBool StartPopulating();
    TBool FinishPopulating(TBool aSuccess);

    
    /** possible states of this cache */
    enum TState
        {
        EInvalid,       ///< initial, invalid
        ENotPopulated,  ///< initialised, but not populated yet
        EPopulating,    ///< in the process of populating
        EPopulated,     ///< successfully populated; the only consistent state.
        };
    
    inline TState State() const;
    inline TBool  UsableState() const;
    inline TBool  FatSectorHasFreeEntry(TUint32 aFatSectorNum) const;
    inline void   SetFreeEntryInFatSector(TUint32 aFatSectorNum, TBool aHasFreeEntry);

    TBool SetFreeFatEntry(TUint32 aFatIndex);
    TInt  FindClosestFreeFatEntry(TUint32& aFatIndex);
    void  MarkFatRange(TUint32 aStartFatIndex, TUint32 aEndFatIndex, TBool aAsFree);


    void Dump() const;

 private:

    //-- outlaws
    CFatBitCache();
    CFatBitCache(const CFatBitCache&);
    CFatBitCache& operator=(const CFatBitCache&);

    CFatBitCache(CFat32LruCache& aOnwerFatCache);

    TInt Initialise();

    inline void SetState(TState aState);
    inline TUint32 FatIndexToCacheSectorNumber(TUint32 aFatIndex) const;
    inline TUint32 CacheSectorNumberToFatIndex(TUint32 aCacheSecNum) const;

 private:

    TState          iState;             ///< internal state of the cache
    RBitVector      iBitCache;          ///< bit vector itself
    TUint32         iFatIdxToSecCoeff;  ///< Log2(FatCacheSectorSize/Sizeof(FAT32 entry)). Used to convert FAT32 index to FAT32 cache sector number and back.
    CFat32LruCache& iOwnerFatCache;     ///< reference to the owner FAT32 LRU cache

    DBG_STATEMENT(TUint  iPopulatingThreadId;)  ///< used in debug mode for checking multithreading issues
};

//---------------------------------------------------------------------------------------------------------------------------------

#include "sl_fatcache32.inl"

#endif //SL_FAT_CACHE_32_H























