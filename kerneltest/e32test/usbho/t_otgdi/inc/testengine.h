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

#ifndef TESTENGINE_H
#define TESTENGINE_H


extern RTest test;

extern TPtrC KMyApplicationName;

extern TBool gSemiAutomated;
extern TBool gVerboseOutput;
extern TInt  gOOMIterations;
extern TInt  gOpenIterations;
extern TBool gTestRoleMaster; 
extern TUint gUSBVidPid;

	
// Forward declarations
class CTestCaseController;

//	
// This class represents the engine for this test module.  It is controlled by user (manual) 
// key commands.
//

class CTestEngine : public CBase  //: public CActive
	{
public:
	/**
	Symbian Construction
	@return a pointer to a new instance of CTestEngine
	*/
	static CTestEngine* NewL();


	~CTestEngine();
	
	/**
	Retrieve the next in the list of test cases identities to run
	@param an empty descriptor to populate with the next test case identity
	@return KErrNone if successful or KErrNotFound if test cases identities depleted
	*/
	TInt NextTestCaseId(TDes& aTestCaseId);

	void Report();
	// keyboard input
	static void GetNumericInput(TInt &aNumber);

private: 
	// From CActive
	void DoCancel();
	
	void RunL();
	
	TInt RunError(TInt aError);	
	
	CTestEngine();
	
	/**
	2nd phase constructor
	*/
	void ConstructL();

	// command-line handling methods
	void ResetTestCaseIndex() {iTestCaseIndex=0;}
	void AddAllDefaultTests();	
	void ProcessCommandLineL();
	void PrintUsage();

	/**
	The test cases that the user wishes to execute
	*/
	RPointerArray<HBufC> iTestCasesIdentities;
	TInt 	iTestCaseIndex;
	TBool 	iHelpRequested;
	
	/**
	The test controller
	*/
	CTestCaseController* iTestCaseController;
	};

	
#endif  // TESTENGINE_H
