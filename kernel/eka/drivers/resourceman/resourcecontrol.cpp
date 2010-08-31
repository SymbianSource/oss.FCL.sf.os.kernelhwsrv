// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\resourceman\resourcecontrol.cpp
//
//

#include <drivers/resourcecontrol.h>
#include <drivers/resourcecontrol_trace.h>
#ifdef DEBUG_VERSION
#define GET_CRITICAL_SECTION_COUNT				\
	DThread& thread = Kern::CurrentThread();	\
	TInt CsCount = thread.iNThread.iCsCount;

#define LOCK_AND_CRITICAL_SECTION_COUNT_CHECK						\
	if(thread.iNThread.iCsCount != CsCount)							\
		Kern::Fault("PowerResourceController CScount", __LINE__);	\
	if(PowerResourceController->iResourceMutex->iHoldCount != 0)	\
		Kern::Fault("PowerResourceController HoldCount", __LINE__);
#else
#define GET_CRITICAL_SECTION_COUNT
#define LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
#endif

/** Allow interface class to call this. */
DPowerResourceController* PowerResourceController = NULL;

/** Resource Controller factory class implementation */
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
#ifndef PRM_ENABLE_EXTENDED_VERSION
_LIT(KPddName,"resourcecontroller.pdd");
#else
_LIT(KPddName, "resourcecontrollerextended.pdd");
#endif
#else
#ifndef PRM_ENABLE_EXTENDED_VERSION
_LIT(KPddName, "resman.pdd");
#else
_LIT(KPddName, "resmanextended.pdd");
#endif
#endif

/** Factory class constructor */
DResConPddFactory::DResConPddFactory()
	{
    //Set Version number
    iVersion = DResConPddFactory::VersionRequired();
	}

TInt DResConPddFactory::Install()
    {
    // Set a Name for Resource Controller Factory class object.
    return(SetName(&KPddName));
    }

/**  Called by the kernel's device driver framework to create a Physical Channel. */
TInt DResConPddFactory::Create(DBase*& aChannel, TInt /*aUint*/, const TDesC8* /*anInfo*/, const TVersion& /*aVer*/)
    {
    //Create new interface for each channel.
	DUserSideProxyInterface *pI = new (DUserSideProxyInterface);
	if(!pI)
		return KErrNoMemory;
	pI->iController = PowerResourceController; //Store the resource controller. 
	aChannel = (DBase*)pI;
    return KErrNone;
    }

/**  Called by the kernel's device driver framework to check if this PDD is suitable for use with a Logical Channel.*/
TInt DResConPddFactory::Validate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
    {
   	if (!Kern::QueryVersionSupported(DResConPddFactory::VersionRequired(),aVer))
		return(KErrNotSupported);
    return KErrNone;
    }

/** Return the driver capabilities */
void DResConPddFactory::GetCaps(TDes8& aDes) const
    {
	// Create a capabilities object
	TCaps caps;
	caps.iVersion = iVersion;
	// Zero the buffer
	TInt maxLen = aDes.MaxLength();
	aDes.FillZ(maxLen);
	// Copy cpabilities
	TInt size=sizeof(caps);
	if(size>maxLen)
	   size=maxLen;
	aDes.Copy((TUint8*)&caps,size);
    }

/** Entry point for a standard physical device driver (PDD) that is also an extension */
#ifndef RESOURCE_MANAGER_SIMULATED_PSL
DECLARE_EXTENSION_PDD()
    {
    return new DResConPddFactory;
    }
#endif

/** Interface class implementation */
TInt TInterface::RegisterClient(TUint& aClientId, const TDesC8& aName, TOwnerType aType)
    {
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::RegisterClient"));
	r = PowerResourceController->RegisterClient(aClientId, aName, aType);
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return r;
    }
 
TInt TInterface::DeRegisterClient(TUint aClientId)
    {
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::DeRegisterClient"));
	r = PowerResourceController->DeRegisterClient(aClientId);
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return r;
    }

TInt TInterface::GetClientName(TUint aClientId, TUint aTargetClientId, TDes8& aName)
    {
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::GetClientName"));
	r = PowerResourceController->GetClientName(aClientId, aTargetClientId, aName);
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return r;
    }

TInt TInterface::GetClientId(TUint aClientId, TDesC8& aClientName, TUint& aTargetClientId)
    {
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::GetClientId"));
	r = PowerResourceController->GetClientId(aClientId, aClientName, aTargetClientId);
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return r;
    }

TInt TInterface::GetResourceId(TUint aClientId, TDesC8& aResourceName, TUint& aResourceId)
    {
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::GetResourceId"));
	r = PowerResourceController->GetResourceId(aClientId, aResourceName, aResourceId);
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return r;
    }

TInt TInterface::GetResourceInfo(TUint aClientId, TUint aResourceId, TAny* aInfo)
    {
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::GetResourceInfo"));
	r = PowerResourceController->GetResourceInfo(aClientId, aResourceId, aInfo);
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return r;
    }

TInt TInterface::GetNumResourcesInUseByClient(TUint aClientId, TUint aTargetClientId, TUint& aNumResource)
    {
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::GetNumResourcesInUseByClient"));
	r = PowerResourceController->GetNumResourcesInUseByClient(aClientId, aTargetClientId, aNumResource);
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return r;
    }

TInt TInterface::GetInfoOnResourcesInUseByClient(TUint aClientId, TUint aTargetClientId, TUint& aNumResources, TAny* aInfo)
    {
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::GetInfoOnResourcesInUseByClient"));
	r = PowerResourceController->GetInfoOnResourcesInUseByClient(aClientId, aTargetClientId, aNumResources, aInfo);
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return r;
    }

TInt TInterface::GetNumClientsUsingResource(TUint aClientId, TUint aResourceId, TUint& aNumClients)
    {
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::GetNumClientsUsingResource"));
	r = PowerResourceController->GetNumClientsUsingResource(aClientId, aResourceId, aNumClients);
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return r;
    }

TInt TInterface::GetInfoOnClientsUsingResource(TUint aClientId, TUint aResourceId, TUint& aNumClients, TAny* aInfo)
    {
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::GetInfoOnClientsUsingResource"));
	r = PowerResourceController->GetInfoOnClientsUsingResource(aClientId, aResourceId, aNumClients, aInfo);
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return r;
    }

TInt TInterface::ChangeResourceState(TUint aClientId, TUint aResourceId, TInt aNewState, TPowerResourceCb* aCb)
    {
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::ChangeResourceState"));
	r = PowerResourceController->ChangeResourceState(aClientId, aResourceId, aNewState, aCb);
	if(!aCb) //Not checking incase of asynchronous function as mutex might be held in RC thread, when this is checked.
		{
		LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
		}
	return r;
    }

TInt TInterface::GetResourceState(TUint aClientId, TUint aResourceId, TBool aCached, TInt& aState, TInt& aLevelOwnerId)
    {
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::GetResourceState"));
    r = PowerResourceController->GetResourceState(aClientId, aResourceId, aCached, aState, aLevelOwnerId);
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return r;
    }

TInt TInterface::GetResourceState(TUint aClientId, TUint aResourceId, TBool aCached, TPowerResourceCb& aCb)
    {
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::GetResourceState"));
    r = PowerResourceController->GetResourceState(aClientId, aResourceId, aCached, aCb);
	return r;
    }

TInt TInterface::CancelAsyncRequestCallBack(TUint aClientId, TUint aResourceId, TPowerResourceCb& aCb)
    {
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::CancelAsyncRequestCallback"));
    r = PowerResourceController->CancelAsyncRequestCallBack(aClientId, aResourceId, aCb);
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return r;
    }

TInt TInterface::RequestNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN)
    {
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::RequestNotification"));
    r = PowerResourceController->RequestNotification(aClientId, aResourceId, aN);
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return r;
    }

TInt TInterface::RequestNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN, TInt aThreshold, 
									                                                               TBool aDirection)
    {
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::RequestNotification"));
    r = PowerResourceController->RequestNotification(aClientId, aResourceId, aN, aThreshold, aDirection);
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return r;
    }

TInt TInterface::CancelNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN)
    {
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::CancelNotification"));
    r = PowerResourceController->CancelNotification(aClientId, aResourceId, aN);
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return r;
    }

TInt TInterface::DeRegisterClientLevelFromResource(TUint aClientId, TUint aResourceId)
	{
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::DeRegisterClientLevelFromResource"));
    r = PowerResourceController->DeRegisterClientLevelFromResource(aClientId, aResourceId);
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return r;
    }

TInt TInterface::AllocReserve(TUint aClientId, TUint8 aNumCl, TUint8 aNumRm)
    {
	GET_CRITICAL_SECTION_COUNT
	TInt r;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::AllocReserve"));
    r = PowerResourceController->AllocReserve(aClientId, aNumCl, aNumRm);
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return r;
    }

/** This function is used by export functions of Resource contoller defined in seperate file */
DPowerResourceController* TInterface::GetPowerResourceController(void)
    {
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::GetPowerResourceController"));
    return PowerResourceController;
    }

TInt TInterface::ControlIO(TUint aClientId, TUint aFunction, TAny* aParam1, TAny* aParam2, TAny* aParam3)
    {
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TInterface::ControlIO"));
    return PowerResourceController->GetInterface(aClientId, aFunction, aParam1, aParam2, aParam3);
    }

/** Resource controller panic */
void DPowerResourceController::Panic(TUint8 aPanic)
	{
	Kern::Fault("Power Resource Controller", aPanic);
	}

/** Constructor for power controller. Creates message queue and generates ID for power controller to use. */
extern RPointerArray <DStaticPowerResource> *StaticResourceArrayPtr;
#ifdef PRM_ENABLE_EXTENDED_VERSION
extern RPointerArray <DStaticPowerResourceD> *StaticResourceDependencyArrayPtr;
#endif
DPowerResourceController::DPowerResourceController()
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("DPowerResourceController::DPowerResouceController()"));
	// Constructor is expected to invoke multiple times, i.e.: 
	// during creation: variant init 0(KModuleEntryReasonVariantInit0) and later extension init 1 (KModuleEntryReasonExtensionInit1)
	if(PowerResourceController)
		{
		// If InitController() was called in the Init3() static resource arrays were populated already and invocation of this 
		// constructor has zeroed the dynamic pointer arrays (calling their default constructors). In such case we need to 
		// restore these arrays from their temporary shadow copies (i.e. copies of RPointerArray objects, not their content)
		// (See comments in RegisterStaticResources())
		if(StaticResourceArrayPtr)
			{
			// by making a (binary) copy of RPointerArray object (compiler's auto-generated code)
			// we are taking the ownership of content (pointers stored/owned by that array) of this temporary array
			iStaticResourceArray = *StaticResourceArrayPtr;
			}
#ifdef PRM_ENABLE_EXTENDED_VERSION
		// the same applies to static resources with dependencies for extended version.
		// Temporary object are de-allocated in InitResources()
		if(StaticResourceDependencyArrayPtr)
			{
			iStaticResDependencyArray = *StaticResourceDependencyArrayPtr;
			}
#endif
		return;
		}
    PowerResourceController = this;
	iClientList.Initialise(0);
	iUserSideClientList.Initialise(0);
	iInitialised = EResConCreated;
#ifdef PRM_ENABLE_EXTENDED_VERSION
	iDynamicResourceList.Initialise(0);
	iDynamicResDependencyList.Initialise(0);
#endif
	}

/** Send notificatins to clients registered for it for the specified resource. */
void DPowerResourceController::CompleteNotifications(TInt aClientId, DStaticPowerResource* aResource, TInt aState, 
													      TInt aReturnCode, TInt aLevelOwnerId, TBool aLock)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::CompleteNotifications"));
    if(aLock)
		Lock();
    DPowerResourceNotification*pN=NULL;
    for(SDblQueLink* pNL=aResource->iNotificationList.First();pNL!=&aResource->iNotificationList.iA; pNL=pNL->iNext)
		{
        pN = _LOFF(pNL, DPowerResourceNotification, iNotificationLink);
#ifdef PRM_ENABLE_EXTENDED_VERSION
		//If dyanmic resource is deregistering, send notification to all clients requested for it
		if((pN->iCallback.iResourceId & KIdMaskDynamic) && (aClientId == KDynamicResourceDeRegistering))
			{
			pN->iCallback.iResult=aReturnCode;			
			pN->iCallback.iMutex = iResourceMutex;
			pN->iCallback.iPendingRequestCount++;
			pN->iCallback.iResult = aReturnCode;
			pN->iCallback.iLevel = aState;
			pN->iCallback.iClientId = aClientId;
			pN->iCallback.iLevelOwnerId = aLevelOwnerId;
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("Notification ClientId = 0x%x, ResourceId = %d, state = %d, Result = %d", 
									pN->iCallback.iClientId, pN->iCallback.iResourceId, aState, aReturnCode));
			PRM_POSTNOTIFICATION_SENT_TRACE
			pN->iCallback.Enque();
			continue;
			}	
#endif
		if((pN->iType==DPowerResourceNotification::EUnconditional) || 
				(pN->iDirection && ((pN->iPreviousLevel < pN->iThreshold) && (aState >= pN->iThreshold))) ||
				(!pN->iDirection && ((pN->iPreviousLevel > pN->iThreshold) && (aState <= pN->iThreshold))))
			{
			pN->iCallback.iResult=aReturnCode;
			pN->iCallback.iMutex = iResourceMutex;
            pN->iCallback.iPendingRequestCount++;			
			pN->iCallback.iLevel=aState;
			pN->iCallback.iClientId = aClientId;
			pN->iCallback.iLevelOwnerId = aLevelOwnerId;
			
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("Notifications ClientId = 0x%x, ResourceId = %d, State = %d, Result = %d",
										pN->iCallback.iClientId, pN->iCallback.iResourceId, aState, aReturnCode));
			PRM_POSTNOTIFICATION_SENT_TRACE

			pN->iCallback.Enque();

			}
		pN->iPreviousLevel = aState; //Update the state
		}
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::CompleteNotifications"));
	if(aLock)
		UnLock();
	return;
	}

/** Complete the asynchronous request. */
void DPowerResourceController::CompleteRequest(TPowerRequest& aRequest)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::CompleteRequest"));
 	// Complete notification for state change operation
    DStaticPowerResource* pR=aRequest.Resource();
	//If request in EChange or ESetDefaultValue and no error and if shared resources and if change is done then 
	//issue notification.
	if(((aRequest.ReqType() == TPowerRequest::EChange) || (aRequest.ReqType() == TPowerRequest::ESetDefaultLevel)) 
		         && (aRequest.ReturnCode() == KErrNone) && ((!pR->Usage()) || (pR->Usage() && aRequest.RequiresChange())))
		{	
    	CompleteNotifications(aRequest.ClientId(), aRequest.Resource(), aRequest.Level(), 
		                                                     aRequest.ReturnCode(), aRequest.ClientId());
		}
	//Do not update the level if the resource is shared and change is not required or any error.
    if(aRequest.ReturnCode()==KErrNone && ((aRequest.ReqType() ==TPowerRequest::EGet) || 
		       (((aRequest.ReqType()==TPowerRequest::EChange) || (aRequest.ReqType()==TPowerRequest::ESetDefaultLevel)) 
			   && ((!pR->Usage()) || (pR->Usage() && aRequest.RequiresChange())))))
        {
        Lock();
		// Cache the latest value
        pR->iCachedLevel=aRequest.Level();
		//Need to update client ID only during state change.
		if(aRequest.ReqType() != TPowerRequest::EGet)
			pR->iLevelOwnerId=aRequest.ClientId();
		// Cache Idle list entry for this reosurce if requested.
        if(pR->iIdleListEntry)
            {
            SIdleResourceInfo* pI=pR->iIdleListEntry;
            if(aRequest.ReqType() != TPowerRequest::EGet)
				pI->iLevelOwnerId= aRequest.ClientId();
            pI->iCurrentLevel=aRequest.Level();
            }
         UnLock();
        }
	
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::CompleteRequest"));
	}

/** Handle/process the asynchronous request sent to resource controller.
   The request can be one of the following
   1) State change of long latency reosurce
   2) Get the state of long latency resource
   3) Set the default value of long latency resource */
void DPowerResourceController::HandleMsg(TPowerRequest& aRequest)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::HandleMsg"));
    DStaticPowerResource* pR=aRequest.Resource();
	//Get client using client ID
	TUint aClientId = aRequest.ClientId();
	SPowerResourceClient* pC = NULL;
#ifdef PRM_ENABLE_EXTENDED_VERSION
	if((TInt)aClientId != KDynamicResourceDeRegistering)
		{
		if(aClientId & USER_SIDE_CLIENT_BIT_MASK)
			pC = iUserSideClientList[TUint16(aClientId & ID_INDEX_BIT_MASK)];
		else
			pC = iClientList[TUint16(aClientId & ID_INDEX_BIT_MASK)];
		}
#else
		if(aClientId & USER_SIDE_CLIENT_BIT_MASK)
			pC = iUserSideClientList[TUint16(aClientId & ID_INDEX_BIT_MASK)];
		else
			pC = iClientList[TUint16(aClientId & ID_INDEX_BIT_MASK)];
#endif		
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("Request Type = %d, ClientId = 0x%x, ResourceId = %d",
		                             aRequest.ReqType(), aRequest.ClientId(), aRequest.ResourceId()));
    if(aRequest.ReqType()==TPowerRequest::EChange)
		{
        if(pR->Usage()) //Handling shared resource
			{
			Lock(); //To avoid race condition between deregister resource level.
			aRequest.ReturnCode() = CheckLevelAndAddClient(pC, &aRequest);
			UnLock();
			if((aRequest.ReturnCode()!= KErrNone) || (!aRequest.RequiresChange()))
				{
				aRequest.Level() = pR->iCachedLevel; //If no change then send the current level back.
    			CompleteRequest(aRequest);
				return;
				}
			}
		else if(pR->iLevelOwnerId ==-1)  //No existing client.
			{
			// Add client Level
			if(pC->iReservedCl==0 && !iClientLevelPoolCount)
				{
				__KTRACE_OPT(KRESMANAGER, Kern::Printf("Reserved Client Level exhausted and its free pool empty"));
                aRequest.ReturnCode() = KErrUnderflow;
                CompleteRequest(aRequest);
				return;
				}
			SPowerResourceClientLevel* pSCL=NULL;
			LIST_POP(iClientLevelPool, pSCL, iNextInList);
			pSCL->iClientId=aClientId;
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
			}
		else
			{
			//Update the level in the client list.
			SPowerResourceClientLevel* pSCL = (SPowerResourceClientLevel*)pR->iClientList.First();
			pSCL->iLevel = aRequest.Level();
			}
		}
	else if(aRequest.ReqType()==TPowerRequest::ESetDefaultLevel)	
		{
#ifdef PRM_ENABLE_EXTENDED_VERSION
		if((aRequest.ResourceId() & KIdMaskDynamic) && ((TInt)aClientId == KDynamicResourceDeRegistering))
			{
			//Try to change the resource to requested level and if that fails try to change it to default level
			if(pR->iDefaultLevel != aRequest.Level())
				{
				aRequest.ReqType() = TPowerRequest::EChange;
				aRequest.ReturnCode() = pR->DoRequest(aRequest);
				if(aRequest.ReturnCode() != KErrNone)
					{
					aRequest.ReqType() = TPowerRequest::ESetDefaultLevel;
					aRequest.Level() = pR->iDefaultLevel;
					pR->DoRequest(aRequest);
					}
				}
			else
				pR->DoRequest(aRequest);
			aRequest.ReturnCode() = KErrNone;
			aRequest.RequiresChange() = ETrue;
			CompleteRequest(aRequest);
			return;
			}
#endif
        if(pR->Usage())
			{
			aRequest.ReturnCode() = CheckLevelAndAddClient(pC, &aRequest);
			if((aRequest.ReturnCode()!= KErrNone) || (!aRequest.RequiresChange()))
				{
				aRequest.Level() = pR->iCachedLevel; //If no change then send the current level back.
    			CompleteRequest(aRequest);
				return;
				}
			}
		else
			{
			aRequest.ClientId() = -1;
			aRequest.Level() = pR->iDefaultLevel;
			}
		}
	if((aRequest.ReqType() == TPowerRequest::EGet) || (pR->iCachedLevel != aRequest.Level()))
		aRequest.ReturnCode() = pR->DoRequest(aRequest);
	CompleteRequest(aRequest);
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::HandleMsg"));
	}

