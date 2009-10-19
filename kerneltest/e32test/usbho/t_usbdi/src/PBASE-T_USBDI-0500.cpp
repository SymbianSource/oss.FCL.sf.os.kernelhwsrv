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
// @file PBASE-T_USBDI-0500.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0500.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "testliterals.h"


 

namespace NUnitTesting_USBDI
	{
const TInt KBulkMaxINTransferSize = 600;
const TInt KDeviceNumWriteBytesPreHalt = 256;
const TUint KHostNumReadBytesPreHalt1 = 512;
const TUint KHostNumReadBytesPreHalt2 = 512;
const TUint KHostNumReadBytesPostHalt1 = 256;
const TUint KHostNumReadBytesPostHalt2 = 257;
const TInt KDeviceNumWriteBytesPostHalt = 512;


//Make these single bit values ... 
// ... so that their completion can be easily recorded in a bit mask!
const TInt KBulkTransferInId0 = 1<<0;
const TInt KBulkTransferInId1 = 1<<4;

const TInt KUnexpectedTransferID = -101;
const TInt KUndefinedStep	 	 = -102;


_LIT(KTestCaseId,"PBASE-T_USBDI-0500");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0500,TBool> CUT_PBASE_T_USBDI_0500::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0500* CUT_PBASE_T_USBDI_0500::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0500* self = new (ELeave) CUT_PBASE_T_USBDI_0500(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_0500::CUT_PBASE_T_USBDI_0500(TBool aHostRole)
:	CBaseBulkTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	} 


void CUT_PBASE_T_USBDI_0500::ConstructL()
	{
	BaseBulkConstructL();
	}


CUT_PBASE_T_USBDI_0500::~CUT_PBASE_T_USBDI_0500()
	{
	LOG_FUNC
	}
	
	
void CUT_PBASE_T_USBDI_0500::Ep0TransferCompleteL(TInt aCompletionCode)
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
			
		case ETransferInHalt:
			RDebug::Printf("Try to receive data (pre halt)");
			iInTransfer[0]->TransferIn(KHostNumReadBytesPreHalt1);
			iInTransfer[1]->TransferIn(KHostNumReadBytesPreHalt2);
			break;
		
		case ETransferIn:
			RDebug::Printf("Try to receive data");
			iInTransfer[0]->TransferIn(KHostNumReadBytesPostHalt1);
			iInTransfer[1]->TransferIn(KHostNumReadBytesPostHalt2);
			break;
	
		default:
			RDebug::Printf("<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	}
	
void CUT_PBASE_T_USBDI_0500::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	LOG_FUNC
	Cancel();
	
	TInt err(KErrNone);
	TBuf<256> msg;
	RDebug::Printf("Transfer completed (id=%d), aCompletionCode = %d",aTransferId, aCompletionCode);

	switch(iCaseStep)
		{
		case ETransferInHalt:
			{
			if(aCompletionCode != KErrUsbStalled)
				{
				iInTransfer[0]->Cancel();
				iInTransfer[1]->Cancel();
				err = KErrCorrupt;
				msg.Format(_L("<Error %d> The transfer completed with no errors but the endpoint should have halted"),aCompletionCode);
				break; //switch(iCaseStep)
				}
	
			switch(aTransferId)
				{
				case KBulkTransferInId0:
				case KBulkTransferInId1:
					iTransferComplete |= aTransferId;
					RDebug::Printf("Transfer %d completed", aTransferId);
					break; //switch(aTransferId)

				default:
					iTransferComplete = 0; //reset
					err = KUnexpectedTransferID;
					msg.Format(_L("<Error %d> Unexpected transfer ID, wanted %d or %d, got %d"),
							       err, KBulkTransferInId0, KBulkTransferInId1, aTransferId);
					break; //switch(aTransferId)
				}

			if(err==KErrNone && iTransferComplete == (KBulkTransferInId0 | KBulkTransferInId1))
				{
				RDebug::Printf("Clear halt and try to send data again. Transfers Completed %d", iTransferComplete);
				iTransferComplete = 0; //reset
				// Acknowledge the stall and clear				
				err = iTestPipeInterface1BulkIn.ClearRemoteStall();
				if(err != KErrNone)
					{
					msg.Format(_L("<Error %d> The remote stall cannot be cleared"),err);
					break; //switch(iCaseStep)
					}
				iCaseStep = ETransferIn;
				TEndpointPatternSynchronousWriteRequest request(1,1, KLiteralEnglish8().Mid(0,KDeviceNumWriteBytesPostHalt), KDeviceNumWriteBytesPostHalt);// EP1 because 1st writter EP
				iControlEp0->SendRequest(request,this);	
				}
			}
			break; //switch(iCaseStep)

		case ETransferIn:
			{
			if(aCompletionCode != KErrNone)
				{
				iInTransfer[0]->Cancel();
				iInTransfer[1]->Cancel();
				err = KErrCorrupt;			
				
				msg.Format(_L("<Error %d> No data sent on bulk IN request"),aCompletionCode);
				break; //switch(iCaseStep)
				}
	
			switch(aTransferId)
				{
				case KBulkTransferInId0:
				case KBulkTransferInId1:
					iTransferComplete |= aTransferId;
					RDebug::Printf("Transfer %d completed", aTransferId);
					break; //switch(aTransferId)

				default:
					iTransferComplete = 0; //reset
					err = KUnexpectedTransferID;
					msg.Format(_L("<Error %d> Unexpected transfer ID, wanted %d or %d, got %d"),
							       err, KBulkTransferInId0, KBulkTransferInId1, aTransferId);
					break; //switch(aTransferId)
				}
			
			if(err==KErrNone && iTransferComplete == (KBulkTransferInId0 | KBulkTransferInId1))
				{
				// ok, compare data rcvd now
				iTransferComplete = 0; //reset
				TPtrC8 data1(iInTransfer[0]->DataPolled());		
				TPtrC8 data2(iInTransfer[1]->DataPolled());		
				//Validate first transfer for number of bytes requested.
				if(ValidateData(data1, KLiteralEnglish8(), KHostNumReadBytesPostHalt1) == EFalse)
					{
					err = KErrCompletion; //indicates data validation failure
					break; //switch(iCaseStep)
					}

				//Validate second transfer for the remainder of bytes that the device should have written.
				//NB The number of bytes requested is more than this remainder to accommodate the ZLP.
				if(ValidateData(data2, KLiteralEnglish8(), KHostNumReadBytesPostHalt1, KDeviceNumWriteBytesPostHalt-KHostNumReadBytesPostHalt1) == EFalse)
					{
					err = KErrCompletion; //indicates data validation failure
					break; //switch(iCaseStep)
					}

				// Comparison is a match
				RDebug::Printf("Comparison for IN transfer is a match");
				iCaseStep = EPassed;
				TTestCasePassed request;
				iControlEp0->SendRequest(request,this);
				}
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
	
void CUT_PBASE_T_USBDI_0500::DeviceInsertedL(TUint aDeviceHandle)
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
		msg.Format(_L("Base class DeviceInsertedL failed"));
		}
	else
		{
		iInTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkMaxINTransferSize,*this,KBulkTransferInId0);
		iInTransfer[1] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkMaxINTransferSize,*this,KBulkTransferInId1);
		
		// Initialise the descriptors for transfer		
		RDebug::Printf("Initialising the transfer descriptors");
		err = iUsbInterface1.InitialiseTransferDescriptors();
		if(err != KErrNone)
			{
			msg.Format(_L("<Error %d> Unable to initialise transfer descriptors"),err);
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
		RDebug::Printf("Asking client for 'Write' and 'Halt'");
		iCaseStep = ETransferInHalt;	
		TEndpointPatternSynchronousWriteAndHaltRequest request(1,1,KLiteralFrench4(),KDeviceNumWriteBytesPreHalt);// EP1 means endpoint index 1 not the actual endpoint number
		iControlEp0->SendRequest(request,this);
		}
	}
	
	} //end namespace
