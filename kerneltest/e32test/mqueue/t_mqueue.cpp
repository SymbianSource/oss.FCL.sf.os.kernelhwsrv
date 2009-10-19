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
// e32test\mqueue\t_mqueue.cpp
// Overview:
// Test message queuing
// API Information:
// RMsgQueue, RMsgQueueBase
// Details:
// - Create various illegal and legal private message queues and verify 
// results are as expected. Test private message queue functionality in
// both single threaded tests and multi-threaded tests.
// - Create various illegal and legal global named message queues and verify 
// results are as expected. Test global named message queue functionality
// in both single threaded tests and multi-threaded tests.
// - Test multi-process queues and template based queues, verify results are 
// as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32svr.h>
#include <e32msgqueue.h>
#include <f32file.h>

LOCAL_D RTest test(_L("t_mqueue"));

//if the test is to run under the debugger, uncomment the following line
//#define _DEBUGGER_BUILD

const TInt KHeapSize=0x2000;
const TInt KTestValue = 42;
_LIT8(KFillPattern, "1234567890");

_LIT(KGLobalName1, "GlobalMessageQueue1");


LOCAL_C void SingleThreadedTests(RMsgQueueBase& aQueue, TInt aSlots, TInt aSize)
	{
	test.Printf(_L("Single Threaded Tests"));


	TRequestStatus stat;
	test.Next(_L("test CancelDataAvailable"));
	aQueue.NotifyDataAvailable(stat);
	test (stat == KRequestPending);
	aQueue.CancelDataAvailable();
	User::WaitForRequest(stat);
	test (stat == KErrCancel);

	TUint8 * pSourceData = (TUint8*)User::Alloc(aSize*2);
	test(pSourceData != NULL);	
	TPtr8 pS(pSourceData, aSize*2, aSize*2);
	pS.Repeat(KFillPattern);

	TUint8 * pDestinationData = (TUint8*)User::Alloc(aSize*2);
	test(pDestinationData != NULL);
	TPtr8 pD(pDestinationData, aSize*2, aSize*2);
	pD.FillZ();


	test.Next(_L("test MessageSize"));
	test(aQueue.MessageSize() == aSize);


	test.Next(_L("Send a legal message through"));
	TInt ret = aQueue.Send(pSourceData, aSize);
	test(ret == KErrNone);

	test.Next(_L("Receive legal message"));
	ret = aQueue.Receive(pDestinationData, aSize);
	test(ret == KErrNone);
	TPtr8 p(pS);
	p.SetLength(aSize);
	pD.SetLength(aSize);
	test(p == pD);
	pD.FillZ();

	test.Next(_L("Send a short message through"));
	ret = aQueue.Send(pSourceData, aSize/2);
	test(ret == KErrNone);

	test.Next(_L("Receive legal message"));
	ret = aQueue.Receive(pDestinationData, aSize);
	test(ret == KErrNone);
	p.SetLength(aSize/2);
	pD.SetLength(aSize/2);
	test(p == pD);
	pD.FillZ();

	test.Next(_L("Test Receive with no message"));
	ret = aQueue.Receive(pDestinationData, aSize);
	test(ret == KErrUnderflow);

	if (aSlots >= 2)
		{	
		test.Next(_L("Send two legal messages through"));
		pS[0] = 0;
		ret = aQueue.Send(pSourceData, aSize);
		test(ret == KErrNone);
		pS[0] = 1;
		ret = aQueue.Send(pSourceData, aSize);
		test(ret == KErrNone);

		test.Next(_L("Receive two legal messages in tx order"));
		ret = aQueue.Receive(pDestinationData, aSize);
		test(ret == KErrNone);
		test(pD[0] == 0);
		pD.FillZ();
		ret = aQueue.Receive(pDestinationData, aSize);
		test(ret == KErrNone);
		test(pD[0] == 1);
		pD.FillZ();

		}
	
	test.Next(_L("Test filling the queue to the max"));
	TInt x;
	for (x = 0; x < aSlots; x++)
		{
		pS[0] = (TUint8)x;
		ret = aQueue.Send(pSourceData, aSize);
		test(ret == KErrNone);
		}

	test.Next(_L("Test one too many sends"));
	ret = aQueue.Send(pSourceData, aSize);
	test(ret == KErrOverflow);

	test.Next(_L("test cancel SpaceAvailable"));
	aQueue.NotifySpaceAvailable(stat);
	test (stat == KRequestPending);
	aQueue.CancelSpaceAvailable();
	User::WaitForRequest(stat);
	test (stat == KErrCancel);


	test.Next(_L("Test emptying the queue"));

	for (x = 0; x < aSlots; x++)
		{
		ret = aQueue.Receive(pDestinationData, aSize);
		test(ret == KErrNone);
		test(pD[0] == (TUint8)x );
		pD.FillZ();
		}

	test.Next(_L("test cancel DataAvailable"));
	aQueue.NotifyDataAvailable(stat);
	test (stat == KRequestPending);
	aQueue.CancelDataAvailable();
	User::WaitForRequest(stat);
	test (stat == KErrCancel);

	test.Next(_L("Test one too many receives"));
	ret = aQueue.Receive(pDestinationData, aSize);
	test(ret == KErrUnderflow);


	test.Next(_L("Test wrap around"));
	test.Printf(_L("fill queue to max\n"));
	for (x = 0; x < aSlots; x++)
		{
		pS[0] = (TUint8)x;
		ret = aQueue.Send(pSourceData, aSize);
		test(ret == KErrNone);
		}

	test.Printf(_L("half empty the queue\n"));
	for (x = 0; x < aSlots/2; x++)
		{
		ret = aQueue.Receive(pDestinationData, aSize);
		test(ret == KErrNone);
		test(pD[0] == (TUint8)x);
		pD.FillZ();
		}

	test.Printf(_L("fill queue to max\n"));
	for (x = 0; x < aSlots/2; x++)
		{
		pS[0] = (TUint8)x;
		ret = aQueue.Send(pSourceData, aSize);
		test (ret == KErrNone);
		}
		ret = aQueue.Send(pSourceData, aSize);
		test (ret == KErrOverflow);

	test.Printf(_L("empty the queue\n"));
	for (x = aSlots/2; x < aSlots; x++)
		{
		ret = aQueue.Receive(pDestinationData, aSize);
		test(ret == KErrNone);
		test(pD[0] == (TUint8)x);
		}
	for (x = 0; x < aSlots/2; x++)
		{
		ret = aQueue.Receive(pDestinationData, aSize);
		test(ret == KErrNone);
		test(pD[0] == (TUint8)x);
		}

	test.Next(_L("Test queue is empty"));
	ret = aQueue.Receive(pDestinationData, aSize);
	test(ret == KErrUnderflow);

	User::Free(pSourceData);
	User::Free(pDestinationData);
	}


