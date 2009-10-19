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
// Class declaration for CMassStorageMountCB.
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __CMASSSTORAGEMOUNTCB_H__
#define __CMASSSTORAGEMOUNTCB_H__

#include <f32fsys.h>

/**
Mass Storage Mount.
Only the MountL, Dismounted and Unlock methods are supported. All other methods
leave with KErrNotReady.
ControlIO is also supported for debug builds and returns KErrNotSupported for Release builds.
@internalTechnology
*/
class TFatBootSector;
class CMassStorageMountCB : public CLocDrvMountCB
	{
	public:
	static CMassStorageMountCB* NewL(const RArray<TInt>& aDriveMapping);
	void MountL(TBool aForceMount);
	TInt ReMount();
	void Dismounted();
	void VolumeL(TVolumeInfo& aVolume) const;
	void SetVolumeL(TDes& aName);
	void MkDirL(const TDesC& aName);
	void RmDirL(const TDesC& aName);
	void DeleteL(const TDesC& aName);
	void RenameL(const TDesC& anOldName,const TDesC& anNewName);
	void ReplaceL(const TDesC& anOldName,const TDesC& anNewName);
	void EntryL(const TDesC& aName,TEntry& anEntry) const;
	void SetEntryL(const TDesC& aName,const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask);
	void FileOpenL(const TDesC& aName,TUint aMode,TFileOpen anOpen,CFileCB* aFile);
	void DirOpenL(const TDesC& aName,CDirCB* aDir);
	void RawReadL(TInt64 aPos, TInt aLength, const TAny* aTrg, TInt anOffset, const RMessagePtr2& aMessage) const;
	void RawWriteL(TInt64 aPos, TInt aLength, const TAny* aTrg, TInt anOffset, const RMessagePtr2& aMessage);
	void ReadSectionL(const TDesC& aName, TInt aPos, TAny* aTrg, TInt aLength, const RMessagePtr2& aMessage);
	void GetShortNameL(const TDesC& aLongName,TDes& aShortName);
	void GetLongNameL(const TDesC& aShorName,TDes& aLongName);
	TInt ControlIO(const RMessagePtr2& aMessage,TInt aCommand,TAny* aParam1,TAny* aParam2);
	TInt Unlock(TMediaPassword& aPassword, TBool aStore);

	private:
	CMassStorageMountCB(const RArray<TInt>& aDriveMapping);
	void WritePasswordData();
	TInt DriveNumberToLun(TInt aDriveNumber);
	TBool ValidateBootSector();
	TInt ReadBootSector(TFatBootSector& aBootSector);
	TInt DetermineFatType(TFatBootSector& aBootSector);
	TInt CheckDriveNumberL();

	private:
	TBool iIs16BitFat;
	TBool iIs32BitFat;
	const RArray<TInt>& iDriveMapping;
	};

const TInt KSizeOfFatBootSector	= 90;
const TInt KVendorIdSize = 8;
const TInt KVolumeLabelSize = 11;
const TInt KFileSysTypeSize = 8;
const TInt KBootSectorSignature = 0xAA55;
const TInt KSizeOfFatDirEntry = 32;

