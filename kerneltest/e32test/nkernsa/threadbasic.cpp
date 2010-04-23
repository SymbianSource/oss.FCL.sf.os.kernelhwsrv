// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\nkernsa\threadbasic.cpp
// 
//

#include <nktest/nkutils.h>

#define SLEEP_TIME 1

#ifndef __SMP__
#define iNThreadBaseSpare7 iSpare7
#endif

struct SThreadInfo1
	{
	volatile TInt iRunCount;
	volatile TInt iBlockEvery;
	volatile TBool iStop;
	CircBuf* iBuf;
	NThread* iThread;
	};

TInt WaitForRun(SThreadInfo1& aI, TInt aCount)
	{
	TUint32 initial = NKern::TickCount();
	TUint32 final = initial + 2;
	FOREVER
		{
		if (aI.iRunCount >= aCount)
			return aI.iRunCount;
		TUint32 x = NKern::TickCount();
		if ((x - final) < 0x80000000u)
			return KErrTimedOut;
		}
	}

void BasicThread(TAny* a)
	{
	SThreadInfo1& info = *(SThreadInfo1*)a;
	NThread* pC = NKern::CurrentThread();

	while (!info.iStop)
		{
		TInt r = info.iBuf->TryPut((TUint32)pC);
		TEST_RESULT(r==KErrNone, "Buffer full");
		TInt c = (TInt)__e32_atomic_add_ord32(&info.iRunCount, 1);
		TInt m = (c+1)%info.iBlockEvery;
		if (!m)
			NKern::WaitForAnyRequest();
		}
	}

void BasicThread0(TAny*)
	{
	NThread* pC = NKern::CurrentThread();
	TInt my_priority = pC->i_NThread_BasePri;
	TInt this_cpu = NKern::CurrentCpu();
	CircBuf* buf = CircBuf::New(KNumPriorities * KMaxCpus * 8);
	TEST_OOM(buf);
	SThreadInfo1* pI = (SThreadInfo1*)malloc(KNumPriorities * KMaxCpus * sizeof(SThreadInfo1));
	TEST_OOM(pI);
	memclr(pI, KNumPriorities * KMaxCpus * sizeof(SThreadInfo1));
	NFastSemaphore exitSem(0);

	TInt pri;
	TInt cpu;
	
	for_each_cpu(cpu)
		{
		for (pri = 1; pri < KNumPriorities; ++pri)
			{
			TInt ix = cpu * KNumPriorities + pri;
			SThreadInfo1& info = pI[ix];
			info.iBlockEvery = 1;
			info.iBuf = buf;
			info.iThread = CreateUnresumedThreadSignalOnExit("Basic", &BasicThread, pri, &info, 0, -1, &exitSem, cpu);
			TEST_OOM(info.iThread);
			}
		}
	TInt c = buf->Count();
	TEST_RESULT1(c==0, "Unexpected count %d", c);	// nothing resumed yet
	for_each_cpu(cpu)
		{
		for (pri = 1; pri < KNumPriorities; ++pri)
			{
			TInt ix = cpu * KNumPriorities + pri;
			SThreadInfo1& info = pI[ix];
			NKern::ThreadResume(info.iThread);
			TInt r = WaitForRun(info, 1);
			if (pri>my_priority || cpu!=this_cpu)
				{
				TEST_RESULT(r==1, "WaitForRun");
				c = buf->Count();
				TEST_RESULT1(c==1, "Unexpected count %d", c);	// thread should have run
				TUint32 x = buf->Get();
				c = buf->Count();
				TEST_RESULT1(c==0, "Unexpected count %d", c);
				TEST_RESULT(x==(TUint32)info.iThread, "Wrong thread");
				}
			else
				{
				TEST_RESULT(r==KErrTimedOut, "WaitForRun");
				c = buf->Count();
				TEST_RESULT1(c==0, "Unexpected count %d", c);	// thread won't have run since current has priority
				}
			}
		}
	NKern::Sleep(10);	// let lower priority threads run
	c = buf->Count();
	TEST_RESULT1(c==my_priority, "Unexpected count %d", c);
	for (pri = my_priority; pri >= 1; --pri)
		{
		TInt ix = this_cpu * KNumPriorities + pri;
		SThreadInfo1& info = pI[ix];
		TEST_RESULT(info.iRunCount==1, "Bad run count");
		TUint32 x = buf->Get();
		TEST_RESULT(x==(TUint32)info.iThread, "Wrong thread");
		}
	for_each_cpu(cpu)
		{
		for (pri = 1; pri < KNumPriorities; ++pri)
			{
			TInt ix = cpu * KNumPriorities + pri;
			SThreadInfo1& info = pI[ix];
			info.iStop = TRUE;
			NKern::ThreadRequestSignal(info.iThread);
			NKern::FSWait(&exitSem);
			}
		}
	free(pI);
	delete buf;
	}

void BasicThreadTest1()
	{
	TEST_PRINT("Testing all thread priorities without timeslice");

	TInt pri;
	TInt cpu;
	
	for_each_cpu(cpu)
		{
		for (pri = 1; pri < KNumPriorities; ++pri)
			{
			TEST_PRINT2("Basic0 pri %d cpu %d", pri, cpu);
			CreateThreadAndWaitForExit("Basic0", &BasicThread0, pri, 0, 0, -1, cpu);
			}
		}
	}

void Spinner(TAny*)
	{
	FOREVER
		{
		}
	}

void BasicThreadTest2()
	{
	TEST_PRINT("Kill an unresumed thread");
	NFastSemaphore exitSem(0);

	TInt cpu;
	for_each_cpu(cpu)
		{
		TEST_PRINT1("Thread on CPU %d", cpu);
		NThread* t = CreateUnresumedThreadSignalOnExit("Spinner", &Spinner, 33, 0, 0, -1, &exitSem, cpu);
		TEST_OOM(t);
		NKern::ThreadKill(t);
		NKern::FSWait(&exitSem);
		TEST_PRINT("OK");
		}

	}

void TimesliceTestThread(TAny* a)
	{
	NThread* pC = NKern::CurrentThread();
	TUint id = pC->iNThreadBaseSpare7;
	CircBuf* buf = (CircBuf*)a;
	TUint32 thresh = norm_fast_counter_freq();
	TUint32 thresh2 = thresh;
	thresh /= 3000;
	if (thresh < 10)
		thresh = 10;
	TUint32 last_interval_begin = norm_fast_counter();
	TUint32 last_seen_time = norm_fast_counter();
	FOREVER
		{
		TUint32 nfc = norm_fast_counter();
		TUint32 delta = nfc - last_seen_time;
		TUint32 interval_length = last_seen_time - last_interval_begin;
		if (delta > thresh || interval_length > thresh2)
			{
			last_interval_begin = nfc;
			TUint32 x = (id<<24) | interval_length;
			TInt r = buf->TryPut(x);
			if (r != KErrNone)
				break;
			}
		last_seen_time = nfc;
		}
	}

