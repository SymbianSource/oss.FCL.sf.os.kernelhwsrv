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
// @file PBASE-T_USBDI-0473.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0473.h"
#include <d32usbc.h>
#include "testdebug.h"
#include "modelleddevices.h"
#include "TestPolicy.h"

namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0473");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0473,TBool> CUT_PBASE_T_USBDI_0473::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0473* CUT_PBASE_T_USBDI_0473::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0473* self = new (ELeave) CUT_PBASE_T_USBDI_0473(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self; 
	}
	   

CUT_PBASE_T_USBDI_0473::CUT_PBASE_T_USBDI_0473(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iSuspendedI0(EFalse),
	iSuspendedI1(EFalse),
	iDeviceNotificationPending(ETrue)
	{
	} 


void CUT_PBASE_T_USBDI_0473::ConstructL()
	{
	iTestDevice = new RUsbDeviceA(this);
	BaseConstructL();
	}


CUT_PBASE_T_USBDI_0473::~CUT_PBASE_T_USBDI_0473()
	{
	LOG_FUNC
	
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
	if(!IsHost() && iTestDevice)
		{
		iTestDevice->Close();
		}		
	delete iTestDevice;
	}


void CUT_PBASE_T_USBDI_0473::ExecuteHostTestCaseL()
	{
	LOG_FUNC
	iCaseStep = EStepSuspend;
	iActorFDF = CActorFDF::NewL(*this);
	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	iInterface0Watcher = new (ELeave) CInterfaceWatcher(iUsbInterface0,TCallBack(CUT_PBASE_T_USBDI_0473::Interface0ResumedL,this));
	iInterface1Watcher = new (ELeave) CInterfaceWatcher(iUsbInterface1,TCallBack(CUT_PBASE_T_USBDI_0473::Interface1ResumedL,this));
	
	// Monitor for device connections
	iActorFDF->Monitor();

	// Start the connection timeout	
	TimeoutIn(30);
	}

void CUT_PBASE_T_USBDI_0473::ExecuteDeviceTestCaseL()
	{
	LOG_FUNC

	// Construct the device for the test case
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();

	// Connect the test device	
	iTestDevice->SoftwareConnect();
	}
	
	
void CUT_PBASE_T_USBDI_0473::HostDoCancel()
	{
	LOG_FUNC

	// Cancel the timeout timer
	CancelTimeout();
	}


void CUT_PBASE_T_USBDI_0473::DeviceDoCancel()
	{
	LOG_FUNC
	
	// Cancel the device	
	iTestDevice->CancelSubscriptionToReports();
	}
	
void CUT_PBASE_T_USBDI_0473::DeviceInsertedL(TUint aDeviceHandle)
	{
	LOG_FUNC

	Cancel(); // Cancel the timer
	TInt err(KErrNone);
	iDeviceHandle = aDeviceHandle;
	iActorFDF->Monitor();
	
	// Validate that device is as expected
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case

		RDebug::Printf("<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,&testDevice.SerialNumber(),&TestCaseId());

		// Start the connection timeout again
		TimeoutIn(30);
		return;
		}	
	// Check tree now	
	CHECK(CheckTreeAfterDeviceInsertion(testDevice, _L("RDeviceA")) == KErrNone);
		
	// Perform the correct test step				
	switch(iCaseStep)
		{
		case EStepSuspend:
			{
			TUint32 token1(0);
			TUint32 token2(0);
	
			RDebug::Printf("Obtaining token for interface 0");
			err = testDevice.Device().GetTokenForInterface(0,token1);
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Token for interface 0 could not be retrieved",err);
				return TestFailed(err);
				}
			RDebug::Printf("Token 1 (%d) retrieved",token1);
			RDebug::Printf("Opening interface 0");
			err = iUsbInterface0.Open(token1); // Alternate interface setting 0
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Interface 0 could not be opened",err);
				return TestFailed(err);
				}
			RDebug::Printf("Interface 0 opened");
		
																
			RDebug::Printf("Obtaining token for interface 1");
			err = testDevice.Device().GetTokenForInterface(1,token2);
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Token for interface 1 could not be retrieved",err);
				return TestFailed(err);			
				}	
			RDebug::Printf("Opening interface 1");
			err = iUsbInterface1.Open(token2); // Alternate interface setting 0
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Interface 1 could not be opened",err);
				return TestFailed(err);
				}
			RDebug::Printf("Interface 1 opened");
	
			// close it
			iUsbInterface1.Close();		
			RDebug::Printf("Interface 1 closed");
	
			//re-open now
			err = iUsbInterface1.Open(token2); // Alternate interface setting 0
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Interface 1 could not be re-opened",err);
				return TestFailed(err);
				}
			RDebug::Printf("Interface 1 re-opened");
			
			
			// Suspend interface 0
			RDebug::Printf("Suspending interface 0");
			iInterface0Watcher->SuspendAndWatch();
			iSuspendedI0 = ETrue;
			
			// Suspend interface 1
			RDebug::Printf("Suspending interface 1");
			iInterface1Watcher->SuspendAndWatch();
			iSuspendedI1 = ETrue;
			
			iCaseStep = EValidateSuspendingInterfaces;
			TimeoutIn(10); // Give 10 seconds for device to suspend
			}
			break;
			
		default:
			TestFailed(KErrCorrupt);
			break;
		}	
	}


