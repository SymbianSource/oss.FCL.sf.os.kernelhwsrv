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
// f32\sfat32\sl_mnt32.cpp
// CFatMountCB code, specific to the EFAT32.FSY
// 
//

/**
 @file 
*/

#include "sl_std.h"
#include "sl_cache.h"
#include "sl_leafdir_cache.h"

//-------------------------------------------------------------------------------------------------------------------


/**
    Write aligned members of TFatBootSector to media
 
    @param  aMediaPos   media position the data will be written to
    @param  aBootSector data to write
    @return Media write error code
*/
TInt CFatMountCB::DoWriteBootSector(TInt64 aMediaPos, const TFatBootSector& aBootSector)  const
    {
    __PRINT2(_L("#- CFatMountCB::DoWriteBootSector() drv:%d, pos:0x%x"),Drive().DriveNumber(), (TUint32)aMediaPos);   

    ASSERT(aMediaPos>=0);

    TBuf8<KDefaultSectorSize> bootSecBuf(KDefaultSectorSize);
    bootSecBuf.FillZ();

    //-- externalize boot sector to the data buffer
    aBootSector.Externalize(bootSecBuf);

    //-- put a boot sector signature to the last 2 bytes
    bootSecBuf[KDefaultSectorSize-2] = 0x55;
    bootSecBuf[KDefaultSectorSize-1] = 0xaa;

    //-- write boot sector to the media
    TInt r=LocalDrive()->Write(aMediaPos, bootSecBuf);
    if (r!=KErrNone)
        {//-- write failure
        __PRINT2(_L("CFatMountCB::DoWriteBootSector() failed! drv:%d, code:%d"),Drive().DriveNumber(),r);
        }

    return r;
    }

//-------------------------------------------------------------------------------------------------------------------

/**
    Read non aligned boot data from media into TFatBootSector structure

    @param  aMediaPos   media position the data will be read from
    @param  aBootSector refrence to TFatBootSector populate
    @return Media read error code
*/
TInt CFatMountCB::DoReadBootSector(TInt64 aMediaPos, TFatBootSector& aBootSector)  const
    {
    __PRINT2(_L("#- CFatMountCB::DoReadBootSector() drv:%d, pos:0x%x"),Drive().DriveNumber(), (TUint32)aMediaPos);   

    ASSERT(aMediaPos>=0);

    TBuf8<KSizeOfFatBootSector> bootSecBuf(KSizeOfFatBootSector);
    
    //-- read boot sector from the media
    TInt r=LocalDrive()->Read(aMediaPos, KSizeOfFatBootSector, bootSecBuf);
    
    if (r != KErrNone)
        {
        __PRINT2(_L("CFatMountCB::DoReadBootSector() failed! drv:%d, code:%d"),Drive().DriveNumber(),r);

	    //-- fiddling with the error code; taken from MountL()
        if (r==KErrNotSupported)
		    return KErrNotReady;
    #if defined(_LOCKABLE_MEDIA)
	    else if(r==KErrLocked)
		    return KErrLocked;
    #endif
	    else if (r!=KErrNoMemory && r!=KErrNotReady && r!=KErrCorrupt && r!=KErrUnknown)
                return KErrCorrupt; 

        return r;
        }

    ASSERT(r==KErrNone);

    //-- initialise TFatBootSector object
    aBootSector.Internalize(bootSecBuf);

    //-- Validate the partition size, and fix up if the out of bounds
    TLocalDriveCapsV2Buf localDriveCaps;
    r = LocalDrive()->Caps(localDriveCaps);
    if (r != KErrNone)
        {
        //-- fiddling with the error code; taken from MountL()
        if (r!=KErrNoMemory && r!=KErrNotReady && r!=KErrCorrupt && r!=KErrUnknown)
            return KErrCorrupt;
        else
            return r;
        }

    if(!(localDriveCaps().iMediaAtt & KMediaAttVariableSize))
        {//-- this is not a RAM drive.
        const TUint32 maxSectors = I64LOW(localDriveCaps().iSize >> KDefSectorSzLog2);

        if(aBootSector.TotalSectors())
            aBootSector.SetTotalSectors(Min(aBootSector.TotalSectors(), maxSectors));
        else
            aBootSector.SetHugeSectors(Min(aBootSector.HugeSectors(), maxSectors));
        }

    return KErrNone;
    }

//-------------------------------------------------------------------------------------------------------------------

/**
    Read and validate the boot sector.
    If there is an error in reading the main boot sector (sec:0) or it is invalid, tries to read backup boot sector from sec:6.
    Flag iMainBootSecValid indicates the validity of the main boot sector. if it is false, but ret. value is KErrNone, it means that
    the backup boot sector was used and it is valid.

    @param      aBootSector reference to the boot sector object to be read.
    @param      aDoNotReadBkBootSec if true, there won't be an attempt to read backup sector
    @return     standard error code.

*/
TInt CFatMountCB::ReadBootSector(TFatBootSector& aBootSector, TBool aDoNotReadBkBootSec/*=EFalse*/)
    {
    iMainBootSecValid = EFalse; 

    //-- read main boot sector from the sector 0
    TInt nRes = DoReadBootSector(KBootSectorNum << KDefSectorSzLog2, aBootSector); 
    if(nRes == KErrNone)
        {
        if(aBootSector.IsValid())
            {
            iMainBootSecValid = ETrue; //-- main boot sector is valid, everything is OK
            return KErrNone;
            }
        else
            {
            __PRINT(_L("MainBoot Sector is invalid! dump:\n"));
            aBootSector.PrintDebugInfo();
            nRes = KErrCorrupt;
            }
        }

    ASSERT(nRes!= KErrNone && !iMainBootSecValid);

    if(aDoNotReadBkBootSec)
        return nRes;

    //-- main boot sector is invalid, try backup one (it might not present at all) 
    __PRINT(_L("Using backup boot sector...\n"));
    nRes=DoReadBootSector(KBkBootSectorNum << KDefSectorSzLog2, aBootSector); 
    if(nRes == KErrNone )
        {
        if(aBootSector.IsValid())
            return KErrNone; //-- main boot sector is bad, but backup one is OK
        else
            {//-- backup boot sector is invalid
            __PRINT(_L("Backup Sector is invalid! dump:\n"));    
            aBootSector.PrintDebugInfo();
            nRes = KErrCorrupt;
            }
        }
    
    //-- can't read boot sectors, or both are invalid
    return nRes;
    }

