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
// f32\sfat32\fat_table32.cpp
// FAT32 File Allocation Table classes implementation
// 
//

/**
 @file
 @internalTechnology
*/



#include "sl_std.h"
#include "sl_fatcache32.h"
#include "fat_table32.h"




//---------------------------------------------------------------------------------------------------------------------------------------
/** 
    Implements automatic locking object.
    Calls TDriveInterface::AcquireLock() on construction and TDriveInterface::ReleaseLock() on destruction. 
    Can be constructed on the stack only.
*/
class XAutoLock
    {
     public:
       inline XAutoLock(CFatMountCB* apOwner) : iDrv(apOwner->DriveInterface()) {iDrv.AcquireLock();}
       inline XAutoLock(TDriveInterface& aDrv) : iDrv(aDrv) {iDrv.AcquireLock();}
       inline ~XAutoLock() {iDrv.ReleaseLock();}

     private:
        void* operator new(TUint); //-- disable creating objects on heap.
        void* operator new(TUint, void*);

     private:
        TDriveInterface &iDrv; ///< reference to the drive interface
    };


//---------------------------------------------------------------------------------------------------------------------------------------



//#######################################################################################################################################
//#     CFatTable class implementation 
//#######################################################################################################################################

/**
    FAT object factory method.
    Constructs either CAtaFatTable or CRamFatTable depending on the media type parameter

    @param aOwner Pointer to the owning mount
    @param aLocDrvCaps local drive attributes
    @leave KErrNoMemory
    @return Pointer to the Fat table
*/
CFatTable* CFatTable::NewL(CFatMountCB& aOwner, const TLocalDriveCaps& aLocDrvCaps)
	{
    CFatTable* pFatTable=NULL;
    
    switch(aLocDrvCaps.iType)
        {
        case EMediaRam:
		    {//-- this is RAM media, try to create CRamFatTable instance.
            const TFatType fatType = aOwner.FatType();
            
            if(fatType != EFat16 && fatType != EFat32)
                {//-- CRamFatTable doesn't support FAT12, FAT16 & FAT32 only.
                __PRINT1(_L("CFatTable::NewL() CRamFatTable doesn't support this FAT type:%d"), fatType);
                ASSERT(0);
                return NULL;
                }
            
            pFatTable = CRamFatTable::NewL(aOwner);
            }
        break;

        default:
            //-- other media
            pFatTable = CAtaFatTable::NewL(aOwner);
        break;
        };

	return pFatTable;
	}


CFatTable::CFatTable(CFatMountCB& aOwner)
{
    iOwner = &aOwner;
    ASSERT(iOwner);
}

CFatTable::~CFatTable()
{
    //-- destroy cache ignoring dirty data in cache
    //-- the destructor isn't an appropriate place to flush the data.
    Dismount(ETrue); 
}

//-----------------------------------------------------------------------------

/**
    Initialise the object, get data from the owning CFatMountCB
*/
void CFatTable::InitializeL()
    {
    ASSERT(iOwner);

    //-- get FAT type from the owner
    iFatType = iOwner->FatType();
    ASSERT(IsFat12() || IsFat16() || IsFat32());

    //-- set the EOC code
    iFatEocCode = EocCodeByFatType(iFatType);
    



    iFreeClusterHint = KFatFirstSearchCluster;

    //-- cache the media attributes
    TLocalDriveCapsV2 caps;
    TPckg<TLocalDriveCapsV2> capsPckg(caps);
    User::LeaveIfError(iOwner->LocalDrive()->Caps(capsPckg));
    iMediaAtt = caps.iMediaAtt;

    //-- obtain maximal number of entries in the table
    iMaxEntries = iOwner->UsableClusters()+KFatFirstSearchCluster; //-- FAT[0] & FAT[1] are not in use

    __PRINT3(_L("CFatTable::InitializeL(), drv:%d, iMediaAtt = %08X, max Entries:%d"), iOwner->DriveNumber(), iMediaAtt, iMaxEntries);
    }

//-----------------------------------------------------------------------------

/** 
    Decrements the free cluster count.
    Note that can be quite expensive operation (especially for overrides with synchronisation), if it is called for every 
    cluster of a large file. Use more than one cluster granularity.
     
    @param  aCount a number of clusters 
*/
void CFatTable::DecrementFreeClusterCount(TUint32 aCount)
    {
    __ASSERT_DEBUG(iFreeClusters >= aCount, Fault(EFatCorrupt));
    iFreeClusters -= aCount;
    }

/** 
    Increments the free cluster count.
    Note that can be quite expensive operation (especially for overrides with synchronisation), if it is called for every 
    cluster of a large file. Use more than one cluster granularity.

    @param  aCount a number of clusters 
*/
void CFatTable::IncrementFreeClusterCount(TUint32 aCount)
    {
	const TUint32 newVal = iFreeClusters+aCount;
    __ASSERT_DEBUG(newVal<=MaxEntries(), Fault(EFatCorrupt));
    
    iFreeClusters = newVal;
    }

/** @return number of free clusters in the FAT */
TUint32 CFatTable::NumberOfFreeClusters(TBool /*aSyncOperation=EFalse*/) const
    {
    return FreeClusters();
    }

void CFatTable::SetFreeClusters(TUint32 aFreeClusters)
    {   
    iFreeClusters=aFreeClusters;
    }

/**
    Get the hint about the last known free cluster number.
    Note that can be quite expensive operation (especially for overrides with synchronisation), if it is called for every 
    cluster of a large file.

    @return cluster number supposedly close to the free one.
*/
TUint32 CFatTable::FreeClusterHint() const 
    {
    ASSERT(ClusterNumberValid(iFreeClusterHint));
    return iFreeClusterHint;
    } 

/**
    Set a free cluster hint. The next search fro the free cluster can start from this value.
    aCluster doesn't have to be a precise number of free FAT entry; it just needs to be as close as possible to the 
    free entries chain.
    Note that can be quite expensive operation (especially for overrides with synchronisation), if it is called for every 
    cluster of a large file.

    @param aCluster cluster number hint.
*/
void CFatTable::SetFreeClusterHint(TUint32 aCluster) 
    {
    ASSERT(ClusterNumberValid(aCluster));
    iFreeClusterHint=aCluster;
    } 

//-----------------------------------------------------------------------------

/**
    Find out the number of free clusters on the volume.
    Reads whole FAT and counts free clusters.
*/
void CFatTable::CountFreeClustersL()
    {
    __PRINT1(_L("#- CFatTable::CountFreeClustersL(), drv:%d"), iOwner->DriveNumber());

    const TUint32 KUsableClusters = iOwner->UsableClusters();
    (void)KUsableClusters;

    TUint32 freeClusters = 0;
    TUint32 firstFreeCluster = 0;

    TTime   timeStart;
    TTime   timeEnd;
    timeStart.UniversalTime(); //-- take start time

    //-- walk through whole FAT table looking for free clusters
	for(TUint i=KFatFirstSearchCluster; i<MaxEntries(); ++i)
    {
	    if(ReadL(i) == KSpareCluster)
            {//-- found a free cluster
		    ++freeClusters;
            
            if(!firstFreeCluster)
                firstFreeCluster = i;
            }
	    }

    timeEnd.UniversalTime(); //-- take end time
    const TInt msScanTime = (TInt)( (timeEnd.MicroSecondsFrom(timeStart)).Int64() / K1mSec);
    __PRINT1(_L("#- CFatTable::CountFreeClustersL() finished. Taken:%d ms"), msScanTime);
    (void)msScanTime;

    if(!firstFreeCluster) //-- haven't found free clusters on the volume
        firstFreeCluster = KFatFirstSearchCluster;

    ASSERT(freeClusters <= KUsableClusters);

    SetFreeClusters(freeClusters);
    SetFreeClusterHint(firstFreeCluster);
    }

//-----------------------------------------------------------------------------

/**
    Count the number of continuous cluster from a start cluster in FAT table.

@param aStartCluster cluster to start counting from
    @param  aEndCluster     contains the end cluster number upon return
@param aMaxCount Maximum cluster required
    @return Number of continuous clusters from aStartCluster.
*/
TUint32 CFatTable::CountContiguousClustersL(TUint32 aStartCluster, TUint32& aEndCluster, TUint32 aMaxCount) const
	{
	__PRINT2(_L("CFatTable::CountContiguousClustersL() start:%d, max:%d"),aStartCluster, aMaxCount);

    ASSERT(ClusterNumberValid(aStartCluster));

    TUint32 currClusterNo = aStartCluster;
    
    TUint32 clusterListLen;
    for(clusterListLen=1; clusterListLen < aMaxCount; ++clusterListLen)
        {
        TUint32 nextClusterNo = currClusterNo;

        if(!GetNextClusterL(nextClusterNo))
            break; //-- end of cluster chain
            
        if(nextClusterNo != currClusterNo+1)
            break; //-- not the next cluster
      
        currClusterNo = nextClusterNo;
        }

    aEndCluster = aStartCluster+clusterListLen-1;

    return clusterListLen;
	}	
//-----------------------------------------------------------------------------

/**
    Extend a file or directory cluster chain, leaves if there are no free clusters (the disk is full).

    @param aNumber  amount of clusters to allocate
    @param aCluster FAT entry index to start with.

    @leave KErrDiskFull + system wide error codes
*/
void CFatTable::ExtendClusterListL(TUint32 aNumber, TUint32& aCluster)
	{
	__PRINT2(_L("CFatTable::ExtendClusterListL() num:%d, clust:%d"), aNumber, aCluster);
	__ASSERT_DEBUG(aNumber>0,Fault(EFatBadParameter));
	
	while(aNumber && GetNextClusterL(aCluster))
		aNumber--;

    if(!aNumber)
        return;

    if(!RequestFreeClusters(aNumber))
		{
		__PRINT(_L("CFatTable::ExtendClusterListL - leaving KErrDirFull"));
		User::Leave(KErrDiskFull);
		}


    TUint32 freeCluster = 0;
    
    //-- note: this can be impoved by trying to fing as long chain of free clusters as possible in FindClosestFreeClusterL()
    for(TUint i=0; i<aNumber; ++i)
        {
        freeCluster = FindClosestFreeClusterL(aCluster);
        WriteFatEntryEofL(freeCluster); //	Must write EOF for FindClosestFreeCluster to work again
        WriteL(aCluster,freeCluster);
        aCluster=freeCluster;
        }
    
    //-- decrement number of available clusters
    DecrementFreeClusterCount(aNumber);

    //-- update free cluster hint, it isn't required to be a precise value, just a hint where to start the from from
    SetFreeClusterHint(aCluster); 
    
    }

//-----------------------------------------------------------------------------

/**
    Allocate and mark as EOF a single cluster as close as possible to aNearestCluster

    @param aNearestCluster Cluster the new cluster should be nearest to
    @leave System wide error codes
    @return The cluster number allocated
*/
TUint32 CFatTable::AllocateSingleClusterL(TUint32 aNearestCluster)
	{
	__PRINT1(_L("CFatTable::AllocateSingleCluster() nearest:%d"), aNearestCluster);
	
    const TInt freeCluster=FindClosestFreeClusterL(aNearestCluster);
	WriteFatEntryEofL(freeCluster);
	DecrementFreeClusterCount(1);

    //-- update free cluster hint, it isn't required to be a precise value, just a hint where to start the from from.
    SetFreeClusterHint(freeCluster); 

	return(freeCluster);
	}	

//-----------------------------------------------------------------------------

