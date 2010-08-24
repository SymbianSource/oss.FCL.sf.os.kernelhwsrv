// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// @file PBASE-T_USBDI-1229.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-1229.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "testliterals.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-1229Traces.h"
#endif


 

namespace NUnitTesting_USBDI
	{
const TUint KTotalBytesToTransfer = 1024*64+511; //64kB + 511 bytes
const TUint KHostNumWriteBytes = 1024*16;
const TInt KBulkMaxTransferSize = KHostNumWriteBytes + 1000;
const TInt KDeviceNumReadBytes = 1024;


//Make these single bit values ... 
// ... so that their completion can be easily recorded in a bit mask!
const TUint32 KBulkTransferOutId[KMaxNumOutTransfers] = {1<<0, 1<<1};
const TUint32 KBulkTransferIdMask = KBulkTransferOutId[0] | KBulkTransferOutId[1];

const TInt KUndefinedStep	 		= -102;
const TInt KUnexpectedTransferID 	= -103;
const TInt KErrReturnedDeviceReadBytesTooVariable = -140;
const TInt KTransferSuccess	 = +100;

const TUint KRepeatedTimerInterval = 10000; //10ms
const TUint KBaseTimer = 0;
const TUint KTestTimer = 1;
const TUint KMaxTimeDiffPercentage = 60; //on inspection worst result was just under 70%
const TUint KMaxBytesReadDiffPercentage = 50;




_LIT(KTestCaseId,"PBASE-T_USBDI-1229");
const TFunctorTestCase<CUT_PBASE_T_USBDI_1229,TBool> CUT_PBASE_T_USBDI_1229::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_1229* CUT_PBASE_T_USBDI_1229::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1229_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_1229* self = new (ELeave) CUT_PBASE_T_USBDI_1229(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1229_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_1229::CUT_PBASE_T_USBDI_1229(TBool aHostRole)
:	CBaseBulkTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1229_CUT_PBASE_T_USBDI_1229_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1229_CUT_PBASE_T_USBDI_1229_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_1229::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1229_CONSTRUCTL_ENTRY, this );
	BaseBulkConstructL();

	iInBuffer = HBufC8::NewL(KTestBufferLength);

	iBulkTestTimer = CBulkTestTimer::NewL(*this);	

	//Create buffer to contain multiple lots of the payload pattern
	//..so that we may grab cyclic chunks of said payload pattern
	//..this is used to send to the client for validation purposes
	//..AND for the host to send data
	TInt repeats = KHostNumWriteBytes / (KLiteralEnglish5().Length()) + 1 + 1; //1 extra to accommodate start point plus 1 to accomodate remainder in division
	iOutBuffer = HBufC8::NewL(KLiteralEnglish5().Length() * repeats);
	iOutBufferPtr.Set(iOutBuffer->Des());
	iOutBufferPtr.Zero();
	for(TInt i=0;i<repeats;i++)
		{
		iOutBufferPtr.Append(KLiteralEnglish5());
		}

	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_CONSTRUCTL, "CUT_PBASE_T_USBDI_1229::ConstructL(): buffer created");
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1229_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_1229::~CUT_PBASE_T_USBDI_1229()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1229_CUT_PBASE_T_USBDI_1229_ENTRY_DUP01, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1229_CUT_PBASE_T_USBDI_1229_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_1229::KillTransfers()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1229_KILLTRANSFERS_ENTRY, this );
	
	iOutTransfer[0]->Cancel();
	iOutTransfer[1]->Cancel();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1229_KILLTRANSFERS_EXIT, this );
	}

