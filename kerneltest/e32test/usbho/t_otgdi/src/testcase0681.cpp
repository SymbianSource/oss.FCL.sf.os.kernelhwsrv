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
#include "testcase0681.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase0681Traces.h"
#endif

#define _REPEATS (oOpenIterations*3)


/* **************************************************************************************
 * the name below is used to add a pointer to our construction method to a pointer MAP in 
 * the class factory
 */
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0681");
const TTestCaseFactoryReceipt<CTestCase0681> CTestCase0681::iFactoryReceipt(KTestCaseId);	

CTestCase0681* CTestCase0681::NewL(TBool aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0681_NEWL);
	    }
	CTestCase0681* self = new (ELeave) CTestCase0681(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0681::CTestCase0681(TBool aHost)
	: CTestCaseB2BRoot(KTestCaseId, aHost, iStatus) 
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0681_CTESTCASE0681);
	    }
		
	} 


/**
 ConstructL
*/
void CTestCase0681::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0681_CONSTRUCTL);
	    }

	iDualRoleCase = ETrue;	// another back-back
		
	BaseConstructL();
	}


CTestCase0681::~CTestCase0681()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0681_DCTESTCASE0681);
	    }
	iCollector.DestroyObservers();
	Cancel();
	}


void CTestCase0681::ExecuteTestCaseL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0681_EXECUTETESTCASEL);
	    }
	iCaseStep = EPreconditions;
	iHNPCounter = 3;	//	To be decremented to govern the number of times we do HNP.
	CActiveScheduler::Add(this);
	SelfComplete();
	}

	
void CTestCase0681::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0681_DOCANCEL);
	    }
	// cancel our timer
	iTimer.Cancel();
	}

void CTestCase0681::StepB2BPreconditions()
	{
	// prompt to insert connector and activate A-end first...
	if (gTestRoleMaster)
		{ // "B" device
		test.Printf(_L("***** Important note *****\n"));
		OstTrace0(TRACE_NORMAL, CTESTCASE0681_STEPB2BPRECONDITIONS, "***** Important note *****\n");
		test.Printf(_L("Before commencing test, please\n"));
		OstTrace0(TRACE_NORMAL, CTESTCASE0681_STEPB2BPRECONDITIONS_DUP01, "Before commencing test, please\n");
		test.Printf(_L("insert 'B'-cable end and activate\n"));
		OstTrace0(TRACE_NORMAL, CTESTCASE0681_STEPB2BPRECONDITIONS_DUP02, "insert 'B'-cable end and activate\n");
		test.Printf(_L("the test on the 'A' device.\n"));
		OstTrace0(TRACE_NORMAL, CTESTCASE0681_STEPB2BPRECONDITIONS_DUP03, "the test on the 'A' device.\n");
		test.Printf(_L("Then, press any key to continue.\n"));
		OstTrace0(TRACE_NORMAL, CTESTCASE0681_STEPB2BPRECONDITIONS_DUP04, "Then, press any key to continue.\n");
		test.Printf(_L("**************************\n"));
		OstTrace0(TRACE_NORMAL, CTESTCASE0681_STEPB2BPRECONDITIONS_DUP05, "**************************\n");
		}
	else
		{
		test.Printf(KInsertACablePrompt);
		OstTrace0(TRACE_NORMAL, CTESTCASE0681_STEPB2BPRECONDITIONS_DUP06, KInsertACablePrompt);
		test.Printf(KPressAnyKeyToContinue);
		OstTrace0(TRACE_NORMAL, CTESTCASE0681_STEPB2BPRECONDITIONS_DUP07, KPressAnyKeyToContinue);
		}

	RequestCharacter();	
	}