/**
    Allocate and link a cluster chain, leaves if there are not enough free clusters.
    Chain starts as close as possible to aNearestCluster, last cluster will be marked as EOF.

    @param aNumber Number of clusters to allocate
    @param aNearestCluster Cluster the new chain should be nearest to
    @leave System wide error codes
    @return The first cluster number allocated
*/
TUint32 CFatTable::AllocateClusterListL(TUint32 aNumber, TUint32 aNearestCluster)
	{
    __PRINT2(_L("CFatTable::AllocateClusterList() N:%d,NearestCL:%d"),aNumber,aNearestCluster);
	__ASSERT_DEBUG(aNumber>0, Fault(EFatBadParameter));

	if(!RequestFreeClusters(aNumber))
    	{
		__PRINT(_L("CFatTable::AllocateClusterListL - leaving KErrDirFull"));
		User::Leave(KErrDiskFull);
		}

	TUint32 firstCluster = aNearestCluster = AllocateSingleClusterL(aNearestCluster);
	if (aNumber>1)
		ExtendClusterListL(aNumber-1, aNearestCluster);

    return(firstCluster);
	}	

//-----------------------------------------------------------------------------

/**
    Notify the media drive about media areas that shall be treated as "deleted" if this feature is supported.
    @param aFreedClusters array with FAT numbers of clusters that shall be marked as "deleted"
*/
void CFatTable::DoFreedClustersNotifyL(RClusterArray &aFreedClusters)
{
    ASSERT(iMediaAtt & KMediaAttDeleteNotify);

    const TUint clusterCount = aFreedClusters.Count();

    if(!clusterCount)
        return;
    
    FlushL(); //-- Commit the FAT changes to disk first to be safe

    const TUint bytesPerCluster = 1 << iOwner->ClusterSizeLog2();

    TInt64  byteAddress = 0;	
	TUint   deleteLen = 0;	// zero indicates no clusters accumulated yet

	for (TUint i=0; i<clusterCount; ++i)
	{
        const TUint currCluster = aFreedClusters[i];
        
        if (deleteLen == 0)
		    byteAddress = DataPositionInBytesL(currCluster); //-- start of the media range
        
        deleteLen += bytesPerCluster;

        //-- if this is the last entry in the array or the net cluster number is not consecutive, notify the driver
		if ((i+1) == clusterCount || aFreedClusters[i+1] != (currCluster+1))
        {
            //__PRINT3(_L("DeleteNotify(%08X:%08X, %u), first cluster %u last cluster #%u"), I64HIGH(byteAddress), I64LOW(byteAddress), deleteLen);
			//__PRINT2(_L("   first cluster %u last cluster #%u"), I64LOW((byteAddress - iOwner->ClusterBasePosition()) >> iOwner->ClusterSizeLog2()) + 2, cluster);
	
            const TInt r = iOwner->LocalDrive()->DeleteNotify(byteAddress, deleteLen);
			if(r != KErrNone)
                {//-- if DeleteNotify() failed, it means that something terribly wrong happened to the NAND media; 
                 //-- in normal circumstances it can not happen. One of the reasons: totally worn out media.
                const TBool platSecEnabled = PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement);
                __PRINT3(_L("CFatTable::DoFreedClustersNotifyL() DeleteNotify failure! drv:%d err:%d, PlatSec:%d"),iOwner->DriveNumber(), r, platSecEnabled);

                if(platSecEnabled)
                    {
                    //-- if PlatSec is enabled, we can't afford jeopardize the security; without DeleteNotify()
                    //-- it's possible to pick up data from deleted files, so, panic the file server.
                    Fault(EFatBadLocalDrive);
                    }
                else
                    {
                    //-- if PlatSec is disabled, it's OK to ignore the NAND fault in release mode.
                    __ASSERT_DEBUG(0, Fault(EFatBadLocalDrive));
                    }        
                }

            
            deleteLen = 0;
        }

    }

    //-- empty the array.
    aFreedClusters.Reset();
}

//-----------------------------------------------------------------------------
/**
    Mark a chain of clusters as free in the FAT. 

    @param aCluster Start cluster of cluster chain to free
    @leave System wide error codes
*/
void CFatTable::FreeClusterListL(TUint32 aCluster)
	{
	__PRINT1(_L("CFatTable::FreeClusterListL startCluster=%d"),aCluster);
	if (aCluster == KSpareCluster)
		return; 

	//-- here we can store array of freed cluster numbers in order to 
    //-- notify media drive about the media addresses marked as "invalid"
    RClusterArray deletedClusters;      
	CleanupClosePushL(deletedClusters);

    //-- if ETrue, we need to notify media driver about invalidated media addressses
    const TBool bFreeClustersNotify = iMediaAtt & KMediaAttDeleteNotify;

    //-- this is a maximal number of FAT entries in the deletedClusters array.
    //-- as soon as we collect this number of entries in the array, FAT cache will be flushed
    //-- and driver notified. The array will be emptied. Used to avoid huge array when deleting
    //--  large files on NAND media 
    const TUint KSubListLen = 4096;
    ASSERT(IsPowerOf2(KSubListLen));

    TUint32 lastKnownFreeCluster = FreeClusterHint();
    TUint32 cntFreedClusters = 0;

    TUint32 currCluster = aCluster;
    TUint32 nextCluster = aCluster;

    for(;;)
    {
        const TBool bEOF = !GetNextClusterL(nextCluster);    
        WriteL(currCluster, KSpareCluster);

        lastKnownFreeCluster = Min(currCluster, lastKnownFreeCluster);

		// Keep a record of the deleted clusters so that we can subsequently notify the media driver. This is only safe 
		// to do once the FAT changes have been written to disk.
        if(bFreeClustersNotify)
            deletedClusters.Append(currCluster);

        ++cntFreedClusters;
        currCluster = nextCluster;

		if (bEOF || aCluster == KSpareCluster)
			break;

        if(bFreeClustersNotify && cntFreedClusters && (cntFreedClusters & (KSubListLen-1))==0)
        {//-- reached a limit of the entries in the array. Flush FAT cache, notify the driver and empty the array.
            IncrementFreeClusterCount(cntFreedClusters);
            cntFreedClusters = 0;

            SetFreeClusterHint(lastKnownFreeCluster);
            DoFreedClustersNotifyL(deletedClusters);
        }

    }

    //-- increase the number of free clusters and notify the driver if required.
    IncrementFreeClusterCount(cntFreedClusters);
    SetFreeClusterHint(lastKnownFreeCluster);
    
    if(bFreeClustersNotify)
        DoFreedClustersNotifyL(deletedClusters);

	CleanupStack::PopAndDestroy(&deletedClusters);
	}

//-----------------------------------------------------------------------------

/**
    Find a free cluster nearest to aCluster, Always checks to the right of aCluster first 
    but checks in both directions in the Fat.

    @param aCluster Cluster to find nearest free cluster to.
    @leave KErrDiskFull + system wide error codes
    @return cluster number found
*/
TUint32 CFatTable::FindClosestFreeClusterL(TUint32 aCluster)
	{
    __PRINT2(_L("CFatTable::FindClosestFreeClusterL() drv:%d cl:%d"),iOwner->DriveNumber(),aCluster);
	
    if(!ClusterNumberValid(aCluster))
        {
        ASSERT(0);
        User::Leave(KErrCorrupt);
        }

    if(!RequestFreeClusters(1))
	    {//-- there is no at least 1 free cluster available
    	__PRINT(_L("CFatTable::FindClosestFreeClusterL() leaving KErrDiskFull #1"));
        User::Leave(KErrDiskFull);
        }

    //-- 1. look if the given index contains a free entry 
    if(ReadL(aCluster) != KSpareCluster)
        {//-- no, it doesn't...
        
        //-- 2. look in both directions starting from the aCluster, looking in the right direction first
        
        const TUint32 maxEntries = MaxEntries();
        const TUint32 MinIdx = KFatFirstSearchCluster;
        const TUint32 MaxIdx = maxEntries-1;

        TBool canGoRight = ETrue;
        TBool canGoLeft = ETrue;
    
        TUint32 rightIdx = aCluster;
        TUint32 leftIdx  = aCluster;
        
        for(TUint i=0; i<maxEntries; ++i)
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
	            {
    	        __PRINT(_L("CFatTable::FindClosestFreeClusterL() leaving KErrDiskFull #2"));
                User::Leave(KErrDiskFull);
                }

            if(canGoRight && ReadL(rightIdx) == KSpareCluster)
			    {
			    aCluster = rightIdx;
			    break;
			    }

		    if (canGoLeft && ReadL(leftIdx) == KSpareCluster)
			    {
			    aCluster = leftIdx;
			    break;
			    }
            }//for(..)

        }//if(ReadL(aCluster) != KSpareCluster)


    //-- note: do not update free cluster hint here by calling SetFreeClusterHint(). This is going to be 
    //-- expensive especially if overridden methods with synchronisation are called. Instead, set the number of 
    //-- the last known free cluster in the caller of this internal method.

    //__PRINT1(_L("CFatTable::FindClosestFreeClusterL found:%d"),aCluster);

    return aCluster;
	}

//-----------------------------------------------------------------------------

/**
    Converts a cluster number to byte offset in the FAT

    @param aFatIndex Cluster number
    @return Number of bytes from the beginning of the FAT
*/
TUint32 CFatTable::PosInBytes(TUint32 aFatIndex) const
	{
    switch(FatType())
        {
        case EFat12:
            return (((aFatIndex>>1)<<1) + (aFatIndex>>1)); //-- 1.5 bytes per FAT entry

        case EFat16:
            return aFatIndex<<1; //-- 2 bytes per FAT entry

        case EFat32:
            return aFatIndex<<2; //-- 4 bytes per FAT entry

        default:
            break;
        };

    ASSERT(0);
    return 0;//-- get rid of warning
	}

//-----------------------------------------------------------------------------

/**
    Checks if we have at least aClustersRequired clusters free in the FAT.
    This is, actually a dummy implementation.

    @param  aClustersRequired number of free clusters required
    @return ETrue if there is at least aClustersRequired free clusters available, EFalse otherwise.
*/
TBool CFatTable::RequestFreeClusters(TUint32 aClustersRequired) const
    {
    //__PRINT1(_L("#- CFatTable::RequestFreeClusters(%d)"),aClustersRequired);
    ASSERT(aClustersRequired >0);
    return (NumberOfFreeClusters() >= aClustersRequired);
    }

    


//#######################################################################################################################################
//#     CAtaFatTable class implementation 
//#######################################################################################################################################

/**
    Constructor
*/
CAtaFatTable::CAtaFatTable(CFatMountCB& aOwner)
             :CFatTable(aOwner), iDriveInteface(aOwner.DriveInterface())
    {
    iState = ENotInitialised;
    }


CAtaFatTable::~CAtaFatTable()
    {
    DestroyHelperThread();
    }


/** factory method */
CAtaFatTable* CAtaFatTable::NewL(CFatMountCB& aOwner)
{
    __PRINT1(_L("CAtaFatTable::NewL() drv:%d"),aOwner.DriveNumber());
    CAtaFatTable* pSelf = new (ELeave) CAtaFatTable(aOwner);

    CleanupStack::PushL(pSelf);
    pSelf->InitializeL();
    CleanupStack::Pop();

    return pSelf;
}


//---------------------------------------------------------------------------------------------------------------------------------------

