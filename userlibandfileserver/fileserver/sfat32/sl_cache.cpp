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
// f32\sfat\sl_cache.cpp
// 
//

#include "sl_std.h"
#include "sl_cache.h"

//---------------------------------------------------------------------------------------------------------------------------------

/**
    CWTCachePage factory function.    
    @param  aPageSizeLog2 Log2(cache page size in bytes)
    @return a pointer to the created object.
*/
CWTCachePage* CWTCachePage::NewL(TUint32 aPageSizeLog2)
    {
    CWTCachePage* pSelf = new (ELeave)CWTCachePage;
    
    pSelf->iData.CreateMaxL(1 << aPageSizeLog2);

    return pSelf;
    }


CWTCachePage::CWTCachePage()
    {
    iStartPos = 0xDeadDeadul;
    iValid  = 0;
    }

CWTCachePage::~CWTCachePage() 
    { 
    iData.Close(); 
    }



//---------------------------------------------------------------------------------------------------------------------------------

CMediaWTCache::CMediaWTCache(TDriveInterface& aDrive)
          :iDrive(aDrive), iPageSizeLog2(0), iAllPagesValid(EFalse)
    {
    iCacheDisabled = EFalse;
    iCacheBasePos  = 0; 
    }

CMediaWTCache::~CMediaWTCache()
    {
    //-- delete pages
    TInt cnt = iPages.Count();
    while(cnt--)
        {
        delete iPages[cnt];
        }

    iPages.Close();
    }


/**
    Directory cache factory function.

    @param  aDrive  reference to the driver for media access.
    @param  aNumPages     number of cache pages to be created
    @param  aPageSizeLog2       Log2 of the page size in bytes, this is the cache read granularity
    @param  aWrGranularityLog2  Log2(cache write granularity)
    
    @return a pointer to the created object.
*/
CMediaWTCache* CMediaWTCache::NewL(TDriveInterface& aDrive, TUint32 aNumPages, TUint32 aPageSizeLog2, TUint32 aWrGranularityLog2)
    {

    CMediaWTCache* pSelf = new (ELeave) CMediaWTCache(aDrive);
    
    CleanupStack::PushL(pSelf);
    pSelf->InitialiseL(aNumPages, aPageSizeLog2, aWrGranularityLog2);
    CleanupStack::Pop();

    return pSelf;
    }

/**
    2nd stage constructor.
    @param  aNumPages number of pages in the directory cache.
    @param  aPageSizeLog2       Log2 of the page size in bytes, this is the cache read granularity
    @param  aWrGranularityLog2  Log2(cache write granularity)
*/
void CMediaWTCache::InitialiseL(TUint32 aNumPages, TUint32 aPageSizeLog2, TUint32 aWrGranularityLog2)
    {
    ASSERT(aNumPages && aPageSizeLog2);
    
    __PRINT3(_L("#CMediaWTCache::InitialiseL() Pages=%d, PageSzLog2=%d, WrGrLog2:%d"), aNumPages, aPageSizeLog2, aWrGranularityLog2);

    ASSERT(aNumPages);
    ASSERT(aPageSizeLog2);
    
    if(aWrGranularityLog2)
        {
        ASSERT(aWrGranularityLog2 >= KDefSectorSzLog2 && aWrGranularityLog2 <= aPageSizeLog2);
        }
    
    iPageSizeLog2 = aPageSizeLog2; 
    iWrGranularityLog2 = aWrGranularityLog2;

    //-- create cache pages
    for(TUint cnt=0; cnt<aNumPages; ++cnt)
        {
        CWTCachePage* pPage = CWTCachePage::NewL(aPageSizeLog2);
        iPages.AppendL(pPage);
        }

    InvalidateCache();  
    }


/**
    @return size of the cache in bytes. Can be 0.
*/
TUint32 CMediaWTCache::CacheSizeInBytes() const
    {
    const TUint32 cacheSz = iPages.Count() << iPageSizeLog2; //-- Page size is always power of 2
    return cacheSz;
    }

/**
Implementation of pure virtual function.
@see MWTCacheInterface::MakePageMRU()
*/
void CMediaWTCache::MakePageMRU(TInt64 /*aPos*/)
	{
	return;
	}

/**
Implementation of pure virtual function.
@see MWTCacheInterface::PageSizeInBytesLog2()
*/
TUint32 CMediaWTCache::PageSizeInBytesLog2() const
	{
	return iPageSizeLog2;
	}

