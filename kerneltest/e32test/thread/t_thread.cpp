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
// e32test\thread\t_thread.cpp
// Overview:
// Tests the RThread class
// API Information:
// RThread, RUndertaker
// Details:
// - Create a thread, verify its priority, change the priority and verify results.
// - Verify naming and renaming threads works as expected.
// - Test logging on, resuming and closing a thread. Verify results are as expected.
// - Test creating threads with a variety of invalid parameters. Verify results.
// - Test the RUndertaker methods: create some threads, logon to the undertaker,
// verify results upon killing a thread and setting the thread handle.
// - Check kernel allocation when creating threads and undertakers. Verify the
// heap has not been corrupted.
// - Run a thread multiple times, panic within the thread, panic external to the
// thread and exit a thread in a variety of ways. Verify results are as expected.
// - Create a semaphore and some threads, verify the threads can wait on and signal
// the semaphore.
// - Test unclosed but completed threads.
// - Suspend and resume some threads in a variety of ways, verify results are as
// expected.
// - Test thread duplication.
// - Test opening a thread using an full name, perform various tests by finding, 
// killing, closing, recreating etc. Verify the results are as expected.
// - Test creating a thread using a duplicate name then reuse the thread with a
// valid name. Verify results are as expected.
// - Verify that a panicked thread releases an existing mutex.
// - Test thread ID: attempt to open a nonexistent thread by ID, verify different
// threads have different IDs, verify open by ID works as expected.
// - Test RThread::StackInfo(), print results and verify results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32panic.h>
#include <e32svr.h>
#include <u32hal.h>
#include <e32atomics.h>
#include <e32def.h>
#include <e32def_private.h>
#include "../misc/prbs.h"

const TInt KNumThreads=20;

const TInt KExitPanicNum=999;
const TInt KHeapSize=0x200;
const TInt KThreadReturnValue=9999;
const TInt KTerminationReason=1234;
const TInt KKillReason=4321;  
enum TInstruction {ENormal,EInstrPanic,EWait};

const TInt KWaitTime=800000;

class TReadInfo
	{
public:
	TDesC* tdesc;
	TPtrC* tptrc;
	TDes* tdes;
	TPtr* tptr;
	HBufC* hbufc;
	TBufC<0x20>* tbufc;
	TBuf<0x20>* tbuf;
	TPtr* tptrdes;
	TAny* anAddress;
	};

LOCAL_D RTest test(_L("T_THREAD"));
LOCAL_D RTest rtest(_L("Read thread tests"));
LOCAL_D	RTest wtest(_L("Write thread tests"));

#define rtest(x) rtest(x,__LINE__)
#define wtest(x) wtest(x,__LINE__)

LOCAL_C TInt LoopyThread(TAny*)
	{
	
	FOREVER
		User::AfterHighRes(1000);
	}

LOCAL_D void testUndertaker(TOwnerType anOwnerType)
//
// Test RThreadWatcher
//
	{

	RThread thread1;
	TInt r;
	test.Start(_L("Test the RUndertaker class"));
	test.Next(_L("Create a thread"));
//	if (anOwnerType==EOwnerThread)
//		User::SetDebugMask(0x8000867c);
	r=thread1.Create(_L("Loopy1"),LoopyThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)NULL,anOwnerType);
	test(r==KErrNone);
	thread1.Resume();

	TRequestStatus stat1;
	TInt threadHandle1;
	RThread w1;
	RUndertaker u1;
	test.Next(_L("Create an RUndertaker"));
	r=u1.Create();
	test(r==KErrNone);
	test.Next(_L("Logon to RUndertaker"));
	r=u1.Logon(stat1,threadHandle1);
	test(r==KErrNone);
	test.Next(_L("Logon again & check we're rejected"));
	r=u1.Logon(stat1,threadHandle1);
	test(r==KErrInUse);
	test.Next(_L("Cancel logon to RUndertaker"));
	r=u1.LogonCancel();
	test(r==KErrNone);
	test(stat1==KErrCancel);

	test.Next(_L("Logon to RUndertaker again"));
	u1.Logon(stat1,threadHandle1);
	
	test.Next(_L("Create another thread"));
	RThread thread2;
	r=thread2.Create(_L("Loopy2"),LoopyThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)NULL,anOwnerType);
	test(r==KErrNone);
	thread2.Resume();

	TRequestStatus stat2;
	TInt threadHandle2;
	RThread w2;
	RUndertaker u2;
	test.Next(_L("Create another RUndertaker"));
	r=u2.Create();
	test(r==KErrNone);
	test.Next(_L("Logon to RUndertaker"));
	r=u2.Logon(stat2,threadHandle2);
	test(r==KErrNone);

	test.Next(_L("Create yet another thread"));
	RThread thread3;
	r=thread3.Create(_L("Loopy3"),LoopyThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)NULL,anOwnerType);
	test(r==KErrNone);
	thread3.Resume();

	test.Next(_L("Kill the first thread & check the undertakers"));
	thread1.Kill(0x489);
	thread1.Close();

	User::WaitForRequest(stat1);
	User::WaitForRequest(stat2);
	test(stat1==KErrDied);
	test(stat2==KErrDied);

	RThread keep1;
	RThread keep2;
	test.Next(_L("Set the RThread handles"));
	keep1.SetHandle(threadHandle1);
	keep2.SetHandle(threadHandle2);

	test.Next(_L("Test the exit reasons"));
	test(keep1.ExitReason()==0x489);
	test(keep2.ExitReason()==0x489);
//	test.Printf(_L("Thread name %S\n"),&(w1.Name()));

	test.Next(_L("Logon again with both watchers"));
	r=u1.Logon(stat1,threadHandle1);
	test(r==KErrNone);
	r=u2.Logon(stat2,threadHandle2);
	test(r==KErrNone);

	test.Next(_L("Kill the 3rd thread & check the undertakers"));
	thread3.Kill(0x999);
	thread3.Close();

	User::WaitForRequest(stat1);
	User::WaitForRequest(stat2);
	test(stat1==KErrDied);
	test(stat2==KErrDied);

	test.Next(_L("Set the RThread handles"));
	w1.SetHandle(threadHandle1);
	w2.SetHandle(threadHandle2);

	test.Next(_L("Test the exit reasons"));
	test(w1.ExitReason()==0x999);
	test(w2.ExitReason()==0x999);
