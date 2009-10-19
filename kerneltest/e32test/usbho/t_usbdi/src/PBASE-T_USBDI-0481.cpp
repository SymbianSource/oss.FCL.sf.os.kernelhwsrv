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
// @file PBASE-T_USBDI-0481.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0481.h"
#include "testpolicy.h"
#include "testdebug.h"
#include "modelleddevices.h"

namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0481");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0481,TBool> CUT_PBASE_T_USBDI_0481::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0481* CUT_PBASE_T_USBDI_0481::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0481* self = new (ELeave) CUT_PBASE_T_USBDI_0481(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	} 
	

CUT_PBASE_T_USBDI_0481::CUT_PBASE_T_USBDI_0481(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	} 


void CUT_PBASE_T_USBDI_0481::ConstructL()
	{
	iTestDevice = new RUsbDeviceA(this);
	BaseConstructL();
	}


CUT_PBASE_T_USBDI_0481::~CUT_PBASE_T_USBDI_0481()
	{
	LOG_FUNC
	
	Cancel();
	
	// Close interfaces	
	iUsbInterface0.Close();
	iUsbInterface1.Close();
	
	// Free resources
	delete iControlEp0;
	delete iActorFDF;
	if(!IsHost() && iTestDevice)
		{
		iTestDevice->Close();
		}		
	delete iTestDevice;
	}
	
void CUT_PBASE_T_USBDI_0481::ExecuteHostTestCaseL()	
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


void CUT_PBASE_T_USBDI_0481::ExecuteDeviceTestCaseL()	
	{
	LOG_FUNC
	
	// Create the test device	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	
	// Connect the device to the host	
	iTestDevice->SoftwareConnect();
	}


void CUT_PBASE_T_USBDI_0481::HostDoCancel()
	{
	LOG_FUNC
	
	// Cancel the test step action timeout timer	
	CancelTimeout();
	}

	
void CUT_PBASE_T_USBDI_0481::DeviceDoCancel()
	{
	LOG_FUNC
	
	// Cancel the device (the activity timer and the error reporting)	
	iTestDevice->CancelSubscriptionToReports();
	}
	
	
void CUT_PBASE_T_USBDI_0481::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
		RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	LOG_FUNC
	
	}
	
	
void CUT_PBASE_T_USBDI_0481::DeviceInsertedL(TUint aDeviceHandle)
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
	
	// Get the token for interface 0	
	err = testDevice.Device().GetTokenForInterface(0,iToken0);
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to retrieve token for interface 0",err);
		TestFailed(err);
		}
	
	// Open interface 0
	err = iUsbInterface0.Open(iToken0);
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to open interface 0");
		TestFailed(err);
		}
		
		// Get the token for interface 1	
	TUint32 token1(0);
	err = testDevice.Device().GetTokenForInterface(1,token1);
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> token1 for interface 1",err);
		return TestFailed(err);
		}
	// Open interface 1
	err = iUsbInterface1.Open(token1); // Alternate interface setting 0
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to open interface 1");
		return TestFailed(err);
		}

	// Get the token for interface 1 again, fails
	TUint32 token1Bis(0);
	err = testDevice.Device().GetTokenForInterface(1,token1Bis);
	if(err != KErrInUse)
		{
		RDebug::Printf("GetTokenForInterface(1,token1Bis), err != KErrInUse");
		return TestFailed(err);
		}
		
	// Open interface 1 again, fails
	RDebug::Printf("open it twice, catch error Code");
	err = iUsbInterface1.Open(token1); // Alternate interface setting 0
	if(err != KErrInUse)
		{
		RDebug::Printf("iUsbInterface1.Open(token1), err != KErrInUse");
		return TestFailed(err);
		}

	// test ok
	User::After(1000000);
	iCaseStep = EPassed;
	TTestCasePassed request;
	iControlEp0->SendRequest(request,this);	
	}
	
void CUT_PBASE_T_USBDI_0481::Ep0TransferCompleteL(TInt aCompletionCode)
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
	
void CUT_PBASE_T_USBDI_0481::DeviceRemovedL(TUint aDeviceHandle)
	{
	LOG_FUNC

	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error
	TestFailed(KErrDisconnected);
	}
	
	
void CUT_PBASE_T_USBDI_0481::BusErrorL(TInt aError)
	{
	LOG_FUNC

	// This test case handles no failiures on the bus
	TestFailed(aError);
	}


void CUT_PBASE_T_USBDI_0481::HostRunL()
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
void CUT_PBASE_T_USBDI_0481::DeviceRunL()
	{
	LOG_FUNC	
	// Disconnect the device	
	iTestDevice->SoftwareDisconnect();	
	// Complete the test case request	
	TestPolicy().SignalTestComplete(iStatus.Int());
	}

	
	}
