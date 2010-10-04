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
// Contains the Domain Manager interface for clients providing policy 
// information. Typically this is the same client that acts in the 
// "Domain Controller" role.
//
// There are two versions of the policy API - the original V1 and the new V2.
// V2 builds upon V1 and specifies the states which clients are allowed to defer 
// their transition timeout if they have not finished, up to a max number of 
// times. This is part of the Domain Manager Transition Monitoring feature.
//
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
Defines the characteristics of a domain.
*/
struct TDmDomainSpec
	{
	/**	The 16-bit domain identifier */
	TDmDomainId					iId;
	
	/**	The domain identifier of the domain's parent */
	TDmDomainId					iParentId;

	/**	The security capability required to join this domain */
	TStaticSecurityPolicy		iJoinPolicy;
	
	/**	The initial state of the domain after construction */
	TDmDomainState				iInitState;
	
	/**	The total time allowed for the domain to complete a state transition.
	Members of the domain must acknowledge a transition within this time.
	Measured in microseconds. */
	TUint32						iTimeBudgetUs;
	};

#define TDM_DOMAIN_SPEC_END  {KDmIdNone, KDmIdNone,	_INIT_SECURITY_POLICY_PASS,	0, 0}



/**
Defines the overall traversal and failure policy for a particular 
domain hierarchy and is returned by DmPolicy::GetPolicy().
Note the failure policy can be overridden for individual 
states in V2 policies. 

@see TDmTransitionFailurePolicy
*/
class TDmHierarchyPolicy
	{
public:
	/** Direction of traversal if target state is after current state */
	TDmTraverseDirection iPositiveTransitions;
	
	/**	Direction of traversal if target state is before current state */
	TDmTraverseDirection iNegativeTransitions;
	
	/** Policy which outlines the action upon transition failure */
	TDmTransitionFailurePolicy iFailurePolicy;
	};



/**
Defines the characteristics of a state and is used by entry points introduced
in version 2 of the policy API. The structure itself is also versioned with V1 
in use with version 2 policy libraries. 

Policy providers provide an object of this type for each state they want to:
- enable the Transition Monitoring feature or/and
- override the default failure policy

Enabling transition monitoring will allow trusted clients to receive more time
to complete their work before final acknowledgment. Enable transition 
monitoring to ensure a fair completion of the transition e.g. shutdown system.
Where transition monitoring is enabled the Domain level timer is not used
and iTimeBudgetUs provided in TDmDomainSpec is ignored.

The global failure policy is returned by DmPolicy::GetPolicy() and in V1 
policies this applies to all state transitions. iFailurePolicy in this 
structure allows different failure policies for different state transitions. 

@see DmPolicy::GetStateSpec()
*/
struct SDmStateSpecV1
	{
	/**	The destination state of the transition */
	TDmDomainState		iState;
	
	/** Transition Monitoring: member transition timeout granularity, in 
	milliseconds, e.g. 200ms. Set to 0 to not use transition monitoring for 
	this state. */
	TInt16				iTimeoutMs;	
		
	/** Transition Monitoring: maximum number of times a domain member may be 
	granted an additional timeout period. Set to 0 when transition 
	monitoring not used for this state. */	
	TInt16				iDeferralLimit;	
	
	/** Specifies the failure policy for transitions to the target state, see
	TDmTransitionFailurePolicy. Overrides the global value returned by 
	the policy function DmPolicyGetPolicy(). 
	Set to ETransitionFailureUsePolicyFromOrdinal3 if override not required. */	
	TInt16			    iFailurePolicy;
	};
	
const TInt KSDmStateSpecV1 = 1;



