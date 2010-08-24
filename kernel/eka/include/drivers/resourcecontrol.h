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
// e32\include\drivers\resourcecontrol.h
//
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __RESOURCECONTROL_H__
#define __RESOURCECONTROL_H__
#include <nklib.h>
#include <kernel/kernel.h>
#include <kernel/kern_priv.h>
#include <e32ver.h>
#define PRM_CONTROLLER
#ifndef PRM_ENABLE_EXTENDED_VERSION
#include <drivers/resource.h>
#else
#include <drivers/resource_extend.h>
#endif 
#include <drivers/resourceman.h>

/** #defines for client bit masks */
#define ID_INDEX_BIT_MASK 0x3FFF  /* bit 0 -13 */
#define USER_SIDE_CLIENT_BIT_MASK 0x4000 //Bit 14
#define CLIENT_THREAD_RELATIVE_BIT_MASK 0x80000000 //Bit 31
#define INSTANCE_COUNT_BIT_MASK 0x1FFF //13 Bits
#define INSTANCE_COUNT_POS 18
#define CLIENT_POWER_CONTROLLER_BIT_MASK 0x8000 //Bit 15
/** Bit to indicate valid post boot level in resource */
#define SET_VALID_POST_BOOT_LEVEL 0x8000 //Bit 15

#define RESOURCE_NOT_IN_OPERATION 0x1	
#define PRM_DYNAMIC_RESOURCE_INITIAL_SIZE 2

#define PRM_STATIC_RESOURCE					0x0
#define PRM_STATIC_DEPENDENCY_RESOURCE		0x1
#define PRM_DYNAMIC_RESOURCE				0x2	
#define PRM_DYNAMIC_DEPENDENCY_RESOURCE		0x3
#define RESOURCE_BIT_IN_ID_CHECK			16


static const TInt KMaxResourceNameLength=0x20; //Maximum allowable resource length is 32 characters.
static const TInt KMaxClientNameLength=0x20; //Maximum allowable client length is 32 characters.


_LIT8(KPowerController, "PowerController");
_LIT8(KDfcThread1Name, "DfcThread1");
_LIT8(KDfcThread0Name, "DfcThread0");
_LIT8(KNullThreadName, "Null");
_LIT8(KNoClient, "NoClient");
_LIT8(KParentResource, "ParentResource");
/** Macro to check the context of client calling RM API.
    Panics if it is called from ISR, IDFC, NULL thread or DFC thread1 */
#ifdef DEBUG_VERSION
#define CHECK_CONTEXT(t)																\
	__ASSERT_ALWAYS(NKern::CurrentContext() == NKern::EThread, Panic(ECalledFromIsr));	\
	const TDesC8* pDfc1 = &KDfcThread1Name;												\
	if(!pDfc1->Compare(*(TDesC8*)t.iName))												\
		Panic(ECalledFromDfcThread1);													\
    const TDesC8* pNull = &KNullThreadName;												\
	if(!pNull->Compare(*(TDesC8*)t.iName))												\
		Panic(ECalledFromNullThread);
#else
#define CHECK_CONTEXT(t)
#endif

/** Macro to unlock and return */
#define UNLOCK_RETURN(retval)       \
    {                               \
    UnLock();                       \
    return(retval);                 \
    }

/** Macro to push the item into the specified list. Item are pushed to the head of the list. */
#define LIST_PUSH(list,item,link)	\
	{                               \
	(item)->link = (list);			\
	(list) = (item);                \
	}

/** Macro to pop the item from the specified list. Item are poped from the head of the list. */
#define LIST_POP(list,item,link)	\
	{                               \
	(item) = (list);				\
	if ((item))						\
		{							\
		(list) = (item)->link;		\
		(item)->link = NULL;		\
		}                           \
	}

