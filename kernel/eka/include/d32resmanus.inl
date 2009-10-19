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
// e32\include\d32resmanus.inl
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __KERNEL_H__

#ifdef RESOURCE_MANAGER_SIMULATED_PSL
#ifdef PRM_ENABLE_EXTENDED_VERSION2
_LIT(KPddName, "resourcecontrollerextendedcore.pdd");
#elif defined(PRM_ENABLE_EXTENDED_VERSION)
_LIT(KPddName,"resourcecontrollerextended.pdd");	// To support testing of the Extended version of the PDD
#else
_LIT(KPddName,"resourcecontroller.pdd");
#endif
#else
#ifdef PRM_ENABLE_EXTENDED_VERSION2
_LIT(KPddName, "resmanextendedcore.pdd");
#elif defined(PRM_ENABLE_EXTENDED_VERSION)
_LIT(KPddName, "resmanextended.pdd");			// To enable operation with the extended version of the PDD
#else
_LIT(KPddName, "ResMan.pdd");				// To enable operation with the basic PDD
#endif
#endif

inline TInt RBusDevResManUs::Open(TDesC8& aClientName)
	 {return(DoCreate(KLddRootName,VersionRequired(),-1,&KPddName,(TDesC8*)&(aClientName),EOwnerThread));}

inline TVersion RBusDevResManUs::VersionRequired() const
	{return(TVersion(EMajorVersionNumber,EMinorVersionNumber,EBuildVersionNumber));}

inline TInt RBusDevResManUs::Initialise(const TUint8 aNumGetStateRes, const TUint8 aNumSetStateRes, const TUint8 aNumListenableRes)
	{TUint8 stateRes[3]; \
	stateRes[0]=aNumGetStateRes; \
	stateRes[1]=aNumSetStateRes; \
	stateRes[2]=aNumListenableRes; \
	TInt r= DoControl(EInitialise,(TAny *)&stateRes); \
	return r;}

	// Synchronous methods
inline TInt RBusDevResManUs::GetNoOfResources(TUint& aNumResources, const TBool aInfoRead)
	{return(DoControl(EGetNoOfResources,(TAny*)&aNumResources, (TAny*)aInfoRead));}

inline TInt RBusDevResManUs::GetAllResourcesInfo(RSimplePointerArray<TResourceInfoBuf>* aInfoPtrs, TUint& aNumResources, const TBool aRefresh)
	{TUint* parms[2]; \
	parms[0]=&aNumResources; \
	parms[1]=(TUint*)aRefresh; \
	return(DoControl(EGetAllResourcesInfo,(TAny*)aInfoPtrs,(TAny*)&parms));}

inline TInt RBusDevResManUs::GetNoOfClients(TUint& aNumClients, const TBool aIncludeKern, const TBool aInfoRead)
	{TUint parms[3]; \
	parms[0]=(TUint)aIncludeKern; \
	parms[1]=0; /* 0 represents all clients */ \
	parms[2]=(TUint)aInfoRead; \
	return(DoControl(EGetNoOfClients,(TAny*)&aNumClients,(TAny*)&parms));}

inline TInt RBusDevResManUs::GetNamesAllClients(RSimplePointerArray<TClientName>* aInfoPtrs, TUint& aNumClients, const TBool aIncludeKern, const TBool aRefresh)
	{TUint* parms[4]; \
	parms[0]=&aNumClients; \
	parms[1]=(TUint*)aIncludeKern; \
	parms[2]=(TUint*)0; /* 0 represents all clients */ \
	parms[3]=(TUint*)aRefresh; \
	return(DoControl(EGetNamesAllClients,(TAny*)aInfoPtrs,(TAny*)&parms));}

inline TInt RBusDevResManUs::GetNumClientsUsingResource(const TUint aResourceId, TUint& aNumClients, const TBool aIncludeKern, const TBool aInfoRead)
	{TUint parms[3]; \
	parms[0]=(TUint)aIncludeKern; \
	parms[1]=aResourceId; \
	parms[2]=(TUint)aInfoRead; \
	return(DoControl(EGetNumClientsUsingResource,(TAny*)&aNumClients,(TAny*)&parms));}

inline TInt RBusDevResManUs::GetInfoOnClientsUsingResource(const TUint aResourceId, TUint& aNumClients, RSimplePointerArray<TClientInfoBuf>* aInfoPtrs, const TBool aIncludeKern, const TBool aRefresh)
	{TUint* parms[4]; \
	parms[0]=&aNumClients; \
	parms[1]=(TUint*)aIncludeKern; \
	parms[2]=(TUint*)aResourceId; \
	parms[3]=(TUint*)aRefresh; \
	return(DoControl(EGetInfoOnClientsUsingResource,(TAny*)aInfoPtrs,(TAny*)&parms));}

inline TInt RBusDevResManUs::GetNumResourcesInUseByClient(TDesC8& aClientName, TUint &aNumResources,const TBool aInfoRead)
	{TUint* parms[2]; \
	parms[0]=&aNumResources; \
	parms[1]=(TUint*)aInfoRead; \
	return(DoControl(EGetNumResourcesInUseByClient,(TAny*)&aClientName,(TAny*)&parms));}

inline TInt RBusDevResManUs::GetInfoOnResourcesInUseByClient(TDesC8& aClientName, TUint &aNumResources, RSimplePointerArray<TResourceInfoBuf>* aInfoPtrs, const TBool aRefresh)
	{TUint* parms[3]; \
	parms[0]=&aNumResources; \
	parms[1]=(TUint*)aInfoPtrs; \
	parms[2]=(TUint*)aRefresh; \
	return(DoControl(EGetInfoOnResourcesInUseByClient,(TAny*)&aClientName,(TAny*)&parms));}

