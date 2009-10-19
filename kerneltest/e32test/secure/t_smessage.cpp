// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\secure\t_smessage.cpp
// Overview:
// Test RMessagePtr2 and server message passing
// API Information:
// RMessagePtr2, RMessage2
// Details:
// - Start a test server and open a server session. Perform various IPC tests
// and verify results are as expected.
// - Start a thread and request the server kill, terminate and panic the client
// thread. Verify results are as expected.
// - Perform various tests of setting the process priority via the 
// RMessagePtr2::SetProcessPriority() method. Verify results are as expected.
// - Verify the ability to open a handle on the client thread using the 
// RMessagePtr2::Client() method.
// - Verify message passing via an RMessage2 object. 
// - Verify the RMessagePtr2::Handle and RMessagePtr::Session methods work
// as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("T_SMESSAGE"));

enum TTestProcessFunctions
	{
	ETestProcessServer,
	};

#include "testprocess.h"

TInt StartServer();

TInt DoTestProcess(TInt aTestNum,TInt aArg1,TInt aArg2)
	{
	(void)aArg1;
	(void)aArg2;

	switch(aTestNum)
		{

	case ETestProcessServer:
		return StartServer();

	default:
		User::Panic(_L("T_SMESSAGE"),1);
		}

	return KErrNone;
	}



class RTestThread : public RThread
	{
public:
	void Create(TThreadFunction aFunction,TAny* aArg=0);
	};

void RTestThread::Create(TThreadFunction aFunction,TAny* aArg)
	{
	TInt r=RThread::Create(_L(""),aFunction,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,aArg);
	test(r==KErrNone);
	}


//
// CTestSession
//

class CTestSession : public CSession2
	{
public:
	enum {EShutdown,ETestIpc,ETestKill,ETestTerminate,ETestPanic,
		ETestSetProcessPriority,ETestClient,ETestRMessage,ETestHandle,ETestSession};
public:
	CTestSession();
	virtual void ServiceL(const RMessage2& aMessage);
public:
	};

CTestSession::CTestSession()
	: CSession2()
	{
	}

const TInt KTestDataMaxLength8 = 20;
const TInt KTestDataMaxLength16 = 40;
_LIT8(KTestData8,"12345678");
_LIT16(KTestData16,"1234567890123456");
_LIT(KTestPanicCategory,"TEST PANIC");

void CTestSession::ServiceL(const RMessage2& aMessage)
	{
	RMessagePtr2 m(aMessage);
	switch (aMessage.Function())
		{
		case CTestSession::ETestIpc:
			{
			TInt r,l,ml;
			// 8 bit descriptors
			TBuf8<KTestDataMaxLength8> buf8;
			buf8.FillZ();
			// GetDesLength
			l = m.GetDesLength(0);
			if(l!=KTestData8().Length()) m.Complete(l<0 ? l : KErrGeneral);
			// Read
			r = m.Read(0,buf8,1);
			if(r!=KErrNone) m.Complete(r);
			if(buf8.Compare(KTestData8().Right(l-1))) m.Complete(KErrGeneral);
			// ReadL
			buf8.Zero();
			buf8.FillZ();
			m.ReadL(0,buf8,1);
			if(buf8.Compare(KTestData8().Right(l-1))) m.Complete(KErrGeneral);
			// GetDesMaxLength
			ml = m.GetDesMaxLength(2);
			if(ml!=KTestDataMaxLength8) m.Complete(ml<0 ? ml : KErrGeneral);
			// Write
			r = m.Write(2,KTestData8(),0);
			if(r!=KErrNone) m.Complete(r);
			// WriteL
			m.WriteL(2,KTestData8(),1);
			// 16 bit descriptors
			TBuf16<KTestDataMaxLength16> buf16;
			buf16.FillZ();
			// GetDesLength
			l = m.GetDesLength(1);
			if(l!=KTestData16().Length()) m.Complete(l<0 ? l : KErrGeneral);
			// Read
			r = m.Read(1,buf16,1);
			if(r!=KErrNone) m.Complete(r);
			if(buf16.Compare(KTestData16().Right(l-1))) m.Complete(KErrGeneral);
			// ReadL
			buf16.Zero();
			buf16.FillZ();
			m.ReadL(1,buf16,1);
			if(buf16.Compare(KTestData16().Right(l-1))) m.Complete(KErrGeneral);
			// GetDesMaxLength
			ml = m.GetDesMaxLength(3);
			if(ml!=KTestDataMaxLength16) m.Complete(ml<0 ? ml : KErrGeneral);
			// Write
			r = m.Write(3,KTestData16(),0);
			if(r!=KErrNone) m.Complete(r);
			// WriteL
			m.WriteL(3,KTestData16(),1);
			// done
			aMessage.Complete(KErrNone);
			}
			break;

		case CTestSession::ETestKill:
			m.Kill(999);
			break;

		case CTestSession::ETestTerminate:
			m.Terminate(999);
			break;

		case CTestSession::ETestPanic:
			m.Panic(KTestPanicCategory,999);
			break;

		case ETestSetProcessPriority:
			m.Complete(m.SetProcessPriority((TProcessPriority)aMessage.Int0()));
			break;

		case ETestClient:
			{
			RThread client;
			m.Client(client);
			m.Complete(client.Id());
			client.Close();
			}
			break;

		case ETestRMessage:
			{
			RMessage2 message2(m);
			if (Mem::Compare((TUint8*)&message2,sizeof(message2),(TUint8*)&aMessage,sizeof(aMessage)))
				m.Complete(KErrGeneral);
			else
				m.Complete(KErrNone);
			}
			break;

		case ETestHandle:
			if (m.Handle()!=aMessage.Handle())
				m.Complete(KErrGeneral);
			else
				m.Complete(KErrNone);
			break;

		case CTestSession::ETestSession:
			if (aMessage.Session()!=this)
				m.Complete(KErrGeneral);
			else
				m.Complete(KErrNone);
			break;

		case CTestSession::EShutdown:
			CActiveScheduler::Stop();
			break;

		default:
			m.Complete(KErrNotSupported);
			break;
		}
	}



