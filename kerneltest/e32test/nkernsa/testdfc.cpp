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
// e32test\nkernsa\testdfc.cpp
// 
//

#include <nktest/nkutils.h>

#ifndef __SMP__
#define iNThreadBaseSpare7 iSpare7
class NSchedulable;
#endif

extern "C" TUint32 set_bit0_if_nonnull(TUint32&);
extern "C" void flip_bit0(TUint32&);
extern "C" TUint32 swap_out_if_bit0_clear(TUint32&);

#ifdef __SMP__
class TAddDfc : public TGenericIPI
#else
class TAddDfc : public NTimer
#endif
	{
public:
	TAddDfc();
	TDfc* Add(TDfc* aDfc, TUint32 aCpuMask);
	static TAddDfc* New();
#ifdef __SMP__
	static void Isr(TGenericIPI*);
#else
	static void TimerCallBack(TAny*);
	void WaitCompletion();
#endif
public:
	TDfc* iDfc;
	};

TAddDfc::TAddDfc()
#ifdef __SMP__
	:	iDfc(0)
#else
	:	NTimer(&TimerCallBack, this),
		iDfc(0)
#endif
	{
	}

TAddDfc* TAddDfc::New()
	{
	TAddDfc* p = new TAddDfc;
	TEST_OOM(p);
	return p;
	}

#ifdef __SMP__
void TAddDfc::Isr(TGenericIPI* a)
#else
void TAddDfc::TimerCallBack(TAny* a)
#endif
	{
	TAddDfc& adder = *(TAddDfc*)a;
	TDfc* dfc = (TDfc*)__e32_atomic_swp_ord_ptr(&adder.iDfc, 0);
	if (dfc)
		dfc->Add();
	}

TDfc* TAddDfc::Add(TDfc* aDfc, TUint32 aCpuMask)
	{
	TDfc* old = (TDfc*)__e32_atomic_swp_ord_ptr(&iDfc, aDfc);
#ifdef __SMP__
	Queue(&Isr, aCpuMask);
#else
	(void)aCpuMask;
	OneShot(1);
#endif
	return old;
	}

#ifndef __SMP__
void TAddDfc::WaitCompletion()
	{
	while (iDfc)
		{}
	}
#endif

class TTestDfc : public TDfc
	{
public:
	TTestDfc(TUint aId);
	TTestDfc(TUint aId, TDfcQue* aQ, TInt aPri);
	static void Run(TAny* aPtr);

	static void CheckEmpty(TInt aLine);
	static void CheckFirstEntry(TInt aLine, TUint32 aCpu, TUint32 aContext, TDfcQue* aQ, TUint32 aId);

	static CircBuf* Buffer;
	static volatile TBool Full;
	static volatile TUint32 Last;

	enum {EBufferSlots=1024};
	};

#define CHECK_EMPTY()	TTestDfc::CheckEmpty(__LINE__)
#define CHECK_FIRST_ENTRY(cpu, ctx, q, id)	TTestDfc::CheckFirstEntry(__LINE__, cpu, ctx, q, id)

CircBuf* TTestDfc::Buffer;
volatile TBool TTestDfc::Full = FALSE;
volatile TUint32 TTestDfc::Last;

TTestDfc::TTestDfc(TUint aId)
	:	TDfc(&Run, (TAny*)aId)
	{
	}

TTestDfc::TTestDfc(TUint aId, TDfcQue* aQ, TInt aPri)
	:	TDfc(&Run, (TAny*)aId, aQ, aPri)
	{
	}

void TTestDfc::Run(TAny* aPtr)
	{
	TUint32 id = (TUint32)aPtr;
	TUint32 tid = 0;
	TUint32 ctx = NKern::CurrentContext();
	TUint32 cpu = NKern::CurrentCpu();
	if (ctx == NKern::EThread)
		{
		NThread* t = NKern::CurrentThread();
		tid = t->iNThreadBaseSpare7;
		}
	TUint32 x = (cpu<<24) | (ctx<<16) | (tid<<8) | id;
	TInt r = Buffer->TryPut(x);
	if (r != KErrNone)
		Full = TRUE;
	Last = id;
	}

void TTestDfc::CheckEmpty(TInt aLine)
	{
	TInt c = Buffer->Count();
	TUint32 x;
	Buffer->TryGet(x);
	if (c!=0)
		{
		TEST_PRINT3("Line %d Buffer not empty C:%d X:%08x", aLine, c, x);
		}
	}

void TTestDfc::CheckFirstEntry(TInt aLine, TUint32 aCpu, TUint32 aContext, TDfcQue* aQ, TUint32 aId)
	{
	TUint32 tid = aQ ? aQ->iThread->iNThreadBaseSpare7 : 0;
	TUint32 expected = (aCpu<<24) | (aContext<<16) | (tid << 8) | aId;
	TUint32 x;
	TInt r = Buffer->TryGet(x);
	if (r!=KErrNone)
		{
		TEST_PRINT2("Line %d Buffer empty, Expected %08x", aLine, expected);
		}
	else if (x != expected)
		{
		TEST_PRINT3("Line %d Got %08x Expected %08x", aLine, x, expected);
		}
	}

class TPauseIDFC : public TDfc
	{
public:
	TPauseIDFC();
	void Pause(TInt aCpu);
	void Release();
	static void Run(TAny*);
public:
	volatile TInt iFlag;
	};

TPauseIDFC::TPauseIDFC()
	:	TDfc(&Run, this),
		iFlag(-1)
	{
	}

void TPauseIDFC::Pause(TInt aCpu)
	{
	TAddDfc adder;
	iFlag = -1;
	__e32_memory_barrier();
	adder.Add(this, 1u<<aCpu);
	adder.WaitCompletion();
	while (iFlag == -1)
		{}
	}

void TPauseIDFC::Release()
	{
	__e32_atomic_store_ord32(&iFlag, 1);
	}

void TPauseIDFC::Run(TAny* aPtr)
	{
	TPauseIDFC* p = (TPauseIDFC*)aPtr;
	__e32_atomic_store_ord32(&p->iFlag, 0);
	while (__e32_atomic_load_acq32(&p->iFlag) == 0)
		{}
	}

class TPauseDFC : public TDfc
	{
public:
	TPauseDFC(TDfcQue* aQ);
	void Pause(TInt aWait=0);
	void BusyPause();
	void Release();
	static void Run(TAny*);
public:
	NFastSemaphore* volatile iSem;
	volatile TInt iWait;
	};

TPauseDFC::TPauseDFC(TDfcQue* aQ)
	:	TDfc(&Run, this, aQ, 0),
		iSem(0)
	{
	}

void TPauseDFC::Pause(TInt aWait)
	{
	iWait = aWait;
	NFastSemaphore entrySem(0);
	iSem = &entrySem;
	Enque();
	NKern::FSWait(&entrySem);
	}

void TPauseDFC::BusyPause()
	{
	volatile TInt& flag = (volatile TInt&)iSem;
	__e32_atomic_store_ord32(&flag, 0xfffffffe);
	Enque();
	while (__e32_atomic_load_acq32(&flag) == 0xfffffffe)
		{}
	}

void TPauseDFC::Release()
	{
	NFastSemaphore* s = (NFastSemaphore*)__e32_atomic_swp_ord_ptr(&iSem, 0);
	if (((TInt)s)==-1)
		{
		volatile TInt& flag = (volatile TInt&)iSem;
		__e32_atomic_store_ord32(&flag, 0);
		}
	else
		NKern::FSSignal(s);
	}

void TPauseDFC::Run(TAny* aPtr)
	{
	TPauseDFC* p = (TPauseDFC*)aPtr;
	volatile TInt& flag = (volatile TInt&)p->iSem;
	if (flag == -2)
		{
		flag = -1;
		__e32_memory_barrier();
		while (flag == -1)
			{}
		}
	else
		{
		NFastSemaphore exitSem(0);
		NFastSemaphore* s = (NFastSemaphore*)__e32_atomic_swp_ord_ptr(&p->iSem, &exitSem);
		if (p->iWait)
			{
			nfcfspin(__microseconds_to_norm_fast_counter(10000));
			NKern::Sleep(p->iWait);
			}
		NKern::FSSignal(s);
		NKern::FSWait(&exitSem);
		}
	}

void DoDFCTest1()
	{
	TEST_PRINT("DFCTest1");
	TInt cpu;
	for_each_cpu(cpu)
		{
		TDfcQue* q = CreateDfcQ("DfcQ0", 1, cpu);
		DestroyDfcQ(q);
		q = CreateDfcQ("DfcQ1", 32, cpu);
		DestroyDfcQ(q);
		}
	}

