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
// e32test\demandpaging\t_datapaging.cpp
// Functional tests for data paging.
// 002 Test UserHeap::ChunkHeap data paging attributes
// 003 Test RThread::Create data paging attributes
// 
//

//! @SYMTestCaseID			KBASE-T_DATAPAGING
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1954
//! @SYMTestCaseDesc		Data Paging functional tests.
//! @SYMTestActions			001 Test RChunk data paging attributes
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <dptest.h>
#include <e32hal.h>
#include <u32exec.h>
#include <e32svr.h>
#include <e32panic.h>
#include "u32std.h"
#include <e32msgqueue.h>
#include <e32atomics.h>
#include <e32math.h>
#include <f32file.h>
#include "t_dpcmn.h"
#include "../mmu/mmudetect.h"
#include "../mmu/d_memorytest.h"
#include "../mmu/paging_info.h"

_LIT(KChunkName, "t_datapaging chunk");

RTest test(_L("T_DATAPAGING"));
SVMSwapInfo InitialSwapInfo;

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
	iV = RThread().Id() + User::NTickCount() + 23;
	}

TUint32 TRandom::Next()
	{
	iV = KA * iV + KB;
	return iV;
	}

void CreatePagedChunk(TInt aSizeInPages, TInt aWipeByte = -1)
	{
	test_Equal(0,gChunk.Handle());
	
	TChunkCreateInfo createInfo;
	TInt size = aSizeInPages * gPageSize;
	createInfo.SetNormal(size, size);
	createInfo.SetPaging(TChunkCreateInfo::EPaged);
	createInfo.SetOwner(EOwnerProcess);
	createInfo.SetGlobal(KChunkName);
	if (aWipeByte != -1)
		createInfo.SetClearByte(aWipeByte);
	test_KErrNone(gChunk.Create(createInfo));
	test(gChunk.IsPaged()); // this is only ever called if data paging is supported
	}

// The contents of a page is represented as type from enum below ORed with a byte value
enum TPageContent
	{
	ETypeUniform    = 0 << 8,
	ETypeIncreasing = 1 << 8,

	EContentValueMask = 255,
	EContentTypeMask  = 255 << 8
	};

// Write to a page to page it in and verify its previous contents
void WritePage(TInt aIndex, TUint aExpectedContents, TUint aNewContents)
	{
	test.Printf(_L("  %3d Write %x\n"), aIndex, aNewContents);
	
	TUint oldType = aExpectedContents & EContentTypeMask;
	TUint oldValue = aExpectedContents & EContentValueMask;
	
	TUint type = aNewContents & EContentTypeMask;
	TUint value = aNewContents & EContentValueMask;
	
	TUint8* page = gChunk.Base() + (gPageSize * aIndex);

	// write first byte first so page is paged in or rejuvenated with write permissions
	page[0] = 0;
	
	for (TInt i = 0 ; i < gPageSize ; ++i)
		{
		if (i != 0)
			test_Equal(oldValue, page[i]);
		if (oldType == ETypeIncreasing)
			oldValue = (oldValue + 1) & 255;
		
		page[i] = value;
		if (type == ETypeIncreasing)
			value = (value + 1) & 255;
		}
	}

// Read a page and verify its contents
void ReadPage(TInt aIndex, TUint aExpectedContents)
	{
	test.Printf(_L("  %3d Read  %x\n"), aIndex, aExpectedContents);
	TUint type = aExpectedContents & EContentTypeMask;
	TUint value = aExpectedContents & EContentValueMask;
	TUint8* page = gChunk.Base() + (gPageSize * aIndex);
	for (TInt i = 0 ; i < gPageSize ; ++i)
		{
		test_Equal(value, page[i]);
		if (type == ETypeIncreasing)
			value = (value + 1) & 255;
		}
	}

void PageOut()
	{
	test.Printf(_L("      PageOut\n"));
	DPTest::FlushCache();
	}

void TestOnePage()
	{
	CreatePagedChunk(1, 0xed);

	// Test initial contents (read)
	ReadPage(0, ETypeUniform | 0xed);

	// Test read initial contents after flush (may or may not actually been paged out)
	PageOut();
	ReadPage(0, ETypeUniform | 0xed);

	// Test page out / page in (read) of dirty contents
	WritePage(0, ETypeUniform | 0xed, ETypeIncreasing | 0x1a);
	PageOut();
	ReadPage(0, ETypeIncreasing | 0x1a);

	// Test page out / page in (read) of clean contents
	PageOut();
	ReadPage(0, ETypeIncreasing | 0x1a);
 
	// Test page out / page in (write) of dirty contents
	WritePage(0, ETypeIncreasing | 0x1a, ETypeIncreasing | 0x23);
	PageOut();
	WritePage(0, ETypeIncreasing | 0x23, ETypeIncreasing | 0x45);

	CLOSE_AND_WAIT(gChunk);
	CreatePagedChunk(1, 0x0d);

	// Test initial contents (write)
	WritePage(0, ETypeUniform | 0x0d, ETypeIncreasing | 0x1a);

	// Test page out / page in (read) of dirty contents
	PageOut();
	ReadPage(0, ETypeIncreasing | 0x1a);
	
	CLOSE_AND_WAIT(gChunk);
	}

TInt PageInThreadFunc(TAny* aArg)
	{
	TUint8* page = (TUint8*)aArg;
	for (;;)
		{
		DPTest::FlushCache();
		RDebug::Printf("Start page in...");
		volatile TInt i = page[0];
		(void)i;
		RDebug::Printf("  done.");
		}
	}

