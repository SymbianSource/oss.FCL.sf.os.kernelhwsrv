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
// f32test\demandpaging\t_wdpstress.cpp
// Data Paging Stress Tests
// Common command lines:
// t_wdpstress lowmem
// debug - switch on debugging information
// silent - no output to the screen or serial port
// single - run the tests in a single thread
// multiple <numThreads> - run the tests in multiple threads where <numThreads> (max 50 simultaneous threads)
// interleave - force thread interleaving
// prio - each thread reschedules in between each function call, causes lots of context changes
// media - perform media access during the tests, very stressful
// lowmem - low memory tests
// stack - perform autotest only with stack paging tests
// chunk - perform autotest only with chunk paging tests 			
// commit - perform autotest only with committing and decommitting paging tests 
// ipc - perform autotest only with ipc pinning tests 			
// all - perform autotest with all paging tests(ipc, stack, chunk and commit)
// badserver - perform ipc pinning tests with dead server
// iters <count> - the number of times to loop 
// 
//

//! @SYMTestCaseID			KBASE-T_WDPSTRESS-xxx
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1954
//! @SYMTestCaseDesc		Writable Data Paging Stress Tests
//! @SYMTestActions			
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
//
#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32ver.h>
RTest test(_L("T_WDPSTRESS"));

#include <e32rom.h>
#include <u32hal.h>
#include <f32file.h>
#include <e32svr.h>
#include <e32hal.h>
#include <f32dbg.h>
#include <e32msgqueue.h>
#include <e32math.h>
#include <dptest.h>
#include <hal.h>
#include "testdefs.h"

#ifdef __X86__
#define TEST_ON_UNPAGED
#endif

#include "t_pagestress.h"

TBool   	TestDebug					= EFalse;
TBool		TestSilent					= EFalse;
TBool		TestExit					= EFalse;


TInt		gPerformTestLoop			= 10;					// Number of times to perform test on a thread
const TUint KMaxTestThreads				= 20;					// The maximum number of threads allowed to run simultaniously
TInt		gNumTestThreads				= KMaxTestThreads;		// The number of threads to run simultaneously

#define TEST_INTERLEAVE_PRIO			EPriorityMore

TBool		TestWeAreTheTestBase		= EFalse;

#define TEST_NONE		0x0
#define TEST_IPC		0x1
#define TEST_STACK		0x2
#define TEST_CHUNK		0x4
#define TEST_COMMIT		0x8
#define TEST_ALL		(TEST_COMMIT | TEST_CHUNK | TEST_STACK | TEST_IPC)

TUint32		gSetTests					= TEST_ALL;
TUint32		gTestWhichTests				= gSetTests;
TBuf<32>	gTestNameBuffer;
TBool		gTestPrioChange				= EFalse;				
TBool		gTestStopMedia				= EFalse;
TBool		gTestMediaAccess			= EFalse;
TBool		gTestInterleave				= EFalse;
TBool		gTestBadServer				= EFalse;

#define TEST_LM_NUM_FREE	0
#define TEST_LM_BLOCKSIZE	1
#define TEST_LM_BLOCKS_FREE	4

RPageStressTestLdd Ldd;
RSemaphore	TestMultiSem;
RMsgQueue<TBuf <64> >	TestMsgQueue;

TBool		gIsDemandPaged			= ETrue;
TBool		gTestRunning				= EFalse;				// To control when to stop flushing
TBool		gMaxChunksReached			= EFalse;				// On moving memory model, the number of chunks per process is capped

TInt		gPageSize;											// The number of bytes per page
TUint		gPageShift;
TUint		gChunksAllocd				= 0;					// The total number of chunks that have been allocated
TUint		gMaxChunks					= 0;					// The max amount of chunks after which KErrOverflow will be returned
RHeap*		gThreadHeap					= NULL;					
RHeap*		gStackHeap					= NULL;

TInt		gTestType					= -1;					// The type of test that is to be performed

#define TEST_NEXT(__args) \
	if (!TestSilent)\
		test.Next __args;

#define RDBGD_PRINT(__args)\
	if (TestDebug)\
	RDebug::Printf __args ;\

#define RDBGS_PRINT(__args)\
	if (!TestSilent)\
	RDebug::Printf __args ;\

#define DEBUG_PRINT(__args)\
if (!TestSilent)\
	{\
	if (aTestArguments.iMsgQueue && aTestArguments.iBuffer && aTestArguments.iTheSem)\
		{\
		aTestArguments.iBuffer->Zero();\
		aTestArguments.iBuffer->Format __args ;\
		aTestArguments.iTheSem->Wait();\
		aTestArguments.iMsgQueue->SendBlocking(*aTestArguments.iBuffer);\
		aTestArguments.iTheSem->Signal();\
		}\
	else\
		{\
		test.Printf __args ;\
		}\
	}

#define RUNTEST(__test, __error)\
	if (!TestSilent)\
		test(__test == __error);\
	else\
		__test;

#define RUNTEST1(__test)\
	if (!TestSilent)\
		test(__test);

#define DEBUG_PRINT1(__args)\
if (TestDebug)\
	{\
	DEBUG_PRINT(__args)\
	}

#define DOTEST(__operation, __condition)\
	if (aLowMem) \
		{\
		__operation;\
		while (!__condition)\
			{\
			Ldd.DoReleaseSomeRam(TEST_LM_BLOCKS_FREE);\
			__operation;\
			}\
		RUNTEST1(__condition);\
		}\
	else\
		{\
		__operation;\
		RUNTEST1(__condition);\
		}

#define DOTEST1(__operation, __condition)\
	if (aTestArguments.iLowMem) \
		{\
		__operation;\
		while (!__condition)\
			{\
			Ldd.DoReleaseSomeRam(TEST_LM_BLOCKS_FREE);\
			__operation;\
			}\
		RUNTEST1(__condition);\
		}\
	else\
		{\
		__operation;\
		RUNTEST1(__condition);\
		}

struct SThreadExitResults
	{
	TInt					iExitType;
	TInt					iExitReason;
	};
SThreadExitResults* gResultsArray;
const TInt KExitTypeReset = -1;

struct SPerformTestArgs
	{
	TInt					iThreadIndex;
	RMsgQueue<TBuf <64> >	*iMsgQueue; 
	TBuf<64>				*iBuffer;
	RSemaphore				*iTheSem;
	TBool					iLowMem;
	TInt					iTestType;
	};


TInt DoTest(TInt gTestType, TBool aLowMem = EFalse);
enum
	{
	ETestSingle, 
	ETestMultiple,
	ETestMedia,
	ETestLowMem,
	ETestInterleave,
	ETestCommit, 
	ETestTypes,
	// This is at the moment manual
	ETestBadServer, 
	ETestTypeEnd, 
	};

TInt FreeRam()
	{
	// wait for any async cleanup in the supervisor to finish first...
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);

	TMemoryInfoV1Buf meminfo;
	TInt r = UserHal::MemoryInfo(meminfo);
	test_KErrNone(r);
	return meminfo().iFreeRamInBytes;
	}

const TUint KStackSize = 20 * 4096;
TUint stackLimit = 150;//*** NEED TO WORK OUT HOW MUCH STACK WE HAVE***

/**
Recursive function
*/
void CallRecFunc(TUint aNum, TInt aThreadIndex)
	{
	RDBGD_PRINT(("ThreadId %d CallRecFunc, aNum = %d\n", aThreadIndex, aNum));
	if (aNum >= stackLimit)
		{// To avoid a stack overflow
		return;
		}
	else
		{
		CallRecFunc(++aNum, aThreadIndex);
		User::After(0);
		}
	RDBGD_PRINT(("ThreadId %d CRF(%d)Returning...", aThreadIndex, aNum));
	return;
	}

/**
Thread that calls a recursive function
*/
TInt ThreadFunc(TAny* aThreadIndex)
	{
	for (TUint i=0; i<1; i++)
		{
		CallRecFunc(0, (TInt)aThreadIndex);
		}
	RDBGD_PRINT(("ThreadId %d ThreadFunc Returning...", (TInt)aThreadIndex));
	return KErrNone;
	}

/**
Thread continuously flushes the paging cache
*/
TInt FlushFunc(TAny* /*aPtr*/)
	{
	RThread().SetPriority(EPriorityMore);
	while(gTestRunning)
		{
		DPTest::FlushCache();	
		User::After((Math::Random()&0xfff)*10);
		}
	return KErrNone;
	}


//
// TestStackPaging
//
// Create a paged thread which calls a recursive function.
// Calls to function will be placed on the stack, which is data paged
//

TInt TestStackPaging(SPerformTestArgs& aTestArguments)
	{
	RDBGD_PRINT(("Creating test thread"));
	TBuf<16> runThreadName;
	runThreadName = _L("");
	TThreadCreateInfo threadCreateInfo(runThreadName, ThreadFunc, KStackSize, (TAny*) aTestArguments.iThreadIndex);
	threadCreateInfo.SetCreateHeap(KMinHeapSize, KMinHeapSize);
	//threadCreateInfo.SetUseHeap(NULL);
	threadCreateInfo.SetPaging(TThreadCreateInfo::EPaged);

	RThread testThread;
	TInt r;
	for(;;)
		{
		r = testThread.Create(threadCreateInfo);
		if(r != KErrNoMemory)
			break;
		if(!aTestArguments.iLowMem)
			break;
		if(Ldd.DoReleaseSomeRam(TEST_LM_BLOCKS_FREE) != KErrNone)
			break;
		RDBGD_PRINT(("TestStackPaging released some RAM\n"));
		}

	RDBGD_PRINT(("TID(%d) TestStackPaging create r = %d freeRam = %d\n", aTestArguments.iThreadIndex, r, FreeRam()));
	if (r != KErrNone)
		return r;

	TRequestStatus threadStatus;
	testThread.Logon(threadStatus);
	
	RDBGD_PRINT(("resuming test thread"));
	testThread.Resume();
	
	RDBGD_PRINT(("waiting for threadstatus"));
	User::WaitForRequest(threadStatus);
	
	RDBGD_PRINT(("Killing threads\n"));
	testThread.Close();

	return KErrNone;
	}

