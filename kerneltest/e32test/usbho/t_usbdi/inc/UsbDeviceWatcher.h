#ifndef __USB_DEVICE_WATCHER_H__
#define __USB_DEVICE_WATCHER_H__

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
* 
*
*/

/**
@file UsbDeviceWatcher.h
*/

#include <e32base.h>

// Forward declarations

class RUsbHubDriver;

namespace NUnitTesting_USBDI
	{
	
// Forward declarations

class CUsbHostDevice;
	
/**
*/
class MUsbDeviceObserver
	{
public:
	/**
	Called when a USB device has been connected to the host
	@param aError an error code relating to USB device attachment from the Hub driver
	*/
	virtual void UsbDeviceAttachedL(TInt aError) = 0;
	};
	
/**
This class represents a watcher for USB devices inserted into the host
*/
class CUsbDeviceWatcher : public CActive
	{
public:
	/**
	Symbian factory construction
	@return a pointer to an instance of a Usb device watcher
	*/
	static CUsbDeviceWatcher* NewL(RUsbHubDriver& aUsbHubDriver,MUsbDeviceObserver& aUsbDeviceObserver);

	/**
	Destructor
	*/
	~CUsbDeviceWatcher();

	/**
	*/
	void StartWatchingL(CUsbHostDevice* aUsbDevice);

private:
	/**
	Cancel watching for new USB devices
	*/
	void DoCancel();
	
	/**
	
	*/
	void RunL();
	
	/**
	@return KErrNone
	*/
	TInt RunError(TInt aError);

private:
	/**
	Constructor
	@param aUsbHubDriver the Host USB Hub driver
	*/
	CUsbDeviceWatcher(RUsbHubDriver& aUsbHubDriver,MUsbDeviceObserver& aUsbDeviceObserver);
	
	/**
	2nd phase constructor
	*/
	void ConstructL();
	
private:
	/**
	The Usb hub driver (uses-a)
	*/
	RUsbHubDriver& iUsbHubDriver;
	
	/**
	The usb device resource (uses-a)
	*/
	CUsbHostDevice* iUsbHostDevice;
	
	/**
	The observer for USB device connection
	*/
	MUsbDeviceObserver& iUsbDeviceObserver;
	};
	
	
	}


#endif