//-------------------------------------------------------------------------------------------------------------------

/**
Write a new volume label to BPB in media

@param aVolumeLabel Descriptor containing the new volume label
@leave 
*/
void CFatMountCB::WriteVolumeLabelL(const TDesC8& aVolumeLabel) const
    {
    if(aVolumeLabel.Length() > KVolumeLabelSize)
        User::Leave(KErrArgument);

    const TUint32 posVolLabel = Is32BitFat() ? KFat32VolumeLabelPos : KFat16VolumeLabelPos;
    User::LeaveIfError(LocalDrive()->Write(posVolLabel, aVolumeLabel)); 
    
    }


//-------------------------------------------------------------------------------------------------------------------

const TUint32 KFat32CleanShutDownMask	= 0x08000000; ///< Mask used to indicate test clean/dirty bit for Fat32
const TUint16 KFat16CleanShutDownMask	= 0x08000;    ///< Mask used to indicate test clean/dirty bit for Fat16

/**
Set or reset "VolumeClean" (ClnShutBitmask) flag.

@param  aClean if ETrue, marks the volume as clean, otherwise as dirty.
@leave  if write error occured.        
*/
void CFatMountCB::SetVolumeCleanL(TBool aClean) 
    {

	//-- The volume can't be set clean if there are disk access objects opened on it. This precondition must be checked before calling this function
    if(aClean && Locked())
        {
        __PRINT1(_L("#- CFatMountCB::SetVolumeCleanL drive:%d isn't free!"),DriveNumber());
        ASSERT(0);
        User::Leave(KErrInUse);
        return;
        }
    
    if(FatType() == EFat12)
        {//-- Fat12 doesn't support this feature; do nothing other than notify the underlying drive (ignoring any error for now as there's nothing we can do with it)
		(void)LocalDrive()->Finalise(aClean);
        return;
        }

    //-- further read and write will be directly from the CProxyDrive, bypassing FAT cache. 
    //-- this is because CFatTable doesn't allow access to FAT[0] & FAT[1]
    //-- We also need to write data through CProxyDrive, because TDriveInterface has a call back that can call this method

    if(Is32BitFat())
        {//-- Fat32
		__PRINT2(_L("#- CFatMountCB::SetVolumeCleanL, drive:%d, param:%d, FAT32"),DriveNumber(), aClean);
		
        TFat32Entry fatEntry;
        const TInt  KFatEntrySize=sizeof(fatEntry); //-- FAT entry size in bytes
        TPtr8       ptrFatEntry((TUint8*)&fatEntry,KFatEntrySize);
        
        User::LeaveIfError(LocalDrive()->Read(StartOfFatInBytes()+KFatEntrySize, KFatEntrySize, ptrFatEntry)); //read FAT32[1] entry

        const TFat32Entry tmp = fatEntry;
        
        if(aClean)
            fatEntry |= KFat32CleanShutDownMask;  //-- set ClnShutBit flag
        else
            fatEntry &= ~KFat32CleanShutDownMask; //-- reset ClnShutBit flag

        if(tmp != fatEntry)
            {//-- write FAT[1] entry to all available FATs
                for(TUint32 i=0; i<NumberOfFats(); ++i)
                {
                const TInt64 pos = StartOfFatInBytes()+KFatEntrySize+(FatSizeInBytes()*i);
                User::LeaveIfError(LocalDrive()->Write(pos, ptrFatEntry)); //write FAT32[1] entry
                }
            }

        __PRINT2(_L("#- CFatMountCB::SetVolumeCleanL() entry:  %x->%x"), tmp, fatEntry);
        }
    else 
    if(Is16BitFat())
        {//-- Fat16. 
            __PRINT2(_L("#- CFatMountCB::SetVolumeCleanL, drive:%d, param:%d, FAT16"),DriveNumber(), aClean);

            if(FatConfig().FAT16_UseCleanShutDownBit())
                {
            TFat16Entry fatEntry;
            const TInt  KFatEntrySize=sizeof(fatEntry); //-- FAT entry size in bytes
            TPtr8       ptrFatEntry((TUint8*)&fatEntry,KFatEntrySize);
        
            User::LeaveIfError(LocalDrive()->Read(StartOfFatInBytes()+KFatEntrySize, KFatEntrySize, ptrFatEntry)); //read FAT16[1] entry

            const TFat16Entry tmp = fatEntry;
        
            if(aClean)
                fatEntry |= KFat16CleanShutDownMask;  //-- set ClnShutBit flag
            else
                fatEntry &= ~KFat16CleanShutDownMask; //-- reset ClnShutBit flag

            if(tmp != fatEntry)
                {//-- write FAT[1] entry to all available FATs
                for(TUint32 i=0; i<NumberOfFats(); ++i)
                    {
                    const TInt64 pos = StartOfFatInBytes()+KFatEntrySize+(FatSizeInBytes()*i);
                    User::LeaveIfError(LocalDrive()->Write(pos, ptrFatEntry)); //write FAT16[1] entry
                    }
                }
		    
            __PRINT2(_L("#- CFatMountCB::SetVolumeCleanL() entry:  %x->%x"), tmp, fatEntry);    
            }
            else
            {
            __PRINT(_L("#- changing FAT16[1] is disabled in config!"));    
            }
        }
    else
        {//-- must never get here
        ASSERT(0);
        }
    
		//-- Notify the underlying media that the mount is consistent  (ignoring any error for now as there's nothing we can do with it)
		(void)LocalDrive()->Finalise(aClean);


    }

