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
// @file PBASE-T_USBDI-0493.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0493.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "testliterals.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0493Traces.h"
#endif


 

namespace NUnitTesting_USBDI
	{
const TInt  KLiteralEnglish16Length = KLiteralEnglish16().Length(); 
// High speed peripheral vasco max packet size is 512. 
// Size for reading/writing shall be multiple of endpoint max packet size to prevent the return of RDevUsbcClient::ReadUntilShort.
const TUint KBulkTransferMaxSize = 640;
const TUint KHostNumWriteBytes = 512;
const TUint KHostNumReadBytes = 512;
const TUint KHostFinalNumReadBytes = 600;

const TUint KStartNumTransferBytes = 2047;
const TUint KFinishNumTransferBytes = 2049;

const TUint KClientNumReadBytes = 4000;

//Make these single bit values ... 
// ... so that their completion can be easily recorded in a bit mask!
const TInt KBulkTransferInId0 = 1<<0;
const TInt KBulkTransferInId1 = 1<<1;
const TInt KBulkTransferInId2 = 1<<2;
const TInt KBulkTransferInId3 = 1<<3;
const TInt KBulkTransferOutId0 = 1<<4;
const TInt KBulkTransferOutId1 = 1<<5;
const TInt KBulkTransferOutId2 = 1<<6;
const TInt KBulkTransferOutId3 = 1<<7;

const TInt KUnexpectedTransferID = -101;
const TInt KUndefinedStep	 = -102;


_LIT(KTestCaseId,"PBASE-T_USBDI-0493");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0493,TBool> CUT_PBASE_T_USBDI_0493::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0493* CUT_PBASE_T_USBDI_0493::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0493_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0493* self = new (ELeave) CUT_PBASE_T_USBDI_0493(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0493_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0493::CUT_PBASE_T_USBDI_0493(TBool aHostRole)
:	CBaseBulkTestCase(KTestCaseId,aHostRole),
	iNumTransferBytes(KStartNumTransferBytes),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0493_CUT_PBASE_T_USBDI_0493_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0493_CUT_PBASE_T_USBDI_0493_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0493::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0493_CONSTRUCTL_ENTRY, this );
	BaseBulkConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0493_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0493::~CUT_PBASE_T_USBDI_0493()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0493_CUT_PBASE_T_USBDI_0493_ENTRY_DUP01, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0493_CUT_PBASE_T_USBDI_0493_EXIT_DUP01, this );
	}

