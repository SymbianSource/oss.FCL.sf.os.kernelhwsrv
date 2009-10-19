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

#ifndef TESTCASE0468_H
#define TESTCASE0468_H


	
//----------------------------------------------------------------------------------------------		
//! @SYMTestCaseID				PBASE-T_OTGDI-0468
//! @SYMTestCaseDesc			Alternative ID_PIN detection API.
//! @SYMFssID 
//! @SYMPREQ					1782
//! @SYMREQ						7080
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			1.	Drive VBus BusRequest()
//!                             2.	Register for VBus notification method QueueOtgVbusNotification() Delay at least 200ms
//!                             3.	Wait 1 second max for VBus HIGH event
//!                             4.	Drop VBus BusDrop()
//!                             5.	Register for VBus notification method QueueOtgVbusNotification()
//!                             6.	Wait 1 second max for VBus LOW event
//!                             7.	Repeat steps 1 through 6, 3 times over
//! @SYMTestExpectedResults 	Between steps 2 and 3, we expect to see an HIGH vbus fire,
//! 							Between steps 5,6 we expect another LOW vbus event. Fail the test 
//!                             if event does not arrive in the 200milli -second time
//----------------------------------------------------------------------------------------------	

	class CTestCase0468 : public CTestCaseRoot
	{
public:
	static CTestCase0468* NewL(TBool aHost);
	virtual ~CTestCase0468(); 	
	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);
	
	void RunStepL();
	virtual void DescribePreconditions();
	TInt GetStepIndex()	{ return(iCaseStep); };

	static void CancelNotify(CTestCaseRoot *pThis);	
	
private:
	CTestCase0468(TBool aHost);
	void ConstructL();


	// DATA
private:		

	TInt iRepeats;		// loop counter, 


	enum TCaseSteps
		{
		EPreconditions,
		ELoadLdd,		// load 
		EDetectAPlug,	// double-check before starting
		ELoopControl,	// loop: loop control  =3x3 times (wait 50ms)
		ELoopDriveVBus,	// loop: drive
		ELoopVerifyVBus,// loop: check
		ELoopWait,      // loop: wait (50ms)
		ELoopDropVBus,  // loop: drop
		ELoopVerifyDrop,// loop: test Vbus dropped, and repeat ELoopDriveVBus
		EUnloadLdd,		// unload 
		ELastStep
		};
	
		
	TCaseSteps iCaseStep;
	
	const static TTestCaseFactoryReceipt<CTestCase0468> iFactoryReceipt;
	
	CTestCaseWatchdog *iWDTimer;	

	void ContinueAfter(TTimeIntervalMicroSeconds32 aMicroSecs, TCaseSteps step);
	};
	
	
#endif // TESTCASE0468_H
