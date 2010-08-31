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
// @file PBASE-T_USBDI-0472.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0472.h"
#include <e32test.h>
#include <d32usbc.h>
#include <e32debug.h>
#include "UsbDescriptorOffsets.h"
#include "TestPolicy.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0472Traces.h"
#endif

namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0472");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0472,TBool> CUT_PBASE_T_USBDI_0472::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0472* CUT_PBASE_T_USBDI_0472::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0472_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0472* self = new (ELeave) CUT_PBASE_T_USBDI_0472(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0472_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0472::CUT_PBASE_T_USBDI_0472(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0472_CUT_PBASE_T_USBDI_0472_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0472_CUT_PBASE_T_USBDI_0472_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0472::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0472_CONSTRUCTL_ENTRY, this );
	iTestDevice = new RUsbDeviceVendor(this);
	BaseConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0472_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0472::~CUT_PBASE_T_USBDI_0472()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0472_CUT_PBASE_T_USBDI_0472_ENTRY_DUP01, this );
	Cancel();

	delete iClientAction;
	delete iActorFDF;
	if(!IsHost() && iTestDevice)
		{
		iTestDevice->Close();
		}		
	delete iTestDevice;
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0472_CUT_PBASE_T_USBDI_0472_EXIT_DUP01, this );
	}


void CUT_PBASE_T_USBDI_0472::ExecuteHostTestCaseL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0472_EXECUTEHOSTTESTCASEL_ENTRY, this );
	iActorFDF = CActorFDF::NewL(*this);
	iClientAction = new (ELeave) CEp0Transfer(iInterface0);
	iCaseStep = EConnectDevice;
	iActorFDF->Monitor();
	TimeoutIn(30);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0472_EXECUTEHOSTTESTCASEL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0472::ExecuteDeviceTestCaseL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0472_EXECUTEDEVICETESTCASEL_ENTRY, this );
	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);	
	SetActive();
	iTestDevice->SoftwareConnect();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0472_EXECUTEDEVICETESTCASEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0472::HostDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0472_HOSTDOCANCEL_ENTRY, this );
	
	// Cancel the test step action timeout timer
	
	CancelTimeout();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0472_HOSTDOCANCEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0472::DeviceDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0472_DEVICEDOCANCEL_ENTRY, this );
	
	// Cancel the test device
	
	iTestDevice->CancelSubscriptionToReports();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0472_DEVICEDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0472::DeviceInsertedL(TUint aDeviceHandle)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_ENTRY, this );

	Cancel();
	TInt err(KErrNone);
	
	// Validate that device is as expected
	
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case	

		OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL, "<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,testDevice.SerialNumber(),TestCaseId());

		// Start the connection timeout again

		TimeoutIn(30);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_EXIT, this );
		return;
		}	
		
	// Perform the correct test step
		
	switch(iCaseStep)
		{
		case EConnectDevice:
			{
			// Validate vendor identity
			OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_DUP01, "Vendor identity: 0x%04x",testDevice.VendorId());
			if(testDevice.VendorId() != 0x0E22)
				{
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_DUP02, "<Error> Vendor identity is not 0x0E22");
				return TestFailed(KErrCorrupt);
				}
				
			// Validate product information
			OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_DUP03, "Usb device supported specification: 0x%04x",testDevice.DeviceSpec());
			if(testDevice.DeviceSpec() != 0x0200)
				{
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_DUP04, "<Error> Usb device supported specification is not 2.0");
				return TestFailed(KErrCorrupt);
				}
			OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_DUP05, "Usb product identity: 0x%04x",testDevice.ProductId());
			if(testDevice.ProductId() != 0x0040)
				{
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_DUP06, "<Error> Usb product idenity is not 0x0040");
				return TestFailed(KErrCorrupt);
				}
		
			OstTraceExt1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_DUP07, "testDevice.ConfigurationString() = %S",testDevice.ConfigurationString());		
			gtest(KErrNone == testDevice.ConfigurationString().Compare(KConfigurationString()));		
					
			OstTraceExt1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_DUP08, "testDevice.Manufacturer() = %S",testDevice.Manufacturer());
			gtest(KErrNone == testDevice.Manufacturer().Compare(KManufacturer()));
			
			OstTraceExt1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_DUP09, "testDevice.SerialNumber = %s",testDevice.SerialNumber());	
			gtest(KErrNone == testDevice.SerialNumber().Compare(KTestCaseId()));
		
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_DUP10, "Waiting for device removal");
			iCaseStep = ERemoveDevice;
			
			TUint32 token;
			err = testDevice.Device().GetTokenForInterface(0,token);
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_DUP11, "<Error %d> Unable to get token for interface 0",err);
				return TestFailed(err);
				}
			err = iInterface0.Open(token);
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_DUP12, "<Error %d> Unable to open interface 0",err);
				return TestFailed(err);
				}
			
			// Send request to client to reconnect in 3 seconds
			
			TReconnectRequest request(3);
			iClientAction->SendRequest(request,this);
			
			// Monitor for the reconnection from the client
			
			iActorFDF->Monitor();
			TimeoutIn(30);
			}
			break;
			
		case EConnectCancelled:
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_DUP13, "<Error> Bus event cancellation not successful");
			TestFailed(KErrCorrupt);
			break;
			
		case EPassed:	// Test case has been successfully concluded
			{
			TUint32 token;
			err = testDevice.Device().GetTokenForInterface(0,token);
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_DUP14, "<Error %d> Unable to get token for interface 0",err);
				return TestFailed(err);
				}
			err = iInterface0.Open(token);
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_DUP15, "<Error %d> Unable to open interface 0",err);
				return TestFailed(err);
				}
	
			// Send test case passed request to client
			TTestCasePassed request;
			iClientAction->SendRequest(request,this);
			}
			break;
			
		default:
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_DUP16, "<Error> Test case actions out of sync");
			TestFailed(KErrCorrupt);
			break;
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0472_DEVICEINSERTEDL_EXIT_DUP01, this );
	}


