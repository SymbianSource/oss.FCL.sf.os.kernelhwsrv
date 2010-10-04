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
// domainmgr/src/domainsrv.cpp
//
//

#include <e32debug.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32property.h>
#include <f32file.h>

#include <domainpolicy.h>
#include "domainsrv.h"
#include "domaincfg.h"

#define __DS_PANIC(aError) User::Panic(_L("domainSrv.cpp"), (-(aError)) | (__LINE__ << 16))
#define __DS_PANIC_IFERR(aError) ((aError == KErrNone) || (User::Panic(_L("domainSrv.cpp"), (-(aError)) | (__LINE__ << 16)), 1))
#define __DS_PANIC_IFNUL(aPtr) ((aPtr != NULL) || (User::Panic(_L("domainSrv.cpp"), (-(KErrNoMemory)) | (__LINE__ << 16)), 1))
#define __DS_ASSERT(aCond) ((aCond) || (User::Panic(_L("domainSrv.cpp; assertion failed"), __LINE__), 1))
#define __DS_ASSERT_STARTUP(aCond) ((aCond) || (User::Panic(_L("Domain Server start-up error, server exiting"), __LINE__), 1))

static _LIT_SECURITY_POLICY_PASS(KAllowAllPolicy);
static _LIT_SECURITY_POLICY_C1(KPowerMgmtPolicy, ECapabilityPowerMgmt);
static _LIT_SECURITY_POLICY_C1(KWddPolicy, ECapabilityWriteDeviceData);
static _LIT_SECURITY_POLICY_C1(KProtServPolicy, ECapabilityProtServ);

// forward refs
class CSvrDomain;
class CDmHierarchy;
class CPowerUpHandler;
class CDmHierarchyPower;
class CDmSvrManager;
class CDmDomainServer;
class CDmDomainSession;
class CDmManagerServer;
class CDmManagerSession;


#if defined(_DEBUG) || defined(__DS_DEBUG)
void GetClientNameFromMessageL(const RMessagePtr2& aMessage, TDes& aBuffer)
	{
	RThread clientThread;
	aMessage.ClientL(clientThread);
	aBuffer = clientThread.FullName();
	clientThread.Close();
	}
#endif

// CSvrDomain
class CSvrDomain : public CTimer
	{
public: 
	static CSvrDomain* New(CDmHierarchy& aHierarchy, const TDmDomainSpec&);
	~CSvrDomain();

	// from CTimer
	void RunL();

	void Attach(CDmDomainSession*);
	void Detach(CDmDomainSession*);
	void AddChild(CSvrDomain*);
	CSvrDomain* Lookup(TDmDomainId);
	TBool CheckPropValue(TInt aPropValue);
	void RequestDomainTransition();
	void CompleteMemberTransition(TInt aError);
	void CancelTransition();
	void SetObserver(TBool aSet);
	TDmDomainState State();

private:
	CSvrDomain(CDmHierarchy& aHierarchy, const TDmDomainSpec*);
	void Construct(const TDmDomainSpec* spec);

	void RequestMembersTransition();
	void RequestChildrenTransition();
	void MembersTransitionDone();
	void ChildrenTransitionDone();
	void CompleteDomainTransition();

	void SetMemberDeferralBudgets();
	TBool ExpireMemberDeferrals();

private:
	CDmHierarchy&		iHierarchy;
	CSvrDomain*			iParent;
	CSvrDomain*			iPeer;
	CSvrDomain*			iChild;
	RProperty			iProperty;
	CDmDomainSession*	iSessions;
	TUint16				iChildrenCount;
	TUint16				iTransCount;

	TOverrideableSetting<TTimeIntervalMicroSeconds32, &CHierarchySettings::GetDomainTimeout>
		iTransTimeBudget;


	TOverrideableSetting<TInt, &CHierarchySettings::GetDeferralBudget>
		iTransitionDeferralBudget;

public:
	const TSecurityPolicy	iJoinPolicy;
	TBool iIsObserved;
	TDmDomainId			iId;
	};


// CDmHierarchy
class CDmHierarchy : public CBase
	{
public:
	~CDmHierarchy();

	static CDmHierarchy* New(TDmHierarchyId aHierarchyId, TDmHierarchyPolicy& aPolicy);

	CSvrDomain* LookupDomain(TDmDomainId aDomainId);
	TInt RequestDomainTransition(TDmDomainId, TDmDomainState aTargetState, TDmTraverseDirection aTraverseDirection, const RMessage2* aMessage);
	void RequestTransition(const RMessage2* aMessage);
	TInt StartObserver( TDmDomainId aDomainId,TDmNotifyType aNotifyType);
	void SetNotifyMessage(const RMessage2* aMessage);
	void CompleteNotification(TInt aError);
	TBool OutstandingNotification();
	void StopObserver();
	virtual TInt RequestSystemTransition(TDmDomainState aTargetState, TDmTraverseDirection aTraverseDirection, const RMessage2* aMessage);
	virtual void CompleteTransition(TInt aError);
	virtual void NotifyCompletion(TInt aReason);

	CHierarchySettings& HierachySettings()
		{
		return *iSettings;
		}

protected:
	CDmHierarchy(TDmHierarchyId aHierarchyId, TDmHierarchyPolicy& aPolicy);
	void SetState(TDmDomainState aTargetState, TDmTraverseDirection aTraverseDirection = ETraverseDefault);

private:
	CSvrDomain*		iObservedDomain;

	CHierarchySettings* iSettings;

	/** Direction of traversal if target state is after current state */
	TDmTraverseDirection iPositiveTransitions;

	/**	Direction of traversal if target state is before current state */
	TDmTraverseDirection iNegativeTransitions;

public:
	/** Policy which outlines the action upon transition failure */
	TOverrideableSetting<TDmTransitionFailurePolicy, &CHierarchySettings::GetFailurePolicy>
		iFailurePolicy;

	TDmHierarchyId	iHierarchyId;
	CSvrDomain*		iRootDomain;
	CSvrDomain*		iTransDomain;
	TInt			iTransPropValue;
	TDmDomainState	iTransState;
	TDmTraverseDirection	iTraverseDirection;
	TUint8			iTransId;
	CDmManagerSession* iControllerSession;	// only one controller per hierarchy


	RArray<TTransitionFailure> iTransitionFailures;

	// observer stuff
	TBool			iObserverStarted;
	TDmNotifyType	iNotifyType;
	RArray<TTransInfo>	iTransitions;
	CDmManagerSession* iObserverSession;	// only one observer per hierarchy
	TInt			iObservedChildren;
	};


// CPowerUpHandler
// Active object used to receive power-up notifications 
// from the Kernel-level power manager
class CPowerUpHandler : public CActive
	{
public: 
	static CPowerUpHandler* New(CDmHierarchyPower& aHierarchyPower);
	
	// from CActive
	void RunL();
	void DoCancel();

	void RequestWakeupEventNotification();
	void Cancel();

private:
	CPowerUpHandler(CDmHierarchyPower& aHierarchyPower);
	void Construct();

private:
	CDmHierarchyPower& iHierarchyPower;
	};


// CDmHierarchyPower
// CDmHierarchy-derived class 
// Interfaces to the Kernel-level power manager
class CDmHierarchyPower : public CDmHierarchy
	{
public:
	static CDmHierarchyPower* New(TDmHierarchyId aHierarchyId, TDmHierarchyPolicy& aPolicy);

	// from CDmHierarchy
	virtual TInt RequestSystemTransition(TDmDomainState, TDmTraverseDirection aTraverseDirection, const RMessage2* aMessage);
	virtual void CompleteTransition(TInt aError);
	virtual void NotifyCompletion(TInt aReason);

	void PowerUp();	// called from CPowerUpHandler

private:
	CDmHierarchyPower(TDmHierarchyId aHierarchyId, TDmHierarchyPolicy& aPolicy);
	void Construct();

private:
	enum 
		{
		EPoweringDown	= 0x01,
		};
	TUint8			iTransStatus;
	CPowerUpHandler* iPowerUpHandler;
	};


