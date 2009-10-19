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
// e32test\buffer\smpl_arr.cpp
// 
//

#include <e32test.h>
#include <e32math.h>
#include <e32std.h>
#include <e32std_private.h>

#define NUM_TESTS 200

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

struct SArray
	{
	TInt iCount;
	TAny* iEntries;
	TInt iEntrySize;
	TInt iKeyOffset;
	TInt iAllocated;
	TInt iGranularity;
	};

GLREF_C TInt OrderSEntry(const SEntry& aLeft, const SEntry& aRight);
GLREF_C TInt OrderSEntryU(const SEntry& aLeft, const SEntry& aRight);
GLREF_C TInt OrderSEntry2(const SEntry& aLeft, const SEntry& aRight);
GLREF_C TInt OrderSEntryU2(const SEntry& aLeft, const SEntry& aRight);
GLREF_C TBool CompareSEntryByKeyKey(const TInt* aKey, const SEntry& aRight);
GLREF_C TBool CompareSEntryByKeyValue(const TInt* aValue, const SEntry& aRight);

GLREF_D TLinearOrder<TInt64> Int64Order;
GLREF_D TIdentityRelation<TInt64> Int64Identity;
GLREF_D TLinearOrder<SEntry> SEntryOrder;
GLREF_D TIdentityRelation<SEntry> SEntryKeyIdentity;
GLREF_D TIdentityRelation<SEntry> SEntryIdentity;

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
		RArray<TInt64> a(aCount);
		TInt64 *pA=new TInt64[aCount];
		if (!pA)
			{
			a.Close();
			return -65535;
			}
		TInt i;
		for (i=0; i<aCount; i++)
			{
			TInt64 x=Random64(aMask);
			pA[i]=x;
			a.Append(x);
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
			pA[i]=Random64(aMask);
			a[i]=pA[i];
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
		delete[] pA;
		a.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt IntRemoveTest()
	{
	TInt m=32;
	TInt n=m*m+1;
	RArray<TInt64> a(n);
	TInt i;
	for (i=0; i<n; i++)
		{
		TInt64 x = MAKE_TINT64(i*i,i);
		a.Append(x);
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
		TInt64 x = MAKE_TINT64(i*i*i*i,i*i);
		if (a[i]!=x)
			{
			a.Close();
			return -2;
			}
		}
	a.Close();
	return KErrNone;
	}

LOCAL_C TInt IntFindTest(TInt aCount, TInt aNumTests, TInt64 aMask)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<TInt64> a(aCount);
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
			a.Append(pA[i]);
			}
		if (a.Count()!=aCount)
			{
			a.Close();
			return -1;
			}
		for (i=0; i<aCount; i++)
			{
			TInt r=a.Find(pA[i],Int64Identity);
			if (r<0 || pA[i]!=pA[r] || r>i)
				{
				a.Close();
				return -2;
				}
			}
		delete[] pA;
		a.Close();
		}
	return KErrNone;
	}

template <class XTestInput>
LOCAL_C TInt FindWithEqualityOp(XTestInput &aObject1, XTestInput &aObject2)
	{
	//Construct a suitable RArray
	RArray<XTestInput> testRArray;

	//Append the test objects to the RArray
	testRArray.AppendL(aObject1);
	testRArray.AppendL(aObject2);
	testRArray.AppendL(aObject1);
	testRArray.AppendL(aObject2);
	
	//Demonstrate that "regular" Find() method returns incorrect result
	TInt pos = KErrNotFound;
	pos = testRArray.Find(aObject1);
	if(pos!=0)
		return KErrNotFound;
	pos = testRArray.Find(aObject2);
	if(pos!=0)
		return KErrNotFound;

	//Test the Find() method using TIdentityRelation default CTOR
	pos = testRArray.Find(aObject1, TIdentityRelation<XTestInput>());
	if(pos!=0)
		return KErrNotFound;
	pos = testRArray.Find(aObject2, TIdentityRelation<XTestInput>());
	if(pos!=1)
		return KErrNotFound;
	
	//Test the FindReverse() method using TIdentityRelation default CTOR
	pos = testRArray.FindReverse(aObject1, TIdentityRelation<XTestInput>());
	if(pos!=2)
		return KErrNotFound;
	pos = testRArray.FindReverse(aObject2, TIdentityRelation<XTestInput>());
	if(pos!=3)
		return KErrNotFound;
	
	//Objects have been found correctly if this point reached
	testRArray.Close();
	
	return KErrNone;
	}

LOCAL_C TInt FindWithEqualityOpTest()
	{
	//Test "complex" objects which would work incorrectly with the regular Find() method

	TPoint p1(0,0);
    TPoint p2(0,1);
    TPoint p3(0,2);
	test(FindWithEqualityOp(p1, p2) == KErrNone);

    TRect rect1(p1,p2);
    TRect rect2(p1,p3);
	test(FindWithEqualityOp(rect1, rect2) == KErrNone);

    TBuf<5> buf1(_L("test1"));
	TBuf<5> buf2(_L("test2"));
	test(FindWithEqualityOp(buf1, buf2) == KErrNone);

	return KErrNone;
	}