void CUT_PBASE_T_USBDI_0472::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0472_EP0TRANSFERCOMPLETEL_ENTRY, this );
	
	switch(iCaseStep)
		{
		case EPassed:
			{
			TestPassed();
			}
			break;
			
		case EFailed:
			{
			TestFailed(KErrCompletion);
			}
			break;
	
		case ERemoveDevice:
			{
			// Doing nothing, client should be re-connecting
			}
			break;
			
		default:
			OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_EP0TRANSFERCOMPLETEL, "<Error %d> Unknown test step",KErrCorrupt);
			TestFailed(KErrCorrupt);
			break;
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0472_EP0TRANSFERCOMPLETEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0472::DeviceRemovedL(TUint aDeviceHandle)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0472_DEVICEREMOVEDL_ENTRY, this );

	Cancel();
	
	switch(iCaseStep)
		{
		case ERemoveDevice:
			{
			iCaseStep = EConnectCancelled;
		
			// start timer now
			TimeoutIn(5);		
			}
			break;
			
		default:
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_DEVICEREMOVEDL, "<Error> Test case actions out of sync");
			TestFailed(KErrCorrupt);
			break;
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0472_DEVICEREMOVEDL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0472::BusErrorL(TInt aError)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0472_BUSERRORL_ENTRY, this );
	// This test case handles no failiures on the bus

	TestFailed(aError);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0472_BUSERRORL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0472::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
	RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0472_DEVICESTATECHANGEL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0472_DEVICESTATECHANGEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0472::HostRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0472_HOSTRUNL_ENTRY, this );
	
	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{		
		if(iCaseStep == EConnectCancelled)
			{
			// not a time-out 
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_HOSTRUNL, "Timer elapsed, reactivating notifications now");
			iCaseStep = EPassed;			
			iActorFDF->Monitor();
			}
		else
			{
			// Action timeout
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_HOSTRUNL_DUP01, "<Error> Action timeout");
			TestFailed(KErrTimedOut);
			}	
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0472_HOSTRUNL_DUP02, "<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0472_HOSTRUNL_EXIT, this );
	}
	

void CUT_PBASE_T_USBDI_0472::DeviceRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0472_DEVICERUNL_ENTRY, this );
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0472_DEVICERUNL_EXIT, this );
	}	
	

	}
	
