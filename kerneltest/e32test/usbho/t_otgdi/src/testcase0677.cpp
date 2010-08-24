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
#include "testcase0677.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase0677Traces.h"
#endif

#define _REPEATS (oOpenIterations*3)



// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0677");
const TTestCaseFactoryReceipt<CTestCase0677> CTestCase0677::iFactoryReceipt(KTestCaseId);	

CTestCase0677* CTestCase0677::NewL(TBool aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0677_NEWL);
	    }
	CTestCase0677* self = new (ELeave) CTestCase0677(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0677::CTestCase0677(TBool aHost)
:	CTestCaseRoot(KTestCaseId, aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0677_CTESTCASE0677);
	    }
		
	} 


/**
 ConstructL
*/
void CTestCase0677::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0677_CONSTRUCTL);
	    }
	iWDTimer = CTestCaseWatchdog::NewL();
	iRepeats = OPEN_REPEATS;
		
	BaseConstructL();
	}


CTestCase0677::~CTestCase0677()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0677_DCTESTCASE0677);
	    }

	Cancel();
	delete iWDTimer;
	
	}


void CTestCase0677::ExecuteTestCaseL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0677_EXECUTETESTCASEL);
	    }
	iCaseStep = EPreconditions;
	
	iRepeats = KOperationRetriesMax;	// VBus event rise retries
	
	CActiveScheduler::Add(this);
	SelfComplete();

	}

	
void CTestCase0677::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0677_DOCANCEL);
	    }

	// cancel our timer
	iTimer.Cancel();
	}


void CTestCase0677::CancelKB(CTestCaseRoot *pThis)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0677_CANCELKB);
	    }
	CTestCase0677 * p = REINTERPRET_CAST(CTestCase0677 *,pThis);
	// cancel any pending call, and then complete our active obj with a cancel value
	p->iConsole->ReadCancel();

	}


void CTestCase0677::CancelNotify(CTestCaseRoot *pThis)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0677_CANCELNOTIFY);
	    }
	CTestCase0677 * p = REINTERPRET_CAST(CTestCase0677 *,pThis);
	// cancel any pending call, and then complete our active obj with a timeout value
	switch (p->iCancelWhat)
		{
			case ECancelEventNotify:
				p->otgCancelOtgEventRequest();
				break;			
			case ECancelMessageNotify: 
				p->otgCancelOtgMessageRequest();
				break;
			case ECancelVBusNotify:
				p->otgCancelOtgVbusNotification();
				break;	
			default:
				ASSERT(0);
				break;
		}
	p->SelfComplete(KTestCaseWatchdogTO);
	}


