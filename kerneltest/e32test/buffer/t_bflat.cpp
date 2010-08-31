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
// e32test\buffer\t_bflat.cpp
// Overview:
// Test all aspects of the CBufFlat class.
// API Information:
// CBufFlat.
// Details:
// - Test all the operations of the class and see if methods are implemented -- 
// including NewL, Reset, Size, Set Reserve, InsertL, Delete, Ptr, Read, ResizeL, Write and Compress.
// - Test CBufFlat constructor is as expected.
// - Insert data into the flat storage dynamic buffer and verify that InsertL method
// is as expected.
// - Delete all data in buffer using Reset() method  and check size is zero.
// - Test Ptr, Free, Size, Backptr and SetReserveL methods work as expected.
// - Insert data into the buffer, delete some data from the beginning, middle, end and
// check for data is as expected. 
// - Verify the data in the buffer before and after Compress and Read methods is as expected.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("T_BFLAT"));

class TestCBufFlat
	{
public:
	void Test1();  // Tests all operations of the class.
	void Test2();	// Test public methods of class.
	};

class TestCBufSeg
	{
public:
	void Test1(); 	// Test all operations of the class.
	};

GLDEF_C void TestCBufFlat::Test1()
//
// Tests all operations of the class.
//
	{
	TText* tp;
	test.Start(_L("Test all operations of CBufFlat"));
	CBufFlat* bf=(CBufFlat*)CBufFlat::NewL(100);
	bf->Reset();
	bf->Size();
	bf->SetReserveL(50); // Panics if 50 < iSize
	bf->InsertL(0,TPtrC8((TText8*)"Hello World"));
	bf->Delete(0,5);
	tp=(TText*)bf->Ptr(3).Ptr();
	tp=(TText*)bf->BackPtr(3).Ptr();
	bf->Read(2,tp,2);
	bf->Write(2,tp,2);
	bf->Compress();
	test.End();
	}

GLDEF_C void TestCBufFlat::Test2()
//
// Test all the methods of the class
//
	{

	test.Start(_L("Test constructor of CBufFlat"));
	CBufFlat* bf1=(CBufFlat*)CBufFlat::NewL(20);
	test(bf1->Size()==0);
	test(bf1->Ptr(0).Length()==0);

	test.Next(_L("Insert, Reset, Ptr, Free, Size"));
	TBuf8<0x40> tb1=(TText8*)"Hello World";
	TBuf8<0x40> tb2=(TText8*)"This string is greater than twenty characters long";
	TBuf8<0x40> tb3;
	bf1->InsertL(0,tb1); // Insert - no expand
	test(bf1->Ptr(0)==tb1);
	test(bf1->Size()==tb1.Size());
	test(bf1->Ptr(0).Length()==tb1.Size());
	bf1->InsertL(bf1->Size(),tb2); // Insert and expand
	test(bf1->Size()==(tb1.Size()+tb2.Size()));
	test(bf1->Ptr(0).Left(tb1.Length())==tb1);
	test(bf1->Ptr(tb1.Length())==tb2);
	bf1->InsertL(bf1->Size(),tb3);	// Insert a null string
	test(bf1->Size()==(tb1.Size()+tb2.Size()));
	test(bf1->Ptr(0).Left(tb1.Length())==tb1);
	test(bf1->Ptr(tb1.Length())==tb2);
	bf1->Reset(); // Reset
	test(bf1->Size()==0);
	bf1->InsertL(0,tb1); // Insert into a string
	bf1->InsertL(5,tb1);
	bf1->Delete(16,bf1->Size()-16);
	test(bf1->Ptr(0)==TPtrC8((TText8*)"HelloHello World"));
	bf1->InsertL(10,tb1,5);
	test(bf1->Ptr(0)==TPtrC8((TText8*)"HelloHelloHello World"));
//
	test.Next(_L("SetReserve"));
	bf1->SetReserveL(50); // SetReserve > 0
	test(bf1->Size()==21);
	test(bf1->Ptr(0).Length()==21);
	bf1->Reset();
	bf1->SetReserveL(0); // SetReserve = 0
	test(bf1->Size()==0);
	test(bf1->Ptr(0).Length()==0);
//
	test.Next(_L("Delete, BackPtr"));
	bf1->InsertL(0,tb1);
	bf1->Delete(6,1); // Delete Middle
	test(bf1->Ptr(0)==TPtrC8((TText8*)"Hello orld"));
	test(bf1->Size()==10);
	bf1->Delete(9,1); // Delete End
	test(bf1->Ptr(bf1->Size()).Length()==0);
	bf1->InsertL(bf1->Size(),tb3);
	test(bf1->Ptr(0)==TPtrC8((TText8*)"Hello orl"));
	bf1->Delete(0,2); // Delete Start / BackPtr
	test(bf1->BackPtr(5)==TPtrC8((TText8*)"llo o"));
	test(bf1->Size()==7);
//
	test.Next(_L("Write, Compress"));
	bf1->Write(1,tb1,5);
	test(bf1->Ptr(0)==TPtrC8((TText8*)"lHellol"));
	test(bf1->Size()==7);
	bf1->Compress(); // Compress
	test(bf1->Size()==7);

	test.Next(_L("Read, Resize"));
	bf1->Read(4,tb1,bf1->Size()-4);
	test(tb1.Size()==3);
	test(tb1==TPtrC8((TText8*)"lol"));
	TBuf8<0x10> tb4=(TText8*)"Hello Hello Sun ";;
	TBuf8<0x10> tb5;
	test(bf1->Size()==7);
	bf1->ResizeL(64); // ResizeL
	test(bf1->Size()==64);
	bf1->Write(0,tb4,16);
	bf1->Write(16,tb4,16);
	bf1->Write(32,tb4,16);
	bf1->Write(48,tb4,16);
	bf1->Read(0,tb5); //Reads maxlength of tb5 that is 16
	bf1->Read(0,tb3); //Reads maxlength of tb3 that is 64
	test(tb5==TPtrC8((TText8*)"Hello Hello Sun "));
	test(tb3==TPtrC8((TText8*)"Hello Hello Sun Hello Hello Sun Hello Hello Sun Hello Hello Sun "));
//
	test.End();
	}

LOCAL_C void test_CBufFlat()
//
// Test the BufFlat class.
//
	{
	TestCBufFlat b;

	test.Start(_L("All operations"));
	b.Test1();
	test.Next(_L("All methods"));
	b.Test2();
//
	test.End();
	}

GLDEF_C TInt E32Main()
//
// Test the ADT Varray types.
//
	{
	test.Title();
	test.Start(_L("class CBufFlat"));
	test_CBufFlat();
	test.End();
	return(0);
	}

