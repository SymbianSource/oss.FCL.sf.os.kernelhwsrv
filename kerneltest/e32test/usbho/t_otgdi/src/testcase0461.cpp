// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// CTestCase0461.cpp
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
#include "testcase0461.h"



// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0461");
const TTestCaseFactoryReceipt<CTestCase0461> CTestCase0461::iFactoryReceipt(KTestCaseId);	

CTestCase0461* CTestCase0461::NewL(TBool aHost)
	{
	LOG_FUNC
	CTestCase0461* self = new (ELeave) CTestCase0461(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0461::CTestCase0461(TBool aHost)
:	CTestCaseRoot(KTestCaseId, aHost)
	{
	LOG_FUNC
		
	} 


/**
 ConstructL
*/
void CTestCase0461::ConstructL()
	{
	LOG_FUNC
	iRepeats = KOperationRetriesMax;
	BaseConstructL();
	
	iWDTimer = CTestCaseWatchdog::NewL();
	}


CTestCase0461::~CTestCase0461()
	{
	LOG_FUNC

	Cancel();
	delete iWDTimer;
	}


void CTestCase0461::ExecuteTestCaseL()
	{
	LOG_FUNC
	iCaseStep = ELoadLdd;
	
	CActiveScheduler::Add(this);
	SelfComplete();

	}


void CTestCase0461::DescribePreconditions()
	{
	test.Printf(_L("Insert 'A' connector beforehand.\n"));
	}

	
void CTestCase0461::DoCancel()
	{
	LOG_FUNC

	// cancel our timer
	iTimer.Cancel();
	}
	
void CTestCase0461::CancelKB(CTestCaseRoot *pThis)
	{
	CTestCase0461 * p = REINTERPRET_CAST(CTestCase0461 *,pThis);
	// cancel any pending call, and then complete our active obj with a timeout value

	p->iConsole->ReadCancel();
	
	}	

// handle event completion	
void CTestCase0461::RunStepL()
	{
	LOG_FUNC
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
			// prompt to insert connector
			test.Printf(KInsertAConnectorPrompt);
			test.Printf(KPressAnyKeyToContinue);
			RequestCharacter();			
			break;
			
		case ELoadLdd:
			if (!StepLoadLDD())
				{
				break;
				}

			iCaseStep = ERegisterForEvents;
			SelfComplete();
			iDequeAttempts = 0;	
			break;
			
			// 2. Prepare to wait on ID_PIN
		case ERegisterForEvents:
			// prompt to remove connector
			test.Printf(KRemoveAConnectorPrompt);		
			if (iDequeAttempts > 3)
				{
				return (TestFailed(KErrCorrupt, _L("<Error> too many irrelevant/incorrect events")));
				}

			otgQueueOtgEventRequest( iOTGEvent, iStatus);
			iIDcheckStart.HomeTime();
			iCaseStep = EGetAndCancelEvent;
			SetActive();
			break;
			
			// 3. pick up events already buffered when we start
		case EEmptyQueue:
			{
			
			TInt aMillisec;
			iIDcheckEnd.HomeTime();
			TTimeIntervalMicroSeconds ivlMicro(iIDcheckEnd.MicroSecondsFrom(iIDcheckStart));
			aMillisec = (TInt)(ivlMicro.Int64())/1000;	// USB times are in uSec, but in ms for the user layer
			iCaseStep = EGetAndCancelEvent;
			// test if too quick!
			if ((aMillisec < KDelayDurationForQEmpty) &&(!gSemiAutomated)) // use 200ms - clocked at 17ms
				{
				iCaseStep = ERegisterForEvents;
				iDequeAttempts++;
				SelfComplete();
				break;
				}
			}// drop through to next step
		case EGetAndCancelEvent: //4.
			OtgEventString(iOTGEvent, aDescription);
			test.Printf(_L("Received event %d '%S' status(%d)"), iOTGEvent, &aDescription, completionCode);
			if (RUsbOtgDriver::EEventAPlugRemoved == iOTGEvent)
				{
				otgQueueOtgEventRequest( iOTGEvent, iStatus);
				// cancel it and then go into a loop
				otgCancelOtgEventRequest();
				// swallow the cancelation now
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
			
			// 5. prepare to go into a loop
		case ECancelNotify:  // insert 'A'
			iRepeats = 3;	// #times to insert+remove
			iExpectedEvents = 0;
			iEventsInQueue = 0;
			iCaseStep = EInsertA;
			SelfComplete();
			break;
			
			// 6
		case EInsertA: // insert 'A' plug
			iWDTimer->Cancel();
			iConsole->ReadCancel();
			test.Printf(KInsertAConnectorPrompt);
			iCaseStep = ERemoveA;
			test.Printf(KPressAnyKeyToContinue);
			
			RequestCharacter();
			iWDTimer->IssueRequest(KDelayDurationForUserActivityMS, this, &CancelKB);			

			iExpectedEvents++;
			break;
			
			// 7. remove 'A' plug
		case ERemoveA:
			iWDTimer->Cancel();
			iConsole->ReadCancel();
			test.Printf(KRemoveAConnectorPrompt);
			if (iRepeats-- >0)
				{
				// Do it again please
				iCaseStep = EInsertA;
				}
			else
				{ // got enough events in Q, let's count them up
				iCaseStep = ETallyEvents;
				}
			test.Printf(KPressAnyKeyToContinue);

			RequestCharacter();			
			iWDTimer->IssueRequest(KDelayDurationForUserActivityMS, this, &CancelKB);			
			iExpectedEvents++;
			break;
			
			// 8.
		case ETallyEvents:
			// Getting here tests if the object does not get double-signalled.
			// a stray signal anywhere would Panic the scheduler when we self complete
			iCaseStep = ETallyNewEvent;
			iStatus = KRequestPending;
			otgQueueOtgEventRequest( iOTGEvent, iStatus);
			SetActive();
			break;
			// 9.
		case ETallyNewEvent:
			iEventsInQueue++;	// got one, add it
			if (iExpectedEvents == iEventsInQueue)
				{
				iCaseStep = EUnloadLdd;	
				}
			else
				{
				iCaseStep = ETallyEvents;
				}
			SelfComplete();
			break;
			
		case EUnloadLdd:
		case ELastStep:
			if (EFalse == StepUnloadLDD())
				return TestFailed(KErrAbort,_L("unload Ldd failure"));	
			
			return TestPassed();

		default:
			test.Printf(_L("<Error> unknown test step"));
			Cancel();
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));

		}
	}

	