/** Macro to remove the item from the list. */
#define LIST_REMOVE(list,item,link,className)		\
	if (list)										\
		{											\
		className* current = (list);				\
		if (current==(item))						\
			{										\
			(list) = (item)->link;					\
			(item)->link = NULL;					\
			}										\
		else										\
			{										\
			className* next = current->link;		\
			while (next)							\
				{									\
				if ((item)==next)					\
					{								\
					current->link=next->link;		\
					next->link = NULL;				\
					break;							\
					}								\
				current = next;						\
				next = next->link;					\
				}									\
			}										\
		}


/* Macro to add dynamic resource to appropriate containers. Used only in extended version */
#define ADD_TO_RESOURCE_CONTAINER(list, res, resId, resIdCount)				\
	{																		\
	TInt growBy = (list).GrowBy();										\
	if(!growBy)																\
		(list).Initialise(PRM_DYNAMIC_RESOURCE_INITIAL_SIZE);		\
    TInt r = (list).Add(res, resId);                                        \
	if(r == KErrNoMemory)								                    \
		{																	\
		r = (list).ReSize(growBy);										    \
        if(r == KErrNone)                                                   \
            {                                                               \
            r = (list).Add(res, resId);										\
            }                                                               \
		}																	\
    if(r != KErrNone)                                                       \
        {                                                                   \
        return r;                                                           \
        }                                                                   \
	res->iResourceId |= resId;												\
	resId = res->iResourceId;												\
	resIdCount++;															\
	}
	
/* Macro to get the resource from appropriate list. Used only in extended version */
#define GET_RESOURCE_FROM_LIST(resId, res)														\
	{																							\
	switch((resId >> RESOURCE_BIT_IN_ID_CHECK) & 0x3)											\
		{																						\
		case PRM_STATIC_RESOURCE:																\
			if((TInt)resId > iStaticResourceArray.Count())										\
				UNLOCK_RETURN(KErrNotFound);													\
			res = iStaticResourceArray[resId - 1];												\
			if(!res)																			\
				UNLOCK_RETURN(KErrNotFound);													\
			break;																				\
		case PRM_STATIC_DEPENDENCY_RESOURCE:													\
			if((TInt)(resId & ID_INDEX_BIT_MASK) > iStaticResDependencyArray.Count())	\
				UNLOCK_RETURN(KErrNotFound);													\
			res = iStaticResDependencyArray[(resId & ID_INDEX_BIT_MASK)  - 1];			\
			break;																				\
		case PRM_DYNAMIC_RESOURCE:																\
			res = iDynamicResourceList[(resId & ID_INDEX_BIT_MASK)];					\
			if(!res)																			\
				UNLOCK_RETURN(KErrNotFound);													\
			break;																				\
		case PRM_DYNAMIC_DEPENDENCY_RESOURCE:													\
			res = iDynamicResDependencyList[(resId & ID_INDEX_BIT_MASK)];				\
			if(!res)																			\
				UNLOCK_RETURN(KErrNotFound);													\
			break;																				\
		default:																				\
			UNLOCK_RETURN(KErrArgument);														\
		}																						\
	}

/**Macro to get the client from appropriate client list based on bit 14 of client ID.
   If the client is registered as thread relative, then check is made to make sure
   it is called from the same thread. */
#define VALIDATE_CLIENT(t)																						\
	if(aClientId & USER_SIDE_CLIENT_BIT_MASK)																	\
		pC = iUserSideClientList[(aClientId & ID_INDEX_BIT_MASK)];										\
	else																										\
		pC = iClientList[(aClientId & ID_INDEX_BIT_MASK)];												\
	if(!pC)																										\
		{																										\
		__KTRACE_OPT(KRESMANAGER, Kern::Printf("Client ID not Found"));											\
		UNLOCK_RETURN(KErrAccessDenied);																		\
		}																										\
	if(pC->iClientId  != aClientId)																				\
		{																										\
		__KTRACE_OPT(KRESMANAGER, Kern::Printf("Client ID does not match"));									\
		UNLOCK_RETURN(KErrAccessDenied);																		\
		}																										\
	if(pC->iClientId & CLIENT_THREAD_RELATIVE_BIT_MASK)															\
		{																										\
		if(pC->iThreadId != t.iId)																				\
			{																									\
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("Client not called from thread context(Thread Relative)"));	\
			UNLOCK_RETURN(KErrAccessDenied);																	\
			}																									\
		}

