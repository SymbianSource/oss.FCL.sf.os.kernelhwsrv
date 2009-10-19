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

#ifndef __DOMAIN_MEMBER_H__
#define __DOMAIN_MEMBER_H__

#include <e32std.h>
#include <e32property.h>
#include <domaindefs.h>

/**
@internalComponent
*/
class RDmDomainSession : public RSessionBase
	{
public:
	TInt Connect(TDmHierarchyId aHierarchyId, TDmDomainId, TUint* aKey);
	void Acknowledge(TInt aValue, TInt aError);
	void RequestTransitionNotification();
	void CancelTransitionNotification();
	};


/**
@publishedPartner
@released

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

	/**
	@internalAll
	*/
	IMPORT_C TInt Connect(TDmHierarchyId aHierarchyId, TDmDomainId aDomainId);
	/**
	@internalAll
	*/
	IMPORT_C void AcknowledgeLastState(TInt aError);
	/**
	@internalAll
	*/
	IMPORT_C TDmDomainState GetState();

private:
	RDmDomainSession	iSession;
	RProperty			iStateProperty;
	TInt				iLastStatePropertyValue;	
	};



/**
@publishedPartner
@released

An abstract class for interfacing to a domain managed by the domain manager.

To make use of this class an application must derive from it and implement a RunL()
method to handle notifications.
*/
class CDmDomain : public CActive
	{
public:
	IMPORT_C CDmDomain(TDmHierarchyId aHierarchyId, TDmDomainId aDomainId);
	IMPORT_C ~CDmDomain();

	IMPORT_C void RequestTransitionNotification();
	/**
	@internalTechnology
	*/
	IMPORT_C void AcknowledgeLastState(TInt aError);
	/**
	@internalTechnology
	*/
	IMPORT_C TDmDomainState GetState();
	virtual void RunL() = 0;
protected:
	// from CActive
	IMPORT_C void DoCancel();
	IMPORT_C void ConstructL();

private:
	RDmDomain iDomain;
	TDmHierarchyId iHierarchyId;
	TDmDomainId iDomainId;
	TInt iReserved[4];	
	};


#endif
