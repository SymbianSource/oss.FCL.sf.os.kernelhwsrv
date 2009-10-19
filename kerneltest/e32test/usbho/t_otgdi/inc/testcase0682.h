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

#ifndef TESTCASE0682_H
#define TESTCASE0682_H

	
//----------------------------------------------------------------------------------------------		
//! @SYMTestCaseID				PBASE-T_OTGDI-0682
//! @SYMTestCaseDesc			B-Device SRP/HNP attempt, A-Device detects SRP, responds with BusRequest
//! @SYMFssID 
//! @SYMPREQ					1305
//! @SYMREQ						8929
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			1.	B-Device triggers SRP, A-Device waits to detect SRP
//!								2.	A-Device calls BusRequest
//!                             2.	A-Device and B-Device wait to arrive in default roles (A-Host, B-Peripheral)
//!								3.	B-Peripheral waits to arrive in configured state, A-Device waits to become A-Peripheral
//!								4.	A-Host enumerates newly attached peripheral, and suspends it
//!								5.	B-Peripheral waits to arrive in suspended state
//!								6.	B-Device makes a bus request (except 1st time around loop!), HNP occurs
//!								7.	A-Device and B-Device arrive in swapped roles (B-Host, A-Peripheral)
//!								8.	A-Peripheral waits to arrive in configured state, B-Device waits to become B-Peripheral
//!								9.	B-Host enumerates newly attached peripheral, and suspends it
//!								11.	A-Peripheral waits to arrive in suspended state
//!								12.	Role swap occurs automatically. Test repeats from step 2, 3 times.
//!								13.	A-Device drops VBus
//!								14.	A-Device and B-Device both observe VBus dropping.
//!								
//! @SYMTestExpectedResults 	VBus rises, followed by multiple role swaps, followed by VBus dropping (within 30 seconds)
//! @SYMTestStatus              Defined	
//----------------------------------------------------------------------------------------------	


	class CTestCase0682 : public CTestCaseB2BRoot
	{
public:
	static CTestCase0682* NewL(TBool aHost);
	virtual ~CTestCase0682(); 	
	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);
	
	void RunStepL();
	
	TInt GetStepIndex()	{ return(iCaseStep); };	
	
private:
	CTestCase0682(TBool aHost);
	void ConstructL();

	virtual void StepB2BPreconditions();

	// DATA
private:		

	enum TCaseSteps
		{
		EPreconditions,
		ELoadLdd,				//	Load LDDs, trigger FDFActor
		EPerformSrp,			//	B-device performs SRP, A-device prepares to detect it
		EAReceivedSrp,			//	A-device responds to SRP
		EDefaultRoles,   		//	B-device as peripheral, A-device as host
		EBConfigured,			//	B-Device is configured
		EBSuspended,			//	B-device is suspended
		ESwappedRoles,			//	B-device as peripheral, A-device as host
		EAConfigured,			//	A-Device is configured
		EASuspended,			//	A-device is suspended

		EDropVBus,				//	Tidyup steps
		EVBusDropped,
		EUnloadLdd,
		EUnloadClient,
		ELastStep
		};
	
	TCaseSteps iCaseStep;
	TInt       iStateRetry;	// swallow other events
	
	TInt		iHNPCounter;	//	We want to do a full HNP cycle (A-Host->A-Peripheral->A-Host) x 3
	TBool		iFirstRoleSwap;
	const static TTestCaseFactoryReceipt<CTestCase0682> iFactoryReceipt;
	};
	
#endif // TESTCASE0682_H