// This test result depends on all the ID detection tests and the VBus driving and dropping tests have not yet passed
void CTestCase0677::DescribePreconditions()
	{
	test.Printf(_L("Using OET, connect oscilloscope chan.A to VBus\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0677_DESCRIBEPRECONDITIONS, "Using OET, connect oscilloscope chan.A to VBus\n");
	test.Printf(_L("Connect oscilloscope chan.B to D+\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0677_DESCRIBEPRECONDITIONS_DUP01, "Connect oscilloscope chan.B to D+\n");
	test.Printf(_L("Trigger once, 200mV, 100ms \n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0677_DESCRIBEPRECONDITIONS_DUP02, "Trigger once, 200mV, 100ms \n");
	test.Printf(_L("Prepare to observe VBus, D+ pulse.\n\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0677_DESCRIBEPRECONDITIONS_DUP03, "Prepare to observe VBus, D+ pulse.\n\n");
	}


void CTestCase0677::ContinueAfter(TTimeIntervalMicroSeconds32 aMicroSecs, TCaseSteps aStep)
	{
	LOG_VERBOSE2(_L("Wait %dms before drop VBus"), (TInt)(aMicroSecs.Int()/1000));
	if(gVerboseOutput)
	    {
	    OstTrace1(TRACE_VERBOSE, CTESTCASE0677_CONTINUEAFTER, "Wait %dms before drop VBus", (TInt)(aMicroSecs.Int()/1000));;
	    }
	iTimer.After(iStatus, aMicroSecs);
	iCaseStep = aStep;
	SetActive();
	}


// handle event completion	
void CTestCase0677::RunStepL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0677_RUNSTEPL);
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
			test.Printf(KAttachOETAsBDevice);
			OstTrace0(TRACE_NORMAL, CTESTCASE0677_RUNSTEPL_DUP01, KAttachOETAsBDevice);
			test.Printf(KPressAnyKeyToContinue);
			OstTrace0(TRACE_NORMAL, CTESTCASE0677_RUNSTEPL_DUP02, KPressAnyKeyToContinue);
			RequestCharacter();			
			break;
			
			// 1. load the LDD and init.		
		case ELoadLdd:
			if (!StepLoadLDD())
				{
				break;
				}
			iCaseStep = EDetectBPlug;
			SelfComplete();
			break;

			// 2. detect 'B' plug now
		case EDetectBPlug:
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				return TestFailed(KErrAbort, _L("User response too slow - FAILED!"));
				}

			// if doing this test in /AUTO mode, we would fail this now
			// however if we can control ID_PIN through an API in future, we turn in on now.
			if (otgIdPinPresent())
				{
				test.Printf(KRemoveAConnectorPrompt);
				OstTrace0(TRACE_NORMAL, CTESTCASE0677_RUNSTEPL_DUP03, KRemoveAConnectorPrompt);
				test.Printf(KPressAnyKeyToContinue);
				OstTrace0(TRACE_NORMAL, CTESTCASE0677_RUNSTEPL_DUP04, KPressAnyKeyToContinue);
				RequestCharacter();

				iCaseStep = EDetectBPlug;
				}
			else
				{
				iCaseStep = ERequestBus;
				SelfComplete();
				}
			break;
						
			// 5. Issue SRP (request VBUS)
		case ERequestBus:
			iWDTimer->Cancel();

			test.Printf(KMsgWaitingForSRPInitiated);
			OstTrace0(TRACE_NORMAL, CTESTCASE0677_RUNSTEPL_DUP05, KMsgWaitingForSRPInitiated);
			otgQueueOtgEventRequest( iOTGEvent, iStatus );

			// turn on VBus (B-SRP)
			err = otgBusRequest();
			iTimeSRPStart.HomeTime();
			
			if (KErrNone != err)
				{
				return TestFailed(KErrAbort, _L("Issue SRP - RUsbOtgDriver::BusRequest() FAILED!"));
				}
			iCaseStep = EWaitForSRPInitiated;
			iCancelWhat = ECancelEventNotify;
			iWDTimer->IssueRequest(32000*2 /*KSpec_TA_SRP_RSPNS +1000*/, this, &CancelNotify);
			SetActive();
			break;
			
			// 6. get SRP initiated event, this is a local indication
		case EWaitForSRPInitiated:			
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				return TestFailed(KErrAbort, _L("SRP Active indication NOT fired in time - FAILED!"));
				}
			OtgEventString(iOTGEvent, aDescription);
			test.Printf(_L("Received event %d '%S' status(%d)\n"), iOTGEvent, &aDescription, completionCode);
			OstTraceExt3(TRACE_NORMAL, CTESTCASE0677_RUNSTEPL_DUP06, "Received event %d '%S' status(%d)\n", iOTGEvent, aDescription, completionCode);
			if (RUsbOtgDriver::EEventSrpInitiated == iOTGEvent)
				{
				// calc interval
				TTimeIntervalMicroSeconds aIntvlMicro;
				TTime aNowTime;
				aNowTime.HomeTime();
				aIntvlMicro = aNowTime.MicroSecondsFrom(iTimeSRPStart);
				LOG_VERBOSE2(_L("SRP active after %d ms\n"), (TInt)(aIntvlMicro.Int64()/1000));
				if(gVerboseOutput)
				    {
				    OstTrace1(TRACE_VERBOSE, CTESTCASE0677_RUNSTEPL_DUP07, "SRP active after %d ms\n", (TInt)(aIntvlMicro.Int64()/1000));;
				    }
				iCancelWhat = ECancelMessageNotify;
				otgQueueOtgMessageRequest( iOTGMessage, iStatus );
				iCaseStep = EWaitForSRPTimeout;
				test.Printf(KMsgWaitingForSRPTimeout);
				OstTrace0(TRACE_NORMAL, CTESTCASE0677_RUNSTEPL_DUP08, KMsgWaitingForSRPTimeout);
				SetActive();
				}
			else
				{
				iCaseStep = EWaitForSRPInitiated;
				test.Printf(KMsgWaitingForSRPInitiated);
				OstTrace0(TRACE_NORMAL, CTESTCASE0677_RUNSTEPL_DUP09, KMsgWaitingForSRPInitiated);
				iStatus = KRequestPending;
				otgQueueOtgEventRequest( iOTGEvent, iStatus );
				SetActive();
				}
			break;
				
			// 6. get SRP timeout event, this is a local indication
		case EWaitForSRPTimeout:			
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				return TestFailed(KErrAbort, _L("SRP T/O NOT fired in time - FAILED!"));
				}
			OtgMessageString(iOTGMessage, aDescription);
			test.Printf(_L("Received message %d '%S' status(%d)\n"), iOTGMessage, &aDescription, completionCode);
			OstTraceExt3(TRACE_NORMAL, CTESTCASE0677_RUNSTEPL_DUP10, "Received message %d '%S' status(%d)\n", iOTGMessage, aDescription, completionCode);
			if (RUsbOtgDriver::EMessageSrpTimeout == iOTGMessage)
				{
				iWDTimer->Cancel();		//	Only cancel WD Timer here, when timed portion of test is over
				// calc interval
				TTimeIntervalMicroSeconds aIntvlMicro;
				TTime aNowTime;
				aNowTime.HomeTime();
				aIntvlMicro = aNowTime.MicroSecondsFrom(iTimeSRPStart);
				LOG_VERBOSE2(_L("SRP times out after %d ms\n"), (TInt)(aIntvlMicro.Int64()/1000));
				if(gVerboseOutput)
				    {
				    OstTrace1(TRACE_VERBOSE, CTESTCASE0677_RUNSTEPL_DUP11, "SRP times out after %d ms\n", (TInt)(aIntvlMicro.Int64()/1000));;
				    }
				// the correct value is 32 seconds, not 4.9 seconds as per the spec.

				iCaseStep = EIssueSRPObservedPrompt;
				SelfComplete();
				}
			else
				{
				iCaseStep = EWaitForSRPTimeout;
				test.Printf(KMsgWaitingForSRPTimeout);
				OstTrace0(TRACE_NORMAL, CTESTCASE0677_RUNSTEPL_DUP12, KMsgWaitingForSRPTimeout);
				iStatus = KRequestPending;
				otgQueueOtgMessageRequest( iOTGMessage, iStatus );
				SetActive();
				}
			break;
				
		case EIssueSRPObservedPrompt:
			{
			test.Printf(_L("\nPress Y to verify that SRP was observed\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0677_RUNSTEPL_DUP13, "\nPress Y to verify that SRP was observed\n");
			test.Printf(_L("or any other key to fail test.\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0677_RUNSTEPL_DUP14, "or any other key to fail test.\n");
			RequestCharacter();
			iCaseStep = ECheckSRPObservedUserInput;
			break;
			}
			
							
		case ECheckSRPObservedUserInput:
			{
			if (('y' != iKeyCodeInput) && ('Y' != iKeyCodeInput))
				{
				return TestFailed(KErrAbort, _L("SRP NOT observed - FAILED!"));
				}
			iCaseStep = EUnloadLdd;
			SelfComplete();
			break;
			}
							
			// 12. unload OTG
		case EUnloadLdd:
			if (EFalse == StepUnloadLDD())
				return TestFailed(KErrAbort,_L("Unload Ldd failure"));	
			
			iCaseStep = ELastStep;
			SelfComplete();
			break;
			
		case ELastStep:
			TestPassed();
			break;
			
		default:
			test.Printf(_L("<Error> unknown test step\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0677_RUNSTEPL_DUP15, "<Error> unknown test step\n");
			Cancel();
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));
		}
	}


