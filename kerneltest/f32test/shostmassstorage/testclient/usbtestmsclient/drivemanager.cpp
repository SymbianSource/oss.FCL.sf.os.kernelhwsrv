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
// Class implementation of CDriveManager and CMassStorageDrive.
// 
//



/**
 @file
 @internalTechnology
*/

#include <f32fsys.h>
#include <e32property.h>


#include "usbmsshared.h"
#include "msctypes.h"

#include "drivepublisher.h"
#include "drivemanager.h"
#include "debug.h"
#include "msdebug.h"


void TMediaParams::Init(TLocalDriveCapsV4& aCaps)
    {
    iSize = aCaps.MediaSizeInBytes();
    TInt64 driveBlocks =  iSize / MAKE_TINT64(0, KDefaultBlockSize);
    iNumBlocks = I64LOW(driveBlocks);
    iMediaAtt = aCaps.iMediaAtt;
    }


void TLocalDriveRef::SetDriveState(TDriveState aState)
    {
    if (iDriveState != aState)
        {
        CMountCB* mount = iProxyDrive.Mount();
        __ASSERT_DEBUG(mount != NULL, User::Invariant());
        if (mount)
            {
            if (!IsActive(iDriveState) && IsActive(aState))
                {
                mount->IncLock();
                }
            else if (IsActive(iDriveState) && !IsActive(aState))
                {
                mount->DecLock();
                }
            __PRINT1(_L("SetDriveState: LockStatus=%d\n"), mount->LockStatus());
            }

        iDriveState = aState;
        iDriveStateChangedPublisher.DriveStateChanged();
        }
    }


TInt TLocalDriveRef::Read(const TInt64& aPos, TInt aLength, TDes8& aBuf, TBool aWholeMedia)
	{
    __MSFNLOG

	TInt err = KErrUnknown; // never return this

	if(aWholeMedia)
		{
		err = iProxyDrive.Read(aPos, aLength, &aBuf, KLocalMessageHandle, 0, RLocalDrive::ELocDrvWholeMedia);
		}
	else
		{
		err = iProxyDrive.Read(aPos, aLength, aBuf);
		}

	if (err == KErrLocked)
		{
		SetDriveState(TLocalDriveRef::ELocked);
		}

	return err;
	}


TInt TLocalDriveRef::Write(const TInt64& aPos, TDesC8& aBuf, TBool aWholeMedia)
    {
	TInt err = KErrNone;

	TDriveState oldState = iDriveState;
	if (oldState != EActive)
        {
		// SCSI hasn't called SetCritical
		SetDriveState(EActive);
		}

	if (aWholeMedia)
		{
		err = iProxyDrive.Write(aPos, aBuf.Length(), &aBuf, KLocalMessageHandle, 0, RLocalDrive::ELocDrvWholeMedia);
		}
	else
		{
		err = iProxyDrive.Write(aPos,aBuf);
		}

	if (err == KErrLocked)
		{
		SetDriveState(ELocked);
		}
	else if (oldState != EActive)
		{
		SetDriveState(oldState);
		}
    return err;
    }


/**
Checks the Media Changed flag, and optionally resets it.
@return The state of the Media Changed flag.
@param aReset If true, the Media Changed flag is reset to EFalse.
*/
TBool TLocalDriveRef::IsMediaChanged(TBool aReset)
	{
    __MSFNLOG
	TBool mediaChanged = iMediaChanged;
	if (aReset)
        {
	   	iMediaChanged = EFalse;
	   	}
	return mediaChanged;
	}


/**
Set the Drive State to Active or Idle.
@return KErrNone on success, KErrNotReady if media not present, KErrDisMounted if not mounted
@param aCritical ETrue for Active, EFalse for Idle
*/
TInt TLocalDriveRef::SetCritical(TBool aCritical)
	{
    __MSFNLOG
	TInt err = KErrNone;
    if (iDriveState == EMediaNotPresent)
		{
		err = KErrNotReady;
		}
	else
		{
        SetDriveState(aCritical ? EActive : EIdle);
		}
	return err;
	}


