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
// @file PBASE-T_USBDI-0495.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0495.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "testliterals.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0495Traces.h"
#endif


 

namespace NUnitTesting_USBDI
	{
const TUint KTotalBytesToTransfer = 2*1024*1024+511; //2MB + 511 bytes
const TUint KHostNumWriteBytes = 1024*256;
const TInt KBulkMaxTransferSize = KHostNumWriteBytes + 1000;
const TInt KDeviceNumReadBytes = 1024;


//Make these single bit values ... 
// ... so that their completion can be easily recorded in a bit mask!
const TUint32 KBulkTransferOutId[KMaxNumOutTransfers] = {1<<0, 1<<1};
const TUint32 KBulkTransferIdMask = KBulkTransferOutId[0] | KBulkTransferOutId[1];

const TInt KUndefinedStep	 		= -102;
const TInt KUnexpectedTransferID 	= -103;



_LIT(KTestCaseId,"PBASE-T_USBDI-0495");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0495,TBool> CUT_PBASE_T_USBDI_0495::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0495* CUT_PBASE_T_USBDI_0495::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0495_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0495* self = new (ELeave) CUT_PBASE_T_USBDI_0495(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0495_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0495::CUT_PBASE_T_USBDI_0495(TBool aHostRole)
:	CBaseBulkTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress),
	iRequestValidationResultPtr(NULL,0)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0495_CUT_PBASE_T_USBDI_0495_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0495_CUT_PBASE_T_USBDI_0495_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0495::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0495_CONSTRUCTL_ENTRY, this );
	BaseBulkConstructL();

	iInBuffer = HBufC8::NewL(KTestBufferLength);

	//Create buffer to contain sufficient repeats of the payload pattern
	//..so that we may grab cyclic chunks of said payload pattern for OUT transfers
	TInt repeats = KHostNumWriteBytes / (KLiteralEnglish5().Length()) + 1 + 1; //1 extra to accommodate start point plus 1 to accomodate remainder in division
	iOutBuffer = HBufC8::NewL(KLiteralEnglish5().Length() * repeats);
	iOutBufferPtr.Set(iOutBuffer->Des());
	iOutBufferPtr.Zero();
	for(TInt i=0;i<repeats;i++)
		{
		iOutBufferPtr.Append(KLiteralEnglish5());
		}

	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_CONSTRUCTL, "CUT_PBASE_T_USBDI_0495::ConstructL(): buffer created");
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0495_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0495::~CUT_PBASE_T_USBDI_0495()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0495_CUT_PBASE_T_USBDI_0495_ENTRY_DUP01, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0495_CUT_PBASE_T_USBDI_0495_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0495::KillTransfers()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0495_KILLTRANSFERS_ENTRY, this );
	
	iOutTransfer[0]->Cancel();
	iOutTransfer[1]->Cancel();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0495_KILLTRANSFERS_EXIT, this );
	}
	
	
	
