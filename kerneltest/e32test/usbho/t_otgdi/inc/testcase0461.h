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

#ifndef TESTCASE0461_H
#define TESTCASE0461_H


	
//----------------------------------------------------------------------------------------------	
//! @SYMTestCaseID				PBASE-T_OTGDI-0460
//! @SYMTestCaseDesc			Cancel ID_PIN notifications.
//! @SYMFssID 
//! @SYMPREQ					1782
//! @SYMREQ						7079
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			1. Queue a request for notification of insertion/removal. QueueOtgEventRequest ()
//! 							2. Cancel notification of insertion/removal: call CancelOtgEventRequest()
//! 							3. Use internal API to manipulate ID pin state to simulate A plug removal
//! 							4. Wait max 5 seconds for removal event
//! @SYMTestExpectedResults 	1. Test must pass
//! 							2. - none
//! 							3. Removal event fires before 15 seconds (manual)
//----------------------------------------------------------------------------------------------
	

	class CTestCase0461 : public CTestCaseRoot
	{
public:
	static CTestCase0461* NewL(TBool aHost);
	virtual ~CTestCase0461(); 	

	virtual void ExecuteTestCaseL();
	void DoCancel();

	void RunStepL();
	virtual void DescribePreconditions();
	TInt GetStepIndex()	{ return(iCaseStep); };
	static void CancelKB(CTestCaseRoot *pThis);
		
private:
	CTestCase0461(TBool aHost);
	void ConstructL();

	CTestCaseWatchdog *iWDTimer;
	// DATA
public:

private:
	TInt iDequeAttempts;
	TInt iRepeats;			// loop counter,
	TInt iExpectedEvents; 
	TInt iEventsInQueue;

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
		ETallyNewEvent,
		EUnloadLdd,
		ELastStep	
		};
	
		
	TCaseSteps iCaseStep;

	TTime		iIDcheckStart;	// start of checking for ID_PIN
	TTime		iIDcheckEnd;
	
	const static TTestCaseFactoryReceipt<CTestCase0461> iFactoryReceipt;
	
	};
	
	
#endif // TESTCASE0461_H
