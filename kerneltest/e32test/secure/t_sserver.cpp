// Copyright (c) 2001-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\secure\t_sserver.cpp
// Overview:
// Test the security aspects of server connect by handle.
// API Information:
// CSession2, CServer2
// Details:
// - Test asynchronous server connect in various ways. Verify results
// are as expected.
// - Attempt to create a protected server without and without 
// KCapabilityProtServ. Verify results are as expected.
// - Attempt to connect to a serve with different Secure IDs, Vendor IDs
// and capabilities. Verify results are as expected.
// - Test creating a sub-session, sending it messages and closing it.
// Verify the results.
// - Test TIpcArgs::Set and IPC argument passing. Verify results are
// as expected.
// - Test TIpcArgs:: Also checks the integrity of the server and Kernel if client passes bad descriptor
// for IPC transfer and the integrity of the client and Kernel if the server does the same.
// - Test CServer2::RunError and CSession2::ServiceError and session
// resource counting.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include "../misc/prbs.h"

LOCAL_D RTest test(_L("T_SSERVER"));

_LIT_SECURE_ID(KTestSecureId,0x101f534d);
_LIT_SECURE_ID(KTestSecureId2,0x101f534e);
_LIT_VENDOR_ID(KTestVendorId,0x01234567);
_LIT_VENDOR_ID(KTestVendorId2,0x01234568);
const TInt KTestRunErrorModifier = 111;
const TInt KTestServiceErrorModifier = 222;
_LIT(KProtectedServerName,"!T_SSERVER-protected-server");



enum TTestProcessFunctions
	{
	ETestProcessServer,
	ETestProcessCreateProtectedServer,
	};

#include "testprocess.h"



//
// RTestThread
//

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

enum TTestSessionFuntions
	{
	ETestShutdown,EPing,
	ETestArgUnspecified,ETestArgDesC8,ETestArgDesC16,ETestArgDes8,ETestArgDes16,
	ETestResourceCountPass=0x01234567,ETestResourceCountFail,ETestServiceLeave,
	ETestCreateSubSession,ETestCloseSubSession,ETestCloseSubSessionHandle,
	ETestEchoArgs,ETestEmptySubSessionMessage, ETestBadClientDescRead, ETestBadClientDescWrite,
	ETestBadServerDescRead, ETestBadServerDescWrite
	};

class CTestSession : public CSession2
	{
public:
	CTestSession();
	virtual void ServiceL(const RMessage2& aMessage);
	virtual void ServiceError(const RMessage2& aMessage,TInt aError);
private:
	TInt CountResources();
private:
	TInt iResourceCount;
	TInt iSubSessionCloseHandle;
	RMessage2 iMessage;
	};

class CTestSecureServer : public CServer2
	{
public:
	CTestSecureServer(TInt aPriority);
	virtual CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;
private:
	void CustomL(const RMessage2& aMessage);
	void QueryL(const RMessage2& aMessage,HBufC8*& aQueryParams);
	TInt RunError(TInt aError);
private:
	friend class CTestSession;
	};

CTestSession::CTestSession()
	: CSession2()
	{}

