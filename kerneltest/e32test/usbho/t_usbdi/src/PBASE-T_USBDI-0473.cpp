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
// @file PBASE-T_USBDI-0473.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0473.h"
#include <d32usbc.h>
#include "testdebug.h"
#include "modelleddevices.h"
#include "TestPolicy.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0473Traces.h"
#endif

namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0473");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0473,TBool> CUT_PBASE_T_USBDI_0473::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0473* CUT_PBASE_T_USBDI_0473::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0473_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0473* self = new (ELeave) CUT_PBASE_T_USBDI_0473(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0473_NEWL_EXIT, ( TUint )( self ) );
	return self; 
	}
	   

CUT_PBASE_T_USBDI_0473::CUT_PBASE_T_USBDI_0473(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iSuspendedI0(EFalse),
	iSuspendedI1(EFalse),
	iDeviceNotificationPending(ETrue)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0473_CUT_PBASE_T_USBDI_0473_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0473_CUT_PBASE_T_USBDI_0473_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0473::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0473_CONSTRUCTL_ENTRY, this );
	iTestDevice = new RUsbDeviceA(this);
	BaseConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0473_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0473::~CUT_PBASE_T_USBDI_0473()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0473_CUT_PBASE_T_USBDI_0473_ENTRY_DUP01, this );
	
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0473_CUT_PBASE_T_USBDI_0473_EXIT_DUP01, this );
	}


void CUT_PBASE_T_USBDI_0473::ExecuteHostTestCaseL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0473_EXECUTEHOSTTESTCASEL_ENTRY, this );
	iCaseStep = EStepSuspend;
	iActorFDF = CActorFDF::NewL(*this);
	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	iInterface0Watcher = new (ELeave) CInterfaceWatcher(iUsbInterface0,TCallBack(CUT_PBASE_T_USBDI_0473::Interface0ResumedL,this));
	iInterface1Watcher = new (ELeave) CInterfaceWatcher(iUsbInterface1,TCallBack(CUT_PBASE_T_USBDI_0473::Interface1ResumedL,this));
	
	// Monitor for device connections
	iActorFDF->Monitor();

	// Start the connection timeout	
	TimeoutIn(30);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0473_EXECUTEHOSTTESTCASEL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0473::ExecuteDeviceTestCaseL()
	{
    OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0473_EXECUTEDEVICETESTCASEL_ENTRY, this );

	// Construct the device for the test case
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();

	// Connect the test device	
	iTestDevice->SoftwareConnect();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0473_EXECUTEDEVICETESTCASEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0473::HostDoCancel()
	{
    OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0473_HOSTDOCANCEL_ENTRY, this );

	// Cancel the timeout timer
	CancelTimeout();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0473_HOSTDOCANCEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0473::DeviceDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0473_DEVICEDOCANCEL_ENTRY, this );
	
	// Cancel the device	
	iTestDevice->CancelSubscriptionToReports();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0473_DEVICEDOCANCEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0473::DeviceInsertedL(TUint aDeviceHandle)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_ENTRY, this );

	Cancel(); // Cancel the timer
	TInt err(KErrNone);
	iDeviceHandle = aDeviceHandle;
	iActorFDF->Monitor();
	
	// Validate that device is as expected
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case

		OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL, "<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,testDevice.SerialNumber(),TestCaseId());

		// Start the connection timeout again
		TimeoutIn(30);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_EXIT, this );
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
	
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_DUP01, "Obtaining token for interface 0");
			err = testDevice.Device().GetTokenForInterface(0,token1);
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_DUP02, "<Error %d> Token for interface 0 could not be retrieved",err);
				return TestFailed(err);
				}
			OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_DUP03, "Token 1 (%d) retrieved",token1);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_DUP04, "Opening interface 0");
			err = iUsbInterface0.Open(token1); // Alternate interface setting 0
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_DUP05, "<Error %d> Interface 0 could not be opened",err);
				return TestFailed(err);
				}
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_DUP06, "Interface 0 opened");
		
																
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_DUP07, "Obtaining token for interface 1");
			err = testDevice.Device().GetTokenForInterface(1,token2);
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_DUP08, "<Error %d> Token for interface 1 could not be retrieved",err);
				return TestFailed(err);			
				}	
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_DUP09, "Opening interface 1");
			err = iUsbInterface1.Open(token2); // Alternate interface setting 0
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_DUP10, "<Error %d> Interface 1 could not be opened",err);
				return TestFailed(err);
				}
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_DUP11, "Interface 1 opened");
	
			// close it
			iUsbInterface1.Close();		
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_DUP12, "Interface 1 closed");
	
			//re-open now
			err = iUsbInterface1.Open(token2); // Alternate interface setting 0
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_DUP13, "<Error %d> Interface 1 could not be re-opened",err);
				return TestFailed(err);
				}
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_DUP14, "Interface 1 re-opened");
			
			
			// Suspend interface 0
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_DUP15, "Suspending interface 0");
			iInterface0Watcher->SuspendAndWatch();
			iSuspendedI0 = ETrue;
			
			// Suspend interface 1
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_DUP16, "Suspending interface 1");
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0473_DEVICEINSERTEDL_EXIT_DUP01, this );
	}


