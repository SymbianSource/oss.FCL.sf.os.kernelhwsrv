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
// f32\sfat\fat_config.cpp
// Implementation of the class that reads FAT filesystem configuration parameters from the estart.txt file, 
// caches and provides access to them
// 
//

/**
 @file
*/

#include <f32fsys.h>

#include "common_constants.h"
#include "sl_std.h"
#include "fat_config.h"

//########################################################################################################################
//-- parameters that can be defined in estart.txt file under [Drive%] section

//=======================================================================================================================
//-- FAT volumes mounting related settings
//=======================================================================================================================

//-- if this parameter is not 0, ScanDrive will skip finalised volume. Applicable for FAT16/32
//-- e.g:   FAT_ScanDrv_SkipFinalisedVolume 1
_LIT8(KPN_ScanDrvSkipFinalisedVolume,"FAT_ScanDrv_SkipFinalisedVolume");  //-- parameter key name
static const TUint32 KDef_ScanDrvSkipFinalisedVolume = 0; //-- parameter default value

//-- if this parameter is not 0, enables asynchronous mounting FAT32 volumes. 
//-- At present it means enabling/disabling FAT32 background scanning for free clusters. Applicable for FAT32 only
//-- e.g:   FAT_FAT32_AsynchMount=0
_LIT8(KPN_FAT32_AsynchMount, "FAT_FAT32_AsynchMount"); 
static const TUint32 KDef_FAT32_AsynchMount = 1;

//-- Using FSInfo sector data during FAT32 mount to find out number of free clusters etc.
//-- e.g:   FAT_FAT32_UseFsInfo=1
_LIT8(KPN_FAT32_UseFsInfoOnMount, "FAT_FAT32_UseFsInfo"); 
static const TUint32 KDef_FAT32_UseFsInfoOnMount = 1;

//-- Using FAT bit supercache for fast free cluster lookup
//-- e.g:   FAT_FAT32_UseBitSupercache 0
_LIT8(KPN_FAT32_UseFatBitSupercache, "FAT_FAT32_UseBitSupercache"); 
static const TUint32 KDef_FAT32_UseFatBitSupercache = 1;

//-- FAT free space scan thread threshold in MegaBytes. If this value is set, the call to ::VolumeL() may be blocked until
//-- the given amount of free space (MB) is counted  by FAT32 free space scan thread. After this calls to ::VolumeL() will
//-- become asynchronous, returning _current_ amount of free space (whic is > than the given threshold)
//-- This may help avoiding the situation when FAT32 asynchronous mounting basically turns into a synchronous one,  when someone is calling RFs::Volume()
//-- just after FAT free space scan thread started. In such a case only first free NNN megabytes will be discovered synchronously, after this
//-- the RFs::Volume() call will become asynchronous, returning the changing amount of free spece on the volume.

//-- the default value is 0, which means: "no threshold, don't use it"
_LIT8(KPN_FAT32_SyncScanThreshold, "FAT_FAT32_SyncScanThr"); 
static const TUint32 KDef_FAT32_SyncScanThreshold = 0;



//-- if this parameter is not 0, "clean shutdown mask" bit in FAT16[1] will be used during volume finalisation.
//-- Otherwise, FAT16[1] will not be affected during finalisation
//-- e.g:   KDef_FAT16_UseCleanShutDownBit 0
_LIT8(KPN_FAT16_UseCleanShutDownBit,"FAT16_UseCleanShutDownBit");  //-- parameter key name
static const TUint32 KDef_FAT16_UseCleanShutDownBit = 1;           //-- parameter default value


//=======================================================================================================================
//-- plain old FAT directory cache settings
//=======================================================================================================================
//-- FAT_DirCache <CacheSizeKB>,<Log2(max page size)>
//-- e.g:   FAT_DirCache 16,12
_LIT8(KPN_FAT_DirCache, "FAT_DirCache"); 

