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
// e32\euser\cbase\ub_svr.cpp
// 
//

#include "ub_std.h"
//#define __DEBUG_IMAGE__ 1
#if defined(__DEBUG_IMAGE__) && defined (__EPOC32__)
#include "e32svr.h" 
#define __IF_DEBUG(t) {RDebug debug;debug.t;}
#else
#define __IF_DEBUG(t)
#endif



_LIT(KSessionPanicCategory,"CSession");
_LIT(KServerPanicCategory,"CServer");



/**
Default constructor.

This constructor is empty.
*/
EXPORT_C CSession2::CSession2()
	{}



/**
Destructor.

It frees resources prior to destruction of the object.
Specifically, it removes this session object from the server
active object’s list of sessions.
*/
EXPORT_C CSession2::~CSession2()
	{
	if (iLink.iNext)
		iLink.Deque();
	}



/**
Completes construction of this server-side client session object.

The function is called by the server active object, the CServer2 derived
class instance, when a client makes a connection request.

The connection request results in the creation of this session object,
followed by a call to this function.

The default implementation is empty.

@see CServer2::NewSessionL()
*/
EXPORT_C void CSession2::CreateL()
	{
	}

/**
Designates the (master or slave) server that is to service this session.

This may be called from the NewSessionL() method of the CServer2-derived
class or from the CreateL() method of the CSession2-derived class.

It must not be called after CreateL() has finished, as this is the point
at which the session binding become irrevocable.

@see CServer::DoConnect()
*/
EXPORT_C void CSession2::SetServer(const CServer2* aServer)
	{
	ASSERT(iLink.iNext == NULL);
	ASSERT(aServer);
	iServer = aServer;
	}

/**
Handles the situation when a call to CSession2::ServiceL(), which services
a client request, leaves.

Servers are active objects, and the call to CSession2::ServiceL() to handle
a client request is executed as part of the server's active object RunL()
function. If the RunL() leaves, the active object framework calls the active
object's RunError() function. The server framework implements this as a call
to ServiceError()

The default behaviour of this function is to complete the message, using
the leave value, if it has not already been completed.

Servers can re-implement this as appropriate.

@param aMessage The message containing the details of the client request that
                caused the leave.
@param aError   The leave code.

@see CActive::RunL()
@see CActive::RunError()
*/
EXPORT_C void CSession2::ServiceError(const RMessage2& aMessage,TInt aError)
	{
	if(!aMessage.IsNull())
		aMessage.Complete(aError);
	}



/**
Called by a server when it receives a disconnect message for the session.
	
This message is sent by the kernel when all client handles to the session have
been closed.
This method deletes the session object and completes the disconnect message.

A derived session implementation may overide this virtual method if it needs to
perform any asynchronous cleanup actions, these actions must end with a call to the
base class implementation of this method, which will delete the session object
and complete the disconnect message

@param aMessage The disconnect message.

@post 'this' session object has been deleted.
*/
EXPORT_C void CSession2::Disconnect(const RMessage2& aMessage)
	{
	delete this;
	aMessage.Complete(0);
	}



/**
Marks the start of resource count checking.

It sets up a starting value for resource count checking.

The function sets up a starting value for resource count checking by using
the value returned by a call to CSession2::CountResources(), and is the value
that will be used for comparison if CSession2::ResourceCountMarkEnd() is called
at some later time.

The client/server framework does not call this function (nor
does it call CSession2::ResourceCountMarkEnd()), but is available for servers
to use, if appropriate.

@see CSession2::CountResources()
@see CSession2::ResourceCountMarkEnd()
*/
EXPORT_C void CSession2::ResourceCountMarkStart()
	{
	iResourceCountMark=CountResources();
	}



/**
Marks the end of resource count checking.

The function takes the current resource count by
calling CSession2::CountResources(), and compares it with the resource count
value saved when CSession2::ResourceCountMarkStart() was called.
If the resource counts differ, then the client thread is panicked (CSession 2)".

The client/server framework does not call this function (nor does it call 
CSession2::ResourceCountMarkStart()), but the function is available for
servers to use, if appropriate.

@param aMessage Represents the details of the client request that is requesting
                this resource check.

@see CSession2::CountResources()
@see CSession2::ResourceCountMarkStart()
*/
EXPORT_C void CSession2::ResourceCountMarkEnd(const RMessage2& aMessage)
	{
	if (iResourceCountMark!=CountResources())
		aMessage.Panic(KSessionPanicCategory,ESesFoundResCountHeaven);
	}



/**
Gets the number of resources currently in use.

Derived classes must provide a suitable implementation.
The meaning of a resource depends on the design intent of the server.

The default implementation panics the calling thread (CSession 1)
before returning KErrGeneral.

@return The current number of resources in use.

@see CSession2::ResourceCountmarkStart()
@see CSession2::ResourceCountmarkEnd()
*/
EXPORT_C TInt CSession2::CountResources()
	{
	User::Panic(KSessionPanicCategory,ESesCountResourcesNotImplemented);
	return KErrGeneral;
	}



