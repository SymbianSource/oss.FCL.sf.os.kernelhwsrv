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
// e32test\resourceman\d_rescontrolcli.h
// 
//

#ifndef __D_RESCONTROLCLI_H__
#define __D_RESCONTROLCLI_H__

#include <e32cmn.h>
#include <e32ver.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif   

#ifndef __KERNEL_MODE__
//Structure to pass the dependency information
struct SResourceDependencyInfo
	{
	TUint iResourceId;
	TUint8 iDependencyPriority;
	};
#endif

#define MAX_CLIENTS 50 //Maximum clients allowed

#ifndef PRM_ENABLE_EXTENDED_VERSION
_LIT(KLddName, "D_RESCONTROLCLI.LDD");
#else
_LIT(KLddName, "D_EXTENDEDRESCONTROLCLI.LDD");
#endif

/** Struture for passing information between user and kernel side.*/
struct TParameterListInfo
	{
	TUint iClientId;
	TAny* iPtr1;
	TAny* iPtr2;
	TAny* iPtr3;
	TAny* iPtr4;
	TAny* iPtr5;
	};

/** User side logical channel */
class RTestResMan : public RBusLogicalChannel
	{
public:
	// Structure for holding driver capabilities information
	class TCaps
		{
	public:
		TVersion iVersion;
		};

private:
	enum TControl //Request types for synchronous operation.
		{ 
		ERegisterClient,
		EDeRegisterClient,
		EGetClientName,
		EGetClientId,
		EGetResourceId,
		EGetResourceInfo,
		EGetNumResourcesInUseByClient,
		EGetInfoOnResourcesInUseByClient,
		EGetNumClientsUsingResource,
		EGetInfoOnClientsUsingResource,
		EAllocReserve,
		ECheckNotifications,
		EChangeResourceStateSync,
		EGetResourceStateSync,
		ERegisterForIdleResourcesInfo,
		EGetIdleResourcesInfo,
		EDeRegisterClientLevelFromResource,
		ERequestNotificationCond,
		ERequestNotificationUncond,
		ECheckPostBootLevelNotifications,
		ECancelNotification, 
		EGetControllerVersion,
		ERegisterResourceController,
#ifdef PRM_ENABLE_EXTENDED_VERSION
		ERegisterDynamicResource,
		EDeRegisterDynamicResource,
		ERegisterResourceDependency,
		EDeRegisterResourceDependency,
		EGetNumDependentsForResource,
		EGetDependentsIdForResource,
#endif //PRM_ENABLE_EXTENDED_VERSION
		EMaxControl,
		};
	enum TRequest //Request types for asynchronous operation
		{
		EChangeResourceStateAsync = EMaxControl + 1,
	    EGetResourceStateAsync, 
#ifdef PRM_ENABLE_EXTENDED_VERSION
		EChangeResStateAndDeregisterDynamicRes,
		ECheckParallelExecutionForChangeResState
#endif
		};
	friend class DTestResManLdd;
public:
   	TInt Open();
    TInt RegisterClient(TUint& aClientId, const TDesC* aClientName, TOwnerType aType=EOwnerProcess);
    TInt DeRegisterClient(TUint aClientId);
    TInt GetClientName(TUint aClientId, TUint aTargetClientId, TDes8* aClientName);
    TInt GetClientId(TUint aClientId, TDesC8& aClientName, TUint& aTargetClientId);
    TInt GetResourceId(TUint aClientId, TDesC8& aResourceName, TUint& aResourceId);
    TInt GetResourceInfo(TUint aClientId, TUint aResourceId, TAny* aBuf);
    TInt GetNumResourcesInUseByClient(TUint aClientId, TUint aTargetClientId,TUint& aNumResources);
    TInt GetInfoOnResourcesInUseByClient(TUint aClientId, TUint aTargetClientId, TUint& aNumResources, TAny* info);
    TInt GetNumClientsUsingResource(TUint aClientId, TUint aResourceId, TUint& aNumClients);
    TInt GetInfoOnClientsUsingResource(TUint aClientId, TUint aResourceId, TUint& aNumClients, TAny* info);
    TInt AllocReserve(TUint aClientId, TUint8 aNumClientLevels, TUint8 aNumResources);
    TInt CheckNotifications(TUint aResourceId, TUint aUnconNoti, TUint aCondNoti);
    TInt ChangeResourceStateSync(TUint aClientId, TUint aResourceId, TInt aNewState);
    void ChangeResourceStateAsync(TUint aClientId, TUint aResourceId, TInt& aState, TRequestStatus& aStatus, TBool aReqCancel = EFalse);
    TInt GetResourceStateSync(TUint aClientId, TUint aResourceId, TBool aCached, TInt& aNewState, TInt& aLevelOwnerId);
    void GetResourceStateAsync(TUint aClientId, TUint aResourceId, TBool aCached, TRequestStatus& aStatus, TInt& aState, TInt& aLevelOwnerId, TBool aReqCancel = EFalse);
    TInt RequestNotification(TUint aClientId, TUint aResourceId);
    TInt RequestNotification(TUint aClientId, TUint aResourceId, TInt aDirection, TInt aThreshold);
    TInt CancelNotification(TUint aClientId, TUint aResourceId, TBool aType);
    TInt RegisterForIdleResourcesInfo(TUint aPowerControllerId, TUint aResourceNum, TAny* anInfo);
    TInt GetIdleResourcesInfo(TUint aResourceNum, TAny* info);
	TInt DeRegisterClientLevelFromResource(TInt aClientId, TUint aResId);
	TInt CheckPostBootLevelNotifications();
	TInt GetResourceControllerVersion(TUint aClientId, TUint& aVersion);
	TInt CheckResourceControllerRegistration();
#ifdef PRM_ENABLE_EXTENDED_VERSION
	TInt RegisterDynamicResource(TUint aClientId, TUint& aResourceId);
	TInt DeRegisterDynamicResource(TUint aClientId, TUint aResourceId, TInt *aLevel);
	TInt RegisterResourceDependency(TUint aClientId, SResourceDependencyInfo aInfo1, SResourceDependencyInfo aInfo2);
	TInt DeRegisterResourceDependency(TUint aClientId, TUint aResourceId1, TUint aResourceId2);
	void ChangeResStateAndDeRegisterDynamicRes(TUint aClientId, TUint aResourceId, TInt &aLevel, TRequestStatus& aStatus);
	TInt GetNumDependentsForResource(TUint aClientId, TUint aResourceId, TUint& aNumDepResource);
	TInt GetDependentsIdForResource(TUint aClientId, TUint aResourceId, TAny* aResIdArray, TUint& aNumDepResources);
	void CheckParallelExecutionForChangeResState(TUint aClientId, TUint aDepResId, TInt& aDepLevel, TUint aResId, TInt aLevel, TRequestStatus& aStatus);
#endif //PRM_ENABLE_EXTENDED_VERSION
    inline static TVersion VersionRequired();
    };

