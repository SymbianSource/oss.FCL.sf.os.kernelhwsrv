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
// e32test\bench\t_svr.cpp
// Overview:
// Tests and benchmarks the Client/Server architecture of the Symbian platform.
// The client and server run in different threads in the same process. 
// API Information:
// CSession2, CServer2, RSessionBase.
// Details:
// - Create and start a server thread. Verify results are as expected.
// - Open a connection with the server, verify arguments are passed to the server
// and back correctly.
// - Server can read and write messages to/from the client and return verify that 
// the results are as expected.
// - Send dummy messages that the server completes immediately and display how many 
// are completed per second.
// - Stop the server and verify the results are as expected.
// - Verify that the kernel does not crash by completing a message with an invalid 
// handle and verify the client is panicked with EBadMessageHandle.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__

#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32svr.h>
#include <hal.h>
#include <e32math.h>

#include "../mmu/mmudetect.h"

const TInt KHeapSize=0x2000;
const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=0;
const TInt KBuildVersionNumber=1;

_LIT(KServerName,"Display");

_LIT(KReadDesContents, "Testing read");
_LIT(KLengthDesContents, "What-ever");
_LIT(KWriteDesContents, "It worked!");
_LIT(KLocalWriteDesContents, "Local write");

enum TSpeedTest
	{
	ESpeedNull,
	ESpeedUnusedDes,
	ESpeedGetDesLength,
	ESpeedGetMaxDesLength,
	ESpeedReadDes,
	ESpeedWriteDes,
	};

_LIT(KTestNameNull,            "Null");
_LIT(KTestNameUnusedDes,       "UnusedDes");
_LIT(KTestNameGetDesLength,    "GetDesLength");
_LIT(KTestNameGetMaxDesLength, "GetMaxDesLength");
_LIT(KTestNameReadDes,         "ReadDes");
_LIT(KTestNameWriteDes,        "WriteDes");

_LIT(KLitLocal, 			   "Local");
_LIT(KLitRemote, 			   "Remote");

const TDesC* KSpeedTestNames[] =
	{
	&KTestNameNull,
	&KTestNameUnusedDes,
	&KTestNameGetDesLength,
	&KTestNameGetMaxDesLength,
	&KTestNameReadDes,
	&KTestNameWriteDes,
	};

class CMySession : public CSession2
	{
public:
	CMySession();
	~CMySession();
	void DisplayName(const RMessage2& aM);
	virtual void ServiceL(const RMessage2& aMessage);		 	 //pure virtual fns.
private:
	RArray<RMessagePtr2> iAsyncRequests;
	};

class CMyServer : public CServer2
	{
public:
	enum {EDisplay,ERead,EGetDesLength,EGetDesMaxLength,EWrite,ELocalWrite,EDupDes,ENull,ESimpleRead,ESimpleWrite,EStartAsync,ECompleteAsync,EStop};
public:
	CMyServer(TInt aPriority);
	static CMyServer* New(TInt aPriority);
	virtual CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;//Overloading
	};

class CMyActiveScheduler : public CActiveScheduler
	{
public:
	virtual void Error(TInt anError) const;	 //Overloading pure virtual function
	};

class RDisplay : public RSessionBase
	{
public:
	TInt Open(TInt aMessageSlots=0);
	TInt Display(const TDesC& aMessage);
	TInt Read(TInt aArgIndex);
	TInt Write(TInt aArgIndex);
	TInt LocalWrite(TInt aArgIndex);
	TInt TestDesLength(TInt aArgIndex);
	TInt Stop();
	TInt SpeedTest(TSpeedTest);
	TInt Echo(TInt aWhat, TInt a0, TInt a1, TInt a2, TInt a3);
	void StartAsync(TRequestStatus& aStatus);
	void CompleteAsync(TInt aIndex);
	TInt SendBlind();
	TVersion Version();
private:
	TInt SendMessage(TInt aMessage, TInt aArgIndex, TDesC* aDesArg, TInt8 aOffset = 0);
	TInt SendMessage(TInt aMessage, TInt aArgIndex, TDes* aDesArg, TInt8 aOffset = 0);
	TInt SendMessageDup(TInt aMessage, TInt aArgIndex, TInt aArgIndex2, TDes* aDesArgs);
	};

LOCAL_D RTest test(_L("T_SVR"));
LOCAL_D RTest testSvr(_L("T_SVR Server"));
LOCAL_D RTest testSpeedy(_L("T_SVR Speedy"));
LOCAL_D TRequestStatus speedTestStatus;
LOCAL_D RThread serverThread;
LOCAL_D RProcess serverProcess;

//
// Constructor
// 
CMySession::CMySession()
	{}

//
// Destructor
// 
CMySession::~CMySession()
	{
	// Call User::Exit so server shuts down when client disconnects
	User::Exit(KErrNone);
	}