static const TUint32 KDef_KDirCacheSizeKB = 16;      //-- default value for the total directory cache size, kilobytes. 16K by default
static const TUint32 KDef_MaxDirCachePageSzLog2 = 12;//-- default value for directory cache single page maximal size Log2, 2^12 (4K) by default


//=======================================================================================================================
//-- FAT cache settings
//=======================================================================================================================

//-- Fat16 fixed cache
//-- FAT_FAT16FixedCache <Log2(read granularity)>,<Log2(Write granularity)> 
//-- e.g:   FAT_FAT16FixedCache 12,9 
_LIT8(KPN_FAT_FAT16FixedCache, "FAT_FAT16FixedCache"); 
static const TUint32 KDef_FAT16_FixedCache_Read_Granularity_LOG2 = 12;  //-- read granularity  Log2, 4K
static const TUint32 KDef_FAT16_FixedCache_Write_Granularity_LOG2 = 9;  //-- write granularity Log2, 512 bytes    


//-- Fat32 LRU cache
//-- FAT_FAT32LruCache <Size KB>,<Log2(read granularity)>,<Log2(Write granularity)> 
//-- e.g:   FAT_FAT32LruCache 128,12,9
_LIT8(KPN_FAT_FAT32LruCache, "FAT_FAT32LruCache"); 


static const TUint32 KDef_FAT32_LruCache_SizeKB = 128;                  //-- LRU cache size, 128KB
static const TUint32 KDef_FAT32_LruCache_Read_Granularity_LOG2 = 12;    //-- read granularity  Log2, 4K     
static const TUint32 KDef_FAT32_LruCache_Write_Granularity_LOG2 = 9;    //-- write granularity Log2, 512 bytes      


//=======================================================================================================================
//-- FAT leaf directory cache settings
//=======================================================================================================================
//-- A leaf directory cache for Fat volumes
_LIT8(KPN_FAT_LeafDirCache, "FAT_LeafDirCacheSize"); 
static const TUint32 KDef_KLeafDirCacheSize = 32;    //-- default number of the most recently visited leaf dirs to be cached

//=======================================================================================================================
//-- DP directory cache settings
//=======================================================================================================================
//-- New directory cache uses the global cache memory manager for dynamic size allocation
_LIT8(KPN_FAT_DynamicDirCacheMin, "FAT_DirCacheSizeMin"); 
_LIT8(KPN_FAT_DynamicDirCacheMax, "FAT_DirCacheSizeMax"); 
static const TUint32 KDef_DynamicDirCacheMin = 128;		// default minimum fat dir cache size in KB
static const TUint32 KDef_DynamicDirCacheMax = 256;		// default maximum fat dir cache size in KB
static const TUint32 KDef_MaxDynamicDirCachePageSzLog2 = 14;    // default value for directory cache single page 
                                                                //  maximal size Log2, 2^14 (16K) by default


//########################################################################################################################



//-----------------------------------------------------------------------------
/**
    Extract a token from the comma-separated tokens string.

    @param  aSrc        source string descriptor
    @param  aTokenNo    token number to extract (the 1st token is number 0)
    @param  aToken      on success there will be extracted token
    
    @return ETrue on success and an extracted token in aToken
*/
static TBool GetCSV_Token(const TDesC8& aSrc, TUint aTokenNo, TPtrC8& aToken)
    {

    const TChar chDelim = ','; //-- token delimiter, comma
    TInt nRes;
    
    //-- 1. find the beginning of the token we are looking for
    TPtrC8 ptrCurrToken(aSrc);
    for(TUint i=0; i<aTokenNo; ++i)
        {
        const TInt len = ptrCurrToken.Length();

        nRes = ptrCurrToken.Locate(chDelim);
        if(nRes == KErrNotFound)
            return EFalse; //-- did tot find the required token

        ptrCurrToken.Set(ptrCurrToken.Right(len-nRes-1));
        }

    //-- 2. set the end position of the token we found
    aToken.Set(ptrCurrToken);

    nRes = ptrCurrToken.Locate(chDelim);
    if(nRes != KErrNotFound)
        {
        aToken.Set(ptrCurrToken.Left(nRes));
        }

    return ETrue;
    }


