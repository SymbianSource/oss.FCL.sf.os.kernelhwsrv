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
// f32\sfat\sl_disk.cpp
// 
//

#include "sl_disk.h"
#include "sl_cache.h"
#include "sl_dir_cache.h"

/**
@file
*/


//################################################################################################################################
//#     CRawDisk implementation
//################################################################################################################################


/**
    Factory function. Constructs objects of the classes derived from CRawDisk.
    
    @param  aOwner      reference to the onwning FAT Mount class
    @param  aLocDrvCaps local drive capabilities from the media driver
    @return pointer to the constructed object. May be NULL on error.
*/
CRawDisk* CRawDisk::NewL(CFatMountCB& aOwner, const TLocalDriveCaps& aLocDrvCaps)
{
    __PRINT1(_L("CRawDisk::NewL() drv:%d"), aOwner.DriveNumber());

    if(aLocDrvCaps.iMediaAtt & KMediaAttVariableSize)
    {//-- this is the RAM drive "attribute"
        ASSERT((aLocDrvCaps.iDriveAtt & (KDriveAttInternal|KDriveAttLocal)) && aLocDrvCaps.iType == EMediaRam);
        if(!aLocDrvCaps.iBaseAddress)
        {
            ASSERT(0);
            return NULL;
        }

        return CRamDisk::NewL(aOwner);
    }

    //-- create CAtaDisk by default
    return CAtaDisk::NewL(aOwner);
}


CRawDisk::CRawDisk(CFatMountCB& aOwner)
    {
    iFatMount = &aOwner;
    }

/**
    Default implementation. Initialises and re-initialises the object.
*/
void CRawDisk::InitializeL()
    {
    ASSERT(iFatMount);
    }


TInt CRawDisk::GetLastErrorInfo(TDes8& /*aErrorInfo*/) const
	{
	return KErrNotSupported;
	}

//################################################################################################################################
//##    CAtaDisk class implementation
//################################################################################################################################

CAtaDisk::CAtaDisk(CFatMountCB& aFatMount)
         :CRawDisk(aFatMount), iDrive(aFatMount.DriveInterface())
	{
	}

CAtaDisk::~CAtaDisk()
    {
    delete ipDirCache;
    delete iUidCache;
    
    }


//-------------------------------------------------------------------------------------

/**
    CAtaDisk factory method.
    
    @param  aFatMount reference to the owner.
    @return pointer to the constructed object.
*/
CAtaDisk* CAtaDisk::NewL(CFatMountCB& aFatMount)
	{
	__PRINT1(_L("CAtaDisk::NewL() drv:%d"), aFatMount.DriveNumber());

    CAtaDisk* pSelf = new (ELeave) CAtaDisk(aFatMount);
    
    CleanupStack::PushL(pSelf);
    
    pSelf->ConstructL();
    pSelf->InitializeL();
    
    CleanupStack::Pop();

	return pSelf;
	}

//-------------------------------------------------------------------------------------

