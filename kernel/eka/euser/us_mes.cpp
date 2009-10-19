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
// e32\euser\us_mes.cpp
// 
//

#include "us_std.h"




EXPORT_C void RServer2::Receive(RMessage2& aMessage)
//
// Receive a message for the server synchronously.
//
	{
	TRequestStatus s;
	Receive(aMessage,s);
	User::WaitForRequest(s);
	__ASSERT_ALWAYS(s==KErrNone,Panic(ETMesReceiveFailed));
	}



GLDEF_C void CompleteRequest(TRequestStatus &aStatus,TInt aCode)
	{
	TRequestStatus *pS=(&aStatus);
	User::RequestComplete(pS,aCode);
	}



EXPORT_C TInt RSessionBase::DoSendReceive(TInt aFunction,const TIpcArgs* aArgs) const
//
// Send a message and wait for the reply synchronously.
//
	{
	if (TUint(aFunction)<=TUint(KMaxTInt))
		return SendSync(aFunction,aArgs);

	Panic(ETMesBadFunctionNumber);
	return 0;
	}



EXPORT_C TInt RSessionBase::DoSend(TInt aFunction,const TIpcArgs* aArgs) const
//
// Send a blind message to the server.
//
	{
	if (TUint(aFunction)<=TUint(KMaxTInt))
		return SendAsync(aFunction,aArgs,NULL);

	Panic(ETMesBadFunctionNumber);
	return 0;
	}



EXPORT_C void RSessionBase::DoSendReceive(TInt aFunction,const TIpcArgs* aArgs,TRequestStatus &aStatus) const
//
// Send a message and wait for the reply asynchronously.
//
	{
	if (TUint(aFunction)<=TUint(KMaxTInt))
		{
		aStatus=KRequestPending;
		TInt r=SendAsync(aFunction,aArgs,&aStatus);
		if (r!=KErrNone)
			::CompleteRequest(aStatus,r);
		}
	else
		Panic(ETMesBadFunctionNumber);
	}



TInt RSubSessionBase::DoCreateSubSession(RSessionBase& aSession,TInt aFunction,const TIpcArgs* aArgs, TBool aAutoClose)
	{
	TIpcArgs a;
	if (aArgs!=NULL)
		{
		a.iArgs[0] = aArgs->iArgs[0];
		a.iArgs[1] = aArgs->iArgs[1];
		a.iArgs[2] = aArgs->iArgs[2];
		a.iFlags = aArgs->iFlags;
		}
	TPckgBuf<TInt> reply;
	a.Set(3,&reply);
	TInt r=aSession.SendReceive(aFunction,a);
	if (r==KErrNone)
		{
		iSubSessionHandle=reply();
		if (aAutoClose)
			{
			iSession=aSession;
			// set the caller's session handle to NULL to discourage the caller from closing it.
			aSession.SetHandle(KNullHandle);
			}
		else
			{
			// Set session handle with 'no close' set, to prevent CloseSubSession() 
			// from closing down the session
			iSession.SetHandleNC(aSession.Handle());
			}
		}
	else
		{
		iSubSessionHandle=0;
		iSession.SetHandle(KNullHandle);
		// Close the caller's session so it isn't left orphaned
		if (aAutoClose)
			aSession.Close();
		}
	return(r);
	}



EXPORT_C TInt RSubSessionBase::DoCreateSubSession(const RSessionBase& aSession,TInt aFunction,const TIpcArgs* aArgs)
	{
	// need to cast away const	
	return DoCreateSubSession( (RSessionBase&) aSession, aFunction, aArgs, EFalse);
	}



