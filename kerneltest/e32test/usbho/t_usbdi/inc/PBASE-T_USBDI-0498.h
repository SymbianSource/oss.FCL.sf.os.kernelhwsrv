#ifndef __TEST_CASE_PBASE_T_USBDI_0498_H
#define __TEST_CASE_PBASE_T_USBDI_0498_H

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
* @file PBASE-T_USBDI-0498.h
* @internalComponent
* 
*
*/



 
#include "BaseBulkTestCase.h"


namespace NUnitTesting_USBDI
	{
const TUint8 KNumOutTransfersPerInterface		= 2; //number of queued OUT transfers in used for each interface
const TUint8 KNumInTransfersPerInterface		= 2; //number of queued IN transfers used for each interface
const TUint8 KNumInitialRequests 				= 4;
/**

	
	@SYMTestCaseID				PBASE-T_USBDI-0498
	@SYMTestCaseDesc			Test use of multiple concurrent bulk transfers on two interfaces
	@SYMFssID 
	@SYMPREQ					1305
	@SYMREQ						7055 [USBD : Bulk transfers]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Open interfaces to connected device
								2. Queue two bulk OUT transfer requests on each interface
								3. Queue two bulk IN transfer requests on each interface
								4. Validate data sent and data received
								5. Compare completion times for each bulk transfer
	@SYMTestExpectedResults 	Round trip transfer data is not corrupt
	@SYMTestStatus				Draft
	

*/

class CUT_PBASE_T_USBDI_0498 :  public CBaseBulkTestCase,
								public MTransferObserver,
								public MCommandObserver
	
	{
public:
	static CUT_PBASE_T_USBDI_0498* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0498(); 

public: // From MUsbBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);

public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);

public: // From MTransferObserver
	void TransferCompleteL(TInt aTransferId,TInt aCompletionCode);

private:
	CUT_PBASE_T_USBDI_0498(TBool aHostRole);
	void ConstructL();

private:
	
	enum TCaseStep
		{
		EInProgress,
		EPassed,
		EFailed,
		ERequestDeviceIFC1Read,
		ERequestDeviceIFC2Read,
		ERequestDeviceIFC1Write,
		ERequestDeviceIFC2Write,
		ETransfer,
		ERequestDeviceValidateIFC1,
		ERequestDeviceValidateIFC2,
		ERequestDeviceValidationResultIFC1,
		ERequestDeviceValidationResultIFC2
		};

	CBulkTransfer* iIfc1InTransfer[KNumInTransfersPerInterface];
	CBulkTransfer* iIfc1OutTransfer[KNumOutTransfersPerInterface];
	
	CBulkTransfer* iIfc2InTransfer[KNumInTransfersPerInterface];
	CBulkTransfer* iIfc2OutTransfer[KNumOutTransfersPerInterface];
		
	TCaseStep iCaseStep;
	TPtr8 iRequestDeviceValidationResultPtr;

private:
	/**
	The functor for this test case
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0498,TBool> iFunctor;
	};

	} //end namespace

#endif
