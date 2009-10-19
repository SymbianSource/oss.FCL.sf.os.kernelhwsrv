#ifndef __TEST_CASE_PBASE_T_USBDI_0484_H
#define __TEST_CASE_PBASE_T_USBDI_0484_H

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
* @file PBASE-T_USBDI-0484.h
* @internalComponent
* 
*
*/



 
#include "BaseBulkTestCase.h"
#include "TestCaseFactory.h"
#include "FDFActor.h"
#include "modelleddevices.h"
#include "controltransferrequests.h"
#include "hosttransfers.h"
#include <d32usbc.h>
#include <d32usbdi.h>
#include <e32debug.h>


namespace NUnitTesting_USBDI
	{

/**

	
	@SYMTestCaseID				PBASE-T_USBDI-0484
	@SYMTestCaseDesc			Test clearing error states (stalls) in transfers
	@SYMFssID 
	@SYMPREQ					1782
	@SYMREQ						7057 [USBD : Clearing error states]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Open interfaces to connected device
								2. Queue a bulk in transfer
								3. While transfer is queued, stall the relervant endpoint
								4. Acknowledge the stall in the transfer
	@SYMTestExpectedResults 	Bulk in transfer completes with stall
	@SYMTestStatus				Implemented
	

*/
class CUT_PBASE_T_USBDI_0484 : public CBaseBulkTestCase,
							   public MTransferObserver,
							   public MCommandObserver
	{
public:
	static CUT_PBASE_T_USBDI_0484* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0484(); 

public: // From MUsbBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);
	
public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);
	
public: // From MTransferObserver
	void TransferCompleteL(TInt aTransferId,TInt aCompletionCode);

private:
	CUT_PBASE_T_USBDI_0484(TBool aHostRole);
	void ConstructL();

private:
	
	enum TCaseStep
		{
		EInProgress,
		EPassed,
		EFailed,
		EStalled,
		ETransferAfterStall
		};

	TCaseStep iCaseStep;

private:
	/**
	The functor for this test case
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0484,TBool> iFunctor;
	};

	}


#endif
