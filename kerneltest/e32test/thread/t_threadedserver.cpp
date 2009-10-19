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
// e32test\thread\t_threadedserver.cpp
// Tests the multi-threaded server/session support in CServer2/CSession2
// Overview:
// Tests the CServer2::SetMaster and CSession2:SetServer functions
// API Information:
// CServer2, CSession2
// Details:
// - Test creating multiple sessions with a single-threaded server
// - Test creating multiple sessions with a multi-threaded server
// - Verify that requests to a multi-threaded server can run in parallel and complete out-of-order
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
//



#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32panic.h>
#include <e32debug.h>

_LIT(KMyName, "T_THREADEDSERVER");
_LIT(KServerName,"MyServer");
const TInt KHeapSize = 4096;

LOCAL_D RTest test(KMyName);

class CMySession : public CSession2
	{
public:
	enum TMyRequests { EFinish, EDelay5, EPing };
	CMySession(TInt aId);
	virtual void ServiceL(const RMessage2& aMessage);
private:
	const TInt iD;
	};

CMySession::CMySession(TInt aId) :
	iD(aId)
	{
	}

void CMySession::ServiceL(const RMessage2& aMessage)
	{
	TInt err = KErrNotSupported;

	RDebug::Printf("Session %d: received message %d", iD, aMessage.Function());
	switch (aMessage.Function())
		{
		case EFinish:
			CActiveScheduler::Stop();
			err = KErrNone;
			break;

		case EDelay5:
			User::After(5000000);
			err = KErrNone;
			break;

		case EPing:
			User::After(1000000);
			err = KErrNone;
			break;

		default:
			break;
		}

	RDebug::Printf("Session %d: completing message %d, err %d", iD, aMessage.Function(), err);
	if (err == KErrNone)
		aMessage.Complete(err);
	else
		aMessage.Panic(KServerName, err);
	}


class CMyServer : public CServer2
	{
public:
	CMyServer();
	virtual CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;
private:
	mutable TInt iCount;
	};

static CMyServer* TheMaster = NULL;
static CMyServer* TheSlave = NULL;

CMyServer::CMyServer() :
	CServer2(0),
	iCount(0)
	{
	}

CSession2* CMyServer::NewSessionL(const TVersion&, const RMessage2&) const
	{
	RDebug::Printf("Server: creating session, slave is $%x", TheSlave);
	iCount += 1;
	CSession2* mySession = new (ELeave) CMySession(iCount);
	if (iCount > 1 && TheSlave != NULL)
		mySession->SetServer(TheSlave);
	RDebug::Printf("Server: created session %d", iCount);
	return mySession;
	}

