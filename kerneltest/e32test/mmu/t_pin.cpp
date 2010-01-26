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
// e32test\mmu\t_pin.cpp
// Tests kernel APIs for logical pinning by pinning memory and using a realtime thread to check that
// no page faults are taken while accessing it.
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>
#include <e32rom.h>
#include <e32kpan.h>
#include <u32hal.h>
#include <dptest.h>
#include "d_memorytest.h"
#include "t_codepaging_dll.h"
#include "mmudetect.h"
#include "freeram.h"

RTest test(_L("T_PIN"));

_LIT(KTCodePagingDll4, "t_codepaging_dll4.dll");
const TInt KMinBufferSize = 16384;

RMemoryTestLdd Ldd;
RMemoryTestLdd Ldd2;
RLibrary PagedLibrary;
const TUint8* PagedBuffer = NULL;
const TUint8* UnpagedBuffer = NULL;
TInt PageSize;

TInt FreeRamNoWait()
	{
	TMemoryInfoV1Buf meminfo;
	UserHal::MemoryInfo(meminfo);
	return meminfo().iFreeRamInBytes;
	}

void CheckMemoryPresent(const TUint8* aBuffer, TInt aSize, TBool aExpected)
	{
	if (aExpected)
		test.Printf(_L("  Checking memory at %08x is present\n"), aBuffer);
	else
		test.Printf(_L("  Checking memory at %08x is not present\n"), aBuffer);
	for (TInt i = 0 ; i < aSize ; i += PageSize)
		test_Equal(aExpected, Ldd.IsMemoryPresent(aBuffer + i));
	}

void FlushPagingCache()
	{
	test_KErrNone(DPTest::FlushCache());
	}

void TestPinVirtualMemoryUnpaged()
	{
	test.Printf(_L("Create logical pin object\n"));
	test_KErrNone(Ldd.CreateVirtualPinObject());
#ifdef __EPOC32__
	CheckMemoryPresent(UnpagedBuffer, KMinBufferSize, ETrue);
	test.Printf(_L("Perform logical pin operation on zero-length buffer\n"));
	test_KErrNone(Ldd.PinVirtualMemory((TLinAddr)UnpagedBuffer, 0));
	CheckMemoryPresent(UnpagedBuffer, KMinBufferSize, ETrue);
	test.Printf(_L("Perform logical unpin operation\n"));
	test_KErrNone(Ldd.UnpinVirtualMemory());
	CheckMemoryPresent(UnpagedBuffer, KMinBufferSize, ETrue);
	test.Printf(_L("Perform logical pin operation on whole buffer\n"));
	test_KErrNone(Ldd.PinVirtualMemory((TLinAddr)UnpagedBuffer, KMinBufferSize));
	CheckMemoryPresent(UnpagedBuffer, KMinBufferSize, ETrue);
	test.Printf(_L("Perform logical unpin operation\n"));
	test_KErrNone(Ldd.UnpinVirtualMemory());
	CheckMemoryPresent(UnpagedBuffer, KMinBufferSize, ETrue);
#else
	// Don't check for memory presence on emulator as paging not supported.
	test.Printf(_L("Perform logical pin operation on zero-length buffer\n"));
	test_KErrNone(Ldd.PinVirtualMemory((TLinAddr)UnpagedBuffer, 0));
	test.Printf(_L("Perform logical unpin operation\n"));
	test_KErrNone(Ldd.UnpinVirtualMemory());
	test.Printf(_L("Perform logical pin operation on whole buffer\n"));
	test_KErrNone(Ldd.PinVirtualMemory((TLinAddr)UnpagedBuffer, KMinBufferSize));
	test.Printf(_L("Perform logical unpin operation\n"));
	test_KErrNone(Ldd.UnpinVirtualMemory());
#endif
	test.Printf(_L("Perform logical unpin operation (again)\n"));
	test_KErrNone(Ldd.UnpinVirtualMemory());	// test double unpin ok
	test.Printf(_L("Destroy logical pin object\n"));
	test_KErrNone(Ldd.DestroyVirtualPinObject());
	test.Printf(_L("Destroy logical pin object (again)\n"));
	test_KErrNone(Ldd.DestroyVirtualPinObject());  // test double destroy ok
	}

