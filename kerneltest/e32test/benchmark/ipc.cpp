// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32test.h>

#include "bm_suite.h"

class RIpcSession : public RSessionBase
	{
public:
	TInt CreateSession(TDesC& aName, TVersion aVer, TInt aSlots)
		{
		return RSessionBase::CreateSession(aName, aVer, aSlots);
		}
	void SendReceive(TInt aService, TIpcArgs& args, TRequestStatus& aStatus)
		{
		RSessionBase::SendReceive(aService, args, aStatus);
		}
	TInt SendReceive(TInt aService, TIpcArgs& args)
		{
		return RSessionBase::SendReceive(aService, args);
		}
	};

class CIpcScheduler : public CActiveScheduler
	{
public:
	CIpcScheduler()
		{
		CActiveScheduler::Install(this);
		}
	};

class CIpcServer : public CServer2
	{
public:

	enum TService 
		{
		ERunTest,
		EGetTimes,
		EStop
		};

	struct TServerTimes
		{
		TBMTicks iNewSessionEntryTime;
		TBMTicks iNewSessionLeaveTime;
		TBMTicks iServiceLTime;
		};

	CIpcServer() : CServer2(0)
		{
		}
	virtual CSession2* NewSessionL(const TVersion& aVer, const RMessage2&) const;

	TServerTimes	iTimes;

	static TInt Entry(TAny* ptr);
	};

class CIpcSession : public CSession2
	{
public:
	CIpcSession(CIpcServer* aServer)
		{
		iServer = aServer;
		}
	virtual void ServiceL(const RMessage2& aMessage);

	CIpcServer*	iServer;
	};

class SpawnArgs : public TBMSpawnArgs
	{
public:

	TBuf<16>	iServerName;
	TVersion	iVersion;

	RSemaphore	iSem;
	TInt		iIterationCount;

	SpawnArgs(const TPtrC& aServerName, TInt aPrio, TInt aRemote, TInt aIter);

	void Close();
	void ServerOpen();
	void ServerClose();

	};

SpawnArgs::SpawnArgs(const TPtrC& aServerName, TInt aPrio, TInt aRemote, TInt aIter) :
	TBMSpawnArgs(CIpcServer::Entry, aPrio, aRemote, sizeof(*this)),
		iServerName(aServerName),
		iVersion(1,0,1), 
		iIterationCount(aIter)
	{
	TInt r;
	if (aRemote)
		{
		r = iSem.CreateGlobal(_L("Semaphore"), 0);
		BM_ERROR(r, r == KErrNone);
		}
	else
		{
		r = iSem.CreateLocal(0);
		BM_ERROR(r, r == KErrNone);
		}
	}

void SpawnArgs::ServerOpen()
	{
	if (iRemote)
		{
		iSem.Duplicate(iParent);
		}
	}

void SpawnArgs::ServerClose()
	{
	if (iRemote)
		{
		iSem.Close();
		}
	}

void SpawnArgs::Close()
	{
	iSem.Close();
	}

void CIpcSession::ServiceL(const RMessage2& aMessage)
	{
	CIpcServer::TService f = (CIpcServer::TService) aMessage.Function();
	switch (f) 
		{
	case CIpcServer::ERunTest:
		::bmTimer.Stamp(&iServer->iTimes.iServiceLTime);
		break;
	case CIpcServer::EGetTimes:
		aMessage.WriteL(0, TPtrC8((TUint8*) &iServer->iTimes, sizeof(iServer->iTimes)));
		break;
	case CIpcServer::EStop:
		CActiveScheduler::Stop();
		break;
	default:
		BM_ASSERT(0);
		}
	aMessage.Complete(KErrNone);
	}
	
CSession2* CIpcServer::NewSessionL(const TVersion&, const RMessage2&) const
	{
	CIpcServer* srv = (CIpcServer*) this;
	::bmTimer.Stamp(&srv->iTimes.iNewSessionEntryTime);
	CSession2* s  = new CIpcSession(srv);
	BM_ERROR(KErrNoMemory, s);
	::bmTimer.Stamp(&srv->iTimes.iNewSessionLeaveTime);
	return s;
	}

TInt CIpcServer::Entry(TAny* ptr)
	{
	SpawnArgs* sa = (SpawnArgs*) ptr;

	sa->ServerOpen();

	CIpcScheduler* sched = new CIpcScheduler();
	BM_ERROR(KErrNoMemory, sched);

	CIpcServer* srv = new CIpcServer();
	BM_ERROR(KErrNoMemory, srv);
	TInt r = srv->Start(sa->iServerName);
	BM_ERROR(r, r == KErrNone);
	
		// signal to the parent the end of the server intialization
	sa->iSem.Signal();
		
	sched->Start();

	delete srv;
	delete sched;

	sa->ServerClose();

	return KErrNone;
}

static const TInt KMaxIpcLatencyResults = 5;

