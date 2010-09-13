// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/mmu/t_shbuf.cpp
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <hal.h>
#include <e32svr.h>
#include <u32hal.h>
#include "d_shbuf.h"
#include <e32shbuf.h>
#include <e32def.h>
#include <e32def_private.h>

#ifdef TEST_CLIENT_THREAD
RTest test(_L("T_SHBUF_CLIENT"));
#else
RTest test(_L("T_SHBUF_OWN"));
#endif

RShPool P1; // User-side pool
RShPool P2; // Kernel-side pool

const TInt KTestPoolSizeInBytes = 1 << 20; // 1MB
const TInt BufferSize[] = {128, 853, 4096, 5051, 131072, 1, 0}; // Last element must be 0

const TInt* PtrBufSize;

static TInt ThreadCounter = 0;

RShBufTestChannel Ldd;

_LIT(KTestSlave, "SLAVE");
_LIT(KTestLowSpaceSemaphore, "LowSpaceSemaphore");

enum TTestSlave
	{
	ETestSlaveError,
	ETestSlaveNoDeallocation,
	};

enum TTestPoolType
	{
	ETestNonPageAligned,
	ETestPageAligned,
	ETestPageAlignedGrowing,
	};

TInt Log2(TInt aNum)
	{
	TInt res = -1;
	while(aNum)
		{
		res++;
		aNum >>= 1;
		}
	return res;
	}

TInt RoundUp(TInt aNum, TInt aAlignmentLog2)
	{
	if (aNum % (1 << aAlignmentLog2) == 0)
		{
		return aNum;
		}
	return (aNum & ~((1 << aAlignmentLog2) - 1)) + (1 << aAlignmentLog2);
	}

void LoadDeviceDrivers()
	{
	TInt r;
	#ifdef TEST_CLIENT_THREAD
	r= User::LoadLogicalDevice(_L("D_SHBUF_CLIENT.LDD"));
	if (r != KErrAlreadyExists)
		{
		test_KErrNone(r);
		}
	#else
	r = User::LoadLogicalDevice(_L("D_SHBUF_OWN.LDD"));
	if (r != KErrAlreadyExists)
		{
		test_KErrNone(r);
		}
	#endif
	}

void FreeDeviceDrivers()
	{
	TInt r = User::FreeLogicalDevice(KTestShBufClient);
	test_KErrNone(r);
	r = User::FreeLogicalDevice(KTestShBufOwn);
	test_KErrNone(r);
	}

void FillShBuf(RShBuf& aBuffer, TUint8 aValue)
	{
	TUint size = aBuffer.Size();
	TUint8* base = aBuffer.Ptr();
	test(size!=0);
	test(base!=0);
	memset(base,aValue,size);
	}

TBool CheckFillShBuf(RShBuf& aBuffer, TUint8 aValue)
	{
	TUint size = aBuffer.Size();
	TUint8* base = aBuffer.Ptr();
	test(size!=0);
	test(base!=0);
	TUint8* ptr = base;
	TUint8* end = ptr+size;
	while(ptr<end)
		{
		TUint8 b = *ptr++;
		if(b!=aValue)
			{
			RDebug::Printf("CheckFillShBuf failed at offset 0x%x, expected 0x%02x but got 0x%02x ",ptr-base-1,aValue,b);
			return EFalse;
			}
		}
	return ETrue;
	}

TBool CheckNotFillShBuf(RShBuf& aBuffer, TUint8 aValue)
	{
	TUint size = aBuffer.Size();
	TUint8* base = aBuffer.Ptr();
	test(size!=0);
	test(base!=0);
	TUint8* ptr = base;
	TUint8* end = ptr+size;
	while(ptr<end)
		{
		TUint8 b = *ptr++;
		if(b==aValue)
			{
			RDebug::Printf("CheckNotFillShBuf failed at offset 0x%x, expected not 0x%02x",ptr-base-1,aValue);
			return EFalse;
			}
		}
	return ETrue;
	}

/*
@SYMTestCaseID				1
@SYMTestCaseDesc			Create pool from user-side
@SYMREQ						REQ11423
@SYMTestActions
	1. Test Thread creates a pool (P1) and passes handle to device driver.
	2. Device driver opens pool and checks its attributes.
@SYMTestExpectedResults
	All OK.
@SYMTestPriority			Critical
*/

void CreateUserPool(TTestPoolType aPoolType)
	{
	test.Next(_L("Create user-side pool"));
	TInt r;
	TInt pagesize;
	r = HAL::Get(HAL::EMemoryPageSize, pagesize);
	test_KErrNone(r);

	switch (aPoolType)
		{
		case ETestNonPageAligned:
		// Non-page-aligned pool
			{
			test.Printf(_L("Non-page-aligned\n"));
			test_Equal(0, P1.Handle());
			TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, *PtrBufSize, KTestPoolSizeInBufs, 8);
			r = P1.Create(inf,KDefaultPoolHandleFlags);
			test_KErrNone(r);

			r = P1.SetBufferWindow(-1, ETrue);
			test_Equal(KErrNotSupported, r);

			TShPoolInfo poolinfotokernel;
			poolinfotokernel.iBufSize = *PtrBufSize;
			poolinfotokernel.iInitialBufs = KTestPoolSizeInBufs;
			poolinfotokernel.iMaxBufs = KTestPoolSizeInBufs;
			poolinfotokernel.iGrowTriggerRatio = 0;
			poolinfotokernel.iGrowByRatio = 0;
			poolinfotokernel.iShrinkHysteresisRatio = 0;
			poolinfotokernel.iAlignment = 8;
			poolinfotokernel.iFlags = EShPoolNonPageAlignedBuffer;
			r = Ldd.OpenUserPool(P1.Handle(), poolinfotokernel);
			test_KErrNone(r);

			TShPoolInfo poolinfo;
			P1.GetInfo(poolinfo);
			test_Equal(*PtrBufSize, poolinfo.iBufSize);
			test_Equal(KTestPoolSizeInBufs, poolinfo.iInitialBufs);
			test_Equal(KTestPoolSizeInBufs, poolinfo.iMaxBufs);
			test_Equal(0, poolinfo.iGrowTriggerRatio);
			test_Equal(0, poolinfo.iGrowByRatio);
			test_Equal(0, poolinfo.iShrinkHysteresisRatio);
			test_Equal(8, poolinfo.iAlignment);
			test(poolinfo.iFlags & EShPoolNonPageAlignedBuffer);
			test(!(poolinfo.iFlags & EShPoolPageAlignedBuffer));
			break;
			}
		case ETestPageAligned:
		// Page-aligned pool
			{
			test.Printf(_L("Page-aligned\n"));
			test_Equal(0, P1.Handle());

			TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, *PtrBufSize, KTestPoolSizeInBufs);
			r = P1.Create(inf,KDefaultPoolHandleFlags);
			test_KErrNone(r);

			r = P1.SetBufferWindow(-1, ETrue);
			test_KErrNone(r);

			TShPoolInfo poolinfo;
			P1.GetInfo(poolinfo);
			test_Equal(*PtrBufSize, poolinfo.iBufSize);
			test_Equal(KTestPoolSizeInBufs, poolinfo.iInitialBufs);
			test_Equal(KTestPoolSizeInBufs, poolinfo.iMaxBufs);
			test_Equal(0, poolinfo.iGrowTriggerRatio);
			test_Equal(0, poolinfo.iGrowByRatio);
			test_Equal(0, poolinfo.iShrinkHysteresisRatio);
			test_Equal(Log2(pagesize), poolinfo.iAlignment);
			test(poolinfo.iFlags & EShPoolPageAlignedBuffer);
			test(!(poolinfo.iFlags & EShPoolNonPageAlignedBuffer));

			r = Ldd.OpenUserPool(P1.Handle(), poolinfo);
			test_KErrNone(r);
			break;
			}
		case ETestPageAlignedGrowing:
		// Page-aligned growing pool
			{
			test.Printf(_L("Page-aligned growing\n"));
			test_Equal(0, P1.Handle());

			TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, *PtrBufSize, KTestPoolSizeInBufs / 2);
			// Set shrink hysteresis high so pool can't shrink
			r = inf.SetSizingAttributes(KTestPoolSizeInBufs, 25, 26, 25600);
			test_KErrNone(r);
			r = P1.Create(inf,KDefaultPoolHandleFlags);
			test_KErrNone(r);

			r = P1.SetBufferWindow(-1, ETrue);
			test_KErrNone(r);

			TShPoolInfo poolinfo;
			P1.GetInfo(poolinfo);
			test_Equal(*PtrBufSize, poolinfo.iBufSize);
			test_Equal(KTestPoolSizeInBufs / 2, poolinfo.iInitialBufs);
			test_Equal(KTestPoolSizeInBufs, poolinfo.iMaxBufs);
			test_Equal(25, poolinfo.iGrowTriggerRatio);
			test_Equal(26, poolinfo.iGrowByRatio);
			test_Equal(25600, poolinfo.iShrinkHysteresisRatio);
			test_Equal(Log2(pagesize), poolinfo.iAlignment);
			test(poolinfo.iFlags & EShPoolPageAlignedBuffer);
			test(!(poolinfo.iFlags & EShPoolNonPageAlignedBuffer));

			r = Ldd.OpenUserPool(P1.Handle(), poolinfo);
			test_KErrNone(r);
			break;
			}
		default:
			test(EFalse);
		}
	}

/*
@SYMTestCaseID				2
@SYMTestCaseDesc			Create pool from kernel-side
@SYMREQ						REQ11423
@SYMTestActions
	1. Device Driver creates a pool (P2) and passes handle to this thread.
	2. Test Thread opens pool and checks its attributes.
@SYMTestExpectedResults
	1. Ok.
	2. Ok.
@SYMTestPriority			Critical
*/

void CreateKernelPool(TTestPoolType aPoolType)
	{
	test.Next(_L("Create kernel-side pool"));
	TInt r;
	TInt pagesize;
	r = HAL::Get(HAL::EMemoryPageSize, pagesize);
	test_KErrNone(r);
	TInt handle;

	switch (aPoolType)
		{
		case ETestNonPageAligned:
		// Non-page-aligned pool
			{
			test.Printf(_L("Non-page-aligned\n"));
			test_Equal(0, P2.Handle());

			TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, *PtrBufSize, KTestPoolSizeInBufs, 8);
			r = Ldd.OpenKernelPool(inf, handle);
			test_KErrNone(r);
			P2.SetHandle(handle);

			TShPoolInfo poolinfo;
			P2.GetInfo(poolinfo);
			test_Equal(*PtrBufSize, poolinfo.iBufSize);
			test_Equal(KTestPoolSizeInBufs, poolinfo.iInitialBufs);
			test_Equal(KTestPoolSizeInBufs, poolinfo.iMaxBufs);
			test_Equal(0, poolinfo.iGrowTriggerRatio);
			test_Equal(0, poolinfo.iGrowByRatio);
			test_Equal(0, poolinfo.iShrinkHysteresisRatio);
			test_Equal(8, poolinfo.iAlignment);
			test(poolinfo.iFlags & EShPoolNonPageAlignedBuffer);
			test(!(poolinfo.iFlags & EShPoolPageAlignedBuffer));
			break;
			}
		case ETestPageAligned:
		// Page-aligned pool
			{
			test.Printf(_L("Page-aligned\n"));
			test_Equal(0, P2.Handle());

			TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, *PtrBufSize, KTestPoolSizeInBufs);
			r = Ldd.OpenKernelPool(inf, handle);
			test_KErrNone(r);
			P2.SetHandle(handle);

			r = P2.SetBufferWindow(-1, ETrue);
			test_KErrNone(r);

			TShPoolInfo poolinfo;
			P2.GetInfo(poolinfo);
			test_Equal(*PtrBufSize, poolinfo.iBufSize);
			test_Equal(KTestPoolSizeInBufs, poolinfo.iInitialBufs);
			test_Equal(KTestPoolSizeInBufs, poolinfo.iMaxBufs);
			test_Equal(0, poolinfo.iGrowTriggerRatio);
			test_Equal(0, poolinfo.iGrowByRatio);
			test_Equal(0, poolinfo.iShrinkHysteresisRatio);
			test_Equal(Log2(pagesize), poolinfo.iAlignment);
			test(poolinfo.iFlags & EShPoolPageAlignedBuffer);
			test(!(poolinfo.iFlags & EShPoolNonPageAlignedBuffer));
			break;
			}
		case ETestPageAlignedGrowing:
		// Page-aligned pool growing
			{
			test.Printf(_L("Page-aligned growing\n"));
			test_Equal(0, P2.Handle());

			TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, *PtrBufSize, KTestPoolSizeInBufs / 2);
			// Set shrink hysteresis high so pool can't shrink
			r = inf.SetSizingAttributes(KTestPoolSizeInBufs, 25, 26, 25600);
			test_KErrNone(r);
			r = Ldd.OpenKernelPool(inf, handle);
			test_KErrNone(r);
			P2.SetHandle(handle);

			r = P2.SetBufferWindow(-1, ETrue);
			test_KErrNone(r);

			TShPoolInfo poolinfo;
			P2.GetInfo(poolinfo);
			test_Equal(*PtrBufSize, poolinfo.iBufSize);
			test_Equal(KTestPoolSizeInBufs / 2, poolinfo.iInitialBufs);
			test_Equal(KTestPoolSizeInBufs, poolinfo.iMaxBufs);
			test_Equal(25, poolinfo.iGrowTriggerRatio);
			test_Equal(26, poolinfo.iGrowByRatio);
			test_Equal(25600, poolinfo.iShrinkHysteresisRatio);
			test_Equal(Log2(pagesize), poolinfo.iAlignment);
			test(poolinfo.iFlags & EShPoolPageAlignedBuffer);
			test(!(poolinfo.iFlags & EShPoolNonPageAlignedBuffer));
			break;
			}
		default:
			test(EFalse);
		}
	}

/*
@SYMTestCaseID				20
@SYMTestCaseDesc			Close pool from kernel-side
@SYMREQ						REQ11423
@SYMTestActions
	1. Device Driver closes P2.
	2. Test Thread closes P2.
@SYMTestExpectedResults
	1. OK and Access Count is now 1.
	2. OK
@SYMTestPriority			Critical
*/

void CloseKernelPool()
	{
	test.Next(_L("Close kernel-side pool"));
	TInt r;

	r = Ldd.CloseKernelPool();
	test_KErrNone(r);

	P2.Close();

	// wait for memory to be freed
	r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)5000, 0);
	test_KErrNone(r);

	}

/*
@SYMTestCaseID				21
@SYMTestCaseDesc			Close pool from user-side
@SYMREQ						REQ11423
@SYMTestActions
	1. Test Thread closes P1.
	2. Device Driver closes P1.
@SYMTestExpectedResults
	1. OK and Access Count is now 1.
	2. OK.
@SYMTestPriority			Critical
*/

void CloseUserPool()
	{
	test.Next(_L("Close user-side pool"));
	TInt r;

	P1.Close();

	r = Ldd.CloseUserPool();
	test_KErrNone(r);

	// wait for memory to be freed
	r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)5000, 0);
	test_KErrNone(r);
	}

/*
@SYMTestCaseID				3
@SYMTestCaseDesc			Buffer allocation from user-side
@SYMREQ						REQ11423
@SYMTestActions
	1. Test Thread creates a shared buffer on P1.
	2. Test Thread passes buffer to Device Driver.
	3. Device Driver obtains buffer and manipulates its contents.
	4. Device Driver releases buffer.
	5. Test Thread releases buffer.
@SYMTestExpectedResults
	1. Ok.
	2. Ok.
	3. Ok.
	4. Ok.
	5. Ok. Buffer de-allocated.
@SYMTestPriority			Critical
*/