void CTestSession::ServiceL(const RMessage2& aMessage)
	{
	TInt r = KErrNotSupported;
	TBuf8<100> buf8;
	buf8.Append(_L8("12345"));
	TBuf16<100> buf16;
	buf16.Append(_L16("12345"));
	TInt testArg = ((TUint)aMessage.Function())>>28;

	TInt badDescriptorError = PlatSec::ConfigSetting(PlatSec::EPlatSecProcessIsolation)&&PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement)
		? KErrBadDescriptor : KErrNone;

	switch(aMessage.Function()&0x0fffffff)
		{
		case ETestShutdown:
			CActiveScheduler::Stop();
			return;

		case EPing:
			r=aMessage.Int0();
			break;

		case ETestArgUnspecified:
			r = aMessage.GetDesLength(testArg);
			if(r!=KErrBadDescriptor)
				goto fail;
			r = aMessage.GetDesMaxLength(testArg);
			if(r!=KErrBadDescriptor)
				goto fail;
			r = aMessage.Read(testArg,buf8);
			if(r!=badDescriptorError)
				goto fail;
			TRAP(r,aMessage.ReadL(testArg,buf8));
			if(r!=badDescriptorError)
				goto fail;
			r = aMessage.Read(testArg,buf16);
			if(r!=badDescriptorError)
				goto fail;
			TRAP(r,aMessage.ReadL(testArg,buf16));
			if(r!=badDescriptorError)
				goto fail;
			r = aMessage.Write(testArg,buf8);
			if(r!=badDescriptorError)
				goto fail;
			TRAP(r,aMessage.WriteL(testArg,buf8));
			if(r!=badDescriptorError)
				goto fail;
			r = aMessage.Write(testArg,buf16);
			if(r!=badDescriptorError)
				goto fail;
			TRAP(r,aMessage.WriteL(testArg,buf16));
			if(r!=badDescriptorError)
				goto fail;
			goto pass;

		case ETestArgDesC8:
			r = aMessage.GetDesLength(testArg);
			if(r<0)
				goto fail;
			r = aMessage.GetDesMaxLength(testArg);
			if(r<0)
				goto fail;
			r = aMessage.Read(testArg,buf8);
			if(r!=KErrNone)
				goto fail;
			TRAP(r,aMessage.ReadL(testArg,buf8));
			if(r!=KErrNone)
				goto fail;
			r = aMessage.Read(testArg,buf16);
			if(r!=badDescriptorError)
				goto fail;
			TRAP(r,aMessage.ReadL(testArg,buf16));
			if(r!=badDescriptorError)
				goto fail;
			r = aMessage.Write(testArg,buf8);
			if(r!=badDescriptorError)
				goto fail;
			TRAP(r,aMessage.WriteL(testArg,buf8));
			if(r!=badDescriptorError)
				goto fail;
			r = aMessage.Write(testArg,buf16);
			if(r!=badDescriptorError)
				goto fail;
			TRAP(r,aMessage.WriteL(testArg,buf16));
			if(r!=badDescriptorError)
				goto fail;
			goto pass;

		case ETestArgDesC16:
			r = aMessage.GetDesLength(testArg);
			if(r<0)
				goto fail;
			r = aMessage.GetDesMaxLength(testArg);
			if(r<0)
				goto fail;
			r = aMessage.Read(testArg,buf8);
			if(r!=badDescriptorError)
				goto fail;
			TRAP(r,aMessage.ReadL(testArg,buf8));
			if(r!=badDescriptorError)
				goto fail;
			r = aMessage.Read(testArg,buf16);
			if(r!=KErrNone)
				goto fail;
			TRAP(r,aMessage.ReadL(testArg,buf16));
			if(r!=KErrNone)
				goto fail;
			r = aMessage.Write(testArg,buf8);
			if(r!=badDescriptorError)
				goto fail;
			TRAP(r,aMessage.WriteL(testArg,buf8));
			if(r!=badDescriptorError)
				goto fail;
			r = aMessage.Write(testArg,buf16);
			if(r!=badDescriptorError)
				goto fail;
			TRAP(r,aMessage.WriteL(testArg,buf16));
			if(r!=badDescriptorError)
				goto fail;
			goto pass;

		case ETestArgDes8:
			r = aMessage.GetDesLength(testArg);
			if(r<0)
				goto fail;
			r = aMessage.GetDesMaxLength(testArg);
			if(r<0)
				goto fail;
			r = aMessage.Read(testArg,buf8);
			if(r!=KErrNone)
				goto fail;
			TRAP(r,aMessage.ReadL(testArg,buf8));
			if(r!=KErrNone)
				goto fail;
			r = aMessage.Read(testArg,buf16);
			if(r!=badDescriptorError)
				goto fail;
			TRAP(r,aMessage.ReadL(testArg,buf16));
			if(r!=badDescriptorError)
				goto fail;
			r = aMessage.Write(testArg,buf8);
			if(r!=KErrNone)
				goto fail;
			TRAP(r,aMessage.WriteL(testArg,buf8));
			if(r!=KErrNone)
				goto fail;
			r = aMessage.Write(testArg,buf16);
			if(r!=badDescriptorError)
				goto fail;
			TRAP(r,aMessage.WriteL(testArg,buf16));
			if(r!=badDescriptorError)
				goto fail;
			goto pass;

		case ETestArgDes16:
			r = aMessage.GetDesLength(testArg);
			if(r<0)
				goto fail;
			r = aMessage.GetDesMaxLength(testArg);
			if(r<0)
				goto fail;
			r = aMessage.Read(testArg,buf8);
			if(r!=badDescriptorError)
				goto fail;
			TRAP(r,aMessage.ReadL(testArg,buf8));
			if(r!=badDescriptorError)
				goto fail;
			r = aMessage.Read(testArg,buf16);
			if(r!=KErrNone)
				goto fail;
			TRAP(r,aMessage.ReadL(testArg,buf16));
			if(r!=KErrNone)
				goto fail;
			r = aMessage.Write(testArg,buf8);
			if(r!=badDescriptorError)
				goto fail;
			TRAP(r,aMessage.WriteL(testArg,buf8));
			if(r!=badDescriptorError)
				goto fail;
			r = aMessage.Write(testArg,buf16);
			if(r!=KErrNone)
				goto fail;
			TRAP(r,aMessage.WriteL(testArg,buf16));
			if(r!=KErrNone)
				goto fail;
			goto pass;
		
		case ETestBadClientDescRead:
			//Testing the integrity of the server and Kernel if the client passes bad descriptor for IPC transfer
			{
			r = aMessage.GetDesLength(testArg);
			TUint8* buff;
			buff = (TUint8*)User::Alloc(r);
			if (!buff)
				{
				r=KErrNoMemory;
				break;
				}
			TPtr8 ptr8(buff, r, r);
			r=aMessage.Read(testArg,ptr8);
			User::Free(buff);
			}
			break;

		case ETestBadClientDescWrite:
			//Testing the integrity of the server and Kernel if the client passes bad descriptor for IPC transfer
			{
			r = aMessage.GetDesLength(testArg);
			TUint8* buff;
			buff = (TUint8*)User::Alloc(r);
			if (!buff)
				{
				r=KErrNoMemory;
				break;
				}
			TPtr8 ptr8(buff, r, r);
			r=aMessage.Write(testArg,ptr8);
			User::Free(buff);
			}
			break;

		case ETestBadServerDescRead:
		case ETestBadServerDescWrite:
			//Testing the integrity of the client and Kernel if server passes bad descriptor for IPC transfer
			{
			//Create a chunk with a hole between addresses 0x1000 and 0x2000
			RChunk c;
			r=c.CreateDisconnectedLocal(0,0,0x200000);
			test(r==KErrNone);
			r=c.Commit(0,0x1000);
			test(r==KErrNone);
			r=c.Commit(0x2000,0x1000);
			test(r==KErrNone);
			
			TInt base,len;
			switch(aMessage.Function()>>28)
				{ 
				case 0:base=0x1000;len=0x500;break;
				case 1:base=0x1001;len=0x500;break;
				case 2:base=0x1007;len=0x500;break;
				case 3:base=0x1ff0;len=0x100;break;
				case 4:base=0x1ff1;len=0x100;break;
				case 5:base=0x1ff2;len=0x100;break;
				case 6:base=0xff3;len=0x100;break;	
				default:base=0xfff;len=0x100;break;
				}

			TPtr8 ptr (c.Base()+base,len,len);
			if ((aMessage.Function()&0x0fffffff) == ETestBadServerDescRead)
				aMessage.Read(0,ptr);	//The server should panic here
			else
				aMessage.Write(0,ptr);	//The server should panic here
			}
			break;

		case ETestResourceCountPass:
			r=aMessage.Function();
			ResourceCountMarkStart();
			ResourceCountMarkEnd(aMessage);
			break;

		case ETestResourceCountFail:
			r=aMessage.Function();
			ResourceCountMarkStart();
			++iResourceCount;
			ResourceCountMarkEnd(aMessage);
			break;

		case ETestServiceLeave:
			iMessage = aMessage;
			User::Leave(aMessage.Int0());
			break;

		case ETestCreateSubSession:
			{
			TInt reply = aMessage.Int0();
			r = aMessage.Write(3,TPtrC8((TUint8*)&reply,sizeof(reply)));
			}
			break;

		case ETestCloseSubSession:
			iSubSessionCloseHandle = aMessage.Int3();
			r = KErrNone;
			break;

		case ETestCloseSubSessionHandle:
			r = iSubSessionCloseHandle;
			iSubSessionCloseHandle = 0;
			break;

		case ETestEchoArgs:
			{
			TInt reply[4];
			reply[0] = aMessage.Int0();
			reply[1] = aMessage.Int1();
			reply[2] = aMessage.Int2();
			reply[3] = aMessage.Int3();
			r = aMessage.Write(0,TPtrC8((TUint8*)&reply,sizeof(reply)));
			}
			break;

		case ETestEmptySubSessionMessage:
			r = aMessage.Int3();
			break;

		default:
			break;
		}
	if(aMessage.Handle())
		aMessage.Complete(r);
	return;
