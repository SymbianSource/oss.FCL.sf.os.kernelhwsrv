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
// @file PBASE-T_USBDI-0478.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0478.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0478Traces.h"
#endif


namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0478");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0478,TBool> CUT_PBASE_T_USBDI_0478::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0478* CUT_PBASE_T_USBDI_0478::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0478_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0478* self = new (ELeave) CUT_PBASE_T_USBDI_0478(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0478_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0478::CUT_PBASE_T_USBDI_0478(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0478_CUT_PBASE_T_USBDI_0478_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0478_CUT_PBASE_T_USBDI_0478_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0478::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0478_CONSTRUCTL_ENTRY, this );
	iTestDevice = new RUsbDeviceA(this);
	BaseConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0478_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0478::~CUT_PBASE_T_USBDI_0478()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0478_CUT_PBASE_T_USBDI_0478_ENTRY_DUP01, this );
	
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0478_CUT_PBASE_T_USBDI_0478_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0478::ExecuteHostTestCaseL()	
	{
    OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0478_EXECUTEHOSTTESTCASEL_ENTRY, this );

	// Create the actor for the Function Driver Framework 
	
	iActorFDF = CActorFDF::NewL(*this);

	// Create the control transfer for requests

	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	
	// Monitor for devices

	iActorFDF->Monitor();
	
	// Start the connection timeout timer

	TimeoutIn(30);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0478_EXECUTEHOSTTESTCASEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0478::HostDoCancel()
	{
    OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0478_HOSTDOCANCEL_ENTRY, this );

	// Cancel the timeout timer for activity

	CancelTimeout();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0478_HOSTDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0478::ExecuteDeviceTestCaseL()	
	{
    OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0478_EXECUTEDEVICETESTCASEL_ENTRY, this );

	// Create the test device for this test case

	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();

	// Connect the test device to the host

	iTestDevice->SoftwareConnect();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0478_EXECUTEDEVICETESTCASEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0478::DeviceDoCancel()
	{
    OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0478_DEVICEDOCANCEL_ENTRY, this );

	// Cancel the test device
	
	iTestDevice->CancelSubscriptionToReports();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0478_DEVICEDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0478::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
	RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0478_DEVICESTATECHANGEL_ENTRY, this );

	Cancel();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0478_DEVICESTATECHANGEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0478::DeviceInsertedL(TUint aDeviceHandle)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0478_DEVICEINSERTEDL_ENTRY, this );

	Cancel();
	TInt err(KErrNone);
	
	// Validate that device is as expected
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case	

		OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_0478_DEVICEINSERTEDL, "<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,testDevice.SerialNumber(),TestCaseId());

		// Start the connection timeout again

		TimeoutIn(30);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0478_DEVICEINSERTEDL_EXIT, this );
		return;
		}
	
	TUint32 token0,token1;
	err = testDevice.Device().GetTokenForInterface(0,token0);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0478_DEVICEINSERTEDL_DUP01, "<Error %d> Token for interface 0 could not be retrieved",err);
		return TestFailed(err);
		}
	err = iUsbInterface0.Open(token0); // Default interface setting 0
	if(err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0478_DEVICEINSERTEDL_DUP02, "<Error %d> Unable to open interface 0 using token %d",err,token0);
		return TestFailed(err);
		}

	err = testDevice.Device().GetTokenForInterface(1,token1);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0478_DEVICEINSERTEDL_DUP03, "<Error %d> Token for interface 1 could not be retrieved",err);
		return TestFailed(err);
		}

	err = iUsbInterface1.Open(token1);
	if(err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0478_DEVICEINSERTEDL_DUP04, "<Error %d> Unable to open interface using token %d",err,token1);
		return TestFailed(err);
		}

	// Acting as another FD open the interface
	
	err = iDuplicateUsbInterface1.Open(token1);
	if(err == KErrNone)
		{
		TBuf<64> msg;
		msg.Format(_L("<Error> Able to open a concurrent hande to an interface"));
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0478_DEVICEINSERTEDL_DUP05, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(KErrCompletion,msg);
		iControlEp0->SendRequest(request,this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0478_DEVICEINSERTEDL_EXIT_DUP01, this );
		return;
		}
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0478_DEVICEINSERTEDL_DUP06, "Attempt to open concurrent interface handle failed with: %d",err);
	
	// Inform client device test case successful 
	User::After(1000000);
	iCaseStep = EPassed;
	TTestCasePassed request;
	iControlEp0->SendRequest(request,this);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0478_DEVICEINSERTEDL_EXIT_DUP02, this );
	}
	
	
void CUT_PBASE_T_USBDI_0478::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0478_EP0TRANSFERCOMPLETEL_ENTRY, this );
	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0478_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{	
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0478_EP0TRANSFERCOMPLETEL_DUP01, msg);
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0478_EP0TRANSFERCOMPLETEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0478::DeviceRemovedL(TUint aDeviceHandle)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0478_DEVICEREMOVEDL_ENTRY, this );

	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error

	TestFailed(KErrDisconnected);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0478_DEVICEREMOVEDL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0478::BusErrorL(TInt aError)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0478_BUSERRORL_ENTRY, this );

	// This test case handles no failiures on the bus
	
	TestFailed(aError);	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0478_BUSERRORL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0478::HostRunL()
	{
    OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0478_HOSTRUNL_ENTRY, this );

	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{
		// Action timeout
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0478_HOSTRUNL, "<Error> Action timeout");
		TestFailed(KErrTimedOut);
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0478_HOSTRUNL_DUP01, "<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0478_HOSTRUNL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0478::DeviceRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0478_DEVICERUNL_ENTRY, this );
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0478_DEVICERUNL_EXIT, this );
	}

	
	}
