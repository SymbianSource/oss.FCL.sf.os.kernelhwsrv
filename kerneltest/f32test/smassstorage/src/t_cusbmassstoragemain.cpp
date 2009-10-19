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
#include <e32base_private.h>
#include "usbmsshared.h"
#include "cusbmassstoragescheduler.h"
#include "t_cusbmassstoragecontroller.h"
#include "cusbmassstorageserver.h"

LOCAL_C void RunServerL();

GLDEF_C TInt E32Main()
/**
 * Entry-point for the USB Manager server.
 *
 * @return The result of UsbMan::Run
 */
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
	// naming the server thread after the server helps to debug panics
	User::LeaveIfError(RThread().RenameMe(KUsbMsServerName));
	//
	// create and install the active scheduler we need
	CUsbMassStorageScheduler* scheduler = CUsbMassStorageScheduler::NewL();
	CleanupStack::PushL(scheduler);
	CUsbMassStorageScheduler::Install(scheduler);
	//
	// create the server (leave it on the cleanup stack)
    CUsbMassStorageController* msController = CUsbMassStorageController::NewL();
    CleanupStack::PushL(msController);
    
	CUsbMassStorageServer* server = CUsbMassStorageServer::NewLC(*msController);
	scheduler->SetServer(*server);
	//
	// Initialisation complete, now signal the client
#ifdef __USBMS_NO_PROCESSES__
	RThread::Rendezvous(KErrNone);
#else
	RProcess::Rendezvous(KErrNone);
#endif

	//
	// Ready to run
	CActiveScheduler::Start();
	//
	// Cleanup the server, mscontroller and scheduler
	CleanupStack::PopAndDestroy(3, scheduler);
	}

#ifdef __USBMS_NO_PROCESSES__

// The server binary is an "EPOCEXE" target type
// Thus the server parameter passing and startup code for WINS and EPOC are
// significantly different.
//
// In EKA1 WINS, the EPOCEXE target is a DLL with an entry point called WinsMain,
// taking no parameters and returning TInt. This is not really valid as a thread
// function which takes a TAny* parameter which we need.
//
// So the DLL entry-point WinsMain() is used to return a TInt representing the
// real thread function within the DLL. This is good as long as
// sizeof(TInt)>=sizeof(TThreadFunction).
//

static TInt ThreadFunction(TAny* /*aPtr*/)
//
// WINS thread entry-point function.
//
	{
	return E32Main();
	}

IMPORT_C TInt WinsMain();
EXPORT_C TInt WinsMain()
//
// WINS DLL entry-point. Just return the real thread function 
// cast to TInt
//
	{
	return reinterpret_cast<TInt>(&ThreadFunction);
	}

TInt E32Dll(TDllReason)
	{
	return KErrNone;
	}

#endif

//
// End of file