void CUT_PBASE_T_USBDI_1229::ExtractDeviceReadBytes()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1229_EXTRACTDEVICEREADBYTES_ENTRY, this );
	
	iControlEp0->LastRequestCompletionTime( iEndTime[KTestTimer]);
	iTimingError = iTimingError == KErrNone ? CheckTimes(KBaseTimer, KTestTimer, KMaxTimeDiffPercentage) : iTimingError;
	ResetTimes(KTestTimer);
	
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EXTRACTDEVICEREADBYTES, "Collect client's return of the number of bytes read on its bulk out endpoint ...");
	TLex8 lex(iInBufferPtr.Left(KNumberStringLength));
	TUint32 numBytes = 0;
	User::LeaveIfError(lex.Val(numBytes, EDecimal));
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EXTRACTDEVICEREADBYTES_DUP01, "********************NUM*BYTES****************************");
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EXTRACTDEVICEREADBYTES_DUP02, "         NUM BYTES READ BY CLIENT ==== %d ====           ", numBytes);
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EXTRACTDEVICEREADBYTES_DUP03, "********************NUM*BYTES****************************");
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EXTRACTDEVICEREADBYTES_DUP04, "\n");

	if(numBytes != 0)
		//Do not count this case - it may result from the remote resetting when all bulk transfers have completed
		{
		TUint numBytesSinceLast = numBytes - iDeviceNumBytesReadInTotal;
		iDeviceNumBytesReadInTotal = numBytes;
		iDeviceMinTimedNumBytesRead = numBytesSinceLast < iDeviceMinTimedNumBytesRead ?  numBytesSinceLast : iDeviceMinTimedNumBytesRead ;
		iDeviceMaxTimedNumBytesRead = numBytesSinceLast > iDeviceMaxTimedNumBytesRead ?  numBytesSinceLast : iDeviceMaxTimedNumBytesRead ;;
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1229_EXTRACTDEVICEREADBYTES_EXIT, this );
	}


void CUT_PBASE_T_USBDI_1229::PostTransferAction()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1229_POSTTRANSFERACTION_ENTRY, this );
	switch(iTransferResult)
		{
		case KErrNone:
			//do nothing
			return;

		case KTransferSuccess:
			//indicates data validation failure
			{
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_POSTTRANSFERACTION, "Asking client to post validation recorded on the endpoint on its interface - ready for collection");
			iCaseStep = ERequestPrepareEndpointValidationResult;
			TRecordedValidationResultRequest request(1,1);
			iControlEp0->SendRequest(request,this);
			}
			OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1229_POSTTRANSFERACTION_EXIT, this );
			return;
		
		default:
			{
			iCaseStep = EFailed;
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_POSTTRANSFERACTION_DUP01, iMsg);
			TTestCaseFailed request(iTransferResult,iMsg);
			iControlEp0->SendRequest(request,this);
			}
			OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1229_POSTTRANSFERACTION_EXIT_DUP01, this );
			return;
		}
	}


TBool CUT_PBASE_T_USBDI_1229::PerformNextTransfer(TInt aTransferId)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1229_PERFORMNEXTTRANSFER_ENTRY, this );
	
	if(iNumWriteBytesRequested >= KTotalBytesToTransfer)
		{
		OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_PERFORMNEXTTRANSFER, "All transfers sent - num bytes actually written = %u, num bytes required to be written = %u", iNumWriteBytesRequested, KTotalBytesToTransfer);
		OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_1229_PERFORMNEXTTRANSFER_EXIT, this, EFalse );
		return EFalse; //Not writing any more - signal to user that no more transfers are required
		}
	TUint bytesToWrite = KTotalBytesToTransfer - iNumWriteBytesRequested;
	TUint numWriteBytes = bytesToWrite < KHostNumWriteBytes ? bytesToWrite : KHostNumWriteBytes;

	_LITDBG("PerformNextTransfer: None existant transfer ID requested");
	__ASSERT_DEBUG(aTransferId==KBulkTransferOutId[0] || aTransferId==KBulkTransferOutId[1], User::Panic(lit, KErrArgument));
	CBulkTransfer& bulkTransfer = aTransferId==KBulkTransferOutId[0]?*iOutTransfer[0]:*iOutTransfer[1];
	bulkTransfer.TransferOut(iOutBufferPtr.Mid(iNumWriteBytesRequested%(KLiteralEnglish5().Length()), numWriteBytes), EFalse);
	iNumWriteBytesRequested += numWriteBytes;

	OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_1229_PERFORMNEXTTRANSFER_EXIT_DUP01, this, ETrue );
	return ETrue;
	}

	
