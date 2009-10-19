// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Inline methods of TFatBootSector class.
// 
//

/**
 @file
 @internalTechnology
 
 Returns Sectors in Fat table for 32 bit volume
 
 @return iFatSectors32
*/
inline TUint32 TFatBootSector::FatSectors32() const	
	{return iFatSectors32;}
/**
Fat flags

@return iFATFlags
*/
inline TUint16 TFatBootSector::FATFlags() const		
	{return iFATFlags;}
/**
Version number of the file system

@return iVersionNumber
*/
inline TUint16 TFatBootSector::VersionNumber() const		
	{return iVersionNumber;}
/**
Cluster number of the root directory

@return iRootClusterNum
*/
inline TUint32 TFatBootSector::RootClusterNum() const	
	{return iRootClusterNum;}
/**
Sector number containing the FSIInfo structure

@return iFSInfoSectorNum
*/
inline TUint16 TFatBootSector::FSInfoSectorNum() const
	{return iFSInfoSectorNum;}
/**
Backup boot sector

@return iBkBootRecSector
*/
inline TUint16 TFatBootSector::BkBootRecSector() const
	{return iBkBootRecSector;}
/**
Sets the number of sectors in Fat table for 32 bit volume

@param aFatSectors32
*/
inline void TFatBootSector::SetFatSectors32(TUint32	aFatSectors32)
	{iFatSectors32 = aFatSectors32;}
/**
Sets the Fat flags

@param aFATFlags
*/
inline void TFatBootSector::SetFATFlags(TUint16 aFATFlags)
	{iFATFlags = aFATFlags;}
/**
Sets the version number of the file system

@param aVersionNumber
*/
inline void TFatBootSector::SetVersionNumber(TUint16 aVersionNumber)
	{iVersionNumber = aVersionNumber;}
/**
Sets the cluster number of the root directory

@param aRootClusterNum
*/
inline void TFatBootSector::SetRootClusterNum(TUint32 aRootClusterNum)	
	{iRootClusterNum = aRootClusterNum;}
/**
Set the sector number containing the FSIInfo structure

@param aFSInfoSectorNum
*/
inline void TFatBootSector::SetFSInfoSectorNum(TUint16 aFSInfoSectorNum)
	{iFSInfoSectorNum = aFSInfoSectorNum;}
/**
Set the backup boot sector

@param aBkBootRecSector
*/
inline void TFatBootSector::SetBkBootRecSector(TUint16 aBkBootRecSector)
	{iBkBootRecSector = aBkBootRecSector;}	
/**
Quick test as to whether the volume is Fat32

@return True for Fat32
*/
inline TBool TFatBootSector::Is32BitFat()
	{return(iRootDirEntries==0);}
/**
Returns the vendor ID of the file system that formatted the volume

@return A descriptor containing the vendor ID 
*/
inline const TPtrC8 TFatBootSector::VendorId() const
	{return TPtrC8(iVendorId,KVendorIdSize);}
/**
Return the bytes per sector

@return iBytesPerSector
*/
inline TUint16 TFatBootSector::BytesPerSector() const
	{return iBytesPerSector;}
/**
Returns the sectors per cluster ratio

@return iSectorsPerCluster
*/
inline TInt TFatBootSector::SectorsPerCluster() const
	{return iSectorsPerCluster;}
/**
Returns the number of reserved sectors on the volume

@return iReservedSectors
*/
inline TInt TFatBootSector::ReservedSectors() const
	{return iReservedSectors;}
/**
Returns the number of Fats on the volume

@return iNumberOfFats
*/
inline TInt TFatBootSector::NumberOfFats() const
	{return iNumberOfFats;}
/**
Returns the number of entries allowed in the root directory, specific to Fat12/16, zero for FAT32

@return iRootDirEntries
*/
inline TInt TFatBootSector::RootDirEntries() const
	{return iRootDirEntries;}
/**
Returns the total sectors on the volume, zero for FAT32

@return iTotalSectors
*/
inline TInt TFatBootSector::TotalSectors() const
	{return iTotalSectors;}
/**
Returns the media descriptor

@return iMediaDescriptor
*/
inline TUint8 TFatBootSector::MediaDescriptor() const
	{return iMediaDescriptor;}
/**
Returns sectors used for the Fat table, zero for FAT32

@return iFatSectors
*/
inline TInt TFatBootSector::FatSectors() const
	{return iFatSectors;}
/**
Returns sectors per track

@return iSectorsPerTrack
*/
inline TInt TFatBootSector::SectorsPerTrack() const
	{return iSectorsPerTrack;}
/**
Returns the number of heads 

@return iNumberOfHeads
*/
inline TInt TFatBootSector::NumberOfHeads() const
	{return iNumberOfHeads;}