//-----------------------------------------------------------------------------
/**
    Extract a token from the comma-separated tokens string and try to convert it into TInt

    @param  aSrc        source string descriptor
    @param  aTokenNo    token number to extract (the 1st token is number 0)
    @param  aVal        On success will contain the value represented by the token

    @return ETrue if the token is extracted and converted to TInt OK, in this case aVal contains value
*/
static TBool GetIntTokenVal(const TDesC8& aSrc, TUint aTokenNo, TInt& aVal)
{
    TPtrC8      ptrToken;
    TLex8       lex;
    TInt        val;

    if(!GetCSV_Token(aSrc, aTokenNo, ptrToken))
        return EFalse;

    lex.Assign(ptrToken);
    lex.SkipSpace();
    
    if(lex.Val(val) != KErrNone)
        return EFalse;

    aVal = val;
    return ETrue;
}

static TBool GetIntTokenVal(const TDesC8& aSrc, TUint aTokenNo, TUint16& aVal)
{
    TInt  val;
    if(!GetIntTokenVal(aSrc, aTokenNo, val))
        return EFalse;

    aVal = (TUint16)val;
    return ETrue;
}


//-----------------------------------------------------------------------------
TFatConfig::TFatConfig()
    {
    iInitialised = EFalse;
    }

//-----------------------------------------------------------------------------

/**
    reads FAT parameters from estart.txt file.
    @param  aDrvNumber a valid drive number
    @param  aForceRead if ETrue parameters will be forcedly re-read.
*/
void TFatConfig::ReadConfig(TInt aDrvNumber, TBool aForceRead /*=EFalse*/)
    {
    if(aForceRead)
        iInitialised = EFalse;

    if(iInitialised)
        return;

    //__PRINT1(_L("#>- TFatConfig::ReadConfig() drive:%d\n"),aDrvNumber);
    ASSERT(aDrvNumber >= EDriveA && aDrvNumber <= EDriveZ);
    
    //-- make a section name, like "DriveX"
    TBuf8<32> section;
    section.Format(_L8("Drive%c"), 'A'+aDrvNumber);
    
    //-- read values from estart.txt and cache them
    iScanDrvSkipFinalisedVolume = ReadUint(section, KPN_ScanDrvSkipFinalisedVolume, KDef_ScanDrvSkipFinalisedVolume);
    iFAT32_AsynchMount          = ReadUint(section, KPN_FAT32_AsynchMount,          KDef_FAT32_AsynchMount);
    iFAT32_UseFSInfoOnMount     = ReadUint(section, KPN_FAT32_UseFsInfoOnMount,     KDef_FAT32_UseFsInfoOnMount);
    iFAT32_UseBitSupercache     = ReadUint(section, KPN_FAT32_UseFatBitSupercache,  KDef_FAT32_UseFatBitSupercache);
    iFAT16_UseCleanShutDownBit  = ReadUint(section, KPN_FAT16_UseCleanShutDownBit,  KDef_FAT16_UseCleanShutDownBit);
    iSyncScanThresholdMB        = ReadUint(section, KPN_FAT32_SyncScanThreshold,    KDef_FAT32_SyncScanThreshold);           

    // If leaf dir cache is supported, read the configuration from estart.txt file
    iLeafDirCacheSize			= ReadUint(section, KPN_FAT_LeafDirCache,  			KDef_KLeafDirCacheSize);
    ProcessDynamicDirCacheParams(section);	//-- read dynamic dir cache parameters;

    ProcessDirCacheParams(section); //-- read FAT directory cache parameters
    ProcessFatCacheParams(section); //-- read FAT cache parameters

    iInitialised = ETrue;

    DumpParameters(); //-- print out parameters in debug mode
    }


