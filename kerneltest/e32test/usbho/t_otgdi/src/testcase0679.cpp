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
#include "testcase0679.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase0679Traces.h"
#endif

#define _REPEATS (oOpenIterations*3)


/* **************************************************************************************
 * the name below is used to add a pointer to our construction method to a pointer MAP in 
 * the class factory
 */
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0679");
const TTestCaseFactoryReceipt<CTestCase0679> CTestCase0679::iFactoryReceipt(KTestCaseId);	

CTestCase0679* CTestCase0679::NewL(TBool aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0679_NEWL);
	    }
	CTestCase0679* self = new (ELeave) CTestCase0679(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0679::CTestCase0679(TBool aHost)
	: CTestCaseB2BRoot(KTestCaseId, aHost, iStatus) 
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0679_CTESTCASE0679);
	    }
		
	} 


/**
 ConstructL
*/
void CTestCase0679::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0679_CONSTRUCTL);
	    }

	iDualRoleCase = ETrue;	// another back-back
		
	BaseConstructL();
	}


CTestCase0679::~CTestCase0679()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0679_DCTESTCASE0679);
	    }
	iCollector.DestroyObservers();
	Cancel();
	}


void CTestCase0679::ExecuteTestCaseL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0679_EXECUTETESTCASEL);
	    }
	iCaseStep = EPreconditions;
	CActiveScheduler::Add(this);
	SelfComplete();
	}

	
void CTestCase0679::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0679_DOCANCEL);
	    }
	// cancel our timer
	iTimer.Cancel();
	}


