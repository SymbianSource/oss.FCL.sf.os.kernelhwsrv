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
// domain\src\domainpolicy.cpp
// 
//

#include <domainpolicy.h>

const TInt KDomainTimeout = 1000000; /* 1000ms */

// Domain spec and policy for the domain hierarchy
static const TDmDomainSpec DomainHierarchy[] = 
	{
		{ KDmIdRoot,	KDmIdNone,	_INIT_SECURITY_POLICY_C1(ECapabilityWriteDeviceData),	EPwActive,	KDomainTimeout	},
		{ KDmIdApps,	KDmIdRoot,	_INIT_SECURITY_POLICY_PASS,								EPwActive,	KDomainTimeout	},
		{ KDmIdUiApps,	KDmIdRoot,	_INIT_SECURITY_POLICY_PASS,								EPwActive,	KDomainTimeout	},
		// end of array marker
		TDM_DOMAIN_SPEC_END	
	};

static const TDmHierarchyPolicy HierarchyPolicyPower	= 
	{ETraverseChildrenFirst, ETraverseParentsFirst, ETransitionFailureContinue};



/**
Gets access to the domain hierarchy specification.

The domain hierarchy specification is a simple array of TDmDomainSpec items.

The default implementation provided by Symbian OS just returns a pointer to
the domain hierarchy specification array.

@return A pointer to the domain hierarchy specification array.
*/
EXPORT_C const TDmDomainSpec* DmPolicy::GetDomainSpecs()
	{
	return (TDmDomainSpec*) DomainHierarchy;
	}




/**
Releases access to the specified domain hierarchy specification.

The domain hierarchy specification is a simple array of TDmDomainSpec items.

As the default Symbian OS implementation of GetDomainSpecs() just returns
a pointer to the domain hierarchy specification array, then the default
implementation of Release() is empty. The API is provided to permit
more complex implementations, if required.

@param aDomainSpec A pointer to the domain hierarchy specification array.
*/
EXPORT_C void DmPolicy::Release(const TDmDomainSpec* /*aDomainSpec*/)
	{
	}


/**
Retrieves the domain hierarchy policy. 

@param	aPolicy a client-supplied policy which on exit
		will contain a copy of the policy for the requested domain hierarchy Id.

  
@return	KErrNone
*/
EXPORT_C TInt DmPolicy::GetPolicy(TDmHierarchyPolicy& aPolicy)
	{
	aPolicy = HierarchyPolicyPower;
	return KErrNone;
	}

