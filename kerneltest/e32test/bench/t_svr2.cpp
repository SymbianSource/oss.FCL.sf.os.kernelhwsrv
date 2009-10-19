// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\bench\t_svr2.cpp
// Overview:
// Tests the Client/Server architecture of the Symbian platform. 
// Tests that messages are completed when a server terminates.
// API Information:
// CSession2, CServer2, RSessionBase.
// Details:
// - Create and start a server thread. Verify results are as expected.
// - Create and close client sessions to enforce granularity expansion.
// - Open and close a session and verify that the heap is OK.
// - Open a connection specifying 0 message slots for the session and verify
// that the server returns KErrNone.
// - Create lot of timers to eliminate kernel granularities.
// - Create a client session and validate kernel heap is ok if client is killed.
// - Create a client session and validate kernel heap is ok if server is killed
// when client has outstanding request to server.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32def.h>
#include <e32def_private.h>

const TInt KHeapSize=0x4000;
const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=0;
const TInt KBuildVersionNumber=1;

_LIT(KServerName,"MyServer");

class CMySession : public CSession2
	{
public:
	enum {ETest, ENeverComplete};
public:
	CMySession();
	virtual void ServiceL(const RMessage2& aMessage);//Overloading pure virtual
	};

class CMyServer : public CServer2
	{
public:
	CMyServer(TInt aPriority);
	static CMyServer *New(TInt aPriority);
	virtual CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const ;
};

class CMyActiveScheduler : public CActiveScheduler
	{
public:
	virtual void Error(TInt anError) const;	 //Overloading pure virtual
	};


class RMySessionBase : public RSessionBase
	{
public:
	TInt Open();
	TInt Test();
	TInt NeverComplete();
	TVersion Version();
	};

_LIT(KSvrSemName, "svrSem");
_LIT(KSvrSem2Name, "svrSem2");
_LIT(KClientSemName, "clientSem");

CMySession::CMySession()
	{}

void CMySession::ServiceL(const RMessage2& aMessage)
//
// Handle messages for this server.
//
	{
	RSemaphore svrSem;

	//CMySession &c=(CMySession &)aClient;// passed as CSession2 since fn overloads pure virtual fn
	TInt r=KErrNone;
	switch (aMessage.Function())
		{
	case ETest:
		break;
	case ENeverComplete:
		r = svrSem.OpenGlobal(KSvrSemName);
		if (r != KErrNone)
			break;
		svrSem.Signal(); // all ready for the server to be killed
		svrSem.Close();
		return;
	default:
		r=KErrNotSupported;
		}
	aMessage.Complete(r);
	}

CMyServer* CMyServer::New(TInt aPriority)
//
// Create a new CMyServer.
//
	{

	return new CMyServer(aPriority);
	}

CMyServer::CMyServer(TInt aPriority)
//
// Constructor.
//
	: CServer2(aPriority)
	{}

CSession2* CMyServer::NewSessionL(const TVersion&, const RMessage2&) const
//
// Create a new client for this server.
//
	{
	return new(ELeave) CMySession();
	}

void CMyActiveScheduler::Error(TInt anError) const
//
// Called if any Run() method leaves.
//
	{
	RTest testSvr(_L("Server"));
	testSvr.Panic(anError,_L("CMyActiveScheduler::Error"));
	}


TInt RMySessionBase::Open()
//
// Open the server.
//
	{
	return CreateSession(KServerName,Version(),0);
	}

TInt RMySessionBase::Test()
//
// Send a message and wait for completion.
//
	{
	return SendReceive(CMySession::ETest, TIpcArgs());
	}

TInt RMySessionBase::NeverComplete()
//
// Send a message and wait for completion.
//
	{
	return SendReceive(CMySession::ENeverComplete, TIpcArgs());
	}

