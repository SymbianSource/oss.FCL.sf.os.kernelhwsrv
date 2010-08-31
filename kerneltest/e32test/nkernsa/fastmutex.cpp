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
// e32test\nkernsa\fastmutex.cpp
// 
//

#include <nktest/nkutils.h>

const TInt KReadCount = 100000;
//const TInt KReadCount = 2000000;
#ifdef __CPU_ARM
const TUint32 KTickLimit = (KReadCount>100000) ? (((TUint32)KReadCount)/10*18) : 180000u;
#else
const TUint32 KTickLimit = (KReadCount>100000) ? (((TUint32)KReadCount)/10*6) : 60000u;
#endif

class NFastMutexX
	{
public:
	TInt iRefCount;
	NFastMutex* iMutex;
public:
	NFastMutexX();
	~NFastMutexX();
	void Create();
	TBool Open();
	TBool Close();
	TBool Wait();
	TBool Signal();
	TBool WaitFull();
	TBool SignalFull();
	};

NFastMutexX::NFastMutexX()
	:	iRefCount(0), iMutex(0)
	{}

void NFastMutexX::Create()
	{
	iMutex = new NFastMutex;
	TEST_OOM(iMutex);
	__e32_atomic_store_rel32(&iRefCount, 1);
	}

NFastMutexX::~NFastMutexX()
	{
	TEST_RESULT2(iRefCount==0, "Bad mutex ref count %d %08x", iRefCount, this);
	memset(this, 0xbf, sizeof(*this));
	}

TBool NFastMutexX::Open()
	{
	return __e32_atomic_tas_ord32(&iRefCount, 1, 1, 0) >  0;
	}

TBool NFastMutexX::Close()
	{
	TInt r = __e32_atomic_tas_ord32(&iRefCount, 1, -1, 0);
	if (r==1)
		{
		memset(iMutex, 0xbf, sizeof(NFastMutex));
		delete iMutex;
		iMutex = 0;
		}
	return r==1;
	}

TBool NFastMutexX::Wait()
	{
	if (Open())
		{
		NKern::FMWait(iMutex);
		return TRUE;
		}
	return FALSE;
	}

TBool NFastMutexX::Signal()
	{
	NKern::FMSignal(iMutex);
	return Close();
	}

TBool NFastMutexX::WaitFull()
	{
	if (Open())
		{
		FMWaitFull(iMutex);
		return TRUE;
		}
	return FALSE;
	}

TBool NFastMutexX::SignalFull()
	{
	FMSignalFull(iMutex);
	return Close();
	}

void FMTest0()
	{
	TEST_PRINT("Testing non-contention case");

	NFastMutex m;

	TEST_RESULT(!m.HeldByCurrentThread(), "Mutex held by current thread");
	TEST_RESULT(!NKern::HeldFastMutex(), "Current thread holds a fast mutex");

	NKern::FMWait(&m);

	TEST_RESULT(m.HeldByCurrentThread(), "Mutex not held by current thread");
	TEST_RESULT(NKern::HeldFastMutex()==&m, "HeldFastMutex() incorrect");

	NKern::FMSignal(&m);

	TEST_RESULT(!m.HeldByCurrentThread(), "Mutex held by current thread");
	TEST_RESULT(!NKern::HeldFastMutex(), "Current thread holds a fast mutex");
	}

struct SFMTest1Info
	{
	NFastMutex iMutex;
	volatile TUint32* iBlock;
	TInt iBlockSize;	// words
	TInt iPriorityThreshold;
	volatile TBool iStop;
	NThread* iThreads[3*KMaxCpus];
	};

void FMTest1Thread(TAny* a)
	{
	SFMTest1Info& info = *(SFMTest1Info*)a;
	NThread* pC = NKern::CurrentThread();
	TUint32 seed[2] = {(TUint32)pC, 0};
	TBool wait = (pC->iPriority > info.iPriorityThreshold);
	TBool thread0 = (pC==info.iThreads[0]);
	TInt n = 0;
	while (!info.iStop)
		{
		if (thread0)
			NKern::ThreadSetPriority(pC, 11);
		NKern::FMWait(&info.iMutex);
		TBool ok = verify_block((TUint32*)info.iBlock, info.iBlockSize);
		TEST_RESULT(ok, "Block corrupt");
		++info.iBlock[0];
		setup_block((TUint32*)info.iBlock, info.iBlockSize);
		++n;
		NKern::FMSignal(&info.iMutex);
		if (wait)
			{
			TUint32 x = random(seed) & 1;
			NKern::Sleep(x+1);
			}
		}
	TEST_PRINT2("Thread %T ran %d times", pC, n);
	}

void FMTest1PInterfererThread(TAny* a)
	{
	SFMTest1Info& info = *(SFMTest1Info*)a;
	NThread* pC = NKern::CurrentThread();
	TEST_PRINT1("Thread %T start", pC);
	TUint32 seed[2] = {(TUint32)pC, 0};
	NThread* t0 = info.iThreads[0];
	TInt n = 0;
	while (!__e32_atomic_load_acq32(&info.iStop))
		{
		while (!__e32_atomic_load_acq32(&info.iStop) && t0->iPriority != 11)
			__chill();
		TUint32 x = random(seed) & 2047;
		while(x)
			{
			__e32_atomic_add_ord32(&x, TUint32(-1));
			}
		if (__e32_atomic_load_acq32(&info.iStop))
			break;
		NKern::ThreadSetPriority(t0, 9);
		++n;
		}
	TEST_PRINT2("Thread %T ran %d times", pC, n);
	}

