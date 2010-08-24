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
// @file testpolicy.cpp
// @internalComponent
// 
//

#include "TestPolicy.h"
#include "TestCaseFactory.h"
#include "BaseTestCase.h"
#include "TestCaseFactory.h"
#include "testdebug.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "TestPolicyTraces.h"
#endif

namespace NUnitTesting_USBDI
	{
	
CBasicTestPolicy* CBasicTestPolicy::NewL()
	{
	OstTraceFunctionEntry0( CBASICTESTPOLICY_NEWL_ENTRY );
	CBasicTestPolicy* self = new (ELeave) CBasicTestPolicy;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CBASICTESTPOLICY_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}

	
CBasicTestPolicy::CBasicTestPolicy()
:	CActive(EPriorityStandard)
	{
	OstTraceFunctionEntry1( CBASICTESTPOLICY_CBASICTESTPOLICY_ENTRY, this );
	CActiveScheduler::Add(this);
	OstTraceFunctionExit1( CBASICTESTPOLICY_CBASICTESTPOLICY_EXIT, this );
	}

	
CBasicTestPolicy::~CBasicTestPolicy()
	{
    OstTraceFunctionEntry1( CBASICTESTPOLICY_CBASICTESTPOLICY_ENTRY_DUP01, this );

	Cancel();
	OstTraceFunctionExit1( CBASICTESTPOLICY_CBASICTESTPOLICY_EXIT_DUP01, this );
	}
	

void CBasicTestPolicy::ConstructL()
	{
	OstTraceFunctionEntry1( CBASICTESTPOLICY_CONSTRUCTL_ENTRY, this );
	OstTraceFunctionExit1( CBASICTESTPOLICY_CONSTRUCTL_EXIT, this );
	}

	
void CBasicTestPolicy::RunTestCaseL(const TDesC& aTestCaseId,TRequestStatus& aNotifierStatus)
	{
    OstTraceFunctionEntryExt( CBASICTESTPOLICY_RUNTESTCASEL_ENTRY, this );

	iNotifierStatus = &aNotifierStatus;

	// Create the specified test case

	iTestCase = RTestFactory::CreateTestCaseL(aTestCaseId,EFalse);
	iTestCase->SetTestPolicy(this);
	iTestCase->PerformTestL();

	// Set the request of the notifier to pending
	
	*iNotifierStatus = iStatus = KRequestPending;
	SetActive();
	OstTraceFunctionExit1( CBASICTESTPOLICY_RUNTESTCASEL_EXIT, this );
	}


void CBasicTestPolicy::DoCancel()
	{
    OstTraceFunctionEntry1( CBASICTESTPOLICY_DOCANCEL_ENTRY, this );

	// Cancel running the test cases
	
	iTestCase->Cancel();

	// Notify the test case controller that test case execution was cancelled
	
	User::RequestComplete(iNotifierStatus,KErrCancel);
	OstTraceFunctionExit1( CBASICTESTPOLICY_DOCANCEL_EXIT, this );
	}


void CBasicTestPolicy::SignalTestComplete(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CBASICTESTPOLICY_SIGNALTESTCOMPLETE_ENTRY, this );
	
	// Complete the test policy request with the test case completion code
	// (Basically self completion)
	
	TRequestStatus* s = &iStatus;	
	User::RequestComplete(s,aCompletionCode);
	OstTraceFunctionExit1( CBASICTESTPOLICY_SIGNALTESTCOMPLETE_EXIT, this );
	}

	
void CBasicTestPolicy::RunL()
	{
	OstTraceFunctionEntry1( CBASICTESTPOLICY_RUNL_ENTRY, this );
	
	// Complete the request of the notifier with the test case 
	// completion code
	
	User::RequestComplete(iNotifierStatus,iStatus.Int());
	
	// Destroy the test case
	
	delete iTestCase;
	OstTraceFunctionExit1( CBASICTESTPOLICY_RUNL_EXIT, this );
	}


TInt CBasicTestPolicy::RunError(TInt aError)
	{
	OstTraceFunctionEntryExt( CBASICTESTPOLICY_RUNERROR_ENTRY, this );
	
	aError = KErrNone;
	OstTraceFunctionExitExt( CBASICTESTPOLICY_RUNERROR_EXIT, this, aError );
	return aError;
	}
	
