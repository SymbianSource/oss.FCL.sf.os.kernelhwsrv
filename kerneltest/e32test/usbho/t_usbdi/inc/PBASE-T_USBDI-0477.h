#ifndef __TEST_CASE_PBASE_T_USBDI_0477_H
#define __TEST_CASE_PBASE_T_USBDI_0477_H

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
* @file PBASE-T_USBDI-0477.h
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

	
	@SYMTestCaseID				PBASE-T_USBDI-0477
	@SYMTestCaseDesc			Selection of two alternate interface settings where the second selection
								fails and the interface rolls back
	@SYMFssID 
	@SYMPREQ					1782
	@SYMREQ						7046 [USBD : Changing alternate interface setting]
								7047 [USBD : Roll-back of alternate interface setting]
								7048 [USBD : Default Alternate Interface setting]
								7049 [USBD : Establishment of pipes]
								7050 [USBD : Closing of pipes]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Open interfaces on connected device
								2. Open and close a bulk pipe on default setting 0
								3. Select alternate interface setting 1 and establish a pipe then close it
								4. If alternate interface selection fails, establish that the pipe previously 
								   opened in step 2 can be opened again.
	@SYMTestExpectedResults 	Pipes can be established in either interface setting
	@SYMTestStatus				Implemented
	

*/	
class CUT_PBASE_T_USBDI_0477 : public CBaseTestCase, public MUsbBusObserver, public MCommandObserver
	{
public:
	static CUT_PBASE_T_USBDI_0477* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0477(); 

public: // From MUsbBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);
	void DeviceRemovedL(TUint aDeviceHandle);
	void BusErrorL(TInt aError);
	void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
		TInt aCompletionCode);

public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);
	
private:
	CUT_PBASE_T_USBDI_0477(TBool aHostRole);
	void ConstructL();
	void ExecuteHostTestCaseL();
	void ExecuteDeviceTestCaseL();
	void HostDoCancel();
	void DeviceDoCancel();
	void HostRunL();
	void DeviceRunL();
	TBool CheckFirstInterfaceDescriptorDeviceA(TUsbInterfaceDescriptor& aIfDescriptor);

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
	
	RUsbDeviceA* iTestDevice;

private:
	/**
	The functor for this test case for the factory
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0477,TBool> iFunctor;
	};

	
	}

#endif