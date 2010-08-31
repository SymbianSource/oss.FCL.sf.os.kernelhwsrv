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
// e32test\heap\t_heapdb.cpp
// Tests the _DEBUG build dependent aspects of RHeap
// Overview:
// Tests debug build dependent aspects of RHeap.  
// API Information:
// RHeap.
// Details:
// Test1:
// - Allocate a variety of user heap objects and verify the nesting level, allocation count, 
// allocation level and length are correct. Also check for heap corruption.
// Test 2:
// - Some assorted indirect calls to alloc, and verify the nesting level, allocation count, 
// allocation level and length are correct. Also check for heap corruption.
// Test3:
// - Allocate a variety of objects and verify that the UHEAP_CHECKALL count is correct.
// - Verify the nesting of UHEAP_MARK and UHEAP_MARKEND macros.
// - Check the validity of the current thread's default heap.
// Test4:
// - Allocate memory for different heaps, check the total number of allocated cells
// for different heaps and for the current nested level is as expected.
// Test5:
// - Simulate heap allocation failures, allocate the memory from user and 
// kernel heap and check results are as expected.
// Platforms/Drives/Compatibility:
// All
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32def.h>
#include <e32def_private.h>
#include "dla.h"
#include "slab.h"
#include "page_alloc.h"
#include "heap_hybrid.h"
#define KHEAPCELLINFO RHybridHeap::SHeapCellInfo

LOCAL_D RTest test(_L("T_HEAPDB"));

#if defined(_DEBUG)

KHEAPCELLINFO CellInfo[4];

class RTestHeap : public RHeap
	{
public:
	void AttachInfo(KHEAPCELLINFO* aInfo)
		{
		RHybridHeap::STestCommand cmd;
		cmd.iData = aInfo;
		cmd.iCommand = RHybridHeap::ETestData;
		DebugFunction(RHeap::EHybridHeap, (TAny*)&cmd);
		}
	};

void AttachToHeap(RHeap* aHeap, TInt aInfo)
	{
	if (!aHeap)
		aHeap = (RHeap*)&User::Allocator();
	((RTestHeap*)aHeap)->AttachInfo(CellInfo + aInfo);
	}

void TestCellInfo(TInt aInfo, TInt aNest, TInt aAllocCount, TInt aLevelAlloc, TInt aSize, TAny* aAddr)
	{
	(void) aSize;
	KHEAPCELLINFO& ci = CellInfo[aInfo];
	RHeap::SDebugCell& cell = *ci.iStranded;
	test(cell.nestingLevel == aNest);
	test(cell.allocCount == aAllocCount);
	test(ci.iLevelAlloc == aLevelAlloc);
	test((&cell+1) == aAddr);
	}

const TInt KMaxFailureRate=100;
const TInt KThreadMemError=-50;

LOCAL_D TInt heapCount=1;
LOCAL_D RSemaphore threadSemaphore;
LOCAL_D TBool array1[KMaxFailureRate+1];
LOCAL_D	TBool array2[KMaxFailureRate+1];

LOCAL_C TInt ThreadEntryPoint(TAny*)
	{
	threadSemaphore.Wait();
	if (User::Alloc(4)==NULL)
		return(KThreadMemError);
	else
		return(KErrNone);
	} 

class TestRHeapDebug
	{
public:
	void Test1(void);
	void Test2(void);
	void Test3(void);
	void Test4(void);
	void Test5(void);
	};

LOCAL_C RHeap* allocHeap(TInt aSize)
//
// Allocate a chunk heap with max size aSize
//
	{

	TName n;
	n.Format(_L("TESTHEAP%d"),heapCount++);
	return(User::ChunkHeap(&n,aSize,aSize));
	}

