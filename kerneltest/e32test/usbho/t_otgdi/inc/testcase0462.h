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

#ifndef TESTCASE0462_H
#define TESTCASE0462_H


	
//----------------------------------------------------------------------------------------------	
//! @SYMTestCaseID				PBASE-T_OTGDI-0462
//! @SYMTestCaseDesc			Cancel all 3 kinds of notifications. (publish&subscribe)
//! @SYMFssID 
//! @SYMPREQ					1782
//! @SYMREQ						7079
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			1. Queue a request for notification of insertion/removal (OtgEventNotification(?))
//!                             2. Cancel notification of insertion/removal (call OtgEventNotificationCancel()?)
//!                             3. Remove A plug
//!                             4. Insert A plug
//!                             5. Repeat step 3, 6 times
//!                             6. Initialize event counter
//!                             7. Count events = 12 (6 insert and 6 remove notifications)
//! @SYMTestExpectedResults 	1. After step 4 we should not see any notification of the A plug removal.
//! 							
//----------------------------------------------------------------------------------------------	

	class CTestCase0462 : public CTestCaseRoot
	{
public:
	static CTestCase0462* NewL(TBool aHost);
	virtual ~CTestCase0462(); 	
	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);

	void RunStepL();
	virtual void DescribePreconditions();
	TInt GetStepIndex()	{ return(iCaseStep); };

private:
	CTestCase0462(TBool aHost);
	void ConstructL();

	CTestCaseWatchdog *iWDTimer;
	// DATA
public:
	
private:

	TInt iDequeAttempts;
	TInt iRepeats;		// loop counter, 

	enum TCaseSteps
		{
		EPreconditions,
		ELoadLdd,
		ERegisterForEvents,		// QueueOtgEventRequest( event, status );
		EEmptyQueue,
		EGetAndCancelEvent,
		ECancelNotify,
		EInsertA,
		ERemoveA,
		ETallyEvents,
		EUnloadLdd,
		ELastStep	
		};
		
		
	TCaseSteps iCaseStep;

	TTime		iIDcheckStart;	// start of checking for ID_PIN
	TTime		iIDcheckEnd;
	
	const static TTestCaseFactoryReceipt<CTestCase0462> iFactoryReceipt;
	
	
	};
	
	
#endif // TESTCASE0462_H
