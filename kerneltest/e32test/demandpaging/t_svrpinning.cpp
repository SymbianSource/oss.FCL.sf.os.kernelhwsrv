// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\demandpaging\t_svrpinning.cpp
// Overview:
// Test the pinning of RMessage descriptor arguments.
// API Information:
// RMessage2, RMessagePtr2, RSessionBase, CSession2, CServer2
// Details:
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__

#include <e32std.h>
#include <e32std_private.h>
#include <e32def.h>
#include <e32def_private.h>
#include <e32test.h>
#include <e32ver.h>
#include <e32panic.h>
#include <dptest.h>
#include <u32hal.h>
#include <hal.h>

const TInt KHeapMinSize=0x1000;
const TInt KHeapMaxSize=0x1000;

const TUint KPageSize = 0x1000;
TInt gPageSize;
TUint gPageMask;
TBool gDataPagingSupport = EFalse;
TBool gProcessPaged;
enum TServerPinning
	{
	EServerDefault,
	EServerPinning,
	EServerNotPinning,
	EServerSetPinningTooLate,
	EServerPinningCount,
	};
TInt gServerPinningState;

class CTestServer : public CServer2
	{
public:
	CTestServer(TInt aPriority);
protected:
	//override the pure virtual functions:
	virtual CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const;
	};


class CTestSession : public CSession2
	{
public:
	enum TTestMode
		{
		EStop,
		ETestRdPinAll,
		ETestRdUnpin3,
		ETestRdUnpin2,
		ETestRdUnpin1,
		ETestRdUnpin0,
		ETestWrPinAll,
		ETestWrPinNone,
		ETestPinOOM,
		ETestPinDefault,
		ETestDeadServer,
		};
//Override pure virtual
	IMPORT_C virtual void ServiceL(const RMessage2& aMessage);
private:
	TInt CheckDesPresent(const RMessage2& aMessage, TUint aArgIndex, TBool aExpected, TBool aWrite);
	TInt CheckArgsPresent(const RMessage2& aMessage, TBool arg0Present, TBool arg1Present, TBool arg2Present, TBool arg3Present, TBool aWrite);
	TBool iClientDied;
	};


class CMyActiveScheduler : public CActiveScheduler
	{
public:
	virtual void Error(TInt anError) const; //override pure virtual error function
	};


class RSession : public RSessionBase
	{
public:
	TInt PublicSendReceive(TInt aFunction, const TIpcArgs &aPtr)
		{
		return (SendReceive(aFunction, aPtr));
		}
	TInt PublicCreateSession(const TDesC& aServer,TInt aMessageSlots)
		{
		return (CreateSession(aServer,User::Version(),aMessageSlots));
		}
	};


_LIT(KServerName,"CTestServer");

TBool UpdateExpected(TBool aExpected)
	{
	if (!gDataPagingSupport												// Data paging is not supported so memory should always be present
		|| gServerPinningState == EServerPinning						// The server is a pinning server.
		/*|| (gServerPinningState == EServerDefault && !gProcessPaged)*/// The process isn't paged and default server policy
		)
		{
		aExpected = ETrue;
		}
	return aExpected;
	}


CTestServer::CTestServer(TInt aPriority)
//
// Constructor - sets name
//
	: CServer2(aPriority)
	{}

CSession2* CTestServer::NewSessionL(const TVersion& aVersion,const RMessage2& /*aMessage*/) const
//
// Virtual fn - checks version supported and creates a CTestSession
//
	{
	TVersion version(KE32MajorVersionNumber,KE32MinorVersionNumber,KE32BuildVersionNumber);
	if (User::QueryVersionSupported(version,aVersion)==EFalse)
		User::Leave(KErrNotSupported);
	CTestSession* newCTestSession = new CTestSession;
	if (newCTestSession==NULL)
		User::Panic(_L("NewSessionL failure"), KErrNoMemory);
	return(newCTestSession);
	}

