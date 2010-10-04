// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/domainmgr/t_domain_monitor.cpp
// Overview:
// Domain manager transition monitoring feature implementation tests
//
// API Information:
// RDmDomain, RDmDomainManager CDmDomain, CDmDomainManager,CDmDomanKeepAlive
//
//  - Domain member deferral and acknowledgments tests
//  - Domain manager policy interface tests
//  - Domain member deferral requests Platsec capability checking tests
//
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32ldr_private.h>

#include <domainobserver.h>

#include "domainpolicytest.h"

#include "t_domain.h"


RTest test(_L(" T_DOMAIN_MONITOR "));


/**
   Domain member deferral requests PlatSec capability checking tests.
*/
_LIT(KSecuritySlavePath1, "t_dmslave_nocaps.exe");
_LIT(KSecuritySlavePath2, "t_dmslave_wdd.exe");
_LIT(KSecuritySlavePath3, "t_dmslave_protsrv.exe");

class CDmTestPlatSec : public CActive, public MDmTest
	{
public:
	CDmTestPlatSec(TPtrC aFileName);
	~CDmTestPlatSec()
		{
		Cancel();
		iManager.Close();
		}
	// from CActive
	void RunL();
	// from MDmTest
	void Perform();
	void Release();
	TInt TransitionNotification(MDmDomainMember&)
		{
		test(0);
		return KErrNone;
		}
	void TransitionRequestComplete()
		{}
private:
	// from CActive
	virtual void DoCancel()
		{
		test(0);
		}
private:
	RDmDomainManager iManager;
	const TPtrC iFileName;
	};


CDmTestPlatSec::CDmTestPlatSec(TPtrC aFileName)
	: CActive(CActive::EPriorityStandard), iFileName(aFileName)
	{}


void CDmTestPlatSec::Perform()
	{
	test.Next(_L("CDmTestPlatSec"));

	// 1. Set up test hierarchy/domain & join it
	TInt r = RDmDomainManager::AddDomainHierarchy(KDmHierarchyIdTestV2);
	test_KErrNone(r);
	r = iManager.Connect(KDmHierarchyIdTestV2);
	test_KErrNone(r);

	// 2. Create a child process
	RProcess proc;
	r = proc.Create(iFileName, KNullDesC);
	test_KErrNone(r);

	// Start process & wait until child has set up itself (3. & 4.)
	TRequestStatus status;
	proc.Rendezvous(status);
	proc.Resume();
	User::WaitForRequest(status);
	test_KErrNone(status.Int());

	// 5. Transition test domain to some other state (child: 6. & 7.)
	CActiveScheduler::Add(this);
	iManager.RequestDomainTransition(KDmIdTestA, EShutdownNonCritical, ETraverseDefault, iStatus);
	CActive::SetActive();
	CActiveScheduler::Start();

	// Child processes do: TC 3.1, 3.2, 3.3

	CLOSE_AND_WAIT(proc);
	}


void CDmTestPlatSec::RunL()
	{
	RDebug::Printf("CDmTestPlatSec::RunL(): %d", iStatus.Int());
	CActiveScheduler::Stop();
	}


void CDmTestPlatSec::Release()
	{
	delete this;
	}


/**
   Checks that upon transition acknowledgment, outstanding deferral is
   completed with KErrCompletion.
*/
class CDmDeferralTestCompletion : public CDmDeferralTest, public MDeferringMember
	{
public:
	CDmDeferralTestCompletion(TDmHierarchyId aId, TDmDomainState aState)
		: CDmDeferralTest(aId, aState)
		{}

	~CDmDeferralTestCompletion()
		{
		delete iKeepAlive;
		}

	void DoPerform()
		{
		test.Next(_L("CDmDeferralTestCompletion\n"));

		iMember = new CDmTestMember(iHierarchyId, KDmIdTestA, 0, this);
		test_NotNull(iMember);

		iKeepAlive = new CTestKeepAlive(iMember->iDomain);
		test_NotNull(iKeepAlive);

		iManager.RequestSystemTransition(iState, ETraverseChildrenFirst, CActive::iStatus);
		}

	void HandleEndOfDeferrals(TInt aError)
		{
		test.Printf(_L("End of deferrals\n"));

		// This is the test (TC 1.1.1.2.1):

		test_Equal(KErrCompletion, aError);
		}

	TInt TransitionNotification(MDmDomainMember& /*aDomainMember*/)
		{
		iKeepAlive->BeginDeferrals(this, 1);
		return KErrNone;
		}

	void TransitionRequestComplete()
		{
		}
private:
	CTestKeepAlive* iKeepAlive;
	};


/**
   Checks that after deferring a given number of times, the deferral
   after fails with KErrNotSupported.
*/
class CDmDeferralTestKErrNotSupported : public CDmDeferralTest, public MDeferringMember
	{
public:
	/**
	@param aCount Number of deferrals to attempt
	*/
	CDmDeferralTestKErrNotSupported(TDmHierarchyId aId, TDmDomainState aState, TInt aCount)
		: CDmDeferralTest(aId, aState), iCount(aCount)
		{}

	~CDmDeferralTestKErrNotSupported()
		{
		delete iKeepAlive;
		}

	// from CDmDeferralTest
	void DoPerform()
		{
		test.Next(_L("CDmDeferralTestKErrNotSupported\n"));
		test.Printf(_L("CDmDeferralTestKErrNotSupported: Hierachy %d, state %d, attempt %d deferrals\n"),
			iHierarchyId, iState, iCount);

		iMember = new CDmTestMember(iHierarchyId, KDmIdTestA, 0, this);
		test_NotNull(iMember);

		iKeepAlive = new CTestKeepAlive(iMember->iDomain);
		test_NotNull(iKeepAlive);

		iManager.RequestSystemTransition(iState, ETraverseChildrenFirst, CActive::iStatus);
		}

	// from MDeferringMember
	void HandleEndOfDeferrals(TInt aError)
		{
		iMember->Acknowledge();

		test.Printf(_L("CDmDeferralTestKErrNotSupported: End of deferrals %d\n"), aError);
		// This is the test (TC 1.1.1.3.1, TC 1.1.1.3.2, TC 1.1.1.3.3):
		test_Equal(KErrNotSupported, aError);
		}

	// from MDmTest
	TInt TransitionNotification(MDmDomainMember& /*aDomainMember*/)
		{
		iKeepAlive->BeginDeferrals(this, iCount);
		return KErrAbort; // Don't acknowledge yet
		}

	void TransitionRequestComplete()
		{
		RDebug::Printf("CDmDeferralTestKErrNotSupported::TransitionRequestComplete()");
		test_KErrNone(iStatus.Int());
		}
private:
	CTestKeepAlive* iKeepAlive;
	const TInt iCount;
	};

/**
   DeferAcknowledgement() with status KErrNone.

   1: Client receives notification, defers once and then acknowledges after the
   next notification
   2: Client receives notification, defers twice and then acknowledges after
   the next notification
   3: Client receives notification, defers once and then fails to acknowledge
*/
class CDmDeferralTestKErrNone : public CDmDeferralTest, public MDeferringMember
	{
public:
	CDmDeferralTestKErrNone(TDmHierarchyId aId, TDmDomainState aState,
							TInt aDeferrals, TBool aAcknowledge);
	~CDmDeferralTestKErrNone();
	// from CDmDeferralTest
	void DoPerform();
	// from MDmTest
	TInt TransitionNotification(MDmDomainMember& aDomainMember);
	void TransitionRequestComplete();
	// from MDeferringMember
	void HandleEndOfDeferrals(TInt aError);

private:
	CTestKeepAlive* iKeepAlive;
	const TInt iDeferrals;
	const TBool iAcknowledge;
	};


CDmDeferralTestKErrNone::CDmDeferralTestKErrNone(TDmHierarchyId aId,
												 TDmDomainState aState,
												 TInt aDeferrals,
												 TBool aAcknowledge)
	: CDmDeferralTest(aId, aState),
	  iDeferrals(aDeferrals),
	  iAcknowledge(aAcknowledge)
	{}


CDmDeferralTestKErrNone::~CDmDeferralTestKErrNone()
	{
	delete iKeepAlive;
	}


void CDmDeferralTestKErrNone::DoPerform()
	{
	test.Next(_L("CDmDeferralTestKErrNone"));

	iMember = new CDmTestMember(iHierarchyId, KDmIdTestCAA, 0, this);
	test_NotNull(iMember);

	iKeepAlive = new CTestKeepAlive(iMember->iDomain);
	test_NotNull(iKeepAlive);

	iManager.RequestSystemTransition(iState, ETraverseDefault, CActive::iStatus);
	}


void CDmDeferralTestKErrNone::HandleEndOfDeferrals(TInt aError)
	{
	test.Printf(_L("HandleEndOfDeferrals(): %d\n"), aError);

	// This is the test (TC 1.1.1.1.1, TC 1.1.1.1.2 , TC 1.1.1.1.3):

	test_Equal(KErrNone, aError);

	if (iAcknowledge)
		{
		RDebug::Printf(" Calling AcknowledgeLastState()");
		iMember->iDomain.AcknowledgeLastState();
		}
	}


TInt CDmDeferralTestKErrNone::TransitionNotification(MDmDomainMember&)
	{
	iKeepAlive->BeginDeferrals(this, iDeferrals);
	// don't acknowledge yet
	return KErrAbort;
	}


void CDmDeferralTestKErrNone::TransitionRequestComplete()
	{
	}


/**
   Test mix of deferral and non-deferral clients (1.3)
  
   1: Three clients receive notification.
   2: One makes three deferrals and then acknowledges after the next notification
   3: The other two non-deferral clients acknowledge without making a deferral      
*/
class CDmDeferralMixed : public CDmDeferralTest, public MDeferringMember
	{
public:
	CDmDeferralMixed(TDmHierarchyId aId, TDmDomainState aState, TInt aDeferrals, TBool aAcknowledge, TBool aDelayAck);
	~CDmDeferralMixed();
	// from CDmDeferralTest
	void DoPerform();
	// from MDmTest
	TInt TransitionNotification(MDmDomainMember& aDomainMember);
	void TransitionRequestComplete();
	// from MDeferringMember
	void HandleEndOfDeferrals(TInt aError);

private:
	CTestKeepAlive* iKeepAlive;	
	CDmTestMember* iMixedDeferralTestMember[2];
	const TInt iDeferrals;
	const TBool iAcknowledge;
	TBool iDoneDeferral;	
	const TBool iDelayAck;
	TBool iNonMemberAck[2];
	MDmDomainMember* iFirstMemberToCompleteAddr;
	};

