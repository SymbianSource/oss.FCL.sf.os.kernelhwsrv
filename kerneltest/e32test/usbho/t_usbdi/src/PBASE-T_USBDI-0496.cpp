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
// @file PBASE-T_USBDI-0496.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0496.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "testliterals.h"


 

namespace NUnitTesting_USBDI
	{
const TUint KTotalBytesToTransfer = 2*1024*1024+511; //2MB + 511 bytes
const TUint KHostNumReadBytes = 1024*256;
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


_LIT(KTestCaseId,"PBASE-T_USBDI-0496");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0496,TBool> CUT_PBASE_T_USBDI_0496::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0496* CUT_PBASE_T_USBDI_0496::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0496* self = new (ELeave) CUT_PBASE_T_USBDI_0496(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_0496::CUT_PBASE_T_USBDI_0496(TBool aHostRole)
:	CBaseBulkTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	} 


void CUT_PBASE_T_USBDI_0496::ConstructL()
	{
	BaseBulkConstructL();

	iInBuffer = HBufC8::NewL(KTestBufferLength);

	//Create buffer to contain two lots of the payload pattern
	//..so that we may grab cyclic chunks of said payload pattern
	TInt repeats = KHostNumReadBytes / (KLiteralEnglish5().Length()) + 1 + 1; //1 extra to accommodate start point plus 1 to accomodate remainder in division
	iValidateBuffer = HBufC8::NewL(KLiteralEnglish5().Length() * repeats);
	iValidateBufferPtr.Set(iValidateBuffer->Des());
	iValidateBufferPtr.Zero();
	for(TInt i=0;i<repeats;i++)
		{
		iValidateBufferPtr.Append(KLiteralEnglish5());
		}
	
	RDebug::Printf("CUT_PBASE_T_USBDI_0496::ConstructL(): buffer created");
	}


CUT_PBASE_T_USBDI_0496::~CUT_PBASE_T_USBDI_0496()
	{
	LOG_FUNC
	}
	
void CUT_PBASE_T_USBDI_0496::KillTransfers()
	{
	LOG_FUNC
	
	iInTransfer[0]->Cancel();
	iInTransfer[1]->Cancel();
	}
	
	
	
void CUT_PBASE_T_USBDI_0496::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	LOG_FUNC
	
	RDebug::Printf("Ep0TransferCompleteL with aCompletionCode = %d", aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{
		if(iCaseStep == EFailed)
			{// ignore error, nad catch the TestFailed method called further down.
			RDebug::Printf("***Failure sending FAIL message to client on endpoint 0***");
			}
		else
			{
			KillTransfers();
			TBuf<256> msg;
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
		
		case ERequestRepeatedWrite:
			{
			RDebug::Printf("Try to perform ALL transfers");
			
			iCaseStep = ETransfer;	
			iIsValid = ETrue; //innocent until proved guilty
			RDebug::Printf("\n");
			iTransferComplete |= ValidatePreviousAndPerformNextTransfers(KBulkTransferInId[0]); //should not validate - just perform necessary transfers
			iTransferComplete |= ValidatePreviousAndPerformNextTransfers(KBulkTransferInId[1]); //should not validate - just perform necessary transfers
			RDebug::Printf("\n");
			if((iTransferComplete & KBulkTransferIdMask) == KBulkTransferIdMask)
				{
				iTransferComplete = 0; //reset
				_LIT(lit, "TEST FAILURE: No data to send!!");
				TBuf<40> msg(lit);
				RDebug::Print(msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(KErrAbort,msg);
				return iControlEp0->SendRequest(request,this);
				}
			}
			break;

		default:
			RDebug::Printf("<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	}
	
void CUT_PBASE_T_USBDI_0496::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	LOG_FUNC
	Cancel();
	
	TInt err(KErrNone);
	TBuf<256> msg;
	RDebug::Printf("Transfer completed (id=%d), aCompletionCode = %d, test step = %d",aTransferId, aCompletionCode, iCaseStep);


	switch(iCaseStep)
		{
		case ETransfer:
			{
			if(aCompletionCode != KErrNone)
				{
				KillTransfers();
				err = KErrCorrupt;
				_LIT(lit, "<Error %d> The transfer completed with an error.");
				msg.Format(lit, aCompletionCode);
				break;
				}
			if(aTransferId != KBulkTransferInId[0] && aTransferId != KBulkTransferInId[1])
				{
				iTransferComplete = 0; //reset
				err = KUnexpectedTransferID;
				_LIT(lit, "<Error %d> Unexpected transfer ID, wanted %d or %d, got %d");
				msg.Format(lit, err, KBulkTransferInId[0], KBulkTransferInId[1], aTransferId);
				break;
				}
	
			RDebug::Printf("\n");
			iTransferComplete |= ValidatePreviousAndPerformNextTransfers(aTransferId);
			RDebug::Printf("\n");
			
			if(err==KErrNone && (iTransferComplete & KBulkTransferIdMask) == KBulkTransferIdMask)
				{
				/*
				Transfers all complete - check all were valid, and if so pass the test
				*/
				RDebug::Printf("All Transfers Completed Successfully: Transfer Completion Aggregation Mask 0x%x", iTransferComplete);
				if(!iIsValid)
					{
					err = KErrCompletion; //indicates data validation failure
					iIsValid = ETrue; //reset
					break;
					}
				// Comparison is a match
				RDebug::Printf("Comparison for IN transfer is a match");
				iCaseStep = EPassed;
				TTestCasePassed request;
				iControlEp0->SendRequest(request,this);
				iTransferComplete = 0; //reset
				}
			}
			break;

		default:
			err = KUndefinedStep;
			_LIT(lit, "<Error %d> Undefined case step %d reached");
			msg.Format(lit,KUndefinedStep, iCaseStep);
			break;
		}


	if(err == KErrCompletion)
		//indicates data validation failure
		{
		_LIT(lit, "<Error %d> Bulk transfer IN data received does not match the data expected");
		msg.Format(lit, err);
		}

	if(err!=KErrNone)
		{	
		KillTransfers();
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);
		}	
	}
	
void CUT_PBASE_T_USBDI_0496::DeviceInsertedL(TUint aDeviceHandle)
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
		RDebug::Printf("Asking client for 'Read' on interface 1");
		iCaseStep = ERequestRepeatedWrite;	
		TRepeatedWriteDataRequest request(1,1,KLiteralEnglish5(),KDeviceNumWriteBytes,KTotalBytesToTransfer);// EP2 means endpoint index 2 not the actual endpoint number, here the ep with 32 byte max packet size
		iControlEp0->SendRequest(request,this);
		}
	}

TUint8 CUT_PBASE_T_USBDI_0496::Index(TUint8 aTransferId)
	{
	switch(aTransferId)
		{
		case KBulkTransferInId0:
			return 0;
		case KBulkTransferInId1:
			return 1;
		default:
			_LIT(lit, "TRANSFER ID OUT OF RANGE");
			User::Panic(lit, KErrArgument);
			return 0; //should never get here
		}
	}

TInt CUT_PBASE_T_USBDI_0496::ValidatePreviousAndPerformNextTransfers(TInt aTransferId)
/**
@param 	aTransferId - specifies trasnfer to use
@return	zero UNLESS no more trasnfers have been queued for the specified ID in which case return the transfer ID
Note:	This return value allows the caller to respond easily if no more transfers are required for the specified 
		transfer ID.
*/
	{
	LOG_FUNC
	
	TUint8 index = Index(aTransferId);
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
		//if we are near the end the other transfer will mop up remaining bytes...
		{
		RDebug::Printf("****ALL DONE for Transfer[%d]****", index);
		return aTransferId; //tell caller that all transfers are complete for this transfer ID
		}

	iValidationStringStartPointTransfer[index] = iNumBytesRequestedSoFar%(KLiteralEnglish5().Length()); //PRIOR TO THIS TRANSFER
	TUint bytesLeftToRead = KTotalBytesToTransfer - iNumBytesRequestedSoFar;
	iNumBytesExpected[index] = bytesLeftToRead < KHostNumReadBytes ? bytesLeftToRead : KHostNumReadBytes;
	iNumBytesRequestedSoFar += iNumBytesExpected[index];
	iInTransfer[index]->TransferIn(KHostNumReadBytes); //rely on ZLP to complete the last 'TransferIn'
	iExpectedNextTransferNumber = 1 - iExpectedNextTransferNumber;

	return 0; //tell caller there are still transfers to complete for the requested transfer ID
	}

	} //end namespace