// CDmSvrManager
class CDmSvrManager : public CBase
	{
public:
	static CDmSvrManager* New();

	TInt BuildDomainTree(TDmHierarchyId aHierarchyId);
	CDmHierarchy* LookupHierarchy(TDmHierarchyId aHierarchyId);
	TInt LookupDomain(TDmHierarchyId aHierarchyId, TDmDomainId aDomainId, CSvrDomain*& aDomain);

private:
	CDmSvrManager();
	void Construct();

	TInt LoadStateSpecs(RLibrary& aLib, CDmHierarchy* aHierarchy);

private:
	RPointerArray<CDmHierarchy> iDomainHierarchies;
	};

// CDmDomainServer
class CDmDomainServer : public CServer2
	{
public: 
	// from CServer2
	CSession2* NewSessionL(const TVersion& aVer, const RMessage2& aMessage) const;

	// Note, tracing of client thread names relies upon
	// EUnsharableSessions being used
	CDmDomainServer(CDmSvrManager* aManager) : CServer2(CActive::EPriorityStandard, EUnsharableSessions), iManager(aManager)
		{}

public:
	CDmSvrManager*	iManager;
	};

// CDmDomainSession
class CDmDomainSession : public CSession2
	{
public: 
	// from CBase
	CDmDomainSession();
	~CDmDomainSession();

	// from CSession2
	void ServiceL(const RMessage2& aMessage);

	// Called by CSvrDomain
	void SetDeferralBudget(TInt);
	TBool DeferralActive() const;
	void ExpireDeferral();
	void CancelDeferral();

private:
	// Handle client calls
	void DeferAcknowledgment(const RMessage2&);
	void CompleteDeferral(TInt);
	void ObsoleteDeferral();

	CSvrDomain*			iDomain;
	TInt				iDeferralsRemaining;
	RMessagePtr2		iDeferralMsg;


public:
	CDmDomainSession*	iNext;
	TUint8				iAcknPending; ///< Indicates if an acknowledgment is pending
	TBool				iNotificationEnabled;

	};

class CDmManagerServer : public CServer2
	{
public: 
	// from CServer2
	CSession2* NewSessionL(const TVersion& aVer, const RMessage2&) const;

	// Note, tracing of client thread names relies upon
	// EUnsharableSessions being used
	CDmManagerServer(CDmSvrManager* aManager) : CServer2(CActive::EPriorityStandard, EUnsharableSessions), iManager(aManager)
		{}
	CDmSvrManager*	iManager;
	};

class CDmManagerSession : public CSession2
	{
public: 
	// from CBase
	CDmManagerSession();
	~CDmManagerSession();
	
	// from CSession2
	void ServiceL(const RMessage2& aMessage);

	RMessagePtr2	iTransMessagePtr;
	RMessagePtr2	iObsvrMessagePtr;

private:
	CDmHierarchy* iHierarchy;	// not owned
	};




//*********************************************************
// TTransitionFailure
//*********************************************************
/**
@internalTechnology

Constructor for transition failure info.

@param aDomainID	Id of the domain of interest
@param aError		error code of transition	 
*/
TTransitionFailure::TTransitionFailure(TDmDomainId aDomainId, TInt aError) :
	iDomainId(aDomainId), iError(aError)
	{
	}

//*********************************************************
// TTransInfo
//*********************************************************

/**
@internalTechnology

Constructor for transition failure info.

@param aDomainID	Id of the domain of interest
@param aState		State of the domain after transition
@param aError		error code of transition	 
*/
TTransInfo::TTransInfo(TDmDomainId aDomainId, TDmDomainState aState, TInt aError) :
	iDomainId(aDomainId), iState(aState), iError(aError)
	{
	}

//*********************************************************
// CSvrDomain
//*********************************************************


CSvrDomain::CSvrDomain(CDmHierarchy& aHierarchy, const TDmDomainSpec* spec)
	:	CTimer(CActive::EPriorityStandard), 
		iHierarchy(aHierarchy),
		iParent(NULL),
		iPeer(NULL),
		iChild(NULL),
		iChildrenCount(0),
		iTransTimeBudget(TTimeIntervalMicroSeconds32(spec->iTimeBudgetUs), &iHierarchy.HierachySettings()),
		iTransitionDeferralBudget(0, &iHierarchy.HierachySettings()),
		iJoinPolicy(spec->iJoinPolicy),
		iId(spec->iId)
	{
	__DS_TRACE((_L("DM: CSvrDomain() @0x%08x, id 0x%x, hierachy id %d"), this, iId, iHierarchy.iHierarchyId));
	}

CSvrDomain* CSvrDomain::New(CDmHierarchy& aHierarchy, const TDmDomainSpec& aSpec)
	{
	CSvrDomain* self = new CSvrDomain(aHierarchy, &aSpec);
	__DS_PANIC_IFNUL(self);

	self->Construct(&aSpec);
	return self;
	}

CSvrDomain::~CSvrDomain()
	{
	__DS_TRACE((_L("DM: ~CSvrDomain() @0x%08x"), this));

	// delete children
	CSvrDomain* child = iChild;
	while(child)
		{
		CSvrDomain* nextChild = child->iPeer;
		delete child;
		child = nextChild;
		iChildrenCount--;
		}
	__DS_ASSERT(iChildrenCount==0);

	TInt r = iProperty.Delete(DmStatePropertyKey(iHierarchy.iHierarchyId, iId));
	__DS_PANIC_IFERR(r);
	}

void CSvrDomain::Construct(const TDmDomainSpec* spec)
	{
	TInt r = iProperty.Define(
		KUidDmPropertyCategory, 
		DmStatePropertyKey(iHierarchy.iHierarchyId, iId), 
		RProperty::EInt,
		KAllowAllPolicy,KPowerMgmtPolicy);

	__DS_PANIC_IFERR(r);
	
	r = iProperty.Attach(KUidDmPropertyCategory, DmStatePropertyKey(
		iHierarchy.iHierarchyId, 
		iId));

	__DS_PANIC_IFERR(r);

	r = iProperty.Set(DmStatePropertyValue(0, spec->iInitState));
	__DS_PANIC_IFERR(r);

	TRAP(r, CTimer::ConstructL());
	__DS_PANIC_IFERR(r);

	CActiveScheduler::Add(this);
	}

void CSvrDomain::Attach(CDmDomainSession* aSession)
	{
	aSession->iNext = iSessions;
	iSessions = aSession;
	}

void CSvrDomain::Detach(CDmDomainSession* aSession)
	{
	CDmDomainSession** prevp = &iSessions;
	while (*prevp != aSession)
		{
		prevp = &((*prevp)->iNext);
		__DS_ASSERT(*prevp);
		}
	*(prevp) = aSession->iNext;
	}

void CSvrDomain::AddChild(CSvrDomain* aChild)
	{
	++iChildrenCount;
	aChild->iParent = this;
	if(iIsObserved)
		aChild->iIsObserved=ETrue;
	// Insert the child in the list of its peers
	aChild->iPeer = iChild;
	iChild = aChild;
	}

CSvrDomain* CSvrDomain::Lookup(TDmDomainId aDomainId)
	{
	if (iId == aDomainId)
		return this;

	CSvrDomain* child = iChild;
	while (child)
		{
		CSvrDomain* domain = child->Lookup(aDomainId);
		if (domain)
			return domain;
		child = child->iPeer;
		}
	return NULL;
	}

TBool CSvrDomain::CheckPropValue(TInt aPropValue)
	{ return iHierarchy.iTransPropValue == aPropValue; }