RSemaphore gSem;
RSemaphore gSem1;

TInt CTestSession::CheckDesPresent(const RMessage2& aMessage, TUint aArgIndex, TBool aExpected, TBool aWrite)
	{
	TRequestStatus clientStat;
	
	if (aExpected)
		{
		RDebug::Printf("  Checking message argument at %d is present", aArgIndex);
		}
	else
		{
		RDebug::Printf("  Checking message argument at %d is not present", aArgIndex);
		// Start watching for client thread death
		RThread clientThread;
		aMessage.Client(clientThread);
		clientThread.Logon(clientStat);
		clientThread.Close();
		}
		
	// Get the length of the descriptor and verify it is as expected.
	TInt length = aMessage.GetDesLength(aArgIndex);
	if (length < KErrNone)
		{
		RDebug::Printf("  Error getting descriptor length %d", length);
		return length;
		}
	if (length < 3)
		{// The incorrect descriptor length.
		RDebug::Printf("  Error - Descriptor length too small %d", length);
		return KErrArgument;
		}
	if (!aWrite)
		{// Now read the descriptor and verify that it is present or not.
		TBuf8<5> des;
		TInt r = aMessage.Read(aArgIndex, des);
		if (r != (aExpected ? KErrNone : KErrBadDescriptor))
			{
			RDebug::Printf("  Unexpected value returned from aMessage.Read:%d", r);
			return KErrGeneral;
			}
		if (r==KErrNone && (des[0] != 'a' || des[1] != 'r' || des[2] != 'g'))
			{// The incorrect descriptor data has been passed.
			RDebug::Printf("  Error in descriptor data is corrupt r %d", r);
			return KErrArgument;
			}
		}
	else
		{// Now write to the maximum length of the descriptor.
		TInt max = aMessage.GetDesMaxLength(aArgIndex);
		if (max < 0)
			{
			RDebug::Printf("  Error getting descriptor max. length %d", max);
			return length;
			}
		HBufC8* argTmp = HBufC8::New(max);
		TPtr8 argPtr = argTmp->Des();
		argPtr.SetLength(max);
		for (TInt i = 0; i < max; i++)
			argPtr[i] = (TUint8)aArgIndex;
		TInt r = aMessage.Write(aArgIndex, argPtr);
		if (r != (aExpected ? KErrNone : KErrBadDescriptor))
			{
			RDebug::Printf("  Unexpected value returned from aMessage.Write:%d", r);
			return KErrGeneral;
			}
		}

	if (!aExpected)
		{// The client should have been killed as the data wasn't present.
		RDebug::Printf("  CheckDesPresent: Waiting for client to die");
		User::WaitForRequest(clientStat);
		iClientDied = ETrue;
		RDebug::Printf("  CheckDesPresent: Client dead");
		}
	return KErrNone;
	}

TInt CTestSession::CheckArgsPresent(const RMessage2& aMessage, TBool arg0Present, TBool arg1Present, TBool arg2Present, TBool arg3Present, TBool aWrite=EFalse)
	{
	// Adjust the pinning status expected based on the default policies.
	// (Must do this before anything else as UpdateExpected() accessed paged global data)
	arg0Present = UpdateExpected(arg0Present);
	arg1Present = UpdateExpected(arg1Present);
	arg2Present = UpdateExpected(arg2Present);
	arg3Present = UpdateExpected(arg3Present);

	// Flush the cache so on paged systems, unpinned paged memory will be discarded.
	DPTest::FlushCache();

	TInt r = User::SetRealtimeState(User::ERealtimeStateOn);
	if (r != KErrNone)
		{
		RDebug::Printf("CheckArgsPresent: Error setting realtime state r = %d", r);
		return r;
		}

	r = CheckDesPresent(aMessage, 0, arg0Present, aWrite);
	if ((r == KErrNone) && !iClientDied)
		r = CheckDesPresent(aMessage, 1, arg1Present, aWrite);
	if ((r == KErrNone) && !iClientDied)
		r = CheckDesPresent(aMessage, 2, arg2Present, aWrite);
	if ((r == KErrNone) && !iClientDied)
		r = CheckDesPresent(aMessage, 3, arg3Present, aWrite);

	User::SetRealtimeState(User::ERealtimeStateOff);

	return r;
	}

