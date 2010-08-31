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
// e32\drivers\resourceman\resourcecontrol_extended.cpp
// 
//

#include <drivers/resourcecontrol.h>
		
extern DPowerResourceController* PowerResourceController;
/**
@internalComponent
@prototype 9.5

Reserves the client level from pool to be used for updating resource client level in dependency resource.

@param aCount Number of client levels to reserve from pool

@return KErrNone On success 
@return KErrNoMemory Not enough memory to grow the pool
*/
TInt DPowerResourceController::ReserveClientLevelPoolCount(TUint16 aCount)
	{
	if(aCount < iResourceLevelPoolCount) 
		iResourceLevelPoolCount = (TUint16)(iResourceLevelPoolCount - aCount);
	else
		{
		TUint allocCount = (iStaticResDependencyArray.Count() / 2) + aCount;
		// coverity[alloc_fn]
		SPowerResourceClientLevel* pCL = new SPowerResourceClientLevel[allocCount];
		if(!pCL)
			return KErrNoMemory;
		for(TUint count = 0;count<(TUint)(allocCount);count++)
			LIST_PUSH(iResourceLevelPool, &pCL[count], iNextInList);
		iResourceLevelPoolCount= (TUint16)(iResourceLevelPoolCount + (iStaticResDependencyArray.Count() / 2));
#ifdef PRM_INSTRUMENTATION_MACRO
		TUint size = allocCount * sizeof(SPowerResourceClientLevel);
		PRM_MEMORY_USAGE_TRACE
#endif
		}
	return KErrNone;
	}

/**
@internalComponent
@prototype 9.5

Return a client level object from pool.

@param aLevelPtr Pointer to update the client level object.

@return None 
*/
void DPowerResourceController::RemoveClientLevelFromPool(SPowerResourceClientLevel *&aLevelPtr)
	{
	LIST_POP(iResourceLevelPool, aLevelPtr, iNextInList);
	return;
	}

/**
Update with number of dependent resources for the specified resource. 
*/
TInt DPowerResourceController::GetNumDependentsForResource(TUint aResourceId, TUint* aNumResources)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::GetNumDependentsForResource"));
	
	if(!(aResourceId & KIdMaskResourceWithDependencies))
		return KErrNotSupported;
	SNode* pN;
	if(aResourceId & KIdMaskDynamic)
		{
		DDynamicPowerResourceD* pDR = iDynamicResDependencyList[(aResourceId & ID_INDEX_BIT_MASK)];		
		if(!pDR)														
			return KErrNotFound;
		pN = pDR->iDependencyList;
		}
	else
		{
		if((aResourceId & ID_INDEX_BIT_MASK) > (TUint)iStaticResDependencyArray.Count())
			return KErrNotFound;
		DStaticPowerResourceD* pDR = iStaticResDependencyArray[(aResourceId & ID_INDEX_BIT_MASK) - 1];
		pN = pDR->iDependencyList;
		}
	*aNumResources = 0;
	for(;pN != NULL; pN = pN->iNext)
		(*aNumResources)++;
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::GetNumDependentsForResource"));
	return KErrNone;
	}

/**
Update the specified array with dependent resource Id's of the specified resource. 
*/
TInt DPowerResourceController::GetDependentsIdForResource(TUint aResourceId, TAny* aInfo, TUint* aNumDepResources)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::GetDependentsIdForResource"));

	if(!(aResourceId & KIdMaskResourceWithDependencies))
		return KErrNotSupported;
	
	if(!aInfo || !*aNumDepResources)
		{
		return KErrArgument;
		}

	TDes8 *pInfo = (TDes8*)aInfo;
	
	if((TUint)(pInfo->MaxLength() - pInfo->Length()) < (sizeof(SResourceDependencyInfo)*(*aNumDepResources)))
		return KErrArgument;
     
	SResourceDependencyInfo sResDepInfo;
	
	SNode* pN;
	if(aResourceId & KIdMaskDynamic)
		{
		DDynamicPowerResourceD* pDR = iDynamicResDependencyList[(aResourceId & ID_INDEX_BIT_MASK)];
		if(!pDR)
			return KErrNotFound;
		pN = pDR->iDependencyList;
		}
	else
		{
		if((aResourceId & ID_INDEX_BIT_MASK) > (TUint)iStaticResDependencyArray.Count())
			return KErrNotFound;
		DStaticPowerResourceD* pDR = iStaticResDependencyArray[(aResourceId & ID_INDEX_BIT_MASK) -1];
		pN = pDR->iDependencyList;
		}
	TUint count = 0;
	TUint resCount = 0;

	for(; pN != NULL; pN = pN->iNext)
		{
		resCount++;
		if(count == *aNumDepResources)
			continue;
		sResDepInfo.iResourceId = pN->iResource->iResourceId;
		sResDepInfo.iDependencyPriority = pN->iPriority;
		pInfo->Append(TPckgC<SResourceDependencyInfo>(sResDepInfo));
		}
	*aNumDepResources = resCount;
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::GetDependentsIdForResource"));
	return KErrNone;
	}

