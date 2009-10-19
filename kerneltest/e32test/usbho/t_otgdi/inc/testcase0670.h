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

#ifndef TESTCASE0670_H
#define TESTCASE0670_H

const TInt KDelayDurationForTest4_5	= 30000;
const TInt KDelayDurationForTest5_5	= 60000;


	
//-------------------------------------------------------------------------------------------------------		
//! @SYMTestCaseID				PBASE-T_OTGDI-0670
//! @SYMTestCaseDesc			Default OPT Support 
//! @SYMFssID 
//! @SYMPREQ					1782
//! @SYMREQ						N/A
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			1.Compliance test role swap against OPT TD4.5 
//! @SYMTestExpectedResults 	1.
//! @SYMTestStatus              Undefined	
//--------------------------------------------------------------------------------------------------------	

	class CTestCase0670 : public CTestCaseRoot
	{
public:
	static CTestCase0670* NewL(TBool aHost);
	virtual ~CTestCase0670(); 	
	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);
	
	void RunStepL();
	virtual void DescribePreconditions();
	TInt GetStepIndex()	{ return(iCaseStep); };

	static void CancelDrive(CTestCaseRoot *pThis);	
	
private:
	CTestCase0670(TBool aHost);
	void ConstructL();

	// DATA
private:		
	TInt iRepeats;		// loop counter, 

	enum TCaseSteps
		{
		EPreconditions,
		ELoadWithOptTestMode,
		EConnectAtoB,
		EStartOptTD4_5,
		EPromptYOpt4_5,
		EConfirmOpt4_5,		
		EUnloadLdd,
		ELastStep	
		};
	
	TCaseSteps iCaseStep;
	const static TTestCaseFactoryReceipt<CTestCase0670> iFactoryReceipt;
	CTestCaseWatchdog *iWDTimer;	
	
	};
	
	
#endif // TESTCASE0670_H
