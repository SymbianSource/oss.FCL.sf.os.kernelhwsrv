// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\nkernsa\testipi.cpp
// 
//

#include <nktest/nkutils.h>

#ifdef __SMP__

class TTestIPI : public TGenericIPI
	{
public:
	TTestIPI();
	static TTestIPI* New();
	static void Isr(TGenericIPI*);
	void Inc(TInt aCpu, TInt aInc=1) {iExpected[aCpu] += aInc;}
	void IncM(TUint32 aMask);
	void Check();
public:
	TInt iCount[KMaxCpus];
	TInt iExpected[KMaxCpus];
	};

TTestIPI::TTestIPI()
	{
	memclr(iCount, sizeof(iCount));
	memclr(iExpected, sizeof(iExpected));
	}

TTestIPI* TTestIPI::New()
	{
	TTestIPI* p = new TTestIPI;
	TEST_OOM(p);
	return p;
	}

void TTestIPI::Isr(TGenericIPI* a)
	{
	TTestIPI& ipi = *(TTestIPI*)a;
	TInt cpu = NKern::CurrentCpu();
	++ipi.iCount[cpu];
	}

void TTestIPI::IncM(TUint32 aMask)
	{
	TInt i;
	for(i=0; i<KMaxCpus; ++i)
		{
		if (aMask & (1<<i))
			++iExpected[i];
		}
	}

void TTestIPI::Check()
	{
	TInt i;
	for(i=0; i<KMaxCpus; ++i)
		{
		TEST_RESULT3(iCount[i]==iExpected[i], "CPU %d Count %d Expected %d", i, iCount[i], iExpected[i]);
		}
	}

void IPITestThread1(TAny*)
	{
	TInt cpu;
	TUint32 all_cpus = ~(0xffffffffu << NKern::NumberOfCpus());

	for_each_cpu(cpu)
		{
		TEST_PRINT("ipi1");
		TTestIPI ipi;
		ipi.Queue(&TTestIPI::Isr, 1u<<cpu);
		ipi.WaitCompletion();
		ipi.Inc(cpu);
		ipi.Check();

		TEST_PRINT("ipi2");
		TTestIPI ipi2;
		TUint32 m = all_cpus & ~(1u<<cpu);
		ipi2.Queue(&TTestIPI::Isr, m);
		ipi2.WaitCompletion();
		ipi2.IncM(m);
		ipi2.Check();
		}

	TEST_PRINT("ipi3");
	TTestIPI ipi3;
	ipi3.QueueAll(&TTestIPI::Isr);
	ipi3.WaitCompletion();
	ipi3.IncM(all_cpus);
	ipi3.Check();

	TEST_PRINT("ipi4");
	TTestIPI ipi4;
	ipi4.QueueAllOther(&TTestIPI::Isr);
	ipi4.WaitCompletion();
	ipi4.IncM(all_cpus & ~(1u<<NKern::CurrentCpu()) );
	ipi4.Check();
	}

void DoIPITest1()
	{
	TInt cpu;
	for_each_cpu(cpu)
		{
		CreateThreadAndWaitForExit("IPITest1", &IPITestThread1, 12, 0, 0, -1, cpu);
		}
	}


class TTestIPI2 : public TGenericIPI
	{
public:
	TTestIPI2(TInt aId);
	static TTestIPI2* New(TInt aId);
	static void Isr(TGenericIPI*);
	static void Thread(TAny*);
public:
	TInt iId;
public:
	enum {EMaxIPI=32};
	enum {EBufSize=256};
	static volatile TInt NextPos;
	static volatile TUint32 Buffer[EBufSize];
	static volatile TUint32 PauseHere;
	};

volatile TInt TTestIPI2::NextPos;
volatile TUint32 TTestIPI2::Buffer[TTestIPI2::EBufSize];
volatile TUint32 TTestIPI2::PauseHere;

__ASSERT_COMPILE(TTestIPI2::EBufSize >= TTestIPI2::EMaxIPI*KMaxCpus);

TTestIPI2::TTestIPI2(TInt aId)
	: iId(aId)
	{
	}

TTestIPI2* TTestIPI2::New(TInt aId)
	{
	TTestIPI2* p = new TTestIPI2(aId);
	TEST_OOM(p);
	return p;
	}

void TTestIPI2::Isr(TGenericIPI* a)
	{
	TTestIPI2& ipi = *(TTestIPI2*)a;
	TUint32 cpu = NKern::CurrentCpu();
	TUint32 x = (cpu<<16) | ipi.iId;
	TInt pos = __e32_atomic_tas_ord32(&NextPos, EBufSize, 0, 1);
	if (pos < EBufSize)
		Buffer[pos] = x;
	while (PauseHere == x)
		{}
	}

void TTestIPI2::Thread(TAny*)
	{
	TTestIPI2* ipi[EMaxIPI];
	TUint32 all_cpus = ~(0xffffffffu << NKern::NumberOfCpus());
	TInt this_cpu = NKern::CurrentCpu();
	TInt pause_cpu = this_cpu + 1;
	if (pause_cpu >= NKern::NumberOfCpus())
		pause_cpu = 0;
	if (pause_cpu == this_cpu)
		pause_cpu = -1;
	TUint32 this_cpu_mask = 1u<<this_cpu;
	TUint32 pause_cpu_mask = (pause_cpu>=0) ? (1u<<pause_cpu) : 0;
	TUint32 other_cpus = all_cpus & ~(this_cpu_mask | pause_cpu_mask);
	TInt num_other_cpus = __e32_bit_count_32(other_cpus);
	TInt i;
	for(i=0; i<EMaxIPI; ++i)
		ipi[i] = New(i+1);

	NextPos = 0;
	PauseHere = 0;
	if (pause_cpu >= 0)
		PauseHere = (pause_cpu<<16) | (EMaxIPI/2);

	TEST_PRINT3("this_cpu=%d pause_cpu=%d PauseHere=%x", this_cpu, pause_cpu, PauseHere);

	TInt irq = NKern::DisableAllInterrupts();
	for (i=0; i<EMaxIPI; ++i)
		ipi[i]->QueueAll(&Isr);

	TInt expected1 = num_other_cpus*EMaxIPI + EMaxIPI/2;
	while (NextPos != expected1)
		{}

	PauseHere = 0;
	TInt expected2 = num_other_cpus*EMaxIPI + EMaxIPI;
	while (NextPos != expected2)
		{}
	NKern::RestoreInterrupts(irq);
	ipi[EMaxIPI-1]->WaitCompletion();

	for(i=0; i<NextPos; ++i)
		{
		TEST_PRINT1("%08x", Buffer[i]);
		}

	for(i=0; i<EMaxIPI; ++i)
		delete ipi[i];
	}

void DoIPITest2()
	{
	TInt cpu;
	for_each_cpu(cpu)
		{
		CreateThreadAndWaitForExit("IPITest2", &TTestIPI2::Thread, 12, 0, 0, -1, cpu);
		}
	}


void TestIPI()
	{
	TEST_PRINT("Testing generic IPIs...");

	DoIPITest1();
	DoIPITest2();
	}

#else	// __SMP__
void TestIPI()
	{
	}
#endif	// __SMP__
