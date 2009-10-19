#ifndef __TEST_CASE_PBASE_T_USBDI_0478_H
#define __TEST_CASE_PBASE_T_USBDI_0478_H

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
* @file PBASE-T_USBDI-0478.h
* @internalComponent
* 
*
*/



 
#include "BaseTestCase.h"
#include "TestCaseFactory.h"
#include "modelleddevices.h"
#include "controltransferrequests.h"
#include "FDFActor.h"
#include <d32usbc.h>
#include <e32debug.h>
#include <d32usbdi.h>

namespace NUnitTesting_USBDI
	{

/**

	
	@SYMTestCaseID				PBASE-T_USBDI-0478
	@SYMTestCaseDesc			Sequential Interface handle opening
	@SYMFssID 
	@SYMPREQ					1782
	@SYMREQ						7066 [USBD : Interface access: Two attempted concurrent opens]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Open all interfaces on a connected device
								2. Establish that a duplicate handle to an interface cannot be opened
								   sequentially
	@SYMTestExpectedResults 	Error from opening an interface twice
	@SYMTestStatus				Implemented
	

*/
class CUT_PBASE_T_USBDI_0478 : public CBaseTestCase, public MUsbBusObserver, public MCommandObserver
	{
public:
	static CUT_PBASE_T_USBDI_0478* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0478(); 

public: // From MUsbBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);
	void DeviceRemovedL(TUint aDeviceHandle);
	void BusErrorL(TInt aError);
	void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
		TInt aCompletionCode);
		
public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);
	
private:
	CUT_PBASE_T_USBDI_0478(TBool aHostRole);
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
	RUsbInterface iUsbInterface0;
	RUsbInterface iUsbInterface1;
	RUsbInterface iDuplicateUsbInterface1;
	CEp0Transfer* iControlEp0;
	TCaseStep iCaseStep;
	
	RUsbDeviceA* iTestDevice;

private:
	/**
	The functor for this test case for the factory
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0478,TBool> iFunctor;
	};

	}


#endif