/**
    CAtaFatTable's FAT cache factory method.
    Creates fixed cache for FAT12/FAT16 or LRU cache for FAT32
*/
void CAtaFatTable::CreateCacheL()
{
    ASSERT(iOwner);
    const TUint32 fatSize=iOwner->FatSizeInBytes();
    __PRINT3(_L("CAtaFatTable::CreateCacheL drv:%d, FAT:%d, FAT Size:%d"), iOwner->DriveNumber(), FatType(), fatSize);
	

    //-- according to FAT specs:
    //-- FAT12 max size is 4084 entries or 6126 bytes                                               => create fixed cache for whole FAT
    //-- FAT16 min size is 4085 entries or 8170 bytes, max size is 65525 entries or 131048 bytes    => create fixed cache for whole FAT
    //-- FAT32 min size is 65526 entries or 262104 bytes                                            => create LRU paged cache of max size: KFat32LRUCacheSize

    ASSERT(!iCache);

    //-- this is used for chaches granularity sanity check 
    const TUint32 KMinGranularityLog2 = KDefSectorSzLog2;  //-- 512 bytes is a minimal allowed granularity
    const TUint32 KMaxGranularityLog2 = 18; //-- 256K is a maximal allowed granularity

    switch(FatType())
    {
        //-- create fixed FAT12 cache
        case EFat12: 
            iCache = CFat12Cache::NewL(iOwner, fatSize); 
        break;
    
        //-- create fixed FAT16 cache
        case EFat16: 
            {
            TUint32 fat16_ReadGranularity_Log2; //-- FAT16 cache read granularity Log2
            TUint32 fat16_WriteGranularity_Log2;//-- FAT16 cache write granularity Log2
            
            iOwner->FatConfig().Fat16FixedCacheParams(fat16_ReadGranularity_Log2, fat16_WriteGranularity_Log2);
            
            //-- check if granularity values look sensible
            const TBool bParamsValid = fat16_ReadGranularity_Log2  >= KMinGranularityLog2 && fat16_ReadGranularity_Log2  <= KMaxGranularityLog2 &&
                                       fat16_WriteGranularity_Log2 >= KMinGranularityLog2 && fat16_WriteGranularity_Log2 <= KMaxGranularityLog2;

             __ASSERT_ALWAYS(bParamsValid, Fault(EFatCache_BadGranularity)); 


            iCache = CFat16FixedCache::NewL(iOwner, fatSize, fat16_ReadGranularity_Log2, fat16_WriteGranularity_Log2); 
            }
        break;

        //-- create FAT32 LRU paged cache
        case EFat32: 
            {
            TUint32 fat32_LRUCache_MaxMemSize;  //-- Maximum memory for the LRU FAT32 cache
            TUint32 fat32_ReadGranularity_Log2; //-- FAT32 cache read granularity Log2
            TUint32 fat32_WriteGranularity_Log2;//-- FAT32 cache write granularity Log2
    
            iOwner->FatConfig().Fat32LruCacheParams(fat32_ReadGranularity_Log2, fat32_WriteGranularity_Log2, fat32_LRUCache_MaxMemSize);

            
            //-- check if granularity  and required cache size values look sensible
            const TBool bParamsValid = fat32_ReadGranularity_Log2  >= KMinGranularityLog2 && fat32_ReadGranularity_Log2  <= KMaxGranularityLog2 &&
                                       fat32_WriteGranularity_Log2 >= KMinGranularityLog2 && fat32_WriteGranularity_Log2 <= KMaxGranularityLog2 &&
                                       fat32_LRUCache_MaxMemSize >= 8*K1KiloByte && fat32_LRUCache_MaxMemSize < 4*K1MegaByte;
            
            __ASSERT_ALWAYS(bParamsValid, Fault(EFatCache_BadGranularity)); 
            
            iCache = CFat32LruCache::NewL(iOwner, fat32_LRUCache_MaxMemSize, fat32_ReadGranularity_Log2, fat32_WriteGranularity_Log2);
            }
        break;

        default:
        ASSERT(0);
        User::Leave(KErrCorrupt);
        break;
    };

    ASSERT(iCache);
}

//---------------------------------------------------------------------------------------------------------------------------------------

/**
    Destroys a helper thread object.
    If the thread is running, stops it first. than deletes the ipHelperThread and sets it to NULL
*/
void CAtaFatTable::DestroyHelperThread()
{

    if(!ipHelperThread)
        return;
  
    __PRINT1(_L("CAtaFatTable::DestroyHelperThread(), drv:%d"), iOwner->DriveNumber());
    ipHelperThread->ForceStop();
    delete ipHelperThread;
    ipHelperThread = NULL;
}

//---------------------------------------------------------------------------------------------------------------------------------------

/**
    Flush the FAT cache on disk
    @leave System wide error codes
*/
void CAtaFatTable::FlushL()
	{
    __PRINT1(_L("CAtaFatTable::FlushL(), drv:%d"), iOwner->DriveNumber());

    //-- the data can't be written if the mount is inconsistent
    iOwner->CheckStateConsistentL();

    if (iCache)
		iCache->FlushL();
	}


//---------------------------------------------------------------------------------------------------------------------------------------

/**
    Dismount the cache. Stops any activity, deallocates caches etc.
    @param aDiscardDirtyData if ETrue, non-flushed data in the cache will be discarded.
*/
void CAtaFatTable::Dismount(TBool aDiscardDirtyData)
	{
    __PRINT3(_L("#=-= CAtaFatTable::Dismount(%d), drv:%d, state:%d"), aDiscardDirtyData, iOwner->DriveNumber(), State());

    //-- if there is a helper thread, stop it and delete its object
    DestroyHelperThread();

    //-- if there is the cache, close it (it may lead to deallocating its memory)
	if(iCache)
		{
        //-- cache's Close() can check if the cache is clean. 
        //-- ignore dirty data in cache if the mount is not in consistent state (it's impossible to flush cache data)
        //-- or if we are asked to do so.
        const TBool bIgnoreDirtyData = aDiscardDirtyData || !iOwner->ConsistentState();
        iCache->Close(bIgnoreDirtyData);

        delete iCache;
		iCache=NULL;
		}

     SetState(EDismounted);
	}

//---------------------------------------------------------------------------------------------------------------------------------------

/**
    Invalidate whole FAT cache.
    Depending of cache type this may just mark cache invalid with reading on demand or re-read whole cache from the media
*/
void CAtaFatTable::InvalidateCacheL()
{
    __PRINT1(_L("CAtaFatTable::InvalidateCache(), drv:%d"), iOwner->DriveNumber());

    //-- if we have a cache, invalidate it entirely
    if(iCache)
        {
        User::LeaveIfError(iCache->Invalidate());
        }

    //-- invalidating whole FAT cache means that something very serious happened.
    //-- if we have a helper thread running, abort it.
    if(ipHelperThread)
        ipHelperThread->ForceStop();

}


//---------------------------------------------------------------------------------------------------------------------------------------

/**
    Invalidate specified region of the FAT cache
    Depending of cache type this may just mark part of the cache invalid with reading on demand later
    or re-read whole cache from the media.

    @param aPos absolute media position where the region being invalidated starts.
    @param aLength length in bytes of region to invalidate / refresh
*/
void CAtaFatTable::InvalidateCacheL(TInt64 aPos, TUint32 aLength)
	{
    __PRINT3(_L("CAtaFatTable::InvalidateCacheL() drv:%d, pos:%LU, len:%u,"), iOwner->DriveNumber(), aPos, aLength);

    if(I64HIGH(aPos) || !aLength || I64HIGH(aPos+aLength))
        return; //-- FAT tables can't span over 4G 

    const TUint32 mediaPos32 = I64LOW(aPos);

    //-- we do not use other copies of FAT, so trach changes only in FAT1
    const TUint32 fat1StartPos = iOwner->StartOfFatInBytes();
    const TUint32 fat1EndPos   = fat1StartPos + iOwner->FatSizeInBytes();

    TUint32 invRegionPosStart = 0; //-- media pos where the invalidated region starts
    TUint32 invRegionLen = 0;      //-- size of the invalidated region, bytes
    
    //-- calculate the FAT1 region being invalidated
    if(mediaPos32 < fat1StartPos)
        {
        if((mediaPos32 + aLength) <= fat1StartPos)
            return;

        invRegionPosStart = fat1StartPos;
        invRegionLen = aLength - (fat1StartPos-mediaPos32);
        }
    else //if(mediaPos32 < fat1StartPos)
        {//-- mediaPos32 >= fat1StartPos)
        if(mediaPos32 >= fat1EndPos)
            return;
    
        invRegionPosStart = mediaPos32;
        
        if((mediaPos32 + aLength) <= fat1EndPos)
            {
            invRegionLen = aLength;
            }
        else 
            {
            invRegionLen = mediaPos32+aLength-fat1EndPos;
            }
        }

    //-- convert the media pos of the region into FAT entries basis, depending on the FAT type
    ASSERT(invRegionPosStart >= fat1StartPos && invRegionLen <= (TUint)iOwner->FatSizeInBytes());
    
    TUint32 startFatEntry=0;
    TUint32 numEntries = 0;

    switch(FatType())
        {
        case EFat12:
        //-- invalidate whole cache; it is not worth making calculations for such small memory region.
        User::LeaveIfError(iCache->Invalidate());
        return;

        case EFat16:
        startFatEntry = (invRegionPosStart-fat1StartPos) >> KFat16EntrySzLog2;
        numEntries = (invRegionLen + (sizeof(TFat16Entry)-1)) >> KFat16EntrySzLog2;
        break;

        case EFat32:
        startFatEntry = (invRegionPosStart-fat1StartPos) >> KFat32EntrySzLog2;
        numEntries = (invRegionLen + (sizeof(TFat32Entry)-1)) >> KFat32EntrySzLog2;
        break;

        default:
        ASSERT(0);
        return;
        };

    if(startFatEntry < KFatFirstSearchCluster)
        {//-- FAT[0] and FAT[1] can't be legally accessed, they are reserved entries. We need to adjust region being refreshed.
        if(numEntries <= KFatFirstSearchCluster)
            return; //-- nothing to refresh
                    
        startFatEntry += KFatFirstSearchCluster;
        numEntries -= KFatFirstSearchCluster;
        }

    User::LeaveIfError(iCache->InvalidateRegion(startFatEntry, numEntries));
	}


//-----------------------------------------------------------------------------
/**
    Initialize the object, create FAT cache if required
    @leave KErrNoMemory
*/
void CAtaFatTable::InitializeL()
	{
    __PRINT2(_L("CAtaFatTable::InitializeL() drv:%d, state%d"), iOwner->DriveNumber(), State());
    CFatTable::InitializeL();

    ASSERT(!iCache);
    ASSERT(State() == ENotInitialised);
    
    //-- create the FAT cache.
    CreateCacheL();

    SetState(EInitialised);

	}

//-----------------------------------------------------------------------------
/**
    Mount the FAT table to the CFatMountCB. Depending on mount parameters and configuration this method 
    can do various things, like counting free clusters synchronously if data from FSInfo isn't valid, 
    or setting up a FAT backround thread and return immediately etc.

    @param  aMountParam mounting parameters, like some data from FSInfo

*/
void CAtaFatTable::MountL(const TMountParams& aMountParam)
    {
    __PRINT2(_L("CAtaFatTable::MountL() drv:%d, state:%d"), iOwner->DriveNumber(), State());

    ASSERT(State() == EInitialised);
    SetState(EMounting);

    if(ipHelperThread)
        {   
        __PRINT(_L("CAtaFatTable::MountL() Helper thread is present!"));
        ASSERT(0);
        DestroyHelperThread();
        }
    

    //-- Check if we have valid data from FSInfo. In this case we don't need to count free clusters
    if(aMountParam.iFsInfoValid)
        {
        ASSERT(IsFat32());
        ASSERT(aMountParam.iFreeClusters <= MaxEntries());
        
        ASSERT(ClusterNumberValid(aMountParam.iFirstFreeCluster));

        SetFreeClusters(aMountParam.iFreeClusters);
        SetFreeClusterHint(aMountParam.iFirstFreeCluster);

        __PRINT2(_L("CAtaFatTable::MountL() Using data from FSInfo sector. free clusters:%d, 1st free:%d"), FreeClusters(), FreeClusterHint());

        //-- We don't need to scan entire FAT to find out the number of free entries, because the data are taken from FSInfo.
        //-- But if we are going to use the FAT32 bit supercache, we need to populate it. So, try to start up a special 
        //-- populating thread.
        CFatBitCache *pFatBitCache = iCache->BitCacheInterface();
        if(pFatBitCache)
            {//-- bit cache is present, we need to populate (or repopulate it)
            //-- create helper thread object and start the thread
            ipHelperThread = CFat32BitCachePopulator::NewL(*this);

            if(ipHelperThread->Launch() != KErrNone)
                {//-- failed for some reason
                DestroyHelperThread();
                }
                else
                {
                //-- background FAT bit cache populating thread is running now.
                //-- the result of thread start up and completion isn't very interesting: If it fails to 
                //-- properly populate the cache, nothing fatal will happen.
                }
            }

        //-- CFat32BitCachePopulator doesn't affect FAT table state. 
        SetState(EMounted);
        return; 
        }

    //-- FSInfo data are invalid; we need to count free clusters by reading whole FAT table
    //-- This method can optionally create a background thread (that will count free clusters) and return immideately.
    CountFreeClustersL();
    }

