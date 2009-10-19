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
// e32test\dll\t_tls.cpp
// Overview:
// Test and benchmark TLS (Thread Local Storage) functions
// API Information:
// UserSvr
// Details:
// - Use the UserSvr methods: DllTls, DllSetTls and DllFreeTls to test 
// the thread local storage. Verify the expected results. Verify that 
// the heap was not corrupted by any of the tests.
// - In a separate thread, use the UserSvr methods: DllTls, DllSetTls and 
// DllFreeTls to test the thread local storage. Verify the expected results.
// - Test the TLS cleanup handlers and verify results are as expected. Verify 
// that the heap was not corrupted by any of the tests.
// - Benchmark how many DllSetTls, DllTls and DllFreeTls operations can be
// performed in 1/2 second.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// Tests and benchmarks Tls functions
// 14/10/96 Modified scheme on Arm release
// SetTls - 352	~1300
// Tls - 3510 ~1600
// 15/10/96 Array scheme on Arm release
// Set Tls - ~1900
// Tls - ~2550
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32svr.h>
#include <f32dbg.h>
#include <e32def.h>
#include <e32def_private.h>
#include "u32std.h"

// Can't tell whether allocations will be on the kernel or user heap, so perform all heap check
// operations on both
#define HEAP_MARK			__UHEAP_MARK; __KHEAP_MARK
#define HEAP_MARKEND		__UHEAP_MARKEND; __KHEAP_MARKEND
#define HEAP_RESET			__UHEAP_RESET; __KHEAP_RESET
#define HEAP_CHECK(x)		__UHEAP_CHECK(x); __KHEAP_CHECK(x)
#define HEAP_FAILNEXT(x)	__UHEAP_FAILNEXT(x); __KHEAP_FAILNEXT(x)

TInt const KCheckHandle=67338721;
TUint8* const KCheckValue=(TUint8*)8525124;

LOCAL_D RTest test(_L("T_TLS"));

_LIT(KTestDllName, "t_tlsdll");
const TInt KTestDllOrdSet	= 1;
const TInt KTestDllOrdGet	= 2;
const TInt KTestDllOrdFree	= 3;

typedef TInt (*TTestDllSetFn)(TAny*);
typedef TAny* (*TTestDllGetFn)();
typedef void (*TTestDllFreeFn)();

TInt TestDllSetTls(RLibrary aLib, TAny* aValue)
	{
	TTestDllSetFn f = (TTestDllSetFn)aLib.Lookup(KTestDllOrdSet);
	return (*f)(aValue);
	}

TAny* TestDllGetTls(RLibrary aLib)
	{
	TTestDllGetFn f = (TTestDllGetFn)aLib.Lookup(KTestDllOrdGet);
	return (*f)();
	}

void TestDllFreeTls(RLibrary aLib)
	{
	TTestDllFreeFn f = (TTestDllFreeFn)aLib.Lookup(KTestDllOrdFree);
	(*f)();
	}

#define DISPLAY_PROGRESS	test.Printf(_L("Line %d\n"), __LINE__)