LOCAL_C TInt EntryFindTest(TInt aCount, TInt aNumTests)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<SEntry> a(aCount,0);	// keyed on iKey
		RArray<SEntry> b(aCount,4);	// keyed on iValue
		SEntry *pE=new SEntry[aCount];
		if (!pE)
			{
			a.Close();
			b.Close();
			return -65535;
			}
		TInt i;
		for (i=0; i<aCount; i++)
			{
			pE[i].iValue=Random();
			pE[i].iKey=Random();
			a.Append(pE[i]);
			b.Append(pE[i]);
			}
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
			return -1;
			}
		for (i=0; i<aCount; i++)
			{
			SEntry e1=pE[i];
			SEntry e2=pE[i];
			e2.iValue=~e2.iValue;
			SEntry e3=pE[i];
			e3.iKey=~e3.iKey;
			TInt r=a.Find(e1);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -2;
				}
			r=a.Find(e2);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -3;
				}
			r=a.Find(e3);
			if (r>=0 && pE[r].iKey!=e3.iKey)
				{
				a.Close();
				b.Close();
				return -4;
				}
			r=b.Find(e1);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -5;
				}
			r=b.Find(e3);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -6;
				}
			r=b.Find(e2);
			if (r>=0 && pE[r].iValue!=e3.iValue)
				{
				a.Close();
				b.Close();
				return -7;
				}
			r=a.Find(e1,SEntryIdentity);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -8;
				}
			r=a.Find(e2,SEntryIdentity);
			if (r>=0 && pE[r]!=e3)
				{
				a.Close();
				b.Close();
				return -9;
				}
			r=a.Find(e3,SEntryIdentity);
			if (r>=0 && pE[r]!=e3)
				{
				a.Close();
				b.Close();
				return -10;
				}
			r=b.Find(e1,SEntryIdentity);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -11;
				}
			r=b.Find(e3,SEntryIdentity);
			if (r>=0 && pE[r]!=e3)
				{
				a.Close();
				b.Close();
				return -12;
				}
			r=b.Find(e2,SEntryIdentity);
			if (r>=0 && pE[r]!=e3)
				{
				a.Close();
				b.Close();
				return -13;
				}
			r=a.Find(e1.iValue,CompareSEntryByKeyValue);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -14;
				}
			r=a.Find(e2.iValue,CompareSEntryByKeyValue);
			if (r>=0 && pE[r].iValue!=e2.iValue)
				{
				a.Close();
				b.Close();
				return -15;
				}			
			r=b.Find(e1.iKey,CompareSEntryByKeyKey);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -16;
				}
			r=b.Find(e3.iKey,CompareSEntryByKeyKey);
			if (r>=0 && pE[r].iKey!=e3.iKey)
				{
				a.Close();
				b.Close();
				return -17;
				}
			}
		delete[] pE;
		a.Close();
		b.Close();
		}
	return KErrNone;
	}
	
LOCAL_C TInt IntFindReverseTest(TInt aCount, TInt aNumTests, TInt64 aMask)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<TInt64> a(aCount);
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
			a.Append(pA[i]);
			}
		if (a.Count()!=aCount)
			{
			a.Close();
			return -1;
			}
		for (i=0; i<aCount; i++)
			{
			TInt r=a.FindReverse(pA[i],Int64Identity);
			if (pA[i]!=pA[r] || r<i)
				{
				a.Close();
				return -2;
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
	for (n=0; n<aNumTests; n++)
		{
		RArray<SEntry> a(aCount,0);	// keyed on iKey
		RArray<SEntry> b(aCount,4);	// keyed on iValue
		SEntry *pE=new SEntry[aCount];
		if (!pE)
			{
			a.Close();
			b.Close();
			return -65535;
			}
		TInt i;
		for (i=0; i<aCount; i++)
			{
			pE[i].iValue=Random();
			pE[i].iKey=Random();
			a.Append(pE[i]);
			b.Append(pE[i]);
			}
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
			return -1;
			}
		for (i=0; i<aCount; i++)
			{
			SEntry e1=pE[i];
			SEntry e2=pE[i];
			e2.iValue=~e2.iValue;
			SEntry e3=pE[i];
			e3.iKey=~e3.iKey;
			TInt r=a.FindReverse(e1);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -2;
				}
			r=a.FindReverse(e2);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -3;
				}
			r=a.FindReverse(e3);
			if (r>=0 && pE[r].iKey!=e3.iKey)
				{
				a.Close();
				b.Close();
				return -4;
				}
			r=b.FindReverse(e1);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -5;
				}
			r=b.FindReverse(e3);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -6;
				}
			r=b.FindReverse(e2);
			if (r>=0 && pE[r].iValue!=e3.iValue)
				{
				a.Close();
				b.Close();
				return -7;
				}
			r=a.FindReverse(e1,SEntryIdentity);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -8;
				}
			r=a.FindReverse(e2,SEntryIdentity);
			if (r>=0 && pE[r]!=e3)
				{
				a.Close();
				b.Close();
				return -9;
				}
			r=a.FindReverse(e3,SEntryIdentity);
			if (r>=0 && pE[r]!=e3)
				{
				a.Close();
				b.Close();
				return -10;
				}
			r=b.FindReverse(e1,SEntryIdentity);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -11;
				}
			r=b.FindReverse(e3,SEntryIdentity);
			if (r>=0 && pE[r]!=e3)
				{
				a.Close();
				b.Close();
				return -12;
				}
			r=b.FindReverse(e2,SEntryIdentity);
			if (r>=0 && pE[r]!=e3)
				{
				a.Close();
				b.Close();
				return -13;
				}
			r=a.FindReverse(e1.iValue,CompareSEntryByKeyValue);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -14;
				}
			r=a.FindReverse(e2.iValue,CompareSEntryByKeyValue);
			if (r>=0 && pE[r].iValue!=e2.iValue)
				{
				a.Close();
				b.Close();
				return -15;
				}			
			r=b.FindReverse(e1.iKey,CompareSEntryByKeyKey);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -16;
				}
			r=b.FindReverse(e3.iKey,CompareSEntryByKeyKey);
			if (r>=0 && pE[r].iKey!=e3.iKey)
				{
				a.Close();
				b.Close();
				return -17;
				}
			}
		delete[] pE;
		a.Close();
		b.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt IntFindInOrderTest(TInt aCount, TInt aNumTests, TInt64 aMask)