void CSvrDomain::RequestMembersTransition()
	{
	__DS_TRACE((_L("DM: CSvrDomain::RequestMembersTransition() hierarchy=%d, domain=0x%x"), iHierarchy.iHierarchyId, iId));
	__DS_ASSERT(iTransCount == 0);

	SetMemberDeferralBudgets();

	for(CDmDomainSession* s = iSessions; s; s = s->iNext)
		if (s->iNotificationEnabled)
			{
			++iTransCount;
			s->iAcknPending = ETrue;
			// notifications will be disabled until the client makes another 
			// call to RDmDomain::RequestTransitionNotification()
			s->iNotificationEnabled = EFalse;
			}

	if(iIsObserved)
		{
		if((iHierarchy.iNotifyType&EDmNotifyTransRequest)==EDmNotifyTransRequest)
			{
			TTransInfo transInfo(iId,State(),KDmErrOutstanding);
			iHierarchy.iTransitions.Append(transInfo);
			if(iHierarchy.OutstandingNotification())
					iHierarchy.CompleteNotification(KErrNone);	
			}
		}
	if (iTransCount > 0)
		CTimer::After(iTransTimeBudget());
	iProperty.Set(iHierarchy.iTransPropValue);
	if (iTransCount == 0)
		MembersTransitionDone();
	}


void CSvrDomain::RequestChildrenTransition()
	{
	__DS_TRACE((_L("DM: CSvrDomain::RequestChildrenTransition() hierarchy=%d, domain=0x%x"), iHierarchy.iHierarchyId, iId));
	__DS_ASSERT(iTransCount == 0);
	iTransCount = iChildrenCount;
	if (iTransCount)
		{
		CSvrDomain* child = iChild;
		__DS_ASSERT(child);
		do	{
			child->RequestDomainTransition();
			child = child->iPeer;
			}
		while(child);
		}
	else
		ChildrenTransitionDone();
	}

void CSvrDomain::RequestDomainTransition()
	{
	__DS_TRACE((_L("DM: CSvrDomain::RequestDomainTransition() hierarchy=%d, domain=0x%x state=0x%x prop=0x%x"), 
						iHierarchy.iHierarchyId, iId, iHierarchy.iTransState, iHierarchy.iTransPropValue));
	__DS_ASSERT(iTransCount == 0);
	if (iHierarchy.iTraverseDirection == ETraverseChildrenFirst)
		RequestChildrenTransition();
	else
		RequestMembersTransition();
	}
		
void CSvrDomain::MembersTransitionDone()
	{
	__DS_TRACE((_L("DM: CSvrDomain::MembersTransitionDone() hierarchy=%d, domain=0x%x"), iHierarchy.iHierarchyId, iId));
	__DS_ASSERT(iTransCount == 0);
	if (iHierarchy.iTraverseDirection == ETraverseChildrenFirst)
		CompleteDomainTransition();
	else
		RequestChildrenTransition();
	}

void CSvrDomain::ChildrenTransitionDone()
	{
	__DS_TRACE((_L("DM: CSvrDomain::ChildrenTransitionDone() hierarchy=%d, domain=0x%x"), iHierarchy.iHierarchyId, iId));
	__DS_ASSERT(iTransCount == 0);
	if (iHierarchy.iTraverseDirection == ETraverseChildrenFirst)
		RequestMembersTransition();
	else
		CompleteDomainTransition();
	}

void CSvrDomain::CompleteMemberTransition(TInt aError)
	{
	__DS_TRACE((_L("DM: CSvrDomain::CompleteMemberTransition() hierarchy=%d, domain=0x%x, aError = %d"), iHierarchy.iHierarchyId, iId, aError));
	__DS_ASSERT(iTransCount);

	if (aError)
		{
		// Add a transition failure to the array
		TTransitionFailure failure(iId, aError);
		iHierarchy.iTransitionFailures.Append(failure);
		
		if(iIsObserved)
			{
				if((iHierarchy.iNotifyType&EDmNotifyFail)==EDmNotifyFail)
				{
				TTransInfo transInfo(iId,State(),aError);
				iHierarchy.iTransitions.Append(transInfo);
				if(iHierarchy.OutstandingNotification())
					iHierarchy.CompleteNotification(KErrNone);
				}
			}
		// examine the failure policy to work out what to do
		if (iHierarchy.iFailurePolicy() == ETransitionFailureStop)
			{
			iHierarchy.CompleteTransition(aError);
			return;
			}
		}
	else if(iIsObserved)
			{
				if((iHierarchy.iNotifyType&EDmNotifyPass) == EDmNotifyPass)
				{
				TTransInfo transInfo(iId,State(),aError);
				iHierarchy.iTransitions.Append(transInfo);
				if(iHierarchy.OutstandingNotification())
					iHierarchy.CompleteNotification(KErrNone);
				}
			}

	if (--iTransCount == 0)
		{
		CTimer::Cancel();
		MembersTransitionDone();
		}
	}

void CSvrDomain::RunL()
	{ // Timer expired 

	if (ExpireMemberDeferrals())
		{
		__DS_TRACE((_L("DM: CSvrDomain::RunL() Deferring transition timeout hierarchy=%d, domain=0x%x"), iHierarchy.iHierarchyId, iId));
		CTimer::After(iTransTimeBudget());
		return;
		}

	__DS_TRACE((_L("DM: CSvrDomain::RunL() Members transition timeout hierarchy=%d, domain=0x%x, iTransCount=%d"), iHierarchy.iHierarchyId, iId, iTransCount));

	// Add a transition failure to the array
	TTransitionFailure failure(iId,KErrTimedOut);
	iHierarchy.iTransitionFailures.Append(failure);


	// Examine the failure policy to work out what to do
	if (iHierarchy.iFailurePolicy() == ETransitionFailureStop)
		{
		// CompleteTransition will in turn call CancelTransition,
		// which will reset iTransCount and the iAcknPending flags
		iHierarchy.CompleteTransition(KErrTimedOut);
		return;
		}

	if (iTransCount)
		{ // Complete transition of all members
		CDmDomainSession* session = iSessions;
		while (session)
			{
			if (session->iAcknPending)
				{
				__DS_TRACE((_L("DM: Member transition timeout domain=0x%x: CDmDomainSession Object 0x%08x"), iId, session));
				session->iAcknPending = EFalse;
				}
			session = session->iNext;
			}
		iTransCount = 0;
		MembersTransitionDone();
		}
	}


void CSvrDomain::CompleteDomainTransition()
	{
	__DS_TRACE((_L("DM: CSvrDomain::CompleteDomainTransition() hierarchy=%d, domain=0x%x"), iHierarchy.iHierarchyId, iId));
	__DS_ASSERT(iTransCount == 0);
	if (iHierarchy.iTransDomain == this)
		{
		const TInt err = (iHierarchy.iTransitionFailures.Count() > 0)? 
			iHierarchy.iTransitionFailures[0].iError : KErrNone;
		iHierarchy.CompleteTransition(err);
		}
	else
		{
		__DS_ASSERT(iParent);
		__DS_ASSERT(iParent->iTransCount);
		if (--iParent->iTransCount == 0)
			iParent->ChildrenTransitionDone();
		}
	}

void CSvrDomain::CancelTransition()
	{
	__DS_TRACE((_L("DM: CSvrDomain::CancelTransition() hierarchy=%d, domain=0x%x"), iHierarchy.iHierarchyId, iId));
	CTimer::Cancel();
	CSvrDomain* child = iChild;
	while (child)
		{
		child->CancelTransition();
		child = child->iPeer;
		}
	CDmDomainSession* session = iSessions;
	while (session)
		{
		// At this point the server says that the current transition may no
		// longer be acknowledged. Therefore, cancel the deferral
		if (session->DeferralActive())
			{
			session->CancelDeferral();
			}

		session->iAcknPending = EFalse;
		session = session->iNext;
		}
	iTransCount = 0;
	}