void CMySession::DisplayName(const RMessage2& aM)
//
// Display the client's name.
//
	{

	RThread t;
	TInt r = aM.Client(t);
	testSvr(r==KErrNone);
	TFullName fn(t.FullName());
	t.Close();
	TBuf<256> text;
	r=aM.Read(0,text);
	testSvr(r==KErrNone);
	testSvr.Printf(_L("Session %S\n%S\n"), &fn, &text);
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
	: CServer2(aPriority)
	{}

CSession2* CMyServer::NewSessionL(const TVersion& aVersion, const RMessage2&) const
//
// Create a new client for this server.
//
	{

	TVersion v(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	if (!User::QueryVersionSupported(v,aVersion))
		User::Leave(KErrNotSupported);
	return(new(ELeave) CMySession());
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
	TBuf<0x10> b;
	TDes* des = NULL;

	TInt message = f & 0xff;
	TInt arg = (f >> 8) & 0xff;
	TInt8 offset = (TInt8)((f >> 16) & 0xff);

	switch (message)
		{
	case CMyServer::EDisplay:
		DisplayName(aMessage);
		break;
	case CMyServer::ERead:
		r=aMessage.Read(arg,b,offset);
		if (r==KErrNone && b!=KReadDesContents)
			r=KErrGeneral;
		break;
	case CMyServer::EGetDesLength:
		r=aMessage.GetDesLength(arg);
		break;
	case CMyServer::EGetDesMaxLength:
		r=aMessage.GetDesMaxLength(arg);
		break;
	case CMyServer::EWrite:
		r=aMessage.Write(arg,KWriteDesContents,offset);
		// Test descriptor length updated
		if (r == KErrNone && aMessage.GetDesLength(arg) != 10)
			r = KErrGeneral;
		// Test reading descriptor back again
		if (r == KErrNone)
			r = aMessage.Read(arg,b,offset);
		if (r==KErrNone && b!=KWriteDesContents)
			r = KErrGeneral;
		break;
	case CMyServer::ELocalWrite:
		switch(arg)
			{
			case 0:
				des = (TDes*)aMessage.Int0();
				break;
			case 1:
				des = (TDes*)aMessage.Int1();
				break;
			case 2:
				des = (TDes*)aMessage.Int2();
				break;
			case 3:
				des = (TDes*)aMessage.Int3();
				break;
			default:
				r = KErrGeneral;
			}
		if (r==KErrNone)
			{
			*des = KLocalWriteDesContents;
			r = aMessage.GetDesLength(arg) == 11 ? KErrNone : KErrGeneral;
			}
		if (r==KErrNone)
			r=aMessage.Read(arg,b,0);
		if (r==KErrNone && b!=KLocalWriteDesContents)
			r=KErrGeneral;
		break;
	case CMyServer::EDupDes:
		r=aMessage.Write(arg,KWriteDesContents);
		if (r == KErrNone && aMessage.GetDesLength(offset/* used to pass 2nd arg here*/) != 10)
			r = KErrGeneral;
		if (r == KErrNone)
			r = aMessage.Read(offset,b);
		if (r==KErrNone && b!=KWriteDesContents)
			r = KErrGeneral;
		break;
	case CMyServer::ENull:
		break;
	case CMyServer::ESimpleRead:
		r=aMessage.Read(arg,b);
		break;
	case CMyServer::ESimpleWrite:
		r=aMessage.Write(arg,KWriteDesContents);
		break;
	case CMyServer::EStartAsync:
		r=iAsyncRequests.Append(aMessage);
		if (r == KErrNone)
			return;  // don't complete message
		break;
	case CMyServer::ECompleteAsync:
		{
		TInt index = aMessage.Int0();
		if (iAsyncRequests.Count() <= index)
			r = KErrNotFound;
		else
			{
			RMessagePtr2 asyncMessage = iAsyncRequests[index];
			iAsyncRequests.Remove(index);
			asyncMessage.Complete(KErrNone);
			r=KErrNone;
			}
		}
		break;
	case CMyServer::EStop:
		CActiveScheduler::Stop();
		break;
	default:
		r=KErrNotSupported;
		}
	aMessage.Complete(r);
	}

void CMyActiveScheduler::Error(TInt anError) const
//
// Called if any Run() method leaves.
//
	{

	testSvr.Panic(anError,_L("CMyActiveScheduler::Error"));
	}

TInt RDisplay::Open(TInt aMessageSlots)
//
// Open the server.
//
	{

	return(CreateSession(KServerName,Version(),aMessageSlots));
	}

TInt RDisplay::Display(const TDesC& aMessage)
//
// Display a message.
//
	{

	TBuf<0x10> b(aMessage);
	return(SendReceive(CMyServer::EDisplay,TIpcArgs(&b)));
	}

TInt RDisplay::SendMessage(TInt aMessage, TInt aArgIndex, TDesC* aDesArg, TInt8 aOffset)
	{
	TInt f = aMessage | (aArgIndex << 8) | ((aOffset << 16) & 0x00ff0000);
	TIpcArgs args;
	args.Set(aArgIndex, aDesArg);
	return SendReceive(f, args);
	}

TInt RDisplay::SendMessage(TInt aMessage, TInt aArgIndex, TDes* aDesArg, TInt8 aOffset)
	{
	TInt f = aMessage | (aArgIndex << 8) | ((aOffset << 16) & 0x00ff0000);
	TIpcArgs args;
	args.Set(aArgIndex, aDesArg);
	return SendReceive(f, args);
	}

TInt RDisplay::SendMessageDup(TInt aMessage, TInt aArgIndex1, TInt aArgIndex2, TDes* aDesArg)
	{
	TInt f = aMessage | (aArgIndex1 << 8) | (aArgIndex2 << 16);
	TIpcArgs args;
	args.Set(aArgIndex1, aDesArg);
	args.Set(aArgIndex2, aDesArg);
	return SendReceive(f, args);
	}

TInt RDisplay::Read(TInt aArgIndex)
//
// Test RMessage2::Read
//
	{
	HBufC* buf = HBufC::New(0x10);
	test_NotNull(buf);
	*buf = KReadDesContents;

	TBufC<0x10> des1(KReadDesContents);
	TBuf<0x10> des2(KReadDesContents);
	TPtrC des3(des1);
	TPtr des4((TUint16*)des2.Ptr(), des2.Length(), des2.MaxLength());
	RBuf des5(buf);
	
	// test successful read
	test_Equal(KErrNone, SendMessage(CMyServer::ERead, aArgIndex, &des1));
	test_Equal(KErrNone, SendMessage(CMyServer::ERead, aArgIndex, &des2));
	test_Equal(KErrNone, SendMessage(CMyServer::ERead, aArgIndex, &des3));
	test_Equal(KErrNone, SendMessage(CMyServer::ERead, aArgIndex, &des4));
	test_Equal(KErrNone, SendMessage(CMyServer::ERead, aArgIndex, &des5));

	// test negative offset
	test_Equal(KErrArgument, SendMessage(CMyServer::ERead, aArgIndex, &des1, -1));

	// test bad descriptors
	if (HaveVirtMem())
		{
		test_Equal(KErrBadDescriptor, SendMessage(CMyServer::ERead, aArgIndex, (TDesC*)0x30000000));

		RChunk chunk;
		const TInt KChunkSize = 4096;
		test_KErrNone(chunk.CreateLocal(KChunkSize, KChunkSize));
		test_Equal(KChunkSize, chunk.Size());

		TDesC* ptr = (TDesC*)(chunk.Base() + KChunkSize - 8);
		Mem::Copy(ptr, &des3, 8);
		test_Equal(KErrNone, SendMessage(CMyServer::ERead, aArgIndex, ptr));
		
		ptr = (TDesC*)(chunk.Base() + KChunkSize - 4);
		Mem::Copy(ptr, &des3, 4);
		test_Equal(KErrBadDescriptor, SendMessage(CMyServer::ERead, aArgIndex, ptr));
		
		ptr = (TDesC*)(chunk.Base() + KChunkSize - 12);
		Mem::Copy(ptr, &des4, 12);
		test_Equal(KErrNone, SendMessage(CMyServer::ERead, aArgIndex, ptr));
		
		ptr = (TDesC*)(chunk.Base() + KChunkSize - 8);
		Mem::Copy(ptr, &des4, 8);
		test_Equal(KErrBadDescriptor, SendMessage(CMyServer::ERead, aArgIndex, ptr));
		
		ptr = (TDesC*)(chunk.Base() + KChunkSize - 4);
		Mem::Copy(ptr, &des4, 4);
		test_Equal(KErrBadDescriptor, SendMessage(CMyServer::ERead, aArgIndex, ptr));
		
		chunk.Close();
		
		((TUint32*)&des3)[1] = 0x00001000; // make descriptor point to invalid memory
		((TUint32*)&des4)[2] = 0x00001000; // make descriptor point to invalid memory
		test_Equal(KErrBadDescriptor, SendMessage(CMyServer::ERead, aArgIndex, &des3));
		test_Equal(KErrBadDescriptor, SendMessage(CMyServer::ERead, aArgIndex, &des4));
		}

	delete buf;
	return KErrNone;
	}

TInt RDisplay::TestDesLength(TInt aArgIndex)
//
// Test RMessage2::GetDesLength and RMessage2::GetDesMaxLength
//
	{
	HBufC* buf = HBufC::New(0x10);
	test_NotNull(buf);
	*buf = KLengthDesContents;

	TBufC<0x10> des1(KLengthDesContents);
	TBuf<0x10> des2(KLengthDesContents);
	TPtrC des3(des1);
	TPtr des4((TUint16*)des2.Ptr(), des2.Length(), des2.MaxLength());
	RBuf des5(buf);
	
	// test GetDesLength
	test_Equal(9, SendMessage(CMyServer::EGetDesLength, aArgIndex, &des1));
	test_Equal(9, SendMessage(CMyServer::EGetDesLength, aArgIndex, &des2));
	test_Equal(9, SendMessage(CMyServer::EGetDesLength, aArgIndex, &des3));
	test_Equal(9, SendMessage(CMyServer::EGetDesLength, aArgIndex, &des4));
	test_Equal(9, SendMessage(CMyServer::EGetDesLength, aArgIndex, &des5));

	// test GetDesMaxLength
	test_Equal(KErrBadDescriptor, SendMessage(CMyServer::EGetDesMaxLength, aArgIndex, &des1));
	test_Equal(0x10, SendMessage(CMyServer::EGetDesMaxLength, aArgIndex, &des2));
	test_Equal(KErrBadDescriptor, SendMessage(CMyServer::EGetDesMaxLength, aArgIndex, &des3));
	test_Equal(0x10, SendMessage(CMyServer::EGetDesMaxLength, aArgIndex, &des4));
	test_Equal(0x10, SendMessage(CMyServer::EGetDesMaxLength, aArgIndex, &des5));

	// test bad descriptors
	if (HaveVirtMem())
		{
		test_Equal(KErrBadDescriptor, SendMessage(CMyServer::EGetDesLength, aArgIndex, (TDesC*)0x30000000));

		RChunk chunk;
		const TInt KChunkSize = 4096;
		test_KErrNone(chunk.CreateLocal(KChunkSize, KChunkSize));
		test_Equal(KChunkSize, chunk.Size());

		TDesC* ptr = (TDesC*)(chunk.Base() + KChunkSize - 8);
		Mem::Copy(ptr, &des3, 8);
		test_Equal(9, SendMessage(CMyServer::EGetDesLength, aArgIndex, ptr));
		
		ptr = (TDesC*)(chunk.Base() + KChunkSize - 4);
		Mem::Copy(ptr, &des3, 4);
		test_Equal(KErrBadDescriptor, SendMessage(CMyServer::EGetDesLength, aArgIndex, ptr));
		
		ptr = (TDesC*)(chunk.Base() + KChunkSize - 12);
		Mem::Copy(ptr, &des4, 12);
		test_Equal(9, SendMessage(CMyServer::EGetDesLength, aArgIndex, ptr));
		test_Equal(0x10, SendMessage(CMyServer::EGetDesMaxLength, aArgIndex, ptr));
		
		ptr = (TDesC*)(chunk.Base() + KChunkSize - 8);
		Mem::Copy(ptr, &des4, 8);
		test_Equal(KErrBadDescriptor, SendMessage(CMyServer::EGetDesLength, aArgIndex, ptr));
		test_Equal(KErrBadDescriptor, SendMessage(CMyServer::EGetDesMaxLength, aArgIndex, ptr));
		
		ptr = (TDesC*)(chunk.Base() + KChunkSize - 4);
		Mem::Copy(ptr, &des4, 4);
		test_Equal(KErrBadDescriptor, SendMessage(CMyServer::EGetDesLength, aArgIndex, ptr));
		test_Equal(KErrBadDescriptor, SendMessage(CMyServer::EGetDesMaxLength, aArgIndex, ptr));
		
		chunk.Close();
		}

	delete buf;
	return KErrNone;
	}

TInt RDisplay::Write(TInt aArgIndex)
//
// Get session to test CSession2::WriteL.
//
	{
	HBufC* buf = HBufC::New(0x10);
	test_NotNull(buf);

	TBufC<0x10> des1;
	TBuf<0x10> des2;
	TPtrC des3(des1);
	TPtr des4((TUint16*)des2.Ptr(), des2.Length(), des2.MaxLength());
	RBuf des5(buf);

	// test successful write
	test_Equal(KErrNone, SendMessage(CMyServer::EWrite, aArgIndex, &des2));
	test(des2 == KWriteDesContents);	
	test_Equal(KErrNone, SendMessage(CMyServer::EWrite, aArgIndex, &des4));
	test(des4 == KWriteDesContents);	
	test_Equal(KErrNone, SendMessage(CMyServer::EWrite, aArgIndex, &des5));
	test(des5 == KWriteDesContents);
	test(*buf == KWriteDesContents);

	// test buffer too short
	TBuf<1> small;
	test_Equal(KErrOverflow, SendMessage(CMyServer::EWrite, aArgIndex, &small));

	// test write to constant descriptors
	test_Equal(KErrBadDescriptor, SendMessage(CMyServer::EWrite, aArgIndex, &des1));
	test_Equal(KErrBadDescriptor, SendMessage(CMyServer::EWrite, aArgIndex, &des3));

	// test negative offset
	test_Equal(KErrArgument, SendMessage(CMyServer::EWrite, aArgIndex, &des2, -1));

	// test multiple instances of same descriptor
	for (TInt i = 0 ; i < 4 ; ++i)
		{
		if (i != aArgIndex)
			{
			des2.Zero();
			test_Equal(KErrNone, SendMessageDup(CMyServer::EDupDes, aArgIndex, i, &des2));
			test(des2 == KWriteDesContents);
			}
		}

	// test bad descriptors - do this last as it corrupts the descriptors.
	if (HaveVirtMem())
		{
		((TUint32*)&des3)[1] = 0x00001000; // make descriptor point to invalid memory
		((TUint32*)&des4)[2] = 0x00001000; // make descriptor point to invalid memory
		test_Equal(KErrBadDescriptor, SendMessage(CMyServer::EWrite, aArgIndex, &des3));
		test_Equal(KErrBadDescriptor, SendMessage(CMyServer::EWrite, aArgIndex, &des4));
		}

	delete buf;
	return KErrNone;
	}

TInt RDisplay::LocalWrite(TInt aArgIndex)
	{
	// test local write to descriptor
	TBuf<0x10> des2;
	des2.Zero();
	test_Equal(KErrNone, SendMessage(CMyServer::ELocalWrite, aArgIndex, &des2));
	test(des2 == KLocalWriteDesContents);
	return KErrNone;
	}

void RDisplay::StartAsync(TRequestStatus& aStatus)
	{
	SendReceive(CMyServer::EStartAsync, TIpcArgs(), aStatus);
	}

void RDisplay::CompleteAsync(TInt aIndex)
	{
	test_KErrNone(SendReceive(CMyServer::ECompleteAsync, TIpcArgs(aIndex)));
	}

TInt RDisplay::SendBlind()
	{
	return Send(CMyServer::EStartAsync);
	}

TInt RDisplay::SpeedTest(TSpeedTest aTest)
	{
	TBuf<0x10> des(KReadDesContents);
	
	TInt count = 0;
	TInt r = KErrNone;
	switch (aTest)
		{
		case ESpeedNull:
			while (speedTestStatus == KRequestPending)
				{
				r = SendReceive(CMyServer::ENull, TIpcArgs());
				count++;
				}
			r = (r == KErrNone) ? count : KErrGeneral;
			break;
			
		case ESpeedUnusedDes:
			while (speedTestStatus == KRequestPending)
				{
				r = SendReceive(CMyServer::ENull, TIpcArgs(&des));
				count++;
				}
			r = (r == KErrNone) ? count : KErrGeneral;
			break;
			
		case ESpeedGetDesLength:
			while (speedTestStatus == KRequestPending)
				{
				r = SendReceive(CMyServer::EGetDesLength, TIpcArgs(&des));
				count++;
				}
			r = (r == 12) ? count : KErrGeneral;
			break;
			
		case ESpeedGetMaxDesLength:
			while (speedTestStatus == KRequestPending)
				{
				r = SendReceive(CMyServer::EGetDesMaxLength, TIpcArgs(&des));
				count++;
				}
			r = (r == 0x10) ? count : KErrGeneral;
			break;
			
		case ESpeedReadDes:
			while (speedTestStatus == KRequestPending)
				{
				r = SendReceive(CMyServer::ESimpleRead, TIpcArgs(&des));
				count++;
				}
			r = (r == KErrNone) ? count : KErrGeneral;
			break;
			
		case ESpeedWriteDes:
			while (speedTestStatus == KRequestPending)
				{
				r = SendReceive(CMyServer::ESimpleWrite, TIpcArgs(&des));
				count++;
				}
			r = (r == KErrNone) ? count : KErrGeneral;
			break;
			
		default:
			r = KErrArgument;
		}
	return r;
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

LOCAL_C TInt serverThreadEntryPoint(TAny*)
//
// The entry point for the producer thread.
//
	{
	RThread().SetPriority(EPriorityMore);

	CMyActiveScheduler* pR=new CMyActiveScheduler;
	testSvr(pR!=NULL);
	CActiveScheduler::Install(pR);
	
	CMyServer* pS=CMyServer::New(0);
	testSvr(pS!=NULL);
	
	TInt r=pS->Start(KServerName);
	testSvr(r==KErrNone);
	
	RThread::Rendezvous(KErrNone);
	RProcess::Rendezvous(KErrNone);
	
	CActiveScheduler::Start();
	
	delete pS;
	testSvr.Close();
	return(KErrNone);
	}

LOCAL_C TInt RunPanicThread(RThread& aThread)
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

LOCAL_C TInt RogueThread1(TAny*)
	{
	// try to kill the kernel
	RMutex mutex;
	TPtrC* p=(TPtrC*)0x00001000; // make descriptor point to invalid memory
	mutex.CreateGlobal(*p,EOwnerProcess);	// this should panic the thread
	return KErrNone;
	}

class RMessageT : public RMessage2
	{
public:
	RMessageT(TLinAddr anAddr) { iFunction=0; iHandle=(TInt)anAddr; }
	};

LOCAL_C TInt RogueThread2(TAny*)
	{
	// try to kill the kernel
	RMessageT m(0x30000000);
	m.Complete(KErrNone);					// this should panic the thread
	return KErrNone;
	}

LOCAL_C TInt RogueThread3(TAny*)
	{
	// try to kill the kernel
	RMessageT m(0x80000000);
	m.Complete(KErrNone);					// this should panic the thread
	return KErrNone;
	}

LOCAL_C TInt RogueThread4(TAny*)
	{
	// try to kill the kernel
	RMessageT m(0x800fff00);				// this should be off the end of the kernel heap
	m.Complete(KErrNone);					// this should panic the thread
	return KErrNone;
	}

LOCAL_C void DisplayThreadExitInfo(const RThread& aThread)
	{
	TFullName fn=aThread.FullName();
	TExitType exitType=aThread.ExitType();
	TInt exitReason=aThread.ExitReason();
	TBuf<32> exitCat=aThread.ExitCategory();
	test.Printf(_L("Thread %S exited %d,%d,%S\n"),&fn,exitType,exitReason,&exitCat);
	}

LOCAL_C void RogueThreadTest()
	{
	test.Start(_L("Rogue thread tests"));
	
	RThread thread;
	TInt r;
	if (HaveVirtMem())
		{
		test.Next(_L("Rogue thread test 1"));
		r=thread.Create(_L("Rogue1"),RogueThread1,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
		test(r==KErrNone);
		RunPanicThread(thread);
		DisplayThreadExitInfo(thread);
		test(thread.ExitType()==EExitPanic);
		test(thread.ExitReason()==ECausedException);
		CLOSE_AND_WAIT(thread);
		}

	test.Next(_L("Rogue thread test 2"));
	r=thread.Create(_L("Rogue2"),RogueThread2,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	RunPanicThread(thread);
	DisplayThreadExitInfo(thread);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitReason()==EBadMessageHandle);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Rogue thread test 3"));
	r=thread.Create(_L("Rogue3"),RogueThread3,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	RunPanicThread(thread);
	DisplayThreadExitInfo(thread);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitReason()==EBadMessageHandle);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Rogue thread test 4"));
	r=thread.Create(_L("Rogue4"),RogueThread4,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	RunPanicThread(thread);
	DisplayThreadExitInfo(thread);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitReason()==EBadMessageHandle);
	CLOSE_AND_WAIT(thread);
	test.End();
	}

class RMySession : public RSessionBase
	{
public:
	TInt Connect(RServer2 aSrv,TRequestStatus& aStat)
		{return CreateSession(aSrv,TVersion(),1,EIpcSession_Unsharable,0,&aStat);}
	void SendTestMessage(TRequestStatus& aStat)
		{SendReceive(0,TIpcArgs(1,2,3,4), aStat);}
	};

TInt BadServerThread(TAny*)
	{
	RServer2 srv;
	RMySession sess;
	TRequestStatus stat;
	RMessage2 msg;
	RMessage2* badMsg = 0;
	TInt r;
	
	// Test receiving connect message to bad address
	
	r = srv.CreateGlobal(KNullDesC);
	if (r != KErrNone)
		return r;
	r = sess.Connect(srv,stat);
	if (r != KErrNone)
		return r;
	srv.Receive(*badMsg);
	srv.Close();
	User::WaitForRequest(stat);
	if (stat != KErrServerTerminated)
		return KErrGeneral;
	sess.Close();

	// Test receiving normal message to bad address
	
	r = srv.CreateGlobal(KNullDesC);
	if (r != KErrNone)
		return r;
	r = sess.Connect(srv,stat);
	if (r != KErrNone)
		return r;
	srv.Receive(msg);
	msg.Complete(KErrNone);
	User::WaitForRequest(stat);
	if (stat != KErrNone)
		return KErrGeneral;
	sess.SendTestMessage(stat);
	srv.Receive(*badMsg);
	srv.Close();
	User::WaitForRequest(stat);
	if (stat != KErrServerTerminated)
		return KErrGeneral;
	sess.Close();

	return 23;
	}

void BadServerTest()
	{
	// This tests the current behaviour of RServer2::Receive when passed a dodgy RMessage2 pointer,
	// which is to ingore exceptions and not panic the server thread.

	RThread thread;
	TInt r=thread.Create(_L("BadServer"),BadServerThread,KDefaultStackSize,NULL,NULL);
	test(r==KErrNone);
	TRequestStatus status;
	thread.Logon(status);
	thread.Resume();
	User::WaitForRequest(status);
	test(thread.ExitType()==EExitKill);
	test(thread.ExitReason()==23);
	CLOSE_AND_WAIT(thread);
	}

void StartServerThread()
	{
	TInt r=serverThread.Create(_L("Server"),serverThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	
	TRequestStatus status;
	serverThread.Rendezvous(status);
	serverThread.Resume();
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	}

void WaitServerThreadDeath()
	{
	TRequestStatus status;
	serverThread.Logon(status);
	User::WaitForRequest(status);
	test(status == KErrNone);
	test(serverThread.ExitReason() == EExitKill);
	CLOSE_AND_WAIT(serverThread);
	}

void StartServerProcess()
	{
	TInt r=serverProcess.Create(_L("t_svr"),_L("slave"));
	test(r==KErrNone);

	TRequestStatus status;
	serverProcess.Rendezvous(status);
	serverProcess.Resume();

	User::WaitForRequest(status);
	test(status == KErrNone);
	}

void WaitServerProcessDeath()
	{
	TRequestStatus status;
	serverProcess.Logon(status);
	User::WaitForRequest(status);
	test(status == KErrNone);
	test(serverProcess.ExitReason() == EExitKill);
	CLOSE_AND_WAIT(serverProcess);
	}

void RunSpeedTest(RDisplay& aSess, TBool aLocal, TSpeedTest aTest)
	{
    User::After(300000);

	RTimer timer;
	test(timer.CreateLocal() == KErrNone);
	
    timer.After(speedTestStatus, 300000);
	TInt r = aSess.SpeedTest(aTest);
	User::WaitForRequest(speedTestStatus);
	test(r > KErrNone);

    timer.After(speedTestStatus, 3000000);
	TUint startCount = User::FastCounter();
	r = aSess.SpeedTest(aTest);
	TUint endCount = User::FastCounter();
	User::WaitForRequest(speedTestStatus);
	test(r > KErrNone);

	timer.Close();

	const TDesC* loc = aLocal ? &KLitLocal : &KLitRemote;
	const TDesC* name = KSpeedTestNames[aTest];
	
	TInt countFreq = 0;
	test(HAL::Get(HAL::EFastCounterFrequency, countFreq) == KErrNone);

	TBool fcCountsUp = 0;
	test(HAL::Get(HAL::EFastCounterCountsUp, fcCountsUp) == KErrNone);

	TInt countDiff = fcCountsUp ? (endCount - startCount) : (startCount - endCount);
	TReal elapsedTimeUs = (1000000.0 * countDiff) / countFreq;
	TReal time = elapsedTimeUs / r;
	
    test.Printf(_L("%S, %S, %f\n"), loc, name, time);
	}

void RunAllSpeedTests(TBool aLocal)
	{
	RDisplay t;
	test(t.Open() == KErrNone);
	
	RunSpeedTest(t, aLocal, ESpeedNull);
	RunSpeedTest(t, aLocal, ESpeedUnusedDes);
	RunSpeedTest(t, aLocal, ESpeedGetDesLength);
	RunSpeedTest(t, aLocal, ESpeedGetMaxDesLength);
	RunSpeedTest(t, aLocal, ESpeedReadDes);
	RunSpeedTest(t, aLocal, ESpeedWriteDes);
	
	t.Close();
	}

const TInt KMaxRequests = 20;
const TInt KSoakIterations = 1000;

void DoTestMultipleOutstandingRequests(RDisplay t)
	{
	TRequestStatus status[KMaxRequests];
	
	test.Start(_L("Test multiple async requests\n"));
	
	test.Next(_L("Test multiple async requests, complete in order\n"));
	TInt i;
	for (i = 0 ; i < KMaxRequests ; ++i)
		{
		t.StartAsync(status[i]);
		test_Equal(KRequestPending, status[i].Int());
		}
	for (i = 0 ; i < KMaxRequests ; ++i)
		{
		t.CompleteAsync(0);  // complete first remaining async request
		User::WaitForAnyRequest();
		test_KErrNone(status[i].Int());
		}

	test.Next(_L("Test multiple async requests, complete in reverse order\n"));
	for (i = 0 ; i < KMaxRequests ; ++i)
		{
		t.StartAsync(status[i]);
		test_Equal(KRequestPending, status[i].Int());
		}
	for (i = KMaxRequests - 1 ; i >= 0 ; --i)
		{
		t.CompleteAsync(i);  // complete last remaining async request
		User::WaitForAnyRequest();
		test_KErrNone(status[i].Int());
		}

	test.Next(_L("Soak test multiple async requests, complete in random order\n"));
	for (i = 0 ; i < KMaxRequests ; ++i)
		{
		t.StartAsync(status[i]);
		test_Equal(KRequestPending, status[i].Int());
		}
	for (TInt j = 0 ; j < KSoakIterations ; ++j)
		{
		// complete random async request
		i = Math::Random() % KMaxRequests;
		t.CompleteAsync(i);
		User::WaitForAnyRequest();
		
		// find which one got completed
		for (i = 0 ; i < KMaxRequests ; ++i)
			if (status[i] != KRequestPending)
				break;
		test(i < KMaxRequests);
		test_KErrNone(status[i].Int());
		
		// re-start request
		t.StartAsync(status[i]);
		test_Equal(KRequestPending, status[i].Int());

		if (j % 100 == 0)
			test.Printf(_L("."));
		}
	test.Printf(_L("\n"));
	for (i = 0 ; i < KMaxRequests ; ++i)
		{
		t.CompleteAsync(0);
		User::WaitForAnyRequest();
		}
	for (i = 0 ; i < KMaxRequests ; ++i)
		test_KErrNone(status[i].Int());

	test.End();
	}

void TestMultipleOutstandingRequests()
	{
	TRequestStatus status[2];

	test.Next(_L("Test zero async message slots\n"));
	RDisplay t;
	StartServerThread();
	test_KErrNone(t.Open(0));
	t.StartAsync(status[0]);
	User::WaitForAnyRequest();
	test_Equal(KErrServerBusy, status[0].Int());
	t.Close();
	WaitServerThreadDeath();

	test.Next(_L("Test one async request\n"));
	StartServerThread();
	test_KErrNone(t.Open(1));
	t.StartAsync(status[0]);
	t.StartAsync(status[1]);
	User::WaitForAnyRequest();
	test_Equal(KRequestPending, status[0].Int());
	test_Equal(KErrServerBusy, status[1].Int());
	User::After(1000);
	test_Equal(KRequestPending, status[0].Int());
	t.CompleteAsync(0);
	User::WaitForAnyRequest();
	test_KErrNone(status[0].Int());

	test.Next(_L("Test one async request, again\n"));
	t.StartAsync(status[0]);
	test_Equal(KRequestPending, status[0].Int());
	t.CompleteAsync(0);
	User::WaitForAnyRequest();
	test_KErrNone(status[0].Int());
	t.Close();
	WaitServerThreadDeath();

	test.Next(_L("Test multiple async requests using dedicated message slots\n"));
	StartServerThread();
	test_KErrNone(t.Open(KMaxRequests));
	DoTestMultipleOutstandingRequests(t);
	t.Close();
	WaitServerThreadDeath();

	test.Next(_L("Test multiple async requests using global pool\n"));
	StartServerThread();
	test_KErrNone(t.Open(-1));
	DoTestMultipleOutstandingRequests(t);
	t.Close();
	WaitServerThreadDeath();	
	}

void CheckNoOutstandingSignals()
	{
	RTimer timer;
	test_KErrNone(timer.CreateLocal());
	TRequestStatus status;
	timer.After(status, 1000);
	User::WaitForAnyRequest();
	test_KErrNone(status.Int());
	timer.Close();
	}

void TestBlindMessages()
	{
	test.Start(_L("Test sending blind messages to server"));
	CheckNoOutstandingSignals();
	
	RDisplay t;
	StartServerThread();
	test_KErrNone(t.Open(0));
	test_Equal(KErrServerBusy, t.SendBlind());
	t.Close();
	WaitServerThreadDeath();
	
	StartServerThread();
	test_KErrNone(t.Open(2));
	test_KErrNone(t.SendBlind());
	test_KErrNone(t.SendBlind());
	test_Equal(KErrServerBusy, t.SendBlind());
	t.CompleteAsync(0);
	test_KErrNone(t.SendBlind());
	test_Equal(KErrServerBusy, t.SendBlind());
	t.CompleteAsync(0);
	t.CompleteAsync(0);
	test_KErrNone(t.SendBlind());
	test_KErrNone(t.SendBlind());
	test_Equal(KErrServerBusy, t.SendBlind());
	t.CompleteAsync(0);
	t.CompleteAsync(0);
	t.Close();
	WaitServerThreadDeath();
	
	CheckNoOutstandingSignals();
	test.End();
	}

void RunCommonServerTests(TBool /*aSameProcess*/)
	{
	test.Start(_L("Connect to server"));
	RDisplay t;
	TInt r=t.Open();
	test(r==KErrNone);

	test.Next(_L("Test all args passed"));
	test(t.Echo(0,3,5,7,11)==3);
	test(t.Echo(1,3,5,7,11)==5);
	test(t.Echo(2,3,5,7,11)==7);
	test(t.Echo(3,3,5,7,11)==11);
	test(t.Echo(4,3,5,7,11)==26);
	test(t.Echo(5,3,5,7,11)==204);

	test.Next(_L("Send to server"));
	r=t.Display(_L("First message"));
	test(r==KErrNone);

	for (TInt i = 0 ; i < 4 ; ++i)
		{
		test.Start(_L("Testing passing descriptors"));
		test.Printf(_L("Descriptor passed as arg %d\n"), i);
		
		test.Next(_L("Read"));
		r=t.Read(i);
		test(r==KErrNone);

		test.Next(_L("GetDesLength, GetDesMaxLength"));
		r=t.TestDesLength(i);
		test(r==KErrNone);

		test.Next(_L("Write"));
		r=t.Write(i);
		test(r==KErrNone);

		/*
		This is now explicitly not supported! 
		if (aSameProcess)
			{
			test.Next(_L("Local write"));
			r=t.LocalWrite(i);
			test(r==KErrNone);
			}
		*/
		
		test.End();
		}

	test.Next(_L("Test RServer2::Receive to dodgy pointer"));
	BadServerTest();

	t.Close();

	test.End();
	}

void RunTests()
//
// Test timers.
//
    {
	test.Title();

	// Turn off evil lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	test.Start(_L("Running tests for server in remote process"));

	StartServerProcess();
	RunCommonServerTests(EFalse);
	WaitServerProcessDeath();
	
	test.Next(_L("Running tests for server in same process"));
	StartServerThread();
	RunCommonServerTests(ETrue);
	WaitServerThreadDeath();

	test.Next(_L("Running rogue thread test"));
	RogueThreadTest();
	
	test.Next(_L("Test multiple outstanding requests"));
	TestMultipleOutstandingRequests();
	
	test.Next(_L("Test sending blind async requests"));
	TestBlindMessages();	
	
#ifndef _DEBUG
	test.Next(_L("Running speed tests"));
    test.Printf(_L("Server process, Test name, Time (uS)\n"));
	
	StartServerProcess();
	RunAllSpeedTests(EFalse);
	WaitServerProcessDeath();
	
	StartServerThread();
	RunAllSpeedTests(ETrue);	
	WaitServerThreadDeath();
#endif

	test.End();
    }


GLDEF_C TInt E32Main()
	{
	if (User::CommandLineLength() == 0)
		{
		RunTests();
		return KErrNone;
		}
	else
		return serverThreadEntryPoint(NULL);
	}