TBool QueueDfc(TDfc* aDfc, TInt aMode, TBool aExRet)
	{
	if (aMode==0)
		return !aExRet == !aDfc->Enque();
	else if (aMode>0)
		{
		TAddDfc adder;
		TInt cpu = aMode - 1;
		adder.Add(aDfc, 1u<<cpu);
		adder.WaitCompletion();
		nfcfspin(__microseconds_to_norm_fast_counter(10000));
		return TRUE;
		}
	else if (aMode==-1)
		{
		NKern::Lock();
		TBool ret = aDfc->Add();
		NKern::Unlock();
		return !aExRet == !ret;
		}
	return FALSE;
	}

#define QUEUE_DFC(dfc, mode, exret)	TEST_RESULT(QueueDfc(dfc,mode,exret),"")

void DoDFCTest2()
	{
	TEST_PRINT("DFCTest2");
	TInt num_cpus = NKern::NumberOfCpus();
	TInt this_cpu = NKern::CurrentCpu();
	TDfcQue* q;
	q = CreateDfcQ("DfcQ2", 1, this_cpu);
	TEST_OOM(q);
	q->iThread->iNThreadBaseSpare7 = 1;

	TTestDfc* d1 = new TTestDfc(1, q, 1);
	TEST_OOM(d1);
	TEST_RESULT(!d1->IsIDFC(), "");
	TTestDfc* d2 = new TTestDfc(2, q, 2);
	TEST_OOM(d2);
	TEST_RESULT(!d2->IsIDFC(), "");
	TTestDfc* d3 = new TTestDfc(3, q, 2);
	TEST_OOM(d3);
	TEST_RESULT(!d3->IsIDFC(), "");
	TTestDfc* d4 = new TTestDfc(4, q, 3);
	TEST_OOM(d4);
	TEST_RESULT(!d4->IsIDFC(), "");

	TInt mode;
	for (mode=-1; mode<=num_cpus; ++mode)
		{
		TEST_PRINT1("Mode %d", mode);
		CHECK_EMPTY();
		TEST_RESULT(!d1->Queued(), "");
		QUEUE_DFC(d1, mode, TRUE);
		TEST_RESULT(d1->Queued(), "");
		QUEUE_DFC(d1, mode, FALSE);
		TEST_RESULT(d1->Queued(), "");
		QUEUE_DFC(d2, mode, TRUE);
		QUEUE_DFC(d3, mode, TRUE);
		QUEUE_DFC(d4, mode, TRUE);
		CHECK_EMPTY();
		NKern::Sleep(30);
		CHECK_FIRST_ENTRY(this_cpu, NKern::EThread, q, 4);
		CHECK_FIRST_ENTRY(this_cpu, NKern::EThread, q, 2);
		CHECK_FIRST_ENTRY(this_cpu, NKern::EThread, q, 3);
		CHECK_FIRST_ENTRY(this_cpu, NKern::EThread, q, 1);
		CHECK_EMPTY();
		QUEUE_DFC(d4, mode, TRUE);
		QUEUE_DFC(d3, mode, TRUE);
		QUEUE_DFC(d2, mode, TRUE);
		QUEUE_DFC(d1, mode, TRUE);
		CHECK_EMPTY();
		NKern::Sleep(30);
		CHECK_FIRST_ENTRY(this_cpu, NKern::EThread, q, 4);
		CHECK_FIRST_ENTRY(this_cpu, NKern::EThread, q, 3);
		CHECK_FIRST_ENTRY(this_cpu, NKern::EThread, q, 2);
		CHECK_FIRST_ENTRY(this_cpu, NKern::EThread, q, 1);
		CHECK_EMPTY();
		QUEUE_DFC(d4, mode, TRUE);
		QUEUE_DFC(d3, mode, TRUE);
		QUEUE_DFC(d2, mode, TRUE);
		QUEUE_DFC(d1, mode, TRUE);
		TEST_RESULT(d4->Queued(), "");
		TEST_RESULT(d4->Cancel(), "");
		TEST_RESULT(!d4->Cancel(), "");
		TEST_RESULT(!d4->Queued(), "");
		CHECK_EMPTY();
		NKern::Sleep(30);
		CHECK_FIRST_ENTRY(this_cpu, NKern::EThread, q, 3);
		CHECK_FIRST_ENTRY(this_cpu, NKern::EThread, q, 2);
		CHECK_FIRST_ENTRY(this_cpu, NKern::EThread, q, 1);
		CHECK_EMPTY();
		QUEUE_DFC(d4, mode, TRUE);
		QUEUE_DFC(d3, mode, TRUE);
		QUEUE_DFC(d2, mode, TRUE);
		QUEUE_DFC(d1, mode, TRUE);
		TEST_RESULT(d3->Queued(), "");
		TEST_RESULT(d3->Cancel(), "");
		TEST_RESULT(!d3->Queued(), "");
		CHECK_EMPTY();
		NKern::Sleep(30);
		CHECK_FIRST_ENTRY(this_cpu, NKern::EThread, q, 4);
		CHECK_FIRST_ENTRY(this_cpu, NKern::EThread, q, 2);
		CHECK_FIRST_ENTRY(this_cpu, NKern::EThread, q, 1);
		CHECK_EMPTY();
		QUEUE_DFC(d4, mode, TRUE);
		QUEUE_DFC(d3, mode, TRUE);
		QUEUE_DFC(d2, mode, TRUE);
		QUEUE_DFC(d1, mode, TRUE);
		TEST_RESULT(d3->Queued(), "");
		TEST_RESULT(d2->Queued(), "");
		TEST_RESULT(d3->Cancel(), "");
		TEST_RESULT(d2->Cancel(), "");
		TEST_RESULT(!d3->Queued(), "");
		TEST_RESULT(!d2->Queued(), "");
		CHECK_EMPTY();
		NKern::Sleep(30);
		CHECK_FIRST_ENTRY(this_cpu, NKern::EThread, q, 4);
		CHECK_FIRST_ENTRY(this_cpu, NKern::EThread, q, 1);
		CHECK_EMPTY();
		QUEUE_DFC(d4, mode, TRUE);
		QUEUE_DFC(d3, mode, TRUE);
		QUEUE_DFC(d2, mode, TRUE);
		QUEUE_DFC(d1, mode, TRUE);
		TEST_RESULT(d3->Cancel(), "");
		TEST_RESULT(d2->Cancel(), "");
		TEST_RESULT(d4->Cancel(), "");
		TEST_RESULT(d1->Cancel(), "");
		CHECK_EMPTY();
		NKern::Sleep(30);
		CHECK_EMPTY();
		QUEUE_DFC(d4, mode, TRUE);
		QUEUE_DFC(d3, mode, TRUE);
		QUEUE_DFC(d2, mode, TRUE);
		QUEUE_DFC(d1, mode, TRUE);
		TEST_RESULT(d1->Queued(), "");
		TEST_RESULT(d3->Cancel(), "");
		TEST_RESULT(d2->Cancel(), "");
		TEST_RESULT(d4->Cancel(), "");
		TEST_RESULT(d1->Cancel(), "");
		TEST_RESULT(!d1->Queued(), "");
		QUEUE_DFC(d1, mode, TRUE);
		TEST_RESULT(d1->Queued(), "");
		QUEUE_DFC(d1, mode, FALSE);
		TEST_RESULT(d1->Queued(), "");
		CHECK_EMPTY();
		NKern::Sleep(30);
		CHECK_FIRST_ENTRY(this_cpu, NKern::EThread, q, 1);
		CHECK_EMPTY();
		}

	delete d4;
	delete d3;
	delete d2;
	delete d1;
	DestroyDfcQ(q);
	}