TInt CUT_PBASE_T_USBDI_0473::Interface0ResumedL(TAny* aPtr)
	{
	LOG_CFUNC
	RDebug::Printf("Interface 0 resumed");
	CUT_PBASE_T_USBDI_0473* self = reinterpret_cast<CUT_PBASE_T_USBDI_0473*>(aPtr);
	RDebug::Printf("watcher 0 iStatus=%d",self->iInterface0Watcher->CompletionCode());
	self->iSuspendedI0 = EFalse;
	return self->CheckForAllResumedNotificationsAndContinueFSM();
	}
	
	
TInt CUT_PBASE_T_USBDI_0473::Interface1ResumedL(TAny* aPtr)
	{
	LOG_CFUNC
	RDebug::Printf("Interface 1 resumed");
	CUT_PBASE_T_USBDI_0473* self = reinterpret_cast<CUT_PBASE_T_USBDI_0473*>(aPtr);
	RDebug::Printf("watcher 1 iStatus=%d",self->iInterface1Watcher->CompletionCode());
	self->iSuspendedI1 = EFalse;
	return self->CheckForAllResumedNotificationsAndContinueFSM();
	}
	
	
void CUT_PBASE_T_USBDI_0473::DeviceRemovedL(TUint aDeviceHandle)
	{
	LOG_FUNC

	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error

	TestFailed(KErrDisconnected);
	}
	
	
void CUT_PBASE_T_USBDI_0473::BusErrorL(TInt aError)
	{
	LOG_FUNC

	// This test case handles no failiures on the bus

	TestFailed(aError);
	}

TInt CUT_PBASE_T_USBDI_0473::CheckForAllResumedNotificationsAndContinueFSM()
	{
	LOG_FUNC
	TBool readyToContinueFSM= ETrue;
	if( iInterface0Watcher->IsActive()
	||  iInterface0Watcher->iStatus == KRequestPending)
		{
		RDebug::Printf("Interface 0 watcher still pending");
		readyToContinueFSM= EFalse;
		}

	if( iInterface1Watcher->IsActive()
	||  iInterface1Watcher->iStatus == KRequestPending)
		{
		RDebug::Printf("Interface 1 watcher still pending");
		readyToContinueFSM= EFalse;
		}

	if( iDeviceNotificationPending)
		{
		readyToContinueFSM= EFalse;
		}

	if( readyToContinueFSM)
		{
		return ContinueFSMAfterAllResumedNotifications();
		}
	else
		{
		return KErrNone;
		}
	}

