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
// f32test\msfs\src\t_ms_clisvr.cpp
// 
//

#define __T_MS_CLISVR__

#include <f32file.h>
#include <e32test.h>
#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include <hal.h>
#include <massstorage.h>
#include "rusbmassstorage.h"
#include "t_ms_main.h"

#ifdef __USBMS_NO_PROCESSES__
#include <e32math.h>
#endif

GLDEF_D RUsbMassStorage gUsbMs;

// Unit test for MS client API and server API
GLREF_C void t_ms_clisvr();

// Unit test for MS file server
GLREF_C void t_ms_fsunit();


LOCAL_C TInt StartServer()
//
// Start the server process or thread
//
	{
	const TUidType serverUid(KNullUid, KNullUid, KUsbMsSvrUid);

#ifdef __USBMS_NO_PROCESSES__
	//
	// In EKA1 WINS the server is a DLL, the exported entrypoint returns a TInt
	// which represents the real entry-point for the server thread
	//
	RLibrary lib;
	TInt err = lib.Load(KUsbMsImg, serverUid);

	if (err != KErrNone )
		{
		return err;
		}
	TLibraryFunction ordinal1 = lib.Lookup(1);
	TThreadFunction serverFunc = reinterpret_cast<TThreadFunction>(ordinal1());

	//
	// To deal with the unique thread (+semaphore!) naming in EPOC, and that we may
	// be trying to restart a server that has just exited we attempt to create a
	// unique thread name for the server.
	// This uses Math::Random() to generate a 32-bit random number for the name
	//
	TName name(KUsbMsServerName);
	name.AppendNum(Math::Random(),EHex);
	
	RThread server;
	err = server.Create (
		name,
		serverFunc,
		KUsbMsStackSize,
		NULL,
		&lib,
		NULL,
		KUsbMsMinHeapSize,
		KUsbMsMaxHeapSize,
		EOwnerProcess
	);

	lib.Close();	// if successful, server thread has handle to library now
#else
	//
	// EPOC and EKA2 is easy, we just create a new server process. Simultaneous
	// launching of two such processes should be detected when the second one
	// attempts to create the server object, failing with KErrAlreadyExists.
	//
	RProcess server;
	TInt err = server.Create(KUsbMsImg1, KNullDesC, serverUid);
	if (err == KErrNotFound)
		{
		err = server.Create(KUsbMsImg, KNullDesC, serverUid);
		}
#endif 
	
	if (err != KErrNone)
		{
		return err;
		}

	TRequestStatus stat;
	server.Rendezvous(stat);
	
	if (stat!=KRequestPending)
		server.Kill(0);		// abort startup
	else
		server.Resume();	// logon OK - start the server

	User::WaitForRequest(stat);		// wait for start or death

	// we can't use the 'exit reason' if the server panicked as this
	// is the panic 'reason' and may be '0' which cannot be distinguished
	// from KErrNone
	err = (server.ExitType() == EExitPanic) ? KErrServerTerminated : stat.Int();

	server.Close();

	return err;
	}

LOCAL_C void DoTestConnect()
    // 
    // Connect to the file server
    // 
    {
    test.Printf(_L("DoTestConnect\n"));
    // Load MS file server
    TInt r = StartServer();
    test(KErrNone == r);
    // Connect to MS file server
    r = gUsbMs.Connect();
    test(KErrNone == r);
    
    test.Printf(_L("DoTestConnect ====> PASS\n"));
    }
    
LOCAL_C void DoTestStart()
    //
    // Start mass storage device
    //
    {

    TMassStorageConfig msConfig;
    msConfig.iVendorId.Copy(t_vendorId);
    msConfig.iProductId.Copy(t_productId);
    msConfig.iProductRev.Copy(t_productRev);

    TInt r = gUsbMs.Start(msConfig);
    test(KErrNone == r);
    
    test.Printf(_L("DoTestStart ====> PASS\n"));
    }

LOCAL_C void DoTestStop()
    //
    // Stop USB device
    //
    {
    test.Printf(_L("TestStop\n"));
    TInt r = gUsbMs.Stop();
    test(KErrNone == r); 
    
    test.Printf(_L("DoTestStop ====> PASS\n"));;
    }

GLDEF_C void t_ms_clisvr()
//
// Do all tests
//
	{
	test.Next( _L("TestConnect") );
    DoTestConnect();
   	test.Next(_L("TestStart"));
    DoTestStart();
    test.Next(_L("TestStop"));
    DoTestStop();
 	gUsbMs.Shutdown();
	gUsbMs.Close();
    
    }

GLDEF_C void CallTestsL()
	{
	test.Start(_L("ClientServer Tests"));
	t_ms_clisvr();
	test.End();
	test.Start(_L("File System Unit Tests"));
	t_ms_fsunit();
	test.End();
	}