void TimesliceTest()
	{
//	NThread* pC = NKern::CurrentThread();
//	TInt my_priority = pC->i_NThread_BasePri;
//	TInt this_cpu = NKern::CurrentCpu();
	CircBuf* buf = CircBuf::New(1024);
	TEST_OOM(buf);
	NFastSemaphore exitSem(0);

	TInt cpu;
	TInt i;
	TInt id = 0;
	NThread* t[KMaxCpus*3];
	TInt timeslice[3] =
		{
		__microseconds_to_timeslice_ticks(20000),
		__microseconds_to_timeslice_ticks(23000),
		__microseconds_to_timeslice_ticks(19000)
		};
	TInt expected[3] =
		{
		__microseconds_to_norm_fast_counter(20000),
		__microseconds_to_norm_fast_counter(23000),
		__microseconds_to_norm_fast_counter(19000)
		};
	for_each_cpu(cpu)
		{
		for (i=0; i<3; ++i)
			{
			t[id] = CreateThreadSignalOnExit("Timeslice", &TimesliceTestThread, 10, buf, 0, timeslice[i], &exitSem, cpu);
			TEST_OOM(t[id]);
			t[id]->iNThreadBaseSpare7 = id;
			++id;
			}
		nfcfspin(__microseconds_to_norm_fast_counter(1000));
		}
	for (i=0; i<id; ++i)
		{
		NKern::FSWait(&exitSem);
		TEST_PRINT("Thread exited");
		}
	TUint32 x;
	TUint32 xtype = 0;
	TUint32 ncpus = NKern::NumberOfCpus();
	TUint32 xcpu = (ncpus>1) ? 1 : 0;
	while (buf->TryGet(x)==KErrNone)
		{
		TUint32 id = x>>24;
		TUint32 time = x&0xffffff;
		TEST_PRINT2("Id %d Time %d", id, time);
		TUint32 xid = xcpu*3 + xtype;
		if (xcpu==0 && ++xtype==3)
			xtype=0;
		if (++xcpu == ncpus)
			xcpu=0;
		TEST_RESULT2(id==xid, "Expected id %d got id %d", xid, id);
		TUint32 exp = expected[id%3];
		TUint32 tol = exp/100;
		if (tol < 2)
			tol = 2;
		TUint32 diff = (time > exp) ? time - exp : exp - time;
		TEST_RESULT2(diff < tol, "Out of Tolerance: exp %d got %d", exp, time);
		}
	delete buf;
	}

struct SThreadInfo2
	{
	enum {ENumTimes=8};
	TInt Add(TUint32 aTime, TUint32 aId);

	NFastMutex* iMutex;
	TInt iSpin1;
	TInt iSpin2;
	TInt iSpin3;
	NThread* iThread2;
	volatile TInt iCount;
	volatile TUint32 iId[ENumTimes];
	volatile TUint32 iTime[ENumTimes];
	};

TInt SThreadInfo2::Add(TUint32 aTime, TUint32 aId)
	{
	TInt c = __e32_atomic_tas_ord32(&iCount, ENumTimes, 0, 1);
	if (c>=ENumTimes)
		return KErrOverflow;
	iTime[c] = aTime;
	iId[c] = aId;
	return KErrNone;
	}

/*
If Thread1 and Thread2 on different CPUs:
	Point0
	PointA	just after Point0
	PointB	PointA + spin1
	PointE	PointA + spin1
	PointC	PointB + spin2
	PointD	PointB + spin2
	PointF	PointE + spin3

If Thread1 and Thread2 on same CPU, no mutex:
	Point0
	PointA	just after Point0
	PointB	PointA + spin1 or PointA + spin1 + timeslice if spin1>=timeslice
	PointE	PointA + spin1 or PointA + timeslice whichever is later

If Thread1 and Thread2 on same CPU, mutex:
	Point0
	PointA	just after Point0
	PointB	PointA + spin1
	PointC	PointB + spin2
	PointE	PointA + spin1 +spin2 or PointA + timeslice whichever is later
	PointD	PointA + spin1 + spin2 if (spin1+spin2)<timeslice, otherwise PointA + spin1 + spin2 + timeslice
*/

void TimesliceTest2Thread1(TAny* a)
	{
	SThreadInfo2& info = *(SThreadInfo2*)a;
	TEST_RESULT(info.Add(norm_fast_counter(),1)==KErrNone, "Add failed");		// Point A
	if (info.iMutex)
		NKern::FMWait(info.iMutex);
	nfcfspin(info.iSpin1);
	NKern::ThreadResume(info.iThread2);
	TEST_RESULT(info.Add(norm_fast_counter(),1)==KErrNone, "Add failed");		// Point B
	nfcfspin(info.iSpin2);
	TEST_RESULT(info.Add(norm_fast_counter(),1)==KErrNone, "Add failed");		// Point C
	if (info.iMutex)
		NKern::FMSignal(info.iMutex);
	TEST_RESULT(info.Add(norm_fast_counter(),1)==KErrNone, "Add failed");		// Point D
	nfcfspin(__microseconds_to_norm_fast_counter(100000));
	}

void TimesliceTest2Thread2(TAny* a)
	{
	SThreadInfo2& info = *(SThreadInfo2*)a;
	TEST_RESULT(info.Add(norm_fast_counter(),2)==KErrNone, "Add failed");		// Point E
	nfcfspin(info.iSpin3);
	TEST_RESULT(info.Add(norm_fast_counter(),2)==KErrNone, "Add failed");		// Point F
	nfcfspin(__microseconds_to_norm_fast_counter(100000));
	}

void DoTimesliceTest2(TInt aCpu, TInt aSpin1, TInt aSpin2, TInt aSpin3, TBool aUseMutex)
	{
	TEST_PRINT5("TT2: C=%1d S1=%d S2=%d S3=%d M=%1d", aCpu, aSpin1, aSpin2, aSpin3, aUseMutex);

	TInt this_cpu = NKern::CurrentCpu();
	NFastSemaphore exitSem(0);
	NFastMutex mutex;
	SThreadInfo2 info;

	info.iMutex = aUseMutex ? &mutex : 0;
	info.iSpin1 = aSpin1;
	info.iSpin2 = aSpin2;
	info.iSpin3 = aSpin3;
	info.iCount = 0;

	TInt timeslice = __microseconds_to_timeslice_ticks(5000);
	NThread* t1 = CreateUnresumedThreadSignalOnExit("Thread1", &TimesliceTest2Thread1, 10, &info, 0, timeslice, &exitSem, this_cpu);
	TEST_OOM(t1);
	info.iThread2 = CreateUnresumedThreadSignalOnExit("Thread2", &TimesliceTest2Thread2, 10, &info, 0, timeslice, &exitSem, aCpu);
	TEST_OOM(info.iThread2);
	NKern::ThreadResume(t1);
	TEST_RESULT(info.Add(norm_fast_counter(),0)==KErrNone, "Add failed");	// Point 0
	NKern::FSWait(&exitSem);
	NKern::FSWait(&exitSem);
	TEST_RESULT1(info.iCount==7, "Wrong count %d", info.iCount);
	TInt i;
	TUint32 pointA=0, pointB=0, pointC=0, pointD=0, pointE=0, pointF=0;
	TInt n1=0, n2=0;
	TUint32 delta = __microseconds_to_norm_fast_counter(100);
	TUint32 ts = __microseconds_to_norm_fast_counter(5000);
	for (i=0; i<info.iCount; ++i)
		{
		if (i>0)
			{
			TUint32 id = info.iId[i];
			TUint32 x = info.iTime[i] - info.iTime[0];
			TEST_PRINT2("%d: %d", id, x);
			if (id==1)
				{
				switch(++n1)
					{
					case 1: pointA = x; break;
					case 2: pointB = x; break;
					case 3: pointC = x; break;
					case 4: pointD = x; break;
					}
				}
			else
				{
				switch(++n2)
					{
					case 1: pointE = x; break;
					case 2: pointF = x; break;
					}
				}
			}
		}
	TEST_RESULT(RANGE_LQ(pointA, delta), "pointA");
	if (aCpu != this_cpu)
		{
		TEST_RESULT(RANGE_CHECK(TUint32(aSpin1), pointB, TUint32(aSpin1)+delta), "pointB");
		TEST_RESULT(RANGE_CHECK(TUint32(aSpin1), pointE, TUint32(aSpin1)+delta), "pointE");
		TEST_RESULT(RANGE_CHECK(pointB+aSpin2, pointC, pointB+aSpin2+delta), "pointC");
		TEST_RESULT(RANGE_CHECK(pointB+aSpin2, pointD, pointB+aSpin2+delta), "pointD");
		TEST_RESULT(RANGE_CHECK(pointE+aSpin3, pointF, pointE+aSpin3+delta), "pointF");
		}
	else if (aUseMutex)
		{
		TEST_RESULT(RANGE_CHECK(TUint32(aSpin1), pointB, aSpin1+delta), "pointB");
		TEST_RESULT(RANGE_CHECK(pointB+aSpin2, pointC, pointB+aSpin2+delta), "pointC");

		TUint32 xpe = aSpin1 + aSpin2;
		TUint32 xpd = xpe;
		if (xpe < ts)
			xpe = ts;
		else
			xpd += ts;

		TEST_RESULT(RANGE_CHECK(xpe, pointE, xpe+delta), "pointE");
		TEST_RESULT(RANGE_CHECK(xpd, pointD, xpd+delta), "pointD");
		}
	else
		{
		TUint32 xpb = aSpin1;
		TUint32 xpe = aSpin1;
		if (xpb >= ts)
			xpb += ts;
		else
			xpe = ts;
		TEST_RESULT(RANGE_CHECK(xpb, pointB, xpb+delta), "pointB");
		TEST_RESULT(RANGE_CHECK(xpe, pointE, xpe+delta), "pointE");
		}
	}