void AllocateUserBuffer()
	{
	test.Next(_L("Allocate user-side buffer"));
	TInt r;
	RShBuf buf;

	// Allocate buffer on POOL 1
	__KHEAP_MARK;
	r = buf.Alloc(P1);
	test_KErrNone(r);
	__KHEAP_CHECK(0);

	TInt i;
	TShPoolInfo poolinfo1;
	P1.GetInfo(poolinfo1);
	TInt blocks = poolinfo1.iBufSize / KTestData1().Length();

	for (i = 0; i < blocks; i++)
		{
		TPtr8(buf.Ptr() + (i * KTestData1().Length()), KTestData1().Length(),KTestData1().Length()).Copy(KTestData1());
		}
	r = Ldd.ManipulateUserBuffer(buf.Handle());

	test_KErrNone(r);

	TBuf8<64> tmp;

	P1.GetInfo(poolinfo1);
	blocks = poolinfo1.iBufSize / tmp.MaxSize();

	for (i = 0 ; i < blocks; i++)
		{
		tmp.Fill(i);
		TPtrC8 ptrc(buf.Ptr() + (i * tmp.Length()), tmp.Length());
		r = tmp.Compare(ptrc);
		test_Equal(0, r);
		}
	buf.Close();
	__KHEAP_MARKEND;

	// Allocate buffer on POOL 2
	__KHEAP_MARK;
	r = buf.Alloc(P2);
	test_KErrNone(r);
	__KHEAP_CHECK(0);

	TShPoolInfo poolinfo2;
	P2.GetInfo(poolinfo2);
	blocks = poolinfo2.iBufSize / KTestData1().Length(); // PC REMOVE

	for (i = 0; i < blocks; i++)
		{
		TPtr8(buf.Ptr() + (i * KTestData1().Length()), KTestData1().Length(),KTestData1().Length()).Copy(KTestData1());
		}

	r = Ldd.ManipulateUserBuffer(buf.Handle());
	test_KErrNone(r);

	P2.GetInfo(poolinfo2);
	blocks = poolinfo2.iBufSize / tmp.MaxSize(); // PC REMOVE

	for (i = 0 ; i < blocks; i++)
		{
		tmp.Fill(i);
		r = tmp.Compare(TPtr8(buf.Ptr() + (i * tmp.Length()), tmp.Length(), tmp.Length()));
		test_Equal(0, r);
		}
	buf.Close();
	__KHEAP_MARKEND;
	}

/*
@SYMTestCaseID				4
@SYMTestCaseDesc			Buffer allocation from kernel-side
@SYMREQ						REQ11423
@SYMTestActions
	1. Device Driver creates a buffer on P2.
	2. Device Driver manipulates buffer and passes it to Test Thread.
	3. Test Thread manipulates buffer and send it back to Device Driver.
	4. Device Driver check buffer's contents and releases it.
@SYMTestExpectedResults
	1. Ok.
	2. Ok.
	3. Ok.
	4. Ok. Buffer de-allocated.
@SYMTestPriority			Critical
*/

void AllocateKernelBuffer()
	{
	test.Next(_L("Allocate kernel-side buffer"));
	TInt r;
	TInt handle;
	RShBuf kbuf0, kbuf1;

	// Allocate buffer on POOL 1
	r = Ldd.AllocateKernelBuffer(0, handle);
	test_KErrNone(r);
	kbuf0.SetHandle(handle);

	TInt i;
	TShPoolInfo poolinfo1;
	P1.GetInfo(poolinfo1);
	TInt blocks = poolinfo1.iBufSize / KTestData2().Length();
	for (i = 0; i < blocks; i++)
		{
		r = KTestData2().Compare(TPtr8(kbuf0.Ptr() + (i * KTestData2().Length()), KTestData2().Length(), KTestData2().Length()));

		test_Equal(0, r);
		}
	kbuf0.Close();

	// Allocate buffer on POOL 2
	r = Ldd.AllocateKernelBuffer(1, handle);
	test_KErrNone(r);
	kbuf1.SetHandle(handle);

	TShPoolInfo poolinfo2;
	P2.GetInfo(poolinfo2);
	blocks = poolinfo2.iBufSize / KTestData2().Length();

	for (i = 0; i < blocks; i++)
		{
		r = KTestData2().Compare(TPtr8(kbuf1.Ptr() + (i * KTestData2().Length()), KTestData2().Length(), KTestData2().Length()));

		test_Equal(0, r);
		}
	kbuf1.Close();
	}


/*
@SYMTestCaseID				X1
@SYMTestCaseDesc			Allocate maximum number of buffers in a pool (user/kernel)
@SYMREQ						REQ11423
@SYMTestActions
	Allocate as many buffers on a pool as possible.
	Free them all and re-allocate them again.
	Free them all.
@SYMTestExpectedResults
	Ok.
@SYMTestPriority			High
*/

void AllocateUserMax(RShPool& aPool)
	{
	test.Next(_L("Exhaust pool memory from user-side"));
	TInt r;

	TShPoolInfo poolinfo;
	aPool.GetInfo(poolinfo);
	TBool aligned = (poolinfo.iFlags & EShPoolPageAlignedBuffer);
	RDebug::Printf("aligned=%d",aligned);

	RArray<RShBuf> bufarray;
	do
		{
		RShBuf buf;
		r = buf.Alloc(aPool);
		RDebug::Printf("alloc buf %d returned %d", bufarray.Count(), r);
		if (r==KErrNoMemory && KTestPoolSizeInBufs>bufarray.Count())
			{
			// try again after a delay, to allow for background resource allocation
			
			User::After(1000000);
			r = buf.Alloc(aPool);
			RDebug::Printf("re-alloc buf %d returned %d", bufarray.Count(), r);
			}
		if (!r)
			{
			r = bufarray.Append(buf);
			test_KErrNone(r);
			FillShBuf(buf,0x99);
			}
		}
	while (r == KErrNone);
	test_Equal(KErrNoMemory, r);
	test_Compare(KTestPoolSizeInBufs, <=, bufarray.Count());

	TInt n = bufarray.Count();
	while (n)
		{
		bufarray[--n].Close();
		}
	RDebug::Printf("closed bufs");

	User::After(500000);

	// Do it once more
	n = 0;
	while (n<bufarray.Count())
		{
		r = bufarray[n].Alloc(aPool);
		RDebug::Printf("alloc buf %d returned %d", n, r);
		if (r==KErrNoMemory)
			{
			// try again after a delay, to allow for background resource allocation
			User::After(1000000);
			r = bufarray[n].Alloc(aPool);
			RDebug::Printf("re-alloc buf %d returned %d", n, r);
			}
		test_Assert(r == KErrNone, test.Printf(_L("n=%d r=%d\n"), n, r));
		if(aligned)
			test(CheckNotFillShBuf(bufarray[n],0x99));
		++n;
		}

	RShBuf extrabuf;
	r = extrabuf.Alloc(aPool);
	RDebug::Printf("alloc extra buf returned %d", r);
	test_Equal(KErrNoMemory, r);

	while (n)
		{
		bufarray[--n].Close();
		}
	RDebug::Printf("closed bufs");

	bufarray.Close();
	}

void AllocateKernelMax()
	{
	test.Next(_L("Exhaust pool memory from kernel-side"));
	TInt r;
	TInt allocated;
	r = Ldd.AllocateMax(0, allocated); // P1
	test_KErrNone(r);
	test_Equal(KTestPoolSizeInBufs, allocated);
	r = Ldd.AllocateMax(1, allocated); // P2
	test_KErrNone(r);
	test_Equal(KTestPoolSizeInBufs, allocated);
	}


/*
@SYMTestCaseID				11
@SYMTestCaseDesc			Buffer alignment (kernel/user)
@SYMREQ						REQ11423
@SYMTestActions
	1. Test Thread creates several pools with different buffer alignment
	   requirements:
	2. Test Thread allocates buffers on all pools.
	3. Test Thread frees all buffers and close pools.
@SYMTestExpectedResults
	1. Ok.
	2. Buffers are aligned to the desired boundary.
	3. Ok.
@SYMTestPriority			High
*/

void BufferAlignmentUser()
	{
	test.Next(_L("Buffer alignment (User)"));
	TInt pagesize;
	TInt r;
	r = HAL::Get(HAL::EMemoryPageSize, pagesize);
	test_KErrNone(r);

	// Non page aligned buffers
	TInt i;
	for (i = 0; i <= Log2(pagesize); i++)
		{
		test.Printf(_L("."));
		TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, *PtrBufSize, 20, i); // TODO: Change minbufs back to 8 when the pool growing code works
		RShPool pool;
		r = pool.Create(inf,KDefaultPoolHandleFlags);
		test_KErrNone(r);

		TInt j;
		RShBuf buf[20];
		for (j = 0; j < 20; j++)
			{
			r = buf[j].Alloc(pool);
			test_KErrNone(r);
			}

		TInt alignment = i;
		if (alignment < KTestMinimumAlignmentLog2)
			{
			alignment = KTestMinimumAlignmentLog2;
			}
		for (j = 0; j < 20; j++)
			{
			test_Assert(!((TUint32) buf[j].Ptr() & ((1 << alignment) - 1)),
				test.Printf(_L("Pool%d buf[%d].Base() == 0x%08x"), i, j, buf[j].Ptr()));
			}

		for (j = 0; j < 20; j++)
			{
			buf[j].Close();
			}
		pool.Close();
		// delay to allow the management dfc to run and close pool
		User::After(100000);
		}
	test.Printf(_L("\n"));

	// Page aligned buffers
	TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, *PtrBufSize, 20); // TODO: Change minbufs back to 8 when the pool growing code works
	RShPool pool;
	r = pool.Create(inf,KDefaultPoolHandleFlags);
	test_KErrNone(r);

	r = pool.SetBufferWindow(-1, ETrue);
	test_KErrNone(r);

	TInt j;
	RShBuf buf[20];
	for (j = 0; j < 20; j++)
		{
		r = buf[j].Alloc(pool);
		test_KErrNone(r);
		}

	for (j = 0; j < 20; j++)
		{
		test_Assert(!((TUint32) buf[j].Ptr() & (pagesize - 1)),
					test.Printf(_L("buf[%d].Base() == 0x%08x"), j, buf[j].Ptr()));
		}
	for (j = 0; j < 20; j++)
		{
		buf[j].Close();
		}
	pool.Close();
	}

void BufferAlignmentKernel()
	{
	test.Next(_L("Buffer alignment (Kernel)"));
	TInt r;

	TInt pagesize;
	r = HAL::Get(HAL::EMemoryPageSize, pagesize);
	test_KErrNone(r);

	for (TInt i = 0; i < Log2(pagesize); i++)
		{
		test.Printf(_L("."));
		r = Ldd.BufferAlignmentKernel(*PtrBufSize, i);
		test_KErrNone(r);
		// delay to allow the management dfc to run
		User::After(100000);
		}
	test.Printf(_L("\n"));
	}

/*
@SYMTestCaseID				6
@SYMTestCaseDesc			Create pool at specific physical address
@SYMREQ						REQ11423
@SYMTestActions
	1. Device Driver allocates memory chunk.
	2. Device Driver requests physical address of this memory chunk.
	3. Device Driver creates pool at physical address of the memory chunk.
	3. Device Driver allocate buffers on pool, free them and close pool.
@SYMTestExpectedResults
	1. Ok.
	2. Ok.
	3. Ok.
	4. Ok
@SYMTestPriority			High
*/

void CreateKernelPoolPhysAddr()
	{
	test.Next(_L("Create pool at specific physical address"));
	TInt r;
	test.Start(_L("Contiguous physical memory"));
	r = Ldd.CreatePoolPhysAddrCont(*PtrBufSize);
	test_KErrNone(r);
	test.Next(_L("Discontiguous physical memory"));
	r = Ldd.CreatePoolPhysAddrNonCont(*PtrBufSize);
	test_KErrNone(r);
	test.End();
	}

/*
@SYMTestCaseID				14
@SYMTestCaseDesc			Buffer separation and overwrites
@SYMREQ						REQ11423
@SYMTestActions
	1. Test Thread creates two pools:
		- A pool with no guard pages.
		- A pool with guard pages.
	2. Allocate two buffers on each pool.
	3. Test Thread creates Secondary Thread.
	4. Secondary Thread starts reading contents of the first buffer and keep
	   reading beyond its limits (using a pointer, not a descriptor).
	5. Secondary Thread starts writing on the first buffer and keep writing beyond
	   its limits (using a pointer, not a descriptor).
	6. Free buffers and close pools.
@SYMTestExpectedResults
	1. Ok.
	2. Ok.
	3. Ok.
	4. Secondary Thread panics when it attempts to read the guard page, if there
	   is one. Otherwise, it moves on to the second buffer. (Secondary Thread will
	   have to be restarted).
	5. Secondary Thread panics when it attempts to write on the guard page if
	   there is one. Otherwise, it carries on writing on to the second buffer.
	6. Ok.
@SYMTestPriority			High
*/

TInt ThreadGuardPagesRead(TAny* aArg)
	{
	TUint8* ptr = (TUint8*) aArg;
	if (ptr == NULL)
		{
		return KErrArgument;
		}
	TInt bufsize = *PtrBufSize;
	TInt i;
	TUint8 val = '$';
	TBool isok = ETrue;
	for (i = 0; i < bufsize; i++)
		{
		if (*(ptr + i) != val)
			{
			isok = EFalse;
			}
		}
	if (!isok)
		{
		return KErrUnknown;
		}
	return KErrNone;
	}

TInt ThreadGuardPagesWrite(TAny* aArg)
	{
	TUint8* ptr = (TUint8*) aArg;
	if (ptr == NULL)
		{
		return KErrArgument;
		}
	TInt bufsize = *PtrBufSize;
	TInt i;
	for (i = 0; i < bufsize; i++)
		{
		*(ptr + i) = '#';
		}
	return KErrNone;
	}

