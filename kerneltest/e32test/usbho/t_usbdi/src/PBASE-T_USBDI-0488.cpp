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
// @file PBASE-T_USBDI-0488.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0488.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include <d32usbdi_errors.h>

namespace NUnitTesting_USBDI
	{

_LIT(KTestCaseId,"PBASE-T_USBDI-0488");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0488,TBool> CUT_PBASE_T_USBDI_0488::iFunctor(KTestCaseId);

CUT_PBASE_T_USBDI_0488* CUT_PBASE_T_USBDI_0488::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0488* self = new (ELeave) CUT_PBASE_T_USBDI_0488(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_0488::CUT_PBASE_T_USBDI_0488(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	} 


void CUT_PBASE_T_USBDI_0488::ConstructL()
	{
	iTestDevice = new RUsbDeviceA(this);
	BaseConstructL();
	}


CUT_PBASE_T_USBDI_0488::~CUT_PBASE_T_USBDI_0488()
	{
	LOG_FUNC
	
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
	}
	
	
void CUT_PBASE_T_USBDI_0488::ExecuteHostTestCaseL()	
	{
	LOG_FUNC
		
	iActorFDF = CActorFDF::NewL(*this);
	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	iActorFDF->Monitor();
	
	TimeoutIn(30);
	}
	
/*static*/ TInt CUT_PBASE_T_USBDI_0488::TestSelectAlternateInterfaceThenPanic(TAny* aTest) 
	{
	LOG_CFUNC
	
	CTrapCleanup* trapHandler=CTrapCleanup::New();
	if(!trapHandler)
		{
		return KErrNoMemory;
		}		
						
	CUT_PBASE_T_USBDI_0488* pTest = reinterpret_cast<CUT_PBASE_T_USBDI_0488*>(aTest);
	
	// change interface setting whilst pipes are opened in another alt. setting, should panic
	pTest->iUsbInterface1.SelectAlternateInterface(0); // panic expected
	
	// Should never get here.*/
	return KErrNone;
	}
	
	
	
void CUT_PBASE_T_USBDI_0488::HostDoCancel()
	{
	LOG_FUNC	
	// Cancel the timeout timer	
	CancelTimeout();
	}
	
	
void CUT_PBASE_T_USBDI_0488::ExecuteDeviceTestCaseL()	
	{
	LOG_FUNC	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	iTestDevice->SoftwareConnect();
	}
	
void CUT_PBASE_T_USBDI_0488::DeviceDoCancel()
	{
	LOG_FUNC	
	// Cancel the device	
	iTestDevice->CancelSubscriptionToReports();
	}
	
	
void CUT_PBASE_T_USBDI_0488::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
		TInt aCompletionCode)
	{
	LOG_FUNC	
	}


void CUT_PBASE_T_USBDI_0488::DeviceInsertedL(TUint aDeviceHandle)
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
		CancelTimeout();
		iTimer.After(iStatus,30000000);
		SetActive();
		return;
		}
	
	
	TUint32 token0,token1;
	err = testDevice.Device().GetTokenForInterface(0,token0);
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Token for interface 0 could not be retrieved",err);
		return TestFailed(err);
		}
	err = iUsbInterface0.Open(token0); // Default interface setting 0
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to open interface 0 using token %d",err,token0);
		return TestFailed(err);
		}
		
	__UHEAP_MARK;
	err = testDevice.Device().GetTokenForInterface(1,token1);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Token for interface 1 could not be retrieved"),err);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		return iControlEp0->SendRequest(request,this);
		}
	err = iUsbInterface1.Open(token1); // Default interface setting 0
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to open interface 1 using token %d"),err,token1);
		RDebug::Print(msg);
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
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		return;
		}  
	
	// Open a pipe for endpoint (Int in)	
	TInt endpointAddress;
	err = GetEndpointAddress(iUsbInterface1,1,KTransferTypeInterrupt,KEpDirectionIn,endpointAddress);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> unable to get endpoint address for interrupt in endpoint"),err);
		RDebug::Print(msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		return;
		}	
	
	err = iUsbInterface1.OpenPipeForEndpoint(iInPipe,endpointAddress,EFalse);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Unable to open pipe for endpoint 1"),err);
		RDebug::Print(msg);
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
	LOG_FUNC
	
	RDebug::Printf("Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{	
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
		RDebug::Print(msg);
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
	}
	

void CUT_PBASE_T_USBDI_0488::DeviceRemovedL(TUint aDeviceHandle)
	{
	LOG_FUNC
	
	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error	
	TestFailed(KErrDisconnected);
	}
	
	
void CUT_PBASE_T_USBDI_0488::BusErrorL(TInt aError)
	{
	LOG_FUNC
	
	// This test case handles no failiures on the bus	
	TestFailed(KErrCompletion);
	}

void CUT_PBASE_T_USBDI_0488::HostRunL()
	{
	LOG_FUNC	
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

void CUT_PBASE_T_USBDI_0488::DeviceRunL()
	{
	LOG_FUNC
	
	// Disconnect the device	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request	
	TestPolicy().SignalTestComplete(iStatus.Int());
	}

	
	}
