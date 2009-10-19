// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// @file PBASE-T_USBDI-0487.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0487.h"
#include "testpolicy.h"
#include "modelleddevices.h"


namespace NUnitTesting_USBDI
	{

_LIT8(KDataPayload1,"12345678911111112"); 
// _LIT8(KDataPayload2,"12345opqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"); unused
// _LIT8(KDataPayload3,"abcdefghijklmnopqrstuvwxyzopqrstuvwxyzabcdefghijklmnopqrstuvwxyz12345"); unused

const TInt KInterruptTransferId1 = 0x01;
const TInt KInterruptTransferId2 = 0x02;
const TInt KInterruptTransferId3 = 0x03;

_LIT(KTestCaseId,"PBASE-T_USBDI-0487");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0487,TBool> CUT_PBASE_T_USBDI_0487::iFunctor(KTestCaseId);

CUT_PBASE_T_USBDI_0487* CUT_PBASE_T_USBDI_0487::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0487* self = new (ELeave) CUT_PBASE_T_USBDI_0487(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_0487::CUT_PBASE_T_USBDI_0487(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	} 


void CUT_PBASE_T_USBDI_0487::ConstructL()
	{
	iTestDevice = new RUsbDeviceA(this);
	BaseConstructL();
	}


CUT_PBASE_T_USBDI_0487::~CUT_PBASE_T_USBDI_0487()
	{
	LOG_FUNC
	
	Cancel();
	delete iTransferIn;
	delete iTransferIn2;
	delete iTransferIn3;
	
	// Close pipe(s) before interface(s)
	iInPipe.Close();

	// Close interfaces
	iUsbInterface1.Close();
	iUsbInterface0.Close();
	
	delete iControlEp0;
	delete iActorFDF;
	if(!IsHost() && iTestDevice)
		{
		iTestDevice->Close();
		}		
	delete iTestDevice;
	}
	
void CUT_PBASE_T_USBDI_0487::ExecuteHostTestCaseL()	
	{
	LOG_FUNC
	
	iActorFDF = CActorFDF::NewL(*this);
	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	iActorFDF->Monitor();
	
	TimeoutIn(30);
	}
	
void CUT_PBASE_T_USBDI_0487::HostDoCancel()
	{
	LOG_FUNC
	
	// Cancel the timeout timer	
	CancelTimeout();
	}
	
	
void CUT_PBASE_T_USBDI_0487::ExecuteDeviceTestCaseL()	
	{
	LOG_FUNC
	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	iTestDevice->SoftwareConnect();
	}
	
void CUT_PBASE_T_USBDI_0487::DeviceDoCancel()
	{
	LOG_FUNC	
	// Cancel the device	
	iTestDevice->CancelSubscriptionToReports();
	}
	
	
void CUT_PBASE_T_USBDI_0487::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
		TInt aCompletionCode)
	{
	LOG_FUNC	
	}


void CUT_PBASE_T_USBDI_0487::DeviceInsertedL(TUint aDeviceHandle)
	{
	LOG_FUNC
	Cancel();
	TInt err(KErrNone);
	
	// Validate that device is as expected
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case	

		RDebug::Printf("<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,&testDevice.SerialNumber(),&TestCaseId());

		// Start the connection timeout again

		CancelTimeout();
		iTimer.After(iStatus,30000000);
		SetActive();
		return;
		}

	TUint32 token0,token1;
	err = testDevice.Device().GetTokenForInterface(0,token0);
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Token for interface 0 could not be retrieved",err);
		return TestFailed(err);
		}
	err = iUsbInterface0.Open(token0); // Default interface setting 0
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to open interface 0 using token %d",err,token0);
		return TestFailed(err);
		}
	
	err = testDevice.Device().GetTokenForInterface(1,token1);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Token for interface 1 could not be retrieved"),err);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);
		}
	err = iUsbInterface1.Open(token1); // Default interface setting 0
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to open interface 1 using token %d"),err,token1);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);
		}

	// Select alternate interface setting 1	
	err = iUsbInterface1.SelectAlternateInterface(1);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Selecting alternate interface setting 1 on interface 1"),err);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		return;
		}
	
	// Open a pipe for endpoint (Int in)	
	TInt endpointAddress;
	err = GetEndpointAddress(iUsbInterface1,1,KTransferTypeInterrupt,KEpDirectionIn,endpointAddress);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> unable to get endpoint address for interrupt in endpoint"),err);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		return;
		}	
	
	err = iUsbInterface1.OpenPipeForEndpoint(iInPipe,endpointAddress,EFalse);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to open pipe for endpoint 1"),err);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);		
		}	
	RDebug::Printf("Opened pipe to endpoint address %08x for interrupt transfer to host",endpointAddress);
	
	
	// create an interrupt transfer
	iTransferIn = new (ELeave) CInterruptTransfer(iInPipe,iUsbInterface1,256,*this,KInterruptTransferId1);
	iTransferIn->RegisterTransferDescriptor();
	
	// create a 2nd  interrupt transfer
	iTransferIn2 = new (ELeave) CInterruptTransfer(iInPipe,iUsbInterface1,256,*this,KInterruptTransferId2);
	iTransferIn2->RegisterTransferDescriptor();
	
	// create a 3rd  interrupt transfer
	iTransferIn3 = new (ELeave) CInterruptTransfer(iInPipe,iUsbInterface1,256,*this,KInterruptTransferId3);
	iTransferIn3->RegisterTransferDescriptor();		
	
	// Initialise the descriptors for transfer
	RDebug::Printf("Initialising the transfer descriptors");
	err = iUsbInterface1.InitialiseTransferDescriptors();
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to initialise transfer descriptors",err);
		return;
		} 
			
	// que interrupt transfer
	err = iTransferIn->TransferInL(KDataPayload1().Length());
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to queue an interrupt transfer"),err);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);	
		}
		
	// que interrupt transfer
	err = iTransferIn2->TransferInL(KDataPayload1().Length());
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to queue an interrupt transfer"),err);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);	
		}
		
	// Que a 3rd interrupt transfer
	err = iTransferIn3->TransferInL(KDataPayload1().Length());
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to queue an interrupt transfer"),err);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);	
		}	
		
	// Timeout the interrupt transfer in 7 seconds	
	TimeoutIn(7);	
	// Instruct the client device to write the following data through the valid endpoint	
	TEndpointWriteRequest request(1,1,KDataPayload1);
	iControlEp0->SendRequest(request,this);
	}


