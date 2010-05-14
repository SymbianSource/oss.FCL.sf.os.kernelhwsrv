// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\heap\t_heap2.cpp
// Overview:
// Tests RHeap class, including a stress test and a "grow in place"
// ReAlloc test.
// API Information:
// RHeap
// Details:
// - Test allocation on fixed length heaps in local, disconnected chunks for
// different heap sizes and alignments.  Assumes knowledge of heap
// implementation.
// - Test allocation, free, reallocation and compression on chunk heaps with
// different maximum and minimum lengths and alignments.  Assumes knowledge
// of heap implementation.      
// - Stress test heap implementation with a single thread that allocates, frees
// and reallocates cells, and checks the heap.
// - Stress test heap implementation with two threads that run concurrently.
// - Create a chunk heap, test growing in place by allocating a cell and 
// then reallocating additional space until failure, verify that the cell 
// did not move and the size was increased.
// - The heap is checked to verify that no cells remain allocated after the 
// tests are complete.
// Platforms/Drives/Compatibility:
// All
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32hal.h>
#include <e32def.h>
#include <e32def_private.h>
#include "dla.h"
#include "slab.h"
#include "page_alloc.h"
#include "heap_hybrid.h"

// Needed for KHeapShrinkHysRatio which is now ROM 'patchdata'
#include "TestRHeapShrink.h"

#define DECL_GET(T,x)		inline T x() const {return i##x;}
#define DECL_GET2(T,x,y)	inline T y() const {return i##x;}


#ifdef __EABI__
       IMPORT_D extern const TInt KHeapMinCellSize;
#else
       const TInt KHeapMinCellSize = 0;
#endif

	   const TInt KAllocCellSize = (TInt)RHeap::EAllocCellSize;
	   const TInt KSizeOfHeap = (TInt)sizeof(RHybridHeap);	   
	   
	   
RTest test(_L("T_HEAP2"));

#define	TEST_ALIGN(p,a)		test((TLinAddr(p)&((a)-1))==0)

struct STestCell
	{
	enum {EMagic = 0xb8aa3b29};

	TUint32 iLength;
	TUint32 iData[1];

	void Set(TInt aLength);
	void Verify(TInt aLength);
	void Verify(const TAny* aInitPtr, TInt aInitLength, TInt aLength);
	};

void STestCell::Set(TInt aLength)
	{
	TInt i;
	TUint32 x = (TUint32)this ^ (TUint32)aLength ^ (TUint32)EMagic;
	if (aLength==0)
		return;
	iLength = x;
	aLength /= sizeof(TUint32);
	for (i=0; i<aLength-1; ++i)
		{
		x *= 69069;
		x += 41;
		iData[i] = x;
		}
	}

void STestCell::Verify(TInt aLength)
	{
	Verify(this, aLength, aLength);
	}

void STestCell::Verify(const TAny* aInitPtr, TInt aInitLength, TInt aLength)
	{
	TInt i;
	TUint32 x = (TUint32)aInitPtr ^ (TUint32)aInitLength ^ (TUint32)EMagic;
	if ( aLength < (TInt) sizeof(*this) )
		return;
	test(iLength == x);
	aLength /= sizeof(TUint32);
	for (i=0; i<aLength-1; ++i)
		{
		x *= 69069;
		x += 41;
		test(iData[i] == x);
		}
	}


	
class RTestHeap : public RHeap
	{
public:
	TInt CheckAllocatedCell(const TAny* aCell) const;
	void FullCheckAllocatedCell(const TAny* aCell) const;
	TAny* TestAlloc(TInt aSize);
	void TestFree(TAny* aPtr);
	TAny* TestReAlloc(TAny* aPtr, TInt aSize, TInt aMode=0);
	void FullCheck();
	static void WalkFullCheckCell(TAny* aPtr, TCellType aType, TAny* aCell, TInt aLen);
	};

TInt RTestHeap::CheckAllocatedCell(const TAny* aCell) const
	{
	TInt len = AllocLen(aCell);
	return len;
	}

void RTestHeap::FullCheckAllocatedCell(const TAny* aCell) const
	{
	((STestCell*)aCell)->Verify(CheckAllocatedCell(aCell));
	}

TAny* RTestHeap::TestAlloc(TInt aSize)
	{
	TAny* p = Alloc(aSize);
	if (p)
		{
		TInt len = CheckAllocatedCell(p);
		test(len>=aSize);		
		((STestCell*)p)->Set(len);
		}
	return p;
	}

void RTestHeap::TestFree(TAny* aPtr)
	{
	if (aPtr)
		FullCheckAllocatedCell(aPtr);
	Free(aPtr);
	}

