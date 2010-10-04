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
// Contains the common definitions for the Domain Manager interface for clients.
//
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __DOMAIN_DEFS_H__
#define __DOMAIN_DEFS_H__

#include <e32cmn.h>
#include <e32power.h>


/**
@internalTechnology
*/
// Property category UID value reserved for Domain Management services.
// This is the same as the process SID (see domainsrv.mmp)
static const TInt32 KUidDmPropertyCategoryValue = 0x1020E406;
/**
@internalTechnology
*/
// Property category UID reserved for Domain Management services
static const TUid KUidDmPropertyCategory = { KUidDmPropertyCategoryValue };
/**
@internalTechnology
*/
static const TUint KDmPropertyKeyInit				= 0x00000001;




/**
@internalAll

Defines the ways in which a domain hierarchy can be traversed.

@see CDmDomainManager::RequestSystemTransition()
@see CDmDomainManager::RequestDomainTransition()
*/
enum TDmTraverseDirection
	{
	/**
	Transition parents first (i.e. before their children).
	*/
	ETraverseParentsFirst,

	/**
	Transition children first (i.e. before their parents).
	*/
	ETraverseChildrenFirst,

	/**
	Use the default direction specified in policy
	*/
	ETraverseDefault,
	
	/**
	Maximum value for traverse mode
	*/
	ETraverseMax = ETraverseDefault
	};




/**
@internalAll

Defines the type of transitions that will be notified to the observer.

One of these values is specified when the observer is started through a 
call to CHierarchyObserver::StartObserver().
*/
enum TDmNotifyType
	{
	/**
	Notifies the observer about all transition requests.
	*/
	EDmNotifyTransRequest =0x02,
	
	/**
	Notifies the observer about successful transitions only.
	*/
	EDmNotifyPass =0x04, 
	
	/**
	Notifies the observer about failing transitions only.
	*/
	EDmNotifyFail=0x08, 
	
	/**
	Notifies the observer about all completed transitions.
	*/
	EDmNotifyTransOnly=EDmNotifyPass|EDmNotifyFail,
	
	/**
	Notifies the observer about successful transitions and transition requests.
	*/
	EDmNotifyReqNPass=EDmNotifyTransRequest|EDmNotifyPass,
	
	/**
	Notifies the observer about failed transitions and transition requests.
	*/
	EDmNotifyReqNFail=EDmNotifyTransRequest|EDmNotifyFail,
	
	/**
	Notifies the observer about every event.
	*/
	EDmNotifyAll=EDmNotifyTransRequest|EDmNotifyFail|EDmNotifyPass, 
	};




/**
@internalAll

Domain hierarchy identifier type.
 
Domain hierarchies are designated by "well known" domain hierarchy identifiers.
The domain policy statically defines the list of domain hierarchies and their 
identifiers.

@see RDmDomainManager::Connect()
@see RDmDomainManager::AddDomainHierarchy()
@see RDmDomain::Connect()
@see CHierarchyObserver
@see CDmDomainManager
@see CDmDomain
*/
typedef TUint8 TDmHierarchyId;




/**
@internalAll

A type used to describe the state of a domain.
*/
typedef TUint TDmDomainState;




/**
@internalTechnology

The power domain hierarchy Id. This hierarchy is used in development 
platforms.
*/
static const TDmHierarchyId	KDmHierarchyIdPower		= 1;




/**
@internalAll

The system state start-up and shutdown domain hierarchy Id. This hierarchy is
used in production devices.
*/
static const TDmHierarchyId	KDmHierarchyIdStartup	= 2;



/**
@internalAll

The start-up domain hierarchy Id. (For use by domain manager and/or SSMA)
*/
static const TDmHierarchyId KDmHierarchyIdSystemState = KDmHierarchyIdStartup;



/**
@publishedPartner
@released

Domain identifier type.
 
Domains are designated by "well known" domain identifiers.
The domain manager statically defines the list of domains and their identifiers.
*/
typedef TUint16 TDmDomainId;



/**
@internalAll

A structure use to describe a transition failure.
*/
class TTransitionFailure
	{
public:
	inline TTransitionFailure() {};
	TTransitionFailure(	TDmDomainId aDomainId, TInt aError);

public:
	/**
	Id of the domain of interest
	*/
	TDmDomainId iDomainId;
	/**
	error code in transition
	*/
	TInt iError;
	};


/**
@internalTechnology

A structure use to describe a successful transition.
*/
class TTransInfo
	{
public:
	inline TTransInfo() {};
	TTransInfo(	TDmDomainId aDomainId, TDmDomainState aState, TInt aError);

public:
	/**
	Id of the domain of interest
	*/
	TDmDomainId iDomainId;				 
	/**
	Final state of the domain after the transition 
	*/
	TDmDomainState iState;
	/**
	Any error in transition
	*/
	TInt iError;
	};



/**
The possible ways in which the domain manager can behave when a transition 
fails. 

@see TDmHierarchyPolicy
@see TDmStateSpecV1
@see DmPolicyGetPolicy
*/
enum TDmTransitionFailurePolicy
	{
	// Failure policies
	
	/**	The domain manager stops at the first transition failure */
	ETransitionFailureStop,

	/** The domain manager continues at any transition failure */
	ETransitionFailureContinue,


	// Special failure policies

	/** Used in SDmStateSpecV1 when the default failure policy for the 
	hierarchy should be used. The default failure policy is
	returned by the DmPolicyGetPolicy() function, ordinal 3 in
	the policy library. */
	ETransitionFailureUsePolicyFromOrdinal3 = 256
	};



/**
@publishedPartner
@released

The null domain identifier.

There are no domains with this identifier.
*/
static const TDmDomainId	KDmIdNone	= 0x00;




/**
@publishedPartner
@released

The common ancestor of all domains.
*/
static const TDmDomainId	KDmIdRoot	= 0x01;




/**
@publishedPartner
@released

The usual domain for all non-UI applications.
*/
static const TDmDomainId	KDmIdApps	= 0x02;




/**
@publishedPartner
@released

The usual domain for all UI applications.
*/
static const TDmDomainId	KDmIdUiApps	= 0x03;




/**
@publishedPartner
@released

Domain manager specific error code - the domain designated by
the specified domain identifier does not exist.
*/
static const TInt KDmErrBadDomainId		= -256;




/**
@publishedPartner
@released

Domain manager specific error code - this RDmDomain object has already been
connected to a domain.
*/
static const TInt KDmErrAlreadyJoin		= -257;




/**
@publishedPartner
@released

Domain manager specific error code - this RDmDomain object is not connected
to any domain.
*/
static const TInt KDmErrNotJoin			= -258;




/**
@publishedPartner
@released

Domain manager specific error code - indicates a client-server protocol internal error.
*/
static const TInt KDmErrBadRequest		= -259;




/**
@publishedPartner
@released

Domain manager specific error code - indicates an internal Domain Manager error.
*/
static const TInt KDmErrBadDomainSpec	= -260;




/**
@publishedPartner
@released

Domain manager specific error code - indicates a bad sequence of requests.

Typically, this occurs when a new request been made while an ongoing domain
transition request has not yet completed.
*/
static const TInt KDmErrBadSequence		= -261;




/**
@internalTechnology

Domain manager specific error code - indicates that a transition is outstanding.
 
*/
static const TInt KDmErrOutstanding		= -262;




/**
@internalAll

Domain manager specific error code - indicates that the domain hierarchy does not exist.
*/
static const TInt KErrBadHierarchyId	= -263;




#endif

