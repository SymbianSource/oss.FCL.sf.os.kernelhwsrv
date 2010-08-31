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
// @file PBASE-T_USBDI-1230.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-1230.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "testliterals.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-1230Traces.h"
#endif


 

namespace NUnitTesting_USBDI
	{
const TUint KTotalBytesToTransfer = 1024*64+511; //64kB + 511 bytes
const TUint KHostNumReadBytes = 1024*16;
const TInt KBulkMaxTransferSize = KHostNumReadBytes + 1000;
const TInt KDeviceNumWriteBytes = 1024;


//Make these single bit values ... 
// ... so that their completion can be easily recorded in a bit mask!
const TUint32 KBulkTransferInId0 = 1<<0;
const TUint32 KBulkTransferInId1 = 1<<1; 
const TUint32 KBulkTransferInId[KMaxNumInTransfers] = {KBulkTransferInId0, KBulkTransferInId1};
const TUint32 KBulkTransferIdMask = KBulkTransferInId[0] | KBulkTransferInId[1];

const TInt KUndefinedStep	 		= -102;
const TInt KUnexpectedTransferID 	= -103;
const TInt KErrReturnedDeviceReadBytesTooVariable = -140;
const TInt KTransferSuccess	 = +100;

const TUint KRepeatedTimerInterval = 10000; //10ms
const TUint KBaseTimer = 0;
const TUint KTestTimer = 1;
const TUint KMaxTimeDiffPercentage = 60; //on inspection worst result was just under 70%
const TUint KMaxBytesWrittenDiffPercentage = 50;




_LIT(KTestCaseId,"PBASE-T_USBDI-1230");
const TFunctorTestCase<CUT_PBASE_T_USBDI_1230,TBool> CUT_PBASE_T_USBDI_1230::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_1230* CUT_PBASE_T_USBDI_1230::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1230_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_1230* self = new (ELeave) CUT_PBASE_T_USBDI_1230(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1230_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_1230::CUT_PBASE_T_USBDI_1230(TBool aHostRole)
:	CBaseBulkTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1230_CUT_PBASE_T_USBDI_1230_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1230_CUT_PBASE_T_USBDI_1230_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_1230::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1230_CONSTRUCTL_ENTRY, this );
	BaseBulkConstructL();

	iInBuffer = HBufC8::NewL(KTestBufferLength);

	iBulkTestTimer = CBulkTestTimer::NewL(*this);	

	//Create buffer to contain multiple lots of the payload pattern
	//..so that we may grab cyclic chunks of said payload pattern
	TInt repeats = KHostNumReadBytes / (KLiteralEnglish5().Length()) + 1 + 1; //1 extra to accommodate start point plus 1 to accomodate remainder in division
	iValidateBuffer = HBufC8::NewL(KLiteralEnglish5().Length() * repeats);
	iValidateBufferPtr.Set(iValidateBuffer->Des());
	iValidateBufferPtr.Zero();
	for(TInt i=0;i<repeats;i++)
		{
		iValidateBufferPtr.Append(KLiteralEnglish5());
		}
	
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_CONSTRUCTL, "CUT_PBASE_T_USBDI_1230::ConstructL(): buffer created");
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1230_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_1230::~CUT_PBASE_T_USBDI_1230()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1230_CUT_PBASE_T_USBDI_1230_ENTRY_DUP01, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1230_CUT_PBASE_T_USBDI_1230_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_1230::KillTransfers()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1230_KILLTRANSFERS_ENTRY, this );
	
	iInTransfer[0]->Cancel();
	iInTransfer[1]->Cancel();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1230_KILLTRANSFERS_EXIT, this );
	}