_LIT(KThread2Name, "thread2");
_LIT(KThread3Name, "thread3");
_LIT(KThread4Name, "thread4");


class TData 
	{
public:
	TData(RMsgQueueBase* aQ, TInt aSize, TInt aSlots,TInt aTest=0, TAny* aData=NULL);
	RMsgQueueBase* iQueue;
	TInt iSize;
	TInt iSlots;
	TInt iTest;
	TAny* iData;
	};

TData::TData(RMsgQueueBase* aQ, TInt aSize, TInt aSlots, TInt aTest, TAny* aData) :	iQueue(aQ), iSize(aSize),
																		iSlots(aSlots), iTest(aTest), iData(aData)
	{
	//empty
	};



LOCAL_C TInt illegalSendEntryPoint(TAny* aData)
	{
	
	TData& data = *(TData *)aData;

	switch (data.iTest)
		{
	case 0:
		data.iQueue->Send(data.iData, data.iSize*2);	//should panic, message size incorrect
		break;

	case 1:
#ifdef _DEBUGGER_BUILD
		#pragma message ("BUILT FOR DEBUGGER")
		User::Panic(_L("test"),ECausedException);
#else
		data.iQueue->Send((TAny*)0xfeed, data.iSize);	//should panic
#endif
	break;

	case 2:
#ifdef _DEBUGGER_BUILD
		#pragma message ("BUILT FOR DEBUGGER")
		User::Panic(_L("test"),ECausedException);
#else
		data.iQueue->Send((TAny*)0xDEDEDEDE, data.iSize);	//dodgy address
#endif	
		break;
		}

	test(0);	//should never get here.  This'll make a Kern Exec 0! as tries to use console from different thread
	return 0;
	}


LOCAL_C TInt illegalReceiveEntryPoint(TAny* aData)
	{
	
	TData& data = *(TData *)aData;
	TUint8 buf[256];

	switch (data.iTest)
		{
	case 0:
		data.iQueue->Receive(buf, data.iSize*2);	//should panic, message size incorrect
		break;

	case 1:
#ifdef _DEBUGGER_BUILD
		#pragma message ("BUILT FOR DEBUGGER")
		User::Panic(_L("test"),ECausedException);
#else
		data.iQueue->Receive((TAny*)0xfeed, data.iSize);	//should panic
#endif
		break;

	case 2:
#ifdef _DEBUGGER_BUILD
		#pragma message ("BUILT FOR DEBUGGER")
		User::Panic(_L("test"),ECausedException);
#else
		data.iQueue->Receive((TAny*)0xDEDEDEDE, data.iSize);	//dodgy address
#endif
		break;

		}

	test(0);	//should never get here.  This'll make a Kern Exec 0!
	return 0;
	}



