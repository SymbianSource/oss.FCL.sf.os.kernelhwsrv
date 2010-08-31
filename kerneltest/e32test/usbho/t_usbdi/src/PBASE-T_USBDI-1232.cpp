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
// @file PBASE-T_USBDI-1232.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-1232.h"
#include <d32usbc.h>
#include "testdebug.h"
#include "modelleddevices.h"
#include "TestPolicy.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-1232Traces.h"
#endif

namespace NUnitTesting_USBDI
	{

	_LIT(KTestCaseId,"PBASE-T_USBDI-1232");
	// the name is very important 
	const TFunctorTestCase<CUT_PBASE_T_USBDI_1232,TBool>
			CUT_PBASE_T_USBDI_1232::iFunctor(KTestCaseId);

	CUT_PBASE_T_USBDI_1232* CUT_PBASE_T_USBDI_1232::NewL(TBool aHostRole)
		{
		CUT_PBASE_T_USBDI_1232* self = new (ELeave) CUT_PBASE_T_USBDI_1232(aHostRole);
		CleanupStack::PushL(self);
		self->ConstructL();
		CleanupStack::Pop(self);
		return self;
		}

	CUT_PBASE_T_USBDI_1232::CUT_PBASE_T_USBDI_1232(TBool aHostRole) :
		CBaseTestCase(KTestCaseId, aHostRole)
		{
		}

	void CUT_PBASE_T_USBDI_1232::ConstructL()
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_CUT_PBASE_T_USBDI_1232, "====> Constructor entry priority = %d", RThread().Priority());

		// Collect existing thread priority (to reinstate later)
		iPriority = RThread().Priority();

		iTestDevice = new RUsbDeviceA(this);
		BaseConstructL();
		}

	CUT_PBASE_T_USBDI_1232::~CUT_PBASE_T_USBDI_1232()
		{

		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232, "====> Destructor entry priority = %d", RThread().Priority());

		// Reinstate original priority

		RThread().SetPriority(iPriority);

		// Cancel any async operations

		Cancel(); // Cancel host timer

		// Destroy the watchers
		// they still use opened interfaces to cancel the suspend if active
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
		}

	void CUT_PBASE_T_USBDI_1232::ExecuteHostTestCaseL()
		{

		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP01, "====> ExecuteHostTestCaseL entry priority = %d",
				RThread().Priority());

		// Bump thread priority for this test only

		RThread().SetPriority(EPriorityAbsoluteHigh);
		OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP02, "Thread priority raised %d->%d", iPriority, RThread().Priority());

		iCaseStep = EInProcess;
		iActorFDF = CActorFDF::NewL(*this);
		iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
		iInterface0Watcher = new (ELeave) CInterfaceWatcher(iUsbInterface0,TCallBack(CUT_PBASE_T_USBDI_1232::Interface0ResumedL,this));
		iInterface1Watcher = new (ELeave) CInterfaceWatcher(iUsbInterface1,TCallBack(CUT_PBASE_T_USBDI_1232::Interface1ResumedL,this));

		// Monitor for device connections
		iActorFDF->Monitor();

		// Start the connection timeout	
		TimeoutIn(30);
		}

	void CUT_PBASE_T_USBDI_1232::ExecuteDeviceTestCaseL()
		{

		// Construct the device for the test case
		iTestDevice->OpenL(TestCaseId());
		iTestDevice->SubscribeToReports(iStatus);
		SetActive();

		// Connect the test device	
		iTestDevice->SoftwareConnect();
		}

	void CUT_PBASE_T_USBDI_1232::HostDoCancel()
		{

		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP03, "====> HostDoCancel entry priority = %d", RThread().Priority());

		// Cancel the timeout timer
		CancelTimeout();
		}

	void CUT_PBASE_T_USBDI_1232::DeviceDoCancel()
		{

		// Cancel the device	
		iTestDevice->CancelSubscriptionToReports();
		}

	void CUT_PBASE_T_USBDI_1232::DeviceInsertedL(TUint aDeviceHandle)
		{

		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP04, "====> DeviceInsertedL entry priority = %d", RThread().Priority());

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

				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP05, "Obtaining token for interface 0");
				err = testDevice.Device().GetTokenForInterface(0, token1);
				if (err != KErrNone)
					{
					RDebug::Printf(
							"<Error %d> Token for interface 0 could not be retrieved",
							err);
					return TestFailed(err);
					}
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP06, "Token 1 (%d) retrieved", token1);
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP07, "Opening interface 0");
				err = iUsbInterface0.Open(token1); // Alternate interface setting 0
				if (err != KErrNone)
					{
					RDebug::Printf(
							"<Error %d> Interface 0 could not be opened", err);
					return TestFailed(err);
					}
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP08, "Interface 0 opened");

				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP09, "Obtaining token for interface 1");
				err = testDevice.Device().GetTokenForInterface(1, token2);
				if (err != KErrNone)
					{
					RDebug::Printf(
							"<Error %d> Token for interface 1 could not be retrieved",
							err);
					return TestFailed(err);
					}
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP10, "Opening interface 1");
				err = iUsbInterface1.Open(token2); // Alternate interface setting 0
				if (err != KErrNone)
					{
					RDebug::Printf(
							"<Error %d> Interface 1 could not be opened", err);
					return TestFailed(err);
					}
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP11, "Interface 1 opened");

				// device go to suspend
				// Suspend interface 0
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP12, "Suspending interface 0");
				iInterface0Watcher->SuspendAndWatch();

				// Suspend interface 1
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP13, "Suspending interface 1");
				iInterface1Watcher->SuspendAndWatch();

				iCaseStep = ESuspendWhenResuming;

				TimeoutIn(10); // Give 10 seconds for device to suspend						
                
				
				iUsbInterface0.CancelWaitForResume(); // a tricky way to close the watcher of interface
				iUsbInterface1.CancelWaitForResume();
				}
				break;

			default:
				TestFailed(KErrCorrupt);
				break;
			}
		}

	TInt CUT_PBASE_T_USBDI_1232::Interface0ResumedL(TAny* aPtr)
		{

		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP14, "====> Interface0ResumedL entry priority = %d", RThread().Priority());

		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP15, " -Interface 0 resumed");
		CUT_PBASE_T_USBDI_1232* self =
				reinterpret_cast<CUT_PBASE_T_USBDI_1232*>(aPtr);
		TInt completionCode = self->iInterface0Watcher->CompletionCode();
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP16, " -watcher 0 iStatus=%d",completionCode);

		switch (self->iCaseStep)
			{	
                
			case EPassed:
				{
				if (completionCode == KErrNone)
					{
					OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP17, "Device resume successed,test passed!");
					self->SendEp0Request(); // stop client site
					}
				else
					{
					OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP18, "Device resume failed, err = %d ",completionCode);
					self->iCaseStep = EFailed;
					self->SendEp0Request();
					}
				}
				break;
			case EFailed:				
				self->SendEp0Request();
				break;

			default:
				break;
			};

		return KErrNone;
		}

	TInt CUT_PBASE_T_USBDI_1232::Interface1ResumedL(TAny* aPtr)
		{

		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP19, "====> Interface1ResumedL entry priority = %d", RThread().Priority());

		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP20, "Interface 1 resumed");
		CUT_PBASE_T_USBDI_1232* self =
				reinterpret_cast<CUT_PBASE_T_USBDI_1232*>(aPtr);
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP21, "watcher 1 iStatus=%d", self->iInterface1Watcher->CompletionCode());
		return KErrNone;
		}

	void CUT_PBASE_T_USBDI_1232::DeviceRemovedL(TUint aDeviceHandle)
		{

		// The test device should not be removed until the test case has passed
		// so this test case has not completed, and state this event as an error

		TestFailed(KErrDisconnected);
		}

	void CUT_PBASE_T_USBDI_1232::BusErrorL(TInt aError)
		{

		// This test case handles no failiures on the bus

		TestFailed(aError);
		}

	void CUT_PBASE_T_USBDI_1232::DeviceStateChangeL(
			RUsbDevice::TDeviceState aPreviousState,
			RUsbDevice::TDeviceState aNewState, TInt aCompletionCode)
		{
		Cancel();

		// test RInterface , the  RUsbDevice notification logic not used . 
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP22, " -Device State change from err=%d",aCompletionCode);

		switch (iCaseStep)
			{
			case ESuspendWhenResuming:
				if (aNewState == RUsbDevice::EDeviceSuspended)
					{
					OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP23, "====> device has suspended!");
                    
					SuspendWhenResuming();					
					}
				break;
			case EValidSuspendWhenResuming:
				if (aPreviousState == RUsbDevice::EDeviceSuspended&&aNewState
						== RUsbDevice::EDeviceSuspended)
					{
					OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP24, "====> device suspended again,suspend while resuming succeed!");
					iCaseStep = EPassed;
					iUsbInterface0.CancelPermitSuspend();
					}
				else
					{
					iCaseStep = EFailed;
					iUsbInterface0.CancelPermitSuspend();
			        }
		
				break;
			default:
				break;
			}

		}

	void CUT_PBASE_T_USBDI_1232::Ep0TransferCompleteL(TInt aCompletionCode)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP25, "Ep0TransferCompleteL with aCompletionCode = %d",
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
		}

	void CUT_PBASE_T_USBDI_1232::HostRunL()
		{

		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP26, "====> HostRunL entry priority = %d", RThread().Priority());

		// Obtain the completion code
		TInt completionCode(iStatus.Int());

		if (completionCode == KErrNone)
			{
			// Action timeout
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP27, "<Error> Action timeout");
			TestFailed(KErrTimedOut);
			}
		else
			{
			OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP28, "<Error %d> Timeout timer could not complete",
					completionCode);
			TestFailed(completionCode);
			}
		}

	void CUT_PBASE_T_USBDI_1232::DeviceRunL()
		{

		// Disconnect the device

		iTestDevice->SoftwareDisconnect();

		// Complete the test case request

		TestPolicy().SignalTestComplete(iStatus.Int());
		}

	void CUT_PBASE_T_USBDI_1232::SuspendWhenResuming()
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP29, "====> SuspendWhenResuming entry priority = %d",
				RThread().Priority());

		// Cancel suspend-in-progress
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP30, "Cancel Suspend interface 0");
				
		iUsbInterface0.CancelPermitSuspend();
		
		// how to prove, see log? 
		// Suspend interface 0
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP31, "Suspending interface 0");
		iInterface0Watcher->SuspendAndWatch();			

		// Suspend interface 1
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1232_DCUT_PBASE_T_USBDI_1232_DUP32, "Suspending interface 1");
		iInterface1Watcher->SuspendAndWatch();

		iCaseStep = EValidSuspendWhenResuming;

		TimeoutIn(10); // Give 10 seconds for device to suspend	

		}

	void CUT_PBASE_T_USBDI_1232::SendEp0Request()
		{
		TTestCasePassed request;
		iControlEp0->SendRequest(request, this);
		}

	}//end namespace


