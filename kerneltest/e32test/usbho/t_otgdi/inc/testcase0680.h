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

#ifndef TESTCASE0680_H
#define TESTCASE0680_H

	
//----------------------------------------------------------------------------------------------		
//! @SYMTestCaseID				PBASE-T_OTGDI-0680
//! @SYMTestCaseDesc			A-Device requests bus, VBUS is high, device is b_hnp_enabled, but not suspended
//! @SYMFssID 
//! @SYMPREQ					1305
//! @SYMREQ						8929
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			1.	A-Device raises VBus
//!                             2.	A-Device and B-Device wait to arrive in default roles (A-Host, B-Peripheral)
//!								3.	B-Peripheral waits to arrive in configured state, A-Device waits to become A-Peripheral
//!								4.	A-Host enumerates newly attached peripheral, but does not suspend it
//!								6.	B-Device makes a bus request
//!								7.	No HNP occurs, B-Device generates message to say HNP is not possible
//!								8.	A-Device drops VBus
//!								9.	A-Device and B-Device both observe VBus dropping.
//!                             
//! @SYMTestExpectedResults		There should be no role swap, and error message should be generated at step 7.
//! @SYMTestStatus              Defined	
//----------------------------------------------------------------------------------------------	

class CTestCase0680 : public CTestCaseB2BRoot
	{
public:
	static CTestCase0680* NewL(TBool aHost);
	virtual ~CTestCase0680(); 	
	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);
	
	void RunStepL();
	
	TInt GetStepIndex()	{ return(iCaseStep); };	
	
private:
	CTestCase0680(TBool aHost);
	void ConstructL();

	// DATA
private:		

	enum TCaseSteps
		{
		EPreconditions,
		ELoadLdd,			//	Load LDDs, trigger FDFActor
		EReadyToRaiseVBus,	//	Wait until VBus arrives
		EDefaultRoles, 		//	B-device as peripheral, A-device as host
		EBConfigured,		//	B-Device is configured
		EBErrorReceived,	//	B-device receives error message (refusal to role-swap because we're not suspended)
		EDropVBus,			//	Tidyup steps...
		EVBusDropped,
		EUnloadLdd,
		EUnloadClient,
		ELastStep
		};
	
	TCaseSteps iCaseStep;
	TInt       iStateRetry;	// swallow other events
	
	const static TTestCaseFactoryReceipt<CTestCase0680> iFactoryReceipt;
	};
	
	
	
#endif // TESTCASE0680_H
