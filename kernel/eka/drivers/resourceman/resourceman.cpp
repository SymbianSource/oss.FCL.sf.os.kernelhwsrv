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
// e32\drivers\resourceman\resourceman.cpp
// 
//

#include <drivers/resourcecontrol.h>
#include <drivers/resourceman.h>
/**
@publishedPartner
@prototype 9.5

Register a client with the resource manager

@param aClientId  A reference to a client ID: returns a unique handle if registration was
                  successful, 0 otherwise.
@param aName      Descriptor with name for client. The descriptor is created by the client
                  in kernel heap or in kernel stack.
                  NOTE: Name should ideally relate to component name and should take care
                  of name uniqueness as this is not checked by resource manager unless 
				  DEBUG_VERSION macro is enabled.
@param aType      Defines ownership
                  EOwnerProcess - The client ID can be used by all thread in the process to
                  call the resource manager API's
                  EOwnerThread - The client ID can only be used by the thread that registered
                  the client to resource manager to call the PRM API's
                  By default this is set to EOwnerProcess.

@return           KErrNone     if the operation was successful,
                  KErrNoMemory if a new client link was needed but could not be created and 
                               added to the client list,
                  KErrTooBig   if the length of the descriptor passed is greater than 32.
				  KErrAlreadyExists if the specified name already exists. This is valid only if 
				                    DEBUG_VERSION macro is enabled. 
				  KErrNotSupported if the number of expected kernel side clients is set to zero
									by PSL during initialisation.
                  
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context, but not from null thread or DFC thread1.
@pre Can be used in a device driver
*/
EXPORT_C TInt PowerResourceManager::RegisterClient(TUint& aClientId, const TDesC8& aName, TOwnerType aType)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::RegisterClient"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("Client Name %S", &aName));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("OwnerType %d", aType));
    return TInterface::RegisterClient(aClientId, aName, aType);
	}

