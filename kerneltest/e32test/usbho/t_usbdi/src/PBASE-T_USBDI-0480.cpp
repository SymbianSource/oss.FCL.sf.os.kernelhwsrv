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
// @file PBASE-T_USBDI-0480.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0480.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0480Traces.h"
#endif

	
namespace NUnitTesting_USBDI
	{
	
_LIT8(KDataPayload,"DEADBEEF");
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0480");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0480,TBool> CUT_PBASE_T_USBDI_0480::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0480* CUT_PBASE_T_USBDI_0480::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0480_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0480* self = new (ELeave) CUT_PBASE_T_USBDI_0480(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0480::CUT_PBASE_T_USBDI_0480(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iPtrTemp(NULL,0)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0480_CUT_PBASE_T_USBDI_0480_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_CUT_PBASE_T_USBDI_0480_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0480::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0480_CONSTRUCTL_ENTRY, this );
	iTestDevice = new RUsbDeviceA(this);
	BaseConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0480::~CUT_PBASE_T_USBDI_0480()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0480_CUT_PBASE_T_USBDI_0480_ENTRY_DUP01, this );
	
	Cancel();
	
	// Close pipe before interface	
	iUsbInterface1.Close();
	iUsbInterface0.Close();
	
	delete iTemp;
	delete iControlEp0;
	delete iActorFDF;
	if(!IsHost() && iTestDevice)
		{
		iTestDevice->Close();
		}		
	delete iTestDevice;
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_CUT_PBASE_T_USBDI_0480_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0480::ExecuteHostTestCaseL()	
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0480_EXECUTEHOSTTESTCASEL_ENTRY, this );
	
	iActorFDF = CActorFDF::NewL(*this);
	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	iActorFDF->Monitor();
	
	TimeoutIn(30);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_EXECUTEHOSTTESTCASEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0480::HostDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0480_HOSTDOCANCEL_ENTRY, this );
	
	CancelTimeout();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_HOSTDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0480::ExecuteDeviceTestCaseL()	
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0480_EXECUTEDEVICETESTCASEL_ENTRY, this );
	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	iTestDevice->SoftwareConnect();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_EXECUTEDEVICETESTCASEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0480::DeviceDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0480_DEVICEDOCANCEL_ENTRY, this );
	
	// Cancel the device	
	iTestDevice->CancelSubscriptionToReports();	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_DEVICEDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0480::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
	RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0480_DEVICESTATECHANGEL_ENTRY, this );
	
	Cancel();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_DEVICESTATECHANGEL_EXIT, this );
	}
	

void CUT_PBASE_T_USBDI_0480::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0480_DEVICEINSERTEDL_ENTRY, this );
	
	Cancel();
	TInt err(KErrNone);
	
	// Validate that device is as expected
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case	

		OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_0480_DEVICEINSERTEDL, "<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,testDevice.SerialNumber(),TestCaseId());

		// Start the connection timeout again
		TimeoutIn(30);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_DEVICEINSERTEDL_EXIT, this );
		return;
		}

	TUint32 token0,token1;
	
	err = testDevice.Device().GetTokenForInterface(0,token0);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0480_DEVICEINSERTEDL_DUP01, "<Error %d> Token for interface 0 could not be retrieved",err);
		return TestFailed(err);
		}
	err = iUsbInterface0.Open(token0); // Default interface setting 0
	if(err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0480_DEVICEINSERTEDL_DUP02, "<Error %d> Unable to open interface 0 using token %d",err,token0);
		return TestFailed(err);
		}
	
	err = testDevice.Device().GetTokenForInterface(1,token1);
	if(err != KErrNone)
		{
		TBuf<64> msg;
		msg.Format(_L("<Error %d> Token for interface 1 could not be retrieved"),err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0480_DEVICEINSERTEDL_DUP03, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_DEVICEINSERTEDL_EXIT_DUP01, this );
		return;
		}
	err = iUsbInterface1.Open(token1);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to open interface 1 using token %d"),err,token1);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0480_DEVICEINSERTEDL_DUP04, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_DEVICEINSERTEDL_EXIT_DUP02, this );
		return;
		}
	
	// Get the endpoint descriptor
	TUsbEndpointDescriptor endpointDescriptor;

	err = iUsbInterface1.GetEndpointDescriptor(0,1,endpointDescriptor);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Descriptor for endpoint 0 cannot be obtained"),err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0480_DEVICEINSERTEDL_DUP05, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_DEVICEINSERTEDL_EXIT_DUP03, this );
		return;
		}
	TUint16 maxPacketSize(endpointDescriptor.MaxPacketSize());
	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0480_DEVICEINSERTEDL_DUP06, "Maximum packet size for endpoint 1 on interface 1 setting 0 is: %d",maxPacketSize);
	
	// Perform a device directed control transfer
	User::After(1000000);	
	iCaseStep = EEmptyDeviceXfer;
	TEmptyDeviceRequest request;
	iControlEp0->SendRequest(request,this);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_DEVICEINSERTEDL_EXIT_DUP04, this );
	}