/**
    Control method.

    @param  aFunction   control function
    @param  aParam1     just arbitrary parameter 
    @param  aParam2     just arbitrary parameter 
    @return Standard error code.
*/

TInt CMediaWTCache::Control(TUint32 aFunction, TUint32 aParam1, TAny* /*aParam2*/)
    {
    TInt nRes = KErrNotSupported;

#ifdef _DEBUG
    switch(aFunction)
        {
        //-- disable / enable cache, for debug
        //-- if aParam1 !=0 cache will be disabled, enabled otherwise
        case EDisableCache: 
            iCacheDisabled = aParam1 ? 1 : 0;
            nRes = KErrNone;
        break;

        case EDumpCache:
        break;
           
        case ECacheInfo:
        break;
   
        default:
            __PRINT1(_L("CMediaWTCache::Control() invalid function: %d"), aFunction);
            ASSERT(0);
        break;
        }
#else
    (void)aFunction; //-- supress warnings
    (void)aParam1;
    User::Invariant(); //-- don't call this method in release build
#endif //_DEBUG   
    
    return nRes;
    }

//-------------------------------------------------------------------------------------
/**
    Invalidate whole cache
*/
void CMediaWTCache::InvalidateCache(void)
    {
    const TUint nPages = iPages.Count();    
    for(TUint i=0; i<nPages; ++i)
        {
        iPages[i]->iValid=EFalse;
        }

    iAllPagesValid = EFalse;
    }

//-------------------------------------------------------------------------------------
/** 
    invalidate a single cache page if the aPos is cached (belongs to some page)
    If the cache user wants to invalidate some media address range, it will have to calculate 
    pages positions itself. The best way to do this - is to write another method that takes lenght of the 
    region being invalidated. 

    @param aPos media position. If it is cached, the corresponding single cache page will be marked as invalid
*/
void CMediaWTCache::InvalidateCachePage(TUint64 aPos)
    {
    const TUint nPages = iPages.Count();    
    for(TUint i=0; i<nPages; ++i)
        {
        if( iPages[i]->PosCached(aPos))  
            {
            iPages[i]->iValid=EFalse;
            iAllPagesValid = EFalse;
            break;
            }
        }

    }

//-------------------------------------------------------------------------------------

/**
    Find cache page by given media position.
    
    @param  aPos    linear media position
    @return positive cache page number or -1 if no pages containing data at aPos found.
*/
TInt CMediaWTCache::FindPageByPos(TInt64 aPos) const
    {
    const TUint nPages = iPages.Count();    
    for(TUint i=0; i<nPages; ++i)
        {
        if( iPages[i]->PosCached(aPos))  
            return i; 
        }

    return KErrNotFound;
    }

/**
    Push given page aPageNo to the 1st position in the pages array. Used for LRU mechanism
    
    @param  aPageNo page number to be made LRU
*/
void CMediaWTCache::MakePageLRU(TInt aPageNo)
    {
    ASSERT(aPageNo >=0);

    if(aPageNo <= 0)
        return; //-- nothing to do
    
    const TInt nPages = iPages.Count();
    ASSERT(aPageNo < nPages);

    if(aPageNo < nPages)
        {
        CWTCachePage* pPage=iPages[aPageNo];
    
        iPages.Remove(aPageNo);
        iPages.Insert(pPage,0); //-- insert the pointer to the 1st position in the array
        ASSERT(nPages == iPages.Count());
        }
    }

/*
    Find a spare page or evict the last from LRU list
    
    @return page number
*/
TUint32  CMediaWTCache::GrabPage() const
    {
    const TUint nPages = iPages.Count();

    if(!iAllPagesValid)
        {//-- try to find unused cache page
        for(TUint i=0; i<nPages; ++i)
            {
            if(! iPages[i]->iValid)
                return i; //-- found unused page
            }
        }

    //-- no spare pages, evict the last one, it shall be last used
    iAllPagesValid = ETrue;
    return nPages-1;
    }

