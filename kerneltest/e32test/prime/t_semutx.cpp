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
// e32test\prime\t_semutx.cpp
// Tests the RSemaphore, RMutex and RCriticalSection classes
// Overview:
// Tests the RSemaphore, RMutex and RCriticalSection classes
// API Information:
// RSemaphore, RMutex, RCriticalSection
// Details:
// - Test RSemaphore and RMutex with the producer/consumer scenario.
// Create two threads, use signal and wait to coordinate the
// threads. Verify results are as expected.
// - Calculate the time required to create, resume and close a thread.
// - Test RSemaphore::Wait(timeout) in a variety ways and timeout 
// values. Verify results are as expected.
// - Test RMutex via two threads which write to an array. The writing
// and updating of the index is wrapped within a mutex pair. Verify 
// results are as expected.
// - Test RCriticalSection via two threads which write to an array. The 
// writing and updating of the index is wrapped within a critical section
// pair. Verify results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <u32std.h>
#include <e32svr.h>

const TInt KMaxBufferSize=10;
const TInt KMaxArraySize=10;
const TInt KNumProducerItems=100;

enum {EThread1ID=1,EThread2ID};

RTest test(_L("T_SEMUTX"));
RMutex mutex;
RCriticalSection criticalSn;	
TInt thread1Count,thread2Count;
TInt arrayIndex;
TInt array[KMaxArraySize];  
TInt consumerArray[KNumProducerItems];
RSemaphore slotAvailable,itemAvailable;  
			 
class CStack
	{
public:	   
	CStack() {iCount=0;};
	void Push(TInt aItem) {iStack[iCount++]=aItem;};
	TInt Pop(void) {return(iStack[--iCount]);};
private:
	TInt iStack[KMaxBufferSize];
	TInt iCount;
	};
CStack stack;


TInt Producer(TAny*)
	{
	for(TInt ii=0;ii<KNumProducerItems;ii++)
		{
		slotAvailable.Wait();
		mutex.Wait();
		stack.Push(ii);
		mutex.Signal();
		itemAvailable.Signal();
		}
	return(KErrNone);
	}

TInt Consumer(TAny*)
	{
	TInt item;
	for(TInt ii=0;ii<KNumProducerItems;ii++)
		{
		itemAvailable.Wait();
		mutex.Wait();
		item=stack.Pop();
		mutex.Signal();
		slotAvailable.Signal();
		consumerArray[item]=item;
		}
	return(KErrNone);
	}

void BusyWait(TInt aMicroseconds)
	{
	TTime begin;
	begin.HomeTime();
	FOREVER
		{
		TTime now;
		now.HomeTime();
		TTimeIntervalMicroSeconds iv=now.MicroSecondsFrom(begin);
		if (iv.Int64()>=TInt64(aMicroseconds))
			return;
		}
	}

TInt MutexThreadEntryPoint1(TAny*)
//
// Mutex test thread 1
//
	{	

	thread1Count=0;
	TBool running=ETrue;
	do
		{
		mutex.Wait();
		BusyWait(100000);
		if (arrayIndex<KMaxArraySize)
			{
			array[arrayIndex++]=EThread1ID;
			thread1Count++;
			}
		else
			running=EFalse;
		mutex.Signal();
		} while (running);
	return(KErrNone);
	}

TInt MutexThreadEntryPoint2(TAny*)
//
// Mutex test thread 2
//
	{

	thread2Count=0;
	TBool running=ETrue;
	do
		{
		mutex.Wait();
		BusyWait(200000);
		if (arrayIndex<KMaxArraySize)
			{
			array[arrayIndex++]=EThread2ID;
			thread2Count++;
			}
		else
			running=EFalse;
		mutex.Signal();
		} while (running);
	return(KErrNone);
	}

TInt CriticalSnThreadEntryPoint1(TAny*)
//
// Critical Section test thread 1
//
	{	

	thread1Count=0;
	TBool running=ETrue;
	do
		{
		criticalSn.Wait();
		User::After(100000);
		if (arrayIndex<KMaxArraySize)
			{
			array[arrayIndex++]=EThread1ID;
			thread1Count++;
			}
		else
			running=EFalse;
		criticalSn.Signal();
		} while (running);
	return(KErrNone);
	}

