// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\t_ramall.cpp
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32uid.h>
#include <hal.h>
#include <e32hal.h>
#include <dptest.h>
#include "d_shadow.h"
#include "mmudetect.h"
#include "freeram.h"
#include "d_gobble.h"

LOCAL_D RTest test(_L("T_RAMALL"));

_LIT(KLddFileName,"D_SHADOW.LDD");

TInt PageSize;
TInt PageShift;
RShadow Shadow;
TInt TotalRam;
RChunk Chunk;
TUint ChunkCommitEnd;
RThread TouchThread;
TRequestStatus TouchStatus;
TBool TouchDataStop;
RThread FragThread;
TRequestStatus FragStatus;
TBool FragThreadStop;
TBool ManualTest;
TBool CacheSizeAdjustable;
TUint OrigMinCacheSize;
TUint OrigMaxCacheSize;

//
// Random number generation
//

TUint32 RandomSeed;

TUint32 Random()
	{
	RandomSeed = RandomSeed*69069+1;
	return RandomSeed;
	}

TUint32 Random(TUint32 aRange)
	{
	return (TUint32)((TUint64(Random())*TUint64(aRange))>>32);
	}

void RandomInit(TUint32 aSeed)
	{
	RandomSeed = aSeed+(aSeed<<8)+(aSeed<<16)+(aSeed<<24);
	Random();
	Random();
	}

TInt AllocPhysicalRam(TUint32& aAddr, TInt aSize, TInt aAlign)
	{
	return Shadow.AllocPhysicalRam(aAddr,aSize,aAlign);
	}

TInt FreePhysicalRam(TUint32 aAddr, TInt aSize)
	{
	return Shadow.FreePhysicalRam(aAddr,aSize);
	}

TInt ClaimPhysicalRam(TUint32 aAddr, TInt aSize)
	{
	return Shadow.ClaimPhysicalRam(aAddr,aSize);
	}

void TestAlignedAllocs()
	{
	TInt align;
	TInt size;
	for (align=PageShift; align<=20; ++align)
		{
		for (size=PageSize; size<=0x100000; size+=PageSize)
			{
			TInt free=FreeRam();
			TUint32 pa=0;
			TInt r=AllocPhysicalRam(pa,size,align);
			test.Printf(_L("Size %08x Align %d r=%d pa=%08x\n"),size,align,r,pa);
			if (r==KErrNone)
				{
				TUint32 as=1u<<align;
				TUint32 am=as-1;
				test(FreeRam()==free-size);
				test((pa&am)==0);
				r=FreePhysicalRam(pa,size);
				test(r==KErrNone);
				}
			test(FreeRam()==free);
			}
		}
	}

void TestClaimPhys()
	{
	TInt free=FreeRam();
	
	TUint32 pa=0;
	TInt r=AllocPhysicalRam(pa,4*PageSize,0);	
	test(r==KErrNone);
	test(FreeRam()==free-4*PageSize);
	
	r=FreePhysicalRam(pa,4*PageSize);
	test(r==KErrNone);
	test(FreeRam()==free);
	
	r=ClaimPhysicalRam(pa,4*PageSize);	
	test(r==KErrNone);
	test(FreeRam()==free-4*PageSize);
	
	r=FreePhysicalRam(pa,3*PageSize);
	test(r==KErrNone);
	test(FreeRam()==free-PageSize);
	
	r=ClaimPhysicalRam(pa,4*PageSize);
	test(r==KErrInUse);
	test(FreeRam()==free-PageSize);
	
#ifdef MANUAL_PANIC_TEST
//This section of the test should be run as a manual test as it results in
// a panic due to attempting to Free an unclaimed page
	if (HaveVirtMem())
		{
		test.Printf(_L("HaveVirtMem() \n"));
		r=FreePhysicalRam(pa,4*PageSize);
		test.Printf(_L("FreePhysicalRam() \n"));
		test(r==KErrGeneral);
		test(FreeRam()==free-PageSize);
		}
#endif
	
	r=FreePhysicalRam(pa+3*PageSize,PageSize);
	test(r==KErrNone);
	test(FreeRam()==free);
	
	}