void DoDFCTest3(TInt aCpu)
	{
	TEST_PRINT1("DFCTest3 CPU %d", aCpu);
	TInt num_cpus = NKern::NumberOfCpus();
	TInt this_cpu = NKern::CurrentCpu();
	TBool same_cpu = (aCpu==this_cpu);
	TDfcQue* q;
	q = CreateDfcQ("DfcQ2", 32, aCpu);
	TEST_OOM(q);
	q->iThread->iNThreadBaseSpare7 = 1;
	TPauseDFC pauser(q);

	TTestDfc* d1 = new TTestDfc(1, q, 1);
	TEST_OOM(d1);
	TEST_RESULT(!d1->IsIDFC(), "");
	TTestDfc* d2 = new TTestDfc(2, q, 2);
	TEST_OOM(d2);
	TEST_RESULT(!d2->IsIDFC(), "");
	TTestDfc* d3 = new TTestDfc(3, q, 2);
	TEST_OOM(d3);
	TEST_RESULT(!d3->IsIDFC(), "");
	TTestDfc* d4 = new TTestDfc(4, q, 3);
	TEST_OOM(d4);
	TEST_RESULT(!d4->IsIDFC(), "");

	TInt mode;
	for (mode=-1; mode<=num_cpus; ++mode)
		{
		TEST_PRINT1("Mode %d", mode);
		CHECK_EMPTY();
		TEST_RESULT(!d1->Queued(), "");
		QUEUE_DFC(d1, mode, TRUE);
		if (!same_cpu)
			while (d1->Queued()) {}
		TEST_RESULT(!d1->Queued(), "");
		QUEUE_DFC(d1, mode, TRUE);
		if (!same_cpu)
			while (d1->Queued()) {}
		TEST_RESULT(!d1->Queued(), "");
		QUEUE_DFC(d2, mode, TRUE);
		if (!same_cpu)
			while (d2->Queued()) {}
		QUEUE_DFC(d3, mode, TRUE);
		if (!same_cpu)
			while (d3->Queued()) {}
		QUEUE_DFC(d4, mode, TRUE);
		if (!same_cpu)
			while (d4->Queued()) {}
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 1);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 1);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 2);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 3);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 4);
		CHECK_EMPTY();
		QUEUE_DFC(d4, mode, TRUE);
		QUEUE_DFC(d3, mode, TRUE);
		QUEUE_DFC(d2, mode, TRUE);
		QUEUE_DFC(d1, mode, TRUE);
		if (!same_cpu)
			while (d1->Queued()) {}
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 4);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 3);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 2);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 1);
		CHECK_EMPTY();
		QUEUE_DFC(d4, mode, TRUE);
		QUEUE_DFC(d3, mode, TRUE);
		QUEUE_DFC(d2, mode, TRUE);
		QUEUE_DFC(d1, mode, TRUE);
		if (!same_cpu)
			while (d1->Queued()) {}
		TEST_RESULT(!d4->Queued(), "");
		TEST_RESULT(!d4->Cancel(), "");
		TEST_RESULT(!d4->Queued(), "");
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 4);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 3);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 2);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 1);
		CHECK_EMPTY();
		pauser.Pause();
		CHECK_EMPTY();
		TEST_RESULT(!d1->Queued(), "");
		QUEUE_DFC(d1, mode, TRUE);
		TEST_RESULT(d1->Queued(), "");
		QUEUE_DFC(d1, mode, FALSE);
		TEST_RESULT(d1->Queued(), "");
		QUEUE_DFC(d2, mode, TRUE);
		QUEUE_DFC(d3, mode, TRUE);
		QUEUE_DFC(d4, mode, TRUE);
		QUEUE_DFC(d4, mode, FALSE);
		CHECK_EMPTY();
		pauser.Release();
		pauser.Pause();
		pauser.Release();
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 4);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 2);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 3);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 1);
		CHECK_EMPTY();
		pauser.Pause();
		CHECK_EMPTY();
		TEST_RESULT(!d1->Queued(), "");
		QUEUE_DFC(d1, mode, TRUE);
		TEST_RESULT(d1->Queued(), "");
		QUEUE_DFC(d1, mode, FALSE);
		TEST_RESULT(d1->Queued(), "");
		QUEUE_DFC(d4, mode, TRUE);
		QUEUE_DFC(d3, mode, TRUE);
		QUEUE_DFC(d2, mode, TRUE);
		CHECK_EMPTY();
		pauser.Release();
		pauser.Pause();
		pauser.Release();
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 4);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 3);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 2);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 1);
		CHECK_EMPTY();
		pauser.Pause();
		CHECK_EMPTY();
		TEST_RESULT(!d1->Queued(), "");
		QUEUE_DFC(d1, mode, TRUE);
		TEST_RESULT(d1->Queued(), "");
		QUEUE_DFC(d1, mode, FALSE);
		TEST_RESULT(d1->Queued(), "");
		QUEUE_DFC(d2, mode, TRUE);
		QUEUE_DFC(d3, mode, TRUE);
		QUEUE_DFC(d4, mode, TRUE);
		CHECK_EMPTY();
		TEST_RESULT(d1->Cancel(), "");
		TEST_RESULT(!d1->Queued(), "");
		pauser.Release();
		pauser.Pause();
		pauser.Release();
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 4);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 2);
		CHECK_FIRST_ENTRY(aCpu, NKern::EThread, q, 3);
		CHECK_EMPTY();
		}

	delete d4;
	delete d3;
	delete d2;
	delete d1;
	DestroyDfcQ(q);
	}

TBool QueueIDfc(TDfc* aDfc, TInt aMode, TBool aExRet)
	{
	if (aMode==0)
		return !aExRet == !aDfc->RawAdd();
	else if (aMode>0)
		{
		TTestDfc::Last = 0xffffffffu;
		TAddDfc adder;
		TInt cpu = (aMode&0xff) - 1;
		adder.Add(aDfc, 1u<<cpu);
		adder.WaitCompletion();
		if (!(aMode&0x100))
			{
			while (TTestDfc::Last != (TUint32)aDfc->iPtr)
				{}
			}
		return TRUE;
		}
	else if (aMode==-1)
		{
		NKern::Lock();
		TBool ret = aDfc->Add();
		NKern::Unlock();
		return !aExRet == !ret;
		}
	return FALSE;
	}

#define QUEUE_IDFC(dfc, mode, exret)	TEST_RESULT(QueueIDfc(dfc,mode,exret),"")

