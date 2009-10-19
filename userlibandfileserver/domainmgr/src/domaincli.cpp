// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// domain\src\domaincli.cpp
// 
//

#include <e32base.h>
#include <e32base_private.h>
#include <e32property.h>

#include <domainmember.h>
#include <domainmanager.h>
#include "domainobserver.h"
#include "domainsrv.h"

#define __DM_PANIC(aError) User::Panic(_L("domainCli.cpp"), (-(aError)) | (__LINE__ << 16))
#define __DM_ASSERT(aCond) __ASSERT_DEBUG(aCond,User::Panic(_L("domainCli.cpp; assertion failed"), __LINE__))

TInt RDmDomainSession::Connect(TDmHierarchyId aHierarchyId, TDmDomainId aDomainId, TUint* aKey)
	{
	TInt r = RSessionBase::CreateSession(KDmDomainServerNameLit, KDmDomainServerVersion, 1);
	if (r != KErrNone)
		return r;
	TIpcArgs a( (TInt)aHierarchyId, (TInt)aDomainId );
	r = RSessionBase::SendReceive(EDmDomainJoin, a);
	if (r != KErrNone)
		{
		RSessionBase::Close();
		return r;
		}
	*aKey = DmStatePropertyKey(
		aHierarchyId, 
		aDomainId);

	return KErrNone;
	}

void RDmDomainSession::Acknowledge(TInt aValue, TInt aError)
	{
	__DM_ASSERT(Handle() != KNullHandle);

	TIpcArgs a(aValue, aError);
	TInt r = RSessionBase::SendReceive(EDmStateAcknowledge, a);
	if (r != KErrNone)
		__DM_PANIC(r);
	}

void RDmDomainSession::RequestTransitionNotification()
	{
	__DM_ASSERT(Handle() != KNullHandle);
	TInt r = RSessionBase::SendReceive(EDmStateRequestTransitionNotification);
	if (r != KErrNone)
		__DM_PANIC(r);
	}

void RDmDomainSession::CancelTransitionNotification()
	{
	if (Handle() != KNullHandle)
		{
		TInt r = RSessionBase::SendReceive(EDmStateCancelTransitionNotification);
		if (r != KErrNone)
			__DM_PANIC(r);
		}
	}



/**
Connects to the domain identified by the specified domain Id.

To connect to the root domain, which has the Id KDmIdRoot,
the capability WriteDeviceData is required.

Once connected, an  application can use this RDmDomain object to read
the domain's power state and to request notification
when the power state changes.

@param aDomainId The identifier of the domain to be connected to.

@return KErrNone, if successful; otherwise one of the other system-wide
        or domain manager specific error codes.

@capability WriteDeviceData If aDomainId==KDmIdRoot
*/
EXPORT_C TInt RDmDomain::Connect(TDmDomainId aDomainId)
	{
	TUint key;
	TInt r = iSession.Connect(KDmHierarchyIdPower, aDomainId, &key);
	if (r != KErrNone)
		return r;
	r = iStateProperty.Attach(KUidDmPropertyCategory, key);
	if (r != KErrNone)
		{
		iSession.Close();
		return r;
		}
	return KErrNone;
	}
	



/**
Connects to the domain identified by the specified domain Id.

To connect to the root domain, which has the Id KDmIdRoot,
the capability WriteDeviceData is required.

Once connected, an  application can use this RDmDomain object to read
the domain's state and to request notification
when the state changes.

@param aHierarchyId	The Id of the domain hierarchy to connect to.
@param aDomainId    The identifier of the domain to be connected to.

@return KErrNone, if successful; otherwise one of the other system-wide
        or domain manager specific error codes.

@capability WriteDeviceData If aDomainId==KDmIdRoot
*/
EXPORT_C TInt RDmDomain::Connect(TDmHierarchyId aHierarchyId, TDmDomainId aDomainId)
	{
	TUint key;
	TInt r = iSession.Connect(aHierarchyId, aDomainId, &key);
	if (r != KErrNone)
		return r;
	r = iStateProperty.Attach(KUidDmPropertyCategory, key);
	if (r != KErrNone)
		{
		iSession.Close();
		return r;
		}
	return KErrNone;
	}
	

	
	
/**
Disconnects from the associated domain.
	
If this object is not connected to any domain, then it returns silently.
*/
EXPORT_C void RDmDomain::Close()
	{
	iSession.Close();
	iStateProperty.Close();
	}




