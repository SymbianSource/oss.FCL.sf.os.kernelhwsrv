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
// e32test\bench\t_svr5.cpp
// Overview:
// Test client server shareable sessions and benchmark their performance.
// API information:
// CSession2, CServer2, RSessionBase, RMessage2, DServer, DSession 
// Details:
// - Start the test server
// - Open a connection with the server specifying 0 message slots should be 
// available for the session and verify the server returns KErrServerBusy 
// when it tries to pass a message to it.
// - Open a connection using a fixed pool of messages and test that the server 
// handles the messages correctly.
// - Open a connection using the kernel's global pool of messages and test the 
// servers handles the messages correctly.
// - Open a shareable session with the server and verify that:
// - all arguments are passed to the server and back correctly
// - server can read and write to/from the client and return the appropriate 
// errors when bad descriptors are passed in.
// - another thread can share the session to send messages to the server.
// - a message sent by one thread can be saved and completed later in 
// response to a message sent by a different thread.
// - another thread can close the session.
// - a different thread can attach to a session by handle.
// - Establish a global shareable session to the server and test it fails, as 
// the server doesn't support global shareable sessions.
// - Open an unshareable session with the server and verify that:
// - all arguments are passed to the server and back correctly.
// - server can reads and write to/from the client and return the 
// appropriate errors when bad descriptors are passed in.
// - the session handle is local (thread owned)
// - Open an automatically shared session using ShareAuto and test it can be 
// shared by threads in the same process.
// - Send dummy messages that the server completes immediately and display how 
// many are completed per second.
// - Verify that stopping the server completes existing pending requests with 
// KErrServerTerminated.
// - Verify that the kernel does not crash by completing a message with an invalid 
// handle and verify the client is panicked with EBadMessageHandle.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// This test does not help to validate an EKA2 port.
// 
//

#include <e32base.h>
#include <e32base_private.h>
#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>

#include "../mmu/mmudetect.h"

const TInt KHeapSize=0x2000;
const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=0;
const TInt KBuildVersionNumber=1;

_LIT(KServerName,"Display");

TInt SessionCreateLResult=0;
TInt SessionCostructCount=0;
TInt SessionDestructCount=0;

class CMySession : public CSession2
	{
public:
	CMySession();
	~CMySession();
	void DisplayName(const RMessage2& aM);
	virtual void ServiceL(const RMessage2& aMessage);
	virtual void CreateL();
	TInt Hold(const RMessage2& aM);
	TInt Release(const RMessage2& aM);
public:
	RPointerArray<TInt> iAsyncMsgArray;	// 'pointers' are actually message handles
	TInt iCompleteIndex;
	};

TInt AsyncMsgOrder(const TInt& aL, const TInt& aR)
	{
	TInt lh = (TInt)&aL;
	TInt rh = (TInt)&aR;
	RMessage2 l(*(const RMessagePtr2*)&lh);
	RMessage2 r(*(const RMessagePtr2*)&rh);
	return l.Int0() - r.Int0();
	}

TInt GetClientName(const RMessagePtr2& aM, TFullName& aN)
	{
	RThread t;
	TInt r = aM.Client(t);
	if (r==KErrNone)
		{
		aN = t.FullName();
		t.Close();
		}
	return r;
	}

class CMyServer : public CServer2
	{
public:
	enum {EDisplay,ERead,EWrite,ETest,EStop,EHold,ERelease,EGetCompleteIndex};
public:
	CMyServer(TInt aPriority);
	static CMyServer* New(TInt aPriority);
	virtual CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const;
	};

class CMyActiveScheduler : public CActiveScheduler
	{
public:
	virtual void Error(TInt anError) const;	 //Overloading pure virtual function
	};

class RDisplay : public RSessionBase
	{
public:
	TInt Open();
	TInt OpenNoAsync();
	TInt OpenDynamic();
	TInt OpenUS();
	TInt OpenS();
	TInt OpenGS();
	TInt Display(const TDesC& aMessage);
	TInt Read();
	TInt Write();
	TInt Stop();
	TInt Test();
	TInt Echo(TInt aWhat, TInt a0, TInt a1, TInt a2, TInt a3);
	TVersion Version();
	TInt Hold(TInt aOrder=0, TInt aArg=0);
	void Hold(TRequestStatus& aStatus, TInt aOrder=0, TInt aArg=0);
	TInt Release(TInt aCount=KMaxTInt);
	TInt CompleteIndex();
	};

