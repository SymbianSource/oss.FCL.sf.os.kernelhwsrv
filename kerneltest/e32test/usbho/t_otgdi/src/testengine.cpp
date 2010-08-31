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
#include <e32cons.h>
#include <e32Test.h>	// RTest headder
#include <e32def.h>
#include <e32def_private.h>
#include "debugmacros.h"
#include "TestEngine.h"
#include "TestCaseController.h"
#include "TestCaseFactory.h"
#include "TestCaseRoot.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testengineTraces.h"
#endif

// Console application parameter options
_LIT(KArgAllTestCases,"/ALL");			// see default test-list below
_LIT(KArgGoTestCase, "/G:");
_LIT(KArgAutomatedTest, "/AUTO");		// removes "press any key to continue" prompts
_LIT(KArgVerboseOutput, "/VERBOSE");	// also turns on RDebug logging of all test output (to serial)
_LIT(KArgSetOpenIterations, "/LOOPO:");	// Open/Close test loop count
_LIT(KArgSetOOMIterations, "/LOOPM:");	// OOM test set #allocs
_LIT(KArgSetRoleMaster, "/MASTER");		// this is the default
_LIT(KArgSetRoleSlave, "/SLAVE");		// slave - Runs a dual-role test's Slave steps instead of Master 
_LIT(KArgOverrideVidPid, "/PID:");		// vendor, product ID XXXX 4 hex digits /PID:0670

_LIT(KidFormatter,"PBASE-USB_OTGDI-%04d");
_LIT(KidFormatterS,"PBASE-USB_OTGDI-%S");

// '/ALL' tests grouping
const TInt KAllDefaultTestIDs[6] = 
	{
	456, // (a) PBASE-USB_OTG-0456 open/close 'A'
	457, // (a) PBASE-USB_OTG-0457 open/close disconnected
	459, // (m) PBASE-USB_OTG-0459 detect 'A'
	460, // (m) PBASE-USB_OTG-0460 detect 'A' removal
	464, // (m) PBASE-USB_OTG-0464 raise
	465  // (m) PBASE-USB_OTG-0465 lower
	};


	
//	
// CTestEngine implementation
//
CTestEngine* CTestEngine::NewL()
	{
	CTestEngine* self = new (ELeave) CTestEngine;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}


CTestEngine::CTestEngine():
	iTestCaseIndex(0),
	iHelpRequested(EFalse)
	{

	}


CTestEngine::~CTestEngine()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTENGINE_DCTESTENGINE);
	    }
	// Destroy the test case controller
	if (iTestCaseController)
		{
		delete iTestCaseController;
		}
	// Destroy the test identity array and its contents
	iTestCasesIdentities.ResetAndDestroy();
	
	}
	
	
