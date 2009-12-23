// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// cusbhostmsderver.cpp
// 
//

/**
 @file
 @internalTechnology
*/

#include <e32svr.h>
#include "msctypes.h"
#include "shared.h"
#include "msgservice.h"
#include "usbmshostpanic.h"
#include "cusbhostmssession.h"
#include "cusbhostmsserver.h"
#include "msdebug.h"
#include "debug.h"
#include "securitypolicy.h"

/**
Constructs a USB mass storage Server
*/
CUsbHostMsServer* CUsbHostMsServer::NewLC()
	{
    __MSFNSLOG
	CUsbHostMsServer* r = new (ELeave) CUsbHostMsServer();
	CleanupStack::PushL(r);
	r->StartL(KUsbHostMsServerName);
	return r;
	}

/**
Destructor
*/
CUsbHostMsServer::~CUsbHostMsServer()
	{
    __MSFNLOG
    // Intentionally left blank
	}


/**
Constructor
*/
CUsbHostMsServer::CUsbHostMsServer()
:   CPolicyServer(EPriorityHigh,KUsbMsServerPolicy, EGlobalSharableSessions)
	{
    __MSFNLOG
	}


/**
Create a new session on this server.

@param aVersion Version of client
@param aMessage& Not used

@return CSession2* A pointer to a session object to be used for the client
*/
CSession2* CUsbHostMsServer::NewSessionL(const TVersion &aVersion, const RMessage2& /*aMessage*/) const
	{
    __MSFNSLOG
	TVersion v(KUsbHostMsSrvMajorVersionNumber,
               KUsbHostMsSrvMinorVersionNumber,
               KUsbHostMsSrvBuildVersionNumber);
	if (!User::QueryVersionSupported(v, aVersion))
		User::Leave(KErrNotSupported);

	CUsbHostMsServer* ncThis = const_cast<CUsbHostMsServer*>(this);

	CUsbHostMsSession* sess = CUsbHostMsSession::NewL(*ncThis);

	return sess;
	}


/**
Inform the client there has been an error.

@param aError The error that has occurred
*/
void CUsbHostMsServer::Error(TInt aError)
	{
    __MSFNLOG
	__HOSTPRINT1(_L("CUsbHostMsServer::Error [aError=%d]\n"), aError);

	Message().Complete(aError);
	ReStart();
	}

/**
Increment the open session count (iSessionCount) by one.

@post The number of open sessions is incremented by one
*/
void CUsbHostMsServer::IncrementSessionCount()
	{
    __MSFNLOG
	__HOSTPRINT1(_L("CUsbHostMsServer::IncrementSessionCount %d\n"), iSessionCount);
	__ASSERT_DEBUG(iSessionCount >= 0, User::Panic(KUsbMsHostPanicCat, EUsbMsPanicIllegalIPC));

	++iSessionCount;

	__HOSTPRINT(_L("CUsbHostMsServer::IncrementSessionCount\n"));
	}

/**
Decrement the open session count (iSessionCount) by one.

@post The number of open sessions is decremented by one
*/
void CUsbHostMsServer::DecrementSessionCount()
	{
    __MSFNLOG
	__HOSTPRINT1(_L("CUsbHostMsServer::DecrementSessionCount %d\n"), iSessionCount);

	__ASSERT_DEBUG(iSessionCount > 0, User::Panic(KUsbMsHostPanicCat, EUsbMsPanicIllegalIPC));

	--iSessionCount;
	}

