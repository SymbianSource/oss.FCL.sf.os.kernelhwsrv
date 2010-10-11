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
// Class implementation of CDriveManager and CMassStorageDrive.
//
//

/**
 @file
 @internalTechnology
*/

#include <e32std.h>
#include <e32base.h>            // C Class Definitions, Cleanup Stack
#include <e32def.h>             // T Type  Definitions
#include <f32fsys.h>
#include <e32property.h>
#include "usbmsshared.h"        // KUsbMsMaxDrives


#include "drivemanager.h"
#include "smassstorage.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "drivemanagerTraces.h"
#endif



///////////////////////////////////////////////////////////////////////////////


/**
A private structure that, when Connected, holds references to
the CProxyDrive and the corresponding TBusLocalDrive's Media Changed flag.
*/
struct CMassStorageDrive::CLocalDriveRef : public CBase
    {
    CLocalDriveRef(CProxyDrive& aProxyDrive, TBool& aMediaChanged)
        : iProxyDrive(aProxyDrive), iMediaChanged(aMediaChanged), iDriveState(EIdle)
        {
        }
    CProxyDrive& iProxyDrive;
    TBool& iMediaChanged;
    /**
    The Drive Media state machine
    */
    TDriveState iDriveState;
    };

/**
@param aCritSec A Critical Section object shared by all drives.
@param aDrives Reference to the list of CMassStorageDrive objects.
@param aDriveMap Reference to array mapping lun to drive number for supported
       mass storage drives.
@post Object is fully constructed
 */
CMassStorageDrive::CMassStorageDrive(RCriticalSection& aCritSec,
                                     RDriveStateChangedPublisher& aDriveStateChangedPublisher)
    :
    iCritSec(aCritSec),
    iMountState(EDisconnected),
    iDriveStateChangedPublisher(aDriveStateChangedPublisher)
    {
    }

CMassStorageDrive::~CMassStorageDrive()
    {
    delete iLocalDrive;
    }

/**
Read from the target drive unit.
@return KErrNone on success, otherwise system wide error code
*/
TInt CMassStorageDrive::Read(const TInt64& aPos, TInt aLength, TDes8& aBuf, TBool aWholeMedia)
    {
    TInt err = KErrUnknown; // never return this
    iCritSec.Wait();

    if(iMountState != EConnected)
        {
        err = KErrDisconnected;
        }
    else
        {
        if(aWholeMedia)
            {
            err = SafeProxyDrive().Read(aPos, aLength, &aBuf, KLocalMessageHandle, 0, RLocalDrive::ELocDrvWholeMedia);
            }
        else
            {
            err = SafeProxyDrive().Read(aPos, aLength, aBuf);
            }

        if(err == KErrNone)
            {
#ifndef USB_TRANSFER_PUBLISHER
            iBytesRead += aBuf.Length();
            OstTrace1(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_100,
                      "iBytesRead=%d", iBytesRead);
#endif
            }

        else if(err == KErrLocked)
            {
            SetDriveState(ELocked);
            }
        }

    iCritSec.Signal();
    return err;
    }

/**
Write to the target drive unit.
@return KErrNone on success, otherwise system wide error code
*/
TInt CMassStorageDrive::Write(const TInt64& aPos, TDesC8& aBuf, TBool aWholeMedia)
    {
    TInt err = KErrNone;
    iCritSec.Wait();

    if (iMountState != EConnected)
        {
        err = KErrDisconnected;
        }
    else
        {
        __ASSERT_DEBUG(iLocalDrive, User::Panic(KUsbMsSvrPncCat, EMsCMassStorageDriveWrite));
        TDriveState oldState = iLocalDrive->iDriveState;
        if(oldState != EActive)
            {
            // SCSI hasn't called SetCritical
            SetDriveState(EActive);
            }

        if (aWholeMedia)
            {
            err = SafeProxyDrive().Write(aPos, aBuf.Length(), &aBuf, KLocalMessageHandle, 0, RLocalDrive::ELocDrvWholeMedia);
            }
        else
            {
            err = SafeProxyDrive().Write(aPos,aBuf);
            }

        if (err == KErrNone)
            {
#ifndef USB_TRANSFER_PUBLISHER
            iBytesWritten += aBuf.Length();
#endif
            }

        if (err == KErrLocked)
            {
            SetDriveState(ELocked);
            }
        else if (oldState != EActive)
            {
            SetDriveState(oldState);
            }
        }

    iCritSec.Signal();
    return err;
    }

