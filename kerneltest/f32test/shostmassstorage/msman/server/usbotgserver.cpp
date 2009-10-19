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
//


#include <e32base.h>
#include <e32debug.h>

#include "cusbotgserver.h"


static void RunServerL()
    {
    CActiveScheduler* scheduler = new(ELeave) CActiveScheduler;
    CleanupStack::PushL(scheduler);
    CActiveScheduler::Install(scheduler);

	CUsbOtgServer::NewLC();
    RProcess::Rendezvous(KErrNone);

    CActiveScheduler::Start();
    CleanupStack::PopAndDestroy(2);
    RDebug::Printf("USB OTG Server shutdown");
    }

TInt E32Main()
//
// Server process entry-point
//
    {
    __UHEAP_MARK;
    CTrapCleanup* cleanup = CTrapCleanup::New();
    TInt r = KErrNoMemory;
    if (cleanup)
        {
        TRAP(r, RunServerL());
        delete cleanup;
        }
    __UHEAP_MARKEND;
    return r;
    }

