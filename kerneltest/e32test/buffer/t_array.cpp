// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\buffer\t_array.cpp
// Overview:
// Simple array tests.
// API Information:
// RArray, RPointerArray.
// Details:
// - Create fixed length array of 32 and 64 bit integer objects, an array 
// of pointers to objects and verify that they are created successfully. 
// - Simulate heap allocation failure test for the current thread's heap, 
// append some 32 & 64 bit integers to the created arrays and verify the 
// returned errors are as expected.
// - Append some 32, 64 bit integers to fixed length arrays of 32 and 64 
// bit integer objects respectively, check that KErrNoMemory is returned 
// as expected.
// - Verify heap allocation granularity.
// - Simulate heap allocation failure, attempt to insert an object into 
// the arrays, verify failure as expected and verify that the array 
// contents were not modified.
// - Remove elements from the arrays and verify that the number of 
// elements held in the arrays are as expected.
// - Append and remove an element to each array (uncompressed) and check
// that the number of elements held in the arrays are as expected.
// - Simulate heap allocation failure, compress the arrays and verify 
// that KErrNoMemory is returned on appending elements to the arrays.
// - Reset the arrays and check the number of elements held in the arrays are 0.
// - Append some 64 bit integer objects to the array of pointers to objects and 
// verify that the number of elements held in the array is as expected.
// - Empty the array of pointers, and verify that the heap has not been corrupted by 
// any of the tests.
// - Using a variety of random sized arrays, test RArray::FindInOrder and 
// RPointerArray::FindInOrder, verify that the results are as expected.
// - Using a variety of random sized arrays, test RArray::FindInSignedKeyOrder 
// and RArray::FindInUnsignedKeyOrder, verify that the results are as expected.
// - Using a variety of random sized arrays of a struct, test RArray::FindInUnsignedKeyOrder
// an dRArray::FindInUnsignedKeyOrder, verify that the results are as expected.
// - Using a variety of random sized arrays, test RPointerArray::FindInAddressOrder, 
// verify that the results are as expected.
// - Verify that the heap has not been corrupted by any of the tests.
// - Tests for RArray and standard array objects:
// - Append random numbers to the arrays and verify that the arrays are as expected. 
// - Append and remove integers to an RArray, check the values are added and removed 
// as expected.
// - Append some random numbers, check that the numbers are found in the array using 
// sequential and binary search techniques.
// - Append some random numbers, insert them into the arrays allowing duplicates 
// entries and without duplicate entries and check the numbers are found as expected.
// - Insert some random numbers into the arrays allowing duplicates, and check the 
// numbers are added as  expected.
// - Insert a sequence of integers into an array, use the SpecificFindInOrder method 
// and verify that results are as expected.
// - Tests for 4 byte RArrays:
// - Append random numbers to the arrays and verify that the arrays are as expected. 
// - Append and remove integers to an RArray, check the values are added and removed 
// as expected.
// - Append some random numbers, check that the numbers are found in the array using 
// sequential and binary search techniques.
// - Append some random numbers, insert them into the arrays allowing duplicates 
// entries and without duplicate entries and check the numbers are found as expected.
// - Insert some random numbers into the arrays allowing duplicates, and check the 
// numbers are added as  expected.
// - Insert a sequence of integers into an array, use the SpecificFindInOrder method 
// and verify that results are as expected.
// - Verify that the heap has not been corrupted by any of the tests.
// - Repeat the above test for arrays of unsigned integers, pointers, 64 bit integer 
// array objects and array of pointers objects.
// - Test and trap a variety of error conditions that cause the array functions to leave. 
// Test on arrays of integers, pointers, unsigned integers and TInts.
// - Verify that the heap has not been corrupted by any of the tests.
// - Perform simple array tests by appending, finding, find in order, insert in order, 
// sorting, growing and compressing arrays. Verify results are as expected.
// - Perform a variety of speed tests on array objects.
// - Test whether the heap has been corrupted by all the tests.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32math.h>