TInt CriticalSnThreadEntryPoint2(TAny*)
//
// Critical Section test thread 2
//
	{

	thread2Count=0;
	TBool running=ETrue;
	do
		{
		criticalSn.Wait();
		User::After(200000);
		if (arrayIndex<KMaxArraySize)
			{
			array[arrayIndex++]=EThread2ID;
			thread2Count++;
			}
		else
			running=EFalse;
		criticalSn.Signal();
		} while (running);
	return(KErrNone);
	}

struct SWaitSem
	{
	RSemaphore iSem;
	TInt iTimeout;
	};

TInt WaitSemThread(TAny* a)
	{
	SWaitSem& ws = *(SWaitSem*)a;
	return ws.iSem.Wait(ws.iTimeout);
	}

void StartWaitSemThread(RThread& aT, SWaitSem& aW, TThreadPriority aP=EPriorityLess)
	{
	TInt r = aT.Create(KNullDesC, &WaitSemThread, 0x1000, 0x1000, 0x1000, &aW);
	test_KErrNone(r);
	aT.SetPriority(aP);
	aT.Resume();
	}

void WaitForWaitSemThread(RThread& aT, TInt aResult)
	{
	TRequestStatus s;
	aT.Logon(s);
	User::WaitForRequest(s);
	test_Equal(EExitKill, aT.ExitType());
	test_Equal(aResult, aT.ExitReason());
	test_Equal(aResult, s.Int());
	CLOSE_AND_WAIT(aT);
	}

TInt DummyThread(TAny*)
	{
	return 0;
	}

