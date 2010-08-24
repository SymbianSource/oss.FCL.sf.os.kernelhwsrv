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
#include "testcase0675.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase0675Traces.h"
#endif

#define _REPEATS (oOpenIterations*3)



// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0675");
const TTestCaseFactoryReceipt<CTestCase0675> CTestCase0675::iFactoryReceipt(KTestCaseId);	

CTestCase0675* CTestCase0675::NewL(TBool aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0675_NEWL);
	    }
	CTestCase0675* self = new (ELeave) CTestCase0675(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0675::CTestCase0675(TBool aHost)
:	CTestCaseRoot(KTestCaseId, aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0675_CTESTCASE0675);
	    }
		
	} 


/**
 ConstructL
*/
void CTestCase0675::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0675_CONSTRUCTL);
	    }
	iWDTimer = CTestCaseWatchdog::NewL();
	iRepeats = OPEN_REPEATS;
		
	BaseConstructL();
	}


CTestCase0675::~CTestCase0675()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0675_DCTESTCASE0675);
	    }

	Cancel();
	delete iWDTimer;
	
	}


void CTestCase0675::ExecuteTestCaseL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0675_EXECUTETESTCASEL);
	    }
	iCaseStep = EPreconditions;
	
	iRepeats = KOperationRetriesMax;	// VBus event rise retries
	
	CActiveScheduler::Add(this);
	SelfComplete();

	}

	
void CTestCase0675::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0675_DOCANCEL);
	    }

	// cancel our timer
	iTimer.Cancel();
	}


void CTestCase0675::CancelKB(CTestCaseRoot *pThis)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0675_CANCELKB);
	    }
	CTestCase0675 * p = REINTERPRET_CAST(CTestCase0675 *,pThis);
	// cancel any pending call, and then complete our active obj with a cancel value
	p->iConsole->ReadCancel();

	}


void CTestCase0675::CancelNotify(CTestCaseRoot *pThis)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0675_CANCELNOTIFY);
	    }
	CTestCase0675 * p = REINTERPRET_CAST(CTestCase0675 *,pThis);
	// cancel any pending call, and then complete our active obj with a timeout value
	p->otgCancelOtgVbusNotification();
	p->SelfComplete(KTestCaseWatchdogTO);
	}