void TimesliceTest2()
	{
	TInt cpu;
	TInt ms = __microseconds_to_norm_fast_counter(1000);
	for_each_cpu(cpu)
		{
		DoTimesliceTest2(cpu, 1*ms, 10*ms, 10*ms, FALSE);
		DoTimesliceTest2(cpu, 2*ms, 10*ms, 10*ms, FALSE);
		DoTimesliceTest2(cpu, 7*ms, 20*ms, 20*ms, FALSE);
		DoTimesliceTest2(cpu, 1*ms, 1*ms, 10*ms, TRUE);
		DoTimesliceTest2(cpu, 1*ms, 2*ms, 10*ms, TRUE);
		DoTimesliceTest2(cpu, 2*ms, 2*ms, 10*ms, TRUE);
		DoTimesliceTest2(cpu, 7*ms, 7*ms, 10*ms, TRUE);
		DoTimesliceTest2(cpu, 7*ms, 7*ms, 50*ms, TRUE);
		}
	}

struct SThreadInfo3
	{
	enum TTestType
		{
		ESpin,
		ECount,
		EWaitFS,
		EWaitFM,
		EExit,
		EHoldFM,
		};

	TTestType iType;
	TAny* iObj;
	TInt iPri;
	TInt iCpu;
	volatile TInt iCount;
	volatile TInt iCurrCpu;
	volatile TBool iStop;
	NFastSemaphore* iExitSem;
	TInt iExitCpu;

	void Set(TTestType aType, TAny* aObj, TInt aPri, TInt aCpu)
		{iType=aType; iObj=aObj; iPri=aPri; iCpu=aCpu; iCount=0; iCurrCpu=-1; iStop=FALSE; iExitSem=0; iExitCpu=-1;}
	NThread* CreateThread(const char* aName, NFastSemaphore* aExitSem);
	static void ExitHandler(TAny* aP, NThread* aT, TInt aC);
	};

void BasicThread3(TAny* a)
	{
	SThreadInfo3& info = *(SThreadInfo3*)a;

	switch (info.iType)
		{
		case SThreadInfo3::ESpin:
			FOREVER
				{
				info.iCurrCpu = NKern::CurrentCpu();
				}

		case SThreadInfo3::ECount:
			FOREVER
				{
				info.iCurrCpu = NKern::CurrentCpu();
				__e32_atomic_add_ord32(&info.iCount, 1);
				}

		case SThreadInfo3::EWaitFS:
			NKern::FSSetOwner((NFastSemaphore*)info.iObj, 0);
			NKern::FSWait((NFastSemaphore*)info.iObj);
			break;

		case SThreadInfo3::EWaitFM:
			NKern::FMWait((NFastMutex*)info.iObj);
			NKern::FMSignal((NFastMutex*)info.iObj);
			break;

		case SThreadInfo3::EExit:
			break;

		case SThreadInfo3::EHoldFM:
			NKern::FMWait((NFastMutex*)info.iObj);
			while (!info.iStop)
				{
				info.iCurrCpu = NKern::CurrentCpu();
				__e32_atomic_add_ord32(&info.iCount, 1);
				}
			NKern::FMSignal((NFastMutex*)info.iObj);
			break;
		}
	}

void SThreadInfo3::ExitHandler(TAny* aP, NThread* aT, TInt aC)
	{
	SThreadInfo3& info = *(SThreadInfo3*)aP;
	switch (aC)
		{
		case EInContext:
			info.iExitCpu = NKern::CurrentCpu();
			break;
		case EBeforeFree:
			{
			NKern::ThreadSuspend(aT, 1);
			NKern::ThreadResume(aT);
			NKern::ThreadResume(aT);
			NKern::ThreadSuspend(aT, 1);
			NKern::ThreadSuspend(aT, 1);
			NKern::ThreadSuspend(aT, 1);
			NKern::ThreadResume(aT);
			NKern::ThreadForceResume(aT);
			NKern::ThreadKill(aT);
			NKern::ThreadSetPriority(aT, 63);
			TEST_RESULT(aT->iPriority == 63, "Priority change when dead");
			TUint32 aff = NKern::ThreadSetCpuAffinity(aT, 0xffffffffu);
			TEST_RESULT(aff==TUint32(info.iExitCpu), "CPU affinity when dead");
			aff = NKern::ThreadSetCpuAffinity(aT, info.iExitCpu);
			TEST_RESULT(aff==0xffffffffu, "CPU affinity when dead");
			break;
			}
		case EAfterFree:
			NKern::FSSignal(info.iExitSem);
			break;
		}
	}

NThread* SThreadInfo3::CreateThread(const char* aName, NFastSemaphore* aExitSem)
	{
	iExitSem = aExitSem;
	iExitCpu = -1;
	NThread* t = ::CreateThread(aName, &BasicThread3, iPri, this, 0, FALSE, -1, &SThreadInfo3::ExitHandler, this, iCpu);
	TEST_OOM(t);
	return t;
	}

#define CHECK_RUNNING(info, cpu)	\
	do {TInt c1 = (info).iCount; NKern::Sleep(SLEEP_TIME); TEST_RESULT((info).iCount!=c1, "Not running"); TEST_RESULT((info).iCurrCpu==(cpu), "Wrong CPU"); } while(0)

#define CHECK_NOT_RUNNING(info, same_cpu)	\
do {if (!same_cpu) NKern::Sleep(SLEEP_TIME); TInt c1 = (info).iCount; NKern::Sleep(SLEEP_TIME); TEST_RESULT((info).iCount==c1, "Running"); } while(0)

