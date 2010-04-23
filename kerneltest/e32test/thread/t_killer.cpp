// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\thread\t_killer.cpp
// Derived from T_MESSGE, tests threads killing each other, not cleaning up etc.
// 
//


#include <e32std.h>
#include <e32std_private.h>
#include <e32math.h>
#include <e32test.h>
#include <e32ver.h>
#include <e32panic.h>

const TInt KHeapMinSize=0x1000;
const TInt KHeapMaxSize=0x1000;

class CTestServer : public CServer2
	{
public:
	IMPORT_C CTestServer(TInt aPriority);
	enum TPanicType{
		EInt0Error=1, EInt1Error, EInt2Error, EInt3Error,
		EPtr0Error, EPtr1Error, EPtr2Error, EPtr3Error,
		ECreateNameError, ENewSessionError,
		EClientError, EWatcherError, EKilled
		};
	static void Panic(TPanicType aReason);
protected:
	//override the pure virtual functions:
	IMPORT_C virtual CSession2* NewSessionL(const TVersion& aVersion, const RMessage2 &) const;
	};


class CTestSession : public CSession2
	{
public:
	enum {EStop,ETestInt,ETestPtr,ETestClient,ETestComplete,ETestPtrComplete,ETestCompletePanic,ETestOtherSession,ETestCompleteAfter};
//Override pure virtual
	IMPORT_C virtual void ServiceL(const RMessage2& aMessage);
private:
	void TestInt(const RMessage2& aMessage);
	void TestPtr(const RMessage2& aMessage);
	void TestClient(const RMessage2& aMessage);
	void TestComplete(const RMessage2& aMessage);
	void TestPtrComplete(const RMessage2& aMessage);
	void TestCompletePanic();
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
	TInt PublicSendReceive(TInt aFunction, const TIpcArgs& aArgs)
		{
		return (SendReceive(aFunction, aArgs));
		}
	void PublicSendReceive(TInt aFunction, const TIpcArgs& aArgs, TRequestStatus& aStatus)
		{
		SendReceive(aFunction, aArgs, aStatus);
		}
	TInt PublicCreateSession(const TDesC& aServer,TInt aMessageSlots)
		{
		return (CreateSession(aServer,User::Version(),aMessageSlots));
		}
	};

//=========================================================================

// CTestServer functions

EXPORT_C CTestServer::CTestServer(TInt aPriority) 
//
// Constructor - sets name
//
	: CServer2(aPriority)
	{}

EXPORT_C  CSession2* CTestServer::NewSessionL(const TVersion& aVersion, const RMessage2 &) const
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

RThread clientThread, serverThread, killerThread;

void CTestServer::Panic(TPanicType aReason)	  //static function
	{
	clientThread.Kill(KErrNone);
	User::Panic(_L("CTestServer"),aReason);
	}

// CTestSession funtions

RSession session, otherSession;
RSemaphore sem;

const TInt KTestInt[] = {-3866,30566,0,200};
const TIpcArgs KIpcArgInt(-3866,30566,0,200);

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
const TIpcArgs KIpcArgPtr(&clientThread, &serverThread, &session, &sem);

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

TFullName clientName;
TInt clientNumber;

void CTestSession::TestClient(const RMessage2& aMessage)
//
// Tests Client()
//
	{

	// Under WINS, thread names are not prefixed with the process name
	TFullName n=RProcess().Name();
	n+=_L("::");
	n+=clientName;

	RThread client;
	TInt r = aMessage.Client(client);
	if (r != KErrNone || client.FullName().CompareF(n)!=0)
		{
		client.Close();
		clientThread.Kill(0);
		CTestServer::Panic(CTestServer::EClientError);
		}
	client.Close();
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
	if (++count2==5)
		for(count2=4; count2>=0; count2--)
			messagePtrs[count2].Complete(10-count2*2);
	}

EXPORT_C  void CTestSession::ServiceL(const RMessage2& aMessage)
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
		default:
			r=KErrNotSupported;

		}						  
 	aMessage.Complete(r);
	
	}

void CMyActiveScheduler::Error(TInt anError) const
//
// Virtual error handler
//
	{
	User::Panic(_L("CMyActiveScheduer::Error"), anError);
	}

LOCAL_D TInt64 TheSeed;
GLDEF_C TInt Random(TInt aRange)
	{
	return (Math::Rand(TheSeed)>>11)%aRange;
	}

TInt KillerThread(TAny*)
//
// Wait a random time and then kill the client thread
//
    {
    RTest test(_L("T_KILLER...Killer"));
    TRequestStatus clientStatus;
    TInt delay=0;

    test.Title();
    test.Start(_L("Logon to client"));
    clientThread.Logon(clientStatus);
    test.Next(_L("Delay...."));
    for (;;)
	{
	User::After(1000);
	delay++;
	if (clientStatus!=KRequestPending)
	    return KErrNone;	// client has already finished
	if (Random(1000)<1)
	    break;		// Time to die!
	}
    test.Printf(_L("Kill client after %d ms\n"), delay);
    clientThread.Kill(CTestServer::EKilled);

    test.Close();	// close console immediately
    // test.End();	// "Press ENTER to exit"
    return KErrNone;
    }