/**
Registers resource dependency. This could be between 2 dynamic resource or between
dynamic and static resource.
*/
TInt DPowerResourceController::RegisterResourceDependency(SPowerResourceClient* aClientPtr, SResourceDependencyInfo* aInfo1, 
														              SResourceDependencyInfo* aInfo2)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DExtendedResourceController::RegisterResourceDependency"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("ClientId = 0x%x, ResourceId1 = 0x%x, ResourceId2 = 0x%x", 
		                             aClientPtr->iClientId, aInfo1->iResourceId, aInfo2->iResourceId));
		                             
	if(iDfcQDependencyLock)
		{
		__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::RegisterResourceDependency::Resource In Use"));
		return KErrInUse;
		}
	
	TInt r = KErrNone;
	//One of the resource must be dynamic resource
	if(!(aInfo1->iResourceId & KIdMaskDynamic) && !(aInfo2->iResourceId & KIdMaskDynamic))
		return KErrNotSupported;
	//Both the resources should have dependency resource bit set in its id. 
	if(!(aInfo1->iResourceId & KIdMaskResourceWithDependencies) || !(aInfo2->iResourceId & KIdMaskResourceWithDependencies))
		return KErrNotSupported;
	
	DDynamicPowerResourceD* pR1 = NULL;
	DDynamicPowerResourceD* pR2 = NULL;
	SNode* pN1 = NULL;
	SNode* pN2 = NULL;
	//Retrieve resource1 from the corresponding list.
	if(aInfo1->iResourceId & KIdMaskDynamic)
		{
		pR1 = iDynamicResDependencyList[(aInfo1->iResourceId & ID_INDEX_BIT_MASK)];
		if(!pR1)
			return KErrNotFound;
		pN1 = pR1->iDependencyList;
		}
	else 
		{
		if((aInfo1->iResourceId & ID_INDEX_BIT_MASK) > (TUint)iStaticResDependencyArray.Count())
			return KErrNotFound;
		pR1 = (DDynamicPowerResourceD*)iStaticResDependencyArray[(aInfo1->iResourceId & ID_INDEX_BIT_MASK) - 1];
		pN1 = ((DStaticPowerResourceD*)pR1)->iDependencyList;
		}
	//Retrieve resource2 from the corresponding list.
	if(aInfo2->iResourceId & KIdMaskDynamic)
		{
		pR2 = iDynamicResDependencyList[(aInfo2->iResourceId & ID_INDEX_BIT_MASK)];
		if(!pR2)
			return KErrNotFound;
		pN2 = pR2->iDependencyList;
		}
	else
		{
		if((aInfo2->iResourceId & ID_INDEX_BIT_MASK) > (TUint)iStaticResDependencyArray.Count())
			return KErrNotFound;
		pR2 = (DDynamicPowerResourceD*)iStaticResDependencyArray[(aInfo2->iResourceId & ID_INDEX_BIT_MASK) - 1];
		pN2 = ((DStaticPowerResourceD*)pR2)->iDependencyList;
		}

	//Only long latency resource is allowed to have dependents.
	if(!pR1->LatencySet())
		pR1->iFlags |= KLongLatencySet;
	if(!pR2->LatencySet())
		pR2->iFlags |= KLongLatencySet;
	
	//Check for closed loop
	//NOTE: Panics, if any closed loop is encountered
	if(pN1)
		CheckForDependencyLoop((DStaticPowerResourceD*)pR1, pR1->iResourceId, pR2->iResourceId);

	if(pN2)
		CheckForDependencyLoop((DStaticPowerResourceD*)pR2, pR2->iResourceId, pR1->iResourceId);

	//Check whether the passed priority already exists.Code will return with KErrAlreadyExists, if it exists.
	CHECK_IF_PRIORITY_ALREADY_EXISTS(pN1, aInfo2->iDependencyPriority)
	CHECK_IF_PRIORITY_ALREADY_EXISTS(pN2, aInfo1->iDependencyPriority)
	UnLock();
	//Allocate nodes
	// coverity[alloc_fn]
	SNode* pSN1 = new (SNode);
	// coverity[alloc_fn]
	SNode* pSN2 = new (SNode);
	Lock();
	if(!pSN1 || !pSN2)
		return KErrNoMemory;
	//Add the link
	pSN1->iResource = (DStaticPowerResourceD*)pR1;
	pSN1->iPropagatedLevel = 0;
	pSN1->iPriority = aInfo1->iDependencyPriority;
	pSN1->iVisited = EFalse;
	pSN1->iNext = NULL;

	pSN2->iResource = (DStaticPowerResourceD*)pR2;
	pSN2->iPropagatedLevel = 0;
	pSN2->iPriority = aInfo2->iDependencyPriority;
	pSN2->iVisited = EFalse;
	pSN2->iNext = NULL;

	if(aInfo1->iResourceId & KIdMaskDynamic) //Dynamic resource
		// coverity[memory_leak]
		ADD_DEPENDENCY_NODE(pSN2, ((DDynamicPowerResourceD*)pR1)->iDependencyList)
	else
		((DStaticPowerResourceD*)pR1)->AddNode(pSN2);
	
	//Add the second node
	if(aInfo2->iResourceId & KIdMaskDynamic) //Dynamic resource
		// coverity[memory_leak]
		ADD_DEPENDENCY_NODE(pSN1, ((DDynamicPowerResourceD*)pR2)->iDependencyList)
	else
		((DStaticPowerResourceD*)pR2)->AddNode(pSN1);
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DExtendedResourceController::RegisterResourceDependency"));
#ifdef PRM_INSTRUMENTATION_MACRO
	PRM_REGISTER_RESOURCE_DEPENDENCY_TRACE
#endif
	return r;
	}

/**
Registers dynamic resource. 
*/
TInt DPowerResourceController::RegisterDynamicResource(SPowerResourceClient* aClientPtr, DDynamicPowerResource* aPDRes, 
													                 TUint* aDynamicResourceId)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DExtendedResourceController::RegisterDynamicResource"));
	TInt r = KErrNone;
	//Check for dynamic resource
	if(!(aPDRes->iResourceId & KIdMaskDynamic))
		return KErrNotSupported;
	//check for count
	else if(aPDRes->LockCount() != 0) 
		return KErrAlreadyExists;
	
	TPowerRequest* req = (TPowerRequest*)&TPowerRequest::Get();
	req->ReqType() = TPowerRequest::ERegisterDynamicResource;
	req->Resource() = (DStaticPowerResource*)aPDRes;
	UnLock();
	req->SendReceive(iMsgQ);
	Lock();
	if(req->ReturnCode() == KErrNone)
		{
		*aDynamicResourceId = req->ResourceId();
		aPDRes-> iOwnerId = aClientPtr->iClientId;
		aPDRes->Lock();
		//Increment dynamic resource count in client
		aClientPtr->iDynamicResCount++;
		}
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DExtendedResourceController::RegisterDynamicResource, resource ID = 0x%x", 
		                                                                                  *aDynamicResourceId));
#ifdef PRM_INSTRUMENTATION_MACRO
	PRM_REGISTER_DYNAMIC_RESOURCE_TRACE
#endif
	return r;
	}