#ifdef PRM_ENABLE_EXTENDED_VERSION
/** Handle/process the dependency resource.
   The request can be one of the following
   1) State change of a dependency resource
   2) Get the state of a dependency resource
   3) Set the default value of a dependency resource */
void DPowerResourceController::HandleDependencyMsg(TPowerRequest& aRequest)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::HandleDependencyMsg"));
    DStaticPowerResource* pR=aRequest.Resource();
	//Get client using client ID
	TUint aClientId = aRequest.ClientId();
	SPowerResourceClient* pC = NULL;

	if((TInt)aClientId != KDynamicResourceDeRegistering)
		{
		if(aClientId & USER_SIDE_CLIENT_BIT_MASK)
			pC = iUserSideClientList[TUint16(aClientId & ID_INDEX_BIT_MASK)];
		else
			pC = iClientList[TUint16(aClientId & ID_INDEX_BIT_MASK)];
		}
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("Request Type = %d, ClientId = 0x%x, ResourceId = %d",  
		                             aRequest.ReqType(), aRequest.ClientId(), aRequest.ResourceId()));
	if((aRequest.ResourceId() & KIdMaskResourceWithDependencies) && (aRequest.ReqType() != TPowerRequest::EGet))
		{
		Lock();
		iDfcQDependencyLock = ETrue;
		UnLock();
		PowerResourceController->HandleDependencyResourceStateChange(pC, aRequest);
		Lock();
		iDfcQDependencyLock = EFalse;
		UnLock();		
		return;
		}
	//Get the resource current level.
	aRequest.ReturnCode() = pR->DoRequest(aRequest);
	CompleteRequest(aRequest);
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::HandleDependencyMsg"));
	}
#endif

/** Function called whenever there is a message in resource controller message queue. */
void DPowerResourceController::MsgQFunc(TAny* aPtr)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::MsgQFunc"));
    DPowerResourceController* pRC=(DPowerResourceController*)aPtr;
    TPowerRequest* aReq=(TPowerRequest*)pRC->iMsgQ->iMessage;
	DStaticPowerResource *pR = aReq->Resource();
	if(aReq->ReqType() == TPowerRequest::EAllocReserve)
		{
		aReq->ReturnCode() = pRC->HandleReservationOfObjects(*aReq);
		aReq->Complete(aReq->ReturnCode(),ETrue);
		return;
		}
	if((aReq->ReqType() == TPowerRequest::ERegisterKernelClient) || (aReq->ReqType() == TPowerRequest::ERegisterUsersideClient))
		{
		aReq->ReturnCode() = pRC->HandleClientRegistration(*aReq);
		aReq->Complete(aReq->ReturnCode(), ETrue);
		return;
		}
#ifdef PRM_ENABLE_EXTENDED_VERSION
	if(aReq->ReqType() == TPowerRequest::ERegisterDynamicResource)
		{
		aReq->ReturnCode() = pRC->HandleResourceRegistration(*aReq);
		aReq->Complete(aReq->ReturnCode(), ETrue);
		return;
		}
#endif
    pRC->HandleMsg(*aReq);
#ifdef PRM_ENABLE_EXTENDED_VERSION
	if((aReq->ResourceId() & KIdMaskDynamic) && (aReq->ResourceCb()))
		{
		pRC->Lock();
		((DDynamicPowerResource*)aReq->Resource())->UnLock();
		pRC->UnLock();
		}
#endif
	//Below code is for Btrace
#ifdef PRM_INSTRUMENTATION_MACRO
	SPowerResourceClient* pC = NULL;
	SPowerResourceClient tRes;
#ifdef PRM_ENABLE_EXTENDED_VERSION
 if((aReq->ClientId() == -1) || (aReq->ClientId() == KDynamicResourceDeRegistering))
#else
	if(aReq->ClientId() == -1)
#endif
		{
        pC = &tRes;
        pC->iClientId = (TUint)-1;
        pC->iName = &KNoClient;
		}
    else if(aReq->ClientId() & USER_SIDE_CLIENT_BIT_MASK)
		pC = pRC->iUserSideClientList[TUint16(aReq->ClientId() & ID_INDEX_BIT_MASK)];
	else
		pC = pRC->iClientList[TUint16(aReq->ClientId() & ID_INDEX_BIT_MASK)];

    TUint aResourceId = aReq->ResourceId();
    TInt r = aReq->ReturnCode();
    if(aReq->ReqType()==TPowerRequest::EGet)
		{
		TInt aState = aReq->Level();
		PRM_RESOURCE_GET_STATE_END_TRACE
		}
    else
		{
		TInt aNewState = aReq->Level();
		PRM_CLIENT_CHANGE_STATE_END_TRACE
		}
#endif
	//Check whether callback is canceled and if not queue the DFC.
    TPowerResourceCb* pCb = aReq->ResourceCb();
    if(pCb)
		{
        pCb->iMutex = pRC->iResourceMutex;
        pCb->iPendingRequestCount++;    
        pCb->iResult=aReq->ReturnCode();
        pCb->iLevel=aReq->Level();
        pCb->iResourceId=aReq->ResourceId();
        pCb->iClientId=aReq->ClientId();
		pCb->iLevelOwnerId = pR->iLevelOwnerId;
	    pCb->Enque();
		}
    aReq->Complete(aReq->ReturnCode(),ETrue);
	if(aReq->ResourceCb())
		pRC->MoveRequestToFreePool(aReq);
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::MsgQFunc"));
	return;
    }

#ifdef PRM_ENABLE_EXTENDED_VERSION
/** Function called whenever there is a message in resource controller Dependency message queue. */
void DPowerResourceController::MsgQDependencyFunc(TAny* aPtr)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::MsgQDependencyFunc"));
    DPowerResourceController* pRC=(DPowerResourceController*)aPtr;
    TPowerRequest* aReq=(TPowerRequest*)pRC->iMsgQDependency->iMessage;
	DStaticPowerResource *pR = aReq->Resource();
	pRC->HandleDependencyMsg(*aReq);
	if((aReq->ResourceId() & KIdMaskDynamic) && (aReq->ResourceCb()))
		{
		pRC->Lock();
		((DDynamicPowerResource*)aReq->Resource())->UnLock();
		pRC->UnLock();
		}
	//Below code is for Btrace
#ifdef PRM_INSTRUMENTATION_MACRO
	SPowerResourceClient* pC = NULL;
	SPowerResourceClient tRes;
	if((aReq->ClientId() != -1) && (aReq->ClientId() != KDynamicResourceDeRegistering) && 
		                                        (aReq->ClientId() & KIdMaskResourceWithDependencies))
		{
		pC = &tRes;
		pC->iClientId = aReq->ClientId();
		DDynamicPowerResourceD* pDRes;
		if(aReq->ClientId() & KIdMaskDynamic)
			pDRes = pRC->iDynamicResDependencyList[(aReq->ClientId() & ID_INDEX_BIT_MASK)];	
		else
			pDRes = (DDynamicPowerResourceD*)pRC->iStaticResDependencyArray[(aReq->ClientId() & ID_INDEX_BIT_MASK) - 1];
		
		if (pDRes != NULL)
		    {
            pC->iName = pDRes->iName;
		    }
		else
		    {
            pC->iName = &KNullDesC;
		    }
		}
	else if((aReq->ClientId() == -1) || (aReq->ClientId() == KDynamicResourceDeRegistering))
		{
        pC = &tRes;
        pC->iClientId = (TUint)-1;
        pC->iName = &KNoClient;
		}
    else if(aReq->ClientId() & USER_SIDE_CLIENT_BIT_MASK)
		pC = pRC->iUserSideClientList[TUint16(aReq->ClientId() & ID_INDEX_BIT_MASK)];
	else
		pC = pRC->iClientList[TUint16(aReq->ClientId() & ID_INDEX_BIT_MASK)];

    TUint aResourceId = aReq->ResourceId();
    TInt r = aReq->ReturnCode();
    if(aReq->ReqType()==TPowerRequest::EGet)
		{
		TInt aState = aReq->Level();
		PRM_RESOURCE_GET_STATE_END_TRACE
		}
    else
		{
		TInt aNewState = aReq->Level();
		PRM_CLIENT_CHANGE_STATE_END_TRACE
		}
#endif
	//Check whether callback is canceled and if not queue the DFC.
    TPowerResourceCb* pCb = aReq->ResourceCb();
    if(pCb)
		{
        pCb->iMutex = pRC->iResourceMutex;
        pCb->iPendingRequestCount++;       
        pCb->iResult=aReq->ReturnCode();
        pCb->iLevel=aReq->Level();
        pCb->iResourceId=aReq->ResourceId();
        pCb->iClientId=aReq->ClientId();
		pCb->iLevelOwnerId = pR->iLevelOwnerId;
	    pCb->Enque();
		}
    aReq->Complete(aReq->ReturnCode(),ETrue);
	if(aReq->ResourceCb())
		pRC->MoveRequestToFreePool(aReq);
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::MsgQDependencyFunc"));
	return;
    }
#endif
/** Function to move the request object to free pool and update client request count accordingly. */
void DPowerResourceController::MoveRequestToFreePool(TPowerRequest *aReq)
	{
    //Return request to free pool
    SPowerRequest* pS=_LOFF(aReq, SPowerRequest, iRequest);
    Lock();
    LIST_PUSH(iRequestPool, pS, iNext);
	SPowerResourceClient* pC = NULL;
	if(aReq->ClientId() & USER_SIDE_CLIENT_BIT_MASK)
		pC = iUserSideClientList[TUint16(aReq->ClientId() & ID_INDEX_BIT_MASK)];
	else
		pC = iClientList[TUint16(aReq->ClientId() & ID_INDEX_BIT_MASK)];
	pC->iPendingReqCount--;
	if(pC->iUnderFlowRmCount > 0)
		{
        iRequestPoolCount++;
        pC->iUnderFlowRmCount--;
		}
    else
        pC->iReservedRm++;
    UnLock();
    return;
	}

/** This function is called by PSL to set the DFC queue created */
void DPowerResourceController::SetDfcQ(TDfcQue* aDfcQ)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::SetDfcQ"));
    iDfcQ=aDfcQ;
    iMsgQ->SetDfcQ(iDfcQ);
	}

#ifdef PRM_ENABLE_EXTENDED_VERSION
/** This function is called by PSL to set the DFC Dependency queue created */
void DPowerResourceController::SetDfcQDependency(TDfcQue* aDfcQDependency)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::SetDfcQDependency"));
    iDfcQDependency=aDfcQDependency;
    iMsgQDependency->SetDfcQ(iDfcQDependency);
	}
#endif
/**This is called as a result of DFC queued in supervisor thread to complete the initialisation
   of resource controller.It registers the resource controller with the power controller. It also
   calls PSL (DoInitResources()) to initialise all static resources to their post-reboot state.
   Finally mark resource controller as fully initialised (ready to accept state change and get request)
   and start the message queue if exists. */
TInt DPowerResourceController::InitResources()
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::InitResources()"));
	TInt count;
	//Create a Kernel client object for Power Controller
	Lock();
	SPowerResourceClient * pC = NULL;
	// By now client pool should be created
	LIST_POP(iClientPool, pC, iNextInList);
	TInt growBy = iClientList.GrowBy();
	if(!pC)
		{
		UnLock();
		// coverity[alloc_fn]
		SPowerResourceClient *pCL = new SPowerResourceClient[growBy];
		if(!pCL)
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("No memory to grow client pool"));
			Panic(ENoMemToCreatePowerControllerClient);
			}
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
		iCleanList.Append(pCL);
#endif
		Lock();
        for(count = 0; count < growBy-1; count++)
			LIST_PUSH(iClientPool, &pCL[count], iNextInList);
		pC = &pCL[count];
#ifdef PRM_INSTRUMENTATION_MACRO
	TUint size = growBy *sizeof(SPowerResourceClient);
	PRM_MEMORY_USAGE_TRACE
#endif
		}
	pC->iName = (const TDesC8*)&KPowerController;
	UnLock();
	if(iClientList.Allocd()==iClientList.Count())
		{
		if(iClientList.ReSize(growBy))
			{
			Panic(ENoMemToCreatePowerControllerClient);
			}
		}
	Lock();
	iClientList.Add(pC, iPowerControllerId);
	pC->iClientId = iPowerControllerId | CLIENT_POWER_CONTROLLER_BIT_MASK;
	iPowerControllerId = pC->iClientId;
    iClientCount++;
    if(TPowerController::PowerController())
		{
		if(TPowerController::PowerController()->RegisterResourceController(this, iPowerControllerId))
			{
#ifndef RESOURCE_MANAGER_SIMULATED_PSL
			Panic(EControllerAlreadyExists);	//	Panic with this error for any error returned by RegisterResourceController
#endif
			}
		}

	iInitialised =EResConStartupCompleted;
	UnLock();
	//Check the resource for postboot level and send notifications to clients registered for it.
	DStaticPowerResource *pR = NULL;
	TInt r;
	TPowerRequest req = TPowerRequest::Get();
	//For Static resource with no dependencies
    for(count = 0; count < iStaticResourceArray.Count(); count++)
		{
		pR = iStaticResourceArray[count];
		if(pR && (pR->iFlags & SET_VALID_POST_BOOT_LEVEL))
			{
            //Form the request message
			req.ReqType() = TPowerRequest::EChange;
			req.ResourceId() = count+1;
			req.ClientId() = -1;
			req.Level() = pR->iPostBootLevel;
			req.Resource() = pR;
			req.ResourceCb() = NULL;
			req.RequiresChange() = ETrue;
			r = pR->DoRequest(req);
			if(r == KErrNone)
				{
				CompleteNotifications(-1, pR, req.Level(), r, -1, ETrue);
				pR->iCachedLevel = req.Level(); //Update the cached level.
				}
			}
		}
#ifdef PRM_ENABLE_EXTENDED_VERSION
	//For Static resource with dependencies 
	for(count = 0; count < iStaticResDependencyArray.Count(); count++)
		{
		pR = iStaticResDependencyArray[count];
		if(pR->iFlags & SET_VALID_POST_BOOT_LEVEL)
			{
			req.ReqType() = TPowerRequest::EChange;
			req.ResourceId() = ((DStaticPowerResourceD*)pR)->iResourceId;
			req.ClientId() = -1;
			req.Level() = pR->iPostBootLevel;
			req.Resource() = pR;
			req.ResourceCb() = NULL;
			req.RequiresChange() = ETrue;
			//Form the request message
			((DStaticPowerResourceD*)pR)->HandleChangePropagation(req, EChangeStart, req.ClientId(), KNoClient);
			}
		}
#endif
	
	// delete the temporary copy of static resource array used during initialization.
	if(StaticResourceArrayPtr)
		{
		delete StaticResourceArrayPtr;
		StaticResourceArrayPtr = NULL;
		}
#ifdef PRM_ENABLE_EXTENDED_VERSION
	// the same applies to dependency resources array for extended version.
	if(StaticResourceDependencyArrayPtr)
		{
		delete StaticResourceDependencyArrayPtr;
		StaticResourceDependencyArrayPtr = NULL;
		}
#endif
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::InitResources()"));
    return KErrNone;
	}

/** @internalComponent
   This function is called for shared resources to determine level for the shared resource.
   This takes care of updating the resource level for each client. 
*/
TInt DPowerResourceController::CheckLevelAndAddClient(SPowerResourceClient* pC, TPowerRequest* aReq)
	{
    //Client level addition in state change needs to be taken care.
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("DPowerResourceController::CheckLevelAndAddClient, ClientId = 0x%x, ResourceId = %d, ReqType = %d",
													pC->iClientId, aReq->ResourceId(), aReq->ReqType()));
	
	SPowerResourceClientLevel* pSCL=NULL; //Place to hold the current client
	SPowerResourceClientLevel* pMCL=NULL; //Place to hold the prevailing client.
	DStaticPowerResource* aResource = aReq->Resource();
    aReq->RequiresChange() = EFalse;
    TInt maxLevel=KMinTInt;
   	TInt CurrentLevel;
	TInt count = 0;
	//Get the nextmaximum, current client information.If the change is requested by client holding the prevailing
	//level of the resource then maxlevel will be next highest level with respect to sense. Otherwise will contain
	//the maximum level.
	SPowerResourceClientLevel* pL = NULL;
    for(SDblQueLink* pCL=aResource->iClientList.First();pCL!=&aResource->iClientList.iA;pCL=pCL->iNext,count++)
		{
        pL=(SPowerResourceClientLevel*)pCL;
		if(pL->iClientId == pC->iClientId)
			{
			pSCL=pL;
			if(aResource->Sense() == DStaticPowerResource::ECustom)
				break;
			continue;
			}
		
		if((count == 0) || ((pSCL != NULL) && (maxLevel == KMinTInt)))
			{
			maxLevel = pL->iLevel;
			pMCL = pL;
			continue;
			}
		if(((aResource->Sense() == DStaticPowerResource::ENegative) && (pL->iLevel < maxLevel)) || 
			                      ((aResource->Sense() == DStaticPowerResource::EPositive) && (pL->iLevel > maxLevel)))
			{
			maxLevel=pL->iLevel;
			pMCL = pL;
		    }
		}
	//Get the current level.
	if(((TInt)pC->iClientId == aResource->iLevelOwnerId))
		// coverity[var_deref_op]
		CurrentLevel = pSCL->iLevel;
    else
		CurrentLevel = maxLevel;