GLREF_C void DoSpeedTests();
GLREF_C void DoIntArrayTests();
GLREF_C void DoUintArrayTests();
GLREF_C void DoPointerArrayTests();
GLREF_C void DoPointerArrayLeavingInterfaceTest();
GLREF_C void DoPointerArrayAnyTests();
GLREF_C void DoPointerArrayAnyLeavingInterfaceTest();
GLREF_C void DoArrayLeavingInterfaceTest();
GLDEF_C void DoTIntArrayLeavingInterfaceTest();
GLDEF_C void DoTUintArrayLeavingInterfaceTest();
GLREF_C void DoSimpleArrayTests();
GLREF_C void DoRArrayTests();

GLDEF_C RTest test(_L("T_ARRAY"));

static TInt64 seed = MAKE_TINT64(0xb504f333,0xf9de6484);
GLDEF_C TInt Random()
	{
	// Using this formula ensures repeated numbers wont come up in the tests.
	seed = ((TUint) (69069*seed + 41));
	return (TInt) seed;
	}

#ifdef _DEBUG
RArray<TInt> *TheIntArray;
RPointerArray<TInt64> *ThePtrArray;
RArray<TInt64> *TheSimpleArray;

void DoAllocTests()
	{
	test.Next(_L("Testing alloc failure"));
	TheIntArray = new RArray<TInt>(16);
	test(TheIntArray!=NULL);
	ThePtrArray = new RPointerArray<TInt64>;
	test(ThePtrArray!=NULL);
	TheSimpleArray = new RArray<TInt64>;
	test(TheSimpleArray!=NULL);
	__UHEAP_MARK;
	__UHEAP_SETFAIL(RHeap::EDeterministic,1);
	TInt64 x = MAKE_TINT64(0xb504f333,0xf9de6484);
	TInt64 y = MAKE_TINT64(0xc90fdaa2,0xc2352168);
	TInt i;
	TInt r=TheIntArray->Append(0);
	test(r==KErrNoMemory);
	r=ThePtrArray->Append(&x);
	test(r==KErrNoMemory);
	r=TheSimpleArray->Append(x);
	test(r==KErrNoMemory);
	__UHEAP_RESET;
	r=TheIntArray->Append(0);
	test(r==KErrNone);
	r=ThePtrArray->Append(&x);
	test(r==KErrNone);
	r=TheSimpleArray->Append(x);
	test(r==KErrNone);
	TUint8* p1=new TUint8[1024];	// alloc a big cell to block simple expansion
	__UHEAP_SETFAIL(RHeap::EDeterministic,1);
	test.Next(_L("Testing granularity"));
	TInt n=0;
	while(r==KErrNone)
		{
		n++;
		r=TheIntArray->Append(0);
		}
	test(r==KErrNoMemory);
	test(n==16);
	test(TheIntArray->Count()==16);
	r=KErrNone;
	n=0;
	while(r==KErrNone)
		{
		n++;
		r=ThePtrArray->Append(&x);
		}
	test(r==KErrNoMemory);
	test(n==8);
	test(ThePtrArray->Count()==8);		// default
	r=KErrNone;
	n=0;
	while(r==KErrNone)
		{
		n++;
		r=TheSimpleArray->Append(x);
		}
	test(r==KErrNoMemory);
	test(n==8);
	test(TheSimpleArray->Count()==8);	// default

	r=TheIntArray->Insert(1,1);
	test(r==KErrNoMemory);
	test(TheIntArray->Count()==16);
	for (i=0; i<TheIntArray->Count(); i++)
		{
		test((*TheIntArray)[i]==0);
		}
	r=ThePtrArray->Insert(&y,1);
	test(r==KErrNoMemory);
	test(ThePtrArray->Count()==8);
	for (i=0; i<ThePtrArray->Count(); i++)
		{
		test((*ThePtrArray)[i]==&x);
		}
	r=TheSimpleArray->Insert(y,1);
	test(r==KErrNoMemory);
	test(TheSimpleArray->Count()==8);
	for (i=0; i<TheSimpleArray->Count(); i++)
		{
		test((*TheSimpleArray)[i]==x);
		}

	for (i=1; i<16; i++)
		{
		TheIntArray->Remove(1);
		}
	for (i=1; i<8; i++)
		{
		ThePtrArray->Remove(1);
		}
	for (i=1; i<8; i++)
		{
		TheSimpleArray->Remove(1);
		}
	test(TheIntArray->Count()==1);
	test(ThePtrArray->Count()==1);
	test(TheSimpleArray->Count()==1);

	__UHEAP_RESET;
	TAny* p2=User::Alloc(48);
	TAny* p3=User::Alloc(24);
	TAny* p4=User::Alloc(24);
	__UHEAP_SETFAIL(RHeap::EDeterministic,1);
	r=TheIntArray->Append(0);
	test(r==KErrNone);
	r=ThePtrArray->Append(&x);
	test(r==KErrNone);
	r=TheSimpleArray->Append(x);
	test(r==KErrNone);
	test(TheIntArray->Count()==2);
	test(ThePtrArray->Count()==2);
	test(TheSimpleArray->Count()==2);
	TheIntArray->Remove(1);
	ThePtrArray->Remove(1);
	TheSimpleArray->Remove(1);
	test(TheIntArray->Count()==1);
	test(ThePtrArray->Count()==1);
	test(TheSimpleArray->Count()==1);
	TheIntArray->Compress();
	ThePtrArray->Compress();
	TheSimpleArray->Compress();
	User::Free(p2);
	User::Free(p3);
	User::Free(p4);
	__UHEAP_RESET;
	p2=User::Alloc(48);
	p3=User::Alloc(24);
	p4=User::Alloc(24);
	__UHEAP_SETFAIL(RHeap::EDeterministic,1);
	r=TheIntArray->Append(0);
	test(r==KErrNoMemory);
	r=ThePtrArray->Append(&x);
	test(r==KErrNoMemory);
	r=TheSimpleArray->Append(x);
	test(r==KErrNoMemory);
	TheIntArray->Reset();
	ThePtrArray->Reset();
	TheSimpleArray->Reset();
	test(TheIntArray->Count()==0);
	test(ThePtrArray->Count()==0);
	test(TheSimpleArray->Count()==0);
	delete p1;
	User::Free(p2);
	User::Free(p3);
	User::Free(p4);
	__UHEAP_RESET;
	test.Next(_L("ResetAndDestroy"));
	TInt64 *i1=new TInt64;
	TInt64 *i2=new TInt64;
	TInt64 *i3=new TInt64;
	TInt64 *i4=new TInt64;
	ThePtrArray->Append(i1);
	ThePtrArray->Append(i2);
	ThePtrArray->Append(i3);
	ThePtrArray->Append(i4);
	test(ThePtrArray->Count()==4);
	ThePtrArray->ResetAndDestroy();
	__UHEAP_MARKEND;
	TheIntArray->Close();
	delete TheIntArray;
	ThePtrArray->Close();
	delete ThePtrArray;
	TheSimpleArray->Close();
	delete TheSimpleArray;
	}
