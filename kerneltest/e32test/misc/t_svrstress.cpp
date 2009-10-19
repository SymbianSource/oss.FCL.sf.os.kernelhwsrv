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
// e32test\misc\t_svrstress.cpp
// This is a stress test for client server session connect and disconnect
//

#include <e32base.h>
#include <e32base_private.h>
#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>
#include "u32std.h"
#include <e32atomics.h>
#include <e32panic.h>
#include <e32def.h>
#include <e32def_private.h>

RTest test(_L("T_SVRSTRESS"));

RSemaphore SyncSemaphore;
TUint32 WaitABit;

TInt NumMessageSlots;
TInt UseGlobalMessagePool;

const TInt BigDesLength = 256 * 1024;

#if 1
#define TRACE(t) RDebug::RawPrint(_L8(t))
#else
#define TRACE(t)
#endif

//
// utility functions...
//

void WaitForRequest()
	{
	User::WaitForAnyRequest();
	RThread().RequestSignal(); // put request semaphore count back
	}


TInt WaitForRequest(TRequestStatus& aStatus,TTimeIntervalMicroSeconds32 aTimeout=2*1000000)
	{
	RTimer timer;
	test_Equal(KErrNone,timer.CreateLocal());

	TRequestStatus timeoutStatus;
	timer.After(timeoutStatus,aTimeout);

	User::WaitForRequest(aStatus,timeoutStatus);

	TInt r;
	if(aStatus.Int()==KRequestPending)
		{
		r = KErrTimedOut;
		}
	else
		{
		r = KErrNone;
		timer.Cancel();
		User::WaitForRequest(timeoutStatus);
		}

	CLOSE_AND_WAIT(timer);

	return r;
	}


//
// CMyServer
//

_LIT(KMyServerName,"StressSvr");

class CMyServer : public CServer2
	{
public:
	CMyServer(TInt aPriority);
	static CMyServer* New(TInt aPriority);
	virtual CSession2* NewSessionL(const TVersion&, const RMessage2&) const;
	};


class CMySession : public CSession2
	{
public:
	virtual void ServiceL(const RMessage2& aMessage);
	};


CMyServer* CMyServer::New(TInt aPriority)
	{
	return new CMyServer(aPriority);
	}


CMyServer::CMyServer(TInt aPriority)
	: CServer2(aPriority, ESharableSessions)
	{}


CSession2* CMyServer::NewSessionL(const TVersion&, const RMessage2&) const
	{
	TRACE("O");
	return new(ELeave) CMySession;
	}


TBool RestartServer;

TInt MyServerThread(TAny*)
	{
	CActiveScheduler* pR=new CActiveScheduler;
	if(!pR)
		return KErrNoMemory;
	CActiveScheduler::Install(pR);
	RestartServer = ETrue;

	while(RestartServer)
		{
		__UHEAP_MARK;
		CMyServer* pS=CMyServer::New(0);
		if(!pS)
			return KErrNoMemory;
		TInt r = pS->Start(KMyServerName);
		if(r!=KErrNone)
			return r;

		TRACE("S");
		RThread::Rendezvous(KErrNone);

		CActiveScheduler::Start();

		delete pS;
		__UHEAP_MARKEND;
		}

	delete pR;
	return KErrNone;
	}


//
// RMyServer
//

class RMyServer : public RSessionBase
	{
public:
	enum TFunction
		{
		EStop,
		ESync,
		EPing,
		EShutdown,
		ECompleteWhileCopying
		};
public:
	TInt Connect();

	inline TInt Send(TFunction aFunction) const
		{ return SendReceive(aFunction); }

	inline TInt Send(TFunction aFunction, const TIpcArgs& aArgs) const
		{ return SendReceive(aFunction, aArgs); }

	inline void Send(TFunction aFunction, TRequestStatus& aStatus) const
		{ SendReceive(aFunction, aStatus); }

	inline void Send(TFunction aFunction, const TIpcArgs& aArgs, TRequestStatus& aStatus) const
		{ SendReceive(aFunction, aArgs, aStatus); }
	};


TInt RMyServer::Connect()
	{
	RMyServer temp;
	TInt r = temp.CreateSession(KMyServerName, TVersion(), UseGlobalMessagePool ? -1 : NumMessageSlots);
	if(r!=KErrNone)
		return r;

	// turn handle into process owned...
	RMyServer temp2(temp);
	r = temp2.Duplicate(RThread());
	temp.Close();

	*this = temp2;
	return r;
	}



//
// CMySession
//

TInt CopierThread(TAny* aPtr)
	{
	RMessage2& msg = *(RMessage2*)aPtr;
	HBufC* bigdes = HBufC::NewMax(BigDesLength);
	if (bigdes == NULL)
		return KErrNoMemory;
	TPtr ptr = bigdes->Des();
	RThread().Rendezvous(KErrNone);
	RDebug::Print(_L("START\n"));
	TInt r = msg.Read(2, ptr);
	RDebug::Print(_L("DONE\n"));
	delete bigdes;
	return r;
	}