//-------------------------------------------------------------------------------------------------------------------

/**
Determine whether "VolumeClean" (ClnShutBitmask) flag is set.

@return ETrue if the volume is marked as clean and EFalse otherwise.
@leave  if is called for FAT12 or if read error occured.
*/
TBool CFatMountCB::VolumeCleanL() 
    {

    //-- read access to the FAT is through TDriveInterface, because CFatTable doesn't allow access to FAT[1]
    TDriveInterface& drive =DriveInterface();

    if(Is32BitFat())
        {//-- Fat32
        TFat32Entry fatEntry;
        const TInt  KFatEntrySize=sizeof(fatEntry); //-- FAT entry size in bytes
        TPtr8       ptrFatEntry((TUint8*)&fatEntry, KFatEntrySize);
        
        User::LeaveIfError(drive.ReadNonCritical(StartOfFatInBytes()+KFatEntrySize, KFatEntrySize, ptrFatEntry)); //read FAT32[1] entry
        return (fatEntry & KFat32CleanShutDownMask);
        }
    else if(Is16BitFat())
        {//-- Fat16
        TFat16Entry fatEntry;
        const TInt  KFatEntrySize=sizeof(fatEntry); //-- FAT entry size in bytes
        TPtr8       ptrFatEntry((TUint8*)&fatEntry, KFatEntrySize);
        
        User::LeaveIfError(drive.ReadNonCritical(StartOfFatInBytes()+KFatEntrySize, KFatEntrySize, ptrFatEntry)); //read FAT16[1] entry
        return (fatEntry & KFat16CleanShutDownMask);
        }
    else
        {//-- Fat12 doesn't support this feature, shan't get here, actually
        ASSERT(0);
        User::Leave(KErrNotSupported);
        return ETrue; //-- to satisfy the compiler
        }
    }


//-------------------------------------------------------------------------------------------------------------------

/**
Mount a Fat volume. 

@param aForceMount Flag to indicate whether mount should be forced to succeed if an error occurs
@leave KErrNoMemory,KErrNotReady,KErrCorrupt,KErrUnknown.
*/
void CFatMountCB::MountL(TBool aForceMount)
	{

    const TInt driveNo = Drive().DriveNumber();
    
    __PRINT3(_L("CFatMountCB::MountL() drv:%d, forceMount=%d, RuggedFAT:%d\n"), driveNo, aForceMount, IsRuggedFSys());

    ASSERT(State() == ENotMounted || State() == EDismounted);
    SetState(EMounting);
    SetReadOnly(EFalse);
   
	User::LeaveIfError(CreateDrive(driveNo));


    //-- read FAT configuration parameters from estart.txt
    iFatConfig.ReadConfig(driveNo);

    //-- initialise interface to the low-level drive access
    if(!iDriverInterface.Init(this))
        User::LeaveIfError(KErrNoMemory);    

	//-- get drive capabilities
    TLocalDriveCapsV2Buf capsBuf;
	User::LeaveIfError(LocalDrive()->Caps(capsBuf));
	

    iSize=capsBuf().iSize;
    iRamDrive = EFalse;

    if(capsBuf().iMediaAtt & KMediaAttVariableSize)
    {//-- this is a RAM drive
        UserSvr::UnlockRamDrive();
        iRamDrive = ETrue;
    }

	if(aForceMount)
	{//-- the state is "forcedly mounted", special case. This is an inconsistent state.
        SetState(EInit_Forced);  
    	return;
    }

    //-- read boot sector. If main is damaged, try to use backup one instead if this is not a RAM drive.
    TFatBootSector bootSector;
    User::LeaveIfError(ReadBootSector(bootSector, iRamDrive));


    //-- print out boot sector debug information
    bootSector.PrintDebugInfo();

    //-- determine FAT type by data from boot sector. This is done by counting number of clusters, not by BPB_RootEntCnt
    SetFatType(bootSector.FatType());
    ASSERT(iFatType != EInvalid); //-- this shall be checked in ReadBootSector()
    

    if(bootSector.RootDirEntries() == 0 && !Is32BitFat())
    {//-- FAT types mismatch. BPB_RootEntCnt is 0, which can be only for FAT32, but the number of clusters is less 
     //-- than required for FAT32. Probably this is incorrectly FAT32 formatted media. Put the drive into ReadOnly mode, assuming
     //-- that is FAT32.
        __PRINT(_L("FAT type mismatch! Setting drive to ReadOnly mode for FAT32. \n"));
        SetFatType(EFat32); //-- force FAT type to be FAT32
        SetReadOnly(ETrue);
    }

    //-- store volume UID, it can be checked on Remount
    iUniqueID = bootSector.UniqueID();

    //-- populate volume parameters with the values from boot sector. They had been validated in TFatBootSector::IsValid()
    iVolParam.Populate(bootSector); 
	
    //-- initialize the volume
    InitializeL(capsBuf());
    ASSERT(State()==EInit_R);

    GetVolumeLabelFromDiskL(bootSector);

	__PRINT2(_L("CFatMountCB::MountL() Completed, drv: %d, state:%d"), DriveNumber(), State());
	}



