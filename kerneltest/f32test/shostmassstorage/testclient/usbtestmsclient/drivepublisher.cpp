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
// Class implementation of the drive publishing classes -
// RDriveMediaErrorPublisher,
// RDriveStateChangedPublisher,
// CDriveTransferPublisher,
// CDriveWriteTransferPublisher,
// CDriveReadTransferPublisher,
// CUsbTransferPublisher,
// CUsbReadTransferPublisher,
// CUsbReadTransferPublisher.
// 
//



/**
 @file
 @internalTechnology
*/

#include <e32std.h>
#include <e32base.h>
#include <e32property.h>
#include <f32file.h>

#include "mstypes.h"
#include "msctypes.h"
#include "usbmsshared.h"

#include "drivepublisher.h"
#include "drivemanager.h"
#include "debug.h"
#include "msdebug.h"

//
// Use Lookup table to translate from the internal pair of state variables
// to the externally published drive state code.
//
LOCAL_D	const TUint8 table[][5] =
{
//TMountState=EDisconnected
	{EUsbMsDriveState_Disconnected,
	 EUsbMsDriveState_Disconnected,
	 EUsbMsDriveState_Disconnected,
	 EUsbMsDriveState_Disconnected,
	 EUsbMsDriveState_Disconnected},
//TMountState=EConnecting
	{EUsbMsDriveState_Connecting,
	 EUsbMsDriveState_Connecting,
	 EUsbMsDriveState_Connecting,
	 EUsbMsDriveState_Connecting,
	 EUsbMsDriveState_Connecting},
//TMountState=EConnected
	//EIdle,EActive,ELocked,EMediaNotPresent,EErrDisMounted
	{EUsbMsDriveState_Connected,
	 EUsbMsDriveState_Active,
	 EUsbMsDriveState_Locked,
	 EUsbMsDriveState_MediaNotPresent,
	 EUsbMsDriveState_Removed},
//TMountState=EDisconnecting
	{EUsbMsDriveState_Disconnecting,
	 EUsbMsDriveState_Disconnecting,
	 EUsbMsDriveState_Disconnecting,
	 EUsbMsDriveState_Disconnecting,
	 EUsbMsDriveState_Disconnecting}
};


//----------------------------------------------------------------------------
/**
Constructor
*/
RDriveMediaErrorPublisher::RDriveMediaErrorPublisher()
	{
	__MSFNLOG
	_LIT_SECURITY_POLICY_PASS(KMassStorageReadPolicy);
	_LIT_SECURITY_POLICY_S0(KMassStorageWritePolicy, KUsbMsDriveState_Category.iUid);

	TInt result = RProperty::Define(EUsbMsDriveState_MediaError, RProperty::EInt,
									KMassStorageReadPolicy, KMassStorageWritePolicy);

	__ASSERT_DEBUG(result == KErrAlreadyExists || result == KErrNone, User::Invariant());

	result = iMediaErrorProperty.Attach(KUsbMsDriveState_Category, EUsbMsDriveState_MediaError);
	__ASSERT_DEBUG(result == KErrNone, User::Invariant());
	}


RDriveMediaErrorPublisher::~RDriveMediaErrorPublisher()
	{
	__MSFNLOG
	iMediaErrorProperty.Close();
	RProperty::Delete(KUsbMsDriveState_Category, EUsbMsDriveState_MediaError);
	}

/**
Publishing method

Publishes the Media Error property event

@param aError ETrue if drive media has an error else EFalse for no error
*/
void RDriveMediaErrorPublisher::PublishErrorL(TBool aError)
	{
    __MSFNLOG
	__PRINT1(_L("<< RDriveMediaErrorPublisher::PublishError %x"), aError);

	TInt oldValue;
	iMediaErrorProperty.Get(oldValue);

	if (oldValue != aError)
		{
		User::LeaveIfError(iMediaErrorProperty.Set(aError));
		}
	}