void CSvrDomain::SetObserver(TBool aSet)
	{
	iIsObserved=aSet;
	if(aSet)
		{
		iHierarchy.iObservedChildren++;
		}
	else 
		{
		// this should be zero at the end
		iHierarchy.iObservedChildren--;
		}
	if(iChildrenCount!=0)
		{
		CSvrDomain* domain=iChild;
		do	{
			domain->SetObserver(aSet);
			domain = domain->iPeer;
			}
		while(domain);
		}
	}

TDmDomainState CSvrDomain::State()
	{
	TInt value;
	iProperty.Get(value);
	return DmStateFromPropertyValue(value);
	}

/**
Called before members are notified of a transition.
*/
void CSvrDomain::SetMemberDeferralBudgets()
	{
	const TInt deferralBudget = iTransitionDeferralBudget();
	CDmDomainSession* session = iSessions;
	while (session)
		{
		__DS_ASSERT(!session->DeferralActive());
		__DS_ASSERT(!session->iAcknPending);
		session->SetDeferralBudget(deferralBudget);
		session = session->iNext;
		}
	}

/**
Called upon completion of the domain's timeout

@return True if at least one member had an active deferral
*/
TBool CSvrDomain::ExpireMemberDeferrals()
	{
	TBool deferDomain = EFalse;
	CDmDomainSession* session = iSessions;
	while (session)
		{
		if (session->DeferralActive())
			{
			__DS_ASSERT(session->iAcknPending);

			deferDomain = ETrue;
			session->ExpireDeferral();
			}
		session = session->iNext;
		}

	return deferDomain;
	}

//*********************************************************
// CDmHierarchy
//*********************************************************

CDmHierarchy* CDmHierarchy::New(TDmHierarchyId aHierarchyId, TDmHierarchyPolicy& aPolicy)
	{
	CDmHierarchy* self;
	
	if (aHierarchyId == KDmHierarchyIdPower)
		self = CDmHierarchyPower::New(aHierarchyId, aPolicy);
	else 
		self = new CDmHierarchy(aHierarchyId, aPolicy);

	__DS_PANIC_IFNUL(self);

	self->iSettings = new CHierarchySettings();
	__DS_TRACE((_L("DM: CDmHierarchy::New() @0x%08x, CHierarchySettings @0x%08x"), self, self->iSettings));

	__DS_PANIC_IFNUL(self->iSettings);

	self->iFailurePolicy.SetSettings(self->iSettings);

	return self;
	}

CDmHierarchy::CDmHierarchy(TDmHierarchyId aHierarchyId, TDmHierarchyPolicy& aPolicy) :
	iPositiveTransitions(aPolicy.iPositiveTransitions),
	iNegativeTransitions(aPolicy.iNegativeTransitions),
	iFailurePolicy(aPolicy.iFailurePolicy, iSettings),
	iHierarchyId(aHierarchyId)
	{
	__DS_TRACE((_L("DM: CDmHierarchy::CDmHierarchy() @0x%08x, hierarchy=%d"), this, iHierarchyId));

	iTransitionFailures.Reset();
	}

CDmHierarchy::~CDmHierarchy()
	{
	__DS_TRACE((_L("DM: CDmHierarchy::~CDmHierarchy() @0x%08x"), this));

	delete iRootDomain;
	delete iSettings;
	iTransitionFailures.Close();
	iTransitions.Close();
	}

CSvrDomain* CDmHierarchy::LookupDomain(TDmDomainId aDomainId)
	{
	return iRootDomain ? iRootDomain->Lookup(aDomainId) : NULL;
	}

void CDmHierarchy::RequestTransition(const RMessage2* aMessage)
	{
	// reset the array of transition failures
	iTransitionFailures.Reset();

	if (aMessage)
		{
		__DS_PANIC_IFNUL(iControllerSession);
		iControllerSession->iTransMessagePtr = *aMessage;
		}

	iSettings->SetCurrentTargetTransition(iTransState);

	iTransPropValue = DmStatePropertyValue(++iTransId, iTransState);

	iTransDomain->RequestDomainTransition();
	}


TInt CDmHierarchy::StartObserver(TDmDomainId aDomainId,TDmNotifyType aNotifyType)
	{
	iObservedDomain = LookupDomain(aDomainId);
		
	if(iObservedDomain==NULL)
		return KDmErrBadDomainId;
	

	iObservedDomain->SetObserver(ETrue);
	iNotifyType=aNotifyType;
	iObserverStarted=ETrue;
	return KErrNone;
	}

void CDmHierarchy::SetNotifyMessage(const RMessage2* aMessage)
	{
	if (aMessage)
		{
		__DS_PANIC_IFNUL(iObserverSession);
		iObserverSession->iObsvrMessagePtr = *aMessage;
		}
	}

TBool CDmHierarchy::OutstandingNotification()
	{
	return iObserverSession && !(iObserverSession->iObsvrMessagePtr.IsNull());
	}

void CDmHierarchy::CompleteNotification(TInt aError)
	{
	if (OutstandingNotification())
		{
		iObserverSession->iObsvrMessagePtr.Complete(aError);
		}
	}

void CDmHierarchy::StopObserver()
	{
	
	iObservedDomain->SetObserver(EFalse);
	iTransitions.Reset();
	iObserverStarted=EFalse;
	}

void CDmHierarchy::NotifyCompletion(TInt aReason)
	{
	iTransDomain = NULL;
	iTransPropValue = 0;

	if(iControllerSession)
		{
		__DS_ASSERT(!(iControllerSession->iTransMessagePtr.IsNull()));
		iControllerSession->iTransMessagePtr.Complete(aReason);
		}
	}

TInt CDmHierarchy::RequestSystemTransition(TDmDomainState aTargetState, TDmTraverseDirection aTraverseDirection, const RMessage2* aMessage)
	{
	iTransDomain = iRootDomain;
	SetState(aTargetState, aTraverseDirection);
	RequestTransition(aMessage);
	return KErrNone;
	}


TInt CDmHierarchy::RequestDomainTransition(
	TDmDomainId aDomainId, 
	TDmDomainState aTargetState, 
	TDmTraverseDirection aTraverseDirection, 
	const RMessage2* aMessage)
	{
	__DS_TRACE((_L("DM: CDmHierarchy::RequestTransition() hierarchy=%d domain=0x%x state=0x%x"), iHierarchyId, aDomainId, aTargetState)); 
	iTransDomain = LookupDomain(aDomainId);
	if (!iTransDomain)
		return KDmErrBadDomainId;
	SetState(aTargetState, aTraverseDirection);
	RequestTransition(aMessage);
	return KErrNone;
	}

void CDmHierarchy::CompleteTransition(TInt aError)
	{
	if (!iTransDomain)
		return;

	__DS_TRACE((_L("DM: CDmHierarchy::CompleteTransition() hierarchy=%d, domain=0x%x, aError=%d"), iHierarchyId, iTransDomain->iId, aError));

	if (iTransDomain)
		{
		iTransDomain->CancelTransition();
		NotifyCompletion(aError);
		}
	}

void CDmHierarchy::SetState(TDmDomainState aTargetState, TDmTraverseDirection aTraverseDirection)
	{
	__DS_ASSERT(iTransDomain);


	if (aTraverseDirection == ETraverseDefault)
		{
		TDmDomainState oldState = iTransDomain->State();

		if (aTargetState >= oldState)
			iTraverseDirection = iPositiveTransitions;
		else
			iTraverseDirection = iNegativeTransitions;
		}
	else
		iTraverseDirection = aTraverseDirection;

	__DS_ASSERT(iTraverseDirection < ETraverseMax);

	iTransState = aTargetState;
	}

//*********************************************************
// CPowerUpHandler
//*********************************************************

CPowerUpHandler* CPowerUpHandler::New(CDmHierarchyPower& aHierarchyPower)
	{
	CPowerUpHandler* self = new CPowerUpHandler(aHierarchyPower);
	__DS_PANIC_IFNUL(self);

	self->Construct();
	return self;
	}

