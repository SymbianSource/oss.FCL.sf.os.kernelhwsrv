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
// e32test\nkernsa\benchmark.cpp
// 
//

#include <nktest/nkutils.h>

typedef void (*PFV)(void);

#ifdef __MARM__
extern "C" TUint32 __get_static_data();
extern "C" TUint32 __get_rwno_tid();
extern "C" TUint32 __cpu_id();
extern "C" TUint32 __trace_cpu_num();
#ifdef __SMP__
extern "C" TUint32 __get_local_timer_address();
extern "C" TUint32 __get_local_timer_count();
extern "C" TUint32 __set_local_timer_count();
extern "C" TUint32 __swp_local_timer_count();

void DoWatchdogTimerTest();
#endif
#endif

TUint32 Threshold;
TUint32 PsPerTick;

TUint32 Counter;
#ifdef __SMP__
TSpinLock SL1(TSpinLock::EOrderGenericIrqLow0);
TSpinLock SL2(TSpinLock::EOrderGenericPreLow0);
#endif

NThread* PingPongThread;
NFastSemaphore PingPongExitSem;
void PingPong(TAny* aPtr)
	{
	NThread* t = (NThread*)aPtr;
	FOREVER
		{
		NKern::WaitForAnyRequest();
		if (t)
			NKern::ThreadRequestSignal(t);
		}
	}

void SetupPingPongThread(TInt aPri, TInt aCpu, TBool aReply=TRUE)
	{
	NThread* t = NKern::CurrentThread();
	NKern::FSSetOwner(&PingPongExitSem, t);
	PingPongThread = CreateThreadSignalOnExit("PingPong", &PingPong, aPri, aReply?t:0, 0, -1, &PingPongExitSem, aCpu);
	}

void DestroyPingPongThread()
	{
	NKern::ThreadKill(PingPongThread);
	NKern::FSWait(&PingPongExitSem);
	}

extern "C" void dummy()
	{
	}

extern "C" void sleep1()
	{
	NKern::Sleep(1);
	}

extern "C" void do_atomic_add_rlx32()
	{
	__e32_atomic_add_rlx32(&Counter, 1);
	}

extern "C" void do_atomic_add_rel32()
	{
	__e32_atomic_add_rel32(&Counter, 1);
	}

extern "C" void do_atomic_add_acq32()
	{
	__e32_atomic_add_acq32(&Counter, 1);
	}

extern "C" void do_atomic_add_ord32()
	{
	__e32_atomic_add_ord32(&Counter, 1);
	}

extern "C" void dis_ena_int()
	{
	TInt irq = NKern::DisableAllInterrupts();
	NKern::RestoreInterrupts(irq);
	}

extern "C" void dis_ena_preempt()
	{
	NKern::Lock();
	NKern::Unlock();
	}

#ifdef __SMP__
extern "C" void sl1_lock_unlock_irq()
	{
	SL1.LockIrq();
	SL1.UnlockIrq();
	}

extern "C" void sl1_lock_unlock_irq_save()
	{
	TInt irq = SL1.LockIrqSave();
	SL1.UnlockIrqRestore(irq);
	}

extern "C" void sl1_lock_unlock_only()
	{
	TInt irq = NKern::DisableAllInterrupts();
	SL1.LockOnly();
	SL1.UnlockOnly();
	NKern::RestoreInterrupts(irq);
	}

extern "C" void sl2_lock_unlock_only()
	{
	NKern::Lock();
	SL2.LockOnly();
	SL2.UnlockOnly();
	NKern::Unlock();
	}
#endif

extern "C" void lock_unlock_system()
	{
	NKern::LockSystem();
	NKern::UnlockSystem();
	}

extern "C" void enter_leave_cs()
	{
	NKern::ThreadEnterCS();
	NKern::ThreadLeaveCS();
	}

extern "C" void resched_to_same()
	{
	NKern::Lock();
	RescheduleNeeded();
	NKern::Unlock();
	}