//--------------------------Server Pinning stuff-----------------------------------------------------
_LIT(KTestServer,"CTestServer");
const TUint KSemServer = 0;

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
		ERead,
		EWrite,
		EReadWrite,
		};
//Override pure virtual
	IMPORT_C virtual void ServiceL(const RMessage2& aMessage);
private:
	TInt ReadWrite(const RMessage2& aMessage, TBool aRead, TBool aWrite);
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

struct SServerArgs
	{
	TBool iBadServer;
	RSemaphore iSemArray;
	};

SServerArgs gServerArgsArray[KMaxTestThreads];

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

TInt CTestSession::ReadWrite(const RMessage2& aMessage, TBool aRead, TBool aWrite)
	{
	TInt r = KErrNone;
	for (TUint argIndex = 0; argIndex < 4; argIndex++)
		{
		// Get the length of the descriptor and verify it is as expected.
		TInt length = aMessage.GetDesLength(argIndex);
		if (length < KErrNone)
			{
			RDebug::Printf("  Error getting descriptor length %d", length);
			return length;
			}

		
		if (aRead)
			{
			// Now read the descriptor
			HBufC8* des = HBufC8::New(length);
			if (!des)
				return KErrNoMemory;
			TPtr8 desPtr = des->Des();
			r = aMessage.Read(argIndex, desPtr);
			if (r != KErrNone)
				{
				delete des;
				return r;
				}
			//TODO: Verify the descriptor
			delete des;
			}

		if (aWrite)
			{
			// Now write to the maximum length of the descriptor.
			TInt max = length;
			HBufC8* argTmp = HBufC8::New(max);
			if (!argTmp)
				return KErrNoMemory;

			TPtr8 argPtr = argTmp->Des();
			argPtr.SetLength(max);
			for (TInt i = 0; i < max; i++)
				argPtr[i] = (TUint8)argIndex;
			r = aMessage.Write(argIndex, argPtr);
			delete argTmp;
			if (r != KErrNone)
				return r;
			}
		}

	return KErrNone;
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
			RDBGD_PRINT(("Stopping server"));
			CActiveScheduler::Stop();
			break;

		case ERead:
			r = ReadWrite(aMessage, ETrue, EFalse);
			break;
		case EWrite:
			r = ReadWrite(aMessage, EFalse, ETrue);
			break;
		case EReadWrite:
			r = ReadWrite(aMessage, ETrue, ETrue);
			break;
	
		default:
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


TInt ServerThread(TAny* aThreadIndex)
//
// Passed as the server thread in 2 tests - sets up and runs CTestServer
//
	{
	RDBGD_PRINT(("ServerThread"));
	TUint threadIndex = (TUint)aThreadIndex;

	TBuf<16> serverName;
	serverName = _L("ServerName_");
	serverName.AppendNum(threadIndex);


	CMyActiveScheduler* pScheduler = new CMyActiveScheduler;
	if (pScheduler == NULL)
		{
		gServerArgsArray[threadIndex].iBadServer = ETrue;
		gServerArgsArray[threadIndex].iSemArray.Signal();
		return KErrNoMemory;
		}

	CActiveScheduler::Install(pScheduler);

	CTestServer* pServer = new CTestServer(0);
	if (pServer == NULL)
		{
		gServerArgsArray[threadIndex].iBadServer = ETrue;
		gServerArgsArray[threadIndex].iSemArray.Signal();
		delete pScheduler;
		return KErrNoMemory;
		}

	//Starting a CServer2 also Adds it to the ActiveScheduler
	TInt r = pServer->Start(serverName);
	if (r != KErrNone)
		{
		gServerArgsArray[threadIndex].iBadServer = ETrue;
		gServerArgsArray[threadIndex].iSemArray.Signal();
		delete pScheduler;
		delete pServer;
		return r;
		}

	RDBGD_PRINT(("Start ActiveScheduler and signal to client"));
	RDBGD_PRINT(("There might be something going on beneath this window\n"));
	gServerArgsArray[threadIndex].iSemArray.Signal();
	CActiveScheduler::Start();

	delete pScheduler;
	delete pServer;

	return KErrNone;
	}

TInt BadServerThread(TAny* /*aThreadIndex*/)
//
// Passed as the server thread in 2 tests - sets up and runs CTestServer
//
	{
	RDBGD_PRINT(("BadServerThread"));
	CMyActiveScheduler* pScheduler = new CMyActiveScheduler;
	if (pScheduler == NULL)
		{
		RDBGD_PRINT(("BST:Fail1"));
		gServerArgsArray[KSemServer].iBadServer = ETrue;
		gServerArgsArray[KSemServer].iSemArray.Signal();
		return KErrNoMemory;
		}

	CActiveScheduler::Install(pScheduler);

	CTestServer* pServer = new CTestServer(0);
	if (pServer == NULL)
		{
		RDBGD_PRINT(("BST:Fail2"));
		gServerArgsArray[KSemServer].iBadServer = ETrue;
		gServerArgsArray[KSemServer].iSemArray.Signal();
		delete pScheduler;
		return KErrNoMemory;
		}

	//pServer->SetPinClientDescriptors(ETrue);


	//Starting a CServer2 also Adds it to the ActiveScheduler
	TInt r = pServer->Start(KTestServer);
	if (r != KErrNone)
		{
		RDBGD_PRINT(("BST:Fail3"));
		gServerArgsArray[KSemServer].iBadServer = ETrue;
		gServerArgsArray[KSemServer].iSemArray.Signal();
		delete pScheduler;
		delete pServer;
		return r;
		}

	RDBGD_PRINT(("Start ActiveScheduler and signal to client"));
	RDBGD_PRINT(("There might be something going on beneath this window\n"));
	gServerArgsArray[KSemServer].iSemArray.Signal();
	CActiveScheduler::Start();

	delete pScheduler;
	delete pServer;
	RDBGD_PRINT(("BST:Pass1"));
	return KErrNone;
	}

TInt SendMessages(TUint aIters, TUint aSize, TDesC& aServerName, TInt aIndex, TBool aLowMem = EFalse)
//
// Passed as the first client thread - signals the server to do several tests
//
	{
	HBufC8* argTmp1;
	HBufC8* argTmp2;
	HBufC8* argTmp3;
	HBufC8* argTmp4;

	DOTEST((argTmp1 = HBufC8::New(aSize)), (argTmp1 != NULL));
	*argTmp1 = (const TUint8*)"argTmp1";
	TPtr8 ptr1 = argTmp1->Des();

	DOTEST((argTmp2 = HBufC8::New(aSize)), (argTmp2 != NULL));
	*argTmp2 = (const TUint8*)"argTmp2";
	TPtr8 ptr2 = argTmp2->Des();

	DOTEST((argTmp3 = HBufC8::New(aSize)), (argTmp3 != NULL));
	*argTmp3 = (const TUint8*)"argTmp3";
	TPtr8 ptr3 = argTmp3->Des();

	DOTEST((argTmp4 = HBufC8::New(aSize)), (argTmp1 != NULL));
	*argTmp4 = (const TUint8*)"argTmp4";
	TPtr8 ptr4 = argTmp4->Des();
	
	RSession session;
	TInt r = KErrNone;
	if(gTestBadServer)
		{//Don't do bad server tests with lowmem
		r = session.PublicCreateSession(aServerName,5);
		}
	else
		{
		DOTEST((r = session.PublicCreateSession(aServerName,5)), (r != KErrNoMemory));
		}
	if (r != KErrNone)
		{
		RDBGD_PRINT(("SendMessages[%d] failed to create session r = %d", aIndex, r));
		return r;
		}
	
	if(gTestBadServer)
		{
		RThread::Rendezvous(KErrNone);
		RDBGD_PRINT(("Wait on sem %d", aIndex));
		//gServerArgsArray[KSemCliSessStarted].iSemArray.Wait();
		}
	
	RDBGD_PRINT(("ID (%d)ReadWrite" ,aIndex));
	for (TUint i = 0; i < aIters; i++)
		{
		TUint mode = (i&0x3) + CTestSession::ERead;
		switch(mode)
			{
			case CTestSession::ERead:
				DOTEST((r = session.PublicSendReceive(CTestSession::ERead, TIpcArgs(&ptr1, &ptr2, &ptr3, &ptr4).PinArgs())), 
						(r != KErrNoMemory));
				if (r != KErrNone)
					return r;
				break;

			case CTestSession::EWrite:
				DOTEST((r = session.PublicSendReceive(CTestSession::EWrite, TIpcArgs(&ptr1, &ptr2, &ptr3, &ptr4).PinArgs())), 
						(r != KErrNoMemory));
				if (r != KErrNone)
					return r;
				break;
			case CTestSession::EReadWrite:
				DOTEST((r = session.PublicSendReceive(CTestSession::EReadWrite, TIpcArgs(&ptr1, &ptr2, &ptr3, &ptr4).PinArgs())), 
						(r != KErrNoMemory));
				if (r != KErrNone)
					return r;
				break;

			}
		}
	RDBGD_PRINT(("ID(%d) Closing session", aIndex));
	session.Close();
	return r;
	}

TInt TestIPCPinning(SPerformTestArgs& aTestArguments)
	{
	TInt r = KErrNone;
	// Create the server thread it needs to have a unpaged stack and heap.
	TBuf<16> serverThreadName;
	serverThreadName = _L("ServerThread_");
	serverThreadName.AppendNum(aTestArguments.iThreadIndex);
	TThreadCreateInfo serverInfo(serverThreadName, ServerThread, KDefaultStackSize, (TAny *) aTestArguments.iThreadIndex);
	serverInfo.SetUseHeap(NULL);
	
	gServerArgsArray[aTestArguments.iThreadIndex].iBadServer = EFalse;

	// Create the semaphores for the IPC pinning tests
	DOTEST1((r = gServerArgsArray[aTestArguments.iThreadIndex].iSemArray.CreateLocal(0)), (r != KErrNoMemory));
	if (r != KErrNone)
		{
		RDBGD_PRINT(("Failed to create semaphonre[%d] r = %d", aTestArguments.iThreadIndex, r));
		return r;
		}

	RThread serverThread;
	TInt r1 = KErrNone;
	DOTEST1((r1 = serverThread.Create(serverInfo)), (r1 != KErrNoMemory));
	if (r1 != KErrNone)
		{
		RDBGD_PRINT(("Failed to create server thread[%d] r1 = %d", aTestArguments.iThreadIndex, r1));
		return r1;
		}
	TRequestStatus serverStat;
	serverThread.Logon(serverStat);
	serverThread.Resume();

	// Wait for the server to start and then create a session to it.
	TBuf<16> serverName;
	serverName = _L("ServerName_");
	serverName.AppendNum(aTestArguments.iThreadIndex);

	gServerArgsArray[aTestArguments.iThreadIndex].iSemArray.Wait();
	
	// First check that the server started successfully
	if (gServerArgsArray[aTestArguments.iThreadIndex].iBadServer)
		return KErrServerTerminated;

	RSession session;
	DOTEST1((r1 = session.PublicCreateSession(serverName,5)), (r1 != KErrNoMemory));
	if (r1 != KErrNone)
		{
		RDBGD_PRINT(("Failed to create session[%d] r1 = %d", aTestArguments.iThreadIndex, r1));
		return r1;
		}
	
	r1 = SendMessages(50, 10, serverName, aTestArguments.iThreadIndex, aTestArguments.iLowMem);
	if (r1 != KErrNone)
		{
		RDBGD_PRINT(("SendMessages[%d] r1 = %d", aTestArguments.iThreadIndex, r1));
		return r1;
		}
	TInt r2 = KErrNone;
	
	// Signal to stop ActiveScheduler and wait for server to stop.
	session.PublicSendReceive(CTestSession::EStop, TIpcArgs());
	session.Close();

	User::WaitForRequest(serverStat);
	if (serverThread.ExitType() == EExitKill &&
		serverThread.ExitReason() != KErrNone)	
		{
		r2 = serverThread.ExitReason();
		}
	if (serverThread.ExitType() != EExitKill)	
		{
		RDBGD_PRINT(("Server thread panic'd"));
		r2 = KErrGeneral;
		}

	serverThread.Close();
	gServerArgsArray[aTestArguments.iThreadIndex].iSemArray.Close();
		
	if (r1 != KErrNone)
		return r1;

	return r2;
	}

TInt ClientThread(TAny* aClientThread)
	{
	TInt r = KErrNone;
	
	TBuf<16> serverName;
	serverName = KTestServer;
	RDBGD_PRINT(("CT(%d):Sending Messages" ,aClientThread));
	r = SendMessages(500, 10, serverName, (TInt) aClientThread);
	if (r != KErrNone)
		{
		RDBGD_PRINT(("SendMessages[%d] r = %d", (TInt) aClientThread, r));
		return r;
		}
	return r;
	}

TInt TestIPCBadServer(SPerformTestArgs& aTestArguments)
	{
	TInt cliRet = KErrNone;
	TInt serRet = KErrNone;

	// Create the server thread it needs to have a unpaged stack and heap.
	TBuf<16> serverThreadName;
	serverThreadName = _L("BadServerThread");
	TThreadCreateInfo serverInfo(serverThreadName, BadServerThread, KDefaultStackSize, NULL);
	serverInfo.SetUseHeap(NULL);
	
	// Create the semaphores for the IPC pinning tests
	DOTEST1((serRet = gServerArgsArray[KSemServer].iSemArray.CreateLocal(0)), (serRet != KErrNoMemory));
	if (serRet != KErrNone)
		{
		RDBGD_PRINT(("Failed to create semaphonre[%d] serRet = %d", KSemServer, serRet));
		return serRet;
		}

	RThread serverThread;
	DOTEST1((serRet = serverThread.Create(serverInfo)), (serRet != KErrNoMemory));
	if (serRet != KErrNone)
		{
		RDBGD_PRINT(("Failed to create server thread serRet = %d", serRet));
		return serRet;
		}
	TRequestStatus serverStat;
	serverThread.Logon(serverStat);
	serverThread.Resume();

	// Wait for the server to start and then create a session to it.
	gServerArgsArray[KSemServer].iSemArray.Wait();
	
	// First check that the server started successfully
	if (gServerArgsArray[KSemServer].iBadServer)
		return KErrServerTerminated;


	//create client threads
	const TUint KNumClientThreads = 50;	
	RThread clientThreads[KNumClientThreads];
	TRequestStatus clientStarted[KNumClientThreads];
	TRequestStatus clientStats[KNumClientThreads];

	// Create the client threads
	TBuf<16> clientThreadName;
	TUint i;
	for (i = 0; i < KNumClientThreads; i++)
		{
		clientThreadName = _L("clientThread_");
		clientThreadName.AppendNum(i);
		TThreadCreateInfo clientInfo(clientThreadName, ClientThread, KDefaultStackSize, (TAny*)i);
		clientInfo.SetPaging(TThreadCreateInfo::EPaged);
		clientInfo.SetCreateHeap(KMinHeapSize, KMinHeapSize);
		cliRet = clientThreads[i].Create(clientInfo);
		if (cliRet != KErrNone)
			{
			RDBGD_PRINT(("Failed to create client thread [%d] cliRet = %d", i, cliRet));
			return cliRet;
			}
		clientThreads[i].Rendezvous(clientStarted[i]);	
		clientThreads[i].Logon(clientStats[i]);
		clientThreads[i].Resume();
		}
	
	// Wait for creation of the client thread sessions
	for (i = 0; i < KNumClientThreads; i++)
		{
		User::WaitForRequest(clientStarted[i]);
		if (clientStarted[i].Int() != KErrNone)
			return clientStarted[i].Int();
		}
	

	// Once the messages are being sent, create a session to the
	// same server and signal to stop ActiveScheduler
	RSession session;
	serRet = session.PublicCreateSession(KTestServer,5);
	if (serRet != KErrNone)
		{
		RDBGD_PRINT(("Failed to create session serRet = %d", serRet));
		return serRet;
		}
	session.PublicSendReceive(CTestSession::EStop, TIpcArgs());
	session.Close();

	// Wait for the client thread to end.
	cliRet = KErrNone;
	for (i = 0; i < KNumClientThreads; i++)
		{
		User::WaitForRequest(clientStats[i]);
		RDBGD_PRINT(("Thread complete clientStats[%d] = %d", i, clientStats[i].Int()));
		if (clientStats[i].Int() != KErrNone && 
			clientStats[i].Int() != KErrServerTerminated)
			{
			cliRet = clientStats[i].Int();
			}
		}

	// Check that the server ended correctly
	serRet = KErrNone;
	User::WaitForRequest(serverStat);
	if (serverThread.ExitType() == EExitKill &&
		serverThread.ExitReason() != KErrNone)	
		{
		serRet = serverThread.ExitReason();
		}
	if (serverThread.ExitType() != EExitKill)	
		{
		RDBGD_PRINT(("Server thread panic'd"));
		serRet = KErrGeneral;
		}

	// Close all the server thread and client threads
	for (i = 0; i < KNumClientThreads; i++)
		{
		clientThreads[i].Close();
		}
	serverThread.Close();
		
	if (cliRet != KErrNone)
		return cliRet;

	return serRet;
	}


//
// RemoveChunkAlloc
//
// Remove ALL chunks allocated
//
// @param aChunkArray The array that stores a reference to the chunks created.
// @param aChunkArraySize The size of aChunkArray.
//
void RemoveChunkAlloc(RChunk*& aChunkArray, TUint aChunkArraySize)
	{
	if (aChunkArray == NULL)
		{// The chunk array has already been deleted.
		return;
		}

	for (TUint i = 0; i < aChunkArraySize; i++)
		{
		if (aChunkArray[i].Handle() != NULL)
			{
			aChunkArray[i].Close();
			gChunksAllocd --;
			if (gChunksAllocd < gMaxChunks)
				gMaxChunksReached = EFalse;
			}
		}
	delete[] aChunkArray;
	aChunkArray = NULL;
	}	

TInt WriteToChunk(RChunk* aChunkArray, TUint aChunkArraySize)
	{
	for (TUint j = 0; j < aChunkArraySize; j++) 
		{
		if (aChunkArray[j].Handle() != NULL)
			{
			TUint32* base = (TUint32*)aChunkArray[j].Base();
			TUint32* end = (TUint32*)(aChunkArray[j].Base() + aChunkArray[j].Size());
			for (TUint32 k = 0; base < end; k++)
				{
				*base++ = k; // write index to the chunk
				}
			}		
		}
	return KErrNone;
	}

TUint32 ReadByte(volatile TUint32* aPtr)
	{
	return *aPtr;
	}

TInt ReadChunk(RChunk* aChunkArray, TUint aChunkArraySize)
	{
	for (TUint j=0; j < aChunkArraySize; j++) //Read all open chunks
		{
		if (aChunkArray[j].Handle() != NULL)
			{
			TUint32* base = (TUint32*)aChunkArray[j].Base();
			TUint32* end = (TUint32*)(aChunkArray[j].Base() + aChunkArray[j].Size());
			for (TUint32 k = 0; base < end; k++)
				{
				TUint value = ReadByte((volatile TUint32*)base++);
				if (value != k)
					{
					RDBGS_PRINT(("Read value incorrect expected 0x%x got 0x%x", k, value));
					return KErrGeneral;
					}
				}
			}		
		}
	return KErrNone;
	}


TInt CreateChunks(SPerformTestArgs& aTestArguments, RChunk*& aChunkArray, TUint aChunkArraySize)
	{
	TInt r = KErrNone;

	TUint chunkSize = 1 << gPageShift;

	// Allocate as many chunks as is specified, either with the default chunk size or a specified chunk size
	if (aChunkArray == NULL)
		{
		DOTEST1((aChunkArray = new RChunk[aChunkArraySize]), (aChunkArray != NULL));
		if (aChunkArray == NULL)
			return KErrNoMemory;
		}
	
	TChunkCreateInfo createInfo;
	createInfo.SetNormal(chunkSize, chunkSize);
	createInfo.SetPaging(TChunkCreateInfo::EPaged);


	// Create chunks for each RChunk with a NULL handle.
	for (TUint i = 0; i < aChunkArraySize; i++)
		{
		DOTEST1((r = aChunkArray[i].Create(createInfo)), (r != KErrNoMemory));
		if (r != KErrNone)
			{
			if (r == KErrOverflow)
				{
				gMaxChunks = gChunksAllocd;
				RDBGD_PRINT(("Max Chunks Allowed = %d", gMaxChunks));
				gMaxChunksReached = ETrue;
				}
			return r;
			}
		gChunksAllocd++;
		RDBGD_PRINT(("TID(%d) aChunkArray[%d], r = %d", aTestArguments.iThreadIndex, i, r));
		}
	RDBGD_PRINT(("TID(%d) created chunks r = %d", aTestArguments.iThreadIndex, r));
	
	return KErrNone;
	}
//
// TestChunkPaging
//
// Create a number of chunks and write to them
// read the chunk back to ensure the values are correct
//

TInt TestChunkPaging(SPerformTestArgs& aTestArguments)
	{
	TInt r = KErrNone;
	const TUint KNumChunks = 10;
	
	
	if(gMaxChunksReached)
		{// We cant create any more chunks as the max number has been reached
		return KErrNone;
		}

	RChunk* chunkArray = NULL;	
	r = CreateChunks(aTestArguments, chunkArray, KNumChunks);
	if (r != KErrNone)
		{
		if (r == KErrOverflow)
			{
			RDBGD_PRINT(("Max number of chunks reached"));
			RemoveChunkAlloc(chunkArray, KNumChunks);
			return KErrNone;
			}
		RDBGD_PRINT(("TID(%d) CreateChunks r = %d", aTestArguments.iThreadIndex, r));
		return r;
		}

	r = WriteToChunk(chunkArray, KNumChunks);
	if (r != KErrNone)
		{
		RemoveChunkAlloc(chunkArray, KNumChunks);
		RDBGD_PRINT(("TID(%d) WriteToChunk r = %d", aTestArguments.iThreadIndex, r));
		return r;
		}

	r = ReadChunk(chunkArray, KNumChunks);
	if (r != KErrNone)
		{
		RemoveChunkAlloc(chunkArray, KNumChunks);
		RDBGD_PRINT(("TID(%d) ReadChunk r = %d", aTestArguments.iThreadIndex, r));
		return r;
		} 
	RemoveChunkAlloc(chunkArray, KNumChunks);
	return KErrNone;
	}


//
// TestChunkCommit
//
// Create a chunk
// commit a page at a time, write to that page and then decommit the page
//

TInt TestChunkCommit(SPerformTestArgs& aTestArguments)
	{
	TInt r = KErrNone;
	RChunk testChunk;

	TUint chunkSize = 70 << gPageShift;

	TChunkCreateInfo createInfo;
	createInfo.SetDisconnected(0, 0, chunkSize);
	createInfo.SetPaging(TChunkCreateInfo::EPaged);
	DOTEST1((r = testChunk.Create(createInfo)), (r != KErrNoMemory));
	if (r != KErrNone)
		{
		return r;
		}
	TUint offset = 0;
	while(offset < chunkSize)
		{
		// Commit a page
		DOTEST1((r = testChunk.Commit(offset,gPageSize)), (r != KErrNoMemory));
		if (r != KErrNone)
			{
			return r;
			}

		// Write to the page
		TUint8* pageStart = testChunk.Base() + offset;
		*pageStart = 0xed;


		// Decommit the page
		r = testChunk.Decommit(offset, gPageSize);
		if (r != KErrNone)
			{
			return r;
			}
		
		offset += gPageSize;
		}
	

	testChunk.Close();
	return r;
	}

//
// PerformTestThread
//
// This is the function that actually does the work.
// It is complicated a little because test.Printf can only be called from the first thread that calls it 
// so if we are using multiple threads we need to use a message queue to pass the debug info from the
// child threads back to the parent for the parent to then call printf.
//
//

LOCAL_C TInt PerformTestThread(SPerformTestArgs& aTestArguments)
	{
	TInt r = KErrNone;
	TUint start = User::TickCount();

	DEBUG_PRINT1((_L("%S : thread Starting %d\n"), &gTestNameBuffer, aTestArguments.iThreadIndex));
	// now select how we do the test...
	TInt	iterIndex = 0;


	if (TEST_ALL == (gTestWhichTests & TEST_ALL))
		{
		#define LOCAL_ORDER_INDEX1	6
		#define LOCAL_ORDER_INDEX2	4
		TInt	order[LOCAL_ORDER_INDEX1][LOCAL_ORDER_INDEX2] = {	{TEST_STACK, TEST_CHUNK,TEST_COMMIT, TEST_IPC},
																	{TEST_STACK, TEST_COMMIT,  TEST_CHUNK, TEST_IPC},
																	{TEST_CHUNK,TEST_STACK, TEST_COMMIT, TEST_IPC},
																	{TEST_CHUNK,TEST_COMMIT,  TEST_STACK, TEST_IPC},
																	{TEST_COMMIT,  TEST_STACK, TEST_CHUNK, TEST_IPC},
																	{TEST_COMMIT,  TEST_CHUNK,TEST_STACK, TEST_IPC}};
		TInt	whichOrder = 0;
		iterIndex = 0;
		for (iterIndex = 0; iterIndex < gPerformTestLoop; iterIndex ++)
			{
			DEBUG_PRINT1((_L("iterIndex = %d\n"), iterIndex));
			TInt    selOrder = ((aTestArguments.iThreadIndex + 1) * (iterIndex + 1)) % LOCAL_ORDER_INDEX1;
			for (whichOrder = 0; whichOrder < LOCAL_ORDER_INDEX2; whichOrder ++)
				{
				DEBUG_PRINT1((_L("whichOrder = %d\n"), whichOrder));
				switch (order[selOrder][whichOrder])
					{
					case TEST_STACK:
					DEBUG_PRINT1((_L("%S : %d Iter %d Stack\n"), &gTestNameBuffer, aTestArguments.iThreadIndex, iterIndex));
					r = TestStackPaging(aTestArguments);
					DEBUG_PRINT1((_L("ThreadId %d Finished TestStackPaging() r = %d\n"), aTestArguments.iThreadIndex, r));
					if (r != KErrNone)
						return r;
					break;

					case TEST_CHUNK:
					DEBUG_PRINT1((_L("%S : %d Iter %d Chunk\n"), &gTestNameBuffer, aTestArguments.iThreadIndex, iterIndex));
					r = TestChunkPaging(aTestArguments);
					DEBUG_PRINT1((_L("ThreadId %d Finished TestChunkPaging() r = %d\n"), aTestArguments.iThreadIndex, r));
					if (r != KErrNone)
						return r;
					break;

					case TEST_COMMIT:
					DEBUG_PRINT1((_L("%S : %d Iter %d Commit\n"), &gTestNameBuffer, aTestArguments.iThreadIndex, iterIndex));
					r = TestChunkCommit(aTestArguments);
					DEBUG_PRINT1((_L("ThreadId %d Finished TestChunkCommit() r = %d\n"), aTestArguments.iThreadIndex, r));
					if (r != KErrNone)
						return r;
					break;

					case TEST_IPC:
					
					if (gTestBadServer)
						{
						DEBUG_PRINT1((_L("%S : %d Iter %d IPC-BadServer\n"), &gTestNameBuffer, aTestArguments.iThreadIndex, iterIndex));
						r = TestIPCBadServer(aTestArguments);
						DEBUG_PRINT1((_L("ThreadId %d Finished TestIPCBadServer() r = %d\n"), aTestArguments.iThreadIndex, r));
						}
					else
						{
						DEBUG_PRINT1((_L("%S : %d Iter %d IPC\n"), &gTestNameBuffer, aTestArguments.iThreadIndex, iterIndex));
						// Limit the IPC pinning stuff to 2 loops else will take a long time to run
						if (gNumTestThreads > 1 && gPerformTestLoop > 2)
							break;
						r = TestIPCPinning(aTestArguments);
						DEBUG_PRINT1((_L("ThreadId %d Finished TestIPCPinning() r = %d\n"), aTestArguments.iThreadIndex, r));
						if (r != KErrNone)
							return r;
						}
					break;
					
					default: // this is really an error.
					break;
					}
				iterIndex++;
				}
			}
		}
	else
		{
		if (gTestWhichTests & TEST_STACK)
			{
			for (iterIndex = 0; iterIndex < gPerformTestLoop; iterIndex ++)
				{
				DEBUG_PRINT1((_L("%S : %d Iter %d Stack\n"), &gTestNameBuffer, aTestArguments.iThreadIndex, iterIndex));
				r = TestStackPaging(aTestArguments);
				DEBUG_PRINT1((_L("ThreadId %d Finished TestStackPaging() r = %d\n"), aTestArguments.iThreadIndex, r));
				if (r != KErrNone)
						return r;
				}
			}
			
		if (gTestWhichTests & TEST_CHUNK)
			{
			for (iterIndex = 0; iterIndex < gPerformTestLoop; iterIndex ++)
				{
				DEBUG_PRINT1((_L("%S : %d Iter %d Chunk\n"), &gTestNameBuffer, aTestArguments.iThreadIndex, iterIndex));
				r = TestChunkPaging(aTestArguments);
				DEBUG_PRINT1((_L("ThreadId %d Finished TestChunkPaging() r = %d\n"), aTestArguments.iThreadIndex, r));
				if (r != KErrNone)
						return r;
				}
			}

		if (gTestWhichTests & TEST_COMMIT)
			{
			for (iterIndex = 0; iterIndex < gPerformTestLoop; iterIndex ++)
				{
				DEBUG_PRINT1((_L("%S : %d Iter %d Commit\n"), &gTestNameBuffer, aTestArguments.iThreadIndex, iterIndex));
				r = TestChunkCommit(aTestArguments);
				DEBUG_PRINT1((_L("ThreadId %d Finished TestChunkCommit() r = %d\n"), aTestArguments.iThreadIndex, r));
				if (r != KErrNone)
					return r;
				}
			}

		if (gTestWhichTests & TEST_IPC)
			{
			// In multiple thread case limit IPC test to 2 loops else will take a long time
			TInt loops = (gPerformTestLoop <= 2 && gNumTestThreads) ? gPerformTestLoop : 2;
			for (iterIndex = 0; iterIndex < loops; iterIndex ++)
				{
				if (gTestBadServer)
					{
					r = TestIPCBadServer(aTestArguments);
					DEBUG_PRINT1((_L("ThreadId %d Finished TestIPCBadServer() r = %d\n"), aTestArguments.iThreadIndex, r));
					}
				else
					{
					DEBUG_PRINT1((_L("%S : %d Iter %d IPC\n"), &gTestNameBuffer, aTestArguments.iThreadIndex, iterIndex));
					r = TestIPCPinning(aTestArguments);
					DEBUG_PRINT1((_L("ThreadId %d Finished TestIPCPinning() r = %d\n"), aTestArguments.iThreadIndex, r));
					if (r != KErrNone)
						return r;
					}
				}
			}
		}
	
	DEBUG_PRINT1((_L("%S : thread Exiting %d (tickcount %u)\n"), &gTestNameBuffer, aTestArguments.iThreadIndex, (User::TickCount() - start)));
	return r;
	}


//
// MultipleTestThread
//
// Thread function, one created for each thread in a multiple thread test.
//

LOCAL_C TInt MultipleTestThread(TAny* aTestArgs)
	{
	TInt r = KErrNone;
	TBuf<64>					localBuffer;

	if (gTestInterleave)	
		{
		RThread				thisThread;
		thisThread.SetPriority((TThreadPriority) TEST_INTERLEAVE_PRIO);
		}
	
	SPerformTestArgs& testArgs = *(SPerformTestArgs*)aTestArgs;
	testArgs.iBuffer = &localBuffer;
	
	RDBGD_PRINT(("Performing test thread ThreadID(%d)\n", testArgs.iThreadIndex));
	r = PerformTestThread(testArgs);
	
	return r;
	}



//
// FindMMCDriveNumber
// 
// Find the first read write drive.
//

TInt FindMMCDriveNumber(RFs& aFs)
	{
	TDriveInfo driveInfo;
	for (TInt drvNum=0; drvNum<KMaxDrives; ++drvNum)
		{
		TInt r = aFs.Drive(driveInfo, drvNum);
		if (r >= 0)
			{
			if (driveInfo.iType == EMediaHardDisk)
				return (drvNum);
			}
		}
	return -1; 
	}

//
// PerformRomAndFileSystemAccess
// 
// Access the rom and dump it out to one of the writeable partitions...
// really just to make the media server a little busy during the test.
//

TInt PerformRomAndFileSystemAccessThread(SPerformTestArgs& aTestArguments)
	{
	TUint maxBytes = KMaxTUint;
	TInt startTime = User::TickCount();

	RFs fs;
	RFile file;
	if (KErrNone != fs.Connect())
		{
		DEBUG_PRINT(_L("PerformRomAndFileSystemAccessThread : Can't connect to the FS\n"));
		return KErrGeneral;
		}

	// get info about the ROM...
	TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
	TUint8* start;
	TUint8* end;
	if(romHeader->iPageableRomStart)
		{
		start = (TUint8*)romHeader + romHeader->iPageableRomStart;
		end = start + romHeader->iPageableRomSize;
		}
	else
		{
		start = (TUint8*)romHeader;
		end = start + romHeader->iUncompressedSize;
		}
	if (end <= start)
		return KErrGeneral;

	// read all ROM pages in a random order...and write out to file in ROFs, 
	TUint size = end - start - gPageSize;
	if(size > maxBytes)
		size = maxBytes;

	TUint32 random = 1;
	TPtrC8 rom;
	TUint8 *theAddr;

	//TInt		drvNum = TestBootedFromMmc ? FindMMCDriveNumber(fs) : FindFsNANDDrive(fs);
	TInt drvNum = FindMMCDriveNumber(fs);
	TBuf<32>	filename = _L("d:\\Pageldrtst.tmp");
	if (drvNum >= 0)
		{
		filename[0] = (TUint16)('a' + drvNum);
		DEBUG_PRINT1((_L("%S : Filename %S\n"), &gTestNameBuffer, &filename));
		}
	else
		DEBUG_PRINT((_L("PerformRomAndFileSystemAccessThread : error getting drive num\n")));

	for(TInt i = (size >> gPageShift); i > 0; --i)
		{
		DEBUG_PRINT1((_L("%S : Opening the file\n"), &gTestNameBuffer));
		if (KErrNone != file.Replace(fs, filename, EFileWrite))
			{
			DEBUG_PRINT1((_L("%S : Opening the file Failed!\n"), &gTestNameBuffer));
			}

		random = random * 69069 + 1;
		theAddr = (TUint8*)(start+((TInt64(random)*TInt64(size))>>32));
		if (theAddr + gPageSize > end)
			{
			DEBUG_PRINT1((_L("%S : address is past the end 0x%x / 0x%x\n"), &gTestNameBuffer, (TInt)theAddr, (TInt)end));
			}
		rom.Set(theAddr,gPageSize);
		DEBUG_PRINT1((_L("%S : Writing the file\n"), &gTestNameBuffer));
		TInt ret = file.Write(rom);
		if (ret != KErrNone)
			{
			DEBUG_PRINT1((_L("%S : Write returned error %d\n"), &gTestNameBuffer, ret));
			}
		DEBUG_PRINT1((_L("%S : Closing the file\n"), &gTestNameBuffer));
		file.Close();

		DEBUG_PRINT1((_L("%S : Deleting the file\n"), &gTestNameBuffer));
		ret = fs.Delete(filename);
		if (KErrNone != ret)
			{
			DEBUG_PRINT1((_L("%S : Delete returned error %d\n"), &gTestNameBuffer, ret));
			}
		if (gTestStopMedia)
			break;
		}
	fs.Close();
	DEBUG_PRINT1((_L("Done in %d ticks\n"), User::TickCount() - startTime));
	return KErrNone;
	}


//
// PerformRomAndFileSystemAccess
//
// Thread function, kicks off the file system access.
//

LOCAL_C TInt PerformRomAndFileSystemAccess(TAny* aTestArgs)
	{
	TBuf<64>					localBuffer;
	
	SPerformTestArgs& testArgs = *(SPerformTestArgs*)aTestArgs;
	testArgs.iBuffer = &localBuffer;
	
	PerformRomAndFileSystemAccessThread(testArgs);
	
	return KErrNone;
	}




//
// StartFlushing
//
// Create a thread that will continuously flush the paging cache
//
void StartFlushing(TRequestStatus &aStatus, RThread &aFlushThread, TBool aLowMem = EFalse)
	{
	TInt ret;
	gTestRunning = ETrue;

	TThreadCreateInfo flushThreadInfo(_L("FlushThread"), FlushFunc, KDefaultStackSize,NULL);
	flushThreadInfo.SetCreateHeap(KMinHeapSize, KMinHeapSize);
	
	if (!aLowMem)
		{
		test_KErrNone(aFlushThread.Create(flushThreadInfo));
		}
	else
		{
		DOTEST((ret = aFlushThread.Create(flushThreadInfo)), (ret != KErrNoMemory));
		test_KErrNone(ret);
		}
	
	
	aFlushThread.Logon(aStatus);
	
	aFlushThread.Resume();
	}

//
// FinishFlushing
//
// Close the thread flushing the paging cache
//
void FinishFlushing(TRequestStatus &aStatus, RThread &aFlushThread)
	{
	gTestRunning = EFalse;
	User::WaitForRequest(aStatus);
	// TO DO: Check Exit tyoe
	CLOSE_AND_WAIT(aFlushThread);
	}


//
// ResetResults
// 
// Clear the previous results from the results array
//
TInt ResetResults()
	{
	for (TUint i = 0; i < KMaxTestThreads; i++)
		{
		gResultsArray[i].iExitType = KExitTypeReset;
		gResultsArray[i].iExitReason = KErrNone;
		}
	return KErrNone;
	}


//
// CheckResults
//
// Check that the results are as expected
//
TInt CheckResults()
	{
	TUint i;
	for (i = 0; i < KMaxTestThreads; i++)
		{
		if (gResultsArray[i].iExitType == KExitTypeReset)
			continue;
		RDBGD_PRINT(("%S : Thread %d ExitType(%d) ExitReason(%d)...\n", 
					&gTestNameBuffer, i, gResultsArray[i].iExitType, gResultsArray[i].iExitReason));
		}
	
	for (i = 0; i < KMaxTestThreads; i++)
		{
		if (gResultsArray[i].iExitType == KExitTypeReset)
			continue;
		
		if (gResultsArray[i].iExitType != EExitKill)
			{
			RDBGS_PRINT(("Thread %d ExitType(%d) Expected(%d)\n", i, gResultsArray[i].iExitType, EExitKill));
			return KErrGeneral;
			}
		
		// Allow for No Memory as we can run out of memory due to high number of threads and
		// Overflow as the number of chunks that can be created on moving memory model is capped
		if (gResultsArray[i].iExitReason != KErrNone &&
			gResultsArray[i].iExitReason != KErrNoMemory &&
			gResultsArray[i].iExitReason != KErrOverflow)
			{
			RDBGS_PRINT(("Thread %d ExitReason(%d) Expected either %d, %d or %d\n", 
							i, gResultsArray[i].iExitReason, KErrNone, KErrNoMemory, KErrOverflow));
			return KErrGeneral;
			}
		}
	return KErrNone;
	}


//
// PrintOptions
//
// Print out the options of the test
//
void PrintOptions()
	{
	SVMCacheInfo  tempPages;
	if (gIsDemandPaged)
		{
		UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0);
		test.Printf(_L("PerformAutoTest : Start cache info: iMinSize 0x%x iMaxSize 0x%x iCurrentSize 0x%x iMaxFreeSize 0x%x\n"),
					 tempPages.iMinSize, tempPages.iMaxSize, tempPages.iCurrentSize ,tempPages.iMaxFreeSize);
		}
	
	test.Printf(_L("Loops (%d), Threads (%d), Tests: "), gPerformTestLoop, gNumTestThreads);
	if (TEST_ALL == (gTestWhichTests & TEST_ALL))
		{
		test.Printf(_L("All, "));
		}
	else if (gTestWhichTests & TEST_STACK)
		{
		test.Printf(_L("Stack, "));
		}
	else if (gTestWhichTests & TEST_CHUNK)
		{
		test.Printf(_L("Chunk, "));
		}
	else if (gTestWhichTests & TEST_COMMIT)
		{
		test.Printf(_L("Commit, "));
		}
	else if (gTestWhichTests & TEST_IPC)
		{
		test.Printf(_L("IPC Pinning, "));
		}
	else
		{
		test.Printf(_L("?, "));
		}
	test.Printf(_L("\nOptions: "));

	if(gTestInterleave)
		test.Printf(_L("Interleave "));
	if(gTestPrioChange)
		test.Printf(_L("Priority "));
	if(gTestMediaAccess)
		test.Printf(_L("Media"));
	if(gTestBadServer)
		test.Printf(_L("BadServer"));
	test.Printf(_L("\n"));
	}