// handle event completion	
void CTestCase0681::RunStepL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0681_RUNSTEPL);
	    }
	// Obtain the completion code for this CActive obj.
	TInt completionCode(iStatus.Int()); 
	TBuf<MAX_DSTRLEN> aDescription;
	TInt err(0);

	if(completionCode != KErrNone)
		{
	
		switch(completionCode)
			{
			case KTestCaseFailureEventReceived:
				{
				TestFailed(KErrAbort, _L("Failure Event Received"));
				break;
				}
			case KTestCaseWatchdogTO:
				{
				TestFailed(KErrAbort, _L("Watchdog timer expired"));			
				break;
				}
			default:
				{
				TestFailed(KErrAbort, _L("Test step completed with Error"));
				break;
				}
			}
		iCollector.DestroyObservers();
		return;
		}

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
			if (!StepLoadClient(0xF681/*use default settings for SRP/HNP support*/))
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
			iCaseStep = EPerformSrp;
			SelfComplete();
			break;
			}
			
		case EPerformSrp:
			{
			test.Printf(_L("Into EPerformSrp step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0681_RUNSTEPL_DUP01, "Into EPerformSrp step...\n");

			if (gTestRoleMaster)
				{
				// Trigger SRP
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateBIdle);
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventSrpInitiated);
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventVbusRaised);	
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateBHost);
				
				//	We shouldn't get configured at any point however since ADevice will call 
				//	BusRespondSrp which should bypass that stage on the way to enabling us to
				//	role swap.
				iCollector.AddFailureNotification(EWatcherPeripheralState, EUsbcDeviceStateConfigured);

				err = otgBusRequest();	
				if (KErrNone != err)
					{
					return TestFailed(KErrAbort, _L("Raise Vbus - RUsbOtgDriver::BusRequest() FAILED!"));
					}

				iCaseStep = ESwappedRoles;
								}
			else
				{ // slave "A"
				// Wait for SRP
				
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventSrpReceived);
				
				iCaseStep = EAReceivedSrp;
				}
			const TInt KTestCase0681Timeout = 30000;			//	30 seconds, should be plenty of time for 3 role swaps
			iCollector.AddStepTimeout(KTestCase0681Timeout);
			SetActive();
			break;
			}

		case EAReceivedSrp:		//	A-Device step only!
			{
			test.Printf(_L("Into EAReceivedSrp step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0681_RUNSTEPL_DUP02, "Into EAReceivedSrp step...\n");

			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}			

			iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventVbusRaised);
			iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventRoleChangedToDevice);
			iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateAPeripheral);
			
			//	We've received SRP. We use BusRespondSRP to raise VBus and cede the host role
			err = otgBusRespondSRP();
			if (KErrNone != err)
				{
				return TestFailed(KErrAbort, _L("Raise Vbus - RUsbOtgDriver::BusRequest() FAILED!"));
				}

			iCaseStep = ESwappedRoles;
			SetActive();
			break;
			}

		case ESwappedRoles:
			{
			test.Printf(_L("Into ESwappedRoles step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0681_RUNSTEPL_DUP03, "Into ESwappedRoles step...\n");
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}			

			if (gTestRoleMaster)
				{
				//	B-Device should now wait until it is B-Peripheral
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
			test.Printf(_L("Into EAConfigured step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0681_RUNSTEPL_DUP04, "Into EAConfigured step...\n");
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
			test.Printf(_L("Into EASuspended step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0681_RUNSTEPL_DUP05, "Into EASuspended step...\n");
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
			
		case EDefaultRoles:
			{
			test.Printf(_L("Into EDefaultRoles step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0681_RUNSTEPL_DUP06, "Into EDefaultRoles step...\n");
			
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
			OstTrace0(TRACE_NORMAL, CTESTCASE0681_RUNSTEPL_DUP07, "Into EBConfigured step...\n");
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
			OstTrace0(TRACE_NORMAL, CTESTCASE0681_RUNSTEPL_DUP08, "Into EBSuspended step...\n");
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}			
			
			// issue HNP
			test.Printf(_L("VBus present, attempting a swap.\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0681_RUNSTEPL_DUP09, "VBus present, attempting a swap.\n");
			iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateBHost);

			err = otgBusRequest();	//	Request the host role

			if (KErrNone != err)
				{
				test.Printf(_L("BusRequest returned %d\n"),err);
				OstTrace1(TRACE_NORMAL, CTESTCASE0681_RUNSTEPL_DUP10, "BusRequest returned %d\n",err);
				return TestFailed(KErrAbort, _L("BusRequest() failed!"));
				}
		
			iCaseStep = ESwappedRoles;
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
			OstTrace0(TRACE_NORMAL, CTESTCASE0681_RUNSTEPL_DUP11, "<Error> unknown test step");
			Cancel();
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));
		}
	}
