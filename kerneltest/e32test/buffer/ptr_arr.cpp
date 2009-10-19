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
// e32test\buffer\ptr_arr.cpp
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

struct SEntry
	{
	TInt iKey;
	TInt iValue;
	inline TBool operator!=(const SEntry& anEntry) const
		{return (iValue!=anEntry.iValue || iKey!=anEntry.iKey);}
	inline TBool operator==(const SEntry& anEntry) const
		{return (iValue==anEntry.iValue && iKey==anEntry.iKey);}
	};

struct SPointerArray
	{
	TInt iCount;
	TAny** iEntries;
	TInt iAllocated;
	TInt iGranularity;
	};

LOCAL_D TInt64* Int64s;
LOCAL_D SEntry* Entries;

LOCAL_C TInt Order64Bit(const TInt64& aLeft, const TInt64& aRight)
	{
	if (aLeft<aRight)
		return -1;
	if (aLeft==aRight)
		return 0;
	return 1;
	}

LOCAL_C TBool Compare64Bit(const TInt64& aLeft, const TInt64& aRight)
	{
	return (aLeft==aRight);
	}

TInt OrderSEntry(const SEntry& aLeft, const SEntry& aRight)
	{
	if (aLeft.iKey<aRight.iKey)
		return -1;
	else if (aLeft.iKey>aRight.iKey)
		return 1;
	else
		return 0;
	}

TInt OrderSEntryU(const SEntry& aLeft, const SEntry& aRight)
	{
	TUint l = (TUint)aLeft.iKey;
	TUint r = (TUint)aRight.iKey;
	if (l < r)
		return -1;
	else if (l > r)
		return 1;
	else
		return 0;
	}

TInt OrderSEntry2(const SEntry& aLeft, const SEntry& aRight)
	{
	if (aLeft.iKey<aRight.iKey)
		return -1;
	else if (aLeft.iKey>aRight.iKey)
		return 1;
	else
		return aLeft.iValue - aRight.iValue;
	}

TInt OrderSEntryU2(const SEntry& aLeft, const SEntry& aRight)
	{
	TUint l = (TUint)aLeft.iKey;
	TUint r = (TUint)aRight.iKey;
	if (l < r)
		return -1;
	else if (l > r)
		return 1;
	else
		return aLeft.iValue - aRight.iValue;
	}

TInt OrderSEntryByKey(const TInt* aKey, const SEntry& aObject)
	{
	if (*aKey<aObject.iKey)
		return -1;
	else if (*aKey>aObject.iKey)
		return 1;
	else
		return 0;
	}

LOCAL_C TBool CompareSEntryKey(const SEntry& aLeft, const SEntry& aRight)
	{
	return (aLeft.iKey==aRight.iKey);
	}

LOCAL_C TBool CompareSEntry(const SEntry& aLeft, const SEntry& aRight)
	{
	return (aLeft.iKey==aRight.iKey && aLeft.iValue==aRight.iValue);
	}

TBool CompareSEntryByKeyKey(const TInt* aKey, const SEntry& aRight)
	{
	return *aKey==aRight.iKey;
	}

TBool CompareSEntryByKeyValue(const TInt* aValue, const SEntry& aRight)
	{
	return *aValue==aRight.iValue;
	}

GLDEF_D TLinearOrder<TInt64> Int64Order(Order64Bit);
GLDEF_D TIdentityRelation<TInt64> Int64Identity(Compare64Bit);
GLDEF_D TLinearOrder<SEntry> SEntryOrder(OrderSEntry);
GLDEF_D TIdentityRelation<SEntry> SEntryKeyIdentity(CompareSEntryKey);
GLDEF_D TIdentityRelation<SEntry> SEntryIdentity(CompareSEntry);

LOCAL_C TInt64 Random64(TInt64& aMask)
	{
	TInt64 x = MAKE_TINT64(Random()&I64HIGH(aMask), Random()&I64LOW(aMask));
	return x;
	}

LOCAL_C TInt IntAppendAndAccessTest(TInt aCount, TInt aNumTests, TInt64 aMask)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RPointerArray<TInt64> a;
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
			if (*a[i]!=pA[i])
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
			if (*a[i]!=Int64s[i])
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

