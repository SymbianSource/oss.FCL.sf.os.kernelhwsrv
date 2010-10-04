// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/domainmgr/t_domain.cpp
// Overview:
// Domain manager tests
// API Information:
// RDmDomain, RDmDomainManager CDmDomain, CDmDomainManager
// Details:
// - Test a variety of domain transitions, check the expected number of
// notifications and the first expected ordinal. Verify results are
// as expected.
// - Test system standby, check the expected number of notifications and 
// the first expected ordinal. Use a timer to request a wakeup event.
// Verify results are as expected.
// - Test domain related simple error situations, verify results are
// as expected.
// - Perform platform security tests: launch a separate process with no 
// capabilities, verify that results are as expected.
// - Test domain transitions by connecting to two domain hierarchies 
// simultaneously, add some test and power hierarchy members, verify
// the expected target state, notifications and leaf nodes. Verify results.
// - Verify that the same hierarchy can not be connected to more than once.
// - Request a positive transition and request that the test domain use 
// ETraverseParentsFirst. Verify results are as expected and verify 
// domains are in the correct state.
// - Request a negative transition and request that the test domain use 
// ETraverseChildrenFirst. Verify results are as expected.
// - Request a positive transition with zero acknowledgements. Verify 
// results are as expected.
// - Request a positive transition with error acknowledgements. Verify 
// results are as expected.
// - Perform a variety of negative tests and verify results are as expected.
// - Perform various tests on domain transitions with activated observer.
// Verify results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32debug.h>
#include <e32ldr_private.h>
#include <domainobserver.h>

#include "domainpolicytest.h"
#include "t_domain.h"


RTest test(_L(" T_DOMAIN "));
_LIT(KThreadName, "t_domain_panic_thread");

TDmHierarchyId GHierarchyIdUnderTest = 0;

#ifdef _DEBUG
#define __PRINT(x) {RDebug::Print x;}
#else
#define __PRINT(x) 
#endif


class CDmTest1 : public CActive, public MDmTest
	{
public: // from CActive
	void RunL();

	// from MDmTest
	void Perform();
	void Release();
	TInt TransitionNotification(MDmDomainMember& aDomainMember);
	void TransitionRequestComplete() {};

	CDmTest1 (TDmDomainId aId, TDmDomainState aState) : CActive(CActive::EPriorityStandard), iDomainId(aId), iState((TPowerState) aState) {}

protected:
	// from CActive
	virtual void DoCancel();

private:
	enum { KMembersMax = 16 };
	CDmTestMember*		iMembers[KMembersMax]; 
	RDmDomainManager	iManager;
	TDmDomainId			iDomainId;
	TPowerState			iState;
	TBool				iAcknowledge;
	TInt				iMembersCount;
	TInt				iCount;
	TUint32				iOrdinal;
	};

void CDmTest1::Perform()
	{
	//
	// Test domain transitions
	//

	test.Next(_L("Test 1"));
	test.Printf(_L("Domain id = 0x%x Target State = 0x%x\n"), iDomainId, iState);
	iMembers[0] = new CDmTestMember(KDmHierarchyIdPower, KDmIdRoot, 0, this);
	test(iMembers[0] != NULL);
	iMembers[1] = new CDmTestMember(KDmHierarchyIdPower, KDmIdRoot, 0, this);
	test(iMembers[1] != NULL);
	iMembers[2] = new CDmTestMember(KDmHierarchyIdPower, KDmIdApps, 1, this);
	test(iMembers[2] != NULL);
	iMembers[3] = new CDmTestMember(KDmHierarchyIdPower, KDmIdApps, 1, this);
	test(iMembers[3] != NULL);
	iMembers[4] = new CDmTestMember(KDmHierarchyIdPower, KDmIdUiApps, 1, this);
	test(iMembers[4] != NULL);
	iMembers[5] = new CDmTestMember(KDmHierarchyIdPower, KDmIdUiApps, 1, this);
	test(iMembers[5] != NULL);
	
	// expected number of notifications
	iMembersCount = (iDomainId == KDmIdRoot) ? 6 : 2;
	// first expected ordinal
	iOrdinal = (iState == EPwActive) ? 0 : 1;

	TInt r = iManager.Connect();
	test(r == KErrNone);

	CActiveScheduler::Add(this);

	iManager.RequestDomainTransition(iDomainId, iState, CActive::iStatus);
	CActive::SetActive();

	CActiveScheduler::Start();
	}

TInt CDmTest1::TransitionNotification(MDmDomainMember& aDomainMember)
	{
	++iCount;
	if (aDomainMember.State() == EPwActive)
		{
		if(aDomainMember.Ordinal() < iOrdinal)
			{
			// Making the test to fail in RunL function inorder to complete the cleanup from domain manager.
			test.Printf(_L("Making test to fail as Ordinal Mismatch Expected : %d, Returned : %d"), aDomainMember.Ordinal(), iOrdinal);
			iCount--;
			}
		}
	else
		{
		if(aDomainMember.Ordinal() > iOrdinal)
			{
			//Making the test to fail in RunL function inorder to complete the cleanup from domain manager.
			test.Printf(_L("Making test to fail as Ordinal Mismatch Expected : %d, Returned : %d"), aDomainMember.Ordinal(), iOrdinal);
			iCount--;
			}
		}
	iOrdinal = aDomainMember.Ordinal();

	// acknowledge one from two
	iAcknowledge = !iAcknowledge;
	return iAcknowledge?KErrNone:KErrGeneral;
	}

void CDmTest1::RunL()
	{
	CActiveScheduler::Stop();

	iManager.Close();

	CDmTestMember** mp;
	for (mp = iMembers; *mp; ++mp)
		delete *mp;
	test(iCount == iMembersCount);
	}

void CDmTest1::DoCancel()
	{
	test(0);
	}

void CDmTest1::Release()
	{
	delete this;
	}

class CDmTest2Timer : public CTimer
	{
public: // fomr CTimer
   void RunL();
public:
	CDmTest2Timer() : CTimer(0) 
		{
		TRAPD(r,
			ConstructL());
		test(r == KErrNone);
		CActiveScheduler::Add(this);
		}
	};

void CDmTest2Timer::RunL()
	{
	test.Printf(_L("Tick count after CDmTest2Timer::RunL() = %d\n"), User::NTickCount());

	// kick the timer again in case power down hasn't happened yet
	TTime wakeup;
	wakeup.HomeTime();
	wakeup += TTimeIntervalSeconds(3);
	At(wakeup);
	}

class CDmTest2 : public CActive, public MDmTest
	{
public: // from CActive
	void RunL();

	// from MDmTest
	void Perform();
	void Release();
	TInt TransitionNotification(MDmDomainMember& aDomainMember);
	void TransitionRequestComplete() {};
	CDmTest2 (TDmDomainState aState) : CActive(CActive::EPriorityStandard), iState((TPowerState) aState) {}

protected:
	// from CActive
	virtual void DoCancel();

private:
	enum { KMembersMax = 16 };
	CDmTestMember*		iMembers[KMembersMax]; 
	RDmDomainManager	iManager;
	TPowerState			iState;
	TBool				iAcknowledge;
	TInt				iMembersCount;
	TInt				iCount;
	TUint32				iOrdinal;
	CDmTest2Timer*		iTimer;
	};


void CDmTest2::Perform()
	{
	//
	// Test system standby
	//

	test.Next(_L("Test 2"));
	test.Printf(_L("Target State = 0x%x\n"), iState);
	iMembers[0] = new CDmTestMember(KDmHierarchyIdPower, KDmIdRoot, 0, this);
	test(iMembers[0] != NULL);
	iMembers[1] = new CDmTestMember(KDmHierarchyIdPower, KDmIdRoot, 0, this);
	test(iMembers[1] != NULL);
	iMembers[2] = new CDmTestMember(KDmHierarchyIdPower, KDmIdApps, 1, this);
	test(iMembers[2] != NULL);
	iMembers[3] = new CDmTestMember(KDmHierarchyIdPower, KDmIdApps, 1, this);
	test(iMembers[3] != NULL);
	iMembers[4] = new CDmTestMember(KDmHierarchyIdPower, KDmIdUiApps, 1, this);
	test(iMembers[4] != NULL);
	iMembers[5] = new CDmTestMember(KDmHierarchyIdPower, KDmIdUiApps, 1, this);
	test(iMembers[5] != NULL);
	
	// expected number of notifications
	iMembersCount = 12;
	// first expected ordinal
	iOrdinal = (iState == EPwActive) ? 0 : 1;

	TInt r = iManager.Connect();
	test(r == KErrNone);

	CActiveScheduler::Add(this);

	// Use an absolute timer to request a wakeup event
	iTimer = new CDmTest2Timer();
	TTime wakeup;
	wakeup.HomeTime();
	wakeup += TTimeIntervalSeconds(5);
	test.Printf(_L("Tick count before timer = %d\n"), User::NTickCount());
	iTimer->At(wakeup);
	
	iManager.RequestSystemTransition(iState, CActive::iStatus);
	CActive::SetActive();

	CActiveScheduler::Start();
	}

TInt CDmTest2::TransitionNotification(MDmDomainMember& aDomainMember)
	{
	++iCount;
	if (aDomainMember.State() == EPwActive)
		{
		if(aDomainMember.Ordinal() < iOrdinal)
			{
			// Making the test to fail in RunL function inorder to complete the cleanup from domain manager.
			test.Printf(_L("Making test to fail as Ordinal Mismatch Expected : %d, Returned : %d, State : %d"), 
																		aDomainMember.Ordinal(), iOrdinal, aDomainMember.State());
			iCount--;
			}
		}
	else
		{
		if(aDomainMember.Ordinal() > iOrdinal)
			{
			// Making the test to fail in RunL function inorder to complete the cleanup from domain manager.
			test.Printf(_L("Making test to fail as Ordinal Mismatch Expected : %d, Returned : %d, State: %d"), 
																		aDomainMember.Ordinal(), iOrdinal, aDomainMember.State());
			iCount--;
			}
		}
	iOrdinal = aDomainMember.Ordinal();

	// acknowledge one from two
	iAcknowledge = !iAcknowledge;
	return iAcknowledge?KErrNone:KErrAbort;
	}