/**
Provides an interface to CProxyDrive::Caps that hides the
package buffer.
@return KErrNone on success, otherwise system wide error code
@param aInfo
*/
TInt TLocalDriveRef::Caps(TLocalDriveCapsV4& aInfo)
	{
    __MSFNLOG
	TLocalDriveCapsV4Buf buf;
	buf.FillZ();

	__PRINT(_L("CMassStorageDrive::DoCaps calling Caps\n"));
	TInt err = iProxyDrive.Caps(buf);
	__PRINT1(_L("CMassStorageDrive::DoCaps: Caps returned %d\n"), err);

	if (err == KErrNone)
		{
		// Invoke function call operator to cast to TLocalDriveCapsV4&
		aInfo = buf();
		}

	return err;
	}


///////////////////////////////////////////////////////////////////////////////

/**
@param aCritSec A Critical Section object shared by all drives.
@param aDrives Reference to the list of CMassStorageDrive objects.
@param aDriveMap Reference to array mapping lun to drive number for supported
	   mass storage drives.
@post Object is fully constructed
 */
CMassStorageDrive* CMassStorageDrive::NewL(RCriticalSection& aCritSec,
                                           RDriveStateChangedPublisher& aDriveStateChangedPublisher)
    {
    __MSFNSLOG
	CMassStorageDrive* self = new (ELeave) CMassStorageDrive(aCritSec, aDriveStateChangedPublisher);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
    }


CMassStorageDrive::CMassStorageDrive(RCriticalSection& aCritSec,
									 RDriveStateChangedPublisher& aDriveStateChangedPublisher)
:   iCritSec(aCritSec),
	iMountState(EDisconnected),
	iDriveStateChangedPublisher(aDriveStateChangedPublisher)
	{
    __MSFNLOG
	}


void CMassStorageDrive::ConstructL()
    {
    __MSFNLOG
    iDriveMediaErrorPublisher = new (ELeave) RDriveMediaErrorPublisher();
    }


CMassStorageDrive::~CMassStorageDrive()
	{
    __MSFNLOG
    delete iDriveMediaErrorPublisher;
	delete iLocalDrive;
	}