CPowerUpHandler::CPowerUpHandler(CDmHierarchyPower& aHierarchyPower) : 
	CActive(CActive::EPriorityStandard),
	iHierarchyPower(aHierarchyPower)
	{
	}

void CPowerUpHandler::Construct()
	{
	CActiveScheduler::Add(this);
	}


void CPowerUpHandler::RequestWakeupEventNotification()
	{
	Power::RequestWakeupEventNotification(iStatus);
	SetActive();
	}


void CPowerUpHandler::Cancel()
	{
	CActive::Cancel();
	}

void CPowerUpHandler::RunL()
	{ 
	// power wakeup event
	iHierarchyPower.PowerUp();
	}


void CPowerUpHandler::DoCancel()
	{
	Power::DisableWakeupEvents();
	Power::CancelWakeupEventNotification(); 
	}



//*********************************************************
// CDmHierarchyPower
//*********************************************************
CDmHierarchyPower* CDmHierarchyPower::New(TDmHierarchyId aHierarchyId, TDmHierarchyPolicy& aPolicy)
	{
	CDmHierarchyPower* self;

	self = new CDmHierarchyPower(aHierarchyId, aPolicy);

	__DS_PANIC_IFNUL(self);

	self->Construct();

	return self;
	}

CDmHierarchyPower::CDmHierarchyPower(TDmHierarchyId aHierarchyId, TDmHierarchyPolicy& aPolicy) :
	CDmHierarchy(aHierarchyId, aPolicy)
	{
	}


void CDmHierarchyPower::Construct()
	{
	iPowerUpHandler = CPowerUpHandler::New(*this);
	__DS_PANIC_IFNUL(iPowerUpHandler);
	}

void CDmHierarchyPower::NotifyCompletion(TInt aReason)
	{
	iTransStatus = 0;
	CDmHierarchy::NotifyCompletion(aReason);
	}

TInt CDmHierarchyPower::RequestSystemTransition(TDmDomainState aTargetState, TDmTraverseDirection aTraverseDirection, const RMessage2* aMessage)
	{
	__DS_TRACE((_L("DM: CDmHierarchyPower::RequestSystemTransition() state = 0x%x"), aTargetState));
	
	TInt r = Power::EnableWakeupEvents((TPowerState) aTargetState);
	if (r != KErrNone)
		return r;
	
	iPowerUpHandler->RequestWakeupEventNotification();

	iTransStatus |= EPoweringDown;

	return CDmHierarchy::RequestSystemTransition(aTargetState, aTraverseDirection, aMessage);
	}

void CDmHierarchyPower::CompleteTransition(TInt aError)
	{
	if (!iTransDomain)
		return;

	__DS_TRACE((_L("DM: CDmHierarchyPower::CompleteTransition() domain=0x%x error=%d"),
				iTransDomain->iId, aError));

	if (iTransDomain && aError == KErrCancel)
		iPowerUpHandler->Cancel();

	if ((iTransStatus & EPoweringDown) && (aError != KErrCancel))
		{
		RFs fs;
		TInt r=fs.Connect();
		__DS_ASSERT(r==KErrNone);	
		__DS_TRACE((_L("DM: CDmSvrManager::CompleteTransition() Calling FinaliseDrives")));
		r=fs.FinaliseDrives();
		__DS_TRACE((_L("DM: CDmSvrManager::CompleteTransition()  Finalise returned %d"),r)); 
		fs.Close();

		Power::PowerDown();
		__DS_ASSERT(iTransState != (TDmDomainState) EPwOff);
		__DS_ASSERT(iPowerUpHandler->iStatus.Int() == KErrNone);
		}	
	else
		{
		CDmHierarchy::CompleteTransition(aError);
		}
	}


void CDmHierarchyPower::PowerUp()
	{
	__DS_TRACE((_L("DM: CDmHierarchyPower::RunL() Wakeup Event")));
	__DS_ASSERT(iTransDomain);

	Power::DisableWakeupEvents();

	iTransStatus &= ~EPoweringDown;
	iTransDomain->CancelTransition();
	SetState((TDmDomainState) EPwActive);
	RequestTransition(NULL);
	}


//*********************************************************
// CDmSvrManager
//*********************************************************

CDmSvrManager* CDmSvrManager::New()
	{
	CDmSvrManager* self = new CDmSvrManager();
	__DS_PANIC_IFNUL(self);

	self->Construct();
	return self;
	}

CDmSvrManager::CDmSvrManager()
	{
	}

void CDmSvrManager::Construct()
	{
	// Load the power hierarchy - other hierarchies need to be loaded
	// explicitly using RDmDomainManager::AddDomainHierarchy()
	TInt r = BuildDomainTree(KDmHierarchyIdPower);
	__DS_PANIC_IFERR(r);

	RProperty prop;
	r = prop.Define(KUidDmPropertyCategory, KDmPropertyKeyInit, RProperty::EInt,
					KAllowAllPolicy,KPowerMgmtPolicy);
	__DS_PANIC_IFERR(r);

	prop.Set(KUidDmPropertyCategory, KDmPropertyKeyInit, ETrue);
	}

TInt CDmSvrManager::BuildDomainTree(TDmHierarchyId aHierarchyId)
	{
	// We have already checked that the hierarchy doesn't already exist.

	// Get the name of the policy DLL.
	// This will be "domainPolicy.dll" for the power hierarchy
	// and "domainPolicy<n>.dll" for other hierarchies where <n> is the hierarchy ID.

	TFullName dllName;
	_LIT(KSysBin,"z:\\sys\\bin\\");
	dllName.Append(KSysBin);
	dllName.Append(_L("domainPolicy"));
	if (aHierarchyId != KDmHierarchyIdPower)
		dllName.AppendNum(aHierarchyId);

	dllName.Append(_L(".dll"));
	RLibrary lib;
	TInt r = lib.Load(dllName);
	if (r == KErrNotFound)
		return KErrBadHierarchyId;
	else if (r != KErrNone)
		return r;

	TLibraryFunction ordinal1 = lib.Lookup(EDmPolicyGetDomainSpecs);
	DmPolicyGetDomainSpecs getDomainSpecs = reinterpret_cast<DmPolicyGetDomainSpecs>(ordinal1);
	if (getDomainSpecs == NULL)
		r = KErrBadHierarchyId;

	TLibraryFunction ordinal2 = lib.Lookup(EDmPolicyRelease);
	DmPolicyRelease release = reinterpret_cast<DmPolicyRelease>(ordinal2);
	if (release == NULL)
		r = KErrBadHierarchyId;

	TLibraryFunction ordinal3 = lib.Lookup(EDmPolicyGetPolicy);
	DmPolicyGetPolicy getPolicy = reinterpret_cast<DmPolicyGetPolicy>(ordinal3);
	if (getPolicy == NULL)
		r = KErrBadHierarchyId;


	// get the domain spec for this hierarchy
	const TDmDomainSpec* spec = NULL;

	if (r == KErrNone)
		{
		spec = (*getDomainSpecs)();
		if (spec == NULL)
			r = KErrBadHierarchyId;
		}
	// get the policy
	TDmHierarchyPolicy hierarchyPolicy;
	if (r == KErrNone)
		{
		r = (*getPolicy)(hierarchyPolicy);
		if (r == KErrNone)
			{
			__DS_ASSERT(hierarchyPolicy.iPositiveTransitions < ETraverseMax);
			__DS_ASSERT(hierarchyPolicy.iNegativeTransitions < ETraverseMax);
			}
		}

	if (r != KErrNone)
		{
		lib.Close();
		return r;
		}

	CDmHierarchy* hierarchy = CDmHierarchy::New(aHierarchyId, hierarchyPolicy);
	__DS_PANIC_IFNUL(hierarchy);

	while (r == KErrNone && spec->iId != KDmIdNone)
		{
		// make sure the domain doesn't already exist in this hierarchy
		CSvrDomain* domain = hierarchy->LookupDomain(spec->iId);
		if (domain)
			{
			r = KErrBadHierarchyId;
			break;
			}

		domain = CSvrDomain::New(*hierarchy, *spec);
		__DS_ASSERT(domain);

		if (spec->iParentId == KDmIdNone)
			{
			if (hierarchy->iRootDomain)
				{
				delete domain;
				domain = NULL;
				r = KDmErrBadDomainSpec;
				break;
				}
			hierarchy->iRootDomain = domain;
			}
		else
			{
			CSvrDomain* parent = hierarchy->LookupDomain(spec->iParentId);
			if (!parent)
				{
				delete domain;
				domain = NULL;
				r = KDmErrBadDomainSpec;
				break;
				}
			parent->AddChild(domain);
			}
		++spec;
		}



	if (spec)
		(*release)(spec);

	// Load state specs
	if (r == KErrNone)
		{
		TInt err = LoadStateSpecs(lib, hierarchy);

		// KErrNotFound indicates that policy does not contain state
		// specs i.e. it is Version 1
		if ( (err !=KErrNone) && (err !=KErrNotFound))
			{
			r = err;
			}
		}

	if (r == KErrNone)
		{
		__DS_ASSERT(hierarchy->iRootDomain);
		iDomainHierarchies.Append(hierarchy);
		}
	else
		{
		delete hierarchy;
		hierarchy = NULL;
		}

	lib.Close();

	return r;
	}