//-------------------------------------------------------------------------------------------------------------------

/**
Initialize the FAT cache and disk access

@param  aLocDrvCaps local drive capabilities
@param  aIgnoreFSInfo if ETrue, FSInfo sector shall be ignored. Used on volume remount to force FAT free clusters counting.

@leave KErrNoMemory,KErrNotReady,KErrCorrupt,KErrUnknown.
*/
void CFatMountCB::InitializeL(const TLocalDriveCaps& aLocDrvCaps, TBool aIgnoreFSInfo/*=EFalse*/) 
	{
    __PRINT1(_L("CFatMountCB::InitializeL() drv:%d"), DriveNumber());

    ASSERT(State() == EMounting); //-- we must get here only from MountL()
   
    //========== Find out number of clusters on the volume
	if(iRamDrive && SectorsPerCluster()!=1)
		{// Align iFirstFreeByte to cluster boundary if internal ram drive
		const TInt sectorsPerClusterLog2=ClusterSizeLog2()-SectorSizeLog2();
		const TInt rootDirEndSector=RootDirEnd()>>SectorSizeLog2();
		const TInt alignedSector=((rootDirEndSector+SectorsPerCluster()-1)>>sectorsPerClusterLog2)<<sectorsPerClusterLog2;
		iFirstFreeByte=alignedSector<<SectorSizeLog2();
		}
	else
		{
		if(Is32BitFat())
			iFirstFreeByte=(NumberOfFats() * FatSizeInBytes()) + (FirstFatSector() << SectorSizeLog2());
		else
			iFirstFreeByte=RootDirEnd();
		}


	    {//-- check if volume geometry looks valid
        const TUint32 usableSectors = TotalSectors()-(iFirstFreeByte>>SectorSizeLog2());
	    iUsableClusters=usableSectors>>(ClusterSizeLog2()-SectorSizeLog2());

        const TUint32 KMinClusters = 32; //-- absolute minimum number of clusters on the volume
        const TUint32 KMaxClusters=(TotalSectors()-FirstFatSector()-NumberOfFats()*(FatSizeInBytes()>>SectorSizeLog2())) >> (ClusterSizeLog2()-SectorSizeLog2());
        
        if(iUsableClusters < KMinClusters || iUsableClusters > KMaxClusters)
            {
            __PRINT(_L("CFatMountCB::InitializeL() Wrong number of usable cluster/sectors on the volume!"));
            User::Leave(KErrCorrupt);
            }
        }

	//========== initialise RawDisk interface
	//-- CFatMountCB parameters might have changed, e.g. after formatting. Reconstruct directory cache with new parameters
	
    delete iRawDisk;
    iRawDisk = NULL;
	iRawDisk=CRawDisk::NewL(*this, aLocDrvCaps);
    iRawDisk->InitializeL();


    //========== Try to read FSInfo and deduct number of free clusters and other information from there
    TBool   bUseDataFromFsInfo = !aIgnoreFSInfo && Is32BitFat(); //-- if ETrue, we are going to use data from FSInfo sector (applicable for FAT32 only)
    
    //-- main boot sector shall be valid, otherwise we can't trust data from FSInfo
    bUseDataFromFsInfo = bUseDataFromFsInfo && iMainBootSecValid;

    //-- 1. check if using FSInfo is disabled in config
    if(bUseDataFromFsInfo && !FatConfig().FAT32_UseFSInfoOnMount())
    {
        __PRINT(_L("#- FSInfo using is disabled in config!"));
        bUseDataFromFsInfo = EFalse;
    }

#ifdef  _DEBUG
    //-- 2. check if FSInfo is disabled by test interface (special debug property). This property is defined and set by the test application.
    TInt nMntDebugFlags;
    if(bUseDataFromFsInfo && RProperty::Get(KSID_Test1, DriveNumber(), nMntDebugFlags) == KErrNone)
        {//-- test property for this drive is defined
            if(nMntDebugFlags & KMntDisable_FsInfo)
            {
            __PRINT(_L("#- FSInfo using is disabled by debug interface."));
            bUseDataFromFsInfo = EFalse;
            }
        }
#endif

    //-- 3. try to read FSInfoSector and its copy if the volume had been properly shut down before (is now clean) 
    CFatTable::TMountParams fatMntParams;
    bUseDataFromFsInfo = bUseDataFromFsInfo && VolumeCleanL();
    if(bUseDataFromFsInfo)
        {
        bUseDataFromFsInfo = ProcessFSInfoSectors(fatMntParams);
        if(!bUseDataFromFsInfo)
            { 
            __PRINT1(_L("#- CFatMountCB::ProcessFSInfoSectors() failed. drv:%d"), DriveNumber());
            }
        }

    //========== create and initialise FAT table 
	
    delete iFatTable;
    iFatTable = NULL;
    iFatTable=CFatTable::NewL(*this, aLocDrvCaps);

    //-- mount the FAT table. Depending on mount parameters and configuration this method 
    //-- can do various things, like counting free clusters synchronously if data from FSInfo isn't valid, 
    //-- or setting up a FAT backround thread and return immediately etc.
    iFatTable->MountL(fatMntParams);
   
    SetState(EInit_R);  //-- the state is "Initialized, but not writen"

    //-- make a callback, telling FileServer about free space discovered.
    const TInt64 freeSpace = ((TInt64)FAT().NumberOfFreeClusters()) << ClusterSizeLog2();
    SetDiskSpaceChange(freeSpace);

    //========== create and setup leaf direcotry cache
		{
	const TUint32 cacheLimit = Max(iFatConfig.LeafDirCacheSize(), 1lu);
	
		delete iLeafDirCache;
        iLeafDirCache = NULL;
		iLeafDirCache = CLeafDirCache::NewL(cacheLimit);
		}


    
    __PRINT3(_L("#- CFatMountCB::InitializeL() done. drv:%d, Free clusters:%d, 1st Free cluster:%d"),DriveNumber(), FAT().NumberOfFreeClusters(), FAT().FreeClusterHint());

    
	}