/**
@publishedPartner
@prototype 9.5

Deregister a client with the resource manager

@param aClientId    The ID of the client which is being deregistered

@return             KErrNone     if the operation was successful
                    KErrNotFound if this client ID could not be found in the current
                                 list of clients
		            KErrArgument if user side client ID is specified or client ID to be used
								 by Power Controller is specified.
		            KErrAccessDenied if client was registered to be thread relative and this API
						             is not called from the same thread. 
					

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
EXPORT_C TInt PowerResourceManager::DeRegisterClient(TUint aClientId)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::DeRegisterClient"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("Client ID %d", aClientId));
    return TInterface::DeRegisterClient(aClientId);
	}

/**
@publishedPartner
@prototype 9.5

Obtain the name of a registered client of the resource manager

@param aClientId       The ID of the client which is requesting the name of
                       another client whose ID is specified in aTargetClientId.
@param aTargetClientId The ID of the client whose name is being requested.
@param aName           Descriptor to be filled with the name of the client.
				       The descriptor is created by the client in kernel stack or heap.

@return                KErrNone if the operation was successful
                       KErrNotFound if this client ID (aTargetClientId) could not be
                                    found in the current list of registered clients.
                       KErrAccessDenied if the client ID (aClientId) could not be found
                                        in the current list of registered clients or if the client was 
				                        registered to be thread relative and this API is not called from 
				                        the same thread.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
EXPORT_C TInt PowerResourceManager::GetClientName(TUint aClientId, TUint aTargetClientId, TDes8&
 aName)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::GetClientName"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aTargetClientId 0x%08x", aTargetClientId));
    return TInterface::GetClientName(aClientId, aTargetClientId, aName);
	}

/**
@publishedPartner
@prototype 9.5

Obtain the Id of registered client of the resource manager

@param aClientId       ID of the client which is requesting the ID of the another
                       client whose name is specified in aClientName
@param aClientName     Descriptor containing the name of the client whose ID is being
                       requested. The client must create the descriptor in kernel stack
                       or heap.
                       NOTE: Resource manager does not check for uniqueness of client
                       name during registration, so if there are multiple clients registered
                       to PRM with same name it will return the ID of the first client encountered
				       with the specified name (order is not guaranteed).
@param aTargetClientId Updates with ID of the requested client on success

@return                KErrNone if the operation was successful
                       KErrNotFound if this client name could not be found in the current list 
					                of registered client.
                       KErrAccessDenied if the client ID (aClientId) could not be found in the current
                                       list of registered client or if the client was registered to 
									   be thread relative and this API is not called from the same thread.
                       KErrTooBig if the length of the descriptor passed is greater than 32.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
EXPORT_C TInt PowerResourceManager::GetClientId(TUint aClientId, TDesC8& aClientName, TUint& aTargetClientId)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::GetClientId"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientName %S", &aClientName));
    return TInterface::GetClientId(aClientId, aClientName, aTargetClientId);
	}

/**
@publishedPartner
@prototype 9.5

Obtain the ID of registered resource of the resource manager.
NOTE: ID of the first matching name found in the resource list will be returned

@param aClientId      ID of the client which is requesting the ID of the
                      resource, by specifying its name.
@param aResourceName  Descriptor containing the name of the resource whose
                      ID is being requested.The client must create descriptor in 
					  kernel stack or heap.
@param aResourceId    Updates with ID of the requested resource on success

@return               KErrNone if the operation was successful
                      KErrAccessDenied if the ID of the client could not be found in the
                                       current list of registered clients or if the client was
						               registered to be thread relative and this API is not called
						               from the same thread. 
                      KErrNotFound if this resource name could not be found in the current
                                   list of registered resources.
		              KErrTooBig if the length of the descriptor passed is greater than 32. 

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
EXPORT_C TInt PowerResourceManager::GetResourceId(TUint aClientId, TDesC8& aResourceName, TUint& aResourceId)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::GetResourceId"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aResourceName %S", &aResourceName));
    return TInterface::GetResourceId(aClientId, aResourceName, aResourceId);
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

@return             KErrNone if the operation was successful
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
EXPORT_C TInt PowerResourceManager::GetResourceInfo(TUint aClientId, TUint aResourceId, TAny* aInfo)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::GetResourceInfo"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aResourceId 0x%08x", aResourceId));
    return TInterface::GetResourceInfo(aClientId, aResourceId, aInfo);
	}


/**
@publishedPartner
@prototype 9.5

Request number of resources that the specified client (aTargetClientId) has
requirement on resource level. Client ID starts from 1, so if 0 is specified in
aTargetClientId, returns the total number of controllable resources registered with PRM.

@param aClientId       ID of the client that is requesting the number of resources for which
                       the specified client (aTargetClientId) holds requirement on the
                       resource level change.
@param aTargetClientId ID of the client for which the number of resources that it 
					   has requested a level on is to be returned.
@param aNumResource    Updated with the number of resources that the specified client
                       has requirement on resource level change, if valid client
                       ID is passed. If client ID is 0, updates the total number
                       of resources registered with resource manager.

@return                KErrNone if the operation was successful.
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
EXPORT_C TInt PowerResourceManager::GetNumResourcesInUseByClient(TUint aClientId, TUint aTargetClientId, 
																 TUint& aNumResource)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::GetNumResourcesInUseByClient"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aTargetClientId 0x%08x", aTargetClientId));
    return TInterface::GetNumResourcesInUseByClient(aClientId, aTargetClientId, aNumResource);
	}

/**
@publishedPartner
@prototype 9.5

Request information on resources.
If client ID (aTargetClientId) is valid, aInfo is updated with the information 
of the resources that this client requested a resource level.
If client ID (aTargetClientId) is 0, aInfo is updated with the information of all the resources 
registered with resource controller.
The number of resources for which information will be provided will be equal or less than 
the number specified in aNumResources.

@param aClientId       ID of the client which is requesting the resource information.
@param aTargetClientId ID of the client. The information of all the resources on
                       which it has requirement on resource level change is requested.
                       Client ID starts from 1, so calling this API with client ID 0 will
                       fill the details of all the controllable resource registered with
                       resource manager starting from resource ID 1.
@param aNumResources   Number of resource whose information needs to be filled in aInfo i.e,
                       it specifies the size of aInfo array.
@param aInfo           A pointer to an array of descriptor containing an information structure
                       (TPowerResourceInfoV01) to be filled in with the information
                       on the resources. It will be assumed that array allocated will be equal
                       to the number passed in aNumResources. The client must create the array
                       in Kernel stack or heap.

@return                KErrNone if the operation was successful
                       KErrAccessDenied if client ID (aClientId) could not be found in the registered
                                        client list or if the client was registered to be thread relative
										and this API is not called from the same thread.
                       KErrNotFound if client ID (aTargetClientId) could not be found in the current list
                                    of registered client and is also not 0.
                       KErrArgument if aNumResources is 0 or aInfo is NULL or if size of aInfo is not 
					                sufficient to hold the resource information of number of resources 
									specified in aNumResource.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
EXPORT_C TInt PowerResourceManager::GetInfoOnResourcesInUseByClient(TUint aClientId, TUint aTargetClientId, 
																	TUint& aNumResource, TAny* aInfo)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::GetInfoOnResourcesInUseByClient"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aTargetClientId 0x%08x", aTargetClientId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aNumResource 0x%08x", aNumResource));
    return TInterface::GetInfoOnResourcesInUseByClient(aClientId, aTargetClientId, aNumResource, aInfo);
	}

/**
@publishedPartner
@prototype 9.5

Request number of clients which has requirements on the resource level change of the specified
resource. Resource ID starts from 1, so 0 can be used to get the number of clients
registered with resource manager.

@param aClientId         ID of the client which is requesting number of clients
                         holding requirement on specified resource.
@param aResourceId       ID of the resource.
@param aNumClient        Upon success, updated with number of clients having a requirement
                         on resource level for the specified resource, if valid resource ID is specified.
                         If resource ID is 0, then it is updated with number of clients
                         registered with PRM.

@return                  KErrNone if the operation was successful
                         KErrAccessDenied if the client ID could not found in the current list of
                                          registered clients or if the client was registered to be thread 
						                  relative and this API is not called from the same thread.
                         KErrNotFound If this resource ID could not be found in the current list
                                      of registered resource and is also not 0.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
EXPORT_C TInt PowerResourceManager::GetNumClientsUsingResource(TUint aClientId, TUint aResourceId, TUint& aNumClients)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::GetNumClientsUsingResource"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aResourceId 0x%08x", aResourceId));
    return TInterface::GetNumClientsUsingResource(aClientId, aResourceId, aNumClients);
	}

/**
@publishedPartner
@prototype 9.5

Request information on clients
If resource ID is valid, aInfo is updated with the information of the clients
which have a requirements on the resource level for the specified resource
If resource ID is 0, aInfo is updated with the information of the clients registered
with resource manager, starting from client ID 1.
The number of clients for which information will be provided will be equal to or less
than the number specified in a NumClients.

@param aClientId        ID of the client which is requesting the information 
@param aResourceId      Id of the resource.
@param aNumClients		Number of clients whose information needs to be filled in aInfo
						ie, it specifies the size of aInfo array.
@param aInfo            A pointer to an array of descriptor containing an information
                        structure (TPowerClientInfoV01) to be filled in with
                        the information on the client. It will be assumed that array
                        allocated will be equal to the number passed in aNumClients.
                        The Client must create the array of descriptors in kernel stack
                        or heap.

@return                 KErrNone if the operation was successful.
                        KErrNotFound if resource ID could not be found in the registered resource list and is 
						             also not 0.
		                KErrAccessDenied if client ID (aClientId) could not be found in the registered client list
						                 or if the client was registered to be thread relative and this API is not 
						                 called from the same thread. 
                        KErrArgument if aNumClients is 0 or aInfo is NULL or if size of aInfo is not sufficient 
						             to hold client information of specified client number in aNumClients.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
EXPORT_C TInt PowerResourceManager::GetInfoOnClientsUsingResource(TUint aClientId, TUint aResourceId, 
																  TUint& aNumClients, TAny* aInfo)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::GetInfoOnClientsUsingResource"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aResourceId 0x%08x", aResourceId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aNumClients 0x%08x", aNumClients));
    return TInterface::GetInfoOnClientsUsingResource(aClientId, aResourceId, aNumClients, aInfo);
	}

/**
@publishedPartner
@prototype 9.5

Request changing the state of a resource
NOTE: If a resource callback is specified for instantaneous resource, then callback
      will be called after resource change and will be executed in the context of the
      client thread.
      If a resource callback is specified for long latency reosurces, then it will be
      executed asynchronously.
      When the request is accepted the API returns immediately and the calling thread
      is unblocked: the callback (called in the client's context) will be invoked when
      the resource change finally takes place.
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
                       happens (if left NULL the API will execute synchrounously).
                   For Instantaneous resource
                       A pointer to a resource callback object which encapsulates a callback
                       function to be called after resource change. This executes in the
                       context of the client thread.

@return            KErrNone If the API is to be executed synchronously it indicates the change was
                            successful, if the API is to be executed asynchronously it indicates
                            the request to change the resource state has been accepted.
                   KErrNotFound if the resource ID could not be found in the current list of
                                controllable resources.
                   KErrAccessDenied if the client ID could not be found in the list of
                                    registered clients or if the client was registered to be thread 
						            relative and this API is not called from the same thread.
                   KErrNotReady if the request is issued before the resource controller completes its
                                internal initialisation.
                   KErrUnderflow if the client has exceeded the reserved number of
                                 SPowerResourceClientLevel and the free pool is empty or if it is
                                 an asynchronous operation on long latency resource and the client has exceeded 
					             the reserved number of TPowerRequest and the free pool is empty.
				   KErrArgument if requested level is out of range (outside of min and max levels)
				   KErrPermissionDenied if the requested state of the resource is not accepted by its dependents.
				                        This error is valid only for dependent resource state change in extened
										version of PRM.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
@pre Do not call synchronous version from DFC thread 0 for long latency resource
*/
EXPORT_C TInt PowerResourceManager::ChangeResourceState(TUint aClientId , TUint aResourceId, 
														TInt aNewState, TPowerResourceCb* aCb)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::ChangeResourceState"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("aResourceId 0x%08x", aResourceId));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("aNewState 0x%08x", aNewState));
	return TInterface::ChangeResourceState(aClientId, aResourceId, aNewState, aCb);
	}