/** Macro to get the target client from appropriate client list based on bit 14 of client ID. */
#define GET_TARGET_CLIENT()																				\
	if(aTargetClientId & USER_SIDE_CLIENT_BIT_MASK) 													\
		pC = iUserSideClientList[(aTargetClientId & ID_INDEX_BIT_MASK)];	    				\
	else																								\
		pC = iClientList[(aTargetClientId & ID_INDEX_BIT_MASK)];								\
	if(!pC)																								\
		{																								\
		__KTRACE_OPT(KRESMANAGER, Kern::Printf("Target Client ID not found"));							\
		UNLOCK_RETURN(KErrNotFound);																	\
		}																								\
	if(pC->iClientId != aTargetClientId)																\
		{																								\
		__KTRACE_OPT(KRESMANAGER, Kern::Printf("Client ID does not match"));							\
		UNLOCK_RETURN(KErrNotFound);																	\
		}

/* Macro definition for entry point of Power Resource Controller */
#define DECLARE_RESOURCE_MANAGER_EXTENSION(TheController)															\
	TDfc* resourceInitDfc = NULL;																					\
	static void ResourceInit(TAny* aController)																		\
		{																											\
		TInt aReason = NKern::EThread;																				\
		PRM_BOOTING_TRACE																							\
		((DPowerResourceController*)aController)->InitResources();													\
		delete resourceInitDfc;																						\
		return;																										\
		}																											\
	void CreateController();																						\
	GLDEF_C TInt KernelModuleEntry(TInt aReason)																	\
		{																											\
		if(aReason==KModuleEntryReasonVariantInit0)																	\
			{																										\
			__KTRACE_OPT(KBOOT, Kern::Printf("Create Resource Controller"));										\
			CreateController();																						\
			return KErrNone;																						\
			}																										\
		if (aReason==KModuleEntryReasonExtensionInit0)																\
			return KExtensionMaximumPriority;																		\
		if (aReason!=KModuleEntryReasonExtensionInit1)																\
			return KErrArgument;																					\
		PRM_BOOTING_TRACE																							\
		__KTRACE_OPT(KBOOT, Kern::Printf("Initialise Resource Controller"));										\
		TInt r = KErrNone;																							\
		r = DPowerResourceController::InitController();																\
		if(r != KErrNone)																							\
			return r;																								\
		__KTRACE_OPT(KBOOT, Kern::Printf("Create PDD and queue ResourceInit DFC"));									\
		DResConPddFactory* device = new DResConPddFactory;															\
		if(!device)																									\
			return KErrNoMemory;																					\
		r = Kern::InstallPhysicalDevice(device);																	\
		if(r != KErrNone)																							\
			return r;																								\
		resourceInitDfc = new TDfc(&ResourceInit,(TAny*)&TheController,Kern::SvMsgQue(),KMaxDfcPriority-1);			\
		if(!resourceInitDfc)																						\
			return KErrNoMemory;																					\
		resourceInitDfc->Enque();																					\
       	return KErrNone;																							\
		}																											\
		GLDEF_C void CreateController()

struct SPowerResourceClient;
struct TPowerRequest;
struct SPowerRequest;
struct SPowerResourceClientLevel;
struct SIdleResourceInfo;
class DPowerResourceController;

