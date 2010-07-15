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
// e32test\demandpaging\t_thrash.cpp
// todo: test combinations of rom / code / data paging
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <dptest.h>
#include <e32hal.h>
#include <u32hal.h>
#include <u32exec.h>
#include <e32svr.h>
#include <e32panic.h>
#include "u32std.h"
#include <e32msgqueue.h>
#include <e32atomics.h>
#include <e32math.h>
#include <hal.h>

#include "t_dpcmn.h"
#include "../mmu/mmudetect.h"
#include "../mmu/d_memorytest.h"
#include "../mmu/t_codepaging_dll.h"

RTest test(_L("T_THRASH"));

volatile TBool gRunThrashTest = EFalse;

_LIT(KChunkName, "t_thrash chunk");

class TPRNG 
	{
public:
	TPRNG();
	TUint32 IntRand();
	TReal FloatRand();

private:
	enum
		{
		KA = 1664525,
		KB = 1013904223
		};
	TUint32 iV;
	};

TPRNG::TPRNG()
	{
	iV = (TUint32)this + RThread().Id() + User::FastCounter() + 23;
	}

TUint32 TPRNG::IntRand()
	{
	iV = KA * iV + KB;
	return iV;
	}

TReal TPRNG::FloatRand()
	{
	return (TReal)IntRand() / KMaxTUint32;
	}

class TRandom
	{
public:
	virtual ~TRandom() { }
	virtual TUint32 Next() = 0;
	};

 class TUniformRandom : public TRandom
	{
public:
	void SetParams(TUint aMax) { iMax = aMax; }
	virtual TUint32 Next();

private:
	TPRNG iRand;
	TUint iMax;
	};

TUint32 TUniformRandom::Next()
	{
	return iRand.IntRand() % iMax;
	}

class TNormalRandom : public TRandom
	{
public:
	void SetParams(TInt aMax, TInt aSd);
	virtual TUint32 Next();

private:
	TUint32 GetNext();

private:
	TPRNG iRand;
	TInt iMax;
	TInt iExpectation;
	TInt iSd;
	TUint32 iCached;
	};

void TNormalRandom::SetParams(TInt aMax, TInt aSd)
	{
	iMax = aMax;
	iExpectation = aMax / 2;
	iSd = aSd;
	iCached = KMaxTUint32;
	}

TUint32 TNormalRandom::Next()
	{
	TUint32 r;
	do
		{
		r = GetNext();
		}
	while (r > (TUint)iMax);
	return r;
	}

TUint32 TNormalRandom::GetNext()
	{
	if (iCached != KMaxTUint32)
		{
		TUint32 r = iCached;
		iCached = KMaxTUint32;
		return r;
		}
	
	// box-muller transform
	// from http://www.taygeta.com/random/gaussian.html

	TReal x1, x2, w, ln_w, y1, y2;
	do
		{
		x1 = 2.0 * iRand.FloatRand() - 1.0;
		x2 = 2.0 * iRand.FloatRand() - 1.0;
		w = x1 * x1 + x2 * x2;
		}
	while ( w >= 1.0 );

	TInt r = Math::Ln(ln_w, w);
	__ASSERT_ALWAYS(r == KErrNone, User::Invariant());
	w = (-2.0 * ln_w ) / w;
	TReal w2;
	r = Math::Sqrt(w2, w);
	__ASSERT_ALWAYS(r == KErrNone, User::Invariant());
	y1 = x1 * w2;
	y2 = x2 * w2;

	y1 = y1 * iSd + iExpectation;
	y2 = y2 * iSd + iExpectation;

	iCached = (TUint32)y2;

	return (TUint32)y1;
	}

static TBool BenchmarksSupported = EFalse;
static TReal BenchmarkMultiplier;

static TInt InitBenchmarks()
	{
	BenchmarksSupported = UserSvr::HalFunction(EHalGroupVM, EVMHalResetPagingBenchmark, (TAny*)EPagingBmReadRomPage, NULL) == KErrNone;
	if (!BenchmarksSupported)
		return KErrNone;
	
	TInt freq = 0;
	TInt r = HAL::Get(HAL::EFastCounterFrequency, freq);
	if (r != KErrNone)
		return r;
	BenchmarkMultiplier = 1000000.0 / freq;
	return KErrNone;
	}