fail:
	aMessage.Complete(KErrGeneral);
	return;
pass:
	aMessage.Complete(KErrNone);
	return;
	}

TInt CTestSession::CountResources()
	{
	return iResourceCount;
	}

void CTestSession::ServiceError(const RMessage2& aMessage,TInt aError)
	{
	if(aMessage!=iMessage) 
		aError = KErrGeneral;   // We got given the right message
	else
		aError += KTestServiceErrorModifier;   // Let test harnes know we came through this routine
	CSession2::ServiceError(aMessage,aError);
	}

//
// CTestSecureServer
//

CTestSecureServer::CTestSecureServer(TInt aPriority)
	: CServer2(aPriority,EGlobalSharableSessions)
	{
	}

CSession2* CTestSecureServer::NewSessionL(const TVersion& aVersion, const RMessage2& /*aMessage*/) const
	{
	if(*(TInt*)&aVersion)
		User::Leave(KErrNotSupported); // Only accept version 0.0.00
	return new (ELeave) CTestSession();
	}

TInt CTestSecureServer::RunError(TInt aError)
	{
	return CServer2::RunError(aError+KTestRunErrorModifier);    // Let test harnes know we came through this routine
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

_LIT(KServerName,"T_SSERVER-server");
const TInt KServerRendezvous = KRequestPending+1;

void DoStartServer()
	{
	CTestActiveScheduler* activeScheduler = new (ELeave) CTestActiveScheduler;
	CActiveScheduler::Install(activeScheduler);
	CleanupStack::PushL(activeScheduler);

	CTestSecureServer* server = new (ELeave) CTestSecureServer(0);
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
		{
		TInt r=CreateSession(KServerName,TVersion());
		if(r) return r;
		return ShareAuto();
		}
	inline TInt Connect(const TSecurityPolicy* aPolicy)
		{
		return CreateSession(KServerName,TVersion(),-1,EIpcSession_Unsharable,aPolicy,0);
		}
	inline TInt Connect(TVersion aVersion,TRequestStatus* aStatus)
		{
		return CreateSession(KServerName,aVersion,-1,EIpcSession_Unsharable,0,aStatus);
		}
	inline TInt Send(TInt aFunction)
		{ return RSessionBase::SendReceive(aFunction); }
	inline TInt Send(TInt aFunction,const TIpcArgs& aArgs)
		{ return RSessionBase::SendReceive(aFunction,aArgs); }
	inline void Send(TInt aFunction,TRequestStatus& aStatus)
		{ RSessionBase::SendReceive(aFunction,aStatus); }
	inline void Send(TInt aFunction,const TIpcArgs& aArgs,TRequestStatus& aStatus)
		{ RSessionBase::SendReceive(aFunction,aArgs,aStatus); }
	};



//
// RTestSubSession
//

class RTestSubSession : public RSubSessionBase
	{
public:
	inline TInt CreateSubSession(RSessionBase& aSession,TInt aFunction,const TIpcArgs& aArgs)
		{ return RSubSessionBase::CreateSubSession(aSession,aFunction,aArgs); }
	inline TInt CreateSubSession(RSessionBase& aSession,TInt aFunction)
		{ return RSubSessionBase::CreateSubSession(aSession,aFunction); }
	inline void CloseSubSession(TInt aFunction)
		{ RSubSessionBase::CloseSubSession(aFunction); }
	inline TInt Send(TInt aFunction)
		{ return RSubSessionBase::SendReceive(aFunction); }
	inline TInt Send(TInt aFunction,const TIpcArgs& aArgs)
		{ return RSubSessionBase::SendReceive(aFunction,aArgs); }
	inline void Send(TInt aFunction,TRequestStatus& aStatus)
		{ RSubSessionBase::SendReceive(aFunction,aStatus); }
	inline void Send(TInt aFunction,const TIpcArgs& aArgs,TRequestStatus& aStatus)
		{ RSubSessionBase::SendReceive(aFunction,aArgs,aStatus); }
	inline TInt BlindSend(TInt aFunction)
		{ return RSubSessionBase::Send(aFunction); }
	};



RTestSession Session;
TInt TestParam = 0;

TInt TestThreadFunction(TAny* aParam)
	{
	return Session.Send((TInt)aParam,TIpcArgs(TestParam));
	}

void DoTest(TInt aFunction,TExitType aExitType,TInt aExitReason,TInt aParam=0)
	{
	TBuf<256> title;
	title.AppendFormat(_L("Function %d"),aFunction);
	test.Next(title);

	RTestThread thread;
	thread.Create(TestThreadFunction,(TAny*)aFunction);
	TRequestStatus logon;
	thread.Logon(logon);
	TestParam = aParam;

	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(logon);
	User::SetJustInTime(ETrue);

	TExitType exitType=thread.ExitType();
	TInt exitReason=thread.ExitReason();
	test(exitType==aExitType);
	test(exitReason==aExitReason);
	CLOSE_AND_WAIT(thread);
	}

//
// Anonymous server
//

class RTestAnonymousSession : public RTestSession
	{
public:
	inline TInt Connect(RServer2 aServer)
		{
		TInt r=CreateSession(aServer,TVersion());
		if(r) return r;
		return ShareProtected();
		}
	inline TInt Connect()
		{
		return CreateSession(_L(""),TVersion());
		}
	};

CTestSecureServer* AnonymousServer;

void DoStartAnonymousServer()
	{
	CTestActiveScheduler* activeScheduler = new (ELeave) CTestActiveScheduler;
	CActiveScheduler::Install(activeScheduler);
	CleanupStack::PushL(activeScheduler);

	CTestSecureServer* server = new (ELeave) CTestSecureServer(0);
	CleanupStack::PushL(server);

	User::LeaveIfError(server->Start(KNullDesC));

	AnonymousServer = server;
	RThread::Rendezvous(KServerRendezvous);

	CActiveScheduler::Start();

	CleanupStack::PopAndDestroy(2);
	}

TInt StartAnonymousServer(TAny* /*aPtr*/)
	{
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	if(!cleanupStack)
		return KErrNoMemory;
	TRAPD(leaveError,DoStartAnonymousServer())
	delete cleanupStack;
	return leaveError;
	}

void TestConnectByHandle()
	{
	RTestAnonymousSession session;
	TRequestStatus logon;
	TInt r;

	test.Start(_L("Starting a local server"));
	RTestThread thread;
	thread.Create(StartAnonymousServer);
	TRequestStatus rendezvous;
	TRequestStatus svrstat;
	thread.Rendezvous(rendezvous);
	thread.Logon(logon);
	thread.NotifyDestruction(svrstat);
	thread.Resume();
	User::WaitForRequest(rendezvous);
	test(rendezvous.Int()==KServerRendezvous);
	thread.Close();

	test.Next(_L("Check connect with null name fails"));
	test((r=session.Connect())!=KErrNone);

	test.Next(_L("Connecting to local server"));
	test((r=session.Connect(AnonymousServer->Server()))==KErrNone);

	test.Next(_L("Test the connection"));
	test((r=session.Send(EPing,TIpcArgs(1234)))==1234);
	test((r=session.Send(EPing,TIpcArgs(2345)))==2345);

	test.Next(_L("Shutting server down"));
	test(logon.Int()==KRequestPending);
	test((r=session.Send(ETestShutdown))==KErrServerTerminated);
	test(r==KErrServerTerminated);
	session.Close();
	User::WaitForRequest(logon);
	test(logon.Int()==KErrNone);
	User::WaitForRequest(svrstat);

	test.End();
	}

TRequestStatus SvrStat;

void RestartTestServer()
	{
	RTestProcess server;
	TRequestStatus rendezvous;
	server.Create(0,ETestProcessServer);
	server.NotifyDestruction(SvrStat);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
	test(rendezvous==KServerRendezvous);
	server.Close();
	test(Session.Connect()==KErrNone);
	}

void TestIpc()
	{
	TBuf8<10> buf8((TUint8*)"abcdefghij");
	TBuf16<10> buf16((TUint16*)L"abcdefghij");
	TInt r;

	test.Start(_L("Testing TIpcArgs::Set"));
	{
	TIpcArgs a(TIpcArgs::ENothing,123,(TAny*)&buf8);
	TIpcArgs b;
	b.Set(0,TIpcArgs::ENothing);
	b.Set(1,123);
	b.Set(2,(TAny*)&buf8);
	test(a.iFlags==b.iFlags);
	// iArgs[0] is uninitialised so don't test
	test(a.iArgs[1]==b.iArgs[1]);
	test(a.iArgs[2]==b.iArgs[2]);
	}
	{
	TIpcArgs a((TDesC8*)&buf8,(TDesC16*)&buf16,&buf8,&buf16);
	TIpcArgs b;
	b.Set(0,(TDesC8*)&buf8);
	b.Set(1,(TDesC16*)&buf16);
	b.Set(2,&buf8);
	b.Set(3,&buf16);
	test(a.iFlags==b.iFlags);
	test(a.iArgs[0]==b.iArgs[0]);
	test(a.iArgs[1]==b.iArgs[1]);
	test(a.iArgs[2]==b.iArgs[2]);
	test(a.iArgs[3]==b.iArgs[3]);
	}

	test.Next(_L("Test Unspecified argument"));
	r = Session.Send(ETestArgUnspecified,TIpcArgs((TAny*)&buf8));
	test(r==KErrNone);
	r = Session.Send(ETestArgUnspecified+(1<<28),TIpcArgs(0,(TAny*)&buf8));
	test(r==KErrNone);
	r = Session.Send(ETestArgUnspecified+(2<<28),TIpcArgs(0,0,(TAny*)&buf8));
	test(r==KErrNone);
	r = Session.Send(ETestArgUnspecified+(3<<28),TIpcArgs(0,0,0,(TAny*)&buf8));
	test(r==KErrNone);

	test.Next(_L("Test DesC8 argument"));
	r = Session.Send(ETestArgDesC8,TIpcArgs((TDesC8*)&buf8));
	test(r==KErrNone);
	r = Session.Send(ETestArgDesC8+(1<<28),TIpcArgs(0,(TDesC8*)&buf8));
	test(r==KErrNone);
	r = Session.Send(ETestArgDesC8+(2<<28),TIpcArgs(0,0,(TDesC8*)&buf8));
	test(r==KErrNone);
	r = Session.Send(ETestArgDesC8+(3<<28),TIpcArgs(0,0,0,(TDesC8*)&buf8));
	test(r==KErrNone);

	test.Next(_L("Test DesC16 argument"));
	r = Session.Send(ETestArgDesC16,TIpcArgs((TDesC16*)&buf16));
	test(r==KErrNone);
	r = Session.Send(ETestArgDesC16+(1<<28),TIpcArgs(0,(TDesC16*)&buf16));
	test(r==KErrNone);
	r = Session.Send(ETestArgDesC16+(2<<28),TIpcArgs(0,0,(TDesC16*)&buf16));
	test(r==KErrNone);
	r = Session.Send(ETestArgDesC16+(3<<28),TIpcArgs(0,0,0,(TDesC16*)&buf16));
	test(r==KErrNone);

	test.Next(_L("Test Des8 argument"));
	r = Session.Send(ETestArgDes8,TIpcArgs(&buf8));
	test(r==KErrNone);
	r = Session.Send(ETestArgDes8+(1<<28),TIpcArgs(0,&buf8));
	test(r==KErrNone);
	r = Session.Send(ETestArgDes8+(2<<28),TIpcArgs(0,0,&buf8));
	test(r==KErrNone);
	r = Session.Send(ETestArgDes8+(3<<28),TIpcArgs(0,0,0,&buf8));
	test(r==KErrNone);

	test.Next(_L("Test Des16 argument"));
	r = Session.Send(ETestArgDes16,TIpcArgs(&buf16));
	test(r==KErrNone);
	r = Session.Send(ETestArgDes16+(1<<28),TIpcArgs(0,&buf16));
	test(r==KErrNone);
	r = Session.Send(ETestArgDes16+(2<<28),TIpcArgs(0,0,&buf16));
	test(r==KErrNone);
	r = Session.Send(ETestArgDes16+(3<<28),TIpcArgs(0,0,0,&buf16));
	test(r==KErrNone);


	test.Next(_L("Test Bad Client Descriptor"));
	//The test should ensure that both server and kernel are safe if client passes faulty descriptor to the server.
	{
	//Create a chunk with a hole between addresses 0x1000 & 0x2000
	RChunk c;
	r=c.CreateDisconnectedLocal(0,0,0x200000);
	test(r==KErrNone);
	r=c.Commit(0,0x1000);
	test(r==KErrNone);
	r=c.Commit(0x2000,0x1000);
	test(r==KErrNone);

	//Each of these steps will pass bad descriptor to the server for IPC transfer in both directions.
	//KErrBadDescriptor should be returned.
	{
	TPtr8 ptr (c.Base()+0x1000,0x100,0x100);
	r = Session.Send(ETestBadClientDescRead,TIpcArgs(&ptr));
	test(KErrBadDescriptor == r);
	r = Session.Send(ETestBadClientDescWrite,TIpcArgs(&ptr));
	test(KErrBadDescriptor == r);
	}
	{
	TPtr8 ptr (c.Base()+0x1003,0x100,0x100);
	r = Session.Send(ETestBadClientDescRead,TIpcArgs(&ptr));
	test(KErrBadDescriptor == r);
	r = Session.Send(ETestBadClientDescWrite,TIpcArgs(&ptr));
	test(KErrBadDescriptor == r);
	}
	{
	TPtr8 ptr (c.Base()+0xe00,0x500,0x500);
	r = Session.Send(ETestBadClientDescRead,TIpcArgs(&ptr));
	test(KErrBadDescriptor == r);
	r = Session.Send(ETestBadClientDescWrite,TIpcArgs(&ptr));
	test(KErrBadDescriptor == r);
	}
	{
	TPtr8 ptr (c.Base()+0xdff,0x500,0x500);
	r = Session.Send(ETestBadClientDescRead,TIpcArgs(&ptr));
	test(KErrBadDescriptor == r);
	r = Session.Send(ETestBadClientDescWrite,TIpcArgs(&ptr));
	test(KErrBadDescriptor == r);
	}
	{
	TPtr8 ptr (c.Base()+0x1ff1,0x500,0x500);
	r = Session.Send(ETestBadClientDescRead,TIpcArgs(&ptr));
	test(KErrBadDescriptor == r);
	r = Session.Send(ETestBadClientDescWrite,TIpcArgs(&ptr));
	test(KErrBadDescriptor == r);
	}

	//The next step will send random descriptors for IPC transfer. Server should return either KErrNone or
	//KErrBadDescriptor
	{	
	TUint seed[2];
	seed[0]=User::TickCount();
	seed[1]=0;
	test.Printf(_L("The initial seed for the random function is:  S0=%xh S1=%xh\n"), seed[0], seed[1]);

	TInt i;
	TPtr8 ptr (0,0,0);
	TInt noErrors = 0;
	TInt badDescriptors = 0;

	for (i=0;i<100;i++)
		{
		TInt descAddress = (TInt) (c.Base() + (Random(seed) % 0x1000));
		TInt descSize=Random(seed)%0x1000;

		TUint* descData = (TUint*)&ptr;
		descData[0] = 0x20000000 + descSize;
		descData[1] = descSize;
		descData[2] = descAddress;

		r = Session.Send(ETestBadClientDescRead,TIpcArgs(&ptr));
		switch (r)
			{
			case KErrNone:			noErrors++;break;
			case KErrBadDescriptor: badDescriptors++;break;
			default:				test.Printf(_L("Error: %d returned"),r);
			}

		r = Session.Send(ETestBadClientDescWrite,TIpcArgs(&ptr));
		switch (r)
			{
			case KErrNone:			noErrors++;break;
			case KErrBadDescriptor: badDescriptors++;break;
			default:				test.Printf(_L("Error: %d returned"),r);
			}
		}
	test.Printf(_L("KErrNoError: %d  KErrBadDescriptor: %d"),noErrors, badDescriptors);
	}

	test.Next(_L("Test Bad Server Descriptor"));
	//The test should ensure that kernel is safe if server passes faulty descriptor for IPC transfer
	{
	TPtr8 ptr (c.Base(),0x1000,0x1000);
	TInt i;
	for (i=0;i<=7;i++)
		{
		r = Session.Send(ETestBadServerDescRead | (i<<28),TIpcArgs(&ptr));
		test(r==KErrServerTerminated);
		RestartTestServer();
		r = Session.Send(ETestBadServerDescWrite | (i<<28),TIpcArgs(&ptr));
		test(r==KErrServerTerminated);
		User::WaitForRequest(SvrStat);
		RestartTestServer();
		}
	}

	c.Close();
	}
	test.End();
	}



void TestSubSessions()
	{
	const TInt KSubSessionHandle = 0x87654321;
	RTestSubSession sub;
	TInt r;

	test.Start(_L("Creating a subsession with no arguments"));
	r = sub.CreateSubSession(Session,ETestCreateSubSession);	
	test(r==KErrNone);
	test(((TInt*)&sub)[0] == (*(TInt*)&Session | CObjectIx::ENoClose));  // check sub.iSession

	test.Next(_L("Creating a subsession with arguments"));
	r = sub.CreateSubSession(Session,ETestCreateSubSession,TIpcArgs(KSubSessionHandle));	
	test(r==KErrNone);
	test(((TInt*)&sub)[0] == (*(TInt*)&Session | CObjectIx::ENoClose));  // check sub.iSession

	test(((TInt*)&sub)[1]==KSubSessionHandle);   // check sub.iSubSessionHandle

	test.Next(_L("Sending message with arguments"));
	TInt reply[4] = {0};
	TPtr8 replyDes((TUint8*)&reply,sizeof(reply));
	r = sub.Send(ETestEchoArgs,TIpcArgs(&replyDes,123,456,789));
	test(r==KErrNone);
	test(reply[0]==(TInt)&replyDes);
	test(reply[1]==123);
	test(reply[2]==456);
	test(reply[3]==KSubSessionHandle);

	test.Next(_L("Sending empty message"));
	r = sub.Send(ETestEmptySubSessionMessage);
	test(r==KSubSessionHandle);

	test.Next(_L("Sending empty message blindly"));
	r = sub.BlindSend(ETestEmptySubSessionMessage);
	test(r==KErrNone);

	test.Next(_L("Closing subsession"));
	sub.CloseSubSession(ETestCloseSubSession);	
	test(((TInt*)&sub)[0]==0);  // check sub.iSession
	test(((TInt*)&sub)[1]==0);   // check sub.iSubSessionHandle
	r = Session.Send(ETestCloseSubSessionHandle);
	test(r==KSubSessionHandle);

	test.End();
	}


void TestProtectedServers()
	{
	RTestProcess process;
	TRequestStatus logonStatus;
	
	test.Start(_L("Trying to create a protected server without KCapabilityProtServ"));
	process.Create(~(1u<<ECapabilityProtServ),ETestProcessCreateProtectedServer);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	if(PlatSec::IsCapabilityEnforced(ECapabilityProtServ))
		{
		test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
		test(logonStatus==EPlatformSecurityTrap);
		}
	else
		{
		test(process.ExitType()==EExitKill);
		test(logonStatus==KErrNone);
		}
	CLOSE_AND_WAIT(process);

	test.Next(_L("Trying to create a protected server with KCapabilityProtServ"));
	process.Create(1<<ECapabilityProtServ,ETestProcessCreateProtectedServer);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==KErrNone);
	CLOSE_AND_WAIT(process);

	test.End();
	}


