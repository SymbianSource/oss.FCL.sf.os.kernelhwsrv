// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0487Traces.h"
#endif


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
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0487_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0487* self = new (ELeave) CUT_PBASE_T_USBDI_0487(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0487::CUT_PBASE_T_USBDI_0487(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0487_CUT_PBASE_T_USBDI_0487_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_CUT_PBASE_T_USBDI_0487_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0487::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0487_CONSTRUCTL_ENTRY, this );
	iTestDevice = new RUsbDeviceA(this);
	BaseConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0487::~CUT_PBASE_T_USBDI_0487()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0487_CUT_PBASE_T_USBDI_0487_ENTRY_DUP01, this );
	
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_CUT_PBASE_T_USBDI_0487_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0487::ExecuteHostTestCaseL()	
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0487_EXECUTEHOSTTESTCASEL_ENTRY, this );
	
	iActorFDF = CActorFDF::NewL(*this);
	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	iActorFDF->Monitor();
	
	TimeoutIn(30);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_EXECUTEHOSTTESTCASEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0487::HostDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0487_HOSTDOCANCEL_ENTRY, this );
	
	// Cancel the timeout timer	
	CancelTimeout();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_HOSTDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0487::ExecuteDeviceTestCaseL()	
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0487_EXECUTEDEVICETESTCASEL_ENTRY, this );
	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	iTestDevice->SoftwareConnect();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_EXECUTEDEVICETESTCASEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0487::DeviceDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0487_DEVICEDOCANCEL_ENTRY, this );
	// Cancel the device	
	iTestDevice->CancelSubscriptionToReports();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_DEVICEDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0487::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
		TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0487_DEVICESTATECHANGEL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_DEVICESTATECHANGEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0487::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_ENTRY, this );
	Cancel();
	TInt err(KErrNone);
	
	// Validate that device is as expected
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case	

		OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL, "<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,testDevice.SerialNumber(),TestCaseId());

		// Start the connection timeout again

		CancelTimeout();
		iTimer.After(iStatus,30000000);
		SetActive();
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_EXIT, this );
		return;
		}

	TUint32 token0,token1;
	err = testDevice.Device().GetTokenForInterface(0,token0);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_DUP01, "<Error %d> Token for interface 0 could not be retrieved",err);
		return TestFailed(err);
		}
	err = iUsbInterface0.Open(token0); // Default interface setting 0
	if(err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_DUP02, "<Error %d> Unable to open interface 0 using token %d",err,token0);
		return TestFailed(err);
		}
	
	err = testDevice.Device().GetTokenForInterface(1,token1);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Token for interface 1 could not be retrieved"),err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_DUP03, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);
		}
	err = iUsbInterface1.Open(token1); // Default interface setting 0
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to open interface 1 using token %d"),err,token1);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_DUP04, msg);
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
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_DUP05, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_EXIT_DUP01, this );
		return;
		}
	
	// Open a pipe for endpoint (Int in)	
	TInt endpointAddress;
	err = GetEndpointAddress(iUsbInterface1,1,KTransferTypeInterrupt,KEpDirectionIn,endpointAddress);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> unable to get endpoint address for interrupt in endpoint"),err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_DUP06, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_EXIT_DUP02, this );
		return;
		}	
	
	err = iUsbInterface1.OpenPipeForEndpoint(iInPipe,endpointAddress,EFalse);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to open pipe for endpoint 1"),err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_DUP07, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);		
		}	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_DUP08, "Opened pipe to endpoint address %08x for interrupt transfer to host",endpointAddress);
	
	
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
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_DUP09, "Initialising the transfer descriptors");
	err = iUsbInterface1.InitialiseTransferDescriptors();
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_DUP10, "<Error %d> Unable to initialise transfer descriptors",err);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_EXIT_DUP03, this );
		return;
		} 
			
	// que interrupt transfer
	err = iTransferIn->TransferInL(KDataPayload1().Length());
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to queue an interrupt transfer"),err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_DUP11, msg);
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
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_DUP12, msg);
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
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_DUP13, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);	
		}	
		
	// Timeout the interrupt transfer in 7 seconds	
	TimeoutIn(7);	
	// Instruct the client device to write the following data through the valid endpoint	
	TEndpointWriteRequest request(1,1,KDataPayload1);
	iControlEp0->SendRequest(request,this);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_DEVICEINSERTEDL_EXIT_DUP04, this );
	}