// DoMultipleTest
// 
// Perform the multiple thread test, spawning a number of threads.
// It is complicated a little because test.Printf can only be called from the first thread that calls it 
// so if we are using multiple threads we need to use a message queue to pass the debug info from the
// child threads back to the parent for the parent to then call printf.
//
TInt DoMultipleTest(TBool aLowMem = EFalse)
	{
	SVMCacheInfo  tempPages;
	memset(&tempPages, 0, sizeof(tempPages));

	if (gIsDemandPaged)
		{
		// get the old cache info
		UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0);
		// set the cache to our test value
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)tempPages.iMinSize,(TAny*)(tempPages.iMaxSize * gNumTestThreads));
		}
	
	if (!TestSilent)
		PrintOptions();

	TUint startTime = User::TickCount();
	TInt			 index;
	TInt ret = KErrNone;
	TBuf<16> multiThreadName;
	TBuf<16> rerunThreadName;
	
	ResetResults();
	
	TRequestStatus flushStatus;
	RThread flushThread;
	StartFlushing(flushStatus, flushThread, aLowMem);

	DOTEST((gThreadHeap = User::ChunkHeap(NULL, 0x1000, 0x1000)), (gThreadHeap != NULL));
	test_NotNull(gThreadHeap);
	
	DOTEST((gStackHeap = User::ChunkHeap(NULL, 0x1000, 0x1000)), (gStackHeap != NULL));
	test_NotNull(gStackHeap);

	TThreadCreateInfo	*pThreadCreateInfo = (TThreadCreateInfo *)User::AllocZ(sizeof(TThreadCreateInfo) * gNumTestThreads);
	RThread				*pTheThreads  = (RThread *)User::AllocZ(sizeof(RThread) * gNumTestThreads);
	TInt				*pThreadInUse = (TInt *)User::AllocZ(sizeof(TInt) * gNumTestThreads);

	TRequestStatus	mediaStatus;
	RThread			mediaThread;
	
	
	DOTEST((ret = TestMsgQueue.CreateLocal(gNumTestThreads * 10, EOwnerProcess)),
	       (KErrNone == ret));

	DOTEST((ret = TestMultiSem.CreateLocal(1)),
	       (KErrNone == ret));

	// make sure we have a priority higher than that of the threads we spawn...
	RThread thisThread;
	TThreadPriority savedThreadPriority = thisThread.Priority();
	const TThreadPriority KMainThreadPriority = EPriorityMuchMore;
	__ASSERT_COMPILE(KMainThreadPriority>TEST_INTERLEAVE_PRIO);
	thisThread.SetPriority(KMainThreadPriority);

	SPerformTestArgs mediaArgs;
	mediaArgs.iMsgQueue = &TestMsgQueue; 
	mediaArgs.iTheSem = &TestMultiSem;
	mediaArgs.iLowMem = aLowMem;
	
	if (gTestMediaAccess)
		{
		TThreadCreateInfo mediaInfo(_L(""),PerformRomAndFileSystemAccess,KDefaultStackSize,(TAny*)&mediaArgs);
		mediaInfo.SetUseHeap(NULL);
		mediaInfo.SetPaging(TThreadCreateInfo::EPaged);
		gTestStopMedia = EFalse;
		ret = mediaThread.Create(mediaInfo);
		if (ret != KErrNone)
			return ret;
		mediaThread.Logon(mediaStatus);
		RUNTEST1(mediaStatus == KRequestPending);
		mediaThread.Resume();
		}

	TThreadCreateInfo** infoPtrs = new TThreadCreateInfo*[gNumTestThreads]; 
	if (infoPtrs == NULL)
		return KErrNoMemory;
	
	SPerformTestArgs *testArgs = new SPerformTestArgs[gNumTestThreads];
	if (testArgs == NULL)
		return KErrNoMemory;

	Mem::FillZ(testArgs, gNumTestThreads * sizeof(SPerformTestArgs));

	for (index = 0; index < gNumTestThreads; index++)
		{
		RDBGD_PRINT(("%S : Starting thread.%d!\n", &gTestNameBuffer, index));
		multiThreadName = _L("TestThread_");
		multiThreadName.AppendNum(index);

		testArgs[index].iThreadIndex = index;
		testArgs[index].iMsgQueue = &TestMsgQueue; 
		testArgs[index].iTheSem = &TestMultiSem;
		testArgs[index].iLowMem = aLowMem;

		RDBGD_PRINT(("Creating thread.%d!\n", index));
		infoPtrs[index] = new TThreadCreateInfo(multiThreadName, MultipleTestThread, KDefaultStackSize, (TAny*)&testArgs[index]);
		if (infoPtrs[index] == NULL)
			continue;
		infoPtrs[index]->SetCreateHeap(KMinHeapSize, KMinHeapSize);
		infoPtrs[index]->SetPaging(TThreadCreateInfo::EPaged);
		//infoPtrs[index]->SetUseHeap(gThreadHeap);
		DOTEST((ret = pTheThreads[index].Create(*infoPtrs[index])), (ret != KErrNoMemory));
		if (ret != KErrNone)
			continue;
		pTheThreads[index].Resume();
		pThreadInUse[index] = 1;
		}
	
	// now process any messages sent from the child threads.
	TBool		anyUsed = ETrue;
	TBuf<64>	localBuffer;
	while(anyUsed)
		{
		anyUsed = EFalse;
		// check the message queue and call printf if we get a message.
		while (KErrNone == TestMsgQueue.Receive(localBuffer))
			{
			if (!TestSilent)
				test.Printf(localBuffer);
			}

		// walk through the thread list to check which are still alive.
		for (index = 0; index < gNumTestThreads; index++)
			{
			if (pThreadInUse[index])
				{
				if (pTheThreads[index].ExitType() != EExitPending)
					{
					if (aLowMem &&
						pTheThreads[index].ExitType() == EExitKill &&
						pTheThreads[index].ExitReason() == KErrNoMemory &&
						Ldd.DoReleaseSomeRam(TEST_LM_BLOCKS_FREE) == KErrNone)
						{// If thread was killed with no memory in a low mem scenario
						// then release some RAM and restart the thread again
						anyUsed = ETrue;
						RDBGD_PRINT(("Thread index %d EExitKill KErrNoMemory\n", index));
						CLOSE_AND_WAIT(pTheThreads[index]);

						RDBGD_PRINT(("Re-running Thread index %d\n", index));
						rerunThreadName = _L("RRTestThread_");
						rerunThreadName.AppendNum(index);
						
						delete infoPtrs[index];				
						infoPtrs[index] = new TThreadCreateInfo(rerunThreadName, MultipleTestThread, KDefaultStackSize, (TAny*)&testArgs[index]);
						if (infoPtrs[index] == NULL)
							continue;
						infoPtrs[index]->SetCreateHeap(KMinHeapSize, KMinHeapSize);
						infoPtrs[index]->SetPaging(TThreadCreateInfo::EPaged);
						//infoPtrs[index]->SetUseHeap(gThreadHeap);
						ret = pTheThreads[index].Create(*infoPtrs[index]);
						if (ret != KErrNone)
							{
							pThreadInUse[index] = 0;
							continue;
							}
						pTheThreads[index].Resume();
						pThreadInUse[index] = 1;
						continue;
						}
					if (pTheThreads[index].ExitType() == EExitPanic)
						{
						RDBGD_PRINT(("%S : Thread Panic'd  %d...\n", &gTestNameBuffer, index));	
						}
					
					//Store the results but let all the threads finish
					gResultsArray[index].iExitType = pTheThreads[index].ExitType();
					gResultsArray[index].iExitReason = pTheThreads[index].ExitReason();
					
					pThreadInUse[index] = EFalse;
					pTheThreads[index].Close();
					}
				else
					{
					anyUsed = ETrue;
					}
				}
			}

		User::AfterHighRes(50000);
		}

	if (gTestMediaAccess)
		{
		gTestStopMedia = ETrue;
		RDBGD_PRINT(("%S : Waiting for media thread to exit...\n", &gTestNameBuffer));	
		User::WaitForRequest(mediaStatus);
		mediaThread.Close();
		}

	TestMsgQueue.Close();
	TestMultiSem.Close();

	// cleanup the resources and exit.
	User::Free(pTheThreads);
	User::Free(pThreadInUse);
	User::Free(pThreadCreateInfo);
	delete infoPtrs;
	delete testArgs;


	FinishFlushing(flushStatus, flushThread);
	gThreadHeap->Close();
	gStackHeap->Close();
	thisThread.SetPriority(savedThreadPriority);
	ret = CheckResults();
	RDBGS_PRINT(("Test Complete (%u ticks)\n", User::TickCount() - startTime));
	
	if (gIsDemandPaged)
		{
		// put the cache back to the the original values.
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)tempPages.iMinSize,(TAny*)tempPages.iMaxSize);
		}
	return ret;
	}