// require aRange*aCount<2^32
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<TInt64> a(aCount);
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
			a.Append(y);
			y+=x;
			}
		if (a.Count()!=aCount)
			{
			a.Close();
			return -1;
			}
		for (i=0; i<aCount; i++)
			{
			TInt r=a.FindInOrder(pA[i],Int64Order);
			if (r<0 || pA[r]!=pA[i])
				{
				a.Close();
				return -2;
				}
			TInt64 x=Random64(aMask);
			r=a.FindInOrder(x,Int64Order);
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
		RArray<SEntry> a(aCount,0);	// keyed on iKey
		RArray<SEntry> b(aCount,4);	// keyed on iValue
		SEntry *pE=new SEntry[aCount];
		SEntry *pF=new SEntry[aCount];
		if (!pE || !pF)
			{
			a.Close();
			b.Close();
			return -65535;
			}
		TInt i=0;
		for(i=0; i<aCount; i++)
			{
			pE[i].iKey=i*19;
			pE[i].iValue=Random();
			a.Append(pE[i]);
			pF[i].iKey=Random();
			pF[i].iValue=i*i;
			b.Append(pF[i]);
			}
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
			TInt r=a.FindInSignedKeyOrder(pE[i]);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -2;
				}
			r=b.FindInSignedKeyOrder(pF[i]);
			if (r!=i)
				{
				a.Close();
				b.Close();
				return -3;
				}
			TInt x=Random()&1023;
			SEntry e;
			e.iKey=x;
			r=a.FindInSignedKeyOrder(e);
			if (r<0)
				{
				if (x%19==0)
					{
					a.Close();
					b.Close();
					return -4;
					}
				}
			else if (x!=r*19)
				{
				a.Close();
				b.Close();
				return -5;
				}
			TInt z=8+(Random()&127);
			TInt y=Random()&15;
			x=z*z+y;
			e.iKey=-2;
			e.iValue=x;
			r=b.FindInSignedKeyOrder(e);
			if (r<0)
				{
				if (y==0 && z<aCount)
					{
					a.Close();
					b.Close();
					return -6;
					}
				}
			else if (y!=0)
				{
				a.Close();
				b.Close();
				return -7;
				}
			}
		delete[] pE;
		delete[] pF;
		a.Close();
		b.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt IntInsertInOrderTest(TInt aCount, TInt aNumTests, TInt64 aMask)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<TInt64> a(aCount);
		RArray<TInt64> b(aCount);
		RArray<TInt64> c(aCount);
		TInt i;
		TInt cc=0;
		for (i=0; i<aCount; i++)
			{
			TInt64 x=Random64(aMask);
			a.Append(x);
			b.InsertInOrderAllowRepeats(x,Int64Order);
			TInt r=c.InsertInOrder(x,Int64Order);
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
			if (c[i]>=c[i+1])
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

LOCAL_C TInt EntryInsertInOrderTest()
	{
	RArray<SEntry> a1(1024,0);	// keyed on iKey
	RArray<SEntry> a2(1024,0);	// keyed on iKey
	RArray<SEntry> b1(1024,4);	// keyed on iValue
	RArray<SEntry> b2(1024,4);	// keyed on iValue
	RArray<SEntry> c1(1024,0);	// keyed on iKey
	RArray<SEntry> c2(1024,0);	// keyed on iKey
	RArray<SEntry> d1(1024,4);	// keyed on iValue
	RArray<SEntry> d2(1024,4);	// keyed on iValue
	TInt i;
	for (i=0; i<1024; i++)
		{
		SEntry e;
		e.iValue=i-512;
		e.iKey=(i&31)-16;
		a1.InsertInSignedKeyOrderAllowRepeats(e);
		a2.InsertInSignedKeyOrder(e);
		b1.InsertInSignedKeyOrderAllowRepeats(e);
		b2.InsertInSignedKeyOrder(e);
		c1.InsertInUnsignedKeyOrderAllowRepeats(e);
		c2.InsertInUnsignedKeyOrder(e);
		d1.InsertInUnsignedKeyOrderAllowRepeats(e);
		d2.InsertInUnsignedKeyOrder(e);
		}
	if (a1.Count()!=1024)
		{
		a1.Close();
		a2.Close();
		b1.Close();
		b2.Close();
		c1.Close();
		c2.Close();
		d1.Close();
		d2.Close();
		return -1;
		}
	if (b1.Count()!=1024)
		{
		a1.Close();
		a2.Close();
		b1.Close();
		b2.Close();
		c1.Close();
		c2.Close();
		d1.Close();
		d2.Close();
		return -2;
		}
	if (c1.Count()!=1024)
		{
		a1.Close();
		a2.Close();
		b1.Close();
		b2.Close();
		c1.Close();
		c2.Close();
		d1.Close();
		d2.Close();
		return -3;
		}
	if (d1.Count()!=1024)
		{
		a1.Close();
		a2.Close();
		b1.Close();
		b2.Close();
		c1.Close();
		c2.Close();
		d1.Close();
		d2.Close();
		return -4;
		}
	for (i=0; i<1024; i++)
		{
		SEntry e=a1[i];
		if (e.iKey!=(i>>5)-16)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -5;
			}
		if ( e.iValue!=(((i&31)<<5 | (i>>5))-512) )
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -6;
			}
		e=b1[i];
		if (e.iKey!=((i&31)-16))
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -7;
			}
		if ( e.iValue!=(i-512) )
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -8;
			}
		e=c1[i];
		TInt j=i>>5;
		j^=16;
		j=((i&31)<<5)|j;
		SEntry f;
		f.iValue=j-512;
		f.iKey=(j&31)-16;
		if (e.iKey!=f.iKey)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -9;
			}
		if (e.iValue!=f.iValue)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -10;
			}
		e=d1[i];
		j=i^512;
		f.iValue=j-512;
		f.iKey=(j&31)-16;
		if (e.iKey!=f.iKey)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -11;
			}
		if (e.iValue!=f.iValue)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -12;
			}
		}
	if (a2.Count()!=32)
		{
		a1.Close();
		a2.Close();
		b1.Close();
		b2.Close();
		c1.Close();
		c2.Close();
		d1.Close();
		d2.Close();
		return -13;
		}
	if (b2.Count()!=1024)
		{
		a1.Close();
		a2.Close();
		b1.Close();
		b2.Close();
		c1.Close();
		c2.Close();
		d1.Close();
		d2.Close();
		return -14;
		}
	if (c2.Count()!=32)
		{
		a1.Close();
		a2.Close();
		b1.Close();
		b2.Close();
		c1.Close();
		c2.Close();
		d1.Close();
		d2.Close();
		return -15;
		}
	if (d2.Count()!=1024)
		{
		a1.Close();
		a2.Close();
		b1.Close();
		b2.Close();
		c1.Close();
		c2.Close();
		d1.Close();
		d2.Close();
		return -16;
		}
	for (i=0; i<1024; i++)
		{
		SEntry e=b2[i];
		if (e.iKey!=((i&31)-16))
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -17;
			}
		if ( e.iValue!=(i-512) )
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -18;
			}
		e=d2[i];
		TInt j=i^512;
		SEntry f;
		f.iValue=j-512;
		f.iKey=(j&31)-16;
		if (e.iKey!=f.iKey)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -19;
			}
		if (e.iValue!=f.iValue)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -20;
			}
		}
	for (i=0; i<31; i++)
		{
		SEntry e=a2[i];
		TInt j=i;
		SEntry f;
		f.iValue=j-512;
		f.iKey=(j&31)-16;
		if (e.iKey!=f.iKey)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -21;
			}
		if (e.iValue!=f.iValue)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -22;
			}
		e=c2[i];
		j=i^16;
		f.iValue=j-512;
		f.iKey=(j&31)-16;
		if (e.iKey!=f.iKey)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -23;
			}
		if (e.iValue!=f.iValue)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -24;
			}
		}
	a1.Close();
	a2.Close();
	b1.Close();
	b2.Close();
	c1.Close();
	c2.Close();
	d1.Close();
	d2.Close();
	return KErrNone;
	}

