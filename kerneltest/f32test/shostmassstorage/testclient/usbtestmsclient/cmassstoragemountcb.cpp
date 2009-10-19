// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// CMassStorageMountCB implementation.
// 
//



/**
 @file
 @internalTechnology
*/

#include <f32fsys.h>

#include "mstypes.h"
#include "msctypes.h"

#include "cmassstoragefilesystem.h"
#include "drivemanager.h"


#include "cusbmassstoragecontroller.h"
#include "cmassstoragemountcb.h"
#include "debug.h"

CMassStorageMountCB::CMassStorageMountCB(const TLunToDriveMap& aDriveMapping)
:   iDriveMapping(aDriveMapping)
	{
	}

CMassStorageMountCB* CMassStorageMountCB::NewL(const TLunToDriveMap& aDriveMapping)
	{
	return new (ELeave) CMassStorageMountCB(aDriveMapping);
	}

/**
Checks that the drive number is supported.

@leave KErrNotReady The drive number is not supported.
*/
TInt CMassStorageMountCB::CheckDriveNumberL()
	{
	__FNLOG("CMassStorageMountCB::CheckDriveNumberL");
	TInt driveNumber;
	driveNumber = Drive().DriveNumber();
	if (!IsValidLocalDriveMapping(driveNumber))
		{
		__PRINT1(_L("CMassStorageMountCB::CheckDriveNumberL: Drive number %d not supported"), driveNumber);
		User::Leave(KErrNotReady);
		}
	__PRINT1(_L("CMassStorageMountCB::CheckDriveNumberL: Drive number = %d"), driveNumber);
	return driveNumber;
	}

/**
Registers the drive with the Mass Storage drive manager.

@leave KErrNotSupported The drive is not compatible with Mass Storage.
*/
void CMassStorageMountCB::MountL(TBool /*aForceMount*/)
	{
	__FNLOG("CMassStorageMountCB::MountL");

	CheckDriveNumberL();
	CMassStorageFileSystem& msFsys = *reinterpret_cast<CMassStorageFileSystem*>(Drive().GetFSys());

	TInt lun = DriveNumberToLun(Drive().DriveNumber());

	if(lun < 0)
		{
		// This is not a supported Mass Storage drive
		User::Leave(KErrNotSupported);
		}

	TBusLocalDrive& localDrive = msFsys.iMediaChangedStatusList[lun].iLocalDrive;

	TInt err = CreateLocalDrive(localDrive);
	User::LeaveIfError(err);

	CProxyDrive* proxyDrive = LocalDrive();

	TLocalDriveCapsV2Buf caps;
	err = localDrive.Caps(caps);

	//Make sure file system is FAT and removable
	if (err == KErrNone)
		{
		err = KErrNotSupported;
		if ((caps().iDriveAtt & KDriveAttRemovable) == KDriveAttRemovable)
			{
			if (caps().iType != EMediaNotPresent)
				{
				err = KErrNone;
				}
			}
		}

	if (err != KErrNone && err != KErrNotReady)
		{
		__PRINT1(_L("CMassStorageMountCB::MountL: Drive is not compatible with Mass Storage, err=%d"), err);
		User::Leave(err);
		}

	__PRINT(_L("CMassStorageMountCB::MountL: Registering drive"));
	// Set media changed to true so that Win2K doesn't used cached drive data
	TBool& mediaChanged = msFsys.iMediaChangedStatusList[lun].iMediaChanged;
    mediaChanged = ETrue;
	msFsys.Controller().DriveManager().RegisterDriveL(*proxyDrive, mediaChanged, lun);
	SetVolumeName(_L("MassStorage").AllocL());
	}

