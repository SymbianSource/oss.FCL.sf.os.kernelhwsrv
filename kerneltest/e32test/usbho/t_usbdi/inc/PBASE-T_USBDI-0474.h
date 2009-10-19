#ifndef __TEST_CASE_PBASE_T_USBDI_0474_H
#define __TEST_CASE_PBASE_T_USBDI_0474_H

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
* @file PBASE-T_USBDI-0474.h
* @internalComponent
* 
*
*/



 
#include "BaseTestCase.h"
#include "TestCaseFactory.h"
#include "modelleddevices.h"
#include <d32usbdi.h>
#include <d32usbdi_hubdriver.h>
#include "FDFActor.h"
#include <d32usbc.h>
#include "testdebug.h"
#include "controltransferrequests.h"
 
namespace NUnitTesting_USBDI
	{

/**

	
	@SYMTestCaseID				PBASE-T_USBDI-0474
	@SYMTestCaseDesc			Acceptance of unknown descriptor from USB device.
	@SYMFssID 
	@SYMPREQ					1782
	@SYMREQ						7059 - [USBD : Descriptor access: unknown descriptors]
                                7060 - [USBD : Descriptor access: backward compatibility]
                                
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Enumerate
								2. Read Descriptor
								3. Compare against reference descriptor tree
	@SYMTestExpectedResults 	Descriptor read and processed into tree correctly.
	@SYMTestStatus				Implemented
	

*/	
class CUT_PBASE_T_USBDI_0474 : public CBaseTestCase, public MUsbBusObserver, public MCommandObserver
	{
public:
	static CUT_PBASE_T_USBDI_0474* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0474(); 

public: // From MUsbBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);
	void DeviceRemovedL(TUint aDeviceHandle);
	void BusErrorL(TInt aError);
	void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
		TInt aCompletionCode);

public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);
	
private:
	CUT_PBASE_T_USBDI_0474(TBool aHostRole);
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
	CEp0Transfer* iClientAction;
	RUsbInterface iUsbInterface0;
	RUsbInterface iUsbInterface1;
	RUsbPipe iTestPipe;
	TCaseStep iCaseStep;
	
	RUsbDeviceB* iTestDevice;

private:
	/**
	The functor for this test case for the factory
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0474,TBool> iFunctor;
	};

	
	}

#endif
