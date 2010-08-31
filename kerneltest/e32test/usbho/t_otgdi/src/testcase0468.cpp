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
#include "testcasewd.h"
#include "testcase0468.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase0468Traces.h"
#endif

#define _REPEATS (oOpenIterations*3)



// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0468");
const TTestCaseFactoryReceipt<CTestCase0468> CTestCase0468::iFactoryReceipt(KTestCaseId);	

CTestCase0468* CTestCase0468::NewL(TBool aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0468_NEWL);
	    }
	CTestCase0468* self = new (ELeave) CTestCase0468(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0468::CTestCase0468(TBool aHost)
:	CTestCaseRoot(KTestCaseId, aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0468_CTESTCASE0468);
	    }
		
	} 


/**
 ConstructL
*/
void CTestCase0468::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0468_CONSTRUCTL);
	    }
	iWDTimer = CTestCaseWatchdog::NewL();
	iRepeats = OPEN_REPEATS;
		
	BaseConstructL();
	}


CTestCase0468::~CTestCase0468()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0468_DCTESTCASE0468);
	    }

	Cancel();
	delete iWDTimer;
	
	}


void CTestCase0468::ExecuteTestCaseL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0468_EXECUTETESTCASEL);
	    }
	iCaseStep = EPreconditions;
	
	iRepeats = KOperationRetriesMax;	// VBus event rise retries
	
	CActiveScheduler::Add(this);
	SelfComplete();

	}

	
void CTestCase0468::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0468_DOCANCEL);
	    }

	// cancel our timer
	iTimer.Cancel();
	}


void CTestCase0468::CancelKB(CTestCaseRoot *pThis)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0468_CANCELKB);
	    }
	CTestCase0468 * p = REINTERPRET_CAST(CTestCase0468 *,pThis);
	// cancel any pending call, and then complete our active obj with a cancel value
	p->iConsole->ReadCancel();

	}


void CTestCase0468::CancelNotify(CTestCaseRoot *pThis)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0468_CANCELNOTIFY);
	    }
	CTestCase0468 * p = REINTERPRET_CAST(CTestCase0468 *,pThis);
	// cancel any pending call, and then complete our active obj with a timeout value
	p->otgCancelOtgVbusNotification();
	p->SelfComplete(KTestCaseWatchdogTO);
	}


