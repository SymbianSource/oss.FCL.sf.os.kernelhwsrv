#ifndef __TEST_CASE_PBASE_T_USBDI_0499_H
#define __TEST_CASE_PBASE_T_USBDI_0499_H

/*
* Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* @file PBASE-T_USBDI-0499.h
* @internalComponent
* 
*
*/



 
#include "BaseBulkTestCase.h"


namespace NUnitTesting_USBDI
	{
/**

	
	@SYMTestCaseID				PBASE-T_USBDI-0499
	@SYMTestCaseDesc			Test resilience to halting an endpoint whilst performing a bulk OUT transfer
	@SYMFssID 
	@SYMPREQ					1305
	@SYMREQ						7055 [USBD : Bulk transfers]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Open interfaces to connected device
								2. Request bulk OUT transfer
								3. Client halts endpoint
								4. Host clears halt on endpoint (via control transfer request)
								5. Request second bulk OUT transfer
								6. Validate data from second bulk OUT transfer.
	@SYMTestExpectedResults 	Round trip transfer data is not corrupt
	@SYMTestStatus				Draft
	@SYMTestExpectedResults 	Round trip transfer data is not corrupt
	@SYMTestStatus				Draft
	

*/

class CUT_PBASE_T_USBDI_0499 :  public CBaseBulkTestCase,
								public MTransferObserver,
								public MCommandObserver
	
	{
public:
	static CUT_PBASE_T_USBDI_0499* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0499(); 

public: // From MUsbBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);

public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);

public: // From MTransferObserver
	void TransferCompleteL(TInt aTransferId,TInt aCompletionCode);

private:
	CUT_PBASE_T_USBDI_0499(TBool aHostRole);
	void ConstructL();

private:
	
	enum TCaseStep
		{
		EInProgress,
		ETransferOutHalt,
		EAwaitClearPreHalt,
		EAwaitCancelRead,
		ETransferOut,
		EPassed,
		EFailed,
		ETransferIn
		};

		TCaseStep iCaseStep;

private:
	/**
	The functor for this test case
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0499,TBool> iFunctor;
	};

	} //end namespace

#endif