/**
Requests notification when the domain's state changes.

This is an asynchronous request that completes when
the domain's state changes.

@param aStatus The request status object for this asynchronous request.

@see RDmDomain::CancelTransitionNotification()
*/
EXPORT_C void RDmDomain::RequestTransitionNotification(TRequestStatus& aStatus)
	{
	iStateProperty.Subscribe(aStatus);
	iSession.RequestTransitionNotification();
	}




/**
Cancels an outstanding notification request.

Any outstanding notification request completes with KErrCancel.
*/
EXPORT_C void RDmDomain::CancelTransitionNotification()
	{
	iSession.CancelTransitionNotification();
	iStateProperty.Cancel();
	}




/**
Gets the domain's power state.
	
An application normally calls this function after a notification request
has completed. It then performs any application-dependent action demanded by
the power state, and then acknowledges the state transition.

Note that the domain manager requires any domain power state change to be
acknowledged by all applications connected to the domain.

@return The connected domain's power state.

@see RDmDomain::AcknowledgeLastState()
*/
EXPORT_C TPowerState RDmDomain::GetPowerState()
	{
	TInt value;
	TInt r = iStateProperty.Get(value);
	if (r != KErrNone)
		__DM_PANIC(r);
	iLastStatePropertyValue = value;
	return (TPowerState) DmStateFromPropertyValue(value);
	}




/**
Acknowledges the state change.
	
An application must acknowledge that it has performed all actions required
by the last known state of the domain.
*/
EXPORT_C void RDmDomain::AcknowledgeLastState()
	{
	iSession.Acknowledge(iLastStatePropertyValue, KErrNone);
	}


/**
Acknowledges the state change with the specified error
	
An application must acknowledge that it has performed all actions required
by the last known state of the domain.

@param aError KDmErrNotJoin if domain is not part of the hierarhcy or a 
	system wide error value associated with the state change.
*/
EXPORT_C void RDmDomain::AcknowledgeLastState(TInt aError)
	{
	iSession.Acknowledge(iLastStatePropertyValue, aError);
	}



/**
Gets the domain's state.
	
An application normally calls this function after a notification request
has completed. It then performs any application-dependent action demanded by
the state, and then acknowledges the state transition.

Note, that the domain manager requires any domain state change to be
acknowledged by all applications connected to the domain.

@return The connected domain's state.
*/
EXPORT_C TDmDomainState RDmDomain::GetState()
	{
	TInt value;
	TInt r = iStateProperty.Get(value);
	if (r != KErrNone)
		__DM_PANIC(r);
	iLastStatePropertyValue = value;
	return DmStateFromPropertyValue(value);
	}

TInt RDmManagerSession::Connect()
	{
	__DM_ASSERT(Handle() == KNullHandle);

	return RSessionBase::CreateSession(
				KDmManagerServerNameLit, 
				KDmManagerServerVersion, 
				2);
	}

TInt RDmManagerSession::ConnectObserver(TDmHierarchyId aHierarchyId)
	{
	TInt r = Connect();
	if (r != KErrNone)
		return r;

	TIpcArgs a( (TInt)aHierarchyId);
	r = RSessionBase::SendReceive(EDmObserverJoin, a);
	if (r != KErrNone)
		{
		RSessionBase::Close();
		return r;
		}
	return KErrNone;
	}

TInt RDmManagerSession::Connect(TDmHierarchyId aHierarchyId)
	{
	TInt r = Connect();
	if (r != KErrNone)
		return r;

	TIpcArgs a( (TInt)aHierarchyId);
	r = RSessionBase::SendReceive(EDmHierarchyJoin, a);
	if (r != KErrNone)
		{
		RSessionBase::Close();
		return r;
		}
	return KErrNone;
	}

void RDmManagerSession::RequestDomainTransition(
	TDmDomainId aDomainId, 
	TDmDomainState aState,
	TDmTraverseDirection aDirection,
	TRequestStatus& aStatus)
	{
	__DM_ASSERT(Handle() != KNullHandle);
	
	if(aDirection < 0 || aDirection > ETraverseMax)
		__DM_PANIC(KErrArgument);

	TIpcArgs a(aDomainId, aState, aDirection);
	RSessionBase::SendReceive(EDmRequestDomainTransition, a, aStatus);
	}

