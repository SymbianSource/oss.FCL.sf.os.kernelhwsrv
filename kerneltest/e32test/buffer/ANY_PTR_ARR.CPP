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
// e32test\buffer\any_arr.cpp
// 
//

#include <e32test.h>
#include <e32math.h>
#include <e32std.h>
#include <e32std_private.h>

#define NUM_TESTS 200
const TInt KArraySize=1024;

GLREF_D RTest test;
GLREF_C TInt Random();

struct SPointerArray
	{
	TInt iCount;
	TAny** iEntries;
	TInt iAllocated;
	TInt iGranularity;
	};

LOCAL_D TInt64* Int64s;

LOCAL_C TInt64 Random64(TInt64& aMask)
	{
	TInt64 x = MAKE_TINT64(Random()&I64HIGH(aMask), Random()&I64LOW(aMask));
	return x;
	}

LOCAL_C TInt AnyAppendAndAccessTest(TInt aCount, TInt aNumTests, TInt64 aMask)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RPointerArray<TAny> a;
		TInt64 *pA=new TInt64[aCount];
		if (!pA)
			{
			a.Close();
			return -65535;
			}
		TInt i;
		for (i=0; i<aCount; i++)
			{
			Int64s[i]=Random64(aMask);
			pA[i]=Int64s[i];
			a.Append(&Int64s[i]);
			}
		if (a.Count()!=aCount)
			{
			a.Close();
			return -1;
			}
		for (i=0; i<aCount; i++)
			{
			if (a[i]!=&Int64s[i])
				{
				a.Close();
				return -2;
				}
			if (*(TInt64*)a[i]!=pA[i])
				{
				a.Close();
				return -3;
				}
			a[i]=&pA[i];
			}
		if (a.Count()!=aCount)
			{
			a.Close();
			return -4;
			}
		for (i=0; i<aCount; i++)
			{
			if (a[i]!=&pA[i])
				{
				a.Close();
				return -5;
				}
			if (*(TInt64*)a[i]!=Int64s[i])
				{
				a.Close();
				return -6;
				}
			}
		delete[] pA;
		a.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt AnyFindTest(TInt aCount, TInt aNumTests, TInt64 aMask)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RPointerArray<TAny> a;
		TInt64 *pA=new TInt64[aCount];
		if (!pA)
			{
			a.Close();
			return -65535;
			}
		TInt i;
		for (i=0; i<aCount; i++)
			{
			pA[i]=Random64(aMask);
			Int64s[i]=pA[i];
			a.Append(&Int64s[i]);
			}
		if (a.Count()!=aCount)
			{
			a.Close();
			return -1;
			}
		for (i=0; i<aCount; i++)
			{
			TInt r=a.Find(&Int64s[i]);
			if (r!=i)
				{
				a.Close();
				return -2;
				}
			r=a.Find(&pA[i]);
			if (r>=0)
				{
				a.Close();
				return -3;
				}
			r=a.FindInAddressOrder(&Int64s[i]);
			if (r!=i)
				{
				a.Close();
				return -4;
				}
			TInt j;
			r=a.FindInAddressOrder(&Int64s[i], j);
			if (r!=KErrNone || j!=i)
				{
				a.Close();
				return -5;
				}
			/*
			r=a.FindInAddressOrder(((TInt*)&Int64s[i])-1, j);
			test.Printf(_L("n == %d, i == %d, r == %d, j == %d"), n, i, r, j);
			if (r!=KErrNotFound || j!=i)
				{
				a.Close();
				return -6;
				}
			*/
			}
		delete[] pA;
		a.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt AnyInsertInAddressOrderTest(TInt aCount, TInt aNumTests, TUint aMask)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RPointerArray<TAny> a;
		RPointerArray<TAny> b;
		RPointerArray<TAny> c;
		TInt i;
		TInt cc=0;
		for (i=0; i<aCount; i++)
			{
			TAny* x=(TAny*)(((TUint)Random())&aMask);  // don't dereference this!
			a.Append(x);
			b.InsertInAddressOrderAllowRepeats(x);
			TInt r=c.InsertInAddressOrder(x);
			if (r==KErrNone)
				cc++;
			}
		if (a.Count()!=aCount)
			{
			a.Close();
			b.Close();
			c.Close();
			return -1;
			}
		if (b.Count()!=aCount)
			{
			a.Close();
			b.Close();
			c.Close();
			return -2;
			}
		for (i=0; i<aCount-1; i++)
			{
			if ((TUint)b[i]>(TUint)b[i+1])
				{
				a.Close();
				b.Close();
				c.Close();
				return -3;
				}
			}
		for (i=0; i<aCount; i++)
			{
			if (a.Find(b[i])<0)
				{
				a.Close();
				b.Close();
				c.Close();
				return -4;
				}
			if (b.Find(a[i])<0)
				{
				a.Close();
				b.Close();
				c.Close();
				return -5;
				}
			if (c.Find(a[i])<0)
				{
				a.Close();
				b.Close();
				c.Close();
				return -6;
				}
			}
		if (c.Count()!=cc)
			{
			a.Close();
			b.Close();
			c.Close();
			return -7;
			}
		for (i=0; i<c.Count()-1; i++)
			{
			if ((TUint)c[i]>=(TUint)c[i+1])
				{
				a.Close();
				b.Close();
				c.Close();
				return -8;
				}
			if (a.Find(c[i])<0)
				{
				a.Close();
				b.Close();
				c.Close();
				return -9;
				}
			}
		a.Close();
		b.Close();
		c.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt AnySortIntoAddressOrderTest(TInt aCount, TInt aNumTests, TUint aMask)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RPointerArray<TAny> a;
		RPointerArray<TAny> b;
		TInt i;
		for (i=0; i<aCount; i++)
			{
			TAny* x=(TAny*)(((TUint)Random())&aMask);  // don't dereference this!
			a.Append(x);
			b.InsertInAddressOrderAllowRepeats(x);
			}
		a.SortIntoAddressOrder();
		if (a.Count()!=aCount)
			{
			a.Close();
			b.Close();
			return -1;
			}
		if (b.Count()!=aCount)
			{
			a.Close();
			b.Close();
			return -2;
			}
		for (i=0; i<aCount; i++)
			{
			if (a[i]!=b[i])
				{
				a.Close();
				b.Close();
				return -3;
				}
			}
		a.Close();
		b.Close();
		}
	return KErrNone;
	}

