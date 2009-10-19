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

#ifndef TESTCASE0466_H
#define TESTCASE0466_H


	
	
//----------------------------------------------------------------------------------------------	
//! @SYMTestCaseID				PBASE-T_OTGDI-0466
//! @SYMTestCaseDesc			VBus errors.
//! @SYMFssID 
//! @SYMPREQ					1782
//! @SYMREQ						7080
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			1.	Apply a dummy Load using the OET of 450 Ohm so that IBus > 8mA
//!                             2.  Wait 1 second.
//!                             3.	Remove the dummy load via the OET board. Retrieve error event from error callback
//!                             4.	Measure VBus
//! @SYMTestExpectedResults 	1. Initial value for VBus > 4.4 and <5.25 volts
//!                                Between steps 2 and 3, we expect to see an error event, with value EMessageVbusProblem
//!                             4. Value for VBus < 0.2v
//----------------------------------------------------------------------------------------------	

	class CTestCase0466 : public CTestCaseRoot
	{
public:
	static CTestCase0466* NewL(TBool aHost);
	virtual ~CTestCase0466(); 	

	virtual void ExecuteTestCaseL();
	void DoCancel();

	void RunStepL();
	virtual void DescribePreconditions();
	TInt GetStepIndex()	{ return(iCaseStep); };
	
	static void CancelDrive(CTestCaseRoot *pThis);
	
private:
	CTestCase0466(TBool aHost);
	void ConstructL();

	// DATA
public:
	
private:	

	TInt iRepeats;		// loop counter, 

	enum TCaseSteps
		{
		EPreconditions,
		ELoadLdd,
		EDriveBus,		// QueueOtgEventRequest( event, status );
		EVerifyBusFail,
		EUnloadLdd,
		ELastStep	
		};
	
		
	TCaseSteps iCaseStep;
	CTestCaseWatchdog *iWDTimer;
		
	const static TTestCaseFactoryReceipt<CTestCase0466> iFactoryReceipt;
	
	};
	
	
#endif // TESTCASE0466_H