// This test result depends on all the ID detection tests and the VBus driving and dropping tests have not yet passed
void CTestCase0675::DescribePreconditions()
	{
	test.Printf(_L("Insert 'A' connector beforehand.\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0675_DESCRIBEPRECONDITIONS, "Insert 'A' connector beforehand.\n");
	
	}

void CTestCase0675::ContinueAfter(TTimeIntervalMicroSeconds32 aMicroSecs, TCaseSteps aStep)
	{
	LOG_VERBOSE2(_L("Wait %dms before drop VBus"), (TInt)(aMicroSecs.Int()/1000));
	if(gVerboseOutput)
	    {
	    OstTrace1(TRACE_VERBOSE, CTESTCASE0675_CONTINUEAFTER, "Wait %dms before drop VBus", (TInt)(aMicroSecs.Int()/1000));;
	    }
	iTimer.After(iStatus, aMicroSecs);
	iCaseStep = aStep;
	SetActive();
	}

// handle event completion	
void CTestCase0675::RunStepL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0675_RUNSTEPL);
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
			OstTrace0(TRACE_NORMAL, CTESTCASE0675_RUNSTEPL_DUP01, KInsertAConnectorPrompt);
			test.Printf(KPressAnyKeyToContinue);
			OstTrace0(TRACE_NORMAL, CTESTCASE0675_RUNSTEPL_DUP02, KPressAnyKeyToContinue);
			RequestCharacter();			
			break;
			
			// 1. load the LDD and init.
		case ELoadLdd:
			if (!StepLoadLDD())
				{
				break;
				}
			if (iAutomated)
				iCaseStep = ELoopDriveVBus1;
			else
				iCaseStep = EDetectAPlug;
			SelfComplete();
			break;
			
			// 2. detect 'A' plug now
		case EDetectAPlug:
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				return TestFailed(KErrAbort, _L("User response too slow - FAILED!"));
				}
			if (!otgIdPinPresent())
				{
				test.Printf(KInsertAConnectorPrompt);
				OstTrace0(TRACE_NORMAL, CTESTCASE0675_RUNSTEPL_DUP03, KInsertAConnectorPrompt);
				test.Printf(KPressAnyKeyToContinue);
				OstTrace0(TRACE_NORMAL, CTESTCASE0675_RUNSTEPL_DUP04, KPressAnyKeyToContinue);
				RequestCharacter();
				iCaseStep = EDetectAPlug;
				}
			else
				{
				iCaseStep = ELoopDriveVBus1;
				SelfComplete();
				}
			break;
			
			// 3. Control/branch step in the loop
		case ELoopControl:
			if (--iRepeats)
				{
				iCaseStep = ELoopDriveVBus1;
				}
			else
				{
				iCaseStep = EUnloadLdd;
				}
			SelfComplete();
			break;
			
			// 4. DRIVE VBUS
		case ELoopDriveVBus1:
			iWDTimer->Cancel();
			test.Printf(_L("Drive VBus, iteration %d/%d\n"), OPEN_REPEATS-iRepeats+1, OPEN_REPEATS);
			OstTraceExt2(TRACE_NORMAL, CTESTCASE0675_RUNSTEPL_DUP05, "Drive VBus, iteration %d/%d\n", OPEN_REPEATS-iRepeats+1, OPEN_REPEATS);
			// test for VBus rise next
			test.Printf(_L("Waiting for VBus Event\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0675_RUNSTEPL_DUP06, "Waiting for VBus Event\n");
			iStatus = KRequestPending;
			otgQueueOtgVbusNotification( iOTGVBus, iStatus );
			SetActive();
			
			// turn on VBus, since the call is not a Queing a-sync call we do this after the async call
			err = otgBusRequest();	// ok to turn on VBus now
			if (KErrNone != err)
				{
				return TestFailed(KErrAbort, _L("Raise Vbus - RUsbOtgDriver::BusRequest() FAILED!"));
				}
			iCaseStep = ELoopVerifyVBus1;
			iWDTimer->IssueRequest(KDelayDurationForLocalTrigger, this, &CancelNotify);
	
			break;
			
			// 5. get VBus rise event
		case ELoopVerifyVBus1:
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				return TestFailed(KErrAbort, _L("Vbus rise not signalled in time - FAILED!"));
				}
			iWDTimer->Cancel();

			// check using the API in a syncronous way too
			if (!otgVbusPresent())
				{
				return TestFailed(KErrAbort, _L("Vbus syncronous call error - FAILED!"));
				}
			iCaseStep = ELoopWait;
			SelfComplete();
			break;

			// 6. DRIVE/claim VBUS 2nd time
		case ELoopDriveVBus2:
			iWDTimer->Cancel();
			test.Printf(_L("Drive VBus double, iteration %d/%d\n"), OPEN_REPEATS-iRepeats+1, OPEN_REPEATS);
			OstTraceExt2(TRACE_NORMAL, CTESTCASE0675_RUNSTEPL_DUP07, "Drive VBus double, iteration %d/%d\n", OPEN_REPEATS-iRepeats+1, OPEN_REPEATS);

			err = otgBusRequest();	// duplicate turn on VBus, we expect an error 
			if (KErrUsbOtgVbusAlreadyRaised != err)
				{
				return TestFailed(KErrAbort, _L("Raise Vbus - RUsbOtgDriver::BusRequest() unexpected result!"));
				}
			iCaseStep = ELoopVerifyVBus2;
			SelfComplete();
			SetActive();
			break;
			
			// 7 - make sure that the error did not end up killing the bus
		case ELoopVerifyVBus2:
			if (!otgVbusPresent())
				{
				return TestFailed(KErrAbort, _L("Raise Vbus twice resulted in session drop"));
				}
			iCaseStep = ELoopWait;
			SelfComplete();
			SetActive();
			break;
			
			// 9. wait 50ms after applying Vbus before we are allowed to drop again
		case ELoopWait:	
			ContinueAfter(KDelayBeforeBusDropUs, ELoopDropVBus);
			break;
			
		case ELoopDropVBus:
			// 10. drop Bus
			otgQueueOtgVbusNotification(iOTGVBus, iStatus );
			// drop Vbus now, since the call is not a Queing a-sync call we do this after the async call
			otgBusDrop();
			SetActive();
			iCaseStep = ELoopVerifyDrop;
			iWDTimer->IssueRequest(KDelayDurationForLocalTrigger, this, &CancelNotify);
			break;
			
		case ELoopVerifyDrop:
			// 11. get Vbus low event
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
			// 12. unload
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
			OstTrace0(TRACE_NORMAL, CTESTCASE0675_RUNSTEPL_DUP08, "<Error> unknown test step");
			Cancel();
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));
		}
	}