//-----------------------------------------------------------------------------
/** 
    process dynamic directory cache parameters 
    @param aSection section name, like "DriveX"
*/
void TFatConfig::ProcessDynamicDirCacheParams(const TDesC8& aSection)
	{
	// we have to process the data in this file as the default values are all static variables, which means
	//  their scope is limited within this file only.
    iDynamicDirCacheMaxPageSizeLog2 = KDef_MaxDynamicDirCachePageSzLog2; 
    iDynamicDirCacheSizeMinKB = ReadUint(aSection, KPN_FAT_DynamicDirCacheMin, KDef_DynamicDirCacheMin);
    iDynamicDirCacheSizeMaxKB = ReadUint(aSection, KPN_FAT_DynamicDirCacheMax, KDef_DynamicDirCacheMax);

    // if less than default values, set to default values
    if (iDynamicDirCacheSizeMinKB < KDef_DynamicDirCacheMin)
    	iDynamicDirCacheSizeMinKB = KDef_DynamicDirCacheMin;
    if (iDynamicDirCacheSizeMaxKB < KDef_DynamicDirCacheMax)
    	iDynamicDirCacheSizeMaxKB = KDef_DynamicDirCacheMax;

    // validate settings for those values that the default values does not apply onto them
    __ASSERT_ALWAYS(iDynamicDirCacheSizeMinKB <= iDynamicDirCacheSizeMaxKB, Fault(EFatBadParameter));
	}

/** 
    process directory cache parameters 
    @param aSection section name, like "DriveX"
*/
void TFatConfig::ProcessDirCacheParams(const TDesC8& aSection)
    {
    TBuf8<128>  buf;

    //-- set default values.
    iDirCacheSizeKB = KDef_KDirCacheSizeKB;
    iDirCacheMaxPageSizeLog2 = KDef_MaxDirCachePageSzLog2;

    //-- read a string containing DirCache parameters
    //-- it looks like this: FAT_DirCache cacheSzKB, maxPageSzBytes
    if(!F32Properties::GetString(aSection, KPN_FAT_DirCache, buf))
        return;
    
    GetIntTokenVal(buf, 0, iDirCacheSizeKB);          //-- 1. extract directory cache size in KB. this is token 0
    GetIntTokenVal(buf, 1, iDirCacheMaxPageSizeLog2); //-- 2. extract directory cache max page size in bytes Log2. this is token 1  
    }

//-----------------------------------------------------------------------------
/** 
    process fat cache parameters 
    @param aSection section name, like "DriveX"
*/
void TFatConfig::ProcessFatCacheParams(const TDesC8& aSection)
{
    TBuf8<128>  buf;
    //-- set default values.
    iFat16FixedCacheReadGrLog2 = KDef_FAT16_FixedCache_Read_Granularity_LOG2;
    iFat16FixedCacheWriteGrLog2 = KDef_FAT16_FixedCache_Write_Granularity_LOG2;

    iFat32LRUCacheSizeKB = KDef_FAT32_LruCache_SizeKB;
    iFat32LRUCacheReadGrLog2 = KDef_FAT32_LruCache_Read_Granularity_LOG2;
    iFat32LRUCacheWriteGrLog2 = KDef_FAT32_LruCache_Write_Granularity_LOG2;

    //-- read a string containing FAT16 fixed cache parameters
    if(F32Properties::GetString(aSection, KPN_FAT_FAT16FixedCache, buf))
    {
        GetIntTokenVal(buf, 0, iFat16FixedCacheReadGrLog2);  //-- 1. extract Log2(FAT16 fixed cache read granularity)
        GetIntTokenVal(buf, 1, iFat16FixedCacheWriteGrLog2); //-- 2. extract Log2(FAT16 fixed cache write granularity)
    }

    //-- read a string containing FAT32 LRU cache parameters
    if(F32Properties::GetString(aSection, KPN_FAT_FAT32LruCache, buf))
    {
        GetIntTokenVal(buf, 0, iFat32LRUCacheSizeKB);        //-- 1. extract FAT32 LRU cache size, Kbytes
        GetIntTokenVal(buf, 1, iFat32LRUCacheReadGrLog2);    //-- 2. extract Log2(FAT32 LRU cache read granularity)
        GetIntTokenVal(buf, 2, iFat32LRUCacheWriteGrLog2);   //-- 3. extract Log2(FAT32 LRU cache write granularity)
    }
}