CDmDeferralMixed::CDmDeferralMixed(TDmHierarchyId aId, 
								   TDmDomainState aState,
								   TInt aDeferrals,
								   TBool aAcknowledge,
								   TBool aDelayAck)	
    :CDmDeferralTest(aId, aState),
 	 iDeferrals(aDeferrals),
	 iAcknowledge(aAcknowledge),
	 iDelayAck(aDelayAck)
	{
	iDoneDeferral=EFalse; 	
	iFirstMemberToCompleteAddr = NULL;
	iNonMemberAck[0]=EFalse;
	iNonMemberAck[1]=EFalse;
	}

CDmDeferralMixed::~CDmDeferralMixed()
	{
	delete iKeepAlive;	
	delete iMixedDeferralTestMember[0];
	delete iMixedDeferralTestMember[1];	   
	}

void CDmDeferralMixed::DoPerform()
	{
	test.Next(_L("CDmDeferralMixed"));	
	
	// Attach three test members to the same domain (KDmIdTestCAA). One of the test
	// member is a deferring member while the other two are non deferring members.
	iMember = new CDmTestMember(iHierarchyId, KDmIdTestCAA, 0, this);
	test_NotNull(iMember);

	iMixedDeferralTestMember[0] = new CDmTestMember(iHierarchyId, KDmIdTestCAA, 0, this);
	test_NotNull(iMixedDeferralTestMember[0]);

	iMixedDeferralTestMember[1] = new CDmTestMember(iHierarchyId, KDmIdTestCAA, 0, this); 
	test_NotNull(iMixedDeferralTestMember[1]);

	iManager.RequestSystemTransition(iState, ETraverseDefault, CActive::iStatus);
	}

void CDmDeferralMixed::HandleEndOfDeferrals(TInt aError)
	{
	test.Printf(_L("HandleEndOfDeferrals(): %d\n"), aError);
	test_Equal(KErrNone, aError);
	if (iAcknowledge)
		{
		RDebug::Printf(" Calling AcknowledgeLastState()");
		//iFirstMemberToCompleteAddr is the first member to complete and is deferred 
		static_cast<CDmTestMember*>(iFirstMemberToCompleteAddr)->iDomain.AcknowledgeLastState();		
		iDoneDeferral = ETrue;
		}
	}

TInt CDmDeferralMixed::TransitionNotification(MDmDomainMember& aDomainMember)
	{
	RDebug::Printf("CDmDeferralMixed::TransitionNotification()");

	// if first member to complete, take note of this member and defer.
	if (iFirstMemberToCompleteAddr ==  NULL)
		{
		iFirstMemberToCompleteAddr =  &aDomainMember; // Get address of first member to complete		
		iKeepAlive = new CTestKeepAlive(static_cast<CDmTestMember*>(iFirstMemberToCompleteAddr)->iDomain);
		test_NotNull(iKeepAlive);		
		}

	if  ( (!iDoneDeferral) && (&aDomainMember == iFirstMemberToCompleteAddr) ) // Defer 
		{
		//TC 1.3.1 Define a client to defer 3 times and then acknowledge and include 2 non deferring clients 	
		test.Printf(_L("CDmDeferralMixed Attempting %d deferrals\n"), iDeferrals);
		iKeepAlive->BeginDeferrals(this, iDeferrals);		
		return KErrAbort;// don't acknowledge yet
		}
	else if (iNonMemberAck[0]== EFalse) 
		{//One of the two non deferring clients which acknowledges without any delay
		iNonMemberAck[0]= ETrue;
		test.Printf(_L("CDmDeferralMixed acknowledging iMixedDeferralTestMember - 1 \n"));	
		return KErrNone;  // Non deferral memeber acknowledges on time
		}
	else if (iNonMemberAck[1]== EFalse ) 
		{//One of the two non deferring clients which acknowledges with and without a delay based on the state of iDelayAck
		iNonMemberAck[1]= ETrue;
		if(!iDelayAck)
			{
			test.Printf(_L("CDmDeferralMixed acknowledging iMixedDeferralTestMember - 2 \n"));
			return KErrNone; // Non deferral member acknowledges on time
			}
		else
			{
			test.Printf(_L("CDmDeferralMixed delaying acknowledgement for iMixedDeferralTestMember - 2  \n"));
			//TC 1.3.2 - 1 fails to acknowledge in time
			return KErrAbort; // Delay acknowlegdement
			}			
		}
	test(EFalse);
	//default
	return KErrNone;
	}					

void CDmDeferralMixed::TransitionRequestComplete()
	{
	RDebug::Printf("CDmDeferralMixed::TransitionRequestComplete()");
	}

/////////////////////////////////////////////// CDmKeepAlive Test cases ///////////////////////////////////////////////

//-------------------------------------------------
// Domain member deferral and acknowledgments tests
//-------------------------------------------------

class MDomainMemberTests;
class CDomainMemberKeepAlive;
const TUint KMembersMax = 32;

// Policy related service functions user by the CDmDomainKeepAlive tests
SDmStateSpecV1 Get0DeferralState()
	{
	TUint i = 0;
	for (i=0; i<StateSpecificationSize; i++)
		{
		if(StateSpecification[i].iDeferralLimit != 0)
			continue;
		return StateSpecification[i];
		}

	// We could not find any state that has a 0 deferral specified in the policy
	test(0);
	return StateSpecification[i]; // get rid of compiler warnings
	}

// Get the first minimal deferral state from the policy greater than 0
SDmStateSpecV1 GetMinDeferralState() 
	{
	SDmStateSpecV1 maxState, minState;
	maxState = StateSpecification[0];
	minState = StateSpecification[0];

	for (TUint i=0; i<StateSpecificationSize; i++)
		{
		if(StateSpecification[i].iDeferralLimit != 0)
			{
			if(StateSpecification[i].iDeferralLimit > maxState.iDeferralLimit)
				maxState = StateSpecification[i];
			if(StateSpecification[i].iDeferralLimit < minState.iDeferralLimit)
				{
				minState = StateSpecification[i];
				}
			else if(minState.iDeferralLimit == 0)
				minState = StateSpecification[i];
			}
		continue;
		}

	test.Printf(_L("minState's TimeoutMs = %d , count = %d\n"), minState.iTimeoutMs, minState.iDeferralLimit );

	// Test whether there exists atleast one minimal deferral state that is greater than 0 deferrals and less than max deferrals
	test(minState.iDeferralLimit > 0 && minState.iDeferralLimit < maxState.iDeferralLimit);
	return minState;
	}

// Get the max deferral state from the policy. This is a simple function that always gets the first max deferral from the list
// The parameter aContinueOnError is used to get a state eith maximum deferral that has the policy error as
// ETransitionFailureContinue or ETransitionFailureStop
SDmStateSpecV1 GetMaxDeferralState(TBool aContinueOnError = EFalse) 
	{
	SDmStateSpecV1 aState;
	aState = StateSpecification[0];

	TUint i;
	for (i=0; i<StateSpecificationSize; i++)
		{
		const SDmStateSpecV1& spec = StateSpecification[i];

		if((spec.iDeferralLimit > aState.iDeferralLimit) &&
			(spec.iFailurePolicy == aContinueOnError ? ETransitionFailureContinue : ETransitionFailureStop))
			{
			aState = StateSpecification[i];
			}
		}

	// Check that a suitable state was found
	test(aState.iFailurePolicy == aContinueOnError ? ETransitionFailureContinue : ETransitionFailureStop);

	test.Printf(_L("Max deferrral state's TimeoutMs = %d , count = %d, failure policy %d\n"), aState.iTimeoutMs, aState.iDeferralLimit, aState.iFailurePolicy);
	return aState;
	}

// CDmDomainKeepAliveTest test
//-------------------------------------------------
// Domain member deferral and acknowledgments tests
//-------------------------------------------------
class CDmDomainKeepAliveTest : public CActive, public MDmTest
	{
public: 
	// from CActive
	void RunL();
 
	// from MDmTest
	void Perform();
	void Release();
	TInt TransitionNotification(MDmDomainMember& aDomainMember);
	void TransitionRequestComplete() {};

	// for the individual tests to handle
	void HandleTransitionL(CDomainMemberKeepAlive* aDmMember);
	TInt HandleDeferralError(TInt aError);

	CDmDomainKeepAliveTest() : CActive(CActive::EPriorityStandard) {}
	void SetExpectedValues(TInt aTestNotificationsExpected, TInt aTransitionsExpected);
	void ValidateTestResults();
	void RequestSystemTransition(TDmDomainState aTestState, TDmTraverseDirection aDirection);
	void DoAsynMemberAck();
	void CancelTransition();
protected:
	// from CActive
	virtual void DoCancel();

private:
	void Init(MDomainMemberTests* aTest);
	void UnInit();

private:
	CDomainMemberKeepAlive*	iTestMembers[KMembersMax]; 
	RDmDomainManager		iTestDomainManager;
	
	MDomainMemberTests*			iCurrentTest;

public:
	static TUint		gTestMemberCount;
	static TUint		gLeafMemberCount;
	TInt				iTestNotifications;
	TInt				iTestNotificationsExpected;

	TInt				iTransitionsCompleted;
	TInt				iTransitionsExpected;
	};

// CDomainMemberKeepAlive
class CDomainMemberKeepAlive : public CDmDomainKeepAlive, public MDmDomainMember
	{
public:
	static CDomainMemberKeepAlive* NewL(TDmHierarchyId aHierarchy, TDmDomainId aId, TUint32 aOrdinal, MDomainMemberTests*);
	~CDomainMemberKeepAlive();

	// from CDmDomainKeepAlive
	TInt HandleDeferralError(TInt aError);
	void HandleTransitionL();

	// from MDmDomainMember
	inline TDmHierarchyId HierarchyId() {return iHierarchy;};
	inline TDmDomainId	DomainId() {return iId;};
	inline TDmDomainState State() {return iState;};
	inline TInt Status() {return iStatus.Int();};
	inline TUint32 Ordinal() {return iOrdinal;};
	inline TInt Notifications() {return iNotifications;};
	static TInt TimerCallback(TAny* obj);
	void DoAsynHandleTransition(const TTimeIntervalMicroSeconds32 aInterval);
private:
	CDomainMemberKeepAlive(TDmHierarchyId aHierarchy, TDmDomainId aId, TUint32 aOrdinal, MDomainMemberTests*);

public:
	// used only for test cases TC 1.1.2.2.2, TC 1.1.2.3.1 and TC 1.1.2.3.2 where DoAsynMemberAck is used
	TBool				iShouldAck; 
	
private:
	TDmHierarchyId		iHierarchy;
	TDmDomainId			iId;
	TDmDomainState		iState;
	TUint32				iOrdinal;
	MDomainMemberTests*	iTest;
	TUint				iNotifications;

	CPeriodic*			iTimer;
	};