#endif

class RHeapMonitor : public RAllocator
	{
public:
	static RHeapMonitor& Install();
	void Uninstall();
	RHeapMonitor();
public:
	virtual TAny* Alloc(TInt);
	virtual void Free(TAny*);
	virtual TAny* ReAlloc(TAny*, TInt, TInt);
	virtual TInt AllocLen(const TAny*) const;
	virtual TInt Compress();
	virtual void Reset();
	virtual TInt AllocSize(TInt&) const;
	virtual TInt Available(TInt&) const;
	virtual TInt DebugFunction(TInt, TAny*, TAny*);
	virtual TInt Extension_(TUint, TAny*&, TAny*);
public:
	RAllocator* iOrig;
	TInt iAllocs;
	TInt iFailedAllocs;
	TInt iFrees;
	TInt iReallocs;
	TInt iFailedReallocs;
	};

RHeapMonitor::RHeapMonitor()
	{
	iOrig = &User::Allocator();
	iAllocs = 0;
	iFailedAllocs = 0;
	iFrees = 0;
	iReallocs = 0;
	iFailedReallocs = 0;
	}

RHeapMonitor& RHeapMonitor::Install()
	{
	RHeapMonitor* m = new RHeapMonitor;
	test(m!=0);
	RAllocator* orig = User::SwitchAllocator(m);
	test(orig == m->iOrig);
	return *m;
	}

void RHeapMonitor::Uninstall()
	{
	RAllocator* m = User::SwitchAllocator(iOrig);
	test(m == this);
	delete this;
	}

