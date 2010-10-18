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
// Common CFatMountCB code for both EFAT.FSY and EFAT32.fsy
//
//

/**
 @file
 @internalTechnology
*/

#include "sl_std.h"
#include "sl_cache.h"
#include "sl_leafdir_cache.h"
#include "sl_dir_cache.h"
#include "sl_scandrv.h"
#include <hal.h>
#include <f32dbg.h>

TShortName DoGenerateShortNameL(const TDesC& aLongName,TInt& aNum,TBool aUseTildeSelectively);


//-----------------------------------------------------------------------------------------

TFatVolParam::TFatVolParam()
    {
    Mem::FillZ(this, sizeof(TFatVolParam));
    }

/**
    populate the object with the values from the boot sector.
    @param aBootSector a reference to the valid boots sector
*/
void TFatVolParam::Populate(const TFatBootSector& aBootSector)
    {
    ASSERT(aBootSector.IsValid());

    iSectorsPerCluster = aBootSector.SectorsPerCluster();
    iSectorSizeLog2    = Log2(aBootSector.BytesPerSector());
    iClusterSizeLog2   = iSectorSizeLog2+Log2(iSectorsPerCluster);
    iFirstFatSector    = aBootSector.FirstFatSector();
    iNumberOfFats      = aBootSector.NumberOfFats();
    iFatSizeInBytes    = aBootSector.TotalFatSectors()*aBootSector.BytesPerSector();
    iTotalSectors      = aBootSector.VolumeTotalSectorNumber();
    iRootClusterNum    = aBootSector.RootClusterNum(); //-- will be 0 for FAT12/16

    iRootDirectorySector = aBootSector.RootDirStartSector();
    iRootDirEnd = (iRootDirectorySector + aBootSector.RootDirSectors()) << SectorSizeLog2(); //-- doesn't matter for FAT32

    //-- get main and backup FSInfo sectors position, these fields will be 0 for FAT12/16
    iFSInfoSectorNum   = aBootSector.FSInfoSectorNum();
    iBkFSInfoSectorNum = (TUint16)(aBootSector.BkBootRecSector()+iFSInfoSectorNum); //-- Bk FSInfo sector must follow the Bk boot sector
    }

TBool TFatVolParam::operator==(const TFatVolParam& aRhs) const
    {
    ASSERT(&aRhs != this);
    if(&aRhs == this)
        return ETrue; //-- comparing with itself

    return (Mem::Compare((TUint8*)this, sizeof(TFatVolParam), (TUint8*)&aRhs, sizeof(TFatVolParam)) == 0);
    }


//-----------------------------------------------------------------------------------------


CFatMountCB::CFatMountCB()
    {
    __PRINT2(_L("CFatMountCB::CFatMountCB() 0x%x, %S"), this, &KThisFsyName);

    SetFatType(EInvalid);
    iState   = ENotMounted;
    
    DBG_STATEMENT(iCBRecFlag = 0); //-- debug flag only
    }

CFatMountCB::~CFatMountCB()
    {
    __PRINT1(_L("#-CFatMountCB::~CFatMountCB() 0x%x"), this);

    DoDismount();

    delete iNotifier;
    delete iFatTable;
    delete iRawDisk;
    delete iLeafDirCache;

    }

//-----------------------------------------------------------------------------------------

CFatMountCB* CFatMountCB::NewL()
    {
    CFatMountCB* pSelf = new(ELeave) CFatMountCB;

    CleanupStack::PushL(pSelf);
    pSelf->ConstructL();
    CleanupStack::Pop(pSelf);

    return pSelf;
    }

// second-stage constructor
void CFatMountCB::ConstructL()
{
    //-- create Notifier
    iNotifier = CAsyncNotifier::New();
    if( !iNotifier )
        {
        Close();
        User::Leave(KErrNoMemory);
        }

    iNotifier->SetMount(this);
    }

//-------------------------------------------------------------------------------------------------------------------

/**
Implementation of CMountCB::FileSystemClusterSize(). Returns cluster size of this mount.
@return Cluster size value if successful; otherwise KErrNotReady if the mount is not ready.
@see CMountCB::FileSystemClusterSize()
*/
TInt CFatMountCB::ClusterSize() const
    {
    if (ClusterSizeLog2())
        return (1 << ClusterSizeLog2());

    return KErrNotReady;
    }

//-------------------------------------------------------------------------------------------------------------------

/**
    @leave KErrAccessDenied if the mount is read-only
*/
void CFatMountCB::CheckWritableL() const
    {
    if(ReadOnly())
        {
        __PRINT(_L("CFatMountCB is RO!"));
        User::Leave(KErrAccessDenied);
        }
    }

/**
    @leave KErrCorrupt if the mount is in inconsistent state i.e high-level file and directory  operations can not be performed
*/
void CFatMountCB::CheckStateConsistentL() const
    {
    if(!ConsistentState())
        {
        __PRINT(_L("CFatMountCB state is inconsistent !"));
        User::Leave(KErrCorrupt);
        }
    }


//-------------------------------------------------------------------------------------------------------------------
/**
    Helper Method. Check if the parameters of the volume being remounted are the same as current ones.
    @return ETrue if volume parameters remained same.
*/
TBool CFatMountCB::CheckVolumeTheSame()
{
    //-- initialise local drive
    TInt nRes =InitLocalDrive();
    if(nRes != KErrNone)
        return EFalse;

    //-- read the boot sector or its backup copy if the main is damaged. It will aslo validate it.
    TFatBootSector bootSector;
    nRes = ReadBootSector(bootSector, iRamDrive);
    if(nRes != KErrNone)
        return EFalse;

    //-- 1. check volume Uid
    if(iUniqueID != bootSector.UniqueID())
        return EFalse;

    //-- check volume parameters, they must remain the same
    TFatVolParam volParam;
    volParam.Populate(bootSector);

    if(!(volParam == iVolParam))
        return EFalse;


    return ETrue;

}
//-------------------------------------------------------------------------------------------------------------------

/**
    Helper Method. Check if the parameters of the volume being remounted are the same as current ones.
    If they are, re-initialises the mount.
*/
void CFatMountCB::DoReMountL()
    {

    if(!CheckVolumeTheSame())
        User::Leave(KErrGeneral);

	//-- get drive capabilities
    TLocalDriveCapsV2Buf capsBuf;
	User::LeaveIfError(LocalDrive()->Caps(capsBuf));

    //-- the volume is the same as it was on original MountL()
    //-- we need to re-initialize for the case when the media was removed, FAT or directory structure changed on other device and the media returned back.
    DoDismount();

    SetState(EMounting);

    InitializeL(capsBuf(), ETrue); //-- forcedly disable FSInfo usage. This will lead to FAT free clusters re-counting.

    }

//-------------------------------------------------------------------------------------------------------------------

/**
    Try remount this Fat volume. Checks if the volume parameters remained the same as on original MountL() call, and
    if they are, re-initialises the mount. This includes resetting all caches.
    ! Do not call this method from TDriveInterface methods, like citical and non-critical notifiers ! This can lead to the
    recursive loops and undefined behaviour.

    @return KErrNone if the remount was OK
            system-wide error code otherwise
*/
TInt CFatMountCB::ReMount()
    {
    __PRINT2(_L("CFatMountCB::ReMount(), drv:%d, curr state:%d"), DriveNumber(), State());

    const TFatMntState currState = State();

    //-- analyse the mount state and find out if we can remount at all.
    switch(currState)
        {
        case ENotMounted:
        __PRINT(_L("CFatMountCB::ReMount() Invalid mount state!"));

        ASSERT(0);
        return KErrGeneral;

        //-- correct state, proceed to remount
        default:
        break;
    }

    //-- there are 2 options here:
    //-- 1. normally initialised mount had been forcedly dismounted (it can optionally have objects opened on it)
    //--    in this case the DoReMountL() will succeed and everything will be fine, the objects will be accessible afterwards
    //-- 2. the mount hasn't been initialised at all (it does not have for example, FAT table created etc.)
    //--    in this case we may need to fake the success. This can only happen on forced mount by CFormatCB
    TInt nRes;
    TRAP(nRes, DoReMountL());

    if(nRes != KErrNone)
        {
        //-- note that the mount may be here left in inconsistent state (EMounting)
        //-- if DoReMountL() fails. This is OK, because we can not make any valid read/write operations in such a state and
        //-- the drive must be dismounted and mounted again. File Server's TDrive shall do this.
        __PRINT1(_L("CFatMountCB::ReMount() failed! code:%d"), nRes);

        //-- If we are in the EInit_Forced state, it means that we are trying to remount the volume that has been formatted.
        //-- scenario: On formatting, if we can't read a bootsector, new _empty_ object of the CFatMountCB is created and
        //-- it is used for performing a format. If the format has finished, but RFormat isn't closed yet and we try to access the volume,
        //-- we will get here, because all members of the constructed mount will be zeroes.
        if(currState == EInit_Forced)
            {
            __PRINT(_L("CFatMountCB::ReMount() simulating normal remount!"));
            SetState(currState);
            return KErrNone;
            }

        return nRes;
        }

    __PRINT1(_L("CFatMountCB::ReMount() Completed drv:%d"), DriveNumber());
    SetState(EInit_R);
    return nRes;
    }

//-------------------------------------------------------------------------------------------------------------------

/**
    Reset the last leaf dir or invalidate leaf dir cache if leaf dir cache is
    instantiated.
*/

void CFatMountCB::InvalidateLeafDirCache()
	{
    if (iLeafDirCache)
    	{
        iLeafDirCache->Reset();
    	}
	}

//-------------------------------------------------------------------------------------------------------------------

/**
    Delete mount's caches
    Moves CFatMountCB into ENotMounted state immediately.
*/
void CFatMountCB::DoDismount()
    {
    __PRINT1(_L("CFatMountCB::DoDismount() drv:%d"), DriveNumber());

    //-- try to flush and destroy FAT cache
    if (iFatTable)
        {
        if(!ConsistentState() || ReadOnly())
            {//-- the mount state is inconsistent, so the data can't be flushed. Ignore dirty cache either.
            iFatTable->Dismount(ETrue);
            }
        else
            {//-- Try to flush the FAT - if this fails there's not much we can do
            TRAPD(r, iFatTable->FlushL());
            iFatTable->Dismount(r != KErrNone); //-- ignore dirty data if we failed to flush the cache
            }
        }

    //-- destroy leafdir name cache, this cache will be re-created while mounting or re-mounting
    //-- see CFatMountCB::InitializeL()
    delete iLeafDirCache;
    iLeafDirCache = NULL;

    //-- destroy directory cache, this cache will be re-created while mounting or re-mounting
    //-- see CFatMountCB::InitializeL()
    delete iRawDisk;
    iRawDisk = NULL;

    //-- Set mount state to "Dismounted". Which means that there might be no caches, but the mount is alive,
    //-- i.e. iFatTable & iRawDisk are valid
    SetState(EDismounted);
    }

//-----------------------------------------------------------------------------------------

/** old implementation */
void CFatMountCB::FinaliseMountL()
    {
    FinaliseMountL(RFs::EFinal_RW);
    }

//-----------------------------------------------------------------------------------------
/**
    Dismount the CFatMountCB and the drive.
    called from TDrive::Dismount().
*/
void CFatMountCB::Dismounted()
    {
    __PRINT1(_L("CFatMountCB::Dismounted() drv:%d"), DriveNumber());

    //-- n.b. it is no safe to do a kind of filnalisatin work here that implies accessing the media.
    //-- this method may be called after the media change occured from the TDrive::Dismount(). It means
    //-- that if we try to write some data here, they could be written into a different medium, if it had been
    //-- physically changed.

    const TFatMntState prevState = State();

    DoDismount(); //-- it will change mount state to EDismounted
    DismountedLocalDrive();

    //-- check if the previous state was EInit_Forced, which means that this method was called
    //-- on the mount that might not be alive (no valid iFatTable & iRawDisk).
    //-- This can happen only during format operation on non-mounted previously volume.
    //-- this EInit_Forced state must be processed separately, see ::Remount()
    if(prevState == EInit_Forced)
        SetState(EInit_Forced);

    }


//-------------------------------------------------------------------------------------------------------------------

/**
    Find out if the mount is finalised.
    @param  aFinalised on exit will be ETrue if the maunt is finalised, EFalse otherwise.
    @return standard error codes.
*/
TInt CFatMountCB::IsFinalised(TBool& aFinalised)
    {
    switch(State())
        {
        case EFinalised: //-- already explicitly finalised
            aFinalised = ETrue;
        return KErrNone;

        case EInit_W: //-- the volume had been written
            aFinalised = EFalse;
        return KErrNone;

        default: //-- it depends on the state
        break;
        }

    //-- find out if the volume is _physically_ finalised.
    //-- It can be in the state EInit_R, but finalised before mounting
    if(!VolCleanFlagSupported())
        return KErrNotSupported;

    TInt nRes = KErrNone;
    TRAP(nRes, aFinalised = VolumeCleanL());

    return nRes;
    }

//-------------------------------------------------------------------------------------------------------------------

/**
    @return ETrue if the mount is in consistent state i.e. normally mounted.
    See TFatMntState enum for more detail.
*/
TBool CFatMountCB::ConsistentState() const
    {
    return (iState==EInit_R) || (iState==EInit_W) || (iState == EFinalised);
    }

//-------------------------------------------------------------------------------------------------------------------

/**
    Open CFatMountCB for write. I.e. perform some actions on the first write attempt.
    This is a callback from TDriveInterface.
    @return System wide error code.
*/
TInt CFatMountCB::OpenMountForWrite()
    {
    if(State() == EInit_W)
        return KErrNone; //-- nothing to do, the mount is already opened for write

    __PRINT1(_L("#- CFatMountCB::OpenMountForWrite() drv:%d\n"),DriveNumber());

    ASSERT(State() == EInit_R || State() == EFinalised);

    //-- Check possible recursion. This method must not be called recursively. SetVolumeCleanL() works through direct disc access and
    //-- can not call TDriveInterface methods that call this method etc.
    ASSERT(iCBRecFlag == 0);
    DBG_STATEMENT(iCBRecFlag = 1); //-- set recursion check flag

    //-- do here some "opening" work, like marking volme as dirty
    //-- be careful here, as soon as this is a callback from TDriveInterface, writing via TDriveInterface may cause some unwanted recursion.

    //-- mark the volume as dirty
    TInt nRes=KErrNone;
    TRAP(nRes, SetVolumeCleanL(EFalse));
    if(nRes == KErrNone)
        {
        SetState(EInit_W);
        }

    DBG_STATEMENT(iCBRecFlag = 0); //-- reset recursion check flag

    return nRes;

    }

//-------------------------------------------------------------------------------------------------------------------

/**
    Unfinalise the mount, reset "VolumeCleanShutDown" flag and change the state if necessarily.
*/
void CFatMountCB::UnFinaliseMountL()
    {
    switch(State())
        {
        case EFinalised:
        case EInit_R:
            SetVolumeCleanL(EFalse); //-- the mount, mark volume "dirty"
            SetState(EInit_R);
        return;

        case EInit_W:
        return; //-- nothing to do

        default:
        //-- other mount states are inconsistent; can't perform this operation
        User::Leave(KErrAbort);
        break;

        }

    }

//-------------------------------------------------------------------------------------------------------------------

/**
    Finalise the mount.

    @param  aOperation  describes finalisation operation ,see RFs::TFinaliseDrvMode
    @param  aParam1     not used, for future expansion
    @param  aParam2     not used, for future expansion

    @leave  System wide error code. particular cases:
            KErrArgument invalid arguments
            KErrInUse    if the volume has opened objects (files, directories etc)
            KErrCorrupt  if the volume is corrupt

*/
void CFatMountCB::FinaliseMountL(TInt aOperation, TAny* /*aParam1*/, TAny* /*aParam2*/)
    {
    __PRINT2(_L("#- CFatMountCB::FinaliseMountL() op:%d, drv:%d"), aOperation, DriveNumber());

    switch(aOperation)
        {
        case RFs::EFinal_RW:
        case RFs::EFinal_RO:
        break;

        case RFs::EForceUnfinalise:
            UnFinaliseMountL();
        return;

        default:
            __PRINT1(_L("#- CFatMountCB::FinaliseMountL() unexpected operation!:%d"), aOperation);
            ASSERT(0);
            User::Leave(KErrArgument);
        return;
        }

    //-- mount finalisation work

    ASSERT(aOperation == RFs::EFinal_RW || aOperation == RFs::EFinal_RO);

    if(State() == EFinalised)
        {//-- the mount is already finalised. All we can do is to set it to RO mode
        if(ReadOnly() && aOperation == RFs::EFinal_RW)
            {
            User::Leave(KErrAccessDenied); //-- can't override RO flag
            }

		(void)LocalDrive()->Finalise(ETrue);

        if(aOperation == RFs::EFinal_RO)
            {
            SetReadOnly(ETrue);
            return;
            }

        return;
        }

    if(Locked())
        {//-- can't finalise the volume if it has opened disk access objects, like Format or RawAccess
        User::Leave(KErrInUse);
        }

    if(State() != EInit_R && State() != EInit_W)
        {//-- can't finalise the mount because it can be in an inconsistent state; e.g. corrupt.
        __PRINT1(_L("#- CFatMountCB::FinaliseMountL() Invalid mount State: %d"),State());
        User::Leave(KErrCorrupt);
        }

    //-- flush FAT cache
    FAT().FlushL();

    //-- for FAT32 we may need to update data in FSInfo sectors
    if(Is32BitFat())
        {
        if(FAT().ConsistentState())
            {//-- FAT table state is consistent and the number of free clusters is known.
             //-- Do it disregarding the mount state, it may help in the situation when 2 copies of the FSInfo are different for some reason.
            DoUpdateFSInfoSectorsL(EFalse);
            }
            else
            {//-- FAT table state is inconsistent, the most probable case here: background scan for free clusters is still working.
             //-- in this case we can't put corect values into the FSInfo.
            if(State() == EInit_W)
                {//-- bad situation: free clusters may be being counted and someone has already written something on the volume at the same time.
                 //-- we do not know the exact number of free clustes and can't wait until scan finishes. Invalidate FSInfo.
                __PRINT(_L("#- CFatMountCB::FinaliseMountL() invalidating FSInfo"));
                DoUpdateFSInfoSectorsL(ETrue);
                }
             else
                {//-- no changes on the volume, just do not update FSInfo
                __PRINT(_L("#- CFatMountCB::FinaliseMountL() FAT state inconsistent; FSInfo isn't updated"));
                }

            }//if(FAT().ConsistentState())

        }//if(Is32BitFat())



    //-- mark the volume as clean
    SetVolumeCleanL(ETrue);

    //-- finally, put the volume into RO mode if required
    if(aOperation == RFs::EFinal_RO)
        SetReadOnly(ETrue);

    (void)LocalDrive()->Finalise(ETrue);

    SetState(EFinalised);
    }


//-------------------------------------------------------------------------------------------------------------------

/**
@return ETrue if "VolumeClean" flag is supported i.e. this is not FAT12
*/
TBool CFatMountCB::VolCleanFlagSupported() const
    {
    const TFatType fatType=FatType();

    ASSERT(fatType == EFat12 || fatType == EFat16 || fatType == EFat32);
    return (fatType != EFat12);
    }


//-----------------------------------------------------------------------------------------
/**
    @return Volume size in bytes according to the number of usable clusters.
    This approach is not applicable to RAM drive, because its size isn't fixed and can be adjusted by the system.
*/
TUint64 CFatMountCB::VolumeSizeInBytes() const
    {
    ASSERT(ConsistentState());
    ASSERT(!iRamDrive);
    return ((TUint64)UsableClusters()) << ClusterSizeLog2();
    }

//-----------------------------------------------------------------------------------------