void TestRHeapDebug::Test1(void)
	{

	TAny* p;

	///////////////////////
	// Test heaven cell is found for each method of allocating memory
	////////////////////////

	// new(TInt aSize)
	__UHEAP_MARK;
	__UHEAP_CHECKALL(0);
	__UHEAP_CHECK(0);
	p=new TUint; 
	__UHEAP_CHECKALL(1);
	__UHEAP_CHECK(1);
	__UHEAP_MARKEND;
	__UHEAP_CHECK(0);
	__UHEAP_CHECKALL(1);
	TestCellInfo(0, 1, 1, 1, User::AllocLen(p), p);
	User::Free(p);

	// new(TInt aSize,TInt anExtraSize)
	__UHEAP_MARK;
	p=new(4) TUint; 
	__UHEAP_MARKEND;
	TestCellInfo(0, 1, 1, 1, User::AllocLen(p), p);
	User::Free(p);

	// 	new(TInt aSize,TLeave)
	__UHEAP_MARK;
	p=new(ELeave) TUint; 
	__UHEAP_MARKEND;
	TestCellInfo(0, 1, 1, 1, User::AllocLen(p), p);
	User::Free(p);

	// Alloc
	__UHEAP_MARK;
	p=User::Alloc(32); 
	__UHEAP_MARKEND;
	TestCellInfo(0, 1, 1, 1, User::AllocLen(p), p);
	User::Free(p);

	// AllocL
	__UHEAP_MARK;
	p=User::AllocL(32);
	__UHEAP_MARKEND;
	TestCellInfo(0, 1, 1, 1, User::AllocLen(p), p);
	User::Free(p);

	// ReAlloc with Null parameter
	__UHEAP_MARK;
	p=User::ReAlloc(NULL, 32); 
	__UHEAP_MARKEND;
	TestCellInfo(0, 1, 1, 1, User::AllocLen(p), p);
	User::Free(p);

	// ReAllocL with Null parameter
	__UHEAP_MARK;
	p=User::ReAllocL(NULL, 32); 
	__UHEAP_MARKEND;
	TestCellInfo(0, 1, 1, 1, User::AllocLen(p), p);
	User::Free(p);

	// ReAlloc with non Null parameter
	__UHEAP_MARK;
	p=User::Alloc(128);	   
	p=User::ReAlloc(p, 4); 
	__UHEAP_MARKEND;
	TestCellInfo(0, 1, 1, 1, User::AllocLen(p), p);
	User::Free(p);
					
	// ReAlloc with non Null parameter such that cell is moved in memory
	__UHEAP_MARK;
	p=User::Alloc(128);	   
	TAny* temp=User::Alloc(128);
	p=User::ReAlloc(p, 526);   
	User::Free(temp);
	__UHEAP_MARKEND;
	TestCellInfo(0, 1, 3, 1, User::AllocLen(p), p);
	User::Free(p);

	// ReAllocL with non Null parameter
	__UHEAP_MARK;
	p=User::Alloc(32);	   
	p=User::ReAllocL(p, 128); 
	__UHEAP_MARKEND;
	TestCellInfo(0, 1, 1, 1, User::AllocLen(p), p);
	User::Free(p);
	}


void TestRHeapDebug::Test2(void)
	{ 
	// Some assorted indirect calls to alloc

	__UHEAP_MARK;
	CBufFlat* pBuf=CBufFlat::NewL(10); 
	__UHEAP_MARKEND;
	TestCellInfo(0, 1, 1, 1, User::AllocLen(pBuf), pBuf);
	delete pBuf;

	__UHEAP_MARK;
	HBufC8* pHBufC=HBufC8::New(10);	
	__UHEAP_MARKEND;
	TestCellInfo(0, 1, 1, 1, User::AllocLen(pHBufC), pHBufC);
	delete pHBufC;

// can also create a HBufC8 from a descriptor by using TDesC::Alloc
	}


