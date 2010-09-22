// Copyright (c) 1995-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\active\t_messge.cpp
// Overview:
// Test the RMessage2 and RMessagePtr2 classes. 
// API Information:
// RMessage2, RMessagePtr2, RSessionBase, CSession2, CServer2
// Details:
// - Verify copy constructor and assignment operator of Rmessage2 is implemented correctly.
// - Verify default constructor, copy constructor and assignment operator of RMessagePtr2 
// is implemented correctly, construct RMessagePtr2 from pointer to RMessage2 and verify
// that it is constructed successfully.
// - Create client and server threads, open a session with specified number of message slots
// - Send integer message arguments from client and test that the server receives the
// same integers.
// - Send pointers as message arguments from client and test that the server receives the
// same pointers. 
// - Test Client() method of RMessage2 and verify it opened a handle to the client thread
// - Test ClientProcessFlags() method of RMessagePtr2 and verify it matches to a known flag
// - Test ClientIsRealtime() method of RMessagePtr2 and check that it isn't true
// - Complete several messages using RMessage2::Complete and verify the client gets the
// error codes back correctly.
// - Complete several messages using RMessagePtr2::Complete and verify the client gets the
// error codes back correctly.
// - Check RMessage construction from RMessagePtr2 is successful.
// - Check creation of other sessions to the same server is successful.
// - Check the server return KErrServerBusy when client has more outstanding requests than
// available message slots.
// - Send stop signal and check that the server has stopped as expected.
// - Check the client and server have exited as expected.
// - Create client and server threads, open a connection to the server and verify that
// - Server is panicked when completing a message twice.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32std.h>
#include <u32std.h>
#include <e32std_private.h>
#include <e32test.h>
#include <e32ver.h>
#include <e32panic.h>

const TInt KHeapMinSize=0x1000;
const TInt KHeapMaxSize=0x1000;

_LIT(KServerName,"CTestServer");

class CTestServer : public CServer2
	{
public:
	IMPORT_C CTestServer(TInt aPriority);
	enum TPanicType{
		EInt0Error=1, EInt1Error, EInt2Error, EInt3Error,
		EPtr0Error, EPtr1Error, EPtr2Error, EPtr3Error,
		ECreateNameError, ENewSessionError,
		EClientError, EClientProcessFlagsError,
		EClientIsRealtimeError
		};
	static void Panic(TPanicType aReason);
protected:
	//override the pure virtual functions:
	IMPORT_C virtual CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const;
	};


class CTestSession : public CSession2
	{
public:
	enum {EStop,ETestInt,ETestPtr,ETestClient,ETestClientProcessFlags,ETestClientIsRealtime,ETestComplete,ETestPtrComplete,ETestCompletePanic,ETestOtherSession,ETestCompleteAfter,ETestMessageConstruction, 
		ETestRMessagePtr2LeavingInterface, ETestKillCompletePanic};
//Override pure virtual
	IMPORT_C virtual void ServiceL(const RMessage2& aMessage);
private:
	void TestInt(const RMessage2& aMessage);
	void TestPtr(const RMessage2& aMessage);
	void TestClient(const RMessage2& aMessage);
	void TestClientProcessFlags(const RMessage2& aMessage);
	void TestClientIsRealtime(const RMessage2& aMessage);
	void TestComplete(const RMessage2& aMessage);
	void TestPtrComplete(const RMessage2& aMessage);
	void TestCompletePanic();
	TInt TestMessageConstruction(const RMessage2& aMessage);
	TInt TestRMessagePtr2LeavingInterface(const RMessage2& aMessage);
	//	
	TInt count1;//initially ==0
	RMessage2 messages[5];//Used in TestComplete()
//
	TInt count2;//initially ==0
	RMessagePtr2 messagePtrs[5];//User in TestPtrComplete()
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
	void PublicSendReceive(TInt aFunction, const TIpcArgs &aPtr, TRequestStatus& aStatus)
		{
		SendReceive(aFunction, aPtr, aStatus);
		}
	TInt PublicCreateSession(const TDesC& aServer,TInt aMessageSlots)
		{
		return (CreateSession(aServer,User::Version(),aMessageSlots));
		}
	};

// CTestServer functions

EXPORT_C CTestServer::CTestServer(TInt aPriority) 
//
// Constructor - sets name
//
	: CServer2(aPriority)
	{}