/** 2nd stage constructor */
void CAtaDisk::ConstructL()
    {
    //===========================  create data WT cache that is primarily used for caching exacutable modules' UIDs
    const TUint32 KUidCachePageSzLog2 = 9; //-- 512 bytes in page 
    const TUint32 KUidCachePages = 64;     //-- 64 pages; total cache size is 32K 

    iUidCache = CMediaWTCache::NewL(iDrive, KUidCachePages, KUidCachePageSzLog2, 0);


    //=========================== create directory cache
    
    //-- Get dir. cache parameters from config. They may be set in estart.txt for a specified drive.
    const TUint32 KDirCacheSize = iFatMount->FatConfig().DirCacheSize(); //- Total directory cache size, bytes.
    const TUint32 KMaxDirCachePageSzLog2 = iFatMount->FatConfig().DirCacheMaxPageSizeLog2(); //- Log2 of the Max. dir. cache page.

    __PRINT2(_L("CAtaDisk::ConstructL() Dir Cache config:%d,%d"),KDirCacheSize,KMaxDirCachePageSzLog2);

    ASSERT(KDirCacheSize >= K1KiloByte && KDirCacheSize <= K1MegaByte);
    ASSERT((KMaxDirCachePageSzLog2 >= KDefSectorSzLog2) && (Pow2(KMaxDirCachePageSzLog2) <= KDirCacheSize));

    //-- calculate the size and number of pages for the dir. cache. 
    //-- if the mount's cluster size is less than max. page size from config, the page size will be cluster size.
    //-- otherwise it will be the value from config. I.e  the minimal page size is cluster size; the maximal page size is taken from config.
    //-- The number of pages depends on total cache size and page size.
    const TUint clustSizeLog2 = iFatMount->ClusterSizeLog2(); //-- current FAT cluster size Log2
    const TUint32 pageSzLog2 = (clustSizeLog2 <= KMaxDirCachePageSzLog2) ? clustSizeLog2 : KMaxDirCachePageSzLog2;
    const TUint32 numPages = KDirCacheSize / (Pow2(pageSzLog2));

    ASSERT(!ipDirCache);

#ifdef USE_DP_DIR_CACHE

    //=========================== create Demand Paging type of the directory cache

    // initialize cache memory manager as all file systems have mounted by now
	if(CCacheMemoryManagerFactory::CacheMemoryManager())
		{
		// Note: the configuration data of min and max cache size are aligned with the memory size it
		//	occupies in kernel as we are using demand paging subsystem for dynamic cache size support. 
		//  Therefore, they are refered as 'Mem Size' in following calculation.
		//  However, 'Data Size' refers to the logical size of a page, i.e. the actual data size each page
		//  contains.
		// The constraints we have to consider when setting up the dynamic cache:
		// 	1. each page's data size is aligned with cluster size, unless cluster size is bigger than
		//  	the default maximum page size allowed (typically 32 KB).
		// 	2. if page's data size is smaller than segment size (typically 4 KB), i.e. the unit size of 
		//     	demand paging subsystem's page management, we will still use up the whole segment for
		// 		that page.
		//  3. the default min and max cache's memory size is pre-defined in  fat_config.cpp file.
		// 		(see KDef_DynamicDirCacheMin & KDef_DynamicDirCacheMax).

		// calculate page data size (logical view of page size)
	    const TUint32 DefMaxCachePageLog2 = iFatMount->FatConfig().DynamicDirCacheMaxPageSizeLog2();
	    const TUint32 PageDataSizeLog2 = clustSizeLog2 < DefMaxCachePageLog2 ? clustSizeLog2 : DefMaxCachePageLog2;
	    
		// calculate page number, based on memory size we have reserved
	    const TUint32 SegmentSizeLog2 = CCacheMemoryManagerFactory::CacheMemoryManager()->SegmentSizeInBytesLog2();
	    const TUint32 PageMemSizeLog2 = PageDataSizeLog2 < SegmentSizeLog2 ? SegmentSizeLog2 : PageDataSizeLog2;
	    TUint32 CacheSizeMinInPages = iFatMount->FatConfig().DynamicDirCacheSizeMin() >> PageMemSizeLog2;
	    TUint32 CacheSizeMaxInPages = iFatMount->FatConfig().DynamicDirCacheSizeMax() >> PageMemSizeLog2;

	    // cache memory client is connected via name 
	    TBuf<0x20> clientName = _L("CACHE_MEM_CLIENT:");
		clientName.Append('A'+iFatMount->DriveNumber());

		TRAPD(err, ipDirCache = CDynamicDirCache::NewL(iDrive, CacheSizeMinInPages, CacheSizeMaxInPages, PageDataSizeLog2, KDefSectorSzLog2, clientName));
		if (err == KErrNone)
	    	return;
		
        //-- fall back to constructing old type of cache

        }
#endif // USE_DP_DIR_CACHE

    //=========================== create legacy type of the directory cache
    ASSERT(!ipDirCache);

    ipDirCache = CMediaWTCache::NewL(iDrive, numPages, pageSzLog2, KDefSectorSzLog2);
    __PRINT3(_L("CDirCache::NewL(drive: %C, NumPages=%d, PageSize=%u)"), 'A'+iFatMount->DriveNumber(), numPages, 1<<pageSzLog2);
    
    }

//-------------------------------------------------------------------------------------

/**
    Initialises and re-initialises the object.
*/
void CAtaDisk::InitializeL()
{
    CRawDisk::InitializeL();
    
    //-- there is a little issue here. after formatting FAT mounts's cluster size can change.
    //-- dir. cache page size depends on the cluster size. This method doesn't change the dir. cache page size.
    //-- At present it is done in CFatMountCB::InitializeL() that deletes this object and then reconstructs it again.

    //-- invalidate directory cache here
    ipDirCache->InvalidateCache();
    
    TInt64  cacheBasePos;
    
    if(iFatMount->FatType() == EFat32)
        {
        //-- this is FAT32, all directories including Root are files and aligned to the cluster heap boundary
        //-- set dir. cache base position to the cluster heap boundary
        const TUint32 offsetMask = (1 << iFatMount->ClusterSizeLog2() )-1;
        cacheBasePos = (iFatMount->ClusterBasePosition() & offsetMask);
        }
    else
        {
        //-- this is FAT12/16. Root directory is a separate volume object and has no alignment.
        //-- set cache base position to its beginning.
        cacheBasePos = iFatMount->StartOfRootDirInBytes();
        }

    ipDirCache->SetCacheBasePos(cacheBasePos);
    
}

