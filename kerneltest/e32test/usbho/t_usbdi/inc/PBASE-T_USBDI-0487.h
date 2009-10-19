#ifndef __TEST_CASE_PBASE_T_USBDI_0487_H
#define __TEST_CASE_PBASE_T_USBDI_0487_H

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
* @file PBASE-T_USBDI-0487.h
* @internalComponent
* 
*
*/


 
#include "basetestcase.h"
#include "testcasefactory.h"
#include "modelleddevices.h"
#include "FDFActor.h"
#include <d32usbc.h>
#include <e32debug.h>
#include <d32usbdi.h>
#include "hosttransfers.h"
#include "controltransferrequests.h"

namespace NUnitTesting_USBDI
	{

/**

	
	@SYMTestCaseID				PBASE-T_USBDI-0487
	@SYMTestCaseDesc			Validation of interrupt transfers
	@SYMFssID 
	@SYMPREQ					1782
	@SYMREQ						7054 [USBD : Interrupt transfers]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Open interfaces on connected device
								2. Select alternate interface that has interrupt in endpoint
								3. Open pipe to interrupt in endpoint and queue interrupt transfer
								4. Send request to device to send data payload through interrupt in endpoint
								5. Receive data payload sent through interrupt transfer
	@SYMTestExpectedResults 	Correct loop-back data received
	@SYMTestStatus				Implemented
	

*/
class CUT_PBASE_T_USBDI_0487 : public CBaseTestCase, public MUsbBusObserver, public MTransferObserver, public MCommandObserver
	{
public:
	static CUT_PBASE_T_USBDI_0487* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0487(); 

public: // From MUsbBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);
	void DeviceRemovedL(TUint aDeviceHandle);
	void BusErrorL(TInt aError);
	void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
	TInt aCompletionCode);
	
public: // From MTransferObserver
	void TransferCompleteL(TInt aTransferId,TInt aCompletionCode);

public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);
	
private:
	CUT_PBASE_T_USBDI_0487(TBool aHostRole);
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
		EFailed,
		EPassed
		};
		
	CActorFDF* iActorFDF;
	CEp0Transfer* iControlEp0;
	
	RUsbInterface iUsbInterface0;
	RUsbInterface iUsbInterface1;
	RUsbPipe iInPipe;
	RUsbPipe iOutPipe;
	CInterruptTransfer* iTransferIn;
	CInterruptTransfer* iTransferIn2;
	CInterruptTransfer* iTransferIn3;
	
	// The current test case step
	TCaseStep iCaseStep;
	
	// The test device for this test case
	RUsbDeviceA* iTestDevice;
	
private:
	/**
	The functor for this test case for the factory
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0487,TBool> iFunctor;
	};
	
	}


#endif