static void ResetBenchmarks()
	{
	if (!BenchmarksSupported)
		return;	
	for (TInt i = 0 ; i < EMaxPagingBm ; ++i)
		{
		TInt r = UserSvr::HalFunction(EHalGroupVM, EVMHalResetPagingBenchmark, (TAny*)i, NULL);
		if (r != KErrNone)
			test.Printf(_L("Error resetting benchmark %d\n"), i);
		test_KErrNone(r);
		}
	}

static TInt GetBenchmark(TPagingBenchmark aBenchmark, TInt& aCountOut, TInt& aTotalTimeInMicrosOut)
	{
	
	SPagingBenchmarkInfo info;
	TInt r = UserSvr::HalFunction(EHalGroupVM, EVMHalGetPagingBenchmark, (TAny*)aBenchmark, &info);
	if (r!=KErrNone)
		return r;
	
	aCountOut = info.iCount;
	aTotalTimeInMicrosOut = (TInt)(info.iTotalTime * BenchmarkMultiplier);
	return KErrNone;
	}

static TInt GetAllBenchmarks(TInt aTestLengthInSeconds, TInt aCountOut[EMaxPagingBm], TInt aTimeOut[EMaxPagingBm])
	{
	for (TInt i = 0 ; i < EMaxPagingBm ; ++i)
		{
		TInt count = 0;
		TInt timeInMicros = 0;
		TInt r = GetBenchmark((TPagingBenchmark)i, count, timeInMicros);
		if (r != KErrNone)
			return r;
		
		aCountOut[i] = count / aTestLengthInSeconds;
		aTimeOut[i] = timeInMicros / aTestLengthInSeconds;
		}
	return KErrNone;	
	}

void CreatePagedChunk(TInt aSizeInPages)
	{
	test_Equal(0,gChunk.Handle());
	
	TChunkCreateInfo createInfo;
	TInt size = aSizeInPages * gPageSize;
	createInfo.SetNormal(size, size);
	createInfo.SetPaging(TChunkCreateInfo::EPaged);
	createInfo.SetOwner(EOwnerProcess);
	createInfo.SetGlobal(KChunkName);
	test_KErrNone(gChunk.Create(createInfo));
	test(gChunk.IsPaged()); // this is only ever called if data paging is supported
	}

TUint32* PageBasePtr(TInt aPage)
	{
	return (TUint32*)(gChunk.Base() + (gPageSize * aPage));
	}

TInt EnsureSystemIdleThread(TAny*)
	{
	RThread::Rendezvous(KErrNone);
	for (;;)
		{
		// Spin
		}
	}

void EnsureSystemIdle()
	{
	const TInt KMaxWait = 60 * 1000000;
	const TInt KSampleTime = 1 * 1000000;
	const TInt KWaitTime = 5 * 1000000;
	
	test.Printf(_L("Waiting for system to become idle\n"));
	TInt totalTime = 0;
	TBool idle;
	do
		{	
		RThread thread;
		test_KErrNone(thread.Create(_L("EnsureSystemIdleThread"), EnsureSystemIdleThread, 1024, NULL, NULL));		
		thread.SetPriority(EPriorityLess);
		TRequestStatus status;
		thread.Rendezvous(status);
		thread.Resume();

		User::WaitForRequest(status);
		test_KErrNone(status.Int());

		User::After(KSampleTime);
		thread.Suspend();

		TTimeIntervalMicroSeconds time;
		test_KErrNone(thread.GetCpuTime(time));
		TReal error = (100.0 * Abs(time.Int64() - KSampleTime)) / KSampleTime;
		test.Printf(_L("    time == %ld, error == %f%%\n"), time.Int64(), error);

		idle = error < 2.0;		
		
		thread.Kill(KErrNone);
		thread.Logon(status);
		User::WaitForRequest(status);
		test_KErrNone(status.Int());
		CLOSE_AND_WAIT(thread);
		
		if (!idle)
			User::After(KWaitTime);		// Allow system to finish whatever it's doing

		totalTime += KSampleTime + KWaitTime;
		test(totalTime < KMaxWait);
		}
	while(!idle);
	}

