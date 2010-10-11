// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implements a Symbian OS server that exposes the RUsbMassStorage API
//
//

/**
 @file
 @internalTechnology
*/

#include <e32svr.h>
#include "usbmsshared.h"
#include "cusbmassstorageserver.h"
#include "cusbmassstoragesession.h"
#include "drivemanager.h"
#include "cusbmassstoragecontroller.h"
#include "smassstorage.h"
#include "usbmsserversecuritypolicy.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "cusbmassstorageserverTraces.h"
#endif

/**
 Constructs a USB mass storage Server

 @return  a pointer to CUsbMassStorageServer object
 */
CUsbMassStorageServer* CUsbMassStorageServer::NewLC(CUsbMassStorageController& aController)
    {
    CUsbMassStorageServer* r = new (ELeave) CUsbMassStorageServer(aController);
    CleanupStack::PushL(r);
    r->StartL(KUsbMsServerName);
    return r;
    }

/**
 Destructor
 */
CUsbMassStorageServer::~CUsbMassStorageServer()
    {
    // Intentionally left blank
    }


/**
 Constructor

 @param aController a USB mass storage controller reference
 */
CUsbMassStorageServer::CUsbMassStorageServer(CUsbMassStorageController& aController)
     : CPolicyServer(EPriorityHigh,KUsbMsServerPolicy)
    , iController(aController)
    {
    OstTraceFunctionEntry0(CUSBMASSSTORAGESERVER_100);
    }

/**
 Create a new session on this server

 @param &aVersion   Version of client
 @return    A pointer to a session object to be used for the client
 */
CSession2* CUsbMassStorageServer::NewSessionL(const TVersion &aVersion, const RMessage2& /*aMessage*/) const
    {
    OstTraceFunctionEntry0(CUSBMASSSTORAGESERVER_110);
    TVersion v(KUsbMsSrvMajorVersionNumber,KUsbMsSrvMinorVersionNumber,KUsbMsSrvBuildVersionNumber);
    if (!User::QueryVersionSupported(v, aVersion))
        User::Leave(KErrNotSupported);

    CUsbMassStorageServer* ncThis = const_cast<CUsbMassStorageServer*>(this);
    CUsbMassStorageSession* sess = CUsbMassStorageSession::NewL(*ncThis);
    return sess;
    }


/**
 Inform the client there has been an error.

 @param aError  The error that has occurred
 */
void CUsbMassStorageServer::Error(TInt aError)
    {
    OstTrace1(TRACE_SMASSSTORAGE_FS, CUSBMASSSTORAGESERVER_120,
              "aError=%d", aError);
    Message().Complete(aError);
    ReStart();
    }

/**
 Increment the open session count (iSessionCount) by one.

 @post  the number of open sessions is incremented by one
 */
void CUsbMassStorageServer::IncrementSessionCount()
    {
    OstTrace1(TRACE_SMASSSTORAGE_FS, CUSBMASSSTORAGESERVER_130,
              "SessionCount=%d", iSessionCount);
    __ASSERT_DEBUG(iSessionCount >= 0, User::Panic(KUsbMsSvrPncCat, EUsbMsPanicIllegalIPC));
    ++iSessionCount;
    }

/**
 Decrement the open session count (iSessionCount) by one.

 @post      the number of open sessions is decremented by one
 */
void CUsbMassStorageServer::DecrementSessionCount()
    {
    OstTrace1(TRACE_SMASSSTORAGE_FS, CUSBMASSSTORAGESERVER_140,
              "SessionCount=%d", iSessionCount);
    __ASSERT_DEBUG(iSessionCount > 0, User::Panic(KUsbMsSvrPncCat, EUsbMsPanicIllegalIPC));
    --iSessionCount;
    }