/**
Get the capabilities of the target drive unit.
@return KErrNone on success, otherwise system wide error code
*/
TInt CMassStorageDrive::Caps(TLocalDriveCapsV4& aInfo)
    {
    TInt err = KErrNone;
    iCritSec.Wait();

    if(iMountState != EConnected)
        {
        err = KErrDisconnected;
        }
    else
        {
        // Initialise in case Caps() fails
        aInfo.iType = ::EMediaUnknown;
        err = DoCaps(aInfo);

        OstTrace1(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_110,
                  "err=%d", err);

        if(err == KErrNotReady || (err==KErrNone && aInfo.iType == ::EMediaNotPresent))
            {
            OstTrace0(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_111,
                      "detected MediaNotPresent");
            SetDriveState(CMassStorageDrive::EMediaNotPresent);
            }
        }

    iCritSec.Signal();
    return err;
    }

/**
Provides an interface to CProxyDrive::Caps that hides the
package buffer.
@return KErrNone on success, otherwise system wide error code
@param aInfo
*/
TInt CMassStorageDrive::DoCaps(TLocalDriveCapsV4& aInfo)
    {
    TLocalDriveCapsV4Buf buf;
    buf.FillZ();
    CProxyDrive& pd = SafeProxyDrive();

    TInt err = pd.Caps(buf);
    OstTrace1(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_121,
              "Caps returned %d", err);

    if(err==KErrNone)
        {
        // Invoke function call operator to cast to TLocalDriveCapsV4&
        aInfo = buf();
        }

    return err;
    }

/**
Publish media error, user should reinsert the memory card.
Similar to FAT32's TDriver::HandleCriticalError.
Note: User notification is not implemented, instead we abort and dismount.
*/
TInt CMassStorageDrive::HandleCriticalError()
    {
    iDriveMediaErrorPublisher.PublishError(ETrue);
    return KErrAbort;
    }


TInt CMassStorageDrive::ClearCriticalError()
    {
    iDriveMediaErrorPublisher.PublishError(EFalse);
    return KErrNone;
    }


/**
Checks the Media Changed flag, and optionally resets it.
@return The state of the Media Changed flag.
@param aReset If true, the Media Changed flag is reset to EFalse.
*/
TBool CMassStorageDrive::IsMediaChanged(TBool aReset)
    {
    iCritSec.Wait();

    TBool mediaChanged = EFalse;
    if(iLocalDrive)
        {
        mediaChanged = iLocalDrive->iMediaChanged;
        if(aReset)
            {
            iLocalDrive->iMediaChanged = EFalse;
            }
        }
    else
        {
        OstTrace0(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_130,
                  "No drive");
        }

    iCritSec.Signal();

    OstTrace1(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_131,
              "IsMediaChanged returning %d", mediaChanged);
    return mediaChanged;
    }

/**
Set the Drive State to Active or Idle.
@return KErrNone on success, KErrNotReady if media not present, KErrDisMounted if not mounted
@param aCritical ETrue for Active, EFalse for Idle
*/
TInt CMassStorageDrive::SetCritical(TBool aCritical)
    {
    TInt err = KErrDisMounted;

    iCritSec.Wait();

    if(iLocalDrive)
        {
        if(iLocalDrive->iDriveState == CMassStorageDrive::EMediaNotPresent)
            {
            err = KErrNotReady;
            }
        else
            {
            SetDriveState(
                aCritical
                ? CMassStorageDrive::EActive
                : CMassStorageDrive::EIdle );

            err = KErrNone;
            }
        }

    iCritSec.Signal();

    return err;
    }

/**
Set the mount state
*/
TInt CMassStorageDrive::SetMountConnected(CProxyDrive& aProxyDrive, TBool& aMediaChanged)
    {
    CLocalDriveRef* localDrive = NULL;

    OstTrace0(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_140,
              "SetMountConnected entering critical section");
    iCritSec.Wait(); // note: signalled in SetMountState

    if (iLocalDrive == NULL)
        {
        localDrive = new CLocalDriveRef(aProxyDrive, aMediaChanged);
        if (localDrive==NULL)
            {
            iCritSec.Signal();
            return KErrNoMemory;
            }
        }

    return SetMountState(EConnected, localDrive);
    }

