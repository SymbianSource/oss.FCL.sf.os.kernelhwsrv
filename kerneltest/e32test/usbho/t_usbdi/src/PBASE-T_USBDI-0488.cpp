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
// @file PBASE-T_USBDI-0488.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0488.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0488Traces.h"
#endif
#include <d32usbdi_errors.h>

namespace NUnitTesting_USBDI
	{

_LIT(KTestCaseId,"PBASE-T_USBDI-0488");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0488,TBool> CUT_PBASE_T_USBDI_0488::iFunctor(KTestCaseId);

CUT_PBASE_T_USBDI_0488* CUT_PBASE_T_USBDI_0488::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0488_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0488* self = new (ELeave) CUT_PBASE_T_USBDI_0488(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0488_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0488::CUT_PBASE_T_USBDI_0488(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0488_CUT_PBASE_T_USBDI_0488_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0488_CUT_PBASE_T_USBDI_0488_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0488::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0488_CONSTRUCTL_ENTRY, this );
	iTestDevice = new RUsbDeviceA(this);
	BaseConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0488_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0488::~CUT_PBASE_T_USBDI_0488()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0488_CUT_PBASE_T_USBDI_0488_ENTRY_DUP01, this );
	
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0488_CUT_PBASE_T_USBDI_0488_EXIT_DUP01, this );
	}
	
	
void CUT_PBASE_T_USBDI_0488::ExecuteHostTestCaseL()	
	{
		OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0488_EXECUTEHOSTTESTCASEL_ENTRY, this );
		
	iActorFDF = CActorFDF::NewL(*this);
	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	iActorFDF->Monitor();
	
	TimeoutIn(30);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0488_EXECUTEHOSTTESTCASEL_EXIT, this );
	}
	
/*static*/ TInt CUT_PBASE_T_USBDI_0488::TestSelectAlternateInterfaceThenPanic(TAny* aTest) 
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0488_TESTSELECTALTERNATEINTERFACETHENPANIC_ENTRY, 0 );
	
	CTrapCleanup* trapHandler=CTrapCleanup::New();
	if(!trapHandler)
		{
		OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0488_TESTSELECTALTERNATEINTERFACETHENPANIC_EXIT, 0, KErrNoMemory );
		return KErrNoMemory;
		}		
						
	CUT_PBASE_T_USBDI_0488* pTest = reinterpret_cast<CUT_PBASE_T_USBDI_0488*>(aTest);
	
	// change interface setting whilst pipes are opened in another alt. setting, should panic
	pTest->iUsbInterface1.SelectAlternateInterface(0); // panic expected
	
	// Should never get here.*/
	OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0488_TESTSELECTALTERNATEINTERFACETHENPANIC_EXIT_DUP01, 0, KErrNone );
	return KErrNone;
	}
	
	
	