void DoBasicThreadTest3SemMutex(TInt aCpu, TInt aCpu2, TBool aMutex)
	{
	SThreadInfo3 info;
	NThread* t;
	NFastSemaphore xs(0);
	NFastSemaphore s;
	NFastMutex m;

	if (aMutex)
		{
		TEST_PRINT("Operations while blocked on mutex");
		}
	else
		{
		TEST_PRINT("Operations while blocked on semaphore");
		}

	SThreadInfo3::TTestType type = aMutex ? SThreadInfo3::EWaitFM : SThreadInfo3::EWaitFS;
	TAny* obj = aMutex ? (TAny*)&m : (TAny*)&s;

	info.Set(type, obj, 63, aCpu);
	t = info.CreateThread("Single2", &xs);
	if (!aMutex)
		TEST_RESULT(s.iCount==0, "Sem count");
	if (aMutex)
		NKern::FMWait(&m);
	NKern::ThreadResume(t);			// resume thread - should wait on semaphore/mutex
	NKern::Sleep(SLEEP_TIME);
	if (!aMutex)
		TEST_RESULT(s.iCount<0, "Sem count");
	TEST_RESULT(info.iExitCpu==-1, "Exit CPU");

	aMutex ? NKern::FMSignal(&m) : NKern::FSSignal(&s);	// signal semaphore/mutex - thread should exit
	NKern::FSWait(&xs);
	if (!aMutex)
		TEST_RESULT(s.iCount==0, "Sem count");
	TEST_RESULT(info.iExitCpu==aCpu, "Exit CPU");

	info.Set(type, obj, 63, aCpu);
	t = info.CreateThread("Single3", &xs);
	if (!aMutex)
		TEST_RESULT(s.iCount==0, "Sem count");
	if (aMutex)
		NKern::FMWait(&m);
	NKern::ThreadResume(t);			// resume thread - should wait on semaphore/mutex
	NKern::Sleep(SLEEP_TIME);
	if (!aMutex)
		TEST_RESULT(s.iCount<0, "Sem count");
	TEST_RESULT(info.iExitCpu==-1, "Exit CPU");
	NKern::ThreadSuspend(t, 1);		// suspend thread while waiting on semaphore/mutex
	NKern::Sleep(SLEEP_TIME);
	if (!aMutex)
		TEST_RESULT(s.iCount<0, "Sem count");
	TEST_RESULT(info.iExitCpu==-1, "Exit CPU");
	aMutex ? NKern::FMSignal(&m) : NKern::FSSignal(&s);	// signal semaphore/mutex - still suspended
	NKern::Sleep(SLEEP_TIME);
	if (!aMutex)
		TEST_RESULT(s.iCount==0, "Sem count");
	TEST_RESULT(info.iExitCpu==-1, "Exit CPU");
	NKern::ThreadResume(t);			// resume - should now exit
	NKern::FSWait(&xs);
	if (!aMutex)
		TEST_RESULT(s.iCount==0, "Sem count");
	TEST_RESULT(info.iExitCpu==aCpu, "Exit CPU");

	info.Set(type, obj, 63, aCpu);
	t = info.CreateThread("Single4", &xs);
	if (!aMutex)
		TEST_RESULT(s.iCount==0, "Sem count");
	if (aMutex)
		NKern::FMWait(&m);
	NKern::ThreadResume(t);			// resume thread - should wait on semaphore/mutex
	NKern::Sleep(SLEEP_TIME);
	if (!aMutex)
		TEST_RESULT(s.iCount<0, "Sem count");
	TEST_RESULT(info.iExitCpu==-1, "Exit CPU");
	NKern::ThreadKill(t);			// kill thread while blocked on semaphore/mutex
	NKern::FSWait(&xs);
	if (!aMutex)
		TEST_RESULT(s.iCount==0, "Sem count");
	TEST_RESULT(info.iExitCpu==aCpu, "Exit CPU");
	if (aMutex)
		NKern::FMSignal(&m);

	info.Set(type, obj, 63, aCpu);
	t = info.CreateThread("Single5", &xs);
	if (!aMutex)
		TEST_RESULT(s.iCount==0, "Sem count");
	if (aMutex)
		NKern::FMWait(&m);
	NKern::ThreadResume(t);			// resume thread - should wait on semaphore/mutex
	NKern::Sleep(SLEEP_TIME);
	if (!aMutex)
		TEST_RESULT(s.iCount<0, "Sem count");
	TEST_RESULT(info.iExitCpu==-1, "Exit CPU");
	NKern::ThreadSuspend(t, 1);		// suspend thread while waiting on semaphore/mutex
	NKern::Sleep(SLEEP_TIME);
	if (!aMutex)
		TEST_RESULT(s.iCount<0, "Sem count");
	TEST_RESULT(info.iExitCpu==-1, "Exit CPU");
	NKern::ThreadKill(t);			// kill thread while blocked on semaphore/mutex and suspended
	NKern::FSWait(&xs);
	if (!aMutex)
		TEST_RESULT(s.iCount==0, "Sem count");
	TEST_RESULT(info.iExitCpu==aCpu, "Exit CPU");
	if (aMutex)
		NKern::FMSignal(&m);

	if (aCpu2>=0)
		{
		info.Set(type, obj, 63, aCpu);
		t = info.CreateThread("Single6", &xs);
		if (!aMutex)
			TEST_RESULT(s.iCount==0, "Sem count");
		if (aMutex)
			NKern::FMWait(&m);
		NKern::ThreadResume(t);			// resume thread - should wait on semaphore/mutex
		NKern::Sleep(SLEEP_TIME);
		if (!aMutex)
			TEST_RESULT(s.iCount<0, "Sem count");
		TEST_RESULT(info.iExitCpu==-1, "Exit CPU");
		NKern::ThreadSetCpuAffinity(t, aCpu2);	// move blocked thread
		aMutex ? NKern::FMSignal(&m) : NKern::FSSignal(&s);	// signal semaphore/mutex - thread should exit
		NKern::FSWait(&xs);
		if (!aMutex)
			TEST_RESULT(s.iCount==0, "Sem count");
		TEST_RESULT(info.iExitCpu==aCpu2, "Exit CPU");

		info.Set(type, obj, 63, aCpu);
		t = info.CreateThread("Single3", &xs);
		if (!aMutex)
			TEST_RESULT(s.iCount==0, "Sem count");
		if (aMutex)
			NKern::FMWait(&m);
		NKern::ThreadResume(t);			// resume thread - should wait on semaphore/mutex
		NKern::Sleep(SLEEP_TIME);
		if (!aMutex)
			TEST_RESULT(s.iCount<0, "Sem count");
		TEST_RESULT(info.iExitCpu==-1, "Exit CPU");
		NKern::ThreadSuspend(t, 1);		// suspend thread while waiting on semaphore/mutex
		NKern::Sleep(SLEEP_TIME);
		if (!aMutex)
			TEST_RESULT(s.iCount<0, "Sem count");
		TEST_RESULT(info.iExitCpu==-1, "Exit CPU");
		NKern::ThreadSetCpuAffinity(t, aCpu2);	// move blocked and suspended thread
		aMutex ? NKern::FMSignal(&m) : NKern::FSSignal(&s);	// signal semaphore/mutex - still suspended
		NKern::Sleep(SLEEP_TIME);
		if (!aMutex)
			TEST_RESULT(s.iCount==0, "Sem count");
		TEST_RESULT(info.iExitCpu==-1, "Exit CPU");
		NKern::ThreadResume(t);			// resume - should now exit
		NKern::FSWait(&xs);
		if (!aMutex)
			TEST_RESULT(s.iCount==0, "Sem count");
		TEST_RESULT(info.iExitCpu==aCpu2, "Exit CPU");

		info.Set(type, obj, 63, aCpu);
		t = info.CreateThread("Single4", &xs);
		if (!aMutex)
			TEST_RESULT(s.iCount==0, "Sem count");
		if (aMutex)
			NKern::FMWait(&m);
		NKern::ThreadResume(t);			// resume thread - should wait on semaphore/mutex
		NKern::Sleep(SLEEP_TIME);
		if (!aMutex)
			TEST_RESULT(s.iCount<0, "Sem count");
		TEST_RESULT(info.iExitCpu==-1, "Exit CPU");
		NKern::ThreadSetCpuAffinity(t, aCpu2);	// move blocked thread
		NKern::ThreadKill(t);			// kill thread while blocked on semaphore/mutex
		NKern::FSWait(&xs);
		if (!aMutex)
			TEST_RESULT(s.iCount==0, "Sem count");
		TEST_RESULT(info.iExitCpu==aCpu2, "Exit CPU");
		if (aMutex)
			NKern::FMSignal(&m);

		info.Set(type, obj, 63, aCpu);
		t = info.CreateThread("Single5", &xs);
		if (!aMutex)
			TEST_RESULT(s.iCount==0, "Sem count");
		if (aMutex)
			NKern::FMWait(&m);
		NKern::ThreadResume(t);			// resume thread - should wait on semaphore/mutex
		NKern::Sleep(SLEEP_TIME);
		if (!aMutex)
			TEST_RESULT(s.iCount<0, "Sem count");
		TEST_RESULT(info.iExitCpu==-1, "Exit CPU");
		NKern::ThreadSuspend(t, 1);		// suspend thread while waiting on semaphore/mutex
		NKern::Sleep(SLEEP_TIME);
		if (!aMutex)
			TEST_RESULT(s.iCount<0, "Sem count");
		TEST_RESULT(info.iExitCpu==-1, "Exit CPU");
		NKern::ThreadSetCpuAffinity(t, aCpu2);	// move blocked and suspended thread
		NKern::ThreadKill(t);			// kill thread while blocked on semaphore/mutex and suspended
		NKern::FSWait(&xs);
		if (!aMutex)
			TEST_RESULT(s.iCount==0, "Sem count");
		TEST_RESULT(info.iExitCpu==aCpu2, "Exit CPU");
		if (aMutex)
			NKern::FMSignal(&m);
		}
	}

