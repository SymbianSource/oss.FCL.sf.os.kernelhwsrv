// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "domainmanager_private.h"
#include "domainobserver.h"
#include "domainsrv.h"
#include "domainmember_private.h"

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

TInt RDmDomainSession::Acknowledge(TInt aValue, TInt aError)
	{
	__DM_ASSERT(Handle() != KNullHandle);

	TIpcArgs a(aValue, aError);
	return RSessionBase::SendReceive(EDmStateAcknowledge, a);
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


void RDmDomainSession::DeferAcknowledgement(TRequestStatus& aStatus)
	{
	__DM_ASSERT(Handle() != KNullHandle);
	RSessionBase::SendReceive(EDmStateDeferAcknowledgement, aStatus);
	}


void RDmDomainSession::CancelDeferral()
	{
	if (Handle() != KNullHandle)
		{
		TInt r = RSessionBase::SendReceive(EDmStateCancelDeferral);
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
        or Domain Manager specific error codes.

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
        or Domain Manager specific error codes.

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

Note that the Domain Manager requires any domain power state change to be
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
	TInt r = iSession.Acknowledge(iLastStatePropertyValue, KErrNone);

	if (r != KErrNone && r != KErrNotFound)
		__DM_PANIC(r);
	}

/**
Acknowledges the state change with the specified error
	
An application must acknowledge that it has performed all actions required
by the last known state of the domain.

@param aError KDmErrNotJoin if domain is not part of the hierarchy or a
	system wide error value associated with the state change.
*/
EXPORT_C void RDmDomain::AcknowledgeLastState(TInt aError)
	{
	TInt r = iSession.Acknowledge(iLastStatePropertyValue, aError);

	if (r != KErrNone && r != KErrNotFound)
		__DM_PANIC(r);
	}

/**
Acknowledges the state change with the specified error

@param aError KDmErrNotJoin if domain is not part of the hierarchy or a
	system wide error value associated with the state change.

@return KErrNone If the acknowledgment was valid, KErrNotFound if it was spurious
*/
TInt RDmDomain::AcknowledgeLastStatePriv(TInt aError)
	{
	return iSession.Acknowledge(iLastStatePropertyValue, aError);
	}

/**
Having received a state transition notification, instead of acknowledging,
request more time. To be sure of deferring in time a client should call this immediately
after receiving notification. This asynchronous call will complete once the original deadline
is reached (ie. one period earlier than the final deadline), at which point the member must either
defer again or acknowledge the transition. In the meantime, the member should perform
its transition actions, whilst remaining responsive to new completion events.

For example, if after receiving a transition notification, the client calls DeferAcknowledgement
once, but fails to acknowledge or renew its deferral, it would be timed out after two time periods.

Once the member has completed all necessary actions it should call AcknowledgeLastState
to indicate a successful transition (it need not wait for the completion of DeferAcknowledgement).

@note Deferrals are not always possible,
whether the member will actually be given more time depends on if
   - The current transition allows deferrals at all.
   - The member still has deferrals left - there may be a maximum number
     allowed.
   - The deferral request was received in time.

@param aStatus Status of request
   - KErrNone Request has completed i.e. The member must either defer again or acknowledge.
   - KErrCompletion The deferral was obsoleted by a subsequent call to AcknowledgeLastState.
   - KErrNotSupported The current transition may not be deferred, or maximum deferral count reached.
   - KErrCancel The deferral was cancelled.
   - KErrNotReady Deferral attempted before a transition notification was received
     or after the deadline for the previous one.
   - KErrPermissionDenied The member lacked the necessary capabilities.
   - KErrServerBusy A deferral was already outstanding.

This function is provided for members to inform the Domain Manager that they
are still active and are responding to a transition notification.
For example, a server may have to persist data using
the file server before shut down. Since this task should be allowed to complete
before shutdown continues, the member should defer the transition, and then persist
the data, using asynchronous calls.

At least one of the below capabilities is required in order to defer a
domain transition. Without them, the client will get KErrPermissionDenied.

@capability WriteDeviceData
@capability ProtServ

@pre The member has been notified of a transition which it has not yet acknowledged
*/
EXPORT_C void RDmDomain::DeferAcknowledgement(TRequestStatus& aStatus)
	{
	iSession.DeferAcknowledgement(aStatus);
	}


/**
Will cancel a call of DeferAcknowledgement(), if one was pending.

If none was pending, it does nothing.
*/
EXPORT_C void RDmDomain::CancelDeferral()
	{
	iSession.CancelDeferral();
	}


/**
Gets the domain's state.

An application normally calls this function after a notification request
has completed. It then performs any application-dependent action demanded by
the state, and then acknowledges the state transition.

Note, that the Domain Manager requires any domain state change to be
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
	
	// Pre-allocate array with a known size which for this case is the value in failureCount 
	// in order to guarantee that future append operations to the array aTransitionFailures would
	// not fail. This is assuming that the pre-allocated size is not exceeded.
	err=aTransitionFailures.Reserve(failureCount); 
	if (err != KErrNone)
		return err;
		
	TTransitionFailure* failures = new TTransitionFailure[failureCount];
	if(failures == NULL)
		{		
		aTransitionFailures.Reset();
		return(KErrNoMemory);
		}
	
	TPtr8 dataPtr(reinterpret_cast<TUint8*>(failures), failureCount * sizeof(TTransitionFailure));

	TIpcArgs a(&dataPtr);
	err = RSessionBase::SendReceive(EDmGetTransitionFailures, a);

	if (err == KErrNone)
		{
		for (TInt i=0; i<failureCount; i++)	
			{
			err = aTransitionFailures.Append(failures[i]);		
			//The pre-allocation made above for the array aTransitionFailures should guarantee
			//that append operations complete succesfully.			
			__DM_ASSERT(err == KErrNone);	
			}
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

	// Pre-allocate array with a known size which for this case is the value in count 
	// in order to guarantee that future append operations to the array aTransitionFailures 
	// would not fail. This is assuming that the pre-allocated size is not exceeded.
	TInt ret=aTransitions.Reserve(count); 
	if (ret != KErrNone)
		return ret;

	TTransInfo* trans = new TTransInfo[count];
	if(trans == NULL)
		{
		aTransitions.Reset();
		return(KErrNoMemory);
		}
	
	TPtr8 dataPtr(reinterpret_cast<TUint8*>(trans), count * sizeof(TTransInfo));

	TIpcArgs a(&dataPtr);
	ret=RSessionBase::SendReceive(EDmObserverGetEvent, a);
	
	if(ret==KErrNone)
		{
		for (TInt i=0; i<count; i++)
			{
			ret = aTransitions.Append(trans[i]);					
			//The pre-allocation made above for the array aTransitions should guarantee
			//that append operations complete succesfully.
			__DM_ASSERT(ret == KErrNone);					
			}
		}
	
	delete [] trans;
	return ret;
	
	}

TInt RDmManagerSession::ObserverDomainCount()
	{
	__DM_ASSERT(Handle() != KNullHandle);
	return(RSessionBase::SendReceive(EDmObserveredCount));
	}



//-- RDmDomainManager ---------------------------------------------------------



/**
Caller blocked until the Domain Manager server has started up and is ready 
for requests.

@return	KErrNone once the Domain Manager server is ready, otherwise one of the 
		other system wide or the Domain Manager specific error codes.
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
			// property exists but the server is not initialised yet
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
in the Domain Manager.

The Domain Manager allows only one open connection at any one time to the 
power domain hierarchy.

@return KErrNone if successful, otherwise one of the other system-wide
        or the Domain Manager specific error codes.
@return KErrInUse when the power hierarchy already has a controller connected.
@return KErrBadHierarchyId when the server has failed to load the power hierarchy. 
@return KErrPermissionDenied when the client has insufficient capabilities

@capability PowerMgmt Required to create a connection to the Domain Manager.
@see KDmHierarchyIdPower
*/
EXPORT_C TInt RDmDomainManager::Connect()
	{
	return iSession.Connect(KDmHierarchyIdPower);
	}


/**
Opens a controlling connection to a specific domain hierarchy previously 
loaded into the Domain Manager by the controller. The Domain Manager allows only 
one open connection at any one time to a particular hierarchy.

@param	aHierarchyId The Id of the domain hierarchy to connect to.

@return KErrNone if successful, otherwise one of the other system-wide
        or the Domain Manager specific error codes.
@return KErrInUse when the power hierarchy already has a controller connected.
@return KErrBadHierarchyId when the server has failed to load the power hierarchy.
@return KErrPermissionDenied when the client has insufficient capabilities
   
@capability PowerMgmt Required to create a connection to the Domain Manager.
*/
EXPORT_C TInt RDmDomainManager::Connect(TDmHierarchyId aHierarchyId)
	{
	return iSession.Connect(aHierarchyId);
	}


/**
Closes this connection to the Domain Manager.
	
If there is no existing connection, then it returns silently.
*/
EXPORT_C void RDmDomainManager::Close()
	{
	iSession.Close();
	}


/**
Requests a system-wide power state transition and is used with the
KDmHierarchyIdPower hierarchy. The domain hierarchy is traversed in the 
default direction. 

A transition to the power state EPwActive is an error and result in 
async completion with KErrArgument.
	
@param aState   The target power state, not EPwActive or >=EPwLimit.
@param aStatus	The request status object for this asynchronous request.

@see RDmDomainManager::CancelTransition()
@see KDmHierarchyIdPower
*/
EXPORT_C void RDmDomainManager::RequestSystemTransition(TPowerState aState, TRequestStatus& aStatus)
	{
	if ((aState == EPwActive) || (aState >= EPwLimit))
		{
		TRequestStatus* status = &aStatus;
		User::RequestComplete(status, KErrArgument);
		return;
		}
	RequestSystemTransition((TDmDomainState) aState, ETraverseDefault, aStatus);
	}


/**
Requests a system-wide power off (EPwOff) state transition to shutdown the 
platform. Applicable to the KDmHierarchyIdPower hierarchy.

This call does not return; the system can only return by physical button restart.

@see KDmHierarchyIdPower
*/
EXPORT_C void RDmDomainManager::SystemShutdown()
	{
	TRequestStatus status;
	RequestSystemTransition((TDmDomainState) EPwOff, ETraverseDefault, status);
	User::WaitForRequest(status);
	__DM_ASSERT(0);
	}


/**
Requests a domain power state transition and is used with the
KDmHierarchyIdPower hierarchy. The domain hierarchy is traversed in the 
default direction.

@param aDomainId The Id of the domain for which the state transition
                 is being requested.
@param aState    The target power state.
@param aStatus   The request status object to receive the asynchronous result.
				 KErrNone if successful, otherwise one of the other system-wide
        		 or the Domain Manager specific error codes.
        		 
@see KDmHierarchyIdPower
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
Cancels an outstanding state transition, whether initiated by a call
to RequestSystemTransition() or RequestDomainTransition().

The outstanding state transition request completes with KErrCancel.
*/
EXPORT_C void RDmDomainManager::CancelTransition()
	{
	iSession.CancelTransition();
	}


/**
Requests a state transition across the whole domain hierarchy that 
this connection is controlling. The domain hierarchy is traversed in the 
specified direction.
		
@param aState	The target state, hierarchy specific state value.
@param aDirection The direction in which to traverse the hierarchy
@param aStatus  The request status object to receive the asynchronous result.
				KErrNone if successful, otherwise one of the other system-wide
        		or the Domain Manager specific error codes.

@panic domainCli.cpp; assertion failed line - if the numerical value of 
		aDirection is greater than the value of ETraverseMax. 

@see RDmDomainManager::CancelTransition()
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
Requests a domain state transition for the hierarchy that this connection is 
controlling. The domain hierarchy is traversed in the specified direction.

@param aDomainId The Id of the domain for which the state transition
                 is being requested.
@param aState    The target state.
@param aDirection The direction in which to traverse the hierarchy.
@param aStatus   The request status object to receive the asynchronous result.
				 KErrNone if successful, otherwise one of the other system-wide
        		 or the Domain Manager specific error codes.

@panic domainCli.cpp; assertion failed VARNUM if the numerical value of aDirection
       is greater than the value of ETraverseMax. NOTE: VARNUM is the line number
       in the source code and may change if the implementation changes.
       
@see RDmDomainManager::CancelTransition()
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
Instructs the Domain Manager to load the domain hierarchy library for
aHierarchyId. The library takes the name "domainpolicyNN.dll" where NN is 
the hierarchy number as supplied.

@param aHierarchyId The Id of the domain hierarchy to be added.

@return KErrNone if successful, otherwise one of the other system-wide
        or the Domain Manager specific error codes.
@return KErrBadHierarchyId If the library is not found or contains invalid data
@return KErrPermissionDenied when the client has insufficient capabilities

@capability PowerMgmt Required to create a connection to the Domain Manager.
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

@param aTransitionFailures A client supplied array of TTransitionFailure objects
		which on exit will contain the failures that have occurred since the 
		last transition	request. 

@return KErrNone if successful, otherwise one of the other system-wide
        or the Domain Manager specific error codes.
*/
EXPORT_C TInt RDmDomainManager::GetTransitionFailures(RArray<const TTransitionFailure>& aTransitionFailures)
	{
	return iSession.GetTransitionFailures(aTransitionFailures);
	}


/**
Gets the number of transition failures since the last transition request.

@return KErrNone if successful, otherwise one of the other system-wide
        or the Domain Manager specific error codes.
*/
EXPORT_C TInt RDmDomainManager::GetTransitionFailureCount()
	{
	return iSession.GetTransitionFailureCount();
	}



//-- CDmDomain ----------------------------------------------------------------



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

Closes the session to the Domain Manager.
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

@note
If the client is ready to acknowledge the last transition, but
would like to register for notification of the next one, it
should call this function and then call AcknowledgeLastState,
immediately afterwards.
This eliminates the possibility of a transition occurring
between acknowledging and registering for notification.
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

@param aError	The error to return to the Domain Manager. The client should
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

//-- CDmDomainKeepAlive ----------------------------------------------------------------


/**
Constructor.

Adds this active object to the active scheduler.

@param aHierarchyId The Id of the domain hierarchy to connect to.
@param aDomainId	The Id of the domain to connect to.

@see CActive
*/
EXPORT_C CDmDomainKeepAlive::CDmDomainKeepAlive(TDmHierarchyId aHierarchyId, TDmDomainId aDomainId)
	: CDmDomain(aHierarchyId, aDomainId), iKeepAlive(NULL)
	{
	}

/**
Destructor.

Cleanup the internal CDmKeepAlive active object.
*/
EXPORT_C CDmDomainKeepAlive::~CDmDomainKeepAlive()
	{
	delete iKeepAlive;
	}

/**
Complete construction of this object. Classes derived from this one
should call this as part of their ConstructL or NewL functions.
*/
EXPORT_C void CDmDomainKeepAlive::ConstructL()
	{
	CDmDomain::ConstructL();
	iKeepAlive = new (ELeave) CDmKeepAlive(iDomain, *this);
	}

/**
Acknowledges the last state change.

An application must acknowledge that it has performed all actions required
by the last known state of the domain.

Once this is done the AO will no longer attempt to defer deadlines.

@param aError	The error to return to the Domain Manager. The client should
				set this to KErrNone if it successfully transitioned to the 
				new state or to one of the system-wide error codes.
*/
EXPORT_C void CDmDomainKeepAlive::AcknowledgeLastState(TInt aError)
	{
	TInt r = iDomain.AcknowledgeLastStatePriv(aError);
	if (r != KErrNone && r != KErrNotFound)
		__DM_PANIC(r);

	if (r == KErrNone)
		{
		// KErrNone indicates that an acknowledgment was accepted
		// (as opposed to being spurious or late)

		iKeepAlive->NotifyOfAcknowledgment();
		}
	}

/**
Handle completion of request notifications, begins deferrals.

@note Clients should not need to override this, they
will be notified of events via calls to HandleTransitionL.
*/
EXPORT_C void CDmDomainKeepAlive::RunL()
	{
	iKeepAlive->DeferNotification();
	HandleTransitionL();
	}

/**
This object will internally, use RDmDomain::DeferAcknowledgement.

The default implementation of this function will simply ignore errors
from RDmDomain::DeferAcknowledgement that
it presumes the client can not or need not handle.

ie.
KErrCompletion - Client has now acknowledged notification - not an error.
KErrCancel - Server cancelled request - client can do nothing.
KErrNotSupported - Deferral not possible - client can do nothing.
KErrNotReady - Deferral too late or too early - client can do nothing.

All other error codes will be returned unhandled to the active scheduler,
leading to a panic.

If the client does want to handle or inspect these errors e.g. for diagnostic
purposes, they can override this method. KErrNone should be returned
for errors that should not be passed to the active scheduler.

@param aError Error code to handle
*/
EXPORT_C TInt CDmDomainKeepAlive::HandleDeferralError(TInt aError)
	{
	switch (aError)
		{
		case KErrCompletion:
		case KErrCancel:
		case KErrNotSupported:
		case KErrNotReady:
			{
			// All the above error codes may occur but signal only
			// that Deferrals should not continue for the current
			// transition.
			return KErrNone;
			}
		case KErrPermissionDenied:
		case KErrServerBusy:
		default:
			{
			return aError;
			}
		}
	}

//-- CDmKeepAlive ----------------------------------------------------------------

CDmKeepAlive::CDmKeepAlive(RDmDomain& aDomain, CDmDomainKeepAlive& aOwnerActiveObject)
	: CActive(CActive::EPriorityHigh), iDomain(aDomain), iOwnerActiveObject(aOwnerActiveObject), iCeaseDeferral(EFalse)
	{
	CActiveScheduler::Add(this);
	}

CDmKeepAlive::~CDmKeepAlive()
	{
	Cancel();
	}

void CDmKeepAlive::DeferNotification()
	{
	__DM_ASSERT(!IsActive());
	iStatus = KRequestPending;
	iDomain.DeferAcknowledgement(iStatus);
	SetActive();
	}

/**
Informs the object that the state transition has
been _successfully_ acknowledged
*/
void CDmKeepAlive::NotifyOfAcknowledgment()
	{
	if (IsActive())
		{
		iCeaseDeferral = ETrue;
		}
	}

void CDmKeepAlive::RunL()
	{
	const TInt error = iStatus.Int();

	TBool ceaseDeferral = iCeaseDeferral;
	iCeaseDeferral = EFalse;

	User::LeaveIfError(error);

	// If a valid acknowledgment
	// has occured since the last deferral
	// then avoid deferring again since
	// this would only lead to KErrNotReady
	if(!ceaseDeferral)
		{
		DeferNotification();
		}
	else
		{
		// At this point we know error == KErrNone
		// However, we return the error code KErrCompletion, as this
		// is what would have happened, had the acknowledgment come in
		// a little earlier,
		// whilst the deferral was still outstanding on the server.
		User::Leave(KErrCompletion);
		}
	}

TInt CDmKeepAlive::RunError(TInt aError)
	{
	return iOwnerActiveObject.HandleDeferralError(aError);
	}

void CDmKeepAlive::DoCancel()
	{
	iDomain.CancelDeferral();
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

Closes the session to the Domain Manager.
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
Adds a domain hierarchy to the Domain Manager.

@param aHierarchyId The Id of the domain hierarchy to add

@return	KErrNone if successful; otherwise one of the other system-wide
        or Domain Manager specific error codes.
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
        or Domain Manager specific error codes.
*/
EXPORT_C TInt CDmDomainManager::GetTransitionFailures(RArray<const TTransitionFailure>& aTransitionFailures)
	{
	return iManager.GetTransitionFailures(aTransitionFailures);
	}




/**
Gets the number of transition failures since the last transition request.

@return	The number of failures if successful, otherwise one of the other system-wide
        or Domain Manager specific error codes.
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

@param	aHierarchyObserver	The implementation of the interface to the Domain Manager.
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
