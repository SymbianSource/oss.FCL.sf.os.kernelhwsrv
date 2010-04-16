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
// f32\sfat32\inc\sl_bpb.h
// @file
// @internalTechnology
// 
//



#ifndef SL_BPB_H 
#define SL_BPB_H

#include "filesystem_fat.h"
using FileSystem_FAT::TFatSubType;
typedef TFatSubType TFatType;


const TInt KVolumeLabelSize			=11;    ///< Volume lable size
const TInt KFileSysTypeSize			=8;     ///< File system type parameter size
const TInt KVendorIdSize			=8;     ///< Vendor ID parameter size
const TUint16 KBootSectorSignature	=0xAA55;///< File system Boot sector signiture
const TInt KFat16VolumeLabelPos 	=43;    ///< Position of volume lable in BPB for Fat12/16
const TInt KFat32VolumeLabelPos		=71;    ///< Position of volume lable in BPB for Fat32
const TInt KSizeOfFatBootSector     = 90;   ///< Size in bytes of Boot sector parameter block (BPB) 

const TUint32 KBootSectorNum        =0;     ///< Main Boot Sector number (always 0)
const TUint32 KFSInfoSectorNum      =1;     ///< Main FSInfo sector number. This is a default value. The actual value shall be taken from the BPB
const TUint32 KReservedBootSectorNum=2;		///< Reserved Boot Sector (always 2)

const TUint32 KBkBootSectorNum      =6;     ///< Backup Boot Sector number (must be 6 by default)
const TUint32 KBkFSInfoSectorNum    =KBkBootSectorNum+1;     ///< Backup FSInfo sector number, follows the backup bpb sector
const TUint32 KBkReservedBootSectorNum=KBkBootSectorNum+2;	///< Backup Reserved Boot Sector number, follows the backup FSInfo sector number



//-------------------------------------------------------------------------------------------------------------------

/**
Boot sector parameter block, enables access to all file system parameters.
Data is populated at mount time from the BPB sector
*/
class TFatBootSector
	{
public:
	//-- simple getters / setters
    inline const TPtrC8 VendorId() const;
	inline TUint16 BytesPerSector() const;
	inline TUint8 SectorsPerCluster() const;
	inline TUint16 ReservedSectors() const;
	inline TUint8 NumberOfFats() const;
	inline TUint16 RootDirEntries() const;
	inline TUint16 TotalSectors() const;
	inline TUint8 MediaDescriptor() const;
	inline TUint16 FatSectors() const;
	inline TUint16 SectorsPerTrack() const;
	inline TUint16 NumberOfHeads() const;
	inline TUint32 HiddenSectors() const;
	inline TUint32 HugeSectors() const;
	inline TUint8 PhysicalDriveNumber() const;
	inline TUint8 ExtendedBootSignature() const;
	inline TUint32 UniqueID() const;
	inline const TPtrC8 VolumeLabel() const;
	inline const TPtrC8 FileSysType() const;
	inline TUint16 BootSectorSignature() const;
	inline TUint32 FatSectors32() const;	
	inline TUint16 FATFlags() const;		
	inline TUint16 VersionNumber() const;		
	inline TUint32 RootClusterNum() const;	
	inline TUint16 FSInfoSectorNum() const;
	inline TUint16 BkBootRecSector() const;

	inline void SetJumpInstruction();
	inline void SetVendorID(const TDesC8& aDes);
	inline void SetBytesPerSector(TUint16 aBytesPerSector);
	inline void SetSectorsPerCluster(TUint aSectorsPerCluster);
	inline void SetReservedSectors(TUint aReservedSectors);
	inline void SetNumberOfFats(TUint8 aNumberOfFats);
	inline void SetRootDirEntries(TUint16 aRootDirEntries);
	inline void SetTotalSectors(TUint aTotalSectors);
	inline void SetMediaDescriptor(TUint8 aMediaDescriptor);
	inline void SetFatSectors(TUint aFatSectors);
	inline void SetSectorsPerTrack(TUint16 aSectorsPerTrack);
	inline void SetNumberOfHeads(TUint16 aNumberOfHeads);
	inline void SetHiddenSectors(TUint32 aHiddenSectors);
	inline void SetHugeSectors(TUint32 aTotalSectors);
	inline void SetPhysicalDriveNumber(TInt aPhysicalDriveNumber);
	inline void SetReservedByte(TUint8 aReservedByte);
	inline void SetExtendedBootSignature(TUint8 anExtendedBootSignature);
	inline void SetUniqueID(TUint32 anUniqueID);
	inline void SetVolumeLabel(const TDesC8& aDes);
	inline void SetFileSysType(const TDesC8& aDes);
	inline void SetFatSectors32(TUint32	aFatSectors32);	
	inline void SetFATFlags(TUint16 aFATFlags);		
	inline void SetVersionNumber(TUint16	aVersionNumber);		
	inline void SetRootClusterNum(TUint32 aRootCusterNum);	
	inline void SetFSInfoSectorNum(TUint16 aFSInfoSectorNum);
	inline void SetBkBootRecSector(TUint16 aBkBootRecSector);

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




//-------------------------------------------------------------------------------------------------------------------

const TUint32	KSizeOfFSInfo	    = 0x200; ///< Size in bytes of the FSInfo structures
const TUint32	KFSInfoReserved1Size= 480;   ///< Size of first reserved area in FSInfo
const TUint32	KFSInfoReserved2Size= 12;    ///< Size of second reserved area in FSInfo

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

    inline TUint32 FreeClusterCount() const;
	inline TUint32 NextFreeCluster() const;

	inline void SetFreeClusterCount(TUint32 aFreeCount);
	inline void SetNextFreeCluster(TUint32 aNextCluster);

protected:
	TUint32 iLeadSig;                           ///< +0         lead in signature, should always equal 0x41615252
	TUint8	iReserved1[KFSInfoReserved1Size];   ///< +4         First reserved region of 480 bytes
	TUint32 iStructureSig;                      ///< +484/0x1e4 Structure signature, should equal 0x61417272
	TUint32 iFreeCount;                         ///< +488/0x1e8 last known free cluster count
	TUint32 iNextFree;                          ///< +492/0x1ec hint to file system as to where to start looking for free clusters
	TUint8	iReserved2[KFSInfoReserved2Size];   ///< +496/0x1f0 Second reserved region of 12 bytes
	TUint32	iTrainlingSig;                      ///< +508/0x1fc Trailing Signature (Bytes 510 and 511 = 55 and AA respectively)
	};



#endif //SL_BPB_H