/**
Extension function


*/
EXPORT_C TInt CSession2::Extension_(TUint aExtensionId, TAny*& a0, TAny* a1)
	{
	return CBase::Extension_(aExtensionId, a0, a1);
	}




/**
Constructs the server object, specifying the server type and the active
object priority.

Derived classes must define and implement a constructor through which
the priority can be specified. A typical implementation calls this server
base class constructor through a constructor initialization list.

@param aPriority The priority of this active object.
@param aType     Indicates the type of session that the server creates.
                 If not explicitly stated, then the server creates
                 a session that is not sharable with other threads.
*/
EXPORT_C CServer2::CServer2(TInt aPriority, TServerType aType)
	:	CActive(aPriority),
		iSessionType((TUint8)aType),
		iSessionQ(_FOFF(CSession2,iLink)),
		iSessionIter(iSessionQ)
	{
	ASSERT(iSessionType == aType);
	__ASSERT_COMPILE(EServerRole_Default == 0);
	__ASSERT_COMPILE(EServerOpt_PinClientDescriptorsDefault == 0);
	}



/**
Frees resources prior to destruction.

Specifically, it cancels any outstanding request for messages, and 
deletes all server-side client session objects.
*/
EXPORT_C CServer2::~CServer2()
	{
	Cancel();
	while (!iSessionQ.IsEmpty())
		{
		CSession2 *pS=iSessionQ.First();
		pS->iLink.Deque();
		pS->iLink.iNext=0;
		delete pS;
		}
	iServer.Close();
	}


/**
Sets whether the kernel will pin descriptors passed to this server in the context of the client
thread.

Setting this is one way of ensuring that the server will not take page faults when accessing client
descriptors, which would otherwise happen if the data was paged out.

This method overrides the default pinning policy of the server which is for the server 
to pin its client's descriptors if the process creating the server is not data paged.
I.e. if CServer2::SetPinClientDescriptors() is not invoked on the server and 
RProcess::DefaultDataPaged() of the process creating the server returns EFalse, 
the server will pin its client's descriptors, otherwise the server will not pin its
client's descriptors.

This method must be called prior to starting the server by calling the Start() method.

@param aPin	Set to ETrue for the server to pin its client's descriptors, set to 
			EFalse otherwise.
@panic E32USER-CBase 106 When this method is invoked after the server has been started.
@see CServer2::Start()

@prototype
*/
EXPORT_C void CServer2::SetPinClientDescriptors(TBool aPin)
	{
	if (iServer.Handle() != KNullHandle)
		{// Server already started so too late to make it a pinning one.
		Panic(ECServer2InvalidSetPin);
		}
	iServerOpts &= ~EServerOpt_PinClientDescriptorsMask;
	if (aPin)
		iServerOpts |= EServerOpt_PinClientDescriptorsEnable;
	else
		iServerOpts |= EServerOpt_PinClientDescriptorsDisable;
	}

/**
Assigns a role (master or slave) for this server.

The master server is typically named, and receives all Connect messages
from clients. It can hand off some sessions to be processed by one or
more anonymous slave servers, each running in a separate thread.

Both master and slave servers must call this function before calling
Start(), in order to define their roles.  Once the server is started,
its role cannot be changed.

@panic E32USER-CBase-? When this method is invoked after the server has been started.
@see CServer2::Start()

@prototype
*/
EXPORT_C void CServer2::SetMaster(const CServer2* aServer)
	{
	// Roles can only be assigned before the call to Start()
	ASSERT(Server().Handle() == KNullHandle);

	if (aServer == NULL)
		iServerRole = EServerRole_Standalone;
	else if (aServer == this)
		iServerRole = EServerRole_Master;
	else
		iServerRole = EServerRole_Slave;
	}


/**
Adds the server with the specified name to the active scheduler,
and issues the first request for messages.

If KNullDesC is specified for the name, then an anonymous server will be created.
To create a session to such a server, an overload of RSessionBase::CreateSession()
which takes RServer2 object as a parameter can be used.

@param aName The name of the server.
             KNullDesC, to create anonymous servers.
@return KErrNone, if successful, otherwise one of the other system wide error codes.

@capability ProtServ if aName starts with a '!' character
*/
EXPORT_C TInt CServer2::Start(const TDesC& aName)
	{
	TInt r = iServer.CreateGlobal(aName, iSessionType, iServerRole, iServerOpts);
	if (r == KErrNone)
		{
		CActiveScheduler::Add(this);
		ReStart();
		}
	return r;
	}


	
/**
Adds the server with the specified name to the active scheduler,
and issues the first request for messages, and leaves if the operation fails.

If KNullDesC is specified for the name, then an anonymous server will be created.
To create a session to such a server, the overload of RSessionBase::CreateSession() 
which takes an RServer2 object as a parameter can be used.

@param aName The name of the server.
             KNullDesC, to create anonymous servers.
@capability  ProtServ if aName starts with a '!' character
*/
EXPORT_C void CServer2::StartL(const TDesC& aName)
	{
	User::LeaveIfError(Start(aName));
	}



/**
Implements the cancellation of any outstanding request for messages.
*/
EXPORT_C void CServer2::DoCancel()
	{

	iServer.Cancel();
	}