void GuardPages()
	{
	test.Next(_L("Guard pages"));
	TInt pagesize;
	TInt r;
	r = HAL::Get(HAL::EMemoryPageSize, pagesize);
	test_KErrNone(r);

	// Create pools
	RShPool pool1;
	RShPool pool2;
	TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, *PtrBufSize, KTestPoolSizeInBufs);
	r = pool1.Create(inf,KDefaultPoolHandleFlags);
	test_KErrNone(r);

	r = pool1.SetBufferWindow(-1, ETrue);
	test_KErrNone(r);

	r = inf.SetGuardPages();
	test_KErrNone(r);
	r = pool2.Create(inf,KDefaultPoolHandleFlags);
	test_KErrNone(r);

	r = pool2.SetBufferWindow(-1, ETrue);
	test_KErrNone(r);

	// Allocate buffers
	RShBuf bufs1[KTestPoolSizeInBufs];
	RShBuf bufs2[KTestPoolSizeInBufs];
	TInt i;
	for (i = 0; i < KTestPoolSizeInBufs; i++)
		{
		r = bufs1[i].Alloc(pool1);
		test_Assert(r == KErrNone, test.Printf(_L("Pool1: i=%d r=%d\n"), i, r));
		TPtr8 ptr(bufs1[i].Ptr(), bufs1[i].Size(),bufs1[i].Size());
		ptr.Fill('$');
		}
	for (i = 0; i < KTestPoolSizeInBufs; i++)
		{
		r = bufs2[i].Alloc(pool2);
		test_Assert(r == KErrNone, test.Printf(_L("Pool2: i=%d r=%d\n"), i, r));
		TPtr8 ptr(bufs2[i].Ptr(), bufs1[i].Size(),bufs1[i].Size());
		ptr.Fill('$');
		}

	_LIT(KTestThreadRead, "GuardPagesReadTS%dP%dB%d");
	for (i = 0; i < KTestPoolSizeInBufs - 1; i++)
		{
		TBuf<40> threadname;
		RThread thread;
		TRequestStatus rs;

		// 1. Simple read within buffer
		// Pool 1
		threadname.Format(KTestThreadRead, 1, 1, i);
		r = thread.Create(threadname, ThreadGuardPagesRead, KDefaultStackSize, KMinHeapSize, KMinHeapSize,
			(TAny*) bufs1[i].Ptr());
		test_KErrNone(r);
		thread.Logon(rs);
		thread.Resume();
		User::WaitForRequest(rs);
		test_KErrNone(rs.Int());
		test_Equal(EExitKill, thread.ExitType());
		test_KErrNone(thread.ExitReason());
		thread.Close();
		// Pool 2
		threadname.Format(KTestThreadRead, 1, 2, i);
		r = thread.Create(threadname, ThreadGuardPagesRead, KDefaultStackSize, KMinHeapSize, KMinHeapSize,
			(TAny*) bufs2[i].Ptr());
		test_KErrNone(r);
		thread.Logon(rs);
		thread.Resume();
		User::WaitForRequest(rs);
		test_KErrNone(rs.Int());
		test_Equal(EExitKill, thread.ExitType());
		test_KErrNone(thread.ExitReason());
		thread.Close();

		// 2. If the buffer size is not a multiple of the MMU page size, it should be
		// possible to read after the buffer end until the page boundary
		if (*PtrBufSize % pagesize)
			{
			// Pool 1
			threadname.Format(KTestThreadRead, 2, 1, i);
			r = thread.Create(threadname, ThreadGuardPagesRead, KDefaultStackSize, KMinHeapSize, KMinHeapSize,
				(TAny*) (bufs1[i].Ptr() + pagesize - *PtrBufSize % pagesize));
			test_KErrNone(r);
			thread.Logon(rs);
			thread.Resume();
			User::WaitForRequest(rs);
			if (rs.Int() != KErrNone)
				{
				test_Equal(KErrUnknown, rs.Int());
				test_Equal(KErrUnknown, thread.ExitReason());
				}
			test_Equal(EExitKill, thread.ExitType());
			thread.Close();
			// Pool 2
			threadname.Format(KTestThreadRead, 2, 2, i);
			r = thread.Create(threadname, ThreadGuardPagesRead, KDefaultStackSize, KMinHeapSize, KMinHeapSize,
				(TAny*) (bufs2[i].Ptr() + pagesize - *PtrBufSize % pagesize));
			test_KErrNone(r);
			thread.Logon(rs);
			thread.Resume();
			User::WaitForRequest(rs);
			if (rs.Int() != KErrNone)
				{
				test_Equal(KErrUnknown, rs.Int());
				test_Equal(KErrUnknown, thread.ExitReason());
				}
			test_Equal(EExitKill, thread.ExitType());
			thread.Close();
			}

		// 3. Now we attempt to read the first byte on the next page after the end of
		// our buffer.
		TInt offset;
		if (*PtrBufSize % pagesize)
			{
			offset = pagesize - *PtrBufSize % pagesize + 1;
			}
		else
			{
			offset = 1;
			}
		// Pool 1
		if (bufs1[i + 1].Ptr() == bufs1[i].Ptr() + RoundUp(*PtrBufSize, Log2(pagesize)))
			{
			// Only perform this test if the next buffer comes immediately next to this
			// one. This is not necessarily the case on the Flexible Memory Model.
			threadname.Format(KTestThreadRead, 3, 1, i);
			r = thread.Create(threadname, ThreadGuardPagesRead, KDefaultStackSize, KMinHeapSize, KMinHeapSize,
				(TAny*) (bufs1[i].Ptr() + offset));
			test_KErrNone(r);
			thread.Logon(rs);
			thread.Resume();
			User::WaitForRequest(rs);
			if (rs.Int() != KErrNone) // No guard page, so it should be fine
				{
				test_Equal(KErrUnknown, rs.Int());
				test_Equal(KErrUnknown, thread.ExitReason());
				}
			test_Equal(EExitKill, thread.ExitType());
			thread.Close();
			}
		// Pool 2
		TBool jit = User::JustInTime();
		User::SetJustInTime(EFalse);
		threadname.Format(KTestThreadRead, 3, 2, i);
		r = thread.Create(threadname, ThreadGuardPagesRead, KDefaultStackSize, KMinHeapSize, KMinHeapSize,
			(TAny*) (bufs2[i].Ptr() + offset));
		test_KErrNone(r);
		thread.Logon(rs);
		thread.Resume();
		User::WaitForRequest(rs);
		test_Equal(3, rs.Int());
		test_Equal(EExitPanic, thread.ExitType());
		test_Equal(3, thread.ExitReason()); // KERN-EXEC 3
		thread.Close();
		User::SetJustInTime(jit);
		}

	_LIT(KTestThreadWrite, "GuardPagesWriteTS%dP%dB%d");
	for (i = 0; i < KTestPoolSizeInBufs - 1; i++)
		{
		TBuf<40> threadname;
		RThread thread;
		TRequestStatus rs;

		// 1. Simple write within buffer
		// Pool 1
		threadname.Format(KTestThreadWrite, 1, 1, i);
		r = thread.Create(threadname, ThreadGuardPagesWrite, KDefaultStackSize, KMinHeapSize, KMinHeapSize,
			(TAny*) bufs1[i].Ptr());
		test_KErrNone(r);
		thread.Logon(rs);
		thread.Resume();
		User::WaitForRequest(rs);
		test_KErrNone(rs.Int());
		test_Equal(EExitKill, thread.ExitType());
		test_KErrNone(thread.ExitReason());
		thread.Close();
		// Pool 2
		threadname.Format(KTestThreadWrite, 1, 2, i);
		r = thread.Create(threadname, ThreadGuardPagesWrite, KDefaultStackSize, KMinHeapSize, KMinHeapSize,
			(TAny*) bufs2[i].Ptr());
		test_KErrNone(r);
		thread.Logon(rs);
		thread.Resume();
		User::WaitForRequest(rs);
		test_KErrNone(rs.Int());
		test_Equal(EExitKill, thread.ExitType());
		test_KErrNone(thread.ExitReason());
		thread.Close();

		// 2. If the buffer size is not a multiple of the MMU page size, it should be
		// possible to write after the buffer end until the page boundary
		if (*PtrBufSize % pagesize)
			{
			// Pool 1
			threadname.Format(KTestThreadWrite, 2, 1, i);
			r = thread.Create(threadname, ThreadGuardPagesWrite, KDefaultStackSize, KMinHeapSize, KMinHeapSize,
				(TAny*) (bufs1[i].Ptr() + pagesize - *PtrBufSize % pagesize));
			test_KErrNone(r);
			thread.Logon(rs);
			thread.Resume();
			User::WaitForRequest(rs);
			test_KErrNone(rs.Int());
			test_Equal(EExitKill, thread.ExitType());
			test_KErrNone(thread.ExitReason());
			thread.Close();
			// Pool 2
			threadname.Format(KTestThreadWrite, 2, 2, i);
			r = thread.Create(threadname, ThreadGuardPagesWrite, KDefaultStackSize, KMinHeapSize, KMinHeapSize,
				(TAny*) (bufs2[i].Ptr() + pagesize - *PtrBufSize % pagesize));
			test_KErrNone(r);
			thread.Logon(rs);
			thread.Resume();
			User::WaitForRequest(rs);
			test_KErrNone(rs.Int());
			test_Equal(EExitKill, thread.ExitType());
			test_KErrNone(thread.ExitReason());
			thread.Close();
			}

		// 3. Now we attempt to write on the first byte on the next page after the
		// end of our buffer.
		TInt offset;
		if (*PtrBufSize % pagesize)
			{
			offset = pagesize - *PtrBufSize % pagesize + 1;
			}
		else
			{
			offset = 1;
			}
		// Pool 1
		if (bufs1[i + 1].Ptr() == bufs1[i].Ptr() + RoundUp(*PtrBufSize, Log2(pagesize)))
			{
			// Only perform this test if the next buffer comes immediately next to this
			// one. This is not necessarily the case on the Flexible Memory Model.
			threadname.Format(KTestThreadWrite, 3, 1, i);
			r = thread.Create(threadname, ThreadGuardPagesWrite, KDefaultStackSize, KMinHeapSize, KMinHeapSize,
				(TAny*) (bufs1[i].Ptr() + offset));
			test_KErrNone(r);
			thread.Logon(rs);
			thread.Resume();
			User::WaitForRequest(rs);
			test_KErrNone(rs.Int());
			test_Equal(EExitKill, thread.ExitType());
			test_KErrNone(thread.ExitReason());
			thread.Close();
			}

		// Pool 2
		TBool jit = User::JustInTime();
		User::SetJustInTime(EFalse);
		threadname.Format(KTestThreadWrite, 3, 2, i);
		r = thread.Create(threadname, ThreadGuardPagesWrite, KDefaultStackSize, KMinHeapSize, KMinHeapSize,
			(TAny*) (bufs2[i].Ptr() + offset));
		test_KErrNone(r);
		thread.Logon(rs);
		thread.Resume();
		User::WaitForRequest(rs);
		test_Equal(3, rs.Int());
		test_Equal(EExitPanic, thread.ExitType());
		test_Equal(3, thread.ExitReason()); // KERN-EXEC 3
		thread.Close();
		User::SetJustInTime(jit);
		}

	// Free buffers
	for (i = 0; i < KTestPoolSizeInBufs; i++)
		{
		bufs1[i].Close();
		bufs2[i].Close();
		}
	pool1.Close();
	pool2.Close();
	}

/*
@SYMTestCaseID				12
@SYMTestCaseDesc			Buffer mapping
@SYMREQ						REQ11423
@SYMTestActions
	1. Test Thread allocates buffer on a mappable pool.
	2. Test Thread spawns Slave Process.
	3. Test Thread passes buffer handle to Slave Process.
	4. Slave Process attempts to read buffer then write to buffer.
	5. Slave Process maps buffer.
	6. Slave Process attempts to read buffer then write to buffer.
	7. Slave Process unmaps buffer.
	8. Slave Process attempts to read buffer then write to buffer.
	9. Test Thread kills Slave Process and frees buffer.
@SYMTestExpectedResults
	1. Ok.
	2. Ok.
	3. Ok.
	4. Slave Process panics. (and will have to be restarted)
	5. Ok.
	6. Ok.
	7. Ok.
	8. Slave Process panics.
	9. Ok.
@SYMTestPriority			High
*/

TInt ThreadBufferMappingRead(TAny* aArg)
	{
	if (!aArg)
		{
		return KErrArgument;
		}
	RShBuf* buf = (RShBuf*) aArg;
	TUint x = 0;
	TUint i;
	volatile TUint8* ptr = buf->Ptr();

	for (i = 0; i < buf->Size(); i++)
		{
		x += *(ptr + i);
		}
	return KErrNone;
	}

TInt ThreadBufferMappingWrite(TAny* aArg)
	{
	if (!aArg)
		{
		return KErrArgument;
		}
	RShBuf* buf = (RShBuf*) aArg;
	TPtr8 ptr(buf->Ptr(), buf->Size(),buf->Size());
	ptr.Fill('Q');
	return KErrNone;
	}

const TInt KTestBufferMappingPoolTypes = 8;
const TInt KTestBufferMappingTypes = 8;

