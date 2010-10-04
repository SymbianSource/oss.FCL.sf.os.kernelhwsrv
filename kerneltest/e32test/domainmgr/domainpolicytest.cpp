// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <domainpolicy.h>
#include "domainpolicytest.h"


/**
Gets access to the test hierarchy specification.

The domain hierarchy specification is a simple array of TDmDomainSpec items.

The default implementation provided by Symbian OS just returns a pointer to
the domain hierarchy specification array.

@return A pointer to the domain hierarchy specification array.
*/
EXPORT_C const TDmDomainSpec* DmPolicy::GetDomainSpecs()
	{
	return DomainHierarchy;
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
Retrieves the domain hierarchy policy

@param	aPolicy a client-supplied policy which on exit
		will contain a copy of the policy for the requested domain hierarchy id.


@return	KErrNone
*/
EXPORT_C TInt DmPolicy::GetPolicy(TDmHierarchyPolicy& aPolicy)
	{
	aPolicy = HierarchyPolicy;
	return KErrNone;
	}


#ifdef DOMAIN_POLICY_V2

EXPORT_C TInt DmPolicy::GetStateSpec(TAny*& aPtr, TUint& aNumElements)
	{
	aNumElements = StateSpecificationSize;
	if (StateSpecificationSize)
		aPtr = (TAny*) StateSpecification;
	else
		aPtr = NULL;
	return StateSpecificationVersion;
	}


EXPORT_C void DmPolicy::ReleaseStateSpec(TAny* /*aStateSpec*/)
	{
	}

#endif


