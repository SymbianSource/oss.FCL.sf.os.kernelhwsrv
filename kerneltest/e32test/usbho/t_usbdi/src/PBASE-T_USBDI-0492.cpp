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
// @file PBASE-T_USBDI-0492.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0492.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "testliterals.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0492Traces.h"
#endif


 

namespace NUnitTesting_USBDI
	{
const TInt KBulkTransferSize = 583;
const TUint KHostNumWriteBytes1 = 210;
const TUint KHostNumWriteBytes2 = 350;
const TUint KHostNumWriteBytes3 = KBulkTransferSize - KHostNumWriteBytes1 - KHostNumWriteBytes2; // 23
const TUint KHostNumReadBytes1 = 301;
const TUint KHostNumReadBytes2 = 21;
const TUint KHostNumReadBytes3 = KBulkTransferSize - KHostNumReadBytes1 - KHostNumReadBytes2; // 261


//Make these single bit values ... 
// ... so that their completion can be easily recorded in a bit mask!
const TInt KBulkTransferInId0 = 1<<0;
const TInt KBulkTransferInId1 = 1<<1;
const TInt KBulkTransferInId2 = 1<<2;
const TInt KBulkTransferOutId0 = 1<<3;
const TInt KBulkTransferOutId1 = 1<<4;
const TInt KBulkTransferOutId2 = 1<<5;

const TInt KUnexpectedTransferID = -101;
const TInt KUndefinedStep	 = -102;


_LIT(KTestCaseId,"PBASE-T_USBDI-0492");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0492,TBool> CUT_PBASE_T_USBDI_0492::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0492* CUT_PBASE_T_USBDI_0492::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0492_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0492* self = new (ELeave) CUT_PBASE_T_USBDI_0492(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0492_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0492::CUT_PBASE_T_USBDI_0492(TBool aHostRole)
:	CBaseBulkTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0492_CUT_PBASE_T_USBDI_0492_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0492_CUT_PBASE_T_USBDI_0492_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0492::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0492_CONSTRUCTL_ENTRY, this );
	BaseBulkConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0492_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0492::~CUT_PBASE_T_USBDI_0492()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0492_CUT_PBASE_T_USBDI_0492_ENTRY_DUP01, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0492_CUT_PBASE_T_USBDI_0492_EXIT_DUP01, this );
	}
	
	