TAny* RHeapMonitor::Alloc(TInt a)
	{
	++iAllocs;
	TAny* p = iOrig->Alloc(a);
	if (!p) ++iFailedAllocs;
	return p;
	}

void RHeapMonitor::Free(TAny* a)
	{
	if (a) ++iFrees;
	iOrig->Free(a);
	}

TAny* RHeapMonitor::ReAlloc(TAny* aCell, TInt aSize, TInt aMode)
	{
	if (aCell && aSize>0)
		++iReallocs;
	else if (aCell)
		++iFrees;
	else
		++iAllocs;
	TAny* p = iOrig->ReAlloc(aCell, aSize, aMode);
	if (!p && aSize>0)
		{
		if (aCell)
			++iFailedReallocs;
		else
			++iFailedAllocs;
		}
	return p;
	}

TInt RHeapMonitor::AllocLen(const TAny* a) const
	{
	return iOrig->AllocLen(a);
	}

TInt RHeapMonitor::Compress()
	{
	return iOrig->Compress();
	}

void RHeapMonitor::Reset()
	{
	iOrig->Reset();
	}

TInt RHeapMonitor::AllocSize(TInt& a) const
	{
	return iOrig->AllocSize(a);
	}

TInt RHeapMonitor::Available(TInt& a) const
	{
	return iOrig->Available(a);
	}

TInt RHeapMonitor::DebugFunction(TInt aFunc, TAny* a1, TAny* a2)
	{
	return iOrig->DebugFunction(aFunc, a1, a2);
	}

TInt RHeapMonitor::Extension_(TUint, TAny*&, TAny*)
	{
	return KErrExtensionNotSupported;
	}

template<class T>
void TestReserveT()
	{
	RHeapMonitor& m = RHeapMonitor::Install();
	TInt r;
	RArray<T> a(1);
	test(a.Count()==0);
	test(m.iAllocs==0);
	test(a.Append(1)==KErrNone);
	test(m.iAllocs==1);
	test(m.iReallocs==0);
	test(a.Append(2)==KErrNone);
	test(m.iReallocs==1);	// should have realloc'd
	a.Close();
	test(m.iFrees==1);
	test(m.iAllocs==1);
	test(m.iReallocs==1);
	test(a.Count()==0);
	test(a.Reserve(2)==KErrNone);
	test(m.iAllocs==2);
	TRAP(r,a.ReserveL(2));
	test(r==KErrNone);
	test(m.iFrees==1);
	test(m.iAllocs==2);
	test(m.iReallocs==1);
	test(a.Append(1)==KErrNone);
	test(m.iFrees==1);
	test(m.iAllocs==2);
	test(m.iReallocs==1);
	test(a.Append(2)==KErrNone);
	test(m.iFrees==1);
	test(m.iAllocs==2);
	test(m.iReallocs==1);	// shouldn't have realloc'd
	test(a.Append(3)==KErrNone);
	test(m.iFrees==1);
	test(m.iAllocs==2);
	test(m.iReallocs==2);	// should have realloc'd
	a.Close();
	test(m.iFrees==2);
	test(m.iAllocs==2);
	test(m.iReallocs==2);
	test(a.Count()==0);
	test(a.Reserve(2)==KErrNone);
	test(m.iFrees==2);
	test(m.iAllocs==3);
	test(m.iReallocs==2);
	test(a.Append(1)==KErrNone);
	test(m.iFrees==2);
	test(m.iAllocs==3);
	test(m.iReallocs==2);
	test(a.Append(2)==KErrNone);
	test(m.iFrees==2);
	test(m.iAllocs==3);
	test(m.iReallocs==2);
	test(a.Reserve(0x20000000)==KErrNoMemory);
	test(m.iFrees==2);
	test(m.iAllocs==3);
	test(m.iReallocs==2);
	test(m.iFrees==2);
	test(m.iAllocs==3);
	test(m.iReallocs==2);
	test(a.Reserve(8)==KErrNone);
	test(m.iFrees==2);
	test(m.iAllocs==3);
	test(m.iReallocs==3);
	test(a.Append(3)==KErrNone);
	test(a.Append(4)==KErrNone);
	test(a.Append(5)==KErrNone);
	test(a.Append(6)==KErrNone);
	test(a.Append(7)==KErrNone);
	test(a.Append(8)==KErrNone);
	test(a.Count()==8);
	test(m.iFrees==2);
	test(m.iAllocs==3);
	test(m.iReallocs==3);
	TInt i;
	for (i=0; i<=8; ++i)
		{
		test(a.Reserve(i)==KErrNone);
		test(m.iFrees==2);
		test(m.iAllocs==3);
		test(m.iReallocs==3);
		}
	test(a.Append(9)==KErrNone);
	test(m.iFrees==2);
	test(m.iAllocs==3);
	test(m.iReallocs==4);
	a.Close();
	test(m.iFrees==3);
	test(m.iAllocs==3);
	test(m.iReallocs==4);
#ifdef _DEBUG
	__UHEAP_FAILNEXT(1);
	test(a.Count()==0);
	test(a.Reserve(0)==KErrNone);
	test(m.iFrees==3);
	test(m.iAllocs==3);
	test(m.iReallocs==4);
	test(m.iFailedAllocs==0);
	test(a.Reserve(1)==KErrNoMemory);
	test(m.iFrees==3);
	test(m.iAllocs==4);
	test(m.iReallocs==4);
	test(m.iFailedAllocs==1);
	test(a.Reserve(1)==KErrNone);
	test(m.iFrees==3);
	test(m.iAllocs==5);
	test(m.iReallocs==4);
	test(m.iFailedAllocs==1);
	a.Close();
	test(m.iFrees==4);
	test(m.iAllocs==5);
	test(m.iReallocs==4);
	test(m.iFailedAllocs==1);
#endif
	m.Uninstall();
	TUint count = 0x80000000u / sizeof(T);

	// don't do this in the heap monitored section because
	// throwing a C++ exception allocates and frees memory
	TRAP(r,a.ReserveL(count));
	test(r==KErrNoMemory);
	}

