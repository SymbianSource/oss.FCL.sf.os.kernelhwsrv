// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
	CUT_PBASE_T_USBDI_1230* self = new (ELeave) CUT_PBASE_T_USBDI_1230(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_1230::CUT_PBASE_T_USBDI_1230(TBool aHostRole)
:	CBaseBulkTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	} 


void CUT_PBASE_T_USBDI_1230::ConstructL()
	{
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
	
	RDebug::Printf("CUT_PBASE_T_USBDI_1230::ConstructL(): buffer created");
	}


CUT_PBASE_T_USBDI_1230::~CUT_PBASE_T_USBDI_1230()
	{
	LOG_FUNC
	}
	
void CUT_PBASE_T_USBDI_1230::KillTransfers()
	{
	LOG_FUNC
	
	iInTransfer[0]->Cancel();
	iInTransfer[1]->Cancel();
	}

void CUT_PBASE_T_USBDI_1230::ExtractDeviceReadBytes()
	{
	LOG_FUNC
	
	iControlEp0->LastRequestCompletionTime( iEndTime[KTestTimer]);
	iTimingError = iTimingError == KErrNone ? CheckTimes(KBaseTimer, KTestTimer, KMaxTimeDiffPercentage) : iTimingError;
	ResetTimes(KTestTimer);
	
	RDebug::Printf("Collect client's return of the number of bytes written on its bulk in endpoint ...");
	TLex8 lex(iInBufferPtr.Left(KNumberStringLength));
	TUint32 numBytes = 0;
	User::LeaveIfError(lex.Val(numBytes, EDecimal));
	RDebug::Printf("********************NUM*BYTES****************************");
	RDebug::Printf("         NUM BYTES READ BY CLIENT ==== %d ====           ", numBytes);
	RDebug::Printf("********************NUM*BYTES****************************");
	RDebug::Printf("\n");

	if(numBytes != 0)
		//Do not count this case - it may result from the remote resetting when all bulk transfers have completed
		{
		TUint numBytesSinceLast = numBytes - iDeviceNumBytesReadInTotal;
		iDeviceNumBytesReadInTotal = numBytes;
		iDeviceMinTimedNumBytesRead = numBytesSinceLast < iDeviceMinTimedNumBytesRead ?  numBytesSinceLast : iDeviceMinTimedNumBytesRead ;
		iDeviceMaxTimedNumBytesRead = numBytesSinceLast > iDeviceMaxTimedNumBytesRead ?  numBytesSinceLast : iDeviceMaxTimedNumBytesRead ;;
		}
	}


void CUT_PBASE_T_USBDI_1230::PostTransferAction()
	{
	switch(iTransferResult)
		{
		case KErrNone:
			//do nothing
			return;

		case KTransferSuccess:
			{
			// Indicates success - comparison is a match
			RDebug::Printf("Comparison for IN transfer is a match");
			iCaseStep = EPassed;
			TTestCasePassed request;
			iTransferComplete = 0; //reset
			iControlEp0->SendRequest(request,this);
			}
			return; 
			
			
		default:
			{
			iCaseStep = EFailed;
			RDebug::Print(iMsg);
			TTestCaseFailed request(iTransferResult,iMsg);
			iControlEp0->SendRequest(request,this);
			}
			return;
		}
	}


TInt CUT_PBASE_T_USBDI_1230::ValidatePreviousAndPerformNextTransfers(TInt aTransferId)
		{
	LOG_FUNC
	
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
			return 0; //should never get here
		}

	RDebug::Printf("\n");
	RDebug::Printf("Transfer[%d]", index);
	
	
	if(iNumBytesExpected[index] != 0)
		{
		TPtrC8 data1(iInTransfer[index]->DataPolled());
		if(ValidateData(data1, iValidateBufferPtr.Mid(iValidationStringStartPointTransfer[index], iNumBytesExpected[index])) == EFalse)
			{
			RDebug::Printf("=====VALIDATION FAILURE: Point of Validation String Entry %d, Newly Read Bytes %d=====",iValidationStringStartPointTransfer[index], iNumBytesExpected[index]);
			iIsValid = EFalse;
			}
		iNumBytesExpected[index] = 0; //reset
		}
	if(iNumBytesRequestedSoFar >= KTotalBytesToTransfer)
		//if we are near the end end the other transfer will mop up remaining bytes...
		{
		RDebug::Printf("****ALL DONE for Transfer[%d]****", index);
		return KBulkTransferInId[index];
		}
	iValidationStringStartPointTransfer[index] = iNumBytesRequestedSoFar%(KLiteralEnglish5().Length()); //PRIOR TO THIS TRANSFER
	TUint bytesLeftToRead = KTotalBytesToTransfer - iNumBytesRequestedSoFar;
	iNumBytesExpected[index] = bytesLeftToRead < KHostNumReadBytes ? bytesLeftToRead : KHostNumReadBytes;
	iNumBytesRequestedSoFar += iNumBytesExpected[index];
	iInTransfer[index]->TransferIn(KHostNumReadBytes); //rely on ZLP to complete the last 'TransferIn'
	iExpectedNextTransferNumber = 1 - iExpectedNextTransferNumber;

	return 0;
	}

	