EXPORT_C void CTestSession::ServiceL(const RMessage2& aMessage)
//
// Virtual message-handler
//
	{
	TInt r = KErrNone;
	iClientDied = EFalse;
	switch (aMessage.Function())
		{
		case EStop:
			CActiveScheduler::Stop();
			break;
		case ETestRdPinAll:
			r = CheckArgsPresent(aMessage, ETrue, ETrue, ETrue, ETrue);
			break;
		case ETestRdUnpin3:
			r = CheckArgsPresent(aMessage, ETrue, ETrue, ETrue, EFalse);
			break;
		case ETestRdUnpin2:
			r = CheckArgsPresent(aMessage, ETrue, ETrue, EFalse, ETrue);
			break;
		case ETestRdUnpin1:
			r = CheckArgsPresent(aMessage, ETrue,  EFalse,  ETrue, ETrue);
			break;
		case ETestRdUnpin0:
			r = CheckArgsPresent(aMessage, EFalse, ETrue, ETrue, ETrue);
			break;
		case ETestPinDefault:
			r = CheckArgsPresent(aMessage, EFalse, EFalse, EFalse, EFalse);
			break;
		case ETestWrPinAll:
			r = CheckArgsPresent(aMessage, ETrue, ETrue, ETrue, ETrue, ETrue);
			break;
		case ETestWrPinNone:
			r = CheckArgsPresent(aMessage, EFalse, EFalse, EFalse, EFalse, ETrue);
			break;
		default:
			RDebug::Printf("CTestSession::ServiceL Unsupported Function: %d", aMessage.Function());
			r = KErrNotSupported;

		}
 	aMessage.Complete(r);

	// If descriptors aren't as expected then panic so the test will fail.
	if (r != KErrNone)
		User::Panic(_L("ServiceL failure"), r);
	}

// CTestSession funtions

void CMyActiveScheduler::Error(TInt anError) const
//
// Virtual error handler
//
	{
	User::Panic(_L("CMyActiveScheduer::Error"), anError);
	}

TInt ServerThread(TAny* aPinningAttrib)
//
// Passed as the server thread in 2 tests - sets up and runs CTestServer
//
	{
	RTest test(_L("T_SVRPINNING...server"));
	CMyActiveScheduler* pScheduler = new CMyActiveScheduler;
	if (pScheduler == NULL)
		{
		gSem.Signal();
		test(0);
		}

	CActiveScheduler::Install(pScheduler);

	CTestServer* pServer = new CTestServer(0);
	if (pServer == NULL)
		{
		gSem.Signal();
		test(0);
		}

	// Set the pinning attributes of the server.
	TServerPinning pinningAttrib = (TServerPinning)(TInt)aPinningAttrib;
	switch (pinningAttrib)
		{
		case EServerDefault :
		case EServerSetPinningTooLate :
			break;
		case EServerPinning :
			pServer->SetPinClientDescriptors(ETrue);
			break;
		case EServerNotPinning :
			pServer->SetPinClientDescriptors(EFalse);
			break;
		default :
			break;
		}

	//Starting a CServer2 also Adds it to the ActiveScheduler
	TInt r = pServer->Start(KServerName);
	if (r != KErrNone)
		{
		gSem.Signal();
		test(0);
		}

	if (pinningAttrib == EServerSetPinningTooLate)
		{
		pServer->SetPinClientDescriptors(EFalse);
		}

	test.Next(_L("Start ActiveScheduler and signal to client"));
	test.Printf(_L("        There might be something going on beneath this window\n"));
	gSem.Signal();
	CActiveScheduler::Start();
	test.Next(_L("Destroy ActiveScheduler"));
	delete pScheduler;
	delete pServer;

	test.Close();

	return (KErrNone);
	}



