// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\t_trap.cpp
// Overview:
// Test TRAP, Leave and Assert
// API Information:
// TRAP, User::Leave, __ASSERT_DEBUG_NO_LEAVE, __ASSERT_ALWAYS_NO_LEAVE
// Details:
// - Test TRAP macro works as expected.
// - Test User::Leave works as expected including leave from
// within nested calls.
// - Verify that a leave without a TRAP causes the thread to panic.
// - Create a thread that asserts and verify the exit type and other
// results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32panic.h>

const TInt KLeaveVal=1111;
const TInt KUnLeaveVal=2222;
const TInt KRecursiveUnLeaveVal=3333;
const TInt KRecursiveSingleLeaveVal=4444;
const TInt KMaxDepth=20;

//#define __TEST_BREAKPOINT_IN_TRAP__

LOCAL_D RTest test(_L("T_TRAP"));


LOCAL_C TInt UnLeaveFunction(void)
	{

	return(KUnLeaveVal);
	}

LOCAL_C TInt LeaveFunction(void)
	{

	User::Leave(KLeaveVal);
	return(0);
	}

LOCAL_C TInt RecursiveUnLeave(TInt level)
	{

	if (level==0)
		return(KRecursiveUnLeaveVal);
	else
		return(RecursiveUnLeave(--level));
	}

LOCAL_C TInt RecursiveSingleLeave(TInt level)
	{

	if (level==0)
		User::Leave(KRecursiveSingleLeaveVal);
	else
		RecursiveSingleLeave(--level);
	return(0);
	}

LOCAL_C TInt RecursiveMultiLeave1(TInt level)
	{

	TInt ret=0;
	TRAP(ret,{if (level==0)	User::Leave(level);	else ret=RecursiveMultiLeave1(level-1);	test(EFalse);})
	test(ret==level);
	User::Leave(level+1);
	return(0);
	}

LOCAL_C TInt RecursiveMultiLeave2(TInt level)
	{

	if (level==0)
		return(1);
	TInt ret=0;
	TRAP(ret,ret=RecursiveMultiLeave2(level-1))
	test(ret==level);
	User::Leave(level+1);
	return(0);
	}
	
LOCAL_C TInt doTrap(TInt aVal)
//
// Nest trap function.
//
	{

	if (aVal)
		{
		TInt j=(-1);
		TRAP(j,j=doTrap(aVal-1))
		test(j==aVal);
		}
	return(aVal+1);
	}

#ifdef __TEST_BREAKPOINT_IN_TRAP__
void bkpt()
	{
	__BREAKPOINT();
	}
#endif

LOCAL_C void doLeave(TInt aLevel,TInt aVal)
//
// Nest trap with leave function.
//
	{

	if (aLevel)
		doLeave(aLevel-1,aVal);
	else
		User::Leave(aVal);
	}

LOCAL_C void testTrap()
//
// Test trap functions O.K.
//
	{

	test.Start(_L("Trap level 1"));
//
	TInt i=2;
	TRAP(i,i=1);
	test(i==1);
#ifdef __TEST_BREAKPOINT_IN_TRAP__
	TRAP(i,bkpt());
	TRAP(i,TRAP(i,bkpt()));
#endif
//
	test.Next(_L("Trap level n"));
	for (i=1;i<KMaxDepth;i++)
		test(doTrap(i)==(i+1));
//
	test.End();
	}

LOCAL_C void testLeave()
//
// Test leave functions O.K.
//
	{

	test.Start(_L("Leave level 1"));
//
	TInt i=0;
	TRAP(i,User::Leave(2))
	test(i==2);
//
	test.Next(_L("Leave level 2"));
	i=0;
	TRAP(i,TRAP(i,User::Leave(3)))
	test(i==3);
//
#ifdef __TEST_BREAKPOINT_IN_TRAP__
	TRAP(i,TRAP(i,User::Leave(33)); bkpt())
	test(i==33);
#endif
//
	test.Next(_L("Leave from nested calls"));
	for (i=1;i<KMaxDepth;i++)
		{
		TInt j=(-1);
		TRAP(j,doLeave(i,i))
		test(j==i);
		}
//
	test.End();
	}

LOCAL_C void testMH(void)
	{

	TInt ret=0;
	TRAP(ret,ret=UnLeaveFunction())
	test(ret==KUnLeaveVal);
	TRAP(ret,LeaveFunction())
	test(ret==KLeaveVal);
	TInt i=0;
	for(;i<=KMaxDepth;i++)
		{
		TRAP(ret,ret=RecursiveUnLeave(i))
		test(ret==KRecursiveUnLeaveVal);
		}
	for(i=0;i<=KMaxDepth;i++)
		{
		TRAP(ret,ret=RecursiveSingleLeave(i))
		test(ret==KRecursiveSingleLeaveVal);
		}
	for(i=0;i<=KMaxDepth;i++)
		{
		TRAP(ret,ret=RecursiveMultiLeave1(i))
		test(ret==i+1);
		}
	for(i=0;i<=KMaxDepth;i++)
		{
		TRAP(ret,ret=RecursiveMultiLeave2(i))
		test(ret==i+1);
		}
	}