TInt RunServerCode(TAny* aFlag)
	{
	TInt mode = (TInt)aFlag;
	TInt r = KErrNoMemory;
	RDebug::Printf("Server: start, mode %d", mode);

	CTrapCleanup* cleanup = CTrapCleanup::New();
	if (cleanup == NULL)
		return r;

	CActiveScheduler* pR = new CActiveScheduler;
	if (pR == NULL)
		return r;

	CActiveScheduler::Install(pR);
	CMyServer* pS = new CMyServer();
	if (pS == NULL)
		return r;

	switch (mode)
		{
		case 0:
			r = pS->Start(KServerName);
			break;
		case 1:
			TheMaster = pS;
			pS->SetMaster(TheMaster);
			r = pS->Start(KServerName);
			break;
		case 2:
			TheSlave = pS;
			pS->SetMaster(TheMaster);
			r = pS->Start(KNullDesC);
			break;
		}
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


class RSession : public RSessionBase
	{
public:
	TInt Create();
	void Request(TInt aRequest, TRequestStatus& aStatus);
	void Request(TInt aRequest);
	};

TInt RSession::Create()
	{
	return CreateSession(KServerName, TVersion());
	}

void RSession::Request(TInt aRequest, TRequestStatus& aStatus)
	{
	SendReceive(aRequest, TIpcArgs(), aStatus);
	}

void RSession::Request(TInt aRequest)
	{
	SendReceive(aRequest, TIpcArgs());
	}

TInt RunClientCode(TAny* aFlag)
	{
	TInt mode = (TInt)aFlag;
	RDebug::Printf("Client: open sessions");
	RSession session1, session2;
	session1.Create();
	session2.Create();

	TBool ooc = EFalse;
	TRequestStatus status1 (KRequestPending), status2 (KRequestPending);
	RDebug::Printf("Client: send request 1");
	session1.Request(CMySession::EDelay5, status1);
	User::After(1000000);
	RDebug::Printf("Client: send request 2");
	session2.Request(CMySession::EPing, status2);

	User::WaitForRequest(status1, status2);
	if (status1 == KRequestPending)
		{
		// request2 finished first ...
		ooc = ETrue;
		RDebug::Printf("Client: request 2 completed %d", status2.Int());
		User::WaitForRequest(status1);
		}
	else
		{
		RDebug::Printf("Client: request 1 completed %d", status1.Int());
		User::WaitForRequest(status2);
		}
	RDebug::Printf("Client: status1 %d status2 %d OOC %d", status1.Int(), status2.Int(), ooc);
	test_KErrNone(status1.Int());
	test_KErrNone(status2.Int());
	test_Equal(ooc, mode > 0);

	if (mode)
		session2.Request(CMySession::EFinish);
	session1.Request(CMySession::EFinish);
	RDebug::Printf("Client: exit");
	return KErrNone;
	}


void TestSequential()
	{
	TRequestStatus status;

	RDebug::Printf("Main: start server");
	RThread serverThread;
	test_KErrNone(serverThread.Create(_L("server"), RunServerCode, KHeapSize, NULL, (TAny*)0));
	serverThread.Rendezvous(status);
	serverThread.Resume();
	User::WaitForRequest(status);
	test_KErrNone(status.Int());

	RDebug::Printf("Main: start client");
	RThread clientThread;
	test_KErrNone(clientThread.Create(_L("client"), RunClientCode, KHeapSize, NULL, (TAny*)0));
	clientThread.Logon(status);
	clientThread.Resume();
	User::WaitForRequest(status);
	test_KErrNone(clientThread.ExitReason());
	test_Equal(EExitKill, clientThread.ExitType());

	serverThread.Logon(status);
	User::WaitForRequest(status);
	test_KErrNone(serverThread.ExitReason());
	test_Equal(EExitKill, serverThread.ExitType());

	User::After(1000);
	RDebug::Printf("Main: exit");
	}

void TestParallel()
	{
	TRequestStatus status;

	RDebug::Printf("Main: start master server");
	RThread masterThread;
	test_KErrNone(masterThread.Create(_L("master"), RunServerCode, KHeapSize, NULL, (TAny*)1));
	masterThread.Rendezvous(status);
	masterThread.Resume();
	User::WaitForRequest(status);
	test_KErrNone(status.Int());

	RDebug::Printf("Main: start slave server");
	RThread slaveThread;
	test_KErrNone(slaveThread.Create(_L("slave"), RunServerCode, KHeapSize, NULL, (TAny*)2));
	slaveThread.Rendezvous(status);
	slaveThread.Resume();
	User::WaitForRequest(status);
	test_KErrNone(status.Int());

	RDebug::Printf("Main: start client");
	RThread clientThread;
	test_KErrNone(clientThread.Create(_L("client2"), RunClientCode, KHeapSize, NULL, (TAny*)3));
	clientThread.Logon(status);
	clientThread.Resume();
	User::WaitForRequest(status);
	test_KErrNone(clientThread.ExitReason());
	test_Equal(EExitKill, clientThread.ExitType());

	slaveThread.Logon(status);
	User::WaitForRequest(status);
	test_KErrNone(slaveThread.ExitReason());
	test_Equal(EExitKill, slaveThread.ExitType());

	masterThread.Logon(status);
	User::WaitForRequest(status);
	test_KErrNone(masterThread.ExitReason());
	test_Equal(EExitKill, masterThread.ExitType());

	User::After(1000);
	RDebug::Printf("Main: exit");
	}


GLDEF_C TInt E32Main()
//
// Main
//
	{	
	test.Title();
	__UHEAP_MARK;

	test.Start(_L("Test threaded server support"));
	test.Next(_L("Test two sessions to the same server (single-threaded)"));
	TestSequential();
	test.Next(_L("Test two sessions to the same server (multi-threaded)"));
	TestParallel();
	test.End();

	__UHEAP_MARKEND;
	return(KErrNone);
	}
