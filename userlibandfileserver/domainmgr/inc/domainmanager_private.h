// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Contains private Domain Manager interface internal to the Kernel Hardware
// Services package.
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __DOMAIN_MANAGER_PRIVATE_H__
#define __DOMAIN_MANAGER_PRIVATE_H__

#include <e32base.h>
#include <domaindefs.h>
#include <domainmanager.h>


/**
Internal class, no clients may use this class.
Not to be used outside the KernelHwSrv package.
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