LOCAL_C TInt IntFindTest(TInt aCount, TInt aNumTests, TInt64 aMask)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RPointerArray<TInt64> a;
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
			r=a.Find(&pA[i],Int64Identity);
			if (r<0 || pA[i]!=Int64s[i] || r>i)
				{
				a.Close();
				return -4;
				}
			}
		delete[] pA;
		a.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt EntryFindTest(TInt aCount, TInt aNumTests)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RPointerArray<SEntry> a;
		SEntry *pE=new SEntry[aCount];
		if (!pE)
			{
			a.Close();
			return -65535;
			}
		TInt i;
		for (i=0; i<aCount; i++)
			{
			pE[i].iValue=Random();
			pE[i].iKey=i&7;
			Entries[i]=pE[i];
			a.Append(&Entries[i]);
			}
		if (a.Count()!=aCount)
			{
			a.Close();
			return -1;
			}
		for (i=0; i<aCount; i++)
			{
			TInt r=a.Find(&Entries[i]);
			if (r!=i)
				{
				a.Close();
				return -2;
				}
			r=a.Find(&pE[i]);
			if (r>=0)
				{
				a.Close();
				return -3;
				}
			r=a.Find(&pE[i],SEntryIdentity);
			if (r<0 || pE[i]!=Entries[i] || r>i)
				{
				a.Close();
				return -4;
				}
			r=a.Find(&pE[i],SEntryKeyIdentity);
			if (r!=(i&7))
				{
				a.Close();
				return -5;
				}
			r=a.Find(pE[i].iValue,CompareSEntryByKeyValue);
			if (r<0 || pE[i].iValue!=Entries[i].iValue || r>i)
				{
				a.Close();
				return -6;
				}
			r=a.Find(pE[i].iKey,CompareSEntryByKeyKey);
			if (r!=(i&7))
				{
				a.Close();
				return -7;
				}	
			}
		delete[] pE;
		a.Close();
		}
	return KErrNone;
	}
	

LOCAL_C TInt IntFindReverseTest(TInt aCount, TInt aNumTests, TInt64 aMask)
	{
	TInt n;
	TInt i;

	for (n=0; n<aNumTests; n++)
		{
		RPointerArray<TInt64> a;
		TInt64 *pA=new TInt64[aCount];
		if (!pA)
			{
			a.Close();
			return -65535;
			}
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
			TInt r=a.FindReverse(&Int64s[i]);
			if (r!=i)
				{
				a.Close();
				return -2;
				}
			r=a.FindReverse(&pA[i]);
			if (r>=0)
				{
				a.Close();
				return -3;
				}
			r=a.FindReverse(&pA[i],Int64Identity);
			if (pA[i]!=Int64s[i] || r<i)
				{
				a.Close();
				return -4;
				}
			}
		delete[] pA;
		a.Close();
		}

	return KErrNone;
	}
	

