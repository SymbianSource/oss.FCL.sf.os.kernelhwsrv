// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\t_condvar.cpp
// Overview:
// Test the use of the RCondVar & RMutex classes.
// API Information:
// RCondVar, RMutex
// Details:
// - Create some local conditional variables and mutexes and verify results
// are as expected.
// - Create a test thread that waits on conditional variables and mutexes, 
// append some items on an array, signal the conditional variable and mutex,
// the thread then counts the number of items on the array and passes the 
// result back to the main process. Verify results are as expected. Repeat
// with different array data.
// - Verify that a RCondVar::Wait() panics when the thread does not hold the
// specified mutex (mutex not locked).
// - Test using two mutexes with 1 conditional variable, append some items to 
// an array, verify results from the thread are as expected. 
// - Create a second thread with higher priority, perform tests similar to
// above, verify results are as expected.
// - Verify the thread timeout values are as expected.
// - Create global conditional variables and global mutexes, using two threads
// test the RCondVar::Signal() and RMutex::Wait() results are as expected.
// - Test various combinations of creating a thread, suspending and killing it
// and signalling a conditional variable and mutex. Verify results are as
// expected.
// - Create a secondary process along with a global chunk, conditional variable 
// and mutex. Signal the conditional variable and verify the results are as 
// expected.
// - Using two threads, benchmark the number of conditional variable/mutex Signal
// and Wait iterations that can be completed per second.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include <e32test.h>
#include <e32ldr.h>
#include <e32def.h>
#include <e32def_private.h>
#include <u32std.h>

RTest test(_L("T_CONDVAR"));
RMutex M1;
RMutex M2;
RCondVar CV1;
RCondVar CV2;

#define __TRACE_LINE__	test.Printf(_L("Line %d\n"),__LINE__)

struct SThreadData
	{
	SThreadData();
	RMutex iM;
	RCondVar iV;
	RArray<TInt>* iA;
	TInt iTotal;
	TInt iInnerLoops;
	TInt iOuterLoops;
	TInt iTimeoutMs;
	TInt iTimeouts;
	TInt iBadCount;
	};

struct SThreadData2
	{
	SThreadData2();
	const TText* iMutexName;
	const TText* iCondVarName;
	TInt iInnerLoops;
	};

SThreadData::SThreadData()
	{
	memset(this, 0, sizeof(*this));
	}

SThreadData2::SThreadData2()
	{
	memset(this, 0, sizeof(*this));
	}

TInt Thread0(TAny*)
	{
	return CV1.Wait(M1);
	}

TInt Thread1(TAny* a)
	{
	TUint32 t1, t2;
	SThreadData& d = *(SThreadData*)a;
	TInt r = KErrNone;
	TInt i = 0;
	d.iM.Wait();
	FOREVER
		{
		while (d.iA->Count()<=i && r==KErrNone)
			{
			t1 = User::NTickCount();
			if (d.iTimeoutMs)
				r = d.iV.TimedWait(d.iM, d.iTimeoutMs*1000);
			else
				r = d.iV.Wait(d.iM);
			t2 = User::NTickCount();
			++d.iInnerLoops;
			if (r == KErrTimedOut)
				{
				++d.iTimeouts;
				TInt iv = (TInt)(t2-t1);
				if (iv<d.iTimeoutMs)
					++d.iBadCount;
				r = KErrNone;
				}
			}
		if (r != KErrNone)
			break;
		++d.iOuterLoops;
		TInt c = d.iA->Count();
		for (; i<c; ++i)
			d.iTotal += (*d.iA)[i];
		}
	return r;
	}

TInt Thread2(TAny* a)
	{
	TUint32 t1, t2;
	SThreadData& d = *(SThreadData*)a;
	TInt r = KErrNone;
	d.iM.Wait();
	RThread::Rendezvous(KErrNone);
	while (r==KErrNone)
		{
		t1 = User::NTickCount();
		if (d.iTimeoutMs)
			r = d.iV.TimedWait(d.iM, d.iTimeoutMs*1000);
		else
			r = d.iV.Wait(d.iM);
		t2 = User::NTickCount();
		++d.iInnerLoops;
		if (r == KErrTimedOut)
			{
			++d.iTimeouts;
			TInt iv = (TInt)(t2-t1);
			if (iv<d.iTimeoutMs)
				++d.iBadCount;
			r = KErrNone;
			}
		}
	return r;
	}

