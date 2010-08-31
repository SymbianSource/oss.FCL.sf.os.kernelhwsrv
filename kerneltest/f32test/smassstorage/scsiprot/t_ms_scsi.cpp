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
//

/**
 @file
 @internalTechnology
*/


#include <f32file.h>
#include <e32test.h>
#include <f32fsys.h>
#include <e32std.h>
#include <e32std_private.h>		// Caps.iMediaAtt

#include "massstoragedebug.h"

#include "t_ms_main.h"
#include "t_ms_scsi.h"
#include "drivepublisher.h"


//
//	TTestScsiTransport
//
/** c'tor */
TTestScsiTransport::TTestScsiTransport ()
: iProtocol(NULL),
  iBufWrite(NULL, 0)
	{
	}

/** InitialiseReadBuf */
void TTestScsiTransport::InitialiseReadBuf ()
	{
	// KMaxBufSize is defined in scsiprot.h
	iBufRead.FillZ(KMaxBufSize * 2);
	// set some values in 1/2 a buffer for testing
	for (TInt i = 0; i<(TInt)KMaxBufSize; i++)
		{
		iBufRead[i] = static_cast<TInt8>(i%0x08);
		}
	}

//
//	CTestMassStorageDrive
//
/**
@param aOwner A reference to the drive manager, for use with aDriveStateChanged.
@param aDriveStateChanged A pointer to the private callback function CDriveManager::DriveStateChanged.
@param aCritSec A Critical Section object shared by all drives.
@post Object is fully constructed
 */
