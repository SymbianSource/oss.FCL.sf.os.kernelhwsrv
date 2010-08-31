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
// f32\sfat32\fat_config.h
// FAT fsy configurator
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef FAT_CONFIGURATOR_H
#define FAT_CONFIGURATOR_H

#include <f32dbg.h>
//-----------------------------------------------------------------------------

/**
    This class provides access to the FAT settings in estart.txt file and the interface to 
    get these setting to the client (CFatMountCB).
*/
class TFatConfig
    {
 public:   

    TFatConfig();
    void ReadConfig(TInt aDrvNumber, TBool aForceRead = EFalse);

    //-- parameters getters
    inline TBool ScanDrvSkipFinalisedVolume() const;
    inline TBool FAT32_AsynchMount() const;
    inline TBool FAT32_UseFSInfoOnMount() const;
    inline TBool FAT32_UseBitSupercache() const;

    inline TUint32 FAT32_SyncScanThresholdMB() const;

    inline TUint32 DirCacheSize() const;
    inline TUint32 DirCacheMaxPageSizeLog2() const;

    inline void Fat16FixedCacheParams(TUint32& aRdGrLog2, TUint32& aWrGrLog2) const;
    inline void Fat32LruCacheParams(TUint32& aRdGrLog2, TUint32& aWrGrLog2, TUint32& aCacheSize) const;
    inline TBool FAT16_UseCleanShutDownBit() const;

    inline TUint32 LeafDirCacheSize() const;
    inline TUint32 DynamicDirCacheSizeMin() const;
    inline TUint32 DynamicDirCacheSizeMax() const;
    inline TUint32 DynamicDirCacheMaxPageSizeLog2() const;


 protected:

    TFatConfig(const TFatConfig&);
    TFatConfig& operator=(const TFatConfig&);

    TUint32 ReadUint(const TDesC8& aSection, const TDesC8& aKeyName, TUint32 aDefaultValue) const; 

 private:
    
    void DumpParameters() const;   
    void DoDumpUintParam(const TDesC8& aKeyName, TUint32 aParam) const; 

    void ProcessDirCacheParams(const TDesC8& aSection);
    void ProcessFatCacheParams(const TDesC8& aSection);
    void ProcessDynamicDirCacheParams(const TDesC8& aSection);

 private:
    
    TUint32 iInitialised : 1; //-- ETrue if the object is initialised, i.e. ReadConfig() called

    //-- cached FAT parameters, see appropriate methods description
    
    //-- boolean values
    TUint32 iScanDrvSkipFinalisedVolume :1; ///< if 1 ScanDrive will skip properly finalised volumes
    TUint32 iFAT32_AsynchMount          :1; ///< if 1 FAT3 Asynchronous mounting is enabled  
    TUint32 iFAT32_UseFSInfoOnMount     :1; ///< 1 enables using FSInfo sector on FAT32 volumes
    TUint32 iFAT32_UseBitSupercache     :1; ///< 1 enables using FAT32 bit supercache for fast free cluster lookup
    TUint32 iFAT16_UseCleanShutDownBit  :1; ///< if 1 "clean shutdown mask" bit in FAT16[1] will be used during volume finalisations.

    //---

    TUint16 iDirCacheSizeKB;                ///< directory cache size, Kbytes
    TUint16 iDirCacheMaxPageSizeLog2;       ///< Log2(Max. dir. cache page size)

    TUint16 iFat16FixedCacheReadGrLog2;     ///< Log2(FAT16 fixed cache read granularity)
    TUint16 iFat16FixedCacheWriteGrLog2;    ///< Log2(FAT16 fixed cache write granularity)

    TUint16 iFat32LRUCacheSizeKB;           ///< FAT32 LRU cache size, Kbytes
    TUint16 iFat32LRUCacheReadGrLog2;       ///< Log2(FAT32 LRU cache read granularity)
    TUint16 iFat32LRUCacheWriteGrLog2;      ///< Log2(FAT32 LRU cache write granularity)
    
    TUint32 iLeafDirCacheSize;              ///< leaf directory cache size, maximum number of most recently visited leaf dirs to be cached
    TUint32 iDynamicDirCacheSizeMinKB;      ///< minimum directory cache size, Kbytes
    TUint32 iDynamicDirCacheSizeMaxKB;      ///< maximum directory cache size, Kbytes
    TUint32 iDynamicDirCacheMaxPageSizeLog2;///< Log2(maximum dynamic dir cache page size)

    TUint32 iSyncScanThresholdMB;           ///< FAT32 Asynchronous Scan threshold in MegaBytes

    };

#include"fat_config.inl"

#endif //FAT_CONFIGURATOR_H






