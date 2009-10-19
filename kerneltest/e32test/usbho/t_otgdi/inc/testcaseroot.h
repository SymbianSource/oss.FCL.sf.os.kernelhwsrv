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

#ifndef TESTCASEROOT_H
#define TESTCASEROOT_H

#include <d32otgdi.h>	// OTGDI thunk headder (under test)
#include "debugmacros.h"
#include "testcasefactory.h"
#include "testpolicy.h"
#include "otgroot.h"		// otg driver facade
#include "b2bwatchers.h"	// otg watchers server


#define MINMAX_CLAMPVALUE(a,min,max) (a < min?a=min:(a>max?a=max:(int)0))

// keyboard input, different from KErrNone and KErrCancel
const TInt KTestCaseWatchdogTO		= 1025;
const TInt KTestCaseFailureEventReceived = 1026;	//	Meaning that some specific undesired event occurred during a test step

const TInt KB2BStepTimeoutDefaultMS 	= 5000;	// 5 seconds
const TInt KB2BStepTimeoutShortMS		= 1000; // timeout for things that should happen virtually immediately
const TInt KB2BStepRoleSustainB_MS		= 30000;	// B-device must stay in role for at least 30 seconds
const TInt KB2BStepRoleSustainA_MS		= 35000;	// A-device must stay in role longer before it drops the bus


// Prompts to the tester
					// H4 width    ****************************
_LIT(KAttachOETAsBDevice,		  "Attach OET, SW9 set as B-Device.\n");
_LIT(KInsertAConnectorPrompt,	  "Please insert 'A'-connector now.\n");
_LIT(KRemoveAConnectorPrompt,  	  "Please remove 'A'-connector now.\n");
_LIT(KInsertBCablePrompt,	   	  "Please insert 'B'-cable end now.\n ");
_LIT(KInsertACablePrompt,	  	  "Please insert 'A'-cable end now.\n ");
_LIT(KInsertAIntoPC,              "Please insert 'A'-connector into PC\n ");
_LIT(KRemoveAFromPC,              "Please remove 'A'-connector from PC\n ");
_LIT(KPressAnyKeyToStart,		  "Press any key to start...");
_LIT(KPressAnyKeyToContinue,	  "Press any key to proceed...");
_LIT(KPressAnyKeyToEnd,			  "Press any key to end.");
_LIT(KMsgErrorPreconditionFailed, "<Error>Test precondition not met!\n");
_LIT(KMsgBPlugNotFound,			  "B plug not detected ERROR!\n");
_LIT(KMsgAPlugNotFound,			  "A plug not detected ERROR!\n");
_LIT(KMsgWaitingForSRPTimeout,    "Waiting for SRP timeout (32 s)\n");
_LIT(KMsgWaitingForSRPInitiated,  "Waiting for SRP initiation\n");


_LIT(KTestTypeB2BMsg, "== B2B connected test ==\n");
_LIT(KRoleMasterMsg,  "== MASTER ==\n");
_LIT(KRoleSlaveMsg,   "== SLAVE  ==\n");

// CActive helper string
_LIT(KMsgAlreadyActive, "Is already Active");		

// name of the OTGDI ldd
_LIT(KOTGDeviceInterfaceDriverName,"otgdi");

// names for the USBC ldd
_LIT(KUsbcLddFileName, "eusbc");
_LIT(KUsbDeviceName, "Usbc");


const TInt KDelayDurationForUserActivityMS	= 15000;
const TInt KDelayDurationForQEmpty			= 200;	// > 17 ms
const TInt KDelayDurationForLocalTrigger	= 50;	// 50 ms (API call with a local side-effect)

const TInt KDelayBeforeBusDropUs	= 50000;	// 50ms time after VBus rise before we may drop it - prevents
											// ambiguous Vbus pulse signalling.

const TInt KTestProductID 		= 0x2670;	// Symbian USB PID product-id 

// Specification values
const TInt KSpec_TA_SRP_RSPNS = 4900; 		// ms [OTG section 5.3.2] - NOTE: A device timeout=32 seconds


const TInt KOperationRetriesMax = 3;	// # retries on various operations.
const TInt KParameterTextLenMax = 80;	// command-line param length

// map the number of times to re-run the OPEN/CLOSE tests to the command-line parameter
// this parameter is also used for many other tests that allow itteration count control
#define OPEN_REPEATS		gOpenIterations	
#define OPEN_MINREPEATS		1
#define OPEN_MAXREPEATS		1000

// # times to repeat the steps
#define OOMOPEN_REPEATS		gOOMIterations
#define OOM_MINREPEATS		1
#define OOM_MAXREPEATS		99

// panics
enum
	{
	EPanicAlreadyActive=1000,
	EPanicPropertyDeleted = 1001,
	EPanicWatchdogError = 1002
	};



