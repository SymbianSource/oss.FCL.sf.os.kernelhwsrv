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
#include "testcase0462.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase0462Traces.h"
#endif



// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0462");
const TTestCaseFactoryReceipt<CTestCase0462> CTestCase0462::iFactoryReceipt(KTestCaseId);	

CTestCase0462* CTestCase0462::NewL(TBool aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0462_NEWL);
	    }
	CTestCase0462* self = new (ELeave) CTestCase0462(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0462::CTestCase0462(TBool aHost)
:	CTestCaseRoot(KTestCaseId, aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0462_CTESTCASE0462);
	    }
		
	} 


/**
 ConstructL
*/
void CTestCase0462::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0462_CONSTRUCTL);
	    }
	iRepeats = KOperationRetriesMax;
	iWDTimer = CTestCaseWatchdog::NewL();
	
	BaseConstructL();
	}


CTestCase0462::~CTestCase0462()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0462_DCTESTCASE0462);
	    }

	Cancel();
	delete iWDTimer;
	}


void CTestCase0462::ExecuteTestCaseL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0462_EXECUTETESTCASEL);
	    }
	iCaseStep = ELoadLdd;
	
	CActiveScheduler::Add(this);
	SelfComplete();

	}


void CTestCase0462::DescribePreconditions()
	{
	test.Printf(_L("Insert 'A' connector beforehand.\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0462_DESCRIBEPRECONDITIONS, "Insert 'A' connector beforehand.\n");
	}

	
void CTestCase0462::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0462_DOCANCEL);
	    }

	// cancel our timer
	iTimer.Cancel();
	}
	
void CTestCase0462::CancelKB(CTestCaseRoot *pThis)
	{
	CTestCase0462 * p = REINTERPRET_CAST(CTestCase0462 *,pThis);
	// cancel any pending call, and then complete our active obj with a timeout value

	p->iConsole->ReadCancel();
	
	}	
	

// handle event completion	
void CTestCase0462::RunStepL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0462_RUNSTEPL);
	    }
	// Obtain the completion code for this CActive obj.
	TInt completionCode(iStatus.Int()); 
	// NOTE: Look at its iStatus.iFlags. 
	// If it's 1, it's b -> you called SetActive() but the service provider didn't set it to KRequestPending or it got overwritten. 
	// If it's 2, it's a -> you didn't called SetActive() when issuing a request.				
	
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
			// prompt to insert connector
			test.Printf(KInsertAConnectorPrompt);
			OstTrace0(TRACE_NORMAL, CTESTCASE0462_RUNSTEPL_DUP01, KInsertAConnectorPrompt);
			test.Printf(KPressAnyKeyToContinue);
			OstTrace0(TRACE_NORMAL, CTESTCASE0462_RUNSTEPL_DUP02, KPressAnyKeyToContinue);
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

			// 2. wait on ID_PIN
		case ERegisterForEvents:
			// prompt to remove connector
			test.Printf(KRemoveAConnectorPrompt);		
			OstTrace0(TRACE_NORMAL, CTESTCASE0462_RUNSTEPL_DUP03, KRemoveAConnectorPrompt);		
			if (iDequeAttempts > 3)
				{
				return (TestFailed(KErrCorrupt, _L("<Error> too many irrelevant/incorrect events")));
				}

			otgQueueOtgEventRequest( iOTGEvent, iStatus);
			iCaseStep = EEmptyQueue;
			iIDcheckStart.HomeTime();
			SetActive();
			break;

			// 3. pick up events already buffered when we started test
		case EEmptyQueue: 
			{
			
			TInt aMillisec;
			iIDcheckEnd.HomeTime();
			TTimeIntervalMicroSeconds ivlMicro(iIDcheckEnd.MicroSecondsFrom(iIDcheckStart));
			aMillisec = (TInt)(ivlMicro.Int64())/1000;	// USB times are in uSec, but in ms for the user layer
			iCaseStep = EGetAndCancelEvent;
			// test if too quick!
			if(aMillisec < KDelayDurationForQEmpty) // use 200ms - clocked at 17ms
				{
				iCaseStep = ERegisterForEvents;
				iDequeAttempts++;
				SelfComplete();
				break;
				}
			}// drop through to next step
			
			// 4.
		case EGetAndCancelEvent:
			OtgEventString(iOTGEvent, aDescription);
			test.Printf(_L("Received event %d '%S' status(%d)"), iOTGEvent, &aDescription, completionCode);
			OstTraceExt3(TRACE_NORMAL, CTESTCASE0462_RUNSTEPL_DUP04, "Received event %d '%S' status(%d)", iOTGEvent, aDescription, completionCode);
			if (RUsbOtgDriver::EEventAPlugRemoved == iOTGEvent)
				{
				otgQueueOtgEventRequest( iOTGEvent, iStatus);
				// cancel it and then generate 6 more events
				otgCancelOtgEventRequest();
				// swallow the cancellation now too
				User::WaitForRequest(iStatus);

				iCaseStep = ECancelNotify;
				}
			else
				{			
				// wrong event in the Q already, keep at it
				iCaseStep = ERegisterForEvents;	
				iDequeAttempts++;
				}
			SelfComplete();
			break;		

			// 5. prepare to insert 'A'
		case ECancelNotify:  
			iRepeats = KOperationRetriesMax;
			iCaseStep = EInsertA;
			SelfComplete();
			break;
			
		case EInsertA: // 6. remove 'A'
			iConsole->ReadCancel();
			test.Printf(KInsertAConnectorPrompt);
			OstTrace0(TRACE_NORMAL, CTESTCASE0462_RUNSTEPL_DUP05, KInsertAConnectorPrompt);

			iCaseStep = ERemoveA;
			test.Printf(KPressAnyKeyToContinue);
			OstTrace0(TRACE_NORMAL, CTESTCASE0462_RUNSTEPL_DUP06, KPressAnyKeyToContinue);
			RequestCharacter();			
			break;
			
			// 7.
		case ERemoveA:
			iConsole->ReadCancel();
			test.Printf(KRemoveAConnectorPrompt);
			OstTrace0(TRACE_NORMAL, CTESTCASE0462_RUNSTEPL_DUP07, KRemoveAConnectorPrompt);
			if (iRepeats-- >0)
				iCaseStep = EInsertA;
			else
				iCaseStep = ETallyEvents;
			test.Printf(KPressAnyKeyToContinue);
			OstTrace0(TRACE_NORMAL, CTESTCASE0462_RUNSTEPL_DUP08, KPressAnyKeyToContinue);
			RequestCharacter();
			break;
			
			// 8.
		case ETallyEvents:
			iConsole->ReadCancel();
			// tests if the object became signalled or not!
			// we expect if it did in fact do so, our Active object would panic (stray signal)
			SelfComplete();
			iCaseStep = EUnloadLdd;
			break;
			
			// 9.
		case EUnloadLdd:
		case ELastStep:
			if (EFalse == StepUnloadLDD())
				return TestFailed(KErrAbort,_L("unload Ldd failure"));	
			
			return TestPassed();

		default:
			test.Printf(_L("<Error> unknown test step"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0462_RUNSTEPL_DUP09, "<Error> unknown test step");
			Cancel();
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));

		}
	}


	