/**
Returns the LUN that corresponds to the specified drive number.

@param aDriveNumber The drive number.
*/
TInt CMassStorageMountCB::DriveNumberToLun(TInt aDriveNumber)
	{
	__FNLOG("CMassStorageMountCB::DriveNumberToLun");
	TInt lun = -1;
	for (TInt i = 0; i < iDriveMapping.Count(); i++)
		{
		if (iDriveMapping[i] == aDriveNumber)
			{
			lun = i;
			break;
			}
		}
	__PRINT2(_L("CMassStorageMountCB::DriveNumberToLun: Drive %d maps to LUN %d"), aDriveNumber, lun);
	return lun;
	}

/**
Deregisters the drive from the Drive Manager.
*/
void CMassStorageMountCB::Dismounted()
	{
	__FNLOG("CMassStorageMountCB::Dismounted");
	TInt driveNumber = -1;
	TRAPD(err, driveNumber = CheckDriveNumberL());
	if (err != KErrNone)
		{
		return;
		}
	__PRINT(_L("CMassStorageMountCB::Dismounted: Deregistering drive"));
    CMassStorageFileSystem& msFsys = *reinterpret_cast<CMassStorageFileSystem*>(Drive().GetFSys());
	msFsys.Controller().DriveManager().DeregisterDrive(DriveNumberToLun(driveNumber));

	DismountedLocalDrive();
	}

/**
Unlocks the drive with the specified password, optionally storing the password for later use.

@param aPassword The password to use for unlocking the drive.
@param aStore True if the password is to be stored.
*/
TInt CMassStorageMountCB::Unlock(TMediaPassword& aPassword, TBool aStore)
	{
	__FNLOG("CMassStorageMountCB::Unlock");
	TInt driveNumber = -1;
	TRAPD(err, driveNumber = CheckDriveNumberL());
	if (err != KErrNone)
		{
		return err;
		}
	TBusLocalDrive& localDrive=GetLocalDrive(driveNumber);
	if(localDrive.Status() == KErrLocked)
		{
		localDrive.Status() = KErrNotReady;
		}
	TInt r = localDrive.Unlock(aPassword, aStore);
	if(r == KErrNone && aStore)
		{
		WritePasswordData();
		}
	return(r);
	}

/**
Stores the password for the drive to the password file.
*/
void CMassStorageMountCB::WritePasswordData()
	{
	__FNLOG("CMassStorageMountCB::WritePasswordData");
	TBusLocalDrive& local=GetLocalDrive(Drive().DriveNumber());
	TInt length = local.PasswordStoreLengthInBytes();
	if(length==0)
		{
		TBuf<sizeof(KMediaPWrdFile)> mediaPWrdFile(KMediaPWrdFile);
		mediaPWrdFile[0] = (TUint8) RFs::GetSystemDriveChar();
		WriteToDisk(mediaPWrdFile,_L8(""));
		return;
		}
	HBufC8* hDes=HBufC8::New(length);
	if(hDes==NULL)
		{
		return;
		}
	TPtr8 pDes=hDes->Des();
	TInt r=local.ReadPasswordData(pDes);
	if(r==KErrNone)
		{
		TBuf<sizeof(KMediaPWrdFile)> mediaPWrdFile(KMediaPWrdFile);
		mediaPWrdFile[0] = (TUint8) RFs::GetSystemDriveChar();
		WriteToDisk(mediaPWrdFile,pDes);
		}
	delete hDes;
	}