TInt Thread3(TAny* a)
	{
	SThreadData2& d = *(SThreadData2*)a;
	RMutex m;
	RCondVar cv;
	TInt r = m.OpenGlobal(TPtrC(d.iMutexName), EOwnerThread);
	if (r!=KErrNone)
		return r;
	r = cv.OpenGlobal(TPtrC(d.iCondVarName), EOwnerThread);
	if (r!=KErrNone)
		return r;
	m.Wait();
	while (r==KErrNone)
		{
		r = cv.Wait(m);
		++d.iInnerLoops;
		}
	return r;
	}

TInt Thread4(TAny* a)
	{
	volatile TInt& count = *(volatile TInt*)a;
	TInt r = KErrNone;
	M2.Wait();
	while (r==KErrNone)
		{
		r = CV2.Wait(M2);
		++count;
		}
	return r;
	}

TInt Thread5(TAny*)
	{
	FOREVER
		{
		M2.Wait();
		CV2.Signal();
		M2.Signal();
		}
	}

void RunBench()
	{
	test.Next(_L("Benchmark"));
	RThread t4, t5;
	TInt count = 0;
	TInt r = t4.Create(KNullDesC, &Thread4, 0x1000, 0x1000, 0x1000, &count);
	test(r==KErrNone);
	t4.SetPriority(EPriorityLess);
	r = t5.Create(KNullDesC, &Thread5, 0x1000, 0x1000, 0x1000, NULL);
	test(r==KErrNone);
	t5.SetPriority(EPriorityMuchLess);
	t4.Resume();
	t5.Resume();
	User::After(500000);
	TInt initc = count;
	User::After(5000000);
	TInt finalc = count;
	test.Printf(_L("%d iterations per second\n"), (finalc-initc)/5);
	t4.Kill(0);
	t5.Kill(0);
	CLOSE_AND_WAIT(t4);
	CLOSE_AND_WAIT(t5);
	}

void CreateThread2(RThread& aThread, SThreadData& aData, TThreadPriority aPri)
	{
	TInt r = aThread.Create(KNullDesC, &Thread2, 0x1000, 0x1000, 0x1000, &aData);
	test(r==KErrNone);
	aThread.SetPriority(aPri);
	TRequestStatus s;
	aThread.Rendezvous(s);
	test(s==KRequestPending);
	aThread.Resume();
	User::WaitForRequest(s);
	test(s==KErrNone);
	test(aThread.ExitType()==EExitPending);
	aData.iM.Wait();
	}

void KillThread2(RThread& aThread)
	{
	TRequestStatus s;
	aThread.Logon(s);
	test(s==KRequestPending);
	aThread.Terminate(0);
	User::WaitForRequest(s);
	test(aThread.ExitType()==EExitTerminate);
	test(aThread.ExitReason()==0);
	test(s==0);
	CLOSE_AND_WAIT(aThread);
	}

void AppendToArray(SThreadData& aD, TInt aCount, ...)
	{
	VA_LIST list;
	VA_START(list,aCount);
	aD.iM.Wait();
	while(--aCount>=0)
		{
		test(aD.iA->Append(VA_ARG(list,TInt))==KErrNone);
		}
	aD.iV.Signal();
	aD.iM.Signal();
	}

void AppendToArrayB(SThreadData& aD, TInt aCount, ...)
	{
	VA_LIST list;
	VA_START(list,aCount);
	aD.iM.Wait();
	while(--aCount>=0)
		{
		test(aD.iA->Append(VA_ARG(list,TInt))==KErrNone);
		}
	aD.iV.Broadcast();
	aD.iM.Signal();
	}

void AppendToArrayB2(SThreadData& aD, TInt aCount, ...)
	{
	VA_LIST list;
	VA_START(list,aCount);
	aD.iM.Wait();
	while(--aCount>=0)
		{
		test(aD.iA->Append(VA_ARG(list,TInt))==KErrNone);
		}
	aD.iM.Signal();
	aD.iV.Broadcast();
	}