/**
Creates a new sub-session within an existing session. The new sub-session takes 
ownership of the session so that when the sub-session is closed, the session is 
closed too. If the creation of the sub-session fails, the session is closed immediately.
In other words, this method will always take ownership of the session, whether it succeeds
or not and the caller should never need to close it.

@param aSession The session to which this sub-session will belong.
@param aFunction The opcode specifying the requested service;
                the server should interpret this as a request to create
                a sub-session.
@param aArgs	The arguments to be sent to the server as part of the 
				sub-session create request. The fourth argument is not 
				sent to the server, instead it is replaced with a descriptor 
				reference to the 32bit value where the server should store 
				the handle of the created sub-session.

@return KErrNone if successful, otherwise one of the other system-wide error
        codes.

*/
EXPORT_C TInt RSubSessionBase::CreateAutoCloseSubSession(RSessionBase& aSession,TInt aFunction,const TIpcArgs& aArgs)
	{
	return DoCreateSubSession(aSession, aFunction, &aArgs, ETrue);
	}



/**
Returns a copy of the session associated with this sub-session.

@return a copy of the session
*/
EXPORT_C const RSessionBase RSubSessionBase::Session() const
	{
	RSessionBase session = iSession;

	// If this is a "normal" subsession, then the ENoClose flag will be set
	// to prevent the closing of the subsession from closing down the main session
	// so in this case we need to turn the ENoClose flag OFF to allow someone
	// to use the returned session to call RSessionBase::Close().
	// If this is a "autoclose" subsession, then the ENoClose flag will be clear
	// to allow the closing of the subsession to close down the main session
	// so in this case we need to turn the ENoClose flag ON to stop someone from
	// using the returned session to call RSessionBase::Close().
	session.iHandle ^= CObjectIx::ENoClose;


	return session;
	}



/**
Closes the sub-session.

@param aFunction The opcode specifying the requested service;
                 the server should interpret this as a request to close
                 the sub-session.
*/
EXPORT_C void RSubSessionBase::CloseSubSession(TInt aFunction)
	{
	if (iSubSessionHandle)
		{
		iSession.SendReceive(aFunction,TIpcArgs(TIpcArgs::ENothing,TIpcArgs::ENothing,TIpcArgs::ENothing,iSubSessionHandle));
		iSubSessionHandle=KNullHandle;

		// Close the session - will only work if CObjectIx::ENoClose is clear 
		// i.e. the sub-session was created using CreateAutoCloseSubSession()
		iSession.Close();
		}
	}



EXPORT_C TInt RSubSessionBase::DoSend(TInt aFunction,const TIpcArgs* aArgs) const
//
// Blind send. 
//
	{
	TIpcArgs a;
	if(aArgs)
		{
		a.iArgs[0] = aArgs->iArgs[0];
		a.iArgs[1] = aArgs->iArgs[1];
		a.iArgs[2] = aArgs->iArgs[2];
		a.iFlags = aArgs->iFlags&((1<<(3*TIpcArgs::KBitsPerType))-1);
		}
	a.iArgs[3] = iSubSessionHandle;
	return(iSession.Send(aFunction,a));
	}



EXPORT_C void RSubSessionBase::DoSendReceive(TInt aFunction,const TIpcArgs* aArgs,TRequestStatus &aStatus) const
//
// Send and wait for reply asynchronously.
//
	{
	TIpcArgs a;
	if(aArgs)
		{
		a.iArgs[0] = aArgs->iArgs[0];
		a.iArgs[1] = aArgs->iArgs[1];
		a.iArgs[2] = aArgs->iArgs[2];
		a.iFlags = aArgs->iFlags&((1<<(3*TIpcArgs::KBitsPerType))-1);
		}
	a.iArgs[3] = iSubSessionHandle;
	iSession.SendReceive(aFunction,a,aStatus);
	}



EXPORT_C TInt RSubSessionBase::DoSendReceive(TInt aFunction,const TIpcArgs* aArgs) const
//
// Send and wait for reply synchronously.
//
	{
	TIpcArgs a;
	if(aArgs)
		{
		a.iArgs[0] = aArgs->iArgs[0];
		a.iArgs[1] = aArgs->iArgs[1];
		a.iArgs[2] = aArgs->iArgs[2];
		a.iFlags = aArgs->iFlags&((1<<(3*TIpcArgs::KBitsPerType))-1);
		}
	a.iArgs[3] = iSubSessionHandle;
	return(iSession.SendReceive(aFunction,a));
	}