void DoIDFCTest1()
	{
	TEST_PRINT("IDFCTest1");

	TInt num_cpus = NKern::NumberOfCpus();
	TInt this_cpu = NKern::CurrentCpu();

	TTestDfc* d1 = new TTestDfc(1);
	TEST_OOM(d1);
	TEST_RESULT(d1->IsIDFC(), "");
	TTestDfc* d2 = new TTestDfc(2);
	TEST_OOM(d2);
	TEST_RESULT(d2->IsIDFC(), "");
	TTestDfc* d3 = new TTestDfc(3);
	TEST_OOM(d3);
	TEST_RESULT(d3->IsIDFC(), "");
	TTestDfc* d4 = new TTestDfc(4);
	TEST_OOM(d4);
	TEST_RESULT(d4->IsIDFC(), "");

	TInt mode;
	for (mode=-1; mode<=num_cpus; ++mode)
		{
		TInt xcpu = (mode>0) ? (mode-1) : this_cpu;
		TEST_PRINT1("Mode %d", mode);
		CHECK_EMPTY();
		TEST_RESULT(!d1->Queued(), "");
		QUEUE_IDFC(d1, mode, TRUE);
		TEST_RESULT(!d1->Queued(), "");
		QUEUE_IDFC(d1, mode, TRUE);
		TEST_RESULT(!d1->Queued(), "");
		QUEUE_IDFC(d2, mode, TRUE);
		QUEUE_IDFC(d3, mode, TRUE);
		QUEUE_IDFC(d4, mode, TRUE);
		CHECK_FIRST_ENTRY(xcpu, NKern::EIDFC, 0, 1);
		CHECK_FIRST_ENTRY(xcpu, NKern::EIDFC, 0, 1);
		CHECK_FIRST_ENTRY(xcpu, NKern::EIDFC, 0, 2);
		CHECK_FIRST_ENTRY(xcpu, NKern::EIDFC, 0, 3);
		CHECK_FIRST_ENTRY(xcpu, NKern::EIDFC, 0, 4);
		CHECK_EMPTY();
		QUEUE_IDFC(d4, mode, TRUE);
		QUEUE_IDFC(d3, mode, TRUE);
		QUEUE_IDFC(d2, mode, TRUE);
		QUEUE_IDFC(d1, mode, TRUE);
		CHECK_FIRST_ENTRY(xcpu, NKern::EIDFC, 0, 4);
		CHECK_FIRST_ENTRY(xcpu, NKern::EIDFC, 0, 3);
		CHECK_FIRST_ENTRY(xcpu, NKern::EIDFC, 0, 2);
		CHECK_FIRST_ENTRY(xcpu, NKern::EIDFC, 0, 1);
		CHECK_EMPTY();
		}
	TInt irq = NKern::DisableAllInterrupts();
	TEST_RESULT(d1->RawAdd(), "");
	TEST_RESULT(d1->Queued(), "");
	CHECK_EMPTY();
	NKern::RestoreInterrupts(irq);
	TEST_RESULT(!d1->Queued(), "");
	CHECK_FIRST_ENTRY(this_cpu, NKern::EIDFC, 0, 1);

	NKern::Lock();
	TEST_RESULT(d1->Add(), "");
	TEST_RESULT(d3->Add(), "");
	TEST_RESULT(d2->Add(), "");
	TEST_RESULT(d4->Add(), "");
	TEST_RESULT(!d1->Add(), "");
	TEST_RESULT(d1->Queued(), "");
	TEST_RESULT(d2->Queued(), "");
	TEST_RESULT(d3->Queued(), "");
	TEST_RESULT(d4->Queued(), "");
	CHECK_EMPTY();
	NKern::Unlock();
	TEST_RESULT(!d1->Queued(), "");
	TEST_RESULT(!d2->Queued(), "");
	TEST_RESULT(!d3->Queued(), "");
	TEST_RESULT(!d4->Queued(), "");
	CHECK_FIRST_ENTRY(this_cpu, NKern::EIDFC, 0, 1);
	CHECK_FIRST_ENTRY(this_cpu, NKern::EIDFC, 0, 3);
	CHECK_FIRST_ENTRY(this_cpu, NKern::EIDFC, 0, 2);
	CHECK_FIRST_ENTRY(this_cpu, NKern::EIDFC, 0, 4);
	CHECK_EMPTY();

	NKern::Lock();
	TEST_RESULT(d1->Add(), "");
	TEST_RESULT(d3->Add(), "");
	TEST_RESULT(d2->Add(), "");
	TEST_RESULT(d4->Add(), "");
	TEST_RESULT(d1->Queued(), "");
	TEST_RESULT(d2->Queued(), "");
	TEST_RESULT(d3->Queued(), "");
	TEST_RESULT(d4->Queued(), "");
	TEST_RESULT(d3->Cancel(), "");
	TEST_RESULT(!d3->Queued(), "");
	TEST_RESULT(!d3->Cancel(), "");
	CHECK_EMPTY();
	NKern::Unlock();
	TEST_RESULT(!d1->Queued(), "");
	TEST_RESULT(!d2->Queued(), "");
	TEST_RESULT(!d4->Queued(), "");
	CHECK_FIRST_ENTRY(this_cpu, NKern::EIDFC, 0, 1);
	CHECK_FIRST_ENTRY(this_cpu, NKern::EIDFC, 0, 2);
	CHECK_FIRST_ENTRY(this_cpu, NKern::EIDFC, 0, 4);
	CHECK_EMPTY();

	NKern::Lock();
	TEST_RESULT(d1->Add(), "");
	TEST_RESULT(d3->Add(), "");
	TEST_RESULT(d2->Add(), "");
	TEST_RESULT(d4->Add(), "");
	TEST_RESULT(d1->Queued(), "");
	TEST_RESULT(d2->Queued(), "");
	TEST_RESULT(d3->Queued(), "");
	TEST_RESULT(d4->Queued(), "");
	TEST_RESULT(d3->Cancel(), "");
	TEST_RESULT(d1->Cancel(), "");
	TEST_RESULT(d2->Cancel(), "");
	TEST_RESULT(d4->Cancel(), "");
	TEST_RESULT(!d1->Queued(), "");
	TEST_RESULT(!d2->Queued(), "");
	TEST_RESULT(!d3->Queued(), "");
	TEST_RESULT(!d4->Queued(), "");
	CHECK_EMPTY();
	NKern::Unlock();
	CHECK_EMPTY();

	TPauseIDFC pauser;
	TInt cpu;
	for_each_cpu(cpu)
		{
		if (cpu == this_cpu)
			continue;
		mode = cpu + 0x101;
		TEST_PRINT1("CPU %d", cpu);
		pauser.Pause(cpu);
		CHECK_EMPTY();
		TEST_RESULT(!d1->Queued(), "");
		QUEUE_IDFC(d1, mode, TRUE);
		TEST_RESULT(d1->Queued(), "");
		QUEUE_IDFC(d1, mode, FALSE);
		TEST_RESULT(d1->Queued(), "");
		QUEUE_IDFC(d2, mode, TRUE);
		QUEUE_IDFC(d3, mode, TRUE);
		QUEUE_IDFC(d4, mode, TRUE);
		CHECK_EMPTY();
		pauser.Release();
		pauser.Pause(cpu);
		pauser.Release();
		CHECK_FIRST_ENTRY(cpu, NKern::EIDFC, 0, 1);
		CHECK_FIRST_ENTRY(cpu, NKern::EIDFC, 0, 2);
		CHECK_FIRST_ENTRY(cpu, NKern::EIDFC, 0, 3);
		CHECK_FIRST_ENTRY(cpu, NKern::EIDFC, 0, 4);
		CHECK_EMPTY();
		pauser.Pause(cpu);
		CHECK_EMPTY();
		TEST_RESULT(!d1->Queued(), "");
		QUEUE_IDFC(d1, mode, TRUE);
		TEST_RESULT(d1->Queued(), "");
		QUEUE_IDFC(d2, mode, TRUE);
		QUEUE_IDFC(d3, mode, TRUE);
		QUEUE_IDFC(d4, mode, TRUE);
		TEST_RESULT(d1->Cancel(), "");
		TEST_RESULT(!d1->Queued(), "");
		TEST_RESULT(!d1->Cancel(), "");
		CHECK_EMPTY();
		pauser.Release();
		pauser.Pause(cpu);
		pauser.Release();
		CHECK_FIRST_ENTRY(cpu, NKern::EIDFC, 0, 2);
		CHECK_FIRST_ENTRY(cpu, NKern::EIDFC, 0, 3);
		CHECK_FIRST_ENTRY(cpu, NKern::EIDFC, 0, 4);
		CHECK_EMPTY();
		pauser.Pause(cpu);
		CHECK_EMPTY();
		TEST_RESULT(!d1->Queued(), "");
		QUEUE_IDFC(d1, mode, TRUE);
		TEST_RESULT(d1->Queued(), "");
		QUEUE_IDFC(d2, mode, TRUE);
		QUEUE_IDFC(d3, mode, TRUE);
		QUEUE_IDFC(d4, mode, TRUE);
		TEST_RESULT(d1->Cancel(), "");
		TEST_RESULT(!d1->Queued(), "");
		TEST_RESULT(d4->Cancel(), "");
		TEST_RESULT(d2->Cancel(), "");
		TEST_RESULT(d3->Cancel(), "");
		CHECK_EMPTY();
		pauser.Release();
		pauser.Pause(cpu);
		pauser.Release();
		CHECK_EMPTY();
		}

	delete d4;
	delete d3;
	delete d2;
	delete d1;
	}

void DoIdleDFCTest1(TInt aCpu)
	{
#ifdef __SMP__
	TEST_PRINT2("IdleDFCTest1 CPU %d (%08x)", aCpu, TheScheduler.iCpusNotIdle);
#else
	TEST_PRINT1("IdleDFCTest1 CPU %d (%08x)", aCpu);
#endif
//	TInt num_cpus = NKern::NumberOfCpus();
	TInt this_cpu = NKern::CurrentCpu();
	TBool same_cpu = (aCpu==this_cpu);
	TDfcQue* q = 0;
	TPauseDFC* pauser = 0;
	if (!same_cpu)
		{
		q = CreateDfcQ("DfcQ3", 1, aCpu);
		TEST_OOM(q);
		pauser = new TPauseDFC(q);
		TEST_OOM(pauser);
		}

	TTestDfc* d1 = new TTestDfc(1);
	TEST_OOM(d1);
	TEST_RESULT(d1->IsIDFC(), "");
	TTestDfc* d2 = new TTestDfc(2);
	TEST_OOM(d2);
	TEST_RESULT(d2->IsIDFC(), "");
	TTestDfc* d3 = new TTestDfc(3);
	TEST_OOM(d3);
	TEST_RESULT(d3->IsIDFC(), "");
	TTestDfc* d4 = new TTestDfc(4);
	TEST_OOM(d4);
	TEST_RESULT(d4->IsIDFC(), "");

	TEST_RESULT(!d1->Queued(), "");
	TEST_RESULT(d1->QueueOnIdle(), "");
	TEST_RESULT(d1->Queued(), "");
	TEST_RESULT(!d1->QueueOnIdle(), "");
	CHECK_EMPTY();
	if (pauser)
		pauser->BusyPause();
	NKern::Sleep(1);
	if (pauser)
		TEST_RESULT(d1->Queued(), "");
	else
		{
		TEST_RESULT(!d1->Queued(), "");
		CHECK_FIRST_ENTRY(this_cpu, NKern::EIDFC, 0, 1);
		}
	CHECK_EMPTY();
	TBool ret = d1->QueueOnIdle();
	TEST_RESULT(pauser?!ret:ret, "");
	TEST_RESULT(d1->Queued(), "");
	TEST_RESULT(d1->Cancel(), "");
	TEST_RESULT(!d1->Queued(), "");
	CHECK_EMPTY();
	NKern::Sleep(1);
	CHECK_EMPTY();
	if (pauser)
		pauser->Release();
	TEST_RESULT(d4->QueueOnIdle(), "");
	TEST_RESULT(d3->QueueOnIdle(), "");
	TEST_RESULT(d1->QueueOnIdle(), "");
	TEST_RESULT(d2->QueueOnIdle(), "");
	TEST_RESULT(d3->Cancel(), "");
	CHECK_EMPTY();
	TInt xcpu = this_cpu;
	if (pauser)
		{
		xcpu = aCpu;
		pauser->Pause(1);
		pauser->Release();
		}
	else
		NKern::Sleep(1);
	CHECK_FIRST_ENTRY(xcpu, NKern::EIDFC, 0, 4);
	CHECK_FIRST_ENTRY(xcpu, NKern::EIDFC, 0, 1);
	CHECK_FIRST_ENTRY(xcpu, NKern::EIDFC, 0, 2);
	CHECK_EMPTY();

	delete d4;
	delete d3;
	delete d2;
	delete d1;
	delete pauser;
	if (q)
		DestroyDfcQ(q);
	}