class MDomainMemberTests
	{
public:
	virtual void Perform() = 0;
	virtual void Release() = 0;
	virtual void HandleTransitionL(CDomainMemberKeepAlive* aDmMember) = 0;
	virtual TInt HandleDeferralError(TInt aError) = 0;
	virtual void DoAsynHandleTransition(CDomainMemberKeepAlive*) {};
	virtual void DoAsynMemberAck(CDomainMemberKeepAlive*) {};
public:
	TDmDomainState		iTestState;
	};

CDomainMemberKeepAlive* CDomainMemberKeepAlive::NewL(TDmHierarchyId aHierarchy, TDmDomainId aId, TUint32 aOrdinal, MDomainMemberTests* aTest)
	{
	CDomainMemberKeepAlive* self=new (ELeave) CDomainMemberKeepAlive(aHierarchy, aId, aOrdinal, aTest);
	CleanupStack::PushL(self);
	self->ConstructL();

	self->RequestTransitionNotification();

	CleanupStack::Pop();
	return self;
	}

CDomainMemberKeepAlive::~CDomainMemberKeepAlive()
	{
	delete iTimer;
	Cancel();
	}

CDomainMemberKeepAlive::CDomainMemberKeepAlive(TDmHierarchyId aHierarchy, TDmDomainId aId, TUint32 aOrdinal, MDomainMemberTests* aTest):
	CDmDomainKeepAlive(aHierarchy, aId), iShouldAck(EFalse),
	iHierarchy(aHierarchy), iId(aId), iOrdinal(aOrdinal), iTest(aTest)
	{
	}

TInt CDomainMemberKeepAlive::HandleDeferralError(TInt aError)
	{
	TInt r = KErrNone;

	TBuf16<4> buf;
	GetDomainDesc(Ordinal(), buf);
	test.Printf(_L("HandleDeferralError domain = %S, error = %d\n"), &buf, aError);

	r = iTest->HandleDeferralError(aError);
	return r;
	}

void CDomainMemberKeepAlive::HandleTransitionL()
	{
	iShouldAck = ETrue;
	iNotifications++;
	iTest->HandleTransitionL(this);
	}

TInt CDomainMemberKeepAlive::TimerCallback(TAny* obj)
	{
    CDomainMemberKeepAlive* member = static_cast<CDomainMemberKeepAlive*>(obj);

	TBuf16<4> buf;
	GetDomainDesc(member->Ordinal(), buf);

	test.Printf(_L("Member asynchronous transition handler, domain = %S\n"), &buf);

	member->iTest->DoAsynHandleTransition(member);
	member->iTimer->Cancel();
	return KErrNone;
	}

void CDomainMemberKeepAlive::DoAsynHandleTransition(const TTimeIntervalMicroSeconds32 aInterval)
	{
	iTimer = CPeriodic::NewL(CActive::EPriorityHigh);

	TCallBack callback(TimerCallback, this);
	iTimer->Start(aInterval, aInterval, callback);
	}

////////////////////////////////////////////////////////////////////////////////////
//                      TC 1.1.2.1.1 (with deferral count 0)                      //
////////////////////////////////////////////////////////////////////////////////////
class TestTransitionWithDeferral0 : public MDomainMemberTests // TC 1.1.2.1.1 (with deferral count 0)
	{
public:
	TestTransitionWithDeferral0(CDmDomainKeepAliveTest& aTester) : iTester(aTester) {}
	// from MDmKeepAliveTest
	void Perform();
	void Release();
	void HandleTransitionL(CDomainMemberKeepAlive* aDmMember);
	TInt HandleDeferralError(TInt aError);

private:
	CDmDomainKeepAliveTest& iTester;
	};

void TestTransitionWithDeferral0::Perform()
	{
	test.Printf(_L("****************TestTransitionWithDeferral0****************\n"));
	test.Next(_L("Test state transition that has 0 deferral"));
	test.Printf(_L("Acknowleding immediately......\n"));

	iTester.SetExpectedValues(iTester.gTestMemberCount, 1);

	SDmStateSpecV1 aStateSpec = Get0DeferralState();

	iTestState =  aStateSpec.iState;

	// request a system transition
	iTester.RequestSystemTransition(iTestState, ETraverseChildrenFirst);
	test.Printf(_L("Requested system transition\n"));

	// wait for test transitions to complete
	CActiveScheduler::Start();
	test.Printf(_L("....transition completed\n"));

	iTester.ValidateTestResults();
	}

void TestTransitionWithDeferral0::Release()
	{
	delete this;
	}

void TestTransitionWithDeferral0::HandleTransitionL(CDomainMemberKeepAlive* aDmMember)
	{
	TInt ackError = iTester.TransitionNotification(*aDmMember);
	aDmMember->GetState();

	aDmMember->AcknowledgeLastState(ackError);
	}

TInt TestTransitionWithDeferral0::HandleDeferralError(TInt aError)
	{
	// Since this test case expects 0 deferral, the KErrNotSupported will happen which is expected
	test(aError == KErrNotSupported);
	return KErrNone;
	}

////////////////////////////////////////////////////////////////////////////////////
//         TC 1.1.2.1.1 (with max deferral count - ack after n deferrals          //
////////////////////////////////////////////////////////////////////////////////////
class TestAckWithinDeferral : public MDomainMemberTests
	{
public:
	TestAckWithinDeferral(CDmDomainKeepAliveTest& aTester) : iTester(aTester) {}
	// from MDmKeepAliveTest
	void Perform();
	void Release();
	void HandleTransitionL(CDomainMemberKeepAlive* aDmMember);
	TInt HandleDeferralError(TInt aError);
	void DoAsynHandleTransition(CDomainMemberKeepAlive* aDmMember);
private:
	CDmDomainKeepAliveTest& iTester;
	TUint iTransitionTime;
	};

void TestAckWithinDeferral::Perform()
	{
	test.Printf(_L("****************TestAckWithinDeferral****************\n"));
	test.Next(_L("Test state transition that has deferral > 0"));
	test.Printf(_L("Acknowleding immediately......\n"));

	iTester.SetExpectedValues(iTester.gTestMemberCount, 1);

	SDmStateSpecV1 aStateSpec = GetMaxDeferralState();

	iTestState =  aStateSpec.iState;
	iTransitionTime = aStateSpec.iTimeoutMs * aStateSpec.iDeferralLimit / 2;

	// request a system transition
	iTester.RequestSystemTransition(iTestState, ETraverseChildrenFirst);
	test.Printf(_L("Requested system transition\n"));

	// wait for test transitions to complete
	CActiveScheduler::Start();
	test.Printf(_L(".......transition completed\n"));

	iTester.ValidateTestResults();
	}

void TestAckWithinDeferral::Release()
	{
	delete this;
	}

void TestAckWithinDeferral::HandleTransitionL(CDomainMemberKeepAlive* aDmMember)
	{
	iTester.TransitionNotification(*aDmMember);

	aDmMember->GetState();

	const TTimeIntervalMicroSeconds32 KInterval = iTransitionTime * 1000; // policy defines in millisec - convert it to microsec

	aDmMember->DoAsynHandleTransition(KInterval);
	}

TInt TestAckWithinDeferral::HandleDeferralError(TInt aError)
	{
	// Since this test case expects acknowledging within the deferral, the KErrCompletion will happen which is expected
	test_Equal(KErrCompletion, aError);
	return KErrNone;
	}

/* By now atleast 3 deferrals should have got finished */
void TestAckWithinDeferral::DoAsynHandleTransition(CDomainMemberKeepAlive* aDmMember)
	{
	aDmMember->AcknowledgeLastState(KErrNone);
	}

////////////////////////////////////////////////////////////////////////////////////
//                 TC 1.1.2.2.1 (But still ongoing with other domain)             //
////////////////////////////////////////////////////////////////////////////////////
class TestAckAfterDomainDeferralExpiry : public MDomainMemberTests
	{
public:
	TestAckAfterDomainDeferralExpiry(CDmDomainKeepAliveTest& aTester) : iTester(aTester) {}
	// from MDmKeepAliveTest
	void Perform();
	void Release();
	void HandleTransitionL(CDomainMemberKeepAlive* aDmMember);
	TInt HandleDeferralError(TInt aError);
	void DoAsynHandleTransition(CDomainMemberKeepAlive* aDmMember);
private:
	CDmDomainKeepAliveTest& iTester;
	TUint iTransitionTime;
	};

void TestAckAfterDomainDeferralExpiry::Perform()
	{
	test.Printf(_L("****************TestAckAfterDomainDeferralExpiry****************\n"));
	test.Next(_L("Test client that does not acknowledge within the allowed number of deferrals "));
	test.Printf(_L("but which then acknowledges while transition still ongoing (in other domain)\n"));

	iTester.SetExpectedValues(iTester.gTestMemberCount, 1);

	SDmStateSpecV1 aStateSpec = GetMaxDeferralState(ETrue);

	iTestState =  aStateSpec.iState;
	iTransitionTime = aStateSpec.iTimeoutMs * aStateSpec.iDeferralLimit * 2;

	// request a system transition
	iTester.RequestSystemTransition(iTestState, ETraverseChildrenFirst);
	test.Printf(_L("Requested system transition\n"));

	// wait for test transitions to complete
	CActiveScheduler::Start();
	test.Printf(_L(".......transition completed\n"));

	iTester.ValidateTestResults();
	}

void TestAckAfterDomainDeferralExpiry::Release()
	{
	delete this;
	}

void TestAckAfterDomainDeferralExpiry::HandleTransitionL(CDomainMemberKeepAlive* aDmMember)
	{
	iTester.TransitionNotification(*aDmMember);

	aDmMember->GetState();

	const TTimeIntervalMicroSeconds32 KInterval = iTransitionTime * 1000; // policy defines in millisec - convert it to microsec

	aDmMember->DoAsynHandleTransition(KInterval);
	}

TInt TestAckAfterDomainDeferralExpiry::HandleDeferralError(TInt aError)
	{
	// We expect the KErrNotSupported happens after expiring the deferral counts
	test(aError == KErrNotSupported);
	return KErrNone;
	}

/* By the time this function is called the server would have transitioned this member
   under the domain and would have moved on to the next domain */
void TestAckAfterDomainDeferralExpiry::DoAsynHandleTransition(CDomainMemberKeepAlive* aDmMember)
	{
	aDmMember->AcknowledgeLastState(KErrNone);
	}