//----------------------------------------------------------------------------
/**
Constructor

@param aDrives
@param aDriveMap
*/
RDriveStateChangedPublisher::RDriveStateChangedPublisher(const TMsDriveList& aDrives,
														 const TLunToDriveMap& aDriveMap)
	:
	iDrives(aDrives),
	iDriveMap(aDriveMap)
	{
	__MSFNLOG
	_LIT_SECURITY_POLICY_PASS(KMassStorageReadPolicy);
	_LIT_SECURITY_POLICY_S0(KMassStorageWritePolicy, KUsbMsDriveState_Category.iUid);

	TInt result = RProperty::Define(KUsbMsDriveState_Category,
									EUsbMsDriveState_DriveStatus, RProperty::EByteArray,
									KMassStorageReadPolicy, KMassStorageWritePolicy,
									KUsbMsMaxDrives*2);
	__ASSERT_DEBUG(result == KErrAlreadyExists || result == KErrNone, User::Invariant());
	result = result;	// remove urel warning
	}


RDriveStateChangedPublisher::~RDriveStateChangedPublisher()
	{
	__MSFNLOG
	RProperty::Delete(KUsbMsDriveState_Category, EUsbMsDriveState_DriveStatus);
	}


/**
Publishing method

Sends a property event on behalf of CMassStorageDrive, with the mountstate and drivestate
values encoded into one 32-bit word.
*/
void RDriveStateChangedPublisher::DriveStateChanged()
	{
	__MSFNLOG
	TUsbMsDrivesStatus allDrivesStatus;

	for(TUint8 i = 0; i < iDrives.Count(); i++)
		{
		allDrivesStatus.Append(iDriveMap[i]);

		CMassStorageDrive::TMountState ms = iDrives[i]->MountState();
		TLocalDriveRef::TDriveState ds = iDrives[i]->DriveState();
		TInt driveStatus = EUsbMsDriveState_Error;
		if((TUint8)ds < sizeof(table[0]) && (TUint8)ms < sizeof(table)/sizeof(table[0]))
			{
			driveStatus = table[ms][ds];
			__PRINT3(_L("ms=%d ds=%d %d"), ms, ds, driveStatus);
			}
		allDrivesStatus.Append(driveStatus);
		}


	__PRINT1(_L("Publishing EUsbMsDriveState_DriveStatus for %d drives\n"),
				allDrivesStatus.Length()/2);

	if(KErrNone != RProperty::Set(KUsbMsDriveState_Category,
								  EUsbMsDriveState_DriveStatus,
								  allDrivesStatus))
		{
		__ASSERT_DEBUG(EFalse,User::Invariant());
		}
	}


//----------------------------------------------------------------------------

/**
Private default constructor to ensure that NewL is used

@param aSubKey
@param aArray
*/
CUsbTransferPublisher::CUsbTransferPublisher(TUsbMsDriveState_Subkey aSubKey,
                                             const TBytesTransferedList& aArray)
:   iSubKey(aSubKey),
	iArray(aArray)
	{
    __MSFNLOG
	}


void CUsbTransferPublisher::ConstructL()
	{
	__MSFNLOG

	_LIT_SECURITY_POLICY_PASS(KMassStorageReadPolicy);
	_LIT_SECURITY_POLICY_S0(KMassStorageWritePolicy, KUsbMsDriveState_Category.iUid);

	TInt r = RProperty::Define(iSubKey, RProperty::EByteArray,
							   KMassStorageReadPolicy, KMassStorageWritePolicy,
							   KUsbMsMaxDrives*sizeof(TInt));

	if (r != KErrAlreadyExists)
		{
		User::LeaveIfError(r);
		}

	// Attach to the properties here. Only do this once, continuously attaching
	// will currently cause a memory leak
	User::LeaveIfError(iProperty.Attach(KUsbMsDriveState_Category, iSubKey));

	// Create the EDataTransferred timer
	iTimer = CPeriodic::NewL(CActive::EPriorityStandard);
	iTimerRunning = EFalse;
	}


/**
Destructor
*/
CUsbTransferPublisher::~CUsbTransferPublisher()
	{
	__MSFNLOG
	if(iTimer)
		{
		iTimer->Cancel();
		}
	delete iTimer;
	iProperty.Close();

	RProperty::Delete(KUsbMsDriveState_Category, iSubKey);
	}


/**
A static wrapper for the DoPublishDataTransferredEvent member function
for use as a timer callback function.

@param obj 'this' pointer
@return not used in CPeriodic callback (see TCallback)
*/
TInt CUsbTransferPublisher::PublishDataTransferredEvent(TAny* obj)
	{
	__MSFNSLOG
	static_cast<CUsbTransferPublisher*>(obj)->DoPublishDataTransferredEvent();
	return 1;
	}


