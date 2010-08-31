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
// @file PBASE-T_USBDI-0479.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0479.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0479Traces.h"
#endif


namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0479");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0479,TBool> CUT_PBASE_T_USBDI_0479::iFunctor(KTestCaseId);

CUT_PBASE_T_USBDI_0479* CUT_PBASE_T_USBDI_0479::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0479_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0479* self = new (ELeave) CUT_PBASE_T_USBDI_0479(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0479_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0479::CUT_PBASE_T_USBDI_0479(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0479_CUT_PBASE_T_USBDI_0479_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0479_CUT_PBASE_T_USBDI_0479_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0479::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0479_CONSTRUCTL_ENTRY, this );
	iTestDevice = new RUsbDeviceVendor(this);
	BaseConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0479_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0479::~CUT_PBASE_T_USBDI_0479()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0479_CUT_PBASE_T_USBDI_0479_ENTRY_DUP01, this );
	
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0479_CUT_PBASE_T_USBDI_0479_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0479::ExecuteHostTestCaseL()	
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0479_EXECUTEHOSTTESTCASEL_ENTRY, this );
	
	iActorFDF = CActorFDF::NewL(*this);
	iClientAction = new (ELeave) CEp0Transfer(iUsbInterface0);
	iActorFDF->Monitor();
	TimeoutIn(30);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0479_EXECUTEHOSTTESTCASEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0479::HostDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0479_HOSTDOCANCEL_ENTRY, this );
	
	// Cancel timing out the test step
	
	CancelTimeout();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0479_HOSTDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0479::ExecuteDeviceTestCaseL()	
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0479_EXECUTEDEVICETESTCASEL_ENTRY, this );
	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	iTestDevice->SoftwareConnect();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0479_EXECUTEDEVICETESTCASEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0479::DeviceDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0479_DEVICEDOCANCEL_ENTRY, this );
	
	// Cancel the device	
	iTestDevice->CancelSubscriptionToReports();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0479_DEVICEDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0479::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
	RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0479_DEVICESTATECHANGEL_ENTRY, this );
	Cancel();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0479_DEVICESTATECHANGEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0479::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0479_DEVICEINSERTEDL_ENTRY, this );
	Cancel();
	TInt err(KErrNone);
	
	// Validate that device is as expected
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case	

		OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_0479_DEVICEINSERTEDL, "<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,testDevice.SerialNumber(),TestCaseId());

		// Start the connection timeout again

		TimeoutIn(30);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0479_DEVICEINSERTEDL_EXIT, this );
		return;
		}

	TUint32 validToken;
	TUint32 invalidToken(KMaxTUint-1); // The known invalid token
	
	err = testDevice.Device().GetTokenForInterface(0,validToken);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0479_DEVICEINSERTEDL_DUP01, "<Error %d> Token for interface could not be retrieved",err);
		return TestFailed(err);
		}
	err = iUsbInterface0.Open(validToken); // Default interface setting 0
	if(err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0479_DEVICEINSERTEDL_DUP02, "<Error %d> Unable to open interface 0 using token %d",err,validToken);
		return TestFailed(err);
		}
	
	err = iDuplicateUsbInterface0.Open(invalidToken);
	if(err == KErrNone)
		{
		TBuf<64> msg;
		msg.Format(_L("<Error %d> Able to open an interface that is not present"),KErrCorrupt);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0479_DEVICEINSERTEDL_DUP03, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(KErrCorrupt,msg);
		iClientAction->SendRequest(request,this);
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0479_DEVICEINSERTEDL_DUP04, "Opening interface with invalid token failed with %d",err);
		User::After(1000000);
		iCaseStep = EPassed;
		TTestCasePassed request;
		iClientAction->SendRequest(request,this);	
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0479_DEVICEINSERTEDL_EXIT_DUP01, this );
	}
	
	
void CUT_PBASE_T_USBDI_0479::DeviceRemovedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0479_DEVICEREMOVEDL_ENTRY, this );
	
	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error

	TestFailed(KErrDisconnected);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0479_DEVICEREMOVEDL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0479::BusErrorL(TInt aError)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0479_BUSERRORL_ENTRY, this );
	
	// This test case handles no failiures on the bus
	
	TestFailed(aError);	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0479_BUSERRORL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0479::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0479_EP0TRANSFERCOMPLETEL_ENTRY, this );
	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0479_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{	
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0479_EP0TRANSFERCOMPLETEL_DUP01, msg);
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0479_EP0TRANSFERCOMPLETEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0479::HostRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0479_HOSTRUNL_ENTRY, this );
	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{
		// Action timeout
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0479_HOSTRUNL, "<Error> Action timeout");
		TestFailed(KErrTimedOut);
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0479_HOSTRUNL_DUP01, "<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0479_HOSTRUNL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0479::DeviceRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0479_DEVICERUNL_ENTRY, this );
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0479_DEVICERUNL_EXIT, this );
	}

	
	}