void TestPinPhysicalMemory()
	{
	
	TInt mm = UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, 0, 0) & EMemModelTypeMask;
	if (mm < EMemModelTypeFlexible)
		{
		test.Printf(_L("Memory model (%d) doesn't support physical pining\n"),mm);
		return;
		}
	TInt i;
	TInt8* UCBase;
	RChunk chunk;

	test.Printf(_L("Allocate user chunk\n"));
	TChunkCreateInfo createInfo;
	createInfo.SetDisconnected(0,KUCPageCount*PageSize,KUCPageCount*PageSize);
	createInfo.SetPaging(TChunkCreateInfo::EPaged);
	test_KErrNone(chunk.Create(createInfo));
	UCBase = (TInt8*)chunk.Base();
	
	test.Printf(_L("Create physical pin object\n"));
	test_KErrNone(Ldd.CreatePhysicalPinObject());

	test.Printf(_L("Perform physical pin operation on zero-length buffer\n"));
	test_KErrNone(Ldd.PinPhysicalMemory((TLinAddr)UCBase, 0));	

	test.Printf(_L("Perform physical unpin operation\n"));
	test_KErrNone(Ldd.UnpinPhysicalMemory());	

	test.Printf(_L("Perform Physical pin operation on the chunk\n"));
	test_KErrNone(Ldd.PinPhysicalMemory((TLinAddr)UCBase, KUCPageCount*PageSize));	

	test.Printf(_L("Test that pinned physical memory preserves its mapping when recommited\n"));
	test_KErrNone(chunk.Decommit(0,KUCPageCount*PageSize));							 //Decommit all
	for (i=KUCPageCount-1;i>=0;i--) test_KErrNone(chunk.Commit(i*PageSize,PageSize)); //Commit in reverse order
	for (i=0;i<KUCPageCount;i++) // Recommited memory is not paged in. So, write into each page, before driver 
		{						// calls Kern::LinearToPhysical or it will get KErrInvalidMemory in return.
		volatile TInt8* ptr = (volatile TInt8*)(UCBase+i*PageSize);
		*ptr = 10;
		}
	test_KErrNone(Ldd.CheckPageList(chunk.Base())); 					// Check that the mapping is preserved. 	
	
	test.Printf(_L("Sync cache & memory of User Chunk\n"));//Test Cache::SyncPhysicalMemoryBeforeDmaWrite
	test_KErrNone(Ldd.SyncPinnedPhysicalMemory(0,KUCPageCount*PageSize));

	test.Printf(_L("Invalidate cache of User Chunk\n"));//Test Cache::SyncPhysicalMemoryBefore/AfterDmaRead
	test_KErrNone(Ldd.InvalidatePinnedPhysicalMemory(0,KUCPageCount*PageSize));
	
	test.Printf(_L("Try to move pinned phys. memory...\n")); //RAM defrag should return error code here.
	i = Ldd.MovePinnedPhysicalMemory(0);
	test.Printf(_L("...returned %d\n"),i);
	test(i!=KErrNone);

	test.Printf(_L("Close the chunk\n")); // Phys. memory is pinned and shouldn't be ...
	chunk.Close();						  // ... mapped to another virtual memory.

	test.Printf(_L("Allocate & initilise the second chunk\n"));// Kernel sholudn't commit pinned physical memory ...
	test_KErrNone(chunk.CreateLocal(KUCPageCount*PageSize,KUCPageCount*PageSize));   // ...that has been just decommited from the first chunk.
	UCBase = (TInt8*)chunk.Base();
	for (i=0;i<KUCPageCount*PageSize;i++) UCBase[i]=0; //Initialise user buffer

	test.Printf(_L("Invalidate cache of pinned memory\n"));//This shouldn't affect the second chunk.
	test_KErrNone(Ldd.InvalidatePinnedPhysicalMemory(0,KUCPageCount*PageSize));

	test.Printf(_L("Check data in the second chunk is unaffected\n"));
	for (i=0;i<KUCPageCount*PageSize;i++) test(UCBase[i]==0);
	
	test.Printf(_L("Close the second chunk\n"));
	chunk.Close();

	test.Printf(_L("Perform physical unpin operation\n"));
	test_KErrNone(Ldd.UnpinPhysicalMemory());	

	test.Printf(_L("Perform physical unpin operation (again)\n"));
	test_KErrNone(Ldd.UnpinPhysicalMemory());	// test double unpin ok

	test.Printf(_L("Destroy physical pin object\n"));
	test_KErrNone(Ldd.DestroyPhysicalPinObject());

	test.Printf(_L("Destroy physical pin object (again)\n"));
	test_KErrNone(Ldd.DestroyPhysicalPinObject());  // test double destroy ok

	test.Printf(_L("Test phys. pinning and sync of kernel memory.\n"));
	test_KErrNone(Ldd.PinKernelPhysicalMemory());// Simple test of phys. pinning of kernel memory
	}