struct SPhysAllocData
	{
	TUint iSize;
	TUint iAlign;
	TBool iCheckMaxAllocs;
	TBool iCheckFreeRam;
	};


TInt FillPhysicalRam(TAny* aArgs)
	{
	SPhysAllocData allocData = *((SPhysAllocData*)aArgs);
	TUint maxAllocs = FreeRam() / allocData.iSize;
	TUint32* physAddrs = new TUint32[maxAllocs + 1];
	if (!physAddrs)
		return KErrNoMemory;
	TUint32* pa = physAddrs;
	TUint32 alignMask = (1 << allocData.iAlign) - 1;
	TUint initialFreeRam = FreeRam();
	TInt r = KErrNone;
	TUint allocations = 0;
	for(; allocations <= maxAllocs; ++allocations)
		{
		TUint freeRam = FreeRam();			
		r = AllocPhysicalRam(*pa, allocData.iSize, allocData.iAlign);
		if (r != KErrNone)
			break;
		if (*pa++ & alignMask)
			{
			r = KErrGeneral;
			RDebug::Printf("Error alignment phys addr 0x%08x", *(pa - 1));
			break;
			}
		TUint newFreeRam = FreeRam();
		if (allocData.iCheckFreeRam && freeRam - allocData.iSize != newFreeRam)
			{
			r = KErrGeneral;
			RDebug::Printf("Error in free ram 0x%08x orig 0x%08x", newFreeRam, freeRam);
			break;
			}
		}

	TUint32* physEnd = pa;
	TBool failFrees = EFalse;
	for (pa = physAddrs; pa < physEnd; pa++)
		{
		if (FreePhysicalRam(*pa, allocData.iSize) != KErrNone)
			failFrees = ETrue;
		}
	if (failFrees)
		r = KErrNotFound;
	if (allocData.iCheckMaxAllocs && allocations > maxAllocs)
		{
		r = KErrOverflow;
		RDebug::Printf("Error able to allocate too many pages");
		}
	TUint finalFreeRam = FreeRam();
	if (allocData.iCheckFreeRam && initialFreeRam != finalFreeRam)
		{
		r = KErrGeneral;
		RDebug::Printf("Error in free ram 0x%08x initial 0x%08x", finalFreeRam, initialFreeRam);
		}
	delete[] physAddrs;
	if (r != KErrNone && r != KErrNoMemory)
		return r;
	TUint possibleAllocs = initialFreeRam / allocData.iSize;
	if (allocData.iCheckMaxAllocs && possibleAllocs != allocations)
		{
		RDebug::Printf("Error in number of allocations possibleAllocs %d allocations %d", possibleAllocs, allocations);
		return KErrGeneral;
		}
	return allocations;
	}


void TestMultipleContiguousAllocations(TUint aNumThreads, TUint aSize, TUint aAlign)
	{
	test.Printf(_L("TestMultiContig threads %d size 0x%x, align %d\n"), aNumThreads, aSize, aAlign);
	SPhysAllocData allocData;
	allocData.iSize = aSize;
	allocData.iAlign = aAlign;
	allocData.iCheckMaxAllocs = EFalse;
	allocData.iCheckFreeRam = EFalse;
	// Start several threads all contiguous allocating memory.
	RThread* threads = new RThread[aNumThreads];
	TRequestStatus* status = new TRequestStatus[aNumThreads];
	TUint i = 0;
	for (; i < aNumThreads; i++)
		{// Need enough heap to store addr of every possible allocation + 1.
		TUint requiredHeapMax = Max(PageSize, ((TotalRam / aSize) * sizeof(TUint32)) + sizeof(TUint32));
		TInt r = threads[i].Create(KNullDesC, FillPhysicalRam, KDefaultStackSize, PageSize, requiredHeapMax, (TAny*)&allocData);
		if (r != KErrNone)
			break;			
		threads[i].Logon(status[i]);
		}
	TUint totalThreads = i;
	for (i = 0; i < totalThreads; i++)
		{
		threads[i].Resume();
		}
	for (i = 0; i < totalThreads; i++)
		{
		User::WaitForRequest(status[i]);
		test_Equal(EExitKill, threads[i].ExitType());
		TInt exitReason = threads[i].ExitReason();
		test_Value(exitReason, exitReason >= 0 || exitReason == KErrNoMemory);
		threads[i].Close();
		}
	delete[] status;
	delete[] threads;
	}