void Test()
	{
	test.Start(_L("Stuff without Setting"));

	HEAP_MARK;

	test(UserSvr::DllTls(KCheckHandle)==NULL);
	UserSvr::DllFreeTls(KCheckHandle);
	test(UserSvr::DllTls(KCheckHandle)==NULL);
	test(UserSvr::DllTls(0)==NULL);
	UserSvr::DllFreeTls(0);
	test(UserSvr::DllTls(0)==NULL);

	HEAP_CHECK(0);

	test.Next(_L("Set"));
	test(UserSvr::DllSetTls(KCheckHandle,KCheckValue)==KErrNone);
	test.Next(_L("Get"));
	test(UserSvr::DllTls(KCheckHandle)==KCheckValue);
	test.Next(_L("Free"));
	UserSvr::DllFreeTls(KCheckHandle);
	test(UserSvr::DllTls(KCheckHandle)==NULL);

	HEAP_CHECK(0);

	test.Next(_L("Set lots"));
	TInt i=0;
	for(;i<1000;i+=3)
		{
		test(UserSvr::DllSetTls(KCheckHandle+i,KCheckValue-i)==KErrNone);
		test(UserSvr::DllTls(KCheckHandle+i)==KCheckValue-i);
		}
	for(i=999;i>0;i-=3)
		{
		test(UserSvr::DllTls(KCheckHandle+i)==KCheckValue-i);
		UserSvr::DllFreeTls(KCheckHandle+i);
		test(UserSvr::DllTls(KCheckHandle+i)==NULL);
		}
	test(UserSvr::DllTls(KCheckHandle+i)==KCheckValue-i);
	UserSvr::DllFreeTls(KCheckHandle+i);
	test(UserSvr::DllTls(KCheckHandle+i)==NULL);

	HEAP_MARKEND;


	test.Next(_L("Test with DLL ID"));

	HEAP_MARK;

	test(UserSvr::DllTls(KCheckHandle, 1)==0);
	test(UserSvr::DllTls(KCheckHandle, 2)==0);
	DISPLAY_PROGRESS;
#ifdef _DEBUG
	HEAP_FAILNEXT(1);
	DISPLAY_PROGRESS;
	test(UserSvr::DllSetTls(KCheckHandle, 1, KCheckValue)==KErrNoMemory);
#endif
	test(UserSvr::DllSetTls(KCheckHandle, 1, KCheckValue)==KErrNone);
	test(UserSvr::DllTls(KCheckHandle, 1)==KCheckValue);
	test(UserSvr::DllTls(KCheckHandle, 2)==0);
	DISPLAY_PROGRESS;
	HEAP_FAILNEXT(1);
	DISPLAY_PROGRESS;
	test(UserSvr::DllSetTls(KCheckHandle, 3, KCheckValue)==KErrNone);

#ifdef _DEBUG
	RSemaphore s;
	test(s.CreateLocal(0)==KErrNoMemory);
#endif

	HEAP_RESET;

	test(UserSvr::DllTls(KCheckHandle, 1)==0);
	test(UserSvr::DllTls(KCheckHandle, 2)==0);
	test(UserSvr::DllTls(KCheckHandle, 3)==KCheckValue);
	UserSvr::DllFreeTls(KCheckHandle);

	DISPLAY_PROGRESS;
	HEAP_MARKEND;
	DISPLAY_PROGRESS;

	test.Next(_L("Test reloading DLL"));

	RLibrary l;
	TInt r = l.Load(KTestDllName);
	test(r==KErrNone);
	for (i=0; i<2; ++i)
		{
		test.Printf(_L("i=%d\n"),i);
		test(TestDllGetTls(l)==0);
		test(TestDllSetTls(l, KCheckValue)==KErrNone);
		test(TestDllGetTls(l)==KCheckValue);
		if (i==0)
			{
			TestDllFreeTls(l);
			test(TestDllGetTls(l)==0);
			}
		l.Close();
		r = l.Load(KTestDllName);
		test(r==KErrNone);
		test(TestDllGetTls(l)==0);
		TestDllFreeTls(l);
		}
	l.Close();

	test.End();
	}

TInt TestThread(TAny*)
	{
	RTest test(_L("T_TLS Thread"));
	test.Start(_L("Stuff without Setting"));
	test(UserSvr::DllTls(KCheckHandle)==NULL);
	UserSvr::DllFreeTls(KCheckHandle);
	test(UserSvr::DllTls(KCheckHandle)==NULL);
	test(UserSvr::DllTls(0)==NULL);
	UserSvr::DllFreeTls(0);
	test(UserSvr::DllTls(0)==NULL);
	test.Next(_L("Set"));
	test(UserSvr::DllSetTls(KCheckHandle,KCheckValue)==KErrNone);
	test.Next(_L("Get"));
	test(UserSvr::DllTls(KCheckHandle)==KCheckValue);
	test.Next(_L("Free"));
	UserSvr::DllFreeTls(KCheckHandle);
	test(UserSvr::DllTls(KCheckHandle)==NULL);
	test.Next(_L("Set lots"));
	TInt i=0;
	for(;i<1000;i+=3)
		{
		test(UserSvr::DllSetTls(KCheckHandle+i,KCheckValue-i)==KErrNone);
		test(UserSvr::DllTls(KCheckHandle+i)==KCheckValue-i);
		}
	for(i=999;i>=0;i-=3)
		{
		test(UserSvr::DllTls(KCheckHandle+i)==KCheckValue-i);
		UserSvr::DllFreeTls(KCheckHandle+i);
		test(UserSvr::DllTls(KCheckHandle+i)==NULL);
		}

	test.Close();
	return KErrNone;
	}