LOCAL_C TInt sendBlockingEntryPoint(TAny* aData)
	{
	TData& data = *(TData *)aData;

	TInt d = KTestValue;
	data.iQueue->SendBlocking(&d, 4);
	return KErrNone;
	}

LOCAL_C TInt receiveBlockingEntryPoint(TAny* aData)
	{
	TData& data = *(TData *)aData;

	TUint8  pData[256];
	TPtr8 pD(pData, data.iSize, data.iSize);
	pD.FillZ();
	data.iQueue->ReceiveBlocking(pData, data.iSize);
	test (*(TInt*)pData == KTestValue);
	return KErrNone;
	}


LOCAL_C TInt notifyDataAvailableEntryPoint(TAny* aData)
	{
	TData& data = *(TData *)aData;

	//check size as well
	test(data.iQueue->MessageSize() == data.iSize);

	TRequestStatus stat;
	data.iQueue->NotifyDataAvailable(stat);
	User::WaitForRequest(stat);
	return KErrNone;
	}

LOCAL_C TInt notifySpaceAvailableEntryPoint(TAny* aData)
	{
	TData& data = *(TData *)aData;

	TRequestStatus stat;
	data.iQueue->NotifySpaceAvailable(stat);
	User::WaitForRequest(stat);
	return KErrNone;
	}



