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
// @file testcaseroot.cpp
// @internalComponent
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <u32std.h> 	// unicode builds
#include <e32base.h>
#include <e32base_private.h>
#include <e32cons.h>
#include <e32Test.h>	// RTest header
//#include <e32ver.h>
#include <e32def.h>
#include <e32def_private.h>
#include <d32otgdi.h>		// OTGDI header
#include <d32usbc.h>		// USBCC header
#include "testcaseroot.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcaserootTraces.h"
#endif

/* Implemention of classes CTestCaseRoot, CTestCaseB2BRoot
 *
 */ 

/* ===============================================================
 *  The root test-case class, provides test-framework features  
 */
CTestCaseRoot::CTestCaseRoot(const TDesC& aTestCaseId, TBool aAutomation)
:	CActive(EPriorityUserInput),
	COtgRoot(),
	iAutomated(aAutomation)
	{
	iTestCaseId.Copy(aTestCaseId);
	}

	
void CTestCaseRoot::BaseConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASEROOT_BASECONSTRUCTL);
	    }
	

	TInt err(iTimer.CreateLocal());
	if (err == KErrNone)
		{
		LOG_VERBOSE1(_L("Test case timer created"));
		if(gVerboseOutput)
		    {
		    OstTrace0(TRACE_VERBOSE, CTESTCASEROOT_BASECONSTRUCTL_DUP01, "Test case timer created");
		    }
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CTESTCASEROOT_BASECONSTRUCTL_DUP02, "<Error %d> Test case timer could not be created",err);
		User::Leave(err);
		}
	//
	iConsole = test.Console();
	iRequestedChar = EFalse;
	}
	
	
CTestCaseRoot::~CTestCaseRoot()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASEROOT_DCTESTCASEROOT);
	    }
	Cancel();
	}

// utility GUI methods
void CTestCaseRoot::DisplayTestCaseOptions()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASEROOT_DISPLAYTESTCASEOPTIONS);
	    }

	// commonly overridden to display any options for that test
	test.Printf(_L("Press <ESC> to end the test.\n"));	
	OstTrace0(TRACE_NORMAL, CTESTCASEROOT_DISPLAYTESTCASEOPTIONS_DUP01, "Press <ESC> to end the test.\n");	
	}

    
/***********************************************************************/
//
void CTestCaseRoot::SetTestPolicy(CBasicTestPolicy* aTestPolicy)
	{
	iTestPolicy = aTestPolicy;
	iAutomated = gSemiAutomated;	// read the global flag at this time
	}


CBasicTestPolicy& CTestCaseRoot::TestPolicy()
	{
	return *iTestPolicy;
	}
	

void CTestCaseRoot::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASEROOT_DOCANCEL);
	    }
	}

    
/** ProcessKey
override this method to perform other tasks, the base-class does nothing so that 
child classes can use it as an any-key-to-continue helper
*/
void CTestCaseRoot::ProcessKey(TKeyCode &aKey)
	{// default implementation - reads the key.
		iKeyCodeInput = aKey;
	}
    
    
void CTestCaseRoot::ProcessEngineKey(TKeyCode &aKey)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASEROOT_PROCESSENGINEKEY);
	    }
	
	if (EKeyEscape == aKey)
		{
		AssertionFailed( 1, _L("Test aborted by the user <ESC>\n"));
		CActiveScheduler::Stop();
		}
	else
		{
			
		// call virtual method here to invoke the test's overridden method
		ProcessKey(aKey);
		// note, we do not go ask for another, that is done by the console handler
		}
	}
    

void CTestCaseRoot::RequestCharacter()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASEROOT_REQUESTCHARACTER);
	    }
	  // A request is issued to the CConsoleBase to accept a
	  // character from the keyboard.

	__ASSERT_ALWAYS(!IsActive(), User::Panic(KMsgAlreadyActive, EPanicAlreadyActive));
	  
	iConsole->Read(iStatus); 
	SetActive();
	iRequestedChar = ETrue;
	}