/**
@internalComponent
@prototype 9.5
Interface class for Resource Manager
Functions from PowerResourceManager calls corresponding functions of this 
class which in turn calls Powercontroller functions.
*/
class TInterface
	{
public:
    static TInt RegisterClient(TUint& aClientId, const TDesC8& aName, TOwnerType aType=EOwnerProcess);
    static TInt DeRegisterClient(TUint aClientId);
    static TInt GetClientName(TUint aClientId, TUint aTargetClientId, TDes8& aName);
    static TInt GetClientId(TUint aClientId, TDesC8& aClientName, TUint& aTargetClientId);
    static TInt GetResourceId(TUint aClientId, TDesC8& aResourceName, TUint& aResourceId);
    static TInt GetResourceInfo(TUint aClientId, TUint aResourceId, TAny* aInfo);
    static TInt GetNumResourcesInUseByClient(TUint aClientId, TUint aTargetClientId, TUint& aNumResources);
    static TInt GetInfoOnResourcesInUseByClient(TUint aClientId, TUint aTargetClientId, TUint& aNumResources, TAny* aInfo);
    static TInt GetNumClientsUsingResource(TUint aClientId, TUint aResourceId, TUint& aNumClients);
    static TInt GetInfoOnClientsUsingResource(TUint aClientId, TUint aResourceId, TUint& aNumClients, TAny* aInfo);
    static TInt AllocReserve(TUint aClientId, TUint8 aNumCl, TUint8 aNumRm);
    static TInt ChangeResourceState(TUint aClientId, TUint aResourceId, TInt aNewState, TPowerResourceCb* aCb=NULL);
    static TInt GetResourceState(TUint aClientId, TUint aResourceId, TBool aCached, TInt& aState, TInt& aLevelOwnerId);
    static TInt GetResourceState(TUint aClientId, TUint aResourceId, TBool aCached, TPowerResourceCb& aCb);
    static TInt CancelAsyncRequestCallBack(TUint aClientId, TUint aResourceId, TPowerResourceCb& aCb);
    static TInt RequestNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN);
    static TInt RequestNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN, TInt aThreshold, TBool aDirection);
    static TInt CancelNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN);
	static TInt DeRegisterClientLevelFromResource(TUint aClientId, TUint aResourceId);
    static DPowerResourceController* GetPowerResourceController(void);
    static TInt ControlIO(TUint aClientId, TUint aFunction, TAny* aParam1, TAny* aParam2, TAny* aParam3);
	};

/**
@internalComponent
@prototype 9.5
Container class to create containers of pointers to clients.
*/
template <class T>
class DResourceCon : public DBase
	{
public:
    inline TInt Initialise(TInt aInitialSize);
    inline void Delete();
    inline T*  operator[](TInt aIndex);
    inline TInt Remove(T* aObj, TInt aIndex);
    inline TInt Add(T* aObj, TUint &aId);
    inline TInt Find(T*& anEntry, TDesC& aName);
    inline TInt ReSize(TInt aGrowBy);
    inline TInt Count() {return iCount;}
    inline TInt Allocd() {return iAllocated;}
	inline TInt GrowBy() {return iGrowBy;}
private:
    TUint16 iGrowBy; //Size to grow the size of the array.
    TUint16 iAllocated;  //Size of the array
    TUint16 iCount; //Valid entries in the array
    TUint16 iInstanceCount; //FreeCounter incremented whenever an entry is added.
    TUint16 iFreeLoc; //Cached free location in the array
    TUint16 iSpare;
    T** iArray;
	};

/** 
@internalComponent
@prototype 9.5
Factory class for physical device 
*/
NONSHARABLE_CLASS(DResConPddFactory) : public DPhysicalDevice
	{
public:
	/**
	Structure for holding PDD capabilities information
	*/
	class TCaps
		{
	public:
		TVersion iVersion;
		};
public:
    DResConPddFactory();
    virtual TInt Install();
    virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
    virtual TInt Validate(TInt aUint, const TDesC8* anInfo, const TVersion& aVer);
    virtual void GetCaps(TDes8& aDes) const;
    inline static TVersion VersionRequired();
	};