LOCAL_C TInt IntSortTest(TInt aCount, TInt aNumTests, TInt64 aMask)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<TInt64> a(aCount);
		RArray<TInt64> b(aCount);
		TInt i;
		for (i=0; i<aCount; i++)
			{
			TInt64 x=Random64(aMask);
			a.Append(x);
			b.InsertInOrderAllowRepeats(x,Int64Order);
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

LOCAL_C TInt EntrySortTest(TInt aCount, TInt aNumTests)
	{
	TInt n;
	for (n=0; n<aNumTests; n++)
		{
		RArray<SEntry> a1(aCount,0);	// keyed on iKey
		RArray<SEntry> a2(aCount,0);	// keyed on iKey
		RArray<SEntry> b1(aCount,4);	// keyed on iValue
		RArray<SEntry> b2(aCount,4);	// keyed on iValue
		RArray<SEntry> c1(aCount,0);	// keyed on iKey
		RArray<SEntry> c2(aCount,0);	// keyed on iKey
		RArray<SEntry> d1(aCount,4);	// keyed on iValue
		RArray<SEntry> d2(aCount,4);	// keyed on iValue
		TInt i;
		for (i=0; i<aCount; i++)
			{
			SEntry e;
			e.iKey=Random();
			e.iValue=Random();
			a1.Append(e);
			a2.InsertInSignedKeyOrderAllowRepeats(e);
			b1.Append(e);
			b2.InsertInSignedKeyOrderAllowRepeats(e);
			c1.Append(e);
			c2.InsertInUnsignedKeyOrderAllowRepeats(e);
			d1.Append(e);
			d2.InsertInUnsignedKeyOrderAllowRepeats(e);
			}
		a1.SortSigned();
		b1.SortSigned();
		c1.SortUnsigned();
		d1.SortUnsigned();
		if (a1.Count()!=aCount)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -1;
			}
		if (a2.Count()!=aCount)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -2;
			}
		if (b1.Count()!=aCount)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -3;
			}
		if (b2.Count()!=aCount)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -4;
			}
		if (c1.Count()!=aCount)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -5;
			}
		if (c2.Count()!=aCount)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -6;
			}
		if (d1.Count()!=aCount)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -7;
			}
		if (d2.Count()!=aCount)
			{
			a1.Close();
			a2.Close();
			b1.Close();
			b2.Close();
			c1.Close();
			c2.Close();
			d1.Close();
			d2.Close();
			return -8;
			}
		for (i=0; i<aCount; i++)
			{
			if (a1[i]!=a2[i])
				{
				a1.Close();
				a2.Close();
				b1.Close();
				b2.Close();
				c1.Close();
				c2.Close();
				d1.Close();
				d2.Close();
				return -9;
				}
			if (b1[i]!=b2[i])
				{
				a1.Close();
				a2.Close();
				b1.Close();
				b2.Close();
				c1.Close();
				c2.Close();
				d1.Close();
				d2.Close();
				return -10;
				}
			if (c1[i]!=c2[i])
				{
				a1.Close();
				a2.Close();
				b1.Close();
				b2.Close();
				c1.Close();
				c2.Close();
				d1.Close();
				d2.Close();
				return -11;
				}
			if (d1[i]!=d2[i])
				{
				a1.Close();
				a2.Close();
				b1.Close();
				b2.Close();
				c1.Close();
				c2.Close();
				d1.Close();
				d2.Close();
				return -12;
				}
			}
		a1.Close();
		a2.Close();
		b1.Close();
		b2.Close();
		c1.Close();
		c2.Close();
		d1.Close();
		d2.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt SortAccessBoundsTest(TInt aCount)
	{
	TInt bytes = aCount * sizeof(TInt);
	RChunk chunk;
	TInt r = chunk.CreateDoubleEndedLocal(4096, bytes + 4096, bytes + 8192);
	if (r != KErrNone)
		return r;
	TInt size = chunk.Size() / sizeof(TInt); // Now rounded up to page boundary
	TInt* data = (TInt*)(chunk.Base() + chunk.Bottom());

	TInt i, j;
	
	for (i = 1 ; i < aCount ; ++i)
		{
		for (j = 0 ; j < i ; ++j)
			data[j] = Random();
		RArray<TInt> a(data, i);
		a.Sort();
		
		for (j = 0 ; j < i ; ++j)
			data[j] = Random();
		RArray<TUint> b((TUint*)data, i);
		b.Sort();

		if (i % 2 == 0)
			{
			RArray<SEntry> c(sizeof(SEntry), (SEntry*)data, i / 2);
		
			for (j = 0 ; j < i ; ++j)
				data[j] = Random();
			c.SortSigned();
		
			for (j = 0 ; j < i ; ++j)
				data[j] = Random();
			c.SortUnsigned();
			}
		}
		
	for (i = 1 ; i < aCount ; ++i)
		{
		for (j = 0 ; j < i ; ++j)
			data[size - j - 1] = Random();
		RArray<TInt> a(data + size - i, i);
		a.Sort();
		
		for (j = 0 ; j < i ; ++j)
			data[size - j - 1] = Random();
		RArray<TUint> b((TUint*)(data + size - i), i);
		b.Sort();

		if (i % 2 == 0)
			{
			RArray<SEntry> c(sizeof(SEntry), (SEntry*)(data + size - i), i / 2);
		
			for (j = 0 ; j < i ; ++j)
				data[size - j - 1] = Random();
			c.SortSigned();
		
			for (j = 0 ; j < i ; ++j)
				data[size - j - 1] = Random();
			c.SortUnsigned();
			}
		}
	
	chunk.Close();	
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
		RArray<SEntry> a;
		RArray<SEntry> b;
		RArray<SEntry> c;
		RArray<SEntry> d;
		TInt i;
		for (i=0; i<aCount; i++)
			{
			TInt x=Random()&aRange;
			x-=(aRange>>1);
			SEntry e;
			e.iKey = x;
			e.iValue = i;
			a.Append(e);
			c.Append(e);
			b.InsertInSignedKeyOrderAllowRepeats(e);
			d.InsertInUnsignedKeyOrderAllowRepeats(e);
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
			TInt fk = a.SpecificFindInSignedKeyOrder(es, EArrayFindMode_First);
			TInt lk = a.SpecificFindInSignedKeyOrder(es, EArrayFindMode_Last);
			TInt ak = a.SpecificFindInSignedKeyOrder(es, EArrayFindMode_Any);
			TInt fki, lki, aki;
			TInt fk2 = a.SpecificFindInSignedKeyOrder(es, fki, EArrayFindMode_First);
			TInt lk2 = a.SpecificFindInSignedKeyOrder(es, lki, EArrayFindMode_Last);
			TInt ak2 = a.SpecificFindInSignedKeyOrder(es, aki, EArrayFindMode_Any);

			TInt first = a.SpecificFindInOrder(es, &OrderSEntry, EArrayFindMode_First);
			TInt last = a.SpecificFindInOrder(es, &OrderSEntry, EArrayFindMode_Last);
			TInt any = a.SpecificFindInOrder(es, &OrderSEntry, EArrayFindMode_Any);
			TInt fi, li, ai;
			TInt first2 = a.SpecificFindInOrder(es, fi, &OrderSEntry, EArrayFindMode_First);
			TInt last2 = a.SpecificFindInOrder(es, li, &OrderSEntry, EArrayFindMode_Last);
			TInt any2 = a.SpecificFindInOrder(es, ai, &OrderSEntry, EArrayFindMode_Any);
			++ntot;
			test(first == fk);
			test(last == lk);
			test(any == ak);
			test(first2 == fk2);
			test(last2 == lk2);
			test(any2 == ak2);
			test(fki == fi);
			test(lki == li);
			test(aki == ai);
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
				test(li==aCount || a[li].iKey>i);
				test(li==0 || a[li-1].iKey<i);
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
				test(a[fi].iKey == i);
				test(a[li-1].iKey == i);
				test(li==aCount || a[li].iKey>i);
				test(ai>=fi && ai<li);
				test(a[ai].iKey == i);
				if (li-fi > 1)
					{
					++nrpt;
					TInt j;
					for (j=fi+1; j<li; ++j)
						test(a[j].iValue > a[j-1].iValue);
					}
				}
			}
		for (i=-(aRange>>1); i<=(aRange>>1); ++i)
			{
			TUint u = (TUint)i;
			SEntry eu;
			eu.iKey = i;
			TInt fk = c.SpecificFindInUnsignedKeyOrder(eu, EArrayFindMode_First);
			TInt lk = c.SpecificFindInUnsignedKeyOrder(eu, EArrayFindMode_Last);
			TInt ak = c.SpecificFindInUnsignedKeyOrder(eu, EArrayFindMode_Any);
			TInt fki, lki, aki;
			TInt fk2 = c.SpecificFindInUnsignedKeyOrder(eu, fki, EArrayFindMode_First);
			TInt lk2 = c.SpecificFindInUnsignedKeyOrder(eu, lki, EArrayFindMode_Last);
			TInt ak2 = c.SpecificFindInUnsignedKeyOrder(eu, aki, EArrayFindMode_Any);

			TInt first = c.SpecificFindInOrder(eu, &OrderSEntryU, EArrayFindMode_First);
			TInt last = c.SpecificFindInOrder(eu, &OrderSEntryU, EArrayFindMode_Last);
			TInt any = c.SpecificFindInOrder(eu, &OrderSEntryU, EArrayFindMode_Any);
			TInt fi, li, ai;
			TInt first2 = c.SpecificFindInOrder(eu, fi, &OrderSEntryU, EArrayFindMode_First);
			TInt last2 = c.SpecificFindInOrder(eu, li, &OrderSEntryU, EArrayFindMode_Last);
			TInt any2 = c.SpecificFindInOrder(eu, ai, &OrderSEntryU, EArrayFindMode_Any);
			++ntot;
			test(first == fk);
			test(last == lk);
			test(any == ak);
			test(first2 == fk2);
			test(last2 == lk2);
			test(any2 == ak2);
			test(fki == fi);
			test(lki == li);
			test(aki == ai);
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
				test(li==aCount || TUint(c[li].iKey)>u);
				test(li==0 || TUint(c[li-1].iKey)<u);
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
				test(c[fi].iKey == i);
				test(c[li-1].iKey == i);
				test(li==aCount || TUint(c[li].iKey)>u);
				test(ai>=fi && ai<li);
				test(c[ai].iKey == i);
				if (li-fi > 1)
					{
					++nrpt;
					TInt j;
					for (j=fi+1; j<li; ++j)
						test(c[j].iValue > c[j-1].iValue);
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

LOCAL_C void TestGrowCompress(RArray<TInt64>* a, ...)
	{
	SArray& pa = *(SArray*)a;
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
				r = a->Append(x);
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
	RArray<TInt64> a;
	TestGrowCompress(&a, 1, 8, 7, 8, 1, 16, 7, 16, 1, 24, -2, 24, -1, 17, 1, 25, -2, 25, -3, 25, -2, 24, -99);
	TestGrowCompress(&a, 1, 8, 7, 8, 1, 16, 7, 16, 1, 24, -2, 24, -1, 17, 1, 25, -2, 25, -3, 25, -3, 25, -2, 16, -99);

	RArray<TInt64> b(100);
	TestGrowCompress(&b, 1, 100, 99, 100, 1, 200, 99, 200, 1, 300, -2, 300, -1, 201, 1, 301, -2, 301, -3, 301, -2, 300, -99);
	TestGrowCompress(&b, 1, 100, 99, 100, 1, 200, 99, 200, 1, 300, -2, 300, -1, 201, 1, 301, -2, 301, -3, 301, -3, 301, -2, 200, -99);

	RArray<TInt64> c(8, 0, 512);
	TestGrowCompress(&c, 1, 8, 7, 8, 1, 16, 7, 16, 1, 32, 15, 32, 1, 64, -2, 40, 7, 40, 1, 80, -1, 41, -99);

	RArray<TInt64> d(20, 0, 640);
	TestGrowCompress(&d, 1, 20, 19, 20, 1, 50, 29, 50, 1, 125, -2, 60, -1, 51, -99);

	RArray<TInt64> e(8, 0, 320);
	TestGrowCompress(&e, 1, 8, 7, 8, 1, 16, 7, 16, 1, 24, 7, 24, 1, 32, 7, 32, 1, 40, 7, 40, 1, 50, 9, 50, 1, 63, -99);

	RArray<TInt64> f(2, 0, 257);
	TestGrowCompress(&f, 1, 2, 255, 256, 256, 512, 128, 640, 1, 643, 2, 643, 1, 646, -99);
	}

GLDEF_C void DoSimpleArrayTests()
	{
	test.Start(_L("Simple Arrays..."));

	test.Next(_L("AppendAndAccess tests..."));
	test.Next(_L("Count 10 Mask 0x0000000300000003"));
	test(IntAppendAndAccessTest(10,NUM_TESTS,MAKE_TINT64(0x3,0x3))==KErrNone);
	test.Next(_L("Count 100 Range all"));
	test(IntAppendAndAccessTest(100,NUM_TESTS,MAKE_TINT64(0xffffffff,0xffffffff))==KErrNone);

	test.Next(_L("Remove test"));
	test(IntRemoveTest()==KErrNone);

	test.Next(_L("Find tests..."));
	test.Next(_L("Count 10 Mask 0x0000000300000003"));
	test(IntFindTest(10,NUM_TESTS,MAKE_TINT64(3,3))==KErrNone);
	test.Next(_L("Count 100 Range all"));
	test(IntFindTest(100,NUM_TESTS,MAKE_TINT64(0xffffffff,0xffffffff))==KErrNone);
	test.Next(_L("SEntry find tests"));
	test(EntryFindTest(128,NUM_TESTS)==KErrNone);
	test.Next(_L("Find with equality operator tests"));
	test(FindWithEqualityOpTest()==KErrNone);


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
	test.Next(_L("SEntry sort test"));
	test(EntrySortTest(128,NUM_TESTS)==KErrNone);

	test.Next(_L("SEntrySpecificFindTests..."));
	test(SEntrySpecificFindTests(100, 10, 15)==KErrNone);
	test(SEntrySpecificFindTests(100, 10, 127)==KErrNone);

	test.Next(_L("Test Grow/Compress"));
	TestGrowCompress();

	test.Next(_L("Test sort methods don't access memory beyond the end of the array"));
	test(SortAccessBoundsTest(128)==KErrNone);
	
	test.End();
	}


GLDEF_C void DoArrayLeavingInterfaceTest()
	{
	TInt trap, ret(0);
	TInt64 Int64s[3];
	for (TInt i=0;i<3;i++) Int64s[i] = i;

	RArray<TInt64> array;
	CleanupClosePushL(array);

	test.Start(_L("Checking Leaving Arrays Interface..."));

	test.Next(_L("AppendL test..."));
	TRAP(trap, array.AppendL(Int64s[0]));
	test(trap==KErrNone);

	test.Next(_L("InsertL test..."));
	TRAP(trap, array.InsertL(Int64s[1],1));
	test(trap==KErrNone);

	test.Next(_L("Test FindL(const T& anEntry) const..."));
	TRAP(trap, ret = array.FindL(Int64s[0]));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = array.FindL(Int64s[2]));
	test(trap==KErrNotFound);
	
	test.Next(_L("Test FindL(const T& anEntry, TIdentityRelation<T> anIdentity) const..."));
	TRAP(trap, ret = array.FindL(Int64s[0],Int64Identity));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = array.FindL(Int64s[2],Int64Identity));
	test(trap==KErrNotFound);

	test.Next(_L("Test FindInSignedKeyOrderL(const T& anEntry) const..."));
	TRAP(trap, ret = array.FindInSignedKeyOrderL(Int64s[0]));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = array.FindInSignedKeyOrderL(Int64s[2]));
	test(trap==KErrNotFound);

	test.Next(_L("Test FindInUnsignedKeyOrderL(const T& anEntry) const..."));
	TRAP(trap, ret = array.FindInUnsignedKeyOrderL(Int64s[0]));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = array.FindInUnsignedKeyOrderL(Int64s[2]));
	test(trap==KErrNotFound);

	test.Next(_L("Test FindInOrderL(const T& anEntry, TLinearOrder<T> anOrder) const..."));
	TRAP(trap, ret = array.FindInOrderL(Int64s[0], Int64Order));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = array.FindInOrderL(Int64s[2], Int64Order));
	test(trap==KErrNotFound);

	test.Next(_L("Test FindInSignedKeyOrderL(const T& anEntry, TInt& anIndex) const..."));
	TRAP(trap, array.FindInSignedKeyOrderL(Int64s[0], ret));
	test(trap==0);
	test(ret==0);
	TRAP(trap, array.FindInSignedKeyOrderL(Int64s[2], ret));
	test(trap==KErrNotFound);

	test.Next(_L("Test FindInUnsignedKeyOrderL(const T& anEntry, TInt& anIndex) const..."));
	TRAP(trap, array.FindInUnsignedKeyOrderL(Int64s[0], ret));
	test(trap==0);
	test(ret==0);
	TRAP(trap, array.FindInUnsignedKeyOrderL(Int64s[2], ret));
	test(trap==KErrNotFound);

	test.Next(_L("Test FindInOrderL(const T& anEntry, TInt& anIndex, TLinearOrder<T> anOrder) const..."));
	TRAP(trap, array.FindInOrderL(Int64s[0], ret, Int64Order));
	test(trap==0);
	test(ret==0);
	TRAP(trap, array.FindInOrderL(Int64s[2], ret, Int64Order));
	test(trap==KErrNotFound);

	test.Next(_L("Test SpecificFindInSignedKeyOrderL(const T& anEntry, TInt aMode) const..."));
	TRAP(trap, ret = array.SpecificFindInSignedKeyOrderL(Int64s[0], EArrayFindMode_First));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = array.SpecificFindInSignedKeyOrderL(Int64s[2], EArrayFindMode_First));
	test(trap==KErrNotFound);

	test.Next(_L("Test SpecificFindInUnsignedKeyOrderL(const T& anEntry, TInt aMode) const..."));
	TRAP(trap, ret = array.SpecificFindInUnsignedKeyOrderL(Int64s[0], EArrayFindMode_First));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = array.SpecificFindInUnsignedKeyOrderL(Int64s[2], EArrayFindMode_First));
	test(trap==KErrNotFound);

	test.Next(_L("Test SpecificFindInOrderL(const T& anEntry, TLinearOrder<T> anOrder, TInt aMode) const..."));
	TRAP(trap, ret = array.SpecificFindInOrderL(Int64s[0], Int64Order, EArrayFindMode_First));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = array.SpecificFindInOrderL(Int64s[2], Int64Order, EArrayFindMode_First));
	test(trap==KErrNotFound);

	test.Next(_L("Test SpecificFindInSignedKeyOrderL(const T& anEntry, TInt& anIndex, TInt aMode) const..."));
	TRAP(trap, array.SpecificFindInSignedKeyOrderL(Int64s[0], ret, EArrayFindMode_First));
	test(trap==0);
	test(ret==0);
	TRAP(trap, array.SpecificFindInSignedKeyOrderL(Int64s[2], ret, EArrayFindMode_First));
	test(trap==KErrNotFound);

	test.Next(_L("Test SpecificFindInUnsignedKeyOrderL(const T& anEntry, TInt& anIndex, TInt aMode) const..."));
	TRAP(trap, array.SpecificFindInUnsignedKeyOrderL(Int64s[0], ret, EArrayFindMode_First));
	test(trap==0);
	test(ret==0);
	TRAP(trap, array.SpecificFindInUnsignedKeyOrderL(Int64s[2], ret, EArrayFindMode_First));
	test(trap==KErrNotFound);

	test.Next(_L("Test SpecificFindInOrderL(const T& anEntry, TInt& anIndex, TLinearOrder<T> anOrder, TInt aMode) const..."));
	TRAP(trap, array.SpecificFindInOrderL(Int64s[0], ret, Int64Order, EArrayFindMode_First));
	test(trap==0);
	test(ret==0);
	TRAP(trap, array.SpecificFindInOrderL(Int64s[2], ret, Int64Order, EArrayFindMode_First));
	test(trap==KErrNotFound);

	test.Next(_L("Test InsertInSignedKeyOrderL(const T& anEntry)..."));
	TRAP(trap, array.InsertInSignedKeyOrderL(Int64s[0]));
	test(trap==KErrAlreadyExists);
	TRAP(trap, array.InsertInSignedKeyOrderL(Int64s[2]));
	test(trap==KErrNone);
	array.Remove(2);

	test.Next(_L("Test InsertInUnsignedKeyOrderL(const T& anEntry)..."));
	TRAP(trap, array.InsertInUnsignedKeyOrderL(Int64s[0]));
	test(trap==KErrAlreadyExists);
	TRAP(trap, array.InsertInUnsignedKeyOrderL(Int64s[2]));
	test(trap==KErrNone);
	array.Remove(2);

	test.Next(_L("Test InsertInOrderL(const T& anEntry, TLinearOrder<T> anOrder)..."));
	TRAP(trap, array.InsertInOrderL(Int64s[0], Int64Order));
	test(trap==KErrAlreadyExists);
	TRAP(trap, array.InsertInOrderL(Int64s[2], Int64Order));
	test(trap==KErrNone);
	array.Remove(2);

	test.Next(_L("Test InsertInSignedKeyOrderAllowRepeatsL(const T& anEntry)..."));
	TRAP(trap, array.InsertInSignedKeyOrderAllowRepeatsL(Int64s[2]));
	test(trap==KErrNone);
	array.Remove(2);

	test.Next(_L("Test InsertInUnsignedKeyOrderAllowRepeatsL(const T& anEntry)..."));
	TRAP(trap, array.InsertInUnsignedKeyOrderAllowRepeatsL(Int64s[2]));
	test(trap==KErrNone);
	array.Remove(2);

	test.Next(_L("Test InsertInOrderAllowRepeatsL(const T& anEntry, TLinearOrder<T> anOrder)..."));
	TRAP(trap, array.InsertInOrderAllowRepeatsL(Int64s[2], Int64Order));
	test(trap==KErrNone);
	array.Remove(2);

	CleanupStack::PopAndDestroy(&array);
	test.End();
	}