//	test.Printf(_L("Thread name %S\n"),&(w1.Name()));
	w1.Close();
	CLOSE_AND_WAIT(w2);

	test.Next(_L("Logon again with both undertakers"));
	r=u1.Logon(stat1,threadHandle1);
	test(r==KErrNone);
	r=u2.Logon(stat2,threadHandle2);
	test(r==KErrNone);

	test.Next(_L("Kill the 2nd thread & check the undertakers"));
	thread2.Kill(0x707);
	thread2.Close();

	User::WaitForRequest(stat1);
	User::WaitForRequest(stat2);
	test(stat1==KErrDied);
	test(stat2==KErrDied);

	test.Next(_L("Set the RThread handles"));
	w1.SetHandle(threadHandle1);
	w2.SetHandle(threadHandle2);

	test.Next(_L("Test the exit reasons"));
	test(w1.ExitReason()==0x707);
	test(w2.ExitReason()==0x707);
//	test.Printf(_L("Thread name %S\n"),&(w1.Name()));

	test.Next(_L("Check kernel allocation"));
	test.Next(_L("Please wait while I create & close masses of threads"));
	RThread t[KNumThreads];
	TInt j;
	for (j=0; j<KNumThreads; j++)
		{
		TBuf<0x10> name;
		name.Format(_L("LoopyThread%d"),j);
		test(t[j].Create(name, LoopyThread, KDefaultStackSize, KHeapSize, KHeapSize, (TAny*)NULL,anOwnerType)==KErrNone);
		}
	for (j=0; j<KNumThreads-1; j++)
		{
		t[j].Kill(666);
		CLOSE_AND_WAIT(t[j]);
		}

	test.Next(_L("Please wait while I close & create some undertakers"));
	u1.Close();
	u2.Close();
	r=u1.Create();
	test(r==KErrNone);
	r=u2.Create();
	test(r==KErrNone);

	test.Next(_L("Mark kernel heap"));
	__KHEAP_MARK;

	test.Next(_L("Create thread"));
	RThread threadx;
	r=threadx.Create(_L("Loopyx"),LoopyThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)NULL,anOwnerType);
	test(r==KErrNone);
	test.Next(_L("Resume thread"));
	threadx.Resume();

	test.Next(_L("Create undertaker"));
	TRequestStatus statx;
	TInt threadHandlex;
	RUndertaker ux;
	r=ux.Create();
	test(r==KErrNone);
	test.Next(_L("Logon to undertaker"));
	r=ux.Logon(statx,threadHandlex);
	test(r==KErrNone);
	test.Next(_L("Kill thread"));
	threadx.Kill(0x666);
	threadx.Close();
	User::WaitForRequest(statx);
	test(statx==KErrDied);
	test.Next(_L("Close thread"));
	RThread wx;
	wx.SetHandle(threadHandlex);
	CLOSE_AND_WAIT(wx);

	test.Next(_L("Close undertaker"));
	ux.Close();

	test.Next(_L("Check kernel heap"));
	__KHEAP_MARKEND;
	w1.Close();
	CLOSE_AND_WAIT(w2);
	keep1.Close();
	CLOSE_AND_WAIT(keep2);
	t[KNumThreads-1].Kill(666);
	CLOSE_AND_WAIT(t[KNumThreads-1]);

	test.Next(_L("Close RUndertakers"));
	u1.Close();
	u2.Close();
	test.End();
	}

volatile TInt IFLAG;

TInt InstructionThread(TAny* anInstruction)
	{
	__e32_atomic_store_ord32(&IFLAG, 1);
	RThread::Rendezvous(KErrNone);
	TInstruction what=(TInstruction)(TInt)anInstruction;
	if (what==EInstrPanic)
		User::Panic(_L("Hello"), KExitPanicNum);
	if (what==ENormal)
		return(KThreadReturnValue);
	User::After(500000);
	return(KErrNone);
	}

TInt StartInstructionThread(RThread& aT, const TDesC& aName, TInt aInstruction, RAllocator* aAllocator, TOwnerType aOwnerType, TRequestStatus* aL, TRequestStatus* aR)
	{
	TInt r;
	
	if (aAllocator == NULL)
		{
		r = aT.Create(aName, &InstructionThread, KDefaultStackSize, KHeapSize, KHeapSize, (TAny*)aInstruction, aOwnerType);
		}
	else
		{
		r = aT.Create(aName, &InstructionThread, KDefaultStackSize, aAllocator, (TAny*)aInstruction, aOwnerType);
		}

	if (r!=KErrNone)
		return r;

	if (aL)
		{
		aT.Logon(*aL);
		TInt s = aL->Int();
		test_Equal(s, KRequestPending);
		}

	if (aR)
		{
		aT.Rendezvous(*aR);
		TInt s = aR->Int();
		test_Equal(s, KRequestPending);
		aT.Resume();
		User::WaitForRequest(*aR);
		s = aR->Int();
		test_KErrNone(s);
		}

	return r;
	}


LOCAL_D TInt test4Thread(TAny *aSem)
//
// Wait to be released on the semaphore.
// Then release the semaphore.
//
	{

	RSemaphore& sem=(*(RSemaphore*)aSem);
	sem.Wait();
	sem.Signal();
	return(KErrNone);
	}

TInt BadPriority(TAny* aThread)
	{
	((RThread*)aThread)->SetPriority(EPriorityNull);
	return KErrNone;
	}

_LIT(KLitRomString,"RomStringRomStringRomStringRomStringRomStringRomStringRomString");

LOCAL_C TInt BadFullNameThread(TAny* aPar)
	{
	RThread thread;
	
	switch ((TInt)aPar)
		{
		case 0:
			{
			HBufC* hBuf = HBufC::New(5);//Size 5 is not sufficient. thread.FullName should panic.
			test(NULL != hBuf);
			RBuf rBuf(hBuf);
			thread.FullName(rBuf);
			rBuf.Close();
			}
			return(KErrNone);

		case 1:
			{
			TPtr ptr((TText*)(KLitRomString.iBuf), KLitRomString.iTypeLength);
			// Passing descriptor whose data is in ROM. This may behave in different ways
			// on differrent platforms. Here, we just check that Kernel is safe.
			thread.FullName(ptr);
			}
			return(KErrNone);
		}
	return(KErrArgument);
	}