enum TWorkload
	{
	EWorkloadSequential,
	EWorkloadUniformRandom,
	EWorkloadNormalRandom1,
	EWorkloadNormalRandom2,
	EWorkloadShuffle,

	EMaxWorkloads
	};

struct SThrashTestArgs
	{
	TInt iThreadGroup;
	TInt iGroupSize;
	TWorkload iWorkload;
	TUint8* iBasePtr;
	volatile TInt iPageCount;
	volatile TInt64 iAccesses;
	};

TInt ThrashTestFunc(TAny* aArg)
	{
	SThrashTestArgs* args = (SThrashTestArgs*)aArg;

	TPRNG random;
	TUniformRandom uniformRand;
	TNormalRandom normalRand;

	TInt startPage = args->iThreadGroup * args->iGroupSize;
	TInt* ptr = (TInt*)(args->iBasePtr + startPage * gPageSize);

	
	switch (args->iWorkload)
		{
		case EWorkloadSequential:
			while (gRunThrashTest)
				{
				for (TUint i = 0 ;
					 gRunThrashTest && i < (args->iPageCount * gPageSize) / sizeof(TInt)  ;
					 ++i)
					{
					ptr[i] = 1;
					__e32_atomic_add_ord64(&args->iAccesses, 1);
					}
				}
			break;
				
		case EWorkloadUniformRandom:
		case EWorkloadNormalRandom1:
		case EWorkloadNormalRandom2:
			{
			TInt acc = 0;
			TInt oldSize = -1;
			TUint32 writeMask = 0;
			switch (args->iWorkload)
				{
				case EWorkloadUniformRandom:
				case EWorkloadNormalRandom1:
					writeMask = 0x80000000; break;
				case EWorkloadNormalRandom2:
					writeMask = 0xc0000000; break;
				default: test(EFalse); break;
				}
			while (gRunThrashTest)
				{
				TInt size = args->iPageCount;
				if (size != oldSize)
					{
					switch (args->iWorkload)
						{
						case EWorkloadUniformRandom:
							uniformRand.SetParams(size); break;
						case EWorkloadNormalRandom1:
						case EWorkloadNormalRandom2:
							normalRand.SetParams(size, size / 8); break;
						default: test(EFalse); break;
						}
					oldSize = size;
					}
				
				TInt page = args->iWorkload == EWorkloadUniformRandom ?
					uniformRand.Next() : normalRand.Next();
				TInt index = page * (gPageSize / sizeof(TInt));
				TBool write = (random.IntRand() & writeMask) == 0;
				if (write)
					ptr[index] = acc;
				else
					acc += ptr[index];
				__e32_atomic_add_ord64(&args->iAccesses, 1);
				}
			}
			break;
			
		case EWorkloadShuffle:
			{
			TInt i = 0;
			while (gRunThrashTest)
				{
				TInt size = (args->iPageCount * gPageSize) / sizeof(TInt);
				Mem::Swap(&ptr[i], &ptr[i + random.IntRand() % (size - i - 1) + 1], sizeof(TInt));
				__e32_atomic_add_ord64(&args->iAccesses, 2);
				++i;
				if (i >= size - 1)
					i = 0;
				}
			}
			break;

		default:
			test(EFalse);
		}

	return KErrNone;;
	}

struct SThrashThreadData
	{
	RThread iThread;
	TRequestStatus iStatus;
	SThrashTestArgs iArgs;
	};