void CUT_PBASE_T_USBDI_0487::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	LOG_FUNC
	
	RDebug::Printf("Ep0TransferCompleteL with aCompletionCode = %d"),aCompletionCode;
	
	if(aCompletionCode != KErrNone)
		{	
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
		RDebug::Print(msg);
		}

	if(iCaseStep == EPassed)
		{	
		if(aCompletionCode == KErrNone)
			{
			return TestPassed();
			}
		// else error
	    iCaseStep = EFailed;
		}
	
	if(iCaseStep == EFailed)
		{
		return TestFailed(KErrCompletion);
		}	
	}


void CUT_PBASE_T_USBDI_0487::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	LOG_FUNC
	Cancel();
	
	RDebug::Printf("Transfer %d completed with %d",aTransferId,aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Transfer %d did not complete successfully"),aCompletionCode,aTransferId);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(aCompletionCode,msg);
		return iControlEp0->SendRequest(request,this);
		}
			
	if(aTransferId == KInterruptTransferId1)
		{
		TPtrC8 data(iTransferIn->DataPolled());
		
		RDebug::Printf("data.Length 1()= %d", data.Length());
		
			TBuf<256> msg;
		for(int i = 0 ; i < data.Length(); i++)
			{		
			msg.AppendFormat(_L("%02x"),data[i]);
			}
			RDebug::Print(msg);	
		// Compare the data to what is expected		
		if(data.Compare(KDataPayload1) != 0)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Interrupt data received does not match data sent"),KErrCompletion);
			RDebug::Print(msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(KErrCompletion,msg);
			return iControlEp0->SendRequest(request,this);
			}
		User::After(3000000);		
		// Comparison is a match, continue
		TEndpointWriteRequest request(1,1,KDataPayload1);
		return iControlEp0->SendRequest(request,this);
		}
	else if(aTransferId == KInterruptTransferId2)
		{
		TPtrC8 data(iTransferIn2->DataPolled());
		RDebug::Printf("data.Length 2()= %d", data.Length());
	
		TBuf<256> msg;
		for(int i = 0 ; i < data.Length(); i++)
			{		
			msg.AppendFormat(_L("%02x"),data[i]);
			}
			RDebug::Print(msg);
				
		// Compare the data to what is expected		
		if(data.Compare(KDataPayload1) != 0)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Interrupt data received does not match data sent"),KErrCompletion);
			RDebug::Print(msg);
			//TODO if test fails! Test fail code below WAS commented out
			iCaseStep = EFailed;
			TTestCaseFailed request(KErrCompletion,msg);
			return iControlEp0->SendRequest(request,this);
			}
		// Comparison is a match, continue
		TEndpointWriteRequest request(1,1,KDataPayload1);
		return iControlEp0->SendRequest(request,this);		
	
		}
	else if(aTransferId == KInterruptTransferId3)
		{
		TPtrC8 data(iTransferIn3->DataPolled());		
		// Compare the data to what is expected		
		if(data.Compare(KDataPayload1) != 0)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Interrupt data received does not match data sent"),KErrCompletion);
			RDebug::Print(msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(KErrCompletion,msg);
			return iControlEp0->SendRequest(request,this);
			}	
		// Comparison is a match, test passed		
		iCaseStep = EPassed;
		TTestCasePassed request;
		return iControlEp0->SendRequest(request,this);
		}		
	else
		{
		RDebug::Printf("<Error> Unknown transfer identity");
		TestFailed(KErrUnknown);	
		}	
	}


void CUT_PBASE_T_USBDI_0487::DeviceRemovedL(TUint aDeviceHandle)
	{
	LOG_FUNC
	
	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error	
	TestFailed(KErrDisconnected);
	}
	
	
void CUT_PBASE_T_USBDI_0487::BusErrorL(TInt aError)
	{
	LOG_FUNC
	
	// This test case handles no failiures on the bus	
	TestFailed(KErrCompletion);
	}

void CUT_PBASE_T_USBDI_0487::HostRunL()
	{
	LOG_FUNC	
	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{
		// Action timeout
		RDebug::Printf("<Error> Action timeout");
		TestFailed(KErrTimedOut);
		}
	else
		{
		RDebug::Printf("<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	}

void CUT_PBASE_T_USBDI_0487::DeviceRunL()
	{
	LOG_FUNC
	
	// Disconnect the device	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request	
	TestPolicy().SignalTestComplete(iStatus.Int());
	}

	
	}
