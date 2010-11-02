// Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// rusbhostmsdevice.cpp
//
//



/**
 @file
 @internalTechnology
*/


#include <e32std.h>
#include <f32file.h>

#include "msmanclientserver.h"
#include "rusbotgsession.h"
#include "debug.h"


TVersion RUsbOtgSession::Version() const
    {
    return(TVersion(KUsbOtgSrvMajorVersionNumber,
                    KUsbOtgSrvMinorVersionNumber,
                    KUsbOtgSrvBuildVersionNumber));
    }


EXPORT_C RUsbOtgSession::RUsbOtgSession()
    {
    }


EXPORT_C RUsbOtgSession::RUsbOtgSession(TInt /* aParam */)
    {
    }

EXPORT_C TInt RUsbOtgSession::Connect()
    {
    TInt retry = 2;
    for (;;)
        {
        TInt r = CreateSession(KUsbOtgServerName, Version());
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


TInt RUsbOtgSession::StartServer()
    {
    const TUidType serverUid(KNullUid, KNullUid, KUsbOtgServerUid3);

    // Create the server process
    RProcess server;
    TInt r = server.Create(KUsbOtgServerName, KNullDesC, serverUid);
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


EXPORT_C TBool RUsbOtgSession::DeviceInserted()
    {
    TPckgBuf<TBool> pckg;
    TIpcArgs args(&pckg);

    SendReceive(EUsbOtgDeviceInserted, args);
    TBool res = pckg();
    return res;
    }


EXPORT_C void RUsbOtgSession::NotifyChange(TBool& /* aChanged */, TRequestStatus& aStatus)
    {
    TPckgBuf<TBool> pckg;
    TIpcArgs args(&pckg);

    SendReceive(EUsbOtgNotifyChange, args, aStatus);
    }


EXPORT_C TInt RUsbOtgSession::NotifyChangeCancel()
    {
    return SendReceive(EUsbOtgNotifyChangeCancel);
    }


EXPORT_C TInt RUsbOtgSession::BusDrop()
    {
    return SendReceive(EUsbOtgBusDrop);
    }
