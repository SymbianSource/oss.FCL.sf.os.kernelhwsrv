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
// @file testcasecontroller.cpp
// @internalComponent
// 
//

#include "basetestcase.h"
#include "testcasecontroller.h"
#include "testcasefactory.h"
#include "testengine.h"
#include "testpolicy.h"
#include "testdebug.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "TestCaseControllerTraces.h"
#endif

_LIT(KClientDeviceDriverName,"EUSBC");
_LIT(KHostDeviceDriverName,"usbhubdriver");
_LIT(KHostDeviceInterfaceDriverName,"usbdi");
_LIT(KOtgdiLddFileName, "otgdi");

namespace NUnitTesting_USBDI
	{
	
CTestCaseController* CTestCaseController::NewL(CTestEngine& aTestEngine,TBool aHostRole)
	{
	OstTraceFunctionEntryExt( CTESTCASECONTROLLER_NEWL_ENTRY, 0 );
	CTestCaseController* self = new (ELeave) CTestCaseController(aTestEngine,aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CTESTCASECONTROLLER_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	
	
CTestCaseController::CTestCaseController(CTestEngine& aTestEngine,TBool aHostRole)
:	CActive(EPriorityStandard),
	iTestEngine(aTestEngine),
	iHostRole(aHostRole)
	{
	OstTraceFunctionEntryExt( CTESTCASECONTROLLER_CTESTCASECONTROLLER_ENTRY, this );
	// Add to current threads active scheduler
	CActiveScheduler::Add(this);
	OstTraceFunctionExit1( CTESTCASECONTROLLER_CTESTCASECONTROLLER_EXIT, this );
	}
	
	
CTestCaseController::~CTestCaseController()
	{
    OstTraceFunctionEntry1( CTESTCASECONTROLLER_CTESTCASECONTROLLER_ENTRY_DUP01, this );

	Cancel(); // Cancels any oustanding test cases

	delete iTestPolicy;
	
	if(iHostRole)
		{	
		TInt err = User::FreeLogicalDevice(KHostDeviceInterfaceDriverName);
		if(err != KErrNone)
			{
			OstTraceExt2(TRACE_NORMAL, CTESTCASECONTROLLER_DCTESTCASECONTROLLER, "<Error %d> Unable to unload driver: %S",err,KHostDeviceInterfaceDriverName());
			}
		
		err = User::FreeLogicalDevice(KHostDeviceDriverName);
		if(err != KErrNone)
			{
			OstTraceExt2(TRACE_NORMAL, CTESTCASECONTROLLER_DCTESTCASECONTROLLER_DUP01, "<Error %d> Unable to unload driver: %S",err,KHostDeviceDriverName());
			}
			
		err = User::FreeLogicalDevice(KOtgdiLddFileName);
		if(err != KErrNone)
			{
			OstTraceExt2(TRACE_NORMAL, CTESTCASECONTROLLER_DCTESTCASECONTROLLER_DUP02, "<Error %d> Unable to unload driver: %S",err,KHostDeviceDriverName());
			}			
		}
	else
		{
		TInt err(User::FreeLogicalDevice(KClientDeviceDriverName));
		if(err != KErrNone)
			{
			OstTraceExt2(TRACE_NORMAL, CTESTCASECONTROLLER_DCTESTCASECONTROLLER_DUP03, "<Error %d> Unable to unload driver: %S",err,KClientDeviceDriverName());
			}		
		}
	OstTraceFunctionExit1( CTESTCASECONTROLLER_CTESTCASECONTROLLER_EXIT_DUP01, this );
	}

void CTestCaseController::ConstructL()
	{
	OstTraceFunctionEntry1( CTESTCASECONTROLLER_CONSTRUCTL_ENTRY, this );
	TInt err = KErrNone;
	
	_LIT(KLoadingNamedDriverString,"loading driver: %S\n");
	_LIT(KLoadedNamedDriverString,"loaded driver: %S\n");
		
	// loading drivers
	if(iHostRole)
		{
		gtest.Printf(KLoadingNamedDriverString,&KHostDeviceDriverName);		
		OstTraceExt1(TRACE_NORMAL, CTESTCASECONTROLLER_CONSTRUCTL, "loading driver: %S\n", KHostDeviceDriverName());		
		// Load both Host USB device drivers
		err = User::LoadLogicalDevice(KHostDeviceDriverName);
		gtest((err == KErrNone) || (err == KErrAlreadyExists));
		gtest.Printf(KLoadedNamedDriverString,&KHostDeviceDriverName);
		OstTraceExt1(TRACE_NORMAL, CTESTCASECONTROLLER_CONSTRUCTL_DUP01, "loaded driver: %S\n",KHostDeviceDriverName());
		
		OstTraceExt1(TRACE_NORMAL, CTESTCASECONTROLLER_CONSTRUCTL_DUP02, "loading driver: %S\n",KHostDeviceInterfaceDriverName());
		err = User::LoadLogicalDevice(KHostDeviceInterfaceDriverName);
		gtest((err == KErrNone) || (err == KErrAlreadyExists));
		gtest.Printf(KLoadedNamedDriverString,&KHostDeviceInterfaceDriverName);
		OstTraceExt1(TRACE_NORMAL, CTESTCASECONTROLLER_CONSTRUCTL_DUP03, "loaded driver: %S\n",KHostDeviceInterfaceDriverName());
		  												 
		// If test cases are running USB host side actions
		// then run each test case in its own thread		
		iTestPolicy = CThreadTestPolicy::NewL();
		}
	else
		{
				  		
		// Load the USB client driver	
		gtest.Printf(KLoadingNamedDriverString,&KClientDeviceDriverName);
		OstTraceExt1(TRACE_NORMAL, CTESTCASECONTROLLER_CONSTRUCTL_DUP04, "loading driver: %S\n",KClientDeviceDriverName());
		err = User::LoadLogicalDevice(KClientDeviceDriverName);
		gtest((err == KErrNone) || (err == KErrAlreadyExists));
		gtest.Printf(KLoadedNamedDriverString,&KClientDeviceDriverName);
		OstTraceExt1(TRACE_NORMAL, CTESTCASECONTROLLER_CONSTRUCTL_DUP05, "loaded driver: %S\n",KClientDeviceDriverName());
		
		// Run each test case in the main thread as its not new API 
		// and not expected to panic
		iTestPolicy = CBasicTestPolicy::NewL();		
		}
	
	// Get the identity of the next test case to run	
	err = iTestEngine.NextTestCaseId(iTestCaseId);
	gtest.Next(iTestCaseId);
	
	// Run the test case	
	iTestPolicy->RunTestCaseL(iTestCaseId,iStatus);
	SetActive();
	OstTraceFunctionExit1( CTESTCASECONTROLLER_CONSTRUCTL_EXIT, this );
	}
	

void CTestCaseController::DoCancel()
	{
	OstTraceFunctionEntry1( CTESTCASECONTROLLER_DOCANCEL_ENTRY, this );
	// Cancel the outstanding test case running

	iTestPolicy->Cancel();
	OstTraceFunctionExit1( CTESTCASECONTROLLER_DOCANCEL_EXIT, this );
	}
	
	
void CTestCaseController::RunL()
	{
    OstTraceFunctionEntry1( CTESTCASECONTROLLER_RUNL_ENTRY, this );

	// Retrieve the completion code of the last test case run
	TInt err(iStatus.Int());
	
	TBuf<64> log;
	if(err != KErrNone)
		{
		iTestCasesResults.Append(EFalse);
		gtest.Printf(_L("FAILED err=%d\n"),err);
		OstTrace1(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL, "FAILED err=%d\n",err);
		}
	else
		{
		iTestCasesResults.Append(ETrue);
		gtest.Printf(_L("PASSED\n"));
		OstTrace0(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP01, "PASSED\n");
		}
		
	// Get the identity of the next test case to run
		
	err = iTestEngine.NextTestCaseId(iTestCaseId);
	if(err == KErrNone)
		{
		OstTrace0(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP02, "\n");
		OstTrace0(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP03, "\n");
		OstTrace0(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP04, "\n");
		gtest.Next(iTestCaseId);
		OstTrace0(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP05, "                              --------------------");
		OstTrace0(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP06, "\n");
		OstTrace0(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP07, "\n");
		OstTrace0(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP08, "\n");
		
		// Run the next test case
		
		iTestPolicy->RunTestCaseL(iTestCaseId,iStatus);
		SetActive();
		}
	else if(err == KErrNotFound)
		{
		OstTrace0(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP09, "All specified test cases performed");
		OstTrace0(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP10, "----------------------------------");
		
		
		// count nb failures
		TUint nbFailures = 0;
		for(TInt test = 0; test < iTestCasesResults.Count() ; test++)
			{
			if(!iTestCasesResults[test])
				//NB iTestCasesResults is a boolean array added to each time a test is run...
				//   ...even if it is a repeat.
				{
				nbFailures++;
				}
			}
		OstTraceExt2(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP11, "There are %d test case results, %d failures", iTestCasesResults.Count(), nbFailures);

		// Number of tests that should have been run (including repeats)
		TUint nbTests = iTestEngine.TestCasesIdentities().Count() * iTestEngine.NumRepeats();
		if(nbTests!=iTestCasesResults.Count())
			{
			OstTraceExt2(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP12, "The number of tests that should have been run (%d) DOES NOT EQUAL the actual number of tests run (%d).", 
						  nbTests, iTestCasesResults.Count());
			OstTrace0(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP13, "This test suite will now PANIC!");
			}
		ASSERT((nbTests==iTestCasesResults.Count()));

		
		for(TInt repeat = 0; repeat < iTestEngine.NumRepeats() ; repeat++)
			{
			if(iTestEngine.NumRepeats() > 1)
				{
				OstTrace1(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP14, "Test Case Loop %d..........",	repeat+1);			
				}
			for(TInt testIndex = 0; testIndex < iTestEngine.TestCasesIdentities().Count() ; testIndex++)
				{
				if(iTestCasesResults[testIndex])
					{
					OstTraceExt1(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP15, "Test Case: %S : PASSED",	*(iTestEngine.TestCasesIdentities())[testIndex]);			
					}
				else
					{
					OstTraceExt1(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP16, "Test Case: %S : FAILED",	*(iTestEngine.TestCasesIdentities())[testIndex]);
					}
				}
			}

		OstTrace0(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP17, "CActiveScheduler::Stop CTestCaseController::RunL");
		CActiveScheduler::Stop();
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CTESTCASECONTROLLER_RUNL_DUP18, "<Error %d> Unknown error from CTestEngine::NextTestCaseId",err);
		User::Leave(err);
		}
	OstTraceFunctionExit1( CTESTCASECONTROLLER_RUNL_EXIT, this );
	}
	
	
TInt CTestCaseController::RunError(TInt aError)
	{
	OstTraceFunctionEntryExt( CTESTCASECONTROLLER_RUNERROR_ENTRY, this );
	
	switch(aError)
		{
		case KErrNoMemory: //follow through
		default:
			// Panic the test module
			gtest(EFalse);
			break;
		}
	OstTraceFunctionExitExt( CTESTCASECONTROLLER_RUNERROR_EXIT, this, KErrNone );
	return KErrNone;
	}


	}




