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
// e32\drivers\resourceman\rescontrol_export.cpp
//
//

#include <drivers/resourcecontrol.h>

#ifdef DEBUG_VERSION
#define GET_CRITICAL_SECTION_COUNT(thread)				\
	TInt CsCount = thread.iNThread.iCsCount;

#define LOCK_AND_CRITICAL_SECTION_COUNT_CHECK(thread)			\
	if(thread.iNThread.iCsCount != CsCount)						\
		Kern::Fault("PowerResourceController", __LINE__);		\
	if(pRC->iResourceMutex->iHoldCount != 0)					\
		Kern::Fault("PowerResourceController", __LINE__);	
#else
#define GET_CRITICAL_SECTION_COUNT(thread)
#define LOCK_AND_CRITICAL_SECTION_COUNT_CHECK(thread)
#endif

/**
	@publishedPartner
	@prototype 9.5
	Kernel extension or variants can call this API to set the post bool value without registering
	as client with the resource controller. This can be used by the resource controller PSL to set the
	specified static resources to appropriate value before resource controller is fully initialized.
	@param aResId ID of the resource whose level should be set after initialisation
	@param aLevel Resource level to set
	@return KErrNone, if operation is success
			KErrNotFound, if resource ID could not be found in the static resource array.
			KErrNotSupported, if this API is called after resource controller is fully initialized
	*/
EXPORT_C TInt DPowerResourceController::PostBootLevel(TUint aResId, TInt aLevel)
	{
#ifdef DEBUG_VERSION
	DThread& thread = Kern::CurrentThread();
	GET_CRITICAL_SECTION_COUNT(thread)
#endif
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::PostBootLevel, aResId = 0x%x, aLevel = %d", 
	                                                                          aResId, aLevel));
    DPowerResourceController* pRC = TInterface::GetPowerResourceController();
	if(!pRC)
		return KErrNotFound;
	pRC->Lock();
    CHECK_CONTEXT(thread);
	//Accept the postboot level only if issued before controller is fully initialised.
	if(pRC->iInitialised == EResConStartupCompleted)
		{
		pRC->UnLock();
		LOCK_AND_CRITICAL_SECTION_COUNT_CHECK(thread)
		return KErrNotSupported;
		}	
#ifndef PRM_ENABLE_EXTENDED_VERSION
    // coverity[deref_ptr]
    // aResId is checked to be more than the array entries before dereferencing pRC->iStaticResourceArray
	if((!aResId) || (aResId > (TUint)pRC->iStaticResourceArray.Count()) || (!pRC->iStaticResourceArray[aResId-1]))
		{
		pRC->UnLock();
		LOCK_AND_CRITICAL_SECTION_COUNT_CHECK(thread)
		return KErrNotFound;
		}
#else
	if(!aResId || ((aResId & KIdMaskResourceWithDependencies) && ((aResId & ID_INDEX_BIT_MASK) > (TUint)pRC->iStaticResDependencyArray.Count())) 
				|| (!(aResId & KIdMaskResourceWithDependencies) && ((aResId > (TUint)pRC->iStaticResourceArray.Count())
				|| (!pRC->iStaticResourceArray[aResId-1]))))
		{
		pRC->UnLock();
		LOCK_AND_CRITICAL_SECTION_COUNT_CHECK(thread)
		return KErrNotFound;
		}
	if(aResId & KIdMaskResourceWithDependencies)
		{
		aResId &= ID_INDEX_BIT_MASK;
		DStaticPowerResource* pR = pRC->iStaticResDependencyArray[--aResId];
		pR->iPostBootLevel=aLevel;
		pR->iFlags |= SET_VALID_POST_BOOT_LEVEL;
		}
	else
#endif
    if((TUint)pRC->iStaticResourceArray.Count() > aResId - 1)
		{
		DStaticPowerResource* pR=pRC->iStaticResourceArray[--aResId];
		pR->iPostBootLevel=aLevel;
		pR->iFlags |= SET_VALID_POST_BOOT_LEVEL; // To indicate valid post boot level is set.
		}
	pRC->UnLock();
	LOCK_AND_CRITICAL_SECTION_COUNT_CHECK(thread)
	return KErrNone;
    }