LOCAL_C void MultiThreadedTests(RMsgQueueBase& aQueue, TInt aSlots, TInt aSize)
	{
	test.Next(_L("multi threaded tests"));
	RThread thread2;
	TInt ret = KErrNone;

	TAny* ptr = User::Alloc(aSize);

	test.Next(_L("test Send with illegal parameters"));
	TInt testnum;
	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	for (testnum = 0; testnum < 3; testnum++)	//testnum range is determined by the number of tests in illegalSendEntryPoint
		{
		TData data(&aQueue, aSize, aSlots, testnum, ptr);
		ret = thread2.Create(KThread2Name, illegalSendEntryPoint, KDefaultStackSize, KHeapSize, KHeapSize, &data);
		test(KErrNone == ret);
		TRequestStatus thread2stat;
		thread2.Logon(thread2stat);
		thread2.Resume();
		User::WaitForRequest(thread2stat);
		test (thread2.ExitType() == EExitPanic);
		switch (testnum)
		{
		case 0:
		test (thread2.ExitReason() == EMsgQueueInvalidLength);
		break;
		case 1:
		test (thread2.ExitReason() == ECausedException);
		break;
		case 2:
		test (thread2.ExitReason() == ECausedException);
		break;
		}

		CLOSE_AND_WAIT(thread2);
		}

	
	User::SetJustInTime(jit);

	
	test.Next(_L("test Receive with illegal parameters"));
	jit = User::JustInTime();
	User::SetJustInTime(EFalse);


	for (testnum = 0; testnum < 3; testnum++)	//testnum range is determined by the number of tests in illegalReceiveEntryPoint
		{
	//put something in the queue
		aQueue.Send(&testnum, 4);
		TData data(&aQueue, aSize, aSlots, testnum, ptr);
		ret = thread2.Create(KThread2Name, illegalReceiveEntryPoint, KDefaultStackSize, KHeapSize, KHeapSize, &data);
		test(KErrNone == ret);
		TRequestStatus thread2stat;
		thread2.Logon(thread2stat);
		thread2.Resume();
		User::WaitForRequest(thread2stat);
		test (thread2.ExitType() == EExitPanic);
		
		switch (testnum)
		{
		case 0:
		test (thread2.ExitReason() == EMsgQueueInvalidLength);
		break;
		case 1:
		test (thread2.ExitReason() == ECausedException);
		break;
		case 2:
		test (thread2.ExitReason() == ECausedException);
		break;
		}

		CLOSE_AND_WAIT(thread2);
		}

	
	User::SetJustInTime(jit);

	while(KErrNone == aQueue.Receive(ptr, aSize))	//empty the queue
		{
		//empty,
		}
	
	test.Next(_L("multi threaded NotifySpaceAvailable"));

	TInt dummydata = KTestValue;
	//fill the queue
	while (KErrNone == aQueue.Send(&dummydata, sizeof (TInt)))
		{
		//empty
		}

	TData data(&aQueue, aSize, aSlots);
	ret = thread2.Create(KThread2Name, notifySpaceAvailableEntryPoint, KDefaultStackSize, KHeapSize, KHeapSize, &data);
	test(KErrNone == ret);
	TRequestStatus thread2stat;
	thread2.Logon(thread2stat);
	thread2.Resume();

	//thread2 should be waiting for space available
	test (thread2stat == KRequestPending);
	aQueue.ReceiveBlocking(ptr, aSize);
	User::WaitForRequest(thread2stat);
	test (thread2stat == KErrNone);
	test (thread2.ExitType() == EExitKill);
	CLOSE_AND_WAIT(thread2);
	//thread 2 has exited OK

	//empty the queue
	while (KErrNone == aQueue.Receive(ptr, aSize))
		{
		//empty
		}

	
	test.Next(_L("multi threaded SendBlocking, ReceiveBlocking"));
	ret = thread2.Create(KThread2Name, receiveBlockingEntryPoint, KDefaultStackSize, KHeapSize, KHeapSize, &data);
	test(KErrNone == ret);
	thread2.Logon(thread2stat);
	thread2.Resume();

	aQueue.SendBlocking(&dummydata, sizeof (TInt));

	User::WaitForRequest(thread2stat);
	test (thread2.ExitType() == EExitKill);
	CLOSE_AND_WAIT(thread2);


	test.Next(_L("multiple ReceiveBlocking"));
	ret = thread2.Create(KThread2Name, receiveBlockingEntryPoint, KDefaultStackSize, KHeapSize, KHeapSize, &data);
	test(KErrNone == ret);

	RThread thread3;
	ret = thread3.Create(KThread3Name, receiveBlockingEntryPoint, KDefaultStackSize, KHeapSize, KHeapSize, &data);
	test(KErrNone == ret);

	RThread thread4;
	ret = thread4.Create(KThread4Name, receiveBlockingEntryPoint, KDefaultStackSize, KHeapSize, KHeapSize, &data);
	test(KErrNone == ret);

	thread2.Logon(thread2stat);

	TRequestStatus thread3stat;
	thread3.Logon(thread3stat);
	
	TRequestStatus thread4stat;
	thread4.Logon(thread4stat);

	thread2.Resume();
	User::After(500000);

	jit = User::JustInTime();
	User::SetJustInTime(EFalse);

	thread3.Resume();
	thread4.Resume();

	
	User::WaitForRequest(thread3stat, thread4stat);
	if (thread3stat != KRequestPending)
		User::WaitForRequest(thread4stat);
	else
		User::WaitForRequest(thread3stat);
	User::SetJustInTime(jit);

	//threads 3 and 4 have exited
	test (thread3.ExitType() == EExitPanic);
	test (thread3.ExitReason() == EMsgQueueRequestPending);
	test (thread4.ExitType() == EExitPanic);
	test (thread4.ExitReason() == EMsgQueueRequestPending);
	
	test (thread2stat == KRequestPending);
	aQueue.SendBlocking(&dummydata, sizeof (TInt));
	User::WaitForRequest(thread2stat);
	test (thread2stat == KErrNone);
	test (thread2.ExitType() == EExitKill);

	CLOSE_AND_WAIT(thread2);
	CLOSE_AND_WAIT(thread3);
	CLOSE_AND_WAIT(thread4);


	//fill the queue
	while (KErrNone == aQueue.Send(&dummydata, sizeof (TInt)))
		{
		//empty
		}

	test.Next(_L("multiple sendblocking"));
	ret = thread2.Create(KThread2Name, sendBlockingEntryPoint, KDefaultStackSize, KHeapSize, KHeapSize, &data);
	test(KErrNone == ret);
	ret = thread3.Create(KThread3Name, sendBlockingEntryPoint, KDefaultStackSize, KHeapSize, KHeapSize, &data);
	test(KErrNone == ret);
	ret = thread4.Create(KThread4Name, sendBlockingEntryPoint, KDefaultStackSize, KHeapSize, KHeapSize, &data);
	test(KErrNone == ret);

	thread2.Logon(thread2stat);
	thread3.Logon(thread3stat);
	thread4.Logon(thread4stat);

	thread2.Resume();
	User::After(500000);

	jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	thread3.Resume();
	thread4.Resume();
	User::WaitForRequest(thread3stat, thread4stat);
	if (thread3stat != KRequestPending)
		User::WaitForRequest(thread4stat);
	else
		User::WaitForRequest(thread3stat);
	User::SetJustInTime(jit);

	//threads 3 and 4 have exited
	test (thread3.ExitType() == EExitPanic);
	test (thread3.ExitReason() == EMsgQueueRequestPending);
	test (thread4.ExitType() == EExitPanic);
	test (thread4.ExitReason() == EMsgQueueRequestPending);
	
	test (thread2stat == KRequestPending);

	//consume one to allow the blocking write
	test(KErrNone == aQueue.Receive(ptr, aSize));

	User::WaitForRequest(thread2stat);
	test (thread2stat == KErrNone);
	test (thread2.ExitType() == EExitKill);

	//consume the rest of the queue
	while (KErrNone == aQueue.Receive(ptr, aSize))
		{
		// empty
		}

	CLOSE_AND_WAIT(thread2);
	CLOSE_AND_WAIT(thread3);
	CLOSE_AND_WAIT(thread4);

	
	test.Next(_L("multi threaded NotifyDataAvailable"));
	ret = thread2.Create(KThread2Name, notifyDataAvailableEntryPoint, KDefaultStackSize, KHeapSize, KHeapSize, &data);
	test(KErrNone == ret);
	thread2.Logon(thread2stat);
	thread2.Resume();

	//thread2 should be waiting for data available
	test (thread2stat == KRequestPending);
	aQueue.SendBlocking(&dummydata, sizeof (TInt));
	User::WaitForRequest(thread2stat);
	test (thread2stat == KErrNone);
	test (thread2.ExitType() == EExitKill);
	CLOSE_AND_WAIT(thread2);
	//thread 2 has exited OK

	//empty the queue
	aQueue.ReceiveBlocking(ptr, aSize);
	test (*(TInt*)ptr == dummydata);

	//create thread 2 again 
	ret = thread2.Create(KThread2Name, notifyDataAvailableEntryPoint, KDefaultStackSize, KHeapSize, KHeapSize, &data);
	test(KErrNone == ret);
	thread2.Logon(thread2stat);
	thread2.Resume();

	//create thread3
	ret = thread3.Create(KThread3Name, notifyDataAvailableEntryPoint, KDefaultStackSize, KHeapSize, KHeapSize, &data);
	test(KErrNone == ret);
	thread3.Logon(thread3stat);
	User::SetJustInTime(EFalse);
	User::After(10000);
	thread3.Resume();

	User::WaitForRequest(thread3stat);
	User::SetJustInTime(jit);
	
	test (thread3.ExitType() == EExitPanic);
	test (thread3.ExitReason() == EMsgQueueRequestPending);
	CLOSE_AND_WAIT(thread3);

	aQueue.SendBlocking(&dummydata, sizeof (TInt));
	User::WaitForRequest(thread2stat);
	test (thread2stat == KErrNone);
	test (thread2.ExitType() == EExitKill);
	CLOSE_AND_WAIT(thread2);

	//empty the queue
	aQueue.ReceiveBlocking(ptr, aSize);
	test (*(TInt*)ptr == dummydata);

	User::Free(ptr);
	}