void FMTest1()
	{
	TEST_PRINT("Testing mutual exclusion");

	NFastSemaphore exitSem(0);
	SFMTest1Info* pI = new SFMTest1Info;
	TEST_OOM(pI);
	memclr(pI, sizeof(SFMTest1Info));
	pI->iBlockSize = 256;
	pI->iBlock = (TUint32*)malloc(pI->iBlockSize*sizeof(TUint32));
	TEST_OOM(pI->iBlock);
	pI->iPriorityThreshold = 10;
	pI->iBlock[0] = 0;
	setup_block((TUint32*)pI->iBlock, pI->iBlockSize);
	pI->iStop = FALSE;
	TInt cpu;
	TInt threads = 0;
	for_each_cpu(cpu)
		{
		CreateThreadSignalOnExit("FMTest1H", &FMTest1Thread, 11, pI, 0, KSmallTimeslice, &exitSem, cpu);
		CreateThreadSignalOnExit("FMTest1L0", &FMTest1Thread, 10, pI, 0, KSmallTimeslice, &exitSem, cpu);
		CreateThreadSignalOnExit("FMTest1L1", &FMTest1Thread, 10, pI, 0, KSmallTimeslice, &exitSem, cpu);
		threads += 3;
		}
	FOREVER
		{
		NKern::Sleep(1000);
		TEST_PRINT1("%d", pI->iBlock[0]);
		if (pI->iBlock[0] > 65536)
			{
			pI->iStop = TRUE;
			break;
			}
		}
	while (threads--)
		NKern::FSWait(&exitSem);
	TEST_PRINT1("Total iterations %d", pI->iBlock[0]);
	free((TAny*)pI->iBlock);
	free(pI);
	}

void FMTest1P()
	{
	TEST_PRINT("Testing priority change");
	if (NKern::NumberOfCpus()==1)
		return;

	NFastSemaphore exitSem(0);
	SFMTest1Info* pI = new SFMTest1Info;
	TEST_OOM(pI);
	memclr(pI, sizeof(SFMTest1Info));
	TEST_PRINT1("Info@0x%08x", pI);
	pI->iBlockSize = 256;
	pI->iBlock = (TUint32*)malloc(pI->iBlockSize*sizeof(TUint32));
	TEST_OOM(pI->iBlock);
	pI->iPriorityThreshold = 9;
	pI->iBlock[0] = 0;
	setup_block((TUint32*)pI->iBlock, pI->iBlockSize);
	pI->iStop = FALSE;
	TInt cpu;
	TInt threadCount = 0;
	TInt pri = 9;
	char name[16] = "FMTest1P.0";
	for_each_cpu(cpu)
		{
		name[9] = (char)(threadCount + '0');
		if (cpu==1)
			pI->iThreads[threadCount] = CreateThreadSignalOnExit("FMTest1PInterferer", &FMTest1PInterfererThread, 12, pI, 0, KSmallTimeslice, &exitSem, 1);
		else
			pI->iThreads[threadCount] = CreateThreadSignalOnExit(name, &FMTest1Thread, pri, pI, 0, KSmallTimeslice, &exitSem, cpu);
		pri = 10;
		threadCount++;
		}
	TUint32 b0 = 0xffffffffu;
	FOREVER
		{
		NKern::Sleep(1000);
		TUint32 b = pI->iBlock[0];
		TEST_PRINT1("%d", b);
		if (b > 1048576)
			{
			pI->iStop = TRUE;
			break;
			}
		if (b == b0)
			{
			__crash();
			}
		b0 = b;
		}
	while (threadCount--)
		NKern::FSWait(&exitSem);
	TEST_PRINT1("Total iterations %d", pI->iBlock[0]);
	free((TAny*)pI->iBlock);
	free(pI);
	}

struct SFMTest2InfoC
	{
	NFastMutex iMutex;
	TInt iMax;
	TBool iStop;
	};

struct SFMTest2InfoT
	{
	SFMTest2InfoC* iCommon;
	TUint32 iMaxDelay;
	TInt iIterations;
	TUint32 iSpinTime;
	TUint32 iBlockTimeMask;
	TUint32 iBlockTimeOffset;
	NThread* iThread;
	union {
		TUint8 iSpoiler;
		TUint32 iDelayThreshold;
		};
	};

TBool StopTest = FALSE;

void FMTest2Thread(TAny* a)
	{
	SFMTest2InfoT& t = *(SFMTest2InfoT*)a;
	SFMTest2InfoC& c = *t.iCommon;
	NThreadBase* pC = NKern::CurrentThread();
	TUint32 seed[2] = {(TUint32)pC, 0};
	while (!c.iStop)
		{
		++t.iIterations;
		if (t.iSpoiler)
			{
			nfcfspin(t.iSpinTime);
			}
		else
			{
			TUint32 initial = norm_fast_counter();
			NKern::FMWait(&c.iMutex);
			TUint32 final = norm_fast_counter();
			TUint32 delay = final - initial;
			if (delay > t.iMaxDelay)
				{
				t.iMaxDelay = delay;
				__e32_atomic_add_ord32(&c.iMax, 1);
				if (delay > t.iDelayThreshold)
					__crash();
				}
			nfcfspin(t.iSpinTime);
			NKern::FMSignal(&c.iMutex);
			}
		if (t.iBlockTimeMask)
			{
			TUint32 sleep = (random(seed) & t.iBlockTimeMask) + t.iBlockTimeOffset;
			NKern::Sleep(sleep);
			}
		}
	TEST_PRINT3("Thread %T %d iterations, max delay %d", pC, t.iIterations, t.iMaxDelay);
	}

