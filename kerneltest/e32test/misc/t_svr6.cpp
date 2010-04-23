// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\t_svr6.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32svr.h>
#include "u32std.h"
#include "../misc/prbs.h"
#include "../mmu/freeram.h"

const TInt KStackSize=0x1000;
const TInt KHeapMaxSize=0x100000;
const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=0;
const TInt KBuildVersionNumber=1;
const TInt KNumMessageSlots=10;

_LIT(KServerName,"StressSvr");

LOCAL_D RTest test(_L("T_SVR6"));

class CMySession : public CSession2
	{
public:
	CMySession();
	virtual void ServiceL(const RMessage2& aMessage);			 //pure virtual fns.
	void Process(const RMessage2& aMessage);
public:
	TInt iOutstanding;
	TUint iSeed[2];
	};

class CMyServer : public CServer2
	{
public:
	enum {ETest};
public:
	CMyServer(TInt aPriority);
	static CMyServer* New(TInt aPriority);
	virtual CSession2* NewSessionL(const TVersion& aVersion, const RMessage2&) const;//Overloading
	};

class CMyActiveScheduler : public CActiveScheduler
	{
public:
	virtual void Error(TInt anError) const;  //Overloading pure virtual function
	};

class RStressSvr : public RSessionBase
	{
public:
	TInt Connect();
	TInt Test();
	void Test(TRequestStatus& aStatus);
	TVersion Version();
	};

class CThread : public CActive
	{
public:
	CThread(TInt aPriority);
	~CThread();
	virtual void RunL();
	virtual void DoCancel();
	virtual void DisplayStats()=0;
	virtual TBool PanicBadHandleAllowed();
	virtual void RegisterAllowedPanic();
	virtual void Cleanup();
	virtual TInt ProcessStartError(TInt anError);
	TInt Start();
public:
	virtual TInt StartThread()=0;
public:
	RThread iThread;
	TInt iExitCount;
	TInt iServerTerminatedCount;
	TInt iTerminateCount;
	};

class CServerThread : public CThread
	{
public:
	static void NewL();
	CServerThread();
	virtual TInt StartThread();
	virtual void DisplayStats();
public:
	TInt iMessagesReceived;
	};

class CClientThread : public CThread
	{
public:
	static void NewL(TInt anId, TInt aPrimaryId);
	CClientThread(TInt anId);
	virtual TInt StartThread();
	virtual void DisplayStats();
	virtual TBool PanicBadHandleAllowed();
	virtual void RegisterAllowedPanic();
	virtual void Cleanup();
	virtual TInt ProcessStartError(TInt anError);
public:
	TInt iId;
	TInt iCloses;
	CClientThread* iPrimary;
	RStressSvr iSession;
	TBool iConnected;
	TBool iWaitingToRestart;
	};

class CRandomTimer : public CActive
	{
public:
	static void NewL();
	CRandomTimer(TInt aPriority);
	~CRandomTimer();
	virtual void RunL();
	virtual void DoCancel();
	void Start();
public:
	RTimer iTimer;
	TUint iSeed[2];
	TInt iCount;
	};

class CStatsTimer : public CActive
	{
public:
	static void NewL();
	CStatsTimer(TInt aPriority);
	~CStatsTimer();
	virtual void RunL();
	virtual void DoCancel();
	void Start();
public:
	RTimer iTimer;
	TInt iInitFreeRam;
	TInt iMaxDelta;
	TInt iCount;
	};

const TInt KNumPrimaryClients=3;
const TInt KNumSecondariesPerPrimary=3;
const TInt KNumClients=KNumPrimaryClients*KNumSecondariesPerPrimary;
LOCAL_D CServerThread* TheServer;
LOCAL_D CClientThread* TheClients[KNumClients];
LOCAL_D CRandomTimer* TheRandomTimer;