TInt PageOutThreadFunc(TAny* aArg)
	{
	TUint8* page = (TUint8*)aArg;
	for (;;)
		{
		page[0] = 1;  // make page dirty
		RDebug::Printf("Start page out...");
		DPTest::FlushCache();
		RDebug::Printf("  done.");
		}
	}

void TestKillThread(TThreadFunction aFunc, TInt aIterations)
	{
	__KHEAP_MARK;
	TRandom random;
	CreatePagedChunk(1);
	TUint8* page = gChunk.Base();
	page[0] = 0;  // make page dirty
	DPTest::FlushCache();
	for (TInt i = 0 ; i < aIterations ; ++i)
		{
		RThread thread;
		test_KErrNone(thread.Create(KNullDesC, aFunc, gPageSize, NULL, page));
		TRequestStatus status;
		thread.Logon(status);
		thread.Resume();
		User::AfterHighRes((random.Next() % 50 + 1) * 1000);
		thread.Kill(123);
		User::WaitForRequest(status);
		test_Equal(123, status.Int());
		CLOSE_AND_WAIT(thread);
		}
	CLOSE_AND_WAIT(gChunk);
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
	__KHEAP_MARKEND;
	}

struct SSoakTestArgs
	{
	TInt iThreadIndex;
	TInt iPages;
	};

TUint32* PageBasePtr(TInt aPage)
	{
	return (TUint32*)(gChunk.Base() + (gPageSize * aPage));
	}

TUint32* PageDataPtr(TInt aPage, TInt aThreadIndex)
	{
	return (TUint32*)((TUint8*)PageBasePtr(aPage) + ((aThreadIndex * 2 + 1) * sizeof(TUint32)));
	}

TUint32 PageTag(TInt aPage)
	{
	return 0x80000000 | aPage;
	}	

void StopSoakTest(RMsgQueue<TInt> aMsgQueue)
	{
	while(aMsgQueue.Send(0) != KErrOverflow)
		;
	}

TBool ContinueSoakTest(RMsgQueue<TInt> aMsgQueue)
	{
	TInt msg;
	return aMsgQueue.Receive(msg) == KErrUnderflow;
	}

_LIT(KMsgQueueName, "t_datapaging_queue");

TInt PinPagesFunc(TAny* aArg)
	{
	SSoakTestArgs* args = (SSoakTestArgs*)aArg;

	RMemoryTestLdd ldd;
	TInt r = ldd.Open();
	if (r != KErrNone)
		return r;
	r = ldd.CreateVirtualPinObject();
	if (r != KErrNone)
		return r;

	RMsgQueue<TInt> msgQueue;
	r = msgQueue.OpenGlobal(KMsgQueueName, EOwnerThread);
	if (r != KErrNone)
		return r;

	TInt i = 0;
	TRandom random;
	while (ContinueSoakTest(msgQueue))
		{
		TInt count = 1 + random.Next() % (args->iPages / 4);
		TInt start = random.Next() % (args->iPages - count);
		TInt sleepInMs = 1 + random.Next() % 20;
		TUint32* ptr = PageBasePtr(start);

		r = ldd.PinVirtualMemory((TLinAddr)ptr, count * gPageSize);
		if (r != KErrNone)
			return r;

		User::AfterHighRes(sleepInMs * 1000);

		r = ldd.UnpinVirtualMemory();
		if (r != KErrNone)
			return r;
	
		++i;
		}

	msgQueue.Close();

	r = ldd.DestroyVirtualPinObject();
	if (r != KErrNone)
		return r;
	ldd.Close();
					
	RDebug::Printf("  thread %d performed %d iterations (pinning)", args->iThreadIndex, i);
	return KErrNone;
	}

TBool TestReadWord(TUint32* aPtr, TUint32 aExpected, TInt aThread, TInt aPage, TInt aIteration, TInt aLine, RMsgQueue<TInt> aMsgQueue)
	{
	TUint32 aActual = *aPtr;
	if (aActual != aExpected)
		{
		StopSoakTest(aMsgQueue);
		RDebug::Printf("  thread %d failure reading page %d at iteration %d address %08x line %d: expected %08x but got %08x",
					   aThread, aPage, aIteration, aPtr, aLine, aExpected, aActual);
		return EFalse;
		}
	return ETrue;
	}