void CTestCaseRoot::RunL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASEROOT_RUNL);
	    }
	TInt complCode(iStatus.Int());
	if (iRequestedChar)
		{
		
		TKeyCode k(iConsole->KeyCode());
		
		// WARNING!
		// EMULATOR QUIRK!
		// When debugging if you have a breakpoint just before you read/process a key, 
		// the EMULATOR will read the key sent to the IDE and you get a (F5) key that 
		// you did not want from the IDE not the emulator.
		
		ProcessEngineKey(k);
		iRequestedChar = EFalse;

		// complete so that other tasks (there are none) can run
		// we will then be scheduled by the ActiveScheduler again to run the next test step.
		SelfComplete();
		}
	else
		{
		// run the next test step
		TInt currentStep(GetStepIndex());
		PreRunStep();
		LOG_VERBOSE2(_L("\n<< RunStepL() step=%d\n"), currentStep);
		if(gVerboseOutput)
		    {
		    OstTrace1(TRACE_VERBOSE, CTESTCASEROOT_RUNL_DUP01, "\n<< RunStepL() step=%d\n", currentStep);
		    }
		RunStepL();
		LOG_VERBOSE3(_L(">> RunStepL() step=%d->%d\n"), currentStep, GetStepIndex());
		if(gVerboseOutput)
		    {
		    OstTraceExt2(TRACE_VERBOSE, CTESTCASEROOT_RUNL_DUP02, ">> RunStepL() step=%d->%d\n", currentStep, GetStepIndex());
		    }
		PostRunStep();
		}
	}


void CTestCaseRoot::PostRunStep()
	{
		// default impl.
	}

void CTestCaseRoot::PreRunStep()
	{
		// default impl.
	}

TInt CTestCaseRoot::RunError(TInt aError)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASEROOT_RUNERROR);
	    }
	test.Printf(_L("Test case C%lS::RunL left with %d"), &iTestCaseId, aError);
	OstTraceExt2(TRACE_NORMAL, CTESTCASEROOT_RUNERROR_DUP01, "Test case C%lS::RunL left with %d", iTestCaseId, aError);
	AssertionFailed(aError, _L("RunError"));
	return KErrNone;
	}
	
	
TDesC& CTestCaseRoot::TestCaseId()
	{
	return iTestCaseId;
	}
	
	
TInt CTestCaseRoot::TestResult() const
	{
	return iTestResult;
	}
	
	
void CTestCaseRoot::PerformTestL()
	{
		// tell user what they should have done before starting!
		DescribePreconditions();
		// run it
		ExecuteTestCaseL();
	}


void CTestCaseRoot::TestFailed(TInt aFailResult, const TDesC &aErrorDescription)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASEROOT_TESTFAILED);
	    }
	iTestResult = aFailResult;
	test.Printf(_L("Test %S\n"), &TestCaseId());
	OstTraceExt1(TRACE_NORMAL, CTESTCASEROOT_TESTFAILED_DUP01, "Test %S\n", TestCaseId());
	test.Printf(_L("Failed (%d)\n"), aFailResult);
	OstTrace1(TRACE_NORMAL, CTESTCASEROOT_TESTFAILED_DUP02, "Failed (%d)\n", aFailResult);
	test.Printf(_L("%S!\n"), &aErrorDescription);
	OstTraceExt1(TRACE_NORMAL, CTESTCASEROOT_TESTFAILED_DUP03, "%S!\n", aErrorDescription);
	if (!iAutomated)
		{
		test.Printf(_L("\n"));
		OstTrace0(TRACE_NORMAL, CTESTCASEROOT_TESTFAILED_DUP04, "\n");
		test.Printf(KPressAnyKeyToContinue);
		OstTrace0(TRACE_NORMAL, CTESTCASEROOT_TESTFAILED_DUP05, KPressAnyKeyToContinue);
		iConsole->Getch();
		}
	// the next call panics the framework!
	TestPolicy().SignalTestComplete(iTestResult);
	}
	
	