void CDmTest2::RunL()
	{
	test.Printf(_L("Tick count after CDmTest2::RunL() = %d\n"), User::NTickCount());

	iTimer->Cancel();	
	CActiveScheduler::Stop();

	iManager.Close();

	CDmTestMember** mp;
	for (mp = iMembers; *mp; ++mp)
		delete *mp;
	test(CActive::iStatus == KErrTimedOut);
	test(iCount == iMembersCount);
	}

void CDmTest2::DoCancel()
	{
	test(0);
	}

void CDmTest2::Release()
	{
	if (iTimer)
		{
		iTimer->Cancel();
		delete iTimer;
		}
	delete this;
	}

class CDmTest3 : public MDmTest
	{
public: 
	// from MDmTest
	void Perform();
	void Release();
	TInt TransitionNotification(MDmDomainMember& aDomainMember);
	void TransitionRequestComplete() {};
	};

void CDmTest3::Perform()
	{
	//
	// Test simple error situation
	//
	RDmDomainManager manager;
	TInt r = manager.Connect();
	test(r == KErrNone);

	RDmDomainManager manager1;
	r = manager1.Connect();
	test(r == KErrInUse);

	RDmDomain domain;
	r = domain.Connect(KDmIdNone);
	test(r == KDmErrBadDomainId);
	CDmTestMember*		testMember;
	testMember = new CDmTestMember(KDmHierarchyIdPower, KDmIdApps, 1, this);
	test (testMember != NULL);

	TRequestStatus status;
	manager.RequestDomainTransition(KDmIdApps, EPwStandby, status);
	test(status.Int() == KRequestPending);

	TRequestStatus status1;
	manager.RequestDomainTransition(KDmIdApps, EPwActive, status1);
	User::WaitForRequest(status1);
	test(status1.Int() == KDmErrBadSequence);
	User::WaitForRequest(status);
	test(status.Int() == KErrTimedOut);

	// Since this test doesn't start the active scheduler, a domain member's RunL() will 
	// not get called so we need to re-request a domain transition notification manually
	User::WaitForRequest(testMember->iStatus);
	test(testMember->iStatus.Int() == KErrNone);
	testMember->iDomain.RequestTransitionNotification(testMember->iStatus);

	manager.RequestDomainTransition(KDmIdApps, EPwActive, status);
	test(status.Int() == KRequestPending);
	manager.CancelTransition();
	test(status.Int() == KErrCancel);
	manager.CancelTransition();
	User::WaitForRequest(status);
	test(status.Int() == KErrCancel);

	testMember->iDomain.CancelTransitionNotification();

	delete testMember;
	
	domain.Close();
	manager.Close();
	}

TInt CDmTest3::TransitionNotification(MDmDomainMember& /*aDomainMember*/)
	{
	test(0);
	return KErrAbort;	// don't acknowledge
	}

void CDmTest3::Release()
	{
	delete this;
	}

class CDmTest4 : public MDmTest
	{
public: 
	// from MDmTest
	void Perform();
	void Release();
	TInt TransitionNotification(MDmDomainMember& aDomainMember);
	void TransitionRequestComplete() {};
private:
	void ExecSlave(TUint arg);
	};

_LIT(KSecuritySlavePath, "t_domain_slave.exe");

void CDmTest4::ExecSlave(TUint aArg)
	{
	RProcess proc;
	TInt r = proc.Create(KSecuritySlavePath, TPtrC((TUint16*) &aArg, sizeof(aArg)/sizeof(TUint16)));
	test(r == KErrNone);
	TRequestStatus status;
	proc.Logon(status);
	proc.Resume();
	User::WaitForRequest(status);

    RDebug::Printf("CDmTest4::ExecSlave(%d) ExitType %d", aArg, proc.ExitType() );
    RDebug::Printf("CDmTest4::ExecSlave(%d) ExitReason %d", aArg, proc.ExitReason() );
	test(proc.ExitType() == EExitKill);
//	test(proc.ExitReason() == KErrPermissionDenied);

	CLOSE_AND_WAIT(proc);
	}

//! @SYMTestCaseID PBASE-T_DOMAIN-4
//! @SYMTestType CT
//! @SYMTestCaseDesc Dmain manager security tests
//! @SYMREQ 3722
//! @SYMTestActions Launches a separate process with no capabilities
//! @SYMTestExpectedResults  DM APIs should fail with KErrPermissionDenied
//! @SYMTestPriority High
//! @SYMTestStatus Defined
void CDmTest4::Perform()
	{
	//
	// Security tests
	//

	ExecSlave(0);

    ExecSlave(1);

	}

TInt CDmTest4::TransitionNotification(MDmDomainMember& /*aDomainMember*/)
	{
	test(0);
	return KErrNone;
	}

void CDmTest4::Release()
	{
	delete this;
	}

// Test hierarchy tests
class CDmTestStartupMember : public CDmTestMember
	{
public:
	CDmTestStartupMember(TDmHierarchyId aHierarchy, TDmDomainId aId, TUint32 aOrdinal, MDmTest*);

public:
private:
	};

CDmTestStartupMember::CDmTestStartupMember(TDmHierarchyId aHierarchy, TDmDomainId aId, TUint32 aOrdinal, MDmTest* aTest) 
	: CDmTestMember(aHierarchy, aId, aOrdinal, aTest)
	{
	}

// Simultaneously testing of test domain defined in DomainPolicy99.dll
// and the power domain defined in DomainPolicy.dll
class CDmTest5 : public CActive, public MDmTest
	{
public: 
	// from CActive
	void RunL();
	// from MDmTest
	void Perform();
	void Release();
	TInt TransitionNotification(MDmDomainMember& aDomainMember);
	void TransitionRequestComplete();	
	void RunTestOnGetTransitionFailures(RArray<const TTransitionFailure>& aTransitionFailure);
	
	CDmTest5(TDmDomainId aPowerId, TDmDomainId aTestId, TDmDomainState aPowerState, TDmDomainState aTestState) : 
		CActive(CActive::EPriorityStandard), 
		iPowerDomainId(aPowerId), iTestDomainId(aTestId), iPowerState(aPowerState), iTestState(aTestState) {}
protected:
	// from CActive
	virtual void DoCancel();

private:
	enum { KMembersMax = 16 };
	enum TAckMode{ KAckAlways, KAckNever, KAckError, KAckOddDomainsOnly };

	CDmTestMember*		iTestMembers[KMembersMax]; 
	CDomainMemberAo*	iPowerMembers[KMembersMax]; 

	RDmDomainManager	iTestDomainManager;
	
	TDmDomainId			iPowerDomainId;
	TDmDomainId			iTestDomainId;

	TDmDomainState		iPowerState;
	TDmDomainState		iTestState;

	// level number for iTestDomainId. E.g 1 for KDmIdRoot, 2 for KDmIdTestA, etc.
	TInt				iTestDomainLevel;	

	TDmTraverseDirection iTraverseDirection;

	TAckMode			iAckMode;

public:
	TInt				iTestNotifications;
	TInt				iPowerNotifications;
	TInt				iTestNotificationsExpected;
	TInt				iPowerNotificationsExpected;

	TInt				iTransitionsCompleted;
	TInt				iTransitionsExpected;
	};

void CDmTest5::RunTestOnGetTransitionFailures(RArray<const TTransitionFailure>& aTransitionFailure)
	{
	//*************************************************
	// Test - OOM Testing on GetTransitionFailures()
	// Simulates heap failure in GetTransitionFailures()
	//*************************************************
	TInt error = 0;
	TInt count = 0;	
	do
		{		
		__UHEAP_SETFAIL(RHeap::EFailNext, ++count);
		error = iTestDomainManager.GetTransitionFailures(aTransitionFailure);						
		test.Printf( _L( "CDmTest5::RunTestOnGetTransitionFailures, simulating heap failure on GetTransitionFailures(), Error=%d, Run=%d\n" ), error, count );
		}while(error == KErrNoMemory);		
		
	__UHEAP_RESET;
	
	//Actual count of heap failure as the final iteration which terminates the loop would not return KErrNoMemory 
	--count;
	test(count > 0);
	test.Printf( _L( "Out of memory tests on GetTransitionFailures() succeeded at heap failure rate of %i\n" ), count );
	}

