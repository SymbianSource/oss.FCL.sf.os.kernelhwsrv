#ifndef __REMOTE_WAKEUP_TIMER_H
#define __REMOTE_WAKEUP_TIMER_H

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
* @file wakeuptimer.h
* @internalComponent
* 
*
*/



#include <e32base.h>
#include "testdevicebase.h"

namespace NUnitTesting_USBDI
	{

/**
*/
class CRemoteWakeupTimer : public CTimer
	{
public:

	/**
	2 phase construction
	@param aTestDevice the test device object that will perform the remote wakeup event
	*/
	
	static CRemoteWakeupTimer* NewL(RUsbTestDevice& aTestDevice);

	/**
	Destructor
	*/
	
	~CRemoteWakeupTimer();

	/**
	Wake-up the test device after the specified interval
	@param aInterval the interval 
	*/
	
	void WakeUp(TUint16 aInterval);

private:
	/**
	*/
	
	CRemoteWakeupTimer(RUsbTestDevice& aTestDevice);

	/**
	*/
	
	void ConstructL();

private: // From CTimer
	/**
	*/
	void RunL();

private:
	/**
	The test device object to instruct to connect of disonnect
	*/
	RUsbTestDevice& iTestDevice;
	};


	}

#endif

