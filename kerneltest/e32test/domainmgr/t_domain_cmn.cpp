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
// Framework code for Domain Manager tests.
//

#include <e32base.h>
#define __E32TEST_EXTENSION__
#include <e32test.h>

#include "domainpolicytest.h"
#include "t_domain.h"


extern RTest test;

// get the least significant domain id character (for debugging purposes)
TBool GetDomainChar(TDmDomainId aDomainId, TChar& aChar)
	{
	TBool found = ETrue;
	switch(aDomainId)
		{
		case KDmIdTestA:	aChar = 'A'; break;
		case KDmIdTestB:	aChar = 'B'; break;
		case KDmIdTestC:	aChar = 'C'; break;
		case KDmIdTestAA:	aChar = 'A'; break;
		case KDmIdTestAB:	aChar = 'B'; break;
		case KDmIdTestBA:	aChar = 'A'; break;
		case KDmIdTestCA:	aChar = 'A'; break;
		case KDmIdTestABA:	aChar = 'A'; break;
		case KDmIdTestABB:	aChar = 'B'; break;
		case KDmIdTestCAA:	aChar = 'A'; break;
		// domain char not found
		case KDmIdNone:
		case KDmIdRoot:
		default:
			found = EFalse;
		}
	return found;
	}


// prints the 4-character domain string into the passed descriptor (for debugging purposes)
// e.g. "CAA" for KDmIdTestCAA
void GetDomainDesc(TUint32 aOrdinal, TDes& aDes)
	{
	if (aOrdinal == KDmIdRoot)
		{
		aDes.Append(_L("root"));
		return;
		}

	TUint32 val =  aOrdinal;

	for (TInt n=0; n<4; n++)
		{
		TDmDomainId domainId = (TDmDomainId) (val >> 24);
		TChar ch;
		TBool found = GetDomainChar(domainId, ch);
		if (found)
			aDes.Append(ch);
		val = val << 8;
		}
	}


CDmTestMember::CDmTestMember(TDmHierarchyId aHierarchy, TDmDomainId aId,
							 TUint32 aOrdinal, MDmTest* aTest)
  : CActive(CActive::EPriorityStandard),
	iHierarchy(aHierarchy), iId(aId), iOrdinal(aOrdinal), iTest(aTest)
	{
	TInt r;

	if (iHierarchy == KDmHierarchyIdPower)
		 r = iDomain.Connect(iId);
	else
		 r = iDomain.Connect(iHierarchy, iId);

	test(r == KErrNone);

	CActiveScheduler::Add(this);

	iDomain.RequestTransitionNotification(CActive::iStatus);
	CActive::SetActive();
	}

CDmTestMember::~CDmTestMember()
	{
	CActive::Cancel();
	iDomain.Close();
	}

void CDmTestMember::Acknowledge()
	{
	iDomain.AcknowledgeLastState();
	}

void CDmTestMember::RunL()
	{
	RDebug::Printf("CDmTestMember::RunL(): %d", iStatus.Int());
	iNotifications++;

	if (iHierarchy == KDmHierarchyIdPower)
		{
		iState = (TDmDomainState) iDomain.GetPowerState();
		}
	else
		{
		iState = iDomain.GetState();
		}
		
	TInt ackError = iTest->TransitionNotification(*this);
	if (ackError == KErrNone)
		{
		iDomain.AcknowledgeLastState();
		}
	else if (ackError == KErrAbort)
		{
		; // don't acknowledge
		}
	else
		{
		iDomain.AcknowledgeLastState(ackError);
		}
	// request another notification (even if we didn't acknowledge the last one)
	iDomain.RequestTransitionNotification(CActive::iStatus);
	CActive::SetActive();
	}

void CDmTestMember::DoCancel()
	{
	iDomain.CancelTransitionNotification();
	}


CDomainMemberAo* CDomainMemberAo::NewL(TDmHierarchyId aHierarchy, TDmDomainId aId,
									   TUint32 aOrdinal, MDmTest* aTest)
	{
	CDomainMemberAo* self=new (ELeave) CDomainMemberAo(aHierarchy, aId, aOrdinal, aTest);
	CleanupStack::PushL(self);
	self->ConstructL();

	self->RequestTransitionNotification();

	CleanupStack::Pop();
	return self;
	}

CDomainMemberAo::CDomainMemberAo(TDmHierarchyId aHierarchy, TDmDomainId aId, TUint32 aOrdinal,
								 MDmTest* aTest)
	: CDmDomain(aHierarchy, aId),
	  iHierarchy(aHierarchy), iId(aId), iOrdinal(aOrdinal), iTest(aTest)
	{
	}