LOCAL_D RTest test(_L("T_SVR"));
LOCAL_D RTest testSvr(_L("T_SVR Server"));
LOCAL_D RTest testSpeedy(_L("T_SVR Speedy"));
LOCAL_D RSemaphore client;
LOCAL_D TInt speedCount;

CMySession::CMySession()
//
// Constructor
//
	{
	++SessionCostructCount;
	}

CMySession::~CMySession()
	{
	iAsyncMsgArray.Close();
	++SessionDestructCount;
	}

void CMySession::CreateL()
	{
	User::LeaveIfError(SessionCreateLResult);
	}

void CMySession::DisplayName(const RMessage2& aM)
//
// Display the client's name.
//
	{

	TFullName fn;
	TInt r = GetClientName(aM, fn);
	testSvr(r==KErrNone);
	TBuf<256> text;
	r=aM.Read(0,text);
	testSvr(r==KErrNone);
	testSvr.Printf(_L("Session %08x %S\n%S\n"), this, &fn, &text);
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

	TInt f = aMessage.Function();
	if (f & 0x40000000)
		{
		TInt what = f & 0x3fffffff;
		TInt a0 = aMessage.Int0();
		TInt a1 = aMessage.Int1();
		TInt a2 = aMessage.Int2();
		TInt a3 = aMessage.Int3();
		switch (what)
			{
			case 0:
				aMessage.Complete(a0);
				return;
			case 1:
				aMessage.Complete(a1);
				return;
			case 2:
				aMessage.Complete(a2);
				return;
			case 3:
				aMessage.Complete(a3);
				return;
			case 4:
				aMessage.Complete(a0+a1+a2+a3);
				return;
			case 5:
				aMessage.Complete(a0*a0+a1*a1+a2*a2+a3*a3);
				return;
			default:
				break;
			}
		}

	TInt r=KErrNone;
	TBuf<0x100> b;
	switch (f)
		{
	case CMyServer::EDisplay:
		DisplayName(aMessage);
		break;
	case CMyServer::ERead:
		testSvr.Printf(_L("read message received\n"));
		r=aMessage.Read(0,b);
		if (r==KErrNone && b!=_L("Testing read"))
			r=KErrGeneral;
		if (r==KErrNone)
			{
            r=aMessage.Read(0,b,-1);
			if (r==KErrNone)
				r=KErrGeneral;
			if (r==KErrArgument)
				r=KErrNone;
			}
		if (r==KErrNone && HaveVirtMem())
			{
			r=aMessage.Read(1,b);
			if (r==KErrNone)
				r=KErrGeneral;
			if (r==KErrBadDescriptor)
				r=KErrNone;
			}
		break;
	case CMyServer::EWrite:
		testSvr.Printf(_L("write message received\n"));
		r=aMessage.Write(0,_L("It worked!"));
		RDebug::Print(_L("Write returns %d"),r);
		if (r==KErrNone)
			{
			r=aMessage.Write(0,b,-1);
			if (r==KErrNone)
				r=KErrGeneral;
			if (r==KErrArgument)
				r=KErrNone;
			}
		if (r==KErrNone)
			{
			r=aMessage.Write(1,b);
			if (r==KErrNone)
				r=KErrGeneral;
			if (r==KErrBadDescriptor)
				r=KErrNone;
			}
		if (r==KErrNone && HaveVirtMem())
			{
			r=aMessage.Write(2,b);
			if (r==KErrNone)
				r=KErrGeneral;
			if (r==KErrBadDescriptor)
				r=KErrNone;
			}
		break;
	case CMyServer::ETest:
		break;
	case CMyServer::EStop:
		testSvr.Printf(_L("stop message received\n"));
		CActiveScheduler::Stop();
		break;
	case CMyServer::EHold:
		{
		r = Hold(aMessage);
		if (r==KErrNone)
			return;
		break;
		}
	case CMyServer::ERelease:
		{
		r = Release(aMessage);
		break;
		}
	case CMyServer::EGetCompleteIndex:
		{
		r = iCompleteIndex;
		break;
		}
	default:
		r=KErrNotSupported;
		}
	aMessage.Complete(r);
	}

