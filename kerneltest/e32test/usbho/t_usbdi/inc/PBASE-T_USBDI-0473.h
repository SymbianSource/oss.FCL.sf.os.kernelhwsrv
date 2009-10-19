#ifndef __TEST_CASE_PBASE_T_USBDI_0473_H
#define __TEST_CASE_PBASE_T_USBDI_0473_H

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
* @file PBASE-T_USBDI-0473.h
* @internalComponent
* 
*
*/




#include "BaseTestCase.h"
#include "TestCaseFactory.h"
#include <d32usbdi.h>
#include <d32usbdi_hubdriver.h>
#include "FDFActor.h"
#include "modelleddevices.h"
#include "controltransferrequests.h"
#include "basicwatcher.h"

namespace NUnitTesting_USBDI
	{

/**

	
	@SYMTestCaseID				PBASE-T_USBDI-0473
	@SYMTestCaseDesc			Device suspension, Resumption and remote wakeup (multiple interfaces)
	@SYMFssID 
	@SYMPREQ					1782
	@SYMREQ						7074 [USBD : Device suspension]
								7075 [USBD : Device suspension: multiple interfaces]
								7076 [USBD : Device resume]
								7077 [USBD : Remote wakeup]
								7078 [USBD : Suspend following a wake up]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Suspend a connected device through its interfaces
								2. Resume a interface
								3. Once device is resumed, enable remote wake-up and suspend all interfaces again 
								4. Remote wake up device
								5. Following a remote wake-up, suspend device again
	@SYMTestExpectedResults 	Correct suspension and device resumption can be established
	@SYMTestStatus				Implemented
	

*/	
class CUT_PBASE_T_USBDI_0473 : public CBaseTestCase, public MUsbBusObserver, public MCommandObserver
	{
public:
	static CUT_PBASE_T_USBDI_0473* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0473(); 

	static TInt Interface0ResumedL(TAny* aPtr);
	static TInt Interface1ResumedL(TAny* aPtr);
	TInt CheckForAllResumedNotificationsAndContinueFSM();
	TInt ContinueFSMAfterAllResumedNotifications();
	
private: 
	enum TCaseSteps
		{
		EStepSuspend,
		EValidateSuspendingInterfaces,
		EValidateResumptionAfterInterfaceSuspension,
		ESuspendForRemoteWakeup,
		EValidateSuspendForRemoteWakeup,
		EValidateResumptionAfterWakeup,
		EValidateSuspendAfterWakeup,
		EFailed,
		EPassed
		};

public: // From MUsbBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);
	void DeviceRemovedL(TUint aDeviceHandle);
	void BusErrorL(TInt aError);
	void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
		TInt aCompletionCode);
	
public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);

private:
	CUT_PBASE_T_USBDI_0473(TBool aHostRole);
	void ConstructL();
	void ExecuteHostTestCaseL();
	void ExecuteDeviceTestCaseL();
	void HostDoCancel();
	void DeviceDoCancel();
	void HostRunL();
	void DeviceRunL();

private:

	CActorFDF* iActorFDF;
	CEp0Transfer* iControlEp0;
	RUsbInterface iUsbInterface0;
	RUsbInterface iUsbInterface1;
	CInterfaceWatcher* iInterface0Watcher;
	CInterfaceWatcher* iInterface1Watcher;
	TBool iSuspendedI0;
	TBool iSuspendedI1;	
	TUint iDeviceHandle;

	TCaseSteps iCaseStep;
	
	RUsbDeviceA* iTestDevice;
	RUsbDevice::TDeviceState iStoredNewState;
	TBool iDeviceNotificationPending;
private:
	/**
	The functor for this test case for the factory
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0473,TBool> iFunctor;
	};

	}
	
#endif
