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
// @file PBASE-T_USBDI-0483.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0483.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0483Traces.h"
#endif


namespace NUnitTesting_USBDI
	{
const TInt KGetConfigDescriptor = 0x0200;	
_LIT(KTestCaseId,"PBASE-T_USBDI-0483");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0483,TBool> CUT_PBASE_T_USBDI_0483::iFunctor(KTestCaseId);

CUT_PBASE_T_USBDI_0483* CUT_PBASE_T_USBDI_0483::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0483_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0483* self = new (ELeave) CUT_PBASE_T_USBDI_0483(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0483_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0483::CUT_PBASE_T_USBDI_0483(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0483_CUT_PBASE_T_USBDI_0483_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0483_CUT_PBASE_T_USBDI_0483_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0483::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0483_CONSTRUCTL_ENTRY, this );
	iTestDevice = new RUsbDeviceB(this);
	BaseConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0483_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0483::~CUT_PBASE_T_USBDI_0483()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0483_CUT_PBASE_T_USBDI_0483_ENTRY_DUP01, this );
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0483_CUT_PBASE_T_USBDI_0483_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0483::ExecuteHostTestCaseL()	
	{
OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0483_EXECUTEHOSTTESTCASEL_ENTRY, this );

	// Create the actor for the Function Driver Framework 
	
	iActorFDF = CActorFDF::NewL(*this);

	// Create the control transfer for requests

	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	
	// Monitor for devices

	iActorFDF->Monitor();
	
	// Start the connection timeout timer

	TimeoutIn(30);

	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0483_EXECUTEHOSTTESTCASEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0483::HostDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0483_HOSTDOCANCEL_ENTRY, this );
	
	// Cancel the test step timeout timer
	
	CancelTimeout();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0483_HOSTDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0483::ExecuteDeviceTestCaseL()	
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0483_EXECUTEDEVICETESTCASEL_ENTRY, this );
	
	// Create the test device
	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	
	// Connect the device to the host
	
	iTestDevice->SoftwareConnect();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0483_EXECUTEDEVICETESTCASEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0483::DeviceDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0483_DEVICEDOCANCEL_ENTRY, this );
	
	// Cancel the reporting or errors from opertaions perfomed by the test device
	
	iTestDevice->CancelSubscriptionToReports();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0483_DEVICEDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0483::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
		RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0483_DEVICESTATECHANGEL_ENTRY, this );
	
	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0483_DEVICESTATECHANGEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0483::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0483_EP0TRANSFERCOMPLETEL_ENTRY, this );
	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0483_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{
		if(iCaseStep == EFailed)
			{// todo, cope with errors
			}
		else
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0483_EP0TRANSFERCOMPLETEL_DUP01, msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(aCompletionCode,msg);
			iControlEp0->SendRequest(request,this);
			OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0483_EP0TRANSFERCOMPLETEL_EXIT, this );
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
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0483_EP0TRANSFERCOMPLETEL_DUP02, "<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0483_EP0TRANSFERCOMPLETEL_EXIT_DUP01, this );
	}	
void CUT_PBASE_T_USBDI_0483::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0483_DEVICEINSERTEDL_ENTRY, this );
	Cancel();
	
	TInt err(KErrNone);
	
	// Validate that device is as expected
	
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case	

		OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_0483_DEVICEINSERTEDL, "<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,testDevice.SerialNumber(),TestCaseId());

		// Start the connection timeout again

		TimeoutIn(30);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0483_DEVICEINSERTEDL_EXIT, this );
		return;
		}	
		
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0483_DEVICEINSERTEDL_DUP01, "iControlEp0->SendRequest(getConfiguration,this)");
	TUint32 token0;
	err = testDevice.Device().GetTokenForInterface(0,token0);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0483_DEVICEINSERTEDL_DUP02, "<Error %d> Token for interface 0 could not be retrieved",err);
		return TestFailed(err);
		}
	err = iUsbInterface0.Open(token0); // Default interface setting 0
	if(err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0483_DEVICEINSERTEDL_DUP03, "<Error %d> Unable to open interface 1 using token %d",err,token0);
		return TestFailed(err);
		} 
	
	// get config. descriptor now
	TUint totalLength = testDevice.ConfigurationDescriptor().TotalLength();
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0483_DEVICEINSERTEDL_DUP04, "totalLength ==== %d",totalLength);
	iConfigDescriptorData = HBufC8::NewL(totalLength); 		
	TPtr8 des(iConfigDescriptorData->Des());
	des.SetLength(totalLength);		
	TDescriptorGetRequest getConfiguration(KGetConfigDescriptor,0,des);
	iControlEp0->SendRequest(getConfiguration,this);	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0483_DEVICEINSERTEDL_EXIT_DUP01, this );
	}	 

	
void CUT_PBASE_T_USBDI_0483::DeviceRemovedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0483_DEVICEREMOVEDL_ENTRY, this );
	
	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error
	
	TestFailed(KErrDisconnected);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0483_DEVICEREMOVEDL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0483::BusErrorL(TInt aError)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0483_BUSERRORL_ENTRY, this );
	
	// This test case handles no failiures on the bus
	
	TestFailed(KErrCompletion);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0483_BUSERRORL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0483::HostRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0483_HOSTRUNL_ENTRY, this );
	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{
		// Action timeout
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0483_HOSTRUNL, "<Error> Action timeout");
		TestFailed(KErrTimedOut);
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0483_HOSTRUNL_DUP01, "<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0483_HOSTRUNL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0483::DeviceRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0483_DEVICERUNL_ENTRY, this );
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0483_DEVICERUNL_EXIT, this );
	}

	
	}