TInt HeapSize(RHeap* aHeap = NULL)
	{
	if (!aHeap)
		aHeap = &User::Heap();
	TInt size;
	aHeap->AllocSize(size);
	return size;
	}

TBool TLSStoredOnUserHeap()
	{
	TInt initCount = HeapSize();
	TInt i;
	const TInt KMax = 2;
	for(i = 0 ; i < KMax ; ++i)
		test(UserSvr::DllSetTls(KCheckHandle+i,KCheckValue-i) == KErrNone);
	TInt finalCount = HeapSize();
	for(i = 0 ; i < KMax ; ++i)
		UserSvr::DllFreeTls(KCheckHandle+i);
	return initCount != finalCount;
	}

class RDummyAllocator : public RAllocator
	{
public:
	TInt AccessCount() { return iAccessCount; }
	};

RHeap* MainHeap = NULL;

TInt HeapAccessCount(TAny* aHeap)
	{
	RDummyAllocator* heap = (RDummyAllocator*)aHeap;
	return heap->AccessCount();
	}

TBool HeapExists(RHeap* aHeap)
	{
	RThread thread;
	test_KErrNone(thread.Create(_L("HeapExistsThread"), HeapAccessCount, 0x1000, MainHeap, (TAny*)aHeap));
	TRequestStatus status;
	thread.Logon(status);
	thread.Resume();
	User::WaitForRequest(status);

	TInt r;
	if (thread.ExitType() == EExitKill)
		{
		r = thread.ExitReason();
		test_NotNegative(r);
		}
	else
		{
		test_Equal(EExitPanic, thread.ExitType());
		test_Equal(3, thread.ExitReason());
		r = 0;
		}
	thread.Close();
	
	return r != 0;
	}

TInt TestUserSideTls1Thread(TAny*)
	{
	// Ensure TLS uses initial heap
	if (UserSvr::DllSetTls(KCheckHandle,KCheckValue) != KErrNone)
		return __LINE__;
	UserSvr::DllFreeTls(KCheckHandle);

	RHeap* tlsHeap = &User::Heap();
	TInt tlsInitSize = HeapSize(tlsHeap);

	// Switch heap
	RHeap* newHeap = User::ChunkHeap(NULL, 0x1000, 0x1000);
	if (!newHeap)
		return __LINE__;
	TInt newInitSize = HeapSize(newHeap);
	User::SwitchHeap(newHeap);
	tlsHeap->Close();

	// Allocate more TLS data
	for(TInt i = 0 ; i < 100 ; ++i)
		{
		if (UserSvr::DllSetTls(KCheckHandle+i,KCheckValue-i) != KErrNone)
			return __LINE__;
		}

	// Test TLS data was allocated on original heap
	if (!(HeapSize(tlsHeap) > tlsInitSize))
		return __LINE__;
	if (HeapSize(newHeap) != newInitSize)
		return __LINE__;
	
	return KErrNone;
	}

void TestUserSideTls1()
	{
	test.Next(_L("Test user-side TLS behaviour when switching heaps"));

	RThread thread;
	test_KErrNone(thread.Create(_L("TestUserSideTls1Thread"), TestUserSideTls1Thread, 0x1000, 0x1000, 0x1000, 0));
	
	TRequestStatus status;
	thread.Logon(status);
	thread.Resume();
	User::WaitForRequest(status);
	
	test_Equal(EExitKill, thread.ExitType());
	test_Equal(KErrNone, thread.ExitReason());
	thread.Close();
	}