/**
Make sure that the file system is fat.
*/
TBool CMassStorageMountCB::ValidateBootSector()
	{
	__FNLOG("CMassStorageMountCB::ValidateBootSector");

	TFatBootSector bootSector;
	TInt r=ReadBootSector(bootSector);
	__PRINT1(_L("CMassStorageMountCB::MountL - ReadBootSector returned %d"),r);
	if (r != KErrNone)
		{
		return EFalse;
		}

	__PRINT(_L("\nBootSector info"));
	__PRINT8BIT1(_L("FAT type = %S"),bootSector.FileSysType());
	__PRINT8BIT1(_L("Vendor ID = %S"),bootSector.VendorId());
	__PRINT1(_L("BytesPerSector %d"),bootSector.BytesPerSector());
	__PRINT1(_L("SectorsPerCluster %d"),bootSector.SectorsPerCluster());
	__PRINT1(_L("ReservedSectors %d"),bootSector.ReservedSectors());
	__PRINT1(_L("NumberOfFats %d"),bootSector.NumberOfFats());
	__PRINT1(_L("RootDirEntries %d"),bootSector.RootDirEntries());
	__PRINT1(_L("Total Sectors = %d"),bootSector.TotalSectors());
	__PRINT1(_L("MediaDescriptor = 0x%x"),bootSector.MediaDescriptor());
	__PRINT1(_L("FatSectors %d"),bootSector.FatSectors());
	__PRINT1(_L("SectorsPerTrack %d"),bootSector.SectorsPerTrack());
	__PRINT1(_L("NumberOfHeads %d"),bootSector.NumberOfHeads());
	__PRINT1(_L("HugeSectors %d"),bootSector.HugeSectors());
	__PRINT1(_L("Fat32 Sectors %d"),bootSector.FatSectors32());
	__PRINT1(_L("Fat32 Flags %d"),bootSector.FATFlags());
	__PRINT1(_L("Fat32 Version Number %d"),bootSector.VersionNumber());
	__PRINT1(_L("Root Cluster Number %d"),bootSector.RootClusterNum());
	__PRINT1(_L("FSInfo Sector Number %d"),bootSector.FSInfoSectorNum());
	__PRINT1(_L("Backup Boot Rec Sector Number %d"),bootSector.BkBootRecSector());
	__PRINT1(_L("PhysicalDriveNumber %d"),bootSector.PhysicalDriveNumber());
	__PRINT1(_L("ExtendedBootSignature %d"),bootSector.ExtendedBootSignature());
	__PRINT1(_L("UniqueID %d"),bootSector.UniqueID());
	__PRINT8BIT1(_L("VolumeLabel %S"),bootSector.VolumeLabel());
	__PRINT8BIT1(_L("FileSysType %S\n"),bootSector.FileSysType());

    iUniqueID=bootSector.UniqueID();
	iIs16BitFat=bootSector.Is16BitFat();

	iIs32BitFat=bootSector.Is32BitFat();
	switch (DetermineFatType(bootSector))
		{
		case 12:
			iIs16BitFat = EFalse;
			iIs32BitFat = EFalse;
			break;
		case 16:
			iIs16BitFat = ETrue;
			iIs32BitFat = EFalse;
			break;
		case 32:
			iIs16BitFat = EFalse;
			iIs32BitFat = ETrue;
			break;
		default:
			return EFalse;
		}

	TInt sectorsPerCluster=bootSector.SectorsPerCluster();
	if (!IsPowerOfTwo(sectorsPerCluster))
		return EFalse;

	TInt sectorSizeLog2=Log2(bootSector.BytesPerSector());
	if (sectorSizeLog2<0 || !IsPowerOfTwo(bootSector.BytesPerSector()))
		return EFalse;

	TInt firstFatSector=bootSector.ReservedSectors();
	if (firstFatSector<1)
		return EFalse;

	TInt fatSizeInBytes;
	if(iIs32BitFat)
		{
		fatSizeInBytes=bootSector.FatSectors32()*bootSector.BytesPerSector();
		if (fatSizeInBytes<bootSector.BytesPerSector())
			return EFalse;
		}
	else
		{
		fatSizeInBytes=bootSector.FatSectors()*bootSector.BytesPerSector();
		if (fatSizeInBytes<bootSector.BytesPerSector())
			return EFalse;

		TInt rootDirectorySector=firstFatSector+bootSector.FatSectors()*bootSector.NumberOfFats();
		if (rootDirectorySector<3)
			return EFalse;

		TInt rootDirSizeInBytes=bootSector.RootDirEntries()*KSizeOfFatDirEntry;
		TInt numOfRootDirSectors=(rootDirSizeInBytes+(1<<sectorSizeLog2)-1)>>sectorSizeLog2;
		TInt rootDirEnd=(rootDirectorySector+numOfRootDirSectors)<<sectorSizeLog2;
		if (rootDirEnd<(4<<sectorSizeLog2))
			return EFalse;
		}


	TInt totalSectors=bootSector.TotalSectors();
	if (totalSectors==0)
		totalSectors=bootSector.HugeSectors();
	if (totalSectors<5)
		return EFalse;

	TInt numberOfFats=bootSector.NumberOfFats();
	if (numberOfFats<1)
		return EFalse;

	return ETrue;
	}

