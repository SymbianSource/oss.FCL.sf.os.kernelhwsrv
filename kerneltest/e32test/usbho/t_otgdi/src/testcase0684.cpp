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
#include "testcase0684.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase0684Traces.h"
#endif

#include <e32debug.h> 

#define _REPEATS (oOpenIterations*3)


/* **************************************************************************************
 * the name below is used to add a pointer to our construction method to a pointer MAP in 
 * the class factory
 */
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0684");
const TTestCaseFactoryReceipt<CTestCase0684> CTestCase0684::iFactoryReceipt(KTestCaseId);	

CTestCase0684* CTestCase0684::NewL(TBool aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0684_NEWL);
	    }
	CTestCase0684* self = new (ELeave) CTestCase0684(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0684::CTestCase0684(TBool aHost)
	: CTestCaseB2BRoot(KTestCaseId, aHost, iStatus) 
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0684_CTESTCASE0684);
	    }
		
	} 


/**
 ConstructL
*/
void CTestCase0684::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0684_CONSTRUCTL);
	    }

	iDualRoleCase = ETrue;
	iBusRequestCounter = 2;
	iIsTimeToDrop = EFalse;
	
	BaseConstructL();
	}


CTestCase0684::~CTestCase0684()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0684_DCTESTCASE0684);
	    }
	iCollector.DestroyObservers();
	Cancel();
	}


void CTestCase0684::ExecuteTestCaseL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0684_EXECUTETESTCASEL);
	    }
	iCaseStep = EPreconditions;
	CActiveScheduler::Add(this);
	SelfComplete();
	}

	
void CTestCase0684::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0684_DOCANCEL);
	    }
	// cancel our timer
	iTimer.Cancel();
	}