/** Abstract base class to derive all tests from
*/
class CTestCaseRoot : public CActive, public COtgRoot
    {
public:
	virtual ~CTestCaseRoot();
		
	/** Provides us with a pointer to the test policy which instantiated this object
	 in OTGDI, the policy does not do any policy making, it merely instantiates the test
	 and provides parameters */
	void SetTestPolicy(CBasicTestPolicy* aTestPolicy);
	
	void DisplayTestCaseOptions();
	
	virtual void DescribePreconditions() = 0;
	virtual TInt GetStepIndex() = 0;

	// Performs the test case
	void PerformTestL();

	virtual void ExecuteTestCaseL()=0; // override to implement	
	
	// Provides to the caller the identity of this test case
	// @return descriptor referrence to the test case identity
	TDesC& TestCaseId();
	
	// Retrieve the test result for the executed test case
	// @return the test case result
	TInt TestResult() const;	

	TBool IsActiveOutstanding() {return(IsActive());}; 
                          

protected:
	/**
	Constructor
	@param aTestCaseId the identity that this test case is to represent
	@param aHostFlag the flag for host role
	*/
	CTestCaseRoot(const TDesC& aTestCaseId, TBool aHost);
	
	/**
	Base class 2nd phase constructor
	*/
	void BaseConstructL();	 

	/**
	Called when Cancel is called (if active)
	*/
	virtual void DoCancel();
	
	/**
	Called when the this AO has been scheduled
	*/
	virtual void RunL();
	
	/**
	This default implementation just informs the test controller that this specific test case RunL
	left with the error code so that the active scheduler will not Panic.  This indicates that the test case failed.
	Derived test cases are expected to override for exact knowledge of error
	@param aError the error from a derived test case execution
	@return KErrNone
	*/
	virtual TInt RunError(TInt aError);
	
	/**
	State that the test case has failed with the given error code
	@param aFailResult the failure result for the test case
	*/
	void TestFailed(TInt aFailResult, const TDesC &aErrorDescription);
	void TestFailed2(TInt aFailResult, const TDesC &aErrorDescription, TInt errorCode);
	
	void AssertionFailed(TInt aFailResult, const TDesC &aErrorDescription) {TestFailed(aFailResult, aErrorDescription);};
	void AssertionFailed2(TInt aFailResult, const TDesC &aErrorDescription, TInt errorCode) {TestFailed2(aFailResult, aErrorDescription, errorCode);}; 
	
	virtual void PrintStepName(const TDesC &aStepName);

	/**
	Instruct the test case that it has passed
	*/
	void TestPassed();                  

	void SelfComplete(TInt aError=KErrNone);
	
	CBasicTestPolicy& TestPolicy();

	// override this method in each test that needs KB input
	virtual void ProcessKey(TKeyCode &aKey);

	void ProcessEngineKey(TKeyCode &aKey);

	// Issue request
	void RequestCharacter();

	virtual void RunStepL() =0;
	virtual void PreRunStep();
	virtual void PostRunStep();


	                  
	// DATA MEMBERS
protected:
	/** bypass some manual steps, will skip preconditioning steps 
		when you want to run the tests in the correct sequence (IE previous test leaves 
		the external inputs in a state ready for this test).
	*/
	TBool iAutomated;	
   
	
	/**
	The timer resource for timeout of test actions
	and possibly other uses
	*/
	RTimer iTimer;	
	
	/**
	The execution result for the test case
	*/
	TInt iTestResult;

	
	// key input status to wait upon (wrap it in a method-call)
	TKeyCode iKeyCodeInput;		
	
	CConsoleBase* iConsole; // A console for reading from
	TBool iDualRoleCase;	// True for tests with 2 roles (Master/Slave)
	
private:	
	TBool iRequestedChar;

    TTime iStartTime;  // time that the test started at

    TBuf<KMaxName> iRootID; // test case name or step name

	/**
	*/
	CBasicTestPolicy* iTestPolicy;
	
	/**
	The identity of the test case
	*/
	TBuf<KMaxName> iTestCaseId;
    
    }; // CTestCaseRoot
        

/* ***************************************************************************
 * Abstract class for B2b test-cases adds B2B helpers and datas to the 
 * CTestCaseRoot class. 
 */
class CTestCaseB2BRoot : public CTestCaseRoot
	{
public:
	static CTestCaseB2BRoot* NewL(const TDesC& aTestCaseId, TBool aHost, TRequestStatus &aStatus);
	virtual ~CTestCaseB2BRoot(); 	
	
	virtual void DescribePreconditions();
	void 	CheckRoleConnections();

	virtual void StepB2BPreconditions();
	
	
protected:
	CTestCaseB2BRoot(const TDesC& aTestCaseId, TBool aHost, TRequestStatus &aStatus);
	void ConstructL();
	
	void PrintStepName(const TDesC &aStepName);
	// the default B2B action is to clear the expected notification Q at the start of step, 
	// and the received q as the step ends
	void PreRunStep(); // from CTestCaseRoot
	void PostRunStep(); // from CTestCaseRoot
	
protected:
	CNotifyCollector	iCollector;
	
	};


#endif // TESTCASEROOT_H
