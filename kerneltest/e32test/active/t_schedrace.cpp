// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\active\t_schedrace.cpp
// Overview:
// Test for race conditions in emulator scheduler
// API Information:
// RMessage2, RMessagePtr2, RSessionBase, CSession2, CServer2, CPeriodic
// Details:
// - The client and server shuttle messages back and forth with variable timing
// - The client also has a periodic timer, which fires at a random phase relative to the IPC
// - The client should have higher priority than the server
// - The race may occur when the timer interrupt preempts kernel code and disables interrupts
// - The "preempted" code nonetheless runs and sees the "impossible" disabled interrupts
// - The result is a precondition failure at a random point
// Platforms/Drives/Compatibility:
// Will run on all platforms, but precondition failure is expected only on the UDEB emulator.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:

#define	__E32TEST_EXTENSION__

#include <e32std.h>
#include <e32std_private.h>
#include <e32math.h>
#include <e32panic.h>
#include <e32svr.h>
#include <e32test.h>
#include <e32ver.h>

_LIT(KServerName, "CTestServer");
const TVersion version(KE32MajorVersionNumber,
					   KE32MinorVersionNumber,
					   KE32BuildVersionNumber);

// Globals for counting number of times each thread/object runs
TInt nIdleCalls, nTimerCalls, nServerCalls;



//
// Server classes and code ...
//
class CTestServer : public CServer2
	{
public:
	IMPORT_C CTestServer(RTest* aTest) : CServer2(EPriorityIdle), iTest(aTest) {};
protected:
	IMPORT_C CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;
public:
	RTest* iTest;
	};

class CTestSession : public CSession2
	{
public:
	IMPORT_C void ServiceL(const RMessage2& aMessage);
	enum Action
		{
		EStop = 0,
		EDelay
		};
public:
	RTest* iTest;
	};

//
// CTestServer functions
//
EXPORT_C CSession2* CTestServer::NewSessionL(const TVersion& aVersion, const RMessage2& /*aMessage*/) const
	{
	if (User::QueryVersionSupported(version, aVersion) == EFalse)
		User::Leave(KErrNotSupported);

	CTestSession* newCTestSession = new CTestSession;
	if (newCTestSession == NULL)
		User::Leave(KErrNoMemory);
	newCTestSession->iTest = iTest;

	return newCTestSession;
	}

//
// CTestSession functions
//
EXPORT_C void CTestSession::ServiceL(const RMessage2& aMessage)
	{
	if ((++nServerCalls & 0x03ff) == 10000)		// never true, to suppress message
		iTest->Printf(_L("S:calls: I=%-7d  S=%-7d  T=%-7d  D=%-7d\n"),
				nIdleCalls, nServerCalls, nTimerCalls, aMessage.Int0());
	TInt r = KErrNone;
	TInt i = 0;

	switch (aMessage.Function())
		{
	case EStop:
		CActiveScheduler::Stop();
		break;

	case EDelay:
		// Spin a bit before replying
		// The compiler can't optimise this away because 'i' is used below ...
		for (i = 0; i < aMessage.Int0(); ++i)
			;
		break;

	default:
		r = KErrNotSupported;
		break;
		}

	if (i != -1)								// always true :)
		aMessage.Complete(r);
	}

//
// Thread to run the server code
//
TInt ServerThread(TAny*)
	{
	RTest test(_L("T_SCHEDRACE server"));
	test.Title();

// UserSvr::FsRegisterThread();

	test.Start(_L("Create and install ActiveScheduler"));
	CActiveScheduler* pScheduler = new CActiveScheduler;
	test_NotNull(pScheduler);
	CActiveScheduler::Install(pScheduler);

	test.Next(_L("Creating and starting Server"));
	CTestServer* pServer = new CTestServer(&test);
	test_NotNull(pServer);
	test_KErrNone(pServer->Start(KServerName));		// Starting a CServer2 also Adds it to the ActiveScheduler

	test.Next(_L("Rendezvous with main thread, then Start ActiveScheduler"));
	RThread self;
	self.Rendezvous(KErrNone);
	test.Printf(_L("        There might be something going on beneath this window\n"));
	CActiveScheduler::Start();

	// This code is not reached until the active scheduler exits.
	test.Next(_L("Destroy Server and ActiveScheduler"));
	delete pServer;
	delete pScheduler;
	test.Close();
	return (KErrNone);
	}