void RDmManagerSession::CancelTransition()
	{
	if (Handle() != KNullHandle)
		{
		TInt r = RSessionBase::SendReceive(EDmCancelTransition);
		if (r != KErrNone)
			__DM_PANIC(r);
		}
	}

void RDmManagerSession::CancelObserver()
	{
	if (Handle() != KNullHandle)
		{
		TInt r = RSessionBase::SendReceive(EDmObserverCancel);
		if (r != KErrNone)
			__DM_PANIC(r);
		}
	}

void RDmManagerSession::RequestSystemTransition(
	TDmDomainState aState,
	TDmTraverseDirection aDirection,
	TRequestStatus& aStatus)
	{
	__DM_ASSERT(Handle() != KNullHandle);

	TIpcArgs a(aState, aDirection);
	RSessionBase::SendReceive(EDmRequestSystemTransition, a, aStatus);
	}

TInt RDmManagerSession::AddDomainHierarchy(TDmHierarchyId aHierarchyId)
	{
	__DM_ASSERT(Handle() != KNullHandle);

	TIpcArgs a( (TInt)aHierarchyId);
	TInt r = RSessionBase::SendReceive(EDmHierarchyAdd, a);

	return r;
	}

TInt RDmManagerSession::GetTransitionFailureCount()
	{
	__DM_ASSERT(Handle() != KNullHandle);

	return RSessionBase::SendReceive(EDmGetTransitionFailureCount);
	}

TInt RDmManagerSession::GetTransitionFailures(RArray<const TTransitionFailure>& aTransitionFailures)
	{
	__DM_ASSERT(Handle() != KNullHandle);

	aTransitionFailures.Reset();

	TInt err = KErrNone;

	TInt failureCount = GetTransitionFailureCount();
	if (failureCount <= 0)
		return failureCount;

	TTransitionFailure* failures = new TTransitionFailure[failureCount];
	if(failures == NULL)
		return(KErrNoMemory);
	TPtr8 dataPtr(reinterpret_cast<TUint8*>(failures), failureCount * sizeof(TTransitionFailure));

	TIpcArgs a(&dataPtr);
	err = RSessionBase::SendReceive(EDmGetTransitionFailures, a);
	
	if (err == KErrNone)
		{
		for (TInt i=0; i<failureCount; i++)
			aTransitionFailures.Append(failures[i]);
		}

	delete [] failures;

	return err;
	}

TInt RDmManagerSession::StartObserver(TDmDomainId aDomainId, TDmNotifyType aNotifyType)
	{
	__DM_ASSERT(Handle() != KNullHandle);
	
	TIpcArgs a(aDomainId,aNotifyType);
	return(RSessionBase::SendReceive(EDmObserverStart,a));
	}

void RDmManagerSession::GetNotification(TRequestStatus& aStatus)
	{
	__DM_ASSERT(Handle() != KNullHandle);
	RSessionBase::SendReceive(EDmObserverNotify,aStatus);
	}
	
TInt RDmManagerSession::GetEventCount()
	{
	__DM_ASSERT(Handle() != KNullHandle);
	return(RSessionBase::SendReceive(EDmObserverEventCount));
	}

TInt RDmManagerSession::GetEvents(RArray<const TTransInfo>& aTransitions)
	{
	__DM_ASSERT(Handle() != KNullHandle);

	aTransitions.Reset();


	TInt count = GetEventCount();
	// This shouldn't happen unless something gone terribly wrong
	if (count <= 0)
		return KErrGeneral;

	TTransInfo* trans = new TTransInfo[count];
	if(trans == NULL)
		return(KErrNoMemory);
	
	TPtr8 dataPtr(reinterpret_cast<TUint8*>(trans), count * sizeof(TTransInfo));

	TIpcArgs a(&dataPtr);
	TInt ret=RSessionBase::SendReceive(EDmObserverGetEvent, a);
	
	if(ret==KErrNone)
		{
		for (TInt i=0; i<count; i++)
			aTransitions.Append(trans[i]);
		}
	
	delete [] trans;
	return ret;
	
	}

TInt RDmManagerSession::ObserverDomainCount()
	{
	__DM_ASSERT(Handle() != KNullHandle);
	return(RSessionBase::SendReceive(EDmObserveredCount));
	}