void TestRHeapDebug::Test3(void)
	{ 

	//  Check num of cells detected is correct and CHECKTOTALNUM is ok
	// NOTE: CHECKTOTALNUM counts the TOTAL number of allocations in the heap regardless of
	// any MARKSTARTs
	// NOTE: the alloc count commences from the FIRST occurrence of a MARKSTART, so if one is nested
	// in another the alloc count will only start from the second MARKSTART if it applies to a 
	// different heap.
	__UHEAP_MARK;
	__UHEAP_CHECKALL(0);
	TAny* p1= new TUint; 
	__UHEAP_CHECKALL(1);
	TAny* p2= new(20) TUint;
	__UHEAP_CHECKALL(2);
	TAny* p3= User::Alloc(15); 
	__UHEAP_CHECKALL(3);
	__UHEAP_MARK;
	__UHEAP_CHECK(0);
	TAny* p4=User::Alloc(1); 
	TAny* p5 =new TUint; 
	__UHEAP_CHECK(2);
	__UHEAP_CHECKALL(5);
	__UHEAP_MARKEND;
	TestCellInfo(0, 2, 4, 2, User::AllocLen(p4), p4);
	__UHEAP_CHECKALL(5);
	__UHEAP_CHECK(3);
	__UHEAP_MARKENDC(3);
	User::Free(p1);
	User::Free(p2);
	User::Free(p3);
	User::Free(p4);
	User::Free(p5);

	// Check some nesting out
	p1=new TUint;
	__UHEAP_MARK;
	p2=new TUint; 
	__UHEAP_MARK;
	p3=new TUint; 
	__UHEAP_MARK;
	p4=new TUint; 
	__UHEAP_MARKEND;
	TestCellInfo(0, 3, 3, 1, User::AllocLen(p4), p4);
	__UHEAP_MARKEND;
	TestCellInfo(0, 2, 2, 1, User::AllocLen(p3), p3);
	__UHEAP_MARKEND;
	TestCellInfo(0, 1, 1, 1, User::AllocLen(p2), p2);
	User::Free(p1);
	User::Free(p2);
	User::Free(p3);
	User::Free(p4);
	User::Check();
	}

void TestRHeapDebug::Test4(void)
	{
	// Test with different heaps
	TAny* p1=new TUint;
	__UHEAP_MARK;	// Default start
	__UHEAP_CHECKALL(1);
	__UHEAP_CHECK(0);
	TAny* p2=new TUint;	  
	RHeap* pHeap1=allocHeap(1000); 
	AttachToHeap(pHeap1,1);
	__RHEAP_MARK(pHeap1); // Heap1 start
	__RHEAP_CHECKALL(pHeap1,0);
	__RHEAP_CHECK(pHeap1,0);
	TAny* p3=pHeap1->Alloc(4); 
	__RHEAP_CHECKALL(pHeap1,1);
	__RHEAP_CHECK(pHeap1,1);
	__RHEAP_CHECKALL(pHeap1,1);
	__UHEAP_CHECKALL(3);
	RHeap* pHeap2=allocHeap(1000);
	AttachToHeap(pHeap2,2);
	RHeap* pHeap3=allocHeap(1000);
	AttachToHeap(pHeap3,3);
	__UHEAP_CHECKALL(5);
	__RHEAP_MARK(pHeap2); // Heap2 start
	__RHEAP_MARK(pHeap3); // Heap3 start
	TAny* p4=pHeap2->Alloc(8); 
	TAny* p5=pHeap2->Alloc(37);
	TAny* p6=pHeap3->Alloc(32);	
	TAny* p7=pHeap1->Alloc(43);	
	__UHEAP_CHECKALL(5);
	__RHEAP_CHECKALL(pHeap1,2);
	__RHEAP_CHECKALL(pHeap2,2);
	__RHEAP_CHECKALL(pHeap3,1);
	__RHEAP_MARKEND(pHeap3); // Heap3 end
	TestCellInfo(3, 1, 1, 1, pHeap3->AllocLen(p6), p6);
	__RHEAP_MARKEND(pHeap2); // Heap2 end
	TestCellInfo(2, 1, 1, 2, pHeap2->AllocLen(p4), p4);
	pHeap1->Free(p3);
	__RHEAP_MARKEND(pHeap1); // Heap1 end
	TestCellInfo(1, 1, 2, 1, pHeap1->AllocLen(p7), p7);
	User::Free(p1);
	User::Free(p2);
	pHeap2->Free(p4);
	pHeap2->Free(p5);
	pHeap3->Free(p6);
	pHeap1->Free(p7);
	__UHEAP_CHECKALL(3);   
	pHeap2->Close();
	pHeap3->Close();
	__UHEAP_MARKEND;
	pHeap1->Close();
	__UHEAP_CHECKALL(0);
	}