void CUT_PBASE_T_USBDI_1229::RequestNumBytesSent(TUint8 aTimerIndex)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1229_REQUESTNUMBYTESSENT_ENTRY, this );
	iInBufferPtr.Set(iInBuffer->Des());
	iInBufferPtr.Zero(); //reset
	iInBufferPtr.SetLength(KNumberStringLength);
	TInterfaceGetRecordedNumBytesReadInPayload request(1,1,iInBufferPtr);
	iControlEp0->SendRequest(request,this);
	iControlEp0->LastRequestStartTime( iStartTime[aTimerIndex]);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1229_REQUESTNUMBYTESSENT_EXIT, this );
	}


void CUT_PBASE_T_USBDI_1229::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1229_EP0TRANSFERCOMPLETEL_ENTRY, this );
	
	OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d, test step = %d", aCompletionCode, iCaseStep);
	
	if(aCompletionCode != KErrNone)
		{
		if(iCaseStep == EFailed)
			{// ignore error, nad catch the TestFailed method called further down.
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EP0TRANSFERCOMPLETEL_DUP01, "***Failure sending FAIL message to client on endpoint 0***");
			}
		else
			{
			TBuf<256> msg;
			KillTransfers();
			_LIT(lit, "<Error %d> Transfer to control endpoint 0 was not successful");
			msg.Format(lit,aCompletionCode);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EP0TRANSFERCOMPLETEL_DUP02, msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(aCompletionCode,msg);
			iControlEp0->SendRequest(request,this);
			OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1229_EP0TRANSFERCOMPLETEL_EXIT, this );
			return;
			}
		}
	
	switch(iCaseStep)
		{
		// Test case passed
		case EPassed:
			TestPassed();
			break;
		
		// Test case failed	
		case EFailed:
			TestFailed(KErrCompletion);
			break;
		
		case EGetTimerBase:
			{
			iControlEp0->LastRequestCompletionTime( iEndTime[KBaseTimer]);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EP0TRANSFERCOMPLETEL_DUP03, "Asking client for continuous 'Read' and 'Validate'");
			iCaseStep = ERequestRepeatedReadAndValidate;
			TRepeatedReadAndValidateDataRequest request(1,1,KLiteralEnglish5(),KDeviceNumReadBytes,KTotalBytesToTransfer);// EP2 means endpoint index 2 not the actual endpoint number, here the ep with 32 byte max packet size
			iControlEp0->SendRequest(request,this);
			}
			break;
			
		case ERequestRepeatedReadAndValidate:
			{
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EP0TRANSFERCOMPLETEL_DUP04, "Try to perform ALL transfers");
	
			iCaseStep = ETransfer;	
			
			PerformNextTransfer(KBulkTransferOutId[0]);
			PerformNextTransfer(KBulkTransferOutId[1]);
			_LITDBG("Bulk test timer NOT instanciated");
			__ASSERT_DEBUG(iBulkTestTimer, User::Panic(lit, KErrGeneral));
			iBulkTestTimer->After(KRepeatedTimerInterval);
			}
			break;
			
		case ETransfer:
		// we must be getting num bytes read
			ExtractDeviceReadBytes();
	
			//Restart timer
			_LITDBG("Bulk test timer NOT instanciated");
			__ASSERT_DEBUG(iBulkTestTimer, User::Panic(lit, KErrGeneral));
			iBulkTestTimer->After(KRepeatedTimerInterval);
			break;
			
		case EDelayedTransferComplete:
			PostTransferAction();
			break;
			
		case ERequestPrepareEndpointValidationResult:
			{
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EP0TRANSFERCOMPLETEL_DUP05, "Asking client to prepare the result of its continuous validation");
			iCaseStep = ERequestValidationResult;
			iInBufferPtr.Set(iInBuffer->Des());
			iInBufferPtr.Zero(); //reset
			iInBufferPtr.SetLength(KPassFailStringLength);
			TInterfaceGetPayloadRequest request(1,iInBufferPtr);
			iControlEp0->SendRequest(request,this);
			}
			break;
	
		case ERequestValidationResult:
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EP0TRANSFERCOMPLETEL_DUP06, "Collect client's return validation  result in a pass or fail string ...");
            OstTraceData(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EP0TRANSFERCOMPLETEL_DUP56, "", iInBuffer->Ptr(), iInBuffer->Length());
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EP0TRANSFERCOMPLETEL_DUP07, "\n");
			iInBufferPtr.Set(iInBuffer->Des());
			if(iInBufferPtr.Compare(KClientPassString) == 0)
				{
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EP0TRANSFERCOMPLETEL_DUP08, "Client Validation Result is a PASS");
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EP0TRANSFERCOMPLETEL_DUP09, "This is the FINAL check - the whole test has a PASSED");
				iCaseStep = EPassed;
				TTestCasePassed request;
				iControlEp0->SendRequest(request,this);
				}
			else
				{
				TBuf<256> msg;
				_LIT(lit, "<Error> Bulk data VALIDATION check was NOT successful");
				msg.Format(lit);
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EP0TRANSFERCOMPLETEL_DUP10, msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(KErrCorrupt,msg);
				iControlEp0->SendRequest(request,this);
				}
			break;
	
		default:
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_EP0TRANSFERCOMPLETEL_DUP11, "<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1229_EP0TRANSFERCOMPLETEL_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_1229::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1229_TRANSFERCOMPLETEL_ENTRY, this );
	Cancel();
	
	iTransferResult = KErrNone;
	OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_TRANSFERCOMPLETEL, "Transfer completed (id=%d), aCompletionCode = %d, test step = %d",aTransferId, aCompletionCode, iCaseStep);


	switch(iCaseStep)
		{
		case ETransfer:
			if(aCompletionCode != KErrNone)
				{
				KillTransfers();
				iTransferResult = KErrCorrupt;
				_LIT(lit, "<Error %d> The transfer completed with an error.");
				iMsg.Format(lit, aCompletionCode);
				break;
				}
			if(aTransferId != KBulkTransferOutId[0] && aTransferId != KBulkTransferOutId[1])
				{
				iTransferComplete = 0; //reset
				iTransferResult = KUnexpectedTransferID;
				_LIT(lit, "<Error %d> Unexpected transfer ID, wanted %d or %d, got %d");
				iMsg.Format(lit, iTransferResult, KBulkTransferOutId[0], KBulkTransferOutId[1], aTransferId);
				break;
				}
			OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_TRANSFERCOMPLETEL_DUP01, "Transfer OUT %d completed - num bytes sent = %d", aTransferId, iNumWriteBytesRequested);
			
			if(PerformNextTransfer(aTransferId)==EFalse)
				{
				iTransferComplete |= aTransferId;
				OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_TRANSFERCOMPLETEL_DUP02, "All transfer OUT %ds completed (Transfer Completion Aggregation Mask 0x%x)", aTransferId, iTransferComplete);
				}
			
			if(iTransferResult==KErrNone && (iTransferComplete & KBulkTransferIdMask) == KBulkTransferIdMask)
				{
				/*
				Transfers all complete - now ask device to validate first interface's transfer OUT
				*/
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_TRANSFERCOMPLETEL_DUP03, "All Transfers Completed Successfully: Transfer Completion Aggregation Mask 0x%x", iTransferComplete);
				if(iTransferResult==KErrNone)
					{
					iBulkTestTimer->Cancel(); //Cancel Timer 
					iTransferResult = KTransferSuccess;
					if(iTimingError == KErrTooBig)
						{
						__PRINT_CONTROL_TRANSFER_TIMER_COMPARISON_WARNING
						iTransferResult = KErrTooBig;
						iTimingError = KErrNone; //reset
						}
					if(KMaxBytesReadDiffPercentage*iDeviceMaxTimedNumBytesRead > KPercent*iDeviceMinTimedNumBytesRead)
						{
						OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_TRANSFERCOMPLETEL_DUP04, "Device APPARENTLY reading rate erratic:-");
						OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_TRANSFERCOMPLETEL_DUP05, "Min Timed Number of Bytes = %d", iDeviceMinTimedNumBytesRead);
						OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_TRANSFERCOMPLETEL_DUP06, "Max Timed Number of Bytes = %d", iDeviceMaxTimedNumBytesRead);
						iTransferResult = KErrTooBig;
						iDeviceMaxTimedNumBytesRead = 0;
						iDeviceMinTimedNumBytesRead = KMaxTUint;
						}
					}
				}
			break;

		default:
			iTransferResult = KUndefinedStep;
			_LIT(lit, "<Error %d> Undefined case step %d reached");
			iMsg.Format(lit,KUndefinedStep, iCaseStep);
			break;
		}


	if(iTransferResult == KErrReturnedDeviceReadBytesTooVariable)
		//indicates apparent device read rate validation failure
		{
		iMsg.Format(_L("<Error %d> Device APPEARS not to be reading bytes at a constant rate"), iTransferResult);
		}

	if(iTransferResult == KErrTooBig)
		//indicates timing validation failure
		{
		iMsg.Format(_L("<Error %d> Timer comparison showed too great a difference in transfer times between the time taken by EP0 transfers with and without a bulk transfer"), iTransferResult);
		}
	
	if(iTransferResult == KErrCompletion)
		//indicates data validation failure
		{
		_LIT(lit, "<Error %d> Client has posted an error discovered in validation");
		iMsg.Format(lit, iTransferResult);
		}

	if(iTransferResult != KErrNone)
		{	
		KillTransfers(); //harmless if tranfers are all done
		if(!iControlEp0->IsActive())
			{
			PostTransferAction();
			}
		else
			{
			iCaseStep = EDelayedTransferComplete; //so that we move forward when the EP0 transfer has completed
			}
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1229_TRANSFERCOMPLETEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_1229::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1229_DEVICEINSERTEDL_ENTRY, this );
	
	Cancel();
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_DEVICEINSERTEDL, "this - %08x", this);
	
	TBuf<256> msg;
	TInt err = KErrNone;
	if(BaseBulkDeviceInsertedL(aDeviceHandle, EFalse) == EDeviceConfigurationError)
		// Prepare for response from control transfer to client
		{
		err = KErrGeneral;
		_LIT(lit, "Base class DeviceInsertedL failed");
		msg.Format(lit);
		}
	else
		{
		if(SetUpInterfaceAndPipesL(aDeviceHandle, 2) == EDeviceConfigurationError)
			// Prepare for response from control transfer to client
			{
			err = KErrGeneral;
			_LIT(lit, "Base class SetUpInterfaceAndPipes for Interface 2 failed");
			msg.Format(lit);
			}
		else
			{
	
			iOutTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkMaxTransferSize,*this,KBulkTransferOutId[0]);
			iOutTransfer[1] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkMaxTransferSize,*this,KBulkTransferOutId[1]);
			
			// Initialise the descriptors for transfer		
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_DEVICEINSERTEDL_DUP01, "Initialising the transfer descriptors - interface 1");
			err = iUsbInterface1.InitialiseTransferDescriptors();
			if(err != KErrNone)
				{
				_LIT(lit, "<Error %d> Unable to initialise transfer descriptors");
				msg.Format(lit,err);
				}
			}
		}
	if(err != KErrNone)
		{
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1229_DEVICEINSERTEDL_DUP02, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		}
	else
		{
		iCaseStep = EGetTimerBase;
		iDeviceMinTimedNumBytesRead = KMaxTUint;
		iDeviceMaxTimedNumBytesRead = 0;
		iDeviceNumBytesReadInTotal = 0;
		RequestNumBytesSent(KBaseTimer);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1229_DEVICEINSERTEDL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_1229::HandleBulkTestTimerFired()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1229_HANDLEBULKTESTTIMERFIRED_ENTRY, this );
	if(iCaseStep == ETransfer)
		{
		RequestNumBytesSent(KTestTimer);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1229_HANDLEBULKTESTTIMERFIRED_EXIT, this );
	}
	
	} //end namespace