//-------------------------------------------------------------------------------------------------------------------

/**
    Write FSInfo sectors to the media

    @param aMediaPos    Media position to write FSInfo sector to
    @param aFSInfo      FSInfo structure to write
    @return System wide error code
*/
TInt CFatMountCB::WriteFSInfoSector(TInt64 aMediaPos, const TFSInfo& aFSInfo)  const
    {
    __PRINT2(_L("#- CFatMountCB::WriteFSInfoSector() drv:%d, pos:0x%x"),Drive().DriveNumber(), (TUint32)aMediaPos);   

    ASSERT(aMediaPos >= 0 && aMediaPos < FirstFatSector()<<SectorSizeLog2());
    ASSERT(Is32BitFat());

    TBuf8<KSizeOfFSInfo> fsInfoSecBuf;

    //-- put data to the sector buffer
    aFSInfo.Externalize(fsInfoSecBuf);

	TInt r=LocalDrive()->Write(aMediaPos, fsInfoSecBuf);
	if (r!=KErrNone)
        {
        __PRINT2(_L("CFatMountCB::WriteFSInfoSector() failed! drv:%d, code:%d"),Drive().DriveNumber(),r);
        }

    return r;
    }

//-------------------------------------------------------------------------------------------------------------------
/**
    Read the FSInfo structure from media 

    @param aMediaPos    Media position to read FSInfo sector from
    @param aFSInfo      data read from FSInfo structure
    @return System wide error code
*/
TInt CFatMountCB::ReadFSInfoSector(TInt64 aMediaPos, TFSInfo& aFSInfo)  const
    {
    __PRINT2(_L("#- CFatMountCB::ReadFSInfoSector() drv:%d, pos:0x%x"),Drive().DriveNumber(), (TUint32)aMediaPos);   

    ASSERT(aMediaPos >= 0 && aMediaPos < FirstFatSector()<<SectorSizeLog2());
    ASSERT(Is32BitFat());

    TBuf8<KSizeOfFSInfo> fsInfoSecBuf;

    TInt r=LocalDrive()->Read(aMediaPos, KSizeOfFSInfo, fsInfoSecBuf); 
	if (r!=KErrNone)
	{
		__PRINT2(_L("CFatMountCB::ReadFSInfoSector() failed! drv:%d, code:%d"),Drive().DriveNumber(),r);
	    return r;
	}

    //-- take FSInfo data from the buffer
    aFSInfo.Internalize(fsInfoSecBuf);
    
    return KErrNone;
    }


/**
    Initialize data to represent the root directory

    @param anEntry Entry to initialise
*/
void CFatMountCB::InitializeRootEntry(TFatDirEntry& anEntry) const
	{
	anEntry.SetName(_L8("ROOT"));
	anEntry.SetAttributes(KEntryAttDir);
	anEntry.SetStartCluster(RootClusterNum()); //--iRootClusterNum is 0 for FAT12/16
	}



/**
    Implementation of CMountCB::FileSystemSubType(). Retrieves the sub type of Fat file system
    and returns the name as a descriptor.

    @param aName Name of the sub type of Fat file system
    @return KErrNone if successful; KErrArgument if aName is not long enough; KErrNotReady if
		    the mount is not ready.

    @see CMountCB::FileSystemSubType()
*/
TInt CFatMountCB::SubType(TDes& aName) const
	{
	if(aName.MaxLength() < 5)
		return KErrArgument;
	
	switch(FatType())
		{
		case EFat12:
			{
			aName = KFSSubType_FAT12;
			return KErrNone;
			}
		case EFat16:
			{
			aName = KFSSubType_FAT16;
			return KErrNone;
			}
		case EFat32:
			{
			aName = KFSSubType_FAT32;
			return KErrNone;
			}

		default:
		    break;
			
		}
	
        return KErrNotReady;
    }


//-------------------------------------------------------------------------------------------------------------------

