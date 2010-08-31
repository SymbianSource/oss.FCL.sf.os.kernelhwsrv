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
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcase0466Traces.h"
#endif



// the name below is used to add a pointer to our construction method to a pointer MAP in 
// the class factory
_LIT(KTestCaseId,"PBASE-USB_OTGDI-0466");
const TTestCaseFactoryReceipt<CTestCase0466> CTestCase0466::iFactoryReceipt(KTestCaseId);	

CTestCase0466* CTestCase0466::NewL(TBool aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0466_NEWL);
	    }
	CTestCase0466* self = new (ELeave) CTestCase0466(aHost);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CTestCase0466::CTestCase0466(TBool aHost)
:	CTestCaseRoot(KTestCaseId, aHost)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0466_CTESTCASE0466);
	    }
		
	} 


/**
 ConstructL
*/
void CTestCase0466::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0466_CONSTRUCTL);
	    }
	iWDTimer = CTestCaseWatchdog::NewL();
	
	BaseConstructL();
	}


CTestCase0466::~CTestCase0466()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0466_DCTESTCASE0466);
	    }

	Cancel();
	delete iWDTimer;
	}


void CTestCase0466::ExecuteTestCaseL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0466_EXECUTETESTCASEL);
	    }
	iCaseStep = EPreconditions;
	
	iRepeats = KOperationRetriesMax;
	
	CActiveScheduler::Add(this);
	SelfComplete();

	}


void CTestCase0466::DescribePreconditions()
	{
	test.Printf(_L("BEFORE running this test\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0466_DESCRIBEPRECONDITIONS, "BEFORE running this test\n");
	test.Printf(_L("\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0466_DESCRIBEPRECONDITIONS_DUP01, "\n");
	test.Printf(_L("Insert the connector\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0466_DESCRIBEPRECONDITIONS_DUP02, "Insert the connector\n");
	test.Printf(_L("from the OET with SW9\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0466_DESCRIBEPRECONDITIONS_DUP03, "from the OET with SW9\n");
	test.Printf(_L("set to 'A-DEVICE' and\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0466_DESCRIBEPRECONDITIONS_DUP04, "set to 'A-DEVICE' and\n");
	test.Printf(_L("all other switches OFF\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0466_DESCRIBEPRECONDITIONS_DUP05, "all other switches OFF\n");
	test.Printf(_L("\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0466_DESCRIBEPRECONDITIONS_DUP06, "\n");
	test.Printf(_L("Confirm passing tests\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0466_DESCRIBEPRECONDITIONS_DUP07, "Confirm passing tests\n");
	test.Printf(_L("\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0466_DESCRIBEPRECONDITIONS_DUP08, "\n");
	test.Printf(_L("ID_PIN detection\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0466_DESCRIBEPRECONDITIONS_DUP09, "ID_PIN detection\n");
	test.Printf(_L("VBus Driving\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0466_DESCRIBEPRECONDITIONS_DUP10, "VBus Driving\n");
	test.Printf(_L("\n"));
	OstTrace0(TRACE_NORMAL, CTESTCASE0466_DESCRIBEPRECONDITIONS_DUP11, "\n");
	}

	
void CTestCase0466::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0466_DOCANCEL);
	    }

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
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASE0466_RUNSTEPL);
	    }
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
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP01, "\n");
			test.Printf(KInsertAConnectorPrompt);
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP02, KInsertAConnectorPrompt);
			test.Printf(_L("\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP03, "\n");
			test.Printf(KPressAnyKeyToContinue);
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP04, KPressAnyKeyToContinue);
			test.Printf(_L("\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP05, "\n");
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
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP06, "\n");
			test.Printf(_L("************************\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP07, "************************\n");
			test.Printf(_L("* Using SW4 on the OET *\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP08, "* Using SW4 on the OET *\n");
			test.Printf(_L("* Apply 100mA LOAD now *\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP09, "* Apply 100mA LOAD now *\n");
			test.Printf(_L("************************\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP10, "************************\n");
			test.Printf(_L("\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP11, "\n");
			
			iCaseStep = EVerifyBusFail;
			
			break;
			
		case EVerifyBusFail:
		
			OtgMessageString(iOTGMessage, aDescription);
			test.Printf(_L("Received message %d '%S' status(%d)\n"), iOTGMessage, &aDescription, completionCode);
			OstTraceExt3(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP12, "Received message %d '%S' status(%d)\n", iOTGMessage, aDescription, completionCode);

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
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP13, "\n");
			test.Printf(_L("************************\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP14, "************************\n");
			test.Printf(_L("* Using SW4 on the OET *\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP15, "* Using SW4 on the OET *\n");
			test.Printf(_L("* Remove 100mA LOAD!   *\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP16, "* Remove 100mA LOAD!   *\n");
			test.Printf(_L("************************\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP17, "************************\n");
			test.Printf(_L("\n"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP18, "\n");
			
			iCaseStep = ELastStep;
			// press any key
			test.Printf(KPressAnyKeyToContinue);
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP19, KPressAnyKeyToContinue);
			RequestCharacter();			
			break;
			
		case ELastStep:
			return TestPassed();
			
		default:
			test.Printf(_L("<Error> unknown test step"));
			OstTrace0(TRACE_NORMAL, CTESTCASE0466_RUNSTEPL_DUP20, "<Error> unknown test step");
			Cancel();
			return (TestFailed(KErrCorrupt, _L("<Error> unknown test step")));

		}

	}