CThreadTestPolicy* CThreadTestPolicy::NewL()
	{
	OstTraceFunctionEntry0( CTHREADTESTPOLICY_NEWL_ENTRY );
	CThreadTestPolicy* self = new (ELeave) CThreadTestPolicy;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CTHREADTESTPOLICY_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	
	
CThreadTestPolicy::CThreadTestPolicy()
	{
	OstTraceFunctionEntry1( CTHREADTESTPOLICY_CTHREADTESTPOLICY_ENTRY, this );
	OstTraceFunctionExit1( CTHREADTESTPOLICY_CTHREADTESTPOLICY_EXIT, this );
	}
	
	
CThreadTestPolicy::~CThreadTestPolicy()
	{
	OstTraceFunctionEntry1( CTHREADTESTPOLICY_CTHREADTESTPOLICY_ENTRY_DUP01, this );
	iTestThread.Close();
	if(iTestCaseId)
		{
		delete iTestCaseId;
		iTestCaseId = NULL;
		}
	OstTraceFunctionExit1( CTHREADTESTPOLICY_CTHREADTESTPOLICY_EXIT_DUP01, this );
	}

void CThreadTestPolicy::ConstructL()
	{
	OstTraceFunctionEntry1( CTHREADTESTPOLICY_CONSTRUCTL_ENTRY, this );
	OstTraceFunctionExit1( CTHREADTESTPOLICY_CONSTRUCTL_EXIT, this );
	}

void CThreadTestPolicy::RunTestCaseL(const TDesC& aTestCaseId,TRequestStatus& aNotifierStatus)
	{
    OstTraceFunctionEntryExt( CTHREADTESTPOLICY_RUNTESTCASEL_ENTRY, this );

	iNotifierStatus = &aNotifierStatus;
	OstTraceExt1(TRACE_NORMAL, CTHREADTESTPOLICY_RUNTESTCASEL, "Creating thread for test case '%S'",aTestCaseId);
		
	if(iTestCaseId)
		{
		delete iTestCaseId;
		iTestCaseId = NULL;
		}
	iTestCaseId = HBufC::NewL(aTestCaseId.Length()); 
	*iTestCaseId = aTestCaseId;
		
	// Create the thread to run the test case in
	TInt err(iTestThread.Create(aTestCaseId,CThreadTestPolicy::ThreadFunction,
			KDefaultStackSize,NULL,reinterpret_cast<TAny*>(iTestCaseId)));
	
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CTHREADTESTPOLICY_RUNTESTCASEL_DUP01, "Test thread creation unsuccessful: %d",err);
		User::Leave(err);
		}

	OstTraceExt1(TRACE_NORMAL, CTHREADTESTPOLICY_RUNTESTCASEL_DUP02, "Test thread '%S' created",aTestCaseId);
	// Start the test case in the thread
	iTestThread.Logon(iStatus);
	SetActive();
	iTestThread.Resume();
	*iNotifierStatus = KRequestPending;
	OstTraceFunctionExit1( CTHREADTESTPOLICY_RUNTESTCASEL_EXIT, this );
	}


void CThreadTestPolicy::SignalTestComplete(TInt aCompletionCode)
	{
    OstTraceFunctionEntryExt( CTHREADTESTPOLICY_SIGNALTESTCOMPLETE_ENTRY, this );

	if(aCompletionCode == KErrNone)
		{
		OstTrace0(TRACE_NORMAL, CTHREADTESTPOLICY_SIGNALTESTCOMPLETE, "CActiveScheduler::Stop CThreadTestPolicy::SignalTestComplete");
		CActiveScheduler::Stop();
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CTHREADTESTPOLICY_SIGNALTESTCOMPLETE_DUP01, "Killing thread with: %d",aCompletionCode);
		iTestThread.Kill(aCompletionCode);
		}
	OstTraceFunctionExit1( CTHREADTESTPOLICY_SIGNALTESTCOMPLETE_EXIT, this );
	}

void CThreadTestPolicy::DoCancel()
	{
    OstTraceFunctionEntry1( CTHREADTESTPOLICY_DOCANCEL_ENTRY, this );

	iTestCase->Cancel();
	OstTrace0(TRACE_NORMAL, CTHREADTESTPOLICY_DOCANCEL, ">> Debug point: 1");
	TInt err(iTestThread.LogonCancel(iStatus));
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CTHREADTESTPOLICY_DOCANCEL_DUP01, "Unable to cancel thread logon: %d",err);
		}
	OstTrace0(TRACE_NORMAL, CTHREADTESTPOLICY_DOCANCEL_DUP02, ">> Debug point: 2");
	TRequestStatus cancelStatus;
	iTestThread.Logon(cancelStatus);
	OstTrace0(TRACE_NORMAL, CTHREADTESTPOLICY_DOCANCEL_DUP03, ">> Debug point: 3");
	iTestThread.Kill(KErrCancel);		
	OstTrace0(TRACE_NORMAL, CTHREADTESTPOLICY_DOCANCEL_DUP04, ">> Debug point: 4");
	User::RequestComplete(iNotifierStatus,cancelStatus.Int());
	OstTraceFunctionExit1( CTHREADTESTPOLICY_DOCANCEL_EXIT, this );
	}
	
