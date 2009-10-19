#ifndef __TEST_CASE_PBASE_T_USBDI_0491_H
#define __TEST_CASE_PBASE_T_USBDI_0491_H

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
* @file PBASE-T_USBDI-0491.h
* @internalComponent
* 
*
*/



 
#include "BaseBulkTestCase.h"


namespace NUnitTesting_USBDI
	{

/**

	
	@SYMTestCaseID				PBASE-T_USBDI-0491
	@SYMTestCaseDesc			Test for bulk transfers requiring more than one packet
	@SYMFssID 
	@SYMPREQ					1305
	@SYMREQ						7055 [USBD : Bulk transfers]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Open interfaces to connected device
								2. Request a bulk out transfer
								3. Request a bulk in transfer (expecting data just transferred to peripheral to be sent back)
								4. Validate round trip data
	@SYMTestExpectedResults 	Round trip transfer data is not corrupt
	@SYMTestStatus				Draft
	

*/
	class CUT_PBASE_T_USBDI_0491 :  public CBaseBulkTestCase,
	public MTransferObserver,
	public MCommandObserver
	{
public:
	static CUT_PBASE_T_USBDI_0491* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0491(); 

public: // From MUsbBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);

public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);

public: // From MTransferObserver
	void TransferCompleteL(TInt aTransferId,TInt aCompletionCode);

private:
	CUT_PBASE_T_USBDI_0491(TBool aHostRole);
	void ConstructL();

private:
	
	enum TCaseStep
		{
		EInProgress,
		EPassed,
		EFailed,
		ETransferIn,
		ETransferOut
		};

	TCaseStep iCaseStep;

private:
	/**
	The functor for this test case
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0491,TBool> iFunctor;
	};

	} //end namespace

#endif