//-----------------------------------------------------------------------------

/**
    Decrements the free cluster count. This is an overridden method with synchronisation.
    @param  aCount a number of clusters 
*/
void CAtaFatTable::DecrementFreeClusterCount(TUint32 aCount)
    {
    XAutoLock lock(iOwner); //-- enter critical section
    CFatTable::DecrementFreeClusterCount(aCount);
    }

/**
    Increments the free cluster count.  This is an overridden method with synchronisation.
    @param  aCount a number of clusters 
*/
void CAtaFatTable::IncrementFreeClusterCount(TUint32 aCount)
    {
    XAutoLock lock(iOwner); //-- enter critical section
    CFatTable::IncrementFreeClusterCount(aCount);
    }

//-----------------------------------------------------------------------------

/**
    Obtain number of free clusters on the volume. This is an overridden method.
    Depending on the "aSyncOperation" parameter this operation can be fully synhronous (exact number of free clusters ) or asynchronous
    (current number of free clusters) if the FAT scanning thread is still running.

    @param aSyncOperation if ETrue, this method will wait until FAT scan thread finishes and return exact number of free clusters
                          if false, it will return current number of free clusters counted by FAT scan thread if it hasn't finished yet.  
    
    @return Number of free clusters. See also CAtaFatTable::RequestFreeClusters()
*/
TUint32 CAtaFatTable::NumberOfFreeClusters(TBool aSyncOperation/*=EFalse*/) const
    {
    if(ipHelperThread && ipHelperThread->ThreadWorking() && ipHelperThread->Type() == CFatHelperThreadBase::EFreeSpaceScanner)
        {//-- here we have running helper thread that counts free entries in FAT.
        //-- if this operation is synchronous, we need to wait until it finish its job in order to get _exact_ number of free cluster,
        //-- not currently counted  
        
        //__PRINT2(_L("#- CAtaFatTable::NumberOfFreeClusters(), drv:%d enter, sync:%d"), iOwner->DriveNumber(), aSyncOperation);

        if(aSyncOperation)
            {//-- wait for background scanning thread to finish counting free clusters if this operation is synchronous
            ipHelperThread->BoostPriority(ETrue);
            ipHelperThread->WaitToFinish();
            }
        
        XAutoLock lock(iOwner); //-- enter critical section
        
        const TUint32 freeClusters = FreeClusters();
        //__PRINT2(_L("#- CAtaFatTable::NumberOfFreeClusters(), drv:%d Exit, clusters:%d"), iOwner->DriveNumber(), freeClusters);
        return freeClusters;
        }

    return FreeClusters();

    }

//-----------------------------------------------------------------------------

/** 
    Set free cluster count. This is an overridden method with synchronisation. 
    @param aFreeClusters new value of free clusters
*/
void CAtaFatTable::SetFreeClusters(TUint32 aFreeClusters)
    {   
    XAutoLock lock(iOwner); //-- enter critical section
    CFatTable::SetFreeClusters(aFreeClusters);
    }

/** 
    This is an overridden method with synchronisation. 
    @return the last known free cluster number.
*/
TUint32 CAtaFatTable::FreeClusterHint() const 
    {
    XAutoLock lock(iOwner); //-- enter critical section
    return CFatTable::FreeClusterHint();
    } 

/** Set next free cluster number. This is an overridden method with synchronisation. */
void CAtaFatTable::SetFreeClusterHint(TUint32 aCluster) 
    {
    XAutoLock lock(iOwner); //-- enter critical section
    CFatTable::SetFreeClusterHint(aCluster);
    } 

/**
    @return ETrue if the state of the object is consistent; i.e. it is 
    fully constructed, valid and the amount of free entries is known.
    Used in the case of asynchronous mounting.
*/
TBool CAtaFatTable::ConsistentState() const
    {
    return State() == EMounted;
    }

//-----------------------------------------------------------------------------

/**
    Request for the raw write access to the FAT area (all copies of FAT).
    If FAT helper thread is running, waits until it finishes.

    @param  aPos absolute media position we are going to write to. Be careful with casting it from TInt64 and losing high word.
    @param  aLen length of the area being written
*/
void CAtaFatTable::RequestRawWriteAccess(TInt64 aPos, TUint32 aLen) const
    {
    if(I64HIGH(aPos))
        return;

    const TUint32 pos32 = I64LOW(aPos);
    const TUint32 posFatStart = iOwner->StartOfFatInBytes(); //-- position of the FAT start on the volume
    const TUint32 posFatsEnd  = posFatStart + iOwner->NumberOfFats()*iOwner->FatSizeInBytes();  //-- position of the ent of ALL FATs

    if(pos32 >= posFatsEnd || (pos32+aLen) <= posFatStart)
        return;

    __PRINT2(_L("#=- CAtaFatTable::RequestRawWriteAccess() pos:%d, len:%d"),pos32, aLen);

    //-- someone tries to write to FAT area directly. Wait for the FAT helper thread to finish
    if(ipHelperThread)
        ipHelperThread->WaitToFinish();     

    }

//-----------------------------------------------------------------------------

/**
    Checks if we have at least "aClustersRequired" clusters free in the FAT.
    If FAT scannng thread is running, waits until requested number of free clusters counted or the thread finishes.

    @param  aClustersRequired number of free clusters required
    @return ETrue if there is at least aClustersRequired free clusters available, EFalse otherwise.
*/
TBool CAtaFatTable::RequestFreeClusters(TUint32 aClustersRequired) const
    {
    //__PRINT1(_L("#- CAtaFatTable::RequestFreeClusters(%d)"),aClustersRequired);
    ASSERT(aClustersRequired >0);

    if(!ipHelperThread || !ipHelperThread->ThreadWorking() || ipHelperThread->Type() != CFatHelperThreadBase::EFreeSpaceScanner)
        {//-- there is no FAT free space scan thread running, number of free entries can't increase in background
        return (FreeClusters() >= aClustersRequired); //-- use simple, non-thread safe method
        }     

    //-- FAT free space scan thread is running, counting free FAT entries. wait until it has counted enough or finish.
    ASSERT(ipHelperThread->Type() == CFatHelperThreadBase::EFreeSpaceScanner);
    
    TUint32 currFreeClusters;
    const TUint KWaitGranularity = 20*K1mSec; //-- wait granularity
    
    ipHelperThread->BoostPriority(ETrue); //-- increase thread priority

    for(;;)
        {
        currFreeClusters = NumberOfFreeClusters(EFalse); //-- get _current_ number of free clusters asynchronously
        if(currFreeClusters >= aClustersRequired)
            break; //-- OK, the request is satisfied

        if(!ipHelperThread->ThreadWorking())
            {//-- the thread has finished its work
            currFreeClusters = NumberOfFreeClusters(EFalse); //-- get _current_ number of free clusters asynchronously
            break; 
            }

        User::After(KWaitGranularity); //-- wait some time allowing FAT scanning thread to count free clusters.     
        }

    ipHelperThread->BoostPriority(EFalse); //-- set thread priority back to normal
    //__PRINT1(_L("#- CAtaFatTable::RequestFreeClusters() #2 curr:%d"),currFreeClusters);
    
    return (currFreeClusters >= aClustersRequired);

    }

//-----------------------------------------------------------------------------

/**
    Parse a buffer filled with FAT16 or FAT32 entries, counting free clusters and looking for the firs free cluster number.
    Note that this method can be called from a helper FAT scan thread.

    @param  aBuf        FAT buffer descriptor. Must contain whole number of FAT16 or FAT32 entries
    @param aScanParam   the structure to be filled with values, like number of counted free and non-free clusters, etc.
*/
void CAtaFatTable::DoParseFatBuf(const TPtrC8& aBuf, TFatScanParam& aScanParam) const
    {
    
    if(IsFat16())
        {//-- we are processing a buffer of FAT16 entries
        ASSERT(!ipHelperThread);
        ASSERT((aBuf.Size() & (sizeof(TFat16Entry)-1)) == 0);
        const TInt KNumEntries = aBuf.Size() >> KFat16EntrySzLog2; 
        const TFat16Entry* const pFatEntry = (const TFat16Entry*)(aBuf.Ptr()); 

        for(TInt i=0; i<KNumEntries; ++i)
            {
            if(aScanParam.iEntriesScanned >= KFatFirstSearchCluster)
                {
                const TFat16Entry entry = pFatEntry[i];

                if(entry == KSpareCluster)
                    {//-- found a free FAT entry
                    aScanParam.iCurrFreeEntries++;
                    
                    if(aScanParam.iFirstFree < KFatFirstSearchCluster)
                        aScanParam.iFirstFree = aScanParam.iEntriesScanned;    

                    }
                 else
                    {//-- found occupied FAT entry, count bad clusters as well 
                    aScanParam.iCurrOccupiedEntries++;
                    }

                }

            aScanParam.iEntriesScanned++;
            }
        }//if(IsFat16())
    else
    if(IsFat32())
        {//-- we are processing a buffer of FAT32 entries.
         //-- note that here we can be in the context of the FAT free entries scan thread.   
        ASSERT((aBuf.Size() & (sizeof(TFat32Entry)-1)) == 0);
        
        //-- pointer to the FAT32 bit supercache. If present, we will populate it here
        CFatBitCache *pFatBitCache = iCache->BitCacheInterface();

        const TInt KNumEntries = aBuf.Size() >> KFat32EntrySzLog2;
        const TFat32Entry* const pFatEntry = (const TFat32Entry*)(aBuf.Ptr()); 

        for(TInt i=0; i<KNumEntries; ++i)
            {
              if(aScanParam.iEntriesScanned >= KFatFirstSearchCluster)
                {
                const TFat32Entry entry = pFatEntry[i] & KFat32EntryMask;

                if(entry == KSpareCluster)
                    {//-- found a free FAT32 entry
                    ++aScanParam.iCurrFreeEntries;
                    
                    if(aScanParam.iFirstFree < KFatFirstSearchCluster)
                        aScanParam.iFirstFree = aScanParam.iEntriesScanned;    

                    
                    //-- feed the information about free FAT entry at index aClustersScanned to the FAT bit supercache. 
                    if(pFatBitCache)
                        {
                        pFatBitCache->SetFreeFatEntry(aScanParam.iEntriesScanned);
                        }

                    
                    }//if(entry == KSpareCluster)
                    else
                        {//-- found occupied FAT32 entry, count bad clusters as well
                        aScanParam.iCurrOccupiedEntries++;
                        }
                }

            ++aScanParam.iEntriesScanned;
            }

        }//if(IsFat32())
    else
        {
        ASSERT(0);
        }
    }