void CUT_PBASE_T_USBDI_1230::ExtractDeviceReadBytes()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1230_EXTRACTDEVICEREADBYTES_ENTRY, this );
	
	iControlEp0->LastRequestCompletionTime( iEndTime[KTestTimer]);
	iTimingError = iTimingError == KErrNone ? CheckTimes(KBaseTimer, KTestTimer, KMaxTimeDiffPercentage) : iTimingError;
	ResetTimes(KTestTimer);
	
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_EXTRACTDEVICEREADBYTES, "Collect client's return of the number of bytes written on its bulk in endpoint ...");
	TLex8 lex(iInBufferPtr.Left(KNumberStringLength));
	TUint32 numBytes = 0;
	User::LeaveIfError(lex.Val(numBytes, EDecimal));
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_EXTRACTDEVICEREADBYTES_DUP01, "********************NUM*BYTES****************************");
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_EXTRACTDEVICEREADBYTES_DUP02, "         NUM BYTES READ BY CLIENT ==== %d ====           ", numBytes);
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_EXTRACTDEVICEREADBYTES_DUP03, "********************NUM*BYTES****************************");
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_EXTRACTDEVICEREADBYTES_DUP04, "\n");

	if(numBytes != 0)
		//Do not count this case - it may result from the remote resetting when all bulk transfers have completed
		{
		TUint numBytesSinceLast = numBytes - iDeviceNumBytesReadInTotal;
		iDeviceNumBytesReadInTotal = numBytes;
		iDeviceMinTimedNumBytesRead = numBytesSinceLast < iDeviceMinTimedNumBytesRead ?  numBytesSinceLast : iDeviceMinTimedNumBytesRead ;
		iDeviceMaxTimedNumBytesRead = numBytesSinceLast > iDeviceMaxTimedNumBytesRead ?  numBytesSinceLast : iDeviceMaxTimedNumBytesRead ;;
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1230_EXTRACTDEVICEREADBYTES_EXIT, this );
	}


void CUT_PBASE_T_USBDI_1230::PostTransferAction()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1230_POSTTRANSFERACTION_ENTRY, this );
	switch(iTransferResult)
		{
		case KErrNone:
			//do nothing
			return;

		case KTransferSuccess:
			{
			// Indicates success - comparison is a match
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_POSTTRANSFERACTION, "Comparison for IN transfer is a match");
			iCaseStep = EPassed;
			TTestCasePassed request;
			iTransferComplete = 0; //reset
			iControlEp0->SendRequest(request,this);
			}
			OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1230_POSTTRANSFERACTION_EXIT, this );
			return; 
			
			
		default:
			{
			iCaseStep = EFailed;
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_POSTTRANSFERACTION_DUP01, iMsg);
			TTestCaseFailed request(iTransferResult,iMsg);
			iControlEp0->SendRequest(request,this);
			}
			OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1230_POSTTRANSFERACTION_EXIT_DUP01, this );
			return;
		}
	}


TInt CUT_PBASE_T_USBDI_1230::ValidatePreviousAndPerformNextTransfers(TInt aTransferId)
		{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1230_VALIDATEPREVIOUSANDPERFORMNEXTTRANSFERS_ENTRY, this );
	
	TUint8 index = 0;
	switch(aTransferId)
		{
		case KBulkTransferInId0:
			index = 0;
			break;

		case KBulkTransferInId1:
			index = 1;
			break;
		
		default:
			_LIT(lit, "TRANSFER ID OUT OF RANGE");
			User::Panic(lit, KErrArgument);
			OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1230_VALIDATEPREVIOUSANDPERFORMNEXTTRANSFERS_EXIT, this );
			return 0; //should never get here
		}

	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_VALIDATEPREVIOUSANDPERFORMNEXTTRANSFERS, "\n");
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_VALIDATEPREVIOUSANDPERFORMNEXTTRANSFERS_DUP01, "Transfer[%d]", index);
	
	
	if(iNumBytesExpected[index] != 0)
		{
		TPtrC8 data1(iInTransfer[index]->DataPolled());
		if(ValidateData(data1, iValidateBufferPtr.Mid(iValidationStringStartPointTransfer[index], iNumBytesExpected[index])) == EFalse)
			{
			OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_VALIDATEPREVIOUSANDPERFORMNEXTTRANSFERS_DUP02, "=====VALIDATION FAILURE: Point of Validation String Entry %u, Newly Read Bytes %u=====",iValidationStringStartPointTransfer[index], iNumBytesExpected[index]);
			iIsValid = EFalse;
			}
		iNumBytesExpected[index] = 0; //reset
		}
	if(iNumBytesRequestedSoFar >= KTotalBytesToTransfer)
		//if we are near the end end the other transfer will mop up remaining bytes...
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_VALIDATEPREVIOUSANDPERFORMNEXTTRANSFERS_DUP03, "****ALL DONE for Transfer[%d]****", index);
		OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_1230_VALIDATEPREVIOUSANDPERFORMNEXTTRANSFERS_EXIT_DUP01, this, (TUint)KBulkTransferInId[index] );
		return KBulkTransferInId[index];
		}
	iValidationStringStartPointTransfer[index] = iNumBytesRequestedSoFar%(KLiteralEnglish5().Length()); //PRIOR TO THIS TRANSFER
	TUint bytesLeftToRead = KTotalBytesToTransfer - iNumBytesRequestedSoFar;
	iNumBytesExpected[index] = bytesLeftToRead < KHostNumReadBytes ? bytesLeftToRead : KHostNumReadBytes;
	iNumBytesRequestedSoFar += iNumBytesExpected[index];
	iInTransfer[index]->TransferIn(KHostNumReadBytes); //rely on ZLP to complete the last 'TransferIn'
	iExpectedNextTransferNumber = 1 - iExpectedNextTransferNumber;

	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1230_VALIDATEPREVIOUSANDPERFORMNEXTTRANSFERS_EXIT_DUP02, this );
	return 0;
	}

	