SFMTest2InfoT* CreateFMTest2Thread(	const char* aName,
									SFMTest2InfoC& aCommon,
									TUint32 aSpinTime,
									TUint32 aBlockTimeMask,
									TUint32 aBlockTimeOffset,
									TBool aSpoiler,
									TInt aPri,
									TInt aTimeslice,
									NFastSemaphore& aExitSem,
									TUint32 aCpu
								  )
	{
	SFMTest2InfoT* ti = new SFMTest2InfoT;
	TEST_OOM(ti);
	ti->iCommon = &aCommon;
	ti->iMaxDelay = 0;
	ti->iDelayThreshold = 0xffffffffu;
	ti->iIterations = 0;
	ti->iSpinTime = aSpinTime;
	ti->iBlockTimeMask = aBlockTimeMask;
	ti->iBlockTimeOffset = aBlockTimeOffset;
	ti->iSpoiler = (TUint8)aSpoiler;
	ti->iThread = 0;

	NThread* t = CreateUnresumedThreadSignalOnExit(aName, &FMTest2Thread, aPri, ti, 0, aTimeslice, &aExitSem, aCpu);
	ti->iThread = t;
	DEBUGPRINT("Thread at %08x, Info at %08x", t, ti);

	return ti;
	}

extern void DebugPrint(const char*, int);
void FMTest2()
	{
	TEST_PRINT("Testing priority inheritance");

	NFastSemaphore exitSem(0);
	SFMTest2InfoC common;
	common.iMax = 0;
	common.iStop = FALSE;
	TInt cpu;
	TInt threads = 0;
	SFMTest2InfoT* tinfo[32];
	memset(tinfo, 0, sizeof(tinfo));
	DEBUGPRINT("Common info at %08x", &common);
	for_each_cpu(cpu)
		{
		tinfo[threads++] = CreateFMTest2Thread("FMTest2H", common, 500, 7, 7, FALSE, 60-cpu, KSmallTimeslice, exitSem, cpu);
		tinfo[threads++] = CreateFMTest2Thread("FMTest2L", common, 500, 0, 0, FALSE, 11, KSmallTimeslice, exitSem, cpu);
		tinfo[threads++] = CreateFMTest2Thread("FMTest2S", common, 10000, 15, 31, TRUE, 32, -1, exitSem, cpu);
		}
	tinfo[0]->iDelayThreshold = 0x300;
	TInt max = 0;
	TInt i;
	TInt iter = 0;
	for (i=0; i<threads; ++i)
		{
		NKern::ThreadResume(tinfo[i]->iThread);
		}
	FOREVER
		{
		NKern::Sleep(5000);
		DebugPrint(".",1);	// only print one char since interrupts are disabled for entire time

		TInt max_now = common.iMax;
		if (max_now==max)
			{
			if (++iter==20)
				break;
			}
		else
			{
			iter = 0;
			max = max_now;
			}
		}
	common.iStop = TRUE;
	for (i=0; i<threads; ++i)
		NKern::FSWait(&exitSem);
	DebugPrint("\r\n",2);
	for (i=0; i<threads; ++i)
		{
		TEST_PRINT3("%d: Iter %10d Max %10d", i, tinfo[i]->iIterations, tinfo[i]->iMaxDelay);
		if (i==0)
			{
			TEST_RESULT(tinfo[0]->iMaxDelay < 700, "Thread 0 MaxDelay too high");
			}
		else if (i==3)
			{
			TEST_RESULT(tinfo[3]->iMaxDelay < 1200, "Thread 1 MaxDelay too high");
			}
		}
	for (i=0; i<threads; ++i)
		delete tinfo[i];
	}

struct SWriterInfo
	{
	void DoInOp(TUint aWhich);
	void DoOutOp(TUint aWhich);

	TUint32* iBuf[6];
	TInt iWords;
	volatile TUint32 iWrites;
	volatile TUint32 iIn;
	volatile TBool iStop;
	NFastMutex* iM;
	NFastMutexX* iMX;
	TUint32 iInSeq;		// do nibble 0 followed by nibble 1 followed by nibble 2
	TUint32 iOutSeq;	// 0=nothing, 1=mutex, 2=freeze, 3=CS, 4=mutex the long way
						// 5 = mutexX, 6 = mutexX the long way, 7=join frozen group
						// 8 = join mutex-holding group, 9=join idle group
	TInt iFrz;
	TInt iPriority;
	TInt iTimeslice;
	TInt iCpu;
	NFastSemaphore iHandshake;
	TUint64 iInitFastCounter;
	TUint32 iFastCounterDelta;
	NThread* volatile iIvThrd;

#ifdef __SMP__
	NThreadGroup* iGroup;
#endif
	};

void SWriterInfo::DoInOp(TUint aWhich)
	{
	switch ((iInSeq>>(aWhich*4))&0xf)
		{
		case 0:	break;
		case 1:	NKern::FMWait(iM); break;
		case 2: iFrz=NKern::FreezeCpu(); break;
		case 3: NKern::ThreadEnterCS(); break;
		case 4: FMWaitFull(iM); break;
		case 5: iMX->Wait(); break;
		case 6: iMX->WaitFull(); break;
#ifdef __SMP__
		case 7:
		case 8:
		case 9:	NKern::JoinGroup(iGroup); break;
#endif
		}
	}

void SWriterInfo::DoOutOp(TUint aWhich)
	{
	switch ((iOutSeq>>(aWhich*4))&0xf)
		{
		case 0:	break;
		case 1:	NKern::FMSignal(iM); break;
		case 2: NKern::EndFreezeCpu(iFrz); break;
		case 3: NKern::ThreadLeaveCS(); break;
		case 4: FMSignalFull(iM); break;
		case 5: iMX->Signal(); break;
		case 6: iMX->SignalFull(); break;
#ifdef __SMP__
		case 7:
		case 8:
		case 9:	NKern::LeaveGroup(); break;
#endif
		}
	}