CDomainMemberAo::~CDomainMemberAo()
	{
	Cancel();
	}

void CDomainMemberAo::RunL()
	{
	RDebug::Printf("CDomainMemberAo::RunL(): %d", iStatus.Int());
	iNotifications++;

	iState = GetState();

	TInt ackError = iTest->TransitionNotification(*this);
	if (ackError == KErrNone)
		AcknowledgeLastState(ackError);
	else if (ackError == KErrAbort)	// don't acknowledge
		;
	else
		AcknowledgeLastState(ackError);
	if (ackError != KErrAbort)
		AcknowledgeLastState(ackError);

	// request another notification (even if we didn't acknowledge the last one)
	RequestTransitionNotification();
	}


CDomainManagerAo* CDomainManagerAo::NewL(TDmHierarchyId aHierarchy, MDmTest& aTest)
	{
	CDomainManagerAo* self=new (ELeave) CDomainManagerAo(aHierarchy, aTest);
	CleanupStack::PushL(self);

	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}

CDomainManagerAo::CDomainManagerAo(TDmHierarchyId aHierarchy, MDmTest& aTest) :
	CDmDomainManager(aHierarchy), iTest(aTest)
	{
	}

CDomainManagerAo::~CDomainManagerAo()
	{
	}

void CDomainManagerAo::RunL()
	{
	RDebug::Printf("CDomainManagerAo::RunL(): %d", iStatus.Int());
	iTest.TransitionRequestComplete();
	}


//
// Deferral Test code
//

//
// CDmDeferralTest
//

CDmDeferralTest::CDmDeferralTest(TDmHierarchyId aId, TDmDomainState aState)
	: CActive(CActive::EPriorityStandard), iHierarchyId(aId), iState(aState)
	{
	CActiveScheduler::Add(this);
	}

CDmDeferralTest::~CDmDeferralTest()
	{
	iManager.Close();
	delete iMember;
	}

/**
Basic test setup : load and connect to appropriate hierarchy
*/
void CDmDeferralTest::Perform()
	{
	TInt r = RDmDomainManager::AddDomainHierarchy(iHierarchyId);
	test_KErrNone(r);

	r = iManager.Connect(iHierarchyId);
	test_KErrNone(r);

	DoPerform();

	CActive::SetActive();
	CActiveScheduler::Start();
	}

void CDmDeferralTest::Release()
	{
	delete this;
	}

/**
aDomainMember has recieved a transition notification
*/
TInt CDmDeferralTest::TransitionNotification(MDmDomainMember& aDomainMember)
	{
	test.Printf(_L("MDmDomainMember notified of transition, Domain ID=%d"),
				aDomainMember.DomainId());
	return KErrNone;
	}

/**
A transition has completed
*/
void CDmDeferralTest::TransitionRequestComplete()
	{}

void CDmDeferralTest::RunL()
	{
	RDebug::Printf("CDmDeferralTest::RunL(): %d", iStatus.Int());
	TransitionRequestComplete();
	CActiveScheduler::Stop();
	}

void CDmDeferralTest::DoCancel()
	{
	test(0);
	}

//
//
// CTestKeepAlive
//


CTestKeepAlive::CTestKeepAlive(RDmDomain& aDomain)
	: CActive(CActive::EPriorityHigh), iDomain(aDomain), iCount(0)
	{
	CActiveScheduler::Add(this);
	}

CTestKeepAlive::~CTestKeepAlive()
	{
	Cancel();
	}

/**
Begin deferrals

@param aMember The object to notify when deferrals cease
@param aDeferralCount Number of times to defer
*/
void CTestKeepAlive::BeginDeferrals(MDeferringMember* aMember, TInt aDeferralCount)
	{
	iDeferringMember = aMember;
	iCount = aDeferralCount;

	if(iCount > 0)
		DeferAcknowledgement();
	}

void CTestKeepAlive::DeferAcknowledgement()
	{
	test.Printf(_L("DeferAcknowledgement() iCount %d\n"), iCount);
	iDomain.DeferAcknowledgement(iStatus);
	SetActive();
	}

void CTestKeepAlive::RunL()
	{
	RDebug::Printf("CTestKeepAlive::RunL(): %d", iStatus.Int());
	test(iCount>0);
	--iCount;
	TInt err = iStatus.Int();

	if((iCount == 0) || (KErrNone != err))
		{
		iDeferringMember->HandleEndOfDeferrals(err);
		iDeferringMember = NULL;
		}
	else
		{
		DeferAcknowledgement();
		}
	}

void CTestKeepAlive::DoCancel()
	{
	iDomain.CancelDeferral();
	}