void TestIdentifiedServers()
	{
	RTestSession session;
	TInt r;
	
	test.Start(_L("Trying to connect to a server with wrong Secure ID"));
	_LIT_SECURITY_POLICY_S0(wrongSid,KTestSecureId2);
	r=session.Connect(&wrongSid);
	test(r==(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement)?KErrPermissionDenied:KErrNone));
	session.Close();

	test.Next(_L("Test connecting to a server with correct Secure ID"));
	_LIT_SECURITY_POLICY_S0(correctSid,KTestSecureId);
	r=session.Connect(&correctSid);
	test(r==KErrNone);
	session.Close();

	test.Next(_L("Trying to connect to a server with wrong Vendor ID"));
	_LIT_SECURITY_POLICY_V0(wrongVid,KTestVendorId2);
	r=session.Connect(&wrongVid);
	test(r==(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement)?KErrPermissionDenied:KErrNone));
	session.Close();

	test.Next(_L("Test connecting to a server with correct Vendor ID"));
	_LIT_SECURITY_POLICY_V0(correctVid,KTestVendorId);
	r=session.Connect(&correctVid);
	test(r==KErrNone);
	session.Close();

	test.Next(_L("Trying to connect to a server with wrong capabilities"));
	_LIT_SECURITY_POLICY_C1(wrongCaps,ECapabilityReadUserData);
	r=session.Connect(&wrongCaps);
	test(r==(PlatSec::IsCapabilityEnforced(ECapabilityReadUserData)?KErrPermissionDenied:KErrNone));
	session.Close();

	test.Next(_L("Test connecting to a server with correct capabilities"));
	_LIT_SECURITY_POLICY_C1(correctCaps,ECapability_None);
	r=session.Connect(&correctCaps);
	test(r==KErrNone);
	session.Close();

	test.Next(_L("Test connecting to a server without specifying a policy"));
	r=session.Connect(0);
	test(r==KErrNone);
	session.Close();

	test.End();
	}