//
// DoSingleTest
// 
// Perform the single thread test,.
//

LOCAL_C TInt DoSingleTest(TBool aLowMem = EFalse)
	{
	TUint origThreadCount = gNumTestThreads;
	gNumTestThreads = 1;
	TInt r = DoMultipleTest(aLowMem);
	gNumTestThreads = origThreadCount;
	return r;
	}



//
// ParseCommandLine 
//
// read the arguments passed from the command line and set global variables to 
// control the tests.
//

TBool ParseCommandLine()
	{
	TBuf<256> args;
	User::CommandLine(args);
	TLex	lex(args);
	TBool	retVal = ETrue;
	
	// initially test for arguments, the parse them, if not apply some sensible defaults.
	TBool	foundArgs = EFalse;	
	
	FOREVER
		{
		TPtrC  token=lex.NextToken();
		if(token.Length()!=0)
			{
			if ((token == _L("help")) || (token == _L("-h")) || (token == _L("-?")))
				{
				RDBGS_PRINT(("\nUsage:  %S n", &gTestNameBuffer));
				RDBGS_PRINT(("\ndebug: Prints out tracing in the test"));
				RDBGS_PRINT(("\n[single | multiple <numThreads>] : Specify to run in a single thread or multiple threads and how many"));
				RDBGS_PRINT(("\n[ipc | stack | chunk| commit| all | badserver] : which type of test to run "));
				RDBGS_PRINT(("\n-> ipc: IPC Pinning tests"));
				RDBGS_PRINT(("\n-> stack: Stack paging tests"));
				RDBGS_PRINT(("\n-> chunk: Chunk paging tests"));
				RDBGS_PRINT(("\n-> commit: Chunk committing tests"));
				RDBGS_PRINT(("\n-> all: All the above tests"));
				RDBGS_PRINT(("\n-> badserver: IPC Pinning tests with a dead server"));
				RDBGS_PRINT(("\n[iters <iters>] : Number of loops each test should perform"));
				RDBGS_PRINT(("\n[media] : Perform multiple test with media activity in the background"));
				RDBGS_PRINT(("\n[lowmem] : Perform testing in low memory situations "));
				RDBGS_PRINT(("\n[interleave]: Perform test with thread interleaving\n\n"));
				test.Getch();
				TestExit = ETrue;
				break;
				}
			else if (token == _L("debug"))
				{
				if (!TestSilent)
					{
					TestDebug = ETrue;
					gTestPrioChange = ETrue;
					}
				}
			else if (token == _L("silent"))
				{
				TestSilent = ETrue;
				TestDebug = EFalse;
				}
			else if (token == _L("single"))
				{
				gTestType = ETestSingle;
				}
			else if (token == _L("multiple"))
				{
				TPtrC val=lex.NextToken();
				TLex lexv(val);
				TInt value;

				if (lexv.Val(value) == KErrNone)
					{
					if ((value <= 0) || (value > (TInt)KMaxTestThreads))
						{
						gNumTestThreads = KMaxTestThreads;
						}
					else
						{
						gNumTestThreads = value;
						}
					}
				else
					{
					RDBGS_PRINT(("Bad value for thread count '%S' was ignored.\n", &val));
					}
				gTestType = ETestMultiple;
				}
			else if (token == _L("prio"))
				{
				gTestPrioChange = !gTestPrioChange;
				}
			else if (token == _L("lowmem"))
				{
				gTestType = ETestLowMem;
				}
			else if (token == _L("media"))
				{
				gTestType = ETestMedia;
				}
			else if (token == _L("stack"))
				{
				gSetTests = TEST_STACK;
				}
			else if (token == _L("chunk"))
				{
				gSetTests = TEST_CHUNK;
				}
			else if (token == _L("commit"))
				{
				gTestType = ETestCommit;
				gSetTests = TEST_COMMIT;
				}
			else if (token == _L("ipc"))
				{
				gSetTests = TEST_IPC;
				}
			else if (token == _L("badserver"))
				{
				gTestType = ETestBadServer;
				}
			else if (token == _L("all"))
				{
				gSetTests = TEST_ALL;
				}
			else  if (token == _L("iters"))
				{
				TPtrC val=lex.NextToken();
				TLex lexv(val);
				TInt value;

				if (lexv.Val(value) == KErrNone)
					{
					gPerformTestLoop = value;
					}
				else
					{
					RDBGS_PRINT(("Bad value for loop count '%S' was ignored.\n", &val));
					retVal = EFalse;
					break;
					}
				}
			else  if (token == _L("interleave"))
				{
				gTestType = ETestInterleave;
				}
			else
				{
				if ((foundArgs == EFalse) && (token.Length() == 1))
					{
					// Single letter argument...only run on 'd'
					if (token.CompareF(_L("d")) == 0)
						{
						break;
						}
					else
						{
						if (!TestSilent)
							{
							test.Title();
							test.Start(_L("Skipping non drive 'd' - Test Exiting."));
							test.End();
							}
						foundArgs = ETrue;
						TestExit = ETrue;
						break;
						}
					}
				RDBGS_PRINT(("Unknown argument '%S' was ignored.\n", &token));
				break;
				}
			foundArgs = ETrue;
			}
		else
			{
			break;
			}
		}
	if (!foundArgs)
		{
		retVal = EFalse;
		}
	return retVal;
	}

