// Copyright (c) 2005-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/buffer/t_hashtab.cpp
// 
//

#include <e32test.h>
#include <e32hashtab.h>
#include <hal.h>

#if defined(__VC32__) || defined(__CW32__)
typedef unsigned short wch;
#else
typedef wchar_t wch;
#endif


RTest test(_L("T_HASHTAB"));
TInt NanoTickPeriod;

typedef TBuf8<80> TTestName;
typedef RHashSet<TInt> TIntSet;
typedef RHashSet<TTestName> TNameSet;
typedef RPtrHashSet<TDesC8> TStringSet8;
typedef RPtrHashSet<TDesC16> TStringSet16;
typedef RPtrHashMap<TDesC8,TDesC8> TStringMap8;
typedef RPtrHashMap<TDesC16,TDesC16> TStringMap16;

#define INTSET(x)	TIntSet x
#define NAMESET(x)	TNameSet x(&HashTestName, &TestNameIdentity)
#define STRSET8(x)	TStringSet8 x
#define STRSET16(x)	TStringSet16 x
#define STRMAP8(x)	TStringMap8 x
#define STRMAP16(x)	TStringMap16 x

RPointerArray<TDesC8> DesC8Array;
RPointerArray<TDesC16> DesC16Array;

class HashTest : public RHashTableBase
	{
public:
	HashTest() : RHashTableBase(0,0,0,0) {}
	using RHashTableBase::ConsistencyCheck;
	using RHashTableBase::Count;
	};

void CCheck(RHashTableBase& aHT)
	{
	TUint32 ci[1024];
	TUint32 dc;
	TUint32 cmp;
	HashTest* h = (HashTest*)&aHT;
	h->ConsistencyCheck(&dc, &cmp, 1024, ci);
	test.Printf(_L("Count = %d, Deleted = %d, Cmp = %d\n"), h->Count(), dc, cmp);
	TInt i;
	for (i=0; i<1024; ++i)
		{
		if (ci[i])
			test.Printf(_L("%d chains of length %d\n"), ci[i], i);
		}
	}

const char* Numbers[] =
	{
	"zero",
	"one",
	"two",
	"three",
	"four",
	"five",
	"six",
	"seven",
	"eight",
	"nine",
	"ten",
	"eleven",
	"twelve",
	"thirteen",
	"fourteen",
	"fifteen",
	"sixteen",
	"seventeen",
	"eighteen",
	"nineteen",
	"twenty",
	"thirty",
	"forty",
	"fifty",
	"sixty",
	"seventy",
	"eighty",
	"ninety",
	"hundred",
	"thousand"
	};

TTestName NumberInWords(const TInt& aNum)
	{
	TInt a = aNum;
	TTestName n;
	if (a<20)
		{
		n.Copy((const TUint8*)Numbers[a]);
		return n;
		}
	if (a<100)
		{
		TInt tens = a/10;
		TInt units = a%10;
		n.Copy((const TUint8*)Numbers[tens-2+20]);
		if (units)
			{
			n.Append(' ');
			n.Append(TPtrC8((const TUint8*)Numbers[units]));
			}
		return n;
		}
	if (a<1000)
		{
		TInt h = a/100;
		n.Copy((const TUint8*)Numbers[h]);
		n.Append(' ');
		n.Append(TPtrC8((const TUint8*)Numbers[28]));
		a%=100;
		if (a)
			{
			TTestName n2(NumberInWords(a));
			n.Append(_L8(" and "));
			n+=n2;
			}
		return n;
		}
	TInt t = a/1000;
	n = NumberInWords(t);
	n.Append(' ');
	n.Append(TPtrC8((const TUint8*)Numbers[29]));
	a%=1000;
	if (a)
		{
		TTestName n2(NumberInWords(a));
		if (a<100)
			n.Append(_L8(" and "));
		else
			n.Append(' ');
		n+=n2;
		}
	return n;
	}

void PrintNumberInWords(TInt a)
	{
	TBuf<256> buf;
	TTestName n(NumberInWords(a));
	buf.Copy(n);
	test.Printf(_L("%d: %S\n"), a, &buf);
	}

TUint32 HashTestName(const TTestName& aN)
	{
	return DefaultHash::Des8(aN);
	}

TBool TestNameIdentity(const TTestName& aA, const TTestName& aB)
	{
	return aA == aB;
	}

/******************************************************************************
 * Utility functions for RHashSet<T>
 ******************************************************************************/
template <class T>
void Union(RHashSet<T>& aD, const RHashSet<T>& aS)
	{
	TInt c = aS.Count();
	TInt c2 = c;
	TInt d0 = aD.Count();
	TInt d1 = d0;
	typename RHashSet<T>::TIter iS(aS);
	FOREVER
		{
		const T* p = iS.Next();
		if (!p)
			break;
		--c2;
		TInt r = aD.Insert(*p);
		test(r==KErrNone);
		++d1;
		}
	test(d1 == aD.Count());
	test(c2 == 0);
	}

template <class T>
void Subtract(RHashSet<T>& aD, const RHashSet<T>& aS)
	{
	TInt c = aS.Count();
	TInt c2 = c;
	TInt d0 = aD.Count();
	TInt d1 = d0;
	TInt nfd = 0;
	typename RHashSet<T>::TIter iS(aS);
	FOREVER
		{
		const T* p = iS.Next();
		if (!p)
			break;
		--c2;
		TInt r = aD.Remove(*p);
		test(r==KErrNone || r==KErrNotFound);
		if (r==KErrNotFound)
			++nfd;
		else
			--d1;
		}
	test(d1 == aD.Count());
	test(c2 == 0);
	test( (d0-d1) + nfd == c);
	}

template <class T>
void Intersect(RHashSet<T>& aD, const RHashSet<T>& aS)
	{
	typename RHashSet<T>::TIter iD(aD);
	FOREVER
		{
		const T* p = iD.Next();
		if (!p)
			break;
		if (!aS.Find(*p))
			iD.RemoveCurrent();
		}
	}

template <class T>
void CheckIdentical(const RHashSet<T>& aA, const RHashSet<T>& aB)
	{
	test(aA.Count()==aB.Count());
	TInt c = aA.Count();
	typename RHashSet<T>::TIter iA(aA);
	FOREVER
		{
		const T* p = iA.Next();
		if (!p)
			break;
		--c;
		test(aB.Find(*p)!=0);
		}
	test(c==0);
	c = aA.Count();
	typename RHashSet<T>::TIter iB(aB);
	FOREVER
		{
		const T* p = iB.Next();
		if (!p)
			break;
		--c;
		test(aA.Find(*p)!=0);
		}
	test(c==0);
	}


/******************************************************************************
 * Utility functions for RPtrHashSet<T>
 ******************************************************************************/
template <class T>
void Union(RPtrHashSet<T>& aD, const RPtrHashSet<T>& aS)
	{
	TInt c = aS.Count();
	TInt c2 = c;
	TInt d0 = aD.Count();
	TInt d1 = d0;
	typename RPtrHashSet<T>::TIter iS(aS);
	FOREVER
		{
		const T* p = iS.Next();
		if (!p)
			break;
		--c2;
		TInt r = aD.Insert(p);
		test(r==KErrNone);
		++d1;
		}
	test(d1 == aD.Count());
	test(c2 == 0);
	}

template <class T>
void Subtract(RPtrHashSet<T>& aD, const RPtrHashSet<T>& aS)
	{
	TInt c = aS.Count();
	TInt c2 = c;
	TInt d0 = aD.Count();
	TInt d1 = d0;
	TInt nfd = 0;
	typename RPtrHashSet<T>::TIter iS(aS);
	FOREVER
		{
		const T* p = iS.Next();
		if (!p)
			break;
		--c2;
		TInt r = aD.Remove(p);
		test(r==KErrNone || r==KErrNotFound);
		if (r==KErrNotFound)
			++nfd;
		else
			--d1;
		}
	test(d1 == aD.Count());
	test(c2 == 0);
	test( (d0-d1) + nfd == c);
	}

template <class T>
void Intersect(RPtrHashSet<T>& aD, const RPtrHashSet<T>& aS)
	{
	typename RPtrHashSet<T>::TIter iD(aD);
	FOREVER
		{
		const T* p = iD.Next();
		if (!p)
			break;
		if (!aS.Find(*p))
			iD.RemoveCurrent();
		}
	}

template <class T>
void CheckIdentical(const RPtrHashSet<T>& aA, const RPtrHashSet<T>& aB)
	{
	test(aA.Count()==aB.Count());
	TInt c = aA.Count();
	typename RPtrHashSet<T>::TIter iA(aA);
	FOREVER
		{
		const T* p = iA.Next();
		if (!p)
			break;
		--c;
		test(aB.Find(*p)!=0);
		}
	test(c==0);
	c = aA.Count();
	typename RPtrHashSet<T>::TIter iB(aB);
	FOREVER
		{
		const T* p = iB.Next();
		if (!p)
			break;
		--c;
		test(aA.Find(*p)!=0);
		}
	test(c==0);
	}


/******************************************************************************
 * Utility functions for RHashMap<K,V>
 ******************************************************************************/