//-----------------------------------------------------------------------------

/**
    Count free clusters in FAT16 or FAT32. Uses relatively large buffer to read FAT entries into; 
    This is faster than usual ReadL() calls.
*/
void CAtaFatTable::DoCountFreeClustersL()
    {
    __PRINT2(_L("#- CAtaFatTable::DoCountFreeClustersL() drv:%d, state:%d"), iOwner->DriveNumber(), State());
    
    if(!IsFat16() && !IsFat32())
        {
        ASSERT(0);
        User::Leave(KErrNotSupported);
        }

    const TUint32 KFat1StartPos = iOwner->StartOfFatInBytes();
    const TUint32 KNumClusters  = MaxEntries(); //-- FAT[0] & FAT[1] are reserved and not counted by UsableClusters()
    const TUint32 KNumFATs      = iOwner->NumberOfFats();
    const TUint32 KFatSize      = KNumClusters * (IsFat32() ? sizeof(TFat32Entry) : sizeof(TFat16Entry)); //-- usable size of one FAT.

    (void)KNumFATs;

    ASSERT(KFat1StartPos >= 1*KDefaultSectorSize);
    ASSERT(KNumClusters > KFatFirstSearchCluster);
    ASSERT(KNumFATs > 0);

    const TUint32 KFatBufSz = 32*K1KiloByte; //-- buffer size for FAT reading. 32K seems to be optimal size

    __ASSERT_COMPILE((KFatBufSz % sizeof(TFat32Entry)) == 0);
    __ASSERT_COMPILE((KFatBufSz % sizeof(TFat16Entry)) == 0);   

    RBuf8 buf;
    CleanupClosePushL(buf);

    //-- allocate memory for FAT parse buffer
    buf.CreateMaxL(KFatBufSz);

    //-- read FAT into the large buffer and parse it
    TUint32 rem = KFatSize;
    TUint32 mediaPos = KFat1StartPos;   
        
    //-- prepare FAT bit supercache to being populated.
    //-- actual populating will happen in ::DoParseFatBuf()
    CFatBitCache *pFatBitCache = iCache->BitCacheInterface();

    if(pFatBitCache)
        {
        pFatBitCache->StartPopulating();
        }

    TFatScanParam fatScanParam;

    //-- used for measuring time
    TTime   timeStart;
    TTime   timeEnd;
    timeStart.UniversalTime(); //-- take start time


    while(rem)
        {
        const TUint32 bytesToRead=Min(rem, KFatBufSz);
        TPtrC8 ptrData(buf.Ptr(), bytesToRead);
        
        //__PRINT2(_L("#=--- CAtaFatTable::DoCountFreeClustersL() read %d bytes pos:0x%x"), bytesToRead, (TUint32)mediaPos);
        User::LeaveIfError(iOwner->LocalDrive()->Read(mediaPos, bytesToRead, buf)); 
        
        DoParseFatBuf(ptrData, fatScanParam);

        mediaPos += bytesToRead;
        rem -= bytesToRead;
        }

    //-- here fatScanParam contains values for the whole FAT. 
    
    timeEnd.UniversalTime(); //-- take end time
    const TInt msScanTime = (TInt)( (timeEnd.MicroSecondsFrom(timeStart)).Int64() / K1mSec);
    (void)msScanTime;
    __PRINT1(_L("#- CAtaFatTable::DoCountFreeClustersL() finished. Taken:%d ms "), msScanTime);
    

    //-- tell FAT bit cache that we have finished populating it
    if(pFatBitCache)
        {
        pFatBitCache->FinishPopulating(ETrue);
        pFatBitCache->Dump();
        }

    if(!fatScanParam.iFirstFree)//-- haven't found free clusters on the volume
        fatScanParam.iFirstFree = KFatFirstSearchCluster;

    ASSERT(fatScanParam.iCurrFreeEntries <= iOwner->UsableClusters());
    ASSERT(ClusterNumberValid(fatScanParam.iFirstFree));
    
    SetFreeClusters(fatScanParam.iCurrFreeEntries);
    SetFreeClusterHint(fatScanParam.iFirstFree);

    CleanupStack::PopAndDestroy(&buf); 
    }

//-----------------------------------------------------------------------------

/**
    Count free clusters on the volume.
    Depending on FAT type can count clusters synchronously or start a thread to do it in background.
*/
void CAtaFatTable::CountFreeClustersL()
    {
    __PRINT3(_L("#=- CAtaFatTable::CountFreeClustersL() drv:%d, FAT%d, state:%d"),iOwner->DriveNumber(),FatType(), State());
    
    ASSERT(State() == EMounting);
    ASSERT(!ipHelperThread);

    TInt nRes;

    switch(FatType())
        {
        case EFat12: //-- use old default scanning, it is synchronous
        CFatTable::CountFreeClustersL();
        SetState(EMounted);
        break;
           
        case EFat16: //-- enhanced FAT scan, but still synchronous
            TRAP(nRes, DoCountFreeClustersL());
            if(nRes !=KErrNone) 
                {
                CFatTable::CountFreeClustersL(); //-- fall back to the legacy method
                }

            SetState(EMounted);
        break;
   
        case EFat32: //-- This is FAT32, try to set up a FAT scanning thread if allowed
            {
                TBool bFat32BkGndScan = ETrue; //-- if true, we will try to start up a background scanning thread.

                //-- 1. check if background FAT scanning is disabled in config
                if(!iOwner->FatConfig().FAT32_AsynchMount())
                {
                    __PRINT(_L("#=- FAT32 BkGnd scan is disabled in config."));
                    bFat32BkGndScan = EFalse;
                }

                //-- 2. check if background FAT scanning is disabled by test interface
#ifdef _DEBUG
                TInt nMntDebugFlags;
                if(bFat32BkGndScan && RProperty::Get(KSID_Test1, iOwner->DriveNumber(), nMntDebugFlags) == KErrNone)
                {//-- test property for this drive is defined
                    if(nMntDebugFlags & KMntDisable_FatBkGndScan)
                    {
                    __PRINT(_L("#- FAT32 BkGnd scan is disabled by debug interface."));
                    bFat32BkGndScan = EFalse;
                    }
            
                }
#endif
                //-- 3. try to start FAT32 free entries scanning thread.
                if(bFat32BkGndScan)
                {
                    __PRINT(_L("#=- Starting up FAT32 free entries scanner thread..."));
                    TRAP(nRes, DoLaunchFat32FreeSpaceScanThreadL());
                    if(nRes == KErrNone) 
                        break; //-- let thread run by itself

                    //-- DoLaunchFat32FreeSpaceScanThreadL() has set this object state.
                }

            //-- we either failed to launch the thread or this feature was disabled somehow. Fall back to the synchronous scan.
            TRAP(nRes, DoCountFreeClustersL());
            if(nRes !=KErrNone) 
                {
                CFatTable::CountFreeClustersL(); //-- fall back to the legacy method
                }

             SetState(EMounted);
            }//case EFat32
        break;
   
        default:
            ASSERT(0);
        break;

        } //switch(FatType())
    }

//-----------------------------------------------------------------------------

/** 
    Set up and start FAT scan thread.
    Leaves on error.
*/
void CAtaFatTable::DoLaunchFat32FreeSpaceScanThreadL()
    {
    __PRINT2(_L("#=- CAtaFatTable::DoLaunchFat32FreeSpaceScanThreadL() drv:%d, state:%d"),iOwner->DriveNumber(), State());
    ASSERT(State() == EMounting);

    //-- 1. check if something is already working (shan't be!)
    if(ipHelperThread)
        {
        if(ipHelperThread->ThreadWorking())
            {
            __PRINT(_L("#=- CAtaFatTable::DoLaunchScanThread() some thread is already running ?"));
            ASSERT(0);
            User::Leave(KErrAlreadyExists);
            }

        DestroyHelperThread();        
        }

    //-- 2. create helper thread object and start the thread
    ipHelperThread = CFat32FreeSpaceScanner::NewL(*this);
    
    SetState(EFreeClustersScan);
    
    User::LeaveIfError(ipHelperThread->Launch()); 
    
    //-- background FAT scanning thread is running now
    }

//-----------------------------------------------------------------------------
/**
    Read an entry from the FAT table

    @param aFatIndex FAT entry number to read
    @return FAT entry value
*/
TUint32 CAtaFatTable::ReadL(TUint32 aFatIndex) const
    {
    if(!ClusterNumberValid(aFatIndex))
        {
        //ASSERT(0); //-- deliberately corrupted (by some tests) DOS directory entries can have 0 in the "first cluster" field.
        __PRINT1(_L("CAtaFatTable::ReadL(%d) bad Index!"), aFatIndex);
        User::Leave(KErrCorrupt);
        }


    const TUint entry = iCache->ReadEntryL(aFatIndex);
    return entry;
    }


//-----------------------------------------------------------------------------
/**
    Write an entry to the FAT table

    @param aFatIndex    aFatIndex FAT entry number to write
    @param aValue       FAT entry to write
    @leave 
*/
void CAtaFatTable::WriteL(TUint32 aFatIndex, TUint32 aValue)
	{

    __PRINT2(_L("CAtaFatTable::WriteL() entry:%d, val:0x%x"), aFatIndex, aValue);
    
    if(!ClusterNumberValid(aFatIndex))
        {
        ASSERT(0); 
        User::Leave(KErrCorrupt);
        }
    
    if(aValue != KSpareCluster && (aValue < KFatFirstSearchCluster || aValue > KFat32EntryMask))
        {
        ASSERT(0);
        User::Leave(KErrCorrupt);
        }

    //-- wait until we are allowed to write FAT entry
    if(ipHelperThread && ipHelperThread->ThreadWorking())
        {
        ASSERT(ipHelperThread->ThreadId() != RThread().Id()); //-- this method must not be called the FAT helper thread	    
        ipHelperThread->RequestFatEntryWriteAccess(aFatIndex);
        }

    //-- write entry to the FAT through FAT cache
    iCache->WriteEntryL(aFatIndex, aValue);

    
    //-- if we are writing "spare" FAT entry, tell FAT bit supercache about it.
    //-- it will store the information that corresponding FAT cache sector has a spare FAT entry.
    //-- writing non-spare FAT entry doesn't mean anything: that FAT cache sector might or might not contain free entries.
    if(aValue == KSpareCluster && iCache->BitCacheInterface())
        {
            CFatBitCache *pFatBitCache = iCache->BitCacheInterface();
            const CFatBitCache::TState cacheState= pFatBitCache->State();
            if(cacheState == CFatBitCache::EPopulated || cacheState == CFatBitCache::EPopulating)
            {//-- bit cache is either normally populated or being populated by one of the helper threads
            if(ipHelperThread && ipHelperThread->ThreadWorking())    
                {
                //-- here we have a multithreading issue. Helper FAT thread can be parsing FAT and optionally calling ReportFreeFatEntry(..) as well.
                //-- in this case we need either to suspend the helper thread in order to prevent corruption of the FAT bit cache data,
                //-- or ignore this call and rely on the fact that the FAT bit supercache is a kind of self-learning and the missing data will be
                //-- fixed during conflict resolution (this can lead to performance degradation).

                //-- ok, suspend the helper thread while we are changing data in the bit cache
                AcquireLock();
                ipHelperThread->Suspend();
                    pFatBitCache->SetFreeFatEntry(aFatIndex);
                ipHelperThread->Resume();
                ReleaseLock();

                }
            else
                {//-- no one else is accessing FAT in this time
                ASSERT(pFatBitCache->UsableState());
                pFatBitCache->SetFreeFatEntry(aFatIndex);
                }
            }

        }//if(aValue == KSpareCluster)

    }

