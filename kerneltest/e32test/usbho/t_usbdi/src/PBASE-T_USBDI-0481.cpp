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
// @file PBASE-T_USBDI-0481.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0481.h"
#include "testpolicy.h"
#include "testdebug.h"
#include "modelleddevices.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0481Traces.h"
#endif

namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0481");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0481,TBool> CUT_PBASE_T_USBDI_0481::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0481* CUT_PBASE_T_USBDI_0481::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0481_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0481* self = new (ELeave) CUT_PBASE_T_USBDI_0481(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0481_NEWL_EXIT, ( TUint )( self ) );
	return self;
	} 
	

CUT_PBASE_T_USBDI_0481::CUT_PBASE_T_USBDI_0481(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0481_CUT_PBASE_T_USBDI_0481_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0481_CUT_PBASE_T_USBDI_0481_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0481::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0481_CONSTRUCTL_ENTRY, this );
	iTestDevice = new RUsbDeviceA(this);
	BaseConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0481_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0481::~CUT_PBASE_T_USBDI_0481()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0481_CUT_PBASE_T_USBDI_0481_ENTRY_DUP01, this );
	
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0481_CUT_PBASE_T_USBDI_0481_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0481::ExecuteHostTestCaseL()	
	{
    OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0481_EXECUTEHOSTTESTCASEL_ENTRY, this );

	// Create the actor for the Function Driver Framework 	
	iActorFDF = CActorFDF::NewL(*this);

	// Create the control transfer for requests
	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	
	// Monitor for devices
	iActorFDF->Monitor();
	
	// Start the connection timeout timer
	TimeoutIn(30);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0481_EXECUTEHOSTTESTCASEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0481::ExecuteDeviceTestCaseL()	
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0481_EXECUTEDEVICETESTCASEL_ENTRY, this );
	
	// Create the test device	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	
	// Connect the device to the host	
	iTestDevice->SoftwareConnect();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0481_EXECUTEDEVICETESTCASEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0481::HostDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0481_HOSTDOCANCEL_ENTRY, this );
	
	// Cancel the test step action timeout timer	
	CancelTimeout();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0481_HOSTDOCANCEL_EXIT, this );
	}

	
void CUT_PBASE_T_USBDI_0481::DeviceDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0481_DEVICEDOCANCEL_ENTRY, this );
	
	// Cancel the device (the activity timer and the error reporting)	
	iTestDevice->CancelSubscriptionToReports();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0481_DEVICEDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0481::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
		RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0481_DEVICESTATECHANGEL_ENTRY, this );
	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0481_DEVICESTATECHANGEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0481::DeviceInsertedL(TUint aDeviceHandle)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0481_DEVICEINSERTEDL_ENTRY, this );

	// Cancel the timeout timer	
	Cancel();
	
	TInt err(KErrNone);
	
	// Validate that device is as expected	
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case
		OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_0481_DEVICEINSERTEDL, "<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,testDevice.SerialNumber(),TestCaseId());

		// Start the connection timeout again
		TimeoutIn(30);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0481_DEVICEINSERTEDL_EXIT, this );
		return;
		}	
	
	// Get the token for interface 0	
	err = testDevice.Device().GetTokenForInterface(0,iToken0);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0481_DEVICEINSERTEDL_DUP01, "<Error %d> Unable to retrieve token for interface 0",err);
		TestFailed(err);
		}
	
	// Open interface 0
	err = iUsbInterface0.Open(iToken0);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0481_DEVICEINSERTEDL_DUP02, "<Error %d> Unable to open interface 0", err);
		TestFailed(err);
		}
		
		// Get the token for interface 1	
	TUint32 token1(0);
	err = testDevice.Device().GetTokenForInterface(1,token1);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0481_DEVICEINSERTEDL_DUP03, "<Error %d> token1 for interface 1",err);
		return TestFailed(err);
		}
	// Open interface 1
	err = iUsbInterface1.Open(token1); // Alternate interface setting 0
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0481_DEVICEINSERTEDL_DUP04, "<Error %d> Unable to open interface 1", err);
		return TestFailed(err);
		}

	// Get the token for interface 1 again, fails
	TUint32 token1Bis(0);
	err = testDevice.Device().GetTokenForInterface(1,token1Bis);
	if(err != KErrInUse)
		{
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0481_DEVICEINSERTEDL_DUP05, "GetTokenForInterface(1,token1Bis), err != KErrInUse");
		return TestFailed(err);
		}
		
	// Open interface 1 again, fails
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0481_DEVICEINSERTEDL_DUP06, "open it twice, catch error Code");
	err = iUsbInterface1.Open(token1); // Alternate interface setting 0
	if(err != KErrInUse)
		{
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0481_DEVICEINSERTEDL_DUP07, "iUsbInterface1.Open(token1), err != KErrInUse");
		return TestFailed(err);
		}

	// test ok
	User::After(1000000);
	iCaseStep = EPassed;
	TTestCasePassed request;
	iControlEp0->SendRequest(request,this);	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0481_DEVICEINSERTEDL_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0481::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0481_EP0TRANSFERCOMPLETEL_ENTRY, this );
	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0481_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{	
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0481_EP0TRANSFERCOMPLETEL_DUP01, msg);
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0481_EP0TRANSFERCOMPLETEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0481::DeviceRemovedL(TUint aDeviceHandle)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0481_DEVICEREMOVEDL_ENTRY, this );

	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error
	TestFailed(KErrDisconnected);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0481_DEVICEREMOVEDL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0481::BusErrorL(TInt aError)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0481_BUSERRORL_ENTRY, this );

	// This test case handles no failiures on the bus
	TestFailed(aError);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0481_BUSERRORL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0481::HostRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0481_HOSTRUNL_ENTRY, this );
	
	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{
		// Action timeout
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0481_HOSTRUNL, "<Error> Action timeout");
		TestFailed(KErrTimedOut);
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0481_HOSTRUNL_DUP01, "<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0481_HOSTRUNL_EXIT, this );
	}

/**
Called when the device has reported any kind of error in its opertaion
or when the device has been informed by the host to report success
*/
void CUT_PBASE_T_USBDI_0481::DeviceRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0481_DEVICERUNL_ENTRY, this );
	// Disconnect the device	
	iTestDevice->SoftwareDisconnect();	
	// Complete the test case request	
	TestPolicy().SignalTestComplete(iStatus.Int());
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0481_DEVICERUNL_EXIT, this );
	}

	
	}
