#ifndef __TEST_CASE_PBASE_T_USBDI_0486_H
#define __TEST_CASE_PBASE_T_USBDI_0486_H

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
* @file PBASE-T_USBDI-0486.h
* @internalComponent
* 
*
*/



 
#include "BaseBulkTestCase.h"
#include "TestCaseFactory.h"
#include "FDFActor.h"
#include "modelleddevices.h"
#include "controltransferrequests.h"
#include <d32usbc.h>
#include "testdebug.h"
#include "hosttransfers.h"

_LIT(KTestDeviceC_SN, "TestDeviceC - CUT_PBASE_T_USBDI_0486");

namespace NUnitTesting_USBDI
	{

/**

	
	@SYMTestCaseID				PBASE-T_USBDI-0486
	@SYMTestCaseDesc			Validation of opening interfaces after a device change
	@SYMFssID 
	@SYMPREQ					1782
	@SYMREQ						7067 - [ USBD : Interface access: Changed device]
                                7068 - [ USBD : Interface access: Consistency across device addition/removal]
                                7069 - [ USBD : Interface access: Re-open an interface]
                                7126 - [ USBD : Cancellation of transfer]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1.  Enumerate A
                                2.  Get token for interface 0 (token 0) – save token 0
                                3.  Open interface 0 with token 0
                                4.  Get token for interface 1 (token 1)
                                5.  Open interface 1 (token 1)
                                6.  Open pipe on interface 1, endpoint 2
                                7.  Queue bulk in transfer on pipe
                                8.  Vendor dependant request : Disconnect A- Connect C sent to device A
                                9.  Transfer completes with cancelled error code
                                10. Close interface 0
                                11. Enumerate C
                                12. Try to open interface 0 with previous token 0, fails since token is invalid
                                13. Get token for interface 0 (token 2) 
                                14. Open interface 0 (token 2)
                                15. Vendor dependant request : Disconnect C- Connect A sent to device C
                                16. Close interface 0
                                17. Enumerate A
                                18. Get token for interface 0 (token 3)
                                19. Open interface 0 with token 3
	@SYMTestExpectedResults 	Unable to open any interface using expired token, Transfer cancelled on disconnection
	@SYMTestStatus				Implemented
	

*/
class CUT_PBASE_T_USBDI_0486 :   public CBaseBulkTestCase,
								 public MTransferObserver, 
								 public MCommandObserver
	{
public:
	static CUT_PBASE_T_USBDI_0486* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0486(); 

public: // From MBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);
	void DeviceRemovedL(TUint aDeviceHandle);
			
public: // From MTransferObserver
	void TransferCompleteL(TInt aTransferId,TInt aCompletionCode);

public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);

private:
	CUT_PBASE_T_USBDI_0486(TBool aHostRole);
	void ConstructL();
	void ExecuteDeviceTestCaseL();
	void DeviceRunL();
	void DeviceDoCancel();
	TBool CheckSN(const TDesC16& aSerialNumberGot, const TDesC& aExpectedSerialNumber);

private:

	enum TCaseStep
		{
		EFailed,
		EPassed,
		EWaitForDeviceCConnection,
		EDeviceCConnected,
		EDeviceAConnected,
		EWaitForDeviceCDisconnection,
		EWaitForDeviceAConnection		
		};

	TCaseStep iCaseStep;	
	TUint32 iToken0DeviceC;
	RUsbDeviceC* iTestDeviceC;
	
	
public :
	RUsbDeviceC* TestDeviceC();
	RUsbDeviceD* TestDeviceD();
	void HandleDeviceDConnection();

private:
	/**
	The functor to create this test case from the factory
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0486,TBool> iFunctor;
	};

	}


#endif
