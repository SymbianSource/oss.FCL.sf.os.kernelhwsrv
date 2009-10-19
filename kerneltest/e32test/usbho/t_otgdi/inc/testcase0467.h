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

#ifndef TESTCASE0467_H
#define TESTCASE0467_H


	
//----------------------------------------------------------------------------------------------		
//! @SYMTestCaseID				PBASE-T_OTGDI-0467
//! @SYMTestCaseDesc			Must be able to detect an A plug insertion using simplified API
//! @SYMFssID 
//! @SYMPREQ					1782
//! @SYMREQ						7080
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			1. Register for events with QueueOtgIdPinNotification()
//!                             2. Manually plug in mini A plug into DEV-board without anything on the other end of cable.
//!                             3. Wait max 1 second for insertion event.
//!                             4. Register for events with QueueOtgIdPinNotification()
//!                             5. Remove plug	
//! 							6. Wait max 1 second for remove event.
//! 							7. Repeat steps 1 through 6, 3 times over.
//! @SYMTestExpectedResults 	Between steps 2 and 3, we expect to see an event,
//! 							Between steps 4,5 we expect another event. Fail the test 
//!                             if event does not arrive in the 1 second time
//----------------------------------------------------------------------------------------------	

	class CTestCase0467 : public CTestCaseRoot
	{
public:
	static CTestCase0467* NewL(TBool aHost);
	virtual ~CTestCase0467(); 	
	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);
	
	void RunStepL();
	virtual void DescribePreconditions();
	TInt GetStepIndex()	{ return(iCaseStep); };

	static void CancelIdPin(CTestCaseRoot *pThis);
	
private:
	CTestCase0467(TBool aHost);
	void ConstructL();


	// DATA
private:		

	enum TCaseSteps
		{
		EPreconditions,
		ELoadLdd,		// load and prompt
		ERepeatLoop,		// set up to repeat the test
		EWaitForIDPresent,	// Q
		EVerifyIDPresent,	// checker step
		EWaitForIDGone,		// Q
		EVerifyIDGone,		// checker step
		EUnloadLdd,		// unload 
		ELastStep	
		};
	
		
	TCaseSteps iCaseStep;
	TInt       iRepeats;		// loop counter, set to run 3 times over
	TInt       iDetectionRetry;	// swallow other events
	
	const static TTestCaseFactoryReceipt<CTestCase0467> iFactoryReceipt;
	
	CTestCaseWatchdog *iWDTimer;	
	
	};
	
	
#endif // TESTCASE0467_H
