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
// Open/Close 'powered peripheral' test
// 
//

#ifndef TESTCASE_0458_H
#define TESTCASE_0458_H


	
//----------------------------------------------------------------------------------------------		
//!	@SYMTestCaseID				PBASE-T_OTGDI-0458
//!	@SYMTestCaseDesc			OTGDI driver session can be repeatedly opened and closed without a) panicking or b) leaking resources.
//!	@SYMFssID 
//!	@SYMPREQ					1782
//!	@SYMREQ						n/a
//!	@SYMTestType				UT
//!	@SYMTestPriority			1 
//!	@SYMTestActions 			1. B plug and and plugged into Host. Call function to open OTGDI driver session.
//!								2. Call function to close OTGDI driver session.
//!								3. Repeat from step 1 (x 3)
//!	@SYMTestExpectedResults 	No panic occurs, no error code returned, we get session handle; session gets closed
//!	@SYMTestStatus				Proto
//----------------------------------------------------------------------------------------------		

	class CTestCase0458 : public CTestCaseRoot
	{
public:
	static CTestCase0458* NewL(TBool aHost);
	virtual ~CTestCase0458(); 	

	virtual void ExecuteTestCaseL();
	void DoCancel();
	void RunStepL();

	virtual void DescribePreconditions();
	TInt GetStepIndex()	{ return(iCaseStep); };
	
private:
	CTestCase0458(TBool aHost);
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
	
	const static TTestCaseFactoryReceipt<CTestCase0458> iFactoryReceipt;
	
	};
	
	
#endif // TESTCASE_0458_H
