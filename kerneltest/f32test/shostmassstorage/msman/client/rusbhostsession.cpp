// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
//



/**
 @file
 @internalTechnology
*/


#include <e32std.h>
#include <f32file.h>

#include "msmanclientserver.h"
#include "rusbhostsession.h"
#include "tmslog.h"
#include "debug.h"


TVersion RUsbHostSession::Version() const
    {
    __MSFNSLOG
    return(TVersion(KUsbHostSrvMajorVersionNumber,
                    KUsbHostSrvMinorVersionNumber,
                    KUsbHostSrvBuildVersionNumber));
    }


EXPORT_C RUsbHostSession::RUsbHostSession()
    {
    __MSFNSLOG
    }


EXPORT_C TInt RUsbHostSession::Connect()
    {
    __MSFNSLOG

    TInt retry = 2;
    for (;;)
        {
        TInt r = CreateSession(KUsbHostServerName, Version());
        if ((r != KErrNotFound) && (r != KErrServerTerminated))
            {
            return r;
            }
        if (--retry == 0)
            {
            return r;
            }

        r = StartServer();
        if ((r != KErrNone) && (r != KErrAlreadyExists))
            {
            return r;
            }
        }
    }


TInt RUsbHostSession::StartServer()
    {
    __MSFNSLOG

    const TUidType serverUid(KNullUid, KNullUid, KUsbHostServerUid3);

    // Create the server process
    RProcess server;
    TInt r = server.Create(KUsbHostServerName, KNullDesC, serverUid);
    if (r != KErrNone)
        {
        return r;
        }

    // Create the rendezvous request with the server process
    TRequestStatus status;
    server.Rendezvous(status);
    if (status != KRequestPending)
        {
        User::WaitForRequest(status);
        server.Kill(0);    // If the outstanding request is not pending then kill the server
        server.Close();
        return status.Int();
        }

	server.SetPriority(EPriorityHigh);
    server.Resume(); // start the server

    // Test whether the process has ended and if it has ended, return how it ended.
    User::WaitForRequest(status);

    if (status == KRequestPending)
        {
        server.Close();
        return status.Int();
        }

    server.Close();
    return KErrNone;
    }



EXPORT_C TInt RUsbHostSession::Start()
	{
	return SendReceive(EUsbHostStart);
	}
