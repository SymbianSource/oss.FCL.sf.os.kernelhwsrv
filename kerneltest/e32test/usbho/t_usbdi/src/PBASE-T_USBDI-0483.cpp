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
// @file PBASE-T_USBDI-0483.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0483.h"
#include "testpolicy.h"
#include "modelleddevices.h"


namespace NUnitTesting_USBDI
	{
const TInt KGetConfigDescriptor = 0x0200;	
_LIT(KTestCaseId,"PBASE-T_USBDI-0483");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0483,TBool> CUT_PBASE_T_USBDI_0483::iFunctor(KTestCaseId);

CUT_PBASE_T_USBDI_0483* CUT_PBASE_T_USBDI_0483::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0483* self = new (ELeave) CUT_PBASE_T_USBDI_0483(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_0483::CUT_PBASE_T_USBDI_0483(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	} 


void CUT_PBASE_T_USBDI_0483::ConstructL()
	{
	iTestDevice = new RUsbDeviceB(this);
	BaseConstructL();
	}


CUT_PBASE_T_USBDI_0483::~CUT_PBASE_T_USBDI_0483()
	{
	Cancel();
	
	// Close the interface
	iUsbInterface0.Close();

	delete iActorFDF;
	delete iConfigDescriptorData;
	if(!IsHost() && iTestDevice)
		{
		iTestDevice->Close();
		}		
	delete iTestDevice;
	}
	
void CUT_PBASE_T_USBDI_0483::ExecuteHostTestCaseL()	
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
	
void CUT_PBASE_T_USBDI_0483::HostDoCancel()
	{
	LOG_FUNC
	
	// Cancel the test step timeout timer
	
	CancelTimeout();
	}
	
	
void CUT_PBASE_T_USBDI_0483::ExecuteDeviceTestCaseL()	
	{
	LOG_FUNC
	
	// Create the test device
	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	
	// Connect the device to the host
	
	iTestDevice->SoftwareConnect();
	}
	
	
void CUT_PBASE_T_USBDI_0483::DeviceDoCancel()
	{
	LOG_FUNC
	
	// Cancel the reporting or errors from opertaions perfomed by the test device
	
	iTestDevice->CancelSubscriptionToReports();
	}
	
	
void CUT_PBASE_T_USBDI_0483::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
		RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	LOG_FUNC       
	
	
	}
	
void CUT_PBASE_T_USBDI_0483::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	LOG_FUNC
	
	RDebug::Printf("Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{
		if(iCaseStep == EFailed)
			{// todo, cope with errors
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
			
		case EInProgress:
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
void CUT_PBASE_T_USBDI_0483::DeviceInsertedL(TUint aDeviceHandle)
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
		
	RDebug::Printf("iControlEp0->SendRequest(getConfiguration,this)");
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
	
	// get config. descriptor now
	TUint totalLength = testDevice.ConfigurationDescriptor().TotalLength();
	RDebug::Printf("totalLength ==== %d",totalLength);
	iConfigDescriptorData = HBufC8::NewL(totalLength); 		
	TPtr8 des(iConfigDescriptorData->Des());
	des.SetLength(totalLength);		
	TDescriptorGetRequest getConfiguration(KGetConfigDescriptor,0,des);
	iControlEp0->SendRequest(getConfiguration,this);	
	}	 

	
void CUT_PBASE_T_USBDI_0483::DeviceRemovedL(TUint aDeviceHandle)
	{
	LOG_FUNC
	
	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error
	
	TestFailed(KErrDisconnected);
	}
	
	
void CUT_PBASE_T_USBDI_0483::BusErrorL(TInt aError)
	{
	LOG_FUNC
	
	// This test case handles no failiures on the bus
	
	TestFailed(KErrCompletion);
	}


void CUT_PBASE_T_USBDI_0483::HostRunL()
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

void CUT_PBASE_T_USBDI_0483::DeviceRunL()
	{
	LOG_FUNC
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	}

	
	}
