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
// @file testengine.cpp
// @internalComponent
// 
//

#include "TestEngine.h"
#include "testdebug.h"
#include "TestCaseController.h"
#include "TestCaseFactory.h"

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
	CTestEngine* self = new (ELeave) CTestEngine;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}



CTestEngine::CTestEngine()
:	CActive(EPriorityUserInput),
	iTestCaseIndex(0), iRepeat(0), iNumRepeats(KDefaultNumRepeats)
	{
	}



CTestEngine::~CTestEngine()
	{
	// Cancel reading user console input
	Cancel();
	
	// Destroy the test case controller
	delete iTestCaseController;
	
	// Destroy the identity array and its contents
	iTestCasesIdentities.ResetAndDestroy();
	
	// Finish test and release resources
	gtest.End();
	gtest.Close();
	}
	
	
	
void CTestEngine::ConstructL()
	{
	LOG_FUNC
	CActiveScheduler::Add(this);

	// Display information (construction text and OS build version number
	gtest.Title();
	gtest.Start(_L("Test Engine Initiation"));
	gtest.Printf(_L(">>\n"));
	gtest.Printf(_L(">>   T E S T   R U N \n"));
	gtest.Printf(_L(">>\n"));

	// Process the command line option for role
	TInt cmdLineLength(User::CommandLineLength());
	HBufC* cmdLine = HBufC::NewMax(cmdLineLength);
	CleanupStack::PushL(cmdLine);
	TPtr cmdLinePtr = cmdLine->Des();
	User::CommandLine(cmdLinePtr);
	
	// be careful, command line length is limited(248 characters)	
	gtest.Printf(_L("***cmdLine = %lS\n"), cmdLine);
		
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
			gtest(EFalse);
			}
		}
	else
		{
		gtest.Printf(_L("Test configuration option not found: %S\n"),&KArgRole);
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
			
						
			iTestCasesIdentities.Append(tc);
			CleanupStack::Pop(tc);
			remCases.Set(testCases.Right(remCases.Length()-pos-1));
			}
		}
	else
		{
		gtest.Printf(_L("Test configuration option not found: %S\n"),&KArgTestCases);
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
			gtest.Printf(_L("DEFAULT to number of repeats = %d\n"),KDefaultNumRepeats);
			iNumRepeats = KDefaultNumRepeats;
			}
		gtest.Printf(_L("Test repeats specified: %d cycles\n"),iNumRepeats);
		}
	else
		{
		gtest.Printf(_L("Test configuration option not found: %S\n"),&KArgTestRepeats);
		gtest.Printf(_L("DEFAULT to number of repeats = %d\n"),KDefaultNumRepeats);
		iNumRepeats = KDefaultNumRepeats;
		}
		
	// Create the test case controller
	gtest.Printf(_L("Creating the test controller\n"));
	iTestCaseController = CTestCaseController::NewL(*this,hostFlag);

	CleanupStack::PopAndDestroy(cmdLine);

	gtest.Console()->Read(iStatus);
	SetActive();	
	}
	

TInt CTestEngine::NextTestCaseId(TDes& aTestCaseId)
	{
	LOG_FUNC
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
		return KErrNone;
		}
	else
		{
		return KErrNotFound;
		}
	}

RPointerArray<HBufC>& CTestEngine::TestCasesIdentities()
	{
	return iTestCasesIdentities;
	}

TUint CTestEngine::NumRepeats()
	{
	return iNumRepeats;
	}

void CTestEngine::DoCancel()
	{
	LOG_FUNC
	gtest.Console()->ReadCancel();	
	}
	

void CTestEngine::RunL()
	{
	LOG_FUNC
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
			RDebug::Printf("CActiveScheduler::Stop CTestEngine::RunL");
			CActiveScheduler::Stop();
			}
		else
			{
			gtest.Printf(_L("%d key pressed"),keyCode);
			}
		}
	else
		{
		gtest.Printf(_L("Manual key error %d\n"),completionCode);
		SetActive();
		}
	}
	
	
TInt CTestEngine::RunError(TInt aError)
	{
	return KErrNone;
	}

	}
