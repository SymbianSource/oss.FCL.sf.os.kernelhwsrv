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
// e32test\buffer\t_bseg.cpp
// Overview:
// Test all aspects of the CBufSeg class.
// API Information:
// CBufSeg.
// Details:
// - Create a segmented dynamic buffer of specified size.
// - Test all operations of CBufSeg class and see if methods are implemented -- 
// including NewL, Reset, Size, InsertL, Delete, Ptr, BackPtr, Read, Write and Compress.
// - Test CBufSeg constructor is as expected.
// - Insert data into the segmented dynamic buffer and verify that InsertL method
// is as expected using the Read and Write methods.
// - Delete all data using Reset method and check size is zero.
// - Test InsertL, Read, Write and Length methods.
// - Test Ptr, Free, Size, Backptr and SetReserveL methods are as expected.
// - Check self consistancy of segment lengths, and check contents of segmented 
// buffer using Ptr() and BackPtr().
// - Verify the size of the of the test buffers are correct.
// - Insert data into the buffer, delete some data from the beginning, middle and end
// then check for results as expected. 
// - Verify the data in the buffer before and after Compress method is as expected.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

class TBufSegLink : public TDblQueLink
	{
public:
	inline TBufSegLink() : iLen(0) {}
	inline TBufSegLink* Next() const {return((TBufSegLink*)iNext);}
	inline TBufSegLink* Prev() const {return((TBufSegLink*)iPrev);}
public:
	TInt iLen;
	};

class TestCBufSeg
	{
public:
	TestCBufSeg() : iDummy(0) {};
	void Test1(); 	// Test all operations of the class.
	void Test2();	// Inherited Methods
	void Test3();	// Insert	
	void Test4();	// Delete
	void Test5();	// Compress
	void Test6L();	// borderlines
protected:
	TInt iDummy;
	static const TInt SegLen;
	void CheckSeg(const CBufSeg*);
	void CheckContents1(CBufSeg* const);
	void CheckContents2(CBufSeg* const);
	};

const TInt TestCBufSeg::SegLen=64;

LOCAL_D RTest test(_L("T_BSEG"));

class CSpy : public CBufBase
	{
public:
	~CSpy();
protected:
	CSpy(TInt anExpandSize);
public:
    TDblQue<TBufSegLink> iQue;
	TBufSegLink* iSeg;
	TInt iBase;
	TInt iOffset;
	};

GLDEF_C void TestCBufSeg::CheckSeg(const CBufSeg* bf)
//
// Check Self consistancy of segment lengths
//
	{

	if (bf->Size()==0)
		return;
	TInt sum=0;
	TBufSegLink* p1=((CSpy*)bf)->iQue.First();
	while (!((CSpy*)bf)->iQue.IsHead(p1))
		{
		test(p1->iLen<=bf->Size());
		sum+=p1->iLen;
		p1=p1->Next();
		} 
	test(sum==bf->Size());
	sum=0;
	p1=((CSpy*)bf)->iQue.Last();
	while (!((CSpy*)bf)->iQue.IsHead(p1))
		{
		test(p1->iLen<=bf->Size());
		sum+=p1->iLen;
		p1=p1->Prev();
		}
	test(sum==bf->Size());
	}

GLDEF_C void TestCBufSeg::CheckContents1(CBufSeg* const bf)
//
// Check contents of segmented buffer using Ptr()
//
	{

	TInt sum=0;
	TInt nbytes=bf->Size();
	for (TInt pos=0;pos<nbytes;)
		{
		TPtr8 p=bf->Ptr(pos);
		TInt len=p.Length();
		TInt8* pT=(TInt8*)p.Ptr();
		for (TInt i=0;i<len;i++)
			sum+=*pT++;
		test(sum==0);
		pos+=len;
		}
	}

GLDEF_C void TestCBufSeg::CheckContents2(CBufSeg* const bf)
//
// Check contents of segmented buffer using BackPtr()
//
	{

	TInt sum=0;
	TInt nbytes=bf->Size();
	for(TInt pos=nbytes;pos>0;)
		{
		TPtr8 p=bf->BackPtr(pos);
		TInt len=p.Length();
		TInt8* pT=(TInt8*)p.Ptr();
		for (TInt i=0;i<len;i++)
			sum+=*pT++;
		pos-=len;
		}
	test(sum==0);
	}

GLDEF_C void TestCBufSeg::Test1()
//
// Test all operations of BufSeg class
//
	{

	test.Start(_L("Test all operations of CBufSeg"));
	CBufSeg* bf=(CBufSeg*)CBufSeg::NewL(100);
	bf->Reset();
	bf->Size();
	bf->InsertL(0,TPtrC8((TText8*)"Hello World"));
	bf->Delete(0,5);
	TText8* tp=(TText8*)bf->Ptr(3).Ptr();
	tp=(TText8*)bf->BackPtr(3).Ptr();
	TBuf8<0x20> txt;
	bf->Read(2,txt,2);
	bf->Write(2,txt,2);
	bf->Compress();
	delete bf;
	test.End();
	}
			