EXPORT_C CSession2* CTestServer::NewSessionL(const TVersion& aVersion,const RMessage2& /*aMessage*/) const
//
// Virtual fn - checks version supported and creates a CTestSession
//
	{
	TVersion version(KE32MajorVersionNumber,KE32MinorVersionNumber,KE32BuildVersionNumber);
	if (User::QueryVersionSupported(version,aVersion)==EFalse)
		User::Leave(KErrNotSupported);
	CTestSession* newCTestSession = new CTestSession;
	if (newCTestSession==NULL)
		Panic(ENewSessionError);
	return(newCTestSession);
	}

RThread clientThread, serverThread;

void CTestServer::Panic(TPanicType aReason)	  //static function
	{
	clientThread.Kill(KErrNone);
	User::Panic(_L("CTestServer"),aReason);
	}

RSession session, otherSession;
RSemaphore sem;

const TInt KTestInt[] = {-3866,30566,0,200};

void CTestSession::TestInt(const RMessage2& aMessage)
//
// Tests to see that the correct Int0/1/2/3 have been received
//
	{
	if (aMessage.Int0()!=KTestInt[0])
		CTestServer::Panic(CTestServer::EInt0Error);
	if (aMessage.Int1()!=KTestInt[1])
		CTestServer::Panic(CTestServer::EInt1Error);
	if (aMessage.Int2()!=KTestInt[2])
		CTestServer::Panic(CTestServer::EInt2Error);
	if (aMessage.Int3()!=KTestInt[3])
		CTestServer::Panic(CTestServer::EInt3Error);
	}

const TAny* KTestPtr[]={&clientThread, &serverThread, &session, &sem};

void CTestSession::TestPtr(const RMessage2& aMessage)
//
// Tests to see that the correct Ptr0/1/2/3 have been received
//
	{
	if (aMessage.Ptr0()!=KTestPtr[0])
		CTestServer::Panic(CTestServer::EPtr0Error);
	if (aMessage.Ptr1()!=KTestPtr[1])
		CTestServer::Panic(CTestServer::EPtr1Error);
	if (aMessage.Ptr2()!=KTestPtr[2])
		CTestServer::Panic(CTestServer::EPtr2Error);
	if (aMessage.Ptr3()!=KTestPtr[3])
		CTestServer::Panic(CTestServer::EPtr3Error);
	}

void CTestSession::TestClient(const RMessage2& aMessage)
//
// Tests Client()
//
	{

	TFullName n=RProcess().Name();
	n+=_L("::Client Thread");
	RThread t;
	TInt r=aMessage.Client(t);
	if (r!=KErrNone)
		User::Invariant();
	if (t.FullName().CompareF(n)!=0)
		{
		clientThread.Kill(0);
		CTestServer::Panic(CTestServer::EClientError);
		}
	t.Close();
	}


void CTestSession::TestClientProcessFlags(const RMessage2& aMessage)
//
// Tests ClientProcessFlags()
//
	{
	TBool justInTime=User::JustInTime();
	TUint flags=aMessage.ClientProcessFlags();

	if ((flags&KProcessFlagJustInTime) && justInTime)
		{
		return; //OK
		}
	else if ((flags&KProcessFlagJustInTime) || justInTime)
		{
		//mismatch
		clientThread.Kill(0);
		CTestServer::Panic(CTestServer::EClientProcessFlagsError);
		}
	}

void CTestSession::TestClientIsRealtime(const RMessage2& aMessage)
//
// Tests ClientIsRealtime()
//
	{
	if(aMessage.ClientIsRealtime())
		{
		clientThread.Kill(0);
		CTestServer::Panic(CTestServer::EClientIsRealtimeError);
		}
	}


void CTestSession::TestComplete(const RMessage2& aMessage)
//
// Stores messages up then Completes in reverse order 
//
	{
	messages[count1] = aMessage;
	if (++count1==5)
		for(count1=4; count1>=0; count1--)
			messages[count1].Complete(5-count1);  //Complete with different 'error messages'
	}

void CTestSession::TestPtrComplete(const RMessage2& aMessage)
//
// Stores messages up as RMessagePtrs then Completes in reverse order
// Also tests RMessage2::MessagePtr()
//
	{

	messagePtrs[count2] = aMessage;
	__ASSERT_ALWAYS(!messagePtrs[count2].IsNull(), User::Invariant());
	if (++count2==5)
		for(count2=4; count2>=0; count2--)
			{
			messagePtrs[count2].Complete(10-count2*2);
			__ASSERT_ALWAYS(messagePtrs[count2].IsNull(), User::Invariant());
			}
	}



