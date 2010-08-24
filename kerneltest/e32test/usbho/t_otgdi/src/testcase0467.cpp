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
#include "testcase0467.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase0467Traces.h"
#endif



// # times to repeat the steps (default=3)
#define INSERT_REPEATS		gOpenIterations

// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0467");
const TTestCaseFactoryReceipt<CTestCase0467> CTestCase0467::iFactoryReceipt(KTestCaseId);	

CTestCase0467* CTestCase0467::NewL(TBool aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0467_NEWL);
	    }
	CTestCase0467* self = new (ELeave) CTestCase0467(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0467::CTestCase0467(TBool aHost)
:	CTestCaseRoot(KTestCaseId, aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0467_CTESTCASE0467);
	    }
		
	} 


/**
 ConstructL
*/
void CTestCase0467::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0467_CONSTRUCTL);
	    }
	iRepeats = INSERT_REPEATS;
	iWDTimer = CTestCaseWatchdog::NewL();
	
	BaseConstructL();
	}


CTestCase0467::~CTestCase0467()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0467_DCTESTCASE0467);
	    }

	Cancel();
	delete iWDTimer;
	
	}


void CTestCase0467::ExecuteTestCaseL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0467_EXECUTETESTCASEL);
	    }
	iCaseStep = EPreconditions;
	
	iRepeats = KOperationRetriesMax;	// VBus event rise retries
	
	CActiveScheduler::Add(this);
	SelfComplete();

	}

	
void CTestCase0467::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0467_DOCANCEL);
	    }

	// cancel our timer
	iTimer.Cancel();
	}


void CTestCase0467::CancelKB(CTestCaseRoot *pThis)
	{
	CTestCase0467 * p = REINTERPRET_CAST(CTestCase0467 *,pThis);
	// cancel the pending call

	p->iConsole->ReadCancel();

	}


void CTestCase0467::CancelIdPin(CTestCaseRoot *pThis)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0467_CANCELIDPIN);
	    }
	
	CTestCase0467 * p = REINTERPRET_CAST(CTestCase0467 *,pThis);
	// cancel any pending call, and then complete our active obj with a timeout value
	p->otgCancelOtgIdPinNotification();
	p->SelfComplete(KTestCaseWatchdogTO);	
	}


