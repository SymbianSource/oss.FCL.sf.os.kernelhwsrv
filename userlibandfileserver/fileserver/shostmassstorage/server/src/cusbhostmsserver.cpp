// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "securitypolicy.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "cusbhostmsserverTraces.h"
#endif


/**
Constructs a USB mass storage Server
*/
CUsbHostMsServer* CUsbHostMsServer::NewLC()
    {
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
    // Intentionally left blank
    }


/**
Constructor
*/
CUsbHostMsServer::CUsbHostMsServer()
:   CPolicyServer(EPriorityHigh,KUsbMsServerPolicy, EGlobalSharableSessions)
    {
    }


/**
Create a new session on this server.

@param aVersion Version of client
@param aMessage& Not used

@return CSession2* A pointer to a session object to be used for the client
*/
CSession2* CUsbHostMsServer::NewSessionL(const TVersion &aVersion, const RMessage2& /*aMessage*/) const
    {
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
    OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, CUSBHOSTMSSERVER_10,
              "CUsbHostMsServer::Error [aError=%d]\n", aError);

    Message().Complete(aError);
    ReStart();
    }

/**
Increment the open session count (iSessionCount) by one.

@post The number of open sessions is incremented by one
*/
void CUsbHostMsServer::IncrementSessionCount()
    {
    OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, CUSBHOSTMSSERVER_11,
              "CUsbHostMsServer::IncrementSessionCount %d", iSessionCount);
    __ASSERT_DEBUG(iSessionCount >= 0, User::Panic(KUsbMsHostPanicCat, EUsbMsPanicIllegalIPC));

    ++iSessionCount;
    }

/**
Decrement the open session count (iSessionCount) by one.

@post The number of open sessions is decremented by one
*/
void CUsbHostMsServer::DecrementSessionCount()
    {
    OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, CUSBHOSTMSSERVER_12,
              "CUsbHostMsServer::DecrementSessionCount %d", iSessionCount);

    __ASSERT_DEBUG(iSessionCount > 0, User::Panic(KUsbMsHostPanicCat, EUsbMsPanicIllegalIPC));

    --iSessionCount;
    }