void TestPhysicalPinOutOfMemory()
	{
	TInt mm = UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, 0, 0) & EMemModelTypeMask;
	if (mm < EMemModelTypeFlexible)
		{
		test.Printf(_L("Memory model (%d) doesn't support physical pining\n"),mm);
		return;
		}
	
	TInt8* UCBase;
	RChunk chunk;

	test.Printf(_L("Allocate user chunk\n"));
	test_KErrNone(chunk.CreateDisconnectedLocal(0,KUCPageCount*PageSize,KUCPageCount*PageSize));
	UCBase = (TInt8*)chunk.Base();
	
	const TInt KMaxKernelAllocations = 1024;
	TInt r=KErrNoMemory;
	TInt i;

	__KHEAP_MARK;
	for (i = 0; i < KMaxKernelAllocations && r == KErrNoMemory; i++)
		{
		__KHEAP_FAILNEXT(i);
		test.Printf(_L("Create physical pin object\n"));
		r = Ldd.CreatePhysicalPinObject();
		__KHEAP_RESET;
		}
	test.Printf(_L("Create physical pin object took %d tries\n"),i);
	test_KErrNone(r);

	r = KErrNoMemory;

	for (i = 0; i < KMaxKernelAllocations && r == KErrNoMemory; i++)
		{
		__KHEAP_FAILNEXT(i);
		test.Printf(_L("Perform physical pin operation\n"));
		r = Ldd.PinPhysicalMemory((TLinAddr)UCBase, KUCPageCount*PageSize);
		__KHEAP_RESET;
		}
	test.Printf(_L("Perform physical pin operation took %d tries\n"),i);
	if (r == KErrNone)
		{
		test.Printf(_L("Perform physical unpin operation\n"));
		Ldd.UnpinPhysicalMemory();
		}

	test.Printf(_L("Destroy physical pin object\n"));
	Ldd.DestroyPhysicalPinObject();

	// wait for any async cleanup in the supervisor to finish first...
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);	
	__KHEAP_MARKEND;

	test.Printf(_L("Close the chunk\n"));
	chunk.Close();
	
	test_KErrNone(r);
	}


void TestPinVirtualMemoryInvalid()
	{
	test.Printf(_L("Create logical pin object\n"));
	test_KErrNone(Ldd.CreateVirtualPinObject());
	test.Printf(_L("Attempt logical pin on bad memory address\n"));
	TLinAddr bad = (TLinAddr)0x10000;
	TInt r = Ldd.PinVirtualMemory(bad,KMinBufferSize);
	test.Printf(_L("%08x r=%d"),bad,r);
	if(r==KErrNone)
		test_KErrNone(Ldd.UnpinVirtualMemory());	
	if ((MemModelAttributes() & EMemModelTypeMask) == EMemModelTypeMultiple)
		{
		// test unused part of code chunk...
		bad = (TLinAddr)0x7f000000;
		r = Ldd.PinVirtualMemory(bad,KMinBufferSize);
		test.Printf(_L("%08x r=%d"),bad,r);
		if(r==KErrNone)
			test_KErrNone(Ldd.UnpinVirtualMemory());	
		}
	test.Printf(_L("Destroy logical pin object\n"));
	test_KErrNone(Ldd.DestroyVirtualPinObject());
	}

void TestPinVirtualMemoryPaged()
	{
	test.Printf(_L("Create logical pin object\n"));
	test_KErrNone(Ldd.CreateVirtualPinObject());
	FlushPagingCache();
	CheckMemoryPresent(PagedBuffer, KMinBufferSize, EFalse);
	test.Printf(_L("Perform logical pin operation on zero-length buffer\n"));
	test_KErrNone(Ldd.PinVirtualMemory((TLinAddr)PagedBuffer, 0));	
	CheckMemoryPresent(PagedBuffer, KMinBufferSize, EFalse);
	test.Printf(_L("Perform logical unpin operation\n"));
	test_KErrNone(Ldd.UnpinVirtualMemory());	
	CheckMemoryPresent(PagedBuffer, KMinBufferSize, EFalse);
	test.Printf(_L("Perform logical pin operation on whole buffer\n"));
	test_KErrNone(Ldd.PinVirtualMemory((TLinAddr)PagedBuffer, KMinBufferSize));	
	CheckMemoryPresent(PagedBuffer, KMinBufferSize, ETrue);
	FlushPagingCache();
	CheckMemoryPresent(PagedBuffer, KMinBufferSize, ETrue);
	test.Printf(_L("Perform logical unpin operation\n"));
	test_KErrNone(Ldd.UnpinVirtualMemory());	
	CheckMemoryPresent(PagedBuffer, KMinBufferSize, ETrue);
	FlushPagingCache();
	CheckMemoryPresent(PagedBuffer, KMinBufferSize, EFalse);
	test.Printf(_L("Perform logical unpin operation (again)\n"));
	test_KErrNone(Ldd.UnpinVirtualMemory());	// test double unpin ok
	test.Printf(_L("Destroy logical pin object\n"));
	test_KErrNone(Ldd.DestroyVirtualPinObject());
	test.Printf(_L("Destroy logical pin object (again)\n"));
	test_KErrNone(Ldd.DestroyVirtualPinObject());  // test double destroy ok
	}


volatile TBool SoakEnd = false;

class TRandom
	{
public:
	TRandom(TUint32 aSeed)
		: iSeed(aSeed) {};
	inline TUint32 Next()
		{ iSeed = iSeed*69069+1; return iSeed; }
	TUint32 operator()(TUint32 aRange)
		{ return (TUint32)((TUint64(Next())*TUint64(aRange))>>32); }
private:
	TUint iSeed;
	};

#define SOAK_CHECK(r)												\
	if(r!=KErrNone)													\
		{															\
		RDebug::Printf("SOAK_CHECK fail at line %d",__LINE__);		\
		return r;													\
		}															\