TInt CMySession::Hold(const RMessage2& a)
	{
	TInt ord = a.Int0();
	TInt arg = a.Int1();
	TFullName fn;
	TInt r = GetClientName(a, fn);
	testSvr(r==KErrNone);
    testSvr.Printf(_L("Hold message from %S ord %08x arg %08x\n"), &fn, ord, arg);
	r = iAsyncMsgArray.InsertInOrder((const TInt*)a.Handle(), &AsyncMsgOrder);
	return r;
	}

TInt CMySession::Release(const RMessage2& a)
	{
	TInt req_count = a.Int0();
	TInt avail = iAsyncMsgArray.Count();
	TInt count = Min(req_count, avail);
	TFullName fn;
	TInt r = GetClientName(a, fn);
	testSvr(r==KErrNone);
	testSvr.Printf(_L("Release message from %S req_count=%d count=%d\n"), &fn, req_count, count);
	while (count--)
		{
		TInt mp = (TInt)iAsyncMsgArray[0];
		iAsyncMsgArray.Remove(0);
		RMessage2 m(*(const RMessagePtr2*)&mp);
		TInt arg = m.Int1();
		TInt code = arg ? arg ^ iCompleteIndex : 0;
		r = GetClientName(m, fn);
		testSvr(r==KErrNone);
		testSvr.Printf(_L("Releasing %S arg=%08x index=%08x code=%08x\n"), &fn, arg, iCompleteIndex, code);
		++iCompleteIndex;
		m.Complete(code);
		}
	return KErrNone;
	}

void CMyActiveScheduler::Error(TInt anError) const
//
// Called if any Run() method leaves.
//
	{

	testSvr.Panic(anError,_L("CMyActiveScheduler::Error"));
	}

TInt RDisplay::Open()
	{
	TInt r=CreateSession(KServerName,Version(),8);	// use fixed pool of 8 slots
	if (r==KErrNone)
		r=ShareAuto();
	return r;
	}

TInt RDisplay::OpenUS()
	{
	return CreateSession(KServerName,Version(),8,EIpcSession_Unsharable);	// use fixed pool of 8 slots
	}

TInt RDisplay::OpenS()
	{
	return CreateSession(KServerName,Version(),8,EIpcSession_Sharable);	// use fixed pool of 8 slots
	}

TInt RDisplay::OpenGS()
	{
	return CreateSession(KServerName,Version(),8,EIpcSession_GlobalSharable);	// use fixed pool of 8 slots
	}

TInt RDisplay::OpenNoAsync()
	{

	TInt r=CreateSession(KServerName,Version(),0);	// no asynchronous messages allowed
	if (r==KErrNone)
		r=ShareAuto();
	return r;
	}

TInt RDisplay::OpenDynamic()
	{
	TInt r=CreateSession(KServerName,Version(),-1);	// use global dynamic message pool
	if (r==KErrNone)
		r=ShareAuto();
	return r;
	}

TInt RDisplay::Display(const TDesC& aMessage)
//
// Display a message.
//
	{

	TBuf<0x100> b(aMessage);
	return SendReceive(CMyServer::EDisplay, TIpcArgs(&b));
	}

TInt RDisplay::Read()
//
// Get session to test CSession2::ReadL.
//
	{

	TBuf<0x10> b(_L("Testing read"));
	TBuf<0x10>* bad = (TBuf<0x10> *)(0x30000000);

	return SendReceive(CMyServer::ERead, TIpcArgs(&b, bad));
	}

TInt RDisplay::Write()
//
// Get session to test CSession2::WriteL.
//
	{

	TBuf<0x10> b;
    TBufC<0x10> c; // Bad descriptor - read only
	TBuf<0x10>* bad = (TBuf<0x10> *)(0x30000000);
	TInt r=SendReceive(CMyServer::EWrite, TIpcArgs(&b, &c, bad));
	if (r==KErrNone && b!=_L("It worked!"))
		r=KErrGeneral;
	return r;
	}

TInt RDisplay::Test()
//
// Send a message and wait for completion.
//
	{

	return SendReceive(CMyServer::ETest, TIpcArgs());
	}

TInt RDisplay::Stop()
//
// Stop the server.
//
	{

	return SendReceive(CMyServer::EStop, TIpcArgs());
	}

