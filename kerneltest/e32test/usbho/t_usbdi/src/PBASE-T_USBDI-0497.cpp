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
// @file PBASE-T_USBDI-0497.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0497.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "testliterals.h"


 

namespace NUnitTesting_USBDI
	{
const TInt KBulkMaxTransferSize = 40000;
const TUint KHostNumWriteBytes1 = 32000;
const TUint KHostNumWriteBytes2 = 32000;
const TInt KDeviceNumReadBytes = 64000;
const TUint KHostNumReadBytes1 = 32000;
const TUint KHostNumReadBytes2 = 32001; //allow or ZLP


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

const TInt KUnexpectedTransferID = -101;
const TInt KUndefinedStep	 = -102;

const TInt KMaxTimeDiffPercentage = 60;



_LIT(KTestCaseId,"PBASE-T_USBDI-0497");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0497,TBool> CUT_PBASE_T_USBDI_0497::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0497* CUT_PBASE_T_USBDI_0497::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0497* self = new (ELeave) CUT_PBASE_T_USBDI_0497(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_0497::CUT_PBASE_T_USBDI_0497(TBool aHostRole)
:	CBaseBulkTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	} 


void CUT_PBASE_T_USBDI_0497::ConstructL()
	{
	BaseBulkConstructL();
	}


CUT_PBASE_T_USBDI_0497::~CUT_PBASE_T_USBDI_0497()
	/**
	The transfer objects have test specific names - so perform transfer specific clean up here.
	*/
	{
	LOG_FUNC
	
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
	}
	
	
void CUT_PBASE_T_USBDI_0497::Ep0TransferCompleteL(TInt aCompletionCode)
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
		
		case ERequestDeviceRead:
			{
			RDebug::Printf("Asking client for 'Read' on interface 2");
			iCaseStep = ETransferOut;	
			TEndpointReadRequest request(2,1,KDeviceNumReadBytes);// EP1 means endpoint index 1 not the actual endpoint number
			iControlEp0->SendRequest(request,this);
			}
			break;
			
		case ETransferOut:
			RDebug::Printf("Try to write data on BOTH interfaces");
	
			iStartTime[0].HomeTime();
			iStartTime[1].HomeTime();
	
			iIfc1OutTransfer[0]->TransferOut(KLiteralFrench4(), KHostNumWriteBytes1, EFalse);
			iIfc1OutTransfer[1]->TransferOut(KLiteralFrench4(), KHostNumWriteBytes1, KHostNumWriteBytes2, EFalse);
			iIfc2OutTransfer[0]->TransferOut(KLiteralEnglish8(), KHostNumWriteBytes1, EFalse);
			iIfc2OutTransfer[1]->TransferOut(KLiteralEnglish8(), KHostNumWriteBytes1, KHostNumWriteBytes2, EFalse);
			break;
		
		case ERequestDeviceWriteBack:
			{
			RDebug::Printf("Asking client for 'Write' back on interface 2");
			iCaseStep = ETransferIn;	
			TWriteCachedReadDataRequest request(2,1,1);// EP1 means endpoint index 1 not the actual endpoint number
			iControlEp0->SendRequest(request,this);
			}
			break;
			
		case ETransferIn:
			RDebug::Printf("Try to receive data on BOTH interfaces");
	
			iStartTime[0].HomeTime();
			iStartTime[1].HomeTime();
			
			iIfc1InTransfer[0]->TransferIn(KHostNumReadBytes1);
			iIfc1InTransfer[1]->TransferIn(KHostNumReadBytes2);
			iIfc2InTransfer[0]->TransferIn(KHostNumReadBytes1);
			iIfc2InTransfer[1]->TransferIn(KHostNumReadBytes2);
			break;
	
		default:
			RDebug::Printf("<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	}
	
void CUT_PBASE_T_USBDI_0497::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	LOG_FUNC
	Cancel();
	
	TInt err(KErrNone);
	TBuf<256> msg;
	RDebug::Printf("Transfer completed (id=%d), aCompletionCode = %d, test step = %d",aTransferId, aCompletionCode, iCaseStep);

	switch(iCaseStep)
		{
		case ETransferOut:
			if(aCompletionCode != KErrNone)
				{
				iIfc1OutTransfer[0]->Cancel();
				iIfc1OutTransfer[1]->Cancel();
				iIfc2OutTransfer[0]->Cancel();
				iIfc2OutTransfer[1]->Cancel();
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
					RDebug::Printf("Transfer OUT %d completed (Transfer Completion Aggregation Mask 0x%x)", aTransferId, iTransferComplete);
					break; //switch(aTransferId)
				default:
					iTransferComplete = 0; //reset
					err = KUnexpectedTransferID;
					msg.Format(_L("<Error %d> Unexpected transfer ID, wanted %d or %d, got %d"),
							       err, KIfc1BulkTransferInId1, KIfc1BulkTransferInId2, aTransferId);
					break; //switch(aTransferId)
				}
			
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
				RDebug::Printf("All OUT Transfers Completed Successfully: Transfer Completion Aggregation Mask 0x%x", iTransferComplete);
				iTransferComplete = 0; //reset
				err = CheckAndResetTimes(0, 1, KMaxTimeDiffPercentage);
				if(err!=KErrNone)
					{
					break; //switch(iCaseStep)
					}
	
				RDebug::Printf("Asking client for 'Write' back on interface 1");
				iCaseStep = ERequestDeviceWriteBack;
				TWriteCachedReadDataRequest request(1,1,1);// EP1 means endpoint index 1 not the actual endpoint number
				iControlEp0->SendRequest(request,this);
				}
			break; //switch(iCaseStep)

		case ETransferIn:
			if(aCompletionCode != KErrNone)
				{
				iIfc1InTransfer[0]->Cancel();
				iIfc1InTransfer[1]->Cancel();
				iIfc2InTransfer[0]->Cancel();
				iIfc2InTransfer[1]->Cancel();
				err = KErrCorrupt;			
				
				msg.Format(_L("<Error %d> No data sent on bulk IN request"),aCompletionCode);
				break; //switch(iCaseStep)
				}
	
			switch(aTransferId)
				{
				case KIfc1BulkTransferInId1:
				case KIfc1BulkTransferInId2:
				case KIfc2BulkTransferInId1:
				case KIfc2BulkTransferInId2:
					iTransferComplete |= aTransferId;
					RDebug::Printf("Transfer OUT %d completed (Transfer Completion Aggregation Mask 0x%x)", aTransferId, iTransferComplete);
					break; //switch(aTransferId)
	
				default:
					iTransferComplete = 0; //reset
					err = KUnexpectedTransferID;
					msg.Format(_L("<Error %d> Unexpected transfer ID, wanted %d or %d, got %d"),
							       err, KIfc1BulkTransferInId1, KIfc1BulkTransferInId2, aTransferId);
					break; //switch(aTransferId)
				}
			         
			if(err==KErrNone && iTimeElapsed[0] == 0 && (iTransferComplete & KIfc1BulkTransferInIdMask) == KIfc1BulkTransferInIdMask)
				//Record time elapsed for Interface 1 if not yet recorded.
				{
				RecordTime(0);
				}

			if(err==KErrNone && iTimeElapsed[1] == 0 && (iTransferComplete & KIfc2BulkTransferInIdMask) == KIfc2BulkTransferInIdMask) 
				//Record time elapsed for Interface 2 if not yet recorded.
				{
				RecordTime(1);
				}

			if(err==KErrNone && (iTransferComplete & KBulkTransferInIdMask) == KBulkTransferInIdMask)
				{
				// ok, compare data rcvd now
				RDebug::Printf("All IN Transfers Completed Successfully: Transfer Completion Aggregation Mask 0x%x", iTransferComplete);
				iTransferComplete = 0; //reset
				err = CheckAndResetTimes(0, 1, KMaxTimeDiffPercentage);
				if(err!=KErrNone)
					{
					break; //switch(iCaseStep)
					}
	
				TPtrC8 data1(iIfc1InTransfer[0]->DataPolled());	
				TPtrC8 data2(iIfc1InTransfer[1]->DataPolled());		
				TPtrC8 data3(iIfc2InTransfer[0]->DataPolled());		
				TPtrC8 data4(iIfc2InTransfer[1]->DataPolled());		
				//Validate first transfer on Interface 1 for number of bytes originally written.
				if(ValidateData(data1, KLiteralFrench4(), KHostNumWriteBytes1) == EFalse)
					{
					RDebug::Printf("Validation failure 1st transfer, Interface 1");
					err = KErrCompletion; //indicates data validation failure
					break; //switch(iCaseStep)
					}

				//Validate second transfer on Interface 1 for number of bytes originally written.
				if(ValidateData(data2, KLiteralFrench4(), KHostNumWriteBytes1, KHostNumWriteBytes2) == EFalse)
					{
					RDebug::Printf("Validation failure 2nd transfer, Interface 1");
					err = KErrCompletion; //indicates data validation failure
					break; //switch(iCaseStep)
					}

				//Validate first transfer on Interface 2 for number of bytes originally written.
				if(ValidateData(data3, KLiteralEnglish8(), KHostNumWriteBytes1) == EFalse)
					{
					RDebug::Printf("Validation failure 1st transfer, Interface 2");
					err = KErrCompletion; //indicates data validation failure
					break; //switch(iCaseStep)
					}

				//Validate second transfer on Interface 2 for number of bytes originally written.
				if(ValidateData(data4, KLiteralEnglish8(), KHostNumWriteBytes1, KHostNumWriteBytes2) == EFalse)
					{
					RDebug::Printf("Validation failure 2nd transfer, Interface 2");
					err = KErrCompletion; //indicates data validation failure
					break; //switch(iCaseStep)
					}

				// Comparison is a match
				RDebug::Printf("Comparison for IN transfer is a match");
				iCaseStep = EPassed;
				TTestCasePassed request;
				iControlEp0->SendRequest(request,this);
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

	if(err == KErrTooBig)
		//indicates timing validation failure
		{
		msg.Format(_L("<Error %d> Timer comparison showed too great a difference in transfer times between the two interfaces"), err);
		}
	
	if(err!=KErrNone)
		{	
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);
		}	
	}
	
void CUT_PBASE_T_USBDI_0497::DeviceInsertedL(TUint aDeviceHandle)
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
			RDebug::Printf("Initialising the transfer descriptors - interface 1");
			err = iUsbInterface1.InitialiseTransferDescriptors();
			if(err != KErrNone)
				{
				msg.Format(_L("<Error %d> Unable to initialise transfer descriptors (Interface 1)"),err);
				}
			else
				{
				// Initialise the descriptors for transfer		
				RDebug::Printf("Initialising the transfer descriptors (Interface 2)");
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
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		}
	else
		{
		RDebug::Printf("Asking client for 'Read' on interface 1");
		iCaseStep = ERequestDeviceRead;	
		TEndpointReadRequest request(1,1,KDeviceNumReadBytes);// EP1 means endpoint index 1 not the actual endpoint number
		iControlEp0->SendRequest(request,this);
		}
	}
	
	} //end namespace