/**
@internalAll
@released
*/
EXPORT_C TInt RDmDomainManager::WaitForInitialization()
	{
	RProperty prop;
	TInt r = prop.Attach(KUidDmPropertyCategory, KDmPropertyKeyInit);
	if (r != KErrNone)
		return r;

#ifdef _DEBUG
	TInt count = RThread().RequestCount();
#endif

	TRequestStatus status;
	for (;;)
		{
		prop.Subscribe(status);
		TInt value;
		r = prop.Get(value);
		if (r == KErrNone)
			{
			if (value) break; // initialized
			// property exists but the server is not intialized yet
			}
		else
			{
			if (r != KErrNotFound) break; // error
			// property doesn't exist yet
			}
		User::WaitForRequest(status);
		if (status.Int() != KErrNone)
			break;	// error
		}

	if (status.Int() == KRequestPending)
		{
		prop.Cancel();
		User::WaitForRequest(status);
		}
	prop.Close();

	__DM_ASSERT(RThread().RequestCount() == count);

	return r;
	}




/**
Opens a controlling connection to the standard power domain hierarchy
in the domain manager.

The domain manger allows only one open connection at any one time to the 
power domain hierarchy.
Connection is usually made by the power policy entity.

@return KErrNone, if successful; otherwise one of the other system-wide
        or the domain manager specific error codes.
        
@see KDmErrAlreadyJoin   
*/
EXPORT_C TInt RDmDomainManager::Connect()
	{
	return iSession.Connect(KDmHierarchyIdPower);
	}




/**
Opens a controlling connection to a specific domain hieararchy owned 
by the domain manager.

The domain manger allows only one open connection at any one time to a 
particular hierarchy.

@param	aHierarchyId	The Id of the domain hierarchy to connect to.

@return KErrNone, if successful; otherwise one of the other system-wide
        or domain manager specific error codes.
        
@see KDmErrAlreadyJoin
@see KErrBadHierarchyId       
*/
EXPORT_C TInt RDmDomainManager::Connect(TDmHierarchyId aHierarchyId)

	{
	return iSession.Connect(aHierarchyId);
	}




/**
Closes this connection to the domain manager.
	
If there is no existing connection, then it returns silently.
*/
EXPORT_C void RDmDomainManager::Close()
	{
	iSession.Close();
	}




/**
Requests a system-wide power state transition.

The domain hierarchy is traversed in the default direction
		
@param aState   The target power state.
@param aStatus  The request status object for this asynchronous request.

@see RDmDomainManager::CancelTransition()
*/
EXPORT_C void RDmDomainManager::RequestSystemTransition(TPowerState aState, TRequestStatus& aStatus)
	{
	if (aState == EPwActive)
		{
		TRequestStatus* status = &aStatus;
		User::RequestComplete(status, KErrArgument);
		return;
		}
	RequestSystemTransition((TDmDomainState) aState, ETraverseDefault, aStatus);
	}




/**
Requests a system-wide power shutdown.

This is a request to change the system's power state to EPwOff.
This call does not return; the system can only return by rebooting.
*/
EXPORT_C void RDmDomainManager::SystemShutdown()
	{
	TRequestStatus status;
	RequestSystemTransition((TDmDomainState) EPwOff, ETraverseDefault, status);
	User::WaitForRequest(status);
	__DM_ASSERT(0);
	}




/**
Requests a domain state transition.

The domain hierarchy is traversed in the default direction.

@param aDomainId The Id of the domain for which the state transition
                 is being requested.
@param aState    The target state.
@param aStatus   The request status object for this asynchronous request.

@see RDmDomainManager::CancelTransition()
*/
EXPORT_C void RDmDomainManager::RequestDomainTransition(
	TDmDomainId aDomainId, 
	TPowerState aState, 
	TRequestStatus& aStatus)
	{
	RequestDomainTransition(aDomainId,(TDmDomainState)  aState, ETraverseDefault, aStatus);
	}




/**
Cancels a state transition, whether initiated by a call
to RequestSystemTransition() or RequestDomainTransition().

An outstanding state transition request completes with KErrCancel.
*/
EXPORT_C void RDmDomainManager::CancelTransition()
	{
	iSession.CancelTransition();
	}




