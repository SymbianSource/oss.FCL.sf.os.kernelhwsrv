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
// e32test\buffer\t_tbma.cpp
// 
//

#define __E32TEST_EXTENSION__
#include "t_tbma.h"
#include <cpudefs.h>
#include <e32atomics.h>

RTest test(_L("T_TBMA"));


/**************************************
 * class TBmaList
 **************************************/

TBmaList* TBmaList::New(TInt aNumBmas)
	{
	TBmaList* pL=new TBmaList;
	if (pL)
		{
		pL->iNumBmas=aNumBmas;
		pL->iBmaList=(TBitMapAllocator**)User::Alloc(aNumBmas*sizeof(TBitMapAllocator*));
		if (pL->iBmaList)
			Mem::FillZ(pL->iBmaList, aNumBmas*sizeof(TBitMapAllocator*));
		pL->iBaseList=(TInt*)User::Alloc((aNumBmas+1)*sizeof(TInt));
		if (!pL->iBmaList || !pL->iBaseList)
			{
			delete pL;
			pL=NULL;
			}
		}
	return pL;
	}

TBmaList* TBmaList::New(const TBitMapAllocator& aBma, TInt aNumSplits, VA_LIST aList)
	{
	TBmaList* pL=TBmaList::New(aNumSplits+1);
	if (!pL)
		return NULL;
	TInt i;
	pL->iBaseList[0]=0;
	for (i=1; i<=aNumSplits; ++i)
		pL->iBaseList[i]=VA_ARG(aList,TInt);
	pL->iBaseList[aNumSplits+1]=aBma.iSize;
	for (i=0; i<=aNumSplits; ++i)
		{
		TInt sz=pL->iBaseList[i+1]-pL->iBaseList[i];
		__ASSERT_ALWAYS(sz>0, TBMA_FAULT());
		pL->iBmaList[i]=TBitMapAllocator::New(sz,EFalse);
		if (!pL->iBmaList[i])
			{
			delete pL;
			return NULL;
			}
		TInt j;
		for (j=0; j<sz; ++j)
			{
			if (aBma.NotAllocated(j+pL->iBaseList[i],1))
				pL->iBmaList[i]->Free(j);
			}
		}
	return pL;
	}

TBmaList::TBmaList()
	{
	iNumBmas=0;
	iBmaList=NULL;
	iBaseList=NULL;
	}

TBmaList::~TBmaList()
	{
	TInt i;
	for (i=0; i<iNumBmas; ++i)
		delete iBmaList[i];
	User::Free(iBmaList);
	User::Free(iBaseList);
	}
/*
 * Extended first fit allocator
 */
TInt TBmaList::AllocConsecutiveFF(TInt aLength)
	{
	TInt base=KErrNotFound;
	TInt bmalen=0;
	TInt carry=0;
	TBitMapAllocator** ppA=iBmaList;		// pointer to list of TBitMapAllocator*
	TBitMapAllocator** pE=ppA+iNumBmas;
	TInt* pB=iBaseList;
	for (; ppA<pE; ++ppA, ++pB)
		{
		TBitMapAllocator* pA=*ppA;
		if (*pB!=base+bmalen)
			{
			// this BMA is not contiguous with previous one
			carry=0;
			}
		base=*pB;
		bmalen=pA->iSize;
		TInt l;
		TInt r=pA->AllocAligned(aLength,0,0,EFalse,carry,l);
		if (r>=0)
			return base+r-carry;
		}
	return KErrNotFound;
	}

/*
 * Extended best fit allocator
 */
TInt TBmaList::AllocConsecutiveBF(TInt aLength)
	{
	TInt bmalen=0;
	TInt carry=0;
	TInt minrun=KMaxTInt;
	TInt minrunpos=KErrNotFound;
	TBitMapAllocator** ppA=iBmaList;		// pointer to list of TBitMapAllocator*
	TBitMapAllocator** pE=ppA+iNumBmas;
	TInt* pB=iBaseList;
	TInt base=*pB;
	for (; ppA<pE; ++ppA, ++pB)
		{
		TBitMapAllocator* pA=*ppA;
		if (*pB!=base+bmalen)
			{
			// this BMA is not contiguous with previous one
			// check final run of previous BMA
			if (carry<minrun)
				{
				minrun=carry;
				minrunpos=base+bmalen-carry;
				}
			carry=0;
			}
		base=*pB;
		bmalen=pA->iSize;
		TInt l=KMaxTInt;
		TInt oldc=carry;
		TInt r=pA->AllocAligned(aLength,0,0,ETrue,carry,l);
		if (r>=0)
			{
			// check shortest run in this BMA
			if (l<minrun)
				{
				minrun=l;
				minrunpos=r ? (base+r) : (base-oldc);
				if (minrun==aLength)
					break;				// exact match so finish
				}
			}
		}
	// check final run of last BMA
	if (ppA==pE && carry>=aLength && carry<minrun)
		minrunpos=base+bmalen-carry;
	return minrunpos;
	}

/*
 * Extended first fit aligned allocator
 */
TInt TBmaList::AllocAlignedFF(TInt aLength, TInt aAlign)
	{
	TUint32 alignsize=1<<aAlign;
	TUint32 alignmask=alignsize-1;
	TInt base=KErrNotFound;
	TInt bmalen=0;
	TInt carry=0;
	TBitMapAllocator** ppA=iBmaList;		// pointer to list of TBitMapAllocator*
	TBitMapAllocator** pE=ppA+iNumBmas;
	TInt* pB=iBaseList;
	for (; ppA<pE; ++ppA, ++pB)
		{
		TBitMapAllocator* pA=*ppA;
		if (*pB!=base+bmalen)
			{
			// this BMA is not contiguous with previous one
			carry=0;
			}
		base=*pB;
		bmalen=pA->iSize;
		TInt l;
		TInt r=pA->AllocAligned(aLength,aAlign,base,EFalse,carry,l);
		if (r>=0)
			return (base+r-carry+alignmask)&~alignmask;
		}
	return KErrNotFound;
	}

