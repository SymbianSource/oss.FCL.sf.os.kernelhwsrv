#ifndef __TEST_CASE_PBASE_T_USBDI_1231_H
#define __TEST_CASE_PBASE_T_USBDI_1231_H

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
* @file PBASE-T_USBDI-1231.h
* @internalComponent
* 
*
*/




#include "BaseTestCase.h"
#include "TestCaseFactory.h"
#include <d32usbdi.h>
#include <d32usbdi_hubdriver.h>
#include "FDFActor.h"
#include "modelleddevices.h"
#include "controltransferrequests.h"
#include "basicwatcher.h"
#include "basebulktestcase.h" // use the bulktranfertimer.
#include "hosttransfers.h"

namespace NUnitTesting_USBDI
	{

/**
	
	@SYMTestCaseID				PBASE-T_USBDI-1231
	@SYMTestCaseDesc			Device suspension immediately resume 
	@SYMFssID                   DEF126984
	@SYMPREQ					BR2609

	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. suspend device and immediately resume.
	@SYMTestExpectedResults 	Correct suspension and device resumption can be established
	@SYMTestStatus				Implemented	

*/	

	class CBulkTransfer;

	class CUT_PBASE_T_USBDI_1231 : public CBaseTestCase,
		public MUsbBusObserver, public MCommandObserver
		
		{
public:
		static CUT_PBASE_T_USBDI_1231* NewL(TBool aHostRole);
		~CUT_PBASE_T_USBDI_1231();

		static TInt Interface0ResumedL(TAny* aPtr);
		static TInt Interface1ResumedL(TAny* aPtr);

private:
		enum TCaseSteps
			{
			EInProcess,
			EResumeWhenSuspending,
			EValidDeviceSuspend,
			EValidDeviceResume,
			EValidResumeWhenSuspending,
			EFailed,
			EPassed
			};

		enum TDeviceInsertedError
			{
			ENone,
			EError,
			EFatalError,
			};

public:
		// From MUsbBusObserver
		void DeviceInsertedL(TUint aDeviceHandle);
		void DeviceRemovedL(TUint aDeviceHandle);
		void BusErrorL(TInt aError);
		void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
				RUsbDevice::TDeviceState aNewState, TInt aCompletionCode);

public:
		// From MCommandObserver
		void Ep0TransferCompleteL(TInt aCompletionCode);


	
private:
		CUT_PBASE_T_USBDI_1231(TBool aHostRole);
		void ConstructL();
		void ExecuteHostTestCaseL();
		void ExecuteDeviceTestCaseL();
		void HostDoCancel();
		void DeviceDoCancel();
		void HostRunL();
		void DeviceRunL();

	
private:

		void ResumeWhenSuspending();	
		void SendEp0Request(); // send control request
		

private:

		CActorFDF* iActorFDF;
		CEp0Transfer* iControlEp0;
		RUsbInterface iUsbInterface0;
		RUsbInterface iUsbInterface1;
		CInterfaceWatcher* iInterface0Watcher;
		CInterfaceWatcher* iInterface1Watcher;
		TUint iDeviceHandle;
		
		RUsbDeviceA* iTestDevice;					
		TCaseSteps iCaseStep;

		TThreadPriority iPriority;
		
public:
        TBool iInterface0Resumed;
		
private:
		/**
		 The functor for this test case for the factory
		 */
		const static TFunctorTestCase<CUT_PBASE_T_USBDI_1231,TBool> iFunctor;
		};

	}
	
#endif