GLDEF_C void TestCBufSeg::Test2()
//
// Test all inherited methods
//
	{

	TBuf8<0x40> tb1=(TText8*)"Hello World";
	TBuf8<0x40> tb2=(TText8*)"String number two";
	TBuf8<0x40> tb3;
	test.Start(_L("Free,Size,Read,Write,Reset"));
	CBufSeg* bf=(CBufSeg*)CBufSeg::NewL(SegLen);
	test(bf->Size()==0);
	bf->InsertL(0,tb1);
	test(bf->Size()==tb1.Length());
	bf->Read(6,tb3,5);
	test(tb3==tb1.Right(5));
	bf->Write(1,tb2,6);
	bf->Read(0,tb3,bf->Size());
	test(tb3==TPtrC8((TText8*)"HStringorld"));
	bf->Reset();
	test(bf->Size()==0);
	while (bf->Size()<400)
		{
		bf->InsertL(bf->Size(),tb1);
		bf->InsertL(bf->Size(),tb2);
		}
	TInt i=0;
	while (i<400)
		{
		bf->Read(i,tb3,tb1.Size());
		test(tb3==tb1);
		i+=tb1.Length();
		bf->Read(i,tb3,tb2.Size());
		test(tb3==tb2);
		i+=tb2.Length();
		}
	i=0;
	while (i<400)
		{
		bf->Write(i,tb2);
		i+=tb2.Length();
		bf->Write(i,tb1);
		i+=tb1.Length();
		}
	i=0;
	while (i<400)
		{
		bf->Read(i,tb3,tb2.Size());
		test(tb3==tb2);
		i+=tb2.Length();
		bf->Read(i,tb3,tb1.Size());
		test(tb3==tb1);
		i+=tb1.Length();
		}
	delete bf;
	test.End();
	}

GLDEF_C void TestCBufSeg::Test3()
//
//	Test input methods
//
	{

	TInt8 bb[1000];
	TInt nbytes;
	test.Start(_L("InsertL"));
	CBufSeg* bf1=(CBufSeg*)CBufSeg::NewL(SegLen);
	CBufSeg* bf2=(CBufSeg*)CBufSeg::NewL(SegLen);
	CBufSeg* bf3=(CBufSeg*)CBufSeg::NewL(SegLen);
	nbytes=0;
	TInt k;
	for(TInt j=0;j<20;j++)
		{
		for(TInt i=0;i<10*j;i+=2)
			{
			k=i%128;
			bb[i]=(TInt8)k;
			bb[i+1]=(TInt8)-k;
			}
		bf1->InsertL(bf1->Size()/3*2,&bb[0],10*j);
		CheckSeg(bf1);
		CheckContents1(bf1);
		CheckContents2(bf1);
		bf2->InsertL(bf2->Size(),&bb[0],10*j);
		CheckSeg(bf2);
		CheckContents1(bf2);
		CheckContents2(bf2);
		bf3->InsertL(0,&bb[0],10*j);
		CheckSeg(bf3);
		CheckContents1(bf3);
		CheckContents2(bf3);
		nbytes+=10*j;
		}
	test(nbytes==bf1->Size());
	test(nbytes==bf2->Size());
	test(nbytes==bf3->Size());
	delete bf1;
	delete bf2;
	delete bf3;
	test.End();
	}

GLDEF_C void TestCBufSeg::Test4()
//
// Delete
// 
	{
	TInt8 bb[1000];

	test.Start(_L("Delete"));
	CBufSeg* bf1=(CBufSeg*)CBufSeg::NewL(SegLen);
	CBufSeg* bf2=(CBufSeg*)CBufSeg::NewL(SegLen);
	CBufSeg* bf3=(CBufSeg*)CBufSeg::NewL(SegLen);
	TInt nbytes=0;
	TInt k;
	for(TInt j=0;j<20;j++)
		{
		for(TInt i=0;i<10*j;i+=2)
			{
			k=i%128;
			bb[i]=(TInt8)k;
			bb[i+1]=(TInt8)-k;
			}
		bf1->InsertL(bf1->Size()/3*2,&bb[0],10*j);
		bf2->InsertL(bf2->Size(),&bb[0],10*j);
		bf3->InsertL(0,&bb[0],10*j);
		nbytes+=10*j;
		}
	TInt len=34;
	TInt aLength;
	while (nbytes>len)
		{
		for (TInt pos=0;pos<nbytes;pos+=44)
		{
			len=((len+17)%23)*2;
			if (len>nbytes-pos)
				len=nbytes-pos;
			bf1->Delete(pos,len);
			CheckSeg(bf1);
			CheckContents1(bf1);
			CheckContents2(bf1);
			aLength=bf2->Ptr(0).Length();
			aLength=((aLength>len) ? aLength : len);
			bf2->Delete(aLength-len,len);
			CheckSeg(bf2);
			CheckContents1(bf2);
			CheckContents2(bf2);
			bf3->Delete(0,len);
			CheckSeg(bf3);
			CheckContents1(bf3);
			CheckContents2(bf3);
			nbytes-=len;
			test(nbytes==bf1->Size());
			test(nbytes==bf2->Size());
			test(nbytes==bf3->Size());
			}
		}
	delete bf1;
	delete bf2;
	delete bf3;
	test.End();
	}

