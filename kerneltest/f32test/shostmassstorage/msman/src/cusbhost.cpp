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
//

#include <d32usbdi_hubdriver.h>
#include <d32usbdi.h>
#include <d32otgdi.h>
#include <d32usbdescriptors.h>
#include <d32usbtransfers.h>
#include <e32property.h>
#include <f32file.h>

#include "usbtypes.h"
#include "rusbhostmsdevice.h"
#include "rextfilesystem.h"
#include "cusbmsmountmanager.h"

#include "mdrivedisplay.h"
#include "cusbhostao.h"
#include "cusbhost.h"
#include "tmslog.h"
#include "debug.h"



_LIT(KHubDriverLddFileName, "usbhubdriver");
_LIT(KUsbdiLddFileName, "usbdi");


CUsbHost* CUsbHost::NewL()
    {
    __MSFNSLOG
	CUsbHost* r = new (ELeave) CUsbHost();
	CleanupStack::PushL(r);

	r->ConstructL();
	CleanupStack::Pop();
	return r;
    }


void CUsbHost::ConstructL()
    {
    __MSFNLOG
    OpenHubL();
	LoadFileSystemL();

    iUsbHostAo = CUsbHostAo::NewL(iHubDriver, iEvent, *this);

    iMountManager = CUsbMsMountManager::NewL();
    }


CUsbHost::CUsbHost()
    {
    __MSFNLOG
    }


CUsbHost::~CUsbHost()
    {
    __MSFNLOG
    delete iUsbHostAo;

    DismountAllFileSystemsL();
    CloseAllDevicesL();
    CloseHubL();

    delete iMountManager;
    }


void CUsbHost::LoadFileSystemL()
    {
    __MSFNLOG
    RFs fs;
    User::LeaveIfError(fs.Connect());
    CleanupClosePushL(fs);
	_LIT(KFsNm, "elocal");

    TInt err;
    err = fs.AddFileSystem(KFsNm);
    if (err != KErrAlreadyExists)
        User::LeaveIfError(err);

    err = fs.AddFileSystem(_L("ELOCAL"));
    if (!(KErrAlreadyExists == err || KErrCorrupt == err))
        User::LeaveIfError(err);

    err = fs.AddProxyDrive(_L("usbhostms.pxy"));
    if (!(KErrAlreadyExists == err || KErrCorrupt == err))
        User::LeaveIfError(err);

    CleanupStack::PopAndDestroy(&fs);
    }


void CUsbHost::OpenHubL()
    {
    __MSFNLOG
    TInt err;
    err = User::LoadLogicalDevice(KHubDriverLddFileName);
    if (err != KErrAlreadyExists)
        User::LeaveIfError(err);

    err = User::LoadLogicalDevice(KUsbdiLddFileName);
    if (err != KErrAlreadyExists)
        User::LeaveIfError(err);

    err = iHubDriver.Open();
    User::LeaveIfError(err);
    }


void CUsbHost::CloseHubL()
    {
    __MSFNLOG
	iHubDriver.StopHost();
	iHubDriver.Close();

	TInt err1 = User::FreeLogicalDevice(KUsbdiLddFileName);
	__ASSERT_DEBUG(err1==KErrNone, User::Panic(KUsbdiLddFileName, err1));

	TInt err2 = User::FreeLogicalDevice(KHubDriverLddFileName);
	__ASSERT_DEBUG(err2==KErrNone, User::Panic(KHubDriverLddFileName, err2));

	User::LeaveIfError(err1);
	User::LeaveIfError(err2);
    }


TToken CUsbHost::OpenDeviceL()
    {
    __MSFNLOG
    CDevice* device = CDevice::NewL();

    TToken token = 0;
    TRAPD(err, token = device->OpenDeviceL(iDeviceHandle, iHubDriver));
    if (err)
        {
        User::Leave(err);
        }

    iMountManager->AddDeviceL(device);
    return token;
    }


void CUsbHost::CloseDeviceL()
    {
    __MSFNLOG
    CDevice* device = iMountManager->RemoveDeviceL(iDeviceHandle);
    device->CloseDeviceL();
    delete device;
    }


void CUsbHost::CloseAllDevicesL()
    {
    __MSFNLOG
    iMountManager->CloseAllDevicesL();
    }


void CUsbHost::MountDeviceL()
    {
    __MSFNLOG
    iMountManager->MountDeviceL(iDeviceHandle);
    }


void CUsbHost::DismountDeviceL()
    {
    __MSFNLOG
    iMountManager->DismountDeviceL(iDeviceHandle);
    }


void CUsbHost::DismountAllFileSystemsL()
    {
    __MSFNLOG
    iMountManager->DismountL();
    }


void CUsbHost::Start()
    {
    __MSFNLOG
    iUsbHostAo->Wait();
    }


