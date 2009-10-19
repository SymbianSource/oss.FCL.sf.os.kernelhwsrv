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
// @file PBASE-T_USBDI-0474.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0474.h"
#include "testpolicy.h"
#include "modelleddevices.h"


namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0474");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0474,TBool> CUT_PBASE_T_USBDI_0474::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0474* CUT_PBASE_T_USBDI_0474::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0474* self = new (ELeave) CUT_PBASE_T_USBDI_0474(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_0474::CUT_PBASE_T_USBDI_0474(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	} 


void CUT_PBASE_T_USBDI_0474::ConstructL()
	{
	iTestDevice = new RUsbDeviceB(this);
	BaseConstructL();
	}


CUT_PBASE_T_USBDI_0474::~CUT_PBASE_T_USBDI_0474()
	{
	LOG_FUNC
	Cancel();
	
	iUsbInterface0.Close();

	delete iClientAction;
	delete iActorFDF;
	if(!IsHost() && iTestDevice)
		{
		iTestDevice->Close();
		}		
	delete iTestDevice;
	}
	
	
void CUT_PBASE_T_USBDI_0474::ExecuteHostTestCaseL()
	{
	LOG_FUNC
	
	iActorFDF = CActorFDF::NewL(*this);
	iClientAction = new (ELeave) CEp0Transfer(iUsbInterface0);
	iActorFDF->Monitor();
	TimeoutIn(30);
	}

	
void CUT_PBASE_T_USBDI_0474::ExecuteDeviceTestCaseL()
	{
	LOG_FUNC
	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	iTestDevice->SoftwareConnect();
	} 
	
	
void CUT_PBASE_T_USBDI_0474::HostDoCancel()
	{
	LOG_FUNC
	
	// Cancel the test step timeout
	
	CancelTimeout();
	}


void CUT_PBASE_T_USBDI_0474::DeviceDoCancel()
	{
	LOG_FUNC
	
	// Cancel the device
	
	iTestDevice->CancelSubscriptionToReports();
	}
			
void CUT_PBASE_T_USBDI_0474::DeviceInsertedL(TUint aDeviceHandle)
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
		
	// Check tree now
	TUsbGenericDescriptor deviceDesc = testDevice.DeviceDescriptor();
	TUsbGenericDescriptor configDesc = testDevice.ConfigurationDescriptor();

	// Check tree now	
	CHECK(CheckTreeAfterDeviceInsertion(testDevice, _L("RDeviceB")) == KErrNone);
		
	// Inform client device test case successful
	TUint32 token0;
	err = testDevice.Device().GetTokenForInterface(0,token0);
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Token for interface 0 could not be retrieved",err);
		return TestFailed(err);
		}
	err = iUsbInterface0.Open(token0); // Default interface setting 0
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to open interface 1 using token %d",err,token0);
		return TestFailed(err);
		} 
	 	
	iCaseStep = EPassed;
	TTestCasePassed request;
	iClientAction->SendRequest(request,this);
	}
	
	
void CUT_PBASE_T_USBDI_0474::DeviceRemovedL(TUint aDeviceHandle)
	{
	LOG_FUNC

	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error

	TestFailed(KErrDisconnected);
	}
	
	
void CUT_PBASE_T_USBDI_0474::BusErrorL(TInt aError)
	{
	LOG_FUNC

	// This test case handles no failiures on the bus
	
	TestFailed(aError);
	}
	
	
void CUT_PBASE_T_USBDI_0474::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
	RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	LOG_FUNC
	Cancel();
	}

void CUT_PBASE_T_USBDI_0474::Ep0TransferCompleteL(TInt aCompletionCode)
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
	
	
void CUT_PBASE_T_USBDI_0474::HostRunL()
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

void CUT_PBASE_T_USBDI_0474::DeviceRunL()
	{
	LOG_FUNC
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	}
	
	
	}

