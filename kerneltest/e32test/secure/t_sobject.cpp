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
// e32test\secure\t_sobject.cpp
// Overview:
// Test the security aspects of the TFind??? (TFindChunk etc.) classes.
// API Information:
// TFind??? (TFindChunk, TFindThread etc.)
// Details:
// - Test TFindPhysicalDevices and attempt to open the found device. 
// Verify results are as expected.
// - Test TFindLogicalDevices and attempt to open the found device. 
// Duplicate the object in another thread. Verify results are as expected.
// - Test TFindLibrary and attempt to open the found device. Verify results 
// are as expected.
// - Test TFindServer and attempt to open the found server. Test duplication 
// of a named or unnamed server. Verify results are as expected.
// - Test TFindProcess and attempt to open the found process. Test duplication 
// of the object. Attempt to open the process by name and by id. Verify 
// results are as expected.
// - Test TFindThread and attempt to open the found thread. Test duplication 
// of the object. Attempt to open the thread in various ways. Verify 
// results are as expected.
// - Test TFindChunk and attempt to open the found object. Test duplication 
// of the object. Attempt to open the object in various ways. Verify 
// results are as expected.
// - Test TFindSemaphore and attempt to open the found object. Attempt to 
// open the object in various ways. Test duplication of the object. Verify 
// results are as expected.
// - Test TFindMutex and attempt to open the found object. Attempt to 
// open the object in various ways. Test duplication of the object. Verify 
// results are as expected.
// - Create some RMsgQueue objects. Attempt to open the objects in various ways. 
// Test duplication of the objects. Verify results are as expected.
// - Create some RCondVar objects. Attempt to open the objects in various ways. 
// Test duplication of the objects. Verify results are as expected.
// - Test passing a handle via IPC: test sending LogicalChannel, Chunk, 
// Semaphore, Mutex, MsgQueue, CondVar and Session handles. Verify results
// are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32msgqueue.h>
#include <f32file.h>
#include "d_sldd.h"

LOCAL_D RTest test(_L("T_SOBJECT"));

TInt PlatSecProcessIsolationError = 1;
TInt PlatSecFindError = 1;

_LIT(KTestMutexName,"T_SOBJECT-test-mutex");
_LIT(KTestSemaphoreName,"T_SOBJECT-test-semaphore");
_LIT(KTestChunkName,"T_SOBJECT-test-chunk");
_LIT(KTestMsgQueueName,"T_SOBJECT-test-msgqueue");
_LIT(KTestCondVarName,"T_SOBJECT-test-condvar");

TFullName Name;
TFullName Name2;

void SetName(RHandleBase a)
	{
	Name = a.FullName();
	}

TBool CheckName(RHandleBase a)
	{
	Name2 = a.FullName();
	return Name==Name2;
	}

class RTestHandle : public RHandleBase
	{
public:
	inline TInt Open(const TFindHandleBase& aHandle,TOwnerType aType=EOwnerThread)
		{ return RHandleBase::Open(aHandle,aType); }
	};



enum TTestProcessFunctions
	{
	ETestProcessServer,
	ETestProcessDuplicate,
	ETestProcessOpenThreadById,
	};

#include "testprocess.h"


TThreadId MainThreadId;

TInt TestThreadDuplicate(TAny* aArg)
	{
	RHandleBase handle;
	handle.SetHandle((TInt)aArg);
	RThread thread;
	TInt r = thread.Open(MainThreadId);
	if(r!=KErrNone)
		r = 999;
	else
		r = handle.Duplicate(thread);
	thread.Close();
	return r;
	}

TInt DuplicateInOtherThread(RHandleBase aHandle)
	{
	RThread thread;
	TRequestStatus logonStatus;
	thread.Create(_L(""),TestThreadDuplicate,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,(TAny*)aHandle.Handle());
	MainThreadId = RThread().Id();
	thread.Logon(logonStatus);
	thread.Resume();
	User::WaitForRequest(logonStatus);
	test(thread.ExitType()==EExitKill);
	CLOSE_AND_WAIT(thread);
	return logonStatus.Int();
	}

TInt DuplicateInOtherProcess(RHandleBase aHandle)
	{
	RTestProcess process;
	TRequestStatus logonStatus;
	process.Create(ETestProcessDuplicate,RThread().Id(),aHandle.Handle());
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	CLOSE_AND_WAIT(process);
	return logonStatus.Int();
	}

TInt OpenThreadByIdInOtherProcess(TThreadId aId)
	{
	RTestProcess process;
	TRequestStatus logonStatus;
	process.Create(ETestProcessOpenThreadById,aId);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	CLOSE_AND_WAIT(process);
	return logonStatus.Int();
	}



//
// RTestSession
//

enum TServerName
	{
	EMainServer,EGlobalSharableServer,EAnonymousServer,ENumServerTypes
	};

_LIT(KServerName,"T_SOBJECT-MainServer");
_LIT(KServerName2,"T_SOBJECT-GlobalSharableServer");

inline const TDesC& ServerName(TServerName aName)
	{
	switch(aName)
		{
	case EMainServer:
		return KServerName;
	case EGlobalSharableServer:
		return KServerName2;
	default:
		return KNullDesC;
		}
	}

