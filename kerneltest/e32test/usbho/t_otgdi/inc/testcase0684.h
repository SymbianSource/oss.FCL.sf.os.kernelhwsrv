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

#ifndef TESTCASE0684_H
#define TESTCASE0684_H

	
//----------------------------------------------------------------------------------------------		
//! @SYMTestCaseID				PBASE-T_OTGDI-0684
//! @SYMTestCaseDesc			'B' Device should accept all bMaxPower levels from 'A' device
//! @SYMFssID 
//! @SYMPREQ					1305
//! @SYMREQ						8931
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			
//!					1. Set the bMaxPower to zero for B device. Set the bMaxPower to Zero for A Device.
//!					2. Calling BusRequest() on A Device to Raise VBus
//!					3. Checking: A device is in A_Host state , 
//!								 B device is in B_Peripheral state, 
//!								 B device received B_HNP_ENABLE , 
//!				                 bus is in SUSPEND
//!					4. call BusRequest() on B device to request a role swap 
//!					5. Checking: B device temporarily get the host role, (since there are no Function driver load) 
//!								 A device received a SET_CONFIGURATION
//!					6. Checking: The end state of role swapping is: 
//!								 A in A_Peripheral and B in B_Host, 
//!								 B device received B_HNP_ENABLE, 
//!                    			 bus is in SUSPEND
//!					7. On A device, set the bMaxPower to 50 ma,
//!					8. On B device, Issue BusRequest()
//!					9. Checking: B device temporarily get the host role, 
//!								 A device received a SET_CONFIGURATION
//!
//! @SYMTestExpectedResults 	 B device should not complain about A device's power request is too high since now it is in B IdPin mode 
//!                              and never supply power to VBus.
//! @SYMTestStatus              Defined	
//----------------------------------------------------------------------------------------------	


	class CTestCase0684 : public CTestCaseB2BRoot
	{
public:
	static CTestCase0684* NewL(TBool aHost);
	virtual ~CTestCase0684(); 	
	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);
	
	void RunStepL();
	
	TInt GetStepIndex()	{ return(iCaseStep); };	
	
private:
	CTestCase0684(TBool aHost);
	void ConstructL();
	void RunStepLOLD();
	// DATA
private:		

	enum TCaseSteps
		{
		// Fixed steps
		EPreconditions,
		ELoadLdd,				//	Load LDDs, trigger FDFActor
		ESetMaxPower2Zero,		//  Set bMaxPower in the configure descriptor to 0  
		EReadyToRaiseVBus,		//	Wait until VBus arrives
		EDefaultRoles,   		//	B-device as peripheral, A-device as host (and link is "busy")
		
		// Steps for this test case only
		EAWaitForHNPEnabled,	//  A wait for HNPEnbaled 
		EAToPeripheral,			// 
		EAToHost,
		EBWaitForHNPEnabled,
		EBToHost,
		EBToPeripheral,
		EBConfigured,			//	B-Device is configured
		EBSuspended,			//	B-device is suspended
		EBWaitForTimeout,
		EAIdleHostPriorToAPeripheral,
		EAConfigured,
		
		// Fixed steps
		EIdleHostPriorToVBusDown,
		EDropVBus,				//	Tidyup steps
		EVBusDropped,
		EUnloadLdd,
		EUnloadClient,
		ELastStep
		};
	
	TCaseSteps iCaseStep;
	TInt       iStateRetry;	// swallow other events
	TInt	   iBusRequestCounter;
	TBool      iIsTimeToDrop;
	const static TTestCaseFactoryReceipt<CTestCase0684> iFactoryReceipt;
	};
	
	
#endif // TESTCASE0683_H

