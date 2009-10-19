// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\d32resmanus.h
// 
//

/**
 @file
 @publishedPartner
 @released
*/


#ifndef __D32RESMANUS_H__
#define __D32RESMANUS_H__
#include <e32cmn.h>
#include <e32ver.h>

#include <drivers/resource_category.h>

// Name lengths repesent byte length (2 bytes per character for Unicode)
#define MAX_RESOURCE_NAME_LENGTH	32 // 8-bit operation
#define MAX_NAME_LENGTH_IN_RESMAN	32 // Maximum length of 8-bit name in Resource Controller
#define MAX_CLIENT_NAME_LENGTH		256 // 8-bit operation

#ifdef RESOURCE_MANAGER_SIMULATED_PSL
#define LEVEL_GAP_REQUIRED_FOR_ASYNC_TESTING 10  // Semi-arbitrary
#define LEVEL_GAP_REQUIRED_FOR_SHARED_TESTING 3  // Semi-arbitrary
#endif

//Structure to pass the dependency information
#ifndef __KERNEL_MODE__
struct SResourceDependencyInfo
	{
	TUint iResourceId;
	TUint8 iDependencyPriority;
	};
#endif

struct TCapsDevResManUs
    {
    /**
     * The device version
     */
    TVersion version;
    };

struct TResourceInfo
	{
	TBuf8<MAX_RESOURCE_NAME_LENGTH>iName;
	TUint iId;
	TResourceClass iClass;
	TResourceType iType;
	TResourceUsage iUsage;
	TResourceSense iSense;
	TInt iMinLevel;
	TInt iMaxLevel;
	};
typedef TPckgBuf<TResourceInfo> TResourceInfoBuf;


typedef TBuf8<MAX_NAME_LENGTH_IN_RESMAN> TClientName;

struct TClientInfo
	{
	TClientName iName;
	TUint iId;
	};

typedef TPckgBuf<TClientInfo> TClientInfoBuf;

template <class T>
class RSimplePointerArray : private RPointerArrayBase
	{
	public:
	inline RSimplePointerArray();
	inline explicit RSimplePointerArray(TInt aGranularity);

	inline void Close() {RPointerArrayBase::Close();};
	inline TInt Count() const {return RPointerArrayBase::Count();};
	inline T* const& operator[](TInt anIndex) const {return (T* const&)At(anIndex);};
	inline T*& operator[](TInt anIndex) {return (T*&)At(anIndex);};
	inline TInt Append(const T* anEntry) {return RPointerArrayBase::Append(anEntry);};
	inline TInt Insert(const T* anEntry, TInt aPos) {return RPointerArrayBase::Insert(anEntry,aPos);};
	inline void Remove(TInt anIndex){RPointerArrayBase::Remove(anIndex);};

	inline T** Entries() {return (T**)(RPointerArrayBase::Entries());};
	};

template <class T>
inline RSimplePointerArray<T>::RSimplePointerArray()
	: RPointerArrayBase()
	{}

template <class T>
inline RSimplePointerArray<T>::RSimplePointerArray(TInt aGranularity)
     : RPointerArrayBase(aGranularity)
     {}


#ifdef RESOURCE_MANAGER_SIMULATED_PSL
#ifdef PRM_ENABLE_EXTENDED_VERSION2
	_LIT(KLddRootName, "resourcecontrollerextendedcore");
#elif defined (PRM_ENABLE_EXTENDED_VERSION)
	_LIT(KLddRootName,"resourcecontrollerextended"); // To support testing of the Extended version
#else
	_LIT(KLddRootName,"resourcecontroller");		 // To support testing of the basic version
#endif
#else
#ifdef PRM_ENABLE_EXTENDED_VERSION2
	_LIT(KLddRootName, "resmanextendedcore");
	_LIT(KLddName, "resmanextendedcore.ldd");
#elif defined(PRM_ENABLE_EXTENDED_VERSION)
	_LIT(KLddRootName,"resmanextended");					// To enable operation with both the basic and Extended PDD 
	_LIT(KLddName, "resmanextended.ldd");					// LDD will support the Extended funcitonality
#else
	_LIT(KLddRootName,"ResMan");					// To enable operation with both the basic and Extended PDD 
	_LIT(KLddName, "ResMan.ldd");					// LDD will support the Extended funcitonality
#endif
#endif

