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


#ifndef CUSBHOST
#define CUSBHOST

class TUsbInterfaceDescriptor;
class TUsbDeviceDescriptor;
class RUsbInterface;
class THostMassStorageConfig;

class CUsbMsMountManager;
class CUsbHostAo;

class MDriveDisplay;

class CUsbHost: public CBase, public MUsbHostBusEventObserver
    {
public:
	static CUsbHost* NewL();
	~CUsbHost();
protected:
    void ConstructL();
	CUsbHost();

public:
    TInt DevicesNumber() const;
    void DriveMap(TDriveMap& aDriveMap) const;
    void DeviceMap(TInt aDeviceIndex, TDeviceMap& aDeviceMap) const;

    void Start();

protected:
    void ProcessBusEventL();

private:
    void OpenHubL();
    void CloseHubL();
    void LoadFileSystemL();

    void StartUsbHostMs(TRequestStatus& aStatus);
    void Cancel();

    RUsbHubDriver::TBusEvent::TEvent WaitForBusEvent();

    TToken OpenDeviceL();
    void CloseDeviceL();
    void CloseAllDevicesL();

    void MountDeviceL();
    void DismountDeviceL();
    void DismountAllFileSystemsL();

private:
    CUsbHostAo* iUsbHostAo;
    
    RUsbHubDriver iHubDriver;

    CUsbMsMountManager* iMountManager;

    TUint iDeviceHandle;

    RUsbHubDriver::TBusEvent iEvent;
    };


class CUsbHostDisp: public CUsbHost
    {
public:
	static CUsbHostDisp* NewL(MDriveDisplay& aDriveDisplay);
	~CUsbHostDisp();
private:
    void ConstructL();
	CUsbHostDisp(MDriveDisplay& aDriveDisplay);

private:
    void ProcessBusEventL();

private:
    MDriveDisplay& iDriveDisplay;
    };

#endif