struct SReaderInfo
	{
	enum TTestType
		{
		ETimeslice,
		ESuspend,
		EKill,
		EMigrate,
		EInterlockedSuspend,
		EInterlockedKill,
		EInterlockedMigrate,
		EMutexLifetime,
		};

	TUint32* iBuf[6];
	TInt iWords;
	volatile TUint32 iReads;
	volatile TUint32 iFails[7];
	volatile TBool iStop;
	TUint32 iReadLimit;
	NThread* volatile iWriter;
	NThread* volatile iReader;
	NThread* volatile iIvThrd;
	NThread* iGroupThrd;
	SWriterInfo* iWriterInfo;
	TInt iTestType;
	NFastSemaphore iExitSem;
	volatile TUint32 iCapturedIn;
	volatile TBool iSuspendResult;
	};

void WriterThread(TAny* a)
	{
	SWriterInfo& info = *(SWriterInfo*)a;
//	TEST_PRINT(">WR");

	while (!info.iStop)
		{
		NThread* t = (NThread*)__e32_atomic_swp_ord_ptr(&info.iIvThrd, 0);
		if (t)
			NKern::ThreadRequestSignal(t);
		if (!info.iFastCounterDelta)
			info.iInitFastCounter = fast_counter();
		TInt n = ++info.iWrites;

		info.DoInOp(0);

		info.iBuf[0][0] = n;
		setup_block_cpu(info.iBuf[0], info.iWords);

		info.DoInOp(1);

		info.iBuf[1][0] = n;
		setup_block_cpu(info.iBuf[1], info.iWords);

		info.DoInOp(2);

		if (NKern::CurrentCpu() == info.iCpu)
			++info.iIn;
		info.iBuf[2][0] = n;
		setup_block_cpu(info.iBuf[2], info.iWords);

		info.DoOutOp(0);

		info.iBuf[3][0] = n;
		setup_block_cpu(info.iBuf[3], info.iWords);

		info.DoOutOp(1);

		info.iBuf[4][0] = n;
		setup_block_cpu(info.iBuf[4], info.iWords);

		info.DoOutOp(2);

		info.iBuf[5][0] = n;
		setup_block_cpu(info.iBuf[5], info.iWords);

		if (!info.iFastCounterDelta)
			info.iFastCounterDelta = (TUint32)(fast_counter() - info.iInitFastCounter);
		if (NKern::CurrentCpu() != info.iCpu)
			{
			NKern::FSSignal(&info.iHandshake);
			NKern::WaitForAnyRequest();
			}
		}
//	TEST_PRINT("<WR");
	}

void ReaderThread(TAny* a)
	{
	SReaderInfo& info = *(SReaderInfo*)a;
	SWriterInfo& winfo = *info.iWriterInfo;
	TInt this_cpu = NKern::CurrentCpu();
	NThread* pC = NKern::CurrentThread();
	info.iReader = pC;
//	TInt my_pri = pC->i_NThread_BasePri;
	TBool create_writer = TRUE;
	NKern::FSSetOwner(&winfo.iHandshake, 0);
	NFastSemaphore exitSem(0);
	TUint32 seed[2] = {0,7};
	TUint32 modulus = 0;
	TUint32 offset = 0;
//	TEST_PRINT1(">RD%d",info.iTestType);

	while (!info.iStop)
		{
		TInt i;
		if (create_writer)
			goto do_create_writer;
		if (info.iTestType==SReaderInfo::EMigrate || info.iTestType==SReaderInfo::EInterlockedMigrate)
			{
			NKern::FSWait(&winfo.iHandshake);
			}
		for (i=0; i<6; ++i)
			{
			TInt cpu = verify_block_cpu_no_trace(info.iBuf[i], info.iWords);
			if (cpu<0)
				++info.iFails[i];
			}
		++info.iReads;
		switch (info.iTestType)
			{
			case SReaderInfo::ETimeslice:
				NKern::ThreadSetTimeslice(info.iWriter, (random(seed) % modulus + offset) );
				NKern::YieldTimeslice();
				break;
			case SReaderInfo::ESuspend:
				winfo.iIvThrd = info.iIvThrd;
				NKern::ThreadResume(info.iWriter);
				break;
			case SReaderInfo::EKill:
				NKern::FSWait(&exitSem);
				create_writer = TRUE;
				break;
			case SReaderInfo::EMigrate:
				NKern::ThreadSetCpuAffinity(info.iWriter, this_cpu);
				if (info.iGroupThrd)
					NKern::ThreadSetCpuAffinity(info.iGroupThrd, this_cpu);
				NKern::ThreadRequestSignal(info.iIvThrd);
				NKern::ThreadRequestSignal(info.iWriter);
				break;
			case SReaderInfo::EInterlockedSuspend:
				NKern::WaitForAnyRequest();
				NKern::FMWait(winfo.iM);
				if (winfo.iIn != info.iCapturedIn && info.iSuspendResult)
					++info.iFails[6];
				winfo.iIvThrd = info.iIvThrd;
				NKern::ThreadResume(info.iWriter, winfo.iM);
				break;
			case SReaderInfo::EInterlockedKill:
				NKern::WaitForAnyRequest();
				NKern::FSWait(&exitSem);
				if (winfo.iIn != info.iCapturedIn)
					++info.iFails[6];
				create_writer = TRUE;
				break;
			case SReaderInfo::EInterlockedMigrate:
				NKern::WaitForAnyRequest();
				if (winfo.iIn != info.iCapturedIn)
					++info.iFails[6];
				NKern::ThreadSetCpuAffinity(info.iWriter, this_cpu);
				if (info.iGroupThrd)
					NKern::ThreadSetCpuAffinity(info.iGroupThrd, this_cpu);
				NKern::ThreadRequestSignal(info.iIvThrd);
				NKern::ThreadRequestSignal(info.iWriter);
				break;
			}
do_create_writer:
		if (create_writer)
			{
			create_writer = FALSE;
			winfo.iCpu = this_cpu;
			info.iWriter = CreateUnresumedThreadSignalOnExit("Writer", &WriterThread, winfo.iPriority, &winfo, 0, winfo.iTimeslice, &exitSem, this_cpu);
			TEST_OOM(info.iWriter);
			winfo.iIvThrd = info.iIvThrd;
			NKern::ThreadResume(info.iWriter);
			while (!winfo.iFastCounterDelta)
				NKern::Sleep(1);
			modulus = __fast_counter_to_timeslice_ticks(3*winfo.iFastCounterDelta);
//			offset = __microseconds_to_timeslice_ticks(64);
			offset = 1;
			}
		}
	winfo.iStop = TRUE;
	NKern::FSWait(&exitSem);
//	TEST_PRINT1("<RD%d",info.iTestType);
	}