void CMySession::ServiceL(const RMessage2& aMessage)
	{
	RThread client;
	RThread copier;
	aMessage.Client(client);
	TRequestStatus* s;
	TRequestStatus* s2;
	TRequestStatus logon, rendez;
	TInt r;
	s = (TRequestStatus*)aMessage.Ptr0();

	switch(aMessage.Function())
		{
	case RMyServer::EStop:
		TRACE("E");
		CActiveScheduler::Stop();
		break;

	case RMyServer::ESync:
		TRACE("Y");
		client.RequestComplete(s,KErrNone);		// let client know we've received the message
		SyncSemaphore.Wait();					// wait for signal from client
		s = (TRequestStatus*)aMessage.Ptr1();	// use second status for later end signal
		aMessage.Complete(KErrNone);			// complete the message
		break;

	case RMyServer::EPing:
		TRACE("P");
		aMessage.Complete(KErrNone);
		break;

	case RMyServer::EShutdown:
		TRACE("D");
		RestartServer = EFalse;
		CActiveScheduler::Stop();
		break;

	case RMyServer::ECompleteWhileCopying:
		s2 = (TRequestStatus*)aMessage.Ptr1();
		r = copier.Create(_L("Copier"),CopierThread,KDefaultStackSize,&User::Allocator(),(TAny*)&aMessage);
		if (r == KErrNone)
			{
			copier.Logon(logon);
			copier.Rendezvous(rendez);
			copier.SetPriority(EPriorityLess);
			copier.Resume();
			User::WaitForRequest(rendez);
			User::AfterHighRes(5000); // 5ms delay to let copy actually start
			RDebug::Print(_L("COMPLETING\n"));
			aMessage.Complete(KErrNone);
			User::WaitForRequest(logon);
			copier.Close();
			}
		client.RequestComplete(s,r);
		s = s2;
		break;

	default:
		TRACE("?");
		aMessage.Complete(KErrNotSupported);
		break;
		}

	// let client know we've completed the message...
	TRACE("X");
	client.RequestComplete(s,KErrNone);

	client.Close();
	}



//
// RStressThread
//

class RStressThread
	{
public:
	RStressThread(TThreadFunction aThreadFunction, const char* aName, TInt aDelay=-1);
	~RStressThread();
	void Start();
	void Restart();
	void Stop();
	// for use by thread...
	static RStressThread& Begin(TAny* aInfo);
	TBool Loop();
private:
	TThreadFunction iThreadFunction;
	const char* iName;
	RThread iThread;
	TRequestStatus iLogon;
	TUint iCount;
	TBool iStop;
	TInt iDelay;

private:
	static TInt iInstanceCounter;
	};


TInt RStressThread::iInstanceCounter = 0;


RStressThread::RStressThread(TThreadFunction aThreadFunction, const char* aName, TInt aDelay)
	: iThreadFunction(aThreadFunction), iName(aName), iLogon(KErrNone), iDelay(aDelay)
	{
	iThread.SetHandle(0);
	}


RStressThread::~RStressThread()
	{
	Stop();
	}


void RStressThread::Start()
	{
	iStop = false;
	iCount = 0;

	TBuf<KMaxKernelName> name;
	name.Copy(TPtrC8((const TUint8*)iName));
	name.Append((TText)'-');
	name.AppendNum(iInstanceCounter++);
	test_Equal(KErrNone,iThread.Create(name,iThreadFunction,KDefaultStackSize,&User::Allocator(),this));

	iThread.Logon(iLogon);
	test_Equal(KRequestPending,iLogon.Int());

	TRequestStatus rendezvous;
	iThread.Rendezvous(rendezvous);

	iThread.Resume();

	User::WaitForRequest(rendezvous);
	test_Equal(KErrNone,rendezvous.Int());
	}


void RStressThread::Stop()
	{
	if(!iThread.Handle())
		return; // thread not running

	iStop = true;
	RDebug::Printf("RStressThread::Stop %s (count=%d)",iName,iCount);
	if(WaitForRequest(iLogon,10*1000000)!=KErrNone)
		test(0);
	CLOSE_AND_WAIT(iThread);
	}


void RStressThread::Restart()
	{
	if(iThread.Handle())
		{
		if(iLogon==KRequestPending)
			return; // thread still running

		User::WaitForRequest(iLogon);
		CLOSE_AND_WAIT(iThread);
		}

	Start();
	}


TBool RStressThread::Loop()
	{
	if(iDelay>=0)
		User::AfterHighRes(iDelay);
	++iCount;
	return !iStop;
	}