struct STouchData
	{
	TUint iSize;
	TUint iFrequency;
	}TouchData;


TInt TouchMemory(TAny*)
	{
	RThread::Rendezvous(KErrNone);	// Signal that this thread has started running.
	RandomInit(TouchData.iSize);
	while (!TouchDataStop)
		{
		TUint8* p = Chunk.Base();
		TUint8* pEnd = p + ChunkCommitEnd;
		TUint8* fragPEnd = p + TouchData.iFrequency;
		for (TUint8* fragP = p + TouchData.iSize; fragPEnd < pEnd && !TouchDataStop;)
			{
			TUint8* data = fragP;
			for (; data < fragPEnd && !TouchDataStop; data += PageSize)
				{
				*data = (TUint8)(data - fragP);
				TUint random = Random();
				if (random & 0x8484)
					User::After(random & 0xFFFF);
				}
			for (data = fragP; data < fragPEnd && !TouchDataStop; data += PageSize)
				{
				if (*data != (TUint8)(data - fragP))
					{
					RDebug::Printf("Error unexpected data 0x%x read from 0x%08x", *data, data);
					return KErrGeneral;
					}
				TUint random = Random();
				if (random & 0x8484)
					User::After(random & 0xFFFF);
				}
			fragP = fragPEnd + TouchData.iSize;
			fragPEnd += TouchData.iFrequency;
			}
		}
	return KErrNone;
	}

struct SFragData
	{
	TUint iSize;
	TUint iFrequency;
	TUint iDiscard;
	TBool iFragThread;
	}FragData;

void FragmentMemoryFunc()
	{
	ChunkCommitEnd = 0;
	TInt r;
	while(KErrNone == (r = Chunk.Commit(ChunkCommitEnd,PageSize)) && !FragThreadStop)
		{
		ChunkCommitEnd += PageSize;
		}
	if (FragThreadStop)
		return;
	test_Equal(KErrNoMemory, r);
	TUint freeBlocks = 0;
	for (	TUint offset = 0; 
			(offset + FragData.iSize) < ChunkCommitEnd; 
			offset += FragData.iFrequency, freeBlocks++)
		{
		test_KErrNone(Chunk.Decommit(offset, FragData.iSize));
		}

	if (FragData.iDiscard && CacheSizeAdjustable && !FragThreadStop)
		{
		TUint minCacheSize = FreeRam();
		TUint maxCacheSize = minCacheSize;
		DPTest::SetCacheSize(minCacheSize, maxCacheSize);
		if (OrigMinCacheSize <= maxCacheSize)
			DPTest::SetCacheSize(OrigMinCacheSize, maxCacheSize);
		}
	}


void UnfragmentMemoryFunc()
	{
	if (FragData.iDiscard && CacheSizeAdjustable)
		DPTest::SetCacheSize(OrigMinCacheSize, OrigMaxCacheSize);
	Chunk.Decommit(0, Chunk.MaxSize());
	}


TInt FragmentMemoryThreadFunc(TAny*)
	{
	RThread::Rendezvous(KErrNone);	// Signal that this thread has started running.
	while (!FragThreadStop)
		{
		FragmentMemoryFunc();
		UnfragmentMemoryFunc();
		}
	return KErrNone;
	}