void InterventionThread(TAny* a)
	{
	SReaderInfo& info = *(SReaderInfo*)a;
	SWriterInfo& winfo = *info.iWriterInfo;
	TInt this_cpu = NKern::CurrentCpu();
	TUint32 seed[2] = {1,0};
	while (!winfo.iFastCounterDelta)
		NKern::Sleep(1);
	TUint32 modulus = 3*winfo.iFastCounterDelta;
	TUint32 offset = TUint32(fast_counter_freq() / TUint64(100000));
	NThread* w = info.iWriter;
	TUint32 lw = 0;
	TUint32 tc = NKern::TickCount();
	NKern::FSSetOwner(&info.iExitSem, 0);

	TEST_PRINT3(">IV%d %d %d", info.iTestType, modulus, offset);
	FOREVER
		{
		if (this_cpu == winfo.iCpu)
			{
 			NKern::Sleep(1);
			}
		else
			{
			TUint32 count = random(seed) % modulus;
			count += offset;
			fcfspin(count);
			}
		if (info.iReads >= info.iReadLimit)
			{
			info.iStop = TRUE;
			winfo.iStop = TRUE;
			NKern::FSWait(&info.iExitSem);
			break;
			}
		if (winfo.iWrites >= lw + 3*info.iReadLimit)
			{
			lw += 3*info.iReadLimit;
			TEST_PRINT1("#W=%d",winfo.iWrites);
			}
		TUint32 tc2 = NKern::TickCount();
		if ( (tc2 - (tc+KTickLimit)) < 0x80000000 )
			{
			tc = tc2;
			TEST_PRINT1("##W=%d",winfo.iWrites);
			DumpMemory("WriterThread", w, 0x400);
			}
		switch (info.iTestType)
			{
			case SReaderInfo::ETimeslice:
				break;
			case SReaderInfo::ESuspend:
				NKern::ThreadSuspend(info.iWriter, 1);
	 			NKern::WaitForAnyRequest();
				break;
			case SReaderInfo::EKill:
				{
				w = info.iWriter;
				info.iWriter = 0;
				NKern::ThreadKill(w);
	 			NKern::WaitForAnyRequest();
				break;
				}
			case SReaderInfo::EMigrate:
				NKern::ThreadSetCpuAffinity(info.iWriter, this_cpu);
				if (info.iGroupThrd)
					NKern::ThreadSetCpuAffinity(info.iGroupThrd, this_cpu);
	 			NKern::WaitForAnyRequest();
				break;
			case SReaderInfo::EInterlockedSuspend:
				{
#if 0
extern TLinAddr __LastIrqRet;
extern TLinAddr __LastSSP;
extern TLinAddr __SSTop;
extern TUint32 __CaptureStack[1024];
extern TLinAddr __InterruptedThread;
extern TUint32 __CaptureThread[1024];
#endif
				NKern::FMWait(winfo.iM);
				info.iCapturedIn = winfo.iIn;
				info.iSuspendResult = NKern::ThreadSuspend(info.iWriter, 1);
				NKern::FMSignal(winfo.iM);
				NKern::ThreadRequestSignal(info.iReader);
#if 0
				NThread* pC = NKern::CurrentThread();
				TUint32 tc0 = NKern::TickCount();
				tc0+=1000;
				FOREVER
					{
					TUint32 tc1 = NKern::TickCount();
					if ((tc1-tc0)<0x80000000u)
						{
						DEBUGPRINT("__LastIrqRet = %08x", __LastIrqRet);
						DEBUGPRINT("__LastSSP    = %08x", __LastSSP);
						DEBUGPRINT("__SSTop      = %08x", __SSTop);

						DumpMemory("WriterStack", __CaptureStack, __SSTop - __LastSSP);

						DumpMemory("CaptureThread", __CaptureThread, sizeof(NThread));

						DumpMemory("Writer", info.iWriter, sizeof(NThread));

						DumpMemory("Reader", info.iReader, sizeof(NThread));

						DumpMemory("SubSched0", &TheSubSchedulers[0], sizeof(TSubScheduler));
						}
					if (pC->iRequestSemaphore.iCount>0)
						break;
					}
#endif
	 			NKern::WaitForAnyRequest();
				break;
				}
			case SReaderInfo::EInterlockedKill:
				{
				NKern::FMWait(winfo.iM);
				info.iCapturedIn = winfo.iIn;
				w = info.iWriter;
				info.iWriter = 0;
				NKern::ThreadKill(w, winfo.iM);
				NKern::ThreadRequestSignal(info.iReader);
	 			NKern::WaitForAnyRequest();
				break;
				}
			case SReaderInfo::EInterlockedMigrate:
				NKern::FMWait(winfo.iM);
				info.iCapturedIn = winfo.iIn;
				NKern::ThreadSetCpuAffinity(info.iWriter, this_cpu);
				if (info.iGroupThrd)
					NKern::ThreadSetCpuAffinity(info.iGroupThrd, this_cpu);
				NKern::FMSignal(winfo.iM);
				NKern::ThreadRequestSignal(info.iReader);
	 			NKern::WaitForAnyRequest();
				break;
			}
		}
	TEST_PRINT1("<IV%d",info.iTestType);
	}

