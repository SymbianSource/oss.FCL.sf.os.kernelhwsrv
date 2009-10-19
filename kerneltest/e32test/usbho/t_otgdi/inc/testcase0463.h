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

#ifndef TESTCASE0463_H
#define TESTCASE0463_H



	
//----------------------------------------------------------------------------------------------
//!	@SYMTestCaseID				PBASE-T_OTGDI-0463
//!	@SYMTestCaseDesc			OTGDI driver session can be repeatedly opened and closed without a) panicking or b) leaking resources.
//!	@SYMFssID 
//!	@SYMPREQ					1782
//!	@SYMREQ						n/a
//!	@SYMTestType				UT
//!	@SYMTestPriority			1 
//!	@SYMTestActions 			1. Call function to open OTGDI driver session.
//!								2. Call function to close OTGDI driver session.
//!								3. Repeat from step 1 (x 3)
//!	@SYMTestExpectedResults 	No panic occurs, no error code returned, we get session handle; session gets closed
//!	@SYMTestStatus				Proto
//----------------------------------------------------------------------------------------------	

	class CTestCase0463 : public CTestCaseRoot
	{
public:
	static CTestCase0463* NewL(TBool aHost);
	virtual ~CTestCase0463(); 	

	virtual void ExecuteTestCaseL();
	void DoCancel();
	void RunStepL();
	virtual void DescribePreconditions();
	TInt GetStepIndex()	{ return(iCaseStep); };
		
private:
	CTestCase0463(TBool aHost);
	void ConstructL();


	// DATA
private:

	enum TCaseSteps
		{
		EMarkStack,
		ELoadLdd,
		EDoSomething,	// does nothing right now
		EUnloadLdd,
		ELoopDecrement,
		ELastStep	
		};
	
	TCaseSteps iCaseStep;
	TInt       iRepeats;		// loop counter, set to run 10 times over
	TInt       iAllocFailNumber; // May be same as the loop index, 
	                             // separated to allow us to adjust separately

	TInt		* aIntegerP;
	
	const static TTestCaseFactoryReceipt<CTestCase0463> iFactoryReceipt;
	
	
	};
	
	
#endif // TESTCASE0463_H
