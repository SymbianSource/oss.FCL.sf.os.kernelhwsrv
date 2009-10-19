#ifndef __TEST_CASE_PBASE_T_USBDI_0483_H
#define __TEST_CASE_PBASE_T_USBDI_0483_H

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
* @file PBASE-T_USBDI-0483.h
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
#include <e32debug.h>
#include <d32usbdi.h>

namespace NUnitTesting_USBDI
	{

/**

	
	@SYMTestCaseID				PBASE-T_USBDI-0483
	@SYMTestCaseDesc			Large configuration
	@SYMFssID 
	@SYMPREQ					1782
	@SYMREQ						Negative testing
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Enumerate
                                2. Retrieve configuration descriptor again
                                3. Validated
	@SYMTestExpectedResults 	Device connection accepted
	@SYMTestStatus				Implemented
	

*/
class CUT_PBASE_T_USBDI_0483 : public CBaseTestCase, public MUsbBusObserver, public MCommandObserver
	{
public:
	static CUT_PBASE_T_USBDI_0483* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0483(); 

public: // From MUsbBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);
	void DeviceRemovedL(TUint aDeviceHandle);
	void BusErrorL(TInt aError);
	void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
			TInt aCompletionCode);
	
public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);
	
private:
	CUT_PBASE_T_USBDI_0483(TBool aHostRole);
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
	CEp0Transfer* iControlEp0;
	HBufC8* iConfigDescriptorData;
	TCaseStep iCaseStep; 
	// The test device with class-specific descriptors for this test case
	RUsbDeviceB* iTestDevice;

private:
	/**
	The functor to create this test case from the factory
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0483,TBool> iFunctor;
	};

	}


#endif