TInt TestUserSideTls2Thread(TAny*)
	{
	// Allocate some TLS data
	for(TInt i = 0 ; i < 100 ; ++i)
		{
		if (UserSvr::DllSetTls(KCheckHandle+i,KCheckValue-i) != KErrNone)
			return __LINE__;
		}

	// Exit normally
	return KErrNone;
	}

void TestUserSideTls2()
	{
	test.Next(_L("Test user-side TLS data cleanup on thread exit"));

	RHeap* tlsHeap = User::ChunkHeap(NULL, 0x1000, 0x1000);
	test_NotNull(tlsHeap);
	TInt initSize = HeapSize(tlsHeap);

	RThread thread;
	test_KErrNone(thread.Create(_L("TestUserSideTls2Thread"), TestUserSideTls2Thread, 0x1000, tlsHeap, 0));
	TThreadId id = thread.Id();
	
	TRequestStatus status;
	thread.Logon(status);
	thread.Resume();
	User::WaitForRequest(status);
	
	test_Equal(EExitKill, thread.ExitType());
	test_Equal(KErrNone, thread.ExitReason());
	thread.Close();

	// Check TLS data freed
	test_Equal(initSize, HeapSize(tlsHeap));	
	tlsHeap->Close();

	// Check heap no longer exists
	test(!HeapExists(tlsHeap));

	// Check thread no longer exists
	RThread thread2;
	test_Equal(KErrNotFound, thread2.Open(id));
	}

TInt TestUserSideTls3Thread(TAny*)
	{
	// Allocate some TLS data
	for(TInt i = 0 ; i < 100 ; ++i)
		{
		if (UserSvr::DllSetTls(KCheckHandle+i,KCheckValue-i) != KErrNone)
			return __LINE__;
		}

	// Panic
	User::Panic(_L("Test"), 999);
	return KErrNone;
	}

void TestUserSideTls3()
	{
	test.Next(_L("Test user-side TLS data cleanup on thread panic"));

	RHeap* tlsHeap = User::ChunkHeap(NULL, 0x1000, 0x1000);
	test_NotNull(tlsHeap);

	RThread thread;
	test_KErrNone(thread.Create(_L("TestUserSideTls3Thread"), TestUserSideTls3Thread, 0x1000, tlsHeap, 0));
	TThreadId id = thread.Id();
	
	TRequestStatus status;
	thread.Logon(status);
	thread.Resume();
	User::WaitForRequest(status);
	
	test_Equal(EExitPanic, thread.ExitType());
	test_Equal(999, thread.ExitReason());
	
	thread.Close();
	tlsHeap->Close();
	
	// Check heap no longer exists
	test(!HeapExists(tlsHeap));

	// Check thread no longer exists
	RThread thread2;
	test_Equal(KErrNotFound, thread2.Open(id));
	}

TInt TestUserSideTls4Thread(TAny*)
	{
	// Shouldn't ever run
	return KErrGeneral;
	}

void TestUserSideTls4()
	{
	test.Next(_L("Test user-side TLS data cleanup on early thread kill"));
	
	RHeap* tlsHeap = User::ChunkHeap(NULL, 0x1000, 0x1000);
	test_NotNull(tlsHeap);

	RThread thread;
	test_KErrNone(thread.Create(_L("TestUserSideTls4Thread"), TestUserSideTls4Thread, 0x1000, tlsHeap, 0));
	TThreadId id = thread.Id();
	
	thread.Kill(101);

	TRequestStatus status;
	thread.Logon(status);
	User::WaitForRequest(status);
	
	test_Equal(EExitKill, thread.ExitType());
	test_Equal(101, thread.ExitReason());
	
	thread.Close();
	tlsHeap->Close();
	
	// Check heap no longer exists
	test(!HeapExists(tlsHeap));

	// Check thread no longer exists
	RThread thread2;
	test_Equal(KErrNotFound, thread2.Open(id));
	}