//
// Client classes and code ...
//
class RTestSession : public RSessionBase
	{
	public:
		RTestSession(RTest* aTest, TInt aTicks) : iTest(aTest), iMax(aTicks)
			{
			iDelay = iMax;
			iDelta = -1;
			};
		TInt Connect(const TDesC& aServer, int aSlots)
			{
			return RSessionBase::CreateSession(aServer, version, aSlots);
			};
		TInt SendBlind(TInt aFunction, const TIpcArgs& aArgs) const
			{
			return RSessionBase::Send(aFunction, aArgs);
			};
		TInt SendSync(TInt aFunction, const TIpcArgs& aArgs) const
			{
			return RSessionBase::SendReceive(aFunction, aArgs);
			};
		void SendAsync(TInt aFunction, const TIpcArgs& aArgs, TRequestStatus& aStatus) const
			{
			RSessionBase::SendReceive(aFunction, aArgs, aStatus);
			};
	public:
		RTest* iTest;
		TInt iMax;
		TInt iDelay;
		TInt iDelta;
	};

RTestSession* TheSession;

class CIdler : public CIdle
	{
	public:
		CIdler() : CIdle(EPriorityIdle)
			{
			CActiveScheduler::Add(this);
			};
		void Callback();
	};

void CIdler::Callback()
	{
	if ((++nIdleCalls & 0x03ff) == 10000)		// never true, to suppress message
		TheSession->iTest->Printf(_L("I:calls: I=%-7d  S=%-7d  T=%-7d  D=%-7d\n"),
						nIdleCalls, nServerCalls, nTimerCalls, TheSession->iDelay);
	// TInt delay = Math::Random() & 0xffff;					// up to ~64ms
	TheSession->SendBlind(CTestSession::EDelay, TIpcArgs(TheSession->iDelay));
	TheSession->SendAsync(CTestSession::EDelay, TIpcArgs(0), iStatus);
	SetActive();
	}

//
// CIdler callback wrapper
//
TInt IdleCallback(TAny* aPtr)
	{
	((CIdler*)aPtr)->Callback();
	return EFalse;
	}

//
// CPeriodic callback
//
TInt TimerCallback(TAny*)
	{
	if ((++nTimerCalls & 0x003f) == 1)			// true in one out of 64 cycles
		TheSession->iTest->Printf(_L("T:calls: I=%-7d  S=%-7d  T=%-7d  D=%-7d\n"),
						nIdleCalls, nServerCalls, nTimerCalls, TheSession->iDelay);
	if (TheSession->iDelay >= TheSession->iMax)
		TheSession->iDelta = -1;
//	if (TheSession->iDelay < 1)
//		TheSession->iDelta = 1;
	TheSession->iDelay += TheSession->iDelta;
	if (TheSession->iDelay < 0)
		CActiveScheduler::Stop();
	return ETrue;
	}

