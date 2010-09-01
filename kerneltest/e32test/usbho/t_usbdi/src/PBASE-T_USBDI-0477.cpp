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
// @file PBASE-T_USBDI-0477.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0477.h"
#include "testpolicy.h"
#include "modelleddevices.h"


namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0477");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0477,TBool> CUT_PBASE_T_USBDI_0477::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0477* CUT_PBASE_T_USBDI_0477::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0477* self = new (ELeave) CUT_PBASE_T_USBDI_0477(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_0477::CUT_PBASE_T_USBDI_0477(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	} 

 
void CUT_PBASE_T_USBDI_0477::ConstructL()
	{
	iTestDevice = new RUsbDeviceA(this);
	BaseConstructL();
	}


CUT_PBASE_T_USBDI_0477::~CUT_PBASE_T_USBDI_0477()
	{
	LOG_FUNC
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
	}
	
	
void CUT_PBASE_T_USBDI_0477::ExecuteHostTestCaseL()
	{
	LOG_FUNC
	
	iActorFDF = CActorFDF::NewL(*this);
	iClientAction = new (ELeave) CEp0Transfer(iUsbInterface0);
	iActorFDF->Monitor();
	TimeoutIn(30);
	}

	
void CUT_PBASE_T_USBDI_0477::ExecuteDeviceTestCaseL()
	{
	LOG_FUNC
	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	iTestDevice->SoftwareConnect();
	}
	
	
void CUT_PBASE_T_USBDI_0477::HostDoCancel()
	{
	LOG_FUNC
	
	// Cancel the test step timeout
	
	CancelTimeout();
	}


void CUT_PBASE_T_USBDI_0477::DeviceDoCancel()
	{
	LOG_FUNC
	
	// Cancel the device
	
	iTestDevice->CancelSubscriptionToReports();
	}
	
TBool CUT_PBASE_T_USBDI_0477::CheckFirstInterfaceDescriptorDeviceA(TUsbInterfaceDescriptor& aIfDescriptor)
	{
	LOG_FUNC	
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
		
		RDebug::Printf("CheckFirstInterfaceDescriptorDeviceA successfull!");
		return ETrue;
	}
	
