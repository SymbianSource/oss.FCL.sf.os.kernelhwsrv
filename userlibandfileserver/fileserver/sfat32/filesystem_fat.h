// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Public header file for "FAT" file system. Contains this file system name and optional file system - specific declarations.
//
//
//


/**
 @file
 @publishedAll
 @released
*/

#if !defined(__FILESYSTEM_FAT_H__)
#define __FILESYSTEM_FAT_H__


#if !defined(__F32FILE_H__)
#include <f32file.h>
#endif



/**
    FAT filesystem name, which shall be provided to RFs::MountFileSystem() and is returned by RFs::FileSystemName() if 
    this file system is mounted on the drive. The literal is case-insensitive.
    @see RFs::MountFileSystem()
    @see RFs::FileSystemName()
*/
_LIT(KFileSystemName_FAT, "FAT");

/**
    FAT file system subtypes, literal values. These values are returned by RFs::FileSystemSubType().
    The literals are case-insensitive.
    File sytem "FAT" mounted on the drive can be one of the FAT12/FAT16/FAT32

    @see RFs::::FileSystemSubType()
*/
_LIT(KFSSubType_FAT12, "FAT12"); ///< corresponds to FAT12
_LIT(KFSSubType_FAT16, "FAT16"); ///< corresponds to FAT16   
_LIT(KFSSubType_FAT32, "FAT32"); ///< corresponds to FAT32

//------------------------------------------------------------------------------

namespace FileSystem_FAT
{

    /** Numeric representation of FAT file system sub types */
    enum TFatSubType
        {
        EInvalid = 0,       ///< invalid terminal value
        ENotSpecified = 0,  ///< not specified

        EFat12  = 12,   ///< corresponds to FAT12
        EFat16  = 16,   ///< corresponds to FAT16
        EFat32  = 32    ///< corresponds to FAT32
        };


const TUint64 KMaxSupportedFatFileSize = 0xFFFFFFFF; ///< theoretical maximum file size supported by all FAT filesystems (4GB-1)

//------------------------------------------------------------------------------

/** 
    This class describes specific parameters for formatting volume with FAT file system.
    The parameters are: FAT sub type (FAT12/16/32), Number of Sectors per cluster, Number of FAT tables, Number of reserved sectors.
    All parameters are optional and if not set, it is up to the file system implementation to decide values.

    This class package (TVolFormatParam_FATBuf) shall be passed to the RFormat::Open() as "Special format information"

    Please note that the parameters may have invalid combinations and it is not always possible to format volume with the specified
    FAT sub type, like FAT12. In this case RFormat::Open() will return corresponding error code (the concrete code depends on file system implementation).

    RFormat::Open() does not modify any data in this structure.

    @see TVolFormatParam_FATBuf
    @see RFormat::Open()
*/ 
class TVolFormatParam_FAT : public TVolFormatParam
{
 public:    
    inline TVolFormatParam_FAT();
    inline void Init();

    inline void SetFatSubType(TFatSubType aSubType);
    inline void SetFatSubType(const TDesC& aSubType);
    inline TFatSubType FatSubType() const;
    //--
    inline void SetSectPerCluster(TUint32 aSpc);
    inline TUint32 SectPerCluster() const;
    //--
    inline void SetNumFATs(TUint32 aNumFATs);
    inline TUint32 NumFATs() const;

    //--
    inline void SetReservedSectors(TUint32 aReservedSectors);
    inline TUint32 ReservedSectors() const;


 private:
    void SetFileSystemName(const TDesC& aFsName);
    