void BufferMapping()
	{
	test.Next(_L("Buffer Mapping"));
#ifdef __WINS__
	test.Printf(_L("Does not run on the emulator. Skipped\n"));
#else
	TInt r;
	RShPool pool[KTestBufferMappingPoolTypes];
	RShBuf buf[KTestBufferMappingTypes][KTestBufferMappingPoolTypes];
	TUint poolflags[KTestBufferMappingPoolTypes];
	TInt bufferwindow[KTestBufferMappingPoolTypes];
	TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, *PtrBufSize, KTestBufferMappingTypes);

	// POOL TYPES
	// ------------------------------------------
	// Pool no.	AutoMap	Writeable	BufWindow
	// 0			0			0			-1
	// 1			1			0			-1
	// 2			0			0			0
	// 3			1			0			0
	// 4			0			1			-1
	// 5			1			1			-1
	// 6			0			1			0
	// 7			1			1			0

	TInt i;
	test.Printf(_L("Create pools:"));
	for (i = 0; i < KTestBufferMappingPoolTypes; i++)
		{
		poolflags[i] = EShPoolAllocate;
		bufferwindow[i] = 0;
		if (i % 2)
			{
			poolflags[i] |= EShPoolAutoMapBuf;
			}
		if (i > 3)
			{
			poolflags[i] |= EShPoolWriteable;
			}
		if (i % 4 > 1)
			{
			bufferwindow[i] = -1;
			}
		r = pool[i].Create(inf, poolflags[i] & ~EShPoolAutoMapBuf);
		test_KErrNone(r);
		r = pool[i].SetBufferWindow(bufferwindow[i], poolflags[i] & EShPoolAutoMapBuf);
		test_KErrNone(r);
		test.Printf(_L("."));
		}
	test.Printf(_L("\n"));

	// BUFFER TYPES
	// Buffer no.	Actions
	// 0			Alloc unmapped.
	// 1			Alloc unmapped then unmap again.
	// 2			Default Alloc. Unmap if it is a AutoMap pool.
	// 3			Alloc unmapped. Map Read-Only.
	// 4			Default Alloc. Unmap if it is a R/W pool and re-map Read-Only.
	// 5			Alloc unmapped. Map R/W
	// 6			Default Alloc. Unmap and re-map.
	// 7            Default Alloc R/W. Map again with Read-Only setting.
	// Depending on the pool type, the actions above might not always be possible.

	// Buffer allocation
	TInt j;
	test.Printf(_L("Allocate buffers\n"));
	for (j = 0; j < KTestBufferMappingPoolTypes; j++)
		{
		test.Printf(_L("\nPool %d:"), j);
		for (i = 0; i < KTestBufferMappingTypes; i++)
			{
			switch (i % KTestBufferMappingTypes)
				{
				// Unmapped buffers
				case 0:
				case 1:
					// This should always result in an unmapped buffer
					r = buf[i][j].Alloc(pool[j], EShPoolAllocNoMap);
					test_KErrNone(r);

					if((i % KTestBufferMappingTypes) == 1)
						{
						// Alloc unmapped then unmap again.
						r = buf[i][j].UnMap();
						test_Equal(KErrNotFound, r);
						}
					break;
				case 2:
					r = buf[i][j].Alloc(pool[j]);
					if (poolflags[j] & EShPoolAutoMapBuf)
						{
						if (bufferwindow[j] == 0)
							{
							// Can't ask for a mapped buffer when buffer window is not set
							test_Equal(KErrNoMemory, r);
							}
						else
							{
							// Alloc'd buffer was mapped - unmap it
							test_KErrNone(r);
							r = buf[i][j].UnMap();
							test_KErrNone(r);
							}
						}
					else
						{
						// Buffer not mapped
						test_KErrNone(r);
						}
					break;

				// Read-Only buffers
				case 3:
					r = buf[i][j].Alloc(pool[j], EShPoolAllocNoMap);
					test_KErrNone(r);
					r = buf[i][j].Map(ETrue);
					if (bufferwindow[j])
						{
						test_KErrNone(r);
						}
					else
						{
						test_Equal(KErrNoMemory, r);
						}
					break;
				case 4:
					r = buf[i][j].Alloc(pool[j]);
					if (poolflags[j] & EShPoolAutoMapBuf)
						{
						if (bufferwindow[j] == 0)
							{
							// Can't ask for a mapped buffer when buffer window is not set
							test_Equal(KErrNoMemory, r);
							}
						else if (poolflags[j] & EShPoolWriteable)
							{
							// Alloc'd buffer was mapped R/W - re-map it R/O
							test_KErrNone(r);
							r = buf[i][j].UnMap();
							test_KErrNone(r);
							r = buf[i][j].Map(ETrue);
							test_KErrNone(r);
							}
						else
							{
							// Nothing to do
							test_KErrNone(r);
							}
						}
					else
						{
						// Buffer not mapped
						test_KErrNone(r);
						if (bufferwindow[j])
							{
							if (poolflags[j] & EShPoolWriteable)
								{
								// Explicitly map Read-Only
								r = buf[i][j].Map(ETrue);
								test_KErrNone(r);
								}
							else
								{
								// If Pool is RO, map default
								r = buf[i][j].Map();
								test_KErrNone(r);
								}
							}
						else
							{
							// Can't map buffer
							r = buf[i][j].Map(ETrue);
							test_Equal(KErrNoMemory, r);
							}
						}
					break;

				// Mapped for Read-Write
				case 5:
					r = buf[i][j].Alloc(pool[j], EShPoolAllocNoMap);
					test_KErrNone(r);
					r = buf[i][j].Map();
					if (bufferwindow[j] == 0)
						{
						test_Equal(KErrNoMemory, r);
						}
					else if (!(poolflags[j] & EShPoolWriteable))
						{
						test_KErrNone(r);
						}
					else
						{
						test_KErrNone(r);
						}
					break;
				case 6:
				case 7:
					r = buf[i][j].Alloc(pool[j]);
					if (poolflags[j] & EShPoolAutoMapBuf)
						{
						if (bufferwindow[j] == 0)
							{
							// Can't ask for a mapped buffer when buffer window is not set
							test_Equal(KErrNoMemory, r);
							}
						else if (poolflags[j] & EShPoolWriteable)
							{
							// Alloc'd buffer was mapped R/W
							test_KErrNone(r);

                            if((i % KTestBufferMappingTypes) == 7)
                                {
                                // Mapped for Read-Write then remapped as Read-Only
                                r = buf[i][j].Map(true);
                                test_Equal(KErrAlreadyExists, r);
                                }
							}
						}
					else
						{
						// Buffer not mapped
						test_KErrNone(r);
						if (bufferwindow[j])
							{
							if (poolflags[j] & EShPoolWriteable)
								{
								// Default mapping
								r = buf[i][j].Map();
                                test_KErrNone(r);

                                if((i % KTestBufferMappingTypes) == 7)
                                    {
                                    // Mapped for Read-Write then remapped as Read-Only
                                    r = buf[i][j].Map(true);
                                    test_Equal(KErrAlreadyExists, r);
                                    }
								}
							}
						else
							{
							// Can't map buffer
							r = buf[i][j].Map(ETrue);
							test_Equal(KErrNoMemory, r);
							}
						}
					break;

	            default: test(EFalse);
				}
			test.Printf(_L("."));
			}
		}
	test.Printf(_L("\n"));

	// Read and write tests
	_LIT(KTestThreadName, "BufferMappingBuf%d(Test%d)");
	test.Printf(_L("Read & Write tests\n"));
	for (j = 0; j < KTestBufferMappingPoolTypes; j++)
		{
		for (i = 0; i < KTestBufferMappingTypes; i++)
			{
			if (buf[i][j].Handle())
				{
				switch (i % KTestBufferMappingTypes)
					{
					case 1:
					case 2:
					// Buffer not mapped - Read should fail
					if (buf[i][j].Ptr() == NULL)
						{
						RThread thread;
						TRequestStatus threadrs;
						TBuf<40> threadname;
						threadname.Format(KTestThreadName, i, (i % KTestBufferMappingTypes) + 1);
						r = thread.Create(threadname, ThreadBufferMappingRead, KDefaultStackSize, KMinHeapSize, KMinHeapSize, (TAny*) &buf[i][j]);
						test_KErrNone(r);
						thread.Logon(threadrs);
						thread.Resume();
						User::WaitForRequest(threadrs);
						test_Equal(3, threadrs.Int());
						test_Equal(EExitPanic, thread.ExitType());
						test_Equal(3, thread.ExitReason()); // KERN-EXEC 3
						CLOSE_AND_WAIT(thread);
						// Map buffer read-only for next test
						r = buf[i][j].Map(ETrue);
						if (bufferwindow[j])
							{
							test_KErrNone(r);
							}
						else
							{
							test_Equal(KErrNoMemory, r);
							}
						}
					case 3:
					case 4:
					// Buffer mapped for R/O access - Read should not fail
					if (bufferwindow[j] == 0)
						{
						break;
						}
					else
						{
						RThread thread;
						TRequestStatus threadrs;
						TBuf<40> threadname;
						threadname.Format(KTestThreadName, i, (i % KTestBufferMappingTypes) + 1);
						r = thread.Create(threadname, ThreadBufferMappingRead, KDefaultStackSize, KMinHeapSize, KMinHeapSize, (TAny*) &buf[i][j]);
						test_KErrNone(r);
						thread.Logon(threadrs);
						thread.Resume();
						User::WaitForRequest(threadrs);
						test_KErrNone(threadrs.Int());
						test_Equal(EExitKill, thread.ExitType());
						test_KErrNone(thread.ExitReason());
						CLOSE_AND_WAIT(thread);
						}
					// Write should fail
					if (buf[i][j].Ptr())
						{
						RThread thread;
						TRequestStatus threadrs;
						TBuf<40> threadname;
						threadname.Format(KTestThreadName, i, (i % KTestBufferMappingTypes) + 2);
						r = thread.Create(threadname, ThreadBufferMappingWrite, KDefaultStackSize, KMinHeapSize, KMinHeapSize,(TAny*) &buf[i][j]);
						test_KErrNone(r);
						thread.Logon(threadrs);
						thread.Resume();
						User::WaitForRequest(threadrs);
						test_Equal(3, threadrs.Int());
						test_Equal(EExitPanic, thread.ExitType());
						test_Equal(3, thread.ExitReason()); // KERN-EXEC 3
						CLOSE_AND_WAIT(thread);
						// Map buffer read-write for next test
						r = buf[i][j].UnMap();
						if(r != KErrNotFound)
						    {
						    test_KErrNone(r);
						    }
						r = buf[i][j].Map();
			   			test_KErrNone(r);
						}
					case 5:
					case 6:
						// Buffer mapped for R/W access - Write should not fail
					if (bufferwindow[j] == 0  || !(poolflags[j] & EShPoolWriteable))
						{
						break;
						}
					else
						{
						RThread thread;
						TRequestStatus threadrs;
						TBuf<40> threadname;
						threadname.Format(KTestThreadName, i, (i % KTestBufferMappingTypes) + 1);
						r = thread.Create(threadname, ThreadBufferMappingWrite, KDefaultStackSize, KMinHeapSize, KMinHeapSize,(TAny*) &buf[i][j]);
						test_KErrNone(r);
						thread.Logon(threadrs);
						thread.Resume();
						User::WaitForRequest(threadrs);
						test_KErrNone(threadrs.Int());
						test_Equal(EExitKill, thread.ExitType());
						test_KErrNone(thread.ExitReason());
						CLOSE_AND_WAIT(thread);
						// Unmap buffer for next test
						r = buf[i][j].UnMap();
						test_KErrNone(r);
						}
					// Buffer not mapped - Read should fail
					if (buf[i][j].Ptr())
						{
						RThread thread;
						TRequestStatus threadrs;
						TBuf<40> threadname;
						threadname.Format(KTestThreadName, i, (i % KTestBufferMappingTypes) + 2);
						r = thread.Create(threadname, ThreadBufferMappingRead, KDefaultStackSize, KMinHeapSize, KMinHeapSize,(TAny*) &buf[i][j]);
						test_KErrNone(r);
						thread.Logon(threadrs);
						thread.Resume();
						User::WaitForRequest(threadrs);
						test_Equal(3, threadrs.Int());
						test_Equal(EExitPanic, thread.ExitType());
						test_Equal(3, thread.ExitReason()); // KERN-EXEC 3
						CLOSE_AND_WAIT(thread);
						}
					}
				}
			buf[i][j].Close();
			test.Printf(_L("."));
			}
		pool[j].Close();
		test.Printf(_L("\n"));
		}
#endif
	}

void BufferWindow()
	{
	test.Next(_L("Buffer Window tests"));
#ifdef __WINS__
	test.Printf(_L("Does not run on the emulator. Skipped\n"));
#else
	TInt r;
	RShPool pool;
	RShBuf buf[KTestPoolSizeInBufs * 2 + 1];
	TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, *PtrBufSize, KTestPoolSizeInBufs * 2);
	r = pool.Create(inf, KDefaultPoolHandleFlags);
	test_KErrNone(r);

	// Allocate buffer but don't map them to this process memory
	TInt i;
	for (i = 0; i < KTestPoolSizeInBufs * 2; i++)
		{
		r = buf[i].Alloc(pool, EShPoolAllocNoMap);
		test_KErrNone(r);
		}

	// Pool is full
	r = buf[KTestPoolSizeInBufs * 2].Alloc(pool, EShPoolAllocNoMap);
	test_Equal(KErrNoMemory, r);
	r = buf[0].Map();
	test_Equal(KErrNoMemory, r);

	// Open a one-buffer window
	r = pool.SetBufferWindow(1, ETrue);
	test_KErrNone(r);
	r = buf[0].Map();
	test_KErrNone(r);
	TPtr8 ptr0(buf[0].Ptr(), buf[0].Size(),buf[0].Size());
	ptr0.Fill('>');
	r = buf[1].Map();
	test_Equal(KErrNoMemory, r);
	r = buf[0].UnMap();
	test_KErrNone(r);
	r = buf[1].Map();
	test_KErrNone(r);
	TPtr8 ptr1(buf[0].Ptr(), buf[0].Size(),buf[0].Size());
	ptr1.Fill('<');
	r = buf[2].Map();
	test_Equal(KErrNoMemory, r);

	// Enlarge window by one buffer
	r = pool.SetBufferWindow(2, ETrue);
	test_Equal(KErrAlreadyExists, r);

	// Close All buffers
	for (i = 0; i < KTestPoolSizeInBufs * 2; i++)
		{
		buf[i].Close();
		}

	pool.Close();
	r = pool.Create(inf, KDefaultPoolHandleFlags);
	test_KErrNone(r);

	r = pool.SetBufferWindow(KTestPoolSizeInBufs, ETrue); // Half the pool size
	test_KErrNone(r);
	for (i = 0; i < KTestPoolSizeInBufs * 2 - 1; i++)
		{
		if (i < KTestPoolSizeInBufs)
			{
			r = buf[i].Alloc(pool, 0);
			test_KErrNone(r);
			TPtr8 ptr(buf[0].Ptr(), buf[0].Size(),buf[0].Size());
			ptr.Fill('?');
			}
		else
			{
			r = buf[i].Alloc(pool, EShPoolAllocNoMap);
			test_KErrNone(r);
			}
		}
	r = buf[KTestPoolSizeInBufs * 2].Alloc(pool, 0);
	test_Equal(KErrNoMemory, r);
	r = buf[KTestPoolSizeInBufs].Map();
	test_Equal(KErrNoMemory, r);
	r = buf[KTestPoolSizeInBufs * 2].Alloc(pool, EShPoolAllocNoMap);
	test_KErrNone(r);

	// That's it
	for (i = 0; i < (KTestPoolSizeInBufs * 2)  + 1; i++)
		{
		buf[i].Close();
		}
	pool.Close();

	// Try again with automap set to false
	RShPool pool2;
	r = pool2.Create(inf, KDefaultPoolHandleFlags);
	test_KErrNone(r);
	for (i = 0; i < KTestPoolSizeInBufs * 2; i++)
		{
		r = buf[i].Alloc(pool2, 0);
		test_KErrNone(r);
		}
	r = pool2.SetBufferWindow(-1, EFalse);
	test_KErrNone(r);
	for (i = 0; i < KTestPoolSizeInBufs * 2; i++)
		{
		r = buf[i].Map(ETrue);
		test_KErrNone(r);
		}
	for (i = 0; i < KTestPoolSizeInBufs * 2; i++)
		{
		buf[i].Close();
		}
	pool2.Close();
#endif
	}

/*
@SYMTestCaseID				7
@SYMTestCaseDesc			Trigger notifications
@SYMREQ						REQ11423
@SYMTestActions
	Set Low Space Notifications on various thresholds.
	In a separate thread, keep allocating buffers.
@SYMTestExpectedResults
	Notifications are completed when their respective levels are reached.
@SYMTestPriority			Medium
*/

TInt ThreadNotifications(TAny* aArg)
	{
	if (!aArg)
		{
		return KErrArgument;
		}
	RShPool* pool = (RShPool*) aArg;
	RArray<RShBuf> bufarray;
	TInt r;
	RSemaphore sem;
	r = sem.OpenGlobal(KTestLowSpaceSemaphore);
	if (r)
		{
		RDebug::Printf("Line %d: r=%d", __LINE__, r);
		return r;
		}
	// Start allocating buffers
	while (pool->FreeCount() > 1)
		{
		RShBuf buf;
		r = buf.Alloc(*pool);
		if (r)
			{
			RDebug::Printf("Line %d: count=%d r=%d", __LINE__, bufarray.Count(), r);
			return r;
			}
		bufarray.Append(buf);
		if ((bufarray.Count() == 1)								// wait for low3
			|| (bufarray.Count() == KTestPoolSizeInBufs - 2)	// wait for low2
			|| (bufarray.Count() == KTestPoolSizeInBufs - 1))	// wait for low1/low4
				{
				r = sem.Wait(5000000); // 5 second timeout
				if (r)
					{
					RDebug::Printf("Line %d: count=%d r=%d", __LINE__, bufarray.Count(), r);
					return r;
					}
				}
		}

	// Free all buffers
	while (bufarray.Count())
		{
		bufarray[0].Close();
		bufarray.Remove(0);
		if ((bufarray.Count() == KTestPoolSizeInBufs - 2)		// wait for free3
			|| (bufarray.Count() == 1)							// wait for free2
			|| (bufarray.Count() == 0))							// wait for free1/free4
				{
				r = sem.Wait(5000000); // 5 second timeout
				if (r)
					{
					RDebug::Printf("Line %d: count=%d r=%d", __LINE__, bufarray.Count(), r);
					return r;
					}
				}
		}
	bufarray.Close();
	sem.Close();
	return KErrNone;
	}

enum TTestLowSpaceType
	{
	ETestCancelNonExistent,
	ETestCancelTwice
	};

struct TTestThreadLowSpacePanicArgs
	{
	RShPool*			iPool;
	TUint				iThreshold1;
	TUint				iThreshold2;
	TTestLowSpaceType	iType;
	};