// State bits 0-7 show how many times timeslices are blocked
// State bits 8-15 show how many times suspend/kill are blocked
// State bits 16-23 show how many times migration is blocked
// State bit 24 set if in CS when fast mutex held
// State bit 25 set if CPU frozen when fast mutex held
TUint32 UpdateState(TUint32 aS, TUint32 aOp, TBool aOut)
	{
	TUint32 x = 0;
	if (aS & 0xff00)
		x |= 0x01000000;
	if (aS & 0xff0000)
		x |= 0x02000000;
	if (aOut)
		{
		switch (aOp)
			{
			case 0:
			case 9:
				return aS;
			case 2:
			case 7:
			case 8:
				return aS-0x010000;
			case 3:
				return aS-0x000100;
			case 1:
			case 4:
				return aS-0x010101;
			}
		}
	else
		{
		switch (aOp)
			{
			case 0:
			case 9:
				return aS;
			case 2:
			case 7:
			case 8:
				return aS+0x010000;
			case 3:
				return aS+0x000100;
			case 1:
			case 4:
				return (aS+0x010101)|x;
			}
		}
	return aS;
	}

void CheckResults(SReaderInfo& info)
	{
	SWriterInfo& winfo = *info.iWriterInfo;
	TUint32 state[7];
	char c[72];
	memset(c, 32, sizeof(c)), c[71]=0;
	state[0] = UpdateState(0, (winfo.iInSeq)&0xf, FALSE);
	state[1] = UpdateState(state[0], (winfo.iInSeq>>4)&0xf, FALSE);
	state[2] = UpdateState(state[1], (winfo.iInSeq>>8)&0xf, FALSE);
	state[3] = UpdateState(state[2], (winfo.iOutSeq)&0xf, TRUE);
	state[4] = UpdateState(state[3], (winfo.iOutSeq>>4)&0xf, TRUE);
	state[5] = UpdateState(state[4], (winfo.iOutSeq>>8)&0xf, TRUE);
	state[6] = (state[5] & 0xff000000) ^ 0x07000000;

	TInt i;
	for (i=0; i<6; ++i)
		state[i] &= 0x00ffffff;

	TEST_PRINT2("Reads %d Writes %d", info.iReads, winfo.iWrites);
	for(i=0; i<6; ++i)
		{
		if (state[i] & 0xff00)
			c[i*10] = 'S';
		if (state[i] & 0xff0000)
			c[i*10+1] = 'M';
		if (state[i] & 0xff)
			c[i*10+2] = 'T';
		}
	TEST_PRINT1("%s",c);
	TEST_PRINT7("F0 %6d F1 %6d F2 %6d F3 %6d F4 %6d F5 %6d F6 %6d", info.iFails[0], info.iFails[1], info.iFails[2], info.iFails[3], info.iFails[4], info.iFails[5], info.iFails[6]);
	memset(c, 32, sizeof(c)), c[71]=0;
	TUint32 mask=0;
	switch(info.iTestType)
		{
		case SReaderInfo::ETimeslice:			mask = 0x040000ff; break;
		case SReaderInfo::ESuspend:				mask = 0x0400ff00; break;
		case SReaderInfo::EKill:				mask = 0x0400ff00; break;
		case SReaderInfo::EMigrate:				mask = 0x04ff0000; break;
		case SReaderInfo::EInterlockedSuspend:	mask = 0x0400ff00; break;
		case SReaderInfo::EInterlockedKill:		mask = 0x0100ff00; break;
		case SReaderInfo::EInterlockedMigrate:	mask = 0x02ff0000; break;
		}
	TUint32 limit = info.iReads/10;
	TInt fail=0;
	for(i=0; i<7; ++i)
		{
		TBool bad = FALSE;
		if (state[i] & mask)
			bad = (info.iFails[i] > 0);
		else
			bad = (info.iFails[i] < limit);
		if (bad)
			{
			++fail;
			char* p = c+i*10+3;
			*p++ = '-';
			*p++ = '-';
			*p++ = '-';
			*p++ = '-';
			*p++ = '-';
			*p++ = '-';
			}
		}
	if (fail)
		{
		c[0] = 'E';
		c[1] = 'R';
		c[2] = 'R';
		TEST_PRINT1("%s",c);
		TEST_RESULT(0,"FAILED");
		}
	}

struct SGroupThreadInfo
	{
	TUint32 iInSeq;
	TUint32 iRun;
	};

void GroupThread(TAny* a)
	{
	SGroupThreadInfo& info = *(SGroupThreadInfo*)a;
	TInt i, frz;
	NFastMutex mutex;
	for (i = 0; i<3; ++i)
		{
		// Find the first nibble that asks for a group option
		// and do what it asks for.
		switch ((info.iInSeq>>(i*4))&0xf)
			{
			case 7:
				frz = NKern::FreezeCpu();
				NKern::WaitForAnyRequest();
				NKern::EndFreezeCpu(frz);
				return;
			case 8:
				NKern::FMWait(&mutex);
				while (__e32_atomic_load_acq32(&info.iRun))
					nfcfspin(10);
				NKern::FMSignal(&mutex);
				return;
			}
		}
	// We weren't needed, but we have to wait to die anyway to avoid lifetime issues
	NKern::WaitForAnyRequest();
	}