#include <e32svr.h>

void dummyFunction(TUint8* /*a0*/, TUint8* /*a1*/, TUint8* /*a2*/, TUint8* /*a3*/, TUint8* /*a4*/, TUint8* /*a5*/)
	{
	}

TInt ClientThread(TAny* aTestMode)
//
// Passed as the first client thread - signals the server to do several tests
//
	{
	// Create the message arguments to be pinned.  Should be one of each type of
	// descriptor to test that the pinning code can correctly pin each type 
	// descriptor data.  Each descriptor's data should in a separate page in 
	// thread's stack to ensure that access to each argument will cause a 
	// separate page fault.

	TUint8 argBufs[KPageSize*(6+2)]; // enough for 6 whole pages
	TUint8* argBuf0 = (TUint8*)(TUintPtr(argBufs+gPageMask)&~gPageMask);
	TUint8* argBuf1 = argBuf0+KPageSize;
	TUint8* argBuf2 = argBuf1+KPageSize;
	TUint8* argBuf3 = argBuf2+KPageSize;
	TUint8* argBuf4 = argBuf3+KPageSize;
	TUint8* argBuf5 = argBuf4+KPageSize;

	argBuf0[0]='a'; argBuf0[1]='r'; argBuf0[2]='g'; argBuf0[3]='0'; argBuf0[4]='\0';
	TPtr8 arg0(argBuf0, 5, 20);

	TBufC8<5>& arg1 = *(TBufC8<5>*)argBuf1;
	new (&arg1) TBufC8<5>((const TUint8*)"arg1");

	argBuf2[0]='a'; argBuf2[1]='r'; argBuf2[2]='g'; argBuf2[3]='1'; argBuf2[4]='\0';
	TPtrC8 arg2((const TUint8*)argBuf2);

	TBuf8<50>& arg3 = *(TBuf8<50>*)argBuf3;
	new (&arg3) TBuf8<50>((const TUint8*)"arg3");

	// For some tests use this 5th and final type of descriptor.
	HBufC8* argTmp = HBufC8::New(7);
	*argTmp = (const TUint8*)"argTmp";
	RBuf8 argTmpBuf(argTmp);

	// Need a couple of extra writable argments
	argBuf4[0]='a'; argBuf4[1]='r'; argBuf4[2]='g'; argBuf4[3]='4'; argBuf4[4]='\0';
	TPtr8 arg4(argBuf4, 5, 20);
	argBuf5[0]='a'; argBuf5[1]='r'; argBuf5[2]='g'; argBuf5[3]='5'; argBuf5[4]='\0';
	TPtr8 arg5(argBuf5, 5, 20);

	RTest test(_L("T_SVRPINNING...client"));
	RSession session;
	TInt r = session.PublicCreateSession(_L("CTestServer"),5);
	if (r != KErrNone)
		{
		gSem.Signal();
		test(0);
		}

	switch((TInt)aTestMode)
		{
		case CTestSession::ETestRdPinAll:
			test.Printf(_L("Test pinning all args\n"));
			r = session.PublicSendReceive(CTestSession::ETestRdPinAll, TIpcArgs(&arg0, &arg1, &arg2, &arg3).PinArgs());
			break;

		case CTestSession::ETestRdUnpin3:
			test.Printf(_L("Read Arg3 unpinned\n"));
			r = session.PublicSendReceive(CTestSession::ETestRdUnpin3, TIpcArgs(&arg0, argTmp, &argTmpBuf, &arg3).PinArgs(ETrue, ETrue, ETrue, EFalse));
			break;

		case CTestSession::ETestRdUnpin2:
			test.Printf(_L("Read Arg2 unpinned\n"));
			r = session.PublicSendReceive(CTestSession::ETestRdUnpin2, TIpcArgs(argTmp, &arg1, &arg2, &arg3).PinArgs(ETrue, ETrue, EFalse,  ETrue));
			break;

		case CTestSession::ETestRdUnpin1:
			test.Printf(_L("Read Arg1 unpinned\n"));
			r = session.PublicSendReceive(CTestSession::ETestRdUnpin1, TIpcArgs(&argTmpBuf, &arg1, &arg2, &arg3).PinArgs(ETrue, EFalse,  ETrue,  ETrue));
			break;

		case CTestSession::ETestRdUnpin0:
			test.Printf(_L("Read Arg0 unpinned\n"));
			r = session.PublicSendReceive(CTestSession::ETestRdUnpin0, TIpcArgs(&arg0, &arg1, &arg2, &arg3).PinArgs(EFalse, ETrue, ETrue, ETrue));
			break;

		case CTestSession::ETestPinDefault:
			test.Printf(_L("Test the default pinning policy of this server\n"));
			r = session.PublicSendReceive(CTestSession::ETestPinDefault, TIpcArgs(&arg0, &arg1, &arg2, argTmp));
			break;

		case CTestSession::ETestWrPinAll:
			test.Printf(_L("Test writing to pinned descriptors\n"));
			r = session.PublicSendReceive(CTestSession::ETestWrPinAll, TIpcArgs(&arg0, &arg3, &arg4, &arg5).PinArgs(ETrue, ETrue, ETrue, ETrue));
			// Verify the index of each argument has been written to each descriptor.
			{
			TUint maxLength = arg0.MaxLength();
			test_Equal(maxLength, arg0.Length());
			TUint j = 0;
			for (; j < maxLength; j++)
				test_Equal(0, arg0[j]);
			maxLength = arg3.MaxLength();
			test_Equal(maxLength, arg3.Length());
			for (j = 0; j < maxLength; j++)
				test_Equal(1, arg3[j]);
			maxLength = arg4.MaxLength();
			test_Equal(maxLength, arg4.Length());
			for (j = 0; j < maxLength; j++)
				test_Equal(2, arg4[j]);
			maxLength = arg5.MaxLength();
			test_Equal(maxLength, arg5.Length());
			for (j = 0; j < maxLength; j++)
				test_Equal(3, arg5[j]);
			}
			break;

		case CTestSession::ETestWrPinNone:
			test.Printf(_L("Test writing to unpinned descriptors\n"));
			r = session.PublicSendReceive(CTestSession::ETestWrPinNone, TIpcArgs(&arg0, &arg3, &arg4, &arg5).PinArgs(EFalse, EFalse, EFalse, EFalse));
			// Verify the index of each argument has been written to each descriptor.
			// Unless this is a pinnning server than the thread will be panicked before we reach there.
			{
			TUint maxLength = arg0.MaxLength();
			test_Equal(maxLength, arg0.Length());
			TUint j = 0;
			for (j = 0; j < maxLength; j++)
				test_Equal(0, arg0[j]);
			maxLength = arg3.MaxLength();
			test_Equal(maxLength, arg3.Length());
			for (j = 0; j < maxLength; j++)
				test_Equal(1, arg3[j]);
			maxLength = arg4.MaxLength();
			test_Equal(maxLength, arg4.Length());
			for (j = 0; j < maxLength; j++)
				test_Equal(2, arg4[j]);
			maxLength = arg5.MaxLength();
			test_Equal(maxLength, arg5.Length());
			for (j = 0; j < maxLength; j++)
				test_Equal(3, arg5[j]);
			}
			break;

		case CTestSession::ETestDeadServer:
			test.Printf(_L("Test pinning to dead server\n"));
			gSem.Signal();
			gSem1.Wait();
			r = session.PublicSendReceive(CTestSession::ETestRdPinAll, TIpcArgs(&arg0, &arg1, &arg2, &arg3).PinArgs());
			break;

		case CTestSession::ETestPinOOM:
			test.Printf(_L("Pinning OOM tests\n"));
			__KHEAP_MARK;
			const TUint KMaxKernelAllocations = 1024;
			TUint i;
			r = KErrNoMemory;
			for (i = 0; i < KMaxKernelAllocations && r == KErrNoMemory; i++)
				{
				__KHEAP_FAILNEXT(i);
				r = session.PublicSendReceive(CTestSession::ETestRdPinAll, TIpcArgs(&arg0, &arg1, &arg2, &arg3).PinArgs());
				__KHEAP_RESET;
				}
			test.Printf(_L("SendReceive took %d tries\n"),i);
			test_KErrNone(r);

			__KHEAP_MARKEND;
			break;
		}

	session.Close();
	test.Close();
	return r;
	}