void DoBasicThreadTest3SemPri(TInt aCpu, TInt aCpu2)
	{
	(void)aCpu2;
	TEST_PRINT("Change priority + semaphore");
	TInt this_cpu = NKern::CurrentCpu();
	TBool same_cpu = (aCpu == this_cpu);
	SThreadInfo3 info;
	NThread* t;
	SThreadInfo3 info2;
	NThread* t2;
	NFastSemaphore xs(0);
	NFastSemaphore s;

	info.Set(SThreadInfo3::EWaitFS, &s, 10, aCpu);
	t = info.CreateThread("SemPri1A", &xs);
	NKern::ThreadResume(t);			// resume thread - should wait on semaphore
	NKern::Sleep(SLEEP_TIME);
	TEST_RESULT(s.iCount<0, "Sem count");
	TEST_RESULT(info.iExitCpu==-1, "Exit CPU");

	info2.Set(SThreadInfo3::ECount, 0, 11, aCpu);
	t2 = info2.CreateThread("SemPri1B", &xs);
	NKern::ThreadResume(t2);		// resume thread - should run in preference to first thread
	CHECK_RUNNING(info2, aCpu);

	NKern::ThreadSetPriority(t, 63);	// change priority while blocked
	NKern::FSSignal(&s);			// signal semaphore - should run and exit immediately
	NKern::FSWait(&xs);
	TEST_RESULT(s.iCount==0, "Sem count");
	TEST_RESULT(info.iExitCpu==aCpu, "Exit CPU");
	CHECK_RUNNING(info2, aCpu);

	info.Set(SThreadInfo3::EWaitFS, &s, 63, aCpu);
	t = info.CreateThread("SemPri1C", &xs);
	NKern::ThreadResume(t);			// resume thread - should wait on semaphore
	NKern::Sleep(SLEEP_TIME);
	TEST_RESULT(s.iCount<0, "Sem count");
	TEST_RESULT(info.iExitCpu==-1, "Exit CPU");
	NKern::ThreadSetPriority(t, 1);	// change priority while blocked
	NKern::FSSignal(&s);			// signal semaphore - shouldn't run because priority lower than 1B
	NKern::Sleep(SLEEP_TIME);
	TEST_RESULT(s.iCount==0, "Sem count");
	TEST_RESULT(info.iExitCpu==-1, "Exit CPU");
	CHECK_RUNNING(info2, aCpu);

	NKern::ThreadKill(t2);
	CHECK_NOT_RUNNING(info2, same_cpu);
	NKern::FSWait(&xs);
	NKern::FSWait(&xs);
	TEST_RESULT(info2.iExitCpu==aCpu, "Exit CPU");
	TEST_RESULT(info.iExitCpu==aCpu, "Exit CPU");
	}