TInt SoakTestFunc(TAny* aArg)
	{
	SSoakTestArgs* args = (SSoakTestArgs*)aArg;

	RMsgQueue<TInt> msgQueue;
	TInt r = msgQueue.OpenGlobal(KMsgQueueName, EOwnerThread);
	if (r != KErrNone)
		return r;

	TUint32* contents = new TUint32[args->iPages];
	if (contents == NULL)
		return KErrNoMemory;
	Mem::Fill(contents, args->iPages * sizeof(TUint32), 0);

	TInt i = 0;
	TRandom random;
	while (ContinueSoakTest(msgQueue))
		{
		TUint32 rand = random.Next();
		TInt page = rand % args->iPages;
		TUint32* ptr = PageDataPtr(page, args->iThreadIndex);
		TInt action = rand >> 31;
		if (action == 0)
			{
			if (!TestReadWord(PageBasePtr(page), PageTag(page), args->iThreadIndex, page, i, __LINE__, msgQueue))
				return KErrGeneral;
			if (!TestReadWord(&ptr[0], contents[page], args->iThreadIndex, page, i, __LINE__, msgQueue))
				return KErrGeneral;
			if (!TestReadWord(&ptr[1], contents[page], args->iThreadIndex, page, i, __LINE__, msgQueue))
				return KErrGeneral;
			}
		else
			{
			TUint newContents = args->iThreadIndex+0x100+(contents[page]&~0xff);
			ptr[0] = newContents;
			if (!TestReadWord(PageBasePtr(page), PageTag(page), args->iThreadIndex, page, i, __LINE__, msgQueue))
				return KErrGeneral;
			if (!TestReadWord(&ptr[1], contents[page], args->iThreadIndex, page, i, __LINE__, msgQueue))
				return KErrGeneral;
			ptr[1] = newContents;
			contents[page] = newContents;
			}
		++i;
		}
	
	for (TInt j = 0 ; j < args->iPages ; ++j)
		{
		TUint32* ptr = PageDataPtr(j, args->iThreadIndex);
		if (!TestReadWord(PageBasePtr(j), PageTag(j), args->iThreadIndex, j, i, __LINE__, msgQueue))
			return KErrGeneral;
		if (!TestReadWord(&ptr[0], contents[j], args->iThreadIndex, j, i, __LINE__, msgQueue))
			return KErrGeneral;
		if (!TestReadWord(&ptr[1], contents[j], args->iThreadIndex, j, i, __LINE__, msgQueue))
			return KErrGeneral;
		}

	delete [] contents;
	msgQueue.Close();

	RDebug::Printf("  thread %d performed %d iterations", args->iThreadIndex, i);
	return KErrNone;
	}

TInt SoakProcess(TInt aProcessIndex, TInt aThreads, TInt aPages, TBool aPinPages)
	{
	TInt pinThreadIndex = aPinPages ? aThreads++ : -1;

	test_KErrNone(gChunk.OpenGlobal(KChunkName, EFalse));
	
	SSoakTestArgs* testArgs = new SSoakTestArgs[aThreads];
	test_NotNull(testArgs);
			
	RThread* threads = new RThread[aThreads];
	test_NotNull(threads);
	
	TRequestStatus* statuses = new TRequestStatus[aThreads];
	test_NotNull(statuses);
	
	TInt i;
	for (i = 0 ; i < aThreads ; ++i)
		{
		testArgs[i].iThreadIndex = aProcessIndex * aThreads + i;
		testArgs[i].iPages = aPages;
		TThreadFunction func = i == pinThreadIndex ? PinPagesFunc : SoakTestFunc;
		test_KErrNone(threads[i].Create(KNullDesC, func, gPageSize, NULL, &testArgs[i]));
		threads[i].Logon(statuses[i]);
		}

	// todo: rendezvous here?
	
	for (i = 0 ; i < aThreads ; ++i)
		threads[i].Resume();
	
	TBool ok = ETrue;		
	for (i = 0 ; i < aThreads ; ++i)
		{
		User::WaitForRequest(statuses[i]);
		if (threads[i].ExitType() != EExitKill || statuses[i].Int() != KErrNone)
			ok = EFalse;
		threads[i].Close();
		}
	
	delete [] testArgs;
	delete [] threads;
	delete [] statuses;
	gChunk.Close();
	
	return ok ? KErrNone : KErrGeneral;
	}

TInt RunSoakProcess()
	{
	TBuf<80> buf;
	if (User::CommandLineLength() > buf.MaxLength())
		return KErrArgument;
	User::CommandLine(buf);
	TLex lex(buf);

	TInt index;
	TInt r = lex.Val(index);
	if (r != KErrNone)
		return r;
	lex.SkipSpace();

	TInt threads;
	r = lex.Val(threads);
	if (r != KErrNone)
		return r;
	lex.SkipSpace();
	
	TInt pages;
	r = lex.Val(pages);
	if (r != KErrNone)
		return r;
	lex.SkipSpace();
	
	TBool pinPages;
	r = lex.Val(pinPages);
	if (r != KErrNone)
		return r;
	
	return SoakProcess(index, threads, pages, pinPages);
	}

