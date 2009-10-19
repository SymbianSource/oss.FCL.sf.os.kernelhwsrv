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
// e32\include\drivers\resourceman.inl
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 
 @publishedPartner
 @released 9.5
 
 Request to get the resource controller version
 
 @param aClientId	ID of the client which is requesting the resource controller version.
 @param aVersion		On Success, returns the version of PRM
 
 @return KErrNone			if successful
 KErrAccessDenied	if the client ID could not be found in the list of registered clients or
 if the client was registered to be thread relative and this API is not 
 called from the same thread.
 
 @pre Interrupts must be enabled
 @pre Kernel must be unlocked
 @pre No fast mutex can be held
 @pre Call in a thread context but not from null thread or DFC thread1
 @pre Can be used in a device driver.
*/
inline TInt PowerResourceManager::GetResourceControllerVersion(TUint aClientId, TUint& aVersion)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::ResourceControllerVersion"));
	return ControlIO(aClientId, KResManControlIoGetVersion, (TAny*)&aVersion, NULL);
	}
#ifdef PRM_ENABLE_EXTENDED_VERSION

/**
@publishedPartner
@released 9.5

Request to register dynamic resource. This is also used to register dynamic resource with dependency

@param aClientId	ID of the client which is requesting the dynamic resource registration.
@param aResource	Dynamic resource to register.
@param aResourceId	On success, updates with resource id corresponding to this resource

@return KErrNone			if successful
		KErrAccessDenied	if the client ID could not be found in the list of registered clients or
							if the client was registered to be thread relative and this API is not 
							called from the same thread or if ID is user side client id.
		KErrNotSupported	if this API is called before PRM is fully intialised or if the resource ID does not
		                    correspond to dynamic resource or dynamic resource which support depedency
		KErrAlreadyExists	if resource is already registered.


@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver.
*/
inline TInt PowerResourceManager::RegisterDynamicResource(TUint aClientId, DDynamicPowerResource* aResource, 
																						TUint& aResourceId)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::RegisterDynamicResource"));
	return ControlIO(aClientId, KResManControlIoRegisterDynamicResource, (TAny*)aResource, (TAny*)&aResourceId);
	}

/**
@publishedPartner
@released 9.5

Request to deregister dynamic resource. This is also used to deregister dynamic resource with dependency

@param aClientId	ID of the client which is requesting the dynamic resource deregistration.
@param aResourceId	Id of dynamic resource to deregister.
@param aState		Pointer to the required final state. This is optional and if left NULL, resource
					will be moved to its default state.

@return KErrNone			if successful
		KErrAccessDenied	if the client ID could not be found in the list of registered clients or
							if the client was registered to be thread relative and this API is not 
							called from the same thread or if ID is user side client id or if the client
							is not the same that registered the resource.
		KErrNotSupported	if this API is called before PRM is fully intialised or if the resource ID does not 
		                    correspond to dynamic resource or dynamic resource which support depedency
		KErrNotFound		if the resource could not found in the resource list. 
		KErrInUse			if the some other operation is in progress or if the resource is shared and
							another client holds requirement on this resource


@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver.
*/
inline TInt PowerResourceManager::DeRegisterDynamicResource(TUint aClientId, TUint aResourceId, TInt* aState)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::DeRegisterDynamicResource"));
	return ControlIO(aClientId, KResManControlIoDeregisterDynamicResource, (TAny*)aResourceId, (TAny*)aState);
	}

