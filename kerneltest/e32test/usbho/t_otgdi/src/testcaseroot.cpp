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
	LOG_FUNC
	

	TInt err(iTimer.CreateLocal());
	if (err == KErrNone)
		{
		LOG_VERBOSE1(_L("Test case timer created"));
		}
	else
		{
		RDebug::Printf("<Error %d> Test case timer could not be created",err);
		User::Leave(err);
		}
	//
	iConsole = test.Console();
	iRequestedChar = EFalse;
	}
	
	
CTestCaseRoot::~CTestCaseRoot()
	{
	LOG_FUNC
	Cancel();
	}

// utility GUI methods
void CTestCaseRoot::DisplayTestCaseOptions()
	{
	LOG_FUNC

	// commonly overridden to display any options for that test
	test.Printf(_L("Press <ESC> to end the test.\n"));	
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
	LOG_FUNC
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
	LOG_FUNC
	
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
	LOG_FUNC
	  // A request is issued to the CConsoleBase to accept a
	  // character from the keyboard.

	__ASSERT_ALWAYS(!IsActive(), User::Panic(KMsgAlreadyActive, EPanicAlreadyActive));
	  
	iConsole->Read(iStatus); 
	SetActive();
	iRequestedChar = ETrue;
	}


void CTestCaseRoot::RunL()
	{
	LOG_FUNC
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
		RunStepL();
		LOG_VERBOSE3(_L(">> RunStepL() step=%d->%d\n"), currentStep, GetStepIndex());
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
	LOG_FUNC
	test.Printf(_L("Test case C%lS::RunL left with %d"), &iTestCaseId, aError);
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
	LOG_FUNC
	iTestResult = aFailResult;
	test.Printf(_L("Test %S\n"), &TestCaseId());
	test.Printf(_L("Failed (%d)\n"), aFailResult);
	test.Printf(_L("%S!\n"), &aErrorDescription);
	if (!iAutomated)
		{
		test.Printf(_L("\n"));
		test.Printf(KPressAnyKeyToContinue);
		iConsole->Getch();
		}
	// the next call panics the framework!
	TestPolicy().SignalTestComplete(iTestResult);
	}
	
	
void CTestCaseRoot::TestFailed2(TInt aFailResult, const TDesC &aErrorDescription, TInt errorCode)
	{
	LOG_FUNC
	iTestResult = aFailResult;
	test.Printf(_L("Test %S FAILED %d '%S %d'!\n"), 
	            &TestCaseId(), 
	            aFailResult, 
	            &aErrorDescription, 
	            errorCode);
	// the next call panics the framework!
	TestPolicy().SignalTestComplete(iTestResult);
	}
	
	
	
void CTestCaseRoot::TestPassed()
	{
	LOG_FUNC
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
		// B2B class method dumps the engine state
		//
		test.Printf(_L("\n--------------\n"));
		}
	}


/* **************************************************************************************
 * 
 */
CTestCaseB2BRoot::CTestCaseB2BRoot(const TDesC& aTestCaseId, TBool aHost, TRequestStatus &aStatus) 
	: CTestCaseRoot(aTestCaseId, aHost) , iCollector(aStatus)
	{
	LOG_FUNC
	
	}


CTestCaseB2BRoot::~CTestCaseB2BRoot()
	{
	LOG_FUNC
	
	}

/* Print step name : B2B override displays the OTG Engine state as well
 */
void CTestCaseB2BRoot::PrintStepName(const TDesC &aStepName)
	{
	if (gVerboseOutput) 
		{
		test.Printf(_L("--------------\n %S "), &aStepName);
		// engine state
		CNotifyWatcherBase *pWatcher = iCollector.GetWatcher(EWatcherState);
		if (pWatcher)
			{
			TBuf<MAX_DSTRLEN> aDescription;
			RUsbOtgDriver::TOtgState aState = static_cast<RUsbOtgDriver::TOtgState>(pWatcher->GetEventValue());
			
			OtgStateString(aState, aDescription);
			LOG_VERBOSE3(_L("OTGState %d '%S' \n"), aState, &aDescription);
			}
		test.Printf(_L(" : time = %dms\n--------------\n"), iCollector.DurationElapsed());
		}
	}

/* Default implementation which describes this as a B2B test-case 
 */
void CTestCaseB2BRoot::DescribePreconditions()
	{
	test.Printf(KTestTypeB2BMsg); // B2B
	if (gTestRoleMaster)
		test.Printf(KRoleMasterMsg);
	else
		test.Printf(KRoleSlaveMsg);
	}


void CTestCaseB2BRoot::StepB2BPreconditions()
	{
	// prompt to insert connector
	if (gTestRoleMaster)
		{ // "B" device
		test.Printf(KInsertBCablePrompt);
		}
	else
		{
		test.Printf(KInsertACablePrompt);
		}
	if (iAutomated)
		{

		SelfComplete();
		return;
		}
	test.Printf(KPressAnyKeyToContinue);
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
			return TestFailed(KErrAbort, KMsgBPlugNotFound);
			}
		}
	else
		{
		if (!otgIdPinPresent())
			{ // oops
			test.Printf(KMsgErrorPreconditionFailed);
			return TestFailed(KErrAbort, KMsgAPlugNotFound);
			}
		}
	}


/* Clears the expected event Q just before calling the step, but not the 
 * recieved Q since the step may still inspect it using the EventReceivedAlready() method.
 */
void CTestCaseB2BRoot::PreRunStep()
	{
	LOG_FUNC
		
		iCollector.ClearAllEvents(EFalse, ETrue);
	}


void CTestCaseB2BRoot::PostRunStep()
	{
	LOG_FUNC
		// clear the recieved event Q, but not the expected Q
		iCollector.ClearAllEvents(ETrue, EFalse);
	
	}



