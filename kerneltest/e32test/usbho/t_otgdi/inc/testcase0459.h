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
// 'A' connector detection
// 
//


#ifndef TESTCASE0459_H
#define TESTCASE0459_H


	
//----------------------------------------------------------------------------------------------		
//!	@SYMTestCaseID				PBASE-T_OTGDI-0459
//!	@SYMTestCaseDesc			Get ID_PIN notifications
//!	@SYMFssID 
//!	@SYMPREQ					1782
//!	@SYMREQ						7079
//!	@SYMTestType				UT
//!	@SYMTestPriority			1 
//!	@SYMTestActions 			1. Fetch the OTG controller state (using QueueOtgStateRequest())
//!								2. Register for events with OtgEventNotification()
//!								3. Use internal API to manipulate ID pin state to simulate A plug presence
//!								4. Wait max 5 seconds for insertion event.
//!								5. Fetch the OTG controller state (using QueueOtgStateRequest())
//!	@SYMTestExpectedResults 	1. 
//!								Between 3. and 4., we expect to get an indication of A plug insertion.
//!								5. 
//----------------------------------------------------------------------------------------------	

	class CTestCase0459 : public CTestCaseRoot
	{
public:
	static CTestCase0459* NewL(TBool aHost);
	virtual ~CTestCase0459(); 	

	virtual void ExecuteTestCaseL();
	void DoCancel();

	void RunStepL();
	virtual void DescribePreconditions();
	TInt GetStepIndex()	{ return(iCaseStep); };
		
private:
	CTestCase0459(TBool aHost);
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
	
	TTime		iIDcheckStart;	// start of checking for ID_PIN
	TTime		iIDcheckEnd;
	
	const static TTestCaseFactoryReceipt<CTestCase0459> iFactoryReceipt;
	
	
	};
	
	
#endif // TESTCASE0459_H
