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
#include <e32Test.h>	// RTest headder
#include "testcaseroot.h"
#include "testcase0464.h"



// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0464");
const TTestCaseFactoryReceipt<CTestCase0464> CTestCase0464::iFactoryReceipt(KTestCaseId);	

CTestCase0464* CTestCase0464::NewL(TBool aHost)
	{
	LOG_FUNC
	CTestCase0464* self = new (ELeave) CTestCase0464(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0464::CTestCase0464(TBool aHost)
:	CTestCaseRoot(KTestCaseId, aHost)
	{
	LOG_FUNC
		
	} 


/**
 ConstructL
*/
void CTestCase0464::ConstructL()
	{
	LOG_FUNC

	BaseConstructL();
	}


CTestCase0464::~CTestCase0464()
	{
	LOG_FUNC

	Cancel();
	}


void CTestCase0464::ExecuteTestCaseL()
	{
	LOG_FUNC
	iCaseStep = EPreconditions;
	
	CActiveScheduler::Add(this);
	SelfComplete();
	}


void CTestCase0464::DescribePreconditions()
	{
	test.Printf(_L("Insert 'A' connector beforehand.\n"));
	}

	
void CTestCase0464::DoCancel()
	{
	LOG_FUNC

	// cancel our timer
	iTimer.Cancel();
	}


void CTestCase0464::CancelKB(CTestCaseRoot *pThis)
	{
	CTestCase0464 * p = REINTERPRET_CAST(CTestCase0464 *,pThis);
	// cancel any pending call, and then complete our active obj with a timeout value

	p->iConsole->ReadCancel();
	
	}
	

// handle event completion	
void CTestCase0464::RunStepL()
	{
	LOG_FUNC
	// Obtain the completion code for this CActive obj.
	TInt completionCode(iStatus.Int()); 
	TBuf<MAX_DSTRLEN> aDescription;
	TInt err(0);
		
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
			// prompt to insert 'A' connector
			test.Printf(KInsertAConnectorPrompt);
			test.Printf(KPressAnyKeyToContinue);
			RequestCharacter();			
			break;

		case ELoadLdd:
			if (!StepLoadLDD())
				{
				break;
				}

			iCaseStep = EDriveBus;
			SelfComplete();
			break;

		case EDriveBus:
			LOG_VERBOSE1(_L("Turn ON VBus\n"))
			err = otgBusRequest();
			if (KErrNone != err)
				{
				return TestFailed(KErrAbort, _L("Raise VBus - RUsbOtgDriver::BusRequest() FAILED!"));
				}
			iCaseStep = EUnloadLdd;
			SelfComplete();
			break;

		case EUnloadLdd:
			LOG_VERBOSE1(_L("Unload.\n"))
			if (EFalse == StepUnloadLDD())
				return TestFailed(KErrAbort,_L("unload Ldd failure"));	
			test.Printf(_L("Measure VBus =0 and then press Y or N to continue."));
			RequestCharacter();
			iCaseStep = EVerifyVBusGone;
			break;

		case EVerifyVBusGone:
			LOG_VERBOSE1(_L("Test !VBus after unload.\n"))
			// test key response (or via an API)
			if (('y' == iKeyCodeInput) ||('Y' == iKeyCodeInput))
				{
				test.Printf(_L("VBUS drop 'seen' \n"));
				SelfComplete();
				}
			else
				{
				return TestFailed(KErrAbort, _L("VBus drop not 'seen' - FAILED!"));
				}
			iCaseStep = ELastStep;
			break;

		case ELastStep:
			return TestPassed();

		default:
			test.Printf(_L("<Error> unknown test step"));
			Cancel();
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));

		}
	}