//
// AreWeTheTestBase
//
// Test whether we are the root of the tests.
//

void AreWeTheTestBase(void)
	{
	if (!TestSilent)
		{
		TFileName  filename(RProcess().FileName());

		TParse	myParse;
		myParse.Set(filename, NULL, NULL);
		gTestNameBuffer.Zero();
		gTestNameBuffer.Append(myParse.Name());
		gTestNameBuffer.Append(_L(".exe"));

		TestWeAreTheTestBase = !gTestNameBuffer.Compare(_L("t_wdpstress.exe"));

		}
	else
		{
		gTestNameBuffer.Zero();
		gTestNameBuffer.Append(_L("t_wdpstress.exe"));
		}
	}

//
// PerformAutoTest
//
// Perform the autotest
//
TInt PerformAutoTest()
	{
	TInt r = KErrNone;

	// Run all the different types of test
	for (TUint testType = 0; testType < ETestTypes; testType++)
		{
		r = DoTest(testType);
		if (r != KErrNone)
			return r;
		}

	return r;
	}

//
// DoLowMemTest
//
// Low Memory Test
//

TInt DoLowMemTest()
	{
	TInt r = User::LoadLogicalDevice(KPageStressTestLddName);
	RUNTEST1(r==KErrNone || r==KErrAlreadyExists);
	RUNTEST(Ldd.Open(),KErrNone);
	
	SVMCacheInfo  tempPages;
	memset(&tempPages, 0, sizeof(tempPages));

	if (gIsDemandPaged)
		{
		// get the old cache info
		UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0);
		TInt	minSize = 8 << gPageShift;
		TInt	maxSize = 256 << gPageShift;
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minSize,(TAny*)maxSize);
		}


	// First load some pages onto the page cache 
	gPerformTestLoop = 1;
	r = DoTest(ETestSingle);
	test_KErrNone(r);


	Ldd.DoConsumeRamSetup(TEST_LM_NUM_FREE, TEST_LM_BLOCKSIZE);
	TEST_NEXT((_L("Single thread with Low memory.")));
	gNumTestThreads	= KMaxTestThreads / 2;
	gPerformTestLoop = 20;

	r = DoTest(ETestSingle, ETrue);
	Ldd.DoConsumeRamFinish();
	test_KErrNone(r);

	TEST_NEXT((_L("Multiple thread with Low memory.")));
	// First load some pages onto the page cache 
	gPerformTestLoop = 1;
	r = DoTest(ETestSingle);
	test_KErrNone(r);

	Ldd.DoConsumeRamSetup(TEST_LM_NUM_FREE, TEST_LM_BLOCKSIZE);
	
	gPerformTestLoop = 10;
	gNumTestThreads	= KMaxTestThreads / 2;
	r = DoTest(ETestMultiple, ETrue);
	Ldd.DoConsumeRamFinish();
	test_KErrNone(r);

	TEST_NEXT((_L("Multiple thread with Low memory, with starting free ram.")));
	// First load some pages onto the page cache 
	gPerformTestLoop = 1;
	r = DoTest(ETestSingle);
	test_KErrNone(r);

	Ldd.DoConsumeRamSetup(32, TEST_LM_BLOCKSIZE);
	
	gPerformTestLoop = 10;
	gNumTestThreads	= KMaxTestThreads / 2;
	r = DoTest(ETestMultiple, ETrue);
	Ldd.DoConsumeRamFinish();
	test_KErrNone(r);

	TEST_NEXT((_L("Close test driver")));
	Ldd.Close();
	RUNTEST(User::FreeLogicalDevice(KPageStressTestLddName), KErrNone);
	if (gIsDemandPaged)
		{
		TInt minSize = tempPages.iMinSize;
		TInt maxSize = tempPages.iMaxSize;
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minSize,(TAny*)maxSize);
		}

	return r;
	}

