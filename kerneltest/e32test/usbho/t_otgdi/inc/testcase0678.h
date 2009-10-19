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

#ifndef TESTCASE0678_H
#define TESTCASE0678_H

	
//----------------------------------------------------------------------------------------------		
//! @SYMTestCaseID				PBASE-T_OTGDI-0678
//! @SYMTestCaseDesc			B-Device Role-swap attempt. VBUS up, b_hnp_enable, bus suspended
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
//!								6.	B-Device makes a bus request, HNP occurs
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


	class CTestCase0678 : public CTestCaseB2BRoot
	{
public:
	static CTestCase0678* NewL(TBool aHost);
	virtual ~CTestCase0678(); 	
	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);
	
	void RunStepL();
	
	TInt GetStepIndex()	{ return(iCaseStep); };	
	
private:
	CTestCase0678(TBool aHost);
	void ConstructL();

	// DATA
private:		

	enum TCaseSteps
		{
		EPreconditions,
		ELoadLdd,				//	Load LDDs, trigger FDFActor
		EReadyToRaiseVBus,			//	Wait until VBus arrives
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
	
	const static TTestCaseFactoryReceipt<CTestCase0678> iFactoryReceipt;
	};
	
	
#endif // TESTCASE0678_H