void CUT_PBASE_T_USBDI_0480::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0480_EP0TRANSFERCOMPLETEL_ENTRY, this );
	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0480_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{
		if(iCaseStep == EFailed)
			{
			}
		else
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0480_EP0TRANSFERCOMPLETEL_DUP01, msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(aCompletionCode,msg);
			iControlEp0->SendRequest(request,this);
			OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_EP0TRANSFERCOMPLETEL_EXIT, this );
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
			
		// Send an empty interface directed request		
		case EEmptyDeviceXfer:
			{
			iCaseStep = EEmptyInterfaceXfer;
			TEmptyInterfaceRequest request(1); // Direct at interface 1
			iControlEp0->SendRequest(request,this);
			}
			break;	
		
		// Send a test payload request to device directed 		
		case EEmptyInterfaceXfer:
			{
			iCaseStep = EDataPutDeviceXfer;
			TDevicePutPayloadRequest request(KDataPayload);
			iControlEp0->SendRequest(request,this);
			}
			break;
			
		// Send a test payload request to interface directed		
		case EDataPutDeviceXfer:
			{
			iCaseStep = EDataPutInterfaceXfer;
			TInterfacePutPayloadRequest request(1,KDataPayload); // Direct at interface 1
			iControlEp0->SendRequest(request,this);
			}
			break;
			
		// Test case passed		
		case EDataPutInterfaceXfer:
			{
			iCaseStep = EPassed;
			User::After(1000000);
			TTestCasePassed request;
			iControlEp0->SendRequest(request,this);			
			}
			break;
			
		default:
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0480_EP0TRANSFERCOMPLETEL_DUP02, "<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_EP0TRANSFERCOMPLETEL_EXIT_DUP01, this );
	}
	
	
void CUT_PBASE_T_USBDI_0480::DeviceRemovedL(TUint aDeviceHandle)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0480_DEVICEREMOVEDL_ENTRY, this );

	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error
	TestFailed(KErrDisconnected);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_DEVICEREMOVEDL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0480::BusErrorL(TInt aError)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0480_BUSERRORL_ENTRY, this );

	// This test case handles no failiures on the bus	
	TestFailed(KErrGeneral);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_BUSERRORL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0480::HostRunL()
	{
    OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0480_HOSTRUNL_ENTRY, this );

	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{
		// Action timeout
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0480_HOSTRUNL, "<Error> Action timeout");
		TestFailed(KErrTimedOut);
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0480_HOSTRUNL_DUP01, "<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_HOSTRUNL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0480::DeviceRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0480_DEVICERUNL_ENTRY, this );
	
	// Disconnect the device	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request	
	TestPolicy().SignalTestComplete(iStatus.Int());
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0480_DEVICERUNL_EXIT, this );
	}

	
	}
