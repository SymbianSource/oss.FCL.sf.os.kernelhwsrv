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
// Open/Close 'powered peripheral' test
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <u32std.h> 	// unicode builds
#include <e32base.h>
#include <e32base_private.h>
#include <e32Test.h>	// RTest headder
#include "testcaseroot.h"
#include "testcase0458.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase0458Traces.h"
#endif




// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0458");
const TTestCaseFactoryReceipt<CTestCase0458> CTestCase0458::iFactoryReceipt(KTestCaseId);	


CTestCase0458* CTestCase0458::NewL(TBool aHost)
	{
	CTestCase0458* self = new (ELeave) CTestCase0458(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0458::CTestCase0458(TBool aHost)
:	CTestCaseRoot(KTestCaseId, aHost)
	{
	} 


/**
 ConstructL
*/
void CTestCase0458::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0458_CONSTRUCTL);
	    }
	iRepeats = OPEN_REPEATS;
	
	BaseConstructL();
	}


CTestCase0458::~CTestCase0458()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0458_DCTESTCASE0458);
	    }

	Cancel();
	}


void CTestCase0458::ExecuteTestCaseL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0458_EXECUTETESTCASEL);
	    }
	iCaseStep = EPreconditions;
	
	CActiveScheduler::Add(this);
	SelfComplete();

	}

void CTestCase0458::DescribePreconditions()
	{
	test.Printf(_L("Insert 'B' connector from cable\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0458_DESCRIBEPRECONDITIONS, "Insert 'B' connector from cable\n");
	test.Printf(_L("attached to powered host beforehand.\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0458_DESCRIBEPRECONDITIONS_DUP01, "attached to powered host beforehand.\n");
	}

	
void CTestCase0458::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0458_DOCANCEL);
	    }

	// cancel our timer
	iTimer.Cancel();
	}


// handle event completion	
void CTestCase0458::RunStepL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0458_RUNSTEPL);
	    }

	// Obtain the completion code for this CActive obj.
	TInt completionCode(iStatus.Int()); 
	
	switch(iCaseStep)
		{
		case EPreconditions:
			{
			test.Printf(KPressAnyKeyToStart);
			OstTrace0(TRACE_NORMAL, CTESTCASE0458_RUNSTEPL_DUP01, KPressAnyKeyToStart);
			iCaseStep = ELoadLdd;
			RequestCharacter();			
			break;			
			}

		case ELoadLdd:
			test.Printf(_L("Load the LDD iteration %d/%d\n"), OPEN_REPEATS-iRepeats+1, OPEN_REPEATS);
			OstTraceExt2(TRACE_NORMAL, CTESTCASE0458_RUNSTEPL_DUP02, "Load the LDD iteration %d/%d\n", OPEN_REPEATS-iRepeats+1, OPEN_REPEATS);
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
			OstTrace0(TRACE_NORMAL, CTESTCASE0458_RUNSTEPL_DUP03, "Repeat test\n");

			if (--iRepeats)
				{
				iCaseStep = ELoadLdd;
				}
			else
				{
				iCaseStep = ELastStep;
				}
			SelfComplete();	
		    break;
		  
		// Finnished    
		case ELastStep:
			// PASS
			return TestPassed();

			
		default:
			test.Printf(_L("<Error> unknown test step"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0458_RUNSTEPL_DUP04, "<Error> unknown test step");
			Cancel();
			TestPolicy().SignalTestComplete(KErrCorrupt);
			break;
		}
		
	}


