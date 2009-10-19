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
// RUsbMsMountManager class.
// 
//



/**
 @file
 @internalTechnology
*/

#ifndef CUSBMSMOUNTMANAGER_H
#define CUSBMSMONUTMANAGER_H


class RUsbHubDriver;
class TUsbInterfaceDescriptor;
class TUsbDeviceDescriptor;
class RUsbInterface;


class TUsbPrint
    {
public:
    TUsbPrint();
    void EnableDebug() {iDebug = ETrue;}
    void DisableDebug() {iDebug = EFalse;}

    void PrintTree(const TUsbGenericDescriptor& aDesc,
                   TInt aDepth = 0);

    void PrintDescriptor(const TUsbDeviceDescriptor& aDeviceDesc,
                         TInt aVariant = 0,
                         RUsbDevice* aDevice = NULL);

    void PrintDescriptor(const TUsbConfigurationDescriptor& aConfigDesc,
                         TInt aVariant = 0,
                         RUsbDevice* aDevice = NULL);

    void PrintDescriptor(const TUsbEndpointDescriptor& aEndpointDesc,
                         TInt aVariant = 0,
                         RUsbDevice* aDevice = NULL);

    void PrintDescriptor(const TUsbStringDescriptor& aStringDesc,
                         TInt aVariant = 0,
                         RUsbDevice* aDevice = NULL);

    void PrintDescriptor(const TUsbInterfaceDescriptor& aInterfaceDesc,
                         TInt aVariant = 0,
                         RUsbDevice* aDevice = NULL);

private:
    void SetLanguageToPrintL(RUsbDevice& aDevice);

    void PrintStringFromIndex(const TDesC& aFormatString,
                              TInt aIndex,
                              RUsbDevice* aDevice);

    TBool iDebug;
    };


class CDevice : public CBase
    {
public:
    static const TInt KMaxLun = 16;

public:
    static CDevice* NewL();
    ~CDevice();
private:
    void ConstructL();
    CDevice();

public:
    TToken OpenDeviceL(TUint aDeviceHandle, RUsbHubDriver& aHub);
    void CloseDeviceL();

    void MountLogicalUnitsL();
    void DismountLogicalUnitsL();
    TToken DeviceToken() const {return iDeviceToken;};
    TUint DeviceHandle() const {return iDeviceHandle;};

    TLun DriveMap(TDriveMap& aDriveMap) const;
    TLun DeviceMap(TDeviceMap& aDeviceMap) const;


private:
    TBool IsDeviceMassStorage(const TUsbInterfaceDescriptor& aInterfaceDesc,
                              const TUsbDeviceDescriptor& aDeviceDesc) const;

    TInt GetEndpointAddress(RUsbInterface& aUsbInterface,
                            TInt aInterfaceSetting,
                            TUint8 aTransferType,
                            TUint8 aDirection,
                            TInt& aEndpointAddress) const;



private:
    RUsbDevice iUsbDevice;
    TUint iDeviceHandle;

    RUsbHostMsDevice iUsbHostMsDevice;

    TToken iDeviceToken;
    TLun iNumLuns;

    RExtFileSystem iExt;

    // index is mapped to LUN
    RArray<TDriveNumber> iLuList;

    THostMassStorageConfig iMsConfig;

    TUsbPrint iUsbPrint;
    };


class CUsbMsMountManager : public CBase
    {
public:
	static CUsbMsMountManager* NewL();
	~CUsbMsMountManager();
private:
    void ConstructL();
	CUsbMsMountManager();

public:
    TInt DevicesNumber() const;
    void DriveMap(TDriveMap& aDriveMap) const;
    void DeviceMap(TInt aDeviceIndex, TDeviceMap& aDeviceMap) const;


public:
    // adds new device entry for this device
    void AddDeviceL(CDevice* aDevice);
    // removes device entry for this device
    CDevice* RemoveDeviceL(TUint aDeviceHandle);

    void CloseAllDevicesL();

    // mounts all LUNs for the device
    void MountDeviceL(TUint aDeviceHandle);
    // dismount all LUNs for this device
    void DismountDeviceL(TUint aDeviceHandle);
    // dismount all LUNs
    void DismountL();

private:
    TInt GetDeviceIndexL(TToken aDeviceToken) const;
    TInt GetHandleIndexL(TUint aDeviceHandle) const;

private:
    RPointerArray<CDevice> iDeviceList;
    };


inline TInt CUsbMsMountManager::DevicesNumber() const
    {
    return iDeviceList.Count();
    }

#endif // CUSBMSMOUNTMANAGER_H