template <class K, class V>
void UnionTransform(RHashMap<K,V>& aD, const RHashSet<K>& aS, V (*aTransform)(const K&) )
	{
	TInt c = aS.Count();
	TInt c2 = c;
	TInt d0 = aD.Count();
	TInt d1 = d0;
	typename RHashSet<K>::TIter iS(aS);
	FOREVER
		{
		const K* p = iS.Next();
		if (!p)
			break;
		--c2;
		TInt r = aD.Insert(*p, (*aTransform)(*p) );
		test(r==KErrNone);
		++d1;
		}
	test(d1 == aD.Count());
	test(c2 == 0);
	}


template <class K, class V>
void UnionMap(RHashSet<V>& aD, const RHashSet<K>& aS, const RHashMap<K,V>& aM)
	{
	TInt c = aS.Count();
	TInt c2 = c;
	TInt d0 = aD.Count();
	TInt d1 = d0;
	typename RHashSet<K>::TIter iS(aS);
	FOREVER
		{
		const K* p = iS.Next();
		if (!p)
			break;
		--c2;
		const V* q = aM.Find(*p);
		if (!q)
			continue;
		TInt r = aD.Insert(*q);
		test(r==KErrNone);
		++d1;
		}
	test(d1 == aD.Count());
	test(c2 == 0);
	}


template <class K, class V>
void UnionKeys(RHashSet<K>& aD, const RHashMap<K,V>& aM)
	{
	typename RHashMap<K,V>::TIter iM(aM);
	FOREVER
		{
		const K* p = iM.NextKey();
		if (!p)
			break;
		aD.Insert(*p);
		}
	}


template <class K, class V>
void UnionValues(RHashSet<V>& aD, const RHashMap<K,V>& aM)
	{
	typename RHashMap<K,V>::TIter iM(aM);
	FOREVER
		{
		const K* p = iM.NextKey();
		if (!p)
			break;
		const V* q = iM.CurrentValue();
		aD.Insert(*q);
		}
	}


template <class K, class V>
void UnionTransform(RHashMap<K,V>& aD, const RHashMap<K,V>& aS, V (*aTransform)(const V&) )
	{
	TInt c = aS.Count();
	TInt c2 = c;
	TInt d0 = aD.Count();
	TInt d1 = d0;
	typename RHashMap<K,V>::TIter iS(aS);
	FOREVER
		{
		const K* p = iS.NextKey();
		if (!p)
			break;
		--c2;
		TInt r = aD.Insert(*p, (*aTransform)(*iS.CurrentValue()) );
		test(r==KErrNone);
		++d1;
		}
	test(d1 == aD.Count());
	test(c2 == 0);
	}


template <class K, class V>
void UnionInverse(RHashMap<V,K>& aD, const RHashMap<K,V>& aS)
	{
	TInt c = aS.Count();
	TInt c2 = c;
	TInt d0 = aD.Count();
	TInt d1 = d0;
	typename RHashMap<K,V>::TIter iS(aS);
	FOREVER
		{
		const K* p = iS.NextKey();
		if (!p)
			break;
		--c2;
		const V* q = iS.CurrentValue();
		TInt r = aD.Insert(*q, *p);
		test(r==KErrNone);
		++d1;
		}
	test(d1 == aD.Count());
	test(c2 == 0);
	}

template <class K, class V>
void SetMap(RHashMap<V,K>& aM, const V& aV)
	{
	typename RHashMap<K,V>::TIter iM(aM);
	FOREVER
		{
		const K* p = iM.NextKey();
		if (!p)
			break;
		V* q = iM.CurrentValue();
		*q = aV;
		test(*q == aV);
		}
	}


/******************************************************************************
 * Utility functions for RPtrHashMap<K,V>
 ******************************************************************************/
template <class K, class V>
void UnionTransform(RPtrHashMap<K,V>& aD, const RPtrHashMap<K,V>& aS, V* (*aTransform)(const V*) )
	{
	TInt c = aS.Count();
	TInt c2 = c;
	TInt d0 = aD.Count();
	TInt d1 = d0;
	typename RPtrHashMap<K,V>::TIter iS(aS);
	FOREVER
		{
		const K* p = iS.NextKey();
		if (!p)
			break;
		--c2;
		TInt r = aD.Insert(p, (*aTransform)(iS.CurrentValue()) );
		test(r==KErrNone);
		++d1;
		}
	test(d1 == aD.Count());
	test(c2 == 0);
	}


template <class A, class B>
void UnionInverse(RPtrHashMap<B,A>& aD, const RPtrHashMap<A,B>& a1)
	{
	typename RPtrHashMap<A,B>::TIter iter(a1);
	const A* pA;
	while ( (pA=iter.NextKey()) != 0 )
		{
		const B* pB = iter.CurrentValue();
		TInt r = aD.Insert(pB, pA);
		test(r==KErrNone);
		}
	}


template <class A, class B, class C>
void UnionCompose(RPtrHashMap<A,C>& aD, const RPtrHashMap<A,B>& a1, const RPtrHashMap<B,C>& a2)
	{
	typename RPtrHashMap<A,B>::TIter iter(a1);
	const A* pA;
	while ( (pA=iter.NextKey()) != 0 )
		{
		const B* pB = iter.CurrentValue();
		const C* pC = a2.Find(*pB);
		if (pC)
			{
			TInt r = aD.Insert(pA, pC);
			test(r==KErrNone);
			}
		}
	}


template <class K, class V>
void CheckIdenticalMaps(const RPtrHashMap<K,V>& aA, const RPtrHashMap<K,V>& aB)
	{
	test(aA.Count()==aB.Count());
	TInt c = aA.Count();
	const K* p;
	const V* q;
	typename RPtrHashMap<K,V>::TIter iA(aA);
	while( (p=iA.NextKey()) != 0)
		{
		--c;
		q = aB.Find(*p);
		test(q && q==iA.CurrentValue());
		}
	test(c==0);
	c = aB.Count();
	typename RPtrHashMap<K,V>::TIter iB(aB);
	while( (p=iB.NextKey()) != 0)
		{
		--c;
		q = aA.Find(*p);
		test(q && q==iB.CurrentValue());
		}
	test(c==0);
	}





/******************************************************************************
 * Tests of TIntSet
 ******************************************************************************/
void Populate(TIntSet& aH, TInt aS, TInt aE, TInt aM)
	{
	TInt x = aS;
	for (; x<=aE; x+=aM)
		aH.Insert(x);
	}

void PrimeSieve(TIntSet& aH, TInt aStart, TInt aEnd)
	{
	TInt x;
	TInt e = (aEnd&1) ? aEnd : aEnd-1;
	TInt s = (aStart<2) ? 2 : aStart|1;
	for (x=s; x<=e; ++x)
		{
//		test.Printf(_L("add %d\n"),x);
		aH.Insert(x);
		}
	TInt m=2;
	FOREVER
		{
		for (; m*m<=e && !aH.Find(m); ++m)
			{}
		if (m*m > e)
			break;
		for (x=m*2; x<=e; x+=m)
			{
//			test.Printf(_L("remove %d\n"),x);
			aH.Remove(x);
			}
		++m;
		}
	}

TBool IsPrime(TInt x)
	{
	if (x==2)
		return ETrue;
	if (!(x&1))
		return EFalse;
	TInt d;
	for (d=3; d*d<=x; d+=2)
		{
		if (x%d==0)
			return EFalse;
		}
	return ETrue;
	}

void CheckSieveOutput(TIntSet& aH)
	{
	RHashSet<TInt>::TIter iter(aH);
	TInt min = KMaxTInt;
	TInt max = KMinTInt;
	TInt c = 0;
	test.Printf(_L("%d elements:\n"),aH.Count());
	FOREVER
		{
		const TInt* p = iter.Next();
		if (!p)
			break;
		++c;
		if (*p>max) max=*p;
		if (*p<min) min=*p;
//		test.Printf(_L("%d\n"),*p);
		test(IsPrime(*p));
		}
	test(c==aH.Count());
	TInt x;
	if (min==2)
		{
		test(aH.Find(2)!=0);
		min++;
		}
	for (x=min; x<=max; x+=2)
		{
		if (IsPrime(x))
			test(aH.Find(x)!=0);
		}
	for (x=min; x<=max; x+=2)
		{
		if (IsPrime(x))
			test(aH.Find(x)==&aH.FindL(x));
		else
			{
			TRAPD(rr,aH.FindL(x));
			test(rr==KErrNotFound);
			}
		}
	}

TUint32 CheckSmallHashSet(const TIntSet& aA)
	{
	TUint32 m = 0;
	RHashSet<TInt>::TIter iter(aA);
	FOREVER
		{
		const TInt* p = iter.Next();
		if (!p)
			break;
		m |= (1<<*p);
		}
	return m;
	}

void AddToSmallHashSet(TIntSet& aA, TUint32 aMask)
	{
	CheckSmallHashSet(aA);
	TInt i;
	TInt r;
	for (i=0; i<32; ++i)
		{
		if (aMask & (1<<i))
			{
			r = aA.Insert(i);
			test(r==KErrNone);
			}
		}
	}

void RemoveFromSmallHashSet(TIntSet& aA, TUint32 aMask)
	{
	TUint32 m = CheckSmallHashSet(aA);
	TInt i;
	TInt r;
	for (i=0; i<32; ++i)
		{
		if (aMask & (1<<i))
			{
			r = aA.Remove(i);
			if (m & (1<<i))
				test(r==KErrNone);
			else
				test(r==KErrNotFound);
			}
		}
	}

