/*
* Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/

/**
@file
@internalTechnology
*/

#include <e32base.h>
#include <e32test.h>
#include <usb.h>
#include "transfersrv.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "transfersrvTraces.h"
#endif

_LIT(KUsbmanImg, "z:\\system\\programs\\t_usb_transfersrv.exe");


static TInt StartServer()
//
// Start the server process or thread
//
	{
	const TUidType serverUid(KNullUid, KNullUid, KTransferSvrUid);
	OstTrace0(TRACE_NORMAL, STARTSERVER_STARTSERVER, "StartServer1");

	RProcess server;
	TInt err = server.Create(KUsbmanImg, KNullDesC, serverUid);
	OstTrace1(TRACE_NORMAL, STARTSERVER_STARTSERVER_DUP01, "StartServer2 %d", err);
	
	if (err != KErrNone)
		{
		return err;
		}
	OstTrace0(TRACE_NORMAL, STARTSERVER_STARTSERVER_DUP02, "StartServer3");

	TRequestStatus stat;
	server.Rendezvous(stat);
	
	if (stat!=KRequestPending)
		server.Kill(0);		// abort startup
	else
		server.Resume();	// logon OK - start the server
	OstTrace0(TRACE_NORMAL, STARTSERVER_STARTSERVER_DUP03, "StartServer4");

	User::WaitForRequest(stat);		// wait for start or death

	// we can't use the 'exit reason' if the server panicked as this
	// is the panic 'reason' and may be '0' which cannot be distinguished
	// from KErrNone
	err = (server.ExitType() == EExitPanic) ? KErrServerTerminated : stat.Int();
	OstTrace0(TRACE_NORMAL, STARTSERVER_STARTSERVER_DUP04, "StartServer5");

	//server.Close();
	
	OstTrace0(TRACE_NORMAL, STARTSERVER_STARTSERVER_DUP05, "transfer server started successfully: \n");

	return err;
	}



/** Constructor */
EXPORT_C RTransferSrv::RTransferSrv()
    {
    }

/** Destructor */
EXPORT_C RTransferSrv::~RTransferSrv()
    {
    }

EXPORT_C TVersion RTransferSrv::Version() const
    {
    return TVersion(    KTransferSrvMajorVersionNumber,
                        KTransferSrvMinorVersionNumber,
                        KTransferSrvBuildVersionNumber
                    );
    }

EXPORT_C TInt RTransferSrv::Connect()
    {
	TInt retry = 2;
	
	OstTrace0(TRACE_NORMAL, RTRANSFERSRV_CONNECT, "Connect1");
	FOREVER
		{
		TInt err = CreateSession(KTransferServerName, Version(), 10);
		OstTrace0(TRACE_NORMAL, RTRANSFERSRV_CONNECT_DUP01, "Connect2");

		if ((err != KErrNotFound) && (err != KErrServerTerminated))
			{
			return err;
			}
		OstTrace0(TRACE_NORMAL, RTRANSFERSRV_CONNECT_DUP02, "Connect3");

		if (--retry == 0)
			{
			return err;
			}
		OstTrace0(TRACE_NORMAL, RTRANSFERSRV_CONNECT_DUP03, "Connect4");

		err = StartServer();

		if ((err != KErrNone) && (err != KErrAlreadyExists))
			{
			return err;
			}
		
		OstTrace0(TRACE_NORMAL, RTRANSFERSRV_CONNECT_DUP04, "Connect5");
		}
    }

EXPORT_C TInt RTransferSrv::SetConfigFileName(TDes& aString)
	{
	
	OstTrace0(TRACE_NORMAL, RTRANSFERSRV_SETCONFIGFILENAME, "SetConfigFileName");
	return SendReceive(ESetConfigFileName, TIpcArgs(&aString));
	}