#ifdef PRM_ENABLE_EXTENDED_VERSION
	if(aResource->iLevelOwnerId & KIdMaskResourceWithDependencies)
		{
		CurrentLevel = aResource->iCachedLevel;
		}
#endif
	TBool newClient = EFalse;
    if(!pSCL)
		{
		// If the client is new, get free client level from pool and populate with client information
		// and add it to the client list and in resource list.
        if((pC->iReservedCl ==0) && !iClientLevelPoolCount)
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("Client level quota exhausted and its free pool empty, iReservedCl = %d, iClientLevelPoolCount = %d", 
													pC->iReservedCl, iClientLevelPoolCount));
			return KErrUnderflow;
			}
        LIST_POP(iClientLevelPool, pSCL, iNextInList);
        pSCL->iClientId=pC->iClientId;
        pSCL->iResourceId=aReq->ResourceId();
        pSCL->iLevel = aReq->Level();
        //Add to the resource list
        aResource->iClientList.Add(pSCL);
        //Add to the client List
        LIST_PUSH(pC->iLevelList, pSCL, iNextInList);
        if(pC->iReservedCl == 0)
			{
			iClientLevelPoolCount--;
			pC->iUnderFlowClCount++;
			}
        else
          pC->iReservedCl--;
		//If no client is holding the resource already and is not custom sense resource, then change is allowed
        if((aResource->iLevelOwnerId == -1) && (aResource->Sense() != DStaticPowerResource::ECustom))
			{
			aReq->RequiresChange() = ETrue;
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DPowerResourceController::CheckLevelAndAddClient"));
			return KErrNone;
			}
		if(aResource->Sense() == DStaticPowerResource::ECustom)
			newClient = ETrue;
		}
    else
		// Capture the new level requested by the client.
        pSCL->iLevel=aReq->Level();

    if(aResource->Sense() == DStaticPowerResource::ECustom)
		{
		
        if(!aResource->iCustomFunction)
            Panic(ECustomFunctionNotSet);
		// coverity[var_deref_op]
		if(aReq->ReqType() == TPowerRequest::EChange)
		    {
            aReq->RequiresChange() = aResource->iCustomFunction(aReq->ClientId(), *(pC->iName), aReq->ResourceId(),
                                                                newClient ? EClientRequestLevel : EClientChangeLevel,
                                                                aReq->Level(), (TAny*)&aResource->iClientList, NULL);
		    }
		else
		    {
            aReq->RequiresChange() = aResource->iCustomFunction(aReq->ClientId(), *(pC->iName), aReq->ResourceId(),
                                                                EClientRelinquishLevel,
                                                                aReq->Level(), (TAny*)&aResource->iClientList, NULL);
		    }
		if((aReq->ClientId() != -1) && (aReq->ClientId() != (TInt)pC->iClientId) )
			{
			//Check whether the updated client Id (by custom function) is in the client level list. 
			for(SDblQueLink* pCL=aResource->iClientList.First();pCL!=&aResource->iClientList.iA;pCL=pCL->iNext)
				{
				pL = (SPowerResourceClientLevel*)pCL;
				if((TInt)pL->iClientId == aReq->ClientId())
					break;
				}
#ifdef PRM_ENABLE_EXTENDED_VERSION
			if(aReq->ClientId() & (1 << RESOURCE_BIT_IN_ID_CHECK))
				{
				if(aResource->iResourceId & KIdMaskDynamic)
					pL = ((DDynamicPowerResourceD*)aResource)->iResourceClientList;
				else
					pL = ((DStaticPowerResourceD*)aResource)->iResourceClientList;
				while(pL != NULL)
					{
					if((TInt)pL->iClientId == aReq->ClientId())
						break;
					}
				}
#endif
			// coverity[var_deref_op]
			if((TInt)pL->iClientId != aReq->ClientId())
				Panic(EClientIdNotInClientLevelList);
			}
		if(!aReq->RequiresChange() && (aReq->ClientId() != (TInt)pC->iClientId))
	        {
		    aResource->iLevelOwnerId=aReq->ClientId();
			//Update resource details for Idle
		    if(aResource->iIdleListEntry)
				aResource->iIdleListEntry->iLevelOwnerId=aReq->ClientId();
			}
		__KTRACE_OPT(KRESMANAGER, Kern::Printf("DPowerResourceController::CheckLevelAndAddClient"));
        return KErrNone;
		}
    //Handle client deregistration
	if(aReq->ReqType() == TPowerRequest::ESetDefaultLevel) 
		{
		aReq->RequiresChange() = ETrue;
		// If the client is the only ask PSL to set to default level.
		if(count == 1)
			{
			aReq->ReqType() = TPowerRequest::ESetDefaultLevel;
			aReq->Level() = aResource->iDefaultLevel;
			aReq->ClientId() = -1;
			}
        else
			{
			//Change the state to next maximum level with respect to sense.
			aReq->ReqType() = TPowerRequest::EChange;
			// coverity[var_deref_op]
			aReq->ClientId() = pMCL->iClientId;
			aReq->Level() = pMCL->iLevel;
			if(pSCL->iLevel == pMCL->iLevel)
				{
                //Change the client alone and level remains the same.
                aResource->iLevelOwnerId = pMCL->iClientId;
                if(aResource->iIdleListEntry)
                    aResource->iIdleListEntry->iLevelOwnerId = pMCL->iClientId;
                aReq->RequiresChange() = EFalse;
				}
			}
		__KTRACE_OPT(KRESMANAGER, Kern::Printf("DPowerResourceController::CheckLevelAndAddClient"));
		return KErrNone;
		}

	//If the level is in increasing order with respect to sense the change is allowed.
    if(((aResource->Sense() == DStaticPowerResource::ENegative) && aReq->Level()<CurrentLevel) || 
		                 ((aResource->Sense() == DStaticPowerResource::EPositive) && aReq->Level()>CurrentLevel))
		{
		__KTRACE_OPT(KRESMANAGER, Kern::Printf("Resource is in increasing order with respect to sense and level is %d",
			                                                                                           aReq->Level()));
        aReq->RequiresChange()=ETrue;
        return KErrNone;
		}
    if((TInt)pC->iClientId == aResource->iLevelOwnerId)
		{
         if(aReq->Level() == CurrentLevel)
			{
			 __KTRACE_OPT(KRESMANAGER, Kern::Printf("DPowerResourceController::CheckLevelAndAddClient"));
			return KErrNone;
			}
         if(count == 1)
           {
           aReq->RequiresChange() = ETrue;
		   __KTRACE_OPT(KRESMANAGER, Kern::Printf("DPowerResourceController::CheckLevelAndAddClient"));
           return KErrNone;
           }
		// If the client requesting is the client holding current level, then chnage it to the nextmaximum level.
		// Next maximum level is the max of requesting level or next maximum level.
		if(((aResource->Sense()==DStaticPowerResource::ENegative) && maxLevel < aReq->Level()) || 
			                  ((aResource->Sense()==DStaticPowerResource::EPositive) && maxLevel > aReq->Level()))
		    {
			aReq->Level() = maxLevel;
			aReq->ClientId() = pMCL->iClientId;
			if(maxLevel == CurrentLevel)
				{
       			aResource->iLevelOwnerId=pMCL->iClientId;
				//Update resource details for Idle
				if(aResource->iIdleListEntry)
             		aResource->iIdleListEntry->iLevelOwnerId=pMCL->iClientId;
				aReq->RequiresChange() = EFalse;
				__KTRACE_OPT(KRESMANAGER, Kern::Printf("DPowerResourceController::CheckLevelAndAddClient"));
				return KErrNone;
				}
			}
	      	aReq->RequiresChange() = ETrue;
		}
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("DPowerResourceController::CheckLevelAndAddClient"));
    return KErrNone;
	}

/**
    Initialise pools of request structures, client strutures and client power level structures. 
	By preallocating sufficiently large structures we remove any allocations whilst the resource manager mutex is held.
    The function basically ensures that sufficient memory is preallocated to the resource manager to ensure that none is
    required at run time.
    @param aKClients number of kernel side clients expected in the resource manager
    @param aUClients number of user side clients expected in the resource manager
    @param aNClientLevels number of client levels the RM should preallocate. This is roughly the number of clients
        that are expected to use shared resources multiplied by the number of shared resources.
    @param aNRequest number of simultaneous asynchronous requests the resource manager is likely to handle
    @return KErrNone if preallocations succeed
    @return KErrNoMemory if one the prealocations fails
    */
TInt DPowerResourceController::InitPools(TUint16 aKClients, TUint16 aUClients, TUint16 aNClientLevels, TUint16 aNRequests)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::InitPools"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("aKClients = %d, aUClients = %d, aNClientLevels = %d, aNRequests = %d",
		                                                     aKClients, aUClients, aNClientLevels, aNRequests));
    __ASSERT_ALWAYS((iInitialised == EResConCreated) && !(iClientPool || iRequestPool || iClientLevelPool), Kern::Fault("Already initialized"
		                                                     __FILE__, __LINE__));

    // Create client pool
	SPowerResourceClient* pC = NULL;
	SPowerResourceClientLevel* pCL = NULL;
	SPowerRequest* pR = NULL;
	aKClients++; //Add one default for PowerController
	if(aKClients + aUClients)
		{
		// coverity[alloc_fn]
		pC = new SPowerResourceClient[aKClients+aUClients];
		if(!pC)
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("Client Pool Allocation Failed"));
			return KErrNoMemory;
			}
		}
    // Create Client level pool
	if(aNClientLevels)
		{
		// coverity[alloc_fn]
		pCL = new SPowerResourceClientLevel[aNClientLevels];
		if(!pCL)
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("Client Level Pool Allocation Failed"));
			delete []pC;
			return KErrNoMemory;
			}
		}
    // Create Request pool
	if(aNRequests)
		{
		// coverity[alloc_fn]
		pR = new SPowerRequest[aNRequests];
		if(!pR)
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("Request Pool Allocation Failed"));
			delete []pC;
			delete []pCL;
			return KErrNoMemory;
			}
		}
    //Create the client Array for kernel and user side clients.
	if(iClientList.Initialise(aKClients))
		{
        delete []pC;
        delete []pCL;
        delete []pR;
        return KErrNoMemory;
		}
 	if(iUserSideClientList.Initialise(aUClients))
 		{
        delete []pC;
        delete []pCL;
        delete []pR;
        iClientList.Delete();
		return KErrNoMemory;
		}
#ifdef PRM_ENABLE_EXTENDED_VERSION
	SPowerResourceClientLevel* pRL = NULL;
	if(iStaticResDependencyArray.Count())
		{
		pRL = new SPowerResourceClientLevel[iStaticResDependencyArray.Count()];
		if(!pRL)
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("Resource level Pool Allocation Failed"));
			delete []pC;
			delete []pCL;
			delete []pR;
			iClientList.Delete();
			iUserSideClientList.Delete();
			return KErrNoMemory;
			}
		}
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
	iCleanList.Append(pC);
#endif
	Lock();
	TInt c;
	for(c = 0; c < iStaticResDependencyArray.Count(); c++)
		{
		LIST_PUSH(iResourceLevelPool, &pRL[c], iNextInList);
		}
	iResourceLevelPoolCount = (TUint16)iStaticResDependencyArray.Count();
#else
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
    iCleanList.Append(pC);
#endif
    Lock();
    TUint16 c;
#endif
    // Create Client pool list
    for(c = 0; c < TUint16(aKClients + aUClients); c++)
		{
        LIST_PUSH(iClientPool, &pC[c], iNextInList);
		}
    // Create client level pool list
    for(c = 0; c < aNClientLevels; c++)
		{
        LIST_PUSH(iClientLevelPool, &pCL[c], iNextInList);
		}
    // Create request pool list
    for(c = 0; c < aNRequests; c++)
		{
        LIST_PUSH(iRequestPool, &pR[c], iNext);
		}
    // When the pool is exhausted they are increased by half of initial size. */
    iClientLevelPoolGrowBy=(TUint16)(aNClientLevels/2);
    iRequestPoolGrowBy=(TUint16)(aNRequests/2);
    // Initialise the free pool size
    iClientLevelPoolCount=aNClientLevels;
    iRequestPoolCount=aNRequests;
#ifdef PRM_INSTRUMENTATION_MACRO
	TUint size = (((aKClients + aUClients)*sizeof(SPowerResourceClient)) + 
		         (aNClientLevels * sizeof(SPowerResourceClientLevel)) + (aNRequests * sizeof(SPowerRequest)));
    PRM_MEMORY_USAGE_TRACE
#endif
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::InitPools"));
    UNLOCK_RETURN(KErrNone);
	}

/**
@publishedPartner
@prototype 9.5

Register a client with the Resource Manager. 

@param aClientId  A reference to a client ID: returns a unique handle if registration was
                  successful, 0 otherwise.
@param aName      Descriptor with name for client. The descriptor is created by the client
                  in kernel data space or its user address space.
                  NOTE: Name should ideally relate to component name and should take care
                  of name uniqueness as it is checked only if DEBUG_VERSION macro is enabled.
@param aType      Defines ownership
                  EOwnerProcess - The client ID can be used by all thread in the process to
                  call the resource manager API's
                  EOwnerThread - The client ID can only be used by the thread that registered
                  the client to resource manager to call the PRM API's
                  By default this is set to EOwnerProcess.

@return           KErrNone if the operation was successful,
                  KErrNoMemory if a new client link was needed but could not be created and
								added to the client list,
                  KErrTooBig if the length of the descriptor passed is greater than 32.
				  KErrAlreadyExists if the specified name already exists. This is valid only if
								DEBUG_VERSION macro is enabled. 
				  KErrNotSupported if number of expected kernel side clients is set to zero by 
								PSL.

@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context, but not from null thread or DFC thread1.
@pre Can be used in a device driver
*/
TInt DPowerResourceController::RegisterClient(TUint& aClientId, const TDesC8& aName, TOwnerType aType)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::RegisterClient, Name = %S, Type = %d", &aName, aType));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	//If number of expected kernel side clients is set to 0 during initial configuration
	//then dont allow to configure kernel side clients.
	if(!iClientList.GrowBy())
		return KErrNotSupported;
	if (aName.Length() > KMaxClientNameLength) return KErrTooBig;
	SPowerResourceClient *pC = NULL;
	Lock();
#ifdef DEBUG_VERSION
   if(!iClientList.Find(pC, (TDesC8&)aName))
       UNLOCK_RETURN(KErrAlreadyExists);
#endif
	//Call from thread Id.
	TPowerRequest* req = (TPowerRequest*)&TPowerRequest::Get();
	req->ReqType() = TPowerRequest::ERegisterKernelClient;
	UnLock();
	req->SendReceive(iMsgQ);
	if(req->ReturnCode() == KErrNone)
		{
		pC = iClientList[(req->ClientId() & ID_INDEX_BIT_MASK)];
		if(aType == EOwnerThread)
			{
			pC->iClientId |= CLIENT_THREAD_RELATIVE_BIT_MASK; //Set 31st bit;
			//Store the current thread Id;
			pC->iThreadId = thread.iId;
			}
		pC->iName = &aName;
		aClientId = pC->iClientId;
	    PRM_CLIENT_REGISTER_TRACE
	    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::RegisterClient, clientId = 0x%x", aClientId));
		}
	return(req->ReturnCode());
	}

#ifdef PRM_ENABLE_EXTENDED_VERSION
TInt DPowerResourceController::HandleResourceRegistration(TPowerRequest& aReq)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::HandleResourceRegistration"));
	DDynamicPowerResource* pDRes = (DDynamicPowerResource*)aReq.Resource();
		//Add to appropriate container
	if(pDRes->iResourceId & KIdMaskResourceWithDependencies)
		ADD_TO_RESOURCE_CONTAINER(iDynamicResDependencyList, ((DDynamicPowerResourceD*)pDRes), aReq.ResourceId(), 
		                                                                   iDynamicResDependencyCount)
	else 
		ADD_TO_RESOURCE_CONTAINER(iDynamicResourceList, pDRes, aReq.ResourceId(), iDynamicResourceCount)

	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::HandleResourceRegistration"));
	return KErrNone;
	}
#endif

/**
@internalComponent
@prototype 9.5

This function runs in the context of the RC thread and 
handles registration of client (kernel and user side). 
*/
TInt DPowerResourceController::HandleClientRegistration(TPowerRequest& aRequest)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::RegisterClient"));
	SPowerResourceClient* pC = NULL;
	TUint clientId;
	if(aRequest.ReqType() == TPowerRequest::ERegisterKernelClient)
		{
		//Get Next client from FreePool
		LIST_POP(iClientPool, pC, iNextInList);

		TInt growBy = iClientList.GrowBy();
		if(!pC)
			{
			//Free Pool is empty, so try to grow the pool.
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("Client pool exhausted so growing client Pool by %d", growBy));
			// coverity[alloc_fn]
			SPowerResourceClient *pCL = (SPowerResourceClient*)Kern::Alloc(sizeof(SPowerResourceClient) * growBy);
			if(!pCL)
				{
				__KTRACE_OPT(KRESMANAGER, Kern::Printf("No memory to grow client pool"));
				return(KErrNoMemory);
				}
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
			iCleanList.Append(pCL);
#endif
			Lock();
			TUint16 count;
			for(count = 0; count < growBy-1; count++)
				LIST_PUSH(iClientPool, &pCL[count], iNextInList);
			UnLock();
#ifdef PRM_INSTRUMENTATION_MACRO
	TUint size = growBy *sizeof(SPowerResourceClient);
	PRM_MEMORY_USAGE_TRACE
#endif
			pC = &pCL[count];
			}
		pC->iClientId = 0;
		if(iClientList.Allocd()==iClientList.Count())
			{
			//Resize the container for holding client list
			if(iClientList.ReSize(growBy)!=KErrNone)
				{
				__KTRACE_OPT(KRESMANAGER, Kern::Printf("No memory for client container allocation"));
				return(KErrNoMemory);
				}
			}
		Lock();
		iClientList.Add(pC, clientId);
		++iClientCount;
		UnLock();
		}
	else // Request is registration of user side client
		{
		//Get Next client from FreePool
		LIST_POP(iClientPool, pC, iNextInList);
		TInt growBy = iUserSideClientList.GrowBy();
		if(!pC)
			{
			//Free Pool is empty, so try to grow the pool.
			SPowerResourceClient *pCL = (SPowerResourceClient*)Kern::Alloc(sizeof(SPowerResourceClient) * growBy);
			if(!pCL)
				{
				return KErrNoMemory;
				}
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
			iCleanList.Append(pCL);
#endif
			Lock();
			TInt count;
			for(count = 0; count < growBy - 1; count++)
				LIST_PUSH(iClientPool, &pCL[count], iNextInList);
			UnLock();
#ifdef PRM_INSTRUMENTATION_MACRO
		TUint size = growBy * sizeof(SPowerResourceClient);
		PRM_MEMORY_USAGE_TRACE
#endif
    		pC = &pCL[count];
			}
		pC->iClientId = 0;
		//User side clients are always thread relative as they execute in the context of proxy driver.
		pC->iClientId = CLIENT_THREAD_RELATIVE_BIT_MASK; //Set 31st bit;
		pC->iClientId|=USER_SIDE_CLIENT_BIT_MASK;
		if(iUserSideClientList.Allocd()==iUserSideClientList.Count())
			{
			//Resize the container for holding client list
			if(iUserSideClientList.ReSize(growBy)!=KErrNone)
				{
				__KTRACE_OPT(KRESMANAGER, Kern::Printf("No memory for container class allocation"));
				return KErrNoMemory;
				}
			}
		Lock();
		iUserSideClientList.Add(pC, clientId);
		++iUserSideClientCount;
		UnLock();
		}
	//Create the unique handle for each client
	//Client Handle format
	//  31  30								   18 16    15    14 13	                                 0
	// ¦----¦-----------------------------------¦----¦-----¦-----¦-----------------------------------¦
	// ¦ T/P¦ Container's instance count(15 bits¦C/R ¦  PC ¦ K/U ¦ Index into Client array container ¦
	// ¦----¦-----------------------------------¦----¦-----¦-----¦-----------------------------------¦
	// T/P -> Thread / process relative
	// PC -> Power Controller reserved ID.
	// K/U -> Kernel / User side clients
	// C/R -> Client / Resource Id. This bit will be set for dependency resource Id, zero for clientId.
  	pC->iLevelList = NULL;
    pC->iNotificationList = NULL;
	pC->iDynamicResCount = 0; 	
    pC->iReservedCl = 0;
    pC->iReservedRm = 0;
    pC->iPendingReqCount = 0;
    pC->iUnderFlowRmCount = 0;
    pC->iUnderFlowClCount = 0;
	pC->iClientId |= clientId;
	aRequest.ClientId() = pC->iClientId;
	return KErrNone;
	}