/**
@publishedPartner
@prototype 9.5

Request the state of the resource synchronously. Client thread will be blocked throughout.

@param aClientId     ID of the client which is requesting the resource state.
@param aResourceId   ID of the resource whose state is being requested.
@param aCached       If ETrue, cached value will be updated in aState.
                     If EFalse, aState will be updated after the resource
                     state is read from resource.
@param aState        Returns the resource state if operation was successful. This
                     could be a binary value for a binary resource, an integer level
                     for a multilevel resource or some platform specific tolen for a
                     multi-property resource.
@param aLevelOwnerId Returns the Id of the client that is currently the owner of the resource.
					 -1	is returned when no client is owner of the resource.

@return              KErrNone if operation was successful
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
EXPORT_C TInt PowerResourceManager::GetResourceState(TUint aClientId, TUint aResourceId, TBool aCached, 
													 TInt& aState, TInt& aLevelOwnerId)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::GetResourceState"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("aResourceId 0x%08x", aResourceId));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("aCached 0x%08x", aCached));
	return TInterface::GetResourceState(aClientId, aResourceId, aCached, aState, aLevelOwnerId);
	}

/**
@publishedPartner
@prototype 9.5

Request the state of the resource asynchrounously.

@param aClientId   ID of the client which is requesting the resource state.
@param aResourceId ID of the resource whose state is being requested.
@param aCached     If ETrue, cached value will be updated in aState
                   If EFalse, will be updated after the resource state is read from resource
@param aCb         For long latency resource:
                      A pointer to a resource callback object which encapsulates a callback function
                      to be called whenever the state of the resource is available for the long
                      latency resource (executes in the context of resource manager)
                   For instantaneous resource:
                      A pointer to a resource callback object which encapsulates a callback
                      function to be called after the resource state is read. This is executed
                      synchronously in the context of the calling thread.
                      NOTE: The client must create the callback object in kernel heap or
                            data section.

@return            KErrNone if the operation was successful
                   KErrAccessDenied if the client ID could not be found in the current list
                                    of registered clients or if the client was registered to be
						            thread relative and this API is not called from the same thread.
                   KErrNotFound if this resource ID could not be found in the current list
                                of controllable resources.
                   KErrNotReady if the request is issued before the resource controller completes
                                its internal initialisation
                   KErrUnderflow if the client has exceeded the reserved number of TPowerRequest
                                 and the TPowerRequest free pool is empty for long latency resource.
		           KErrArgument if callback object is NULL.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
EXPORT_C TInt PowerResourceManager::GetResourceState(TUint aClientId, TUint aResourceId, TBool aCached, TPowerResourceCb& aCb)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::GetResourceState"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aResourceId 0x%08x", aResourceId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aCached 0x%08x", aCached));
    return TInterface::GetResourceState(aClientId, aResourceId, aCached, aCb);
	}

/**
@publishedPartner
@prototype 9.5  

Cancel an asynchronous request(or its callback).

@param aClientId       ID of the client which is requesting the cancellation of the request.
@param aResourceId     ID for the resource which the request that is being cancelled operates
                       upon.
@param aCb             A reference to the resource callback object specified with the request
                       that is being cancelled.

@return                KErrCancel if the request was cancelled.
                       KErrNotFound if this resource ID could not be found in the current list of 
					                controllable resources.
                       KErrCompletion if request is no longer pending.
                       KErrAccessDenied if the client ID could not be found in the current list of registered
		                                clients or if the client was registered to be thread relative and this API
						                is not called from the same thread or if client is not the same that 
						                requested the resource state change.
		               KErrInUse if the request cannot be cancelled as processing of the request already started 
				                 and will run to completion. 

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
EXPORT_C TInt PowerResourceManager::CancelAsyncRequestCallBack(TUint aClientId, TUint aResourceId, TPowerResourceCb& aCb)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::CancelAsyncRequestCallback"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aResourceId 0x%08x", aResourceId));
    return TInterface::CancelAsyncRequestCallBack(aClientId, aResourceId, aCb);
	}

/**
@publishedPartner
@prototype 9.5

Request notification of changes to the state of a resource.

NOTE: This API should return immediately; however the notification will
only happen when a resource change occurs.Notification request is idempotent, 
if the same notification has already been requested for this resource ID, 
the API returns with no further action. Notifications remain queued until 
they are cancelled.

@param aClientId     ID of the client which is requesting the notification.
@param aResourceId   ID of the resource for which notification of state changes
                     is being requested.
@param aN            A reference to a notification object which encapsulates a callback
                     function to be called whenever a resource state change takes place.
                     NOTE: The client must create the notification object in kernel heap
                           or data section.

@return              KErrNone if the operation of requesting a notification was successful.
                     KErrNotFound if this resource ID could not be found in the current list
                                  of controllable resources.
                     KErrAccessDenied if the client ID could not be found in the current
                                      list of registered clients or if the client was registered to be thread
						              relative and this API is not called from the same thread.
		             KErrInUse if the passed notification object is used already.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
EXPORT_C TInt PowerResourceManager::RequestNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::RequestNotification"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aResourceId 0x%08x", aResourceId));
    return TInterface::RequestNotification(aClientId, aResourceId, aN);
	}

/**
@publishedPartner
@prototype 9.5

Request notification when the state of a resource reaches a specified threshold or
goes above or below that threshold (for multilevel resource only) based on direction.
In other words it is issued when a threshold on the specified resource state is crossed
in the direction specified.

NOTE:This API should return immediately; however the notification will only
happen when a resource change occurs. Notification request is idempotent, 
if the same notification has already been requested for this resource ID, 
the API returns with no further action. Notification remain queued until 
they are cancelled.

@param aClientId    ID of the client which is requesting the notification.
@param aResourceId  ID for the resource whose notification of state changes is
                    being requested.
@param aN           A reference to a notification object which encapsulates a callback
                    function to be called whenever the conditions to issue the notification
                    (specified in the API) are met.
                    NOTE: The client must create the notification object in kernel heap
                          or data section.
@param aThreshold   The level of the resource state that will trigger the notification
                    when reached.
@param aDirection   Specifies the direction of change of the resource state that will
                    trigger a notification. EFalse means the notification will be issued
                    when the resource state change to a specified threshold value or below
                    the specified threshold, ETrue means the notification will be issued
                    when the resource state change to a specified threshold value or above
                    the specified threshold.

@return             KErrNone if the operation of requesting a notification was successful.
                    KErrNotFound if this resource ID could not be found in the current list
                                 of controllable reosurces.
                    KErrAccessDenied if the client ID could not be found in the list of
                                     registered clients or if the client was registered to be
						             thread relative and this API is not called from the same thread.
		            KErrInUse if the passed notification object is used already.
		            KErrArgument if the specified threshold is out of range.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
EXPORT_C TInt PowerResourceManager::RequestNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN,
														TInt aThreshold, TBool aDirection)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::RequestNotification"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aResourceId 0x%08x", aResourceId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aThreshold 0x%08x", aThreshold));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aDirection 0x%08x", aDirection));
    return TInterface::RequestNotification(aClientId, aResourceId, aN, aThreshold, aDirection);
	}

/**
@publishedPartner
@prototype 9.5

Cancel and remove from queue a previously issued request for notification on a
resource state change.

@param aClientId    ID of the client which is requesting to cancel the notification
@param aResourceId  ID of the resource whose pending notification of state changes
                    is being cancelled.
@param aN           A reference to the notification object that was associated with
                    the notification request that is being cancelled. This will be
                    used to identify the notification that is being cancelled.

@return             KErrCancel if the notification request was successfully cancelled.
                    KErrNotFound if the specified notification object is not found in the current list
					             of notification objects for the specified resource.
                    KErrAccessDenied if the client requesting the cancellation is not the same
                                     which registered the notification or if the resource id does not
						             match or if the client ID could not be found in the list of 
             						 registered clients or if the client was registered to be 
			             			 thread relative and this API is not called from the same thread.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
EXPORT_C TInt PowerResourceManager::CancelNotification(TUint aClientId, TUint aResourceId, DPowerResourceNotification& aN)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::CancelNotification"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aResourceId 0x%08x", aResourceId));
    return TInterface::CancelNotification(aClientId, aResourceId, aN);
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

@return           KErrNone if the allocation was successful
                  KErrAccessDenied if the client ID could not be found in the list of
                                   registered clients or if the client was registered to be 
						           thread relative and this API is not called from the same thread.
                  KErrNoMemory if there is no sufficient memory for allocation of requested
                               number of objects.

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver
*/
EXPORT_C TInt PowerResourceManager::AllocReserve(TUint aClientId, TUint8 aNumCl, TUint8 aNumRm)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::AllocReserve"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aNumCl 0x%02x", aNumCl));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aNumRm 0x%02x", aNumRm));
    return TInterface::AllocReserve(aClientId, aNumCl, aNumRm);
	}