/**
@return KErrNone
@param aNewState
@param aLocalDrive Only provide this if aNewState is EConnected.
*/
TInt CMassStorageDrive::SetMountState(TMountState aNewState, CLocalDriveRef* aLocalDrive/*=NULL*/)
    {
    if(iMountState == aNewState)
        {
        OstTrace0(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_150,
                  "SetMountState: No change");
        }
    else
        {
        // If called from SetMountConnected, already in critical section,
        // otherwise, must enter it here.
        if(EConnected!=aNewState)
            {
            OstTrace0(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_151,
                      "SetMountState entering critical section");
            iCritSec.Wait();
            }

        switch(aNewState)
            {
            case EConnected:
                if(aLocalDrive)
                    {
                    __ASSERT_DEBUG(iLocalDrive, User::Panic(KUsbMsSvrPncCat, EMsCMassStorageDriveSetMountState_iLocalDrive));
                    iLocalDrive = aLocalDrive;
                    }
                __ASSERT_DEBUG(iLocalDrive, User::Panic(KUsbMsSvrPncCat, EMsCMassStorageDriveSetMountState_aLocalDrive));
                break;

            case EDisconnected:
                delete iLocalDrive;
                iLocalDrive = NULL;
#ifndef USB_TRANSFER_PUBLISHER
                iBytesWritten = iBytesRead = 0;
#endif
                break;

            case EDisconnecting:
            case EConnecting:
                // Do not change iLocalDrive for these state changes
                break;
            }

        iMountState = aNewState;
        OstTrace1(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_152,
                  "SetMountState: state=%d", iMountState);

        iDriveStateChangedPublisher.DriveStateChanged();

        iCritSec.Signal();
        OstTrace0(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_153,
                  "SetMountState has left the critical section");
        }

    return KErrNone;
    }

/**
@return Current drive media state
*/
CMassStorageDrive::TDriveState CMassStorageDrive::DriveState() const
    {
    return iLocalDrive ? iLocalDrive->iDriveState : EErrDisMounted;
    }

/**
Check for media not present, and return the drive state.
@return Current drive media state
*/
CMassStorageDrive::TDriveState CMassStorageDrive::CheckDriveState()
    {
    CMassStorageDrive::TDriveState state = EErrDisMounted;

    iCritSec.Wait();

    if (iLocalDrive)
        {
        TInt err = KErrGeneral;
        TLocalDriveCapsV4 caps;

        FOREVER
            {
            // Initialise in case Caps() fails
            caps.iType = ::EMediaNotPresent;

            err = DoCaps(caps);
            OstTrace1(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_160,
                      "DoCaps err=%d", err);
            if (err == KErrNotReady || (err == KErrNone && caps.iType == ::EMediaNotPresent))
                {
                OstTrace0(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_161,
                          "Detected MediaNotPresent");

                SetDriveState(CMassStorageDrive::EMediaNotPresent);

                if (HandleCriticalError() == KErrAbort)
                    break;
                }
            else
                {
                ClearCriticalError();
                break;
                }
            }

        if (err == KErrNone && caps.iType != ::EMediaNotPresent)
            {
            if (iLocalDrive->iDriveState == CMassStorageDrive::EMediaNotPresent)
                {
                OstTrace0(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_162,
                          "Detected media inserted");
                SetDriveState(CMassStorageDrive::EIdle);
                }
            else if (iLocalDrive->iDriveState == CMassStorageDrive::ELocked &&
                     !(caps.iMediaAtt & KMediaAttLocked))
                {
                OstTrace0(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_163,
                          "Detected media unlocked");
                SetDriveState(CMassStorageDrive::EIdle);
                }
            else if (caps.iMediaAtt & KMediaAttLocked)
                {
                OstTrace0(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_164,
                          "Detected media locked");
                SetDriveState(CMassStorageDrive::ELocked);
                }

            iWholeMediaAccess = !(caps.iDriveAtt & KDriveAttLogicallyRemovable);
            }

        // Get the current state
        state = iLocalDrive->iDriveState;
        }

    iCritSec.Signal();

    return state;
    }

static TBool IsActive(CMassStorageDrive::TDriveState aDriveState)
    {
    return aDriveState==CMassStorageDrive::EActive;
    }

