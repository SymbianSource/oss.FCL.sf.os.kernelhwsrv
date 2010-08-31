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
// @file testengine.cpp
// @internalComponent
// 
//

#include "TestEngine.h"
#include "testdebug.h"
#include "TestCaseController.h"
#include "TestCaseFactory.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "TestEngineTraces.h"
#endif

// Console application options

_LIT(KArgRole,"-role=");
_LIT(KArgTestCases,"-cases=");
_LIT(KArgTestRepeats,"-repeats=");

// Role values 

_LIT(KArgRoleHost,"host");
_LIT(KArgRoleClient,"client");


extern RTest gtest;

namespace NUnitTesting_USBDI
	{
const TUint KDefaultNumRepeats = 1;
const TUint KTestIdSize = 4;
_LIT(KTestStringPreamble,"PBASE-T_USBDI-");
	
CTestEngine* CTestEngine::NewL()
	{
	OstTraceFunctionEntry0( CTESTENGINE_NEWL_ENTRY );
	CTestEngine* self = new (ELeave) CTestEngine;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CTESTENGINE_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}



CTestEngine::CTestEngine()
:	CActive(EPriorityUserInput),
	iTestCaseIndex(0), iRepeat(0), iNumRepeats(KDefaultNumRepeats)
	{
	OstTraceFunctionEntry1( CTESTENGINE_CTESTENGINE_ENTRY, this );
	OstTraceFunctionExit1( CTESTENGINE_CTESTENGINE_EXIT, this );
	}



CTestEngine::~CTestEngine()
	{
	OstTraceFunctionEntry1( CTESTENGINE_CTESTENGINE_ENTRY_DUP01, this );
	// Cancel reading user console input
	Cancel();
	
	// Destroy the test case controller
	delete iTestCaseController;
	
	// Destroy the identity array and its contents
	iTestCasesIdentities.ResetAndDestroy();
	
	// Finish test and release resources
	gtest.End();
	gtest.Close();
	OstTraceFunctionExit1( CTESTENGINE_CTESTENGINE_EXIT_DUP01, this );
	}
	
	
	
void CTestEngine::ConstructL()
	{
	OstTraceFunctionEntry1( CTESTENGINE_CONSTRUCTL_ENTRY, this );
	CActiveScheduler::Add(this);

	// Display information (construction text and OS build version number
	gtest.Title();
	gtest.Start(_L("Test Engine Initiation"));
	gtest.Printf(_L(">>\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL, ">>\n");
	gtest.Printf(_L(">>   T E S T   R U N \n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP01, ">>   T E S T   R U N \n");
	gtest.Printf(_L(">>\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP02, ">>\n");

	// Process the command line option for role
	TInt cmdLineLength(User::CommandLineLength());
	HBufC* cmdLine = HBufC::NewMax(cmdLineLength);
	CleanupStack::PushL(cmdLine);
	TPtr cmdLinePtr = cmdLine->Des();
	User::CommandLine(cmdLinePtr);
	
	// be careful, command line length is limited(248 characters)	
	gtest.Printf(_L("***cmdLine = %lS\n"), cmdLine);
	OstTraceExt1(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP03, "***cmdLine = %lS\n", *cmdLine);
		
	TLex args(*cmdLine);
	args.SkipSpace();
	
	// Obtain the role of this test module
	TPtrC roleToken = args.NextToken(); // e.g. -role=host
	TBool hostFlag(ETrue);
	
	TInt pos(roleToken.FindF(KArgRole));
	if(pos != KErrNotFound)
		{
		pos = roleToken.FindF(_L("="));
		TPtrC role = roleToken.Right(roleToken.Length()-pos-1);
		if(role.Compare(KArgRoleHost) == 0)
			{
			hostFlag = ETrue;
			}
		else if(role.Compare(KArgRoleClient) == 0)
			{
			hostFlag = EFalse;
			}
		else
			{
			gtest.Printf(_L("Test configuration: could not find option -role\n"));
			OstTrace0(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP04, "Test configuration: could not find option -role\n");
			gtest(EFalse);
			}
		}
	else
		{
		gtest.Printf(_L("Test configuration option not found: %S\n"),&KArgRole);
		OstTraceExt1(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP05, "Test configuration option not found: %S\n",KArgRole);
		gtest(EFalse);
		}
		
	// Obtain the test cases to be run
	TPtrC casesToken = args.NextToken();
	
	pos = casesToken.FindF(KArgTestCases);
	if(pos != KErrNotFound)
		{
		pos = casesToken.FindF(_L("="));
		TPtrC testCases = casesToken.Right(casesToken.Length()-pos-1);
	
		// Remaining test cases
		TPtrC remCases(testCases);
		while(pos != KErrNotFound)
			{
			pos = remCases.FindF(_L(","));
			HBufC* tc = HBufC::NewLC(KTestCaseIdLength);
			TPtr tcPtr = tc->Des();
			tcPtr.Append(KTestStringPreamble);	
			
			if(pos == KErrNotFound)
				{
				// This is the last test case identity			
				tcPtr.Append(remCases);
				}
			else
				{ 			
				tcPtr.Append(remCases.Left(KTestIdSize));
				}									
							
			gtest.Printf(_L("Test case specified: %S\n"),tc);
			OstTraceExt1(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP06, "Test case specified: %S\n",*tc);
			
						
			iTestCasesIdentities.Append(tc);
			CleanupStack::Pop(tc);
			remCases.Set(testCases.Right(remCases.Length()-pos-1));
			}
		}
	else
		{
		gtest.Printf(_L("Test configuration option not found: %S\n"),&KArgTestCases);
		OstTraceExt1(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP07, "Test configuration option not found: %S\n",KArgTestCases());
		gtest(EFalse);		
		}
				
	// Obtain the role of this test module
	TPtrC repeatsToken = args.NextToken(); // e.g. -repeats=4
	
	pos = repeatsToken.FindF(KArgTestRepeats);
	if(pos != KErrNotFound)
		{
		pos = repeatsToken.FindF(_L("="));
		TPtrC repeats = repeatsToken.Right(repeatsToken.Length()-pos-1);
		TLex lex(repeats);
		TInt ret = lex.Val(iNumRepeats, EDecimal);
		if(ret)
			{
			gtest.Printf(_L("Test configuration option not found: %S\n"),&KArgTestRepeats);
			OstTraceExt1(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP08, "Test configuration option not found: %S\n",KArgTestRepeats());
			gtest.Printf(_L("DEFAULT to number of repeats = %d\n"),KDefaultNumRepeats);
			OstTrace1(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP09, "DEFAULT to number of repeats = %d\n",KDefaultNumRepeats);
			iNumRepeats = KDefaultNumRepeats;
			}
		gtest.Printf(_L("Test repeats specified: %d cycles\n"),iNumRepeats);
		OstTrace1(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP10, "Test repeats specified: %d cycles\n",iNumRepeats);
		}
	else
		{
		gtest.Printf(_L("Test configuration option not found: %S\n"),&KArgTestRepeats);
		OstTraceExt1(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP11, "Test configuration option not found: %S\n",KArgTestRepeats());
		gtest.Printf(_L("DEFAULT to number of repeats = %d\n"),KDefaultNumRepeats);
		OstTrace1(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP12, "DEFAULT to number of repeats = %d\n",KDefaultNumRepeats);
		iNumRepeats = KDefaultNumRepeats;
		}
		
	// Create the test case controller
	gtest.Printf(_L("Creating the test controller\n"));
	OstTrace0(TRACE_NORMAL, CTESTENGINE_CONSTRUCTL_DUP13, "Creating the test controller\n");
	iTestCaseController = CTestCaseController::NewL(*this,hostFlag);

	CleanupStack::PopAndDestroy(cmdLine);

	gtest.Console()->Read(iStatus);
	SetActive();	
	OstTraceFunctionExit1( CTESTENGINE_CONSTRUCTL_EXIT, this );
	}
	

TInt CTestEngine::NextTestCaseId(TDes& aTestCaseId)
	{
	OstTraceFunctionEntryExt( CTESTENGINE_NEXTTESTCASEID_ENTRY, this );
	if(iTestCaseIndex < iTestCasesIdentities.Count())
		{
		aTestCaseId = *iTestCasesIdentities[iTestCaseIndex++];
		if(iTestCaseIndex==iTestCasesIdentities.Count())
			{
			iRepeat++;
			if(iRepeat < iNumRepeats)
				{
				iTestCaseIndex = 0; //prepare to start again
				}
			}
		OstTraceFunctionExitExt( CTESTENGINE_NEXTTESTCASEID_EXIT, this, KErrNone );
		return KErrNone;
		}
	else
		{
		OstTraceFunctionExitExt( CTESTENGINE_NEXTTESTCASEID_EXIT_DUP01, this, KErrNotFound );
		return KErrNotFound;
		}
	}

RPointerArray<HBufC>& CTestEngine::TestCasesIdentities()
	{
	OstTraceFunctionEntry1( CTESTENGINE_TESTCASESIDENTITIES_ENTRY, this );
	OstTraceFunctionExitExt( CTESTENGINE_TESTCASESIDENTITIES_EXIT, this, ( TUint )&( iTestCasesIdentities ) );
	return iTestCasesIdentities;
	}

TUint CTestEngine::NumRepeats()
	{
	OstTraceFunctionEntry1( CTESTENGINE_NUMREPEATS_ENTRY, this );
	OstTraceFunctionExitExt( CTESTENGINE_NUMREPEATS_EXIT, this, iNumRepeats );
	return iNumRepeats;
	}

void CTestEngine::DoCancel()
	{
	OstTraceFunctionEntry1( CTESTENGINE_DOCANCEL_ENTRY, this );
	gtest.Console()->ReadCancel();	
	OstTraceFunctionExit1( CTESTENGINE_DOCANCEL_EXIT, this );
	}
	

void CTestEngine::RunL()
	{
	OstTraceFunctionEntry1( CTESTENGINE_RUNL_ENTRY, this );
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{
		// Possibility of displaying a range of key commands
		// then gtest.Console()->Getch()
		
		TKeyCode keyCode(gtest.Console()->KeyCode());
		if(keyCode == EKeySpace)
			{
			iTestCaseController->Cancel();
			gtest.Printf(_L("Test module terminating\n"));
			OstTrace0(TRACE_NORMAL, CTESTENGINE_RUNL, "Test module terminating\n");
			OstTrace0(TRACE_NORMAL, CTESTENGINE_RUNL_DUP01, "CActiveScheduler::Stop CTestEngine::RunL");
			CActiveScheduler::Stop();
			}
		else
			{
			gtest.Printf(_L("%d key pressed"),keyCode);
			OstTrace1(TRACE_NORMAL, CTESTENGINE_RUNL_DUP02, "%d key pressed",keyCode);
			}
		}
	else
		{
		gtest.Printf(_L("Manual key error %d\n"),completionCode);
		OstTrace1(TRACE_NORMAL, CTESTENGINE_RUNL_DUP03, "Manual key error %d\n",completionCode);
		SetActive();
		}
	OstTraceFunctionExit1( CTESTENGINE_RUNL_EXIT, this );
	}
	
	
TInt CTestEngine::RunError(TInt aError)
	{
	OstTraceFunctionEntryExt( CTESTENGINE_RUNERROR_ENTRY, this );
	OstTraceFunctionExitExt( CTESTENGINE_RUNERROR_EXIT, this, KErrNone );
	return KErrNone;
	}

	}