void SoakTest(TInt aProcesses, TInt aThreads, TInt aPages, TBool aPinPages, TInt aDurationInSeconds)
	{
	RDebug::Printf("Soak test: %d processes, %d threads, %d pages, %s pinning for %d seconds",
				   aProcesses, aThreads, aPages, (aPinPages ? "with" : "without"), aDurationInSeconds);
	DPTest::FlushCache();
	
	TInt totalThreads = (aThreads + (aPinPages ? 1 : 0)) * aProcesses;
	test(totalThreads < 512); // each thread uses two words in a page

	TMediaPagingStats dummy=EMediaPagingStatsRomAndCode;
	PagingInfo::ResetBenchmarks(-1, dummy);	// Don't worry about locmedia stats.

	RMsgQueue<TInt> msgQueue;
	test_KErrNone(msgQueue.CreateGlobal(KMsgQueueName, totalThreads, EOwnerThread));

	CreatePagedChunk(aPages, 0);
	TInt i;
	for (i = 0 ; i < aPages ; ++i)
		*PageBasePtr(i) = PageTag(i);
			
	RProcess* processes = new RProcess[aProcesses];
	TRequestStatus* statuses = new TRequestStatus[aProcesses];
	for (i = 0 ; i < aProcesses ; ++i)
		{
		TBuf<80> args;
		args.AppendFormat(_L("%d %d %d %d"), i, aThreads, aPages, aPinPages);
		test_KErrNone(processes[i].Create(_L("t_datapaging"), args));
		processes[i].Logon(statuses[i]);
		}

	RThread().SetPriority(EPriorityMore); // so we don't get starved of CPU by worker threads

	for (i = 0 ; i < aProcesses ; ++i)
		processes[i].Resume();

	User::After(aDurationInSeconds * 1000000);
	StopSoakTest(msgQueue);
	
	TBool ok = ETrue;		
	for (i = 0 ; i < aProcesses ; ++i)
		{
		User::WaitForRequest(statuses[i]);
		if (processes[i].ExitType() != EExitKill || statuses[i].Int() != KErrNone)
			{
			ok = EFalse;
			RDebug::Printf("  process %i died with %d,%d", i, processes[i].ExitType(), statuses[i].Int());
			}
		processes[i].Close();
		}

	RThread().SetPriority(EPriorityNormal);

	if (!ok)
		{
		for (i = 0 ; i < aPages ; ++i)
			{
			test.Printf(_L("%3d %08x"), i, *PageBasePtr(i));
			for (TInt j = 0 ; j < totalThreads ; ++j)
				{
				TUint32* ptr = PageDataPtr(i, j);
				test.Printf(_L(" %08x,%08x"), ptr[0], ptr[1]);
				}
			test.Printf(_L("\n"), i);
			}
		}
	test(ok);	

	gChunk.Close();
	
	User::After(1000000);
	RDebug::Printf("  done");
	RDebug::Printf("\n");
	
	msgQueue.Close();
	delete [] processes;
	delete [] statuses;

	PagingInfo::PrintBenchmarks(-1, dummy);	// Don't worry about locmedia stats.
	}

void CommitPage(RChunk chunk, TInt aPageIndex)
	{
	test_KErrNone(chunk.Commit(aPageIndex * gPageSize, gPageSize));
	}

void DecommitPage(RChunk chunk, TInt aPageIndex)
	{
	test_KErrNone(chunk.Decommit(aPageIndex * gPageSize, gPageSize));
	}

void WaitForNotifiers()
	{
	// wait until notifiers have had chance to signal us...
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
	}

void TestSwapHal()
	{
	test.Next(_L("Test EVMHalGetSwapInfo"));

	TChunkCreateInfo createInfo;
	createInfo.SetDisconnected(0, 0, 256 * gPageSize);
	createInfo.SetPaging(TChunkCreateInfo::EPaged);
	RChunk chunk;
	test_KErrNone(chunk.Create(createInfo));
	if (gDataPagingSupported)
		test(chunk.IsPaged());
	
	SVMSwapInfo swapInfo;
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalGetSwapInfo, &swapInfo, 0));
	test(swapInfo.iSwapFree <= swapInfo.iSwapSize);
	test.Printf(_L("  Swap size == 0x%x bytes\n"), swapInfo.iSwapSize);
	test.Printf(_L("  Swap free == 0x%x bytes\n"), swapInfo.iSwapFree);
	test(swapInfo.iSwapSize != 0);
	InitialSwapInfo = swapInfo;
		
	CommitPage(chunk, 0);
	SVMSwapInfo swapInfo2;
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalGetSwapInfo, &swapInfo2, 0));
	test_Equal(swapInfo.iSwapSize, swapInfo2.iSwapSize);
	test_Equal(swapInfo.iSwapFree - gPageSize, swapInfo2.iSwapFree);
		
	DecommitPage(chunk, 0);
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalGetSwapInfo, &swapInfo2, 0));
	test_Equal(swapInfo.iSwapSize, swapInfo2.iSwapSize);
	test_Equal(swapInfo.iSwapFree, swapInfo2.iSwapFree);

	// Test that closing the chunk releases the swap page.
	CommitPage(chunk, 0);
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalGetSwapInfo, &swapInfo2, 0));
	test_Equal(swapInfo.iSwapSize, swapInfo2.iSwapSize);
	test_Equal(swapInfo.iSwapFree - gPageSize, swapInfo2.iSwapFree);
		
	chunk.Close();
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalGetSwapInfo, &swapInfo2, 0));
	test_Equal(swapInfo.iSwapSize, swapInfo2.iSwapSize);
	test_Equal(swapInfo.iSwapFree, swapInfo2.iSwapFree);

	// Chunk must be created for rest of testing.
	test_KErrNone(chunk.Create(createInfo));
	if (gDataPagingSupported)
		test(chunk.IsPaged());
	
	//	EVMHalSetSwapThresholds,
	test.Next(_L("Test EVMHalSetSwapThresholds"));
	SVMSwapThresholds thresholds;
	thresholds.iLowThreshold = 1;
	thresholds.iGoodThreshold = 0;
	test_Equal(KErrArgument, UserSvr::HalFunction(EHalGroupVM, EVMHalSetSwapThresholds, &thresholds, 0));
	thresholds.iLowThreshold = swapInfo.iSwapSize + 1;
	thresholds.iGoodThreshold = swapInfo.iSwapSize + 1;
	test_Equal(KErrArgument, UserSvr::HalFunction(EHalGroupVM, EVMHalSetSwapThresholds, &thresholds, 0));
	thresholds.iLowThreshold = 0;
	thresholds.iGoodThreshold = 0;
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalSetSwapThresholds, &thresholds, 0));
	thresholds.iLowThreshold = swapInfo.iSwapSize;
	thresholds.iGoodThreshold = swapInfo.iSwapSize;
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalSetSwapThresholds, &thresholds, 0));

	// test thresholds trigger ok
	
	RChangeNotifier changes;
	test_KErrNone(changes.Create());
	TRequestStatus status;
	test_KErrNone(changes.Logon(status));
	User::WaitForRequest(status);
	test_KErrNone(changes.Logon(status));
	test_Equal(KRequestPending, status.Int());
	
	thresholds.iLowThreshold = swapInfo.iSwapFree - 2 * gPageSize;
	thresholds.iGoodThreshold = swapInfo.iSwapFree - gPageSize;
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalSetSwapThresholds, &thresholds, 0));

	CommitPage(chunk, 0);
	CommitPage(chunk, 1);
	WaitForNotifiers();
	test_Equal(KRequestPending, status.Int());
	CommitPage(chunk, 2);
	WaitForNotifiers();
	test_Equal(EChangesFreeMemory | EChangesLowMemory, status.Int());
	User::WaitForRequest(status);
	
	test_KErrNone(changes.Logon(status));
	DecommitPage(chunk, 2);
	WaitForNotifiers();
	test_Equal(KRequestPending, status.Int());
	DecommitPage(chunk, 1);
	WaitForNotifiers();
	test_Equal(EChangesFreeMemory, status.Int());
	User::WaitForRequest(status);
	DecommitPage(chunk, 0);
	
	CLOSE_AND_WAIT(changes);

	// leave some sensible thresholds set
	thresholds.iLowThreshold = (10 * swapInfo.iSwapSize) / 100;
	thresholds.iGoodThreshold = (20 * swapInfo.iSwapSize) / 100;
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalSetSwapThresholds, &thresholds, 0));

	CLOSE_AND_WAIT(chunk);
	}