TInt LeaveNoTrapThread(TAny*)
	{
	User::Leave(KErrGeneral);
	return KErrNone;
	}

void TestLeaveNoTrap()
	{
	RThread thread;
	TInt r=thread.Create(_L("Leave without Trap thread"),LeaveNoTrapThread,0x1000,&User::Allocator(),NULL);
	test(r==KErrNone);
	TRequestStatus stat;
	thread.Logon(stat);
	test(stat==KRequestPending);
	TBool justInTime=User::JustInTime();
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(stat);
	User::SetJustInTime(justInTime);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitReason()==EUserLeaveWithoutTrap);
	test(thread.ExitCategory()==_L("USER"));
	CLOSE_AND_WAIT(thread);
	}

enum TAssertTest
	{
	EAssertTest_Debug = 1,
	EAssertTest_Leave = 2,
	EAssertTest_Ret = 4,
	};

TInt AssertThread(TAny* a)
	{
	TInt f = (TInt)a;
	TInt r = f | EAssertTest_Ret;
	if (f & EAssertTest_Leave)
		{
		if (f & EAssertTest_Debug)
			{
			__ASSERT_DEBUG_NO_LEAVE(User::Leave(r));
			}
		else
			{
			__ASSERT_ALWAYS_NO_LEAVE(User::Leave(r));
			}
		}
	else
		{
		if (f & EAssertTest_Debug)
			{
			__ASSERT_DEBUG_NO_LEAVE(RThread().Terminate(r));
			}
		else
			{
			__ASSERT_ALWAYS_NO_LEAVE(RThread().Terminate(r));
			}
		}
	return r;
	}

TInt _AssertThread(TAny* a)
	{
	TInt s=0;
	TRAP_IGNORE(s=AssertThread(a));
	return s;
	}

void TestAssert(TInt aTest)
	{
	test.Printf(_L("Assert %d\n"), aTest);
	RThread t;
	TInt r = t.Create(_L("assert"), &_AssertThread, 0x1000, NULL, (TAny*)aTest);
	test(r==KErrNone);
	TRequestStatus s;
	t.Logon(s);
	test(s==KRequestPending);
	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	t.Resume();
	User::WaitForRequest(s);
	User::SetJustInTime(jit);
	TInt exitType = t.ExitType();
	TInt exitReason = t.ExitReason();
	const TDesC& exitCat = t.ExitCategory();
	CLOSE_AND_WAIT(t);
	test.Printf(_L("Exit %d,%d,%S\n"), exitType, exitReason, &exitCat);
	if (aTest & EAssertTest_Leave)
		{
		if (aTest & EAssertTest_Debug)
			{
#ifdef _DEBUG
			test(exitType == EExitPanic);
			test(exitReason == EUnexpectedLeave);
#else
			test(exitType == EExitKill);
			test(exitReason == KErrNone);
#endif
			}
		else
			{
			test(exitType == EExitPanic);
			test(exitReason == EUnexpectedLeave);
			}
		}
	else
		{
		test(exitType == EExitTerminate);
		test(exitReason == (aTest | EAssertTest_Ret));
		}
	}

/*============== server for testing exceptions in TRAP implementation ====================*/

#include <e32base.h>
#include <e32base_private.h>

#include "../mmu/mmudetect.h"

const TInt KHeapSize=0x2000;

_LIT(KServerName,"Display");

class CMySession : public CSession2
	{
public:
	CMySession();
	virtual void ServiceL(const RMessage2& aMessage);
	};

class CMyServer : public CServer2
	{
public:
	enum {ERead,EStop};
public:
	CMyServer(TInt aPriority);
	static CMyServer* New(TInt aPriority);
	virtual CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;//Overloading
	};

class RDisplay : public RSessionBase
	{
public:
	TInt Open();
	void Read(TRequestStatus& aStatus);
	TInt Stop();
	};

LOCAL_D RTest testSvr(_L("T_TRAP Server"));
LOCAL_D RSemaphore client;
LOCAL_D RSemaphore server;
LOCAL_D RDisplay display;
LOCAL_D const RMessage2* message;

// Constructor
//
// 
CMySession::CMySession()
	{}

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