/**
@internalComponent
@prototype 9.5
Interface class for user side resource controller proxy. For each user side channel opened an object of 
this class is created in heap and pointer to resource controller is stored in iController member variable. 
User side resource controller proxy calls the resource controller API's by deferencing the pointer. 
This class is required as when the channel is closed the device driver framework tries to delete 
the object stored in ipdd, because of which it is not possible to pass the controller pointer directly.
*/
class DUserSideProxyInterface: public DBase
	{
	public:
	DPowerResourceController *iController;
	};


/** 
@publishedPartner
@prototype 9.5
resource manager implementation base class
*/
NONSHARABLE_CLASS (DPowerResourceController) : public DBase
	{
public:
    TInt RegisterClient(TUint& aClientId, const TDesC8& aName, TOwnerType aType=EOwnerProcess);
    TInt DeRegisterClient(TUint aClientId);
    virtual TInt GetClientName(TUint aClientId, TUint aTargetClientId, TDes8& aName);
    virtual TInt GetClientId(TUint aClientId, TDesC8& aClientName, TUint& aTargetClientId);
    virtual TInt GetResourceId(TUint aClientId, TDesC8& aResourceName, TUint& aResourceId);
    virtual TInt GetResourceInfo(TUint aClientId, TUint aResourceId, TAny* aInfo);
    virtual TInt GetNumResourcesInUseByClient(TUint aClientId, TUint aTargetClientId, TUint& aNumResources);
    virtual TInt GetInfoOnResourcesInUseByClient(TUint aClientId, TUint aTargetClientId, TUint& aNumResources, TAny* aInfo);
    virtual TInt GetNumClientsUsingResource(TUint aClientId, TUint aResourceId, TUint& aNumClients);
    virtual TInt GetInfoOnClientsUsingResource(TUint aClientId, TUint aResourceId, TUint& aNumClients, TAny* aInfo);
    virtual TInt AllocReserve(TUint aClientId, TUint8 aNumCl, TUint8 aNumRm);
    virtual TInt ChangeResourceState(TUint aClientId, TUint aResourceId, TInt aNewState, TPowerResourceCb* aCb=NULL);
    virtual TInt GetResourceState(TUint aClientId, TUint aResourceId, TBool aCached, TInt& aState, TInt& aLevelOwnerId);
    virtual TInt GetResourceState(TUint aClientId, TUint aResourceId, TBool aCached, TPowerResourceCb& aCb);
    virtual TInt CancelAsyncRequestCallBack(TUint aClientId, TUint aResourceId, TPowerResourceCb& aCb);
    virtual TInt RequestNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN);
    virtual TInt RequestNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN, TInt aThreshold, TBool aDirection);
    virtual TInt CancelNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN);
    virtual TInt DeRegisterClientLevelFromResource(TUint aClientId, TUint aResourceId);
public:
	enum TResConPanic
		{
		ECalledFromDfcThread0 = 0,
		ECalledFromIsr = 1,
		ECalledFromNullThread = 2,
		ECalledFromDfcThread1 = 3,
		EClientHasPendingAsyncRequest = 4,
		EClientHasNotificationObject = 5,
		EControllerAlreadyExists = 6,
		ECustomFunctionNotSet = 7,
		EClientIdNotInClientLevelList = 8,
		ENoMemToCreatePowerControllerClient = 9,
		EResourceNameExceedsLimit = 10, 
		EObjectNotFoundInList = 11 
		};
#ifdef PRM_ENABLE_EXTENDED_VERSION	
	enum TExtendedResConPanic
		{
		EClosedLoopDependencies = EObjectNotFoundInList + 2, //13
		ERegisteringNonDependentStaticResource = 14,
		EClientHasDynamicResourceRegistered = 15,
		EDynamicResourceStillRegistered = 16,
		ERegisteringDependentStaticResourceWithHoles = 17
		};
