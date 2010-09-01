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

#define _REPEATS (oOpenIterations*3)



// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0469");
const TTestCaseFactoryReceipt<CTestCase0469> CTestCase0469::iFactoryReceipt(KTestCaseId);	

CTestCase0469* CTestCase0469::NewL(TBool aHost)
	{
	LOG_FUNC
	CTestCase0469* self = new (ELeave) CTestCase0469(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
CTestCase0469::CTestCase0469(TBool aHost)
	: CTestCaseB2BRoot(KTestCaseId, aHost, iStatus) 
	{
	LOG_FUNC
		
	} 

/**
 ConstructL
*/
void CTestCase0469::ConstructL()
	{
	LOG_FUNC
    iDualRoleCase = EFalse; // Not back to back
	BaseConstructL();
	}


CTestCase0469::~CTestCase0469()
	{
	LOG_FUNC
    iCollector.DestroyObservers();
	Cancel();
	}


void CTestCase0469::ExecuteTestCaseL()
	{
	LOG_FUNC
	iCaseStep = EPreconditions;
	iRepeats = 3;
	
	CActiveScheduler::Add(this);
	SelfComplete();
	}

	
void CTestCase0469::DoCancel()
	{
	LOG_FUNC
	// cancel our timer
	iTimer.Cancel();
	}

// handle event completion	
void CTestCase0469::RunStepL()
	{
	LOG_FUNC
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
        test.Printf(KRemoveAFromPC);
        test.Printf(KPressAnyKeyToContinue);
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
            RDebug::Printf("ELoopControl around again %d", iRepeats);
            iCaseStep = ETestVbusRise;
            }
        else
            {
            RDebug::Printf("ELoopControl we're done");
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
        SetActive();
        break;

    case EUnloadLdd:
        LOG_STEPNAME(_L("EUnloadLdd"))
        iCollector.DestroyObservers();
        RDebug::Printf("Destroyed observers");
        if (EFalse == StepUnloadLDD())
            return TestFailed(KErrAbort,_L("unload Ldd failure"));	
        RDebug::Printf("unloaded ldd");
        if (!StepUnloadClient())
            return TestFailed(KErrAbort,_L("Client Unload Failure"));	
        RDebug::Printf("unloaded client");

        iCaseStep = ELastStep;
        SelfComplete();
        break;
			
    case ELastStep:
        LOG_STEPNAME(_L("ELastStep"))
        TestPassed();
        break;
			
    default:
        test.Printf(_L("<Error> unknown test step"));
        Cancel();
        return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));
		}
	}