void TestHashSet()
	{
	test.Next(_L("Test RHashSet"));

	INTSET(hs);
	CCheck(hs);		// check consistency for empty table
	INTSET(hs2);
	INTSET(hs3);
	PrimeSieve(hs, 1, 100);
	CheckSieveOutput(hs);
	test(hs.Reserve(1000)==KErrNone);
	CCheck(hs);
	CheckSieveOutput(hs);	// check that Reserve() preserves existing entries

	INTSET(m1);
	INTSET(m2);
	INTSET(m3);
	INTSET(m5);
	INTSET(m7);
	Populate(m1,2,100,1);
	Populate(m2,4,100,2);
	Populate(m3,6,100,3);
	Populate(m5,10,100,5);
	Populate(m7,14,100,7);
	Union(hs3, m1);
	Subtract(hs3, m2);
	Subtract(hs3, m3);
	Subtract(hs3, m5);
	Subtract(hs3, m7);
	CheckSieveOutput(hs3);
	CheckIdentical(hs,hs3);
	hs3.Close();
	INTSET(cm2);
	INTSET(cm3);
	INTSET(cm5);
	INTSET(cm7);
	Union(cm2, m1);
	Subtract(cm2, m2);
	Union(cm3, m1);
	Subtract(cm3, m3);
	Union(cm5, m1);
	Subtract(cm5, m5);
	Union(cm7, m1);
	Subtract(cm7, m7);
	Union(hs3, m1);
	Intersect(hs3, cm7);
	Intersect(hs3, cm3);
	Intersect(hs3, cm5);
	Intersect(hs3, cm2);
	CheckSieveOutput(hs3);
	CheckIdentical(hs,hs3);
	hs3.Close();


	cm2.Close();
	cm3.Close();
	cm5.Close();
	cm7.Close();
	m1.Close();
	m2.Close();
	m3.Close();
	m5.Close();
	m7.Close();

	PrimeSieve(hs2, 1, 1000);
	CheckSieveOutput(hs2);
	test(hs2.Reserve(1000)==KErrNone);
	CCheck(hs2);
	CheckSieveOutput(hs2);	// check that Reserve() preserves existing entries

	PrimeSieve(hs, 100, 997);
	CheckSieveOutput(hs);
	CCheck(hs);
	CheckIdentical(hs,hs2);

	hs2.Close();
	Union(hs2, hs);
	CheckIdentical(hs,hs2);
	CheckSieveOutput(hs2);

	hs2.Close();
	hs.Close();

	test(CheckSmallHashSet(hs)==0);
	AddToSmallHashSet(hs, 1);
	test(CheckSmallHashSet(hs)==1);
	AddToSmallHashSet(hs, 4);
	test(CheckSmallHashSet(hs)==5);
	AddToSmallHashSet(hs, 0x80000001);
	test(CheckSmallHashSet(hs)==0x80000005);
	AddToSmallHashSet(hs, 0x317217f8);
	test(CheckSmallHashSet(hs)==0xb17217fd);
	RemoveFromSmallHashSet(hs, 0x00000007);
	test(CheckSmallHashSet(hs)==0xb17217f8);
	RemoveFromSmallHashSet(hs, 0xffffffff);
	test(CheckSmallHashSet(hs)==0);
	hs.Close();
	}

void TestHashIter()
	{
	test.Next(_L("Test iterators"));

	RHashSet<TInt> hs;		// empty
	RHashSet<TInt>::TIter iter(hs);
	test(iter.Next() == 0);
	test(iter.Current() == 0);
	iter.RemoveCurrent();
	test(iter.Next() == 0);
	test(iter.Current() == 0);

	test(hs.Insert(1) == KErrNone);
	test(hs.Insert(2) == KErrNone);
	test(hs.Insert(3) == KErrNone);
	iter.Reset();
	TUint mask = 14;
	TInt i;
	for (i=0; i<3; ++i)
		{
		const TInt* p = iter.Next();
		test(p!=0);
		TInt x = *p;
		test (x>=1 && x<=3 && (mask&(1<<x))!=0);
		mask &= ~(1<<x);
		}
	test(iter.Next() == 0);
	test(mask==0);
	test(CheckSmallHashSet(hs)==0x0e);
	test(hs.Insert(4) == KErrNone);
	test(hs.Insert(5) == KErrNone);
	test(hs.Insert(6) == KErrNone);
	test(hs.Insert(7) == KErrNone);
	test(CheckSmallHashSet(hs)==0xfe);
	iter.Reset();
	while(iter.Next())
		{
		if ((*iter.Current() & 1) == 0)
			iter.RemoveCurrent();
		}
	test(CheckSmallHashSet(hs)==0xaa);
	iter.Reset();
	while(iter.Next())
		{
		iter.RemoveCurrent();
		}
	test(CheckSmallHashSet(hs)==0);
	iter.Reset();
	test(iter.Next() == 0);
	test(iter.Current() == 0);
	RHashSet<TInt> empty;
	CheckIdentical(hs, empty);
#ifdef _DEBUG
	__UHEAP_FAILNEXT(1);
	test(empty.Insert(1)==KErrNoMemory);
	test(empty.Insert(1)==KErrNone);
	empty.Close();
	__UHEAP_FAILNEXT(1);
	test(hs.Insert(1)==KErrNone);
	hs.Close();
	__UHEAP_FAILNEXT(1);
	test(hs.Insert(1)==KErrNoMemory);
#endif
	hs.Close();
	}

void Print(const char* aTitle, const RHashMap<TInt,TTestName>& aM)
	{
	TBuf<256> buf;
	buf.Copy(TPtrC8((const TUint8*)aTitle));
	test.Printf(_L("%S\n"), &buf);
	RHashMap<TInt,TTestName>::TIter iter(aM);
	FOREVER
		{
		const TInt* p = iter.NextKey();
		if (!p) break;
		buf.Copy(*iter.CurrentValue());
		test.Printf(_L("%d: %S\n"), *p, &buf);
		}
	}

void Print(const char* aTitle, const RHashSet<TTestName>& aS)
	{
	TBuf<256> buf;
	buf.Copy(TPtrC8((const TUint8*)aTitle));
	test.Printf(_L("%S\n"), &buf);
	RHashSet<TTestName>::TIter iter(aS);
	FOREVER
		{
		if (!iter.Next())
			break;
		buf.Copy(*iter.Current());
		test.Printf(_L("%S\n"), &buf);
		}
	}