////////////////////////////////////////////////////////////////////////////////////
//                                   TC 1.1.2.2.2                                 //
////////////////////////////////////////////////////////////////////////////////////
class TestAckAfterTransitionCompletes : public MDomainMemberTests
	{
public:
	TestAckAfterTransitionCompletes(CDmDomainKeepAliveTest& aTester) : iTester(aTester) {}
	// from MDmKeepAliveTest
	void Perform();
	void Release();
	void HandleTransitionL(CDomainMemberKeepAlive* aDmMember);
	TInt HandleDeferralError(TInt aError);
	void DoAsynMemberAck(CDomainMemberKeepAlive* aDmMember);
private:
	CDmDomainKeepAliveTest& iTester;
	TInt iExpectedErrorCode;
	};

void TestAckAfterTransitionCompletes::Perform()
	{
	test.Printf(_L("****************TestAckAfterTransitionCompletes****************\n"));

	test.Next(_L("Test client that does not acknowledge within the allowed number of deferrals "));
	test.Printf(_L("but which then acknowledges while transition has completed\n"));

	iTester.SetExpectedValues(iTester.gTestMemberCount, 1);

	SDmStateSpecV1 aStateSpec = GetMaxDeferralState(ETrue);

	iTestState =  aStateSpec.iState;

	// request a system transition
	iTester.RequestSystemTransition(iTestState, ETraverseChildrenFirst);
	test.Printf(_L("Requested system transition\n"));

	// wait for test transitions to complete
	CActiveScheduler::Start();
	test.Printf(_L(".......transition completed\n"));

	iTester.ValidateTestResults();

	iTester.DoAsynMemberAck();
	}

void TestAckAfterTransitionCompletes::Release()
	{
	delete this;
	}

void TestAckAfterTransitionCompletes::HandleTransitionL(CDomainMemberKeepAlive* aDmMember)
	{
	iTester.TransitionNotification(*aDmMember);
	aDmMember->GetState();
	}

void TestAckAfterTransitionCompletes::DoAsynMemberAck(CDomainMemberKeepAlive* aDmMember)
	{
	aDmMember->AcknowledgeLastState(KErrNone);
	}

TInt TestAckAfterTransitionCompletes::HandleDeferralError(TInt aError)
	{
	test(aError == KErrNotSupported);
	return KErrNone;
	}

////////////////////////////////////////////////////////////////////////////////////
//					TC 1.1.2.2.3	and    TC 1.1.2.2.4						      //
////////////////////////////////////////////////////////////////////////////////////
class TestAckPrevTransAfterNewTransStart : public MDomainMemberTests
	{
public:
	TestAckPrevTransAfterNewTransStart(CDmDomainKeepAliveTest& aTester, TBool aAckPrevTran) :
				iShouldAck(EFalse), iAckPrevTran(aAckPrevTran), iTester(aTester) {}
	// from MDmKeepAliveTest
	void Perform();
	void Release();
	void HandleTransitionL(CDomainMemberKeepAlive* aDmMember);
	TInt HandleDeferralError(TInt aError);

private:
	TBool iShouldAck;
	TBool iAckPrevTran;
	CDmDomainKeepAliveTest& iTester;
	};

void TestAckPrevTransAfterNewTransStart::Perform()
	{
	if(iAckPrevTran)
		{
		test.Printf(_L("**************** TestAckPrevTransAfterNewTransStart (Ack Previous Transition after new transition started) ****************\n"));
		test.Next(_L("Test client that does not acknowledge within the allowed number of deferrals "));
		test.Printf(_L("but which then acknowledges while next new transition has started\n"));
		}
	else
		{
		test.Printf(_L("**************** TestAckPrevTransAfterNewTransStart (Never Ack Previous Transition) ****************\n"));
		test.Next(_L("Test client that does not acknowledge within the allowed number of deferrals "));
		test.Printf(_L("but which never acknowledges and continues handling next transition\n"));
		}

	iTester.SetExpectedValues(iTester.gTestMemberCount, 1);

	SDmStateSpecV1 aStateSpec = GetMaxDeferralState(ETrue);

	iTestState =  aStateSpec.iState;

	// request a system transition
	iTester.RequestSystemTransition(iTestState, ETraverseChildrenFirst);
	test.Printf(_L("Requested system transition\n"));

	// wait for test transitions to complete
	CActiveScheduler::Start();
	test.Printf(_L("....transition completed\n"));

	iTester.ValidateTestResults();

	if(iAckPrevTran)
		test.Printf(_L("Now request another transition for which the domain members should ack both transitions)\n"));
	else
		test.Printf(_L("Now request another transition for which the domain members should ack only the last transitions)\n"));

	iTester.SetExpectedValues(iTester.gTestMemberCount*2, 2);
	aStateSpec = GetMinDeferralState();
	iTestState =  aStateSpec.iState;

	iShouldAck = ETrue;
	// request a system transition
	iTester.RequestSystemTransition(iTestState, ETraverseChildrenFirst);
	test.Printf(_L("Requested system transition\n"));

	// wait for test transitions to complete
	CActiveScheduler::Start();
	test.Printf(_L("....transition completed\n"));

	iTester.ValidateTestResults();
	}

void TestAckPrevTransAfterNewTransStart::Release()
	{
	delete this;
	}

void TestAckPrevTransAfterNewTransStart::HandleTransitionL(CDomainMemberKeepAlive* aDmMember)
	{
	TInt ackError = iTester.TransitionNotification(*aDmMember);
	if(iShouldAck == EFalse)
		{
		aDmMember->GetState();
		// request another notification (even if we didn't acknowledge the last one)
		aDmMember->RequestTransitionNotification();
		test.Printf(_L("....Return without acknowledging\n"));
		return;
		}

	if(iAckPrevTran)
		{
		test.Printf(_L("....Acknowledge the first transition\n"));
		/* First ack the previous notification */
		aDmMember->AcknowledgeLastState(ackError);
		}
	else
		{
		test.Printf(_L("Skipping to acknowledge the first transition...\n"));
		/* We are not going to ack the previous notification handled in the above if condition 
	   Intentionally not acking and continuing to do a GetState to handle the next notification */
		}

	test.Printf(_L("Acknowledge the second transition\n"));
	/* Now handle the current notification */
	aDmMember->GetState();
	aDmMember->AcknowledgeLastState(ackError);
	}

TInt TestAckPrevTransAfterNewTransStart::HandleDeferralError(TInt aError)
	{
	if(!iShouldAck)
		test(aError == KErrNotSupported);
	else
		test_Equal(KErrCompletion, aError);

	return KErrNone;
	}

////////////////////////////////////////////////////////////////////////////////////
//                      TC 1.1.2.3.1    and    TC 1.1.2.3.2                       //
////////////////////////////////////////////////////////////////////////////////////
class TestCancelTransitonWithMemberAck : public MDomainMemberTests
	{
public:
	TestCancelTransitonWithMemberAck(CDmDomainKeepAliveTest& aTester, TInt aErrorCode) : 
			iTester(aTester), iCancelCount(0), iErrorCode(aErrorCode) {}
	// from MDmKeepAliveTest
	void Perform();
	void Release();
	void HandleTransitionL(CDomainMemberKeepAlive* aDmMember);
	TInt HandleDeferralError(TInt aError);
	void DoAsynMemberAck(CDomainMemberKeepAlive* aDmMember);
	static TInt CancelTransitionTimerCallback(TAny* obj);
	static TInt DelayTimerCallback(TAny* obj);
	void StopScheduler();
private:
	CDmDomainKeepAliveTest& iTester;
	TUint iTransitionTime;
	TUint iCancelCount;
	TInt  iErrorCode;
	CPeriodic *iCancelTransitionTimer;
	CPeriodic *iDelayTimer;
	};

TInt TestCancelTransitonWithMemberAck::CancelTransitionTimerCallback(TAny* obj)
	{
    TestCancelTransitonWithMemberAck* thisTest = static_cast<TestCancelTransitonWithMemberAck*>(obj);

	thisTest->iTester.CancelTransition();
	thisTest->iCancelTransitionTimer->Cancel();
	return KErrNone;
	}

TInt TestCancelTransitonWithMemberAck::DelayTimerCallback(TAny* obj)
	{
    TestCancelTransitonWithMemberAck* thisTest = static_cast<TestCancelTransitonWithMemberAck*>(obj);

	thisTest->iDelayTimer->Cancel();
	thisTest->StopScheduler();
	return KErrNone;
	}

void TestCancelTransitonWithMemberAck::StopScheduler()
	{
	CActiveScheduler::Stop();
	}

void TestCancelTransitonWithMemberAck::Perform()
	{
	test.Printf(_L("****************TestCancelTransitonWithMemberAck****************\n"));
	test.Next(_L("Test state transition cancelation...."));
	test.Printf(_L("that acknowledes KErrNone......\n"));

	iTester.SetExpectedValues(iTester.gLeafMemberCount, 1);

	SDmStateSpecV1 aStateSpec = GetMaxDeferralState();

	iTestState =  aStateSpec.iState;
	iTransitionTime = aStateSpec.iTimeoutMs * aStateSpec.iDeferralLimit / 2;

	// request a system transition
	iTester.RequestSystemTransition(iTestState, ETraverseChildrenFirst);
	test.Printf(_L("Requested system transition\n"));

	iCancelTransitionTimer = CPeriodic::NewL(CActive::EPriorityHigh);

	TCallBack cancelCb(CancelTransitionTimerCallback, this);
	iCancelTransitionTimer->Start(aStateSpec.iTimeoutMs, aStateSpec.iTimeoutMs, cancelCb);

	// wait for test transitions to complete
	CActiveScheduler::Start();
	test.Printf(_L("....transition cancelled\n"));

	iDelayTimer = CPeriodic::NewL(CActive::EPriorityStandard);

	TCallBack delayCb(DelayTimerCallback, this);
	TUint delayTime = iTransitionTime * 3;
	iDelayTimer->Start(delayTime, delayTime, delayCb);

	CActiveScheduler::Start();
	test.Printf(_L("........expected members got cancelation notified\n"));

	iTester.DoAsynMemberAck();

	iTester.ValidateTestResults();
	}

void TestCancelTransitonWithMemberAck::Release()
	{
	delete iDelayTimer;
	delete iCancelTransitionTimer;
	delete this;
	}

void TestCancelTransitonWithMemberAck::HandleTransitionL(CDomainMemberKeepAlive* aDmMember)
	{
	iTester.TransitionNotification(*aDmMember);

	aDmMember->GetState();
	}

TInt TestCancelTransitonWithMemberAck::HandleDeferralError(TInt aError)
	{
	// KErrNotReady is possible if the cancellation
	// ocurred in between member deferrrals
	test(aError == KErrCancel || aError == KErrNotReady);
	test(++iCancelCount <= iTester.gLeafMemberCount);
	return KErrNone;
	}

void TestCancelTransitonWithMemberAck::DoAsynMemberAck(CDomainMemberKeepAlive* aDmMember)
	{
	aDmMember->AcknowledgeLastState(iErrorCode);
	}

