// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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


#include "drivepublisher.h"
#include "drivemanager.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "drivepublisherTraces.h"
#endif



//
// Use Lookup table to translate from the internal pair of state variables
// to the externally published drive state code.
//
static const TUint8 table[][5] =
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
    iMediaErrorProperty.Close();
    RProperty::Delete(KUsbMsDriveState_Category, EUsbMsDriveState_MediaError);
    }

/**
Publishing method

Publishes the Media Error property event

@param aError ETrue if drive media has an error else EFalse for no error
*/
void RDriveMediaErrorPublisher::PublishError(TBool aError)
    {
    OstTraceFunctionEntry0(DRIVEPUBLISHER_100);
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
RDriveStateChangedPublisher::RDriveStateChangedPublisher(TRefMsDriveList aDrives,
                                                         TRefDriveMap aDriveMap)
    :
    iDrives(aDrives),
    iDriveMap(aDriveMap)
    {
    _LIT_SECURITY_POLICY_PASS(KMassStorageReadPolicy);
    _LIT_SECURITY_POLICY_S0(KMassStorageWritePolicy, KUsbMsDriveState_Category.iUid);

    TInt result = RProperty::Define(KUsbMsDriveState_Category,
                                    EUsbMsDriveState_DriveStatus, RProperty::EByteArray,
                                    KMassStorageReadPolicy, KMassStorageWritePolicy,
                                    KUsbMsMaxDrives*2);
    __ASSERT_DEBUG(result == KErrAlreadyExists || result == KErrNone, User::Invariant());
    result = result;    // remove urel warning
    }


RDriveStateChangedPublisher::~RDriveStateChangedPublisher()
    {
    RProperty::Delete(KUsbMsDriveState_Category, EUsbMsDriveState_DriveStatus);
    }


/**
Publishing method

Sends a property event on behalf of CMassStorageDrive, with the mountstate and drivestate
values encoded into one 32-bit word.
*/
void RDriveStateChangedPublisher::DriveStateChanged()
    {
    TUsbMsDrivesStatus allDrivesStatus;
    for(TUint8 i=0; i<KUsbMsMaxDrives && iDrives[i]; i++)
        {
        allDrivesStatus.Append(iDriveMap[i]);

        CMassStorageDrive::TMountState ms = iDrives[i]->MountState();
        CMassStorageDrive::TDriveState ds = iDrives[i]->DriveState();
        TInt driveStatus = EUsbMsDriveState_Error;
        if((TUint8)ds < sizeof(table[0]) && (TUint8)ms < sizeof(table)/sizeof(table[0]))
            {
            driveStatus = table[ms][ds];
            OstTraceExt3(TRACE_SMASSSTORAGE_DRIVE, DRIVEPUBLISHER_110,
                         "ms=%d ds=%d %d", ms, ds, driveStatus);
            }
        allDrivesStatus.Append(driveStatus);
        }

    OstTrace1(TRACE_SMASSSTORAGE_DRIVE, DRIVEPUBLISHER_111,
              "Publishing EUsbMsDriveState_DriveStatus for %d drives", allDrivesStatus.Length()/2);

    if(KErrNone != RProperty::Set(KUsbMsDriveState_Category,
                                  EUsbMsDriveState_DriveStatus,
                                  allDrivesStatus))
        {
        __ASSERT_DEBUG(EFalse,User::Invariant());
        }
    }


//----------------------------------------------------------------------------
#ifndef USB_TRANSFER_PUBLISHER
/**
Private default constructor to ensure that NewL is used

@param aSubKey
@param aDrives
*/
CDriveTransferPublisher::CDriveTransferPublisher(
    TUsbMsDriveState_Subkey aSubKey,
    TRefMsDriveList aDrives)
    :
    iSubKey(aSubKey),
    iDrives(aDrives)
    {
    }


void CDriveTransferPublisher::ConstructL()
    {
    _LIT_SECURITY_POLICY_PASS(KMassStorageReadPolicy);
    _LIT_SECURITY_POLICY_S0(KMassStorageWritePolicy, KUsbMsDriveState_Category.iUid);

    TInt r = RProperty::Define(iSubKey, RProperty::EByteArray,
                               KMassStorageReadPolicy, KMassStorageWritePolicy,
                               KUsbMsMaxDrives*sizeof(TInt));

    if (r != KErrAlreadyExists)
        {
        User::LeaveIfError(r);
        }

    User::LeaveIfError(iProperty.Attach(KUsbMsDriveState_Category, iSubKey));

    // Create the EDataTransferred timer
    iTimer = CPeriodic::NewL(CActive::EPriorityStandard);
    iTimerRunning = EFalse;
    }


/**
Destructor
*/
CDriveTransferPublisher::~CDriveTransferPublisher()
    {
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
TInt CDriveTransferPublisher::PublishDataTransferredEvent(TAny* obj)
    {
    static_cast<CDriveTransferPublisher*>(obj)->DoPublishDataTransferredEvent();
    return 1;
    }


/**
Update the data transferred properties if the counts have changed since
the last update.
*/
void CDriveTransferPublisher::DoPublishDataTransferredEvent()
    {
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
TBool CDriveTransferPublisher::PublishDataTransferred()
    {
    TUsbMsBytesTransferred bytesTransferred;
    TBool dataTransferred = EFalse;

    for (TInt i=0; i < iDrives.Count() && iDrives[i]; i++)
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
        for (TInt j=0; j < iDrives.Count() && iDrives[j]; j++)
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
void CDriveTransferPublisher::StartTimer()
    {
    if (!iTimerRunning)
        {
        // EDataTransferred event every second
        const TTimeIntervalMicroSeconds32 interval = 1 * 1000 * 1000;
        TCallBack callback(PublishDataTransferredEvent, this);
        OstTrace0(TRACE_SMASSSTORAGE_DRIVE, DRIVEPUBLISHER_140, "Starting timer");
        iTimer->Start(interval, interval, callback);
        iTimerRunning = ETrue;
        }
    }


/**
Ensure that the Timer is stopped
*/
void CDriveTransferPublisher::StopTimer()
    {
    if (iTimerRunning)
        {
        OstTrace0(TRACE_SMASSSTORAGE_DRIVE, DRIVEPUBLISHER_141, "Stopping timer");
        iTimer->Cancel();
        iTimerRunning = EFalse;
        }
    }


//----------------------------------------------------------------------------
/**
Constructor for Write property

@param aDrives
*/
CDriveWriteTransferPublisher* CDriveWriteTransferPublisher::NewL(TRefMsDriveList aDrives)
    {
    CDriveWriteTransferPublisher* self = new (ELeave) CDriveWriteTransferPublisher(aDrives);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }


/**
Constructor

@param aDrives
*/
CDriveWriteTransferPublisher::CDriveWriteTransferPublisher(TRefMsDriveList aDrives)
    :
    CDriveTransferPublisher(EUsbMsDriveState_KBytesWritten, aDrives)
    {
    }


/**
Transfer function for Write property

@param aLun
*/
TUint CDriveWriteTransferPublisher::GetBytesTransferred(TUint aLun) const
    {
    return iDrives[aLun]->KBytesWritten();
    }


//----------------------------------------------------------------------------
/**
Constructor for Read property

@param aDrives
*/
CDriveReadTransferPublisher* CDriveReadTransferPublisher::NewL(TRefMsDriveList aDrives)
    {
    CDriveReadTransferPublisher* self = new (ELeave) CDriveReadTransferPublisher(aDrives);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }


/**
Constructor

@param aDrives
*/
CDriveReadTransferPublisher::CDriveReadTransferPublisher(TRefMsDriveList aDrives)
    :
    CDriveTransferPublisher(EUsbMsDriveState_KBytesRead, aDrives)
    {
    }


/**
Transfer function for Read property

@param aLun
*/
TUint CDriveReadTransferPublisher::GetBytesTransferred(TUint aLun) const
    {
    return iDrives[aLun]->KBytesRead();
    }


//----------------------------------------------------------------------------
#else
/**
Private default constructor to ensure that NewL is used

@param aSubKey
@param aArray
*/
CUsbTransferPublisher::CUsbTransferPublisher(
    TUsbMsDriveState_Subkey aSubKey,
    TRefBytesTransferedList aArray)
    :
    iSubKey(aSubKey),
    iArray(aArray)
    {
    }


void CUsbTransferPublisher::ConstructL()
    {
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
    static_cast<CUsbTransferPublisher*>(obj)->DoPublishDataTransferredEvent();
    return 1;
    }


/**
Update the data transferred properties if the counts have changed since
the last update.
*/
void CUsbTransferPublisher::DoPublishDataTransferredEvent()
    {
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
    if (!iTimerRunning)
        {
        // EDataTransferred event every second
        const TTimeIntervalMicroSeconds32 interval = 1 * 1000 * 1000;
        TCallBack callback(PublishDataTransferredEvent, this);
        OstTrace0(TRACE_SMASSSTORAGE_DRIVE, DRIVEPUBLISHER_120, "Starting timer");
        iTimer->Start(interval, interval, callback);
        iTimerRunning = ETrue;
        }
    }


/**
Ensure that the Timer is stopped
*/
void CUsbTransferPublisher::StopTimer()
    {
    if (iTimerRunning)
        {
        OstTrace0(TRACE_SMASSSTORAGE_DRIVE, DRIVEPUBLISHER_130, "Stopping timer");
        iTimer->Cancel();
        iTimerRunning = EFalse;
        }
    }


//----------------------------------------------------------------------------
/**
Constructor for Write property

@param aArray
*/
CUsbWriteTransferPublisher* CUsbWriteTransferPublisher::NewL(TRefBytesTransferedList aArray)
    {
    CUsbWriteTransferPublisher* self = new (ELeave) CUsbWriteTransferPublisher(aArray);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }


CUsbWriteTransferPublisher::CUsbWriteTransferPublisher(
    TRefBytesTransferedList aArray)
    :
    CUsbTransferPublisher(EUsbMsDriveState_KBytesWritten, aArray)
    {
    }


//----------------------------------------------------------------------------
/**
Constructor for Read property

@param aArray
*/
CUsbReadTransferPublisher* CUsbReadTransferPublisher::NewL(TRefBytesTransferedList aArray)
    {
    CUsbReadTransferPublisher* self = new (ELeave) CUsbReadTransferPublisher(aArray);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }


CUsbReadTransferPublisher::CUsbReadTransferPublisher(
    TRefBytesTransferedList aArray)
    :
    CUsbTransferPublisher(EUsbMsDriveState_KBytesRead, aArray)
    {
    }
#endif // USB_TRANSFER_PUBLISHER