GLDEF_C void TestCBufSeg::Test5()
//
// Compress
// 
	{
	TInt8 bb[1000];

	test.Start(_L("Compress"));
	CBufSeg* bf1=(CBufSeg*)CBufSeg::NewL(SegLen);
	CBufSeg* bf2=(CBufSeg*)CBufSeg::NewL(SegLen);
	CBufSeg* bf3=(CBufSeg*)CBufSeg::NewL(SegLen);
	TInt nbytes=0;
	TInt k;
	for(TInt j=0;j<20;j++)
		{
		for(TInt i=0;i<10*j;i+=2)
			{
			k=i%128;
			bb[i]=(TInt8)k;
			bb[i+1]=(TInt8)-k;
			}
		bf1->InsertL(bf1->Size()/3*2,&bb[0],10*j);
		bf2->InsertL(bf2->Size(),&bb[0],10*j);
		bf3->InsertL(0,&bb[0],10*j);
		nbytes+=10*j;
		}
	TInt len=34;
	TInt aLength;
	while (nbytes>len)
		{
		for (TInt pos=0;pos<nbytes;pos+=44)
		{
			if (len>nbytes-pos)
				len=nbytes-pos;
			bf1->Delete(pos,len);
			bf1->Compress();
			CheckSeg(bf1);
			CheckContents1(bf1);
			CheckContents2(bf1);
			aLength=bf2->Ptr(0).Length();
			aLength=((aLength>len)? aLength : len);
			bf2->Delete(aLength-len,len);
			bf2->Compress();
			CheckSeg(bf2);
			CheckContents1(bf2);
			CheckContents2(bf2);
			bf3->Delete(0,len);
			bf3->Compress();
			CheckSeg(bf3);
			CheckContents1(bf3);
			CheckContents2(bf3);
			nbytes-=len;
			test(nbytes==bf1->Size());
			test(nbytes==bf2->Size());
			test(nbytes==bf3->Size());
			}
		}
	delete bf1;
	delete bf2;
	delete bf3;
	test.End();
	}

void TestCBufSeg::Test6L()
	{
	test.Start(_L("Test compress frees empty cells"));
	__UHEAP_MARK;
	TUint8 alphabet[27] = "abcdefghijklmnopqrstuvwxyz";
	CBufSeg* buf = CBufSeg::NewL(10);
	CleanupStack::PushL(buf);
	buf->InsertL(0, alphabet, 16);	// "abcdefghij" ++ "klmnop"
	buf->Delete(5, 5);			// "abcde" ++ "klmnop"
	buf->Delete(10, 1);			// "abcde" ++ "klmno"
	buf->Compress();				// "abcdefklmno". i.e. empty cell should be freed.
	CleanupStack::PopAndDestroy(buf);
	__UHEAP_MARKEND;
	test.End();
	}

LOCAL_C void test_CBufSeg()
//
// Test the BufSeg class
//
	{

	TestCBufSeg b;
	test.Start(_L("All operations"));
	b.Test1();
	test.Next(_L("Inherited Methods"));
	b.Test2();
	test.Next(_L("Insert"));
	b.Test3();
	test.Next(_L("Delete"));
	b.Test4();
	test.Next(_L("Compress"));
	b.Test5();
	test.Next(_L("Bordeline cases"));
	TRAPD(r,b.Test6L());
	test(r==KErrNone);
	//
	test.End();
	}

GLDEF_C TInt E32Main()
//
// Test the ADT segmented varray.
//
	{
	
	test.Title();
	__UHEAP_MARK;
//
// Install a trap handler
//
	CTrapCleanup* trapHandler=CTrapCleanup::New();
	test(trapHandler!=NULL);
//	CleanupStack::NextLevel();
	test.Start(_L("class CBufSeg"));
	test_CBufSeg();
	delete trapHandler;
	__UHEAP_MARKEND;
	test.End();
	return(0);
	}

