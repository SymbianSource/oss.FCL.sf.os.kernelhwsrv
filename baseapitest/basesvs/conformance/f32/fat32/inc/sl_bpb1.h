/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/



#ifndef SL_BPB1_H
#define SL_BPB1_H

#include <f32fsys.h>
#include <f32ver.h>
#include <f32dbg.h>
#include <e32svr.h>

#pragma warning (disable:4103) //  : used #pragma pack to change alignment

static const TInt KFatDirNameSize=11;
static const TInt KFatDirReservedSize=10;
static const TInt KVolumeLabelSize=11;
static const TInt KFileSysTypeSize=8;
static const TInt KVendorIdSize=8;
static const TInt KVFatEntryAttribute=0x0F;
static const TInt KBootSectorSignature=0xAA55;

class TFatDirEntry;
enum TFault
	{
	EVFatNoLongName,
	EFatChkDskFailed,
	EFatBadParameter,
	EFatCacheBadRead,
	EFatCacheNotFatSector,
	EFatBadLocalDrive,
	EFatBadBootSectorParameter,
	EFatBadDirEntryParameter,
	EFatAddClusterNotLastInList,
	EFatBadEntryValue,
	EFatBadClusterValue,
	EFatBadStdFormatName,
	EFatBadDosFormatName,
	EFatCorrupt,
	EFatChkDskBitListOverFlow,
	EFatChkDskIndexOverFlow,
	EFatChkDskIllegalClusterNumber,
	EFatChkDskClusterAlreadyInUse,
	EFatChkDskBadCluster,
	EFatChkDskInvalidEntrySize,
	EFatDesTooBig,
	EFatFilePosBeyondEnd,
	EFatFileSeekIndexTooSmall,
	EFatFileSeekIndexTooSmall2,
	EFatDataAllocBadSeg,
	EFatFileSystemBadMemory,
	EFatFileSystemCreate1,
	EFatFileSystemCreate2,
	EFatFileSystemCreate3,
	ECacheAllocatorSetMax,
	EFatLruCacheBadGranularity,
	EFatFixedCacheBadGranularity,
	EFatFixedCacheBadCall,
	EFatRawReadTooBig,
	EFatRawWriteTooBig,
	EWinsBadRamDriveRead,
	EWinsBadRamDriveWrite,
	EFatReadUidFailed,
	ELruCacheFlushNotSupported
	};

struct SFatDirEntry
	{
    TUint8 iName[KFatDirNameSize];
    TUint8 iAttributes;
    TUint8 iReserved[KFatDirReservedSize];
    TUint16 iTime;
    TUint16 iDate;
    TUint16 iStartCluster;
    TUint32 iSize;
	};	
#define pDir ((SFatDirEntry*)&iData[0])