/**
@publishedPartner
@released 9.5

Request to register resource dependency. This could be between 2 dynamic resource or between
dynamic and static resource.

@param aClientId	ID of the client which is requesting the resource dependency registration.
@param aResDependencyInfo1 Dependency information about the first resource in the dependency link.
@param aResDependencyInfo2 Dependency information about the second resource in the dependency link.

@return KErrNone			if successful
		KErrAccessDenied	if the client ID could not be found in the list of registered clients or
							if the client was registered to be thread relative and this API is not 
							called from the same thread or if ID is user side client id 
		KErrNotSupported	if any of the specified resource is not dynamic resource with depedency or 
		                    is not dependency resource or is instantaneous
		KErrNotFound		if any of the specified resource could not found in the resource list. 


@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver.
*/
inline TInt PowerResourceManager::RegisterResourceDependency(TUint aClientId, SResourceDependencyInfo* aResDependencyInfo1, 
																			SResourceDependencyInfo* aResDependencyInfo2)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::RegisterResourceDependency"));
	return ControlIO(aClientId, KResManControlIoRegisterDependency, (TAny*)aResDependencyInfo1, (TAny*)aResDependencyInfo2);
	}

/**
@publishedPartner
@released 9.5

Request to deregister resource dependency.
 
@param aClientId	ID of the client which is requesting the resource dependency deregistration.
@param aResId1 Id of the first resource in the dependency link that is being deregistered.
@param aResId2 Id of the second resource in the dependency link that is being deregistered.

@return KErrNone			if successful
		KErrAccessDenied	if the client ID could not be found in the list of registered clients or
							if the client was registered to be thread relative and this API is not 
							called from the same thread or if ID is user side client id 
							or if any of the specified resource does not support dependency.
		KErrNotFound		if any of the specified resource could not found in the resource list or 
		                    dependency link does not exist between the specified resources.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver.
*/
inline TInt PowerResourceManager::DeRegisterResourceDependency(TUint aClientId, TUint aResourceId1, TUint aResourceId2)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::DeRegisterResourceDependency"));
	return ControlIO(aClientId, KResManControlIoDeregisterDependency, (TAny*)aResourceId1, (TAny*)aResourceId2);
	}

/**
@internalComponent
@released 9.5

Request to update with number of dependent resources for the specified resource. 

@param aResourceId Id of the resource whose number of dependents is requested
@param aNumDepResources On success will be updated with number of dependent resources.

@return KErrNone			if successful
		KErrAccessDenied	if the client ID could not be found in the list of registered clients or
							if the client was registered to be thread relative and this API is not 
							called from the same thread or if ID is user side client id 
		KErrNotSupported	if this API is called before PRM is fully intialised or if the resource ID 
		                    does not correspond to dependency resource 
		KErrNotFound		if the resource could not be found in the resource list. 

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver.
*/
inline TInt PowerResourceManager::GetNumDependentsForResource(TUint aClientId, TUint aResourceId, TUint& aNumDepResource)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::GetNumDependentsForResource"));
	return ControlIO(aClientId, KResManControlIoGetNumDependents, (TAny*)aResourceId, (TAny*)&aNumDepResource);
	}

/**
@internalComponent
@released 9.5

Request to update the specified array with dependent resource Id's of the specified resource. 

@param aResourceId Id of the resource whose dependent resource Id's are requested.
@param aResIdArray On success array will be updated with the dependent resource information. Client need to 
					create the array in kernel heap or data space.
@param aNumDepResources Specifies the size of array. On success, updated with actual number of dependent resources.

@return KErrNone			if successful
		KErrAccessDenied	if the client ID could not be found in the list of registered clients or
							if the client was registered to be thread relative and this API is not 
							called from the same thread or if ID is user side client id 
		KErrNotSupported	if this API is called before PRM is fully initialised or if the resource ID 
		                    does not correspond to dependency resource 
		KErrNotFound		if the resource could not be found in the resource list. 
		KErrArgument		if passed array is null or pass dependent resource number is 0.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver.
*/
inline TInt PowerResourceManager::GetDependentsIdForResource(TUint aClientId, TUint aResourceId, TAny* aResIdArray, 
																							TUint& aNumDepResources)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::GetDependentsIdForResource"));
	return ControlIO(aClientId, KResManControlIoGetDependentsId, (TAny*)aResourceId, (TAny*)aResIdArray, 
																							(TAny*)&aNumDepResources);
	}

#endif