LOCAL_C void TestGrowCompress(RPointerArray<TAny>* a, ...)
	{
	SPointerArray& pa = *(SPointerArray*)a;
	VA_LIST list;
	VA_START(list, a);
	TInt64 x;
	FOREVER
		{
		TInt r = KErrNone;
		TInt action = VA_ARG(list, TInt);
		if (action == -99)
			break;
		TInt result = VA_ARG(list, TInt);
		TInt orig = pa.iAllocated;
		if (action == -1)
			a->Compress();
		else if (action == -2)
			a->GranularCompress();
		else if (action == -3)
			a->Remove(pa.iCount - 1);
		else if (action > 0)
			{
			TInt i;
			for (i=0; i<action && r==KErrNone; ++i)
				r = a->Append(&x);
			}
		if ( (r<0 && (result!=r || pa.iAllocated!=orig)) || (r==0 && pa.iAllocated!=result) )
			{
			test.Printf(_L("Action %d Orig %d Expected %d r=%d newalloc=%d\n"), action, orig, result, r, pa.iAllocated);
			test(0);
			}
		}
	a->Reset();
	}

LOCAL_C void TestGrowCompress()
	{
	RPointerArray<TAny> a;
	TestGrowCompress(&a, 1, 8, 7, 8, 1, 16, 7, 16, 1, 24, -2, 24, -1, 17, 1, 25, -2, 25, -3, 25, -2, 24, -99);
	TestGrowCompress(&a, 1, 8, 7, 8, 1, 16, 7, 16, 1, 24, -2, 24, -1, 17, 1, 25, -2, 25, -3, 25, -3, 25, -2, 16, -99);

	RPointerArray<TAny> b(100);
	TestGrowCompress(&b, 1, 100, 99, 100, 1, 200, 99, 200, 1, 300, -2, 300, -1, 201, 1, 301, -2, 301, -3, 301, -2, 300, -99);
	TestGrowCompress(&b, 1, 100, 99, 100, 1, 200, 99, 200, 1, 300, -2, 300, -1, 201, 1, 301, -2, 301, -3, 301, -3, 301, -2, 200, -99);

	RPointerArray<TAny> c(8, 512);
	TestGrowCompress(&c, 1, 8, 7, 8, 1, 16, 7, 16, 1, 32, 15, 32, 1, 64, -2, 40, 7, 40, 1, 80, -1, 41, -99);

	RPointerArray<TAny> d(20, 640);
	TestGrowCompress(&d, 1, 20, 19, 20, 1, 50, 29, 50, 1, 125, -2, 60, -1, 51, -99);

	RPointerArray<TAny> e(8, 320);
	TestGrowCompress(&e, 1, 8, 7, 8, 1, 16, 7, 16, 1, 24, 7, 24, 1, 32, 7, 32, 1, 40, 7, 40, 1, 50, 9, 50, 1, 63, -99);

	RPointerArray<TAny> f(2, 257);
	TestGrowCompress(&f, 1, 2, 255, 256, 256, 512, 128, 640, 1, 643, 2, 643, 1, 646, -99);
	}