//
// CTestServer
//

class CTestServer : public CServer2
	{
public:
	CTestServer(TInt aPriority);
	virtual CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const;
	};

CTestServer::CTestServer(TInt aPriority)
	: CServer2(aPriority)
	{
	}

CSession2* CTestServer::NewSessionL(const TVersion& /*aVersion*/,const RMessage2& /*aMessage*/) const
	{
	return new (ELeave) CTestSession();
	}



//
// CTestActiveScheduler
//

class CTestActiveScheduler : public CActiveScheduler
	{
public:
	virtual void Error(TInt anError) const;
	};

void CTestActiveScheduler::Error(TInt anError) const
	{
	User::Panic(_L("TestServer Error"),anError);
	}



//
// Server thread
//

_LIT(KServerName,"T_SMESSAGE-server");
const TInt KServerRendezvous = KRequestPending+1;

void DoStartServer()
	{
	CTestActiveScheduler* activeScheduler = new (ELeave) CTestActiveScheduler;
	CActiveScheduler::Install(activeScheduler);
	CleanupStack::PushL(activeScheduler);

	CTestServer* server = new (ELeave) CTestServer(0);
	CleanupStack::PushL(server);

	User::LeaveIfError(server->Start(KServerName));

	RProcess::Rendezvous(KServerRendezvous);

	CActiveScheduler::Start();

	CleanupStack::PopAndDestroy(2);
	}

TInt StartServer()
	{
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	if(!cleanupStack)
		return KErrNoMemory;
	TRAPD(leaveError,DoStartServer())
	delete cleanupStack;
	return leaveError;
	}



//
// RTestSession
//

class RTestSession : public RSessionBase
	{
public:
	inline TInt Connect()
		{ return CreateSession(KServerName,TVersion());}
	inline TInt Send(TInt aFunction)
		{ return RSessionBase::SendReceive(aFunction); }
	inline TInt Send(TInt aFunction,const TIpcArgs& aArgs)
		{ return RSessionBase::SendReceive(aFunction,aArgs); }
	inline void Send(TInt aFunction,TRequestStatus& aStatus)
		{ RSessionBase::SendReceive(aFunction,aStatus); }
	inline void Send(TInt aFunction,const TIpcArgs& aArgs,TRequestStatus& aStatus)
		{ RSessionBase::SendReceive(aFunction,aArgs,aStatus); }
	};



TInt TestThreadServerKill(TAny* aArg)
	{
	RTestSession Session;
	TInt r = Session.Connect();
	if(r!=KErrNone)
		return KErrGeneral;
	User::SetJustInTime(EFalse);
	r = Session.Send((TInt)aArg);
	Session.Close();
	return KErrGeneral;
	}


RTestSession Session;

void TestIPC()
	{
	TBuf8<KTestDataMaxLength8> buf8;
	TBuf16<KTestDataMaxLength16> buf16;
	TInt r = Session.Send(CTestSession::ETestIpc,TIpcArgs(&KTestData8,&KTestData16,&buf8,&buf16));
	test(r==KErrNone);
	TInt l = KTestData8().Length();
	test(buf8.Length()==l+1);
	test(KTestData8().Compare(buf8.Right(l))==0);
	l = KTestData16().Length();
	test(buf16.Length()==l+1);
	test(KTestData16().Compare(buf16.Right(l))==0);
	}