/**
    Try to extract useful information from the FSInfo sectors.
    The information from FSInfo sectors will be trusted only if there are main and backup sectors,
    they are both valid and the same.

    @param  aFatInitParams  on success will contain the number of free clusters on the volume and 1st free cluster number from the FSInfo 
    @return ETrue on success.
*/
TBool CFatMountCB::ProcessFSInfoSectors(CFatTable::TMountParams& aFatInitParams) const
    {

    aFatInitParams.iFreeClusters = 0;
    aFatInitParams.iFirstFreeCluster = 0;
    aFatInitParams.iFsInfoValid = EFalse;

    const TUint32 currFsInfoSec   = iVolParam.FSInfoSectorNum();    //-- values from the boot sector
    const TUint32 currFsInfoBkSec = iVolParam.BkFSInfoSectorNum();                                                                              

    if(!Is32BitFat() || currFsInfoSec < KFSInfoSectorNum || currFsInfoSec >= FirstFatSector())
        {
        ASSERT(0);  //-- main FSInfo sector must have sensible location
        return EFalse;
        }

    if(currFsInfoBkSec < KFSInfoSectorNum || currFsInfoBkSec >= FirstFatSector() || currFsInfoBkSec <= currFsInfoSec)
        return EFalse; //-- something is wrong with backup copy location

    TFSInfo fsInfo;
	TInt    nRes;

    //-- 1. read and validate main FS Info sector
    nRes = ReadFSInfoSector(currFsInfoSec << SectorSizeLog2(), fsInfo);
    if(nRes != KErrNone)
        return EFalse;

    fsInfo.PrintDebugInfo();

    if(!fsInfo.IsValid())
        return EFalse;

    const TUint32 freeCount=fsInfo.FreeClusterCount(); // last known free cluster count
	const TUint32 nextFree =fsInfo.NextFreeCluster();  // hint to file system as to where to start looking for free clusters

    //-- 2. read and check backup FS Info sector, it must be the same as the main one
    nRes = ReadFSInfoSector(currFsInfoBkSec << SectorSizeLog2(), fsInfo);
    
    if(nRes != KErrNone || !fsInfo.IsValid() || freeCount != fsInfo.FreeClusterCount() || nextFree != fsInfo.NextFreeCluster())
    {
        __PRINT(_L("#- CFatMountCB::ProcessFSInfoSectors(): copies of FSInfo are different!"));
        return EFalse;
    }

    if(freeCount < 1 || freeCount > UsableClusters())
        return EFalse; //-- looks like invalid value
    
    if(nextFree < KFatFirstSearchCluster || nextFree >= UsableClusters()+KFatFirstSearchCluster)
        return EFalse; //-- looks like invalid value

    //-- success
    aFatInitParams.iFreeClusters = freeCount;
    aFatInitParams.iFirstFreeCluster = nextFree;
    aFatInitParams.iFsInfoValid = ETrue;
    
    return ETrue;
    }

//-------------------------------------------------------------------------------------------------------------------
/**
    Internal helper method. Writes FSInfo sector and its backup copy to the volume if necessary. 
    
    @param  aInvalidateFSInfo if ETrue, FSInfo data (free clusters count) will be invalidated.
                              otherwise, data from the CFatTable object will be used
                                    
    @leave  if disk opertion fails
*/
void CFatMountCB::DoUpdateFSInfoSectorsL(TBool aInvalidateFSInfo)
    {
    __PRINT2(_L("#- CFatMountCB::DoUpdateFSInfoSectorsL(%d) drv:%d"),aInvalidateFSInfo, Drive().DriveNumber());
    
    ASSERT(Is32BitFat());
    
    if(!aInvalidateFSInfo)
        {
        ASSERT(FAT().ConsistentState());
        }
        
    //-- 1. check that FSInfoSector numbers are valid
    TBool bCanWriteFSInfo_Main = EFalse;//-- if ETrue, it's OK to write main FSInfo sector
    TBool bCanWriteFSInfo_Bk = EFalse;  //-- if ETrue, it's OK to write backup FSInfo sector

    const TUint32 currFsInfoSec   = iVolParam.FSInfoSectorNum();    //-- values from the boot sector
    const TUint32 currFsInfoBkSec = iVolParam.BkFSInfoSectorNum();
        
    if(currFsInfoSec > 0 && currFsInfoSec < FirstFatSector())
        {//-- seems to be OK
        bCanWriteFSInfo_Main = ETrue;
        }
    else
        {
        __PRINT1(_L("#- DoUpdateFSInfoSectorsL() iFSInfoSectorNum is wrong!: sec:%d"), currFsInfoSec);
        }

    if(currFsInfoBkSec > 0 && currFsInfoBkSec < FirstFatSector() && currFsInfoBkSec > currFsInfoSec)
        {//-- seems to be OK
        bCanWriteFSInfo_Bk = bCanWriteFSInfo_Main;
        }
    else
        {
        __PRINT1(_L("#- DoUpdateFSInfoSectorsL() iBkFSInfoSectorNum is wrong!: sec:%d"),currFsInfoBkSec);
        }

    if(!bCanWriteFSInfo_Main && !bCanWriteFSInfo_Bk)
        return; //-- nothing to do

    
    const TUint32 KFsInfoSecPos_Main = currFsInfoSec   << SectorSizeLog2();  //-- main FSInfo sector media position
    const TUint32 KFsInfoSecPos_Bk   = currFsInfoBkSec << SectorSizeLog2();  //-- backup FSInfo sector media position
    
    TFSInfo fsInfoSector;

    TBool bNeedWriteFSInfo_Main = EFalse;
    TBool bNeedWriteFSInfo_Bk = EFalse;

    
    const TUint32 KInvalidVal = 0xFFFFFFFF; //-- invalid value for FSInfo fields, see FAT specs
    
    //-- we need here _exact_ number of free clusters, so make FAT().NumberOfFreeClusters() operation synchronous
    const TUint32 KFreeClusters    = aInvalidateFSInfo ? KInvalidVal : FAT().NumberOfFreeClusters(ETrue); 
    const TUint32 KNextFreeCluster = aInvalidateFSInfo ? KInvalidVal : FAT().FreeClusterHint(); 
    
    //-- check if the main FSInfo sector differs from the FAT information
    if(bCanWriteFSInfo_Main)
        {
        User::LeaveIfError(ReadFSInfoSector(KFsInfoSecPos_Main, fsInfoSector));
        bNeedWriteFSInfo_Main = !fsInfoSector.IsValid() ||
                                fsInfoSector.FreeClusterCount() != KFreeClusters || 
                                fsInfoSector.NextFreeCluster()  != KNextFreeCluster; 
        }

    //-- check if the backup FSInfo sector differs from the FAT information
    if(bCanWriteFSInfo_Bk)
        {
        User::LeaveIfError(ReadFSInfoSector(KFsInfoSecPos_Bk, fsInfoSector));
        bNeedWriteFSInfo_Bk =   !fsInfoSector.IsValid() ||
                                fsInfoSector.FreeClusterCount() != KFreeClusters ||
                                fsInfoSector.NextFreeCluster()  != KNextFreeCluster; 
        }

    //-- setup data in FSInfo sector to write
    fsInfoSector.Initialise();
    fsInfoSector.SetFreeClusterCount(KFreeClusters);
    fsInfoSector.SetNextFreeCluster(KNextFreeCluster);

    if(!bNeedWriteFSInfo_Main && !bNeedWriteFSInfo_Bk)
        return; //-- nothing to do

    SetVolumeCleanL(EFalse); //-- mark volume dirty, just in case something will happen on FSInfo sectors write

    if(bNeedWriteFSInfo_Main)
        User::LeaveIfError(WriteFSInfoSector(KFsInfoSecPos_Main, fsInfoSector));

    if(bNeedWriteFSInfo_Bk)
        User::LeaveIfError(WriteFSInfoSector(KFsInfoSecPos_Bk, fsInfoSector));

    }