//! @SYMTestCaseID PBASE-T_DOMAIN-5
//! @SYMTestType CT
//! @SYMTestCaseDesc Connects to two domain hierarchies simulteneously and perform various tests
//! @SYMREQ 3704,3705,3706,3707,3708,3709,3710,3711,3720,3721,3724,3725,3726,3727
//! @SYMTestActions Open two hiearchies simultaneously and perform various actions.
//! @SYMTestExpectedResults  All tests should pass
//! @SYMTestPriority High
//! @SYMTestStatus Defined
void CDmTest5::Perform()
	{

 	__UHEAP_MARK;

	//
	// Test domain transitions
	//
	CActiveScheduler::Add(this);

	TInt r = RDmDomainManager::AddDomainHierarchy(GHierarchyIdUnderTest);

    RDebug::Printf("RDmDomainManager::AddDomainHierarchy returns %d", r );

	test(r == KErrNone);

	CDomainManagerAo* powerDomainManager = NULL;
	TRAP(r, powerDomainManager = CDomainManagerAo::NewL(KDmHierarchyIdPower, *this));
	test (powerDomainManager != NULL);

	r = CDomainManagerAo::AddDomainHierarchy(KDmHierarchyIdPower);
	test(r == KErrNone);

	//*************************************************
	//	Test 5a - connect to two domain hierarchies simultaneously
	//*************************************************
	test.Next(_L("Test 5a - connect to two domain hierarchies simultaneously"));

	test.Printf(_L("Domain id = 0x%x, Target State = 0x%x\n"), iTestDomainId, iTestState);

	TInt testMemberCount = 0;

	// Add some test hierarchy members - these use the RDmDomain API
	iTestMembers[testMemberCount] = new CDmTestMember(GHierarchyIdUnderTest, KDmIdRoot, ORDINAL_FROM_DOMAINID0(KDmIdRoot), this);
	test(iTestMembers[testMemberCount++] != NULL);
	iTestMembers[testMemberCount] = new CDmTestMember(GHierarchyIdUnderTest, KDmIdRoot, ORDINAL_FROM_DOMAINID0(KDmIdRoot), this);
	test(iTestMembers[testMemberCount++] != NULL);
	
	// row 1
	iTestMembers[testMemberCount] = new CDmTestMember(GHierarchyIdUnderTest, KDmIdTestA, ORDINAL_FROM_DOMAINID1(KDmIdRoot, KDmIdTestA), this);
	test(iTestMembers[testMemberCount++] != NULL);
	iTestMembers[testMemberCount] = new CDmTestMember(GHierarchyIdUnderTest, KDmIdTestB, ORDINAL_FROM_DOMAINID1(KDmIdRoot, KDmIdTestB), this);
	test(iTestMembers[testMemberCount++] != NULL);
	iTestMembers[testMemberCount] = new CDmTestMember(GHierarchyIdUnderTest, KDmIdTestC, ORDINAL_FROM_DOMAINID1(KDmIdRoot, KDmIdTestC), this);
	test(iTestMembers[testMemberCount++] != NULL);
	
	// row2
	iTestMembers[testMemberCount] = new CDmTestMember(GHierarchyIdUnderTest, KDmIdTestAA, ORDINAL_FROM_DOMAINID2(KDmIdRoot, KDmIdTestA, KDmIdTestAA), this);
	test(iTestMembers[testMemberCount++] != NULL);
	iTestMembers[testMemberCount] = new CDmTestMember(GHierarchyIdUnderTest, KDmIdTestAB, ORDINAL_FROM_DOMAINID2(KDmIdRoot, KDmIdTestA, KDmIdTestAB), this);
	test(iTestMembers[testMemberCount++] != NULL);
	iTestMembers[testMemberCount] = new CDmTestMember(GHierarchyIdUnderTest, KDmIdTestBA, ORDINAL_FROM_DOMAINID2(KDmIdRoot, KDmIdTestB, KDmIdTestBA), this);
	test(iTestMembers[testMemberCount++] != NULL);
	iTestMembers[testMemberCount] = new CDmTestMember(GHierarchyIdUnderTest, KDmIdTestCA, ORDINAL_FROM_DOMAINID2(KDmIdRoot, KDmIdTestC, KDmIdTestCA), this);
	test(iTestMembers[testMemberCount++] != NULL);
	
	// row 3
	iTestMembers[testMemberCount] = new CDmTestMember(GHierarchyIdUnderTest, KDmIdTestABA, ORDINAL_FROM_DOMAINID3(KDmIdRoot, KDmIdTestA, KDmIdTestAB, KDmIdTestABA), this);
	test(iTestMembers[testMemberCount++] != NULL);
	iTestMembers[testMemberCount] = new CDmTestMember(GHierarchyIdUnderTest, KDmIdTestABB, ORDINAL_FROM_DOMAINID3(KDmIdRoot, KDmIdTestA, KDmIdTestAB, KDmIdTestABB), this);
	test(iTestMembers[testMemberCount++] != NULL);
	iTestMembers[testMemberCount] = new CDmTestMember(GHierarchyIdUnderTest, KDmIdTestCAA, ORDINAL_FROM_DOMAINID3(KDmIdRoot, KDmIdTestC, KDmIdTestCA, KDmIdTestCAA), this);
	test(iTestMembers[testMemberCount++] != NULL);

	// add some power hierarchy members - these use the CDmDomain AO API
	TInt powerMemberCount = 0;
	TRAP(r, iPowerMembers[powerMemberCount] = CDomainMemberAo::NewL(KDmHierarchyIdPower, KDmIdRoot, KDmIdRoot, this));
	test(iTestMembers[powerMemberCount++] != NULL);
	TRAP(r, iPowerMembers[powerMemberCount] = CDomainMemberAo::NewL(KDmHierarchyIdPower, KDmIdApps, KDmIdApps, this));
	test(iTestMembers[powerMemberCount++] != NULL);
	TRAP(r, iPowerMembers[powerMemberCount] = CDomainMemberAo::NewL(KDmHierarchyIdPower, KDmIdUiApps, KDmIdUiApps, this));
	test(iTestMembers[powerMemberCount++] != NULL);


	RArray<const TTransitionFailure> testFailures;
	TInt testFailureCount;
	RArray<const TTransitionFailure> powerFailures;
	TInt powerFailureCount;



	// calculate the expected number of notifications
	TInt expectedTestNotifications = 0;
	TInt leafNodes = 0;
	

	// work out the domain level, the number of leaf nodes and the expected number of 
	// notifications for the domain that is being transitioned
	switch(iTestDomainId)
		{
		case KDmIdRoot		:	iTestDomainLevel = 1; leafNodes = 5; expectedTestNotifications = testMemberCount; break;
		case KDmIdTestA		:	iTestDomainLevel = 2; leafNodes = 3; expectedTestNotifications = 5; break;
		case KDmIdTestB		:	iTestDomainLevel = 2; leafNodes = 1; expectedTestNotifications = 2; break;
		case KDmIdTestC		:	iTestDomainLevel = 2; leafNodes = 1; expectedTestNotifications = 3; break;

		case KDmIdTestAA	:	iTestDomainLevel = 3; leafNodes = 1; expectedTestNotifications = 1; break;
		case KDmIdTestAB	:	iTestDomainLevel = 3; leafNodes = 2; expectedTestNotifications = 3; break;
		case KDmIdTestBA	:	iTestDomainLevel = 3; leafNodes = 1; expectedTestNotifications = 1; break;
		case KDmIdTestCA	:	iTestDomainLevel = 3; leafNodes = 1; expectedTestNotifications = 2; break;

		case KDmIdTestABA	:	iTestDomainLevel = 4; leafNodes = 1; expectedTestNotifications = 1; break;
		case KDmIdTestABB	:	iTestDomainLevel = 4; leafNodes = 1; expectedTestNotifications = 1; break;
		case KDmIdTestCAA	:	iTestDomainLevel = 4; leafNodes = 1; expectedTestNotifications = 1; break;
		default:
			test(0);
		}
	test.Printf(_L("Test Domain id = 0x%x, Level = %d, Target State = 0x%x, expected notifications = %d, leafNodes = %d\n"), 
		iTestDomainId, iTestDomainLevel, iTestState, expectedTestNotifications, leafNodes);

	TInt expectedPowerNotifications = 0;
	switch(iPowerDomainId)
		{
		case KDmIdRoot		:	expectedPowerNotifications = powerMemberCount; break;
		case KDmIdApps		:	expectedPowerNotifications = 1; break;
		case KDmIdUiApps	:	expectedPowerNotifications = 1; break;
		default:
			test(0);
		}



	// connect to the test hierarchy
	r = iTestDomainManager.Connect(GHierarchyIdUnderTest);
	test(r == KErrNone);

	// verify that we can't connect to the same hierarchy more than once
	RDmDomainManager	domainManager;
	r = domainManager.Connect(GHierarchyIdUnderTest);
	test(r == KErrInUse);



	//*************************************************
	// Test 5b - request a positive transition
	// issue a positive  transition (i.e. transition state increases)
	// and request that the test domain use ETraverseParentsFirst
	//*************************************************
	test.Next(_L("Test 5b - request a positive transition"));
	iAckMode = KAckAlways;

	iTransitionsCompleted = iTestNotifications = iPowerNotifications = 0;
	iPowerNotificationsExpected = 0;
	iTestNotificationsExpected = expectedTestNotifications;
	iTransitionsExpected = 1;

	// DON'T request any domain transition on the power hierarchy
	// powerDomainManager->RequestDomainTransition(iPowerDomainId, EPwActive);
	// request a domain transition on the test hierarchy
	iTraverseDirection = ETraverseParentsFirst;
	if (iTestDomainId == KDmIdRoot)
		iTestDomainManager.RequestSystemTransition(iTestState, ETraverseDefault, CActive::iStatus);
	else
		iTestDomainManager.RequestDomainTransition(iTestDomainId, iTestState, ETraverseDefault, CActive::iStatus);
	CActive::SetActive();

	CActiveScheduler::Start();
	test(powerDomainManager->iStatus == KErrNone);
	test(iStatus == KErrNone);
	test(iTestNotifications == iTestNotificationsExpected);
	test(iPowerNotifications == iPowerNotificationsExpected);

	//*************************************************
	// Test 5c- verify domains are in correct state
	//*************************************************
	test.Next(_L("Test 5c- verify domains are in correct state"));
	RDmDomain domainMember;
	r = domainMember.Connect(GHierarchyIdUnderTest, iTestDomainId);
	test (r == KErrNone);
	TDmDomainState state = domainMember.GetState();
	domainMember.Close();
	test (state == iTestState);

	// if the transition request is not on the root, verify that that 
	// the root domain and the transition domain are in different states
	if (iTestDomainId != KDmIdRoot && iTestState != EStartupCriticalStatic)
		{
		r = domainMember.Connect(GHierarchyIdUnderTest, KDmIdRoot);
		test (r == KErrNone);
		TDmDomainState state = domainMember.GetState();
		domainMember.Close();
		test (state != iTestState);
		}


	//*************************************************
	// Test 5d- request a negative transition
	// issue a negative transition (i.e. transition state decreases)
	// and request that the test domain use ETraverseChildrenFirst
	//*************************************************
	test.Next(_L("Test 5d- request a negative transition"));
	iAckMode = KAckAlways;
	iTestState--;	// EStartupCriticalStatic;
	iPowerState--;	// EPwStandby

	iTransitionsCompleted = iTestNotifications = iPowerNotifications = 0;
	iPowerNotificationsExpected = expectedPowerNotifications;
	iTestNotificationsExpected = expectedTestNotifications;
	iTransitionsExpected = 2;

	// DO request a domain transition on the power hierarchy
	powerDomainManager->RequestDomainTransition(iPowerDomainId, iPowerState, ETraverseDefault);

	// request a domain transition on the test hierarchy
	iTraverseDirection = ETraverseChildrenFirst;
	iTestDomainManager.RequestDomainTransition(iTestDomainId, iTestState, iTraverseDirection, CActive::iStatus);
	CActive::SetActive();

	// wait for all test & power transitions to complete
	CActiveScheduler::Start();
	test(powerDomainManager->iStatus == KErrNone);
	test(iStatus == KErrNone);
	test(iTestNotifications == iTestNotificationsExpected);
	test(iPowerNotifications == iPowerNotificationsExpected);
	

	//*************************************************
	// Test 5e- request a positive transition, with zero acknowledgements
	// issue a positive transition with no members acknowledging the transition
	// Expect timeout from server
	// Also covers Testcase 2.3.2 from Transition Monitoring test suite when
	// policy used in 96 or 97.
	//*************************************************
	test.Next(_L("Test 5e- request a positive transition, with zero acknowledgements"));
	iAckMode = KAckNever;
	iTestState++;		// EStartupCriticalDynamic;
	iPowerState++;		// EPwActive

	// power hierarchy should continue on failure, so we all power domains should transition
	// test hierarchy should stop on failure, so should get notifications from all leaf nodes
	iTransitionsCompleted = iTestNotifications = iPowerNotifications = 0;
	iPowerNotificationsExpected = expectedPowerNotifications;
	iTestNotificationsExpected = leafNodes;	// 5 leaf nodes for root domain
	iTransitionsExpected = 2;

	// DO request a domain transition on the power hierarchy
	powerDomainManager->RequestDomainTransition(iPowerDomainId, iPowerState, ETraverseDefault);

	// request a domain transition on the test hierarchy
	iTraverseDirection = ETraverseChildrenFirst;
	iTestDomainManager.RequestDomainTransition(iTestDomainId, iTestState, iTraverseDirection, CActive::iStatus);
	CActive::SetActive();

	// wait for all test & power transitions to complete
	CActiveScheduler::Start();
	test(powerDomainManager->iStatus == KErrTimedOut);
	test(iStatus == KErrTimedOut);
	test(iTestNotifications == iTestNotificationsExpected);
	test(iPowerNotifications == iPowerNotificationsExpected);
	
	// get the failures on the test hierarchy
	testFailureCount = iTestDomainManager.GetTransitionFailureCount();
	test (testFailureCount == 1);

#ifdef _DEBUG
	//***************************************************************
	// OOM Testing: Simulates heap failure in GetTransitionFailures()
	//***************************************************************
	__UHEAP_MARK;
	RArray<const TTransitionFailure> oomTestFailures;
	RunTestOnGetTransitionFailures(oomTestFailures);
	test(oomTestFailures.Count()==1);
	oomTestFailures.Close();
	__UHEAP_MARKEND;
#endif
	
	r = iTestDomainManager.GetTransitionFailures(testFailures);
	test(r == KErrNone);
	test(testFailureCount == testFailures.Count());

	test.Printf(_L("Test failures = %d\n"), testFailureCount);
	TInt i;
	for (i=0; i<testFailureCount; i++)
		{
		test.Printf(_L("%d: iDomainId %d, iError %d\n"), 
			i, testFailures[i].iDomainId, testFailures[i].iError);
		test(testFailures[i].iError == KErrTimedOut);
		}

	// get the failures on the power hierarchy
	powerFailureCount = powerDomainManager->GetTransitionFailureCount();
	test (powerFailureCount == expectedPowerNotifications);

	r = powerDomainManager->GetTransitionFailures(powerFailures);
	test(r == KErrNone);
	test(powerFailureCount == powerFailures.Count());

	test.Printf(_L("Power failures = %d\n"), powerFailureCount);
	for (i=0; i<powerFailureCount; i++)
		{
		test.Printf(_L("%d: iDomainId %d, iError %d\n"), 
			i, powerFailures[i].iDomainId, powerFailures[i].iError);
		test(powerFailures[i].iError == KErrTimedOut);
		}

	
	//*************************************************
	// Test 5f- request a positive transition, with error acknowledgements
	// issue a positive transition with all members nack'ing the transition
	//*************************************************
	test.Next(_L("Test 5f- request a positive transition, with error acknowledgements"));
	iAckMode = KAckError;
	iTestState++;		
	iPowerState++;		

	// power hierarchy should continue on failure, so all power domains should transition
	// test hierarchy should stop on failure, so should get notifications from 
	// anything from 1 to all the leaf nodes
	iTransitionsCompleted = iTestNotifications = iPowerNotifications = 0;
	iPowerNotificationsExpected = expectedPowerNotifications;
	iTestNotificationsExpected = leafNodes;	// 5 leaf nodes for root domain
	iTransitionsExpected = 2;

	// DO request a domain transition on the power hierarchy
	powerDomainManager->RequestDomainTransition(iPowerDomainId, iPowerState, ETraverseDefault);

	// request a domain transition on the test hierarchy
	iTraverseDirection = ETraverseChildrenFirst;
	iTestDomainManager.RequestDomainTransition(iTestDomainId, iTestState, iTraverseDirection, CActive::iStatus);
	CActive::SetActive();

	// wait for all test & power transitions to complete
	CActiveScheduler::Start();
	test(powerDomainManager->iStatus == KErrGeneral);
	test(iStatus == KErrGeneral);
	test(iTestNotifications <= iTestNotificationsExpected);
	test(iPowerNotifications == iPowerNotificationsExpected);
	
	// get the failures on the test hierarchy
	testFailureCount = iTestDomainManager.GetTransitionFailureCount();
	test (testFailureCount == 1);

	r = iTestDomainManager.GetTransitionFailures(testFailures);
	test(r == KErrNone);
	test(testFailureCount == testFailures.Count());

	test.Printf(_L("Test failures = %d\n"), testFailureCount);
	for (i=0; i<testFailureCount; i++)
		{
		test.Printf(_L("%d: iDomainId %d, iError %d\n"), 
			i, testFailures[i].iDomainId, testFailures[i].iError);
		test(testFailures[i].iError == KErrGeneral);
		}

	// get the failures on the power hierarchy
	powerFailureCount = powerDomainManager->GetTransitionFailureCount();
	test (powerFailureCount == expectedPowerNotifications);

	r = powerDomainManager->GetTransitionFailures(powerFailures);
	test(r == KErrNone);
	test(powerFailureCount == powerFailures.Count());

	test.Printf(_L("Power failures = %d\n"), powerFailureCount);
	for (i=0; i<powerFailureCount; i++)
		{
		test.Printf(_L("%d: iDomainId %d, iError %d\n"), 
			i, powerFailures[i].iDomainId, powerFailures[i].iError);
		test(powerFailures[i].iError == KErrGeneral);
		}

	
	// cleanup

	testFailures.Reset();
	powerFailures.Reset();

	iTestDomainManager.Close();
	delete powerDomainManager;
	powerDomainManager = NULL;

	CDmTestMember** mt;
	for (mt = iTestMembers; *mt; ++mt)
		delete *mt;

	CDomainMemberAo** mp;
	for (mp = iPowerMembers; *mp; ++mp)
		delete *mp;


	// restore the domain hierarchies to their initial state so as not to 
	// upset any subsequent tests which rely on this
	{
	RDmDomainManager manager;
	TInt r = manager.Connect();
	test (r == KErrNone);
	TRequestStatus status;
	manager.RequestDomainTransition(KDmIdRoot, EPwActive, status);
	test(status.Int() == KRequestPending);
	User::WaitForRequest(status);
	test(status.Int() == KErrNone);
	manager.Close();
	
	r = manager.Connect(GHierarchyIdUnderTest);
	test (r == KErrNone);
	manager.RequestDomainTransition(KDmIdRoot, EStartupCriticalStatic, ETraverseDefault, status);
	test(status.Int() == KRequestPending);
	User::WaitForRequest(status);
	test(status.Int() == KErrNone);
	manager.Close();
	}

 	__UHEAP_MARKEND;
	}

