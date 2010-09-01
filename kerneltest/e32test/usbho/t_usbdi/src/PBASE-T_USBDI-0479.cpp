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
// @file PBASE-T_USBDI-0479.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0479.h"
#include "testpolicy.h"
#include "modelleddevices.h"


namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0479");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0479,TBool> CUT_PBASE_T_USBDI_0479::iFunctor(KTestCaseId);

CUT_PBASE_T_USBDI_0479* CUT_PBASE_T_USBDI_0479::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0479* self = new (ELeave) CUT_PBASE_T_USBDI_0479(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_0479::CUT_PBASE_T_USBDI_0479(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	} 


void CUT_PBASE_T_USBDI_0479::ConstructL()
	{
	iTestDevice = new RUsbDeviceVendor(this);
	BaseConstructL();
	}


CUT_PBASE_T_USBDI_0479::~CUT_PBASE_T_USBDI_0479()
	{
	LOG_FUNC
	
	Cancel();
	
	iDuplicateUsbInterface0.Close();
	iUsbInterface0.Close();

	delete iClientAction;
	delete iActorFDF;
	if(!IsHost() && iTestDevice)
		{
		iTestDevice->Close();
		}		
	delete iTestDevice;
	}
	
void CUT_PBASE_T_USBDI_0479::ExecuteHostTestCaseL()	
	{
	LOG_FUNC
	
	iActorFDF = CActorFDF::NewL(*this);
	iClientAction = new (ELeave) CEp0Transfer(iUsbInterface0);
	iActorFDF->Monitor();
	TimeoutIn(30);
	}
	
void CUT_PBASE_T_USBDI_0479::HostDoCancel()
	{
	LOG_FUNC
	
	// Cancel timing out the test step
	
	CancelTimeout();
	}
	
	
void CUT_PBASE_T_USBDI_0479::ExecuteDeviceTestCaseL()	
	{
	LOG_FUNC
	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	iTestDevice->SoftwareConnect();
	}
	
void CUT_PBASE_T_USBDI_0479::DeviceDoCancel()
	{
	LOG_FUNC
	
	// Cancel the device	
	iTestDevice->CancelSubscriptionToReports();
	}
	
	
void CUT_PBASE_T_USBDI_0479::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
	RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	LOG_FUNC
	Cancel();
	}
	
void CUT_PBASE_T_USBDI_0479::DeviceInsertedL(TUint aDeviceHandle)
	{
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

	TUint32 validToken;
	TUint32 invalidToken(KMaxTUint-1); // The known invalid token
	
	err = testDevice.Device().GetTokenForInterface(0,validToken);
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Token for interface could not be retrieved",err);
		return TestFailed(err);
		}
	err = iUsbInterface0.Open(validToken); // Default interface setting 0
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to open interface 0 using token %d",err,validToken);
		return TestFailed(err);
		}
	
	err = iDuplicateUsbInterface0.Open(invalidToken);
	if(err == KErrNone)
		{
		TBuf<64> msg;
		msg.Format(_L("<Error %d> Able to open an interface that is not present"),KErrCorrupt);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(KErrCorrupt,msg);
		iClientAction->SendRequest(request,this);
		}
	else
		{
		RDebug::Printf("Opening interface with invalid token failed with %d",err);
		User::After(1000000);
		iCaseStep = EPassed;
		TTestCasePassed request;
		iClientAction->SendRequest(request,this);	
		}
	}
	
	
void CUT_PBASE_T_USBDI_0479::DeviceRemovedL(TUint aDeviceHandle)
	{
	LOG_FUNC
	
	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error

	TestFailed(KErrDisconnected);
	}
	
	
void CUT_PBASE_T_USBDI_0479::BusErrorL(TInt aError)
	{
	LOG_FUNC
	
	// This test case handles no failiures on the bus
	
	TestFailed(aError);	
	}


void CUT_PBASE_T_USBDI_0479::Ep0TransferCompleteL(TInt aCompletionCode)
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
	
	
void CUT_PBASE_T_USBDI_0479::HostRunL()
	{
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

void CUT_PBASE_T_USBDI_0479::DeviceRunL()
	{
	LOG_FUNC
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	}

	
	}