/**
Read from the target drive unit.
@return KErrNone on success, otherwise system wide error code
*/
TInt CMassStorageDrive::Read(const TInt64& aPos, TInt aLength, TDes8& aBuf, TBool aWholeMedia)
	{
    __MSFNLOG

	TInt err = KErrUnknown; // never return this
	iCritSec.Wait();

	if(iMountState != EConnected)
		{
		err = KErrDisconnected;
		}
	else
		{
        err = iLocalDrive->Read(aPos, aLength, aBuf, aWholeMedia);
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
    __MSFNLOG

	TInt err = KErrNone;
	iCritSec.Wait();

	if (iMountState != EConnected)
		{
		err = KErrDisconnected;
		}
	else
		{
		__ASSERT_DEBUG(iLocalDrive, User::Invariant());
        err = iLocalDrive->Write(aPos, aBuf, aWholeMedia);
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
    __MSFNLOG
    TInt err = KErrDisMounted;

    if (iLocalDrive)
        {
        err = iLocalDrive->Caps(aInfo);
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
    __MSFNLOG
	TRAPD(err, iDriveMediaErrorPublisher->PublishErrorL(ETrue));
    // ignore leave
    err = err;
	return KErrAbort;
	}


TInt CMassStorageDrive::ClearCriticalError()
	{
    __MSFNLOG
	TRAPD(err, iDriveMediaErrorPublisher->PublishErrorL(EFalse));
    // ignore leave
    err = err;
	return KErrNone;
	}


/**
Checks the Media Changed flag, and optionally resets it.
@return The state of the Media Changed flag.
@param aReset If true, the Media Changed flag is reset to EFalse.
*/
TBool CMassStorageDrive::IsMediaChanged(TBool aReset)
	{
    __MSFNLOG

	iCritSec.Wait();

	TBool mediaChanged = EFalse;
	if (iLocalDrive)
		{
		mediaChanged = iLocalDrive->IsMediaChanged(aReset);
        }

	iCritSec.Signal();

	__PRINT1(_L("CMassStorageDrive::IsMediaChanged: returning %d\n"), mediaChanged);
	return mediaChanged;
	}

/**
Set the Drive State to Active or Idle.
@return KErrNone on success, KErrNotReady if media not present, KErrDisMounted if not mounted
@param aCritical ETrue for Active, EFalse for Idle
*/
TInt CMassStorageDrive::SetCritical(TBool aCritical)
	{
    __MSFNLOG

	TInt err = KErrDisMounted;
	iCritSec.Wait();
	if (iLocalDrive)
		{
        err = iLocalDrive->SetCritical(aCritical);
		}

	iCritSec.Signal();
	return err;
	}

/**
Set the mount state
*/
void CMassStorageDrive::SetMountConnectedL(CProxyDrive& aProxyDrive,
                                           TBool& aMediaChanged,
                                           RDriveStateChangedPublisher& aDriveStateChangedPublisher)
	{
    __MSFNLOG
	TLocalDriveRef* localDrive = NULL;

	__PRINT(_L("SetMountConnectedL entering critical section\n"));
	iCritSec.Wait(); // note: signalled in SetMountState

   	TRAPD(err, localDrive = new (ELeave) TLocalDriveRef(aProxyDrive,
                                                        aMediaChanged,
                                                        aDriveStateChangedPublisher));
   	if (err)
   		{
   		iCritSec.Signal();
   		User::Leave(err);
   		}
	iLocalDrive = localDrive;
	SetMountState(EConnected, ETrue);
	}

/**
@return KErrNone
@param aNewState
@param aLocalDrive Only provide this if aNewState is EConnected.
*/
void CMassStorageDrive::SetMountState(TMountState aNewState, TBool aCriticalSection/*=EFalse*/)
	{
    __MSFNLOG
	if(iMountState == aNewState)
		{
		__PRINT(_L("SetMountState: No change\n"));
		}
	else
		{
		// If called from SetMountConnected, already in critical section,
        // otherwise, must enter it here.
        if (!aCriticalSection)
            {
			iCritSec.Wait();
            }

		switch(aNewState)
			{
			case EDisconnected:
				delete iLocalDrive;
				iLocalDrive = NULL;
				break;

			case EConnected:
			case EDisconnecting:
			case EConnecting:
				// Do not change iLocalDrive for these state changes
				break;
			}

		iMountState = aNewState;
		__PRINT1(_L("SetMountState: state=%d\n"), iMountState);

		iDriveStateChangedPublisher.DriveStateChanged();
		iCritSec.Signal();
		__PRINT(_L("SetMountState has left the critical section\n"));
		}
	}

/**
@return Current drive media state
*/
TLocalDriveRef::TDriveState CMassStorageDrive::DriveState() const
	{
    __MSFNSLOG
	return iLocalDrive ? iLocalDrive->DriveState() : TLocalDriveRef::EErrDisMounted;
	}

/**
Check for media not present, and return the drive state.
@return Current drive media state
*/
TLocalDriveRef::TDriveState CMassStorageDrive::CheckDriveState()
	{
    __MSFNLOG
	TLocalDriveRef::TDriveState state = TLocalDriveRef::EErrDisMounted;
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

			__PRINTERR(_L("CheckDriveState: DoCaps err=%d\n"), err);
			if (err == KErrNotReady || (err == KErrNone && caps.iType == ::EMediaNotPresent))
				{
				__PRINT(_L("CheckDriveState: detected MediaNotPresent\n"));

				SetDriveState(TLocalDriveRef::EMediaNotPresent);

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
            iMediaParams.Init(caps);
            TLocalDriveRef::TDriveState driveState = TLocalDriveRef::EIdle;

			if (iLocalDrive->DriveState() == TLocalDriveRef::EMediaNotPresent)
				{
				__PRINT(_L("CheckDriveState: detected media inserted\n"));
				}
			else if (iLocalDrive->DriveState() == TLocalDriveRef::ELocked &&
					 !(caps.iMediaAtt & KMediaAttLocked))
				{
				__PRINT(_L("CheckDriveState: detected media unlocked\n"));
				}
			else if (caps.iMediaAtt & KMediaAttLocked)
				{
				__PRINT(_L("CheckDriveState: detected media locked\n"));
				driveState = TLocalDriveRef::ELocked;
				}
            SetDriveState(driveState);
			}

		// Get the current state
		state = iLocalDrive->DriveState();
		}

	iCritSec.Signal();

	return state;
	}


/**
@param aNewState
*/
void CMassStorageDrive::SetDriveState(TLocalDriveRef::TDriveState aNewState)
	{
    __MSFNLOG
	__ASSERT_DEBUG(aNewState == TLocalDriveRef::EIdle ||
                   (iMountState == EConnected && NULL != iLocalDrive) ||
                   (iMountState == EDisconnecting && NULL != iLocalDrive),
        User::Invariant());

	if (!iLocalDrive)
		{
		__PRINT(_L("SetDriveState: Drive not mounted.\n"));
		}
	else
		{
        iLocalDrive->SetDriveState(aNewState);
		__PRINT2(_L("SetDriveState: %d->%d\n"), iLocalDrive->iDriveState, aNewState);
		}
	}


/////////////////////////////////////////////////////////////////

/**
Construct a CDriveManager object.
@param aDriveMap Reference to array mapping lun to drive number for supported
	   mass storage drives.
*/
CDriveManager* CDriveManager::NewL(const TLunToDriveMap& aDriveMap)
	{
    __MSFNSLOG
	__PRINT1(_L("CDriveManager::NewL - %d drives\n"), aDriveMap.Count());

	CDriveManager* self = new (ELeave) CDriveManager(aDriveMap.Count() -1);
	CleanupStack::PushL(self);
	self->ConstructL(aDriveMap);
	CleanupStack::Pop();
	return self;
	}

CDriveManager::CDriveManager(TLun aMaxLun)
:   iMaxLun(aMaxLun)
	{
    __MSFNLOG
    }

/**
Construct a CDriveManager object.
*/
void CDriveManager::ConstructL(const TLunToDriveMap& aDriveMap)
	{
    __MSFNLOG
	User::LeaveIfError(iDriveCritSec.CreateLocal());

    iDriveStateChangedPublisher = new (ELeave) RDriveStateChangedPublisher(iDrives, aDriveMap);

    iDrives.Reserve(iMaxLun + 1);

	for (TLun lun = 0; lun < iMaxLun + 1; lun++)
		{
		iDrives.Append(CMassStorageDrive::NewL(iDriveCritSec,
                                               *iDriveStateChangedPublisher));
		}

	// Publish initial drive state
	if (iDrives.Count() > 0)
		{
		iDriveStateChangedPublisher->DriveStateChanged();
		}
	}

/**
Destructor
*/
CDriveManager::~CDriveManager()
	{
    __MSFNLOG
	iDrives.ResetAndDestroy();
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
void CDriveManager::RegisterDriveL(CProxyDrive& aProxyDrive, TBool& aMediaChanged, TLun aLun)
	{
    __MSFNLOG
	__PRINT1(_L("Lun=%d \n"),aLun);
	CMassStorageDrive* drive = Drive(aLun);
	drive->SetMountConnectedL(aProxyDrive, aMediaChanged, *iDriveStateChangedPublisher);
	}

/**
Set the mount state to Disconnected.
@return KErrNone on success, otherwise system wide error code
@param aLun The Logical Drive Unit identifier (0..numDrives-1)
@post The Mount State will be Disconnected.
*/
void CDriveManager::DeregisterDrive(TLun aLun)
	{
    __MSFNLOG
    CMassStorageDrive* drive = Drive(aLun);
	drive->SetMountDisconnected();
	}

/**
Return a pointer to the drive specified aLun, or NULL if aLun is invalid.

@return Pointer to the specified drive, or NULL.
@param aLun The Logical Drive Unit identifier (0..numDrives-1)
@param aError KErrNone on success, KErrArgument if NULL is returned.
*/
CMassStorageDrive* CDriveManager::Drive(TLun aLun) const
	{
    __MSFNSLOG
	__ASSERT_DEBUG(aLun < iDrives.Count(), User::Invariant());
    return iDrives[aLun];
	}

/**
Checks the Media Changed flag, and optionally resets it.
@return The state of the Media Changed flag.
@param aLun The Logical Drive Unit identifier (0..numDrives-1)
@param aReset If true, the Media Changed flag is reset to EFalse.
*/
TBool CDriveManager::IsMediaChanged(TLun aLun, TBool aReset)
	{
    __MSFNLOG
	CMassStorageDrive* drive = Drive(aLun);
	return drive->IsMediaChanged(aReset);
	}

/**
Set the Drive State to Active or Idle.
Ref: 3.6.3.2 - PREVENT_MEDIUM_REMOVAL
@return KErrNone on success, otherwise system wide error code
@param aLun The Logical Drive Unit identifier (0..numDrives-1)
@param aCritical ETrue for Active, EFalse for Idle
*/
TInt CDriveManager::SetCritical(TLun aLun, TBool aCritical)
	{
    __MSFNLOG
	TInt err = KErrUnknown; // never return this

	TLun i = aLun;
	TLun cnt = aLun + 1;

	if (aLun == KAllLuns)
		{
		i = 0;
		cnt = iMaxLun + 1;
		}

	for(; i < cnt; i++)
		{
		CMassStorageDrive* drive = Drive(i);
		err = drive->SetCritical(aCritical);
		}
	return err;
	}

void CDriveManager::Connect()
	{
	__FNLOG("CDriveManager::Connect");
    TLun lun = iMaxLun;
    do
        {
        Connect(lun);
        }
    while(--lun >= 0);
    }

/**
Inititiate transition to Connected.
@return KErrNone on success, otherwise system wide error code
@param aLun The Logical Drive Unit identifier (0..numDrives-1)
@post The Mount State will be Connected or Connecting.
*/
void CDriveManager::Connect(TLun aLun)
	{
    __MSFNLOG
	CMassStorageDrive* drive = Drive(aLun);

	__PRINT2(_L("CDriveManager::Connect lun=%d, mountState=%d\n"), aLun, drive->MountState());

   	switch(drive->MountState())
   		{
 	case CMassStorageDrive::EDisconnected:
 		drive->SetMountConnecting();
 		break;
 	case CMassStorageDrive::EDisconnecting:
 		drive->SetMountConnected();
 		break;
 	case CMassStorageDrive::EConnected:
 	case CMassStorageDrive::EConnecting:
    default:
 		// do nothing
 		break;
		}
	}

void CDriveManager::Disconnect()
	{
	__FNLOG("CDriveManager::Disconnect");
    TLun lun = iMaxLun;
    do
        {
        Disconnect(lun);
        }
    while(--lun >= 0);
    }

/**
Inititiate transition to Disconnected.
@return KErrNone on success, otherwise system wide error code
@param aLun The Logical Drive Unit identifier (0..numDrives-1)
@post The Mount State will be Disconnected or Disconnecting.
*/
void CDriveManager::Disconnect(TLun aLun)
	{
    __MSFNLOG
	CMassStorageDrive* drive = Drive(aLun);
   	switch (drive->MountState())
   		{
   	case CMassStorageDrive::EConnected:
   		drive->SetMountDisconnecting();
   		break;
   	case CMassStorageDrive::EConnecting:
   		drive->SetMountDisconnected();
   		break;
   	case CMassStorageDrive::EDisconnected:
   	case CMassStorageDrive::EDisconnecting:
   		// do nothing
   		break;
   		}
	}