void CUT_PBASE_T_USBDI_0492::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0492_EP0TRANSFERCOMPLETEL_ENTRY, this );
	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0492_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{
		if(iCaseStep == EFailed)
			{// ignore error, nad catch the TestFailed method called further down.
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0492_EP0TRANSFERCOMPLETEL_DUP01, "***Failure sending FAIL message to client on endpoint 0***");
			}
		else
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0492_EP0TRANSFERCOMPLETEL_DUP02, msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(aCompletionCode,msg);
			iControlEp0->SendRequest(request,this);
			OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0492_EP0TRANSFERCOMPLETEL_EXIT, this );
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
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0492_EP0TRANSFERCOMPLETEL_DUP03, "Try to send data");
			iOutTransfer[0]->TransferOut(KLiteralEnglish8().Mid(0, KHostNumWriteBytes1), EFalse);
			iOutTransfer[1]->TransferOut(KLiteralEnglish8().Mid(KHostNumWriteBytes1, KHostNumWriteBytes2), EFalse);
			iOutTransfer[2]->TransferOut(KLiteralEnglish8().Mid(KHostNumWriteBytes1+KHostNumWriteBytes2, KHostNumWriteBytes3), ETrue); //do not suppress ZLP on this last one  (though should be irrelevant here)    
			break;
		
		case ETransferIn:
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0492_EP0TRANSFERCOMPLETEL_DUP04, "Try to receive data");
			iInTransfer[0]->TransferIn(KHostNumReadBytes1);
			iInTransfer[1]->TransferIn(KHostNumReadBytes2);
			iInTransfer[2]->TransferIn(KHostNumReadBytes3);
			break;
	
		default:
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0492_EP0TRANSFERCOMPLETEL_DUP05, "<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0492_EP0TRANSFERCOMPLETEL_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0492::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0492_TRANSFERCOMPLETEL_ENTRY, this );
	Cancel();
	
	TInt err(KErrNone);
	TBuf<256> msg;
	OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0492_TRANSFERCOMPLETEL, "Transfer completed (id=%d), aCompletionCode = %d",aTransferId, aCompletionCode);

	switch(iCaseStep)
		{
		case ETransferOut:
			if(aCompletionCode != KErrNone)
				{
				err = KErrCorrupt;
				msg.Format(_L("<Error %d> No data sent on bulk OUT request"),aCompletionCode);
				break; // switch(iCaseStep)
				}
	
			switch(aTransferId)
				{
				case KBulkTransferOutId0:
				case KBulkTransferOutId1:
				case KBulkTransferOutId2:
					iTransferComplete |= aTransferId;
					OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0492_TRANSFERCOMPLETEL_DUP01, "Transfer %d completed", aTransferId);
					break; // switch(aTransferId)

				default:
					iTransferComplete = 0; //reset
					err = KUnexpectedTransferID;
					msg.Format(_L("<Error %d> Unexpected transfer ID, wanted %d, %d or %d, got %d"),
							       err, KBulkTransferOutId0, KBulkTransferOutId1, KBulkTransferOutId2, aTransferId);
					break; // switch(aTransferId)
				}

			if(err==KErrNone && iTransferComplete == (KBulkTransferOutId0 | KBulkTransferOutId1 | KBulkTransferOutId2))
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0492_TRANSFERCOMPLETEL_DUP02, "Try to receive back sent data. Transfers Completed %d", iTransferComplete);
				iCaseStep = ETransferIn;
				TUint numBytes[KNumSplitWriteSections] = {KHostNumReadBytes1, KHostNumReadBytes2, KHostNumReadBytes3};
				TSplitWriteCachedReadDataRequest request(1,1,1,numBytes);
				iControlEp0->SendRequest(request,this);	
				iTransferComplete = 0; //reset
				}
			break; // switch(iCaseStep)

		case ETransferIn:
			if(aCompletionCode != KErrNone)
				{
				err = KErrCorrupt;
				msg.Format(_L("<Error %d> No data sent on bulk IN request"),aCompletionCode);
				break; // switch(iCaseStep)
				}
	
			switch(aTransferId)
				{
				case KBulkTransferInId0:
				case KBulkTransferInId1:
				case KBulkTransferInId2:
					iTransferComplete |= aTransferId;
					break; // switch(aTransferId)

				default:
					iTransferComplete = 0; //reset
					err = KUnexpectedTransferID;
					msg.Format(_L("<Error %d> Unexpected transfer ID, wanted %d, %d or %d, got %d"),
						       err, KBulkTransferInId0, KBulkTransferInId1, KBulkTransferInId2, aTransferId);
					break; // switch(aTransferId)
				}
				
			if(err==KErrNone && iTransferComplete == (KBulkTransferInId0 | KBulkTransferInId1 | KBulkTransferInId2))
				{
				// compare data rcvd now
				TPtrC8 data1(iInTransfer[0]->DataPolled());		
				TPtrC8 data2(iInTransfer[1]->DataPolled());		
				TPtrC8 data3(iInTransfer[2]->DataPolled());		
				if(ValidateData(data1, KLiteralEnglish8().Mid(0, KHostNumReadBytes1)) == EFalse)
					{
					err = KErrCompletion; //indicates data validation failure
					break; // switch(iCaseStep)
					}

				if(ValidateData(data2, KLiteralEnglish8().Mid(KHostNumReadBytes1, KHostNumReadBytes2)) == EFalse)
					{
					err = KErrCompletion; //indicates data validation failure
					break; // switch(iCaseStep)
					}

				if(ValidateData(data3, KLiteralEnglish8().Mid(KHostNumReadBytes1+KHostNumReadBytes2, KHostNumReadBytes3)) == EFalse)
					{
					err = KErrCompletion; //indicates data validation failure
					break; // switch(iCaseStep)
					}

				// Comparison is a match
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0492_TRANSFERCOMPLETEL_DUP03, "Comparison for IN transfer is a match");
				iCaseStep = EPassed;
				TTestCasePassed request;
				iControlEp0->SendRequest(request,this);
				iTransferComplete = 0; //reset
				}
			break; // switch(iCaseStep)
			
		default:
			err = KUndefinedStep;
			msg.Format(_L("<Error %d> Undefined case step %d reached"),KUndefinedStep, iCaseStep);
			break; // switch(iCaseStep)
		}
	
	if(err == KErrCompletion)
		{
		//indicates data validation failure
		msg.Format(_L("<Error %d> Bulk transfer IN data received does not match Bulk Transfer OUT data"), err);
		}
	
	if(err!=KErrNone)
		{	
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0492_TRANSFERCOMPLETEL_DUP04, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		}	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0492_TRANSFERCOMPLETEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0492::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0492_DEVICEINSERTEDL_ENTRY, this );
	
	Cancel();
	
	if(BaseBulkDeviceInsertedL(aDeviceHandle) == EDeviceConfigurationError)
		// Prepare for response from control transfer to client
		{
		iCaseStep = EFailed;
		}
	
	// Create the bulk transfers	
	iInTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkTransferSize,*this,KBulkTransferInId0);
	iInTransfer[1] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkTransferSize,*this,KBulkTransferInId1);
	iInTransfer[2] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkTransferSize,*this,KBulkTransferInId2);
	iOutTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkTransferSize,*this,KBulkTransferOutId0);
	iOutTransfer[1] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkTransferSize,*this,KBulkTransferOutId1);
	iOutTransfer[2] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkTransferSize,*this,KBulkTransferOutId2);
	
	// Initialise the descriptors for transfer		
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0492_DEVICEINSERTEDL, "Initialising the transfer descriptors");
	TInt err = iUsbInterface1.InitialiseTransferDescriptors();
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to initialise transfer descriptors"),err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0492_DEVICEINSERTEDL_DUP01, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0492_DEVICEINSERTEDL_EXIT, this );
		return;
		}

	iCaseStep = ETransferOut;	
	TEndpointReadRequest request(1,1,KBulkTransferSize);// EP1 means endpoint index 1 not the actual endpoint number
	iControlEp0->SendRequest(request,this);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0492_DEVICEINSERTEDL_EXIT_DUP01, this );
	}
	
	} //end namespace