void ThrashTest(const TDesC& aTestName,	// name and description
				TInt aThreads,			// number of threads to run
				TBool aSharedData,		// whether all threads share the same data
				TWorkload aWorkload,
				TInt aBeginPages,		// number of pages to start with for last/all threads
				TInt aEndPages,			// number of pages to end with for last/all threads
				TInt aOtherPages)		// num of pages for other threads, or zero to use same value for all
	{
	const TInt KTestLengthInSeconds = 2;
	
	test.Next(_L("Thrash test"));
	
	DPTest::FlushCache();
	EnsureSystemIdle();

	TInt i;
	test.Printf(_L("Table: %S\n"), &aTestName);
	test.Printf(_L("totalPages, totalAccesses, thrashLevel"));
	if (BenchmarksSupported)
		test.Printf(_L(", rejuveCount, rejuveTime, codePageInCount, codePageInTime, initCount, initTime, readCount, readTime, writePages, writeCount, writeTime"));
	if (aThreads > 1)
		{
		for (TInt i = 0 ; i < aThreads ; ++i)
			test.Printf(_L(", Thread%dPages, Thread%dAccesses"), i, i);
		}
	test.Printf(_L("\n"));

	TInt pagesNeeded;
	TInt maxPages = Max(aBeginPages, aEndPages);
	TInt groupSize = 0;
	if (aSharedData)
		pagesNeeded = Max(maxPages, aOtherPages);
	else
		{
		if (aOtherPages)
			{
			groupSize = aOtherPages;
			pagesNeeded = (aThreads - 1) * aOtherPages + maxPages;
			}
		else
			{
			groupSize = maxPages;
			pagesNeeded = aThreads * maxPages;
			}
		}
	CreatePagedChunk(pagesNeeded);
	
	SThrashThreadData* threads = new SThrashThreadData[aThreads];
	test_NotNull(threads);
	
	gRunThrashTest = ETrue;
	const TInt maxSteps = 30;
	TInt step = aEndPages >= aBeginPages ? Max((aEndPages - aBeginPages) / maxSteps, 1) : Min((aEndPages - aBeginPages) / maxSteps, -1);
	TInt pageCount = aBeginPages - 5 * step; // first run ignored
	
	for (i = 0 ; i < aThreads ; ++i)
		{
		SThrashThreadData& thread = threads[i];
		thread.iArgs.iThreadGroup = aSharedData ? 0 : i;
		thread.iArgs.iGroupSize = groupSize;
		thread.iArgs.iWorkload = aWorkload;
		thread.iArgs.iBasePtr = gChunk.Base();
		if (aOtherPages)
			thread.iArgs.iPageCount = (i == aThreads - 1) ? pageCount : aOtherPages;
		else
			thread.iArgs.iPageCount = pageCount;
		test_KErrNone(thread.iThread.Create(KNullDesC, ThrashTestFunc, gPageSize, NULL, &thread.iArgs));
		thread.iThread.Logon(thread.iStatus);
		thread.iThread.SetPriority(EPriorityLess);
		threads[i].iThread.Resume();
		}

	for (;;)
		{
		if (aOtherPages)
			threads[aThreads - 1].iArgs.iPageCount = pageCount;
		else
			{
			for (i = 0 ; i < aThreads ; ++i)
				threads[i].iArgs.iPageCount = pageCount;
			}
		
		for (i = 0 ; i < aThreads ; ++i)
			__e32_atomic_store_ord64(&threads[i].iArgs.iAccesses, 0);
		ResetBenchmarks();
		
		User::After(KTestLengthInSeconds * 1000 * 1000);

		TInt thrashLevel = UserSvr::HalFunction(EHalGroupVM, EVMHalGetThrashLevel, 0, 0);
		test(thrashLevel >= 0 && thrashLevel <= 255);
		
		TInt64 totalAccesses = 0;
		TInt totalPages = 0;
		for (i = 0 ; i < aThreads ; ++i)
			{
			totalAccesses += __e32_atomic_load_acq64(&threads[i].iArgs.iAccesses);
			if (aSharedData)
				totalPages = Max(totalPages, threads[i].iArgs.iPageCount);
			else
				totalPages += threads[i].iArgs.iPageCount;
			}
		TInt accessesPerSecond = (TInt)(totalAccesses / KTestLengthInSeconds);

		TBool warmingUp = (step > 0) ? pageCount < aBeginPages : pageCount > aBeginPages;
		if (!warmingUp)
			{
			test.Printf(_L("%10d, %13d, %11.2f"), totalPages, accessesPerSecond, (TReal)thrashLevel / 255);
		
			if (BenchmarksSupported)
				{
				TInt benchmarkCount[EMaxPagingBm];
				TInt benchmarkTime[EMaxPagingBm];
				test_KErrNone(GetAllBenchmarks(KTestLengthInSeconds, benchmarkCount, benchmarkTime));

				TInt otherPageInCount = benchmarkCount[EPagingBmReadRomPage] + benchmarkCount[EPagingBmReadCodePage];
				TInt otherPageInTime = benchmarkTime[EPagingBmReadRomPage] + benchmarkTime[EPagingBmReadCodePage];
		
				TInt initCount = benchmarkCount[EPagingBmReadDataPage] - benchmarkCount[EPagingBmReadDataMedia];
				TInt initTime = benchmarkTime[EPagingBmReadDataPage] - benchmarkTime[EPagingBmReadDataMedia];

				test.Printf(_L(", %11d, %10d, %15d, %14d, %9d, %8d, %9d, %8d, %10d, %10d, %9d"),
							benchmarkCount[EPagingBmRejuvenate], benchmarkTime[EPagingBmRejuvenate],
							otherPageInCount, otherPageInTime,
							initCount, initTime,
							benchmarkCount[EPagingBmReadDataMedia], benchmarkTime[EPagingBmReadDataMedia],
							benchmarkCount[EPagingBmWriteDataPage], 
							benchmarkCount[EPagingBmWriteDataMedia], benchmarkTime[EPagingBmWriteDataMedia]);
				}
		
			if (aThreads > 1)
				{
				for (i = 0 ; i < aThreads ; ++i)
					{
					test.Printf(_L(", %12d, %15ld"),
								threads[i].iArgs.iPageCount,
								__e32_atomic_load_acq64(&threads[i].iArgs.iAccesses));
					test_Equal(KRequestPending, threads[i].iStatus.Int());
					}
				}
			test.Printf(_L("\n"));
			}

		pageCount += step;
		if (aEndPages >= aBeginPages ? pageCount >= aEndPages : pageCount < aEndPages)
			break;
		}
	
	gRunThrashTest = EFalse;
	
	for (i = 0 ; i < aThreads ; ++i)
		{
		SThrashThreadData& thread = threads[i];
		User::WaitForRequest(thread.iStatus);
		test_Equal(EExitKill, thread.iThread.ExitType());
		test_KErrNone(thread.iStatus.Int());
		thread.iThread.Close();
		}

	gChunk.Close();	
	test.Printf(_L("\n"));
	}