void TestHashMap()
	{
	test.Next(_L("Test RHashMap"));

	RHashMap<TInt,TInt> ht;
	CCheck(ht);		// check consistency for empty table
	TInt x;
	for (x=0; x<200; x++)
		{
		TInt r = ht.Insert(x*x, x);
		test(r==KErrNone);
		}
	test(ht.Count()==200);

	TInt z = 0;
	TInt y;
	for (x=0; x<40000; x++)
		{
		const TInt* e = ht.Find(x);
		if (e)
			{
			const TInt& ee = ht.FindL(x);
			test(&ee==e);
			y = *e;
//			test.Printf(_L("Find(%d) -> %d\n"), x, y);
			test(x == z*z);
			test(y == z);
			++z;
			}
		else
			{
			TRAPD(rr, ht.FindL(x));
			test(rr==KErrNotFound);
			}
		}
	CCheck(ht);

	for (x=0; x<200; x++)
		{
		TInt r = ht.Insert(x*x*x, x);
		test(r==KErrNone);
		}
	test(ht.Count()==200*2-6);
	CCheck(ht);

	TInt sq = 0;
	TInt cb = 0;
	for (x=0; x<8000000; x++)
		{
		const TInt* e = ht.Find(x);
		if (e)
			{
			const TInt& ee = ht.FindL(x);
			test(&ee==e);
			y = *e;
//			test.Printf(_L("Find(%d) -> %d\n"), x, y);
			if (x == cb*cb*cb)
				{
				test(y==cb);
				++cb;
				if (x == sq*sq)
					++sq;
				}
			else if (x == sq*sq)
				{
				test(y==sq);
				++sq;
				}
			}
		}

	for (x=0; x<200; x++)
		{
		TInt r = ht.Remove(x*x);
		test(r==KErrNone);
		}
	test(ht.Count()==200-6);
	CCheck(ht);

	cb = 2;
	for (x=2; x<8000000; x++)
		{
		const TInt* e = ht.Find(x);
		if (e)
			{
			const TInt& ee = ht.FindL(x);
			test(&ee==e);
			y = *e;
//			test.Printf(_L("Find(%d) -> %d\n"), x, y);
			if (cb == 4 || cb == 9 || cb == 16 || cb == 25) ++cb;
			test(x == cb*cb*cb);
			test(y == cb);
			++cb;
			}
		}

	SetMap<TInt,TInt>(ht, 17);

	ht.Close();

	PrintNumberInWords(2000);
	PrintNumberInWords(2001);
	PrintNumberInWords(131072);
	PrintNumberInWords(111111);
	PrintNumberInWords(524288);

	INTSET(all);
	Populate(all,0,1000,1);
	RHashMap<TInt,TTestName> to_words(&DefaultHash::Integer, &DefaultIdentity::Integer);
	UnionTransform<TInt,TTestName>(to_words, all, &NumberInWords);
	RHashMap<TTestName,TInt> from_words(&HashTestName, &TestNameIdentity);
	UnionInverse<TInt,TTestName>(from_words, to_words);
//	Print("TO WORDS:", to_words);
	INTSET(primes);
	PrimeSieve(primes, 1, 100);
//	Print("Primes 1-100", primes);
	RHashMap<TInt,TTestName> prime_map(&DefaultHash::Integer, &DefaultIdentity::Integer);
	UnionTransform<TInt,TTestName>(prime_map, primes, &NumberInWords);
//	Print("Prime map 1-100", prime_map);
	INTSET(pmkeys);
	UnionKeys<TInt,TTestName>(pmkeys, prime_map);
	NAMESET(pmval);
	NAMESET(pmval2);
	UnionValues<TInt,TTestName>(pmval, prime_map);
	CheckIdentical(pmkeys, primes);
	INTSET(pr2);
	UnionMap<TTestName,TInt>(pr2, pmval, from_words);
	CheckIdentical(pr2, primes);
	pr2.Close();
	Union(pmval2, pmval);
//	Print("pmval",pmval);
//	Print("pmval2",pmval2);
	CheckIdentical(pmval2, pmval);
	UnionMap<TTestName,TInt>(pr2, pmval2, from_words);
	CheckIdentical(pr2, primes);

	pr2.Close();
	pmval.Close();
	pmval2.Close();
	pmkeys.Close();
	prime_map.Close();
	primes.Close();
	all.Close();

	INTSET(m);
	Populate(all,2,1000,1);

	NAMESET(pr3);
	NAMESET(nm);
	UnionMap<TInt,TTestName>(pr3, all, to_words);
	all.Close();
	CCheck(pr3);

	Populate(m,4,1000,2);
	UnionMap<TInt,TTestName>(nm, m, to_words);
	m.Close();
	Subtract(pr3, nm);
	nm.Close();

	Populate(m,6,1000,3);
	UnionMap<TInt,TTestName>(nm, m, to_words);
	m.Close();
	Subtract(pr3, nm);
	nm.Close();

	Populate(m,10,1000,5);
	UnionMap<TInt,TTestName>(nm, m, to_words);
	m.Close();
	Subtract(pr3, nm);
	nm.Close();

	Populate(m,14,1000,7);
	UnionMap<TInt,TTestName>(nm, m, to_words);
	m.Close();
	Subtract(pr3, nm);
	nm.Close();

	Populate(m,22,1000,11);
	UnionMap<TInt,TTestName>(nm, m, to_words);
	m.Close();
	Subtract(pr3, nm);
	nm.Close();

	Populate(m,26,1000,13);
	UnionMap<TInt,TTestName>(nm, m, to_words);
	m.Close();
	Subtract(pr3, nm);
	nm.Close();

	Populate(m,34,1000,17);
	UnionMap<TInt,TTestName>(nm, m, to_words);
	m.Close();
	Subtract(pr3, nm);
	nm.Close();

	Populate(m,38,1000,19);
	UnionMap<TInt,TTestName>(nm, m, to_words);
	m.Close();
	Subtract(pr3, nm);
	nm.Close();

	Populate(m,46,1000,23);
	UnionMap<TInt,TTestName>(nm, m, to_words);
	m.Close();
	Subtract(pr3, nm);
	nm.Close();

	Populate(m,58,1000,29);
	UnionMap<TInt,TTestName>(nm, m, to_words);
	m.Close();
	Subtract(pr3, nm);
	nm.Close();

	Populate(m,62,1000,31);
	UnionMap<TInt,TTestName>(nm, m, to_words);
	m.Close();
	Subtract(pr3, nm);
	nm.Close();

//	Print("pr3",pr3);
	PrimeSieve(primes, 1, 1000);
	UnionMap<TInt,TTestName>(nm, primes, to_words);
	CheckIdentical(nm, pr3);
	CCheck(pr3);
	UnionMap<TTestName,TInt>(m, pr3, from_words);
	CheckIdentical(m, primes);

	m.Close();
	nm.Close();
	primes.Close();
	pr3.Close();
	from_words.Close();
	to_words.Close();
	}

void PopulateArray8(TInt aMax)
	{
	TInt i;
	for (i=0; i<aMax; ++i)
		{
		TTestName n(NumberInWords(i));
		HBufC8* p = n.Alloc();
		test(p!=0);
		TInt r = DesC8Array.Append(p);
		test(r==KErrNone);
		}
	}

void PopulateArray16(TInt aMax)
	{
	TInt i;
	for (i=0; i<aMax; ++i)
		{
		TTestName n(NumberInWords(i));
		TBuf<128> n16;
		n16.Copy(n);
		HBufC16* p = n16.Alloc();
		test(p!=0);
		TInt r = DesC16Array.Append(p);
		test(r==KErrNone);
		}
	}

TUint32 istrh8(const TDesC8* const & a)
	{
	return DefaultHash::Des8(*a);
	}

TBool istrid8(const TDesC8* const & aA, const TDesC8* const & aB)
	{
	return *aA == *aB;
	}

TUint32 istrh16(const TDesC16* const & a)
	{
	return DefaultHash::Des16(*a);
	}

TBool istrid16(const TDesC16* const & aA, const TDesC16* const & aB)
	{
	return *aA == *aB;
	}

void TestPtrHashSet()
	{
	test.Next(_L("Test RPtrHashSet"));

	STRSET8(s);
	CCheck(s);		// check consistency for empty table
	RHashMap<const TDesC8*, TInt> hm(&istrh8, &istrid8);
	RPtrHashSet<TDesC8>::TIter iter(s);
	const TDesC8* q;
	TInt i;
	for (i=0; i<4096; ++i)
		{
		TInt r = s.Insert(DesC8Array[i]);
		test(r==KErrNone);
		r = hm.Insert(DesC8Array[i], i);
		test(r==KErrNone);
		}
	CCheck(s);
	for (i=0; i<4100; ++i)
		{
		TTestName n(NumberInWords(i));
		const TDesC8* p = s.Find(n);
		if (i<4096)
			{
			const TDesC8& pp = s.FindL(n);
			test(p && *p==n && p==DesC8Array[i]);
			test(&pp == p);
			}
		else
			{
			test(!p);
			TRAPD(rr,s.FindL(n));
			test(rr==KErrNotFound);
			}
		}
	while((q=iter.Next())!=0)
		{
		const TInt* qi = hm.Find(q);
		test(qi!=0 && *qi>=0 && *qi<4096);
		test(DesC8Array[*qi]==q);
		}
	for (i=0; i<4100; i+=2)
		{
		TTestName n(NumberInWords(i));
		TInt r = s.Remove(&n);
		if (i<4096)
			test(r==KErrNone);
		else
			test(r==KErrNotFound);
		}
	test(s.Count()==2048);
	CCheck(s);
	for (i=0; i<4100; ++i)
		{
		TTestName n(NumberInWords(i));
		const TDesC8* p = s.Find(n);
		if (i<4096 && (i&1))
			test(p && *p==n && p==DesC8Array[i]);
		else
			test(!p);
		}
	iter.Reset();
	while((q=iter.Next())!=0)
		{
		const TInt* qi = hm.Find(q);
		test(qi!=0 && *qi>=0 && *qi<4096 && (*qi&1));
		test(DesC8Array[*qi]==q);
		}
	for (i=2; i<4096; i+=4)
		{
		TInt r = s.Insert(DesC8Array[i]);
		test(r==KErrNone);
		}
	test(s.Count()==3072);
	CCheck(s);
	for (i=0; i<4100; ++i)
		{
		TTestName n(NumberInWords(i));
		const TDesC8* p = s.Find(n);
		if (i<4096 && (i&3))
			test(p && *p==n && p==DesC8Array[i]);
		else
			test(!p);
		}
	iter.Reset();
	while((q=iter.Next())!=0)
		{
		const TInt* qi = hm.Find(q);
		test(qi!=0 && *qi>=0 && *qi<4096 && (*qi&3));
		test(DesC8Array[*qi]==q);
		}
	s.Close();

	// test ResetAndDestroy
	for (i=0; i<16; ++i)
		{
		TInt r = s.Insert(DesC8Array[i]->Alloc());
		test(r==KErrNone);
		}
	iter.Reset();
	while((q=iter.Next())!=0)
		{
		const TInt* qi = hm.Find(q);
		test(qi!=0 && *qi>=0 && *qi<16);
		test(*DesC8Array[*qi]==*q);
		test(DesC8Array[*qi]!=q);
		}
	s.ResetAndDestroy();
	hm.Close();


	STRSET16(S);
	RHashMap<const TDesC16*, TInt> HM(&istrh16, &istrid16);
	RPtrHashSet<TDesC16>::TIter ITER(S);
	const TDesC16* Q;
	for (i=0; i<4096; ++i)
		{
		TInt r = S.Insert(DesC16Array[i]);
		test(r==KErrNone);
		r = HM.Insert(DesC16Array[i], i);
		test(r==KErrNone);
		}
	CCheck(S);
	for (i=0; i<4100; ++i)
		{
		TTestName n(NumberInWords(i));
		TBuf<80> buf;
		buf.Copy(n);
		const TDesC16* p = S.Find(buf);
		if (i<4096)
			test(p && *p==buf && p==DesC16Array[i]);
		else
			test(!p);
		}
	while((Q=ITER.Next())!=0)
		{
		const TInt* qi = HM.Find(Q);
		test(qi!=0 && *qi>=0 && *qi<4096);
		test(DesC16Array[*qi]==Q);
		}
	for (i=0; i<4100; i+=2)
		{
		TTestName n(NumberInWords(i));
		TBuf<80> buf;
		buf.Copy(n);
		TInt r = S.Remove(&buf);
		if (i<4096)
			test(r==KErrNone);
		else
			test(r==KErrNotFound);
		}
	test(S.Count()==2048);
	CCheck(S);
	for (i=0; i<4100; ++i)
		{
		TTestName n(NumberInWords(i));
		TBuf<80> buf;
		buf.Copy(n);
		const TDesC16* p = S.Find(buf);
		if (i<4096 && (i&1))
			test(p && *p==buf && p==DesC16Array[i]);
		else
			test(!p);
		}
	ITER.Reset();
	while((Q=ITER.Next())!=0)
		{
		const TInt* qi = HM.Find(Q);
		test(qi!=0 && *qi>=0 && *qi<4096 && (*qi&1));
		test(DesC16Array[*qi]==Q);
		}
	for (i=2; i<4096; i+=4)
		{
		TInt r = S.Insert(DesC16Array[i]);
		test(r==KErrNone);
		}
	test(S.Count()==3072);
	CCheck(S);
	for (i=0; i<4100; ++i)
		{
		TTestName n(NumberInWords(i));
		TBuf<80> buf;
		buf.Copy(n);
		const TDesC16* p = S.Find(buf);
		if (i<4096 && (i&3))
			test(p && *p==buf && p==DesC16Array[i]);
		else
			test(!p);
		}
	ITER.Reset();
	while((Q=ITER.Next())!=0)
		{
		const TInt* qi = HM.Find(Q);
		test(qi!=0 && *qi>=0 && *qi<4096 && (*qi&3));
		test(DesC16Array[*qi]==Q);
		}
	S.Close();

	// test ResetAndDestroy
	for (i=0; i<16; ++i)
		{
		TInt r = S.Insert(DesC16Array[i]->Alloc());
		test(r==KErrNone);
		}
	ITER.Reset();
	while((Q=ITER.Next())!=0)
		{
		const TInt* qi = HM.Find(Q);
		test(qi!=0 && *qi>=0 && *qi<16);
		test(*DesC16Array[*qi]==*Q);
		test(DesC16Array[*qi]!=Q);
		}
	S.ResetAndDestroy();
	HM.Close();
	}