LOCAL_C TInt EntryFindReverseTest(TInt aCount, TInt aNumTests)
	{
	TInt n;
	TInt revOffs = aCount-(((aCount-1)&7)+1);

	for (n=0; n<aNumTests; n++)
		{
		RPointerArray<SEntry> a;
		SEntry *pE=new SEntry[aCount];
		if (!pE)
			{
			a.Close();
			return -65535;
			}
		TInt i;
		for (i=0; i<aCount; i++)
			{
			pE[i].iValue=Random();
			pE[i].iKey=((aCount-1)-i)&7;
			Entries[i]=pE[i];
			a.Append(&Entries[i]);
			}
		if (a.Count()!=aCount)
			{
			a.Close();
			return -1;
			}
		for (i=0; i<aCount; i++)
			{
			TInt r=a.FindReverse(&Entries[i]);
			if (r!=i)
				{
				a.Close();
				return -2;
				}
			r=a.FindReverse(&pE[i]);
			if (r>=0)
				{
				a.Close();
				return -3;
				}
			r=a.FindReverse(&pE[i],SEntryIdentity);
			if (pE[i]!=Entries[i] || r<i)
				{
				a.Close();
				return -4;
				}
			r=a.FindReverse(&pE[i],SEntryKeyIdentity);
			if (r!=revOffs+(i&7))
				{
				a.Close();
				return -5;
				}
			r=a.FindReverse(pE[i].iValue,CompareSEntryByKeyValue);
			if (pE[i].iValue!=Entries[i].iValue || r<i)
				{
				a.Close();
				return -6;
				}
			r=a.FindReverse(pE[i].iKey,CompareSEntryByKeyKey);
			if (r!=revOffs+(i&7))
				{
				a.Close();
				return -7;
				}	
			}
		delete[] pE;
		a.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt IntFindInOrderTest(TInt aCount, TInt aNumTests, TInt64 aMask)
// require aRange*aCount<2^32
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RPointerArray<TInt64> a;
		TInt64 *pA=new TInt64[aCount];
		if (!pA)
			{
			a.Close();
			return -65535;
			}
		TInt i=0;
		TInt64 y=-256;
		for(i=0; i<aCount; i++)
			{
			TInt64 x=Random64(aMask);	// this is always >=0
			pA[i]=y;
			Int64s[i]=y;
			y+=x;
			a.Append(&Int64s[i]);
			}
		if (a.Count()!=aCount)
			{
			a.Close();
			return -1;
			}
		for (i=0; i<aCount; i++)
			{
			TInt r=a.FindInOrder(&pA[i],Int64Order);
			if (r<0 || Int64s[r]!=pA[i])
				{
				a.Close();
				return -2;
				}
			TInt64 x=Random64(aMask);
			r=a.FindInOrder(&x,Int64Order);
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
			else if (Int64s[r]!=x)
				{
				a.Close();
				return -4;
				}
			}
		delete[] pA;
		a.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt EntryFindInOrderTest(TInt aCount, TInt aNumTests)
// require aRange*aCount<2^32
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RPointerArray<SEntry> a;
		SEntry *pE=new SEntry[aCount];
		if (!pE)
			{
			a.Close();
			return -65535;
			}
		TInt i=0;
		for(i=0; i<aCount; i++)
			{
			pE[i].iKey=i*19;
			pE[i].iValue=Random();
			Entries[i]=pE[i];
			a.Append(&Entries[i]);
			}
		if (a.Count()!=aCount)
			{
			a.Close();
			return -1;
			}
		for (i=0; i<aCount; i++)
			{
			TInt r=a.FindInOrder(&pE[i],SEntryOrder);
			if (r!=i)
				{
				a.Close();
				return -2;
				}
			TInt x=Random()&1023;
			SEntry e;
			e.iKey=x;
			r=a.FindInOrder(&e,SEntryOrder);
			if (r<0)
				{
				if (x%19==0)
					{
					a.Close();
					return -3;
					}
				}
			else if (x!=r*19)
				{
				a.Close();
				return -4;
				}
			r=a.FindInOrder(x,OrderSEntryByKey);
			if (r<0)
				{
				if (x%19==0)
					{
					a.Close();
					return -5;
					}
				}
			else if (x!=r*19)
				{
				a.Close();
				return -6;
				}
			}
		delete[] pE;
		a.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt IntInsertInOrderTest(TInt aCount, TInt aNumTests, TInt64 aMask)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RPointerArray<TInt64> a;
		RPointerArray<TInt64> b;
		RPointerArray<TInt64> c;
		TInt i;
		TInt cc=0;
		for (i=0; i<aCount; i++)
			{
			Int64s[i]=Random64(aMask);
			a.Append(&Int64s[i]);
			b.InsertInOrderAllowRepeats(&Int64s[i],Int64Order);
			TInt r=c.InsertInOrder(&Int64s[i],Int64Order);
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
			if (*b[i]>*b[i+1])
				{
				a.Close();
				b.Close();
				c.Close();
				return -3;
				}
			}
		for (i=0; i<aCount; i++)
			{
			if (a.Find(b[i],Int64Identity)<0)
				{
				a.Close();
				b.Close();
				c.Close();
				return -4;
				}
			if (b.Find(a[i],Int64Identity)<0)
				{
				a.Close();
				b.Close();
				c.Close();
				return -5;
				}
			if (c.Find(a[i],Int64Identity)<0)
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
			if (*c[i]>=*c[i+1])
				{
				a.Close();
				b.Close();
				c.Close();
				return -8;
				}
			if (a.Find(c[i],Int64Identity)<0)
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

LOCAL_C TInt SEntrySpecificFindTests(TInt aCount, TInt aNumTests, TInt aRange)
	{
	TInt n;
	TInt nmiss = 0;
	TInt nrpt = 0;
	TInt ntot = 0;
	for (n=0; n<aNumTests; n++)
		{
		RPointerArray<SEntry> a;
		RPointerArray<SEntry> b;
		RPointerArray<SEntry> c;
		RPointerArray<SEntry> d;
		TInt i;
		for (i=0; i<aCount; i++)
			{
			TInt x=Random()&aRange;
			x-=(aRange>>1);
			SEntry& e = Entries[i];
			e.iKey = x;
			e.iValue = i;
			a.Append(&e);
			c.Append(&e);
			b.InsertInOrderAllowRepeats(&e, &OrderSEntry);
			d.InsertInOrderAllowRepeats(&e, &OrderSEntryU);
			}
		a.Sort(&OrderSEntry2);
		c.Sort(&OrderSEntryU2);
		test(a.Count()==aCount);
		test(b.Count()==aCount);
		test(c.Count()==aCount);
		test(d.Count()==aCount);
		for (i=0; i<aCount; i++)
			{
			test(a[i]==b[i]);
			test(c[i]==d[i]);
			}
		for (i=-(aRange>>1); i<=(aRange>>1); ++i)
			{
			SEntry es;
			es.iKey = i;
			TInt first = a.SpecificFindInOrder(&es, &OrderSEntry, EArrayFindMode_First);
			TInt last = a.SpecificFindInOrder(&es, &OrderSEntry, EArrayFindMode_Last);
			TInt any = a.SpecificFindInOrder(&es, &OrderSEntry, EArrayFindMode_Any);
			TInt fi, li, ai;
			TInt first2 = a.SpecificFindInOrder(&es, fi, &OrderSEntry, EArrayFindMode_First);
			TInt last2 = a.SpecificFindInOrder(&es, li, &OrderSEntry, EArrayFindMode_Last);
			TInt any2 = a.SpecificFindInOrder(&es, ai, &OrderSEntry, EArrayFindMode_Any);
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
				test(li==aCount || a[li]->iKey>i);
				test(li==0 || a[li-1]->iKey<i);
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
				test(a[fi]->iKey == i);
				test(a[li-1]->iKey == i);
				test(li==aCount || a[li]->iKey>i);
				test(ai>=fi && ai<li);
				test(a[ai]->iKey == i);
				if (li-fi > 1)
					{
					++nrpt;
					TInt j;
					for (j=fi+1; j<li; ++j)
						test(a[j]->iValue > a[j-1]->iValue);
					}
				}
			}
		for (i=-(aRange>>1); i<=(aRange>>1); ++i)
			{
			TUint u = (TUint)i;
			SEntry eu;
			eu.iKey = i;
			TInt first = c.SpecificFindInOrder(&eu, &OrderSEntryU, EArrayFindMode_First);
			TInt last = c.SpecificFindInOrder(&eu, &OrderSEntryU, EArrayFindMode_Last);
			TInt any = c.SpecificFindInOrder(&eu, &OrderSEntryU, EArrayFindMode_Any);
			TInt fi, li, ai;
			TInt first2 = c.SpecificFindInOrder(&eu, fi, &OrderSEntryU, EArrayFindMode_First);
			TInt last2 = c.SpecificFindInOrder(&eu, li, &OrderSEntryU, EArrayFindMode_Last);
			TInt any2 = c.SpecificFindInOrder(&eu, ai, &OrderSEntryU, EArrayFindMode_Any);
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
				test(li==aCount || TUint(c[li]->iKey)>u);
				test(li==0 || TUint(c[li-1]->iKey)<u);
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
				test(c[fi]->iKey == i);
				test(c[li-1]->iKey == i);
				test(li==aCount || TUint(c[li]->iKey)>u);
				test(ai>=fi && ai<li);
				test(c[ai]->iKey == i);
				if (li-fi > 1)
					{
					++nrpt;
					TInt j;
					for (j=fi+1; j<li; ++j)
						test(c[j]->iValue > c[j-1]->iValue);
					}
				}
			}
		a.Close();
		b.Close();
		c.Close();
		d.Close();
		}
	test.Printf(_L("ntot=%d nmiss=%d nrpt=%d\n"), ntot, nmiss, nrpt);
	return KErrNone;
	}

LOCAL_C TInt EntryInsertInOrderTest()
	{
	RPointerArray<SEntry> b;
	RPointerArray<SEntry> c;
	TInt i;
	for (i=0; i<1024; i++)
		{
		SEntry e;
		e.iValue=i;
		e.iKey=i&31;
		Entries[i]=e;
		b.InsertInOrderAllowRepeats(&Entries[i],SEntryOrder);
		c.InsertInOrder(&Entries[i],SEntryOrder);
		}
	if (b.Count()!=1024)
		{
		b.Close();
		c.Close();
		return -1;
		}
	for (i=0; i<1024; i++)
		{
		SEntry e=*b[i];
		if (e.iKey!=i>>5)
			{
			b.Close();
			c.Close();
			return -2;
			}
		if (e.iValue!=((i&31)<<5 | (i>>5)))
			{
			b.Close();
			c.Close();
			return -3;
			}
		}
	if (c.Count()!=32)
		{
		b.Close();
		c.Close();
		return -4;
		}
	for (i=0; i<31; i++)
		{
		SEntry e=*c[i];
		if (e.iKey!=i)
			{
			b.Close();
			c.Close();
			return -5;
			}
		if (e.iValue!=i)
			{
			b.Close();
			c.Close();
			return -6;
			}
		}
	b.Close();
	c.Close();
	return KErrNone;
	}

LOCAL_C TInt IntSortTest(TInt aCount, TInt aNumTests, TInt64 aMask)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RPointerArray<TInt64> a;
		RPointerArray<TInt64> b;
		TInt i;
		for (i=0; i<aCount; i++)
			{
			Int64s[i]=Random64(aMask);
			a.Append(&Int64s[i]);
			b.InsertInOrderAllowRepeats(&Int64s[i],Int64Order);
			}
		a.Sort(Int64Order);
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
			if (*a[i]!=*b[i])
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

LOCAL_C void TestGrowCompress(RPointerArray<TInt64>* a, ...)
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
	RPointerArray<TInt64> a;
	TestGrowCompress(&a, 1, 8, 7, 8, 1, 16, 7, 16, 1, 24, -2, 24, -1, 17, 1, 25, -2, 25, -3, 25, -2, 24, -99);
	TestGrowCompress(&a, 1, 8, 7, 8, 1, 16, 7, 16, 1, 24, -2, 24, -1, 17, 1, 25, -2, 25, -3, 25, -3, 25, -2, 16, -99);

	RPointerArray<TInt64> b(100);
	TestGrowCompress(&b, 1, 100, 99, 100, 1, 200, 99, 200, 1, 300, -2, 300, -1, 201, 1, 301, -2, 301, -3, 301, -2, 300, -99);
	TestGrowCompress(&b, 1, 100, 99, 100, 1, 200, 99, 200, 1, 300, -2, 300, -1, 201, 1, 301, -2, 301, -3, 301, -3, 301, -2, 200, -99);

	RPointerArray<TInt64> c(8, 512);
	TestGrowCompress(&c, 1, 8, 7, 8, 1, 16, 7, 16, 1, 32, 15, 32, 1, 64, -2, 40, 7, 40, 1, 80, -1, 41, -99);

	RPointerArray<TInt64> d(20, 640);
	TestGrowCompress(&d, 1, 20, 19, 20, 1, 50, 29, 50, 1, 125, -2, 60, -1, 51, -99);

	RPointerArray<TInt64> e(8, 320);
	TestGrowCompress(&e, 1, 8, 7, 8, 1, 16, 7, 16, 1, 24, 7, 24, 1, 32, 7, 32, 1, 40, 7, 40, 1, 50, 9, 50, 1, 63, -99);

	RPointerArray<TInt64> f(2, 257);
	TestGrowCompress(&f, 1, 2, 255, 256, 256, 512, 128, 640, 1, 643, 2, 643, 1, 646, -99);
	}