TInt SoakThread(TAny*)
	{
	RMemoryTestLdd ldd;
	TInt r = ldd.Open();
	SOAK_CHECK(r)

	r = ldd.CreateVirtualPinObject();
	SOAK_CHECK(r)

	TRandom random((TUint32)&ldd);

	while(!SoakEnd)
		{
		TUint start = random(KMinBufferSize);
		TUint end = random(KMinBufferSize);
		if(start>end)
			{
			TUint temp = start;
			start = end;
			end = temp;
			}
		const TUint32 KPageMask = 0xfff;
		start &= ~KPageMask;
		end = (end+KPageMask)&~KPageMask;

		r = ldd.PinVirtualMemory((TLinAddr)(PagedBuffer+start),end-start);
		SOAK_CHECK(r)

		r = ldd.UnpinVirtualMemory();
		SOAK_CHECK(r)
		}

	r = ldd.DestroyVirtualPinObject();
	SOAK_CHECK(r)

	CLOSE_AND_WAIT(ldd);
	return KErrNone;
	}


void TestPinVirtualMemoryPagedSoak()
	{
	test.Start(_L("Create timer"));
	RTimer timer;
	test_KErrNone(timer.CreateLocal());

	test.Next(_L("Create threads"));
	const TUint KNumThreads = 4;
	TRequestStatus status[KNumThreads];
	RThread thread[KNumThreads];
	TUint i;
	for(i=0; i<KNumThreads; i++)
		{
		test_KErrNone(thread[i].Create(KNullDesC, SoakThread, 0x1000, NULL, 0));
		thread[i].Logon(status[i]);
		test(status[i].Int()==KRequestPending);
		}

	test.Next(_L("Start threads"));
	RThread().SetPriority(EPriorityMore); // make sure we are higher priority than soak threads
	for(i=0; i<KNumThreads; i++)
		thread[i].Resume();

	test.Next(_L("Wait..."));
	TRequestStatus timeoutStatus;
	timer.After(timeoutStatus,10*1000000);
	User::WaitForAnyRequest();
	test_KErrNone(timeoutStatus.Int()); // we should have timed out if soak threads are still running OK

	test.Next(_L("Stop threads and check results"));
	for(i=0; i<KNumThreads; i++)
		test_Equal(KRequestPending,status[i].Int());
	SoakEnd = true;
	timer.After(timeoutStatus,10*1000000);
	for(i=0; i<KNumThreads; i++)
		{
		User::WaitForAnyRequest();
		test_Equal(KRequestPending,timeoutStatus.Int());
		}
	timer.Cancel();
	User::WaitForRequest(timeoutStatus);
	RThread().SetPriority(EPriorityNormal); // restore thread priority

	// cleanup...
	CLOSE_AND_WAIT(timer);
	for(i=0; i<KNumThreads; i++)
		CLOSE_AND_WAIT(thread[i]);

	test.End();
	}