void CUT_PBASE_T_USBDI_0488::HostDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0488_HOSTDOCANCEL_ENTRY, this );
	// Cancel the timeout timer	
	CancelTimeout();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0488_HOSTDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0488::ExecuteDeviceTestCaseL()	
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0488_EXECUTEDEVICETESTCASEL_ENTRY, this );
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	iTestDevice->SoftwareConnect();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0488_EXECUTEDEVICETESTCASEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0488::DeviceDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0488_DEVICEDOCANCEL_ENTRY, this );
	// Cancel the device	
	iTestDevice->CancelSubscriptionToReports();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0488_DEVICEDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0488::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
		TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0488_DEVICESTATECHANGEL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0488_DEVICESTATECHANGEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0488::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0488_DEVICEINSERTEDL_ENTRY, this );
	Cancel();
	TInt err(KErrNone);
	
	// Validate that device is as expected
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case
		OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_0488_DEVICEINSERTEDL, "<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,testDevice.SerialNumber(),TestCaseId());
		// Start the connection timeout again
		CancelTimeout();
		iTimer.After(iStatus,30000000);
		SetActive();
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0488_DEVICEINSERTEDL_EXIT, this );
		return;
		}
	
	
	TUint32 token0,token1;
	err = testDevice.Device().GetTokenForInterface(0,token0);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0488_DEVICEINSERTEDL_DUP01, "<Error %d> Token for interface 0 could not be retrieved",err);
		return TestFailed(err);
		}
	err = iUsbInterface0.Open(token0); // Default interface setting 0
	if(err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0488_DEVICEINSERTEDL_DUP02, "<Error %d> Unable to open interface 0 using token %d",err,token0);
		return TestFailed(err);
		}
		
	__UHEAP_MARK;
	err = testDevice.Device().GetTokenForInterface(1,token1);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Token for interface 1 could not be retrieved"),err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0488_DEVICEINSERTEDL_DUP03, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);
		}
	err = iUsbInterface1.Open(token1); // Default interface setting 0
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to open interface 1 using token %d"),err,token1);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0488_DEVICEINSERTEDL_DUP04, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);
		} 
	
	// Select alternate interface setting 1	
	err = iUsbInterface1.SelectAlternateInterface(1);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Selecting alternate interface setting 1 on interface 1"),err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0488_DEVICEINSERTEDL_DUP05, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0488_DEVICEINSERTEDL_EXIT_DUP01, this );
		return;
		}  
	
	// Open a pipe for endpoint (Int in)	
	TInt endpointAddress;
	err = GetEndpointAddress(iUsbInterface1,1,KTransferTypeInterrupt,KEpDirectionIn,endpointAddress);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> unable to get endpoint address for interrupt in endpoint"),err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0488_DEVICEINSERTEDL_DUP06, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0488_DEVICEINSERTEDL_EXIT_DUP02, this );
		return;
		}	
	
	err = iUsbInterface1.OpenPipeForEndpoint(iInPipe,endpointAddress,EFalse);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to open pipe for endpoint 1"),err);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0488_DEVICEINSERTEDL_DUP07, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);		
		}		

	RThread rt;
	TInt r = rt.Create(_L("PanicThread"), CUT_PBASE_T_USBDI_0488::TestSelectAlternateInterfaceThenPanic, KDefaultStackSize, 0x1000, 0x1000, reinterpret_cast<TAny*>(this));
	gtest(r == KErrNone);
	TRequestStatus status;
	rt.Logon(status);
	rt.Resume();
	rt.Close();
	User::WaitForRequest(status);
	gtest(status.Int() == UsbdiPanics::EInterfaceSettingChangeWithPipeOpen);
	
	iUsbInterface1.Close();	
	 	
	__UHEAP_MARKEND;
	
	// ok now, inform client that the test has passed
	iCaseStep = EPassed;
	TTestCasePassed request;
	return iControlEp0->SendRequest(request,this);
	}


void CUT_PBASE_T_USBDI_0488::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0488_EP0TRANSFERCOMPLETEL_ENTRY, this );
	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0488_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{	
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0488_EP0TRANSFERCOMPLETEL_DUP01, msg);
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0488_EP0TRANSFERCOMPLETEL_EXIT, this );
	}
	

void CUT_PBASE_T_USBDI_0488::DeviceRemovedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0488_DEVICEREMOVEDL_ENTRY, this );
	
	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error	
	TestFailed(KErrDisconnected);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0488_DEVICEREMOVEDL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0488::BusErrorL(TInt aError)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0488_BUSERRORL_ENTRY, this );
	
	// This test case handles no failiures on the bus	
	TestFailed(KErrCompletion);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0488_BUSERRORL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0488::HostRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0488_HOSTRUNL_ENTRY, this );
	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{
		// Action timeout
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0488_HOSTRUNL, "<Error> Action timeout");
		TestFailed(KErrTimedOut);
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0488_HOSTRUNL_DUP01, "<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0488_HOSTRUNL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0488::DeviceRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0488_DEVICERUNL_ENTRY, this );
	
	// Disconnect the device	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request	
	TestPolicy().SignalTestComplete(iStatus.Int());
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0488_DEVICERUNL_EXIT, this );
	}

	
	}