void TestPtrHashMap()
	{
	test.Next(_L("Test RPtrHashMap"));

	STRMAP8(map);
	CCheck(map);		// check consistency for empty table
	STRMAP8(id);
	TInt i;
	for (i=0; i<4096; ++i)
		{
		TInt r = map.Insert(DesC8Array[i], DesC8Array[(i+1)&0xfff]);
		test(r==KErrNone);
		r = id.Insert(DesC8Array[i], DesC8Array[i]);
		test(r==KErrNone);
		}
	for (i=0; i<4096; ++i)
		{
		const TDesC8* p = map.Find(*DesC8Array[i]);
		test(p == DesC8Array[(i+1)&0xfff]);
		const TDesC8& pp = map.FindL(*DesC8Array[i]);
		test(&pp == p);
		}
	TRAPD(rr,map.FindL(_L8("two")));
	test(rr==KErrNone);
	TRAP(rr,map.FindL(_L8("twx")));
	test(rr==KErrNotFound);
	CCheck(map);
	STRMAP8(map2);
	STRMAP8(mapi);
	STRMAP8(map3);
	UnionInverse<TDesC8,TDesC8>(mapi, map);
	UnionCompose<TDesC8,TDesC8,TDesC8>(map2, map, map);
	UnionCompose<TDesC8,TDesC8,TDesC8>(map3, map, mapi);
	CheckIdenticalMaps<TDesC8,TDesC8>(map3, id);
	map3.Close();
	UnionCompose<TDesC8,TDesC8,TDesC8>(map3, map2, mapi);
	CheckIdenticalMaps<TDesC8,TDesC8>(map3, map);
	map3.Close();
	for (i=0; i<4096; ++i)
		{
		TInt r = map3.Insert(DesC8Array[i], DesC8Array[(i+2)&0xfff]);
		test(r==KErrNone);
		}
	CheckIdenticalMaps<TDesC8,TDesC8>(map3, map2);
	map3.Close();

	mapi.Close();
	map2.Close();
	id.Close();
	map.Close();

	// test ResetAndDestroy()
	for(i=0; i<16; ++i)
		{
		TInt r = map.Insert(DesC8Array[i]->Alloc(), DesC8Array[i*i]->Alloc());
		test(r==KErrNone);
		}
	map.ResetAndDestroy();
	}

void TestInsertL()
    {
    TInt i;
    TInt count=50;
    const TInt KMaxBufferSize=256;
    // Array used to hold test data to store in the hash objects
    RPointerArray<TDesC16> iPointerArray;
    
    for (i=0; i<count; ++i)
        {
        HBufC* hbuf = HBufC::NewLC(KMaxBufferSize);
        TPtr buf = hbuf->Des();
        TTestName number(NumberInWords(i));
        buf.Copy(number);
		// Append "one" "two" "three" .....
        iPointerArray.AppendL(hbuf);
        CleanupStack::Pop(hbuf);
        }
        
    // Creates an object ptrHashMap using the template class RPtrHashMap
    RPtrHashMap<TDesC16, TDesC16> ptrHashMap;
    // Push object on to the cleanup stack
    CleanupClosePushL(ptrHashMap);
    // Expand the array with the number of key-value pairs for which space should be allocated
    ptrHashMap.ReserveL(count);
    
    // Insert a key-value pairs into the array
    for (i=0; i<count; ++i)
        {
        // "zero"-"forty nine" 
        // "one"-"forty eight" .....
        ptrHashMap.InsertL(iPointerArray[i], iPointerArray[count-1-i]);
        }
    
    // Close and Cleanup
    CleanupStack::PopAndDestroy(&ptrHashMap);
    iPointerArray.ResetAndDestroy();
    }
                                       

void TestOOM()
	{
	// Max out memory and check it still works
	test.Next(_L("Test OOM"));

	TInt x = 0;
	TInt n = 0;
	TInt r;
	INTSET(set);
	FOREVER
		{
		x += 0x58b90bfb;
		r = set.Insert(x);
		if (r != KErrNone)
			break;
		++n;
		}
	test(r==KErrNoMemory);
	TRAPD(rr,set.InsertL(x));
	test(rr==KErrNoMemory);
	test.Printf(_L("%d integers stored\n"), n);
	test(set.Count()==n);
	TRAP(rr,set.InsertL(0x58b90bfb));	// already there
	test(rr==KErrNone);	// should succeed

	// final count should be a power of 2 minus 1
	test( (n&(n+1)) == 0 );

	x = 0;
	TInt i;
	for (i=0; i<=n; ++i)		// check everything has been stored correctly
		{
		x += 0x58b90bfb;
		const TInt* p = set.Find(x);
		if (i < n)
			test(p && *p == x);
		else
			test(!p);
		}
	set.Close();
	TInt nn;
	for (nn=256; nn<=n+256; nn+=256)
		{
		r = set.Reserve(nn);
		set.Close();
		if (r!=KErrNone)
			break;
		}
	test.Printf(_L("Max reserve %d\n"),nn);
	TInt thresh = 3*((n+1)>>2);
	test(nn == thresh + 256);
	}

class RDummyAllocator : public RAllocator
	{
public:
	virtual TAny* Alloc(TInt) {test(0); return 0;}
	virtual void Free(TAny*) {test(0);}
	virtual TAny* ReAlloc(TAny*, TInt, TInt) {test(0); return 0;}
	virtual TInt AllocLen(const TAny*) const  {test(0); return 0;}
	virtual TInt Compress() {test(0); return 0;}
	virtual void Reset() {test(0);}
	virtual TInt AllocSize(TInt&) const  {test(0); return 0;}
	virtual TInt Available(TInt&) const  {test(0); return 0;}
	virtual TInt DebugFunction(TInt, TAny*, TAny*) {test(0); return 0;}
	virtual TInt Extension_(TUint, TAny*&, TAny*) {test(0); return 0;}
	};