#include <e32panic.h>

TInt TestThreadAsyncConnect(TAny*)
	{
	RTestSession session;
	TRequestStatus status;
	TInt r=session.Connect(TVersion(1,1,1),&status);
	if(r!=KErrNone)
		return r;
	if(status!=KRequestPending)   // server can't have created session yet because we have higher priority
		return status.Int();
	User::WaitForRequest(status);
	if(status!=KErrNotSupported)
		return status.Int();
	return session.Send(EPing,TIpcArgs(1234));   // Should panic us
	}

void TestAsynchronousConnect()
	{
	RTestSession session;
	TRequestStatus status;
	TInt r;
	
	test.Start(_L("Test successful asynchronous connect"));
	r=session.Connect(TVersion(0,0,0),&status);
	test(r==KErrNone);
	test(status==KRequestPending);   // server can't have created session yet because we have higher priority
	User::WaitForRequest(status);
	test(status==KErrNone);
	test((r=session.Send(EPing,TIpcArgs(1234)))==1234);
	session.Close();

	test.Next(_L("Test unsuccessful asynchronous connect"));
	r=session.Connect(TVersion(1,1,1),&status);
	test(r==KErrNone);
	test(status==KRequestPending);   // server can't have created session yet because we have higher priority
	User::WaitForRequest(status);
	test(status==KErrNotSupported);
	session.Close();

	test.Next(_L("Test using unsuccessful asynchronous connect"));
	RTestThread thread;
	thread.Create(TestThreadAsyncConnect,0);
	TRequestStatus logon;
	thread.Logon(logon);
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(logon);
	User::SetJustInTime(ETrue);
	TExitType exitType=thread.ExitType();
	TInt exitReason=thread.ExitReason();
	test(exitType==EExitPanic);
	test(exitReason==CServer2::ESessionNotConnected);
	thread.Close();

	test.End();
	}