// This handles a transition notification from either a power domain member or 
// a test domain member.
// Verifies that the domain state is as expected.
// Updates the number of notifications for each hierarchy and verifies that all parent 
// domains have transitioned already (for parent-to-child transitions) or that all child 
// domains have been transitioned already (for child-to-parent transitions).

TInt CDmTest5::TransitionNotification(MDmDomainMember& aDomainMember)
	{
	if (aDomainMember.HierarchyId() == KDmHierarchyIdPower)
		iPowerNotifications++;
	else
		iTestNotifications++;

	if (aDomainMember.HierarchyId() == KDmHierarchyIdPower)
		{
		__PRINT((_L("CDmTest5::TransitionNotification(), Hierarchy = %d, iOrdinal = 0x%08X, state = 0x%x, status = %d\n"), 
			aDomainMember.HierarchyId(), aDomainMember.Ordinal(), aDomainMember.State(), aDomainMember.Status()));
		test(aDomainMember.State() == iPowerState);
		}
	else if (aDomainMember.HierarchyId() == GHierarchyIdUnderTest)
		{
		TBuf16<4> buf;
		GetDomainDesc(aDomainMember.Ordinal(), buf);

		__PRINT((_L("CDmTest5::TransitionNotification(), Hierarchy = %d, domain = %S, iOrdinal = 0x%08X, state = 0x%x, status = %d\n"), 
			aDomainMember.HierarchyId(), &buf, aDomainMember.Ordinal(), aDomainMember.State(), aDomainMember.Status()));
		test(aDomainMember.State() == iTestState);
		}
	else
		{
		test(0);
		}

	// if we're going from parent to child, 
	// check that each parent domain has received a notification already
	// if not, check that each child domain has received a notification already

	CDmTestMember** mp;

	if (aDomainMember.HierarchyId() == GHierarchyIdUnderTest && iAckMode == KAckAlways)
		{

		if (iTraverseDirection == ETraverseParentsFirst)
			{
			TUint ordThis = aDomainMember.Ordinal();
			TUint ordParent = PARENT_ORDINAL(ordThis);

			TInt levelParent = ORDINAL_LEVEL(ordParent);

			TBuf16<4> buf;
			GetDomainDesc(ordParent, buf);
			if (levelParent >= iTestDomainLevel)
				{
				__PRINT((_L("Searching for parent domain = %S, ordinal = %08X \n"), &buf, ordParent));
				for (mp = iTestMembers; *mp; ++mp)
					{
					if ((*mp)->Ordinal() == ordParent)
						{
						TBuf16<4> buf;
						GetDomainDesc((*mp)->Ordinal(), buf);
						__PRINT((_L("Found parent (%S). notification = %d\n"), &buf, (*mp)->Notifications()));
						test ((*mp)->Notifications() == aDomainMember.Notifications());
						break;
						}
					}
				}
			}
		else
			{
			__PRINT((_L("Searching for children\n")));
			for (mp = iTestMembers; *mp; ++mp)
				{

				TUint ordParent = PARENT_ORDINAL((*mp)->Ordinal());
				if (ordParent == aDomainMember.Ordinal())
					{
					TBuf16<4> buf;
					GetDomainDesc((*mp)->Ordinal(), buf);
					__PRINT((_L("Found child (%S). notification = %d\n"), &buf, (*mp)->Notifications()));
					test ((*mp)->Notifications() == aDomainMember.Notifications());
					}
				}
			}
		}

	TInt ackError;
	switch (iAckMode)
		{
		case KAckNever:
			ackError = KErrAbort;
			break;
		case KAckError:		// return an error to the DM
			ackError = KErrGeneral;
			break;
		case KAckOddDomainsOnly:
			ackError = (aDomainMember.DomainId() & 1)?KErrNone:KErrAbort;
			break;
		case KAckAlways:
		default:
			ackError = KErrNone;
			break;
		}
	return ackError;
	}

