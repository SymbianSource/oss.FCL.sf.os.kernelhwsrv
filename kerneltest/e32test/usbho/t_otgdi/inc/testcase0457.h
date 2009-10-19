#ifndef TESTCASE_0457_H
#define TESTCASE_0457_H
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
// Open/Close 'disconnected' test
// 2. Call function to close OTGDI driver session.
// 3. Repeat from step 1 (x 3)
// 
//

//! @SYMTestCaseID				PBASE-T_OTGDI-0457
//! @SYMTestCaseDesc			OTGDI driver session can be repeatedly opened and closed without a) panicking or b) leaking resources.
//! @SYMFssID 
//! @SYMPREQ					1782
//! @SYMREQ						n/a
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			1. A plug removed. Call function to open OTGDI driver session.
//! @SYMTestExpectedResults 	No panic occurs, no error code returned, we get session handle; session gets closed
//! @SYMTestStatus				Proto

	class CTestCase0457 : public CTestCaseRoot
	{
public:
	static CTestCase0457* NewL(TBool aHost);
	virtual ~CTestCase0457(); 	

	virtual void ExecuteTestCaseL();
	void DoCancel();
	void RunStepL();

	virtual void DescribePreconditions();
	TInt GetStepIndex()	{ return(iCaseStep); };
		
private:
	CTestCase0457(TBool aHost);
	void ConstructL();


	// DATA
private:
	enum TCaseSteps
		{
		EPreconditions,
		ELoadLdd,
		EUnloadLdd,
		ELoopDecrement,
		ELastStep	
		};
	
	TCaseSteps iCaseStep;
	TInt       iRepeats;		// loop counter, set to run 3 times over
	
	const static TTestCaseFactoryReceipt<CTestCase0457> iFactoryReceipt;
	
	};
	
	
#endif // TESTCASE_0457_H
