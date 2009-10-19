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
// e32test\buffer\bin_srch.cpp
// 
//

#include <e32test.h>
#include <e32math.h>

GLREF_D RTest test;

#define KEEP_RUNNING	100

struct TestMe
	{
	TBuf<0x10>	name;
	TInt32		key1;
	TUint32		key2;
	};

LOCAL_C void fillArray(RArray<TestMe>& arr, TInt size)
	{
	TInt32 seed = 1 + Math::Random() % size;
	TestMe testMe;
	for(TInt i=0;i<size;i++)
		{
		testMe.key1 = seed;
		arr.Append(testMe);
		seed += 2 + Math::Random() % (2 + size%5);
		}

	}

LOCAL_C void fillArray(RArray<TInt32>& arr, TInt size)
	{
	TInt32 seed = 1 + Math::Random() % size;
	for(TInt i=0;i<size;i++)
		{
		arr.Append(seed);
		seed += 2 + Math::Random() % (2 + size%5);
		}
	}

LOCAL_C void fillArray(RArray<TInt32>& arr, RPointerArray<TUint32>& parr, TInt size)
	{
	TInt32 seed = 1 + Math::Random() % size;
	TInt i;
	for(i=0;i<size;i++)
		{
		arr.Append(seed);
		seed += 2 + Math::Random() % (2 + size%5);
		}
	for(i=0;i<size;i++)
		{
		parr.Append((const TUint32*)&arr[i]);
		}
	}

LOCAL_C void fillArray(RPointerArray<TUint32>& arr, TInt size)
	{
	TUint32 seed = 1 + Math::Random() % size;
	TUint32 dummy;
	for(TInt i=0;i<size;i++)
		{
		arr.Append((&dummy) + seed);
		seed += 2 + Math::Random() % (2 + size%5);
		}
	}

LOCAL_C TInt simpleOrder(const TInt32& a1, const TInt32& a2)
	{
	return a1 - a2;
	}

LOCAL_C TInt simpleOrder2(const TUint32& a1, const TUint32& a2)
	{
	return (a1==a2)?0:(a1>a2?1:-1);
	}