/*
 * Extended best fit aligned allocator
 */
TInt TBmaList::AllocAlignedBF(TInt aLength, TInt aAlign)
	{
	TInt bmalen=0;
	TInt carry=0;
	TInt minrun=KMaxTInt;
	TInt minrunpos=KErrNotFound;
	TUint32 alignsize=1<<aAlign;
	TUint32 alignmask=alignsize-1;
	TBitMapAllocator** ppA=iBmaList;		// pointer to list of TBitMapAllocator*
	TBitMapAllocator** pE=ppA+iNumBmas;
	TInt* pB=iBaseList;
	TInt base=*pB;
	for (; ppA<pE; ++ppA, ++pB)
		{
		TBitMapAllocator* pA=*ppA;
		if (*pB!=base+bmalen)
			{
			// this BMA is not contiguous with previous one
			// check final run of previous BMA
			if (carry<minrun)
				{
				TInt fpos=base+bmalen-carry;
				TInt lost=((fpos+base+alignmask)&~alignmask)-base-fpos;
				if (carry-lost>=aLength)
					{
					minrun=carry;
					minrunpos=fpos;
					}
				}
			carry=0;
			}
		base=*pB;
		bmalen=pA->iSize;
		TInt l=KMaxTInt;
		TInt oldc=carry;
		TInt r=pA->AllocAligned(aLength,aAlign,base,ETrue,carry,l);
		if (r>=0)
			{
			// check shortest run in this BMA
			if (l<minrun)
				{
				minrun=l;
				minrunpos=r ? (base+r) : (base-oldc);
				if (minrun==aLength)
					break;				// exact match so finish
				}
			}
		}
	// check final run of last BMA
	if (ppA==pE && carry<minrun)
		{
		TInt fpos=base+bmalen-carry;
		TInt lost=((fpos+alignmask)&~alignmask)-fpos;
		if (carry-lost>=aLength)
			{
			minrun=carry;
			minrunpos=fpos;
			}
		}
	return (minrunpos<0) ? minrunpos : ((minrunpos+alignmask)&~alignmask);
	}








void Display(TBitMapAllocator* aBma)
	{
	test.Printf(_L("Free %d FirstCheck %08x Size %d Map %08x\n"),aBma->iAvail,aBma->iCheckFirst,aBma->iSize,aBma->iMap);
	TInt i;
	TInt l=0;
	for (i=0; i<((aBma->iSize+31)>>5); i++)
		{
		if (++l==10)
			{
			l=0;
//			test.Getch();
			}
		TUint32 x=aBma->iMap[i];
		TBuf<80> buf;
		buf.NumFixedWidth(x,EBinary,32);
		buf.Append(_L("\n"));
		test.Printf(buf);
		}
	test.Getch();
	}

void Check(TBitMapAllocator& a)
	{
	TInt l=a.iSize;
	l=(l+31)>>5;
	TInt n=0;
	TInt i;
	TUint32* first=NULL;
	for (i=0; i<l; ++i)
		{
		TUint32 w=a.iMap[i];
		if (w && !first)
			first=a.iMap+i;
		n+=__e32_bit_count_32(w);
		}
	test(a.Avail()==n);
	test(first?(a.iCheckFirst<=first):(a.iCheckFirst>=a.iMap && a.iCheckFirst<=a.iMap+l));
	}

void TestConstruct(TInt aSize)
	{
	test.Printf(_L("TestConstruct %d\n"),aSize);
	TBitMapAllocator* pA;
	pA=TBitMapAllocator::New(aSize, EFalse);
	test(pA!=NULL);
	test(pA->Avail()==0);
	Check(*pA);
	delete pA;
	pA=TBitMapAllocator::New(aSize, ETrue);
	test(pA!=NULL);
	test(pA->Avail()==aSize);
	Check(*pA);
	delete pA;
	}

void TestAlloc1(TInt aSize)
	{
	test.Printf(_L("TestAlloc1 %d\n"),aSize);
	TBitMapAllocator* pA=TBitMapAllocator::New(aSize, ETrue);
	test(pA!=NULL);
	test(pA->Avail()==aSize);
	Check(*pA);
	TInt i;
	for (i=0; i<aSize; ++i)
		{
		TInt r=pA->Alloc();
		test(r==i);
		test(pA->Avail()==aSize-i-1);
		test(pA->iCheckFirst==pA->iMap+i/32);
		Check(*pA);
		}
	test(pA->Alloc()<0);
	delete pA;
	}

void TestFree1(TInt aSize)
	{
	test.Printf(_L("TestFree1 %d\n"),aSize);
	TBitMapAllocator* pA=TBitMapAllocator::New(aSize, EFalse);
	test(pA!=NULL);
	test(pA->Avail()==0);
	TInt i;
	for (i=aSize-1; i>=0; --i)
		{
		pA->Free(i);
		test(pA->Avail()==aSize-i);
		test(pA->Alloc()==i);
		pA->Free(i);
		test(pA->iCheckFirst==pA->iMap+i/32);
		Check(*pA);
		}
	delete pA;
	}