void TestSwapInfoUnchanged()
	{
	SVMSwapInfo swapInfo;
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalGetSwapInfo, &swapInfo, 0));
	test.Printf(_L("  Swap size == 0x%x bytes\n"), swapInfo.iSwapSize);
	test.Printf(_L("  Swap free == 0x%x bytes\n"), swapInfo.iSwapFree);
	test_Equal(InitialSwapInfo.iSwapSize, swapInfo.iSwapSize);
	test_Equal(InitialSwapInfo.iSwapFree, swapInfo.iSwapFree);
	}

void TestSwapHalNotSupported()
	{
	test_Equal(KErrNotSupported, UserSvr::HalFunction(EHalGroupVM, EVMHalGetSwapInfo, 0, 0));
	test_Equal(KErrNotSupported, UserSvr::HalFunction(EHalGroupVM, EVMHalSetSwapThresholds, 0, 0));
	}

void TestHal()
	{
	if (gDataPagingSupported)
		TestSwapHal();
	else
		TestSwapHalNotSupported();
	}


TBool gStealEnable = false;

TInt DecommitThread(TAny*)
	{
	RThread().SetPriority(EPriorityLess); // so this thread gets pre-empted by StealThread
	TUint8* base = gChunk.Base();
	TInt size = gChunk.MaxSize();
	for(;;)
		{
		// dirty all pages
		for(TInt i=0; i<size; i+=gPageSize)
			base[i] = 0;
		// free pages...
		gStealEnable = true;
		gChunk.Adjust(0);
		gStealEnable = false;
		// recommit pages...
		TInt r = gChunk.Adjust(size);
		if(r!=KErrNone)
			return r; // error
		}
	}


TInt StealThread(TAny*)
	{
	for(;;)
		{
		while(!gStealEnable)
			User::AfterHighRes(0);
		DPTest::FlushCache();
		}
	}


void TestDecommitAndStealInteraction(TInt aSeconds)
	{
	__KHEAP_MARK;

	CreatePagedChunk(256);

	RThread thread1;
	test_KErrNone(thread1.Create(_L("DecommitThread"), DecommitThread, gPageSize, NULL, 0));
	TRequestStatus status1;
	thread1.Logon(status1);

	RThread thread2;
	test_KErrNone(thread2.Create(_L("StealThread"), StealThread, gPageSize, NULL, 0));
	TRequestStatus status2;
	thread1.Logon(status2);

	RTimer timer;
	test_KErrNone(timer.CreateLocal());
	TRequestStatus timeoutStatus;
	timer.After(timeoutStatus,aSeconds*1000000);

	thread1.Resume();
	thread2.Resume();
	User::WaitForAnyRequest();

	thread1.Kill(123);
	User::WaitForRequest(status1);
	test_Equal(123, status1.Int());
	CLOSE_AND_WAIT(thread1);

	thread2.Kill(123);
	User::WaitForRequest(status2);
	test_Equal(123, status2.Int());
	CLOSE_AND_WAIT(thread2);

	CLOSE_AND_WAIT(timer);
	test_KErrNone(timeoutStatus.Int());
	
	CLOSE_AND_WAIT(gChunk);
	
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
	
	__KHEAP_MARKEND;
	}

TInt ThreadAtomic64Flush(TAny*)
	{
	TInt64 seed = 0x33333333;
	FOREVER
		{
		DPTest::FlushCache();
		User::After(Math::Rand(seed) & 0x48);
		}
	}

enum TAtomic64Test
	{
	EAtomic64Add,
	EAtomic64Logic,
	EAtomic64Cas,
	EAtomic64Steps,
	};

struct SAtomic64Args
	{
	TUint iIters;
	TUint64* iData;
	TInt iIncs;
	TUint iClears[64];
	TUint iSets[64];
	};