void TestPinVirtualMemoryDecommit()
	{
	const TInt KChunk = 4*1024*1024; // offset of page table boundary on X86 and ARM
	const TInt KPage = PageSize;
	const TInt TestData[][2] =
		{
			{0,				KPage},
			{KPage,			KPage},
			{KPage,			2*KPage},
			{KChunk-KPage,	KPage},
			{KChunk-2*KPage,2*KPage},
			{KChunk-KPage,	2*KPage},
			{0,0} // end marker
		};

	for(TInt i=0; TestData[i][1]; ++i)
		{
		TInt commitOffset = TestData[i][0];
		TInt commitSize = TestData[i][1];
		test.Printf(_L("Create chunk 0x%x+0x%x\n"),commitOffset,commitSize);

		TChunkCreateInfo createInfo;
		createInfo.SetDisconnected(commitOffset,commitOffset+commitSize,commitOffset+commitSize);
		createInfo.SetPaging(TChunkCreateInfo::EPaged);
		RChunk chunk;
		test_KErrNone(chunk.Create(createInfo));
		TUint8* buffer = chunk.Base()+commitOffset;
		TUint bufferSize = commitSize;
		FlushPagingCache(); // start with blank slate as far as paged memory is concerned

		test.Printf(_L("Create virtual pin object\n"));
		test_KErrNone(Ldd.CreateVirtualPinObject());
		test_KErrNone(Ldd2.CreateVirtualPinObject());
		CheckMemoryPresent(buffer, bufferSize, EFalse);
		TInt initialFreeRam = FreeRam();

		test.Printf(_L("Pin memory\n"));
		test_KErrNone(Ldd.PinVirtualMemory((TLinAddr)buffer, bufferSize));	
		CheckMemoryPresent(buffer, bufferSize, ETrue);
		TInt pinnedFreeRam = FreeRam();
		test_Compare(pinnedFreeRam,<,initialFreeRam);
		TUint8 c = *buffer;
		memset(buffer,~c,bufferSize); // invert memory

		test.Printf(_L("Decommit pinned memory\n"));
		test_KErrNone(chunk.Decommit(commitOffset,commitSize));	
		CheckMemoryPresent(buffer, bufferSize, EFalse);
		test_Equal(pinnedFreeRam,FreeRam()); // decommited memory should not be freed as it is pinned

		test.Printf(_L("Unpin memory\n"));
		test_KErrNone(Ldd.UnpinVirtualMemory());	
		CheckMemoryPresent(buffer, bufferSize, EFalse);
		test_Equal(initialFreeRam,FreeRam()); // memory should be now freed

		//
		// test recommitting decommitted pinned memory...
		//

		test.Printf(_L("Commit memory\n"));
		test_KErrNone(chunk.Commit(commitOffset,commitSize));	
		CheckMemoryPresent(buffer, bufferSize, EFalse);
		test_Equal(initialFreeRam,FreeRam());

		test.Printf(_L("Read memory\n"));
		volatile TUint8* p = buffer;
		volatile TUint8* pEnd = buffer+bufferSize;
		while(p<pEnd)
			test_Equal(c,*p++); // memory should have been wiped
		test_Equal(initialFreeRam,FreeRam()); // memory now paged in

		test.Printf(_L("Pin memory which is already paged in\n"));
		test_KErrNone(Ldd.PinVirtualMemory((TLinAddr)buffer, bufferSize));	
		CheckMemoryPresent(buffer, bufferSize, ETrue);
		test_Equal(pinnedFreeRam,FreeRam());
		memset(buffer,~c,bufferSize); // invert memory

		test.Printf(_L("Decommit pinned memory\n"));
		test_KErrNone(chunk.Decommit(commitOffset,commitSize));	
		CheckMemoryPresent(buffer, bufferSize, EFalse);
		test_Equal(pinnedFreeRam,FreeRam());

		test.Printf(_L("Commit pinned memory again\n"));
		test_KErrNone(chunk.Commit(commitOffset,commitSize));	
		CheckMemoryPresent(buffer, bufferSize, EFalse);
		test_Equal(pinnedFreeRam,FreeRam());
		p = buffer;
		pEnd = buffer+bufferSize;
		while(p<pEnd)
			test_Equal(c,*p++); // memory should have been wiped

		test.Printf(_L("Unpin memory\n"));
		test_KErrNone(Ldd.UnpinVirtualMemory());	
		CheckMemoryPresent(buffer, bufferSize, ETrue);
		test_Equal(initialFreeRam,FreeRam());

		test.Printf(_L("Decommit memory\n"));
		test_KErrNone(chunk.Decommit(commitOffset,commitSize));	
		CheckMemoryPresent(buffer, bufferSize, EFalse);
		test_Compare(FreeRam(),<=,initialFreeRam);

		//
		// test pin twice...
		//

		test.Printf(_L("Commit memory\n"));
		test_KErrNone(chunk.Commit(commitOffset,commitSize));	
		CheckMemoryPresent(buffer, bufferSize, EFalse);
		test_Equal(initialFreeRam,FreeRam());

		test.Printf(_L("Pin memory\n"));
		test_KErrNone(Ldd.PinVirtualMemory((TLinAddr)buffer, bufferSize));	
		CheckMemoryPresent(buffer, bufferSize, ETrue);
		test_Equal(pinnedFreeRam,FreeRam());

		test.Printf(_L("Pin memory again\n"));
		test_KErrNone(Ldd2.PinVirtualMemory((TLinAddr)buffer, bufferSize));	
		CheckMemoryPresent(buffer, bufferSize, ETrue);
		test_Equal(pinnedFreeRam,FreeRam());

		test.Printf(_L("Decommit pinned memory\n"));
		test_KErrNone(chunk.Decommit(commitOffset,commitSize));	
		CheckMemoryPresent(buffer, bufferSize, EFalse);
		test_Equal(pinnedFreeRam,FreeRam()); // decommited memory should not be freed as it is pinned

		test.Printf(_L("Unpin memory\n"));
		test_KErrNone(Ldd2.UnpinVirtualMemory());	
		CheckMemoryPresent(buffer, bufferSize, EFalse);
		test_Equal(pinnedFreeRam,FreeRam()); // memory shouldn't be freed as another pin exists

		test.Printf(_L("Unpin memory again\n"));
		test_KErrNone(Ldd.UnpinVirtualMemory());	
		CheckMemoryPresent(buffer, bufferSize, EFalse);
		test_Equal(initialFreeRam,FreeRam()); // memory should be now freed

		//
		// test page stealing of decommited memory
		//

		test.Printf(_L("Commit memory\n"));
		test_KErrNone(chunk.Commit(commitOffset,commitSize));	
		CheckMemoryPresent(buffer, bufferSize, EFalse);
		test_Equal(initialFreeRam,FreeRam());

		test.Printf(_L("Pin memory\n"));
		test_KErrNone(Ldd.PinVirtualMemory((TLinAddr)buffer, bufferSize));	
		CheckMemoryPresent(buffer, bufferSize, ETrue);
		test_Equal(pinnedFreeRam,FreeRam());

		test.Printf(_L("Decommit pinned memory\n"));
		test_KErrNone(chunk.Decommit(commitOffset,commitSize));	
		CheckMemoryPresent(buffer, bufferSize, EFalse);
		test_Equal(pinnedFreeRam,FreeRam());

		test.Printf(_L("Unpin memory a higher priority that supervisor thread\n"));
		RThread().SetPriority(EPriorityRealTime);
		test_KErrNone(Ldd.UnpinVirtualMemory());	
		// on single core system, supervisor thread can't run and free pages yet
		// because we're a higher priority...
		test.Printf(_L("memory freed = %d\n"),initialFreeRam==FreeRamNoWait());

		test.Printf(_L("Force decommited unpinned pages out of live list\n"));
		FlushPagingCache();
		RThread().SetPriority(EPriorityNormal);
		test_Equal(initialFreeRam,FreeRam()); // memory should be now freed

		//
		// cleanup...
		//

		test.Printf(_L("Destroy pin object\n"));
		test_KErrNone(Ldd.DestroyVirtualPinObject());
		test_KErrNone(Ldd2.DestroyVirtualPinObject());
		chunk.Close();
		}

	test.Printf(_L("Flush paging cache\n"));
	FlushPagingCache(); // this is a test that has shown up bugs in the past
	}


