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
// e32test\buffer\int_arr.cpp
// 
//

#include <e32test.h>
#include <e32math.h>

#define NUM_TESTS 500

GLREF_D RTest test;
GLREF_C TInt Random();

struct SInt
	{
	SInt(TInt a) {i=a;}
	operator TInt() {return i;}

	TInt i;
	};

LOCAL_C TInt IntAppendAndAccessTest(TInt aCount, TInt aNumTests, TInt aRange)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<TInt> a;
		TInt *pA=(TInt*)User::Alloc(aCount*sizeof(TInt));
		if (!pA)
			{
			a.Close();
			return -65535;
			}
		TInt i;
		for (i=0; i<aCount; i++)
			{
			TInt x=Random()&aRange;
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
			TInt x=Random()&aRange;
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
	RArray<TInt> a;
	TInt i;
	for (i=0; i<n; i++)
		{
		a.Append(i);
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
		if (a[i]!=i*i)
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
		RArray<TInt> a;
		TInt *pA=(TInt*)User::Alloc(aCount*sizeof(TInt));
		if (!pA)
			{
			a.Close();
			return -65535;
			}
		TInt i;
		for (i=0; i<aCount; i++)
			{
			TInt x=Random()&aRange;
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
			TInt x=Random()&aRange;
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
		RArray<TInt> a;
		TInt *pA=(TInt*)User::Alloc(aCount*sizeof(TInt));
		if (!pA)
			{
			a.Close();
			return -65535;
			}
		TInt i=0;
		TInt y=-256;
		for(i=0; i<aCount; i++)
			{
			TInt x=Random()&aRange;	// this is always >=0
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
			TInt x=Random()&aRange;
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
	RArray<TInt> sq;
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
		RArray<TInt> a;
		RArray<TInt> b;
		RArray<TInt> c;
		TInt i;
		TInt cc=0;
		for (i=0; i<aCount; i++)
			{
			TInt x=Random()&aRange;
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
	RArray<TInt> sq;
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
		TInt y=0;
		if (i<1023)
			y=-(1023-i)*(1023-i);
		else if (i==1023)
			y=0;
		else if (i>1023)
			y=(i-1023)*(i-1023);
		if (sq[i]!=y)
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
		RArray<TInt> a;
		RArray<TInt> b;
		TInt i;
		for (i=0; i<aCount; i++)
			{
			TInt x=Random()&aRange;
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

LOCAL_C TInt SIntAppendAndAccessTest(TInt aCount, TInt aNumTests, TInt aRange)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<SInt> a;
		TInt *pA=(TInt*)User::Alloc(aCount*sizeof(TInt));
		if (!pA)
			{
			a.Close();
			return -65535;
			}
		TInt i;
		for (i=0; i<aCount; i++)
			{
			TInt x=Random()&aRange;
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
			TInt x=Random()&aRange;
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

LOCAL_C TInt SIntRemoveTest()
	{
	TInt m=32;
	TInt n=m*m+1;
	RArray<SInt> a;
	TInt i;
	for (i=0; i<n; i++)
		{
		a.Append(i);
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
		if (a[i]!=i*i)
			{
			a.Close();
			return -2;
			}
		}
	a.Close();
	return KErrNone;
	}

LOCAL_C TInt SIntFindTest(TInt aCount, TInt aNumTests, TInt aRange)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<SInt> a;
		TInt *pA=(TInt*)User::Alloc(aCount*sizeof(TInt));
		if (!pA)
			{
			a.Close();
			return -65535;
			}
		TInt i;
		for (i=0; i<aCount; i++)
			{
			TInt x=Random()&aRange;
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
			TInt x=Random()&aRange;
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

LOCAL_C TInt SIntFindInOrderTest(TInt aCount, TInt aNumTests, TInt aRange)
// require aRange*aCount<2^32
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<SInt> a;
		TInt *pA=(TInt*)User::Alloc(aCount*sizeof(TInt));
		if (!pA)
			{
			a.Close();
			return -65535;
			}
		TInt i=0;
		TInt y=-256;
		for(i=0; i<aCount; i++)
			{
			TInt x=Random()&aRange;	// this is always >=0
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
			TInt r=a.FindInSignedKeyOrder(pA[i]);
			if (r<0 || pA[r]!=pA[i])
				{
				a.Close();
				return -2;
				}
			TInt x=Random()&aRange;
			r=a.FindInSignedKeyOrder(x);
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

LOCAL_C TInt SIntFindInOrderTest2()
	{
	TInt i;
	RArray<SInt> sq;
	for (i=0; i<1024; i++)
		{
		TInt j=i*i;
		sq.Append(j);
		}
	for (i=0; i<10000; i++)
		{
		TInt x=Random()&1023;
		TInt y=x*x;
		TInt r=sq.FindInSignedKeyOrder(y);
		if (r!=x)
			{
			sq.Close();
			return -1;
			}
		}
	sq.Close();
	return 0;
	}

LOCAL_C TInt SIntInsertInOrderTest(TInt aCount, TInt aNumTests, TInt aRange)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<SInt> a;
		RArray<SInt> b;
		RArray<SInt> c;
		TInt i;
		TInt cc=0;
		for (i=0; i<aCount; i++)
			{
			TInt x=Random()&aRange;
			a.Append(x);
			b.InsertInSignedKeyOrderAllowRepeats(x);
			TInt r=c.InsertInSignedKeyOrder(x);
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

LOCAL_C TInt SIntInsertInOrderTest2()
	{
	TInt i;
	RArray<SInt> sq;
	for (i=0; i<1024; i++)
		{
		TInt j=i*i;
		sq.InsertInSignedKeyOrder(j);
		sq.InsertInSignedKeyOrder(-j);
		}
	if (sq.Count()!=2047)
		{
		sq.Close();
		return -1;
		}
	for (i=0; i<2047; i++)
		{
		TInt y=0;
		if (i<1023)
			y=-(1023-i)*(1023-i);
		else if (i==1023)
			y=0;
		else if (i>1023)
			y=(i-1023)*(i-1023);
		if (sq[i]!=y)
			{
			sq.Close();
			return -2;
			}
		}
	sq.Close();
	return 0;
	}

LOCAL_C TInt SIntSortTest(TInt aCount, TInt aNumTests, TInt aRange)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<SInt> a;
		RArray<SInt> b;
		TInt i;
		for (i=0; i<aCount; i++)
			{
			TInt x=Random()&aRange;
			a.Append(x);
			b.InsertInSignedKeyOrderAllowRepeats(x);
			}
		a.SortSigned();
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

TInt IntSpecificFindTests1()
	{
	RArray<TInt> a;
	TInt i;
	TInt v = 0;
	TInt first, last, any;
	for (i=0; i<101; ++i)
		{
		if (v*v<i)
			++v;
		TInt r = a.InsertInOrderAllowRepeats(v);
		test(r==KErrNone);
		}
	test (a.Count()==101);
	for (v=0; v<=10; ++v)
		{
		first = a.SpecificFindInOrder(v, EArrayFindMode_First);
		last = a.SpecificFindInOrder(v, EArrayFindMode_Last);
		any = a.SpecificFindInOrder(v, EArrayFindMode_Any);
		test(first>=0 && first<a.Count());
		test(last>=0 && last<=a.Count());
		test(any>=first && any<last);
		if (v==0)
			{
			test(first==0 && last==1);
			}
		else if (v==1)
			{
			test(first==1 && last==2);
			}
		else
			{
			TInt expf = (v-1)*(v-1)+1;
			TInt expl = v*v+1;
			test(expf==first && expl==last);
			}
		TInt n = last - first;
		TInt expected = v ? 2*v-1 : 1;
		test(n == expected);
		}
	first = a.SpecificFindInOrder(11, EArrayFindMode_First);
	test(first == KErrNotFound);
	last = a.SpecificFindInOrder(11, EArrayFindMode_Last);
	test(last == KErrNotFound);
	any = a.SpecificFindInOrder(11, EArrayFindMode_Any);
	test(any == KErrNotFound);
	a.Close();
	return KErrNone;
	}

TInt IntSpecificFindTests2(TInt aCount, TInt aNumTests, TInt aRange)
	{
	TInt n;
	TInt nmiss = 0;
	TInt nrpt = 0;
	TInt ntot = 0;
	for (n=0; n<aNumTests; n++)
		{
		RArray<TInt> a;
		RArray<TInt> b;
		TInt i;
		for (i=0; i<aCount; i++)
			{
			TInt x=Random()&aRange;
			x-=(aRange>>1);
			a.Append(x);
			b.InsertInOrderAllowRepeats(x);
			}
		a.Sort();
		test(a.Count()==aCount);
		test(b.Count()==aCount);
		for (i=0; i<aCount; i++)
			test(a[i]==b[i]);
		for (i=-(aRange>>1); i<=(aRange>>1); ++i)
			{
			TInt first = a.SpecificFindInOrder(i, EArrayFindMode_First);
			TInt last = a.SpecificFindInOrder(i, EArrayFindMode_Last);
			TInt any = a.SpecificFindInOrder(i, EArrayFindMode_Any);
			TInt fi, li, ai;
			TInt first2 = a.SpecificFindInOrder(i, fi, EArrayFindMode_First);
			TInt last2 = a.SpecificFindInOrder(i, li, EArrayFindMode_Last);
			TInt any2 = a.SpecificFindInOrder(i, ai, EArrayFindMode_Any);
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
				test(li==aCount || a[li]>i);
				test(li==0 || a[li-1]<i);
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
				test(a[fi] == i);
				test(a[li-1] == i);
				test(li==aCount || a[li]>i);
				test(ai>=fi && ai<li);
				test(a[ai] == i);
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

TInt SIntSpecificFindTests2(TInt aCount, TInt aNumTests, TInt aRange)
	{
	TInt n;
	TInt nmiss = 0;
	TInt nrpt = 0;
	TInt ntot = 0;
	for (n=0; n<aNumTests; n++)
		{
		RArray<SInt> a;
		RArray<SInt> b;
		TInt i;
		for (i=0; i<aCount; i++)
			{
			TInt x=Random()&aRange;
			x-=(aRange>>1);
			a.Append(x);
			b.InsertInSignedKeyOrderAllowRepeats(x);
			}
		a.SortSigned();
		test(a.Count()==aCount);
		test(b.Count()==aCount);
		for (i=0; i<aCount; i++)
			test(a[i]==b[i]);
		for (i=-(aRange>>1); i<=(aRange>>1); ++i)
			{
			TInt first = a.SpecificFindInSignedKeyOrder(i, EArrayFindMode_First);
			TInt last = a.SpecificFindInSignedKeyOrder(i, EArrayFindMode_Last);
			TInt any = a.SpecificFindInSignedKeyOrder(i, EArrayFindMode_Any);
			TInt fi, li, ai;
			TInt first2 = a.SpecificFindInSignedKeyOrder(i, fi, EArrayFindMode_First);
			TInt last2 = a.SpecificFindInSignedKeyOrder(i, li, EArrayFindMode_Last);
			TInt any2 = a.SpecificFindInSignedKeyOrder(i, ai, EArrayFindMode_Any);
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
				test(li==aCount || a[li]>i);
				test(li==0 || a[li-1]<i);
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
				test(a[fi] == i);
				test(a[li-1] == i);
				test(li==aCount || a[li]>i);
				test(ai>=fi && ai<li);
				test(a[ai] == i);
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

GLDEF_C void DoIntArrayTests()
	{
	test.Start(_L("Signed Integer Arrays..."));

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

	test.End();

	test.Start(_L("4 byte RArrays..."));

	test.Next(_L("AppendAndAccess tests..."));
	test.Next(_L("Count 10 Range 15"));
	test(SIntAppendAndAccessTest(10,NUM_TESTS,15)==KErrNone);
	test.Next(_L("Count 20 Range 255"));
	test(SIntAppendAndAccessTest(20,NUM_TESTS,255)==KErrNone);
	test.Next(_L("Count 100 Range all"));
	test(SIntAppendAndAccessTest(100,NUM_TESTS,-1)==KErrNone);

	test.Next(_L("Remove tests..."));
	test(SIntRemoveTest()==KErrNone);

	test.Next(_L("Find tests..."));
	test.Next(_L("Count 10 Range 15"));
	test(SIntFindTest(10,NUM_TESTS,15)==KErrNone);
	test.Next(_L("Count 20 Range 255"));
	test(SIntFindTest(20,NUM_TESTS,255)==KErrNone);
	test.Next(_L("Count 100 Range all"));
	test(SIntFindTest(100,NUM_TESTS,-1)==KErrNone);

	test.Next(_L("FindInOrder tests..."));
	test.Next(_L("Count 10 Range 15"));
	test(SIntFindInOrderTest(10,NUM_TESTS,15)==KErrNone);
	test.Next(_L("Count 20 Range 255"));
	test(SIntFindInOrderTest(20,NUM_TESTS,255)==KErrNone);
	test.Next(_L("Count 100 Range 4095"));
	test(SIntFindInOrderTest(100,NUM_TESTS,4095)==KErrNone);
	test.Next(_L("Squares"));
	test(SIntFindInOrderTest2()==KErrNone);

	test.Next(_L("InsertInOrder tests..."));
	test.Next(_L("Count 10 Range 15"));
	test(SIntInsertInOrderTest(10,NUM_TESTS,15)==KErrNone);
	test.Next(_L("Count 20 Range 255"));
	test(SIntInsertInOrderTest(20,NUM_TESTS,255)==KErrNone);
	test.Next(_L("Count 100 Range all"));
	test(SIntInsertInOrderTest(100,NUM_TESTS,-1)==KErrNone);
	test.Next(_L("Squares"));
	test(SIntInsertInOrderTest2()==KErrNone);

	test.Next(_L("Sort tests..."));
	test.Next(_L("Count 10 Range 15"));
	test(SIntSortTest(10,NUM_TESTS,15)==KErrNone);
	test.Next(_L("Count 20 Range 255"));
	test(SIntSortTest(20,NUM_TESTS,255)==KErrNone);
	test.Next(_L("Count 100 Range all"));
	test(SIntSortTest(100,NUM_TESTS,-1)==KErrNone);

	test.Next(_L("IntSpecificFindTests..."));
	test(IntSpecificFindTests1()==KErrNone);
	test(IntSpecificFindTests2(100, 10, 15)==KErrNone);
	test(IntSpecificFindTests2(100, 10, 127)==KErrNone);

	test.Next(_L("SIntSpecificFindTests..."));
	test(SIntSpecificFindTests2(100, 10, 15)==KErrNone);
	test(SIntSpecificFindTests2(100, 10, 127)==KErrNone);
	test.End();
	}
