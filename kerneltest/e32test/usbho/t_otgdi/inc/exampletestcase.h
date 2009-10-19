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

#ifndef EXAMPLETESTCASE_H
#define EXAMPLETESTCASE_H


	
/**	
	@SYMTestCaseID				EXAMPLETESTCASE
	@SYMTestCaseDesc			Description - shows various test-framework features
	@SYMFssID 					none
	@SYMPREQ					preq#
	@SYMREQ						n/a
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			1. Empty step
								2. Press any key step (with watchdog)
								3. Another press key step (with watchdog)
								4. Watchdog cancel function works
								5. delay step
								6. Empty step
								7. End the test
	@SYMTestExpectedResults 	Example
	@SYMTestStatus				Proto
	
*/
	
// Example test case:
// Something to test the Engine with if ever it changes. Verify development changes, and 
// new test-case creation steps here.
class CExampleTestCase : public CTestCaseRoot
	{
public:
	static CExampleTestCase* NewL(TBool aHost);
	virtual ~CExampleTestCase();

	void WatchdogExpired();

	virtual void ExecuteTestCaseL();
	void DoCancel();
	void RunStepL();
	
	//override the base-class
	void ProcessKey(TKeyCode &aKey);
	TInt GetStepIndex()	{ return(iCaseStep); };
		
	virtual void DescribePreconditions();
	
private:
	CExampleTestCase(TBool aHost);
	void ConstructL();

	
	CTestCaseWatchdog *iWDTimer;
	
	// cancel callback methods (not implemented as a 'Mixin)
	static void FuncA(CTestCaseRoot *pThis);
	static void FuncB(CTestCaseRoot *pThis);
	static void FuncC(CTestCaseRoot *pThis);
	
private:

	enum TCaseSteps
		{
		EFirstStep,
		ESecondStep,
		EThirdStep,
		EFourthStep,
		EFifthStep,
		ESixthStep,
		ELastStep	
		};
	
	TCaseSteps iCaseStep;
	
	const static TTestCaseFactoryReceipt<CExampleTestCase> iFactoryReceipt;
	
	
	};
	
	
#endif // EXAMPLETESTCASE_H