TDfc* Dfcs[6];
NFastSemaphore* IdleDFCTest2Fs;

void IdleDFCTest2Fn(TAny* a)
	{
	TUint32 id = (TUint32)a;
	if (id==1)
		{
		TEST_RESULT(Dfcs[1]->Cancel(), "");
		TEST_RESULT(Dfcs[3]->QueueOnIdle(), "");
		TEST_RESULT(Dfcs[4]->QueueOnIdle(), "");
		TEST_RESULT(Dfcs[5]->QueueOnIdle(), "");
		}
	if (id==1 || id==4)
		IdleDFCTest2Fs->Signal();
	if (id==3)
		{
		TEST_RESULT(Dfcs[5]->Cancel(), "");
		}
	TTestDfc::Run(a);
	}

void DoIdleDFCTest2()
	{
	TEST_PRINT("IdleDFCTest2");
	NFastSemaphore sem(0);
	TInt this_cpu = NKern::CurrentCpu();
	TInt i;
	for (i=0; i<6; ++i)
		{
		Dfcs[i] = new TDfc(&IdleDFCTest2Fn, (TAny*)(i+1));
		TEST_OOM(Dfcs[i]);
		}
	TEST_RESULT(Dfcs[0]->QueueOnIdle(), "");
	TEST_RESULT(Dfcs[1]->QueueOnIdle(), "");
	TEST_RESULT(Dfcs[2]->QueueOnIdle(), "");
	IdleDFCTest2Fs = &sem;
	CHECK_EMPTY();
	NKern::FSWait(&sem);
	CHECK_FIRST_ENTRY(this_cpu, NKern::EIDFC, 0, 1);
	CHECK_FIRST_ENTRY(this_cpu, NKern::EIDFC, 0, 3);
	CHECK_EMPTY();
	NKern::FSWait(&sem);
	CHECK_FIRST_ENTRY(this_cpu, NKern::EIDFC, 0, 4);
	CHECK_FIRST_ENTRY(this_cpu, NKern::EIDFC, 0, 5);
	CHECK_EMPTY();
	for (i=0; i<6; ++i)
		delete Dfcs[i];
	}

#ifdef __SMP__

class TDfcStress;
class TDfcX
	{
public:
	enum
		{
		EFlag_IdleDFC=1,
		EFlag_IDFC=2,
		EFlag_DFC=4,
		EFlag_Timer=8,
		EFlag_Tied=16
		};
public:
	TDfcX();
	~TDfcX();
	static void IDfcFn(TAny*);
	static void DfcFn(TAny*);
	static void TimerIsrFn(TAny*);
	static void TimerDfcFn(TAny*);
	void Update();
	TBool Add(TAny* a=0);
	TBool Cancel(TAny* a=0);
	TBool Enque(TAny* a=0);
	TBool QueueOnIdle(TAny* a=0);
	TBool SafeAdd();
	TBool SafeCancel();
	TBool SafeEnque();
	TBool SafeQueueOnIdle();
	void CreateDfcOrTimer();
	void GetDesc(char* aDesc);
	inline TBool IsIDFC()
		{return (iFlags & (EFlag_IDFC|EFlag_Timer)) == EFlag_IDFC;}
	inline TBool IsIdler()
		{return iFlags & EFlag_IdleDFC;}
	void ThreadActivity();
public:
	union
		{
		TDfc* volatile iDfc;
		NTimer* volatile iTimer;
		};
	TDfcQue* iDfcQ;
	TUint32 iQueueCount;
	TUint32 iRunCount[KMaxCpus];
	TUint32 iCancelCount;
	TUint32 iExclusionFailCount;
	TUint32 iId;
	TUint32 iFlags;
	TDfcStress* iS;
	TUint64 iTimeQueued;
	TUint64 iMaxTime;
	TUint64 iSumTime;
	TSpinLock iSpinLock;
	NSchedulable* iXTied;
	volatile TUint32* iExclusionCheck;
	};

TDfcX::TDfcX()
	: iSpinLock(TSpinLock::EOrderGenericIrqLow1)
	{
	memclr(this,sizeof(TDfcX));
	new (&iSpinLock) TSpinLock(TSpinLock::EOrderGenericIrqLow1);
	}

TDfcX::~TDfcX()
	{
	TAny* p = __e32_atomic_swp_ord_ptr(&iDfc, 0);
	if (p)
		{
		if (iFlags & EFlag_Timer)
			delete ((NTimer*)p);
		else
			delete ((TDfc*)p);
		}
	}

class TDfcStress
	{
public:
	enum
		{
		EMode_Wait			=0x00000001u,
		EMode_AllowCancel	=0x00000002u,
		EMode_AllowIdle		=0x00000004u,
		EMode_AllowEnque	=0x00000008u,
		EMode_Recycle		=0x00000010u,
		EMode_UseTied		=0x00000020u,
		EMode_Migrate		=0x00000040u,
		EMode_SelfMigrate	=0x00000080u,
		EMode_Exit			=0x80000000u
		};
public:
	enum {EMaxDfc=48, EMaxDfcQ=8};

	TDfcStress();
	static TDfcStress* New(TInt aNumDfc, TInt aNumDfcQ, TBool aTimerTest);
	void Create();
	static void DoThreadFn(TAny*);
	void ThreadFn();
	static void BackStopFn(TAny*);
	void Run();
	void Close();
	void DoTestPhase(TInt aMode, TInt aTime, TInt aCount);
	void GetModeText(char* aName);
public:
	TDfcX* NewDfc(TUint32 aId, TUint32 aFlags, TInt aDfcQ);
	TDfcX* NewIDfc(TUint32 aId, TUint32 aFlags, NSchedulable* aTied=0);
	TDfcX* NewTimer(TUint32 aId, TUint32 aFlags, TInt aDfcQ, NSchedulable* aTied);
	void CreateDfc(TUint32 aId);
public:
	TInt iNumDfc;
	TDfcX* iDfcX[EMaxDfc];
	TInt iNumDfcQ;
	TBool iTimerTest;
	TDfcQue* iDfcQ[EMaxDfcQ];
	NThread* iThread[KMaxCpus];
	volatile TBool iStop;
	volatile TInt iRunning;
	volatile TInt iMode;
	NFastSemaphore* iExitSem;
	TDfcX* volatile iGarbage;
	TUint32 iRandomTimeLimit;
	TDfc* iBackStopIdleDfc;
	TDfcQue* iBackStopIdleDfcQ;
	};

