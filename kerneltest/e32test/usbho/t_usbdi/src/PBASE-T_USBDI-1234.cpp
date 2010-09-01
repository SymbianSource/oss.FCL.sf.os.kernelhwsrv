// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// @file PBASE-T_USBDI-1234.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-1234.h"
#include "modelleddevices.h"
#include "testpolicy.h"
#include "testliterals.h"

namespace NUnitTesting_USBDI
	{
	const TInt KBulkTransferSize = 583;
	const TUint KHostNumWriteBytes1 = 210;
	const TUint KHostNumWriteBytes2 = 350;
	const TUint KHostNumWriteBytes3 = KBulkTransferSize - KHostNumWriteBytes1
			- KHostNumWriteBytes2; // 23
	const TUint KHostNumReadBytes1 = 301;
	const TUint KHostNumReadBytes2 = 21;
	const TUint KHostNumReadBytes3 = KBulkTransferSize - KHostNumReadBytes1
			- KHostNumReadBytes2; // 261


	//Make these single bit values ... 
	// ... so that their completion can be easily recorded in a bit mask!
	const TInt KBulkTransferInId0 = 1<<0;
	const TInt KBulkTransferInId1 = 1<<1;
	const TInt KBulkTransferInId2 = 1<<2;
	const TInt KBulkTransferOutId0 = 1<<3;
	const TInt KBulkTransferOutId1 = 1<<4;
	const TInt KBulkTransferOutId2 = 1<<5;

	const TInt KUnexpectedTransferID = -101;
	const TInt KUndefinedStep = -102;

	_LIT(KTestCaseId,"PBASE-T_USBDI-1234");
	// the name is very important 
	const TFunctorTestCase<CUT_PBASE_T_USBDI_1234,TBool>
			CUT_PBASE_T_USBDI_1234::iFunctor(KTestCaseId);

	CUT_PBASE_T_USBDI_1234* CUT_PBASE_T_USBDI_1234::NewL(TBool aHostRole)
		{
		CUT_PBASE_T_USBDI_1234* self = new (ELeave) CUT_PBASE_T_USBDI_1234(aHostRole);
		CleanupStack::PushL(self);
		self->ConstructL();
		CleanupStack::Pop(self);
		return self;
		}

	CUT_PBASE_T_USBDI_1234::CUT_PBASE_T_USBDI_1234(TBool aHostRole) :
		CBaseBulkTestCase(KTestCaseId, aHostRole), iCaseStep(EInProgress)
		{

		}

	void CUT_PBASE_T_USBDI_1234::ExecuteHostTestCaseL()
		{
		CBaseBulkTestCase::ExecuteHostTestCaseL();
		iInterface0Watcher = new (ELeave) CInterfaceWatcher(iUsbInterface0,TCallBack(CUT_PBASE_T_USBDI_1234::Interface0ResumedL,this));
		iInterface1Watcher = new (ELeave) CInterfaceWatcher(iUsbInterface1,TCallBack(CUT_PBASE_T_USBDI_1234::Interface1ResumedL,this));
		iInterface2Watcher = new (ELeave) CInterfaceWatcher(iUsbInterface1,TCallBack(CUT_PBASE_T_USBDI_1234::Interface2ResumedL,this));
		}

	void CUT_PBASE_T_USBDI_1234::ConstructL()
		{
		BaseBulkConstructL();
		}

	CUT_PBASE_T_USBDI_1234::~CUT_PBASE_T_USBDI_1234()
		{
		LOG_FUNC
		// Cancel any async operations

		Cancel(); // Cancel host timer

		if (iInterface1Watcher)
			{
			delete iInterface1Watcher;
			}
		if (iInterface0Watcher)
			{
			delete iInterface0Watcher;
			}
		if (iInterface2Watcher)
			{
			delete iInterface2Watcher;
			}

		}

	void CUT_PBASE_T_USBDI_1234::DeviceInsertedL(TUint aDeviceHandle)
		{
		LOG_FUNC

		Cancel();

		if (BaseBulkDeviceInsertedL(aDeviceHandle) == EDeviceConfigurationError)
		// Prepare for response from control transfer to client
			{
			iCaseStep = EFailed;
			}

		// Create the bulk transfers	
		iInTransfer[0]
				= new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkTransferSize,*this,KBulkTransferInId0);
		iInTransfer[1]
				= new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkTransferSize,*this,KBulkTransferInId1);
		iInTransfer[2]
				= new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkTransferSize,*this,KBulkTransferInId2);
		iOutTransfer[0]
				= new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkTransferSize,*this,KBulkTransferOutId0);
		iOutTransfer[1]
				= new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkTransferSize,*this,KBulkTransferOutId1);
		iOutTransfer[2]
				= new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkTransferSize,*this,KBulkTransferOutId2);

		// Initialise the descriptors for transfer		
		RDebug::Printf("Initialising the transfer descriptors");
		TInt err = iUsbInterface1.InitialiseTransferDescriptors();
		if (err != KErrNone)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Unable to initialise transfer descriptors"),err);
			RDebug::Print(msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(err, msg);
			iControlEp0->SendRequest(request, this);
			return;
			}

		iCaseStep = ESuspendDevice;

		TrySuspendDeviceByInterfaces();

		}

	TInt CUT_PBASE_T_USBDI_1234::Interface0ResumedL(TAny* aPtr)
		{
		
		// when device is really resumed , the whole of interfaces' previous call  PermitSuspendAndWaitForResume()'s requests of the device will be completed , random chose one .
		LOG_CFUNC
		RDebug::Printf("Interface 0 resumed");
		CUT_PBASE_T_USBDI_1234* self =
				reinterpret_cast<CUT_PBASE_T_USBDI_1234*>(aPtr);
		TInt completionCode = self->iInterface0Watcher->CompletionCode();
		RDebug::Printf("watcher 0 errCode=%d",completionCode);
		self->iSuspendedI0 = EFalse;

		switch (self->iCaseStep)
			{

			case EValidateResumebyInterface:
				{
				if (completionCode == KErrNone)
					{
					RDebug::Printf("device resumed succeed, do bulk transfer!");
					self->iCaseStep = EBulkTransferOutWhenResume;
					self->SendEpTransferRequest();
					}
				else
					{
					RDebug::Printf("device resumed failed,<err %d> ",completionCode);
					self->iCaseStep = EFailed;
					self->SendEpRequest();
					}
				}

				break;

			default:
				break;
			};

		return KErrNone;
		}

	TInt CUT_PBASE_T_USBDI_1234::Interface1ResumedL(TAny* aPtr)
		{
		LOG_CFUNC
		RDebug::Printf("Interface 1 resumed");
		CUT_PBASE_T_USBDI_1234* self =
				reinterpret_cast<CUT_PBASE_T_USBDI_1234*>(aPtr);
		RDebug::Printf("watcher 1 iStatus=%d",
				self->iInterface1Watcher->CompletionCode());
		self->iSuspendedI1 = EFalse;
		return KErrNone;
		}

	TInt CUT_PBASE_T_USBDI_1234::Interface2ResumedL(TAny* aPtr)
		{
		LOG_CFUNC
		RDebug::Printf("Interface 2 resumed");
		CUT_PBASE_T_USBDI_1234* self =
				reinterpret_cast<CUT_PBASE_T_USBDI_1234*>(aPtr);
		RDebug::Printf("watcher 2 iStatus=%d",
				self->iInterface2Watcher->CompletionCode());
		self->iSuspendedI2 = EFalse;
		return KErrNone;
		}

	void CUT_PBASE_T_USBDI_1234::DeviceStateChangeL(
			RUsbDevice::TDeviceState aPreviousState,
			RUsbDevice::TDeviceState aNewState, TInt aCompletionCode)
		{
		LOG_FUNC
		Cancel();

		RDebug::Printf(
				"Device State change from %d to %d err=%d",
				aPreviousState, aNewState, aCompletionCode);
		}

	void CUT_PBASE_T_USBDI_1234::Ep0TransferCompleteL(TInt aCompletionCode)
		{
		LOG_FUNC

		RDebug::Printf(
				"Ep0TransferCompleteL with aCompletionCode = %d",
				aCompletionCode);

		if (aCompletionCode != KErrNone)
			{
			if (iCaseStep == EFailed)
				{// ignore error, and catch the TestFailed method called further down.
				RDebug::Printf("***Failure sending FAIL message to client on endpoint 0***");
				}
			else
				{
				TBuf<256> msg;
				msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
				RDebug::Print(msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(aCompletionCode, msg);
				iControlEp0->SendRequest(request, this);
				return;
				}
			}

		switch (iCaseStep)
			{
			// Test case passed
			case EPassed:
				TestPassed();
				break;

				// Test case failed	
			case EFailed:
				TestFailed(KErrCompletion);
				break;

			case EBulkTransferOutWhenResume:
				RDebug::Printf("Try to send data");
				iOutTransfer[0]->TransferOut(KLiteralEnglish8().Mid(0, KHostNumWriteBytes1), EFalse);
				iOutTransfer[1]->TransferOut(KLiteralEnglish8().Mid(KHostNumWriteBytes1, KHostNumWriteBytes2), EFalse);
				iOutTransfer[2]->TransferOut(KLiteralEnglish8().Mid(KHostNumWriteBytes1+KHostNumWriteBytes2,
						KHostNumWriteBytes3), ETrue); //do not suppress ZLP on this last one  (though should be irrelevant here)    
				break;

			case EValidBulkTransfeOut:
				RDebug::Printf("Try to receive data");
				iInTransfer[0]->TransferIn(KHostNumReadBytes1);
				iInTransfer[1]->TransferIn(KHostNumReadBytes2);
				iInTransfer[2]->TransferIn(KHostNumReadBytes3);
				break;

			default:
				RDebug::Printf("<Error> Unknown test step");
				TestFailed(KErrUnknown);
				break;
			}
		}

	void CUT_PBASE_T_USBDI_1234::TransferCompleteL(TInt aTransferId,
			TInt aCompletionCode)
		{
		LOG_FUNC
		Cancel();

		TInt err(KErrNone);
		TBuf<256> msg;
		RDebug::Printf("Transfer completed (id=%d), aCompletionCode = %d",
				aTransferId, aCompletionCode);

		switch (iCaseStep)
			{
			case EBulkTransferOutWhenResume:
				if (aCompletionCode != KErrNone)
					{
					err = KErrCorrupt;
					msg.Format(_L("<Error %d> No data sent on bulk OUT request"),aCompletionCode);
					break; // switch(iCaseStep)
					}

				switch (aTransferId)
					{
					case KBulkTransferOutId0:
					case KBulkTransferOutId1:
					case KBulkTransferOutId2:
						iTransferComplete |= aTransferId;
						RDebug::Printf("Transfer %d completed", aTransferId);
						break; // switch(aTransferId)

					default:
						iTransferComplete = 0; //reset
						err = KUnexpectedTransferID;
						msg.Format(_L("<Error %d> Unexpected transfer ID, wanted %d, %d or %d, got %d"),
						err, KBulkTransferOutId0, KBulkTransferOutId1, KBulkTransferOutId2, aTransferId);
						break; // switch(aTransferId)
					}

				if (err == KErrNone && iTransferComplete
						== (KBulkTransferOutId0 | KBulkTransferOutId1
								| KBulkTransferOutId2))
					{
					RDebug::Printf(
							"Try to receive back sent data. Transfers Completed %d",
							iTransferComplete);
					iCaseStep = EValidBulkTransfeOut;
					TUint numBytes[KNumSplitWriteSections] =
						{
						KHostNumReadBytes1, KHostNumReadBytes2,
								KHostNumReadBytes3
						};
					TSplitWriteCachedReadDataRequest request(1, 1, 1, numBytes);
					iControlEp0->SendRequest(request, this);
					iTransferComplete = 0; //reset
					}
				break; // switch(iCaseStep)

			case EValidBulkTransfeOut: //transfer in
				if (aCompletionCode != KErrNone)
					{
					err = KErrCorrupt;
					msg.Format(_L("<Error %d> No data sent on bulk IN request"),aCompletionCode);
					break; // switch(iCaseStep)
					}

				switch (aTransferId)
					{
					case KBulkTransferInId0:
					case KBulkTransferInId1:
					case KBulkTransferInId2:
						iTransferComplete |= aTransferId;
						break; // switch(aTransferId)

					default:
						iTransferComplete = 0; //reset
						err = KUnexpectedTransferID;
						msg.Format(_L("<Error %d> Unexpected transfer ID, wanted %d, %d or %d, got %d"),
						err, KBulkTransferInId0, KBulkTransferInId1, KBulkTransferInId2, aTransferId);
						break; // switch(aTransferId)
					}

				if (err==KErrNone && iTransferComplete == (KBulkTransferInId0
						| KBulkTransferInId1 | KBulkTransferInId2))
					{
					// compare data rcvd now
					TPtrC8 data1(iInTransfer[0]->DataPolled());
					TPtrC8 data2(iInTransfer[1]->DataPolled());
					TPtrC8 data3(iInTransfer[2]->DataPolled());
					if (ValidateData(data1, KLiteralEnglish8().Mid(0, KHostNumReadBytes1)) == EFalse)
						{
						err = KErrCompletion; //indicates data validation failure
						break; // switch(iCaseStep)
						}

					if (ValidateData(data2, KLiteralEnglish8().Mid(KHostNumReadBytes1, KHostNumReadBytes2))
							== EFalse)
						{
						err = KErrCompletion; //indicates data validation failure
						break; // switch(iCaseStep)
						}

					if (ValidateData(data3, KLiteralEnglish8().Mid(KHostNumReadBytes1+KHostNumReadBytes2,
							KHostNumReadBytes3)) == EFalse)
						{
						err = KErrCompletion; //indicates data validation failure
						break; // switch(iCaseStep)
						}

					// Comparison is a match
					RDebug::Printf("Comparison for IN transfer is a match");
					iCaseStep = EPassed;
					TTestCasePassed request;
					iControlEp0->SendRequest(request, this);
					iTransferComplete = 0; //reset
					}
				break; // switch(iCaseStep)

			default:
				err = KUndefinedStep;
				msg.Format(_L("<Error %d> Undefined case step %d reached"),KUndefinedStep, iCaseStep);
				break; // switch(iCaseStep)
			}

		if (err == KErrCompletion)
			{
			//indicates data validation failure
			msg.Format(_L("<Error %d> Bulk transfer IN data received does not match Bulk Transfer OUT data"), err);
			}

		if (err!=KErrNone)
			{
			RDebug::Print(msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(err, msg);
			iControlEp0->SendRequest(request, this);
			}

		}

	void CUT_PBASE_T_USBDI_1234::TrySuspendDeviceByInterfaces()
		{
		// Suspend interface 0
		RDebug::Printf("Suspending interface 0");
		iInterface0Watcher->SuspendAndWatch();
		iSuspendedI0 = ETrue;

		// Suspend interface 1
		RDebug::Printf("Suspending interface 1");
		iInterface1Watcher->SuspendAndWatch();
		iSuspendedI1 = ETrue;

		// Suspend interface 2
		RDebug::Printf("Suspending interface 2");
		iInterface2Watcher->SuspendAndWatch();
		iSuspendedI2 = ETrue;

		TimeoutIn(10); // Give 10 seconds for device to suspend

		// scenario2 : device could do the bulk transfer must wait device really resumed. 
		iUsbInterface0.CancelPermitSuspend();

		iCaseStep = EValidateResumebyInterface;

		}

	void CUT_PBASE_T_USBDI_1234::SendEpRequest()
		{
		TTestCasePassed request;
		iControlEp0->SendRequest(request, this);
		}
	void CUT_PBASE_T_USBDI_1234::SendEpTransferRequest()
		{
		TEndpointReadRequest request(1, 1, KBulkTransferSize);// EP1 means endpoint index 1 not the actual endpoint number
		iControlEp0->SendRequest(request, this);
		}

	}//end namespace