void TestThrashing()
	{
	TInt minPages = (3 * gMaxCacheSize) / 4 - 4;
	TInt maxPages = (5 * gMaxCacheSize) / 4;
	TInt minPages2 = (3 * gMaxCacheSize) / 8 - 4;
	TInt maxPages2 = (5 * gMaxCacheSize) / 8;
	TInt minPages4 = (3 * gMaxCacheSize) / 16 - 4;
	TInt maxPages4 = (5 * gMaxCacheSize) / 16;

	// Single thread increasing in size
	ThrashTest(_L("single thread, sequential workload"),
			   1, ETrue, EWorkloadSequential, minPages, maxPages, 0);
	
	ThrashTest(_L("single thread, random workload"),
			   1, ETrue, EWorkloadUniformRandom, minPages, maxPages, 0);
	
	ThrashTest(_L("single thread, shuffle workload"),
			   1, ETrue, EWorkloadShuffle, minPages, maxPages, 0);

	// Multiple threads with shared data, one thread incresing in size
	ThrashTest(_L("two threads with shared data, one thread increasing, random workload"),
			   2, ETrue, EWorkloadUniformRandom, minPages, maxPages, minPages);
	
	ThrashTest(_L("four threads with shared data, one thread increasing, random workload"),
			   4, ETrue, EWorkloadUniformRandom, minPages, maxPages, minPages);

	// Multiple threads with shared data, all threads incresing in size
	ThrashTest(_L("two threads with shared data, all threads increasing, random workload"),
			   2, ETrue, EWorkloadUniformRandom, minPages, maxPages, 0);
	
	ThrashTest(_L("four threads with shared data, all threads increasing, random workload"),
			   4, ETrue, EWorkloadUniformRandom, minPages, maxPages, 0);
	
	// Multiple threads with independent data, one thread incresing in size
	ThrashTest(_L("two threads with independent data, one thread increasing, random workload"),
			   2, EFalse, EWorkloadUniformRandom, minPages2, maxPages2, gMaxCacheSize / 2);
	
	ThrashTest(_L("four threads with independent data, one thread increasing, random workload"),
			   4, EFalse, EWorkloadUniformRandom, minPages4, maxPages4, gMaxCacheSize / 4);
	
	// Multiple threads with independant data, all threads incresing in size
	ThrashTest(_L("two threads with independent data, all threads increasing, random workload"),
			   2, EFalse, EWorkloadUniformRandom, minPages2, maxPages2, 0);

	ThrashTest(_L("four threads with independent data, all threads increasing, random workload"),
			   4, EFalse, EWorkloadUniformRandom, minPages4, maxPages4, 0);

	// Attempt to create thrash state where there is sufficient cache
	TInt halfCacheSize = gMaxCacheSize / 2;
	ThrashTest(_L("two threads with independent data, one threads decreasing, random workload"),
			   2, EFalse, EWorkloadUniformRandom, halfCacheSize + 10, halfCacheSize - 30, halfCacheSize);
	}

