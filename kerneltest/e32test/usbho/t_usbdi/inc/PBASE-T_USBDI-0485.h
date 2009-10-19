#ifndef __TEST_CASE_PBASE_T_USBDI_0485_H
#define __TEST_CASE_PBASE_T_USBDI_0485_H

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
* @file PBASE-T_USBDI-0485.h
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

	
	@SYMTestCaseID				PBASE-T_USBDI-0485
	@SYMTestCaseDesc			
	@SYMFssID 
	@SYMPREQ					1782
	@SYMREQ						
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Open interface on connected device
								2. Send a control transfer request that is NAK'd by the USB device
								3. Close the interface that has this request outstanding
								4. Acknowledge transfer cancelled
	@SYMTestExpectedResults 	Outstanding NAK control transfer is cancelled
	@SYMTestStatus				Implemented
	

*/
class CUT_PBASE_T_USBDI_0485 : public CBaseTestCase, public MUsbBusObserver, public MCommandObserver
	{
public:
	static CUT_PBASE_T_USBDI_0485* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0485(); 

public: // From MBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);
	void DeviceRemovedL(TUint aDeviceHandle);
	void BusErrorL(TInt aError);
	void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
			TInt aCompletionCode);

public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);

private:
	CUT_PBASE_T_USBDI_0485(TBool aHostRole);
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
	TUint32 iToken0;

	TCaseStep iCaseStep;
	
	RUsbDeviceVendor* iTestDevice;
	
private:
	/**
	The functor to create this test case from the factory
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0485,TBool> iFunctor;
	};

	}


#endif