TInt ClientThread(TAny* aPtr)
//
// Passed as the first client thread - signals the server to do several tests
//
    {
    RTest test(_L("T_KILLER...client"));
    TInt repeat = (TInt)aPtr;
    
    test.Title(); 
    test.Start(_L("Client thread"));

    do 
		{
		test.Next(_L("Client loop"));
		test.Start(_L("Create Session"));
		TInt r=session.PublicCreateSession(_L("CTestServer"),5);
		if (r!=KErrNone)
			User::Panic(_L("CreateSessn failure"),r);

		test.Next(_L("Signal to test Int0/1/2/3()"));
		r=session.PublicSendReceive(CTestSession::ETestInt, KIpcArgInt);
		test(r==KErrNone);

		test.Next(_L("Signal to test Ptr0/1/2/3()"));
		r=session.PublicSendReceive(CTestSession::ETestPtr, KIpcArgPtr);
		test(r==KErrNone);

		test.Next(_L("Signal to test Client()"));
		r=session.PublicSendReceive(CTestSession::ETestClient, TIpcArgs());
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

		test.Next(_L("Try another session"));
		r=otherSession.PublicCreateSession(_L("CTestServer"),5);
		test(r==KErrNone);

		r=otherSession.PublicSendReceive(CTestSession::ETestOtherSession, TIpcArgs());
		test(r==KErrNone);
	
//	test.Next(_L("Try to disconnect"));
//	r=session.PublicSendReceive(RMessage2::EDisConnect,NULL);//Panics user
//	test(r==KErrNone);

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
		test.End();
		}
	while (--repeat > 0);
    test.Start(_L("Signal to stop ActiveScheduler"));
    session.PublicSendReceive(CTestSession::EStop, TIpcArgs());			
    test.Close();
    
    return (KErrNone);
    }

TInt ServerThread(TAny*)
//
// Passed as the server thread in 2 tests - sets up and runs CTestServer
//
	{
	RTest test(_L("T_KILLER...server"));
	
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

	TInt r=pServer->Start(_L("CTestServer"));//Starting a CServer2 also Adds it to the ActiveScheduler
	if (r!=KErrNone)
		{
		clientThread.Kill(0);
		User::Panic(_L("StartServr failure"),r);
		}


	test.Next(_L("Start ActiveScheduler and signal to client"));
	test.Printf(_L("        There might be something going on beneath this window"));
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

RTest test(_L("Main T_KILLER test"));

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
	RThread client1;
	test(message1.Client(client1) == KErrNone);
	RThread client2;
	test(message2.Client(client2) == KErrNone);
 	test(client1.Handle()==client2.Handle());
	client2.Close();

	test.Next(_L("Assignment operator"));
	RMessage2 message3(*(RMessage2*) SimpleRMessage);// Pass some rubbish so message3 is definitely != message1
	message3=message1;
 	test(message1.Function()==message3.Function());
	test(message1.Int0()==message3.Int0());
	test(message1.Int1()==message3.Int1());
	test(message1.Int2()==message3.Int2());
	test(message1.Int3()==message3.Int3());
	RThread client3;
	test(message3.Client(client3) == KErrNone);
 	test(client1.Handle()==client3.Handle());
	client3.Close();
	client1.Close();
	test.End();
	}