CMySession::CMySession()
//
// Constructor
//
	{
	iSeed[0]=User::TickCount();
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
	: CServer2(aPriority, ESharableSessions)
	{}

CSession2* CMyServer::NewSessionL(const TVersion& aVersion, const RMessage2&) const
//
// Create a new client for this server.
//
	{

	TVersion v(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	if (!User::QueryVersionSupported(v,aVersion))
		User::Leave(KErrNotSupported);
	return new(ELeave) CMySession;
	}

void CMySession::ServiceL(const RMessage2& aMessage)
//
// Handle messages for this server.
//
	{

	++TheServer->iMessagesReceived;
	switch (aMessage.Function())
		{
		case CMyServer::ETest:
			if (iOutstanding==KNumMessageSlots-1)
				Process(aMessage);
			else
				++iOutstanding;
			break;
		default:
			aMessage.Complete(KErrNotSupported);
			break;
		}
	}

void CMySession::Process(const RMessage2& aMessage)
	{
	TUint x=Random(iSeed)&16383;
	if (x==0)
		User::Exit(0);			// exit the server
	else if (x<8)
		aMessage.Terminate(0);	// terminate the client
	else
		aMessage.Complete(KErrNone);
	}

void CMyActiveScheduler::Error(TInt anError) const
//
// Called if any Run() method leaves.
//
	{

	User::Panic(_L("Server Error"),anError);
	}

TInt RStressSvr::Connect()
//
// Connect to the server
//
	{

	TInt r=CreateSession(KServerName,Version(),KNumMessageSlots);
	if (r==KErrNone)
		r=ShareAuto();
	return r;
	}

TInt RStressSvr::Test()
//
// Send a message and wait for completion.
//
	{

	return SendReceive(CMyServer::ETest);
	}

void RStressSvr::Test(TRequestStatus& aStatus)
//
// Send a message asynchronously
//
	{

	SendReceive(CMyServer::ETest,aStatus);
	}

TVersion RStressSvr::Version()
//
// Return the current version.
//
	{

	TVersion v(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	return(v);
	}

LOCAL_C TInt ServerThread(TAny*)
	{

	CMyActiveScheduler* pR=new CMyActiveScheduler;
	if (!pR)
		return KErrNoMemory;
	CActiveScheduler::Install(pR);
	CMyServer* pS=CMyServer::New(0);
	if (!pS)
		return KErrNoMemory;
	TInt r=pS->Start(KServerName);
	if (r!=KErrNone)
		return r;

	CActiveScheduler::Start();

	delete pS;
	delete pR;
	return KErrNone;
	}

LOCAL_C TInt ClientThread(TAny* aPtr)
	{
	CClientThread* pT=(CClientThread*)aPtr;
	CClientThread* pP=pT->iPrimary;
	TBool primary=(pP==pT);
	RStressSvr& d=pP->iSession;
	TUint seed[2];
	seed[0]=User::TickCount();
	seed[1]=0;
	TInt r=KErrNone;
	TInt i;
	if (primary)
		{
		FOREVER
			{
			FOREVER
				{
				r=d.Connect();
				if (r!=KErrNotFound)
					break;
				User::After(50000);
				}
			if (r!=KErrNone)
				return r;
			pT->iConnected=ETrue;
			TRequestStatus s[KNumMessageSlots];
			for (i=0; i<KNumMessageSlots-1; i++)
				d.Test(s[i]);
			TInt n=Random(seed)&16383;
			for (i=0; i<n && r==KErrNone; i++)
				r=d.Test();
			pT->iConnected=EFalse;
			d.Close();
			++pT->iCloses;
			}
		}
	else
		{
		TRequestStatus s[KNumMessageSlots];
		for (i=0; i<KNumMessageSlots-1; i++)
			d.Test(s[i]);
		FOREVER
			{
			if (pP->iConnected)
				{
				RStressSvr dd;
				dd.SetHandle(pP->iSession.Handle());
				TInt n=Random(seed)&16383;
				for (i=0; i<n && r==KErrNone; i++)
					r=dd.Test();
				}
			TInt ms=(Random(seed)&7)+1;
			User::AfterHighRes(ms*1000);
			}
		}
	}

CThread::CThread(TInt aPriority)
	: CActive(aPriority)
	{
	}

CThread::~CThread()
	{
	Cancel();
	iThread.Kill(0);
	iThread.Close();
	}

_LIT(KLitKernExec,"KERN-EXEC");
void CThread::RunL()
	{
	TExitType exitType=iThread.ExitType();
	TInt exitReason=iThread.ExitReason();
	TBuf<32> exitCat=iThread.ExitCategory();
	TBool bad=EFalse;
	if (exitType==EExitKill)
		{
		if (exitReason!=KErrNone && exitReason!=KErrServerTerminated)
			bad=ETrue;
		}
	else if (exitType==EExitPanic)
		{
		if (!PanicBadHandleAllowed() || exitCat!=KLitKernExec || exitReason!=EBadHandle)
			bad=ETrue;
		else
			RegisterAllowedPanic();
		}
	if (bad)
		{
		TFullName n(iThread.FullName());
		test.Printf(_L("Thread %S exited %d,%d,%S\n"),&n,exitType,exitReason,&exitCat);
		CActiveScheduler::Stop();
		return;
		}
	Cleanup();
	iThread.Close();
	if (exitType==EExitTerminate)
		++iTerminateCount;
	else if (exitType==EExitKill && exitReason==KErrNone)
		++iExitCount;
	else if (exitReason==KErrServerTerminated)
		++iServerTerminatedCount;
	TInt r=Start();
	if (r!=KErrNone)
		{
		test.Printf(_L("Start thread error %d\n"),r);
		CActiveScheduler::Stop();
		}
	}

void CThread::DoCancel()
	{
	iThread.LogonCancel(iStatus);
	}

const TInt KThreadStartAttempts=3;
TInt CThread::Start()
	{
	TInt r=KErrNone;
	TInt n=KThreadStartAttempts;
	while(n--)
		{
		r=StartThread();
		if (r==KErrNone)
			break;
		if (r!=KErrAlreadyExists)
			break;
		User::After(100000);
		}
	if (r==KErrNone)
		{
		iThread.Logon(iStatus);
		SetActive();
		}
	return ProcessStartError(r);
	}

TBool CThread::PanicBadHandleAllowed()
	{
	return EFalse;
	}

void CThread::RegisterAllowedPanic()
	{
	}

void CThread::Cleanup()
	{
	}

TInt CThread::ProcessStartError(TInt anError)
	{
	return anError;
	}

CServerThread::CServerThread()
	: CThread(0)
	{
	}

TInt CServerThread::StartThread()
	{
	TUint seed[2];
	seed[1]=0;
	seed[0]=User::TickCount();
	TInt heapMin=TInt(Random(seed)&0x0f)+1;
	heapMin<<=12;
	TInt r=iThread.Create(KNullDesC(),ServerThread,KStackSize,heapMin,KHeapMaxSize,this);	// use unnamed thread
	if (r!=KErrNone)
		return r;
	iThread.Resume();
	return KErrNone;
	}

void CServerThread::NewL()
	{
	CServerThread* pT=new (ELeave) CServerThread;
	TheServer=pT;
	CActiveScheduler::Add(pT);
	User::LeaveIfError(pT->Start());
	}

void CServerThread::DisplayStats()
	{
	test.Printf(_L("Svr  : X:%9d ST:%9d T:%9d RX:%9d\n"),iExitCount,iServerTerminatedCount,iTerminateCount,iMessagesReceived);
	}

CClientThread::CClientThread(TInt anId)
	: CThread(0), iId(anId)
	{
	}

TInt CClientThread::StartThread()
	{
	TInt r=iThread.Create(KNullDesC(),ClientThread,KStackSize,NULL,this);	// use unnamed threads
	if (r!=KErrNone)
		return r;
	iSession.SetHandle(0);
	iThread.Resume();
	return KErrNone;
	}

void CClientThread::NewL(TInt anId, TInt aPrimaryId)
	{
	CClientThread* pT=new (ELeave) CClientThread(anId);
	TheClients[anId]=pT;
	pT->iPrimary=TheClients[aPrimaryId];
	CActiveScheduler::Add(pT);
	User::LeaveIfError(pT->Start());
	}

void CClientThread::DisplayStats()
	{
	test.Printf(_L("Cli %1d: X:%9d ST:%9d T:%9d CL:%9d\n"),iId,iExitCount,iServerTerminatedCount,iTerminateCount,iCloses);
	}

TBool CClientThread::PanicBadHandleAllowed()
	{
	return (iPrimary!=this);
	}

void CClientThread::RegisterAllowedPanic()
	{
	++iCloses;
	}

void CClientThread::Cleanup()
	{
	TInt r=KErrNone;
	if (iPrimary==this)
		{
		if (!IsLocalHandle(iSession.Handle()))	// don't close if not shared yet
			iSession.Close();
		CClientThread* pS1=TheClients[iId+1];
		CClientThread* pS2=TheClients[iId+2];
		if (pS1->iWaitingToRestart)
			r=pS1->Start();
		if (r==KErrNone && pS2->iWaitingToRestart)
			r=pS2->Start();
		if (r!=KErrNone)
			{
			test.Printf(_L("Start thread error %d\n"),r);
			CActiveScheduler::Stop();
			}
		}
	}

TInt CClientThread::ProcessStartError(TInt anError)
	{
	if (anError==KErrAlreadyExists && iPrimary!=this && !iWaitingToRestart)
		{
		iWaitingToRestart=ETrue;
		return KErrNone;
		}
	iWaitingToRestart=EFalse;
	return anError;
	}

void CRandomTimer::NewL()
	{
	CRandomTimer* pR=new (ELeave) CRandomTimer(20);
	User::LeaveIfError(pR->iTimer.CreateLocal());
	CActiveScheduler::Add(pR);
	TheRandomTimer=pR;
	pR->Start();
	}

CRandomTimer::CRandomTimer(TInt aPriority)
	: CActive(aPriority)
	{
	iSeed[0]=User::TickCount();
	}

CRandomTimer::~CRandomTimer()
	{
	Cancel();
	iTimer.Close();
	}

void CRandomTimer::RunL()
	{
	++iCount;
	TUint x=Random(iSeed)&15;
	CThread* pT=NULL;
	if (x==0)
		pT=TheServer;
	else if (x<10)
		{
		pT=TheClients[x-1];
		if (((CClientThread*)pT)->iWaitingToRestart)
			pT=NULL;
		}
	if (pT)
		pT->iThread.Kill(0);
	Start();
	}

void CRandomTimer::Start()
	{
	TUint x=Random(iSeed)&63;
	x+=64;
	iTimer.HighRes(iStatus, x*1000);
	SetActive();
	}

void CRandomTimer::DoCancel()
	{
	iTimer.Cancel();
	}

void CStatsTimer::NewL()
	{
	CStatsTimer* pT=new (ELeave) CStatsTimer(-10);
	User::LeaveIfError(pT->iTimer.CreateLocal());
	CActiveScheduler::Add(pT);
	pT->Start();
	}

CStatsTimer::CStatsTimer(TInt aPriority)
	: CActive(aPriority)
	{
	iInitFreeRam = FreeRam();
	}

CStatsTimer::~CStatsTimer()
	{
	Cancel();
	iTimer.Close();
	}

void CStatsTimer::RunL()
	{
	TheServer->DisplayStats();
	TInt i;
	for (i=0; i<KNumClients; i++)
		TheClients[i]->DisplayStats();
	test.Printf(_L("RndTm: %9d\n"),TheRandomTimer->iCount);
	TInt free_ram = FreeRam();
	TInt delta_ram = iInitFreeRam - free_ram;
	if (delta_ram > iMaxDelta)
		iMaxDelta = delta_ram;
	if (++iCount==10)
		{
		test.Printf(_L("Max RAM delta %dK Free RAM %08x\n"), iMaxDelta/1024, free_ram);
		iCount=0;
		}
	Start();
	}

void CStatsTimer::Start()
	{
	iTimer.After(iStatus, 1000000);
	SetActive();
	}

void CStatsTimer::DoCancel()
	{
	iTimer.Cancel();
	}

void InitialiseL()
	{
	CActiveScheduler* pA=new (ELeave) CActiveScheduler;
	CActiveScheduler::Install(pA);
	CServerThread::NewL();
	TInt p;
	TInt s;
	TInt id=0;
	for (p=0; p<KNumClients; p+=KNumSecondariesPerPrimary)
		{
		for (s=0; s<KNumSecondariesPerPrimary; s++)
			{
			CClientThread::NewL(id,p);
			id++;
			}
		}
	CRandomTimer::NewL();
	CStatsTimer::NewL();
	}

GLDEF_C TInt E32Main()
//
// Test timers.
//
	{

	test.Title();

	User::SetCritical(User::ESystemCritical);
	RThread().SetPriority(EPriorityMore);
	User::SetJustInTime(EFalse);	// prevent the debugger picking up expected thread panics.

	TRAPD(r,InitialiseL());
	test(r==KErrNone);

	CActiveScheduler::Start();

	test(0);

	return(0);
	}

// Override heap creation for this process
// This function runs at the beginning of every thread
// Initial heap is shared but subsequent heaps are single threaded

EXPORT_C TInt UserHeap::SetupThreadHeap(TBool aNotFirst, SStdEpocThreadCreateInfo& aInfo)
	{
	TInt r = KErrNone;
	if (!aInfo.iAllocator && aInfo.iHeapInitialSize>0)
		{
		// new heap required
		RHeap* pH = NULL;
		r = CreateThreadHeap(aInfo, pH, 0, aNotFirst);
		}
	else if (aInfo.iAllocator)
		{
		// sharing a heap
		RAllocator* pA = aInfo.iAllocator;
		pA->Open();
		User::SwitchAllocator(pA);
		}
	return r;
	}