void IntegerBenchmark(TInt aCount, TBool aReserve)
	{
	RArray<TInt> array;
	TUint32 before, after, diff;
	TInt x=0;
	TInt i;
	double avg;

	test.Printf(_L("**** INTEGER BENCHMARKS ***\n"));

	if (!aReserve)
		{
		before = User::NTickCount();
		for (i=0; i<aCount; ++i)
			{
			x += 0x58b90bfb;
			TInt r = array.InsertInOrder(x);
			test(r==KErrNone);
			}
		after = User::NTickCount();
		diff = after - before;
		diff *= NanoTickPeriod;
		avg = (double)diff / (double)aCount;
		test.Printf(_L("ARRAY:      %d insertions take %dus (%.2gus each)\n"), aCount, diff, avg);

		x=0;
		before = User::NTickCount();
		for (i=0; i<aCount; ++i)
			{
			x += 0x58b90bfb;
			TInt r = array.FindInOrder(x);
			test(r>=0);
			}
		after = User::NTickCount();
		diff = after - before;
		diff *= NanoTickPeriod;
		avg = (double)diff / (double)aCount;
		test.Printf(_L("ARRAY:      %d successful finds take %dus (%.2gus each)\n"), aCount, diff, avg);

		before = User::NTickCount();
		for (i=0; i<aCount; ++i)
			{
			x += 0x58b90bfb;
			TInt r = array.FindInOrder(x);
			test(r==KErrNotFound);
			}
		after = User::NTickCount();
		diff = after - before;
		diff *= NanoTickPeriod;
		avg = (double)diff / (double)aCount;
		test.Printf(_L("ARRAY:      %d unsuccessful finds take %dus (%.2gus each)\n"), aCount, diff, avg);

		x=0;
		before = User::NTickCount();
		for (i=0; i<aCount; ++i)
			{
			x += 0x58b90bfb;
			TInt r = array.FindInOrder(x);
			test(r>=0);
			array.Remove(r);
			}
		after = User::NTickCount();
		diff = after - before;
		diff *= NanoTickPeriod;
		avg = (double)diff / (double)aCount;
		test.Printf(_L("ARRAY:      %d deletions take %dus (%.2gus each)\n"), aCount, diff, avg);
		array.Close();
		}

	INTSET(set);
	x=0;
	RAllocator* pA = 0;
	RDummyAllocator da;
	if (aReserve)
		{
		test(set.Reserve(aCount)==KErrNone);
		pA = User::SwitchAllocator(&da);			// check that no memory accesses occur
		test(set.Reserve(10)==KErrNone);			// shouldn't need to do anything
		test(set.Reserve(aCount/2)==KErrNone);		// shouldn't need to do anything
		test(set.Reserve(aCount)==KErrNone);		// shouldn't need to do anything
		}
	before = User::NTickCount();
	for (i=0; i<aCount; ++i)
		{
		x += 0x58b90bfb;
		TInt r = set.Insert(x);	// we have no heap here so this tests that Reserve() has preallocated sufficient memory
		test(r==KErrNone);
		}
	after = User::NTickCount();
	diff = after - before;
	diff *= NanoTickPeriod;
	avg = (double)diff / (double)aCount;
	if (aReserve)
		User::SwitchAllocator(pA);
	test.Printf(_L("HASH TABLE: %d insertions take %dus (%.2gus each)\n"), aCount, diff, avg);

	x=0;
	before = User::NTickCount();
	for (i=0; i<aCount; ++i)
		{
		x += 0x58b90bfb;
		const TInt* p = set.Find(x);
		test(p!=0);
		}
	after = User::NTickCount();
	diff = after - before;
	diff *= NanoTickPeriod;
	avg = (double)diff / (double)aCount;
	test.Printf(_L("HASH TABLE: %d successful finds take %dus (%.2gus each)\n"), aCount, diff, avg);

	before = User::NTickCount();
	for (i=0; i<aCount; ++i)
		{
		x += 0x58b90bfb;
		const TInt* p = set.Find(x);
		test(!p);
		}
	after = User::NTickCount();
	diff = after - before;
	diff *= NanoTickPeriod;
	avg = (double)diff / (double)aCount;
	test.Printf(_L("HASH TABLE: %d unsuccessful finds take %dus (%.2gus each)\n"), aCount, diff, avg);

	x=0;
	before = User::NTickCount();
	for (i=0; i<aCount; ++i)
		{
		x += 0x58b90bfb;
		TInt r = set.Remove(x);
		test(r==KErrNone);
		}
	after = User::NTickCount();
	diff = after - before;
	diff *= NanoTickPeriod;
	avg = (double)diff / (double)aCount;
	test.Printf(_L("HASH TABLE: %d deletions take %dus (%.2gus each)\n"), aCount, diff, avg);
	set.Close();
	}

TInt des8order(const TDesC8& aA, const TDesC8& aB)
	{
	return aA.Compare(aB);
	}

void StringBenchmark8(TInt aCount, TBool aReserve)
	{
	RPointerArray<TDesC8> array;
	TUint32 before, after, diff;
	TInt x=0;
	TInt i;
	double avg;
	const TDesC8** base = (const TDesC8**)&DesC8Array[0];
	test(base[1331] == DesC8Array[1331]);

	test.Printf(_L("**** 8 BIT STRING BENCHMARKS ***\n"));

	if (!aReserve)
		{
		before = User::NTickCount();
		for (i=0; i<aCount; ++i)
			{
			x = (x+4339)&0x3fff;
			TInt r = array.InsertInOrder(base[x], &des8order);
			test(r==KErrNone);
			}
		after = User::NTickCount();
		diff = after - before;
		diff *= NanoTickPeriod;
		avg = (double)diff / (double)aCount;
		test.Printf(_L("ARRAY:      %d insertions take %dus (%.2gus each)\n"), aCount, diff, avg);

		x=0;
		before = User::NTickCount();
		for (i=0; i<aCount; ++i)
			{
			x = (x+4339)&0x3fff;
			TInt r = array.FindInOrder(base[x], &des8order);
			test(r>=0);
			}
		after = User::NTickCount();
		diff = after - before;
		diff *= NanoTickPeriod;
		avg = (double)diff / (double)aCount;
		test.Printf(_L("ARRAY:      %d successful finds take %dus (%.2gus each)\n"), aCount, diff, avg);

		before = User::NTickCount();
		for (i=0; i<aCount; ++i)
			{
			x = (x+4339)&0x3fff;
			TInt r = array.FindInOrder(base[x], &des8order);
			test(r==KErrNotFound);
			}
		after = User::NTickCount();
		diff = after - before;
		diff *= NanoTickPeriod;
		avg = (double)diff / (double)aCount;
		test.Printf(_L("ARRAY:      %d unsuccessful finds take %dus (%.2gus each)\n"), aCount, diff, avg);

		x=0;
		before = User::NTickCount();
		for (i=0; i<aCount; ++i)
			{
			x = (x+4339)&0x3fff;
			TInt r = array.FindInOrder(base[x], &des8order);
			test(r>=0);
			array.Remove(r);
			}
		after = User::NTickCount();
		diff = after - before;
		diff *= NanoTickPeriod;
		avg = (double)diff / (double)aCount;
		test.Printf(_L("ARRAY:      %d deletions take %dus (%.2gus each)\n"), aCount, diff, avg);
		array.Close();
		}

	STRSET8(set);
	x=0;
	if (aReserve)
		test(set.Reserve(aCount)==KErrNone);
	before = User::NTickCount();
	for (i=0; i<aCount; ++i)
		{
		x = (x+4339)&0x3fff;
		TInt r = set.Insert(base[x]);
		test(r==KErrNone);
		}
	after = User::NTickCount();
	diff = after - before;
	diff *= NanoTickPeriod;
	avg = (double)diff / (double)aCount;
	test.Printf(_L("HASH TABLE: %d insertions take %dus (%.2gus each)\n"), aCount, diff, avg);

	x=0;
	before = User::NTickCount();
	for (i=0; i<aCount; ++i)
		{
		x = (x+4339)&0x3fff;
		const TDesC8* p = set.Find(*base[x]);
		test(p!=0);
		}
	after = User::NTickCount();
	diff = after - before;
	diff *= NanoTickPeriod;
	avg = (double)diff / (double)aCount;
	test.Printf(_L("HASH TABLE: %d successful finds take %dus (%.2gus each)\n"), aCount, diff, avg);

	before = User::NTickCount();
	for (i=0; i<aCount; ++i)
		{
		x = (x+4339)&0x3fff;
		const TDesC8* p = set.Find(*base[x]);
		test(!p);
		}
	after = User::NTickCount();
	diff = after - before;
	diff *= NanoTickPeriod;
	avg = (double)diff / (double)aCount;
	test.Printf(_L("HASH TABLE: %d unsuccessful finds take %dus (%.2gus each)\n"), aCount, diff, avg);

	x=0;
	before = User::NTickCount();
	for (i=0; i<aCount; ++i)
		{
		x = (x+4339)&0x3fff;
		TInt r = set.Remove(base[x]);
		test(r==KErrNone);
		}
	after = User::NTickCount();
	diff = after - before;
	diff *= NanoTickPeriod;
	avg = (double)diff / (double)aCount;
	test.Printf(_L("HASH TABLE: %d deletions take %dus (%.2gus each)\n"), aCount, diff, avg);
	set.Close();
	}

TInt des16order(const TDesC16& aA, const TDesC16& aB)
	{
	return aA.Compare(aB);
	}