inline TVersion RTestResMan::VersionRequired()
	{
	const TInt KMajorVersionNumber=1;
	const TInt KMinorVersionNumber=0;
	const TInt KBuildVersionNumber=KE32BuildVersionNumber;
	return TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}

#ifndef __KERNEL_MODE__

/** Open a channel for the driver.This driver does not allow more than one channel open at a time. */ 
TInt RTestResMan::Open()
	{
    return DoCreate(KLddName, VersionRequired(),KNullUnit,NULL,NULL);
	}

/** Request to register a client with resource manager
	@Param	- aClientId, On Success with be updated with a valid unique clientID,
			- aClientName, Name of the client to register with RM,
			- aType, Defines ownership, can be either process relative or thread relative
	@return- KErrNone on Sucess or one of system wide errors.
	*/
TInt RTestResMan::RegisterClient(TUint& aClientId, const TDesC* aClientName, TOwnerType aType)
	{
    TParameterListInfo anInfo;
    anInfo.iPtr1 = (TAny*)&aClientId;
    anInfo.iPtr2 = (TAny*)aClientName;
    anInfo.iPtr3 = (TAny*)aType;
    return DoControl(ERegisterClient, (TAny*)&anInfo);
	}

/** Request to deregister a client from RM.
	@Param - aClientID, The ID of the Client to deregister
	@return- KErrNone on Sucess or one of system wide errors.
	*/
