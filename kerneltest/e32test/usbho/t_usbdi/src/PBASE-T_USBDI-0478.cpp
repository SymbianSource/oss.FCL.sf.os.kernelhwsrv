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
// @file PBASE-T_USBDI-0478.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0478.h"
#include "testpolicy.h"
#include "modelleddevices.h"


namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0478");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0478,TBool> CUT_PBASE_T_USBDI_0478::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0478* CUT_PBASE_T_USBDI_0478::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0478* self = new (ELeave) CUT_PBASE_T_USBDI_0478(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_0478::CUT_PBASE_T_USBDI_0478(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	} 


void CUT_PBASE_T_USBDI_0478::ConstructL()
	{
	iTestDevice = new RUsbDeviceA(this);
	BaseConstructL();
	}


CUT_PBASE_T_USBDI_0478::~CUT_PBASE_T_USBDI_0478()
	{
	LOG_FUNC
	
	Cancel();

	// Close the interfaces
	
	iDuplicateUsbInterface1.Close();
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
	
void CUT_PBASE_T_USBDI_0478::ExecuteHostTestCaseL()	
	{
	LOG_FUNC

	// Create the actor for the Function Driver Framework 
	
	iActorFDF = CActorFDF::NewL(*this);

	// Create the control transfer for requests

	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	
	// Monitor for devices

	iActorFDF->Monitor();
	
	// Start the connection timeout timer

	TimeoutIn(30);
	}
	
void CUT_PBASE_T_USBDI_0478::HostDoCancel()
	{
	LOG_FUNC

	// Cancel the timeout timer for activity

	CancelTimeout();
	}
	
	
void CUT_PBASE_T_USBDI_0478::ExecuteDeviceTestCaseL()	
	{
	LOG_FUNC

	// Create the test device for this test case

	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();

	// Connect the test device to the host

	iTestDevice->SoftwareConnect();
	}
	
void CUT_PBASE_T_USBDI_0478::DeviceDoCancel()
	{
	LOG_FUNC

	// Cancel the test device
	
	iTestDevice->CancelSubscriptionToReports();
	}
	
	
void CUT_PBASE_T_USBDI_0478::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
	RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	LOG_FUNC

	Cancel();
	}
	
	
void CUT_PBASE_T_USBDI_0478::DeviceInsertedL(TUint aDeviceHandle)
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
		RDebug::Printf("<Error %d> Token for interface 1 could not be retrieved",err);
		return TestFailed(err);
		}

	err = iUsbInterface1.Open(token1);
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to open interface using token %d",err,token1);
		return TestFailed(err);
		}

	// Acting as another FD open the interface
	
	err = iDuplicateUsbInterface1.Open(token1);
	if(err == KErrNone)
		{
		TBuf<64> msg;
		msg.Format(_L("<Error> Able to open a concurrent hande to an interface"));
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(KErrCompletion,msg);
		iControlEp0->SendRequest(request,this);
		return;
		}
	RDebug::Printf("Attempt to open concurrent interface handle failed with: %d",err);
	
	// Inform client device test case successful 
	User::After(1000000);
	iCaseStep = EPassed;
	TTestCasePassed request;
	iControlEp0->SendRequest(request,this);
	}
	
	
void CUT_PBASE_T_USBDI_0478::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	LOG_FUNC
	
	RDebug::Printf("Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
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
	
	
void CUT_PBASE_T_USBDI_0478::DeviceRemovedL(TUint aDeviceHandle)
	{
	LOG_FUNC

	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error

	TestFailed(KErrDisconnected);
	}
	
	
void CUT_PBASE_T_USBDI_0478::BusErrorL(TInt aError)
	{
	LOG_FUNC

	// This test case handles no failiures on the bus
	
	TestFailed(aError);	
	}

void CUT_PBASE_T_USBDI_0478::HostRunL()
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

void CUT_PBASE_T_USBDI_0478::DeviceRunL()
	{
	LOG_FUNC
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	}

	
	}
