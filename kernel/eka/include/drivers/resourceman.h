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
// e32\include\drivers\resourceman.h
// 
//

/**
 @file
 @publishedPartner
 @released 9.5
*/
#ifndef __RESOURCEMAN_H__
#define __RESOURCEMAN_H__

#ifdef PRM_ENABLE_EXTENDED_VERSION
#include <drivers/resource_extend.h>
#else
#include <drivers/resource.h>
#endif

static const TUint KResManControlIoGetVersion=0x0;
static const TUint KResManControlIoRegisterDynamicResource=0x1;
static const TUint KResManControlIoDeregisterDynamicResource=0x2;
static const TUint KResManControlIoRegisterDependency=0x3;
static const TUint KResManControlIoDeregisterDependency=0x4;
static const TUint KResManControlIoGetNumDependents=0x5;
static const TUint KResManControlIoGetDependentsId=0x6;

//Resource controller versions
static const TUint KResControllerBasicVersion=0x0;
static const TUint KResControllerExtendedVersion=0x01;

/**
Resource Manager API class
API's are exported from this class to kernel side components
*/
NONSHARABLE_CLASS (PowerResourceManager)
	{
public:
    IMPORT_C static TInt RegisterClient(TUint& aClientId, const TDesC8& aName, TOwnerType aType=EOwnerProcess);
    IMPORT_C static TInt DeRegisterClient(TUint aClientId);
    IMPORT_C static TInt GetClientName(TUint aClientId, TUint aTargetClientId, TDes8& aName);
    IMPORT_C static TInt GetClientId(TUint aClientId, TDesC8& aClientName, TUint& aTargetClientId);
    IMPORT_C static TInt GetResourceId(TUint aClientId, TDesC8& aResourceName, TUint& aResourceId);
    IMPORT_C static TInt GetResourceInfo(TUint aClientId, TUint aResourceId, TAny* aInfo);
    IMPORT_C static TInt GetNumResourcesInUseByClient(TUint aClientId, TUint aTargetClientId, TUint& aNumResources);
    IMPORT_C static TInt GetInfoOnResourcesInUseByClient(TUint aClientId, TUint aTargetClientId, TUint& aNumResources, TAny* aInfo);
    IMPORT_C static TInt GetNumClientsUsingResource(TUint aClientId, TUint aResourceId, TUint& aNumClients);
    IMPORT_C static TInt GetInfoOnClientsUsingResource(TUint aClientId, TUint aResourceId, TUint& aNumClients, TAny* aInfo);
    IMPORT_C static TInt AllocReserve(TUint aClientId, TUint8 aNumCl, TUint8 aNumRm);
    IMPORT_C static TInt ChangeResourceState(TUint aClientId, TUint aResourceId, TInt aNewState, TPowerResourceCb* aCb=NULL);
    IMPORT_C static TInt GetResourceState(TUint aClientId, TUint aResourceId, TBool aCached, TInt& aState, TInt& aLevelOwnerId);
    IMPORT_C static TInt GetResourceState(TUint aClientId, TUint aResourceId, TBool aCached, TPowerResourceCb& aCb);
    IMPORT_C static TInt CancelAsyncRequestCallBack(TUint aClientId, TUint aResourceId, TPowerResourceCb& aCb);
    IMPORT_C static TInt RequestNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN);
    IMPORT_C static TInt RequestNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN, TInt aThreshold, TBool aDirection);
    IMPORT_C static TInt CancelNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN);
	IMPORT_C static TInt DeRegisterClientLevelFromResource(TUint aClientId, TUint aResourceId);
    IMPORT_C static TInt ControlIO(TUint aClientId, TUint aFunction, TAny* aParam1, TAny* aParam2, TAny* aParam3 = NULL);
	inline static TInt GetResourceControllerVersion(TUint aClientId, TUint& aVersion);
#ifdef PRM_ENABLE_EXTENDED_VERSION
	inline static TInt RegisterDynamicResource(TUint aClientId, DDynamicPowerResource* aResource, TUint& aResourceId);
	inline static TInt DeRegisterDynamicResource(TUint aClientId, TUint aResourceId, TInt* aState);
	inline static TInt RegisterResourceDependency(TUint aClientId, SResourceDependencyInfo* aResDependecyInfo1, SResourceDependencyInfo* aResDependencyInfo2);
	inline static TInt DeRegisterResourceDependency(TUint aClientId, TUint aResourceId1, TUint aResourceId2);
	inline static TInt GetNumDependentsForResource(TUint aClientId, TUint aResourceId, TUint& aNumDepResource);
	inline static TInt GetDependentsIdForResource(TUint aClientId, TUint aResourceId, TAny* aResIdArray, TUint& aNumDepResources);
#endif
	};    

#include <drivers/resourceman.inl>

#endif //__RESOURCEMAN_H__
