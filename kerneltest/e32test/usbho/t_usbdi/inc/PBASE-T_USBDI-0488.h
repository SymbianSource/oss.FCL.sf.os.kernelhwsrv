#ifndef __TEST_CASE_PBASE_T_USBDI_0488_H
#define __TEST_CASE_PBASE_T_USBDI_0488_H

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
* @file PBASE-T_USBDI-0488.h
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

	
	@SYMTestCaseID				PBASE-T_USBDI-0488
	@SYMTestCaseDesc			Resource cleanup following panic in client.
	@SYMFssID 
	@SYMPREQ					1782
	@SYMREQ						7065 - [ USBD : Closing interface handles: Unclean shutdown (ie. kernel closes handle]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Enumerate
                                2. Add heap mark
                                3. Select alternate setting 1
                                4. Open pipe for endpoint on interface 1
                                5. Select alternate setting 0
                                6. Validate panic cause (Interface setting changed while pipe is opened on other alternate setting)
                                7. Close interface 1
                                8. Check heap mark is equal to original mark
    @SYMTestExpectedResults     Resources successfully cleaned up following a specific panic.	
	@SYMTestStatus				Implemented
	

*/
class CUT_PBASE_T_USBDI_0488 : public CBaseTestCase, public MUsbBusObserver, public MCommandObserver
	{
public:
	static CUT_PBASE_T_USBDI_0488* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0488(); 

public: // From MUsbBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);
	void DeviceRemovedL(TUint aDeviceHandle);
	void BusErrorL(TInt aError);
	void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
	TInt aCompletionCode);
	

public: // From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);
	
private:
	CUT_PBASE_T_USBDI_0488(TBool aHostRole);
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
		
public :

	RUsbInterface iUsbInterface1;
	
	
private:	
	
	CEp0Transfer* iControlEp0;
	CActorFDF* iActorFDF;	
	RUsbInterface iUsbInterface0;
	RUsbPipe iInPipe;
	// The current test case step
	TCaseStep iCaseStep;
	
	// The test device for this test case
	RUsbDeviceA* iTestDevice;
	

	
private:
	/**
	The functor for this test case for the factory
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0488,TBool> iFunctor;
	
	/** 
	static method, function of spawned thread.
	@param[in] aTestCase pointer
	*/	 
	static TInt TestSelectAlternateInterfaceThenPanic(TAny* aTest);
	};
	
	}


#endif