    enum ///< offsets of the data units in parent class container
        {
        KOffsetSubType =0,  //-- 0
        KOffsetReservedSec, //-- 1
        KOffsetSpc,         //-- 2  !! do not change this offset. 
        KOffsetNumFATs,     //-- 3  !! do not change this offset. 
        
        };

}; //TVolFormatParam_FAT


/**
    TVolFormatParam_FAT package buffer to be passed to RFormat::Open().
    @see TVolFormatParam_FAT
    @see RFormat::Open()
*/ 
typedef TPckgBuf<TVolFormatParam_FAT> TVolFormatParam_FATBuf;



//------------------------------------------------------------------------------
//-- inline functions 
//------------------------------------------------------------------------------

TVolFormatParam_FAT::TVolFormatParam_FAT() : TVolFormatParam() 
    {
     __ASSERT_COMPILE(sizeof(TVolFormatParam_FAT) == sizeof(TVolFormatParam));
     __ASSERT_COMPILE(KOffsetSpc == 2);
     __ASSERT_COMPILE(KOffsetNumFATs == 3);
     
     Init();
    }

//------------------------------------------------------------------------------
/** initialises the data structure with default values for all parameters and automatically sets file system name as "FAT" */
void TVolFormatParam_FAT::Init() 
    {
    TVolFormatParam::Init(); 
    TVolFormatParam::SetFileSystemName(KFileSystemName_FAT);
    }

//------------------------------------------------------------------------------
/**
    Set desired FAT subtype. 
    @param  aSubType specifies FAT12/16/32 subtype. Value 0 means "the file system will decide itself what to use"
*/
void TVolFormatParam_FAT::SetFatSubType(TFatSubType aSubType)
    {
    ASSERT(aSubType == ENotSpecified || aSubType == EFat12 || aSubType == EFat16 || aSubType == EFat32);
    SetVal(KOffsetSubType, aSubType);
    }

//------------------------------------------------------------------------------
/**
    Set desired FAT subtype using string literals, @see KFSSubType_FAT12, @see KFSSubType_FAT16, @see KFSSubType_FAT32               
    @param  aSubType    string descriptor, like "FAT16"
*/
void TVolFormatParam_FAT::SetFatSubType(const TDesC& aSubType)
    {
    TFatSubType fatType = ENotSpecified;

    if(aSubType.CompareF(KFSSubType_FAT12) == 0)
        fatType = EFat12;
    else if(aSubType.CompareF(KFSSubType_FAT16) == 0)
        fatType = EFat16;
    else if(aSubType.CompareF(KFSSubType_FAT32) == 0)
        fatType = EFat32;
    else
        { ASSERT(0);}


        SetFatSubType(fatType);
    }

//------------------------------------------------------------------------------
/** @return FAT sub type value, which is set by SetFatSubType()*/
TFatSubType TVolFormatParam_FAT::FatSubType() const 
    {
    return (TFatSubType)GetVal(KOffsetSubType);
    }

//------------------------------------------------------------------------------
/**
    Set Number of "Sectors per cluster". For valid values see FAT specs.
    @param  aSpc    Number of "Sectors per cluster". Value 0 means "the file system will decide itself what to use"       
*/
void TVolFormatParam_FAT::SetSectPerCluster(TUint32 aSpc)
    {
    SetVal(KOffsetSpc, aSpc);
    }

//------------------------------------------------------------------------------
/** @return value previously set by SetSectPerCluster() */
TUint32 TVolFormatParam_FAT::SectPerCluster() const 
    {
    return GetVal(KOffsetSpc);
    }

//------------------------------------------------------------------------------
/**
    Set Number of FAT tables on the volume. The maximum is supported by the FAT FS implementation is 2
    @param  aNumFATs    Number of FAT tables. Value 0 means "the file system will decide itself what to use"       
*/
void TVolFormatParam_FAT::SetNumFATs(TUint32 aNumFATs) 
    {
    SetVal(KOffsetNumFATs, aNumFATs);
    }

//------------------------------------------------------------------------------
/** @return value previously set by SetNumFATs() */
TUint32 TVolFormatParam_FAT::NumFATs() const 
    {
    return GetVal(KOffsetNumFATs);
    } 

//------------------------------------------------------------------------------
/**
    Set number of reserved sectors on FAT volume. The file system will validate this parameter before formatting.
    @param  aReservedSectors  number of reserved sectors. Value 0 means "the file system will decide itself what to use"       
*/
void TVolFormatParam_FAT::SetReservedSectors(TUint32 aReservedSectors)
    {
    SetVal(KOffsetReservedSec, aReservedSectors);
    }

//------------------------------------------------------------------------------
/** @return value previously set by SetReservedSectors() */
TUint32 TVolFormatParam_FAT::ReservedSectors() const 
    {
    return GetVal(KOffsetReservedSec);
    } 




//------------------------------------------------------------------------------



}//namespace FileSystem_FAT






#endif //__FILESYSTEM_FAT_H__