GLDEF_C void DoPointerArrayTests()
	{
	test.Start(_L("Pointer Arrays..."));
	test.Next(_L("Allocate memory"));
	Int64s=new TInt64[KArraySize];
	Entries=new SEntry[KArraySize];
	test(Int64s && Entries);

	test.Next(_L("AppendAndAccess tests..."));
	test.Next(_L("Count 10 Mask 0x0000000300000003"));
	test(IntAppendAndAccessTest(10,NUM_TESTS,MAKE_TINT64(0x3,0x3))==KErrNone);
	test.Next(_L("Count 100 Range all"));
	test(IntAppendAndAccessTest(100,NUM_TESTS,MAKE_TINT64(0xffffffff,0xffffffff))==KErrNone);

	test.Next(_L("Find tests..."));
	test.Next(_L("Count 10 Mask 0x0000000300000003"));
	test(IntFindTest(10,NUM_TESTS,MAKE_TINT64(3,3))==KErrNone);
	test.Next(_L("Count 100 Range all"));
	test(IntFindTest(100,NUM_TESTS,MAKE_TINT64(0xffffffff,0xffffffff))==KErrNone);
	test.Next(_L("SEntry find tests"));
	test(EntryFindTest(128,NUM_TESTS)==KErrNone);
	
	test.Next(_L("FindReverse tests..."));
	test.Next(_L("Count 10 Mask 0x0000000300000003"));
	test(IntFindReverseTest(10,NUM_TESTS,MAKE_TINT64(3,3))==KErrNone);
	test.Next(_L("Count 100 Range all"));
	test(IntFindReverseTest(100,NUM_TESTS,MAKE_TINT64(0xffffffff,0xffffffff))==KErrNone);
	test.Next(_L("SEntry find tests"));

	test(EntryFindReverseTest(128,NUM_TESTS)==KErrNone);

	test.Next(_L("FindInOrder tests..."));
	test.Next(_L("Count 20 Mask 0x00000003C0000000"));
	test(IntFindInOrderTest(20,NUM_TESTS,MAKE_TINT64(0x3,0xc0000000))==KErrNone);
	test.Next(_L("Count 100 Mask 0x0000000FF0000000"));
	test(IntFindInOrderTest(100,NUM_TESTS,MAKE_TINT64(0xf,0xf0000000))==KErrNone);
	test.Next(_L("SEntry FindInOrder test"));
	test(EntryFindInOrderTest(128,NUM_TESTS)==KErrNone);

	test.Next(_L("InsertInOrder tests..."));
	test.Next(_L("Count 50 Mask 0x00000003C0000000"));
	test(IntInsertInOrderTest(50,NUM_TESTS,MAKE_TINT64(0x3,0xc0000000))==KErrNone);
	test.Next(_L("Count 100 all"));
	test(IntInsertInOrderTest(100,NUM_TESTS,MAKE_TINT64(0xffffffff,0xffffffff))==KErrNone);
	test.Next(_L("SEntry InsertInOrder test"));
	test(EntryInsertInOrderTest()==KErrNone);

	test.Next(_L("Sort tests..."));
	test.Next(_L("Count 30 Mask 0x00000003C0000000"));
	test(IntSortTest(30,NUM_TESTS,MAKE_TINT64(0x3,0xc0000000))==KErrNone);
	test.Next(_L("Count 100 all"));
	test(IntSortTest(100,NUM_TESTS,MAKE_TINT64(0xffffffff,0xffffffff))==KErrNone);

	test.Next(_L("SEntrySpecificFindTests..."));
	test(SEntrySpecificFindTests(100, 10, 15)==KErrNone);
	test(SEntrySpecificFindTests(100, 10, 127)==KErrNone);

	test.Next(_L("Test Grow/Compress"));
	TestGrowCompress();

	test.Next(_L("Test RPointerArray::Array..."));
	TInt a;
	TInt b;
	TInt c;
	RPointerArray<TInt> ptrArr;
	ptrArr.Append(&a);
	ptrArr.Append(&b);
	ptrArr.Append(&c);

	TArray<TInt*> arr=ptrArr.Array();
	test(arr.Count()==3);
	test(arr[0]==&a);
	test(arr[1]==&b);
	test(arr[2]==&c);

	ptrArr.Reset();

	delete[] Int64s;
	delete[] Entries;
	test.End();
	}