void CUT_PBASE_T_USBDI_1230::RequestNumBytesSent(TUint8 aTimerIndex)
	{
	iInBufferPtr.Set(iInBuffer->Des());
	iInBufferPtr.Zero(); //reset
	iInBufferPtr.SetLength(KNumberStringLength);
	TInterfaceGetRecordedNumBytesReadInPayload request(1,1,iInBufferPtr);
	iControlEp0->SendRequest(request,this);
 	iControlEp0->LastRequestStartTime( iStartTime[aTimerIndex]);
	}


void CUT_PBASE_T_USBDI_1230::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	LOG_FUNC
	
	RDebug::Printf("Ep0TransferCompleteL with aCompletionCode = %d, test step = %d", aCompletionCode, iCaseStep);
	
	if(aCompletionCode != KErrNone)
		{
		if(iCaseStep == EFailed)
			{// ignore error, nad catch the TestFailed method called further down.
			RDebug::Printf("***Failure sending FAIL message to client on endpoint 0***");
			}
		else
			{
			TBuf<256> msg;
			KillTransfers();
			_LIT(lit, "<Error %d> Transfer to control endpoint 0 was not successful");
			msg.Format(lit,aCompletionCode);
			RDebug::Print(msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(aCompletionCode,msg);
			iControlEp0->SendRequest(request,this);
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
			RDebug::Printf("Asking client for repeated 'Write'");
			iCaseStep = ERequestRepeatedWrite;	
			TRepeatedWriteDataRequest request(1,1,KLiteralEnglish5(),KDeviceNumWriteBytes,KTotalBytesToTransfer);// EP2 means endpoint index 2 not the actual endpoint number, here the ep with 32 byte max packet size
			iControlEp0->SendRequest(request,this);
			}
			break;
			
		case ERequestRepeatedWrite:
			{
			RDebug::Printf("Try to perform ALL transfers");
			
			iCaseStep = ETransfer;	
			iIsValid = ETrue; //innocent until proved guilty
			RDebug::Printf("\n");
			ValidatePreviousAndPerformNextTransfers(KBulkTransferInId[0]); //should not validate - just perform necessary transfers
			ValidatePreviousAndPerformNextTransfers(KBulkTransferInId[1]); //should not validate - just perform necessary transfers
			RDebug::Printf("\n");
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
			RDebug::Printf("<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	}
	
void CUT_PBASE_T_USBDI_1230::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	LOG_FUNC
	Cancel();
	
	iTransferResult = KErrNone;
	RDebug::Printf("Transfer completed (id=%d), aCompletionCode = %d, test step = %d",aTransferId, aCompletionCode, iCaseStep);


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
		
			RDebug::Printf("Transfer IN %d completed - num bytes requested = %d", aTransferId, iNumBytesRequestedSoFar);
	
			RDebug::Printf("\n");
			iTransferComplete |= ValidatePreviousAndPerformNextTransfers(aTransferId);
			RDebug::Printf("\n");
			
			if(iTransferResult==KErrNone && (iTransferComplete & KBulkTransferIdMask) == KBulkTransferIdMask)
				{
				/*
				Transfers all complete - now check validation
				*/
				RDebug::Printf("All Transfers Completed Successfully: Transfer Completion Aggregation Mask 0x%x", iTransferComplete);
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
					RDebug::Printf("Device APPARENTLY reading rate erratic:-");
					RDebug::Printf("Min Timed Number of Bytes = %d", iDeviceMinTimedNumBytesRead);
					RDebug::Printf("Max Timed Number of Bytes = %d", iDeviceMaxTimedNumBytesRead);
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
	}
	
void CUT_PBASE_T_USBDI_1230::DeviceInsertedL(TUint aDeviceHandle)
	{
	LOG_FUNC
	
	Cancel();
	RDebug::Printf("this - %08x", this);
	
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
			RDebug::Printf("Initialising the transfer descriptors - interface 1");
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
		RDebug::Print(msg);
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
	}

void CUT_PBASE_T_USBDI_1230::HandleBulkTestTimerFired()
	{
	if(iCaseStep == ETransfer)
		{
		RequestNumBytesSent(KTestTimer);
		}
	}
	
	} //end namespace
