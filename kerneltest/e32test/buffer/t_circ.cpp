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
// e32test\buffer\t_circ.cpp
// Overview:
// Test methods of CCirBuffer and CCirBuf template class.
// API Information:
// CCirBuffer, CCirBuf.
// Details:
// - Create a circular buffer, add integers to the circular buffer and check that 
// they are added as specified.
// - Verify Remove and Add work as expected.
// - Check that all the added objects are removed.
// - Create a circular buffer, add concrete data type object one, many at a time to the 
// circular buffer and check that they are added as specified.
// - Remove the concrete data type object one, many at a time from the circular buffer
// and check that they are removed as expected.
// - Check that all the added objects are removed.
// - Create a circular buffer, add 8-bit unsigned character object one, many at a time
// to the circular buffer and check that they are added as specified.
// - Remove multiple 8-bit unsigned character objects from the circular buffer and 
// check the added and removed objects are same.
// - Create a circular buffer of unsigned integers, add single, multiple objects to the 
// buffer and check that they are added as specified.
// - Remove multiple objects from the circular buffer and check the added and removed 
// objects are same.
// - Create a circular buffer, add multiple signed integer objects to it, remove 
// the objects and verify that the added objects are removed.
// - Create a circular buffer, add integers to the circular buffer and check that 
// they are added as specified.
// - Verify Remove works as expected.
// - Test whether the heap has been corrupted by all the tests.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

class VTester
	{
public:
	VTester(){a=b=c=d=0;}
	VTester(TInt anInt){a=b=c=d=anInt;}
	VTester& operator=(TInt anInt){a=b=c=d=anInt;return *this;}
	TBool operator==(const VTester& aVal) const{if (a==aVal.a && b==aVal.b && c==aVal.c && d==aVal.d) return 1; else return 0;}
	TBool operator==(const TInt& aVal) const{if (a==aVal && b==aVal && c==aVal && d==aVal) return 1; else return 0;}
public:
	TInt a;
	TInt b;
	TInt c;
	TInt d;
	};

LOCAL_D RTest test(_L("T_CIRC"));
LOCAL_D TText8* theCharArray=(TText8*)"abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ!\xA3$%^&*()\0";

LOCAL_C void TestInt()
//
// Test with unsigned integers
//
	{
	
	CCirBuf<TInt>* cbInt=new CCirBuf<TInt>;
	TRAPD(ret,cbInt->SetLengthL(5));
	test(ret==KErrNone);
	TInt one(1);
	TInt two(2);
	TInt three(3);
	TInt four(4);
	TInt five(5);
	TInt six(6);
	TInt dummy;
	TInt numbers[]={7,8,9,10,11,12,13,14,15};
	TInt* numBuf=new TInt[10];
	test(numBuf!=NULL);
	test(cbInt->Add(&one)==1);
	test(cbInt->Add(&two)==1);
	test(cbInt->Add(&three)==1);
	test(cbInt->Add(&four)==1);
	test(cbInt->Add(&five)==1);
	one=two=three=four=0;
	test(cbInt->Add(&six)==0);
	test(cbInt->Remove(&one)==1);
	test(cbInt->Add(&six)==1);
	test(cbInt->Add(&dummy)==0);
	test(cbInt->Remove(&two)==1);
	test(cbInt->Remove(&three)==1);
	test(cbInt->Remove(&four)==1);
	test(one==1);
	test(two==2);
	test(three==3);
	test(four==4);
	test(cbInt->Add(numbers,6)==3);
	test(cbInt->Add(numbers,3)==0);
	test(cbInt->Remove(numBuf,7)==5);
	test(cbInt->Remove(numBuf,5)==0);
	for(TInt j(0);j<5;j++)
		test(numBuf[j]==j+5);
	delete [] numBuf;
	delete cbInt;
	}

LOCAL_C void TestClass()
//
// Test with objects
//
	{

	CCirBuf<VTester>* cbInt=new CCirBuf<VTester>;
	TRAPD(ret,cbInt->SetLengthL(5));
	test(ret==KErrNone);
	VTester one(1);
	VTester two(2);
	VTester three(3);
	VTester four(4);
	VTester five(5);
	VTester six(6);
	VTester dummy(0xffff);
	VTester numbers[]={7,8,9,10,11,12,13,14,15};
	VTester* numBuf=new VTester[10];
	test(numBuf!=NULL);
	test(cbInt->Add(&one)==1);
	test(cbInt->Add(&two)==1);
	test(cbInt->Add(&three)==1);
	test(cbInt->Add(&four)==1);
	test(cbInt->Add(&five)==1);
	one=two=three=four=0;
	test(cbInt->Add(&six)==0);
	test(cbInt->Remove(&one)==1);
	test(cbInt->Add(&six)==1);
	test(cbInt->Add(&dummy)==0);
	test(cbInt->Remove(&two)==1);
	test(cbInt->Remove(&three)==1);
	test(cbInt->Remove(&four)==1);
	test(one==1);
	test(two==2);
	test(three==3);
	test(four==4);
	test(cbInt->Add(numbers,6)==3);
	test(cbInt->Add(numbers,3)==0);
	test(cbInt->Remove(numBuf,7)==5);
	test(cbInt->Remove(numBuf,5)==0);
	for(TInt j(0);j<5;j++)
		test(numBuf[j]==j+5);
	delete [] numBuf;
	delete cbInt;
	}