void DoBasicThreadTest3MutexPri(TInt aCpu, TInt aCpu2, TBool aKill)
	{
	TEST_PRINT1("Change priority + mutex ... kill=%d", aKill);
	TInt this_cpu = NKern::CurrentCpu();
	TBool same_cpu = (aCpu == this_cpu);
//	TBool same_cpu2 = (aCpu2 == this_cpu);
	SThreadInfo3 info;
	NThread* t;
	SThreadInfo3 info2;
	NThread* t2;
	SThreadInfo3 info3;
	NThread* t3;
	NFastSemaphore xs(0);
	NFastMutex m;

	info.Set(SThreadInfo3::EHoldFM, &m, 10, aCpu);
	t = info.CreateThread("MutexPri1A", &xs);
	NKern::ThreadResume(t);		// start first thread - it should grab mutex then spin
	CHECK_RUNNING(info, aCpu);
	TEST_RESULT(t->iPriority==10, "Priority");
	info2.Set(SThreadInfo3::EWaitFM, &m, 12, aCpu);
	t2 = info2.CreateThread("MutexPri1B", &xs);
	info3.Set(SThreadInfo3::ECount, 0, 11, aCpu);
	t3 = info3.CreateThread("MutexPri1C", &xs);
	NKern::ThreadResume(t3);	// start t3 - should preempt t1
	CHECK_RUNNING(info3, aCpu);
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadResume(t2);	// start t2 - should wait on mutex, increasing t1's priority in the process
	CHECK_RUNNING(info, aCpu);
	CHECK_NOT_RUNNING(info3, same_cpu);
	TEST_RESULT(info2.iExitCpu==-1, "Exit CPU");
	TEST_RESULT(t->iPriority==12, "Priority");
	NKern::ThreadSetPriority(t2, 9);	// lower t2's priority - should lower t1's as well so t1 stops running
	CHECK_RUNNING(info3, aCpu);
	CHECK_NOT_RUNNING(info, same_cpu);
	TEST_RESULT(t->iPriority==10, "Priority");
	NKern::ThreadSetPriority(t2, 15);	// increase t2's priority - should increase t1's as well
	CHECK_RUNNING(info, aCpu);
	CHECK_NOT_RUNNING(info3, same_cpu);
	TEST_RESULT(t->iPriority==15, "Priority");
	NKern::ThreadSuspend(t2, 1);		// suspend t2 - t1 should now lose inherited priority
	CHECK_RUNNING(info3, aCpu);
	CHECK_NOT_RUNNING(info, same_cpu);
	TEST_RESULT(t->iPriority==10, "Priority");
	NKern::ThreadResume(t2);			// resume t2 - t1 should now regain inherited priority
	CHECK_RUNNING(info, aCpu);
	CHECK_NOT_RUNNING(info3, same_cpu);
	TEST_RESULT(t->iPriority==15, "Priority");
	TEST_RESULT(info2.iExitCpu==-1, "Exit CPU");

	NKern::ThreadSuspend(t2, 1);		// suspend t2 - t1 should now lose inherited priority
	CHECK_RUNNING(info3, aCpu);
	CHECK_NOT_RUNNING(info, same_cpu);
	TEST_RESULT(t->iPriority==10, "Priority");
	NKern::ThreadSetPriority(t2, 9);	// lower t2's priority - should have no effect on t1
	CHECK_RUNNING(info3, aCpu);
	CHECK_NOT_RUNNING(info, same_cpu);
	TEST_RESULT(t->iPriority==10, "Priority");
	NKern::ThreadSetPriority(t2, 15);	// raise t2's priority - should have no effect on t1
	CHECK_RUNNING(info3, aCpu);
	CHECK_NOT_RUNNING(info, same_cpu);
	TEST_RESULT(t->iPriority==10, "Priority");
	NKern::ThreadSetPriority(t2, 9);	// lower t2's priority - should have no effect on t1
	CHECK_RUNNING(info3, aCpu);
	CHECK_NOT_RUNNING(info, same_cpu);
	TEST_RESULT(t->iPriority==10, "Priority");
	NKern::ThreadResume(t2);			// resume t2 - should have no effect on t1
	CHECK_RUNNING(info3, aCpu);
	CHECK_NOT_RUNNING(info, same_cpu);
	TEST_RESULT(t->iPriority==10, "Priority");
	NKern::ThreadSetPriority(t2, 15);	// increase t2's priority - should increase t1's as well
	CHECK_RUNNING(info, aCpu);
	CHECK_NOT_RUNNING(info3, same_cpu);
	TEST_RESULT(t->iPriority==15, "Priority");
	TEST_RESULT(info2.iExitCpu==-1, "Exit CPU");

	if (aCpu2>=0)
		{
		NKern::ThreadSetCpuAffinity(t2, aCpu2);	// move t2 - should have no effect on t1
		CHECK_RUNNING(info, aCpu);
		CHECK_NOT_RUNNING(info3, same_cpu);
		TEST_RESULT(t->iPriority==15, "Priority");
		NKern::ThreadSuspend(t2, 1);		// suspend t2 - t1 should now lose inherited priority
		CHECK_RUNNING(info3, aCpu);
		CHECK_NOT_RUNNING(info, same_cpu);
		TEST_RESULT(t->iPriority==10, "Priority");
		NKern::ThreadResume(t2);			// resume t2 - t1 should now regain inherited priority
		CHECK_RUNNING(info, aCpu);
		CHECK_NOT_RUNNING(info3, same_cpu);
		TEST_RESULT(t->iPriority==15, "Priority");
		NKern::ThreadSetPriority(t2, 9);	// lower t2's priority - should lower t1's as well so t1 stops running
		CHECK_RUNNING(info3, aCpu);
		CHECK_NOT_RUNNING(info, same_cpu);
		TEST_RESULT(t->iPriority==10, "Priority");
		NKern::ThreadSetPriority(t2, 15);	// increase t2's priority - should increase t1's as well
		CHECK_RUNNING(info, aCpu);
		CHECK_NOT_RUNNING(info3, same_cpu);
		TEST_RESULT(t->iPriority==15, "Priority");
		TEST_RESULT(info2.iExitCpu==-1, "Exit CPU");
		}

	TInt xcpu = (aCpu2>=0) ? aCpu2: aCpu;
	if (aKill)
		{
		NKern::ThreadKill(t2);	// kill t2 - t1 should lose inherited priority
		NKern::FSWait(&xs);
		CHECK_RUNNING(info3, aCpu);
		CHECK_NOT_RUNNING(info, same_cpu);
		TEST_RESULT(t->iPriority==10, "Priority");
		TEST_RESULT(info2.iExitCpu==xcpu, "Exit CPU");
		info.iStop = TRUE;
		NKern::ThreadKill(t3);
		NKern::FSWait(&xs);
		NKern::FSWait(&xs);
		CHECK_NOT_RUNNING(info, same_cpu);
		TEST_RESULT(info.iExitCpu==aCpu, "Exit CPU");
		}
	else
		{
		info.iStop = TRUE;	// tell t1 to release mutex and exit
		NKern::FSWait(&xs);	// t2 should also exit
		TEST_RESULT(info2.iExitCpu==xcpu, "Exit CPU");
		TEST_RESULT(info.iExitCpu==-1, "Exit CPU");	// t1 won't exit until we kill t3
		NKern::ThreadKill(t3);
		NKern::FSWait(&xs);
		NKern::FSWait(&xs);
		CHECK_NOT_RUNNING(info, same_cpu);
		TEST_RESULT(info.iExitCpu==aCpu, "Exit CPU");
		}
	CHECK_NOT_RUNNING(info3, same_cpu);
	TEST_RESULT(info3.iExitCpu==aCpu, "Exit CPU");
	}