#if 0
class CTest : public CBase
	{
public:
	static CTest* New(TInt aSize, TInt aTls, TInt aLinkedTls);
	virtual ~CTest();
public:
	TAny* iAlloc;
	TInt iTls;
	TInt iLinkedTls;
	};

void TlsCleanup2(TAny* a)
	{
	delete ((CTest*)a);
	}

CTest::~CTest()
	{
	RDebug::Print(_L("~CTest %d %d"), iTls, iLinkedTls);
	User::Free(iAlloc);
	UserSvr::DllFreeTls(iTls);
	if (iLinkedTls)
		{
		CTest* p = (CTest*)UserSvr::DllTls(iLinkedTls);
		if (p)
			delete p;
		}
	}

CTest* CTest::New(TInt aSize, TInt aTls, TInt aLinkedTls)
	{
	CTest* p = new CTest;
	test(p!=NULL);
	p->iTls = aTls;
	p->iAlloc = User::Alloc(aSize);
	test(p->iAlloc!=NULL);
	p->iLinkedTls = aLinkedTls;
	test(UserSvr::DllSetTls(aTls, p, TlsCleanup2)==KErrNone);
	return p;
	}


void TlsCleanupCheck(TAny*)
	{
	__UHEAP_MARKEND;
	}

void TlsCleanup1(TAny* a)
	{
	User::Free(a);
	}

TInt TlsCleanupThread1(TAny* a)
	{
	__UHEAP_MARK;
	TInt n = (TInt)a;
	TInt i;
	TInt r = UserSvr::DllSetTls(0, NULL, TlsCleanupCheck);
	if (r!=KErrNone)
		return r;
	for (i=0; i<n; ++i)
		{
		TAny* p = User::Alloc(64);
		if (!p)
			return KErrNoMemory;
		r = UserSvr::DllSetTls(i+1, p, TlsCleanup1);
		if (r!=KErrNone)
			return r;
		}
	return KErrNone;
	}

TInt TlsCleanupThread2(TAny*)
	{
	__UHEAP_MARK;
	TInt r = UserSvr::DllSetTls(0, NULL, TlsCleanupCheck);
	if (r!=KErrNone)
		return r;

	CTest::New(256, 1, 2);
	CTest::New(256, 2, 3);
	CTest::New(256, 3, 0);
	CTest::New(256, 6, 5);
	CTest::New(256, 5, 4);
	CTest::New(256, 4, 0);
	CTest::New(256, 9, 8);
	CTest::New(256, 8, 7);
	CTest::New(256, 7, 9);
	CTest::New(256, 10, 11);
	CTest* p = CTest::New(256, 11, 12);
	CTest::New(256, 12, 0);

	delete p;

	return KErrNone;
	}

void DisplayExitInfo(RThread t)
	{
	TInt exitType = t.ExitType();
	TInt exitReason = t.ExitReason();
	TBuf<32> exitCat = t.ExitCategory();
	test.Printf(_L("Exit Info: %d,%d,%S\n"), exitType, exitReason, &exitCat);
	}

void DoTestCleanupHandler(TThreadFunction aFunc, TAny* aArg)
	{
	RThread t;
	TInt r = t.Create(KNullDesC, aFunc, 0x1000, NULL, aArg);
	test(r==KErrNone);
	TRequestStatus s;
	t.Logon(s);
	t.Resume();
	User::WaitForRequest(s);
	DisplayExitInfo(t);
	test(t.ExitType()==EExitKill);
	test(t.ExitReason()==KErrNone);
	test(s==KErrNone);
	CLOSE_AND_WAIT(t);
	}

void TestCleanupHandler()
	{
	HEAP_MARK;

	test.Start(_L("Test TLS cleanup handlers"));
	DoTestCleanupHandler(TlsCleanupThread1, (TAny*)1 );
	DoTestCleanupHandler(TlsCleanupThread1, (TAny*)2 );
	DoTestCleanupHandler(TlsCleanupThread1, (TAny*)3 );
	DoTestCleanupHandler(TlsCleanupThread2, NULL );

	test.End();

	HEAP_MARKEND;
	}