TInt ThreadAtomic64Cas(TAny* aArgs)
	{
	SAtomic64Args& args = *(SAtomic64Args*)aArgs;
	for (TUint i = 0; i < args.iIters; i++)
		{
		TUint64 setMask = UI64LIT(0xffffffffffffffff);
		TUint64 clrMask = 0;
		if (__e32_atomic_cas_ord64(args.iData, &setMask, clrMask))
			args.iClears[0]++;
		// Undo any clearing of setMask which will happen if iData is 0.
		setMask = UI64LIT(0xffffffffffffffff);
		if (__e32_atomic_cas_ord64(args.iData, &clrMask, setMask))
			args.iSets[0]++;
		}
	return KErrNone;
	}


TInt ThreadAtomic64Logic(TAny* aArgs)
	{
	TInt r = KErrNone;
	SAtomic64Args& args = *(SAtomic64Args*)aArgs;
	for(TUint i = 0; i < args.iIters; i++)
		{
		TUint bitNo = (i & 0x3f);
		TUint64 bitMask = ((TUint64)1) << bitNo;
		TUint64 andMask = ~bitMask;

		TUint64 old = __e32_atomic_and_ord64(args.iData, andMask);
		if (old & bitMask)
			args.iClears[bitNo]++;

		old = __e32_atomic_ior_ord64(args.iData, bitMask);
		if (!(old & bitMask))
			args.iSets[bitNo]++;

		old = __e32_atomic_xor_ord64(args.iData, bitMask);
		if (old & bitMask)
			args.iClears[bitNo]++;
		else
			args.iSets[bitNo]++;

		old = __e32_atomic_axo_ord64(args.iData, UI64LIT(0xffffffffffffffff), bitMask);
		if (old & bitMask)
			args.iClears[bitNo]++;
		else
			args.iSets[bitNo]++;
		
		}
	return r;
	}


TInt ThreadAtomic64Add(TAny* aArgs)
	{
	TInt r = KErrNone;
	SAtomic64Args& args = *(SAtomic64Args*)aArgs;
	for(TUint i = 0; i < args.iIters; i++)
		{
		TUint64 old = __e32_atomic_add_ord64(args.iData, 1);
		args.iIncs += 1;
		old = __e32_atomic_tau_ord64(args.iData, 1000, 1, 2);
		args.iIncs += (old >= 1000)? 1 : 2;
		old = __e32_atomic_tas_ord64(args.iData, 1000, 1, -1);
		args.iIncs += (old >= 1000)? 1 : -1;
		}
	return r;
	}


void TestAtomic64()
	{
	CreatePagedChunk(sizeof(TUint64));
	TUint64* data = (TUint64*)gChunk.Base();

	const TUint KThreads = 25;
	RThread threads[KThreads];
	TRequestStatus stats[KThreads];
	SAtomic64Args* args = new SAtomic64Args[KThreads];
	test_NotNull(args);

	for (TInt testStep = EAtomic64Add; testStep < EAtomic64Steps; testStep++)
		{
		switch (testStep)
			{
			case EAtomic64Add:
				test.Next(_L("Test 64-bit atomic addition operations"));
					break;
			case EAtomic64Logic:
				test.Next(_L("Test 64-bit atomic logic operations"));
				break;
			case EAtomic64Cas:
				test.Next(_L("Test 64-bit atomic cas operations"));
				break;
			}
		*data = 0;
		RThread threadFlush;
		test_KErrNone(threadFlush.Create(_L("ThreadAtomicFlush"), ThreadAtomic64Flush, gPageSize, NULL, NULL));
		TRequestStatus status1;
		threadFlush.Logon(status1);
		threadFlush.SetPriority(EPriorityAbsoluteHigh);

		memclr(args, sizeof(SAtomic64Args)*KThreads);
		TUint i = 0;
		for (; i < KThreads; i++)
			{
			args[i].iIters = 10000;
			args[i].iData = data;
			switch (testStep)
				{
				case EAtomic64Add:
					test_KErrNone(threads[i].Create(KNullDesC, ThreadAtomic64Add, gPageSize, NULL, (TAny*)&args[i]));
					break;
				case EAtomic64Logic:
					test_KErrNone(threads[i].Create(KNullDesC, ThreadAtomic64Logic, gPageSize, NULL, (TAny*)&args[i]));
					break;
				case EAtomic64Cas:
					test_KErrNone(threads[i].Create(KNullDesC, ThreadAtomic64Cas, gPageSize, NULL, (TAny*)&args[i]));
					break;
				}
			threads[i].Logon(stats[i]);
			}
		threadFlush.Resume();
		for (i = 0; i < KThreads; i++)
			{
			threads[i].Resume();
			}

		// Wait for add threads to complete and kill flushing thread.
		for (i = 0; i < KThreads; i++)
			{
			User::WaitForRequest(stats[i]);
			test_KErrNone(stats[i].Int());
			}
		threadFlush.Kill(KErrNone);
		User::WaitForRequest(status1);
		test_KErrNone(status1.Int());
		TInt64 expected = 0;
		switch (testStep)
			{
			case EAtomic64Add:
				{
				for (TUint i = 0; i < KThreads; i++)
					{
					threads[i].Close();
					expected += args[i].iIncs;
					}
				break;
				}
			case EAtomic64Logic:
				{
				TUint totalSets[64];
				TUint totalClears[64];
				memclr(totalSets, sizeof(TUint)*64);
				memclr(totalClears, sizeof(TUint)*64);
				for (TUint i = 0; i < KThreads; i++)
					{
					threads[i].Close();
					for (TUint j = 0; j < 64; j++)
						{
						totalSets[j] += args[i].iSets[j];
						totalClears[j] += args[i].iClears[j];
						}
					}
				for (TUint j = 0; j < 64; j++)
					{
					TUint64 bitMask = 1 << j;
					if (totalSets[j] > totalClears[j])
						{
						test_Equal(totalSets[j] - 1, totalClears[j]);
						expected |= bitMask;
						}
					else
						{// Can only clear a bit if it was previously set.
						test_Equal(totalClears[j], totalSets[j]);
						}
					}
				break;
				}
			case EAtomic64Cas:
				{
				TUint totalSets = 0;
				TUint totalClears = 0;
				for (TUint i = 0; i < KThreads; i++)
					{
					threads[i].Close();
					totalSets += args[i].iSets[0];
					totalClears += args[i].iClears[0];
					}
				if (totalSets > totalClears)
					{
					test_Equal(totalSets - 1, totalClears);
					expected = UI64LIT(0xffffffffffffffff);
					}
				else
					{// Can only clear a word if it was previously set.
					test_Equal(totalClears, totalSets);
					}
				break;
				}
			}
		test_Equal(expected, *data);
		CLOSE_AND_WAIT(threadFlush);
		}
	delete[] args;
	CLOSE_AND_WAIT(gChunk);
	}


