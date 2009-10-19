// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __DOMAIN_OBSERVER_H__
#define __DOMAIN_OBSERVER_H__

#include <e32std.h>

#include <domaindefs.h>
#include <domainmanager.h>


/**
@internalTechnology

An interface to the domain manager for an observer.

To make use of this class, an application must derive from it and implement the pure vitual functions
to handle transition event notifications.
*/
class MHierarchyObserver
	{
public:
	/**
	When the observer is active, then upon any successful transition, this will be called indicating 
	the Id of the transitioned domain and the state of the domain after the transition. Clients are 
	expected to use this for debugging purpose.
	*/
	virtual void TransProgEvent(TDmDomainId aDomainId, TDmDomainState aState) = 0;
	/**
	When observer is active, then upon any failing transition, this will be called indicating 
	the Id of the domain, the state of the domain for the transition and the error occured. 
	Clients are expected to use this for debugging purpose.
	*/
	virtual void TransFailEvent(TDmDomainId aDomainId, TDmDomainState aState, TInt aError) = 0;
	/**
	When observer is active, then upon any request for transition, this will be called indicating 
	the Id of the domain and the desired state of the domain. 
	Clients are expected to use this for debugging purpose.
	*/
	virtual void TransReqEvent(TDmDomainId aDomainId, TDmDomainState aState) = 0;
	};



 
/**
@internalTechnology

An abstract class to allow a domain controller to interface to the domain manager.

To make use of this class an application must derive from it.
*/
class CHierarchyObserver: public CActive
	{
public:
	/**
	@internalAll
	*/
	IMPORT_C static CHierarchyObserver* NewL(MHierarchyObserver& aHierarchyObserver,TDmHierarchyId aHierarchyId);
	/**
	@internalAll
	*/
	IMPORT_C ~CHierarchyObserver();
	/**
	@internalAll
	*/
	IMPORT_C TInt StartObserver(TDmDomainId aDomainId, TDmNotifyType aNotifyType);
	/**
	@internalAll
	*/
	IMPORT_C TInt StopObserver();
	/**
	@internalAll
	*/
	IMPORT_C TInt ObserverDomainCount();

private:
	CHierarchyObserver(MHierarchyObserver& aHierarchyObserver,TDmHierarchyId aHierarchyId);
	void RunL();
	void DoCancel();
	void GetNotification();
	TInt GetEvents();

private:
	RDmManagerSession iSession;
	TDmHierarchyId iHierarchyId; 
	MHierarchyObserver& iObserver;
	TDmNotifyType iNotifyType;
	TDmDomainId iDomainId;
	TBool iObserverStarted;
	RArray<const TTransInfo> iTransitionEvents;
	};

#endif