/**	@internalComponent
	@prototype 9.5
	This is called as the result of client deregistration and takes care of resource state changes
	(to appropriate levels) of all the resources the client is holding active requirement. */
void DPowerResourceController::ResourceStateChangeOfClientLevels(SPowerResourceClient* pC)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::ResourceStateChangeOfClientLevels"));
    TPowerRequest* pReq = (TPowerRequest*)&TPowerRequest::Get();
    DStaticPowerResource* pR = NULL;
    SPowerResourceClientLevel* pCL = pC->iLevelList;
    SPowerResourceClientLevel* pCLL = NULL;
	while(pCL != NULL)
		{
        __KTRACE_OPT(KRESMANAGER, Kern::Printf("Client 0x%x has requirement on resource %d", pCL->iClientId, pCL->iResourceId));
#ifdef PRM_ENABLE_EXTENDED_VERSION
		switch((pCL->iResourceId >>RESOURCE_BIT_IN_ID_CHECK) & 0x3)
			{
			case PRM_STATIC_RESOURCE:
			pR = iStaticResourceArray[pCL->iResourceId - 1];
				break;
			case PRM_DYNAMIC_RESOURCE:
			pR = (iDynamicResourceList[(pCL->iResourceId & ID_INDEX_BIT_MASK)]);
				break;
			case PRM_STATIC_DEPENDENCY_RESOURCE:
			pR = (iStaticResDependencyArray[(pCL->iResourceId & ID_INDEX_BIT_MASK) - 1]);
				break;
			case PRM_DYNAMIC_DEPENDENCY_RESOURCE:
			pR = (iDynamicResDependencyList[(pCL->iResourceId & ID_INDEX_BIT_MASK)]);
				break;
			}
#else
		pR = iStaticResourceArray[pCL->iResourceId - 1];
#endif
#ifdef PRM_ENABLE_EXTENDED_VERSION
		if(pR  &&  (((pR->Sense() == DStaticPowerResource::ECustom) || ((TInt)pCL->iClientId == pR->iLevelOwnerId)) && (!(pCL->iResourceId & KIdMaskDynamic) ||
			         ((pCL->iResourceId & KIdMaskDynamic) && (((DDynamicPowerResource*)pR)->LockCount() != 0)))))
#else
		if(pR  &&  ((pR->Sense() == DStaticPowerResource::ECustom) || ((TInt)pCL->iClientId == pR->iLevelOwnerId))) 
#endif
		    {
            pReq->ReqType() = TPowerRequest::ESetDefaultLevel;
            pReq->ResourceId() = pCL->iResourceId;
            pReq->ClientId() = pCL->iClientId;
            pReq->Resource() = pR;
			pReq->Level() = pR->iCachedLevel;
            pReq->ResourceCb() = NULL;
			pReq->ReturnCode() = KErrNone;
#ifdef PRM_INSTRUMENTATION_MACRO
			//Setting level to current level as correct level will be known only at the end,
			TInt aNewState = pR->iCachedLevel; 
			TUint aResourceId = pReq->ResourceId(); 
			PRM_CLIENT_CHANGE_STATE_START_TRACE
#endif
			TInt r = KErrNone;
			if(pR->LatencySet())
				{
#ifdef PRM_ENABLE_EXTENDED_VERSION
				if(pCL->iResourceId & KIdMaskDynamic)
					((DDynamicPowerResource*)pR)->Lock();
#endif
				UnLock();
#ifdef PRM_ENABLE_EXTENDED_VERSION
				if(pR->iResourceId & KIdMaskResourceWithDependencies) //Dependency resource
					r = pReq->SendReceive(iMsgQDependency);	
				else
#endif
				r = pReq->SendReceive(iMsgQ);
				Lock();
#ifdef PRM_ENABLE_EXTENDED_VERSION
				if(pCL->iResourceId & KIdMaskDynamic)
					((DDynamicPowerResource*)pR)->UnLock();
#endif
				}
			else
				{
	            if(pR->Usage())
					{
					//Not checking return value as there is no memory allocation at this point
					CheckLevelAndAddClient(pC, pReq); 
					}
				else
					{
					pReq->ClientId() = -1;
					pReq->Level() = pR->iDefaultLevel;
					}

				if((!pR->Usage()) || (pR->Usage() && pReq->RequiresChange()))
					{
					// NOTE:Not checking error here as no action can be taken based on error.
					if(pR->iCachedLevel != pReq->Level())
						{
						UnLock();
						r = pR->DoRequest(*pReq);
						Lock();
						}
					CompleteNotifications(pReq->ClientId(), pReq->Resource(), pReq->Level(), 
						                            pReq->ReturnCode(), pReq->ClientId(), EFalse);
#ifdef PRM_INSTRUMENTATION_MACRO
					PRM_CLIENT_CHANGE_STATE_END_TRACE
#endif
					pR->iLevelOwnerId = pReq->ClientId();
					pR->iCachedLevel = pReq->Level();
					if(pR->iIdleListEntry)
						{
						SIdleResourceInfo* pI = (SIdleResourceInfo*)pR->iIdleListEntry;
						pI->iLevelOwnerId = pReq->ClientId();
						pI->iCurrentLevel = pReq->Level();
						}
					}
				}

			}
		/* Deque from resource */
		pCLL = pCL;
		pCL = pCL->iNextInList;
		pCLL->Deque();
		iClientLevelPoolCount++;
		LIST_PUSH(iClientLevelPool,pCLL,iNextInList); // back to free pool
		}
	pC->iLevelList = NULL;
	//Add reserved client level to free pool
	iClientLevelPoolCount = (TUint16)(iClientLevelPoolCount + (TUint16)pC->iReservedCl);
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::ResourceStateChangeOfClientLevels"));
	}

