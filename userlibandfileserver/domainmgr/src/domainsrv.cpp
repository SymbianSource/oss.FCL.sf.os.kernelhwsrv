// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// domain\src\domainsrv.cpp
// 
//

#include <e32debug.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32property.h>
#include <f32file.h>

#include <domainpolicy.h>
#include "domainsrv.h"

#define __DS_PANIC(aError) User::Panic(_L("domainSrv.cpp"), (-(aError)) | (__LINE__ << 16))
#define __DS_ASSERT(aCond) ((aCond) || (User::Panic(_L("domainSrv.cpp; assertion failed"), __LINE__), 1))

//#define __DS_DEBUG

#ifdef __DS_DEBUG
#define __DS_TRACE(s) RDebug::Print s
#else
#define __DS_TRACE(s)
#endif

static _LIT_SECURITY_POLICY_PASS(KAllowAllPolicy);
static _LIT_SECURITY_POLICY_C1(KPowerMgmtPolicy,ECapabilityPowerMgmt);

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



// CSvrDomain
class CSvrDomain : public CTimer
	{
public: 
	static CSvrDomain* New(CDmHierarchy& aHierarchy, const TDmDomainSpec&);

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

private:
	CDmHierarchy&		iHierarchy;
	CSvrDomain*			iParent;
	CSvrDomain*			iPeer;
	CSvrDomain*			iChild;
	RProperty			iProperty;
	CDmDomainSession*	iSessions;
	TUint16				iChildrenCount;
	TUint16				iTransCount;
	TTimeIntervalMicroSeconds32	iTransTimeBudget;

public:
	const TSecurityPolicy	iJoinPolicy;
	TBool iIsObserved;
	TDmDomainId			iId;
	};


// CDmHierarchy
class CDmHierarchy : public CBase
	{
public:
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
	
protected:
	CDmHierarchy(TDmHierarchyId aHierarchyId, TDmHierarchyPolicy& aPolicy);
	void SetState(TDmDomainState aTargetState, TDmTraverseDirection aTraverseDirection = ETraverseDefault);

private:
	RMessagePtr2	iTransMessagePtr;
	RMessagePtr2	iObsvrMessagePtr;
	CSvrDomain*		iObservedDomain;
	TBool			iOutstandingNotification;
public:
	TDmHierarchyId	iHierarchyId;
	CSvrDomain*		iRootDomain;
	CSvrDomain*		iTransDomain;
	TInt			iTransPropValue;
	TDmDomainState	iTransState;
	TDmTraverseDirection	iTraverseDirection;
	TUint8			iTransId;
	CDmManagerSession* iControllerSession;	// only one controller per hierarchy
	TDmHierarchyPolicy iPolicy;
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

	TInt BuildDomainTree(TDmHierarchyId aHierarchyId, CDmHierarchy*& aHierarchy);
	CDmHierarchy* LookupHierarchy(TDmHierarchyId aHierarchyId);
	TInt LookupDomain(TDmHierarchyId aHierarchyId, TDmDomainId aDomainId, CSvrDomain*& aDomain);

private:
	CDmSvrManager();
	void Construct();

private:
	RPointerArray<CDmHierarchy> iDomainHierarchies;
	};

// CDmDomainServer
class CDmDomainServer : public CServer2
	{
public: 
	// from CServer2
	CSession2* NewSessionL(const TVersion& aVer) const;
	CSession2* NewSessionL(const TVersion& aVer, const RMessage2& aMessage) const;

	CDmDomainServer(CDmSvrManager* aManager) : CServer2(CActive::EPriorityStandard), iManager(aManager)
		{}

public:
	CDmSvrManager*	iManager;
	};

// CDmDomainSession
class CDmDomainSession : public CSession2
	{
public: 
	// from CBase
	~CDmDomainSession();

	// from CSession2
	void ServiceL(const RMessage2& aMessage);

private:
	CSvrDomain*			iDomain;

public:
	CDmDomainSession*	iNext;
	TUint8				iPending;
	TBool				iNotificationEnabled;
	};

