// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\t_svr_connect.cpp
// Overview:
// Tests correct operation of server when interleaving connect/disconnect/
// other messages and creating the user-side session object (setting the
// session cookie) in interesting manners.
// Tests that clients/servers are panicked when performing illegal operations
// w.r.t. server connection, i.e. a client thread may not send more than one
// connect message simultaneously, nor may it send another connect message
// once a connect message has been successfully completed. Similarly, a server
// may not set the cookie twice nor may it set the cookie to be NULL. Also, a
// server may only set the cookie from a connect message and from no other.
// API Information:
// RServer2
// Details:
// - Test asynchronous server connect in various ways. Verify results
// are as expected.
// - Test illegal client/server behaviour. Verify threads are panicked,
// as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// None.
// Failures and causes:
// Failure of this test would be caused by an incorrect modification of
// the internal client/server mechanism in the kernel.
// Base Port information:
// This is a unit test of the client/server mechanism within the kernel.
// It should be invariant to base ports. If the kernel proper has not been
// modified, this test should not fail with any new base port.
// 
//

#include <e32test.h>
#include "../misc/int_svr_calls.h"
#include <e32kpan.h>

_LIT(KTestName, "t_svr_connect");
LOCAL_D RTest test(KTestName);
LOCAL_D RTest test_client(KTestName);
LOCAL_D RTest test_server(KTestName);

_LIT(KServerName, "t_svr_connect");
_LIT8(KServerName8, "t_svr_connect");
_LIT(KMainThread, "main thread");
_LIT(KServerThread, "server thread");
_LIT(KClientThread, "client thread");
_LIT(KClientThread2, "client thread2");
_LIT(KClientThread3, "client thread3");

class RTestServer : public RSessionBase
	{
public:
	inline static TInt CreateSession(TInt aMsgSlots) { return SessionCreate(KServerName8,aMsgSlots,NULL,EIpcSession_Sharable); }
	};

// Messages to send:
//
//		connect,				1
//		a.n.other,				2
//		disconnect				3
//
// Things to do in the server:
//
//		recieve connect			4
//		set cookie				5
//		complete connect		6
//		receive a.n.other		7
//		complete a.n.other		8
//		receive disconnect		9
//		complete disconnect		10
//		

enum TMsgType
	{
	EConnect = -1,
	EDisConnect = -2,
	EANOther = 1,
	EANOther2 = 2,
	EMsgCount = 4
	};

enum TServerControl
	{
	EReceiveMsg = 0,
	EReceiveBlocked = 1,
	EWaitForReceive = 2,
	ESetCookie = 3,
	ESetNullCookie = 4,
	ECompleteMsg = 5,
	EServerDie = 6
	};

enum TClientControl
	{
	ESendMsg = 0,
	EWaitMsg = 1,
	ECreateSession = 2,
	EClientDie = 3,
	ESetCritical = 4
	};

static TMsgType TheMsgType;
static TServerControl TheServerControl;
static TClientControl TheClientControl;
static RSemaphore ServerSemaphore;
static RSemaphore ClientSemaphore;
static RSemaphore TaskCompletionSemaphore;
static RMessage2 Msgs[EMsgCount];
static RServer2 Server;
static RThread ServerThread;
static RThread ClientThread;
static RTestServer ServerHandle;
static TRequestStatus ConnectStatus;
static TRequestStatus ANOtherStatus;
static TRequestStatus ANOther2Status;
static TRequestStatus ServerReceiveStatus;
static TInt TheNumberOfMsgSlots;
static TInt ErrorExpected;
static User::TCritical CriticalLevel;


static const TAny* const KSessionCookie = (const TAny*)0x12345;
static TRequestStatus* volatile KNullReference = 0;

void DoServerAction();
void DoClientAction();
TInt Index();

TInt TestThreadServer(TAny*)
	{
	test_server(Server.CreateGlobal(KServerName) == KErrNone);
	RThread::Rendezvous(KErrNone);

	for (;;)
		{
		ServerSemaphore.Wait();
		DoServerAction();
		TaskCompletionSemaphore.Signal();
		}
	}

TInt Index()
	{
	switch (TheMsgType)
		{
	case EConnect:
		return 0;
	case EDisConnect:
		return 3;
	case EANOther:
		return 1;
	case EANOther2:
		return 2;
	default:
		return -1;
		};
	}