void CUT_PBASE_T_USBDI_0477::DeviceInsertedL(TUint aDeviceHandle)
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
		TimeoutIn(30);
		return;
		}
			
	TUint32 token0,token1; // Token for an interface
	
	err = testDevice.Device().GetTokenForInterface(0,token0);
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Token for interface 0 could not be retrieved",err);
		return TestFailed(err);
		}
		
	err = iUsbInterface0.Open(token0);
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to open interface 0 using token %d",err,token0);
		return TestFailed(err);
		}
	
	err = testDevice.Device().GetTokenForInterface(1,token1);
	if(err != KErrNone)
		{
		// Test case has failed
		TBuf<256> msg;
		_LIT(string, "<Error %d> Token for interface could not be retrieved");
		msg.Format(string,err);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iClientAction->SendRequest(request,this);
		return;
		}

	err = iUsbInterface1.Open(token1); // Default interface setting 0
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to open interface using token %d"),err,token1);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iClientAction->SendRequest(request,this);
		return;
		}
		
			
	// check interface descriptor now				 
	RDebug::Printf("check Interface descriptor now");
	TUsbInterfaceDescriptor ifDescriptor;
	CHECK(iUsbInterface1.GetInterfaceDescriptor(ifDescriptor) == KErrNone);	
	CHECK(CheckFirstInterfaceDescriptorDeviceA(ifDescriptor));
																				
	//  Enumerate Endpoints On Interface 1 alt. setting 0
	CHECK(iUsbInterface1.EnumerateEndpointsOnInterface(0) == 2);
	
	// get busId now				
	TUsbBusId busId;	 
	CHECK(iUsbInterface1.GetBusId(busId) == KErrNone);
	RDebug::Printf("busId(Interface) = %d",busId);
	
	// get device speed now
	RUsbInterface::TDeviceSpeed deviceSpeed;
	CHECK(iUsbInterface1.GetDeviceSpeed(deviceSpeed) == KErrNone);
	RDebug::Printf("GetDeviceSpeed = %d", deviceSpeed);
	CHECK(deviceSpeed == RUsbInterface::EFullSpeed);	
		
	RDebug::Printf("Number of alternate interface settings available: %d",iUsbInterface1.GetAlternateInterfaceCount());
	gtest(iUsbInterface1.GetAlternateInterfaceCount() == 2);
	
	// Select alternate interface setting 1	
	err = iUsbInterface1.SelectAlternateInterface(1);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Selecting alternate interface setting 1 on interface 1"),err);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iClientAction->SendRequest(request,this);
		return;
		}	
	
	// Open a pipe for endpoint (Bulk out)	
	TInt endpointAddress;
	err = GetEndpointAddress(iUsbInterface1,1,KTransferTypeBulk,KEpDirectionOut,endpointAddress);
	if(err != KErrNone)
		{
		TBuf<128> msg;
		msg.Format(_L("<Error %d> Could not get address for Bulk out endpoint"),err);
		RDebug::Print(msg);
		TTestCaseFailed request(err,msg);
		iClientAction->SendRequest(request,this);
		return;
		}
		
	RDebug::Printf("Opening pipe for endpoint address %02x on interface 1 setting 1",endpointAddress);
	err = iUsbInterface1.OpenPipeForEndpoint(iTestPipe,endpointAddress,EFalse);
	if(err != KErrNone)
		{
		TBuf<128> msg;
		msg.Format(_L("<Error %d> Unable to open pipe on interface 1 setting 0"),err);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iClientAction->SendRequest(request,this);
		return;
		}
		
	// get busId				
	TUsbBusId busIdPipe;	 
	CHECK(iTestPipe.GetBusId(busIdPipe) == KErrNone);
	RDebug::Printf("busId(Pipe) = %d",busIdPipe);
	CHECK(busIdPipe == busId);
	
	TUsbEndpointId usbEpId;
	CHECK(iTestPipe.GetEndpointId(usbEpId) == KErrNone);
	RDebug::Printf("EndpointId = %d",usbEpId); 
	
	// check ep descriptor now				 
	RDebug::Printf("check ep descriptor now");
	TUsbEndpointDescriptor epDescriptor;
	

	CHECK(iTestPipe.GetEndpointDescriptor(epDescriptor) == KErrNone);
	TUsbEndpointDescriptor::Cast(&epDescriptor);
	CHECK(&epDescriptor != 0);
	CHECK(epDescriptor.iFirstChild == 0); // no children	
	CHECK(epDescriptor.iNextPeer != 0);	// 1 peer

		
	RDebug::Printf("Pipe established now closing");
	iTestPipe.Close();
	
	// Select alternate interface 2, error expected
	err = iUsbInterface1.SelectAlternateInterface(2);
	if(err != KErrNone)
		{
		RDebug::Printf("<Warning %d> Selecting alternate interface 2",err);
		RDebug::Printf("...Rolling Back....");
		
		// Establish pipes on rollback
		// Open a pipe for endpoint (Bulk out)		
		err = iUsbInterface1.OpenPipeForEndpoint(iTestPipe,endpointAddress,EFalse);
		if(err != KErrNone)
			{
			TBuf<128> msg;
			msg.Format(_L("<Error %d> Unable to open pipe on interface 1 setting 1 for rollback case"),err);
			RDebug::Print(msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(err,msg);
			iClientAction->SendRequest(request,this);
			return;
			}
		
		RDebug::Printf("Pipe established on rollback now closing");
		iTestPipe.Close();
		}
	
	// Inform client device test case successful	
	iCaseStep = EPassed;
	TTestCasePassed request;
	iClientAction->SendRequest(request,this);
	}
	
	
void CUT_PBASE_T_USBDI_0477::DeviceRemovedL(TUint aDeviceHandle)
	{
	LOG_FUNC

	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error

	TestFailed(KErrDisconnected);
	}
	
	
void CUT_PBASE_T_USBDI_0477::BusErrorL(TInt aError)
	{
	LOG_FUNC

	// This test case handles no failiures on the bus
	
	TestFailed(aError);
	}
	
	
void CUT_PBASE_T_USBDI_0477::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
	RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	LOG_FUNC
	Cancel();
	}

void CUT_PBASE_T_USBDI_0477::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	LOG_FUNC
	
	RDebug::Printf("Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(iCaseStep == EPassed)
		{
		TestPassed();
		}
		
	if(iCaseStep == EFailed)
		{
		TestFailed(KErrCompletion);
		}
	}
	
	
void CUT_PBASE_T_USBDI_0477::HostRunL()
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

void CUT_PBASE_T_USBDI_0477::DeviceRunL()
	{
	LOG_FUNC
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	}
	
	
	}