void StringBenchmark16(TInt aCount, TBool aReserve)
	{
	RPointerArray<TDesC16> array;
	TUint32 before, after, diff;
	TInt x=0;
	TInt i;
	double avg;
	const TDesC16** base = (const TDesC16**)&DesC16Array[0];
	test(base[1331] == DesC16Array[1331]);

	test.Printf(_L("**** 16 BIT STRING BENCHMARKS ***\n"));

	if (!aReserve)
		{
		before = User::NTickCount();
		for (i=0; i<aCount; ++i)
			{
			x = (x+4339)&0x3fff;
			TInt r = array.InsertInOrder(base[x], &des16order);
			test(r==KErrNone);
			}
		after = User::NTickCount();
		diff = after - before;
		diff *= NanoTickPeriod;
		avg = (double)diff / (double)aCount;
		test.Printf(_L("ARRAY:      %d insertions take %dus (%.2gus each)\n"), aCount, diff, avg);

		x=0;
		before = User::NTickCount();
		for (i=0; i<aCount; ++i)
			{
			x = (x+4339)&0x3fff;
			TInt r = array.FindInOrder(base[x], &des16order);
			test(r>=0);
			}
		after = User::NTickCount();
		diff = after - before;
		diff *= NanoTickPeriod;
		avg = (double)diff / (double)aCount;
		test.Printf(_L("ARRAY:      %d successful finds take %dus (%.2gus each)\n"), aCount, diff, avg);

		before = User::NTickCount();
		for (i=0; i<aCount; ++i)
			{
			x = (x+4339)&0x3fff;
			TInt r = array.FindInOrder(base[x], &des16order);
			test(r==KErrNotFound);
			}
		after = User::NTickCount();
		diff = after - before;
		diff *= NanoTickPeriod;
		avg = (double)diff / (double)aCount;
		test.Printf(_L("ARRAY:      %d unsuccessful finds take %dus (%.2gus each)\n"), aCount, diff, avg);

		x=0;
		before = User::NTickCount();
		for (i=0; i<aCount; ++i)
			{
			x = (x+4339)&0x3fff;
			TInt r = array.FindInOrder(base[x], &des16order);
			test(r>=0);
			array.Remove(r);
			}
		after = User::NTickCount();
		diff = after - before;
		diff *= NanoTickPeriod;
		avg = (double)diff / (double)aCount;
		test.Printf(_L("ARRAY:      %d deletions take %dus (%.2gus each)\n"), aCount, diff, avg);
		array.Close();
		}

	STRSET16(set);
	x=0;
	if (aReserve)
		test(set.Reserve(aCount)==KErrNone);
	before = User::NTickCount();
	for (i=0; i<aCount; ++i)
		{
		x = (x+4339)&0x3fff;
		TInt r = set.Insert(base[x]);
		test(r==KErrNone);
		}
	after = User::NTickCount();
	diff = after - before;
	diff *= NanoTickPeriod;
	avg = (double)diff / (double)aCount;
	test.Printf(_L("HASH TABLE: %d insertions take %dus (%.2gus each)\n"), aCount, diff, avg);

	x=0;
	before = User::NTickCount();
	for (i=0; i<aCount; ++i)
		{
		x = (x+4339)&0x3fff;
		const TDesC16* p = set.Find(*base[x]);
		test(p!=0);
		}
	after = User::NTickCount();
	diff = after - before;
	diff *= NanoTickPeriod;
	avg = (double)diff / (double)aCount;
	test.Printf(_L("HASH TABLE: %d successful finds take %dus (%.2gus each)\n"), aCount, diff, avg);

	before = User::NTickCount();
	for (i=0; i<aCount; ++i)
		{
		x = (x+4339)&0x3fff;
		const TDesC16* p = set.Find(*base[x]);
		test(!p);
		}
	after = User::NTickCount();
	diff = after - before;
	diff *= NanoTickPeriod;
	avg = (double)diff / (double)aCount;
	test.Printf(_L("HASH TABLE: %d unsuccessful finds take %dus (%.2gus each)\n"), aCount, diff, avg);

	x=0;
	before = User::NTickCount();
	for (i=0; i<aCount; ++i)
		{
		x = (x+4339)&0x3fff;
		TInt r = set.Remove(base[x]);
		test(r==KErrNone);
		}
	after = User::NTickCount();
	diff = after - before;
	diff *= NanoTickPeriod;
	avg = (double)diff / (double)aCount;
	test.Printf(_L("HASH TABLE: %d deletions take %dus (%.2gus each)\n"), aCount, diff, avg);
	set.Close();
	}

void Benchmark()
	{
	test.Next(_L("Benchmarks ..."));

	IntegerBenchmark(1000, EFalse);
	IntegerBenchmark(1000, ETrue);
	IntegerBenchmark(2000, EFalse);
	IntegerBenchmark(2000, ETrue);
	IntegerBenchmark(4000, EFalse);
	IntegerBenchmark(4000, ETrue);
	IntegerBenchmark(8000, EFalse);
	IntegerBenchmark(8000, ETrue);
	IntegerBenchmark(16000, EFalse);
	IntegerBenchmark(16000, ETrue);
	IntegerBenchmark(32000, EFalse);
	IntegerBenchmark(32000, ETrue);

	PopulateArray8(16384);
	StringBenchmark8(1000, EFalse);
	StringBenchmark8(2000, EFalse);
	StringBenchmark8(4000, EFalse);
	StringBenchmark8(8000, EFalse);
	DesC8Array.ResetAndDestroy();

	PopulateArray16(16384);
	StringBenchmark16(1000, EFalse);
	StringBenchmark16(2000, EFalse);
	StringBenchmark16(4000, EFalse);
	StringBenchmark16(8000, EFalse);
	DesC16Array.ResetAndDestroy();
	}


TUint32 mp(TUint32 x)
	{
	TUint32 x3 = x+(x<<1);
	TUint32 x7 = x+(x3<<1);
	TUint32 x15 = x+(x7<<1);
	TUint32 y = x + (x7<<3) + (x3<<7) + (x15<<11) + (x7<<16) + (x3<<20) + (x15<<25) + (x<<31);
	return y;
	}

TUint32 strh(const char* aIn)
	{
	TUint32 h = 0;
	TInt i = 0;
	while (*aIn)
		{
		if (i==0)
			h = mp(h);
		TUint32 c = *aIn++;
		c <<= (8*i);
		h ^= c;
		i = (i+1)&3;
		}
	return mp(h);
	}

TUint32 strh(const wch* aIn)
	{
	TUint32 h = 0;
	TInt i = 0;
	while (*aIn)
		{
		if (i==0)
			h = mp(h);
		TUint32 c = *aIn++;
		switch (i)
			{
			case 0:		break;
			case 1:		c<<=16; break;
			case 2:		c<<=8; break;
			default:	c=(c<<24)|(c>>8); break;
			};
		h ^= c;
		i = (i+1)&3;
		}
	return mp(h);
	}

void TestHash(TInt* aIn, TUint32 aExpected)
	{
	THashFunction32<TInt*> hf(&DefaultHash::IntegerPtr);
	TUint32 out = hf.Hash(aIn);
	test(aExpected == mp((TUint32)aIn));
	if (out != aExpected)
		{
		test.Printf(_L("Hashing %08x Expected %08x Got %08x\n"), aIn, aExpected, out);
		test(0);
		}
	}

void TestHash(TDesC8* aIn, TUint32 aExpected)
	{
	THashFunction32<TDesC8*> hf(&DefaultHash::Des8Ptr);
	TUint32 out = hf.Hash(aIn);
	test(aExpected == mp((TUint32)aIn));
	if (out != aExpected)
		{
		test.Printf(_L("Hashing %08x Expected %08x Got %08x\n"), aIn, aExpected, out);
		test(0);
		}
	}

void TestHash(TDesC16* aIn, TUint32 aExpected)
	{
	THashFunction32<TDesC16*> hf(&DefaultHash::Des16Ptr);
	TUint32 out = hf.Hash(aIn);
	test(aExpected == mp((TUint32)aIn));
	if (out != aExpected)
		{
		test.Printf(_L("Hashing %08x Expected %08x Got %08x\n"), aIn, aExpected, out);
		test(0);
		}
	}


void TestHash(TInt aIn, TUint32 aExpected)
	{
	THashFunction32<TInt> hf(&DefaultHash::Integer);
	TUint32 out = hf.Hash(aIn);
	test(aExpected == mp((TUint32)aIn));
	if (out != aExpected)
		{
		test.Printf(_L("Hashing %08x Expected %08x Got %08x\n"), aIn, aExpected, out);
		test(0);
		}
	}

void TestHash(const char* aIn, TUint32 aExpected)
	{
	THashFunction32<TDesC8> hf(&DefaultHash::Des8);
	TPtrC8 p((const TUint8*)aIn);
	TUint32 out = hf.Hash(p);
	test(aExpected == strh(aIn));
	if (out != aExpected)
		{
		TBuf<256> buf;
		buf.Copy(p);
		test.Printf(_L("Hashing %S (len %d) Expected %08x Got %08x\n"), &buf, p.Length(), aExpected, out);
		test(0);
		}
	}

void TestHash(const wch* aIn)
	{
	THashFunction32<TDesC16> hf(&DefaultHash::Des16);
	TPtrC16 p((const TUint16*)aIn);
	TUint32 out = hf.Hash(p);
	TUint32 exp = strh(aIn);
	if (out != exp)
		{
		test.Printf(_L("Hashing %S (len %d) Expected %08x Got %08x\n"), &p, p.Size(), exp, out);
		test(0);
		}
	}

void TestHash()
	{
	test.Next(_L("Test integer hash"));
	TestHash(1,0x9e3779b9);
	TestHash(2,0x3c6ef372);
	TestHash(4,0x78dde6e4);
	TestHash(8,0xf1bbcdc8);
	TestHash(16,0xe3779b90);
	TestHash(0xc90fdaa2,0xbf999112);
	TestHash(0xb504f334,0x7fb35494);
	TestHash(0xddb3d743,0xd11a3a6b);
	TestHash(0xadf85458,0x873a8b98);
	TestHash(0x11730859,0xb4321951);
	TestHash(0x64636261,0x8628f119);
	}

