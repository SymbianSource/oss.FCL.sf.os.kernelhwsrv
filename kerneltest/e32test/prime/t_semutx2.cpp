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
// e32test\prime\t_semutx2.cpp
// Test RSemaphore and RMutex
// Overview:
// Tests the RSemaphore and RMutex
// API Information:
// RSemaphore, RMutex
// Details:
// - Test and verify that thread priorities work as expected.
// - Test and verify that signalling an RMutex from the wrong
// thread fails as expected.
// - Test and verify that mutex priority inheritance works as
// expected.
// - Perform an exhaustive state transition test using mutexs
// and up to ten threads. Verify priorities, order of execution, 
// mutex signalling, suspend, resume, kill and close. Verify 
// results are as expected.
// - Test semaphore speed by counting how many Wait/Signal 
// operations can be completed in one second.
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

RMutex M1;
RMutex M2;
RSemaphore S;

const TInt KBufferSize=4096;
TUint ThreadId[KBufferSize];
TInt PutIx;
TInt GetIx;
TInt Count;
RThread Main;


RTest test(_L("T_SEMUTX2"));

/*****************************************************************************
 * Utility functions / macros
 *****************************************************************************/
#define TRACE_ON	User::SetDebugMask(0xffdfffff);
#define TRACE_OFF	User::SetDebugMask(0x80000000);

//#define MCOUNT(m,c)	test((m).Count() ==(c))
// mutex count value is not visible for user any more
#define MCOUNT(m,c) (void)(1)
#define IDCHECK(x) test_Equal((x), GetNextId())
#define NUMCHECK(x)	test_Equal((x), NumIdsPending())

#define id0		id[0]
#define id1		id[1]
#define id2		id[2]
#define id3		id[3]
#define id4		id[4]
#define id5		id[5]
#define id6		id[6]
#define id7		id[7]
#define id8		id[8]
#define id9		id[9]
#define id10	id[10]

TBool Exists(const TDesC& aName)
	{
	TFullName n(RProcess().Name());
	n+=_L("::");
	n+=aName;
	TFindThread ft(n);
	TFullName fn;
	return ft.Next(fn)==KErrNone;
	}

