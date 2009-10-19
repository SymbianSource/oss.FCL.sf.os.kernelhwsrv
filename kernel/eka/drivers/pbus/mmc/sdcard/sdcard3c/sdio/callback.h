// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Class definitions for SDIO Callback objects
// 
//

/**
 @file callback.h
 @internalTechnology
*/

#ifndef __CALLBACK_H__
#define __CALLBACK_H__

#include <e32cmn.h>
#include <drivers/pbus.h>

class DMMCSocket;

enum TSDIOFunctionCallbackReason
/**
 	@publishedPartner
	@released
     
    SDIO Function Callback reason codes
	These are passed to the clients callback function
	@see TSDIOFunction::RegisterClient
*/
	{
	/** Card has powered up */
	ESdioNotifyPowerUp			  = 0,
	/** Card requests to power down */
	ESdioNotifyPowerDownPending	  = 1,
	/** Card has powered down */
	ESdioNotifyPowerDown		  = 2,
	/** Request to enter sleep mode */
	ESdioNotifyPowerSleep		  = 3,
	/** Emergency power down */
	ESdioNotifyEmergencyPowerDown = 4,
	/** PSU fault */
	ESdioNotifyPsuFault			  = 5,
	/** Card has been removed */
	ESdioNotifyCardRemoved		  = 6,
	/** Card has been inserted */
	ESdioNotifyCardInserted		  = 7,
	};

typedef TInt (*TSDIOCallbackFunction)(TAny* aPtr, TSDIOFunctionCallbackReason aReason);

class TSDIOFunctionCallback
/** SDIO Function Notification Callback
	Registered with TSDIOFunction::RegisterClient, and provides clients of the
	function with events that occur on the bus (such as card removal, power down etc..)
	@see TSDIOFunction::RegisterClient
 */
	{
public:
	inline  TSDIOFunctionCallback();

	inline ~TSDIOFunctionCallback();

	inline TSDIOFunctionCallback(TSDIOCallbackFunction aCallbackFn);

	inline TSDIOFunctionCallback(TSDIOCallbackFunction aCallbackFn, TAny* aPtr);
	
	IMPORT_C void Register(DMMCSocket* aSocketP);

	inline TInt CallBack(TSDIOFunctionCallbackReason aReason) const;

private:
	static void NotificationCallBack(TAny* aPtr, TInt aReason, TAny* a1, TAny* a2);
	
	void DoStateChange(TPBusState aNewState, TInt aReasonCode);

public:
	TSDIOCallbackFunction iCallbackFn;	// Clients callback function
	TAny* iPtr;							// Clients user data

private:	
	TPBusCallBack iCallBack;			// Master notification callback
	TPBusState iBusState;				// Current power/card state
	
private:
    //
    // Reserved members to maintain binary compatibility
    TInt iReserved[2];	
	};

#include <drivers/sdio/callback.inl>

#endif	// #ifndef __CALLBACK_H__