void FragmentMemory(TUint aSize, TUint aFrequency, TBool aDiscard, TBool aTouchMemory, TBool aFragThread)
	{
	test_Value(aTouchMemory, !aTouchMemory || !aFragThread);
	test_Value(aSize, aSize < aFrequency);
	FragData.iSize = aSize;
	FragData.iFrequency = aFrequency;
	FragData.iDiscard = aDiscard;
	FragData.iFragThread = aFragThread;

	TChunkCreateInfo chunkInfo;
	chunkInfo.SetDisconnected(0, 0, TotalRam);
	chunkInfo.SetPaging(TChunkCreateInfo::EUnpaged);
	chunkInfo.SetClearByte(0x19);
	test_KErrNone(Chunk.Create(chunkInfo));

	if (aFragThread)
		{
		TInt r = FragThread.Create(_L("FragThread"), FragmentMemoryThreadFunc, KDefaultStackSize, PageSize, PageSize, NULL);
		test_KErrNone(r);
		FragThread.Logon(FragStatus);
		FragThreadStop = EFalse;
		TRequestStatus threadInitialised;
		FragThread.Rendezvous(threadInitialised);
		FragThread.Resume();
		User::WaitForRequest(threadInitialised);
		test_KErrNone(threadInitialised.Int());
		}
	else
		{
		FragmentMemoryFunc();
		}
	if (aTouchMemory && !ManualTest)
		{
		TouchData.iSize = aSize;
		TouchData.iFrequency = aFrequency;
		TInt r = TouchThread.Create(_L("TouchThread"), TouchMemory, KDefaultStackSize, PageSize, PageSize, NULL);
		test_KErrNone(r);
		TouchThread.Logon(TouchStatus);
		TouchDataStop = EFalse;
		TRequestStatus threadInitialised;
		TouchThread.Rendezvous(threadInitialised);
		TouchThread.Resume();
		User::WaitForRequest(threadInitialised);
		test_KErrNone(threadInitialised.Int());
		}
	}


void UnfragmentMemory(TBool aDiscard, TBool aTouchMemory, TBool aFragThread)
	{
	test_Value(aTouchMemory, !aTouchMemory || !aFragThread);
	if (aTouchMemory && !ManualTest)
		{
		TouchDataStop = ETrue;
		User::WaitForRequest(TouchStatus);
		test_Equal(EExitKill, TouchThread.ExitType());
		test_KErrNone(TouchThread.ExitReason());
		CLOSE_AND_WAIT(TouchThread);
		}
	if (aFragThread)
		{
		FragThreadStop = ETrue;
		User::WaitForRequest(FragStatus);
		test_Equal(EExitKill, FragThread.ExitType());
		test_KErrNone(FragThread.ExitReason());
		CLOSE_AND_WAIT(FragThread);
		}
	else
		UnfragmentMemoryFunc();
	if (CacheSizeAdjustable)
		test_KErrNone(DPTest::SetCacheSize(OrigMinCacheSize, OrigMaxCacheSize));
	CLOSE_AND_WAIT(Chunk);
	}


void TestFillPhysicalRam(TUint aFragSize, TUint aFragFreq, TUint aAllocSize, TUint aAllocAlign, TBool aDiscard, TBool aTouchMemory)
	{
	test.Printf(_L("TestFillPhysicalRam aFragSize 0x%x aFragFreq 0x%x aAllocSize 0x%x aAllocAlign %d dis %d touch %d\n"),
				aFragSize, aFragFreq, aAllocSize, aAllocAlign, aDiscard, aTouchMemory);
	FragmentMemory(aFragSize, aFragFreq, aDiscard, aTouchMemory, EFalse);
	SPhysAllocData allocData;
	// Only check free all ram could be allocated in manual tests as fixed pages may be fragmented.
	allocData.iCheckMaxAllocs = (ManualTest && !aTouchMemory && !aAllocAlign);
	allocData.iCheckFreeRam = ETrue;
	allocData.iSize = aAllocSize;
	allocData.iAlign = aAllocAlign;
	TInt r = FillPhysicalRam(&allocData);
	test_Value(r, r >= 0);
	UnfragmentMemory(aDiscard, aTouchMemory, EFalse);
	}