/**
Deregisters dynamic resource.
*/
TInt DPowerResourceController::DeregisterDynamicResource(SPowerResourceClient* aClientPtr, TUint aResourceId, 
														 TInt* aPDefLevel)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DExtendedResourceController::DeregisterDynamicResource"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId = 0x%x, aDynamicResourceId = 0x%x, Default Level = %d", 
		                               aClientPtr->iClientId, aResourceId, aPDefLevel ? *aPDefLevel : 0));
	TInt r = KErrNone;
	DDynamicPowerResource* pDR = NULL;
	//Check for dynamic resource bit
	if(!(aResourceId & KIdMaskDynamic))
		return KErrNotSupported;

	//Get the resource from appropriate container
	if(aResourceId & KIdMaskResourceWithDependencies)
		{
		pDR = iDynamicResDependencyList[(aResourceId & ID_INDEX_BIT_MASK)];		
		if(!pDR)														
			return KErrNotFound;
		}
	else
		{
		pDR = iDynamicResourceList[(aResourceId & ID_INDEX_BIT_MASK)];		
		if(!pDR)														
			return KErrNotFound;
		}
	//Client which registered the dynamic resource is only allowed to deregister. 
	if(aClientPtr->iClientId != pDR->iOwnerId)
		{
		__KTRACE_OPT(KRESMANAGER, Kern::Printf("Client attempting to deregister a dynamic resource which is not the owner!"));
		return KErrAccessDenied;
		}
	// Don't allow to deregister if the some other operation is in progress or if the resource is shared and
	// another client holds requirement on this resource
	if((pDR->LockCount() > RESOURCE_NOT_IN_OPERATION) || pDR->InUse())
		{
		return KErrInUse;
		}
	TPowerRequest* req = (TPowerRequest*)&TPowerRequest::Get();
	req->ResourceCb() = NULL;
	req->ReturnCode() = KErrNone;
	req->RequiresChange() = EFalse;
	pDR->UnLock(); //Marked as deleted so that no other operation will be taking place.
	req->ReqType() = TPowerRequest::ESetDefaultLevel;
	//Handle dynamic resource with dependencies
	if(aResourceId & KIdMaskResourceWithDependencies)
		{
		for(SNode* pNode = ((DDynamicPowerResourceD*)pDR)->iDependencyList; pNode != NULL; pNode = pNode->iNext)
			{
			if((TUint)pNode->iResource->iLevelOwnerId == aResourceId)
				{
				req->ResourceId() = pNode->iResource->iResourceId;
				req->ClientId() = aResourceId;
				req->Level() = pNode->iResource->iCachedLevel;
				req->Resource() = pNode->iResource; 
				((DDynamicPowerResourceD*)(pNode->iResource))->Lock();
				UnLock();
				req->SendReceive(iMsgQDependency);
				Lock();
				}
			//Remove entry from resource dependency list
			for(SNode* pSN = pNode->iResource->iDependencyList; pSN != NULL; pSN = pSN->iNext)
				{
				if(pSN->iResource->iResourceId == aResourceId)
					{
					LIST_REMOVE(pNode->iResource->iDependencyList, pSN, iNext, SNode);
					UnLock();
					delete pSN;
					Lock();
					break;
					}
				}
			//Remove from dependent resource "resource client level" list
			for(SPowerResourceClientLevel* pL = ((DDynamicPowerResourceD*)(pNode->iResource))->iResourceClientList; 
			                                                                pL != NULL; pL = pL->iNextInList)
				{
				if(pL->iClientId == aResourceId)
					{
					LIST_REMOVE(((DDynamicPowerResourceD*)(pNode->iResource))->iResourceClientList, pL, iNextInList, 
						                                                                SPowerResourceClientLevel);
					//Move to free pool
					LIST_PUSH(iResourceLevelPool, pL, iNextInList);
					iResourceLevelPoolCount++;
					}
				}
			((DDynamicPowerResource*)(pNode->iResource))->UnLock();
			}
		//Move the resource to default level
		req->ClientId() = -1;
		req->ResourceId() = aResourceId;
		req->Resource() = pDR;
		if(aPDefLevel)
			req->Level() = *aPDefLevel; //Set the resource to the passed level
		else
			req->Level() = pDR->iDefaultLevel; //Set the resource level to default level
		UnLock();
		req->SendReceive(iMsgQDependency);
		}
	else
		{
		UnLock();
		req->ResourceId() = aResourceId;
		req->ClientId() = KDynamicResourceDeRegistering;
		req->Resource() = pDR;
		req->ResourceCb() = NULL;
		if(aPDefLevel)
			req->Level() = *aPDefLevel; //Set the resource to the passed level
		else
			req->Level() = pDR->iDefaultLevel; //Set the resource level to default level
		if(pDR->LatencySet())
			{
			r = req->SendReceive(iMsgQ);
			}
		else
			{
			//Call custom function for custom sense resource
			if(pDR->Sense() == DStaticPowerResource::ECustom)
				{
				if(!pDR->iCustomFunction)
					Panic(ECustomFunctionNotSet);
				pDR->iCustomFunction(req->ClientId(), *(aClientPtr->iName), aResourceId,
				                     EDynamicResourceDeregister, req->Level(),
				                     (TAny*)&pDR->iClientList, NULL);
				}
			//Not checking for error condition as the resource needs to be moved to default state
			if(aPDefLevel)
				{
				//If the resource change to requested level fails trying to change it to default level.
				req->ReqType() = TPowerRequest::EChange;
				r = pDR->DoRequest(*req);
				if(r != KErrNone)
					{
					req->ReqType() = TPowerRequest::ESetDefaultLevel;
					req->Level() = pDR->iDefaultLevel;
					pDR->DoRequest(*req);
					}
				}
			else
				pDR->DoRequest(*req);
			//Send notifications. Passing -2 in clientId to indicate that this dynamic resource is deregistering
			CompleteNotifications(KDynamicResourceDeRegistering, pDR, req->Level(),KErrNone, req->ClientId());
			}
		}
	Lock();
	//Remove client level
	SPowerResourceClientLevel *pCL;
	SPowerResourceClient *pC;
	for(SDblQueLink* pRC = pDR->iClientList.First(); pRC != &pDR->iClientList.iA; pRC = pRC->iNext)
		{
		pCL = (SPowerResourceClientLevel*)pRC;
		if(pCL->iClientId & USER_SIDE_CLIENT_BIT_MASK)
			pC = iUserSideClientList[(pCL->iClientId & ID_INDEX_BIT_MASK)];								
		else																										
			pC = iClientList[(pCL->iClientId & ID_INDEX_BIT_MASK)];										
		LIST_REMOVE(pC->iLevelList, pCL, iNextInList, SPowerResourceClientLevel);
		LIST_PUSH(iClientLevelPool, pCL, iNextInList);
		if(pC->iUnderFlowClCount > 0)
			{
			pC->iUnderFlowClCount--;
			iClientLevelPoolCount++;
			}
		else
			pC->iReservedCl++;
		}
	//Decrement dynamic resource count in client
	aClientPtr->iDynamicResCount--;
	if(aResourceId & KIdMaskResourceWithDependencies)
		{
		iDynamicResDependencyList.Remove((DDynamicPowerResourceD*)pDR, (pDR->iResourceId & ID_INDEX_BIT_MASK));
		iDynamicResDependencyCount--;
		}
	else
		{
		iDynamicResourceList.Remove(pDR, (pDR->iResourceId & ID_INDEX_BIT_MASK));
		iDynamicResourceCount--;
		}
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DExtendedResourceController::DeregisterDynamicResource"));
#ifdef PRM_INSTRUMENTATION_MACRO
	TInt level = req->Level();
	PRM_DEREGISTER_DYNAMIC_RESOURCE_TRACE