LOCAL_D void test1()
//
// Test 1
//
	{
	
	__UHEAP_MARK;
	RThread thread;
	TRequestStatus stat;
	TInt r;

	test.Start(_L("Close without create"));
	thread.Close();
	
	test.Next(_L("Create ENormal"));
	r = StartInstructionThread(thread, _L("Thread"), ENormal, NULL, EOwnerProcess, 0, 0);
	test_KErrNone(r);

	test.Next(_L("Test priorities"));
	test(thread.Priority()==EPriorityNormal);
	thread.SetPriority(EPriorityRealTime);	// WINS will commute this to EPriorityMuchMore
#if defined(__EPOC32__)
	test(thread.Priority()==EPriorityRealTime);
#endif
	thread.SetPriority(EPriorityMuchMore);
	test(thread.Priority()==EPriorityMuchMore);
//	thread.SetPriority(EPriorityNull);
	RThread badThread;
	r = badThread.Create(_L("Bad Priority"),BadPriority,KDefaultStackSize,KHeapSize,KHeapSize,&thread);
	test(r==KErrNone);
	badThread.Logon(stat);
	test(stat==KRequestPending);
	badThread.Resume();
	User::WaitForRequest(stat);
	test(stat==EBadPriority);
	test(badThread.ExitCategory()==_L("KERN-EXEC"));
	test(badThread.ExitReason()==EBadPriority);
	test(badThread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(badThread);
	test(thread.Priority()==EPriorityMuchMore);

#if defined(__EPOC32__)
	test.Next(_L("Test setting process priority from thread"));
	test(thread.ProcessPriority()==EPriorityForeground);
	thread.SetProcessPriority(EPriorityHigh);
	test(thread.ProcessPriority()==EPriorityHigh);
	test(RProcess().Priority()==EPriorityHigh);
	thread.SetProcessPriority(EPriorityForeground);
	test(thread.ProcessPriority()==EPriorityForeground);
	test(RProcess().Priority()==EPriorityForeground);
#endif

	TBuf<0x100> name;
	test.Next(_L("Test thread name"));
	test(thread.Name()==_L("Thread"));
	test.Next(_L("Get owning process name"));
	RProcess p;
	test(thread.Process(p)==KErrNone);
	name=p.Name();
	name.Append(_L("::"));
	name.Append(thread.Name());
	test.Next(_L("Test fullname - via TFullName RHandleBase::FullName"));
	test(thread.FullName().CompareF(name)==0);

	test.Next(_L("Test fullname - via void RHandleBase::FullName(TDes& aName)"));
	HBufC* hBuf = HBufC::New(100);
	test(NULL != hBuf);
	TPtr ptr = hBuf->Des();
	thread.FullName(ptr);
	test(ptr.CompareF(name)==0);
	RBuf rBuf(hBuf);
	thread.FullName(rBuf);
	test(rBuf.CompareF(name)==0);
	rBuf.Close();

	test.Next(_L("Test void RHandleBase::FullName(TDes& aName) when aName is too short"));
	TInt aaa=badThread.Create(_L("BadFullNameThread1"),BadFullNameThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)0);
	test(aaa==KErrNone);
	badThread.Logon(stat);
	test(badThread.ExitType()==EExitPending);
	badThread.Resume();
	User::WaitForRequest(stat);
	test(badThread.ExitCategory()==_L("KERN-EXEC"));
	test(badThread.ExitReason()==EKUDesSetLengthOverflow);
	test(badThread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(badThread);

	test.Next(_L("Test void RHandleBase::FullName(TDes& aName) where aName has data in ROM "));
	aaa=badThread.Create(_L("BadFullNameThread2"),BadFullNameThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)1);
	test(aaa==KErrNone);
	badThread.Logon(stat);
	test(badThread.ExitType()==EExitPending);
	badThread.Resume();
	User::WaitForRequest(stat);
	test.Printf(_L("BadFullNameThread2 exited with ExitReason=%d and ExitType=%d\n"),badThread.ExitReason(),badThread.ExitType());
	CLOSE_AND_WAIT(badThread);

	test.Next(_L("Rename current thread"));
	test(User::RenameThread(_L("renamed"))==KErrNone);
	name=p.Name();
	name.Append(_L("::"));
	RThread me;
	name.Append(me.Name());
	test(me.Name()==_L("renamed"));
	test(me.FullName().CompareF(name)==0);

	test.Next(_L("Test running exit types"));
	test(thread.ExitType()==EExitPending);
	test(thread.ExitReason()==0);
	// no getters for iUserHeap and iFrame
	test(thread.ExitCategory()==KNullDesC);

	test.Next(_L("Test logging on"));
	thread.Logon(stat);
	RThread t;
	test(t.RequestCount()==0);
	test(stat==KRequestPending);
	r=thread.LogonCancel(stat); // this generates a signal 
	test(r==KErrNone);
	test(stat==KErrNone);
	test(t.RequestCount()==1); // the request count is 1 due to the signal generated by LogonCancel
	test(thread.RequestCount()==0);

	test.Next(_L("Resuming thread"));
	thread.Resume();   
	test.Next(_L("Absorb cancel"));
	User::WaitForRequest(stat);	
	test.Next(_L("Test LogonCancel on dead thread is ok"));
	r=thread.LogonCancel(stat);
	test(r==KErrGeneral);
	test.Next(_L("Close thread"));
	CLOSE_AND_WAIT(thread);
	test.Next(_L("Close again"));
	thread.Close();
	thread.Close();
	thread.Close();
	thread.Close();
	__UHEAP_MARKEND;
	test.End();
	}