/**
Read non aligned boot data from media into TFatBootSector structure

@param aBootSector refrence to TFatBootSector populate
@return Media read error code
*/
TInt CMassStorageMountCB::ReadBootSector(TFatBootSector& aBootSector)
	{
	__FNLOG("CMassStorageMountCB::ReadBootSector");
	TInt pos=0;
	TUint8 data[KSizeOfFatBootSector];
    TPtr8 buf(&data[0],KSizeOfFatBootSector);
    TInt r=LocalDrive()->Read(0,KSizeOfFatBootSector,buf);
	if (r!=KErrNone)
		{
		__PRINT1(_L("LocalDrive::Read() failed - %d"),r);
		return(r);
		}
//	0	TUint8 iJumpInstruction[3]
	Mem::Copy(&aBootSector.iJumpInstruction,&data[pos],3);
	pos+=3;
// 3	TUint8 iVendorId[KVendorIdSize]
	Mem::Copy(&aBootSector.iVendorId,&data[pos],KVendorIdSize);
	pos+=KVendorIdSize;
// 11	TUint16 iBytesPerSector
	Mem::Copy(&aBootSector.iBytesPerSector,&data[pos],2);
	pos+=2;
// 13	TUint8 sectorsPerCluster
	Mem::Copy(&aBootSector.iSectorsPerCluster,&data[pos],1);
	pos+=1;
// 14	TUint16 iReservedSectors
	Mem::Copy(&aBootSector.iReservedSectors,&data[pos],2);
	pos+=2;
// 16	TUint8 numberOfFats
	Mem::Copy(&aBootSector.iNumberOfFats,&data[pos],1);
	pos+=1;
// 17	TUint16 iRootDirEntries
	Mem::Copy(&aBootSector.iRootDirEntries,&data[pos],2);
	pos+=2;
// 19	TUint16 totalSectors
	Mem::Copy(&aBootSector.iTotalSectors,&data[pos],2);
	pos+=2;
// 21	TUint8 iMediaDescriptor
	Mem::Copy(&aBootSector.iMediaDescriptor,&data[pos],1);
	pos+=1;
// 22	TUint16 iFatSectors
	Mem::Copy(&aBootSector.iFatSectors,&data[pos],2);
	pos+=2;
// 24	TUint16 iSectorsPerTrack
	Mem::Copy(&aBootSector.iSectorsPerTrack,&data[pos],2);
	pos+=2;
// 26	TUint16 iNumberOfHeads
	Mem::Copy(&aBootSector.iNumberOfHeads,&data[pos],2);
	pos+=2;
// 28	TUint32 iHiddenSectors
	Mem::Copy(&aBootSector.iHiddenSectors,&data[pos],4);
	pos+=4;
// 32	TUint32 iHugeSectors
	Mem::Copy(&aBootSector.iHugeSectors,&data[pos],4);
	pos+=4;

	if(aBootSector.iRootDirEntries == 0)	//indicates we have FAT32 volume
		{
		__PRINT(_L("\nFile system thinks Fat32"));

		//36 TUint32 iFatSectors32
		Mem::Copy(&aBootSector.iFatSectors32, &data[pos],4);
		pos+=4;
		//40 TUint16 iFATFlags
		Mem::Copy(&aBootSector.iFATFlags, &data[pos],2);
		pos+=2;
		//42 TUint16 iVersionNumber
		Mem::Copy(&aBootSector.iVersionNumber, &data[pos],2);
		pos+=2;
		//44 TUint32 iRootClusterNum
		Mem::Copy(&aBootSector.iRootClusterNum, &data[pos],4);
		pos+=4;
		//48 TUint16 iFSInfoSectorNum
		Mem::Copy(&aBootSector.iFSInfoSectorNum, &data[pos],2);
		pos+=2;
		//50 TUint16 iBkBootRecSector
		Mem::Copy(&aBootSector.iBkBootRecSector, &data[pos],2);
		pos+=(2+12);//extra 12 for the reserved bytes
		}

// 36|64	TUint8 iPhysicalDriveNumber
	Mem::Copy(&aBootSector.iPhysicalDriveNumber,&data[pos],1);
	pos+=1;
// 37|65	TUint8 iReserved
	Mem::Copy(&aBootSector.iReserved,&data[pos],1);
	pos+=1;
// 38|66	TUint8 iExtendedBootSignature
	Mem::Copy(&aBootSector.iExtendedBootSignature,&data[pos],1);
	pos+=1;
// 39|67	TUint32 iUniqueID
	Mem::Copy(&aBootSector.iUniqueID,&data[pos],4);
	pos+=4;
// 43|71	TUint8 iVolumeLabel[KVolumeLabelSize]
	Mem::Copy(&aBootSector.iVolumeLabel,&data[pos],KVolumeLabelSize);
	pos+=KVolumeLabelSize;
// 54|82	TUint8 iFileSysType[KFileSysTypeSize]
	Mem::Copy(&aBootSector.iFileSysType,&data[pos],KFileSysTypeSize);
// 62|90

	return(KErrNone);
	}