TInt CDmSvrManager::LoadStateSpecs(RLibrary& aLib, CDmHierarchy* aHierarchy)
	{
	__DS_TRACE((_L("DM: CDmSvrManager::LoadStateSpecs() on CDmHierarchy @0x%08x"), aHierarchy));
	TLibraryFunction ordinal4 = aLib.Lookup(EDmPolicyGetStateSpec);
	DmPolicyGetStateSpec getStateSpec = reinterpret_cast<DmPolicyGetStateSpec>(ordinal4);
	if (getStateSpec == NULL)
		return KErrNotFound;

	TLibraryFunction ordinal5 = aLib.Lookup(EDmPolicyReleaseStateSpec);
	DmPolicyReleaseStateSpec releaseStateSpec = reinterpret_cast<DmPolicyReleaseStateSpec>(ordinal5);
	if (releaseStateSpec == NULL)
		return KErrNotFound;

	TAny* spec = NULL;
	TUint count = 0;
	TInt version = (*getStateSpec)(spec, count);
	TInt r = KErrNone;

	switch (version)
		{
		case 0:
			{
			r = KErrNotFound;
			break;
			}
		case 1:
			{
			if (count < 1)
				{
				r = KDmErrBadDomainSpec;
				break;
				}

			SDmStateSpecV1* specV1 = reinterpret_cast<SDmStateSpecV1*>(spec);
			for(TUint i = 0; i<count; ++i)
				{
				const TTransitionConfig transitionCfg(specV1[i]);
				r = transitionCfg.CheckValues();
				if (r != KErrNone)
					{
					break;
					}
				TRAP(r, aHierarchy->HierachySettings().StoreConfigL(transitionCfg));
				if (r != KErrNone)
					{
					break;
					}
				}
			break;
			}
		default:
			{
			if (version > 1)
				{
				r = KDmErrBadDomainSpec;
				}
			else
				{
				r = version;
				}
			break;
			}
		}

	if(spec)
		(*releaseStateSpec)(spec);

	return r;
	}

CDmHierarchy* CDmSvrManager::LookupHierarchy(TDmHierarchyId aHierarchyId)

	{
	// need to find the correct hierarchy first
	TInt len = iDomainHierarchies.Count();

	CDmHierarchy* hierarchy = NULL;
	for (TInt n=0; n<len; n++)
		{
		if (iDomainHierarchies[n]->iHierarchyId == aHierarchyId)
			{
			hierarchy = iDomainHierarchies[n];
			break;
			}
		}

	return hierarchy;
	}
	

TInt CDmSvrManager::LookupDomain(TDmHierarchyId aHierarchyId, TDmDomainId aDomainId, CSvrDomain*& aDomain)
	{
	// need to find the correct hierarchy first
	CDmHierarchy* hierarchy = LookupHierarchy(aHierarchyId);
	if (hierarchy == NULL)
		return KErrBadHierarchyId;

	aDomain = hierarchy->LookupDomain(aDomainId);
	if (aDomain == NULL)
		return KDmErrBadDomainId;
	
	return KErrNone;
	}


CSession2* CDmManagerServer::NewSessionL(const TVersion&, const RMessage2& aMessage) const
	{
    // If the client does not have ECapabilityPowerMgmt capability, then it has no
    // right to make this request. Blow it up.
    if (!KPowerMgmtPolicy.CheckPolicy(aMessage))
		{
        User::Leave(KErrPermissionDenied);
		}

	CDmManagerSession* session = new CDmManagerSession();
#if defined(_DEBUG) || defined(__DS_DEBUG)
	CleanupStack::PushL(session);
	TFullName clientName;
	GetClientNameFromMessageL(aMessage, clientName);

	// Sessions on this server may not be shared by clients
	__DS_TRACE((_L("DM: New CDmManagerSession @0x%08x, client %S"), session, &clientName));
	CleanupStack::Pop();
#else
	(void)aMessage;
#endif
	return session;
	}

CDmManagerSession::CDmManagerSession()
	{
	__DS_TRACE((_L("DM: CDmManagerSession() @0x%08x"), this));
	}

CDmManagerSession::~CDmManagerSession()
	{
	__DS_TRACE((_L("DM: ~CDmManagerSession() @0x%08x"), this));

	if (iHierarchy)
		{
		if (iHierarchy->iControllerSession == this)
			{
			iHierarchy->iControllerSession = NULL;
			}
		if (iHierarchy->iObserverSession == this)
			{
			iHierarchy->iObserverSession = NULL;
			}
		}

	if(!iTransMessagePtr.IsNull())
		iTransMessagePtr.Complete(KErrCancel);

	if(!iObsvrMessagePtr.IsNull())
		iObsvrMessagePtr.Complete(KErrCancel);
	}