GLDEF_C TInt E32Main()
	{
	TInt err;

#ifdef __WINS__
	User::SetDebugMask(0xa04);  // KSERVER+KTHREAD+KLOGON
#endif
	test.Title();
	
	test.Next(_L("Sending messages between two threads"));
	TRequestStatus clientStat,killerStat,serverStat;
	TInt exitType;

	test.Start(_L("Create and start the server"));
	sem.CreateLocal(0);
	serverThread.Create(_L("Server Thread"),ServerThread,KDefaultStackSize,KHeapMinSize,KHeapMaxSize,NULL);
	serverThread.Logon(serverStat);
	serverThread.Resume();
	sem.Wait();

	for (TInt i=0; serverStat==KRequestPending && i<100; i++)
	    {
	    test.Next(_L("Run and kill a client"));
	    clientName.Format(_L("Client Thread %d"),++clientNumber);

	    test.Start(_L("Create client and killer threads"));
	    err=clientThread.Create(clientName,ClientThread,KDefaultStackSize,KHeapMinSize,KHeapMaxSize,(TAny *)3);
	    if (err)
		test.Panic(_L("!!clientThread .Create failed"), err);
	    err=killerThread.Create(_L("Killer Thread"),KillerThread,KDefaultStackSize,KHeapMinSize,KHeapMaxSize,NULL);
	    if (err)
		test.Panic(_L("!!killerThread .Create failed"), err);
	    
	    test.Next(_L("Logon to the threads"));
	    clientThread.Logon(clientStat);
	    killerThread.Logon(killerStat);

	    test.Next(_L("Start the threads"));
	    clientThread.Resume();
	    killerThread.Resume();
	    
	    test.Next(_L("Wait for the client to stop"));
	    User::WaitForRequest(clientStat);
	    test.Next(_L("Wait for the killer to stop"));
	    User::WaitForRequest(killerStat);
	    exitType=clientThread.ExitType();
	    switch (exitType)
		{
		case EExitKill:
			test.Printf(_L("  Client thread killed\n")); 
			break;
		case EExitTerminate:
			test.Printf(_L("!!Client thread terminated:"));
			test.Panic(clientThread.ExitCategory(), clientThread.ExitReason());
		case EExitPanic:
			test.Printf(_L("!!Client thread panicked:"));
			test.Panic(clientThread.ExitCategory(), clientThread.ExitReason());
		default:
			test.Panic(_L("!!Client thread did something bizarre"), clientThread.ExitReason());
		}
	    exitType=killerThread.ExitType();
	    switch (exitType)
		{
		case EExitKill:
			test.Printf(_L("  Killer thread killed\n")); 
			break;
		case EExitTerminate:
			test.Printf(_L("!!Killer thread terminated:"));
			test.Panic(killerThread.ExitCategory(), killerThread.ExitReason());
		case EExitPanic:
			test.Printf(_L("!!Killer thread panicked:"));
			test.Panic(killerThread.ExitCategory(), killerThread.ExitReason());
			//
			// To catch a panic put a breakpoint in User::Panic() (in UCDT\UC_UNC.CPP).
			//
		default:
			test.Panic(_L("!!Killer thread did something bizarre"), killerThread.ExitReason());
		}
	    test.Next(_L("Close the threads"));
	    clientThread.Close();
	    killerThread.Close();
	    // test.Next(_L("Pause for 1 second"));
	    // User::After(1000000);
	    test.End();
	    }
	test.Next(_L("Close the server thread"));
	serverThread.Kill(0);	// in case we got through the 100 iterations without killing it
	User::WaitForRequest(serverStat);
	exitType=serverThread.ExitType();
	switch (exitType)
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
	serverThread.Close();
	test.End();

	test.Next(_L("The Panic() function"));
	RThread panicThread;
	panicThread.Create(_L("Panic Test Thread"),PanicTestThread,KDefaultStackSize,KHeapMinSize,KHeapMaxSize,NULL);
	TRequestStatus stat;
	panicThread.Logon(stat);
	// don't want just in time debugging as we trap panics
	TBool justInTime=User::JustInTime(); 
	User::SetJustInTime(EFalse); 
	panicThread.Resume();
	User::WaitForRequest(stat);
	test(panicThread.ExitType()==EExitPanic);
	test(panicThread.ExitCategory().Compare(_L("Testing Panic"))==0);
	test(panicThread.ExitReason()==KTestPanic);
	panicThread.Close(); //If this Close() is missed out Wins build 48 throws a wobbler when we next connect a server

	
 	test.Next(_L("Check it Panics if you try to Complete a message twice"));
	test.Start(_L("Create client and server threads"));
	clientThread.Create(_L("Client Thread"),CompletePanicClientThread,KDefaultStackSize,KHeapMinSize,KHeapMaxSize,NULL);
	serverThread.Create(_L("Server Thread"),ServerThread,KDefaultStackSize,KHeapMinSize,KHeapMaxSize,NULL);
	
	test.Next(_L("Logon to the threads"));
	clientThread.Logon(clientStat);
	serverThread.Logon(serverStat);

	test.Next(_L("Start the threads"));
	sem.CreateLocal(0);
	clientThread.Resume();
	serverThread.Resume();
	
	test.Next(_L("Wait for the threads to stop"));
	User::WaitForRequest(clientStat);
	User::WaitForRequest(serverStat);
	test.Next(_L("Check the exit categories"));
	test(clientThread.ExitType()==EExitKill);
	test(clientThread.ExitCategory().Compare(_L("Kill"))==0);
	test(clientThread.ExitReason()==KErrNone);
		
	test(serverThread.ExitType()==EExitPanic);
	test(serverThread.ExitCategory().Compare(_L("USER"))==0);
	test(serverThread.ExitReason()==ETMesCompletion);

	User::SetJustInTime(justInTime);

	test.Next(_L("Close the threads"));
	clientThread.Close();
	serverThread.Close();
	test.End();	  
  	
	test.End();
  
	return (KErrNone);
	}