TInt RTestResMan::DeRegisterClient(TUint aClientId)
	{
    return DoControl(EDeRegisterClient, (TAny*)aClientId);
	}

/** Request to obtain the name of the specified client of the RM
	@Param	- aClientId, Requesting ClientId
			- aTargetClientId, Id of the client whose name is requested.
			- aClientName, On Success returned with client name.
	@return- KErrNone on Sucess or one of system wide errors.
	*/
TInt RTestResMan::GetClientName(TUint aClientId, TUint aTargetClientId, TDes8* aClientName)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)aTargetClientId;
    anInfo.iPtr2 = (TAny*)aClientName;
    return DoControl(EGetClientName, (TAny*)&anInfo);
	}

/** Request to obtain the ID of the specified client of the RM
	@Param  - aClientId, Requesting ClientId,
			- aClientName, Client Name whose Id is being requested,
			- aTargetClientId, On Success with be updated with requested client Id
	@return- KErrNone on Sucess or one of system wide errors.	
	*/
TInt RTestResMan::GetClientId(TUint aClientId, TDesC8& aClientName, TUint& aTargetClientId)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)&aClientName;
    anInfo.iPtr2 = (TAny*)&aTargetClientId;
    return DoControl(EGetClientId, (TAny*)&anInfo);
	}

/** Request to obtain the ID of the specified resource of the RM
	@Param	- aClientId, Requesting ClientId,
			- aResourceName, Resource name whose ID is being requested
			- aResourceId, On Success returned with resource id.
	@return- KErrNone on Sucess or one of system wide errors.
	*/
TInt RTestResMan::GetResourceId(TUint aClientId, TDesC8& aResourceName, TUint& aResourceId)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)&aResourceName;
    anInfo.iPtr2 = (TAny*)&aResourceId;
    return DoControl(EGetResourceId, (TAny*)&anInfo);
	}

/** Request to obtain the information of the specified resource
	@Param	- aClientId, Requesting ClientId,
			- aResourceId, Resource Id whose information is requested
			- aBuf, On Success filled with resource information
	@return- KErrNone on Sucess or one of system wide errors.
	*/
TInt RTestResMan::GetResourceInfo(TUint aClientId, TUint aResourceId, TAny* aBuf)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)aResourceId;
    anInfo.iPtr2 = aBuf;
    return DoControl(EGetResourceInfo, (TAny*)&anInfo);
	}

/** Request to obtain the number of resources the specified client has requirement on resource level.
	@Param	- aClientId, Requesting ClientId,
			- aTargetClientId, ClientId, the number of resources on which it has requirement is requested.
			- aNumResources, On Success contains the number of resources the client has requirement
	@return- KErrNone on Sucess or one of system wide errors.
	*/
TInt RTestResMan::GetNumResourcesInUseByClient(TUint aClientId, TUint aTargetClientId,TUint& aNumResources)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*) aTargetClientId;
    anInfo.iPtr2 = (TAny*)&aNumResources;
    return DoControl(EGetNumResourcesInUseByClient, (TAny*)&anInfo);
	}

/** Request to obtain the information on resources
	@Param	- aClientId, Requesting ClientId,
			- aTargetClientId, ClientId, information on all the resources on which it has requirement is requested
			- aNumResources, Number of resources whose information needs to be filled (size of info)
			- info, On sucess will be filled with resources information.
	@return- KErrNone on Sucess or one of system wide errors.
	*/
TInt RTestResMan::GetInfoOnResourcesInUseByClient(TUint aClientId, TUint aTargetClientId, TUint& aNumResources, TAny* info)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)aTargetClientId;
    anInfo.iPtr2 = (TAny*)&aNumResources;
    anInfo.iPtr3 = info;
    return DoControl(EGetInfoOnResourcesInUseByClient, (TAny*)&anInfo);
	}

