// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\heap\t_fail.cpp
// Overview:
// Test deterministic, random and fail-next heap failure modes. 
// API Information:
// RHeap.
// Details:
// - Simulate the EFailNext, EDeterministic, ERandom, ETrueRandom, 
// ENone modes of heap allocation failures and the burst variants.
// - Reallocate the size of an existing cell without moving it 
// and check the result is as expected.
// - Check whether heap has been corrupted by all the tests.
// Platforms/Drives/Compatibility:
// All
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <hal.h>
#include <f32file.h>
#include <e32panic.h>
#include <e32def.h>
#include <e32def_private.h>
#include <e32ldr.h>
#include <e32ldr_private.h>
#include "d_kheap.h"

LOCAL_D RTest test(_L("T_FAIL"));
RKHeapDevice KHeapDevice;


#if defined _DEBUG

/**
	Test we fail burst times for EBurstFailNext
	Defined as a macro so that it is easier to determine which test is failing.

@param aCount The number of allocations before it should fail.
@param aBurst The number of allocations that should fail.
*/

#define __UHEAP_TEST_BURST_FAILNEXT(aCount, aBurst) \
	__UHEAP_BURSTFAILNEXT(aCount, aBurst); \
	TEST_BURST_FAILNEXT(__UHEAP_CHECKFAILURE, aCount, aBurst)


#define __RHEAP_TEST_BURST_FAILNEXT(aHeap, aCount, aBurst) \
	__RHEAP_BURSTFAILNEXT(aHeap, aCount, aBurst); \
	TEST_BURST_FAILNEXT(__RHEAP_CHECKFAILURE(aHeap), aCount, aBurst)


#define __KHEAP_TEST_BURST_FAILNEXT(aCount, aBurst) \
	__KHEAP_BURSTFAILNEXT(aCount, aBurst); \
	test_Equal(0, __KHEAP_CHECKFAILURE); \
	test_KErrNone(KHeapDevice.TestBurstFailNext(aCount, aBurst)); \
	test_Equal(aBurst, __KHEAP_CHECKFAILURE)


#define TEST_BURST_FAILNEXT(aCheckFailure, aCount, aBurst) \
	{ \
	test_Equal(0, aCheckFailure); \
	for (i = 0; i < aCount; i++) \
		{ \
		if (i < aCount - 1) \
			{ \
			p = new TInt; \
			test_NotNull(p); \
			delete p; \
			} \
		else \
			{ \
			for (TUint j = 0; j < aBurst; j++) \
				{ \
				p = new TInt; \
				test_Equal(NULL, p); \
				test_Equal(j + 1, aCheckFailure); \
				} \
			} \
		} \
	p = new TInt; \
	test_NotNull(p); \
	delete p; \
	}


/**
	Test we fail burst times for EBurstDeterministic
	Defined as a macro so that it is easier to determine which test is failing.

@param aRate The rate of each set of failures.
@param aBurst The number of allocations that should fail.
*/
#define __UHEAP_TEST_BURST_DETERMINISTIC(aRate, aBurst) \
	__UHEAP_SETBURSTFAIL(RHeap::EBurstDeterministic, aRate, aBurst); \
	TEST_BURST_DETERMINISTIC(__UHEAP_CHECKFAILURE, aRate, aBurst)

#define __RHEAP_TEST_BURST_DETERMINISTIC(aHeap, aRate, aBurst) \
	__RHEAP_SETBURSTFAIL(aHeap, RHeap::EBurstDeterministic, aRate, aBurst); \
	TEST_BURST_DETERMINISTIC(__RHEAP_CHEAKFAILURE(aHeap), aRate, aBurst)

#define __KHEAP_TEST_BURST_DETERMINISTIC(aRate, aBurst) \
	__KHEAP_SETBURSTFAIL(RHeap::EBurstDeterministic, aRate, aBurst); \
	test_Equal(0, __KHEAP_CHECKFAILURE); \
	test_KErrNone(KHeapDevice.TestBurstDeterministic(aRate, aBurst)); \
	test_Equal(aBurst * KHeapFailCycles, __KHEAP_CHECKFAILURE)