void TestPinOutOfMemory()
	{
	// Ensure that if pinning fails with KErrNoMemory,
	// there isn't a memory leak
	const TInt KMaxKernelAllocations = 1024;
	TInt r=KErrNoMemory;
	TInt i;
	const TUint8* buffer = NULL;
	if (PagedBuffer)
		{
		buffer = PagedBuffer;
		}
	else
		{
		buffer = UnpagedBuffer;
		}
	test_NotNull(buffer);

	__KHEAP_MARK;
	for (i = 0; i < KMaxKernelAllocations && r == KErrNoMemory; i++)
		{
		__KHEAP_FAILNEXT(i);
		test.Printf(_L("Create logical pin object\n"));
		r = Ldd.CreateVirtualPinObject();
		__KHEAP_RESET;
		}
	test.Printf(_L("Create logical pin object took %d tries\n"),i);
	test_KErrNone(r);

	r = KErrNoMemory;
	for (i = 0; i < KMaxKernelAllocations && r == KErrNoMemory; i++)
		{
		__KHEAP_FAILNEXT(i);
		test.Printf(_L("Perform logical pin operation\n"));
		r = Ldd.PinVirtualMemory((TLinAddr)buffer, KMinBufferSize);
		__KHEAP_RESET;
		}
	test.Printf(_L("Perform logical pin operation took %d tries\n"),i);
	if (r == KErrNone)
		{
		test.Printf(_L("Perform logical unpin operation\n"));
		Ldd.UnpinVirtualMemory();
		}
	
	test.Printf(_L("Destroy logical pin object\n"));
	Ldd.DestroyVirtualPinObject();
	// wait for any async cleanup in the supervisor to finish first...
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);	
	__KHEAP_MARKEND;

	test_KErrNone(r);
	}


TInt KernelModifyData(TAny*)
	{
	Ldd.KernelMapReadAndModifyMemory();
	return KErrNone;
	}