void RestoreDefaults()
	{
	gPerformTestLoop			= 10;					
	gNumTestThreads				= KMaxTestThreads;	

	gTestInterleave				= EFalse;
	
	gTestWhichTests				= gSetTests;
	gTestPrioChange				= EFalse;
	gTestStopMedia				= EFalse;
	gTestMediaAccess			= EFalse;
	}



TInt DoTest(TInt gTestType, TBool aLowMem)
	{
	TInt r = KErrNone;
	
	switch(gTestType)
		{
		case ETestSingle:
			TEST_NEXT((_L("Single thread")));
			r = DoSingleTest(aLowMem);
			break;

		case ETestMultiple:
			TEST_NEXT((_L("Multiple thread")));
			
			r = DoMultipleTest(aLowMem);
			break;

		case ETestLowMem:
			TEST_NEXT((_L("Low Memory Tests")));
			r = DoLowMemTest();
			break;

		case ETestMedia:
			TEST_NEXT((_L("Background Media Activity Tests")));
			gTestMediaAccess = ETrue;
			gPerformTestLoop = 2;					
			gNumTestThreads	= KMaxTestThreads / 2;	
			r = DoMultipleTest(aLowMem);
			break;

		case ETestCommit:
			TEST_NEXT((_L("Committing and Decommitting Tests")));	
			gTestWhichTests = TEST_COMMIT;
			r = DoSingleTest(aLowMem);
			break;

		case ETestInterleave:
			TEST_NEXT((_L("Testing multiple with thread interleaving")));
			gTestInterleave = ETrue;
			r = DoMultipleTest(aLowMem);
			break;

		case ETestBadServer:
			TEST_NEXT((_L("Testing multiple with thread interleaving")));
			gTestBadServer = ETrue;
			gTestWhichTests = TEST_IPC;
			r = DoSingleTest(aLowMem);
			break;


		}
	RestoreDefaults();
	return r;
	}