void TestReserve()
	{
	test.Start(_L("Test Reserve()"));
	__UHEAP_MARK;

	TestReserveT<TInt>();
	TestReserveT<TInt64>();

	__UHEAP_MARKEND;
	test.End();
	}

GLDEF_C TInt E32Main()
	{

	CTrapCleanup* trapHandler=CTrapCleanup::New();
	test(trapHandler!=NULL);

	test.Title();
	test.Start(_L("Simple array tests"));
#ifdef _DEBUG
	DoAllocTests();
#endif
	TestReserve();
	__UHEAP_MARK;
	DoRArrayTests();
	__UHEAP_MARKEND;
	__UHEAP_MARK;
	DoIntArrayTests();
	__UHEAP_MARKEND;
	__UHEAP_MARK;
	DoUintArrayTests();
	__UHEAP_MARKEND;
	__UHEAP_MARK;
	DoPointerArrayTests();
	__UHEAP_MARKEND;
	__UHEAP_MARK;
	TRAPD(ret,DoArrayLeavingInterfaceTest());
	test(ret==KErrNone);
	__UHEAP_MARKEND;
	__UHEAP_MARK;
	TRAP(ret,DoPointerArrayLeavingInterfaceTest());
	test(ret==KErrNone);
	__UHEAP_MARKEND;
	__UHEAP_MARK;
	TRAP(ret,DoTIntArrayLeavingInterfaceTest());
	test(ret==KErrNone);
	__UHEAP_MARKEND;
	__UHEAP_MARK;
	TRAP(ret,DoTUintArrayLeavingInterfaceTest());
	test(ret==KErrNone);
	__UHEAP_MARKEND;
	__UHEAP_MARK;
	DoSimpleArrayTests();
	__UHEAP_MARKEND;
	__UHEAP_MARK;
	DoPointerArrayAnyTests();
	__UHEAP_MARKEND;
	__UHEAP_MARK;
	TRAP(ret,DoPointerArrayAnyLeavingInterfaceTest());
	test(ret==KErrNone);
	__UHEAP_MARKEND;
	__UHEAP_MARK;
	DoSpeedTests();
	__UHEAP_MARKEND;
	test.End();

	delete trapHandler;
	return KErrNone;
	}