void TestMapAndPinMemory()
	{
	
	TInt mm = UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, 0, 0) & EMemModelTypeMask;
	if (mm < EMemModelTypeFlexible)
		{
		test.Printf(_L("Memory model (%d) doesn't support physical pining\n"),mm);
		return;
		}
	TInt i;
	TUint KUCBytes = KUCPageCount * PageSize;
	RChunk chunk;

	test.Printf(_L("Allocate user chunk\n"));
	TChunkCreateInfo createInfo;
	createInfo.SetDisconnected(0, KUCBytes, KUCBytes);
	createInfo.SetPaging(TChunkCreateInfo::EPaged);
	test_KErrNone(chunk.Create(createInfo));
	TUint8* chunkBase = (TUint8*)chunk.Base();
	
	test.Printf(_L("Create kernel map object\n"));
	test_KErrNone(Ldd.CreateKernelMapObject(0));

	test.Printf(_L("Perform kernel map operation on zero-length buffer\n"));
	test_KErrNone(Ldd.KernelMapMemory((TLinAddr)chunkBase, 0));	

	test.Printf(_L("Perform kernel unmap operation\n"));
	test_KErrNone(Ldd.KernelUnmapMemory());	

	test.Printf(_L("Perform kernel map operation on the chunk\n"));
	test_KErrNone(Ldd.KernelMapMemory((TLinAddr)chunkBase, KUCBytes));

	test.Printf(_L("Attempt to map the memory again while already mapped\n"));
	test_Equal(KErrInUse, Ldd.KernelMapMemory((TLinAddr)chunkBase, KUCBytes));

	test.Printf(_L("Use the kernel mapping to modify the data and verify it\n"));
	TUint8* p = chunkBase;
	for (i = 0; i < (TInt)KUCBytes; i++)
		*p++ = (TUint8)i;
	test_KErrNone(Ldd.KernelMapReadAndModifyMemory());
	p = chunkBase;
	for (i = 0; i < (TInt)KUCBytes; i++)
		test_Equal((TUint8)(i + 1), *p++);	

	test.Printf(_L("Test that kernel mapped memory preserves its mapping when recommited\n"));
	test_KErrNone(chunk.Decommit(0,KUCPageCount*PageSize));							 //Decommit all
	for (i=KUCPageCount-1;i>=0;i--) test_KErrNone(chunk.Commit(i*PageSize,PageSize)); //Commit in reverse order
	for (i=0;i<KUCPageCount;i++) // Recommited memory is not paged in. So, write into each page, before driver 
		{						// calls Kern::LinearToPhysical or it will get KErrInvalidMemory in return.
		volatile TInt8* ptr = (volatile TInt8*)(chunkBase+i*PageSize);
		*ptr = 10;
		}
	test_KErrNone(Ldd.KernelMapCheckPageList(chunkBase)); 	// Check that the mapping is preserved. 	
	
	test.Printf(_L("Sync cache & memory of User Chunk\n"));	//Test Cache::SyncMemoryBeforeDmaWrite
	test_KErrNone(Ldd.KernelMapSyncMemory());

	test.Printf(_L("Invalidate cache of User Chunk\n"));//Test Cache::SyncMemoryBefore/AfterDmaRead
	test_KErrNone(Ldd.KernelMapInvalidateMemory());
	
	test.Printf(_L("Try to move kernel map memory...\n")); //RAM defrag should return error code here.
	for (i = 0; i < KUCPageCount; i++)
		{
		TInt r = Ldd.KernelMapMoveMemory(0);
		test.Printf(_L("...[%d] returned %d\n"), i, r);
		test(r != KErrNone);
		}

	test.Printf(_L("Unmap the memory and attempt to map with invalid attributes\n"));
	test_KErrNone(Ldd.KernelUnmapMemory());
	test_Equal(KErrArgument, Ldd.KernelMapMemoryInvalid((TLinAddr)chunkBase, KUCBytes));

	test.Printf(_L("Map the memory read only and attempt to modify it kernel side\n"));
	test_KErrNone(Ldd.KernelMapMemoryRO((TLinAddr)chunkBase, KUCBytes));
	// Reset the contents of the memory.
	p = chunkBase;
	for (i = 0; i < (TInt)KUCBytes; i++)
		*p++ = (TUint8)i;

	RThread modThread;
	test_KErrNone(modThread.Create(KNullDesC, KernelModifyData, PageSize, PageSize, PageSize, (TAny*)NULL));
	TRequestStatus status;
	modThread.Logon(status);
	test_Equal(KRequestPending, status.Int());
	modThread.Resume();
	User::WaitForRequest(status);
	test_Equal(EExitPanic, modThread.ExitType());
	test(modThread.ExitCategory() == _L("KERN-EXEC"));
	test_Equal(ECausedException, modThread.ExitReason());
	CLOSE_AND_WAIT(modThread);

	test.Printf(_L("Close the chunk\n")); // Phys. memory is pinned and shouldn't be ...
	chunk.Close();						  // ... mapped to another virtual memory.

	test.Printf(_L("Allocate & initilise the second chunk\n"));// Kernel shouldn't commit pinned physical memory ...
	test_KErrNone(chunk.CreateLocal(KUCBytes, KUCBytes));   // ...that has just been decommited from the first chunk.
	chunkBase = (TUint8*)chunk.Base();
	for (i = 0; i < KUCPageCount * PageSize; i++) 
		chunkBase[i] = 0; //Initialise user buffer

	test.Printf(_L("Invalidate cache of pinned memory\n"));//This shouldn't affect the second chunk.
	test_KErrNone(Ldd.KernelMapInvalidateMemory());

	test.Printf(_L("Check data in the second chunk is unaffected\n"));
	for (i=0; i < KUCPageCount * PageSize; i++) 
		test(chunkBase[i]==0);
	
	test.Printf(_L("Close the second chunk\n"));
	chunk.Close();

	test.Printf(_L("Perform kernel unmap operation\n"));
	test_KErrNone(Ldd.KernelUnmapMemory());	

	test.Printf(_L("Perform physical unpin operation (again)\n"));
	test_KErrNone(Ldd.KernelUnmapMemory());	// test double unpin ok

	test.Printf(_L("Destroy physical pin object\n"));
	test_KErrNone(Ldd.DestroyKernelMapObject());

	test.Printf(_L("Destroy physical pin object (again)\n"));
	test_KErrNone(Ldd.DestroyKernelMapObject());  // test double destroy ok

	//
	//	Test a kernel mapping with preserved resources doesn't allocate when mapping and pinning.
	//
	test.Printf(_L("Create a pre-reserving kernel mapping object\n"));
	TUint mappingSize = KUCBytes>>1;
	// This test step relies on mapping objet being smaller than the user chunk
	// and as mapping object will always be >=2 pages, user chunk must be at least 4.
	__ASSERT_COMPILE(KUCPageCount >= 4);
	test_KErrNone(Ldd.CreateKernelMapObject(mappingSize));
	TChunkCreateInfo chunkInfo;
	chunkInfo.SetNormal(KUCBytes, KUCBytes);
	chunkInfo.SetPaging(TChunkCreateInfo::EUnpaged);
	test_KErrNone(chunk.Create(chunkInfo));

	test.Printf(_L("Map and pin an unpaged chunk with pre-reserved resources\n"));
	__KHEAP_FAILNEXT(1);	// Ensure any attempted kernel heap allocations fail.
	test_KErrNone(Ldd.KernelMapMemory((TLinAddr)chunk.Base(), mappingSize));
	test_KErrNone(Ldd.KernelUnmapMemory());

	test.Printf(_L("Map more memory than we have pre-reserved resources for\n"));
	test_Equal(KErrArgument, Ldd.KernelMapMemory((TLinAddr)chunk.Base(), mappingSize*2));

	test.Printf(_L("Destroy the kernel map object with pre-reserved resources\n"));
	test_KErrNone(Ldd.DestroyKernelMapObject());	// This will also unpin the memory.
	// Clear the kernel heap fail next.
	__KHEAP_RESET;
	chunk.Close();
	}