TInt CTestSession::TestMessageConstruction(const RMessage2& aMessage)
	{
	RMessage2 m2((RMessagePtr2&)aMessage);
	if(m2!=aMessage)
		return KErrGeneral;
	return KErrNone;
	}

TInt CTestSession::TestRMessagePtr2LeavingInterface(const RMessage2& aMessage)
	{
	RThread thread1, thread2;
	TProcessPriority priority = EPriorityForeground;
	
	//Check that both Client and ClientL methods return the same thread and error code.
	if (aMessage.Client(thread1)) 
		return KErrGeneral;

	TRAPD(r1, aMessage.ClientL(thread2))
	if (KErrNone != r1)	
		{
		thread1.Close();
		return r1;
		}
	if (thread1.Id() != thread2.Id()) 
		{
		thread1.Close();
		thread2.Close();
		return KErrBadHandle;
		}

	//Check that both SetPriority & SetPriorityL methods return the same error code 
	r1 = aMessage.SetProcessPriority(priority);
	TRAPD(r2, aMessage.SetProcessPriorityL(priority))

	thread1.Close();
	thread2.Close();
	if (r1 != r2)
		return KErrGeneral;

	return KErrNone;
	}

EXPORT_C void CTestSession::ServiceL(const RMessage2& aMessage)
//
// Virtual message-handler
//
	{
	TInt r=KErrNone;
	switch (aMessage.Function())
		{
		case EStop:
			CActiveScheduler::Stop();
			break;		
		case ETestInt:
			TestInt(aMessage);
			break;
		case ETestPtr:
			TestPtr(aMessage);
			break;
		case ETestClient:
			TestClient(aMessage);
			break;
		case ETestClientProcessFlags:
			TestClientProcessFlags(aMessage);
			break;
		case ETestClientIsRealtime:
			TestClientIsRealtime(aMessage);
			break;
		case ETestComplete:
			TestComplete(aMessage);
			return;
		case ETestPtrComplete:
			TestPtrComplete(aMessage);
			return;
		case ETestCompletePanic:
			aMessage.Complete(KErrNone);
			break;
		case ETestOtherSession:
			break;
		case ETestCompleteAfter:
			User::After(7000000);
			break;
		case ETestMessageConstruction:
			r=TestMessageConstruction(aMessage);
			break;
		case ETestRMessagePtr2LeavingInterface:
			r=TestRMessagePtr2LeavingInterface(aMessage);
			break;
		case ETestKillCompletePanic:
			aMessage.Complete(KErrNone);
			aMessage.Panic(_L("Testing Panic"),0xFF); //This will panic the server!
			break;
		default:
			r=KErrNotSupported;

		}						  
 	aMessage.Complete(r);
	
	}

// CTestSession funtions

void CMyActiveScheduler::Error(TInt anError) const
//
// Virtual error handler
//
	{
	User::Panic(_L("CMyActiveScheduer::Error"), anError);
	}

#include <e32svr.h>

