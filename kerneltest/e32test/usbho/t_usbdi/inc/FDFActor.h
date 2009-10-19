#ifndef __FDF_ACTOR_H
#define __FDF_ACTOR_H

/*
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* @file FDFActor.h
* @internalComponent
* 
*
*/



#include <e32base.h>
#include <e32hashtab.h>
#include <d32usbdescriptors.h>
#include <d32usbdi_hubdriver.h>
#include <d32usbdi.h>
#include <d32otgdi.h>
#include <e32test.h>

extern RTest gtest;

namespace NUnitTesting_USBDI
	{

/**
This class describes an observer for USB bus events reported from the acting
Function Driver Framework (Hub Driver)
*/	
class MUsbBusObserver
	{
public:

/**
Called when a usb client device was connected to the host and has a bus address
@param aDeviceHandle the unique handle to the device
*/
	virtual void DeviceInsertedL(TUint aDeviceHandle) = 0;
	
/**
Called when a previously connected usb device has disconnected from the bus
@param aDeviceHandle the handle to the device that has just disconnected.
	   This handle will (after this function) now be void
*/
	virtual void DeviceRemovedL(TUint aDeviceHandle) = 0;
	
/**
Called when there has been a error on the bus as seen by the Hub Driver
@param aError the bus error code
*/
	virtual void BusErrorL(TInt aError) = 0;
	
/**
Called when the device has changed state
@param aPreviousState the previous state of the device
@param aNewState the now new observerd state of the device
@param aCompletionCode the comletion code of state change
*/
	virtual void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
		TInt aCompletionCode) = 0;
	};




/**
This class represents the host side object for a USB test device.  This means a device that
is connected to the bus.
*/
class CUsbTestDevice : public CActive
	{
public:
/**
Construct a USB bus connected device
@param aHubDriver the USB hub driver object
@param aDeviceHandle the unique handle value to the device
@param aObserver the 
*/
	static CUsbTestDevice* NewL(RUsbHubDriver& aHubDriver,TUint aDeviceHandle,MUsbBusObserver& aObserver);	
	
/**
Destructor
*/
	~CUsbTestDevice();
	
/**
Obtain the USB specification that this device complies to
@return the usb compliance specification
*/
	TUint16 DeviceSpec() const;
	
/**
Obtain the Identity of the product
@return the product identity
*/
	TUint16 ProductId() const;
	
/**
Obtain the Identity of the vendor
@return the vendor identity
*/
	TUint16 VendorId() const;
	
/**
Obtain the product string for the connected device
@return the device's product string
*/
	const TDesC16& Product() const;
	
/**
Obtani the configuration string for the connected device
@return the device's configuration string
*/
	const TDesC16& ConfigurationString() const;
	
/**
Obtain the serial number of the connected device
@return the device's serial number
*/
	const TDesC16& SerialNumber() const;
	
/**
Obtain the manufacturer of the connected device
@return the manufacturer as string
*/
	const TDesC16& Manufacturer() const;
	
/**
Access the raw referrence to the USB device API object
@return a referrence to the RUsbDevice object
*/
	RUsbDevice& Device();
	
/**
Access the device descriptor
@return a referrence to the device descriptor
*/	
const TUsbDeviceDescriptor& DeviceDescriptor() const;

/**
Access the configuration descriptor
@return a referrence to the configuration descriptor
*/
const TUsbConfigurationDescriptor& ConfigurationDescriptor() const;


protected:

/**
Constructor, build a host side usb test object
@param aHubDriver the USB host hub driver
@param aHandle the unique handle to the usb device
@param aObserver the observer of usb host bus events
*/
	CUsbTestDevice(RUsbHubDriver& aHubDriver,TUint aHandle,MUsbBusObserver& aObserver);
	
/**
2nd phase construction
*/
	void ConstructL();
	
protected: // From CActive

/**
Cancels the current asynchronous activity of this test device
*/
	void DoCancel();
	
/**
*/
	void RunL();
	
/** 
*/
	TInt RunError(TInt aError);
	
private:
	RUsbHubDriver& iDriver;
	RUsbDevice::TDeviceState iCurrentState;
	TUint iHandle;
	RUsbDevice iDevice;
	TUsbDeviceDescriptor iDeviceDescriptor;
	TUsbConfigurationDescriptor iConfigDescriptor;
	TUint16 iDeviceSpec;
	TUint16 iPid;
	TUint16 iVid;
	
/**
The observer of events on the bus bus, noticed by the host hub driver
*/	
	MUsbBusObserver& iObserver;	
	
/**
The string for the configuration
*/
	TUsbStringDescriptor* iConfigStringDesc;
	TBuf8<255> iConfigStringData;
	TBuf16<128> iConfigString;
	
/**
The string for the manufacturer
*/
	TUsbStringDescriptor* iManufacturerStringDesc;
	TBuf8<255> iManufacturerStringData;
	TBuf16<128> iManufacturerString;
	
/**
The string for the product name
*/
	TUsbStringDescriptor* iProductStringDesc;
	TBuf8<255> iProductStringData;
	TBuf16<128> iProductString;
	
/**
The string for the serial number
*/
	TUsbStringDescriptor* iSerialNumberDesc;
	TBuf8<255> iSerialNumberStringData;
	TBuf16<128> iSerialNumber;
	};
	
/**
This class represents a minimal version of the Function driver framework.
This will usually manage more than one device but only one device can currently be supported.
The actual FDF has well defined responsibilities but this actor FDF will be controlled by the
current test case running.
*/
class CActorFDF : public CActive
	{
public:

/**
Factory construction of the acting Function Driver Framework
@param aNotifier the notifier 
*/
	static CActorFDF* NewL(MUsbBusObserver& aObserver);
	virtual ~CActorFDF();

	void Monitor();
	
	CUsbTestDevice& DeviceL(TUint aDeviceHandle);
	
private:
	CActorFDF(MUsbBusObserver& aObserver);
	void ConstructL();
		
private:
	void DoCancel();
	void RunL();
	TInt RunError(TInt aError);


private:
	RUsbHubDriver iDriver;
	RHashMap<TUint,CUsbTestDevice*> iDevices;
	RUsbHubDriver::TBusEvent iBusEvent;
	MUsbBusObserver& iObserver;	
	RProcess iOtgUsbMan;	 	
	};	
	
	
	}


#endif