/**
    Obtain the volume information.
    All information except iSize and iFree has been added by TDrive::Volume().

    @param  aVolume on return will contain iSize & iFree fields filled with actual data.
*/
void CFatMountCB::VolumeL(TVolumeInfo& aVolume) const
    {

    //-- if true, this operation will be synchronous, i.e the client will be suspended until FAT32 scanning thread finishes, if running.
    //-- the information if this operation is synchronous or not can be passed by client in TVolumeInfo::iFileCacheFlags field.
    //-- if the client sets aVolume.iVolSizeAsync flag there, RFs::Volume() will be asynchronous, i.e the _current_ number of free clusters
    //-- will be returned.
    const TBool bSyncOp = !aVolume.iVolSizeAsync;
    aVolume.iVolSizeAsync = EFalse; //-- reset this flag in order it not to be reused on the client side

	__PRINT2(_L("CFatMountCB::VolumeL() drv:%d, synch:%d"), DriveNumber(), bSyncOp);
    const TDriveInfo& drvInfo=aVolume.iDrive;

#if defined(__EPOC32__)
    // if RAM drive, cap size according to HAL.
    if (drvInfo.iType==EMediaRam)
        {
        TLocalDriveCapsV2Buf caps;
        LocalDrive()->Caps(caps);

        const TInt max_drive_size=TInt(caps().iEraseBlockSize);
        const TInt cur_drive_size=I64INT(caps().iSize);

        aVolume.iSize=max_drive_size;
        aVolume.iFree=max_drive_size-cur_drive_size;

        aVolume.iSize=aVolume.iFree+iSize;

        TInt maxSize;
        if (HAL::Get(HAL::EMaxRAMDriveSize, maxSize) == KErrNone)
            {
            // iSize will never grow beyond maxRam because of a check in medint.
            // d <= f; (s{f} + f) - m <= f; s{f} <= m
            __ASSERT_DEBUG(iSize <= maxSize, Fault(EFatRAMDriveSizeInvalid));
            if (aVolume.iSize > maxSize)
                {
                TInt64 d = aVolume.iSize - maxSize;
                __ASSERT_DEBUG(d <= aVolume.iFree, Fault(EFatRAMDriveFreeInvalid));
                aVolume.iSize -= d;
                aVolume.iFree -= d;
                }
            }

        aVolume.iSize-=ClusterBasePosition(); // Allow for bytes used by FAT etc
        aVolume.iSize=(aVolume.iSize>>ClusterSizeLog2())<<ClusterSizeLog2();  //-- round down to cluster size

        return;
        }//if (drvInfo.iType==EMediaRam)

#endif

    //-- find out number of free clusters on the volume
    TUint32 freeClusters = FAT().NumberOfFreeClusters(EFalse);  //-- this is a _current_ amount of free clusters, this operation is non-blocking

    if(bSyncOp)
        {//-- the "::VolumeL()" query is synchronous, see if we can make it semi-synchronous
        const TUint32 KSyncScanThresholdMB = FatConfig().FAT32_SyncScanThresholdMB();    
        
        if(!KSyncScanThresholdMB)
            {//-- the free clusters scan threshold isn't set, the query is fully synchronous.
             //-- this call will block until FAT scan thread finishes     
            __PRINT1(_L("CFatMountCB::VolumeL() drv:%d #1"), DriveNumber());
            freeClusters = FAT().NumberOfFreeClusters(ETrue);  //-- this will be _true_ amount of free clusters 
            }
         else
            {//-- request number of free clusters enough to satisfy the threshold value
            const TUint32 KClustersRequired = (TUint32)((TUint64)KSyncScanThresholdMB << 20) >> ClusterSizeLog2();    
            __PRINT2(_L("CFatMountCB::VolumeL() drv:%d req clusters:%d"), DriveNumber(), KClustersRequired);
            (void)FAT().RequestFreeClusters(KClustersRequired);  
            freeClusters = FAT().NumberOfFreeClusters(EFalse);  //-- _current_ amount of free clusters, this operation is non-blocking
            }
        
        }
    
    aVolume.iFree = (TInt64)freeClusters << ClusterSizeLog2();
    __PRINT2(_L("CFatMountCB::VolumeL() drv:%d, free clusters:%d"), DriveNumber(), freeClusters);


    if(drvInfo.iType==EMediaRam)
        {//-- a special case. RAM drive size is variable and adjustable. It should be calculated from aVolume.iFree and CMountCB::iFree
        ASSERT(iRamDrive);
        aVolume.iSize=aVolume.iFree+iSize;
        aVolume.iSize-=ClusterBasePosition(); // Allow for bytes used by FAT etc
        aVolume.iSize=(aVolume.iSize >> ClusterSizeLog2()) << ClusterSizeLog2();  //-- round down to cluster size
        }
    else
        {//-- normal case; the volume size is determined by amount of usable clusters
        aVolume.iSize = VolumeSizeInBytes();
        }

    }


//-----------------------------------------------------------------------------------------

//
//  Set the volume label (write aVolume label into BPB & Volume Label File)
//  aName string may be zero length but is assumed to contain no illegal characters or NULLs.
//
void CFatMountCB::SetVolumeL(TDes& aName)
    {

    __PRINT(_L("CFatMountCB::SetVolumeL"));

    CheckStateConsistentL();
    CheckWritableL();

    __ASSERT_ALWAYS(aName.Length()<=KVolumeLabelSize,User::Leave(KErrBadName));

    TBuf8<KVolumeLabelSize> buf8(KVolumeLabelSize);
    buf8.Zero();
	LocaleUtils::ConvertFromUnicodeL(buf8, aName, TFatUtilityFunctions::EOverflowActionLeave);
	aName.Zero();
	LocaleUtils::ConvertToUnicodeL(aName, buf8); // adjust aName (which may contain more underscores after this line than before)

    const TInt lengthOfBuf8=buf8.Length();
    // Pad to end with spaces if not empty.
    if (lengthOfBuf8>0 && lengthOfBuf8<KVolumeLabelSize)
        {
        buf8.SetLength(KVolumeLabelSize);
        Mem::Fill(&buf8[lengthOfBuf8],KVolumeLabelSize-lengthOfBuf8,' ');
        }

    // Write a volume label file
    WriteVolumeLabelFileL( buf8 );

    // Write the boot sector volume label
    // Always pad to full length with spaces
    if (lengthOfBuf8==0)
        {
        buf8.Fill(' ',KVolumeLabelSize);
        }

    WriteVolumeLabelL(buf8);
    }

//-----------------------------------------------------------------------------------------

/**
    Make a directory.
    @param aName full path to the directory to create. Name validity is checked by file server.
    all trailing dots from the name will be removed
*/
void CFatMountCB::MkDirL(const TDesC& aName)
    {
    __PRINT2(_L("CFatMountCB::MkDirL, drv:%d, %S"), DriveNumber(), &aName);

    CheckStateConsistentL();
    CheckWritableL();

    TPtrC dirName = RemoveTrailingDots(aName); //-- remove trailing dots from the name

    TInt namePos=dirName.LocateReverse(KPathDelimiter)+1; // There is always a path delimiter
    TPtrC name=dirName.Mid(namePos);
    TLeafDirData leafDir;
    const TEntryPos dirPos(FindLeafDirL(dirName.Left(namePos), leafDir),0);
    TEntryPos dumPos=dirPos;
    TFatDirEntry dumEntry;

    TBool isOriginalNameLegal = IsLegalDosName(name,EFalse,EFalse,EFalse,EFalse,ETrue);
    iFileCreationHelper.InitialiseL(name);
    TFileName fileName;
    TEntryPos startPos;
    TFatDirEntry startEntry;
    
    TRAPD(ret,DoFindL(name,KEntryAttMaskSupported,
			    		startPos,startEntry,dumPos,dumEntry,
			    		fileName,KErrNotFound,
			    		&iFileCreationHelper,
			    		leafDir));

    if (ret!=KErrNotFound && ret!=KErrNone)
        User::Leave(ret);
    if (ret!=KErrNotFound)
        {
        if (dumEntry.Attributes()&KEntryAttDir)
            User::Leave(KErrAlreadyExists);
        else
            User::Leave(KErrAccessDenied);
        }
    TShortName shortName;

    if (iFileCreationHelper.GetValidatedShortName(shortName) == KErrNotFound)
    	{
    	GenerateShortNameL(dirPos.iCluster,name,shortName,ETrue);
    	}

    TInt numEntries=1;
    if (isOriginalNameLegal==EFalse)
        numEntries=NumberOfVFatEntries(name.Length());
    dumPos=dirPos;

    if (iFileCreationHelper.IsNewEntryPosFound())
    	{
    	dumPos = iFileCreationHelper.EntryAddingPos();
    	}

    AddDirEntryL(dumPos,numEntries);    //  Directory entry in leaf directory
    TInt startCluster;
    FOREVER
        {
        //-- FAT().FreeClusterHint() will give us a hint of the last free cluster
        startCluster=FAT().AllocateSingleClusterL(dumPos.iCluster ? dumPos.iCluster : FAT().FreeClusterHint());

        FAT().FlushL();
        TRAPD(r, InitializeFirstDirClusterL(startCluster,dirPos.iCluster));
        if(r == KErrNone)
            break;
        if(r != KErrCorrupt)
            User::Leave(r);
        FAT().MarkAsBadClusterL(startCluster);
        }
    TFatDirEntry fatDirEntry;
    fatDirEntry.SetName(shortName);
    fatDirEntry.SetAttributes(KEntryAttDir);
    TTime now;
	now.UniversalTime();
    fatDirEntry.SetTime(now, TimeOffset());
    fatDirEntry.SetCreateTime(now, TimeOffset());
    fatDirEntry.SetStartCluster(startCluster);
    fatDirEntry.SetSize(0);
    if (isOriginalNameLegal)
        WriteDirEntryL(dumPos,fatDirEntry);
    else
        WriteDirEntryL(dumPos,fatDirEntry,name);

    iFileCreationHelper.Close();
    }

//-----------------------------------------------------------------------------------------

/**
    Setup 1st cluster of the new directory

    @param  aStartCluster   this entry start cluster number
    @param  aParentCluster  parent entry start cluster number
*/
void CFatMountCB::InitializeFirstDirClusterL(TUint32 aStartCluster, TUint32 aParentCluster)
    {
    const TUint32 KClusterSz= 1<<ClusterSizeLog2();
    const TUint32 KMaxBufSz = KClusterSz;           //-- max. nuffer size is a cluster
    const TUint32 KMinBufSz = 1<<SectorSizeLog2();  //-- min. buffer size is 1 sector (for OOM case)

    //-- allocate a buffer for directory file 1st cluster initialisation
    RBuf8 buf;
    CleanupClosePushL(buf);

    if(buf.CreateMax(KMaxBufSz) != KErrNone)
        buf.CreateMaxL(KMinBufSz); //-- OOM, try to create smaller buffer

    buf.FillZ();

    //-- copy "." directory entry to the buffer

    //-- "." directory entry
    TFatDirEntry entry;
    TTime now;
	now.UniversalTime();
    entry.SetTime(now, TimeOffset() );
    entry.SetAttributes(KEntryAttDir);
    entry.SetCurrentDirectory();
    entry.SetStartCluster(aStartCluster);
    Mem::Copy(&buf[0],&entry,KSizeOfFatDirEntry);

    //-- append ".." directory entry
    entry.SetParentDirectory();
    entry.SetStartCluster(aParentCluster==RootIndicator() ? 0 : aParentCluster);
    Mem::Copy(&buf[0]+KSizeOfFatDirEntry,&entry,KSizeOfFatDirEntry);

    TEntryPos entryPos(aStartCluster,0);

    //-- write buffer to the beginning of the directory file.
    DirWriteL(entryPos, buf); //-- use special interface to access FAT directory file

    //-- fill in the rest of the cluster if we used a small buffer
    if((TUint32)buf.Size() < KClusterSz) //-- use special interface to access FAT directory file
    {
        buf.FillZ();
        const TInt restCnt = SectorsPerCluster() - 1;
        ASSERT(restCnt >=1);

        for(TInt i=0; i<restCnt; ++i)
        {
            entryPos.iPos += KMinBufSz;
            DirWriteL(entryPos, buf); //-- use directory cache when dealing with directories
        }

    }

    CleanupStack::PopAndDestroy(&buf);

    }

//-----------------------------------------------------------------------------------------

/**
    Remove a directory.
    @param aName directory name
    all trailing dots from the name will be removed
*/
void CFatMountCB::RmDirL(const TDesC& aName)
    {
    __PRINT2(_L("CFatMountCB::RmDirL, drv:%d, %S"), DriveNumber(), &aName);

    CheckStateConsistentL();
    CheckWritableL();

    TPtrC dirName = RemoveTrailingDots(aName); //-- remove trailing dots from the name

    TFatDirEntry dirEntry;
    TEntryPos dirEntryPos(RootIndicator(),0); // Already checked entry is a directory
    FindEntryStartL(dirName,KEntryAttMatchMask|KEntryAttMatchExclusive,dirEntry,dirEntryPos);
    TEntryPos dosEntryPos=dirEntryPos;
    TFatDirEntry dosEntry=dirEntry;
    MoveToDosEntryL(dosEntryPos,dosEntry);
    if (!IsDirectoryEmptyL(StartCluster(dosEntry)))
        User::Leave(KErrInUse);
    // Remove the directory from cache before erasing
    if(iLeafDirCache && iLeafDirCache->CacheCount() > 0)
    	{
    	iLeafDirCache->RemoveDirL(StartCluster(dosEntry));
    	}

    EraseDirEntryL(dirEntryPos,dirEntry);
    FAT().FreeClusterListL(StartCluster(dosEntry));
    FAT().FlushL();
    }

//-----------------------------------------------------------------------------------------

/**
    Delete a file
    @param aName file name
    all trailing dots from the name will be removed
*/
void CFatMountCB::DeleteL(const TDesC& aName)
    {
    __PRINT2(_L("CFatMountCB::DeleteL, drv:%d, %S"), DriveNumber(), &aName);

    CheckStateConsistentL();
    CheckWritableL();

    TPtrC fullName = RemoveTrailingDots(aName); //-- remove trailing dots from the name

    TFatDirEntry firstEntry;
    TEntryPos firstEntryPos(RootIndicator(),0);
    FindEntryStartL(fullName,KEntryAttMaskSupported,firstEntry,firstEntryPos);
    TEntryPos dosEntryPos=firstEntryPos;
    TFatDirEntry dosEntry=firstEntry;
    MoveToDosEntryL(dosEntryPos,dosEntry);
    if ((dosEntry.Attributes()&KEntryAttReadOnly) || (dosEntry.Attributes()&KEntryAttDir))
        User::Leave(KErrAccessDenied);
    // Can not delete a file if it is clamped
    CMountCB* basePtr=(CMountCB*)this;
    TInt startCluster=StartCluster(dosEntry);
    if(basePtr->IsFileClamped(MAKE_TINT64(0,startCluster)) > 0)
        User::Leave(KErrInUse);
    EraseDirEntryL(firstEntryPos,firstEntry);
    FAT().FreeClusterListL(StartCluster(dosEntry));
    FAT().FlushL();
    }

//-----------------------------------------------------------------------------------------

/**

    Rename or replace a directory entry.
    Assumes all files are closed and replace is only passed files.
    Assumes rename target does not exist or is the source file.

    --------------- operating mode --------------------------------------------

    * rename mode

    aOldName exists  |  aNewName exists |   result
        N                    N              leave KErrNotFound
        N                    Y              leave KErrNotFound
        Y                    N              rename aOldName -> aNewName
        Y                    Y              leave KErrAlreadyExists if(aOldName!=aNewName); otherwise do nothing

    * replace mode

        N                    N              leave KErrNotFound
        N                    Y              leave KErrNotFound
        Y                    N              rename aOldName -> aNewName
        Y                    Y              contents and all file attributes of the "aNewName" are replaced with aOldName's. "aOldName" entries are deleted then.


    @param   aOldName           entry name to be renamed or replaced
    @param   aNewName           a new entry name
    @param   aMode              specifies renaming / replacing
    @param   aNewDosEntryPos    on exit contains new entry Pos.
*/
void CFatMountCB::DoRenameOrReplaceL(const TDesC& aOldName, const TDesC& aNewName, TRenMode aMode, TEntryPos& aNewName_DosEntryPos)
	{
    __PRINT3(_L("CFatMountCB::DoRenameOrReplaceL() mode:%d old:%S, new:%S"), aMode, &aOldName, &aNewName);

    const TBool namesAreIdentical = FileNamesIdentical(aOldName, aNewName); //-- this is case-insensitive.
    const TBool renameMode = (aMode == EModeRename);
    const TBool replaceMode = !renameMode;
    TInt  nRes;

    if(namesAreIdentical && replaceMode)
        return; //-- nothing to do, replacing file with itself

    //---------------------------------------------------------------------------------------------------------------------------
    //-- 1. find the entries of 'aOldName' file. It must always succeed, because FileServer firstly tries to locate 'aOldName'

    TFatDirEntry oldName_FirstEntry; //-- first entry of the "aOldName" entryset
    TEntryPos    oldName_FirstEntryPos(RootIndicator(), 0); //-- dir. pos of the start "aOldName" VFAT entry set

    FindEntryStartL(aOldName, KEntryAttMaskSupported, oldName_FirstEntry, oldName_FirstEntryPos);

    TFatDirEntry oldName_DosEntry    = oldName_FirstEntry;   //-- "aOldName" entry set DOS entry
    TEntryPos    oldName_DosEntryPos = oldName_FirstEntryPos;//-- dir. pos of the "aOldName" DOS entry

    MoveToDosEntryL(oldName_DosEntryPos, oldName_DosEntry);

    const TBool bOldNameIsVFAT = !(oldName_DosEntryPos == oldName_FirstEntryPos); //-- ETrue if "aOldName" is VFAT name, i.e. consists of mode than 1 entry

    //-- check if the file "aOldName" is clamped. In this case it can't be replaced.
    if(replaceMode && (IsFileClamped(StartCluster(oldName_DosEntry)) > 0))
        User::Leave(KErrInUse);

    //---------------------------------------------------------------------------------------------------------------------------
    //-- 2. find the entry of 'aNewName' file. Further behavior depends on rename/replace mode and if this file exists or not

    //-- extract new file name from the full path
    TPtrC ptrNewName;
    TPtrC ptrNewNameParentDir;
    const TInt delimPos = aNewName.LocateReverse(KPathDelimiter);
    ptrNewName.Set(aNewName.Mid(delimPos+1));
    ptrNewNameParentDir.Set(aNewName.Left(delimPos+1));

    //-- find the parent directory of the "aNewName" and create iterator for it
    TLeafDirData leafDir;
    const TEntryPos aNewName_ParentDirPos = TEntryPos(FindLeafDirL(ptrNewNameParentDir, leafDir), 0); //-- 'aNewName' parent directory iterator
    aNewName_DosEntryPos = aNewName_ParentDirPos;

    TEntryPos    newName_VFatEntryPos; //-- dir. pos of the start "aNewName" VFAT entry set
    TFatDirEntry newName_DosEntry;

    TFileName fileName;
    iFileCreationHelper.InitialiseL(ptrNewName);
    TFatDirEntry startEntry;

    TRAP(nRes, DoFindL(ptrNewName, KEntryAttMaskSupported,
	    		newName_VFatEntryPos, startEntry, aNewName_DosEntryPos, newName_DosEntry,
	    		fileName, KErrNotFound,
	    		&iFileCreationHelper,
	    		leafDir));

    if (nRes!=KErrNone && nRes!=KErrNotFound)
        User::Leave(nRes);

    const TBool newFileExists = (nRes == KErrNone); //-- ETrue if 'aNewName' file exists.
    const TBool bNewNameIsVFAT = !IsLegalDosName(ptrNewName, EFalse, EFalse, EFalse, EFalse, ETrue);

    if(!newFileExists)
    {//-- invalidate directory iterators if aNewName doesn't exist
        newName_VFatEntryPos.SetEndOfDir();
        aNewName_DosEntryPos.SetEndOfDir();
    }


    if(renameMode && newFileExists)
    	{
        if(!namesAreIdentical)
        {
        if ((newName_DosEntry.Attributes()&KEntryAttDir) != (oldName_DosEntry.Attributes()&KEntryAttDir))
        	{
        	User::Leave(KErrAccessDenied); 	//-- leave with KErrAccessDenied if it is trying to rename a file
        									//		to a dir or vice versa.
        	}
        User::Leave(KErrAlreadyExists); //-- can't rename file if the file with 'aNewName' already exists
        }
        else
        	{
            if(!bNewNameIsVFAT && !bOldNameIsVFAT)
                return; //-- renaming DOS name to itself
        	}
        //-- allow renaming entry to itself. "namesAreIdentical" is case-insensitive. use case: "FILE" -> "File"
    	}

    //---------------------------------------------------------------------------------------------------------------------------

    if(replaceMode && newFileExists)
    	{
        //---------------------------------------------------------------------------------------------------------------------------
        //-- replace contents of the 'aNewName' with 'aOldName' and remove 'aOldName' entries.

        //-- check if we are still trying to replace the file with itself, probably using short name alias
        if(aNewName_DosEntryPos == oldName_DosEntryPos)
            return; //-- nothing to do, it's the same file

        const TInt oldNameStartCluster = StartCluster(oldName_DosEntry);
        const TInt newNameStartCluster = StartCluster(newName_DosEntry); //-- store starting cluster of the chain to be unlinked

        newName_DosEntry.SetStartCluster(oldNameStartCluster);
        newName_DosEntry.SetSize(oldName_DosEntry.Size());
        newName_DosEntry.SetTime(oldName_DosEntry.Time(TTimeIntervalSeconds(0)), TTimeIntervalSeconds(0));
        newName_DosEntry.SetAttributes(oldName_DosEntry.Attributes());

            if(IsRuggedFSys())
            	{
            	//-- Note 1.
                //-- set a special Id in reserved section for old and new entries.
                //-- if write fails before the old entry gets erased, we will have 2 entries pointing to the same clusterchain.
                //-- ScanDrive is responsible for fixing this situation by erasing entry with ID KReservedIdOldEntry.
                //-- note that  SetRuggedFatEntryId() uses "LastAccessTime" DOS FAT entry field to store the ID.
                //-- in normal situation this field isn't used, though Windows checkdisk can chack its validiy.
                //-- KReservedIdNewEntry == 0x0000 that corresponds to year 1980.

	            newName_DosEntry.SetRuggedFatEntryId(KReservedIdNewEntry);
	            oldName_DosEntry.SetRuggedFatEntryId(KReservedIdOldEntry);
	            WriteDirEntryL(oldName_DosEntryPos, oldName_DosEntry);
                }

        //-- write 'aNewName' DOS dir. entry data back
        WriteDirEntryL(aNewName_DosEntryPos, newName_DosEntry);

        //-- erase "oldName" entryset.
        EraseDirEntryL(oldName_FirstEntryPos, oldName_FirstEntry);

        //-- free 'aNewName' cluster list
        FAT().FreeClusterListL(newNameStartCluster);

        if(IsRuggedFSys())
            FAT().FlushL();

    	}
    else //if(replaceMode && newFileExists)
    	{
        //---------------------------------------------------------------------------------------------------------------------------
        //-- Renaming 'aOldName' to 'aNewName': add 'aNewName' entry set and remove 'aOldName' entryset

        TFatDirEntry newDosEntry = oldName_DosEntry;
        //-- generate short name for the 'aNewName' entryset and make new DOS entry
        if(bNewNameIsVFAT)
        	{//-- need to generate a short name for VFAT entryset DOS entry
            TShortName shortName;

		    if (iFileCreationHelper.GetValidatedShortName(shortName) == KErrNotFound)
		    	{
		        GenerateShortNameL(aNewName_ParentDirPos.Cluster(), ptrNewName, shortName); 
		    	}

            newDosEntry.SetName(shortName);
        	}
        else
        	{//-- just use 'aNewName' as DOS name.
            TBuf8<KFatDirNameSize+1> tmp; //-- the name may be "XXXXXXXX.YYY"
            tmp.Copy(ptrNewName);
            newDosEntry.SetName(DosNameToStdFormat(tmp));
        	}

        if(IsRuggedFSys())
        	{//-- the the note(1) above
            newDosEntry.SetRuggedFatEntryId(KReservedIdNewEntry);
            oldName_DosEntry.SetRuggedFatEntryId(KReservedIdOldEntry);
            WriteDirEntryL(oldName_DosEntryPos, oldName_DosEntry);
        	}

        //-- add new entryset to the directory
        aNewName_DosEntryPos.iPos = 0;
        aNewName_DosEntryPos.iCluster = aNewName_ParentDirPos.Cluster();

	    if (iFileCreationHelper.IsNewEntryPosFound())
	    	{
	    	aNewName_DosEntryPos = iFileCreationHelper.EntryAddingPos();
	    	}

        if(bNewNameIsVFAT)
        	{
            const TInt numEntries = NumberOfVFatEntries(ptrNewName.Length());
            AddDirEntryL(aNewName_DosEntryPos, numEntries);
            WriteDirEntryL(aNewName_DosEntryPos, newDosEntry, ptrNewName);
            }
        else
            {//-- new name is one DOS entry only
            AddDirEntryL(aNewName_DosEntryPos, 1);
            WriteDirEntryL(aNewName_DosEntryPos, newDosEntry);
            }

        //-- erase old entryset.
        EraseDirEntryL(oldName_FirstEntryPos, oldName_FirstEntry);

        //-- if we have renamed (moved) a directory, need to update its pointer to parent directory ('..' entry)
        if((newDosEntry.Attributes() & KEntryAttDir))
        	{
            TEntryPos parentPtrEntPos(StartCluster(newDosEntry), 1*KSizeOfFatDirEntry);

            TFatDirEntry chFatEnt;
            ReadDirEntryL(parentPtrEntPos, chFatEnt);

            const TUint parentDirStartCluster_Old = StartCluster(chFatEnt);
                  TUint parentDirStartCluster_New = aNewName_ParentDirPos.Cluster();

            if(parentDirStartCluster_New == RootClusterNum() && parentDirStartCluster_New != 0)
            	{//-- we are in the root directory. for some reason, '..' entries of the directories in the root dir.
            	//-- must have starting cluster 0
                parentDirStartCluster_New = 0;
            	}

            if(parentDirStartCluster_Old != parentDirStartCluster_New)
            	{
                chFatEnt.SetStartCluster(parentDirStartCluster_New);
                WriteDirEntryL(parentPtrEntPos, chFatEnt);
            	}
            // Invalidate leaf dir cache as it is hard to track the dir structure changes now
            if (iLeafDirCache)
            	{
                iLeafDirCache->Reset();
            	}
        	}
    	}//else if(replaceMode && newFileExists)

    iFileCreationHelper.Close();
	}