/**
Work out if we have a FAT12|16|32 volume.
Returns 12, 16 or 32 as appropriate.
Returns 0 if can't be calculated (invalid values)
*/
TInt CMassStorageMountCB::DetermineFatType(TFatBootSector& aBootSector)
	{
	TUint32 ressectors = aBootSector.ReservedSectors();

	if (aBootSector.SectorsPerCluster() < 1)
		return 0;

	if (aBootSector.RootDirEntries() != 0)
		{
		TUint32 rootdirbytes;
		rootdirbytes = aBootSector.RootDirEntries() * 32 + aBootSector.BytesPerSector() - 1;
		ressectors += rootdirbytes / aBootSector.BytesPerSector();
		}

	if (aBootSector.FatSectors() != 0)
		ressectors += aBootSector.NumberOfFats() * aBootSector.FatSectors();
	else
		ressectors += aBootSector.NumberOfFats() * aBootSector.FatSectors32();

	TUint32 totalsectors;
	if (aBootSector.TotalSectors() != 0)
		totalsectors = aBootSector.TotalSectors();
	else
		totalsectors = aBootSector.HugeSectors();

	if (ressectors < 1 || totalsectors < 1)
		return 0;

	TUint32 datasec;
	datasec = totalsectors - ressectors;

	TUint32 countofclusters;
	countofclusters = datasec / aBootSector.SectorsPerCluster();

	__PRINT1(_L("CFatMountCB: Count of clusters = %d\n"), countofclusters);

	if (countofclusters < 4085)
		{
		return 12;
		}
	else if (countofclusters < 65525)
		{
		return 16;
		}
	else
		{
		return 32;
		}
	}

TInt CMassStorageMountCB::ReMount()
	{
    RDebug::Printf("CMassStorageMountCB::ReMount()");
	return KErrNotReady;
	}

void CMassStorageMountCB::VolumeL(TVolumeInfo& /*aVolume*/) const
	{
	User::Leave(KErrNotReady);
	}

void CMassStorageMountCB::SetVolumeL(TDes& /*aName*/)
	{
	User::Leave(KErrNotReady);
	}