LOCAL_D void test2(TOwnerType anOwnerType)
//
// Test 2
//
	{                                  

	__UHEAP_MARK;
	RThread thread;
	TRequestStatus stat;
	TRequestStatus rstat;
	TInt r;

	test.Start(_L("Run thread 10 times"));
	for (TInt xx=0;xx<10;xx++)
		{
		test.Printf(_L("\r%02d"),xx);
		r = StartInstructionThread(thread, _L("Thread1"), ENormal, NULL, anOwnerType, &stat, 0);
		test_KErrNone(r);
		thread.Resume();
		User::WaitForRequest(stat);
		CLOSE_AND_WAIT(thread);
		}
	test.Printf(_L("\n"));

	test.Next(_L("Panic within thread"));
	r = StartInstructionThread(thread, _L("Thread2"), EInstrPanic, NULL, anOwnerType, &stat, 0);
	test_KErrNone(r);
	test(thread.ExitType()==EExitPending);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitCategory()==_L("Hello"));
	test(thread.ExitReason()==KExitPanicNum);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Panic external to thread"));
	TInt ijk;
	TUint seed[2] = { 0xadf85458, 0 };
	TUint maxcount = 0;
	RHeap* temporaryHeap = User::ChunkHeap(NULL, KHeapSize*8192, KHeapSize*8192);
	test(temporaryHeap != NULL);
	for (ijk=0; ijk<8192; ++ijk)
		{
		if (!(ijk&255))
			test.Printf(_L("%d\n"), ijk);

		//
		// For this test we need to use a temporary heap created in advance as we
		// will be panicking the thread at any point during its creation and since
		// the heap would have been allocated by the user side thread, it is
		// possible that if we let it allocate its own heap, the kernel may not
		// be able to close it in the temporary states of creation or when
		// TLocalThreadData::DllSetTls() grabs a temporary handle on the heap.
		// In those cases RTest::CloseHandleAndWaitForDestruction() would timeout
		// and the test would fail.
		//
		// In addition, if we shared the creating thread's heap allocations may
		// be left behind and cause the heap mark test to fail on this thread.
		//
		r = StartInstructionThread(thread, _L("Thread3"), EWait, temporaryHeap, anOwnerType, &stat, 0);
		test_KErrNone(r);
		__e32_atomic_store_ord32(&IFLAG, 0);
		thread.Resume();
		thread.SetPriority(EPriorityMore);
		if (maxcount==0)
			{
			while (__e32_atomic_load_acq32(&IFLAG)==0 && --maxcount!=0)
				{
				}
			maxcount = 0u - maxcount;
			test.Printf(_L("maxcount=%u\n"), maxcount);
			}
		else
			{
			TUint random = Random(seed);
			random %= maxcount;
			++random;
			while (__e32_atomic_load_acq32(&IFLAG)==0 && --random!=0)
				{
				}
			}
		thread.Panic(_L("panic"), 123);
		User::WaitForRequest(stat);
		test(thread.ExitCategory()==_L("panic"));
		test(thread.ExitReason()==123);
		test(thread.ExitType()==EExitPanic);
		r = RTest::CloseHandleAndWaitForDestruction(thread);
		test_KErrNone(r);
		}
	temporaryHeap->Close();
	
	test.Next(_L("Internal exit"));
	r = StartInstructionThread(thread, _L("Thread4"), ENormal, NULL, anOwnerType, &stat, 0);
	test_KErrNone(r);
	test(thread.ExitType()==EExitPending);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitCategory()==_L("Kill"));
	test(thread.ExitReason()==KThreadReturnValue);
	test(thread.ExitType()==EExitKill);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("External terminate"));
	r = StartInstructionThread(thread, _L("Thread5"), EWait, NULL, anOwnerType, &stat, &rstat);
	test_KErrNone(r);
	test.Next(_L("Terminate"));
	thread.Terminate(KTerminationReason);
	test.Next(_L("Wait"));
	User::WaitForRequest(stat);
	test(thread.ExitCategory()==_L("Terminate"));
	test(thread.ExitReason()==KTerminationReason);
	test(thread.ExitType()==EExitTerminate);
	test.Next(_L("Close"));
	CLOSE_AND_WAIT(thread);
  
	test.Next(_L("External kill"));
	r = StartInstructionThread(thread, _L("Thread6"), EWait, NULL, anOwnerType, &stat, &rstat);
	test_KErrNone(r);
	thread.Suspend();
	thread.Resume();
	thread.Kill(KKillReason);
	User::WaitForRequest(stat);
	test(thread.ExitCategory()==_L("Kill"));
	test(thread.ExitReason()==KKillReason);
	test(thread.ExitType()==EExitKill);
	test.Next(_L("Kill again"));
	thread.Kill(KErrNone);
	thread.Kill(KErrNone);
	thread.Kill(KErrNone);
	CLOSE_AND_WAIT(thread);
	test.End();
  	__UHEAP_MARKEND;
	}

LOCAL_D void test3()
//
// Test 3.
//
	{

	test.Start(_L("Read across thread"));
	TReadInfo info;
	TPtrC des1=_L("tdesc");
	info.tdesc=(&des1);
	TPtrC ptr1=_L("tptrc");
	info.tptrc=&ptr1;
	TBuf<0x20> tdes(_L("tdes"));
	info.tdes=&tdes;
	TBuf<0x20> tptrbuf(_L("tptr"));
	TPtr tptr((TText*)tptrbuf.Ptr(),tptrbuf.Length(),tptrbuf.MaxLength());
	info.tptr=&tptr;
	TBuf<0x20> hbufc(_L("hbufc"));
	HBufC *pH=hbufc.Alloc();
	test(pH!=NULL);
	info.hbufc=pH;
	TBufC<0x20> tbufc(_L("tbufc"));
	info.tbufc=&tbufc;
	TBuf<0x20> tbuf(_L("tbuf"));
	info.tbuf=&tbuf;
	TBufC<0x20> tptrdes(_L("tptrdes"));
	TPtr des=tptrdes.Des();
	info.tptrdes=&des;
	TBuf<0x10> b(_L("Hello"));
	info.anAddress=(&b);
	info.anAddress= info.anAddress; //prevents warning (var set but never used)
	delete pH;
	test.End();
	}

LOCAL_D void test4()
//
// Test 4.
//
	{

	test.Start(_L("Create sempahore"));
	RSemaphore sem;
	TInt r=sem.CreateLocal(0);
	test(r==KErrNone);
//
	test.Next(_L("Create thread 1"));
	RThread t;
	r=t.Create(_L("Thread1"),test4Thread,KDefaultStackSize,KHeapSize,KHeapSize,&sem);
	test(r==KErrNone);
	t.Resume();
	t.Close();
//
	test.Next(_L("Create thread 2"));
	r=t.Create(_L("Thread2"),test4Thread,KDefaultStackSize,KHeapSize,KHeapSize,&sem);
	test(r==KErrNone);
	t.Resume();
	t.Close();
//
	test.Next(_L("Create thread 3"));
	r=t.Create(_L("Thread3"),test4Thread,KDefaultStackSize,KHeapSize,KHeapSize,&sem);
	test(r==KErrNone);
	t.Resume();
	t.Close();
//
	test.Next(_L("Release threads"));
	sem.Signal(3);
//
	test.Next(_L("Wait 1"));
	sem.Wait();
//
	test.Next(_L("Wait 2"));
	sem.Wait();
//
	test.Next(_L("Wait 2"));
	sem.Wait();
	sem.Close();
//
	test.End();
	}