inline TInt RBusDevResManUs::GetResourceIdByName(TDesC8& aResourceName, TUint& aResourceId)
	{return(DoControl(EGetResourceIdByName,(TAny*)&aResourceName,(TAny*)&aResourceId));}

inline TInt RBusDevResManUs::GetResourceInfo(const TUint aResourceId, TResourceInfoBuf* aInfo)
	{return(DoControl(EGetResourceInfo,(TAny*)aResourceId,aInfo));}

inline TInt RBusDevResManUs::GetResourceControllerVersion(TUint& aVer)
	{return(DoControl(EGetResourceControllerVersion,(TAny*)&aVer,NULL));}

inline TInt RBusDevResManUs::GetNumDependentsForResource(const TUint aResourceId, TUint* aNumDependents, const TBool aInfoRead)
	{TUint parms[2]; \
	parms[0]=aResourceId; \
	parms[1]=(TUint)aInfoRead; \
	return(DoControl(EGetNumDependentsForResource,(TAny*)aNumDependents,(TAny*)&parms));}

inline TInt RBusDevResManUs::GetDependentsIdForResource(const TUint aResourceId, TDes8& aResIdArray, TUint* aNumDepResources, const TBool aRefresh)
	{
	TUint parms[3]; \
	parms[0]=aResourceId; \
	parms[1]=(TUint)(&aResIdArray); \
	parms[2]=(TUint)aRefresh; \
	return(DoControl(EGetDependentsIdForResource,(TAny*)aNumDepResources,(TAny*)&parms));}

	//Asynchronous methods
inline void RBusDevResManUs::ChangeResourceState(TRequestStatus& aStatus, const TUint aResourceId, const TInt aNewState)
	{DoRequest(EChangeResourceState,aStatus,(TAny*)aResourceId,(TAny*)aNewState);}

inline void RBusDevResManUs::GetResourceState(TRequestStatus& aStatus, const TUint aResourceId, const TBool aCached, TInt* aState, TInt *aLevelOwnerId)
	{TInt* parms[3]; \
	parms[0]=(TInt*)aCached; \
	parms[1]=aState; \
	parms[2]=aLevelOwnerId; \
	DoRequest(EGetResourceState,aStatus,(TAny*)aResourceId,(TAny*)&parms);}

inline void RBusDevResManUs::RequestNotification(TRequestStatus& aStatus, const TUint aResourceId)
	{DoRequest(ERequestChangeNotification,aStatus,(TAny*)aResourceId);}

inline void RBusDevResManUs::RequestNotification(TRequestStatus& aStatus, const TUint aResourceId, 
											const TInt aThreshold, const TBool aDirection)
	{TUint parms[2]; \
	parms[0]=aResourceId; \
	parms[1]=(TUint)aDirection; \
	DoRequest(ERequestQualifiedChangeNotification, aStatus, (TAny*)aThreshold, (TAny*)(&parms[0]));}

	// Specific request cancellation functions
inline TInt RBusDevResManUs::CancelChangeResourceState (TRequestStatus& aStatus)
	{return(DoControl(ECancelChangeResourceState,(TAny*)&aStatus));}

inline TInt RBusDevResManUs::CancelGetResourceState(TRequestStatus& aStatus)
	{return(DoControl(ECancelGetResourceState,(TAny*)&aStatus));}

inline TInt RBusDevResManUs::CancelRequestNotification(TRequestStatus& aStatus)
	{return(DoControl(ECancelRequestChangeNotification,(TAny*)&aStatus));}

	// Resource-specific request cancellation functions
inline TInt RBusDevResManUs::CancelChangeResourceStateRequests (const TUint aResourceId)
	{return(DoControl(ECancelChangeResourceStateRequests,(TAny*)aResourceId));}

inline TInt RBusDevResManUs::CancelGetResourceStateRequests(const TUint aResourceId)
	{return(DoControl(ECancelGetResourceStateRequests,(TAny*)aResourceId));}

inline TInt RBusDevResManUs::CancelNotificationRequests(const TUint aResourceId)
	{return(DoControl(ECancelChangeNotificationRequests,(TAny*)aResourceId));}

	// Generic Cancel function
inline void RBusDevResManUs::CancelAsyncOperation(TRequestStatus* aStatus)
	{DoCancel((TInt)aStatus);}


#ifdef RESOURCE_MANAGER_SIMULATED_PSL
	// Requests to support testing
inline	TInt RBusDevResManUs::GetNumCandidateAsyncResources(TUint& aNumResources)
	{return(DoControl(EGetNumCandidateAsyncResources,(TAny*)&aNumResources));}

inline	TInt RBusDevResManUs::GetCandidateAsyncResourceId(TUint aIndex, TUint& aResourceId)
	{return(DoControl(EGetCandidateAsyncResourceId,(TAny*)aIndex,(TAny*)&aResourceId));}

inline	TInt RBusDevResManUs::GetNumCandidateSharedResources(TUint& aNumResources)
	{return(DoControl(EGetNumCandidateSharedResources,(TAny*)&aNumResources));}

inline	TInt RBusDevResManUs::GetCandidateSharedResourceId(TUint aIndex, TUint& aResourceId)
	{return(DoControl(EGetCandidateSharedResourceId,(TAny*)aIndex,(TAny*)&aResourceId));}

#endif

#endif
