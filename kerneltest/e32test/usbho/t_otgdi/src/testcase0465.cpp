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
#include "testcase0465.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase0465Traces.h"
#endif




// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0465");
const TTestCaseFactoryReceipt<CTestCase0465> CTestCase0465::iFactoryReceipt(KTestCaseId);	

CTestCase0465* CTestCase0465::NewL(TBool aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0465_NEWL);
	    }
	CTestCase0465* self = new (ELeave) CTestCase0465(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0465::CTestCase0465(TBool aHost)
:	CTestCaseRoot(KTestCaseId, aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0465_CTESTCASE0465);
	    }
		
	} 


/**
 ConstructL
*/
void CTestCase0465::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0465_CONSTRUCTL);
	    }
	iWDTimer = CTestCaseWatchdog::NewL();
	
	BaseConstructL();
	}


CTestCase0465::~CTestCase0465()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0465_DCTESTCASE0465);
	    }

	Cancel();
	delete iWDTimer;
	
	}


void CTestCase0465::ExecuteTestCaseL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0465_EXECUTETESTCASEL);
	    }
	iCaseStep = EPreconditions;
	
	iRepeats = KOperationRetriesMax;	// VBus event rise retries
	
	CActiveScheduler::Add(this);
	SelfComplete();

	}

	
void CTestCase0465::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0465_DOCANCEL);
	    }

	// cancel our timer
	iTimer.Cancel();
	}


void CTestCase0465::CancelKB(CTestCaseRoot *pThis)
	{
	CTestCase0465 * p = REINTERPRET_CAST(CTestCase0465 *,pThis);
	// cancel the pending call

	p->iConsole->ReadCancel();

	}


void CTestCase0465::CancelDrive(CTestCaseRoot *pThis)
	{
	CTestCase0465 * p = REINTERPRET_CAST(CTestCase0465 *,pThis);
	// cancel any pending call, and then complete our active obj with a timeout value
	p->SelfComplete(KTestCaseWatchdogTO);	
	}