RStressThread& RStressThread::Begin(TAny* aInfo)
	{
	RStressThread& t = *(RStressThread*)aInfo;
	if(t.iDelay>=0)
		RThread().SetPriority(EPriorityMore); // so this preempts threads after delay
	RThread::Rendezvous(KErrNone);
	return t;
	}

//
//
//


RMyServer Session;
RThread ServerThread;


void NewSession()
	{
	RMyServer newSession;
	TRACE("o");
	test_Equal(KErrNone,newSession.Connect());

	RMyServer oldSession(Session);
	Session = newSession;

	TRACE("c");
	if(oldSession.Handle())
		CLOSE_AND_WAIT(oldSession);
	}


TInt SessionCloserThread(TAny* aInfo)
	{
	RStressThread& t = RStressThread::Begin(aInfo);
	do
		{
		NewSession();
		}
	while(t.Loop());
	return KErrNone;
	}


TInt ServerStopperThread(TAny* aInfo)
	{
	RStressThread& t = RStressThread::Begin(aInfo);
	do
		{
		TRACE("s");
		TRequestStatus rendezvous;
		ServerThread.Rendezvous(rendezvous);

		TRequestStatus s1 = KRequestPending;
		TRequestStatus s2;
		Session.Send(RMyServer::EStop,TIpcArgs(&s1),s2);
		User::WaitForRequest(s1,s2);
		if(s2!=KRequestPending)
			{
			test_Equal(KErrServerTerminated,s2.Int());
			User::WaitForRequest(s1);
			}

		User::WaitForRequest(rendezvous);
		NewSession();
		}
	while(t.Loop());
	return KErrNone;
	}


TInt SessionPingerThread(TAny* aInfo)
	{
	RStressThread& t = RStressThread::Begin(aInfo);
	do
		{
		TRACE("p");
		TRequestStatus s1 = KRequestPending;
		TRequestStatus s2;
		Session.Send(RMyServer::EPing,TIpcArgs(&s1),s2);
		User::WaitForRequest(s1,s2);
		if(s2.Int()==KErrNone)
			{
			// message completed OK, wait for servers extra signal
			User::WaitForRequest(s1);
			}
		else if(s2.Int()==KErrServerTerminated)
			{
			// server died before message processed, there shouldn't be an extra signal
			test_Equal(KRequestPending,s1.Int());
			}
		else
			{
			// assume message was completed by server, but we didn't get signalled because session was closed
			test_Equal(KRequestPending,s2.Int());
			test_Equal(KErrNone,s1.Int());
			}
		}
	while(t.Loop());
	return KErrNone;
	}


void TestInit()
	{
	RThread().SetPriority(EPriorityMuchMore); // so this main thread is higher priority than workers

	test_Equal(KErrNone,SyncSemaphore.CreateLocal(0,EOwnerProcess));

	// calculate async cleanup timeout value...
	TInt factor = UserSvr::HalFunction(EHalGroupVariant, EVariantHalTimeoutExpansion, 0, 0);
	if (factor<=0)
		factor = 1;
	if (factor>1024)
		factor = 1024;
	WaitABit = 200000 * (TUint32)factor;
	}


void StartServer()
	{
	// start test server...
	test_Equal(KErrNone,ServerThread.Create(_L("Server"),MyServerThread,KDefaultStackSize,1<<12,1<<20,0));
	TRequestStatus rendezvous;
	ServerThread.Rendezvous(rendezvous);
	ServerThread.Resume();
	User::WaitForRequest(rendezvous);
	test_Equal(KErrNone,rendezvous.Int());
	test_Equal(EExitPending,ServerThread.ExitType());
	}


void StopServer()
	{
	TRequestStatus logon;
	NewSession();
	TRequestStatus s1 = KRequestPending;
	TRequestStatus s2;
	ServerThread.Logon(logon);
	Session.Send(RMyServer::EShutdown,TIpcArgs(&s1),s2);
	User::WaitForRequest(s1,s2);
	if(s2!=KRequestPending)
		{
		test_Equal(KErrServerTerminated,s2.Int());
		User::WaitForRequest(s1);
		}
	CLOSE_AND_WAIT(Session);
	User::WaitForRequest(logon);
	test_KErrNone(logon.Int());
	test_Equal(EExitKill, ServerThread.ExitType());
	CLOSE_AND_WAIT(ServerThread);
	}


