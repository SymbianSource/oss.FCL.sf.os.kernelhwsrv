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
#include "testcasewd.h"
#include "testcasefactory.h"
#include "exampletestcase.h"



// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"EXAMPLETESTCASE");

// UNcomment the next line to add the test to the catalog(factory)
const TTestCaseFactoryReceipt<CExampleTestCase> CExampleTestCase::iFactoryReceipt(KTestCaseId);	

CExampleTestCase* CExampleTestCase::NewL(TBool aHost)
	{
	LOG_FUNC
	CExampleTestCase* self = new (ELeave) CExampleTestCase(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CExampleTestCase::CExampleTestCase(TBool aHost)
:	CTestCaseRoot(KTestCaseId, aHost)
	{
	LOG_FUNC
	} 


void CExampleTestCase::ConstructL()
	{
	LOG_FUNC
	BaseConstructL();
	iWDTimer = CTestCaseWatchdog::NewL();
	}


CExampleTestCase::~CExampleTestCase()
	{
	LOG_FUNC

	delete iWDTimer;
	Cancel();
	}


void CExampleTestCase::ExecuteTestCaseL()
	{
	LOG_FUNC
	iCaseStep = EFirstStep;

	//	
	CActiveScheduler::Add(this);
	// run our root RunL()
	SelfComplete();

	}

void CExampleTestCase::DescribePreconditions()
	{
	test.Printf(_L("This is an example test, there is nothing to do beforehand.\n"));
	}
	
void CExampleTestCase::DoCancel()
	{
	LOG_FUNC

	}


void CExampleTestCase::ProcessKey(TKeyCode &aKey)
	{	// the default implementation does this already, but we will store the key here again anyway
	iKeyCodeInput = aKey;

	}

void CExampleTestCase::FuncA(CTestCaseRoot *pThis)
	{
	CExampleTestCase * p = REINTERPRET_CAST(CExampleTestCase *,pThis);
	// cancel any pending call, and then complete our active obj with a timeout value
	test.Printf(_L("@@@ FuncA cancel a keyboard Read() @@@\n"));

	p->iConsole->ReadCancel();

	}

void CExampleTestCase::FuncB(CTestCaseRoot *pThis)
	{
	CExampleTestCase * p = REINTERPRET_CAST(CExampleTestCase *,pThis);
	// cancel any pending call, and then complete our active obj with a timeout value
	test.Printf(_L("@@@ FuncB cancel a 'B' keyboard Read() @@@\n"));

	p->Cancel();
	p->iConsole->ReadCancel();

	TRequestStatus* s = &p->iStatus;
	p->iStatus = KRequestPending;

	p->SetActive();
	User::RequestComplete(s, KTestCaseWatchdogTO);

	}


// handle event completion	
void CExampleTestCase::RunStepL()
	{
	LOG_FUNC

	// Obtain the completion code for this CActive obj.
	TInt completionCode(iStatus.Int()); 
	RDebug::Printf("Example test iStatus compl.=%d\n", completionCode);
	//test.Printf(_L("Example test iStatus compl.=%d\n"), completionCode);

	switch(iCaseStep)
		{
		case EFirstStep:
			iCaseStep=ESecondStep;
			test.Printf(_L("Test step 1\n"));

			SelfComplete();
			break;
		case ESecondStep:
			iCaseStep=EThirdStep;
			test.Printf(_L("Test step 2\n"));
			test.Printf(_L("(this test step uses Keyboard)\n"));
			test.Printf(_L("Press ANY key once you have removed the 'A' connector...\n"));
			RequestCharacter();
			iWDTimer->IssueRequest(KDelayDurationForUserActivityMS, this, &FuncA);
			break;
		case EThirdStep:
			test.Printf(_L("key was a '%c'\n"), iKeyCodeInput);
			iWDTimer->Cancel();
			iCaseStep=EFourthStep;
			test.Printf(_L("Test step 3\n"));
			test.Printf(_L("(this test step uses Keyboard)\n"));
			test.Printf(_L("Press <SPACE> key once you have removed the 'A' connector...\n"));
			RequestCharacter();
			iWDTimer->IssueRequest(KDelayDurationForUserActivityMS, this, &FuncB);
			
			break;
		case EFourthStep:
			test.Printf(_L("key was a '%c'\n"), iKeyCodeInput);
			test.Printf(_L("Test step 4\n"));
			iWDTimer->Cancel();

			iCaseStep=EFifthStep;
			if (' ' != iKeyCodeInput)
				{
				
				iCaseStep=EThirdStep;
				}
			SelfComplete();
			break;
		case EFifthStep:
			iCaseStep=ESixthStep;
			test.Printf(_L("Test step 5\n"));
			test.Printf(_L("(this test uses a delay)\n"));
			iTimer.After(iStatus, 500000);
			SetActive();
			break;
		case ESixthStep:
			iCaseStep=ELastStep;
			test.Printf(_L("Test step 6(%d)\n"), completionCode);
			RequestCharacter();
			iConsole->ReadCancel();
			Cancel();
			RequestCharacter();
			
			//SelfComplete();
			break;
		case ELastStep:
			iCaseStep=ESecondStep;
			test.Printf(_L("LAST step7 code (%d)\n"), completionCode);
			TestPolicy().SignalTestComplete(KErrNone);
			return TestPassed();
			//break;

		default:
			//test.Printf(_L("<Error> unknown test step"));
			Cancel();
			TestFailed(KErrCorrupt, _L("unknown test step"));
			break;
		}
	
	}