void TDfcX::Update()
	{
	TUint32 exc0 = 0;
	TUint32 exc1 = 0;
	if (iExclusionCheck)
		exc0 = *iExclusionCheck;
	TInt cpu = NKern::CurrentCpu();
	__e32_atomic_add_ord32(&iRunCount[cpu], 1);
	TInt ctx = NKern::CurrentContext();
	TBool is_idfc = IsIDFC();
	TBool is_timer = iFlags & EFlag_Timer;
	TBool is_tied = iFlags & EFlag_Tied;
	if ((is_idfc||is_timer) && is_tied && !(iS->iMode & (TDfcStress::EMode_Migrate|TDfcStress::EMode_SelfMigrate)))
		{
		TInt cpu = NKern::CurrentCpu();
		TInt xcpu = iXTied->iCpuAffinity;
		if (cpu != xcpu)
			{
			__crash();
			}
		}
	TInt irq=0;
	if ((ctx!=NKern::EThread) && (iS->iMode & TDfcStress::EMode_AllowCancel))
		irq = iSpinLock.LockIrqSave();
	TUint64 now = fast_counter();
	TUint64 delta = now - iTimeQueued;
	if (TInt64(delta)>=0)
		{
		if (delta > iMaxTime)
			iMaxTime = delta;
		iSumTime += delta;
		}
	if ((ctx!=NKern::EThread) && (iS->iMode & TDfcStress::EMode_AllowCancel))
		iSpinLock.UnlockIrqRestore(irq);
	if (IsIdler())
		{
		TInt i;
		NKern::Lock();
		for (i=0; i<KMaxCpus; ++i)
			{
			NThread* t = iS->iThread[i];
			if (t)
				t->iRequestSemaphore.Reset();
			}
		NKern::Unlock();
		iS->iBackStopIdleDfc->Cancel();
		}
	if (iExclusionCheck)
		exc1 = *iExclusionCheck;
	if (exc0!=exc1)
		__e32_atomic_add_ord32(&iExclusionFailCount, 1);
	}

void TDfcStress::BackStopFn(TAny* a)
	{
	TDfcStress* s = (TDfcStress*)a;
	TInt i;
	NKern::Lock();
	for (i=0; i<KMaxCpus; ++i)
		{
		NThread* t = s->iThread[i];
		if (t)
			t->iRequestSemaphore.Reset();
		}
	NKern::Unlock();
	}

void TDfcX::IDfcFn(TAny* a)
	{
	TDfcX* d = (TDfcX*)a;
	d->Update();
	}

void TDfcX::DfcFn(TAny* a)
	{
	TDfcX* d = (TDfcX*)a;
	d->ThreadActivity();
	d->Update();
	d->ThreadActivity();
	}

void TDfcX::TimerDfcFn(TAny* a)
	{
	TDfcX* d = (TDfcX*)a;
	d->ThreadActivity();
	d->Update();
	d->ThreadActivity();
	}

void TDfcX::TimerIsrFn(TAny* a)
	{
	TDfcX* d = (TDfcX*)a;
	d->Update();
	}

void TDfcX::ThreadActivity()
	{
	TInt ncpus = NKern::NumberOfCpus();
	TInt ocpu = NKern::CurrentCpu();
	NThread* pC = NKern::CurrentThread();
	volatile TUint32* pX = (volatile TUint32*)&pC->iRunCount.i32[1];	// HACK!
	TInt cpu = ocpu;
	TInt i;
	if ((iS->iMode & TDfcStress::EMode_SelfMigrate) && !pC->iEvents.IsEmpty())
		{
		for (i=0; i<ncpus; ++i)
			{
			++*pX;
			if (++cpu == ncpus)
				cpu = 0;
			NKern::ThreadSetCpuAffinity(pC, cpu);
			}
		}
	else
		{
		++*pX;
		++*pX;
		++*pX;
		++*pX;
		++*pX;
		++*pX;
		++*pX;
		++*pX;
		}
	++*pX;
	++*pX;
	++*pX;
	}

TBool TDfcX::Add(TAny* a)
	{
	TBool is_timer = iFlags & EFlag_Timer;
	if (!a)
		a = iDfc;
	TUint64 time = fast_counter();
	TBool ok;
	if (is_timer)
		ok = ((NTimer*)a)->OneShot(1) == KErrNone;
	else
		ok = ((TDfc*)a)->Add();
	if (ok)
		{
		iTimeQueued = time;
		__e32_atomic_add_ord32(&iQueueCount, 1);
		}
	return ok;
	}

TBool TDfcX::Cancel(TAny* a)
	{
	TBool is_timer = iFlags & EFlag_Timer;
	if (!a)
		a = iDfc;
	TBool ok;
	if (is_timer)
		ok = ((NTimer*)a)->Cancel();
	else
		ok = ((TDfc*)a)->Cancel();
	if (ok)
		__e32_atomic_add_ord32(&iCancelCount, 1);
	return ok;
	}

TBool TDfcX::Enque(TAny* a)
	{
	TBool is_timer = iFlags & EFlag_Timer;
	if (!a)
		a = iDfc;
	TUint64 time = fast_counter();
	TBool ok;
	if (is_timer)
		ok = ((NTimer*)a)->Again(2) == KErrNone;
	else
		ok = ((TDfc*)a)->Enque();
	if (ok)
		{
		iTimeQueued = time;
		__e32_atomic_add_ord32(&iQueueCount, 1);
		}
	return ok;
	}

TBool TDfcX::QueueOnIdle(TAny* a)
	{
	TBool is_timer = iFlags & EFlag_Timer;
	if (is_timer)
		return FALSE;
	if (!a)
		a = iDfc;
	TUint64 time = fast_counter();
	TBool ok = ((TDfc*)a)->QueueOnIdle();
	if (ok)
		{
		iTimeQueued = time;
		__e32_atomic_add_ord32(&iQueueCount, 1);
		}
	return ok;
	}

TBool TDfcX::SafeAdd()
	{
	TBool ret = FALSE;
	TUint32 x = set_bit0_if_nonnull((TUint32&)iDfc);
	if (x && !(x&1))
		{
		TDfc* p = (TDfc*)x;
		ret = Add(p);
		flip_bit0((TUint32&)iDfc);
		}
	return ret;
	}

TBool TDfcX::SafeEnque()
	{
	TBool ret = FALSE;
	TUint32 x = set_bit0_if_nonnull((TUint32&)iDfc);
	if (x && !(x&1))
		{
		TDfc* p = (TDfc*)x;
		ret = Enque(p);
		flip_bit0((TUint32&)iDfc);
		}
	return ret;
	}

TBool TDfcX::SafeQueueOnIdle()
	{
	TBool ret = FALSE;
	TUint32 x = set_bit0_if_nonnull((TUint32&)iDfc);
	if (x && !(x&1))
		{
		TDfc* p = (TDfc*)x;
		ret = QueueOnIdle(p);
		flip_bit0((TUint32&)iDfc);
		}
	return ret;
	}

TBool TDfcX::SafeCancel()
	{
	TBool ret = FALSE;
	TUint32 x = swap_out_if_bit0_clear((TUint32&)iDfc);
	if (x && !(x&1))
		{
		if (iFlags & EFlag_Timer)
			{
			NTimer* p = (NTimer*)x;
			ret = Cancel(p);
			p->~NTimer();
			memset(p, 0xbb, sizeof(NTimer));
			free(p);
			}
		else
			{
			TDfc* p = (TDfc*)x;
			ret = Cancel(p);
			p->~TDfc();
			memset(p, 0xbb, sizeof(TDfc));
			free(p);
			}
		CreateDfcOrTimer();
		}
	return ret;
	}

void TDfcX::GetDesc(char* a)
	{
	memset(a, 0x20, 8);
	if (iFlags & EFlag_Timer)
		*a++ = 'T';
	if (iFlags & EFlag_IDFC)
		*a++ = 'I';
	if (iFlags & EFlag_DFC)
		*a++ = 'D';
	if (iFlags & EFlag_IdleDFC)
		*a++ = 'i';
	if (iFlags & EFlag_Tied)
		*a++ = 't';
	}

TDfcStress::TDfcStress()
	{
	memclr(this, sizeof(*this));
	}

TDfcX* TDfcStress::NewDfc(TUint32 aId, TUint32 aFlags, TInt aDfcQ)
	{
	TDfcX* d = new TDfcX;
	TEST_OOM(d);
	d->iId = aId;
	d->iFlags = aFlags;
	d->iS = this;

	d->iDfcQ = iDfcQ[aDfcQ];
	d->CreateDfcOrTimer();
	return d;
	}

TDfcX* TDfcStress::NewIDfc(TUint32 aId, TUint32 aFlags, NSchedulable* aTied)
	{
	TDfcX* d = new TDfcX;
	TEST_OOM(d);
	d->iId = aId;
	d->iFlags = aFlags;
	d->iS = this;

	d->iXTied = aTied;
	d->CreateDfcOrTimer();
	return d;
	}

TDfcX* TDfcStress::NewTimer(TUint32 aId, TUint32 aFlags, TInt aDfcQ, NSchedulable* aTied)
	{
	TDfcX* d = new TDfcX;
	TEST_OOM(d);
	d->iId = aId;
	d->iFlags = aFlags;
	d->iS = this;

	d->iDfcQ = (aFlags & TDfcX::EFlag_DFC) ? iDfcQ[aDfcQ] : 0;
	d->iXTied = (aFlags & TDfcX::EFlag_Tied) ? aTied : 0;
	d->CreateDfcOrTimer();
	return d;
	}