#define TEST_BURST_DETERMINISTIC(aCheckFailure, aRate, aBurst) \
	{ \
	test_Equal(0, aCheckFailure); \
	TUint failures = 0; \
	for (i = 1; i <= aRate * KHeapFailCycles; i++) \
		{ \
		if (i % aRate == 0) \
			{ \
			for (TInt j = 0; j < aBurst; j++) \
				{ \
				p = new TInt; \
				test_Equal(NULL, p); \
				test_Equal(++failures, aCheckFailure); \
				} \
			} \
		else \
			{ \
			p = new TInt; \
			test(p!=NULL); \
			delete p; \
			} \
		} \
	}

/**
	Test we fail burst times for EBurstRandom and EBurstTrueRandom.
	Even though it is random it should always fail within aRate allocations.
	Defined as a macro so that it is easier to determine which test is failing.

@param aRate The limiting rate of each set of failures.
@param aBurst The number of allocations that should fail.
	
*/
#define __UHEAP_TEST_BURST_RANDOM(aRate, aBurst) \
	__UHEAP_SETBURSTFAIL(RHeap::EBurstRandom, aRate, aBurst); \
	TEST_BURST_RANDOM(aRate, aBurst)

#define __RHEAP_TEST_BURST_RANDOM(aHeap, aRate, aBurst) \
	__RHEAP_SETBURSTFAIL(aHeap, RHeap::EBurstRandom, aRate, aBurst); \
	TEST_BURST_RANDOM(aRate, aBurst)

#define __UHEAP_TEST_BURST_TRUERANDOM(aRate, aBurst) \
	__UHEAP_SETBURSTFAIL(RHeap::EBurstTrueRandom, aRate, aBurst); \
	TEST_BURST_RANDOM(aRate, aBurst)

#define __RHEAP_TEST_BURST_TRUERANDOM(aHeap, aRate, aBurst) \
	__RHEAP_SETBURSTFAIL(aHeap, RHeap::EBurstTrueRandom, aRate, aBurst); \
	TEST_BURST_RANDOM(aRate, aBurst)


#define TEST_BURST_RANDOM(aRate, aBurst) \
	failed = 0; \
	for (i = 0; i < aRate * KHeapFailCycles; i++) \
		{ \
		p = new TInt; \
		if (p == NULL) \
			{/* we've started failing so check that we fail burst times*/ \
			failed++; \
			for (TInt j = 1; j < aBurst; j++) \
				{ \
				p = new TInt; \
				test_Equal(NULL, p); \
				} \
			} \
		delete p; \
		} \
	test_NotNull(failed);

struct SBurstPanicParams
	{
	TInt iRate;
	TUint iBurst;
	};

TInt TestBurstPanicThread(TAny* aParams)
	{
	SBurstPanicParams* burstParams = (SBurstPanicParams*) aParams;
	__UHEAP_SETBURSTFAIL(RHeap::EBurstDeterministic, burstParams->iRate, burstParams->iBurst); \
	return KErrNone;
	}

#define __UHEAP_TEST_BURST_PANIC(aRate, aBurst) \
	{ \
	RThread thread; \
	TRequestStatus status; \
	SBurstPanicParams threadParams; \
	threadParams.iRate = aRate; \
	threadParams.iBurst = aBurst; \
	test_KErrNone(thread.Create(_L("TestBurstPanicThread"), TestBurstPanicThread, 0x1000, NULL, (TAny*)&threadParams)); \
	thread.Logon(status); \
	thread.Resume(); \
	User::WaitForRequest(status); \
	test_Equal(EExitPanic, thread.ExitType()); \
	test_Equal(ETHeapBadDebugFailParameter, status.Int()); \
	CLOSE_AND_WAIT(thread); \
	}

