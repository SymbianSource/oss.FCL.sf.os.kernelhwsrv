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
// @file PBASE-T_USBDI-1235.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-1235.h"
#include <d32usbc.h>
#include "testdebug.h"
#include "modelleddevices.h"
#include "TestPolicy.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-1235Traces.h"
#endif

namespace NUnitTesting_USBDI
	{

	_LIT(KTestCaseId,"PBASE-T_USBDI-1235");
	// the name is very important 
	const TFunctorTestCase<CUT_PBASE_T_USBDI_1235,TBool>
			CUT_PBASE_T_USBDI_1235::iFunctor(KTestCaseId);

	CUT_PBASE_T_USBDI_1235* CUT_PBASE_T_USBDI_1235::NewL(TBool aHostRole)
		{
		OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1235_NEWL_ENTRY, aHostRole );
		CUT_PBASE_T_USBDI_1235* self = new (ELeave) CUT_PBASE_T_USBDI_1235(aHostRole);
		CleanupStack::PushL(self);
		self->ConstructL();
		CleanupStack::Pop(self);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_NEWL_EXIT, ( TUint )( self ) );
		return self;
		}

	CUT_PBASE_T_USBDI_1235::CUT_PBASE_T_USBDI_1235(TBool aHostRole) :
		CBaseTestCase(KTestCaseId, aHostRole), iSuspendedI0(EFalse),
				iSuspendedI1(EFalse)
		{
		OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1235_CUT_PBASE_T_USBDI_1235_ENTRY, this );
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_CUT_PBASE_T_USBDI_1235_EXIT, this );
		}

	void CUT_PBASE_T_USBDI_1235::ConstructL()
		{
		OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1235_CONSTRUCTL_ENTRY, this );
		iTestDevice = new RUsbDeviceA(this);
		BaseConstructL();
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_CONSTRUCTL_EXIT, this );
		}

	CUT_PBASE_T_USBDI_1235::~CUT_PBASE_T_USBDI_1235()
		{
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1235_CUT_PBASE_T_USBDI_1235_ENTRY_DUP01, this );

		// Cancel any async operations

		Cancel(); // Cancel host timer

		// Destroy the watchers
		delete iInterface1Watcher;
		delete iInterface0Watcher;

		// Close the interfaces
		iUsbInterface1.Close();
		iUsbInterface0.Close();

		delete iControlEp0;
		delete iActorFDF;
		if (!IsHost() && iTestDevice)
			{
			iTestDevice->Close();
			}
		delete iTestDevice;
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_CUT_PBASE_T_USBDI_1235_EXIT_DUP01, this );
		}

	void CUT_PBASE_T_USBDI_1235::ExecuteHostTestCaseL()
		{
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1235_EXECUTEHOSTTESTCASEL_ENTRY, this );

		iCaseStep = EInProcess;
		iActorFDF = CActorFDF::NewL(*this);
		iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
		iInterface0Watcher = new (ELeave) CInterfaceWatcher(iUsbInterface0,TCallBack(CUT_PBASE_T_USBDI_1235::Interface0ResumedL,this));
		iInterface1Watcher = new (ELeave) CInterfaceWatcher(iUsbInterface1,TCallBack(CUT_PBASE_T_USBDI_1235::Interface1ResumedL,this));

		// Monitor for device connections
		iActorFDF->Monitor();

		// Start the connection timeout	
		TimeoutIn(30);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_EXECUTEHOSTTESTCASEL_EXIT, this );
		}

	void CUT_PBASE_T_USBDI_1235::ExecuteDeviceTestCaseL()
		{
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1235_EXECUTEDEVICETESTCASEL_ENTRY, this );

		// Construct the device for the test case
		iTestDevice->OpenL(TestCaseId());
		iTestDevice->SubscribeToReports(iStatus);
		SetActive();

		// Connect the test device	
		iTestDevice->SoftwareConnect();
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_EXECUTEDEVICETESTCASEL_EXIT, this );
		}

	void CUT_PBASE_T_USBDI_1235::HostDoCancel()
		{
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1235_HOSTDOCANCEL_ENTRY, this );

		// Cancel the timeout timer
		CancelTimeout();
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_HOSTDOCANCEL_EXIT, this );
		}

	void CUT_PBASE_T_USBDI_1235::DeviceDoCancel()
		{
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1235_DEVICEDOCANCEL_ENTRY, this );

		// Cancel the device	
		iTestDevice->CancelSubscriptionToReports();
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_DEVICEDOCANCEL_EXIT, this );
		}

	void CUT_PBASE_T_USBDI_1235::DeviceInsertedL(TUint aDeviceHandle)
		{
        OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1235_DEVICEINSERTEDL_ENTRY, this );

		Cancel(); // Cancel the timer
		TInt err(KErrNone);
		iDeviceHandle = aDeviceHandle;
		iActorFDF->Monitor();

		// Validate that device is as expected
		CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
		if (testDevice.SerialNumber().Compare(TestCaseId()) != 0)
			{
			// Incorrect device for this test case

			RDebug::Printf(
					"<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
					KErrNotFound, &testDevice.SerialNumber(), &TestCaseId());

			// Start the connection timeout again
			TimeoutIn(30);
			OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_DEVICEINSERTEDL_EXIT, this );
			return;
			}
		// Check tree now	
		CHECK(CheckTreeAfterDeviceInsertion(testDevice, _L("RDeviceA")) == KErrNone);

		// Perform the correct test step				
		switch (iCaseStep)
			{
			case EInProcess:
				{
				TUint32 token1(0);
				TUint32 token2(0);

				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235, "Obtaining token for interface 0");
				err = testDevice.Device().GetTokenForInterface(0, token1);
				if (err != KErrNone)
					{
					RDebug::Printf(
							"<Error %d> Token for interface 0 could not be retrieved",
							err);
					return TestFailed(err);
					}
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP01, "Token 1 (%d) retrieved", token1);
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP02, "Opening interface 0");
				err = iUsbInterface0.Open(token1); // Alternate interface setting 0
				if (err != KErrNone)
					{
					RDebug::Printf(
							"<Error %d> Interface 0 could not be opened", err);
					return TestFailed(err);
					}
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP03, "Interface 0 opened");

				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP04, "Obtaining token for interface 1");
				err = testDevice.Device().GetTokenForInterface(1, token2);
				if (err != KErrNone)
					{
					RDebug::Printf(
							"<Error %d> Token for interface 1 could not be retrieved",
							err);
					return TestFailed(err);
					}
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP05, "Opening interface 1");
				err = iUsbInterface1.Open(token2); // Alternate interface setting 0
				if (err != KErrNone)
					{
					RDebug::Printf(
							"<Error %d> Interface 1 could not be opened", err);
					return TestFailed(err);
					}
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP06, "Interface 1 opened");

				SuspendDeviceByInterfacesAndCancelWaitForResume();

				iCaseStep = EValidCancelSuspendAfterInterfaceSuspend;
				
				}
				break;

			default:
				TestFailed(KErrCorrupt);
				break;
			}
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_DEVICEINSERTEDL_EXIT_DUP01, this );
		}

	TInt CUT_PBASE_T_USBDI_1235::Interface0ResumedL(TAny* aPtr)
		{
		OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1235_INTERFACE0RESUMEDL_ENTRY, 0 );
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP07, "Interface 0 resumed");
		CUT_PBASE_T_USBDI_1235* self =
				reinterpret_cast<CUT_PBASE_T_USBDI_1235*>(aPtr);
		TInt completionCode=self->iInterface0Watcher->CompletionCode();
		RDebug::Printf(
				"watcher 0 iStatus=%d",
				completionCode);
		self->iSuspendedI0 = EFalse;

		switch (self->iCaseStep)
			{

			case EValidCancelSuspendAfterInterfaceSuspend:
				{
				if (completionCode == KErrCancel)
					{
					OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP08, "CancelWaitForResume request: Success <%d>!",completionCode);
				    // do not care the device's status.
					self->iCaseStep = EPassed;					
					}
				else
					{
					RDebug::Printf(
							"CancelWaitForResume request: Failed, < err %d >",
							completionCode);
					self->iCaseStep = EFailed;
					
					}
				}

				break;

			default:
				break;
			};

		OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_1235_INTERFACE0RESUMEDL_EXIT, 0, KErrNone );
		return KErrNone;
		}

	TInt CUT_PBASE_T_USBDI_1235::Interface1ResumedL(TAny* aPtr)
		{
		OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1235_INTERFACE1RESUMEDL_ENTRY, 0 );
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP09, "Interface 1 resumed");
		CUT_PBASE_T_USBDI_1235* self =
				reinterpret_cast<CUT_PBASE_T_USBDI_1235*>(aPtr);
		TInt completionCode = self->iInterface1Watcher->CompletionCode();		
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP10, "watcher 1 iStatus=%d",completionCode);
		self->iSuspendedI1 = EFalse;
		
		if(self->iCaseStep == EPassed && completionCode == KErrNone )
			{
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP11, "Device resumed,test passed!");
			self->SendEp0Request(); // stop client;
			}
		
		OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_1235_INTERFACE1RESUMEDL_EXIT, 0, KErrNone );
		return KErrNone;
		}

	void CUT_PBASE_T_USBDI_1235::DeviceRemovedL(TUint aDeviceHandle)
		{
        OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1235_DEVICEREMOVEDL_ENTRY, this );

		// The test device should not be removed until the test case has passed
		// so this test case has not completed, and state this event as an error

		TestFailed(KErrDisconnected);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_DEVICEREMOVEDL_EXIT, this );
		}

	void CUT_PBASE_T_USBDI_1235::BusErrorL(TInt aError)
		{
        OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1235_BUSERRORL_ENTRY, this );

		// This test case handles no failiures on the bus

		TestFailed(aError);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_BUSERRORL_EXIT, this );
		}


	void CUT_PBASE_T_USBDI_1235::DeviceStateChangeL(
			RUsbDevice::TDeviceState aPreviousState,
			RUsbDevice::TDeviceState aNewState, TInt aCompletionCode)
		{
		OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1235_DEVICESTATECHANGEL_ENTRY, this );
		Cancel();
		
		// test RInterface , the  RUsbDevice notification logic not used . 
		RDebug::Printf(
				"-Device State change from %d to %d err=%d",
				aPreviousState, aNewState, aCompletionCode);

		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_DEVICESTATECHANGEL_EXIT, this );
		}

	void CUT_PBASE_T_USBDI_1235::Ep0TransferCompleteL(TInt aCompletionCode)
		{
		OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1235_EP0TRANSFERCOMPLETEL_ENTRY, this );
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP12, "Ep0TransferCompleteL with aCompletionCode = %d",
				aCompletionCode);
		switch (iCaseStep)
			{

			default:
			case EFailed:
				TestFailed(KErrCompletion);
				break;			

			case EPassed:
				TestPassed();
				break;
			}
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_EP0TRANSFERCOMPLETEL_EXIT, this );
		}

	void CUT_PBASE_T_USBDI_1235::HostRunL()
		{
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1235_HOSTRUNL_ENTRY, this );

		// Obtain the completion code
		TInt completionCode(iStatus.Int());

		if (completionCode == KErrNone)
			{
			// Action timeout
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP13, "<Error> Action timeout");
			TestFailed(KErrTimedOut);
			}
		else
			{
			OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP14, "<Error %d> Timeout timer could not complete",
					completionCode);
			TestFailed(completionCode);
			}
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_HOSTRUNL_EXIT, this );
		}

	void CUT_PBASE_T_USBDI_1235::DeviceRunL()
		{
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1235_DEVICERUNL_ENTRY, this );

		// Disconnect the device

		iTestDevice->SoftwareDisconnect();

		// Complete the test case request

		TestPolicy().SignalTestComplete(iStatus.Int());
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_DEVICERUNL_EXIT, this );
		}

	void CUT_PBASE_T_USBDI_1235::SuspendDeviceByInterfacesAndCancelWaitForResume()
		{
		OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1235_SUSPENDDEVICEBYINTERFACESANDCANCELWAITFORRESUME_ENTRY, this );
		// Suspend interface 0
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP15, "Suspending interface 0");
		iInterface0Watcher->SuspendAndWatch();
		iSuspendedI0 = ETrue;
		
		// Suspend interface 1
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP16, "Suspending interface 1");
		iInterface1Watcher->SuspendAndWatch();
		iSuspendedI1 = ETrue;
		
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP17, "CancelPermitSuspend interface 1");
		// try to resume device,then host can notify peripheral test passed.
		iUsbInterface1.CancelPermitSuspend();

		// Only test the CancelWaitForResume() operation 
		// If we let the devic to suspend ,the periperal site could not receive the test control transfer which test finish result.
        // select interface0 to test the function CancelWaitForResume	
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1235_DCUT_PBASE_T_USBDI_1235_DUP18, "CacelWaitForResume interface 0");
		iUsbInterface0.CancelWaitForResume();

		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_SUSPENDDEVICEBYINTERFACESANDCANCELWAITFORRESUME_EXIT, this );
		}
	
	void CUT_PBASE_T_USBDI_1235::SendEp0Request()
		{		
		OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1235_SENDEP0REQUEST_ENTRY, this );
		TTestCasePassed request;
		iControlEp0->SendRequest(request, this);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1235_SENDEP0REQUEST_EXIT, this );
		}

	}//end namespace