void TDfcX::CreateDfcOrTimer()
	{
//	volatile TUint32* xc = 0;
	NThreadBase* t = iS->iDfcQ[0]->iThread;
	volatile TUint32* xc = &t->iRunCount.i32[1];	// HACK!
	if (!(iFlags & EFlag_Timer))
		{
		TDfc* d = 0;
		if (!(iFlags & EFlag_IDFC))
			{
			d = new TDfc(&TDfcX::DfcFn, this, iDfcQ, 1);
			xc = (volatile TUint32*)&iDfcQ->iThread->iRunCount.i32[1];
			}
		else if (iFlags & EFlag_Tied)
			{
			d = new TDfc(iXTied, &TDfcX::IDfcFn, this);
			xc = (volatile TUint32*)&iXTied->iRunCount.i32[1];
			}
		else
			d = new TDfc(&TDfcX::IDfcFn, this);
		__NK_ASSERT_ALWAYS(d!=0);
		__e32_atomic_store_rel_ptr(&iDfc, d);
		}
	else
		{
		NTimer* tmr = 0;
		if (iFlags & EFlag_DFC)
			{
			tmr = new NTimer(&TDfcX::TimerDfcFn, this, iDfcQ, 1);
			xc = (volatile TUint32*)&iDfcQ->iThread->iRunCount.i32[1];
			}
		else if (iFlags & EFlag_Tied)
			{
			tmr = new NTimer(iXTied, &TDfcX::TimerIsrFn, this);
			xc = (volatile TUint32*)&iXTied->iRunCount.i32[1];
			}
		else
			tmr = new NTimer(&TDfcX::TimerIsrFn, this);
		__NK_ASSERT_ALWAYS(tmr!=0);
		__e32_atomic_store_rel_ptr(&iTimer, tmr);
		}
	iExclusionCheck = xc;
	}

TDfcStress* TDfcStress::New(TInt aNumDfc, TInt aNumDfcQ, TBool aTimerTest)
	{
	TDfcStress* p = new TDfcStress;
	TEST_OOM(p);
	p->iTimerTest = aTimerTest;
	p->iNumDfc = aNumDfc;
	p->iNumDfcQ = aNumDfcQ;
	p->Create();
	return p;
	}

void TDfcStress::Create()
	{
DEBUGPRINT("TDfcStress @ %08x", this);
	TInt i;
	TInt num_cpus = NKern::NumberOfCpus();
	TInt cpu = 0;
	iExitSem = new NFastSemaphore(0);
	TEST_OOM(iExitSem);
	for (i=0; i<iNumDfcQ; ++i)
		{
		char c[8] = "DFCQ*";
		c[4] = (char)('0'+i);
		TDfcQue* q = CreateDfcQ(c, 32, cpu);
		TEST_OOM(q);
		iDfcQ[i] = q;
		if (++cpu == num_cpus)
			cpu = 0;
		NThreadBase* t = q->iThread;
DEBUGPRINT("DfcQ %2d @ %08x Thread @%08x Stack %08x+%08x", i, iDfcQ[i], t, t->iStackBase, t->iStackSize);
		}
	iBackStopIdleDfcQ = CreateDfcQ("BackStop", 1, 0);
	TEST_OOM(iBackStopIdleDfcQ);
	iBackStopIdleDfc = new TDfc(&BackStopFn, this, iBackStopIdleDfcQ, 1);
	TEST_OOM(iBackStopIdleDfc);
	for (i=0; i<num_cpus; ++i)
		{
		char c[8] = "THRD*";
		c[4] = (char)('0'+i);
		NThread* t = CreateUnresumedThreadSignalOnExit(c, &DoThreadFn, 11, this, 0, -1, iExitSem, i);
		TEST_OOM(t);
		iThread[i] = t;
DEBUGPRINT("Thread %2d @ %08x (Stack %08x+%08x)", i, iThread[i], t->iStackBase, t->iStackSize);
		}
	for (i=0; i<iNumDfc; ++i)
		{
		CreateDfc(i);
DEBUGPRINT("DfcX %2d @ %08x (DFC @ %08x)", i, iDfcX[i], iDfcX[i]->iDfc);
		}
	}

void TDfcStress::CreateDfc(TUint32 aId)
	{
	TUint32 type = aId & 7;
	TUint32 q = aId % iNumDfcQ;
	TDfcX* d = 0;
	switch (type)
		{
		case 0:
		case 1:
		case 2:
		case 3:
			if (iTimerTest)
				d = NewTimer(aId, TDfcX::EFlag_Timer|TDfcX::EFlag_DFC, q, 0);
			else
				d = NewDfc(aId, TDfcX::EFlag_DFC, q);
			break;
		case 4:
		case 5:
			if (iTimerTest)
				d = NewTimer(aId, TDfcX::EFlag_Timer|TDfcX::EFlag_Tied, 0, iDfcQ[iNumDfcQ-1-(type&1)]->iThread);
			else
				{
				if (aId>=16 && aId<32 && iNumDfcQ>2)
					d = NewIDfc(aId, TDfcX::EFlag_IDFC|TDfcX::EFlag_Tied, iDfcQ[2]->iThread);
				else
					d = NewIDfc(aId, TDfcX::EFlag_IDFC);
				}
			break;
		case 6:
		case 7:
			if (iTimerTest)
				d = NewTimer(aId, TDfcX::EFlag_Timer, 0, 0);
			else
				d = NewDfc(aId, TDfcX::EFlag_DFC|TDfcX::EFlag_IdleDFC, q);
			break;
		};
	__e32_atomic_store_rel_ptr(&iDfcX[aId], d);
	}

void TDfcStress::Close()
	{
	TInt i;

	// delete DFCs before the DFC queues they might be on
	for (i=0; i<iNumDfc; ++i)
		{
		TDfcX* d = iDfcX[i];
		delete d;
		}
	delete iBackStopIdleDfc;

	for (i=0; i<iNumDfcQ; ++i)
		DestroyDfcQ(iDfcQ[i]);
	DestroyDfcQ(iBackStopIdleDfcQ);

	delete iExitSem;
	delete this;
	}

void TDfcStress::DoThreadFn(TAny* a)
	{
	((TDfcStress*)a)->ThreadFn();
	}

void append(char*& a, const char* s)
	{
	while(*s)
		*a++ = *s++;
	*a=0;
	}

void TDfcStress::GetModeText(char* a)
	{
	memclr(a,128);
	if (iMode==0)
		{
		append(a, "Add only");
		return;
		}
	if (iMode & EMode_Wait)
		append(a, "Wait ");
	if (iMode & EMode_AllowCancel)
		append(a, "Cancel ");
	if (iMode & EMode_AllowIdle)
		append(a, "Idle ");
	if (iMode & EMode_AllowEnque)
		append(a, "Enque ");
	if (iMode & EMode_Recycle)
		append(a, "Recycle ");
	if (iMode & EMode_Migrate)
		append(a, "Migrate ");
	if (iMode & EMode_SelfMigrate)
		append(a, "SelfMigrate ");
	}

/*
Test Mode:

Bit 31	If set causes thread to exit
Bit 0	If set does random wait after each operation
Bit 1	Allows Cancel operations if set
Bit 2	Allows idle operations if set
Bit 3	Test Enque() as well as Add()
Bit 4	Use SafeXXX operations
Bit 5	Use tied IDFCs
Bit 6	Migrate threads with things tied to them
Bit 7	Threads with things tied to them migrate themselves during execution

*/
void TDfcStress::ThreadFn()
	{
	TBool finish = FALSE;
	TUint32 seed[2];
	seed[0] = NKern::CurrentCpu() ^ 0xddb3d743;
	seed[1] = 0;
	FOREVER
		{
		if (iStop)
			{
			__e32_atomic_add_ord32(&iRunning, (TUint32)(-1));
			while (iStop)
				{
				if (iMode<0)
					{
					finish = TRUE;
					break;
					}
				}
			if (finish)
				break;
			else
				__e32_atomic_add_ord32(&iRunning, 1);
			}
		if (iMode & EMode_Wait)
			{
			TUint32 wait = random(seed);
			wait %= iRandomTimeLimit;
			wait += 1;
			fcfspin(wait);
			}
		TUint32 action = random(seed);
		TUint32 action2 = random(seed);
		if (action & 0xff000000)
			{
			// queue or cancel a DFC or timer
			TBool cancel = action2 & 2;
			TUint32 id = action % iNumDfc;
			TDfcX* d = iDfcX[id];
			if (iMode & EMode_Recycle)
				{
				TBool isIDFC = d->IsIDFC();
				TBool isIdler = d->IsIdler();
				if (cancel)
					d->SafeCancel();
				else if (isIdler)
					d->SafeQueueOnIdle();
				else if ((iMode & EMode_AllowEnque) && (action2 & 1) && !isIDFC)
					d->SafeEnque();
				else
					{
					NKern::Lock();
					d->SafeAdd();
					NKern::Unlock();
					}
				}
			else
				{
				if (cancel && (iMode & EMode_AllowCancel))
					{
					d->Cancel();
					}
				else if (!d->IsIdler())
					{
					if ((iMode & EMode_AllowEnque) && (action2 & 1) && !d->IsIDFC())
						{
						d->Enque();
						}
					else
						{
						NKern::Lock();
						d->Add();
						NKern::Unlock();
						}
					}
				else
					{
					d->QueueOnIdle();
					}
				}
			continue;
			}
		if (iMode & EMode_AllowIdle)
			{
			iBackStopIdleDfc->QueueOnIdle();
			NKern::WaitForAnyRequest();
			}
		}
	}