////////////////////////////////////////////////////////////////////////////////////
//                                TC 1.1.2.3.3                                    //
////////////////////////////////////////////////////////////////////////////////////
class TestCancelTransitonNeverAck : public MDomainMemberTests // TC1.1.2.3.3
	{
public:
	TestCancelTransitonNeverAck(CDmDomainKeepAliveTest& aTester) : iTester(aTester), iCancelCount(0), iShouldAck(EFalse){}
	// from MDmKeepAliveTest
	void Perform();
	void Release();
	void HandleTransitionL(CDomainMemberKeepAlive* aDmMember);
	TInt HandleDeferralError(TInt aError);
	static TInt CancelTransitionTimerCallback(TAny* obj);
	static TInt DelayTimerCallback(TAny* obj);
	void StopScheduler();
private:
	CDmDomainKeepAliveTest& iTester;
	TUint iTransitionTime;
	TUint iCancelCount;
	TBool iShouldAck;
	CPeriodic *iCancelTransitionTimer;
	CPeriodic *iDelayTimer;
	};

TInt TestCancelTransitonNeverAck::CancelTransitionTimerCallback(TAny* obj)
	{
    TestCancelTransitonNeverAck* thisTest = static_cast<TestCancelTransitonNeverAck*>(obj);

	thisTest->iTester.CancelTransition();
	thisTest->iCancelTransitionTimer->Cancel();
	return KErrNone;
	}

TInt TestCancelTransitonNeverAck::DelayTimerCallback(TAny* obj)
	{
    TestCancelTransitonNeverAck* thisTest = static_cast<TestCancelTransitonNeverAck*>(obj);

	thisTest->iDelayTimer->Cancel();
	thisTest->StopScheduler();
	return KErrNone;
	}

void TestCancelTransitonNeverAck::StopScheduler()
	{
	CActiveScheduler::Stop();
	}

void TestCancelTransitonNeverAck::Perform()
	{
	test.Printf(_L("****************TestCancelTransitonNeverAck****************\n"));
	test.Next(_L("Test state transition cancelation...."));
	test.Printf(_L("that never acknowledes ......\n"));

	iTester.SetExpectedValues(iTester.gLeafMemberCount, 1);

	SDmStateSpecV1 aStateSpec = GetMaxDeferralState();

	iTestState =  aStateSpec.iState;
	iTransitionTime = aStateSpec.iTimeoutMs * aStateSpec.iDeferralLimit / 2;

	// request a system transition
	iTester.RequestSystemTransition(iTestState, ETraverseChildrenFirst);
	test.Printf(_L("Requested system transition\n"));

	iCancelTransitionTimer = CPeriodic::NewL(CActive::EPriorityHigh);

	TCallBack cancelCb(CancelTransitionTimerCallback, this);
	iCancelTransitionTimer->Start(iTransitionTime, iTransitionTime, cancelCb);

	// wait for test transitions to complete
	CActiveScheduler::Start();
	test.Printf(_L("....transition cancelled\n"));

	iDelayTimer = CPeriodic::NewL(CActive::EPriorityStandard);

	TCallBack delayCb(DelayTimerCallback, this);
	TUint delayTime = iTransitionTime * 3;
	iDelayTimer->Start(delayTime, delayTime, delayCb);

	CActiveScheduler::Start();
	test.Printf(_L("........expected members got cancelation notified\n"));

	iTester.ValidateTestResults();

	test.Printf(_L("Now request another transition for which the domain members should ack only the last transitions)\n"));

	iTester.SetExpectedValues(iTester.gTestMemberCount + iTester.gLeafMemberCount, 2);
	aStateSpec = GetMinDeferralState();

	iTestState =  aStateSpec.iState;

	iShouldAck = ETrue;
	// request a system transition
	iTester.RequestSystemTransition(iTestState, ETraverseChildrenFirst);
	test.Printf(_L("Requested system transition\n"));

	// wait for test transitions to complete
	CActiveScheduler::Start();
	test.Printf(_L(".......transition completed\n"));

	iTester.ValidateTestResults();
	}

void TestCancelTransitonNeverAck::Release()
	{
	delete iDelayTimer;
	delete iCancelTransitionTimer;
	delete this;
	}

void TestCancelTransitonNeverAck::HandleTransitionL(CDomainMemberKeepAlive* aDmMember)
	{
	TInt ackError = iTester.TransitionNotification(*aDmMember);

	if(iShouldAck == EFalse)
		{
		aDmMember->GetState();

		// request another notification (even if we didn't acknowledge the last one)
		aDmMember->RequestTransitionNotification();
		test.Printf(_L("Return without acknowledging...\n"));
		return;
		}

	test.Printf(_L("Skipping to acknowledge the first transition...\n"));
	/* We are not going to ack the previous notification handled in the above if condition 
	   Intentionally not acking and continuing to do a GetState to handle the next notification */

	test.Printf(_L("Acknowledge the second transition...\n"));
	/* Now handle the current notification */
	aDmMember->GetState();
	aDmMember->AcknowledgeLastState(ackError);
	}

TInt TestCancelTransitonNeverAck::HandleDeferralError(TInt aError)
	{
	if(!iShouldAck)
		{
		// KErrNotReady is possible if the cancellation
		// ocurred in between member deferrrals
		test(aError == KErrCancel || aError == KErrNotReady);
		}
	else
		test_Equal(KErrCompletion, aError);

	if(aError == KErrCancel || (aError == KErrNotReady && !iShouldAck))
		test(++iCancelCount <= iTester.gLeafMemberCount);

	return KErrNone;
	}

////////////////////////////////////////////////////////////////////////////////////
//                           CDmDomainKeepAliveTest                               //
////////////////////////////////////////////////////////////////////////////////////
void CDmDomainKeepAliveTest::Init(MDomainMemberTests* aTest)
	{
	TInt r = RDmDomainManager::AddDomainHierarchy(KDmHierarchyIdTestV2);
	test_KErrNone(r);

	r = iTestDomainManager.Connect(KDmHierarchyIdTestV2);
	test_KErrNone(r);

	iTransitionsCompleted = 0;
	iTestNotifications = 0;
	gTestMemberCount = 0;
	gLeafMemberCount = 0;

	// Add some test hierarchy members
	test_TRAP(iTestMembers[gTestMemberCount++] = CDomainMemberKeepAlive::NewL(KDmHierarchyIdTestV2, KDmIdRoot, ORDINAL_FROM_DOMAINID0(KDmIdRoot), aTest));

	test_TRAP(iTestMembers[gTestMemberCount++] = CDomainMemberKeepAlive::NewL(KDmHierarchyIdTestV2, KDmIdRoot, ORDINAL_FROM_DOMAINID0(KDmIdRoot), aTest));
	
	// row 1
	test_TRAP(iTestMembers[gTestMemberCount++] = CDomainMemberKeepAlive::NewL(KDmHierarchyIdTestV2, KDmIdTestA, ORDINAL_FROM_DOMAINID1(KDmIdRoot, KDmIdTestA), aTest));

	test_TRAP(iTestMembers[gTestMemberCount++] = CDomainMemberKeepAlive::NewL(KDmHierarchyIdTestV2, KDmIdTestB, ORDINAL_FROM_DOMAINID1(KDmIdRoot, KDmIdTestB), aTest));

	test_TRAP(iTestMembers[gTestMemberCount++] = CDomainMemberKeepAlive::NewL(KDmHierarchyIdTestV2, KDmIdTestC, ORDINAL_FROM_DOMAINID1(KDmIdRoot, KDmIdTestC), aTest));
	
	// row2
	test_TRAP(iTestMembers[gTestMemberCount++] = CDomainMemberKeepAlive::NewL(KDmHierarchyIdTestV2, KDmIdTestAA, ORDINAL_FROM_DOMAINID2(KDmIdRoot, KDmIdTestA, KDmIdTestAA), aTest));
	gLeafMemberCount++;

	test_TRAP(iTestMembers[gTestMemberCount++] = CDomainMemberKeepAlive::NewL(KDmHierarchyIdTestV2, KDmIdTestAB, ORDINAL_FROM_DOMAINID2(KDmIdRoot, KDmIdTestA, KDmIdTestAB), aTest));

	test_TRAP(iTestMembers[gTestMemberCount++] = CDomainMemberKeepAlive::NewL(KDmHierarchyIdTestV2, KDmIdTestBA, ORDINAL_FROM_DOMAINID2(KDmIdRoot, KDmIdTestB, KDmIdTestBA), aTest));
	gLeafMemberCount++;

	test_TRAP(iTestMembers[gTestMemberCount++] = CDomainMemberKeepAlive::NewL(KDmHierarchyIdTestV2, KDmIdTestCA, ORDINAL_FROM_DOMAINID2(KDmIdRoot, KDmIdTestC, KDmIdTestCA), aTest));
	
	// row 3
	test_TRAP(iTestMembers[gTestMemberCount++] = CDomainMemberKeepAlive::NewL(KDmHierarchyIdTestV2, KDmIdTestABA, ORDINAL_FROM_DOMAINID3(KDmIdRoot, KDmIdTestA, KDmIdTestAB, KDmIdTestABA), aTest));
	gLeafMemberCount++;

	test_TRAP(iTestMembers[gTestMemberCount++] = CDomainMemberKeepAlive::NewL(KDmHierarchyIdTestV2, KDmIdTestABB, ORDINAL_FROM_DOMAINID3(KDmIdRoot, KDmIdTestA, KDmIdTestAB, KDmIdTestABB), aTest));
	gLeafMemberCount++;

	test_TRAP(iTestMembers[gTestMemberCount++] = CDomainMemberKeepAlive::NewL(KDmHierarchyIdTestV2, KDmIdTestCAA, ORDINAL_FROM_DOMAINID3(KDmIdRoot, KDmIdTestC, KDmIdTestCA, KDmIdTestCAA), aTest));
	gLeafMemberCount++;

	}

void CDmDomainKeepAliveTest::SetExpectedValues(TInt aTestNotificationsExpected, TInt aTransitionsExpected)
	{
	iTestNotificationsExpected = aTestNotificationsExpected;
	iTransitionsExpected = aTransitionsExpected;
	}

void CDmDomainKeepAliveTest::ValidateTestResults()
	{
	test.Printf(_L("ValidateResults().....\n"));

	test.Printf(_L("iTestNotifications = %d iTestNotificationsExpected = %d\n"), iTestNotifications ,
				iTestNotificationsExpected);
	test(iTestNotifications == iTestNotificationsExpected);
	}

void CDmDomainKeepAliveTest::UnInit()
	{
	iTestDomainManager.Close();

	// cleanup
	
	for (TUint i = 0; i<gTestMemberCount; i++)
		delete iTestMembers[i];
	}