void CServer2::Connect(const RMessage2& aMessage)
//
// Handle a connect request. Ptr0()==Version.
// NOTE: We don't want this to leave as that may kill the server
//
	{

	if (aMessage.Session())
		{
		aMessage.Panic(KServerPanicCategory,ESessionAlreadyConnected);
		return;
		}
	DoConnect(aMessage);
	}



//
//This is all of the Leaving code for connection creation.
//This is in a seperate function in an effort to force compilers to store aSession
//on the stack which enables cleanup to perform correctly when a Leave occurs
//
void CServer2::DoConnectL(const RMessage2& aMessage,CSession2* volatile& aSession)
	{
	TVersion v;
	*(TInt*)&v = aMessage.Int0();
	aSession = NewSessionL(v, aMessage);
	if (!aSession->iServer)
		aSession->iServer = this;
	aSession->CreateL();
	iSessionQ.AddLast(*aSession);
	Exec::SetSessionPtr(aMessage.Handle(), aSession);
	}



/**
Handles the connect request from the client.  We trap Leaves, to ensure
that existing sessions aren't affected by failure to create a new one.

@param aMessage The Connect message sent by the client requesting the
                connection. aMessage.Ptr0() is the required Version.
*/
EXPORT_C void CServer2::DoConnect(const RMessage2& aMessage)
	{
	ASSERT(aMessage.Function() == RMessage2::EConnect);
	ASSERT(aMessage.Session() == NULL);
	ASSERT(!aMessage.IsNull());

	CSession2* newSession = NULL;
	TRAPD(err, DoConnectL(aMessage, newSession));
	if (err != KErrNone)
		{
		// Connect failed
		delete newSession;
		aMessage.Complete(err);
		}
	else
		{
		ASSERT(newSession != NULL);
		CServer2* sessionServer = const_cast<CServer2*>(newSession->Server());
		ASSERT(sessionServer != NULL);

		// The return value of Server() will be 'this', unless it was
		// changed by a call to SetServer().
		if (sessionServer == this)
			{
			// no SetServer() call, so just complete the Connect message
			aMessage.Complete(err);
			}
		else
			{
			// Transfer the new Csession to the specified slave Cserver
			newSession->iLink.Deque();
			sessionServer->iSessionQ.AddLast(*newSession);

			// Ask the kernel to transfer the DSession to the slave DServer.
			// Note: this Exec call also completes the Connect message.
			TInt msgHandle = aMessage.iHandle;
			const_cast<TInt&>(aMessage.iHandle) = 0;
			ASSERT(msgHandle);
			Exec::TransferSession(msgHandle, sessionServer->Server().Handle());
			}
		}

	ASSERT(aMessage.IsNull());
	}



/**
Handles the situation where a call to CServer2::RunL(), leaves.

This is the server active object's implementation of the active object
framework's RunError() function.

In practice, the leave can only be caused by a session's ServiceL() function,
which is called from this RunL(); this error is reflected back to that session
by calling its ServiceError() function.

@param aError The leave code.

@return KErrNone.

@see CActive::RunL()
@see CActive::RunError()
@see CSession2::ServiceError()
*/
EXPORT_C TInt CServer2::RunError(TInt aError)
	{
	Message().Session()->ServiceError(Message(),aError);
	if (!IsActive())
		ReStart();
	return KErrNone;
	}



/**
Restarts the server.

The function issues a request for messages.
*/
EXPORT_C void CServer2::ReStart()
	{

	iServer.Receive(iMessage,iStatus);
	SetActive();
	}



#ifndef __CSERVER_MACHINE_CODED__
/**
Handles the receipt of a message.
*/
EXPORT_C void CServer2::RunL()
	{
	TInt fn = Message().Function();

	if(fn>=0)
		{
		// Service the message
		CSession2* session=Message().Session();
		if(session)
			session->ServiceL(Message());
		else
			NotConnected(Message());
		}
	else if(fn==RMessage2::EConnect)
		{
		Connect(Message());
		}
	else if(fn==RMessage2::EDisConnect)
		{
		Disconnect(Message());
		}
	else
		{
		BadMessage(Message());
		}
	// Queue reception of next message if it hasn't already been done
	if(!IsActive())
		ReStart();
	}

#endif



void CServer2::Disconnect(const RMessage2& aMessage)
//
// Process a disconnect message
//
	{
	CSession2* session=Message().Session();
	if(!session)
		{
		// Session not created yet, so just complete message.
		aMessage.Complete(0);
		return;
		}
	session->Disconnect(aMessage);
	}



void CServer2::BadMessage(const RMessage2& aMessage)
	{
	aMessage.Panic(KServerPanicCategory,EBadMessageNumber);
	}



void CServer2::NotConnected(const RMessage2& aMessage)
	{
	aMessage.Panic(KServerPanicCategory,ESessionNotConnected);
	}

	

/**
Extension function


*/
EXPORT_C TInt CServer2::Extension_(TUint aExtensionId, TAny*& a0, TAny* a1)
	{
	return CActive::Extension_(aExtensionId, a0, a1);
	}