void CDmTest5::RunL()
	{
	iTransitionsCompleted++;

	__PRINT((_L("CDmTest5::RunL(), error = %d, iTestNotifications %d, iPowerNotifications %d\n"), 
		iStatus.Int(), iTestNotifications , iPowerNotifications));

	if (iTransitionsCompleted == iTransitionsExpected)
		CActiveScheduler::Stop();
	}

void CDmTest5::TransitionRequestComplete()
	{
	iTransitionsCompleted++;

	__PRINT((_L("CDmTest5::TransitionRequestComplete(), error = %d, iTestNotifications %d, iPowerNotifications %d\n"), 
		iStatus.Int(), iTestNotifications , iPowerNotifications));
	
	if (iTransitionsCompleted == iTransitionsExpected)
		CActiveScheduler::Stop();
	}

void CDmTest5::DoCancel()
	{
	test(0);
	}

void CDmTest5::Release()
	{
	delete this;
	}

const TInt KMembersMax = 16;

// Negative testing 
class CDmTest6 : public CActive, public MDmTest
	{
public:
	enum 
	{
	ENegTestTransitionNoConnect,
	ENegTestGetStateNoConnect,
	ENegTestTransitionInvalidMode
	};

	class TData 
		{
	public:
		inline TData(TInt aTest) : iTest(aTest){};
		TInt iTest;
		};

public: 
	// from CActive
	void RunL();
 
	// from MDmTest
	void Perform();
	void Release();
	TInt TransitionNotification(MDmDomainMember& aDomainMember);
	void TransitionRequestComplete();


	CDmTest6() : CActive(CActive::EPriorityStandard) {}

protected:
	// from CActive
	virtual void DoCancel();

private:
	static TInt PanicThreadFunc(TAny* aData);
	void PanicTest(TInt aTestNumber);


	CDomainMemberAo*	iTestMembers[KMembersMax]; 
	CDomainManagerAo*	iTestDomainManager;
	
	TDmDomainId			iTestDomainId;
	TDmDomainState		iTestState;

public:
	TInt				iTestNotifications;
	TInt				iTestNotificationsExpected;

	TInt				iTransitionsCompleted;
	TInt				iTransitionsExpected;
	};

TInt CDmTest6::PanicThreadFunc(TAny* aData)
	{
	const TData* data = (const TData*)aData;
	switch (data->iTest)
		{
		case ENegTestTransitionNoConnect:
			{
			// request a transition notification without connecting first (should panic)
			RDmDomain domainMember;
			TRequestStatus status;
			User::SetJustInTime(EFalse);
			domainMember.RequestTransitionNotification(status);
			}
			break;
		case ENegTestGetStateNoConnect:
			{
			// Get the domain state without connecting (should panic)
			RDmDomain domainMember;
			User::SetJustInTime(EFalse);
			domainMember.GetState();
			}
			break;
		case ENegTestTransitionInvalidMode:
			{
			RDmDomainManager manager;
			TRequestStatus status;
			TInt r = manager.Connect(GHierarchyIdUnderTest);
			test(r == KErrNone);

			User::SetJustInTime(EFalse);
			manager.RequestDomainTransition(KDmIdRoot, 0, TDmTraverseDirection(-1), status);
			}
			break;
		default:
			break;
		}
	return KErrNone;
	}

void CDmTest6::PanicTest(TInt aTestNumber)
	{
	test.Printf(_L("panic test number %d\n"), aTestNumber);

	TBool jit = User::JustInTime();

	TData data(aTestNumber);

	TInt KHeapSize=0x2000;

	RThread thread;
	TInt ret = thread.Create(KThreadName, PanicThreadFunc, KDefaultStackSize, KHeapSize, KHeapSize, &data);
	test(KErrNone == ret);
	TRequestStatus stat;
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);

	User::SetJustInTime(jit);

	// The thread must panic
	test(thread.ExitType() == EExitPanic);
	TInt exitReason = thread.ExitReason();
	test.Printf(_L("panic test exit reason = %d\n"), exitReason);

	switch(aTestNumber)
		{
		case ENegTestTransitionNoConnect:
			test (exitReason == EBadHandle);
			break;
		case ENegTestGetStateNoConnect:
			test (exitReason == EBadHandle);
			break;
		case ENegTestTransitionInvalidMode:
			break;
		default:
			break;
		}

	CLOSE_AND_WAIT(thread);
	}