TRequestStatus& RequestStatus()
	{
	switch (TheMsgType)
		{
	case EConnect:
		return ConnectStatus;
	case EANOther:
		return ANOtherStatus;
	case EANOther2:
		return ANOther2Status;
	default:
		User::Invariant();
		};
	return *(TRequestStatus*)KNullReference;
	}

void DoServerAction()
	{
	switch (TheServerControl)
		{
	case EReceiveMsg:
		{
		RMessage2& msg = Msgs[Index()];
		Server.Receive(msg);
		test_server(msg.Function() == TheMsgType);
		}
		break;
	case EReceiveBlocked:
		{
		RMessage2& msg = Msgs[Index()];
		Server.Receive(msg, ServerReceiveStatus);
		test_server(ServerReceiveStatus.Int() == KRequestPending);
		}
		break;
	case EWaitForReceive:
		{
		User::WaitForRequest(ServerReceiveStatus);
		test_server(ServerReceiveStatus.Int() == KErrNone);
		test_server(Msgs[Index()].Function() == TheMsgType);
		}
		break;
	case ESetCookie:
		{
		SetSessionPtr(Msgs[Index()].Handle(), KSessionCookie);
		}
		break;
	case ESetNullCookie:
		{
		SetSessionPtr(Msgs[Index()].Handle(), NULL);
		}
		break;
	case ECompleteMsg:
		{
		Msgs[Index()].Complete(KErrNone);
		}
		break;
	case EServerDie:
		{
		User::Exit(0);
		}
		break;
	default:
		{
		}
		break;
		};
	}

void StartServer()
	{
	ServerThread.Create(KServerThread, TestThreadServer, KDefaultStackSize, 4096, 4096, NULL);

	TRequestStatus started;
	ServerThread.Rendezvous(started);

	ServerThread.Resume();
	User::WaitForRequest(started);

	test(started.Int() == KErrNone);
	}

void StopServer()
	{
	TRequestStatus logon;
	ServerThread.Logon(logon);

	TheServerControl = EServerDie;
	ServerSemaphore.Signal();

	User::WaitForRequest(logon);
	test(logon.Int() == KErrNone);

	CLOSE_AND_WAIT(ServerThread);
	}

TInt TestThreadClient(TAny*)
	{
	RThread::Rendezvous(KErrNone);

	for (;;)
		{
		ClientSemaphore.Wait();
		DoClientAction();
		TaskCompletionSemaphore.Signal();
		}
	}

void DoClientAction()
	{
	switch (TheClientControl)
		{
	case ESendMsg:
		{
		if (TheMsgType == EDisConnect)
			ServerHandle.Close();
		else
			SessionSend(ServerHandle.Handle(), TheMsgType, NULL, &RequestStatus());
		}
		break;
	case EWaitMsg:
		{
		User::WaitForRequest(RequestStatus());
		}
		break;
	case ECreateSession:
		{
		TInt err = ServerHandle.SetReturnedHandle(RTestServer::CreateSession(TheNumberOfMsgSlots));
		
		if (err != ErrorExpected)
			{
			test_client.Printf(_L("Error returned = %d\n"),err);
			test_client(EFalse,__LINE__);
			}
		}
		break;
	case EClientDie:
		{
		User::SetCritical(User::ENotCritical);
		User::Exit(0);
		}
		break;
	case ESetCritical:
		{
		User::SetCritical(CriticalLevel);
		break;
		}
	default:
		{
		}
		break;
		};
	}

// I'm lazy and I haven't completed all the IPC the client thread does,
// so even when the client thread panics, the DObject is still kept alive
// until the server goes away. Therefore if I want another client I rename.
void StartClient(const TDesC& aName)
	{
	ClientThread.Create(aName, TestThreadClient, KDefaultStackSize, 4096, 4096, NULL);

	TRequestStatus started;
	ClientThread.Rendezvous(started);

	ClientThread.Resume();
	User::WaitForRequest(started);

	test(started.Int() == KErrNone);
	}

void StartClient()
	{
	StartClient(KClientThread);
	}

void StopClient()
	{
	TRequestStatus logon;
	ClientThread.Logon(logon);

	TheClientControl = EClientDie;
	ClientSemaphore.Signal();

	User::WaitForRequest(logon);
	test(logon.Int() == KErrNone);

	CLOSE_AND_WAIT(ClientThread);
	}