/**
@publishedPartner
@prototype 9.5

Request to deregister client level from the specified resource for the specified client.

@param aClientId	ID of the client which is requesting the deregistration of client level.
@param aResourceId	ID of the resource from which to remove the specified clients 'client level'.

@return             KErrNone if successful
		            KErrAccessDenied if the client ID could not be found in the list of registered clients or
							         if the client was registered to be thread relative and this API is not 
							         called from the same thread.
		            KErrNotFound if the resource ID could not be found in the current list of controllable 
							     resources or if the client is not holding any level with the specified 
							     resource (no client level found for the specified client).

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver.
*/
EXPORT_C TInt PowerResourceManager::DeRegisterClientLevelFromResource(TUint aClientId, TUint aResourceId)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::DeRegisterClientLevelFromResource"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("aResourceId 0x%08x", aResourceId));
	return TInterface::DeRegisterClientLevelFromResource(aClientId, aResourceId);
	}

/**
@publishedPartner
@prototype 9.5

Interface to provide extended functionality.This provides support
to register and deregister dynamic resources and handling of resource dependency, registering
and deregistering resource dependency.
This is not supported in basic version
It is used for getting version (supported in both version). 

@pre Interrupts must be enabled
@pre Kernel must be unlocked
@pre No fast mutex can be held
@pre Call in a thread context but not from null thread or DFC thread1
@pre Can be used in a device driver.
*/
EXPORT_C TInt PowerResourceManager::ControlIO(TUint aClientId, TUint aFunction, TAny* aParam1, TAny* aParam2, 
											  TAny* aParam3)
    {
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">PowerResourceManager::ControlIO"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aClientId 0x%08x", aClientId));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aFunction %d", aFunction));
    return TInterface::ControlIO(aClientId, aFunction, aParam1, aParam2, aParam3);
	}