#endif
	enum TResConStartSequence
		{
		EResConCreated,
		EResConInitialised,
		EResConStartupCompleted
		};
    //early initialization
    IMPORT_C static TInt InitController();
    TInt InitResources();
    //request a post-boot level for the resource
    IMPORT_C static TInt PostBootLevel(TUint aResId, TInt aLevel);
    //request registration of static resource
    IMPORT_C static TInt RegisterStaticResource(TUint aClientId, DStaticPowerResource* pR);
	//request registration of group/array of static resources
	IMPORT_C static TInt RegisterArrayOfStaticResources(TUint aClientId, DStaticPowerResource**& aStaticResourceArray, TUint aResCount);
    //registration for proxy client
    virtual TInt RegisterProxyClient(TUint& aProxyId, const TDesC8& aName);
    virtual TInt DeregisterProxyClient(TUint aClientId);
    //register list of resources whose state matter to Idle
    virtual TInt RegisterResourcesForIdle(TInt aPowerControllerId, TUint aNumResources, TPtr* aBuf);
    static void Panic(TUint8 aPanic);
    virtual TInt GetInterface(TUint aClientId, TUint aInterfaceId, TAny* aParam1, TAny* aParam2, TAny* aParam3);
	/**@internalComponent*/
	void CompleteNotifications(TInt aClientId, DStaticPowerResource* aResource, TInt aState, TInt aReturnCode, TInt aLevelOwnerId, TBool aLock = ETrue);
#ifdef PRM_ENABLE_EXTENDED_VERSION
	/**@internalComponent*/
	TInt ReserveClientLevelPoolCount(TUint16 aCount);
	/**@internalComponent*/
	void RemoveClientLevelFromPool(SPowerResourceClientLevel *&aLevelPtr);
	/**@internalComponent*/
	TInt HandleResourceChange(TPowerRequest &aRequest, TPropagation aProp, TUint aOriginatorId,
									const TDesC8& aOriginatorName, DStaticPowerResourceD* aResource);
#endif
protected:
    //generic layer function to be called by the PSL
    DPowerResourceController();
    void SetDfcQ(TDfcQue* aDfcQ);
	#ifdef PRM_ENABLE_EXTENDED_VERSION
    void SetDfcQDependency(TDfcQue* aDfcQ);
	#endif
    TInt InitPools(TUint16 aKClients, TUint16 aUClients, TUint16 aNClientLevels, TUint16 aNRequests);
	/* Lock the resource controller mutex */
	inline void Lock()	{ NKern::ThreadEnterCS();
						  Kern::MutexWait(*iResourceMutex); }
	inline void UnLock()	{ Kern::MutexSignal(*iResourceMutex);
							  NKern::ThreadLeaveCS();}
#ifdef PRM_ENABLE_EXTENDED_VERSION
	//Default implementation, PSL re-implements these if features supported
	virtual TInt DoRegisterStaticResourcesDependency(RPointerArray <DStaticPowerResourceD> & aStaticResourceDArray);
#endif
private:
    // pure virtual implemented by PSL - to be called by PIL
    virtual TInt DoInitController()=0;
    virtual TInt DoRegisterStaticResources(RPointerArray <DStaticPowerResource> & aStaticResourceArray)=0;
    /**@internalComponent*/
    TInt CheckLevelAndAddClient(SPowerResourceClient* pC, TPowerRequest* Request);
    static void MsgQFunc(TAny* aPtr);
    #ifdef PRM_ENABLE_EXTENDED_VERSION
    static void MsgQDependencyFunc(TAny* aPtr);
    #endif
    
	/**@internalComponent*/
	void ResourceStateChangeOfClientLevels(SPowerResourceClient* pC);
	/**@internalComponent*/
    void HandleMsg(TPowerRequest& aRequest);
	#ifdef PRM_ENABLE_EXTENDED_VERSION
    /**@internalComponent*/
    void HandleDependencyMsg(TPowerRequest& aRequest);
	#endif
	/**@internalComponent*/
    void CompleteRequest(TPowerRequest& aRequest);
	/**@internalComponent*/
    void MoveRequestToFreePool(TPowerRequest *aReq);
	/**@internalComponent*/
	TInt HandleReservationOfObjects(TPowerRequest& aRequest);
	/**@internalComponent*/
	TInt HandleClientRegistration(TPowerRequest& aRequest);