void TestBlockAlloc(TInt aSize)
	{
	test.Printf(_L("TestBlockAlloc %d\n"),aSize);
	const TInt begin[]={0,1,2,7,16,29,31,32,33,63,64,65,83,128};
	TBitMapAllocator* pA=TBitMapAllocator::New(aSize, ETrue);
	test(pA!=NULL);
	test(pA->Avail()==aSize);
	pA->Alloc(0,aSize);
	test(pA->Avail()==0);
	Check(*pA);
	pA->Free(0,aSize);
	test(pA->Avail()==aSize);
	Check(*pA);
	TInt i;
	for (i=0; i<(TInt)(sizeof(begin)/sizeof(TInt)); ++i)
		{
		TInt j=begin[i];
		if (j>aSize)
			break;
		TInt l;
		for (l=1; l<=aSize-j; ++l)
			{
//			test.Printf(_L("j=%d, l=%d, s=%d\n"),j,l,aSize);
			pA->Alloc(j,l);
			test(pA->Avail()==aSize-l);
			test(!pA->NotAllocated(j,l));
			if (j+l<aSize)
				test(pA->NotAllocated(j,l+1));
			if (j>0)
				test(pA->NotAllocated(j-1,l));
			TInt r=pA->Alloc();
			if (j==0)
				{
				if (l<aSize)
					test(r==l);
				else
					test(r<0);
				}
			else
				test(r==0);
			if (r==0)
				{
				pA->Free(r);
				pA->Free(j,l);
				}
			else if (r>0)
				pA->Free(j,l+1);
			else
				pA->Free(j,l);
			test(pA->Avail()==aSize);
			Check(*pA);
			}
		}
	delete pA;
	}

void TestBlockFree(TInt aSize)
	{
	test.Printf(_L("TestBlockFree %d\n"),aSize);
	const TInt begin[]={0,1,2,7,16,29,31,32,33,63,64,65,83,128};
	TBitMapAllocator* pA=TBitMapAllocator::New(aSize, EFalse);
	test(pA!=NULL);
	test(pA->Avail()==0);
	TInt i;
	for (i=0; i<(TInt)(sizeof(begin)/sizeof(TInt)); ++i)
		{
		TInt j=begin[i];
		if (j>aSize)
			break;
		TInt l;
		for (l=1; l<=aSize-j; ++l)
			{
//			test.Printf(_L("j=%d, l=%d, s=%d\n"),j,l,aSize);
			pA->Free(j,l);
			test(pA->Avail()==l);
			test(!pA->NotFree(j,l));
			if (j+l<aSize)
				test(pA->NotFree(j,l+1));
			if (j>0)
				test(pA->NotFree(j-1,l));
			TInt r=pA->Alloc();
			test(r==j);
			if (l>1)
				pA->Alloc(j+1,l-1);
			test(pA->Avail()==0);
			Check(*pA);
			}
		}
	delete pA;
	}

void TestNotFree(TInt aSize)
	{
	test.Printf(_L("TestNotFree %d\n"),aSize);
	TBitMapAllocator* pA=TBitMapAllocator::New(aSize, ETrue);
	test(pA!=NULL);
	test(pA->Avail()==aSize);
	test(!pA->NotFree(0,aSize));
	TInt i;
	for (i=0; i<aSize; ++i)
		{
		pA->Alloc(i,1);
		test(pA->NotFree(0,aSize));
		TInt j;
		for (j=1; j*j<=i || j*j+i<=aSize; ++j)
			{
			TInt a=Max(i-j*j,0);
			TInt b=Min(i+j*j,aSize);
			test(pA->NotFree(a,b-a));
			}
		pA->Free(i);
		test(!pA->NotFree(0,aSize));
		}
	const TInt begin[]={0,1,2,7,16,29,31,32,33,63,64,65,83,128};
	const TInt size[]={2,3,7,16,23,31,32,33,63,64,65,89,128};
	const TInt* pB=begin;
	const TInt* pBE=pB+sizeof(begin)/sizeof(TInt);
	const TInt* pS=size;
	const TInt* pSE=pS+sizeof(size)/sizeof(TInt);
	for (; pB<pBE; ++pB)
		{
		TInt b=*pB;
		if (b>=aSize)
			continue;
		for (; pS<pSE; ++pS)
			{
			TInt l=*pS;
			if (b+l>aSize)
				continue;
			pA->Alloc(b,l);
			TInt j;
			for (j=1; j<aSize; ++j)
				{
				if (j<=b)
					test(!pA->NotFree(0,j));
				else
					test(pA->NotFree(0,j));
				if (aSize-j>=b+l)
					test(!pA->NotFree(aSize-j,j));
				else
					test(pA->NotFree(aSize-j,j));
				}
			pA->Free(b,l);
			}
		}
	delete pA;
	}

void TestNotAllocated(TInt aSize)
	{
	test.Printf(_L("TestNotAllocated %d\n"),aSize);
	TBitMapAllocator* pA=TBitMapAllocator::New(aSize, EFalse);
	test(pA!=NULL);
	test(pA->Avail()==0);
	test(!pA->NotAllocated(0,aSize));
	TInt i;
	for (i=0; i<aSize; ++i)
		{
		pA->Free(i);
		test(pA->NotAllocated(0,aSize));
		TInt j;
		for (j=1; j*j<=i || j*j+i<=aSize; ++j)
			{
			TInt a=Max(i-j*j,0);
			TInt b=Min(i+j*j,aSize);
			test(pA->NotAllocated(a,b-a));
			}
		pA->Alloc(i,1);
		test(!pA->NotAllocated(0,aSize));
		}
	const TInt begin[]={0,1,2,7,16,29,31,32,33,63,64,65,83,128};
	const TInt size[]={2,3,7,16,23,31,32,33,63,64,65,89,128};
	const TInt* pB=begin;
	const TInt* pBE=pB+sizeof(begin)/sizeof(TInt);
	const TInt* pS=size;
	const TInt* pSE=pS+sizeof(size)/sizeof(TInt);
	for (; pB<pBE; ++pB)
		{
		TInt b=*pB;
		if (b>=aSize)
			continue;
		for (; pS<pSE; ++pS)
			{
			TInt l=*pS;
			if (b+l>aSize)
				continue;
			pA->Free(b,l);
			TInt j;
			for (j=1; j<aSize; ++j)
				{
				if (j<=b)
					test(!pA->NotAllocated(0,j));
				else
					test(pA->NotAllocated(0,j));
				if (aSize-j>=b+l)
					test(!pA->NotAllocated(aSize-j,j));
				else
					test(pA->NotAllocated(aSize-j,j));
				}
			pA->Alloc(b,l);
			}
		}
	delete pA;
	}

