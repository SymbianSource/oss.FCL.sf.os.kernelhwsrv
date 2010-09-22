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
// @file PBASE-T_USBDI-0498.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0498.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "testliterals.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0498Traces.h"
#endif


 

namespace NUnitTesting_USBDI
	{
// write/read size shall be a multiple of endpoint.MaxPacketSize(Hight speed == 512)			
const TInt KBulkMaxTransferSize = 80000;
const TUint KHostNumWriteBytes1 = 64000;
const TUint KHostNumWriteBytes2 = 64000;
const TInt KDeviceNumReadBytes = 128000;
const TInt KDeviceNumWriteBytes = 128000;
const TUint KHostNumReadBytes1 = 64000;
const TUint KHostNumReadBytes2 = 64001; //allow or ZLP


//Make these single bit values ... 
// ... so that their completion can be easily recorded in a bit mask!
const TUint32 KIfc1BulkTransferInId1  = 1<<0;
const TUint32 KIfc1BulkTransferInId2  = 1<<1;
const TUint32 KIfc2BulkTransferInId1  = 1<<2;
const TUint32 KIfc2BulkTransferInId2  = 1<<3;
const TUint32 KIfc1BulkTransferInIdMask = KIfc1BulkTransferInId1 | KIfc1BulkTransferInId2; 
const TUint32 KIfc2BulkTransferInIdMask = KIfc2BulkTransferInId1 | KIfc2BulkTransferInId2;
const TUint32 KBulkTransferInIdMask = KIfc1BulkTransferInIdMask | KIfc2BulkTransferInIdMask;
const TUint32 KIfc1BulkTransferOutId1 = 1<<4;
const TUint32 KIfc1BulkTransferOutId2 = 1<<5;
const TUint32 KIfc2BulkTransferOutId1 = 1<<6;
const TUint32 KIfc2BulkTransferOutId2 = 1<<7;
const TUint32 KIfc1BulkTransferOutIdMask = KIfc1BulkTransferOutId1 | KIfc1BulkTransferOutId2; 
const TUint32 KIfc2BulkTransferOutIdMask = KIfc2BulkTransferOutId1 | KIfc2BulkTransferOutId2;
const TUint32 KBulkTransferOutIdMask = KIfc1BulkTransferOutIdMask | KIfc2BulkTransferOutIdMask;
const TUint32 KBulkTransferIdMask = KBulkTransferInIdMask | KBulkTransferOutIdMask;

const TInt KUnexpectedTransferID = -101;
const TInt KUndefinedStep	 = -102;

const TInt KMaxTimeDiffPercentage = 25;



_LIT(KTestCaseId,"PBASE-T_USBDI-0498");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0498,TBool> CUT_PBASE_T_USBDI_0498::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0498* CUT_PBASE_T_USBDI_0498::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0498_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0498* self = new (ELeave) CUT_PBASE_T_USBDI_0498(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0498_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0498::CUT_PBASE_T_USBDI_0498(TBool aHostRole)
:	CBaseBulkTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress),
	iRequestDeviceValidationResultPtr(NULL,0)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0498_CUT_PBASE_T_USBDI_0498_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0498_CUT_PBASE_T_USBDI_0498_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0498::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0498_CONSTRUCTL_ENTRY, this );
	BaseBulkConstructL();

	iInBuffer = HBufC8::NewL(KTestBufferLength);
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_CONSTRUCTL, "CUT_PBASE_T_USBDI_0491::ConstructL(): buffer created");
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0498_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0498::~CUT_PBASE_T_USBDI_0498()
/**
The transfer objects have test specific names - so perform transfer specific clean up here.
*/
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0498_CUT_PBASE_T_USBDI_0498_ENTRY_DUP01, this );
	
	Cancel();

	//Do this before deleting the transfer objects
	CloseInterfaceAndPipes();
	
	TUint8 count;
	for(count=0;count<KNumOutTransfersPerInterface;count++)
		{
		delete iIfc1OutTransfer[count];
		iIfc1OutTransfer[count] = NULL;
		delete iIfc2OutTransfer[count];
		iIfc2OutTransfer[count] = NULL;
		}
	for(count=0;count<KNumInTransfersPerInterface;count++)
		{
		delete iIfc1InTransfer[count];
		iIfc1InTransfer[count] = NULL;
		delete iIfc2InTransfer[count];
		iIfc2InTransfer[count] = NULL;
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0498_CUT_PBASE_T_USBDI_0498_EXIT_DUP01, this );
	}
	
	