CSession2* CMyServer::NewSessionL(const TVersion& /*aVersion*/, const RMessage2&) const
//
// Create a new client for this server.
//
	{
	return(new(ELeave) CMySession());
	}

void CMySession::ServiceL(const RMessage2& aMessage)
//
// Handle messages for this server.
//
	{
	TInt r=KErrNone;
	switch (aMessage.Function())
		{
	case CMyServer::ERead:
		testSvr.Printf(_L("read message received\n"));
		if (HaveVirtMem())
			{
			message = &aMessage;
			}
		client.Signal();
		server.Wait();
		break;
	case CMyServer::EStop:
		testSvr.Printf(_L("stop message received\n"));
		CActiveScheduler::Stop();
		break;
	default:
		r=KErrNotSupported;
		}
	aMessage.Complete(r);
	}

TInt RDisplay::Open()
//
// Open the server.
//
	{
	return(CreateSession(KServerName,TVersion(),1));
	}

void RDisplay::Read(TRequestStatus& aStatus)
//
// Get session to test CSession2::ReadL.
//
	{
	TBuf<0x10>* bad = (TBuf<0x10> *)(0x30000000);
	SendReceive(CMyServer::ERead, TIpcArgs(bad), aStatus);
	}

TInt RDisplay::Stop()
//
// Stop the server.
//
	{
	return SendReceive(CMyServer::EStop, TIpcArgs());
	}

LOCAL_C TInt serverThreadEntryPoint(TAny*)
//
// The entry point for the server thread.
//
	{
	testSvr.Title();
	testSvr.Start(_L("Create CActiveScheduler"));
	CActiveScheduler* pR=new CActiveScheduler;
	testSvr(pR!=NULL);
	CActiveScheduler::Install(pR);
//
	testSvr.Next(_L("Create CMyServer"));
	CMyServer* pS=CMyServer::New(0);
	testSvr(pS!=NULL);
//
	testSvr.Next(_L("Start CMyServer"));
	TInt r=pS->Start(KServerName);
	testSvr(r==KErrNone);
//
	testSvr.Next(_L("Signal to client that we have started"));
	client.Signal();
//
	testSvr.Next(_L("Start CActiveScheduler"));
	CActiveScheduler::Start();
//
	testSvr.Next(_L("Exit server"));
	delete pS;
	testSvr.Close();
	return(KErrNone);
	}

