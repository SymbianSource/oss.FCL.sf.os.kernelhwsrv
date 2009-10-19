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

#include "t_dpcmn.h"
#include "../mmu/mmudetect.h"
#include "../mmu/d_memorytest.h"
#include "../mmu/t_codepaging_dll.h"

RTest test(_L("T_THRASH"));

volatile TBool gRunThrashTest = EFalse;

_LIT(KChunkName, "t_thrash chunk");

class TRandom
	{
public:
	TRandom();
	TUint32 Next();

private:
	enum
		{
		KA = 1664525,
		KB = 1013904223
		};
	TUint32 iV;
	};

TRandom::TRandom()
	{
	iV = (TUint32)this + RThread().Id() + User::FastCounter() + 23;
	}

TUint32 TRandom::Next()
	{
	iV = KA * iV + KB;
	return iV;
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

enum TWorkload
	{
	EWorkloadSequential,
	EWorkloadRandom,
	EWorkloadShuffle
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

	TRandom random;
	TInt startPage = args->iThreadGroup * args->iGroupSize;
	TInt* ptr = (TInt*)(args->iBasePtr + startPage * gPageSize);
	switch (args->iWorkload)
		{
		case EWorkloadSequential:
			while (gRunThrashTest)
				{
				TInt size = (args->iPageCount * gPageSize) / sizeof(TInt);
				for (TInt i = 0 ; i < size && gRunThrashTest ; ++i)
					{
					ptr[i] = random.Next();
					__e32_atomic_add_ord64(&args->iAccesses, 1);
					}
				}
			break;
				
		case EWorkloadRandom:
			{
			TInt acc = 0;
			while (gRunThrashTest)
				{
				TInt size = (args->iPageCount * gPageSize) / sizeof(TInt);
				for (TInt i = 0 ; i < size && gRunThrashTest ; ++i)
					{
					TUint32 rand = random.Next();
					TInt action = rand >> 31;
					TInt r = rand % size;
					if (action == 0)
						acc += ptr[r];
					else
						ptr[r] = acc;
					__e32_atomic_add_ord64(&args->iAccesses, 1);
					}
				}
			}
			break;
			
		case EWorkloadShuffle:
			{
			TInt i;
			while (gRunThrashTest)
				{
				TInt size = (args->iPageCount * gPageSize) / sizeof(TInt);
				for (i = 0 ; gRunThrashTest && i < (size - 1) ; ++i)
					{
					Mem::Swap(&ptr[i], &ptr[i + random.Next() % (size - i - 1) + 1], sizeof(TInt));
					__e32_atomic_add_ord64(&args->iAccesses, 2);
					}
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

void ThrashTest(TInt aThreads,			// number of threads to run
				TBool aSharedData,		// whether all threads share the same data
				TWorkload aWorkload,
				TInt aBeginPages,		// number of pages to start with for last/all threads
				TInt aEndPages,			// number of pages to end with for last/all threads
				TInt aOtherPages)		// num of pages for other threads, or zero to use same value for all
	{
	RDebug::Printf("\nPages Accesses     ThL");

	DPTest::FlushCache();
	User::After(1000000);

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
	TInt pageCount = aBeginPages;
	const TInt maxSteps = 30;
	TInt step = aEndPages >= aBeginPages ? Max((aEndPages - aBeginPages) / maxSteps, 1) : Min((aEndPages - aBeginPages) / maxSteps, -1);
	
	TInt i;
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
		
		User::After(2000000);		
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

		test.Printf(_L("%5d %12ld %3d"), totalPages, totalAccesses, thrashLevel);
		for (i = 0 ; i < aThreads ; ++i)
			{
			test.Printf(_L(" %5d %12ld"),
						threads[i].iArgs.iPageCount,
						__e32_atomic_load_acq64(&threads[i].iArgs.iAccesses));
			test_Equal(KRequestPending, threads[i].iStatus.Int());
			}
		test.Printf(_L("\n"));

		if (aEndPages >= aBeginPages ? pageCount >= aEndPages : pageCount < aEndPages)
			break;
		pageCount += step;
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
	RDebug::Printf("\n");
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
	test.Next(_L("Thrash test: single thread, sequential workload"));
	ThrashTest(1, ETrue, EWorkloadSequential, minPages, maxPages, 0);
	
	test.Next(_L("Thrash test: single thread, random workload"));
	ThrashTest(1, ETrue, EWorkloadRandom, minPages, maxPages, 0);
	
	test.Next(_L("Thrash test: single thread, shuffle workload"));
	ThrashTest(1, ETrue, EWorkloadShuffle, minPages, maxPages, 0);

	// Multiple threads with shared data, one thread incresing in size
	test.Next(_L("Thrash test: two threads with shared data, one thread increasing, random workload"));
	ThrashTest(2, ETrue, EWorkloadRandom, minPages, maxPages, minPages);
	
	test.Next(_L("Thrash test: four threads with shared data, one thread increasing, random workload"));
	ThrashTest(4, ETrue, EWorkloadRandom, minPages, maxPages, minPages);

	// Multiple threads with shared data, all threads incresing in size
	test.Next(_L("Thrash test: two threads with shared data, all threads increasing, random workload"));
	ThrashTest(2, ETrue, EWorkloadRandom, minPages, maxPages, 0);
	
	test.Next(_L("Thrash test: four threads with shared data, all threads increasing, random workload"));
	ThrashTest(4, ETrue, EWorkloadRandom, minPages, maxPages, 0);
	
	// Multiple threads with independent data, one thread incresing in size
	test.Next(_L("Thrash test: two threads with independent data, one thread increasing, random workload"));
	ThrashTest(2, EFalse, EWorkloadRandom, minPages2, maxPages2, gMaxCacheSize / 2);
	
	test.Next(_L("Thrash test: four threads with independent data, one thread increasing, random workload"));
	ThrashTest(4, EFalse, EWorkloadRandom, minPages4, maxPages4, gMaxCacheSize / 4);
	
	// Multiple threads with independant data, all threads incresing in size
	test.Next(_L("Thrash test: two threads with independent data, all threads increasing, random workload"));
	ThrashTest(2, EFalse, EWorkloadRandom, minPages2, maxPages2, 0);

	test.Next(_L("Thrash test: four threads with independent data, all threads increasing, random workload"));
	ThrashTest(4, EFalse, EWorkloadRandom, minPages4, maxPages4, 0);

	// Attempt to create thrash state where there is sufficient cache
	test.Next(_L("Thrash test: two threads with independent data, one threads decreasing, random workload"));
	TInt halfCacheSize = gMaxCacheSize / 2;
	ThrashTest(2, EFalse, EWorkloadRandom, halfCacheSize + 10, halfCacheSize - 30, halfCacheSize);
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
	ThrashTest(1, ETrue, EWorkloadRandom, gMaxCacheSize * 2, gMaxCacheSize * 2 + 5, 0);
	r = UserSvr::HalFunction(EHalGroupVM, EVMHalGetThrashLevel, 0, 0);
	test(r >= 0 && r <= 255);
	test.Printf(_L("Thrash level == %d\n"), r);
	test(r > 200);  // should indicate thrashing
	test_Equal(EChangesThrashLevel, status.Int());
	User::WaitForAnyRequest();

	// wait for system to calm down and check notification again
	test_KErrNone(notifier.Logon(status));
	User::WaitForAnyRequest();
	test_Equal(EChangesThreadDeath, status.Int());

	test_KErrNone(notifier.Logon(status));
	User::After(2000000);
	r = UserSvr::HalFunction(EHalGroupVM, EVMHalGetThrashLevel, 0, 0);
	test(r >= 0 && r <= 255);
	test.Printf(_L("Thrash level == %d\n"), r);
	test(r <= 10);  // should indicate lightly loaded system
	test_Equal(EChangesThrashLevel, status.Int());	
	User::WaitForAnyRequest();
	}

void TestThrashHalNotSupported()
	{
	test_Equal(KErrNotSupported, UserSvr::HalFunction(EHalGroupVM, EVMHalGetThrashLevel, 0, 0));
	test_Equal(KErrNotSupported, UserSvr::HalFunction(EHalGroupVM, EVMHalSetThrashThresholds, 0, 0));
	}

TInt E32Main()
	{
	test.Title();
	test.Start(_L("Test thrashing monitor"));

	test_KErrNone(GetGlobalPolicies());

	TBool flexibleMemoryModel = (MemModelAttributes() & EMemModelTypeMask) == EMemModelTypeFlexible;
	if (flexibleMemoryModel)
		TestThrashHal();
	else
		TestThrashHalNotSupported();
	
	if (gDataPagingSupported && User::CommandLineLength() > 0)
		{		
		test.Next(_L("Extended thrashing tests"));
		TestThrashing();
		}

	test.End();
	return 0;
	}