/** Request to obtain the number of clients holding the specified resource.
	@Param	- aClientId, Requesting ClientId,
			- aResourceId, ID of the resource.
			- aNumClients, On Success contains the number of clients holding requirement on specified resource
	@return- KErrNone on Sucess or one of system wide errors.
	*/
TInt RTestResMan::GetNumClientsUsingResource(TUint aClientId, TUint aResourceId, TUint& aNumClients)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)aResourceId;
    anInfo.iPtr2 = (TAny*)&aNumClients;
    return DoControl(EGetNumClientsUsingResource, (TAny*)&anInfo);
	}

/** Request to obtain the information on clients
	@Param	- aClientId, Requesting ClientId,
			- aResourceId, Id of the resource
			- aNumClients, Number of Clients whose information needs to be filled (size of info)
			- info, On sucess will be filled with client information.
	@return- KErrNone on Sucess or one of system wide errors.
	*/
TInt RTestResMan::GetInfoOnClientsUsingResource(TUint aClientId, TUint aResourceId, TUint& aNumClients, TAny* info)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)aResourceId;
    anInfo.iPtr2 = (TAny*)&aNumClients;
    anInfo.iPtr3 = info;
    return DoControl(EGetInfoOnClientsUsingResource, (TAny*)&anInfo);
	}

/** Request to preallocate the clientlevel and requests RM internal structures.
	@Param	- aClientId, Requesting ClientId,
			- aNumClientLevels, Client Level objects to preallocate
			- aNumRequests, Request level objects to preallocate
	@return- KErrNone on Sucess or one of system wide errors.
	*/
TInt RTestResMan::AllocReserve(TUint aClientId, TUint8 aNumClientLevels, TUint8 aNumRequests)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)aNumClientLevels;
    anInfo.iPtr2 = (TAny*)aNumRequests;
    return DoControl(EAllocReserve, (TAny*)&anInfo);
	}

/** Function to verify the notification sent for the earlier resource state change.
	@Param	- aResourceId, Id of the resource whose notifications are verified.
			- aUncondNoti, Number of expected unconditional notification
			- aCondNoti, Number of expected conditional notification
	@return- KErrNone on Sucess or KErrUnderflow
	*/
TInt RTestResMan::CheckNotifications(TUint aResourceId, TUint aUncondNoti, TUint aCondNoti)
	{
    TParameterListInfo anInfo;
    anInfo.iPtr1 = (TAny*)aResourceId;
    anInfo.iPtr2 = (TAny*)aUncondNoti;
    anInfo.iPtr3 = (TAny*)aCondNoti;
    return DoControl(ECheckNotifications, (TAny*)&anInfo);
	}

/** Request to change the state of the resource synchronously
	@Param	- aClientId, Requesting clientId
			- aResourceId, Id of the resource whose state change is requested.
			- aNewState, Requested new state
	@return- KErrNone on Sucess or one of system wide errors.
	*/
TInt RTestResMan::ChangeResourceStateSync(TUint aClientId, TUint aResourceId, TInt aNewState)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)aResourceId;
    anInfo.iPtr2 = (TAny*)aNewState;
    return DoControl(EChangeResourceStateSync, (TAny*)&anInfo);
	}

/** Request to change the state of the resource asynchronously
	@Param	- aClientId, Requesting clientId
			- aResourceId, Id of the resource whose state change is requested.
			- aState, Requested new state
			- aStatus, TRequestStatus object to indicate completion of operation
			- aReqCancel, If true CancelAsyncOperation API of RM is called immediately after asynchronos request operation
	*/
void RTestResMan::ChangeResourceStateAsync(TUint aClientId, TUint aResourceId, TInt& aState, TRequestStatus& aStatus, TBool aReqCancel)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)aResourceId;
    anInfo.iPtr2 = (TAny*)&aState;
    anInfo.iPtr3 = (TAny*)aReqCancel;
    DoRequest(EChangeResourceStateAsync, aStatus, (TAny*)&anInfo);
	}

/** Request to obtain the state of the resource synchronously
	@Param	- aClientId, Requesting clientId
			- aResourceId, Id of the resource whose state change is requested.
			- aCached, if true requesting for cached value
			- aNewState, On success returns the new state
			- aLevelOwnerId, On success returns the Id of the client currently holding the resource.
	@return- KErrNone on Sucess or one of system wide errors.
	*/
