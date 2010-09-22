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
// @file PBASE-T_USBDI-0477.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0477.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0477Traces.h"
#endif


namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0477");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0477,TBool> CUT_PBASE_T_USBDI_0477::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0477* CUT_PBASE_T_USBDI_0477::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0477_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0477* self = new (ELeave) CUT_PBASE_T_USBDI_0477(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0477::CUT_PBASE_T_USBDI_0477(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0477_CUT_PBASE_T_USBDI_0477_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_CUT_PBASE_T_USBDI_0477_EXIT, this );
	} 

 
void CUT_PBASE_T_USBDI_0477::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0477_CONSTRUCTL_ENTRY, this );
	iTestDevice = new RUsbDeviceA(this);
	BaseConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0477::~CUT_PBASE_T_USBDI_0477()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0477_CUT_PBASE_T_USBDI_0477_ENTRY_DUP01, this );
	Cancel();

	iTestPipe.Close();
	iUsbInterface1.Close();
	iUsbInterface0.Close();

	delete iClientAction;
	delete iActorFDF;
	if(!IsHost() && iTestDevice)
		{
		iTestDevice->Close();
		}		
	delete iTestDevice;
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_CUT_PBASE_T_USBDI_0477_EXIT_DUP01, this );
	}
	
	
void CUT_PBASE_T_USBDI_0477::ExecuteHostTestCaseL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0477_EXECUTEHOSTTESTCASEL_ENTRY, this );
	
	iActorFDF = CActorFDF::NewL(*this);
	iClientAction = new (ELeave) CEp0Transfer(iUsbInterface0);
	iActorFDF->Monitor();
	TimeoutIn(30);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_EXECUTEHOSTTESTCASEL_EXIT, this );
	}

	
void CUT_PBASE_T_USBDI_0477::ExecuteDeviceTestCaseL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0477_EXECUTEDEVICETESTCASEL_ENTRY, this );
	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	iTestDevice->SoftwareConnect();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_EXECUTEDEVICETESTCASEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0477::HostDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0477_HOSTDOCANCEL_ENTRY, this );
	
	// Cancel the test step timeout
	
	CancelTimeout();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_HOSTDOCANCEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0477::DeviceDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0477_DEVICEDOCANCEL_ENTRY, this );
	
	// Cancel the device
	
	iTestDevice->CancelSubscriptionToReports();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_DEVICEDOCANCEL_EXIT, this );
	}
	
TBool CUT_PBASE_T_USBDI_0477::CheckFirstInterfaceDescriptorDeviceA(TUsbInterfaceDescriptor& aIfDescriptor)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0477_CHECKFIRSTINTERFACEDESCRIPTORDEVICEA_ENTRY, this );
	/*	Interface0	[setting0]
		Interface1	[setting0]
						[endpoint1] Bulk out
						[endpoint2] Bulk in
					[setting1]			
						[endpoint1] Interrupt in
						[endpoint2] Bulk out
						[endpoint3] Bulk in
	*/					
		// alt. setting 0
		TUsbInterfaceDescriptor::Cast(&aIfDescriptor); // is it an interface?
		CHECK_RET_BOOL(&aIfDescriptor != 0);
		
		// EP1
		TUsbGenericDescriptor* desc = aIfDescriptor.iFirstChild;
		TUsbEndpointDescriptor::Cast(desc);
		CHECK_RET_BOOL(desc != 0);
						
		// EP 2
		desc = desc->iNextPeer;
		TUsbEndpointDescriptor::Cast(desc);
		CHECK_RET_BOOL(desc != 0);		
		desc = desc->iNextPeer; // no peer
		CHECK_RET_BOOL(desc == 0);
		
		// alt. setting 1
		desc = aIfDescriptor.iNextPeer;
		TUsbInterfaceDescriptor::Cast(desc);
		CHECK_RET_BOOL(desc != 0);
		
		// EP1
		desc = desc->iFirstChild;
		TUsbEndpointDescriptor::Cast(desc);
		CHECK_RET_BOOL(desc != 0);
		
		// EP 2
		desc = desc->iNextPeer;
		TUsbEndpointDescriptor::Cast(desc);
		CHECK_RET_BOOL(desc != 0);
		
		// EP 3 
		desc = desc->iNextPeer;
		TUsbEndpointDescriptor::Cast(desc);
		CHECK_RET_BOOL(desc != 0);			
		desc = desc->iNextPeer; // no peer
		CHECK_RET_BOOL(desc == 0); 
		
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_CHECKFIRSTINTERFACEDESCRIPTORDEVICEA, "CheckFirstInterfaceDescriptorDeviceA successfull!");
		OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0477_CHECKFIRSTINTERFACEDESCRIPTORDEVICEA_EXIT, this, ETrue );
		return ETrue;
	}
	
