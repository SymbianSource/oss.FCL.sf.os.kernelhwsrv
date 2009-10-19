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

#ifndef TESTCASE0675_H
#define TESTCASE0675_H


	
//----------------------------------------------------------------------------------------------		
//! @SYMTestCaseID				PBASE-T_OTGDI-0675
//! @SYMTestCaseDesc			A-Device requests bus, VBUS is high (PBASE-USB_OTGDI-0675)
//! @SYMFssID 
//! @SYMPREQ					1305
//! @SYMREQ						8927
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			1.	Call function on OTGDI to trigger raise VBUS - BusRequest()
//!                             2.	Retrieve error event from error callback
//! @SYMTestExpectedResults 	Error KErrUsbOtgVbusAlreadyRaised should be observed
//! @SYMTestStatus              Implemented	
//----------------------------------------------------------------------------------------------	

	class CTestCase0675 : public CTestCaseRoot
	{
public:
	static CTestCase0675* NewL(TBool aHost);
	virtual ~CTestCase0675(); 	
	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);
protected:	
	void RunStepL();
	virtual void DescribePreconditions();
	TInt GetStepIndex()	{ return(iCaseStep); };

	static void CancelNotify(CTestCaseRoot *pThis);	
	
private:
	CTestCase0675(TBool aHost);
	void ConstructL();

	// DATA
private:		
	TInt iRepeats;		// loop counter, 


	enum TCaseSteps
		{
		EPreconditions,
		ELoadLdd,			// load 
		EDetectAPlug,		// double-check before starting
		ELoopControl,		// loop: START = 3x3 times (wait 50ms)
		ELoopDriveVBus1,	// loop: drive1
		ELoopVerifyVBus1,	// loop: check1
		ELoopDriveVBus2,	// loop: drive2 (error event fires ok)
		ELoopVerifyVBus2,	// loop: check2
		ELoopWait,      	// loop: wait (50ms)
		ELoopDropVBus,  	// loop: drop (prepare for next itteration)
		ELoopVerifyDrop,    // loop: check (prepare for next itteration)
		EUnloadLdd,		// unload 
		ELastStep
		};
	
		
	TCaseSteps iCaseStep;
	
	const static TTestCaseFactoryReceipt<CTestCase0675> iFactoryReceipt;
	
	CTestCaseWatchdog *iWDTimer;	

	void ContinueAfter(TTimeIntervalMicroSeconds32 aMicroSecs, TCaseSteps step);
	};

	
#endif // TESTCASE0675_H