//-------------------------------------------------------------------------------------------------------------------

/**
    CFatMountCB control method.
    @param  aLevel  specifies the operation to perfrom on the mount
    @param  aOption specific option for the given operation
    @param  aParam  pointer to generic parameter, its meaning depends on aLevel and aOption

    @return standard error code.
*/
TInt CFatMountCB::MountControl(TInt aLevel, TInt aOption, TAny* aParam)
    {
    __PRINT3(_L("CFatMountCB::MountControl() drv:%d, level:%d, opt:%d"),Drive().DriveNumber(), aLevel, aOption);
    
    TInt nRes = KErrNotSupported; 
    
    if(aLevel == ECheckFsMountable)
        {
        return MntCtl_DoCheckFileSystemMountable();
        }
    
    //-- todo: move these functions code into separate methods ??

    //-- mount state query: check if is in finalised state 
    if(aLevel == EMountStateQuery && aOption == ESQ_IsMountFinalised)
        {
        TBool bFinalised;
        nRes = IsFinalised(bFinalised);
        if(nRes == KErrNone)
            {
            ASSERT(aParam);
            *((TBool*)aParam) = bFinalised;
            }
        
        return nRes;
        }

    //-- mount-specific volume parameters queries that might not be handled by CFatMountCB::VolumeL
    if(aLevel == EMountVolParamQuery)
        {
        ASSERT(ConsistentState()); //-- volume state shall be consistent, otherwise its parameters do not make sense

		// Ram Drives calculate their total / free space based on querying HAL parameters
		// in ::VolumeL(). To make all interfaces return consistent results, we need to force
		// a fallback to that for RAM drives.
		if (iRamDrive)
			return (KErrNotSupported);

        switch(aOption)
            {
            //-- Request a certain amount of free space on the volume.
            case ESQ_RequestFreeSpace:
                {
                TUint64* pVal = (TUint64*)aParam; //-- in: number of free bytes on the volume required, out: resulted amount of free space.
                const TUint32 KClustersRequired = (TUint32)((*pVal + ClusterSize() - 1) >> ClusterSizeLog2());
                __PRINT2(_L("MountControl() ReqFreeSpace:%LU, clusters:%d"), *pVal, KClustersRequired);    

                if(KClustersRequired)
                    {//-- actually, this doesn't guarantee that it will finally be KClustersRequired available.
                    (void)FAT().RequestFreeClusters(KClustersRequired);  
                    }
         
                const TUint32 freeClusters = FAT().NumberOfFreeClusters(EFalse); //-- _current_ amount of free clusters  
                *pVal = (TInt64)freeClusters << ClusterSizeLog2();

                return KErrNone;
                }

            //-- A request to obtain the _current_ amount of free space on the volume asynchronously, without blocking.
            case ESQ_GetCurrentFreeSpace:
                {
                TUint64* pVal = (TUint64*)aParam; //-- out: resulted amount of free space.

                const TUint32 freeClusters = FAT().NumberOfFreeClusters(EFalse); //-- _current_ amount of free clusters  
                *pVal = (TInt64)freeClusters << ClusterSizeLog2();
                __PRINT1(_L("MountControl() Asynch. request; curent amount of free clusters: %d"), freeClusters);

                return KErrNone;
                }
        
            //-- A request to obtain size of the mounted volume without blocking (CMountCB::VolumeL() can block).
            case ESQ_MountedVolumeSize:
                {
                TUint64* pVal = (TUint64*)aParam; 
                *pVal = VolumeSizeInBytes();
                __PRINT1(_L("MountControl() MountedVolumeSize:%LU"), *pVal);
                return KErrNone;
                }

            default:
            __PRINT1(_L("MountControl() unsupported opt:%d"), aOption);
            ASSERT(0);
            break;
            };

        }

        //-- File System - specific queries 
        if(aLevel == EMountFsParamQuery && aOption == ESQ_GetMaxSupportedFileSize)
            {//-- this is a query to provide the max. supported file size; aParam is a pointer to TUint64 to return the value
            *(TUint64*)aParam = KMaxSupportedFatFileSize;    
            return KErrNone;
            }


    
    
    return KErrNotSupported; 
    }

