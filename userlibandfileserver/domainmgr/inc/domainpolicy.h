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

#ifndef __DOMAIN_POLICY_H__
#define __DOMAIN_POLICY_H__

#include <e32std.h>

#include <domaindefs.h>




/**
@publishedPartner
@released

Defines the characteristics of a domain.
*/
struct TDmDomainSpec
	{
	/**
	The domain identifier.
	*/
	TDmDomainId					iId;
	
	/**
	The domain identifier of the domain's parent.
	*/
	TDmDomainId					iParentId;

	/**
	The security capability required to join this domain
	*/
	TStaticSecurityPolicy		iJoinPolicy;
	
	/**
	The initial state of the domain after construction.
	*/
	TDmDomainState				iInitState;
	
	/**
	The total time allowed for members of the domain to acknowledge
	a transition.
	*/
	TUint32						iTimeBudgetUs;
	};




/**
@internalAll

The possible ways in which the domain manager can behave
when a transition fails.

This is defined for each domain hierarchy.

@see TDmHierarchyPolicy
*/
enum TDmTransitionFailurePolicy
	{
	/**
	The domain manager stops at the first transition failure.
	*/
	ETransitionFailureStop,

	/**
	The domain manager continues at any transition failure.
	*/
	ETransitionFailureContinue
	};



/**
@internalTechnology

Defines the policy for a particular domain hierarchy.
*/
class TDmHierarchyPolicy
	{
public:
	/**
	direction of traverse if target state is after current state
	*/
	TDmTraverseDirection iPositiveTransitions;
	/**
	direction of traverse if target state is before current state
	*/
	TDmTraverseDirection iNegativeTransitions;
	/**
	policy which outlines the action upon transition failure
	*/
	TDmTransitionFailurePolicy iFailurePolicy;
	};


/**
@internalAll

Defines the function type for a static function that is implemented by
a device's domain policy DLL.

The domain manager uses this function to access the hierarchy's policy.
*/
typedef const TDmDomainSpec* (*DmPolicyGetDomainSpecs)();


/**
@internalAll

Defines the function type for a static function that is implemented by
a device's domain policy DLL.

The domain manager uses this function to release the domain
hierarchy specification.
*/
typedef void (*DmPolicyRelease) (const TDmDomainSpec* aDomainSpec);


/**
@internalAll

Defines the function type for a static function that is implemented by
a device's domain policy DLL.

The domain manager uses this function to access the domain
hierarchy specification.
*/
typedef TInt (*DmPolicyGetPolicy) (TDmHierarchyPolicy& aPolicy);

/**
@publishedPartner
@released

A set of static functions implemented by a device's domain policy DLL that
the domain manager uses to access, and release, the domain
hierarchy specification.
*/
class DmPolicy
	{
public:
	IMPORT_C static const TDmDomainSpec* GetDomainSpecs();
	IMPORT_C static void Release(const TDmDomainSpec* aDomainSpec);
	IMPORT_C static TInt GetPolicy(TDmHierarchyPolicy& aPolicy);
	};


/**
@internalTechnology
*/
enum DmPolicyOrdinals
	{
	EDmPolicyGetDomainSpecs = 1,
	EDmPolicyRelease,
	EDmPolicyGetPolicy
	};

#endif