void CDmManagerSession::ServiceL(const RMessage2& aMessage)
	{
	TInt r;
	CDmSvrManager* manager = ((CDmManagerServer*) Server()) -> iManager;

	switch (aMessage.Function())
		{
		case EDmHierarchyAdd:
			{
			r = KErrNone;
			TDmHierarchyId hierarchyId = (TDmHierarchyId) aMessage.Int0();
			__DS_TRACE((_L("DM: CDmManagerSession::ServiceL @0x%08x EDmHierarchyAdd, HierarchyId %d"), this, hierarchyId));

			if (manager->LookupHierarchy(hierarchyId) == NULL)
				r = manager->BuildDomainTree(hierarchyId);
			aMessage.Complete(r);
			}
			break;

		case EDmHierarchyJoin:
			{
			r = KErrNone;
			TDmHierarchyId hierarchyId = (TDmHierarchyId) aMessage.Int0();
			__DS_TRACE((_L("DM: CDmManagerSession::ServiceL @0x%08x EDmHierarchyJoin, HierarchyId %d"), this, hierarchyId));

			iHierarchy = manager->LookupHierarchy(hierarchyId);
			if (iHierarchy == NULL)
				r = KErrBadHierarchyId;

			if (r == KErrNone)
				{
				// is the hierarchy already in use ?
				if (iHierarchy->iControllerSession)
					r = KErrInUse;
				else
					iHierarchy->iControllerSession = this;
				}

			aMessage.Complete(r);
			}
			break;

		case EDmRequestSystemTransition:
			{
			const TDmDomainState targetState = aMessage.Int0();
			const TDmTraverseDirection direction = (TDmTraverseDirection)aMessage.Int1();

			__DS_TRACE((_L("DM: CDmManagerSession::ServiceL @0x%08x EDmRequestSystemTransition, TargetState %d, Direction %d"),
				this, targetState, direction));

			if (iHierarchy==NULL)
				{
				aMessage.Complete(KErrBadHierarchyId);
				break;
				}
			if (iHierarchy->iTransDomain)
				{
				aMessage.Complete(KDmErrBadSequence);
				break;
				}

			r = iHierarchy->RequestSystemTransition(
				targetState,
				direction,
				&aMessage);

			if (r != KErrNone)
				aMessage.Complete(r);
			break;
			}

		case EDmRequestDomainTransition:
			{
			const TDmDomainId domain = (TDmDomainId)aMessage.Int0();
			const TDmDomainState targetState = aMessage.Int1();
			const TDmTraverseDirection direction = (TDmTraverseDirection)aMessage.Int2();

			__DS_TRACE((_L("DM: CDmManagerSession::ServiceL @0x%08x EDmRequestDomainTransition, Domain 0x%x, TargetState %d, Direction %d"),
				this, domain, targetState, direction));

			if (iHierarchy==NULL)
				{
				aMessage.Complete(KErrBadHierarchyId);
				break;
				}
			if (iHierarchy->iTransDomain)
				{
				aMessage.Complete(KDmErrBadSequence);
				break;
				}

			r = iHierarchy->RequestDomainTransition(
				domain,
				targetState,
				direction,
				&aMessage);

			if (r != KErrNone)
				aMessage.Complete(r);
			break;
			}

		case EDmGetTransitionFailureCount:
			{
			if (iHierarchy==NULL)
				{
				aMessage.Complete(KErrBadHierarchyId);
				break;
				}
			TInt failureCount = iHierarchy->iTransitionFailures.Count();
			aMessage.Complete(failureCount);
			}
			break;
		
		case EDmGetTransitionFailures:
			{
			if (iHierarchy==NULL)
				{
				aMessage.Complete(KErrBadHierarchyId);
				break;
				}
			TInt failureCount = iHierarchy->iTransitionFailures.Count();
			TInt clientFailureSize = aMessage.GetDesMaxLength(0);
			TInt clientFailureCount = clientFailureSize / sizeof(TTransitionFailure);
			__DS_ASSERT( (clientFailureSize % sizeof(TTransitionFailure)) == 0);
			__DS_ASSERT(failureCount >= clientFailureCount);
			
			HBufC8* hBuf = HBufC8::New(clientFailureSize);
			if(hBuf == NULL)
				{
				aMessage.Complete(KErrNoMemory);
				break;
				}
			TPtr8 pBuf = hBuf->Des();
			pBuf.Zero();
			for (TInt i=0; i<clientFailureCount; i++)
				{
				TPtrC8 ptr = TPtrC8((TUint8*) &iHierarchy->iTransitionFailures[i], sizeof(TTransitionFailure));
				pBuf.Append(ptr);
				}
			r = aMessage.Write(0, pBuf);
			delete hBuf;

			aMessage.Complete(r);
			}
			break;

		case EDmCancelTransition:
			{
			__DS_TRACE((_L("DM: CDmManagerSession::ServiceL @0x%08x EDmCancelTransition"), this));

			if (iHierarchy == NULL)
				{
				aMessage.Complete(KErrBadHierarchyId);
				break;
				}
			iHierarchy->CompleteTransition(KErrCancel);
			if (iHierarchy->iObserverStarted)
				{
				iHierarchy->CompleteNotification(KErrCancel);
				iHierarchy->StopObserver();
				}
			aMessage.Complete(KErrNone);
			break;
			}
		case EDmObserverCancel:
			if (iHierarchy == NULL)
				{
				aMessage.Complete(KErrBadHierarchyId);
				break;
				}
			if(!iHierarchy->iObserverSession)
				{
				aMessage.Complete(KDmErrBadSequence);
				break;
				}
			if (iHierarchy->iObserverStarted)
				{
				iHierarchy->CompleteNotification(KErrCancel);
				iHierarchy->StopObserver();
				}
			aMessage.Complete(KErrNone);
			break;

		case EDmObserverJoin:
			{
			TDmHierarchyId hierarchyId = (TDmHierarchyId) aMessage.Int0();

			iHierarchy = manager->LookupHierarchy(hierarchyId);
			if(iHierarchy==NULL)
				{
				aMessage.Complete(KErrBadHierarchyId);
				break;
				}
			if(iHierarchy->iObserverSession)
				{
				aMessage.Complete(KDmErrBadSequence);
				break;
				}
			iHierarchy->iTransitions.Reset();
			iHierarchy->iObserverSession = this;
			aMessage.Complete(KErrNone);
			}
			break;

		case EDmObserverStart:
			{
			if (iHierarchy==NULL)
				{
				aMessage.Complete(KErrBadHierarchyId);
				break;
				}
			
			if(iHierarchy->iObserverStarted || iHierarchy->iObserverSession != this)
				{
				aMessage.Complete(KDmErrBadSequence); 
				break;
				}
			TInt ret= iHierarchy->StartObserver((TDmDomainId)aMessage.Int0(),(TDmNotifyType)aMessage.Int1());
			aMessage.Complete(ret);
			}
			break;

		case EDmObserverNotify:
			{
			if (iHierarchy==NULL)
				{
				aMessage.Complete(KErrBadHierarchyId);
				break;
				}
			if(!iHierarchy->iObserverStarted || iHierarchy->iObserverSession != this)
				{
				aMessage.Complete(KDmErrBadSequence); 
				break;
				}
			//	Check to see if we have any events stored 
			//	If so, then notify the client
			if(iHierarchy->iTransitions.Count()>0)
				{
				aMessage.Complete(KErrNone);
				break;
				}
			//	No events are stored. complete this message later
			iHierarchy->SetNotifyMessage(&aMessage);
			}
			break;
		
		case EDmObserverEventCount:
			{
			if (iHierarchy==NULL)
				{
				aMessage.Complete(KErrBadHierarchyId);
				break;
				}
			if(!iHierarchy->iObserverStarted || iHierarchy->iObserverSession != this)
				{
				aMessage.Complete(KDmErrBadSequence);
				break;
				}
			TInt count = iHierarchy->iTransitions.Count();
			aMessage.Complete(count);
			}
			break;
		
		case EDmObserverGetEvent:
			{
			if (iHierarchy==NULL)
				{
				aMessage.Complete(KErrBadHierarchyId);
				break;
				}
			if(!iHierarchy->iObserverStarted || iHierarchy->iObserverSession != this)
				{
				aMessage.Complete(KDmErrBadSequence);
				break;
				}
			TInt transitionCount = iHierarchy->iTransitions.Count();
			TInt clientTransitionSize = aMessage.GetDesMaxLength(0);
			TInt clientTransitionCount = clientTransitionSize / sizeof(TTransInfo);
			__DS_ASSERT( (clientTransitionSize % sizeof(TTransInfo)) == 0);
			__DS_ASSERT(transitionCount >= clientTransitionCount);
			
			HBufC8* hBuf = HBufC8::New(clientTransitionSize);
			if(hBuf == NULL)
				{
				aMessage.Complete(KErrNoMemory);
				break;
				}
			TPtr8 pBuf = hBuf->Des();
			pBuf.Zero();
			for (TInt i=0; i<clientTransitionCount; i++)
				{
				TPtrC8 ptr = TPtrC8((TUint8*) &iHierarchy->iTransitions[0], sizeof(TTransInfo));
				pBuf.Append(ptr);
				iHierarchy->iTransitions.Remove(0);
				}
			r = aMessage.Write(0, pBuf);
			delete hBuf;

			aMessage.Complete(r);
			}
			break;
		case EDmObserveredCount:
			{
			if (iHierarchy==NULL)
				{
				aMessage.Complete(KErrBadHierarchyId);
				break;
				}
			if(!iHierarchy->iObserverStarted || iHierarchy->iObserverSession != this)
				{
				aMessage.Complete(KDmErrBadSequence);
				break;
				}
			aMessage.Complete(iHierarchy->iObservedChildren);
			}
			break;
		default:
			aMessage.Complete(KDmErrBadRequest);
			break;
		}
	}