GLDEF_C void DoTIntArrayLeavingInterfaceTest()
	{
	TInt trap, ret(0);
	TInt Ints[3];
	for (TInt i=0;i<3;i++) Ints[i] = i;

	RArray<TInt> array;
	CleanupClosePushL(array);

	test.Start(_L("Checking Leaving Array<TInt> Interface..."));

	test.Next(_L("AppendL test..."));
	TRAP(trap, array.AppendL(Ints[0]));
	test(trap==KErrNone);

	test.Next(_L("InsertL test..."));
	TRAP(trap, array.InsertL(Ints[1],1));
	test(trap==KErrNone);

	test.Next(_L("Test FindL(TInt anEntry) const..."));
	TRAP(trap, ret = array.FindL(Ints[0]));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = array.FindL(Ints[2]));
	test(trap==KErrNotFound);
	

	test.Next(_L("Test FindInOrderL(TInt anEntry) const..."));
	TRAP(trap, ret = array.FindInOrderL(Ints[0]));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = array.FindInOrderL(Ints[2]));
	test(trap==KErrNotFound);

	test.Next(_L("Test FindInOrderL(TInt anEntry, TInt& anIndex) const..."));
	TRAP(trap, array.FindInOrderL(Ints[0], ret));
	test(trap==0);
	test(ret==0);
	TRAP(trap, array.FindInOrderL(Ints[2], ret));
	test(trap==KErrNotFound);

	test.Next(_L("Test SpecificFindInOrderL(TInt anEntry, TInt aMode) const..."));
	TRAP(trap, ret = array.SpecificFindInOrderL(Ints[0], EArrayFindMode_First));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = array.SpecificFindInOrderL(Ints[2], EArrayFindMode_First));
	test(trap==KErrNotFound);

	test.Next(_L("Test SpecificFindInOrderL(TInt anEntry, TInt& anIndex, TInt aMode) const..."));
	TRAP(trap, array.SpecificFindInOrderL(Ints[0], ret, EArrayFindMode_First));
	test(trap==0);
	test(ret==0);
	TRAP(trap, array.SpecificFindInOrderL(Ints[2], ret, EArrayFindMode_First));
	test(trap==KErrNotFound);

	test.Next(_L("Test InsertInOrderL(TInt anEntry)..."));
	TRAP(trap, array.InsertInOrderL(Ints[0]));
	test(trap==KErrAlreadyExists);
	TRAP(trap, array.InsertInOrderL(Ints[2]));
	test(trap==KErrNone);
	array.Remove(2);

	test.Next(_L("Test InsertInOrderAllowRepeatsL(TInt anEntry)..."));
	TRAP(trap, array.InsertInOrderAllowRepeatsL(Ints[2]));
	test(trap==KErrNone);
	array.Remove(2);

	CleanupStack::PopAndDestroy(&array);
	test.End();
	}