void TestSemaphore2()
	{
	test.Start(_L("Test semaphore wait with timeout"));
	SWaitSem ws;
	RThread t;
	TTime initial;
	TTime final;
	TInt elapsed=0;
	TInt r = ws.iSem.CreateLocal(0);
	test_KErrNone(r);

	RThread().SetPriority(EPriorityAbsoluteVeryLow);
	TInt threadcount=0;
	initial.HomeTime();
	while (elapsed<1000000)
		{
		r = t.Create(KNullDesC, &DummyThread, 0x1000, NULL, NULL);
		test_KErrNone(r);
		t.SetPriority(EPriorityMore);
		t.Resume();
		t.Close();
		++threadcount;
		final.HomeTime();
		elapsed = I64INT(final.Int64()-initial.Int64());
		}
	RThread().SetPriority(EPriorityNormal);
	test.Printf(_L("%d threads in 1 sec\n"),threadcount);
	TInt overhead = 1000000/threadcount;
	test.Printf(_L("overhead = %dus\n"),overhead);

	ws.iTimeout=1000000;
	initial.HomeTime();
	StartWaitSemThread(t, ws);
	WaitForWaitSemThread(t, KErrTimedOut);
	final.HomeTime();
	elapsed = I64INT(final.Int64()-initial.Int64());
	test.Printf(_L("Time taken = %dus\n"), elapsed);
	test(elapsed>=900000+overhead && elapsed<1500000+overhead);

	ws.iTimeout=-1;
	initial.HomeTime();
	StartWaitSemThread(t, ws);
	WaitForWaitSemThread(t, KErrArgument);
	final.HomeTime();
	elapsed = I64INT(final.Int64()-initial.Int64());
	test.Printf(_L("Time taken = %dus\n"), elapsed);

	ws.iTimeout=2000000;
	initial.HomeTime();
	StartWaitSemThread(t, ws);
	User::After(1000000);
	ws.iSem.Signal();
	WaitForWaitSemThread(t, KErrNone);
	final.HomeTime();
	elapsed = I64INT(final.Int64()-initial.Int64());
	test.Printf(_L("Time taken = %dus\n"), elapsed);
	test(elapsed>=900000+overhead && elapsed<1500000+overhead);

	ws.iTimeout=100000;
	StartWaitSemThread(t, ws, EPriorityMore);
	t.Suspend();
	ws.iSem.Signal();
	User::After(200000);
	t.Resume();
	WaitForWaitSemThread(t, KErrTimedOut);
	test_KErrNone(ws.iSem.Wait(1));

	ws.iTimeout=100000;
	StartWaitSemThread(t, ws, EPriorityMore);
	t.Suspend();
	ws.iSem.Signal();
	User::After(50000);
	t.Resume();
	WaitForWaitSemThread(t, KErrNone);
	test_Equal(KErrTimedOut, ws.iSem.Wait(1));

	RThread t2;
	ws.iTimeout=100000;
	StartWaitSemThread(t, ws, EPriorityMuchMore);
	StartWaitSemThread(t2, ws, EPriorityMore);
	t.Suspend();
	ws.iSem.Signal();
	test_Equal(EExitKill, t2.ExitType());
	test_Equal(EExitPending, t.ExitType());
	t.Resume();
	WaitForWaitSemThread(t, KErrTimedOut);
	WaitForWaitSemThread(t2, KErrNone);
	test_Equal(KErrTimedOut, ws.iSem.Wait(1));

	ws.iTimeout=1000000;
	initial.HomeTime();
	StartWaitSemThread(t2, ws, EPriorityMore);
	StartWaitSemThread(t, ws, EPriorityMuchMore);
	ws.iSem.Signal();
	WaitForWaitSemThread(t, KErrNone);
	final.HomeTime();
	elapsed = I64INT(final.Int64()-initial.Int64());
	test.Printf(_L("Time taken = %dus\n"), elapsed);
	WaitForWaitSemThread(t2, KErrTimedOut);
	final.HomeTime();
	elapsed = I64INT(final.Int64()-initial.Int64());
	test.Printf(_L("Time taken = %dus\n"), elapsed);
	test(elapsed>=900000+2*overhead && elapsed<1500000+2*overhead);

	ws.iTimeout=1000000;
	initial.HomeTime();
	StartWaitSemThread(t2, ws, EPriorityMore);
	StartWaitSemThread(t, ws, EPriorityMuchMore);
	WaitForWaitSemThread(t, KErrTimedOut);
	final.HomeTime();
	elapsed = I64INT(final.Int64()-initial.Int64());
	test.Printf(_L("Time taken = %dus\n"), elapsed);
	WaitForWaitSemThread(t2, KErrTimedOut);
	final.HomeTime();
	elapsed = I64INT(final.Int64()-initial.Int64());
	test.Printf(_L("Time taken = %dus\n"), elapsed);
	test(elapsed>=900000+2*overhead && elapsed<1500000+2*overhead);

	ws.iTimeout=1000000;
	initial.HomeTime();
	StartWaitSemThread(t2, ws, EPriorityMore);
	StartWaitSemThread(t, ws, EPriorityMuchMore);
	t.Kill(299792458);
	WaitForWaitSemThread(t2, KErrTimedOut);
	WaitForWaitSemThread(t, 299792458);
	final.HomeTime();
	elapsed = I64INT(final.Int64()-initial.Int64());
	test.Printf(_L("Time taken = %dus\n"), elapsed);
	test(elapsed>=900000+2*overhead && elapsed<1500000+2*overhead);

	ws.iTimeout=1000000;
	initial.HomeTime();
	StartWaitSemThread(t, ws, EPriorityMore);
	StartWaitSemThread(t2, ws, EPriorityMuchMore);
	test_Equal(EExitPending, t.ExitType());
	test_Equal(EExitPending, t2.ExitType());
	ws.iSem.Close();
	test_Equal(EExitKill, t.ExitType());
	test_Equal(EExitKill, t2.ExitType());
	WaitForWaitSemThread(t2, KErrGeneral);
	WaitForWaitSemThread(t, KErrGeneral);
	final.HomeTime();
	elapsed = I64INT(final.Int64()-initial.Int64());
	test.Printf(_L("Time taken = %dus\n"), elapsed);
	test(elapsed<=50000+3*overhead);

	test.End();
	}