void TestDistribution(TRandom& aRandom, TInt aSamples)
	{
	TUint32* data = new TUint32[aSamples];
	test_NotNull(data);

	TInt i;
	TReal mean = 0.0;
	for (i = 0 ; i < aSamples ; ++i)
		{
		data[i] = aRandom.Next();
		mean += (TReal)data[i] / aSamples;
		}

	TReal sum2 = 0.0;
	for (i = 0 ; i < aSamples ; ++i)
		{
		TReal d = (TReal)data[i] - mean;
		sum2 += d * d;
		}
	TReal variance = sum2 / (aSamples - 1);

	test.Printf(_L("  mean == %f\n"), mean);
	test.Printf(_L("  variance == %f\n"), variance);

	delete [] data;
	}

void TestDistributions()
	{
 	test.Next(_L("Test uniform distribution"));
	TUniformRandom rand1;
	rand1.SetParams(100);
	TestDistribution(rand1, 10000);
	
 	test.Next(_L("Test normal distribution"));	
	TNormalRandom rand2;
	rand2.SetParams(100, 25);
	TestDistribution(rand2, 10000);
	}

void BenchmarkReplacement()
	{
	// Report block write and physical access settings
	test.Next(_L("Report media physical access and preferred write size settings"));
	TInt physAccessSupported = UserSvr::HalFunction(EHalGroupVM, EVMHalGetPhysicalAccessSupported, 0, 0);
	test(physAccessSupported == 0 || physAccessSupported == 1);
	if (physAccessSupported)
 		test.Printf(_L("Physical access supported\n"));
	else
 		test.Printf(_L("Physical access not supported\n"));

	TInt preferredWriteSize = UserSvr::HalFunction(EHalGroupVM, EVMHalGetPreferredDataWriteSize, 0, 0);
	test(preferredWriteSize >= 0);
	test.Printf(_L("Preferred write size %d pages\n"), 1 << preferredWriteSize);

	for (TInt physAccess = 0 ; physAccess <= physAccessSupported ; ++physAccess)
		{
		test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalSetUsePhysicalAccess, (TAny*)physAccess, 0));
		test_Equal(physAccess, UserSvr::HalFunction(EHalGroupVM, EVMHalGetUsePhysicalAccess, 0, 0));
		TInt writeSize = 0;
		for (;;)
			{
			test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalSetDataWriteSize, (TAny*)writeSize, 0));
			TInt writeSizeSet = UserSvr::HalFunction(EHalGroupVM, EVMHalGetDataWriteSize, 0, 0);
			test (writeSizeSet >= 0);
			if (writeSizeSet != writeSize)
				break;  // stop loop when we reach limit of supported write size

			TBuf<128> title;
			title.AppendFormat(_L("Thrash test: single thread, normal random workload 2, phys access %d, write size %dKB"),
							   physAccess, 1 << (writeSize - 2));
			ThrashTest(title, 1, ETrue, EWorkloadNormalRandom2, (2 * gMaxCacheSize) / 3, 2 * gMaxCacheSize, 0);

			++writeSize;
			}
		}

	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalSetUsePhysicalAccess, (TAny*)physAccessSupported, 0));
	test_Equal(physAccessSupported, UserSvr::HalFunction(EHalGroupVM, EVMHalGetUsePhysicalAccess, 0, 0));
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalSetDataWriteSize, (TAny*)preferredWriteSize, 0));
	test_Equal(preferredWriteSize, UserSvr::HalFunction(EHalGroupVM, EVMHalGetDataWriteSize, 0, 0));

	ThrashTest(_L("Thrash test: single thread, normal random workload 1"),
			   1, ETrue, EWorkloadNormalRandom1, (2 * gMaxCacheSize) / 3, 2 * gMaxCacheSize, 0);
	
	ThrashTest(_L("Thrash test: single thread, normal random workload 2"),
			   1, ETrue, EWorkloadNormalRandom2, (2 * gMaxCacheSize) / 3, 2 * gMaxCacheSize, 0);

	ThrashTest(_L("Thrash test: single thread, uniform random workload"),
			   1, ETrue, EWorkloadUniformRandom, (2 * gMinCacheSize) / 3, (3 * gMaxCacheSize) / 2, 0);
	}