void CTestCaseRoot::TestFailed2(TInt aFailResult, const TDesC &aErrorDescription, TInt errorCode)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASEROOT_TESTFAILED2);
	    }
	iTestResult = aFailResult;
	test.Printf(_L("Test %S FAILED %d '%S %d'!\n"), 
	            &TestCaseId(), 
	            aFailResult, 
	            &aErrorDescription, 
	            errorCode);
	OstTraceExt4(TRACE_NORMAL, CTESTCASEROOT_TESTFAILED2_DUP01, "Test %S FAILED %d '%S %d'!\n", 
	            TestCaseId(), 
	            aFailResult, 
	            aErrorDescription, 
	            errorCode);
	// the next call panics the framework!
	TestPolicy().SignalTestComplete(iTestResult);
	}
	
	
	
void CTestCaseRoot::TestPassed()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASEROOT_TESTPASSED);
	    }
	iTestResult = KErrNone;
	TestPolicy().SignalTestComplete(iTestResult);
	}
	
	
	
			
/** Complete the servicing of the Active object and signal it ready to run again 
 in next scheduler slot. This method works as drop-through when a test step has 
 nothing more to do .
*/
void CTestCaseRoot::SelfComplete(TInt aCode)
	{	
	TRequestStatus* s = &iStatus;
	iStatus = KRequestPending;
	User::RequestComplete(s, aCode);
	SetActive();
	}

/* Prints a little banner at the top of a test-step : 
 * B2B tests also display the duration of the last step, and the current OTG state
 */
void CTestCaseRoot::PrintStepName(const TDesC &aStepName)
	{
	if (gVerboseOutput) 
		{
		test.Printf(_L("--------------\n %S "), &aStepName);
		OstTraceExt1(TRACE_NORMAL, CTESTCASEROOT_PRINTSTEPNAME, "--------------\n %S ", aStepName);
		// B2B class method dumps the engine state
		//
		test.Printf(_L("\n--------------\n"));
		OstTrace0(TRACE_NORMAL, CTESTCASEROOT_PRINTSTEPNAME_DUP01, "\n--------------\n");
		}
	}


/* **************************************************************************************
 * 
 */
CTestCaseB2BRoot::CTestCaseB2BRoot(const TDesC& aTestCaseId, TBool aHost, TRequestStatus &aStatus) 
	: CTestCaseRoot(aTestCaseId, aHost) , iCollector(aStatus)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASEB2BROOT_CTESTCASEB2BROOT);
	    }
	
	}


CTestCaseB2BRoot::~CTestCaseB2BRoot()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASEB2BROOT_DCTESTCASEB2BROOT);
	    }
	
	}

/* Print step name : B2B override displays the OTG Engine state as well
 */
void CTestCaseB2BRoot::PrintStepName(const TDesC &aStepName)
	{
	if (gVerboseOutput) 
		{
		test.Printf(_L("--------------\n %S "), &aStepName);
		OstTraceExt1(TRACE_NORMAL, CTESTCASEB2BROOT_PRINTSTEPNAME, "--------------\n %S ", aStepName);
		// engine state
		CNotifyWatcherBase *pWatcher = iCollector.GetWatcher(EWatcherState);
		if (pWatcher)
			{
			TBuf<MAX_DSTRLEN> aDescription;
			RUsbOtgDriver::TOtgState aState = static_cast<RUsbOtgDriver::TOtgState>(pWatcher->GetEventValue());
			
			OtgStateString(aState, aDescription);
			LOG_VERBOSE3(_L("OTGState %d '%S' \n"), aState, &aDescription);
			if(gVerboseOutput)
			    {
			    OstTraceExt2(TRACE_VERBOSE, CTESTCASEB2BROOT_PRINTSTEPNAME_DUP01, "OTGState %d '%S' \n", aState, aDescription);
			    }
			}
		test.Printf(_L(" : time = %dms\n--------------\n"), iCollector.DurationElapsed());
		OstTrace1(TRACE_NORMAL, CTESTCASEB2BROOT_PRINTSTEPNAME_DUP02, " : time = %dms\n--------------\n", iCollector.DurationElapsed());
		}
	}