void CUsbHost::ProcessBusEventL()
    {
    __MSFNLOG

    __USBHOSTPRINT2(_L(">> CUsbHost RUsbHubDriver Event[%d] Device Handle = %d"),
                    iEvent.iEventType, iEvent.iDeviceHandle);
    __USBHOSTPRINT2(_L("Error = %d reason = %x"),
                    iEvent.iError, iEvent.iReason);

    iDeviceHandle = iEvent.iDeviceHandle;
    RUsbHubDriver::TBusEvent::TEvent event = iEvent.iEventType;

    if (event == RUsbHubDriver::TBusEvent::EDeviceAttached)
        {
        /* Jungo stack has attached the device */
        TUint32 token = OpenDeviceL();
        MountDeviceL();
	    __USBHOSTPRINT(_L("CUsbHost: device attached"));
        }
    else if (event == RUsbHubDriver::TBusEvent::EDeviceRemoved)
        {
		TRAPD(err, DismountDeviceL());
	    CloseDeviceL();
        User::LeaveIfError(err);
        __USBHOSTPRINT(_L("CUsbHost: device removed"));
        }

    else
        {
        // nothing to do
        }
    }


RUsbHubDriver::TBusEvent::TEvent CUsbHost::WaitForBusEvent()
    {
    __MSFNLOG
    TRequestStatus status;
    RUsbHubDriver::TBusEvent event;
    TBool eventReceived = EFalse;
    do
        {
        iHubDriver.WaitForBusEvent(event, status);
        __USBHOSTPRINT(_L("Waiting..."));
        User::WaitForRequest(status);
        __USBHOSTPRINT2(_L(">> CUsbHost RUsbHubDriver Event[%d] Device Handle = %d)"),
                        iEvent.iEventType, iEvent.iDeviceHandle);
        __USBHOSTPRINT2(_L("Error = %d reason = %x"),
                        iEvent.iError, iEvent.iReason);

        if (status != KErrNone)
            {
            __USBHOSTPRINT1(_L("Status error = %d"), status.Int());
            }
        iDeviceHandle = event.iDeviceHandle;

        switch (event.iEventType)
            {
            case RUsbHubDriver::TBusEvent::EDeviceAttached:
            case RUsbHubDriver::TBusEvent::EDeviceRemoved:
                eventReceived = ETrue;
                break;
            default:
                break;
            }

        } while (!eventReceived);
    return event.iEventType;
    }



void CUsbHost::Cancel()
    {
    iHubDriver.CancelWaitForBusEvent();
    }


void CUsbHost::DriveMap(TDriveMap& aDriveMap) const
    {
    __MSFNSLOG
    iMountManager->DriveMap(aDriveMap);
    }


void CUsbHost::DeviceMap(TInt aDeviceIndex, TDeviceMap& aDeviceMap) const
    {
    __MSFNSLOG
    iMountManager->DeviceMap(aDeviceIndex, aDeviceMap);
    }


TInt CUsbHost::DevicesNumber() const
    {
    return iMountManager->DevicesNumber();
    }


CUsbHostDisp* CUsbHostDisp::NewL(MDriveDisplay& aDriveDisplay)
    {
    __MSFNSLOG
	CUsbHostDisp* r = new (ELeave) CUsbHostDisp(aDriveDisplay);
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
    }


void CUsbHostDisp::ConstructL()
    {
    __MSFNLOG
    CUsbHost::ConstructL();
    }


CUsbHostDisp::CUsbHostDisp(MDriveDisplay& aDriveDisplay)
:   CUsbHost(),
    iDriveDisplay(aDriveDisplay)
    {
    __MSFNLOG
    }


CUsbHostDisp::~CUsbHostDisp()
    {
    __MSFNLOG
    }

void CUsbHostDisp::ProcessBusEventL()
    {
    CUsbHost::ProcessBusEventL();

    // update display
    iDriveDisplay.DriveListL();

    // Devices attached
    TInt devicesNumber = DevicesNumber();
    iDriveDisplay.DevicesNumber(devicesNumber);

    // LUNs for each device
    TDeviceMap deviceMap;
    TInt deviceIndex;
    TInt row;
    for (row = 0, deviceIndex = (devicesNumber - 1); deviceIndex >= 0 ; row++, deviceIndex--)
        {
        deviceMap.Reset();
        // get map
        DeviceMap(deviceIndex, deviceMap);

        // display
        iDriveDisplay.DeviceMapL(row, deviceIndex, deviceMap);
        }

    iDriveDisplay.DeviceMapClear(row);

    // Display all Drives
    TDriveMap driveMap;
    driveMap.Reset();
    DriveMap(driveMap);
    iDriveDisplay.DriveMapL(driveMap);
    }