//-----------------------------------------------------------------------------
/**
    This is an overridden method from CFatTable. See CFatTable::FindClosestFreeClusterL(...)
    Does the same, i.e looks for the closest to "aCluster" free FAT entry, but more advanced,
    it can use FAT bit supercache for quick lookup.

    @param aCluster Cluster to find nearest free cluster to.
    @leave KErrDiskFull + system wide error codes
    @return cluster number found
*/
TUint32 CAtaFatTable::FindClosestFreeClusterL(TUint32 aCluster)
    {
    __PRINT2(_L("CAtaFatTable::FindClosestFreeClusterL() drv:%d cl:%d"),iOwner->DriveNumber(),aCluster);

    if(!ClusterNumberValid(aCluster))
        {
        ASSERT(0);
        User::Leave(KErrCorrupt);
        }


    if(!RequestFreeClusters(1))
	    {//-- there is no at least 1 free cluster available
    	__PRINT(_L("CAtaFatTable::FindClosestFreeClusterL() leaving KErrDiskFull #1"));
        User::Leave(KErrDiskFull);
        }

    //-- check if we have FAT bit supercache and it is in consistent state
    CFatBitCache *pFatBitCache = iCache->BitCacheInterface();
    if(!pFatBitCache)
        return CFatTable::FindClosestFreeClusterL(aCluster); //-- fall back to the old search method

    ASSERT(IsFat32());

    if(!pFatBitCache->UsableState())
        {
        //__PRINT(_L("#++ CAtaFatTable::FindClosestFreeClusterL() FAT bit cache isn't consistent!"));
        return CFatTable::FindClosestFreeClusterL(aCluster); //-- fall back to the old search method
        }

    //-- ask FAT bit supercache to find us FAT cache sector (closest to the aCluster) that contains free FAT entries.
    //__PRINT2(_L("#++ CAtaFatTable::FindClosestFreeClusterL(%d) hint free cl:%d"), aCluster, FreeClusterHint());
    
    const TInt KMaxLookupRetries = 2;
    for(TInt i=0; i<KMaxLookupRetries; ++i)
        {
        const TInt nRes = pFatBitCache->FindClosestFreeFatEntry(aCluster);
        switch(nRes)
            {
            case KErrNone:
            //-- FAT bit supercache has found a free FAT entry in the FAT32 cache
            //__PRINT1(_L("#++ CAtaFatTable::FindClosestFreeClusterL FOUND! cl:%d"), aCluster);
            
            ASSERT(ClusterNumberValid(aCluster));

            //-- do not update the last known free cluster, it can be quite expensive.
            //-- do it in the caller method with bigger granularity.
            return aCluster;        

            case KErrNotFound:
            //-- there was a bit cache conflict, when FAT cache sector is marked as having free FAT entries, but it doesn't have them in reality.
            //-- It can happen because FAT bit cache entry is marked '1' only on populating the bit vector or if someone writes KSpareCluster into the 
            //-- corresponding FAT cache sector. Such conflict can happen quite often.
            //-- Try search again, the search is very likely to succeed very close, because the FAT bit cache entry had already been fixed as the result of conflict resolution.
            break;

            case KErrCorrupt: 
            //-- pFatBitCache->FindClosestFreeFatEntry failed to read a page from the media
            //-- break out from the loop and fall back to old search just in case.

            case KErrEof:
            //-- there are no '1' entries in whole FAT bit cache vector at all, which is quite unlikely
            //-- break out from the loop and fall back to old search.
            i=KMaxLookupRetries;
            break;

            //-- unexpected result code.
            default:
            ASSERT(0); 
            i=KMaxLookupRetries;
            break;

        
            };//switch(nRes)

        }//for(TInt i=0; i<KMaxLookupRetries; ++i)

    //-- something went wrong, Bit Fat supercache could not find FAT cache sector that contains at least one free FAT entry.
    //-- this is most likely because of the FAT data mismatch between FAT and bit cache.
    __PRINT(_L("#++ CAtaFatTable::FindClosestFreeClusterL FALLBACK #1"));
    
    //!!!!?? use  not aCluster, but previous search result here ???
    return CFatTable::FindClosestFreeClusterL(aCluster); //-- fall back to the old search method
    }



/**
    Get the next cluster in the chain from the FAT

    @param aCluster number to read, contains next cluster upon return
    @return False if end of cluster chain
*/
TBool CFatTable::GetNextClusterL(TUint32& aCluster) const
    {
	__PRINT1(_L("CAtaFatTable::GetNextClusterL(%d)"), aCluster);
    
    const TUint32 nextCluster = ReadL(aCluster);
    const TBool bEOC = IsEndOfClusterCh(nextCluster);

    if(bEOC) 
        return EFalse; //-- the end of cluster chain

    aCluster = nextCluster;
    
    return ETrue;    
    }

/**
    Write EOF to aFatIndex
    @param aFatIndex index in FAT (cluster number) to be written
*/
void CFatTable::WriteFatEntryEofL(TUint32 aFatIndex)
	{
	__PRINT1(_L("CFatTable::WriteFatEntryEofL(%d)"), aFatIndex);

    //-- use EOF_32Bit (0x0fffffff) for all types of FAT, FAT cache will mask it appropriately
    WriteL(aFatIndex, EOF_32Bit);
    }



/** 
    Mark cluster number aFatIndex in FAT as bad 
    @param aFatIndex index in FAT (cluster number) to be written
*/
void CFatTable::MarkAsBadClusterL(TUint32 aFatIndex)
    {
    __PRINT1(_L("CAtaFatTable::MarkAsBadClusterL(%d)"),aFatIndex);

    //-- use KBad_32Bit (0x0ffffff7) for all types of FAT, FAT cache will mask it appropriately
    WriteL(aFatIndex, KBad_32Bit);
    
    FlushL();
	}


/**
    Return media position in bytes of the cluster start

    @param aCluster to find location of
    @return Byte offset of the cluster data 
*/
TInt64 CAtaFatTable::DataPositionInBytesL(TUint32 aCluster) const
	{
    if(!ClusterNumberValid(aCluster))
        {
        __ASSERT_DEBUG(0, Fault(EFatTable_InvalidIndex));
        User::Leave(KErrCorrupt);
        }

    const TUint32 clusterBasePosition=iOwner->ClusterBasePosition();
	return(((TInt64(aCluster)-KFatFirstSearchCluster) << iOwner->ClusterSizeLog2()) + clusterBasePosition);
	}




//#######################################################################################################################################
//#     CFatHelperThreadBase  implementation
//#######################################################################################################################################

//-----------------------------------------------------------------------------
CFatHelperThreadBase::CFatHelperThreadBase(CAtaFatTable& aOwner)
                      :iOwner(aOwner)
    {

    SetState(EInvalid);
    }

CFatHelperThreadBase::~CFatHelperThreadBase()
    {
    Close();
    }

//-----------------------------------------------------------------------------
/**
    Closes the thread object handle.
    The thread shall not be running.
*/
void CFatHelperThreadBase::Close()
    {
    if(ThreadWorking())
        {
        ASSERT(0);
        ForceStop();
        }

    iThread.Close();
    }

//-----------------------------------------------------------------------------
/**
    Waits for the thread to finish (thread function exit). if it is running.
    @return thread completion code.

    !!!! definitely need a timeout processing here to avoid any possibitlity of hanging forever !!

*/
TInt CFatHelperThreadBase::WaitToFinish() const
    {
    if(!ThreadWorking())
        return ThreadCompletionCode();

    
    //--todo: use timeout and assert to avoid hanging forever ?
    __PRINT1(_L("#= CFatHelperThreadBase::WaitToFinish(), stat:%d"),iThreadStatus.Int());
    User::WaitForRequest(iThreadStatus);
    return iThreadStatus.Int();
    }

//-----------------------------------------------------------------------------

/**
    Requests the fat helper thread function to finish gracefully ASAP; then closes the thread handle. 
    Just sets a flag that is analysed by the thread function and waits thread's request completion.
*/
void CFatHelperThreadBase::ForceStop()
    {
    if(ThreadWorking())
        {
        DBG_STATEMENT(TName name = iThread.Name();)
        __PRINT3(_L("#=!! CFatHelperThreadBase::ForceStop() id:%u, name:%S, status:%d"), (TUint)iThread.Id(), &name, ThreadCompletionCode());
        DBG_STATEMENT(name.Zero()); //-- to avoid warning

        iOwner.AcquireLock();
    
        AllowToLive(EFalse) ; //-- signal the thread to exit ASAP

        iOwner.ReleaseLock();

        WaitToFinish(); //-- wait for the thread to finish.

        //-- don't know why but we need a delay, at least on the emulator. Otherwise thread object doesn't look destroyed.
        //-- probably something with scheduling.
        User::After(10*K1mSec); 
        }

    iThread.Close();
    }


//-----------------------------------------------------------------------------


/**
    Created, initialises and starts the helper thread.
    
    @param  aFunction           pointer to the thread function
    @param  aThreadParameter    parameter to be passed to the thread function. Its interpretation depends on the thread function.
    @return KErrNone on success; standard error code otherwise
*/
TInt CFatHelperThreadBase::DoLaunchThread(TThreadFunction aFunction, TAny* aThreadParameter)
    {
    __PRINT2(_L("#=- CFatHelperThreadBase::DoLaunchThread() thread stat:%d, state:%d"), ThreadCompletionCode(), State());
    
    ASSERT(aFunction);
    ASSERT(State() != EWorking);

    if(ThreadWorking())
        {
        ASSERT(0);
        return KErrInUse;
        }

    if(iOwner.OwnerMount()->Drive().IsSynchronous())
        {
        //-- if the drive is synchronous, this is a main File Server thread. Don't play with it, it has its own scheduler
        //-- and completing other requests rather than native CFsRequest leads to the stray events, because it waits on the 
        //-- User::WaitForAnyRequest and doesn't check which request has completed.
        __PRINT(_L("CFatHelperThreadBase::DoLaunchThread() the drive is synchronous, skipping."));
        return KErrNotSupported;
        }


    TInt nRes;
    TName nameBuf; //-- this will be initial thread name, it will rename itself in its thread function
    nameBuf.Format(_L("Fat32HelperThread_drv_%d"), iOwner.OwnerMount()->DriveNumber());
    const TInt stackSz = 4*K1KiloByte; //-- thread stack size, 4K

    iThread.Close();

    //-- 1. create the thread
    nRes = iThread.Create(nameBuf, aFunction, stackSz, &User::Allocator(), aThreadParameter, EOwnerProcess);
	if(nRes != KErrNone)
	    {
        __PRINT1(_L("#=- CFatHelperThreadBase::DoLaunchThread() failure#1 res:%d"), nRes);
        iThread.Close();
        ASSERT(0);
        return nRes;
        }

    //-- 2. set up its working environment
    AllowToLive(ETrue);
	iThread.SetPriority((TThreadPriority)EHelperPriorityNormal); //-- initially the thread has very low priority
	
    //-- the state of this object now will be controlled by the thread 
    SetState(ENotStarted);

    //-- 3. resume thread and wait until it finishes its initialisation
    TRequestStatus rqStatInit(KRequestPending);
    
    iThread.Logon(iThreadStatus);
    iThread.Rendezvous(rqStatInit);
    iThread.Resume();
   
    User::WaitForRequest(rqStatInit);

    if(rqStatInit.Int() != KErrNone)
        {//-- thread couldn't initialise
        __PRINT1(_L("#=- CFatHelperThreadBase::DoLaunchThread() failure#2 res:%d"), nRes);
        ForceStop();
        ASSERT(0);
        return nRes;
        }

    //-- Helper FAT thread is running now
    return KErrNone; 
    }


