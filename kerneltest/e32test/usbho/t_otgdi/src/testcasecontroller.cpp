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
// testengine.cpp
// @internalComponent
// 
//


#include <e32std.h>
#include <e32std_private.h>
#include <u32std.h> 	// unicode builds
#include <e32base.h>
#include <e32base_private.h>
#include <e32cons.h>
#include <e32Test.h>	// RTest headder
#include <e32def.h>
#include <e32def_private.h>
#include "testcaseroot.h"
#include "testcasecontroller.h"
#include "testengine.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcasecontrollerTraces.h"
#endif



	
CTestCaseController* CTestCaseController::NewL(CTestEngine& aTestEngine,TBool aHostRole)
	{
	CTestCaseController* self = new (ELeave) CTestCaseController(aTestEngine,aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
	
CTestCaseController::CTestCaseController(CTestEngine& aTestEngine,TBool aHostRole)
:	CActive(EPriorityStandard),
	iTestEngine(aTestEngine),
	iHostRole(aHostRole)
	{
	// Add to current threads active scheduler
	CActiveScheduler::Add(this);
	}
	
	
CTestCaseController::~CTestCaseController()
	{
	// Cancel any outstanding test cases
	Cancel();
	delete iTestPolicy;	
	}


void CTestCaseController::ConstructL()
	{
	TInt err(KErrNone);
	
	iTestPolicy = CBasicTestPolicy::NewL();
	
	err = iTestEngine.NextTestCaseId(iTestCaseId);
	if (err != KErrNone)
		User::Leave(-2);
	
	test.Next(iTestCaseId);
	// pass control to the test-policy (which merely instantiates and runs the test)
	iTestPolicy->RunTestCaseL(iTestCaseId, &iStatus);
	SetActive(); // when the 1st test finnishes, we drop into our own RunL active processing loop
	}
	

void CTestCaseController::DoCancel()
	{
	// Cancel the outstanding test case running
	iTestPolicy->Cancel();
	}
	
	
void CTestCaseController::RunL()
	{
	// Retrieve the completion code of the last test case run
	TInt err(iStatus.Int());
	
	if (err != KErrNone)
		{
		test.Printf(_L("<Error> Test case %lS failed\n"),&iTestCaseId);
		OstTraceExt1(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL, "<Error> Test case %lS failed\n",iTestCaseId);
		}
	else
		{
		test.Printf(_L("Test case %lS passed\n"),&iTestCaseId);
		OstTraceExt1(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP01, "Test case %lS passed\n",iTestCaseId);
		}
		
	// Find next test to run	
	err = iTestEngine.NextTestCaseId(iTestCaseId);
	if (err == KErrNone)
		{
		test.Printf(_L("\n"));	// ensures blank line between tests
		OstTrace0(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP02, "\n");	// ensures blank line between tests
		test.Printf(_L("\n"));
		OstTrace0(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP03, "\n");
		test.Next(iTestCaseId);
		
		// run the next test here
		iTestPolicy->RunTestCaseL(iTestCaseId, &iStatus);
		SetActive();
		}
	else if (err == KErrNotFound)
		{
		OstTrace0(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP04, "All specified test cases performed");
		CActiveScheduler::Stop();
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP05, "<Error %d> Unknown error from CTestEngine::NextTestCaseId",err);
		User::Leave(err);
		}
	}
	
	
TInt CTestCaseController::RunError(TInt aError)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASECONTROLLER_RUNERROR);
	    }
	switch (aError)
		{
		case KErrNoMemory:
		default:
			// Panic the test module
			test(EFalse);
			break;
		}
	return KErrNone;
	}