void TestRHeapDebug::Test5()
// Check the alloc failure macros
	{
	TAny *p, *p1;
	RHeap* pHeap=allocHeap(1000);

	// DETERMINISTIC FAILURE
	__UHEAP_RESET;
	__UHEAP_FAILNEXT(1);
	test(User::Alloc(1)==NULL);
	p=User::Alloc(1);
	test(p!=NULL);
	User::FreeZ(p);
	__UHEAP_RESET;

	__RHEAP_RESET(pHeap);
	__RHEAP_FAILNEXT(pHeap,1);
	test(pHeap->Alloc(1)==NULL);
	p=pHeap->Alloc(1);
	test(p!=NULL);
	pHeap->FreeZ(p);
   	__RHEAP_RESET(pHeap);

	__KHEAP_RESET;
	__KHEAP_FAILNEXT(1);
	RSemaphore semaphore;
	test(semaphore.CreateLocal(1)==KErrNoMemory); // allocated from the kernel heap
	test(semaphore.CreateLocal(1)==KErrNone);
	semaphore.Close();
	__KHEAP_RESET;

	__UHEAP_SETFAIL(RHeap::EDeterministic,0);
	test(User::Alloc(1)==NULL);
	__UHEAP_RESET;

	__RHEAP_SETFAIL(pHeap,RHeap::EDeterministic,0);
	test(pHeap->Alloc(1)==NULL);
	__RHEAP_RESET(pHeap);

	__KHEAP_SETFAIL(RHeap::EDeterministic,0);
	test(semaphore.CreateLocal(1)==KErrNoMemory);
	__KHEAP_RESET;

	TInt determinism;
	for(determinism=1; determinism<=KMaxFailureRate; determinism++)
		{
		__UHEAP_SETFAIL(RHeap::EDeterministic,determinism);
		__RHEAP_SETFAIL(pHeap,RHeap::EDeterministic,determinism);
		for(TInt ii=1; ii<=determinism; ii++)
			{
			p=User::Alloc(1);
			p1=pHeap->Alloc(1);
			if(ii%determinism==0)
				{
				test(p==NULL);
				test(p1==NULL);
				}
			else
				{
				test(p!=NULL);
				test(p1!=NULL);
				pHeap->Free(p1); 
				User::Free(p);
				}
			}
		}
	__UHEAP_RESET;
	__RHEAP_RESET(pHeap);

	// Test SetKernelAllocFail
	// its not possible to test SetKernelAllocFail as above as it is not possible to control the
	// number of calls to Alloc for the dernel heap - but the following will definitely fail:
	__KHEAP_SETFAIL(RHeap::EDeterministic,1);
	RSemaphore r; 
	test(r.CreateLocal(1)==KErrNoMemory); // allocated from the kernel heap
	__KHEAP_SETFAIL(RHeap::EDeterministic,50);
	test(r.CreateLocal(1)==KErrNone);
	r.Close();
	__KHEAP_RESET;

	// RANDOM TESTS
	TInt numOccurences1, numOccurences2;

	__UHEAP_SETFAIL(RHeap::ERandom,1);
	test(User::Alloc(1)==NULL);
	__UHEAP_RESET;

	__RHEAP_SETFAIL(pHeap,RHeap::ERandom,1);
	test(pHeap->Alloc(1)==NULL);
	__RHEAP_RESET(pHeap);

//	__KHEAP_SETFAIL(RHeap::ERandom,1);
//	test(semaphore.CreateLocal(1)==KErrNoMemory);
//	__KHEAP_RESET;

	__UHEAP_SETFAIL(RHeap::ETrueRandom,1);
	test(User::Alloc(1)==NULL);
	__UHEAP_RESET;

	__RHEAP_SETFAIL(pHeap,RHeap::ETrueRandom,1);
	test(pHeap->Alloc(1)==NULL);
	__RHEAP_RESET(pHeap);

//	__KHEAP_SETFAIL(RHeap::ETrueRandom,1);
//	test(semaphore.CreateLocal(1)==KErrNoMemory);
//	__KHEAP_RESET;

	for(determinism=1; determinism<=KMaxFailureRate; determinism++)
		{
		__UHEAP_SETFAIL(RHeap::ERandom,determinism);
		__RHEAP_SETFAIL(pHeap,RHeap::ERandom,determinism);
        TInt ii;
		for(ii=1; ii<=determinism; ii++)
			{
			p=User::Alloc(1);
			p1=pHeap->Alloc(1);
			array1[ii]=(p==NULL);
			array2[ii]=(p==NULL);
			if(p)
				User::Free(p);
			if(p1)
				pHeap->Free(p1);
			}
		numOccurences1=0;
		numOccurences2=0;
		for(ii=1; ii<=determinism; ii++)
			{
			if(array1[ii])
				numOccurences1++;					
			if(array2[ii])
				numOccurences2++;
			}
		test(numOccurences1==1);
		test(numOccurences2==1);
		}
	__UHEAP_RESET;
	__RHEAP_RESET(pHeap);		

	__UHEAP_SETFAIL(RHeap::ERandom,5);
	TInt ii;
	for(ii=1; ii<=50; ii++)
		{
		p=User::Alloc(1);
		array1[ii]=(p==NULL);
		if(p)	
			User::Free(p);
		}
	numOccurences1=0;
	numOccurences2=0;
	for(ii=1; ii<=50; ii++)
		{
		if(array1[ii])
			{
			numOccurences1++;
			numOccurences2++;
			}
		if(ii%5==0)
			{
			test(numOccurences1==1);
			numOccurences1=0;
			}
		}
	test(numOccurences2==50/5);	
	
	// Cannot really test random failure of the kernel heap accurately

	pHeap->Close();
	//client.Disconnect();

	// Test failing the heap of a child thread
	// 1st test that it allocates normally
	TRequestStatus stat;
	RThread thread;
	test(threadSemaphore.CreateLocal(0)==KErrNone);
	test(thread.Create(_L("Thread"),ThreadEntryPoint,KDefaultStackSize,0x200,0x200,NULL)==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	threadSemaphore.Signal();
	User::WaitForRequest(stat);
	test(thread.ExitReason()==KErrNone);
	thread.Close();
#if defined(CAN_TEST_THREADS)
	// Now make the thread's heap fail
	test(thread.Create(_L("Thread"),ThreadEntryPoint,KDefaultStackSize,0x200,0x200,NULL)==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	TH_FAILNEXT(thread.Handle());
	threadSemaphore.Signal();
	User::WaitForRequest(stat);
	test(thread.ExitReason()==KThreadMemError);
	thread.Close();
	threadSemaphore.Close();
#endif
	}	

GLDEF_C TInt E32Main(void)
    {

	test.Title();
	AttachToHeap(NULL,0);
	test.Start(_L("Test1"));
	TestRHeapDebug T;
	T.Test1();	 
	test.Next(_L("Test2"));
	T.Test2();
	test.Next(_L("Test3"));
	T.Test3();
	test.Next(_L("Test4"));
	T.Test4();
	test.Next(_L("Test5"));
	T.Test5();
	test.End();
	return(0);
    }
#else
GLDEF_C TInt E32Main()
//
// Test unavailable in release build.
//
    {

	test.Title();	
	test.Start(_L("No tests for release builds"));
	test.End();
	return(0);
    }
#endif