TInt ClientThread(TAny*)
//
// Passed as the first client thread - signals the server to do several tests
//
	{
	RTest test(_L("T_MESSGE...client"));
	
	test.Title(); 
	test.Start(_L("Wait for server to start"));
	sem.Wait();
	
	test.Next(_L("Create Session"));
	TInt r=session.PublicCreateSession(_L("CTestServer"),5);
	if (r!=KErrNone)
		User::Panic(_L("CreateSessn failure"),r);

	test.Next(_L("Signal to test Int0/1/2/3()"));
	r=session.PublicSendReceive(CTestSession::ETestInt,TIpcArgs(-3866,30566,0,200) );
	test(r==KErrNone);

	test.Next(_L("Signal to test Ptr0/1/2/3()"));
	
	r=session.PublicSendReceive(CTestSession::ETestPtr, TIpcArgs(&clientThread, &serverThread, &session, &sem));
	test(r==KErrNone);

	test.Next(_L("Signal to test Client()"));
	r=session.PublicSendReceive(CTestSession::ETestClient, TIpcArgs());
	test(r==KErrNone);

	test.Next(_L("Signal to test ClientProcessFlags()"));
	r=session.PublicSendReceive(CTestSession::ETestClientProcessFlags, TIpcArgs());
	test(r==KErrNone);
	
	test.Next(_L("Signal to test ClientIsRealtime()"));
	r=session.PublicSendReceive(CTestSession::ETestClientIsRealtime, TIpcArgs());
	test(r==KErrNone);

	test.Next(_L("Test RMessage2::Complete()"));
	TRequestStatus stat[7];
	for (r=0;r<4;r++)
		{
		session.PublicSendReceive(CTestSession::ETestComplete, TIpcArgs(), stat[r]);
		test(stat[r]==KRequestPending);
		}
	session.PublicSendReceive(CTestSession::ETestComplete, TIpcArgs(), stat[4]);
	User::WaitForRequest(stat[0]);
	for (r=0;r<5;r++)
		test(stat[r]==5-r);	 //Test the 'error messages' set by Complete()
	test.Next(_L("Test RMessagePtr2::Complete()"));
	for (r=0;r<4;r++)
		{
		session.PublicSendReceive(CTestSession::ETestPtrComplete, TIpcArgs(), stat[r]);
		test(stat[r]==KRequestPending);
		}
	session.PublicSendReceive(CTestSession::ETestPtrComplete, TIpcArgs(), stat[4]);
	User::WaitForRequest(stat[0]);
	for (r=0;r<5;r++)
		test(stat[r]==10-r*2);

	test.Next(_L("Test RMessage contruction from Ptr"));
	if(UserSvr::IpcV1Available())
		{
		r=session.PublicSendReceive(CTestSession::ETestMessageConstruction, TIpcArgs(111,222,333,444));
		test(r==KErrNone);
		}
	else
		{
		test.Printf(_L("NOT TESTED - IPC V1 is not supported by system"));
		}

	test.Next(_L("Test RMessagePtr leaving interface"));
	r=session.PublicSendReceive(CTestSession::ETestRMessagePtr2LeavingInterface, TIpcArgs());
	test(r==KErrNone);

	test.Next(_L("Try another session"));
	r=otherSession.PublicCreateSession(_L("CTestServer"),5);
	test(r==KErrNone);

	r=otherSession.PublicSendReceive(CTestSession::ETestOtherSession, TIpcArgs());
	test(r==KErrNone);
	
	test.Next(_L("Saturate server"));
	for(r=0;r<7;r++)
		{
		test.Printf(_L("Send %d\r"),r);
		session.PublicSendReceive(CTestSession::ETestCompleteAfter,TIpcArgs(),stat[r]);			
		if (r<5)
			test(stat[r]==KRequestPending);
		else
			test(stat[r]==KErrServerBusy);
		}
 	test.Printf(_L("\n"));
	for(r=0;r<5;r++)
		{
		test.Printf(_L("Wait %d\r"),r);
		User::WaitForRequest(stat[r]);
		test(stat[r]==KErrNone);
		}
 	test.Printf(_L("\n"));

	test.Next(_L("Signal to stop ActiveScheduler"));
	session.PublicSendReceive(CTestSession::EStop, TIpcArgs());	
	session.Close();
	otherSession.PublicSendReceive(CTestSession::EStop, TIpcArgs());
	otherSession.Close();
	test.Close();
	
	return (KErrNone);
	}

TInt ServerThread(TAny*)
//
// Passed as the server thread in 2 tests - sets up and runs CTestServer
//
	{
	RTest test(_L("T_MESSGE...server"));
	
	test.Title();
	test.Start(_L("Create and install ActiveScheduler"));
	CMyActiveScheduler* pScheduler = new CMyActiveScheduler;
	if (pScheduler==NULL)
		{
		clientThread.Kill(0);
		User::Panic(_L("CreateSched failure"),KErrNoMemory);
		}

	CActiveScheduler::Install(pScheduler);

	test.Next(_L("Creating and starting Server"));
	CTestServer* pServer = new CTestServer(0);
	if (pServer==NULL)
		{
		clientThread.Kill(0);
		User::Panic(_L("CreateServr failure"),KErrNoMemory);
		}

	TInt r=pServer->Start(KServerName);//Starting a CServer2 also Adds it to the ActiveScheduler
	if (r!=KErrNone)
		{
		clientThread.Kill(0);
		User::Panic(_L("StartServr failure"),r);
		}


	test.Next(_L("Start ActiveScheduler and signal to client"));
	test.Printf(_L("        There might be something going on beneath this window\n"));
	sem.Signal();
	CActiveScheduler::Start();
	test.Next(_L("Destroy ActiveScheduler"));
	delete pScheduler;
	delete pServer;

	test.Close();
		
	return (KErrNone);
	}

const TInt KTestPanic = 14849;

TInt PanicTestThread (TAny*)
//
// Passed as a thread entry - just calls RMessage2::Panic()
//
	{
	RMessage2 message;
	message.Panic(_L("Testing Panic"),KTestPanic);
	return(KErrNone);
	}