/**
Requests a system-wide state transition.

The domain hierarchy is traversed in the specified direction.
		
@param aState   The target state.
@param aDirection The direction in which to traverse the hierarchy
@param aStatus  The request status object for this asynchronous request.

@see RDmDomainManager::CancelTransition()

@panic domainCli.cpp; assertion failed VARNUM if the numerical value of aDirection
       is greater than the value of ETraverseMax. NOTE: VARNUM is the line number
       in the source code and may change if the implementation changes.
*/
EXPORT_C void RDmDomainManager::RequestSystemTransition(
	TDmDomainState aState, 
	TDmTraverseDirection aDirection, 
	TRequestStatus& aStatus)
	{
	__DM_ASSERT(aDirection <= ETraverseMax);
	iSession.RequestSystemTransition(aState, aDirection, aStatus);
	}




/**
Requests a domain state transition.

The domain hierarchy is traversed in the specified direction

@param aDomainId The Id of the domain for which the state transition
                 is being requested.
@param aState    The target state.
@param aDirection The direction in which to traverse the hierarchy.
@param aStatus   The request status object for this asynchronous request.

@see RDmDomainManager::CancelTransition()

@panic domainCli.cpp; assertion failed VARNUM if the numerical value of aDirection
       is greater than the value of ETraverseMax. NOTE: VARNUM is the line number
       in the source code and may change if the implementation changes.
*/
EXPORT_C void RDmDomainManager::RequestDomainTransition(
	TDmDomainId aDomainId, 
	TDmDomainState aState, 
	TDmTraverseDirection aDirection,
	TRequestStatus& aStatus)
	{
	__DM_ASSERT(aDirection <= ETraverseMax);
	iSession.RequestDomainTransition(aDomainId, aState, aDirection, aStatus);
	}




/**
Adds a domain hierarchy to the domain manager.

@param aHierarchyId The Id of the domain hierarchy to be added.

@return	KErrNone if successful; otherwise one of the other system-wide
        or domain manager specific error codes.
*/
EXPORT_C TInt RDmDomainManager::AddDomainHierarchy(TDmHierarchyId aHierarchyId)
	{
	RDmManagerSession	session;
	TInt r = session.Connect();
	if (r != KErrNone)
		return r;
	r = session.AddDomainHierarchy(aHierarchyId);
	session.Close();
	return r;
	}



/**
Requests a list of transition failures since the last transition request.

@param aTransitionFailures A client-supplied array of TTransitionFailure objects which 
		on exit will contain the failures that have occurred since the last transition 
		request. 
@pre	The session must be connected.

@return KErrNone, if successful; otherwise one of the other system-wide
        or domain manager specific error codes.
*/
EXPORT_C TInt RDmDomainManager::GetTransitionFailures(RArray<const TTransitionFailure>& aTransitionFailures)
	{
	return iSession.GetTransitionFailures(aTransitionFailures);
	}



/**
Gets the number of transition failures since the last transition request.

@return	The number of failures, if successful; otherwise one of the other system-wide
        or domain manager specific error codes.
*/
EXPORT_C TInt RDmDomainManager::GetTransitionFailureCount()
	{
	return iSession.GetTransitionFailureCount();
	}



// CDmDomain

/**
Constructor.

Adds this active object to the active scheduler. The priority of the active object
is the standard value, i.e. CActive::EPriorityStandard.

@param aHierarchyId The Id of the domain hierarchy to connect to.
@param aDomainId	The Id of the domain to connect to.

@see CActive
@see CActive::TPriority
*/
EXPORT_C CDmDomain::CDmDomain(TDmHierarchyId aHierarchyId, TDmDomainId aDomainId) : 
	CActive(CActive::EPriorityStandard), 
	iHierarchyId(aHierarchyId), 
	iDomainId(aDomainId)
	{
	CActiveScheduler::Add(this);
	}




/**
Destructor.

Closes the session to the domain manager.
*/
EXPORT_C CDmDomain::~CDmDomain()
	{
	Cancel();
	iDomain.Close();
	}




/**
Second-phase constructor.

The function attempts to connect to the domain specified in the constructor.

@leave One of the system-wide error codes
*/
EXPORT_C void CDmDomain::ConstructL()
	{
	User::LeaveIfError(iDomain.Connect(iHierarchyId, iDomainId));
	}




/**
Requests notification when the domain's state changes.

RunL() will be called when this happens.
*/
EXPORT_C void CDmDomain::RequestTransitionNotification()
	{
	__DM_ASSERT(!IsActive());
	iDomain.RequestTransitionNotification(iStatus);
	SetActive();
	}




