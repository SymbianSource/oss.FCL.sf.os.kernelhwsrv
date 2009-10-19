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
// @file PBASE-T_USBDI-0494.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0494.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "testliterals.h"


 

namespace NUnitTesting_USBDI
	{
const TUint KBulkTransferSize = 1024*512; //512kB
const TUint KBulkNumReadBytes = 1024*600; //600kB
const TUint KBulkTransferMaxSize = KBulkNumReadBytes+KBulkTransferSize;
const TInt KBulkTranferInId = 1<<0;
const TInt KBulkTranferOutId = 1<<1;

const TInt KUnexpectedTransferID = -101;
const TInt KUndefinedStep	 = -102;


_LIT(KTestCaseId,"PBASE-T_USBDI-0494");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0494,TBool> CUT_PBASE_T_USBDI_0494::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0494* CUT_PBASE_T_USBDI_0494::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0494* self = new (ELeave) CUT_PBASE_T_USBDI_0494(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_0494::CUT_PBASE_T_USBDI_0494(TBool aHostRole)
:	CBaseBulkTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	} 


void CUT_PBASE_T_USBDI_0494::ConstructL()
	{
	BaseBulkConstructL();
	}


CUT_PBASE_T_USBDI_0494::~CUT_PBASE_T_USBDI_0494()
	{
	LOG_FUNC
	}

void CUT_PBASE_T_USBDI_0494::Ep0TransferCompleteL(TInt aCompletionCode)
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
			
		case ETransferOut:
			RDebug::Printf("Try to send data");
			iOutTransfer[0]->TransferOut(KLiteralEnglish8(), KBulkTransferSize, ETrue);
			break;
		
		case ETransferIn:
			RDebug::Printf("Try to receive data");
			iInTransfer[0]->TransferIn(KBulkNumReadBytes); //require completion using ZLP
			break;
	
		default:
			RDebug::Printf("<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	}
	
void CUT_PBASE_T_USBDI_0494::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	LOG_FUNC
	Cancel();
	
	TInt err(KErrNone);
	TBuf<256> msg;
	RDebug::Printf("Transfer completed (id=%d), aCompletionCode = %d",aTransferId, aCompletionCode);

	switch(iCaseStep)
		{
		case ETransferOut:
			{
			if(aCompletionCode != KErrNone)
				{
				err = KErrCorrupt;
				msg.Format(_L("<Error %d> No data sent on bulk OUT request"),aCompletionCode);
				break;
				}

			if(aTransferId != KBulkTranferOutId)
				{
				err = KUnexpectedTransferID;
				msg.Format(_L("<Error %d> Unexpected tranfer ID, wanted %d, got %d"),
						       err, KBulkTranferOutId, aTransferId);
				break;
				}

			RDebug::Printf("Try to receive back sent data");
			iCaseStep = ETransferIn;
			TWriteSynchronousCachedReadDataRequest request(1,1,1);  //Use first read EP and first write EP (on interface 1)
			iControlEp0->SendRequest(request,this);
			}
			break;

		case ETransferIn:
			{
			if(aCompletionCode != KErrNone)
				{
				err = KErrCorrupt;
				msg.Format(_L("<Error %d> No data sent on bulk IN request"),aCompletionCode);
				break;
				}

			if(aTransferId != KBulkTranferInId)
				{
				err = KUnexpectedTransferID;
				msg.Format(_L("<Error %d> Unexpected tranfer ID, wanted %d, got %d"),
						       err, KBulkTranferInId, aTransferId);
				break;
				}

			// Compare the data to what is expected
			if(ValidateData(iInTransfer[0]->DataPolled(), KLiteralEnglish8(), KBulkTransferSize) == EFalse)
				{
				err = KErrCompletion; //indicates data validation failure
				msg.Format(_L("<Error %d> Bulk transfer IN data received does not match Bulk Transfer OUT data"), err);
				break;
				}

			if(err == KErrNone)
				{
				// Comparison is a match
				RDebug::Printf("Comparison for IN transfer is a match");
				iCaseStep = EPassed;
				TTestCasePassed request;
				iControlEp0->SendRequest(request,this);
				}
			}
			break;

		default:
			err = KUndefinedStep;
			msg.Format(_L("<Error %d> Undefined case step %d reached"),KUndefinedStep, iCaseStep);
			break;
		}
	
	if(err!=KErrNone)
		{	
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);
		}	
	}
	
void CUT_PBASE_T_USBDI_0494::DeviceInsertedL(TUint aDeviceHandle)
	{
	LOG_FUNC
	Cancel();
	
	if(BaseBulkDeviceInsertedL(aDeviceHandle) == EDeviceConfigurationError)
		// Prepare for response from control transfer to client
		{
		iCaseStep = EFailed;
		}
	
	// Create the bulk transfers	
	iInTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkTransferMaxSize,*this,KBulkTranferInId);
	iOutTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkTransferMaxSize,*this,KBulkTranferOutId);
	
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

	iCaseStep = ETransferOut;	
	TEndpointReadUntilShortRequest request(1,1,KBulkTransferSize);// EP1 because 1st reader EP ??FIXME
	iControlEp0->SendRequest(request,this);
	}
	
	} //end namespace
