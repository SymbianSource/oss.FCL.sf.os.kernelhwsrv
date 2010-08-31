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
#include "testcase0463.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase0463Traces.h"
#endif




// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0463");
const TTestCaseFactoryReceipt<CTestCase0463> CTestCase0463::iFactoryReceipt(KTestCaseId);	


CTestCase0463* CTestCase0463::NewL(TBool aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0463_NEWL);
	    }
	CTestCase0463* self = new (ELeave) CTestCase0463(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0463::CTestCase0463(TBool aHost)
:	CTestCaseRoot(KTestCaseId, aHost)
	{	
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0463_CTESTCASE0463);
	    }
	} 


/**
 ConstructL
*/
void CTestCase0463::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0463_CONSTRUCTL);
	    }
	iRepeats = OOMOPEN_REPEATS;
	iAllocFailNumber = OOMOPEN_REPEATS + 1; // allocs 1..11 fail
	
	BaseConstructL();
	}


CTestCase0463::~CTestCase0463()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0463_DCTESTCASE0463);
	    }

	Cancel();
	}


void CTestCase0463::ExecuteTestCaseL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0463_EXECUTETESTCASEL);
	    }
	iCaseStep = EMarkStack;
	CActiveScheduler::Add(this);

	SelfComplete();
	
	}

void CTestCase0463::DescribePreconditions()
	{
	test.Printf(_L("Insert A connector beforehand.\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0463_DESCRIBEPRECONDITIONS, "Insert A connector beforehand.\n");
	}

	
void CTestCase0463::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0463_DOCANCEL);
	    }

	// cancel our timer
	iTimer.Cancel();
	}


// handle event completion	
void CTestCase0463::RunStepL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0463_RUNSTEPL);
	    }

	// Obtain the completion code for this CActive obj.
	TInt completionCode(iStatus.Int()); 
	
	switch(iCaseStep)
		{
		case EMarkStack:
			SelfComplete();
			iCaseStep = ELoadLdd;
			break;
			
		case ELoadLdd:
			__UHEAP_SETFAIL(RHeap::EDeterministic, iAllocFailNumber);
			__UHEAP_MARK;
			test.Printf(_L("Load the LDD iteration %d/%d\n"), OOMOPEN_REPEATS-iRepeats+1, OOMOPEN_REPEATS);
			OstTraceExt2(TRACE_NORMAL, CTESTCASE0463_RUNSTEPL_DUP01, "Load the LDD iteration %d/%d\n", OOMOPEN_REPEATS-iRepeats+1, OOMOPEN_REPEATS);
			aIntegerP = new TInt;
			CleanupStack::PushL(aIntegerP);
			if (!StepLoadLDD())
				{
				break;
				}
			// panic if the cleanupstack was messed
			CleanupStack::PopAndDestroy(aIntegerP);

			// SAMPLE : some code to test that the __HEAP macros did their stuff
			// uncomment this to verify the test flow would fail
			//	{
			//	TInt *aInt = new TInt;
			//	if (NULL ==aInt)
			//		test.Printf(_L("Alloc failed!!!!!!\n"));
			//	delete aInt;
			//	}
			iCaseStep = EUnloadLdd;
			SelfComplete();
			break;
			
		case EUnloadLdd:
			if (EFalse == StepUnloadLDD())
				return TestFailed(KErrAbort,_L("Unload Ldd failure"));	
			__UHEAP_MARKEND;
			test.Printf(_L("Heap intact: Asize %d\n"), iAllocFailNumber);
			OstTrace1(TRACE_NORMAL, CTESTCASE0463_RUNSTEPL_DUP03, "Heap intact: Asize %d\n", iAllocFailNumber);
			iCaseStep = ELoopDecrement;
			SelfComplete();			
			break;
						
		case ELoopDecrement:
			test.Printf(_L("Repeat test\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0463_RUNSTEPL_DUP04, "Repeat test\n");
			__UHEAP_RESET;
			iAllocFailNumber--;
			
			if (iRepeats--)
				iCaseStep = ELoadLdd;
			else
				iCaseStep = ELastStep;
			SelfComplete();	
		    break;
		  
		case ELastStep:

			TestPolicy().SignalTestComplete(KErrNone);
			break;
			
		default:
			test.Printf(_L("<Error> unknown test step"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0463_RUNSTEPL_DUP05, "<Error> unknown test step");
			Cancel();
			TestPolicy().SignalTestComplete(KErrCorrupt);
			break;
		}
	}

	