/**
Update the data transferred properties if the counts have changed since
the last update.
*/
void CUsbTransferPublisher::DoPublishDataTransferredEvent()
	{
    __MSFNLOG
	if (PublishDataTransferred())
		{
		// some data has been transfered so reset the counter
		iTimerCancelCnt = ETimerCancelDelay;
		}

	// Update the cancel count if no data was transferred the last
	// (few) times this has been called
	if (--iTimerCancelCnt == 0)
		{
		StopTimer();
		iTimerCancelCnt = ETimerCancelDelay;
		}
	}


/**
Update the data transferred properties if the counts have changed since
the last update.
*/
TBool CUsbTransferPublisher::PublishDataTransferred()
	{
	__MSFNLOG
	TUsbMsBytesTransferred bytesTransferred;
	TBool dataTransferred = EFalse;

	for (TInt i = 0; i < iArray.Count(); i++)
		{
		bytesTransferred[i] = GetBytesTransferred(i);
		}

	// Update the properties only if they have changed
	// (or if there's an error reading the old value.)
	// Possible optimisation: keep a copy of the value
	// as a member variable so we don't need the Get.
	TUsbMsBytesTransferred oldValue;

	if ((iProperty.Get(oldValue) != KErrNone) || (oldValue != bytesTransferred))
		{
#ifdef __PRINT3
		// trace of the bytes transferred
		for (TInt j=0; j < iArray.Count(); j++)
			{
			if(oldValue[j] != bytesTransferred[j])
				{
				__PRINT3(_L("CDrivePublisher: KBytes[%d] %d->%d\n"), j, oldValue[j], bytesTransferred[j]);
				}
			}
#endif
		if (KErrNone != iProperty.Set(bytesTransferred))
			{
			__ASSERT_DEBUG(EFalse, User::Invariant());
			}
		dataTransferred = ETrue;
		}

	return dataTransferred;
	}


/**
Starts timer to periodically publish results.
If the timer is not yet running then start it.
*/
void CUsbTransferPublisher::StartTimer()
	{
	__MSFNLOG
	if (!iTimerRunning)
		{
		// EDataTransferred event every second
		const TTimeIntervalMicroSeconds32 interval = 1 * 1000 * 1000;
		TCallBack callback(PublishDataTransferredEvent, this);
		__PRINT(_L("Starting timer"));
		iTimer->Start(interval, interval, callback);
		iTimerRunning = ETrue;
		}
	}


/**
Ensure that the Timer is stopped
*/
void CUsbTransferPublisher::StopTimer()
	{
	__MSFNLOG
	if (iTimerRunning)
		{
		__PRINT(_L("Stopping timer"));
		iTimer->Cancel();
		iTimerRunning = EFalse;
		}
	}


//----------------------------------------------------------------------------
/**
Constructor for Write property

@param aArray
*/
CUsbWriteTransferPublisher* CUsbWriteTransferPublisher::NewL(const TBytesTransferedList& aArray)
	{
	__MSFNSLOG
	CUsbWriteTransferPublisher* self = new (ELeave) CUsbWriteTransferPublisher(aArray);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}


CUsbWriteTransferPublisher::CUsbWriteTransferPublisher(const TBytesTransferedList& aArray)
:   CUsbTransferPublisher(EUsbMsDriveState_KBytesWritten, aArray)
	{
    __MSFNLOG
	}


//----------------------------------------------------------------------------
/**
Constructor for Read property

@param aArray
*/
CUsbReadTransferPublisher* CUsbReadTransferPublisher::NewL(const TBytesTransferedList& aArray)
	{
	__MSFNSLOG
	CUsbReadTransferPublisher* self = new (ELeave) CUsbReadTransferPublisher(aArray);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}


CUsbReadTransferPublisher::CUsbReadTransferPublisher(const TBytesTransferedList& aArray)
:   CUsbTransferPublisher(EUsbMsDriveState_KBytesRead, aArray)
	{
    __MSFNLOG
	}

/**
Transfer function for the property

@return Cumulative bytes read since the host connected to the drive,
        in multiples of KBytesPerKilobyte rounded to nearest integer value.
        The KBytes refer to multiples of 1000, not 1024.
*/
TUint CUsbTransferPublisher::GetBytesTransferred(TLun aLun) const
{
	return I64LOW(iArray[aLun] / (TUint64)1000);
}
