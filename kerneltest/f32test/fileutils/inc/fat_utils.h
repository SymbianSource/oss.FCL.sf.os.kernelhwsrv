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
// @file
// various FAT utilities header file
// 
//



#ifndef TEST_FAT_UTILS_HEADER
#define TEST_FAT_UTILS_HEADER

#include "f32_test_utils.h"


namespace Fat_Test_Utils
{
using namespace F32_Test_Utils;


#include "filesystem_fat.h"
using namespace FileSystem_FAT;
typedef TFatSubType TFatType;



//#############################################################################
//#     FAT-specific declarations
//#############################################################################



TFatType GetFatType(RFs &aFs, TInt aDrive);

//-----------------------------------------------------------------------------

const TInt KDefaultSectorLog2 = 9; ///< Log2 of default sector size for FAT
const TInt KDefaultSectorSize = 1 << KDefaultSectorLog2; ///< Default sector size for FAT, 512 bytes

//-----------------------------------------------------------------------------

class TFatBootSector;
class TFSInfo;

TInt  ReadBootSector(RFs &aFs, TInt aDrive, TInt64 aMediaPos, TFatBootSector& aBootSector);
TInt  WriteBootSector(RFs &aFs, TInt aDrive, TInt64 aMediaPos, const TFatBootSector& aBootSector);

TInt  ReadFSInfoSector(RFs &aFs, TInt aDrive, TInt64 aMediaPos, TFSInfo& aFsInfo);
TInt  WriteFSInfoSector(RFs &aFs, TInt aDrive, TInt64 aMediaPos, const TFSInfo& aFsInfo);

//-----------------------------------------------------------------------------


/** FAT formatting parameters structure */
struct TFatFormatParam
{
    TFatFormatParam();
        
    TFatType iFatType;          ///< explicit FAT type
    TUint    iSecPerCluster;    ///< Sectors per cluster; 0 means "default"
    TUint    iReservedSectors;  ///< Reserved Sectors; 0 means "default"
};


TInt FormatFatDrive(RFs &aFs, TInt aDrive, TBool aQuickFormat, const TFatFormatParam* apFmtParams=NULL, TBool aForceErase = EFalse); 


//-----------------------------------------------------------------------------
const TUint KFatFirstSearchCluser   =2;     ///< FAT usable clusters start from 2; FAT[0] and FAT[1] are reserved
const TInt KFatDirNameSize			=11;    ///< Dos directory/File name length
const TUint KFatDirReserved1Size	=8;     ///< Size of reserved area one in a directory enrty
const TInt KVolumeLabelSize			=11;    ///< Volume lable size
const TInt KFileSysTypeSize			=8;     ///< File system type parameter size
const TInt KVendorIdSize			=8;     ///< Vendor ID parameter size
const TUint KVFatEntryAttribute		=0x0F;  ///< VFat entry attribute setting
const TUint KBootSectorSignature	=0xAA55;///< File system Boot sector signiture
const TUint8 KDotEntryByte			=0x2e;  ///< Dot value for self and parent pointer directory entries
const TUint8 KBlankSpace			=0x20;  ///< Blank space in a directory entry
const TUint  KSizeOfFatDirEntry		=32;    ///< Size in bytes of a Fat directry entry 
const TUint  KSizeOfFatDirEntryLog2	=5;     ///< Log2 of size in bytes of a Fat directry entry 
const TUint  KFat16VolumeLabelPos 	=43;    ///< Position of volume lable in BPB for Fat12/16
const TUint  KFat32VolumeLabelPos	=71;    ///< Position of volume lable in BPB for Fat32
const TUint  KReservedIdOldEntry	=1;	    ///< used for TFatDirEntry::SetReserved2(..)
const TUint  KReservedIdNewEntry	=0;
const TInt KSizeOfFatBootSector     =90;    ///< Size in bytes of Boot sector parameter block (BPB) 

const TUint32 KBootSectorNum        =0;     ///< Main Boot Sector number (always 0)
const TUint32 KFSInfoSectorNum      =1;     ///< Main FSInfo sector number. This is a default value. The actual value shall be taken from the BPB

const TUint32 KBkBootSectorNum      =6;     ///< Backup Boot Sector number (must be 6 by default)
const TUint32 KBkFSInfoSectorNum    =KBkBootSectorNum+1;     ///< Backup FSInfo sector number, follows the backup bpb sector

typedef TBuf8<KFatDirNameSize> TShortName;  ///< Buffer type fot short names in dos entries

//#############################################################################
//#                 FAT/FAT32 Boot sector support
//#############################################################################

/**
Boot sector parameter block, enables access to all file system parameters.
Supports FAT12/16/32; code is taken from sl_bpb.h
*/
class TFatBootSector
	{
public:
    //-- simple getters / setters
	const TPtrC8 VendorId() const;
	TUint16 BytesPerSector() const;
	TInt SectorsPerCluster() const;
	TInt ReservedSectors() const;
	TInt NumberOfFats() const;
	TInt RootDirEntries() const;
	TInt TotalSectors() const;
	TUint8 MediaDescriptor() const;
	TInt FatSectors() const;
	TInt SectorsPerTrack() const;
	TInt NumberOfHeads() const;
	TInt HiddenSectors() const;
	TInt HugeSectors() const;
	TInt PhysicalDriveNumber() const;
	TInt ExtendedBootSignature() const;
	TUint32 UniqueID() const;
	const TPtrC8 VolumeLabel() const;
	const TPtrC8 FileSysType() const;
	TInt BootSectorSignature() const;
	TUint32 FatSectors32() const;	
	TUint16 FATFlags() const;		
	TUint16 VersionNumber() const;		
	TUint32 RootClusterNum() const;	
	TUint16 FSInfoSectorNum() const;
	TUint16 BkBootRecSector() const;