TAny* RTestHeap::TestReAlloc(TAny* aPtr, TInt aSize, TInt aMode)
	{
	TInt old_len = aPtr ? CheckAllocatedCell(aPtr) : 0;
	if (aPtr)
		((STestCell*)aPtr)->Verify(old_len);
	TAny* p = ReAlloc(aPtr, aSize, aMode);
	if (!p)
		{
		((STestCell*)aPtr)->Verify(old_len);
		return p;
		}
	TInt new_len = CheckAllocatedCell(p);
	test(new_len>=aSize);		
	if (p == aPtr)
		{
		((STestCell*)p)->Verify(p, old_len, Min(old_len, new_len));
		if (new_len != old_len)
			((STestCell*)p)->Set(new_len);
		return p;
		}
	test(!(aMode & ENeverMove));
	test((new_len > old_len) || (aMode & EAllowMoveOnShrink));
	if (old_len)
		((STestCell*)p)->Verify(aPtr, old_len, Min(old_len, aSize));		
    ((STestCell*)p)->Set(new_len);
	return p;
	}

struct SHeapCellInfo
	{
	RTestHeap* iHeap;
	TInt iTotalAlloc;
	TInt iTotalAllocSize;
	TInt iTotalFree;
	TUint8* iNextCell;
	};

void RTestHeap::WalkFullCheckCell(TAny* aPtr, TCellType aType, TAny* aCell, TInt aLen)
	{
	(void)aCell;
	::SHeapCellInfo& info = *(::SHeapCellInfo*)aPtr;
	switch(aType)
		{
		case EGoodAllocatedCell:
			{
			TInt len = aLen;
			info.iTotalAllocSize += len;
			STestCell* pT = (STestCell*)aCell;
			++info.iTotalAlloc;
			pT->Verify(len);
			break;
			}
		case EGoodFreeCell:
			{
			++info.iTotalFree;
			break;
			}
		default:
			test.Printf(_L("TYPE=%d ??\n"),aType);
			test(0);
			break;
		}
	}

void RTestHeap::FullCheck()
	{
	::SHeapCellInfo info;
	Mem::FillZ(&info, sizeof(info));
	info.iHeap = this;
	DebugFunction(EWalk, (TAny*)&WalkFullCheckCell, &info);
	TInt count = AllocSize(iTotalAllocSize);
	test(info.iTotalAlloc == count);
	test(info.iTotalAllocSize == iTotalAllocSize);

	}
	
struct SHeapStress
	{
	RThread iThread;
	volatile TBool iStop;
	TInt iAllocs;
	TInt iFailedAllocs;
	TInt iFrees;
	TInt iReAllocs;
	TInt iFailedReAllocs;
	TInt iChecks;
	TUint32 iSeed;
	RAllocator* iAllocator;

	TUint32 Random();
	};

TUint32 SHeapStress::Random()
	{
	iSeed *= 69069;
	iSeed += 41;
	return iSeed;
	}

TInt RandomLength(TUint32 aRandom)
	{
	TUint8 x = (TUint8)aRandom;
	if (x & 0x80)
		return (x & 0x7f) << 7;
	return x & 0x7f;
	}

TInt HeapStress(TAny* aPtr)
	{
	SHeapStress& hs = *(SHeapStress*)aPtr;
	RTestHeap* h = (RTestHeap*)&User::Allocator();
	TUint8* cell[256];
	TInt len[256];

	Mem::FillZ(cell, sizeof(cell));
	Mem::FillZ(len, sizeof(len));

	RThread::Rendezvous(KErrNone);
	while (!hs.iStop)
		{
		// allocate all cells
		TInt i;
		for (i=0; i<256; ++i)
			{
			if (!cell[i])
				{
				++hs.iAllocs;
				cell[i] = (TUint8*)h->TestAlloc(RandomLength(hs.Random()));
				if (cell[i])
					len[i] = h->AllocLen(cell[i]);
				else
					++hs.iFailedAllocs;
				}
			}

		// free some cells
		TInt n = 64 + (hs.Random() & 127);
		while (--n)
			{
			i = hs.Random() & 0xff;
			if (cell[i])
				{
				test(h->AllocLen(cell[i]) == len[i]);
				h->TestFree(cell[i]);
				cell[i] = NULL;
				len[i] = 0;
				++hs.iFrees;
				}
			}

		// realloc some cells
		n = 64 + (hs.Random() & 127);
		while (--n)
			{
			TUint32 rn = hs.Random();
			i = (rn >> 8) & 0xff;
			TInt new_len = RandomLength(rn);
			if (cell[i])
				{
				test(h->AllocLen(cell[i]) == len[i]);
				++hs.iReAllocs;
				TUint8* p = (TUint8*)h->TestReAlloc(cell[i], new_len, rn >> 16);
				if (p)
					{
					cell[i] = p;
					len[i] = h->AllocLen(p);
					}
				else
					++hs.iFailedReAllocs;
				}
			}

		// check the heap
		h->Check();
		++hs.iChecks;
		}
	return 0;
	}