CSession2* CDmDomainServer::NewSessionL(const TVersion&, const RMessage2& aMessage) const
	{
	CDmDomainSession* session =  new (ELeave) CDmDomainSession;
#ifdef __DS_DEBUG
	CleanupStack::PushL(session);
	TFullName clientName;
	GetClientNameFromMessageL(aMessage, clientName);

	// Sessions on this server may not be shared by clients
	__DS_TRACE((_L("DM: New CDmDomainSession @0x%08x, client %S"), session, &clientName));
	CleanupStack::Pop();
#else
	(void)aMessage;
#endif
	return session;
	}

CDmDomainSession::CDmDomainSession()
	: iDeferralsRemaining(0)
	{
	__DS_TRACE((_L("DM: CDmDomainSession() @0x%08x"), this));
	}

CDmDomainSession::~CDmDomainSession()
	{
	__DS_TRACE((_L("DM: ~CDmDomainSession() @0x%08x"), this));
	if (iAcknPending)
		iDomain->CompleteMemberTransition(KErrNone);
	if (iDomain)
		iDomain->Detach(this);
	if (DeferralActive())
		CancelDeferral();
	}

void CDmDomainSession::ServiceL(const RMessage2& aMessage)
	{
	TInt r = KErrNone;
	CDmSvrManager* manager = ((CDmManagerServer*) Server()) -> iManager;

	switch (aMessage.Function())
		{
	case EDmDomainJoin:
		{
		TDmHierarchyId hierarchyId = (TDmHierarchyId) aMessage.Int0();
		TDmDomainId domainId = (TDmDomainId) aMessage.Int1();

		__DS_TRACE((_L("DM: CDmDomainSession::ServiceL @0x%08x EDmDomainJoin, HierarchyId %d, DomainId 0x%x"), this, hierarchyId, domainId));

		r = manager->LookupDomain(hierarchyId, domainId, iDomain);

		if (r != KErrNone)
			break;

		// Check client has capability to join the domain
		if (!iDomain->iJoinPolicy.CheckPolicy(aMessage))
			{
			r = KErrPermissionDenied;
			iDomain = NULL;
			break;
			}

		iDomain->Attach(this);
		break;
		}

	case EDmStateRequestTransitionNotification:
		__DS_TRACE((_L("DM: CDmDomainSession::ServiceL @0x%08x EDmStateRequestTransitionNotification"), this));
		iNotificationEnabled = ETrue;
		break;

	case EDmStateCancelTransitionNotification:
		__DS_TRACE((_L("DM: CDmDomainSession::ServiceL @0x%08x EDmStateCancelTransitionNotification"), this));
		iNotificationEnabled = EFalse;
		break;

	case EDmStateAcknowledge:
		{
		TInt propValue = aMessage.Int0();
		TInt error = aMessage.Int1();

		__DS_TRACE((_L("DM: CDmDomainSession::ServiceL @0x%08x EDmStateAcknowledge, error %d"), this, error));

		if (!iDomain)
			{
			r = KDmErrNotJoin;
			break;
			}
		if (iAcknPending && iDomain->CheckPropValue(propValue))
			{
			if (DeferralActive())
				ObsoleteDeferral();

			iAcknPending = EFalse;
			iDomain->CompleteMemberTransition(error);
			}
		else
			{
			// This error code indicates that there was no pending transition
			// corresponding to the cookie supplied in propValue.
			r = KErrNotFound;
			}

		break;
		}

	case EDmStateDeferAcknowledgement:
		{
		__DS_TRACE((_L("DM: CDmDomainSession::ServiceL @0x%08x EDmStateDeferAcknowledgement"), this));

		// PlatSec check
		if (!KWddPolicy.CheckPolicy(aMessage,
									__PLATSEC_DIAGNOSTIC_STRING("KWriteDeviceDataPolicy")) &&
			!KProtServPolicy.CheckPolicy(aMessage,
										 __PLATSEC_DIAGNOSTIC_STRING("KProtServPolicy")))
			{
			// Try harder...
			r = KErrPermissionDenied;
			break;
			}
		DeferAcknowledgment(aMessage);
		return;												// return, not break
		}

	case EDmStateCancelDeferral:
		{
		__DS_TRACE((_L("DM: CDmDomainSession::ServiceL @0x%08x EDmStateCancelDeferral"), this));
		CancelDeferral();
		break;
		}
	default:
		r = KDmErrBadRequest;
		break;
		}

	aMessage.Complete(r);
	}

void CDmDomainSession::SetDeferralBudget(TInt aDeferralCount)
	{
	__DS_ASSERT(!DeferralActive());
	iDeferralsRemaining = aDeferralCount;
	}

TBool CDmDomainSession::DeferralActive() const
	{
	return (!iDeferralMsg.IsNull());
	}

void CDmDomainSession::ExpireDeferral()
	{
	__DS_ASSERT(DeferralActive());
	__DS_ASSERT(iDeferralsRemaining >= 0);

	CompleteDeferral(KErrNone);
	}

void CDmDomainSession::DeferAcknowledgment(const RMessage2& aMessage)
	{
	__DS_ASSERT(aMessage.Function() == EDmStateDeferAcknowledgement);
	__DS_ASSERT(!DeferralActive());

	TInt r = KErrNone;

	if (!iAcknPending)
		{
		r = KErrNotReady;
		}
	else if (iDeferralsRemaining == 0)
		{
		r = KErrNotSupported;
		}

	if(KErrNone != r)
		{
		aMessage.Complete(r);
		return;
		}

	--iDeferralsRemaining;
	iDeferralMsg = aMessage;
	}

void CDmDomainSession::CancelDeferral()
	{
	if (DeferralActive())
		{
		CompleteDeferral(KErrCancel);
		}
	}

void CDmDomainSession::ObsoleteDeferral()
	{
	__DS_ASSERT(DeferralActive());

	CompleteDeferral(KErrCompletion);
	}

void CDmDomainSession::CompleteDeferral(TInt aError)
	{
	__DS_TRACE((_L("DM: CDmDomainSession::CompleteDeferral() @0x%08x, aError %d"), this, aError));
	__DS_ASSERT(DeferralActive());

	iDeferralMsg.Complete(aError);
	}


TInt E32Main()
	{
	// Make DM a system critical server
	User::SetProcessCritical(User::ESystemCritical);
	User::SetCritical(User::ESystemCritical);

	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	__DS_ASSERT_STARTUP(cleanupStack);

	CActiveScheduler* sched = new CActiveScheduler();
	__DS_ASSERT_STARTUP(sched);

	CActiveScheduler::Install(sched);

	CDmSvrManager* mngr = CDmSvrManager::New();
	__DS_ASSERT_STARTUP(mngr);

	CDmManagerServer* msrv = new CDmManagerServer(mngr);
	__DS_ASSERT_STARTUP(msrv);

	TInt r=msrv->Start(KDmManagerServerNameLit);
	__DS_ASSERT_STARTUP(r == KErrNone);

	CDmDomainServer* dsrv = new CDmDomainServer(mngr);
	__DS_ASSERT_STARTUP(dsrv);

	r=dsrv->Start(KDmDomainServerNameLit);
	__DS_ASSERT_STARTUP(r == KErrNone);

	CActiveScheduler::Start();

	// Server should never shutdown, hence panic to highlight such an event
	__DS_PANIC(0);

	return KErrNone;
	}