/**
Boot sector parameter block, enables access to all file system parameters.
Data is populated at mount time from the BPB sector
@internalTechnology
*/
class TFatBootSector
	{
public:
	inline const TPtrC8 VendorId() const;
	inline TUint16 BytesPerSector() const;
	inline TInt SectorsPerCluster() const;
	inline TInt ReservedSectors() const;
	inline TInt NumberOfFats() const;
	inline TInt RootDirEntries() const;
	inline TInt TotalSectors() const;
	inline TUint8 MediaDescriptor() const;
	inline TInt FatSectors() const;
	inline TInt SectorsPerTrack() const;
	inline TInt NumberOfHeads() const;
	inline TInt HiddenSectors() const;
	inline TInt HugeSectors() const;
	inline TInt PhysicalDriveNumber() const;
	inline TInt ExtendedBootSignature() const;
	inline TUint32 UniqueID() const;
	inline const TPtrC8 VolumeLabel() const;
	inline const TPtrC8 FileSysType() const;
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

	inline void SetFatSectors32(TUint32	aFatSectors32);
	inline void SetFATFlags(TUint16 aFATFlags);
	inline void SetVersionNumber(TUint16	aVersionNumber);
	inline void SetRootClusterNum(TUint32 aRootCusterNum);
	inline void SetFSInfoSectorNum(TUint16 aFSInfoSectorNum);
	inline void SetBkBootRecSector(TUint16 aBkBootRecSector);
	inline TUint32 FatSectors32() const;
	inline TUint16 FATFlags() const;
	inline TUint16 VersionNumber() const;
	inline TUint32 RootClusterNum() const;
	inline TUint16 FSInfoSectorNum() const;
	inline TUint16 BkBootRecSector() const;
public:
	inline TBool Is16BitFat();
	inline TBool Is32BitFat();
	inline TInt FirstFatSectorPos();
	inline TInt RootDirStartSector();
	inline TInt FirstFreeSector();
public:
	/**
	Jump instruction used for bootable volumes
	*/
    TUint8 iJumpInstruction[3];
	/**
	Vendor ID of the file system that formatted the volume
	*/
    TUint8 iVendorId[KVendorIdSize];
	/**
	Bytes per sector
	*/
    TUint16 iBytesPerSector;
	/**
	Sectors per cluster ratio
	*/
    TUint8 iSectorsPerCluster;
	/**
	Number of reserved sectors on the volume
	*/
    TUint16 iReservedSectors;
	/**
	Number of Fats on the volume
	*/
    TUint8 iNumberOfFats;
	/**
	Number of entries allowed in the root directory, specific to Fat12/16, zero for FAT32
	*/
    TUint16 iRootDirEntries;
	/**
	Total sectors on the volume, zero for FAT32
	*/
    TUint16 iTotalSectors;
	/**
	Media descriptor
	*/
    TUint8 iMediaDescriptor;
	/**
	Sectors used for the Fat table, zero for FAT32
	*/
    TUint16 iFatSectors;
	/**
	Sectors per track
	*/
    TUint16 iSectorsPerTrack;
	/**
	Number of heads
	*/
    TUint16 iNumberOfHeads;
	/**
	Number of hidden sectors in the volume
	*/
    TUint32 iHiddenSectors;
	/**
	Total sectors in the volume, Used if totalSectors > 65535
	*/
    TUint32 iHugeSectors;

	/**
	Start of additional elements @ offset 36 for FAT32
	Sectors in Fat table for 32 bit volume
	*/
	TUint32	iFatSectors32;
	/**
	Fat flags
	*/
	TUint16 iFATFlags;
	/**
	Version number of the file system
	*/
	TUint16	iVersionNumber;
	/**
	Cluster number of the root directory
	*/
	TUint32 iRootClusterNum;
	/**
	Sector number containing the FSIInfo structure
	*/
	TUint16 iFSInfoSectorNum;
	/**
	Backup boot sector
	*/
	TUint16 iBkBootRecSector;
	/**
	Reserved space
	End of Fat32 Only parameters section
	*/
	TUint8	iReserved2[12];

	/**
	Physical drive number, not used in Symbian OS
	*/
	TUint8 iPhysicalDriveNumber;
	/**
	Reserved byte
	*/
    TUint8 iReserved;
	/**
	Extended boot signiture
	*/
    TUint8 iExtendedBootSignature;
	/**
	Unique volume ID
	*/
    TUint32 iUniqueID;
	/**
	The volume's label
	*/
    TUint8 iVolumeLabel[KVolumeLabelSize];
	/**
	File system type
	*/
	TUint8 iFileSysType[KFileSysTypeSize];
	};

#include "tfatbootsector.inl"

#endif //__CMASSSTORAGEMOUNTCB_H__