#ifdef PRM_ENABLE_EXTENDED_VERSION
	TInt RegisterDynamicResource(SPowerResourceClient* aClientPtr, DDynamicPowerResource* aPDRes, TUint* aDynamicResourceId);
	TInt DeregisterDynamicResource(SPowerResourceClient* aClientPtr, TUint aDynamicResourceId, TInt* aPDefLevel);
	TInt RegisterResourceDependency(SPowerResourceClient* aClientPtr, SResourceDependencyInfo* aInfo1, SResourceDependencyInfo* aInfo2);
	/**@internalComponent*/
	void CheckForDependencyLoop(DStaticPowerResourceD* pR, TUint aParentResId, TUint aTargetResId);
	TInt DeregisterResourceDependency(SPowerResourceClient* aClientPtr, TUint aResId1, TUint aResId2);
	/**@internalComponent*/
	TInt HandleDependencyResourceStateChange(SPowerResourceClient* pC, TPowerRequest& aRequest);
	TInt GetNumDependentsForResource(TUint aResourceId, TUint* aNumResources);
	TInt GetDependentsIdForResource(TUint aResourceId, TAny* aInfo, TUint* aNumDepResources);
	TInt HandleResourceRegistration(TPowerRequest& aReq);
#endif
protected:
	DMutex* iResourceMutex;
	TDfcQue* iDfcQ;
    TMessageQue *iMsgQ;
#ifdef PRM_ENABLE_EXTENDED_VERSION
	TDfcQue* iDfcQDependency;
	TMessageQue* iMsgQDependency;
	TBool iDfcQDependencyLock;
#endif
	RPointerArray <DStaticPowerResource> iStaticResourceArray;
    DResourceCon<SPowerResourceClient> iClientList;
    DResourceCon<SPowerResourceClient> iUserSideClientList;
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
	RPointerArray<SPowerResourceClient> iCleanList;
#endif
    SPowerResourceClient* iClientPool;
    SPowerRequest* iRequestPool;
    SPowerResourceClientLevel* iClientLevelPool;
	TUint iPowerControllerId; //Stores the ID allocated to PowerController
    SIdleResourceInfo* iListForIdle;
    TUint iInitialised;
    TUint16 iClientCount;
    TUint16 iUserSideClientCount;
    TUint16 iClientLevelPoolCount;
    TUint16 iClientLevelPoolGrowBy;
    TUint16 iRequestPoolCount;
    TUint16 iRequestPoolGrowBy;
	TUint16 iStaticResourceCount;  //Actual number of static resources registered (valid entries).
	TUint	iReserved2; //Reserved for future use
#ifdef PRM_ENABLE_EXTENDED_VERSION
	DResourceCon<DDynamicPowerResource>   iDynamicResourceList;
	DResourceCon<DDynamicPowerResourceD>  iDynamicResDependencyList;
	RPointerArray <DStaticPowerResourceD> iStaticResDependencyArray;
	SPowerResourceClientLevel* iResourceLevelPool;
	TUint16 iResourceLevelPoolCount;
	TUint16 iDynamicResourceCount;
	TUint8  iDynamicResDependencyCount;
	TUint8  iSpare1;
	TUint16 iSpare2;
	TUint  iReserved3; //Reserved for future use.
#endif
	};