void CDmDomainKeepAliveTest::RequestSystemTransition(TDmDomainState aTestState, TDmTraverseDirection aDirection) 
	{
	iTestDomainManager.RequestSystemTransition(aTestState, aDirection, CActive::iStatus);
	CActive::SetActive();
	}

void CDmDomainKeepAliveTest::Perform()
	{
 	__UHEAP_MARK;

	CActiveScheduler::Add(this);

	MDomainMemberTests* tests[] = 
		{
			new TestTransitionWithDeferral0(*this), // TC 1.1.2.1.1 (with deferral count 0)
			new TestAckWithinDeferral(*this), // TC 1.1.2.1.1 (with max deferral count)
			new TestAckAfterDomainDeferralExpiry(*this), // TC 1.1.2.2.1 (But still ongoing with other domain)
			new TestAckAfterTransitionCompletes(*this), // TC 1.1.2.2.2
			new TestAckPrevTransAfterNewTransStart(*this, ETrue), // TC 1.1.2.2.3
			new TestAckPrevTransAfterNewTransStart(*this, EFalse), // TC 1.1.2.2.4
			new TestCancelTransitonWithMemberAck(*this, KErrNone), // TC1.1.2.3.1
			new TestCancelTransitonWithMemberAck(*this, KErrCompletion), // TC1.1.2.3.2
			new TestCancelTransitonNeverAck(*this), // TC1.1.2.3.3
		};

	for (unsigned int i = 0; i < sizeof(tests)/sizeof(*tests); ++i)
		{
		test(tests[i] != NULL);
		Init(tests[i]);
		iCurrentTest = tests[i];
		tests[i]->Perform();
		tests[i]->Release();
		UnInit();
		}
	__UHEAP_MARKEND;
	}

void CDmDomainKeepAliveTest::DoAsynMemberAck()
	{
	for (TUint i = 0; i<gTestMemberCount; i++)
		{
		if(iTestMembers[i]->iShouldAck)
			{
			TBuf16<4> buf;
			GetDomainDesc(iTestMembers[i]->Ordinal(), buf);
			test.Printf(_L("Request current test to ack %S.......\n"), &buf);
			iCurrentTest->DoAsynMemberAck(iTestMembers[i]);
			iTestMembers[i]->iShouldAck = EFalse;
			}
		}
	}

void CDmDomainKeepAliveTest::CancelTransition()
	{
	iTestDomainManager.CancelTransition();
	}

	// This handles a transition notification from a test domain member.
TInt CDmDomainKeepAliveTest::TransitionNotification(MDmDomainMember& aDomainMember)
	{
	TInt status = aDomainMember.Status();

	iTestNotifications++;

	test (aDomainMember.HierarchyId() == KDmHierarchyIdTestV2);

	TBuf16<4> buf;
	GetDomainDesc(aDomainMember.Ordinal(), buf);

	test.Printf(_L("TransitionNotification Hierarchy = %d, domain = %S, iOrdinal = 0x%08X, state = 0x%x, status = %d\n"), 
		aDomainMember.HierarchyId(), &buf, aDomainMember.Ordinal(), aDomainMember.State(), status);

	return KErrNone;
	}

void CDmDomainKeepAliveTest::RunL()
	{
	iTransitionsCompleted++;

	TInt error = iStatus.Int();

	test.Printf(_L("CDmDomainKeepAliveTest::RunL() error = %d, iTransitionsCompleted %d iTransitionsExpected %d\n"), 
		error, iTransitionsCompleted, iTransitionsExpected);

	if (iTransitionsCompleted == iTransitionsExpected)
		CActiveScheduler::Stop();
	}

void CDmDomainKeepAliveTest::DoCancel()
	{
	test(0);
	}

void CDmDomainKeepAliveTest::Release()
	{
	delete this;
	}


TUint CDmDomainKeepAliveTest::gTestMemberCount = 0;
TUint CDmDomainKeepAliveTest::gLeafMemberCount = 0;


/**
   DeferAcknowledgement() with status KErrServerBusy.

   Client receives notification, defers once and then defers again before the
   next notification.
*/
class ActiveMember : public CActive
	{
public:
	ActiveMember(CDmTestMember* aMember)
		: CActive(CActive::EPriorityHigh), iMember(aMember)
		{
		CActiveScheduler::Add(this);
		}
	~ActiveMember()
		{
		Cancel();
		}
	void Defer()
		{
		iMember->iDomain.DeferAcknowledgement(iStatus);
		SetActive();
		}
	void DoCancel()
		{
		iMember->iDomain.CancelDeferral();
		}
protected:
	CDmTestMember* const iMember;
	};


class TestServerBusy : public ActiveMember
	{
public:
	TestServerBusy(CDmTestMember* aMember);
	~TestServerBusy();
	void PrimeTimer();
private:
	void RunL();
private:
	RTimer iTimer;
	TBool iDeferred;
	const TInt iInstance;
	static TInt iInstances;
	};

TInt TestServerBusy::iInstances = 0;


TestServerBusy::TestServerBusy(CDmTestMember* aMember)
	: ActiveMember(aMember), iDeferred(EFalse), iInstance(++iInstances)
	{
	const TInt r = iTimer.CreateLocal();
	test_KErrNone(r);
	}


TestServerBusy::~TestServerBusy()
	{
	Cancel();
	iTimer.Close();
	iInstances--;
	}


void TestServerBusy::PrimeTimer()
	{
	// let the timers fire at different times (first one in 1ms, second one in
	// 50ms)
	const TTimeIntervalMicroSeconds32 t = (iInstance == 1) ? 1000 : 50000;
	iTimer.After(iStatus, t);
	SetActive();
	}


void TestServerBusy::RunL()
	{
	RDebug::Printf("TestServerBusy(#%d)::RunL(): %d", iInstance, iStatus.Int());
	if (!iDeferred)
		{
		iDeferred = ETrue;
		Defer();
		}
	else if (iInstance == 2)
		{
		// This is the test (TC 1.1.1.7.1):
		test_Equal(KErrServerBusy, iStatus.Int());
		}
	else if (iInstance == 1)
		{
		// Acknowledge at last
		test_KErrNone(iStatus.Int());
		iMember->Acknowledge();
		}
	else
		test(0);
	}


class CDmDeferralTestKErrServerBusy : public CDmDeferralTest
	{
public:
	CDmDeferralTestKErrServerBusy(TDmHierarchyId aId, TDmDomainState aState);
	~CDmDeferralTestKErrServerBusy();
	// from CDmDeferralTest
	void DoPerform();
	// from MDmTest
	TInt TransitionNotification(MDmDomainMember& aDomainMember);
	void TransitionRequestComplete();
private:
	TestServerBusy* iDeferral1;
	TestServerBusy* iDeferral2;
	};


CDmDeferralTestKErrServerBusy::CDmDeferralTestKErrServerBusy(TDmHierarchyId aId,
															 TDmDomainState aState)
	: CDmDeferralTest(aId, aState)
	{
	}


CDmDeferralTestKErrServerBusy::~CDmDeferralTestKErrServerBusy()
	{
	delete iDeferral1;
	delete iDeferral2;
	}


void CDmDeferralTestKErrServerBusy::DoPerform()
	{
	test.Next(_L("CDmDeferralTestKErrServerBusy"));

	iMember = new CDmTestMember(iHierarchyId, KDmIdTestCAA, 0, this);
	test_NotNull(iMember);

	iDeferral1 = new TestServerBusy(iMember);
	test_NotNull(iDeferral1);
	iDeferral2 = new TestServerBusy(iMember);
	test_NotNull(iDeferral2);

	iManager.RequestSystemTransition(iState, ETraverseDefault, CActive::iStatus);
	}


TInt CDmDeferralTestKErrServerBusy::TransitionNotification(MDmDomainMember&)
	{
	RDebug::Printf("CDmDeferralTestKErrServerBusy::TransitionNotification()");

	iDeferral1->PrimeTimer();

	iDeferral2->PrimeTimer();

	// don't acknowledge yet
	return KErrAbort;
	}


void CDmDeferralTestKErrServerBusy::TransitionRequestComplete()
	{
	RDebug::Printf("CDmDeferralTestKErrServerBusy::TransitionRequestComplete()");
	}


/**
   DeferAcknowledgement() with status KErrCancel.

   1. Client receives notification, defers once and then cancels before the next
   notification.
   2. Client receives notification, cancels deferral without one outstanding.
*/
class TestCancel : public ActiveMember
	{
public:
	TestCancel(CDmTestMember* aMember)
		: ActiveMember(aMember)
		{
		}
	void RunL()
		{
		RDebug::Printf("TestCancel::RunL(): %d", iStatus.Int());

		// This is the test (TC 1.1.1.4.1):
		test_Equal(KErrCancel, iStatus.Int());

		// Acknowledge at last
		iMember->Acknowledge();
		}
	};


class CDmDeferralTestKErrCancel : public CDmDeferralTest
	{
public:
	CDmDeferralTestKErrCancel(TDmHierarchyId aId, TDmDomainState aState, TBool aDeferFirst);
	~CDmDeferralTestKErrCancel();
	// from CDmDeferralTest
	void DoPerform();
	// from MDmTest
	TInt TransitionNotification(MDmDomainMember& aDomainMember);
	void TransitionRequestComplete();
private:
	TestCancel* iCancel;
	const TBool iDeferFirst;
	};


CDmDeferralTestKErrCancel::CDmDeferralTestKErrCancel(TDmHierarchyId aId,
													 TDmDomainState aState,
													 TBool aDeferFirst)
	: CDmDeferralTest(aId, aState), iDeferFirst(aDeferFirst)
	{
	}


CDmDeferralTestKErrCancel::~CDmDeferralTestKErrCancel()
	{
	delete iCancel;
	}


void CDmDeferralTestKErrCancel::DoPerform()
	{
	test.Next(_L("CDmDeferralTestKErrCancel"));

	iMember = new CDmTestMember(iHierarchyId, KDmIdTestABA, 0, this);
	test_NotNull(iMember);

	iCancel = new TestCancel(iMember);
	test_NotNull(iCancel);

	iManager.RequestSystemTransition(iState, ETraverseDefault, CActive::iStatus);
	}


TInt CDmDeferralTestKErrCancel::TransitionNotification(MDmDomainMember&)
	{
	RDebug::Printf("CDmDeferralTestKErrCancel::TransitionNotification()");

	if (iDeferFirst)
		{
		// Test case 1: First ask for a deferral...
		iCancel->Defer();
		}

	// Test cases 1 & 2: Cancel deferral request.

	// RDmDomainSession::CancelDeferral() checks if
	// RSessionBase::SendReceive(EDmStateCancelDeferral) returned KErrNone;
	// if not it will panic the client. (TC 1.1.1.4.2)

	iMember->iDomain.CancelDeferral();

	if (iDeferFirst)
		{
		// don't acknowledge yet (RunL() will)
		return KErrAbort;
		}
	else
		{
		return KErrNone;
		}
	}