class TTemplateTestData
	{
public:
	TTemplateTestData();
	TTemplateTestData(TInt a, TUint b, TUint8 c, TBool d, TInt e);
	TInt first;
	TUint second;
	TUint8 bob;
	TBool fred;
	TInt chipper;
	};

TTemplateTestData::TTemplateTestData()  : first(0), second(0), bob(0), fred(0), chipper(0)
	{
	}

TTemplateTestData::TTemplateTestData(TInt a, TUint b, TUint8 c, TBool d, TInt e) : first(a), second(b), bob(c), fred(d), chipper(e)
	{
	}


enum TQueueType {ECreateLocal, ECreateGlobal};

LOCAL_C TInt illegalQueueCreation(TAny* aData)
	{
	TData& data = *(TData *)aData;
	switch (data.iTest)
		{
		case ECreateLocal:	//CreateLocal
			{
			RMsgQueueBase queue;
			queue.CreateLocal(data.iSlots, data.iSize);
			break;
			}
		case ECreateGlobal:	//create global named
			{
			RMsgQueueBase queue;
			queue.CreateGlobal(KGLobalName1,  data.iSlots, data.iSize);
			break;
			}
		}
	test(0);	//should never get here.  This'll make a Kern Exec 0! as tries to use console from different thread
	return 0;
	}


LOCAL_C void TestIllegalCreation(TInt aSlots, TInt aSize, TQueueType aQueueType, TInt aExpectedReason)
		{
		RThread thread;
		TData data(NULL, aSize, aSlots, aQueueType, NULL);
		TRequestStatus threadstat;
		TBool jit = User::JustInTime();
		User::SetJustInTime(EFalse);
		TInt ret = thread.Create(KThread2Name, illegalQueueCreation, KDefaultStackSize, KHeapSize, KHeapSize, &data);
		test(KErrNone == ret);
		thread.Logon(threadstat);
		thread.Resume();
		User::WaitForRequest(threadstat);
		test (thread.ExitType() == EExitPanic);
		test (thread.ExitReason() == aExpectedReason);
		CLOSE_AND_WAIT(thread);
		User::SetJustInTime(jit);
		}

