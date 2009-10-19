#ifndef __TEST_CASE_PBASE_T_USBDI_1234_H
#define __TEST_CASE_PBASE_T_USBDI_1234_H

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
* @file PBASE-T_USBDI-1234.h
* @internalComponent
* 
*
*/



#include "BaseBulkTestCase.h"
#include "basicwatcher.h"

namespace NUnitTesting_USBDI
	{

	/**
	 
	 @SYMTestCaseID				PBASE-T_USBDI-1234
	 @SYMTestCaseDesc			Device suspend and resume only depend on the RUsbInterface 
	 @SYMFssID                  Def126984
     @SYMPREQ					BR2609
	 @SYMTestType				UT
	 @SYMTestPriority			1 
	 @SYMTestActions 			1. after device suspended,call resume interface and immediately do some bulk transfer operation  
	 @SYMTestExpectedResults 	Correct suspension and device resumption can be established
	 @SYMTestStatus				Implemented	

	 */
	class CUT_PBASE_T_USBDI_1234 : public CBaseBulkTestCase,
		public MTransferObserver, public MCommandObserver

		{
public:
		static CUT_PBASE_T_USBDI_1234* NewL(TBool aHostRole);
		~CUT_PBASE_T_USBDI_1234();

		static TInt Interface0ResumedL(TAny* aPtr);
		static TInt Interface1ResumedL(TAny* aPtr);
		static TInt Interface2ResumedL(TAny* aPtr);

private:
	      enum TCaseSteps
			{
			EInProgress,
			ESuspendDevice,
			EValidateResumebyInterface,
			EBulkTransferOutWhenResume,
			EValidBulkTransfeOut,   // tranfer in read 
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
		void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
				RUsbDevice::TDeviceState aNewState, TInt aCompletionCode);

public:
		// From MCommandObserver
		void Ep0TransferCompleteL(TInt aCompletionCode);

public:
		// From MTransferObserver
		void TransferCompleteL(TInt aTransferId, TInt aCompletionCode);
		

private:
		CUT_PBASE_T_USBDI_1234(TBool aHostRole);
		void ConstructL();
		void ExecuteHostTestCaseL();

private:

	

private:

		void TrySuspendDeviceByInterfaces();
		
		void SendEpRequest();
		void SendEpTransferRequest();
		
		TInt DoBulkOutTransfer(TInt aTransferId, TInt aCompletionCode,
				TDes & aMsg);		
		void TryResumeTransferImmediately(RUsbDevice::TDeviceState aState);
		

private:
		
		CInterfaceWatcher* iInterface0Watcher;
		CInterfaceWatcher* iInterface1Watcher;
		CInterfaceWatcher* iInterface2Watcher;
		
		TBool iSuspendedI0;
		TBool iSuspendedI1;
		TBool iSuspendedI2;

		TCaseSteps iCaseStep;
	

private:
		/**
		 The functor for this test case for the factory
		 */
		const static TFunctorTestCase<CUT_PBASE_T_USBDI_1234,TBool> iFunctor;
		};

	}

#endif