/* Default implementation which describes this as a B2B test-case 
 */
void CTestCaseB2BRoot::DescribePreconditions()
	{
	test.Printf(KTestTypeB2BMsg); // B2B
	OstTrace0(TRACE_NORMAL, CTESTCASEB2BROOT_DESCRIBEPRECONDITIONS, KTestTypeB2BMsg); // B2B
	if (gTestRoleMaster)
	   {
		test.Printf(KRoleMasterMsg);
		OstTrace0(TRACE_NORMAL, CTESTCASEB2BROOT_DESCRIBEPRECONDITIONS_DUP01, KRoleMasterMsg);
		}
	else
	    {
		test.Printf(KRoleSlaveMsg);
		OstTrace0(TRACE_NORMAL, CTESTCASEB2BROOT_DESCRIBEPRECONDITIONS_DUP02, KRoleSlaveMsg);
		}
	}


void CTestCaseB2BRoot::StepB2BPreconditions()
	{
	// prompt to insert connector
	if (gTestRoleMaster)
		{ // "B" device
		test.Printf(KInsertBCablePrompt);
		OstTrace0(TRACE_NORMAL, CTESTCASEB2BROOT_STEPB2BPRECONDITIONS, KInsertBCablePrompt);
		}
	else
		{
		test.Printf(KInsertACablePrompt);
		OstTrace0(TRACE_NORMAL, CTESTCASEB2BROOT_STEPB2BPRECONDITIONS_DUP01, KInsertACablePrompt);
		}
	if (iAutomated)
		{

		SelfComplete();
		return;
		}
	test.Printf(KPressAnyKeyToContinue);
	OstTrace0(TRACE_NORMAL, CTESTCASEB2BROOT_STEPB2BPRECONDITIONS_DUP02, KPressAnyKeyToContinue);
	RequestCharacter();	
	}


/* Test for A or B plug as relevant for MASTER/SLAVE role, fail the test if wrong
 */
void CTestCaseB2BRoot::CheckRoleConnections()
	{
	if (gTestRoleMaster)
		{
		if (otgIdPinPresent())
			{ // oops
			test.Printf(KMsgErrorPreconditionFailed);
			OstTrace0(TRACE_NORMAL, CTESTCASEB2BROOT_CHECKROLECONNECTIONS, KMsgErrorPreconditionFailed);
			return TestFailed(KErrAbort, KMsgBPlugNotFound);
			}
		}
	else
		{
		if (!otgIdPinPresent())
			{ // oops
			test.Printf(KMsgErrorPreconditionFailed);
			OstTrace0(TRACE_NORMAL, CTESTCASEB2BROOT_CHECKROLECONNECTIONS_DUP01, KMsgErrorPreconditionFailed);
			return TestFailed(KErrAbort, KMsgAPlugNotFound);
			}
		}
	}


/* Clears the expected event Q just before calling the step, but not the 
 * recieved Q since the step may still inspect it using the EventReceivedAlready() method.
 */
void CTestCaseB2BRoot::PreRunStep()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASEB2BROOT_PRERUNSTEP);
	    }
		
		iCollector.ClearAllEvents(EFalse, ETrue);
	}


void CTestCaseB2BRoot::PostRunStep()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASEB2BROOT_POSTRUNSTEP);
	    }
		// clear the recieved event Q, but not the expected Q
		iCollector.ClearAllEvents(ETrue, EFalse);
	
	}