GLDEF_C void DoPointerArrayAnyTests()
	{
	test.Start(_L("TAny Pointer Arrays..."));
	test.Next(_L("Allocate memory"));
	Int64s=new TInt64[KArraySize];
	test(Int64s != NULL);

	test.Next(_L("AppendAndAccess tests..."));
	test.Next(_L("Count 10 Mask 0x0000000300000003"));
	test(AnyAppendAndAccessTest(10,NUM_TESTS,MAKE_TINT64(0x3,0x3))==KErrNone);
	test.Next(_L("Count 100 Range all"));
	test(AnyAppendAndAccessTest(100,NUM_TESTS,MAKE_TINT64(0xffffffff,0xffffffff))==KErrNone);

	test.Next(_L("Find tests..."));
	test.Next(_L("Count 10 Mask 0x0000000300000003"));
	test(AnyFindTest(10,NUM_TESTS,MAKE_TINT64(3,3))==KErrNone);
	test.Next(_L("Count 100 Range all"));
	test(AnyFindTest(100,NUM_TESTS,MAKE_TINT64(0xffffffff,0xffffffff))==KErrNone);

	test.Next(_L("InsertInAddressOrder tests..."));
	test.Next(_L("Count 50 Mask 0x3C000000"));
	test(AnyInsertInAddressOrderTest(50,NUM_TESTS,0x3C000000)==KErrNone);
	test.Next(_L("Count 100 all"));
	test(AnyInsertInAddressOrderTest(100,NUM_TESTS,0xffffffff)==KErrNone);

	test.Next(_L("Sort tests..."));
	test.Next(_L("Count 30 Mask 0x3C000000"));
	test(AnySortIntoAddressOrderTest(30,NUM_TESTS,0x3C000000)==KErrNone);
	test.Next(_L("Count 100 all"));
	test(AnySortIntoAddressOrderTest(100,NUM_TESTS,0xffffffff)==KErrNone);
	
	test.Next(_L("Test Grow/Compress"));
	TestGrowCompress();

	test.Next(_L("Test RPointerArray::Array..."));
	TInt a;
	TInt b;
	TInt c;
	RPointerArray<TAny> ptrArr;
	ptrArr.Append(&a);
	ptrArr.Append(&b);
	ptrArr.Append(&c);

	TArray<TAny*> arr=ptrArr.Array();
	test(arr.Count()==3);
	test(arr[0]==&a);
	test(arr[1]==&b);
	test(arr[2]==&c);

	ptrArr.Reset();

	delete[] Int64s;
	test.End();
	}

GLDEF_C void DoPointerArrayAnyLeavingInterfaceTest()
	{
	TInt trap, ret(0);
	TInt64 Int64s[3];
	for (TInt i=0;i<3;i++) Int64s[i] = i;

	RPointerArray<TAny> pArray;
	CleanupClosePushL(pArray);

	test.Start(_L("Checking Leaving TAny Pointer Arrays Interface..."));

	test.Next(_L("AppendL test..."));
	TRAP(trap, pArray.AppendL(&Int64s[0]));
	test(trap==KErrNone);

	test.Next(_L("InsertL test..."));
	TRAP(trap, pArray.InsertL(&Int64s[1],1));
	test(trap==KErrNone);

	test.Next(_L("Test FindL(const T* anEntry) const..."));
	TRAP(trap, ret = pArray.FindL(&Int64s[0]));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = pArray.FindL(&Int64s[2]));
	test(trap==KErrNotFound);

	test.Next(_L("Test FindInAddressOrderL(const T* anEntry) const..."));
	TRAP(trap, ret = pArray.FindInAddressOrderL(&Int64s[0]));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = pArray.FindInAddressOrderL(&Int64s[2]));
	test(trap==KErrNotFound);

	test.Next(_L("Test FindInAddressOrderL(const T* anEntry, TInt& anIndex) const..."));
	TRAP(trap, pArray.FindInAddressOrderL(&Int64s[0], ret));
	test(trap==0);
	test(ret==0);
	TRAP(trap, pArray.FindInAddressOrderL(&Int64s[2], ret));
	test(trap==KErrNotFound);

	test.Next(_L("Test SpecificFindInAddressOrderL(const T* anEntry, TInt aMode) const..."));
	TRAP(trap, ret = pArray.SpecificFindInAddressOrderL(&Int64s[0], EArrayFindMode_First));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = pArray.SpecificFindInAddressOrderL(&Int64s[2], EArrayFindMode_First));
	test(trap==KErrNotFound);

	test.Next(_L("Test SpecificFindInAddressOrderL(const T* anEntry, TInt& anIndex, TInt aMode) const..."));
	TRAP(trap, pArray.SpecificFindInAddressOrderL(&Int64s[0], ret, EArrayFindMode_First));
	test(trap==0);
	test(ret==0);
	TRAP(trap, pArray.SpecificFindInAddressOrderL(&Int64s[2], ret, EArrayFindMode_First));
	test(trap==KErrNotFound);

	test.Next(_L("Test InsertInAddressOrderL(const T* anEntry)..."));
	TRAP(trap, pArray.InsertInAddressOrderL(&Int64s[0]));
	test(trap==KErrAlreadyExists);
	TRAP(trap, pArray.InsertInAddressOrderL(&Int64s[2]));
	test(trap==KErrNone);
	pArray.Remove(2);
	
	test.Next(_L("Test InsertInAddressOrderAllowRepeatsL(const T* anEntry)..."));
	TRAP(trap, pArray.InsertInAddressOrderAllowRepeatsL(&Int64s[2]));
	test(trap==KErrNone);
	pArray.Remove(2);
	
	CleanupStack::PopAndDestroy(&pArray);
	test.End();
	}