GLDEF_C void DoTUintArrayLeavingInterfaceTest()
	{
	TInt trap, ret(0);
	TInt UInts[3];
	for (TInt i=0;i<3;i++) UInts[i] = i;

	RArray<TInt> array;
	CleanupClosePushL(array);

	test.Start(_L("Checking Leaving Array<TUint> Interface..."));

	test.Next(_L("AppendL test..."));
	TRAP(trap, array.AppendL(UInts[0]));
	test(trap==KErrNone);

	test.Next(_L("InsertL test..."));
	TRAP(trap, array.InsertL(UInts[1],1));
	test(trap==KErrNone);

	test.Next(_L("Test FindL(TUint anEntry) const..."));
	TRAP(trap, ret = array.FindL(UInts[0]));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = array.FindL(UInts[2]));
	test(trap==KErrNotFound);
	

	test.Next(_L("Test FindInOrderL(TUint anEntry) const..."));
	TRAP(trap, ret = array.FindInOrderL(UInts[0]));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = array.FindInOrderL(UInts[2]));
	test(trap==KErrNotFound);

	test.Next(_L("Test FindInOrderL(TUint anEntry, TInt& anIndex) const..."));
	TRAP(trap, array.FindInOrderL(UInts[0], ret));
	test(trap==0);
	test(ret==0);
	TRAP(trap, array.FindInOrderL(UInts[2], ret));
	test(trap==KErrNotFound);

	test.Next(_L("Test SpecificFindInOrderL(TUint anEntry, TInt aMode) const..."));
	TRAP(trap, ret = array.SpecificFindInOrderL(UInts[0], EArrayFindMode_First));
	test(trap==0);
	test(ret==0);
	TRAP(trap, ret = array.SpecificFindInOrderL(UInts[2], EArrayFindMode_First));
	test(trap==KErrNotFound);

	test.Next(_L("Test SpecificFindInOrderL(TUint anEntry, TInt& anIndex, TInt aMode) const..."));
	TRAP(trap, array.SpecificFindInOrderL(UInts[0], ret, EArrayFindMode_First));
	test(trap==0);
	test(ret==0);
	TRAP(trap, array.SpecificFindInOrderL(UInts[2], ret, EArrayFindMode_First));
	test(trap==KErrNotFound);

	test.Next(_L("Test InsertInOrderL(TUint anEntry)..."));
	TRAP(trap, array.InsertInOrderL(UInts[0]));
	test(trap==KErrAlreadyExists);
	TRAP(trap, array.InsertInOrderL(UInts[2]));
	test(trap==KErrNone);
	array.Remove(2);

	test.Next(_L("Test InsertInOrderAllowRepeatsL(TUint anEntry)..."));
	TRAP(trap, array.InsertInOrderAllowRepeatsL(UInts[2]));
	test(trap==KErrNone);
	array.Remove(2);

	CleanupStack::PopAndDestroy(&array);
	test.End();
	}