GLDEF_C TInt E32Main(void)
    {

	test.Title();
	test.Start(_L("Test the heap debug failure mechanisms"));

	// Prepare for __UHEAP tests
	__UHEAP_RESET;
	__UHEAP_MARK;

	// Make sure that we can retrieve the failure type set with __UHEAP_SETFAIL
	test.Next(_L("Set and get user heap failure simulation mode"));
	__UHEAP_SETFAIL(RHeap::EFailNext, 1);
	test(User::__DbgGetAllocFail(EFalse) == RHeap::EFailNext);
	__UHEAP_SETFAIL(RHeap::ENone, 0);

	// Make sure that we can retrieve the failure type set with __KHEAP_SETFAIL
	test.Next(_L("Set and get kernel heap failure simulation mode"));
	__KHEAP_SETFAIL(RHeap::EFailNext, 1);
	test(User::__DbgGetAllocFail(ETrue) == RHeap::EFailNext);
	__KHEAP_SETFAIL(RHeap::ENone, 0);

	// Prepare for __RHEAP tests
	TInt pageSize;
	test_KErrNone(HAL::Get(HAL::EMemoryPageSize, pageSize));
	
	RChunk heapChunk;
	test_KErrNone(heapChunk.CreateLocal(pageSize<<1, pageSize<<1));
	RHeap* rHeap = UserHeap::ChunkHeap(NULL, 0, pageSize);
	test_NotNull(rHeap);
	__RHEAP_RESET(rHeap);
	__RHEAP_MARK(rHeap);


	// Prepare for __KHEAP tests by:
	// Turning off lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	// Loading the kernel heap test driver
	test.Next(_L("Load/open d_kheap test driver"));
	TInt r = User::LoadLogicalDevice(KHeapTestDriverName);
	test( r==KErrNone || r==KErrAlreadyExists);
	if( KErrNone != (r=KHeapDevice.Open()) )	
		{
		User::FreeLogicalDevice(KHeapTestDriverName);
		test.Printf(_L("Could not open LDD"));
		test(0);
		}
	__KHEAP_RESET;
	__KHEAP_MARK;

//=============================================================================
	test.Next(_L("Test __UHEAP EFailNext"));
	TInt *p;
	TInt *q;
	p=new int;
	test(p!=NULL);
	delete p;
	__UHEAP_FAILNEXT(1);
	p=new int;
	test(p==NULL);
	p=new int;
	test(p!=NULL);
	delete p;
	__UHEAP_FAILNEXT(2);
	p=new int;
	q=new int;
	test(p!=NULL);
	test(q==NULL);
	delete p;
	__UHEAP_FAILNEXT(10);
	TUint i;
	for (i=0; i<9; i++)
		{
		p=new int;
		test(p!=NULL);
		delete p;
		}
	p=new int;
	test(p==NULL);
	for (i=0; i<30; i++)
		{
		p=new int;
		test(p!=NULL);
		delete p;
		}

	// Test EFailNext with burst macro should default to burst of 1
	 __UHEAP_SETBURSTFAIL(RAllocator::EFailNext, 5, 5);
	for (i = 0; i < 4; i++)
		{
		p = new TInt;
		test_NotNull(p);
		delete p;
		}
	p = new TInt;
	test_Equal(NULL, p);
	p = new TInt;
	test_NotNull(p);
	delete p;
	

//=============================================================================
	test.Next(_L("Test __UHEAP BurstFailNext"));
	__UHEAP_TEST_BURST_FAILNEXT(2, 1);
	__UHEAP_TEST_BURST_FAILNEXT(10, 12);
	__UHEAP_TEST_BURST_FAILNEXT(5, 50);
	__UHEAP_TEST_BURST_FAILNEXT(50, 5);
	
	// test using burst with non-burst macro should default to burst=1
	__UHEAP_SETFAIL(RHeap::EBurstFailNext, 5);
	for (i = 0; i < 4; i++)
		{
		p = new TInt;
		test_NotNull(p);
		delete p;
		}
	q = new TInt;
	test_Equal(NULL, q);
	q = new TInt;
	test_NotNull(q);
	delete q;


	test.Next(_L("Test __RHEAP BurstFailNext"));
	RHeap* origHeap = User::SwitchHeap(rHeap);

	__RHEAP_TEST_BURST_FAILNEXT(rHeap, 2, 1);
	__RHEAP_TEST_BURST_FAILNEXT(rHeap, 10, 12);
	__RHEAP_TEST_BURST_FAILNEXT(rHeap, 5, 50);
	__RHEAP_TEST_BURST_FAILNEXT(rHeap, 50, 5);

	User::SwitchHeap(origHeap);
		
	test.Next(_L("Test __KHEAP BurstFailNext"));
	__KHEAP_TEST_BURST_FAILNEXT(1, 1);
	__KHEAP_TEST_BURST_FAILNEXT(10, 12);
	__KHEAP_TEST_BURST_FAILNEXT(5, 50);
	__KHEAP_TEST_BURST_FAILNEXT(50, 5);
	__KHEAP_RESET;
	
		
//=============================================================================
	test.Next(_L("Test __UHEAP EDeterministic"));
	__UHEAP_SETFAIL(RHeap::EDeterministic, 1);
	for (i=0; i<20; i++)
		{
		p=new int;
		test(p==NULL);
		}
	__UHEAP_SETFAIL(RHeap::EDeterministic, 2);
	for (i=0; i<20; i++)
		{
		p=new int;
		q=new int;
		test(p!=NULL);
		test(q==NULL);
		delete p;
		}
	__UHEAP_SETFAIL(RHeap::EDeterministic, 11);
	for (i=1; i<=100; i++)
		{
		p=new int;
		if (i%11==0)
			test(p==NULL);
		else
			test(p!=NULL);
		delete p;
		}
	// Test using burst macro for non-burst fail type
	// The burst value will be ignored.
	__UHEAP_SETBURSTFAIL(RHeap::EDeterministic, 2, 3);
	for (i=0; i<20; i++)
		{
		p=new int;
		q=new int;
		test(p!=NULL);
		test(q==NULL);
		delete p;
		}

//=============================================================================
	test.Next(_L("Test __UHEAP EBurstDeterministic"));
	__UHEAP_TEST_BURST_DETERMINISTIC(1, 1);
	__UHEAP_TEST_BURST_DETERMINISTIC(2, 1);
	__UHEAP_TEST_BURST_DETERMINISTIC(11, 2);
	__UHEAP_TEST_BURST_DETERMINISTIC(2, 3);	// Test with burst > rate.

	// Test setting EBurstDeterministic with non-burst MACRO
	// it should still work but default to a burst rate of 1
	__UHEAP_SETFAIL(RHeap::EBurstDeterministic, 2);
	for (i=0; i<20; i++)
		{
		p = new int;
		q = new int;
		test_NotNull(p);
		test_Equal(NULL, q);
		delete p;
		}

	test.Next(_L("Test __RHEAP EBurstDeterministic"));
	origHeap = User::SwitchHeap(rHeap);

	__UHEAP_TEST_BURST_DETERMINISTIC(1, 1);
	__UHEAP_TEST_BURST_DETERMINISTIC(2, 1);
	__UHEAP_TEST_BURST_DETERMINISTIC(11, 2);
	__UHEAP_TEST_BURST_DETERMINISTIC(2, 3);	// Test with burst > rate.

	User::SwitchHeap(origHeap);


	test.Next(_L("Test __KHEAP EBurstDeterministic"));
	__KHEAP_TEST_BURST_DETERMINISTIC(1, 1);
	__KHEAP_TEST_BURST_DETERMINISTIC(2, 1);
	__KHEAP_TEST_BURST_DETERMINISTIC(11, 2);
	__KHEAP_TEST_BURST_DETERMINISTIC(2, 3);	// Test with burst > rate.
	__KHEAP_RESET;

//=============================================================================
	test.Next(_L("Test __UHEAP ERandom"));
	__UHEAP_SETFAIL(RHeap::ERandom, 1);
	for (i=1; i<=100; i++)
		{
		p=new int;
		test(p==NULL);
		}
	__UHEAP_SETFAIL(RHeap::ERandom, 2);
	for (i=1; i<=100; i++)
		{
		p=new int;
		q=new int;
		test(p==NULL || q==NULL);
		delete p;
		delete q;
		}
	__UHEAP_SETFAIL(RHeap::ERandom, 10);
	TInt failed=0;
	for (i=0; i<10; i++)
		{
		p=new int;
		if (p==NULL) failed++;
		delete p;
		}
	test(failed);
	for (i=0; i<10; i++)
		{
		p=new int;
		if (p==NULL) failed++;
		delete p;
		}
	test(failed>=2);

	// Test using the burst macro for ERandom.
	// Can't really check that it only fails once as being random
	// it may fail again immediately after a previous failure.
	__UHEAP_SETBURSTFAIL(RHeap::ERandom, 10, 5);
	TEST_BURST_RANDOM(10, 1);

//=============================================================================
	test.Next(_L("Test __UHEAP EBurstRandom"));
	__UHEAP_TEST_BURST_RANDOM(10, 2);
	__UHEAP_TEST_BURST_RANDOM(15, 5);
	__UHEAP_TEST_BURST_RANDOM(10, 20);

	// Test using EBurstRandom with non-burst macro.
	// Can't really check that it only fails once as being random
	// it may fail again immediately after a previous failure.
	__UHEAP_SETFAIL(RHeap::EBurstRandom, 10);
	__UHEAP_TEST_BURST_RANDOM(10, 1); 


	test.Next(_L("Test __RHEAP EBurstRandom"));
	origHeap = User::SwitchHeap(rHeap);

	__RHEAP_TEST_BURST_RANDOM(rHeap, 10, 2);
	__RHEAP_TEST_BURST_RANDOM(rHeap, 15, 5);
	__RHEAP_TEST_BURST_RANDOM(rHeap, 10, 20);

	User::SwitchHeap(origHeap);
	
	// No random modes for kernel heap

//=============================================================================
	test.Next(_L("Test __UHEAP ETrueRandom"));
	__UHEAP_SETFAIL(RHeap::ETrueRandom, 10);
	failed=0;
	for (i=0; i<10; i++)
		{
		p=new int;
		if (p==NULL) failed++;
		delete p;
		}
	test(failed);
	for (i=0; i<10; i++)
		{
		p=new int;
		if (p==NULL) failed++;
		delete p;
		}
	test(failed>=2);

	// Test using ETrueRandom with burst macro.
	// Can't really check that it only fails once as being random
	// it may fail again immediately after a previous failure.
	__UHEAP_SETBURSTFAIL(RHeap::ETrueRandom, 10, 2);
	TEST_BURST_RANDOM(10, 1);

//=============================================================================
	test.Next(_L("Test __UHEAP EBurstTrueRandom"));
	__UHEAP_TEST_BURST_TRUERANDOM(10, 2);
	__UHEAP_TEST_BURST_TRUERANDOM(15, 5);
	__UHEAP_TEST_BURST_TRUERANDOM(10, 20);

	// Test using EBurstRandom with non-burst macro.
	// Can't really check that it only fails once as being random
	// it may fail again immediately after a previous failure.
	__UHEAP_SETFAIL(RHeap::EBurstTrueRandom, 10);
	TEST_BURST_RANDOM(10, 1);

	test.Next(_L("Test __RHEAP EBurstTrueRandom"));
	origHeap = User::SwitchHeap(rHeap);

	__RHEAP_TEST_BURST_TRUERANDOM(rHeap, 10, 2);
	__RHEAP_TEST_BURST_TRUERANDOM(rHeap, 15, 5);
	__RHEAP_TEST_BURST_TRUERANDOM(rHeap, 10, 20);

	User::SwitchHeap(origHeap);

	// No random modes for kernel heap

//=============================================================================
	test.Next(_L("Test __UHEAP ENone"));
	__UHEAP_SETFAIL(RHeap::ENone, 0);
	for (i=0; i<100; i++)
		{
		p=new int;
		test(p!=NULL);
		delete p;
		}

	// Test using ENone with burst macro.
	__UHEAP_SETBURSTFAIL(RHeap::ENone, 0, 0);
	for (i=0; i<100; i++)
		{
		p = new TInt;
		test_NotNull(p);
		delete p;
		}

//=============================================================================
	test.Next(_L("Test __UHEAP Reset"));
	__UHEAP_SETFAIL(RHeap::EDeterministic, 1);
	for (i=0; i<10; i++)
		{
		p=new int;
		test(p==NULL);
		}
	__UHEAP_RESET;
	p=new int;
	test(p!=NULL);
	delete p;


	// Test using EReset with non-burst macro.
	__UHEAP_SETFAIL(RHeap::EDeterministic, 1);
	for (i=0; i<10; i++)
		{
		p=new int;
		test(p==NULL);
		}
	__UHEAP_SETFAIL(RHeap::EReset, 1);
	p=new int;
	test(p!=NULL);
	delete p;

	// Test using EReset with burst macro.
	__UHEAP_SETFAIL(RHeap::EDeterministic, 1);
	for (i=0; i<10; i++)
		{
		p=new int;
		test(p==NULL);
		}
	__UHEAP_SETBURSTFAIL(RHeap::EReset, 1, 1);
	p=new int;
	test(p!=NULL);
	delete p;

//=============================================================================
	test.Next(_L("Test ETHeapBadDebugFailParameter panics"));
	__UHEAP_TEST_BURST_PANIC(50, KMaxTUint16 + 1);
	__UHEAP_TEST_BURST_PANIC(KMaxTUint16 + 1, 2);
	__UHEAP_TEST_BURST_PANIC(-50, 3);

	// Test maximum aRate and aBurst values don't panic.
	__UHEAP_TEST_BURST_FAILNEXT(2, KMaxTUint16);	// Use failnext as quicker
	__UHEAP_TEST_BURST_FAILNEXT(KMaxTUint16, 2);

//=============================================================================
	test.Next(_L("Test __UHEAP User::ReAlloc without cell moving"));
	TAny* a = User::Alloc(256);
	test(a!=NULL);
	__UHEAP_FAILNEXT(1);
	TAny* a2 = User::ReAlloc(a,192);
	test(a2==a);
	a2 = User::ReAlloc(a,128);
	test(a2==a);
	a2 = User::ReAlloc(a,256);
	test(a2==NULL);
	a2 = User::ReAlloc(a,256);
	test(a2==a);
	User::Free(a);

//=============================================================================
	// Clean up
	__RHEAP_MARKEND(rHeap);
	rHeap->Close();

	__KHEAP_MARKEND;
	// Ensure all kernel heap debug failures are not active for future tests etc.
	__KHEAP_RESET;	
	KHeapDevice.Close();
	User::FreeLogicalDevice(KHeapTestDriverName);	

	__UHEAP_MARKEND;
	test.End();
	return(KErrNone);
    }

#else
GLDEF_C TInt E32Main()
//
// __KHEAP_SETFAIL etc. not available in release mode, so don't test
//
	{	 

	test.Title();
	test.Start(_L("No tests in release mode"));
	test.End();
	return(KErrNone);
	}
#endif

