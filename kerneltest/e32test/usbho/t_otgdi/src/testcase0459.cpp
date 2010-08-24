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
// 'A' connector detection
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <u32std.h> 	// unicode builds
#include <e32base.h>
#include <e32base_private.h>
#include <e32Test.h>	// RTest headder
#include "testcaseroot.h"
#include "testcasefactory.h"
#include "testcase0459.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase0459Traces.h"
#endif



// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0459");
const TTestCaseFactoryReceipt<CTestCase0459> CTestCase0459::iFactoryReceipt(KTestCaseId);	

CTestCase0459* CTestCase0459::NewL(TBool aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0459_NEWL);
	    }
	CTestCase0459* self = new (ELeave) CTestCase0459(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0459::CTestCase0459(TBool aHost)
:	CTestCaseRoot(KTestCaseId, aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0459_CTESTCASE0459);
	    }
		
	} 


/**
 ConstructL
*/
void CTestCase0459::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0459_CONSTRUCTL);
	    }
	
	BaseConstructL();
	}


CTestCase0459::~CTestCase0459()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0459_DCTESTCASE0459);
	    }

	Cancel();
	}


void CTestCase0459::ExecuteTestCaseL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0459_EXECUTETESTCASEL);
	    }
	iCaseStep = EPreconditions;
	
	CActiveScheduler::Add(this);
	SelfComplete();

	}


void CTestCase0459::DescribePreconditions()
	{
	test.Printf(_L("Remove 'A' connector beforehand.\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0459_DESCRIBEPRECONDITIONS, "Remove 'A' connector beforehand.\n");
	}

	
void CTestCase0459::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0459_DOCANCEL);
	    }

	// cancel our timer
	iTimer.Cancel();
	}
	

// handle event completion	
void CTestCase0459::RunStepL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0459_RUNSTEPL);
	    }
	// Obtain the completion code for this CActive obj.
	TInt completionCode(iStatus.Int()); 
	TBuf<MAX_DSTRLEN> aDescription;
		
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
			// prompt to remove connector
			test.Printf(KRemoveAConnectorPrompt);
			OstTrace0(TRACE_NORMAL, CTESTCASE0459_RUNSTEPL_DUP01, KRemoveAConnectorPrompt);
			test.Printf(KPressAnyKeyToContinue);
			OstTrace0(TRACE_NORMAL, CTESTCASE0459_RUNSTEPL_DUP02, KPressAnyKeyToContinue);
			RequestCharacter();			
			break;
			
		case ELoadLdd:
			if (!StepLoadLDD())
				{
				break;
				}

			iCaseStep = ERegisterForEvents;
			iDequeAttempts = 0;	
			SelfComplete();
			break;
			
			// wait on ID_PIN
		case ERegisterForEvents:
			if (iDequeAttempts > 3)
				{
				return (TestFailed(KErrCorrupt, _L("<Error> too many irrelevant/incorrect events")));
				}

			test.Printf(KInsertAConnectorPrompt);
			OstTrace0(TRACE_NORMAL, CTESTCASE0459_RUNSTEPL_DUP03, KInsertAConnectorPrompt);
			iCaseStep = ETestStateA;
			test.Printf(_L("Waiting for OTG Event\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0459_RUNSTEPL_DUP04, "Waiting for OTG Event\n");

			otgQueueOtgEventRequest( iOTGEvent, iStatus);

			// start timer
			iIDcheckStart.HomeTime();
			SetActive();
			break;
			
		case EDriveID_PIN:
			// please turn on ID_PIN prompt (or programming API call)
			// skipped untill we can do this.
			SetActive();
			break;
			
		case EWait5:
		case ETestStateA:
			{
			TInt aMillisec;
			OtgEventString(iOTGEvent, aDescription);

			iIDcheckEnd.HomeTime();
			TTimeIntervalMicroSeconds ivlMicro(iIDcheckEnd.MicroSecondsFrom(iIDcheckStart));
			aMillisec = (TInt)(ivlMicro.Int64())/1000;	// USB times are in uSec, but in ms for the user layer
			test.Printf(_L("Received event %d '%S' status(%d) in %d ms"), iOTGEvent, &aDescription, completionCode, aMillisec);
			OstTraceExt4(TRACE_NORMAL, CTESTCASE0459_RUNSTEPL_DUP05, "Received event %d '%S' status(%d) in %d ms", iOTGEvent, aDescription, completionCode, aMillisec);
			
			// check the parameters gathered
			if (RUsbOtgDriver::EEventAPlugInserted == iOTGEvent)
				{
				iCaseStep = EUnloadLdd;
				// test if too quick!
				if(aMillisec < KDelayDurationForQEmpty) // use 200ms - clocked at 17ms
					{
					// 'A' was in the receptacle when we started the stack, so it fires immediately, consume it and wait for another.
					test.Printf(_L("Please first remove and then replace the A connector.\n"));
					OstTrace0(TRACE_NORMAL, CTESTCASE0459_RUNSTEPL_DUP06, "Please first remove and then replace the A connector.\n");
					// wrong event in the Q already, keep at it
					iCaseStep = ERegisterForEvents;	
					}
				}
			else
				{
				// wrong event in the Q already, keep at it
				iCaseStep = ERegisterForEvents;	
				iDequeAttempts++;
				}
			SelfComplete();
			}
			break;		
		case EUnloadLdd:
		case ELastStep:
			if (EFalse == StepUnloadLDD())
				{
				return TestFailed(KErrAbort,_L("unload Ldd failure"));	
				}
			return TestPassed();

		default:
			test.Printf(_L("<Error> unknown test step"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0459_RUNSTEPL_DUP07, "<Error> unknown test step");
			Cancel();
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));

		}

	}

	
