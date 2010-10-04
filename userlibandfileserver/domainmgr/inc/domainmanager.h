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
// Contains the Domain Manager interface for clients acting in the role of
// "Domain Controller".
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
Internal macro used by EStart source. It provides the filename of the Domain 
Manager server.
Not to be used outside the KernelHwSrv package.
*/
#define KDmManagerFileNameLit	_L("domainSrv.exe")



/**
Internal session class used inside the Domain Manager client library. 
Not to be used outside the KernelHwSrv package.
*/
class RDmManagerSession : public RSessionBase
	{
public:
	// Power hierarchy connect	
	TInt Connect();
	 	
	// Specified hierarchy connect
	TInt Connect(TDmHierarchyId aHierarchyId);
	TInt ConnectObserver(TDmHierarchyId aHierarchyId);
	
	// Controller related functions
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
Published platform interface of the Domain Manager for clients performing
the "Domain Controller" role implementing domain policy on the platform.

Controllers can use this class to request domain state transitions either
system-wide across the whole domain hierarchy or only within a specific domain 
subtree. 

The Domain Manager may utilize the Transition Monitoring feature for some state
transitions (as defined in the policy) which allows trusted clients to request
more time to complete their work for the transition e.g. power down transitions.

It also provides utility methods allowing policy controllers to load additional
hierarchies (from a domain policy library) and to monitor the Domain Manager's
start-up.

See the RDmDomain class for the interface used by Domain Member clients. 
 
Also see domaindefs.h for Domain Manager specific error codes used with this
API.
*/
class RDmDomainManager
	{
public:
	IMPORT_C static TInt WaitForInitialization();
		
public:
	// Power Hierarchy
	IMPORT_C TInt Connect();
	IMPORT_C void RequestSystemTransition(TPowerState aState, TRequestStatus& aStatus);
	IMPORT_C void RequestDomainTransition(TDmDomainId aDomain, TPowerState aState, TRequestStatus& aStatus);
	IMPORT_C void SystemShutdown();

	// Controller Specified Hierarchy
	IMPORT_C static TInt AddDomainHierarchy(TDmHierarchyId aHierarchyId);
	IMPORT_C TInt Connect(TDmHierarchyId aHierarchyId);
	IMPORT_C void RequestSystemTransition(TDmDomainState aState, TDmTraverseDirection aDirection, TRequestStatus& aStatus);
	IMPORT_C void RequestDomainTransition(TDmDomainId aDomain, TDmDomainState aState, TDmTraverseDirection aDirection, TRequestStatus& aStatus);
	
	// Common session/hierarchy 
	IMPORT_C TInt GetTransitionFailures(RArray<const TTransitionFailure>& aTransitionFailures);
	IMPORT_C TInt GetTransitionFailureCount();
	IMPORT_C void CancelTransition();
	IMPORT_C void Close();
	
private:
	RDmManagerSession	iSession;
	};



#endif