//-----------------------------------------------------------------------------
/**
    Just a helper method. Reads TUint32 value from the estart.txt
    
    @param  aSection        section name, e.g. [DriveD]
    @param  aKeyName        Key name
    @param  aDefaultValue   Default value to return in parameter isn't found
    @return a value from config file or default aDefaultValue
*/
TUint32 TFatConfig::ReadUint(const TDesC8& aSection, const TDesC8& aKeyName, TUint32 aDefaultValue) const
    {
    TInt32 val = aDefaultValue;
	// coverity[check_return] coverity[unchecked_value]
    F32Properties::GetInt(aSection, aKeyName, val);

    return val;
    }

//-----------------------------------------------------------------------------
/** Debug method, prints out the parameters*/
void TFatConfig::DumpParameters() const
    {
#ifdef _DEBUG

    ASSERT(iInitialised);
    __PRINT(_L("\n\n"));
    __PRINT(_L("#>- TFatConfig parameters:\n"));

    DoDumpUintParam(KPN_ScanDrvSkipFinalisedVolume, iScanDrvSkipFinalisedVolume);
    DoDumpUintParam(KPN_FAT32_AsynchMount, iFAT32_AsynchMount);
    DoDumpUintParam(KPN_FAT32_UseFsInfoOnMount, iFAT32_UseFSInfoOnMount);
    DoDumpUintParam(KPN_FAT32_UseFatBitSupercache, iFAT32_UseBitSupercache);
    DoDumpUintParam(KPN_FAT16_UseCleanShutDownBit, iFAT16_UseCleanShutDownBit);
    DoDumpUintParam(KPN_FAT32_SyncScanThreshold, iSyncScanThresholdMB);
    

    DoDumpUintParam(_L8("FAT_DirCache Size, KB"), iDirCacheSizeKB);
    DoDumpUintParam(_L8("FAT_DirCache MaxPage Size Log2"), iDirCacheMaxPageSizeLog2);

    DoDumpUintParam(_L8("FAT_16Cache RdGr Log2"), iFat16FixedCacheReadGrLog2);
    DoDumpUintParam(_L8("FAT_16Cache WrGr Log2"), iFat16FixedCacheWriteGrLog2);

    DoDumpUintParam(_L8("FAT_32Cache Size, KB"),  iFat32LRUCacheSizeKB);
    DoDumpUintParam(_L8("FAT_32Cache RdGr Log2"), iFat32LRUCacheReadGrLog2);
    DoDumpUintParam(_L8("FAT_32Cache WrGr Log2"), iFat32LRUCacheWriteGrLog2);


    
    
    DoDumpUintParam(KPN_FAT_LeafDirCache,       iLeafDirCacheSize);
    DoDumpUintParam(KPN_FAT_DynamicDirCacheMin, iDynamicDirCacheSizeMinKB);
    DoDumpUintParam(KPN_FAT_DynamicDirCacheMax, iDynamicDirCacheSizeMaxKB);
    DoDumpUintParam(_L8("DynamicDirCacheMaxPageSizeLog2"), iDynamicDirCacheMaxPageSizeLog2);

    __PRINT(_L("#>------ end -------<#\n\n"));

#endif
    }

void TFatConfig::DoDumpUintParam(const TDesC8& aKeyName, TUint32 aParam) const
    {
#ifdef _DEBUG
    TBuf<100> buf;

    buf.Copy(aKeyName);
    buf.Insert(0,_L("#>-  "));
    buf.AppendFormat(_L(": %d"), aParam);
    __PRINT(buf);
#else
    (void)aKeyName; (void)aParam; //-- get rid of warning

#endif
    }


























