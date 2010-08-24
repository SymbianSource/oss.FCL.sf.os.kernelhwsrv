// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-1234Traces.h"
#endif

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
		OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1234_NEWL_ENTRY, aHostRole );
		CUT_PBASE_T_USBDI_1234* self = new (ELeave) CUT_PBASE_T_USBDI_1234(aHostRole);
		CleanupStack::PushL(self);
		self->ConstructL();
		CleanupStack::Pop(self);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1234_NEWL_EXIT, ( TUint )( self ) );
		return self;
		}

	CUT_PBASE_T_USBDI_1234::CUT_PBASE_T_USBDI_1234(TBool aHostRole) :
		CBaseBulkTestCase(KTestCaseId, aHostRole), iCaseStep(EInProgress)
		{
        OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1234_CUT_PBASE_T_USBDI_1234_ENTRY, this );

		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1234_CUT_PBASE_T_USBDI_1234_EXIT, this );
		}

	void CUT_PBASE_T_USBDI_1234::ExecuteHostTestCaseL()
		{
		OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1234_EXECUTEHOSTTESTCASEL_ENTRY, this );
		CBaseBulkTestCase::ExecuteHostTestCaseL();
		iInterface0Watcher = new (ELeave) CInterfaceWatcher(iUsbInterface0,TCallBack(CUT_PBASE_T_USBDI_1234::Interface0ResumedL,this));
		iInterface1Watcher = new (ELeave) CInterfaceWatcher(iUsbInterface1,TCallBack(CUT_PBASE_T_USBDI_1234::Interface1ResumedL,this));
		iInterface2Watcher = new (ELeave) CInterfaceWatcher(iUsbInterface1,TCallBack(CUT_PBASE_T_USBDI_1234::Interface2ResumedL,this));
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1234_EXECUTEHOSTTESTCASEL_EXIT, this );
		}

	void CUT_PBASE_T_USBDI_1234::ConstructL()
		{
		OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1234_CONSTRUCTL_ENTRY, this );
		BaseBulkConstructL();
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1234_CONSTRUCTL_EXIT, this );
		}

	CUT_PBASE_T_USBDI_1234::~CUT_PBASE_T_USBDI_1234()
		{
		OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1234_CUT_PBASE_T_USBDI_1234_ENTRY_DUP01, this );
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

		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1234_CUT_PBASE_T_USBDI_1234_EXIT_DUP01, this );
		}

	void CUT_PBASE_T_USBDI_1234::DeviceInsertedL(TUint aDeviceHandle)
		{
        OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1234_DEVICEINSERTEDL_ENTRY, this );

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
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234, "Initialising the transfer descriptors");
		TInt err = iUsbInterface1.InitialiseTransferDescriptors();
		if (err != KErrNone)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Unable to initialise transfer descriptors"),err);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP01, msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(err, msg);
			iControlEp0->SendRequest(request, this);
			OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1234_DEVICEINSERTEDL_EXIT, this );
			return;
			}

		iCaseStep = ESuspendDevice;

		TrySuspendDeviceByInterfaces();

		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1234_DEVICEINSERTEDL_EXIT_DUP01, this );
		}

	TInt CUT_PBASE_T_USBDI_1234::Interface0ResumedL(TAny* aPtr)
		{
		OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1234_INTERFACE0RESUMEDL_ENTRY, 0 );
		
		// when device is really resumed , the whole of interfaces' previous call  PermitSuspendAndWaitForResume()'s requests of the device will be completed , random chose one .
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP02, "Interface 0 resumed");
		CUT_PBASE_T_USBDI_1234* self =
				reinterpret_cast<CUT_PBASE_T_USBDI_1234*>(aPtr);
		TInt completionCode = self->iInterface0Watcher->CompletionCode();
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP03, "watcher 0 errCode=%d",completionCode);
		self->iSuspendedI0 = EFalse;

		switch (self->iCaseStep)
			{

			case EValidateResumebyInterface:
				{
				if (completionCode == KErrNone)
					{
					OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP04, "device resumed succeed, do bulk transfer!");
					self->iCaseStep = EBulkTransferOutWhenResume;
					self->SendEpTransferRequest();
					}
				else
					{
					OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP05, "device resumed failed,<err %d> ",completionCode);
					self->iCaseStep = EFailed;
					self->SendEpRequest();
					}
				}

				break;

			default:
				break;
			};

		OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_1234_INTERFACE0RESUMEDL_EXIT, 0, KErrNone );
		return KErrNone;
		}

	TInt CUT_PBASE_T_USBDI_1234::Interface1ResumedL(TAny* aPtr)
		{
		OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1234_INTERFACE1RESUMEDL_ENTRY, 0 );
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP06, "Interface 1 resumed");
		CUT_PBASE_T_USBDI_1234* self =
				reinterpret_cast<CUT_PBASE_T_USBDI_1234*>(aPtr);
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP07, "watcher 1 iStatus=%d",
				self->iInterface1Watcher->CompletionCode());
		self->iSuspendedI1 = EFalse;
		OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_1234_INTERFACE1RESUMEDL_EXIT, 0, KErrNone );
		return KErrNone;
		}

	TInt CUT_PBASE_T_USBDI_1234::Interface2ResumedL(TAny* aPtr)
		{
		OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1234_INTERFACE2RESUMEDL_ENTRY, 0 );
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP08, "Interface 2 resumed");
		CUT_PBASE_T_USBDI_1234* self =
				reinterpret_cast<CUT_PBASE_T_USBDI_1234*>(aPtr);
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP09, "watcher 2 iStatus=%d",
				self->iInterface2Watcher->CompletionCode());
		self->iSuspendedI2 = EFalse;
		OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_1234_INTERFACE2RESUMEDL_EXIT, 0, KErrNone );
		return KErrNone;
		}

	void CUT_PBASE_T_USBDI_1234::DeviceStateChangeL(
			RUsbDevice::TDeviceState aPreviousState,
			RUsbDevice::TDeviceState aNewState, TInt aCompletionCode)
		{
		OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1234_DEVICESTATECHANGEL_ENTRY, this );
		Cancel();

		RDebug::Printf(
				"Device State change from %d to %d err=%d",
				aPreviousState, aNewState, aCompletionCode);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1234_DEVICESTATECHANGEL_EXIT, this );
		}

	void CUT_PBASE_T_USBDI_1234::Ep0TransferCompleteL(TInt aCompletionCode)
		{
        OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1234_EP0TRANSFERCOMPLETEL_ENTRY, this );

		RDebug::Printf(
				"Ep0TransferCompleteL with aCompletionCode = %d",
				aCompletionCode);

		if (aCompletionCode != KErrNone)
			{
			if (iCaseStep == EFailed)
				{// ignore error, and catch the TestFailed method called further down.
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP10, "***Failure sending FAIL message to client on endpoint 0***");
				}
			else
				{
				TBuf<256> msg;
				msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP11, msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(aCompletionCode, msg);
				iControlEp0->SendRequest(request, this);
				OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1234_EP0TRANSFERCOMPLETEL_EXIT, this );
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
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP12, "Try to send data");
				iOutTransfer[0]->TransferOut(KLiteralEnglish8().Mid(0, KHostNumWriteBytes1), EFalse);
				iOutTransfer[1]->TransferOut(KLiteralEnglish8().Mid(KHostNumWriteBytes1, KHostNumWriteBytes2), EFalse);
				iOutTransfer[2]->TransferOut(KLiteralEnglish8().Mid(KHostNumWriteBytes1+KHostNumWriteBytes2,
						KHostNumWriteBytes3), ETrue); //do not suppress ZLP on this last one  (though should be irrelevant here)    
				break;

			case EValidBulkTransfeOut:
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP13, "Try to receive data");
				iInTransfer[0]->TransferIn(KHostNumReadBytes1);
				iInTransfer[1]->TransferIn(KHostNumReadBytes2);
				iInTransfer[2]->TransferIn(KHostNumReadBytes3);
				break;

			default:
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP14, "<Error> Unknown test step");
				TestFailed(KErrUnknown);
				break;
			}
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1234_EP0TRANSFERCOMPLETEL_EXIT_DUP01, this );
		}

	void CUT_PBASE_T_USBDI_1234::TransferCompleteL(TInt aTransferId,
			TInt aCompletionCode)
		{
		OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1234_TRANSFERCOMPLETEL_ENTRY, this );
		Cancel();

		TInt err(KErrNone);
		TBuf<256> msg;
		OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP15, "Transfer completed (id=%d), aCompletionCode = %d",
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
						OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP16, "Transfer %d completed", aTransferId);
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
					OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP17, "Comparison for IN transfer is a match");
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
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP18, msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(err, msg);
			iControlEp0->SendRequest(request, this);
			}

		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1234_TRANSFERCOMPLETEL_EXIT, this );
		}

	void CUT_PBASE_T_USBDI_1234::TrySuspendDeviceByInterfaces()
		{
		OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1234_TRYSUSPENDDEVICEBYINTERFACES_ENTRY, this );
		// Suspend interface 0
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP19, "Suspending interface 0");
		iInterface0Watcher->SuspendAndWatch();
		iSuspendedI0 = ETrue;

		// Suspend interface 1
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP20, "Suspending interface 1");
		iInterface1Watcher->SuspendAndWatch();
		iSuspendedI1 = ETrue;

		// Suspend interface 2
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1234_DCUT_PBASE_T_USBDI_1234_DUP21, "Suspending interface 2");
		iInterface2Watcher->SuspendAndWatch();
		iSuspendedI2 = ETrue;

		TimeoutIn(10); // Give 10 seconds for device to suspend

		// scenario2 : device could do the bulk transfer must wait device really resumed. 
		iUsbInterface0.CancelPermitSuspend();

		iCaseStep = EValidateResumebyInterface;

		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1234_TRYSUSPENDDEVICEBYINTERFACES_EXIT, this );
		}

	void CUT_PBASE_T_USBDI_1234::SendEpRequest()
		{
		OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1234_SENDEPREQUEST_ENTRY, this );
		TTestCasePassed request;
		iControlEp0->SendRequest(request, this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1234_SENDEPREQUEST_EXIT, this );
		}
	void CUT_PBASE_T_USBDI_1234::SendEpTransferRequest()
		{
		OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1234_SENDEPTRANSFERREQUEST_ENTRY, this );
		TEndpointReadRequest request(1, 1, KBulkTransferSize);// EP1 means endpoint index 1 not the actual endpoint number
		iControlEp0->SendRequest(request, this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1234_SENDEPTRANSFERREQUEST_EXIT, this );
		}

	}//end namespace


