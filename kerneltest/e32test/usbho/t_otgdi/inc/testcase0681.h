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

#ifndef TESTCASE0681_H
#define TESTCASE0681_H

//----------------------------------------------------------------------------------------------		
//! @SYMTestCaseID				PBASE-T_OTGDI-0681
//! @SYMTestCaseDesc			B-Device SRP/HNP attempt, A-Device detects SRP, responds with BusRespondSrp
//! @SYMFssID 
//! @SYMPREQ					1305
//! @SYMREQ						8929
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			1.	B-Device triggers SRP, A-Device waits to detect SRP
//!								2.	A-Device calls BusRespondSrp
//!								3.	A-Device and B-Device arrive in swapped roles (B-Host, A-Peripheral)
//!								4.	A-Peripheral waits to arrive in configured state, B-Device waits to become B-Peripheral
//!								5.	B-Host enumerates newly attached peripheral, and suspends it
//!								6.	A-Peripheral waits to arrive in suspended state
//!								7.	Role swap occurs automatically
//!                             8.	A-Device and B-Device wait to arrive in default roles (A-Host, B-Peripheral)
//!								9.	B-Peripheral waits to arrive in configured state, A-Device waits to become A-Peripheral
//!								10.	A-Host enumerates newly attached peripheral, and suspends it
//!								11.	B-Peripheral waits to arrive in suspended state
//!								12.	B-Device makes a bus request, HNP occurs. Test repeats from step 3, 3 times.
//!								13.	A-Device drops VBus
//!								14.	A-Device and B-Device both observe VBus dropping.
//!								
//! @SYMTestExpectedResults 	SRP is detected and responded to. VBus rises, followed by multiple role swaps,
//!								followed by VBus dropping (all within 30 seconds)
//! @SYMTestStatus              Defined	
//----------------------------------------------------------------------------------------------	


	class CTestCase0681 : public CTestCaseB2BRoot
	{
public:
	static CTestCase0681* NewL(TBool aHost);
	virtual ~CTestCase0681(); 	
	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);
	
	void RunStepL();
	
	TInt GetStepIndex()	{ return(iCaseStep); };	
	
private:
	CTestCase0681(TBool aHost);
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
		ESwappedRoles,			//	B-device as peripheral, A-device as host
		EAConfigured,			//	A-Device is configured
		EASuspended,			//	A-device is suspended
		EDefaultRoles,   		//	B-device as peripheral, A-device as host
		EBConfigured,			//	B-Device is configured
		EBSuspended,			//	B-device is suspended

		EDropVBus,				//	Tidyup steps
		EVBusDropped,
		EUnloadLdd,
		EUnloadClient,
		ELastStep
		};
	
	TCaseSteps iCaseStep;
	TInt       iStateRetry;	// swallow other events
	
	TInt		iHNPCounter;	//	We want to do a full HNP cycle (A-Host->A-Peripheral->A-Host) x 3
	
	const static TTestCaseFactoryReceipt<CTestCase0681> iFactoryReceipt;
	};
	
	
	
#endif // TESTCASE0681_H