TInt CUT_PBASE_T_USBDI_0473::Interface0ResumedL(TAny* aPtr)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0473_INTERFACE0RESUMEDL_ENTRY, 0 );
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_INTERFACE0RESUMEDL, "Interface 0 resumed");
	CUT_PBASE_T_USBDI_0473* self = reinterpret_cast<CUT_PBASE_T_USBDI_0473*>(aPtr);
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_INTERFACE0RESUMEDL_DUP01, "watcher 0 iStatus=%d",self->iInterface0Watcher->CompletionCode());
	self->iSuspendedI0 = EFalse;
	return self->CheckForAllResumedNotificationsAndContinueFSM();
	}
	
	
TInt CUT_PBASE_T_USBDI_0473::Interface1ResumedL(TAny* aPtr)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0473_INTERFACE1RESUMEDL_ENTRY, 0 );
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_INTERFACE1RESUMEDL, "Interface 1 resumed");
	CUT_PBASE_T_USBDI_0473* self = reinterpret_cast<CUT_PBASE_T_USBDI_0473*>(aPtr);
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_INTERFACE1RESUMEDL_DUP01, "watcher 1 iStatus=%d",self->iInterface1Watcher->CompletionCode());
	self->iSuspendedI1 = EFalse;
	return self->CheckForAllResumedNotificationsAndContinueFSM();
	}
	
	
void CUT_PBASE_T_USBDI_0473::DeviceRemovedL(TUint aDeviceHandle)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0473_DEVICEREMOVEDL_ENTRY, this );

	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error

	TestFailed(KErrDisconnected);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0473_DEVICEREMOVEDL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0473::BusErrorL(TInt aError)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0473_BUSERRORL_ENTRY, this );

	// This test case handles no failiures on the bus

	TestFailed(aError);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0473_BUSERRORL_EXIT, this );
	}

TInt CUT_PBASE_T_USBDI_0473::CheckForAllResumedNotificationsAndContinueFSM()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0473_CHECKFORALLRESUMEDNOTIFICATIONSANDCONTINUEFSM_ENTRY, this );
	TBool readyToContinueFSM= ETrue;
	if( iInterface0Watcher->IsActive()
	||  iInterface0Watcher->iStatus == KRequestPending)
		{
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_CHECKFORALLRESUMEDNOTIFICATIONSANDCONTINUEFSM, "Interface 0 watcher still pending");
		readyToContinueFSM= EFalse;
		}

	if( iInterface1Watcher->IsActive()
	||  iInterface1Watcher->iStatus == KRequestPending)
		{
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_CHECKFORALLRESUMEDNOTIFICATIONSANDCONTINUEFSM_DUP01, "Interface 1 watcher still pending");
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
		OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0473_CHECKFORALLRESUMEDNOTIFICATIONSANDCONTINUEFSM_EXIT, this, KErrNone );
		return KErrNone;
		}
	}

TInt CUT_PBASE_T_USBDI_0473::ContinueFSMAfterAllResumedNotifications()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0473_CONTINUEFSMAFTERALLRESUMEDNOTIFICATIONS_ENTRY, this );
	iDeviceNotificationPending= ETrue;
	if(iSuspendedI0)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_CONTINUEFSMAFTERALLRESUMEDNOTIFICATIONS, "<Error %d> Interface 0 still suspended",KErrCompletion);
		TestFailed(KErrCompletion);
		OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0473_CONTINUEFSMAFTERALLRESUMEDNOTIFICATIONS_EXIT, this, KErrCompletion );
		return KErrCompletion;
		}

	if(iSuspendedI1)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_CONTINUEFSMAFTERALLRESUMEDNOTIFICATIONS_DUP01, "<Error %d> Interface 1 still suspended",KErrCompletion);
		TestFailed(KErrCompletion);
		OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0473_CONTINUEFSMAFTERALLRESUMEDNOTIFICATIONS_EXIT_DUP01, this, KErrCompletion );
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
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_CONTINUEFSMAFTERALLRESUMEDNOTIFICATIONS_DUP02, "<Error %d> Unable to permit remote device wakeup",err);
				iCaseStep = EFailed;
				TTestCaseFailed request(err,_L8("Unable to permit remote device wakeup"));
				iControlEp0->SendRequest(request,this);
				}
			else
				{
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_CONTINUEFSMAFTERALLRESUMEDNOTIFICATIONS_DUP03, "Device is resumed, send request to client: Remote wake up in 6 secs");
				
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
			
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_CONTINUEFSMAFTERALLRESUMEDNOTIFICATIONS_DUP04, "Suspending interface 0");
				iInterface0Watcher->SuspendAndWatch();
				iSuspendedI0 = ETrue;
				
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_CONTINUEFSMAFTERALLRESUMEDNOTIFICATIONS_DUP05, "Suspending interface 1");
				iInterface1Watcher->SuspendAndWatch();
				iSuspendedI1 = ETrue;
				
				iCaseStep = EValidateSuspendAfterWakeup;
				}
			else
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_CONTINUEFSMAFTERALLRESUMEDNOTIFICATIONS_DUP06, "<Error %d> Device is still suspended",KErrCompletion);
				TestFailed(KErrCompletion);
				OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0473_CONTINUEFSMAFTERALLRESUMEDNOTIFICATIONS_EXIT_DUP02, this, KErrCompletion );
				return KErrCompletion;
				}
			}
			break;

		default:
			OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_CONTINUEFSMAFTERALLRESUMEDNOTIFICATIONS_DUP07, "CUT_PBASE_T_USBDI_0473::ContinueFSMAfterAllResumedNotifications: Invalid state %d", iCaseStep);
			TestFailed(KErrCompletion);
			OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0473_CONTINUEFSMAFTERALLRESUMEDNOTIFICATIONS_EXIT_DUP03, this, KErrCompletion );
			return KErrCompletion;
		}

	OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0473_CONTINUEFSMAFTERALLRESUMEDNOTIFICATIONS_EXIT_DUP04, this, KErrNone );
	return KErrNone;
	}