void CUT_PBASE_T_USBDI_0477::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_ENTRY, this );
	Cancel();
	TInt err(KErrNone);
	
	// Validate that device is as expected
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case	

		OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL, "<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,testDevice.SerialNumber(),TestCaseId());

		// Start the connection timeout again
		TimeoutIn(30);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_EXIT, this );
		return;
		}
			
	TUint32 token0,token1; // Token for an interface
	
	err = testDevice.Device().GetTokenForInterface(0,token0);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP01, "<Error %d> Token for interface 0 could not be retrieved",err);
		return TestFailed(err);
		}
		
	err = iUsbInterface0.Open(token0);
	if(err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP02, "<Error %d> Unable to open interface 0 using token %d",err,token0);
		return TestFailed(err);
		}
	
	err = testDevice.Device().GetTokenForInterface(1,token1);
	if(err != KErrNone)
		{
		// Test case has failed
		TBuf<256> msg;
		_LIT(string, "<Error %d> Token for interface could not be retrieved");
		msg.Format(string,err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP03, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iClientAction->SendRequest(request,this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_EXIT_DUP01, this );
		return;
		}

	err = iUsbInterface1.Open(token1); // Default interface setting 0
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to open interface using token %d"),err,token1);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP04, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iClientAction->SendRequest(request,this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_EXIT_DUP02, this );
		return;
		}
		
			
	// check interface descriptor now				 
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP05, "check Interface descriptor now");
	TUsbInterfaceDescriptor ifDescriptor;
	CHECK(iUsbInterface1.GetInterfaceDescriptor(ifDescriptor) == KErrNone);	
	CHECK(CheckFirstInterfaceDescriptorDeviceA(ifDescriptor));
																				
	//  Enumerate Endpoints On Interface 1 alt. setting 0
	CHECK(iUsbInterface1.EnumerateEndpointsOnInterface(0) == 2);
	
	// get busId now				
	TUsbBusId busId;	 
	CHECK(iUsbInterface1.GetBusId(busId) == KErrNone);
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP06, "busId(Interface) = %d",busId);
	
	// get device speed now
	RUsbInterface::TDeviceSpeed deviceSpeed;
	CHECK(iUsbInterface1.GetDeviceSpeed(deviceSpeed) == KErrNone);
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP07, "GetDeviceSpeed = %d", deviceSpeed);
	CHECK(deviceSpeed == RUsbInterface::EHighSpeed);	
		
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP08, "Number of alternate interface settings available: %d",iUsbInterface1.GetAlternateInterfaceCount());
	gtest(iUsbInterface1.GetAlternateInterfaceCount() == 2);
	
	// Select alternate interface setting 1	
	err = iUsbInterface1.SelectAlternateInterface(1);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Selecting alternate interface setting 1 on interface 1"),err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP09, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iClientAction->SendRequest(request,this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_EXIT_DUP03, this );
		return;
		}	
	
	// Open a pipe for endpoint (Bulk out)	
	TInt endpointAddress;
	err = GetEndpointAddress(iUsbInterface1,1,KTransferTypeBulk,KEpDirectionOut,endpointAddress);
	if(err != KErrNone)
		{
		TBuf<128> msg;
		msg.Format(_L("<Error %d> Could not get address for Bulk out endpoint"),err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP10, msg);
		TTestCaseFailed request(err,msg);
		iClientAction->SendRequest(request,this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_EXIT_DUP04, this );
		return;
		}
		
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP11, "Opening pipe for endpoint address %02x on interface 1 setting 1",endpointAddress);
	err = iUsbInterface1.OpenPipeForEndpoint(iTestPipe,endpointAddress,EFalse);
	if(err != KErrNone)
		{
		TBuf<128> msg;
		msg.Format(_L("<Error %d> Unable to open pipe on interface 1 setting 0"),err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP12, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iClientAction->SendRequest(request,this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_EXIT_DUP05, this );
		return;
		}
		
	// get busId				
	TUsbBusId busIdPipe;	 
	CHECK(iTestPipe.GetBusId(busIdPipe) == KErrNone);
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP13, "busId(Pipe) = %d",busIdPipe);
	CHECK(busIdPipe == busId);
	
	TUsbEndpointId usbEpId;
	CHECK(iTestPipe.GetEndpointId(usbEpId) == KErrNone);
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP14, "EndpointId = %d",usbEpId); 
	
	// check ep descriptor now				 
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP15, "check ep descriptor now");
	TUsbEndpointDescriptor epDescriptor;
	

	CHECK(iTestPipe.GetEndpointDescriptor(epDescriptor) == KErrNone);
	TUsbEndpointDescriptor::Cast(&epDescriptor);
	CHECK(&epDescriptor != 0);
	CHECK(epDescriptor.iFirstChild == 0); // no children	
	CHECK(epDescriptor.iNextPeer != 0);	// 1 peer

		
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP16, "Pipe established now closing");
	iTestPipe.Close();
	
	// Select alternate interface 2, error expected
	err = iUsbInterface1.SelectAlternateInterface(2);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP17, "<Warning %d> Selecting alternate interface 2",err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP18, "...Rolling Back....");
		
		// Establish pipes on rollback
		// Open a pipe for endpoint (Bulk out)		
		err = iUsbInterface1.OpenPipeForEndpoint(iTestPipe,endpointAddress,EFalse);
		if(err != KErrNone)
			{
			TBuf<128> msg;
			msg.Format(_L("<Error %d> Unable to open pipe on interface 1 setting 1 for rollback case"),err);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP19, msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(err,msg);
			iClientAction->SendRequest(request,this);
			OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_EXIT_DUP06, this );
			return;
			}
		
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_DUP20, "Pipe established on rollback now closing");
		iTestPipe.Close();
		}
	
	// Inform client device test case successful	
	iCaseStep = EPassed;
	TTestCasePassed request;
	iClientAction->SendRequest(request,this);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_DEVICEINSERTEDL_EXIT_DUP07, this );
	}
	
	
