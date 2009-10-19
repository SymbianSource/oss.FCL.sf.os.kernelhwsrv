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
// e32test\buffer\uint_arr.cpp
// 
//

#include <e32test.h>
#include <e32math.h>

#define NUM_TESTS 250

GLREF_D RTest test;
GLREF_C TInt Random();

LOCAL_C TInt IntAppendAndAccessTest(TInt aCount, TInt aNumTests, TInt aRange)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<TUint> a;
		TUint *pA=(TUint*)User::Alloc(aCount*sizeof(TUint));
		if (!pA)
			{
			a.Close();
			return -65535;
			}
		TInt i;
		for (i=0; i<aCount; i++)
			{
			TUint x=Random()&aRange;
			a.Append(x);
			pA[i]=x;
			}
		if (a.Count()!=aCount)
			{
			a.Close();
			return -1;
			}
		for (i=0; i<aCount; i++)
			{
			if (a[i]!=pA[i])
				{
				a.Close();
				return -2;
				}
			TUint x=Random()&aRange;
			a[i]=x;
			pA[i]=x;
			}
		if (a.Count()!=aCount)
			{
			a.Close();
			return -3;
			}
		for (i=0; i<aCount; i++)
			{
			if (a[i]!=pA[i])
				{
				a.Close();
				return -4;
				}
			}
		delete pA;
		a.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt IntRemoveTest()
	{
	TInt m=32;
	TInt n=m*m+1;
	RArray<TUint> a;
	TInt i;
	for (i=0; i<n; i++)
		{
		a.Append(TUint(i));
		}
	TInt p=2;
	for (i=2; i<=m; i++)
		{
		TInt j;
		for (j=0; j<(2*i-2); j++)
			{
			a.Remove(p);
			}
		p++;
		}
	if (a.Count()!=m+1)
		{
		a.Close();
		return -1;
		}
	for (i=0; i<m; i++)
		{
		if (a[i]!=TUint(i*i))
			{
			a.Close();
			return -2;
			}
		}
	a.Close();
	return KErrNone;
	}

LOCAL_C TInt IntFindTest(TInt aCount, TInt aNumTests, TInt aRange)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<TUint> a;
		TUint *pA=(TUint*)User::Alloc(aCount*sizeof(TUint));
		if (!pA)
			{
			a.Close();
			return -65535;
			}
		TInt i;
		for (i=0; i<aCount; i++)
			{
			TUint x=Random()&aRange;
			a.Append(x);
			pA[i]=x;
			}
		if (a.Count()!=aCount)
			{
			a.Close();
			return -1;
			}
		for (i=0; i<aCount; i++)
			{
			TInt r=a.Find(pA[i]);
			if (r<0 || pA[r]!=pA[i] || r>i)
				{
				a.Close();
				return -2;
				}
			TUint x=Random()&aRange;
			r=a.Find(x);
			if (r<0)
				{
				TInt j;
				for (j=0; j<aCount; j++)
					{
					if (pA[j]==x)
						{
						a.Close();
						return -3;
						}
					}
				}
			else if (pA[r]!=x)
				{
				a.Close();
				return -4;
				}
			else
				{
				TInt j;
				for (j=0; j<r; j++)
					{
					if (pA[j]==x)
						{
						a.Close();
						return -5;
						}
					}
				}
			}
		delete pA;
		a.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt IntFindInOrderTest(TInt aCount, TInt aNumTests, TInt aRange)