void TestKill()
	{
	RTestThread thread;
	TRequestStatus logonStatus;

	test.Start(_L("Test Kill"));
	thread.Create(TestThreadServerKill,(TAny*)CTestSession::ETestKill);
	thread.Logon(logonStatus);
	thread.Resume();
	User::WaitForRequest(logonStatus);
	User::SetJustInTime(ETrue);
	test(thread.ExitType()==EExitKill);
	test(logonStatus==999);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Test Terminate"));
	thread.Create(TestThreadServerKill,(TAny*)CTestSession::ETestTerminate);
	thread.Logon(logonStatus);
	thread.Resume();
	User::WaitForRequest(logonStatus);
	User::SetJustInTime(ETrue);
	test(thread.ExitType()==EExitTerminate);
	test(logonStatus==999);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Test Panic"));
	thread.Create(TestThreadServerKill,(TAny*)CTestSession::ETestPanic);
	thread.Logon(logonStatus);
	thread.Resume();
	User::WaitForRequest(logonStatus);
	User::SetJustInTime(ETrue);
	test(thread.ExitType()==EExitPanic);
	test(logonStatus==999);
	CLOSE_AND_WAIT(thread);

	test.End();
	}



void TestSetProcessPriority()
	{
	RProcess process;
	TProcessPriority priority(process.Priority());
	TInt r;

	test.Start(_L("Try changing priority when Priority Control disabled"));
	r = Session.Send(CTestSession::ETestSetProcessPriority,TIpcArgs(EPriorityBackground));
	test(r==KErrPermissionDenied);
	test(process.Priority()==priority);

	r = Session.Send(CTestSession::ETestSetProcessPriority,TIpcArgs(EPriorityForeground));
	test(r==KErrPermissionDenied);
	test(process.Priority()==priority);

	r = Session.Send(CTestSession::ETestSetProcessPriority,TIpcArgs(EPriorityLow));
	test(r==KErrPermissionDenied);
	test(process.Priority()==priority);

	r = Session.Send(CTestSession::ETestSetProcessPriority,TIpcArgs(EPriorityHigh));
	test(r==KErrPermissionDenied);
	test(process.Priority()==priority);

	test.Next(_L("Test changing priority when Priority Control enabled"));
	User::SetPriorityControl(ETrue);

	r = Session.Send(CTestSession::ETestSetProcessPriority,TIpcArgs(EPriorityBackground));
	test(r==KErrNone);
	test(process.Priority()==EPriorityBackground);

	r = Session.Send(CTestSession::ETestSetProcessPriority,TIpcArgs(EPriorityForeground));
	test(r==KErrNone);
	test(process.Priority()==EPriorityForeground);

	r = Session.Send(CTestSession::ETestSetProcessPriority,TIpcArgs(EPriorityLow));
	test(r==KErrPermissionDenied);
	test(process.Priority()==EPriorityForeground);

	r = Session.Send(CTestSession::ETestSetProcessPriority,TIpcArgs(EPriorityHigh));
	test(r==KErrPermissionDenied);
	test(process.Priority()==EPriorityForeground);

	User::SetPriorityControl(EFalse);

	process.SetPriority(priority);

	test.End();
	}



GLDEF_C TInt E32Main()
    {
	TBuf16<512> cmd;
	User::CommandLine(cmd);
	if(cmd.Length() && TChar(cmd[0]).IsDigit())
		{
		TInt function = -1;
		TInt arg1 = -1;
		TInt arg2 = -1;
		TLex lex(cmd);
		lex.Val(function);
		lex.SkipSpace();
		lex.Val(arg1);
		lex.SkipSpace();
		lex.Val(arg2);
		return DoTestProcess(function,arg1,arg2);
		}

	test.Title();

	test.Start(_L("Starting test server"));
	RTestProcess server;
	TRequestStatus rendezvous;
	TRequestStatus svrstat;
	server.Create(ETestProcessServer);
	server.NotifyDestruction(svrstat);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
	test(rendezvous==KServerRendezvous);
	server.Close();

	test.Next(_L("Openning server session"));
	TInt r = Session.Connect();
	test(r==KErrNone);

	test.Next(_L("Test IPC data transfer"));
	TestIPC();

	test.Next(_L("Test RMessagePtr2::Kill, Panic and Teminate"));
	TestKill();

	test.Next(_L("Test RMessagePtr2::SetProcessPriority"));
	TestSetProcessPriority();

	test.Next(_L("Test RMessagePtr2::Client"));
	test(Session.Send(CTestSession::ETestClient)==(TInt)RThread().Id());

	test.Next(_L("Test RMessagePtr2::RMessage2"));
	test(Session.Send(CTestSession::ETestRMessage,TIpcArgs(111111,222222,333333,444444))==KErrNone);

	test.Next(_L("Test RMessagePtr2::Handle"));
	test(Session.Send(CTestSession::ETestHandle,TIpcArgs())==KErrNone);

	test.Next(_L("Test RMessagePtr2::Session"));
	test(Session.Send(CTestSession::ETestSession,TIpcArgs())==KErrNone);

	test.Next(_L("Stopping test server"));
	Session.Send(CTestSession::EShutdown);
	Session.Close();
	User::WaitForRequest(svrstat);
	test(svrstat == KErrNone);

	test.End();
	return(0);
    }