void TestAllocList(TInt aSize)
	{
	test.Printf(_L("TestAllocList %d\n"),aSize);
	TBitMapAllocator* pA=TBitMapAllocator::New(aSize, EFalse);
	test(pA!=NULL);
	test(pA->Avail()==0);
	TInt i;
	TInt list[256];
	for (i=0; i<aSize; ++i)
		{
		pA->Free(i);
		test(pA->AllocList(128,list)==1);
		test(list[0]==i);
		test(pA->Avail()==0);
		}
	TInt j;
	for (i=0; i<aSize-1; ++i)
		{
		for (j=i+1; j<aSize; ++j)
			{
			pA->Free(i);
			pA->Free(j);
			test(pA->AllocList(1,list)==1);
			test(list[0]==i);
			test(pA->Avail()==1);
			pA->Free(i);
			test(pA->AllocList(128,list)==2);
			test(list[0]==i && list[1]==j);
			test(pA->Avail()==0);
			}
		}
	TInt l;
	for (l=1; l<80; ++l)
		{
		if (2*l+1>aSize)
			break;
		for (i=l+1; i<=aSize-l; ++i)
			{
			pA->Free(0,l);
			pA->Free(i,l);
			test(pA->Avail()==2*l);
			TInt l2;
			for (l2=Max(l-1,1); l2<=2*l; ++l2)
				{
				TInt r=pA->AllocList(l2,list);
//				test.Printf(_L("l2=%d r=%d\n"),l2,r);
				test(r==l2);
				for (j=0; j<Min(l2,l); j++)
					test(list[j]==j);
				for (j=l; j<l2; j++)
					test(list[j]==j-l+i);
				for (j=0; j<l2; ++j)
					pA->Free(list[j]);
				pA->SelectiveFree(0,l);
				pA->SelectiveFree(i,l);
				test(pA->Avail()==2*l);
				}
			pA->Alloc(0,l);
			pA->Alloc(i,l);
			Check(*pA);
			}
		}
	delete pA;
	}

void TestSelectiveFree(TInt aSize)
	{
	test.Printf(_L("TestSelectiveFree %d\n"),aSize);
	TBitMapAllocator* pA=TBitMapAllocator::New(aSize, ETrue);
	test(pA!=NULL);
	test(pA->Avail()==aSize);
	TInt i;
	TInt j;
	TInt l;
	for (i=2; i<8; ++i)
		{
		for (l=1; l<=aSize; ++l)
			{
			new (pA) TBitMapAllocator(aSize, ETrue);
			for (j=0; j<aSize; j+=i)
				pA->Alloc(j,1);
			TInt orig=pA->Avail();
			test(orig==aSize-(aSize+i-1)/i);
			pA->SelectiveFree(0,l);
			TInt freed=pA->Avail()-orig;
			test(freed==(l+i-1)/i);
			Check(*pA);
			}
		}
	for (i=0; i<=Min(32,aSize-1); ++i)
		{
		for (l=1; l<=aSize-i; ++l)
			{
			for (j=1; j<=aSize; ++j)
				{
				new (pA) TBitMapAllocator(aSize, ETrue);
				pA->Alloc(i,l);
				test(pA->Avail()==aSize-l);
				pA->SelectiveFree(0,j);
				test(pA->Avail()==aSize-l+Max(0,Min(i+l,j)-i));
				test(!pA->NotFree(0,j));
				if (j>=i && j<i+l)
					test(pA->NotFree(0,j+1));
				Check(*pA);
				}
			}
		}
	delete pA;
	}


void TestSelectiveAlloc(TInt aSize)
	{
	test.Printf(_L("TestSelectiveAlloc %d\n"),aSize);
	TBitMapAllocator* pA=TBitMapAllocator::New(aSize, ETrue);
	test(pA!=NULL);
	test(pA->Avail()==aSize);
	// Allocate whole free bma
	test_Equal(aSize, pA->SelectiveAlloc(0, aSize));
	test_Equal(0,pA->Avail());
	// Allocate whole full bma
	test_Equal(0, pA->SelectiveAlloc(0, aSize));
	test_Equal(0,pA->Avail());
	TInt i;
	TInt j;
	TInt l;
	for (i=2; i<8; ++i)
		{
		for (l=1; l<=aSize; ++l)
			{
			new (pA) TBitMapAllocator(aSize, ETrue);
			for (j=0; j<aSize; j+=i)
				pA->Alloc(j,1);
			TInt orig=pA->Avail();
			test_Equal(aSize-(aSize+i-1)/i, orig);
			TUint newAllocs = pA->SelectiveAlloc(0,l);
			TInt allocated = orig - pA->Avail();
			test_Equal(allocated, newAllocs);
			test_Equal(l - (l+i-1)/i, allocated);
			Check(*pA);
			}
		}
	for (i=0; i<=Min(32,aSize-1); ++i)
		{
		for (l=1; l<=aSize-i; ++l)
			{
			for (j=1; j<=aSize; ++j)
				{
				new (pA) TBitMapAllocator(aSize, ETrue);
				pA->Alloc(i,l);
				test_Equal(aSize-l, pA->Avail());
				TUint newAllocs = pA->SelectiveAlloc(0,j);
				TUint allocated = j - Max(0,Min(i+l,j)-i);
				test_Equal(allocated, newAllocs);
				test_Equal(pA->Avail(), aSize-l-allocated);
				test(!pA->NotAllocated(0,j));
				if (j>=i && j<i+l)
					test(!pA->NotAllocated(0,j+1));
				Check(*pA);
				}
			}
		}
	delete pA;
	}


