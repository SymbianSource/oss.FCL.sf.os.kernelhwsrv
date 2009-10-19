#ifndef __USB_HOST_DEVICE_H__
#define __USB_HOST_DEVICE_H__

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
@file UsbHostDevice.h
*/

#include <e32usbdi_hubdriver.h>

namespace NUnitTesting_USBDI
	{

/**
*/
class MUsbDeviceRemovalObserver
	{
public:
	/**
	*/
	virtual void UsbDeviceRemovedL(TInt aError) = 0;
	};

/**
This class represents Hosts representation of a USB device
*/
class CUsbHostDevice : public CActive
	{
	/**
	*/
	friend class CUsbDeviceWatcher;
	
public:
	/**
	Constructor, 1st phase building a host representation of a usb device
	*/
	CUsbHostDevice(RUsbHubDriver& aUsbHubDriver);
	
	/**
	Destructor
	*/
	~CUsbHostDevice();

	/**
	Monitors for this device being removed from the host
	@param aUsbDeviceRemovalObserver an observer of device removal
	*/
	void MonitorRemovalL(MUsbDeviceRemovalObserver* aUsbDeviceRemovalObserver);

private: // From CActive
	/**
	*/
	void DoCancel();
	
	/**
	*/
	void RunL();
	
	/**
	*/
	TInt RunError(TInt aError);	

private:
	/**
	2nd phase constructor called by CUsbDeviceWatcher
	*/
	void ConstructL();

private:
	/*
	The host USB device resource
	*/
	RUsbDevice iUsbDevice;
	
	/**
	*/
	MUsbDeviceRemovalObserver* iRemovalObserver;
	};

	}

#endif