RTest test(_L("Main T_MESSGE test"));

TInt CompletePanicClientThread (TAny*)
//
// Passed as the second client thread entry - signals to server to call Complete() twice
//
	{
	sem.Wait();
	
	TInt r=session.PublicCreateSession(_L("CTestServer"),1);
	test(r==KErrNone);

	r=session.PublicSendReceive(CTestSession::ETestCompletePanic, TIpcArgs());
	test(r==KErrNone);

	session.PublicSendReceive(CTestSession::EStop, TIpcArgs());//panic should occur before this is serviced 
	return(KErrNone);
	}
	
TInt KillCompletePanicClientThread (TAny*)
//
//  A client thread entry - signals to server to kill client after completing the message
//
	{
	sem.Wait();
	
	TInt r=session.PublicCreateSession(_L("CTestServer"),1);
	test(r==KErrNone);

	r=session.PublicSendReceive(CTestSession::ETestKillCompletePanic, TIpcArgs());
	test(r==KErrNone);

	session.PublicSendReceive(CTestSession::EStop, TIpcArgs());//panic should occur before this is serviced 
	return(KErrNone);
	}

void SimpleRMessage()
//
// Simple RMessage2 Tests - constructors and assignment
//
	{
	
	test.Start(_L("Default constructor"));
	RMessage2 message1;

	test.Next(_L("Copy constructor"));
	RMessage2 message2(message1);
	test(message1.Function()==message2.Function());
	test(message1.Int0()==message2.Int0());
	test(message1.Int1()==message2.Int1());
	test(message1.Int2()==message2.Int2());
	test(message1.Int3()==message2.Int3());

	test.Next(_L("Assignment operator"));
	RMessage2 message3;
	Mem::Fill(&message3,sizeof(message3),0xb9);	// first fill message 3 with rubbish
	message3=message1;
 	test(message1.Function()==message3.Function());
	test(message1.Int0()==message3.Int0());
	test(message1.Int1()==message3.Int1());
	test(message1.Int2()==message3.Int2());
	test(message1.Int3()==message3.Int3());

	test.End();
	}

void SimpleRMessagePtr()
//
// Simple RMessagePtr2 tests - constructors and assignment
//
	{
	test.Start(_L("Default constructor"));
	RMessagePtr2 messagePtr1;
	test(messagePtr1.IsNull());

	test.Next(_L("Copy constructor"));
	RMessagePtr2 messagePtr2(messagePtr1);
	test(messagePtr2.IsNull());

	test.Next(_L("Constructor from pointer to RMessage2"));
	RMessage2 message;
    messagePtr1= message;

	test.Next(_L("Assignment operator"));
	messagePtr1=messagePtr2;

	test.End();
	}
	
void TestServerCompleteTwicePanic(void)
	{
	TRequestStatus clientStat,serverStat;
	
	TBool justInTime=User::JustInTime();
	
 	test.Next(_L("Check server panics if you try to complete a message twice"));
	test.Start(_L("Create client and server threads"));
	clientThread.Create(_L("Client Thread1"),CompletePanicClientThread,KDefaultStackSize,KHeapMinSize,KHeapMaxSize,NULL);
	serverThread.Create(_L("Server Thread"),ServerThread,KDefaultStackSize,KHeapMinSize,KHeapMaxSize,NULL);
	
	test.Next(_L("Logon to the threads"));
	clientThread.Logon(clientStat);
	serverThread.Logon(serverStat);

	test.Next(_L("Start the threads"));
	sem.CreateLocal(0);
	User::SetJustInTime(EFalse); 
	clientThread.Resume();
	serverThread.Resume();
	
	test.Next(_L("Wait for the threads to stop"));
	User::WaitForRequest(clientStat); //
	User::WaitForRequest(serverStat);
	User::SetJustInTime(justInTime); 
	test.Next(_L("Check the exit categories"));
	test(clientThread.ExitType()==EExitKill);
	test(clientThread.ExitCategory().Compare(_L("Kill"))==0);
	test(clientThread.ExitReason()==KErrNone);
		
	test(serverThread.ExitType()==EExitPanic);
	test(serverThread.ExitCategory().Compare(_L("USER"))==0);
	test(serverThread.ExitReason()==ETMesCompletion);

	test.Next(_L("Close the threads"));
	CLOSE_AND_WAIT(serverThread);
	CLOSE_AND_WAIT(clientThread);
	test.End();	  
	}
	
