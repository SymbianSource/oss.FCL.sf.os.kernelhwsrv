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
// @file PBASE-T_USBDI-0489.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0489.h"
#include "testpolicy.h"
#include "testdebug.h"
#include "modelleddevices.h"

namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0489");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0489,TBool> CUT_PBASE_T_USBDI_0489::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0489* CUT_PBASE_T_USBDI_0489::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0489* self = new (ELeave) CUT_PBASE_T_USBDI_0489(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	} 
	

CUT_PBASE_T_USBDI_0489::CUT_PBASE_T_USBDI_0489(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	} 


void CUT_PBASE_T_USBDI_0489::ConstructL()
	{
	iTestDevice = new RUsbDeviceVendor(this);
	BaseConstructL();
	}


CUT_PBASE_T_USBDI_0489::~CUT_PBASE_T_USBDI_0489()
	{
	LOG_FUNC
	
	Cancel();
	
	// Close the interface
	iUsbInterface0.Close();
	
	delete iControlEp0;
	delete iActorFDF;
	if(!IsHost() && iTestDevice)
		{
		iTestDevice->Close();
		}		
	delete iTestDevice;
	}
	
void CUT_PBASE_T_USBDI_0489::ExecuteHostTestCaseL()	
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


void CUT_PBASE_T_USBDI_0489::ExecuteDeviceTestCaseL()	
	{
	LOG_FUNC
	
	// Create the test device
	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	
	// Connect the device to the host
	
	iTestDevice->SoftwareConnect();
	}


void CUT_PBASE_T_USBDI_0489::HostDoCancel()
	{
	LOG_FUNC
	
	// Cancel the test step action timeout timer
	
	CancelTimeout();
	}

	
void CUT_PBASE_T_USBDI_0489::DeviceDoCancel()
	{
	LOG_FUNC
	
	// Cancel the device (the activity timer and the error reporting)
	
	iTestDevice->CancelSubscriptionToReports();
	}
	
	
void CUT_PBASE_T_USBDI_0489::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
		RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	LOG_FUNC
	
	}
	
	
void CUT_PBASE_T_USBDI_0489::DeviceInsertedL(TUint aDeviceHandle)
	{
	LOG_FUNC

	// Cancel the timeout timer
	
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
	
	// Get the token for the interface
	
	err = testDevice.Device().GetTokenForInterface(0,iToken0);
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to retrieve token for interface 0",err);
		TestFailed(err);
		}
	
	// Open the interface	
	err = iUsbInterface0.Open(iToken0);
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to open interface 0");
		TestFailed(err);
		}
	

	// Send a request to control endpoint 0 to continuously NAK the request
	
	iCaseStep = EInProgress;
	TNakRequest request(0);
	iControlEp0->SendRequest(request,this);
	
	// Wait 1 second then cancel EP0 transfer
	User::After(1000000);
	RDebug::Printf("Cancelling EP0 transfer");
	iUsbInterface0.CancelEP0Transfer();
	}
	
void CUT_PBASE_T_USBDI_0489::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	LOG_FUNC
	
	switch(iCaseStep)
		{
		case EInProgress:
			{
			if(aCompletionCode != KErrCancel)
				{
				RDebug::Printf("<Error %d> Nakking request was not cancelled by stack",aCompletionCode);
				return TestFailed(aCompletionCode);
				}
		
			// No panic or leave so passed
			
			RDebug::Printf("No leave or panic occured so open interface again and send test passed");
			
			// Open the interface		
			/*TODO DMA TInt err(iUsbInterface0.Open(iToken0));
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Unable to open interface 0"));
				return TestFailed(err);
				}
			
			RDebug::Printf("Interface 0 re-opened"));*/
			
			iCaseStep = EPassed;
			TTestCasePassed request;
			iControlEp0->SendRequest(request,this);			
			}
			break;
			
		case EPassed:
			TestPassed();
			break;
			
		case EFailed:
		default:
			TestFailed(KErrCompletion);
			break;
		}
	}
	
void CUT_PBASE_T_USBDI_0489::DeviceRemovedL(TUint aDeviceHandle)
	{
	LOG_FUNC

	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error

	TestFailed(KErrDisconnected);
	}
	
	
void CUT_PBASE_T_USBDI_0489::BusErrorL(TInt aError)
	{
	LOG_FUNC

	// This test case handles no failiures on the bus

	TestFailed(aError);
	}


void CUT_PBASE_T_USBDI_0489::HostRunL()
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

/**
Called when the device has reported any kind of error in its opertaion
or when the device has been informed by the host to report success
*/
void CUT_PBASE_T_USBDI_0489::DeviceRunL()
	{
	LOG_FUNC
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	}

	
	}