//-----------------------------------------------------------------------------------------

/**
Reports whether the specified interface is supported - if it is,
the supplied interface object is modified to it

@param aInterfaceId     The interface of interest
@param aInterface       The interface object
@return                 KErrNone if the interface is supported, otherwise KErrNotFound 

@see CMountCB::GetInterface()
*/
TInt CFatMountCB::GetInterface(TInt aInterfaceId, TAny*& aInterface,TAny* aInput)
    {
    switch(aInterfaceId)
        {
        case (CMountCB::EFileAccessor):
            ((CMountCB::MFileAccessor*&) aInterface) = this;
            return KErrNone;
        
        case (CMountCB::EGetFileSystemSubType):
            aInterface = (MFileSystemSubType*) (this);
            return KErrNone;

        case (CMountCB::EGetClusterSize):
            aInterface = (MFileSystemClusterSize*) (this);
            return KErrNone;

        case CMountCB::ELocalBufferSupport:
            // RAM drives doesn't support local buffers (this results in file caching being disabled)
            if (iRamDrive) 
                return KErrNotSupported;
            else
                return LocalDrive()->LocalBufferSupport();
        
		case EGetProxyDrive:
			((CProxyDrive*&)aInterface) = LocalDrive();
			return KErrNone;
        
        case CMountCB::EFileExtendedInterface:
       		// For supporting large file ReadFileSection
       		((CMountCB::MFileExtendedInterface*&) aInterface) = this;
           	return KErrNone;
        
        default:
            break;
        }
    
    return(CMountCB::GetInterface(aInterfaceId, aInterface, aInput));
    }

//-----------------------------------------------------------------------------------------
void CFatMountCB::ReadSection64L(const TDesC& aName, TInt64 aPos, TAny* aTrg, TInt aLength, const RMessagePtr2& aMessage)
// From CMountCB::MFileExtendedInterface
	{
    __PRINT(_L("CFatMountCB::ReadSection64L"));
	
    CheckStateConsistentL();
    TEntryPos dosEntryPos(RootIndicator(),0);
    TFatDirEntry dosEntry;
    TFileName fileName; 

    TInt namePos=aName.LocateReverse(KPathDelimiter)+1; // There is always a path delimiter
    TLeafDirData leafDir(0);
    dosEntryPos.iCluster=FindLeafDirL(aName.Left(namePos), leafDir);
    dosEntryPos.iPos=0;
    TEntryPos firstEntryPos;
    TFatDirEntry firstEntry;
    DoFindL(aName.Mid(namePos),KEntryAttMaskSupported,
    		firstEntryPos,firstEntry,dosEntryPos,dosEntry,
    		fileName,KErrNotFound,
    		NULL,
    		leafDir);
    
//  Check that reading from aPos for aLength lies within the file
//  if aPos is within the file, and aLength is too long, read up to EOF
//  If aPos is beyond the end of the file, return a zero length descriptor

	TInt64 fileSize = MAKE_TINT64(0,dosEntry.Size());
	if (aPos>=fileSize)
        User::Leave(KErrEof);
	
    if (aPos+aLength>fileSize)
        aLength=(TInt)(fileSize-aPos);
    
    TInt cluster=StartCluster(dosEntry);	
	TInt64 pos = aPos;
	
    TUint32 endCluster;
    TInt clusterSize=1<<ClusterSizeLog2();      //  Size of file clusters
	TInt readTotal = 0;
	
	// Total number of clusters in file
    TInt maxClusters=(TInt)((fileSize+clusterSize-1)>>ClusterSizeLog2());

	// Read data
    FOREVER
        {
		//  Get the maximum number of clusters that can be read contiguously
        TInt64 clusterListLen=FAT().CountContiguousClustersL(cluster,endCluster,maxClusters);
        __ASSERT_DEBUG(clusterListLen>0,Fault(EReadFileSectionFailed));

		//  If start position within this block, then read some data
        if (pos<(clusterListLen<<ClusterSizeLog2()))
            {
			//  Read the remaining length or the entire cluster block whichever is smaller
			TInt readLength = (TInt)Min((TInt64)(aLength-readTotal),(clusterListLen<<ClusterSizeLog2())-pos);
			__ASSERT_DEBUG(readLength>0,Fault(EReadFileSectionFailed));
			TInt64 dataAddress=(FAT().DataPositionInBytesL(cluster))+pos;
			iRawDisk->ReadL(dataAddress,readLength,aTrg,aMessage,readTotal, 0);
			readTotal += readLength;

			if (readTotal == aLength)
				return;

			pos += readLength;
			}
		
		// Get the next cluster in file
		pos-=(clusterListLen<<ClusterSizeLog2());
#if defined(_DEBUG)
		TBool remainingClusters=
#endif
			((CFatMountCB*)this)->FAT().GetNextClusterL(endCluster);
		__ASSERT_DEBUG(remainingClusters,Fault(EReadFileSectionFailed));		
		cluster=endCluster;
		}
    }