/** 
	@publishedPartner
	@prototype 9.5
	Kernel extensions or variants can call this API to register the static resources before resource controller
	is fully initialised.
	@Param aClientId             ID of the client that is requesting resource registration
	@Param aStaticResourceArray  Static resources to register with RC.
	                             Note, that in the special case, when aResCount equals to one, this parameter is treated as a pointer to the
	                             DStaticPowerResource (DStaticPowerResource*). Otherwise - is the pointer to array of such pointers (DStaticPowerResource*).
	@Param aResCount             Number of static resources to register with RC. This equals the size of the passed array.
	@return KErrNone, if operation is success
	        KErrAccessDenied if clientId could not be found in the current list of registered clients or if this
			                 client was registered as thread relative and was not called from the same thread.
			KErrNotSupported if called after resource controller is fully initialised or if called from user side proxy
			                 or if the resource is dynamic or dependency resource.
			KErrNoMemory     if there is insufficient memory.
			KErrArgument     if passed array is null or passed number of resources count is 0.
	*/
EXPORT_C TInt DPowerResourceController::RegisterArrayOfStaticResources(TUint aClientId, DStaticPowerResource**& aStaticResourceArray, TUint aResCount)
	{
	DThread& thread = Kern::CurrentThread();
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::RegisterStaticResource"));
    DPowerResourceController* pRC = TInterface::GetPowerResourceController();
	if(!pRC)
		return KErrNotFound;

	if(!aStaticResourceArray || (aResCount == 0))
		return KErrArgument;
	CHECK_CONTEXT(thread);
	//Accept the registration of static resource only if issued before controller is fully initialised.
	if(pRC->iInitialised == EResConStartupCompleted)
		{
		return KErrNotSupported;
		}
	//User side clients and resource with dependency are not supported.
	if(aClientId & USER_SIDE_CLIENT_BIT_MASK)
		{
		return KErrNotSupported;
		}
#ifdef PRM_ENABLE_EXTENDED_VERSION
	// if aResCount equals to 1 aStaticResourceArray contains not an array, but simply a pointer to the resource.
	if(aResCount == 1)
		{
		if(((DStaticPowerResource*)aStaticResourceArray)->iResourceId & (KIdMaskResourceWithDependencies | KIdMaskDynamic))
			{
			return KErrNotSupported;
			}
		}
	else
		{
		for(TUint rescount = 0; rescount < aResCount; rescount++)
			{
			if(aStaticResourceArray[rescount] &&
			  (aStaticResourceArray[rescount]->iResourceId & (KIdMaskResourceWithDependencies | KIdMaskDynamic)))
				{
				return KErrNotSupported;
				}
			}
		}
#endif
	SPowerResourceClient* pC = pRC->iClientList[(TUint16)(aClientId & ID_INDEX_BIT_MASK)];
	if(!pC)
		{
		__KTRACE_OPT(KRESMANAGER, Kern::Printf("Client ID not Found"));
		return KErrAccessDenied;
		}
	if(pC->iClientId != aClientId)
		{
		__KTRACE_OPT(KRESMANAGER, Kern::Printf("Client ID instance count does not match"));
		return KErrAccessDenied;
		}
	if(pC->iClientId & CLIENT_THREAD_RELATIVE_BIT_MASK)
		{
		if(pC->iThreadId != thread.iId)
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("Client not called from thread context(Thread Relative)"));
			return KErrAccessDenied;
			}
		}

    TInt r = KErrNone;
	if(aResCount == 1)
		{
		// if aResCount equals to one, threat the pointer as a pointer to resource
		r = pRC->iStaticResourceArray.Append((DStaticPowerResource*)aStaticResourceArray);
		// increment count of valid resources
		if(r == KErrNone && aStaticResourceArray)
			pRC->iStaticResourceCount++;
		}
	else
		{
		for(TUint count = 0; count < aResCount; count++)
			{
			r = pRC->iStaticResourceArray.Append(aStaticResourceArray[count]);
			if(r != KErrNone)
				{
				__KTRACE_OPT(KRESMANAGER, Kern::Printf("Could not add new static resources, r = %d", r));
				break;
				}
			// increment count of valid resources
			if(aStaticResourceArray[count])
				pRC->iStaticResourceCount++;
			}
		}

	return r;
	}