GLDEF_C void DoRArrayTests()
	{
	{
	RArray<TInt32>* rArr1 = new RArray<TInt32>(0x10);
	test(rArr1!=NULL);
	RPointerArray<TUint32>* rpArr1 = new RPointerArray<TUint32>(0x10);
	test(rpArr1!=NULL);
	TInt i;
	TInt size = 25;
	test.Next(_L("Testing RArray::FindInOrder, RPointerArray::FindInOrder, RArrayBase::BinarySearch and RPointerArrayBase::BinarySearch with arrays of different sizes\r\n"));
	for(i=0;i<KEEP_RUNNING;i++)
		{
		test.Printf(_L("Testing with a random array of size %d \r\n"), size);
		fillArray(*rArr1,*rpArr1,size);
		test(rArr1->Count()==rpArr1->Count());
		TInt index;
		//test(KErrNotFound==rArr1->BinarySearch((TAny*)(rArr1->operator[](0)-1),index,(TGeneralLinearOrder)simpleOrder));
		test(KErrNotFound==rArr1->FindInOrder(rArr1->operator[](0)-1,index,TLinearOrder<TInt32>(simpleOrder)));
		test(index==0);
		TUint32 t = *rpArr1->operator[](0)-1;
		test(KErrNotFound==rpArr1->FindInOrder(&t,index,TLinearOrder<TUint32>(simpleOrder2)));
		test(index==0);
		for(TInt k=0;k<rArr1->Count();k++)
			{
			test(KErrNone==rArr1->FindInOrder(rArr1->operator[](k),index,TLinearOrder<TInt32>(simpleOrder)));
			test(index==k);
			test(KErrNone==rpArr1->FindInOrder(rpArr1->operator[](k),index,TLinearOrder<TUint32>(simpleOrder2)));
			test(index==k);
			if(k<rArr1->Count()-1)
				{
				test(KErrNotFound==rArr1->FindInOrder((rArr1->operator[](k)+rArr1->operator[](k+1))>>1,index,TLinearOrder<TInt32>(simpleOrder)));
				test(index==k+1);
				t = (*rpArr1->operator[](k)+*rpArr1->operator[](k+1))>>1;
				test(KErrNotFound==rpArr1->FindInOrder(&t,index,TLinearOrder<TUint32>(simpleOrder2)));
				test(index==k+1);
				}
			}
		test(KErrNotFound==rArr1->FindInOrder(rArr1->operator[](rArr1->Count()-1)+5,index,TLinearOrder<TInt32>(simpleOrder)));
		test(index==rArr1->Count());
		t = *rpArr1->operator[](rpArr1->Count()-1) + 5;
		test(KErrNotFound==rpArr1->FindInOrder(&t,index,TLinearOrder<TUint32>(simpleOrder2)));
		test(index==rpArr1->Count());
		size += 2 + Math::Random() % (2 + size%5);
		rArr1->Reset();
		rpArr1->Reset();
		}
	delete rpArr1;

	test.Next(_L("Testing RArray::FindInSignedKeyOrder and RArrayBase::BinarySignedSearch with arrays of different sizes\r\n"));	
	for(i=0;i<KEEP_RUNNING;i++)
		{
		test.Printf(_L("Testing with a random array of size %d \r\n"), size);
		fillArray(*rArr1,size);
		TInt index;
		//test(KErrNotFound==rArr1->BinarySearch((TAny*)(rArr1->operator[](0)-1),index,(TGeneralLinearOrder)simpleOrder));
		test(KErrNotFound==rArr1->FindInSignedKeyOrder(rArr1->operator[](0)-1,index));
		test(index==0);
		for(TInt k=0;k<rArr1->Count();k++)
			{
			test(KErrNone==rArr1->FindInSignedKeyOrder(rArr1->operator[](k),index));
			test(index==k);
			if(k<rArr1->Count()-1)
				{
				test(KErrNotFound==rArr1->FindInSignedKeyOrder((rArr1->operator[](k)+rArr1->operator[](k+1))>>1,index));
				test(index==k+1);
				}
			}
		test(KErrNotFound==rArr1->FindInSignedKeyOrder(rArr1->operator[](rArr1->Count()-1)+5,index));
		test(index==rArr1->Count());
		size += 2 + Math::Random() % (2 + size%5);
		rArr1->Reset();
		}

	size=25;
	test.Next(_L("Testing RArray::FindInUnsignedKeyOrder and RArrayBase::BinaryUnsignedSearch with arrays of different sizes\r\n"));	
	for(i=0;i<KEEP_RUNNING;i++)
		{
		test.Printf(_L("Testing with a random array of size %d \r\n"), size);
		fillArray(*rArr1,size);
		TInt index;
		//test(KErrNotFound==rArr1->BinarySearch((TAny*)(rArr1->operator[](0)-1),index,(TGeneralLinearOrder)simpleOrder));
		test(KErrNotFound==rArr1->FindInUnsignedKeyOrder(rArr1->operator[](0)-1,index));
		test(index==0);
		for(TInt k=0;k<rArr1->Count();k++)
			{
			test(KErrNone==rArr1->FindInUnsignedKeyOrder(rArr1->operator[](k),index));
			test(index==k);
			if(k<rArr1->Count()-1)
				{
				test(KErrNotFound==rArr1->FindInUnsignedKeyOrder((rArr1->operator[](k)+rArr1->operator[](k+1))>>1,index));
				test(index==k+1);
				}
			}
		test(KErrNotFound==rArr1->FindInUnsignedKeyOrder(rArr1->operator[](rArr1->Count()-1)+5,index));
		test(index==rArr1->Count());
		size += 2 + Math::Random() % (2 + size%5);
		rArr1->Reset();
		}
	delete rArr1;
	}

	{
	RArray<TestMe>* rArr1 = new RArray<TestMe>(0x10,_FOFF(TestMe,key1));
	test(rArr1!=NULL);
	TInt i;
	TInt size = 25;
	test.Next(_L("Testing RArray::FindInSignedOrder and RArrayBase::BinarySignedSearch with a structure + key\r\n"));
	TestMe testMe;
	for(i=0;i<KEEP_RUNNING;i++)
		{
		test.Printf(_L("Testing with a random array of size %d \r\n"), size);
		fillArray(*rArr1,size);
		TInt index;
		//test(KErrNotFound==rArr1->BinarySearch((TAny*)(rArr1->operator[](0)-1),index,(TGeneralLinearOrder)simpleOrder));
		testMe=rArr1->operator[](0);
		testMe.key1 = rArr1->operator[](0).key1-1;
		test(KErrNotFound==rArr1->FindInSignedKeyOrder(testMe,index));
		test(index==0);
		for(TInt k=0;k<rArr1->Count();k++)
			{
			testMe.key1 = rArr1->operator[](k).key1;
			test(KErrNone==rArr1->FindInSignedKeyOrder(testMe,index));
			test(index==k);
			if(k<rArr1->Count()-1)
				{
				testMe.key1 = (rArr1->operator[](k).key1+rArr1->operator[](k+1).key1)>>1;
				test(KErrNotFound==rArr1->FindInSignedKeyOrder(testMe,index));
				test(index==k+1);
				}
			}
		testMe.key1 = rArr1->operator[](rArr1->Count()-1).key1+5;
		test(KErrNotFound==rArr1->FindInSignedKeyOrder(testMe,index));
		test(index==rArr1->Count());
		size += 2 + Math::Random() % (2 + size%5);
		rArr1->Reset();
		}

	size=25;
	test.Next(_L("Testing RArray::FindInUnsignedKeyOrder and RArrayBase::BinaryUnsignedSearch with arrays of different sizes\r\n"));	
	for(i=0;i<KEEP_RUNNING;i++)
		{
		test.Printf(_L("Testing with a random array of size %d \r\n"), size);
		fillArray(*rArr1,size);
		TInt index;
		//test(KErrNotFound==rArr1->BinarySearch((TAny*)(rArr1->operator[](0)-1),index,(TGeneralLinearOrder)simpleOrder));
		testMe.key1 = rArr1->operator[](0).key1-1;
		test(KErrNotFound==rArr1->FindInUnsignedKeyOrder(testMe,index));
		test(index==0);
		for(TInt k=0;k<rArr1->Count();k++)
			{
			testMe.key1 = rArr1->operator[](k).key1;
			test(KErrNone==rArr1->FindInUnsignedKeyOrder(testMe,index));
			test(index==k);
			if(k<rArr1->Count()-1)
				{
				testMe.key1 = (rArr1->operator[](k).key1+rArr1->operator[](k+1).key1)>>1;
				test(KErrNotFound==rArr1->FindInUnsignedKeyOrder(testMe,index));
				test(index==k+1);
				}
			}
		testMe.key1 = rArr1->operator[](rArr1->Count()-1).key1+5;
		test(KErrNotFound==rArr1->FindInUnsignedKeyOrder(testMe,index));
		test(index==rArr1->Count());
		size += 2 + Math::Random() % (2 + size%5);
		rArr1->Reset();
		}
	delete rArr1;
	}

	{
	RPointerArray<TUint32>* rArr1 = new RPointerArray<TUint32>(0x10);
	test(rArr1!=NULL);
	TInt i;
	TInt size = 25;
	test.Next(_L("Testing RPointerArray::FindInAddressOrder and RPointerArrayBase::BinaryUnsignedSearch with arrays of different sizes\r\n"));
	for(i=0;i<KEEP_RUNNING;i++)
		{
		test.Printf(_L("Testing with a random array of size %d \r\n"), size);
		fillArray(*rArr1,size);
		TInt index;
		//test(KErrNotFound==rArr1->BinarySearch((TAny*)(rArr1->operator[](0)-1),index,(TGeneralLinearOrder)simpleOrder));
		test(KErrNotFound==rArr1->FindInAddressOrder(rArr1->operator[](0)-1,index));
		test(index==0);
		for(TInt k=0;k<rArr1->Count();k++)
			{
			test(KErrNone==rArr1->FindInAddressOrder(rArr1->operator[](k),index));
			test(index==k);
			if(k<rArr1->Count()-1)
				{
				test(KErrNotFound==rArr1->FindInAddressOrder((const TUint32*)(((TUint32)rArr1->operator[](k))/2+((TUint32)rArr1->operator[](k+1))/2 + (((TUint32)rArr1->operator[](k))%2 + ((TUint32)rArr1->operator[](k+1))%2)/2),index));
				test(index==k+1);
				}
			}
		test(KErrNotFound==rArr1->FindInAddressOrder(rArr1->operator[](rArr1->Count()-1)+5,index));
		test(index==rArr1->Count());
		size += 2 + Math::Random() % (2 + size%5);
		rArr1->Reset();
		}

	delete rArr1;
	}

	}
