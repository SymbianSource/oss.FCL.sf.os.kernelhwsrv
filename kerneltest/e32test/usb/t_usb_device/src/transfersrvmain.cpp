/*
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32base.h>
#include <e32test.h>
#include "transfersrv.h"
#include "transferserver.h"

static void RunServerL();


RTest test(_L("T_USB_TRANSFER"));


GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;

	CTrapCleanup* cleanup = CTrapCleanup::New();

	TInt ret = KErrNoMemory;

	if (cleanup)
		{
		TRAP(ret, RunServerL());
		delete cleanup;
		}

	__UHEAP_MARKEND;

	return ret;
	}

static void RunServerL()
//
// Perform all server initialisation, in particular creation of the
// scheduler and server and then run the scheduler
//
	{
	test.Start(_L("transfer handle"));

	// naming the server thread after the server helps to debug panics
	User::LeaveIfError(User::RenameThread(KTransferServerName));
	//
	// create and install the active scheduler we need
	CActiveScheduler* scheduler = new(ELeave) CActiveScheduler;
	CleanupStack::PushL(scheduler);
	CActiveScheduler::Install(scheduler);
	//
	// create the server (leave it on the cleanup stack)
	CTransferServer* server = CTransferServer::NewLC();
	//
	RProcess::Rendezvous(KErrNone);

	//
	// Ready to run
	RDebug::Printf(">>>CActiveScheduler::Start");
	CActiveScheduler::Start();
	RDebug::Printf("<<<CActiveScheduler::Start");
	test.End();
	test.Close();

	//
	// Cleanup the server and scheduler
	RDebug::Printf("tranfermain exit 1");
	CleanupStack::PopAndDestroy(2, scheduler);
	RDebug::Printf("tranfermain exit 2");
	}

//
// End of file