void CreateSemaphores()
	{
	TInt err = ServerSemaphore.CreateLocal(0);
	test(err == KErrNone);
	err = ClientSemaphore.CreateLocal(0);
	test(err == KErrNone);
	err = TaskCompletionSemaphore.CreateLocal(0);
	test(err == KErrNone);
	}

void CloseSemaphores()
	{
	ServerSemaphore.Close();
	ClientSemaphore.Close();
	TaskCompletionSemaphore.Close();
	}

void CreateSession(TInt aErrorExpected=KErrNone, TInt aMsgSlots=-1)
	{
	TheClientControl = ECreateSession;
	TheNumberOfMsgSlots = aMsgSlots;
	ErrorExpected=aErrorExpected;
	ClientSemaphore.Signal();
	TaskCompletionSemaphore.Wait();
	}

void SendMsg(TMsgType aType)
	{
	TheClientControl = ESendMsg;
	TheMsgType = aType;
	ClientSemaphore.Signal();
	TaskCompletionSemaphore.Wait();
	}

void SendMsg_NoWait(TMsgType aType)
	{
	TheClientControl = ESendMsg;
	TheMsgType = aType;
	ClientSemaphore.Signal();
	}

void WaitMsg(TMsgType aType)
	{
	TheClientControl = EWaitMsg;
	TheMsgType = aType;
	ClientSemaphore.Signal();
	TaskCompletionSemaphore.Wait();
	}

void ReceiveBlocked(TMsgType aType)
	{
	TheServerControl = EReceiveBlocked;
	TheMsgType = aType;
	ServerSemaphore.Signal();
	TaskCompletionSemaphore.Wait();
	}

void WaitForReceive(TMsgType aType)
	{
	TheServerControl = EWaitForReceive;
	TheMsgType = aType;
	ServerSemaphore.Signal();
	TaskCompletionSemaphore.Wait();
	}

void ReceiveMsg(TMsgType aType)
	{
	TheServerControl = EReceiveMsg;
	TheMsgType = aType;
	ServerSemaphore.Signal();
	TaskCompletionSemaphore.Wait();
	}

void CompleteMsg(TMsgType aType)
	{
	TheServerControl = ECompleteMsg;
	TheMsgType = aType;
	ServerSemaphore.Signal();
	TaskCompletionSemaphore.Wait();
	}

void SetCookie()
	{
	TheServerControl = ESetCookie;
	TheMsgType = EConnect;
	ServerSemaphore.Signal();
	TaskCompletionSemaphore.Wait();
	}

void SetCookie_NoWait()
	{
	TheServerControl = ESetCookie;
	TheMsgType = EConnect;
	ServerSemaphore.Signal();
	}

void SetNullCookie()
	{
	TheServerControl = ESetNullCookie;
	TheMsgType = EConnect;
	ServerSemaphore.Signal();
	}

void SetBadCookie(TMsgType aType)
	{
	TheServerControl = ESetCookie;
	test(aType != EConnect);
	TheMsgType = aType;
	ServerSemaphore.Signal();
	}
void SetClientCritical(User::TCritical aCritical)
	{	
	TheClientControl = ESetCritical;
	CriticalLevel=aCritical;
	ClientSemaphore.Signal();
	TaskCompletionSemaphore.Wait();
	}
	
void Test1()
	{
	test.Next(_L("Create session with test server"));
	CreateSession();
	test.Next(_L("Send connect message"));
	SendMsg(EConnect);
	test.Next(_L("Send A.N.Other message"));
	SendMsg(EANOther);
	test.Next(_L("Sending disconnect message"));
	SendMsg(EDisConnect);
	test.Next(_L("Receive A.N.Other message"));
	ReceiveMsg(EANOther);
	test(Msgs[Index()].Session() == NULL);
	test.Next(_L("Receive disconnect message"));
	ReceiveMsg(EDisConnect);
	test.Next(_L("Check the session has gone"));
	test(Msgs[Index()].Session() == NULL);
	test.Next(_L("Set up receive for next test"));
	ReceiveBlocked(EConnect);
	}

