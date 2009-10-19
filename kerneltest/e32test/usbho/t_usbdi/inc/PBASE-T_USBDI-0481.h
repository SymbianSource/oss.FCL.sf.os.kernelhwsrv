#ifndef __TEST_CASE_PBASE_T_USBDI_0481_H
#define __TEST_CASE_PBASE_T_USBDI_0481_H

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
* @file PBASE-T_USBDI-0481.h
* @internalComponent
* 
*
*/



 
#include "BaseTestCase.h"
#include "TestCaseFactory.h"
#include "FDFActor.h"
#include "modelleddevices.h"
#include "controltransferrequests.h"
#include <d32usbc.h>
#include "testdebug.h"

namespace NUnitTesting_USBDI
	{

/**

	
	@SYMTestCaseID				PBASE-T_USBDI-0481
	@SYMTestCaseDesc			Validation of error codes when re-opening interfaces.
	@SYMFssID 
	@SYMPREQ					1782
	@SYMREQ						7066 - [ USBD : Interface access: Two attempted concurrent opens]
                                7126 - [ USBD : Cancellation of transfer]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Enumerate A
                                2. Get the token for interface 0
                                3. Open interface 0
                                4. Get the token for interface 1
                                5. Open interface 1
                                6. Get the token for interface 1 again, validate error code
                                7. Open interface 1 again, validate error code

	@SYMTestExpectedResults 	Error codes validated 
	@SYMTestStatus				Implemented
	

*/
class CUT_PBASE_T_USBDI_0481 : public CBaseTestCase, public MUsbBusObserver, public MCommandObserver
	{
public:
	static CUT_PBASE_T_USBDI_0481* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0481(); 

public: // From MBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);
	void DeviceRemovedL(TUint aDeviceHandle);
	void BusErrorL(TInt aError);
	void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
			TInt aCompletionCode);

public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);

private:
	CUT_PBASE_T_USBDI_0481(TBool aHostRole);
	void ConstructL();
	void ExecuteHostTestCaseL();
	void ExecuteDeviceTestCaseL();
	void HostDoCancel();
	void DeviceDoCancel();
	void HostRunL();
	void DeviceRunL();

private:

	enum TCaseStep
		{
		EInProgress,
		EFailed,
		EPassed
		};

	CActorFDF* iActorFDF;
	
	CEp0Transfer* iControlEp0;
	RUsbInterface iUsbInterface0;
	RUsbInterface iUsbInterface1;
	TUint32 iToken0;

	TCaseStep iCaseStep;
	
	RUsbDeviceA* iTestDevice;
	
private:
	/**
	The functor to create this test case from the factory
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0481,TBool> iFunctor;
	};

	}


#endif