#endif
	return r;
	}

/**
@publishedPartner
@prototype 9.6
Default implementation, PSL re-implements this if features supported.
*/
TInt DPowerResourceController::DoRegisterStaticResourcesDependency(RPointerArray <DStaticPowerResourceD> & aStaticResourceDArray)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("DExtendedResourceController::DoRegisterStaticResourcesDependency default implementation"));
	aStaticResourceDArray.Reset();
	return KErrNone;
	}

/**
This function checks for any closed loop dependency, if so panics.
*/
void DPowerResourceController::CheckForDependencyLoop(DStaticPowerResourceD* pR, TUint aParentResId, TUint aTargetResId)
	{
	SNode *pN;

	if(pR->iResourceId & KIdMaskDynamic)
		pN = ((DDynamicPowerResourceD*)pR)->iDependencyList;
	else
		pN = pR->iDependencyList;

	for(; pN != NULL; pN = pN->iNext)
		{
		if(pN->iResource->iResourceId == aParentResId)
			continue;
		if(pN->iVisited || (pN->iResource->iResourceId == aTargetResId))
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("Loop encountered\n"));
			DPowerResourceController::Panic(DPowerResourceController::EClosedLoopDependencies);
			}
		pN->iVisited = ETrue;
		CheckForDependencyLoop(pN->iResource, pR->iResourceId, aTargetResId);
		pN->iVisited = EFalse;
		}
	}

