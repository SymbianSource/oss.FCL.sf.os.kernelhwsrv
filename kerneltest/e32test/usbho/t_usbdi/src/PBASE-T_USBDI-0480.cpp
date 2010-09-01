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
// @file PBASE-T_USBDI-0480.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0480.h"
#include "testpolicy.h"
#include "modelleddevices.h"

	
namespace NUnitTesting_USBDI
	{
	
_LIT8(KDataPayload,"DEADBEEF");
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0480");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0480,TBool> CUT_PBASE_T_USBDI_0480::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0480* CUT_PBASE_T_USBDI_0480::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0480* self = new (ELeave) CUT_PBASE_T_USBDI_0480(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_0480::CUT_PBASE_T_USBDI_0480(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iPtrTemp(NULL,0)
	{
	} 


void CUT_PBASE_T_USBDI_0480::ConstructL()
	{
	iTestDevice = new RUsbDeviceA(this);
	BaseConstructL();
	}


CUT_PBASE_T_USBDI_0480::~CUT_PBASE_T_USBDI_0480()
	{
	LOG_FUNC
	
	Cancel();
	
	// Close pipe before interface	
	iUsbInterface1.Close();
	iUsbInterface0.Close();
	
	delete iTemp;
	delete iControlEp0;
	delete iActorFDF;
	if(!IsHost() && iTestDevice)
		{
		iTestDevice->Close();
		}		
	delete iTestDevice;
	}
	
void CUT_PBASE_T_USBDI_0480::ExecuteHostTestCaseL()	
	{
	LOG_FUNC
	
	iActorFDF = CActorFDF::NewL(*this);
	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	iActorFDF->Monitor();
	
	TimeoutIn(30);
	}
	
void CUT_PBASE_T_USBDI_0480::HostDoCancel()
	{
	LOG_FUNC
	
	CancelTimeout();
	}
	
	
void CUT_PBASE_T_USBDI_0480::ExecuteDeviceTestCaseL()	
	{
	LOG_FUNC
	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	iTestDevice->SoftwareConnect();
	}
	
void CUT_PBASE_T_USBDI_0480::DeviceDoCancel()
	{
	LOG_FUNC
	
	// Cancel the device	
	iTestDevice->CancelSubscriptionToReports();	
	}
	
	
void CUT_PBASE_T_USBDI_0480::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
	RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	LOG_FUNC
	
	Cancel();
	}
	

void CUT_PBASE_T_USBDI_0480::DeviceInsertedL(TUint aDeviceHandle)
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
		TBuf<64> msg;
		msg.Format(_L("<Error %d> Token for interface 1 could not be retrieved"),err);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		return;
		}
	err = iUsbInterface1.Open(token1);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to open interface 1 using token %d"),err,token1);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		return;
		}
	
	// Get the endpoint descriptor
	TUsbEndpointDescriptor endpointDescriptor;

	err = iUsbInterface1.GetEndpointDescriptor(0,1,endpointDescriptor);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Descriptor for endpoint 0 cannot be obtained"),err);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		return;
		}
	TUint16 maxPacketSize(endpointDescriptor.MaxPacketSize());
	
	RDebug::Printf("Maximum packet size for endpoint 1 on interface 1 setting 0 is: %d",maxPacketSize);
	
	// Perform a device directed control transfer
	User::After(1000000);	
	iCaseStep = EEmptyDeviceXfer;
	TEmptyDeviceRequest request;
	iControlEp0->SendRequest(request,this);
	}


void CUT_PBASE_T_USBDI_0480::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	LOG_FUNC
	
	RDebug::Printf("Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{
		if(iCaseStep == EFailed)
			{
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
			
		// Send an empty interface directed request		
		case EEmptyDeviceXfer:
			{
			iCaseStep = EEmptyInterfaceXfer;
			TEmptyInterfaceRequest request(1); // Direct at interface 1
			iControlEp0->SendRequest(request,this);
			}
			break;	
		
		// Send a test payload request to device directed 		
		case EEmptyInterfaceXfer:
			{
			iCaseStep = EDataPutDeviceXfer;
			TDevicePutPayloadRequest request(KDataPayload);
			iControlEp0->SendRequest(request,this);
			}
			break;
			
		// Send a test payload request to interface directed		
		case EDataPutDeviceXfer:
			{
			iCaseStep = EDataPutInterfaceXfer;
			TInterfacePutPayloadRequest request(1,KDataPayload); // Direct at interface 1
			iControlEp0->SendRequest(request,this);
			}
			break;
			
		// Test case passed		
		case EDataPutInterfaceXfer:
			{
			iCaseStep = EPassed;
			User::After(1000000);
			TTestCasePassed request;
			iControlEp0->SendRequest(request,this);			
			}
			break;
			
		default:
			RDebug::Printf("<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	}
	
	
void CUT_PBASE_T_USBDI_0480::DeviceRemovedL(TUint aDeviceHandle)
	{
	LOG_FUNC

	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error
	TestFailed(KErrDisconnected);
	}
	
	
void CUT_PBASE_T_USBDI_0480::BusErrorL(TInt aError)
	{
	LOG_FUNC

	// This test case handles no failiures on the bus	
	TestFailed(KErrGeneral);
	}

void CUT_PBASE_T_USBDI_0480::HostRunL()
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

void CUT_PBASE_T_USBDI_0480::DeviceRunL()
	{
	LOG_FUNC
	
	// Disconnect the device	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request	
	TestPolicy().SignalTestComplete(iStatus.Int());
	}

	
	}
