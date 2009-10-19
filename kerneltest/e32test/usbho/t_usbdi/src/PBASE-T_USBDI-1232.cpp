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
// @file PBASE-T_USBDI-1232.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-1232.h"
#include <d32usbc.h>
#include "testdebug.h"
#include "modelleddevices.h"
#include "TestPolicy.h"

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
		RDebug::Printf("====> Constructor entry priority = %d", RThread().Priority());

		// Collect existing thread priority (to reinstate later)
		iPriority = RThread().Priority();

		iTestDevice = new RUsbDeviceA(this);
		BaseConstructL();
		}

	CUT_PBASE_T_USBDI_1232::~CUT_PBASE_T_USBDI_1232()
		{
		LOG_FUNC

		RDebug::Printf("====> Destructor entry priority = %d", RThread().Priority());

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
		LOG_FUNC

		RDebug::Printf("====> ExecuteHostTestCaseL entry priority = %d",
				RThread().Priority());

		// Bump thread priority for this test only

		RThread().SetPriority(EPriorityAbsoluteHigh);
		RDebug::Printf("Thread priority raised %d->%d", iPriority, RThread().Priority());

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
		LOG_FUNC

		// Construct the device for the test case
		iTestDevice->OpenL(TestCaseId());
		iTestDevice->SubscribeToReports(iStatus);
		SetActive();

		// Connect the test device	
		iTestDevice->SoftwareConnect();
		}

	void CUT_PBASE_T_USBDI_1232::HostDoCancel()
		{
		LOG_FUNC

		RDebug::Printf("====> HostDoCancel entry priority = %d", RThread().Priority());

		// Cancel the timeout timer
		CancelTimeout();
		}

	void CUT_PBASE_T_USBDI_1232::DeviceDoCancel()
		{
		LOG_FUNC

		// Cancel the device	
		iTestDevice->CancelSubscriptionToReports();
		}

	void CUT_PBASE_T_USBDI_1232::DeviceInsertedL(TUint aDeviceHandle)
		{
		LOG_FUNC

		RDebug::Printf("====> DeviceInsertedL entry priority = %d", RThread().Priority());

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

				RDebug::Printf("Obtaining token for interface 0");
				err = testDevice.Device().GetTokenForInterface(0, token1);
				if (err != KErrNone)
					{
					RDebug::Printf(
							"<Error %d> Token for interface 0 could not be retrieved",
							err);
					return TestFailed(err);
					}
				RDebug::Printf("Token 1 (%d) retrieved", token1);
				RDebug::Printf("Opening interface 0");
				err = iUsbInterface0.Open(token1); // Alternate interface setting 0
				if (err != KErrNone)
					{
					RDebug::Printf(
							"<Error %d> Interface 0 could not be opened", err);
					return TestFailed(err);
					}
				RDebug::Printf("Interface 0 opened");

				RDebug::Printf("Obtaining token for interface 1");
				err = testDevice.Device().GetTokenForInterface(1, token2);
				if (err != KErrNone)
					{
					RDebug::Printf(
							"<Error %d> Token for interface 1 could not be retrieved",
							err);
					return TestFailed(err);
					}
				RDebug::Printf("Opening interface 1");
				err = iUsbInterface1.Open(token2); // Alternate interface setting 0
				if (err != KErrNone)
					{
					RDebug::Printf(
							"<Error %d> Interface 1 could not be opened", err);
					return TestFailed(err);
					}
				RDebug::Printf("Interface 1 opened");

				// device go to suspend
				// Suspend interface 0
				RDebug::Printf("Suspending interface 0");
				iInterface0Watcher->SuspendAndWatch();

				// Suspend interface 1
				RDebug::Printf("Suspending interface 1");
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
		LOG_CFUNC

		RDebug::Printf("====> Interface0ResumedL entry priority = %d", RThread().Priority());

		RDebug::Printf(" -Interface 0 resumed");
		CUT_PBASE_T_USBDI_1232* self =
				reinterpret_cast<CUT_PBASE_T_USBDI_1232*>(aPtr);
		TInt completionCode = self->iInterface0Watcher->CompletionCode();
		RDebug::Printf(" -watcher 0 iStatus=%d",completionCode);

		switch (self->iCaseStep)
			{	
                
			case EPassed:
				{
				if (completionCode == KErrNone)
					{
					RDebug::Printf("Device resume successed,test passed!");
					self->SendEp0Request(); // stop client site
					}
				else
					{
					RDebug::Printf("Device resume failed, err = %d ",completionCode);
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
		LOG_CFUNC

		RDebug::Printf("====> Interface1ResumedL entry priority = %d", RThread().Priority());

		RDebug::Printf("Interface 1 resumed");
		CUT_PBASE_T_USBDI_1232* self =
				reinterpret_cast<CUT_PBASE_T_USBDI_1232*>(aPtr);
		RDebug::Printf("watcher 1 iStatus=%d", self->iInterface1Watcher->CompletionCode());
		return KErrNone;
		}

	void CUT_PBASE_T_USBDI_1232::DeviceRemovedL(TUint aDeviceHandle)
		{
		LOG_FUNC

		// The test device should not be removed until the test case has passed
		// so this test case has not completed, and state this event as an error

		TestFailed(KErrDisconnected);
		}

	void CUT_PBASE_T_USBDI_1232::BusErrorL(TInt aError)
		{
		LOG_FUNC

		// This test case handles no failiures on the bus

		TestFailed(aError);
		}

	void CUT_PBASE_T_USBDI_1232::DeviceStateChangeL(
			RUsbDevice::TDeviceState aPreviousState,
			RUsbDevice::TDeviceState aNewState, TInt aCompletionCode)
		{
		LOG_FUNC
		Cancel();

		// test RInterface , the  RUsbDevice notification logic not used . 
		RDebug::Printf(" -Device State change from err=%d",aCompletionCode);

		switch (iCaseStep)
			{
			case ESuspendWhenResuming:
				if (aNewState == RUsbDevice::EDeviceSuspended)
					{
					RDebug::Printf("====> device has suspended!");
                    
					SuspendWhenResuming();					
					}
				break;
			case EValidSuspendWhenResuming:
				if (aPreviousState == RUsbDevice::EDeviceSuspended&&aNewState
						== RUsbDevice::EDeviceSuspended)
					{
					RDebug::Printf("====> device suspended again,suspend while resuming succeed!");
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
		LOG_FUNC
		RDebug::Printf("Ep0TransferCompleteL with aCompletionCode = %d",
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
		LOG_FUNC

		RDebug::Printf("====> HostRunL entry priority = %d", RThread().Priority());

		// Obtain the completion code
		TInt completionCode(iStatus.Int());

		if (completionCode == KErrNone)
			{
			// Action timeout
			RDebug::Printf("<Error> Action timeout");
			TestFailed(KErrTimedOut);
			}
		else
			{
			RDebug::Printf("<Error %d> Timeout timer could not complete",
					completionCode);
			TestFailed(completionCode);
			}
		}

	void CUT_PBASE_T_USBDI_1232::DeviceRunL()
		{
		LOG_FUNC

		// Disconnect the device

		iTestDevice->SoftwareDisconnect();

		// Complete the test case request

		TestPolicy().SignalTestComplete(iStatus.Int());
		}

	void CUT_PBASE_T_USBDI_1232::SuspendWhenResuming()
		{
		RDebug::Printf("====> SuspendWhenResuming entry priority = %d",
				RThread().Priority());

		// Cancel suspend-in-progress
		RDebug::Printf("Cancel Suspend interface 0");
				
		iUsbInterface0.CancelPermitSuspend();
		
		// how to prove, see log? 
		// Suspend interface 0
		RDebug::Printf("Suspending interface 0");
		iInterface0Watcher->SuspendAndWatch();			

		// Suspend interface 1
		RDebug::Printf("Suspending interface 1");
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