void StopTimeout(TAny*)
	{
	__crash();
	}

void TDfcStress::DoTestPhase(TInt aMode, TInt aTime, TInt aCount)
	{
	char mode_text[128];
	TInt i;
	TUint32 maxavg = 0;
	TInt n;
	iMode = aMode;
	iStop = FALSE;
	GetModeText(mode_text);
	TEST_PRINT1("Testing with: %s", mode_text);
	for (i=0; i<aCount; ++i)
		{
		NKern::Sleep(aTime);
		DebugPrint(".",1);
		}
	DebugPrint("\r\n",2);
	TEST_PRINT("Stopping ...");
	iStop = TRUE;
	NTimer timer(&StopTimeout, 0);
	timer.OneShot(2000);
	BackStopFn(this);
	n = 0;
	while (iRunning && ++n<=100)
		NKern::Sleep(10);
	if (iRunning)
		{
		__crash();
		}
	iBackStopIdleDfc->Cancel();
	timer.Cancel();
	TEST_PRINT("Threads stopped");
	for (i=0; i<iNumDfcQ; ++i)
		{
		TUint32 ev = iDfcQ[i]->iThread->iEventState;
		DEBUGPRINT("DfcThread %d EventState = %08x", i, ev);
		TEST_RESULT(!(ev & NSchedulable::EEventCountMask), "");
		}
	for (i=0; i<NKern::NumberOfCpus(); ++i)
		{
		TUint32 ev = iThread[i]->iEventState;
		DEBUGPRINT("Thread    %d EventState = %08x", i, ev);
		TEST_RESULT(!(ev & NSchedulable::EEventCountMask), "");
		}
	NKern::Sleep(10);
	for (i=0; i<iNumDfc; ++i)
		{
		TDfcX* d = iDfcX[i];
		d->Cancel();
		TUint32 qc = d->iQueueCount;
		TUint32* rc = d->iRunCount;
		TUint32 totrc = rc[0] + rc[1] + rc[2] + rc[3] + rc[4] + rc[5] + rc[6] + rc[7];
		TUint32 cc = d->iCancelCount;
		TUint32 f = d->iFlags;
//		TUint32 imm = d->IsIDFC()?1:0;
		TUint32 max = d->iMaxTime;
		TUint32 avg = 0;
		TUint32 xfc = d->iExclusionFailCount;
		if (totrc)
			avg = TUint32(d->iSumTime / TUint64(totrc));
		if (avg > maxavg)
			maxavg = avg;
		char desc[16];
		memclr(desc,16);
		d->GetDesc(desc);
		DEBUGPRINT("%2d: %s QC %9d RC %9d CC %9d MAX %9d AVG %9d XFC %9d RC %9d %9d %9d %9d %9d %9d %9d %9d", i, desc, qc, totrc, cc, max, avg, xfc, rc[0], rc[1], rc[2], rc[3], rc[4], rc[5], rc[6], rc[7]);
		TInt diff = (TInt)(qc - (totrc+cc));
		TEST_RESULT1(diff==0, "Counts mismatched, diff=%d", diff);
		TEST_RESULT(!(f&TDfcX::EFlag_Tied) || xfc==0, "Exclusion Failure!");
		d->iQueueCount = 0;
		memclr(d->iRunCount, sizeof(d->iRunCount));
		d->iCancelCount = 0;
		d->iMaxTime = 0;
		d->iSumTime = 0;
		}
	if (!iRandomTimeLimit)
		iRandomTimeLimit = maxavg + (maxavg>>1);
	}

void TDfcStress::Run()
	{
	TInt i;
	NThread* t;
	iStop = FALSE;
	iMode = 0;
	TInt num_cpus = NKern::NumberOfCpus();
	iRunning = num_cpus;
	for (i=0; i<KMaxCpus; ++i)
		{
		t = iThread[i];
		if (t)
			NKern::ThreadResume(t);
		}
	TEST_PRINT("Threads resumed");


	DoTestPhase(0x00, 10000, 1);
	if (iTimerTest)
		{
		const TInt N = 20;
		DoTestPhase(EMode_AllowCancel|EMode_AllowEnque|EMode_Recycle, 10000, N);
		DoTestPhase(EMode_AllowCancel|EMode_AllowEnque|EMode_Wait|EMode_Recycle, 10000, N);
		DoTestPhase(EMode_AllowCancel|EMode_AllowEnque|EMode_Recycle|EMode_SelfMigrate, 10000, N);
		DoTestPhase(EMode_AllowCancel|EMode_AllowEnque|EMode_Wait|EMode_Recycle|EMode_SelfMigrate, 10000, N);
		DoTestPhase(EMode_AllowCancel|EMode_AllowEnque, 10000, N);
		DoTestPhase(EMode_AllowCancel|EMode_AllowEnque|EMode_Wait, 10000, N);
		DoTestPhase(EMode_AllowCancel|EMode_AllowEnque|EMode_SelfMigrate, 10000, N);
		DoTestPhase(EMode_AllowCancel|EMode_AllowEnque|EMode_Wait|EMode_SelfMigrate, 10000, N);
		}
	else
		{
		DoTestPhase(EMode_AllowCancel|EMode_AllowEnque, 10000, 20);
		DoTestPhase(EMode_AllowCancel|EMode_AllowIdle|EMode_AllowEnque, 10000, 20);
		DoTestPhase(EMode_AllowIdle|EMode_AllowEnque, 10000, 20);
		DoTestPhase(EMode_AllowCancel|EMode_AllowIdle, 10000, 20);
		DoTestPhase(EMode_AllowCancel|EMode_Wait, 10000, 20);
		DoTestPhase(EMode_AllowCancel, 10000, 20);
		DoTestPhase(EMode_AllowCancel|EMode_AllowIdle|EMode_AllowEnque|EMode_Recycle, 10000, 20);
		}

	iMode = EMode_Exit;
	TEST_PRINT("Terminating threads");
	for (i=0; i<num_cpus; ++i)
		NKern::FSWait(iExitSem);
	TEST_PRINT("Done");
	}

void DoStressTest(TBool aTimerTest)
	{
	TEST_PRINT("Stress test...");
	TInt ndfcs=0;
	TInt ndfcq=0;
	switch (NKern::NumberOfCpus())
		{
		case 1:	ndfcs=16; ndfcq=2; break;
		case 2:	ndfcs=16; ndfcq=2; break;
		case 3:	ndfcs=24; ndfcq=2; break;
		case 4:	ndfcs=32; ndfcq=3; break;
		case 5:	ndfcs=32; ndfcq=3; break;
		case 6:	ndfcs=48; ndfcq=4; break;
		case 7:	ndfcs=48; ndfcq=4; break;
		case 8:	ndfcs=48; ndfcq=4; break;
		default:
			__NK_ASSERT_ALWAYS(0);
			break;
		}
	TDfcStress* ds = TDfcStress::New(ndfcs, ndfcq, aTimerTest);
	ds->Run();
	ds->Close();
	}

#endif

void TestDFCs()
	{
	TEST_PRINT("Testing DFCs...");

	TTestDfc::Buffer = CircBuf::New(TTestDfc::EBufferSlots);
	TInt cpu;
	(void)cpu;
#ifdef __SMP__
	DoStressTest(TRUE);
#endif

	DoDFCTest1();
	DoDFCTest2();
	for_each_cpu(cpu)
		{
		DoDFCTest3(cpu);
		}
	DoIDFCTest1();
	for_each_cpu(cpu)
		{
		DoIdleDFCTest1(cpu);
		}
	DoIdleDFCTest2();

#ifdef __SMP__
	DoStressTest(FALSE);
#endif

	delete TTestDfc::Buffer;
	TTestDfc::Buffer = 0;
	}