void Thread2Test()
	{
	test.Next(_L("Thread2Test"));
	RCondVar cv2;
	RMutex m3;
	TInt r = cv2.CreateLocal();
	test(r==KErrNone);
	r = m3.CreateLocal();
	test(r==KErrNone);
	SThreadData d1;
	d1.iM = m3;
	d1.iV = cv2;
	RThread t1;

	CreateThread2(t1, d1, EPriorityLess);
	cv2.Signal();
	m3.Signal();
	User::After(100000);
	test(d1.iInnerLoops == 1);
	KillThread2(t1);

	CreateThread2(t1, d1, EPriorityLess);
	KillThread2(t1);
	m3.Signal();
	test(d1.iInnerLoops == 1);

	CreateThread2(t1, d1, EPriorityLess);
	m3.Signal();
	User::After(10000);
	KillThread2(t1);
	test(d1.iInnerLoops == 1);

	CreateThread2(t1, d1, EPriorityLess);
	cv2.Signal();
	User::After(10000);
	KillThread2(t1);
	m3.Signal();
	test(d1.iInnerLoops == 1);

	CreateThread2(t1, d1, EPriorityLess);
	t1.Suspend();
	KillThread2(t1);
	m3.Signal();
	test(d1.iInnerLoops == 1);

	CreateThread2(t1, d1, EPriorityLess);
	User::After(10000);
	t1.Suspend();
	KillThread2(t1);
	m3.Signal();
	test(d1.iInnerLoops == 1);

	CreateThread2(t1, d1, EPriorityLess);
	cv2.Signal();
	t1.Suspend();
	KillThread2(t1);
	m3.Signal();
	test(d1.iInnerLoops == 1);

	CreateThread2(t1, d1, EPriorityLess);
	cv2.Signal();
	User::After(10000);
	t1.Suspend();
	KillThread2(t1);
	m3.Signal();
	test(d1.iInnerLoops == 1);

	cv2.Close();
	m3.Close();
	}

const TText* KMutex1Name = _S("mtx1");
const TText* KMutex2Name = _S("mtx2");
const TText* KCondVar1Name = _S("cv1");
const TText* KCondVar2Name = _S("cv2");

void TestGlobal()
	{
	test.Next(_L("Test Global"));
	RMutex mg1, mg2;
	RCondVar cvg1, cvg2;
	TInt r = mg1.CreateGlobal(TPtrC(KMutex1Name));
	test(r==KErrNone);
	r = mg2.CreateGlobal(TPtrC(KMutex2Name));
	test(r==KErrNone);
	r = cvg1.CreateGlobal(TPtrC(KCondVar1Name));
	test(r==KErrNone);
	r = cvg2.CreateGlobal(TPtrC(KCondVar2Name));
	test(r==KErrNone);
	SThreadData2 d1, d2;
	d1.iMutexName = KMutex1Name;
	d1.iCondVarName = KCondVar1Name;
	d2.iMutexName = KMutex2Name;
	d2.iCondVarName = KCondVar2Name;

	RThread t1, t2;
	r = t1.Create(KNullDesC, &Thread3, 0x1000, 0x1000, 0x1000, &d1);
	test(r==KErrNone);
	t1.SetPriority(EPriorityMore);
	TRequestStatus s1;
	t1.Logon(s1);
	t1.Resume();
	r = t2.Create(KNullDesC, &Thread3, 0x1000, 0x1000, 0x1000, &d2);
	test(r==KErrNone);
	t2.SetPriority(EPriorityMore);
	TRequestStatus s2;
	t2.Logon(s2);
	t2.Resume();

	test(s1==KRequestPending);
	test(s2==KRequestPending);
	test(d1.iInnerLoops == 0);
	test(d2.iInnerLoops == 0);
	cvg1.Signal();
	test(d1.iInnerLoops == 1);
	test(d2.iInnerLoops == 0);
	cvg2.Signal();
	test(d1.iInnerLoops == 1);
	test(d2.iInnerLoops == 1);

	cvg1.Close();
	cvg2.Close();
	test(s1==KRequestPending);
	test(s2==KRequestPending);
	test(d1.iInnerLoops == 1);
	test(d2.iInnerLoops == 1);

	t1.Kill(0);
	t2.Kill(0);
	User::WaitForRequest(s1);
	User::WaitForRequest(s2);
	test(t1.ExitType()==EExitKill);
	test(t1.ExitReason()==0);
	test(t2.ExitType()==EExitKill);
	test(t2.ExitReason()==0);
	CLOSE_AND_WAIT(t1);
	CLOSE_AND_WAIT(t2);
	r = cvg1.OpenGlobal(TPtrC(KCondVar1Name));
	test(r==KErrNotFound);
	test(cvg1.Handle()==0);
	mg1.Close();
	mg2.Close();
	}