TInt CThreadTestPolicy::ThreadFunction(TAny* aThreadParameter)
	{
	OstTraceFunctionEntry0( CTHREADTESTPOLICY_THREADFUNCTION_ENTRY);

	TInt err(KErrNone);
	TInt leaveCode(KErrNone);
	
	HBufC* testCaseId = reinterpret_cast<HBufC*>(aThreadParameter);
	
	// Create the cleanup stack for this thread
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if(cleanup == NULL)
		{
		OstTraceFunctionExitExt( CTHREADTESTPOLICY_THREADFUNCTION_EXIT, 0, KErrNoMemory );
		return KErrNoMemory;
		}
		
	TRAP(leaveCode,err = CThreadTestPolicy::DoTestL(*testCaseId));
	if(leaveCode != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CTHREADTESTPOLICY_THREADFUNCTION, "<Error %d> Thread '%S' DoTest",leaveCode, *testCaseId);
		err = leaveCode;
		}
	
	delete cleanup;
	OstTraceFunctionExitExt( CTHREADTESTPOLICY_THREADFUNCTION_EXIT_DUP01, 0, err );
	return err;
	}
	
	
TInt CThreadTestPolicy::DoTestL(const TDesC& aTestCaseId)
	{
	OstTraceFunctionEntryExt( CTHREADTESTPOLICY_DOTESTL_ENTRY, 0 );
	TInt err(KErrNone);
	
	// Create a new active scheduler for this thread
	CActiveScheduler* sched = new (ELeave) CActiveScheduler;
	CleanupStack::PushL(sched);
	CActiveScheduler::Install(sched);

	CBaseTestCase* testCase = RTestFactory::CreateTestCaseL(aTestCaseId,ETrue);
	CleanupStack::PushL(testCase);
		
	// Do the test
	testCase->PerformTestL();
	
	if(!testCase->IsHostOnly())
		{
		// Loop for active objects
		OstTrace0(TRACE_NORMAL, CTHREADTESTPOLICY_DOTESTL, "CActiveScheduler::Start in CThreadTestPolicy::DoTestL");
		CActiveScheduler::Start();
		}	
	// Get the test case execution result 
	err = testCase->TestResult();
	
	// Destroy test case
	CleanupStack::PopAndDestroy(testCase);
	
	// Destroy the active scheduler	
	CleanupStack::PopAndDestroy(sched);
	
	OstTraceFunctionExitExt( CTHREADTESTPOLICY_DOTESTL_EXIT, 0, err );
	return err;
	}

void CThreadTestPolicy::RunL()
	{
	OstTraceFunctionEntry1( CTHREADTESTPOLICY_RUNL_ENTRY, this );
	TInt completionCode(iStatus.Int());
	
	TExitType exitType(iTestThread.ExitType());
	TExitCategoryName exitName(iTestThread.ExitCategory());
	TInt exitReason(iTestThread.ExitReason());
	OstTraceExt2(TRACE_NORMAL, CTHREADTESTPOLICY_RUNL, "Test thread '%S' completed with completion code %d",iTestThread.Name(),completionCode);
		
	switch(exitType)
		{
		// The test thread has panicked
		// This will occur if test API panics or RTest expression is false (i.e. test case fails)
		case EExitPanic:
			{
			OstTraceExt3(TRACE_NORMAL, CTHREADTESTPOLICY_RUNL_DUP01, "Test thread '%S' has panicked with category '%S' reason %d",
						iTestThread.Name(),exitName,exitReason);
			// May require to stop and start host/client USB depending on what panic category it is
			// can no longer trust RUsbHubDriver/RDevUsbcClient to be in good state
			completionCode = KErrAbort;
			}
			break;
				
		// The thread has been terminated
		case EExitTerminate:
			{
			OstTraceExt3(TRACE_NORMAL, CTHREADTESTPOLICY_RUNL_DUP02, "Test thread '%S' terminated with category %s reason %d",
				iTestThread.Name(),exitName,exitReason);
			}
			break;
				
		// The thread has been killed
		// This will occur when the test thread executes normally or is cancelled
		case EExitKill:
			{
			OstTraceExt2(TRACE_NORMAL, CTHREADTESTPOLICY_RUNL_DUP03, "Test thread '%S' has been killed with reason %d",iTestThread.Name(),exitReason);
			}
			break;
				
		// These cases should not occur at this stage
		case EExitPending: //follow through
		default:
			gtest(EFalse); // Panic main thread
			break;
		}

	// Close the just executed test thread
	iTestThread.Close();
		
	// Complete the notifier's request status
	User::RequestComplete(iNotifierStatus,completionCode);
	OstTraceFunctionExit1( CTHREADTESTPOLICY_RUNL_EXIT, this );
	}

TInt CThreadTestPolicy::RunError(TInt aError)
	{
	OstTraceFunctionEntryExt( CTHREADTESTPOLICY_RUNERROR_ENTRY, this );
	OstTrace1(TRACE_NORMAL, CTHREADTESTPOLICY_RUNERROR, "<Error %d><Test Policy> RunError",aError);
	OstTraceFunctionExitExt( CTHREADTESTPOLICY_RUNERROR_EXIT, this, KErrNone );
	return KErrNone;
	}
	
	}