/**
This is called from the controller thread to handle the dependency resource state change operation.
*/
TInt DPowerResourceController::HandleDependencyResourceStateChange(SPowerResourceClient* pC, TPowerRequest& aRequest)
	{
	DStaticPowerResourceD* pR = (DStaticPowerResourceD*)aRequest.Resource();
	if(aRequest.ReqType() == TPowerRequest::EChange) //Handle resource change operation
		{
		if(aRequest.Resource()->Usage()) //Shared resource
			{
			Lock();
			aRequest.ReturnCode() = CheckLevelAndAddClient(pC, &aRequest);
			UnLock();
			if((aRequest.ReturnCode()!= KErrNone) || (!aRequest.RequiresChange()))
				{
				aRequest.Level() = pR->iCachedLevel; //If no change then send the current level back.
				return aRequest.ReturnCode();
				}
			}
		else if(pR->iClientList.IsEmpty())
			{
			Lock();
			if(pC->iReservedCl==0 && !iClientLevelPoolCount)
				{
				__KTRACE_OPT(KRESMANAGER, Kern::Printf("Reserved Client Level exhausted and its free pool empty"));
                aRequest.ReturnCode() = KErrUnderflow;
				UNLOCK_RETURN(KErrUnderflow);
				}
			SPowerResourceClientLevel* pSCL=NULL;
			LIST_POP(iClientLevelPool, pSCL, iNextInList);
			pSCL->iClientId=pC->iClientId;
			pSCL->iResourceId=aRequest.ResourceId();
			pSCL->iLevel=aRequest.Level();
			LIST_PUSH(pC->iLevelList, pSCL, iNextInList); //Add to client
			pR->iClientList.Add(pSCL); //Add in resource
			if(pC->iReservedCl==0)
				{
				iClientLevelPoolCount--;
				pC->iUnderFlowClCount++;
				}
            else
			   pC->iReservedCl--;
			if(pR->iCachedLevel == aRequest.Level())
				{
				pR->iLevelOwnerId = aRequest.ClientId();
				if(pR->iIdleListEntry)
					pR->iIdleListEntry->iLevelOwnerId = aRequest.ClientId();
				aRequest.ReturnCode() = KErrNone;
				UNLOCK_RETURN(KErrNone);
				}
			UnLock();
			}
		else
			{
			//Update the level in the client list.
			SPowerResourceClientLevel* pSCL = (SPowerResourceClientLevel*)pR->iClientList.First();
			pSCL->iLevel = aRequest.Level();
			}
	//Call appropriate resource's handle change propagation function
	if(pR->iResourceId & KIdMaskDynamic)
		aRequest.ReturnCode() = ((DDynamicPowerResourceD*)pR)->HandleChangePropagation(aRequest, EChangeStart, pC->iClientId, *(pC->iName));
	else
		aRequest.ReturnCode() = pR->HandleChangePropagation(aRequest, EChangeStart,pC->iClientId, *(pC->iName));
	return aRequest.ReturnCode();
	}
	if(aRequest.ClientId() == -1) //Special where the resource needs to set to default level, when dynamic resource deregisters
		{
		//If resource is asked to change to certain value, instead of default then 
		//try to set it to that value. If not able to set then try to set it to default level.
		if(aRequest.Level() != pR->iDefaultLevel)
			aRequest.ReqType() = TPowerRequest::EChange;
		aRequest.ReturnCode() = pR->DoRequest(aRequest);
		if((aRequest.ReturnCode() != KErrNone) && (aRequest.ReqType() == TPowerRequest::EChange))
			{
			aRequest.ReqType() = TPowerRequest::ESetDefaultLevel;
			aRequest.Level() = pR->iDefaultLevel;
			pR->DoRequest(aRequest);
			}
		//Set clientId to -2, indicating that the resource is deregistered.
		CompleteNotifications(KDynamicResourceDeRegistering, pR, aRequest.Level(), KErrNone, aRequest.ClientId());
		return KErrNone;
		}
	//Handle custom sense resource
	if(aRequest.Resource()->Sense() == DStaticPowerResource::ECustom)
		{
		if(pR->iResourceId & KIdMaskDynamic)
		    {
			aRequest.RequiresChange() = ((DDynamicPowerResourceD*)pR)->iDepCustomFunction(aRequest.ClientId(), *(pC->iName), aRequest.ResourceId(), 
			                                                                              EClientRelinquishLevel, aRequest.Level(), (TAny*)&pR->iClientList,
			                                                                              (TAny*)&((DDynamicPowerResourceD*)pR)->iResourceClientList, NULL);
			}
		else
		    {
			aRequest.RequiresChange() = pR->iDepCustomFunction(aRequest.ClientId(), *(pC->iName), aRequest.ResourceId(),
			                                                   EClientRelinquishLevel, aRequest.Level(), (TAny*)&pR->iClientList,
			                                                   (TAny*)&pR->iResourceClientList, NULL);
		    }
		}
	else
		{
		SPowerResourceClientLevel* pL = NULL;
		SPowerResourceClientLevel* pMCL = NULL;
		TInt maxLevel = KMinTInt;
		//Find the maximum level from client
		for(SDblQueLink* pCL = pR->iClientList.First(); pCL != &pR->iClientList.iA; pCL = pCL->iNext)
			{
			pL = (SPowerResourceClientLevel*)pCL;
			if(pL->iClientId == (TUint)aRequest.ClientId())
				continue;
			if(pMCL == NULL)
				{	
				maxLevel = pL->iLevel;
				pMCL = pL;
				continue;
				}
			if(((pR->Sense() == DStaticPowerResource::ENegative) && (pL->iLevel < maxLevel)) || 
				                    ((pR->Sense() == DStaticPowerResource::EPositive) && (pL->iLevel > maxLevel)))
				{
				maxLevel = pL->iLevel;
				pMCL = pL;
				}
			}	
		//Find the maximum level from resource client level
		if(pR->iResourceId & KIdMaskDynamic)
			pL = ((DDynamicPowerResourceD*)pR)->iResourceClientList;
		else
			pL = pR->iResourceClientList;
		for(; pL != NULL; pL = pL->iNextInList)
			{
			if(pL->iClientId == (TUint)aRequest.ClientId())
				continue;
			if(pMCL == NULL)
					{
				maxLevel = pL->iLevel;
				pMCL = pL;
				continue;
				}
			if(((pR->Sense() == DStaticPowerResource::ENegative) && (pL->iLevel < maxLevel)) || 
				           ((pR->Sense() == DStaticPowerResource::EPositive) && (pL->iLevel > maxLevel)))
				{
				maxLevel = pL->iLevel;
				pMCL = pL;
				}
			}
		if(pMCL == NULL)
			{
			aRequest.ClientId() = -1;
			aRequest.Level() = pR->iDefaultLevel;
			}
		else
			{
			aRequest.ClientId() = pMCL->iClientId;
			aRequest.Level() = maxLevel;
			}
		}
	if((aRequest.Level() == pR->iCachedLevel) && !aRequest.RequiresChange()) //No need to change the resource just update the owner 
		{
		pR->iLevelOwnerId = aRequest.ClientId();
		if(pR->iIdleListEntry)
			pR->iIdleListEntry->iLevelOwnerId = aRequest.ClientId();
		aRequest.ReturnCode() = KErrNone;
		return KErrNone;
		}
	aRequest.ReqType() = TPowerRequest::EChange; //Make the change otherwise PSL set to default level

	const TDesC8 *name;
	if(aRequest.ClientId() == -1)
		name = &KNoClient;
	else
		{
		if(aRequest.ClientId() & (1 << RESOURCE_BIT_IN_ID_CHECK))										
			{																							
			DStaticPowerResourceD* pResource;												
			if(aRequest.ClientId() & KIdMaskDynamic)										
				pResource = (DStaticPowerResourceD*)iDynamicResDependencyList[(aRequest.ClientId() & ID_INDEX_BIT_MASK)];
			else																						
				pResource = iStaticResDependencyArray[(aRequest.ClientId() & ID_INDEX_BIT_MASK)  - 1];	
			name = pResource->iName;																	
			}																							
		else																							
			{																							
			SPowerResourceClient* pClient;																
			if(aRequest.ClientId() & USER_SIDE_CLIENT_BIT_MASK)										
				pClient = iUserSideClientList[(aRequest.ClientId() & ID_INDEX_BIT_MASK)];	
			else // coverity[returned_null]
				pClient = iClientList[(aRequest.ClientId() & ID_INDEX_BIT_MASK)];			
			name = pClient->iName;				
			}
		}

	if(pR->iResourceId & KIdMaskDynamic)
		aRequest.ReturnCode() = ((DDynamicPowerResourceD*)pR)->HandleChangePropagation(aRequest, EChangeStart, aRequest.ClientId(), *name);
	else
		aRequest.ReturnCode() = pR->HandleChangePropagation(aRequest, EChangeStart, aRequest.ClientId(), *name);
	if(aRequest.ReturnCode() == KErrPermissionDenied) //Update the ownerId alone
		{
		pR->iLevelOwnerId = aRequest.ClientId();
		if(pR->iIdleListEntry)
			pR->iIdleListEntry->iLevelOwnerId = aRequest.ClientId();
		}
	return KErrNone;
	}