void CDmDeferralTestKErrCancel::TransitionRequestComplete()
	{
	RDebug::Printf("CDmDeferralTestKErrCancel::TransitionRequestComplete()");
	}


/**
   DeferAcknowledgement() with status KErrNotReady.

   1. Client defers before a transition notification.
   2. Client receives notification, defers once and then defers again after the
   next notification.
*/
class TestNotReady : public ActiveMember
	{
public:
	TestNotReady(CDmTestMember* aMember)
		: ActiveMember(aMember)
		{
		}
	void RunL()
		{
		RDebug::Printf("TestNotReady::RunL(): %d", iStatus.Int());

		// This is the test (TC 1.1.1.5.1):
		test_Equal(KErrNotReady, iStatus.Int());

		CActiveScheduler::Stop();
		}
	};


class CDmDeferralTestKErrNotReady : public CDmDeferralTest
	{
public:
	CDmDeferralTestKErrNotReady(TDmHierarchyId aId, TDmDomainState aState);
	~CDmDeferralTestKErrNotReady();
	// from CDmDeferralTest
	void DoPerform();
	// from MDmTest
	TInt TransitionNotification(MDmDomainMember& aDomainMember);
	void TransitionRequestComplete();
private:
	TestNotReady* iNotReady;
	};


CDmDeferralTestKErrNotReady::CDmDeferralTestKErrNotReady(TDmHierarchyId aId,
														 TDmDomainState aState)
	: CDmDeferralTest(aId, aState)
	{
	}


CDmDeferralTestKErrNotReady::~CDmDeferralTestKErrNotReady()
	{
	delete iNotReady;
	}


void CDmDeferralTestKErrNotReady::DoPerform()
	{
	test.Next(_L("CDmDeferralTestKErrNotReady"));

	iMember = new CDmTestMember(iHierarchyId, KDmIdTestABA, 0, this);
	test_NotNull(iMember);

	iNotReady = new TestNotReady(iMember);
	test_NotNull(iNotReady);

	iNotReady->Defer();
	CActiveScheduler::Start();

	iManager.RequestSystemTransition(iState, ETraverseDefault, CActive::iStatus);
	}


TInt CDmDeferralTestKErrNotReady::TransitionNotification(MDmDomainMember&)
	{
	RDebug::Printf("CDmDeferralTestKErrNotReady::TransitionNotification()");

	// don't acknowledge yet
	return KErrAbort;
	}


void CDmDeferralTestKErrNotReady::TransitionRequestComplete()
	{
	RDebug::Printf("CDmDeferralTestKErrNotReady::TransitionRequestComplete()");

	TRequestStatus status;
	iMember->iDomain.DeferAcknowledgement(status);
	User::WaitForRequest(status);

	RDebug::Printf("Deferral status: %d", status.Int());

	// This is the test (TC 1.1.1.5.2):
	test_Equal(KErrNotReady, status.Int());

	}


/**
   Policy interface tests - negative tests.

   1. Ordinals return null or error.
   2. Structure returned contains invalid values.
*/
class CDmPolicyInterfaceTest : public MDmTest
	{
public:
	void Perform();
	void Release();
	TInt TransitionNotification(MDmDomainMember&)
		{
		return KErrNone;
		}
	void TransitionRequestComplete()
		{}
	};


void CDmPolicyInterfaceTest::Perform()
	{
	test.Next(_L("CDmPolicyInterfaceTest"));

	// In domainpolicy95.dll ordinal 4 (DmPolicy::GetStateSpec) returns an
	// error. This will lead to the failure of the following call, which will
	// also execute the destructors of classes CDmHierarchy and
	// CHierarchySettings.
	TInt r = RDmDomainManager::AddDomainHierarchy(KDmHierarchyIdTestV2_95);

	// This is the test (TC 1.4.2.1):
	test_Equal(KDmErrBadDomainSpec, r);

	// domainpolicy94.dll contains garbage values in the SDmStateSpecV1 struct.
	r = RDmDomainManager::AddDomainHierarchy(KDmHierarchyIdTestV2_94);

	// This is the test (TC 1.4.2.2):
	test_Equal(KDmErrBadDomainSpec, r);
	}


void CDmPolicyInterfaceTest::Release()
	{
	delete this;
	}


/////////////////////// Failure Policy Tests //////////////////////////////////
// 2.4 [M] Domain Controller DC5 (different failure policies for different 
//         system state transitions)
//    * TC 2.4.1 Create V2 policy where some states are "stop" and some are 
//      "continue" on failure, get member(s) to fail 
//

class CDmFailurePolicyTest : public CActive, public MDmTest
	{
public:
	CDmFailurePolicyTest(TDmDomainState aState, TDmTransitionFailurePolicy aPolicy);
	~CDmFailurePolicyTest();
		
	// from CActive
	void RunL();
	
	// from MDmTest
	void Perform();
	void Release();
	TInt TransitionNotification(MDmDomainMember&);
	void TransitionRequestComplete()
		{}
		
private:
	// from CActive
	virtual void DoCancel()
		{ test(0); }
		
private:
	RDmDomainManager iManager;
	CDmTestMember*	 iMembers[2];
	
	TDmDomainState				iDmState;
	TDmTransitionFailurePolicy	iFailPolicy;
	};


CDmFailurePolicyTest::CDmFailurePolicyTest(TDmDomainState aState, TDmTransitionFailurePolicy aPolicy)
	: CActive(CActive::EPriorityStandard), iDmState(aState), iFailPolicy(aPolicy)
	{
	iMembers[0] = iMembers[1] = 0;
	}
	
CDmFailurePolicyTest::~CDmFailurePolicyTest()
	{
	Cancel();
	for (int i = 0; i < 2; i++)
		delete iMembers[i], iMembers[i]= 0;
	iManager.Close();
	}

void CDmFailurePolicyTest::Perform()
	{
	test.Next(_L("CDmFailurePolicyTest"));
	
	RDebug::Printf("CDmFailurePolicyTest::Perform: iFailPolicy(%d)", iFailPolicy);

	// 1. Set up test hierarchy/domain & join it
	TInt r = RDmDomainManager::AddDomainHierarchy(KDmHierarchyIdTestV2);
	test_KErrNone(r);
	r = iManager.Connect(KDmHierarchyIdTestV2);
	test_KErrNone(r);

	// 2. Create the two members needed for this test. First does not ackn.
	iMembers[0] = new CDmTestMember(KDmHierarchyIdTestV2, KDmIdTestB, (iFailPolicy<<8)+0, this);
	test_NotNull(iMembers[0]);
	iMembers[1] = new CDmTestMember(KDmHierarchyIdTestV2, KDmIdTestBA, (iFailPolicy<<8)+1, this);
	test_NotNull(iMembers[1]);

	// 3. Initiate state transition from iInitState to iDmState
	CActiveScheduler::Add(this);
	iManager.RequestDomainTransition(KDmIdTestB, iDmState, ETraverseParentsFirst, iStatus);
	CActive::SetActive();
	CActiveScheduler::Start();

	// Close iManager when this object is destroyed in destructor
	}

TInt CDmFailurePolicyTest::TransitionNotification(MDmDomainMember& aMember)
	{
	RDebug::Printf("CDmFailurePolicyTest::TransitionNotification: aMember(%d)", aMember.Ordinal());
	
    if ((aMember.Ordinal() & 0xff ) == 0)		// Member in domain B
	{
		if (((aMember.Ordinal() & 0xff00) >> 8) == 0) // Stop policy
		{
			return KErrAbort; // Dont let this member in domain B ackn.
		}
		else // Continue policy
		{
			return KErrCommsParity; // Ackn with bizarre error
		}	
    }
    
	// Should only reach here for Members of sub-domain BA
	return KErrNone;
	}


void CDmFailurePolicyTest::RunL()
	{
	RDebug::Printf("CDmFailurePolicyTest::RunL: istatus(%d)", iStatus.Int());
	
	// Handle Transition completion code here. Should be a time out.
	
	// Based on failure policy check to see if the second member was transitioned
	// (continue) or whether it was not (stop). Since ETraverseParentsFirst is used
	// in the transition iMember[0] in domain B should fail and the iMember [1]
	// in domain BA may or may not then be transitioned....
	
	test_Equal(1, iMembers[0]->Notifications());
	
	if (iFailPolicy == ETransitionFailureContinue)
		{
		test_Equal(KErrCommsParity, iStatus.Int());		
		test_Equal(1, iMembers[1]->Notifications()); // Proves it did continue with transition
		}
	else if (iFailPolicy == ETransitionFailureStop)
		{
		test_Equal(KErrTimedOut, iStatus.Int());
		test_Equal(0, iMembers[1]->Notifications()); // Proves it did stop transition
		}
	else
		{
		test(0); // Panic default case
		}	
		
	test.Printf(_L("Test passed - failure policy (%d)\n"), iFailPolicy);
	
	CActiveScheduler::Stop();
	}


void CDmFailurePolicyTest::Release()
	{
	RDebug::Printf("CDmFailurePolicyTest::Release");
	delete this;
	}

/****************************** CDmDomainKeepAlive Functional coverage test ******************************/
// CDmKeepAliveFuncCov
class CDmKeepAliveFuncCov : public CDmDomainKeepAlive
	{
public:
	enum TMemKeepAliveCovTests { ECovHandleError, ECovDoCancel };

	static CDmKeepAliveFuncCov* NewL(TDmHierarchyId aHierarchy, TDmDomainId aId);
	~CDmKeepAliveFuncCov();

	// from CDmDomainKeepAlive
	void HandleTransitionL();
private:
	CDmKeepAliveFuncCov(TDmHierarchyId aHierarchy, TDmDomainId aId);

public:
	TMemKeepAliveCovTests iDmMemCov;
	};

CDmKeepAliveFuncCov* CDmKeepAliveFuncCov::NewL(TDmHierarchyId aHierarchy, TDmDomainId aId)
	{
	CDmKeepAliveFuncCov* self=new (ELeave) CDmKeepAliveFuncCov(aHierarchy, aId);
	CleanupStack::PushL(self);
	self->ConstructL();

	self->RequestTransitionNotification();

	CleanupStack::Pop();
	return self;
	}

CDmKeepAliveFuncCov::~CDmKeepAliveFuncCov()
	{
	Cancel();
	}