//
typedef TBuf8<KFatDirNameSize> TShortName;
//
const TInt KSizeOfFatBootSector=62;
const TInt KSizeOfFatDirEntry=32;
class TFatBootSector
	{
public:
	inline const TPtrC8 VendorId() const;
	inline TInt BytesPerSector() const{return iBytesPerSector;}
	inline TInt SectorsPerCluster() const{return iSectorsPerCluster;}
	inline TInt ReservedSectors() const {return iReservedSectors;};
	inline TInt NumberOfFats() const {return iNumberOfFats;}
	inline TInt RootDirEntries() const{return iRootDirEntries;}
	inline TInt TotalSectors() const{return iTotalSectors;}
	inline TUint8 MediaDescriptor() const {return iMediaDescriptor;}
	inline TInt FatSectors() const {return iFatSectors;}
	inline TInt SectorsPerTrack() const {return iSectorsPerTrack;}
	inline TInt NumberOfHeads() const{return iNumberOfHeads;}
	inline TInt HiddenSectors() const {return iHiddenSectors;}
	inline TInt HugeSectors() const{return iHugeSectors;}
	inline TInt PhysicalDriveNumber() const;
	inline TInt ExtendedBootSignature() const;
	inline TUint32 UniqueID() const;
	inline const TPtrC8 VolumeLabel() const;
	inline const TPtrC8 FileSysType() const {return TPtrC8(iFileSysType,KFileSysTypeSize);}
	inline TInt BootSectorSignature() const;
	inline void SetJumpInstruction();
	inline void SetVendorID(const TDesC8& aDes);
	inline void SetBytesPerSector(TInt aBytesPerSector);
	inline void SetSectorsPerCluster(TInt aSectorsPerCluster);
	inline void SetReservedSectors(TInt aReservedSectors);
	inline void SetNumberOfFats(TInt aNumberOfFats);
	inline void SetRootDirEntries(TInt aRootDirEntries);
	inline void SetTotalSectors(TInt aTotalSectors);
	inline void SetMediaDescriptor(TUint8 aMediaDescriptor);
	inline void SetFatSectors(TInt aFatSectors);
	inline void SetSectorsPerTrack(TInt aSectorsPerTrack);
	inline void SetNumberOfHeads(TInt aNumberOfHeads);
	inline void SetHiddenSectors(TUint32 aHiddenSectors);
	inline void SetHugeSectors(TUint32 aTotalSectors);
	inline void SetPhysicalDriveNumber(TInt aPhysicalDriveNumber);
	inline void SetReservedByte(TUint8 aReservedByte);
	inline void SetExtendedBootSignature(TInt anExtendedBootSignature);
	inline void SetUniqueID(TUint32 anUniqueID);
	inline void SetVolumeLabel(const TDesC8& aDes);
	inline void SetFileSysType(const TDesC8& aDes);
	
public:
	inline TBool Is16BitFat() {return(FileSysType()==_L8("FAT16   "));}
	TInt FirstFatSectorPos()
	{
	return(ReservedSectors()*BytesPerSector());
	}
	inline TInt RootDirStartSector();
	inline TInt FirstFreeSector();
public:
    TUint8  iJumpInstruction[3];        ///< Jump instruction used for bootable volumes
    TUint8  iVendorId[KVendorIdSize];   ///< Vendor ID of the file system that formatted the volume
    TUint16 iBytesPerSector;            ///< Bytes per sector 
    TUint8  iSectorsPerCluster;         ///< Sectors per cluster ratio
    TUint16 iReservedSectors;           ///< Number of reserved sectors on the volume
    TUint8  iNumberOfFats;              ///< Number of Fats on the volume
    TUint16 iRootDirEntries;	        ///< Number of entries allowed in the root directory, specific to Fat12/16, zero for FAT32
    TUint16 iTotalSectors;              ///< Total sectors on the volume, zero for FAT32
    TUint8  iMediaDescriptor;           ///< Media descriptor
    TUint16 iFatSectors;                ///< Sectors used for the Fat table, zero for FAT32
    TUint16 iSectorsPerTrack;           ///< Sectors per track
    TUint16 iNumberOfHeads;             ///< Number of heads 
    TUint32 iHiddenSectors;             ///< Number of hidden sectors in the volume
    TUint32 iHugeSectors;               ///< Total sectors in the volume, Used if totalSectors > 65535
	TUint32	iFatSectors32;              ///< Start of additional elements @ offset 36 for FAT32, Sectors in Fat table for 32 bit volume
	TUint16 iFATFlags;                  ///< Fat flags
	TUint16	iVersionNumber;		        ///< Version number of the file system
	TUint32 iRootClusterNum;            ///< Cluster number of the root directory
	TUint16 iFSInfoSectorNum;           ///< Sector number containing the FSIInfo structure
	TUint16 iBkBootRecSector;           ///< Backup boot sector
	TUint8	iReserved2[12];             ///< Reserved space, End of Fat32 Only parameters section
	TUint8  iPhysicalDriveNumber;       ///< Physical drive number, not used in  OS
    TUint8  iReserved;                  ///< Reserved byte
    TUint8  iExtendedBootSignature;     ///< Extended boot signiture
    TUint32 iUniqueID;                  ///< Unique volume ID
    TUint8  iVolumeLabel[KVolumeLabelSize];  ///< 	The volume's label
	TUint8  iFileSysType[KFileSysTypeSize];  ///< 	File system type
	};
	
void Fault(TAny* aTestStep,TFault anError);
GLREF_C TBool IsPowerOfTwo(TInt aNum);
GLREF_C TInt Log2(TInt aNum);
GLREF_C TTime DosTimeToTTime(TInt aDosTime,TInt aDosDate);
GLREF_C TInt DosTimeFromTTime(const TTime& aTime);
GLREF_C TInt DosDateFromTTime(const TTime& aTime);

class TFatDirEntry
	{
public:
	const TPtrC8 Name() const
	{return TPtrC8((TUint8*)&(pDir->iName),11);
	}
	TInt Attributes() const 
	{
	return pDir->iAttributes;
	}
	inline TTime Time() const;
	TInt StartCluster() const
	{
	return pDir->iStartCluster;
	}
	inline TUint32 Size() const;
	TBool IsErased() const
	{
	return (TBool)(iData[0]==0xe5);
	}
	inline TBool IsCurrentDirectory() const;
	inline TBool IsParentDirectory() const;
	TBool IsEndOfDirectory() const 
	{
	return (TBool)(iData[0]==0x00);
	}
	TBool IsGarbage() const;
	inline void SetName(const TDesC8& aDes);
	inline void SetAttributes(TInt anAtt);
	inline void SetTime(const TTime& aTime);
	void SetStartCluster(TInt aStartCluster)
	{
	__ASSERT_DEBUG(!(aStartCluster&~KMaxTUint16),Fault( this,EFatBadDirEntryParameter));
	pDir->iStartCluster=(TUint16)aStartCluster;
	}
	inline void SetSize(TUint32 aFilesize);
	inline void SetErased();
	inline void SetCurrentDirectory();
	inline void SetParentDirectory();
	inline void SetEndOfDirectory();
	inline void ZeroReserved();
public:
	void InitializeRootEntry();
	void InitializeAsVFat(TUint8 aCheckSum);
	void SetVFatEntry(const TDesC& aName,TInt aRemainderLen);
	void ReadVFatEntry(TDes16& aVBuf);
	inline TBool IsLongNameStart() const;
	TBool IsVFatEntry() const 
	{
	return (TBool)(Attributes()==0x0F && IsEndOfDirectory()==EFalse);
	}
	inline TInt NumFollowing() const;
public:
	TUint8 iData[KSizeOfFatDirEntry];
	};

#endif // SL_BPB1_H