void CUT_PBASE_T_USBDI_0493::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0493_EP0TRANSFERCOMPLETEL_ENTRY, this );
	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0493_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{
		if(iCaseStep == EFailed)
			{// todo, cope with errors
			}
		else
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0493_EP0TRANSFERCOMPLETEL_DUP01, msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(aCompletionCode,msg);
			iControlEp0->SendRequest(request,this);
			OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0493_EP0TRANSFERCOMPLETEL_EXIT, this );
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
			OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0493_EP0TRANSFERCOMPLETEL_DUP02, "Try to send %d bytes of data", iNumTransferBytes);
			{
			    const TPtrC8& midKLiteralEnglish = KLiteralEnglish16().Mid(0*KHostNumWriteBytes, KHostNumWriteBytes);
                OstTraceData(TRACE_NORMAL, CUT_PBASE_T_USBDI_0493_EP0TRANSFERCOMPLETEL_DUP52, "", midKLiteralEnglish.Ptr(), midKLiteralEnglish.Length());
			}
			iOutTransfer[0]->TransferOut(KLiteralEnglish16().Mid(0*KHostNumWriteBytes, KHostNumWriteBytes), EFalse);
			iOutTransfer[1]->TransferOut(KLiteralEnglish16().Mid(1*KHostNumWriteBytes, KHostNumWriteBytes), EFalse);
			iOutTransfer[2]->TransferOut(KLiteralEnglish16().Mid(2*KHostNumWriteBytes, KHostNumWriteBytes), EFalse);
			iOutTransfer[3]->TransferOut(KLiteralEnglish16().Mid(3*KHostNumWriteBytes, iNumTransferBytes - 3*KHostNumWriteBytes), ETrue);
			break;
		
		case ETransferIn:
			OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0493_EP0TRANSFERCOMPLETEL_DUP03, "Try to receive max %d bytes of data", 3*KHostNumReadBytes + KHostFinalNumReadBytes);
			iInTransfer[0]->TransferIn(KHostNumReadBytes);
			iInTransfer[1]->TransferIn(KHostNumReadBytes);
			iInTransfer[2]->TransferIn(KHostNumReadBytes);
			iInTransfer[3]->TransferIn(KHostFinalNumReadBytes);
			break;
	
		default:
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0493_EP0TRANSFERCOMPLETEL_DUP04, "<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0493_EP0TRANSFERCOMPLETEL_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0493::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0493_TRANSFERCOMPLETEL_ENTRY, this );
	Cancel();
	
	TInt err(KErrNone);
	TBuf<256> msg;
	OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0493_TRANSFERCOMPLETEL, "Transfer completed (id=%d), aCompletionCode = %d",aTransferId, aCompletionCode);

	switch(iCaseStep)
		{
		case ETransferOut:
			if(aCompletionCode != KErrNone)
				{
				err = KErrCorrupt;
				msg.Format(_L("<Error %d> No data sent on bulk OUT request"),aCompletionCode);
				break; //switch(iCaseStep)
				}

			switch(aTransferId)
				{
				case KBulkTransferOutId0:
				case KBulkTransferOutId1:
				case KBulkTransferOutId2:		
				case KBulkTransferOutId3:
					iTransferComplete |= aTransferId;
					OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0493_TRANSFERCOMPLETEL_DUP01, "Transfer %d completed", aTransferId);
					break; //switch(aTransferId)
					
				default:
					iTransferComplete = 0; //reset
					err = KUnexpectedTransferID;
					msg.Format(_L("<Error %d> Unexpected transfer ID, wanted %d or %d or %d or %d, got %d"),
							       err, KBulkTransferOutId0, KBulkTransferOutId1, KBulkTransferOutId2, KBulkTransferOutId3, aTransferId);					
					break; //switch(aTransferId)
				}

			if(err==KErrNone && iTransferComplete == (KBulkTransferOutId0 | KBulkTransferOutId1 | KBulkTransferOutId2 | KBulkTransferOutId3))
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0493_TRANSFERCOMPLETEL_DUP02, "Try to receive back sent data. Transfers Completed %d", iTransferComplete);
				iCaseStep = ETransferIn;
				TWriteSynchronousCachedReadDataRequest request(1,1,1);  //Use first read EP and first write EP (on interface 1)
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
			switch(aTransferId)
				{
				case KBulkTransferInId0:
				case KBulkTransferInId1:
				case KBulkTransferInId2:
				case KBulkTransferInId3:
					iTransferComplete |= aTransferId;
					break; //switch(aTransferId)
					
				default:
					OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0493_TRANSFERCOMPLETEL_DUP03, "Bad Transfer ID");
					iTransferComplete = 0; //reset
					err = KUnexpectedTransferID;
					msg.Format(_L("<Error %d> Unexpected transfer ID, wanted %d or %d or %d or %d, got %d"),
						       err, KBulkTransferInId0, KBulkTransferInId1, KBulkTransferInId2, KBulkTransferInId3, aTransferId);
					break; //switch(aTransferId)
				}
				
			if(err==KErrNone && iTransferComplete == (KBulkTransferInId0 | KBulkTransferInId1 | KBulkTransferInId2 | KBulkTransferInId3))
				{
				// compare data rcvd now
				TPtrC8 data1(iInTransfer[0]->DataPolled());		
				TPtrC8 data2(iInTransfer[1]->DataPolled());		
				TPtrC8 data3(iInTransfer[2]->DataPolled());		
				TPtrC8 data4(iInTransfer[3]->DataPolled());		
				if(ValidateData(data1, KLiteralEnglish16().Mid(0*KHostNumReadBytes, KHostNumReadBytes)) == EFalse)
					{
					err = KErrCompletion; //indicates data validation failure
					break; //switch(iCaseStep)
					}

				if(ValidateData(data2, KLiteralEnglish16().Mid(1*KHostNumReadBytes, KHostNumReadBytes)) == EFalse)
					{
					err = KErrCompletion; //indicates data validation failure
					break; //switch(iCaseStep)
					}

				if(ValidateData(data3, KLiteralEnglish16().Mid(2*KHostNumReadBytes, KHostNumReadBytes)) == EFalse)
					{
					err = KErrCompletion; //indicates data validation failure
					break; //switch(iCaseStep)
					}

				if(ValidateData(data4, KLiteralEnglish16().Mid(3*KHostNumReadBytes, iNumTransferBytes - 3*KHostNumReadBytes)) == EFalse)
					{
					err = KErrCompletion; //indicates data validation failure
					break; //switch(iCaseStep)
					}

				if(err == KErrNone)
					{
					// Comparison is a match
					OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0493_TRANSFERCOMPLETEL_DUP04, "Comparison for IN transfer is a match - Number of Transfer Bytes %d", iNumTransferBytes);
					iTransferComplete = 0; //reset
					if(++iNumTransferBytes<=KFinishNumTransferBytes)
						/*
						Loop round again having added one to 
						the number of transfer bytes to be used.
						In this test there should be three repeats of the underlying 
						"Transfer Out followed by Transfer In" test.
						The repeats will use
						KStartNumTransferBytes, 
						KStartNumTransferBytes+1,
						and finally
						KFinishNumTransferBytes.
						*/
						{
						iCaseStep = ETransferOut;
						TEndpointReadUntilShortRequest request(1,1,KClientNumReadBytes);// EP1 because 1st reader EP
						iControlEp0->SendRequest(request,this);
						break; //switch(iCaseStep)
						}
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
	
	if(err!=KErrNone)
		{	
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0493_TRANSFERCOMPLETEL_DUP05, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0493_TRANSFERCOMPLETEL_EXIT, this );
		return;
		}	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0493_TRANSFERCOMPLETEL_EXIT_DUP01, this );
	}
	

