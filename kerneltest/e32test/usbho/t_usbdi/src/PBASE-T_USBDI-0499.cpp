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
// @file PBASE-T_USBDI-0499.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0499.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "testliterals.h"


 

namespace NUnitTesting_USBDI
	{
const TInt KBulkMaxINTransferSize = 1200;
const TInt KBulkMaxOUTTransferSize = 150000;
const TUint KHostNumWriteBytesPreHalt1 = 128000;
const TUint KHostNumWriteBytesPreHalt2 = 128000;
const TUint KHostNumWriteBytesPostHalt1 = 512;
const TUint KHostNumWriteBytesPostHalt2 = 512;
const TInt KDeviceNumWrittenBytesPreHalt = KHostNumWriteBytesPreHalt1+KHostNumWriteBytesPreHalt2;
const TInt KDeviceNumReadBytesPostHalt = KHostNumWriteBytesPostHalt1+KHostNumWriteBytesPostHalt2;
const TInt KDeviceNumReadBytesPreHalt = 128;
const TInt KHostNumReadBytesPostHalt = KDeviceNumReadBytesPostHalt+1; //read cached data, allow 1 for ZLP


//Make these single bit values ... 
// ... so that their completion can be easily recorded in a bit mask!
const TInt KBulkTransferInId0 = 1<<0;
const TInt KBulkTransferOutId0 = 1<<3;
const TInt KBulkTransferOutId1 = 1<<4;

const TInt KUnexpectedTransferID = -101;
const TInt KUndefinedStep	 = -102;


_LIT(KTestCaseId,"PBASE-T_USBDI-0499");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0499,TBool> CUT_PBASE_T_USBDI_0499::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0499* CUT_PBASE_T_USBDI_0499::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0499* self = new (ELeave) CUT_PBASE_T_USBDI_0499(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_0499::CUT_PBASE_T_USBDI_0499(TBool aHostRole)
:	CBaseBulkTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	} 


void CUT_PBASE_T_USBDI_0499::ConstructL()
	{
	BaseBulkConstructL();
	}


CUT_PBASE_T_USBDI_0499::~CUT_PBASE_T_USBDI_0499()
	{
	LOG_FUNC
	}
	
	
void CUT_PBASE_T_USBDI_0499::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	LOG_FUNC
	
	RDebug::Printf("Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{
		if(iCaseStep == EFailed)
			{// ignore error, nad catch the TestFailed method called further down.
			RDebug::Printf("***Failure sending FAIL message to client on endpoint 0***");
			}
		else
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
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
			
		case ETransferOutHalt:
			iOutTransfer[0]->TransferOut(KLiteralFrench4(), KHostNumWriteBytesPreHalt1, EFalse);
			iOutTransfer[1]->TransferOut(KLiteralFrench4(), KHostNumWriteBytesPreHalt2, EFalse);
			break;
		
		case EAwaitClearPreHalt:
			{
			RDebug::Printf("Client has been asked to clear endpoint buffer");
			User::After(1000000); //Give client time to clear buffer
			TEndpointCancelReadRequest request(1,1);
			iControlEp0->SendRequest(request,this);
			iCaseStep = EAwaitCancelRead;
			}
			break;

		case EAwaitCancelRead:
			{
			TEndpointReadRequest request(1,1, KDeviceNumReadBytesPostHalt);// EP1 because 1st writter EP
			iControlEp0->SendRequest(request,this);
			iCaseStep = ETransferOut;
			}
			break;
			
		case ETransferOut:
			RDebug::Printf("Try to send data (post halt)");
			iOutTransfer[0]->TransferOut(KLiteralEnglish8().Mid(0, KHostNumWriteBytesPostHalt1), EFalse);
			iOutTransfer[1]->TransferOut(KLiteralEnglish8().Mid(KHostNumWriteBytesPostHalt1, KHostNumWriteBytesPostHalt2), EFalse);
			break;
		
		case ETransferIn:
			RDebug::Printf("Try to receive data");
			iInTransfer[0]->TransferIn(KHostNumReadBytesPostHalt);
			break;
	
		default:
			RDebug::Printf("<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	}
	
void CUT_PBASE_T_USBDI_0499::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	LOG_FUNC
	Cancel();
	
	TInt err(KErrNone);
	TBuf<256> msg;
	RDebug::Printf("Transfer completed (id=%d), aCompletionCode = %d",aTransferId, aCompletionCode);

	switch(iCaseStep)
		{
		case ETransferOutHalt:
			{
			if(aCompletionCode != KErrUsbStalled)
				{
				iOutTransfer[0]->Cancel();
				iOutTransfer[1]->Cancel();
				err = KErrCorrupt;
				msg.Format(_L("<Error %d> The transfer completed with no errors but the endpoint should have halted"),aCompletionCode);
				break; //switch(iCaseStep)
				}
	
			switch(aTransferId)
				{
				case KBulkTransferOutId0:
				case KBulkTransferOutId1:
					iTransferComplete |= aTransferId;
					RDebug::Printf("Transfer %d stalled", aTransferId);
					break; //switch(aTransferId)
				
				default:
					iTransferComplete = 0; //reset
					err = KUnexpectedTransferID;
					msg.Format(_L("<Error %d> Unexpected transfer ID, wanted %d or %d, got %d"),
							       err, KBulkTransferOutId0, KBulkTransferOutId1, aTransferId);
					break; //switch(aTransferId)
				}

			if(err==KErrNone && iTransferComplete == (KBulkTransferOutId0 | KBulkTransferOutId1))
				{
				RDebug::Printf("Clear halt and try to send data again. Transfers Completed %d", iTransferComplete);
				// Acknowledge the stall and clear				
				err = iTestPipeInterface1BulkOut.ClearRemoteStall();
				if(err != KErrNone)
					{
					msg.Format(_L("<Error %d> The remote stall cannot be cleared"),err);
					break; //switch(iCaseStep)
					}
				iCaseStep = EAwaitClearPreHalt;
				TEndpointReadRequest request(1,1, KDeviceNumWrittenBytesPreHalt);// EP1 because 1st writter EP
				iControlEp0->SendRequest(request,this);	
				iTransferComplete = 0; //reset
				}
			}
			break; //switch(iCaseStep)

		case ETransferOut:
			{
			if(aCompletionCode != KErrNone)
				{
				iOutTransfer[0]->Cancel();
				iOutTransfer[1]->Cancel();
				err = KErrCorrupt;
				msg.Format(_L("<Error %d> No data sent on bulk OUT request"),aCompletionCode);
				break; //switch(iCaseStep)
				}
	
			switch(aTransferId)
				{
				case KBulkTransferOutId0:
				case KBulkTransferOutId1:
					iTransferComplete |= aTransferId;
					RDebug::Printf("Transfer %d completed", aTransferId);
					break; //switch(aTransferId)

				default:
					iTransferComplete = 0; //reset
					err = KUnexpectedTransferID;
					msg.Format(_L("<Error %d> Unexpected transfer ID, wanted %d or %d, got %d"),
							       err, KBulkTransferOutId0, KBulkTransferOutId1, aTransferId);
					break; //switch(aTransferId)
				}

			if(err==KErrNone && iTransferComplete == (KBulkTransferOutId0 | KBulkTransferOutId1))
				{
				RDebug::Printf("Try to receive back sent data. Transfers Completed %d", iTransferComplete);
				iCaseStep = ETransferIn;
				TWriteSynchronousCachedReadDataRequest request(1,1,1);
				iControlEp0->SendRequest(request,this);	
				iTransferComplete = 0; //reset
				}
			break; //switch(iCaseStep)

		case ETransferIn:
			if(aCompletionCode != KErrNone)
				{
				err = KErrCorrupt;
				msg.Format(_L("<Error %d> No data sent on bulk IN request"),aCompletionCode);
				break; //switch(iCaseStep)
				}
	
			if(aTransferId != KBulkTransferInId0)
				{
				iTransferComplete = 0; //reset
				err = KUnexpectedTransferID;
				msg.Format(_L("<Error %d> Unexpected transfer ID, wanted %d , got %d"),
					       err, KBulkTransferInId0, aTransferId);
				break; //switch(iCaseStep)
				}
				
			// else ok, compare data rcvd now
			TPtrC8 data1(iInTransfer[0]->DataPolled());		
			if(ValidateData(data1, KLiteralEnglish8().Mid(0, KDeviceNumReadBytesPostHalt)) == EFalse)
				{
				err = KErrCompletion; //indicates data validation failure
				msg.Format(_L("<Error %d> Bulk transfer IN data received does not match Bulk Transfer OUT data"), err);
				break; //switch(iCaseStep)
				}
			
			// Comparison is a match
			RDebug::Printf("Comparison for IN transfer is a match");
			iCaseStep = EPassed;
			TTestCasePassed request;
			iControlEp0->SendRequest(request,this);
			iTransferComplete = 0; //reset
			}
			break; //switch(iCaseStep)

		default:
			err = KUndefinedStep;
			msg.Format(_L("<Error %d> Undefined case step %d reached"),KUndefinedStep, iCaseStep);
			break; //switch(iCaseStep)
		}
	
	if(err == KErrCompletion)
		//indicates data validation failure
		{
		msg.Format(_L("<Error %d> Bulk transfer IN data received does not match Bulk Transfer OUT data"), err);
		}
	
	if(err!=KErrNone)
		{	
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);
		}	
	}
	
void CUT_PBASE_T_USBDI_0499::DeviceInsertedL(TUint aDeviceHandle)
	{
	LOG_FUNC
	
	Cancel();
	
	if(BaseBulkDeviceInsertedL(aDeviceHandle) == EDeviceConfigurationError)
		// Prepare for response from control transfer to client
		{
		iCaseStep = EFailed;
		}
	
	// Create the bulk transfers	
	iInTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkMaxINTransferSize,*this,KBulkTransferInId0);
	iOutTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkMaxOUTTransferSize,*this,KBulkTransferOutId0);
	iOutTransfer[1] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkMaxOUTTransferSize,*this,KBulkTransferOutId1);
	
	// Initialise the descriptors for transfer		
	RDebug::Printf("Initialising the transfer descriptors");
	TInt err = iUsbInterface1.InitialiseTransferDescriptors();
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to initialise transfer descriptors"),err);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		return;
		}

	iCaseStep = ETransferOutHalt;	
	TEndpointReadAndHaltRequest request(1,1,KDeviceNumReadBytesPreHalt);// EP1 means endpoint index 1 not the actual endpoint number
	iControlEp0->SendRequest(request,this);
	}
	
	} //end namespace