/**
@publishedPartner
@prototype 9.5
power level of client in a shared resource
*/
struct SPowerResourceClientLevel : public SDblQueLink
	{
    TUint iClientId;
    TUint iResourceId;
    TInt iLevel;
    SPowerResourceClientLevel* iNextInList;
	};

/**
@internalComponent 
@prototype 9.5
respresent client in resource manager
*/
struct SPowerResourceClient
	{
    TUint iClientId;
    const TDesC8* iName;
    SPowerResourceClient* iNextInList;
    SPowerResourceClientLevel* iLevelList;
    DPowerResourceNotification* iNotificationList;
    TUint8 iReservedCl;
    TUint8 iReservedRm;
    TUint8 iPendingReqCount;
    TUint8 iUnderFlowRmCount;
    TUint8 iUnderFlowClCount;
	TUint8 iDynamicResCount; //Counter for dynamic resource registered by the client. Used only in extended version
	TUint8 iSpare1;
	TUint8 iSpare2;
    union
       {
       TUint iThreadId;
       TAny* iSpare3;
       };
	};

/**
@publishedPartner
@prototype 9.5
represents a request inside the resource manager
*/
struct TPowerRequest : public TThreadMessage
	{
    /** requests can either be to get the resource value or to change the resource value*/
    enum TReqType {EGet, EChange, ESetDefaultLevel, ERegisterKernelClient, ERegisterUsersideClient, EAllocReserve,
					ERegisterDynamicResource	};
    /** @return thread's own message and turn into a power request. Used for sync/instant calls*/
    inline static TPowerRequest& Get()
            {return (TPowerRequest&)Kern::Message();}
    /** @return type of request get or set */
    inline TReqType& ReqType()		// one of TReqType
            {return *(TReqType*)&iValue;}
    /** @return resource id which is being requested*/
    inline TUint& ResourceId()
            {return *(TUint*)&iArg[0];}
    /** @return id of client making request (only valid on change requests)*/
    inline TInt& ClientId()
            {return *(TInt*)&iArg[1];}
	/**
	    On resource state change operations the PIL sets this field with the required level before 
		invoking the DoRequest(..) function; on return from DoRequest(..) function the PSL sets this field
		with the real state of the resource to be cached by the PIL.On resource state read operations PSL
		sets it with the level read.
	*/
	inline TInt& Level()		
            {return *(TInt*)&iArg[2];}
    /** @return pointer the resource being requested */
    inline DStaticPowerResource*& Resource()
            {return *(DStaticPowerResource**)&iArg[3];}
    /** @return pointer to resource callback structure, used for async requests */
    inline TPowerResourceCb*& ResourceCb()
            {return *(TPowerResourceCb**)&iArg[4];}
    /** @return return code of resource's DoRequest function when request has been processed */
    inline TInt& ReturnCode()
            {return *(TInt*)&iArg[5];}
    /** @return return ETrue if a change is required on a shared resource */
    inline TBool& RequiresChange()
            {return *(TInt*)&iArg[6];}
	/** @return number of client level objects requested by a client to reserve */
	inline TInt& ClientLevelCount()
			{return *(TInt*)&iArg[7];}
	/** @return number of request objects requested by a client to reserve */
	inline TInt& RequestCount()
			{return *(TInt*)&iArg[8];}
	};

/**
@internalComponent
@prototype 9.5
*/
struct SPowerRequest
	{
    TPowerRequest iRequest;
    SPowerRequest* iNext;
	};

/**
@publishedPartner
@prototype 9.5
Structure representing resource information used for Idle power management
*/
struct SIdleResourceInfo
	{
    TUint iResourceId; 
    TInt iLevelOwnerId; //Owner of the resource.  
    TInt iCurrentLevel; //Cached resource state
	TInt iReserved1;	//Reserved for future use.
	TInt iReserved2;	//Reserved for future use.
	TInt iReserved3;	//Reserved for future use.
	};

#include <drivers/resourcecontrol.inl>

#endif //__RESOURCECONTROL_H__


