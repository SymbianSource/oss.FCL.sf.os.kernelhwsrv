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

#ifndef TESTCASE0683_H
#define TESTCASE0683_H

	
//----------------------------------------------------------------------------------------------		
//! @SYMTestCaseID				PBASE-T_OTGDI-0683
//! @SYMTestCaseDesc			Role-swap attempt while monitoring Connection Idle notification.
//! @SYMFssID 
//! @SYMPREQ					1305
//! @SYMREQ						8931
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			1.	A-Device raises VBus
//!                             2.	A-Device and B-Device wait to arrive in default roles (A-Host, B-Peripheral). A-Host must be "busy"
//!								3.	B-Peripheral waits to arrive in configured state, A-Device waits to become "idle".
//!								4.	A-Host enumerates newly attached peripheral, and suspends it.
//!								5	A-Host becomes idle, then waits to arrive in A-Peripheral state and also become "busy"
//!								6.	B-Peripheral waits to arrive in suspended state
//!								7.	B-Device makes a bus request, HNP occurs
//!								8.	A-Device and B-Device arrive in swapped roles (B-Host, A-Peripheral). A-Peripheral must be "busy"
//!								9.	A-Peripheral waits to arrive in configured state, B-Device waits to become B-Peripheral
//!								10.	B-Host enumerates newly attached peripheral, and suspends it
//!								11.	A-Peripheral becomes configured, then waits to arrive in suspended state and also become "idle"
//!								12.	Role swap occurs automatically. Test repeats from step 2, 3 times.
//!								13	Upon B-Peripheral reaching suspended state for the third time, the B-Peipheral does not make a bus request.
//!								14	A-Host waits to become "idle"
//!								15.	A-Device drops VBus
//!								16.	A-Device and B-Device both observe VBus dropping.
//!								
//! @SYMTestExpectedResults 	VBus rises, followed by multiple role swaps, followed by VBus dropping (within 30 seconds)
//! @SYMTestStatus              Defined	
//----------------------------------------------------------------------------------------------	


	class CTestCase0683 : public CTestCaseB2BRoot
	{
public:
	static CTestCase0683* NewL(TBool aHost);
	virtual ~CTestCase0683(); 	
	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);
	
	void RunStepL();
	
	TInt GetStepIndex()	{ return(iCaseStep); };	
	
private:
	CTestCase0683(TBool aHost);
	void ConstructL();

	// DATA
private:		

	enum TCaseSteps
		{
		EPreconditions,
		ELoadLdd,				//	Load LDDs, trigger FDFActor
		EReadyToRaiseVBus,			//	Wait until VBus arrives
		EDefaultRoles,   		//	B-device as peripheral, A-device as host (and link is "busy")
		EBConfigured,			//	B-Device is configured
		EBSuspended,			//	B-device is suspended
		EAIdleHostPriorToAPeripheral,	//	A-device detects suspension as "idle" prior to becoming A-Peripheral
		ESwappedRoles,			//	B-device as peripheral, A-device as host
		EAConfigured,			//	A-Device is configured
		EASuspended,			//	A-device is suspended, also detects "idle"
		EAIdleHostPriorToVBusDown,	//	A-device detects suspension as "idle" prior to shutting down VBus
		EDropVBus,				//	Tidyup steps
		EVBusDropped,
		EUnloadLdd,
		EUnloadClient,
		ELastStep
		};
	
	TCaseSteps iCaseStep;
	TInt       iStateRetry;	// swallow other events
	
	TInt		iHNPCounter;	//	We want to do a full HNP cycle (A-Host->A-Peripheral->A-Host) x 3
	
	const static TTestCaseFactoryReceipt<CTestCase0683> iFactoryReceipt;
	};
	
	
#endif // TESTCASE0683_H