void CreateServer()
	{
	test.Next(_L("Creating client semaphore"));
	TInt r=client.CreateLocal(0);
	test(r==KErrNone);
//
	test.Next(_L("Creating server semaphore"));
	r=server.CreateLocal(0);
	test(r==KErrNone);
//
	test.Next(_L("Creating server thread"));
	RThread server;
	r=server.Create(_L("Server"),serverThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	server.SetPriority(EPriorityMore);
//
	test.Next(_L("Resume server thread"));
	server.Resume();
	test(ETrue);
//
	test.Next(_L("Wait for server to start"));
	client.Wait();
//
	test.Next(_L("Connect to server"));
	r=display.Open();
	test(r==KErrNone);
	}

void StopServer()
	{
	test.Next(_L("Stop server"));
	TInt r=display.Stop();
	test(r==KErrNone);
//
	test.Next(_L("Close connection"));
	display.Close();
//
	test.Next(_L("Close all"));
	server.Close();
	client.Close();
    }

/*============== end of server for testing exceptions in TRAP implementation ====================*/

#undef TRAP_INSTRUMENTATION_START
#undef TRAP_INSTRUMENTATION_NOLEAVE
#undef TRAP_INSTRUMENTATION_LEAVE
#define TRAP_INSTRUMENTATION_START			++TrapStart;
#define TRAP_INSTRUMENTATION_NOLEAVE		++TrapNoLeave; TestExcInInstrumentation();
#define TRAP_INSTRUMENTATION_LEAVE(aReason) TrapLeave=aReason;

TInt TrapStart = 0;
TInt TrapNoLeave = 0;
TInt TrapLeave = 123;

//
// This is mostly for the benefit of WINS, where Win32 exceptions
// have a nasty habit of interacting badly with C++ exceptions
//

void TestExcInInstrumentation()
	{
	TRequestStatus status;
	display.Read(status);
	test(status.Int() == KRequestPending);

	client.Wait();

	TBuf<0x100> buf;
	if (message)
		test(message->Read(0,buf) == KErrBadDescriptor);

	server.Signal();

	User::WaitForRequest(status);
	test(status.Int() == KErrNone);
	}

void TestTrapInstrumentation()
	{
	CreateServer();

	test.Start(_L("TRAPD No Leave"));
	TRAPD(r,User::LeaveIfError(0));
	test(TrapStart==1);
	test(TrapLeave==123);
	test(r==0);
	test(TrapNoLeave==1);

	test.Next(_L("TRAP No Leave"));
	TRAP(r,User::LeaveIfError(0));
	test(TrapStart==2);
	test(TrapLeave==123);
	test(r==0);
	test(TrapNoLeave==2);

	test.Next(_L("TRAP_IGNORE No Leave"));
	TRAP_IGNORE(User::LeaveIfError(0));
	test(TrapStart==3);
	test(TrapLeave==123);
	test(TrapNoLeave==3);

	test.Next(_L("TRAPD Leave"));
	TRAPD(r2,User::LeaveIfError(-999));
	test(TrapStart==4);
	test(TrapLeave==-999);
	test(r2==TrapLeave);
	test(TrapNoLeave==3);

	test.Next(_L("TRAP Leave"));
	TRAP(r2,User::LeaveIfError(-666));
	test(TrapStart==5);
	test(TrapLeave==-666);
	test(r2==TrapLeave);
	test(TrapNoLeave==3);

	test.Next(_L("TRAP_IGNORE Leave"));
	TRAP_IGNORE(User::LeaveIfError(-333));
	test(TrapStart==6);
	test(TrapLeave==-333);
	test(TrapNoLeave==3);

	test.Next(_L("Leave"));
	test.End();

	StopServer();
	}

#undef TRAP_INSTRUMENTATION_START
#undef TRAP_INSTRUMENTATION_NOLEAVE
#undef TRAP_INSTRUMENTATION_LEAVE
#define TRAP_INSTRUMENTATION_START
#define TRAP_INSTRUMENTATION_NOLEAVE
#define TRAP_INSTRUMENTATION_LEAVE(aReason)

#ifdef __WINS__
TUint32* Stack;
volatile TInt* volatile Q;
const TInt A[] = {17,19,23,29,31,37,41,43,47,53};

void ExceptionHandler(TExcType)
	{
	TUint32* sp = Stack;
	for (; *sp!=0xfacefeed; --sp) {}
	*sp = (TUint32)(Q++);
	}

__NAKED__ TInt GetNext()
	{
	_asm mov Stack, esp
	_asm mov eax, 0facefeedh
	_asm mov eax, [eax]
	_asm ret
	}

void testExceptionsInTrap()
	{
	TInt ix = 0;
	TInt r;
	User::SetExceptionHandler(&ExceptionHandler, 0xffffffff);
	Q = (volatile TInt* volatile)A;
	r = GetNext();
	test(r==A[ix++]);
	TInt i;
	TRAP(i,r=GetNext());
	test(i==0);
	test(r==A[ix++]);
	TRAP(i,TRAP(i,r=GetNext()));
	test(i==0);
	test(r==A[ix++]);
	TRAP(i, TRAP(i,User::Leave(271));r=GetNext(); );
	test(i==271);
	test(r==A[ix++]);
	TRAP(i, TRAP(i, TRAP(i,User::Leave(487));r=GetNext(); ); );
	test(i==487);
	test(r==A[ix++]);
	TInt s=-1;
	TRAP(i, TRAP(i, TRAP(i, TRAP(i,User::Leave(999));r=GetNext(); ); s=GetNext(); ); );
	test(i==999);
	test(r==A[ix++]);
	test(s==A[ix++]);
	TInt j=-1, k=-1, l=-1;
	TRAP(l,										\
		TRAP(k,									\
			TRAP(j,								\
				TRAP(i,User::Leave(9991));		\
				r=GetNext();					\
				);								\
			User::Leave(9992);					\
			);									\
		s=GetNext();							\
		);
	test(i==9991);
	test(j==0);
	test(k==9992);
	test(l==0);
	test(r==A[ix++]);
	test(s==A[ix++]);
	}
#endif

GLDEF_C TInt E32Main()
    {
	test.Title();
//
	test.Start(_L("Trap"));
	testTrap();
//
	test.Next(_L("Leave"));
	testLeave();
//
	test.Next(_L("Assorted"));
	testMH();
//
	test.Next(_L("Leave without Trap"));
	TestLeaveNoTrap();
//
	test.Next(_L("Assertions"));
	TestAssert(0);
	TestAssert(EAssertTest_Debug);
	TestAssert(EAssertTest_Leave);
	TestAssert(EAssertTest_Leave | EAssertTest_Debug);

#ifdef __LEAVE_EQUALS_THROW__
	test.Next(_L("Trap instrumentation"));
	TestTrapInstrumentation();
#endif

#ifdef __WINS__
	testExceptionsInTrap();
#endif

	test.End();
	return(0);
    }