TInt MinimalThread(TAny*)
//
// Minimal thread, used in test 5
//
	{
	return(KErrNone);
	}

LOCAL_D void test5()
//
// Test 5 - tests unclosed but completed threads
//
	{

	test.Start(_L("Start thread"));
	RThread thread1;
	test(thread1.Create(_L("Test Thread1"),MinimalThread,KDefaultStackSize,KHeapSize,KHeapSize,NULL)==KErrNone);
	TRequestStatus stat1;
	thread1.Logon(stat1);
	thread1.Resume();
	User::WaitForRequest(stat1);
	test(thread1.ExitType()==EExitKill);
	// 'missing'  thread1.Close();

	test.Next(_L("Start another thread"));
	RThread thread2;
	test(thread2.Create(_L("Test Thread2"),MinimalThread,KDefaultStackSize,KHeapSize,KHeapSize,NULL)==KErrNone);
	TRequestStatus stat2;
	thread2.Logon(stat2);
	thread2.Resume();
	User::WaitForRequest(stat2);  //Goes wrong here in build 48
	test(thread2.ExitType()==EExitKill);
	
	test.Next(_L("Close both threads"));
	CLOSE_AND_WAIT(thread2);
	CLOSE_AND_WAIT(thread1);

	test.End();
	}

LOCAL_D TInt test6Thread(TAny *anArg)
//
//
//
	{
	((RSemaphore*)anArg)->Wait();
	RThread t;
	RThread dup;
	dup.Duplicate(t);
	dup.Panic(_L("Test"),0);

	return KErrNone;
	}

void test6()
//
// Test thread duplication
//
	{

	test.Start(_L("Create thread"));
	RSemaphore sem;
	TInt r=sem.CreateLocal(0);
	test(r==KErrNone);

	RThread t;
	t.Create(_L("test6thread"),test6Thread,KDefaultStackSize,KHeapSize,KHeapSize,&sem);
	test.Next(_L("Resume thread"));
	TRequestStatus stat;
	t.Logon(stat);
	t.Resume();
	sem.Signal();
	User::WaitForRequest(stat);
	test.Next(_L("Close thread"));
	t.Close();
	sem.Close();
	test.End();
	}

RSemaphore gsem;
enum TThreadProgress { EBeforeStart, EStarted, EWaiting, EDoneWaiting, EFinished };
TThreadProgress progress=EBeforeStart;
LOCAL_D TInt test7thread(TAny * /*anArg*/)
//
//
//
	{

	progress=EStarted;
	progress=EWaiting;
	gsem.Wait();
	progress=EDoneWaiting;
	gsem.Wait();
	progress=EFinished;
	return KErrNone;
	}