/**
Cancels an outstanding notification request.

Any outstanding notification request completes with KErrCancel.
*/
EXPORT_C void CDmDomain::DoCancel()
	{
	iDomain.CancelTransitionNotification();
	}




/**
Acknowledges the last state change.
	
An application must acknowledge that it has performed all actions required
by the last known state of the domain.

@param aError	The error to return to the domain manager. The client should
				set this to KErrNone if it successfully transitioned to the 
				new state or to one of the system-wide error codes.
*/
EXPORT_C void CDmDomain::AcknowledgeLastState(TInt aError)
	{
	iDomain.AcknowledgeLastState(aError);
	}




/**
Gets the domain's state.
	
An application normally calls this function after a notification request
has completed. It then performs any application-dependent action demanded by
the state, and then acknowledges the state transition.

@return The connected domain's state.
*/
EXPORT_C TDmDomainState CDmDomain::GetState()
	{
	return iDomain.GetState();
	}

// CDmDomainManager

/**
Constructor.

Adds this active object to the active scheduler.

@param aHierarchyId The Id of the domain hierarchy to connect to
*/
EXPORT_C CDmDomainManager::CDmDomainManager(TDmHierarchyId aHierarchyId) : 
	CActive(CActive::EPriorityStandard), 
	iHierarchyId(aHierarchyId)
	{
	CActiveScheduler::Add(this);
	}




/**
Destructor.

Closes the session to the domain manager.
*/
EXPORT_C CDmDomainManager::~CDmDomainManager()
	{
	Cancel();
	iManager.Close();
	}




/**
The second-phase constructor.

This function attempts to connect to the hierarchy 
specified in the constructor.

@leave One of the system-wide error codes.
*/
EXPORT_C void CDmDomainManager::ConstructL()
	{
	User::LeaveIfError(iManager.Connect(iHierarchyId));
	}




/**
Requests a system-wide state transition.

The domain hierarchy is traversed in the specified direction
		
@param aState   The target state.
@param aDirection The direction in which to traverse the hierarchy.
*/
EXPORT_C void CDmDomainManager::RequestSystemTransition(TDmDomainState aState, TDmTraverseDirection aDirection)
	{
	__DM_ASSERT(!IsActive());
	iStatus = KRequestPending;
	iManager.RequestSystemTransition(aState, aDirection, iStatus);
	SetActive();
	}




/**
Requests a domain state transition.

The domain hierarchy is traversed in the specified direction.

@param aDomain The Id of the domain for which the state transition
                 is being requested.
@param aState    The target state.
@param aDirection The direction in which to traverse the hierarchy.
*/
EXPORT_C void CDmDomainManager::RequestDomainTransition(TDmDomainId aDomain, TDmDomainState aState, TDmTraverseDirection aDirection)
	{
	__DM_ASSERT(!IsActive());
	iStatus = KRequestPending;
	iManager.RequestDomainTransition(aDomain, aState, aDirection, iStatus);
	SetActive();
	}




/**
Adds a domain hierarchy to the domain manager.

@param aHierarchyId The Id of the domain hierarchy to add

@return	KErrNone if successful; otherwise one of the other system-wide
        or domain manager specific error codes.
*/
EXPORT_C TInt CDmDomainManager::AddDomainHierarchy(TDmHierarchyId aHierarchyId)
	{
	RDmManagerSession	session;
	TInt r = session.Connect();
	if (r != KErrNone)
		return r;
	r = session.AddDomainHierarchy(aHierarchyId);
	session.Close();
	return r;

	}




/**
Cancels a pending event.
*/
EXPORT_C void CDmDomainManager::DoCancel()
	{
	iManager.CancelTransition();
	}




/**
Requests a list of transition failures since the last transition request.

@param aTransitionFailures A client-supplied array of TTransitionFailure objects which 
		on exit will contain the failures that have occurred since the last transition 
		request. 
@pre	The session must be connected.

@return KErrNone, if successful; otherwise one of the other system-wide
        or domain manager specific error codes.
*/
EXPORT_C TInt CDmDomainManager::GetTransitionFailures(RArray<const TTransitionFailure>& aTransitionFailures)
	{
	return iManager.GetTransitionFailures(aTransitionFailures);
	}




