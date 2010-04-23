// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\t_mmubm.cpp
// Overview:
// Time MMU allocation & deallocation
// API Information:
// RChunk
// Details:
// - Allocate and release a 1 MB chunk 100 times and time how long it takes.
// - Allocate and release a 2 MB chunk 100 times and time how long it takes.
// - Allocate and release a 7 MB chunk 100 times and time how long it takes.
// - Allocate a 7 MB chunk once and time how long it takes.
// - Deallocate a 7 MB chunk once and time how long it takes.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include "mmudetect.h"
#include "d_gobble.h"
#include "d_memorytest.h"
#include "freeram.h"

RTest test(_L("T_MMUBM"));

typedef TInt (*TestFunction)(TUint32 aIters, TUint32 aSize);

RMemoryTestLdd MemoryTest;
RChunk TheChunk;

LOCAL_C TInt AllocPhysical(TUint32 aIters, TUint32 aSize)
	{
	return MemoryTest.AllocPhysTest(aIters, aSize * 0x00100000);
	}

LOCAL_C TInt AllocPhysical1(TUint32 aIters, TUint32 aSize)
	{
	return MemoryTest.AllocPhysTest1(aIters, aSize * 0x00100000);
	}

LOCAL_C TInt AdjustChunk(TUint32 aIters, TUint32 aSize)
	{
	TUint32 i;
	for (i=0; i<aIters; i++)
		{
		TInt r=TheChunk.Adjust(aSize * 0x00100000);
		if (r!=KErrNone)
			return r;
		r=TheChunk.Adjust(0);
		if (r!=KErrNone)
			return r;
		}
	return KErrNone;
	}

LOCAL_C TInt AllocateChunk(TUint32 /*aIters*/, TUint32 aSize)
	{
	return TheChunk.Adjust(aSize * 0x00100000);
	}

LOCAL_C TInt DeallocateChunk(TUint32 , TUint32 )
	{
	return TheChunk.Adjust(0);
	}

LOCAL_C TInt TimedCall(TestFunction aFunction, TUint32& aTime, TUint32 aIters, TUint32 aSize)
	{
	TUint32 before=User::NTickCount();
	TInt r=(*aFunction)(aIters, aSize);
	TUint32 after=User::NTickCount();
	aTime=after-before;
	return r;
	}

#define DO_TEST(__ret, __func, __time, __iters, __size)\
	tempSize = __size;\
	__ret=TimedCall(__func, __time, __iters, __size);\
	if (__ret==KErrNone)\
		{\
		test.Printf(_L("  %dMb %d times ... %d ms\n"),__size, __iters,__time);\
		}\
	else if (tempSize < 7)\
		{\
		test(__ret==KErrNone);\
		}

#define DO_TEST_GOB(__ret, __func, __time, __iters, __size, __rem,__taken)\
	tempSize = __size;\
	__ret = gobbler.Open();\
	test(__ret==KErrNone);\
	/* gobble leaving x+1MB then test adjusting to x...*/\
	__taken = gobbler.GobbleRAM(__rem*1024*1024);\
	test.Printf(_L("Gobbled: %dK freeRam %dK\n"), __taken/1024, FreeRam()/1024);\
	/* adjust to 1MB*/\
	__ret=TimedCall(__func, __time, __iters, __size);\
	if (__ret==KErrNone)\
		{\
		test.Printf(_L(" %dMb %d times ... %d ms\n"),__size, __iters,__time);\
		}\
	else if (tempSize < 7)\
		{\
		test(__ret==KErrNone);\
		}\
	gobbler.Close();
	

GLDEF_C TInt E32Main()
	{

	test.Title();
	if (!HaveVirtMem())
		{
		test.Printf(_L("This test requires an MMU\n"));
		return KErrNone;
		}
	test.Start(_L("Timing MMU allocation/deallocation..."));

	test(KErrNone==MemoryTest.Open());

	TInt r;
	TUint32 t;
	TUint32 tempSize;
	r=TheChunk.CreateLocal(0,0x01000000);
	test(r==KErrNone);

	test.Printf(_L("Adjusting Chunks\n"));
	DO_TEST(r, AdjustChunk, t, 100, 1);
	DO_TEST(r, AdjustChunk, t, 100, 2);
	DO_TEST(r, AdjustChunk, t, 100, 7);
	r=TimedCall(AllocateChunk,t,1,7);
	if (r==KErrNone)
		{
		test.Printf(_L("Allocate 7Mb once ... %d ms\n"),t);
		r=TimedCall(DeallocateChunk,t, 1, 0);
		test.Printf(_L("Deallocate 7Mb once ... %d ms\n"),t);
		}
	else
		{
		test.Printf(_L("Alloc chunk 7Mb failed! %d\n"), r);
		}
	
	test.Next(_L("Test physical memory allocations...."));

	DO_TEST(r, AllocPhysical, t, 100, 1);
	DO_TEST(r, AllocPhysical1, t, 100, 1);
	DO_TEST(r, AllocPhysical, t, 100, 2);
	DO_TEST(r, AllocPhysical1, t, 100, 2);
	DO_TEST(r, AllocPhysical, t, 100, 7);
	DO_TEST(r, AllocPhysical1, t, 100, 7);

	test.Next(_L("Now the test with low memory...."));

	r = User::LoadLogicalDevice(KGobblerLddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);

	RGobbler gobbler;
	TUint32 taken = 0;

	DO_TEST_GOB(r, AdjustChunk, t, 100, 1, 2, taken);
	DO_TEST_GOB(r, AdjustChunk, t, 100, 2, 3, taken);
	DO_TEST_GOB(r, AdjustChunk, t, 100, 7, 8, taken);

	r=TimedCall(AllocateChunk,t,1,7);
	if (r==KErrNone)
		{
		test.Printf(_L("Allocate 7Mb once ... %d ms\n"),t);
		r=TimedCall(DeallocateChunk,t, 1, 0);
		test.Printf(_L("Deallocate 7Mb once ... %d ms\n"),t);
		}

	// gobble leaving 8MB then test allocing 7...
	r = gobbler.Open();
	test(r==KErrNone);
	taken = gobbler.GobbleRAM(8*1024*1024);
	test.Printf(_L("Gobbled: %dK freeRam %dK\n"), taken/1024, FreeRam()/1024);
	// allocate 7mb
	r=TimedCall(AllocateChunk,t,1,7);
	if (r==KErrNone)
		{
		test.Printf(_L("Allocate 7Mb once ... %d ms\n"),t);
		r=TimedCall(DeallocateChunk,t, 1, 0);
		test.Printf(_L("Deallocate 7Mb once ... %d ms\n"),t);
		}
	else
		test.Printf(_L("Alloc chunk 7Mb failed! %d\n"), r);
	gobbler.Close();

	test.Next(_L("Test physical memory allocations with low memory...."));
	DO_TEST_GOB(r, AllocPhysical, t, 100, 1, 2, taken);
	DO_TEST_GOB(r, AllocPhysical1, t, 100, 1, 2, taken);
	DO_TEST_GOB(r, AllocPhysical, t, 100, 2, 3, taken);
	DO_TEST_GOB(r, AllocPhysical1, t, 100, 2, 3, taken);
	DO_TEST_GOB(r, AllocPhysical, t, 100, 7, 8, taken);
	DO_TEST_GOB(r, AllocPhysical1, t, 100, 7, 8, taken);

	test.End();
	return 0;
	}