void DoBasicThreadTest3(TInt aCpu, TInt aCpu2)
	{
	TEST_PRINT2("aCpu=%d aCpu2=%d", aCpu, aCpu2);

	TInt this_cpu = NKern::CurrentCpu();
	TBool same_cpu = (aCpu == this_cpu);
	TBool same_cpu2 = (aCpu2 == this_cpu);
	TBool same_cpux = (aCpu2>=0) ? same_cpu2 : same_cpu;

	SThreadInfo3 info;
	NThread* t;
	NFastSemaphore xs(0);

	info.Set(SThreadInfo3::ECount, 0, 11, aCpu);
	t = info.CreateThread("Single1", &xs);
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadSuspend(t, 1);	// suspend newly created thread before it has been resumed
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadResume(t);		// resume - should still be suspended
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadResume(t);		// resume - now running
	CHECK_RUNNING(info, aCpu);
	NKern::ThreadResume(t);		// resume while running - should be no-op
	CHECK_RUNNING(info, aCpu);
	NKern::ThreadSuspend(t, 1);	// suspend running thread
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadResume(t);		// resume
	CHECK_RUNNING(info, aCpu);
	NKern::ThreadSuspend(t, 3);	// suspend running thread multiple times
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadResume(t);		// resume - still suspended twice
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadResume(t);		// resume - still suspended once
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadResume(t);		// resume - now running
	CHECK_RUNNING(info, aCpu);
	NKern::ThreadSuspend(t, 3);	// suspend multiple times
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadForceResume(t);	// force resume - cancel all suspensions at once
	CHECK_RUNNING(info, aCpu);
	NKern::ThreadSuspend(t, 1);	// suspend running thread
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadSuspend(t, 3);	// suspend multiple times when already suspended
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadResume(t);		// resume - still suspended three times
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadResume(t);		// resume - still suspended twice
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadResume(t);		// resume - still suspended once
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadResume(t);		// resume - now running
	CHECK_RUNNING(info, aCpu);

	if (aCpu2>=0)
		{
		NKern::ThreadSetCpuAffinity(t, aCpu2);	// move running thread to another CPU
		CHECK_RUNNING(info, aCpu2);
		NKern::ThreadSetCpuAffinity(t, aCpu);	// move it back
		CHECK_RUNNING(info, aCpu);
		NKern::ThreadSuspend(t, 2);				// suspend
		CHECK_NOT_RUNNING(info, same_cpu);
		NKern::ThreadSetCpuAffinity(t, aCpu2);	// move suspended thread to another CPU
		CHECK_NOT_RUNNING(info, same_cpu2);
		NKern::ThreadResume(t);					// resume - still suspended
		CHECK_NOT_RUNNING(info, same_cpu2);
		NKern::ThreadResume(t);					// resume - now running on other CPU
		CHECK_RUNNING(info, aCpu2);
		}
	NKern::ThreadKill(t);
	CHECK_NOT_RUNNING(info, same_cpux);
	NKern::FSWait(&xs);
	TEST_RESULT(info.iExitCpu == ((aCpu2>=0)?aCpu2:aCpu), "Exit CPU");

	SThreadInfo3 info2;
	NThread* t2;

	info.Set(SThreadInfo3::ECount, 0, 10, aCpu);
	t = info.CreateThread("Pair1A", &xs);
	CHECK_NOT_RUNNING(info, same_cpu);

	info2.Set(SThreadInfo3::ECount, 0, 11, aCpu);
	t2 = info2.CreateThread("Pair1B", &xs);
	CHECK_NOT_RUNNING(info2, same_cpu);

	NKern::ThreadResume(t);						// resume new thread
	CHECK_RUNNING(info, aCpu);
	CHECK_NOT_RUNNING(info2, same_cpu);
	NKern::ThreadResume(t2);					// resume higher priority thread - should preempt
	CHECK_RUNNING(info2, aCpu);
	CHECK_NOT_RUNNING(info, same_cpu);

	NKern::ThreadSetPriority(t, 12);			// increase priority of ready but not running thread - should preempt
	CHECK_RUNNING(info, aCpu);
	NKern::ThreadSetPriority(t, 10);			// lower priority of running thread - should yield
	CHECK_NOT_RUNNING(info, same_cpu);

	NKern::ThreadSetPriority(t2, 9);			// lower priority of running thread - should yield
	CHECK_RUNNING(info, aCpu);
	NKern::ThreadSetPriority(t2, 11);			// increase priority of ready but not running thread - should preempt
	CHECK_NOT_RUNNING(info, same_cpu);

	NKern::ThreadSetPriority(t2, 14);			// increase priority of running thread - stays running
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadSetPriority(t, 13);			// check priority increase has occurred
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadSetPriority(t2, 11);			//
	CHECK_RUNNING(info, aCpu);
	NKern::ThreadSetPriority(t, 10);			//
	CHECK_NOT_RUNNING(info, same_cpu);

	if (aCpu2>=0)
		{
		NKern::ThreadSetCpuAffinity(t, aCpu2);	// move ready but not running thread to other CPU
		CHECK_RUNNING(info, aCpu2);
		CHECK_RUNNING(info2, aCpu);
		NKern::ThreadSetCpuAffinity(t, aCpu);	// move it back
		CHECK_RUNNING(info2, aCpu);
		CHECK_NOT_RUNNING(info, same_cpu);
		NKern::ThreadSetCpuAffinity(t2, aCpu2);	// move running thread to other CPU - let other thread run on this one
		CHECK_RUNNING(info, aCpu);
		CHECK_RUNNING(info2, aCpu2);
		NKern::ThreadSetCpuAffinity(t2, aCpu);	// move it back
		CHECK_RUNNING(info2, aCpu);
		CHECK_NOT_RUNNING(info, same_cpu);
		}

	NKern::ThreadSuspend(t2, 1);		// suspend running thread
	CHECK_RUNNING(info, aCpu);
	CHECK_NOT_RUNNING(info2, same_cpu);
	NKern::ThreadSetPriority(t2, 9);	// lower priority while suspended
	CHECK_RUNNING(info, aCpu);
	CHECK_NOT_RUNNING(info2, same_cpu);
	NKern::ThreadResume(t2);			// resume - can't now start running again
	CHECK_RUNNING(info, aCpu);
	CHECK_NOT_RUNNING(info2, same_cpu);
	NKern::ThreadSuspend(t2, 1);		// suspend again
	CHECK_RUNNING(info, aCpu);
	CHECK_NOT_RUNNING(info2, same_cpu);
	NKern::ThreadSetPriority(t2, 11);	// increase priority while suspended
	CHECK_RUNNING(info, aCpu);
	CHECK_NOT_RUNNING(info2, same_cpu);
	NKern::ThreadResume(t2);			// resume - starts running again
	CHECK_RUNNING(info2, aCpu);
	CHECK_NOT_RUNNING(info, same_cpu);

	NKern::ThreadSuspend(t, 1);			// suspend ready but not running thread
	CHECK_RUNNING(info2, aCpu);
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadSetPriority(t2, 1);	// lower running thread priority - stays running
	CHECK_RUNNING(info2, aCpu);
	CHECK_NOT_RUNNING(info, same_cpu);
	NKern::ThreadResume(t);				// resume other thread - now preempts
	CHECK_RUNNING(info, aCpu);
	CHECK_NOT_RUNNING(info2, same_cpu);
	NKern::ThreadSetPriority(t2, 11);	// increase other thread priority - should preempt
	CHECK_RUNNING(info2, aCpu);
	CHECK_NOT_RUNNING(info, same_cpu);

	if (aCpu2>=0)
		{
		NKern::ThreadSuspend(t2, 1);		// suspend running thread
		CHECK_RUNNING(info, aCpu);
		CHECK_NOT_RUNNING(info2, same_cpu);
		NKern::ThreadSetCpuAffinity(t2, aCpu2);	// move suspended thread to other CPU
		CHECK_RUNNING(info, aCpu);
		CHECK_NOT_RUNNING(info2, same_cpu2);
		NKern::ThreadResume(t2);			// resume - should start running on other CPU
		CHECK_RUNNING(info, aCpu);
		CHECK_RUNNING(info2, aCpu2);
		}

	NKern::ThreadKill(t2);
	CHECK_NOT_RUNNING(info2, same_cpux);
	CHECK_RUNNING(info, aCpu);
	NKern::ThreadKill(t);
	NKern::FSWait(&xs);
	NKern::FSWait(&xs);
	TEST_RESULT(info2.iExitCpu == ((aCpu2>=0)?aCpu2:aCpu), "Exit CPU");
	TEST_RESULT(info.iExitCpu == aCpu, "Exit CPU");

	DoBasicThreadTest3SemMutex(aCpu, aCpu2, FALSE);
	DoBasicThreadTest3SemMutex(aCpu, aCpu2, TRUE);
	DoBasicThreadTest3SemPri(aCpu, aCpu2);
	DoBasicThreadTest3MutexPri(aCpu, aCpu2, FALSE);
	DoBasicThreadTest3MutexPri(aCpu, aCpu2, TRUE);
	}

void BasicThreadTest3()
	{
	TEST_PRINT("Testing miscellaneous thread operations");

	DoBasicThreadTest3(0,1);
	DoBasicThreadTest3(1,0);
	}

#ifdef __SMP__
struct SThreadGroupTest1Info
	{
	volatile TUint32* iSharedCount;
	volatile TUint32 iThreadCount;
	volatile TBool iDone;
	TUint32 iLimit;
	};

TUint32 Inc(TUint32 a)
	{
	return a+1;
	}

NThreadGroup TG1;

