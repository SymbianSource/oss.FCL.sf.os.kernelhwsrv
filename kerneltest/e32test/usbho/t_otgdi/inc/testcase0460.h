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
// 'A' connector removal test
// 
//


#ifndef TESTCASE0460_H
#define TESTCASE0460_H


	
//----------------------------------------------------------------------------------------------	
//!	@SYMTestCaseID				PBASE-T_OTGDI-0460
//!	@SYMTestCaseDesc			Must be able to detect an A plug insertion. Test proves we can subscribe for insertion events.
//!	@SYMFssID 
//!	@SYMPREQ					1782
//!	@SYMREQ						7079
//!	@SYMTestType				UT
//!	@SYMTestPriority			1 
//!	@SYMTestActions 			1. Call OtgEventNotification() to register for insert/remove event.
//!								2. Remove A-Plug, removal is detected.
//!	@SYMTestExpectedResults 	2. A-Plug removal event fires when it should.
//----------------------------------------------------------------------------------------------
	
	class CTestCase0460 : public CTestCaseRoot
	{
public:
	static CTestCase0460* NewL(TBool aHost);
	virtual ~CTestCase0460(); 	

	virtual void ExecuteTestCaseL();
	void DoCancel();

	void RunStepL();
	virtual void DescribePreconditions();
	TInt GetStepIndex()	{ return(iCaseStep); };
		
private:
	CTestCase0460(TBool aHost);
	void ConstructL();

	// DATA
public:

	
private:
	TInt iDequeAttempts;

	enum TCaseSteps
		{
		EPreconditions,
		ELoadLdd,
		ERegisterForEvents,		// QueueOtgEventRequest( event, status );
		EDriveID_PIN,
		EWait5,
		ETestStateA,
		EUnloadLdd,
		ELastStep	
		};
			
	TCaseSteps iCaseStep;
	
	const static TTestCaseFactoryReceipt<CTestCase0460> iFactoryReceipt;
	};
	
	
#endif // TESTCASE0460_H