TBitMapAllocator* DoSetupBMA(TInt aSize, VA_LIST aList)
	{
	TBitMapAllocator* pA=TBitMapAllocator::New(aSize, EFalse);
	test(pA!=NULL);
	test(pA->Avail()==0);
	TInt totalfree=0;
	FOREVER
		{
		TInt i=VA_ARG(aList,TInt);
		if (i<0)
			break;
		TInt l=VA_ARG(aList,TInt);
		pA->Free(i,l);
		totalfree+=l;
		}
	test(pA->Avail()==totalfree);
	return pA;
	}

TBitMapAllocator* SetupBMA(TInt aSize, ...)
	{
	VA_LIST list;
	VA_START(list,aSize);
	return DoSetupBMA(aSize,list);
	}

void PopulateRangeArray(RArray<SRange>& aArray, VA_LIST aList)
	{
	aArray.Reset();
	TInt n=0;
	FOREVER
		{
		SRange rng;
		rng.iBase=VA_ARG(aList,TInt);
		if (rng.iBase<0)
			break;
		rng.iLength=VA_ARG(aList,TInt);
		rng.iLength<<=8;
		rng.iLength+=n;
		++n;
		test(aArray.Append(rng)==KErrNone);
		}
	}

TInt FirstFitPos(RArray<SRange>& aArray, TInt aLength)
	{
	TInt c=aArray.Count();
	SRange* pR=&aArray[0];
	SRange* pE=pR+c;
	for (; pR<pE; ++pR)
		{
		TInt l=pR->iLength>>8;
		if (l>=aLength)
			{
//			test.Printf(_L("FFP %d = %d\n"),aLength,pR->iBase);
			return pR->iBase;
			}
		}
//	test.Printf(_L("FFP %d = -1\n"),aLength);
	return -1;
	}

TInt AlignedFirstFitPos(RArray<SRange>& aArray, TInt aSize, TInt aLength, TInt aAlign, TInt aBase, TInt aOffset=0, TBool aBestFit=EFalse)
	{
	TInt alignSize=1<<aAlign;
	TInt alignMask=alignSize-1;
	TInt minRun=0;
	TInt minRunStart=0;
	TBool runFound = EFalse;
	TInt c=aArray.Count();
	SRange* pR=&aArray[0];
	// Best fit mode should ignore any final run TBitMapAllocator will 
	// always ignore the final run in best fit mode and rely on carry being
	// checked by the caller.
	SRange* pE = pR + c - 1;
	if (!aBestFit || aSize > pE->iBase + (pE->iLength >> 8))
		pE++;

	for (; pR<pE; ++pR)
		{
		TInt l=pR->iLength>>8;
		TInt b=pR->iBase;
		if (aOffset != 0)
			{
			aOffset = ((aOffset + aBase + alignMask) & ~alignMask) - aBase;
			if (aOffset + aLength - 1 >= b + l)
				{// The offset is after the end of this region.
				continue;
				}
			l -= (aOffset <= b)? 0 : aOffset - b;
			b += (aOffset <= b)? 0 : aOffset - b;	// Start the search from aOffset
			}
		TInt ab=((b+aBase+alignMask)&~alignMask)-aBase;
		TInt al = l + b - ab;
		if (al >= aLength)
			{
			if (!aBestFit || l == aLength)
				{
//				test.Printf(_L("AFFP %d %d %d = %d\n"),aLength,aAlign,aBase,ab);
				return ab;
				}
			if (!runFound || minRun > l)
				{ 
				minRun = l;
				minRunStart = ab;
				runFound = ETrue;
				}
			}
		}
	if (runFound)
		{
		return minRunStart;
		}
//	test.Printf(_L("AFFP %d %d %d = -1\n"),aLength,aAlign,aBase);
	return -1;
	}

void DoConsecTest(TInt aSize, ...)
	{
	test.Printf(_L("DoConsecTest %d\n"),aSize);
	VA_LIST list;
	VA_LIST list2;
	VA_START(list,aSize);
	VA_START(list2,aSize);
	TBitMapAllocator* pA=DoSetupBMA(aSize,list2);
	RArray<SRange> rangeArray(8,_FOFF(SRange,iLength));
	PopulateRangeArray(rangeArray, list);
	TInt n;
	for (n=1; n<=aSize; ++n)
		{
		TInt r1=pA->AllocConsecutive(n,EFalse);
		TInt r2=FirstFitPos(rangeArray,n);
//		test.Printf(_L("ALC(%d,0) = %d [%d]\n"),n,r1,r2);
		test_Equal(r2, r1);
		}
	rangeArray.SortUnsigned();	// sort array in ascending order of length
	for (n=1; n<=aSize; ++n)
		{
		TInt r1=pA->AllocConsecutive(n,ETrue);
		TInt r2=FirstFitPos(rangeArray,n);
//		test.Printf(_L("ALC(%d,1) = %d [%d]\n"),n,r1,r2);
		test_Equal(r2, r1);
		}
	rangeArray.Close();
	delete pA;
	}