//! @SYMTestCaseID PBASE-T_DOMAIN-6
//! @SYMTestType CT
//! @SYMTestCaseDesc Negative testing
//! @SYMPREQ 810
//! @SYMTestActions Various negative tests
//! @SYMTestExpectedResults  All tests should pass
//! @SYMTestPriority High
//! @SYMTestStatus Defined
void CDmTest6::Perform()
	{

 	__UHEAP_MARK;

	CActiveScheduler::Add(this);

	CDomainManagerAo* iTestDomainManager = NULL;
	TRAP_IGNORE(iTestDomainManager = CDomainManagerAo::NewL(GHierarchyIdUnderTest, *this));
	test (iTestDomainManager != NULL);

	TInt r = CDomainManagerAo::AddDomainHierarchy(GHierarchyIdUnderTest);
	test(r == KErrNone);

	//*************************************************
	// Test 6a - Connect to the same hierarchy twice
	//*************************************************
	test.Next(_L("Test 6a - Connect to the same hierarchy twice"));

	// verify that we can't connect to the same hierarchy more than once
	CDomainManagerAo* testDomainManager = NULL;
	TRAP(r, testDomainManager = CDomainManagerAo::NewL(GHierarchyIdUnderTest, *this));
	test(r == KErrInUse);
	test (testDomainManager == NULL);


	TInt testMemberCount = 0;

	// Add some test hierarchy members
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdRoot, ORDINAL_FROM_DOMAINID0(KDmIdRoot), this));
	test(iTestMembers[testMemberCount++] != NULL);
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdRoot, ORDINAL_FROM_DOMAINID0(KDmIdRoot), this));
	test(iTestMembers[testMemberCount++] != NULL);
	
	// row 1
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestA, ORDINAL_FROM_DOMAINID1(KDmIdRoot, KDmIdTestA), this));
	test(iTestMembers[testMemberCount++] != NULL);
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestB, ORDINAL_FROM_DOMAINID1(KDmIdRoot, KDmIdTestB), this));
	test(iTestMembers[testMemberCount++] != NULL);
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestC, ORDINAL_FROM_DOMAINID1(KDmIdRoot, KDmIdTestC), this));
	test(iTestMembers[testMemberCount++] != NULL);
	
	// row2
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestAA, ORDINAL_FROM_DOMAINID2(KDmIdRoot, KDmIdTestA, KDmIdTestAA), this));
	test(iTestMembers[testMemberCount++] != NULL);
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestAB, ORDINAL_FROM_DOMAINID2(KDmIdRoot, KDmIdTestA, KDmIdTestAB), this));
	test(iTestMembers[testMemberCount++] != NULL);
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestBA, ORDINAL_FROM_DOMAINID2(KDmIdRoot, KDmIdTestB, KDmIdTestBA), this));
	test(iTestMembers[testMemberCount++] != NULL);
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestCA, ORDINAL_FROM_DOMAINID2(KDmIdRoot, KDmIdTestC, KDmIdTestCA), this));
	test(iTestMembers[testMemberCount++] != NULL);
	
	// row 3
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestABA, ORDINAL_FROM_DOMAINID3(KDmIdRoot, KDmIdTestA, KDmIdTestAB, KDmIdTestABA), this));
	test(iTestMembers[testMemberCount++] != NULL);
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestABB, ORDINAL_FROM_DOMAINID3(KDmIdRoot, KDmIdTestA, KDmIdTestAB, KDmIdTestABB), this));
	test(iTestMembers[testMemberCount++] != NULL);
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestCAA, ORDINAL_FROM_DOMAINID3(KDmIdRoot, KDmIdTestC, KDmIdTestCA, KDmIdTestCAA), this));
	test(iTestMembers[testMemberCount++] != NULL);


	//*************************************************
	// Test 6b change to current state
	//*************************************************
	test.Next(_L("Test 6b change to current state"));
	iTestState =  EStartupCriticalStatic;
	iTestDomainId = KDmIdRoot;

	iTransitionsCompleted = iTestNotifications = 0;
	iTestNotificationsExpected = testMemberCount;
	iTransitionsExpected = 1;

	// request a domain transition
	iTestDomainManager->RequestDomainTransition(iTestDomainId, iTestState, ETraverseDefault);

	// wait for test transitions to complete
	CActiveScheduler::Start();
	test(iStatus == KErrNone);
	test(iTestNotifications == iTestNotificationsExpected);
	

	// cancel a member notification request 
	//*************************************************
	// Test 6c cancel a member notification request
	//*************************************************
	test.Next(_L("Test 6c cancel a member notification request"));
	RDmDomain domainMember;
	TRequestStatus status;
	domainMember.Connect(GHierarchyIdUnderTest, iTestDomainId);
	domainMember.RequestTransitionNotification(status);
	domainMember.CancelTransitionNotification();
	User::WaitForRequest(status);
	domainMember.Close();

	//*************************************************
	// Test 6d cancel a member notification request without having first requested a notification
	//*************************************************
	test.Next(_L("Test 6d cancel a member notification request without having first requested a notification"));
	domainMember.Connect(GHierarchyIdUnderTest, iTestDomainId);
	domainMember.CancelTransitionNotification();
	domainMember.Close();

	//*************************************************
	// Test 6e domain controller adds invalid hierarchy
	//*************************************************
	test.Next(_L("Test 6e domain controller connects to invalid hierarchy"));
	r = RDmDomainManager::AddDomainHierarchy(TDmHierarchyId(-1));
	test(r == KErrBadHierarchyId);

	//*************************************************
	// Test 6f domain member connects to invalid hierarchy
	//*************************************************
	test.Next(_L("Test 6f domain member connects to invalid hierarchy"));
	r = domainMember.Connect(TDmHierarchyId(-1), TDmDomainId(KDmIdRoot));
	test (r == KErrBadHierarchyId);

	//*************************************************
	// Test 6g domain member connects to valid hierarchy but invalid domain
	//*************************************************
	test.Next(_L("Test 6g domain member connects to valid hierarchy but invalid domain"));
	r = domainMember.Connect(GHierarchyIdUnderTest, TDmDomainId(-1));
	test (r == KDmErrBadDomainId);

	delete iTestDomainManager;
	iTestDomainManager = NULL;

	// Panic tests

	//*************************************************
	// Test 6h request a transition notification without connecting first
	//*************************************************
	test.Next(_L("Test 6h request a transition notification without connecting first"));
	PanicTest(ENegTestTransitionNoConnect);

	//*************************************************
	// Test 6i Get the domain state without connecting
	//*************************************************
	test.Next(_L("Test 6i Get the domain state without connecting"));
	PanicTest(ENegTestGetStateNoConnect);

	//*************************************************
	// Test 6j request a transition notification with an invalid transition mode
	//*************************************************
	test.Next(_L("Test 6j request a transition notification with an invalid transition mode"));
	PanicTest(ENegTestTransitionInvalidMode);


	// cleanup

	CDomainMemberAo** mt;
	for (mt = iTestMembers; *mt; ++mt)
		delete *mt;

 	__UHEAP_MARKEND;
	}

// This handles a transition notification from a test domain member.
TInt CDmTest6::TransitionNotification(MDmDomainMember& aDomainMember)
	{
	TInt status = aDomainMember.Status();
		
	iTestNotifications++;

	test (aDomainMember.HierarchyId() == GHierarchyIdUnderTest);

	TBuf16<4> buf;
	GetDomainDesc(aDomainMember.Ordinal(), buf);

	test.Printf(_L("CDmTest6::TransitionNotification(), Hierarchy = %d, domain = %S, iOrdinal = 0x%08X, state = 0x%x, status = %d\n"), 
		aDomainMember.HierarchyId(), &buf, aDomainMember.Ordinal(), aDomainMember.State(), status);


	return KErrNone;
	}

void CDmTest6::RunL()
	{
	iTransitionsCompleted++;

	TInt error = iStatus.Int();

	test.Printf(_L("CDmTest6::RunL(), error = %d, iTestNotifications %d\n"), 
		error, iTestNotifications);

	if (iTransitionsCompleted == iTransitionsExpected)
		CActiveScheduler::Stop();
	}

void CDmTest6::TransitionRequestComplete()
	{
	iTransitionsCompleted++;

	TInt error = iStatus.Int();
	
	test.Printf(_L("CDmTest6::TransitionRequestComplete(), error = %d, iTestNotifications %d\n"), 
		error, iTestNotifications);
	
	if (iTransitionsCompleted == iTransitionsExpected)
		CActiveScheduler::Stop();
	}

void CDmTest6::DoCancel()
	{
	test(0);
	}

void CDmTest6::Release()
	{
	delete this;
	}

// Transition progress Observer testing
class CDmTest7 : public CActive, public MDmTest, public MHierarchyObserver
	{
public: 
	// from CActive
	void RunL();
 
	// from MDmTest
	void Perform();
	void Release();
	TInt TransitionNotification(MDmDomainMember& aDomainMember);
	void TransitionRequestComplete();

	// from MHierarchyObserver
	virtual void TransProgEvent(TDmDomainId aDomainId, TDmDomainState aState);
	virtual void TransFailEvent(TDmDomainId aDomainId, TDmDomainState aState, TInt aError);
	virtual void TransReqEvent(TDmDomainId aDomainId, TDmDomainState aState);



	CDmTest7(TDmDomainId aDomainId) : CActive(CActive::EPriorityStandard), iObservedDomainId(aDomainId) {}

protected:
	// from CActive
	virtual void DoCancel();

private:
	void TestForCompletion();


private:

	enum { KMembersMax = 16 };

	CDomainMemberAo*	iTestMembers[KMembersMax]; 
	CDomainManagerAo*	iTestDomainManager;
	
	TDmDomainId			iTestDomainId;
	TDmDomainState		iTestState;
	TDmDomainId			iObservedDomainId;

public:
	TInt				iTestNotifications;
	TInt				iTestNotificationsExpected;

	TInt				iTransitionsCompleted;
	TInt				iTransitionsExpected;

	TInt				iTransProgEvents;
	TInt				iTransFailEvents;
	TInt				iTransReqEvents;

	TInt				iTransProgEventsExpected;
	TInt				iTransFailEventsExpected;
	TInt				iTransReqEventsExpected;
	};