/**
@param aNewState
*/
void CMassStorageDrive::SetDriveState(TDriveState aNewState)
    {
    __ASSERT_DEBUG(aNewState == EIdle ||
                   (iMountState == EConnected && NULL != iLocalDrive) ||
                   (iMountState == EDisconnecting && NULL != iLocalDrive),
                   User::Panic(KUsbMsSvrPncCat, EMsCMassStorageDriveSetDriveState_State));

    if(!iLocalDrive)
        {
        OstTrace0(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_170,
                  "Drive not mounted.");
        }
    else
        {
        OstTraceExt2(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_171,
                     "Drive state change %d->%d", iLocalDrive->iDriveState, aNewState);

        if(iLocalDrive->iDriveState != aNewState)
            {
            CMountCB* mount = SafeProxyDrive().Mount();
            __ASSERT_DEBUG(mount != NULL, User::Panic(KUsbMsSvrPncCat, EMsCMassStorageDriveSetDriveState_Mount));

            if(mount)
                {
                if(!IsActive(iLocalDrive->iDriveState) && IsActive(aNewState))
                    {
                    mount->IncLock();
                    }
                else if(IsActive(iLocalDrive->iDriveState) && !IsActive(aNewState))
                    {
                    mount->DecLock();
                    }
                OstTrace1(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_172,
                          "LockStatus=%d", mount->LockStatus());
                }

            iLocalDrive->iDriveState = aNewState;

            iDriveStateChangedPublisher.DriveStateChanged();
            }
        }
    }

/**
Accessor for iProxyDrive; asserts if NULL
*/
CProxyDrive& CMassStorageDrive::SafeProxyDrive() const
    {
    __ASSERT_ALWAYS(NULL!=iLocalDrive, User::Invariant());
    return iLocalDrive->iProxyDrive;
    }

/////////////////////////////////////////////////////////////////

/**
Construct a CDriveManager object.
@param aDriveMap Reference to array mapping lun to drive number for supported
       mass storage drives.
*/
CDriveManager* CDriveManager::NewL(TRefDriveMap aDriveMap)
    {
    OstTrace1(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_180,
              "%d drives available", aDriveMap.Count());

    CDriveManager* self = new (ELeave) CDriveManager(aDriveMap);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }

CDriveManager::CDriveManager(const RArray<TInt>& aDriveMap)
    : iDriveMap(aDriveMap)
    {}

/**
Construct a CDriveManager object.
*/
void CDriveManager::ConstructL()
    {
    User::LeaveIfError(iDriveCritSec.CreateLocal());

    iDriveStateChangedPublisher = new (ELeave) RDriveStateChangedPublisher(iDrives, iDriveMap);

    for(TInt i = 0; i < iDriveMap.Count(); i++)
        {
        iDrives[i] = new (ELeave) CMassStorageDrive(iDriveCritSec, *iDriveStateChangedPublisher);
        }

    // Publish initial drive state
    if (iDriveMap.Count() > 0)
        {
        iDriveStateChangedPublisher->DriveStateChanged();
        }
    }

/**
Destructor
*/
CDriveManager::~CDriveManager()
    {
    iDrives.DeleteAll();
    delete iDriveStateChangedPublisher;
    iDriveCritSec.Close();
    }

/**
Set the mount state to Connected and specify the Proxy Drive.
@return KErrNone on success, otherwise system wide error code
@param aDrive The mounted Proxy Drive
@param aLun The Logical Drive Unit identifier (0..numDrives-1)
@pre If the Mount State is Connected, then aDrive must be the
     same as it was the last time this function was called.
@post The Mount State will be Connected.
*/
TInt CDriveManager::RegisterDrive(CProxyDrive& aProxyDrive, TBool& aMediaChanged, TUint aLun)
    {
    OstTrace1(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_190, "Lun=%d", aLun);
    TInt err = KErrUnknown; // never return this
    CMassStorageDrive* drive = CDriveManager::Drive(aLun, err);
    if(drive)
        {
        drive->SetMountConnected(aProxyDrive, aMediaChanged);
        }

    OstTrace1(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_191, "err=%d", err);
    return err;
    }

/**
Set the mount state to Disconnected.
@return KErrNone on success, otherwise system wide error code
@param aLun The Logical Drive Unit identifier (0..numDrives-1)
@post The Mount State will be Disconnected.
*/
TInt CDriveManager::DeregisterDrive(TUint aLun)
    {
    TInt err = KErrUnknown; // never return this
    if(CMassStorageDrive* drive = Drive(aLun, err))
        {
        err = drive->SetMountDisconnected();
        }

    OstTrace1(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_200, "err=%d", err);
    return err;
    }