/**
@publishedPartner
@prototype 9.5

Deregister a client with the resource manager

@param aClientId    The ID of the client which is being deregistered

@return KErrNone if the operation was successful
        KErrNotFound if this client ID could not be found in the current
					 list of clients
		KErrArgument if user side client Id is specified or client ID to be used 
					 by Power Controller is specified.
		KErrAccessDenied if client was registered to be thread relative and this 
						 API is not called from the same thread.							

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
TInt DPowerResourceController::DeRegisterClient(TUint aClientId)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::DeRegisterClient, ClientId = 0x%x", aClientId));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	if((aClientId & USER_SIDE_CLIENT_BIT_MASK) || (aClientId == iPowerControllerId))
		return KErrArgument;
	//Get the index from client ID
	Lock();
	SPowerResourceClient* pC = iClientList[(aClientId & ID_INDEX_BIT_MASK)];
    if(!pC)
	    {
        __KTRACE_OPT(KRESMANAGER, Kern::Printf("Client ID not Found"));
        UNLOCK_RETURN(KErrNotFound);
		}
	if(pC->iClientId != aClientId)
		{
        __KTRACE_OPT(KRESMANAGER, Kern::Printf("Client ID does not match"));
	     UNLOCK_RETURN(KErrNotFound);
		}
	if(pC->iClientId & CLIENT_THREAD_RELATIVE_BIT_MASK)
		{
        if(pC->iThreadId != thread.iId)
			{
            __KTRACE_OPT(KRESMANAGER, Kern::Printf("Client not called from thread context(Thread Relative)"));
            UNLOCK_RETURN(KErrAccessDenied);
			}
		}	
	//Check for any pending request
	if(pC->iPendingReqCount)
		{
		UnLock();
		Panic(EClientHasPendingAsyncRequest);
		}
	//Check for notification request
	if(pC->iNotificationList)
		{
		UnLock();
		Panic(EClientHasNotificationObject);
		}
#ifdef PRM_ENABLE_EXTENDED_VERSION
	if(pC->iDynamicResCount)
		{
		UnLock();
		Panic(DPowerResourceController::EClientHasDynamicResourceRegistered);
		}
#endif
	//Check for registration of dynamic resource
	ResourceStateChangeOfClientLevels(pC);
	// Add reserved request to pool
	iRequestPoolCount = (TUint16)(iRequestPoolCount + (TUint16)pC->iReservedRm);
	PRM_CLIENT_DEREGISTER_TRACE
	//Increment the free pool count for client level and request level.
	iClientList.Remove(pC, (pC->iClientId & ID_INDEX_BIT_MASK));
	pC->iName = NULL;
	iClientCount--; //Decrement client count
	LIST_PUSH(iClientPool, pC, iNextInList);
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::DeRegisterClient"));
	UNLOCK_RETURN(KErrNone);
	}

/**
@publishedPartner
@prototype 9.5

Obtain the name of a registered client of the resource manager

@param aClientId   The ID of the client which is requesting the name of
                   another client whose ID is specified in aTargetClientId.
@param aTargetClientId The ID of the client whose name is being requested.
@param aName       Descriptor to be filled with the name of the client. The descriptor
                   is created by the client in kernel stack or heap.

@return            KErrNone if the operation was successful
                   KErrNotFound if this client ID (aTargetClientId) could not be
                   found in the current list of registered clients.
                   KErrAccessDenied if the client ID (aClientId) could not be found
                   in the current list of registered clients or if client was registered
				   to be thread relative and this API is not called from the same thread. 
                   KErrArgument if size of aName is less than 32.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
TInt DPowerResourceController::GetClientName(TUint aClientId, TUint aTargetClientId, TDes8& aName)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::GetClientName, CallingClientId = 0x%x, TargetClientId = 0x%x", aClientId, aTargetClientId));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	if((aName.MaxLength() - aName.Length()) < KMaxClientNameLength)
		return KErrArgument;
	SPowerResourceClient* pC = NULL;
	Lock();
	VALIDATE_CLIENT(thread);
	GET_TARGET_CLIENT();
	aName.Append(*pC->iName);
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::GetClientName, ClientName = %S", &aName));
	UNLOCK_RETURN(KErrNone);
	}

/**
@publishedPartner
@prototype 9.5

Obtain the Id of registered client of the resource manager

@param aClientId   ID of the client which is requesting the ID of the another
                   client whose name is specified in aClientName
@param aClientName Descriptor containing the name of the client whose ID is being
                   requested. The client must create the descriptor in kernel stack
                   or heap.
                   NOTE: Resource manager does not check for uniqueness of client
                   name during registration, so if there are multiple clients registered
                   to PRM with same name it will return the ID of the first client it encounters.
@param aTargetClientId Updates with ID of the requested client on success

@return  KErrNone if the operation was successful
         KErrNotFound if this client name could not be found in the current list of registered
                      client
         KErrAccessDenied if the client ID (aClientId) could not be found in the current
                          list of registered client or if the client was registered to be 
						  thread relative and this API is not called from the same thread. 
         KErrTooBig if the length of the descriptor passed is greater than 32.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
TInt DPowerResourceController::GetClientId(TUint aClientId, TDesC8& aClientName, TUint& aTargetClientId)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::GetClientId CallingClientId = 0x%x, ClientName = %S", aClientId, &aClientName));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	if(aClientName.Length() > KMaxClientNameLength)
		return KErrTooBig;
	SPowerResourceClient* pC = NULL;
	Lock();
	VALIDATE_CLIENT(thread);
	//Find the client ID with the specified name first from kernel client list & then user side.
	if(iClientList.Find(pC, aClientName) && iUserSideClientList.Find(pC, aClientName))
		UNLOCK_RETURN(KErrNotFound);
	aTargetClientId = pC->iClientId;
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::GetClientId TargetClientId = 0x%x", aTargetClientId));
	UNLOCK_RETURN(KErrNone);
	}

/**
@publishedPartner
@prototype 9.5

Obtain the ID of registered resource of the resource manager.
NOTE: ID of the first matching name found in the resource list will be returned

@param aClientId      ID of the client which is requesting the ID of the
                      resource, by specifying its name.
@param aResourceName  Descriptor containing the name of the resource whose
                      ID is being requested.
@param aResourceId    Updates with ID of the requested resource on success

@return KErrNone if the operation was successful
        KErrAccessDenied if the ID of the client could not be found in the
                         current list of registered clients or if the client was 
						 registered to be thread relative and this API is not called
						 from the same thread. 
        KErrNotFound if this resource name could not be found in the current
                     list of registered resources.
		KErrTooBig if the length of the descriptor passed is greater than maximum
				   allowable resource name length (32).
@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
TInt DPowerResourceController::GetResourceId(TUint aClientId, TDesC8& aResourceName, TUint& aResourceId)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::GetResourceId"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("CallingClientId = 0x%x, ResourceName = %S", aClientId, &aResourceName));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	SPowerResourceClient* pC;
	if(aResourceName.Length() > KMaxResourceNameLength)
		return KErrTooBig;
	Lock();
	VALIDATE_CLIENT(thread);
	TInt count = 0;
	//Search in static resource with no dependencies array for specified resource name.
	for(count = 0; count < iStaticResourceArray.Count(); count++)
		{
		if((iStaticResourceArray[count]) && (!(aResourceName.Compare(*(const TDesC8*)iStaticResourceArray[count]->iName))))
			{
			aResourceId = ++count;
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::GetResourceId, ResourceId = 0x%x", aResourceId));
			UNLOCK_RETURN(KErrNone);
			}
		}
#ifdef PRM_ENABLE_EXTENDED_VERSION
	//Search in dynamic resource with no dependencies array for specified resource name.
	DDynamicPowerResource* pDR = NULL;
	if(PowerResourceController->iDynamicResourceCount && 
		           !PowerResourceController->iDynamicResourceList.Find(pDR, aResourceName))
		{
		aResourceId = pDR->iResourceId;
		__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::GetResourceId, ResourceId = 0x%x", aResourceId));
		UNLOCK_RETURN(KErrNone);
		}
	//Search in static resource with dependencies (if exists) for specified resource name
	for(count = 0; count < iStaticResDependencyArray.Count(); count++)
		{
		if(!(aResourceName.Compare(*(const TDesC8*)iStaticResDependencyArray[count]->iName)))
			{
			aResourceId = iStaticResDependencyArray[count]->iResourceId;
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::GetResourceId, ResourceId = 0x%x", aResourceId));
			UNLOCK_RETURN(KErrNone);
			}
		}
	//Search in dynamic resource with dependencies (if exists) for specified resource name
	DDynamicPowerResourceD* pDRD;
	if(iDynamicResDependencyCount && !iDynamicResDependencyList.Find(pDRD, aResourceName))
		{
		aResourceId = pDRD->iResourceId;
		__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::GetResourceId, ResourceId = 0x%x", aResourceId));
		UNLOCK_RETURN(KErrNone);
		}
#endif
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::GetResourceId, ResourceID not found"));
	UNLOCK_RETURN(KErrNotFound);
	}

/**
@publishedPartner
@prototype 9.5

Request a structure containing information on a resource.

@param aClientId    ID of the client which is requesting the resource information
@param aResourceId  ID of the resource whose information is being requested.
@param aInfo        A pointer to descriptor containing resource information
                    structure (TPowerResourceInfoV01) to be filled in
                    with the requested resource information. The client must
                    create the descriptor in kernel stack or heap.

@return KErrNone if the operation was successful
        KErrAccessDenied if the client ID could not be found in the current list
                         of registered clients or if the client was registered to be 
						 thread relative and this API is not called from the same thread.
        KErrNotFound if this resource ID could not be found in the current list
                     of controllable resource.
        KErrArgument if aInfo is NULL or size of descriptor passed is less than size of
					 TPowerResourceInfoV01.
@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
TInt DPowerResourceController::GetResourceInfo(TUint aClientId, TUint aResourceId, TAny* aInfo)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::GetResourceInfo"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("CallingClientId = 0x%x, ResourceId = %d", aClientId, aResourceId));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	if(!aInfo)
		return KErrArgument;
	SPowerResourceClient* pC = NULL;
	Lock();
	VALIDATE_CLIENT(thread);
	TDes8* buf = (TDes8*)aInfo;
	TInt r = KErrNone;
	DStaticPowerResource *pR = NULL; 

	//Validate buffer size
	if((TUint)(buf->MaxLength() - buf->Length()) < sizeof(TPowerResourceInfoV01))
	   UNLOCK_RETURN(KErrArgument);

#ifndef PRM_ENABLE_EXTENDED_VERSION
	if((!aResourceId) || (aResourceId > (TUint)iStaticResourceArray.Count()))
		UNLOCK_RETURN(KErrNotFound);
	//Get resource from static resource array. 0(1) operation.
	pR = iStaticResourceArray[aResourceId-1];
	if(!pR)
		{
		UNLOCK_RETURN(KErrNotFound);
		}
#else
	if(!aResourceId)
		{
		__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::GetResourceInfo, return value = %d", KErrNotFound));
		UNLOCK_RETURN(KErrNotFound);
		}
	//Get resource from corresponding container
	GET_RESOURCE_FROM_LIST(aResourceId, pR) 
#endif
	//Update resource info
	TPowerResourceInfoBuf01 infoBuf;
	r = pR->GetInfo((TDes8*)infoBuf.Ptr());
	//Update ResourceId
	((TPowerResourceInfoV01*)infoBuf.Ptr())->iResourceId = aResourceId;
	if(r == KErrNone)
	   buf->Append(infoBuf);
	UNLOCK_RETURN(r);
	}

/**
@publishedPartner
@prototype 9.5

Request number of resources the specified client (aTargetClientId) has
requirement on resource level. Client ID starts from 1, so if 0 is specified in
aTargetClientId, returns the number of controllable resources registered with PRM.

@param aClientId ID of the client which is requesting the number of resources
                 the specified client (aTargetClientId) holds requirement on
                 resource level change.
@param aTargetClientId ID of the client. The number of resources on which it
                       has requirement on resource level change is requested.
@param aNumResource Updated with the number of resources the specified client
                    has requirement on resource level change, if valid client
                    ID is passed. If client ID is 0, updates the total number
                    of resources registered with resource manager.

@return KErrNone if the operation was successful.
        KErrAccessDenied if the client ID (aClientId) could not be found in the
                         current list of registered clients or if the client was registered
						 to be thread relative and this API is not called from the same thread. 
        KErrNotFound if the client ID (aTargetClientId) could not be found in the
                     current list of registered clients and is not 0.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
TInt DPowerResourceController::GetNumResourcesInUseByClient(TUint aClientId, TUint aTargetClientId, TUint& aNumResource)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::GetNumResourcesInUseByClient"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("CallingClientId = 0x%x, TargetClientId = 0x%x", aClientId, aTargetClientId));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	SPowerResourceClient* pC = NULL;
	Lock();
	VALIDATE_CLIENT(thread);
	//Special case, return number of resources registered resource controller.
	if(!aTargetClientId)
		{
#ifdef PRM_ENABLE_EXTENDED_VERSION
		aNumResource = iStaticResourceCount + iDynamicResourceCount + iStaticResDependencyArray.Count() + 
			                                                          iDynamicResDependencyCount; 
#else
		aNumResource = iStaticResourceCount;
#endif
		__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::GetNumResourcesInUseByClient, numResources = %d", aNumResource));
		UNLOCK_RETURN(KErrNone);
		}
	GET_TARGET_CLIENT();
	SPowerResourceClientLevel* pCL = pC->iLevelList;
	aNumResource = 0;
	while(pCL)
		{
		aNumResource++;
		pCL = pCL->iNextInList;
		}
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::GetNumResourcesInUseByClient, numResources = %d", aNumResource));
	UNLOCK_RETURN(KErrNone);
	}

/**
@publishedPartner
@prototype 9.5

Request information on resources.
If client ID (aTargetClientId) is valid, aInfo is updated with the information of the resources 
this client hold requirement on the resource level.
If client ID (aTargetClientId) is 0, aInfo is updated with the information of the resources 
registered with resource controller.
Number of resource information updated will be equal or less than the number specified in aNumResources.

@param aClientId   ID of the client which is requesting the resource information.
@param aTargetClientId ID of the client. The information of all the resources on
                       which it has requirement on resource level change is requested.
                       Client ID starts from 1, so calling this API with client ID 0 will
                       fill the details of all the controllable resource registered with
                       resource manager.
@param aNumResources   Number of resource whose information needs to be filled in aInfo i.e,
                       it specifies the size of aInfo array.
@param aInfo           A pointer to an array of descriptor containing an information structure
                       (TPowerResourceInfoV01) to be filled in with the information
                       on the resources. It will be assumed that array allocated will be equal
                       to the number passed in aNumResources. The client must create the array
                       in Kernel stack or heap.

@return KErrNone if the operation was successful
        KErrAccessDenied if client ID (aClientId) could not be found in the registered
                         client list or if the client was registered to be thread relative
						 and this API is not called from the same thread. 
        KErrNotFound if client ID (aTargetClientId) could not be found in the current list
                     of registered client and is also not 0.
        KErrArgument if aNumResources is 0 or aInfo is NULL or if size of aInfo is not sufficient
				     to hold the resource information of number of resources specified in aNumResources.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
TInt DPowerResourceController::GetInfoOnResourcesInUseByClient(TUint aClientId, TUint aTargetClientId, 
															    TUint& aNumResources, TAny* anInfo)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::GetInfoOnResourcesInUseByClient"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("CallingClientId = 0x%x, TargetClientId = 0x%x, NumResources = %d", \
		                                                            aClientId, aTargetClientId, aNumResources));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	if(!anInfo || !aNumResources)
		return KErrArgument;
	SPowerResourceClient* pC = NULL;
	Lock();
	VALIDATE_CLIENT(thread);
	DStaticPowerResource* pR = NULL;
    TDes8 *pInfo = (TDes8*)anInfo;
    if((TUint)(pInfo->MaxLength() - pInfo->Length()) < (sizeof(TPowerResourceInfoV01) * aNumResources))
         UNLOCK_RETURN(KErrArgument);
    TPowerResourceInfoBuf01 buf;

	TInt count = 0;
	TInt r = KErrNone;
	//Special case, if aTargetClientId is 0 fill with all the resource
	if(!aTargetClientId)
		{
		TInt numResources = aNumResources;
#ifndef PRM_ENABLE_EXTENDED_VERSION
		aNumResources = iStaticResourceCount;
#else
		aNumResources = iStaticResourceCount + iDynamicResourceCount + iStaticResDependencyArray.Count() + 
			                                                           iDynamicResDependencyCount;
#endif
		UnLock();
		while(count < iStaticResourceArray.Count())
			{
			if(numResources == 0)
				return KErrNone;
			pR = iStaticResourceArray[count++];
			if(!pR)
				continue;
            r = pR->GetInfo((TDes8*)buf.Ptr());
			if(r != KErrNone)
				return r;
            //Update Resource Id.
            ((TPowerResourceInfoV01*)buf.Ptr())->iResourceId = count;
			pInfo->Append(buf);
			numResources--;
			}	
#ifdef PRM_ENABLE_EXTENDED_VERSION
		count = 0;
		while(count < iStaticResDependencyArray.Count())
			{
			if(count >= numResources)
				return KErrNone;
			pR = iStaticResDependencyArray[count++];
			r = pR->GetInfo((TDes8*)buf.Ptr());
			//Update Resource Id.
			((TPowerResourceInfoV01*)buf.Ptr())->iResourceId = ((DStaticPowerResourceD*)pR)->iResourceId;
			if(r != KErrNone)
				return r;
			pInfo->Append(buf);
			}
		numResources -= iStaticResDependencyArray.Count();
		if((!numResources) || (!iDynamicResourceCount && !iDynamicResDependencyCount))
			return r;
		Lock();
		TInt resCount = 0;
		for(count = 0; count < iDynamicResourceList.Allocd(); count++)
			{
			pR = iDynamicResourceList[count];
			if(!pR)
				continue;
			if((resCount >= iDynamicResourceCount) || (resCount >= numResources))
				UNLOCK_RETURN(KErrNone);
			r = pR->GetInfo((TDes8*)buf.Ptr());  
			if(r != KErrNone)
				UNLOCK_RETURN(r);
			((TPowerResourceInfoV01*)buf.Ptr())->iResourceId = ((DDynamicPowerResource*)pR)->iResourceId;
			pInfo->Append(buf);
			resCount++;
			}
		numResources -= resCount;
		resCount = 0;
		for(count = 0; count < (TInt)iDynamicResDependencyList.Allocd(); count++) 
			{
			pR = iDynamicResDependencyList[count];
			if(!pR)
				continue;
			if((resCount >= iDynamicResDependencyCount) || (resCount >= numResources))
				UNLOCK_RETURN(KErrNone);
			r = pR->GetInfo((TDes8*)buf.Ptr());
			if(r != KErrNone)
				UNLOCK_RETURN(r);
			((TPowerResourceInfoV01*)buf.Ptr())->iResourceId = ((DDynamicPowerResourceD*)pR)->iResourceId;
			pInfo->Append(buf);
			resCount++;
			}
		UnLock();
#endif
		return r;
		}
	GET_TARGET_CLIENT();
	SPowerResourceClientLevel* pCL = pC->iLevelList;
	for (count= 0; pCL; count++, pCL = pCL->iNextInList)
		{
		if(count >= (TInt)aNumResources)
			continue;
#ifndef PRM_ENABLE_EXTENDED_VERSION
		pR = iStaticResourceArray[pCL->iResourceId-1];
#else
		GET_RESOURCE_FROM_LIST(pCL->iResourceId, pR);
#endif
		r = pR->GetInfo((TDes8*)buf.Ptr());
        //Update Resource Id.
        ((TPowerResourceInfoV01*)buf.Ptr())->iResourceId = pCL->iResourceId;
		if(r != KErrNone)
			UNLOCK_RETURN(r);
		pInfo->Append(buf);
		}
	aNumResources = count;
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::GetInfoOnResourcesInUseByClient, AcutalNoOfResources = %d", aNumResources));
	UNLOCK_RETURN(KErrNone);
	}

/**
@publishedPartner
@prototype 9.5

Request number of clients which has requirements on the resource level of the specified
resource. Resource ID starts from 1, so 0 can be used to get the number of clients
registered with resource manager.

@param aClientId         ID of the client which is requesting number of clients
                         holding requirement on specified resource.
@param aResourceId       ID of the resource. 
@param aNumClient        This is updated with number of clients having a requirement
                         on resource level if valid resource ID is specified.
                         If resource ID is 0, then it is updated with number of clients
                         registered with PRM.

@return  KErrNone if the operation was successful
         KErrAccessDenied if the client ID could not found in the current list of
                          registered clients or if the client was registered to be thread 
						  relative and this API is not called from the same thread. 
         KErrNotFound     If this resource ID could not be found in the current list
                          of registered resource and is also not 0.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
TInt DPowerResourceController::GetNumClientsUsingResource(TUint aClientId, TUint aResourceId, TUint& aNumClients)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::GetNumClientsUsingResource"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("ClientId = 0x%x, ResourceId = %d", aClientId, aResourceId));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	SPowerResourceClient* pC = NULL;
	Lock();
	VALIDATE_CLIENT(thread);
	if(!aResourceId)
		{
		//Special case return the number of clients registered with resource controller.
		aNumClients = iClientCount + iUserSideClientCount;
		UNLOCK_RETURN(KErrNone);
		}
#ifdef PRM_ENABLE_EXTENDED_VERSION
	DStaticPowerResource* pR = NULL;
	GET_RESOURCE_FROM_LIST(aResourceId, pR) 
#else
	if(aResourceId > (TUint)iStaticResourceArray.Count())
		UNLOCK_RETURN(KErrNotFound);
	DStaticPowerResource* pR = iStaticResourceArray[aResourceId-1];
	if(!pR)
		UNLOCK_RETURN(KErrNotFound);
#endif
	aNumClients = 0;
	for(SDblQueLink*pCL = pR->iClientList.First(); pCL != &pR->iClientList.iA; pCL=pCL->iNext)
	   aNumClients++;
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::GetNumClientsUsingResource, NumClients = %d", aNumClients));
	UNLOCK_RETURN(KErrNone);
	}

/**
@publishedPartner
@prototype 9.5

Request information on clients
If resource ID is valid, aInfo is updated with the information of the clients
which have a requirement on the resource level for the specified resource
If resource ID is 0, aInfo is updated with the information of the clients registered
with resource manager, starting from client ID 1.
The number of clients for which information will be provided will be equal to or less 
than the number specified in aNumClients.
@param aClientId        ID of the client which is requesting the information on
                        the clients which holds requirement on specified
                        resource's level change.
@param aResourceId      Id of the resource.
@param aNumClients		Number of clients whose information needs to be filled in aInfo
						i.e., it specifies the size of aInfo array.
@param aInfo            A pointer to an array of descriptor containing an information
                        structure (TPowerClientInfoV01) to be filled in with
                        the information on the client. It will be assumed that array
                        allocated will be equal to the number passed in aNumClients.
                        The Client must create the array of descriptors in kernel stack
                        or heap.

@return KErrNone if the operation was successful.
        KErrNotFound if resource ID could not be found in the registered resource list and is not 0.
		KErrAccessDenied if client ID (aClientId) could not be found in the registered client
						 list or if the client was registered to be thread relative and this API is not
						 called from the same thread.
        KErrArgument if aNumClients is 0 or aInfo is NULL or if size of aInfo is not sufficient to hold
					 client information of specified client number in aNumClients.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
TInt DPowerResourceController::GetInfoOnClientsUsingResource(TUint aClientId, TUint aResourceId, 
															  TUint& aNumClients, TAny* anInfo)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::GetInfoOnClientsUsingResource"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("ClientId = 0x%x, ResourceId = %d, NumClients = %d", \
		                                                      aClientId, aResourceId, aNumClients));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	if(!anInfo || !aNumClients)
		return KErrArgument;
	SPowerResourceClient* pC = NULL;
	Lock();
	VALIDATE_CLIENT(thread);
    TDes8 *pInfo = (TDes8*)anInfo;
    if((TUint)(pInfo->MaxLength() - pInfo->Length()) < (sizeof(TPowerClientInfoV01) * aNumClients))
        UNLOCK_RETURN(KErrArgument);
    TPowerClientInfoV01 info;
	if(aResourceId == 0)
		{
        TUint16 count = 0, resCount = 0;
        for(count = 0; count < iClientList.Allocd(); count++)
		    {
            if((resCount >= iClientCount) || (resCount >= aNumClients))
				break;
            pC = iClientList[count];
            if(!pC)
				continue;
            resCount++;
            info.iClientId = pC->iClientId;
            info.iClientName = (TDesC8*)pC->iName;
            pInfo->Append(TPckgC<TPowerClientInfoV01>(info));
	        }
        aNumClients -= resCount;
        resCount = 0;
        for(count = 0; count < iUserSideClientList.Allocd(); count++)
	        {
            if((resCount >= iUserSideClientCount) || (resCount >= aNumClients))
				break;
            pC = iUserSideClientList[count];
            if(!pC)
				continue;
            resCount++;
            info.iClientId = pC->iClientId;
            info.iClientName = (TDesC8*)pC->iName;
            pInfo->Append(TPckgC<TPowerClientInfoV01>(info));
			}
		aNumClients = iClientCount + iUserSideClientCount;
		UNLOCK_RETURN(KErrNone);
		}
#ifdef PRM_ENABLE_EXTENDED_VERSION
	DStaticPowerResource* pR = NULL;
	GET_RESOURCE_FROM_LIST(aResourceId, pR) 
#else
	if(aResourceId > (TUint)iStaticResourceArray.Count())
		UNLOCK_RETURN(KErrNotFound);
	DStaticPowerResource* pR = iStaticResourceArray[aResourceId-1];
	if(!pR)
		UNLOCK_RETURN(KErrNotFound);
#endif
	SPowerResourceClientLevel* pCL = NULL;
    TUint c = 0;
	for(SDblQueLink* pRC = pR->iClientList.First(); pRC != &pR->iClientList.iA; pRC = pRC->iNext, c++)
		{
		if(c >= aNumClients)
			continue;
		pCL = (SPowerResourceClientLevel*)pRC;
		if(pCL->iClientId & USER_SIDE_CLIENT_BIT_MASK)
			pC = iUserSideClientList[(pCL->iClientId & ID_INDEX_BIT_MASK)];
		else
			pC = iClientList[(pCL->iClientId & ID_INDEX_BIT_MASK)];
		info.iClientId = pC->iClientId;
		info.iClientName =  (TDesC8*)pC->iName;
        pInfo->Append(TPckgC<TPowerClientInfoV01>(info));
		}
	aNumClients = c;
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::GetInfoOnClientsUsingResource, NumClients = %d", aNumClients));
	UNLOCK_RETURN(KErrNone);
	}

/**
@publishedPartner
@prototype 9.5

Request changing the state of a resource
NOTE: If a resource callback is specified for instantaneous resource, then callback
      will be called after resource change and will be executed in the context of the
      client thread.
      If a resource callback is specified for long latency resources, then it will be
      executed asynchronously.When the request is accepted the API returns immediately
	  and the calling thread is unblocked: the callback (called in the client's context) 
	  will be invoked when the resource change finally takes place.
      If aCb is not specified (NULL by default) the API executes synchronously and will
      only return when the resource change has taken place for long latency resource.
      The client thread is blocked throughout
      When state change for a shared resource is requested, only minimum state that
      satisfy the requirement is guaranteed and it is not guaranteed for the absolute
      value change.

@param aClientId   ID of the client which is requesting the resource change.
@param aResourceId ID of the resource whose state is to be changed.
@param aNewState   The new state of the resource. This could be a binary value for a
                   binary resource, an integer level for a multilevel resource or some
                   platform specific token for a multi-property resource.
@param aCb         For Long latency resource
                       A pointer to a resource callback object which encapsulates a
                       callback function to be called whenever the resource state change
                       happens (if left NULL the API will execute synchronously).
                   For Instantaneous resource
                       A pointer to a resource callback object which encapsulates a callback
                       function to be called after resource change. This executes in the
                       context of the client thread.

@return KErrNone   If the API is to be executed synchronously it indicates the change was
                   successful, if the API is to be executed asynchronously it indicates
                   the request to change the resource state has been accepted.
        KErrNotFound if the resource ID could not be found in the current list of
                     controllable resources.
        KErrAccessDenied if the client ID could not be found in the list of
                         registered clients or if the client was registered to be thread 
						 relative and this API is not called from the same thread or if the
						 resource is single user resource and another client is already holding 
						 the resource.
        KErrNotReady if the request is issued before the resource controller completes its
                     internal initialisation.
        KErrUnderflow if the client has exceeded the reserved number of
                      SPowerResourceClientLevel and the free pool is empty or if it is
                      an asynchronous operation on a long latency resource and the client has 
					  exceeded the reserved number of TPowerRequest and the free pool is empty.
		KErrArgument if requested level is out of range (outside of min and max levels).
		KErrCorrupt  if internal data structure is corrupted.
		KErrPermissionDenied if the requested state of the resource is not accepted by its dependents. 
							 This error is valid only for dependent resource state change in extended version
							 of PRM.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
@pre Do not call synchronous version from DFC thread 0 for long latency resource 
*/
TInt DPowerResourceController::ChangeResourceState(TUint aClientId, TUint aResourceId, TInt aNewState, 
												                                TPowerResourceCb* aCb)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::ChangeResourceState"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("ClientId = 0x%x, ResourceId = %d, NewState = %d", aClientId, \
		                                                                          aResourceId, aNewState));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	if(iInitialised <= EResConCreated)
		return KErrNotReady;
	if(!aResourceId)
		return KErrNotFound;
	SPowerResourceClient* pC = NULL;
	TInt r = KErrNone;
	Lock();
	VALIDATE_CLIENT(thread);
#ifdef PRM_ENABLE_EXTENDED_VERSION
	DStaticPowerResource *pR = NULL;
	GET_RESOURCE_FROM_LIST(aResourceId, pR) 
#else
	if(aResourceId > (TUint)iStaticResourceArray.Count())
		UNLOCK_RETURN(KErrNotFound);
	DStaticPowerResource* pR = iStaticResourceArray[aResourceId-1];
	if(!pR)
		UNLOCK_RETURN(KErrNotFound);