class CDmManagerServer : public CServer2
	{
public: 
	// from CServer2
	CSession2* NewSessionL(const TVersion& aVer) const;
	CSession2* NewSessionL(const TVersion& aVer, const RMessage2&) const;

	CDmManagerServer(CDmSvrManager* aManager) : CServer2(CActive::EPriorityStandard), iManager(aManager)
		{}
	CDmSvrManager*	iManager;
	};

class CDmManagerSession : public CSession2
	{
public: 
	// from CBase
	~CDmManagerSession();
	
	// from CSession2
	void ServiceL(const RMessage2& aMessage);

	CDmManagerSession();
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
		iTransTimeBudget(spec->iTimeBudgetUs),
		iJoinPolicy(spec->iJoinPolicy),
		iId(spec->iId)
	{}

CSvrDomain* CSvrDomain::New(CDmHierarchy& aHierarchy, const TDmDomainSpec& aSpec)
	{

	CSvrDomain* self = new CSvrDomain(aHierarchy, &aSpec);

	if (!self)
		__DS_PANIC(KErrNoMemory);
	self->Construct(&aSpec);
	return self;
	}

void CSvrDomain::Construct(const TDmDomainSpec* spec)
	{
	TInt r = iProperty.Define(
		KUidDmPropertyCategory, 
		DmStatePropertyKey(iHierarchy.iHierarchyId, iId), 
		RProperty::EInt,
		KAllowAllPolicy,KPowerMgmtPolicy);

	if (r != KErrNone)
		__DS_PANIC(r);
	
	r = iProperty.Attach(KUidDmPropertyCategory, DmStatePropertyKey(
		iHierarchy.iHierarchyId, 
		iId));

	if (r != KErrNone)
		__DS_PANIC(r);

	r = iProperty.Set(DmStatePropertyValue(0, spec->iInitState));
	if (r != KErrNone)
		__DS_PANIC(r);

	TRAP(r, CTimer::ConstructL());
	if (r != KErrNone)
		__DS_PANIC(r);
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
	__DS_TRACE((_L("CSvrDomain::RequestMembersTransition() hierarchy=%d, domain=0x%x"), iHierarchy.iHierarchyId, iId));
	__DS_ASSERT(iTransCount == 0);
	
	for(CDmDomainSession* s = iSessions; s; s = s->iNext)
		if (s->iNotificationEnabled)
			{
			++iTransCount;
			s->iPending = ETrue;
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
		CTimer::After(iTransTimeBudget);
	iProperty.Set(iHierarchy.iTransPropValue);
	if (iTransCount == 0)
		MembersTransitionDone();
	}


void CSvrDomain::RequestChildrenTransition()
	{
	__DS_TRACE((_L("CSvrDomain::RequestChildrenTransition() hierarchy=%d, domain=0x%x"), iHierarchy.iHierarchyId, iId));
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
	__DS_TRACE((_L("CSvrDomain::RequestDomainTransition() hierarchy=%d, domain=0x%x state=0x%x prop=0x%x"), 
						iHierarchy.iHierarchyId, iId, iHierarchy.iTransState, iHierarchy.iTransPropValue));
	__DS_ASSERT(iTransCount == 0);
	if (iHierarchy.iTraverseDirection == ETraverseChildrenFirst)
		RequestChildrenTransition();
	else
		RequestMembersTransition();
	}
		
void CSvrDomain::MembersTransitionDone()
	{
	__DS_TRACE((_L("CSvrDomain::MembersTransitionDone() hierarchy=%d, domain=0x%x"), iHierarchy.iHierarchyId, iId));
	__DS_ASSERT(iTransCount == 0);
	if (iHierarchy.iTraverseDirection == ETraverseChildrenFirst)
		CompleteDomainTransition();
	else
		RequestChildrenTransition();
	}

void CSvrDomain::ChildrenTransitionDone()
	{
	__DS_TRACE((_L("CSvrDomain::ChildrenTransitionDone() hierarchy=%d, domain=0x%x"), iHierarchy.iHierarchyId, iId));
	__DS_ASSERT(iTransCount == 0);
	if (iHierarchy.iTraverseDirection == ETraverseChildrenFirst)
		RequestMembersTransition();
	else
		CompleteDomainTransition();
	}

void CSvrDomain::CompleteMemberTransition(TInt aError)
	{
	__DS_TRACE((_L("CSvrDomain::CompleteMemberTransition() hierarchy=%d, domain=0x%x, aError = %d"), iHierarchy.iHierarchyId, iId, aError));
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
		if (iHierarchy.iPolicy.iFailurePolicy == ETransitionFailureStop)
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
	__DS_TRACE((_L("CSvrDomain::RunL() Members transition timeout hierarchy=%d, domain=0x%x"), iHierarchy.iHierarchyId, iId));

	// Add a transition failure to the array
	TTransitionFailure failure(iId,KErrTimedOut);
	iHierarchy.iTransitionFailures.Append(failure);


	// examine the failure policy to work out what to do
	if (iHierarchy.iPolicy.iFailurePolicy == ETransitionFailureStop)
		{
		iHierarchy.CompleteTransition(KErrTimedOut);
		return;
		}

	if (iTransCount)
		{ // Complete transition of all members
		CDmDomainSession* session = iSessions;
		while (session)
			{
			session->iPending = EFalse;
			session = session->iNext;
			}
		iTransCount = 0;
		MembersTransitionDone();
		}
	}


void CSvrDomain::CompleteDomainTransition()
	{
	__DS_TRACE((_L("CSvrDomain::CompleteDomainTransition() hierarchy=%d, domain=0x%x"), iHierarchy.iHierarchyId, iId));
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
	__DS_TRACE((_L("CSvrDomain::CancelTransition() hierarchy=%d, domain=0x%x"), iHierarchy.iHierarchyId, iId));
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
		session->iPending = EFalse;
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

	if (!self)
		__DS_PANIC(KErrNoMemory);

	return self;
	}

CDmHierarchy::CDmHierarchy(TDmHierarchyId aHierarchyId, TDmHierarchyPolicy& aPolicy) :
	iOutstandingNotification(EFalse),
	iHierarchyId(aHierarchyId),
	iPolicy(aPolicy)
	{
	iTransitionFailures.Reset();
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
		iTransMessagePtr = *aMessage;
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
		iObsvrMessagePtr = *aMessage;
		iOutstandingNotification=ETrue;
		}		
	}

TBool CDmHierarchy::OutstandingNotification()
	{
	return iOutstandingNotification;
	}

void CDmHierarchy::CompleteNotification(TInt aError)
	{
	if(iOutstandingNotification)
		{
		iObsvrMessagePtr.Complete(aError);
		iOutstandingNotification=EFalse;
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
	iTransMessagePtr.Complete(aReason);
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
	__DS_TRACE((_L("CDmHierarchy::RequestTransition() hierarchy=%d domain=0x%x state=0x%x"), iHierarchyId, aDomainId, aTargetState)); 
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

	__DS_TRACE((_L("CDmHierarchy::CompleteTransition() hierarchy=%d, domain=0x%x, aError=%d"), iHierarchyId, iTransDomain->iId, aError));

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
			iTraverseDirection = iPolicy.iPositiveTransitions;
		else
			iTraverseDirection = iPolicy.iNegativeTransitions;
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
	if (!self)
		__DS_PANIC(KErrNoMemory);
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

	if (!self)
		__DS_PANIC(KErrNoMemory);

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
	if (!iPowerUpHandler)
		__DS_PANIC(KErrNoMemory);
	}

void CDmHierarchyPower::NotifyCompletion(TInt aReason)
	{
	iTransStatus = 0;
	CDmHierarchy::NotifyCompletion(aReason);
	}

TInt CDmHierarchyPower::RequestSystemTransition(TDmDomainState aTargetState, TDmTraverseDirection aTraverseDirection, const RMessage2* aMessage)
	{
	__DS_TRACE((_L("CDmSvrManager::RequestSystemTransition() state = 0x%x"), aTargetState));
	
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

	__DS_TRACE((_L("CDmHierarchyPower::CompleteTransition() domain=0x%x"), iTransDomain->iId));

	if (iTransDomain && aError == KErrCancel)
		iPowerUpHandler->Cancel();

	if (iTransStatus & EPoweringDown)
		{
		RFs fs;
		TInt r=fs.Connect();
		__DS_ASSERT(r==KErrNone);	
		__DS_TRACE((_L("CDmSvrManager::CompleteTransition() Calling FinaliseDrives")));
		r=fs.FinaliseDrives();
		__DS_TRACE((_L("CDmSvrManager::CompleteTransition()  Finalise returned %d"),r)); 
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
	__DS_TRACE((_L("CDmHierarchyPower::RunL() Wakeup Event")));
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
	if (!self)
		__DS_PANIC(KErrNoMemory);
	self->Construct();
	return self;
	}

CDmSvrManager::CDmSvrManager()
	{
	}

void CDmSvrManager::Construct()
	{
	// load the power hierarchy-  Other hieratchies need to be loaded 
	// explicitly using RDmDomainManager::AddDomainHierarchy()
	CDmHierarchy* hierarchy;
	TInt r = BuildDomainTree(KDmHierarchyIdPower, hierarchy);
	if (r != KErrNone)
		__DS_PANIC(r);


	RProperty prop;
	r = prop.Define(KUidDmPropertyCategory, KDmPropertyKeyInit, RProperty::EInt,
							KAllowAllPolicy,KPowerMgmtPolicy);
	if (r != KErrNone)
		__DS_PANIC(r);

	prop.Set(KUidDmPropertyCategory, KDmPropertyKeyInit, ETrue);
	}

TInt CDmSvrManager::BuildDomainTree(TDmHierarchyId aHierarchyId, CDmHierarchy*& aHierarchy)
	{

	aHierarchy = NULL;

	// assume we have already checked that the hierarchy doesn't already exist

	// Get the name of the policy Dll
	// This will be "domainPolicy.dll" for the power hierarchy
	// and "domainPolicy<n>.dll" for other hierarchies where <n> is the hierarchy ID.
	//
	// If the hierarchy ID is less than KMaxCriticalPolicyDll, load only from ROM

	TFullName dllName;

	// is this policy "critical" i.e non-replaceable ?
	_LIT(KSysBin,"z:\\sys\\bin\\");	
	// const TInt KMaxCriticalPolicyDll = 1000;
	// if (aHierarchyId < KMaxCriticalPolicyDll) // <-- cannot be false while aHierarchyId is a TUint8 (typedef'd to TDmHierarchyId)
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
	if (hierarchy == NULL)
		__DS_PANIC(KErrNoMemory);

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
				r = KDmErrBadDomainSpec;
				break;
				}
			parent->AddChild(domain);
			}
		++spec;
		}

	if (spec)
		(*release)(spec);


	if (r == KErrNone)
		{
		__DS_ASSERT(hierarchy->iRootDomain);
		iDomainHierarchies.Append(hierarchy);
		aHierarchy = hierarchy;
		}
	else
		{
		delete hierarchy;
		hierarchy = NULL;
		}

	lib.Close();

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

	return new CDmManagerSession();
	}