// This test result depends on all the ID detection tests and the VBus driving and dropping tests have not yet passed
void CTestCase0467::DescribePreconditions()
	{
	test.Printf(_L("Remove 'A' connector beforehand.\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0467_DESCRIBEPRECONDITIONS, "Remove 'A' connector beforehand.\n");
	test.Printf(_L("ID_PIN detection and VBus driving tests must already pass.\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0467_DESCRIBEPRECONDITIONS_DUP01, "ID_PIN detection and VBus driving tests must already pass.\n");
	}
		

// handle event completion	
void CTestCase0467::RunStepL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0467_RUNSTEPL);
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
			OstTrace0(TRACE_NORMAL, CTESTCASE0467_RUNSTEPL_DUP01, KRemoveAConnectorPrompt);
			test.Printf(KPressAnyKeyToContinue);
			OstTrace0(TRACE_NORMAL, CTESTCASE0467_RUNSTEPL_DUP02, KPressAnyKeyToContinue);
			RequestCharacter();			
			break;
			
		case ELoadLdd:
			// 1. load the LDD and init.
			if (!StepLoadLDD())
				{
				break;
				}
			// initialize variables for loop control
			
			iCaseStep = EWaitForIDPresent;
			SelfComplete();
			break;
			
		case ERepeatLoop: // 2
			test.Printf(_L("Repeat test %d/%d\n"), INSERT_REPEATS-iRepeats+1, INSERT_REPEATS);
			OstTraceExt2(TRACE_NORMAL, CTESTCASE0467_RUNSTEPL_DUP03, "Repeat test %d/%d\n", INSERT_REPEATS-iRepeats+1, INSERT_REPEATS);

			if (--iRepeats)
				iCaseStep = EWaitForIDPresent;
			else
				iCaseStep = EUnloadLdd;
			SelfComplete();	
			break;
			
		case EWaitForIDPresent:
			// 3. prompt for insertion
			test.Printf(KInsertAConnectorPrompt);
			OstTrace0(TRACE_NORMAL, CTESTCASE0467_RUNSTEPL_DUP04, KInsertAConnectorPrompt);
			otgQueueOtgIdPinNotification( iOTGIdPin, iStatus );

			SetActive();
			iCaseStep = EVerifyIDPresent;
			iDetectionRetry = 0;
			break;
			
		case EVerifyIDPresent:
			// 4. detect id_pin event
			LOG_VERBOSE1(_L("STEP4 - detect id_pin event 'A'\n"));
			if(gVerboseOutput)
			    {
			    OstTrace0(TRACE_VERBOSE, CTESTCASE0467_RUNSTEPL_DUP05, "STEP4 - detect id_pin event 'A'\n");;
			    }

			// retrieve the current ID_PIN value
			otgQueueOtgIdPinNotification( iOTGIdPin, iStatus );
			otgCancelOtgIdPinNotification();
			User::WaitForRequest(iStatus); // swallow the cancellation event (serves to test that it does cancel)


			if (iOTGIdPin != RUsbOtgDriver::EIdPinAPlug)
				{
				if (iDetectionRetry++ < 3)
					{
					iCaseStep = EVerifyIDPresent;
					otgQueueOtgIdPinNotification( iOTGIdPin, iStatus );

					SetActive();
					break;
					}
				else
					{
					return TestFailed(KErrAbort, _L("ID_PIN NOT 'seen' - FAILED!"));
					}
				}
				
			// check using the API in a syncronous way too
			// NOTE: we test this twice to explore and expose timing problems
			if (!otgIdPinPresent())
				{
				return TestFailed(KErrAbort, _L("ID_PIN syncronous call error - FAILED!"));
				}
			iCaseStep = EWaitForIDGone;
			SelfComplete();
			iDetectionRetry = 0;
			break;
			
		case EWaitForIDGone: 
			// 5. prompt for insertion
			test.Printf(KRemoveAConnectorPrompt);
			OstTrace0(TRACE_NORMAL, CTESTCASE0467_RUNSTEPL_DUP06, KRemoveAConnectorPrompt);
			otgQueueOtgIdPinNotification( iOTGIdPin, iStatus );
			SetActive();
			iCaseStep = EVerifyIDGone;
			break;
			
		case EVerifyIDGone:
			// 6. detect id_pin gone event
			LOG_VERBOSE1(_L("STEP4 - detect id_pin remove event 'B'\n"));
			if(gVerboseOutput)
			    {
			    OstTrace0(TRACE_VERBOSE, CTESTCASE0467_RUNSTEPL_DUP07, "STEP4 - detect id_pin remove event 'B'\n");;
			    }
			test.Printf(_L("ID_PIN=%d\n"), iOTGIdPin);
			OstTrace1(TRACE_NORMAL, CTESTCASE0467_RUNSTEPL_DUP08, "ID_PIN=%d\n", iOTGIdPin);

			User::After(5000); // 5ms
			otgQueueOtgIdPinNotification( iOTGIdPin, iStatus );
			otgCancelOtgIdPinNotification();
			User::WaitForRequest(iStatus); // swallow it
			test.Printf(_L("ID_PIN=%d\n"), iOTGIdPin);
			OstTrace1(TRACE_NORMAL, CTESTCASE0467_RUNSTEPL_DUP09, "ID_PIN=%d\n", iOTGIdPin);

			if (iOTGIdPin != RUsbOtgDriver::EIdPinBPlug)
				{
				if (iDetectionRetry++ < 3)
					{
					iCaseStep = EVerifyIDGone;
					otgQueueOtgIdPinNotification( iOTGIdPin, iStatus );
					SetActive();
					break;
					}
				else
					{
					return TestFailed(KErrAbort, _L("ID_PIN NOT 'removed' - FAILED!"));
					}
				}
			// check using the API in a syncronous way too
			if (otgIdPinPresent())
				{
				return TestFailed(KErrAbort, _L("ID_PIN syncronous call error - FAILED!"));
				}
			iCaseStep = ERepeatLoop;
			SelfComplete();
			break;
			
		case EUnloadLdd:
			// 7. unload
			if (EFalse == StepUnloadLDD())
				return TestFailed(KErrAbort,_L("Unload Ldd failure"));
			
			iCaseStep = ELastStep;
			SelfComplete();
			break;
			
		case ELastStep:
			TestPassed();
			break;
			
		default:
			test.Printf(_L("<Error> unknown test step"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0467_RUNSTEPL_DUP10, "<Error> unknown test step");
			Cancel();
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));
		}
	}

