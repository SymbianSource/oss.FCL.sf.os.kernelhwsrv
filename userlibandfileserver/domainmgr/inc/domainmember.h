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
//
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __DOMAIN_MEMBER_H__
#define __DOMAIN_MEMBER_H__

#include <e32base.h>
#include <e32property.h>
#include <domaindefs.h>


class RDmDomainSession : public RSessionBase
	{
public:
	TInt Connect(TDmHierarchyId aHierarchyId, TDmDomainId, TUint* aKey);
	TInt Acknowledge(TInt aValue, TInt aError);
	void RequestTransitionNotification();
	void CancelTransitionNotification();
	void DeferAcknowledgement(TRequestStatus& aStatus);
	void CancelDeferral();
	};


/**
The application's interface to the domain manager.
*/
class RDmDomain
	{
public:
	IMPORT_C TInt Connect(TDmDomainId aId);
	IMPORT_C void Close();

	IMPORT_C void RequestTransitionNotification(TRequestStatus& aStatus);
	IMPORT_C void CancelTransitionNotification();

	IMPORT_C TPowerState GetPowerState();
	IMPORT_C void AcknowledgeLastState();

	IMPORT_C TInt Connect(TDmHierarchyId aHierarchyId, TDmDomainId aDomainId);
	IMPORT_C void AcknowledgeLastState(TInt aError);
	IMPORT_C TDmDomainState GetState();

	IMPORT_C void DeferAcknowledgement(TRequestStatus& aStatus);
	IMPORT_C void CancelDeferral();

private:
	friend class CDmDomainKeepAlive;
	TInt AcknowledgeLastStatePriv(TInt aError);

	RDmDomainSession	iSession;
	RProperty			iStateProperty;
	TInt				iLastStatePropertyValue;
	};


/**
An abstract class for interfacing to a domain managed by the domain manager.

To make use of this class an application must derive from it and implement a
RunL() method to handle notifications.
*/
class CDmDomain : public CActive
	{
public:
	IMPORT_C CDmDomain(TDmHierarchyId aHierarchyId, TDmDomainId aDomainId);
	IMPORT_C ~CDmDomain();

	IMPORT_C void RequestTransitionNotification();
	IMPORT_C void AcknowledgeLastState(TInt aError);
	IMPORT_C TDmDomainState GetState();
	virtual void RunL() = 0;

protected:
	// from CActive
	IMPORT_C void DoCancel();
	IMPORT_C void ConstructL();

	RDmDomain iDomain;

private:
	TDmHierarchyId iHierarchyId;
	TDmDomainId iDomainId;
	TInt iReserved[4];
	};


class CDmKeepAlive;

/**
This derived class extends the parent class by automatically deferring
transitions as long as possible after the original notification is received.

To make use of this class, derive and implement the HandleTransitionL()
function. HandleTransitionL() will be called when the transition notification
comes in. Thereafter, the active object will continually defer the transition.

This object is intended to simplify the handling of notifications and
deferrals. The member must ensure that other active objects do not block or
have long-running RunL()s; this is to ensure that the Active Scheduler will
remain responsive to the expiry of deadline deferrals.

The capabilities needed are the same as those needed for
RDmDomain::DeferAcknowledgement() (which this active object uses).

@capability WriteDeviceData
@capability ProtServ
@see RDmDomain::DeferAcknowledgement()
*/
class CDmDomainKeepAlive : public CDmDomain
	{
public:
	IMPORT_C CDmDomainKeepAlive(TDmHierarchyId aHierarchyId, TDmDomainId aDomainId);
	IMPORT_C ~CDmDomainKeepAlive();

	IMPORT_C void AcknowledgeLastState(TInt aError);

	/**
	The derived class active object will call this function to indicate the
	completion of the asynchronous call RequestTransitionNotification().

	The implementation of this function should be used first to call
	RequestTransitionNotification() again if required, and then to initiate the
	response to the transition. It should be kept as quick as possible, any
	slow operations (e.g. File Server calls) should be initiated asynchronously
	and handled using other active objects.

	Once the Domain Member's transition operations are complete, it should call
	AcknowledgeLastState() on this active object, to indicate it is ready to be
	transitioned.

	HandleTransitionL() should not call AcknowledgeLastState() unless it can
	trivially determine that no action at all is required for the given
	transition.
	*/
	virtual void HandleTransitionL() =0;

	IMPORT_C virtual TInt HandleDeferralError(TInt aError);

protected:
	IMPORT_C void ConstructL();

	IMPORT_C void RunL();

private:
	CDmKeepAlive* iKeepAlive;

	TUint32 iReservedSpace[2];
	};

#endif