//-------------------------------------------------------------------------------------

/**
    Read data from the media through LRU data cache cache. 

    @param  aPos        absolute media position
    @param  aLength     how many bytes to read
    @param  aDes        data descriptor

    @leave on error
*/
void CAtaDisk::ReadCachedL(TInt64 aPos,TInt aLength,TDes8& aDes) const
	{
    __PRINT3(_L("CAtaDisk::ReadL() pos:%u:%u, len:%u"), I64HIGH(aPos), I64LOW(aPos), aLength);
    iUidCache->ReadL(aPos, aLength, aDes);
	}

//-------------------------------------------------------------------------------------

/**
    Write data to the media through LRU data cache

    @param  aPos        absolute media position
    @param  aDes        data descriptor

    @leave on error
*/
void CAtaDisk::WriteCachedL(TInt64 aPos, const TDesC8& aDes)
	{
    __PRINT3(_L("CAtaDisk::WriteL() pos:%u:%u, len:%u"), I64HIGH(aPos), I64LOW(aPos), aDes.Size());
    iUidCache->WriteL(aPos, aDes);
	}


//-------------------------------------------------------------------------------------

/**
    Read data from the media directly without any caches.
    Mostly used by file IO

    @param  aPos        absolute media position
    @param  aLength     how many bytes to read
	@param  aTrg		Pointer to the data descriptor, i.e. (const TAny*)(&TDes8)
	@param  aMessage	Refrence to server message from request
	@param  anOffset	Offset into read data to write

    @leave on error
*/
void CAtaDisk::ReadL(TInt64 aPos,TInt aLength,const TAny* aTrg,const RMessagePtr2 &aMessage,TInt anOffset, TUint aFlag) const
	{

	__PRINT4(_L("CAtaDisk::ReadL() pos:%u:%u, len:%u, offset:%u"), I64HIGH(aPos), I64LOW(aPos), aLength, anOffset);
	User::LeaveIfError(iDrive.ReadNonCritical(aPos,aLength,aTrg,aMessage,anOffset, aFlag));
	}

//-------------------------------------------------------------------------------------

/**
    Write data to the media directly without any cached.
    Mostly used by file IO

    This method shall invalidate some data caches to keep them in synch with the media.

    @param aPos		Media position in bytes
    @param aLength	Length in bytes of write
	@param aTrg		Pointer to the data descriptor, i.e. (const TAny*)(&TDes8)
	@param aMessage	Refrence to server message from request, contains data
	@param anOffset	Offset into write data to use in write

    @leave on error
*/
void CAtaDisk::WriteL(TInt64 aPos,TInt aLength,const TAny* aSrc,const RMessagePtr2 &aMessage,TInt anOffset, TUint aFlag)
	{
    __PRINT4(_L("CAtaDisk::WriteL() pos:%u:%u, len:%u, offset:%u"), I64HIGH(aPos), I64LOW(aPos), aLength, anOffset);

	//-- write data to the media directly
    User::LeaveIfError(iDrive.WriteNonCritical(aPos,aLength,aSrc,aMessage,anOffset, aFlag));

    //-- we need to invalidate UID cache page that corresponds to aPos (if any). This is UID caching specific. UID is stored in the first few bytes of 
    //-- the executable module and therefore belongs to one cache page only.
    //-- If someone writes to the beginning of the exe module file, corresponding UID cache page will be invalidated and re-read from the media later
    iUidCache->InvalidateCachePage(aPos); 

    //-- invalidate affected(if any) part of the FAT cache in the case if someone used to write data to FAT area, which usually do not happen 
    iFatMount->FAT().InvalidateCacheL(aPos,aLength);

	}

//-------------------------------------------------------------------------------------

/** Get information for last disk error */
TInt CAtaDisk::GetLastErrorInfo(TDes8& aErrorInfo) const
	{
	return iDrive.GetLastErrorInfo(aErrorInfo);
	}


//-------------------------------------------------------------------------------------
/** Invalidate whole UID cache */
void CAtaDisk::InvalidateUidCache()
{
    ASSERT(iUidCache);
    iUidCache->InvalidateCache();
}

/** 
    Invalidate the UID cache page that has aPos cached.
    This method doesn't pay attention to the length of the block being invalidated because
    UID lives in the very beginning of the exe module and always fits into a single page
*/
void CAtaDisk::InvalidateUidCachePage(TUint64 aPos)
{
    ASSERT(iUidCache);
    iUidCache->InvalidateCachePage(aPos);
}