// This test result depends on all the ID detection tests and the VBus driving and dropping tests have not yet passed
void CTestCase0465::DescribePreconditions()
	{
	test.Printf(_L("BEFORE running this test\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0465_DESCRIBEPRECONDITIONS, "BEFORE running this test\n");
	test.Printf(_L("\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0465_DESCRIBEPRECONDITIONS_DUP01, "\n");
	test.Printf(_L("Insert 'A' connector\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0465_DESCRIBEPRECONDITIONS_DUP02, "Insert 'A' connector\n");
	test.Printf(_L("\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0465_DESCRIBEPRECONDITIONS_DUP03, "\n");
	test.Printf(_L("Confirm passing tests\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0465_DESCRIBEPRECONDITIONS_DUP04, "Confirm passing tests\n");
	test.Printf(_L("\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0465_DESCRIBEPRECONDITIONS_DUP05, "\n");
	test.Printf(_L("ID_PIN detection\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0465_DESCRIBEPRECONDITIONS_DUP06, "ID_PIN detection\n");
	test.Printf(_L("VBus Driving\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0465_DESCRIBEPRECONDITIONS_DUP07, "VBus Driving\n");
	test.Printf(_L("\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0465_DESCRIBEPRECONDITIONS_DUP08, "\n");
	}
		

// handle event completion	
void CTestCase0465::RunStepL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0465_RUNSTEPL);
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
			test.Printf(_L("\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0465_RUNSTEPL_DUP01, "\n");
			test.Printf(KInsertAConnectorPrompt);
			OstTrace0(TRACE_NORMAL, CTESTCASE0465_RUNSTEPL_DUP02, KInsertAConnectorPrompt);
			test.Printf(_L("\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0465_RUNSTEPL_DUP03, "\n");
			test.Printf(KPressAnyKeyToContinue);
			OstTrace0(TRACE_NORMAL, CTESTCASE0465_RUNSTEPL_DUP04, KPressAnyKeyToContinue);
			test.Printf(_L("\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0465_RUNSTEPL_DUP05, "\n");
			RequestCharacter();			
			break;
			
		case ELoadLdd:
			// 1. load the LDD and init.
			if (!StepLoadLDD())
				{
				break;
				}
			iCaseStep = EDriveVBus;
			SelfComplete();
			break;
			
		case EDriveVBus:
			// 2. DRIVE VBUS
			// wait for 100 ms to allow the stack time to settle
			User::After(100000);
			err = otgBusRequest();
			if (KErrNone != err)
				{
				return TestFailed(KErrAbort, _L("Raise VBus - RUsbOtgDriver::BusRequest() FAILED!"));
				}

			// wait for 1000 ms as per test document
			User::After(1000000);

			// test for VBus rise next
			otgQueueOtgEventRequest( iOTGEvent, iStatus );
			SetActive();
			iCaseStep = EVerifyVBus;
			break;
			
		case EVerifyVBus:
			// 3. get VBus rise event
			iCaseStep = EUnloadLdd;
			OtgEventString(iOTGEvent, aDescription);
			test.Printf(_L("Received event %d '%S' status(%d)\n"), iOTGEvent, &aDescription, completionCode);
			OstTraceExt3(TRACE_NORMAL, CTESTCASE0465_RUNSTEPL_DUP06, "Received event %d '%S' status(%d)\n", iOTGEvent, aDescription, completionCode);
			
			if (iOTGEvent != RUsbOtgDriver::EEventVbusRaised)
				{
				iCaseStep = EVerifyVBus;
				iRepeats--;
				if (iRepeats <0)
					{
					return TestFailed(KErrAbort, _L("Raise VBus event not detected. FAILED!"));
					}

				otgQueueOtgEventRequest( iOTGEvent, iStatus );
				SetActive();				
				break;	
				}
			test.Printf(_L("VBus seen OK.\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0465_RUNSTEPL_DUP07, "VBus seen OK.\n");
			
			SelfComplete();
			break;
			
		case EUnloadLdd:
			// unload
			if (EFalse == StepUnloadLDD())
				{
				return TestFailed(KErrAbort,_L("unload Ldd failure"));	
				}	
			
			// wait for 100 ms to allow the stack time to settle
			User::After(100000);
			
			test.Printf(_L("Use meter or oscilloscope\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0465_RUNSTEPL_DUP08, "Use meter or oscilloscope\n");
			test.Printf(_L("to measure VBUS, which should\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0465_RUNSTEPL_DUP09, "to measure VBUS, which should\n");
			test.Printf(_L("have dropped\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0465_RUNSTEPL_DUP10, "have dropped\n");
			test.Printf(_L("\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0465_RUNSTEPL_DUP11, "\n");
			test.Printf(_L("Is it below 0.2 volts?\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0465_RUNSTEPL_DUP12, "Is it below 0.2 volts?\n");
			test.Printf(_L("\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0465_RUNSTEPL_DUP13, "\n");
			test.Printf(_L("Select Y or N to continue\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0465_RUNSTEPL_DUP14, "Select Y or N to continue\n");
			RequestCharacter();
			iWDTimer->IssueRequest(KDelayDurationForUserActivityMS, this, &CancelKB);
			iCaseStep = EVerifyBusGone;
			break;
			
		case EVerifyBusGone:
			// since the LDD is gone, there is no OTGDI API from here on
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
				return TestFailed(KErrAbort, _L("Operator response timeout"));
				}
			iWDTimer->Cancel();
			// user intput
			if (('y' == iKeyCodeInput) ||('Y' == iKeyCodeInput))
				{
				test.Printf(_L("VBUS 'drop' seen\n"));
				OstTrace0(TRACE_NORMAL, CTESTCASE0465_RUNSTEPL_DUP15, "VBUS 'drop' seen\n");
				SelfComplete();
				}
			else
				{
				return TestFailed(KErrAbort, _L("VBus drop NOT 'seen' - FAILED!"));
				}
			iCaseStep = ELastStep;
			break;
			
		case ELastStep:
			TestPassed();
			break;
			
		default:
			test.Printf(_L("<Error> unknown test step\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0465_RUNSTEPL_DUP16, "<Error> unknown test step\n");
			Cancel();
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));
		}
	}


