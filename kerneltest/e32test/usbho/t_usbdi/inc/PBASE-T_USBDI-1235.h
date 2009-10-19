#ifndef __TEST_CASE_PBASE_T_USBDI_1235_H
#define __TEST_CASE_PBASE_T_USBDI_1235_H

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
* @file PBASE-T_USBDI-1235.h
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
#include "basebulktestcase.h" // use the bulktransfertimer.
#include "hosttransfers.h"

namespace NUnitTesting_USBDI
	{

/**
	
	@SYMTestCaseID				PBASE-T_USBDI-1235
	@SYMTestCaseDesc			Cancel immediately after requesting device suspension , do not care the devic's tate is suspend or resume.
	@SYMFssID                   DEF126984
	@SYMPREQ					BR2609

	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			suspend device and immediately cancel suspend request.
	@SYMTestExpectedResults 	Correct suspension and device resumption can be established
	@SYMTestStatus				Implemented	

*/	

	class CBulkTransfer;

	class CUT_PBASE_T_USBDI_1235 : public CBaseTestCase,
		public MUsbBusObserver, public MCommandObserver
		
		{
public:
		static CUT_PBASE_T_USBDI_1235* NewL(TBool aHostRole);
		~CUT_PBASE_T_USBDI_1235();

		static TInt Interface0ResumedL(TAny* aPtr);
		static TInt Interface1ResumedL(TAny* aPtr);

private:
		enum TCaseSteps
			{
			EInProcess,
			ECancelSuspendWhenInterfaceSuspension,
			EValidCancelSuspendAfterInterfaceSuspend,
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

public:
		

public:
	
private:
		CUT_PBASE_T_USBDI_1235(TBool aHostRole);
		void ConstructL();
		void ExecuteHostTestCaseL();
		void ExecuteDeviceTestCaseL();
		void HostDoCancel();
		void DeviceDoCancel();
		void HostRunL();
		void DeviceRunL();

	
private:

		void SuspendDeviceByInterfacesAndCancelWaitForResume();	
		void SendEp0Request(); // for b device stop
		

private:

		CActorFDF* iActorFDF;
		CEp0Transfer* iControlEp0;
		RUsbInterface iUsbInterface0;
		RUsbInterface iUsbInterface1;
		CInterfaceWatcher* iInterface0Watcher;
		CInterfaceWatcher* iInterface1Watcher;
		TBool iSuspendedI0;
		TBool iSuspendedI1;
		TUint iDeviceHandle;
		
		RUsbDeviceA* iTestDevice;					
		TCaseSteps iCaseStep;


private:
		/**
		 The functor for this test case for the factory
		 */
		const static TFunctorTestCase<CUT_PBASE_T_USBDI_1235,TBool> iFunctor;
		};

	}
	
#endif