void CUT_PBASE_T_USBDI_0493::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0493_DEVICEINSERTEDL_ENTRY, this );
	Cancel();
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0493_DEVICEINSERTEDL, "****** Father William Pattern Length is %d bytes! *********", KLiteralEnglish16Length);
	if(BaseBulkDeviceInsertedL(aDeviceHandle) == EDeviceConfigurationError)
		// Prepare for response from control transfer to client
		{
		iCaseStep = EFailed;
		}
	
	// Create the bulk transfers	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0493_DEVICEINSERTEDL_DUP01, "Trying to create the bulk transfers - size 0x%u", KBulkTransferMaxSize);
	iInTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkTransferMaxSize,*this,KBulkTransferInId0);
	iInTransfer[1] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkTransferMaxSize,*this,KBulkTransferInId1);
	iInTransfer[2] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkTransferMaxSize,*this,KBulkTransferInId2);
	iInTransfer[3] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,KBulkTransferMaxSize,*this,KBulkTransferInId3);
	iOutTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkTransferMaxSize,*this,KBulkTransferOutId0);
	iOutTransfer[1] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkTransferMaxSize,*this,KBulkTransferOutId1);
	iOutTransfer[2] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkTransferMaxSize,*this,KBulkTransferOutId2);
	iOutTransfer[3] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkOut,iUsbInterface1,KBulkTransferMaxSize,*this,KBulkTransferOutId3);
		
	// Initialise the descriptors for transfer		
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0493_DEVICEINSERTEDL_DUP02, "Initialising the transfer descriptors");
	TInt err = iUsbInterface1.InitialiseTransferDescriptors();
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to initialise transfer descriptors"),err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0493_DEVICEINSERTEDL_DUP03, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0493_DEVICEINSERTEDL_EXIT, this );
		return;
		}
			
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0493_DEVICEINSERTEDL_DUP04, "Ask client to read %d bytes of data", KClientNumReadBytes);
	iCaseStep = ETransferOut;
	TEndpointReadUntilShortRequest request(1,1,KClientNumReadBytes);// EP1 because 1st reader EP
	iControlEp0->SendRequest(request,this);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0493_DEVICEINSERTEDL_EXIT_DUP01, this );
	}
	
	} //end namespace