class IpcLatency : public BMProgram
	{
public :

	TBool		iRemote;
	TInt		iPriority;
	TBMResult	iResults[KMaxIpcLatencyResults];

	IpcLatency(TBool aRemote, TInt aPriority) : 
		BMProgram(
			aRemote 
				? ((aPriority == KBMPriorityHigh) 
					? _L("Client-server Framework[Remote High Priority Server]")
					: _L("Client-server Framework[Remote Low Priority Server]"))
				: ((aPriority == KBMPriorityHigh) 
					? _L("Client-server Framework[Local High Priority Server]")
					: _L("Client-server Framework[Local Low Priority Server]")))
		{
		iPriority = aPriority;
		iRemote = aRemote;
		}

	virtual TBMResult* Run(TBMUInt64 aIter, TInt* aCount);
	};

TBMResult* IpcLatency::Run(TBMUInt64 aIter, TInt* aCount)
	{
	SpawnArgs sa(_L("BMServer"), iPriority, iRemote, (TInt) aIter);

		// spawn the server
	MBMChild* child = SpawnChild(&sa);

	TInt n = 0;
	iResults[n++].Reset(_L("Connection Request Latency"));
	iResults[n++].Reset(_L("Connection Reply Latency"));
	iResults[n++].Reset(_L("Request Latency"));
	iResults[n++].Reset(_L("Request Response Time"));
	iResults[n++].Reset(_L("Reply Latency"));
	BM_ASSERT(KMaxIpcLatencyResults >= n);

		// wait for the srever intialization
	sa.iSem.Wait();
		// wait 2ms (ie more than one tick) to allow the server to complete its ActiveScheduler intialization ...
	User::After(2000);

	RIpcSession s;

	for (TBMUInt64 i = 0; i < aIter; ++i)
		{	
		TBMTicks t1;
		::bmTimer.Stamp(&t1);
		TInt r = s.CreateSession(sa.iServerName, sa.iVersion, 1);
		BM_ERROR(r, r == KErrNone);
		TBMTicks t2;
		::bmTimer.Stamp(&t2);

		TRequestStatus st;

		TBMTicks t3;
		::bmTimer.Stamp(&t3);

			{
			TIpcArgs args;
			s.SendReceive(CIpcServer::ERunTest, args, st);
			}

		TBMTicks t4;
		::bmTimer.Stamp(&t4);

		User::WaitForRequest(st);
		BM_ERROR(r, st == KErrNone);

		TBMTicks t5;
		::bmTimer.Stamp(&t5);

		CIpcServer::TServerTimes serverTimes;
		TPtr8 serverTimesDes((TUint8*) &serverTimes, sizeof(serverTimes), sizeof(serverTimes));

			{
			TIpcArgs args(&serverTimesDes);
			r = s.SendReceive(CIpcServer::EGetTimes, args);
			BM_ERROR(r, r == KErrNone);
			}

		s.Close();	
		
		n = 0;
		iResults[n++].Cumulate(TBMTicksDelta(t1, serverTimes.iNewSessionEntryTime));
		iResults[n++].Cumulate(TBMTicksDelta(serverTimes.iNewSessionLeaveTime, t2));
		iResults[n++].Cumulate(TBMTicksDelta(t3, serverTimes.iServiceLTime));
		iResults[n++].Cumulate(TBMTicksDelta(t3, t4));
		iResults[n++].Cumulate(TBMTicksDelta(serverTimes.iServiceLTime, t5));
		BM_ASSERT(KMaxIpcLatencyResults >= n);	

			// wait 2ms (ie more than one tick) to allow the server to complete.
		User::After(2000);
		}

	TInt r = s.CreateSession(sa.iServerName, sa.iVersion, 1);
	BM_ERROR(r, r == KErrNone);
		{
		TIpcArgs args;
		s.SendReceive(CIpcServer::EStop, args);
		}
	s.Close();
		
	child->WaitChildExit();

	sa.Close();

	for (TInt j = 0; j < KMaxIpcLatencyResults; ++j)
		{
		iResults[j].Update();
		}

	*aCount = KMaxIpcLatencyResults;
	return iResults;
	}

IpcLatency test1(EFalse,KBMPriorityHigh);
IpcLatency test2(EFalse,KBMPriorityLow );
IpcLatency test3(ETrue, KBMPriorityHigh);
IpcLatency test4(ETrue, KBMPriorityLow );

void AddIpc()
	{
	BMProgram* next = bmSuite;
	bmSuite=(BMProgram*)&test4;
	bmSuite->Next()=next;
	bmSuite=(BMProgram*)&test3;
	bmSuite->Next()=&test4;
	bmSuite=(BMProgram*)&test2;
	bmSuite->Next()=&test3;
	bmSuite=(BMProgram*)&test1;
	bmSuite->Next()=&test2;
	}