GLDEF_C void DoPointerArrayLeavingInterfaceTest()
	{
	TInt trap, ret(0);
	TInt64 Int64s[3];
	for (TInt i=0;i<3;i++) Int64s[i] = i;

	RPointerArray<TInt64> pArray;
	CleanupClosePushL(pArray);

	test.Start(_L("Checking Leaving Pointer Arrays Interface..."));

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
	
	test.Next(_L("Test FindL(const T* anEntry, TIdentityRelation<T> anIdentity) const..."));
	TRAP(trap, ret = pArray.FindL(&Int64s[0],Int64Identity));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = pArray.FindL(&Int64s[2],Int64Identity));
	test(trap==KErrNotFound);

	test.Next(_L("Test FindInAddressOrderL(const T* anEntry) const..."));
	TRAP(trap, ret = pArray.FindInAddressOrderL(&Int64s[0]));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = pArray.FindInAddressOrderL(&Int64s[2]));
	test(trap==KErrNotFound);

	test.Next(_L("Test FindInOrderL(const T* anEntry, TLinearOrder<T> anOrder) const..."));
	TRAP(trap, ret = pArray.FindInOrderL(&Int64s[0], Int64Order));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = pArray.FindInOrderL(&Int64s[2], Int64Order));
	test(trap==KErrNotFound);

	test.Next(_L("Test FindInAddressOrderL(const T* anEntry, TInt& anIndex) const..."));
	TRAP(trap, pArray.FindInAddressOrderL(&Int64s[0], ret));
	test(trap==0);
	test(ret==0);
	TRAP(trap, pArray.FindInAddressOrderL(&Int64s[2], ret));
	test(trap==KErrNotFound);

	test.Next(_L("Test FindInOrderL(const T* anEntry, TInt& anIndex, TLinearOrder<T> anOrder) const..."));
	TRAP(trap, pArray.FindInOrderL(&Int64s[0], ret, Int64Order));
	test(trap==0);
	test(ret==0);
	TRAP(trap, pArray.FindInOrderL(&Int64s[2], ret, Int64Order));
	test(trap==KErrNotFound);

	test.Next(_L("Test SpecificFindInAddressOrderL(const T* anEntry, TInt aMode) const..."));
	TRAP(trap, ret = pArray.SpecificFindInAddressOrderL(&Int64s[0], EArrayFindMode_First));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = pArray.SpecificFindInAddressOrderL(&Int64s[2], EArrayFindMode_First));
	test(trap==KErrNotFound);

	test.Next(_L("Test SpecificFindInOrderL(const T* anEntry, TLinearOrder<T> anOrder, TInt aMode) const..."));
	TRAP(trap, ret = pArray.SpecificFindInOrderL(&Int64s[0], Int64Order, EArrayFindMode_First));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = pArray.SpecificFindInOrderL(&Int64s[2], Int64Order, EArrayFindMode_First));
	test(trap==KErrNotFound);

	test.Next(_L("Test SpecificFindInAddressOrderL(const T* anEntry, TInt& anIndex, TInt aMode) const..."));
	TRAP(trap, pArray.SpecificFindInAddressOrderL(&Int64s[0], ret, EArrayFindMode_First));
	test(trap==0);
	test(ret==0);
	TRAP(trap, pArray.SpecificFindInAddressOrderL(&Int64s[2], ret, EArrayFindMode_First));
	test(trap==KErrNotFound);

	test.Next(_L("Test SpecificFindInOrderL(const T* anEntry, TInt& anIndex, TLinearOrder<T> anOrder, TInt aMode) const..."));
	TRAP(trap, pArray.SpecificFindInOrderL(&Int64s[0], ret, Int64Order, EArrayFindMode_First));
	test(trap==0);
	test(ret==0);
	TRAP(trap, pArray.SpecificFindInOrderL(&Int64s[2], ret, Int64Order, EArrayFindMode_First));
	test(trap==KErrNotFound);

	test.Next(_L("Test InsertInAddressOrderL(const T* anEntry)..."));
	TRAP(trap, pArray.InsertInAddressOrderL(&Int64s[0]));
	test(trap==KErrAlreadyExists);
	TRAP(trap, pArray.InsertInAddressOrderL(&Int64s[2]));
	test(trap==KErrNone);
	pArray.Remove(2);

	test.Next(_L("Test InsertInOrderL(const T* anEntry, TLinearOrder<T> anOrder)..."));
	TRAP(trap, pArray.InsertInOrderL(&Int64s[0], Int64Order));
	test(trap==KErrAlreadyExists);
	TRAP(trap, pArray.InsertInOrderL(&Int64s[2], Int64Order));
	test(trap==KErrNone);
	pArray.Remove(2);
	
	test.Next(_L("Test InsertInAddressOrderAllowRepeatsL(const T* anEntry)..."));
	TRAP(trap, pArray.InsertInAddressOrderAllowRepeatsL(&Int64s[2]));
	test(trap==KErrNone);
	pArray.Remove(2);

	test.Next(_L("Test InsertInOrderAllowRepeatsL(const T* anEntry, TLinearOrder<T> anOrder)..."));
	TRAP(trap, pArray.InsertInOrderAllowRepeatsL(&Int64s[2], Int64Order));
	test(trap==KErrNone);
	pArray.Remove(2);
	
	CleanupStack::PopAndDestroy(&pArray);
	test.End();
	}