CSession2* CDmManagerServer::NewSessionL(const TVersion&) const
	{
	__DS_PANIC(KErrGeneral);
	return 0;
	}

CDmManagerSession::CDmManagerSession()
	{}

CDmManagerSession::~CDmManagerSession()
	{
	if (iHierarchy && iHierarchy->iControllerSession == this)
		iHierarchy->iControllerSession = NULL;
	if (iHierarchy && iHierarchy->iObserverSession == this)
		iHierarchy->iObserverSession = NULL;
	}

class MyMessage : public RMessage2
	{
public:
	TInt* ArgRef(TInt i)
		{ return &iArgs[i]; }
	};

void CDmManagerSession::ServiceL(const RMessage2& aMessage)
	{
	TInt r;
	CDmSvrManager* manager = ((CDmManagerServer*) Server()) -> iManager;

	// Check client has ECapabilityPowerMgmt capability
/*	
    if (!KPowerMgmtPolicy.CheckPolicy(aMessage))
		{
		aMessage.Complete(KErrPermissionDenied);
		return;
		}
*/
	switch (aMessage.Function())
		{
		case EDmHierarchyAdd:
			{
			r = KErrNone;
			TDmHierarchyId hierarchyId = (TDmHierarchyId) aMessage.Int0();

			CDmHierarchy* hierarchy = manager->LookupHierarchy(hierarchyId);
			if (hierarchy == NULL)
				r = manager->BuildDomainTree(hierarchyId, hierarchy);
			aMessage.Complete(r);
			}
			break;

		case EDmHierarchyJoin:
			{
			r = KErrNone;
			TDmHierarchyId hierarchyId = (TDmHierarchyId) aMessage.Int0();

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
				(TDmDomainState) aMessage.Int0(),
				(TDmTraverseDirection) aMessage.Int1(),
				&aMessage);

			if (r != KErrNone)
				aMessage.Complete(r);
			break;

		case EDmRequestDomainTransition:
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
				(TDmDomainId) aMessage.Int0(), 
				(TDmDomainState) aMessage.Int1(), 
				(TDmTraverseDirection) aMessage.Int2(),
				&aMessage);

			if (r != KErrNone)
				aMessage.Complete(r);
			break;

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

CSession2* CDmDomainServer::NewSessionL(const TVersion&, const RMessage2&) const
	{

	return new CDmDomainSession();
	}

CSession2* CDmDomainServer::NewSessionL(const TVersion&) const
	{
	__DS_PANIC(KErrGeneral);
	return 0;
	}

CDmDomainSession::~CDmDomainSession()
	{
	if (iPending)
		iDomain->CompleteMemberTransition(KErrNone);
	if (iDomain)
		iDomain->Detach(this);
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
		iNotificationEnabled = ETrue;
		break;

	case EDmStateCancelTransitionNotification:
		iNotificationEnabled = EFalse;
		break;

	case EDmStateAcknowledge:
		{
		TInt propValue = aMessage.Int0();
		TInt error = aMessage.Int1();
		if (!iDomain)
			{
			r = KDmErrNotJoin;
			break;
			}
		if (iPending && iDomain->CheckPropValue(propValue))
			{
			iPending = EFalse;
			iDomain->CompleteMemberTransition(error);
			}
		}
		break;
	default:
		r = KDmErrBadRequest;
		break;
		}
	aMessage.Complete(r);
	}

	
TInt E32Main()
	{
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	if(!cleanupStack)
		__DS_PANIC(KErrNoMemory);

	CActiveScheduler* sched = new CActiveScheduler();
	if (!sched)
		__DS_PANIC(KErrNoMemory);

	CActiveScheduler::Install(sched);

	CDmSvrManager* mngr = CDmSvrManager::New();
	__DS_ASSERT(mngr);

	CDmManagerServer* msrv = new CDmManagerServer(mngr);
	if (!msrv)
		__DS_PANIC(KErrNoMemory);

	TInt r=msrv->Start(KDmManagerServerNameLit);
	if (r != KErrNone)
		__DS_PANIC(r);

	CDmDomainServer* dsrv = new CDmDomainServer(mngr);
	if (!dsrv)
		__DS_PANIC(KErrNoMemory);

	r=dsrv->Start(KDmDomainServerNameLit);
	if (r != KErrNone)
		__DS_PANIC(r);

	CActiveScheduler::Start();

	__DS_PANIC(0);

	return KErrNone;
	}
