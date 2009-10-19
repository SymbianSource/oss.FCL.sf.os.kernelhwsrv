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

#ifndef TESTCASE0679_H
#define TESTCASE0679_H

	
//----------------------------------------------------------------------------------------------		
//! @SYMTestCaseID				PBASE-T_OTGDI-0679
//! @SYMTestCaseDesc			A-Device requests bus, VBUS is high, device not b_hnp_enabled
//! @SYMFssID 
//! @SYMPREQ					1305
//! @SYMREQ						8929
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			1.	A-Device raises VBus
//!                             2.	A-Device and B-Device wait to arrive in default roles (A-Host, B-Peripheral)
//!								3.	B-Peripheral waits to arrive in configured state, A-Device waits to become A-Peripheral
//!								4.	A-Host enumerates newly attached peripheral, and suspends it
//!								5.	B-Peripheral waits to arrive in suspended state
//!								6.	B-Device makes a bus request
//!								7.	No HNP occurs, B-Device generates message to say HNP is not possible
//!								8.	A-Device drops VBus
//!								9.	A-Device and B-Device both observe VBus dropping.
//!                             
//! @SYMTestExpectedResults		There should be no role swap, and error message should be generated at step 7.
//!                             No state changes will have occurred and A-Host and B-Peripheral should be observed.
//! @SYMTestStatus              Defined	
//----------------------------------------------------------------------------------------------	

class CTestCase0679 : public CTestCaseB2BRoot
	{
public:
	static CTestCase0679* NewL(TBool aHost);
	virtual ~CTestCase0679(); 	
	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);
	
	void RunStepL();
	
	TInt GetStepIndex()	{ return(iCaseStep); };	
	
private:
	CTestCase0679(TBool aHost);
	void ConstructL();

	// DATA
private:		

	enum TCaseSteps
		{
		EPreconditions,
		ELoadLdd,				//	Load LDDs, trigger FDFActor
		EReadyToRaiseVBus,		//	Wait until VBus arrives
		EDefaultRoles,   		//	B-device as peripheral, A-device as host
		EBConfigured,			//	B-Device is configured
		EBSuspended,			//	B-device is suspended
		EBErrorReceived,		//	B-device receives error message (refusal to role-swap because HNP is not enabled)

		EDropVBus,				//	Tidyup steps
		EVBusDropped,
		EUnloadLdd,
		EUnloadClient,
		ELastStep
		};
	
	TCaseSteps iCaseStep;
	TInt       iStateRetry;	// swallow other events
	
	const static TTestCaseFactoryReceipt<CTestCase0679> iFactoryReceipt;
	};
	
	
	
#endif // TESTCASE0679_H