#endif

void Benchmark()
	{
	const TInt KMaxEntries=(512*1024)/sizeof(STls); // limits TLS entries to 512K of storage
	const TInt KMaxTime=500000; // limits time to half a second
	
	HEAP_MARK;

	test.Start(_L("SetTls()"));//Note: much slower if done in reverse order
	TInt count=0;
	TTime start;
	TTime finish;
	TTime now;
	start.HomeTime();
	finish.HomeTime();
	finish+=TTimeIntervalMicroSeconds(KMaxTime);
	while (now.HomeTime(),now<finish && count<KMaxEntries)
		{
		test(UserSvr::DllSetTls(count,KCheckValue)==KErrNone);
		count++;
		}
	TTimeIntervalMicroSeconds elapsed=now.MicroSecondsFrom(start);
	TInt scount=(TInt)((count*500000.0)/elapsed.Int64()); // scale count up to 1/2 second
	test.Printf(_L("     Done %d in 1/2 sec\n"),scount);
	if (count==KMaxEntries)
		test.Printf(_L("     (%d in %f sec)\n"),count,elapsed.Int64()/1000000.0);

	TInt max=count;

	test.Next(_L("Tls()"));
	TInt i=0;
	count=0;
	finish.HomeTime();
	finish+=TTimeIntervalMicroSeconds(KMaxTime);
	while (now.HomeTime(),now<finish)
		{
		test(UserSvr::DllTls(i)==KCheckValue);
		count++;
		if (++i==max)
			i=0;
		}
	elapsed=now.MicroSecondsFrom(start);
	scount=(TInt)((count*500000.0)/elapsed.Int64()); // scale count up to 1/2 second
	test.Printf(_L("     Done %d in 1/2 sec\n"),scount);

	test.Next(_L("FreeTls()"));//Note: much faster if done in reverse order
	count=0;
	start.HomeTime();
	finish.HomeTime();
	finish+=TTimeIntervalMicroSeconds(KMaxTime);
	while (now.HomeTime(),now<finish)
		{
		UserSvr::DllFreeTls(count++);
		if (count==max)
			break;
		}
	elapsed=now.MicroSecondsFrom(start);
	scount=(TInt)((count*500000.0)/elapsed.Int64()); // scale count up to 1/2 second
	test.Printf(_L("     Done %d in 1/2 sec\n"),scount);
	if (count==max)
		test.Printf(_L("     (%d in %f sec)\n"),count,elapsed.Int64()/1000000.0);
	
	while (count<max)
		UserSvr::DllFreeTls(--max);

	HEAP_MARKEND;
	test.End();
	}

TInt E32Main()
	{

	test.Title();
	test.Start(_L("Test"));

	// Turn off lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	Test();
	test.Next(_L("Thread Test"));

	HEAP_MARK;

	RThread thread;
	TInt r=thread.Create(_L("TLS test thread"),TestThread,KDefaultStackSize,0x1000,0x8000,NULL);
	test(r==KErrNone);
	TRequestStatus stat;
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	test(thread.ExitCategory()==_L("Kill"));
	test(thread.ExitType()==EExitKill); 
	test(thread.ExitReason()==KErrNone); 
	CLOSE_AND_WAIT(thread);
	
	HEAP_MARKEND;

	if (TLSStoredOnUserHeap())
		{
		MainHeap = &User::Heap();
		TestUserSideTls1();
		TestUserSideTls2();
		TestUserSideTls3();
		TestUserSideTls4();		
		}
	
	User::After(100);
	
	// On serial screen systems, WSERV may not yet have completed the disconnect
	// from the second thread. This causes the kernel heap check to fail.
	// So we wait here until WSERV can run.
	test.Printf(_L("Wait for WSERV\n"));

//	TestCleanupHandler();

	test.Next(_L("Benchmark"));
	Benchmark();
	test.End();
	return KErrNone;
	}