//
// Thread to run the client code
//
TInt ClientThread(TAny*)
	{
	RTest test(_L("T_SCHEDRACE client"));
	test.Title();

// UserSvr::FsRegisterThread();

	test.Start(_L("Create Session"));
	TheSession = new RTestSession(&test, 5*60*64);	// will run for 5 minutes
	test_NotNull(TheSession);
	test_KErrNone(TheSession->Connect(KServerName, 2));

	test.Start(_L("Create and install ActiveScheduler"));
	CActiveScheduler* pScheduler = new CActiveScheduler;
	test_NotNull(pScheduler);
	CActiveScheduler::Install(pScheduler);

	test.Next(_L("Create timer and idle task"));
	CPeriodic* pTimer = CPeriodic::New(CActive::EPriorityStandard);
	test_NotNull(pTimer);
	CIdler* pIdler = new CIdler();
	test_NotNull(pIdler);

	test.Next(_L("Rendezvous with main thread"));
	RThread self;
	self.Rendezvous(KErrNone);

	test.Next(_L("Start idle task, timer, and active scheduler"));
	pIdler->Start(TCallBack(IdleCallback, pIdler));
	pTimer->Start(1000000, 1000, TCallBack(TimerCallback, pTimer));
	test.Printf(_L("        There might be something going on beneath this window\n"));
	CActiveScheduler::Start();

	// This code is not reached until the active scheduler exits.
	TheSession->SendSync(CTestSession::EStop, TIpcArgs());
	TheSession->Close();
	test.Next(_L("Destroy Idle task, Timer, and ActiveScheduler"));
	delete pIdler;
	delete pTimer;
	delete pScheduler;
	test.Close();
	return (KErrNone);
	}


//
// Main program ...
//
GLDEF_C TInt E32Main()
	{
	RTest test(_L("Main T_SCHEDRACE test"));
	test.Title();

	test.Start(_L("Create server and client threads"));
	const TInt KHeapMinSize = 0x1000;
	const TInt KHeapMaxSize = 0x1000;
	RThread serverThread, clientThread;
	test_KErrNone(serverThread.Create(_L("Server Thread"), ServerThread, KDefaultStackSize, KHeapMinSize, KHeapMaxSize, NULL));
	test_KErrNone(clientThread.Create(_L("Client Thread"), ClientThread, KDefaultStackSize, KHeapMinSize, KHeapMaxSize, NULL));

	TRequestStatus serverStat, clientStat;
	serverThread.Rendezvous(serverStat);
	clientThread.Rendezvous(clientStat);
	serverThread.SetPriority(EPriorityMuchLess);
	clientThread.SetPriority(EPriorityMore);

	test.Next(_L("Start the threads"));
	serverThread.Resume();
	User::WaitForRequest(serverStat);
	test_KErrNone(serverStat.Int());
	clientThread.Resume();
	User::WaitForRequest(clientStat);
	test_KErrNone(clientStat.Int());

	test.Next(_L("Wait for the threads to stop"));
	serverThread.Logon(serverStat);
	clientThread.Logon(clientStat);
	User::WaitForRequest(clientStat);
	User::WaitForRequest(serverStat);

	switch (clientThread.ExitType())
		{
	case EExitKill:
		test.Printf(_L("  Client thread killed\n"));
		break;

	case EExitTerminate:
		test.Printf(_L("!!Client thread terminated:"));
		test.Panic(clientThread.ExitCategory(), clientThread.ExitReason());
		break;

	case EExitPanic:
		test.Panic(_L("!!Client thread panicked:"));
		test.Panic(clientThread.ExitCategory(), clientThread.ExitReason());
		break;

	default:
		test.Panic(_L("!!Client thread did something bizarre"), clientThread.ExitReason());
		break;
		}

	switch (serverThread.ExitType())
		{
	case EExitKill:
		test.Printf(_L("  Server thread killed\n"));
		break;

	case EExitTerminate:
		test.Printf(_L("!!Server thread terminated:"));
		test.Panic(serverThread.ExitCategory(), serverThread.ExitReason());
		break;

	case EExitPanic:
		//
		// To catch a panic put a breakpoint in User::Panic() (in UCDT\UC_UNC.CPP).
		//
		test.Printf(_L("!!Server thread panicked:"));
		test.Panic(serverThread.ExitCategory(), serverThread.ExitReason());
		break;

	default:
		test.Panic(_L("!!Server thread did something bizarre"), serverThread.ExitReason());
		break;
		}

	test.Next(_L("Close the threads"));
	CLOSE_AND_WAIT(serverThread);
	CLOSE_AND_WAIT(clientThread);
	test.End();
	return (KErrNone);
	}

