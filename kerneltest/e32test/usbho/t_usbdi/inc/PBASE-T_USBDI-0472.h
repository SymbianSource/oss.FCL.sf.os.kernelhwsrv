#ifndef __TEST_CASE_PBASE_T_USBDI_0472_H
#define __TEST_CASE_PBASE_T_USBDI_0472_H

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
* @file PBASE-T_USBDI-0472.h 
* @internalComponent
* 
*
*/




#include "BaseTestCase.h"
#include "TestCaseFactory.h"
#include <d32usbdi.h>
#include "modelleddevices.h"
#include "FDFActor.h"
#include "controltransferrequests.h"

namespace NUnitTesting_USBDI
	{

/**

	
	@SYMTestCaseID				PBASE-T_USBDI-0472
	@SYMTestCaseDesc			Test for device attachement notification
	@SYMFssID 
	@SYMPREQ					1782
	@SYMREQ						7072 [USBD : Device attachment notification]
								7073 [USBD : Device removal notification]
								7058 [USBD : Descriptor access]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Monitor for USB device connections
								2. Once a device is connected wait for notification of removal
								3. Monitor for device connection again and then succesfully cancel
								   the notification.
	@SYMTestExpectedResults 	Notification of correct device attached
	@SYMTestStatus				Implemented
	

*/	
class CUT_PBASE_T_USBDI_0472 : public CBaseTestCase, public MUsbBusObserver, public MCommandObserver
	{
public:
	static CUT_PBASE_T_USBDI_0472* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0472(); 

public: // From MDeviceNotifier
	void DeviceInsertedL(TUint aDeviceHandle);
	void DeviceRemovedL(TUint aDeviceHandle);
	void BusErrorL(TInt aError);
	void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
		TInt aCompletionCode);

public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);

private:
	CUT_PBASE_T_USBDI_0472(TBool aHostRole);
	void ConstructL();
	void ExecuteHostTestCaseL();
	void ExecuteDeviceTestCaseL();
	void HostDoCancel();
	void DeviceDoCancel();
	void HostRunL();
	void DeviceRunL();
	
private:
	enum TCaseSteps 
		{
		EConnectDevice,
		ERemoveDevice,
		EConnectCancelled,
		EPassed,
		EFailed
		};
		
	RUsbDeviceVendor* iTestDevice;
	CActorFDF* iActorFDF;
	RUsbInterface iInterface0;
	CEp0Transfer* iClientAction;
	TCaseSteps iCaseStep;
	
private:
	/**
	The functor for this test case for the factory
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0472,TBool> iFunctor;
	};

	}
	
#endif // __TEST_CASE_PBASE_T_USBDI_0472_H