#endif
	//Return if the resource is already in that state and client is also the same.
	if((aNewState == pR->iCachedLevel) && ((TInt)aClientId == pR->iLevelOwnerId))
		{
		if(aCb)
			{
			//Invoke callback function
            TUint ClientId = aClientId;
            TUint ResourceId = aResourceId;
            TInt Level = aNewState;
            TInt LevelOwnerId = pR->iLevelOwnerId;
            TInt Result = KErrNone;
            TAny* Param = aCb->iParam;
            aCb->iPendingRequestCount++;
            UnLock();
            // Call the client specified callback function
            aCb->iCallback(ClientId, ResourceId, Level, LevelOwnerId, Result, Param);   
            Lock();
            aCb->iPendingRequestCount--;
            if(aCb->iPendingRequestCount == 0)
                {
                aCb->iResult = KErrCompletion;
                }
			}
        UNLOCK_RETURN(KErrNone);
		}
	
	PRM_CLIENT_CHANGE_STATE_START_TRACE
	//If long latency resource requested synchronously from DFC thread 0 Panic

    const TDesC8* pDfc0 = &KDfcThread0Name;
	if((pR->LatencySet() && !aCb) && !(pDfc0->Compare(*(TDesC8*)thread.iName)))
		{
		UnLock();
		Panic(ECalledFromDfcThread0);
		}
	if(!pR->Usage() && !pR->iClientList.IsEmpty())
		{
		SPowerResourceClientLevel* pCL = (SPowerResourceClientLevel*)pR->iClientList.First();
		if((pCL != NULL) && (pCL->iClientId != pC->iClientId))
			{
			r = KErrAccessDenied;
			PRM_CLIENT_CHANGE_STATE_END_TRACE
			UNLOCK_RETURN(r);
			}
		}
#ifdef PRM_ENABLE_EXTENDED_VERSION
	if(aResourceId & KIdMaskDynamic)
		{
		//Resource in the process of deregistration
		if(((DDynamicPowerResource*)pR)->LockCount() == 0) 
			{
			r = KErrNotFound;
			PRM_CLIENT_CHANGE_STATE_END_TRACE
			UNLOCK_RETURN(r);
			}
		}
#endif
    //Validate requested level
    TPowerResourceInfoBuf01 buf;
    r = pR->GetInfo((TDes8*)buf.Ptr());
    if(r != KErrNone)
		{
		PRM_CLIENT_CHANGE_STATE_END_TRACE
		UNLOCK_RETURN(r);
		}
    TPowerResourceInfoV01 *pBuf = (TPowerResourceInfoV01*)buf.Ptr();
    if(((pBuf->iMinLevel > pBuf->iMaxLevel) && ((aNewState > pBuf->iMinLevel) || (aNewState < pBuf->iMaxLevel))) 
		    || ((pBuf->iMaxLevel > pBuf->iMinLevel) && ((aNewState > pBuf->iMaxLevel) || (aNewState < pBuf->iMinLevel))))
		{
        r = KErrArgument;
        PRM_CLIENT_CHANGE_STATE_END_TRACE
        UNLOCK_RETURN(r);
		}

	TPowerRequest* req;
	SPowerRequest* pS=NULL;
	if(pR->LatencySet() && aCb)
		{
		// Get request object from free pool, as it is long latency resource as client
		// will be unblocked once message is sent to controller, so can't use thread message.
		if(pC->iReservedRm ==0 && !iRequestPoolCount)
			{
            r = KErrUnderflow;
            PRM_CLIENT_CHANGE_STATE_END_TRACE
			UNLOCK_RETURN(r);
			}

		LIST_POP(iRequestPool, pS, iNext);
		if(!pS)
			UNLOCK_RETURN(KErrCorrupt); //This should not happen
		if(pC->iReservedRm==0)
			{
			iRequestPoolCount--;
			pC->iUnderFlowRmCount++;
			}
		else
		    pC->iReservedRm--;
		req=&pS->iRequest;
		pC->iPendingReqCount++;
		}
	else
        req=(TPowerRequest*)&TPowerRequest::Get();
#ifdef PRM_ENABLE_EXTENDED_VERSION
	if(aResourceId & KIdMaskDynamic)
		((DDynamicPowerResource*)pR)->Lock();
#endif
	req->Level() = aNewState;
	req->ResourceId() = aResourceId;
	req->ClientId() = aClientId;
	req->ReqType() = TPowerRequest::EChange;
	req->Resource() = pR;
	if(aCb)
		{
		aCb->iResult = KErrNone;
		aCb->iResourceId = aResourceId;
		aCb->iClientId = aClientId;
		}
	req->ResourceCb() = aCb;
	if(pR->LatencySet())
		{
		UnLock();
		if(aCb)
			{
#ifdef PRM_ENABLE_EXTENDED_VERSION
			if (aCb->iResourceId & KIdMaskResourceWithDependencies) //Dependency resource
				{
				req->Send(iMsgQDependency); // Send the request to DFC thread.
				return KErrNone;
				}
			else
#endif
				{
				req->Send(iMsgQ); // Send the request to Resource Controller thread.
				return KErrNone;
				}
			}
#ifdef PRM_ENABLE_EXTENDED_VERSION
		if(aResourceId & KIdMaskResourceWithDependencies) //Dependency resource
			{
			r = req->SendReceive(iMsgQDependency); // Send the request to DFC thread.
			}
#endif
		else
			{
			r = req->SendReceive(iMsgQ); // Block till the controller completes with the request.
			}
#ifdef PRM_ENABLE_EXTENDED_VERSION
		Lock();
		if(aResourceId & KIdMaskDynamic)
			((DDynamicPowerResource*)pR)->UnLock();
		UnLock();
#endif
		return r;
		}
	if(pR->Usage())
		{
		r = CheckLevelAndAddClient(pC, req);
		if((r != KErrNone)|| !req->RequiresChange())
			{
			req->Level() = pR->iCachedLevel;
#ifdef PRM_ENABLE_EXTENDED_VERSION
		    if(aResourceId & KIdMaskDynamic)
				((DDynamicPowerResource*)pR)->UnLock();
#endif
			if(aCb)
				{
                //Invoke callback function
                TUint ClientId = req->ClientId();
                TUint ResourceId = aResourceId;
                TInt Level = req->Level();
                TInt LevelOwnerId = pR->iLevelOwnerId;
                TInt Result = r;
                TAny* Param = aCb->iParam;
                aCb->iPendingRequestCount++;
                UnLock();
                // Call the client specified callback function
                aCb->iCallback(ClientId, ResourceId, Level, LevelOwnerId, Result, Param);   
                Lock();
                aCb->iPendingRequestCount--;
                if(aCb->iPendingRequestCount == 0)
                    {
                    aCb->iResult = KErrCompletion;
                    }
                }

			PRM_CLIENT_CHANGE_STATE_END_TRACE
			UNLOCK_RETURN(r);
			}
		}
	else if(pR->iLevelOwnerId == -1)
		{
		/* Add client Level */
		if(pC->iReservedCl<=0 && !iClientLevelPoolCount)
			{
			r = KErrUnderflow;
			PRM_CLIENT_CHANGE_STATE_END_TRACE
#ifdef PRM_ENABLE_EXTENDED_VERSION
			if(aResourceId & KIdMaskDynamic)
				((DDynamicPowerResource*)pR)->UnLock();
#endif
			UnLock();
     		return(r);
			}
		SPowerResourceClientLevel* pSCL=NULL;
		LIST_POP(iClientLevelPool, pSCL, iNextInList);
		pSCL->iClientId=aClientId;
		pSCL->iResourceId=aResourceId;
		pSCL->iLevel=aNewState;
		LIST_PUSH(pC->iLevelList, pSCL, iNextInList);
		pR->iClientList.Add(pSCL);
		if(pC->iReservedCl==0)
			{
			iClientLevelPoolCount--;
			pC->iUnderFlowClCount++;
			}
		else
		     pC->iReservedCl--;
		}
	else
		{
		//Update the level in the client list.
		SPowerResourceClientLevel* pSCL = (SPowerResourceClientLevel*)pR->iClientList.First();
		pSCL->iLevel = aNewState;
		}
	UnLock();
	r = pR->DoRequest(*req);
	Lock();
	if(r==KErrNone)
		{
		//Notification to clients
		CompleteNotifications(req->ClientId(), pR, req->Level(), r, aClientId, EFalse);
		//Cache the state
		pR->iCachedLevel=req->Level();
		pR->iLevelOwnerId=req->ClientId();
		//Update resource details for Idle
		if(pR->iIdleListEntry)
			{
            pR->iIdleListEntry->iLevelOwnerId=req->ClientId();
			pR->iIdleListEntry->iCurrentLevel=req->Level();
			}
		}
#ifdef PRM_ENABLE_EXTENDED_VERSION
	if(aResourceId & KIdMaskDynamic)
		((DDynamicPowerResource*)pR)->UnLock();
#endif
	if(aCb)
		{
        //Invoke callback function
        TUint ClientId = req->ClientId();
        TUint ResourceId = aResourceId;
        TInt Level = req->Level();
        TInt LevelOwnerId = pR->iLevelOwnerId;
        TInt Result = r;
        TAny* Param = aCb->iParam;
        aCb->iPendingRequestCount++;
        UnLock();
        // Call the client specified callback function
        aCb->iCallback(ClientId, ResourceId, Level, LevelOwnerId, Result, Param);   
        Lock();
        aCb->iPendingRequestCount--;
        if(aCb->iPendingRequestCount == 0)
            {
            aCb->iResult = KErrCompletion;
            }
		}
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::ChangeResourceState, Level = %d", req->Level()));
    PRM_CLIENT_CHANGE_STATE_END_TRACE
    UNLOCK_RETURN(r);
	}

/**
@publishedPartner
@prototype 9.5

Request the state of the resource synchronously

@param aClientId  ID of the client which is requesting the resource state.
@param aResourceId ID of the resource whose state is being requested.
@param aCached     If ETrue, cached value will be updated in aState.
                   If EFalse, aState will be updated after the resource
                   state is read from resource.
@param aState      Returns the resource state if operation was successful. This
                   could be a binary value for a binary resource, an integer level
                   for a multilevel resource or some platform specific token for a
                   multi-property resource.
@param aLevelOwnerId Returns the Id of the client that is currently holding the resource.
					 -1	is returned when no client is holding the resource.

@return KErrNone   if operation was successful
        KErrAccessDenied if the client ID could not be found in the current list
                         of registered clients or if the client was registered to be thread
						 relative and this API is not called from the same thread. 
        KErrNotFound if this resource ID could not be found in the current list
                     of controllable resources.
        KErrNotReady if the request is issued before the resource controller completes
                     its internal initialization.


@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
@pre Do not call from DFC thread 0 for long latency resource with caching disabled.
*/
TInt DPowerResourceController::GetResourceState(TUint aClientId, TUint aResourceId, TBool aCached, TInt& aState, 
																						TInt& aLevelOwnerId)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::GetResourceState(synchronous)"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("ClientId = 0x%x, ResourceId = %d, Cached = %d", aClientId, aResourceId, aCached));

	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	if(iInitialised <= EResConCreated) 
		return KErrNotReady;
	SPowerResourceClient* pC = NULL;
	TInt r = KErrNone;
	Lock();
	VALIDATE_CLIENT(thread);
	if(!aResourceId)
		UNLOCK_RETURN(KErrNotFound);
#ifdef PRM_ENABLE_EXTENDED_VERSION
	DStaticPowerResource *pR = NULL;
	GET_RESOURCE_FROM_LIST(aResourceId, pR) 
	if(aResourceId & KIdMaskDynamic)
		{
		if(((DDynamicPowerResource*)pR)->LockCount() == 0)
			UNLOCK_RETURN(KErrNotFound);
		}
#else
	if(aResourceId > (TUint)iStaticResourceArray.Count())
		UNLOCK_RETURN(KErrNotFound);
	DStaticPowerResource *pR = iStaticResourceArray[aResourceId-1];
	if(!pR)
		UNLOCK_RETURN(KErrNotFound);
#endif
	PRM_RESOURCE_GET_STATE_START_TRACE
	//Panic if long latency resource called to execute synchronously from DFC thread0
	const TDesC8* pDfc0 = &KDfcThread0Name;
	if((!aCached && pR->LatencyGet()) && !(pDfc0->Compare(*(TDesC*)thread.iName)))
		{
		UnLock();
		Panic(ECalledFromDfcThread0);
		}
	if(aCached)
		{
		//Return the cached value.
		aState = pR->iCachedLevel;
		aLevelOwnerId = pR->iLevelOwnerId;
        PRM_RESOURCE_GET_STATE_END_TRACE
    	UNLOCK_RETURN(KErrNone);
		}
#ifdef PRM_ENABLE_EXTENDED_VERSION
	if(aResourceId & KIdMaskDynamic)
		((DDynamicPowerResource*)pR)->Lock();
#endif
	//Call from thread Id.
	TPowerRequest* req = (TPowerRequest*)&TPowerRequest::Get();
	req->ResourceId() = aResourceId;
	req->ReqType() = TPowerRequest::EGet;
	req->ClientId() = aClientId;
	req->Resource() = pR;
	req->ResourceCb() = NULL;
	if(pR->LatencyGet())
		{
#ifdef PRM_ENABLE_EXTENDED_VERSION
		if(	req->ResourceId() & KIdMaskResourceWithDependencies ) // Dependency Resource
			{
			UnLock();
			req->SendReceive(iMsgQDependency); //Send the request to DFC Thread
			Lock();
			}
		else  // Plain resource
#endif
			{
			UnLock();
			req->SendReceive(iMsgQ);
			Lock();
			}
		}
	else
		{
		UnLock();
		r = pR->DoRequest(*req); // Call PSL to get the state of resource.
		Lock();
		if(r==KErrNone)
			{
			//Update the cache value and cache for idle thread usage if requested for this resource.
			pR->iCachedLevel=req->Level();
			if(pR->iIdleListEntry)
				{
				SIdleResourceInfo* pI=pR->iIdleListEntry;
				pI->iCurrentLevel=req->Level();
				}
			}
		}
	aState = req->Level();
	aLevelOwnerId = pR->iLevelOwnerId;
#ifdef PRM_ENABLE_EXTENDED_VERSION
	if(aResourceId & KIdMaskDynamic)
		((DDynamicPowerResource*)pR)->UnLock();
#endif
	UnLock();
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::GetResourceState(synchronous), Level = %d", aState));
	if(pR->LatencyGet()) //For long latency resource btrace is done in controller thread.
		return r;
	PRM_RESOURCE_GET_STATE_END_TRACE
	return r;
	}