void TestFragmentedAllocation()
	{
	// Test every other page free.
	TestFillPhysicalRam(PageSize, PageSize * 2, PageSize, 0, EFalse, EFalse);
	if (ManualTest)
		{
		TestFillPhysicalRam(PageSize, PageSize * 2, PageSize * 2, 0, EFalse, EFalse);
		TestFillPhysicalRam(PageSize, PageSize * 2, PageSize, 0, EFalse, ETrue);
		}
	TestFillPhysicalRam(PageSize, PageSize * 2, PageSize * 2, 0, EFalse, ETrue);
	// Test every 2 pages free.
	TestFillPhysicalRam(PageSize * 2, PageSize * 4, PageSize * 8, 0, EFalse, EFalse);
	if (ManualTest)
		TestFillPhysicalRam(PageSize * 2, PageSize * 4, PageSize * 8, 0, EFalse, ETrue);
	// Test 10 pages free then 20 pages allocated, allocate 256 pages (1MB in most cases).
	if (ManualTest)
		TestFillPhysicalRam(PageSize * 10, PageSize * 30, PageSize * 256, 0, EFalse, EFalse);
	TestFillPhysicalRam(PageSize * 10, PageSize * 30, PageSize * 256, 0, EFalse, ETrue);

	if (CacheSizeAdjustable)
		{// It is possible to adjust the cache size so test phyiscally contiguous 
		// allocations discard and move pages when required.
		test.Next(_L("TestFragmentedAllocations with discardable data no true free memory"));
		// Test every other page free.
		TestFillPhysicalRam(PageSize, PageSize * 2, PageSize, 0, ETrue, EFalse);
		if (ManualTest)
			{
			TestFillPhysicalRam(PageSize, PageSize * 2, PageSize, 0, ETrue, ETrue);
			TestFillPhysicalRam(PageSize, PageSize * 2, PageSize * 2, 0, ETrue, EFalse);
			}
		TestFillPhysicalRam(PageSize, PageSize * 2, PageSize * 2, 0, ETrue, ETrue);
		// Test every 2 pages free.
		TestFillPhysicalRam(PageSize * 2, PageSize * 4, PageSize * 8, 0, ETrue, EFalse);
		if (ManualTest)
			TestFillPhysicalRam(PageSize * 2, PageSize * 4, PageSize * 8, 0, ETrue, ETrue);
		// Test 10 pages free then 20 pages allocated, allocate 256 pages (1MB in most cases).
		if (ManualTest)
			TestFillPhysicalRam(PageSize * 10, PageSize * 30, PageSize * 256, 0, ETrue, EFalse);
		TestFillPhysicalRam(PageSize * 10, PageSize * 30, PageSize * 256, 0, ETrue, ETrue);
		}
	}


