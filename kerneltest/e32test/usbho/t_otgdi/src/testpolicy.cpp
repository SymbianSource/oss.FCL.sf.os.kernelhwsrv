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
#include <e32cons.h>
#include <e32Test.h>	// RTest headder
#include "testcaseroot.h"
#include "testpolicy.h"
#include "testcasefactory.h"




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

		
void CBasicTestPolicy::ConstructL()
	{
	LOG_FUNC

	}
	

CBasicTestPolicy::~CBasicTestPolicy()
	{
	Cancel();
	if (iTestCase)
		{
		delete iTestCase;
		}
	}
	
		
/* Because the test policy can be referenced from the test case, parameters to pass to 
 the test-case may be stored in this class in future for access on ad-hoc basis as each test 
 case desires, without a need to modify all test cases
 */
void CBasicTestPolicy::RunTestCaseL(const TDesC& aTestCaseId, TRequestStatus* aNotifierStatus)
	{
	LOG_FUNC
	iNotifierStatus = aNotifierStatus;
	// delete previous test run
	if (iTestCase)
		{
		delete iTestCase;
		iTestCase = NULL;
		}

	// create the test class
	iTestCase = RTestFactory::CreateTestCaseL(aTestCaseId);
	// configure it
	iTestCase->SetTestPolicy(this);
	
	iTestCase->DisplayTestCaseOptions(); // test preconditions and detail

	// run the test-case
	iTestCase->PerformTestL();
	
	*iNotifierStatus = KRequestPending;
	}


void CBasicTestPolicy::DoCancel()
	{
	iTestCase->Cancel();
	
	User::RequestComplete(iNotifierStatus, KErrCancel);
	}


void CBasicTestPolicy::SignalTestComplete(TInt aCompletionCode)
	{
	if (KErrNone == aCompletionCode)
		{
		// Note there is no way to pass __FILE__ __LINE__ 'in' to this 'macro'
		test(ETrue);
		}
	else
		{
		test(EFalse); // failed	(PANIC here)
		}
		
	if (iNotifierStatus)	// will be null if we already signalled earlier
		{
		User::RequestComplete(iNotifierStatus, aCompletionCode);
		}
	}

	
void CBasicTestPolicy::RunL()
	{ 
	LOG_FUNC
	
	}


TInt CBasicTestPolicy::RunError(TInt /*aError*/)
	{
	return KErrNone;
	}