// require aRange*aCount<2^32
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<TUint> a;
		TUint *pA=(TUint*)User::Alloc(aCount*sizeof(TUint));
		if (!pA)
			{
			a.Close();
			return -65535;
			}
		TInt i=0;
		TUint y=TUint(Random())>>16;
		for(i=0; i<aCount; i++)
			{
			TUint x=Random()&aRange;	// this is always >=0
			a.Append(y);
			pA[i]=y;
			y+=x;
			}
		if (a.Count()!=aCount)
			{
			a.Close();
			return -1;
			}
		for (i=0; i<aCount; i++)
			{
			TInt r=a.FindInOrder(pA[i]);
			if (r<0 || pA[r]!=pA[i])
				{
				a.Close();
				return -2;
				}
			TUint x=Random()&aRange;
			r=a.FindInOrder(x);
			if (r<0)
				{
				TInt j;
				for (j=0; j<aCount; j++)
					{
					if (pA[j]==x)
						{
						a.Close();
						return -3;
						}
					}
				}
			else if (pA[r]!=x)
				{
				a.Close();
				return -4;
				}
			}
		delete pA;
		a.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt IntFindInOrderTest2()
	{
	TInt i;
	RArray<TUint> sq;
	for (i=0; i<1024; i++)
		{
		TInt j=i*i;
		sq.Append(j);
		}
	for (i=0; i<10000; i++)
		{
		TInt x=Random()&1023;
		TInt y=x*x;
		TInt r=sq.FindInOrder(y);
		if (r!=x)
			{
			sq.Close();
			return -1;
			}
		}
	sq.Close();
	return 0;
	}

