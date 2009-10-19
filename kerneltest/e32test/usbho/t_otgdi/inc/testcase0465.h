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

#ifndef TESTCASE0465_H
#define TESTCASE0465_H

	
//----------------------------------------------------------------------------------------------		
//! @SYMTestCaseID				PBASE-T_OTGDI-0465
//! @SYMTestCaseDesc			VBus drive and drop.
//! @SYMFssID 
//! @SYMPREQ					1782
//! @SYMREQ						7080
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			1. Apply a dummy Load using the OET of 450 Ohm so that IBus > 8mA
//! 							2. Wait 1 second.
//! 							3. Remove the dummy load via the OET board. Retrieve error event from error callback
//! 							4. Measure VBus=0
//! @SYMTestExpectedResults 	1. Initial value for VBus > 4.4 and <5.25 volts
//! 							Between steps 2 and 3, we expect to see an error event, with value EMessageVbusProblem
//! 							5. Value for VBus < 0.2v (See Notes on VBus in section 6.5.2)
//----------------------------------------------------------------------------------------------	

	class CTestCase0465 : public CTestCaseRoot
	{
public:
	static CTestCase0465* NewL(TBool aHost);
	virtual ~CTestCase0465(); 	
	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);
	
	void RunStepL();
	virtual void DescribePreconditions();
	TInt GetStepIndex()	{ return(iCaseStep); };

	static void CancelDrive(CTestCaseRoot *pThis);	
	
private:
	CTestCase0465(TBool aHost);
	void ConstructL();


	// DATA
private:		

	TInt iRepeats;		// loop counter, 

	enum TCaseSteps
		{
		EPreconditions,
		ELoadLdd,		// load and ...
		EDriveVBus,		// drive VBUS
		EVerifyVBus,	// double-checker step
		EVerifyBusGone,	// verify VBus dropped (manually!)
		EUnloadLdd,		// unload 
		ELastStep	
		};
	
		
	TCaseSteps iCaseStep;
	
	const static TTestCaseFactoryReceipt<CTestCase0465> iFactoryReceipt;
	
	CTestCaseWatchdog *iWDTimer;	
	
	};
	
	
#endif // TESTCASE0465_H