TInt DyingDataAvailableThread( TAny* )
	{
	RMsgQueue<TInt>	theQ;
	if( KErrNone != theQ.OpenGlobal(_L("TestNotifiedThreadDied")) )
		User::Panic( _L("TESTTH"), 0 );

	TRequestStatus stat;
	theQ.NotifyDataAvailable( stat );
	// now just exit
	return KErrNone;
	}

TInt DyingSpaceAvailableThread( TAny* )
	{
	RMsgQueue<TInt>	theQ;
	if( KErrNone != theQ.OpenGlobal(_L("TestNotifiedThreadDied")) )
		User::Panic( _L("TESTTH"), 0 );

	TRequestStatus stat;
	theQ.NotifySpaceAvailable( stat );
	// now just exit
	return KErrNone;
	}

struct TThreadParams
	{
	TInt imyQHandle;
	TRequestStatus* iRequest;
	};
	
TInt DyingRequestDataNotification(TAny* aThreadParams)
	{
	CTrapCleanup* trapHandler = CTrapCleanup::New();
	if(!trapHandler)
		return KErrNoMemory;

	TThreadParams* tp = reinterpret_cast<TThreadParams*>(aThreadParams);
	
	RMsgQueue<TInt> msgQue2;
	msgQue2.SetHandle(tp->imyQHandle);
	msgQue2.NotifyDataAvailable(*tp->iRequest);
	
	delete trapHandler;
	return KErrNone;
	}

void TestNotifiedThreadDied()
{
	RThread th;
	TRequestStatus stat;
	RMsgQueue<TInt>	myQ;
	test( KErrNone == myQ.CreateGlobal( _L("TestNotifiedThreadDied"), 1 ) );

	//Test when thread waiting on data available dies
	test( KErrNone == th.Create( _L("DyingDataAvailableThread"), DyingDataAvailableThread, 1024, 1024, 8192, NULL ) );
	th.Logon( stat );
	th.Resume();
	User::WaitForRequest( stat );
	test(stat.Int()==KErrNone);
	
	User::After( 1000000 );

	myQ.NotifyDataAvailable( stat );
	myQ.CancelDataAvailable();
	CLOSE_AND_WAIT(th);

	//Test when thread waiting on space available dies
	myQ.Send(0);//This will fill in the whole message queue and block any thread waiting on space available.

	test( KErrNone == th.Create( _L("DyingSpaceAvailableThread"), DyingSpaceAvailableThread, 1024, 1024, 8192, NULL ) );
	th.Logon( stat );
	th.Resume();
	User::WaitForRequest( stat );
	test(stat.Int()==KErrNone);
	
	User::After( 1000000 );

	myQ.NotifySpaceAvailable( stat );
	myQ.CancelSpaceAvailable();
	myQ.Close();
	CLOSE_AND_WAIT(th);

	// Calling cancel notification should not crash as the thread that requested notification dies
	test( KErrNone == myQ.CreateLocal(1, EOwnerProcess));

	TThreadParams tp;
	tp.imyQHandle = myQ.Handle();
	tp.iRequest = &stat;

	test( KErrNone == th.Create(_L("DyingRequestDataNotificationThread"), DyingRequestDataNotification, KDefaultStackSize, 
									KHeapSize, KHeapSize, reinterpret_cast<TAny*>(&tp)));
	TRequestStatus status;
	th.Logon(status);
	th.Resume();
	th.Close();

	User::WaitForRequest(status);
	test(status.Int() == KErrNone);

	myQ.CancelDataAvailable();
	myQ.Close();

}

