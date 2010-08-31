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
#include "testcase0469.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase0469Traces.h"
#endif

#define _REPEATS (oOpenIterations*3)



// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0469");
const TTestCaseFactoryReceipt<CTestCase0469> CTestCase0469::iFactoryReceipt(KTestCaseId);	

CTestCase0469* CTestCase0469::NewL(TBool aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0469_NEWL);
	    }
	CTestCase0469* self = new (ELeave) CTestCase0469(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
CTestCase0469::CTestCase0469(TBool aHost)
	: CTestCaseB2BRoot(KTestCaseId, aHost, iStatus) 
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0469_CTESTCASE0469);
	    }
		
	} 

/**
 ConstructL
*/
void CTestCase0469::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0469_CONSTRUCTL);
	    }
    iDualRoleCase = EFalse; // Not back to back
	BaseConstructL();
	}


CTestCase0469::~CTestCase0469()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0469_DCTESTCASE0469);
	    }
    iCollector.DestroyObservers();
	Cancel();
	}


void CTestCase0469::ExecuteTestCaseL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0469_EXECUTETESTCASEL);
	    }
	iCaseStep = EPreconditions;
	iRepeats = 3;
	
	CActiveScheduler::Add(this);
	SelfComplete();
	}

	
void CTestCase0469::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0469_DOCANCEL);
	    }
	// cancel our timer
	iTimer.Cancel();
	}

// handle event completion	
void CTestCase0469::RunStepL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0469_RUNSTEPL);
	    }
	// Obtain the completion code for this CActive obj.
	TInt completionCode(iStatus.Int()); 
	TBuf<MAX_DSTRLEN> aDescription;
    //	15 seconds, should be plenty of time for 3 cycles of plug pulling
    const TInt KTestCase0469Timeout = 15000;

	switch(iCaseStep)
		{
    case EPreconditions:
        LOG_STEPNAME(_L("EPreconditions"))
        iCaseStep = ELoadLdd;
        // prompt to insert connectors
        test.Printf(KInsertBCablePrompt);
        OstTrace0(TRACE_NORMAL, CTESTCASE0469_RUNSTEPL_DUP01, KInsertBCablePrompt);
        test.Printf(KRemoveAFromPC);
        OstTrace0(TRACE_NORMAL, CTESTCASE0469_RUNSTEPL_DUP02, KRemoveAFromPC);
        test.Printf(KPressAnyKeyToContinue);
        OstTrace0(TRACE_NORMAL, CTESTCASE0469_RUNSTEPL_DUP03, KPressAnyKeyToContinue);
        RequestCharacter();			
        break;
			
    case ELoadLdd:
        LOG_STEPNAME(_L("ELoadLdd"))
        if (!StepLoadClient(0xF678/*use default settings for SRP/HNP support*/))
            {
            return TestFailed(KErrAbort, _L("Client Load Failure"));
            }

        if (!StepLoadLDD())
            {
            return TestFailed(KErrAbort, _L("OTG Load Failure"));
            }

        // subscribe to OTG states,events and messages now that it has loaded OK
        TRAPD(result, iCollector.CreateObserversL(*this));
        if (KErrNone != result)
            {
            return(TestFailed(KErrNoMemory, _L("Unable to create observers")));
            }
        iCollector.ClearAllEvents();

        iCaseStep = ELoopControl;

		iCollector.AddStepTimeout(KTestCase0469Timeout);

        SelfComplete();
        break;
			
    case ELoopControl:
        LOG_STEPNAME(_L("ELoopControl"))

        // Check for timeout
        if (KTestCaseWatchdogTO == iStatus.Int())
            {
            iCollector.DestroyObservers();
            return TestFailed(KErrAbort, _L("Timeout"));
            }

        if (iRepeats--)
            {
            OstTrace1(TRACE_NORMAL, CTESTCASE0469_RUNSTEPL_DUP04, "ELoopControl around again %d", iRepeats);
            iCaseStep = ETestVbusRise;
            }
        else
            {
            OstTrace0(TRACE_NORMAL, CTESTCASE0469_RUNSTEPL_DUP05, "ELoopControl we're done");
            iCaseStep = EUnloadLdd;
            }
        SelfComplete();
        break;

    case ETestVbusRise:
        LOG_STEPNAME(_L("ETestVbusRise"))
        // Check for timeout
        if (KTestCaseWatchdogTO == iStatus.Int())
            {
            iCollector.DestroyObservers();
            return TestFailed(KErrAbort, _L("Timeout"));
            }

        iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventVbusRaised);
        iCaseStep = ETestVbusFall;
        test.Printf(KInsertAIntoPC);
        OstTrace0(TRACE_NORMAL, CTESTCASE0469_RUNSTEPL_DUP06, KInsertAIntoPC);
        SetActive();
        break;

    case ETestVbusFall:
        LOG_STEPNAME(_L("ETestVbusFall"))
        // Check for timeout
        if (KTestCaseWatchdogTO == iStatus.Int())
            {
            iCollector.DestroyObservers();
            return TestFailed(KErrAbort, _L("Timeout"));
            }

        iCollector.AddRequiredNotification(EWatcherEvent, RUsbOtgDriver::EEventVbusDropped);
        iCaseStep = ELoopControl;
        test.Printf(KRemoveAFromPC);
        OstTrace0(TRACE_NORMAL, CTESTCASE0469_RUNSTEPL_DUP07, KRemoveAFromPC);
        SetActive();
        break;

    case EUnloadLdd:
        LOG_STEPNAME(_L("EUnloadLdd"))
        iCollector.DestroyObservers();
        OstTrace0(TRACE_NORMAL, CTESTCASE0469_RUNSTEPL_DUP08, "Destroyed observers");
        if (EFalse == StepUnloadLDD())
            return TestFailed(KErrAbort,_L("unload Ldd failure"));	
        OstTrace0(TRACE_NORMAL, CTESTCASE0469_RUNSTEPL_DUP09, "unloaded ldd");
        if (!StepUnloadClient())
            return TestFailed(KErrAbort,_L("Client Unload Failure"));	
        OstTrace0(TRACE_NORMAL, CTESTCASE0469_RUNSTEPL_DUP10, "unloaded client");

        iCaseStep = ELastStep;
        SelfComplete();
        break;
			
    case ELastStep:
        LOG_STEPNAME(_L("ELastStep"))
        TestPassed();
        break;
			
    default:
        test.Printf(_L("<Error> unknown test step"));
        OstTrace0(TRACE_NORMAL, CTESTCASE0469_RUNSTEPL_DUP11, "<Error> unknown test step");
        Cancel();
        return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));
		}
	}