TInt RTestResMan::GetResourceStateSync(TUint aClientId, TUint aResourceId, TBool aCached, TInt& aNewState, TInt& aLevelOwnerId)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)aResourceId;
    anInfo.iPtr2 = (TAny*)aCached;
    anInfo.iPtr3 = (TAny*)&aNewState;
	anInfo.iPtr4 = (TAny*)&aLevelOwnerId;
    return DoControl(EGetResourceStateSync, (TAny*)&anInfo);
	}

/** Request to obtain the state of the resource asynchronously
	@Param	- aClientId, Requesting clientId
			- aResourceId, Id of the resource whose state change is requested.
			- aCached, if true requesting for cached value
			- aStatus, TRequestStatus object to indicate completion of operation
			- aReqCancel, If true CancelAsyncOperation API of RM is called immediately after asynchronos request operation
	*/
void RTestResMan::GetResourceStateAsync(TUint aClientId, TUint aResourceId, TBool aCached, TRequestStatus& aStatus, TInt& aState, TInt& aLevelOwnerId, TBool aReqCancel)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)aResourceId;
    anInfo.iPtr2 = (TAny*)aCached;
    anInfo.iPtr3 = (TAny*)aReqCancel;
	anInfo.iPtr4 = (TAny*)&aState;
	anInfo.iPtr5 = (TAny*)&aLevelOwnerId;
    DoRequest(EGetResourceStateAsync, aStatus, (TAny*)&anInfo);
	}

/** Request to notify when the state of the specified resource changes
	@Param	- aClientId, Requesting clientId,
			- aResourceId, ID of the resource
	@return- KErrNone on Sucess or one of system wide errors.
	*/
TInt RTestResMan::RequestNotification(TUint aClientId, TUint aResourceId)
	{
    return DoControl(ERequestNotificationUncond, (TAny*)aClientId, (TAny*)aResourceId);
	}

/** Request to notify when the state of the specified resource change crosses the threshold in the specified direction
	@Param	- aClientId, Requesting clientId,
			- aResourceId, ID of the resource,
			- aDirection, Direction of change of the resource state that will trigger the notification
			- aThreshold, Level of resource state that will trigger the notifications when reached.
	@return- KErrNone on Sucess or one of system wide errors.
	*/
TInt RTestResMan::RequestNotification(TUint aClientId, TUint aResourceId, TInt aDirection, TInt aThreshold)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)aResourceId;
    anInfo.iPtr2 = (TAny*)aThreshold;
    anInfo.iPtr3 = (TAny*)aDirection;
    return DoControl(ERequestNotificationCond, (TAny*)&anInfo);
	}

/** Request to cancel the previously requested notification
	@Param	- aClientId, Requesting clientId
			- aResourceId, ID of the resource
			- aType, ETrue Conditional
	@return- KErrCancel on Sucess or one of system wide errors.
	*/
TInt RTestResMan::CancelNotification(TUint aClientId, TUint aResourceId, TBool aType)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)aResourceId;
    anInfo.iPtr2 = (TAny*)aType;
    return DoControl(ECancelNotification, (TAny*)&anInfo);
	}

/** Request to register the specified number of resources to RM to keep the list updated with the
	cached value of the resources and owner of the resources. This is used for testing of the API.
	@Param	- aResourceNum - Number of resource, whose information needs to be cached
	@return- KErrNone on Sucess or one of system wide errors.
	*/
TInt RTestResMan::RegisterForIdleResourcesInfo(TUint aPowerControllerId, TUint aResourceNum, TAny* aPtr)
     {
	 TParameterListInfo anInfo;
	 anInfo.iClientId = aPowerControllerId;
	 anInfo.iPtr1 = (TAny*)aResourceNum;
	 anInfo.iPtr2 = (TAny*)aPtr;
     return DoControl(ERegisterForIdleResourcesInfo, (TAny*)&anInfo);
     }

