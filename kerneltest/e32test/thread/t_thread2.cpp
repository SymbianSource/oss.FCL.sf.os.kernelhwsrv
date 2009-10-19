// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\thread\t_thread2.cpp
// More tests the RThread class (T_THREAD was getting too big)
// Overview:
// Tests the RThread class
// API Information:
// RThread
// Details:
// - Test running a thread that suspends itself.  This activity has 
// deadlocked the Emulator in the past.
// - Test a server panicking a thread in another process that's already exited
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32panic.h>
#include <e32debug.h>

const TInt KHeapSize=0x200;

_LIT(KMyName, "T_THREAD2");

LOCAL_D RTest test(KMyName);

LOCAL_C TInt SuspendThread(TAny*)
	{
	
	RThread().Suspend();
	return(KErrNone);
	}


LOCAL_D void TestSelfSuspend(TOwnerType anOwnerType)
//
// Test running a thread that suspends itself.  This activity has 
// deadlocked the Emulator in the past
//
	{

	RThread suspendThread;
	TInt r;
	TRequestStatus s;
	TInt jit=User::JustInTime();
	test.Start(_L("Test running a thread which suspends itself"));
	test.Next(_L("Create the thread"));
	r=suspendThread.Create(KNullDesC,SuspendThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)NULL,anOwnerType);
	test(r==KErrNone);
	suspendThread.Logon(s);
	suspendThread.Resume();
	test.Next(_L("Wait a second"));
	User::After(1000000);
	User::SetJustInTime(EFalse);
	suspendThread.Panic(_L("FEDCBA9876543210fedcba"),999);
	User::WaitForRequest(s);
	User::SetJustInTime(jit);
	test(suspendThread.ExitType()==EExitPanic);
	test(suspendThread.ExitReason()==999);
	test(suspendThread.ExitCategory()==_L("FEDCBA9876543210"));
	CLOSE_AND_WAIT(suspendThread);
	test.End();
	}


_LIT(KServerName,"MyServer");

RSemaphore TheSemaphore;

class CMySession : public CSession2
	{
public:
	CMySession();
	virtual void ServiceL(const RMessage2& aMessage);
	};

class CMyServer : public CServer2
	{
public:
	CMyServer();
	virtual CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;
	};

class RSession : public RSessionBase
	{
public:
	TInt Open();
	void Test(TRequestStatus& aStatus);
	};

CMySession::CMySession()
	{
	}

CMyServer::CMyServer() :
	CServer2(0)
	{
	}

CSession2* CMyServer::NewSessionL(const TVersion&, const RMessage2&) const
	{
	RDebug::Printf("Server: create session");
	return new (ELeave) CMySession;
	}

void CMySession::ServiceL(const RMessage2& aMessage)
	{
	RDebug::Printf("Server: receive message");
	TheSemaphore.Wait();
	RDebug::Printf("Server: panic client");
	aMessage.Panic(_L("!!!"), 1);
	CActiveScheduler::Stop();
	}

TInt RSession::Open()
	{
	return CreateSession(KServerName, TVersion());
	}

void RSession::Test(TRequestStatus& aStatus)
	{
	RDebug::Printf("Client: send message");
	SendReceive(0, TIpcArgs(), aStatus);
	RDebug::Printf("Client: send done");
	}

TInt ServerThread(TAny*)
	{
	RDebug::Printf("Server: start");
	
	CTrapCleanup* cleanup = CTrapCleanup::New();

	CActiveScheduler* pR = new CActiveScheduler;
	if (pR == NULL)
		return KErrNoMemory;
	CActiveScheduler::Install(pR);
	
	CMyServer* pS = new CMyServer;
	if (pS == NULL)
		return KErrNoMemory;
	TInt r = pS->Start(KServerName);
	if (r != KErrNone)
		return r;
	RThread::Rendezvous(KErrNone);
	
	TRAP(r, CActiveScheduler::Start());
	if (r != KErrNone)
		return r;
	
	delete pS;
	delete pR;
	delete cleanup;
	
	RDebug::Printf("Server: exit");
	return KErrNone;
	}

TInt ClientProcess()
	{
	RDebug::Printf("Client: open session");
	RSession session;
	session.Open();
	TRequestStatus status;
	RDebug::Printf("Client: send request");
	session.Test(status);
	RDebug::Printf("Client: exit");
	return KErrNone;
	}

void TestServerPanic()
	{
	TRequestStatus status;
	
	test_KErrNone(TheSemaphore.CreateLocal(0));
	
	RDebug::Printf("Main: start server");
	RThread serverThread;
	test_KErrNone(serverThread.Create(_L("server"), ServerThread, 4096, NULL, NULL));
	serverThread.Rendezvous(status);
	serverThread.Resume();
	User::WaitForRequest(status);
	test_KErrNone(status.Int());

	RDebug::Printf("Main: start client");
	RProcess clientProcess;
	test_KErrNone(clientProcess.Create(KMyName, _L("client")));
	clientProcess.Resume();
	clientProcess.Logon(status);
	User::WaitForRequest(status);
	test_KErrNone(clientProcess.ExitReason());
	test_Equal(EExitKill, clientProcess.ExitType());

	RDebug::Printf("Main: kick server");
	TheSemaphore.Signal();
	
	RDebug::Printf("Main: wait for server to exit");
	serverThread.Logon(status);
	User::WaitForRequest(status);
	test_KErrNone(serverThread.ExitReason());
	test_Equal(EExitKill, serverThread.ExitType());

	User::After(1);
	RDebug::Printf("Main: exit");
	}


GLDEF_C TInt E32Main()
//
// Main
//
	{	

	if (User::CommandLineLength())
		return ClientProcess();

	test.Title();
	__UHEAP_MARK;
	
	test.Start(_L("Test threads"));

	test.Next(_L("Test a thread suspending itself"));
	TestSelfSuspend(EOwnerProcess);
	TestSelfSuspend(EOwnerThread);

	test.Next(_L("Test a server panicking a thread in another process that's already exited"));
	TestServerPanic();
	
	test.End();
	__UHEAP_MARKEND;
	return(KErrNone);
	}