void Test2()
	{
	test.Next(_L("Create session with test server"));
	CreateSession();
	test.Next(_L("Send connect message"));
	SendMsg(EConnect);
	test.Next(_L("Send A.N.Other message"));
	SendMsg(EANOther);
	test.Next(_L("Receive connect message"));
	WaitForReceive(EConnect);
	test(Msgs[Index()].Session() == NULL);
	test.Next(_L("Receive A.N.Other message"));
	ReceiveMsg(EANOther);
	test(Msgs[Index()].Session() == NULL);
	test.Next(_L("Sending disconnect message"));
	SendMsg(EDisConnect);
	test.Next(_L("Await disconnect message"));
	ReceiveBlocked(EDisConnect);
	test.Next(_L("Set cookie"));
	SetCookie();
	test.Next(_L("Check disconnect message received"));
	WaitForReceive(EDisConnect);
	test(Msgs[Index()].Session() == KSessionCookie);
	test.Next(_L("Complete connect message"));
	CompleteMsg(EConnect);
	}

void Test2a()
	{
	CreateSession();
	SendMsg(EConnect);
	SendMsg(EANOther);
	ReceiveMsg(EConnect);
	test(Msgs[Index()].Session() == NULL);
	ReceiveMsg(EANOther);
	test(Msgs[Index()].Session() == NULL);
	SendMsg(EDisConnect);
	ReceiveBlocked(EDisConnect);
	CompleteMsg(EConnect);
	WaitForReceive(EDisConnect);
	test(Msgs[Index()].Session() == NULL);
	}

void Test2b()
	{
	CreateSession();
	SendMsg(EConnect);
	SendMsg(EANOther);
	ReceiveMsg(EConnect);
	test(Msgs[Index()].Session() == NULL);
	SetCookie();
	ReceiveMsg(EANOther);
	test(Msgs[Index()].Session() == KSessionCookie);
	SendMsg(EDisConnect);
	ReceiveMsg(EDisConnect);
	test(Msgs[Index()].Session() == KSessionCookie);
	}

void Test3()
	{
	CreateSession();
	SendMsg(EConnect);
	ReceiveMsg(EConnect);
	test(Msgs[Index()].Session() == NULL);
	SendMsg(EANOther);
	ReceiveMsg(EANOther);
	test(Msgs[Index()].Session() == NULL);
	SendMsg(EDisConnect);
	ReceiveBlocked(EDisConnect);
	SetCookie();
	WaitForReceive(EDisConnect);
	test(Msgs[Index()].Session() == KSessionCookie);
	CompleteMsg(EConnect);
	}

void Test3a()
	{
	CreateSession();
	SendMsg(EConnect);
	ReceiveMsg(EConnect);
	test(Msgs[Index()].Session() == NULL);
	SendMsg(EANOther);
	ReceiveMsg(EANOther);
	test(Msgs[Index()].Session() == NULL);
	SendMsg(EDisConnect);
	ReceiveBlocked(EDisConnect);
	CompleteMsg(EConnect);
	WaitForReceive(EDisConnect);
	test(Msgs[Index()].Session() == NULL);
	}

void Test3b()
	{
	CreateSession();
	SendMsg(EConnect);
	ReceiveMsg(EConnect);
	test(Msgs[Index()].Session() == NULL);
	SetCookie();
	SendMsg(EANOther);
	ReceiveMsg(EANOther);
	test(Msgs[Index()].Session() == KSessionCookie);
	SendMsg(EDisConnect);
	ReceiveMsg(EDisConnect);
	test(Msgs[Index()].Session() == KSessionCookie);
	}

void Test4()
	{
	CreateSession();
	SendMsg(EANOther);
	SendMsg(EConnect);
	SendMsg(EANOther2);
	SendMsg(EDisConnect);
	ReceiveMsg(EANOther);
	test(Msgs[Index()].Session() == NULL);
	ReceiveMsg(EANOther2);
	test(Msgs[Index()].Session() == NULL);
	ReceiveMsg(EDisConnect);
	test(Msgs[Index()].Session() == NULL);
	}

void Test5()
	{
	CreateSession();
	SendMsg(EANOther);
	SendMsg(EConnect);
	SendMsg(EANOther2);
	ReceiveMsg(EANOther);
	test(Msgs[Index()].Session() == NULL);
	SendMsg(EDisConnect);
	ReceiveMsg(EANOther2);
	test(Msgs[Index()].Session() == NULL);
	ReceiveMsg(EDisConnect);
	test(Msgs[Index()].Session() == NULL);
	ReceiveBlocked(EANOther);
	}