//
// E32Main
//
// Main entry point.
//

TInt E32Main()
	{
#ifndef TEST_ON_UNPAGED
	TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
	if(!romHeader->iPageableRomStart)
		{
		gIsDemandPaged = EFalse;
		}
#endif
	TUint start = User::TickCount();
	
	gResultsArray = (SThreadExitResults *)User::AllocZ(sizeof(SThreadExitResults) * KMaxTestThreads);
	if (gResultsArray == NULL)
		return KErrNoMemory;

	AreWeTheTestBase();
	RestoreDefaults();

	TBool parseResult = ParseCommandLine();

	if (TestExit)
		{
		return KErrNone;
		}

	// Retrieve the page size and use it to detemine the page shift (assumes 32-bit system).
	TInt r = HAL::Get(HAL::EMemoryPageSize, gPageSize);
	if (r != KErrNone)
		{
		RDBGS_PRINT(("Cannot obtain the page size\n"));
		return r;
		}
	else
		{
		RDBGS_PRINT(("page size = %d\n", gPageSize));
		}


	TUint32 pageMask = gPageSize;
	TUint i = 0;
	for (; i < 32; i++)
		{
		if (pageMask & 1)
			{
			if (pageMask & ~1u)
				{
				test.Printf(_L("ERROR - page size not a power of 2"));
				return KErrNotSupported;
				}
			gPageShift = i;
			break;
			}
		pageMask >>= 1;
		}

	TInt  minSize = 8 << gPageShift;
	TInt  maxSize = 64 << gPageShift; 
	SVMCacheInfo  tempPages;
	memset(&tempPages, 0, sizeof(tempPages));
	if (gIsDemandPaged)
		{
		// get the old cache info
		UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0);
		// set the cache to our test value
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minSize,(TAny*)maxSize);
		}


	if (!TestSilent)
		{
		test.Title();
		test.Start(_L("Writable Data Paging stress tests..."));
		test.Printf(_L("%S\n"), &gTestNameBuffer);
		}

	if (parseResult)
		{
		if (!TestSilent)
			{
			extern TInt *CheckLdmiaInstr(void);
			test.Printf(_L("%S : CheckLdmiaInstr\n"), &gTestNameBuffer);
			TInt   *theAddr = CheckLdmiaInstr();
			test.Printf(_L("%S : CheckLdmiaInstr complete 0x%x...\n"), &gTestNameBuffer, (TInt)theAddr);
			}
		
		if (gTestType < 0 || gTestType >= ETestTypeEnd)
			{
			r = PerformAutoTest();
			test_KErrNone(r);
			}
		else
			{
			r = DoTest(gTestType);
			test_KErrNone(r);
			}
		}
	else
		{
		r = PerformAutoTest();
		test_KErrNone(r);
		}

	if (gIsDemandPaged)
		{
		minSize = tempPages.iMinSize;
		maxSize = tempPages.iMaxSize;
		// put the cache back to the the original values.
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minSize,(TAny*)maxSize);
		}

	if (!TestSilent)
		{
		test.Printf(_L("%S : Complete (%u ticks)\n"), &gTestNameBuffer, User::TickCount() - start);	
		test.End();
		}

	User::Free(gResultsArray);
	gResultsArray = NULL;
	
	return 0;
	}