//################################################################################################################################
//##    CRamDisk class implementation
//################################################################################################################################


/**
    CRamDisk factory method.
    
    @param  aFatMount reference to the owner.
    @return pointer to the constructed object.
*/
CRamDisk* CRamDisk::NewL(CFatMountCB& aFatMount)
	{
    __PRINT1(_L("CRamDisk::NewL() drv:%d"), aFatMount.DriveNumber());
	CRamDisk* pSelf = new(ELeave)CRamDisk(aFatMount);

    CleanupStack::PushL(pSelf);
  
    pSelf->InitializeL();
    
    CleanupStack::Pop();

	return pSelf;
	}

CRamDisk::CRamDisk(CFatMountCB& aFatMount)
	     :CRawDisk(aFatMount)
    {
	}

//-------------------------------------------------------------------------------------

/**
    Initialises and re-initialises the object.
*/
void CRamDisk::InitializeL()
{
    CRawDisk::InitializeL();

    //-- set the RAM disk base
    TLocalDriveCapsV2 caps;
    TPckg<TLocalDriveCapsV2> capsPckg(caps);
    User::LeaveIfError(iFatMount->LocalDrive()->Caps(capsPckg));
  
    ASSERT(caps.iMediaAtt & KMediaAttVariableSize);
    
    //-- set RAM disk base
    iRamDiskBase = caps.iBaseAddress; 
    ASSERT(iRamDiskBase);
}



/** @return the start address of the Ram Drive in low memory */
TUint8* CRamDisk::RamDiskBase() const
	{
	return iRamDiskBase;
	}

//-------------------------------------------------------------------------------------
//
// Read aLength of data from the disk
//
void CRamDisk::ReadCachedL(TInt64 aPos,TInt aLength,TDes8& aDes) const
	{
	
	__PRINT3(_L("CRamDisk::ReadL Base 0x%x Pos 0x%x, Len %d"),RamDiskBase(),I64LOW(aPos),aLength);
	__ASSERT_ALWAYS((aPos+aLength<=I64INT(iFatMount->Size())) && (aLength>=0),User::Leave(KErrCorrupt));
	Mem::Copy((TUint8*)aDes.Ptr(),RamDiskBase()+I64LOW(aPos),aLength);
	aDes.SetLength(aLength);
	}

//-------------------------------------------------------------------------------------
//
// Write aLength of data to the disk
//
void CRamDisk::WriteCachedL(TInt64 aPos,const TDesC8& aDes)
	{

	__PRINT3(_L("CRamDisk::WriteL Base 0x%x Pos 0x%x, Len %d"),RamDiskBase(),aPos,aDes.Length());
	__ASSERT_ALWAYS(aPos+aDes.Length()<=I64INT(iFatMount->Size()),User::Leave(KErrCorrupt));
	Mem::Copy(RamDiskBase()+I64LOW(aPos),(TUint8*)aDes.Ptr(),aDes.Length());
	}
	

//-------------------------------------------------------------------------------------
//
// Read from ramDrive into thread relative descriptor
//
void CRamDisk::ReadL(TInt64 aPos,TInt aLength,const TAny* /*aTrg*/,const RMessagePtr2 &aMessage,TInt anOffset, TUint /*aFlag*/) const
	{
	__PRINT2(_L("CRamDisk::ReadL TAny* Pos 0x%x, Len %d"),aPos,aLength);
	__ASSERT_ALWAYS((aPos+aLength<=I64INT(iFatMount->Size())) && (aLength>=0),User::Leave(KErrCorrupt));
	TUint8* pos=RamDiskBase()+I64LOW(aPos);
	TPtrC8 buf(pos,aLength);
	aMessage.WriteL(0,buf,anOffset);
	}

//-------------------------------------------------------------------------------------
//
// Write from thread relative descriptor into ramDrive
//
void CRamDisk::WriteL(TInt64 aPos,TInt aLength,const TAny* /*aSrc*/,const RMessagePtr2 &aMessage,TInt anOffset, TUint /*aFlag*/)
	{
	__PRINT2(_L("CRamDisk::WriteL TAny* Pos 0x%x, Len %d"),aPos,aLength);
	__ASSERT_ALWAYS(aPos+aLength<=I64INT(iFatMount->Size()),User::Leave(KErrCorrupt));
	TUint8* pos=RamDiskBase()+I64LOW(aPos);
	TPtr8 buf(pos,aLength);
	aMessage.ReadL(0,buf,anOffset);
	}



