TInt ThreadLowSpacePanic(TAny* aArg)
	{
	if (!aArg)
		{
		return KErrArgument;
		}
	TTestThreadLowSpacePanicArgs& targs = *(TTestThreadLowSpacePanicArgs*) aArg;
	TRequestStatus rs;
	if (targs.iType == ETestCancelNonExistent)
		{
		targs.iPool->CancelLowSpaceNotification(rs); // should panic
		}
	else if (targs.iType == ETestCancelTwice)
		{
		targs.iPool->RequestLowSpaceNotification(targs.iThreshold1, rs);
		targs.iPool->CancelLowSpaceNotification(rs);
		targs.iPool->CancelLowSpaceNotification(rs); // should panic
		}
	else
		{
		return KErrArgument;
		}
	return KErrNone;
	}

/*
 * CancelLowSpaceNotification() no longer panic()s if it can't find the
 * notification, so this routine not currently called.
 */
void RequestLowSpacePanic(RShPool& aPool, TUint aThreshold1, TUint aThreshold2, TTestLowSpaceType aType, TInt aLine)
	{
	TBuf<40> threadname;
	threadname.Format(_L("ThreadLowSpacePanic%d"), ++ThreadCounter);
	test.Printf(_L("RequestLowSpacePanic@%d(%S)\n"), aLine, &threadname);
	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	TInt expectedpaniccode = KErrNone;	// Initialised to silence compiler warnings
	switch (aType)
		{
		case ETestCancelNonExistent:
		case ETestCancelTwice:
			expectedpaniccode = KErrNotFound;
			break;
		default:
			test(EFalse);
		}
	//
	TTestThreadLowSpacePanicArgs targs;
	targs.iPool = &aPool;
	targs.iThreshold1 = aThreshold1;
	targs.iThreshold2 = aThreshold2;
	targs.iType = aType;
	//
	RThread threadpanic;
	TRequestStatus threadpanicrs;
	TInt r;
	r = threadpanic.Create(threadname, ThreadLowSpacePanic, KDefaultStackSize, KMinHeapSize, 1 << 20, (TAny*) &targs);
	test_KErrNone(r);
	threadpanic.Logon(threadpanicrs);
	threadpanic.Resume();
	User::WaitForRequest(threadpanicrs);
	//
	test_Equal(expectedpaniccode, threadpanicrs.Int());
	test_Equal(EExitPanic, threadpanic.ExitType());
	test_Equal(expectedpaniccode, threadpanic.ExitReason());
	threadpanic.Close();
	User::SetJustInTime(jit);
	}

void NotificationRequests(RShPool& aPool)
	{
	test.Next(_L("Notifications"));
	TInt r;

	RSemaphore sem;
	r = sem.CreateGlobal(KTestLowSpaceSemaphore, 0);
	test_KErrNone(r);
	RTimer timer;
	r = timer.CreateLocal();
	test_KErrNone(r);
	RThread thread;
	TRequestStatus threadrs;
	TBuf<40> threadname;
	threadname.Format(_L("ThreadNotifications%d"), ++ThreadCounter);
	test.Printf(_L("Create %S\n"), &threadname);
	r = thread.Create(threadname, ThreadNotifications, KDefaultStackSize, KMinHeapSize, 1 << 20, (TAny*) &aPool);
	test_KErrNone(r);
	thread.SetPriority(EPriorityMore);
	thread.Logon(threadrs);

	test.Printf(_L("Low space notification\n"));
	TRequestStatus low1;
	TRequestStatus low2;
	TRequestStatus low3;
	TRequestStatus low4;
	TRequestStatus low5;
	TRequestStatus low6;
	aPool.RequestLowSpaceNotification(1, low1);
	test_Equal(KRequestPending, low1.Int());
	aPool.RequestLowSpaceNotification(2, low2);
	test_Equal(KRequestPending, low2.Int());
	aPool.RequestLowSpaceNotification(aPool.FreeCount() - 1, low3);
	test_Equal(KRequestPending, low3.Int());
	aPool.RequestLowSpaceNotification(1, low4);
	test_Equal(KRequestPending, low4.Int());
	aPool.RequestLowSpaceNotification(0, low5); // Never completes
	test_Equal(KRequestPending, low5.Int());
	aPool.RequestLowSpaceNotification(KMaxTUint, low6); // Completes instantly
	TRequestStatus timeoutlow;
	timer.After(timeoutlow, 5000000); // 5 seconds time out
	User::WaitForRequest(low6, timeoutlow);
	test_KErrNone(low6.Int());
	test_Equal(KRequestPending, low1.Int());
	test_Equal(KRequestPending, low2.Int());
	test_Equal(KRequestPending, low3.Int());
	test_Equal(KRequestPending, low4.Int());
	test_Equal(KRequestPending, low5.Int());
	timer.Cancel();
	User::WaitForRequest(timeoutlow);
	thread.Resume();
	User::WaitForRequest(low3, threadrs);
	test_KErrNone(low3.Int());
	test_Equal(KRequestPending, low1.Int());
	test_Equal(KRequestPending, low2.Int());
	test_Equal(KRequestPending, low4.Int());
	test_Equal(KRequestPending, low5.Int());
	sem.Signal();
	User::WaitForRequest(low2, threadrs);
	test_KErrNone(low2.Int())
	test_Equal(KRequestPending, low1.Int());
	test_Equal(KRequestPending, low4.Int());
	test_Equal(KRequestPending, low5.Int());
	sem.Signal();
	User::WaitForRequest(low1, threadrs);
	test_KErrNone(low1.Int());
	User::WaitForRequest(low4, threadrs);
	test_KErrNone(low4.Int());
	test_Equal(KRequestPending, low5.Int());
	test_Equal(EExitPending, thread.ExitType()); // Thread is still running
	test_Compare(aPool.FreeCount(), <=, 1);

	test.Printf(_L("Free space notification\n"));
	TRequestStatus free1;
	TRequestStatus free2;
	TRequestStatus free3;
	TRequestStatus free4;
	TRequestStatus free5;
	TRequestStatus free6;
	aPool.RequestFreeSpaceNotification(KTestPoolSizeInBufs, free1);
	test_Equal(KRequestPending, free1.Int());
	aPool.RequestFreeSpaceNotification(KTestPoolSizeInBufs - 1, free2);
	test_Equal(KRequestPending, free2.Int());
	aPool.RequestFreeSpaceNotification(aPool.FreeCount() + 1, free3);
	test_Equal(KRequestPending, free3.Int());
	aPool.RequestFreeSpaceNotification(KTestPoolSizeInBufs, free4);
	test_Equal(KRequestPending, free4.Int());
	aPool.RequestFreeSpaceNotification(KTestPoolSizeInBufs + 1, free5); // Never completes
	test_Equal(KRequestPending, free5.Int());
	aPool.RequestFreeSpaceNotification(0, free6); // Completes instantly

	TRequestStatus timeoutfree;
	timer.After(timeoutfree, 5000000); // 5 seconds time out
	User::WaitForRequest(free6, timeoutfree);
	test_KErrNone(free6.Int());

	test_Equal(KRequestPending, free1.Int());
	test_Equal(KRequestPending, free2.Int());
	test_Equal(KRequestPending, free3.Int());
	test_Equal(KRequestPending, free4.Int());
	test_Equal(KRequestPending, free5.Int());

	timer.Cancel();
	User::WaitForRequest(timeoutfree);

	sem.Signal();	// resume thread execution
	User::WaitForRequest(free3, threadrs);
	test_KErrNone(free3.Int());
	test_Equal(KRequestPending, free1.Int());
	test_Equal(KRequestPending, free2.Int());
	test_Equal(KRequestPending, free4.Int());
	test_Equal(KRequestPending, free5.Int());

	sem.Signal();
	User::WaitForRequest(free2, threadrs);
	test_KErrNone(free2.Int())

	test_Equal(KRequestPending, free1.Int());
	test_Equal(KRequestPending, free4.Int());
	test_Equal(KRequestPending, free5.Int());
	sem.Signal();

	User::WaitForRequest(free1, threadrs);
	test_KErrNone(free1.Int());
	test_KErrNone(free4.Int());

	test_Equal(KRequestPending, free5.Int());
	test_Equal(EExitPending, thread.ExitType()); // Thread is still running

	test_Compare(aPool.FreeCount(), >=, KTestPoolSizeInBufs);

	// Complete the requests still pending...
	aPool.CancelLowSpaceNotification(low5);
	User::WaitForRequest(low5);

	aPool.CancelFreeSpaceNotification(free5);
	User::WaitForRequest(free5);

	// Let thread complete
	sem.Signal();
	User::WaitForRequest(threadrs);
	test_Equal(EExitKill, thread.ExitType());
	test_KErrNone(thread.ExitReason());
	thread.Close();
	sem.Close();
	timer.Close();
	}

/*
@SYMTestCaseID				9
@SYMTestCaseDesc			Cancel low- and free-space notifications
@SYMREQ						REQ11423
@SYMTestActions
	Set Low/High LowSpace Notifications.
	Cancel them.
@SYMTestExpectedResults
	All OK.
@SYMTestPriority			Medium
*/

void CancelNotificationRequests(RShPool& aPool)
	{
	test.Next(_L("Cancel notifications"));
	TInt r;

	RSemaphore sem;
	r = sem.CreateGlobal(KTestLowSpaceSemaphore, 0);
	test_KErrNone(r);

	TBuf<40> threadname;
	threadname.Format(_L("ThreadCancelNotifications%d"), ++ThreadCounter);
	test.Printf(_L("Create %S\n"), &threadname);
	RThread thread;
	TRequestStatus threadrs;
	r = thread.Create(threadname, ThreadNotifications, KDefaultStackSize, KMinHeapSize, 1 << 20, (TAny*) &aPool);
	test_KErrNone(r);
	thread.SetPriority(EPriorityLess);
	thread.Logon(threadrs);

	test.Printf(_L("Cancel low space notifications\n"));
	// Low space notification cancel
	TRequestStatus low;
	aPool.RequestLowSpaceNotification(1, low);
	aPool.CancelLowSpaceNotification(low);
	test_Equal(KErrCancel, low.Int());
	// We should be able to cancel again without panic()ing
	// (no guarantees on return code; maybe Cancel() should have void return type?)
	aPool.CancelLowSpaceNotification(low);
	test.Printf(_L("Second cancel returned %d\n"), low.Int());
	TRequestStatus low2;
	aPool.RequestLowSpaceNotification(1, low2); // For thread sync
	thread.Resume();
	sem.Signal(2);
	User::WaitForRequest(low2, threadrs);
	test_KErrNone(low2.Int());
	test_Equal(EExitPending, thread.ExitType()); // Thread is still running
	test_Compare(aPool.FreeCount(), <=, 1);

	test.Printf(_L("Cancel free space notifications\n"));
	TRequestStatus free;
	aPool.CancelFreeSpaceNotification(free);	// Cancel non-existant notification
	aPool.RequestFreeSpaceNotification(KTestPoolSizeInBufs, free);
	aPool.CancelLowSpaceNotification(free);		// Use wrong method
	aPool.CancelFreeSpaceNotification(free);		// Use wrong method
	test_Equal(KErrCancel, free.Int());
	aPool.CancelFreeSpaceNotification(free);		// Already cancelled

	// Complete the requests still pending...
	User::WaitForRequest(low);

	sem.Signal(4); // Resume thread execution and let it complete
	User::WaitForRequest(threadrs);
	test_KErrNone(threadrs.Int());
	test_Equal(EExitKill, thread.ExitType());
	test_KErrNone(thread.ExitReason());
	test_Compare(aPool.FreeCount(), >=, KTestPoolSizeInBufs);
	thread.Close();
	sem.Close();
	}


/*
@SYMTestCaseID				10
@SYMTestCaseDesc			Grow and shrink pool
@SYMREQ						REQ11423
@SYMTestActions
	1. Test Thread creates pools with various size attributes
	2. Test Thread keeps allocating buffers on pool.
	3. Test Thread keeps freeing buffers on pool
	4. Test Thread frees all buffers and close pool.
@SYMTestExpectedResults
	Pools grows and shrink grows as expected.
@SYMTestPriority			High
*/

const TInt KTestFreeCountTimeOut = 20000000; // 20 seconds (of thread inactivity)
const TInt KTestWaitBeforeRetry = 2000; // 0.002 second

TUint MultFx248(TUint n, TUint f)
	{
	TUint64 r = (TUint64) n * f;
	I64LSR(r, 8);
	return r > KMaxTUint32 ? KMaxTUint32 : I64LOW(r);
	}

class TTestPoolModel
	{
public:
	TTestPoolModel(TShPoolInfo& aInfo);
	void Alloc();
	void Free();
	TUint FreeCount();
	void DisplayCounters();
private:
	void CalcGSP();
	void CheckGrowShrink();
	void Grow();
	void Shrink();
private:
	TUint iAllocated;
	TUint iFree;
	//
	TUint iInitial;
	TUint iMax;
	TUint iGrowTriggerRatio;
	TUint iGrowByRatio;
	TUint iShrinkByRatio;
	TUint iShrinkHysteresisRatio;
	TUint iPoolFlags;
	//
	TUint iGrowTrigger;
	TUint iShrinkTrigger;
	//
	TBool iDebug;
	};

TTestPoolModel::TTestPoolModel(TShPoolInfo& aInfo)
	{
	iInitial = aInfo.iInitialBufs;
	iMax = aInfo.iMaxBufs;
	iGrowTriggerRatio = aInfo.iGrowTriggerRatio;
	iGrowByRatio = aInfo.iGrowByRatio;
	iShrinkByRatio = 256 - 65536 / (256 + iGrowByRatio);
	iShrinkHysteresisRatio = aInfo.iShrinkHysteresisRatio;
	iPoolFlags = aInfo.iFlags;
	iAllocated = 0;
	iFree = iInitial;
	iDebug = EFalse; // Set this to ETrue to display detailed information
	
	CalcGSP();
	if (iDebug)
		{
		test.Printf(_L("A     F     A+F   GT    ST    \n"));
		test.Printf(_L("==============================\n"));
		DisplayCounters();
		}
	}

void TTestPoolModel::Alloc()
	{
	iAllocated++;
	iFree--;
	CheckGrowShrink();
	}

void TTestPoolModel::Free()
	{
	iAllocated--;
	iFree++;
	CheckGrowShrink();
	}

TUint TTestPoolModel::FreeCount()
	{
	return iFree;
	}

void TTestPoolModel::CalcGSP()
	{
	TUint n = iAllocated + iFree;

	// If the pool is at its maximum size, we can't grow
	if (n >= iMax || iGrowTriggerRatio == 0 /*|| iCommittedPages >= iMaxPages*/)
		{
		iGrowTrigger = 0;
		}
	else
		{
		iGrowTrigger = MultFx248(n, iGrowTriggerRatio);

		// Deal with rounding towards zero
		if (iGrowTrigger == 0)
			iGrowTrigger = 1;
		}

	// If no growing has happened, we can't shrink
	if (n <= iInitial || iGrowTriggerRatio == 0 || (iPoolFlags & EShPoolSuppressShrink) != 0)
		{
		iShrinkTrigger = iMax;
		}
	else
		{
		// To ensure that shrinking doesn't immediately happen after growing, the trigger
		// amount is the grow trigger + the grow amount (which is the number of free buffers
		// just after a grow) times the shrink hysteresis value.
		iShrinkTrigger = MultFx248(n, iGrowTriggerRatio + iGrowByRatio);
		iShrinkTrigger = MultFx248(iShrinkTrigger, iShrinkHysteresisRatio);

		// Deal with rounding towards zero
		if (iShrinkTrigger == 0)
			iShrinkTrigger = 1;

		// If the shrink trigger ends up > the number of buffers currently in
		// the pool, set it to that number (less 1, since the test is "> trigger").
		// This means the pool will only shrink when all the buffers have been freed.
		if (iShrinkTrigger >= n)
			iShrinkTrigger = n - 1;
		}
	if (iDebug)
		{
		DisplayCounters();
		}
	}