void CreateStressThread(SHeapStress& aInfo)
	{
	Mem::FillZ(&aInfo, _FOFF(SHeapStress, iSeed));
	RThread& t = aInfo.iThread;
	TInt r = t.Create(KNullDesC(), &HeapStress, 0x2000, aInfo.iAllocator, &aInfo);
	test(r==KErrNone);
	t.SetPriority(EPriorityLess);
	TRequestStatus s;
	t.Rendezvous(s);
	test(s == KRequestPending);
	t.Resume();
	User::WaitForRequest(s);
	test(s == KErrNone);
	test(t.ExitType() == EExitPending);
	t.SetPriority(EPriorityMuchLess);
	}

void StopStressThread(SHeapStress& aInfo)
	{
	RThread& t = aInfo.iThread;
	TRequestStatus s;
	t.Logon(s);
	aInfo.iStop = ETrue;
	User::WaitForRequest(s);
	const TDesC& exitCat = t.ExitCategory();
	TInt exitReason = t.ExitReason();
	TInt exitType = t.ExitType();
	test.Printf(_L("Exit type %d,%d,%S\n"), exitType, exitReason, &exitCat);
	test(exitType == EExitKill);
	test(exitReason == KErrNone);
	test(s == KErrNone);
	test.Printf(_L("Total Allocs    : %d\n"), aInfo.iAllocs);
	test.Printf(_L("Failed Allocs   : %d\n"), aInfo.iFailedAllocs);
	test.Printf(_L("Total Frees		: %d\n"), aInfo.iFrees);
	test.Printf(_L("Total ReAllocs  : %d\n"), aInfo.iReAllocs);
	test.Printf(_L("Failed ReAllocs : %d\n"), aInfo.iFailedReAllocs);
	test.Printf(_L("Heap checks     : %d\n"), aInfo.iChecks);
	}

void DoStressTest1(RAllocator* aAllocator)
	{
	RTestHeap* h = (RTestHeap*)aAllocator;
	test.Printf(_L("Test Stress 1: max=%x\n"),	h->MaxLength());
	SHeapStress hs;
	hs.iSeed = 0xb504f334;
	hs.iAllocator = aAllocator;
	CreateStressThread(hs);
	User::After(10*1000000);
	StopStressThread(hs);
	CLOSE_AND_WAIT(hs.iThread);
	h->FullCheck();
	}

void DoStressTest2(RAllocator* aAllocator)
	{
	RTestHeap* h = (RTestHeap*)aAllocator;
	test.Printf(_L("Test Stress 2: max=%x\n"),	h->MaxLength());	
	SHeapStress hs1;
	SHeapStress hs2;
	hs1.iSeed = 0xb504f334;
	hs1.iAllocator = aAllocator;
	hs2.iSeed = 0xddb3d743;
	hs2.iAllocator = aAllocator;
	CreateStressThread(hs1);
	CreateStressThread(hs2);
	User::After(20*1000000);
	StopStressThread(hs1);
	StopStressThread(hs2);
	CLOSE_AND_WAIT(hs1.iThread);
	CLOSE_AND_WAIT(hs2.iThread);
	h->FullCheck();
	}

void StressTests()
	{
	RHeap* h;
	h = UserHeap::ChunkHeap(&KNullDesC(), 0x1000, 0x100000, 0x1000, 4);
	test(h != NULL);
	DoStressTest1(h);
	h->Reset();
	DoStressTest2(h);
	h->Close();
	h = UserHeap::ChunkHeap(&KNullDesC(), 0x1000, 0x100000, 0x1000, 8);
	test(h != NULL);
	DoStressTest1(h);
	h->Reset();
	DoStressTest2(h);
	h->Close();
	}
		