//#######################################################################################################################################
//#     CFat32ScanThread implementation
//#######################################################################################################################################


CFat32ScanThread::CFat32ScanThread(CAtaFatTable& aOwner)
                 :CFatHelperThreadBase(aOwner)
    {
    }

//-----------------------------------------------------------------------------

/**
    Launches the FAT32_ScanThread scaner thread.
    @return  KErrNone if the thread launched OK
             standard error code otherwise
*/
TInt CFat32ScanThread::Launch()
    {
    return DoLaunchThread(FAT32_ScanThread, this);    
    }

//-----------------------------------------------------------------------------

/**
    FAT32_ScanThread preamble function. It gets called by the scan thread at the very beginning.
    Does some initialisation work and its return code is signaled to the thread owner by RThread::Rendezvous();
    
    @return Thread object initialisation code, KErrNone on success.
*/
TInt CFat32ScanThread::Thread_Preamble()
    {
    //__PRINT(_L("#=-  CFat32ScanThread::Thread_Preamble()"));

    ipFatBitCache = iOwner.iCache->BitCacheInterface();
    iTimeStart.UniversalTime(); //-- take thread start time
    
    ASSERT(State() == CFatHelperThreadBase::ENotStarted); //-- see the thread launcher
    
    if(!iOwner.IsFat32())
        {//-- this stuff is supposed to work for FAT32 only
        ASSERT(0);
        return KErrArgument; 
        }

    return KErrNone;
    }

//-----------------------------------------------------------------------------
/**
    FAT32_ScanThread postamble function. It gets called by the scan thread just before its function exits.
    Does some finalisation work and its return code is the thread completion code;
    
    @return Thread object finalisation code, KErrNone on success.
*/
TInt CFat32ScanThread::Thread_Postamble(TInt aResult)
    {
    //__PRINT(_L("#=-  CFat32ScanThread::Thread_Postamble()"));

#ifdef _DEBUG    
    //-- print out time taken the thread to finish
    TName nameBuf;
    iTimeEnd.UniversalTime(); //-- take end time
    const TInt msScanTime = (TInt)( (iTimeEnd.MicroSecondsFrom(iTimeStart)).Int64() / K1mSec);
    nameBuf.Copy(RThread().Name());
    nameBuf.Insert(0,_L("#=-<<<")); 
    nameBuf.AppendFormat(_L(" Thread Exit. id:%d, Code:%d, time:%d ms"), (TUint)RThread().Id(), aResult, msScanTime);
    __PRINT(nameBuf);
#endif
    
    //-- tell FAT bit supercache (if we have it) that we have finished populating it, successfully or not
    if(ipFatBitCache) 
        {
        ipFatBitCache->FinishPopulating(aResult == KErrNone);
        ipFatBitCache->Dump();
        }

    //-- close FAT chunk buffer
    iFatChunkBuf.Close();

    //-- set the host object state depending on the work results.
    if(aResult == KErrNone)
        SetState(CFatHelperThreadBase::EFinished_OK); 
    else
        SetState(CFatHelperThreadBase::EFailed); 

   
    return aResult;
    }

//#######################################################################################################################################
//#     CFat32FreeSpaceScanner implementation
//#######################################################################################################################################

CFat32FreeSpaceScanner::CFat32FreeSpaceScanner(CAtaFatTable& aOwner)
                       :CFat32ScanThread(aOwner) 
    {
    }

/**
    Factory method.
    @param  aOwner owning CAtaFatTable
    @return pointer to the constructed instance of the class
*/
CFat32FreeSpaceScanner* CFat32FreeSpaceScanner::NewL(CAtaFatTable& aOwner)
    {
    CFat32FreeSpaceScanner* pThis = NULL;
    pThis = new (ELeave) CFat32FreeSpaceScanner(aOwner);    
    
    return pThis;
    }

//-----------------------------------------------------------------------------

/**
    Waits until FAT32 free clusters scan thread allows other thread (caller) to write to the FAT entry "aFatIndex".
    Thread scans FAT from the beginning to the end and just waits untill scanning passes the entry number "aFatIndex"
    
    @param  aFatIndex index of the FAT entry we are going to write.
*/
void CFat32FreeSpaceScanner::RequestFatEntryWriteAccess(TUint32 aFatIndex) const
    {
    if(!ThreadWorking())
        return;

    ASSERT(iOwner.ClusterNumberValid(aFatIndex));

    const TUint KWaitGranularity = 20*K1mSec; //-- wait granularity

    //-- wait until FAT[aFatIndex] is available to write
    while(aFatIndex > ClustersScanned() && ThreadWorking())
        {
        BoostPriority(ETrue); //-- Boost scan thread priority
        User::After(KWaitGranularity); 
        }
    }

//-----------------------------------------------------------------------------

/** just an internal helper method. Stores the number of FAT entries already scanned by FAT free entries scan thread.  */
void CFat32FreeSpaceScanner::SetClustersScanned(TUint32 aClusters) 
    {
    XAutoLock lock(iOwner.DriveInterface()); //-- enter critical section
    iClustersScanned=aClusters;
    }

/** just an internal helper method. returns the number of FAT entries already scanned by FAT free entrie sscan thread.  */
TUint32 CFat32FreeSpaceScanner::ClustersScanned() const
    {
    XAutoLock lock(iOwner.DriveInterface()); //-- enter critical section
    return iClustersScanned;
    }

//-----------------------------------------------------------------------------

/**
    overriden FAT32_ScanThread preamble function. 
    See CFat32ScanThread::Thread_Preamble()
*/
TInt CFat32FreeSpaceScanner::Thread_Preamble()
    {
    __PRINT1(_L("#=- CFat32FreeSpaceScanner::Thread_Preamble(), FAT state:%d"), iOwner.State());
  
    ASSERT(iOwner.State() == CAtaFatTable::EFreeClustersScan);
    
    //-- invoke generic preamble first
    TInt nRes = CFat32ScanThread::Thread_Preamble();
    if(nRes != KErrNone)
        return nRes;

    //-- do specific to this thread object initialisation work

    //-- rename the thread
    TName nameBuf;
    const CFatMountCB& fatMount = *(iOwner.OwnerMount());
    nameBuf.Format(_L("Fat32FreeSpaceScanner_drv_%d"), fatMount.DriveNumber());
    RThread::RenameMe(nameBuf);

    //-- allocate FAT chunk buffer; its size will depend on FAT table size.
    const TUint32 fatSz = iOwner.MaxEntries() << KFat32EntrySzLog2;

    if(fatSz < KBigSzFat_Threshold)
        {//-- create a small buffer
        if(iFatChunkBuf.CreateMax(KFatChunkBufSize_Small) != KErrNone)
            return KErrNoMemory;
        }
    else
        {//-- try to create larger buffer
        if(iFatChunkBuf.CreateMax(KFatChunkBufSize_Big) != KErrNone && iFatChunkBuf.CreateMax(KFatChunkBufSize_Small) != KErrNone)
            return KErrNoMemory;
        }


    //-- setup FAT table's parameters
    //-- No free clusters yet; be careful with SetFreeClusters(), free clusters count can be 
    //-- modified from other thread, e.g. from FreeClusterList. Use read-modify-write instead of assignment.
    SetClustersScanned(0);
    iOwner.SetFreeClusters(0); 

    //-- calculate number of FAT entires need to be processed for CMountCB::SetDiskSpaceChange() call.
    //-- if number of processed entries in FAT exceeds iEntriesNotifyThreshold, CMountCB::SetDiskSpaceChange()
    //-- will be called and the iEntriesNotifyThreshold will be updated.
    iNfyThresholdInc = (TUint32)KVolSpaceNotifyThreshold >> fatMount.ClusterSizeLog2();
    iEntriesNotifyThreshold = iNfyThresholdInc;

    //-- if there is an interface to the FAT bit supercache, tell it to start populating.  
    //-- We will be populating this cache while reading and parsing FAT32.
    if(ipFatBitCache)
        ipFatBitCache->StartPopulating();


    return KErrNone;
    }

//-----------------------------------------------------------------------------
/**
    overriden FAT32_ScanThread postamble function. 
    See CFat32ScanThread::Thread_Postamble()
*/
TInt CFat32FreeSpaceScanner::Thread_Postamble(TInt aResult)
    {
    __PRINT2(_L("#=- CFat32FreeSpaceScanner::Thread_Postamble(%d), FAT state:%d"), aResult, iOwner.State());
    __PRINT2(_L("#=- FAT_ScanThread: counted Free clusters:%d, 1st free:%d"), iOwner.NumberOfFreeClusters(), iOwner.FreeClusterHint());

    ASSERT(iOwner.State() == CAtaFatTable::EFreeClustersScan);

    //-- there was an error somewhere within FAT32 scan thread
    if(aResult != KErrNone)
        {
        //-- indicate that the FAT table initialisation failed
        __PRINT(_L("#=- Asynch FAT table initialisation failed !"));

        iOwner.SetState(CAtaFatTable::EMountAborted);
     
        //-- fix up some FAT table parameters
        if(iOwner.FreeClusterHint() < KFatFirstSearchCluster)
            iOwner.SetFreeClusterHint(KFatFirstSearchCluster);    

        }

   
    //-- call generic postamble
    TInt nRes = CFat32ScanThread::Thread_Postamble(aResult);
    
    if(nRes == KErrNone)
        {//-- FAT table now fully initialised
        ASSERT(aResult == KErrNone);
        iOwner.SetState(CAtaFatTable::EMounted);

        //-- free space counting finished OK, call the notifier last time
        CFatMountCB& fatMount = *(iOwner.OwnerMount());
        
        iOwner.AcquireLock();
        const TInt64 currFreeSpace = ((TInt64)iOwner.FreeClusters()) << fatMount.ClusterSizeLog2();
        iOwner.ReleaseLock();

        fatMount.SetDiskSpaceChange(currFreeSpace);


        }
    else if(aResult == KErrNone)
        {//-- CFat32ScanThread::Thread_Postamble() signaled a fault
        iOwner.SetState(CAtaFatTable::EMountAborted);
        }

    return aResult;
    }

//-----------------------------------------------------------------------------
/**
    Process free FAT entries collected by the scan thread that parses chunk of FAT data.
    This method gets called by the FAT scanning thread after a portion of FAT is read into the buffer and parsed

    @param  aFreeEntriesInChunk number of free FAT entries counted in FAT chunk
    @param  aCurrFirstFreeEntry current number of the first free FAT entry found
    @param  aClustersScanned    total number of FAT entries scanned by the thread

    @return standard error code, KErrNone on success
*/
TInt CFat32FreeSpaceScanner::Thread_ProcessCollectedFreeEntries(const CAtaFatTable::TFatScanParam& aFatScanParam)
    {
    ASSERT(State() == CFatHelperThreadBase::EWorking); 

    CAtaFatTable& ataFatTable = iOwner;

    //-------------------------------------------
    //-- publish values to the CAtaFatTable object
    ataFatTable.AcquireLock();
            
    //-- publish free cluster count, use read-modify-write here
    //-- CFatTable::iFreeClusters can be already modified from other thread.
    TUint32 currFreeClusters = ataFatTable.FreeClusters(); //-- simple non-thread safe method
    
    currFreeClusters += aFatScanParam.iCurrFreeEntries;

    ataFatTable.SetFreeClusters(currFreeClusters);

    //-- store total number of scanned clusters (not to be modified from other thread)
    const TUint32 scannedEntries = aFatScanParam.iEntriesScanned;
    SetClustersScanned(scannedEntries); 
   

    if(aFatScanParam.iFirstFree >= KFatFirstSearchCluster)
        ataFatTable.SetFreeClusterHint(aFatScanParam.iFirstFree);//-- probably found next free cluster number 

    ataFatTable.ReleaseLock();

    //-- check if we need to call CMountCB::SetDiskSpaceChange() to notify it that the amount of processed FAT entries has reached the given threshold
    if(scannedEntries >= iEntriesNotifyThreshold)
        {
        iEntriesNotifyThreshold += iNfyThresholdInc;

        CFatMountCB& fatMount = *(iOwner.OwnerMount());
        const TInt64 currFreeSpace = ((TInt64)currFreeClusters) << fatMount.ClusterSizeLog2();
        fatMount.SetDiskSpaceChange(currFreeSpace);
        }


    return KErrNone;
    }