void CTestCase0684::RunStepL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0684_RUNSTEPL);
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
			
			if (!StepLoadClient(0xF684/*use default settings for SRP/HNP support*/))
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
			iCollector.ClearAllEvents();
			iCaseStep = ESetMaxPower2Zero;
			SelfComplete();
			break;
			}
			
		case ESetMaxPower2Zero:
			LOG_STEPNAME(_L("ESetMaxPower2Zero"))
			iCaseStep = EReadyToRaiseVBus;
			SetMaxPowerToL(0);
			SelfComplete();
			break;
			
		case EReadyToRaiseVBus:
			{
			test.Printf(_L("Into EReadyToRaiseVBus step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0684_RUNSTEPL_DUP01, "Into EReadyToRaiseVBus step...\n");
			
			if (gTestRoleMaster)
				{
				// B device, so , just wait for Vbus to be raised
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateBIdle);
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventVbusRaised);
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventHnpEnabled);
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateBPeripheral);
				//	All as usual - we should report a failure if the B-Device ever reports "busy"
				iCollector.AddFailureNotification(EWatcherAConnectionIdle, RUsbOtgDriver::EConnectionBusy);
				}
			else
				{ 
				// A device, Raise VBus, then wait for default role
				err = otgBusRequest();	// ok to turn on VBus now
				
				if (KErrNone != err)
					{
					return TestFailed(KErrAbort, _L("Raise Vbus - RUsbOtgDriver::BusRequest() FAILED!"));
					}
				
				// we might also wait for and EStateAIdle
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateAIdle);
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventVbusRaised);
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventRoleChangedToHost);
				iCollector.AddRequiredNotification(EWatcherAConnectionIdle, RUsbOtgDriver::EConnectionBusy);
				}
			
			iCaseStep = EDefaultRoles;
			const TInt KTestCase0684Timeout = 30000;			//	30 seconds, should be plenty of time for 3 role swaps
			iCollector.AddStepTimeout(KTestCase0684Timeout);
			SetActive();
			break;
			}
			
		case EDefaultRoles:
			{
			test.Printf(_L("Into EDefaultRoles step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0684_RUNSTEPL_DUP02, "Into EDefaultRoles step...\n");
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}			
						
			if(gTestRoleMaster){
				// B
				iCollector.AddRequiredNotification(EWatcherPeripheralState, EUsbcDeviceStateConfigured);
				iCaseStep = EBConfigured;
			}
			else{
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateAPeripheral);
				iCaseStep = EAToPeripheral;
			}
			SetActive();
			break;
			}
			
		case EAIdleHostPriorToAPeripheral:	//	an "A-Device only" step
			{
			test.Printf(_L("Into EAIdleHostPriorToAPeripheral step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0684_RUNSTEPL_DUP03, "Into EAIdleHostPriorToAPeripheral step...\n");
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}			
				
			//	A-Device should expect nothing more until it becomes A-Peripheral
			iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateAPeripheral);
			iCollector.AddRequiredNotification(EWatcherAConnectionIdle, RUsbOtgDriver::EConnectionBusy);
			iCollector.AddRequiredNotification(EWatcherPeripheralState, EUsbcDeviceStateConfigured);
			iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateAHost);
			iCaseStep = EAToHost;
			SetActive();
			break;
			}	
			
		case EBConfigured:
			{
			test.Printf(_L("Into EBConfigured step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0684_RUNSTEPL_DUP04, "Into EBConfigured step...\n");
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				OstTrace0(TRACE_NORMAL, CTESTCASE0684_RUNSTEPL_DUP05, "Timeout");
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
			OstTrace0(TRACE_NORMAL, CTESTCASE0684_RUNSTEPL_DUP06, "Into EBSuspended step...\n");					
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				OstTrace0(TRACE_NORMAL, CTESTCASE0684_RUNSTEPL_DUP07, "Timeout");
				return TestFailed(KErrAbort, _L("Timeout"));
				}
			
			if(iBusRequestCounter > 0){
				test.Printf(_L("VBus present, attempting a swap.\n"));
				OstTrace0(TRACE_NORMAL, CTESTCASE0684_RUNSTEPL_DUP08, "VBus present, attempting a swap.\n");
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateBHost);
				
				iBusRequestCounter --;
				err = otgBusRequest();	//	Request the host role
				if (KErrNone != err)
					{
					test.Printf(_L("BusRequest returned %d\n"),err);
					OstTrace1(TRACE_NORMAL, CTESTCASE0684_RUNSTEPL_DUP09, "BusRequest returned %d\n",err);
					return TestFailed(KErrAbort, _L("BusRequest() failed!"));
					}
				iCaseStep = EBToHost;
			}
			else{
				iCollector.AddRequiredNotification(EWatcherAConnectionIdle, RUsbOtgDriver::EConnectionIdle);
				iCaseStep = EIdleHostPriorToVBusDown;
			}	
			SetActive();
			break;
			}
			
		case EAToHost:
			{
			test.Printf(_L("Into EAToHost step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0684_RUNSTEPL_DUP10, "Into EAToHost step...\n");
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}
			
			if( ! iIsTimeToDrop){
				iIsTimeToDrop = ETrue;
				SetMaxPowerToL(50);
				TUint16 val = 0;
				GetMaxPower(val);
				test.Printf(_L("bMaxPower= %d\n"), val);
				OstTrace1(TRACE_NORMAL, CTESTCASE0684_RUNSTEPL_DUP11, "bMaxPower= %d\n", val);
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateAPeripheral);
				iCaseStep = EAToPeripheral;
			}
			else{
				iCollector.AddRequiredNotification(EWatcherAConnectionIdle, RUsbOtgDriver::EConnectionIdle);
				iCaseStep = EIdleHostPriorToVBusDown;
			}
			
			SetActive();
			break;			
			}
			
		case EAToPeripheral:
			{
			test.Printf(_L("Into EAToPeripheral step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0684_RUNSTEPL_DUP12, "Into EAToPeripheral step...\n");
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}
			iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateAHost);
			iCaseStep = EAToHost;
			SetActive();
			break;
			}
		
		case EBToHost:
			{
			test.Printf(_L("Into EBToHost step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0684_RUNSTEPL_DUP13, "Into EBToHost step...\n");
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}
			iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateBPeripheral);
			iCaseStep = EBToPeripheral;
			SetActive();
			break;
			}
			
		case EBToPeripheral:
			{
			test.Printf(_L("Into EBToPeripheral step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0684_RUNSTEPL_DUP14, "Into EBToPeripheral step...\n");
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}
			if(iBusRequestCounter > 0)
				{
				iCollector.AddRequiredNotification(EWatcherPeripheralState, EUsbcDeviceStateConfigured);
				iCaseStep = EBConfigured;
				}
			else
				{
				iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventVbusDropped);
				iCaseStep = EVBusDropped;
				}
			SetActive();
			break;
			}
			
		case EIdleHostPriorToVBusDown:
			{
			test.Printf(_L("Into EAIdleHostPriorToVBusDown step...\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0684_RUNSTEPL_DUP15, "Into EAIdleHostPriorToVBusDown step...\n");
			
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}			
	
			iCaseStep = EDropVBus;
			SelfComplete();
			break;
			}
			
		case EDropVBus:
			LOG_STEPNAME(_L("EDropVBus"))
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				iCollector.DestroyObservers();
				return TestFailed(KErrAbort, _L("Timeout"));
				}			

			iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventVbusDropped);
			if ( gTestRoleMaster)
				{
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateBIdle);
				}
			else
				{ // SLAVE "A"
				otgBusDrop();
				iCollector.AddRequiredNotification(EWatcherState, RUsbOtgDriver::EStateAIdle);
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
			if (EFalse == StepUnloadLDD()){
				return TestFailed(KErrAbort,_L("unload Ldd failure"));
			}
			if (!StepUnloadClient()){
				return TestFailed(KErrAbort,_L("Client Unload Failure"));
			}

			iCaseStep = ELastStep;
			SelfComplete();
			break;
			
		case ELastStep:
			LOG_STEPNAME(_L("ELastStep"))
			TestPassed();
			RequestCharacter();	
			break;
			
		default:
			test.Printf(_L("<Error> unknown test step"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0684_RUNSTEPL_DUP16, "<Error> unknown test step");
			Cancel();
			RequestCharacter();	
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));
		}
	
	}