class RBusDevResManUs : public RBusLogicalChannel
	{
	enum TVer {EMajorVersionNumber=1,EMinorVersionNumber=0,EBuildVersionNumber=KE32BuildVersionNumber};

	public:
    /**
     * Control requests
     */
    enum TControl
		{
		EInitialise,						/**< Specify resource requirement and register			*/
        EGetNoOfResources,					/**< Read the total number of resources					*/
        EGetAllResourcesInfo,				/**< Read the info for all resources into a buffer		*/
        EGetNoOfClients,					/**< Read the total number of (Rsource Manager) clients	*/
        EGetNamesAllClients,				/**< Read the names of all clients into a buffer		*/
        EGetNumClientsUsingResource,		/**< Read the number of clients for specified resource  */
        EGetInfoOnClientsUsingResource,		/**< Read the info for clients for specified resource	*/
        EGetNumResourcesInUseByClient,		/**< Read the number of resources registered by a client*/
        EGetInfoOnResourcesInUseByClient,	/**< Read the info for all resources registered by the 
												 named client into a buffer							*/
        EGetResourceIdByName,				/**< Read the ID for a named resource					*/
        EGetResourceInfo,					/**< Read the info for specified resource to a buffer	*/
        ECancelChangeResourceStateRequests,	/**< Cancel all change state requests for a resource	*/
        ECancelGetResourceStateRequests,	/**< Cancel all get state requests for a resource		*/
        ECancelChangeNotificationRequests,	/**< Cancel all notification requests for a resource	*/
        ECancelChangeResourceState,			/**< Cancel a specific change state request				*/
        ECancelGetResourceState,			/**< Cancel a specific get state request				*/
        ECancelRequestChangeNotification,	/**< Cancel a specific notification request				*/
		EGetResourceControllerVersion,		/**< Read the version of the Resource Controller		*/
		EGetNumDependentsForResource,		/**< Read the number of dependents for a resource		*/
		EGetDependentsIdForResource			/**< Read the dependency information for a resource		*/
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
		,
		// Requests to support testing
		EGetNumCandidateAsyncResources,
		EGetCandidateAsyncResourceId,
		EGetNumCandidateSharedResources,
		EGetCandidateSharedResourceId
#endif
		};

    /**
     * Asynchronous requests
     */
    enum TRequest
		{
		EChangeResourceState,				/**< Change the state of a resource							*/
        EGetResourceState,					/**< Get the state of a resource							*/
        ERequestChangeNotification,			/**< Register for resource change notifications				*/
        ERequestQualifiedChangeNotification /**< Register for qualified resource change notifications	*/
		};

#ifndef __KERNEL_MODE__
    public:

	inline TInt Open(TDesC8& aClientName);
	inline TInt Initialise(const TUint8 aNumGetStateRes, const TUint8 aNumSetStateRes, const TUint8 aNumListenableRes);
	inline TVersion VersionRequired() const;

	// Synchronous requests
	inline TInt GetNoOfResources(TUint& aNumResources, const TBool aInfoRead=ETrue);	
	inline TInt GetAllResourcesInfo(RSimplePointerArray<TResourceInfoBuf>* aInfoPtrs, TUint& aNumResources, const TBool aRefresh=EFalse);
	inline TInt GetNoOfClients(TUint& aNumClients,const TBool aIncludeKern,const TBool aInfoRead=ETrue);
	inline TInt GetNamesAllClients(RSimplePointerArray<TClientName>* aInfoPtrs, TUint& aNumClients, const TBool aIncludeKern, const TBool aRefresh=EFalse);
	inline TInt GetNumClientsUsingResource(const TUint aResourceId, TUint& aNumClients, const TBool aIncludeKern,const TBool aInfoRead=ETrue);
	inline TInt GetInfoOnClientsUsingResource(const TUint aResourceId, TUint& aNumClients, RSimplePointerArray<TClientInfoBuf>* aInfoPtrs, const TBool aIncludeKern, const TBool aRefresh=EFalse);
	inline TInt GetNumResourcesInUseByClient(TDesC8& aClientName, TUint &aNumResources,const TBool aInfoRead=ETrue);
	inline TInt GetInfoOnResourcesInUseByClient(TDesC8& aClientName, TUint &aNumResources, RSimplePointerArray<TResourceInfoBuf>* aInfoPtrs, const TBool aRefresh=EFalse);

	inline TInt GetResourceIdByName(TDesC8& aResourceName, TUint& aResourceId);
	inline TInt GetResourceInfo(const TUint aResourceId, TResourceInfoBuf* aInfo);
	inline TInt GetResourceControllerVersion(TUint& aVer);
	inline TInt GetNumDependentsForResource(const TUint aResourceId, TUint* aNumDependents, const TBool aInfoRead=ETrue);
	inline TInt GetDependentsIdForResource(const TUint aResourceId, TDes8& aResIdArray, TUint* aNumDepResources, const TBool aRefresh=EFalse);
	// Asynchronous requests
	inline void ChangeResourceState(TRequestStatus& aStatus, const TUint aResourceId, const TInt aNewState);
	inline void GetResourceState(TRequestStatus& aStatus, const TUint aResourceId, const TBool aCached, TInt* aState, TInt *aLevelOwnerId);
	inline void RequestNotification(TRequestStatus& aStatus, const TUint aResourceId);
	inline void RequestNotification(TRequestStatus& aStatus, const TUint aResourceId, const TInt aThreshold, const TBool aDirection);

	// Cancel requests
	inline TInt CancelChangeResourceStateRequests(const TUint aResourceId);
	inline TInt CancelGetResourceStateRequests(const TUint aResourceId);
	inline TInt CancelNotificationRequests(const TUint aResourceId);
	//
	inline TInt CancelChangeResourceState(TRequestStatus& aStatus);
	inline TInt CancelGetResourceState(TRequestStatus& aStatus);
	inline TInt CancelRequestNotification(TRequestStatus& aStatus);
	//
	inline void CancelAsyncOperation(TRequestStatus* aStatus);

#ifdef RESOURCE_MANAGER_SIMULATED_PSL
	// Requests to support testing
	inline	TInt GetNumCandidateAsyncResources(TUint& aNumResources);
	inline	TInt GetCandidateAsyncResourceId(TUint aIndex, TUint& aResourceId);
	inline	TInt GetNumCandidateSharedResources(TUint& aNumResources);
	inline	TInt GetCandidateSharedResourceId(TUint aIndex, TUint& aResourceId);
#endif

#endif
	};


#ifndef __KERNEL_MODE__
#include <d32resmanus.inl>
#endif


#endif