void TestSemaphore()
	{
/*********** TO DO ************/
// Check it panics if the count <0

	test.Start(_L("Create"));
	RSemaphore semaphore;
	RThread thread1, thread2;

	semaphore.CreateLocal(0); 	// creates a DPlatSemaphore but casts it to a pointer to a DSemaphore
								// sets semaphore count to the value of the parameter, 
								// adds object to the K::Semaphores container, sets iHandle
								// Local sets DSemaphore.iName to NULL & iOwner to Kern::CurrentProcess()
								// Global sets iName to that passed and iOwner to NULL
								// Adds a record into CObjectIx containing a pointer to the semaphore object
/*	test.Next(_L("Find"));
	fullName=semaphore.FullName();	
	find.Find(fullName);	// sets iMatch to fullName	(misleadingly named method as it doesn't find anything)
	test(find.Next(fullName)== KErrNone);	
*/
	test.Next(_L("Producer/Consumer scenario"));
	// Test Rsemaphore with the producer/consumer scenario	RThread thread1, thread2;
	TRequestStatus stat1, stat2;
	test_KErrNone(mutex.CreateLocal());
	test_KErrNone(slotAvailable.CreateLocal(KMaxBufferSize));
	test_KErrNone(itemAvailable.CreateLocal(0));
	test_KErrNone(thread1.Create(_L("Thread1"),Producer,KDefaultStackSize,0x200,0x200,NULL));
	test_KErrNone(thread2.Create(_L("Thread2"),Consumer,KDefaultStackSize,0x200,0x200,NULL));
	thread1.Logon(stat1);
	thread2.Logon(stat2);
	test_Equal(KRequestPending, stat1.Int());
	test_Equal(KRequestPending, stat2.Int());
	thread1.Resume(); 
	thread2.Resume();
	User::WaitForRequest(stat1);
	User::WaitForRequest(stat2);
	test_KErrNone(stat1.Int());
	test_KErrNone(stat2.Int());
	for(TInt jj=0;jj<KNumProducerItems;jj++)
		test_Equal(jj, consumerArray[jj]);		
	
	test.Next(_L("Close"));
	mutex.Close();
	CLOSE_AND_WAIT(thread1);
	CLOSE_AND_WAIT(thread2);
	test.End();
	}

void TestMutex2()
	{
	RMutex m;
	test.Start(_L("Create"));
	test_KErrNone(m.CreateLocal());

	// Test RMutex::IsHeld()
	test.Next(_L("IsHeld ?"));
	test(!m.IsHeld());
	test.Next(_L("Wait"));
	m.Wait();
	test.Next(_L("IsHeld ?"));
	test(m.IsHeld());
	test.Next(_L("Signal"));
	m.Signal();
	test.Next(_L("IsHeld ?"));
	test(!m.IsHeld());

	test.End();
	}

void TestMutex()
	{
	test.Start(_L("Create"));
	test_KErrNone(mutex.CreateLocal());
	
	test.Next(_L("Threads writing to arrays test"));
//
// Create two threads which write to two arrays. The arrays and indexs
// are global and each thread writes an identifier to the arrays. For
// one array the writing and updating of the index is wrapped in a mutex
// pair. The other array is a control and is not wrapaped within mutex.
// Each thread records the number of instances it "thinks" it wrote to
// each array. For the mutex controlled array the actual instances
// written to the array should always be the same as the threads think.
//
	arrayIndex=0;
	RThread thread1,thread2;	
	test_KErrNone(thread1.Create(_L("Thread1"),MutexThreadEntryPoint1,KDefaultStackSize,0x2000,0x2000,NULL));
	test_KErrNone(thread2.Create(_L("Thread2"),MutexThreadEntryPoint2,KDefaultStackSize,0x2000,0x2000,NULL));			 
	TRequestStatus stat1,stat2;
	thread1.Logon(stat1);
	thread2.Logon(stat2);
	test_Equal(KRequestPending, stat1.Int());
	test_Equal(KRequestPending, stat2.Int());
	thread1.Resume(); 
	thread2.Resume();
	User::WaitForRequest(stat1);
	User::WaitForRequest(stat2);
	test_KErrNone(stat1.Int());
	test_KErrNone(stat2.Int()); 
	TInt thread1ActualCount=0; 
	TInt thread2ActualCount=0;
	TInt ii=0;
	while(ii<KMaxArraySize)
		{
		if (array[ii]==EThread1ID)
			thread1ActualCount++;
		if (array[ii]==EThread2ID)
			thread2ActualCount++;
		ii++;
		}
	test.Printf(_L("T1 %d T1ACT %d T2 %d T2ACT %d"),thread1Count,thread1ActualCount,thread2Count,thread2ActualCount);
	test_Equal(thread1Count, thread1ActualCount);
	test_Equal(thread2Count, thread2ActualCount);
	test_Equal(thread2Count, thread1Count);
	test_Equal((KMaxArraySize>>1), thread1Count);
	
	test.Next(_L("Close"));
	CLOSE_AND_WAIT(thread1);
	CLOSE_AND_WAIT(thread2);
	mutex.Close();
	test.End();
	}