void DoAlignedTest(TInt aSize, ...)
	{
	test.Printf(_L("DoAlignedTest %d\n"),aSize);
	VA_LIST list;
	VA_LIST list2;
	VA_START(list,aSize);
	VA_START(list2,aSize);
	TBitMapAllocator* pA=DoSetupBMA(aSize,list2);
	RArray<SRange> rangeArray(8,_FOFF(SRange,iLength));
	PopulateRangeArray(rangeArray, list);
	TInt finalRunLength = 0;
	SRange& lastRun = rangeArray[rangeArray.Count() - 1];
	if (lastRun.iBase + (lastRun.iLength>>8) == aSize)
		{// The last run is at the end of the bma.
		finalRunLength = lastRun.iLength >> 8;
		}
	TInt a;
	TInt b;
	TInt n;
	TUint offset;
	for (a=0; ((1<<a)<=aSize); ++a)
		{
		TInt alignsize=1<<a;
		TInt alignmask=alignsize-1;
		for (b=0; b<(1<<a); ++b)
			{
//			test.Printf(_L("size %d a=%d b=%d First\n"),aSize,a,b);
			for (n=1; n<=aSize; ++n)
				{
				for (offset = 1; offset < (TUint)aSize; offset <<= 1)
					{
					TInt carry = 0;
					TInt runLength;
					TInt r1=pA->AllocAligned(n,a,b,EFalse, carry, runLength, offset);
					TInt r2=AlignedFirstFitPos(rangeArray,aSize, n,a,b, offset);
					if (r2 < 0 && pA->iSize == pA->iAvail)
						{// Totally empty bmas return KErrOverflow on failure.
						r2 = KErrOverflow;
						}
//					test.Printf(_L("ALA %d %d %d %d 0 = %d [%d]\n"),n,a,b,offset,r1,r2);
					test( (r1<0) || ((r1+b)&alignmask)==0 );
					test( (r1<0) || !pA->NotFree(r1,n));
					test( (r1<0) || runLength >= n);
					test_Equal(r2, r1);
					}
				}
			}
		}
	for (a=0; ((1<<a)<=aSize); ++a)
		{
		TInt alignsize=1<<a;
		TInt alignmask=alignsize-1;
		for (b=0; b<(1<<a); ++b)
			{
//			test.Printf(_L("size %d a=%d b=%d Best\n"),aSize,a,b);
			for (n=1; n<=aSize; ++n)
				{// test for with offset=0 as that has screwed best fit in past.
				for (offset = 0; offset < (TUint)aSize; offset <<= 1)
					{
					TInt carry = 0;
					TInt runLength;
					TInt r1=pA->AllocAligned(n,a,b,ETrue, carry, runLength, offset);
					TInt r2=AlignedFirstFitPos(rangeArray,aSize, n,a,b, offset, ETrue);
					if (pA->iSize == pA->iAvail)
						{// Totally empty bmas return KErrOverflow always on best fit mode.
						r2 = KErrOverflow;
						}
//					test.Printf(_L("ALA %d %d %d 1 = %d [%d]\n"),n,a,b,r1,r2);
					test( (r1<0) || ((r1+b)&alignmask)==0 );
					test( (r1<0) || !pA->NotFree(r1,n));
					test( (r1<0) || runLength >= n);
					test_Equal(r2, r1);
					// No carry in so if run found then carry should be zero.
					// If no run found then carry should set to the length of
					// any run at the end of the bma minus the aligned offset.
					TInt lost = 0;
					TInt alignOffset = ((offset + b + alignmask) & ~alignmask) - b;
					if (finalRunLength && offset &&	lastRun.iBase < alignOffset)
						{// This search had started past the start of the final run
						// so the final run length found will be shorter than its 
						// total length.
						if (alignOffset < aSize)
							{
							lost = Min(alignOffset - lastRun.iBase, finalRunLength);
							}
						else // alignedOffset starts past end of bma.
							lost = finalRunLength;
						}
					test((r1>=0 && carry == 0) || carry == finalRunLength - lost);
					offset = (offset)? offset : 1;
					}
				}
			}
		}
	rangeArray.Close();
	delete pA;
	}

void Clone(TAny* aDest, const TBitMapAllocator* aSrc)
	{
	TInt nmapw=(aSrc->iSize+31)>>5;
	TInt memsz=sizeof(TBitMapAllocator)+(nmapw-1)*sizeof(TUint32);
	Mem::Move(aDest,aSrc,memsz);
	}

void TestAllocConsecutive()
	{
	test.Printf(_L("TestAllocConsecutive\n"));
	DoConsecTest(256, 0,4 , 20,8 , 38,1 , 58,6 , 65,10, 78,16 , 127,72, 222,19 , 244,12 , -1);
	DoConsecTest(255, 0,2 , 3,2 , 6,3 , 10,3 , 14,5 , 20,5 , 26,7 , 34,7 , 42,11 , 54,11 , 66,13 , 80,37,
																	118,19 , 138,23 , 162,47 , 254,1 , -1);
	DoConsecTest(1023, 0,2 , 32,32 , 65,31 , 99,30 , 144,64 , 256,519 , 776,1, 778,245 , -1);

	DoAlignedTest(256, 0,4 , 20,8 , 38,1 , 58,6 , 65,10, 78,16 , 127,72, 222,19 , 244,12 , -1);
	DoAlignedTest(255, 0,2 , 3,2 , 6,3 , 10,3 , 14,5 , 20,5 , 26,7 , 34,7 , 42,11 , 54,11 , 66,13 , 80,37,
																	118,19 , 138,23 , 162,47 , 254,1 , -1);
	DoAlignedTest(1023, 0,2 , 32,32 , 65,31 , 99,30 , 144,64 , 256,519 , 776,1, 778,245 , -1);
	// Test some completely free bmas
	DoAlignedTest(255, 0,255, -1);
	DoAlignedTest(256, 0,256, -1);
	DoAlignedTest(1023, 0,1023, -1);
	DoAlignedTest(1024, 0,1024, -1);
	}