#ifdef __SMP__
extern "C" void get_timestamp()
	{
	NKern::Timestamp();
	}
#endif

NFastSemaphore Sem;

extern "C" void sem_signal()
	{
	NKern::FSSignal(&Sem);
	}

extern "C" void sem_signal_wait()
	{
	NKern::FSSignal(&Sem);
	NKern::FSWait(&Sem);
	}

TDfcQue* DfcQ;
TDfc* Dfc;

void BenchmarkDfcFn(TAny* aPtr)
	{
	NThread* t = (NThread*)aPtr;
	if (t)
		NKern::ThreadRequestSignal(t);
	}

void BenchmarkIDfcFn(TAny* aPtr)
	{
	NThread* t = (NThread*)aPtr;
	if (t)
		t->RequestSignal();
	}

void SetupBenchmarkDfcQ(TInt aPri, TInt aCpu, TBool aReply=TRUE)
	{
	NThread* t = NKern::CurrentThread();
	if (aPri>=0)
		{
		DfcQ = CreateDfcQ("Benchmark", aPri, (TUint32)aCpu);
		Dfc = new TDfc(&BenchmarkDfcFn, aReply?t:0, DfcQ, 1);
		}
	else
		Dfc = new TDfc(&BenchmarkIDfcFn, aReply?t:0);
	}

void DestroyBenchmarkDfcQ()
	{
	Dfc->Cancel();
	delete Dfc;
	if (DfcQ)
		DestroyDfcQ(DfcQ);
	Dfc = 0;
	DfcQ = 0;
	}




TUint32 Ps(TUint64 x)
	{
	x*=PsPerTick;
	if (x>>32)
		return KMaxTUint32;
	return (TUint32)x;
	}

TUint64 Iterate(PFV f, TUint32 aCount)
	{
	TUint64 initial = fast_counter();
	do	{
		(*f)();
		--aCount;
		} while(aCount);
	TUint64 final = fast_counter();
	return final - initial;
	}

TUint32 Measure(PFV f)
	{
	TUint32 n = 1;
	TUint64 time;
	do	{
		n<<=1;
		time = Iterate(f, n);
		} while(time < Threshold);
	time *= PsPerTick;
	time /= n;
	if (time >> 32)
		return KMaxTUint32;
	return (TUint32)time;
	}

void ping_pong_threads()
	{
	NKern::ThreadRequestSignal(PingPongThread);
	NKern::WaitForAnyRequest();
	}

void ping_pong_threads_nr()
	{
	NKern::ThreadRequestSignal(PingPongThread);
	}

TUint32 DoPingPongTest(TInt aPri, TInt aCpu, TBool aReply=TRUE)
	{
	SetupPingPongThread(aPri, aCpu, aReply);
	TUint32 x;
	if (aReply)
		x = Measure(&ping_pong_threads);
	else
		x = Measure(&ping_pong_threads_nr);
	DestroyPingPongThread();
	TEST_PRINT4("PingPong: Pri %2d Cpu %2d Reply %1d -> %ups", aPri, aCpu, aReply, x);
	return x;
	}

void do_dfc_test()
	{
	Dfc->Enque();
	NKern::WaitForAnyRequest();
	}

void do_dfc_test_nr()
	{
	Dfc->Enque();
	}

void do_idfc_test()
	{
	NKern::Lock();
	Dfc->Add();
	NKern::Unlock();
	NKern::WaitForAnyRequest();
	}

void do_idfc_test_nr()
	{
	NKern::Lock();
	Dfc->Add();
	NKern::Unlock();
	}

TUint32 DoDfcTest(TInt aPri, TInt aCpu, TBool aReply=TRUE)
	{
	SetupBenchmarkDfcQ(aPri, aCpu, aReply);
	TUint32 x;
	PFV f = aReply ? (aPri<0 ? &do_idfc_test : &do_dfc_test) : (aPri<0 ? &do_idfc_test_nr : &do_dfc_test_nr);
	x = Measure(f);
	DestroyBenchmarkDfcQ();
	TEST_PRINT4("Dfc: Pri %2d Cpu %2d Reply %1d -> %ups", aPri, aCpu, aReply, x);
	return x;
	}

