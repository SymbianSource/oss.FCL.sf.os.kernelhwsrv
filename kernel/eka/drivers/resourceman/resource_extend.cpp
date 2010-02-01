// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\resourceman\resource_extend.cpp
// 
//

#include <drivers/resourcecontrol.h>

/**
    @publishedPartner
    @prototype 9.5
    Constructor for static resource with dependency
    This sets the passed resource name and default level.
	This also sets the corresponding bit to identify that it is static resource with dependencies.
    @param aName The name for the resource to be set.
	@param aDefaultLevel Default level of the resource. 
    */
DStaticPowerResourceD::DStaticPowerResourceD(const TDesC8& aName, TInt aDefaultLevel) :
                                                           DStaticPowerResource(aName, aDefaultLevel)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DStaticPowerResourceD::DStaticPowerResourceD"));
	iResourceId |= KIdMaskStaticWithDependencies;
	}

/**
    @publishedPartner
    @prototype 9.5
    Constructor for dynamic resource
    This sets the passed resource name and default level and also sets the corresponding bit to identify 
	that it is dynamic resource with no dependencies.
    @param aName The name for the resource to be set.
	@param aDefaultLevel Default level of the resource. 
    */
EXPORT_C DDynamicPowerResource::DDynamicPowerResource(const TDesC8& aName, TInt aDefaultLevel) : 
	                                                       DStaticPowerResource(aName, aDefaultLevel)
	{
	//Set the corresponding to identify that it is dynamic resource with no dependencies
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DDynamicPowerResource::DDynamicPowerResource"));
	iResourceId |= KIdMaskDynamic;
	}

/**
    @publishedPartner
    @prototype 9.5
    Destructor for dynamic resource
	Panics if the resource is still registered with PRM.
    */
EXPORT_C DDynamicPowerResource::~DDynamicPowerResource()
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DDynamocPowerResource::~DDynamicPowerResource"));
	if(LockCount()) //Lock count is expected to be 0
		DPowerResourceController::Panic(DPowerResourceController::EDynamicResourceStillRegistered);	
	}

/**
    @publishedPartner
    @prototype 9.5
    Constructor for dynamic resource with dependencies
    This sets the passed resource name and default level and also sets the corresponding bit to identify 
	that it is dynamic resource with dependencies.
    @param aName The name for the resource to be set.
	@param aDefaultLevel Default level of the resource. 
    */
EXPORT_C DDynamicPowerResourceD::DDynamicPowerResourceD(const TDesC8& aName, TInt aDefaultLevel) : 
	                                                   DDynamicPowerResource(aName, aDefaultLevel)
	{
	//Set the corresponding to identify that it is dynamic resource with no dependencies
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DDynamicPowerResourceD::DDynamicPowerResourceD"));
	iResourceId |= KIdMaskDynamicWithDependencies;
	}

/**
    @publishedPartner
    @prototype 9.5
    Destructor for dynamic resource with dependencies
	Panics if the resource is still registered with PRM.
    */
EXPORT_C DDynamicPowerResourceD::~DDynamicPowerResourceD()
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DDynamocPowerResource::~DDynamicPowerResource"));
	if(LockCount()) //Lock count is expected to be 0
		DPowerResourceController::Panic(DPowerResourceController::EDynamicResourceStillRegistered);	
	}

/**
    @internalComponent
    @prototype 9.5
    This function is called in response to PowerResourceManager::ControlIO(..) with 
	KResManControlIoDeregisterDynamicResource and returns ETrue if this resource has a 
	'client level' from another client in its 'client level' list.
    */
TBool DDynamicPowerResource::InUse()
	{
	SPowerResourceClientLevel* pRCL = NULL;
	for(SDblQueLink* pRC = iClientList.First(); pRC != &iClientList.iA; pRC = pRC->iNext)
		{
		pRCL = (SPowerResourceClientLevel*)pRC;
		if(pRCL->iClientId != iOwnerId)
			return ETrue;
		}
	return EFalse;
	}

/**
    @publishedPartner
    @prototype 9.5
    This function is used to establish the resource's dependency list for static resource and 
	will be used by PSL to establish dependency between static resources.
	Panics, if the passed priority is already in use
    */
TInt DStaticPowerResourceD::AddNode(SNode* aNode)
	{
	ADD_DEPENDENCY_NODE(aNode, iDependencyList)
	return KErrNone;
	}

/**
	@publishedPartner
	@prototype 9.5
	This function takes care of resource state change of dynamic dependency resource.
	This propagates the change to all of its dependents.
	*/
EXPORT_C TInt DDynamicPowerResourceD::HandleChangePropagation(TPowerRequest aRequest, TPropagation aProp, TUint aOriginatorId, const TDesC8& aOriginatorName)
	{
	TInt result = KErrNone;
	static DPowerResourceController* pRC = TInterface::GetPowerResourceController();
	result = pRC->HandleResourceChange(aRequest, aProp, aOriginatorId, aOriginatorName, (DStaticPowerResourceD*)this);
	return result;
	}