void CUT_PBASE_T_USBDI_1230::RequestNumBytesSent(TUint8 aTimerIndex)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1230_REQUESTNUMBYTESSENT_ENTRY, this );
	iInBufferPtr.Set(iInBuffer->Des());
	iInBufferPtr.Zero(); //reset
	iInBufferPtr.SetLength(KNumberStringLength);
	TInterfaceGetRecordedNumBytesReadInPayload request(1,1,iInBufferPtr);
	iControlEp0->SendRequest(request,this);
 	iControlEp0->LastRequestStartTime( iStartTime[aTimerIndex]);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1230_REQUESTNUMBYTESSENT_EXIT, this );
	}


void CUT_PBASE_T_USBDI_1230::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1230_EP0TRANSFERCOMPLETEL_ENTRY, this );
	
	OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d, test step = %d", aCompletionCode, iCaseStep);
	
	if(aCompletionCode != KErrNone)
		{
		if(iCaseStep == EFailed)
			{// ignore error, nad catch the TestFailed method called further down.
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_EP0TRANSFERCOMPLETEL_DUP01, "***Failure sending FAIL message to client on endpoint 0***");
			}
		else
			{
			TBuf<256> msg;
			KillTransfers();
			_LIT(lit, "<Error %d> Transfer to control endpoint 0 was not successful");
			msg.Format(lit,aCompletionCode);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_EP0TRANSFERCOMPLETEL_DUP02, msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(aCompletionCode,msg);
			iControlEp0->SendRequest(request,this);
			OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1230_EP0TRANSFERCOMPLETEL_EXIT, this );
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
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_EP0TRANSFERCOMPLETEL_DUP03, "Asking client for repeated 'Write'");
			iCaseStep = ERequestRepeatedWrite;	
			TRepeatedWriteDataRequest request(1,1,KLiteralEnglish5(),KDeviceNumWriteBytes,KTotalBytesToTransfer);// EP2 means endpoint index 2 not the actual endpoint number, here the ep with 32 byte max packet size
			iControlEp0->SendRequest(request,this);
			}
			break;
			
		case ERequestRepeatedWrite:
			{
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_EP0TRANSFERCOMPLETEL_DUP04, "Try to perform ALL transfers");
			
			iCaseStep = ETransfer;	
			iIsValid = ETrue; //innocent until proved guilty
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_EP0TRANSFERCOMPLETEL_DUP05, "\n");
			ValidatePreviousAndPerformNextTransfers(KBulkTransferInId[0]); //should not validate - just perform necessary transfers
			ValidatePreviousAndPerformNextTransfers(KBulkTransferInId[1]); //should not validate - just perform necessary transfers
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_EP0TRANSFERCOMPLETEL_DUP06, "\n");
			_LITDBG("Bulk test timer NOT instanciated");
			__ASSERT_DEBUG(iBulkTestTimer, User::Panic(lit, KErrGeneral));
			iBulkTestTimer->After(KRepeatedTimerInterval);
			}
			break;
			
		case ETransfer:
		// we must be getting num bytes read
			{
			ExtractDeviceReadBytes();
	
			//Restart timer
			_LITDBG("Bulk test timer NOT instanciated");
			__ASSERT_DEBUG(iBulkTestTimer, User::Panic(lit, KErrGeneral));
			iBulkTestTimer->After(KRepeatedTimerInterval);
			}
			break;
			
		case EDelayedTransferComplete:
			{
			PostTransferAction();
			}
			break;
			
		default:
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_EP0TRANSFERCOMPLETEL_DUP07, "<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1230_EP0TRANSFERCOMPLETEL_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_1230::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1230_TRANSFERCOMPLETEL_ENTRY, this );
	Cancel();
	
	iTransferResult = KErrNone;
	OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_TRANSFERCOMPLETEL, "Transfer completed (id=%d), aCompletionCode = %d, test step = %d",aTransferId, aCompletionCode, iCaseStep);


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
			if(aTransferId != KBulkTransferInId[0] && aTransferId != KBulkTransferInId[1])
				{
				iTransferComplete = 0; //reset
				iTransferResult = KUnexpectedTransferID;
				_LIT(lit, "<Error %d> Unexpected transfer ID, wanted %d or %d, got %d");
				iMsg.Format(lit, iTransferResult, KBulkTransferInId[0], KBulkTransferInId[1], aTransferId);
				break;
				}
		
			OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_TRANSFERCOMPLETEL_DUP01, "Transfer IN %d completed - num bytes requested = %u", aTransferId, iNumBytesRequestedSoFar);
	
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_TRANSFERCOMPLETEL_DUP02, "\n");
			iTransferComplete |= ValidatePreviousAndPerformNextTransfers(aTransferId);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_TRANSFERCOMPLETEL_DUP03, "\n");
			
			if(iTransferResult==KErrNone && (iTransferComplete & KBulkTransferIdMask) == KBulkTransferIdMask)
				{
				/*
				Transfers all complete - now check validation
				*/
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_TRANSFERCOMPLETEL_DUP04, "All Transfers Completed Successfully: Transfer Completion Aggregation Mask 0x%x", iTransferComplete);
				iBulkTestTimer->Cancel(); //Cancel Timer 
				iTransferResult = KTransferSuccess;
				if(!iIsValid)
					{
					iTransferResult = KErrCompletion; //indicates data validation failure
					iIsValid = ETrue; //reset
					}
			
				if(iTimingError == KErrTooBig)
					{
					__PRINT_CONTROL_TRANSFER_TIMER_COMPARISON_WARNING
					iTransferResult = KErrTooBig;
					iTimingError = KErrNone; //reset
					}
				
				if(KMaxBytesWrittenDiffPercentage*iDeviceMaxTimedNumBytesRead > KPercent*iDeviceMinTimedNumBytesRead)
					{
					OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_TRANSFERCOMPLETEL_DUP05, "Device APPARENTLY reading rate erratic:-");
					OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_TRANSFERCOMPLETEL_DUP06, "Min Timed Number of Bytes = %d", iDeviceMinTimedNumBytesRead);
					OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_TRANSFERCOMPLETEL_DUP07, "Max Timed Number of Bytes = %d", iDeviceMaxTimedNumBytesRead);
					iTransferResult = KErrTooBig;
					iDeviceMaxTimedNumBytesRead = 0;
					iDeviceMinTimedNumBytesRead = KMaxTUint;
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1230_TRANSFERCOMPLETEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_1230::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1230_DEVICEINSERTEDL_ENTRY, this );
	
	Cancel();
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_DEVICEINSERTEDL, "this - %08x", this);
	
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
	
			iInTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkMaxTransferSize,*this,KBulkTransferInId[0]);
			iInTransfer[1] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkMaxTransferSize,*this,KBulkTransferInId[1]);
			
			// Initialise the descriptors for transfer		
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_DEVICEINSERTEDL_DUP01, "Initialising the transfer descriptors - interface 1");
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
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1230_DEVICEINSERTEDL_DUP02, msg);
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1230_DEVICEINSERTEDL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_1230::HandleBulkTestTimerFired()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1230_HANDLEBULKTESTTIMERFIRED_ENTRY, this );
	if(iCaseStep == ETransfer)
		{
		RequestNumBytesSent(KTestTimer);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1230_HANDLEBULKTESTTIMERFIRED_EXIT, this );
	}
	
	} //end namespace