/** Request to Deregister client level from resource. 
	@Param - aClientId - ID of the client requesting deregistration.
	@Param - aResId - ID of the resource from which to deregister the client level of requested client.
	@return KErrNone on Sucess or one of system wide errors.
	*/
TInt RTestResMan::DeRegisterClientLevelFromResource(TInt aClientId, TUint aResId)
	{
	return DoControl(EDeRegisterClientLevelFromResource, (TAny*)aClientId, (TAny*)aResId);
	}


/** Request to get the information of the all the resources cached by RM as requested by 
	RegisterForIdleResourcesInfo API
	@Param	- aResourceNum - Number of resources (size of info)
			- info, On success returns with resource infomations
	@return- KErrNone on Sucess or one of system wide errors.
	*/
TInt RTestResMan::GetIdleResourcesInfo(TUint aResourceNum, TAny* info)
     {
     return DoControl(EGetIdleResourcesInfo, (TAny*)aResourceNum, (TAny*)info);
     }

/** Request to check the notifications recieved as a result of postboot level setting. 
	@return KErrNone on success or one of system wide errors.
	*/
TInt RTestResMan::CheckPostBootLevelNotifications()
	{
	return DoControl(ECheckPostBootLevelNotifications, (TAny*)NULL, (TAny*)NULL);
	}

/** Get the version of Resource Controller
	@Param - aClientId - Id of the client requesting version information
	@Param - aVersion - Version will be updated. 0 - Basic, 1- extended version
	@return KErrNone on success or one of system wide errors.
	*/
TInt RTestResMan::GetResourceControllerVersion(TUint aClientId, TUint& aVersion)
	{
	return DoControl(EGetControllerVersion, (TAny*)aClientId, (TAny*)&aVersion);
	}

TInt RTestResMan::CheckResourceControllerRegistration()
	{
	return DoControl(ERegisterResourceController, (TAny*)NULL, (TAny*)NULL);
	}

#ifdef PRM_ENABLE_EXTENDED_VERSION
/** Register dynamic resource.
	@Param - aClientId - Id of the client that is registering the dynamic resource
	@Param - aResourceId - On success will be updated with the resource id allocated for this resource
	@return KErrNone on success or one of system wide errors.
	*/
TInt RTestResMan::RegisterDynamicResource(TUint aClientId, TUint& aResourceId)
	{
	return DoControl(ERegisterDynamicResource, (TAny*)aClientId, (TAny*)&aResourceId);
	}

/** Deregister dynamic resource.
	@Param - aClientId - Id of the client that is deregistering the dynamic resource
	@Param - aResourceId - Id of the dynamic resource to deregister
	@Param - aLevel - Pointer to the required final state. 
	@return KErrNone on success or one of system wide errors.
	*/
TInt RTestResMan::DeRegisterDynamicResource(TUint aClientId, TUint aResourceId, TInt *aLevel)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)aResourceId;
    anInfo.iPtr2 = (TAny*)aLevel;
    return DoControl(EDeRegisterDynamicResource, (TAny*)&anInfo);
	}

/** Change the resource state asynchronously and immediately try to deregister dynamic resource.
	@Param - aClientId - Id of the client that is requesting the resource state change.
	@Param - aResourceId - Id of the resource whose state change is requested.
	@Param - aLevel - Requested new state
	@Param - aStatus, TRequestStatus object to indicate completion of operation
	@return KErrNone on success or one of system wide errors.
	*/
void RTestResMan::ChangeResStateAndDeRegisterDynamicRes(TUint aClientId, TUint aResourceId, TInt &aLevel, TRequestStatus &aStatus)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)aResourceId;
    anInfo.iPtr2 = (TAny*)&aLevel;
    DoRequest(EChangeResStateAndDeregisterDynamicRes, aStatus, (TAny*)&anInfo);
	}

/** Register depedency between resources.
	@Param - aClientId - Id of the client that is establishing the dependency.
	@Param - aInfo1 - Infomation about the first resource in the dependency link.
	@Param - aInfo2 - Information about the second resource in the dependency link.
	@return KErrNone on success or one of the system wide errors.
    */