/**
Return a pointer to the drive specified aLun, or NULL if
aLun is invalid.

@return Pointer to the specified drive, or NULL.
@param aLun The Logical Drive Unit identifier (0..numDrives-1)
@param aError KErrNone on success, KErrArgument if NULL is returned.
*/
CMassStorageDrive* CDriveManager::Drive(TUint aLun, TInt& aError) const
    {
    aError = KErrNone;
    CMassStorageDrive* drive = NULL;

    // Check if aLun exceeds the specified number of drives
    // (This will panic if it exceeds KMaxLun).
    if(aLun>=KUsbMsMaxDrives || !iDrives[aLun])
        {
        aError = KErrArgument;
        }
    else
        {
        drive = iDrives[aLun];
        }
    return drive;
    }

/**
Checks the Media Changed flag, and optionally resets it.
@return The state of the Media Changed flag.
@param aLun The Logical Drive Unit identifier (0..numDrives-1)
@param aReset If true, the Media Changed flag is reset to EFalse.
*/
TBool CDriveManager::IsMediaChanged(TUint aLun, TBool aReset)
    {
    TInt err; // not used, but is a required parameter
    CMassStorageDrive* drive = Drive(aLun, err);

    if(!drive)
        {
        OstTrace1(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_210,
                  "LUN=%d not found, returning false", aLun);
        return ETrue;
        }
    else
        {
        return drive->IsMediaChanged(aReset);
        }
    }

/**
Set the Drive State to Active or Idle.
Ref: 3.6.3.2 - PREVENT_MEDIUM_REMOVAL
@return KErrNone on success, otherwise system wide error code
@param aLun The Logical Drive Unit identifier (0..numDrives-1)
@param aCritical ETrue for Active, EFalse for Idle
*/
TInt CDriveManager::SetCritical(TUint aLun, TBool aCritical)
    {
    TInt err = KErrUnknown; // never return this

    TInt i=aLun;
    TInt cnt=aLun+1;

    if (aLun == KAllLuns)
        {
        i=0;
        cnt= iDriveMap.Count();
        }

    for(; i<cnt; i++)
        {

        CMassStorageDrive* drive = Drive(i, err);
        if(drive)
            {
            err = drive->SetCritical(aCritical);
            }
        }
    return err;
    }

/**
Inititiate transition to Connected.
@return KErrNone on success, otherwise system wide error code
@param aLun The Logical Drive Unit identifier (0..numDrives-1)
@post The Mount State will be Connected or Connecting.
*/
TInt CDriveManager::Connect(TUint aLun)
    {
    TInt err = KErrUnknown; // never return this
    CMassStorageDrive* drive = Drive(aLun, err);

    OstTraceExt3(TRACE_SMASSSTORAGE_DRIVESTATE, DRIVERMANAGER_230,
                 "lun=%d err=%d mountState=%d", aLun, err, drive->MountState());

    if(drive)
        {
        switch(drive->MountState())
            {
            case CMassStorageDrive::EDisconnected:
                err = drive->SetMountConnecting();
                break;
            case CMassStorageDrive::EDisconnecting:
                err = drive->SetMountConnected();
                break;
            case CMassStorageDrive::EConnected:
            case CMassStorageDrive::EConnecting:
                // do nothing
                break;
            }
        }
    return err;
    }

/**
Inititiate transition to Disconnected.
@return KErrNone on success, otherwise system wide error code
@param aLun The Logical Drive Unit identifier (0..numDrives-1)
@post The Mount State will be Disconnected or Disconnecting.
*/
TInt CDriveManager::Disconnect(TUint aLun)
    {
    TInt err = KErrUnknown; // never return this
    CMassStorageDrive* drive = Drive(aLun, err);

    if(drive)
        {
        switch(drive->MountState())
            {
            case CMassStorageDrive::EConnected:
                err = drive->SetMountDisconnecting();
                break;
            case CMassStorageDrive::EConnecting:
                err = drive->SetMountDisconnected();
                break;
            case CMassStorageDrive::EDisconnected:
            case CMassStorageDrive::EDisconnecting:
                // do nothing
                break;
            }
        }
    return err;
    }