/**
	@publishedPartner
	@prototype 9.5
	Kernel extensions or variants can call this API to register the static resources before resource controller
	is fully initialized. 
	@Param aClientId ID of the client that is requesting resource registration
	@Param pR        Static resource to register with RC. 
	@return KErrNone, if operation is success
			KErrAccessDenied if clientId could not be found in the current list of registered clients or if this 
							 client was registered as thread relative and was not called from the same thread. 
			KErrNotSupported if called after resource controller is fully initialized or if called from user side proxy
							 or if the resource is dynamic or dependency resource
			KErrNoMemory if there is insufficient memory.
			KErrArgument if passed array is null
	*/
EXPORT_C TInt DPowerResourceController::RegisterStaticResource(TUint aClientId, DStaticPowerResource* pR)
	{
	return RegisterArrayOfStaticResources(aClientId, (DStaticPowerResource**&)pR, 1);
    }

/**
	@publishedPartner
	@prototype 9.5
	This function initialises the controller. 
	@return KErrNone, if operation is success or one of the system wide errors.
	*/
RPointerArray <DStaticPowerResource> *StaticResourceArrayPtr;
#ifdef PRM_ENABLE_EXTENDED_VERSION
RPointerArray <DStaticPowerResourceD> *StaticResourceDependencyArrayPtr;
#endif
EXPORT_C TInt DPowerResourceController::InitController()
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DPowerResourceController::InitController()"));
	DPowerResourceController* pRC = TInterface::GetPowerResourceController();
	if(!pRC)
		return KErrNotFound;
	if(pRC->iInitialised >= EResConInitialised)
		{
		__KTRACE_OPT(KRESMANAGER, Kern::Printf("InitController already initialised %d\n", pRC->iInitialised));
		return KErrNone;
		}
    _LIT(KResMutexName, "RESCTRL");
    TInt r=KErrNone;
	//Create the message queue
	pRC->iMsgQ = new TMessageQue(DPowerResourceController::MsgQFunc, pRC, NULL, 2);
	if(!pRC->iMsgQ)
		return KErrNoMemory;
#ifdef PRM_ENABLE_EXTENDED_VERSION
	//Create the message queue for dependency resource processing.
	pRC->iMsgQDependency = new TMessageQue(DPowerResourceController::MsgQDependencyFunc, pRC, NULL, 1);
	if(!pRC->iMsgQDependency)
		return KErrNoMemory;
#endif
	// This method can be called in two situations - before the constructor of DPowerResourceController was called 
	// for the second time (placement new in the extension psl entry macro) e.g. as a result of the call to InitResources() 
	// from the variant::Init3() method) or after that.

	// In order not to make any assumption on number of constructor invocations, a copy (binary) of the iStaticResourceArray object 
	// is created below, so that it could be used to later restore the original iStaticResoureceArray object if the constructor
	// was called after this method. The reason for that is, that in this destructor calls the default RPointerArrayBase()
	// which resets the array, i.e. it looses the information, but allocated area and pointers still exist in the memory. 
	// It is then valid to restore the object directly (which will copy all members, including iSize and iEntries pointers). 
	// This temporary object will be deleted in DPowerResourceController::InitResources() at the last stage of initialization.
	// (see also comments in DPowerResourceController::DPowerResourceController()) 

	StaticResourceArrayPtr = new RPointerArray <DStaticPowerResource>;
	if(!StaticResourceArrayPtr)
		return KErrNoMemory;

	r = pRC->DoRegisterStaticResources(pRC->iStaticResourceArray);
	if(r != KErrNone)
		return r;

	// make a copy (see above comment)
	*StaticResourceArrayPtr = pRC->iStaticResourceArray; 

	// Get the actual number of static resource registered count
	for(TInt resCnt = 0; resCnt < pRC->iStaticResourceArray.Count(); resCnt++)
		{
		if(pRC->iStaticResourceArray[resCnt])
			pRC->iStaticResourceCount++;
		}
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("Actual number of static resource registered = %d\n", pRC->iStaticResourceCount));
#ifdef PRM_INSTRUMENTATION_MACRO
	// Btrace output of resource information of each resource.
	DStaticPowerResource* pR = NULL;
	TPowerResourceInfoBuf01 resInfo;
	TPowerResourceInfoV01 *pResInfo;
	for(TInt resCount = 0; resCount < pRC->iStaticResourceArray.Count(); resCount++)
		{
		pR = pRC->iStaticResourceArray[resCount];
		if(!pR)
			continue;
		pR->GetInfo((TDes8*)resInfo.Ptr());
		pResInfo = (TPowerResourceInfoV01*)resInfo.Ptr();
		PRM_REGISTER_RESOURCE_TRACE
		}
