#ifndef __USB_CLIENT_STATE_WATCHER_H
#define __USB_CLIENT_STATE_WATCHER_H

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
* @file UsbClientStateWatcher.h
* @internalComponent
* 
*
*/



#include <e32base.h>
#include <d32usbc.h>
#include "testdebug.h"

namespace NUnitTesting_USBDI
	{
	
/**
This class describes an interface class that observers state changes in the client
*/
class MUsbClientStateObserver
	{
public:
/**
Called when the host has changed the state of the client device (e.g. to suspended)
@param aNewState the new state of the device configured by the host
@param aChangeCompletionCode the request completion code
*/
	virtual void StateChangeL(TUsbcDeviceState aNewState,TInt aChangeCompletionCode) = 0;
	};

/**
This class describes an observer that gets notified when the host selects an alternate setting
*/
class MAlternateSettingObserver
	{
public:
/**
Called when the host selects an alternate interface setting
@param aAlternateInterfaceSetting the alternate interface setting number
*/
	virtual void AlternateInterfaceSelectedL(TInt aAlternateInterfaceSetting) = 0;
	};

/**
This class represents a watcher of USB client device states
@internal
*/
class CUsbClientStateWatcher : public CActive
	{
public:
/**
Factory two-phase construction
@param aUsbClientDriver a referrence to a USB client driver interface object
*/
	static CUsbClientStateWatcher* NewL(RDevUsbcClient& aClientDriver,MUsbClientStateObserver& aStateObserver);

/**
Destructor
*/
	~CUsbClientStateWatcher();
	
private:

/**
Constructor, builds a USB client state change watcher
@param aUsbClientDriver the referrence to the USB client driver
*/
	CUsbClientStateWatcher(RDevUsbcClient& aClientDriver,MUsbClientStateObserver& aStateObserver);

/**
2nd phase construction
*/
	void ConstructL();

private: // from CActive

/**
Cancels the notification of device state changes
*/
	void DoCancel();
	
/**
Handles any Leave errors from this AO as it receives notification
of state changes
@param aError the leave code error from RunL
@return KErrNone (currently)
*/
	TInt RunError(TInt aError);
	
/**
Code that is scheduled when this AO completes
*/
	void RunL();

private:
/**
The referrence to the USB client driver
*/
	RDevUsbcClient& iClientDriver;
	
/**
The current state of the USB device (integer value)
*/
	TUint iState;

/**
The observers for the state of the USB client
*/	
	MUsbClientStateObserver& iStateObserver;
	};
	
	
/**
This class represents a watcher of Alternate interface selections
@intenal
*/
class CAlternateInterfaceSelectionWatcher : public CActive
	{
public:
/**
Symbian construction of a watcher of alternate interface setting selections
@param aClientDriver the channel to the client driver
@param aObserver the observer of alternate interface setting selections
*/
	static CAlternateInterfaceSelectionWatcher* NewL(RDevUsbcClient& aClientDriver,MAlternateSettingObserver& aObserver);

/**
Destructor
*/
	~CAlternateInterfaceSelectionWatcher();

private:

/**
Constructor, builds an object that watchers for selections of alternate interface settings
@param aClientDriver the channel to the client driver
@param aObserver the observer of alternate interface setting selections
*/
	CAlternateInterfaceSelectionWatcher(RDevUsbcClient& aClientDriver,MAlternateSettingObserver& aObserver);

/**
2nd phase construction
*/
	void ConstructL();

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
The referrence to the USB client driver
*/
	RDevUsbcClient& iClientDriver;
		
/**
The current state of the USB device (integer value)
*/
	TUint iState;
		
/**
The observer that will be notified when a host selects any alternate interface setting 
that this watcher knows about
*/
	MAlternateSettingObserver& iObserver;
	};

	}

#endif