//-----------------------------------------------------------------------------------------

/**
    Rename 'aOldName' file/directory to 'aNewName'
    all trailing dots from the names will be removed

    @param  aOldName        existing object name
    @param  aNewName        new object name
*/
void CFatMountCB::RenameL(const TDesC& aOldName, const TDesC& aNewName)
    {
    __PRINT3(_L("CFatMountCB::RenameL, drv:%d, %S %S"), DriveNumber(), &aOldName, &aNewName);

    CheckStateConsistentL();
    CheckWritableL();

    TEntryPos newEntryPos;
    DoRenameOrReplaceL(RemoveTrailingDots(aOldName), RemoveTrailingDots(aNewName) ,EModeRename, newEntryPos);

    if(!IsRuggedFSys())
        FAT().FlushL();
    }

//-----------------------------------------------------------------------------------------

/**
    Replace contents of the 'aNewName' with the contents of 'aOldName'
    all trailing dots from the names will be removed

    @param  aOldName        existing object name
    @param  aNewName        new object name
*/
void CFatMountCB::ReplaceL(const TDesC& aOldName,const TDesC& aNewName)
    {

    __PRINT3(_L("CFatMountCB::ReplaceL, drv:%d, %S %S"), DriveNumber(), &aOldName, &aNewName);

    CheckStateConsistentL();
    CheckWritableL();

    TEntryPos newEntryPos;
    DoRenameOrReplaceL(RemoveTrailingDots(aOldName), RemoveTrailingDots(aNewName), EModeReplace, newEntryPos);
    if(!IsRuggedFSys())
        FAT().FlushL();
    }

//-----------------------------------------------------------------------------------------


/**
    Try to find a directory entry by the given name and path.
    This method _must_ leave if the entry is not found. See the caller.

    @param  aName   path to the directory object. all trailing dots from the name will be removed.
    @param  anEntry on return will contain the entry data

    @leave  KErrPathNotFound if there is no path to the aName
            KErrNotFound     if the entry corresponding to the aName is not found
            system-wide erorr code of media read failure.
*/
void CFatMountCB::EntryL(const TDesC& aName,TEntry& anEntry) const
    {
    __PRINT2(_L("CFatMountCB::EntryL, drv:%d, %S"), DriveNumber(), &aName);

    CheckStateConsistentL();

    TEntryPos entryPos(RootIndicator(),0);
    TFatDirEntry entry;
    TPtr fileName(anEntry.iName.Des());

    TPtrC fullName = RemoveTrailingDots(aName);
    TInt namePos=fullName.LocateReverse(KPathDelimiter)+1; // There is always a path delimiter
    TLeafDirData leafDir;
    entryPos.iCluster=FindLeafDirL(fullName.Left(namePos), leafDir);
    entryPos.iPos=0;
    TEntryPos startPos;
    TFatDirEntry startEntry;

    DoFindL(fullName.Mid(namePos),KEntryAttMaskSupported,
    		startPos,startEntry,entryPos,entry,
    		fileName,KErrNotFound,
    		NULL,
    		leafDir);


    anEntry.iAtt=entry.Attributes();
    anEntry.iSize=entry.Size();
    anEntry.iModified=entry.Time(TimeOffset());

	if (fileName.Length()==0)
        {
        TBuf8<0x20> dosName(DosNameFromStdFormat(entry.Name()));
        LocaleUtils::ConvertToUnicodeL(fileName,dosName);
        }
    if ((TUint)anEntry.iSize>=sizeof(TCheckedUid))
        ReadUidL(StartCluster(entry),anEntry);
    }

//-----------------------------------------------------------------------------------------

/**
    Set directory entry details.
    @param  aName           entry name; all trailing dots from the name will be removed
    @param  aTime           entry modification time (and last access as well)
    @param  aSetAttMask     entry attributes OR mask
    @param  aClearAttMask   entry attributes AND mask

*/
void CFatMountCB::SetEntryL(const TDesC& aName,const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask)
    {
    __PRINT2(_L("CFatMountCB::SetEntryL, drv:%d, %S"), DriveNumber(), &aName);

    CheckStateConsistentL();
    CheckWritableL();

    TEntryPos firstEntryPos(RootIndicator(),0);
    TFatDirEntry firstEntry;
    FindEntryStartL(RemoveTrailingDots(aName),KEntryAttMaskSupported,firstEntry,firstEntryPos);
    MoveToDosEntryL(firstEntryPos,firstEntry);
    TUint setAttMask=aSetAttMask&KEntryAttMaskSupported;
	TInt oldAtt = firstEntry.Attributes();
	TInt att = oldAtt;
    if (setAttMask|aClearAttMask)
        {
        att|=setAttMask;
        att&=(~aClearAttMask);
        firstEntry.SetAttributes(att);
        }
    if (aSetAttMask&KEntryAttModified)
		{
		firstEntry.SetTime(aTime,TimeOffset());
		}
	else if (att == oldAtt)
		return;					// no change - don't bother writing entry
    WriteDirEntryL(firstEntryPos,firstEntry);
    }

//-----------------------------------------------------------------------------------------

void CFatMountCB::DoCheckFatForLoopsL(TUint32 aCluster, TUint32& aPreviousCluster, TUint32& aChangePreviousCluster, TUint32& aCount) const
//
// Check one fat cluster for loops.
//
    {

    if (aCluster==aPreviousCluster)
        User::Leave(KErrCorrupt); // Found loop

    aCount++;
    if (aCount==aChangePreviousCluster)
        {
        aCount=0;
        aChangePreviousCluster<<=1;
        aPreviousCluster=aCluster;
        }
    }

//-----------------------------------------------------------------------------------------

void CFatMountCB::CheckFatForLoopsL(const TFatDirEntry& anEntry) const
//
// Check for loops
//
    {

    TUint32 cluster = StartCluster(anEntry);
    if (cluster==0 && anEntry.Size()==0)
        return;

    TUint32 previousCluster=cluster;
    TUint32 changePreviousCluster=1;
    TUint32 count=0;


    for(;;)
        {
        if ((TUint)cluster < KFatFirstSearchCluster || (!IsEndOfClusterCh(cluster) && (TUint)cluster>MaxClusterNumber()))
            User::Leave(KErrCorrupt);

         if(!FAT().GetNextClusterL(cluster))
            break;

         DoCheckFatForLoopsL(cluster, previousCluster, changePreviousCluster, count);
        }

    }

//-----------------------------------------------------------------------------------------

/**
    Open/Create/Replace a file on the current mount.

    @param  aName   file name; all trailing dots from the name will be removed
    @param  aMode   File open mode, See TFileMode
    @param  anOpen  specifies action: open, create or replace the file
    @param  aFile   pointer to the CFileCB object to populate

*/
void CFatMountCB::FileOpenL(const TDesC& aName,TUint aMode,TFileOpen anOpen,CFileCB* aFile)
    {
    __PRINT3(_L("CFatMountCB::FileOpenL, drv:%d, mode:%d, name:%S"), DriveNumber(), anOpen, &aName);

    CheckStateConsistentL();

    TPtrC fullName = RemoveTrailingDots(aName); //-- remove trailing dots from the name

    TFatDirEntry firstEntry;
    TEntryPos firstEntryPos(RootIndicator(),0);
    TInt nPos=fullName.LocateReverse(KPathDelimiter)+1; // There is always a path delimiter
    TPtrC name(fullName.Mid(nPos));
    TInt ret = KErrNone;

    iFileCreationHelper.Close();
    if (anOpen == EFileCreate || anOpen == EFileReplace)
    	{
    	iFileCreationHelper.InitialiseL(name);
        TRAP(ret,FindEntryStartL(fullName,KEntryAttMaskSupported,firstEntry,firstEntryPos,&iFileCreationHelper));
    	}
    else
        {
        TRAP(ret,FindEntryStartL(fullName,KEntryAttMaskSupported,firstEntry,firstEntryPos));
        }

    if (ret!=KErrNone && ret!=KErrNotFound)
        User::Leave(ret);

    if (ret==KErrNone)
        {
        MoveToDosEntryL(firstEntryPos,firstEntry);
        if ((firstEntry.Attributes()&KEntryAttDir) || (firstEntry.Attributes()&KEntryAttVolume))
            User::Leave(KErrAccessDenied);
        if (anOpen==EFileCreate)
            User::Leave(KErrAlreadyExists);
        if ((firstEntry.Attributes()&KEntryAttReadOnly) && aMode&EFileWrite)
            User::Leave(KErrAccessDenied);
        if((aMode & EFileWrite) && (IsFileClamped(StartCluster(firstEntry))>0))
            User::Leave(KErrInUse);
        CheckFatForLoopsL(firstEntry);
        }
    else
        {
        if (anOpen==EFileOpen)
            User::Leave(KErrNotFound);

        //-- here we try to either create or replace file
        CheckWritableL();

    	TLeafDirData leafDir;

        TInt numEntries = iFileCreationHelper.NumOfAddingEntries();
        TShortName shortName;
        if (iFileCreationHelper.GetValidatedShortName(shortName) == KErrNotFound)
        	{
            firstEntryPos.iCluster=FindLeafDirL(fullName.Left(nPos), leafDir);
            GenerateShortNameL(firstEntryPos.iCluster,name,shortName,ETrue);
        	}

        if (iFileCreationHelper.IsNewEntryPosFound())
	    	{
	    	firstEntryPos = iFileCreationHelper.EntryAddingPos();
	    	}
        else
        	{
        	firstEntryPos.iCluster=FindLeafDirL(fullName.Left(nPos), leafDir);
        	firstEntryPos.iPos=0;
        	}

        AddDirEntryL(firstEntryPos,numEntries);
        firstEntry.InitZ();
        firstEntry.SetName(shortName);
        firstEntry.SetStartCluster(0);

        TTime now;
		now.UniversalTime();
        firstEntry.SetCreateTime(now, TimeOffset() );

        if (iFileCreationHelper.IsTrgNameLegalDosName())
            WriteDirEntryL(firstEntryPos,firstEntry);
        else
            WriteDirEntryL(firstEntryPos,firstEntry,name);
        }

    CFatFileCB& file=(*((CFatFileCB*)aFile));
    file.SetupL(firstEntry, firstEntryPos);

    if (anOpen==EFileReplace && file.Size())
        {
        file.SetSizeL(0);
        }

    if (anOpen==EFileReplace || anOpen==EFileCreate)
        file.SetArchiveAttribute();

    if(!IsRuggedFSys())
        FAT().FlushL();

    iFileCreationHelper.Close();
    }

//-----------------------------------------------------------------------------------------


/**
    Open a directory on the current mount.

    @param  aName   path to the object in the directory we want to open; all trailing dots from the name will be removed
    @param  aDir    dir. CB to be filled in.

    If there is no such a path, this method must leave with KErrPathNotFound

    @leave  KErrPathNotFound if thereis no such path
    @leave  error code on media read fault
*/
void CFatMountCB::DirOpenL(const TDesC& aName,CDirCB* aDir)
    {
    __PRINT2(_L("CFatMountCB::DirOpenL, drv:%d, %S"), DriveNumber(), &aName);

    CheckStateConsistentL();

    const TPtrC dirName = RemoveTrailingDots(aName); //-- remove trailing dots from the name

    TInt namePos=dirName.LocateReverse(KPathDelimiter);

    TFatDirEntry dosEntry;
    TEntryPos dosEntryPos(RootIndicator(),0);
    if (namePos==0)
        InitializeRootEntry(dosEntry);
    else
        {
        TPtrC dirPath=dirName.Left(namePos);
        TInt dirPos=dirPath.LocateReverse(KPathDelimiter)+1;
        TLeafDirData leafDir;
        dosEntryPos.iCluster=FindLeafDirL(dirPath.Left(dirPos), leafDir); // Find directory before leaf
        dosEntryPos.iPos=0;

        TFileName fileName;
        TEntryPos startPos;
        TFatDirEntry startEntry;
        DoFindL(dirPath.Mid(dirPos),
        		KEntryAttMatchMask|KEntryAttMatchExclusive,
        		startPos, startEntry, dosEntryPos, dosEntry,
        		fileName, KErrPathNotFound,
        		NULL,
        		leafDir);


        }

    TPtrC matchName(dirName.Mid(namePos+1));
    if (matchName.Length()==0)
        matchName.Set(_L("*"));

    ((CFatDirCB*)aDir)->SetDirL(dosEntry,matchName);

    }

//-----------------------------------------------------------------------------------------

//
// Check aCluster contains no directory entries other than . and ..
//
TBool CFatMountCB::IsDirectoryEmptyL(TUint32 aCluster)
    {

    __PRINT(_L("CFatMountCB::IsDirectoryEmptyL"));
    TEntryPos dirEntryPos(aCluster,0);
    TFatDirEntry dirEntry;
    FOREVER
        {
        ReadDirEntryL(dirEntryPos,dirEntry);
        MoveToDosEntryL(dirEntryPos,dirEntry);
        if (dirEntry.IsParentDirectory() || dirEntry.IsCurrentDirectory())
            goto LoopEnd;
        
        if (dirEntry.IsEndOfDirectory())
            return ETrue;
        
        if (IsRootDir(dirEntryPos)&&(dirEntryPos.iPos+StartOfRootDirInBytes()==RootDirEnd()))
            return ETrue;   //  Root Directory has no end of directory marker
        
        if (!dirEntry.IsErased())
            break; //-- found a valid dir. entry
LoopEnd:
        MoveToNextEntryL(dirEntryPos);
        }
    
    return EFalse;
    }

//-----------------------------------------------------------------------------------------

