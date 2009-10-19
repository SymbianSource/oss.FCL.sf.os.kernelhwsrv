#ifndef __TEST_CASE_PBASE_T_USBDI_0475_H
#define __TEST_CASE_PBASE_T_USBDI_0475_H

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
* @file PBASE-T_USBDI-0475.h
* @internalComponent
* 
*
*/



 
#include "basetestcase.h"
#include "testcasefactory.h"
#include "FDFActor.h"
#include <d32usbc.h>
#include <e32debug.h>
#include "hosttransfers.h"

namespace NUnitTesting_USBDI
	{

/**

	
	@SYMTestCaseID				PBASE-T_USBDI-0475
	@SYMTestCaseDesc			Validation of ill-formed USB descriptors
	@SYMFssID 
	@SYMPREQ					1782
	@SYMREQ						7058 - [USBD : Descriptor access]
                                7060 - [USBD : Descriptor access: backward compatibility]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Parse descriptors using a raw data set.	
                                2. Process descriptor 	
                                3. Compare generated trees to reference ones	
	@SYMTestExpectedResults 	No errors
	@SYMTestStatus				Implemented
	

*/
class CUT_PBASE_T_USBDI_0475 : public CBaseTestCase
	{
public:
	static CUT_PBASE_T_USBDI_0475* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0475(); 

public: // From MUsbBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);
	void DeviceRemovedL(TUint aDeviceHandle);
	void BusErrorL(TInt aError);
	void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
		TInt aCompletionCode);
	
public: // From MTransferObserver
	void TransferCompleteL(TInt aTransferId,TInt aCompletionCode);
	
private:
	CUT_PBASE_T_USBDI_0475(TBool aHostRole);
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
		EPassed,
		EFailed
		};

	TCaseStep iCaseStep;

private:
	/**
	The functor for this test case for the factory
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0475,TBool> iFunctor;
	};

	}


#endif