/**
Returns the number of hidden sectors in the volume

@return iHiddenSectors
*/
inline TInt TFatBootSector::HiddenSectors() const
	{return iHiddenSectors;}
/**
Returns total sectors in the volume, Used if totalSectors > 65535

@return iHugeSectors
*/
inline TInt TFatBootSector::HugeSectors() const
	{return iHugeSectors;}
/**
Returns the physical drive number, not used in Symbian OS

@return iPhysicalDriveNumber
*/
inline TInt TFatBootSector::PhysicalDriveNumber() const
	{return iPhysicalDriveNumber;}
/**
Returns the extended boot signiture

@return iExtendedBootSignature
*/
inline TInt TFatBootSector::ExtendedBootSignature() const
	{return iExtendedBootSignature;}
/**
Returns the unique volume ID

@return iUniqueID
*/
inline TUint32 TFatBootSector::UniqueID() const
	{return iUniqueID;}
/**
Returns the volume's label

@return A descriptor containing the volume label
*/
inline const TPtrC8 TFatBootSector::VolumeLabel() const
	{return TPtrC8(iVolumeLabel,KVolumeLabelSize);}
/**
Returns the file system type

@return A descriptor containing the file system type
*/
inline const TPtrC8 TFatBootSector::FileSysType() const
	{return TPtrC8(iFileSysType,KFileSysTypeSize);}
/**
Returns the boot sector signiture

@return KBootSectorSignature
*/
inline TInt TFatBootSector::BootSectorSignature() const
	{return KBootSectorSignature;}
/**
Set the jump instruction 
*/
inline void TFatBootSector::SetJumpInstruction()
	{iJumpInstruction[0]=0xE9;iJumpInstruction[2]=0x90;}
/**
Set the vendor ID of the file system that formatted the volume

@param aDes Descriptor containing the Vendor ID
*/
inline void TFatBootSector::SetVendorID(const TDesC8& aDes)
	{
	__ASSERT_DEBUG(aDes.Length()<=KVendorIdSize,User::Panic(_L("FATFILESYS"),1));
	TPtr8 buf(iVendorId,KVendorIdSize);
	buf=aDes;
	}
/**
Sets the bytes per sector 

@param aBytesPerSector Number of bytes per sector
*/
inline void TFatBootSector::SetBytesPerSector(TInt aBytesPerSector)
	{
	__ASSERT_DEBUG(!(aBytesPerSector&~KMaxTUint16),User::Panic(_L("FATFILESYS"),1));
	iBytesPerSector=(TUint16)aBytesPerSector;
	}
/**
Set the sectors per cluster ratio

@param aSectorsPerCluster Number of sectors per cluster
*/
inline void TFatBootSector::SetSectorsPerCluster(TInt aSectorsPerCluster)
	{
	__ASSERT_DEBUG(!(aSectorsPerCluster&~KMaxTUint8),User::Panic(_L("FATFILESYS"),1));
	iSectorsPerCluster=(TUint8)aSectorsPerCluster;
	}
/**
Sets the number of reserved sectors on the volume

@param aReservedSectors Number of reserved sectors
*/
inline void TFatBootSector::SetReservedSectors(TInt aReservedSectors)
	{
	__ASSERT_DEBUG(!(aReservedSectors&~KMaxTUint16),User::Panic(_L("FATFILESYS"),1));
	iReservedSectors=(TUint16)aReservedSectors;
	}
/**
Sets the number of Fats on the volume

@param aNumberOfFats Number of fats
*/
inline void TFatBootSector::SetNumberOfFats(TInt aNumberOfFats)
	{
	__ASSERT_DEBUG(!(aNumberOfFats&~KMaxTUint8),User::Panic(_L("FATFILESYS"),1));
	iNumberOfFats=(TUint8)aNumberOfFats;
	}
/**
Number of entries allowed in the root directory, specific to Fat12/16, zero for FAT32

@param aRootDirEntries
*/
inline void TFatBootSector::SetRootDirEntries(TInt aRootDirEntries)
	{
	__ASSERT_DEBUG(!(aRootDirEntries&~KMaxTUint16),User::Panic(_L("FATFILESYS"),1));
	iRootDirEntries=(TUint16)aRootDirEntries;
	}
/**
Total sectors on the volume, zero for FAT32

@param aTotalSectors Total number of sectors
*/
inline void TFatBootSector::SetTotalSectors(TInt aTotalSectors)
	{
	__ASSERT_DEBUG(!(aTotalSectors&~KMaxTUint16),User::Panic(_L("FATFILESYS"),1));
	iTotalSectors=(TUint16)aTotalSectors;
	}
/**
Set the media descriptor

@param aMediaDescriptor
*/
inline void TFatBootSector::SetMediaDescriptor(TUint8 aMediaDescriptor)
	{iMediaDescriptor=aMediaDescriptor;}
