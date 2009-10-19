#ifndef __TEST_CASE_PBASE_T_USBDI_1229_H
#define __TEST_CASE_PBASE_T_USBDI_1229_H

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
* @file PBASE-T_USBDI-1229.h
* @internalComponent
* 
*
*/



 
#include "BaseBulkTestCase.h"


namespace NUnitTesting_USBDI
	{
const TUint8 KNumInitialRequests	= 4;
/**

	
	@SYMTestCaseID				PBASE-T_USBDI-1229
	@SYMTestCaseDesc			Test use of two queued bulk OUT transfers to send a large amount of data working together another type of transfer
	@SYMFssID 
	@SYMPREQ					1305
	@SYMREQ						7055 [USBD : Bulk transfers]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Open interfaces to connected device
								2. Request two bulk OUT transfers
								3. When each completes request another one until specified number of bytes have been sent
								4. Whilst bulk transfers are active, send EP0 transfers and check they have priority over bulk transfers
								5. Validate data as it is read
	@SYMTestExpectedResults 	Round trip transfer data is not corrupt
	@SYMTestStatus				Draft
	

*/

class CUT_PBASE_T_USBDI_1229 :  public CBaseBulkTestCase,
								public MTransferObserver,
								public MCommandObserver
	
	{
public:
	static CUT_PBASE_T_USBDI_1229* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_1229(); 

public: // From MUsbBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);

public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);

public: // From MTransferObserver
	void TransferCompleteL(TInt aTransferId,TInt aCompletionCode);
	void HandleBulkTestTimerFired();

private:
	CUT_PBASE_T_USBDI_1229(TBool aHostRole);
	void ConstructL();
	void KillTransfers();
	void ExtractDeviceReadBytes();
	void PostTransferAction();
	TBool PerformNextTransfer(TInt aTransferId);
	void RequestNumBytesSent(TUint8 aTimerIndex);

private:
	
	enum TCaseStep
		{
		EInProgress,
		EPassed,
		EFailed,
		EGetTimerBase,
		ERequestRepeatedReadAndValidate,
		ETransfer,
		EDelayedTransferComplete,
		ERequestPrepareEndpointValidationResult,
		ERequestValidationResult
		};

		TUint iNumWriteBytesRequested;
		TCaseStep iCaseStep;
		
		TUint iDeviceNumBytesReadInTotal;
		TUint iDeviceMinTimedNumBytesRead;
		TUint iDeviceMaxTimedNumBytesRead;

private:
	/**
	The functor for this test case
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_1229,TBool> iFunctor;
	};

	} //end namespace

#endif
