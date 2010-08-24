#ifndef __ENDPOINT_STALL_WATCHER_H
#define __ENDPOINT_STALL_WATCHER_H

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
* @file BaseTestCase.h
* @internalComponent
* 
*
*/



#include <d32usbc.h>
#include <e32base.h>
#include "testdebug.h"


namespace NUnitTesting_USBDI
	{
	
/**
*/
class CEndpointStallWatcher : public CActive
	{
public:
	/**
	C++ constructor
	*/
	CEndpointStallWatcher(RDevUsbcClient& aClientDriver)
	:	CActive(EPriorityUserInput),
		iClientDriver(aClientDriver),
		iEpMask(0)
		{
		CActiveScheduler::Add(this);
		RequestNotification();
		}
	
	/**
	Destructor
	*/
	~CEndpointStallWatcher()
		{
		Cancel();
		}
	
protected:
	/**
	*/
	void DoCancel()
		{
		iClientDriver.EndpointStatusNotifyCancel();
		}
		
	/**
	*/
	void RunL()
	/*
	This is only called if the host alters the stall status of an endpoint.
	It will NOT be called if the client\peripheral has altered the stall ststus
	of an endpoint.
	*/
		{
		TUint epMask = iEpMask;

		if(iStatus.Int() != KErrNone)
			/*
			...for example a reset has occurred.
			The EP Mask will not be filled.
			*/
			{
			RequestNotification();
			return;
			}

		RDebug::Printf("The HOST has halted or cleared a halt on an endpoint.");
		if(epMask==0)
			{
			RDebug::Printf("Now NO endpoints are stalled!");
			RequestNotification();
			return;
			}
			
		_LIT(KStalledEndpoints, "Currently Stalled Endpoints: ");
		_LIT(KComma, ", ");
		TUint KLSB = 0x01;
		TBuf<128> msg(KStalledEndpoints);
		for(TUint8 count = 1; count <= KMaxEndpointsPerClient; count++)
			//Notifier does not notify for EP0, so count from EP1
			//up to max endpoints per interface allowed by the Symbian client
			{
			epMask>>=1;
			if(epMask & KLSB)
				{
				msg.AppendNum(count);
				msg.Append(KComma);
				}
			}
		RDebug::Print(msg);
		RequestNotification();
		return;
		}
		
	/**
	*/
	TInt RunError(TInt aError)
		{
		return KErrNone;
		}

private:
	void RequestNotification()
		{
		iClientDriver.EndpointStatusNotify(iStatus,iEpMask);
		SetActive();
		}
private:
	/**
	The channel to the client driver
	*/
	RDevUsbcClient& iClientDriver;	
	
	/**
	*/
	TUint iEpMask;	
	};
	

	}
	

#endif



