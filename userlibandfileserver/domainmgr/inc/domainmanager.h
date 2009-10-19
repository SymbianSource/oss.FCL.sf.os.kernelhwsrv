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
//
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __DOMAIN_MANAGER_H__
#define __DOMAIN_MANAGER_H__

#include <e32std.h>

#include <domaindefs.h>

/**
@internalComponent
*/
#define KDmManagerFileNameLit	_L("domainSrv.exe")

/**
@internalComponent
*/
class RDmManagerSession : public RSessionBase
	{
public:
	TInt Connect();
	TInt Connect(TDmHierarchyId aHierarchyId);
	TInt ConnectObserver(TDmHierarchyId aHierarchyId);
	void RequestSystemTransition(TDmDomainState aState, TDmTraverseDirection aDirection, TRequestStatus& aStatus);
	void RequestDomainTransition(TDmDomainId, TDmDomainState, TDmTraverseDirection aDirection, TRequestStatus& aStatus);
	void CancelTransition();
	TInt AddDomainHierarchy(TDmHierarchyId aHierarchyId);
	TInt GetTransitionFailureCount();
	TInt GetTransitionFailures(RArray<const TTransitionFailure>& aTransitionFailures);
	// Observer related functions
	TInt StartObserver(TDmDomainId aDomainId,TDmNotifyType aNotifyType);
	void GetNotification(TRequestStatus& aStatus);
	TInt GetEventCount();
	TInt GetEvents(RArray<const TTransInfo>& aTransitions);
	void CancelObserver();
	TInt ObserverDomainCount();
	};


/**
@publishedPartner
@released

The policy's interface to the domain manager.
*/
class RDmDomainManager
	{
public:
	IMPORT_C static TInt WaitForInitialization();
public:
	IMPORT_C TInt Connect();
	IMPORT_C void Close();

	IMPORT_C void RequestSystemTransition(TPowerState aState, TRequestStatus& aStatus);
	IMPORT_C void RequestDomainTransition(TDmDomainId aDomain, TPowerState aState, TRequestStatus& aStatus);
	IMPORT_C void CancelTransition();

	IMPORT_C void SystemShutdown();
	/**
	@internalAll
	*/
	IMPORT_C TInt Connect(TDmHierarchyId aHierarchyId);
	/**
	@internalAll
	*/
	IMPORT_C void RequestSystemTransition(TDmDomainState aState, TDmTraverseDirection aDirection, TRequestStatus& aStatus);
	/**
	@internalAll
	*/
	IMPORT_C void RequestDomainTransition(TDmDomainId aDomain, TDmDomainState aState, TDmTraverseDirection aDirection, TRequestStatus& aStatus);
	/**
	@internalAll
	*/
	IMPORT_C static TInt AddDomainHierarchy(TDmHierarchyId aHierarchyId);
	/**
	@internalAll
	*/
	IMPORT_C TInt GetTransitionFailures(RArray<const TTransitionFailure>& aTransitionFailures);
	/**
	@internalAll
	*/
	IMPORT_C TInt GetTransitionFailureCount();
private:
	RDmManagerSession	iSession;
	};




/**
@internalComponent

An abstract class to allow a domain controller to interface to the domain manager.

To make use of this class an application must derive from it and implement a RunL() 
method to handle notifications.
After the derived class has been instantiated, it must call ConstructL().
*/
class CDmDomainManager : public CActive
	{
public:
	IMPORT_C CDmDomainManager(TDmHierarchyId aHierarchyId);
	IMPORT_C ~CDmDomainManager();

	IMPORT_C void RequestSystemTransition(TDmDomainState aState, TDmTraverseDirection aDirection);
	IMPORT_C void RequestDomainTransition(TDmDomainId aDomainId, TDmDomainState aState, TDmTraverseDirection aDirection);
	IMPORT_C static TInt AddDomainHierarchy(TDmHierarchyId aHierarchyId);
	IMPORT_C TInt GetTransitionFailures(RArray<const TTransitionFailure>& aTransitionFailures);
	IMPORT_C TInt GetTransitionFailureCount();
	virtual void RunL() = 0;
protected:
	// from CActive
	IMPORT_C void DoCancel();
	IMPORT_C void ConstructL();

private:
	RDmDomainManager iManager;
	TDmHierarchyId iHierarchyId;
	TInt iReserved[4];	
	};

#endif