//#######################################################################################################################################
//#     CFat32BitCachePopulator implementation
//#######################################################################################################################################
CFat32BitCachePopulator::CFat32BitCachePopulator(CAtaFatTable& aOwner)
                       :CFat32ScanThread(aOwner) 
    {
    }

/**
    Factory method.
    @param  aOwner owning CAtaFatTable
    @return pointer to the constructed instance of the class
*/
CFat32BitCachePopulator* CFat32BitCachePopulator::NewL(CAtaFatTable& aOwner)
    {
    CFat32BitCachePopulator* pThis = NULL;
    pThis = new (ELeave) CFat32BitCachePopulator(aOwner);    
    
    return pThis;
    }

//-----------------------------------------------------------------------------

/**
    The main FS thread tries to write the "aFatIndex" entry in FAT while this thread is running.
    We can't do anything useful here, because FAT32 bit supercache doesn't work on FAT entry level and
    deals with much less scale - FAT32 cache sector, which can consist from many FAT32 entries.
    The conflict situation will be resolved in the CAtaFatTable::WriteL()
*/
void CFat32BitCachePopulator::RequestFatEntryWriteAccess(TUint32 /*aFatIndex*/) const
    {
    //-- do nothing here, do not block the caller
    }


//-----------------------------------------------------------------------------
/**
    overriden FAT32_ScanThread preamble function. 
    See CFat32ScanThread::Thread_Preamble()
*/
TInt CFat32BitCachePopulator::Thread_Preamble()
    {
    __PRINT(_L("#=- CFat32BitCachePopulator::Thread_Preamble()"));
    
    //-- invoke generic preamble
    TInt nRes = CFat32ScanThread::Thread_Preamble();
    if(nRes != KErrNone)
        return nRes;

    //-- do specific to this thread object initialisation work
    iTotalOccupiedFatEntries = 0;

    //-- rename the thread
    TName nameBuf;
    const CFatMountCB& fatMount = *(iOwner.OwnerMount());
    nameBuf.Format(_L("CFat32BitCachePopulator_drv_%d"), fatMount.DriveNumber());
    RThread::RenameMe(nameBuf);

    //-- allocate FAT chunk buffer
    nRes = iFatChunkBuf.CreateMax(KFatChunkBufSize);
    if(nRes != KErrNone)
        return nRes;

    
    if(!ipFatBitCache)
        {//-- this is a bit cache populator and the bit cache object must have been constructed before setting up the populating thread.
        ASSERT(0);
        return KErrCorrupt;
        }
    
    //-- Tell FAT bit supercache to start populating. We will be populating this cache while reading and parsing FAT32.
    if(ipFatBitCache->StartPopulating())
        nRes = KErrNone;
    else
        nRes = KErrCorrupt;

    return nRes;
    }

//-----------------------------------------------------------------------------

/**
    overriden FAT32_ScanThread postamble function. 
    See CFat32ScanThread::Thread_Postamble()
*/
TInt CFat32BitCachePopulator::Thread_Postamble(TInt aResult)
    {
    __PRINT1(_L("#=- CFat32BitCachePopulator::Thread_Postamble(%d)"), aResult);

    //-- nothing specific to do, just call generic method
    return CFat32ScanThread::Thread_Postamble(aResult);
    }

//-----------------------------------------------------------------------------
/**
    This method gets called by the FAT scanning thread after a portion of FAT is read into the buffer and parsed
    @return standard error code, KErrNone on success
*/
TInt CFat32BitCachePopulator::Thread_ProcessCollectedFreeEntries(const CAtaFatTable::TFatScanParam& aFatScanParam)
    {
    ASSERT(State() == CFatHelperThreadBase::EWorking); 
    
    //-- check the bit cache state
    if(ipFatBitCache->State() != CFatBitCache::EPopulating)
        {//-- something wrong happened to the cache, e.g. someone forcedly invalidated it (probably from another thread)
        return KErrAbort;
        }

    
    //-- if CFat32BitCachePopulator has already counted all _occupied_ FAT entries, there is no need to 
    //-- continue FAT reading; just mark the rest of the FAT bit supercache as containing free FAT entries and abort scanning

    CAtaFatTable& ataFatTable = iOwner;

    ataFatTable.AcquireLock();
    
    //-- current amount of non-free entries in FAT, excluding FAT[0] & FAT[1]
    const TUint32 KCurrNonFreeEntries = ataFatTable.MaxEntries() - ataFatTable.FreeClusters() - KFatFirstSearchCluster;
    
    iTotalOccupiedFatEntries += aFatScanParam.iCurrOccupiedEntries;
    
    //-- check if the thread needs to continue it work
    const TBool KNoNeedToScanFurther = (iTotalOccupiedFatEntries >= KCurrNonFreeEntries);

    if(KNoNeedToScanFurther)
        {
        //-- tell FAT bit supercache to mark the range from currently scanned FAT entry to the end of the FAT as containing free entries.
        __PRINT2(_L("#=- CFat32BitCachePopulator::Thread_ProcessCollectedFreeEntries() counted: %d/%d; aborting scan."), iTotalOccupiedFatEntries, KCurrNonFreeEntries);
        
        const TUint32 entryStart = aFatScanParam.iEntriesScanned; //-- first FAT entry in the range to be marked as 'free'
        const TUint32 entryEnd   = ataFatTable.MaxEntries()-1;    //-- last FAT entry in the range to be marked as 'free', last FAT entry

        ipFatBitCache->MarkFatRange(entryStart, entryEnd, ETrue);
        
        //-- signal that the thread shall finish with normal (KErrNone) reason
        //-- it will also normally finish FAT bit cache populating in postamble
        AllowToLive(EFalse); 
        }

    ataFatTable.ReleaseLock();

    
    return KErrNone;
    }


//#######################################################################################################################################
/**
    FAT32 free entries scan thread function. Walks through FAT32 and counts free entries.
    It uses its own buffer to read FAT and parse it in order to avoid multithreaded problems with FAT cache and don't thrash it.

    @param apHostObject pointer to the host object of CFat32ScanThread base class.
*/
//#######################################################################################################################################
TInt FAT32_ScanThread(TAny* apHostObject)
    {
    TInt    nRes;

#ifdef _DEBUG
    TName   nameBuf;
    nameBuf.Copy(RThread().Name());
    nameBuf.Insert(0,_L("#=->>>")); nameBuf.AppendFormat(_L(" Thread Enter (id:%d)"), (TUint)RThread().Id());
    __PRINT(nameBuf);
#endif
    
    ASSERT(apHostObject);
    CFat32FreeSpaceScanner* pSelf = (CFat32FreeSpaceScanner*)apHostObject;

    CAtaFatTable& ataFatTable = pSelf->iOwner;
    CFatMountCB&  fatMount = *(ataFatTable.OwnerMount());

    const TUint32 KFat32EntrySz = sizeof(TFat32Entry); 
    const TUint32 KFat1StartPos = fatMount.StartOfFatInBytes();
    const TUint32 KNumClusters = ataFatTable.MaxEntries(); //-- FAT[0] & FAT[1] are reserved and not counted by UsableClusters()

    //-- perform thread preamble work
    nRes = pSelf->Thread_Preamble();

    //-- signal the thread initialisation result
    RThread::Rendezvous(nRes);


    //-- Initialisation OK, do real job: FAT scanning
    if(nRes == KErrNone)
        {
        pSelf->SetState(CFatHelperThreadBase::EWorking); 

        TUint32 rem = KNumClusters * KFat32EntrySz; 
        TUint32 mediaPos = KFat1StartPos;   
        
        CAtaFatTable::TFatScanParam fatScanParam; //-- FAT scanning parameters

        //============================================
        //=== FAT read and parse loop ================
        //-- in this loop we read portions of raw FAT32 data in a buffer, than parse this buffer 
        //-- in order to find out the number of free FAT entries there and other stuff
        while(rem)
            {
            const TUint32 bytesToRead=Min(rem, (TUint32)pSelf->iFatChunkBuf.Size());
            TPtrC8 ptrData(pSelf->iFatChunkBuf.Ptr(), bytesToRead);

            //-- check for sudden media change
            if(fatMount.Drive().IsChanged()) 
                {
                __PRINT(_L("#=--- FAT32_ScanThread: Media change occured, aborting!"));
                nRes = KErrAbort;
                break;
                }

            //-------------------------------------------
            //-- read a portion of FAT into the buffer
            ataFatTable.AcquireLock();
            
            //-- check if the thread was requested to finish
            if(!pSelf->AllowedToLive()) 
                {
                ataFatTable.ReleaseLock();
                nRes = KErrAbort;
                break;
                }

            //-- actual read
            //__PRINT3(_L("#=--- FAT32_ScanThread: read %d bytes pos:0x%x, boost:%d"), bytesToRead, mediaPos, pSelf->IsPriorityBoosted());

            nRes = fatMount.LocalDrive()->Read(mediaPos, bytesToRead, pSelf->iFatChunkBuf); 
            
            ataFatTable.ReleaseLock();

            //-------------------------------------------
            //-- analyse the read error code
            if(nRes != KErrNone)
                {
                __PRINT1(_L("#=--- FAT32_ScanThread read error! res:%d"), nRes);
                break; //-- abort scanning
                }

            //-------------------------------------------
            //-- parse FAT from the buffer
            
            //-- we need number of free and occupied entries in the _current_ FAT chunk being read and parsed
            fatScanParam.iCurrFreeEntries     = 0;
            fatScanParam.iCurrOccupiedEntries = 0;

            ataFatTable.DoParseFatBuf(ptrData, fatScanParam);

            //--- process the the results of FAT buffer parsing
            nRes = pSelf->Thread_ProcessCollectedFreeEntries(fatScanParam);
            if(nRes != KErrNone || !pSelf->AllowedToLive())
                {//-- some types of worker threads may wish to finish normally but prematurely, by the result of Thread_ProcessCollectedFreeEntries()
                break; //-- abort scanning
                }


            //-- allow this thread to be preempted by another one that wants to access the media driver.
            //-- without this wait we will have priority inversion, because this (low priority) thread continiously reads data by big chunks 
            //-- and doesn't allow others to access the driver.
            //-- On the other hand, if the thread's priority is boosted, there is no reason to be so polite.
            if(!pSelf->IsPriorityBoosted())
                {//-- User::After() granularity can be much coarser than 1ms, e.g. 1/64 Sec. This will add up to the scanning time
                User::After(K1mSec); 
                }
            else
                {//-- use much less coarse granularity to allow this thread to be preempted even if its priority is boosted.
                User::AfterHighRes(128); 
                }

            //-------------------------------------------
            mediaPos += bytesToRead;
            rem -= bytesToRead;
        
            }//while(rem)
    
        }//if(nRes == KErrNone)


    //-- perform thread postamble work
    nRes = pSelf->Thread_Postamble(nRes);

    return nRes;
    }

