/**
Deregisters resource dependency.
*/
TInt DPowerResourceController::DeregisterResourceDependency(SPowerResourceClient* aClientPtr, TUint aResId1, TUint aResId2)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::DeregisterResourceDependency"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("ClientId = 0x%x, ResourceId1 = 0x%x, ResourceId2 = 0x%x", 
		                                                                  aClientPtr->iClientId, aResId1, aResId2));
	if(iDfcQDependencyLock)
		{
		__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::RegisterResourceDependency::Resource In Use"));
		UNLOCK_RETURN(KErrInUse);
		}
	DDynamicPowerResourceD *pDR1 = NULL;
	DDynamicPowerResourceD *pDR2 = NULL;
	SNode* pN1 = NULL;
	SNode* pN2 = NULL;
	SPowerResourceClientLevel* pCL1 = NULL;
	SPowerResourceClientLevel* pCL2 = NULL;
	//Get first resource from list
	if(!(aResId1 & KIdMaskResourceWithDependencies) || !(aResId2 & KIdMaskResourceWithDependencies))
		UNLOCK_RETURN(KErrAccessDenied);

	if(aResId1 & KIdMaskDynamic)
		{
		pDR1 = iDynamicResDependencyList[(aResId1 & ID_INDEX_BIT_MASK)];	
		if(!pDR1)															
			UNLOCK_RETURN(KErrNotFound);
		pN1 = pDR1->iDependencyList;
		pCL1 = pDR1->iResourceClientList;
		}
	else
		{
		if((aResId1 & ID_INDEX_BIT_MASK) > (TUint)iStaticResDependencyArray.Count())			
			UNLOCK_RETURN(KErrNotFound);			
		pDR1 = (DDynamicPowerResourceD*)iStaticResDependencyArray[(aResId1 & ID_INDEX_BIT_MASK) - 1];
		pN1 = ((DStaticPowerResourceD*)pDR1)->iDependencyList;
		pCL1 = ((DStaticPowerResourceD*)pDR1)->iResourceClientList;
		}

	//Get second resource from list
	if(aResId2 & KIdMaskDynamic)
		{
		pDR2 = iDynamicResDependencyList[(aResId2 & ID_INDEX_BIT_MASK)];	
		if(!pDR2)															
			UNLOCK_RETURN(KErrNotFound);
		pN2 = pDR2->iDependencyList;
		pCL2 = pDR2->iResourceClientList;
		}
	else
		{
		if((aResId2 & ID_INDEX_BIT_MASK)> (TUint)iStaticResDependencyArray.Count())			
				UNLOCK_RETURN(KErrNotFound);			
		pDR2 = (DDynamicPowerResourceD*)iStaticResDependencyArray[(aResId2 & ID_INDEX_BIT_MASK) - 1];
		pN2 = ((DStaticPowerResourceD*)pDR2)->iDependencyList;
		pCL2 = ((DStaticPowerResourceD*)pDR2)->iResourceClientList;
		}

	//Check whether dependency exist between the two
	SNode* pN = NULL;
	for(pN = pN1; pN != NULL; pN = pN->iNext)
		{
		if(pN->iResource->iResourceId == pDR2->iResourceId)
			break;
		}
	if(pN == NULL)
		UNLOCK_RETURN(KErrNotFound);
	pN1 = pN; //Storing for later use.
	for(pN = pN2; pN != NULL; pN = pN->iNext)
		{
		if(pN->iResource->iResourceId == pDR1->iResourceId)
			break;
		}
	if(pN == NULL)
		return KErrNotFound;
	pN2 = pN; //Storing for later use
	//Remove the dependency link from both the resource
	if(aResId1 & KIdMaskDynamic)
		{
		LIST_REMOVE(pDR1->iDependencyList, pN1, iNext, SNode);
		}
	else
		{
		LIST_REMOVE(((DStaticPowerResourceD*)pDR1)->iDependencyList, pN1, iNext, SNode);
		}

	if(aResId2 & KIdMaskDynamic)
		{	
		LIST_REMOVE(pDR2->iDependencyList, pN2, iNext, SNode);
		}
	else
		{	
		LIST_REMOVE(((DStaticPowerResourceD*)pDR2)->iDependencyList, pN2, iNext, SNode);
		}

	//Remove the resource client level from each resource
	for(; pCL1 != NULL; pCL1 = pCL1->iNextInList)
		{
		if(pCL1->iClientId == pDR2->iResourceId)
			{
			if(aResId1 & KIdMaskDynamic)
				{	
				LIST_REMOVE(pDR1->iResourceClientList, pCL1, iNextInList, SPowerResourceClientLevel);
				}	
			else
				{
				LIST_REMOVE(((DStaticPowerResourceD*)pDR1)->iResourceClientList, pCL1, iNextInList, 
					                                            SPowerResourceClientLevel);
				}
			LIST_PUSH(iResourceLevelPool, pCL1, iNextInList);
			iResourceLevelPoolCount++;
			break;
			}
		}
	for(; pCL2 != NULL; pCL2 = pCL2->iNextInList)
		{
		if(pCL2->iClientId == pDR1->iResourceId)
			{
			if(aResId2 & KIdMaskDynamic)
				{	
				LIST_REMOVE(pDR2->iResourceClientList, pCL2, iNextInList, SPowerResourceClientLevel);
				}
			else
				{
				LIST_REMOVE(((DStaticPowerResourceD*)pDR2)->iResourceClientList, pCL2, iNextInList, 
					                                              SPowerResourceClientLevel);
				}
			LIST_PUSH(iResourceLevelPool, pCL2, iNextInList);
			iResourceLevelPoolCount++;
			break;
			}
		}

	TPowerRequest* req = (TPowerRequest*)&TPowerRequest::Get();
	req->ResourceCb() = NULL;
	req->ReqType() = TPowerRequest::ESetDefaultLevel;
	req->RequiresChange() = EFalse;
	req->ReturnCode() = KErrNone;
	if((TUint)pDR1->iLevelOwnerId == pDR2->iResourceId)
		{
		//Ask to change to default level. Process this in the RC thread;
		req->ResourceId() = pDR1->iResourceId;
		req->ClientId() = pDR2->iResourceId;
		req->Resource() = pDR1;
		req->Level() = pDR1->iDefaultLevel;
		if(aResId1 & KIdMaskDynamic)
			pDR1->Lock();
		UnLock();
		req->SendReceive(iMsgQDependency);
		Lock();
		if(aResId1 & KIdMaskDynamic)
			pDR1->UnLock();
		}
	if((TUint)pDR2->iLevelOwnerId == pDR1->iResourceId)
		{
		//Ask to change to default level. Process this in the RC thread.
		req->ResourceId() = pDR2->iResourceId;
		req->ClientId() = pDR1->iResourceId;
		req->Resource() = pDR2;
		req->Level() = pDR2->iDefaultLevel;
		if(aResId2 & KIdMaskDynamic)
			pDR2->Lock();
		UnLock();
		req->SendReceive(iMsgQDependency);
		Lock();
		if(aResId2 & KIdMaskDynamic)
			pDR2->UnLock();
		}
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::DeregisterResourceDependency"));
#ifdef PRM_INSTRUMENTATION_MACRO
	PRM_DEREGISTER_RESOURCE_DEPENDENCY_TRACE
#endif
	UnLock();
	delete pN1;
	delete pN2;
	return KErrNone;
	}

/**
This function takes care of resource state change of static dependency resource.
This propagates the change to all of its dependents.
This function takes Originator name and id as parameter as this needs to be passed for custom sense function.
*/
TInt DStaticPowerResourceD::HandleChangePropagation(TPowerRequest aRequest, TPropagation aProp, TUint aOriginatorId, const TDesC8& aOriginatorName)
	{
	TInt result = KErrNone;
	result = PowerResourceController->HandleResourceChange(aRequest, aProp, aOriginatorId, aOriginatorName, this);
	return result;
	}
