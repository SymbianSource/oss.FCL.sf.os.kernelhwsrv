#ifndef __TEST_CASE_PBASE_T_USBDI_0480_H
#define __TEST_CASE_PBASE_T_USBDI_0480_H

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
* @file PBASE-T_USBDI-0480.h
* @internalComponent
* 
*
*/


 
#include "basetestcase.h"
#include "testcasefactory.h"
#include "fdfactor.h"
#include "controltransferrequests.h"
#include "modelleddevices.h"
#include "hosttransfers.h"
#include <d32usbc.h>
#include <e32debug.h>
#include <d32usbdi.h>
#include <d32usbtransfers.h>

namespace NUnitTesting_USBDI
	{

/**

	
	@SYMTestCaseID				PBASE-T_USBDI-0480
	@SYMTestCaseDesc			Validation of USB control transfers to/from connected device
	@SYMFssID 
	@SYMPREQ					1782
	@SYMREQ						7053 [USBD : Control transfers]
								7071 [USBD : Querying packet sizes for transfers]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Open interfaces on connected device
								2. Send control transfers to endpoint0
									- Empty control transfers
									- Device and Interface directed (with/without payload)
	@SYMTestExpectedResults 	3. Client USB device providing appropriate response
	@SYMTestStatus				Implemented
	

*/
class CUT_PBASE_T_USBDI_0480 : public CBaseTestCase, public MUsbBusObserver, public MCommandObserver
	{
public:
	static CUT_PBASE_T_USBDI_0480* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0480(); 

public: // From MUsbBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);
	void DeviceRemovedL(TUint aDeviceHandle);
	void BusErrorL(TInt aError);
	void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
		TInt aCompletionCode);
		
public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);

private:
	CUT_PBASE_T_USBDI_0480(TBool aHostRole);
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
		EEmptyDeviceXfer,
		EEmptyInterfaceXfer,
		EDataPutDeviceXfer,
		EDataGetDeviceXfer,
		EDataPutInterfaceXfer,
		EDataGetInterfaceXfer,
		EFailed,
		EPassed
		};

	CActorFDF* iActorFDF;
	CEp0Transfer* iControlEp0;
	RUsbInterface iUsbInterface0;
	RUsbInterface iUsbInterface1;
	TCaseSteps iCaseStep;
	HBufC8* iTemp;
	TPtr8 iPtrTemp;
		
	RUsbDeviceA* iTestDevice;
	
private:
	/**
	The functor for this test case for the factory
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0480,TBool> iFunctor;
	};

	}


#endif