//
// soak test for writeable paged code...
//

const TUint KCodeStride = 20; // spacing between generated code

void CodeStart(TUint8* aCode, TUint8* aTarget, TUint32 aInit)
	{
#if defined(__CPU_X86)
	aCode[0] = 0xb8; *(TUint32*)&(aCode[1]) = aInit;				// mov eax,aInit
	aCode[5] = 0xe9; *(TUint32*)&(aCode[6]) = aTarget-(aCode+10);	// jmp aTarget
	__ASSERT_COMPILE(KCodeStride>=10);

#elif defined(__CPU_ARM)
	*(TUint32*)&(aCode[0]) = 0xe59f0000;			// ldr r0, [pc, #0]
	TInt32 offset = (aTarget-aCode-4-8)/4;
	if(offset&0xff000000u)
		{
		offset ^= 0xff000000u;
		test_Equal(0,offset&0xff000000u);
		}
	*(TUint32*)&(aCode[4]) = 0xea000000|offset;		// b aTarget
	*(TUint32*)&(aCode[8]) = aInit;					// dcd aInit
	__ASSERT_COMPILE(KCodeStride>=12);

#else
#error Unknown CPU
#endif
	}


void CodeStep(TUint8* aCode, TUint8* aTarget, TUint32 aAdd)
	{
#if defined(__CPU_X86)
	aCode[0] = 0xd1; aCode[1] = 0xc0;								// rol eax, 1
	aCode[2] = 0x05; *(TUint32*)&(aCode[3]) = aAdd;					// add eax, aAdd
	aCode[7] = 0xe9; *(TUint32*)&(aCode[8]) = aTarget-(aCode+12);	// jmp aTarget
	__ASSERT_COMPILE(KCodeStride>=12);

#elif defined(__CPU_ARM)
	*(TUint32*)&(aCode[0]) = 0xe1a00fe0;			// ror r0, r0, #31
	*(TUint32*)&(aCode[4]) = 0xe59f1004;			// ldr r1, [pc, #4]
	*(TUint32*)&(aCode[8]) = 0xe0800001;			// add r0, r0, r1
	TInt32 offset = (aTarget-aCode-12-8)/4;
	if(offset&0xff000000u)
		{
		offset ^= 0xff000000u;
		test_Equal(0,offset&0xff000000u);
		}
	*(TUint32*)&(aCode[12]) = 0xea000000|offset;	// b aTarget
	*(TUint32*)&(aCode[16]) = aAdd;					// dcd aAdd
	__ASSERT_COMPILE(KCodeStride>=20);

#else
#error Unknown CPU
#endif
	}


void CodeEnd(TUint8* aCode)
	{
#if defined(__CPU_X86)
	aCode[0] = 0xc3;						// ret
	__ASSERT_COMPILE(KCodeStride>=1);

#elif defined(__CPU_ARM)
	*(TUint32*)&(aCode[0]) = 0xe12fff1e;	// bx lr
	__ASSERT_COMPILE(KCodeStride>=4);

#else
#error Unknown CPU
#endif
	}