void CUT_PBASE_T_USBDI_0477::DeviceRemovedL(TUint aDeviceHandle)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0477_DEVICEREMOVEDL_ENTRY, this );

	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error

	TestFailed(KErrDisconnected);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_DEVICEREMOVEDL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0477::BusErrorL(TInt aError)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0477_BUSERRORL_ENTRY, this );

	// This test case handles no failiures on the bus
	
	TestFailed(aError);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_BUSERRORL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0477::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
	RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0477_DEVICESTATECHANGEL_ENTRY, this );
	Cancel();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_DEVICESTATECHANGEL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0477::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0477_EP0TRANSFERCOMPLETEL_ENTRY, this );
	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(iCaseStep == EPassed)
		{
		TestPassed();
		}
		
	if(iCaseStep == EFailed)
		{
		TestFailed(KErrCompletion);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_EP0TRANSFERCOMPLETEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0477::HostRunL()
	{
    OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0477_HOSTRUNL_ENTRY, this );

	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{
		// Action timeout
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_HOSTRUNL, "<Error> Action timeout");
		TestFailed(KErrTimedOut);
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0477_HOSTRUNL_DUP01, "<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_HOSTRUNL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0477::DeviceRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0477_DEVICERUNL_ENTRY, this );
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0477_DEVICERUNL_EXIT, this );
	}
	
	
	}

