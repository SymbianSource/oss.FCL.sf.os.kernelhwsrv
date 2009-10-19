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

#ifndef TESTCASE0676_H
#define TESTCASE0676_H


	
//----------------------------------------------------------------------------------------------		
//! @SYMTestCaseID				PBASE-T_OTGDI-0476
//! @SYMTestCaseDesc			A-Device requests bus, VBUS down (PBASE-USB_OTGDI-0676)
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

	class CTestCase0676 : public CTestCaseRoot
	{
public:
	static CTestCase0676* NewL(TBool aHost);
	virtual ~CTestCase0676();
	
protected:	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);
	
	void RunStepL();
	virtual void DescribePreconditions();
	TInt GetStepIndex()	{ return(iCaseStep); };

	static void CancelNotify(CTestCaseRoot *pThis);	
	
private:
	CTestCase0676(TBool aHost);
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
	
	const static TTestCaseFactoryReceipt<CTestCase0676> iFactoryReceipt;
	
	CTestCaseWatchdog *iWDTimer;	

	void ContinueAfter(TTimeIntervalMicroSeconds32 aMicroSecs, TCaseSteps step);
	};

	
#endif // TESTCASE0676_H