class RTestSession : public RSessionBase
	{
public:
	inline TInt Connect(TServerName aName=EMainServer)
		{
		TInt r=CreateSession(ServerName(aName),TVersion());
		if(r) return r;
		return ShareAuto();
		}
	inline TInt ConnectProtected(TServerName aName=EMainServer)
		{
		TInt r=CreateSession(ServerName(aName),TVersion());
		if(r) return r;
		return ShareProtected();
		}
	inline TInt Open(RMessagePtr2 aMessage,TInt aParam,TOwnerType aType=EOwnerProcess)
		{ return RSessionBase::Open(aMessage,aParam,aType); }
	inline TInt Open(RMessagePtr2 aMessage,TInt aParam,const TSecurityPolicy& aServerPolicy,TOwnerType aType=EOwnerProcess)
		{ return RSessionBase::Open(aMessage,aParam,aServerPolicy,aType); }
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
// CTestSession
//

class CTestSession : public CSession2
	{
public:
	enum {EShutdown,EPing,ETestMutex,ETestSemaphore,ETestMsgQueue,ETestCondVar,ETestChunk,ETestChunkAdjust,ETestLdd,
		ETestSession,ETestSession2,ETestSession3,ETestServerDuplicateInThread,ETestServerDuplicateInProcess};
public:
	CTestSession();
	virtual void ServiceL(const RMessage2& aMessage);
public:
	};

CTestSession::CTestSession()
	: CSession2()
	{
	}


void CTestSession::ServiceL(const RMessage2& aMessage)
	{
	RMessagePtr2 m(aMessage);
	TFullName name;
	TInt r = KErrGeneral;

	switch (aMessage.Function())
		{
		case CTestSession::EShutdown:
			CActiveScheduler::Stop();
			break;

		case CTestSession::EPing:
			r=aMessage.Int0();
			break;

		case CTestSession::ETestMutex:
			{
			RMutex object;

			r = object.Open(m,0,EOwnerThread);
			if(r!=KErrBadHandle || object.Handle())
				goto fail;

			r = object.Open(m,1,EOwnerProcess);
			if(r!=KErrNone)
				break;
			name = object.FullName();
			object.Close();
			if(name!=KTestMutexName)
				goto fail;

			r = object.Open(m,2,EOwnerThread);
			if(r!=KErrNone)
				break;
			SetName(object);
			m.Read(3,Name2);
			if(Name!=Name2)
				goto fail;

			m.Complete(object);
			object.Close();
			return;
			}

		case CTestSession::ETestSemaphore:
			{
			RSemaphore object;

			r = object.Open(m,0,EOwnerThread);
			if(r!=KErrBadHandle || object.Handle())
				goto fail;

			r = object.Open(m,1,EOwnerProcess);
			if(r!=KErrNone)
				break;
			name = object.FullName();
			object.Close();
			if(name!=KTestSemaphoreName)
				goto fail;

			r = object.Open(m,2,EOwnerThread);
			if(r!=KErrNone)
				break;
			SetName(object);
			m.Read(3,Name2);
			if(Name!=Name2)
				goto fail;

			m.Complete(object);
			object.Close();
			return;
			}

		case CTestSession::ETestMsgQueue:
			{
			RMsgQueue<TInt> object;

			r = object.Open(m,0,EOwnerThread);
			if(r!=KErrBadHandle || object.Handle())
				goto fail;

			r = object.Open(m,1,EOwnerProcess);
			if(r!=KErrNone)
				break;
			name = object.FullName();
			object.Close();
			if(name!=KTestMsgQueueName)
				goto fail;

			r = object.Open(m,2,EOwnerThread);
			if(r!=KErrNone)
				break;
			SetName(object);
			m.Read(3,Name2);
			if(Name!=Name2)
				goto fail;

			m.Complete(object);
			object.Close();
			return;
			}


		case CTestSession::ETestCondVar:
			{
			RCondVar object;

			r = object.Open(m,0,EOwnerThread);
			if(r!=KErrBadHandle || object.Handle())
				goto fail;

			r = object.Open(m,1,EOwnerProcess);
			if(r!=KErrNone)
				break;
			name = object.FullName();
			object.Close();
			if(name!=KTestCondVarName)
				goto fail;

			r = object.Open(m,2,EOwnerThread);
			if(r!=KErrNone)
				break;
			SetName(object);
			m.Read(3,Name2);
			if(Name!=Name2)
				goto fail;

			m.Complete(object);
			object.Close();
			return;
			}


		case CTestSession::ETestChunk:
			{
			RChunk object;

			r = object.Open(m,0,EOwnerThread);
			if(r!=KErrBadHandle || object.Handle())
				goto fail;

			r = object.Open(m,1,EOwnerProcess);
			if(r!=KErrNone)
				break;
			name = object.FullName();
			object.Close();
			if(name!=KTestChunkName)
				goto fail;

			r = object.Open(m,2,EOwnerThread);
			if(r!=KErrNone)
				break;
			SetName(object);
			m.Read(3,Name2);
			if(Name!=Name2)
				goto fail;

			m.Complete(object);
			object.Close();
			return;
			}


		case CTestSession::ETestChunkAdjust:
			{
			RChunk object;
			r = object.Open(m,0,EOwnerThread);
			if(r==KErrNone)
				r = object.Adjust(0x1000);
			object.Close();
			}
			break;

		case CTestSession::ETestLdd:
			{
			RLddTest object;
			r = object.Open(m,0);
			if(r!=KErrBadHandle || object.Handle())
				goto fail;
			r = object.Open(m,1);
			if(r!=KErrNone)
				break;
			SetName(object);
			m.Read(3,Name2);
			if(Name!=Name2)
				goto fail;
			r = object.Test1();
			if(r!=aMessage.Int2())
				goto fail;
			m.Complete(object);
			object.Close();
			return;
			}

		case CTestSession::ETestSession:
			{
			RTestSession object;
			r = object.Open(m,0);
			if(r!=KErrBadHandle || object.Handle())
				goto fail;
			r = object.Open(m,1,TSecurityPolicy(ECapabilityTCB));
			if(r!=KErrPermissionDenied || object.Handle())
				goto fail;
			r = object.Open(m,1);
			if(r!=KErrNone)
				break;
			SetName(object);
			m.Read(3,Name2);
			if(Name!=Name2)
				goto fail;

			RFs fs;
			r = fs.Open(m,2,TSecurityPolicy(ECapabilityTCB));
			if(r!=KErrNone)
				goto fail;
			fs.Close();

			r=object.Send(CTestSession::EPing,TIpcArgs(234));
			if(r!=234)
				goto fail;

			m.Complete(object);
			object.Close();
			return;
			}

		case CTestSession::ETestSession2:
			{
			RTestSession object;
			object.Connect(EGlobalSharableServer);
			m.Complete(object);
			object.Close();
			return;
			}

		case CTestSession::ETestSession3:
			{
			RFs object;
			object.Connect();
			object.ShareProtected();
			m.Complete(object);
			object.Close();
			return;
			}

		case CTestSession::ETestServerDuplicateInThread:
			r = DuplicateInOtherThread(Server()->Server());
			break;

		case CTestSession::ETestServerDuplicateInProcess:
			r = DuplicateInOtherProcess(Server()->Server());
			break;

		default:
			m.Complete(KErrNotSupported);
			break;
		}
	m.Complete(r);
	return;
fail:
	m.Complete(KErrGeneral);
	return;
	}

RTestSession Session;



//
// CTestServer
//

class CTestServer : public CServer2
	{
public:
	CTestServer(TInt aPriority,TInt aType=ESharableSessions);
	virtual CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const;
	};

CTestServer::CTestServer(TInt aPriority,TInt aType)
	: CServer2(aPriority,(TServerType)aType)
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

const TInt KServerRendezvous = KRequestPending+1;

RServer2 Servers[ENumServerTypes];

void DoStartServer(TServerName aName)
	{
	CTestActiveScheduler* activeScheduler = new (ELeave) CTestActiveScheduler;
	CActiveScheduler::Install(activeScheduler);
	CleanupStack::PushL(activeScheduler);

	TInt type = 1; // ESharableSessions
	if(aName==EGlobalSharableServer)
		type = 2; // EGlobalSharableSessions;
	CTestServer* server = new (ELeave) CTestServer(0,type);
	CleanupStack::PushL(server);

	User::LeaveIfError(server->Start(ServerName(aName)));

	Servers[aName] = server->Server();
	RProcess::Rendezvous(KServerRendezvous);
	RThread::Rendezvous(KServerRendezvous);

	CActiveScheduler::Start();

	Servers[aName].SetHandle(0);
	CleanupStack::PopAndDestroy(2);
	}

TInt StartServer(TServerName aName)
	{
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	if(!cleanupStack)
		return KErrNoMemory;
	TRAPD(leaveError,DoStartServer(aName))
	delete cleanupStack;
	return leaveError;
	}

TInt DoThreadStartServer(TAny* aArg)
	{
	return StartServer((TServerName)(TInt)aArg);
	}

TRequestStatus ThrdSvrStat;

TInt StartServerInThread(TServerName aName)
	{
	RThread thread;
	TRequestStatus rendezvousStatus;
	thread.Create(_L(""),DoThreadStartServer,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,(TAny*)aName);
	thread.NotifyDestruction(ThrdSvrStat);
	thread.Rendezvous(rendezvousStatus);
	thread.Resume();
	User::WaitForRequest(rendezvousStatus);
	thread.Close();
	if(rendezvousStatus.Int()!=KServerRendezvous)
		return KErrGeneral;
	return KErrNone;
	}



void TestPhysicalDevices()
	{
	TFullName name;
	test.Start(_L("Test find named object"));
	TFindPhysicalDevice find(_L("*"));
	test((find.Next(name))==KErrNone);

	test.Next(_L("Try open found object"));
	RTestHandle testObject;
	test((testObject.Open(find))==PlatSecProcessIsolationError);
	testObject.Close();

	test.End();
	}



void TestLogicalDevices()
	{
	TFullName name;	
	RDevice device;

	test.Start(_L("Test find named object"));
	TFindLogicalDevice find(_L("*"));
	test((find.Next(name))==KErrNone);

	test.Next(_L("Test open found object"));
	test((device.Open(find))==KErrNone);

	test.Next(_L("Test duplicate object in other thread"));
	test((DuplicateInOtherThread(device))==KErrNone);

	test.Next(_L("Test duplicate object in other process"));
	test((DuplicateInOtherProcess(device))==KErrNone);

	device.Close();

	test.Next(_L("Test open device by name"));
	test((device.Open(name))==KErrNone);
	device.Close();

	test.End();
	}



void TestLibraries()
	{
	TFullName name;
	test.Start(_L("Test find named object"));
	TFindLibrary find(_L("*"));
	test((find.Next(name))==KErrNone);

	test.Next(_L("Try open found object"));
	RTestHandle testObject;
	test((testObject.Open(find))==PlatSecProcessIsolationError);
	testObject.Close();

	test.End();
	}



void TestServers()
	{
	TFullName name;
	RServer2 localObject(Servers[EAnonymousServer]);

	test.Start(_L("Test find named object"));
	TFindServer find(ServerName(EMainServer));
	test((find.Next(name))==KErrNone);

	test.Next(_L("Try open found object"));
	RTestHandle testObject;
	test((testObject.Open(find))==KErrPermissionDenied);

	test.Next(_L("Test duplicate named server in other thread"));
	test((Session.Send(CTestSession::ETestServerDuplicateInThread))==KErrNone);

	test.Next(_L("Try duplicate named server in other process"));
	test((Session.Send(CTestSession::ETestServerDuplicateInProcess))==KErrPermissionDenied);

	test.Next(_L("Test duplicate unnamed server in other thread"));
	test((DuplicateInOtherThread(localObject))==KErrNone);

	test.Next(_L("Try duplicate unnamed server in other process"));
	test((DuplicateInOtherProcess(localObject))==KErrPermissionDenied);

	test.End();
	}



void TestProcesses()
	{
	TFullName name;	
	RProcess process;

	test.Start(_L("Test find named object"));
	TFindProcess find(_L("EKern*"));
	test((find.Next(name))==KErrNone);

	test.Next(_L("Test open found object"));
	test((process.Open(find))==KErrNone);

	test.Next(_L("Test duplicate object in other thread"));
	test((DuplicateInOtherThread(process))==KErrNone);

	test.Next(_L("Test duplicate object in other process"));
	test((DuplicateInOtherProcess(process))==KErrNone);

	process.Close();

	test.Next(_L("Test open process by name"));
	test((process.Open(name))==KErrNone);
	TProcessId id=process.Id();
	process.Close();

	test.Next(_L("Test open process by id"));
	test((process.Open(id))==KErrNone);
	test(name==process.FullName());
	process.Close();

	test.End();
	}



void TestThreads()
	{
	TFullName name;
	
	test.Start(_L("Creating threads"));
	RThread globalObject;
	RThread localObject;
	RThread testObject;
	test((globalObject.Create(_L("T_SOBJECT-test-global-thread"),TestThreadDuplicate,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,NULL))==KErrNone);
	test((localObject.Create(_L(""),TestThreadDuplicate,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,NULL))==KErrNone);

	test.Next(_L("Test find named thread"));
	TFindThread find(globalObject.FullName());
	test((find.Next(name))==KErrNone);

	test.Next(_L("Test open found object"));
	test((testObject.Open(find))==KErrNone);
	testObject.Close();

	test.Next(_L("Check can't find unnamed thread"));
	TName objectName(localObject.FullName());
	find.Find(objectName);
	test((find.Next(name))==PlatSecFindError);

	test.Next(_L("Test open named thread by name"));
	test((testObject.Open(globalObject.FullName()))==KErrNone);
	testObject.Close();

	test.Next(_L("Check can't open unnamed thread by name"));
	test((testObject.Open(localObject.FullName()))==PlatSecFindError);
	testObject.Close();

	test.Next(_L("Check can't open with no name"));
	test((testObject.Open(KNullDesC))==KErrNotFound);
	testObject.Close();

	test.Next(_L("Test open named thread by id (in same process)"));
	test((testObject.Open(globalObject.Id()))==KErrNone);
	testObject.Close();

	test.Next(_L("Test open named thread by id (in other process)"));
	test((OpenThreadByIdInOtherProcess(globalObject.Id()))==KErrNone);

	test.Next(_L("Test open unnamed thread by id (in same process)"));
	test((testObject.Open(localObject.Id()))==KErrNone);

	test.Next(_L("Check can't open unnamed thread by id (in other process)"));
	test((OpenThreadByIdInOtherProcess(localObject.Id()))==PlatSecProcessIsolationError);

	test.Next(_L("Test duplicate named thread in other process"));
	test((DuplicateInOtherProcess(globalObject))==KErrNone);

	test.Next(_L("Check can't duplicate unnamed thread in other process"));
	test((DuplicateInOtherProcess(localObject))==PlatSecProcessIsolationError);

	test.Next(_L("Test duplicate named thread in other thread"));
	test((DuplicateInOtherThread(globalObject))==KErrNone);

	test.Next(_L("Test duplicate unnamed thread in other thead"));
	test((DuplicateInOtherThread(localObject))==KErrNone);

	test.Next(_L("Closing threads"));
	globalObject.Close();
	localObject.Close();

	test.End();
	}



void TestChunks()
	{
	TFullName name;
	
	test.Start(_L("Creating chunks"));
	RChunk globalObject;
	RChunk localObject;
	RChunk testObject;
	test((globalObject.CreateGlobal(_L("T_SOBJECT-test-global-chunk"),4096,1024*1024))==KErrNone);
	test((localObject.CreateLocal(4096,1024*1024))==KErrNone);

	test.Next(_L("Test find global object"));
	TFindChunk find(globalObject.FullName());
	test((find.Next(name))==KErrNone);

	test.Next(_L("Test open found object"));
	test((testObject.Open(find))==KErrNone);
	testObject.Close();

	test.Next(_L("Check can't find local object"));
	TName objectName(localObject.FullName());
	find.Find(objectName);
	test((find.Next(name))==PlatSecFindError);

	test.Next(_L("Test open with null name"));
	test((testObject.OpenGlobal(KNullDesC,ETrue))==KErrNotFound);
	testObject.Close();

	test.Next(_L("Test open global object by name"));
	test((testObject.OpenGlobal(globalObject.FullName(),ETrue))==KErrNone);
	testObject.Close();

	test.Next(_L("Check can't open local object by name"));
	test((testObject.OpenGlobal(localObject.FullName(),ETrue))==PlatSecFindError);
	testObject.Close();

	test.Next(_L("Test duplicate global object in other process"));
	test((DuplicateInOtherProcess(globalObject))==KErrNone);

	test.Next(_L("Check can't duplicate local object in other process"));
	test((DuplicateInOtherProcess(localObject))==PlatSecProcessIsolationError);

	test.Next(_L("Test duplicate global object in other thread"));
	test((DuplicateInOtherThread(globalObject))==KErrNone);

	test.Next(_L("Test duplicate local object in other thead"));
	test((DuplicateInOtherThread(localObject))==KErrNone);

	test.Next(_L("Test Chunk protection"));
	{
	RChunk protectedChunk;
	test((protectedChunk.CreateGlobal(KNullDesC,0x1000,0x100000,EOwnerProcess))==KErrNone);
	test((Session.Send(CTestSession::ETestChunkAdjust,TIpcArgs(protectedChunk)))==KErrNone);
	protectedChunk.SetRestrictions(RChunk::EPreventAdjust);
	test((Session.Send(CTestSession::ETestChunkAdjust,TIpcArgs(protectedChunk)))==KErrAccessDenied);
	protectedChunk.Close();
	}

	test.Next(_L("Closing chunks"));
	globalObject.Close();
	localObject.Close();

	test.End();
	}



void TestSemaphores()
	{
	TFullName name;
	
	test.Start(_L("Creating semaphores"));
	RSemaphore globalObject;
	RSemaphore localObject;
	RSemaphore testObject;
	test((globalObject.CreateGlobal(_L("T_SOBJECT-test-global-semaphore"),1))==KErrNone);
	test((localObject.CreateLocal(1))==KErrNone);

	test.Next(_L("Test find global object"));
	TFindSemaphore find(globalObject.FullName());
	test((find.Next(name))==KErrNone);

	test.Next(_L("Test open found object"));
	test((testObject.Open(find))==KErrNone);
	testObject.Close();

	test.Next(_L("Check can't find local object"));
	TName objectName(localObject.FullName());
	find.Find(objectName);
	test((find.Next(name))==PlatSecFindError);

	test.Next(_L("Test open with null name"));
	test((testObject.OpenGlobal(KNullDesC))==KErrNotFound);
	testObject.Close();

	test.Next(_L("Test open global object by name"));
	test((testObject.OpenGlobal(globalObject.FullName()))==KErrNone);
	testObject.Close();

	test.Next(_L("Check can't open local object by name"));
	test((testObject.OpenGlobal(localObject.FullName()))==PlatSecFindError);
	testObject.Close();

	test.Next(_L("Test duplicate global object in other process"));
	test((DuplicateInOtherProcess(globalObject))==KErrNone);

	test.Next(_L("Check can't duplicate local object in other process"));
	test((DuplicateInOtherProcess(localObject))==PlatSecProcessIsolationError);

	test.Next(_L("Test duplicate global object in other thread"));
	test((DuplicateInOtherThread(globalObject))==KErrNone);

	test.Next(_L("Test duplicate local object in other thead"));
	test((DuplicateInOtherThread(localObject))==KErrNone);

	test.Next(_L("Closing Semaphores"));
	globalObject.Close();
	localObject.Close();

	test.End();
	}



void TestMutexes()
	{
	TFullName name;
	
	test.Start(_L("Creating mutexes"));
	RMutex globalObject;
	RMutex localObject;
	RMutex testObject;
	test((globalObject.CreateGlobal(_L("T_SOBJECT-test-global-mutex")))==KErrNone);
	test((localObject.CreateLocal())==KErrNone);

	test.Next(_L("Test find global object"));
	TFindMutex find(globalObject.FullName());
	test((find.Next(name))==KErrNone);

	test.Next(_L("Test open found object"));
	test((testObject.Open(find))==KErrNone);
	testObject.Close();

	test.Next(_L("Check can't find local object"));
	TName objectName(localObject.FullName());
	find.Find(objectName);
	test((find.Next(name))==PlatSecFindError);

	test.Next(_L("Test open with null name"));
	test((testObject.OpenGlobal(KNullDesC))==KErrNotFound);
	testObject.Close();

	test.Next(_L("Test open global object by name"));
	test((testObject.OpenGlobal(globalObject.FullName()))==KErrNone);
	testObject.Close();

	test.Next(_L("Check can't open local object by name"));
	test((testObject.OpenGlobal(localObject.FullName()))==PlatSecFindError);
	testObject.Close();

	test.Next(_L("Test duplicate global object in other process"));
	test((DuplicateInOtherProcess(globalObject))==KErrNone);

	test.Next(_L("Check can't duplicate local object in other process"));
	test((DuplicateInOtherProcess(localObject))==PlatSecProcessIsolationError);

	test.Next(_L("Test duplicate global object in other thread"));
	test((DuplicateInOtherThread(globalObject))==KErrNone);

	test.Next(_L("Test duplicate local object in other thead"));
	test((DuplicateInOtherThread(localObject))==KErrNone);

	test.Next(_L("Closing mutexes"));
	globalObject.Close();
	localObject.Close();

	test.End();
	}



void TestMessageQueues()
	{
	test.Start(_L("Creating message queues"));
	RMsgQueue<TInt> globalObject;
	RMsgQueue<TInt> localObject;
	RMsgQueue<TInt> testObject;
	test((globalObject.CreateGlobal(_L("T_SOBJECT-test-global-msgqueue"),1))==KErrNone);
	test((localObject.CreateLocal(1))==KErrNone);

	test.Next(_L("Test open with null name"));
	test((testObject.OpenGlobal(KNullDesC))==KErrNotFound);
	testObject.Close();

	test.Next(_L("Test open global object by name"));
	test((testObject.OpenGlobal(globalObject.FullName()))==KErrNone);
	testObject.Close();

	test.Next(_L("Check can't open local object by name"));
	test((testObject.OpenGlobal(localObject.FullName()))==PlatSecFindError);
	testObject.Close();

	test.Next(_L("Test duplicate global object in other process"));
	test((DuplicateInOtherProcess(globalObject))==KErrNone);

	test.Next(_L("Check can't duplicate local object in other process"));
	test((DuplicateInOtherProcess(localObject))==PlatSecProcessIsolationError);

	test.Next(_L("Test duplicate global object in other thread"));
	test((DuplicateInOtherThread(globalObject))==KErrNone);

	test.Next(_L("Test duplicate local object in other thead"));
	test((DuplicateInOtherThread(localObject))==KErrNone);

	test.Next(_L("Closing message queues"));
	globalObject.Close();
	localObject.Close();

	test.End();
	}



void TestConditionVariables()
	{	
	test.Start(_L("Creating condition variables"));
	RCondVar globalObject;
	RCondVar localObject;
	RCondVar testObject;
	test((globalObject.CreateGlobal(_L("T_SOBJECT-test-global-condvar")))==KErrNone);
	test((localObject.CreateLocal())==KErrNone);

	test.Next(_L("Test open with null name"));
	test((testObject.OpenGlobal(KNullDesC))==KErrNotFound);
	testObject.Close();

	test.Next(_L("Test open global object by name"));
	test((testObject.OpenGlobal(globalObject.FullName()))==KErrNone);
	testObject.Close();

	test.Next(_L("Check can't open local object by name"));
	test((testObject.OpenGlobal(localObject.FullName()))==PlatSecFindError);
	testObject.Close();

	test.Next(_L("Test duplicate global object in other process"));
	test((DuplicateInOtherProcess(globalObject))==KErrNone);

	test.Next(_L("Check can't duplicate local object in other process"));
	test((DuplicateInOtherProcess(localObject))==PlatSecProcessIsolationError);

	test.Next(_L("Test duplicate global object in other thread"));
	test((DuplicateInOtherThread(globalObject))==KErrNone);

	test.Next(_L("Test duplicate local object in other thead"));
	test((DuplicateInOtherThread(localObject))==KErrNone);

	test.Next(_L("Closing message queues"));
	globalObject.Close();
	localObject.Close();

	test.End();
	}



void TestIPCHandles()
	{
	RTestProcess server;
	TRequestStatus rendezvous;
	TInt r;
	
	test.Next(_L("Test sending LogicalChannel handles"));
	{
	RLddTest localLdd;
	RLddTest protectedLdd;
	RLddTest returnLdd;
	r=User::LoadLogicalDevice(_L("D_SLDD.LDD"));
	test(r==KErrNone || r==KErrAlreadyExists);
	r=localLdd.OpenLocal();
	test(r==KErrNone);
	r=protectedLdd.OpenProtected();
	test(r==KErrNone);
	TInt lddValue=protectedLdd.Test1();
	test(lddValue==RLddTest::ETest1Value);
	SetName(protectedLdd);
	r = Session.Send(CTestSession::ETestLdd,TIpcArgs(localLdd,protectedLdd,lddValue,&Name));
	r = returnLdd.SetReturnedHandle(r);
	test(r==KErrNone);
	test(CheckName(returnLdd));
	protectedLdd.Close();
	returnLdd.Close();
	localLdd.Close();
	}

	test.Next(_L("Test sending Chunk handles"));
	{
	RChunk localChunk;
	RChunk globalChunk;
	RChunk protectedChunk;
	RChunk returnChunk;
	test((r=localChunk.CreateLocal(0x1000,0x100000,EOwnerThread))==KErrNone);
	test((r=globalChunk.CreateGlobal(KTestChunkName,0x1000,0x100000,EOwnerProcess))==KErrNone);
	test((r=protectedChunk.CreateGlobal(KNullDesC,0x1000,0x100000,EOwnerProcess))==KErrNone);
	SetName(protectedChunk);
	r = Session.Send(CTestSession::ETestChunk,TIpcArgs(localChunk,globalChunk,protectedChunk,&Name));
	r = returnChunk.SetReturnedHandle(r);
	test(r==KErrNone);
	test(CheckName(returnChunk));
	returnChunk.Close();
	protectedChunk.Close();
	globalChunk.Close();
	localChunk.Close();
	}

	test.Next(_L("Test sending Semaphore handles"));
	{
	RSemaphore localSemaphore;
	RSemaphore globalSemaphore;
	RSemaphore protectedSemaphore;
	RSemaphore returnSemaphore;
	test((r=localSemaphore.CreateLocal(1,EOwnerThread))==KErrNone);
	test((r=globalSemaphore.CreateGlobal(KTestSemaphoreName,1,EOwnerProcess))==KErrNone);
	test((r=protectedSemaphore.CreateGlobal(KNullDesC,1,EOwnerProcess))==KErrNone);
	SetName(protectedSemaphore);
	r = Session.Send(CTestSession::ETestSemaphore,TIpcArgs(localSemaphore,globalSemaphore,protectedSemaphore,&Name));
	r = returnSemaphore.SetReturnedHandle(r);
	test(r==KErrNone);
	test(CheckName(returnSemaphore));
	returnSemaphore.Close();
	protectedSemaphore.Close();
	globalSemaphore.Close();
	localSemaphore.Close();
	}

	test.Next(_L("Test sending Mutex handles"));
	{
	RMutex localMutex;
	RMutex globalMutex;
	RMutex protectedMutex;
	RMutex returnMutex;
	test((r=localMutex.CreateLocal(EOwnerThread))==KErrNone);
	test((r=globalMutex.CreateGlobal(KTestMutexName,EOwnerProcess))==KErrNone);
	test((r=protectedMutex.CreateGlobal(KNullDesC,EOwnerProcess))==KErrNone);
	SetName(protectedMutex);
	r = Session.Send(CTestSession::ETestMutex,TIpcArgs(localMutex,globalMutex,protectedMutex,&Name));
	r = returnMutex.SetReturnedHandle(r);
	test(r==KErrNone);
	test(CheckName(returnMutex));
	returnMutex.Close();
	protectedMutex.Close();
	globalMutex.Close();
	localMutex.Close();
	}

	test.Next(_L("Test sending MsgQueue handles"));
	{
	RMsgQueue<TInt> localMsgQueue;
	RMsgQueue<TInt> globalMsgQueue;
	RMsgQueue<TInt> protectedMsgQueue;
	RMsgQueue<TInt> returnMsgQueue;
	test((r=localMsgQueue.CreateLocal(1,EOwnerThread))==KErrNone);
	test((r=globalMsgQueue.CreateGlobal(KTestMsgQueueName,1,EOwnerProcess))==KErrNone);
	test((r=protectedMsgQueue.CreateGlobal(KNullDesC,1,EOwnerProcess))==KErrNone);
	SetName(protectedMsgQueue);
	r = Session.Send(CTestSession::ETestMsgQueue,TIpcArgs(localMsgQueue,globalMsgQueue,protectedMsgQueue,&Name));
	r = returnMsgQueue.SetReturnedHandle(r);
	test(r==KErrNone);
	test(CheckName(returnMsgQueue));
	returnMsgQueue.Close();
	protectedMsgQueue.Close();
	globalMsgQueue.Close();
	localMsgQueue.Close();
	}

	test.Next(_L("Test sending CondVar handles"));
	{
	RCondVar localCondVar;
	RCondVar globalCondVar;
	RCondVar protectedCondVar;
	RCondVar returnCondVar;
	test((r=localCondVar.CreateLocal(EOwnerThread))==KErrNone);
	test((r=globalCondVar.CreateGlobal(KTestCondVarName,EOwnerProcess))==KErrNone);
	test((r=protectedCondVar.CreateGlobal(KNullDesC,EOwnerProcess))==KErrNone);
	SetName(protectedCondVar);
	r = Session.Send(CTestSession::ETestCondVar,TIpcArgs(localCondVar,globalCondVar,protectedCondVar,&Name));
	r = returnCondVar.SetReturnedHandle(r);
	test(r==KErrNone);
	test(CheckName(returnCondVar));
	returnCondVar.Close();
	protectedCondVar.Close();
	globalCondVar.Close();
	localCondVar.Close();
	}

	test.Start(_L("Starting test server 2"));
	server.Create(ETestProcessServer,EGlobalSharableServer);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
	test(rendezvous==KServerRendezvous);
	server.Close();

	test.Next(_L("Test sending Session handles"));
	{
	RTestSession localSession;
	RTestSession protectedSession;
	RTestSession returnSession;
	RFs fsSession;
	test((r=localSession.Connect(EGlobalSharableServer))==KErrNone);
	test((r=localSession.Send(CTestSession::EPing,TIpcArgs(123)))==123);
	test((r=protectedSession.ConnectProtected(EGlobalSharableServer))==KErrNone);
	test((r=protectedSession.Send(CTestSession::EPing,TIpcArgs(123)))==123);
	test((r=fsSession.Connect())==KErrNone);
	test((r=fsSession.ShareProtected())==KErrNone);
	SetName(protectedSession);
	r = Session.Send(CTestSession::ETestSession,TIpcArgs(localSession,protectedSession,fsSession,&Name));
	r = returnSession.SetReturnedHandle(r);
	test(r==KErrNone);
	test(CheckName(returnSession));
	returnSession.Close();
	protectedSession.Close();
	localSession.Close();
	fsSession.Close();
	}

	test.Next(_L("Test receiving Session handles"));
	{
	RTestSession returnSession;
	r = Session.Send(CTestSession::ETestSession2);
	r = returnSession.SetReturnedHandle(r);
	test(r==KErrNone);
	test((r=Session.Send(CTestSession::EPing,TIpcArgs(123)))==123);  // So we know server has closed returnedSession
	test((r=returnSession.Send(CTestSession::EPing,TIpcArgs(123)))==123);
	returnSession.Close();
	RFs returnFsSession;
	r = Session.Send(CTestSession::ETestSession3);
	r = returnSession.SetReturnedHandle(r,TSecurityPolicy(TSecureId(0x100039e3))); // f32 sid
	test(r==KErrNone);
	returnFsSession.Close();
	}

	test.Next(_L("Try global sharing of Sessions without server support"));
	{
	RTestSession protectedSession;
	test((r=protectedSession.ConnectProtected(EMainServer))==KErrPermissionDenied);
	}

	test.Next(_L("Stopping test server 2"));
	{
	RTestSession session;
	test((r=session.Connect(EGlobalSharableServer))==KErrNone);
	session.Send(CTestSession::EShutdown);
	session.Close();
	}

	test.End();
	}

// Test uniqness of object names is enforced by DObjectCon when creating objects
void TestObjectNames()
	{
	_LIT(KNameFormat, "TestObject-%d");
	const TInt KObjectCount = 10;
	RMutex objects[KObjectCount];

	test.Start(_L("Test uniqueness of object names"));
	
	// Test creating named and unnamed objects is ok
	TInt i;
	for (i = 0 ; i < KObjectCount ; ++i)
		{
		if (i % 2)
			test(objects[i].CreateLocal() == KErrNone);
		else
			{
			TBuf<16> name;
			name.AppendFormat(KNameFormat, i);
			test(objects[i].CreateGlobal(name) == KErrNone);
			}
		}

	// Test we cannot create objects with duplicate names
	for (i = 0 ; i < KObjectCount ; i+=2)
		{
		TBuf<16> name;
		name.AppendFormat(KNameFormat, i);
		test(objects[i].CreateGlobal(name) == KErrAlreadyExists);
		}	
	
	// Close all objects
	for (i = 0 ; i < KObjectCount ; ++i)
		{
		objects[i].Close();
		}

	test.End();
	}


TInt DoTestProcess(TInt aTestNum,TInt aArg1,TInt aArg2)
	{
	TInt r;

	switch(aTestNum)
		{

	case ETestProcessServer:
		return StartServer((TServerName)aArg1);

	case ETestProcessDuplicate:
		{
		RHandleBase handle;
		handle.SetHandle(aArg2);
		RThread thread;
		r = thread.Open(TThreadId(aArg1));
		if(r!=KErrNone)
			r = 999;
		else
			r = handle.Duplicate(thread);
		thread.Close();
		}
		return r;

	case ETestProcessOpenThreadById:
		{
		RThread thread;
		r = thread.Open(TThreadId(aArg1));
		if(r==KErrNone)
			thread.Close();
		}
		return r;


	default:
		User::Panic(_L("T_SOBJECT"),1);
		}

	return KErrNone;
	}


GLDEF_C TInt E32Main()
    {
	PlatSecProcessIsolationError = PlatSec::ConfigSetting(PlatSec::EPlatSecProcessIsolation)&&PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement)
		? KErrPermissionDenied : KErrNone;
	PlatSecFindError = PlatSec::ConfigSetting(PlatSec::EPlatSecProcessIsolation)
		? KErrNotFound : KErrNone;

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
	
	test.Start(_L("Starting test servers"));
	RTestProcess server;
	TRequestStatus rendezvous;
	TRequestStatus svrstat;
	server.Create(ETestProcessServer,EMainServer);
	server.NotifyDestruction(svrstat);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
	test(rendezvous==KServerRendezvous);
	server.Close();
	test((StartServerInThread(EAnonymousServer))==KErrNone);

	test.Next(_L("Openning server session"));
	test((Session.Connect())==KErrNone);

	test.Next(_L("Test Find and Open PhysicalDevices"));
	TestPhysicalDevices();

	test.Next(_L("Test Find and Open LogicalDevices"));
	TestLogicalDevices();

	test.Next(_L("Test Find and Open Libraries"));
	TestLibraries();

	test.Next(_L("Test Find and Open Servers"));
	TestServers();

	test.Next(_L("Test Find and Open Processes"));
	TestProcesses();

	test.Next(_L("Test Find and Open Threads"));
	TestThreads();

	test.Next(_L("Test Find and Open Chunks"));
	TestChunks();

	test.Next(_L("Test Find and Open Semaphores"));
	TestSemaphores();

	test.Next(_L("Test Find and Open Mutexes"));
	TestMutexes();

	test.Next(_L("Test Message Queues"));
	TestMessageQueues();

	test.Next(_L("Test Condition Variables"));
	TestConditionVariables();

	test.Next(_L("Test passing handle via IPC"));
	TestIPCHandles();

	test.Next(_L("Test object names"));
	TestObjectNames();

	test.Next(_L("Stopping test server"));
	Session.Send(CTestSession::EShutdown);
	Session.Close();
	User::WaitForRequest(svrstat);
	test(svrstat == KErrNone);

	test.End();

	return(0);
    }

