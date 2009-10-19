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
#include "tmslog.h"
#include "debug.h"


TVersion RUsbOtgSession::Version() const
    {
    __MSFNSLOG
    return(TVersion(KUsbOtgSrvMajorVersionNumber,
                    KUsbOtgSrvMinorVersionNumber,
                    KUsbOtgSrvBuildVersionNumber));
    }


EXPORT_C RUsbOtgSession::RUsbOtgSession()
    {
    __MSFNSLOG
    }


EXPORT_C RUsbOtgSession::RUsbOtgSession(TInt /* aParam */)
    {
    __MSFNSLOG
    }

EXPORT_C TInt RUsbOtgSession::Connect()
    {
    __MSFNSLOG

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
    __MSFNSLOG

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
    __MSFNSLOG
    TPckgBuf<TBool> pckg;
    TIpcArgs args(&pckg);

    SendReceive(EUsbOtgDeviceInserted, args);
    TBool res = pckg();
    return res;
    }


EXPORT_C void RUsbOtgSession::NotifyChange(TBool& /* aChanged */, TRequestStatus& aStatus)
    {
    __MSFNSLOG
    TPckgBuf<TBool> pckg;
    TIpcArgs args(&pckg);

    SendReceive(EUsbOtgNotifyChange, args, aStatus);
    }


EXPORT_C TInt RUsbOtgSession::NotifyChangeCancel()
    {
    __MSFNSLOG
    return SendReceive(EUsbOtgNotifyChangeCancel);
    }


EXPORT_C TInt RUsbOtgSession::BusDrop()
	{
    __MSFNSLOG
	return SendReceive(EUsbOtgBusDrop);
	}
