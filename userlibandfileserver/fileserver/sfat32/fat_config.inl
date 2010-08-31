// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfat\fat_config.inl
// FAT fsy configurator
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef FAT_CONFIGURATOR_INL
#define FAT_CONFIGURATOR_INL

//-----------------------------------------------------------------------------
/**
    Meaning of this parameter:
    if not 0, ScanDrive will not run on finalised FAT volume, i.e. that has "CleanShutdown" flag set in FAT[1] by
    proper finalisation.
    See CFatMountCB::ScanDrive().
*/
TBool TFatConfig::ScanDrvSkipFinalisedVolume() const
    {
    ASSERT(iInitialised);
    return iScanDrvSkipFinalisedVolume;
    }

//-----------------------------------------------------------------------------
/**
    Meaning of this parameter:
    if not 0, enables asynchronous mounting of the FAT32 volumes.
    In particular background FAT32 scan for free clusters. See CAtaFatTable::CountFreeClustersL().
*/
TBool TFatConfig::FAT32_AsynchMount() const
    {
    ASSERT(iInitialised);
    return iFAT32_AsynchMount;
    }

//-----------------------------------------------------------------------------
/**
    Meaning of this parameter:
    if not 0, enables using FSInfo sector data during FAT32 volumes mounting to determine number of free clusters etc.
*/
TBool TFatConfig::FAT32_UseFSInfoOnMount() const
    {
    ASSERT(iInitialised);
    return iFAT32_UseFSInfoOnMount;
    }

//-----------------------------------------------------------------------------
/**
    Meaning of this parameter:
    if not 0, enables using FAT32 bit supercache that makes free FAT entries lookup faster
*/
TBool TFatConfig::FAT32_UseBitSupercache() const
    {
    ASSERT(iInitialised);
    return iFAT32_UseBitSupercache;
    }

//-----------------------------------------------------------------------------
/**
    Meaning of this parameter:
    Overall FAT directory cache size in bytes
*/
TUint32 TFatConfig::DirCacheSize() const
    {
    ASSERT(iInitialised);
    return iDirCacheSizeKB * K1KiloByte;
    }

//-----------------------------------------------------------------------------

/**
    Meaning of this parameter:
    Log2 of the maximal size of the dir. cache page in bytes. (Minimal size will be  current cluster size)
*/
TUint32 TFatConfig::DirCacheMaxPageSizeLog2() const
    {
    ASSERT(iInitialised);
    return iDirCacheMaxPageSizeLog2;
    }

/**
    Get FAT16 fixed cache parameters
    @param  aRdGrLog2 Log2(read granularity)
    @param  aWrGrLog2 Log2(write granularity)
*/
void TFatConfig::Fat16FixedCacheParams(TUint32& aRdGrLog2, TUint32& aWrGrLog2) const
    {
    ASSERT(iInitialised);
    aRdGrLog2 = iFat16FixedCacheReadGrLog2; 
    aWrGrLog2 = iFat16FixedCacheWriteGrLog2;
    }
/**
    Get FAT32 LRU cache parameters
    @param  aRdGrLog2 Log2(read granularity)
    @param  aWrGrLog2 Log2(write granularity)
    @param  aCacheSize maximal cache size, bytes
*/
void TFatConfig::Fat32LruCacheParams(TUint32& aRdGrLog2, TUint32& aWrGrLog2, TUint32& aCacheSize) const
    {
    ASSERT(iInitialised);
    aRdGrLog2 = iFat32LRUCacheReadGrLog2;
    aWrGrLog2 = iFat32LRUCacheWriteGrLog2;
    aCacheSize = iFat32LRUCacheSizeKB * K1KiloByte;
    }


/**
    Meaning of this parameter:
    if not 0, "clean shutdown mask" bit in FAT16[1] will be used during volume finalisation.
    Otherwise, FAT16[1] will not be affected during finalisation
*/
TBool TFatConfig::FAT16_UseCleanShutDownBit() const
    {
    ASSERT(iInitialised);
    return iFAT16_UseCleanShutDownBit;
    }



//-----------------------------------------------------------------------------
/**
    Get leaf dir cache size
	@return leaf dir cache size
*/
TUint32 TFatConfig::LeafDirCacheSize() const
    {
    ASSERT(iInitialised);
    return iLeafDirCacheSize;
    }

/**
	get the minimum cache size setting for dynamic dir cache
	@return minimum cache size in bytes				
*/
TUint32 TFatConfig::DynamicDirCacheSizeMin() const
	{
    ASSERT(iInitialised);
    ASSERT(iDynamicDirCacheSizeMinKB < (KMaxTUint32 >> K1KiloByteLog2));		//check data overflow
    return iDynamicDirCacheSizeMinKB << K1KiloByteLog2;
	}

/**
	get the maximum cache size setting for dynamic dir cache
	@return maximum cache size in bytes				
*/
TUint32 TFatConfig::DynamicDirCacheSizeMax() const
	{
    ASSERT(iInitialised);
    ASSERT(iDynamicDirCacheSizeMaxKB < (KMaxTUint32 >> K1KiloByteLog2));		//check data overflow
    return iDynamicDirCacheSizeMaxKB << K1KiloByteLog2;
	}

/**
    retrieve the size of the maximal size of the dynamic dir cache page in log2. 
    (Minimal size will be  current cluster size)
    @return maximum page size in bytes in log2
*/
TUint32 TFatConfig::DynamicDirCacheMaxPageSizeLog2() const
    {
    ASSERT(iInitialised);
    return iDynamicDirCacheMaxPageSizeLog2;
    }

//-----------------------------------------------------------------------------
/**
    @return FAT32 Asynchronous Scan threshold in MegaBytes
*/
TUint32 TFatConfig::FAT32_SyncScanThresholdMB() const
    {
    ASSERT(iInitialised);
    return iSyncScanThresholdMB;
    }



#endif //FAT_CONFIGURATOR_INL