void Test6()
	{
	CreateSession();
	SendMsg(EANOther);
	SendMsg(EConnect);
	SendMsg(EANOther2);
	WaitForReceive(EANOther);
	test(Msgs[Index()].Session() == NULL);
	ReceiveMsg(EConnect);
	test(Msgs[Index()].Session() == NULL);
	ReceiveMsg(EANOther2);
	test(Msgs[Index()].Session() == NULL);
	SendMsg(EDisConnect);
	ReceiveBlocked(EDisConnect);
	SetCookie();
	WaitForReceive(EDisConnect);
	test(Msgs[Index()].Session() == KSessionCookie);
	CompleteMsg(EConnect);
	}

void Test6a()
	{
	CreateSession();
	SendMsg(EANOther);
	SendMsg(EConnect);
	SendMsg(EANOther2);
	ReceiveMsg(EANOther);
	test(Msgs[Index()].Session() == NULL);
	ReceiveMsg(EConnect);
	test(Msgs[Index()].Session() == NULL);
	ReceiveMsg(EANOther2);
	test(Msgs[Index()].Session() == NULL);
	SendMsg(EDisConnect);
	ReceiveBlocked(EDisConnect);
	CompleteMsg(EConnect);
	WaitForReceive(EDisConnect);
	test(Msgs[Index()].Session() == NULL);
	}

void Test6b()
	{
	CreateSession();
	SendMsg(EANOther);
	SendMsg(EConnect);
	SendMsg(EANOther2);
	ReceiveMsg(EANOther);
	test(Msgs[Index()].Session() == NULL);
	ReceiveMsg(EConnect);
	test(Msgs[Index()].Session() == NULL);
	SetCookie();
	ReceiveMsg(EANOther2);
	test(Msgs[Index()].Session() == KSessionCookie);
	SendMsg(EDisConnect);
	ReceiveMsg(EDisConnect);
	test(Msgs[Index()].Session() == KSessionCookie);
	}

void Test7()
	{
	CreateSession();
	SendMsg(EANOther);
	SendMsg(EConnect);
	ReceiveMsg(EANOther);
	test(Msgs[Index()].Session() == NULL);
	SendMsg(EANOther2);
	ReceiveMsg(EConnect);
	test(Msgs[Index()].Session() == NULL);
	SetCookie();
	ReceiveMsg(EANOther2);
	test(Msgs[Index()].Session() == KSessionCookie);
	SendMsg(EDisConnect);
	ReceiveMsg(EDisConnect);
	test(Msgs[Index()].Session() == KSessionCookie);
	}

void Test8()
	{
	CreateSession();
	SendMsg(EANOther);
	SendMsg(EConnect);
	ReceiveMsg(EANOther);
	test(Msgs[Index()].Session() == NULL);
	ReceiveMsg(EConnect);
	test(Msgs[Index()].Session() == NULL);
	SetCookie();
	SendMsg(EANOther2);
	ReceiveMsg(EANOther2);
	test(Msgs[Index()].Session() == KSessionCookie);
	SendMsg(EDisConnect);
	ReceiveMsg(EDisConnect);
	test(Msgs[Index()].Session() == KSessionCookie);
	}

void Test9()
	{
	CreateSession();
	SendMsg(EANOther);
	ReceiveMsg(EANOther);
	test(Msgs[Index()].Session() == NULL);
	SendMsg(EConnect);
	ReceiveMsg(EConnect);
	test(Msgs[Index()].Session() == NULL);
	SetCookie();
	SendMsg(EANOther2);
	ReceiveMsg(EANOther2);
	test(Msgs[Index()].Session() == KSessionCookie);
	SendMsg(EDisConnect);
	ReceiveMsg(EDisConnect);
	test(Msgs[Index()].Session() == KSessionCookie);
	}

void Test10()
	{
	// Try connecting with too many message slots
	// (Check for DEF091903 regression)
	CreateSession(KErrArgument, 1000);
	}

_LIT(KKernExec, "KERN-EXEC");

