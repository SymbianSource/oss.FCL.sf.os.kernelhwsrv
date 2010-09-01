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
// @file testpolicy.cpp
// @internalComponent
// 
//

#include "TestPolicy.h"
#include "TestCaseFactory.h"
#include "BaseTestCase.h"
#include "TestCaseFactory.h"
#include "testdebug.h"

namespace NUnitTesting_USBDI
	{
	
CBasicTestPolicy* CBasicTestPolicy::NewL()
	{
	CBasicTestPolicy* self = new (ELeave) CBasicTestPolicy;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

	
CBasicTestPolicy::CBasicTestPolicy()
:	CActive(EPriorityStandard)
	{
	CActiveScheduler::Add(this);
	}

	
CBasicTestPolicy::~CBasicTestPolicy()
	{
	LOG_FUNC

	Cancel();
	}
	

void CBasicTestPolicy::ConstructL()
	{
	}

	
void CBasicTestPolicy::RunTestCaseL(const TDesC& aTestCaseId,TRequestStatus& aNotifierStatus)
	{
	LOG_FUNC

	iNotifierStatus = &aNotifierStatus;

	// Create the specified test case

	iTestCase = RTestFactory::CreateTestCaseL(aTestCaseId,EFalse);
	iTestCase->SetTestPolicy(this);
	iTestCase->PerformTestL();

	// Set the request of the notifier to pending
	
	*iNotifierStatus = iStatus = KRequestPending;
	SetActive();
	}


void CBasicTestPolicy::DoCancel()
	{
	LOG_FUNC

	// Cancel running the test cases
	
	iTestCase->Cancel();

	// Notify the test case controller that test case execution was cancelled
	
	User::RequestComplete(iNotifierStatus,KErrCancel);
	}


void CBasicTestPolicy::SignalTestComplete(TInt aCompletionCode)
	{
	LOG_FUNC
	
	// Complete the test policy request with the test case completion code
	// (Basically self completion)
	
	TRequestStatus* s = &iStatus;	
	User::RequestComplete(s,aCompletionCode);
	}

	
void CBasicTestPolicy::RunL()
	{
	LOG_FUNC
	
	// Complete the request of the notifier with the test case 
	// completion code
	
	User::RequestComplete(iNotifierStatus,iStatus.Int());
	
	// Destroy the test case
	
	delete iTestCase;
	}


TInt CBasicTestPolicy::RunError(TInt aError)
	{
	LOG_FUNC
	
	aError = KErrNone;
	return aError;
	}
	
CThreadTestPolicy* CThreadTestPolicy::NewL()
	{
	CThreadTestPolicy* self = new (ELeave) CThreadTestPolicy;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
	
CThreadTestPolicy::CThreadTestPolicy()
	{
	}
	
	
CThreadTestPolicy::~CThreadTestPolicy()
	{
	iTestThread.Close();
	if(iTestCaseId)
		{
		delete iTestCaseId;
		iTestCaseId = NULL;
		}
	}

void CThreadTestPolicy::ConstructL()
	{
	}

void CThreadTestPolicy::RunTestCaseL(const TDesC& aTestCaseId,TRequestStatus& aNotifierStatus)
	{
	LOG_FUNC

	iNotifierStatus = &aNotifierStatus;
	RDebug::Printf("Creating thread for test case '%S'",&aTestCaseId);
		
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
		RDebug::Printf("Test thread creation unsuccessful: %d",err);
		User::Leave(err);
		}

	RDebug::Printf("Test thread '%S' created",&aTestCaseId);
	// Start the test case in the thread
	iTestThread.Logon(iStatus);
	SetActive();
	iTestThread.Resume();
	*iNotifierStatus = KRequestPending;
	}


void CThreadTestPolicy::SignalTestComplete(TInt aCompletionCode)
	{
	LOG_FUNC

	if(aCompletionCode == KErrNone)
		{
		RDebug::Printf("CActiveScheduler::Stop CThreadTestPolicy::SignalTestComplete");
		CActiveScheduler::Stop();
		}
	else
		{
		RDebug::Printf("Killing thread with: %d",aCompletionCode);
		iTestThread.Kill(aCompletionCode);
		}
	}

void CThreadTestPolicy::DoCancel()
	{
	LOG_FUNC

	iTestCase->Cancel();
	LOG_POINT(1)
	TInt err(iTestThread.LogonCancel(iStatus));
	if(err != KErrNone)
		{
		RDebug::Printf("Unable to cancel thread logon: %d",err);
		}
	LOG_POINT(2)
	TRequestStatus cancelStatus;
	iTestThread.Logon(cancelStatus);
	LOG_POINT(3)
	iTestThread.Kill(KErrCancel);		
	LOG_POINT(4)
	User::RequestComplete(iNotifierStatus,cancelStatus.Int());
	}
	
TInt CThreadTestPolicy::ThreadFunction(TAny* aThreadParameter)
	{
	LOG_CFUNC

	TInt err(KErrNone);
	TInt leaveCode(KErrNone);
	
	HBufC* testCaseId = reinterpret_cast<HBufC*>(aThreadParameter);
	
	// Create the cleanup stack for this thread
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if(cleanup == NULL)
		{
		return KErrNoMemory;
		}
		
	TRAP(leaveCode,err = CThreadTestPolicy::DoTestL(*testCaseId));
	if(leaveCode != KErrNone)
		{
		RDebug::Printf("<Error %d> Thread '%S' DoTest",leaveCode,&testCaseId);
		err = leaveCode;
		}
	
	delete cleanup;
	return err;
	}
	
	
TInt CThreadTestPolicy::DoTestL(const TDesC& aTestCaseId)
	{
	LOG_CFUNC
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
		RDebug::Printf("CActiveScheduler::Start in CThreadTestPolicy::DoTestL");	
		CActiveScheduler::Start();
		}	
	// Get the test case execution result 
	err = testCase->TestResult();
	
	// Destroy test case
	CleanupStack::PopAndDestroy(testCase);
	
	// Destroy the active scheduler	
	CleanupStack::PopAndDestroy(sched);
	
	return err;
	}

void CThreadTestPolicy::RunL()
	{
	LOG_FUNC
	TInt completionCode(iStatus.Int());
	
	TExitType exitType(iTestThread.ExitType());
	TExitCategoryName exitName(iTestThread.ExitCategory());
	TInt exitReason(iTestThread.ExitReason());
	RDebug::Printf("Test thread '%S' completed with completion code %d",&iTestThread.Name(),completionCode);
		
	switch(exitType)
		{
		// The test thread has panicked
		// This will occur if test API panics or RTest expression is false (i.e. test case fails)
		case EExitPanic:
			{
			RDebug::Printf("Test thread '%S' has panicked with category '%S' reason %d",
						&iTestThread.Name(),&exitName,exitReason);
			// May require to stop and start host/client USB depending on what panic category it is
			// can no longer trust RUsbHubDriver/RDevUsbcClient to be in good state
			completionCode = KErrAbort;
			}
			break;
				
		// The thread has been terminated
		case EExitTerminate:
			{
			RDebug::Printf("Test thread '%S' terminated with category %s reason %d",
				&iTestThread.Name(),&exitName,exitReason);
			}
			break;
				
		// The thread has been killed
		// This will occur when the test thread executes normally or is cancelled
		case EExitKill:
			{
			RDebug::Printf("Test thread '%S' has been killed with reason %d",&iTestThread.Name(),exitReason);
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
	}

TInt CThreadTestPolicy::RunError(TInt aError)
	{
	RDebug::Printf("<Error %d><Test Policy> RunError",aError);
	return KErrNone;
	}
	
	}
