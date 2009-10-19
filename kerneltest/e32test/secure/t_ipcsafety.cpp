// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\secure\t_ipcsafety.cpp
// Overview:
// Test if it's possible for a thread in a server process to access the IPC alias
// region outside the control of the kernel.
// API Information:
// RMessage2
// Details:
// - Create a server which will take a long time IPCing any client request.
// - Create a high priority thread which will attempt to write to a given
// location in the IPC region, with an exception handler to retry if it fails.
// - Create a client process which connects to the server and offers a
// stack-based descriptor for IPC, as well as the address of another stack
// variable that should not be able to be accessed.
// - The bad writer will attempt to jump in and overwrite the variable,
// causing the client to return a detectable error.
// - Verify that this does not happen.
// Platforms/Drives/Compatibility:
// ARM with multiple memory model only.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32debug.h>
#include <e32base.h>
#include <e32base_private.h>
#include "mmudetect.h"

LOCAL_D RTest test(_L("T_IPCSAFETY"));

void GoodExitWithError();

TInt* DataToSplat;
RSemaphore BadSemaphore;

// Server stuff

_LIT(KBadServerName,"BadServer");

class CBadSession : public CSession2
	{
	virtual void ServiceL(const RMessage2& aMessage);
	};

class CBadServer : public CServer2
	{
public:
	CBadServer(CActive::TPriority aPriority) : CServer2(aPriority)
		{}
	virtual CBadSession* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const
		{
		return new (ELeave) CBadSession();
		}
	};

void CBadSession::ServiceL(const RMessage2& aMessage)
	{
	TBuf16<1024> buf;
	DataToSplat = (TInt*)aMessage.Ptr1();
	BadSemaphore.Signal();
	// Read the buffer lots of times to widen the time window
	for (TInt i=0; i<1024; i++)
		aMessage.Read(0, buf, 0);
	CActiveScheduler::Stop();
	aMessage.Complete(KErrNone);
	}

TInt BadServerThread(TAny*)
	{
	CTrapCleanup* cleanup=CTrapCleanup::New();
	if (!cleanup)
		return KErrNoMemory;
	CActiveScheduler* scheduler = new CActiveScheduler();
	if (!scheduler)
		return KErrNoMemory;
	CActiveScheduler::Install(scheduler);
	CBadServer* server = new CBadServer(CActive::EPriorityStandard);
	if (!server)
		return KErrNoMemory;
	TInt r = server->Start(KBadServerName);
	if (r != KErrNone)
		return r;
	RThread::Rendezvous(KErrNone);
	CActiveScheduler::Start();
	delete server;
	delete scheduler;
	delete cleanup;
	return KErrNone;
	}

class RBadSession : public RSessionBase
	{
public:
	TInt Connect()
		{
		return CreateSession(KBadServerName, TVersion(0,0,0));
		}
	void AccessMe(TDesC* aBuf, TInt* aValue);
	};

void RBadSession::AccessMe(TDesC* aBuf, TInt* aValue)
	{
	SendReceive(0, TIpcArgs(aBuf, aValue));
	};

// Bad writer thread

TInt * const KAliasRegion = (TInt*)0x00200000;
const TUint KAliasMask = 0x000fffff;

void BadExceptionHandler(TExcType, TInt, TInt, TInt, TUint aStackArgument)
	{
	// just retry the instruction after a delay
	User::AfterHighRes(0);
	return;
	}

TInt BadWriterThread(TAny*)
	{
	// set the exception handler so that we don't die when touching the ipc region
	// as it won't be mapped until an unpredictable time
	User::SetExceptionHandler((TExceptionHandler)BadExceptionHandler, KExceptionFault);
	// wait for the server to tell us where to overwrite
	BadSemaphore.Wait();

	TInt* target = (TInt*)(((TUint)DataToSplat&KAliasMask)|(TUint)KAliasRegion);
	*target = KErrGeneral;

	return KErrNone;
	}

// The server process

TInt BadServerProcess()
	{
	test.Title();
	test.Start(_L("Test bad server overwriting good client memory"));

	BadSemaphore.CreateLocal(0);

	test.Next(_L("Setup bad server"));
	RThread serverThread;
	TRequestStatus serverStatus, serverRendezvous;
	test_KErrNone(serverThread.Create(_L("BadServer"), BadServerThread, KDefaultStackSize, NULL, NULL));
	serverThread.Logon(serverStatus);
	serverThread.Rendezvous(serverRendezvous);
	serverThread.Resume();
	User::WaitForRequest(serverRendezvous);

	test.Next(_L("Start bad writer thread"));
	RThread writerThread;
	TRequestStatus writerStatus;
	test_KErrNone(writerThread.Create(_L("BadWriter"), BadWriterThread, KDefaultStackSize, NULL, NULL));
	writerThread.Logon(writerStatus);
	writerThread.SetPriority(EPriorityMore);
	writerThread.Resume();

	test.Next(_L("Run the good client"));
	RProcess goodProcess;
	TRequestStatus goodStatus;
	test_KErrNone(goodProcess.Create(_L("T_IPCSAFETY"), _L("client")));
	goodProcess.Logon(goodStatus);
	goodProcess.Resume();

	test.Next(_L("Wait for server to die"));
	User::WaitForRequest(serverStatus);
	test_Equal(EExitKill, serverThread.ExitType());
	test_KErrNone(serverThread.ExitReason());

	test.Next(_L("Check if client had memory overwritten"));
	User::WaitForRequest(goodStatus);
	test_Equal(EExitKill, goodProcess.ExitType());
	test_KErrNone(goodProcess.ExitReason());

	test.Next(_L("Kill off writer thread"));
	writerThread.Kill(KErrNone);
	User::WaitForRequest(writerStatus);
	test_Equal(EExitKill, writerThread.ExitType());
	test_KErrNone(writerThread.ExitReason());

	test.End();
	return KErrNone;
	}

// The client process

TInt GoodClientProcess()
	{
	RBadSession bad;
	TBuf16<1024> buf;
	TInt r = KErrNone;
	buf.SetLength(1024);
	// just keep trying to connect if the server isn't talkative yet
	while (bad.Connect() != KErrNone)
		User::After(1);
	bad.AccessMe(&buf, &r);
	// Returns r, which logically should be KErrNone as servers aren't
	// supposed to be able to modify
	return r;
	}

// Main

GLDEF_C TInt E32Main()
    {
	TBuf16<512> cmd;
	User::CommandLine(cmd);

	// this test hardcodes various multiple memory model parameters
	// and the moving model's aliasing technique is not susceptible to
	// the problem in the first place
	TUint32 memmodel = MemModelAttributes();
	if ((memmodel & EMemModelTypeMask) != EMemModelTypeMultiple)
		return KErrNone;

	if(cmd.Length())
		return GoodClientProcess();
	else
		return BadServerProcess();
    }