TBool Exists(TInt aNum)
	{
	TBuf<4> b;
	b.Num(aNum);
	return Exists(b);
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

void Kick(RThread& t)
	{
	TRequestStatus s;
	TRequestStatus* pS=&s;
	t.RequestComplete(pS,0);
	}

TUint GetNextId()
	{
	if (GetIx<PutIx)
		return ThreadId[GetIx++];
	return 0;
	}

TInt NumIdsPending()
	{
	return PutIx-GetIx;
	}

/*****************************************************************************
 * General tests
 *****************************************************************************/
TInt Test0Thread(TAny* aPtr)
	{
	TInt& count=*(TInt*)aPtr;
	++count;
	Main.SetPriority(EPriorityMuchMore);
	++count;
	Main.SetPriority(EPriorityMuchMore);
	++count;
	RThread().SetPriority(EPriorityNormal);
	++count;
	return 0;
	}

void Test0()
	{
	User::After(100000);	// Test fails intermittently on hardware unless we pause here 
	
	test.Start(_L("Test thread priorities work"));
	test.Next(_L("Create thread"));
	RThread t;
	TInt count=0;
	TRequestStatus s;
	TInt r=t.Create(_L("Test0"),Test0Thread,0x1000,NULL,&count);
	test_KErrNone(r);
	t.Logon(s);
	test_KErrNone(r);
	User::After(10000);		// make sure we have a full timeslice
	t.Resume();

	test_Equal(0, count);			// t shouldn't have run yet
	RThread().SetPriority(EPriorityMuchMore);	// shouldn't reschedule (priority unchanged)
	test_Equal(0, count);
	RThread().SetPriority(EPriorityMore);	// shouldn't reschedule (priority decreasing, but not enough)
	test_Equal(0, count);
	RThread().SetPriority(EPriorityMuchMore);	// shouldn't reschedule (priority increasing)
	test_Equal(0, count);
	RThread().SetPriority(EPriorityNormal);	// should reschedule (we go behind t)
	test_Equal(1, count);
	RThread().SetPriority(EPriorityLess);	// should reschedule (priority decreasing to below t)
	test_Equal(2, count);
	t.SetPriority(EPriorityMuchMore);		// shouldn't reschedule (round-robin, timeslice not expired)
	test_Equal(2, count);
	t.SetPriority(EPriorityNormal);			// shouldn't reschedule (t's priority decreasing)
	test_Equal(2, count);
	t.SetPriority(EPriorityNormal);			// shouldn't reschedule (t's priority unchanged)
	test_Equal(2, count);
	BusyWait(100000);		// use up our timeslice
	t.SetPriority(EPriorityMuchMore);		// should reschedule (round-robin, timeslice expired)
	test_Equal(3, count);
	test_Equal(KRequestPending, s.Int());
	test_Equal(EExitPending, t.ExitType());
	t.SetPriority(EPriorityRealTime);		// should reschedule (t increases above current)
	test_Equal(4, count);
	test_KErrNone(s.Int());						// t should have exited
	test_Equal(EExitKill, t.ExitType());
	User::WaitForRequest(s);
	RThread().SetPriority(EPriorityMuchMore);
	t.Close();
	test.End();
	}

TInt Test1Thread(TAny*)
	{
	M1.Signal();
	return 0;
	}

void Test1()
	{
	test.Start(_L("Test signalling from wrong thread"));
	TInt r=M1.CreateLocal();
	test_KErrNone(r);
	M1.Wait();
	RThread t;
	r=t.Create(_L("Test1"),Test1Thread,0x1000,NULL,NULL);
	test_KErrNone(r);
	TRequestStatus s;
	t.Logon(s);
	t.Resume();
	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	User::WaitForRequest(s);
	User::SetJustInTime(jit);
	test_Equal(EAccessDenied, s.Int());
	test_Equal(EExitPanic, t.ExitType());
	test_Equal(EAccessDenied, t.ExitReason());
	test(t.ExitCategory()==_L("KERN-EXEC"));
	t.Close();
	M1.Close();
	test.End();
	}

/*****************************************************************************
 * Mutex priority inheritance
 *****************************************************************************/

const TInt KTestDelay = 1000000;

TInt LowThread(TAny* aPtr)
	{
	TInt& count=*(TInt*)aPtr;
	FOREVER
		{
		M1.Wait();
		++count;
		BusyWait(KTestDelay);
		M1.Signal();
		User::WaitForAnyRequest();
		}
	}

TInt MedThread(TAny* aPtr)
	{
	TInt& count=*(TInt*)aPtr;
	FOREVER
		{
		++count;
		User::WaitForAnyRequest();
		}
	}

TInt HighThread(TAny* aPtr)
	{
	TInt& count=*(TInt*)aPtr;
	FOREVER
		{
		M2.Wait();
		++count;
		M1.Wait();
		++count;
		BusyWait(KTestDelay);
		M1.Signal();
		M2.Signal();
		User::WaitForAnyRequest();
		}
	}

void TestMutex1()
	{
	test.Start(_L("Test mutex priority inheritance"));

	test.Next(_L("Create mutex"));
	TInt r=M1.CreateLocal();
	test_KErrNone(r);

	test.Next(_L("Create low priority thread"));
	TInt lowcount=0;
	RThread low;
	r=low.Create(_L("low"),LowThread,0x1000,NULL,&lowcount);
	test_KErrNone(r);
	low.SetPriority(EPriorityMuchLess);
	test(Exists(_L("low")));

	test.Next(_L("Create medium priority thread"));
	TInt medcount=0;
	RThread med;
	r=med.Create(_L("med"),MedThread,0x1000,NULL,&medcount);
	test_KErrNone(r);
	med.SetPriority(EPriorityNormal);
	test(Exists(_L("med")));

	test.Next(_L("Start low priority thread"));
	low.Resume();
	User::AfterHighRes(KTestDelay/10);
	test_Equal(1, lowcount);
//	MCOUNT(M1,0);

	test.Next(_L("Start medium priority thread"));
	med.Resume();
	User::AfterHighRes(KTestDelay/10);
	test_Equal(1, medcount);
	Kick(med);
	User::AfterHighRes(KTestDelay/10);
	test_Equal(2, medcount);
	Kick(med);

	M1.Wait();
	test_Equal(1, lowcount);
	test_Equal(2, medcount);
	test.Next(_L("Wait, check medium runs"));
	User::AfterHighRes(KTestDelay/10);
	test_Equal(3, medcount);
	M1.Signal();

	test.Next(_L("Create mutex 2"));
	r=M2.CreateLocal();
	test_KErrNone(r);

	test.Next(_L("Create high priority thread"));
	TInt highcount=0;
	RThread high;
	r=high.Create(_L("high"),HighThread,0x1000,NULL,&highcount);
	test_KErrNone(r);
	high.SetPriority(EPriorityMore);
	test(Exists(_L("high")));

	Kick(low);
	User::AfterHighRes(KTestDelay/10);
//	MCOUNT(M1,0);
	Kick(med);

//	MCOUNT(M2,1);
	high.Resume();
	User::AfterHighRes(KTestDelay/10);
//	MCOUNT(M2,0);
//	MCOUNT(M1,-1);
	test_Equal(1, highcount);

	M2.Wait();
	test_Equal(2, lowcount);
	test_Equal(3, medcount);
	test_Equal(2, highcount);
	test.Next(_L("Wait, check medium runs"));
	User::AfterHighRes(KTestDelay/10);
	test_Equal(4, medcount);
	M2.Signal();

	test.Next(_L("Kill threads"));
	low.Kill(0);
	med.Kill(0);
	high.Kill(0);
	low.Close();
	med.Close();
	high.Close();
	test(!Exists(_L("low")));
	test(!Exists(_L("med")));
	test(!Exists(_L("high")));

	M1.Close();
	test.End();
	}

/*****************************************************************************
 * Utilities for mutex exhaustive state transition test
 *****************************************************************************/
void MutexWait()
	{
	M1.Wait();
	++Count;
	ThreadId[PutIx++]=(TUint)RThread().Id();
	}

void MutexSignal()
	{
	M1.Signal();
	}

typedef void (*PFV)(void);
TInt ThreadFunction(TAny* aPtr)
	{
	PFV& f=*(PFV*)aPtr;
	FOREVER
		{
		MutexWait();
		if (f)
			f();
		MutexSignal();
		User::WaitForAnyRequest();
		}
	}

void Exit()
	{
	User::Exit(0);
	}

TUint CreateThread(RThread& t, TInt n, TAny* aPtr)
	{
	TBuf<4> b;
	b.Num(n);
	TInt r=t.Create(b,ThreadFunction,0x1000,NULL,aPtr);
	test_KErrNone(r);
	t.Resume();
	TUint id=t.Id();
	test.Printf(_L("id=%d\n"),id);
	return id;
	}

/*
Possible thread relationships with mutex:
	Holding
	Waiting
	Waiting + suspended
	Hold Pending

Need to verify correct behaviour when the following actions occur for each of these states:
	Suspend thread
	Resume thread
	Change thread priority
	Thread exits
	Thread is killed
	Mutex deleted
*/

PFV HoldExtra;
void KickMain()
	{
	RThread me;
	Kick(Main);
	User::WaitForAnyRequest();
	me.SetPriority(EPriorityMuchMore);
	MutexSignal();						// this should wake up t8
	MutexWait();
	MutexSignal();						// this should wake up t9
	MutexWait();
	Kick(Main);
	User::WaitForAnyRequest();
	if (HoldExtra)
		HoldExtra();
	}

void RackEmUp(RThread* t, PFV* f, TUint* id)
	{
	// set up t4 holding
	// t1, t2, t5, t10 waiting
	// t3, t6, t7 waiting+suspended
	// t8, t9 pending
	MCOUNT(M1,1);			// check mutex free
	Kick(t[4]);
	f[4]=&KickMain;
	User::WaitForAnyRequest();
	MCOUNT(M1,0);			// check mutex now held
	TInt i;
	for (i=1; i<=10; ++i)
		if (i!=4)
			Kick(t[i]);		// wake up threads
	User::After(50000);		// let threads wait
	MCOUNT(M1,-9);			// check 9 threads waiting
	Kick(t[4]);
	User::WaitForAnyRequest();
	MCOUNT(M1,-7);			// check 7 threads waiting
	NUMCHECK(3);
	IDCHECK(id4);			// from the initial wait
	IDCHECK(id4);			// now have t8, t9 pending, t4 holding, rest waiting
	IDCHECK(id4);			// now have t8, t9 pending, t4 holding, rest waiting
	t[4].SetPriority(EPriorityNormal);
	t[7].Resume();			// test resume when not suspended
	MCOUNT(M1,-7);			// check 7 threads waiting
	t[3].Suspend();
	t[6].Suspend();
	t[7].Suspend();			// now have required state
	t[3].Suspend();			// suspend and resume t3 again for good measure
	t[3].Resume();
	MCOUNT(M1,-7);			// check 7 threads waiting
	HoldExtra=NULL;
	}

void SimpleCheck(TInt n, const TUint* id, ...)
	{
	VA_LIST list;
	VA_START(list,id);
	User::After(50000);		// let stuff happen
	NUMCHECK(n);
	TInt i;
	for (i=0; i<n; ++i)
		{
		TInt tn=VA_ARG(list,TInt);
		IDCHECK(id[tn]);
		}
	}

void Resurrect(TInt n, TThreadPriority aPriority, RThread* t, PFV* f, TUint* id)
	{
	f[n]=NULL;
	id[n]=CreateThread(t[n],n,f+n);
	t[n].SetPriority(EPriorityRealTime);
	t[n].SetPriority(aPriority);
	NUMCHECK(1);
	IDCHECK(id[n]);
	}

/*****************************************************************************
 * Mutex exhaustive state transition test
 *****************************************************************************/
void TestMutex2()
	{
	test.Start(_L("Test mutex state transitions"));
	RThread t[11];
	TUint id[11];
	PFV f[11]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	id[0]=(TUint)RThread().Id();
	PutIx=0;
	GetIx=0;
	Count=0;
	test.Next(_L("Create mutex"));
	TInt r=M1.CreateLocal();
	test_KErrNone(r);
	MCOUNT(M1,1);
	MutexWait();
	MCOUNT(M1,0);
	IDCHECK(id[0]);
	test.Next(_L("Create threads"));
	TInt i;
	for (i=1; i<=5; ++i)
		id[i]=CreateThread(t[i],i,f+i);
	User::After(50000);		// let threads wait on mutex
	MCOUNT(M1,-5);			// check 5 threads waiting
	for (i=-4; i<=0; ++i)
		{
		MutexSignal(); // wake up next thread
		MutexWait();
		MCOUNT(M1,i);		// check right number of threads waiting
		IDCHECK(id0);		// check we got mutex back straight away
		}
	MutexSignal();
	User::After(50000);		// let threads claim mutex
	MutexWait();
	MCOUNT(M1,0);			// check no threads waiting
	for (i=1; i<=5; ++i)
		{
		IDCHECK(id[i]);		// check they ran in order t1...t5
		Kick(t[i]);			// wake up thread
		}
	IDCHECK(id0);			// check we got it back last
	t[4].SetPriority(EPriorityMore);	// make t4 higher priority
	User::After(50000);		// let threads wait on mutex
	MCOUNT(M1,-5);			// check 5 threads waiting
//	temp = M1.Count();
	MutexSignal();
//	temp = M1.Count();
	User::After(50000);		// let threads claim mutex
	MutexWait();
//	temp = M1.Count();
	MCOUNT(M1,0);			// check no threads waiting
	IDCHECK(id4);			// check they ran in order t4,t1,t2,t3,t5
	IDCHECK(id1);
	IDCHECK(id2);
	IDCHECK(id3);
	IDCHECK(id5);
	IDCHECK(id0);
	t[4].SetPriority(EPriorityNormal);	// make t4 normal priority
	for (i=1; i<=5; ++i)
		Kick(t[i]);			// wake up thread
	User::After(50000);		// let threads wait on mutex
	MCOUNT(M1,-5);			// check 5 threads waiting

	t[3].SetPriority(EPriorityMore);	// make t3 higher priority
//	temp = M1.Count();
	MutexSignal();
//	temp = M1.Count();
	User::After(50000);		// let threads claim mutex
	MutexWait();
//	temp = M1.Count();
	MCOUNT(M1,0);			// check no threads waiting
	IDCHECK(id3);			// check they ran in order t3,t1,t2,t4,t5
	IDCHECK(id1);
	IDCHECK(id2);
	IDCHECK(id4);
	IDCHECK(id5);
	IDCHECK(id0);
	t[3].SetPriority(EPriorityNormal);	// make t4 normal priority
	for (i=1; i<=5; ++i)
		Kick(t[i]);			// wake up threads
	User::After(50000);		// let threads wait on mutex
	MCOUNT(M1,-5);			// check 5 threads waiting

	t[2].SetPriority(EPriorityMore);	// make t2 higher priority
	t[1].SetPriority(EPriorityLess);	// make t1 lower priority
	MutexSignal();
	User::After(50000);		// let threads claim mutex
	MutexWait();
	MCOUNT(M1,0);			// check no threads waiting
	IDCHECK(id2);			// check they ran in order t2,t3,t4,t5,t1
	IDCHECK(id3);
	IDCHECK(id4);
	IDCHECK(id5);
	IDCHECK(id1);
	IDCHECK(id0);

	for (i=1; i<=5; ++i)
		Kick(t[i]);			// wake up threads
	User::After(50000);		// let threads wait on mutex
	MCOUNT(M1,-5);			// check 5 threads waiting
	MutexSignal();
	User::After(50000);		// let threads claim mutex
	MutexWait();
	MCOUNT(M1,0);			// check no threads waiting
	IDCHECK(id2);			// check they ran in order t2,t3,t4,t5,t1
	IDCHECK(id3);
	IDCHECK(id4);
	IDCHECK(id5);
	IDCHECK(id1);
	IDCHECK(id0);

	test(Exists(2));
	for (i=1; i<=5; ++i)
		Kick(t[i]);			// wake up threads
	User::After(50000);		// let threads wait on mutex
	MCOUNT(M1,-5);			// check 5 threads waiting
	f[2]=&Exit;				// make t2 exit while holding the mutex
	MutexSignal();
	User::After(50000);		// let threads claim mutex
	MutexWait();
	MCOUNT(M1,0);			// check no threads waiting
	test_Equal(EExitKill, t[2].ExitType());	// check t2 has exited
	t[2].Close();
	test(!Exists(2));
	IDCHECK(id2);			// check they ran in order t2,t3,t4,t5,t1
	IDCHECK(id3);
	IDCHECK(id4);
	IDCHECK(id5);
	IDCHECK(id1);
	IDCHECK(id0);
	f[2]=NULL;
	id[2]=CreateThread(t[2],2,f+2);	// recreate t2
	User::After(50000);		// let new t2 wait on mutex
	MCOUNT(M1,-1);			// check 1 thread waiting
	MutexSignal();
	User::After(50000);		// let t2 claim mutex
	MutexWait();
	MCOUNT(M1,0);			// check no threads waiting
	IDCHECK(id2);
	IDCHECK(id0);

	t[2].SetPriority(EPriorityLess);	// make t2 lower priority
	for (i=1; i<=5; ++i)
		Kick(t[i]);			// wake up threads
	User::After(50000);		// let threads wait on mutex
	MCOUNT(M1,-5);			// check 5 threads waiting
	MutexSignal();			// t3 now pending
	MCOUNT(M1,-3);			// check 4 threads waiting, mutex free
	t[3].Suspend();			// this should wake up t4
	MCOUNT(M1,-2);			// check 3 threads waiting, mutex free
	User::After(50000);		// let threads claim mutex
	MutexWait();
	MCOUNT(M1,0);			// check no threads still waiting
	IDCHECK(id4);			// check they ran in order t4,t5,t1,t2
	IDCHECK(id5);
	IDCHECK(id1);
	IDCHECK(id2);
	IDCHECK(id0);
	Kick(t[1]);				// wake up t1
	User::After(50000);		// let thread wait on mutex
	MCOUNT(M1,-1);			// check 1 thread waiting
	t[3].Resume();			// resume pending t3
	MutexSignal();
	User::After(50000);		// let t2 claim mutex
	MutexWait();
	MCOUNT(M1,0);			// check no threads waiting
	IDCHECK(id3);			// check order t3,t1
	IDCHECK(id1);
	IDCHECK(id0);

	for (i=1; i<=5; ++i)
		Kick(t[i]);			// wake up threads
	User::After(50000);		// let threads wait on mutex
	MCOUNT(M1,-5);			// check 5 threads waiting
	t[4].Suspend();			// suspend t4
	MCOUNT(M1,-5);			// check 5 threads waiting
	t[4].Suspend();			// suspend t4 again
	MCOUNT(M1,-5);			// check 5 threads waiting
	MutexSignal();
	User::After(50000);		// let threads claim mutex
	MutexWait();
	MCOUNT(M1,-1);			// check 1 thread still waiting
	IDCHECK(id3);			// check they ran in order t3,t5,t1,t2
	IDCHECK(id5);
	IDCHECK(id1);
	IDCHECK(id2);
	IDCHECK(id0);
	MutexSignal();
	t[4].Resume();
	User::After(50000);		// let threads claim mutex
	MutexWait();
	IDCHECK(id0);			// check thread didn't get mutex (still suspended)
	MutexSignal();
	t[4].Resume();
	User::After(50000);		// let threads claim mutex
	MutexWait();
	IDCHECK(id4);			// check order t4 then this
	IDCHECK(id0);

	for (i=1; i<=5; ++i)
		Kick(t[i]);			// wake up threads
	User::After(50000);		// let threads wait on mutex
	MCOUNT(M1,-5);			// check 5 threads waiting
	MutexWait();			// wait on mutex again
	IDCHECK(id0);
	MutexSignal();			// signal once
	MCOUNT(M1,-5);			// check 5 threads still waiting
	MutexSignal();			// signal again
	MCOUNT(M1,-3);			// check one thread has been woken up and mutex is now free
	User::After(50000);		// let threads claim mutex
	MutexWait();
	MCOUNT(M1,0);			// check no threads still waiting
	IDCHECK(id3);			// check they ran in order t3,t4,t5,t1,t2
	IDCHECK(id4);
	IDCHECK(id5);
	IDCHECK(id1);
	IDCHECK(id2);
	IDCHECK(id0);

	test.Next(_L("Create more threads"));
	for (i=6; i<=10; ++i)
		id[i]=CreateThread(t[i],i,f+i);
	User::After(50000);		// let threads wait on mutex
	MCOUNT(M1,-5);			// check 5 threads waiting
	MutexSignal();
	User::After(50000);		// let threads claim mutex
	MCOUNT(M1,1);			// check no threads still waiting and mutex free
	IDCHECK(id6);			// check they ran in order t6,t7,t8,t9,t10
	IDCHECK(id7);
	IDCHECK(id8);
	IDCHECK(id9);
	IDCHECK(id10);
	t[8].SetPriority(EPriorityMore);	// t1-t3=less, t4-t7=normal, t8-t10 more, t0 much more
	t[9].SetPriority(EPriorityMore);
	t[10].SetPriority(EPriorityMore);
	t[2].SetPriority(EPriorityLess);
	t[3].SetPriority(EPriorityLess);

	RackEmUp(t,f,id);
	SimpleCheck(0,NULL,NULL);	// holding thread still blocked
	Kick(t[4]);
	SimpleCheck(6,id,10,8,9,5,1,2);	// 3,6,7 suspended
	t[3].Resume();
	t[6].Resume();
	t[7].Resume();
	SimpleCheck(3,id,6,7,3);	// 3,6,7 resumed

	RackEmUp(t,f,id);
	SimpleCheck(0,NULL,NULL);	// holding thread still blocked
	Kick(t[4]);
	t[4].Suspend();
	SimpleCheck(0,NULL,NULL);	// holding thread suspended
	t[4].Resume();
	SimpleCheck(6,id,10,8,9,5,1,2);	// 3,6,7 suspended
	t[3].Resume();
	t[6].Resume();
	t[7].Resume();
	SimpleCheck(3,id,6,7,3);	// 3,6,7 resumed

	RackEmUp(t,f,id);
	Kick(t[4]);
	t[4].SetPriority(EPriorityRealTime);
	MCOUNT(M1,-5);			// should be 6 waiting, mutex free
	t[4].SetPriority(EPriorityNormal);
	t[8].SetPriority(EPriorityRealTime);	// change pending thread priority
	MCOUNT(M1,-4);			// should be 5 waiting, mutex free
	t[8].SetPriority(EPriorityMore);
	NUMCHECK(1);
	IDCHECK(id8);
	t[3].SetPriority(EPriorityRealTime);	// change suspended thread priority
	SimpleCheck(5,id,9,10,5,1,2);	// 3,6,7 suspended
	t[6].Resume();
	t[7].Resume();
	t[3].Resume();			// this should run right away
	NUMCHECK(1);
	IDCHECK(id3);
	SimpleCheck(2,id,6,7);	// 6,7 resumed
	t[3].SetPriority(EPriorityLess);

	RackEmUp(t,f,id);
	Kick(t[4]);
	t[1].SetPriority(EPriorityRealTime);	// change waiting thread priority
											// this should run right away
	NUMCHECK(1);
	IDCHECK(id1);
	t[1].SetPriority(EPriorityLess);
	// t8,t9,t10 should now be pending
	MCOUNT(M1,1-5);
	t[8].Suspend();			// this should wake up t5
	t[9].Suspend();			// this should wake up t2
	MCOUNT(M1,1-3);
	t[8].Suspend();			// this should have no further effect
	t[8].Resume();			// this should have no further effect
	MCOUNT(M1,1-3);
	SimpleCheck(3,id,10,5,2);
	MCOUNT(M1,1-3);
	t[3].Resume();
	t[6].Resume();
	t[7].Resume();
	t[8].Resume();
	t[9].Resume();
	SimpleCheck(5,id,8,9,6,7,3);

	RackEmUp(t,f,id);
	MCOUNT(M1,-7);
	t[8].Suspend();			// this shouldn't wake anything up
	t[9].Suspend();			// this shouldn't wake anything up
	MCOUNT(M1,-7);
	Kick(t[4]);
	t[4].SetPriority(EPriorityRealTime);
	MCOUNT(M1,1-6);				// should be 6 waiting, mutex free, t10 pending
	t[4].SetPriority(EPriorityNormal);
	t[10].SetPriority(EPriorityLess);	// this should wake up t5
	MCOUNT(M1,1-5);				// should be 5 waiting, mutex free, t10, t5 pending
	SimpleCheck(4,id,5,10,1,2);
	t[3].SetPriority(EPriorityRealTime);	// boost suspended+waiting thread
	MCOUNT(M1,1-3);			// should be 3 waiting+suspended, mutex free, t8, t9 pending+suspended
	t[6].Resume();
	t[7].Resume();
	t[8].Resume();
	t[9].Resume();
	t[3].Resume();			// this should run immediately
	MCOUNT(M1,1);			// t8,t9,t6,t7 pending, mutex free
	NUMCHECK(1);
	IDCHECK(id3);			// t3 should have run
	t[3].SetPriority(EPriorityLess);
	t[9].SetPriority(EPriorityMuchLess);	// lower pending thread priority
	SimpleCheck(4,id,8,6,7,9);
	t[9].SetPriority(EPriorityMore);
	t[10].SetPriority(EPriorityMore);

	RackEmUp(t,f,id);
	MCOUNT(M1,-7);
	t[8].Suspend();			// this shouldn't wake anything up
	t[9].Suspend();			// this shouldn't wake anything up
	MCOUNT(M1,-7);
	Kick(t[4]);
	MCOUNT(M1,-7);
	t[4].SetPriority(EPriorityRealTime);
	MCOUNT(M1,1-6);			// should be 6 waiting, mutex free, t10 pending, t8,t9 pending+suspended
	t[4].SetPriority(EPriorityNormal);
	t[10].SetPriority(EPriorityMuchLess);	// lower pending thread priority
	MCOUNT(M1,1-5);			// should now be 5 waiting, mutex free, t10,t5 pending, t8,t9 pending+suspended
	t[6].Resume();
	t[7].Resume();
	t[3].Resume();			// this gets made READY straight away
	SimpleCheck(7,id,5,6,7,3,1,2,10);
	t[8].Resume();
	t[9].Resume();
	SimpleCheck(2,id,8,9);
	t[10].SetPriority(EPriorityMore);

	RackEmUp(t,f,id);
	MCOUNT(M1,-7);
	Kick(t[4]);
	t[9].Kill(0);			// kill pending thread
	t[9].Close();
	test(!Exists(9));
	t[1].Kill(0);			// kill waiting thread
	t[1].Close();
	test(!Exists(1));
	t[6].Kill(0);			// kill suspended+waiting thread
	t[6].Close();
	t[7].Resume();
	t[3].Resume();
	test(!Exists(6));
	SimpleCheck(6,id,10,8,5,7,2,3);	// 8 runs first and gets blocked behind 10
	Resurrect(9,EPriorityMore,t,f,id);
	Resurrect(1,EPriorityLess,t,f,id);
	Resurrect(6,EPriorityNormal,t,f,id);

	RackEmUp(t,f,id);
	MCOUNT(M1,-7);
	t[8].Suspend();			// this shouldn't wake anything up
	t[9].Suspend();			// this shouldn't wake anything up
	MCOUNT(M1,-7);
	Kick(t[4]);
	MCOUNT(M1,-7);
	t[4].SetPriority(EPriorityRealTime);
	MCOUNT(M1,1-6);			// should be 6 waiting, mutex free, t10 pending, t8,t9 pending+suspended
	t[4].SetPriority(EPriorityNormal);
	t[10].Kill(0);			// kill pending thread - this should wake up t5
	t[10].Close();
	test(!Exists(10));
	MCOUNT(M1,1-5);			// should be 5 waiting, mutex free, t5 pending, t8,t9 pending+suspended
	t[5].SetPriority(EPriorityRealTime);	// this should make t5 run
	MCOUNT(M1,1-4);			// should be 4 waiting, mutex free, t1 pending, t8,t9 pending+suspended
	t[5].SetPriority(EPriorityNormal);
	NUMCHECK(1);
	IDCHECK(id5);
	t[8].SetPriority(EPriorityRealTime);	// this shouldn't make anything happen
	MCOUNT(M1,1-4);			// mutex free, t1 pending, t8,t9 pending+suspended, t3,t6,t7 wait+susp, t2 waiting
	NUMCHECK(0);
	t[8].Resume();
	MCOUNT(M1,1-3);			// mutex free, t1,t2 pending, t9 pending+suspended, t3,t6,t7 wait+susp
	NUMCHECK(1);
	IDCHECK(id8);
	t[8].SetPriority(EPriorityMore);
	t[3].Resume();
	t[6].Resume();
	t[7].Resume();
	t[9].Resume();
	SimpleCheck(6,id,9,6,7,1,2,3);
	Resurrect(10,EPriorityMore,t,f,id);

	RackEmUp(t,f,id);
	MCOUNT(M1,-7);
	t[8].Suspend();			// this shouldn't wake anything up
	t[9].Suspend();			// this shouldn't wake anything up
	MCOUNT(M1,-7);
	Kick(t[4]);
	MCOUNT(M1,-7);
	t[4].SetPriority(EPriorityRealTime);
	MCOUNT(M1,1-6);			// mutex free, t10 pending, t8,t9 pending+susp, t3,t6,t7 wait+susp, t1,t2,t5 wait
	t[4].SetPriority(EPriorityNormal);
	t[1].SetPriority(EPriorityRealTime);	// this should be able to run and claim the mutex
	NUMCHECK(1);
	IDCHECK(id1);
	MCOUNT(M1,1-4);			// mutex free, t10,t5 pending, t8,t9 pending+susp, t3,t6,t7 wait+susp, t2 wait
	t[1].SetPriority(EPriorityLess);
	t[3].Resume();
	t[6].Resume();
	t[7].Resume();
	t[9].Resume();
	t[8].Resume();
	SimpleCheck(8,id,10,9,8,5,6,7,3,2);

	RackEmUp(t,f,id);
	MCOUNT(M1,-7);
	Kick(t[4]);
	M1.Close();				// close the mutex - non-suspended threads should all panic with KERN-EXEC 0
	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	User::After(1000000);
	User::SetJustInTime(jit);
	for (i=1; i<=10; ++i)
		{
		if (i==3 || i==6 || i==7)
			{
			test_Equal(EExitPending, t[i].ExitType());
			}
		else
			{
			test_Equal(EExitPanic, t[i].ExitType());
			test_Equal(EBadHandle, t[i].ExitReason());
			test(t[i].ExitCategory()==_L("KERN-EXEC"));
			t[i].Close();
			test(!Exists(i));
			}
		}
	t[3].Resume();
	t[6].Resume();
	t[7].Resume();
	User::SetJustInTime(EFalse);
	User::After(1000000);
	User::SetJustInTime(jit);
	for (i=1; i<=10; ++i)
		{
		if (i==3 || i==6 || i==7)
			{
			test_Equal(EExitPanic, t[i].ExitType());
			test_Equal(EBadHandle, t[i].ExitReason());
			test(t[i].ExitCategory()==_L("KERN-EXEC"));
			t[i].Close();
			test(!Exists(i));
			}
		}

	test.End();
	}

/*****************************************************************************
 * Mutex benchmarks
 *****************************************************************************/
TInt MutexSpeed(TAny* aPtr)
	{
	TInt& count=*(TInt*)aPtr;
	RThread().SetPriority(EPriorityMore);
	FOREVER
		{
		M1.Wait();
		M1.Signal();
		++count;
		}
	}

TInt MutexSpeed2(TAny* aPtr)
	{
	TInt& count=*(TInt*)aPtr;
	RThread().SetPriority(EPriorityMore);
	FOREVER
		{
		M1.Wait();
		M1.Wait();
		M1.Signal();
		M1.Signal();
		++count;
		}
	}

void TestMutexSpeed()
	{
	test.Start(_L("Test mutex speed"));
	TInt count=0;
	TInt r=M1.CreateLocal();
	test_KErrNone(r);

	RThread t;
	r=t.Create(_L("Speed"),MutexSpeed,0x1000,NULL,&count);
	test_KErrNone(r);
	t.SetPriority(EPriorityRealTime);
	t.Resume();
	User::AfterHighRes(1000000);
	t.Kill(0);
	t.Close();
	test(!Exists(_L("Speed")));
	test.Printf(_L("%d wait/signal in 1 second\n"),count);

	TInt count2=0;
	r=t.Create(_L("Speed2"),MutexSpeed2,0x1000,NULL,&count2);
	test_KErrNone(r);
	t.SetPriority(EPriorityRealTime);
	t.Resume();
	User::AfterHighRes(1000000);
	t.Kill(0);
	t.Close();
	test(!Exists(_L("Speed2")));
	test.Printf(_L("%d double wait/signal in 1 second\n"),count2);

	M1.Close();
	test.End();
	}

/*****************************************************************************
 * Utilities for semaphore test
 *****************************************************************************/
void SemWait()
	{
	S.Wait();
	++Count;
	ThreadId[PutIx++]=(TUint)RThread().Id();
	}

void SemSignal()
	{
	S.Signal();
	}

TInt SemThreadFunction(TAny* aPtr)
	{
	PFV& f=*(PFV*)aPtr;
	FOREVER
		{
		SemWait();
		if (f)
			f();
		SemSignal();
		User::WaitForAnyRequest();
		}
	}

void Wait()
	{
	User::WaitForAnyRequest();
	}

TUint CreateSemThread(RThread& t, TInt n, TAny* aPtr)
	{
	TBuf<4> b;
	b.Num(n);
	TInt r=t.Create(b,SemThreadFunction,0x1000,NULL,aPtr);
	test_KErrNone(r);
	t.Resume();
	TUint id=t.Id();
	return id;
	}

/*
Possible thread relationships with semaphore:
	Waiting
	Waiting + suspended

Need to verify correct behaviour when the following actions occur for each of these states:
	Suspend thread
	Resume thread
	Change thread priority
	Thread exits
	Thread is killed
	Semaphore deleted
*/

void RackEmUp2(RThread* t, PFV* f, TUint* id)
	{
	// set up
	// t1, t2, t4, t5, t6, t8, t9 waiting
	// t3, t7, t10 waiting+suspended
	(void)f;
	MCOUNT(S,2);			// check semaphore level = 2
	SemWait();
	SemWait();
	MCOUNT(S,0);			// check semaphore level = 0
	NUMCHECK(2);
	IDCHECK(id0);
	IDCHECK(id0);
	TInt i;
	for (i=1; i<=10; ++i)
		Kick(t[i]);			// wake up threads
	User::After(50000);		// let threads wait
	MCOUNT(S,-10);			// check 10 threads waiting
	t[7].Resume();			// test resume when not suspended
	MCOUNT(S,-10);			// check 7 threads waiting
	t[3].Suspend();
	t[7].Suspend();
	t[10].Suspend();		// now have required state
	t[3].Suspend();			// suspend and resume t3 again for good measure
	t[3].Resume();
	MCOUNT(S,-7);			// check 7 threads waiting
	}

void Resurrect2(TInt n, TThreadPriority aPriority, RThread* t, PFV* f, TUint* id)
	{
	f[n]=NULL;
	id[n]=CreateSemThread(t[n],n,f+n);
	t[n].SetPriority(EPriorityRealTime);
	t[n].SetPriority(aPriority);
	NUMCHECK(1);
	IDCHECK(id[n]);
	}

/*****************************************************************************
 * Semaphore exhaustive state transition test
 *****************************************************************************/
void TestSemaphore()
	{
	test.Start(_L("Test semaphore state transitions"));
	RThread t[11];
	TUint id[11];
	PFV f[11]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	id[0]=(TUint)RThread().Id();
	PutIx=0;
	GetIx=0;
	Count=0;
	test.Next(_L("Create semaphore"));
	TInt r=S.CreateLocal(2);
	test_KErrNone(r);
	MCOUNT(S,2);
	SemWait();
	MCOUNT(S,1);
	SemSignal();
	MCOUNT(S,2);
	SemWait();
	SemWait();
	MCOUNT(S,0);
	S.Signal(2);
	MCOUNT(S,2);
	NUMCHECK(3);
	IDCHECK(id0);
	IDCHECK(id0);
	IDCHECK(id0);
	test.Next(_L("Create threads"));
	TInt i;
	for (i=1; i<=10; ++i)
		{
		id[i]=CreateSemThread(t[i],i,f+i);
		f[i]=&Wait;
		}
	t[8].SetPriority(EPriorityMore);	// t1-t3=less, t4-t7=normal, t8-t10 more, t0 much more
	t[9].SetPriority(EPriorityMore);
	t[10].SetPriority(EPriorityMore);
	t[1].SetPriority(EPriorityLess);
	t[2].SetPriority(EPriorityLess);
	t[3].SetPriority(EPriorityLess);
	User::After(50000);
	MCOUNT(S,-8);			// check 8 waiting
	NUMCHECK(2);
	IDCHECK(id8);
	IDCHECK(id9);			// check t8,t9 got through
	t[8].SetPriority(EPriorityRealTime);
	Kick(t[8]);				// let t8 run and signal
	t[8].SetPriority(EPriorityMore);
	MCOUNT(S,-7);			// check 7 waiting
	User::After(50000);		// let next thread obtain semaphore
	MCOUNT(S,-7);			// check 7 waiting
	NUMCHECK(1);
	IDCHECK(id10);			// check t10 got it
	Kick(t[10]);			// let t10 run and signal
	User::After(50000);		// let next thread obtain semaphore
	MCOUNT(S,-6);			// check 6 waiting
	NUMCHECK(1);
	IDCHECK(id4);			// check t4 got it
	t[1].SetPriority(EPriorityRealTime);	// boost t1
	MCOUNT(S,-6);			// check 6 still waiting
	User::After(50000);		// let next thread obtain semaphore
	MCOUNT(S,-6);			// check 6 still waiting
	NUMCHECK(0);
	Kick(t[9]);				// make t9 ready to run and signal
	MCOUNT(S,-6);			// check 6 still waiting
	User::After(50000);		// let next thread obtain semaphore
	MCOUNT(S,-5);			// check 5 waiting
	NUMCHECK(1);
	IDCHECK(id1);			// check t1 got it
	t[1].SetPriority(EPriorityLess);
	Kick(t[1]);				// kick all remaining threads
	Kick(t[2]);
	Kick(t[3]);
	Kick(t[4]);
	Kick(t[5]);
	Kick(t[6]);
	Kick(t[7]);
	User::After(50000);		// let them run and obtain/signal the semaphore
	MCOUNT(S,2);			// check semaphore now back to initial level
	SimpleCheck(5,id,5,6,7,2,3);

	for (i=1; i<=10; ++i)
		f[i]=NULL;
	RackEmUp2(t,f,id);		// set up threads waiting on semaphore again
	S.Signal();
	SimpleCheck(7,id,8,9,4,5,6,1,2);	// let them go
	MCOUNT(S,1);
	S.Wait();
	t[3].SetPriority(EPriorityRealTime);	// change suspended thread priority
	t[7].Resume();
	SimpleCheck(0,id);		// t7 should wait for signal
	S.Signal();
	SimpleCheck(1,id,7);
	MCOUNT(S,1);
	t[3].Resume();
	t[10].Resume();
	NUMCHECK(1);
	IDCHECK(id3);			// t3 should have grabbed semaphore as soon as we resumed it
	SimpleCheck(1,id,10);
	t[3].SetPriority(EPriorityLess);
	S.Signal();				// put level back to 2

	RackEmUp2(t,f,id);		// set up threads waiting on semaphore again
	S.Signal();
	SimpleCheck(7,id,8,9,4,5,6,1,2);	// let them go
	MCOUNT(S,1);
	S.Wait();
	t[3].SetPriority(EPriorityRealTime);	// change suspended thread priority
	t[7].Resume();
	SimpleCheck(0,id);		// t7 should wait for signal
	S.Signal();
	SimpleCheck(1,id,7);
	MCOUNT(S,1);
	t[10].Resume();
	t[3].Resume();			// t3 not woken up here since t10 has already been given the semaphore
	NUMCHECK(0);
	SimpleCheck(2,id,10,3);
	t[3].SetPriority(EPriorityLess);
	S.Signal();				// put level back to 2

	RackEmUp2(t,f,id);		// set up threads waiting on semaphore again
	S.Signal();
	SimpleCheck(7,id,8,9,4,5,6,1,2);	// let them go
	MCOUNT(S,1);
	S.Wait();
	t[3].SetPriority(EPriorityRealTime);	// change suspended thread priority
	t[7].Resume();
	SimpleCheck(0,id);		// t7 should wait for signal
	S.Signal();
	S.Signal();				// put level back to 2
	SimpleCheck(1,id,7);
	MCOUNT(S,2);
	t[10].Resume();
	t[3].Resume();			// t3 and t10 both woken up here, t3 should run and signal
	MCOUNT(S,1);
	NUMCHECK(1);
	IDCHECK(id3);
	SimpleCheck(1,id,10);
	t[3].SetPriority(EPriorityLess);

	RackEmUp2(t,f,id);		// set up threads waiting on semaphore again
	t[9].Kill(0);			// kill waiting thread
	t[9].Close();
	test(!Exists(9));
	t[10].Kill(0);			// kill suspended thread
	t[10].Close();
	test(!Exists(10));
	MCOUNT(S,-6);
	f[5]=&Exit;				// get t5 to exit after acquiring semaphore
	S.Signal();
	SimpleCheck(3,id,8,4,5);	// let them go
	MCOUNT(S,-3);			// check one signal has been lost due to t5 exiting
	t[5].Close();
	test(!Exists(5));
	t[3].Resume();
	t[7].Resume();
	MCOUNT(S,-5);
	S.Signal();
	SimpleCheck(5,id,6,7,1,2,3);	// let them go
	MCOUNT(S,1);
	Resurrect2(9,EPriorityMore,t,f,id);
	Resurrect2(10,EPriorityMore,t,f,id);
	Resurrect2(5,EPriorityNormal,t,f,id);
	S.Signal();

	RackEmUp2(t,f,id);		// set up threads waiting on semaphore again
	f[5]=&Exit;				// get t5 to exit after acquiring semaphore
	S.Close();				// close semaphore - threads should panic except for 5

	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	User::After(1000000);
	User::SetJustInTime(jit);
	for (i=1; i<=10; ++i)
		{
		if (i==3 || i==7 || i==10)
			{
			test_Equal(EExitPending, t[i].ExitType());
			}
		else if (i!=5)
			{
			test_Equal(EExitPanic, t[i].ExitType());
			test_Equal(EBadHandle, t[i].ExitReason());
			test(t[i].ExitCategory()==_L("KERN-EXEC"));
			t[i].Close();
			test(!Exists(i));
			}
		else
			{
			test_Equal(EExitKill, t[i].ExitType());
			test_Equal(0, t[i].ExitReason());
			t[i].Close();
			test(!Exists(i));
			}
		}
	t[3].Resume();
	t[7].Resume();
	t[10].Resume();
	User::SetJustInTime(EFalse);
	User::After(1000000);
	User::SetJustInTime(jit);
	for (i=1; i<=10; ++i)
		{
		if (i==3 || i==7 || i==10)
			{
			test_Equal(EExitPanic, t[i].ExitType());
			test_Equal(EBadHandle, t[i].ExitReason());
			test(t[i].ExitCategory()==_L("KERN-EXEC"));
			t[i].Close();
			test(!Exists(i));
			}
		}

	test.End();
	}

/*****************************************************************************
 * Semaphore benchmarks
 *****************************************************************************/
TInt SemSpeed(TAny* aPtr)
	{
	TInt& count=*(TInt*)aPtr;
	RThread().SetPriority(EPriorityMore);
	FOREVER
		{
		S.Wait();
		S.Signal();
		++count;
		}
	}

void TestSemSpeed()
	{
	test.Start(_L("Test semaphore speed"));
	TInt count=0;
	TInt r=S.CreateLocal(1);
	test_KErrNone(r);

	RThread t;
	r=t.Create(_L("SemSpeed"),SemSpeed,0x1000,NULL,&count);
	test_KErrNone(r);
	t.SetPriority(EPriorityRealTime);
	t.Resume();
	User::AfterHighRes(1000000);
	t.Kill(0);
	t.Close();
	test(!Exists(_L("SemSpeed")));
	test.Printf(_L("%d wait/signal in 1 second\n"),count);

	S.Close();
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
		test.Printf(_L("T_SEMUTX2 skipped, does not work on SMP\n"));
		return KErrNone;
		}	
	
	test.Title();

	test.Start(_L("Test mutexes and semaphores"));
	RThread().SetPriority(EPriorityMuchMore);
	TInt r=Main.Duplicate(RThread());
	test_KErrNone(r);

	Test0();
	Test1();
	TestMutex1();
	TestMutex2();
	TestSemaphore();
	
	TestMutexSpeed();
	TestSemSpeed();

	Main.Close();
	test.End();
	return KErrNone;
	}