void TestIntegerPtrHash()
	{
	TInt i[5];
	TInt* ptr;
	test.Next(_L("Test Integer pointer hash"));
	for (ptr=i; ptr<i+5; ptr++)
		{
		TestHash(ptr, DefaultHash::IntegerPtr(ptr));
		}
	}

void TestDes8PtrHash()
	{
	TBuf8<8> i[5];
	TDesC8* ptr;
	test.Next(_L("Test Des8 pointer hash"));
	for (ptr=i; ptr<i+5; ptr++)
		{
		TestHash(ptr, DefaultHash::Des8Ptr(ptr));
		}
	}

void TestDes16PtrHash()
	{
	TBuf16<8> i[5];
	TDesC16* ptr;
	test.Next(_L("Test Des16 pointer hash"));
	for (ptr=i; ptr<i+5; ptr++)
		{
		TestHash(ptr, DefaultHash::Des16Ptr(ptr));
		}
	}

const char teststr[] = "zyxwvutsrq";
void TestStringHash()
	{
	test.Next(_L("Test 8 bit string hash"));
	TestHash("",0x0);
	TestHash("a",0xf3051f19);
	TestHash("b",0x913c98d2);
	TestHash("ab",0x2f9df119);
	TestHash("ba",0x965bb1d2);
	TestHash("abc",0x4228f119);
	TestHash("abcd",0x8628f119);
	TestHash("abcde",0xb75e1e9c);
	TestHash("abcdef",0x3693149c);
	TestHash("abcdefg",0xc1c2149c);
	TestHash("abcdefgh",0xe9c2149c);
	TestHash("abcdefghi",0x5fcbf20d);
	TestHash("abcdefghj",0xfe036bc6);

	TestHash(teststr, 0x108ca51e);
	TestHash(teststr+1, 0x551002ad);
	TestHash(teststr+2, 0x37dc0d6c);
	TestHash(teststr+3, 0x2937f92c);
	TestHash(teststr+4, 0xf0818a94);
	TestHash(teststr+5, 0xb1b25f1c);
	TestHash(teststr+6, 0x7a3342d4);
	TestHash(teststr+7, 0x81c9101b);
	TestHash(teststr+8, 0xf16edd62);
	TestHash(teststr+9, 0xd67cbaa9);
	TestHash(teststr+10, 0);

	char t[16];
	int i,j;
	for (i=0; i<=4; ++i)
		{
		for (j=0; j<=10; ++j)
			{
			const char* s = teststr + j;
			int l = User::StringLength((const TUint8*)s);
			memset(t, 0xbb, 16);
			memcpy(t+i, s, l+1);
			TUint32 h;
			switch (j)
				{
				case 0: h = 0x108ca51e; break;
				case 1: h = 0x551002ad; break;
				case 2: h = 0x37dc0d6c; break;
				case 3: h = 0x2937f92c; break;
				case 4: h = 0xf0818a94; break;
				case 5: h = 0xb1b25f1c; break;
				case 6: h = 0x7a3342d4; break;
				case 7: h = 0x81c9101b; break;
				case 8: h = 0xf16edd62; break;
				case 9: h = 0xd67cbaa9; break;
				default: h = 0; break;
				};
			TestHash(t+i, h);
			}
		}
	}

const wch wteststr[] = L"zyxwvutsrq";
void TestWStringHash()
	{
	test.Next(_L("Test 16 bit string hash"));
	TestHash(L"");
	TestHash(L"a");
	TestHash(L"b");
	TestHash(L"ab");
	TestHash(L"ba");
	TestHash(L"abc");
	TestHash(L"abcd");
	TestHash(L"abcde");
	TestHash(L"abcdef");
	TestHash(L"abcdefg");
	TestHash(L"abcdefgh");
	TestHash(L"abcdefghi");
	TestHash(L"abcdefghj");

	TestHash(wteststr);
	TestHash(wteststr+1);
	TestHash(wteststr+2);
	TestHash(wteststr+3);
	TestHash(wteststr+4);
	TestHash(wteststr+5);
	TestHash(wteststr+6);
	TestHash(wteststr+7);
	TestHash(wteststr+8);
	TestHash(wteststr+9);
	TestHash(wteststr+10);

	wch t[16];
	int i,j;
	for (i=0; i<=4; ++i)
		{
		for (j=0; j<=10; ++j)
			{
			const wch* s = wteststr + j;
			int l = User::StringLength((const TUint16*)s);
			memset(t, 0xbb, 2*16);
			memcpy(t+i, s, 2*(l+1));
			TestHash(t+i);
			}
		}
	}
template <class K,class V> 
void TestHashMapPtr(RHashMap<K,V> &map, K ptr, V* i)
{
	test(map.Reserve(5) == KErrNone);
	for (ptr=i;ptr<i+5;ptr++)
		{
		test(map.Insert(ptr,*ptr) == KErrNone);
		}
	for (ptr=i+4;ptr>=i;ptr--)
		{
		test(*(map.Find(ptr)) == *ptr);
		}
	test(map.Count() == 5);
	test(map.Remove(i) == KErrNone);
	test(map.Count()==4);
	test(map.Find(i)==NULL);
	map.Close();
}

void TestPtrHashMaps()
	{	

	test.Next(_L("Test RHashMap of default pointer types"));
	TInt i[5];
	TInt *ptr=i;
	RHashMap<TInt*,TInt> mp;
	TestHashMapPtr(mp, ptr, i);

	TInt32 i1[5];
	TInt32 *ptr1=i1;
	RHashMap<TInt32*,TInt32> mp1;
	TestHashMapPtr(mp1,ptr1,i1);
	
	TUint i2[5];
	TUint *ptr2=i2;
	RHashMap<TUint*,TUint> mp2;
	TestHashMapPtr(mp2,ptr2,i2);

	TUint32 i3[5];
	TUint32 *ptr3=i3;
	RHashMap<TUint32*,TUint32> mp3;
	TestHashMapPtr(mp3,ptr3,i3);

	TBuf8<5> i4[5];
	TBuf8<5> *ptr4=i4;
	RHashMap<TDesC8*,TDesC8> mp4;
	for (ptr4=i4; ptr4 < i4+5; ptr4++) 
		{
		test(mp4.Insert(ptr4,*ptr4) == KErrNone);
		}
	for (ptr4=i4+4; ptr4 >= i4; ptr4--) 
		{
		test(*(mp4.Find(ptr4)) == *ptr4);
		}
	test(mp4.Count()==5);
	test(mp4.Remove(i4) == KErrNone);
	test(mp4.Find(i4) == NULL);
	test(mp4.Count()==4);
	mp4.Close();


	TBuf16<5> i5[5];
	TBuf16<5> *ptr5=i5;
	RHashMap<TDesC16*,TDesC16> mp5;
	for (ptr5=i5; ptr5 < i5+5; ptr5++) 
		{
		test(mp5.Insert(ptr5,*ptr5) == KErrNone);
		}
	for (ptr5=i5+4; ptr5 >= i5; ptr5--) 
		{
		test(*(mp5.Find(ptr5)) == *ptr5);
		}
	test(mp5.Count()==5);
	test(mp5.Remove(i5) == KErrNone);
	test(mp5.Find(i5) == NULL);
	test(mp5.Count()==4);
	mp5.Close();
	
}

/** Tests that Reserve() will always allocate memory for new tables 
	even for small reserve sizes
	See DEF087906.
*/
void TestSmallReserve()
	{
	test.Next(_L("Test RHashTableBase::Reserve preallocates memory, even for small no of elements"));
	RAllocator* pA = 0;
	RDummyAllocator da;
	
	// Reserve should allocate the memory required for the table of 1 element
	INTSET(set);
	RHashMap<TInt,TInt> hashMap;
	
	test(set.Reserve(1) == KErrNone);
	test(hashMap.Reserve(1) == KErrNone);
	
	pA = User::SwitchAllocator(&da);
	
	// No more memory should be allocated for the table as it should
	// have been already allocated by Reserve()
	test(set.Insert(123) == KErrNone);
	test(hashMap.Insert(123,456) == KErrNone);
	
	// Switch back to allow set to be closed
	User::SwitchAllocator(pA);
	set.Close();
	hashMap.Close();
	}
	

TInt E32Main()
	{
	test.Title();

	test(HAL::Get(HAL::ENanoTickPeriod, NanoTickPeriod)==KErrNone);
	test.Printf(_L("NanoTickPeriod %dus\n"), NanoTickPeriod);

	__UHEAP_MARK;

	test.Start(_L("Testing hash tables"));

	TestHash();
	TestStringHash();
	TestWStringHash();
	TestIntegerPtrHash();
	TestDes8PtrHash();
	TestDes16PtrHash();

	TestHashSet();
	TestHashIter();
	TestHashMap();
	TestPtrHashMaps();

	PopulateArray8(4096);
	PopulateArray16(4096);
	TestPtrHashSet();
	TestPtrHashMap();
	DesC16Array.ResetAndDestroy();
	DesC8Array.ResetAndDestroy();


	TestOOM();
	Benchmark();
	TestSmallReserve();
	
	// Get cleanup stack
	CTrapCleanup* cleanup = CTrapCleanup::New();
	TInt result = KErrNoMemory;
	if (cleanup)
	    {
	    TRAP(result, TestInsertL());
	    // Destroy the cleanup stack
	    delete cleanup;
	    }
		
	test.End();

	__UHEAP_MARKEND;
	return 0;
	}