GLDEF_C TInt E32Main()
//
// Test RAM allocation
//
    {
	test.Title();
	test.Start(_L("Load test LDD"));
	TInt r=User::LoadLogicalDevice(KLddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);

	r=UserHal::PageSizeInBytes(PageSize);
	test(r==KErrNone);

	TInt psz=PageSize;
	PageShift=-1;
	for (; psz; psz>>=1, ++PageShift);

	TUint currentCacheSize;
	CacheSizeAdjustable = DPTest::CacheSize(OrigMinCacheSize, OrigMaxCacheSize, currentCacheSize) == KErrNone;

	TUint memodel = UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, NULL, NULL) & EMemModelTypeMask;

	TInt cmdLineLen = User::CommandLineLength();
	if(cmdLineLen)
		{
		_LIT(KManual, "manual");
		RBuf cmdLine;
		test_KErrNone(cmdLine.Create(cmdLineLen));
		User::CommandLine(cmdLine);
		cmdLine.LowerCase();
		ManualTest = cmdLine.Find(KManual) != KErrNotFound;
		}

	// Turn off lazy dll unloading and ensure any supervisor clean up has completed 
	// so the free ram checking isn't affected.
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);

	test_KErrNone(HAL::Get(HAL::EMemoryRAM, TotalRam));

	test.Printf(_L("Free RAM=%08x, Page size=%x, Page shift=%d\n"),FreeRam(),PageSize,PageShift);

	test.Next(_L("Open test LDD"));
	r=Shadow.Open();
	test(r==KErrNone);
	
	test.Next(_L("TestAlignedAllocs"));
	TestAlignedAllocs();
	
	test.Next(_L("TestClaimPhys"));
	TestClaimPhys();

	if (memodel >= EMemModelTypeFlexible)
		{
		// To stop these tests taking too long leave only 8MB of RAM free.
		const TUint KFreePages = 2048;
		test.Next(_L("Load gobbler LDD"));
		TInt r = User::LoadLogicalDevice(KGobblerLddFileName);
		test_Value(r, r == KErrNone || r == KErrAlreadyExists);
		RGobbler gobbler;
		r = gobbler.Open();
		test_KErrNone(r);
		TUint32 taken = gobbler.GobbleRAM(KFreePages * PageSize);
		test.Printf(_L("Gobbled: %dK\n"), taken/1024);
		test.Printf(_L("Free RAM 0x%08X bytes\n"),FreeRam());

		test.Next(_L("TestFragmentedAllocation"));
		TestFragmentedAllocation();

		test.Next(_L("TestMultipleContiguousAllocations"));
		TestMultipleContiguousAllocations(20, PageSize * 16, 0);
		TestMultipleContiguousAllocations(20, PageSize * 16, PageShift + 1);
		TestMultipleContiguousAllocations(20, PageSize * 128, PageShift + 2);

		FragmentMemory(PageSize, PageSize * 2, EFalse, EFalse, EFalse);
		TestMultipleContiguousAllocations(20, PageSize * 128, PageShift + 2);
		UnfragmentMemory(EFalse, EFalse, EFalse);

		test.Next(_L("TestMultipleContiguousAllocations while accessing memory"));
		FragmentMemory(PageSize, PageSize * 2, EFalse, ETrue, EFalse);
		TestMultipleContiguousAllocations(20, PageSize * 128, PageShift + 2);
		UnfragmentMemory(EFalse, ETrue, EFalse);
		FragmentMemory(PageSize, PageSize * 2, ETrue, ETrue, EFalse);
		TestMultipleContiguousAllocations(50, PageSize * 256, PageShift + 5);
		UnfragmentMemory(ETrue, ETrue, EFalse);
		FragmentMemory(PageSize * 16, PageSize * 32, ETrue, ETrue, EFalse);
		TestMultipleContiguousAllocations(10, PageSize * 512, PageShift + 8);
		UnfragmentMemory(ETrue, ETrue, EFalse);
		FragmentMemory(PageSize * 32, PageSize * 64, ETrue, ETrue, EFalse);
		TestMultipleContiguousAllocations(10, PageSize * 1024, PageShift + 10);
		UnfragmentMemory(ETrue, ETrue, EFalse);

		test.Next(_L("TestMultipleContiguousAllocations with repeated movable and discardable allocations"));
		FragmentMemory(PageSize, PageSize * 2, EFalse, EFalse, ETrue);
		TestMultipleContiguousAllocations(20, PageSize * 2, PageShift);
		UnfragmentMemory(EFalse, EFalse, ETrue);
		FragmentMemory(PageSize, PageSize * 2, EFalse, EFalse, ETrue);
		TestMultipleContiguousAllocations(20, PageSize * 128, PageShift + 2);
		UnfragmentMemory(EFalse, EFalse, ETrue);
		FragmentMemory(PageSize, PageSize * 2, ETrue, EFalse, ETrue);
		TestMultipleContiguousAllocations(50, PageSize * 256, PageShift + 5);
		UnfragmentMemory(ETrue, EFalse, ETrue);
		FragmentMemory(PageSize * 16, PageSize * 32, ETrue, EFalse, ETrue);
		TestMultipleContiguousAllocations(20, PageSize * 512, PageShift + 8);
		UnfragmentMemory(ETrue, EFalse, ETrue);
		FragmentMemory(PageSize * 32, PageSize * 64, ETrue, EFalse, ETrue);
		TestMultipleContiguousAllocations(20, PageSize * 1024, PageShift + 10);
		UnfragmentMemory(ETrue, EFalse, ETrue);

		gobbler.Close();
		r = User::FreeLogicalDevice(KGobblerLddFileName);
		test_KErrNone(r);
		}

	Shadow.Close();
	r = User::FreeLogicalDevice(KLddFileName);
	test_KErrNone(r);
	test.Printf(_L("Free RAM=%08x at end of test\n"),FreeRam());
	test.End();
	return(KErrNone);
    }
