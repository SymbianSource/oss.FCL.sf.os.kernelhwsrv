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
// Open/Close 'disconnected' test
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <u32std.h> 	// unicode builds
#include <e32base.h>
#include <e32base_private.h>
#include <e32Test.h>	// RTest headder
#include "testcaseroot.h"
#include "testcase0457.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase0457Traces.h"
#endif



// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0457");
const TTestCaseFactoryReceipt<CTestCase0457> CTestCase0457::iFactoryReceipt(KTestCaseId);	

// # times to repeat the steps
#define OPEN_REPEATS		gOpenIterations

CTestCase0457* CTestCase0457::NewL(TBool aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0457_NEWL);
	    }
	CTestCase0457* self = new (ELeave) CTestCase0457(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0457::CTestCase0457(TBool aHost)
:	CTestCaseRoot(KTestCaseId, aHost)
	{	
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0457_CTESTCASE0457);
	    }
	} 


/**
 ConstructL
*/
void CTestCase0457::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0457_CONSTRUCTL);
	    }
	iRepeats = OPEN_REPEATS;
	
	BaseConstructL();
	}


CTestCase0457::~CTestCase0457()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0457_DCTESTCASE0457);
	    }

	Cancel();
	}


void CTestCase0457::ExecuteTestCaseL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0457_EXECUTETESTCASEL);
	    }
	iCaseStep = EPreconditions;
	
	CActiveScheduler::Add(this);
	SelfComplete();
	}


void CTestCase0457::DescribePreconditions()
	{
	test.Printf(_L("Remove any USB plug beforehand.\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0457_DESCRIBEPRECONDITIONS, "Remove any USB plug beforehand.\n");
	}

	
void CTestCase0457::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0457_DOCANCEL);
	    }

	// cancel our timer
	iTimer.Cancel();
	}


// handle event completion	
void CTestCase0457::RunStepL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0457_RUNSTEPL);
	    }

	// Obtain the completion code for this CActive obj.
	TInt completionCode(iStatus.Int()); 
	
	switch(iCaseStep)
		{
		case EPreconditions:
			{
			test.Printf(KPressAnyKeyToStart);
			OstTrace0(TRACE_NORMAL, CTESTCASE0457_RUNSTEPL_DUP01, KPressAnyKeyToStart);
			iCaseStep = ELoadLdd;
			RequestCharacter();			
			break;			
			}

		case ELoadLdd:
			test.Printf(_L("Load the LDD iteration %d/%d\n"), OPEN_REPEATS-iRepeats+1, OPEN_REPEATS);
			OstTraceExt2(TRACE_NORMAL, CTESTCASE0457_RUNSTEPL_DUP02, "Load the LDD iteration %d/%d\n", OPEN_REPEATS-iRepeats+1, OPEN_REPEATS);
			if (!StepLoadLDD())
				{
				break;
				}

			iCaseStep = EUnloadLdd;
			SelfComplete();
			break;
		case EUnloadLdd:
			if (EFalse == StepUnloadLDD())
				return TestFailed(KErrAbort,_L("Unload Ldd failure"));	
			
			iCaseStep = ELoopDecrement;
			SelfComplete();			
			break;
						
		case ELoopDecrement:
			test.Printf(_L("Repeat test\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0457_RUNSTEPL_DUP03, "Repeat test\n");

			if (--iRepeats)
				iCaseStep = ELoadLdd;
			else
				iCaseStep = ELastStep;
			SelfComplete();	
		    break;
		  
		// Finnished 
		case ELastStep:
			// PASS
			return TestPassed();

			
		default:
			test.Printf(_L("<Error> unknown test step"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0457_RUNSTEPL_DUP04, "<Error> unknown test step");
			Cancel();
			TestPolicy().SignalTestComplete(KErrCorrupt);
			break;
		}
		

	}


