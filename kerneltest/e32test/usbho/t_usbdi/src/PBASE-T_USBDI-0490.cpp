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
// @file PBASE-T_USBDI-0490.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0490.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0490Traces.h"
#endif


 

namespace NUnitTesting_USBDI
	{
_LIT8(KPayloadPattern,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"); // 52bytes

const TUint KBulkTransferSize = 52;
const TInt KBulkTransferInId = 1<<0;
const TInt KBulkTransferOutId = 1<<1;

const TInt KUnexpectedTransferID = -101;
const TInt KUndefinedStep	 = -102;


_LIT(KTestCaseId,"PBASE-T_USBDI-0490");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0490,TBool> CUT_PBASE_T_USBDI_0490::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0490* CUT_PBASE_T_USBDI_0490::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0490_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0490* self = new (ELeave) CUT_PBASE_T_USBDI_0490(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0490_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0490::CUT_PBASE_T_USBDI_0490(TBool aHostRole)
:	CBaseBulkTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0490_CUT_PBASE_T_USBDI_0490_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0490_CUT_PBASE_T_USBDI_0490_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0490::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0490_CONSTRUCTL_ENTRY, this );
	BaseBulkConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0490_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0490::~CUT_PBASE_T_USBDI_0490()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0490_CUT_PBASE_T_USBDI_0490_ENTRY_DUP01, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0490_CUT_PBASE_T_USBDI_0490_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0490::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0490_EP0TRANSFERCOMPLETEL_ENTRY, this );
	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0490_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{
		if(iCaseStep == EFailed)
			{// ignore error, nad catch the TestFailed method called further down.
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0490_EP0TRANSFERCOMPLETEL_DUP01, "***Failure sending FAIL message to client on endpoint 0***");
			}
		else
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0490_EP0TRANSFERCOMPLETEL_DUP02, msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(aCompletionCode,msg);
			iControlEp0->SendRequest(request,this);
			OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0490_EP0TRANSFERCOMPLETEL_EXIT, this );
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
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0490_EP0TRANSFERCOMPLETEL_DUP03, "Try to send data");
			iOutTransfer[0]->TransferOut(KPayloadPattern);
			break;
		
		case ETransferIn:
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0490_EP0TRANSFERCOMPLETEL_DUP04, "Try to receive data");
			iInTransfer[0]->TransferIn(KBulkTransferSize);
			break;
	
		default:
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0490_EP0TRANSFERCOMPLETEL_DUP05, "<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0490_EP0TRANSFERCOMPLETEL_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0490::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0490_TRANSFERCOMPLETEL_ENTRY, this );
	Cancel();
	
	TInt err(KErrNone);
	TBuf<256> msg;
	OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0490_TRANSFERCOMPLETEL, "Transfer completed (id=%d), aCompletionCode = %d",aTransferId, aCompletionCode);

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
			if(aTransferId != KBulkTransferOutId)
				{
				err = KUnexpectedTransferID;
				msg.Format(_L("<Error %d> Unexpected transfer ID, wanted %d, got %d"),
						       err, KBulkTransferOutId, aTransferId);
				break;
				}
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0490_TRANSFERCOMPLETEL_DUP01, "Try to receive back sent data");
			iCaseStep = ETransferIn;
			TWriteSynchronousCachedReadDataRequest request(1,1,1); //Use first read EP and first write EP (on interface 1)
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
			if(aTransferId != KBulkTransferInId)
				{
				err = KUnexpectedTransferID;
				msg.Format(_L("<Error %d> Unexpected transfer ID, wanted %d, got %d"),
						       err, KBulkTransferInId, aTransferId);
				break;
				}
			// Compare the data to what is expected
			if(ValidateData(iInTransfer[0]->DataPolled(), KPayloadPattern, KBulkTransferSize) == EFalse)
				{
				err = KErrCompletion; //indicates data validation failure
				msg.Format(_L("<Error %d> Bulk transfer IN data received does not match Bulk Transfer OUT data"), err);
				break;
				}
			if(err == KErrNone)
				{
				// Comparison is a match
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0490_TRANSFERCOMPLETEL_DUP02, "Comparison for IN transfer is a match");
				iCaseStep = EPassed;
				TTestCasePassed request;
				iControlEp0->SendRequest(request,this);
				break;
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
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0490_TRANSFERCOMPLETEL_DUP03, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);
		}	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0490_TRANSFERCOMPLETEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0490::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0490_DEVICEINSERTEDL_ENTRY, this );
	Cancel();
	
	if(BaseBulkDeviceInsertedL(aDeviceHandle) == EDeviceConfigurationError)
		// Prepare for response from control transfer to client
		{
		iCaseStep = EFailed;
		}
	
	// Create the bulk transfers	
	iInTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkTransferSize,*this,KBulkTransferInId);
	iOutTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkTransferSize,*this,KBulkTransferOutId);
	
	// Initialise the descriptors for transfer		
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0490_DEVICEINSERTEDL, "Initialising the transfer descriptors");
	TInt err = iUsbInterface1.InitialiseTransferDescriptors();
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to initialise transfer descriptors"),err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0490_DEVICEINSERTEDL_DUP01, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0490_DEVICEINSERTEDL_EXIT, this );
		return;
		}

	iCaseStep = ETransferOut;
	TEndpointReadUntilShortRequest request(1,1,KBulkTransferSize);// EP1 because 1st reader EP
	iControlEp0->SendRequest(request,this);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0490_DEVICEINSERTEDL_EXIT_DUP01, this );
	}
	
	} //end namespace
