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
// @file PBASE-T_USBDI-0485.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0485.h"
#include "testpolicy.h"
#include "testdebug.h"
#include "modelleddevices.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0485Traces.h"
#endif

namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0485");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0485,TBool> CUT_PBASE_T_USBDI_0485::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0485* CUT_PBASE_T_USBDI_0485::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0485_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0485* self = new (ELeave) CUT_PBASE_T_USBDI_0485(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0485_NEWL_EXIT, ( TUint )( self ) );
	return self;
	} 
	

CUT_PBASE_T_USBDI_0485::CUT_PBASE_T_USBDI_0485(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0485_CUT_PBASE_T_USBDI_0485_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0485_CUT_PBASE_T_USBDI_0485_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0485::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0485_CONSTRUCTL_ENTRY, this );
	iTestDevice = new RUsbDeviceVendor(this);
	BaseConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0485_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0485::~CUT_PBASE_T_USBDI_0485()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0485_CUT_PBASE_T_USBDI_0485_ENTRY_DUP01, this );
	
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0485_CUT_PBASE_T_USBDI_0485_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0485::ExecuteHostTestCaseL()	
	{
OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0485_EXECUTEHOSTTESTCASEL_ENTRY, this );

	// Create the actor for the Function Driver Framework 
	
	iActorFDF = CActorFDF::NewL(*this);

	// Create the control transfer for requests

	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	
	// Monitor for devices

	iActorFDF->Monitor();
	
	// Start the connection timeout timer

	TimeoutIn(30);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0485_EXECUTEHOSTTESTCASEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0485::ExecuteDeviceTestCaseL()	
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0485_EXECUTEDEVICETESTCASEL_ENTRY, this );
	
	// Create the test device
	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	
	// Connect the device to the host
	
	iTestDevice->SoftwareConnect();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0485_EXECUTEDEVICETESTCASEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0485::HostDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0485_HOSTDOCANCEL_ENTRY, this );
	
	// Cancel the test step action timeout timer
	
	CancelTimeout();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0485_HOSTDOCANCEL_EXIT, this );
	}

	
void CUT_PBASE_T_USBDI_0485::DeviceDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0485_DEVICEDOCANCEL_ENTRY, this );
	
	// Cancel the device (the activity timer and the error reporting)
	
	iTestDevice->CancelSubscriptionToReports();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0485_DEVICEDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0485::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
		RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0485_DEVICESTATECHANGEL_ENTRY, this );
	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0485_DEVICESTATECHANGEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0485::DeviceInsertedL(TUint aDeviceHandle)
	{
OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0485_DEVICEINSERTEDL_ENTRY, this );

	// Cancel the timeout timer
	
	Cancel();
	
	TInt err(KErrNone);
	
	// Validate that device is as expected
	
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case	

		OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_0485_DEVICEINSERTEDL, "<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,testDevice.SerialNumber(),TestCaseId());

		// Start the connection timeout again

		TimeoutIn(30);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0485_DEVICEINSERTEDL_EXIT, this );
		return;
		}	
	
	// Get the token for the interface
	
	err = testDevice.Device().GetTokenForInterface(0,iToken0);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0485_DEVICEINSERTEDL_DUP01, "<Error %d> Unable to retrieve token for interface 0",err);
		TestFailed(err);
		}
	
	// Open the interface	
	err = iUsbInterface0.Open(iToken0);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0485_DEVICEINSERTEDL_DUP02, "<Error %d> Unable to open interface 0", err);
		TestFailed(err);
		}
	

	// Send a request to control endpoint 0 to continuously NAK the request
	
	iCaseStep = EInProgress;
	TNakRequest request(0);
	iControlEp0->SendRequest(request,this);
	
	// Wait 1 second then close the interface	
	User::After(1000000);
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0485_DEVICEINSERTEDL_DUP03, "Closing interface 0");
	iUsbInterface0.Close();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0485_DEVICEINSERTEDL_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0485::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0485_EP0TRANSFERCOMPLETEL_ENTRY, this );
	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0485_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
		
	switch(iCaseStep)
		{
		case EInProgress:
			{
			if(aCompletionCode != KErrCancel)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0485_EP0TRANSFERCOMPLETEL_DUP01, "<Error %d> Nakking request was not cancelled by stack",aCompletionCode);
				return TestFailed(aCompletionCode);
				}
		
			// No panic or leave so passed
			
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0485_EP0TRANSFERCOMPLETEL_DUP02, "No leave or panic occured so open interface again and send test passed");
			
			// Open the interface
			
			TInt err(iUsbInterface0.Open(iToken0));
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0485_EP0TRANSFERCOMPLETEL_DUP03, "<Error %d> Unable to open interface 0", err);
				return TestFailed(err);
				}
			
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0485_EP0TRANSFERCOMPLETEL_DUP04, "Interface 0 re-opened");
			
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0485_EP0TRANSFERCOMPLETEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0485::DeviceRemovedL(TUint aDeviceHandle)
	{
OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0485_DEVICEREMOVEDL_ENTRY, this );

	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error

	TestFailed(KErrDisconnected);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0485_DEVICEREMOVEDL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0485::BusErrorL(TInt aError)
	{
OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0485_BUSERRORL_ENTRY, this );

	// This test case handles no failiures on the bus

	TestFailed(aError);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0485_BUSERRORL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0485::HostRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0485_HOSTRUNL_ENTRY, this );
	
	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{
		// Action timeout
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0485_HOSTRUNL, "<Error> Action timeout");
		TestFailed(KErrTimedOut);
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0485_HOSTRUNL_DUP01, "<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0485_HOSTRUNL_EXIT, this );
	}

/**
Called when the device has reported any kind of error in its opertaion
or when the device has been informed by the host to report success
*/
void CUT_PBASE_T_USBDI_0485::DeviceRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0485_DEVICERUNL_ENTRY, this );
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0485_DEVICERUNL_EXIT, this );
	}

	
	}