void test7()
//
//	Suspend/ Resume tests
//
	{

	TInt r=gsem.CreateLocal(0);
	test(r==KErrNone);
	test.Start(_L("Create thread"));
	RThread t;
	r=t.Create(_L("test7thread"), test7thread, KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	TRequestStatus stat;
	t.Logon(stat);
	test.Next(_L("Resume thread"));
	t.Resume();
	User::After(KWaitTime); // wait a bit;
	test.Next(_L("Make thread wait on a semaphore"));
	test(progress==EWaiting);
	test.Next(_L("Suspend waiting thread"));
	t.Suspend();
	test.Next(_L("Signal the semaphore"));
	gsem.Signal();
	User::After(KWaitTime);
	test.Next(_L("Test thread still suspended"));
	test(progress==EWaiting);
	test.Next(_L("resume thread"));
	t.Resume();
	test.Next(_L("Test the thread no longer waiting on the semaphore"));
	User::After(KWaitTime);
	test(progress==EDoneWaiting);
	test.Next(_L("Wait for thread to finish"));
	gsem.Signal();
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	test(progress==EFinished);
	CLOSE_AND_WAIT(t);

	RThread tt;
	progress=EBeforeStart;
	test.Next(_L("Create Thread"));
	r=tt.Create(_L("test7thread"), test7thread, KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	tt.Logon(stat);
	test.Next(_L("Suspend thread without starting it"));
	tt.Suspend();
	tt.Suspend();
	test.Next(_L("Resume and test suspend/resume balance"));
	tt.Resume();
	tt.Resume();
	User::After(KWaitTime);
	test(progress==EBeforeStart);
	tt.Resume();
	test.Next(_L("test thread is suspended on semaphore"));
	User::After(KWaitTime);
	test(progress==EWaiting);
	test.Next(_L("suspend thread"));
	tt.Suspend();
	tt.Suspend();
	test.Next(_L("resume thread"));
	tt.Resume();
	tt.Resume();
	test.Next(_L("test thread still suspended on semaphore"));
	User::After(KWaitTime);
	test(progress==EWaiting);
	test.Next(_L("Suspend, Suspend, Signal semaphore, Suspend"));
	tt.Suspend();
	tt.Suspend();
	gsem.Signal();
	tt.Suspend();
	test.Next(_L("test thread still suspended on semaphore"));
	User::After(KWaitTime);
	test(progress==EWaiting);
	test.Next(_L("Resume thread, checking suspend/resume balance"));
	tt.Resume();
	User::After(KWaitTime);
	test(progress==EWaiting);
	tt.Resume();
	User::After(KWaitTime);
	test(progress==EWaiting);
	tt.Resume();
	User::After(KWaitTime);
	test(progress==EDoneWaiting);
	test.Next(_L("Resume an executing thread"));
	tt.Resume();
	tt.Resume();
//	test.Next(_L("Suspend and check balance"));
//	tt.Suspend();
//	tt.Suspend();
	test.Next(_L("Wait for thread to finish"));
	gsem.Signal();
	User::After(KWaitTime);
	test(progress==EFinished);
	User::WaitForRequest(stat);
	CLOSE_AND_WAIT(tt);

//
	test.Next(_L("Create Thread"));
	r=tt.Create(_L("test7thread"), test7thread, KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	tt.Logon(stat);
	test.Next(_L("Resume"));
	tt.Resume();
	test.Next(_L("Hang thread on semaphore"));
	User::After(KWaitTime);
	test(progress==EWaiting);
	test.Next(_L("Suspend then Resume thread"));
	tt.Suspend();
	tt.Suspend();
	tt.Resume();
	User::After(KWaitTime);
	test(progress==EWaiting);
	tt.Resume();
	test.Next(_L("Check still hanging on semaphore"));
	User::After(KWaitTime);
	test(progress==EWaiting);
	test.Next(_L("Signal Semaphore"));
	gsem.Signal();
	test.Next(_L("Test thread executing again"));
	User::After(KWaitTime);
	test(progress==EDoneWaiting);
	test.Next(_L("Hang thread on another semaphore, and suspend"));
	tt.Suspend();
	test.Next(_L("Signal semaphore, and suspend again"));
	gsem.Signal();
	User::After(KWaitTime);
	test(progress==EDoneWaiting);
	tt.Suspend();
	test.Next(_L("Resume the thread"));
	tt.Resume();
	User::After(KWaitTime);
	test(progress==EDoneWaiting);
	tt.Resume();
	test.Next(_L("Wait for thread to finish"));
	User::After(KWaitTime);
	test(progress==EFinished);
	User::WaitForRequest(stat);
	CLOSE_AND_WAIT(tt);
	test.End();
	}

#if 0
RSemaphore Sem;
LOCAL_D TInt test8thread(TAny* aPtr)
//
//
//
	{

	typedef TBuf<0x20> TTestBuf;
	typedef volatile TTestBuf* TTestBufPtr;
	volatile TTestBufPtr& pB=*(volatile TTestBufPtr*)aPtr;
	if ((TUint)pB != 0xc90fdaa2)
		return KErrGeneral;
	Sem.Wait();
	TDesC* pD=(TDesC*)pB;
	test(*pD==_L("Everything's just hunky-dory"));
	delete (TTestBufPtr*)pB;
	__UHEAP_MARKEND;
	return KErrNone;
	}
#endif

void test8()
//
// Get Heap
//
	{
	// !!! RThread::SetInitialParameter no longer exists

 	/*
	typedef TBuf<0x20> TTestBuf;
	TTestBuf* buf=(TTestBuf*)0xc90fdaa2;

	test.Start(_L("Create thread"));
	RThread thread;
	TInt r=thread.Create(_L("test8thread"),test8thread,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	r=Sem.CreateLocal(0);
	test(r==KErrNone);
	test.Next(_L("Set parameter"));
	r=thread.SetInitialParameter(&buf);
	test(r==KErrNone);
	TRequestStatus stat;
	thread.Logon(stat);
	thread.SetPriority(EPriorityMore);
	test.Next(_L("Resume thread"));
	thread.Resume();
	test.Next(_L("Set initial parameter NULL"));
	r=thread.SetInitialParameter(NULL);
	test(thread.ExitType()==EExitPending);

	test.Next(_L("Get heap"));
	RHeap* heap;
	heap=thread.Heap();
	test.Next(_L("Alloc inside heap"));
	__RHEAP_MARK(heap);
	buf=(TTestBuf*)heap->Alloc(sizeof(TTestBuf));
	test(buf!=NULL);
	new(buf) TTestBuf;
	*buf=(_L("Everything's just hunky-dory"));

	Sem.Signal();
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	test(thread.ExitType()==EExitKill);
	test(thread.ExitReason()==KErrNone);

	test.Next(_L("Close"));
	thread.Close();
	Sem.Close();
	test.End();
	*/
	}

TInt Thread(TAny* /*aAny*/)
	{

	RTest test(_L("Any old thread"));
	test.Next(_L("Find remote thread"));
	// find the main thread
	TFullName name;
	name=RProcess().Name();
	name.Append(_L("::*"));
	TFindThread ft;
	ft.Find(name);
	TInt r=ft.Next(name);
	test(r==KErrNone);
	RThread t;
	t.Open(ft);

	t.Close();
	return KErrNone;
	}

void test9()
	{

	test.Start(_L("Test create a NULL TPtr"));
	TPtr p(NULL, 10, 10);
	test.Next(_L("Create and run remote thread"));
	RThread t;
	TInt r;
	r=t.Create(_L("Any Old Thread"), Thread, 0x2000, 0x2000, 0x2000, (TAny *)&p);
	test(KErrNone==r);
	TRequestStatus stat;
	t.Logon(stat);
	t.Resume();
	test.Next(_L("Wait for thread to complete"));
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	test(t.ExitCategory()==_L("Kill"));
	test(t.ExitReason()==KErrNone);
	test(t.ExitType()==EExitKill);
	CLOSE_AND_WAIT(t);
	test.End();
    }



TInt FoghornLeghorn(TAny* aMutex)
//
// Thread function
//
	{

	((RSemaphore*)aMutex)->Wait();
	RThread thread;
	TInt r=thread.Create(_L("I say * boy"),FoghornLeghorn,KDefaultStackSize,NULL,aMutex);
	test(r==KErrBadName);
	return KErrNone;
	}

void testOpen()
	{
	
	test.Start(_L("Create Foghorn Leghorn"));
	RSemaphore fogMut;
	fogMut.CreateLocal(0);
	RThread foghorn;
	TInt r=foghorn.Create(_L("Foghorn Leghorn"),FoghornLeghorn,KDefaultStackSize,KHeapSize,KHeapSize,&fogMut);
	test(r==KErrNone);
	test.Next(_L("Logon"));
	TRequestStatus stat;
	foghorn.Logon(stat);
	test(stat==KRequestPending);
	test.Next(_L("Resume Foghorn Leghorn"));
	foghorn.Resume();
	test.Next(_L("Get full name"));
	TFindThread find(_L("*Foghorn Leghorn"));
	TFullName name;
	r=find.Next(name);
	test(r==KErrNone);
	test.Next(_L("Open another handle using full name"));
	RThread leghorn;
	r=leghorn.Open(name);
	test(r==KErrNone);
	test.Next(_L("Kill using second handle"));
	leghorn.Kill(34523);
	User::WaitForRequest(stat);
	test(stat==34523);
	test.Next(_L("Close handles"));
	foghorn.Close();
	CLOSE_AND_WAIT(leghorn);

	test.Next(_L("Again! - Create Foghorn Leghorn"));
	r=foghorn.Create(_L("Foghorn Leghorn"),FoghornLeghorn,KDefaultStackSize,KHeapSize,KHeapSize,&fogMut);
	test(r==KErrNone);
	test.Next(_L("Logon"));
	foghorn.Logon(stat);
	test(stat==KRequestPending);
	test.Next(_L("Resume Foghorn Leghorn"));
	foghorn.Resume();
	test.Next(_L("Get full name"));
	find.Find(_L("*Foghorn Leghorn"));
	r=find.Next(name);
	test(r==KErrNone);
	test.Next(_L("Open another handle using full name"));
	r=leghorn.Open(name);
	test(r==KErrNone);
	test.Next(_L("Kill using second handle"));
	leghorn.Kill(67857);
	User::WaitForRequest(stat);
	test(stat==67857);
	test.Next(_L("Close handles"));
	foghorn.Close();
	CLOSE_AND_WAIT(leghorn);

	test.Next(_L("Create Foghorn Leghorn"));
	r=foghorn.Create(_L("Foghorn Leghorn"),FoghornLeghorn,KDefaultStackSize,KHeapSize,KHeapSize,&fogMut);
	test(r==KErrNone);
	test.Next(_L("Logon"));
	foghorn.Logon(stat);
	test(stat==KRequestPending);
	test.Next(_L("Resume Foghorn Leghorn"));
	foghorn.Resume();
	test.Next(_L("Now close it"));
	foghorn.Close();

	test.Next(_L("Get full name"));
	find.Find(_L("*Foghorn Leghorn"));
	r=find.Next(name);
	test(r==KErrNone);
	test.Next(_L("Open using full name"));
	r=leghorn.Open(name);
	test(r==KErrNone);
	test.Next(_L("Kill"));
	leghorn.Kill(67857);
	User::WaitForRequest(stat);
	test(stat==67857);
	test.Next(_L("Close"));
	CLOSE_AND_WAIT(leghorn);

	test.Next(_L("Start and get it to try to start a new thread"));
	r=foghorn.Create(_L("Foghorn Leghorn"),FoghornLeghorn,KDefaultStackSize,KHeapSize,KHeapSize,&fogMut);
	test(r==KErrNone);
	foghorn.Logon(stat);
	test(stat==KRequestPending);
	foghorn.Resume();
	fogMut.Signal();
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	test(foghorn.ExitCategory()==_L("Kill"));
	test(foghorn.ExitReason()==KErrNone);
	test(foghorn.ExitType()==EExitKill);
	test.Next(_L("Close"));
	CLOSE_AND_WAIT(foghorn);
	fogMut.Close();

	test.End();
	}

TInt Bunny(TAny*)
//
// Thread function
//
	{

	FOREVER
		;
	}

void testReuse()
	{
	
	test.Start(_L("Create thread with duplicate name"));
	RThread thread;
	TFullName name=thread.Name();
	TInt r=thread.Create(name,Bunny,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrAlreadyExists);
//	thread.Resume(); handle will be invalid since create failed
	test.Next(_L("Create with a good name"));
	r=thread.Create(_L("Bugs Bunny"),Bunny,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	TRequestStatus stat;
	thread.Logon(stat);
	test.Next(_L("Resume"));
	thread.Resume();
	test.Next(_L("Kill"));
	thread.Kill(15);
	User::WaitForRequest(stat);
	test(stat==15);
	CLOSE_AND_WAIT(thread);

	test.End();
	}
	

TInt HongKongPhooey(TAny * /*aAny*/)
	{

	RMutex m;
	m.OpenGlobal(_L("Test Mutex"));
	m.Wait();
	User::Panic(_L("Hello"),900);
	return KErrNone;
	}

void testReleaseMutex()
//
// Bug HA-187
//
	{
	TInt r;
	test.Start(_L("Create a global Mutex"));
	RMutex m;
	r=m.CreateGlobal(_L("Test Mutex"));
	test.Next(_L("Create a thread"));
	RThread number1SuperGuy;
	r=number1SuperGuy.Create(_L("Hong Kong Phooey"), HongKongPhooey, KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	TRequestStatus s;
	number1SuperGuy.Logon(s);
	test.Next(_L("Resume Thread"));
	number1SuperGuy.Resume();
	test.Next(_L("Wait on Mutex and Panic"));
	User::WaitForRequest(s);
	test(number1SuperGuy.ExitType()==EExitPanic);
	test(number1SuperGuy.ExitCategory()==_L("Hello"));
	test(number1SuperGuy.ExitReason()==900);
	User::After(100000);	// wait a bit for everything to be cleaned up
	m.Wait();
	test.Next(_L("Close everything"));
	m.Close();
	CLOSE_AND_WAIT(number1SuperGuy);
	test.End();
	}

void testId()
	{

	test.Start(_L("Try to open nonexistant thread by ID"));
	RThread thread;
	TInt r=thread.Open(*(TThreadId*)&KMaxTUint);
	test(r==KErrNotFound);
	test.Next(_L("Get thread ID"));
	r=thread.Create(_L("Buster Bunny"),Bunny,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	TThreadId id=thread.Id();
	TThreadId id2=thread.Id();
	test(id==id2);
	RThread thread2;
	r=thread2.Create(_L("Babs Bunny"),Bunny,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	id2=thread2.Id();
	test(id!=id2);
	test(*(TUint*)&id+1==*(TUint*)&id2);
	test.Next(_L("Open by ID"));
	TRequestStatus stat;
	thread.Logon(stat);
	thread.Kill(54624);
	User::WaitForRequest(stat);
	test(stat==54624);
	thread.Close();
	r=thread.Open(id2);
	test(r==KErrNone);
	test(thread.Name()==_L("Babs Bunny"));
	test(thread.FullName()==thread2.FullName());
	thread2.Close();
	id=thread.Id();
	test(id==id2);
	thread.Logon(stat);
	thread.Kill(88863);
	User::WaitForRequest(stat);
	test(stat==88863);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Test getting own thread ID"));
	id=RThread().Id();
	id2=RThread().Id();
	test(id==id2);
	r=thread.Open(id);
	test(r==KErrNone);
	id2=thread.Id();
	test(id==id2);
	thread.Close();

	test.End();
	}

struct SCreateInfo
	{
	TInt iStackSize;
	TInt iMinHeapSize;
	TInt iMaxHeapSize;
	};

TInt BadCreation(TAny* aCreateInfo)
	{
	SCreateInfo& info=*((SCreateInfo*)aCreateInfo);
	RThread thread;
	thread.Create(_L("Won't work"),Bunny,info.iStackSize,info.iMinHeapSize,info.iMaxHeapSize,NULL);
	return KErrNone;
	}

void testCreate()
	{
	test.Start(_L("Negative stack size"));
	RThread thread;
	TRequestStatus stat;
	TInt r;
	{
	SCreateInfo info={-1,0x1000,0x1000};
	r=thread.Create(_L("Test Create"),BadCreation,KDefaultStackSize,KHeapSize,KHeapSize,&info);
	test(KErrNone==r);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(stat==EThrdStackSizeNegative);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitReason()==EThrdStackSizeNegative);
	test(thread.ExitCategory()==_L("USER"));
	CLOSE_AND_WAIT(thread);
	}
//
	test.Next(_L("Negative heap min size"));
	{
	SCreateInfo info={0x1000,-1,0x1000};
	r=thread.Create(_L("Test Create"),BadCreation,KDefaultStackSize,KHeapSize,KHeapSize,&info);
	test(KErrNone==r);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(stat==EThrdHeapMinTooSmall);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitReason()==EThrdHeapMinTooSmall);
	test(thread.ExitCategory()==_L("USER"));
	CLOSE_AND_WAIT(thread);
	}
	test.Next(_L("Negative heap max size"));
	{
	SCreateInfo info={0x1000,0x1000,-1};
	r=thread.Create(_L("Test Create"),BadCreation,KDefaultStackSize,KHeapSize,KHeapSize,&info);
	test(KErrNone==r);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(stat==EThrdHeapMaxLessThanMin);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitReason()==EThrdHeapMaxLessThanMin);
	test(thread.ExitCategory()==_L("USER"));
	CLOSE_AND_WAIT(thread);
	}
	test.Next(_L("heap max size < heap min size"));
	{
	SCreateInfo info={0x1000,0x2001,0x1000};
	r=thread.Create(_L("Test Create"),BadCreation,KDefaultStackSize,KHeapSize,KHeapSize,&info);
	test(KErrNone==r);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(stat==EThrdHeapMaxLessThanMin);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitReason()==EThrdHeapMaxLessThanMin);
	test(thread.ExitCategory()==_L("USER"));
	CLOSE_AND_WAIT(thread);
	}
	test.Next(_L("Little min heap size"));
	{
	SCreateInfo info={0x1000,KMinHeapSize-1,0x1000};
	r=thread.Create(_L("Test Create"),BadCreation,KDefaultStackSize,KHeapSize,KHeapSize,&info);
	test(KErrNone==r);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(stat==EThrdHeapMinTooSmall);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitReason()==EThrdHeapMinTooSmall);
	test(thread.ExitCategory()==_L("USER"));
	CLOSE_AND_WAIT(thread);
	}
	test.End();
	}

TInt StackInfoThread(TAny*)
	{
	TInt a;
	RThread::Rendezvous((TInt)&a);  // Complete rendezvous using address of 'a' which is on the stack
	return 0;
	}

void testThreadStackInfo()
	{
	// Check the info about the current thread's stack
	RThread thread;
	TThreadStackInfo info;
	TInt r = thread.StackInfo(info);
	test(r==KErrNone);
	TLinAddr a = (TLinAddr)&info;
	test.Printf(_L("address on stack=%x iBase=%x iLimit=%x iExpandLimit=%x"),a,info.iBase,info.iLimit,info.iExpandLimit);
	test(a<=info.iBase);
	test(a>=info.iLimit);
	test(info.iExpandLimit<=info.iLimit);

	// Create another thread
	r=thread.Create(_L("StackInfoThread"),StackInfoThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)NULL);
	test(r==KErrNone);
	thread.SetPriority(EPriorityLess);

	// Resume thread and wait for it to run
	TRequestStatus stat;
	thread.Rendezvous(stat);
	thread.Resume();
	User::WaitForRequest(stat);

	// Test getting stack info of another thread
	r = thread.StackInfo(info);
	test(r==KErrNone);
	a = stat.Int(); // a = an address on the threads stack
	test.Printf(_L("address on stack=%x iBase=%x iLimit=%x iExpandLimit=%x"),a,info.iBase,info.iLimit,info.iExpandLimit);
	test(a<=info.iBase);
	test(a>=info.iLimit);
	test(info.iExpandLimit<=info.iLimit);

	// Let thread run to end
	thread.Logon(stat);
	User::WaitForRequest(stat);
	test(stat.Int()==0);
	}

GLDEF_C TInt E32Main()
//
// Main
//
	{	
 
	// don't want just in time debugging as we trap panics
	TBool justInTime=User::JustInTime(); 
	User::SetJustInTime(EFalse); 

	test.Title();
	__UHEAP_MARK;
	

	TFullName name;
	name=RThread().Name();

	test.Start(_L("Test threads"));

	test.Next(_L("Test 1"));
	test1();
 
	test.Next(_L("Test create"));
	testCreate();

	test.Next(_L("Test RUndertaker"));
	testUndertaker(EOwnerProcess);

	test.Next(_L("Test2"));
	test2(EOwnerProcess);
	User::SetJustInTime(justInTime);	
	test.Next(_L("Test3"));
	test3();
	test.Next(_L("Test4"));
	test4();
	test.Next(_L("Completed but unclosed thread"));
	User::SetJustInTime(EFalse);
	test5();
	User::SetJustInTime(justInTime);
	test.Next(_L("Suspend/Resume"));
	test7();
	test.Next(_L("Testing thread duplication"));
	User::SetJustInTime(EFalse);
	test6();
	User::SetJustInTime(justInTime);
	test.Next(_L("Get thread's heap"));
	test8();
	test.Next(_L("Test read NULL remotely (HA-178)"));
	test9();
	test.Next(_L("Test Open(aFullName)"));
	testOpen();
	test.Next(_L("Test Reuse after a failed create"));
	testReuse();
	test.Next(_L("Test thread releases Mutex (HA-178)"));
	User::SetJustInTime(EFalse);
	testReleaseMutex();
	User::SetJustInTime(justInTime);
	test.Next(_L("Test Thread ID"));
	testId();
	test.Next(_L("Test RThread::StackInfo"));
	testThreadStackInfo();
	test.End();
	__UHEAP_MARKEND;
	return(KErrNone);
	}