	void SetJumpInstruction();
	void SetVendorID(const TDesC8& aDes);
	void SetBytesPerSector(TInt aBytesPerSector);
	void SetSectorsPerCluster(TInt aSectorsPerCluster);
	void SetReservedSectors(TInt aReservedSectors);
	void SetNumberOfFats(TInt aNumberOfFats);
	void SetRootDirEntries(TInt aRootDirEntries);
	void SetTotalSectors(TInt aTotalSectors);
	void SetMediaDescriptor(TUint8 aMediaDescriptor);
	void SetFatSectors(TInt aFatSectors);
	void SetSectorsPerTrack(TInt aSectorsPerTrack);
	void SetNumberOfHeads(TInt aNumberOfHeads);
	void SetHiddenSectors(TUint32 aHiddenSectors);
	void SetHugeSectors(TUint32 aTotalSectors);
	void SetPhysicalDriveNumber(TInt aPhysicalDriveNumber);
	void SetReservedByte(TUint8 aReservedByte);
	void SetExtendedBootSignature(TInt anExtendedBootSignature);
	void SetUniqueID(TUint32 anUniqueID);
	void SetVolumeLabel(const TDesC8& aDes);
	void SetFileSysType(const TDesC8& aDes);
	void SetFatSectors32(TUint32	aFatSectors32);	
	void SetFATFlags(TUint16 aFATFlags);		
	void SetVersionNumber(TUint16	aVersionNumber);		
	void SetRootClusterNum(TUint32 aRootCusterNum);	
	void SetFSInfoSectorNum(TUint16 aFSInfoSectorNum);
	void SetBkBootRecSector(TUint16 aBkBootRecSector);

public:
    TFatBootSector();

    void Initialise();
    TBool IsValid() const;
    TFatType FatType(void) const;

    void Internalize(const TDesC8& aBuf);
    void Externalize(TDes8& aBuf) const;
	void PrintDebugInfo() const;

    //-- more advanced API, works for all FAT types
    TInt FirstFatSector() const;
	TInt RootDirStartSector() const;
	TInt FirstDataSector() const;
    
    TUint32 VolumeTotalSectorNumber() const;
    TUint32 TotalFatSectors() const;
    TUint32 RootDirSectors() const;

    TBool operator==(const TFatBootSector& aRhs);

protected:

    TUint8  iJumpInstruction[3];            ///< +0         Jump instruction used for bootable volumes
    TUint8  iVendorId[KVendorIdSize];       ///< +3         Vendor ID of the file system that formatted the volume
    TUint16 iBytesPerSector;                ///< +11/0x0b   Bytes per sector 
    TUint8  iSectorsPerCluster;             ///< +13/0x0d   Sectors per cluster ratio
    TUint16 iReservedSectors;               ///< +14/0x0e   Number of reserved sectors on the volume
    TUint8  iNumberOfFats;                  ///< +16/0x10   Number of Fats on the volume
    TUint16 iRootDirEntries;	            ///< +17/0x11   Number of entries allowed in the root directory, specific to Fat12/16, zero for FAT32
    TUint16 iTotalSectors;                  ///< +19/0x13   Total sectors on the volume, zero for FAT32
    TUint8  iMediaDescriptor;               ///< +12/0x15   Media descriptor
    TUint16 iFatSectors;                    ///< +22/0x16   Sectors used for the Fat table, zero for FAT32
    TUint16 iSectorsPerTrack;               ///< +24/0x18   Sectors per track
    TUint16 iNumberOfHeads;                 ///< +26/0x1a   Number of heads 
    TUint32 iHiddenSectors;                 ///< +28/0x1c   Number of hidden sectors in the volume
    TUint32 iHugeSectors;                   ///< +32/0x20   Total sectors in the volume, Used if totalSectors > 65535
	TUint32	iFatSectors32;                  ///< +36/0x24   Start of additional elements @ offset 36 for FAT32, Sectors in Fat table for 32 bit volume
	TUint16 iFATFlags;                      ///< +40/0x28   Fat flags
	TUint16	iVersionNumber;		            ///< +42/0x2a   Version number of the file system
	TUint32 iRootClusterNum;                ///< +44/0x2c   Cluster number of the root directory
	TUint16 iFSInfoSectorNum;               ///< +48/0x30   Sector number containing the FSIInfo structure
	TUint16 iBkBootRecSector;               ///< +50/0x32   Backup boot sector
	TUint8	iReserved2[12];                 ///< +52/0x34   Reserved space, End of Fat32 Only parameters section
	TUint8  iPhysicalDriveNumber;           ///< +64/0x40   Physical drive number, not used in Symbian OS
    TUint8  iReserved;                      ///< +65/0x41   Reserved byte
    TUint8  iExtendedBootSignature;         ///< +66/0x42   Extended boot signiture
    TUint32 iUniqueID;                      ///< +67/0x43   Unique volume ID
    TUint8  iVolumeLabel[KVolumeLabelSize]; ///< +71/0x47   The volume's label
	TUint8  iFileSysType[KFileSysTypeSize]; ///< +82/0x52   File system type
	};




//#############################################################################
//#                     FAT32 FSInfo sector support
//#############################################################################


const TUint32	KSizeOfFSInfo	    = 0x200; ///< Size in bytes of the FSInfo structures
const TUint32	KFSInfoReserved1Size= 480;   ///< Size of first reserved area in FSInfo
const TUint32	KFSInfoReserved2Size= 12;    ///< Size of second reserved area in FSInfo

//-----------------------------------------------------------------------------

/**
TFSinfo hold the File system information comprising the free cluster count
and next free cluster. It is found in the sector after the BPB and contains
several identification signitures along with resverved space. It is not
mandatory to support this feature.
*/
class TFSInfo
	{

public:
	TFSInfo();

    void  Initialise();  
    TBool IsValid() const;
    void Internalize(const TDesC8& aBuf);
    void Externalize(TDes8& aBuf) const;
	void PrintDebugInfo() const;

public:

    TUint32 FreeClusterCount() const; 
	TUint32 NextFreeCluster() const;

	void SetFreeClusterCount(TUint32 aFreeCount);
	void SetNextFreeCluster(TUint32 aNextCluster);

