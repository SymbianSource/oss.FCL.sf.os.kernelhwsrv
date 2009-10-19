// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\debug\t_eventtracker.cpp
// User-side harness for a LDD-based debug agent which keeps 
// track of various events (e.g. thread creation) and tests 
// them for consistency (e.g. a thread deletion event should be 
// preceded with a thread creation one).
// USAGE:
// 1.	start t_eventtracker usehook
// to hook event tracker to the stop-mode debugger breakpoint, or
// start t_eventtracker
// to add it to the kernel event handler queue
// 2.	<< do something e.g. run some tests >>
// 3.	t_eventtracker stop
// KNOWN ISSUES:
// * The debug agent allocates memory on the kernel heap.
// Consequently, it will perturbate any test using 
// __KHEAP_FAILNEXT() (e.g. t_kheap).
// * There is a window of opportunity when tracking is started
// or stopped during which some events may be lost, leading
// to incorrect results.  Therefore t_eventtracker should
// be called only when the system is idle.
// 
//

#include <e32test.h>
#include "reventtracker.h"

RTest test(_L("T_EVENTTRACKER"));

_LIT(KTrackerServerName, "EventTrackerServer");

//////////////////////////////////////////////////////////////////////////////

/** Wrapper around LDD session with debug agent */

class CTracker : public CBase
	{
public:
	~CTracker();
	void Start(TBool aUseHook);
	TInt Stop();
private:
	REventTracker iEventTracker;
	};

CTracker::~CTracker()
	{
	iEventTracker.Close();
	}

void CTracker::Start(TBool aUseHook)
	{
	TInt r = User::LoadLogicalDevice(_L("D_EVENTTRACKER.LDD"));
	test(r == KErrNone || r == KErrAlreadyExists);

	test(iEventTracker.Open(aUseHook) == KErrNone);
	test(iEventTracker.Start() == KErrNone);
	}

TInt CTracker::Stop()
	{
	TInt r=iEventTracker.Stop();
	iEventTracker.Close();
	return r;
	}

//////////////////////////////////////////////////////////////////////////////


/** Server used to interact with the debug agent */

class CTrackerServer : public CServer2
	{
public:
	static CTrackerServer* NewLC(TBool aUseHook);
	~CTrackerServer();
	// from CServer2
	CSession2* NewSessionL(const TVersion& aVersion,const RMessage2&) const;
private:
	CTrackerServer();
public:
	CTracker* iTracker;
	};

/** Session used to stop event tracking */

class CTrackerSession : public CSession2
	{
private:
	// from CSession2
	void ServiceL(const RMessage2& aMessage);
	};

void CTrackerSession::ServiceL(const RMessage2& aMessage)
	{
	TInt r = ((CTrackerServer*)Server())->iTracker->Stop();
	CActiveScheduler::Stop();
	aMessage.Complete(r);
	}

CTrackerServer* CTrackerServer::NewLC(TBool aUseHook)
	{
	CTrackerServer* server = new(ELeave) CTrackerServer;
	CleanupStack::PushL(server);
	server->iTracker = new(ELeave) CTracker;
	server->StartL(KTrackerServerName);
	server->iTracker->Start(aUseHook);
	return server;
	}

CTrackerServer::CTrackerServer()
	: CServer2(EPriorityStandard)
	{
	}

CTrackerServer::~CTrackerServer()
	{
	delete iTracker;
	}

CSession2* CTrackerServer::NewSessionL(const TVersion& /*aVersion*/,const RMessage2&) const
	{
	return new(ELeave) CTrackerSession;
	}

//////////////////////////////////////////////////////////////////////////////

/** Handle to client/server session */

class RTracker : public RSessionBase
	{
public:
	TInt Open() { return CreateSession(KTrackerServerName, TVersion(), 0); }
	TInt Stop() { return SendReceive(0); }
	};

//////////////////////////////////////////////////////////////////////////////

void MainL()
	{
	_LIT(KStop,"stop");
	_LIT(KUseHook,"usehook");
	TBuf<64> c;
	User::CommandLine(c);
	if (c.FindF(KStop) >= 0)
		{
		// Stop event tracking
		RTracker t;
		test(t.Open() == KErrNone);
		test(t.Stop() == KErrNone);
		t.Close();
		}
	else
		{
		// Create server and start event tracking
		test(User::RenameThread(KTrackerServerName) == KErrNone);

		TBool useHook = EFalse;
		if (c.FindF(KUseHook) >= 0)
			{
			useHook = ETrue;
			}

		CTrackerServer* server = CTrackerServer::NewLC(useHook);

		CActiveScheduler::Start();

		CleanupStack::PopAndDestroy(server);
		}
	}


TInt E32Main()
	{
	test.Title();
	
	__UHEAP_MARK;

	CTrapCleanup* cleanup = CTrapCleanup::New();
	test(cleanup != NULL);
	CActiveScheduler* scheduler = new CActiveScheduler;
	test(scheduler != NULL);
	CActiveScheduler::Install(scheduler);

	TRAPD(r, MainL());
	if (r != KErrNone)
		{
		test.Printf(_L("Unexpected leave: %d\n"), r);
		test(EFalse);
		}

	delete scheduler;
	delete cleanup;

	__UHEAP_MARKEND;

	return 0;
	}