void TestExecutableMemory()
	{
	__KHEAP_MARK;

#if defined(__CPU_ARM)
	const TUint KMaxChunkSize = 31*1024*1024; // ARM branch instruction limit
#else
	const TUint KMaxChunkSize = 1024*1024*1024; // 1GB
#endif
	const TUint KMaxPages = KMaxChunkSize/gPageSize;
	TUint sizeInPages = gMaxCacheSize*2;
	if(sizeInPages>KMaxPages)
		sizeInPages = KMaxPages;

	// create code chunk...
	test.Start(_L("Create code chunk"));
	TChunkCreateInfo createInfo;
	TInt size = sizeInPages * gPageSize;
	createInfo.SetCode(size, size);
	createInfo.SetPaging(TChunkCreateInfo::EPaged);
	createInfo.SetClearByte(0);
	RChunk chunk;
	test_KErrNone(chunk.Create(createInfo));
	test(chunk.IsPaged()); // this is only ever called if data paging is supported
	TUint8* base = chunk.Base();

	// create code path through the pages in the chunk with quadratic distribution...
	test.Next(_L("Weave path"));
	TInt pathLength = 0;
	const TUint maxStepsPerPage = gPageSize/KCodeStride;
	const TInt maxPathLength = sizeInPages*maxStepsPerPage;
	TUint8** path = (TUint8**)User::Alloc(maxPathLength*sizeof(TUint8*));
	test(path!=0);
	for(TUint page=0; page<sizeInPages; ++page)
		{
		TUint step = (maxStepsPerPage-1)*(page*page)/(sizeInPages*sizeInPages)+1;
		do path[pathLength++] = base+page*gPageSize+step*KCodeStride;
		while(--step);
		}
	TUint32 rand = 0x12345678;
	for(TUint scramble=pathLength*4; scramble>0; --scramble)
		{
		// swap random pair of entries on path...
		TUint i = (TUint)(TUint64(TUint64(rand)*TUint64(pathLength))>>32);
		rand = rand*69069+1;
		TUint j = (TUint)(TUint64(TUint64(rand)*TUint64(pathLength))>>32);
		rand = rand*69069+1;
		TUint8* t = path[i];
		path[i] = path[j];
		path[j] = t;
		}

	// write code to generated path...
	test.Next(_L("Write code"));
	TUint32 a = 0;
	TUint32 (*code)() = (TUint32 (*)())path[pathLength-1];
	CodeStart(path[pathLength-1],path[pathLength-2],a);
	while(--pathLength>1)
		{
		rand = rand*69069+1;
		CodeStep(path[pathLength-1],path[pathLength-2],rand);
		a = (a<<1)+(a>>31);
		a += rand;
		}
	CodeEnd(path[0]);
	--pathLength;
	test_Equal(0,pathLength);
	test.Next(_L("IMB"));
	User::IMB_Range(base,base+chunk.Size());

	// run code...
	TMediaPagingStats dummy=EMediaPagingStatsRomAndCode;
	PagingInfo::ResetBenchmarks(-1, dummy);	// Don't worry about locmedia stats.
	test.Next(_L("Execute code"));
	TUint32 result = code();
	test_Equal(a,result);
	PagingInfo::PrintBenchmarks(-1, dummy);	// Don't worry about locmedia stats.

	// cleanup...
	test.Next(_L("Cleanup"));
	User::Free(path);
	CLOSE_AND_WAIT(chunk);

	test.End();

	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
	__KHEAP_MARKEND;
	}



TInt E32Main()
	{
	test_KErrNone(UserHal::PageSizeInBytes(gPageSize));
	
	if (User::CommandLineLength() != 0)
		return RunSoakProcess();
	
	test.Title();
	test_KErrNone(GetGlobalPolicies());

	_LIT(KFileName,"Z:\\Test\\not_data_paged.txt");
	RFs fs;
	RFile file;
	TInt error;
	test(KErrNone == fs.Connect());
	error = file.Open(fs, KFileName, EFileRead);
	TBool isFilePresent = (error == KErrNone);
	file.Close();
	fs.Close();
	test(gDataPagingSupported == !isFilePresent);

	test.Start(_L("Test HAL APIs"));
	TestHal();

	if (gDataPagingSupported)
		{
		test.Next(_L("Test reading and writing to a single page"));
		TestOnePage();

		test.Next(_L("Test 64-bit atomic operations are atomic with paged out data"));
		TestAtomic64();

		test.Next(_L("Test interaction between decommit and steal"));
		TestDecommitAndStealInteraction(10);

		test.Next(_L("Test killing a thread while it's paging in"));
		TestKillThread(PageInThreadFunc, 200);
				
		test.Next(_L("Test killing a thread while it's paging out"));
		TestKillThread(PageOutThreadFunc, 200);
		
		test.Next(_L("Test executable memory"));
		TestExecutableMemory();

		test.Next(_L("Soak tests"));
		DPTest::FlushCache();

		test.Next(_L("Soak test: change maximum cache size to minimal"));
		TUint cacheOriginalMin = 0;
		TUint cacheOriginalMax = 0;
		TUint cacheCurrentSize = 0;
		//store original values
		DPTest::CacheSize(cacheOriginalMin, cacheOriginalMax, cacheCurrentSize);
		gMaxCacheSize = 256;
		gMinCacheSize = 64;
		test_KErrNone(DPTest::SetCacheSize(gMinCacheSize * gPageSize, gMaxCacheSize * gPageSize));

		for (TUint totalThreads = 1 ; totalThreads <= 64 ; totalThreads *= 4)
			{
			for (TUint processes = 1 ; processes <= 16 && processes <= totalThreads ; processes *= 4)
				{
				TUint threads = totalThreads / processes;
				for (TUint pages = gMaxCacheSize / 2 ; pages <= gMaxCacheSize * 2 ; pages *= 2)
					{
					for (TUint pin = 0 ; pin <= 1 ; ++pin)
						{
						test.Printf(_L("processes=%d threads=%d pages=%d maxcachesize=%d pin=%d\r\n"),processes, threads, pages, gMaxCacheSize,pin);
						SoakTest(processes, threads, pages, pin, 5);
						}
					}
				}
			}

		//Reset the cache size to normal
		test.Next(_L("Soak test: Reset cache size to normal"));
		test_KErrNone(DPTest::SetCacheSize(cacheOriginalMin, cacheOriginalMax)); 

		test.Next(_L("Check we haven't leaked any swap in the course of the test"));
		TestSwapInfoUnchanged();
		}

	test.End();
	return 0;
	}
