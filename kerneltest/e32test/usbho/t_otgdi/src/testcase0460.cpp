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
// 'A' connector removal test
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <u32std.h> 	// unicode builds
#include <e32base.h>
#include <e32base_private.h>
#include <e32Test.h>	// RTest headder
#include "testcaseroot.h"
#include "testcasefactory.h"
#include "testcase0460.h"




// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0460");
const TTestCaseFactoryReceipt<CTestCase0460> CTestCase0460::iFactoryReceipt(KTestCaseId);	

CTestCase0460* CTestCase0460::NewL(TBool aHost)
	{
	LOG_FUNC
	CTestCase0460* self = new (ELeave) CTestCase0460(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0460::CTestCase0460(TBool aHost)
:	CTestCaseRoot(KTestCaseId, aHost)
	{
	LOG_FUNC
		
	} 


/**
 ConstructL
*/
void CTestCase0460::ConstructL()
	{
	LOG_FUNC
	
	BaseConstructL();
	}


CTestCase0460::~CTestCase0460()
	{
	LOG_FUNC

	Cancel();
	}


void CTestCase0460::ExecuteTestCaseL()
	{
	LOG_FUNC
	iCaseStep = EPreconditions;
	
	CActiveScheduler::Add(this);
	SelfComplete();

	}


void CTestCase0460::DescribePreconditions()
	{
	test.Printf(_L("Insert 'A' connector beforehand.\n"));
	}

	
void CTestCase0460::DoCancel()
	{
	LOG_FUNC

	// cancel our timer
	iTimer.Cancel();
	}
	

// handle event completion	
void CTestCase0460::RunStepL()
	{
	LOG_FUNC
	// Obtain the completion code for this CActive obj.
	TInt completionCode(iStatus.Int()); 
	TBuf<MAX_DSTRLEN> aDescription;
		
	switch(iCaseStep)
		{
		case EPreconditions:
			iCaseStep = ELoadLdd;
			if (iAutomated)
				{
				iCaseStep = ELoadLdd;
				SelfComplete();
				break;
				}
			// prompt to remove connector
			test.Printf(KRemoveAConnectorPrompt);
			test.Printf(KPressAnyKeyToContinue);
			RequestCharacter();			
			break;
			
		case ELoadLdd:
			if (!StepLoadLDD())
				{
				break;
				}

			iCaseStep = ERegisterForEvents;
			iDequeAttempts = 0;	

			// prompt to insert connector
			test.Printf(KInsertAConnectorPrompt);
			test.Printf(KPressAnyKeyToContinue);
			RequestCharacter();			
			break;
			// wait on ID_PIN
		case ERegisterForEvents:
			if (iDequeAttempts > KOperationRetriesMax)
				{
				return (TestFailed(KErrCorrupt, _L("<Error> too many irrelevant/incorrect events")));
				}
			iCaseStep = ETestStateA;

			test.Printf(_L("Waiting for OTG Event\n"));
			otgQueueOtgEventRequest( iOTGEvent, iStatus);

			SetActive();
			break;
			
		case EDriveID_PIN:
			// API call for simulating ID_PIN is not implemented, so the user plugs in the cable, 
			// then test for A plug
			SetActive();
			break;
			
		case EWait5:
		case ETestStateA:
			OtgEventString(iOTGEvent, aDescription);
			test.Printf(_L("Received event %d '%S' status(%d)"), iOTGEvent, &aDescription, completionCode);
			if (RUsbOtgDriver::EEventAPlugInserted == iOTGEvent)
				{
				iCaseStep = EUnloadLdd;
				}
			else
				{
				// wrong event in the Q already, keep at it
				iCaseStep = ERegisterForEvents;	
				iDequeAttempts++;
				}
	
			SelfComplete();
			break;		
			
		case EUnloadLdd:
		case ELastStep:
			if (EFalse == StepUnloadLDD())
				return TestFailed(KErrAbort,_L("unload Ldd failure"));	
			
			return TestPassed();

		default:
			test.Printf(_L("<Error> unknown test step"));
			Cancel();
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));

		}
	}