TInt CUT_PBASE_T_USBDI_0473::ContinueFSMAfterAllResumedNotifications()
	{
	LOG_FUNC
	iDeviceNotificationPending= ETrue;
	if(iSuspendedI0)
		{
		RDebug::Printf("<Error %d> Interface 0 still suspended",KErrCompletion);
		TestFailed(KErrCompletion);
		return KErrCompletion;
		}

	if(iSuspendedI1)
		{
		RDebug::Printf("<Error %d> Interface 1 still suspended",KErrCompletion);
		TestFailed(KErrCompletion);
		return KErrCompletion;
		}

	switch(iCaseStep)
		{
		case EValidateResumptionAfterInterfaceSuspension:
			{
			// Device is resumed, send request to client: Remote wake up in 6 secs
			TInt err= iUsbInterface1.PermitRemoteWakeup(ETrue);
			
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Unable to permit remote device wakeup",err);
				iCaseStep = EFailed;
				TTestCaseFailed request(err,_L8("Unable to permit remote device wakeup"));
				iControlEp0->SendRequest(request,this);
				}
			else
				{
				RDebug::Printf("Device is resumed, send request to client: Remote wake up in 6 secs");
				
				TRemoteWakeupRequest request(6);
				iControlEp0->SendRequest(request,this);			
				iCaseStep = ESuspendForRemoteWakeup;		
				}
			}
			break;

		case EValidateResumptionAfterWakeup:
			{
			if(iStoredNewState == RUsbDevice::EDeviceActive)
				{
				// Now suspend the device again after resumption from remote wakeup
			
				RDebug::Printf("Suspending interface 0");
				iInterface0Watcher->SuspendAndWatch();
				iSuspendedI0 = ETrue;
				
				RDebug::Printf("Suspending interface 1");
				iInterface1Watcher->SuspendAndWatch();
				iSuspendedI1 = ETrue;
				
				iCaseStep = EValidateSuspendAfterWakeup;
				}
			else
				{
				RDebug::Printf("<Error %d> Device is still suspended",KErrCompletion);
				TestFailed(KErrCompletion);
				return KErrCompletion;
				}
			}
			break;

		default:
			RDebug::Printf("CUT_PBASE_T_USBDI_0473::ContinueFSMAfterAllResumedNotifications: Invalid state %d", iCaseStep);
			TestFailed(KErrCompletion);
			return KErrCompletion;
		}

	return KErrNone;
	}