TVersion RDisplay::Version()
//
// Return the current version.
//
	{

	TVersion v(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	return v;
	}

TInt RDisplay::Echo(TInt aWhat, TInt a0, TInt a1, TInt a2, TInt a3)
	{
	return SendReceive(0x40000000|aWhat, TIpcArgs(a0,a1,a2,a3));
	}

TInt RDisplay::Hold(TInt aOrder, TInt aArg)
	{

	return Send(CMyServer::EHold, TIpcArgs(aOrder, aArg));
	}

void RDisplay::Hold(TRequestStatus& aStatus, TInt aOrder, TInt aArg)
	{

	SendReceive(CMyServer::EHold, TIpcArgs(aOrder, aArg), aStatus);
	}

TInt RDisplay::Release(TInt aCount)
	{

	return SendReceive(CMyServer::ERelease, TIpcArgs(aCount));
	}

TInt RDisplay::CompleteIndex()
	{
	return SendReceive(CMyServer::EGetCompleteIndex, TIpcArgs());
	}

TInt serverThreadEntryPoint(TAny*)
//
// The entry point for the producer thread.
//
	{

	testSvr.Title();
	testSvr.Start(_L("Create CActiveScheduler"));
	CMyActiveScheduler* pR=new CMyActiveScheduler;
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

TInt speedyThreadEntryPoint(TAny* aDisplay)
//
// The entry point for the speed test thread.
//
	{

	RDisplay& t=*(RDisplay*)aDisplay;
	TInt r=t.Test();
	testSpeedy(r==KErrNone);
	speedCount=0;
	client.Signal();
	while ((r=t.Test())==KErrNone)
		speedCount++;
	testSpeedy(r==KErrServerTerminated);
	return(KErrNone);
	}

TInt RunPanicThread(RThread& aThread)
	{
	TRequestStatus s;
	aThread.Logon(s);
	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	aThread.Resume();
	User::WaitForRequest(s);
	User::SetJustInTime(jit);
	return s.Int();
	}

TInt RogueThread1(TAny*)
	{
	// try to kill the kernel
	RMutex mutex;
	TPtrC* p=(TPtrC*)0x30000000;
	mutex.CreateGlobal(*p,EOwnerProcess);	// this should panic the thread
	return KErrNone;
	}

class RMessageT : public RMessage2
	{
public:
	RMessageT(TLinAddr anAddr) { iFunction=0; iHandle=(TInt)anAddr; }
	};

TInt RogueThread2(TAny*)
	{
	// try to kill the kernel
	RMessageT m(0x30000000);
	m.Complete(KErrNone);					// this should panic the thread
	return KErrNone;
	}

TInt RogueThread3(TAny*)
	{
	// try to kill the kernel
	RMessageT m(0x80000000);
	m.Complete(KErrNone);					// this should panic the thread
	return KErrNone;
	}

TInt RogueThread4(TAny*)
	{
	// try to kill the kernel
	RMessageT m(0x800fff00);				// this should be off the end of the kernel heap
	m.Complete(KErrNone);					// this should panic the thread
	return KErrNone;
	}

TInt RogueThread5(TAny*)
	{
	// try to masquerade as the WindowServer
	UserSvr::WsRegisterThread();			// this should panic the thread
	return KErrNone;
	}

TInt RogueThread6(TAny*)
	{
	// try to masquerade as the FileServer
	UserSvr::FsRegisterThread();			// this should panic the thread
	return KErrNone;
	}

TInt RogueThread7(TAny*)
	{
	// try to masquerade as the FileServer
	RChunk myChunk;
	TInt err = myChunk.CreateLocal(4096, 8192);
	if (err == KErrNone)
		UserSvr::RegisterTrustedChunk(myChunk.Handle());	// this should panic the thread
	return err;
	}

void DisplayThreadExitInfo(const RThread& aThread)
	{
	TFullName fn=aThread.FullName();
	TExitType exitType=aThread.ExitType();
	TInt exitReason=aThread.ExitReason();
	TBuf<32> exitCat=aThread.ExitCategory();
	test.Printf(_L("Thread %S exited %d,%d,%S\n"),&fn,exitType,exitReason,&exitCat);
	}

void RogueThreadTest()
	{
	RThread thread;
	TInt r;
	if (HaveVirtMem())
		{
		test.Next(_L("Rogue thread test 1"));
		r=thread.Create(_L("Rogue1"),RogueThread1,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
		test_KErrNone(r);
		RunPanicThread(thread);	// wait for rogue thread to die
		DisplayThreadExitInfo(thread);
		test(thread.ExitType()==EExitPanic);
		test(thread.ExitReason()==ECausedException);
		thread.Close();
		}

	test.Next(_L("Rogue thread test 2"));
	r=thread.Create(_L("Rogue2"),RogueThread2,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test_KErrNone(r);
	RunPanicThread(thread);	// wait for rogue thread to die
	DisplayThreadExitInfo(thread);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitReason()==EBadMessageHandle);
	thread.Close();

	test.Next(_L("Rogue thread test 3"));
	r=thread.Create(_L("Rogue3"),RogueThread3,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test_KErrNone(r);
	RunPanicThread(thread);	// wait for rogue thread to die
	DisplayThreadExitInfo(thread);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitReason()==EBadMessageHandle);
	thread.Close();

	test.Next(_L("Rogue thread test 4"));
	r=thread.Create(_L("Rogue4"),RogueThread4,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test_KErrNone(r);
	RunPanicThread(thread);	// wait for rogue thread to die
	DisplayThreadExitInfo(thread);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitReason()==EBadMessageHandle);
	thread.Close();

	test.Next(_L("Rogue thread test 5"));
	r=thread.Create(_L("Rogue5"),RogueThread5,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test_KErrNone(r);
	RunPanicThread(thread);	// wait for rogue thread to die
	DisplayThreadExitInfo(thread);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitReason()==EAccessDenied);
	thread.Close();

	test.Next(_L("Rogue thread test 6"));
	r=thread.Create(_L("Rogue6"),RogueThread6,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test_KErrNone(r);
	RunPanicThread(thread);	// wait for rogue thread to die
	DisplayThreadExitInfo(thread);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitReason()==EAccessDenied);
	thread.Close();

	test.Next(_L("Rogue thread test 7"));
	r=thread.Create(_L("Rogue7"),RogueThread7,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test_KErrNone(r);
	RunPanicThread(thread);	// wait for rogue thread to die
	DisplayThreadExitInfo(thread);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitReason()==EAccessDenied);
	thread.Close();
	}

TInt SecondClient(TAny* aSession)
	{
	RDisplay d;
	d.SetHandle(TInt(aSession));
	TInt r=d.Display(_L("Second client"));
	if (r!=KErrNone)
		return r;
	r = d.Echo(0,19,31,83,487);
	if (r!=19)
		return -999990;
	r = d.Echo(1,19,31,83,487);
	if (r!=31)
		return -999991;
	r = d.Echo(2,19,31,83,487);
	if (r!=83)
		return -999992;
	r = d.Echo(3,19,31,83,487);
	if (r!=487)
		return -999993;
	r = d.Echo(4,19,31,83,487);
	if (r!=620)
		return -999994;
	r = d.Echo(5,19,31,83,487);
	if (r!=245380)
		return -999995;
	r=d.Read();
	if (r!=KErrNone)
		return r;
	r=d.Write();
	if (r!=KErrNone)
		return r;
	d.Release();
	return KErrNone;
	}

TInt ThirdClient(TAny* aSession)
	{
	RDisplay d;
	d.SetHandle(TInt(aSession));
	TInt r=d.Display(_L("Third client"));
	if (r!=KErrNone)
		return r;
	r=d.Hold();
	if (r!=KErrNone)
		return r;
	r=d.Read();
	if (r!=KErrNone)
		return r;
	r=d.Write();
	if (r!=KErrNone)
		return r;
	return KErrNone;
	}

void TestSecondClient(RSessionBase& aSession)
	{
	RThread t;
	TInt r=t.Create(_L("SecondClient"),SecondClient,0x1000,NULL,(TAny*)aSession.Handle());
	test_KErrNone(r);
	TRequestStatus s;
	t.Logon(s);
	t.Resume();
	User::WaitForRequest(s);
	test_KErrNone(s.Int());
	t.Close();
	}

void TestThirdClient(RSessionBase& aSession)
	{
	RThread t;
	TInt r=t.Create(_L("ThirdClient"),ThirdClient,0x1000,NULL,(TAny*)aSession.Handle());
	test_KErrNone(r);
	TRequestStatus s;
	t.Logon(s);
	t.Resume();
	User::WaitForRequest(s);
	test_KErrNone(s.Int());
	t.Close();
	}

TInt FourthClient(TAny* aSession)
	{
	RDisplay d;
	d.SetHandle(TInt(aSession));
	TInt r=d.Display(_L("Fourth client"));
	if (r!=KErrNone)
		return r;
	d.Close();
	return KErrNone;
	}

void TestSessionClose()
	{
	RDisplay d;
	TInt r=d.Open();
	test_KErrNone(r);
	TRequestStatus msgStat;
	d.Hold(msgStat);
	test(msgStat==KRequestPending);
	RThread t;
	r=t.Create(_L("FourthClient"),FourthClient,0x1000,NULL,(TAny*)d.Handle());
	test_KErrNone(r);
	TRequestStatus s;
	t.Logon(s);
	t.Resume();
	User::WaitForRequest(s);
	test_KErrNone(s.Int());
	t.Close();
	User::After(1000000);
	test(msgStat==KRequestPending);
	}

TInt FifthClient(TAny* aDisplay)
	{
	RDisplay& d=*(RDisplay*)aDisplay;
	TInt r=d.Open();
	if (r!=KErrNone)
		return r;
	r=d.Display(_L("Fifth client"));
	if (r!=KErrNone)
		return r;
	return KErrNone;
	}

void TestZeroShares()
	{
	RDisplay d;
	d.SetHandle(0);
	RThread t;
	TInt r=t.Create(_L("FifthClient"),FifthClient,0x1000,NULL,&d);
	test_KErrNone(r);
	TRequestStatus s;
	t.Logon(s);
	t.Resume();
	User::WaitForRequest(s);
	test_KErrNone(s.Int());
	test(d.Handle()!=0);
	t.Close();
	r=d.Display(_L("Access after last share closed"));
	test_KErrNone(r);
	d.Close();
	}

TInt AttachClient1(TAny* aSession)
	{
	RDisplay d;
	d.SetHandle(TInt(aSession));
	__KHEAP_MARK;
	TInt r=d.Display(_L("Attach client 1"));
	__KHEAP_MARKEND;	// shouldn't need to allocate memory
	return r;
	}

TInt AttachClient2(TAny* aSession)
	{
	RDisplay d;
	d.SetHandle(TInt(aSession));
	TInt r=d.Display(_L("Attach client 2"));
	if (r!=KErrNone)
		return r;
	__KHEAP_MARK;
	r=d.Display(_L("Attach client 2"));
	__KHEAP_MARKEND;	// check no memory was allocated by message send
	return r;
	}

void TestAttach(RSessionBase& aSession)
	{
	test.Next(_L("Test Attach"));
	RThread t;
	TInt r=t.Create(_L("AttachClient1"),AttachClient1,0x1000,NULL,(TAny*)aSession.Handle());
	test_KErrNone(r);
	r=RunPanicThread(t);	// wait for thread to die
	test_KErrNone(r);
	test(t.ExitType()==EExitKill);
	test_KErrNone(t.ExitReason());
	t.Close();

	r=t.Create(_L("AttachClient2"),AttachClient2,0x1000,NULL,(TAny*)aSession.Handle());
	test_KErrNone(r);
	TRequestStatus s;
	t.Logon(s);
	t.Resume();
	User::WaitForRequest(s);
	test_KErrNone(s.Int());
	test(t.ExitType()==EExitKill);
	test_KErrNone(t.ExitReason());
	t.Close();
	}

TInt SixthClient(TAny* aDisplay)
	{
	RDisplay& d=*(RDisplay*)aDisplay;
	TInt r=d.Display(_L("Sixth client"));
	if (r!=KErrNone)
		return r;
	TRequestStatus s;
	User::WaitForRequest(s);
	return KErrNone;
	}

TInt SeventhClient(TAny* aDisplay)
	{
	RDisplay& d=*(RDisplay*)aDisplay;
	TInt r=d.Display(_L("Seventh client"));
	if (r!=KErrNone)
		return r;
	TRequestStatus s;
	User::WaitForRequest(s);
	return KErrNone;
	}

TInt EighthClient(TAny* aDisplay)
	{
	RDisplay& d=*(RDisplay*)aDisplay;
	TInt r=d.Display(_L("Eighth client"));
	if (r!=KErrNone)
		return r;
	TRequestStatus s;
	User::WaitForRequest(s);
	return KErrNone;
	}

TInt NinthClient(TAny* aDisplay)
	{
	RDisplay& d=*(RDisplay*)aDisplay;
	TInt r=d.Display(_L("Ninth client"));
	if (r!=KErrNone)
		return r;
	TRequestStatus s;
	User::WaitForRequest(s);
	return KErrNone;
	}

void TestAsync(RDisplay& aD)
	{
	TInt ci = aD.CompleteIndex();
	TRequestStatus s[128];
	TInt i;
	TInt r;
	for (i=0; i<128; ++i)
		{
		TInt ord = i ^ 0x55;
		TInt arg = i + 1;
		aD.Hold(s[i], ord, arg);
		if (s[i]==KErrServerBusy)
			{
			User::WaitForRequest(s[i]);
			break;
			}
		test(s[i]==KRequestPending);
		}
	r = aD.Release();
	test_KErrNone(r);
	TInt j;
	for (j=0; j<128; ++j)
		{
		if ( (j^0x55) >= i )
			continue;
		TInt code = s[j^0x55].Int();
		TInt expected = ci ^ ((j^0x55)+1);
		if (code != expected)
			{
			test.Printf(_L("j=%02x i=%02x expected=%08x got=%08x\n"),j,j^0x55,expected,code);
			test(0);
			}
		User::WaitForRequest(s[j^0x55]);
		++ci;
		}
	}

void TestSession(RDisplay& aSess)
	{
//
	TInt r;
	test.Next(_L("Test all args passed"));
	test(aSess.Echo(0,3,5,7,11)==3);
	test(aSess.Echo(1,3,5,7,11)==5);
	test(aSess.Echo(2,3,5,7,11)==7);
	test(aSess.Echo(3,3,5,7,11)==11);
	test(aSess.Echo(4,3,5,7,11)==26);
	test(aSess.Echo(5,3,5,7,11)==204);
//
	test.Next(_L("Send to server"));
	r=aSess.Display(_L("First message"));
	test_KErrNone(r);
//
	test.Next(_L("Read"));
	r=aSess.Read();
	test_KErrNone(r);
//
	test.Next(_L("Write"));
	r=aSess.Write();
	test_KErrNone(r);
//
	TestThirdClient(aSess);
	User::After(1000000);
	TestSecondClient(aSess);
	User::After(1000000);
	TestSessionClose();
	User::After(1000000);
	TestZeroShares();
	User::After(1000000);
	TestAttach(aSess);
	User::After(1000000);
	}

void TestUnsharableSession(RDisplay& aSess)
	{
//
	TInt r;
	test.Next(_L("Test all args passed"));
	test(aSess.Echo(0,3,5,7,11)==3);
	test(aSess.Echo(1,3,5,7,11)==5);
	test(aSess.Echo(2,3,5,7,11)==7);
	test(aSess.Echo(3,3,5,7,11)==11);
	test(aSess.Echo(4,3,5,7,11)==26);
	test(aSess.Echo(5,3,5,7,11)==204);
//
	test.Next(_L("Send to server"));
	r=aSess.Display(_L("First message"));
	test_KErrNone(r);
//
	test.Next(_L("Read"));
	r=aSess.Read();
	test_KErrNone(r);
//
	test.Next(_L("Write"));
	r=aSess.Write();
	test_KErrNone(r);
//
	TInt h = aSess.Handle();
	test.Printf(_L("HANDLE %08x\n"), h);
	test((h & KHandleFlagLocal)!=0);
	}

void TestSessionCreateLLeaving()
	{
	SessionCreateLResult=KErrGeneral;

	TInt c=SessionCostructCount;
	TInt d=SessionDestructCount;

	RDisplay t;
	TInt r = t.Open();
	test(r==SessionCreateLResult);

	test(SessionCostructCount==c+1);
	test(SessionDestructCount==d+1);

	SessionCreateLResult=KErrNone;
	}

GLDEF_C TInt E32Main()
//
// Test timers.
//
    {

	test.Title();
	test.Start(_L("Creating client semaphore"));
	TInt r=client.CreateLocal(0);
	test_KErrNone(r);
//
	test.Next(_L("Creating server thread"));
	RThread server;
	r=server.Create(_L("Server"),serverThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test_KErrNone(r);
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
	RDisplay t;
	r = t.OpenNoAsync();
	test_KErrNone(r);
	test.Next(_L("Send to server"));
	r=t.Display(_L("AsyncMsg"));
	test_KErrNone(r);
	TRequestStatus s0;
	t.Hold(s0);
	test(s0==KErrServerBusy);
	t.Close();
//
	test.Next(_L("Test Session::CreateL leaving"));
	TestSessionCreateLLeaving();
//
	test.Next(_L("Async fixed pool"));
	r = t.Open();
	test_KErrNone(r);
	TestAsync(t);
	TestAsync(t);
	t.Close();
	test.Next(_L("Async global pool"));
	r = t.OpenDynamic();
	test_KErrNone(r);
	TestAsync(t);
	TestAsync(t);
	t.Close();
//
	r=t.Open();
	test_KErrNone(r);
//
	TestSession(t);
//
	RDisplay tt;
	r = tt.OpenS();
	test.Printf(_L("OpenS -> %d\n"),r);
	test_KErrNone(r);
	TestSession(tt);
	tt.Close();
//
	r = tt.OpenGS();
	test.Printf(_L("OpenGS -> %d\n"),r);
	test(r==KErrPermissionDenied);
	tt.Close();
//
	r = tt.OpenUS();
	test.Printf(_L("OpenUS -> %d\n"),r);
	test_KErrNone(r);
	TestUnsharableSession(tt);
	tt.Close();
//
	test.Next(_L("Starting speedy client"));
	RThread speedy;
	TRequestStatus speedyStat;
	r=speedy.Create(_L("Speedy"),speedyThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,&t);
	test_KErrNone(r);
	speedy.Logon(speedyStat);
	RThread t1;
	RThread t2;
	RThread t3;
	RThread t4;
	TRequestStatus s1;
	TRequestStatus s2;
	TRequestStatus s3;
	TRequestStatus s4;
	r=t1.Create(_L("SixthClient"),SixthClient,0x1000,NULL,&t);
	test_KErrNone(r);
	t1.Logon(s1);
	r=t2.Create(_L("SeventhClient"),SeventhClient,0x1000,NULL,&t);
	test_KErrNone(r);
	t2.Logon(s2);
	r=t3.Create(_L("EighthClient"),EighthClient,0x1000,NULL,&t);
	test_KErrNone(r);
	t3.Logon(s3);
	r=t4.Create(_L("NinthClient"),NinthClient,0x1000,NULL,&t);
	test_KErrNone(r);
	t4.Logon(s4);
	t1.Resume();
	t2.Resume();
	t3.Resume();
	t4.Resume();
	User::After(1000000);
//
	test.Next(_L("Wait for speedy to start"));
    speedy.SetPriority(EPriorityNormal);
	RThread().SetPriority(EPriorityMuchMore);
	speedy.Resume();
	client.Wait();
//
	test.Printf(_L("Starting speed test...\n"));
    User::After(300000);
    TInt b=speedCount;
    User::After(3000000);
    TInt n=speedCount;
    test.Printf(_L("Count = %d in 1 second\n"),(n-b)/3);
//
	test.Next(_L("Stop server"));
	r=t.Stop();
	test_KErrNone(r);
	User::After(0); // Allow the speed client to die
//
	test.Next(_L("Close extra threads"));
	t1.Kill(0);
	User::WaitForRequest(s1);
	test(t1.ExitType()==EExitKill && s1==KErrNone);
	t2.Kill(0);
	User::WaitForRequest(s2);
	test(t2.ExitType()==EExitKill && s2==KErrNone);
	t3.Kill(0);
	User::WaitForRequest(s3);
	test(t3.ExitType()==EExitKill && s3==KErrNone);
	t4.Kill(0);
	User::WaitForRequest(s4);
	test(t4.ExitType()==EExitKill && s4==KErrNone);
	User::WaitForRequest(speedyStat);
	test(speedy.ExitType()==EExitKill && speedyStat==KErrNone);
//
	test.Next(_L("Close all"));
	t1.Close();
	t2.Close();
	t3.Close();
	t4.Close();
	speedy.Close();
	server.Close();
	client.Close();
//
	test.Next(_L("Close connection"));
	t.Close();
//
	RogueThreadTest();
//
	test.End();
	return(0);
    }