/**
    Overwrite as many contiguous file clusters as possible.
*/
void CFatMountCB::DoWriteToClusterListL(TEntryPos& aPos,TInt aLength,const TAny* aSrc,const RMessagePtr2& aMessage,TInt anOffset, TUint aLastcluster, TUint& aBadcluster, TUint& aGoodcluster, TUint aFlag)
    {

    __PRINT(_L("CFatMountCB::DoWriteToClusterListL"));
    __ASSERT_ALWAYS(aPos.Cluster()>=KFatFirstSearchCluster,User::Leave(KErrCorrupt));

    TUint32 endCluster=0;

    const TInt clusterRelativePos=ClusterRelativePos(aPos.iPos);
    const TInt maxClusters=((aLength+clusterRelativePos-1)>>ClusterSizeLog2())+1;
    const TInt clusterListLen=FAT().CountContiguousClustersL(aPos.iCluster,endCluster,maxClusters);
    const TInt writeLength=Min(aLength,(clusterListLen<<ClusterSizeLog2())-clusterRelativePos);
    TInt64 dataStart=FAT().DataPositionInBytesL(aPos.iCluster)+clusterRelativePos;

    TRAPD(r, iRawDisk->WriteL(dataStart,writeLength,aSrc,aMessage,anOffset, aFlag));

    if(r == KErrNone) // Write succeded
        {
        aPos.iPos+=writeLength;
        aPos.iCluster=endCluster;
        return;
        }

    if(r != KErrCorrupt) // failure not due to corruption so propogate up
        User::Leave(r);

    TErrorInfoBuf errinf;
    r = iRawDisk->GetLastErrorInfo(errinf);

    if(r == KErrNone && errinf().iReasonCode == TErrorInfo::EBadSector) // GetLastErrorInfo succeded and Last Error was caused by bad sector
        {

        const TUint32 badcluster = (TInt)(((dataStart + errinf().iErrorPos) - ClusterBasePosition())>>ClusterSizeLog2())+KFatFirstSearchCluster;
              TUint32 goodcluster = FAT().AllocateSingleClusterL(badcluster);

        //Calculate cluster number to check whether this write started at the beginning of new cluster or middle of previous cluster.
        TUint32 cluster = aPos.iCluster;
        if ( (aPos.iPos) && ((aPos.iPos)==((aPos.iPos >> ClusterSizeLog2())<<ClusterSizeLog2())))
            cluster--;

        if((aPos.iPos != 0) && (badcluster == aPos.iCluster) && (aLastcluster == 0) && (aPos.iCluster == cluster))
            { //Copy the contents already present in this cluster to new cluster allocated.
            const TInt sizeToRead = aPos.iPos - ((aPos.iPos >> ClusterSizeLog2()) << ClusterSizeLog2());
            dataStart = FAT().DataPositionInBytesL(aPos.iCluster) + ClusterRelativePos((aPos.iPos - sizeToRead));


            //-- Allocate the buffer required to copy the contents from bad cluster
            RBuf8 clustBuf;
            CleanupClosePushL(clustBuf);
            if(clustBuf.CreateMax(sizeToRead) != KErrNone)
                {
                FAT().FreeClusterListL(goodcluster);
                User::Leave(KErrNoMemory);
                }

            r = LocalDrive()->Read(dataStart, sizeToRead, clustBuf); //Read the contents into buffer
            if(r != KErrNone) //If read fails dont do anything not even marking bad cluster.
                {
                FAT().FreeClusterListL(goodcluster);
                User::Leave(r);
                }

            //Copy the bad and good cluster,required to adjust the start cluster number.
            if(aBadcluster == 0)
                aBadcluster = badcluster;

            aGoodcluster = goodcluster;

            FOREVER
                {
                //Calculate and copy the contents to new cluster.
                aPos.iCluster = goodcluster;
                dataStart = FAT().DataPositionInBytesL(aPos.iCluster) + ClusterRelativePos(aPos.iPos - sizeToRead);

                r = LocalDrive()->Write(dataStart, clustBuf);
                if(r == KErrNone)
                    { // Copied contents to new cluster so fix up the chain and mark the cluster as bad.
                    FAT().WriteL(goodcluster, FAT().ReadL(badcluster));
                    FAT().MarkAsBadClusterL(badcluster);
                    aGoodcluster = goodcluster;
                    CleanupStack::PopAndDestroy(&clustBuf); //-- deallocate a cluster buffer
                    return;
                    }
                else if(r == KErrCorrupt)
                    {
                    r = LocalDrive()->GetLastErrorInfo(errinf);
                    if(r == KErrNone && errinf().iReasonCode == TErrorInfo::EBadSector)
                        { //Allocate new cluster and adjust the cluster list.
                        goodcluster = FAT().AllocateSingleClusterL(aPos.iCluster);
                        FAT().MarkAsBadClusterL(aPos.iCluster);
                        continue;
                        }
                        r = KErrCorrupt;
                    }
                    //Not able to write successfully so dont alter the original list.
                    aBadcluster = aGoodcluster = 0;
                    FAT().FreeClusterListL(goodcluster);
                    User::Leave(r);
                }

        }//if((aPos.iPos != 0) && (badcluster == aPos.iCluster) && (aLastcluster == 0) && (aPos.iCluster == cluster))

        if((badcluster == aPos.iCluster) && (aLastcluster == 0)) //bad cluster at beginning of original clusterlist
            {
            // return bad and good clusters for CFatFileCB to fix up
            FAT().WriteL(goodcluster, FAT().ReadL(badcluster));
            aBadcluster = badcluster;
            aGoodcluster = goodcluster;
            aPos.iCluster = goodcluster;
            }
        else    //fix up chain
            {
            FAT().WriteL(goodcluster, FAT().ReadL(badcluster));
            if(badcluster > aPos.iCluster)  //bad cluster not first in this contiguous list
                FAT().WriteL(badcluster-1, goodcluster);
            else    //first cluster of this contigous list bad so update last cluster of previous contiguous list
                FAT().WriteL(aLastcluster, goodcluster);
            }

        FAT().MarkAsBadClusterL(badcluster);


        return;
        }
    User::Leave(KErrCorrupt);
    }

//-----------------------------------------------------------------------------------------

void CFatMountCB::WriteToClusterListL(TEntryPos& aPos,TInt aLength,const TAny* aSrc,const RMessagePtr2& aMessage,TInt anOffset, TUint& aBadcluster, TUint& aGoodcluster, TUint aFlag)
//
// Overwrite cluster list.
//
    {

    __PRINT(_L("CFatMountCB::WriteToClusterListL"));
    __ASSERT_ALWAYS(aPos.Cluster()>=KFatFirstSearchCluster,User::Leave(KErrCorrupt));

    const TUint startPos=aPos.iPos;
    const TUint temp=startPos>>ClusterSizeLog2();
    const TUint length = (TUint)aLength;

    if ( (startPos) && ((startPos)==(temp<<ClusterSizeLog2())) )
        {
        __ASSERT_ALWAYS(FAT().GetNextClusterL(aPos.iCluster),User::Leave(KErrCorrupt));
        }

    TUint offset=0;
    TInt previouscluster=0;
    FOREVER
        {
        DoWriteToClusterListL(aPos,length-offset,aSrc,aMessage,anOffset+offset, previouscluster, aBadcluster, aGoodcluster, aFlag);
        if (offset == (aPos.iPos-startPos))
            continue;
        offset=aPos.iPos-startPos;
        __ASSERT_ALWAYS(aPos.iPos>startPos,User::Leave(KErrCorrupt));
        previouscluster=aPos.iCluster;
        if (offset<length)
            {__ASSERT_ALWAYS(FAT().GetNextClusterL(aPos.iCluster),User::Leave(KErrCorrupt));}
        if (offset>=length)
            break;
        }
    }

//-----------------------------------------------------------------------------------------

void CFatMountCB::DoReadFromClusterListL(TEntryPos& aPos,TInt aLength,const TAny* aTrg,const RMessagePtr2& aMessage,TInt anOffset, TUint aFlag) const
//
// Read from as many contiguous file clusters as possible
//
    {

    __PRINT(_L("CFatMountCB::DoReadFromClusterListL"));

    TUint32 endCluster=0;

    const TInt clusterRelativePos=ClusterRelativePos(aPos.iPos);
    const TInt maxClusters=((aLength+clusterRelativePos-1)>>ClusterSizeLog2())+1;
    const TInt clusterListLen=FAT().CountContiguousClustersL(aPos.iCluster,endCluster,maxClusters);
    const TInt readLength=Min(aLength,(clusterListLen<<ClusterSizeLog2())-clusterRelativePos);
    const TInt64 dataStart=FAT().DataPositionInBytesL(aPos.iCluster)+clusterRelativePos;

    TRAPD(r, iRawDisk->ReadL(dataStart,readLength,aTrg,aMessage,anOffset, aFlag));

    if(r == KErrNone) // Read succeded
        {
        aPos.iPos+=readLength;
        aPos.iCluster=endCluster;
        return;
        }
    if(r != KErrCorrupt) // failure not due to corruption so propogate up
        User::Leave(r);

    TErrorInfoBuf errinf;
    r = iRawDisk->GetLastErrorInfo(errinf);

    if(r == KErrNone && errinf().iReasonCode == TErrorInfo::EBadSector) // GetLastErrorInfo succeded and Last Error was caused by bad sector
        {
        TInt badcluster = (TInt)(((dataStart + errinf().iErrorPos) - ClusterBasePosition())>>ClusterSizeLog2())+KFatFirstSearchCluster;
        FAT().MarkAsBadClusterL(badcluster);
        }

    User::Leave(KErrCorrupt);
    }

//-----------------------------------------------------------------------------------------

void CFatMountCB::ReadFromClusterListL(TEntryPos& aPos,TInt aLength,const TAny* aTrg,const RMessagePtr2& aMessage,TInt anOffset, TUint aFlag) const
//
// Read from cluster list
//
    {

    __PRINT(_L("CFatMountCB::ReadFromClusterListL"));
    __ASSERT_ALWAYS(aPos.Cluster()>=KFatFirstSearchCluster,User::Leave(KErrCorrupt));

    const TInt startPos=aPos.iPos;
    const TInt temp=startPos>>ClusterSizeLog2();

    if ( (startPos) && ((startPos)==(temp<<ClusterSizeLog2())) )
        {
        __ASSERT_ALWAYS(FAT().GetNextClusterL(aPos.iCluster),User::Leave(KErrCorrupt));
        }

    TInt offset=0;
    FOREVER
        {
        DoReadFromClusterListL(aPos,aLength-offset,aTrg,aMessage,anOffset+offset, aFlag);
        offset=aPos.iPos-startPos;
        if ((offset<aLength))
            {
            __ASSERT_ALWAYS(FAT().GetNextClusterL(aPos.iCluster),User::Leave(KErrCorrupt));
            }
        if (offset>=aLength)
            break;
        }
    }

//-----------------------------------------------------------------------------------------

TInt CFatMountCB::FindLeafDirL(const TDesC& aName, TLeafDirData& aLeafDir) const
//
// Navigate the path to find the leaf directory.
// Returns the startcluster of data for the directory found.
//
    {

    __PRINT2(_L("CFatMountCB::FindLeafDirL drv:%d, dir:%S"),DriveNumber() ,&aLeafDir);

    TLex lex(aName);
    TInt r;
    TEntryPos entryPos(RootIndicator(),0);

    ASSERT(iLeafDirCache);

        if (iLeafDirCache->CacheCount() > 0 && aName.Length() > 1)
        	{
        const TInt err = iLeafDirCache->FindInCache(aName, aLeafDir);
        	if (err == KErrNone)
        		{
        	ASSERT(ClusterNumberValid(aLeafDir.iClusterNum)); 
        		return aLeafDir.iClusterNum;
        		}
        	else if (err != KErrNotFound)
        		{
        		User::LeaveIfError(err);
        		}
        	}

    TFatDirEntry entry;
    TFileName fileName;
    TEntryPos startPos;
    TFatDirEntry startEntry;

    for(;;)
        {
        lex.Inc(); // Skip path delimiter
        lex.Mark();
        r=lex.Remainder().Locate(KPathDelimiter);
        
        if (r==KErrNotFound)
            r=lex.Remainder().Length();
        
        if (r==0) // End of the path
            break;
        
        lex.Inc(r); // Set the token length
        
        
        DoFindL(lex.MarkedToken(),
        		KEntryAttMatchMask|KEntryAttMatchExclusive,
        		startPos, startEntry, entryPos, entry,
        		fileName, KErrPathNotFound,
        		NULL,
        		aLeafDir);


        entryPos.iCluster=StartCluster(entry);
        entryPos.iPos=0;
        }// for(;;)

        if (aName.Length() > 1)
        	{
        	aLeafDir = TLeafDirData(entryPos.iCluster);
            iLeafDirCache->AddToCacheL(aName, aLeafDir);
        	}

    return entryPos.iCluster;
    }

//-----------------------------------------------------------------------------------------

/**
    Search for a specified name winthin directory cache
    Works similary to TBool CFatMountCB::DoFindL()

    @param  anAtt           attributes of the object to find
    @param  aStartEntryPos  on return in case of VFAT entry will contain start position of the VFAT dir. entry
    @param  aStartEntry     on return will contain first VFAT dir entry
    @param  aDosEntryPos    the search will start from this position of dir entry, on return it will contain result DOS entry position, last one for VFAT case
    @param  aDosEntry       on return will contain DOS dir entry (the last one for VFAT case)
    @param  aFileName       in the case of VFAT entry and on success here will be returned a long filename
    @param  aAuxParam       some parameters package
    @param  aFileCreationHelper       a helper package for file creations

    @return ETrue if the specified name is found in the cache. In this case aStartEntryPos, aStartEntry, aDosEntryPos, aDosEntry, aFileName will contain valid values
*/
TBool CFatMountCB::DoRummageDirCacheL(const TUint anAtt, TEntryPos& aStartEntryPos,  
										TFatDirEntry& aStartEntry,	TEntryPos& aDosEntryPos,
										TFatDirEntry& aDosEntry,	TDes& aFileName,
										const TFindHelper& aAuxParam,
										XFileCreationHelper* aFileCreationHelper,
										const TLeafDirData& aLeafDir) const
	{
    TBool bCacheMatchFound = EFalse;

    //-- get an interface to the Dir. cache
    MWTCacheInterface* pDirCache = iRawDisk->DirCacheInterface();
    ASSERT(pDirCache);
    if(!pDirCache)
        return EFalse;

    //-- save original values in order to restore them in the case of negative search results
    TEntryPos       StartEntryPos1(aStartEntryPos);
    TEntryPos       DosEntryPos1(aDosEntryPos);
    TFatDirEntry    StartEntry1(aStartEntry);
    TFatDirEntry    DosEntry1(aDosEntry);

    const TUint32 clSize = 1 << ClusterSizeLog2(); //-- media cluster size
    const TUint32 cacheSz = pDirCache->CacheSizeInBytes(); //-- cache size in bytes
    const TUint32 maxDirEntries = cacheSz >> KSizeOfFatDirEntryLog2;  //-- maximal number of dir entries that can be in the cache

    const TUint	  pageSzLog2 = pDirCache->PageSizeInBytesLog2();
    TBool ScanMRUPageFirst 	= EFalse;
	TBool MRUPageScanned 	= EFalse;

	// if MRU pos is availale, start with MRU page
	if (aLeafDir.iMRUPos.Cluster())
    	{
    	ScanMRUPageFirst = ETrue;
    	DosEntryPos1 = aLeafDir.iMRUPos;
    	}

	TInt numFound = 0;
	TEntryPos startPos = DosEntryPos1;
	TUint32 clusterNum = DosEntryPos1.iCluster;

    for(TUint32 entryCnt=0; entryCnt < maxDirEntries; ++entryCnt)
        {//-- walk through directory cluster list. The loop is limited by maximal number of dir entries
         //-- that can be cached. Helps to avoid problems with infinite (looped) directories

        if (IsEndOfClusterCh(DosEntryPos1.iCluster))
        	{
        	// refer back to the last stored cluster position
        	//  note aFileCreationHelper may not be initialised for file opening operations
        	if (aFileCreationHelper && aFileCreationHelper->IsInitialised() && clusterNum != DosEntryPos1.iCluster)
        		{
        	    TEntryPos dummyPos(clusterNum, clSize - KSizeOfFatDirEntry);
        	    aFileCreationHelper->SetEntryAddingPos(dummyPos);
        		aFileCreationHelper->SetIsNewEntryPosFound(ETrue);
        		}

        	if (ScanMRUPageFirst && !MRUPageScanned)
        		{
            	DosEntryPos1 = aDosEntryPos;
            	MRUPageScanned = ETrue;
            	continue;
        		}
            break; //-- this was the last cluster in this directory
        	}

        const TUint32 pageStartPos = CalculatePageOffsetInCluster(DosEntryPos1.iPos, pageSzLog2);
    	DosEntryPos1.iPos = pageStartPos;
        TBool	PassedPageBoundary = EFalse;

        const TInt64  entryLinPos = MakeLinAddrL(DosEntryPos1); //-- linear media position of the cluster for this directory
        const TUint32 cachePageSz = pDirCache->PosCached(entryLinPos); //-- indicates if entryLinPos is cached
        if(cachePageSz)
            {//-- current page is in the directory cache
             //__PRINT2(_L("#-!! CFatMountCB::DoRummageDirCacheL() Searching cl:%d, lin Pos:%X"),DosEntryPos1.iCluster,(TUint32)entryLinPos);

            //-- search to the end of the cached page.
            // Note GetDirEntry() will read data beyond cache page boundary
            const TUint32 nEntries = (1 << pageSzLog2) >> KSizeOfFatDirEntryLog2;

            TInt nErr;
            //-- extract dir entries from the cached page and see if they match given name (aName)
            /// until it reaches the next page
            for(;;)
                {
                StartEntryPos1 = DosEntryPos1;
                TUint32 clSave = DosEntryPos1.iCluster; //-- need to save current cluster number because GetDirEntry() & MoveToNextEntryL() can change it

                //-- get directory entry from the cache. We know that the DosEntryPos1 is cached.
                nErr = GetDirEntry(DosEntryPos1, DosEntry1, StartEntry1, aFileName);
                if(nErr != KErrNone)
                    break;

                if(DosEntry1.IsEndOfDirectory())
                	{
                	if (aFileCreationHelper && aFileCreationHelper->IsInitialised() && !aFileCreationHelper->IsNewEntryPosFound())
		            	{
		            	// note it is impossible to be at the end of the cluster chain here.
	            		aFileCreationHelper->SetEntryAddingPos(DosEntryPos1);
	            		aFileCreationHelper->SetIsNewEntryPosFound(ETrue);
		            	}

                	if (ScanMRUPageFirst && !MRUPageScanned)
                		{
                    	break;
                		}

                	// if (!ScanMRUPageFirst || ScanMRUPageFirst && MRUPageScanned)
                    goto Exit; //-- this was the last entry in this directory, no reason to look further
                	}

                if (aFileCreationHelper && aFileCreationHelper->IsInitialised() && !aFileCreationHelper->IsNewEntryPosFound())
		        	{
		            if (!DosEntry1.IsErased() && !DosEntry1.IsGarbage())
		            	{
		            	numFound = 0;
		            	}
		            else
		            	{
		            	if (numFound == 0)
		            		{
		            		startPos = DosEntryPos1;
		            		}
		            	numFound++;
		            	if (numFound == aFileCreationHelper->NumOfAddingEntries())
		            		{
		            		aFileCreationHelper->SetEntryAddingPos(startPos);
		            		aFileCreationHelper->SetIsNewEntryPosFound(ETrue);
		            		}
		            	}
		        	}
                if(MatchEntryAtt(DosEntry1.Attributes(),anAtt))
                    {//-- FAT or VFAT dir entry is extracted and attributes match. Compare names then.

                    if(StartEntry1.IsVFatEntry())
                        {//-- extracted entry is VFAT one, name can be in UNICODE

                        // we only check short name candidates for long file names with VFAT entries,
                        //  if it is a valid dos name, it will be checked by default
                        // note here target name is always fully specified
                        if (aFileCreationHelper && aFileCreationHelper->IsInitialised())
                        	{
                        	aFileCreationHelper->CheckShortNameCandidates(DosEntry1.Name().Ptr());
                        	}

                        TPtrC ptrAssembledName = RemoveTrailingDots(aFileName);

                        if(ptrAssembledName.MatchF(aAuxParam.iTargetName) != KErrNotFound)
                            {//-- found match in cache
                            bCacheMatchFound = ETrue;
                            goto Exit;
                            }
                        else if(aAuxParam.TrgtNameIsLegalDos())
                            {
                            if(aAuxParam.MatchDosEntryName(DosEntry1.Name().Ptr()))
                                {
                                bCacheMatchFound = ETrue;
                                goto Exit;
                                }
                            }
                        }//if(StartEntry1.IsVFatEntry())
                    else if(aAuxParam.TrgtNameIsLegalDos())
                        {//-- this is an old DOS FAT entry

                          if(aAuxParam.MatchDosEntryName(DosEntry1.Name().Ptr()))
                            {
                            //-- Here is the trick that helps with the situation when VFAT entry is split into 2 halves
                            //-- between 2 clusters (or/and cache pages). I.e. 1st part of this long entry belongs to one cluster and even more might not be cached,
                            //-- While the rest of the entry, DOS part of it is the 1st entry in the cluster and cached.
                            //-- In this case if we search for short file name, we find it, but the aStartEntryPos will be incorrect, which leads to the directory corruption.
                            //-- The simple and quick solution - discard 1st DOS entry and return to old search. It shall be quite rare.
                            if(StartEntryPos1.iPos == 0)
                                {//-- this is the 1st FAT entry in the cluster. Discard it, see comments above.
                                __PRINT(_L("#------ CFatMountCB::DoRummageDirCacheL() discarding FAT Entry!!"));
                                goto Exit;
                                }

                            bCacheMatchFound = ETrue;
                            goto Exit;
                            }
                        }

                    }//if(bGotEntry && MatchEntryAtt(DosEntry1.Attributes(),anAtt))

                // check boundaries after GetDirEntry()
                // if we have cross the cluster boundary, break the for loop
                if(DosEntryPos1.iCluster != clSave)
                    {//-- GetDirEntry() has decided to move to the next cluster.
                    DosEntryPos1.iCluster = clSave;
                    break;
                    }

                // if we are still in the same cluster, check the page boundary by
                /// exam how many entries we have scanned within the cluster
                const TUint entriesLooked =  ((DosEntryPos1.iPos + KSizeOfFatDirEntry)- pageStartPos) >> KSizeOfFatDirEntryLog2;
                if(entriesLooked > nEntries)
                	{
                	PassedPageBoundary = ETrue;
                    break;
                	}


                // move to next entry before scanning next file
                TRAP(nErr,MoveToNextEntryL(DosEntryPos1));
                if(nErr != KErrNone)
                    goto Exit;

                // check boundaries after MoveToNextEntryL()
                if(DosEntryPos1.iCluster != clSave)
                    {
                    DosEntryPos1.iCluster = clSave;
                    break;
                    }

                if (entriesLooked + 1 > nEntries)
                	{
                	PassedPageBoundary = ETrue;
                    break;
                	}

                } //for(;;)

            } //if(iRawDisk->PosCached(...))

        // scanning did not happen because the page is not cached,
        // or
        // scanning finished in last page and file is not found

        // if MRU page is not cached or
        // we scan MRU page first and it is not scanned yet, then this must be the MRU page,
        //  we now start to scan from the beginning
        if (ScanMRUPageFirst && !MRUPageScanned)
        	{
        	MRUPageScanned = ETrue;
        	DosEntryPos1 = aDosEntryPos;
        	DosEntryPos1.iPos = 0;
        	continue;
        	}

        // if we just finished scanning a page and still in the same cluster, then we crossed page
        // 	boundary, continue with next page.
        // note: although we are in the 'next page' already, this page might not be cached, so we need to
        //  check it via pDirCache->PosCached(entryLinPos, nCachedLinPos) and scan it properly.
        if (PassedPageBoundary)
        	{
        	DosEntryPos1.iPos = CalculatePageOffsetInCluster(DosEntryPos1.iPos, pageSzLog2);
        	PassedPageBoundary = EFalse;
        	continue;
        	}

        //-- try to move to the next cluster of the directory file

        if(DosEntryPos1.Cluster() < KFatFirstSearchCluster)  //-- small trick to get rid of TRAPping GetNextClusterL()
            break;

        // record previous cluster number before move on
        clusterNum = DosEntryPos1.iCluster;

        if(! FAT().GetNextClusterL(DosEntryPos1.iCluster))
            break;


    } //for(TUint32 entryCnt=0; entryCnt< maxDirEntries; ++entryCnt)

    //---------------------------------
    Exit:

    if(bCacheMatchFound)
        {
        //-- if the position of the found in cache object is less than given, pretend that we haven't found anything
        //-- Return to the old search, because it can be the case of the end of directory, which is quite difficult to
        //-- detect in this situation. Note that the old part of DoFindL() leaves when the search reaches the end of dir.
        TBool bFallBack=EFalse;

        if(DosEntryPos1.iCluster == aDosEntryPos.iCluster)
            {
            if(DosEntryPos1.iPos < aDosEntryPos.iPos)
                bFallBack = ETrue;
            }
        else
            {
            if(MakeLinAddrL(DosEntryPos1) < MakeLinAddrL(aDosEntryPos))
                bFallBack = ETrue;
            }

        if(bFallBack)
            {
            return EFalse;
            }

        //-- Update parameters with new values
        aStartEntryPos= StartEntryPos1;
        aDosEntryPos  = DosEntryPos1;
        aStartEntry   = StartEntry1;
        aDosEntry     = DosEntry1;

        const TInt64  mruPos = MakeLinAddrL(aDosEntryPos);
        
        pDirCache->MakePageMRU(mruPos);

    	//-- if the corresponding leaf directory name is cached, associate the last search positionin this directory.
        //-- the next search in this dir. will start from this position (and will wrap around over the dir. beginning).
        //-- the "last search position" will is the position of current VFAT entryset start. 
    	if (aLeafDir.iClusterNum)
    		{
            iLeafDirCache->UpdateMRUPos(TLeafDirData(aLeafDir.iClusterNum, aStartEntryPos));
    		}
        }
    return bCacheMatchFound;
    }