#endif

#ifdef PRM_ENABLE_EXTENDED_VERSION
	StaticResourceDependencyArrayPtr = new RPointerArray <DStaticPowerResourceD>;
	if(!StaticResourceDependencyArrayPtr)
		return KErrNoMemory;

	// Call PSL to register static resources with dependency if any exists
	r = pRC->DoRegisterStaticResourcesDependency(pRC->iStaticResDependencyArray);

	if(r != KErrNone)
		return r;

	// make a copy (see above comments for StaticResourceArrayPtr)
	*StaticResourceDependencyArrayPtr = pRC->iStaticResDependencyArray;

	if(pRC->iStaticResDependencyArray.Count())
		{
		DStaticPowerResourceD* pRD = NULL;
		TUint count;
		//Assign resource index in resource id
		for(count = 0; count < (TUint)pRC->iStaticResDependencyArray.Count(); count++)
			{
			pRD = pRC->iStaticResDependencyArray[count];
			if(!pRD)
				Panic(DPowerResourceController::ERegisteringDependentStaticResourceWithHoles);
			pRD->iResourceId |= ((count + 1) & ID_INDEX_BIT_MASK);
			}
		//Check for dependency closed loops
		for(count = 0; count < (TUint)pRC->iStaticResDependencyArray.Count(); count++)
			{
			pRD = pRC->iStaticResDependencyArray[count];
			if(!(pRD->iResourceId & KIdMaskStaticWithDependencies))
				Panic(DPowerResourceController::ERegisteringNonDependentStaticResource);
			//Upgrade latency state change from instantaneous to long latency 
			if(!pRD->LatencySet()) 
				pRD->iFlags |= KLongLatencySet;
			pRC->CheckForDependencyLoop(pRD, pRD->iResourceId, pRD->iResourceId);
			}
#ifdef PRM_INSTRUMENTATION_MACRO
		for(count = 0; count < (TUint)pRC->iStaticResDependencyArray.Count(); count++)
			{
			pR = pRC->iStaticResDependencyArray[count];
			pR->GetInfo((TDes8*)resInfo.Ptr());
			pResInfo = (TPowerResourceInfoV01*)resInfo.Ptr();
			PRM_REGISTER_STATIC_RESOURCE_WITH_DEPENDENCY_TRACE
			}
#endif
		}
#endif // PRM_ENABLE_EXTENDED_VERSION
	// Create mutex object
	r=Kern::MutexCreate(pRC->iResourceMutex, KResMutexName, KMutexOrdResourceManager);
	if(r==KErrNone) // Call PSL to create DFC queue and creation of pools with the help API's provided by generic layer.
		r=pRC->DoInitController();
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DPowerResourceController::InitController()"));
	if(r == KErrNone)
		{
		pRC->iInitialised = EResConInitialised;
	    if(pRC->iDfcQ)
			pRC->iMsgQ->Receive();
#ifdef PRM_ENABLE_EXTENDED_VERSION
	    if(pRC->iDfcQDependency)
			pRC->iMsgQDependency->Receive();
#endif
		}
    return r;
	}
