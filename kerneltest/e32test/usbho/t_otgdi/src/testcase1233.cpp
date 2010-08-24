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
// @internalComponent
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <u32std.h> 	// unicode builds
#include <e32base.h>
#include <e32base_private.h>
#include <e32Test.h>	// RTest headder
#include "testcaseroot.h"
#include "b2bwatchers.h"
#include "testcase1233.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase1233Traces.h"
#endif

#include <e32debug.h> 

#define LOG_INTO_STEP(a) test.Printf(_L("\nInto Step [%S]\n\n"), &a);


/* **************************************************************************************
 * the name below is used to add a pointer to our construction method to a pointer MAP in 
 * the class factory
 */
_LIT(KTestCaseId,"PBASE-USB_OTGDI-1233");
const TTestCaseFactoryReceipt<CTestCase1233> CTestCase1233::iFactoryReceipt(KTestCaseId);	

CTestCase1233* CTestCase1233::NewL(TBool aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE1233_NEWL);
	    }
	CTestCase1233* self = new (ELeave) CTestCase1233(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase1233::CTestCase1233(TBool aHost)
	: CTestCaseB2BRoot(KTestCaseId, aHost, iStatus) 
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE1233_CTESTCASE1233);
	    }
		
	} 


/**
 ConstructL
*/
void CTestCase1233::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE1233_CONSTRUCTL);
	    }

	iTestVID = 0x0E22;		// Symbian
	iTestPID = 0xF000 + 1233; // Test 1233
	
	BaseConstructL();
	}


CTestCase1233::~CTestCase1233()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE1233_DCTESTCASE1233);
	    }
	iCollector.DestroyObservers();
	Cancel();
	}


void CTestCase1233::ExecuteTestCaseL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE1233_EXECUTETESTCASEL);
	    }
	iCaseStep = EPreconditions;
	CActiveScheduler::Add(this);
	SelfComplete();
	}

	
void CTestCase1233::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE1233_DOCANCEL);
	    }
	// cancel our timer
	iTimer.Cancel();
	}