TInt E32Main()
	{
	test.Title();
	test.Start(_L("Test kernel pinning APIs"));

	if (DPTest::Attributes() & DPTest::ERomPaging)
		test.Printf(_L("Rom paging supported\n"));
	if (DPTest::Attributes() & DPTest::ECodePaging)
		test.Printf(_L("Code paging supported\n"));
	if (DPTest::Attributes() & DPTest::EDataPaging)
		test.Printf(_L("Data paging supported\n"));
	
	test.Next(_L("Loading test drivers"));
	test_KErrNone(Ldd.Open());
	test_KErrNone(Ldd2.Open());

	test.Next(_L("Getting page size"));
	test_KErrNone(UserSvr::HalFunction(EHalGroupKernel,EKernelHalPageSizeInBytes,&PageSize,0));
	
	test.Next(_L("Setting up paged and unpaged buffers"));

#ifdef __EPOC32__
	// Use unpaged rom for our unpaged buffer
	TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
	UnpagedBuffer = (TUint8*)romHeader;
	TInt size = romHeader->iPageableRomStart ? romHeader->iPageableRomStart : romHeader->iUncompressedSize;
	test(size >= KMinBufferSize);
	
	if (DPTest::Attributes() & DPTest::ERomPaging)
		{
		// Use end of paged ROM for our paged buffer
		test(romHeader->iPageableRomStart);
		TInt offset = romHeader->iPageableRomStart + romHeader->iPageableRomSize - KMinBufferSize;
		offset &= ~0xfff;
		test(offset>=romHeader->iPageableRomStart);
		PagedBuffer = (TUint8*)romHeader + offset;
		}
	else if (DPTest::Attributes() & DPTest::ECodePaging)
		{
		// Use code paged DLL for our paged buffer
		test_KErrNone(PagedLibrary.Load(KTCodePagingDll4));		
		TGetAddressOfDataFunction func = (TGetAddressOfDataFunction)PagedLibrary.Lookup(KGetAddressOfDataFunctionOrdinal);
		TInt size;
		PagedBuffer = (TUint8*)func(size);
		test_NotNull(PagedBuffer);
		test(size >= KMinBufferSize);
		}
#else
	UnpagedBuffer = (TUint8*)User::Alloc(KMinBufferSize);
	test_NotNull(UnpagedBuffer);
#endif

	RDebug::Printf("UnpagedBuffer=%x\n",UnpagedBuffer);
	RDebug::Printf("PagedBuffer=%x\n",PagedBuffer);

	__KHEAP_MARK;
	
	test.Next(_L("Logical pin unpaged memory"));
	TestPinVirtualMemoryUnpaged();

	test.Next(_L("Logical pin invalid memory"));
	TestPinVirtualMemoryInvalid();

	test.Next(_L("Physical pinning"));
	TestPinPhysicalMemory();
	
	test.Next(_L("Physical pinning OOM"));
	TestPhysicalPinOutOfMemory();

	test.Next(_L("Kernel pin mapping"));
	TestMapAndPinMemory();

	test.Next(_L("Pin OOM Tests"));
	TestPinOutOfMemory();

	if (PagedBuffer)
		{
		test.Next(_L("Logical pin paged memory"));
		TestPinVirtualMemoryPaged();

		test.Next(_L("Logical pin paged memory soak test"));
		TestPinVirtualMemoryPagedSoak();
		}

	if (DPTest::Attributes() & DPTest::EDataPaging)
		{
		test.Next(_L("Logical pin then decommit memory"));
		TestPinVirtualMemoryDecommit();
		}

	// wait for any async cleanup in the supervisor to finish first...
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);	
	__KHEAP_MARKEND;

#ifndef __EPOC32__
	User::Free((TAny*)UnpagedBuffer);
#endif

	PagedLibrary.Close();
	Ldd.Close();
	Ldd2.Close();
	test.End();

	return KErrNone;
	}
