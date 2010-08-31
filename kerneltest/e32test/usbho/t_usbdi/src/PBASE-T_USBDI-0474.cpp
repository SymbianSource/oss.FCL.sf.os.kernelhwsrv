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
// @file PBASE-T_USBDI-0474.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0474.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0474Traces.h"
#endif


namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0474");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0474,TBool> CUT_PBASE_T_USBDI_0474::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0474* CUT_PBASE_T_USBDI_0474::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0474_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0474* self = new (ELeave) CUT_PBASE_T_USBDI_0474(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0474_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0474::CUT_PBASE_T_USBDI_0474(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0474_CUT_PBASE_T_USBDI_0474_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0474_CUT_PBASE_T_USBDI_0474_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0474::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0474_CONSTRUCTL_ENTRY, this );
	iTestDevice = new RUsbDeviceB(this);
	BaseConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0474_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0474::~CUT_PBASE_T_USBDI_0474()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0474_CUT_PBASE_T_USBDI_0474_ENTRY_DUP01, this );
	Cancel();
	
	iUsbInterface0.Close();

	delete iClientAction;
	delete iActorFDF;
	if(!IsHost() && iTestDevice)
		{
		iTestDevice->Close();
		}		
	delete iTestDevice;
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0474_CUT_PBASE_T_USBDI_0474_EXIT_DUP01, this );
	}
	
	
void CUT_PBASE_T_USBDI_0474::ExecuteHostTestCaseL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0474_EXECUTEHOSTTESTCASEL_ENTRY, this );
	
	iActorFDF = CActorFDF::NewL(*this);
	iClientAction = new (ELeave) CEp0Transfer(iUsbInterface0);
	iActorFDF->Monitor();
	TimeoutIn(30);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0474_EXECUTEHOSTTESTCASEL_EXIT, this );
	}

	
void CUT_PBASE_T_USBDI_0474::ExecuteDeviceTestCaseL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0474_EXECUTEDEVICETESTCASEL_ENTRY, this );
	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	iTestDevice->SoftwareConnect();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0474_EXECUTEDEVICETESTCASEL_EXIT, this );
	} 
	
	
void CUT_PBASE_T_USBDI_0474::HostDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0474_HOSTDOCANCEL_ENTRY, this );
	
	// Cancel the test step timeout
	
	CancelTimeout();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0474_HOSTDOCANCEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0474::DeviceDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0474_DEVICEDOCANCEL_ENTRY, this );
	
	// Cancel the device
	
	iTestDevice->CancelSubscriptionToReports();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0474_DEVICEDOCANCEL_EXIT, this );
	}
			
void CUT_PBASE_T_USBDI_0474::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0474_DEVICEINSERTEDL_ENTRY, this );
	Cancel();
	TInt err(KErrNone); 
	
	// Validate that device is as expected
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case	

		OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_0474_DEVICEINSERTEDL, "<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,testDevice.SerialNumber(),TestCaseId());

		// Start the connection timeout again

		TimeoutIn(30);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0474_DEVICEINSERTEDL_EXIT, this );
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
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0474_DEVICEINSERTEDL_DUP01, "<Error %d> Token for interface 0 could not be retrieved",err);
		return TestFailed(err);
		}
	err = iUsbInterface0.Open(token0); // Default interface setting 0
	if(err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0474_DEVICEINSERTEDL_DUP02, "<Error %d> Unable to open interface 1 using token %d",err,token0);
		return TestFailed(err);
		} 
	 	
	iCaseStep = EPassed;
	TTestCasePassed request;
	iClientAction->SendRequest(request,this);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0474_DEVICEINSERTEDL_EXIT_DUP01, this );
	}
	
	
void CUT_PBASE_T_USBDI_0474::DeviceRemovedL(TUint aDeviceHandle)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0474_DEVICEREMOVEDL_ENTRY, this );

	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error

	TestFailed(KErrDisconnected);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0474_DEVICEREMOVEDL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0474::BusErrorL(TInt aError)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0474_BUSERRORL_ENTRY, this );

	// This test case handles no failiures on the bus
	
	TestFailed(aError);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0474_BUSERRORL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0474::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
	RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0474_DEVICESTATECHANGEL_ENTRY, this );
	Cancel();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0474_DEVICESTATECHANGEL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0474::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0474_EP0TRANSFERCOMPLETEL_ENTRY, this );
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0474_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(iCaseStep == EPassed)
		{
		TestPassed();
		}
		
	if(iCaseStep == EFailed)
		{
		TestFailed(KErrCompletion);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0474_EP0TRANSFERCOMPLETEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0474::HostRunL()
	{
    OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0474_HOSTRUNL_ENTRY, this );

	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{
		// Action timeout
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0474_HOSTRUNL, "<Error> Action timeout");
		TestFailed(KErrTimedOut);
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0474_HOSTRUNL_DUP01, "<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0474_HOSTRUNL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0474::DeviceRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0474_DEVICERUNL_ENTRY, this );
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0474_DEVICERUNL_EXIT, this );
	}
	
	
	}

