#ifndef __TEST_CASE_PBASE_T_USBDI_0476_H
#define __TEST_CASE_PBASE_T_USBDI_0476_H

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
* @file PBASE-T_USBDI-0476.h
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

	
	@SYMTestCaseID				PBASE-T_USBDI-0476
	@SYMTestCaseDesc			Validation of USB descriptors containing one or more IADs.
	@SYMFssID 
	@SYMPREQ					1782
	@SYMREQ						7058 - [USBD : Descriptor access]
                                7060 - [USBD : Descriptor access: backward compatibility]
	
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Parse descriptors using raw data sets.	
                                2. Process descriptors 	
                                3. Compare generated trees to reference ones.	
	@SYMTestExpectedResults 	No errors
	@SYMTestStatus				Implemented
	

*/
class CUT_PBASE_T_USBDI_0476 : public CBaseTestCase
	{
public:
	static CUT_PBASE_T_USBDI_0476* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0476(); 

public: // From MUsbBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);
	void DeviceRemovedL(TUint aDeviceHandle);
	void BusErrorL(TInt aError);
	void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
		TInt aCompletionCode);
	
public: // From MTransferObserver
	void TransferCompleteL(TInt aTransferId,TInt aCompletionCode);

private:
	CUT_PBASE_T_USBDI_0476(TBool aHostRole);
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
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0476,TBool> iFunctor;
	};

	}


#endif
