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
//#include "testcasewd.h"
#include "b2bwatchers.h"
#include "testcase0678.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase0678Traces.h"
#endif

#define _REPEATS (oOpenIterations*3)


/* **************************************************************************************
 * the name below is used to add a pointer to our construction method to a pointer MAP in 
 * the class factory
 */
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0678");
const TTestCaseFactoryReceipt<CTestCase0678> CTestCase0678::iFactoryReceipt(KTestCaseId);	

CTestCase0678* CTestCase0678::NewL(TBool aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0678_NEWL);
	    }
	CTestCase0678* self = new (ELeave) CTestCase0678(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0678::CTestCase0678(TBool aHost)
	: CTestCaseB2BRoot(KTestCaseId, aHost, iStatus) 
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0678_CTESTCASE0678);
	    }
		
	} 


/**
 ConstructL
*/
void CTestCase0678::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0678_CONSTRUCTL);
	    }

	iDualRoleCase = ETrue;	// another back-back
		
	BaseConstructL();
	}


CTestCase0678::~CTestCase0678()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0678_DCTESTCASE0678);
	    }
	iCollector.DestroyObservers();
	Cancel();
	}


void CTestCase0678::ExecuteTestCaseL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0678_EXECUTETESTCASEL);
	    }
	iCaseStep = EPreconditions;
	iHNPCounter = 3;	//	To be decremented to govern the number of times we do HNP.
	CActiveScheduler::Add(this);
	SelfComplete();
	}

	
void CTestCase0678::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0678_DOCANCEL);
	    }
	// cancel our timer
	iTimer.Cancel();
	}


// handle event completion	
void CTestCase0678::RunStepL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0678_RUNSTEPL);
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
			if (!StepLoadClient(0xF678/*use default settings for SRP/HNP support*/))
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
			iCaseStep = EDefaultRoles;
			const TInt KTestCase0678Timeout = 30000;			//	30 seconds, should be plenty of time for 3 role swaps
			iCollector.AddStepTimeout(KTestCase0678Timeout);
			SetActive();
			break;
			}
			
		case EDefaultRoles:
			{
			test.Printf(_L("Into EDefaultRoles step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0678_RUNSTEPL_DUP01, "Into EDefaultRoles step...\n");
			
			if ( --iHNPCounter >= 0)
				{
				//	We want to do further role swapping
				if (gTestRoleMaster)
					{
					//	B-Device should now wait until it is configured
					iCollector.AddRequiredNotification(EWatcherPeripheralState, EUsbcDeviceStateConfigured);
					iCaseStep = EBConfigured;
					}
				else
					{
					//	A-Device should expect nothing more until it becomes A-Peripheral
					iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateAPeripheral);
					iCaseStep = ESwappedRoles;
					}
				SetActive();
				}
			else
				{
				//	We've done 3 x HNP cycles back to default roles. 
				//	Time to shut down VBus and stop the test case.
				iCaseStep = EDropVBus;
				SelfComplete();
				}
			break;
			}

			
		case EBConfigured:	//	A B-Device only step!
			{
			test.Printf(_L("Into EBConfigured step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0678_RUNSTEPL_DUP02, "Into EBConfigured step...\n");
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
			OstTrace0(TRACE_NORMAL, CTESTCASE0678_RUNSTEPL_DUP03, "Into EBSuspended step...\n");
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}			
			
			// issue HNP
			test.Printf(_L("VBus present, attempting a swap.\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0678_RUNSTEPL_DUP04, "VBus present, attempting a swap.\n");
			iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateBHost);

			err = otgBusRequest();	//	Request the host role

			if (KErrNone != err)
				{
				test.Printf(_L("BusRequest returned %d\n"),err);
				OstTrace1(TRACE_NORMAL, CTESTCASE0678_RUNSTEPL_DUP05, "BusRequest returned %d\n",err);
				return TestFailed(KErrAbort, _L("BusRequest() failed!"));
				}
		
			iCaseStep = ESwappedRoles;
			SetActive();
			break;
			}
			
		case ESwappedRoles:
			{
			test.Printf(_L("Into ESwappedRoles step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0678_RUNSTEPL_DUP06, "Into ESwappedRoles step...\n");
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}			

			if (gTestRoleMaster)
				{
				//	B-Device should now wait until it is back to B-Peripheral
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateBPeripheral);
				iCaseStep = EDefaultRoles;
				}
			else
				{
				//	A-Device should wait to become a configured A-Peripheral
				iCollector.AddRequiredNotification(EWatcherPeripheralState, EUsbcDeviceStateConfigured);
				iCaseStep = EAConfigured;
				}
			SetActive();
			break;
			}
			
		case EAConfigured:	//	A-Device only step
			{
			test.Printf(_L("Into EWaitTillAConfigured step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0678_RUNSTEPL_DUP07, "Into EWaitTillAConfigured step...\n");
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}			

			iCollector.AddRequiredNotification(EWatcherPeripheralState, EUsbcDeviceStateSuspended);
			iCaseStep = EASuspended;
			SetActive();
			break;
			}
		
		case EASuspended:	//	A-Device only step
			{
			test.Printf(_L("Into EWaitTillASuspended step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0678_RUNSTEPL_DUP08, "Into EWaitTillASuspended step...\n");
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}			
			
			iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateAHost);	//	Swapping back to default role

			iCaseStep = EDefaultRoles;
			SetActive();
			break;
			}

		case EDropVBus:
			LOG_STEPNAME(_L("EDropVBus"))

			iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventVbusDropped);
			if ( gTestRoleMaster)
				{
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateBIdle);
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventRoleChangedToDevice);
				}
			else
				{ // SLAVE "A"
				otgBusDrop();
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateAIdle);
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventRoleChangedToHost);
				}
			iCaseStep = EVBusDropped;
			SetActive();
			break;
			
		case EVBusDropped:
			LOG_STEPNAME(_L("ELoopVerifyDrop"))
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
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
			otgDeactivateFdfActor();
			iCollector.DestroyObservers();
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
			OstTrace0(TRACE_NORMAL, CTESTCASE0678_RUNSTEPL_DUP09, "<Error> unknown test step");
			Cancel();
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));
		}
	}