GLDEF_C TInt E32Main()
	{
	RTest test(_L("T_SVRPINNING...main"));
	test.Title();


	if (DPTest::Attributes() & DPTest::ERomPaging)
		test.Printf(_L("Rom paging supported\n"));
	if (DPTest::Attributes() & DPTest::ECodePaging)
		test.Printf(_L("Code paging supported\n"));
	if (DPTest::Attributes() & DPTest::EDataPaging)
		{
		test.Printf(_L("Data paging supported\n"));
		gDataPagingSupport = ETrue;
		}

	// Determine the data paging attribute.
	RProcess process;	// Default to point to current process.
	gProcessPaged = process.DefaultDataPaged();
	test.Printf(_L("Process data paged %x\n"), gProcessPaged);

	test.Start(_L("Test IPC message arguments pinning"));
	test_KErrNone(HAL::Get(HAL::EMemoryPageSize, gPageSize));
	gPageMask = gPageSize - 1;
	test_Equal(KPageSize, gPageSize);
	// Disable JIT as we are testing panics and don't want the emulator to hang.
	TBool justInTime = User::JustInTime();
	User::SetJustInTime(EFalse);

	TBool exitFailure = EFalse;
	for (	gServerPinningState = EServerDefault; 
			gServerPinningState < EServerSetPinningTooLate && !exitFailure; 
			gServerPinningState++)
		{
		// Create the server with the specified pinning mode.
		switch (gServerPinningState)
			{
			case EServerDefault : 
				test.Next(_L("Test server with default pinning policy"));
				break;
			case EServerPinning : 
				test.Next(_L("Test server with pinning policy"));
				break;
			case EServerNotPinning : 
				test.Next(_L("Test server with not pinning policy"));
				break;
			}
		test_KErrNone(gSem.CreateLocal(0));
		test_KErrNone(gSem1.CreateLocal(0));
		// Create the server thread it needs to have a unpaged stack and heap.
		TThreadCreateInfo serverInfo(_L("Server Thread"), ServerThread, KDefaultStackSize, (TAny*)gServerPinningState);
		serverInfo.SetPaging(TThreadCreateInfo::EUnpaged);
		serverInfo.SetCreateHeap(KHeapMinSize, KHeapMaxSize);
		RThread serverThread;
		test_KErrNone(serverThread.Create(serverInfo));
		TRequestStatus serverStat;
		serverThread.Logon(serverStat);
		serverThread.Resume();

		// Wait for the server to start and then create a session to it.
		gSem.Wait();
		RSession session;
		test_KErrNone(session.PublicCreateSession(_L("CTestServer"),5));
		
		for (	TUint clientTest = CTestSession::ETestRdPinAll; 
				clientTest <= CTestSession::ETestPinDefault && !exitFailure;
				clientTest++)
			{
			// Create the client thread it needs to have a paged stack and heap.
			TThreadCreateInfo clientInfo(_L("Client Thread"), ClientThread, 10 * gPageSize, (TAny*)clientTest);
			clientInfo.SetPaging(TThreadCreateInfo::EPaged);
			clientInfo.SetCreateHeap(KHeapMinSize, KHeapMaxSize);
			RThread clientThread;
			test_KErrNone(clientThread.Create(clientInfo));

			TRequestStatus clientStat;
			clientThread.Logon(clientStat);
			clientThread.Resume();

			// Wait for the client thread to end.
			User::WaitForRequest(clientStat);

			// If all the descriptor arguments were not pinned then the client 
			// thread should have been panicked.
			TBool expectPanic = (clientTest == CTestSession::ETestRdPinAll || 
								clientTest == CTestSession::ETestWrPinAll ||
								clientTest == CTestSession::ETestPinOOM )? 0 : 1;
			expectPanic = !UpdateExpected(!expectPanic);

			TInt exitReason = clientThread.ExitReason();
			TInt exitType = clientThread.ExitType();
			if (expectPanic)
				{
				if (exitType != EExitPanic || 
					exitReason != EIllegalFunctionForRealtimeThread ||
					clientThread.ExitCategory() != _L("KERN-EXEC"))
					{
					test.Printf(_L("Thread didn't panic as expected\n"));
					exitFailure = ETrue;
					}
				}
			else
				{
				if (exitType != EExitKill || exitReason != KErrNone)
					{
					test.Printf(_L("Thread didn't exit gracefully as expected\n"));
					exitFailure = ETrue;
					}
				}
			test(!exitFailure);
			CLOSE_AND_WAIT(clientThread);
			}

		test.Next(_L("Test client sending message to closed server"));
		TThreadCreateInfo clientInfo(_L("Client Thread"), ClientThread, 10 * gPageSize, (TAny*)CTestSession::ETestDeadServer);
		clientInfo.SetPaging(TThreadCreateInfo::EPaged);
		clientInfo.SetCreateHeap(KHeapMinSize, KHeapMaxSize);
		RThread clientThread;
		test_KErrNone(clientThread.Create(clientInfo));
		TRequestStatus clientStat;
		clientThread.Logon(clientStat);
		clientThread.Resume();
		gSem.Wait();
		
		// Signal to stop ActiveScheduler and wait for server to stop.
		session.PublicSendReceive(CTestSession::EStop, TIpcArgs());
		session.Close();
		User::WaitForRequest(serverStat);
		if (serverThread.ExitType() != EExitKill)
			{
			test.Printf(_L("!!Server thread did something bizarre %d\n"), serverThread.ExitReason());
			}

		gSem1.Signal();
		User::WaitForRequest(clientStat);
		test_Equal(EExitKill, clientThread.ExitType());
		test_Equal(KErrServerTerminated, clientThread.ExitReason());

		CLOSE_AND_WAIT(clientThread);
		CLOSE_AND_WAIT(serverThread);
		CLOSE_AND_WAIT(gSem);
		CLOSE_AND_WAIT(gSem1);
		}

	test.Next(_L("Test server setting pinning policy after server started"));
	RThread serverThread;
	test_KErrNone(serverThread.Create(_L("Server Thread"),ServerThread,KDefaultStackSize,KHeapMinSize,KHeapMaxSize, (TAny*)gServerPinningState));
	TRequestStatus serverStat;
	serverThread.Logon(serverStat);
	serverThread.Resume();
	// The server should have panicked with E32USER-CBase 106.
	User::WaitForRequest(serverStat);
	TInt exitReason = serverThread.ExitReason();
	TInt exitType = serverThread.ExitType();
	test_Equal(EExitPanic, exitType);
	test_Equal(ECServer2InvalidSetPin, exitReason);
	if (_L("E32USER-CBase") != serverThread.ExitCategory())
		test(0);
	CLOSE_AND_WAIT(serverThread);

	test.End();

	// Set JIT back to original state.
	User::SetJustInTime(justInTime);

	return (KErrNone);
	}
