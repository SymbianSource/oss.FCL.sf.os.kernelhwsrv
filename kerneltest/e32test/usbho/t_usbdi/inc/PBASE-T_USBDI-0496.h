#ifndef __TEST_CASE_PBASE_T_USBDI_0496_H
#define __TEST_CASE_PBASE_T_USBDI_0496_H

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
* @file PBASE-T_USBDI-0496.h
* @internalComponent
* 
*
*/



 
#include "BaseBulkTestCase.h"


namespace NUnitTesting_USBDI
	{
const TUint8 KNumInitialRequests	= 4;
/**

	
	@SYMTestCaseID				PBASE-T_USBDI-0496
	@SYMTestCaseDesc			Test use of two queued bulk IN transfers to retrieve large amount of data
	@SYMFssID 
	@SYMPREQ					1305
	@SYMREQ						7055 [USBD : Bulk transfers]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Open interfaces to connected device
								2. Request two bulk IN transfers
								3. When each completes request another one until specified number of bytes read
								4. Validate data as it is read
	@SYMTestExpectedResults 	Round trip transfer data is not corrupt
	@SYMTestStatus				Draft
	

*/

class CUT_PBASE_T_USBDI_0496 :  public CBaseBulkTestCase,
								public MTransferObserver,
								public MCommandObserver
	
	{
public:
	static CUT_PBASE_T_USBDI_0496* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0496(); 

public: // From MUsbBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);

public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);

public: // From MTransferObserver
	void TransferCompleteL(TInt aTransferId,TInt aCompletionCode);

private:
	CUT_PBASE_T_USBDI_0496(TBool aHostRole);
	void ConstructL();
	void KillTransfers();
	TUint8 Index(TUint8 aTransferId);
	TInt ValidatePreviousAndPerformNextTransfers(TInt aTransferId);

private:
	
	enum TCaseStep
		{
		EInProgress,
		EPassed,
		EFailed,
		ERequestRepeatedWrite,
		ETransfer
		};

		TUint iNumBytesExpected[KMaxNumInTransfers];
		TUint iValidationStringStartPointTransfer[KMaxNumInTransfers];
		TInt iExpectedNextTransferNumber;
		TUint iNumBytesRequestedSoFar;
		TBool iIsValid;
		TBool iIsTransferError;
		TCaseStep iCaseStep;
		TInt iCountWaitForRequestShortfall;

private:
	/**
	The functor for this test case
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0496,TBool> iFunctor;
	};

	} //end namespace

#endif