CDmKeepAliveFuncCov::CDmKeepAliveFuncCov(TDmHierarchyId aHierarchy, TDmDomainId aId):
	CDmDomainKeepAlive(aHierarchy, aId)
	{
	}

void CDmKeepAliveFuncCov::HandleTransitionL()
	{
	switch(iDmMemCov)
		{
		case ECovHandleError:
			// Simply ack. Since the request transition is for 0 deferral 
			// KErrNotSupported will anyways happen
			GetState();
			AcknowledgeLastState(KErrNone);
			RequestTransitionNotification();
			break;
		case ECovDoCancel:
			// do nothing, let the keep alive deferrals be active and let the CDmKeepAliveFuncCovTest delete this object
			CActiveScheduler::Stop();
			break;
		default:
			User::Leave(KErrUnknown);
			break;
		}
	}

class CDmDomainManFuncCov : public CDmDomainManager
	{
public:
	static CDmDomainManFuncCov* NewL(TDmHierarchyId aHierarchy);
	~CDmDomainManFuncCov();

	// from CDmDomainManager
	void RunL();
private:
	CDmDomainManFuncCov(TDmHierarchyId aHierarchy);
	};

CDmDomainManFuncCov* CDmDomainManFuncCov::NewL(TDmHierarchyId aHierarchy)
	{
	CDmDomainManFuncCov* self=new (ELeave) CDmDomainManFuncCov(aHierarchy);
	CleanupStack::PushL(self);

	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}

CDmDomainManFuncCov::~CDmDomainManFuncCov()
	{
	Cancel();
	}

CDmDomainManFuncCov::CDmDomainManFuncCov(TDmHierarchyId aHierarchy) :
	CDmDomainManager(aHierarchy)
	{
	}

void CDmDomainManFuncCov::RunL()
	{
	CActiveScheduler::Stop();
	}

class CDmKeepAliveFuncCovTest : public CActive, public MDmTest
	{
public:
	CDmKeepAliveFuncCovTest() : CActive(CActive::EPriorityStandard) {};
	void Perform();
	void Release();

	TInt TransitionNotification(MDmDomainMember&) { return KErrNone; };
	void TransitionRequestComplete() {};

		// from CActive
	void RunL() {};
	virtual void DoCancel()
		{
		test(0);
		}

public:
	CDmDomainManFuncCov*		iTestDomainManager;
	};

void CDmKeepAliveFuncCovTest::Perform()
	{
 	__UHEAP_MARK;

	CActiveScheduler::Add(this);

	test.Printf(_L("****************CFunctionalCoverageTest****************\n"));
	test.Next(_L("Test to perform code coverage"));

	TInt r = CDmDomainManager::AddDomainHierarchy(KDmHierarchyIdTestV2);
	test(r == KErrNone);

	TRAP_IGNORE(iTestDomainManager = CDmDomainManFuncCov::NewL(KDmHierarchyIdTestV2));
	test (iTestDomainManager != NULL);

	CDmKeepAliveFuncCov* member = NULL;
	// Add some test hierarchy members
	TRAP(r, member = CDmKeepAliveFuncCov::NewL(KDmHierarchyIdTestV2, KDmIdRoot));
	test(member != NULL);

	SDmStateSpecV1 aStateSpec = Get0DeferralState();

	TDmDomainState		testState =  aStateSpec.iState;

	// request a system transition
	iTestDomainManager->RequestSystemTransition(testState, ETraverseChildrenFirst);
	test.Printf(_L("Requested system transition\n"));

	member->iDmMemCov = CDmKeepAliveFuncCov::ECovHandleError;

	test.Printf(_L("HandleDeferralError functional coverage...\n"));
	// wait for test transition to complete
	CActiveScheduler::Start();

	test.Printf(_L("......system transition completed\n"));

	aStateSpec = GetMaxDeferralState();
	testState =  aStateSpec.iState;

	// request a system transition
	iTestDomainManager->RequestSystemTransition(testState, ETraverseChildrenFirst);
	test.Printf(_L("Requested system transition\n"));

	member->iDmMemCov = CDmKeepAliveFuncCov::ECovDoCancel;
	test.Printf(_L("DoCancel functional coverage...\n"));
	// wait for the member to call CActiveScheduler::Stop
	CActiveScheduler::Start();
	delete member;

	// wait for test transition to complete
	CActiveScheduler::Start();
	test.Printf(_L("......system transition completed\n"));

	// Add some test hierarchy members
	TRAP(r, member = CDmKeepAliveFuncCov::NewL(KDmHierarchyIdTestV2, KDmIdRoot));
	test(member != NULL);

	aStateSpec = GetMaxDeferralState();
	testState =  aStateSpec.iState;

	// request a system transition
	iTestDomainManager->RequestSystemTransition(testState, ETraverseChildrenFirst);
	test.Printf(_L("Requested system transition\n"));

	member->iDmMemCov = CDmKeepAliveFuncCov::ECovDoCancel; // just so that the member will call CActiveScheduler::Stop

	test.Printf(_L("DoCancel functional coverage...\n"));
	// wait for the member to call CActiveScheduler::Stop
	CActiveScheduler::Start();

	delete iTestDomainManager;
	delete member;

	__UHEAP_MARKEND;
	}

void CDmKeepAliveFuncCovTest::Release()
	{
	delete this;
	}


///////////////////////////////////////////////////////////////////////////////
// --- Main() ---

GLDEF_C TInt E32Main()
	{
	CTrapCleanup* trapHandler = CTrapCleanup::New();
	test(trapHandler != NULL);

	CActiveScheduler* scheduler = new CActiveScheduler();
	test(scheduler != NULL);
	CActiveScheduler::Install(scheduler);

	// Turn off evil lazy dll unloading
	RLoader l;
	test(l.Connect() == KErrNone);
	test(l.CancelLazyDllUnload()== KErrNone);
	l.Close();

	// Default number of iteration
    TInt iter = 1;

	TInt len = User::CommandLineLength();
	if (len)
		{
		// Copy the command line in a buffer
		HBufC* hb = HBufC::NewMax(len);
		test(hb != NULL);
		TPtr cmd((TUint16*) hb->Ptr(), len);
		User::CommandLine(cmd);
		// Extract the number of iterations
		TLex l(cmd);
		TInt i;
		TInt r = l.Val(i);
		if (r == KErrNone)
			iter = i;
		else
			// strange command - silently ignore
			{}
		delete hb;
		}

	test.Title();

	test.Start(_L("Test starting..."));

	test.Printf(_L("Go for %d iterations\n"), iter);

	// Remember the number of open handles. Just for a sanity check
	TInt start_thc, start_phc;
	RThread().HandleCount(start_phc, start_thc);

	for (TInt i = 1; i <= iter; i++)
		{
		test.Printf(_L("\nThis iteration: %d\n"), i);

		MDmTest* tests[] =
			{
			// DM Client PlatSec tests
			new CDmTestPlatSec(TPtrC(KSecuritySlavePath1)),
			new CDmTestPlatSec(TPtrC(KSecuritySlavePath2)),
			new CDmTestPlatSec(TPtrC(KSecuritySlavePath3)),

			// Domain Member R-Class API tests
			new CDmDeferralTestKErrNone(KDmHierarchyIdTestV2, EShutdownCritical, 1, ETrue), // TC 1.1.1.1.1
			new CDmDeferralTestKErrNone(KDmHierarchyIdTestV2, EShutdownCritical, 2, ETrue), // TC 1.1.1.1.2
			new CDmDeferralTestKErrNone(KDmHierarchyIdTestV2, EShutdownCritical, 1, EFalse), // TC 1.1.1.1.3
			new CDmDeferralTestCompletion(KDmHierarchyIdTestV2, EBackupMode), // TC 1.1.1.2.1
			new CDmDeferralTestKErrNotSupported(KDmHierarchyIdTestV2, EShutdownCritical, 6), // TC 1.1.1.3.1
			new CDmDeferralTestKErrNotSupported(KDmHierarchyIdTestV2, ENormalRunning, 1), // TC 1.1.1.3.2
			new CDmDeferralTestKErrNotSupported(KDmHierarchyIdTest, EBackupMode, 1), // TC 1.1.1.3.3
			new CDmDeferralTestKErrCancel(KDmHierarchyIdTestV2, EBackupMode, ETrue), // TC 1.1.1.4.1
			new CDmDeferralTestKErrCancel(KDmHierarchyIdTestV2, EBackupMode, EFalse), // TC 1.1.1.4.2
			new CDmDeferralTestKErrNotReady(KDmHierarchyIdTestV2, EBackupMode), // TC 1.1.1.5.1
			new CDmDeferralTestKErrServerBusy(KDmHierarchyIdTestV2, ERestoreMode), // TC 1.1.1.7.1

			new CDmDeferralTestKErrNone(KDmHierarchyIdTestV2, ERestoreMode, 0, ETrue), // TC 1.2.0
			new CDmDeferralTestKErrNone(KDmHierarchyIdTestV2, ERestoreMode, 1, ETrue), // TC 1.2.1
			new CDmDeferralTestKErrNone(KDmHierarchyIdTestV2, ERestoreMode, 2, ETrue), // TC 1.2.2
			new CDmDeferralTestKErrNone(KDmHierarchyIdTestV2, ERestoreMode, 3, ETrue), // TC 1.2.3

			new	CDmDeferralMixed(KDmHierarchyIdTestV2, EShutdownCritical, 3, ETrue,EFalse), // TC 1.3.1
			new	CDmDeferralMixed(KDmHierarchyIdTestV2, EShutdownCritical, 3, ETrue,ETrue),  // TC 1.3.2

			// Domain Member C-Class API tests
			new CDmDomainKeepAliveTest(),

			// Policy State Spec Failure Policy tests - transition timeouts
			//   ETransitionFailureUsePolicyFromOrdinal3
			new CDmFailurePolicyTest(EStartupCriticalStatic, HierarchyPolicy.iFailurePolicy),
			new CDmFailurePolicyTest(EStartupCriticalDynamic, ETransitionFailureStop),
			new CDmFailurePolicyTest(ENormalRunning, ETransitionFailureContinue),

			// Policy Interface tests
			new CDmPolicyInterfaceTest(),

			// Functional coverage test
			new CDmKeepAliveFuncCovTest(),
			};

		for (TUint j = 0; j < sizeof(tests)/sizeof(*tests); j++)
			{
			test(tests[j] != NULL);
			tests[j]->Perform();
			tests[j]->Release();
			}
		}

	test.End();

	// Sanity check for open handles and for pending requests
	TInt end_thc, end_phc;
	RThread().HandleCount(end_phc, end_thc);
	test(start_thc == end_thc);
	test(start_phc == end_phc);
	test(RThread().RequestCount() >= 0);

	delete scheduler;
	delete trapHandler;

	return KErrNone;
	}