void TestThrashHal()
	{			 
	test.Next(_L("Test EVMHalSetThrashThresholds"));
	test_Equal(KErrArgument, UserSvr::HalFunction(EHalGroupVM, EVMHalSetThrashThresholds, (TAny*)256, 0));
	test_Equal(KErrArgument, UserSvr::HalFunction(EHalGroupVM, EVMHalSetThrashThresholds, (TAny*)0, (TAny*)1));
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalSetThrashThresholds, 0, 0));
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalSetThrashThresholds, (TAny*)255, 0));
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalSetThrashThresholds, (TAny*)200, (TAny*)150));

	test.Next(_L("Test EVMHalGetThrashLevel"));
	User::After(2000000);
	TInt r = UserSvr::HalFunction(EHalGroupVM, EVMHalGetThrashLevel, 0, 0);
	test(r >= 0 && r <= 255);
	test.Printf(_L("Thrash level == %d\n"), r);
	test(r <= 10);  // should indicate lightly loaded system

	if (!gDataPagingSupported)
		return;  // rest of this test relies on data paging

	// set up thrashing notification
	RChangeNotifier notifier;
	test_KErrNone(notifier.Create());
	TRequestStatus status;
	test_KErrNone(notifier.Logon(status));
	test_KErrNone(notifier.Logon(status));  // first logon completes immediately
	test_Equal(KRequestPending, status.Int());
	
	// stress system and check thrash level and notification
	ThrashTest(_L("stress system"),
			   1, ETrue, EWorkloadUniformRandom, gMaxCacheSize * 2, gMaxCacheSize * 2 + 5, 0);
	r = UserSvr::HalFunction(EHalGroupVM, EVMHalGetThrashLevel, 0, 0);
	test(r >= 0 && r <= 255);
	test.Printf(_L("Thrash level == %d\n"), r);
	test(r > 200);  // should indicate thrashing

	TBool gotThrashNotification = EFalse;
	
	// wait for EChangesThrashLevel notification
	while(status.Int() != KRequestPending)
		{
		gotThrashNotification = (status.Int() & EChangesThrashLevel) != 0;
		User::WaitForAnyRequest();
		test_KErrNone(notifier.Logon(status));
		User::After(1);		
		}
	test(gotThrashNotification);
	
	User::After(2000000);
	r = UserSvr::HalFunction(EHalGroupVM, EVMHalGetThrashLevel, 0, 0);
	test(r >= 0 && r <= 255);
	test.Printf(_L("Thrash level == %d\n"), r);
	test(r <= 10);  // should indicate lightly loaded system

	// wait for EChangesThrashLevel notification
	gotThrashNotification = EFalse;
	while(status.Int() != KRequestPending)
		{
		gotThrashNotification = (status.Int() & EChangesThrashLevel) != 0;
		User::WaitForAnyRequest();
		test_KErrNone(notifier.Logon(status));
		User::After(1);		
		}
	test(gotThrashNotification);
	test_KErrNone(notifier.LogonCancel());
	User::WaitForAnyRequest();
	notifier.Close();
	}