//! @SYMTestCaseID PBASE-T_DOMAIN-7
//! @SYMTestType CT
//! @SYMTestCaseDesc Transition progress Observer testing
//! @SYMREQ REQ3723
//! @SYMTestActions Various negative tests
//! @SYMTestExpectedResults  All tests should pass
//! @SYMTestPriority High
//! @SYMTestStatus Defined
void CDmTest7::Perform()
	{

 	__UHEAP_MARK;

	//
	// Test domain transitions with activated observer
	//
	CActiveScheduler::Add(this);

	TInt r = RDmDomainManager::AddDomainHierarchy(GHierarchyIdUnderTest);
	test(r == KErrNone);

	CDomainManagerAo* iTestDomainManager = NULL;
	TRAP_IGNORE(iTestDomainManager = CDomainManagerAo::NewL(GHierarchyIdUnderTest, *this));
	test (iTestDomainManager != NULL);

	r = CDomainManagerAo::AddDomainHierarchy(GHierarchyIdUnderTest);
	test(r == KErrNone);

	//*************************************************
	// Test 7a - Testing observer notifications
	//*************************************************
	
	test.Next(_L("Test 7a - Testing observer notifications"));

	TInt testMemberCount = 0;

	// Add some test hierarchy members
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdRoot, ORDINAL_FROM_DOMAINID0(KDmIdRoot), this));
	test(iTestMembers[testMemberCount++] != NULL);
	
	// row 1
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestA, ORDINAL_FROM_DOMAINID1(KDmIdRoot, KDmIdTestA), this));
	test(iTestMembers[testMemberCount++] != NULL);
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestB, ORDINAL_FROM_DOMAINID1(KDmIdRoot, KDmIdTestB), this));
	test(iTestMembers[testMemberCount++] != NULL);
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestC, ORDINAL_FROM_DOMAINID1(KDmIdRoot, KDmIdTestC), this));
	test(iTestMembers[testMemberCount++] != NULL);
	
	// row2
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestAA, ORDINAL_FROM_DOMAINID2(KDmIdRoot, KDmIdTestA, KDmIdTestAA), this));
	test(iTestMembers[testMemberCount++] != NULL);
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestAB, ORDINAL_FROM_DOMAINID2(KDmIdRoot, KDmIdTestA, KDmIdTestAB), this));
	test(iTestMembers[testMemberCount++] != NULL);
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestBA, ORDINAL_FROM_DOMAINID2(KDmIdRoot, KDmIdTestB, KDmIdTestBA), this));
	test(iTestMembers[testMemberCount++] != NULL);
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestCA, ORDINAL_FROM_DOMAINID2(KDmIdRoot, KDmIdTestC, KDmIdTestCA), this));
	test(iTestMembers[testMemberCount++] != NULL);
	
	// row 3
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestABA, ORDINAL_FROM_DOMAINID3(KDmIdRoot, KDmIdTestA, KDmIdTestAB, KDmIdTestABA), this));
	test(iTestMembers[testMemberCount++] != NULL);
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestABB, ORDINAL_FROM_DOMAINID3(KDmIdRoot, KDmIdTestA, KDmIdTestAB, KDmIdTestABB), this));
	test(iTestMembers[testMemberCount++] != NULL);
	TRAP(r, iTestMembers[testMemberCount] = CDomainMemberAo::NewL(GHierarchyIdUnderTest, KDmIdTestCAA, ORDINAL_FROM_DOMAINID3(KDmIdRoot, KDmIdTestC, KDmIdTestCA, KDmIdTestCAA), this));
	test(iTestMembers[testMemberCount++] != NULL);

	// create an observer
	CHierarchyObserver* observer = NULL;
	TRAP(r, observer = CHierarchyObserver::NewL(*this, GHierarchyIdUnderTest));
	test (r == KErrNone);
	test(observer != NULL);
	observer->StartObserver(iObservedDomainId, EDmNotifyAll);
	
	// request a state change
	iTestState =  EStartupCriticalDynamic;
	iTestDomainId = KDmIdRoot;
	iTransitionsCompleted = iTestNotifications = 0;
	iTestNotificationsExpected = testMemberCount;
	iTransitionsExpected = 1;

	iTransProgEvents = iTransFailEvents = iTransReqEvents = 0;
	
	iTransReqEventsExpected = iTransProgEventsExpected = observer->ObserverDomainCount();
	iTransFailEventsExpected = 0;


	iTestDomainManager->RequestDomainTransition(iTestDomainId, iTestState, ETraverseDefault);

	// wait for test transitions to complete
	CActiveScheduler::Start();
	test(iStatus == KErrNone);
	test(iTestNotifications == iTestNotificationsExpected);
	test (iTransProgEvents == iTransProgEventsExpected);
	test (iTransFailEvents == iTransFailEventsExpected);
	test (iTransReqEvents == iTransReqEventsExpected);


	// cleanup
	delete observer; 
	observer = NULL;

	//*************************************************
	// Test 7b - start & stop the observer
	//*************************************************
	test.Next(_L("Test 7b - start & stop the observer"));

	// create an observer, start it stop and then start it again
	TRAP(r, observer = CHierarchyObserver::NewL(*this, GHierarchyIdUnderTest));
	test (r == KErrNone);
	test(observer != NULL);
	observer->StartObserver(iObservedDomainId, EDmNotifyAll);
	observer->StopObserver();
	observer->StartObserver(iObservedDomainId, EDmNotifyAll);

	// request a state change
	iTestState++;
	iTestDomainId = KDmIdRoot;
	iTransitionsCompleted = iTestNotifications = 0;
	iTestNotificationsExpected = testMemberCount;
	iTransitionsExpected = 1;

	iTransProgEvents = iTransFailEvents = iTransReqEvents = 0;
	
	iTransProgEventsExpected = iTransReqEventsExpected = observer->ObserverDomainCount();
	iTransFailEventsExpected = 0;

	iTestDomainManager->RequestDomainTransition(iTestDomainId, iTestState, ETraverseDefault);

	// wait for test transitions to complete
	CActiveScheduler::Start();
	test(iStatus == KErrNone);
	test(iTestNotifications == iTestNotificationsExpected);
	test (iTransProgEvents == iTransProgEventsExpected);
	test (iTransFailEvents == iTransFailEventsExpected);
	test (iTransReqEvents == iTransReqEventsExpected);

	// stop the observer & request another state change
	observer->StopObserver();
	iTestState++;
	iTestDomainId = KDmIdRoot;
	iTransitionsCompleted = iTestNotifications = 0;
	iTestNotificationsExpected = testMemberCount;
	iTransitionsExpected = 1;

	iTransProgEvents = iTransFailEvents = iTransReqEvents = 0;
	
	iTransProgEventsExpected = 0;
	iTransFailEventsExpected = 0;
	iTransReqEventsExpected = 0;

	iTestDomainManager->RequestDomainTransition(iTestDomainId, iTestState, ETraverseDefault);
	// wait for test transitions to complete
	CActiveScheduler::Start();
	test(iStatus == KErrNone);
	test(iTestNotifications == iTestNotificationsExpected);
	test (iTransProgEvents == iTransProgEventsExpected);
	test (iTransFailEvents == iTransFailEventsExpected);
	test (iTransReqEvents == iTransReqEventsExpected);

	// Start the observer again on a different domain and only ask for transition requests
	// Then request another state change
	observer->StartObserver((iObservedDomainId == KDmIdRoot)?KDmIdTestCA:KDmIdRoot, EDmNotifyTransRequest);
	iTestState++;
	iTestDomainId = KDmIdRoot;
	iTransitionsCompleted = iTestNotifications = 0;
	iTestNotificationsExpected = testMemberCount;
	iTransitionsExpected = 1;

	iTransProgEvents = iTransFailEvents = iTransReqEvents = 0;
	
	iTransReqEventsExpected = observer->ObserverDomainCount();
	iTransProgEventsExpected = 0;
	iTransFailEventsExpected = 0;


	iTestDomainManager->RequestDomainTransition(iTestDomainId, iTestState, ETraverseDefault);
	// wait for test transitions to complete
	CActiveScheduler::Start();
	test(iStatus == KErrNone);
	test(iTestNotifications == iTestNotificationsExpected);
	test (iTransProgEvents == iTransProgEventsExpected);
	test (iTransFailEvents == iTransFailEventsExpected);
	test (iTransReqEvents == iTransReqEventsExpected);

	delete observer; 
	observer = NULL;

	//*************************************************
	// Test 7c - invalid arguments testing for observer
	//*************************************************
	test.Next(_L("Test 7c - Invalid arguments testing for observer"));
	
	const TDmHierarchyId	KDmHierarchyIdInvalid = 110;
	
	test.Printf(_L("Test 7c.1 - create observer with invalid hierarchy Id\n"));
	
	// create an observer
	TRAP(r, observer = CHierarchyObserver::NewL(*this, KDmHierarchyIdInvalid));
	test (r == KErrBadHierarchyId);
	
	
	test.Printf(_L("Test 7c.2 - Starting the observer with wrong domain Id\n"));
	TRAP(r, observer = CHierarchyObserver::NewL(*this, GHierarchyIdUnderTest));
	test (r == KErrNone);
	test(observer != NULL);

	//Wrong domain Id
	const TDmDomainId	KDmIdInvalid	= 0x0f;
	r= observer->StartObserver(KDmIdInvalid, EDmNotifyAll);
	test(r==KDmErrBadDomainId);

	test.Printf(_L("Test 7c.3 - Trying to create second observer on the same hierarchy\n"));
	TRAP(r, CHierarchyObserver::NewL(*this, GHierarchyIdUnderTest));
	test (r == KDmErrBadSequence);

	
	
	//*************************************************
	// Test 7d - Wrong sequence of API calls for observer
	//*************************************************
	test.Next(_L("Test 7d - Observer wrong sequence of calls"));
	
	test.Printf(_L("Test 7d.1 - Stopping Observer before starting it\n"));
	r = observer->StopObserver();
	test(r==KDmErrBadSequence);
	
	test.Printf(_L("Test 7d.2 - Starting Observer twice\n"));
	r= observer->StartObserver(KDmIdRoot, EDmNotifyAll);
	test(r==KErrNone);

	r= observer->StartObserver(KDmIdRoot, EDmNotifyAll);
	test(r==KDmErrBadSequence);

	
	delete observer;

	/***************************************/

	delete iTestDomainManager;
	iTestDomainManager = NULL;

	CDomainMemberAo** mt;
	for (mt = iTestMembers; *mt; ++mt)
		delete *mt;


	// restore the domain hierarchies to their initial state so as not to 
	// upset any subsequent tests which rely on this
	{
	RDmDomainManager manager;
	TRequestStatus status;
	TInt r = manager.Connect(GHierarchyIdUnderTest);
	test (r == KErrNone);
	manager.RequestDomainTransition(KDmIdRoot, EStartupCriticalStatic, ETraverseDefault, status);
	User::WaitForRequest(status);
	test(status.Int() == KErrNone);
	manager.Close();
	}

 	__UHEAP_MARKEND;
	}

