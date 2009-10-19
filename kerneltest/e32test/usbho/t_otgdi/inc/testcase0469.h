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

#ifndef TESTCASE0469_H
#define TESTCASE0469_H


	
//----------------------------------------------------------------------------------------------		
//! @SYMTestCaseID				PBASE-T_OTGDI-0469
//! @SYMTestCaseDesc			Alternative ID_PIN detection API.
//! @SYMFssID 
//! @SYMPREQ					1782
//! @SYMREQ						7080
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			1. Load the LDD
//!                             2. Register for Vbus notification method QueueOtgVbusNotification()
//!                             3. Plug other end of cable into a PC Host
//!                             4. Verify VBus High event received within 1 second.
//!                             5. Remove plug from PC Host
//!                             6. Verify VBus LOW event received within 1 second.
//!                             7. Repeat Steps 1 through 6, 3 times over.
//!                             8. Unload LDD
//! @SYMTestExpectedResults 	Between steps 3 and 4, we expect to see Vbus HIGH event,
//! 							Between steps 5,6 we expect Vbus LOW event. 
//!                             Fail the test if event does not arrive in the 1 second time
//----------------------------------------------------------------------------------------------	

class CTestCase0469 : public CTestCaseB2BRoot
	{
public:
	static CTestCase0469* NewL(TBool aHost);
	virtual ~CTestCase0469(); 	
	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);
	
	void RunStepL();
	TInt GetStepIndex()	{ return(iCaseStep); };

	static void CancelNotify(CTestCaseRoot *pThis);	
	
private:
	CTestCase0469(TBool aHost);
	void ConstructL();


	// DATA
private:		

	TInt iRepeats;		// loop counter, 

	enum TCaseSteps
		{
        EPreconditions,
        ELoadLdd,
        ELoopControl,
        ETestVbusRise,
        ETestVbusFall,
        EUnloadLdd,
        ELastStep
		};
	
	TCaseSteps iCaseStep;
	
	const static TTestCaseFactoryReceipt<CTestCase0469> iFactoryReceipt;
	
	void ContinueAfter(TTimeIntervalMicroSeconds32 aMicroSecs, TCaseSteps step);
	};
	
	
#endif // TESTCASE0469_H