TInt RTestResMan::RegisterResourceDependency(TUint aClientId, SResourceDependencyInfo aInfo1, SResourceDependencyInfo aInfo2)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)&aInfo1;
    anInfo.iPtr2 = (TAny*)&aInfo2;
    return DoControl(ERegisterResourceDependency, (TAny*)&anInfo);
	}

/** Deregister dependency between resources.
	@Param - aClientId - Id of the client that is deregistering the resource dependency
	@Param - aResourceId1 - Id of the first resource in the dependency link that is being deregistered.
	@Param - aResourceId2 - Id of the second resource in the dependency link that is being deregistered.
	@return KErrNone on success or one of the system wide errors.
	*/
TInt RTestResMan::DeRegisterResourceDependency(TUint aClientId, TUint aResourceId1, TUint aResourceId2)
	{
    TParameterListInfo anInfo;
    anInfo.iClientId = aClientId;
    anInfo.iPtr1 = (TAny*)aResourceId1;
    anInfo.iPtr2 = (TAny*)aResourceId2;
    return DoControl(EDeRegisterResourceDependency, (TAny*)&anInfo);
	}

/** Get number of dependents for the specified resource
	@Param - aClientId - Id of the client that is requesting dependents count
	@Param - aResourceId - Id of the resource whose number of dependents is requested.
	@Param - aNumDepResource - On success will be updated with number of dependents.
	@return KErrNone on success or one of the system wide errors.
	*/
TInt RTestResMan::GetNumDependentsForResource(TUint aClientId, TUint aResourceId, TUint& aNumDepResource)
	{
	TParameterListInfo anInfo;
	anInfo.iClientId = aClientId;
	anInfo.iPtr1 = (TAny*)aResourceId;
	anInfo.iPtr2 = (TAny*)&aNumDepResource;
	return DoControl(EGetNumDependentsForResource, (TAny*)&anInfo);
	}

/** Get dependent resource id's for the specified resource
	@Param - aClientId - Id of the client that is requesting the dependent id list.
	@Param - aResIdArray - On success will be updated with dependent resource id.
	@Param - aNumDepResources - Will be updated with actual number of dependents.
	@return KErrNone on success or one of the system wide errors.
	*/
TInt RTestResMan::GetDependentsIdForResource(TUint aClientId, TUint aResourceId, TAny* aResIdArray, TUint& aNumDepResources)
	{
	TParameterListInfo anInfo;
	anInfo.iClientId = aClientId;
	anInfo.iPtr1 = (TAny*)aResourceId;
	anInfo.iPtr2 = (TAny*)aResIdArray;
	anInfo.iPtr3 = (TAny*)&aNumDepResources;
	return DoControl(EGetDependentsIdForResource, (TAny*)&anInfo);
	}
/** Checks for state change of dependency resource while deregistering the non-dependency resource.
	@Param - aClientId - Id of the client that is requesting this operation
	@Param - aDepResInfo - Embeds resource id, level and status for dependency resource
	@Param - aDepLevel - the level to which the dependency resource is moved to.
	@Param - aResId - Id of the static resource.
	@Param - aLevel - the level to which the static resource is moved to.
	@Param - aStatus - to recieve the status of asynchronous operation.
	@return KErrNone on success or one of the system wide errors.
	*/
void RTestResMan::CheckParallelExecutionForChangeResState(TUint aClientId, TUint aDepResId, TInt& aDepLevel, 
												TUint aResId, TInt aLevel, TRequestStatus& aStatus)
	{
	TParameterListInfo anInfo;
	anInfo.iClientId = aClientId;
	anInfo.iPtr1 = (TAny*)aDepResId;
	anInfo.iPtr2 = (TAny*)&aDepLevel;
	anInfo.iPtr3 = (TAny*)aResId;
	anInfo.iPtr4 = (TAny*)aLevel;
	DoRequest(ECheckParallelExecutionForChangeResState, aStatus, (TAny*)&anInfo);
	}
#endif //PRM_ENABLE_EXTENDED_VERSION

#endif //__KERNEL_MODE__
#endif //__D_RESCONTROLCLI_H__
