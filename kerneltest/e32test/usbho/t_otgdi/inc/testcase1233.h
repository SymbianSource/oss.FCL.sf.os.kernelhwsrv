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

#ifndef TESTCASE1233_H
#define TESTCASE1233_H

	
//----------------------------------------------------------------------------------------------		
//! @SYMTestCaseID				PBASE-T_OTGDI-1233
//! @SYMTestCaseDesc			Exercise all VID/PID pairs for High-Speed Electrical Test
//! @SYMFssID 
//! @SYMPREQ					
//! @SYMREQ						
//! @SYMTestType				UT
//! @SYMTestPriority			1 
//! @SYMTestActions 			This test utilises the USB HOST/OTG stack which handles the 
//!								Host side of detection of a Device attachment where the Device  
//!								exposes itself as one of the USB High-Speed test device pairs of  
//!								VID/PID. 
//!								 
//!								The applicable VID/PID pairs are shown on the Host end by logging,  
//!								and the end result is confirmed by user interaction with the T_OTGDI  
//!								program. 
//!								 
//!								The program runs as a back-to-back H4 pair: 
//!								 
//!								t_otgdi /slave      is used on the 'A' connected default-Host side 
//!								t_otgdi /master  is used on the 'B' connected default-Peripheral side 
//!								 
//!								To activate this test, select the menu item that access test  
//!								PBASE-USB_T_OTGDI-1233 (at 24/10/2008 this was selector 26) 
//!								 
//!								The VID/PID pairs are as defined in the OTG Supplement (v1.3)  
//!								section 6.6.6.1 (table 6-5) and are: 
//!								 
//!								0x1A0A/0x0101 
//!								0x1A0A/0x0102 
//!								0x1A0A/0x0103 
//!								0x1A0A/0x0104 
//!								0x1A0A/0x0105 
//!								0x1A0A/0x0106 
//!								0x1A0A/0x0107 
//!								0x1A0A/0x0108 
//!								 
//!								NB: since this runs on an H4, which does not provide High-Speed support,  
//!								the linkage down to hardware is not implemented: instead, the test  
//!								relies on recording the appropriate test routing.
//!
//! @SYMTestExpectedResults 	
//! @SYMTestStatus              Defined	
//----------------------------------------------------------------------------------------------	


	class CTestCase1233 : public CTestCaseB2BRoot
	{
public:
	static CTestCase1233* NewL(TBool aHost);
	virtual ~CTestCase1233(); 	
	
	virtual void ExecuteTestCaseL();
	void DoCancel();
	static void CancelKB(CTestCaseRoot *pThis);
	
	void RunStepL();
	
	TInt GetStepIndex()	{ return(iCaseStep); };	
	
private:
	CTestCase1233(TBool aHost);
	void ConstructL();

	// DATA

private:		

	enum TCaseSteps
		{
		// Fixed steps
		EPreconditions,
		ELoadLdd,

		// Steps for this test case only
		ELoopToNextPID,
		
		ERaiseVBus,
		EVBusRaised,
		
		EDropVBus,
		EVBusDropped,
				
		// Fixed steps
		EUnloadLdd,
		ELastStep
		};
	
	TCaseSteps iCaseStep;

	TInt	   iTestVID;
	TInt	   iTestPID;

	const static TTestCaseFactoryReceipt<CTestCase1233> iFactoryReceipt;
	};
	
	
#endif // TESTCASE1233_H