void TestCriticalSection()
//
//As TestMutex, but for RCriticalSection
//
	{
	
	test.Start(_L("Create"));
	test_KErrNone(criticalSn.CreateLocal());

/***************** TO DO ***********************

	test.Next(_L("Find"));
//
// Test finding the RCriticalSection
//
	TFindCriticalSection find;
	TFullName fullName;
	fullName=criticalSn.FullName();
	find.Find(fullName);
	test(find.Next(fullName)==KErrNone);
	test(fullName==criticalSn.FullName());

************************************************/

	test.Next(_L("Threads writing to arrays test"));
//
// Create two threads which write to two arrays. The arrays and indexs
// are global and each thread writes an identifier to the arrays. For
// one array the writing and updating of the index is wrapped in a critical
// section pair. The other array is a control and is not wrapaped within
// a critical section. Each thread records the number of instances it
// "thinks" it wrote to each array. For the mutex controlled array the
// actual instances written to the array should always be the same as the
// threads think.
//
	arrayIndex=0;
	RThread thread1,thread2;	
	test_KErrNone(thread1.Create(_L("Thread1"),CriticalSnThreadEntryPoint1,KDefaultStackSize,0x2000,0x2000,NULL));
	test_KErrNone(thread2.Create(_L("Thread2"),CriticalSnThreadEntryPoint2,KDefaultStackSize,0x2000,0x2000,NULL));			 
	TRequestStatus stat1,stat2;
	thread1.Logon(stat1);
	thread2.Logon(stat2);
	test_Equal(KRequestPending, stat1.Int());
	test_Equal(KRequestPending, stat2.Int());
	thread1.Resume(); 
	thread2.Resume();
	User::WaitForRequest(stat1);
	User::WaitForRequest(stat2);
	test_KErrNone(stat1.Int());
	test_KErrNone(stat2.Int()); 
	TInt thread1ActualCount=0; 
	TInt thread2ActualCount=0;
	TInt ii=0;
	while(ii<KMaxArraySize)
		{
		if (array[ii]==EThread1ID)
			thread1ActualCount++;
		if (array[ii]==EThread2ID)
			thread2ActualCount++;
		ii++;
		}
	test_Equal(thread1Count, thread1ActualCount);
	test_Equal(thread2Count, thread2ActualCount);
	test_Equal(thread2Count, thread1Count);
	test_Equal((KMaxArraySize>>1), thread1Count);

	test.Next(_L("Close"));
	CLOSE_AND_WAIT(thread1);
	CLOSE_AND_WAIT(thread2);
	criticalSn.Close();
	test.End();
	}


GLDEF_C TInt E32Main()
	{	
	TInt cpus = UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0);
	if (cpus != 1)
		{
		test(cpus>1);
		// This test will require compatibility mode (and probably other changes)
		// to work on SMP - it depends on explicit scheduling order.
		test.Printf(_L("T_SEMUTX skipped, does not work on SMP\n"));
		return KErrNone;
		}	
	

	test.Title();
 	__UHEAP_MARK;
	test.Start(_L("Test RSemaphore"));
	TestSemaphore();
	TestSemaphore2();
	test.Next(_L("Test RMutex"));
	TestMutex();
	TestMutex2();
	test.Next(_L("Test RCriticalSection"));
	TestCriticalSection();
	test.End();
	__UHEAP_MARKEND;
	return(KErrNone);
	}