void CTestCase1233::RunStepL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE1233_RUNSTEPL);
	    }
	
	// Obtain the completion code for this CActive obj.
	TInt completionCode(iStatus.Int()); 
	TBuf<MAX_DSTRLEN> aDescription;

	switch(iCaseStep)
		{
		/*==================================================*/
		/* Fixed 'pre' steps								*/
		/*==================================================*/
		case EPreconditions:
			{
			LOG_INTO_STEP(_L("EPreconditions"))
		
			iCaseStep = ELoadLdd;
			StepB2BPreconditions();	
			break;
			}
			
		case ELoadLdd:
			{
			LOG_INTO_STEP(_L("ELoadLdd"))
			
			// Note we use here the hex equivalent of #1233 for the PID

			if (!StepLoadClient(0xF4D1/*use default settings for SRP/HNP support*/))
				{
				return TestFailed(KErrAbort, _L("Client Load Failure"));
				}
			
			//  load OTG ldd and init.		
			if (!StepLoadLDD())
				{
				return TestFailed(KErrAbort, _L("OTG Load Failure"));
				}
				
			if(otgActivateFdfActor()!=KErrNone)
				{
				return TestFailed(KErrAbort, _L("Couldn't load FDF Actor"));
				}
			
			// test that the right cable is in
			CheckRoleConnections();
					
			// subscribe to OTG states,events and messages now that it has loaded OK
			TRAPD(result, iCollector.CreateObserversL(*this));
			if (KErrNone != result)
				{
				return(TestFailed(KErrNoMemory, _L("Unable to create observers")));
				}

			// Allow enough time for 8 enumerations, say a second each, plus two of the
			// tests require 15 seconds of SOF traffic
			
			const TInt KTestCase1233Timeout = 45000;
			iCollector.AddStepTimeout(KTestCase1233Timeout);

			iCollector.ClearAllEvents();
			iCaseStep = ERaiseVBus;

			SelfComplete();
			break;
			}
			
		/*==================================================*/
		/* Steps for this test case only					*/
		/*==================================================*/
		
		case ELoopToNextPID:
			{
			LOG_INTO_STEP(_L("ELoopToNextPID"));
			
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}
			
			if ( iTestVID == 0x0E22 )
				{
				iTestVID = 0x1A0A;	// OPT and test devices
				iTestPID = 0x0100; 	// One *before* the first HS test ID
				}

			iTestPID++;
			
			if ( iTestPID > 0x0108 )
				{
				test.Printf(_L("All VID/PID pairs done\n"));
				OstTrace0(TRACE_NORMAL, CTESTCASE1233_RUNSTEPL_DUP01, "All VID/PID pairs done\n");
				
				iCaseStep = EUnloadLdd;
				}
			else
				{
				if(gTestRoleMaster)
					{
					// B
					test.Printf(_L("Setting VID/PID of 0x%04x/0x%04x\n"),iTestVID,iTestPID);
					OstTraceExt2(TRACE_NORMAL, CTESTCASE1233_RUNSTEPL_DUP02, "Setting VID/PID of 0x%04x/0x%04x\n",(TUint32)iTestVID,(TUint32)iTestPID);
					
					if (!StepChangeVidPid(iTestVID,iTestPID))
						{
						return TestFailed(KErrAbort, _L("VID/PID Change Failure"));
						}
					}
				else
					{
					// A
					test.Printf(_L("Expecting VID/PID of 0x%04x/0x%04x\n"),iTestVID,iTestPID);
					OstTraceExt2(TRACE_NORMAL, CTESTCASE1233_RUNSTEPL_DUP03, "Expecting VID/PID of 0x%04x/0x%04x\n",(TUint32)iTestVID,(TUint32)iTestPID);
					}
				
				iCollector.ClearAllEvents();
				iCaseStep = ERaiseVBus;
				}
			
			SelfComplete();
			break;			
			}

		case ERaiseVBus:
			{
			LOG_INTO_STEP(_L("ERaiseVBus"));
			
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}			

			if (gTestRoleMaster)
				{
				// B device
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventVbusRaised);
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateBPeripheral);
				iCollector.AddRequiredNotification(EWatcherPeripheralState, EUsbcDeviceStateDefault);
				iCollector.AddRequiredNotification(EWatcherPeripheralState, EUsbcDeviceStateAddress);
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventHnpEnabled);
				iCollector.AddRequiredNotification(EWatcherPeripheralState, EUsbcDeviceStateConfigured);
				}
			else
				{ 
				// A device
				test.Printf(_L("Raising VBUS for VID/PID = 0x%04x/0x%04x\n"),iTestVID,iTestPID);
				OstTraceExt2(TRACE_NORMAL, CTESTCASE1233_RUNSTEPL_DUP04, "Raising VBUS for VID/PID = 0x%04x/0x%04x\n",(TUint32)iTestVID,(TUint32)iTestPID);
				
				if ( otgBusRequest() != KErrNone )
					{
					return TestFailed(KErrAbort, _L("Raise Vbus - RUsbOtgDriver::BusRequest() FAILED!"));
					}
				
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventVbusRaised);
				iCollector.AddRequiredNotification(EWatcherAConnectionIdle, RUsbOtgDriver::EConnectionBusy);
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateAHost);
				}
			
			iCaseStep = EVBusRaised;
			SetActive();
			break;
			}
			
		case EVBusRaised:
			{
			LOG_INTO_STEP(_L("EVBusRaised"));

			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				return TestFailed(KErrAbort, _L("Timeout"));
				}

			if ( otgVbusPresent() )
				{
				test.Printf(_L("...VBUS is UP\n"));
				OstTrace0(TRACE_NORMAL, CTESTCASE1233_RUNSTEPL_DUP05, "...VBUS is UP\n");
				iCaseStep = EDropVBus;
				}
			else
				{
				test.Printf(_L("...VBUS is DOWN\n"));
				OstTrace0(TRACE_NORMAL, CTESTCASE1233_RUNSTEPL_DUP06, "...VBUS is DOWN\n");
				return TestFailed(KErrAbort, _L("Vbus did not rise - FAILED!"));
				}

			if (gTestRoleMaster)
				{
				// B device
				SelfComplete();
				}
			else
				{ 
				// A device
				
				// The default device (0x0E22/0xB4D1) is passed to the FDF, which will (eventually)
				// suspend and enter connection idle state.
				
				// The HS test devices (0x1A0A) will not be presented to FDF, so there will be no
				// trailing connection idle to find.
				
				if ( iTestVID == 0x0E22 )
					{
					iCollector.AddRequiredNotification(EWatcherAConnectionIdle, RUsbOtgDriver::EConnectionIdle);
					SetActive();
					}
				else
					{
					SelfComplete();
					}
				}

			break;
			}
			
		case EDropVBus:
			{
			LOG_INTO_STEP(_L("EDropVBus"));
			
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}			

			if ( gTestRoleMaster)
				{
				// B device
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventVbusDropped);
				}
			else
				{
				// A device
				otgBusDrop();
				
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventVbusDropped);
				}
			
			iCaseStep = EVBusDropped;
			SetActive();
			break;
			}
			
		case EVBusDropped:
			{
			LOG_INTO_STEP(_L("EVBusDropped"));

			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				return TestFailed(KErrAbort, _L("Timeout"));
				}

			if ( otgVbusPresent() )
				{
				test.Printf(_L("...VBUS is UP\n"));
				OstTrace0(TRACE_NORMAL, CTESTCASE1233_RUNSTEPL_DUP07, "...VBUS is UP\n");
				return TestFailed(KErrAbort, _L("Vbus did not rise - FAILED!"));
				}
			else
				{
				test.Printf(_L("...VBUS is DOWN\n"));
				OstTrace0(TRACE_NORMAL, CTESTCASE1233_RUNSTEPL_DUP08, "...VBUS is DOWN\n");
				iCaseStep = ELoopToNextPID;
				}

			SelfComplete();
			break;
			}
			
		/*==================================================*/
		/* Fixed 'post' steps								*/
		/*==================================================*/
			
		case EUnloadLdd:
			{
			LOG_INTO_STEP(_L("EUnloadLdd"))

			otgDeactivateFdfActor();
			iCollector.DestroyObservers();
			if (EFalse == StepUnloadLDD()){
				return TestFailed(KErrAbort,_L("unload Ldd failure"));
			}
			if (!StepUnloadClient()){
				return TestFailed(KErrAbort,_L("Client Unload Failure"));
			}

			iCaseStep = ELastStep;
			SelfComplete();
			break;
			}
			
		case ELastStep:
			{
			LOG_INTO_STEP(_L("ELastStep"))
			
			TestPassed();
			RequestCharacter();	
			break;
			}
			
		default:
			{
			LOG_INTO_STEP(_L("DEFAULT! (unknown test step)"));
			Cancel();
			RequestCharacter();	
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));
			}
		}
	
	}