/**   
@publishedPartner
@prototype 9.5

Request the state of the resource asynchronously for long latency resource and
synchronously for instantaneous resource

@param aClientId  ID of the client which is requesting the resource state.
@param aResourceId ID of the resource whose state is being requested.
@param aCached If ETrue, cached value will be updated in aState
               If EFalse, will be updated after the resource state is read from resource
@param aCb     For long latency resource:
               A pointer to a resource callback object which encapsulates a callback function
               to be called whenever the state of the resource is available for the long
               latency resource (executes in the context of resource manager)
               For instantaneous resource:
               A pointer to a resource callback object which encapsulates a callback
               function to be called after the resource state is read. This is executed
               synchronously in the context of the calling thread.
               NOTE: The client must create the callback object in kernel heap or
               data section.

@return KErrNone if the operation was successful
		KErrArgument if callback object is NULL
        KErrAccessDenied if the client ID could not be found in the current list
                         of registered clients or if the client was registered to be thread
						 relative and this API is not called from the same thread.
        KErrNotFound if this resource ID could not be found in the current list
                     of controllable resources.
        KErrNotReady if the request is issued before the resource controller completes
                     its internal initialisation
        KErrUnderflow if the client has exceeded the reserved number of TPowerRequest
                      and the TPowerRequest free pool is empty for long latency resource.
		KErrCorrupt if internal data structure is corrupt.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
TInt DPowerResourceController::GetResourceState(TUint aClientId, TUint aResourceId, TBool aCached,  TPowerResourceCb& aCb)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::GetResourceState(asynchronous)"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("ClientId = 0x%x, ResourceId = %d, Cached = %d", aClientId, aResourceId, aCached));

	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	if(iInitialised <= EResConCreated) 
		return KErrNotReady;
	SPowerResourceClient* pC = NULL;
	TInt r = KErrNone;
	Lock();
	VALIDATE_CLIENT(thread);
	if(!aResourceId)
		UNLOCK_RETURN(KErrNotFound);
#ifdef PRM_ENABLE_EXTENDED_VERSION
	DStaticPowerResource *pR = NULL;
	GET_RESOURCE_FROM_LIST(aResourceId, pR) 
	if(aResourceId & KIdMaskDynamic)
		{
		//Dynamic resource in process of deregistration
		if(((DDynamicPowerResource*)pR)->LockCount() == 0)
			UNLOCK_RETURN(KErrNotFound);
		}
#else
	if(aResourceId > (TUint)iStaticResourceArray.Count())
		UNLOCK_RETURN(KErrNotFound);
	DStaticPowerResource *pR = iStaticResourceArray[aResourceId-1];
	if(!pR)
		UNLOCK_RETURN(KErrNotFound);
#endif
	aCb.iResult = KErrNone;
	aCb.iResourceId = aResourceId;
	aCb.iClientId = aClientId;

	PRM_RESOURCE_GET_STATE_START_TRACE
	if(aCached) //Call the callback directly
		{
        //Invoke callback function
        TUint ClientId = aClientId;
        TUint ResourceId = aResourceId;
        TInt Level = pR->iCachedLevel;
        TInt LevelOwnerId = pR->iLevelOwnerId;
        TInt Result = KErrNone;
        TAny* Param = aCb.iParam;
        aCb.iPendingRequestCount++;
        UnLock();
        // Call the client specified callback function
        aCb.iCallback(ClientId, ResourceId, Level, LevelOwnerId, Result, Param);   
        Lock();
        aCb.iPendingRequestCount--;
        if(aCb.iPendingRequestCount == 0)
            {
            aCb.iResult = KErrCompletion; //Mark the callback object to act properly during cancellation of this request.
            }
#ifdef PRM_INSTRUMENTATION_MACRO
		TInt aState = pR->iCachedLevel;
        PRM_RESOURCE_GET_STATE_END_TRACE
#endif
		UNLOCK_RETURN(KErrNone);
		}
	TPowerRequest* req=NULL;
	if(pR->LatencyGet())
		{
		//Check the client quota of requests
		if(pC->iReservedRm==0 && !iRequestPoolCount)
			UNLOCK_RETURN(KErrUnderflow);
		if(pC->iReservedRm ==0)
			{
			iRequestPoolCount--;
			pC->iUnderFlowRmCount++;
			}
		else
		    pC->iReservedRm--;
		//Get the request from pool
		SPowerRequest* pS;
		LIST_POP(iRequestPool, pS, iNext);
		if(!pS)
			UNLOCK_RETURN(KErrCorrupt); //This should not be called
		req = &pS->iRequest;
		//Increment pending request count of the client
		pC->iPendingReqCount++;
		}
	else
		//Asynchronous instantaneous resource execute in the context of client thread.
		req = (TPowerRequest*)&TPowerRequest::Get();
#ifdef PRM_ENABLE_EXTENDED_VERSION
	if(aResourceId & KIdMaskDynamic)
		((DDynamicPowerResource*)pR)->Lock();
#endif
	UnLock();
	req->ReqType() = TPowerRequest::EGet;
	req->ResourceId() = aResourceId;
	req->ClientId() = aClientId;
	req->Resource() = pR;
	req->ResourceCb() = &aCb;
	if(pR->LatencyGet())
		{
#ifdef PRM_ENABLE_EXTENDED_VERSION
		if(	req->ResourceId() & KIdMaskResourceWithDependencies ) // Dependency Resource
			{
			req->Send(iMsgQDependency); // Send the request to DFC thread.
			}
		else  // Plain resource
#endif
			{
			req->Send(iMsgQ);
			}
		}
	else
		{
		r = pR->DoRequest(*req);
		Lock();
		if(r == KErrNone)
			{
			//Update the cache value and cache for idle thread usage if requested for this resource.
			pR->iCachedLevel = req->Level();
			if(pR->iIdleListEntry)
				pR->iIdleListEntry->iCurrentLevel=req->Level();
			}
#ifdef PRM_ENABLE_EXTENDED_VERSION
		if(aResourceId & KIdMaskDynamic)
			((DDynamicPowerResource*)pR)->UnLock();
#endif

        //Invoke callback function
        TUint ClientId = aClientId;
        TUint ResourceId = aResourceId;
        TInt Level = req->Level();
        TInt LevelOwnerId = pR->iLevelOwnerId;
        TInt Result = r;
        TAny* Param = aCb.iParam;
        aCb.iPendingRequestCount++;
        UnLock();
        // Call the client specified callback function
        aCb.iCallback(ClientId, ResourceId, Level, LevelOwnerId, Result, Param);   
        Lock();
        aCb.iPendingRequestCount--;
        if(aCb.iPendingRequestCount == 0)
            {
            aCb.iResult = KErrCompletion; //Mark the callback object to act properly during cancellation of this request.
            }       
        UnLock(); 
		}
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::GetResourceState(asynchronous), Level = %d", req->Level()));
	if(pR->LatencyGet())
		return r;
#ifdef PRM_INSTRUMENTATION_MACRO
	TInt aState = req->Level();
	PRM_RESOURCE_GET_STATE_END_TRACE
#endif
	return r;
	}


/**
@publishedPartner
@prototype 9.5

Cancel an asynchronous request(or its callback).

@param aClientId       ID of the client which is requesting the cancellation of the request.
@param aResourceId     ID for the resource which the request that is being canceled operates
                       upon.
@param aCb             A reference to the resource callback object specified with the request
                       that is being canceled.

@return KErrCancel if the request was canceled.
        KErrNotFound if this resource ID could not be found in the current list of controllable
                     resources.
        KErrCompletion if request is no longer pending.
        KErrAccessDenied if the client ID could not be found in the current list of registered
		clients or if the client was registered to be thread relative and this API is not called
		from the same thread or if client is not the same that requested the resource state change.
		KErrInUse if the request cannot be canceled as processing of the request already started 
		and will run to completion. 

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
TInt DPowerResourceController::CancelAsyncRequestCallBack(TUint aClientId, TUint aResourceId, TPowerResourceCb& aCb)
	{
    TInt r = KErrInUse;
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::CancelAsyncRequestCallBack"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("ClientId = 0x%x, ResourceId = %d", aClientId, aResourceId));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	SPowerResourceClient* pC = NULL;
	Lock();
	VALIDATE_CLIENT(thread);
	if((!aResourceId) || (aCb.iResourceId != aResourceId))
		UNLOCK_RETURN(KErrNotFound);
#ifdef PRM_INSTRUMENTATION_MACRO
#ifdef PRM_ENABLE_EXTENDED_VERSION
    DStaticPowerResource *pR = NULL;
	GET_RESOURCE_FROM_LIST(aResourceId, pR)
#else
	DStaticPowerResource *pR = iStaticResourceArray[aResourceId-1];
	if(!pR)
		UNLOCK_RETURN(KErrNotFound);
#endif
#endif
	if(aCb.iClientId != aClientId)
		{
        __KTRACE_OPT(KRESMANAGER, Kern::Printf("aCb.iClientId = 0x%x, aClientId = 0x%x", aCb.iClientId, aClientId));
        r = KErrAccessDenied;
#ifdef PRM_INSTRUMENTATION_MACRO
        PRM_RESOURCE_CANCEL_LONGLATENCY_OPERATION_TRACE
#endif
        UNLOCK_RETURN(r);
		}
	if(aCb.iResult == KErrCompletion)
		{
        r = KErrCompletion;
#ifdef PRM_INSTRUMENTATION_MACRO
        PRM_RESOURCE_CANCEL_LONGLATENCY_OPERATION_TRACE
#endif
		UNLOCK_RETURN(r);
		}
	//Search in the controller message queue for this message
#ifdef PRM_ENABLE_EXTENDED_VERSION
	if(aResourceId & KIdMaskResourceWithDependencies) //long latency resource with dependency and will be processed in dependency thread.
		{
		//Search in the controller message queue for this message
		for(SDblQueLink* pRM = iMsgQDependency->iQ.First(); pRM != &(iMsgQDependency->iQ.iA); pRM = pRM->iNext)
			{
			TMessageBase* pMsgQ = (TMessageBase*)pRM;
			TPowerRequest* pReq=(TPowerRequest*)pMsgQ;
			if(pReq->ResourceCb() == &aCb)
				{
				r = KErrCancel;
				pRM->Deque();
				pMsgQ->iState = TMessageBase::EFree; //Reset the state
				MoveRequestToFreePool(pReq);
#ifdef PRM_INSTRUMENTATION_MACRO
				PRM_RESOURCE_CANCEL_LONGLATENCY_OPERATION_TRACE
#endif
				UNLOCK_RETURN(r);
				}
			}
		}
	else // long latency resource without dependency and will be processed in RC thread.
#endif
		{
		for(SDblQueLink* pRM = iMsgQ->iQ.First(); pRM != &(iMsgQ->iQ.iA); pRM = pRM->iNext)
			{
			TMessageBase* pMsgQ = (TMessageBase*)pRM;
			TPowerRequest* pReq=(TPowerRequest*)pMsgQ;
			if(pReq->ResourceCb() == &aCb)
				{
				r = KErrCancel;
				pRM->Deque();
				pMsgQ->iState = TMessageBase::EFree; //Reset the state
				MoveRequestToFreePool(pReq);
#ifdef PRM_INSTRUMENTATION_MACRO
				PRM_RESOURCE_CANCEL_LONGLATENCY_OPERATION_TRACE
#endif
				UNLOCK_RETURN(r);
				}
			}
		}
#ifdef PRM_INSTRUMENTATION_MACRO
    PRM_RESOURCE_CANCEL_LONGLATENCY_OPERATION_TRACE
#endif
	UNLOCK_RETURN(r);
	}

/**
@publishedPartner
@prototype 9.5

Request notification of changes to the state of a resource.

@param aClientId     ID of the client which is requesting the notification.
@param aResourceId   ID of the resource for which notification of state changes
                     is being requested.
@param aN            A reference to a notification object which encapsulates a callback
                     function to be called whenever a resource state change takes place.
                     NOTE: The client must create the notification object in kernel heap
                           or data section.

@return KErrNone if the operation of requesting a notification was successful.
        KErrNotFound if this resource ID could not be found in the current list
                     of controllable resources.
        KErrAccessDenied if the client ID could not be found in the current
                         list of registered clients or if the client was registered to be 
						 thread relative and this API is not called from the same thread. 
		KErrInUse if the passed notification object is used already.
NOTE: This API should return immediately; however the notification will
only happen when a resource change occurs.Notification request is idempotent, 
if the same notification has already been requested for this resource ID, 
the API returns with no further action.Notifications remain queued until they are canceled.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
TInt DPowerResourceController::RequestNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN)
	{
    TInt r = KErrNone;    
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::RequestNotification(unconditional)"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("ClientId = 0x%x, ResourceId = %d", aClientId, aResourceId));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	SPowerResourceClient* pC = NULL;
	Lock();
	VALIDATE_CLIENT(thread);
	if((!aResourceId))
		{
        r = KErrNotFound;
        PRM_POSTNOTIFICATION_REGISTER_TRACE
		UNLOCK_RETURN(r);
		}
#ifdef PRM_ENABLE_EXTENDED_VERSION
	DStaticPowerResource *pR = NULL;
	GET_RESOURCE_FROM_LIST(aResourceId, pR)
#else
	if(aResourceId > (TUint)iStaticResourceArray.Count())
		{
        r = KErrNotFound;
		PRM_POSTNOTIFICATION_REGISTER_TRACE
		UNLOCK_RETURN(r);
		}

	DStaticPowerResource *pR = iStaticResourceArray[aResourceId-1];
	if(!pR)
		{
		r = KErrNotFound;
		PRM_POSTNOTIFICATION_REGISTER_TRACE
		UNLOCK_RETURN(r);
		}
#endif
	if(aN.iRegistered) //Check if the same notification object is used already
		{
        r = KErrInUse;
        PRM_POSTNOTIFICATION_REGISTER_TRACE
		UNLOCK_RETURN(r);
		}
	aN.iRegistered++;
	aN.iType = DPowerResourceNotification::EUnconditional;
	aN.iOwnerId=(TUint16)aClientId;
	aN.iCallback.iClientId= aClientId;
	aN.iCallback.iResourceId=aResourceId;
	//Add to resource notification list
	pR->iNotificationList.Add(&(aN.iNotificationLink));
	//Add to client notification list
	LIST_PUSH(pC->iNotificationList, &aN, iNextInClient);
    PRM_POSTNOTIFICATION_REGISTER_TRACE
	UNLOCK_RETURN(KErrNone);
	}

/**
@publishedPartner
@prototype 9.5

Request notification when the state of a resource reaches a specified threshold or
goes above or below that threshold (for multilevel resource only) based on direction.
In other words it is issued when a threshold on the specified resource state is crossed
in the direction specified.

@param aClientId  ID of the client which is requesting the notification.
@param aResourceId ID for the resource whose notification of state changes is
                   being requested.
@param aN          A reference to a notification object which encapsulates a callback
                   function to be called whenever the conditions to issue the notification
                   (specified in the API) are met.
                   NOTE: The client must create the notification object in kernel heap
                   or data section.
@param aThreshold  The level of the resource state that will trigger the notification
                   when reached.
@param aDirection  Specifies the direction of change of the resource state that will
                   trigger a notification. EFalse means the notification will be issued
                   when the resource state change to a specified threshold value or below
                   the specified threshold, ETrue means the notification will be issued
                   when the resource state change to a specified threshold value or above
                   the specified threshold.



@return KErrNone if the operation of requesting a notification was successful.
        KErrNotFound if this resource ID could not be found in the current list
                     of controllable resources.
        KErrAccessDenied if the client ID could not be found in the list of
                         registered clients or if the client was registered to be thread
						 relative and this API is not called from the same thread. 
		KErrInUse if the passed notification object is used already.
		KErrArgument if the specified threshold is out of range.
NOTE: This API should return immediately; however the notification will only
happen when a resource change occurs. Notification request is idempotent, 
if the same notification has already been requested for this resource ID, 
the API returns with no further action. Notification remain queued until they are canceled.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
TInt DPowerResourceController::RequestNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN, 
																					TInt aThreshold, TBool aDirection)
	{
    TInt r = KErrNone;
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::RequestNotification(conditional)"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("ClientId = 0x%x, ResourceId = %d, Threshold = %d, Direction = %d", \
																aClientId, aResourceId, aThreshold, aDirection));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	Lock();
	SPowerResourceClient* pC = NULL;
	VALIDATE_CLIENT(thread);
	if(!aResourceId)
		{
        r = KErrNotFound;
		PRM_POSTNOTIFICATION_REGISTER_TRACE
		UNLOCK_RETURN(r);
		}
#ifdef PRM_ENABLE_EXTENDED_VERSION
	DStaticPowerResource *pR = NULL;
	GET_RESOURCE_FROM_LIST(aResourceId, pR)
#else
	if(aResourceId > (TUint)iStaticResourceArray.Count())
		{
        r = KErrNotFound;
        PRM_POSTNOTIFICATION_REGISTER_TRACE
		UNLOCK_RETURN(r);
		}

	DStaticPowerResource *pR = iStaticResourceArray[aResourceId-1];
	if(!pR)
		{
		r = KErrNotFound;
		PRM_POSTNOTIFICATION_REGISTER_TRACE
		UNLOCK_RETURN(r);
		}
#endif
	if(aN.iRegistered) //Check if the same notification object is used already
		{
        r = KErrInUse;
        PRM_POSTNOTIFICATION_REGISTER_TRACE
		UNLOCK_RETURN(r);
		}

	//Validate threshold for correctness
    TPowerResourceInfoBuf01 buf;
    r = pR->GetInfo((TDes8*)buf.Ptr());
    if(r != KErrNone)
		{
		PRM_POSTNOTIFICATION_REGISTER_TRACE
		UNLOCK_RETURN(r);
		}
    TPowerResourceInfoV01 *pBuf = (TPowerResourceInfoV01*)buf.Ptr();
    if(((pBuf->iMinLevel > pBuf->iMaxLevel) && ((aThreshold > pBuf->iMinLevel) || (aThreshold < pBuf->iMaxLevel))) || 
		     ((pBuf->iMaxLevel > pBuf->iMinLevel) && ((aThreshold > pBuf->iMaxLevel) || (aThreshold < pBuf->iMinLevel))))
		{
        r = KErrArgument;
        PRM_POSTNOTIFICATION_REGISTER_TRACE
        UNLOCK_RETURN(r);
		}
	aN.iRegistered++;
	aN.iType = DPowerResourceNotification::EConditional;
	aN.iThreshold = aThreshold;
	aN.iDirection = aDirection;
	aN.iOwnerId = (TUint16)aClientId;
	aN.iCallback.iClientId = aClientId;
	aN.iCallback.iResourceId = aResourceId;
	//Store the current level of the resource as will be used for issuing notification
	aN.iPreviousLevel = pR->iCachedLevel; 
	//Add to resource notification list
	pR->iNotificationList.Add(&(aN.iNotificationLink));
	//Add to client notification list
	LIST_PUSH(pC->iNotificationList, &aN, iNextInClient);
	PRM_POSTNOTIFICATION_REGISTER_TRACE
	UNLOCK_RETURN(KErrNone);
	}

/**
@publishedPartner
@prototype 9.5

Cancel and remove from queue a previously issued request for notification on a
resource state change.

@param aClientId ID of the client which is requesting to cancel the notification
@param aResourceId for the resource whose pending notification of state changes
                   is being canceled.
@param aN          A reference to the notification object that was associated with
                   the notification request that is being canceled. This will be
                   used to identify the notification that is being canceled.

@return KErrCancel if the notification request was successfully canceled.
        KErrNotFound if the specified notification object is 
					 not found in the current list of notification objects for the 
					 specified resource.
        KErrAccessDenied if the client requesting the cancellation is not the same
                         which registered the notification or if the resource id does not match or
						 if the client ID could not be found in the list of registered clients or 
						 if the client was registered to be thread relative and this API is 
						 not called from the same thread. 

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
TInt DPowerResourceController::CancelNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN)
	{
     TInt r = KErrCancel;
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::CancelNotification"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("ClientId = 0x%x, ResourceId = %d", aClientId, aResourceId));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	SPowerResourceClient* pC = NULL;
	Lock();
	VALIDATE_CLIENT(thread);

	if(!aN.iRegistered)
		{
        r = KErrNotFound;
        PRM_POSTNOTIFICATION_DEREGISTER_TRACE
		UNLOCK_RETURN(r);
		}
	//Remove from the client list
	TBool found = EFalse;
	DPowerResourceNotification* pCNL = pC->iNotificationList;
	DPowerResourceNotification* pCNLNext = pCNL;
	if(pCNL == &aN)
		{
        if(pCNL->iOwnerId != (TUint16)aClientId)
	        {
            r = KErrAccessDenied;
            PRM_POSTNOTIFICATION_DEREGISTER_TRACE
           UNLOCK_RETURN(r);
		    }
        found = ETrue;
		}
    else
		{
		while(pCNLNext)
			{
			if(pCNLNext == &aN)
				{
				if(pCNL->iOwnerId != (TUint16)aClientId)
					{
					r = KErrAccessDenied;
					PRM_POSTNOTIFICATION_DEREGISTER_TRACE
					UNLOCK_RETURN(r);
					}
				pCNL->iNextInClient = pCNLNext->iNextInClient;
				pCNL = pCNLNext;
				found = ETrue;
				break;
				}
			pCNL = pCNLNext;
			pCNLNext = pCNLNext->iNextInClient;
			}
		}
	if(!found)
		{
        r = KErrNotFound;
        PRM_POSTNOTIFICATION_DEREGISTER_TRACE
		UNLOCK_RETURN(r);
		}
	if(pCNL->iCallback.iResourceId != aResourceId)
		{
		r = KErrAccessDenied;
		PRM_POSTNOTIFICATION_DEREGISTER_TRACE
		UNLOCK_RETURN(r);
		}
	//Remove from resource list
	pCNL->iNotificationLink.Deque();
	pCNL->iCallback.Cancel();
	//Remove from client list
	LIST_REMOVE(pC->iNotificationList, pCNL, iNextInClient, DPowerResourceNotification);
	pCNL->iRegistered--;
	PRM_POSTNOTIFICATION_DEREGISTER_TRACE
	UNLOCK_RETURN(KErrCancel);
	}

/**
@publishedPartner
@prototype 9.5

Request pre-allocation of specified number of client level and request message objects.

@param aClientId  ID of the client which is requesting the pre-allocation.
@param aNumCl     Number of client level objects that needs to be pre-allocated
                  for this client.
@param aNumRm     Number of request message objects that needs to be pre-allocated
                  for this client.

@return KErrNone if the allocation was successful
        KErrAccessDenied if the client ID could not be found in the list of
                         registered clients or if the client was registered to be thread
						 relative and this API is not called from the same thread. 
        KErrNoMemory if there is no sufficient memory for allocation of requested
                     number of objects.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
TInt DPowerResourceController::AllocReserve(TUint aClientId, TUint8 aNumCl, TUint8 aNumRm)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::AllocReserve"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("ClientId = 0x%x, Numclients = %d, NumResource = %d", aClientId, aNumCl, aNumRm));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	SPowerResourceClient* pC = NULL;
	Lock();
	VALIDATE_CLIENT(thread);
	//Call from thread Id.
	TPowerRequest* req = (TPowerRequest*)&TPowerRequest::Get();
	req->ReqType() = TPowerRequest::EAllocReserve;
	req->ClientId() = aClientId;
	req->RequestCount() = aNumRm;
	req->ClientLevelCount() = aNumCl;

	UnLock();
	req->SendReceive(iMsgQ);
	return (req->ReturnCode());
	}
/**
@internalComponent
@prototype 9.5

This function runs in the context of the RC thread and 
handles creation of memory pools. 
*/
TInt DPowerResourceController::HandleReservationOfObjects(TPowerRequest& aRequest)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::AllocReserve"));

	SPowerResourceClientLevel* pCL = NULL;
	SPowerRequest* pR = NULL;

	Lock();
	TInt clientPoolCount = iClientLevelPoolCount;
	TInt requestPoolCount = iRequestPoolCount;
	SPowerResourceClient* pC;
	if(aRequest.ClientId() & USER_SIDE_CLIENT_BIT_MASK)
		pC = iUserSideClientList[(aRequest.ClientId() & ID_INDEX_BIT_MASK)];
	else																				
		pC = iClientList[(aRequest.ClientId() & ID_INDEX_BIT_MASK)];
	UnLock();

	if(clientPoolCount < aRequest.ClientLevelCount())
		{
		//Grow the client level pool
		// coverity[alloc_fn]
		pCL = new SPowerResourceClientLevel[iClientLevelPoolGrowBy + aRequest.ClientLevelCount()];
		if(!pCL)
			return(KErrNoMemory);
		}
	if(requestPoolCount < aRequest.RequestCount())
		{
		//Grow the request pool
		// coverity[alloc_fn]
		pR = new SPowerRequest[iRequestPoolGrowBy + aRequest.RequestCount()];
		if(!pR)
			{
			if(pCL) //If client level is allocated delete the same.
				delete []pCL;
			return(KErrNoMemory);
			}
		}
	//Push the memory to list and adjust the counter.
	Lock();
	TUint count;
	if(pCL)
		{
		for(count = 0;count<(TUint)(iClientLevelPoolGrowBy+aRequest.ClientLevelCount());count++)
			LIST_PUSH(iClientLevelPool, &pCL[count], iNextInList);
		iClientLevelPoolCount= (TUint16)(iClientLevelPoolCount + iClientLevelPoolGrowBy);
		pC->iReservedCl= (TUint8)(pC->iReservedCl + aRequest.ClientLevelCount());
		}
		else
		{
		//Reserve memory from free pool to this client
		iClientLevelPoolCount = (TUint16)(iClientLevelPoolCount - aRequest.ClientLevelCount());
		pC->iReservedCl = (TUint8)(pC->iReservedCl + aRequest.ClientLevelCount());
		}

	if(pR)
		{
		for(count=0;count<(TUint)(iRequestPoolGrowBy+aRequest.RequestCount());count++)
 			LIST_PUSH(iRequestPool, &pR[count], iNext);
		iRequestPoolCount = (TUint16)(iRequestPoolCount + iRequestPoolGrowBy);
		pC->iReservedRm =(TUint8)(pC->iReservedRm + aRequest.RequestCount());
		}
	else
		{
		//Reserve memory from free pool to this client
		iRequestPoolCount = (TUint16)(iRequestPoolCount - aRequest.RequestCount());
		pC->iReservedRm = (TUint8)(pC->iReservedRm + aRequest.RequestCount());
		}
	UnLock();