/**
Sectors used for the Fat table, zero for FAT32

@param aFatSectors Number of Fat sectors
*/
inline void TFatBootSector::SetFatSectors(TInt aFatSectors)
	{
	__ASSERT_DEBUG(!(aFatSectors&~KMaxTUint16),User::Panic(_L("FATFILESYS"),1));
	iFatSectors=(TUint16)aFatSectors;
	}
/**
Set the sectors per track

@param aSectorsPerTrack Number of sectors per track
*/
inline void TFatBootSector::SetSectorsPerTrack(TInt aSectorsPerTrack)
	{
	__ASSERT_DEBUG(!(aSectorsPerTrack&~KMaxTUint16),User::Panic(_L("FATFILESYS"),1));
	iSectorsPerTrack=(TUint16)aSectorsPerTrack;
	}
/**
Set the number of heads

@param aNumberOfHeads Number of heads
*/
inline void TFatBootSector::SetNumberOfHeads(TInt aNumberOfHeads)
	{
	__ASSERT_DEBUG(!(aNumberOfHeads&~KMaxTUint16),User::Panic(_L("FATFILESYS"),1));
	iNumberOfHeads=(TUint16)aNumberOfHeads;
	}
/**
Set the number of hidden sectors in the volume

@param aHiddenSectors Number of hidden sectors
*/
inline void TFatBootSector::SetHiddenSectors(TUint32 aHiddenSectors)
	{
	iHiddenSectors=(TUint32)(aHiddenSectors);
	}
/**
Set the total sectors in the volume, Used if totalSectors > 65535

@param aHugeSectors
*/
inline void TFatBootSector::SetHugeSectors(TUint32 aHugeSectors)
	{iHugeSectors=aHugeSectors;}
/**
Physical drive number, not used in Symbian OS

@param aPhysicalDriveNumber Physical drive number 
*/
inline void TFatBootSector::SetPhysicalDriveNumber(TInt aPhysicalDriveNumber)
	{
	__ASSERT_DEBUG(!(aPhysicalDriveNumber&~KMaxTUint8),User::Panic(_L("FATFILESYS"),1));
	iPhysicalDriveNumber=(TUint8)aPhysicalDriveNumber;
	}
/**
Set the reserved byte value

@param aReservedByte Value for reserved byte
*/
inline void TFatBootSector::SetReservedByte(TUint8 aReservedByte)
	{iReserved=aReservedByte;}
/**
Set the extended boot signiture

@param anExtendedBootSignature The extended boot signiture
*/
inline void TFatBootSector::SetExtendedBootSignature(TInt anExtendedBootSignature)
	{
	__ASSERT_DEBUG(!(anExtendedBootSignature&~KMaxTUint8),User::Panic(_L("FATFILESYS"),1));
	iExtendedBootSignature=(TUint8)anExtendedBootSignature;
	}
/**
Set the unique volume ID

@param anUniqueID Set the unique ID
*/
inline void TFatBootSector::SetUniqueID(TUint32 anUniqueID)
	{iUniqueID=anUniqueID;}
/**
Set the volume's label

@param aDes A descriptor containg the volume label
*/
inline void TFatBootSector::SetVolumeLabel(const TDesC8& aDes)
	{
	__ASSERT_DEBUG(aDes.Length()<=KVolumeLabelSize,User::Panic(_L("FATFILESYS"),1));
	TPtr8 buf(iVolumeLabel,KVolumeLabelSize);
	buf=aDes;
	}
/**
Set the file system type

@param aDes A descriptor containing the file system type
*/
inline void TFatBootSector::SetFileSysType(const TDesC8& aDes)
	{
	__ASSERT_DEBUG(aDes.Length()<=8,User::Panic(_L("FATFILESYS"),1));
	TPtr8 buf(iFileSysType,8);
	buf=aDes;
	}
/**
Tests if the volume is Fat 16 or not

@return True if the volume is Fat16
*/
inline TBool TFatBootSector::Is16BitFat()
	{return(FileSysType()==_L8("FAT16   "));}
/**
Returns the position of the first sector of the first Fat

@return The first Fat sector's byte position
*/
inline TInt TFatBootSector::FirstFatSectorPos()
	{return(ReservedSectors()*BytesPerSector());}
/**
Returns the start sector number of the root directory

@return Start sector number of the root directory
*/
inline TInt TFatBootSector::RootDirStartSector()
	{return(ReservedSectors()+FatSectors()*NumberOfFats());}
/**
Returns the sector number of the first sector after the root directory 

@return 
*/
inline TInt TFatBootSector::FirstFreeSector()
	{return(RootDirStartSector()+(RootDirEntries()*KSizeOfFatDirEntry+BytesPerSector()-1)/BytesPerSector());}