//////////////////////////////////////////////////////////////////////////////
// This thread function increments its iThreadCount until it reaches iLimit
// Each time around the loop it increments iSharedCount with interrupts
// disabled, but without otherwise taking any precautions to be atomic.
//
// If the thread is in the group, then this should behave the same as on a
// uniprocessor system: the increment is atomic. Otherwise, some updates will
// be lost.

void ThreadGroupTest1Thread(TAny* aPtr)
	{
	SThreadGroupTest1Info& a = *(SThreadGroupTest1Info*)aPtr;
	a.iThreadCount = 0;
	NKern::ThreadSetPriority(NKern::CurrentThread(), 12);
	FOREVER
		{
		TUint32 x = ++a.iThreadCount;
		TInt irq = NKern::DisableAllInterrupts();
		TUint32 y = *a.iSharedCount;
		y = Inc(y);
		*a.iSharedCount = y;
		NKern::RestoreInterrupts(irq);
		if (x>=a.iLimit)
			break;
		}
	a.iDone = TRUE;
	NKern::WaitForAnyRequest();
	}

//////////////////////////////////////////////////////////////////////////////
// ThreadGroupTest1
//
// Attempt to prove various properties of thread group scheduling by creating
// a number of copies of a thread that manipulate a shared counter.
// 
// 1) Priority scheduling is strictly observed within a group - lower priority
// threads do not run if any higher priority threads are runnable, no matter
// the number of available CPUs.
// 2) Only one thread in a group is ever running at one time, regardless of
// priorities or the number of available CPUs.
//
// Parameters:
//  aCount: how many threads to create
//  aJoin: whether to have threads join the group


void ThreadGroupTest1(TInt aCount, TBool aJoin, TBool aMigrate, TBool aReJoin)
	{
	TEST_PRINT4("ThreadGroupTest1 aCount=%d aJoin=%d aMigrate=%d aReJoin=%d", aCount, aJoin, aMigrate, aReJoin);
	NFastSemaphore exitSem(0);
	NThread* t[16];
	SThreadGroupTest1Info info[16];
	volatile TUint32 shared=0;
	memclr(t,sizeof(t));
	memclr(&info,sizeof(info));
	TInt i;
	NThreadGroup* group = aJoin ? &TG1 : 0;
	SNThreadGroupCreateInfo ginfo;
	ginfo.iCpuAffinity = 0xffffffff;
	ginfo.iDestructionDfc = 0;	//FIXME
	TInt r = KErrNone;
	if (group)
		r = NKern::GroupCreate(group, ginfo);
	TEST_RESULT(r==KErrNone, "");
	NThreadGroup* g;
	g = NKern::LeaveGroup();
	TEST_RESULT(!g, "");
	char name[8]={0x54, 0x47, 0x54, 0x31, 0, 0, 0, 0};
	for (i=0; i<aCount; ++i)
		{
		info[i].iThreadCount = KMaxTUint32;
		info[i].iSharedCount = &shared;
		info[i].iLimit = 10000000;
		name[4] = (char)('a'+i);
		t[i] = CreateUnresumedThreadSignalOnExit(name, &ThreadGroupTest1Thread, 17, &info[i], 0, __microseconds_to_timeslice_ticks(2000), &exitSem, 0xffffffff, group);
		TEST_OOM(t[i]);
		}
	if (group)
		{
		NKern::JoinGroup(group);
		}
	for (i=0; i<aCount; ++i)
		{
		// Each thread starts with count KMaxTUint32
		TEST_RESULT(info[i].iThreadCount == KMaxTUint32, "");
		NKern::ThreadResume(t[i]);
		// Property 1:
		// After resuming, the thread is higher priority than this one.
		// It sets the count to 0 then lowers its priority to less than this.
		// Thus, if we are in a group with it, then we should get preempted while
		// it sets its count, then regain control after it does. If we were not in
		// a group, we could observe other values of iThreadCount at this point as
		// it may not have run at all (scheduled on another CPU which is busy with
		// a higher priority thread) or may have run for longer (on another CPU)
		if (group)
			{
			TEST_RESULT(info[i].iThreadCount == 0, "");
			}
		TEST_PRINT2("Thread %d Count=%d", i, info[i].iThreadCount);
		}
	if (group)
		{
		TEST_PRINT2("Group Count=%d, SharedCount=%d", group->iThreadCount, shared);
		TEST_RESULT(group->iThreadCount == aCount+1, "");
		g = NKern::LeaveGroup();
		TEST_RESULT(g==group, "");
		g = NKern::LeaveGroup();
		TEST_RESULT(!g, "");
		}
	else
		{
		TEST_PRINT1("SharedCount=%d", shared);
		}
	if (aMigrate)
		{
		TInt cpu = 0;
		TInt ncpus = NKern::NumberOfCpus();
		TUint32 s0 = shared - 1;
		FOREVER
			{
			TInt dead = 0;
			for (i=0; i<aCount; ++i)
				if (info[i].iDone)
					++dead;
			if (dead == aCount)
				break;
			if (shared != s0)
				{
				if (++cpu == ncpus)
					cpu = 1;
				NKern::ThreadSetCpuAffinity(t[aCount-1], cpu);
				s0 = shared;
				}
			nfcfspin(__microseconds_to_norm_fast_counter(2797));
			if (aReJoin)
				{
				NKern::JoinGroup(group);
				TEST_RESULT(NKern::CurrentCpu()==cpu,"");
				TUint32 s1 = shared;
				nfcfspin(__microseconds_to_norm_fast_counter(2797));
				TEST_RESULT(shared==s1,"");
				NThreadGroup* gg = NKern::LeaveGroup();
				TEST_RESULT(gg==group,"");
				NKern::ThreadSetCpuAffinity(NKern::CurrentThread(), 0xffffffff);
				}
			}
		}
	for (i=0; i<aCount; ++i)
		{
		NKern::ThreadRequestSignal(t[i]);
		}
	for (i=0; i<aCount; ++i)
		{
		NKern::FSWait(&exitSem);
		}
	TUint32 total = 0;
	for (i=0; i<aCount; ++i)
		{
		TEST_PRINT2("Thread %d Count=%d", i, info[i].iThreadCount);
		TEST_RESULT(info[i].iThreadCount == info[i].iLimit, "");
		total += info[i].iLimit;
		}
	TEST_PRINT1("SharedCount=%d", shared);
	if (aJoin)
		{
		// Property 2:
		// If the threads were all in a group, then disabling interrupts would
		// suffice to make the increment atomic, and the total count should be
		// the same as the sum of the per-thread counts
		TEST_RESULT(shared == total, "");
		}
	else
		{
		// Property 2 continued:
		// If the threads were not in a group, then disabling interrupts is not
		// enough, and it's overwhelmingly likely that at least one increment
		// will've been missed.
		TEST_RESULT(shared < total, "");
		}
	if (group)
		NKern::GroupDestroy(group);
	}
#endif

void BasicThreadTests()
	{
	BasicThreadTest1();
	BasicThreadTest2();
	TimesliceTest();
	TimesliceTest2();
	BasicThreadTest3();

#ifdef __SMP__
	ThreadGroupTest1(2,0,FALSE,FALSE);
	ThreadGroupTest1(2,1,FALSE,FALSE);
	ThreadGroupTest1(3,0,FALSE,FALSE);
	ThreadGroupTest1(3,1,FALSE,FALSE);
	ThreadGroupTest1(3,1,TRUE,FALSE);
	ThreadGroupTest1(3,1,TRUE,TRUE);
#endif
	}