// This handles a transition notification from a test domain member.
TInt CDmTest7::TransitionNotification(MDmDomainMember& aDomainMember)
	{
		
	iTestNotifications++;

	test (aDomainMember.HierarchyId() == GHierarchyIdUnderTest);

	TBuf16<4> buf;
	GetDomainDesc(aDomainMember.Ordinal(), buf);

	__PRINT((_L("CDmTest7::TransitionNotification(), Hierarchy = %d, domain = %S, iOrdinal = 0x%08X, state = 0x%x, status = %d\n"), 
		aDomainMember.HierarchyId(), &buf, aDomainMember.Ordinal(), aDomainMember.State(), aDomainMember.Status()));

	return KErrNone;
	}

void CDmTest7::RunL()
	{
	iTransitionsCompleted++;

	__PRINT((_L("CDmTest7::RunL(), error = %d, iTestNotifications %d\n"), 
		iStatus.Int(), iTestNotifications));

	TestForCompletion();
	}

void CDmTest7::TransitionRequestComplete()
	{
	iTransitionsCompleted++;

	__PRINT((_L("CDmTest7::TransitionRequestComplete(), error = %d, iTestNotifications %d\n"), 
		iStatus.Int(), iTestNotifications));
	
	TestForCompletion();
	}

void CDmTest7::DoCancel()
	{
	test(0);
	}

void CDmTest7::Release()
	{
	delete this;
	}

void CDmTest7::TestForCompletion()
	{

	if (iTransitionsCompleted == iTransitionsExpected &&
		iTransProgEvents == iTransProgEventsExpected && 
		iTransFailEvents == iTransFailEventsExpected &&
		iTransReqEvents == iTransReqEventsExpected)
		{
		CActiveScheduler::Stop();
		}
	}

#ifdef _DEBUG
void CDmTest7::TransProgEvent(TDmDomainId aDomainId, TDmDomainState aState)
#else
void CDmTest7::TransProgEvent(TDmDomainId /*aDomainId*/, TDmDomainState /*aState*/)
#endif
	{
	iTransProgEvents++;
	__PRINT((_L("CDmTest7::TransProgEvent(), aDomainId = %d, aState %d, iTransProgEvents %d\n"), 
		aDomainId, aState, iTransProgEvents));
	TestForCompletion();
	}

#ifdef _DEBUG
void CDmTest7::TransFailEvent(TDmDomainId aDomainId, TDmDomainState aState, TInt aError)
#else
void CDmTest7::TransFailEvent(TDmDomainId /*aDomainId*/, TDmDomainState /*aState*/, TInt /*aError*/)
#endif

	{
	iTransFailEvents++;
	__PRINT((_L("CDmTest7::TransFailEvent(), aDomainId = %d, aState %d aError %d, iTransFailEvents %d\n"), 
		aDomainId, aState, iTransFailEvents, aError));
	TestForCompletion();
	}

#ifdef _DEBUG
void CDmTest7::TransReqEvent(TDmDomainId aDomainId, TDmDomainState aState)
#else
void CDmTest7::TransReqEvent(TDmDomainId /*aDomainId*/, TDmDomainState /*aState*/)
#endif
	{
	iTransReqEvents++;
	__PRINT((_L("CDmTest7::TransReqEvent(), aDomainId = %d, aState %d, iTransReqEvents %d\n"), 
		aDomainId, aState, iTransReqEvents));
	TestForCompletion();
	}


/**
   Increase code coverage, in particular get CPowerUpHandler::DoCancel()
   called.
*/
class CDmPowerCoverageTest : public CActive, public MDmTest
	{
public:
	CDmPowerCoverageTest();
	~CDmPowerCoverageTest()
		{
		Cancel();
		iManager.Close();
		delete iMember;
		}
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
		{
		test(0);
		}
private:
	CDmTestMember* iMember;
	RDmDomainManager iManager;
	};


CDmPowerCoverageTest::CDmPowerCoverageTest()
	: CActive(CActive::EPriorityStandard)
	{}


void CDmPowerCoverageTest::RunL()
	{
	RDebug::Printf("CDmPowerCoverageTest::RunL(): %d", iStatus.Int());
	CActiveScheduler::Stop();
	}


void CDmPowerCoverageTest::Perform()
	{
	test.Next(_L("CDmPowerCoverageTest"));

	iMember = new CDmTestMember(KDmHierarchyIdPower, KDmIdApps, 0, this);
	test(iMember != NULL);

	TInt r = iManager.Connect();
	test(r == KErrNone);

	CActiveScheduler::Add(this);
	iManager.RequestSystemTransition(EPwStandby, iStatus);
	iManager.CancelTransition();
	CActive::SetActive();

	CActiveScheduler::Start();
	}


TInt CDmPowerCoverageTest::TransitionNotification(MDmDomainMember&)
	{
	RDebug::Printf("CDmPowerCoverageTest::TransitionNotification()");
	// Don't acknowledge
	return KErrAbort;
	}


void CDmPowerCoverageTest::Release()
	{
	delete this;
	}


/**
Test disconnecting domain controller from server whilst
transition in progress
*/
class CDmTestDisconnect : public CBase, public MDmTest
	{
public:
	~CDmTestDisconnect();

	// from MDmTest
	void Perform();
	void Release();
	TInt TransitionNotification(MDmDomainMember& aDomainMember);
	void TransitionRequestComplete() {test(EFalse);}

protected:
	CDmTestMember*		iMember;
	RDmDomainManager	iManager;
	};

CDmTestDisconnect::~CDmTestDisconnect()
	{
	delete iMember;
	iManager.Close();
	}

void CDmTestDisconnect::Perform()
	{
	TInt r = RDmDomainManager::AddDomainHierarchy(GHierarchyIdUnderTest);
	test_KErrNone(r);
	test.Next(_L("Test disconnecting controller during transition"));
	iMember = new (ELeave) CDmTestMember(GHierarchyIdUnderTest, KDmIdTestAB, ORDINAL_FROM_DOMAINID2(KDmIdRoot, KDmIdTestA, KDmIdTestAB), this);
	r = iManager.Connect(GHierarchyIdUnderTest);
	test_KErrNone(r);

	test_Equal(0, RThread().RequestCount());
	TRequestStatus status;
	iManager.RequestDomainTransition(KDmIdRoot, EStartupCriticalStatic, ETraverseDefault, status);
	CActiveScheduler::Start();

	// No User::WaitForRequest is used, since it is expected that
	// the outstanding request will not be completed
	test_Equal(0, RThread().RequestCount());
	}

TInt CDmTestDisconnect::TransitionNotification(MDmDomainMember&)
	{
	iManager.Close();
	CActiveScheduler::Stop();
	return KErrNone;
	}

void CDmTestDisconnect::Release()
	{
	delete this;
	}

void RunTests(TInt aIter)
	{
	while (aIter--)
		{
		MDmTest* tests[] = 
			{
			new CDmTestDisconnect(),
			new CDmTest1(KDmIdRoot, EPwStandby),
			new CDmTest1(KDmIdRoot, EPwOff),
			new CDmTest1(KDmIdRoot, EPwActive),
			new CDmTest1(KDmIdApps, EPwStandby),
			new CDmTest1(KDmIdApps, EPwOff),
			new CDmTest1(KDmIdApps, EPwActive),
			new CDmTest1(KDmIdUiApps, EPwStandby),
			new CDmTest1(KDmIdUiApps, EPwOff),
			new CDmTest1(KDmIdUiApps, EPwActive),
			new CDmTest2(EPwStandby),
			new CDmTest3(),
	
			// platform security tests
			new CDmTest4(),
	
			// PREQ810 tests :
			// note that we use a fictitious power state to prevent any 
			new CDmTest5(KDmIdRoot, KDmIdRoot, EPwActive+10, EStartupCriticalDynamic),
			new CDmTest5(KDmIdUiApps, KDmIdTestAB, EPwActive+10, EStartupCriticalDynamic),
	
		    // negative tests
			new CDmTest6(),
	
	
			// observer tests
	 		new CDmTest7(KDmIdTestA),
			new CDmTest7(KDmIdRoot),
			
			// increase code coverage
			new CDmPowerCoverageTest(),
			};
	
		for (unsigned int i = 0; i < sizeof(tests)/sizeof(*tests); ++i)
			{
			test(tests[i] != NULL);
			tests[i]->Perform();
			tests[i]->Release();
			}
		}
	}


GLDEF_C TInt E32Main()
	{
	CTrapCleanup* trapHandler=CTrapCleanup::New();
	test(trapHandler!=NULL);

	CActiveScheduler* scheduler = new CActiveScheduler();
	test(scheduler != NULL);
	CActiveScheduler::Install(scheduler);

	// Turn off evil lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	//
	// Perform the number of iterations specifed by the command line argument.
	//
	// If no arguments - perform one iteration
	//
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
	test.Printf(_L("Go for %d iterations\n"), iter);

	// Remember the number of open handles. Just for a sanity check ....
	TInt start_thc, start_phc;
	RThread().HandleCount(start_phc, start_thc);


	test.Start(_L("Test run with original test Hierarchy"));	
	GHierarchyIdUnderTest = KDmHierarchyIdTest;
	RunTests(iter);
	test.End();
	
	
	test.Start(_L("Test run with original test Hierarchy as V2 policy"));	
	GHierarchyIdUnderTest = KDmHierarchyIdTestV2_97;
	RunTests(iter);
	test.End();

	test.Start(_L("Test run with original test Hierarchy as V2 policy, but NULL state specification"));	
	GHierarchyIdUnderTest = KDmHierarchyIdTestV2_96;
	RunTests(iter);
	test.End();

	// Sanity check for open handles and for pending requests ...
	TInt end_thc, end_phc;
	RThread().HandleCount(end_phc, end_thc);
	test(start_thc == end_thc);
	test(start_phc == end_phc);
	test(RThread().RequestCount() >= 0);

	delete scheduler;
	delete trapHandler;

	return KErrNone;
	}