//-----------------------------------------------------------------------------------------

/**
    initialise find helper with the target file name.
    This is a quite expensive operation and initialisation is done only once. After this we know if the name is a legal dos one
    and also have the corresponding generated DOS name for it.

    @param aTargetName target file name we are looking for in ::DoFindL()
*/
void CFatMountCB::TFindHelper::InitialiseL(const TDesC& aTargetName)
    {
    if(isInitialised)
        return;

     TInt count = 1;

     iTargetName.Set(aTargetName);
     isLegalDosName = IsLegalDosName(aTargetName, ETrue, EFalse, EFalse, ETrue, EFalse); 

     if(isLegalDosName)
        {//-- iShortName will contain generated short DOS name by long filename
        iShortName = DoGenerateShortNameL(aTargetName, count, ETrue);
        }

     isInitialised = ETrue;
    }

/**
    Perform binary comparison between a given the DOS entry name and the DOS name we generated in TFindHelper::Initialise().
    @param  apDosEntryName pointer to the DOS entry name in XXXXXXXXYYY format
    @return ETrue if the apDosEntryName is the same as generated iShortName
*/
TBool CFatMountCB::TFindHelper::MatchDosEntryName(const TUint8* apDosEntryName) const
    {
    ASSERT(isInitialised);

    if(!isLegalDosName)
        return EFalse;

    return (Mem::Compare(iShortName.Ptr(), KFatDirNameSize, apDosEntryName, KFatDirNameSize) == 0);
    }

//-----------------------------------------------------------------------------------------
const TInt KShortNameCandidatesNum = 4;
/**
Constructor of XFileCreationHelper class
*/
CFatMountCB::XFileCreationHelper::XFileCreationHelper()
	{
    isInitialised = EFalse;
	}

/**
Destructor of XFileCreationHelper class
*/
CFatMountCB::XFileCreationHelper::~XFileCreationHelper()
	{
	Close();
	}

/**
Initialises a TFileCreationHelper object, generate a short name candidate pool.

@param	aTargetName	Target file name for the potential new file.
@post	TFileCreationHelper is fully initialised.
*/
void CFatMountCB::XFileCreationHelper::InitialiseL(const TDesC& aTargetName)
	{
    // close before use, to avoid memory leak
	Close();

    iTargetName.Set(aTargetName);
	// generates short name candidate(s)
    TInt count = 1;
    while (count <= KShortNameCandidatesNum)
		{
		TShortName shortNameCandidate = DoGenerateShortNameL(aTargetName, count, ETrue);
		TInt err = iShortNameCandidates.Append(shortNameCandidate);
		User::LeaveIfError(err);

		if (count == -1)	// No tilde and number is needed
			{
			break;
			}
		else
			count++;
		}

    // calculate number of new entries needed
    iNumOfAddingEntries = 1;
    isTrgNameLegalDosName = IsLegalDosName(aTargetName, EFalse, EFalse, EFalse, EFalse, ETrue);
    if (!isTrgNameLegalDosName)
    	iNumOfAddingEntries = (TUint16) NumberOfVFatEntries(iTargetName.Length());

    isNewEntryPosFound = EFalse;
    isInitialised = ETrue;
    }

/**
Close function of XFileCreationHelper class
*/
void CFatMountCB::XFileCreationHelper::Close()
	{
	iShortNameCandidates.Close();
	isInitialised = EFalse;
	}

/**
Validates short name candidates. If the input dos entry name is found in the short name
 candidate pool, the corresponding short name candidate will be removed from the pool.

@param	apDosEntryName	An existing short name, to compare with the candidates.
@pre 	Object should be initialised
*/
void CFatMountCB::XFileCreationHelper::CheckShortNameCandidates(const TUint8* apDosEntryName)
    {
    ASSERT(isInitialised);
    if (!isInitialised)
    	return;

    if (iShortNameCandidates.Count() > 0)
    	{
    	for (TInt i = 0; i < iShortNameCandidates.Count(); i++)
    		{
    		if (Mem::Compare(iShortNameCandidates[i].Ptr(), KFatDirNameSize, apDosEntryName, KFatDirNameSize) == 0)
    			{
    			iShortNameCandidates.Remove(i);
    			break;
    			}
    		}
    	}
    }

/**
Gets a validated short name from the short name candidate pool.

@param	aShortName	On return, contains a validated short name if found, otherwise zeroed.
@return	TInt		Returns KErrNone if a validated short name found successfully,
 					 else KErrNotFound is returned.
 					Returns KErrNotReady if object is not initialised.
@pre 	Object should be initialised
*/
TInt CFatMountCB::XFileCreationHelper::GetValidatedShortName(TShortName& aShortName) const
	{
	aShortName.Zero();

	ASSERT(isInitialised);
	if (!isInitialised)
		return KErrNotReady;

	if (iShortNameCandidates.Count() > 0)
		{
		aShortName.Copy(iShortNameCandidates[0]);
		return KErrNone;
		}

	return KErrNotFound;
	}

//-----------------------------------------------------------------------------------------


/**
    Scan a directory looking for aName.

    @param  aTrgtName       a name of an object we are looking up in directory
    @param  anAtt           attributes of this object
    @param  aStartEntryPos  on return in case of VFAT entry will contain start position of the VFAT dir. entry
    @param  aStartEntry     on return will contain first VFAT dir entry
    @param  aDosEntryPos    the search will start from this position of dir entry, on return it will contain result DOS entry position, last one for VFAT case
    @param  aDosEntry       on return will contain DOS dir entry (the last one for VFAT case)
    @param  aFileName       in the case of VFAT entry and on success here will be returned a long filename
    @param  anError         This function might leave with this given error code
	@param  aFileCreationHelper       a helper package for file creations

    @return ETrue if extracted entry is VFAT one, EFalse, if it's old DOS-style one
    @leave  can leave with anError code on error or if the search has reached the end of directory (!)
*/
TBool CFatMountCB::DoFindL(const TDesC& aTrgtName,TUint anAtt,
						TEntryPos& aStartEntryPos,TFatDirEntry& aStartEntry,
						TEntryPos& aDosEntryPos,TFatDirEntry& aDosEntry,
						TDes& aFileName,TInt anError,
						XFileCreationHelper* aFileCreationHelper,
						const TLeafDirData& aLeafDirData) const
	{
    // check that the entry position to be read next is not past the end of the
    // root directory. If this is the case then when GetDirEntryL(..) is called
    // this will lead to MakeLinAddr(..) leaving with KErrDirFull.

    if (IsRootDir(aDosEntryPos)&&(aDosEntryPos.iPos+StartOfRootDirInBytes()>=RootDirEnd()))
        User::Leave(anError);//Allows maximum number of entries in root directory

    __PRINT2(_L("CFatMountCB::DoFindL() drv:%d, %S"),Drive().DriveNumber(),&aTrgtName);

    TUint32 previousCluster=aDosEntryPos.iCluster;
    TUint previousPosition=aDosEntryPos.iPos;
    TUint32 changePreviousCluster=1;
    TUint32 count=0;

    TBool trgNameIsWildCard     = EFalse; //-- ETrue if the name we are looking for is a wildcard
    TBool trgNameFullySpecified = ETrue;  //-- ETrue if the name we are looking for doesn't contain wildcards


    {
    //-- find out if the name we are looking for is a wildcard ("*" or "*.*")
    const TInt len = aTrgtName.Length();

    if(len == 1)
        trgNameIsWildCard = (aTrgtName[0] == '*');
    else if(len == 3)
        {
        _LIT(KAllFiles, "*.*");
        trgNameIsWildCard = (aTrgtName==KAllFiles);
        }

    //-- find out if the name we are looking for contains wildcharacters: "*" or "?"
    if(trgNameIsWildCard)
        trgNameFullySpecified = EFalse;
    else
        {
        for(TInt i=0; i<len; ++i)
            {
            const TChar ch = aTrgtName[i];
            if(ch == (TChar)'*' || ch == (TChar)'?')
                {
                trgNameFullySpecified = EFalse;
                break;
                }
            }
        }
    }


    TPtrC trgtNameNoDot(aTrgtName);

    TFindHelper findHelper;
    //---------------------------------------------------
    //-- if we have fully specified name and directory cache is present, try to
    //-- locate the name in the cache first to avoid reading from media
    //-- if the entry belongs to the root directory (for FAT12,16) skip the lookup, because root directory isn't aligned by cluster size boundary,
    //-- while directory cache pages are. For FAT32 it doesn't matter, because root dir is a usual file.
    
    //-- the "rummage dir. cache" can be swithed off. This is not affecting the functionality, only the performance.
 #ifdef USE_DIR_CACHE_RUMMAGE
                                                                 
    if(iRawDisk->DirCacheInterface() && trgNameFullySpecified && !IsRootDir(aDosEntryPos) && !aFileCreationHelper)
        {//-- aName is fully specified, i.e doesn't contain wildcards

        findHelper.InitialiseL(trgtNameNoDot);

        const TBool bMatchFound = DoRummageDirCacheL(anAtt, aStartEntryPos, aStartEntry, aDosEntryPos, aDosEntry, aFileName, findHelper, aFileCreationHelper, aLeafDirData);
        if(bMatchFound)
            {
            return(aStartEntry.IsVFatEntry());
            }
        }
 #endif

    //---------------------------------------------------

    // we need to scan ahead from the mru pos then come back to beginning, if startcluster is provided
    TBool scanAhead = EFalse;
    // if we have a starting cluster number (and it's not root directory in FAT16/12 case)&&
    //  we found a lastScanned entry's cluster (and it's not root directory in FAT16/12 case)&&
    // if we don't have a starting cluster number, we draw back to original scanning algorithm
    if (!IsRootDir(aDosEntryPos) 			// we don't do forward scanning for root dir &
    		&& aLeafDirData.iClusterNum != 0 	// if we have a starting cluster number &
    		&& aLeafDirData.iMRUPos.Cluster() != 0) 	// if we have a starting cluster number &
    	{
    	scanAhead = ETrue;
    	aDosEntryPos = aLeafDirData.iMRUPos;
    	}

    TInt numFound = 0;
    TEntryPos startPos = aDosEntryPos;
    TInt clustNum = aDosEntryPos.Cluster();

    for (TInt scanCnt = 1; scanCnt <= 2; ++scanCnt)
    	{
    	// if we are not scanning ahead, we don't need this outer for loop
    	if (!scanAhead)
    		scanCnt++;

    	TBool found = EFalse;

        FOREVER //FOREVER2 -- walk through all directory entries in the current directory until find a match or directory end
            {
	        //-- read full directory entry starting from aDosEntryPos. On return aFileName may contain assembled long filename (if the entry is VFAT)
	        //-- aDosEntry will contain a DOS entry of the directory entry we have read.
	        aStartEntryPos=aDosEntryPos;
	        User::LeaveIfError(GetDirEntry(aDosEntryPos, aDosEntry, aStartEntry, aFileName));

	        if (aDosEntry.IsEndOfDirectory())
	            {//-- the end of directory reached.

	            // if new entry position for adding has not been found yet.
	            // note aFileCreationHelper may not be initialised for pure file opening operations
	            if (aFileCreationHelper && aFileCreationHelper->IsInitialised() && !aFileCreationHelper->IsNewEntryPosFound())
	            	{
	            	// if MoveToNextEntryL have gone to the next cluster which is the end of cluster chain,
	            	//  we pass the last scanned entry position to AddDirEntryL
	            	if (IsEndOfClusterCh(aDosEntryPos.iCluster))
	            		{
	            	    TInt clusterSize=1<<ClusterSizeLog2();
	            	    TEntryPos dummyPos(clustNum, clusterSize - KSizeOfFatDirEntry);
	            	    aFileCreationHelper->SetEntryAddingPos(dummyPos);
	            		aFileCreationHelper->SetIsNewEntryPosFound(ETrue);
	            		}
	            	// or we reached the end of the directory.
	            	else
	            		{
	            		aFileCreationHelper->SetEntryAddingPos(aDosEntryPos);
	            		aFileCreationHelper->SetIsNewEntryPosFound(ETrue);
	            		}
	            	}

	            // if we are scanning ahead and this is the first scanning, we break out to restart scanning
	            if (scanAhead && scanCnt == 1)
	            	{
	            	break; // from FOREVER, restart scanning
	            	}

	            // if (!scanAhead || scanAhead && scanCnt == 2)
	            User::Leave(anError);
	            }


	        // entry space searching for potential new file/directory creation
	        if (aFileCreationHelper && aFileCreationHelper->IsInitialised() && !aFileCreationHelper->IsNewEntryPosFound())
	        	{
	            if (!aDosEntry.IsErased() && !aDosEntry.IsGarbage())
	            	{
	            	numFound = 0;
	            	}
	            else
	            	{
	            	if (numFound == 0)
	            		{
	            		startPos = aDosEntryPos;
	            		}
	            	numFound++;
	            	if (numFound == aFileCreationHelper->NumOfAddingEntries())
	            		{
	            		aFileCreationHelper->SetEntryAddingPos(startPos);
	            		aFileCreationHelper->SetIsNewEntryPosFound(ETrue);
	            		}
	            	}
	        	}


	        if (IsRootDir(aDosEntryPos)&&(aDosEntryPos.iPos+StartOfRootDirInBytes()==(RootDirEnd()-KSizeOfFatDirEntry)))
	            if (aDosEntry.IsErased())
	                {
	                User::Leave(anError);//Allows maximum number of entries in root directory
	                }


	        const TBool bFileNameEntry = !aDosEntry.IsCurrentDirectory() && !aDosEntry.IsParentDirectory() && !aDosEntry.IsErased() && !aDosEntry.IsGarbage();

	        if (bFileNameEntry && MatchEntryAtt(aDosEntry.Attributes(), anAtt))
	            {//-- we have read a filename entry and entry's attributes match required; compare names then.

	            if (trgNameIsWildCard)
	            	{
	            	found = ETrue;
	            	break; //-- we were looking for '*' or '*.*', so will be satisfied with any current file name.
	            	}


	            if (aStartEntry.IsVFatEntry())
	                {//-- we've read a VFAT entry, aFileName is supposed to contain long filename, aDosEntry - DOS entry for this name.
	                 //-- note: aFileName.Length() may be 0, while DOS entry (short name is OK) in the case of orphaned VFAT entries


	                // we only check short name candidates for long file names with VFAT entries,
	                //  if it is a valid dos name, it will be checked by default
	                // note, for file creation cases, target name will be always fully specified
	                if (aFileCreationHelper && aFileCreationHelper->IsInitialised() && trgNameFullySpecified)
		            	 {
		            	 aFileCreationHelper->CheckShortNameCandidates(aDosEntry.Name().Ptr());
		            	 }

	                //-- discard trailing dots from aFileName if present
	                 TPtrC ptrAssembledName = RemoveTrailingDots(aFileName);

	                 if(ptrAssembledName.MatchF(trgtNameNoDot) != KErrNotFound)
	                	 {
						 found = ETrue;
	                	 break; //-- OK, found a match.
	                	 }
	                 else if (trgNameFullySpecified)
	                	 {
	                	 //-- long name assembled by GetDirEntry() doesn't match the target. But if he target name is fully specified,
	                	 //-- we need to compare corresponding DOS entries, because VFAT entries may be damaged, while DOS ones are OK.
	                     findHelper.InitialiseL(trgtNameNoDot);

	                     if(findHelper.MatchDosEntryName(aDosEntry.Name().Ptr()))
	                    	 {
							 found = ETrue;
	                    	 break; //-- DOS entries match, success.
	                    	 }

	                	 }
	                 else if (!trgNameFullySpecified)
	                	 {//-- target name contains wildcards, we need to use MatchF with dos name
                         TBuf8<0x20> dosName8(DosNameFromStdFormat(aDosEntry.Name()));
	                     TBuf<0x20>  dosName;
	                     LocaleUtils::ConvertToUnicodeL(dosName, dosName8); //-- convert DOS name to unicode (implies locale settings)
	                     if (dosName.MatchF(trgtNameNoDot)!=KErrNotFound)
	                    	 {
							 found = ETrue;
							 break;
	                    	 }
                         }


	                }
	            else //if (aStartEntry.IsVFatEntry())
	                {//-- we've read a legacy FAT entry, so compare DOS entries
	                findHelper.InitialiseL(trgtNameNoDot);

	                if(findHelper.TrgtNameIsLegalDos())
	                    {//-- we are looking for a legal DOS name
	                    if(trgNameFullySpecified)
	                        {//-- if the target name is fully specified, we can yse binary comparison of the DOS entries
	                        if(findHelper.MatchDosEntryName(aDosEntry.Name().Ptr()))
	                        	{
								found = ETrue;
	                            break;
	                        	}
	                        }
	                    else
	                        {//-- target name contains wildcards, we neeed to use MatchF
	                        TBuf8<0x20> dosName8(DosNameFromStdFormat(aDosEntry.Name()));
	                        TBuf<0x20>  dosName;
	                        LocaleUtils::ConvertToUnicodeL(dosName, dosName8); //-- convert DOS name to unicode (implies locale settings)
	                        if (dosName.MatchF(trgtNameNoDot)!=KErrNotFound)
	                        	{
								found = ETrue;
	                            break;
	                        	}

	                        }
	                    } //if(findHelper.TrgtNameIsLegalDos())

	                } //else if (aStartEntry.IsVFatEntry())

	            } //if (bFileNameEntry && MatchEntryAtt(aDosEntry.Attributes(),anAtt))


	        // record previous cluster number
	        clustNum = aDosEntryPos.iCluster;

	        // this is the 2nd scanning and we have just passed the pos we started.
	        if (scanAhead && scanCnt == 2)
	        	{
	        	if (aDosEntryPos.Cluster() == aLeafDirData.iMRUPos.Cluster()
	        			&& aDosEntryPos.Pos() >= aLeafDirData.iMRUPos.Pos())
	        		{
	        		User::Leave(anError);
	        		}
	        	}


	        MoveToNextEntryL(aDosEntryPos); //-- goto the next entry in the directory

	        if (IsRootDir(aDosEntryPos)&&(aDosEntryPos.iPos+StartOfRootDirInBytes()>=RootDirEnd()))
	            {
	            User::Leave(anError);//Allows maximum number of entries in root directory
	            }


	        if (!scanAhead || scanCnt == 2)
	        	{
		        if (aDosEntryPos.iCluster && (aDosEntryPos.iPos <= previousPosition))
		            DoCheckFatForLoopsL(aDosEntryPos.iCluster,previousCluster,changePreviousCluster,count);

		        previousPosition=aDosEntryPos.iPos;
	        	}
	    	}	// FOREVER -- the actual scanning is done inside this loop


        if (found)
        	{
        	break;
        	}

        // if not found:
    	// if we have not found in the first scanning and we are doing scanning ahead,
        //  we need to go back to the starting pos of this dir and scan from start until
        //  we reach lastscannedPos
        if (scanAhead && scanCnt == 1)
        	{
        	aDosEntryPos = TEntryPos(aLeafDirData.iClusterNum, 0);
        	continue;
        	}
        else
        	{
        	// there are only two exits: either found or reached end of dir in the 1st scanning
        	ASSERT(0);
        	break;
        	}
    	} // for (TInt scanCnt = 1; scanCnt <= 2; ++scanCnt)

    //---------------------------------------------------
    if (iRawDisk->DirCacheInterface() && aDosEntryPos.Cluster())
    	{
    	TInt64 mruPos = MakeLinAddrL(aDosEntryPos);
        iRawDisk->DirCacheInterface()->MakePageMRU(mruPos);

    	//-- if the corresponding leaf directory name is cached, associate the last search positionin this directory.
        //-- the next search in this dir. will start from this position (and will wrap around over the dir. beginning).
        //-- the "last search position" will is the position of current VFAT entryset start. 
    	if(aLeafDirData.iClusterNum)
    		{
            iLeafDirCache->UpdateMRUPos(TLeafDirData(aLeafDirData.iClusterNum, aStartEntryPos));
            }
    	}

    return (aStartEntry.IsVFatEntry());
    }