void CUT_PBASE_T_USBDI_0473::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0473_DEVICESTATECHANGEL_ENTRY, this );
	Cancel();
	
	OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICESTATECHANGEL, "Device State change from %d to %d err=%d",aPreviousState,aNewState,aCompletionCode);

	switch(iCaseStep)
		{
	
		// Validate that the device was suspended by individual interface suspension
	
		case EValidateSuspendingInterfaces:
			{
			if(aNewState == RUsbDevice::EDeviceSuspended)
				{
				// Device state is suspended now resume it by resuming one of the interfaces
				
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICESTATECHANGEL_DUP01, "Device is suspended now resume device by resuming one of the interfaces");
				iUsbInterface0.CancelPermitSuspend();
				iCaseStep = EValidateResumptionAfterInterfaceSuspension;
				}
			else
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICESTATECHANGEL_DUP02, "<Error %d> State was not suspended",KErrCompletion);
				
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
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICESTATECHANGEL_DUP03, "<Error %d> Device is still suspended",KErrCompletion);
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
				
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICESTATECHANGEL_DUP04, "Now awaiting a remote wake up state change notification");
				
				CancelTimeout();
				iTimer.After(iStatus,10000000); // Give 10 seconds for device to signal remote wake-up
				iCaseStep = EValidateResumptionAfterWakeup;
				SetActive();
				}
			else
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICESTATECHANGEL_DUP05, "<Error %d> State was not suspended",KErrCompletion);
				
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
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICESTATECHANGEL_DUP06, "Resumed before suspended");
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
	
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICESTATECHANGEL_DUP07, "Device is now suspended, now activate the device again to send test case completed request to device");
	
				CUsbTestDevice& testDevice = iActorFDF->DeviceL(iDeviceHandle);
	
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICESTATECHANGEL_DUP08, "Resuming at device level");
				TInt err(testDevice.Device().Resume());
				if(err != KErrNone)
					{
					OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICESTATECHANGEL_DUP09, "<Error %d> Unable to suspend the device",err);
					iCaseStep = EFailed;
					TTestCaseFailed request(err,_L8("Unable to suspend the device"));
					iControlEp0->SendRequest(request,this);
					}
				
				iCaseStep = EPassed;
				}
			else
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICESTATECHANGEL_DUP10, "<Error %d> State was not suspended",KErrCompletion);
				
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
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICESTATECHANGEL_DUP11, "Device is active again, test case passed");
				TTestCasePassed request;
				iControlEp0->SendRequest(request,this);
				}
			else
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_DEVICESTATECHANGEL_DUP12, "<Error %d> Device is still suspended",KErrCompletion);
				return TestFailed(KErrCompletion);
				}
			}
			break;
			
		default:
			break;
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0473_DEVICESTATECHANGEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0473::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0473_EP0TRANSFERCOMPLETEL_ENTRY, this );
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	switch(iCaseStep)
		{
		case ESuspendForRemoteWakeup:
			{
			// Suspend device again so a remote wakeup can be achieved
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_EP0TRANSFERCOMPLETEL_DUP01, "Suspend device again so a remote wakeup can be achieved");
	
			// Suspend interface 0
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_EP0TRANSFERCOMPLETEL_DUP02, "Suspending interface 0");
			iInterface0Watcher->SuspendAndWatch();
			iSuspendedI0 = ETrue;
			
			// Suspend interface 1
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_EP0TRANSFERCOMPLETEL_DUP03, "Suspending interface 1");
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0473_EP0TRANSFERCOMPLETEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0473::HostRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0473_HOSTRUNL_ENTRY, this );
	
	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{
		// Action timeout
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_HOSTRUNL, "<Error> Action timeout");
		TestFailed(KErrTimedOut);
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0473_HOSTRUNL_DUP01, "<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0473_HOSTRUNL_EXIT, this );
	}
	

void CUT_PBASE_T_USBDI_0473::DeviceRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0473_DEVICERUNL_ENTRY, this );
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0473_DEVICERUNL_EXIT, this );
	}
	
	
	}