void CMassStorageMountCB::MkDirL(const TDesC& /*aName*/)
	{
	User::Leave(KErrNotReady);
	}

void CMassStorageMountCB::RmDirL(const TDesC& /*aName*/)
	{
	User::Leave(KErrNotReady);
	}

void CMassStorageMountCB::DeleteL(const TDesC& /*aName*/)
	{
	User::Leave(KErrNotReady);
	}

void CMassStorageMountCB::RenameL(const TDesC& /*anOldName*/,const TDesC& /*anNewName*/)
	{
	User::Leave(KErrNotReady);
	}

void CMassStorageMountCB::ReplaceL(const TDesC& /*anOldName*/,const TDesC& /*anNewName*/)
	{
	User::Leave(KErrNotReady);
	}

void CMassStorageMountCB::EntryL(const TDesC& /*aName*/,TEntry& /*anEntry*/) const
	{
	User::Leave(KErrNotReady);
	}

void CMassStorageMountCB::SetEntryL(const TDesC& /*aName*/,const TTime& /*aTime*/,TUint /*aSetAttMask*/,TUint /*aClearAttMask*/)
	{
	User::Leave(KErrNotReady);
	}

void CMassStorageMountCB::FileOpenL(const TDesC& /*aName*/,TUint /*aMode*/,TFileOpen /*anOpen*/,CFileCB* /*aFile*/)
	{
	User::Leave(KErrNotReady);
	}

void CMassStorageMountCB::DirOpenL(const TDesC& /*aName*/,CDirCB* /*aDir*/)
	{
	User::Leave(KErrNotReady);
	}


void CMassStorageMountCB::RawReadL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aTrg*/,TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/) const
	{
	User::Leave(KErrNotReady);
	}

void CMassStorageMountCB::RawWriteL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aSrc*/,TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/)
	{
	User::Leave(KErrNotReady);
	}


void CMassStorageMountCB::GetShortNameL(const TDesC& /*aLongName*/,TDes& /*aShortName*/)
	{
	User::Leave(KErrNotReady);
	}

void CMassStorageMountCB::GetLongNameL(const TDesC& /*aShorName*/,TDes& /*aLongName*/)
	{
	User::Leave(KErrNotReady);
	}

#if defined(_DEBUG)
TInt CMassStorageMountCB::ControlIO(const RMessagePtr2& aMessage,TInt aCommand,TAny* aParam1,TAny* aParam2)
//
// Debug function
//
	{
	if(aCommand>=(KMaxTInt/2))
		return LocalDrive()->ControlIO(aMessage,aCommand-(KMaxTInt/2),aParam1,aParam2);
	else
		return KErrNotSupported;
	}
#else
TInt CMassStorageMountCB::ControlIO(const RMessagePtr2& /*aMessage*/,TInt /*aCommand*/,TAny* /*aParam1*/,TAny* /*aParam2*/)
	{return(KErrNotSupported);}
#endif

void CMassStorageMountCB::ReadSectionL(const TDesC& /*aName*/,TInt /*aPos*/,TAny* /*aTrg*/,TInt /*aLength*/,const RMessagePtr2& /*aMessage*/)
	{
	User::Leave(KErrNotReady);
	}

/**
Returns ETrue if aNum is a power of two
*/
TBool CMassStorageMountCB::IsPowerOfTwo(TInt aNum)
	{

	if (aNum==0)
		return(EFalse);

	while(aNum)
		{
		if (aNum & 0x01)
			{
			if (aNum>>1)
				return EFalse;
			break;
			}
		aNum>>=1;
		}
	return ETrue;
	}

/**
Returns the position of the highest bit in aNum or -1 if aNum == 0
*/
TInt CMassStorageMountCB::Log2(TInt aNum)
	{

	TInt res=-1;
	while(aNum)
		{
		res++;
		aNum>>=1;
		}
	return(res);
	}