//-----------------------------------------------------------------------------------------
/**
    Locate an directory entry entry from its full path name.

    @param  aName           a name of an object we are looking for
    @param  anAtt           attributes of this object
    @param  anEntry         on return will contain first VFAT dir entry
    @param  anEntryPos      on return in case of VFAT entry will contain start position of the VFAT dir. entry

    @leave  can leave with KErrNotFound if the search has reached the end of directory
*/
void CFatMountCB::FindEntryStartL(const TDesC& aName,TUint anAtt,TFatDirEntry& anEntry,TEntryPos& anEntryPos) const
    {
    __PRINT(_L("CFatMountCB::FindEntryStartL()"));
    TInt namePos=aName.LocateReverse(KPathDelimiter)+1; // There is always a path delimiter
    TFileName fileName;
    TLeafDirData leafDir;
    TEntryPos dosEntryPos(FindLeafDirL(aName.Left(namePos),leafDir),0);
    TFatDirEntry dosEntry;

    DoFindL(aName.Mid(namePos),anAtt,anEntryPos,anEntry,dosEntryPos,dosEntry,fileName,KErrNotFound,NULL,leafDir);
    }


//-----------------------------------------------------------------------------------------

/**
    Locate an directory entry entry from its full path name.

    @param  aName           a name of an object we are looking for
    @param  anAtt           attributes of this object
    @param  anEntry         on return will contain first VFAT dir entry
    @param  anEntryPos      on return in case of VFAT entry will contain start position of the VFAT dir. entry

    @leave  can leave with KErrNotFound if the search has reached the end of directory
*/
void CFatMountCB::FindEntryStartL(const TDesC& aName,TUint anAtt,TFatDirEntry& anEntry,TEntryPos& anEntryPos,XFileCreationHelper* aFileCreationHelper) const
    {
    __PRINT(_L("CFatMountCB::FindEntryStartL()"));
    TInt namePos=aName.LocateReverse(KPathDelimiter)+1; // There is always a path delimiter
    TFileName fileName;
    TLeafDirData leafDir;
    TEntryPos dosEntryPos(FindLeafDirL(aName.Left(namePos),leafDir),0);
    TFatDirEntry dosEntry;
    DoFindL(aName.Mid(namePos),anAtt,anEntryPos,anEntry,dosEntryPos,dosEntry,fileName,KErrNotFound,aFileCreationHelper,leafDir);
    }

//-----------------------------------------------------------------------------------------
void CFatMountCB::FindDosNameL(const TDesC& aName,TUint anAtt,TEntryPos& aDosEntryPos,TFatDirEntry& aDosEntry,TDes& aFileName,TInt anError) const
//
// Scan a directory looking for aName.
// aCluster and anEntryAddr give the location of the entry.
//
    {

    __PRINT(_L("CFatMountCB::FindDosNameL()"));
    TEntryPos startPos;
    TFatDirEntry startEntry;

    TLeafDirData leafDir;			// leaf dir data is zero initialized, no scannig ahead
    DoFindL(aName,anAtt,startPos,startEntry,aDosEntryPos,aDosEntry,aFileName,anError,NULL,leafDir);
    }
//-----------------------------------------------------------------------------------------

void CFatMountCB::AddDirEntryL(TEntryPos& aPos,TInt aNumOfEntries)
//
// Find space for a new directory entry. Leave KErrEof if no space
//
    {

    __PRINT(_L("CFatMountCB::AddDirEntryL"));
    TInt numFound=0;
    TFatDirEntry entry;
    TEntryPos startPos(RootIndicator(),0);
    TInt clusterNum=aPos.iCluster;
    FOREVER
        {
        ReadDirEntryL(aPos,entry);
        if (entry.IsEndOfDirectory())
            break;
        if (!entry.IsErased() && !entry.IsGarbage())
            numFound=0;
        else
            {
            if (numFound==0)
                startPos=aPos;
            numFound++;
            if (numFound==aNumOfEntries)
                {
                aPos=startPos;
                return;
                }
            }
        clusterNum=aPos.iCluster;
        MoveToNextEntryL(aPos);
        if (IsRootDir(aPos)&&(StartOfRootDirInBytes()+aPos.iPos==RootDirEnd()))
    //  No end of directory marker at end of root directory
            User::Leave(KErrDirFull);
        }

    TUint clusterSize=1<<ClusterSizeLog2();
    if (IsEndOfClusterCh(aPos.iCluster))
        { // End of last cluster in directory
        aPos.iCluster=clusterNum;
        aPos.iPos=clusterSize;
        }

    TEntryPos eofPos(aPos.iCluster,aPos.iPos+KSizeOfFatDirEntry*aNumOfEntries);

    if (IsRootDir(aPos))
        { // Special case of root directory
        if (eofPos.iPos+StartOfRootDirInBytes()>RootDirEnd())
            User::Leave(KErrDirFull);
        else
            return;
        }

    if (eofPos.iPos==clusterSize)
        return; // No need to allocate
    if (eofPos.iPos>clusterSize)
        {
        TInt numNeeded=eofPos.iPos>>ClusterSizeLog2();
        if(IsRuggedFSys())
            {
            ExtendClusterListZeroedL(numNeeded,eofPos.iCluster);
            }
        else
            {
            FAT().ExtendClusterListL(numNeeded,eofPos.iCluster);
            ZeroDirClusterL(eofPos.iCluster);
            }

        eofPos.iPos-=numNeeded<<ClusterSizeLog2();
        if(aPos.iPos==clusterSize)
            {
            if (!FAT().GetNextClusterL(aPos.iCluster))
                {
                __PRINT(_L("CFatMountCB::AddDirEntryL corrupt#1"))
                User::Leave(KErrCorrupt);
                }
            aPos.iPos=0;
            }
        }
    else if(Drive().IsRemovable())
        {
        // check if entry is already zeroed
        ReadDirEntryL(eofPos,entry);
        if(!entry.IsEndOfDirectory())
            {
            // some removable media may not have directory zeroed
            entry.SetEndOfDirectory();
            WriteDirEntryL(eofPos,entry);
            }
        }
    }

/**
    Zero fill a cluster
    @param  aCluster cluster number to zero-fill
*/
void CFatMountCB::ZeroDirClusterL(TUint32 aCluster)
    {

    __PRINT1(_L("CFatMountCB::ZeroDirClusterL %d"),aCluster);

    const TUint32 KClusterSz= 1<<ClusterSizeLog2();
    const TUint32 KMaxBufSz = KClusterSz;           //-- max. nuffer size is a cluster
    const TUint32 KMinBufSz = 1<<SectorSizeLog2();  //-- min. buffer size is 1 sector (for OOM case)

    //-- allocate a buffer for zero-filling a cluster
    RBuf8 buf;
    CleanupClosePushL(buf);

    if(buf.CreateMax(KMaxBufSz) != KErrNone)
        buf.CreateMaxL(KMinBufSz); //-- OOM, try to create smaller buffer

    buf.FillZ();

    TEntryPos entryPos(aCluster,0);

    //-- write buffer to the beginning of the directory file.
    DirWriteL(entryPos, buf); //-- use special interface to access FAT directory file

    //-- fill in the rest of the cluster if we used a small buffer
    if((TUint32)buf.Size() < KClusterSz) //--  KMaxBufSz may == KMinBufSz if we have 1 sector per cluster
    {
        const TInt restCnt = SectorsPerCluster() - 1;
        ASSERT(restCnt >=1);

        for(TInt i=0; i<restCnt; ++i)
        {
            entryPos.iPos += KMinBufSz;
            DirWriteL(entryPos, buf); //-- use special interface to access FAT directory file
        }

    }

    CleanupStack::PopAndDestroy(&buf);
    }


/**
    Internal method. Retrieves directory entry from given position.

    @param  aPos            on enter shall contain start position, from where the entry will be read. On return contains position of the DOS entry (the last one for object name for the VFAT case)
    @param  aDosEntry       On return contains DOS entry for the VFAT case
    @param  aStartEntry     On return contains start entry of the directory object for the VFAT case
    @param  aLongFileName   On return contains VFAT or long filename

    @return  ETrue  if whole FAT entry is OK: only 1 entry for DOS name or _ALL_ entries for a long name
             EFalse if there was an error in assembling entries to the long file name. In this case this entry shall be ignored by upper level.

    can leave because of ReadDirEntryL() and MoveToNextEntryL() [end of dir].
*/
TBool CFatMountCB::DoGetDirEntryL(TEntryPos& aPos, TFatDirEntry& aDosEntry, TFatDirEntry& aStartEntry, TDes& aLongFileName) const
    {

//    __PRINT3(_L("CFatMountCB::GetDirEntryL() drv:%d, pos:%d:%d"), Drive().DriveNumber(), aPos.iCluster, aPos.iPos);

    ReadDirEntryL(aPos,aStartEntry);
    aDosEntry=aStartEntry;
    if (!aDosEntry.IsVFatEntry() || aDosEntry.IsErased() || aDosEntry.IsGarbage())
        {//-- This is either a 8.3 FAT entry or garbage
        aLongFileName.SetLength(0);
        return ETrue;
        }

    //-- process VFAT entries

    if(!aDosEntry.IsLongNameStart())
        return EFalse; //-- wrong counter in the 1st VFat entry, consider it as orphaned


    TInt count = aDosEntry.NumFollowing(); //-- count of the following VFat entries

    TBuf16<KMaxVFatEntryName> vBuf(KMaxVFatEntryName);
    aDosEntry.ReadVFatEntry(vBuf);

    TInt vLength=vBuf.Locate('\0');
    if (vLength==KErrNotFound)
        vLength=KMaxVFatEntryName;

    vBuf.SetLength(vLength);

    const TInt nameLen = vLength+KMaxVFatEntryName*(count-1);
    if(nameLen <= 0 || nameLen > KMaxFileName)
        return EFalse; //--  wrong long file name length, consider VFAT entry as orphaned

    aLongFileName.SetLength(nameLen);

    const TUint8 entryCheckSum = aDosEntry.CheckSum(); //-- check sum from the 1st VFat entry

    TUint nameChunkOffset = KMaxVFatEntryName*(count-1);

    while (count--)
        {
        TPtr fileNamePtr(&aLongFileName[0]+nameChunkOffset, aLongFileName.Length()-nameChunkOffset);
        fileNamePtr.Copy(vBuf);
        if (count==0)
            break; //-- all VFat entries read, only DOS entry remained
        
        ASSERT(nameChunkOffset >= (TUint)KMaxVFatEntryName);
        nameChunkOffset-=KMaxVFatEntryName;

        MoveToNextEntryL(aPos);
        ReadDirEntryL(aPos,aDosEntry);

        //-- check if it is correct VFat entry.
        //-- if not, this is the "orphaned" entry and will be ignored
        if(!aDosEntry.IsVFatEntry() || aDosEntry.IsErased() || entryCheckSum != aDosEntry.CheckSum() || aDosEntry.NumFollowing() != count)
            return EFalse;  //-- bad VFAT entry

        aDosEntry.ReadVFatEntry(vBuf);
        }

    if (IsRootDir(aPos)&&(aPos.iPos+StartOfRootDirInBytes()==(RootDirEnd()-KSizeOfFatDirEntry)))
        return ETrue;//Allows maximum number of entries in root directory

    //-- read the last, DOS FAT entry
    MoveToNextEntryL(aPos);
    ReadDirEntryL(aPos,aDosEntry);

    //-- check if it is corect
    if(aDosEntry.IsEndOfDirectory() || aDosEntry.IsErased() || aDosEntry.IsVFatEntry())
        return EFalse; //-- Bad DOS entry

    //-- verify ChechSum here if it is incorrect, use DOS name only
    const TUint8 calcNameChkSum = CalculateShortNameCheckSum(aDosEntry.Name());
    if(calcNameChkSum != entryCheckSum)
        {
        aLongFileName.SetLength(0);//-- don't use long filename
        __PRINT2(_L("CFatMountCB::GetDirEntryL() CheckSum mismatch: VFat:0x%x, DOS:0x%d"),entryCheckSum, calcNameChkSum);
        }

    return ETrue;
    }


/**
    Read a number of VFAT entries from the directory file.
    for parameters see DoGetDirEntryL()

    @return KErrNone if everything is OK, system wide error code otherwise

*/
TInt CFatMountCB::GetDirEntry(TEntryPos& aPos,TFatDirEntry& aDosEntry,TFatDirEntry& aStartEntry,TDes& aLongFileName) const
    {

    TBool bEntryOK=ETrue;
    TRAPD(nErr, bEntryOK = DoGetDirEntryL(aPos, aDosEntry, aStartEntry, aLongFileName));

    if(nErr !=KErrNone)
        return nErr;

    if(!bEntryOK)
        {//-- DoGetDirEntryL could not assemble whole VFat entry, probably some parts of it are damaged.
         //-- consider it as an "orphaned" entry and skip
        aDosEntry.iData[0] = 0xFF;      // Mark entry as garbage
        aLongFileName.SetLength(0);     // No long filename
        }

    return KErrNone;
    }

void CFatMountCB::MoveToNextEntryL(TEntryPos& aPos) const
//
// If anEntry is at the end of the cluster, and we are not the root dir,
// move it to the next in the list.
//
    {

//  __PRINT(_L("CFatMountCB::MoveToNextEntryL"));
    if (IsEndOfClusterCh(aPos.iCluster))
        return;
    const TUint temp = 1<<ClusterSizeLog2();
    if (aPos.iPos+KSizeOfFatDirEntry!=temp || IsRootDir(aPos))
        {
        aPos.iPos+=KSizeOfFatDirEntry;
        }
    else
        {
        if (FAT().GetNextClusterL(aPos.iCluster)==EFalse)
            {
            SetEndOfClusterCh(aPos.iCluster);
            }
        aPos.iPos=0;
        }
    }

//-----------------------------------------------------------------------------------------

/**
    Starting from a VFat entry walk down the directory until the associated dos entry is found

    @param aPos     in: VFAT entry position. out: if this is a VFAT entry set, it will be DOS entry position. otherwise not changed
    @param anEntry  on return will contain DOS dir. entry contents (if aPos points to the VFAT entry)
*/
void CFatMountCB::MoveToDosEntryL(TEntryPos& aPos,TFatDirEntry& anEntry) const
    {

    //__PRINT(_L("CFatMountCB::MoveToDosEntryL"));
    if (anEntry.IsVFatEntry()==EFalse)
        return;
    FOREVER
        {
        MoveToNextEntryL(aPos);
        ReadDirEntryL(aPos,anEntry);
        if (anEntry.IsVFatEntry()==EFalse)
            break;
        if (IsRootDir(aPos)&&(aPos.iPos+StartOfRootDirInBytes()==(RootDirEnd()-KSizeOfFatDirEntry)))
            break;  //  Allows maximum number of entries in root directory
        }
    }

//-----------------------------------------------------------------------------------------

/** Read the Uid of the entry starting at aCluster */
void CFatMountCB::ReadUidL(TUint32 aCluster,TEntry& anEntry) const
    {

    __PRINT1(_L("CFatMountCB::ReadUidL(%d)"), aCluster);

    if(aCluster < KFatFirstSearchCluster || aCluster >= UsableClusters()+KFatFirstSearchCluster)
        User::Leave(KErrCorrupt);

    TBuf8<sizeof(TCheckedUid)> uidBuf;
    iRawDisk->ReadCachedL(FAT().DataPositionInBytesL(aCluster),sizeof(TCheckedUid),uidBuf);
    __ASSERT_DEBUG(uidBuf.Length()==sizeof(TCheckedUid),Fault(EFatReadUidFailed));
    TCheckedUid uid(uidBuf);
    anEntry.iType=uid.UidType();
    }

//-----------------------------------------------------------------------------------------

