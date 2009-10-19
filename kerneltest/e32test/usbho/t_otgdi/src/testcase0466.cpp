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
#include "testcasewd.h"
#include "testcase0466.h"



// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0466");
const TTestCaseFactoryReceipt<CTestCase0466> CTestCase0466::iFactoryReceipt(KTestCaseId);	

CTestCase0466* CTestCase0466::NewL(TBool aHost)
	{
	LOG_FUNC
	CTestCase0466* self = new (ELeave) CTestCase0466(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0466::CTestCase0466(TBool aHost)
:	CTestCaseRoot(KTestCaseId, aHost)
	{
	LOG_FUNC
		
	} 


/**
 ConstructL
*/
void CTestCase0466::ConstructL()
	{
	LOG_FUNC
	iWDTimer = CTestCaseWatchdog::NewL();
	
	BaseConstructL();
	}


CTestCase0466::~CTestCase0466()
	{
	LOG_FUNC

	Cancel();
	delete iWDTimer;
	}


void CTestCase0466::ExecuteTestCaseL()
	{
	LOG_FUNC
	iCaseStep = EPreconditions;
	
	iRepeats = KOperationRetriesMax;
	
	CActiveScheduler::Add(this);
	SelfComplete();

	}


void CTestCase0466::DescribePreconditions()
	{
	test.Printf(_L("BEFORE running this test\n"));
	test.Printf(_L("\n"));
	test.Printf(_L("Insert the connector\n"));
	test.Printf(_L("from the OET with SW9\n"));
	test.Printf(_L("set to 'A-DEVICE' and\n"));
	test.Printf(_L("all other switches OFF\n"));
	test.Printf(_L("\n"));
	test.Printf(_L("Confirm passing tests\n"));
	test.Printf(_L("\n"));
	test.Printf(_L("ID_PIN detection\n"));
	test.Printf(_L("VBus Driving\n"));
	test.Printf(_L("\n"));
	}

	
void CTestCase0466::DoCancel()
	{
	LOG_FUNC

	// cancel our timer
	iTimer.Cancel();
	}
	
	// static cb helper
void CTestCase0466::CancelDrive(CTestCaseRoot *pThis)
	{
	CTestCase0466 * p = REINTERPRET_CAST(CTestCase0466 *,pThis);
	// cancel any pending call, and then complete our active obj with a timeout value
	p->SelfComplete(KTestCaseWatchdogTO);
	
	}


// handle event completion	
void CTestCase0466::RunStepL()
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
			// prompt to insert connector
			test.Printf(_L("\n"));
			test.Printf(KInsertAConnectorPrompt);
			test.Printf(_L("\n"));
			test.Printf(KPressAnyKeyToContinue);
			test.Printf(_L("\n"));
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
			
			// wait on 
		case EDriveBus:
			// wait for 100 ms to allow the stack time to settle
			User::After(100000);
			
			// drive VBus
			// NOTE: A-Plug must be inserted because it is illegal to power VBus unless 
			// A-Plug is detected (ID pin is low). The Thunk checks this.
			err = otgBusRequest();
			
			if (KErrNone != err)
				{
				return TestFailed(KErrAbort, _L("Raise VBus - RUsbOtgDriver::BusRequest() FAILED!"));
				}

			// wait for 1 second as described in the test document
			User::After(1000000);

			// subscribe to error events...
			otgQueueOtgMessageRequest(iOTGMessage, iStatus);
			SetActive();

			// ...and tell user to apply load	
			test.Printf(_L("\n"));
			test.Printf(_L("************************\n"));
			test.Printf(_L("* Using SW4 on the OET *\n"));
			test.Printf(_L("* Apply 100mA LOAD now *\n"));
			test.Printf(_L("************************\n"));
			test.Printf(_L("\n"));
			
			iCaseStep = EVerifyBusFail;
			
			break;
			
		case EVerifyBusFail:
		
			OtgMessageString(iOTGMessage, aDescription);
			test.Printf(_L("Received message %d '%S' status(%d)\n"), iOTGMessage, &aDescription, completionCode);

			if (RUsbOtgDriver::EMessageVbusError == iOTGMessage)
				{
				err = otgBusClearError();
				
				if ( err )
					{
					return TestFailed(KErrAbort, _L("VBUS Error Clear - FAILED!"));
					}

				iCaseStep = EUnloadLdd;
				SelfComplete();
				}
			else
				{
				iRepeats--;
				if (iRepeats <0)
					{
					return TestFailed(KErrAbort, _L("VBUS Fall FAILED!"));
					}

				otgQueueOtgMessageRequest(iOTGMessage, iStatus);
				SetActive();

				iCaseStep = EVerifyBusFail;
				}

			break;
			
		case EUnloadLdd:
			
			if (EFalse == StepUnloadLDD())
				{
				return TestFailed(KErrAbort,_L("unload Ldd failure"));
				}

			// remove 100ma Load - this reminds the user
			test.Printf(_L("\n"));
			test.Printf(_L("************************\n"));
			test.Printf(_L("* Using SW4 on the OET *\n"));
			test.Printf(_L("* Remove 100mA LOAD!   *\n"));
			test.Printf(_L("************************\n"));
			test.Printf(_L("\n"));
			
			iCaseStep = ELastStep;
			// press any key
			test.Printf(KPressAnyKeyToContinue);
			RequestCharacter();			
			break;
			
		case ELastStep:
			return TestPassed();
			
		default:
			test.Printf(_L("<Error> unknown test step"));
			Cancel();
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));

		}

	}


