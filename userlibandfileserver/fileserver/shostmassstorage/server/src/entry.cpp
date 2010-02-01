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
//

/**
 @file
 @internalTechnology
*/

#include <e32base.h>

#include "cusbhostmsserver.h"

static void RunServerL()
//
// Perform all server initialisation, in particular creation of the
// scheduler and server and then run the scheduler
//
    {
	CUsbHostMsServer* iServer;

    // naming the server thread after the server helps to debug panics
    //
    // create and install the active scheduler we need
    CActiveScheduler* s=new(ELeave) CActiveScheduler;
    CleanupStack::PushL(s);
    CActiveScheduler::Install(s);

	iServer = CUsbHostMsServer::NewLC();
	CleanupStack::PushL(iServer);

    //
    // Initialisation complete, now signal the client
    RProcess::Rendezvous(KErrNone);
    //
    // Ready to run
    CActiveScheduler::Start();
    //
    // Cleanup the server and scheduler
    CleanupStack::PopAndDestroy(2);
    }

TInt E32Main()
//
// Server process entry-point
//
    {
    __UHEAP_MARK;
    //
    CTrapCleanup* cleanup=CTrapCleanup::New();
    TInt r=KErrNoMemory;
    if (cleanup)
        {
        TRAP(r,RunServerL());
        delete cleanup;
        }
    //
    __UHEAP_MARKEND;
    return r;
    }