void CUT_PBASE_T_USBDI_0498::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_ENTRY, this );
	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{
		if(iCaseStep == EFailed)
			{// ignore error, nad catch the TestFailed method called further down.
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP01, "***Failure sending FAIL message to client on endpoint 0***");
			}
		else
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP02, msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(aCompletionCode,msg);
			iControlEp0->SendRequest(request,this);
			OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_EXIT, this );
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
		
		case ERequestDeviceIFC1Read:
			{
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP03, "Asking client for 'Read' on interface 2");
			iCaseStep = ERequestDeviceIFC2Read;	
			TEndpointReadRequest request(2,1,KDeviceNumReadBytes);// EP1 means endpoint index 1 not the actual endpoint number
			iControlEp0->SendRequest(request,this);
			}
			break;
			
		case ERequestDeviceIFC2Read:
			{
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP04, "Asking client to 'Write' on interface 1");
			iCaseStep = ERequestDeviceIFC1Write;	
			TEndpointPatternWriteRequest request(1,1,KLiteralFrench4(),KDeviceNumWriteBytes);
			iControlEp0->SendRequest(request,this);
			}
			break;
			
		case ERequestDeviceIFC1Write:
			{
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP05, "Asking client to 'Write' on interface 2");
			iCaseStep = ERequestDeviceIFC2Write;	
			TEndpointPatternWriteRequest request(2,1,KLiteralEnglish2(),KDeviceNumWriteBytes);
			iControlEp0->SendRequest(request,this);
			}
			break;
			
		case ERequestDeviceIFC2Write:
			{
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP06, "Try to perform ALL transfers on BOTH interfaces");
	
			iCaseStep = ETransfer;	
	
			iStartTime[0].HomeTime();
			iStartTime[1].HomeTime();
			iStartTime[2].HomeTime();
			iStartTime[3].HomeTime();
	
			iIfc1OutTransfer[0]->TransferOut(KLiteralFrench4(), KHostNumWriteBytes1, EFalse);
			iIfc1OutTransfer[1]->TransferOut(KLiteralFrench4(), KHostNumWriteBytes1, KHostNumWriteBytes2, EFalse);
			iIfc2OutTransfer[0]->TransferOut(KLiteralEnglish2(), KHostNumWriteBytes1, EFalse);
			iIfc2OutTransfer[1]->TransferOut(KLiteralEnglish2(), KHostNumWriteBytes1, KHostNumWriteBytes2, EFalse);
	
			iIfc1InTransfer[0]->TransferIn(KHostNumReadBytes1);
			iIfc1InTransfer[1]->TransferIn(KHostNumReadBytes2);
			iIfc2InTransfer[0]->TransferIn(KHostNumReadBytes1);
			iIfc2InTransfer[1]->TransferIn(KHostNumReadBytes2);
			}
			break;
			
		case ERequestDeviceValidateIFC1:
			{
			iCaseStep = ERequestDeviceValidationResultIFC1;
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP07, "Interface 1: Request Client Validation Result");
			iRequestDeviceValidationResultPtr.Set(iInBuffer->Des());
			iRequestDeviceValidationResultPtr.Zero(); //reset
			iRequestDeviceValidationResultPtr.SetLength(KPassFailStringLength);
			TInterfaceGetPayloadRequest request(1,iRequestDeviceValidationResultPtr);
			iControlEp0->SendRequest(request,this);
			}
			break;
		
		case ERequestDeviceValidationResultIFC1:
			{
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP08, "Interface 1: Client Validation Result string ...");
            OstTraceData(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP58, "", iInBuffer->Ptr(), iInBuffer->Length());
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP09, "\n");
			TPtr8 ptr(iInBuffer->Des());
			if(ptr.Compare(KClientPassString) == 0)
				{
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP10, "Client Validation Interface 1 Result is a PASS");
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP11, "Now performing Client Validation for Interface 2");
				iCaseStep = ERequestDeviceValidateIFC2;
				TEndpointStringValidationRequest request(2,1,KLiteralEnglish2(),KDeviceNumReadBytes);
				iControlEp0->SendRequest(request,this);
				}
			else
				{
				TBuf<256> msg;
				msg.Format(_L("<Error> Bulk data VALIDATION check was NOT successful"));
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP12, msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(KErrCorrupt,msg);
				iControlEp0->SendRequest(request,this);
				}
			}
			break;
			
		case ERequestDeviceValidateIFC2:
			{
			iCaseStep = ERequestDeviceValidationResultIFC2;
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP13, "Interface 2: Request Client Validation Result");
			iRequestDeviceValidationResultPtr.Set(iInBuffer->Des());
			iRequestDeviceValidationResultPtr.Zero(); //reset
			iRequestDeviceValidationResultPtr.SetLength(KPassFailStringLength);
			TInterfaceGetPayloadRequest request(2,iRequestDeviceValidationResultPtr);
			iControlEp0->SendRequest(request,this);
			}
			break;
		
		case ERequestDeviceValidationResultIFC2:
			{
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP14, "Interface 1: Client Validation Result string ...");
            OstTraceData(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP54, "", iInBuffer->Ptr(), iInBuffer->Length());
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP15, "\n");
			TPtr8 ptr(iInBuffer->Des());
			if(ptr.Compare(KClientPassString) == 0)
				{
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP16, "Client Validation Interface 2 Result is a PASS");
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP17, "This is the FINAL check - the whole test has a PASSED");
				iCaseStep = EPassed;
				TTestCasePassed request;
				iControlEp0->SendRequest(request,this);
				}
			else
				{
				TBuf<256> msg;
				msg.Format(_L("<Error> Bulk data VALIDATION check was NOT successful"));
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP18, msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(KErrCorrupt,msg);
				iControlEp0->SendRequest(request,this);
				}
			}
			break;
	
		default:
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_DUP19, "<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0498_EP0TRANSFERCOMPLETEL_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0498::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0498_TRANSFERCOMPLETEL_ENTRY, this );
	Cancel();
	
	TInt err(KErrNone);
	TBuf<256> msg;
	OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_TRANSFERCOMPLETEL, "Transfer completed (id=%d), aCompletionCode = %d, test step = %d",aTransferId, aCompletionCode, iCaseStep);


	switch(iCaseStep)
		{
		case ETransfer:
			if(aCompletionCode != KErrNone)
				{
				iIfc1OutTransfer[0]->Cancel();
				iIfc1OutTransfer[1]->Cancel();
				iIfc2OutTransfer[0]->Cancel();
				iIfc2OutTransfer[1]->Cancel();
				iIfc1InTransfer[0]->Cancel();
				iIfc1InTransfer[1]->Cancel();
				iIfc2InTransfer[0]->Cancel();
				iIfc2InTransfer[1]->Cancel();
				err = KErrCorrupt;
				msg.Format(_L("<Error %d> The transfer completed with an error."), aCompletionCode);
				break; //switch(iCaseStep)
				}
	
			switch(aTransferId)
				{
				case KIfc1BulkTransferOutId1:
				case KIfc1BulkTransferOutId2:
				case KIfc2BulkTransferOutId1:
				case KIfc2BulkTransferOutId2:
					iTransferComplete |= aTransferId;
					OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_TRANSFERCOMPLETEL_DUP01, "Transfer OUT %d completed (Transfer Completion Aggregation Mask 0x%x)", aTransferId, iTransferComplete);
					break; //switch(aTransferId)

				case KIfc1BulkTransferInId1:
				case KIfc1BulkTransferInId2:
				case KIfc2BulkTransferInId1:
				case KIfc2BulkTransferInId2:
					iTransferComplete |= aTransferId;
					OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_TRANSFERCOMPLETEL_DUP02, "Transfer OUT %d completed (Transfer Completion Aggregation Mask 0x%x)", aTransferId, iTransferComplete);
					break; //switch(aTransferId)

				default:
					iTransferComplete = 0; //reset
					err = KUnexpectedTransferID;
					msg.Format(_L("<Error %d> Unexpected transfer ID, GOT %d, (wanted one of:- %d, %d, %d, %d, %d, %d, %d, or%d)"),
							       err, aTransferId,
							       KIfc1BulkTransferInId1, 
							       KIfc1BulkTransferInId2, 
							       KIfc2BulkTransferInId1, 
							       KIfc2BulkTransferInId2, 
							       KIfc1BulkTransferOutId1, 
							       KIfc1BulkTransferOutId2, 
							       KIfc2BulkTransferOutId1, 
							       KIfc2BulkTransferOutId2
							       );
					break; //switch(aTransferId)
				}
			
			// Transfer Out Response
			if(err==KErrNone && iTimeElapsed[0] == 0 && (iTransferComplete & KIfc1BulkTransferOutIdMask) == KIfc1BulkTransferOutIdMask) 
				//Record time elapsed for Interface 1 if not yet recorded.
				{
				RecordTime(0);
				}
			if(err==KErrNone && iTimeElapsed[1] == 0 && (iTransferComplete & KIfc2BulkTransferOutIdMask) == KIfc2BulkTransferOutIdMask)
				//Record time elapsed for Interface 2 if not yet recorded.
				{
				RecordTime(1);
				}
			if(err==KErrNone && (iTransferComplete & KBulkTransferOutIdMask) == KBulkTransferOutIdMask)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_TRANSFERCOMPLETEL_DUP03, "All OUT Transfers Completed Successfully: Transfer Completion Aggregation Mask 0x%x", iTransferComplete);
				//Leave validation to the point at which all transfers have completed.
				}
	
			// Transfer In Response
			if(err==KErrNone && iTimeElapsed[2] == 0 && (iTransferComplete & KIfc1BulkTransferInIdMask) == KIfc1BulkTransferInIdMask)
				//Record time elapsed for Interface 1 if not yet recorded.
				{
				RecordTime(2);
				}
			if(err==KErrNone && iTimeElapsed[3] == 0 && (iTransferComplete & KIfc2BulkTransferInIdMask) == KIfc2BulkTransferInIdMask) 
				//Record time elapsed for Interface 2 if not yet recorded.
				{
				RecordTime(3);
				}
			if(err==KErrNone && (iTransferComplete & KBulkTransferInIdMask) == KBulkTransferInIdMask)
				{
				// ok, compare data rcvd now
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_TRANSFERCOMPLETEL_DUP04, "All IN Transfers Completed Successfully: Transfer Completion Aggregation Mask 0x%x", iTransferComplete);
	
				TPtrC8 data1(iIfc1InTransfer[0]->DataPolled());	
				TPtrC8 data2(iIfc1InTransfer[1]->DataPolled());		
				TPtrC8 data3(iIfc2InTransfer[0]->DataPolled());		
				TPtrC8 data4(iIfc2InTransfer[1]->DataPolled());		
				//Validate first transfer on Interface 1 for number of bytes originally written.
				if(ValidateData(data1, KLiteralFrench4(), KHostNumWriteBytes1) == EFalse)
					{
					OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_TRANSFERCOMPLETEL_DUP05, "Validation failure 1st transfer, Interface 1");
					err = KErrCompletion; //indicates data validation failure
					break; //switch(iCaseStep)
					}

				//Validate second transfer on Interface 1 for number of bytes originally written.
				if(ValidateData(data2, KLiteralFrench4(), KHostNumWriteBytes1, KHostNumWriteBytes2) == EFalse)
					{
					OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_TRANSFERCOMPLETEL_DUP06, "Validation failure 2nd transfer, Interface 1");
					err = KErrCompletion; //indicates data validation failure
					break; //switch(iCaseStep)
					}
				
				//Validate first transfer on Interface 2 for number of bytes originally written.
				if(ValidateData(data3, KLiteralEnglish2(), KHostNumWriteBytes1) == EFalse)
					{
					OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_TRANSFERCOMPLETEL_DUP07, "Validation failure 1st transfer, Interface 2");
					err = KErrCompletion; //indicates data validation failure
					break; //switch(iCaseStep)
					}
				
				//Validate second transfer on Interface 2 for number of bytes originally written.
				if(ValidateData(data4, KLiteralEnglish2(), KHostNumWriteBytes1, KHostNumWriteBytes2) == EFalse)
					{
					OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_TRANSFERCOMPLETEL_DUP08, "Validation failure 2nd transfer, Interface 2");
					err = KErrCompletion; //indicates data validation failure
					break; //switch(iCaseStep)
					}
				
				// Comparison is a match
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_TRANSFERCOMPLETEL_DUP09, "Comparison for IN transfer is a match");
				}
			break; //switch(iCaseStep)

		default:
			err = KUndefinedStep;
			msg.Format(_L("<Error %d> Undefined case step %d reached"),KUndefinedStep, iCaseStep);
			break; //switch(iCaseStep)
		}

	if(err == KErrNone && iTransferComplete == KBulkTransferIdMask)
	/*
	Transfers all complete - now ask device to validate first interface's transfer OUT
	*/
		{
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_TRANSFERCOMPLETEL_DUP10, "Checking all times against each other");
		err = CheckTimes(0, 1, KMaxTimeDiffPercentage);
		err = err?err:CheckTimes(0, 2, KMaxTimeDiffPercentage);
		err = err?err:CheckTimes(0, 3, KMaxTimeDiffPercentage);
		err = err?err:CheckTimes(1, 2, KMaxTimeDiffPercentage);
		err = err?err:CheckTimes(1, 3, KMaxTimeDiffPercentage);
		err = err?err:CheckTimes(2, 3, KMaxTimeDiffPercentage);
		ResetTimes(0);
		ResetTimes(1);
		ResetTimes(2);
		ResetTimes(3);
		
		if(err==KErrNone)
			{
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_TRANSFERCOMPLETEL_DUP11, "Asking client for 'Validate' data written on interface 1");
			iCaseStep = ERequestDeviceValidateIFC1;
			TEndpointStringValidationRequest request(1,1,KLiteralFrench4(),KDeviceNumReadBytes);
			iControlEp0->SendRequest(request,this);
			}
		}

	if(err == KErrCompletion)
		//indicates data validation failure
		{
		msg.Format(_L("<Error %d> Bulk transfer IN data received does not match Bulk Transfer OUT data"), err);
		}

	if(err == KErrTooBig)
		//indicates timing validation failure
		{
		msg.Format(_L("<Error %d> Timer comparison showed too great a difference in transfer times between the two interfaces"), err);
		}
	
	if(err!=KErrNone)
		{	
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_TRANSFERCOMPLETEL_DUP12, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);
		}	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0498_TRANSFERCOMPLETEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0498::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0498_DEVICEINSERTEDL_ENTRY, this );
	
	Cancel();
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_DEVICEINSERTEDL, "this - %08x", this);
	
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
		if(SetUpInterfaceAndPipesL(aDeviceHandle, 2) == EDeviceConfigurationError)
			// Prepare for response from control transfer to client
			{
			err = KErrGeneral;
			msg.Format(_L("Base class SetUpInterfaceAndPipes for Interface 2 failed"));
			}
		else
			{
	
			iIfc1InTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkMaxTransferSize,*this,KIfc1BulkTransferInId1);
			iIfc1InTransfer[1] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkMaxTransferSize,*this,KIfc1BulkTransferInId2);
			iIfc1OutTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkMaxTransferSize,*this,KIfc1BulkTransferOutId1);
			iIfc1OutTransfer[1] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkMaxTransferSize,*this,KIfc1BulkTransferOutId2);

			iIfc2InTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface2BulkIn,iUsbInterface2,KBulkMaxTransferSize,*this,KIfc2BulkTransferInId1);
			iIfc2InTransfer[1] = new (ELeave) CBulkTransfer(iTestPipeInterface2BulkIn,iUsbInterface2,KBulkMaxTransferSize,*this,KIfc2BulkTransferInId2);
			iIfc2OutTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface2BulkOut1,iUsbInterface2,KBulkMaxTransferSize,*this,KIfc2BulkTransferOutId1);
			iIfc2OutTransfer[1] = new (ELeave) CBulkTransfer(iTestPipeInterface2BulkOut1,iUsbInterface2,KBulkMaxTransferSize,*this,KIfc2BulkTransferOutId2);
			
			// Initialise the descriptors for transfer		
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_DEVICEINSERTEDL_DUP01, "Initialising the transfer descriptors - interface 1");
			err = iUsbInterface1.InitialiseTransferDescriptors();
			if(err != KErrNone)
				{
				msg.Format(_L("<Error %d> Unable to initialise transfer descriptors (Interface 1)"),err);
				}
			else
				{
				// Initialise the descriptors for transfer		
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_DEVICEINSERTEDL_DUP02, "Initialising the transfer descriptors (Interface 2)");
				err = iUsbInterface2.InitialiseTransferDescriptors();
				if(err != KErrNone)
					{
					msg.Format(_L("<Error %d> Unable to initialise transfer descriptors"),err);
					}
				}
			}
		}
	if(err != KErrNone)
		{
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_DEVICEINSERTEDL_DUP03, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		}
	else
		{
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0498_DEVICEINSERTEDL_DUP04, "Asking client for 'Read' on interface 1");
		iCaseStep = ERequestDeviceIFC1Read;	
		TEndpointReadRequest request(1,1,KDeviceNumReadBytes);// EP1 means endpoint index 1 not the actual endpoint number
		iControlEp0->SendRequest(request,this);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0498_DEVICEINSERTEDL_EXIT, this );
	}
	
	} //end namespace
