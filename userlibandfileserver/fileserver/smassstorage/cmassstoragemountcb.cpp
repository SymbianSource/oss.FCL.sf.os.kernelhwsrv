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
// CMassStorageMountCB implementation.
//
//

/**
 @file
 @internalTechnology
*/

#include <f32fsys.h>
#include <f32file.h>

#include "drivemanager.h"
#include "cusbmassstoragecontroller.h"
#include "cmassstoragefilesystem.h"
#include "cmassstoragemountcb.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "cmassstoragemountcbTraces.h"
#endif


CMassStorageMountCB::CMassStorageMountCB(const RArray<TInt>& aDriveMapping)
    : iDriveMapping(aDriveMapping)
    {
    }

CMassStorageMountCB* CMassStorageMountCB::NewL(const RArray<TInt>& aDriveMapping)
    {
    return new (ELeave) CMassStorageMountCB(aDriveMapping);
    }

/**
Checks that the drive number is supported.

@leave KErrNotReady The drive number is not supported.
*/
TInt CMassStorageMountCB::CheckDriveNumberL()
    {
    TInt driveNumber;
    driveNumber = Drive().DriveNumber();
    OstTrace1(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEMOUNTCB_100,
              "Drive=%d", driveNumber);
    if (!IsValidLocalDriveMapping(driveNumber))
        {
        OstTrace0(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEMOUNTCB_101,
                  "Drive number not supported");
        User::Leave(KErrNotReady);
        }
    return driveNumber;
    }

/**
Registers the drive with the Mass Storage drive manager.

@leave KErrNotSupported The drive is not compatible with Mass Storage.
*/
void CMassStorageMountCB::MountL(TBool /*aForceMount*/)
    {
    CheckDriveNumberL();
    CMassStorageFileSystem& msFsys = *reinterpret_cast<CMassStorageFileSystem*>(Drive().GetFSys());

    TInt lun = DriveNumberToLun(Drive().DriveNumber());

    OstTrace1(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEMOUNTCB_120,
              "LUN=%d", lun);

    if(lun < 0)
        {
        // This is not a supported Mass Storage drive
        User::Leave(KErrNotSupported);
        }

    TBusLocalDrive& localDrive = msFsys.iLocalDriveForMediaFlag[lun];

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
        OstTrace1(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEMOUNTCB_121,
                  "Drive is not compatible with Mass Storage err=%d", err);
        User::Leave(err);
        }

    // Set media changed to true so that Win2K doesn't used cached drive data
    (*msFsys.iMediaChanged)[lun] = ETrue;

    msFsys.Controller().DriveManager().RegisterDrive(*proxyDrive, (*msFsys.iMediaChanged)[lun], lun);

    SetVolumeName(_L("MassStorage").AllocL());
    }

/**
Returns the LUN that corresponds to the specified drive number.

@param aDriveNumber The drive number.
*/
TInt CMassStorageMountCB::DriveNumberToLun(TInt aDriveNumber)
    {
    TInt lun = -1;
    for (TInt i = 0; i < iDriveMapping.Count(); i++)
        {
        if (iDriveMapping[i] == aDriveNumber)
            {
            lun = i;
            break;
            }
        }
    OstTraceExt2(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEMOUNTCB_130,
                 "Drive %d maps to LUN %d", aDriveNumber, lun);
    return lun;
    }

/**
Deregisters the drive from the Drive Manager.
*/
void CMassStorageMountCB::Dismounted()
    {
    TInt driveNumber = -1;
    TRAPD(err, driveNumber = CheckDriveNumberL());
    if (err != KErrNone)
        {
        return;
        }
    OstTrace1(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEMOUNTCB_140,
              "Dismounted drive %d", driveNumber);
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
    TInt driveNumber = -1;
    TRAPD(err, driveNumber = CheckDriveNumberL());
    OstTrace1(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEMOUNTCB_150,
              "Unlock drive %d", driveNumber);
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
    TBusLocalDrive& local=GetLocalDrive(Drive().DriveNumber());
    OstTrace1(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEMOUNTCB_160,
              "WritePasswordData drive %d", DriveNumber());
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

TInt CMassStorageMountCB::ReMount()
    {
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