LOCAL_C void TestText()
//
// Test with text
//
	{

	TInt i,j;
	TInt arraySize=User::StringLength(theCharArray);
	TText8* buf=new TText8[arraySize+1];
	Mem::FillZ(buf,arraySize);
	CCirBuf<TText8>* cbInt=new CCirBuf<TText8>;
	TRAPD(ret,cbInt->SetLengthL(arraySize+11));
	test(ret==KErrNone);
	for (i=0;i<10;i++)
		{
		test(cbInt->Add(theCharArray,arraySize)==arraySize);
		test(cbInt->Add(theCharArray+i)==1);
		test(cbInt->Remove(buf,arraySize)==arraySize);
		}
	TRAP(ret,cbInt->SetLengthL(arraySize*60));
	test(ret==KErrNone);
	for (j=0;j<10;j++)
		{
		for (i=0;i<9;i++)
			test(cbInt->Add(theCharArray,arraySize)==arraySize);
		for (i=0;i<arraySize;i++)
			test(cbInt->Add(theCharArray+i)==1);
		for (i=0;i<5;i++)
			{
			Mem::FillZ(buf,arraySize);
			test(cbInt->Remove(buf,arraySize)==arraySize);
			test(Mem::Compare(buf,arraySize,theCharArray,arraySize)==KErrNone);
			}
		}
	delete [] buf;
	delete cbInt;
	}

void TestBuf()
//
// Test with buffers
//
	{

	TInt i,j;
	TInt arraySize=User::StringLength(theCharArray);
	TText8* buf=new TText8[arraySize+1];
	Mem::FillZ(buf,arraySize);
	CCirBuffer* cbInt=new CCirBuffer;
	TRAPD(ret,cbInt->SetLengthL(arraySize+11));
	test(ret==KErrNone);
	for (i=0;i<10;i++)
		{
		test(cbInt->Add(theCharArray,arraySize)==arraySize);
		test(cbInt->Add(theCharArray+i)==1);
		test(cbInt->Remove(buf,arraySize)==arraySize);
		}
	TRAP(ret,cbInt->SetLengthL(arraySize*60));
	test(ret==KErrNone);
	for (j=0;j<10;j++)
		{
		for (i=0;i<9;i++)
			test(cbInt->Add(theCharArray,arraySize)==arraySize);
		for (i=0;i<arraySize;i++)
			test(cbInt->Add(theCharArray+i)==1);
		for (i=0;i<5;i++)
			{
			Mem::FillZ(buf,arraySize);
			test(cbInt->Remove(buf,arraySize)==arraySize);
			test(Mem::Compare(buf,arraySize,theCharArray,arraySize)==KErrNone);
			}
		}
	
	// Test Reset, Put and Get
	TInt count = cbInt->Count();
	test(count>0);
	cbInt->Reset();
	count = cbInt->Count();
	test(count==0);
	TUint index = 0;
	
	// Put 100 integers to the circular buffer.
	TUint numberOfObjects= 100;
	for(index=1;index<=numberOfObjects; index++)
	    {
	    TInt result= cbInt->Put(index);
	    User::LeaveIfError(result);
	    }
	count = cbInt->Count();
	test(count==100);
	
	// Get 50 integers from the circular buffer.
	for(index=1;index<=(numberOfObjects/2); index++)
	    {
	    TUint cb = cbInt->Get();
	    test(cb==index);
	    }
	count = cbInt->Count();
	test(count==50);
		
	delete [] buf;
	delete cbInt;
	}

LOCAL_C void TestRemove()
//
// Show remove bug (fixed in rel 050)
//
{

	CCirBuf<TInt>* cbInt=new CCirBuf<TInt>; TRAPD(ret,cbInt->SetLengthL(5));
	test(ret==KErrNone);
	TInt numbers[]={1,2,3,4,5};
	TInt* result=new TInt[5];

	test(cbInt->Add(numbers,5)==5);
	test(cbInt->Remove(result,2)==2);
	test(result[0]==1);
	test(result[1]==2);
	test(cbInt->Remove(result,3)==3);
	test(result[0]==3);
	test(result[1]==4);
	test(result[2]==5);

	delete [] result;
	delete cbInt;
	}


TInt E32Main()
//
// Test CCirBuf<T>
//
    {

	test.Title();
	__UHEAP_MARK;
//
	test.Start(_L("Testing with built in Type"));
	TestInt();
//
	test.Next(_L("Testing with concrete data type"));
	TestClass();
//	
	test.Next(_L("Testing with text"));
	TestText();
//
	test.Next(_L("Testing character buffer"));
	TestBuf();
//
	test.Next(_L("Testing Remove"));
	TestRemove();

	__UHEAP_MARKEND;
	test.End();
	return(KErrNone);
	}