void TTestPoolModel::CheckGrowShrink()
	{
	if (iFree < iGrowTrigger)
		{
		Grow();
		CheckGrowShrink();
		}
	if (iFree > iShrinkTrigger)
		{
		Shrink();
		CheckGrowShrink();
		}
	}

void TTestPoolModel::Grow()
	{
	TUint headroom = iMax - (iAllocated + iFree);
	TUint growby = MultFx248(iAllocated + iFree, iGrowByRatio);
	if (growby == 0)			// Handle round-to-zero
		growby = 1;
	if (growby > headroom)
		growby = headroom;
	iFree += growby;
	if (iDebug)
		{
		test.Printf(_L("GROW by %d!\n"), growby);
		}
	CalcGSP();
	}

void TTestPoolModel::Shrink()
	{
	TUint grownBy = iAllocated + iFree - iInitial;
	TUint shrinkby = MultFx248(iAllocated + iFree, iShrinkByRatio);
	if (shrinkby == 0)			// Handle round-to-zero
		shrinkby = 1;
	if (shrinkby > grownBy)
		shrinkby = grownBy;
	if (shrinkby > iFree)
		shrinkby = iFree;
	iFree -= shrinkby;
	if (iDebug)
		{
		test.Printf(_L("SHRINK by %d!\n"), shrinkby);
		}
	CalcGSP();
	}

void TTestPoolModel::DisplayCounters()
	{
	test.Printf(_L("%-6u%-6u%-6u%-6u%-6u\n"), iAllocated, iFree, iAllocated + iFree, iGrowTrigger, iShrinkTrigger);
	}

const TInt KMaxCountAlignmentRetries = 4;
#define CHECK_COUNT_ALIGNMENT(a,b) CheckCountAlignmentWithTimeout(a,b,(TUint)__LINE__)
#define RETRY_BUF_ALLOC(a,b,c,d,e) RetryBufAlloc(a,b,c,d,e,(TUint)__LINE__)
#define RETRY_BUF_FREE(a,b,c,d) RetryBufFree(a,b,c,d,(TUint)__LINE__)

TInt CheckCountAlignmentWithTimeout(TTestPoolModel& aModel, RShPool& aPool, const TUint aLineNum)
	{
	// Allow a maximum time for the model and pool counts to align
	// Return the period remaining after they align (zero if alignement is not reached)
	TInt timeout = KTestFreeCountTimeOut / KTestWaitBeforeRetry;
	while (aModel.FreeCount() != aPool.FreeCount())
		{
		User::After(KTestWaitBeforeRetry);
		if(--timeout == 0)
			{
			test.Printf(_L("Timeout: Free==%u (expected %u, line %d)\n"), aPool.FreeCount(), aModel.FreeCount(), aLineNum);
			aModel.DisplayCounters();
			return timeout;
			}
		if ((timeout * KTestWaitBeforeRetry) % 1000000 == 0)
			{
			test.Printf(_L("Time out in %d seconds! (line %d)\n"), timeout * KTestWaitBeforeRetry / 1000000, aLineNum);
			}
		--timeout;
		};
	return timeout;
	}

TInt RetryBufAlloc(RArray<RShBuf>& aBufarray, TTestPoolModel& aModel, RShPool& aPool, RShBuf& aBuf, TUint aBufferFlags, const TUint aLineNum)
	{
	// Free a buffer, then alloc
	TShPoolInfo info;
	aPool.GetInfo(info);

	aBufarray[aBufarray.Count() - 1].Close();
	aBufarray.Remove(aBufarray.Count() - 1);
	aModel.Free();		
	//
	TInt r = aBuf.Alloc(aPool, aBufferFlags);
	if (r)
		{
		test.Printf(_L("Line %d, Re-Alloc fail, after %d of %d; Free==%u (expected %u)\n"),
			aLineNum, aBufarray.Count(), info.iMaxBufs, aPool.FreeCount(), aModel.FreeCount());
		}
	else
		{
		aModel.Alloc();
		if (!(aBufferFlags & EShPoolAllocNoMap))
			{
			TPtr8 ptr(aBuf.Ptr(), aBuf.Size(),aBuf.Size());
			ptr.Fill(aBufarray.Count() % 256);
			}
		aBufarray.Append(aBuf);
		}

	return r;
	}

TInt RetryBufFree(RArray<RShBuf>& aBufarray, TTestPoolModel& aModel, RShPool& aPool, TUint aBufferFlags, const TUint aLineNum)
	{
	// Allocate a buffer, then free it
	TShPoolInfo info;
	aPool.GetInfo(info);
	RShBuf buf;
	TInt r = buf.Alloc(aPool, aBufferFlags);
	if (r)
		{
		test.Printf(_L("Line %d, Re-Alloc fail, after %d of %d; Free==%u (expected %u)\n"),
			aLineNum, aBufarray.Count(), info.iMaxBufs, aPool.FreeCount(), aModel.FreeCount());
		}
	else
		{
		aModel.Alloc();
		if (!(aBufferFlags & EShPoolAllocNoMap))
			{
			TPtr8 ptr(buf.Ptr(), buf.Size(),buf.Size());
			ptr.Fill(aBufarray.Count() % 256);
			}
		aBufarray.Append(buf);
		//
		aBufarray[aBufarray.Count() - 1].Close();
		aBufarray.Remove(aBufarray.Count() - 1);
		aModel.Free();		
		}
	return r;
	}



void PoolGrowingTestRoutine(const TShPoolCreateInfo& aInfo, TUint aBufferFlags = 0)
	{
	TInt r;
	RShPool pool;
	r = pool.Create(aInfo, KDefaultPoolHandleFlags);
	test_KErrNone(r);

	TShPoolInfo info;
	pool.GetInfo(info);

	// Only set the buffer window if we're going to map the buffers
	if (!(aBufferFlags & EShPoolAllocNoMap) && (info.iFlags & EShPoolPageAlignedBuffer))
		{
		r = pool.SetBufferWindow(-1, ETrue);
		test_KErrNone(r)
		}

	TTestPoolModel model(info);
	RArray<RShBuf> bufarray;
	test_Equal(info.iInitialBufs, pool.FreeCount());

	// Buffer allocation
	TInt retriesRemaining = KMaxCountAlignmentRetries;
	do
		{
		RShBuf buf;
		r = buf.Alloc(pool, aBufferFlags);
		if (r == KErrNoMemory)
			{
			// We expect to get a failure when all buffers are allocated
			if ((TUint) bufarray.Count() == info.iMaxBufs)
				break;
			if (!(aBufferFlags & EShPoolAllocCanWait))
				{
				// Give the Management DFC some time to run, then try allocating again
				User::After(1000000); // 1 second
				r = buf.Alloc(pool);
				if (r)
					{
					test.Printf(_L("Alloc fail after %d of %d; Free==%u (expected %u)\n"),
						bufarray.Count(), info.iMaxBufs, pool.FreeCount(), model.FreeCount());
					break;
					}
				}
			}

		if (r == KErrNone)
			{
			model.Alloc();
			if (!(aBufferFlags & EShPoolAllocNoMap))
				{
				TPtr8 ptr(buf.Ptr(), buf.Size(),buf.Size());
				ptr.Fill(bufarray.Count() % 256);
				}
			bufarray.Append(buf);

			while ((!CHECK_COUNT_ALIGNMENT(model,pool)) && retriesRemaining--)
				{
				// Count mismatch. Due to the operation of this test (single Alloc, then wait and check)
				// it is possible for a count mis-match to occur. This is not a problem in normal operation
				// as the kernel-side count will increase on the next Alloc call, triggering the pool growth.
				// For now, remove the just-added buffer then repeat the  Alloc
				// (but only do this for a maximum number of times, to preclude getting stuck in an infinite loop).
				test_Assert(retriesRemaining,
					test.Printf(_L("Timeout: Free==%u (expected %u), retries remaining: %d\n"), pool.FreeCount(), model.FreeCount(), retriesRemaining);
					model.DisplayCounters();
					);
				//
				r = RETRY_BUF_ALLOC(bufarray, model, pool, buf, aBufferFlags);
				};
			retriesRemaining = KMaxCountAlignmentRetries;
			}

		}
	while (r == KErrNone);

	test_Equal(KErrNoMemory, r);
	test_Equal(info.iMaxBufs, bufarray.Count());
	test_Equal(0, pool.FreeCount());

	// Now free no more than 1/3 of these buffers...
	while ((TUint) bufarray.Count() > 2 * info.iMaxBufs / 3)
		{
		// remove buffers from the back of the array
		if (!(aBufferFlags & EShPoolAllocNoMap))
			{
			TPtr8 ptr(bufarray[bufarray.Count() - 1].Ptr(), bufarray[bufarray.Count() - 1].Size(),bufarray[bufarray.Count() - 1].Size());
			ptr.Fill((bufarray.Count() + 1) % 256);
			}
		bufarray[bufarray.Count() - 1].Close();
		bufarray.Remove(bufarray.Count() - 1);
		model.Free();		
		
		while ((!CHECK_COUNT_ALIGNMENT(model,pool)) && retriesRemaining--)
			{
			// Count mismatch. Re-add the buffer then repeat the Free.
			// (but only do this for a maximum number of times, to preclude getting stuck in an infinite loop).
			test_Assert(retriesRemaining,
				test.Printf(_L("Timeout: Free==%u (expected %u), retries remaining: %d\n"), pool.FreeCount(), model.FreeCount(), retriesRemaining);
				model.DisplayCounters();
				);
			//
			r = RETRY_BUF_FREE(bufarray, model, pool, aBufferFlags);
			};
		retriesRemaining = KMaxCountAlignmentRetries;		
		}

	// ... and re-allocate them
	do
		{
		RShBuf buf;
		r = buf.Alloc(pool, aBufferFlags);
		if (r == KErrNoMemory)
			{
			// We expect to get a failure when all buffers are allocated
			if ((TUint) bufarray.Count() == info.iMaxBufs)
				break;
			if (!(aBufferFlags & EShPoolAllocCanWait))
				{
				// Give the Management DFC some time to run, then try allocating again
				User::After(1000000); // 1 second
				r = buf.Alloc(pool);
				if (r)
					{
					test.Printf(_L("Alloc fail after %d of %d; Free==%u (expected %u)\n"),
						bufarray.Count(), info.iMaxBufs, pool.FreeCount(), model.FreeCount());
					break;
					}
				}
			}

		if (r == KErrNone)
			{
			model.Alloc();
			if (!(aBufferFlags & EShPoolAllocNoMap))
				{
				TPtr8 ptr(buf.Ptr(), buf.Size(),buf.Size());
				ptr.Fill(bufarray.Count() % 256);
				}
			bufarray.Append(buf);

			while ((!CHECK_COUNT_ALIGNMENT(model,pool)) && retriesRemaining--)
				{
				// Count mismatch. Remove the just-added buffer then repeat the  Alloc
				// (but only do this for a maximum number of times, to preclude getting stuck in an infinite loop).
				test_Assert(retriesRemaining,
					test.Printf(_L("Timeout: Free==%u (expected %u), retries remaining: %d\n"), pool.FreeCount(), model.FreeCount(), retriesRemaining);
					model.DisplayCounters();
					);
				//
				r = RETRY_BUF_ALLOC(bufarray, model, pool, buf, aBufferFlags);
				};
			retriesRemaining = KMaxCountAlignmentRetries;
			}
		}
	while (r == KErrNone);

	test_Equal(KErrNoMemory, r);
	test_Equal(info.iMaxBufs, bufarray.Count());
	test_Equal(0, pool.FreeCount());

	// Free all buffers
	while (bufarray.Count())
		{
		// remove buffers from the back of the array
		if (!(aBufferFlags & EShPoolAllocNoMap))
			{
			TPtr8 ptr(bufarray[bufarray.Count() - 1].Ptr(), bufarray[bufarray.Count() - 1].Size(),bufarray[bufarray.Count() - 1].Size());
			ptr.Fill((bufarray.Count() + 1) % 256);
			}
		bufarray[bufarray.Count() - 1].Close();
		bufarray.Remove(bufarray.Count() - 1);
		model.Free();	
		
		while ((!CHECK_COUNT_ALIGNMENT(model,pool)) && retriesRemaining--)
			{
			// Count mismatch. Re-add the buffer then repeat the Free.
			// (but only do this for a maximum number of times, to preclude getting stuck in an infinite loop).
			test_Assert(retriesRemaining,
				test.Printf(_L("Timeout: Free==%u (expected %u), retries remaining: %d\n"), pool.FreeCount(), model.FreeCount(), retriesRemaining);
				model.DisplayCounters();
				);
			//
			r = RETRY_BUF_FREE(bufarray, model, pool, aBufferFlags);
			};
		retriesRemaining = KMaxCountAlignmentRetries;		
		}

	// Pool should have shrunk back to its initial size
	test_Equal(info.iInitialBufs, pool.FreeCount());
	bufarray.Close();
	pool.Close();
	}