void DoRWTest(TInt aTestType, TUint32 aReadLimit, TUint32 aInSeq, TUint32 aOutSeq, TInt aRWCpu, TInt aICpu)
	{
	NFastMutex mutex;
	SWriterInfo* winfo = new SWriterInfo;
	TEST_OOM(winfo);
	memclr(winfo, sizeof(SWriterInfo));
	SReaderInfo* info = new SReaderInfo;
	TEST_OOM(info);
	memclr(info, sizeof(SReaderInfo));
	SGroupThreadInfo* gtinfo = new SGroupThreadInfo;
	TEST_OOM(gtinfo);
	memclr(gtinfo, sizeof(SGroupThreadInfo));
	TUint32 bufwords = 256;
	TUint32* buf = (TUint32*)malloc(6 * bufwords * sizeof(TUint32));
	TEST_OOM(buf);
	memclr(buf, 6 * bufwords * sizeof(TUint32));
	TInt i;
	for (i=0; i<6; ++i)
		{
		info->iBuf[i] = buf + i * bufwords;
		winfo->iBuf[i] = buf + i * bufwords;
		}
	winfo->iWords = bufwords;
	winfo->iM = &mutex;
	winfo->iInSeq = aInSeq;
	winfo->iOutSeq = aOutSeq;
	winfo->iPriority = 11;
	winfo->iTimeslice = __microseconds_to_timeslice_ticks(10000);
	winfo->iCpu = aRWCpu;

	NFastSemaphore localExit(0);

#ifdef __SMP__
	NThreadGroup group;
	SNThreadGroupCreateInfo ginfo;
	ginfo.iCpuAffinity = aRWCpu;
	ginfo.iDestructionDfc = 0;	//FIXME
	TInt r = NKern::GroupCreate(&group, ginfo);
	TEST_RESULT(r==KErrNone, "");
	winfo->iGroup = &group;
	gtinfo->iRun = 1;
	gtinfo->iInSeq = aInSeq;
	NThread* groupThrd = CreateThreadSignalOnExit("GroupThrd", &GroupThread, 1, gtinfo, 0, KSmallTimeslice, &localExit, aRWCpu, &group);
	TEST_OOM(groupThrd);
	info->iGroupThrd = groupThrd;
	NKern::Sleep(100);
#endif

	info->iWords = bufwords;
	info->iReadLimit = aReadLimit;
	info->iWriterInfo = winfo;
	info->iTestType = aTestType;

	TInt rpri = (aTestType == SReaderInfo::ETimeslice) ? 11 : 10;
	NThread* reader = CreateThreadSignalOnExit("Reader", &ReaderThread, rpri, info, 0, -1, &info->iExitSem, aRWCpu);
	TEST_OOM(reader);
	info->iReader = reader;
	NKern::Sleep(10);
	NThread* ivt = CreateThreadSignalOnExit("Intervention", &InterventionThread, 12, info, 0, KSmallTimeslice, &localExit, aICpu);
	TEST_OOM(ivt);
	info->iIvThrd = ivt;

	NKern::FSWait(&localExit);
#ifdef __SMP__
	NKern::ThreadRequestSignal(groupThrd);
#endif
	__e32_atomic_store_rel32(&gtinfo->iRun, 0);
	NKern::FSWait(&localExit);

#ifdef __SMP__
	NKern::GroupDestroy(&group);
#endif

	free(buf);

	TEST_PRINT6("Type %d RL %d ISEQ %03x OSEQ %03x RWCPU %d ICPU %d", aTestType, aReadLimit, aInSeq, aOutSeq, aRWCpu, aICpu);
	CheckResults(*info);

	free(info);
	free(winfo);
	free(gtinfo);
	}


void TestFastMutex()
	{
	TEST_PRINT("Testing Fast Mutexes...");

	FMTest0();
	FMTest1();
	FMTest1P();
	FMTest2();
	}

void TestSuspendKillMigrate()
	{
	TEST_PRINT("Testing Suspend/Kill/Migrate...");

	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x000, 0x000, 0, 1);
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x010, 0x100, 0, 1);
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x040, 0x400, 0, 1);
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x132, 0x231, 0, 1);
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x432, 0x234, 0, 1);
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x310, 0x310, 0, 1);
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x340, 0x340, 0, 1);

	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x000, 0x000, 0, 1);
	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x010, 0x100, 0, 1);
	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x040, 0x400, 0, 1);
	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x132, 0x231, 0, 1);
	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x432, 0x234, 0, 1);
	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x310, 0x310, 0, 1);
	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x340, 0x340, 0, 1);

	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x000, 0x000, 0, 1);
	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x010, 0x100, 0, 1);
	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x040, 0x400, 0, 1);
	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x132, 0x231, 0, 1);
	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x432, 0x234, 0, 1);
	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x310, 0x310, 0, 1);
	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x340, 0x340, 0, 1);

	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x000, 0x000, 0, 1);
	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x132, 0x231, 0, 1);
	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x432, 0x234, 0, 1);
	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x020, 0x200, 0, 1);
	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x010, 0x100, 0, 1);
	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x040, 0x400, 0, 1);
	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x030, 0x300, 0, 1);

	DoRWTest(SReaderInfo::EInterlockedSuspend,	KReadCount, 0x432, 0x234, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedSuspend,	KReadCount, 0x040, 0x400, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedSuspend,	KReadCount, 0x010, 0x100, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedSuspend,	KReadCount, 0x340, 0x340, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedSuspend,	KReadCount, 0x310, 0x310, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedSuspend,	KReadCount, 0x132, 0x231, 0, 1);

	DoRWTest(SReaderInfo::EInterlockedKill,		KReadCount, 0x040, 0x400, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedKill,		KReadCount, 0x010, 0x100, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedKill,		KReadCount, 0x432, 0x234, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedKill,		KReadCount, 0x340, 0x340, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedKill,		KReadCount, 0x132, 0x231, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedKill,		KReadCount, 0x310, 0x310, 0, 1);

	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x040, 0x400, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x010, 0x100, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x310, 0x310, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x340, 0x340, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x132, 0x231, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x432, 0x234, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x120, 0x210, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x420, 0x240, 0, 1);