void CUT_PBASE_T_USBDI_0473::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	LOG_FUNC
	Cancel();
	
	RDebug::Printf("Device State change from %d to %d err=%d",aPreviousState,aNewState,aCompletionCode);

	switch(iCaseStep)
		{
	
		// Validate that the device was suspended by individual interface suspension
	
		case EValidateSuspendingInterfaces:
			{
			if(aNewState == RUsbDevice::EDeviceSuspended)
				{
				// Device state is suspended now resume it by resuming one of the interfaces
				
				RDebug::Printf("Device is suspended now resume device by resuming one of the interfaces");
				iUsbInterface0.CancelPermitSuspend();
				iCaseStep = EValidateResumptionAfterInterfaceSuspension;
				}
			else
				{
				RDebug::Printf("<Error %d> State was not suspended",KErrCompletion);
				
				// Since the device is not suspended, send test case failed to the device
	
				iCaseStep = EFailed;
				TTestCaseFailed request(KErrCompletion,_L8("The device was not in the expected suspend state"));
				iControlEp0->SendRequest(request,this);
				}
			}
			break;
			
		// Validate that device is now active after resuming one of the interfaces
	
		case EValidateResumptionAfterInterfaceSuspension:
			{
			iDeviceNotificationPending= EFalse;

			if(aNewState == RUsbDevice::EDeviceActive)
				{
				CheckForAllResumedNotificationsAndContinueFSM();
				}
			else
				{
				RDebug::Printf("<Error %d> Device is still suspended",KErrCompletion);
				return TestFailed(KErrCompletion);
				}
			}
			break;
			
		// Validate that the device is now suspended for the device to remote-wakeup
		case EValidateSuspendForRemoteWakeup:
			{
			if(aNewState == RUsbDevice::EDeviceSuspended)
				{
				// Now awaiting a remote wake up state change notification
				
				RDebug::Printf("Now awaiting a remote wake up state change notification");
				
				CancelTimeout();
				iTimer.After(iStatus,10000000); // Give 10 seconds for device to signal remote wake-up
				iCaseStep = EValidateResumptionAfterWakeup;
				SetActive();
				}
			else
				{
				RDebug::Printf("<Error %d> State was not suspended",KErrCompletion);
				
				// Since the device is not suspended, send test case failed to the device
	
				iCaseStep = EFailed;
				TTestCaseFailed request(KErrCompletion,_L8("State was not suspended"));
				iControlEp0->SendRequest(request,this);
				}
			}
			break;
			
		// This step should never be reached as ep0 complete traps this step, but if it does test fails.
		case ESuspendForRemoteWakeup:
			{
			RDebug::Printf("Resumed before suspended");	    
		    iCaseStep = EFailed;
		    TTestCaseFailed request(KErrCompletion,_L8("State was not suspended"));
		    iControlEp0->SendRequest(request,this);
		    break;	
			}	    
				
		// Validate that the device is now active from a remote wakeup
		case EValidateResumptionAfterWakeup:
			{
			iDeviceNotificationPending= EFalse;
			iStoredNewState= aNewState;
			CheckForAllResumedNotificationsAndContinueFSM();
			}
			break;
			
		// Validate that the device can be suspended after a remote wakeup
	
		case EValidateSuspendAfterWakeup:
			{
			// Successfully suspended after a remote wake up event
			if(aNewState == RUsbDevice::EDeviceSuspended)
				{
				// Device is now suspended, now activate the device again to send test case
				// completed request to device
	
				RDebug::Printf("Device is now suspended, now activate the device again to send test case completed request to device");
	
				CUsbTestDevice& testDevice = iActorFDF->DeviceL(iDeviceHandle);
	
				RDebug::Printf("Resuming at device level");
				TInt err(testDevice.Device().Resume());
				if(err != KErrNone)
					{
					RDebug::Printf("<Error %d> Unable to suspend the device",err);
					iCaseStep = EFailed;
					TTestCaseFailed request(err,_L8("Unable to suspend the device"));
					iControlEp0->SendRequest(request,this);
					}
				
				iCaseStep = EPassed;
				}
			else
				{
				RDebug::Printf("<Error %d> State was not suspended",KErrCompletion);
				
				// Since the device is not suspended, send test case failed to the device
	
				iCaseStep = EFailed;
				TTestCaseFailed request(KErrCompletion,_L8("State was not suspended"));
				iControlEp0->SendRequest(request,this);
				}
			}
			break;
	
		// Validate that the device is now active again 
	
		case EPassed:
			{
			if(aNewState == RUsbDevice::EDeviceActive)
				{
				RDebug::Printf("Device is active again, test case passed");
				TTestCasePassed request;
				iControlEp0->SendRequest(request,this);
				}
			else
				{
				RDebug::Printf("<Error %d> Device is still suspended",KErrCompletion);
				return TestFailed(KErrCompletion);
				}
			}
			break;
			
		default:
			break;
		}
	}


void CUT_PBASE_T_USBDI_0473::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	LOG_FUNC
	RDebug::Printf("Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	switch(iCaseStep)
		{
		case ESuspendForRemoteWakeup:
			{
			// Suspend device again so a remote wakeup can be achieved
			RDebug::Printf("Suspend device again so a remote wakeup can be achieved");
	
			// Suspend interface 0
			RDebug::Printf("Suspending interface 0");
			iInterface0Watcher->SuspendAndWatch();
			iSuspendedI0 = ETrue;
			
			// Suspend interface 1
			RDebug::Printf("Suspending interface 1");
			iInterface1Watcher->SuspendAndWatch();
			iSuspendedI1 = ETrue;
			
			iCaseStep = EValidateSuspendForRemoteWakeup;
			TimeoutIn(10); // Give 10 seconds for device to suspend	
			}
			break;
	
		// Fail the test case
	
		default:
		case EFailed:
			TestFailed(KErrCompletion);
			break;
	
		// Pass the test case
	
		case EPassed:
			TestPassed();
			break;
			}
	}


void CUT_PBASE_T_USBDI_0473::HostRunL()
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
	

void CUT_PBASE_T_USBDI_0473::DeviceRunL()
	{
	LOG_FUNC
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	}
	
	
	}