void TestThrashHalNotSupported()
	{
	test_Equal(KErrNotSupported, UserSvr::HalFunction(EHalGroupVM, EVMHalGetThrashLevel, 0, 0));
	test_Equal(KErrNotSupported, UserSvr::HalFunction(EHalGroupVM, EVMHalSetThrashThresholds, 0, 0));
	}

_LIT(KUsageMessage, "usage: t_thrash [ test ] [ thrashing ] [ benchmarks ]\n");

enum TTestAction
	{
	EActionTest       = 1 << 0,
	EActionThrashing  = 1 << 1,
	EActionBenchmarks = 1 << 2
	};

void BadUsage()
	{
	test.Printf(KUsageMessage);
	test(EFalse);
	}

TInt ParseCommandLine()
	{
	const TInt KMaxLineLength = 64;
	
	if (User::CommandLineLength() > KMaxLineLength)
		BadUsage();
	TBuf<KMaxLineLength> buffer;
	User::CommandLine(buffer);

	if (buffer == KNullDesC)
		return EActionTest;
	
	TLex lex(buffer);
	TInt result = 0;
	while (!lex.Eos())
		{
		TPtrC word = lex.NextToken();
		if (word == _L("test"))
			result |= EActionTest;
		else if (word == _L("thrashing"))
			result |= EActionThrashing;
		else if (word == _L("benchmarks"))
			result |= EActionBenchmarks;
		else
			{
			test.Printf(_L("bad token '%S'\n"), &word);
			BadUsage();
			}
		}
	
	return result;
	}

TInt E32Main()
	{
	test.Title();
	test.Start(_L("Test thrashing monitor"));
	
	test_KErrNone(InitBenchmarks());

	TInt actions = ParseCommandLine();
	
	test_KErrNone(GetGlobalPolicies());

	TUint cacheOriginalMin = 0;
	TUint cacheOriginalMax = 0;

	if (gDataPagingSupported)
		{
		test.Next(_L("Thrash test: change cache size to maximum 2Mb"));
		TUint cacheCurrentSize = 0;
		DPTest::CacheSize(cacheOriginalMin, cacheOriginalMax, cacheCurrentSize);
		gMinCacheSize = 512;
		gMaxCacheSize = 520;
		test_KErrNone(DPTest::SetCacheSize(gMinCacheSize * gPageSize, gMaxCacheSize * gPageSize));
		}
	
	if (actions & EActionTest)
		{
		TBool flexibleMemoryModel = (MemModelAttributes() & EMemModelTypeMask) == EMemModelTypeFlexible;
		if (flexibleMemoryModel)
			TestThrashHal();
		else
			TestThrashHalNotSupported();
		}

	if (actions & EActionThrashing)
		{	
		test.Next(_L("Extended thrashing tests"));
		TestThrashing();
		}
	
	if (actions & EActionBenchmarks)
		{	
		test.Next(_L("Benchmarking page replacement"));
		TestDistributions();
		BenchmarkReplacement();
		}

	if (gDataPagingSupported)
		{
		test.Next(_L("Thrash test: Reset cache size to normal"));
		test_KErrNone(DPTest::SetCacheSize(cacheOriginalMin, cacheOriginalMax));
		}
	
	test.End();
	return 0;
	}