void TestSecondaryProcess()
	{
	test.Next(_L("Test Secondary Process"));

	RProcess p;
	RChunk c;
	RMutex m;
	RCondVar cv;

	//cancel lazy dll unloading
	RLoader loader;
	TInt r = loader.Connect();
	test(r==KErrNone);
	r = loader.CancelLazyDllUnload();
	test(r==KErrNone);
	loader.Close();

	r = c.CreateGlobal(KNullDesC, 0x1000, 0x1000);
	test(r==KErrNone);
	volatile TInt& x = *(volatile TInt*)c.Base();
	x = 0;
	r = m.CreateGlobal(KNullDesC);
	test(r==KErrNone);
	r = cv.CreateGlobal(KNullDesC);
	test(r==KErrNone);
	r = p.Create(RProcess().FileName(), KNullDesC);
	test(r==KErrNone);
	p.SetPriority(EPriorityHigh);
	r = p.SetParameter(1, cv);
	test(r==KErrNone);
	r = p.SetParameter(2, m);
	test(r==KErrNone);
	r = p.SetParameter(3, c);
	test(r==KErrNone);
	TRequestStatus s;
	p.Logon(s);
	p.Resume();
	test(s==KRequestPending);
	test(x==0);
	TInt i;
	for (i=0; i<10; ++i)
		{
		cv.Signal();
		test(x == i+1);
		}
	cv.Close();
	test(s==KRequestPending);
	test(x==10);
	p.Terminate(0);
	User::WaitForRequest(s);
	test(p.ExitType()==EExitTerminate);
	test(p.ExitReason()==0);
	CLOSE_AND_WAIT(p);
	m.Close();
	c.Close();
	}

TInt SecondaryProcess(RCondVar aCV)
	{
	RDebug::Print(_L("SecProc"));
	RMutex mp;
	RChunk cp;
	TInt r = mp.Open(2);
	if (r!=KErrNone)
		return r;
	r = cp.Open(3);
	if (r!=KErrNone)
		return r;
	volatile TInt& x = *(volatile TInt*)cp.Base();
	mp.Wait();
	r = KErrNone;
	while (r==KErrNone)
		{
		r = aCV.Wait(mp);
		++x;
		RDebug::Print(_L("SecProc r=%d x=%d"), r, x);
		}
	return r;
	}