void CUT_PBASE_T_USBDI_0487::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0487_EP0TRANSFERCOMPLETEL_ENTRY, this );
	
	RDebug::Printf("Ep0TransferCompleteL with aCompletionCode = %d"),aCompletionCode;
	
	if(aCompletionCode != KErrNone)
		{	
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_EP0TRANSFERCOMPLETEL, msg);
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_EP0TRANSFERCOMPLETEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0487::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0487_TRANSFERCOMPLETEL_ENTRY, this );
	Cancel();
	
	OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_TRANSFERCOMPLETEL, "Transfer %d completed with %d",aTransferId,aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Transfer %d did not complete successfully"),aCompletionCode,aTransferId);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_TRANSFERCOMPLETEL_DUP01, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(aCompletionCode,msg);
		return iControlEp0->SendRequest(request,this);
		}
			
	if(aTransferId == KInterruptTransferId1)
		{
		TPtrC8 data(iTransferIn->DataPolled());
		
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_TRANSFERCOMPLETEL_DUP02, "data.Length 1()= %d", data.Length());
		
			TBuf<256> msg;
		for(int i = 0 ; i < data.Length(); i++)
			{		
			msg.AppendFormat(_L("%02x"),data[i]);
			}
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_TRANSFERCOMPLETEL_DUP03, msg);
		// Compare the data to what is expected		
		if(data.Compare(KDataPayload1) != 0)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Interrupt data received does not match data sent"),KErrCompletion);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_TRANSFERCOMPLETEL_DUP04, msg);
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
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_TRANSFERCOMPLETEL_DUP05, "data.Length 2()= %d", data.Length());
	
		TBuf<256> msg;
		for(int i = 0 ; i < data.Length(); i++)
			{		
			msg.AppendFormat(_L("%02x"),data[i]);
			}
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_TRANSFERCOMPLETEL_DUP06, msg);
				
		// Compare the data to what is expected		
		if(data.Compare(KDataPayload1) != 0)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Interrupt data received does not match data sent"),KErrCompletion);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_TRANSFERCOMPLETEL_DUP07, msg);
			//TODO if test fails! Test fail code below WAS commented out
			iCaseStep = EFailed;
			TTestCaseFailed request(KErrCompletion,msg);
			return iControlEp0->SendRequest(request,this);
			}
		User::After(2000000); // so that interrupt transfer 2 complete on peripheral side			
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
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_TRANSFERCOMPLETEL_DUP08, msg);
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
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_TRANSFERCOMPLETEL_DUP09, "<Error> Unknown transfer identity");
		TestFailed(KErrUnknown);	
		}	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_TRANSFERCOMPLETEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0487::DeviceRemovedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0487_DEVICEREMOVEDL_ENTRY, this );
	
	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error	
	TestFailed(KErrDisconnected);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_DEVICEREMOVEDL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0487::BusErrorL(TInt aError)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0487_BUSERRORL_ENTRY, this );
	
	// This test case handles no failiures on the bus	
	TestFailed(KErrCompletion);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_BUSERRORL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0487::HostRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0487_HOSTRUNL_ENTRY, this );
	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{
		// Action timeout
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_HOSTRUNL, "<Error> Action timeout");
		TestFailed(KErrTimedOut);
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0487_HOSTRUNL_DUP01, "<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_HOSTRUNL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0487::DeviceRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0487_DEVICERUNL_ENTRY, this );
	
	// Disconnect the device	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request	
	TestPolicy().SignalTestComplete(iStatus.Int());
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0487_DEVICERUNL_EXIT, this );
	}

	
	}