#ifdef __SMP__
	// Tests from above that involve freezing, except by joining a frozen group instead
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x137, 0x731, 0, 1);
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x437, 0x734, 0, 1);

	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x137, 0x731, 0, 1);
	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x437, 0x734, 0, 1);

	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x137, 0x731, 0, 1);
	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x437, 0x734, 0, 1);

	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x137, 0x731, 0, 1);
	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x437, 0x734, 0, 1);
	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x070, 0x700, 0, 1);

	DoRWTest(SReaderInfo::EInterlockedSuspend,	KReadCount, 0x437, 0x734, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedSuspend,	KReadCount, 0x137, 0x731, 0, 1);

	DoRWTest(SReaderInfo::EInterlockedKill,		KReadCount, 0x437, 0x734, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedKill,		KReadCount, 0x137, 0x731, 0, 1);

	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x137, 0x731, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x437, 0x734, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x170, 0x710, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x470, 0x740, 0, 1);

	// Tests from above that involve freezing, except by joining a group with a mutex-holder instead
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x138, 0x831, 0, 1);
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x438, 0x834, 0, 1);

	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x138, 0x831, 0, 1);
	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x438, 0x834, 0, 1);

	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x138, 0x831, 0, 1);
	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x438, 0x834, 0, 1);

	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x138, 0x831, 0, 1);
	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x438, 0x834, 0, 1);
	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x080, 0x800, 0, 1);

	DoRWTest(SReaderInfo::EInterlockedSuspend,	KReadCount, 0x438, 0x834, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedSuspend,	KReadCount, 0x138, 0x831, 0, 1);

	DoRWTest(SReaderInfo::EInterlockedKill,		KReadCount, 0x438, 0x834, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedKill,		KReadCount, 0x138, 0x831, 0, 1);

	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x138, 0x831, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x438, 0x834, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x180, 0x810, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x480, 0x840, 0, 1);

	// Tests from above that have a noop, except join a group that's doing nothing instead
	// Most of these do "join group, other op, leave group, undo other op" - this is
	// supposed to work, even though you can't *join* a group while frozen or holding a mutex
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x090, 0x900, 0, 1);
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x019, 0x190, 0, 1);
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x049, 0x490, 0, 1);
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x319, 0x319, 0, 1);
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x349, 0x349, 0, 1);

	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x090, 0x900, 0, 1);
	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x019, 0x190, 0, 1);
	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x049, 0x490, 0, 1);
	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x319, 0x319, 0, 1);
	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x349, 0x349, 0, 1);

	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x090, 0x900, 0, 1);
	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x019, 0x190, 0, 1);
	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x049, 0x490, 0, 1);
	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x319, 0x319, 0, 1);
	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x349, 0x349, 0, 1);

	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x090, 0x900, 0, 1);
	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x029, 0x290, 0, 1);
	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x019, 0x190, 0, 1);
	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x049, 0x490, 0, 1);
	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x039, 0x390, 0, 1);

	DoRWTest(SReaderInfo::EInterlockedSuspend,	KReadCount, 0x049, 0x490, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedSuspend,	KReadCount, 0x019, 0x190, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedSuspend,	KReadCount, 0x349, 0x349, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedSuspend,	KReadCount, 0x319, 0x319, 0, 1);

	DoRWTest(SReaderInfo::EInterlockedKill,		KReadCount, 0x049, 0x490, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedKill,		KReadCount, 0x019, 0x190, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedKill,		KReadCount, 0x349, 0x349, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedKill,		KReadCount, 0x319, 0x319, 0, 1);

	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x049, 0x490, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x019, 0x190, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x319, 0x319, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x349, 0x349, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x129, 0x219, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x429, 0x249, 0, 1);

	// Test freezing or acquiring a mutex while in a group that also does one of those things
	// and then leave the group.
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x017, 0x170, 0, 1);
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x018, 0x180, 0, 1);
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x027, 0x270, 0, 1);
	DoRWTest(SReaderInfo::ETimeslice,			KReadCount, 0x028, 0x280, 0, 1);

	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x017, 0x170, 0, 1);
	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x018, 0x180, 0, 1);
	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x027, 0x270, 0, 1);
	DoRWTest(SReaderInfo::ESuspend,				KReadCount, 0x028, 0x280, 0, 1);

	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x017, 0x170, 0, 1);
	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x018, 0x180, 0, 1);
	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x027, 0x270, 0, 1);
	DoRWTest(SReaderInfo::EKill,				KReadCount, 0x028, 0x280, 0, 1);

	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x017, 0x170, 0, 1);
	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x018, 0x180, 0, 1);
	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x027, 0x270, 0, 1);
	DoRWTest(SReaderInfo::EMigrate,				KReadCount, 0x028, 0x280, 0, 1);

	DoRWTest(SReaderInfo::EInterlockedSuspend,	KReadCount, 0x017, 0x170, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedSuspend,	KReadCount, 0x018, 0x180, 0, 1);

	DoRWTest(SReaderInfo::EInterlockedKill,		KReadCount, 0x017, 0x170, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedKill,		KReadCount, 0x018, 0x180, 0, 1);

	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x017, 0x170, 0, 1);
	DoRWTest(SReaderInfo::EInterlockedMigrate,	KReadCount, 0x018, 0x180, 0, 1);
#endif
}