void CUT_PBASE_T_USBDI_0495::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0495_EP0TRANSFERCOMPLETEL_ENTRY, this );
	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d", aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{
		if(iCaseStep == EFailed)
			{// ignore error, nad catch the TestFailed method called further down.
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_EP0TRANSFERCOMPLETEL_DUP01, "***Failure sending FAIL message to client on endpoint 0***");
			}
		else
			{
			TBuf<256> msg;
			KillTransfers();
			_LIT(lit, "<Error %d> Transfer to control endpoint 0 was not successful");
			msg.Format(lit,aCompletionCode);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_EP0TRANSFERCOMPLETEL_DUP02, msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(aCompletionCode,msg);
			iControlEp0->SendRequest(request,this);
			OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0495_EP0TRANSFERCOMPLETEL_EXIT, this );
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
		
		case ERequestRepeatedReadAndValidate:
			{
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_EP0TRANSFERCOMPLETEL_DUP03, "Try to perform ALL transfers");
	
			iCaseStep = ETransfer;	
			
			iTransferComplete |= PerformNextTransfer(KBulkTransferOutId[0]); //should not validate - just perform necessary transfers
			iTransferComplete |= PerformNextTransfer(KBulkTransferOutId[1]); //should not validate - just perform necessary transfers
			if((iTransferComplete & KBulkTransferIdMask) == KBulkTransferIdMask)
				{
				_LIT(lit, "TEST FAILURE: No data to send!!");
				TBuf<40> msg(lit);
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_EP0TRANSFERCOMPLETEL_DUP04, msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(KErrAbort,msg);
				return iControlEp0->SendRequest(request,this);
				}
			}
			break;
			
		case ERequestPrepareEndpointValidationResult:
			{
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_EP0TRANSFERCOMPLETEL_DUP05, "Asking client to prepare the result of its continuous validation");
			iCaseStep = ERequestValidationResult;
			iRequestValidationResultPtr.Set( iInBuffer->Des());
			iRequestValidationResultPtr.Zero(); //reset
			iRequestValidationResultPtr.SetLength(KPassFailStringLength);
			TInterfaceGetPayloadRequest request(1,iRequestValidationResultPtr);
			iControlEp0->SendRequest(request,this);
			}
			break;
	
		case ERequestValidationResult:
			{
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_EP0TRANSFERCOMPLETEL_DUP06, "Collect client's return validation  result in a pass or fail string ...");
            OstTraceData(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_EP0TRANSFERCOMPLETEL_DUP56, "", iInBuffer->Ptr(), iInBuffer->Length());
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_EP0TRANSFERCOMPLETEL_DUP07, "\n");
			TPtr8 ptr(iInBuffer->Des());
			if(ptr.Compare(KClientPassString) == 0)
				{
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_EP0TRANSFERCOMPLETEL_DUP08, "Client Validation Result is a PASS");
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_EP0TRANSFERCOMPLETEL_DUP09, "This is the FINAL check - the whole test has a PASSED");
				iCaseStep = EPassed;
				TTestCasePassed request;
				iControlEp0->SendRequest(request,this);
				}
			else
				{
				TBuf<256> msg;
				_LIT(lit, "<Error> Bulk data VALIDATION check was NOT successful");
				msg.Format(lit);
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_EP0TRANSFERCOMPLETEL_DUP10, msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(KErrCorrupt,msg);
				iControlEp0->SendRequest(request,this);
				}
			}
			break;
	
		default:
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_EP0TRANSFERCOMPLETEL_DUP11, "<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0495_EP0TRANSFERCOMPLETEL_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0495::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0495_TRANSFERCOMPLETEL_ENTRY, this );
	Cancel();
	
	TInt err(KErrNone);
	TBuf<256> msg;
	OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_TRANSFERCOMPLETEL, "Transfer completed (id=%d), aCompletionCode = %d, test step = %d",aTransferId, aCompletionCode, iCaseStep);


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

			if(aTransferId != KBulkTransferOutId[0] && aTransferId != KBulkTransferOutId[1])
				{
				iTransferComplete = 0; //reset
				err = KUnexpectedTransferID;
				_LIT(lit, "<Error %d> Unexpected transfer ID, wanted %d or %d, got %d");
				msg.Format(lit, err, KBulkTransferOutId[0], KBulkTransferOutId[1], aTransferId);
				break;
				}

			OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_TRANSFERCOMPLETEL_DUP01, "Transfer OUT %d completed - num bytes sent = %u", aTransferId, iNumWriteBytesRequested);
			
			iTransferComplete |= PerformNextTransfer(aTransferId);
			
			if(err==KErrNone && (iTransferComplete & KBulkTransferIdMask) == KBulkTransferIdMask)
				{
				/*
				Transfers all complete - now ask device to validate first interface's transfer OUT
				*/
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_TRANSFERCOMPLETEL_DUP02, "All Transfers Completed Successfully: Transfer Completion Aggregation Mask 0x%x", iTransferComplete);
				if(err==KErrNone)
					{
					OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_TRANSFERCOMPLETEL_DUP03, "Asking client to post validation recorded on the endpoint on its interface - ready for collection");
					iCaseStep = ERequestPrepareEndpointValidationResult;
					TRecordedValidationResultRequest request(1,1);
					iControlEp0->SendRequest(request,this);
					}
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
		_LIT(lit, "<Error %d> Client has posted an error discovered in validation");
		msg.Format(lit, err);
		}

	if(err!=KErrNone)
		{	
		KillTransfers();
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_TRANSFERCOMPLETEL_DUP04, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);
		}	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0495_TRANSFERCOMPLETEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0495::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0495_DEVICEINSERTEDL_ENTRY, this );
	
	Cancel();
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_DEVICEINSERTEDL, "this - %08x", this);
	
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
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_DEVICEINSERTEDL_DUP01, "Initialising the transfer descriptors - interface 1");
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
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_DEVICEINSERTEDL_DUP02, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		}
	else
		{
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_DEVICEINSERTEDL_DUP03, "Asking client for continuous 'Read' and 'Validate'");
		iCaseStep = ERequestRepeatedReadAndValidate;	
		TRepeatedReadAndValidateDataRequest request(1,1,KLiteralEnglish5,KDeviceNumReadBytes,KTotalBytesToTransfer);// EP2 means endpoint index 2 not the actual endpoint number, here the ep with 32 byte max packet size
		iControlEp0->SendRequest(request,this);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0495_DEVICEINSERTEDL_EXIT, this );
	}

TInt CUT_PBASE_T_USBDI_0495::PerformNextTransfer(TInt aTransferId)
/**
@param 	aTransferId - specifies trasnfer to use
@return	zero UNLESS no more trasnfers have been queued for the specified ID in which case return the transfer ID
Note:	This return value allows the caller to respond easily if no more transfers are required for the specified 
		transfer ID.
*/
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0495_PERFORMNEXTTRANSFER_ENTRY, this );
	
	if(iNumWriteBytesRequested >= KTotalBytesToTransfer)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_PERFORMNEXTTRANSFER, "****ALL DONE for Transfer using ID %d****", aTransferId);
		OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0495_PERFORMNEXTTRANSFER_DUP01, "Num bytes actually written = %u, num bytes required to be written = %u", iNumWriteBytesRequested, KTotalBytesToTransfer);
		OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0495_PERFORMNEXTTRANSFER_EXIT, this, aTransferId );
		return aTransferId; //Not writing any more - signal to user that no more transfers are required on this transfer ID
		}
	TUint bytesToWrite = KTotalBytesToTransfer - iNumWriteBytesRequested;
	TUint numWriteBytes = bytesToWrite < KHostNumWriteBytes ? bytesToWrite : KHostNumWriteBytes;

	_LITDBG("PerformNextTransfer: None existant transfer ID requested");
	__ASSERT_DEBUG(aTransferId==KBulkTransferOutId[0] || aTransferId==KBulkTransferOutId[1], User::Panic(lit, KErrArgument));
	CBulkTransfer& bulkTransfer = aTransferId==KBulkTransferOutId[0]?*iOutTransfer[0]:*iOutTransfer[1];
	bulkTransfer.TransferOut(iOutBufferPtr.Mid(iNumWriteBytesRequested%(KLiteralEnglish5().Length()), numWriteBytes), EFalse);
	iNumWriteBytesRequested += numWriteBytes;

	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0495_PERFORMNEXTTRANSFER_EXIT_DUP01, this );
	return 0; //Signal to the user that another transfer is queued on the specified transfer ID
	}


	} //end namespace