LOCAL_C void RunTests(void)
	{
	TInt ret = KErrNone;
	test.Start(_L("Testing"));


	RMsgQueueBase mqueue;

//	LOCAL message queues


	test.Next(_L("Check when thread dies waiting to be notified."));
	TestNotifiedThreadDied();

	test.Next(_L("Create private message queue with 0 length params"));
	TestIllegalCreation(0,0,ECreateLocal, EMsgQueueInvalidLength);

	test.Next(_L("Create private message queue with 0  slots"));
	TestIllegalCreation(0,4,ECreateLocal, EMsgQueueInvalidSlots);

	test.Next(_L("Create private message queue with 0 size message"));
	TestIllegalCreation(5, 0, ECreateLocal, EMsgQueueInvalidLength);

	test.Next(_L("Create private message queue with none multiple of 4 size message"));
	TestIllegalCreation(5, 9, ECreateLocal, EMsgQueueInvalidLength);

	test.Next(_L("Create private message queue with illegal max length "));
	TestIllegalCreation(8,RMsgQueueBase::KMaxLength+1, ECreateLocal, EMsgQueueInvalidLength);


	test.Next(_L("Create private message queue, 43 slots, length 8"));
	ret = mqueue.CreateLocal(43,8, EOwnerThread);
	test (KErrNone == ret);
	mqueue.Close();

	test.Next(_L("Create private message queue with max length "));
	ret = mqueue.CreateLocal(8, RMsgQueueBase::KMaxLength, EOwnerProcess);
	test (KErrNone == ret);
	mqueue.Close();

	test.Next(_L("test private message queue functionality"));
	
	test.Printf(_L("two slots, small queue"));
	ret = mqueue.CreateLocal(2, 4);
	test(KErrNone == ret);
	SingleThreadedTests(mqueue, 2, 4);
	MultiThreadedTests(mqueue, 2, 4);
	mqueue.Close();

	test.Printf(_L("16 slots, max queue"));
	ret = mqueue.CreateLocal(16, RMsgQueueBase::KMaxLength);
	test(KErrNone == ret);
	SingleThreadedTests(mqueue, 16, RMsgQueueBase::KMaxLength);
	MultiThreadedTests(mqueue, 16, RMsgQueueBase::KMaxLength);
	mqueue.Close();

	test.Printf(_L("big slots, max queue"));
	ret = mqueue.CreateLocal(KMaxTInt, RMsgQueueBase::KMaxLength);
	test(KErrNoMemory == ret);


	/**************************************************************************/
//	GLOBAL Named message queues
	test.Next(_L("Create global named message queue with 0 length params"));
	TestIllegalCreation(0, 0, ECreateGlobal, EMsgQueueInvalidLength);

	test.Next(_L("Create global named message queue with 0  slots"));
	TestIllegalCreation(0, 4, ECreateGlobal, EMsgQueueInvalidSlots);

	test.Next(_L("Create global message queue with 0 size message"));
	TestIllegalCreation(5, 0, ECreateGlobal, EMsgQueueInvalidLength);

	test.Next(_L("Create global message queue with none multiple of 4 size message"));
	TestIllegalCreation(5, 9, ECreateGlobal, EMsgQueueInvalidLength);

	test.Next(_L("Create global named message queue with illegal max length "));
	TestIllegalCreation(8, RMsgQueueBase::KMaxLength+1, ECreateGlobal, EMsgQueueInvalidLength);

	test.Next(_L("Create global named message queue"));
	ret = mqueue.CreateGlobal(KGLobalName1, 10,8, EOwnerThread);
	test (KErrNone == ret);
	mqueue.Close();

	test.Next(_L("Create global named message queue with max length "));
	ret = mqueue.CreateGlobal(KGLobalName1, 8, RMsgQueueBase::KMaxLength, EOwnerProcess);
	test (KErrNone == ret);
	mqueue.Close();

	test.Next(_L("test global named message queue functionality"));
	
	test.Printf(_L("small queue, two slots"));
	ret = mqueue.CreateGlobal(KGLobalName1, 2, 4);
	test(KErrNone == ret);
	SingleThreadedTests(mqueue, 2, 4);
	MultiThreadedTests(mqueue, 2, 4);
	mqueue.Close();

	test.Printf(_L("max queue, 16 slots"));
	ret = mqueue.CreateGlobal(KGLobalName1, 16, RMsgQueueBase::KMaxLength);
	test(KErrNone == ret);
	SingleThreadedTests(mqueue, 16, RMsgQueueBase::KMaxLength);
	MultiThreadedTests(mqueue, 16, RMsgQueueBase::KMaxLength);
	mqueue.Close();

	test.Printf(_L("32byte queue, 1000 slots"));
	ret = mqueue.CreateGlobal(KGLobalName1, 1000, 32);
	test(KErrNone == ret);
	SingleThreadedTests(mqueue, 1000, 32);
	MultiThreadedTests(mqueue, 1000, 32);
	mqueue.Close();

	test.Printf(_L("12 byte queue, 1 slot"));
	ret = mqueue.CreateGlobal(KGLobalName1, 1, 12);
	test(KErrNone == ret);
	SingleThreadedTests(mqueue, 1, 12);
	MultiThreadedTests(mqueue, 1, 12);
	mqueue.Close();


	test.Printf(_L("max queue, maxint! slots"));
	ret = mqueue.CreateGlobal(KGLobalName1, KMaxTInt, RMsgQueueBase::KMaxLength);
	test(KErrNoMemory == ret);

	_LIT(KNonQueueName,"non-queue name");
	test.Printf(_L("open a non-existant queue"));
	ret = mqueue.OpenGlobal(KNonQueueName, EOwnerProcess);
	test(ret == KErrNotFound);


	ret = mqueue.CreateGlobal(KGLobalName1, 16, 4);
	test(KErrNone == ret);
	SingleThreadedTests(mqueue, 16, 4);
	MultiThreadedTests(mqueue, 16, 4);
	RMsgQueueBase open;

	ret = open.OpenGlobal(KGLobalName1);
	test(KErrNone == ret);
	SingleThreadedTests(open, 16,4);
	MultiThreadedTests(open, 16, 4);


	test.Next(_L("Send a legal message through"));
	TInt src = 45;
	TInt dst = 0;
	ret = mqueue.Send(&src, sizeof (TInt));
	test(ret == KErrNone);

	test.Next(_L("Receive legal message"));
	ret = open.Receive(&dst, 4);
	test(ret == KErrNone);
	test (src == dst);

	test.Next(_L("Send a legal message through"));
	ret = mqueue.Send(&src, sizeof (TInt));
	test(ret == KErrNone);

	open.Close();
	mqueue.Close();
	

	ret = mqueue.CreateGlobal(KNullDesC, 5, 4);
	test(KErrNone == ret);
	SingleThreadedTests(mqueue, 5,4);
	MultiThreadedTests(mqueue, 5, 4);

	ret = open.OpenGlobal(KNullDesC);
	test(KErrNotFound == ret);

	mqueue.Close();
	

	test.Next(_L("Multi Process Queue Tests"));
	
_LIT(KQueueA, "A");
_LIT(KQueueB, "B");
_LIT(KProcessName, "T_MQUEUEECHO.EXE");

	RMsgQueueBase inQueue;
	RMsgQueueBase outQueue;

	TInt sizes[6] = {4,8,16,32,100,256};

	TInt x;
	for (x = 0; x < 6; x++)
		{
		TUint8* p = (TUint8*)User::Alloc(sizes[x]);
		TRequestStatus stat;
		test (p != NULL);
		ret = inQueue.CreateGlobal(KQueueB, 1, sizes[x]);
		test (KErrNone == ret);
		ret = outQueue.CreateGlobal(KQueueA, 1, sizes[x]);
		test (KErrNone == ret);

		//start other process
		RProcess proc;
		ret = proc.Create(KProcessName, KNullDesC);
		test (KErrNone == ret);

		//logon to it
		proc.Logon(stat);

		proc.Resume();

		TInt y[64] = {1000};

		while (--y[0] >= 0)
			{
			outQueue.SendBlocking(&y,sizes[x]);
			inQueue.ReceiveBlocking(p, sizes[x]);
			test (y[0] == *(TInt*)p);
			}
		
		User::Free(p);
		inQueue.Close();
		outQueue.Close();

		//wait for the process to terminate
		User::WaitForRequest(stat);
		test(stat == KErrNone);
		CLOSE_AND_WAIT(proc);
		}

	test.Next(_L("test templated queue"));
	RMsgQueue<TTemplateTestData> templateQueue;
	TTemplateTestData ch(1,2,3,ETrue,4);
	TTemplateTestData ch2;
	TTemplateTestData ch3;

	test(KErrNone == templateQueue.CreateLocal(12));

	test (KErrNone == templateQueue.Send(ch));
	test (ch.first != ch2.first);
	test (ch.chipper != ch2.chipper);
	test (KErrNone == templateQueue.Receive(ch2));
	test (ch.first == ch2.first);
	test (ch.chipper == ch2.chipper);

	templateQueue.SendBlocking(ch);
	test (ch.first != ch3.first);
	test (ch.chipper != ch3.chipper);
	templateQueue.ReceiveBlocking(ch3);
	test (ch.first == ch3.first);
	test (ch.chipper == ch3.chipper);

	templateQueue.Close();

	test(KErrNone == templateQueue.CreateGlobal(KNullDesC, 79));
	templateQueue.Close();

_LIT(KTestName, "testQueue");

	test(KErrNone == templateQueue.CreateGlobal(KTestName, 986));

	RMsgQueue<TTemplateTestData> templateQueue2;
	test(KErrNone == templateQueue2.OpenGlobal(KTestName));
	templateQueue.Close();
	templateQueue2.Close();

	
	test.Next(_L("Ending test.\n"));
	test.End();
	
	test.Close();
	}

GLDEF_C TInt E32Main()
//
//
    {
	test.Title();

	// Turn off evil lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();
	
	RunTests();
	return KErrNone;
    }

