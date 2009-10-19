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

#ifndef TESTCASE0464_H
#define TESTCASE0464_H


	
//----------------------------------------------------------------------------------------------		
//! @SYMTestCaseID				KPBASE-T_OTGDI-0464
//! @SYMTestCaseDesc			VBus drive and drop.
//! @SYMFssID 
//! @SYMPREQ					1782
//! @SYMREQ						7080
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			1. Load LDD and start stack
//! 							2. raise Vbus
//! 							3. wait for confirmation input (7 seconds)
//! 							4. drop vBus
//! 							5. wait for confirmation manual input (7 sec)
//! 							6. end test
//! @SYMTestExpectedResults 	1. user input at step 3 and 5 is a 'Y' to indicate 
//!                                5V and 0V measured at each occasion
//----------------------------------------------------------------------------------------------		

	class CTestCase0464 : public CTestCaseRoot
	{
public:
	static CTestCase0464* NewL(TBool aHost);
	virtual ~CTestCase0464(); 	

	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);
	
	void RunStepL();
	virtual void DescribePreconditions();
	TInt GetStepIndex()	{ return(iCaseStep); };
		
private:
	CTestCase0464(TBool aHost);
	void ConstructL();

	// DATA
public:

private:	

	enum TCaseSteps
		{
		EPreconditions,
		ELoadLdd,
		EDriveBus,	
		EUnloadLdd,
		EVerifyVBusGone,
		ELastStep	
		};
	
		
	TCaseSteps iCaseStep;
	
	const static TTestCaseFactoryReceipt<CTestCase0464> iFactoryReceipt;
	
	
	};
	
	
#endif // TESTCASE0464_H