void CheckClientDeath(TInt aReason)
	{
	TRequestStatus logon;

	ClientThread.Logon(logon);
	User::WaitForRequest(logon);

	test(ClientThread.ExitType() == EExitPanic);
	test(ClientThread.ExitCategory() == KKernExec);
	test(ClientThread.ExitReason() == aReason);

	ClientThread.Close();
	ClientThread.SetHandle(KNullHandle);
	}

void CheckServerDeath(TInt aReason)
	{
	TRequestStatus logon;

	ServerThread.Logon(logon);
	User::WaitForRequest(logon);

	test(ServerThread.ExitType() == EExitPanic);
	test(ServerThread.ExitCategory() == KKernExec);
	test(ServerThread.ExitReason() == aReason);

	CLOSE_AND_WAIT(ServerThread);
	ServerThread.SetHandle(KNullHandle);
	}

void TestNaughty()
	{
	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);

	SetClientCritical(User::ENotCritical);
	test.Next(_L("Two connect msgs at once is naughty"));
	CreateSession();
	SendMsg(EConnect);
	ReceiveMsg(EConnect);
	SetCookie();
	SendMsg_NoWait(EConnect);
	CheckClientDeath(ERequestAlreadyPending);
	StartClient(KClientThread2);
	test.Next(_L("Another connect message after a sucessful connect msg is naughty"));
	CreateSession();
	SendMsg(EConnect);
	ReceiveMsg(EConnect);
	SetCookie();
	CompleteMsg(EConnect);
	SendMsg_NoWait(EConnect);
	CheckClientDeath(ESessionAlreadyConnected);
	StartClient(KClientThread3);

	test.Next(_L("A null session cookie is naughty"));
	CreateSession();
	SendMsg(EConnect);
	ReceiveMsg(EConnect);
	SetNullCookie();
	CheckServerDeath(ESessionNullCookie);
	StartServer();

	test.Next(_L("Setting the session cookie twice is naughty"));
	CreateSession();
	SendMsg(EConnect);
	ReceiveMsg(EConnect);
	SetCookie();
	SetCookie_NoWait();
	CheckServerDeath(ESessionCookieAlreadySet);
	StartServer();

	test.Next(_L("Trying to set the session cookie from a non-connect message is naughty"));
	CreateSession();
	SendMsg(EConnect);
	ReceiveMsg(EConnect);
	SendMsg(EANOther);
	ReceiveMsg(EANOther);
	SetBadCookie(EANOther);
	CheckServerDeath(ESessionInvalidCookieMsg);
	StartServer();

	User::SetJustInTime(jit);
	}

TInt E32Main()
    {
	User::RenameThread(KMainThread);

	__UHEAP_MARK;

	test.Title();

	test.Start(_L("Creating semaphores"));
	CreateSemaphores();

	test.Next(_L("Starting test server"));
	StartServer();

	test.Next(_L("Starting test client"));
	StartClient();
	SetClientCritical(User::EProcessCritical);
	// test combinations of receiving messages with messages sent by the client in the
	// correct order (w.r.t to each other, but not necessarily to when messages are received)
	test.Next(_L("Sending in order"));
	test.Next(_L("1"));
	Test1();
	test.Next(_L("2"));
	Test2();
	test.Next(_L("2a"));
	Test2a();
	test.Next(_L("2b"));
	Test2b();
	test.Next(_L("3"));
	Test3();
	test.Next(_L("3a"));
	Test3a();
	test.Next(_L("3b"));
	Test3b();

	// test combinations of receiving messages with messages sent by the client in the
	// wrong order (w.r.t to each other, but not necessarily to when messages are received)
	test.Next(_L("Sending out of order"));
	test.Next(_L("4"));
	Test4();
	test.Next(_L("5"));
	Test5();
	test.Next(_L("6"));
	Test6();
	test.Next(_L("6a"));
	Test6a();
	test.Next(_L("6b"));
	Test6b();
	test.Next(_L("7"));
	Test7();
	test.Next(_L("8"));
	Test8();
	test.Next(_L("9"));
	Test9();
	test.Next(_L("10"));
	Test10();

	test.Next(_L("Test other naughty behaviour is trapped"));
	TestNaughty();

	test.Next(_L("Stopping test client"));
	StopClient();

	test.Next(_L("Stopping test server"));
	StopServer();

	test.Next(_L("Closing semaphores"));
	CloseSemaphores();

	test.End();
	test.Close();

	__UHEAP_MARKEND;

	return KErrNone;
    }