TInt DoTestProcess(TInt aTestNum,TInt aArg1,TInt aArg2)
	{
	(void)aArg1;
	(void)aArg2;
	switch(aTestNum)
		{

	case ETestProcessServer:
		RProcess().SetPriority(EPriorityLow);
		return StartServer();

	case ETestProcessCreateProtectedServer:
		{
		test.Title();
		RServer2 server;
		TInt r = server.CreateGlobal(KProtectedServerName);
		server.Close();
		return r;
		}

	default:
		User::Panic(_L("T_SSERVER"),1);
		}

	return KErrNone;
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

	TInt r;

	test.Title();

	test.Start(_L("Testing Server Connect by Handle"));
	TestConnectByHandle();

	test.Next(_L("Starting test server"));
	RTestProcess server;
	TRequestStatus rendezvous;
	server.Create(0,ETestProcessServer);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
	test(rendezvous==KServerRendezvous);
	server.Close();

	test.Next(_L("Conecting to test server"));
	test((r=Session.Connect())==KErrNone);

	test.Next(_L("Test asynchronous server connect"));
	TestAsynchronousConnect();

	test.Next(_L("Test protected servers"));
	TestProtectedServers();

	test.Next(_L("Test connecting to identified servers"));
	TestIdentifiedServers();

	test.Next(_L("Testing SubSessions"));
	TestSubSessions();

	test.Next(_L("Testing IPC argument checking"));
	TestIpc();

	test.Next(_L("Testing CServer2::RunError and CSession2::ServiceError()"));
	const TInt KTestServiceLeaveValue = 555;
	r = Session.Send(ETestServiceLeave,TIpcArgs(KTestServiceLeaveValue));
	test(r==KTestServiceLeaveValue+KTestRunErrorModifier+KTestServiceErrorModifier);

	test.Next(_L("Testing session resource counting"));
	test.Start(_L(""));
	DoTest(ETestResourceCountPass,EExitKill,ETestResourceCountPass);
	DoTest(ETestResourceCountFail,EExitPanic,CSession2::ESesFoundResCountHeaven);
	test.End();

	test.Next(_L("Shutting server down"));
	{
	Session.Send(ETestShutdown,TIpcArgs());
	Session.Close();
	}
	User::WaitForRequest(SvrStat);

	test.End();
	return(0);
    }

