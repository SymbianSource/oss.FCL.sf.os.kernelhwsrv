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
// @file testcasecontroller.cpp
// @internalComponent
// 
//

#include "basetestcase.h"
#include "testcasecontroller.h"
#include "testcasefactory.h"
#include "testengine.h"
#include "testpolicy.h"
#include "testdebug.h"

_LIT(KClientDeviceDriverName,"EUSBC");
_LIT(KHostDeviceDriverName,"usbhubdriver");
_LIT(KHostDeviceInterfaceDriverName,"usbdi");
_LIT(KOtgdiLddFileName, "otgdi");

namespace NUnitTesting_USBDI
	{
	
CTestCaseController* CTestCaseController::NewL(CTestEngine& aTestEngine,TBool aHostRole)
	{
	CTestCaseController* self = new (ELeave) CTestCaseController(aTestEngine,aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
	
CTestCaseController::CTestCaseController(CTestEngine& aTestEngine,TBool aHostRole)
:	CActive(EPriorityStandard),
	iTestEngine(aTestEngine),
	iHostRole(aHostRole)
	{
	// Add to current threads active scheduler
	CActiveScheduler::Add(this);
	}
	
	
CTestCaseController::~CTestCaseController()
	{
	LOG_FUNC 

	Cancel(); // Cancels any oustanding test cases

	delete iTestPolicy;
	
	if(iHostRole)
		{	
		TInt err = User::FreeLogicalDevice(KHostDeviceInterfaceDriverName);
		if(err != KErrNone)
			{
			RDebug::Printf("<Error %d> Unable to unload driver: %S",err,&KHostDeviceInterfaceDriverName);
			}
		
		err = User::FreeLogicalDevice(KHostDeviceDriverName);
		if(err != KErrNone)
			{
			RDebug::Printf("<Error %d> Unable to unload driver: %S",err,&KHostDeviceDriverName);
			}
			
		err = User::FreeLogicalDevice(KOtgdiLddFileName);
		if(err != KErrNone)
			{
			RDebug::Printf("<Error %d> Unable to unload driver: %S",err,&KHostDeviceDriverName);
			}			
		}
	else
		{
		TInt err(User::FreeLogicalDevice(KClientDeviceDriverName));
		if(err != KErrNone)
			{
			RDebug::Printf("<Error %d> Unable to unload driver: %S",err,&KClientDeviceDriverName);
			}		
		}
	}

void CTestCaseController::ConstructL()
	{
	LOG_FUNC
	TInt err = KErrNone;
	
	_LIT(KLoadingNamedDriverString,"loading driver: %S\n");
	_LIT(KLoadedNamedDriverString,"loaded driver: %S\n");
		
	// loading drivers
	if(iHostRole)
		{
		gtest.Printf(KLoadingNamedDriverString,&KHostDeviceDriverName);		
		// Load both Host USB device drivers
		err = User::LoadLogicalDevice(KHostDeviceDriverName);
		gtest((err == KErrNone) || (err == KErrAlreadyExists));
		gtest.Printf(KLoadedNamedDriverString,&KHostDeviceDriverName);
		
		RDebug::Print(KLoadingNamedDriverString,&KHostDeviceInterfaceDriverName);
		err = User::LoadLogicalDevice(KHostDeviceInterfaceDriverName);
		gtest((err == KErrNone) || (err == KErrAlreadyExists));
		gtest.Printf(KLoadedNamedDriverString,&KHostDeviceInterfaceDriverName);
		  												 
		// If test cases are running USB host side actions
		// then run each test case in its own thread		
		iTestPolicy = CThreadTestPolicy::NewL();
		}
	else
		{
				  		
		// Load the USB client driver	
		gtest.Printf(KLoadingNamedDriverString,&KClientDeviceDriverName);
		err = User::LoadLogicalDevice(KClientDeviceDriverName);
		gtest((err == KErrNone) || (err == KErrAlreadyExists));
		gtest.Printf(KLoadedNamedDriverString,&KClientDeviceDriverName);
		
		// Run each test case in the main thread as its not new API 
		// and not expected to panic
		iTestPolicy = CBasicTestPolicy::NewL();		
		}
	
	// Get the identity of the next test case to run	
	err = iTestEngine.NextTestCaseId(iTestCaseId);
	gtest.Next(iTestCaseId);
	
	// Run the test case	
	iTestPolicy->RunTestCaseL(iTestCaseId,iStatus);
	SetActive();
	}
	

void CTestCaseController::DoCancel()
	{
	// Cancel the outstanding test case running

	iTestPolicy->Cancel();
	}
	
	
void CTestCaseController::RunL()
	{
	LOG_FUNC

	// Retrieve the completion code of the last test case run
	TInt err(iStatus.Int());
	
	TBuf<64> log;
	if(err != KErrNone)
		{
		iTestCasesResults.Append(EFalse);
		gtest.Printf(_L("FAILED err=%d\n"),err);
		}
	else
		{
		iTestCasesResults.Append(ETrue);
		gtest.Printf(_L("PASSED\n"));
		}
		
	// Get the identity of the next test case to run
		
	err = iTestEngine.NextTestCaseId(iTestCaseId);
	if(err == KErrNone)
		{
		RDebug::Printf("\n");
		RDebug::Printf("\n");
		RDebug::Printf("\n");
		gtest.Next(iTestCaseId);
		RDebug::Printf("                              --------------------");
		RDebug::Printf("\n");
		RDebug::Printf("\n");
		RDebug::Printf("\n");
		
		// Run the next test case
		
		iTestPolicy->RunTestCaseL(iTestCaseId,iStatus);
		SetActive();
		}
	else if(err == KErrNotFound)
		{
		RDebug::Printf("All specified test cases performed");
		RDebug::Printf("----------------------------------");
		
		
		// count nb failures
		TUint nbFailures = 0;
		for(TInt test = 0; test < iTestCasesResults.Count() ; test++)
			{
			if(!iTestCasesResults[test])
				//NB iTestCasesResults is a boolean array added to each time a test is run...
				//   ...even if it is a repeat.
				{
				nbFailures++;
				}
			}
		RDebug::Printf("There are %d test case results, %d failures", iTestCasesResults.Count(), nbFailures);

		// Number of tests that should have been run (including repeats)
		TUint nbTests = iTestEngine.TestCasesIdentities().Count() * iTestEngine.NumRepeats();
		if(nbTests!=iTestCasesResults.Count())
			{
			RDebug::Printf("The number of tests that should have been run (%d) DOES NOT EQUAL the actual number of tests run (%d).", 
						  nbTests, iTestCasesResults.Count());
			RDebug::Printf("This test suite will now PANIC!");
			}
		ASSERT((nbTests==iTestCasesResults.Count()));

		
		for(TInt repeat = 0; repeat < iTestEngine.NumRepeats() ; repeat++)
			{
			if(iTestEngine.NumRepeats() > 1)
				{
				RDebug::Printf("Test Case Loop %d..........",	repeat+1);			
				}
			for(TInt testIndex = 0; testIndex < iTestEngine.TestCasesIdentities().Count() ; testIndex++)
				{
				if(iTestCasesResults[testIndex])
					{
					RDebug::Print(_L("Test Case: %S : PASSED"),	(iTestEngine.TestCasesIdentities())[testIndex]);			
					}
				else
					{
					RDebug::Print(_L("Test Case: %S : FAILED"),	(iTestEngine.TestCasesIdentities())[testIndex]);
					}
				}
			}

		RDebug::Printf("CActiveScheduler::Stop CTestCaseController::RunL");
		CActiveScheduler::Stop();
		}
	else
		{
		RDebug::Printf("<Error %d> Unknown error from CTestEngine::NextTestCaseId",err);
		User::Leave(err);
		}
	}
	
	
TInt CTestCaseController::RunError(TInt aError)
	{
	LOG_FUNC
	
	switch(aError)
		{
		case KErrNoMemory: //follow through
		default:
			// Panic the test module
			gtest(EFalse);
			break;
		}
	return KErrNone;
	}


	}




