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

#ifndef TESTCASE0677_H
#define TESTCASE0677_H

	
//----------------------------------------------------------------------------------------------		
//! @SYMTestCaseID				PBASE-T_OTGDI-0677
//! @SYMTestCaseDesc			B-Device Role-swap attempt, VBUS down
//! @SYMFssID 
//! @SYMPREQ					1305
//! @SYMREQ						8929
//! @SYMTestType				UT
//! @SYMTestPriority			1
//!                             LDD is loaded. OTGDI driver session is open
//!                             OET inserted with SW9 set to B-Device
//!                             No A device connected, A plug not connected
//! @SYMTestActions 			1.	Call function on OTGDI to attempt role swap – BusRequest() 
//!                             2.	Retrieve error event from error callback
//! @SYMTestExpectedResults 	The call to BusRequest() will trigger the sending of an SRP.
//!                             The SRP must be observed with a dual-trace oscilloscope measuring VBus and D+ and demonstrate the expected waveforms (Figure 5.5 in [R5])
//!                             The A device, if present, should NOT raise VBus.
//!                             The Error retrieved from the callback should be KErrUsbOtgSrpTimeout (-6691)
//!                             Note this test case doesn’t, and isn’t intended to complete role swap.  That is tested in PBASE-USB_OTG-0681
//! @SYMTestStatus              Defined	
//----------------------------------------------------------------------------------------------	

	class CTestCase0677 : public CTestCaseRoot
	{
public:
	static CTestCase0677* NewL(TBool aHost);
	virtual ~CTestCase0677(); 	
	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);
	
	void RunStepL();
	virtual void DescribePreconditions();
	TInt GetStepIndex()	{ return(iCaseStep); };

	static void CancelNotify(CTestCaseRoot *pThis);	
	
private:
	CTestCase0677(TBool aHost);
	void ConstructL();

	enum ECancelMethods {ECancelVBusNotify, ECancelEventNotify, ECancelMessageNotify};
	ECancelMethods iCancelWhat;
	TTime iTimeSRPStart; // time in microseconds since 0AD nominal Gregorian
	//TTime iTimeNow;
	
	// DATA
private:		

	TInt iRepeats;		// loop counter, 


	enum TCaseSteps
		{
		EPreconditions,
		ELoadLdd,				// load client+otg
		EDetectBPlug,			// double-check before starting
		ERequestBus,			// drive
		EWaitForSRPInitiated,	// Wait for SRP active event
		EWaitForSRPTimeout,		// Wait for SRP timeout event
		EIssueSRPObservedPrompt,// Issue message to user to verify SRP was observed
		ECheckSRPObservedUserInput,	// Check user input from previous step.
		EUnloadLdd,				// unload otg 
		ELastStep
		};
	
		
	TCaseSteps iCaseStep;
	
	const static TTestCaseFactoryReceipt<CTestCase0677> iFactoryReceipt;
	
	CTestCaseWatchdog *iWDTimer;	

	void ContinueAfter(TTimeIntervalMicroSeconds32 aMicroSecs, TCaseSteps step);
	};

	
#endif // TESTCASE0677_H