void DoTestChain(const TBitMapAllocator& aBma, TInt aNumSplits, ...)
	{
	test.Printf(_L("DoTestChain %d %d\n"),aBma.iSize,aNumSplits);
	VA_LIST list;
	VA_START(list,aNumSplits);

	TBmaList* pL=TBmaList::New(aBma,aNumSplits,list);
	test(pL!=NULL);

	TInt n;
	for (n=1; n<=aBma.iSize; ++n)
		{
		TInt r1=aBma.AllocConsecutive(n,EFalse);
		TInt r2=pL->AllocConsecutiveFF(n);
//		test.Printf(_L("CHAIN C FF %d: r1=%d r2=%d\n"),n,r1,r2);
		test(r1==r2);
		}
	for (n=1; n<=aBma.iSize; ++n)
		{
		TInt r1=aBma.AllocConsecutive(n,ETrue);
		TInt r2=pL->AllocConsecutiveBF(n);
//		test.Printf(_L("CHAIN C BF %d: r1=%d r2=%d\n"),n,r1,r2);
		test(r1==r2);
		}

	TInt a;
	for (a=0; ((1<<a)<=aBma.iSize); ++a)
		{
		for (n=1; n<=aBma.iSize; ++n)
			{
			if (n==264 && a==9)
				{
				++n;
				--n;
				}
			TInt r1=aBma.AllocAligned(n,a,0,EFalse);
			TInt r2=pL->AllocAlignedFF(n,a);
//			test.Printf(_L("CHAIN A FF %d,%d: r1=%d r2=%d\n"),n,a,r1,r2);
			test(r1==r2);
			}
		}
	for (a=0; ((1<<a)<=aBma.iSize); ++a)
		{
		for (n=1; n<=aBma.iSize; ++n)
			{
			if (n==240 && a==3)
				{
				++n;
				--n;
				}
			TInt r1=aBma.AllocAligned(n,a,0,ETrue);
			TInt r2=pL->AllocAlignedBF(n,a);
//			test.Printf(_L("CHAIN A BF %d,%d: r1=%d r2=%d\n"),n,a,r1,r2);
			test(r1==r2);
			}
		}

	delete pL;
	}

void TestChain()
	{
	test.Printf(_L("TestChain\n"));
	TBitMapAllocator* pA;
	pA=SetupBMA(1023, 0,2 , 32,32 , 65,31 , 99,30 , 144,64 , 256,519 , 776,1, 778,245 , -1);
	test(pA!=NULL);
	DoTestChain(*pA, 2, 300, 700);
	DoTestChain(*pA, 3, 64, 301, 702);
	delete pA;
	pA=SetupBMA(512, 0,2 , 20,10 , 32,32 , 65,31 , 144,64 , 399,113 , -1);
	test(pA!=NULL);
	DoTestChain(*pA, 2, 256, 384);
	DoTestChain(*pA, 3, 128, 256, 384);
	DoTestChain(*pA, 3, 80, 208, 384);
	DoTestChain(*pA, 3, 80, 208, 400);
	delete pA;
	}