/*
    Find a spare page or evict the last from LRU list, then read data to this page from media starting from aPos
    
    @param  aPos    media linear position from where the data will be read to the page
    @return cache page number
*/
TUint32 CMediaWTCache::GrabReadPageL(TInt64 aPos)
    {
    //-- find a spare or page to evict
    TUint nPage = GrabPage();
    CWTCachePage& page = *iPages[nPage]; 

    //-- read data to this page
    page.iStartPos = CalcPageStartPos(aPos);
    
    __PRINT4(_L("#CMediaWTCache::GrabReadPageL() Reading page:%d, Pos=0x%x, PageStartPos=0x%x, page=0x%X"),nPage, (TUint32)aPos, (TUint32)page.iStartPos, iPages[nPage]);
        
    const TInt nErr = iDrive.ReadNonCritical(page.iStartPos, PageSize(), page.iData);
    if(nErr !=KErrNone)
        {//-- some serious problem occured during reading, invalidate cache.
        InvalidateCache();
        User::Leave(nErr);
        }
    
    page.iValid = ETrue;

    return nPage;
    }

/**
    Try to find the page with cached data at "aPos" media position.
    If such page found, returns its number, otherwise takes least recently used page and reads data there.
    
    @param  aPos    media linear position to find in the cache
    @return cache page number

*/
TUint32 CMediaWTCache::FindOrGrabReadPageL(TInt64 aPos)
    {
    //-- find out if aPos is in cache
    TInt nPage=FindPageByPos(aPos);
    
    if(nPage < 0)
        {//-- no page contains data to read
        nPage = GrabReadPageL(aPos); //-- find a spare page and read data into it
        }

    return nPage;
    }

/**
    Finds out if the media position "aPosToSearch" is in the cache and returns cache page information in this case.

    @param  aPosToSearch    linear media position to lookup in the cache
    @param  aCachedPosStart if "aPosToSearch" is cached, here will be media position of this page start
    
    @return 0 if aPosToSearch isn't cached, otherwise  cache page size in bytes (see also aCachedPosStart).
*/
TUint32 CMediaWTCache::PosCached(TInt64 aPosToSearch)
    {
    TInt nPage = FindPageByPos(aPosToSearch);
    if(nPage <0 )
        return 0; //-- cache page containing aPos not found

    return PageSize();
    }

/**
    Read data from the media through the directory cache.
    
    @param  aPos    linear media position to start reading with
    @param  aLength how many bytes to read
    @param  aDes    data will be placed there
*/
void CMediaWTCache::ReadL(TInt64 aPos,TInt aLength,TDes8& aDes)
    {
    
#ifdef _DEBUG
    if(iCacheDisabled)
        {//-- cache is disabled for debug purposes
        User::LeaveIfError(iDrive.ReadNonCritical(aPos, aLength, aDes));
        return;
        }
#endif //_DEBUG

    const TUint32 PageSz = PageSize();//-- cache page size

    //-- find out if aPos is in cache. If not, find a spare page and read data there
    TInt nPage = FindOrGrabReadPageL(aPos);
    CWTCachePage* pPage = iPages[nPage];

    const TUint32 bytesToPageEnd = (TUint32)(pPage->iStartPos+PageSz - aPos); //-- number of bytes from aPos to the end of the page

//    __PRINT5(_L("CMediaWTCache::ReadL: aPos=%lx, aLength=%x, page:%lx, pageSz:%x, bytesToPageEnd=%x"), aPos, aLength, pPage->iStartPos, PageSz, bytesToPageEnd);
    if((TUint32)aLength <= bytesToPageEnd) 
        {//-- the data section is in the cache page entirely, take data directly from the cache
        aDes.Copy(pPage->PtrInCachePage(aPos), aLength);
        }
    else
        {//-- Data to be read cross cache page boundary or probably we have more than 1 page to read

        TUint32 dataLen(aLength);   //-- current data length
        TInt64  currMediaPos(aPos); //-- current media position

        //-- 1. read data that are already in the current page
        aDes.Copy(pPage->PtrInCachePage(currMediaPos), bytesToPageEnd);

        dataLen -= bytesToPageEnd;
        currMediaPos += bytesToPageEnd;

        //-- 2. read whole pages of data
        while(dataLen >= PageSz)
            {
            nPage = FindOrGrabReadPageL(currMediaPos); //-- find out if currMediaPos is in cache. If not, find a spare page and read data there
            pPage = iPages[nPage];

            aDes.Append(pPage->PtrInCachePage(currMediaPos),PageSz);
        
            dataLen -= PageSz;
            currMediaPos += PageSz;
        
            MakePageLRU(nPage); //-- push the page to the top of the priority list
            }

        //-- 3. read the rest of the data
        if(dataLen >0)
            {
            nPage = FindOrGrabReadPageL(currMediaPos); //-- find out if currMediaPos is in cache. If not, find a spare page and read data there
            pPage = iPages[nPage];

            aDes.Append(pPage->PtrInCachePage(currMediaPos), dataLen);
            }

        } //else((TUint32)aLength <= bytesToPageEnd) 


    MakePageLRU(nPage); //-- push the page to the top of the priority list
    
    }

