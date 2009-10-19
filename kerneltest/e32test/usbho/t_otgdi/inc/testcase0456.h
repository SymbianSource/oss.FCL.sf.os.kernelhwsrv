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
// Open/Close 'A'
// 
//

#ifndef TESTCASE_0456_H
#define TESTCASE_0456_H


	
//----------------------------------------------------------------------------------------------		
//!	@SYMTestCaseID				PBASE-T_OTGDI-0456
//!	@SYMTestType				UT
//!	@SYMPREQ					PREQ1782
//!	@SYMREQ						n/a
//!	@SYMTestCaseDesc			OTGDI driver session can be repeatedly opened and closed without a) panicking or b) leaking resources.
//!	@SYMTestActions 			1. A plug inserted. Call function to open OTGDI driver session.
//!								2. Call function to close OTGDI driver session.
//!								3. Repeat from step 1 (x 3)
//!	@SYMTestExpectedResults 	No panic occurs, no error code returned, we get session handle; session gets closed
//!	@SYMTestPriority			High
//!	@SYMTestStatus				Implemented
//----------------------------------------------------------------------------------------------	
	
	class CTestCase0456 : public CTestCaseRoot
	{
public:
	static CTestCase0456* NewL(TBool aHost);
	virtual ~CTestCase0456(); 	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	void RunStepL();
	virtual void DescribePreconditions();

	TInt GetStepIndex()	{ return(iCaseStep); };
	
private:
	CTestCase0456(TBool aHost);
	void ConstructL();

	
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
	
	const static TTestCaseFactoryReceipt<CTestCase0456> iFactoryReceipt;
	
	};
	
	
#endif // TESTCASE_0456_H