void TestMessageCompleteOnClosedSession()
	{
	__KHEAP_MARK;

	test.Start(_L("Start server"));
	StartServer();

	test.Next(_L("Connect"));
	test_Equal(KErrNone,Session.Connect());

	test.Next(_L("Send message"));
	TRequestStatus s1 = KRequestPending;
	TRequestStatus s2 = KRequestPending;
	TRequestStatus s3;
	Session.Send(RMyServer::ESync,TIpcArgs(&s1,&s2),s3);
	test_Equal(KRequestPending,s3.Int());

	test.Next(_L("Wait for s1"));
	test_Equal(KErrNone,WaitForRequest(s1));
	test_Equal(KErrNone,s1.Int());
	test_Equal(KRequestPending,s2.Int());
	test_Equal(KRequestPending,s3.Int());

	test.Next(_L("Close session"));
	Session.Close();
	test_Equal(KRequestPending,s2.Int());
	test_Equal(KRequestPending,s3.Int());

	test.Next(_L("Trigger message completion"));
	SyncSemaphore.Signal();

	test.Next(_L("Wait for s2"));
	test_Equal(KErrNone,WaitForRequest(s2));
	test_Equal(KErrNone,s2.Int());
	test_Equal(KRequestPending,s3.Int());

	test.Next(_L("Stop server"));
	StopServer();

	test.End();

	User::After(WaitABit);	// allow asynchronous cleanup to happen

	__KHEAP_MARKEND;
	}


void TestMessageCompleteWhileCopying()
	{
	__KHEAP_MARK;

	test.Start(_L("Start server"));
	StartServer();

	test.Next(_L("Connect"));
	test_Equal(KErrNone,Session.Connect());

	test.Next(_L("Create large descriptor"));
	HBufC* bigdes = HBufC::NewMax(BigDesLength);
	test_NotNull(bigdes);
	TPtr ptr = bigdes->Des();

	test.Next(_L("Send message"));
	TRequestStatus s1 = KRequestPending;
	TRequestStatus s2 = KRequestPending;
	TRequestStatus s3;
	Session.Send(RMyServer::ECompleteWhileCopying,TIpcArgs(&s1,&s2,&ptr),s3);

	test.Next(_L("Wait for s3"));
	test_Equal(KErrNone,WaitForRequest(s3,10*1000000));
	test_Equal(KErrNone,s3.Int());

	test.Next(_L("Wait for s2"));
	test_Equal(KErrNone,WaitForRequest(s2,10*1000000));
	test_Equal(KErrNone,s2.Int());

	test.Next(_L("Wait for s1"));
	test_Equal(KErrNone,WaitForRequest(s1,10*1000000));
	test_Equal(KErrNone,s1.Int());

	test.Next(_L("Close session"));
	Session.Close();

	test.Next(_L("Stop server"));
	StopServer();

	test.End();

	User::After(WaitABit);	// allow asynchronous cleanup to happen

	__KHEAP_MARKEND;
	}


void RunStressThreads(RStressThread& aThread1, RStressThread& aThread2, TInt aTimeout=1000000)
	{
	__KHEAP_MARK;

	StartServer();

	NewSession();

	aThread1.Start();
	aThread2.Start();

	RTimer timer;
	test_Equal(KErrNone,timer.CreateLocal());
	TRequestStatus timeoutStatus;
	timer.After(timeoutStatus,aTimeout);
	do
		{
		aThread1.Restart();
		aThread2.Restart();
		WaitForRequest();
		}
	while(timeoutStatus==KRequestPending);
	User::WaitForRequest(timeoutStatus);
	CLOSE_AND_WAIT(timer);

	aThread2.Stop();
	aThread1.Stop();

	CLOSE_AND_WAIT(Session);
	StopServer();

	User::After(WaitABit);	// allow asynchronous cleanup to happen
	__KHEAP_MARKEND;
	}


GLDEF_C TInt E32Main()
	{
	TInt i;

	test.Title();

	test.Start(_L("Initialise"));
	TestInit();

	for(UseGlobalMessagePool=0; UseGlobalMessagePool<2; ++UseGlobalMessagePool)
		{
		if(UseGlobalMessagePool)
			test.Next(_L("Tests using global message pool"));
		else
			test.Next(_L("Tests using local message pool"));

		NumMessageSlots = 1;

		test.Start(_L("Check completing messages on dead session"));
		TestMessageCompleteOnClosedSession();

		for (i=0; i<10; i++)
			{
			test.Next(_L("Check completing message while IPC copying"));
			TestMessageCompleteWhileCopying();
			}

		test.Next(_L("Stress closing session whilst in use"));
		RStressThread closer(SessionCloserThread,"SessionCloser",0);
		RStressThread pinger1(SessionPingerThread,"Pinger");
		RunStressThreads(closer, pinger1);

		NumMessageSlots = 2;

		test.Next(_L("Stress stopping server whilst in use"));
		RStressThread stopper(ServerStopperThread,"ServerStopper",0);
		RStressThread pinger2(SessionPingerThread,"Pinger");
		RunStressThreads(stopper, pinger2);

		test.End();
		}

	test.End();
	return(0);
	}

