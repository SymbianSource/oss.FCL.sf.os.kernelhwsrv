#ifndef __SOFTWARE_CONNECT_TIMER_H
#define __SOFTWARE_CONNECT_TIMER_H

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
* @file softwareconnecttimer.h
* @internalComponent
* 
*
*/



#include <e32base.h>
#include "testdevicebase.h"

namespace NUnitTesting_USBDI
	{
	
/**
This class represents a timer for performing USB connection and disconnection
in software. i.e. the D+/- pull-ups
*/
	
class CSoftwareConnectTimer : public CTimer
	{
private:
	// The type of connection to be performed
	enum TConnectionType
		{
		EUnknown,
		EConnect,
		EDisconnect
		};
		
public:

	/**
	2 phase construction
	@param aTestDevice a client usb test device
	*/
	
	static CSoftwareConnectTimer* NewL(RUsbTestDevice& aTestDevice);
	
	/**
	Destructor
	*/
	
	~CSoftwareConnectTimer();
	
	/**
	Perform software connection after the specified interval has elapsed
	@param aInterval the time gap before connecting in seconds
	*/
	
	void SoftwareConnect(TInt aInterval);
	
	/**
	Perform software disconnection after the specified interval has elapsed
	@param aInterval the time gap before disconnecting in seconds
	*/	
	
	void SoftwareDisconnect(TInt aInterval);

	/**
	Peform a software disconnection and then after the specified interval, perform a software connection.
	@param aInterval the time gap between disconnecting and connecting in a reconnect step in seconds
	*/
	
	void SoftwareReConnect(TInt aInterval);
	
private:

	/**
	Constructor, build a timer for performing software usb connection and disconnection
	*/
	
	CSoftwareConnectTimer(RUsbTestDevice& aTestDevice);

	/**
	*/
	void RunL();	

private:
	/**
	The test device object to instruct to connect of disonnect
	*/
	
	RUsbTestDevice& iTestDevice;
	
	/**
	The type of connection to be performed
	*/
	
	TConnectionType iConnectType;	
	};
	
	
	
	}
	
	
#endif

	
	