#ifdef PRM_INSTRUMENTATION_MACRO
    TUint size =0;
    if(pCL)
		size = (iClientLevelPoolGrowBy+aRequest.ClientLevelCount())*sizeof(SPowerResourceClientLevel);
    if(pR)
		size += (iRequestPoolGrowBy+aRequest.RequestCount())*sizeof(SPowerRequest);
    if(size)
        PRM_MEMORY_USAGE_TRACE
#endif
	return(KErrNone);
	}

/*  Register the proxy client to resource controller.
	This is called as the result of new user side client opening a channel.*/
TInt DPowerResourceController::RegisterProxyClient(TUint& aClientId, const TDesC8& aName)
	{
	GET_CRITICAL_SECTION_COUNT
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::RegisterProxyClient"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("Proxy client name %S", &aName));

	DThread& t = Kern::CurrentThread();	
	CHECK_CONTEXT(t)
	//If number of expected user side clients is set to 0 during initial configuration
	//then dont allow to configure user side clients.
	if(!iUserSideClientList.GrowBy())
		return KErrNotSupported;
	//Maximum allowable length of a client's name is 32 characters.
	if (aName.Length() > KMaxClientNameLength) return KErrTooBig;

	SPowerResourceClient *pC = NULL;
    Lock();
#ifdef DEBUG_VERSION
    if(!iUserSideClientList.Find(pC, (TDesC8&)aName))
		{
		UnLock();
		LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
		return KErrAlreadyExists;
		}
#endif
	TPowerRequest* req = (TPowerRequest*)&TPowerRequest::Get();
	req->ReqType() = TPowerRequest::ERegisterUsersideClient;
	UnLock();
	req->SendReceive(iMsgQ);
	if(req->ReturnCode() == KErrNone)
		{
		pC = iUserSideClientList[(req->ClientId() & ID_INDEX_BIT_MASK)];
		pC->iName=&aName;
		//Store the current thread Id;
		pC->iThreadId = t.iId;
		aClientId = pC->iClientId;
	    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::RegisterProxyClient, clientId = 0x%x", aClientId));
	    PRM_CLIENT_REGISTER_TRACE
		}
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return KErrNone;
	}

/*  Deregister the specified user side client from resource controller.
	This is called as the result of client closing the channel. */
TInt DPowerResourceController::DeregisterProxyClient(TUint aClientId)
	{
	GET_CRITICAL_SECTION_COUNT
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::DeregisterProxyClient"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("ClientId = 0x%x", aClientId));
	DThread& t = Kern::CurrentThread();	
	CHECK_CONTEXT(t)
	//Get the index from client ID
	if(!(aClientId & USER_SIDE_CLIENT_BIT_MASK))
		return KErrArgument;
	Lock();
	SPowerResourceClient* pC = iUserSideClientList[(aClientId & ID_INDEX_BIT_MASK)];
	if(!pC)
		{
		UnLock();
		LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
		return KErrNotFound;
		}
	if(pC->iClientId != aClientId)
		{
        __KTRACE_OPT(KRESMANAGER, Kern::Printf("Client ID does not match"));
	     UNLOCK_RETURN(KErrNotFound);
		}
	if(pC->iClientId & CLIENT_THREAD_RELATIVE_BIT_MASK)
		{
		if(pC->iThreadId != t.iId)
			{
			UnLock();
			LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
			return KErrAccessDenied;
			}
		}
	//Check for any pending request
	if(pC->iPendingReqCount)
		{
		UnLock();
		LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
		Panic(EClientHasPendingAsyncRequest);
		}
	//Check for notification request
	if(pC->iNotificationList)
		{
		UnLock();
		LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
		Panic(EClientHasNotificationObject);
		}
#ifdef PRM_ENABLE_EXTENDED_VERSION
	if(pC->iDynamicResCount)
		{
		UnLock();
		LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
		Panic(DPowerResourceController::EClientHasDynamicResourceRegistered);
		}
#endif
	ResourceStateChangeOfClientLevels(pC);
	//Add reserved request to pool
	iRequestPoolCount = (TUint16)(iRequestPoolCount + pC->iReservedRm);
	PRM_CLIENT_DEREGISTER_TRACE
	//Increment the free pool count for client level and request level.
	iUserSideClientList.Remove(pC, (pC->iClientId & ID_INDEX_BIT_MASK));
	pC->iName = NULL;
	iUserSideClientCount--; //Decrement client count
	LIST_PUSH(iClientPool, pC, iNextInList);
	UnLock();
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return KErrNone;
	}

/* This is called from power controller to cache the state of resource whose
   state information it is interested in for accessing from null thread. This
   list needs to be accessed from the Idle thread using direct access. */
TInt DPowerResourceController::RegisterResourcesForIdle(TInt aPowerControllerId, TUint aNumResources, TPtr* aBuf)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::RegisterResourceForIdle"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("PowerControllerId = 0x%x, NumResources = %d", aPowerControllerId, aNumResources));
#ifdef DEBUG_VERSION //Surrounding with macro to avoid warnings.
	DThread& t = Kern::CurrentThread();	
	CHECK_CONTEXT(t)
#endif
	if(!aBuf)
		return KErrArgument;
	if((TUint)aPowerControllerId != iPowerControllerId)
		return KErrAccessDenied;
	if(iListForIdle) //Only one list is allowed.
		return KErrInUse;
	if((TUint)(aBuf->MaxLength() - aBuf->Length()) < (sizeof(SIdleResourceInfo) * aNumResources))
		return KErrArgument;
	GET_CRITICAL_SECTION_COUNT
	Lock();
	SIdleResourceInfo* pS=(SIdleResourceInfo*)aBuf->Ptr();
	DStaticPowerResource* pR=NULL;
	TUint count=0;
	TUint id=0;
	for(count=0;count<aNumResources;count++) //Check for valid resource ID.
		{
#ifndef PRM_ENABLE_EXTENDED_VERSION
		if((!pS->iResourceId) || (pS->iResourceId > (TUint)iStaticResourceArray.Count()) || (!iStaticResourceArray[pS->iResourceId-1]))
			{
			UnLock();
			LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
			return KErrNotFound;
			}
#else
		if(pS->iResourceId & KIdMaskDynamic)
			{
			UnLock();
			LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
			return KErrNotSupported;
			}
		if((!pS->iResourceId) || ((pS->iResourceId & KIdMaskResourceWithDependencies) && 
			     (pS->iResourceId > (TUint)iStaticResDependencyArray.Count())) || (!(pS->iResourceId & KIdMaskResourceWithDependencies) && 
					((pS->iResourceId > (TUint)iStaticResourceArray.Count()) || (!iStaticResourceArray[pS->iResourceId-1]))))
			{
			UnLock();
			LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
			return KErrNotFound;
			}
#endif
		pS++;
		}
	pS = (SIdleResourceInfo*)aBuf->Ptr();
	for(count=0;count<aNumResources;count++)
		{
		id=pS->iResourceId;
#ifdef PRM_ENABLE_EXTENDED_VERSION
		if(id & KIdMaskResourceWithDependencies) //Dependency resource
			pR = iStaticResDependencyArray[(id & ID_INDEX_BIT_MASK)-1];
		else
#endif
		pR=iStaticResourceArray[id-1];
		pS->iLevelOwnerId = pR->iLevelOwnerId;
		pS->iCurrentLevel = pR->iCachedLevel;
		pR->iIdleListEntry=pS;
		pS++;
		}
	iListForIdle=(SIdleResourceInfo*)aBuf->Ptr();
	UnLock();
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK
	return KErrNone;
	}
	
/**
@publishedPartner
@prototype 9.6

Request to deregister client level from the specified resource for the specified client. 

@param aClientId	ID of the client which is requesting the deregistration of client level.
@param aResourceId	ID of the resource from which to remove the specified client's level.

@return KErrNone			if successful
		KErrAccessDenied	if the client ID could not be found in the list of registered clients or
							if the client was registered to be thread relative and this API is not 
							called from the same thread.
		KErrNotFound		if the resource ID could not be found in the current list of controllable 
							resources or if the client is not holding any level with the specified 
							resource (no client level found for the specified client).

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver.
*/
TInt DPowerResourceController::DeRegisterClientLevelFromResource(TUint aClientId, TUint aResourceId)
	{
	TInt r = KErrNone;
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::DeRegisterClientLevelFromResource\n"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId = 0x%x, aResourceId = 0x%x\n", aClientId, aResourceId));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	SPowerResourceClient* pC = NULL;
	Lock();
	VALIDATE_CLIENT(thread);
	//Validate resource
	if((!aResourceId))
		{
		UNLOCK_RETURN(KErrNotFound);
		}
	DStaticPowerResource *pR = NULL;
#ifdef PRM_ENABLE_EXTENDED_VERSION
	GET_RESOURCE_FROM_LIST(aResourceId, pR)
	if(aResourceId & KIdMaskDynamic)
		((DDynamicPowerResource*)pR)->Lock();
#else
	if(aResourceId > (TUint)iStaticResourceArray.Count())
		{
		UNLOCK_RETURN(KErrNotFound);
		}
	pR = iStaticResourceArray[aResourceId - 1];
	if(!pR)
		UNLOCK_RETURN(KErrNotFound);
#endif
	//Need to change the state of the resource if the client is holding the current resource.
	if((pR->iLevelOwnerId == (TInt)aClientId) || (pR->Sense() == DStaticPowerResource::ECustom)) 
		{
		//If Longlatency resource then process it in the resource controller thread
		TPowerRequest* req = (TPowerRequest*)&TPowerRequest::Get();
		req->ResourceId() = aResourceId;
		req->ReqType() = TPowerRequest::ESetDefaultLevel;
		req->ClientId() = aClientId;
		req->Resource() = pR;
		req->ResourceCb() = NULL;
#ifdef PRM_INSTRUMENTATION_MACRO
		//Setting to current state as exact state will be known only before calling the PSL.
		TInt aNewState = pR->iCachedLevel; 
		PRM_CLIENT_CHANGE_STATE_START_TRACE
#endif
		if(pR->LatencySet())
			{
			UnLock();
#ifdef PRM_ENABLE_EXTENDED_VERSION
			if(pR->iResourceId & KIdMaskResourceWithDependencies) //Dependency resource
				{
				r = req->SendReceive(iMsgQDependency); // Send the request to DFC thread.
				}
			else
#endif
				{
				r = req->SendReceive(iMsgQ); //Long latency resource request are processed in controller thread.
				}
			Lock();
			}
		else if(pR->Usage())
			{
			//Shared resource
			//Not checking the return value here because there is no allocation of client levels. 
			CheckLevelAndAddClient(pC, req); 
			}
		else
			{
			//Single user set it to default
			req->ClientId() = -1;
			req->ReqType() = TPowerRequest::ESetDefaultLevel;
			}
		//Change the state of resource for instantaneous resource.
		if((!pR->LatencySet()) && ((!pR->Usage()) || (pR->Usage() && req->RequiresChange())))
			{
			UnLock();
			r = pR->DoRequest(*req);
			Lock();
			if(r == KErrNone)
				{
				//Complete notifications
				CompleteNotifications(req->ClientId(), pR, req->Level(), KErrNone, req->ClientId(), EFalse);
				//Update the cache
				pR->iLevelOwnerId = req->ClientId();
				pR->iCachedLevel = req->Level();
				if(pR->iIdleListEntry)
					{
					pR->iIdleListEntry->iLevelOwnerId = req->ClientId();
					pR->iIdleListEntry->iCurrentLevel = req->Level();
					}
				}
			}
#ifdef PRM_INSTRUMENTATION_MACRO
		if(!pR->LatencySet())
			{
			aNewState = req->Level();
			PRM_CLIENT_CHANGE_STATE_END_TRACE
			}
#endif
		}
	//Remove clientLevel from client
	r = KErrNotFound;
	for(SPowerResourceClientLevel* pCL = pC->iLevelList; pCL != NULL; pCL= pCL->iNextInList)
		{
		if(pCL->iResourceId == aResourceId)
			{
			LIST_REMOVE(pC->iLevelList, pCL, iNextInList, SPowerResourceClientLevel);
			//Remove from Resource
			pCL->Deque();
			LIST_PUSH(iClientLevelPool,pCL,iNextInList); // back to free pool
			if(pC->iUnderFlowClCount > 0)
				{
				pC->iUnderFlowClCount--;
				iClientLevelPoolCount++;
				}
			else
				pC->iReservedCl++;
			r = KErrNone;
			break;
			}
		}
#ifdef PRM_ENABLE_EXTENDED_VERSION
	if(aResourceId & KIdMaskDynamic)
		((DDynamicPowerResource*)pR)->UnLock();
#endif
	UNLOCK_RETURN(r);
	}

/**
@publishedPartner
@prototype 9.5

Interface to provide extended functionality.This provides support
to register and deregister dynamic resources and handling of resource dependency, registering
and deregistering resource dependency.
This is not supported in basic version

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver.
*/
TInt DPowerResourceController::GetInterface(TUint aClientId, TUint aInterfaceId, TAny* aParam1, TAny* aParam2, 
											                                             TAny* aParam3)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::GetInterface"));
	DThread& thread = Kern::CurrentThread();	
	CHECK_CONTEXT(thread)
	if((iInitialised != EResConStartupCompleted) && (aInterfaceId != KResManControlIoGetVersion))
		return KErrNotSupported;
	TInt r = KErrNone;
	Lock();
	SPowerResourceClient* pC = NULL;
	VALIDATE_CLIENT(thread);
#ifndef PRM_ENABLE_EXTENDED_VERSION
	if(aInterfaceId == KResManControlIoGetVersion)
		{
		if(!aParam1)
			r = KErrArgument;
		else
			*(TUint*)aParam1  = KResControllerBasicVersion;
		}
	else
		r = KErrNotSupported;
	(void) aParam2;
	(void) aParam3;
#else
	//User side client is not allowed to register/deregister dynamic resource and dependencies
	if((aClientId & USER_SIDE_CLIENT_BIT_MASK) && (aInterfaceId >= KResManControlIoRegisterDynamicResource) && 
		                                                 (aInterfaceId <= KResManControlIoDeregisterDependency))
		return KErrAccessDenied;
	switch (aInterfaceId)
		{
		case KResManControlIoGetVersion:
			{
			if(!aParam1)
				r = KErrArgument;
			else
				*(TUint*)aParam1 = KResControllerExtendedVersion;
			break;
			}
		case KResManControlIoRegisterDynamicResource:
			{
			r = RegisterDynamicResource(pC, (DDynamicPowerResource *)aParam1, (TUint*)aParam2);
			break;
			}
		case KResManControlIoDeregisterDynamicResource:
			{
			r = DeregisterDynamicResource(pC, (TUint)aParam1, (TInt*)aParam2);
			break;
			}
		case KResManControlIoRegisterDependency:
			{

			r = RegisterResourceDependency(pC, (SResourceDependencyInfo*)aParam1, (SResourceDependencyInfo*)aParam2);
			break;
			}
		case KResManControlIoDeregisterDependency:
			{
			r = DeregisterResourceDependency(pC, (TUint)aParam1, (TUint)aParam2);
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::GetInterface"));
			return(r);
			}
		case KResManControlIoGetNumDependents:
			{
			r = GetNumDependentsForResource((TUint)aParam1, (TUint*)aParam2);
			break;
			}
		case KResManControlIoGetDependentsId:
			{
			r = GetDependentsIdForResource((TUint)aParam1, (TAny*)aParam2, (TUint*)aParam3);
			break;
			}
		default:
			{
			r = KErrNotSupported;
			break;
			}
		}
#endif
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::GetInterface"));
	UNLOCK_RETURN(r);
	}