CMassStorageDrive::CMassStorageDrive(
	RCriticalSection& aCritSec,
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

/** Read data from the drive */
TInt CMassStorageDrive::Read(const TInt64& aPos, TInt aLength, TDes8& aBuf, TBool aWholeMedia)
	{
	TInt err = KErrUnknown;
	
	if( iMountState != EConnected )
		{
		err = KErrDisconnected;
		}
	else
		{
		ASSERT(iLocalDrive);
		if(aWholeMedia)
			{
			err = iLocalDrive->iProxyDrive.Read(aPos, aLength, &aBuf, KLocalMessageHandle, 0, RLocalDrive::ELocDrvWholeMedia);
			}
		else
			{
			err = iLocalDrive->iProxyDrive.Read(aPos,aLength,aBuf);
			}
		}

	return err;
	}

/** Write data to the drive */
TInt CMassStorageDrive::Write(const TInt64& aPos, TDesC8& aBuf, TBool aWholeMedia)
	{
	TInt err = KErrUnknown;
	
	if( iMountState != EConnected )
		{
		err = KErrDisconnected;
		}
	else
		{
		ASSERT(iLocalDrive);
		if(aWholeMedia)
			{
			err = iLocalDrive->iProxyDrive.Write(aPos, aBuf.Length(), &aBuf, KLocalMessageHandle, 0, RLocalDrive::ELocDrvWholeMedia);
			}
		else
			{
			err = iLocalDrive->iProxyDrive.Write(aPos,aBuf);
			}
		}

	return err;
	}

/** Get CAPs */
TInt CMassStorageDrive::Caps(TLocalDriveCapsV4& aInfo)
	{
	TInt err = KErrUnknown;
	
	if( iMountState != EConnected )
		{
		err = KErrDisconnected;
		}
	else
		{
		err = DoCaps(aInfo);
		}
	
	return err;
	}

/** Get CAPs helper */
TInt CMassStorageDrive::DoCaps(TLocalDriveCapsV4& aInfo)
	{
	if (!iLocalDrive)
		{ return KErrBadHandle; }

	TLocalDriveCapsV4Buf buf;
	TInt err = iLocalDrive->iProxyDrive.Caps(buf);
	
	// Invoke function call operator to cast to TLocalDriveCapsV4&
	aInfo = buf();
	
	return err;
	}

/**
Set the mount state
*/
TInt CMassStorageDrive::SetMountConnected(CProxyDrive& aProxyDrive, TBool& aMediaChanged)
	{
		__FNLOG("CMassStorageDrive::SetMountConnected");
		TInt err = KErrUnknown; //Never return this
	
		if(iMountState == EConnected)
			{
			err = KErrNone;
			}
		else
			{
			CLocalDriveRef* localDrive = new CLocalDriveRef(aProxyDrive, aMediaChanged);
			err = (localDrive==NULL) 
				? KErrNoMemory
				: SetMountState(EConnected, localDrive);
			}

		return err;
	}

/**
@return KErrNone on success, KErrArgument if arguments are illegal
@param aNewState
@param aLocalDrive Only provide this if aNewState is EConnected, in which case it is required.
Only sets/clears iLocalDrive if new state is Connected or Disconnected.
*/
TInt CMassStorageDrive::SetMountState(TMountState aNewState, CLocalDriveRef* aLocalDrive/*=NULL*/)
	{
	__FNLOG("CMassStorageDrive::SetMountState");
	TInt err = KErrUnknown; //Never return this
	
	if(iMountState != aNewState)
		{

		if 	(iMountState == EConnected 		&& aNewState==EConnecting ||
			iMountState == EDisconnected 	&& aNewState==EDisconnecting)
			{
			return KErrNone;
			}
		
		iMountState = aNewState;
		if(aNewState==EDisconnected || aNewState==EConnecting)
			{
			// Reset the drive state on disconnection.
			// Note: This should be called before ProxyDrive is NULLed.
			SetDriveState(EErrDisMounted);
			}

		// Only mounting and unmounting transitions affect iProxyDrive
		if(aNewState==EConnected || aNewState==EDisconnected)
			{
			delete iLocalDrive;
			iLocalDrive = aLocalDrive;  // possibly NULL
			}

#ifndef USB_TRANSFER_PUBLISHER
		// The data transferred counts are "since the host connected to the drive"
		// so reset them when going to the connected state.
		if(aNewState==EConnected)
			{			
			iBytesWritten = iBytesRead = 0;
			}
#endif			
		
		__PRINT1(_L("SetMountState: state=%d\n"), iMountState);

		err = KErrNone;
		}
	else if(aLocalDrive != iLocalDrive)
		{
		// Caller is not allowed to change the proxy drive
		err = KErrArgument;
		}
		
	return err;
	}

/**
Checks the Media Removed flag, and optionally resets it.
The media has been changed (reinserted) if this function returns true
and the Drive State is not EMediaNotPresent.
@return The state of the Media Removed flag.
@param aReset If true, the Media Removed flag is reset to EFalse.
*/
TBool CMassStorageDrive::IsMediaChanged(TBool aReset)
	{
	__FNLOG("CMassStorageDrive::IsMediaChanged");

	TBool mediaChanged = EFalse;
	if(iLocalDrive)
		{
		mediaChanged = iLocalDrive->iMediaChanged;
		if(aReset) 
			{
			iLocalDrive->iMediaChanged = EFalse;
			}
		}
	return mediaChanged;
	}

/**
Set the Drive State to Active or Idle.
@return KErrNone on success, KErrNotReady if media not present
@param aCritical ETrue for Active, EFalse for Idle
*/
TInt CMassStorageDrive::SetCritical(TBool aCritical)
	{
	__FNLOG("CMassStorageDrive::SetCritical");

	TInt err = KErrDisMounted;
	if(iLocalDrive)
		{
		if(iLocalDrive->iDriveState == CMassStorageDrive::EMediaNotPresent)
			{
			err = KErrNotReady;
			}
		else
			{
			iCritSec.Wait();
			
			SetDriveState(
				aCritical 
				? CMassStorageDrive::EActive
				: CMassStorageDrive::EIdle );
				
			iCritSec.Signal();
			err = KErrNone;
			}
		}
	return err;
	}

/**
@return Current drive media state
*/
CMassStorageDrive::TDriveState	CMassStorageDrive::DriveState() const
	{
	return iLocalDrive ? iLocalDrive->iDriveState : EErrDisMounted;
	}

/**
This test version does not check for media removal
*/
CMassStorageDrive::TDriveState	CMassStorageDrive::CheckDriveState()
	{
	return iLocalDrive ? iLocalDrive->iDriveState : EErrDisMounted;
	}
	
/**
@param aNewState
*/
void CMassStorageDrive::SetDriveState(TDriveState aNewState)
	{
	if(iLocalDrive)
		{
		__PRINT2(_L("SetDriveState: %d->%d\n"), iLocalDrive->iDriveState, aNewState);
		iLocalDrive->iDriveState = aNewState;
		}
	}




/////////////////////////////////////////////////////////////////
//
//	CTestDriveManager
//
/** NewL */
CDriveManager* CDriveManager::NewL(const RArray<TInt>& aDriveMap)
	{
	__FNLOG ("CDriveManager::NewL");
	CDriveManager* self = new (ELeave) CDriveManager(aDriveMap);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}

/** construct
*/
void CDriveManager::ConstructL()
	{
	__FNLOG("CDriveManager::ConstructL");
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

CDriveManager::CDriveManager(const RArray<TInt>& aDriveMap)
	:
	iDriveMap(aDriveMap)
	{}

/** d'tor */
CDriveManager::~CDriveManager()
	{
	iDrives.DeleteAll();
	delete iDriveStateChangedPublisher;
	iDriveCritSec.Close();
	}

/** register drive
@param aDrive - a proxy drive to register
@param aLun - Lun to register to
@return system wide return code
*/
TInt CDriveManager::RegisterDrive(CProxyDrive&, TBool&, TUint)
	{
	// not used in test code
	ASSERT(EFalse);
	return KErrNone;
	}
	
/** register drive
@param aDrive - a proxy drive to register
@param aLun - Lun to register to
@return system wide return code
*/
TInt CDriveManager::DeregisterDrive(TUint)
	{
	// not used in test code
	ASSERT(EFalse);
	return KErrNone;
	}

/** get a drive by aLun
@param aLun - lun
@param aError - operation error
@return pointer to drive
*/
CMassStorageDrive* CDriveManager::Drive(TUint aLun, TInt& aError) const
	{
	aError = KErrNone;
	CMassStorageDrive* drive = NULL;

	if(aLun>=static_cast<TUint>(iDrives.Count()))
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
@return true if media is changed
@param aLun - lun
@param aReset - reset flag
*/
TBool CDriveManager::IsMediaChanged(TUint, TBool)
	{
	// not used in test code
	ASSERT(EFalse);
	return EFalse;
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
	__PRINT1(_L("CDriveManager::SetCritical lun=%d\n"), aLun);
	
	TInt err = KErrUnknown; // never return this
	CMassStorageDrive* drive = Drive(aLun, err);
	if(drive)
		{
		err = drive->SetCritical(aCritical);
		}
		
	return err;
	}

/** connect a drive according to lun
@param aLun - lun
@return system wide error code
*/
TInt CDriveManager::Connect(TUint aLun)
	{
	TInt err = KErrNone;
	CMassStorageDrive* drive = Drive(aLun, err);
	
	if(drive)
		{
		drive->SetMountConnecting();
		}

	return err;
	}

/** disconnect drive according to lun
@param aLun - lun
@return system wide error code
*/
TInt CDriveManager::Disconnect(TUint aLun)
	{
	TInt err = KErrNone;
	CMassStorageDrive* drive = Drive(aLun, err);
	
	if(drive)
		{
		drive->SetMountDisconnecting();
		}

	return err;
	}


// 
// Dummy publishers
//	
#ifdef USB_TRANSFER_PUBLISHER
CUsbWriteTransferPublisher* CUsbWriteTransferPublisher::NewL(
	TRefBytesTransferedList /* aDrives */)
	{
	return NULL;
	}


CUsbReadTransferPublisher* CUsbReadTransferPublisher::NewL(
	TRefBytesTransferedList /* aDrives */)
	{
	return NULL;
	}


void CUsbTransferPublisher::DoPublishDataTransferredEvent()
	{
	}


CUsbTransferPublisher::~CUsbTransferPublisher()
	{
	}


void CUsbTransferPublisher::StartTimer()
	{
	}


#else
CDriveWriteTransferPublisher* CDriveWriteTransferPublisher::NewL(
	TFixedArray<CMassStorageDrive*,KUsbMsMaxDrives>& /* aDrives */)
	{
	return NULL;
	}


CDriveReadTransferPublisher* CDriveReadTransferPublisher::NewL(
	TFixedArray<CMassStorageDrive*,KUsbMsMaxDrives>& /* aDrives */)
	{
	return NULL;
	}


void CDriveTransferPublisher::DoPublishDataTransferredEvent()
	{
	}


CDriveTransferPublisher::~CDriveTransferPublisher()
	{
	}


void CDriveTransferPublisher::StartTimer()
	{
	}
#endif


RDriveStateChangedPublisher::RDriveStateChangedPublisher(
	TFixedArray<CMassStorageDrive*,KUsbMsMaxDrives>& aDrives,
	const RArray<TInt>& aDriveMap)
	:
	iDrives(aDrives),
	iDriveMap(aDriveMap)
	{
	}

	
void RDriveStateChangedPublisher::DriveStateChanged()
	{
	}

	
RDriveMediaErrorPublisher::RDriveMediaErrorPublisher()
	{
	}

	
RDriveStateChangedPublisher::~RDriveStateChangedPublisher()
	{
	}

	
RDriveMediaErrorPublisher::~RDriveMediaErrorPublisher()
	{
	}


//
// Proxy Drive
//
/** c'tor */
CProxyDrive::CProxyDrive(CMountCB* aMount)
: iMount(aMount)
	{}

EXPORT_C TInt CProxyDrive::ControlIO(const RMessagePtr2&, TInt, TAny*, TAny*)
	{ return KErrNone; }

EXPORT_C TInt CProxyDrive::Read (TInt64 aPos, TInt aLength, const TAny* aTrg, TInt, TInt, TInt)
	{ return Read(aPos, aLength, *(TDes8*)aTrg); }

EXPORT_C TInt CProxyDrive::Write (TInt64 aPos, TInt, const TAny* aSrc, TInt, TInt, TInt)
	{ return Write(aPos, *(TDesC8*)aSrc); }

// Implemented the GetInterface method here as this are usually 
// exported by EFILE, but these unit tests don't link to it.

EXPORT_C TInt CProxyDrive::GetInterface(TInt /*aInterfaceId*/, TAny*& /*aInterface*/, TAny* /*aInput*/)
	{ return KErrNotSupported; }

// Implemented the GetLastErrorInfo method here as this is usually 
// exported by EFILE, but these unit tests don't link to it.
EXPORT_C TInt CProxyDrive::GetLastErrorInfo(TDes8& /*anErrorInfo*/)
	{ return KErrNotSupported; }

CProxyDrive::~CProxyDrive()
	{ }

EXPORT_C TInt CProxyDrive::DeleteNotify(TInt64, TInt)
	{ return KErrNone; }


/** c'tor */
CTestProxyDrive::CTestProxyDrive()
: CProxyDrive (NULL)
	{
	// default settings
	iCaps.iSize = KMaxBufSize * 2;
	iCaps.iSectorSizeInBytes = 512;
	iCaps.iNumberOfSectors   = I64LOW(iCaps.iSize / iCaps.iSectorSizeInBytes);
	iCaps.iNumPagesPerBlock  = 1;
	}

/** d'or */
CTestProxyDrive::~CTestProxyDrive()
	{}
	
/** Initializs test proxy drive by initializing read buffer 
@return KErrNone
*/
TInt CTestProxyDrive::Initialise()
	{
	__FNLOG ("CTestProxyDrive::Initialise");
	
	// KMaxBufSize is defined in scsiprot.h
	iMediaBuf.FillZ(KMaxBufSize * 2);
	// initialize 1/2 a buffer for testing
	for (TInt i = 0; i<(TInt)KMaxBufSize; i++)
		{
		iMediaBuf[i] = static_cast<TInt8>(i%0x10);
		}

	return KErrNone;
	}

/** Reading data 
@param  aPos position to read
@param  aLength number of bytes to read
@param  aTrg targer buffer
@return KErrNone or KErrOverflow  in case of an error
*/
TInt CTestProxyDrive::Read(TInt64 aPos, TInt aLength, TDes8& aTrg)
	{
	__FNLOG ("CTestProxyDrive::Read");

	// TInt64 is not supported by Copy()
	ASSERT (I64HIGH(aPos) == 0);
	if(!aLength)
		{ return KErrNone; }

	if( (static_cast<TUint>(iMediaBuf.Length()) <= I64LOW(aPos)) && (static_cast<TUint>(iMediaBuf.Length()) <= I64LOW(aPos) + static_cast<TUint>(aLength)) )
		{ return KErrOverflow; }

	aTrg.Copy(&iMediaBuf[I64LOW(aPos)], aLength);
	return KErrNone;
	}
	
/** Writing data 
@param  aPos position to write
@param  aSrc source buffer
@return KErrNone or KErrOverflow in case of an error
*/
TInt CTestProxyDrive::Write (TInt64 aPos, const TDesC8& aSrc)
	{
	__FNLOG ("CTestProxyDrive::Write");

	// TInt64 is not supported by Copy()
	ASSERT(0 == I64HIGH(aPos));
	if (!aSrc.Length())
		{ return KErrNone; }

	if ( (static_cast<TUint>(iMediaBuf.Length()) <= I64LOW(aPos)) && (static_cast<TUint>(iMediaBuf.Length()) <= I64LOW(aPos) + static_cast<TUint>(aSrc.Length())) )
		{ return KErrOverflow; }

	TPtr8 ptr (&iMediaBuf[I64LOW(aPos)], aSrc.Length(), aSrc.Length());
	ptr.Copy(aSrc);
	return KErrNone;
	}


// 
// Command wrappers
//

/** c'tor */
TReadWrite10Cmd::TReadWrite10Cmd()
	{
	iCmd.FillZ (iCmd.MaxLength());
	
	// SBC-2 doc, p. 50
	iCmd[0] = static_cast<TInt8>(iCmd.MaxLength()-1);	// size of the Cmd
	}

/** accessor for BlockAddress 
@param aAddress - new address
*/
void TReadWrite10Cmd::SetBlockAddress(TUint32 aAddress)
	{
	iCmd[3] = static_cast<TInt8>((aAddress & 0xFF000000) >> 24);
	iCmd[4] = static_cast<TInt8>((aAddress & 0x00FF0000) >> 16);
	iCmd[5] = static_cast<TInt8>((aAddress & 0x0000FF00) >> 8);
	iCmd[6] = static_cast<TInt8>(aAddress & 0x000000FF);
	}