void PoolGrowingUser()
	{
	test.Next(_L("Pool Growing/Shrinking (User)"));
	TInt r;
	TInt pagesize;
	r = HAL::Get(HAL::EMemoryPageSize, pagesize);
	test_KErrNone(r);
	// Pool A: Non-page aligned pool (64-byte alignment)
		{
		TInt alignment = 6;
		TInt maxbufs = KTestPoolSizeInBytes / RoundUp(*PtrBufSize, alignment);
		if (maxbufs > 32000)
			{
			maxbufs = 32000;
			}
		TInt initialbufs = maxbufs / 2;
		TInt growtrigger = 32;
		TInt growby = 32;
		TInt shrinkhys = 288;
		test.Printf(_L("POOL A: BufSize=%d InitialBufs=%d MaxBufs=%d GrowTrigger=%d GrowBy=%d ShrinkHys=%d Alignment=%d\n"),
			*PtrBufSize, initialbufs, maxbufs, growtrigger, growby, shrinkhys, alignment);
		TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, *PtrBufSize, initialbufs, alignment);
		r = inf.SetSizingAttributes(maxbufs, growtrigger, growby, shrinkhys);
		test_KErrNone(r);
		PoolGrowingTestRoutine(inf);
		}

	// Pool B: Non-page aligned pool (maximum alignment)
		{
		TInt alignment = Log2(pagesize);
		TInt maxbufs = KTestPoolSizeInBytes / RoundUp(*PtrBufSize, alignment);
		if (maxbufs > 32000)
			{
			maxbufs = 32000;
			}
		TInt initialbufs = maxbufs / 4;
		TInt growtrigger = 32;
		TInt growby = 32;
		TInt shrinkhys = 288;
		test.Printf(_L("POOL B: BufSize=%d InitialBufs=%d MaxBufs=%d GrowTrigger=%d GrowBy=%d ShrinkHys=%d Alignment=%d\n"),
			*PtrBufSize, initialbufs, maxbufs, growtrigger, growby, shrinkhys, alignment);
		TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, *PtrBufSize, initialbufs, alignment);
		r = inf.SetSizingAttributes(maxbufs, growtrigger, growby, shrinkhys);
		test_KErrNone(r);
		PoolGrowingTestRoutine(inf);
		}
	
	// Pool C: Page aligned pool without guard pages
		{
		TInt maxbufs = KTestPoolSizeInBytes / RoundUp(*PtrBufSize, Log2(pagesize));
		if (maxbufs > 32000)
			{
			maxbufs = 32000;
			}
		TInt initialbufs = maxbufs * 3 / 8;
		TInt growtrigger = 32;
		TInt growby = 32;
		TInt shrinkhys = 288;
		test.Printf(_L("POOL C: BufSize=%d InitialBufs=%d MaxBufs=%d GrowTrigger=%d GrowBy=%d ShrinkHys=%d Page-Aligned\n"),
			*PtrBufSize, initialbufs, maxbufs, growtrigger, growby, shrinkhys);
		TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, *PtrBufSize, initialbufs);
		r = inf.SetSizingAttributes(maxbufs, growtrigger, growby, shrinkhys);
		test_KErrNone(r);
		PoolGrowingTestRoutine(inf);
		}

	// Pool D: Page aligned pool without guard pages
		{
		TInt maxbufs = KTestPoolSizeInBytes / RoundUp(*PtrBufSize, Log2(pagesize));
		if (maxbufs > 32000)
			{
			maxbufs = 32000;
			}
		TInt initialbufs = maxbufs / 2;
		TInt growtrigger = 32;
		TInt growby = 32;
		TInt shrinkhys = 288;
		test.Printf(_L("POOL D: BufSize=%d InitialBufs=%d MaxBufs=%d GrowTrigger=%d GrowBy=%d ShrinkHys=%d Page-Aligned+Guard\n"),
			*PtrBufSize, initialbufs, maxbufs, growtrigger, growby, shrinkhys);
		TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, *PtrBufSize, initialbufs);
		r = inf.SetSizingAttributes(maxbufs, growtrigger, growby, shrinkhys);
		test_KErrNone(r);
		r = inf.SetGuardPages();
		test_KErrNone(r);
		PoolGrowingTestRoutine(inf);
		}

	// Pool A': Non-page aligned pool (64-byte alignment)
		{
		TInt alignment = 6;
		TInt maxbufs = KTestPoolSizeInBytes / RoundUp(*PtrBufSize, alignment);
		if (maxbufs > 32000)
			{
			maxbufs = 32000;
			}
		TInt initialbufs = 1;
		TInt growtrigger = 32;
		TInt growby = 256;
		TInt shrinkhys = 512;
		test.Printf(_L("POOL A': BufSize=%d InitialBufs=%d MaxBufs=%d GrowTrigger=%d GrowBy=%d ShrinkHys=%d Alignment=%d\n"),
			*PtrBufSize, initialbufs, maxbufs, growtrigger, growby, shrinkhys, alignment);
		TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, *PtrBufSize, initialbufs, alignment);
		r = inf.SetSizingAttributes(maxbufs, growtrigger, growby, shrinkhys);
		test_KErrNone(r);
		PoolGrowingTestRoutine(inf);
		}

	// Pool A'': Non-page aligned pool (64-byte alignment) - AllocCanWait
		{
		TInt alignment = 6;
		TInt maxbufs = KTestPoolSizeInBytes / RoundUp(*PtrBufSize, alignment);
		if (maxbufs > 32000)
			{
			maxbufs = 32000;
			}
		TInt initialbufs = 1;
		TInt growtrigger = 1;
		TInt growby = 1;
		TInt shrinkhys = 257;
		test.Printf(_L("POOL A'': BufSize=%d InitialBufs=%d MaxBufs=%d GrowTrigger=%d GrowBy=%d ShrinkHys=%d Alignment=%d\n"),
			*PtrBufSize, initialbufs, maxbufs, growtrigger, growby, shrinkhys, alignment);
		TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, *PtrBufSize, initialbufs, alignment);
		r = inf.SetSizingAttributes(maxbufs, growtrigger, growby, shrinkhys);
		test_KErrNone(r);
		PoolGrowingTestRoutine(inf, EShPoolAllocCanWait);
		}

	// Pool D': Page aligned pool without guard pages
		{
		TInt maxbufs = KTestPoolSizeInBytes / RoundUp(*PtrBufSize, Log2(pagesize));
		if (maxbufs > 32000)
			{
			maxbufs = 32000;
			}
		TInt initialbufs = 1;
		TInt growtrigger = 1;
		TInt growby = 1024;
		TInt shrinkhys = 2048;
		test.Printf(_L("POOL D': BufSize=%d InitialBufs=%d MaxBufs=%d GrowTrigger=%d GrowBy=%d ShrinkHys=%d Page-Aligned+Guard\n"),
			*PtrBufSize, initialbufs, maxbufs, growtrigger, growby, shrinkhys);
		TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, *PtrBufSize, initialbufs);
		r = inf.SetSizingAttributes(maxbufs, growtrigger, growby, shrinkhys);
		test_KErrNone(r);
		r = inf.SetGuardPages();
		test_KErrNone(r);
		PoolGrowingTestRoutine(inf);
		}
	// Pool D'': Page aligned pool without guard pages - NoBufferMap
		{
		TInt maxbufs = KTestPoolSizeInBytes / RoundUp(*PtrBufSize, Log2(pagesize));
		if (maxbufs > 32000)
			{
			maxbufs = 32000;
			}
		TInt initialbufs = maxbufs / 2;
		TInt growtrigger = 32;
		TInt growby = 32;
		TInt shrinkhys = 288;
		test.Printf(_L("POOL D'': BufSize=%d InitialBufs=%d MaxBufs=%d GrowTrigger=%d GrowBy=%d ShrinkHys=%d Page-Aligned+Guard\n"),
			*PtrBufSize, initialbufs, maxbufs, growtrigger, growby, shrinkhys);
		TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, *PtrBufSize, initialbufs);
		r = inf.SetSizingAttributes(maxbufs, growtrigger, growby, shrinkhys);
		test_KErrNone(r);
		r = inf.SetGuardPages();
		test_KErrNone(r);
		PoolGrowingTestRoutine(inf, EShPoolAllocNoMap);
		}
	}

/*
@SYMTestCaseID				X3
@SYMTestCaseDesc			Contiguous buffer allocation
@SYMREQ						REQ11423
@SYMTestActions
	Create a pool with the Contiguous attribute and allocate buffers.
@SYMTestExpectedResults
	Buffers memory is physically contiguous.
@SYMTestPriority			High
*/

void ContiguousPoolKernel()
	{
	test.Next(_L("Contiguous Pool (Kernel)"));
#ifdef __WINS__
	test.Printf(_L("Does not run on the emulator. Skipped\n"));
#else
	TInt r;
	TInt pagesize;
	r = HAL::Get(HAL::EMemoryPageSize, pagesize);
	test_KErrNone(r);
	if (*PtrBufSize <= pagesize)
		{
		test.Printf(_L("Buffer size <= page size. Skipped.\n"));
		return;
		}

	TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, *PtrBufSize, KTestPoolSizeInBufs);
//	r = inf.SetSizingAttributes(KTestPoolSizeInBufs, 25, 25, 25600);
//	test_KErrNone(r);

	r = Ldd.ContiguousPoolKernel(inf);
	test_KErrNone(r);

#endif // __WINS__
	}

void ShBufPin()
	{
	test.Next(_L("Buffer pinning"));
#ifdef __WINS__
	test.Printf(_L("Does not run on the emulator. Skipped\n"));
#else
	TInt r;
	RShPool pool1;
	RShBuf buf1;
	TShPoolCreateInfo inf1(TShPoolCreateInfo::ENonPageAlignedBuffer, *PtrBufSize * KTestPoolSizeInBufs, 1, KTestMinimumAlignmentLog2);
	r = pool1.Create(inf1, KDefaultPoolHandleFlags);
	test_KErrNone(r);
	r = buf1.Alloc(pool1);
	test_KErrNone(r);
	r = Ldd.PinBuffer(pool1.Handle(), buf1.Handle());
	test_KErrNone(r);
	buf1.Close();
	pool1.Close();
	
	RShPool pool2;
	RShBuf buf2;
	TShPoolCreateInfo inf2(TShPoolCreateInfo::ENonPageAlignedBuffer, *PtrBufSize * KTestPoolSizeInBufs, 1, KTestMinimumAlignmentLog2);
	r = pool2.Create(inf2, KDefaultPoolHandleFlags);
	test_KErrNone(r);
	r = buf2.Alloc(pool2);
	test_KErrNone(r);
	r = Ldd.PinBuffer(pool2.Handle(), buf2.Handle());
	test_KErrNone(r);
	buf2.Close();
	pool2.Close();
#endif // _WINS_
	}

/*
@SYMTestCaseID
@SYMTestCaseDesc
@SYMREQ
@SYMTestActions
@SYMTestExpectedResults
@SYMTestPriority
*/

void SingleBufferPool()
	{
	test.Next(_L("Single Buffer Pool"));
	TInt r;

	RShPool pool;
	RShBuf buf;
	RShBuf buf2;

	TShPoolCreateInfo infpa(TShPoolCreateInfo::EPageAlignedBuffer, *PtrBufSize * KTestPoolSizeInBufs, 1);
	r = infpa.SetGuardPages();
	test_KErrNone(r);
	r = pool.Create(infpa, KDefaultPoolHandleFlags);
	test_KErrNone(r);
	r = pool.SetBufferWindow(-1, ETrue);
	test_KErrNone(r);
	r = buf.Alloc(pool);
	test_KErrNone(r);
	r = buf2.Alloc(pool);
	test_Equal(KErrNoMemory, r);
	TPtr8(buf.Ptr(), buf.Size(), buf.Size()).Fill('!');
	buf.Close();
	pool.Close();

	TShPoolCreateInfo infnpa(TShPoolCreateInfo::ENonPageAlignedBuffer, *PtrBufSize * KTestPoolSizeInBufs, 1, KTestMinimumAlignmentLog2);
	r = pool.Create(infnpa, KDefaultPoolHandleFlags);
	test_KErrNone(r);
	r = buf.Alloc(pool);
	test_KErrNone(r);
	r = buf2.Alloc(pool);
	test_Equal(KErrNoMemory, r);
	TPtr8(buf.Ptr(), buf.Size(),buf.Size()).Fill('?');
	buf.Close();
	pool.Close();
	}

/*
@SYMTestCaseID				X4
@SYMTestCaseDesc			Negative tests (user/kernel)
@SYMREQ						REQ11423
@SYMTestActions
	API calls with invalid arguments.
@SYMTestExpectedResults
	Appropriate error code returned.
@SYMTestPriority			High
*/

void NegativeTestsUser()
	{
	test.Next(_L("Negative tests (User)"));
	TInt r;
	TInt pagesize;
	TInt ram;
	r = HAL::Get(HAL::EMemoryPageSize, pagesize);
	test_KErrNone(r);
	r = HAL::Get(HAL::EMemoryRAM, ram);
	test_KErrNone(r);
	test_Compare(ram, >, 0);
	// Total system RAM returned by EMemoryRAM should always be < 2GB anyway
	TUint uram = (TUint) ram;
	test.Printf(_L("Total system RAM: %u bytes\n"), uram);
	test.Printf(_L("Page size: %d bytes\n"), pagesize);

	RShPool pool;
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 0, 0); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 100, 0); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 0, 100); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, KMaxTUint, 10); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 10, KMaxTUint); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, KMaxTUint, KMaxTUint); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 65537, 65536); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 10, 1 + (1 << (32 - Log2(pagesize)))); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 4096, 10); r = pool.Create(inf, KDefaultPoolHandleFlags); test_Equal(KErrNone, r); pool.Close(); }
	// XXX The following test will need updating in Phase 2, when exclusive access will be supported
	// (page-aligned-buffer pools only)
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 4096, 10); inf.SetExclusive(); r = pool.Create(inf, KDefaultPoolHandleFlags); test_Equal(KErrNotSupported, r); pool.Close(); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 4096, 10, 12); r = pool.Create(inf, KDefaultPoolHandleFlags); test_Equal(KErrNone, r); pool.Close(); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 4096, 10, 12); inf.SetExclusive(); r = pool.Create(inf, KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); pool.Close(); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 128 * pagesize, (uram / (128 * pagesize)) + 1); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrNoMemory, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 0, 0, 0); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 100, 0, 0); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 0, 100, 0); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, KMaxTUint, 10, 0); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 10, KMaxTUint, 0); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, KMaxTUint, KMaxTUint, 0); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 65537, 65536, 0); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 10, 10, KMaxTUint); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 10, 10, 33); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 10, 300, 24); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 10, 65537, 16); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }
	{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 10, 10, Log2(pagesize) + 1); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r); }

		{
		TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, *BufferSize, KTestPoolSizeInBufs, 0);
		inf.SetGuardPages();
		r = pool.Create(inf, KDefaultPoolHandleFlags); test_Equal(KErrArgument, r);
		r = inf.SetSizingAttributes(KTestPoolSizeInBufs - 1, 25, 25, 280); test_KErrNone(r); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r);
		// Either grow trigger ratio or grow by ratio == 0 => non-growable pool
		// Such pools must have initial buffers == max buffers
		r = inf.SetSizingAttributes(KTestPoolSizeInBufs * 2, 1, 0, 1); test_Equal(KErrArgument, r); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r);
		r = inf.SetSizingAttributes(KTestPoolSizeInBufs * 2, 1, 0, 0); test_Equal(KErrArgument, r); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r);
		// shrink hysteresis ratio must be > 256
		r = inf.SetSizingAttributes(KTestPoolSizeInBufs - 1, 25, 25, 256); test_Equal(KErrArgument, r); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r);
		// grow ratio must be < 256
		r = inf.SetSizingAttributes(KTestPoolSizeInBufs * 2, 256, 25, 260); test_Equal(KErrArgument, r); r = pool.Create(inf,KDefaultPoolHandleFlags); test_Equal(KErrArgument, r);
		}

	// Can't have a non-aligned, contiguous pool that grows
	TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 200, 10, 0);
	r = inf.SetSizingAttributes(KTestPoolSizeInBufs * 2, 25, 25, 280);
	test_KErrNone(r);
	}

void NegativeTestsKernel()
	{
	test.Next(_L("Negative tests (Kernel)"));
	TInt r;
	r = Ldd.NegativeTestsKernel();
	test_KErrNone(r);
	}

/*
@SYMTestCaseID				23
@SYMTestCaseDesc			Out of memory testing
@SYMREQ
@SYMTestActions
	TBD
@SYMTestExpectedResults
@SYMTestPriority			High
*/

