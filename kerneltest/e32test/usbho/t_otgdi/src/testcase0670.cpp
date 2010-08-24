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
#include "testcase0670.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase0670Traces.h"
#endif



// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0670");
const TTestCaseFactoryReceipt<CTestCase0670> CTestCase0670::iFactoryReceipt(KTestCaseId);	

CTestCase0670* CTestCase0670::NewL(TBool aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0670_NEWL);
	    }
	CTestCase0670* self = new (ELeave) CTestCase0670(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0670::CTestCase0670(TBool aHost)
:	CTestCaseRoot(KTestCaseId, aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0670_CTESTCASE0670);
	    }
		
	} 


/**
 ConstructL
*/
void CTestCase0670::ConstructL()
	{
	iWDTimer = CTestCaseWatchdog::NewL();
	
	BaseConstructL();
	}


CTestCase0670::~CTestCase0670()
	{

	Cancel();
	delete iWDTimer;	
	}


void CTestCase0670::ExecuteTestCaseL()
	{
	iCaseStep = EPreconditions;	
	iRepeats = KOperationRetriesMax;	// VBus event rise retries
	CActiveScheduler::Add(this);
	SelfComplete();

	}

	
void CTestCase0670::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0670_DOCANCEL);
	    }

	// cancel our timer
	iTimer.Cancel();
	}


void CTestCase0670::CancelKB(CTestCaseRoot *pThis)
	{
	CTestCase0670 * p = REINTERPRET_CAST(CTestCase0670 *,pThis);
	
	// cancel the pending call
	p->iConsole->ReadCancel();
	}


void CTestCase0670::CancelDrive(CTestCaseRoot *pThis)
	{
	CTestCase0670 * p = REINTERPRET_CAST(CTestCase0670 *,pThis);
	// cancel any pending call, and then complete our active obj with a timeout value
	p->SelfComplete(KTestCaseWatchdogTO);	
	}


void CTestCase0670::DescribePreconditions()
	{
	// H4 width     ****************************
	test.Printf(_L("***************************\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0670_DESCRIBEPRECONDITIONS, "***************************\n");
	test.Printf(_L("* This test uses a Mini-A *\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0670_DESCRIBEPRECONDITIONS_DUP01, "* This test uses a Mini-A *\n");
	test.Printf(_L("* to Mini-B cable to link *\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0670_DESCRIBEPRECONDITIONS_DUP02, "* to Mini-B cable to link *\n");
	test.Printf(_L("* the H4 board to the OPT *\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0670_DESCRIBEPRECONDITIONS_DUP03, "* the H4 board to the OPT *\n");
	test.Printf(_L("* and makes use of the    *\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0670_DESCRIBEPRECONDITIONS_DUP04, "* and makes use of the    *\n");
	test.Printf(_L("*  USB OPT test code      *\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0670_DESCRIBEPRECONDITIONS_DUP05, "*  USB OPT test code      *\n");
	test.Printf(_L("***************************\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0670_DESCRIBEPRECONDITIONS_DUP06, "***************************\n");
	}


// handle event completion	
void CTestCase0670::RunStepL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0670_RUNSTEPL);
	    }
	// Obtain the completion code for this CActive obj.
	TInt completionCode(iStatus.Int()); 
	TBuf<MAX_DSTRLEN> aDescription;
		
	switch(iCaseStep)
		{
		case EPreconditions:
			if (iAutomated)
				{
				return TestFailed(KErrAbort,_L("This Test Cannot Run in Automated Mode"));
				}
			SelfComplete();
			iCaseStep = ELoadWithOptTestMode;
			break;

		case ELoadWithOptTestMode:
			if (!StepLoadClient(0x0670/*use default settings for SRP/HNP support*/))
				{
				return TestFailed(KErrAbort,_L("Client Load Failure"));
				}
			StepSetOptActive();
			if (!StepLoadLDD())
				{
				return TestFailed(KErrAbort,_L("OTG Load Failure"));
				}
			iCaseStep = EConnectAtoB;
			SelfComplete();
			break;
			
		case EConnectAtoB:
			// H4 width     ****************************
			test.Printf(_L("\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0670_RUNSTEPL_DUP01, "\n");
			test.Printf(_L("***********************\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0670_RUNSTEPL_DUP02, "***********************\n");
//			WAIT_ON_KEYOP( "Connect H4(A) to OPT(B)\n");
			test.Printf(_L("Connect H4(A) to OPT(B)\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0670_RUNSTEPL_DUP03, "Connect H4(Ato OPT(B)\n");
			test.Printf(KPressAnyKeyToContinue);
			OstTrace0(TRACE_NORMAL, CTESTCASE0670_RUNSTEPL_DUP04, KPressAnyKeyToContinue);

			iCaseStep = EStartOptTD4_5;
			RequestCharacter();	
			break;

		case EStartOptTD4_5:
			// H4 width     ****************************
			test.Printf(_L("On the OPT, select:\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0670_RUNSTEPL_DUP05, "On the OPT, select:\n");
			test.Printf(_L("  Certified FS-A-UUT Test\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0670_RUNSTEPL_DUP06, "  Certified FS-A-UUT Test\n");
			test.Printf(_L("  Test TD.4.5-2.9ms-100ms\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0670_RUNSTEPL_DUP07, "  Test TD.4.5-2.9ms-100ms\n");
			test.Printf(_L("  And then Click 'Run' "));
			OstTrace0(TRACE_NORMAL, CTESTCASE0670_RUNSTEPL_DUP08, "  And then Click 'Run' ");
			test.Printf(_L("  When test starts, press any key"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0670_RUNSTEPL_DUP09, "  When test starts, press any key");
			iCaseStep = EPromptYOpt4_5;
			RequestCharacter();	
			break;

		case EPromptYOpt4_5:
			iCaseStep = EConfirmOpt4_5;
			test.Printf(_L("Did it PASS (Y/N)?"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0670_RUNSTEPL_DUP10, "Did it PASS (Y/N?");
			RequestCharacter(); // 30 seconds for user input
			iWDTimer->IssueRequest(KDelayDurationForTest4_5, this, &CancelKB);
			break;

		case EConfirmOpt4_5:
			// Check watchdog timeout, assume it failed
			if (KTestCaseWatchdogTO == iStatus.Int())
				{
									// H4 width  ****************************
				return TestFailed(KErrAbort, _L("OPT timeout - Test Failed"));
				}
			iWDTimer->Cancel();

			// Check user response, look only for 'Y'
			if (('y' == iKeyCodeInput) ||('Y' == iKeyCodeInput))
				{
				iCaseStep = EUnloadLdd;
				SelfComplete();
				}
			else
				{
				return TestFailed(KErrAbort, _L("TD.4.5 - FAILED!"));
				}
			break;

		case EUnloadLdd:
			// unload otg
			if (!StepUnloadLDD())
				{
				return TestFailed(KErrAbort,_L("OTG Unload Failure"));	
				}
			// unload client
			if (!StepUnloadClient())
				{
				return TestFailed(KErrAbort,_L("Client Unload Failure"));	
				}
			iCaseStep = ELastStep;
			SelfComplete();
			break;

		case ELastStep:
			TestPassed();
			break;
			
		default:
			test.Printf(_L("<Error> unknown test step"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0670_RUNSTEPL_DUP11, "<Error> unknown test step");
			Cancel();
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));
		}
	}