void BenchmarkTests()
	{
	TUint64 fcf = fast_counter_freq();
	Threshold = (TUint32)fcf;
	if (Threshold > 10000000u)
		Threshold /= 10u;
	else if (Threshold > 1000000u)
		Threshold = 1000000u;
	TUint64 ps_per_tick = UI64LIT(1000000000000);
	ps_per_tick /= fcf;
	PsPerTick = (TUint32)ps_per_tick;
	TEST_PRINT1("Threshold %u", Threshold);
	TEST_PRINT1("PsPerTick %u", PsPerTick);

	TUint32 dummy_time = Measure(&dummy);
	TEST_PRINT1("Dummy %ups", dummy_time);

	TUint32 dmb_time = Measure(&__e32_memory_barrier);
	TEST_PRINT1("DMB loop %ups", dmb_time);
	dmb_time -= dummy_time;
	TEST_PRINT1("DMB %ups", dmb_time);

	TUint32 dsb_time = Measure(&__e32_io_completion_barrier);
	TEST_PRINT1("DSB loop %ups", dsb_time);
	dsb_time -= dummy_time;
	TEST_PRINT1("DSB %ups", dsb_time);

#ifdef __SMP__
	TUint32 timestamp_time = Measure(&get_timestamp) - dummy_time;
	TEST_PRINT1("NKern::Timestamp() %ups", timestamp_time);
#endif

	TUint32 ps;
	ps = Measure(&sleep1);
	TEST_PRINT1("Sleep(1) %ups", ps-dummy_time);
	ps = Measure(&do_atomic_add_rlx32);
	TEST_PRINT1("atomic_add_rlx32 %ups", ps-dummy_time);
	ps = Measure(&do_atomic_add_acq32);
	TEST_PRINT1("atomic_add_acq32 %ups", ps-dummy_time);
	ps = Measure(&do_atomic_add_rel32);
	TEST_PRINT1("atomic_add_rel32 %ups", ps-dummy_time);
	ps = Measure(&do_atomic_add_ord32);
	TEST_PRINT1("atomic_add_ord32 %ups", ps-dummy_time);

	TUint32 dis_ena_int_time = Measure(&dis_ena_int) - dummy_time;
	TEST_PRINT1("dis_ena_int %ups", dis_ena_int_time);

	TUint32 dis_ena_preempt_time = Measure(&dis_ena_preempt) - dummy_time;
	TEST_PRINT1("dis_ena_preempt %ups", dis_ena_preempt_time);

#ifdef __SMP__
	TUint32 sl1_irq_time = Measure(&sl1_lock_unlock_irq) - dummy_time;
	TEST_PRINT1("sl1_irq_time %ups", sl1_irq_time);

	TUint32 sl1_irqsave_time = Measure(&sl1_lock_unlock_irq_save) - dummy_time;
	TEST_PRINT1("sl1_irqsave_time %ups", sl1_irqsave_time);

	TUint32 sl1_only_time = Measure(&sl1_lock_unlock_only) - dis_ena_int_time - dummy_time;
	TEST_PRINT1("sl1_only_time %ups", sl1_only_time);

	TUint32 sl2_only_time = Measure(&sl2_lock_unlock_only) - dis_ena_preempt_time - dummy_time;
	TEST_PRINT1("sl2_only_time %ups", sl2_only_time);
#endif

	TUint32 lock_unlock_system_time = Measure(&lock_unlock_system) - dummy_time;
	TEST_PRINT1("lock_unlock_system_time %ups", lock_unlock_system_time);

	TUint32 enter_leave_cs_time = Measure(&enter_leave_cs) - dummy_time;
	TEST_PRINT1("enter_leave_cs_time %ups", enter_leave_cs_time);

	TUint32 resched_to_same_time = Measure(&resched_to_same) - dummy_time;
	TEST_PRINT1("resched_to_same_time %ups", resched_to_same_time);

#ifdef __MARM__
	TUint32 get_static_data_time = Measure((PFV)&__get_static_data) - dummy_time;
	TEST_PRINT1("get_static_data_time %ups", get_static_data_time);

	TUint32 get_sp_time = Measure((PFV)&__stack_pointer) - dummy_time;
	TEST_PRINT1("get_sp_time %ups", get_sp_time);

	TUint32 get_cpsr_time = Measure((PFV)&__cpu_status_reg) - dummy_time;
	TEST_PRINT1("get_cpsr_time %ups", get_cpsr_time);

#ifdef __SMP__
	TUint32 get_cpu_id_time = Measure((PFV)&__cpu_id) - dummy_time;
	TEST_PRINT1("get_cpu_id_time %ups", get_cpu_id_time);

	TUint32 trace_cpu_num_time = Measure((PFV)&__trace_cpu_num) - dummy_time;
	TEST_PRINT1("trace_cpu_num_time %ups", trace_cpu_num_time);

	TUint32 get_rwno_tid_time = Measure((PFV)&__get_rwno_tid) - dummy_time;
	TEST_PRINT1("get_rwno_tid_time %ups", get_rwno_tid_time);

	TUint32 get_lta_time = Measure((PFV)&__get_local_timer_address) - dummy_time;
	TEST_PRINT1("get_local_timer_address %ups", get_lta_time);

	TUint32 get_ltc_time = Measure((PFV)&__get_local_timer_count) - dummy_time;
	TEST_PRINT1("get_local_timer_count %ups", get_ltc_time);

	TUint32 set_ltc_time = Measure((PFV)&__set_local_timer_count) - dummy_time;
	TEST_PRINT1("set_local_timer_count %ups", set_ltc_time);

	TUint32 swp_ltc_time = Measure((PFV)&__swp_local_timer_count) - dummy_time;
	TEST_PRINT1("swp_local_timer_count %ups", swp_ltc_time);
#endif

	TUint32 get_current_thread_time = Measure((PFV)&NKern::CurrentThread) - dummy_time;
	TEST_PRINT1("get_current_thread_time %ups", get_current_thread_time);

#ifdef __SMP__
	TUint32 get_current_threadL_time = Measure((PFV)&NCurrentThreadL) - dummy_time;
	TEST_PRINT1("get_current_threadL_time %ups", get_current_threadL_time);
#endif
#endif

	NThread* t = NKern::CurrentThread();
	NKern::FSSetOwner(&Sem, t);

	TUint32 sem_signal_time = Measure(&sem_signal) - dummy_time;
	TEST_PRINT1("sem_signal_time %ups", sem_signal_time);

	new (&Sem) NFastSemaphore(t);

	TUint32 sem_signal_wait_time = Measure(&sem_signal_wait) - dummy_time;
	TEST_PRINT1("sem_signal_wait_time %ups", sem_signal_wait_time);

	DoPingPongTest(31, 0);
	DoPingPongTest(11, 0);
	DoPingPongTest(31, 1);
	DoPingPongTest(11, 1);
	DoPingPongTest(31, -1);
	DoPingPongTest(11, -1);
	DoPingPongTest(31, 0, FALSE);

	DoDfcTest(31, 0);
	DoDfcTest(11, 0);
	DoDfcTest(31, 1);
	DoDfcTest(11, 1);
	DoDfcTest(31, -1);
	DoDfcTest(11, -1);
	DoDfcTest(31, 0, FALSE);
	DoDfcTest(-1, 0, TRUE);
	DoDfcTest(-1, 0, FALSE);
#if defined(__MARM__) && defined(__SMP__)
	DoWatchdogTimerTest();
#endif
	}