/**
    Write data to the media through the directory cache.
    
    @param  aPos    linear media position to start writing with
    @param  aDes    data to write
*/
void CMediaWTCache::WriteL(TInt64 aPos,const TDesC8& aDes)
    {

#ifdef _DEBUG
    if(iCacheDisabled)
        {//-- cache is disabled for debug purposes
        User::LeaveIfError(iDrive.WriteCritical(aPos,aDes));
        return;
        }
#endif //_DEBUG

          TUint32 dataLen = aDes.Size();
    const TUint8* pData   = aDes.Ptr();
    const TUint32 PageSz  = PageSize(); //-- cache page size

    //-- find out if aPos is in cache. If not, find a spare page and read data there
    TInt nPage = FindOrGrabReadPageL(aPos);
    CWTCachePage* pPage = iPages[nPage];

    const TUint32 bytesToPageEnd = (TUint32)(pPage->iStartPos+PageSize() - aPos); //-- number of bytes from aPos to the end of the page
//    __PRINT5(_L("CMediaWTCache::WriteL: aPos=%lx, aLength=%x, page:%lx, pageSz:%x, bytesToPageEnd=%x"), aPos, dataLen, pPage->iStartPos, PageSz, bytesToPageEnd);
    if(dataLen <= bytesToPageEnd)
        {//-- data section completely fits to the cache page
        Mem::Copy(pPage->PtrInCachePage(aPos), pData, dataLen);   //-- update cache

        //-- make small write a multiple of a write granularity size (if it is used at all)
        //-- this is not the best way to use write granularity, but we would need to refactor cache pages code to make it normal
        TPtrC8 desBlock(aDes);
        
        if(iWrGranularityLog2)
            {//-- write granularity is used
            const TInt64  newPos = (aPos >> iWrGranularityLog2) << iWrGranularityLog2; //-- round position down to the write granularity size
            TUint32 newLen = (TUint32)(aPos - newPos)+dataLen;  //-- round block size up to the write granularity size
            newLen = RoundUp(newLen, iWrGranularityLog2);
       
            const TUint8* pd = pPage->PtrInCachePage(newPos);
            desBlock.Set(pd, newLen);
            aPos = newPos;
            }


        //-- write data to the media
        const TInt nErr = iDrive.WriteCritical(aPos, desBlock); 
        if(nErr != KErrNone)
            {//-- some serious problem occured during writing, invalidate cache.
            InvalidateCache();
            User::Leave(nErr);
            }

        }
    else
        {//-- Data to be written cross cache page boundary or probably we have more than 1 page to write
         //-- this is a very rare case.   

        TInt64  currMediaPos(aPos); //-- current media position

        //-- 1. update the current page
        Mem::Copy(pPage->PtrInCachePage(currMediaPos), pData, bytesToPageEnd);

        pData += bytesToPageEnd;
        currMediaPos += bytesToPageEnd;
        dataLen -= bytesToPageEnd;

        //-- 2. write whole pages of data to the cache
        while(dataLen >= PageSz)
            {
            nPage = FindPageByPos(currMediaPos); //-- ?? shall we read data there ??
            if(nPage >=0)
                {
                pPage = iPages[nPage];
                Mem::Copy(pPage->PtrInCachePage(currMediaPos), pData, PageSz);
                MakePageLRU(nPage); //-- push the page to the top of the priority list
                }
            else
                nPage=0;

            pData += PageSz;
            currMediaPos += PageSz;
            dataLen -= PageSz;
            }

        //-- 3. write the rest of the data
        if(dataLen)
            {
            nPage = FindOrGrabReadPageL(currMediaPos); //-- find out if currMediaPos is in cache. If not, find a spare page and read data there
            pPage = iPages[nPage];

            Mem::Copy(pPage->PtrInCachePage(currMediaPos), pData, dataLen);
            }

    //-- write data to the media
    const TInt nErr = iDrive.WriteCritical(aPos,aDes); 
    if(nErr != KErrNone)
        {//-- some serious problem occured during writing, invalidate cache.
        InvalidateCache();
        User::Leave(nErr);
        }

        }// else(dataLen <= bytesToPageEnd)

    

    MakePageLRU(nPage); //-- push the page to the top of the priority list
    }

