void TestServerKillCompletePanic(void)
	{
	TRequestStatus clientStat,serverStat;
	
	TBool justInTime=User::JustInTime();
	
	test.Next(_L("Check Server panics if you try to panic a client using a completed message"));
	test.Start(_L("Create client and server threads"));
	clientThread.Create(_L("Client Thread2"),KillCompletePanicClientThread,KDefaultStackSize,KHeapMinSize,KHeapMaxSize,NULL);
	serverThread.Create(_L("Server Thread"),ServerThread,KDefaultStackSize,KHeapMinSize,KHeapMaxSize,NULL);
	
	test.Next(_L("Logon to the threads"));
	clientThread.Logon(clientStat);
	serverThread.Logon(serverStat);

	test.Next(_L("Start the threads"));
	sem.CreateLocal(0);
	User::SetJustInTime(EFalse); 
	clientThread.Resume();
	serverThread.Resume();
	
	test.Next(_L("Wait for the threads to stop"));
	User::WaitForRequest(clientStat); 
	User::WaitForRequest(serverStat);
	User::SetJustInTime(justInTime); 
	test.Next(_L("Check the exit categories"));
	test(clientThread.ExitType()==EExitKill);
	test(clientThread.ExitCategory().Compare(_L("Kill"))==0);
	test(clientThread.ExitReason()==KErrNone);
		
	test(serverThread.ExitType()==EExitPanic);
	test(serverThread.ExitCategory().Compare(_L("KERN-EXEC"))==0);
	test(serverThread.ExitReason()==EBadMessageHandle);
	
	test.Next(_L("Close the threads"));
	CLOSE_AND_WAIT(serverThread);
	CLOSE_AND_WAIT(clientThread);
	test.End();	 
	}	

GLDEF_C TInt E32Main()
	{
	test.Title();
	
	test.Start(_L("Simple RMessage2 tests"));
	SimpleRMessage();

	test.Next(_L("Simple RMessagePtr2 tests"));
	SimpleRMessagePtr();

	test.Next(_L("Sending messages between two threads"));
	test.Start(_L("Create client and server threads"));
	clientThread.Create(_L("Client Thread"),ClientThread,KDefaultStackSize,KHeapMinSize,KHeapMaxSize,NULL);
	serverThread.Create(_L("Server Thread"),ServerThread,KDefaultStackSize,KHeapMinSize,KHeapMaxSize,NULL);
	
	test.Next(_L("Logon to the threads"));
	TRequestStatus clientStat,serverStat;	
	clientThread.Logon(clientStat);
	serverThread.Logon(serverStat);

	test.Next(_L("Start the threads"));
	sem.CreateLocal(0);
	clientThread.Resume();
	serverThread.Resume();
	
	test.Next(_L("Wait for the threads to stop"));
	if(clientStat==KRequestPending)
		User::WaitForRequest(clientStat);
	if(serverStat==KRequestPending)
		User::WaitForRequest(serverStat);
	switch (clientThread.ExitType())
		{
		case EExitKill:
			test.Printf(_L("  Client thread killed\n")); 
			break;
		case EExitTerminate:
			test.Printf(_L("!!Client thread terminated:"));
			test.Panic(clientThread.ExitCategory(), clientThread.ExitReason());
		case EExitPanic:
			test.Panic(_L("!!Client thread panicked:"));
			test.Panic(clientThread.ExitCategory(), clientThread.ExitReason());
		default:
			test.Panic(_L("!!Client thread did something bizarre"), clientThread.ExitReason());
		}
	switch (serverThread.ExitType())
		{
		case EExitKill:
			test.Printf(_L("  Server thread killed\n")); 
			break;
		case EExitTerminate:
			test.Printf(_L("!!Server thread terminated:"));
			test.Panic(serverThread.ExitCategory(), serverThread.ExitReason());
		case EExitPanic:
			test.Printf(_L("!!Server thread panicked:"));
			test.Panic(serverThread.ExitCategory(), serverThread.ExitReason());
			//
			// To catch a panic put a breakpoint in User::Panic() (in UCDT\UC_UNC.CPP).
			//
		default:
			test.Panic(_L("!!Server thread did something bizarre"), serverThread.ExitReason());
		}

	test.Next(_L("Close the threads"));
	CLOSE_AND_WAIT(serverThread);
	CLOSE_AND_WAIT(clientThread);
	test.End();
	
	TestServerCompleteTwicePanic();
	
	TestServerKillCompletePanic();
  	
	test.End();
  
	return (KErrNone);
	}