void OutOfMemory()
	{
	test.Next(_L("Out of memory"));
#ifdef _DEBUG

	
	const TInt KMaxKernelAllocations = 1024;
	TInt i, r;
	RShPool pool;
	TShPoolCreateInfo inf0(TShPoolCreateInfo::EPageAlignedBuffer, *PtrBufSize, 1);
	TShPoolCreateInfo inf1(TShPoolCreateInfo::ENonPageAlignedBuffer, *PtrBufSize, 1, 0);
	r = inf0.SetSizingAttributes(4, 100, 1024, 300);
	test_KErrNone(r);
	r = inf1.SetSizingAttributes(4, 100, 1024, 300);
	test_KErrNone(r);
	
	for(TInt j = 0; j <= 1; j++)
		{

		if(j == 0)
			test.Printf(_L("OOM testing for page-aligned pool\n"));
		else
			test.Printf(_L("OOM testing for non-page-aligned pool\n"));

		r = KErrNoMemory;

		__KHEAP_RESET;
		
		//Create the pool
		for (i = 0; i < KMaxKernelAllocations && r == KErrNoMemory; i++)
			{
			__KHEAP_FAILNEXT(i);
			if(j == 0)
				r = pool.Create(inf0,KDefaultPoolHandleFlags);
			else
				r = pool.Create(inf1,KDefaultPoolHandleFlags);
			__KHEAP_RESET;
			}
		test.Printf(_L("Create pool took %d tries\n"),i);
		test_KErrNone(r);

		//Allocate buffers with automatic pool growing enabled
		r = KErrNoMemory;
		RShBuf buf1;
		for (i = 0; i < KMaxKernelAllocations && r == KErrNoMemory; i++)
			{
			__KHEAP_FAILNEXT(i);
			if(j == 0)
				r = buf1.Alloc(pool, EShPoolAllocNoMap);
			else
				r = buf1.Alloc(pool);
			__KHEAP_RESET;
			}
		test.Printf(_L("Allocate shared buffer 1 took %d tries\n"),i);	
		test_KErrNone(r);

		// delay to allow the pool to grow
		User::After(20000);

		r = KErrNoMemory;
		RShBuf buf2;
		for (i = 0; i < KMaxKernelAllocations && r == KErrNoMemory; i++)
			{
			__KHEAP_FAILNEXT(i);
			if(j == 0)
				r = buf2.Alloc(pool, EShPoolAllocNoMap);
			else
				r = buf2.Alloc(pool);
			__KHEAP_RESET;
			User::After(20000);
			}
		test.Printf(_L("Allocate shared buffer 2 took %d tries\n"),i);	
		test_KErrNone(r);

		// delay to allow the pool to grow again
		User::After(20000);

		r = KErrNoMemory;
		RShBuf buf3;
		for (i = 0; i < KMaxKernelAllocations && r == KErrNoMemory; i++)
			{
			__KHEAP_FAILNEXT(i);
			if(j == 0)
				r = buf3.Alloc(pool, EShPoolAllocNoMap);
			else
				r = buf3.Alloc(pool);
			__KHEAP_RESET;
			}
		test.Printf(_L("Allocate shared buffer 3 took %d tries\n"),i);	
		test_KErrNone(r);

		//Map a buffer in page-aligned-pool case
		if(j == 0)
			{
			//Open a one-buffer window
			r = pool.SetBufferWindow(1, ETrue);
			test_KErrNone(r);

			//Map a buffer
			r = KErrNoMemory;
  			for (i = 0; i < KMaxKernelAllocations && r == KErrNoMemory; i++)
				{
				buf1.UnMap();
				__KHEAP_FAILNEXT(i);
				r = buf1.Map();
				__KHEAP_RESET;
				}
			test.Printf(_L("Mapping buffer 1 took %d tries\n"),i);	
			test_KErrNone(r);
			}

		//Setup low-space notification
		TRequestStatus low;
		low = KErrNoMemory;
		for (i = 0; i < KMaxKernelAllocations && low != KRequestPending; i++)
			{
			__KHEAP_FAILNEXT(i);
			pool.RequestLowSpaceNotification(1, low);
			__KHEAP_RESET;
			}
		test.Printf(_L("Setting up low-space notification took %d tries\n"),i);
		test_Equal(low.Int(), KRequestPending);
	
		//Setup free-space notification
		TRequestStatus free;
		free = KErrNoMemory;
		for (i = 0; i < KMaxKernelAllocations && free != KRequestPending; i++)
			{
			__KHEAP_FAILNEXT(i);
			pool.RequestFreeSpaceNotification(4, free);
			__KHEAP_RESET;
			}
		test.Printf(_L("Setting up free-space notification took %d tries\n"),i);
		test_Equal(free.Int(), KRequestPending);
		
		//No allocations should occur here
		__KHEAP_FAILNEXT(1);
		if(j == 0)
			{
			//Unmap the buffer
			r = buf1.UnMap();
			}

		//Cancel the notifications
		pool.CancelLowSpaceNotification(low);
		pool.CancelFreeSpaceNotification(free);
	
		//Close the buffers and the pool
		buf1.Close();
		buf2.Close();
		buf3.Close();
		pool.Close();
		__KHEAP_RESET;

		}

	// Allocate kernel-side buffer on Pool 2
	TInt handle = 0;
	RShBuf kbuf;
	r = KErrNoMemory;
	for (i = 0; i < KMaxKernelAllocations && r == KErrNoMemory; i++)
		{
		__KHEAP_FAILNEXT(i);
		r = Ldd.AllocateKernelBuffer(1, handle);
		__KHEAP_RESET;
		}
	test.Printf(_L("Allocate kernel buffer took %d tries\n"),i);
	test_KErrNone(r);
     
	__KHEAP_FAILNEXT(1);
	kbuf.SetHandle(handle);
	__KHEAP_RESET;

	r = KErrNoMemory;
	for (i = 0; i < KMaxKernelAllocations && r == KErrNoMemory; i++)
		{
        r = kbuf.UnMap();
		__KHEAP_FAILNEXT(i);
		r = kbuf.Map();
		__KHEAP_RESET;
		}
	test.Printf(_L("Mapping kernel buffer took %d tries\n"),i);
	test_KErrNone(r);

	__KHEAP_FAILNEXT(1);
	r = kbuf.UnMap();
	kbuf.Close();
	__KHEAP_RESET;


#else // _DEBUG
	test.Printf(_L("Debug builds only. Test skipped."));
#endif // _DEBUG
	}

/*
@SYMTestCaseID				22
@SYMTestCaseDesc			Stress testing
@SYMREQ
@SYMTestActions
	TBD
@SYMTestExpectedResults
@SYMTestPriority			Medium
*/

TInt StressThread1(TAny*)
	{
	TInt r;
	TInt pagesize;
	r = HAL::Get(HAL::EMemoryPageSize, pagesize);
	test_KErrNone(r);

	TInt i = 0;
	FOREVER
		{
		RShPool pool;
		if (i % 2)
			{
			TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 1000, 512);
			r = pool.Create(inf,KDefaultPoolHandleFlags);
			if (r)
				{
				RDebug::Printf("Error %d line %d", r, __LINE__);
				break;
				}

			r = pool.SetBufferWindow(-1, ETrue);
			test_KErrNone(r);

			}
		else
			{
			TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 10000, 200, 0);
			r = pool.Create(inf,KDefaultPoolHandleFlags);
			if (r)
				{
				RDebug::Printf("Error %d line %d", r, __LINE__);
				break;
				}
			}
		pool.Close();
		i++;
		if (i % 100 == 0)
			{
			RDebug::Printf("ST1 %d iterations", i);
			}
		}
	return r;
	}

TInt StressThread2(TAny*)
	{
	TInt r = KErrUnknown;
	TShPoolInfo inf1;
	TShPoolInfo inf2;
	P1.GetInfo(inf1);
	P2.GetInfo(inf2);
	TInt j = 0;
	FOREVER
		{
		TUint i;
		RArray<RShBuf> bufarray1;
		RArray<RShBuf> bufarray2;
		for (i = 0; i < inf1.iMaxBufs; i++)
			{
			RShBuf buf;
			r = buf.Alloc(P1);
			if (r)
				{
				RDebug::Printf("Error %d line %d i=%d", r, __LINE__, i);
				break;
				}
			TPtr8(buf.Ptr(), buf.Size(),buf.Size()).Fill('1');
			r = bufarray1.Append(buf);
			if (r)
				{
				buf.Close();
				RDebug::Printf("Error %d line %d i=%d", r, __LINE__, i);
				break;
				}
			}
		for (i = 0; i < inf2.iMaxBufs; i++)
			{
			RShBuf buf;
			r = buf.Alloc(P2);
			if (r)
				{
				RDebug::Printf("Error %d line %d i=%d", r, __LINE__, i);
				break;
				}
			TPtr8(buf.Ptr(), buf.Size(),buf.Size()).Fill('2');
			bufarray2.Append(buf);
			}
		i = 0;
		while (bufarray1.Count())
			{
			bufarray1[0].Close();
			bufarray1.Remove(0);
			i++;
			}

		while (bufarray2.Count())
			{
			bufarray2[0].Close();
			bufarray2.Remove(0);
			}
		bufarray1.Close();
		bufarray2.Close();
		if (r)
			{
			break;
			}
		j++;
		if (j % 10 == 0)
			{
			RDebug::Printf("ST2 %d iterations", j);
			}
		}
	return r;
	}

void StressTesting(TInt aSecs)
	{
	test.Next(_L("Stress testing"));
	TInt r;

	test.Start(_L("Create pools"));
	TShPoolCreateInfo inf1(TShPoolCreateInfo::ENonPageAlignedBuffer, 2000, 500, 11);
	r = P1.Create(inf1,KDefaultPoolHandleFlags);
	test_KErrNone(r);
	TInt handle;
	TShPoolCreateInfo inf2(TShPoolCreateInfo::EPageAlignedBuffer, 5000, 150);
	r = Ldd.OpenKernelPool(inf2, handle);
	test_KErrNone(r);
	P2.SetHandle(handle);

	r = P2.SetBufferWindow(-1, ETrue);
	test_KErrNone(r);

	test.Next(_L("Create threads"));
	RThread t1;
	r = t1.Create(_L("THREAD1"), StressThread1, KDefaultStackSize, KMinHeapSize, KMinHeapSize, NULL);
	test_KErrNone(r);
	RThread t2;
	r = t2.Create(_L("THREAD2"), StressThread2, KDefaultStackSize*2, KMinHeapSize, 1 << 20, NULL);
	test_KErrNone(r);
	test.Next(_L("Start threads"));
	test.Printf(_L("Wait for %d seconds\n"), aSecs);
	RThread().SetPriority(EPriorityMore);
	TRequestStatus t1rs;
	TRequestStatus t2rs;
	t1.Logon(t1rs);
	t2.Logon(t2rs);
	t1.Resume();
	t2.Resume();
	User::After(aSecs * 1000000);

	test.Next(_L("Kill threads"));
	t1.Kill(KErrNone);
	t2.Kill(KErrNone);

	// wait for threads to actually die
	User::WaitForRequest(t1rs);
	User::WaitForRequest(t2rs);

	t1.Close();
	t2.Close();
	RThread().SetPriority(EPriorityNormal);

	test.Next(_L("Close pools"));
	P1.Close();
	r = Ldd.CloseKernelPool();
	test_KErrNone(r);
	P2.Close();
	test.End();
	}

/*
@SYMTestCaseID
@SYMTestCaseDesc
@SYMREQ
@SYMTestActions
@SYMTestExpectedResults
@SYMTestPriority
*/

void NoDeallocation()
	{
	test.Next(_L("No deallocation"));
	TInt r;
	TBuf<10> command;
	command.Format(_L("%S %d"), &KTestSlave, ETestSlaveNoDeallocation);
	RProcess p;
	r = p.Create(RProcess().FileName(), command);
	test_KErrNone(r);
	TRequestStatus rs;
	p.Logon(rs);
	p.Resume();
	User::WaitForRequest(rs);

	// wait for memory to be freed
	r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)5000, 0);
	test_KErrNone(r);

	__KHEAP_MARKEND;
	test_KErrNone(rs.Int());
	test_Equal(EExitKill, p.ExitType());
	test_KErrNone(p.ExitReason());
	p.Close();
	}

TInt SlaveNoDeallocation()
	{
	__KHEAP_MARK;
	TInt r;
	RShPool pool;
	TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, *BufferSize, KTestPoolSizeInBufs);
	r = pool.Create(inf,KDefaultPoolHandleFlags);
	test_KErrNone(r);

	pool.SetBufferWindow(-1, ETrue);
	test_KErrNone(r);

	if (!r)
		{
		RShBuf buf;
		r = buf.Alloc(pool);
		}
	return r;
	}

TInt E32Main()
	{
	__UHEAP_MARK;

	// Parse command line for slave processes
	TInt r = KErrArgument;
	TBuf<KMaxFullName> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	if (lex.NextToken() == KTestSlave)
		{
		TInt function;
		TLex functionlex(lex.NextToken());
		functionlex.Val(function);
		switch (function)
			{
			case ETestSlaveNoDeallocation:
				r = SlaveNoDeallocation();
				break;
			}
		__UHEAP_MARKEND;
		return r;
		}
	// Test starts here
	test.Title();

	test.Start(_L("Check for Shared Buffers availability"));
	RShPool pool;
	TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, *BufferSize, KTestPoolSizeInBufs);
	r = pool.Create(inf,KDefaultPoolHandleFlags);
	if (r == KErrNotSupported)
		{
		test.Printf(_L("Not supported by this memory model.\n"));
		}
	else
		{
		test_KErrNone(r);
		pool.Close();

		test.Next(_L("No device driver"));
		test.Start(_L("Start test loop"));
		for (PtrBufSize = BufferSize; *PtrBufSize != 0; PtrBufSize++)
			{
			TBuf<30> title;
			title.Format(_L("Buffer size = %d bytes"), *PtrBufSize);
			test.Next(title);
			test.Start(_L("New test iteration"));
			BufferAlignmentUser();
			BufferMapping();
			BufferWindow();
			GuardPages();
			PoolGrowingUser();
			SingleBufferPool();
			test.End();
			}
		test.End();
		test.Next(_L("Load Device Driver"));
		LoadDeviceDrivers();

		#ifdef TEST_CLIENT_THREAD
		test.Next(_L("Device driver in client thread"));
		r = Ldd.Open(0);
		#else
		test.Next(_L("Device driver in own thread"));
		r = Ldd.Open(1);
		#endif

		test_KErrNone(r);

		test.Start(_L("Start test loop"));
		for (PtrBufSize = BufferSize; *PtrBufSize != 0; PtrBufSize++)
			{
			TBuf<30> title;
			title.Format(_L("Buffer size = %d bytes"), *PtrBufSize);
			test.Next(title);
			test.Start(_L("New test iteration"));
			CreateUserPool(ETestNonPageAligned);
			CreateKernelPool(ETestNonPageAligned);
			AllocateUserBuffer();
			AllocateKernelBuffer();
			AllocateUserMax(P1);
			AllocateUserMax(P2);
			AllocateKernelMax();
			BufferAlignmentKernel();
			CreateKernelPoolPhysAddr();
			NotificationRequests(P1);
			NotificationRequests(P2);
			CancelNotificationRequests(P1);
			CancelNotificationRequests(P2);
			ShBufPin();
			CloseKernelPool();
			CloseUserPool();
			ContiguousPoolKernel();
			CreateUserPool(ETestPageAligned);
			CreateKernelPool(ETestPageAligned);
			OutOfMemory();
			AllocateUserBuffer();
			AllocateKernelBuffer();
			AllocateUserMax(P1);
			AllocateUserMax(P2);
			AllocateKernelMax();
			NotificationRequests(P1);
			NotificationRequests(P2);
			CloseUserPool();
			CloseKernelPool();
			CreateUserPool(ETestPageAlignedGrowing);
			CreateKernelPool(ETestPageAlignedGrowing);
			OutOfMemory();
			AllocateKernelMax();
			AllocateUserMax(P1);
			AllocateUserMax(P2);
			CloseUserPool();
			CloseKernelPool();
			test.End();
			}
		NegativeTestsKernel();
		StressTesting(5);
		test.End();
		Ldd.Close();

		NegativeTestsUser();
		NoDeallocation();

		test.Next(_L("Unload Device Drivers"));
		FreeDeviceDrivers();
		}
	test.End();
	test.Close();

	__UHEAP_MARKEND;
	return KErrNone;
	}