    TBool operator==(const TFSInfo& aRhs);

protected:
	TUint32 iLeadSig;                           ///< +0         lead in signature, should always equal 0x41615252
	TUint8	iReserved1[KFSInfoReserved1Size];   ///< +4         First reserved region of 480 bytes
	TUint32 iStructureSig;                      ///< +484/0x1e4 Structure signature, should equal 0x61417272
	TUint32 iFreeCount;                         ///< +488/0x1e8 last known free cluster count
	TUint32 iNextFree;                          ///< +492/0x1ec hint to file system as to where to start looking for free clusters
	TUint8	iReserved2[KFSInfoReserved2Size];   ///< +496/0x1f0 Second reserved region of 12 bytes
	TUint32	iTrainlingSig;                      ///< +508/0x1fc Trailing Signature (Bytes 510 and 511 = 55 and AA respectively)
	};



//#############################################################################
//#                     FAT directory entries support
//#############################################################################

const TInt KMaxFatFileName=12;
const TInt KMaxFatFileNameWithoutExt=8;
const TInt KMaxDuplicateShortName=0xFF;
const TInt KMaxVFatEntryName=13;

//-- some helper functions
TTime DosTimeToTTime(TInt aDosTime,TInt aDosDate);
TInt DosTimeFromTTime(const TTime& aTime);
TInt DosDateFromTTime(const TTime& aTime);
TBuf8<12> DosNameToStdFormat(const TDesC8& aDosName);
TBuf8<12> DosNameFromStdFormat(const TDesC8& aStdFormatName);
TInt NumberOfVFatEntries(TInt aNameLength);
TUint8 CalculateShortNameCheckSum(const TDesC8& aShortName);


/**
    Fat DOS directory entry structure
*/
struct SFatDirEntry
    {
    TUint8  iName[KFatDirNameSize]; ///< :0  File/Directory name
    TUint8  iAttributes;            ///< :11 File/Directory attributes
    TUint8  iReserved1[2];          ///< :12 2 reserved bytes(in our implementation), some versions of Windows may use them
    TUint16 iTimeC;                 ///< :14 Creation time
    TUint16 iDateC;                 ///< :16 Creation date
    TUint16 iReserved2;             ///< :18 2 reserved bytes(in our implementation), FAT specs say that this is "last access date". Rugged FAT uses them as a special entry ID
    TUint16 iStartClusterHi;        ///< :20 High 16 bits of the File/Directory cluster number (Fat32 only)
    TUint16 iTime;                  ///< :22 last write access time 
    TUint16 iDate;                  ///< :24 last write access date 
    TUint16 iStartClusterLo;        ///< :26 Low 16 bits of the File/Directory cluster number 
    TUint32 iSize;                  ///< :28 File/Directory size in bytes
    };


//-------------------------------------------------------------------------------------------------------------------

/**
Provides access to the Fat directory entry parameters
*/
class TFatDirEntry
    {
public:
    TFatDirEntry();
    void InitZ();

    const TPtrC8 Name() const;
    TUint Attributes() const;
    TTime Time() const;
    TUint32 StartCluster() const;
    TUint32 Size() const;
    TBool IsErased() const;
    TBool IsCurrentDirectory() const;
    TBool IsParentDirectory() const;
    TBool IsEndOfDirectory() const;
    TBool IsGarbage() const;
    void SetName(const TDesC8& aDes);
    void SetAttributes(TUint anAtt);
    void SetTime(TTime aTime);
    void SetCreateTime(TTime aTime);
    void SetStartCluster(TUint32 aStartCluster);
    void SetSize(TUint32 aFilesize);
    void SetErased();
    void SetCurrentDirectory();
    void SetParentDirectory();
    void SetEndOfDirectory();
    //TUint RuggedFatEntryId() const;
    //void  SetRuggedFatEntryId(TUint16 aId);

public:
    void InitializeAsVFat(TUint8 aCheckSum);
    void SetVFatEntry(const TDesC& aName,TInt aRemainderLen);
    void ReadVFatEntry(TDes16& aVBuf) const;
    TBool IsLongNameStart() const;
    TBool IsVFatEntry() const;
    TInt NumFollowing() const;
    TUint8 CheckSum() const;


public:
    TUint8 iData[KSizeOfFatDirEntry]; ///< The directory entry data
    };



}//namespace Fat_Test_Utils


#endif //TEST_FAT_UTILS_HEADER


