TInt E32Main()
	{
	TInt cpus = UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0);
	if (cpus != 1)
		{
		test(cpus>1);
		// This test will require compatibility mode (and probably other changes)
		// to work on SMP - it depends on explicit scheduling order.
		test.Printf(_L("T_CONDVAR skipped, does not work on SMP\n"));
		return KErrNone;
		}	
	
	__KHEAP_MARK;
	__UHEAP_MARK;

	TInt r;
	RCondVar cvp;
	r = cvp.Open(1);
	if (r==KErrNone)
		return SecondaryProcess(cvp);
	test.Title();
	test.Start(_L("Create condition variable"));
	r = CV1.CreateLocal();
	test(r==KErrNone);
	r = CV2.CreateLocal();
	test(r==KErrNone);

	test.Next(_L("Signal with no-one waiting"));
	CV1.Signal();

	test.Next(_L("Broadcast with no-one waiting"));
	CV1.Broadcast();

	test.Next(_L("Create mutexes"));
	r = M1.CreateLocal();
	test(r==KErrNone);
	r = M2.CreateLocal();
	test(r==KErrNone);

	RArray<TInt> array;
	SThreadData d0;
	d0.iM = M2;
	d0.iV = CV1;
	d0.iA = &array;
	test.Next(_L("Create thread to use mutex 2"));
	RThread t0;
	r = t0.Create(KNullDesC, &Thread1, 0x1000, 0x1000, 0x1000, &d0);
	test(r==KErrNone);
	t0.SetPriority(EPriorityMore);
	TRequestStatus s0;
	t0.Logon(s0);
	t0.Resume();
	__TRACE_LINE__;
	AppendToArray(d0, 1, 4);
	test(d0.iTotal==4);
	__TRACE_LINE__;
	AppendToArray(d0, 2, -3, 17);
	test(d0.iTotal==18);
	t0.Terminate(11);
	User::WaitForRequest(s0);
	test(t0.ExitType()==EExitTerminate);
	test(t0.ExitReason()==11);
	CLOSE_AND_WAIT(t0);
	array.Reset();

	SThreadData d;
	d.iM = M1;
	d.iV = CV1;
	d.iA = &array;
	test.Next(_L("Create thread to use mutex 1"));
	RThread t;
	r = t.Create(KNullDesC, &Thread1, 0x1000, 0x1000, 0x1000, &d);
	test(r==KErrNone);
	t.SetPriority(EPriorityMore);
	TRequestStatus s;
	t.Logon(s);
	t.Resume();

	test.Next(_L("Test wait with mutex unlocked"));
	r = t0.Create(KNullDesC, &Thread0, 0x1000, 0x1000, 0x1000, NULL);
	test(r==KErrNone);
	t0.SetPriority(EPriorityMore);
	t0.Logon(s0);
	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	t0.Resume();
	User::WaitForRequest(s0);
	User::SetJustInTime(jit);
	test(t0.ExitType()==EExitPanic);
	test(t0.ExitCategory()==_L("KERN-EXEC"));
	test(t0.ExitReason()==ECondVarWaitMutexNotLocked);
	CLOSE_AND_WAIT(t0);

	test.Next(_L("Test trying to use two mutexes with 1 condition variable"));
	M2.Wait();
	r = CV1.Wait(M2);
	M2.Signal();
	test(r==KErrInUse);

	test(d.iTotal==0);
	__TRACE_LINE__;
	AppendToArray(d, 1, 3);
	test(d.iTotal==3);
	__TRACE_LINE__;
	AppendToArray(d, 2, 3, 19);
	test(d.iTotal==25);
	__TRACE_LINE__;
	AppendToArray(d, 4, 15, -1, -2, -30);
	test(d.iTotal==7);
	test(d.iInnerLoops==3);
	test(d.iOuterLoops==3);
	__TRACE_LINE__;
	t.Suspend();
	__TRACE_LINE__;
	t.Resume();
	test(d.iTotal==7);
	test(d.iInnerLoops==4);
	test(d.iOuterLoops==3);
	__TRACE_LINE__;
	t.SetPriority(EPriorityLess);
	test(d.iTotal==7);
	test(d.iInnerLoops==4);
	test(d.iOuterLoops==3);
	__TRACE_LINE__;
	t.SetPriority(EPriorityMore);
	test(d.iTotal==7);
	test(d.iInnerLoops==5);
	test(d.iOuterLoops==3);
	__TRACE_LINE__;
	t.Suspend();
	__TRACE_LINE__;
	AppendToArray(d, 1, 4);
	test(d.iTotal==7);
	test(d.iInnerLoops==5);
	test(d.iOuterLoops==3);
	__TRACE_LINE__;
	t.Resume();
	test(d.iTotal==11);
	test(d.iInnerLoops==6);
	test(d.iOuterLoops==4);

	SThreadData d2;
	d2.iM = M1;
	d2.iV = CV1;
	d2.iA = &array;

	test.Next(_L("Create 2nd thread"));
	RThread t2;
	r = t2.Create(KNullDesC, &Thread1, 0x1000, NULL, &d2);
	test(r==KErrNone);
	t2.SetPriority(EPriorityMuchMore);
	TRequestStatus s2;
	t2.Logon(s2);
	__TRACE_LINE__;
	t2.Resume();

	test(d2.iTotal == 11);
	test(d2.iInnerLoops == 0);
	test(d2.iOuterLoops == 1);
	__TRACE_LINE__;
	AppendToArray(d, 2, 9, 10);
	test(d2.iTotal == 30);
	test(d2.iInnerLoops == 1);
	test(d2.iOuterLoops == 2);
	test(d.iTotal==11);
	test(d.iInnerLoops==6);
	test(d.iOuterLoops==4);
	__TRACE_LINE__;
	AppendToArrayB(d, 2, 20, 30);
	test(d2.iTotal == 80);
	test(d2.iInnerLoops == 2);
	test(d2.iOuterLoops == 3);
	test(d.iTotal == 80);
	test(d.iInnerLoops == 7);
	test(d.iOuterLoops == 5);
	__TRACE_LINE__;
	AppendToArrayB2(d, 2, -10, -6);
	test(d2.iTotal == 64);
	test(d2.iInnerLoops == 3);
	test(d2.iOuterLoops == 4);
	test(d.iTotal == 64);
	test(d.iInnerLoops == 8);
	test(d.iOuterLoops == 6);
	__TRACE_LINE__;
	t2.Suspend();
	__TRACE_LINE__;
	AppendToArray(d, 2, -8, -8);
	test(d2.iTotal == 64);
	test(d2.iInnerLoops == 3);
	test(d2.iOuterLoops == 4);
	test(d.iTotal == 48);
	test(d.iInnerLoops == 9);
	test(d.iOuterLoops == 7);
	__TRACE_LINE__;
	t2.Resume();
	test(d2.iTotal == 48);
	test(d2.iInnerLoops == 4);
	test(d2.iOuterLoops == 5);
	test(d.iTotal == 48);
	test(d.iInnerLoops == 9);
	test(d.iOuterLoops == 7);

	// test timeouts
	d.iTimeoutMs = 1000;
	__TRACE_LINE__;
	t.Suspend();
	__TRACE_LINE__;
	t.Resume();
	test(d2.iTotal == 48);
	test(d2.iInnerLoops == 4);
	test(d2.iOuterLoops == 5);
	test(d2.iTimeouts == 0);
	test(d.iTotal == 48);
	test(d.iInnerLoops == 10);
	test(d.iOuterLoops == 7);
	test(d.iTimeouts == 0);
	test(array.Append(1)==0);
	TInt nt = 0;
	do	{
		if (d.iTimeouts > nt)
			{
			test(d.iTimeouts-nt == 1);
			nt = d.iTimeouts;
			test.Printf(_L("Timeout %d\n"), nt);
			test(d2.iTotal == 48);
			test(d2.iInnerLoops == 4);
			test(d2.iOuterLoops == 5);
			test(d2.iTimeouts == 0);
			test(d.iTotal == 48+nt);
			test(d.iInnerLoops == 10+nt);
			test(d.iOuterLoops == 7+nt);
			test(array.Append(1)==0);
			}
		} while (nt<10);

	d.iTimeoutMs = 0;
	AppendToArrayB(d, 0);
	test(d2.iTotal == 59);
	test(d2.iInnerLoops == 5);
	test(d2.iOuterLoops == 6);
	test(d2.iTimeouts == 0);
	test(d.iTotal == 59);
	test(d.iInnerLoops == 21);
	test(d.iOuterLoops == 18);
	test(d.iTimeouts == 10);

	__TRACE_LINE__;
	t.SetPriority(EPriorityLess);
	__TRACE_LINE__;
	AppendToArrayB(d, 1, 11);
	test(d2.iTotal == 70);
	test(d2.iInnerLoops == 6);
	test(d2.iOuterLoops == 7);
	test(d2.iTimeouts == 0);
	test(d.iTotal == 59);
	test(d.iInnerLoops == 21);
	test(d.iOuterLoops == 18);
	test(d.iTimeouts == 10);
	User::After(50000);
	test(d2.iTotal == 70);
	test(d2.iInnerLoops == 6);
	test(d2.iOuterLoops == 7);
	test(d2.iTimeouts == 0);
	test(d.iTotal == 70);
	test(d.iInnerLoops == 22);
	test(d.iOuterLoops == 19);
	test(d.iTimeouts == 10);



	__TRACE_LINE__;
	CV1.Close();
	User::WaitForRequest(s);
	test(t.ExitType()==EExitKill);
	test(t.ExitReason()==KErrGeneral);
	User::WaitForRequest(s2);
	test(t2.ExitType()==EExitKill);
	test(t2.ExitReason()==KErrGeneral);
	CLOSE_AND_WAIT(t);
	CLOSE_AND_WAIT(t2);


	M1.Close();

	TestGlobal();

	Thread2Test();

	TestSecondaryProcess();

	RunBench();
	M2.Close();
	CV2.Close();
	array.Close();

	test.End();
	test.Close();

	__UHEAP_MARKEND;
	__KHEAP_MARKEND;
	return KErrNone;
	}