// This test result depends on all the ID detection tests and the VBus driving and dropping tests have not yet passed
void CTestCase0468::DescribePreconditions()
	{
	test.Printf(_L("Insert 'A' connector beforehand.\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0468_DESCRIBEPRECONDITIONS, "Insert 'A' connector beforehand.\n");
	test.Printf(_L("ID_PIN detection and VBus driving tests must already pass.\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0468_DESCRIBEPRECONDITIONS_DUP01, "ID_PIN detection and VBus driving tests must already pass.\n");
	}

void CTestCase0468::ContinueAfter(TTimeIntervalMicroSeconds32 aMicroSecs, TCaseSteps aStep)
	{
	LOG_VERBOSE2(_L("Wait %dms before drop VBus"), (TInt)(aMicroSecs.Int()/1000));
	if(gVerboseOutput)
	    {
	    OstTrace1(TRACE_VERBOSE, CTESTCASE0468_CONTINUEAFTER, "Wait %dms before drop VBus", (TInt)(aMicroSecs.Int()/1000));;
	    }
	iTimer.After(iStatus, aMicroSecs);
	iCaseStep = aStep;
	SetActive();
	}

// handle event completion	
void CTestCase0468::RunStepL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0468_RUNSTEPL);
	    }
	// Obtain the completion code for this CActive obj.
	TInt completionCode(iStatus.Int()); 
	TBuf<MAX_DSTRLEN> aDescription;
	TInt err(0);


	switch(iCaseStep)
		{
		case EPreconditions:
			iCaseStep = ELoadLdd;
			if (iAutomated)
				{
				iCaseStep = ELoadLdd;
				SelfComplete();
				break;
				}
			// prompt to insert connector
			test.Printf(KInsertAConnectorPrompt);
			OstTrace0(TRACE_NORMAL, CTESTCASE0468_RUNSTEPL_DUP01, KInsertAConnectorPrompt);
			test.Printf(KPressAnyKeyToContinue);
			OstTrace0(TRACE_NORMAL, CTESTCASE0468_RUNSTEPL_DUP02, KPressAnyKeyToContinue);
			RequestCharacter();			
			break;
			
		case ELoadLdd:
			// 1. load the LDD and init.
			if (!StepLoadLDD())
				{
				break;
				}
			iCaseStep = EDetectAPlug;
			SelfComplete();
			break;
			
			// 2. detect 'A' plug now
		case EDetectAPlug:
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				return TestFailed(KErrAbort, _L("User response too slow - FAILED!"));
				}

			// if doing this test in /AUTO mode, we would fail this now
			// however if we can control ID_PIN through an API in future, we turn in on now.
			if (!otgIdPinPresent())
				{
				test.Printf(KInsertAConnectorPrompt);
				OstTrace0(TRACE_NORMAL, CTESTCASE0468_RUNSTEPL_DUP03, KInsertAConnectorPrompt);
				test.Printf(KPressAnyKeyToContinue);
				OstTrace0(TRACE_NORMAL, CTESTCASE0468_RUNSTEPL_DUP04, KPressAnyKeyToContinue);
				RequestCharacter();

				iCaseStep = EDetectAPlug;
				}
			else
				{
				iCaseStep = ELoopDriveVBus;
				SelfComplete();
				}
			break;
			
			// 3. Control/branch step in the loop
		case ELoopControl:
			if (--iRepeats)
				{
				iCaseStep = ELoopDriveVBus;
				}
			else
				{
				iCaseStep = EUnloadLdd;
				}
			SelfComplete();
			break;
			
		case ELoopDriveVBus:
			// 4. DRIVE VBUS
			iWDTimer->Cancel();
			test.Printf(_L("Drive VBus, iteration %d/%d\n"), OPEN_REPEATS-iRepeats+1, OPEN_REPEATS);
			OstTraceExt2(TRACE_NORMAL, CTESTCASE0468_RUNSTEPL_DUP05, "Drive VBus, iteration %d/%d\n", OPEN_REPEATS-iRepeats+1, OPEN_REPEATS);
			// test for VBus rise next
			test.Printf(_L("Waiting for VBus Event\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0468_RUNSTEPL_DUP06, "Waiting for VBus Event\n");
			iStatus = KRequestPending;
			otgQueueOtgVbusNotification( iOTGVBus, iStatus );

			// turn on VBus, since the call is not a Queing a-sync call we do this after the async call
			err = otgBusRequest();	// ok to turn on VBus now
			if (KErrNone != err)
				{
				return TestFailed(KErrAbort, _L("Raise Vbus - RUsbOtgDriver::BusRequest() FAILED!"));
				}
			iCaseStep = ELoopVerifyVBus;
			iWDTimer->IssueRequest(KDelayDurationForLocalTrigger, this, &CancelNotify);
			SetActive();
			break;
			
		case ELoopVerifyVBus:
			// 5. get VBus rise event
			LOG_VERBOSE2(_L("Post notification val=%d\n\n"), iOTGVBus);
			if(gVerboseOutput)
			    {
			    OstTrace1(TRACE_VERBOSE, CTESTCASE0468_RUNSTEPL_DUP07, "Post notification val=%d\n\n", iOTGVBus);;
			    }
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				return TestFailed(KErrAbort, _L("Vbus rise not signalled in time - FAILED!"));
				}
			iWDTimer->Cancel();

			// fetch the current value
			otgQueueOtgVbusNotification( iOTGVBus, iStatus );
			otgCancelOtgVbusNotification();
			User::WaitForRequest(iStatus);

			if (iOTGVBus != RUsbOtgDriver::EVbusHigh)
				{
		
				return TestFailed(KErrAbort, _L("Vbus rise NOT 'seen' - FAILED!"));
				}

			// Double-check using the API in a syncronous way too
			// NOTE: once again this is to explore timing issues
			if (!otgVbusPresent())
				{
				return TestFailed(KErrAbort, _L("Vbus syncronous call error - FAILED!"));
				}
			iCaseStep = ELoopWait;
			SelfComplete();
			break;
			
		case ELoopWait:	// 6. wait 50ms after applying Vbus before we are allowed to 
			ContinueAfter(KDelayBeforeBusDropUs, ELoopDropVBus);
			break;
			
		case ELoopDropVBus:
			// 7. drop Bus
			otgQueueOtgVbusNotification(iOTGVBus, iStatus );
			// drop Vbus now, since the call is not a Queing a-sync call we do this after the async call
			otgBusDrop();
			SetActive();
			iCaseStep = ELoopVerifyDrop;
			iWDTimer->IssueRequest(KDelayDurationForLocalTrigger, this, &CancelNotify);
			break;
			
		case ELoopVerifyDrop:
			// 8. get Vbus low event
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				return TestFailed(KErrAbort, _L("Vbus drop not signalled in time - FAILED!"));
				}
			iWDTimer->Cancel();

			// fetch the value			
			otgQueueOtgVbusNotification( iOTGVBus, iStatus );
			otgCancelOtgVbusNotification();
			User::WaitForRequest(iStatus);

			if (iOTGVBus != RUsbOtgDriver::EVbusLow)
				{
				return TestFailed(KErrAbort, _L("Vbus drop NOT 'seen' - FAILED!"));
				}
				
			// test again using the 'syncronous' variation
			if (otgVbusPresent())
				{
				return TestFailed(KErrAbort, _L("Vbus syncronous call error - FAILED!"));
				}
				
			// wait 50ms and then go back to beginning
			ContinueAfter(KDelayBeforeBusDropUs, ELoopControl);
			break;
			
		case EUnloadLdd:
			// 9. unload
			if (EFalse == StepUnloadLDD())
				return TestFailed(KErrAbort,_L("unload Ldd failure"));	
			
			iCaseStep = ELastStep;
			SelfComplete();
			break;
			
		case ELastStep:
			TestPassed();
			break;
			
		default:
			test.Printf(_L("<Error> unknown test step"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0468_RUNSTEPL_DUP08, "<Error> unknown test step");
			Cancel();
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));
		}
	}