//Function to change the resource state of dependency resource. 
TInt DPowerResourceController::HandleResourceChange(TPowerRequest &aRequest, TPropagation aProp, TUint aOriginatorId, 
													const TDesC8& aOriginatorName, DStaticPowerResourceD* aResource)
	{
	static TUint16 clientLevelCount = 0;
	DStaticPowerResourceD* pDR = (DStaticPowerResourceD*)aRequest.Resource();
	DStaticPowerResourceD* pDepRes = NULL;
	SNode* dependencyList = NULL;
	TPowerRequest depRequest;
	TInt result = KErrNone;
	TInt resState;
	depRequest.ReqType() = TPowerRequest::EChange;
	depRequest.ResourceCb() = NULL;
	depRequest.ReturnCode() = KErrNone;
	depRequest.RequiresChange() = EFalse;

	if(pDR->iResourceId & KIdMaskDynamic)
		dependencyList = ((DDynamicPowerResourceD*)pDR)->iDependencyList;
	else
		dependencyList = pDR->iDependencyList;
	switch(aProp)																									
		{																											
		case EChangeStart:																							
			{
			if(!dependencyList) /*No dependents so change state of the resource*/
				{																									
				aRequest.ReturnCode() = pDR->DoRequest(aRequest);													
				if(aRequest.ReturnCode() == KErrNone)																
					{																								
					aResource->iCachedLevel = aRequest.Level();																
					aResource->iLevelOwnerId = aRequest.ClientId();															
					if(aResource->iIdleListEntry)																				
						{																							
						aResource->iIdleListEntry->iCurrentLevel = aRequest.Level();											
						aResource->iIdleListEntry->iLevelOwnerId = aRequest.ClientId();										
						}			
					CompleteNotifications(aRequest.ClientId(), pDR,									
							aRequest.Level(), aRequest.ReturnCode(), aRequest.ClientId());							
					}																								
					break;																		
				}					
			depRequest.ResourceId() = aRequest.ResourceId();														
			depRequest.ClientId() = aRequest.ResourceId();															
			depRequest.Level() = aRequest.Level();																	
			depRequest.Resource() = pDR;
			result = pDR->HandleChangePropagation(depRequest, ECheckChangeAllowed, aOriginatorId, aOriginatorName);	
			if(result != KErrNone)																					
				return result;																						
			/*Adjust resource client level*/																		
			if(clientLevelCount)																					
				{					
				result = ReserveClientLevelPoolCount(clientLevelCount);								
				if(result != KErrNone)																				
					return result;																					
				}																									
			/*Resource change of dependents */																		
			pDR->HandleChangePropagation(aRequest, ERequestStateChange, aOriginatorId, aOriginatorName);				
			/*Notification to dependents */																			
			pDR->HandleChangePropagation(aRequest, EIssueNotifications, aOriginatorId, aOriginatorName);				
			break;																									
			}																										
		case ECheckChangeAllowed:																					
			{																										
			TChangePropagationStatus status;																		
			for(SNode* depNode = dependencyList; depNode != NULL; depNode = depNode->iNext)					
				{																									
				pDepRes = depNode->iResource;													
				if((aRequest.ClientId() & KIdMaskResourceWithDependencies) &&										
						(pDepRes->iResourceId == (TUint)aRequest.ClientId()))										
					continue;	
				/*Resource need not change if it is already in that state, so continue with							
						another dependent state.*/																	
				if(pDepRes->iResourceId & KIdMaskDynamic)															
					status = ((DDynamicPowerResourceD*)pDepRes)->TranslateDependentState(aRequest.ResourceId(),		
																				aRequest.Level(), resState);		
				else																								
					status = ((DStaticPowerResourceD*)pDepRes)->TranslateDependentState(aRequest.ResourceId(),		
																					aRequest.Level(), resState);	
				if((status == ENoChange) || (pDepRes->iCachedLevel == resState))									
					{																								
					depNode->iRequiresChange = EFalse;																
					continue;																						
					}																								
				if(status == EChangeNotAccepted)																	
					return KErrPermissionDenied;	
				depRequest.ResourceId() = pDepRes->iResourceId;														
				depRequest.ClientId() = aRequest.ResourceId(); /*ID of the dependent resource */					
				depRequest.Level() = resState;																		
				depRequest.Resource() = pDepRes;		
				/*Check resource client list and resource list to see whether change is allowed.*/					
				if(pDepRes->Sense() == DStaticPowerResource::ECustom)												
					{																								
					/*Call custom function to check whether change is allowed.*/									
					if(pDepRes->iResourceId & KIdMaskDynamic)														
						depRequest.RequiresChange() = ((DDynamicPowerResourceD*)pDepRes)->iDepCustomFunction(depRequest.ClientId(),	
							aOriginatorName, depRequest.ResourceId(), EClientRequestLevel, depRequest.Level(), (TAny*)&pDepRes->iClientList,		
									(TAny*)&((DDynamicPowerResourceD*)pDepRes)->iResourceClientList, NULL);				
					else																							
						depRequest.RequiresChange() = ((DStaticPowerResourceD*)pDepRes)->iDepCustomFunction(depRequest.ClientId(),		
							aOriginatorName, depRequest.ResourceId(), EClientRequestLevel, depRequest.Level(), (TAny*)&pDepRes->iClientList,		
									(TAny*)&((DStaticPowerResourceD*)pDepRes)->iResourceClientList, NULL);				
					if(!depRequest.RequiresChange())																
						return KErrPermissionDenied;																
					}						
				SPowerResourceClientLevel*pN=NULL;																	
				for(SDblQueLink* pNL=pDepRes->iClientList.First();pNL!=&pDepRes->iClientList.iA; pNL=pNL->iNext)	
					{																								
					pN = (SPowerResourceClientLevel*)pNL;															
					if(pDepRes->Sense() == DStaticPowerResource::EPositive)											
						{																							
						if(pN->iLevel > depRequest.Level())															
							return KErrPermissionDenied;															
						}																							
					else if(pDepRes->Sense() == DStaticPowerResource::ENegative)									
						{																							
						if(pN->iLevel < depRequest.Level())															
							return KErrPermissionDenied;															
						}																							
					}																								
				/*check through the resource client level */														
				SPowerResourceClientLevel*pCL = NULL;																
				if(pDepRes->iResourceId & KIdMaskDynamic)															
					pCL = ((DDynamicPowerResourceD*)pDepRes)->iResourceClientList;									
				else																								
					pCL = ((DStaticPowerResourceD*)pDepRes)->iResourceClientList;									
				for(; pCL != NULL; pCL = pCL->iNextInList)															
					{																								
					if(pCL->iClientId == pDR->iResourceId)															
						break;																						
					}																								
				if(!pCL)																							
					clientLevelCount++;																				
				/*check dependent resource client list & resource list to see whether change is allowed */			
				if(pDepRes->iResourceId & KIdMaskDynamic)															
					result = ((DDynamicPowerResourceD*)pDepRes)->HandleChangePropagation(depRequest,				
																ECheckChangeAllowed, aOriginatorId, aOriginatorName);	
				else																								
					result = ((DStaticPowerResourceD*)pDepRes)->HandleChangePropagation(depRequest,					
											ECheckChangeAllowed, aOriginatorId, aOriginatorName);						
				if(result != KErrNone)																				
					return result;																					
				depNode->iPropagatedLevel = resState;																
				depNode->iRequiresChange = ETrue;																	
				}																									
			break;																									
			}																										
		case ERequestStateChange:																					
			{																										
			SPowerResourceClientLevel* pCL = NULL;																	
			for(SNode* depNode = dependencyList; depNode != NULL; depNode = depNode->iNext)					
				{																									
				pDepRes = depNode->iResource;													
				if((!depNode->iRequiresChange) || (pDepRes->iResourceId == (TUint)aRequest.ClientId()))				
					continue;																						
				depRequest.ResourceId() = pDepRes->iResourceId;														
				depRequest.ClientId() = aRequest.ResourceId();														
				depRequest.Level() = depNode->iPropagatedLevel;														
				depRequest.Resource() = pDepRes;									
				if(pDepRes->iResourceId & KIdMaskDynamic)															
					((DDynamicPowerResourceD*)pDepRes)->HandleChangePropagation(depRequest, ERequestStateChange,	
																					aOriginatorId, aOriginatorName);	
				else																								
					((DStaticPowerResourceD*)pDepRes)->HandleChangePropagation(depRequest, ERequestStateChange,		
																					aOriginatorId, aOriginatorName);	
				/*Update level if resource client level is already present for this resource.*/						
				if(pDepRes->iResourceId & KIdMaskDynamic)															
					pCL = ((DDynamicPowerResourceD*)pDepRes)->iResourceClientList;									
				else																								
					pCL = ((DStaticPowerResourceD*)pDepRes)->iResourceClientList;									
				for(; pCL != NULL; pCL = pCL->iNextInList)															
					{																								
					if(pCL->iClientId == pDR->iResourceId)															
						{																							
						pCL->iLevel = depNode->iPropagatedLevel;													
						break;																						
						}																							
					}																								
				if(!pCL) /*Create a new resource client level*/														
					{																								
					RemoveClientLevelFromPool(pCL);													
					pCL->iClientId = pDR->iResourceId;																
					pCL->iResourceId = pDepRes->iResourceId;														
					pCL->iLevel = depNode->iPropagatedLevel;														
					if(pDepRes->iResourceId & KIdMaskDynamic)														
						{																							
						LIST_PUSH(((DDynamicPowerResourceD*)pDepRes)->iResourceClientList, pCL, iNextInList);		
						}																							
					else																							
						{																							
						LIST_PUSH(((DStaticPowerResourceD*)pDepRes)->iResourceClientList, pCL, iNextInList);		
						}																							
					clientLevelCount--;																				
					}																								
				}	
#ifdef PRM_INSTRUMENTATION_MACRO			
			if(aRequest.ClientId() & KIdMaskResourceWithDependencies)								
				{																									
				SPowerResourceClient res;																			
				SPowerResourceClient* pC = &res;																	
				pC->iClientId = aRequest.ClientId();																
				pC->iName = &KParentResource;																		
				DStaticPowerResource*pR = (DStaticPowerResource*)pDR;												
				TUint aResourceId = pDR->iResourceId;																
				TInt aNewState = aRequest.Level();																	
				PRM_CLIENT_CHANGE_STATE_START_TRACE																	
				}																									
#endif
				aResource->DoRequest(aRequest);																				
#ifdef PRM_INSTRUMENTATION_MACRO
			if(aRequest.ClientId() & KIdMaskResourceWithDependencies)								
				{																									
				SPowerResourceClient res;																			
				SPowerResourceClient* pC = &res;																	
				pC->iClientId = aRequest.ClientId();																
				pC->iName = &KParentResource;																		
				DStaticPowerResource*pR = (DStaticPowerResource*)pDR;												
				TUint aResourceId = pDR->iResourceId;																
				TInt aNewState = aRequest.Level();																	
				TInt r = KErrNone;																					
				PRM_CLIENT_CHANGE_STATE_END_TRACE																	
				}													
#endif												
			pDR->iCachedLevel = aRequest.Level();																	
			pDR->iLevelOwnerId = aRequest.ClientId();																
			if(pDR->iIdleListEntry)																					
				{																									
				pDR->iIdleListEntry->iCurrentLevel = aRequest.Level();												
				pDR->iIdleListEntry->iLevelOwnerId = aRequest.ClientId();											
				}									
			break;																									
			}																										
		case EIssueNotifications:																					
			{																										
			for(SNode* depNode = dependencyList; depNode != NULL; depNode = depNode->iNext)					
				{																									
				pDepRes = depNode->iResource;													
				if((!depNode->iRequiresChange) || (pDepRes->iResourceId == (TUint)aRequest.ClientId()))				
					continue;																						
				depRequest.ResourceId() = pDepRes->iResourceId;														
				depRequest.ClientId() = pDepRes->iLevelOwnerId;														
				depRequest.Level() = pDepRes->iCachedLevel;															
				depRequest.Resource() = pDepRes;																	
				if(pDepRes->iResourceId & KIdMaskDynamic)															
					((DDynamicPowerResourceD*)pDepRes)->HandleChangePropagation(depRequest, EIssueNotifications,	
																					aOriginatorId, aOriginatorName);	
				else																								
					((DStaticPowerResourceD*)pDepRes)->HandleChangePropagation(depRequest, EIssueNotifications,		
																					aOriginatorId, aOriginatorName);	
				}							
			CompleteNotifications(aRequest.ClientId(), pDR, aRequest.Level(), KErrNone,				
																					aRequest.ClientId());			
			break;																									
			}																										
		default:																									
			return KErrNotSupported;																				
		}																											
	return result;
	}