void CTestEngine::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTENGINE_CONSTRUCTL);
	    }
	TInt menuSelection(0);

	// Display information 
	test.Title();
	test.Start(_L("Test Engine Initiation v2.00 "));
	test.Printf(_L(">>\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP01, ">>\n");
	test.Printf(_L(">>   T E S T   R U N \n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP02, ">>   T E S T   R U N \n");
	test.Printf(_L(">>\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP03, ">>\n");
	

	// Process the command line parameters for batch/etc
	TRAPD(err, ProcessCommandLineL());
	if (err != KErrNone)
		{
		User::Panic(_L("Test F/W Err"), KErrNoMemory);
		}
	
	if (iHelpRequested)
		{
		PrintUsage();
		User::Leave(-2);	// nothing to do!
		}

	// if no command-line, we use a menu UI
	if (!iTestCasesIdentities.Count())
		{
		RPointerArray<HBufC> testCaseNames;
		// no tests added, select ONE to run from the menu

		// list test cases (PRINT MENU) - in numeric order
		RTestFactory::ListRegisteredTestCases(testCaseNames);

		iTestCaseIndex = 0;	// be sure we go back to beginning of the collection!
		iTestCasesIdentities.ResetAndDestroy();
		
		test.Printf(_L("Please select 0 to %d\n"), RTestFactory::TestCaseCount()-1);
		OstTrace1(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP04, "Please select 0 to %d\n", RTestFactory::TestCaseCount()-1);
		test.Printf(_L("or 99<ENTER> to exit\n"));
		OstTrace0(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP05, "or 99<ENTER> to exit\n");
		GetNumericInput(menuSelection);
		if ((menuSelection >=0) &&(menuSelection < RTestFactory::TestCaseCount()))
			{
			// add it to the list,and we can go
			TBuf<KTestCaseIdLength> aSelectionID;
			HBufC* tc = HBufC::NewLC(KTestCaseIdLength);
			
			// get name from index
			*tc = aSelectionID;
			*tc = *testCaseNames[menuSelection];

			iTestCasesIdentities.Append(tc);
			CleanupStack::Pop(tc);
			}
		testCaseNames.ResetAndDestroy();
		}
	
	if ((menuSelection < RTestFactory::TestCaseCount()) && (menuSelection>=0))
		{
		// Create the test case controller
		test.Printf(_L("Creating the test controller\n"));
		OstTrace0(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP06, "Creating the test controller\n");
		iTestCaseController = CTestCaseController::NewL(*this, ETrue);
		
		// Test-engine is non CActive class
		}
	else
		{
		// nothing to do, exit. USER aborted
		test.Printf(_L("Test run stopped by user, nothing to do.\n"));
		OstTrace0(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP07, "Test run stopped by user, nothing to do.\n");
		User::Leave(-2);
		}
	}
	

/* Displayed if used supplied no parameters, garbage, or a ? in the parameters
 */
void CTestEngine::PrintUsage()
	{
	test.Printf(_L("OTGDI Unit Test Suite.\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_PRINTUSAGE, "OTGDI Unit Test Suite.\n");
	test.Printf(_L("Usage : t_otgdi.exe [/option] /G:<TESTNUM1>\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_PRINTUSAGE_DUP01, "Usage : t_otgdi.exe [/option] /G:<TESTNUM1>\n");
	test.Printf(_L("  /ALL = add default test subset to List\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_PRINTUSAGE_DUP02, "  /ALL = add default test subset to List\n");
	test.Printf(_L("  /G:<TESTNUM>  where <testname> is the test# to add \n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_PRINTUSAGE_DUP03, "  /G:<TESTNUM>  where <testname> is the test# to add \n");
	test.Printf(_L("  /AUTO  = largely unattended operation\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_PRINTUSAGE_DUP04, "  /AUTO  = largely unattended operation\n");
	test.Printf(_L("  /VERBOSE = test debugging info\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_PRINTUSAGE_DUP05, "  /VERBOSE = test debugging info\n");
	test.Printf(_L("  /LOOPO:<n> = Open/close repeat counter<n>\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_PRINTUSAGE_DUP06, "  /LOOPO:<n> = Open/close repeat counter<n>\n");
	test.Printf(_L("  /LOOPM:<n> = OOM HEAP_ALLOCS counter<n>\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_PRINTUSAGE_DUP07, "  /LOOPM:<n> = OOM HEAP_ALLOCS counter<n>\n");
	test.Printf(_L("  /SLAVE = Test-peer server mode\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_PRINTUSAGE_DUP08, "  /SLAVE = Test-peer server mode\n");
	test.Printf(_L("  /PID:<n> = USB VID/PID in hex eg 2670\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_PRINTUSAGE_DUP09, "  /PID:<n> = USB VID/PID in hex eg 2670\n");
	test.Printf(_L("Valid test ID range 0456...0469\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_PRINTUSAGE_DUP10, "Valid test ID range 0456...0469\n");
	test.Printf(_L("and 0675...0684 .\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_PRINTUSAGE_DUP11, "and 0675...0684 .\n");
	test.Printf(_L("\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_PRINTUSAGE_DUP12, "\n");
	}
	
/** process the command-line, ; arguments appear in any order
 IN   : User::CommandLine()
 OUT  : iTestCasesIdentities
        iHelpRequested
 		gSemiAutomated
 		gVerboseOutput
 		gOpenIterations
 		gOOMIterations
 		gTestRoleMaster
 		gUSBVidPid
*/
void CTestEngine::ProcessCommandLineL()
	{
	// example t_otgdi.exe /ALL /G:0468 /VERBOSE
	TInt cmdLineLength(User::CommandLineLength());
	HBufC* cmdLine = HBufC::NewMaxLC(cmdLineLength);
	TPtr cmdLinePtr = cmdLine->Des();
	User::CommandLine(cmdLinePtr);
	TBool  tokenParsed(EFalse);

	TLex args(*cmdLine);
	args.SkipSpace(); // args are separated by spaces
	
	// first arg is the exe name, skip it
	TPtrC cmdToken = args.NextToken();
	HBufC* tc = HBufC::NewLC(KParameterTextLenMax);
	*tc = cmdToken;
	while (tc->Length())
		{
		tokenParsed = EFalse;
		
		// '/?' help wanted flag '?' or /? parameter
		TInt pos(0);
		if ((0== tc->FindF(_L("?"))) || (0==tc->FindF(_L("/?")))) 
			{
			iHelpRequested = ETrue;
			tokenParsed = ETrue;
			}	
		
		// '/ALL' parameter
		pos = tc->FindF(KArgAllTestCases);
		if (pos != KErrNotFound)
			{
			AddAllDefaultTests();
			tokenParsed = ETrue;
			}

		// '/AUTO'	
		pos = tc->FindF(KArgAutomatedTest);
		if (pos != KErrNotFound)
			{
			// skip some of the press-any key things
			test.Printf(_L("Test semi-automated mode.\n"));
			OstTrace0(TRACE_NORMAL, CTESTENGINE_PROCESSCOMMANDLINEL, "Test semi-automated mode.\n");
			gSemiAutomated = ETrue;
			tokenParsed = ETrue;
			}

		// '/G:TESTNAME'
		pos = tc->FindF(KArgGoTestCase);
		if (pos != KErrNotFound)
			{ 
			HBufC* tcPart = HBufC::NewLC(KTestCaseIdLength);
			TPtrC testID = tc->Right(tc->Length() - pos - KArgGoTestCase().Length());

			LOG_VERBOSE2(_L("Parameter found:'%S'\n"), &testID);
			if(gVerboseOutput)
			    {
			    OstTraceExt1(TRACE_VERBOSE, CTESTENGINE_PROCESSCOMMANDLINEL_DUP01, "Parameter found:'%S'\n", testID);
			    }

			// Check if it is a test we know of in our suite, users may provide the full  
			// name "PBASE-USB_OTGDI-0466", or just the last 4 digits "0466", in such cases, fetch the full name
			if (!RTestFactory::TestCaseExists(testID))
				{ // try use just the test#part
				TPtr  tcDes = tcPart->Des();

				// build and add the full name
				tcDes.Format(KidFormatterS, &testID);
				if (!RTestFactory::TestCaseExists(tcDes))
					{
					
					test.Printf(_L("Test case does NOT Exist: '%lS'\n"), &testID);
					OstTraceExt1(TRACE_NORMAL, CTESTENGINE_PROCESSCOMMANDLINEL_DUP02, "Test case does NOT Exist: '%lS'\n", testID);
					}
				else
					{ // only the number was supplied, copy the full name
					testID.Set(tcDes);
					}
				}
			// check that it's valid before adding it to the run-list
			if (RTestFactory::TestCaseExists(testID))
				{
				HBufC* testIdentity = HBufC::NewLC(KTestCaseIdLength);
				*testIdentity = testID;
				test.Printf(_L("Test case specified: %lS\n"), testIdentity);
				OstTraceExt1(TRACE_NORMAL, CTESTENGINE_PROCESSCOMMANDLINEL_DUP03, "Test case specified: %lS\n", *testIdentity);

				iTestCasesIdentities.Append(testIdentity);
				CleanupStack::Pop(testIdentity);
				}
			CleanupStack::PopAndDestroy(tcPart);
			tokenParsed = ETrue;
			}
			
		// '/VERBOSE' option	
		pos = tc->FindF(KArgVerboseOutput);
		if (pos != KErrNotFound)
			{ 
			gVerboseOutput = ETrue;
			tokenParsed = ETrue;
			
			// turn on logging of test Printf() output to serial debug/log at the same time
			test.SetLogged(ETrue);
			
			}

		// '/LOOPO:n' option (Set #times to run open/close tests amongst others)
		pos = tc->FindF(KArgSetOpenIterations);
		if (pos != KErrNotFound)
			{ 
			TPtrC iterationStr = tc->Right(tc->Length() - pos - KArgSetOpenIterations().Length());
			TLex  lex(iterationStr);
			lex.Val(gOpenIterations);
			MINMAX_CLAMPVALUE(gOpenIterations, OPEN_MINREPEATS, OPEN_MAXREPEATS);
			tokenParsed = ETrue;
			}

		// '/LOOPM:n' option (Set # of allocs to start at for OOM test)
		pos = tc->FindF(KArgSetOOMIterations);
		if (pos != KErrNotFound)
			{ 
			TPtrC iterationStr = tc->Right(tc->Length() - pos - KArgSetOOMIterations().Length());
			TLex  lex(iterationStr);
			lex.Val(gOOMIterations);
			MINMAX_CLAMPVALUE(gOOMIterations, OOM_MINREPEATS, OOM_MAXREPEATS);
			tokenParsed = ETrue;
			}
		
		
		// '/VID:nnnn' option (Set Symbian or other VID-Pid example /VID:0670)
		pos = tc->FindF(KArgOverrideVidPid);
		if (pos != KErrNotFound)
			{ 
			TPtrC vidpidStr = tc->Right(tc->Length() - pos - KArgOverrideVidPid().Length());
			TUint16 prodID;
			TLex  lex(vidpidStr);
			
			if (KErrNone == lex.Val(prodID, EHex))
				{
				if (prodID> 0xFFFF)
					prodID = 0xFFFF;
				tokenParsed = ETrue;
				LOG_VERBOSE2(_L(" accept param %04X \n\n"), prodID);
				if(gVerboseOutput)
				    {
				    OstTrace1(TRACE_VERBOSE, CTESTENGINE_PROCESSCOMMANDLINEL_DUP05, " accept param %04X \n\n", prodID);
				    }
				gUSBVidPid = prodID; // replace the vid-pid with the user-supplied one 
				}
			else
				{
				// print error
				test.Printf(_L("Warning: VID+PID '%lS' not parsed .\n"), tc);
				OstTraceExt1(TRACE_NORMAL, CTESTENGINE_PROCESSCOMMANDLINEL_DUP06, "Warning: VID+PID '%lS' not parsed .\n", *tc);
				}
			}
		
		// '/SLAVE' (peer)
		pos = tc->FindF(KArgSetRoleSlave);
		if (pos != KErrNotFound)
			{ 
			gTestRoleMaster = EFalse;
			tokenParsed = ETrue;
			}
		// '/MASTER' - default role
		pos = tc->FindF(KArgSetRoleMaster); // note that master is the default role, so this parameter is optional
		if (pos != KErrNotFound)
			{ 
			gTestRoleMaster = ETrue;
			tokenParsed = ETrue;
			}		
		
		if (!tokenParsed)
			{
			// warn about unparsed parameter
			test.Printf(_L("Warning: '%lS'??? not parsed\n"), tc);
			OstTraceExt1(TRACE_NORMAL, CTESTENGINE_PROCESSCOMMANDLINEL_DUP07, "Warning: '%lS'??? not parsed\n", *tc);
			iHelpRequested = ETrue;
			}
			
		// next parameter
		*tc = args.NextToken();
		}
	CleanupStack::PopAndDestroy(tc);
	CleanupStack::PopAndDestroy(cmdLine);
	}


/** Add all default tests to the front of the test-list so we run them all in sequence
*/	
void CTestEngine::AddAllDefaultTests()
	{
	test.Printf(_L("Adding default set test cases\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_ADDALLDEFAULTTESTS, "Adding default set test cases\n");
	//
	TInt index(0);
	while (index < sizeof(KAllDefaultTestIDs)/sizeof(KAllDefaultTestIDs[0]))
		{
		// allocate heap string
		HBufC* tc(NULL);
		TRAPD(err, tc = HBufC::NewL(KTestCaseIdLength))
		if (err != KErrNone)
			{
			User::Panic(_L("Test F/W Err"), KErrNoMemory);
			}
		TPtr  tcDes = tc->Des();

		// build and add it
		tcDes.Format(KidFormatter, KAllDefaultTestIDs[index]);
		iTestCasesIdentities.Append(tc);
		index++;
		}
	}


/* Return subsequent test case IDs from the test run-list KerrNotFound = end of list.
 */ 	
TInt CTestEngine::NextTestCaseId(TDes& aTestCaseId)
	{
	if (iTestCaseIndex < iTestCasesIdentities.Count())
		{
		aTestCaseId = *iTestCasesIdentities[iTestCaseIndex++];
		return KErrNone;
		}
	else
		{
		return KErrNotFound;
		}
	}

/////////////////////////////////////////////////////////////////////////////
// utility functions


void CTestEngine::GetNumericInput(TInt &aNumber)
	{
	TUint value(0);
	TUint digits(0);
	TKeyCode key = (TKeyCode) 0;

	aNumber = -1;
	while ( key != EKeyEnter )
		{
		key = test.Getch();

		if ( ( key >= '0' ) && ( key <= '9' ) )
			{
			test.Printf(_L("%c"),key);
			OstTraceExt1(TRACE_NORMAL, CTESTENGINE_GETNUMERICINPUT, "%c",key);
			
			value = ( 10 * value ) + ( key - '0' );
			digits++;
			} else 
			{ // very basic keyboard processing, backspace
				if (key == EKeyBackspace)
				{
				value = value/10;
				digits--;
				test.Printf(_L("\r    \r%d"), value);
				OstTrace1(TRACE_NORMAL, CTESTENGINE_GETNUMERICINPUT_DUP01, "\r    \r%d", value);
				}
			}
		}

	if (digits > 0)
		{
		aNumber = value;
		}
	test.Printf(_L("\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_GETNUMERICINPUT_DUP02, "\n");
	}


/** Print a report at the end of a test run of all PASSED tests, Note: If a 
 test fails, the framework gets Panic'd */
void CTestEngine::Report()
	{
	TBuf<KTestCaseIdLength> aTestCaseId;
	test.Printf(_L("============================\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_REPORT, "============================\n");
	test.Printf(_L("PASSED TESTS:\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_REPORT_DUP01, "PASSED TESTS:\n");
	// itterate our list of tests to perform
	ResetTestCaseIndex();
	while (KErrNone == NextTestCaseId(aTestCaseId))
		{
		test.Printf(_L("%S\n"), &aTestCaseId);
		OstTraceExt1(TRACE_NORMAL, CTESTENGINE_REPORT_DUP02, "%S\n", aTestCaseId);
		}
	}
	
	
void CTestEngine::DoCancel()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTENGINE_DOCANCEL);
	    }
	test.Console()->ReadCancel();	
	}
		
	
TInt CTestEngine::RunError(TInt aError)
	{
	return aError;
	}