/**
Defines the function type for a static function that is implemented by
a device's domain policy DLL at ordinal 1.
The domain manager uses this function to access the domain
hierarchy specification. The pointer returned must point to an array of
TDmDomainSpec objects where the last object in the array is defined
using the END macro, as shown below.
@code
	. . .
	TDM_DOMAIN_SPEC_END
	};
@endcode

The implementation should return NULL if it is unable to supply the requested
information due to an error. This will abort policy loading and hierarchy 
creation. 
The implementation must never panic or leave in any way.

@see DmPolicy
*/
typedef const TDmDomainSpec* (*DmPolicyGetDomainSpecs)();


/**
Defines the function type for a static function that is implemented by
a device's domain policy DLL at ordinal 2. The domain manager uses this 
function to release the domain hierarchy specification returned by ordinal 1.
The implementation must never panic or leave in any way.

@see DmPolicy
*/
typedef void (*DmPolicyRelease) (const TDmDomainSpec* aDomainSpec);


/**
Defines the function type for a static function that is implemented by
a device's domain policy DLL at ordinal 3. The domain manager uses this 
function to access the hierarchy's policy.
The implementation may return a system-wide error code if it encounters an
error. This will abort policy loading and hierarchy creation. 
The implementation must never panic or leave in any way.

@see DmPolicy
*/
typedef TInt (*DmPolicyGetPolicy) (TDmHierarchyPolicy& aPolicy);


/**
Defines the function type for a static function that is implemented by
a device's domain policy DLL at ordinal 4. The domain manager uses this
function to obtain the state specification to configure the client transition
monitoring feature. 
This method is new in V2 of the domain policy and should not be present in 
V1 policy library.
The implementation must never panic or leave in any way.

The implementation returns either an error or the version of the state 
specification structure used in the array pointed to by aPtr on exit. 
When the return value is >=1, aNumElements must be >0.
Where a state specification is not required (i.e. client transition monitoring 
is not required) the implementation returns 0. When an error occurs a negative
system-wide error code is returned. In both these cases the output parameters are
ignored.

@see SDmStateSpecV1
@see DmPolicy
*/
typedef TInt (*DmPolicyGetStateSpec) (TAny*& aPtr, TUint& aNumElements);

/**
Defines the function type for a static function that is implemented by
a device's domain policy DLL at ordinal 5. The domain manager uses this 
function to release the state specification returned by ordinal 4. The 
implementation may be empty and simply return if no release action needs 
to be taken.

This method is new in V2 of the domain policy and should not be present in 
V1 policy library.
The implementation must never panic or leave in any way.

@see DmPolicy
*/
typedef void (*DmPolicyReleaseStateSpec) (TAny* aStateSpec);



/**
A set of static functions implemented in a domain hierarchy policy DLL that
the domain manager uses to access and release the domain hierarchy and 
domain state specifications.

@see DmPolicyOrdinals
*/
class DmPolicy
	{
public:
	// Original policy version methods
	IMPORT_C static const TDmDomainSpec* GetDomainSpecs();
	IMPORT_C static void Release(const TDmDomainSpec* aDomainSpec);
	IMPORT_C static TInt GetPolicy(TDmHierarchyPolicy& aPolicy);
	
	// Version 2 methods
	IMPORT_C static TInt GetStateSpec(TAny*& aPtr, TUint& aNumElements);
	IMPORT_C static void ReleaseStateSpec(TAny* aStateSpec);
	};



/**
Describes the purpose (and thus each function prototype) of each ordinal in the
policy DLL. There are two versions of this DLL in use:
- V1 DLLs implement ordinals 1..3
- V2 DLLs implement ordinals 1..5 

@see DmPolicy 
*/
enum DmPolicyOrdinals
	{
	// Policy DLL version 1 ordinals 
	EDmPolicyGetDomainSpecs = 1,
	EDmPolicyRelease,
	EDmPolicyGetPolicy,
	
	// Policy DLL version 2 ordinals for the "Transition Monitoring" feature.
	// These entry points are not needed in V1 policy libraries.
	EDmPolicyGetStateSpec,
	EDmPolicyReleaseStateSpec			
	};



#endif