/**
    Read file section without opening this file on a file server side.

    @param  aName       file name; all trailing dots from the name will be removed
    @param  aFilePos    start read position within a file
    @param  aLength     how many bytes to read; on return will be how many bytes actually read
    @param  aDes        local buffer desctriptor
    @param  aMessage    from file server, used to write data to the buffer in different address space.

    @leave on media read error
*/
void CFatMountCB::ReadSectionL(const TDesC& aName,TInt aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage)
    {
    __PRINT4(_L("CFatMountCB::ReadSectionL, drv:%d, pos:%d, len:%d, FN:%S"), DriveNumber(), aPos, aLength, &aName);

    CheckStateConsistentL();

    TEntryPos dosEntryPos(RootIndicator(),0);
    TFatDirEntry dosEntry;
    TFileName fileName;


    TInt namePos=RemoveTrailingDots(aName).LocateReverse(KPathDelimiter)+1; // There is always a path delimiter
    TLeafDirData leafDir;
    dosEntryPos.iCluster=FindLeafDirL(RemoveTrailingDots(aName).Left(namePos), leafDir);
    dosEntryPos.iPos=0;
    TEntryPos startPos;
    TFatDirEntry startEntry;
    DoFindL(RemoveTrailingDots(aName).Mid(namePos),KEntryAttMaskSupported,
    		startPos,startEntry,dosEntryPos,dosEntry,
    		fileName,KErrNotFound,
    		NULL,
    		leafDir);

//  Check that reading from aPos for aLength lies within the file
//  if aPos is within the file, and aLength is too long, read up to EOF
//  If aPos is beyond the end of the file, return a zero length descriptor

	TUint32 fileSize = dosEntry.Size();
	if ((TUint)aPos>=fileSize)
        User::Leave(KErrEof);

    if ((TUint)(aPos+aLength)>fileSize)
        aLength=fileSize-aPos;

    TUint32 cluster=StartCluster(dosEntry);
	TInt pos = aPos;

    TUint32 endCluster;
    TInt clusterSize=1<<ClusterSizeLog2();      //  Size of file clusters
	TInt readTotal = 0;

	// Total number of clusters in file
    TInt maxClusters=((fileSize+clusterSize-1)>>ClusterSizeLog2());

	// Read data
    FOREVER
        {
		//  Get the maximum number of clusters that can be read contiguously
        TInt clusterListLen=FAT().CountContiguousClustersL(cluster,endCluster,maxClusters);
        __ASSERT_DEBUG(clusterListLen>0,Fault(EReadFileSectionFailed));

		//  If start position within this block, then read some data
        if (pos<(clusterListLen<<ClusterSizeLog2()))
            {
			//  Read the remaining length or the entire cluster block whichever is smaller
			TInt readLength = Min(aLength-readTotal,(clusterListLen<<ClusterSizeLog2())-pos);
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


//-----------------------------------------------------------------------------------------

void CFatMountCB::RawReadL(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt anOffset,const RMessagePtr2& aMessage) const
//
// Read aLength of data from disk directly to thread relative descriptor
//
    {
    iRawDisk->ReadL(aPos,aLength,aTrg,aMessage,anOffset, 0);
    }

//-----------------------------------------------------------------------------------------

void CFatMountCB::RawWriteL(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt anOffset,const RMessagePtr2& aMessage)
//
// Write aLength of data from thread relative descriptor to disk
//
    {
    CheckWritableL();

	//-- check if we are trying to write to the FAT directly and wait until FAT scan thread finishes in this case.
    FAT().RequestRawWriteAccess(aPos, aLength);

    iRawDisk->WriteL(aPos,aLength,aSrc,aMessage,anOffset, 0);
    //-- Note: FAT directory cache will be invalidated in MountL()
    }

//-----------------------------------------------------------------------------------------
/**
    This method must be used when writing to the FAT directory file.
    If FAT directory cache is present on this drive, it will be used.
    @param  aPos    entry media position
    @param  aDes    data descriptor
*/
void CFatMountCB::DirWriteL(const TEntryPos& aPos,const TDesC8& aDes)
    {
        CheckWritableL();
        const TInt64 posAddr=MakeLinAddrL(aPos);

        if(!iRawDisk->DirCacheInterface())
            {
            iRawDisk->WriteCachedL(posAddr,aDes);
            }
        else
            {//-- if there is an interface to the FAT directory cache, use it
            iRawDisk->DirCacheInterface()->WriteL(posAddr, aDes);
            }
    }

//-----------------------------------------------------------------------------------------

/**
    This method must be used when reading from the FAT directory file.
    If FAT directory cache is present on this drive, it will be used.

    @param  aPos    entry media position
    @param  aLength how many bytes to read
    @param  aDes    input data descriptor
*/
void CFatMountCB::DirReadL(const TEntryPos& aPos, TInt aLength, TDes8& aDes) const
    {
        const TInt64 posAddr=MakeLinAddrL(aPos);

        if(!iRawDisk->DirCacheInterface())
            {
            iRawDisk->ReadCachedL(posAddr,aLength,aDes);
            }
        else
            {//-- if there is an interface to the FAT directory cache, use it
            iRawDisk->DirCacheInterface()->ReadL(posAddr, aLength, aDes);
            }
    }

//-----------------------------------------------------------------------------------------

/**
    Write a FAT directory entry to disk. Assumes sufficient space has been created for it by AddDirEntry.

    @param  aPos        dir. entry position 
    @param  aDirEntry   entry data
*/
void CFatMountCB::WriteDirEntryL(const TEntryPos& aPos,const TFatDirEntry& aDirEntry)
    {

    __PRINT2(_L("CFatMountCB::WriteDirEntryL cl:%d, pos:%d"), aPos.Cluster(), aPos.Pos());

    //-- use special interface to access FAT directory file
    DirWriteL(aPos,TPtrC8((TUint8*)&aDirEntry,KSizeOfFatDirEntry));
    }

//-----------------------------------------------------------------------------------------

/**
    Mark a dir entry as erased
    @param  aPos dir. entry position 
*/
void CFatMountCB::EraseDirEntryL(const TEntryPos& aPos)
    {
    __PRINT2(_L("CFatMountCB::EraseDirEntryL cl:%d, pos:%d"), aPos.Cluster(), aPos.Pos());

    //-- use special interface to access FAT directory file
    DirWriteL(aPos,TPtrC8((TUint8*)&KEntryErasedMarker,sizeof(TUint8)));
    }

//-----------------------------------------------------------------------------------------

/**
    Read a FAT directory entry 
    @param  aPos        dir. entry position 
    @param  aDirEntry   entry data
*/
void CFatMountCB::ReadDirEntryL(const TEntryPos& aPos,TFatDirEntry& aDirEntry) const
    {

//  __PRINT(_L("CFatMountCB::ReadDirEntryL"));
    if (IsEndOfClusterCh(aPos.iCluster))
        {
        aDirEntry.InitZ();
        return;
        }
    TPtr8 buf=TPtr8((TUint8*)&aDirEntry,KSizeOfFatDirEntry);

    //-- use special interface to access FAT directory file
    DirReadL(aPos,KSizeOfFatDirEntry,buf);
    }

//-----------------------------------------------------------------------------------------

/**
    Enlarge the disk's size.
    This method can be called only for variable size media, i.e. RAM drive

    @param aSize size increment (bytes)
*/
void CFatMountCB::EnlargeL(TInt aSize)
    {
    __PRINT2(_L("CFatMountCB::EnlargeL by 0x%x currentsize=0x%x"),aSize,iSize);

    ASSERT(iRamDrive);

    TInt maxSize;
    if (HAL::Get(HAL::EMaxRAMDriveSize, maxSize) == KErrNone && iSize + aSize > maxSize)
        User::Leave(KErrDiskFull);
    User::LeaveIfError(LocalDrive()->Enlarge(aSize));
    iSize+=aSize;

    if (&FAT())
        {
        FAT().InitializeL();
        }

    if (&RawDisk())
        {
        RawDisk().InitializeL();
        }

    }

//-----------------------------------------------------------------------------------------

void CFatMountCB::ReduceSizeL(TInt aPos,TInt aLength)
//
// Reduce the disk's size
//
    {

    __PRINT2(_L("CFatMountCB::ReduceSizeL aPos=0x%x aLength=0x%x"),aPos,aLength);
    User::LeaveIfError(LocalDrive()->ReduceSize(aPos,aLength));
    iSize-=aLength;
    }

//-----------------------------------------------------------------------------------------

TInt64 CFatMountCB::MakeLinAddrL(const TEntryPos& aPos) const
//
// Convert cluster/position into linear address
//
    {

    //__PRINT2(_L("CFatMountCB::MakeLinAddrL, cl:%d, pos:%d"), aPos.iCluster, aPos.iPos);
    if (!IsRootDir(aPos))
        {
        TInt relPos=ClusterRelativePos(aPos.iPos);
        return FAT().DataPositionInBytesL(aPos.iCluster)+relPos;
        }
    if (aPos.iPos+StartOfRootDirInBytes()>=RootDirEnd())
        User::Leave(KErrDirFull); // Past last root dir entry
    return StartOfRootDirInBytes()+aPos.iPos;
    }

//-----------------------------------------------------------------------------------------

void CFatMountCB::GetShortNameL(const TDesC& aLongName,TDes& aShortName)
//
// Get the short name associated with a long file name
//
    {
    __PRINT(_L("CFatMountCB::GetShortNameL"));
    TEntryPos firstEntryPos(RootIndicator(),0);
    TFatDirEntry firstEntry;
    FindEntryStartL(aLongName,KEntryAttMaskSupported,firstEntry,firstEntryPos);
    MoveToDosEntryL(firstEntryPos,firstEntry);
    TBuf8<0x20> dosName(DosNameFromStdFormat(firstEntry.Name()));
    LocaleUtils::ConvertToUnicodeL(aShortName, dosName);
    }

//-----------------------------------------------------------------------------------------

void CFatMountCB::GetLongNameL(const TDesC& aShortName,TDes& aLongName)
//
// Get the long name associated with a short file name
//
    {
    __PRINT(_L("CFatMountCB::GetLongNameL"));
    TEntryPos pos(RootIndicator(),0);
    TFatDirEntry entry;
    const TInt namePos=aShortName.LocateReverse(KPathDelimiter)+1; // There is always a path delimiter
    const TPtrC shortNameWithoutPathDelimiter(aShortName.Mid(namePos));
    __ASSERT_ALWAYS(shortNameWithoutPathDelimiter.Length()<=12,User::Leave(KErrBadName));

    TLeafDirData leafDir;
    pos.iCluster=FindLeafDirL(aShortName.Left(namePos), leafDir);
    
    for(;;)
        {
        TFatDirEntry startEntry;
        User::LeaveIfError(GetDirEntry(pos,entry,startEntry,aLongName));
        if (entry.IsEndOfDirectory())
            User::Leave(KErrNotFound);
        TBool entryIsVFat=EFalse;
        if (startEntry.IsVFatEntry())
            entryIsVFat=ETrue;
        if (!entry.IsParentDirectory() && !entry.IsCurrentDirectory() && !entry.IsGarbage() && !entry.IsErased())
            {
            TBuf8<0x20> entryName8(DosNameFromStdFormat(entry.Name()));
            TBuf<0x20> entryName;
            LocaleUtils::ConvertToUnicodeL(entryName, entryName8);
            if (shortNameWithoutPathDelimiter.MatchF(entryName)!=KErrNotFound)
                {
                if(!entryIsVFat)
                    aLongName=shortNameWithoutPathDelimiter;
                
                break;
                }
            }
        MoveToNextEntryL(pos);
        }//for(;;)
    }



//-----------------------------------------------------------------------------------------

/**
    Extend a file or directory, zeroing cluster chain and flushing after every write to FAT.
    This method is called for rugged FAT only.
    for parameters see CFatTable::ExtendClusterListL
*/
void CFatMountCB::ExtendClusterListZeroedL(TUint32 aNumber, TUint32& aCluster)
    {
    __PRINT(_L("CFatMountCB::ExtendClusterListZeroedL"));
    __ASSERT_DEBUG(aNumber>0,Fault(EFatBadParameter));

    while(aNumber && FAT().GetNextClusterL(aCluster))
        aNumber--;

    //-- request aNumber free clusters from the FAT, this request may wait until FAT scan thread counted enough free clusters if it is running.
    if(!FAT().RequestFreeClusters(aNumber))
        {
        __PRINT(_L("CFatMountCB::ExtendClusterListL - leaving KErrDirFull"));
        User::Leave(KErrDiskFull);
        }
    while (aNumber--)
        {
        TInt freeCluster=FAT().AllocateSingleClusterL(aCluster);
        FAT().FlushL();
        ZeroDirClusterL(freeCluster);
        FAT().WriteL(aCluster,freeCluster);
        FAT().FlushL();
        aCluster=freeCluster;
        }
    }

//-----------------------------------------------------------------------------------------

#if defined(_DEBUG)
TInt CFatMountCB::ControlIO(const RMessagePtr2& aMessage,TInt aCommand,TAny* aParam1,TAny* aParam2)
//
// Debug function
//
    {
    if(aCommand>=EExtCustom)
        {
        if(LocalDrive())
            return LocalDrive()->ControlIO(aMessage,aCommand-EExtCustom,aParam1,aParam2);
        else
            return KErrNotSupported;
        }
    switch(aCommand)
        {
        case ECriticalWriteFailOn:
            {
            TInt r;
            TInt16 args[2];
            TPtr8 des((TUint8*)args,4,4);
            TRAP(r,aMessage.ReadL(2,des,0));
            if(r!=KErrNone)
                return(r);
            SetWriteFail(ETrue);
            SetWriteFailCount(args[0]);
            SetWriteFailError(args[1]);
            break;
            }
        case ECriticalWriteFailOff:SetWriteFail(EFalse);break;
        case ERuggedFSysOn: SetRuggedFSys(ETrue);break;
        case ERuggedFSysOff: SetRuggedFSys(EFalse);break;
        case EIsRuggedFSys:
            {
            TInt r;
            TUint8 val = (IsRuggedFSys()!=0); // val = 0 or 1 for false/true
            TPtr8 pVal(&val,1,1);
            TRAP(r,aMessage.WriteL(2,pVal,0));
            if(r!=KErrNone)
                return(r);
            break;
            }
		case ELocalTimeForRemovableMediaOn:
			{
			FatFileSystem().SetUseLocalTime(ETrue);
			break;
			}
		case ELocalTimeForRemovableMediaOff:
			{
			FatFileSystem().SetUseLocalTime(EFalse);
			break;
			}
		case ELocalTimeUsedOnRemovableMedia:
			{
			TBool flag = FatFileSystem().GetUseLocalTime();
			TPckgC<TBool> flagPckg(flag);
			TInt r = aMessage.Write(2, flagPckg);
			if(r!=KErrNone)
				return r;
			break;
			}
		case ECreationTime:
			{
			CheckStateConsistentL();

			TEntryPos firstEntryPos(RootIndicator(),0);
			TFatDirEntry firstEntry;
			//RFs::ControlIO restricts you to use narrow descriptors
			//so convert narrow back to wide.
			TBuf8<KMaxPath> fileNameNarrow;
			aMessage.Read(2, fileNameNarrow);

			TFileName fileNameWide;
			fileNameWide.Copy(fileNameNarrow);

			//find the long file name entry
			TRAPD(r, FindEntryStartL(fileNameWide,KEntryAttMaskSupported,firstEntry,firstEntryPos) );
			if(r!=KErrNone)
              return(r);
			//Find the corresponding 8.3 short name entry, for metadata
			MoveToDosEntryL(firstEntryPos,firstEntry);
			TTime creationTime=0;
			TPckg<TTime> timePckg(creationTime);
			SFatDirEntry* sEntry = reinterpret_cast<SFatDirEntry*>(firstEntry.iData);
			creationTime = DosTimeToTTime(sEntry->iTimeC, sEntry->iDateC);
			r = aMessage.Write(3, timePckg);
			if(r!=KErrNone)
				return r;
			break;
			}
		case EDisableFATDirCache:
			{
		    MWTCacheInterface* pDirCache = iRawDisk->DirCacheInterface();
		    TUint32 KEDisableFATDirCache = MWTCacheInterface::EDisableCache;
		    pDirCache->Control(KEDisableFATDirCache, (TUint32) aParam1, NULL);
			break;
			}
		case EDumpFATDirCache:
			{
		    MWTCacheInterface* pDirCache = iRawDisk->DirCacheInterface();
		    if (pDirCache)
		        {
	            TUint32 KEDumpFATDirCache = MWTCacheInterface::EDumpCache;
	            pDirCache->Control(KEDumpFATDirCache, 0, NULL);
		        }
			break;
			}
		case EFATDirCacheInfo:
			{
			MWTCacheInterface* pDCache = iRawDisk->DirCacheInterface();
		    if (pDCache)
		        {
	            TUint32 KEFATDirCacheInfo = MWTCacheInterface::ECacheInfo;
	            TDirCacheInfo aInfo;
	            TInt r = pDCache->Control(KEFATDirCacheInfo, 0, static_cast<TAny*>(&aInfo));
	            if (r == KErrNone)
	                {
	                TPckgBuf<TDirCacheInfo> pkgBuf(aInfo);
	                r = aMessage.Write(2,pkgBuf);
	                }
                return r;
		        }
		    return KErrNotSupported;
			}


        default: return(KErrNotSupported);
        }
    return(KErrNone);
    }
#else
TInt CFatMountCB::ControlIO(const RMessagePtr2& /*aMessage*/,TInt /*aCommand*/,TAny* /*aParam1*/,TAny* /*aParam2*/)
    {return(KErrNotSupported);}
#endif


//-----------------------------------------------------------------------------------------

TInt CFatMountCB::Lock(TMediaPassword& aOld,TMediaPassword& aNew,TBool aStore)
//
// lock media device
//
    {
    __PRINT(_L("CFatMountCB::Lock"));
	TInt r=CreateDrive(Drive().DriveNumber());
    if (r!=KErrNone)
        return r;

    TBusLocalDrive* local;
    r=LocalDrive()->GetLocalDrive(local);
    if (r!=KErrNone)
        return r;

#ifdef _LOCKABLE_MEDIA
    if(local->Status()==KErrLocked)
        local->Status() = KErrNotReady;
#endif
    r=local->SetPassword(aOld,aNew,aStore);
    if(r==KErrNone&&aStore)
        WritePasswordData();
    return(r);
    }

//-----------------------------------------------------------------------------------------

TInt CFatMountCB::Unlock(TMediaPassword& aPassword,TBool aStore)
//
// Unlock media device
//
    {
    __PRINT(_L("CFatMountCB::Unlock"));
	TInt r=CreateDrive(Drive().DriveNumber());
    if (r!=KErrNone)
        return r;

    TBusLocalDrive* local;
    r=LocalDrive()->GetLocalDrive(local);
    if (r!=KErrNone)
        return r;

#ifdef _LOCKABLE_MEDIA
    if(local->Status()==KErrLocked)
        local->Status() = KErrNotReady;
#endif
    r=local->Unlock(aPassword,aStore);
    if(r==KErrNone&&aStore)
        WritePasswordData();
    return(r);
    }

//-----------------------------------------------------------------------------------------

TInt CFatMountCB::ClearPassword(TMediaPassword& aPassword)
//
// Clear password from media device
//
    {
    __PRINT(_L("CFatMountCB::ClearPassword"));
	TInt r=CreateDrive(Drive().DriveNumber());
    if (r!=KErrNone)
        return r;

    TBusLocalDrive* local;
    r=LocalDrive()->GetLocalDrive(local);
    if (r!=KErrNone)
        return r;

#ifdef _LOCKABLE_MEDIA
    if(local->Status()==KErrLocked)
        local->Status() = KErrNotReady;
#endif
    r=local->Clear(aPassword);
    if(r==KErrNone)
        WritePasswordData();
    return(r);
    }

//-----------------------------------------------------------------------------------------

TInt CFatMountCB::ErasePassword()
//
// Forcibly erase the password from a media device
//
    {
    __PRINT(_L("CFatMountCB::ErasePassword"));

	TInt r=CreateDrive(Drive().DriveNumber());
    if (r!=KErrNone)
        return r;

    TBusLocalDrive* local;
    r=LocalDrive()->GetLocalDrive(local);
    if (r!=KErrNone)
        return r;

#ifdef _LOCKABLE_MEDIA
    if(local->Status()==KErrLocked)
        local->Status() = KErrNotReady;
#endif
    r=local->ErasePassword();
    if(r==KErrNone)
        {
        //-- ReMount whole driver stack since MBR may have been rewritten and partition may have moved / changed size.
        //-- this is mostly applicable to SD cards formatting, since SD stack takes care of creating partition table.
        //-- use KForceMediaChangeReOpenAllMediaDrivers flag that will cause remounting media 
        //-- drivers associatied with the current partition only and not affecting other ones.
        r = LocalDrive()->ForceRemount((TUint)RFs::KForceMediaChangeReOpenMediaDriver);
        
        local->Status() = KErrNotReady;
        WritePasswordData();
        }
    return(r);
    }

//-----------------------------------------------------------------------------------------

TInt CFatMountCB::ForceRemountDrive(const TDesC8* aMountInfo,TInt aMountInfoMessageHandle,TUint aFlags)
//
// Force a remount of the drive
//
    {
    __PRINT(_L("CFatMountCB::ForceRemountDrive"));
	TInt r=CreateDrive(Drive().DriveNumber());
    if (r==KErrNone)
		r=LocalDrive()->SetMountInfo(aMountInfo,aMountInfoMessageHandle);
    if (r==KErrNone)
        r=LocalDrive()->ForceRemount(aFlags);
    return(r);
    }

//-----------------------------------------------------------------------------------------

void CFatMountCB::WritePasswordData()
//
// Write store password data to disk
//
    {
    __PRINT(_L("CFatMountCB::WritePasswordData"));
    TBuf<sizeof(KMediaPWrdFile)> mediaPWrdFile(KMediaPWrdFile);
    mediaPWrdFile[0] = (TUint8) RFs::GetSystemDriveChar();
    __PRINT1TEMP(_L("disk file = %S"),mediaPWrdFile);
    TBusLocalDrive& local=GetLocalDrive(Drive().DriveNumber());
    TInt length=local.PasswordStoreLengthInBytes();
    if(length==0)
        {
        WriteToDisk(mediaPWrdFile,_L8(""));
        return;
        }
    HBufC8* hDes=HBufC8::New(length);
    if(hDes==NULL)
        return;
    TPtr8 pDes=hDes->Des();
    TInt r=local.ReadPasswordData(pDes);
    if(r==KErrNone)
        WriteToDisk(mediaPWrdFile,pDes);
    delete hDes;
    }

//-----------------------------------------------------------------------------------------

/**
Trim trailing spaces of volume label descriptor and adjust its length
*/
void CFatMountCB::TrimVolumeLabel(TDes8& aLabel) const
    {
    // Locate first '\0'
    TInt nullPos = aLabel.Locate('\0');
    if (nullPos == KErrNotFound)
        nullPos = KVolumeLabelSize;

    // Trim trailing spaces
    TInt i;
    for (i=nullPos-1; i>=0; --i)
        if (aLabel[i] != 0x20)
            break;
    aLabel.SetLength(i+1);
    }

//-----------------------------------------------------------------------------------------

/**
Searches for the volume label file

@param aLabel The name of the volume label file returned upon successful search
@return KErrNone if it finds the volume label file, otherwise KErrNotFound
*/
TInt CFatMountCB::ReadVolumeLabelFile(TDes8& aLabel)
    {
    __PRINT(_L("+CFatMountCB::ReadVolumeLabelFile"));
    TEntryPos pos(RootIndicator(),0);
    TFatDirEntry entry;
    TRAPD(r, FindVolumeLabelFileL(aLabel, pos, entry));
    __PRINT1(_L("-CFatMountCB::ReadVolumeLabelFile: %d"),r);
    return r;
    }

//-----------------------------------------------------------------------------------------

/**
Creates or updates the volume label file with name aNewName

@param aNewName The new name for the volume label file
*/
void CFatMountCB::WriteVolumeLabelFileL(const TDesC8& aNewName)
    {
    __PRINT1(_L("+CFatMountCB::WriteVolumeLabelFileL: [%S]"), &aNewName);
    TEntryPos pos(RootIndicator(),0);
    TFatDirEntry entry;

    TBuf8<KVolumeLabelSize> oldName;
    TRAPD(r, FindVolumeLabelFileL(oldName, pos, entry));

    if( KErrNone == r )
        {
        // Found existing volume label file, rename or delete
        if(oldName == aNewName)
            {
            __PRINT(_L("-CFatMountCB::WriteVolumeLabelFileL: found: names match"));
            return;
            }
        else
            {
            if(aNewName.Length() == 0)
                {
                // delete the volume label file
                __PRINT(_L("CFatMountCB::WriteVolumeLabelFileL: found: delete"));
                EraseDirEntryL(pos, entry);
                }
            else
                {
                __PRINT(_L("CFatMountCB::WriteVolumeLabelFileL: found: replace"));
                entry.SetName(aNewName);
                WriteDirEntryL(pos, entry);
                }
            FAT().FlushL();
            }
        }
    else if( KErrNotFound == r )
        {
        // Not found, need to create if aNewName is not empty
        // Windows allows a volume label file to have the same name as
        // an existing file or directory
        if(aNewName.Length() > 0)
            {
            __PRINT(_L("CFatMountCB::WriteVolumeLabelFileL: not found: create"));
            TEntryPos dirPos(RootIndicator(),0);
            AddDirEntryL(dirPos,1);
            TFatDirEntry fatDirEntry;
            fatDirEntry.SetName(aNewName);
            fatDirEntry.SetAttributes(KEntryAttVolume);

            TTime now;
			now.UniversalTime();
			fatDirEntry.SetTime(now, TimeOffset() );
            fatDirEntry.SetStartCluster(0);
            fatDirEntry.SetSize(0);
            WriteDirEntryL(dirPos, fatDirEntry);
            FAT().FlushL();
            }
        }
    else
        {
        // Some other error
        User::Leave(r);
        }
    }


//-----------------------------------------------------------------------------------------

/**
Scans the root directory for a volume label file. Leaves with an error if not found

@param aLabel Name of the volume label file upon successful search
@param aDosEntryPos Pointer to position of the volume label file upon successful search
@param aDosEntry Contains the entry for the volume label file upon successful search
*/
void CFatMountCB::FindVolumeLabelFileL(TDes8& aLabel, TEntryPos& aDosEntryPos, TFatDirEntry& aDosEntry)
    {
    __PRINT(_L("+CFatMountCB::FindVolumeLabelFileL"));

    if(IsRootDir(aDosEntryPos) && (aDosEntryPos.iPos+StartOfRootDirInBytes()>=RootDirEnd()))
        {
        __PRINT(_L("-CFatMountCB::FindVolumeLabelFileL: abort, exceeds root"));
        User::Leave(KErrNotFound); // Allows maximum number of entries in root directory
        }

    TUint32 previousCluster= aDosEntryPos.iCluster;
    TUint previousPosition= aDosEntryPos.iPos;
    TUint32 changePreviousCluster=1;
    TUint32 count=0;

    TFatDirEntry startEntry;
    TFileName dummyLongName;

    FOREVER
        {
        User::LeaveIfError(GetDirEntry(aDosEntryPos, aDosEntry, startEntry, dummyLongName));

        if(aDosEntry.IsEndOfDirectory())
            {
            __PRINT(_L("-CFatMountCB::FindVolumeLabelFileL: end of dir"));
            User::Leave(KErrNotFound);
            }

        if(IsRootDir(aDosEntryPos) && (aDosEntryPos.iPos+StartOfRootDirInBytes()==(RootDirEnd()-KSizeOfFatDirEntry)))
            {
            if(aDosEntry.IsErased())
                {
                __PRINT(_L("-CFatMountCB::FindVolumeLabelFileL: erased end of root"));
                User::Leave(KErrNotFound); //Allows maximum number of entries in root directory
                }
            }

        if(!aDosEntry.IsCurrentDirectory() && !aDosEntry.IsParentDirectory() && !aDosEntry.IsErased() && !aDosEntry.IsGarbage())
            {
            if(aDosEntry.Attributes() & KEntryAttVolume)
                {
                aLabel = aDosEntry.Name();
                dummyLongName.Copy(aLabel);
                __PRINT1(_L("-CFatMountCB::FindVolumeLabelFileL: found [%S]"), &dummyLongName);
                break;
                }
            }
        
        MoveToNextEntryL(aDosEntryPos);
        
        if(IsRootDir(aDosEntryPos) && (aDosEntryPos.iPos+StartOfRootDirInBytes()>=RootDirEnd()))
            {
            __PRINT(_L("-CFatMountCB::FindVolumeLabelFileL: Not found"));
            User::Leave(KErrNotFound); //Allows maximum number of entries in root directory
            }
        
        if(aDosEntryPos.iCluster && (aDosEntryPos.iPos <= previousPosition))
            {
            DoCheckFatForLoopsL(aDosEntryPos.iCluster, previousCluster, changePreviousCluster, count);
            }

        previousPosition=aDosEntryPos.iPos;
        }
    }

//-----------------------------------------------------------------------------------------

/**
Read volume label from disk and trim trailing 0x20 & 0x00 characters
*/
void CFatMountCB::GetVolumeLabelFromDiskL(const TFatBootSector& aBootSector)
    {
    // Read volume label as 8 bit descriptor
    TBuf8<KVolumeLabelSize> volName8;
    TInt r = ReadVolumeLabelFile(volName8);
    if(r != KErrNone)   // No volume label file in root directory
        volName8 = aBootSector.VolumeLabel();
    TrimVolumeLabel(volName8);

    TBuf16<KVolumeLabelSize> volName16;
    LocaleUtils::ConvertToUnicodeL(volName16, volName8);
    SetVolumeName(volName16.AllocL());
    }


//-----------------------------------------------------------------------------------------

/**
Populates iMap member of aInfo with contiguous block group maps.

@param aPos     Start position for a desired section of the file.
@param sLength  Length of the desired data to produce the block map for.
@param aInfo    A structure describing a group of block maps.
*/
void CFatMountCB::BlockMapReadFromClusterListL(TEntryPos& aPos, TInt aLength, SBlockMapInfo& aInfo)
    {
    __PRINT(_L("CFatMountCB::BlockMapReadFromClusterListL"));
    __ASSERT_ALWAYS(aPos.Cluster()>=KFatFirstSearchCluster,User::Leave(KErrCorrupt));
    TBlockMapEntry blockMapEntry;

    TUint i = 0;
    TInt clusterRelativePos;
    TInt maxClusters;
    TUint32 endCluster;
    TInt clusterListLen;
    TInt readLength;
    TInt temp;
    TInt currentPos;
    TLocalDriveCapsBuf caps;
    TInt r;
    TInt64 realPosition = 0;

    do
        {
        currentPos = aPos.iPos;
        temp = currentPos>>ClusterSizeLog2();
        if ( (currentPos) && ( (currentPos) == (temp<<ClusterSizeLog2()) ) )
            {
            if (!FAT().GetNextClusterL(aPos.iCluster))
                {
				__PRINT(_L("CFatMountCB::BlockMapReadFromClusterListL corrupt#1"))
                User::Leave(KErrCorrupt);
                }
            }
        clusterRelativePos = ClusterRelativePos( aPos.iPos );
        maxClusters = ((aLength + clusterRelativePos - 1)>>ClusterSizeLog2())+1;
        clusterListLen = FAT().CountContiguousClustersL(aPos.iCluster, endCluster, maxClusters);
        readLength = Min( aLength, (clusterListLen<<ClusterSizeLog2()) - clusterRelativePos);

        blockMapEntry.SetNumberOfBlocks( clusterListLen );
        if (aPos.iCluster < 2)
            User::Leave(KErrCorrupt);
        r = LocalDrive()->Caps(caps);
        if ( r != KErrNone )
            User::LeaveIfError(r);
        if ( caps().iType&EMediaRam )
            {
            realPosition = FAT().DataPositionInBytesL( aPos.iCluster );
            aPos.iCluster = I64LOW((realPosition - aInfo.iStartBlockAddress)>>ClusterSizeLog2());
            blockMapEntry.SetStartBlock( aPos.iCluster );
            }
        else
            blockMapEntry.SetStartBlock( aPos.iCluster - 2);
        aInfo.iMap.Append(TPckgC<TBlockMapEntry>(blockMapEntry));
        aPos.iPos += readLength;
        aPos.iCluster = endCluster;
        aLength -= readLength;
        }
    while( ( aLength > 0 ) && ( ++i < KMaxMapsPerCall ) );
    }


//-----------------------------------------------------------------------------------------

TInt CFatMountCB::GetDosEntryFromNameL(const TDesC& aName, TEntryPos& aDosEntryPos, TFatDirEntry& aDosEntry)
    {
    TFatDirEntry firstEntry;
    TEntryPos firstEntryPos(RootIndicator(),0); // Already checked entry is a directory
    FindEntryStartL(aName,KEntryAttMaskSupported,firstEntry,firstEntryPos);

    aDosEntryPos=firstEntryPos;
    aDosEntry=firstEntry;
    MoveToDosEntryL(aDosEntryPos,aDosEntry);

    return KErrNone;
    }

//-----------------------------------------------------------------------------------------

TInt CFatMountCB::GetFileUniqueId(const TDesC& aName, TInt64& aUniqueId)
    {
    // Get first cluster of file
    TEntryPos dosEntryPos(RootIndicator(),0);
    TFatDirEntry dosEntry;
    InitializeRootEntry(dosEntry);  // Nugatory initialisation to placate warnings
    TRAPD(err,GetDosEntryFromNameL(aName,dosEntryPos,dosEntry));
    if(err!=KErrNone)
        return err;

    TInt startCluster=StartCluster(dosEntry);
    // Empty files will return a cluster of zero
    if(startCluster==0)
        return KErrEof;

    aUniqueId=MAKE_TINT64(0,startCluster);
    return KErrNone;
    }
//-----------------------------------------------------------------------------------------


TInt CFatMountCB::Spare3(TInt /*aVal*/, TAny* /*aPtr1*/, TAny* /*aPtr2*/)
    {
    return KErrNotSupported;
    }

TInt CFatMountCB::Spare2(TInt /*aVal*/, TAny* /*aPtr1*/, TAny* /*aPtr2*/)
    {
    return KErrNotSupported;
    }

TInt CFatMountCB::Spare1(TInt /*aVal*/, TAny* /*aPtr1*/, TAny* /*aPtr2*/)
    {
    return KErrNotSupported;
    }

//-----------------------------------------------------------------------------------------

/** 
    Check file system for errors. 
    @return KErrNone if no errors found, otherwise a error code hopefully describing the problem found.
*/
TInt CFatMountCB::CheckDisk()
	{

    __PRINT1(_L("CFatMountCB::CheckDisk() drv:%d"), DriveNumber());

    if(!ConsistentState())
        return KErrCorrupt;

    //-- create a bit representation of the FAT
    const TUint32 MaxClusters = UsableClusters()+KFatFirstSearchCluster; //-- UsableClusters() doesn't count first 2 unused clusers
    if (MaxClusters == 0)
        return KErrCorrupt;

    //-- used for measuring time
    TTime   timeStart;
    TTime   timeEnd;
    timeStart.UniversalTime(); //-- take start time

    TInt nRes;
 
    CScanDrive* pScnDrv = NULL;
    TRAP(nRes, pScnDrv=CScanDrive::NewL(this));
    if(nRes != KErrNone)
        return nRes;

    //-- start ScanDrive in "checkdisk" mode
    TRAPD(nScnDrvRes, pScnDrv->StartL(CScanDrive::ECheckDisk));
    
    timeEnd.UniversalTime(); //-- take end time
    const TInt msScanTime = (TInt)( (timeEnd.MicroSecondsFrom(timeStart)).Int64() / K1mSec);
    (void)msScanTime;
    __PRINT1(_L("#@@@ CheckDisk() time taken:%d ms"), msScanTime);
 
    CScanDrive::TGenericError chkDskRes = pScnDrv->ProblemsDiscovered();
    const TBool bProblemsFound = (nScnDrvRes!=KErrNone) || pScnDrv->ProblemsDiscovered();
    
    if(bProblemsFound && chkDskRes == CScanDrive::ENoErrors)
        {//-- ScanDrive in this mode can leave unexpectedly without setting an error code that is returned by ProblemsDiscovered();
         //-- leave itself means a problem
        chkDskRes = nScnDrvRes == KErrNone ? CScanDrive::EUnknownError : (CScanDrive::TGenericError) nScnDrvRes;
        }

    delete pScnDrv;
    pScnDrv = NULL;

    if(chkDskRes != KErrNone)
        {
        __PRINT2(_L("CFatMountCB::CheckDisk() drv:%d, result:%d"), DriveNumber(), chkDskRes);
        }
    
    return chkDskRes;

    }


//-------------------------------------------------------------------------------------------------------------------

/**
    Creates a scan drive object and starts the scan.
*/
TInt CFatMountCB::DoRunScanDrive()
{
    TInt nRes;

    CScanDrive* pScnDrv = NULL;
    TRAP(nRes, pScnDrv=CScanDrive::NewL(this));
    if(nRes != KErrNone)
        return nRes;

    TRAPD(nScnDrvRes, pScnDrv->StartL(CScanDrive::EScanAndFix));

    const TBool bNeedFatRemount = (nScnDrvRes!=KErrNone) || pScnDrv->ProblemsDiscovered();
    delete pScnDrv;
    pScnDrv = NULL;

    if(bNeedFatRemount)
        {//-- ScanDrive found and probably fixed some errors.
        // ensure cached fat and free cluster count are updated
        DoDismount(); //-- dismount
        TRAP(nRes, MountL(EFalse)); //-- mount again
        }

    if(nScnDrvRes != KErrNone)
        return nScnDrvRes;


    //-- if ScanDrive hasn't found anything wrong or has fixed recoverable errors, mark the volume clean
    if(VolCleanFlagSupported())
        {
        //-- if there is a background FAT scanning thread, we need to wait until it finishes its work.
        //-- otherwise it's possible to have incorrect amount of free space on the volume until next remounting.
        (void)FAT().NumberOfFreeClusters(ETrue);
        TRAP(nRes, FinaliseMountL());
        ASSERT(nRes == KErrNone);
        }

    return KErrNone;
}

//-------------------------------------------------------------------------------------------------------------------

/**
    Run scan drive on the given volume.
    The ScanDrive may be skipped on the finalised volumes, i.e. those, that had been shut down properly.


    @return Either  KErrCorrupt if an error was found that is not caused by write failure due to power removal.
                    KErrNone if no error was found. One of four positive codes explaining what type of error was rectified
*/
TInt CFatMountCB::ScanDrive()
{
    __PRINT1(_L("CFatMountCB::ScanDrive() starting on drive %d"), DriveNumber());

    if(!ConsistentState())
        return KErrCorrupt;

    TInt nRes;

    if(LockStatus()!=0)
        {//-- can't run if the volume has opened objects, like files, directories, formats etc.
		__PRINT(_L("CFatMountCB::ScanDrive() locked!\n"));
        return KErrInUse;
        }

    if(iRamDrive)
        {//-- Do not check internal RAM drive
        __PRINT(_L("CFatMountCB::ScanDrive() Skipping Internal RAM drive."));
        return KErrNone;
        }

    //-- check if the volume is finalised and skip running ScanDrive if this option is enabled in estart.txt
    if(VolCleanFlagSupported() && FatConfig().ScanDrvSkipFinalisedVolume())
        {
        TBool bVolClean = EFalse;
        TRAP(nRes, bVolClean = VolumeCleanL());

        if(nRes == KErrNone && bVolClean)
            {
            __PRINT(_L("Skipping ScanDrive on finalised volume!"));
            return KErrNone; //-- skip ScanDrive on a clean volume
            }
        }

    //-- run ScanDrive
    nRes = Open();
    if(nRes != KErrNone)
        return nRes;

    nRes = DoRunScanDrive();

    Close();

    __PRINT2(_L("~ CFatMountCB::ScanDrive() finished for drive %d with the code %d"),DriveNumber(), nRes);

    return nRes;

}

//-----------------------------------------------------------------------------------------
/**
Returns the offset between UTC time and timestamps on the filesystem. This will return User::UTCOffset
if the flag iUseLocalTime has been set in CFatFileSystem and this mount is on a removable drive. If not
a null offset is returned.

@return The offset in seconds that timestamps on the filesystem have, relative to UTC.
*/
TTimeIntervalSeconds CFatMountCB::TimeOffset() const
	{
    if((Drive().Att() & KDriveAttRemovable) && FatFileSystem().GetUseLocalTime() )
        return User::UTCOffset();

    
    return TTimeIntervalSeconds(0);
	}




//-----------------------------------------------------------------------------------------
/** 
    Check is this file system can be mounted on the drive at all.
    Just read and validate boot region, no real mounting overhead. 
    
    @return KErrNone    boot region is OK, the file system can be mounted.
            KErrLocked  the media is locked on a physical level.
            other error codes depending on the implementation

*/
TInt CFatMountCB::MntCtl_DoCheckFileSystemMountable()
    {
    TInt nRes;
    
    const TInt driveNo = Drive().DriveNumber();
    __PRINT1(_L("CFatMountCB::MntCtl_DoCheckFileSystemMountable() drv:%d"),driveNo);

    nRes = CreateDrive(driveNo);
    if(nRes != KErrNone)
        {
        __PRINT1(_L(" ..CreateDrive() err:%d \n"), nRes);    
        return nRes;
        }

    //-- try reading boot sector. This doesn't require iDriverInterface setup, it uses LocalDrive()
    TFatBootSector bootSector;
    nRes = ReadBootSector(bootSector);

    DismountedLocalDrive();

    return nRes;
    }

//-----------------------------------------------------------------------------------------
/** 
    Internal helper method.
    @param      aFatType FAT type
    @return     End Of Cluster Chain code that depend on FAT type, 0xff8 for FAT12, 0xfff8 for FAT16, and 0xffffff8 for FAT32 
*/
TUint32 EocCodeByFatType(TFatType aFatType)
    {
    switch(aFatType)
        {
        case EFat32: 
        return EOF_32Bit-7; //-- 0xffffff8
        
        case EFat16: 
        return  EOF_16Bit-7; //-- 0xfff8
        
        case EFat12: 
        return  EOF_12Bit-7; //-- 0xff8
        
        default: 
        break;
        }

    ASSERT(aFatType == EInvalid); 
    return 0;
    }

//-----------------------------------------------------------------------------------------
/**
    Set FAT type that this object of CFatMountCB will be dealing with.
*/
void CFatMountCB::SetFatType(TFatType aFatType)
    {
    ASSERT(State() == ENotMounted || State() == EDismounted || State() == EMounting) ;
    
    iFatType = aFatType;
    iFatEocCode = EocCodeByFatType(aFatType);
    }

