TInt TestHeapGrowInPlace(TInt aMode)
    {
    TBool reAllocs=EFalse;
    RHeap* myHeap;
	//
	// Fixed DL heap used. 
	//
	myHeap = UserHeap::ChunkHeap(NULL,0x4000,0x4000,0x1000);
    
    TAny *testBuffer,*testBuffer2;
    // Start size chosen so that 1st realloc will use up exactly all the heap.
    // Later iterations wont, and there will be a free cell at the end of the heap.
    TInt currentSize = ((0x800) - KSizeOfHeap) - KAllocCellSize;
    TInt growBy = 0x800;

    testBuffer2 = myHeap->Alloc(currentSize);

    do 
    {
    	testBuffer = testBuffer2;
	    currentSize+=growBy;
		testBuffer2 = myHeap->ReAlloc(testBuffer,currentSize,aMode);	
		
		if (testBuffer2) 
			{
				
			if (testBuffer!=testBuffer2)
					reAllocs = ETrue;
			}
		growBy-=16;
 	} while (testBuffer2);
    currentSize-=growBy;	
    
    myHeap->Free(testBuffer);
    myHeap->Close();
    
    // How did we do?
    if (reAllocs) 
    	{
    	test.Printf(_L("Failure - Memory was moved!\n"));
    	return -100;
    	}
    if (currentSize<= 0x3000) 
    	{
    	test.Printf(_L("Failed to grow by a reasonable amount!\n"));
    	return -300;
    	}
        
    return KErrNone;
    }
    
void ReAllocTests()
	{
	test.Next(_L("Testing Grow In Place"));
	test(TestHeapGrowInPlace(0)==KErrNone);
    test(TestHeapGrowInPlace(RHeap::ENeverMove)==KErrNone);
	}

RHeap* TestDEF078391Heap = 0;

TInt TestDEF078391ThreadFunction(TAny*)
	{
    TestDEF078391Heap = UserHeap::ChunkHeap(NULL,0x1000,0x100000,KMinHeapGrowBy,0,EFalse);
	return TestDEF078391Heap ? KErrNone : KErrGeneral;
	}

void TestDEF078391()
	{
	// Test that creating a multithreaded heap with UserHeap::ChunkHeap
	// doesn't create any reference counts on the creating thread.
	// This is done by creating a heap in a named thread, then exiting
	// the thread and re-creating it with the same name.
	// This will fail with KErrAlreadyExists if the orinal thread has
	// not died because of an unclosed reference count.
	test.Next(_L("Test that creating a multithreaded heap doesn't open references of creator"));
	_LIT(KThreadName,"ThreadName");
	RThread t;
	TInt r=t.Create(KThreadName,TestDEF078391ThreadFunction,0x1000,0x1000,0x100000,NULL);
	test(r==KErrNone);
	TRequestStatus status;
	t.Logon(status);
	t.Resume();
	User::WaitForRequest(status);
	test(status==KErrNone);
	test(t.ExitType()==EExitKill);
	test(t.ExitReason()==KErrNone);
	CLOSE_AND_WAIT(t);
	test(TestDEF078391Heap!=0);
	User::After(1000000); // give more opportunity for thread cleanup to happen

	// create thread a second time
	r=t.Create(KThreadName,TestDEF078391ThreadFunction,0x1000,0x1000,0x100000,NULL);
	test(r==KErrNone);
	t.Kill(0);
	CLOSE_AND_WAIT(t);

	// close the heap that got created earlier
	TestDEF078391Heap->Close();
	}

void PageBitmapGrowTest()
	{
	// Create a large heap to allocate 4 Mb memory (64 * 68 kb).
	test.Next(_L("Allocate 64 * 68 kbytes to cause page bitmap growing"));
	RHeap* myHeap;
	myHeap = UserHeap::ChunkHeap(NULL,0x1000,0x500000,0x1000);
	test(myHeap!=NULL);
	TInt OrigSize = myHeap->Size();	
	TUint8* cell[64];
		// allocate all cells
	TInt i;
	RTestHeap* h = (RTestHeap*)myHeap;
	for (i=0; i<64; ++i)
		{
		cell[i] = (TUint8*)h->TestAlloc(0x11000);
		test(cell[i]!=NULL);
    	}
	h->FullCheck();
	
	// Release all allocated buffers by reseting heap
	TInt Size = myHeap->Size();
	test(Size > 0x400000);	
	myHeap->Reset();
	TInt Count = myHeap->AllocSize(Size);
 	test(Count==0);
	test(Size==0);
	Size = myHeap->Size();
	test(Size==OrigSize);
	
	h->Close();
	
	}
	
TInt E32Main()
	{
	test.Title();
	__KHEAP_MARK;
	test.Start(_L("Testing heaps"));
	TestDEF078391();
	StressTests();
	ReAllocTests();
	//
	// Some special tests for slab- and paged allocator
	//
	PageBitmapGrowTest();	
	test.End();
	__KHEAP_MARKEND;
	return 0;
	}