void TestBitOps()
	{
	test.Next(_L("Bit operations (32 bit)"));
	test(__e32_find_ls1_32(0x00000000)==-1);
	TInt i, j, k;
	TInt count = 0;
	for (i=0; i<=31; ++i)
		test(__e32_find_ls1_32(1u<<i)==i);
	TUint x = 0;
	for (i=0; i<1000; ++i)
		{
		x = 69069*x + 41;
		TInt bit = x&31;

		x = 69069*x + 41;
		TUint y = ((x|1)<<bit);

		test(__e32_find_ls1_32(y) == bit);
		}

	test(__e32_find_ms1_32(0x00000000)==-1);
	for (i=0; i<=31; ++i)
		test(__e32_find_ms1_32(1u<<i)==i);
	for (i=0; i<1000; ++i)
		{
		x = 69069*x + 41;
		TInt bit = x&31;

		x = 69069*x + 41;
		TUint y = ((x|0x80000000u)>>bit);

		test(__e32_find_ms1_32(y) == 31-bit);
		}

	test(__e32_bit_count_32(0)==0);
	test(__e32_bit_count_32(0xffffffff)==32);
	for (i=0; i<32; ++i)
		{
		TUint32 y = 0xffffffffu << i;
		TUint32 z = 0xffffffffu >> i;
		test(__e32_bit_count_32(y) == 32-i);
		test(__e32_bit_count_32(z) == 32-i);
		test(__e32_bit_count_32(~y) == i);
		test(__e32_bit_count_32(~z) == i);
		}
	for (i=0; i<32; ++i)
		for (j=0; j<32; ++j)
			for (k=0; k<32; ++k)
				{
				TUint32 b0 = 1u<<i;
				TUint32 b1 = 1u<<j;
				TUint32 b2 = 1u<<k;
				TUint32 y = b0 | b1 | b2;
				TInt n;
				if (i==j && j==k) n=1;
				else if (i==j || j==k || i==k) n=2;
				else n=3;
				test(__e32_bit_count_32(y) == n);
				test(__e32_bit_count_32(~y) == 32-n);
				++count;
				}
	test.Printf(_L("%d iterations done\n"), count);
	for (i=0; i<=31; ++i)
		{
		test(__e32_bit_count_32(0xaaaaaaaau<<i)==16-(i+1)/2);
		test(__e32_bit_count_32(0x55555555u<<i)==16-i/2);
		}
	test(__e32_bit_count_32(0x33333333u)==16);
	test(__e32_bit_count_32(0x88888888u)==8);

	test.Next(_L("Bit operations (64 bit)"));
	test(__e32_find_ls1_64(0x00000000)==-1);
	for (i=0; i<=63; ++i)
		{
		TUint64 x = 1u;
		x<<=i;
		test(__e32_find_ls1_64(x)==i);
		}
	x = 487;
	for (i=0; i<1000; ++i)
		{
		x = 69069*x + 41;
		TInt bit = x&63;

		x = 69069*x + 41;
		TUint32 xl = x|1;
		x = 69069*x + 41;
		TUint32 xh = x;
		TUint64 y = MAKE_TUINT64(xh,xl);
		y <<= bit;
		test(__e32_find_ls1_64(y) == bit);
		}

	test(__e32_find_ms1_64(0x00000000)==-1);
	for (i=0; i<=63; ++i)
		{
		TUint64 x = 1u;
		x<<=i;
		test(__e32_find_ms1_64(x)==i);
		}
	x = 1039;
	for (i=0; i<1000; ++i)
		{
		x = 69069*x + 41;
		TInt bit = x&63;

		x = 69069*x + 41;
		TUint32 xl = x;
		x = 69069*x + 41;
		TUint32 xh = x|0x80000000u;
		TUint64 y = MAKE_TUINT64(xh,xl);
		y >>= bit;
		test(__e32_find_ms1_64(y) == 63-bit);
		}

	test(__e32_bit_count_64(0)==0);
	test(__e32_bit_count_64(MAKE_TUINT64(0x00000000,0xffffffff))==32);
	test(__e32_bit_count_64(MAKE_TUINT64(0xffffffff,0x00000000))==32);
	test(__e32_bit_count_64(MAKE_TUINT64(0xffffffff,0xffffffff))==64);
	for (i=0; i<64; ++i)
		{
		TUint64 y = MAKE_TUINT64(0xffffffff,0xffffffff);
		TUint64 z = y >> i;
		y <<= i;
		test(__e32_bit_count_64(y) == 64-i);
		test(__e32_bit_count_64(z) == 64-i);
		test(__e32_bit_count_64(~y) == i);
		test(__e32_bit_count_64(~z) == i);
		}
	count = 0;
	for (i=0; i<64; ++i)
		for (j=0; j<64; ++j)
			for (k=0; k<64; ++k)
				{
				TUint64 b0 = 1u;
				TUint64 b1 = 1u;
				TUint64 b2 = 1u;
				b0 <<= i;
				b1 <<= j;
				b2 <<= k;
				TUint64 y = b0 | b1 | b2;
				TUint64 z = ~y;
				TInt n;
				if (i==j && j==k) n=1;
				else if (i==j || j==k || i==k) n=2;
				else n=3;
				test(__e32_bit_count_64(y) == n);
				test(__e32_bit_count_64(z) == 64-n);
				++count;
				}
	test.Printf(_L("%d iterations done\n"), count);
	for (i=0; i<64; ++i)
		{
		TUint64 y = MAKE_TUINT64(0xaaaaaaaa,0xaaaaaaaa);
		TUint64 z = ~y;
		test(__e32_bit_count_64(y<<i)==32-(i+1)/2);
		test(__e32_bit_count_64(z<<i)==32-i/2);
		}
	test(__e32_bit_count_64(MAKE_TUINT64(0x33333333u,0x33333333u))==32);
	test(__e32_bit_count_64(MAKE_TUINT64(0x88888888u,0x88888888u))==16);
	}

GLDEF_C TInt E32Main()
	{
	test.Title();
	__UHEAP_MARK;
	test.Start(_L("TBitMapAllocator tests"));

	TestBitOps();

	TestConstruct(3);
	TestConstruct(29);
	TestConstruct(256);
	TestConstruct(487);

	TestAlloc1(3);
	TestAlloc1(29);
	TestAlloc1(256);
	TestAlloc1(181);

	TestFree1(3);
	TestFree1(29);
	TestFree1(256);
	TestFree1(181);

	TestBlockAlloc(3);
	TestBlockAlloc(29);
	TestBlockAlloc(256);
	TestBlockAlloc(179);

	TestBlockFree(3);
	TestBlockFree(31);
	TestBlockFree(256);
	TestBlockFree(149);

	TestNotFree(3);
	TestNotFree(31);
	TestNotFree(256);
	TestNotFree(149);

	TestNotAllocated(3);
	TestNotAllocated(31);
	TestNotAllocated(256);
	TestNotAllocated(149);

	TestAllocList(3);
	TestAllocList(31);
	TestAllocList(128);
	TestAllocList(149);

	TestSelectiveFree(3);
	TestSelectiveFree(31);
	TestSelectiveFree(128);
	TestSelectiveFree(149);

	TestSelectiveAlloc(3);
	TestSelectiveAlloc(31);
	TestSelectiveAlloc(128);
	TestSelectiveAlloc(149);

	TestAllocConsecutive();

	TestChain();

	__UHEAP_MARKEND;
	test.End();
	return 0;
	}