TVersion RMySessionBase::Version()
//
// Return the current version.
//
	{
	TVersion v(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	return v;
	}

LOCAL_C TInt serverThreadEntryPoint(TAny*)
//
// The entry point for the server thread.
//
	{
	CMyActiveScheduler *pR = new CMyActiveScheduler;
	if (pR == NULL)
		return KErrNoMemory;
	CActiveScheduler::Install(pR);

	CMyServer* pS = CMyServer::New(0);
	if (pS == NULL)
		return KErrNoMemory;
	TInt r = pS->Start(KServerName);
	if (r != KErrNone)
		return r;

	RSemaphore svrSem2;
	r = svrSem2.OpenGlobal(KSvrSem2Name);
	if (r != KErrNone)
		return r;
	svrSem2.Signal();
	svrSem2.Close();
	CActiveScheduler::Start();
	return KErrNone;
	}


LOCAL_C TInt clientThreadEntryPoint(TAny *param)
//
// The entry point for the client thread.
//
	{
	RSemaphore svrSem2;
	RSemaphore clientSem;
	TInt r = clientSem.OpenGlobal(KClientSemName);
	if (r != KErrNone)
		return r;
	RMySessionBase t;
	RMySessionBase t2;
//	RTimer rt[40];
//	TUint c;

	switch ((TUint32)param)
		{
	case 1:
		r=t.Open();
		if (r != KErrNone)
			break;
		clientSem.Signal();
		clientSem.Close();
		FOREVER
			;
	case 2:
		// Now create a lot of timers to eliminate kernel granularities
/*		for (c=0;c<40;c++)
			{
			TInt r=rt[c].CreateLocal();
			test(r==KErrNone);
			}
		for (c=0;c<39;c++)
			rt[c].Close();
*/
		r=svrSem2.OpenGlobal(KSvrSem2Name);
		if (r != KErrNone)
			break;
		clientSem.Signal();
		svrSem2.Wait();
		svrSem2.Close();
		User::After(2000000);
		r=t2.Open();
		if (r != KErrNone)
			break;
		r=t2.NeverComplete(); // r should be KErrServerTerminated
		if (r != KErrServerTerminated)
			break;
		t2.Close();
		clientSem.Signal();
		clientSem.Close();
		FOREVER
			;
	default:
		break;
		}
	return r;
	}

GLDEF_C TInt E32Main()
//
// Test server & session cleanup.
//
    {
	RSemaphore svrSem;
	RSemaphore svrSem2;
	RSemaphore clientSem;
	RTest test(_L("T_SVR2"));
	test.Title();

	__KHEAP_MARK;

	test.Start(_L("Creating server semaphore"));
	TInt r=svrSem.CreateGlobal(KSvrSemName, 0);
	test(r==KErrNone);
	test.Next(_L("Creating server semaphore 2"));
	r=svrSem2.CreateGlobal(KSvrSem2Name, 0);
	test(r==KErrNone);
	test.Next(_L("Creating client semaphore"));
	r=clientSem.CreateGlobal(KClientSemName, 0);
	test(r==KErrNone);

	test.Next(_L("Creating server"));
	RThread server;
	r=server.Create(_L("MyServer"),serverThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)1);
	test(r==KErrNone);
	server.Resume();
	svrSem2.Wait(); // server up & running

	test.Next(_L("Forcing granularity expansion"));
	const TInt KNumberOfSessions=10;
	TInt i;
	RMySessionBase ts[KNumberOfSessions];
	for (i=0; i<KNumberOfSessions; i++)
		ts[i].Open();
	for (i=0; i<KNumberOfSessions; i++)
		ts[i].Close(); 

	test.Next(_L("Opening a session and closing it"));
	RMySessionBase t;
	r=t.Open();
	test(r==KErrNone);
	t.Close();
	RThread clientx;
	r=clientx.Create(_L("MyClientx"),clientThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)1);
	test(r==KErrNone);
	clientx.Resume();
	clientSem.Wait(); // client connected

	test.Next(_L("Creating client & killing it"));

	RThread client;
	r=client.Create(_L("MyClient"),clientThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)1);
	test(r==KErrNone);
	client.Resume();
	clientSem.Wait(); // client connected
	User::After(1000000);
	client.Kill(666); // kill the client

	clientx.Kill(666); // kill the client
	CLOSE_AND_WAIT(clientx);
	CLOSE_AND_WAIT(client);

	test.Next(_L("Creating client and killing server"));
	server.Kill(0);	// first kill the existing server 
	CLOSE_AND_WAIT(server);
	RThread client2;	// and create the client so the heap is in a good state for the mark
	r=client2.Create(_L("MyClient"),clientThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)2);
	test(r==KErrNone);
	client2.Resume();
	clientSem.Wait(); // client running but not connected

	RThread server2;
	r=server2.Create(_L("MyServer"),serverThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)2);
	test(r==KErrNone);
	server2.Resume();
	svrSem.Wait(); // client has request outstanding to server
	server2.Kill(666); // kill the server
	clientSem.Wait(); // client's request has completed & client has closed session
	User::After(1000000);
	client2.Kill(666);
	CLOSE_AND_WAIT(client2);
	CLOSE_AND_WAIT(server2);

	svrSem.Close();
	svrSem2.Close();
	clientSem.Close();

	__KHEAP_MARKEND; // and check the kernel's heap is OK

	test.End();
 	return(KErrNone);
    }