/**
Gets the number of transition failures since the last transition request.

@return	The number of failures if successful, otherwise one of the other system-wide
        or domain manager specific error codes.
*/
EXPORT_C TInt CDmDomainManager::GetTransitionFailureCount()
	{
	return iManager.GetTransitionFailureCount();
	}




CHierarchyObserver::CHierarchyObserver(MHierarchyObserver& aHierarchyObserver, TDmHierarchyId aHierarchyId):
	CActive(CActive::EPriorityStandard), 
	iHierarchyId(aHierarchyId),
	iObserver(aHierarchyObserver)
	{
	iTransitionEvents.Reset();
	CActiveScheduler::Add(this);
	}




/**
Constructs a new observer on the domain hierarchy.

Note that only one observer per domain hierarchy is allowed.

@param	aHierarchyObserver	The implementation of the interface to the domain manager.
@param	aHierarchyId		The Id of the domain hierarchy.

@return The newly created CHierarchyObserver object.
*/
EXPORT_C CHierarchyObserver* CHierarchyObserver::NewL(MHierarchyObserver& aHierarchyObserver,TDmHierarchyId aHierarchyId)
	{
	CHierarchyObserver* observer=new(ELeave)CHierarchyObserver(aHierarchyObserver,aHierarchyId);

	CleanupStack::PushL(observer);
	User::LeaveIfError(observer->iSession.ConnectObserver(aHierarchyId));
	CleanupStack::Pop();
	
	return(observer);
	}




/**
Destructor.

Frees resources prior to destruction of the object.
*/
EXPORT_C CHierarchyObserver::~CHierarchyObserver()
	{
	Cancel();
	iSession.Close();
	iTransitionEvents.Reset();
	}

void CHierarchyObserver::DoCancel()
	{
	iObserverStarted=EFalse;
	iSession.CancelObserver();
	}

void CHierarchyObserver::RunL()
//
// Process the reply to client's request for domain transition/failure
//
	{

	TInt ret= iSession.GetEvents(iTransitionEvents);
	
	User::LeaveIfError(ret);
	
	TInt count = iTransitionEvents.Count();

	for(TInt i=0;i<count;i++)
		{
		if(iTransitionEvents[i].iError==KErrNone)
			iObserver.TransProgEvent(iTransitionEvents[i].iDomainId,iTransitionEvents[i].iState);
		else if(iTransitionEvents[i].iError==KDmErrOutstanding)
			iObserver.TransReqEvent(iTransitionEvents[i].iDomainId,iTransitionEvents[i].iState);
		else
			iObserver.TransFailEvent(iTransitionEvents[i].iDomainId,iTransitionEvents[i].iState,iTransitionEvents[i].iError);
		}

	GetNotification();
	}




/**
Starts the observer. 

@param	aDomainId		The Id of the domain to which the obsever is attached.
@param	aNotifyType		The type of notifications of interest to the observer.

@return KErrNone on successful start, otherwise one of the other system wide error codes.
*/
EXPORT_C TInt CHierarchyObserver::StartObserver(TDmDomainId aDomainId, TDmNotifyType aNotifyType)
	{
	iNotifyType=aNotifyType;
	iDomainId=aDomainId;
	
	TInt ret=iSession.StartObserver(iDomainId, iNotifyType);
	if(ret!=KErrNone)
		return ret;
	iObserverStarted=ETrue;
	GetNotification();
	return KErrNone;
	}




/**
Stops the observer. 

@return KErrNone if successful; KDmErrBadSequence, if the observer 
        has not already started.
*/
EXPORT_C TInt CHierarchyObserver::StopObserver()
	{
	if(!iObserverStarted)
		return(KDmErrBadSequence);
	Cancel();
	return(KErrNone);
	}

void CHierarchyObserver::GetNotification()
	{
	iSession.GetNotification(iStatus);
	SetActive();
	}

/**
Gets the number of domains that are being observed.

This value is the number of children of the domain member to which the observer
is attached, including itself. 

@return The number of observed domain members.
        One of the other system wide error codes may be returned on failure;
        specifically KErrNotFound if the observer is not already started.
*/
EXPORT_C TInt CHierarchyObserver::ObserverDomainCount()
	{
	if(!iObserverStarted)
		return KErrNotFound;
	return(iSession.ObserverDomainCount());	
	}