// handle event completion	
void CTestCase0679::RunStepL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0679_RUNSTEPL);
	    }
	// Obtain the completion code for this CActive obj.
	TInt completionCode(iStatus.Int()); 
	TBuf<MAX_DSTRLEN> aDescription;
	TInt err(0);

	switch(iCaseStep)
		{
		case EPreconditions:
			{
			LOG_STEPNAME(_L("EPreconditions"))
			iCaseStep = ELoadLdd;
			StepB2BPreconditions();				
			break;
			}
			
			// 1. Load the Client LDD 
		case ELoadLdd:
			{
			LOG_STEPNAME(_L("ELoadLdd"))
			if (!StepLoadClient(0xF679, EFalse /*no HNP support!*/))
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
				return(TestFailed(KErrNoMemory, _L("Unable to create observers")));
			iCollector.ClearAllEvents();
			iCaseStep = EReadyToRaiseVBus;
			SelfComplete();
			break;
			}
			
		case EReadyToRaiseVBus:
			{
			if (gTestRoleMaster)
				{
				// wait for Vbus to be raised
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateBIdle);
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventVbusRaised);
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateBPeripheral);
				}
			else
				{ // slave "A"
				// Raise VBus, then wait for default role
				err = otgBusRequest();	// ok to turn on VBus now
				if (KErrNone != err)
					{
					return TestFailed(KErrAbort, _L("Raise Vbus - RUsbOtgDriver::BusRequest() FAILED!"));
					}
				
				// we might also wait for and EStateAIdle
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateAIdle);
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventVbusRaised);
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventRoleChangedToHost);
				}

			const TInt KTestCase0679VBusRaiseTimeout = 2000;	//	2 seconds, should be plenty of time for this test 
			iCollector.AddStepTimeout(KTestCase0679VBusRaiseTimeout);

			iCaseStep = EDefaultRoles;
			SetActive();									
			break;
			}
			
		case EDefaultRoles:
			{
			test.Printf(_L("Into EDefaultRoles step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0679_RUNSTEPL_DUP01, "Into EDefaultRoles step...\n");
			LOG_STEPNAME(_L("EWaitEnumeration"));
			
			if (gTestRoleMaster)
				{
				//	B-Device should now wait until it is configured
				iCollector.AddRequiredNotification(EWatcherPeripheralState, EUsbcDeviceStateConfigured);

				const TInt KTestCase0679BTimeout = 5000;			//	5 seconds, should be plenty of time for overall test 
				iCollector.AddStepTimeout(KTestCase0679BTimeout);

				iCaseStep = EBConfigured;
				}
			else
				{
				//	A-Device should expect nothing more (role swap won't happen
				//	because the B-Device doesn't indicate support for OTG in its
				//	OTG descriptor, so the host won't have sent SetFeature(b_hnp_enable))
				//	Instead, just rely on the test timeout and display a request for the user 
				//	to observe the B-Device test code for the pass/fail
				const TInt KTestCase0679ATimeout = 4000;			//	4 seconds before A-Device drops VBus (before B test times out at 5 seconds)
				iCollector.AddStepTimeout(KTestCase0679ATimeout);	//	NB. In this test on the A-Device, we expect to timeout
																	//	so a timeout isn't treated as a failure
				test.Printf(_L("NOTE : Please observe test result on B-Device...\n"));
				OstTrace0(TRACE_NORMAL, CTESTCASE0679_RUNSTEPL_DUP02, "NOTE : Please observe test result on B-Device...\n");
				iCaseStep = EDropVBus;	//	This is the step the A-Device will go to
										//	when the timer (set up in previous test) fires
				}
			SetActive();
			break;
			}

			
		case EBConfigured:	//	A B-Device only step!
			{
			test.Printf(_L("Into EBConfigured step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0679_RUNSTEPL_DUP03, "Into EBConfigured step...\n");
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}
			iCollector.AddRequiredNotification(EWatcherPeripheralState, EUsbcDeviceStateSuspended);
			iCaseStep = EBSuspended;
			SetActive();
			break;
			}
			
		case EBSuspended:	
			{
			test.Printf(_L("Into EBSuspended step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0679_RUNSTEPL_DUP04, "Into EBSuspended step...\n");
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}			
			
			// issue HNP
			test.Printf(_L("Attempting a swap on a non-HNP enabled link...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0679_RUNSTEPL_DUP05, "Attempting a swap on a non-HNP enabled link...\n");
			iCollector.AddRequiredNotification(EWatcherMessage, RUsbOtgDriver::EMessageHnpNotEnabled);

			err = otgBusRequest();	//	Request the host role

			if (KErrNone != err)
				{
				test.Printf(_L("BusRequest returned %d)"),err);
				OstTrace1(TRACE_NORMAL, CTESTCASE0679_RUNSTEPL_DUP06, "BusRequest returned %d)",err);
				//DS Temp! return TestFailed(KErrAbort, _L("BusRequest() failed!"));
				}
		
			iCaseStep = EBErrorReceived;
			SetActive();
			break;
			}
			
		case EBErrorReceived:
			{
			test.Printf(_L("Into EBErrorReceived step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0679_RUNSTEPL_DUP07, "Into EBErrorReceived step...\n");
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}			
			iCaseStep = EDropVBus;	//	Test has pretty much passed now. Just wait for A-Device to drop VBus.
			SelfComplete();
			break;
			}

		case EDropVBus:
			LOG_STEPNAME(_L("EDropVBus"))

			iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventVbusDropped);
			if ( gTestRoleMaster)
				{
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateBIdle);
				}
			else
				{ // SLAVE "A"
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateAIdle);
				otgBusDrop();
				}
			iCaseStep = EVBusDropped;
			SetActive();
			break;
			
		case EVBusDropped:
			LOG_STEPNAME(_L("EVBusDropped"))
			if (KTestCaseWatchdogTO == iStatus.Int() && gTestRoleMaster)
				{
				//	Remember, a timeout is only a failure for the B-Device in this test,
				//	we're expecting and relying on a timeout on the A-Device to conclude
				//	the test and bring VBus down.
				return TestFailed(KErrAbort, _L("Timeout"));
				}
				
			if (otgVbusPresent())
				{
				return TestFailed(KErrAbort, _L("Vbus did not drop - FAILED!"));
				}
			iCaseStep = EUnloadLdd;
			SelfComplete();
			break;
			
		case EUnloadLdd:
			LOG_STEPNAME(_L("EUnloadLdd"))
			iCollector.DestroyObservers();
			otgDeactivateFdfActor();
			if (EFalse == StepUnloadLDD())
				return TestFailed(KErrAbort,_L("unload Ldd failure"));	
			if (!StepUnloadClient())
				return TestFailed(KErrAbort,_L("Client Unload Failure"));	

			iCaseStep = ELastStep;
			SelfComplete();
			break;
			
		case ELastStep:
			LOG_STEPNAME(_L("ELastStep"))
			TestPassed();
			break;
			
		default:
			test.Printf(_L("<Error> unknown test step"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0679_RUNSTEPL_DUP08, "<Error> unknown test step");
			Cancel();
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));
		}
	}