LOCAL_C TInt IntInsertInOrderTest(TInt aCount, TInt aNumTests, TInt aRange)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<TUint> a;
		RArray<TUint> b;
		RArray<TUint> c;
		TInt i;
		TInt cc=0;
		for (i=0; i<aCount; i++)
			{
			TUint x=Random()&aRange;
			a.Append(x);
			b.InsertInOrderAllowRepeats(x);
			TInt r=c.InsertInOrder(x);
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
			if (b[i]>b[i+1])
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
			if (c[i]>=c[i+1])
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

LOCAL_C TInt IntInsertInOrderTest2()
	{
	TInt i;
	RArray<TUint> sq;
	for (i=0; i<1024; i++)
		{
		TInt j=i*i;
		sq.InsertInOrder(j);
		sq.InsertInOrder(-j);
		}
	if (sq.Count()!=2047)
		{
		sq.Close();
		return -1;
		}
	for (i=0; i<2047; i++)
		{
		TInt y;
		if (i<1024)
			y=i*i;
		else
			y=-(2047-i)*(2047-i);
		if (sq[i]!=TUint(y))
			{
			sq.Close();
			return -2;
			}
		}
	sq.Close();
	return 0;
	}

LOCAL_C TInt IntSortTest(TInt aCount, TInt aNumTests, TInt aRange)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<TUint> a;
		RArray<TUint> b;
		TInt i;
		for (i=0; i<aCount; i++)
			{
			TUint x=Random()&aRange;
			a.Append(x);
			b.InsertInOrderAllowRepeats(x);
			}
		a.Sort();
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

TInt UIntSpecificFindTests(TInt aCount, TInt aNumTests, TInt aRange)
	{
	TInt n;
	TInt nmiss = 0;
	TInt nrpt = 0;
	TInt ntot = 0;
	for (n=0; n<aNumTests; n++)
		{
		RArray<TUint> a;
		RArray<TUint> b;
		TInt i;
		for (i=0; i<aCount; i++)
			{
			TInt x=Random()&aRange;
			x-=(aRange>>1);
			a.Append(TUint(x));
			b.InsertInOrderAllowRepeats(TUint(x));
			}
		a.Sort();
		test(a.Count()==aCount);
		test(b.Count()==aCount);
		for (i=0; i<aCount; i++)
			test(a[i]==b[i]);
		for (i=-(aRange>>1); i<=(aRange>>1); ++i)
			{
			TUint u = (TUint)i;
			TInt first = a.SpecificFindInOrder(u, EArrayFindMode_First);
			TInt last = a.SpecificFindInOrder(u, EArrayFindMode_Last);
			TInt any = a.SpecificFindInOrder(u, EArrayFindMode_Any);
			TInt fi, li, ai;
			TInt first2 = a.SpecificFindInOrder(u, fi, EArrayFindMode_First);
			TInt last2 = a.SpecificFindInOrder(u, li, EArrayFindMode_Last);
			TInt any2 = a.SpecificFindInOrder(u, ai, EArrayFindMode_Any);
			++ntot;
			if (first < 0)
				{
				test(first == KErrNotFound);
				test(first == last);
				test(first == any);
				test(first == first2);
				test(first == last2);
				test(first == any2);
				test(fi == li);
				test(fi == ai);
				test(li==aCount || a[li]>u);
				test(li==0 || a[li-1]<u);
				++nmiss;
				}
			else
				{
				test(first2 == KErrNone);
				test(last2 == KErrNone);
				test(any2 == KErrNone);
				test(first == fi);
				test(last == li);
				test(any == ai);
				test(a[fi] == u);
				test(a[li-1] == u);
				test(li==aCount || a[li]>u);
				test(ai>=fi && ai<li);
				test(a[ai] == u);
				if (li-fi > 1)
					++nrpt;
				}
			}
		a.Close();
		b.Close();
		}
	test.Printf(_L("ntot=%d nmiss=%d nrpt=%d\n"), ntot, nmiss, nrpt);
	return KErrNone;
	}

GLDEF_C void DoUintArrayTests()
	{
	test.Start(_L("Unsigned Integer Arrays..."));

	test.Next(_L("AppendAndAccess tests..."));
	test.Next(_L("Count 10 Range 15"));
	test(IntAppendAndAccessTest(10,NUM_TESTS,15)==KErrNone);
	test.Next(_L("Count 20 Range 255"));
	test(IntAppendAndAccessTest(20,NUM_TESTS,255)==KErrNone);
	test.Next(_L("Count 100 Range all"));
	test(IntAppendAndAccessTest(100,NUM_TESTS,-1)==KErrNone);

	test.Next(_L("Remove tests..."));
	test(IntRemoveTest()==KErrNone);

	test.Next(_L("Find tests..."));
	test.Next(_L("Count 10 Range 15"));
	test(IntFindTest(10,NUM_TESTS,15)==KErrNone);
	test.Next(_L("Count 20 Range 255"));
	test(IntFindTest(20,NUM_TESTS,255)==KErrNone);
	test.Next(_L("Count 100 Range all"));
	test(IntFindTest(100,NUM_TESTS,-1)==KErrNone);

	test.Next(_L("FindInOrder tests..."));
	test.Next(_L("Count 10 Range 15"));
	test(IntFindInOrderTest(10,NUM_TESTS,15)==KErrNone);
	test.Next(_L("Count 20 Range 255"));
	test(IntFindInOrderTest(20,NUM_TESTS,255)==KErrNone);
	test.Next(_L("Count 100 Range 4095"));
	test(IntFindInOrderTest(100,NUM_TESTS,4095)==KErrNone);
	test.Next(_L("Squares"));
	test(IntFindInOrderTest2()==KErrNone);

	test.Next(_L("InsertInOrder tests..."));
	test.Next(_L("Count 10 Range 15"));
	test(IntInsertInOrderTest(10,NUM_TESTS,15)==KErrNone);
	test.Next(_L("Count 20 Range 255"));
	test(IntInsertInOrderTest(20,NUM_TESTS,255)==KErrNone);
	test.Next(_L("Count 100 Range all"));
	test(IntInsertInOrderTest(100,NUM_TESTS,-1)==KErrNone);
	test.Next(_L("Squares"));
	test(IntInsertInOrderTest2()==KErrNone);

	test.Next(_L("Sort tests..."));
	test.Next(_L("Count 10 Range 15"));
	test(IntSortTest(10,NUM_TESTS,15)==KErrNone);
	test.Next(_L("Count 20 Range 255"));
	test(IntSortTest(20,NUM_TESTS,255)==KErrNone);
	test.Next(_L("Count 100 Range all"));
	test(IntSortTest(100,NUM_TESTS,-1)==KErrNone);

	test.Next(_L("UIntSpecificFindTests..."));
	test(UIntSpecificFindTests(100, 10, 15)==KErrNone);
	test(UIntSpecificFindTests(100, 10, 127)==KErrNone);

	test.End();
	}
