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
// e32test\buffer\t_des.cpp
// Overview:
// Test methods of the TDes template class.
// API Information:
// TDes
// Details:
// - Test assigning TPtrs in various ways and confirm results are as expected.
// - Test locating in reverse on an empty TPtr, verify it returns KErrNoFound.
// - For TBuf, TBuf8 and TBuf16 objects: 
// - Construct a default TBuf object and verify Length and MaxLength values are
// as expected.
// - Construct a TBuf object with length and verify Length and MaxLength values 
// are as expected.
// - Construct a TBuf object over a string, verify Length and MaxLength values 
// are as expected. Verify the string data is correct.
// - Construct a TBuf object over a descriptor, verify Length and MaxLength values 
// are as expected. Verify the data is correct.
// - Construct a TBuf object over a buffer, verify Length and MaxLength values 
// are as expected. Verify the data is correct.
// - Construct a TBuf object and assign a string to it, verify Length and MaxLength 
// values are as expected. Verify the data is correct.
// - Construct a TBuf object and assign a descriptor to it, verify Length and MaxLength 
// values are as expected. Verify the data is correct.
// - Construct a TBuf object and assign a buffer to it, verify Length and MaxLength 
// values are as expected. Verify the data is correct.
// - Check the available user heap space and check for memory leaks by comparing it to 
// the initial value.
// - For TPtrC, TPtrC8 and TPtrC16 objects: 
// - Verify the invariant for a null pointer descriptor
// - Construct a TPtrC object over a constant string, verify Length and string data 
// are correct.
// - Construct a TPtrC object over a constant descriptor, verify Length and data are 
// correct.
// - Construct a TPtrC object over a constant buffer with length, verify Length and 
// data are correct.
// - Check the available user heap space and check for memory leaks by comparing it to 
// the initial value.
// - For TPtr, TPtr8 and TPtr16 objects: 
// - Verify the invariant for a null pointer descriptor
// - Construct a TPtr object over a buffer with length and maxlength, verify Length,
// MaxLength and data are correct.
// - Construct a TPtr object over a buffer with maxlength, verify Length, MaxLength 
// and Ptr are correct.
// - Construct a TPtr object and assign a string to it. Verify Length, MaxLength and
// the string data are correct.
// - Construct a TPtr object and assign a buffer to it. Verify Length, MaxLength and
// the data are correct.
// - Construct a TPtr object and assign a descriptor to it. Verify Length, MaxLength
// and the data are correct.
// - Check the available user heap space and check for memory leaks by comparing it to 
// the initial value.
// - Verify the TPtr copy constructor by setting and checking a text string.
// - For TBufC, TBufC8 and TBufC16 objects: 
// - Construct a default TBufC object. Verify default Length is as expected.
// - Construct a TBufC object over a string, verify Length and data are correct.
// - Construct a TBufC object over a TBufC, verify Length and data are correct.
// - Construct a TBufC object and assign a string to it, verify Length and data 
// are correct.
// - Construct a TBufC object and assign a descriptor to it, verify Length and data 
// are correct.
// - Construct a TBufC object and assign a TBufC to it, verify Length and data 
// are correct.
// - Check the available user heap space and check for memory leaks by comparing it to 
// the initial value.
// - For HBufC, HBufC8 and HBufC16 objects: 
// - Create a new descriptor using the New() method, verify non-NULL return value.
// - Construct a HBufC object and assign a string to it, verify Length and data 
// are correct.
// - Construct a HBufC object and assign a descriptor to it, verify Length and data 
// are correct.
// - Construct a HBufC object and assign a HBufC to it, verify Length and data 
// are correct.
// - Construct a HBufC object and call ReAlloc method, verify Length and data are 
// correct.
// - Check the available user heap space and check for memory leaks by comparing it to 
// the initial value.
// - Test ASCII destination with Unicode source. Verify string results are as expected.
// - For TLitC, TLitC8 and TLitC16 objects: 
// - Verify member operators.
// - Assign and validate literal values
// - For TBuf, TBuf8 and TBuf16 objects, test TDes derived objects that return TPtrs.
// - Extract and verify the left portion of the descriptor (LeftTPtr).
// - Extract and verify the right portion of the descriptor (RightTPtr).
// - Extract and verify the middle portion of the descriptor (MidTPtr).
// - For TPtr, TPtr8 and TPtr16 objects, test TDes derived objects that return TPtrs.
// - Extract and verify the left portion of the descriptor (LeftTPtr).
// - Extract and verify the right portion of the descriptor (RightTPtr).
// - Extract and verify the middle portion of the descriptor (MidTPtr).
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("T_DES"));

template<class T,class S, class L, class R>	
class TestTDes
	{
public:
	TestTDes(); 		// Test class constructor.
	void Test1();
	void Test2();
	void Test3();
	void Test4();
	void Test5();
	void Test6();
	void Test7();
	void Test8();
	void Test9();
	void Test10();
	void Test11();
	void Test12();
	void Test13();
	void Test14();
	void Test15();
	void Test16();
	void Test17();
	void Test18();
	void TestA();
	void TestB();
	void TestC();
	void TestD();
	void TestE();
	void TestF();
	void TestG();
	void TestH();
	void TestI();
	void TestJ();
	void TestK();
	void TestL();
	void TestM();
	void TestN();
	void TestO();
	void TestP();
	void test_TPtrC(); 	// Test TPtrC classes
	void test_TPtr();	// Test TPtr classes
	void test_TBuf();	// Test TBuf classes
	void test_TBufC();	// Test TBufC classes
	void test_HBufC();	// Test HBufC classes
	void test_TBufReturningModifiable();
	void test_TPtrReturningModifiable();
	void test_TDesReturningModifiable(const T& a, R& b);
	void testHeapSpace(TInt);
	};

template <class T, class S, class L, class R>
GLDEF_C TestTDes<T,S,L,R>::TestTDes()
//
//	Constructor
//
	{}

template <class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::testHeapSpace(TInt startAvail)
//
//	Test no memory leakage has occured
//
	{

	test.Next(_L("Testing for memory leaks"));
	TInt endAvail,temp;
	endAvail=User::Available(temp);
	test(endAvail==startAvail);
	}

template <class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::Test1()
//
//	Constructs a view over a constant string
//
	{

	T a(_TL("ABc"));
	a.__DbgTestInvariant();
	test(a.Length()==3);
	test(*a.Ptr()=='A');
	test(*(a.Ptr()+1)=='B');
	test(*(a.Ptr()+2)=='c');
	}

template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::Test2()
//
//	Constructs a view over a descriptor
//
	{

	T aDes(_TL("ABc"));
	aDes.__DbgTestInvariant();
	T b(aDes);
	b.__DbgTestInvariant();
	test(b.Length()==aDes.Length());
	test(*b.Ptr()==*aDes.Ptr());
	test(*(b.Ptr()+1)==*(aDes.Ptr()+1));
	test(*(b.Ptr()+2)==*(aDes.Ptr()+2));
	}

template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::Test3()
//
//	Constructs a view over a length specified buffer
//
	{

	S* aBuf = _TL("ABCDEF");
	T aDes(aBuf,6);
	aDes.__DbgTestInvariant();
	test(aDes.Length()==6);
	for(S count=0; count<aDes.Length(); count++)
		test(*(aDes.Ptr()+count)==('A' + count));
	test(aDes.Ptr()==aBuf);
	}

template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::Test4()
//
//	Constructs a r/w view over a buffer with length & maxlength
//
	{

	S* aBuf;
	aBuf=_TL("LMNOPQ");
	T aDesW(aBuf,6,0x0A);
	aDesW.__DbgTestInvariant();
	test(aDesW.Length()==6);
	for(S count=0; count<aDesW.Length(); count++)
		test(*(aDesW.Ptr()+count)==('L' + count));
	test(aDesW.Ptr()==aBuf);
	test(aDesW.MaxLength()==0x0A);
	}

template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::Test5()
//
//	Constructs a r/w view over a buffer with maxlength
//
	{

	S* aBuf;
	aBuf=_TL("ZYXWVU");
	T aDesW(aBuf,0x06);
	aDesW.__DbgTestInvariant();
	test(aDesW.Length()==0);
	test(aDesW.MaxLength()==0x06);
	test(aDesW.Ptr()==aBuf);
	}

template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::Test6()
//
//	Constructs a r/w view over a LcbBase with maxlength
//
	{
	// Cant use yet, see calling function
	}

template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::Test7()
//
//	Constructs a default TBuf view 
//
	{

	T aBuf;
	aBuf.__DbgTestInvariant();
	test(aBuf.Length()==0);
	test(aBuf.MaxLength()==0x40);
	}

template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::Test8()
//
//	Constructs a TBuf view with length
//
	{

	T aBuf(10);
	aBuf.__DbgTestInvariant();
	test(aBuf.Length()==10);
	test(aBuf.MaxLength()==0x40);
	}

template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::Test9()
//
//	Constructs a TBuf view over a string
//
	{

	T aBuf(_TL("ABCDEF"));
	aBuf.__DbgTestInvariant();
	test(aBuf.Length()==6);
	test(aBuf.MaxLength()==0x40);
	for(S count=0;count<aBuf.Length();count++)
		test(*(aBuf.Ptr()+count)==('A' + count));
	}

template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::Test10()
//
//	Constructs a TBuf view over a descriptor
//
	{

	R aDesR(_TL("ABCDEF"));
	aDesR.__DbgTestInvariant();
	T aBuf(aDesR);
	aBuf.__DbgTestInvariant();
	test(aBuf.MaxLength()==0x40);
	test(aBuf.Length()==aDesR.Length());
	test(aBuf.Length()==6);
	for(S count=0;count<aBuf.Length();count++)
		{
		test(*(aBuf.Ptr()+count)==('A' + count));
		test(*(aBuf.Ptr()+count)==*(aDesR.Ptr()+count));
		}
	}

template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::Test11()
//
//	Constructs a TBuf view over a buffer
//
	{

	T aBuf(_TL("ABCDEF"));
	aBuf.__DbgTestInvariant();
	T bBuf(aBuf);
	bBuf.__DbgTestInvariant();
	test(bBuf.MaxLength()==aBuf.MaxLength());
	test(bBuf.MaxLength()==0x40);
	test(bBuf.Length()==aBuf.Length());
	test(bBuf.Length()==6);
	for(S count=0;count<bBuf.Length();count++)
		{
		test(*(bBuf.Ptr()+count)==('A' + count));
		test(*(bBuf.Ptr()+count)==*(aBuf.Ptr()+count));
		}
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::Test12()
//
//	Exercise the default TBufC constructor
//
	{

	T buf;
	buf.__DbgTestInvariant();
	test(buf.Length()==0);
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::Test13()
//
//	Constructor over a string
//
	{

	S* pString=(_TL("12"));
	T buf(pString);
	buf.__DbgTestInvariant();
	test(buf.Length()==2);
	test(*(buf.Ptr())=='1');
	test(*(buf.Ptr()+1)=='2');
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::Test14()
//
//	Constructor over a TDesC (TPtrC)
//
	{

	R des(_TL("Test 14"));
	des.__DbgTestInvariant();
	T buf(des);
	buf.__DbgTestInvariant();
	test(buf.Length()==7);
	test(buf.Length()==des.Length());
	test(*(buf.Ptr())=='T');
	test(*(buf.Ptr()+1)=='e');
	test(*(buf.Ptr()+2)=='s');
	test(*(buf.Ptr()+3)=='t');
	test(*(buf.Ptr()+4)==' ');
	test(*(buf.Ptr()+5)=='1');
	test(*(buf.Ptr()+6)=='4');
	}


template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::Test15()
//
//	Constructor over a TBufC
//
	{

	T oldBuf(_TL("Test  15"));
	oldBuf.__DbgTestInvariant();
	T newBuf(oldBuf);
	newBuf.__DbgTestInvariant();
	test(newBuf.Length()==8);
	test(newBuf.Length()==oldBuf.Length());
	test(*(newBuf.Ptr())=='T');
	test(*(newBuf.Ptr()+1)=='e');
	test(*(newBuf.Ptr()+2)=='s');
	test(*(newBuf.Ptr()+3)=='t');
	test(*(newBuf.Ptr()+4)==' ');
	test(*(newBuf.Ptr()+5)==' ');
	test(*(newBuf.Ptr()+6)=='1');
	test(*(newBuf.Ptr()+7)=='5');
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::Test16()
//
//	New operator - without leave
//
	{

	T* pBuf=T::New(0x40);
	test(pBuf!=NULL);
	pBuf->__DbgTestInvariant();
	User::Free(pBuf);
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::Test17()
//
//	NewL operator - forcing a leave
//
	{

/*

    Reinstate if some method is found of forcing alloc fail
    This test no longer works due to heap growing functionality of E32 057

	T* pBuf;
	TInt temp;
	TInt available=User::Available(temp);
	TAny* pDummy=User::Alloc(available);
	TRAPD(ret,pBuf=T::NewL(0x40))
	test(ret==KErrNoMemory);
	User::Free(pDummy);
*/
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::Test18()
//
//	NewL operator - forcing success
//
	{

	T* pBuf=NULL;
	TRAPD(ret,pBuf=T::NewL(0x40))
	test(ret==0);
	test(pBuf!=NULL);
	pBuf->__DbgTestInvariant();
	User::Free(pBuf);
	}

template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::TestA()
//
//	String Assignment
//
	{

    S myBuf[7];
    Mem::Copy(myBuf,_TL("ABCDEF"),7*sizeof(S));
	T aDes(myBuf,0x06,0x06);
	aDes=_TL("ZYXWVU");
	aDes.__DbgTestInvariant();
	test(aDes.Length()==6);
	test(aDes.MaxLength()==0x06);
	S count=0;
	for(;count<aDes.Length(); count++)
		{
		test(*(aDes.Ptr()+count)==('Z'-count));
		}	
	aDes=_TL("123");
	aDes.__DbgTestInvariant();
	test(aDes.Length()==3);
	test(aDes.MaxLength()==0x06);
	for(count=0; count<aDes.Length(); count++)
		{
		test(*(aDes.Ptr()+count)==('1'+count));
		}	
	}


template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::TestB()
//	DesW Assignment
	{
	S aBuf[11];
	S bBuf[11];
	
    Mem::Copy(aBuf,_TL("ZYXWVUTSRQ"),11*sizeof(S));
    Mem::Copy(bBuf,_TL("ABCDEFGHIJ"),11*sizeof(S));
	T aDes(aBuf,0x0A,0x0A);	
	T bDes(bBuf,0x0A);
	bDes=aDes;
	aDes.__DbgTestInvariant();
 	bDes.__DbgTestInvariant();
	test(aDes.Length()==bDes.Length());
	test(aDes.MaxLength()==0x0A);
	test(bDes.MaxLength()==0x0A);		
	for(S count=0; count<bDes.Length(); count++)
		{
		test(*(aDes.Ptr()+count)==*(bDes.Ptr()+count));
		}
	}



template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::TestC()
//	DesC Assignment
	{
	S aBuf[11];
	R aDesR(_TL("1234567890"));	
    Mem::Copy(aBuf,_TL("ABCDEFGHIJ"),11*sizeof(S));
	T aDes(aBuf,0x0A,0x0A);
	aDes=aDesR;
	aDes.__DbgTestInvariant();
	test(aDes.Length()==aDesR.Length());
	test(aDes.MaxLength()==0x0A);
	for(S count=0; count<aDes.Length(); count++)
		{
		test(*(aDes.Ptr()+count)==*(aDesR.Ptr()+count));
		}
	}


template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::TestD()
//	TBuf string Assignment
	{
	T aBuf;
	aBuf=(_TL("ABCDEF"));
	aBuf.__DbgTestInvariant();
	test(aBuf.Length()==6);
	test(aBuf.MaxLength()==0x40);
	for(S count=0; count<aBuf.Length(); count++)
		{
		test(*(aBuf.Ptr()+count)==('A'+count));
		}
	}


template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::TestE()
//	TBuf descriptor Assignment
	{
	R aDesR(_TL("ABCDEF"));
	T aBuf;
	aBuf=aDesR;
	aBuf.__DbgTestInvariant();
	test(aBuf.MaxLength()==0x40);
	test(aBuf.Length()==aDesR.Length());
	test(aBuf.Length()==6);
	for(S count=0;count<aBuf.Length();count++)
		{
		test(*(aBuf.Ptr()+count)==('A' + count));
		test(*(aBuf.Ptr()+count)==*(aDesR.Ptr()+count));
		}
	}


template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::TestF()
//	TBuf buffer Assignment
	{
	T aBuf(_TL("ABCDEF"));
	T bBuf;
	bBuf=aBuf;
	bBuf.__DbgTestInvariant();
	test(bBuf.MaxLength()==aBuf.MaxLength());
	test(bBuf.MaxLength()==0x40);
	test(bBuf.Length()==aBuf.Length());
	test(bBuf.Length()==6);
	for(S count=0;count<bBuf.Length();count++)
		{
		test(*(bBuf.Ptr()+count)==('A' + count));
		test(*(bBuf.Ptr()+count)==*(aBuf.Ptr()+count));
		}
 	}
	 

template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::TestG()
//	TBufC string assignment test
	{
	S* pString=_TL("TestG");
	T buf;

	test(buf.Length()==0);
	buf=pString;
	buf.__DbgTestInvariant();
	test(buf.Length()==5);
	test(*(buf.Ptr())=='T');
	test(*(buf.Ptr()+1)=='e');
	test(*(buf.Ptr()+2)=='s');
	test(*(buf.Ptr()+3)=='t');
	test(*(buf.Ptr()+4)=='G');
	}


template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::TestH()
//	TBufC descriptor assignment
	{
	R des(_TL("TestH"));
	T buf;

	test(buf.Length()==0);
	buf=des;
	buf.__DbgTestInvariant();
	test(buf.Length()==5);
	test(buf.Length()==des.Length());
	test(*(buf.Ptr())=='T');
	test(*(buf.Ptr()+1)=='e');
	test(*(buf.Ptr()+2)=='s');
	test(*(buf.Ptr()+3)=='t');
	test(*(buf.Ptr()+4)=='H');
	}

template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::TestI()
//	TBufC TBufC assignment
	{
	T oldBuf(_TL("TEsti"));
	T newBuf;

	test(newBuf.Length()==0);
	newBuf=oldBuf;
	newBuf.__DbgTestInvariant();
	test(newBuf.Length()==5);
	test(newBuf.Length()==oldBuf.Length());
	test(*(newBuf.Ptr())=='T');
	test(*(newBuf.Ptr()+1)=='E');
	test(*(newBuf.Ptr()+2)=='s');
	test(*(newBuf.Ptr()+3)=='t');
	test(*(newBuf.Ptr()+4)=='i');
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::TestJ()
//
//	HBufC string assignment
//
	{

	S* pString=_TL("tEStJ");
	T* pBuf=NULL;
	TRAPD(ret,pBuf=T::NewL(0x40))
	test(ret==0);
	test(pBuf!=NULL);
	*pBuf=pString;
	pBuf->__DbgTestInvariant();
	test(pBuf->Length()==5);
	test(*(pBuf->Ptr())=='t');
	test(*(pBuf->Ptr()+1)=='E');	
	test(*(pBuf->Ptr()+2)=='S');	
	test(*(pBuf->Ptr()+3)=='t');	
	test(*(pBuf->Ptr()+4)=='J');	
	User::Free(pBuf);
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::TestK()
//
//	HBufC descriptor assignment
//
	{

	R des(_TL("TestK"));
	T* pBuf=NULL;
	TRAPD(ret,pBuf=T::NewL(0x40))
	test(ret==0);
	test(pBuf!=NULL);
	*pBuf=des;
	pBuf->__DbgTestInvariant();
	test(pBuf->Length()==5);
	test(pBuf->Length()==des.Length());
	test(*(pBuf->Ptr())=='T');
	test(*(pBuf->Ptr()+1)=='e');	
	test(*(pBuf->Ptr()+2)=='s');	
	test(*(pBuf->Ptr()+3)=='t');	
	test(*(pBuf->Ptr()+4)=='K');	
	User::Free(pBuf);
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::TestL()
//
//	HBufC HBufC assignment
//
	{

	S* pString=_TL("testl");
	T* pOldBuf=NULL;
	T* pNewBuf=NULL;
	TRAPD(ret,pOldBuf=T::NewL(0x40))
	test(ret==0);
	TRAP(ret,pNewBuf=T::NewL(0x40))
	test(ret==0);
	test(pNewBuf!=NULL);
	test(pOldBuf!=NULL);
	*pOldBuf=pString;
	*pNewBuf=*pOldBuf;
	pOldBuf->__DbgTestInvariant();
	pNewBuf->__DbgTestInvariant();
	test(pNewBuf->Length()==5);
	test(pOldBuf->Length()==pNewBuf->Length());
	test(*(pNewBuf->Ptr())=='t');
	test(*(pNewBuf->Ptr()+1)=='e');	
	test(*(pNewBuf->Ptr()+2)=='s');	
	test(*(pNewBuf->Ptr()+3)=='t');	
	test(*(pNewBuf->Ptr()+4)=='l');	
	User::Free(pOldBuf);
	User::Free(pNewBuf);
//
	TRAP(ret,pOldBuf=T::NewMaxL(0x40))
	test(ret==0);
	TRAP(ret,pNewBuf=T::NewMaxL(0x40))
	test(ret==0);
	test(pNewBuf!=NULL);
	test(pOldBuf!=NULL);
	pOldBuf->__DbgTestInvariant();
	pNewBuf->__DbgTestInvariant();
	test(pNewBuf->Length()==0x40);
	test(pOldBuf->Length()==0x40);
	pNewBuf->Des().Zero();
	pOldBuf->Des().Zero();
	pOldBuf->__DbgTestInvariant();
	pNewBuf->__DbgTestInvariant();
	test(pNewBuf->Length()==0);
	test(pOldBuf->Length()==0);
	*pOldBuf=pString;
	*pNewBuf=*pOldBuf;
	pOldBuf->__DbgTestInvariant();
	pNewBuf->__DbgTestInvariant();
	test(pNewBuf->Length()==5);
	test(pOldBuf->Length()==pNewBuf->Length());
	test(*(pNewBuf->Ptr())=='t');
	test(*(pNewBuf->Ptr()+1)=='e');	
	test(*(pNewBuf->Ptr()+2)=='s');	
	test(*(pNewBuf->Ptr()+3)=='t');	
	test(*(pNewBuf->Ptr()+4)=='l');	
	User::Free(pOldBuf);
	User::Free(pNewBuf);
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::TestM()
//
//	HBufC ReAlloc
//
	{

	S* pString=_TL("TESTM");
	T* pBuf;
	pBuf=T::NewL(0x40);
	*pBuf=pString;
	pBuf->ReAlloc(0x40);
	test(pBuf!=NULL);
	pBuf->__DbgTestInvariant();
	test(pBuf->Length()==5);
	test(*(pBuf->Ptr())=='T');
	test(*(pBuf->Ptr()+1)=='E');	
	test(*(pBuf->Ptr()+2)=='S');	
	test(*(pBuf->Ptr()+3)=='T');	
	test(*(pBuf->Ptr()+4)=='M');	
	User::Free(pBuf);
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::TestN()
//
//	HBufC ReAllocL - forcing a leave
//
	{

/*

    Reinstate if some method is found of forcing alloc fail
    This test no longer works due to heap growing functionality of E32 057

	S* pString=_TL("tEStJ");
	T* pBuf;
	TInt available, temp;
	TAny* pDummy;
	pBuf=T::NewL(0x40);
	test(pBuf!=NULL);
	*pBuf=pString;
	available=User::Available(temp);
	pDummy=User::Alloc(available);
	TRAPD(ret,pBuf->ReAllocL(0x50))
	test(ret==KErrNoMemory);
	User::Free(pDummy);
	User::Free(pBuf);
*/
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::TestO()
//	HBufC ReAllocL - forcing success
	{

	S* pString=_TL("tEStJ");
	T* pBuf;
	pBuf=T::NewL(0x40);
	test(pBuf!=0);
	*pBuf=pString;
	TRAPD(ret,pBuf->ReAllocL(0x40))
	test(ret==0);
	test(pBuf!=NULL);
	pBuf->__DbgTestInvariant();
	test(pBuf->Length()==5);
	test(*(pBuf->Ptr())=='t');
	test(*(pBuf->Ptr()+1)=='E');	
	test(*(pBuf->Ptr()+2)=='S');	
	test(*(pBuf->Ptr()+3)=='t');	
	test(*(pBuf->Ptr()+4)=='J');	
	User::Free(pBuf);
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::TestP()
//	Invariant for null pointer descriptor
//	Added to reproduce DEF023302
	{
	T p(NULL, 0);
	p.__DbgTestInvariant();
	}

GLDEF_C void TestCopy16()
//	Test TPtr16 copy constructor
	{
    TBuf16<0x20> buf(_S16("Paddington Station"));
	TPtr16 ptr1((TText16*)buf.Ptr(),buf.Length(),buf.MaxLength());
	ptr1.__DbgTestInvariant();
	TPtr16 ptr2(ptr1);
	ptr2.__DbgTestInvariant();
	ptr2[11]='B';
	ptr2[12]='e';
	ptr2[13]='a';
	ptr2[14]='r';
	ptr2[15]=' ';
	ptr1.SetLength(15);
	ptr2.Append(_L16(" acid"));
	test(ptr1==_L16("Paddington Bear"));
	test(ptr2==_L16("Paddington Bear on acid"));
	}

GLDEF_C void Test16To8()
//	Test ASCII dest with Unicode source
	{
    TBuf8<0x20> buf(_S8("Paddington Station"));
	TPtr8 ptr1((TText8*)buf.Ptr(),buf.Length(),buf.MaxLength());
	ptr1.__DbgTestInvariant();
	TPtr8 ptr2(ptr1);
	ptr2.__DbgTestInvariant();
	ptr2[11]='B';
	ptr2[12]='e';
	ptr2[13]='a';
	ptr2[14]='r';
	ptr2[15]=' ';
	ptr1.SetLength(15);
	ptr1.__DbgTestInvariant();
	ptr2.Append(_L16(" acid"));
	ptr2.__DbgTestInvariant();
	test(ptr1==_L8("Paddington Bear"));
	test(ptr2==_L8("Paddington Bear on acid"));
	}


GLDEF_C void TestCopy8()
//	Test TPtr8 copy constructor
	{
    TBuf8<0x20> buf(_S8("Paddington Station"));
	TPtr8 ptr1((TText8*)buf.Ptr(),buf.Length(),buf.MaxLength());
	TPtr8 ptr2(ptr1);
	ptr2[11]='B';
	ptr2[12]='e';
	ptr2[13]='a';
	ptr2[14]='r';
	ptr2[15]=' ';
	ptr1.SetLength(15);
	ptr2.Append(_L8(" acid"));
	ptr1.__DbgTestInvariant();
	ptr2.__DbgTestInvariant();
	test(ptr1==_L8("Paddington Bear"));
	test(ptr2==_L8("Paddington Bear on acid"));
	}

GLDEF_C void TestCopy()
//	Test TPtr copy constructor
	{
    TBuf<0x20> buf(_S("Paddington Station"));
	TPtr ptr1((TText*)buf.Ptr(),buf.Length(),buf.MaxLength());
	TPtr ptr2(ptr1);
	ptr2[11]='B';
	ptr2[12]='e';
	ptr2[13]='a';
	ptr2[14]='r';
	ptr2[15]=' ';
	ptr1.SetLength(15);
	ptr2.Append(_L(" acid"));
	ptr1.__DbgTestInvariant();
	ptr2.__DbgTestInvariant();
	test(ptr1==_L("Paddington Bear"));
	test(ptr2==_L("Paddington Bear on acid"));
	}

template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::test_TPtrC()
//
// Test the TDesC class special member functions
//
	{

 	test.Start(_L("Invariant for null pointer descriptor"));
 	TestP();
	test.Next(_L("R/O view over constant string"));
	Test1();
	test.Next(_L("R/O view over constant descriptor"));
	Test2();
	test.Next(_L("R/O view over constant buffer with length"));
	Test3();
	}

template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::test_TPtr()
//
//	Test the TPtr class special member functions
//
	{

 	test.Start(_L("Invariant for null pointer descriptor"));
 	TestP();
	test.Next(_L("R/W view over buffer with length & maxlength"));
	Test4();
	test.Next(_L("R/W view over buffer with maxlength"));
	Test5();
//	Skipped the following test because Colly hasn't
//	written the templates yet for the TBufC8 & 16 classes
//	test.Next(_L("R/W view over LcbBase with maxlength"));
//	Test6();
	test.Next(_L("String assignment"));
	TestA();
	test.Next(_L("DesW assignment"));
	TestB();
	test.Next(_L("DesC assignment"));
	TestC();
	}

template<class T, class S, class L, class R>
GLDEF_C void TestTDes<T,S,L,R>::test_TBuf()
//
// Test the TBuf class special member functions
//
	{

	test.Start(_L("TBuf default view, no params"));
	Test7();
	test.Next(_L("TBuf view with a length"));
	Test8();
	test.Next(_L("TBuf view over a string"));
	Test9();
	test.Next(_L("TBuf view over a descriptor"));
	Test10();
	test.Next(_L("TBuf view over a buffer"));
	Test11();
	test.Next(_L("String assignment"));
	TestD();
	test.Next(_L("Descriptor assignment"));
	TestE();
	test.Next(_L("Buffer assignment"));
	TestF();
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::test_TBufC()
//
// Test the TBufC class methods
//
	{

	test.Start(_L("Default constructor"));
	Test12();
	test.Next(_L("With a string"));
	Test13();
	test.Next(_L("With a Descriptor (TPtrC)"));
	Test14();
	test.Next(_L("With a TBufC"));
	Test15();
	test.Next(_L("String assignment"));
	TestG();
	test.Next(_L("Descriptor assignment (TPtrC)"));
	TestH();
	test.Next(_L("TBufC assignment"));
	TestI();
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::test_HBufC()
//
//	Test the HBufC class methods.
//
	{

	test.Start(_L("New"));
	Test16();
	test.Next(_L("NewL - forcing a leave"));
	Test17();
	test.Next(_L("NewL - showing success"));
	Test18();
	test.Next(_L("String assignment"));
	TestJ();
	test.Next(_L("Descriptor assignment"));
	TestK();
	test.Next(_L("HBufC assignment"));
	TestL();
	test.Next(_L("ReAlloc"));
	TestM();
	test.Next(_L("ReAllocL - forcing a leave"));
	TestN();
	test.Next(_L("ReAllocL - showing success"));
	TestO();
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::test_TBufReturningModifiable()
	{
	T a;
	R b(0,0);
	a.Copy(_TL("abcdefg"));
	test_TDesReturningModifiable(a,b);
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::test_TPtrReturningModifiable()
	{
	S* aBuf;
	aBuf=_TL("abcdefg");
	T a(aBuf,7,7);
	R b(0,0);
	test_TDesReturningModifiable(a,b);
	}

template<class T,class S,class L,class R>
GLDEF_C void TestTDes<T,S,L,R>::test_TDesReturningModifiable(const T& a, R& b)
	{
	test.Start(_L("Test TDes derived objects that return TPtrs"));

	test.Next(_L("LeftTPtr"));
	b.Set(a.LeftTPtr(3));
	b.__DbgTestInvariant();
	test(b == a.Left(3));

	b.Set(a.LeftTPtr(0));
	b.__DbgTestInvariant();
	test(b == a.Left(0));

	b.Set(a.LeftTPtr(7));
	b.__DbgTestInvariant();
	test(b == a.Left(7));

	b = a.LeftTPtr(42);
	b.__DbgTestInvariant();
	test(b == a.Left(7));

	test.Next(_L("RightTPtr"));
	b.Set(a.RightTPtr(5));
	b.__DbgTestInvariant();
	test(b == a.Right(5));

	b.Set(a.RightTPtr(7));
	b.__DbgTestInvariant();
	test(b == a.Right(7));

	b.Set(a.RightTPtr(0));
	b.__DbgTestInvariant();
	test(b == a.Right(0));

	b.Set(a.RightTPtr(42));
	b.__DbgTestInvariant();
	test(b == a.Right(42));

	test.Next(_L("MidTPtr"));
	b.Set(a.MidTPtr(5));
	b.__DbgTestInvariant();
	test(b == a.Mid(5));

	b.Set(a.MidTPtr(7));
	b.__DbgTestInvariant();
	test(b == a.Mid(7));

	b.Set(a.MidTPtr(0));
	b.__DbgTestInvariant();
	test(b == a.Mid(0));

	test.Next(_L("MidTPtr (2 parms)"));
	b.Set(a.MidTPtr(4,3));
	b.__DbgTestInvariant();
	test(b == a.Mid(4,3));
	test(b == a.Mid(4));

	b.Set(a.MidTPtr(0,0));
	b.__DbgTestInvariant();
	test(b == a.Mid(0,0));

	b.Set(a.MidTPtr(3,2));
	b.__DbgTestInvariant();
	test(b == a.Mid(3,2));

	b.Set(a.MidTPtr(7,0));
	b.__DbgTestInvariant();
	test(b == a.Mid(7,0));
	}

LOCAL_C void testAssignTPtr()
//
// Test assigning TPtr's.
//
	{

	TBufC<0x20> x(_S("Hello"));
	TPtr p=x.Des();
	p+=_L(" World");
	test(p==_L("Hello World"));
	test(x==_L("Hello World"));
	p.SetLength(5);
	test(p==_L("Hello"));
	test(x==_L("Hello"));
	TBufC<0x20> y(_S("ByeBye"));
	TPtr q=y.Des();
	q.Set(p);
	test(q==p);
	q+=_L(" World");
	test(q==_L("Hello World"));
	test(x==_L("Hello World"));
	test(y==_L("ByeBye"));
	q.Set((TText*)x.Ptr(),x.Length(),x.Des().MaxLength());
	test(q==_L("Hello World"));
	test(q.MaxLength()==0x20);
	q.SetLength(1);
	test(q==_L("H"));
	test(x==_L("Hello World"));
	}

LOCAL_C void testTPtrLocateReverse()
//
// Test locating in reverse on an empty TPtr
//
	{

	TPtrC a;
	test(a.LocateReverse('0')==KErrNotFound);
	test(a.LocateReverseF('0')==KErrNotFound);
	}

_LIT8(KLitTest1_8,"1");
_LIT8(KLitTest12_8,"12");
_LIT8(KLitTest123_8,"123");
_LIT8(KLitTest1234_8,"1234");
_LIT8(KLitTestalpha_8,"abcdefghijklmnopqrstuvwxyz");

LOCAL_C const TDesC8& testByRef8(TRefByValue<const TDesC8> aRef)
	{
	return aRef;
	}

LOCAL_C void testTLitC8()
	{
	test.Start(_L("All members"));
	test (_L8("")==KNullDesC8);			// operator const TDesC8&
	test (KNullDesC8()==TPtrC8());		// operator()
	const TDesC8* pNull=&KNullDesC8;	// operator&
	test (pNull->Compare(_L8(""))==0);
	test (&testByRef8(KNullDesC8)==&KNullDesC8);	// operator const TRefByValue<const TDesC8>
//
	test.Next(_L("Literal values"));
	const TDesC8& t1=KLitTest1_8;
	test (t1.Length()==1);
	test (t1[0]=='1');
	test (t1==_L8("1"));
	const TDesC8& t12=KLitTest12_8;
	test (t12.Length()==2);
	test (t12[0]=='1');
	test (t12==_L8("12"));
	const TDesC8& t123=KLitTest123_8;
	test (t123.Length()==3);
	test (t123[0]=='1');
	test (t123==_L8("123"));
	const TDesC8& t1234=KLitTest1234_8;
	test (t1234.Length()==4);
	test (t1234[0]=='1');
	test (t1234==_L8("1234"));
	const TDesC8& talpha=KLitTestalpha_8;
	test (talpha.Length()==26);
	test (talpha[0]=='a');
	test (talpha==_L8("abcdefghijklmnopqrstuvwxyz"));
//
	test.End();
	}

_LIT16(KLitTest1_16,"1");
_LIT16(KLitTest12_16,"12");
_LIT16(KLitTest123_16,"123");
_LIT16(KLitTest1234_16,"1234");
_LIT16(KLitTestalpha_16,"abcdefghijklmnopqrstuvwxyz");

LOCAL_C const TDesC16& testByRef16(TRefByValue<const TDesC16> aRef)
	{
	return aRef;
	}

LOCAL_C void testTLitC16()
	{
	test.Start(_L("All members"));
	test (_L16("")==KNullDesC16);			// operator const TDesC16&
	test (KNullDesC16()==TPtrC16());		// operator()
	const TDesC16* pNull=&KNullDesC16;	// operator&
	test (pNull->Compare(_L16(""))==0);
	test (&testByRef16(KNullDesC16)==&KNullDesC16);	// operator const TRefByValue<const TDesC16>
//
	test.Next(_L("Literal values"));
	const TDesC16& t1=KLitTest1_16;
	test (t1.Length()==1);
	test (t1[0]=='1');
	test (t1==_L16("1"));
	const TDesC16& t12=KLitTest12_16;
	test (t12.Length()==2);
	test (t12[0]=='1');
	test (t12==_L16("12"));
	const TDesC16& t123=KLitTest123_16;
	test (t123.Length()==3);
	test (t123[0]=='1');
	test (t123==_L16("123"));
	const TDesC16& t1234=KLitTest1234_16;
	test (t1234.Length()==4);
	test (t1234[0]=='1');
	test (t1234==_L16("1234"));
	const TDesC16& talpha=KLitTestalpha_16;
	test (talpha.Length()==26);
	test (talpha[0]=='a');
	test (talpha==_L16("abcdefghijklmnopqrstuvwxyz"));
//
	test.End();
	}

_LIT(KLitTest1,"1");
_LIT(KLitTest12,"12");
_LIT(KLitTest123,"123");
_LIT(KLitTest1234,"1234");
_LIT(KLitTestalpha,"abcdefghijklmnopqrstuvwxyz");

LOCAL_C const TDesC& testByRef(TRefByValue<const TDesC> aRef)
	{
	return aRef;
	}

LOCAL_C void testTLitC()
	{
	test.Start(_L("All members"));
	test (_L("")==KNullDesC);			// operator const TDesC&
	test (KNullDesC()==TPtrC());		// operator()
	const TDesC* pNull=&KNullDesC;	// operator&
	test (pNull->Compare(_L(""))==0);
	test (&testByRef(KNullDesC)==&KNullDesC);	// operator const TRefByValue<const TDesC>
#if defined(_UNICODE)
	test (_L16("")==KNullDesC);			// operator const TDesC&
	test (KNullDesC()==TPtrC16());		// operator()
	const TDesC16* pNull16=&KNullDesC;	// operator&
	test (pNull16->Compare(_L16(""))==0);
	test (&testByRef16(KNullDesC)==&KNullDesC);	// operator const TRefByValue<const TDesC>
#else
	test (_L8("")==KNullDesC);			// operator const TDesC&
	test (KNullDesC()==TPtrC8());		// operator()
	const TDesC8* pNull8=&KNullDesC;	// operator&
	test (pNull8->Compare(_L8(""))==0);
	test (&testByRef8(KNullDesC)==&KNullDesC);	// operator const TRefByValue<const TDesC>
#endif
//
	test.Next(_L("Literal values"));
	const TDesC& t1=KLitTest1;
	test (t1.Length()==1);
	test (t1[0]=='1');
	test (t1==_L("1"));
	const TDesC& t12=KLitTest12;
	test (t12.Length()==2);
	test (t12[0]=='1');
	test (t12==_L("12"));
	const TDesC& t123=KLitTest123;
	test (t123.Length()==3);
	test (t123[0]=='1');
	test (t123==_L("123"));
	const TDesC& t1234=KLitTest1234;
	test (t1234.Length()==4);
	test (t1234[0]=='1');
	test (t1234==_L("1234"));
	const TDesC& talpha=KLitTestalpha;
	test (talpha.Length()==26);
	test (talpha[0]=='a');
	test (talpha==_L("abcdefghijklmnopqrstuvwxyz"));
//
	test.End();
	}

LOCAL_C void testLiteral()
	{
	test.Next(_L("class TLitC8"));
	testTLitC8();
	test.Next(_L("class TLitC16"));
	testTLitC16();
	test.Next(_L("class TLitC"));
	testTLitC();
	}

/**
@SYMTestCaseID          TI18N-TDESC-CIT-4012
@SYMTestCaseDesc        Check surrogate function  
@SYMTestPriority        High
@SYMTestActions         Test Match, Locate,  FindCorruptSurrogate, Match2
@SYMTestExpectedResults It works as function specifies
@SYMREQ                 REQ12064, REQ12065
*/

LOCAL_C void testSurrogateAwareInterfaces()
	{
	test.Next(_L("new TDesC interfaces"));
	TInt count;
	
	// string 1: all BMP characters
	_LIT(KBmpString1,			"abcdcf");
	TBuf16<128> s01(KBmpString1);
	test(s01.FindCorruptSurrogate() == KErrNotFound);
	test(s01.Locate2('f') == 5);
	test(s01.LocateReverse2('c') == 4);
	test(s01.Match(_L("*cdc*")) == 2);
	test(s01.Match2(_L("*bcdc*")) == 1);
	test(s01.Match2(_L("*c?c*")) == 2);
	
	// string 2: all non-BMP characters
	_LIT(KSurrogateString1,		"\xD840\xDDAA\xD840\xDDAB\xD840\xDDAC\xD840\xDDAD\xD840\xDDAE\xD840\xDDAF");
	TBuf16<128> s02(KSurrogateString1);
	for (count=0; count<=11; count++)
	test(s02.FindCorruptSurrogate() == KErrNotFound);
	test(s02.Locate2(0x201AE) == 8);
	test(s02.LocateReverse2(0x201AC) == 4);
	test(s02.Match2(_L("*\xD840\xDDAB\xD840\xDDAC*")) == 2);
	test(s02.Match2(_L("*\xD840\xDDBB*")) == KErrNotFound);
	test(s02.Match2(_L("*\xD840\xDDAD*")) == 6);

	// string 3: mixed
	_LIT(KMixedString1,			"ab\xD840\xDDAD e\xD801\xDC27");
	TBuf16<128> s03(KMixedString1);
	test(s03.FindCorruptSurrogate() == KErrNotFound);
	test(s03.Locate2(0x10427) == 6);
	test(s03.Locate2('e') == 5);
	test(s03.LocateF2(0x1044F) == 6);	// lower case=U+1044F(D801, DC4F), upper case=U+10427(D801, DC27), title case=U+10427
	TBuf16<128> s03a;
	s03a.CopyLC2(s03);
	s03a.Append2(0x21000);
	TBuf16<128> s03b;
	s03b.Copy(s03);
	
	s03b.AppendFill2(0x21000, 2);
	test(s03a != s03b);
	test(s03.Match2(_L("*b\xD840\xDDAD*")) == 1);
	test(s03.Match2(_L("* e*")) == 4);
	test(s03.Match2(_L("*\xD840\xDDAD?*")) == 2);
	
	// string 4: mixed, with corrupt surrogate
	_LIT(KCorruptString1,		"ab\xD840\xDDAD e\xDDAD\xD840");
	TBuf16<128> s04(KCorruptString1);
	test(s04.FindCorruptSurrogate() == 6);
	
	// string 5: fill
	_LIT(KOddString5,           "0123456");
	TBuf16<128> s05(KOddString5);
	s05.Fill2(0x21000);
    test(s05 == _L("\xD844\xDC00\xD844\xDC00\xD844\xDC00\xD844"));
    s05.Fill2(' ');
    test(s05 == _L("       "));
	s05.AppendFill2(0x22222, 2);
	s05.AppendFill2(0x22222, 3);
	test(s05 == _L("       \xD848\xDE22\xD848\xDE22\xD848"));
	
	// string 6: locate
	// from Unicode 5.0, CaseFolding.txt
	// 10400; C; 10428; # DESERET CAPITAL LETTER LONG I
	// 0x10400's fold is 0x10428
	TCharF f06(0x10400);
	test(f06 == 0x10428);  // just to ensure the property is correct
	// 0x10400: D801, DC00
	// 0x10428: D801, DC28
	_LIT(KMixedString6,         "ab\xD801\xDC00 e\xD801\xDC27");
	TBuf16<128> s06(KMixedString6);
	TInt pos6 = s06.LocateReverseF2(0x10428);
	test(pos6 == 2);
	}


#ifndef _DEBUG
#pragma warning (disable: 4702)// Unreachable code
#pragma warning (disable: 4701)// Local variable used without being initialized
#pragma warning (disable: 4710)// Function not expanded
#endif

GLDEF_C TInt E32Main()
//
// Test the TDes type.
//
    {
	test.Title();
//
	TInt startAvail,temp;
	startAvail=User::Available(temp);
//
	test.Start(_L("Assigning TPtr's"));
	testAssignTPtr();
//
	test.Next(_L("Locate reverse on empty TPtr"));
	testTPtrLocateReverse();
//
	test.Next(_L("class TBuf<0x40>"));
	TestTDes<TBuf<0x40>,TText,TUint, TPtrC> jj;
	jj.test_TBuf();
	jj.testHeapSpace(startAvail);
	test.End();
//
	test.Next(_L("class TBuf8<0x40>"));
	TestTDes<TBuf8<0x40>,TUint8,TUint8, TPtrC8> ii;
	ii.test_TBuf();
	ii.testHeapSpace(startAvail);
	test.End();
//
	test.Next(_L("class TBuf16<0x40>"));
	TestTDes<TBuf16<0x40>,TUint16,TUint16, TPtrC16> hh;
	hh.test_TBuf();
	hh.testHeapSpace(startAvail);
	test.End();	
//
	test.Next(_L("class TPtrC"));
	TestTDes<TPtrC,TText,TUint, TPtrC> dd;
	dd.test_TPtrC();
	dd.testHeapSpace(startAvail);
	test.End();
//
	test.Next(_L("class TPtrC8"));
	TestTDes<TPtrC8,TUint8,TUint8, TPtrC8> cc;
	cc.test_TPtrC();
	cc.testHeapSpace(startAvail);
	test.End();
//
	test.Next(_L("class TPtrC16"));
	TestTDes<TPtrC16,TUint16,TUint16, TPtrC16> bb;
	bb.test_TPtrC();
	bb.testHeapSpace(startAvail);
	test.End();
//
	test.Next(_L("class TPtr"));
	TestTDes<TPtr,TText,TUint, TPtrC> gg;
	gg.test_TPtr();
	gg.testHeapSpace(startAvail);
	test.Next(_L("Copy constructor"));
	TestCopy();
	test.End();	
//
	test.Next(_L("class TPtr8"));
	TestTDes<TPtr8,TUint8,TUint8, TPtrC8> ff;
	ff.test_TPtr();
	ff.testHeapSpace(startAvail);
	test.Next(_L("Copy constructor"));
	TestCopy8();
	test.End();	
//
	test.Next(_L("class TPtr16"));
	TestTDes<TPtr16,TUint16,TUint16, TPtrC16> ee;
	ee.test_TPtr();
	ee.testHeapSpace(startAvail);
	test.Next(_L("Copy constructor"));
	TestCopy16();
	test.End();	
//
	test.Next(_L("class TBufC"));
	TestTDes<TBufC<0x40>, TText, TUint16, TPtrC> kk;
	kk.test_TBufC();
	kk.testHeapSpace(startAvail);
	test.End();
//
	test.Next(_L("class TBufC8"));
	TestTDes<TBufC8<0x40>, TUint8, TUint8, TPtrC8> ll;
	ll.test_TBufC();
	ll.testHeapSpace(startAvail);
	test.End();
//
	test.Next(_L("class TBufC16"));
	TestTDes<TBufC16<0x40>, TUint16, TUint16, TPtrC16> mm;
	mm.test_TBufC();
	mm.testHeapSpace(startAvail);
	test.End();
//
	test.Next(_L("class HBufC"));
	TestTDes<HBufC, TText, TUint, TPtrC>  nn;
	nn.test_HBufC();
	nn.testHeapSpace(startAvail);
	test.End();
//
	test.Next(_L("class HBufC8"));
	TestTDes<HBufC8, TUint8, TUint8, TPtrC8>  oo;
	oo.test_HBufC();
	oo.testHeapSpace(startAvail);
	test.End();

	test.Next(_L("Test Unicode to Ascii stuff"));
	Test16To8();
//
	test.Next(_L("class HBufC16"));
	TestTDes<HBufC16, TUint16, TUint16, TPtrC16>  pp;
	pp.test_HBufC();
	pp.testHeapSpace(startAvail);
	test.End();
//
	testLiteral();
//
	test.Next(_L("class TBuf returning TPtr"));
	TestTDes<TBuf<0x40>, TText, TUint, TPtr> qq;
	qq.test_TBufReturningModifiable();
	test.End();

	test.Next(_L("class TBuf8 returning TPtr8"));
	TestTDes<TBuf8<0x40>, TText8, TUint8, TPtr8> rr;
	rr.test_TBufReturningModifiable();
	test.End();

	test.Next(_L("class TBuf16 returning TPtr16"));
	TestTDes<TBuf16<0x40>, TText16, TUint16, TPtr16> ss;
	ss.test_TBufReturningModifiable();
	test.End();

	test.Next(_L("class TPtr returning TPtr"));
	TestTDes<TPtr, TText, TUint, TPtr> tt;
	tt.test_TPtrReturningModifiable();
	test.End();

	test.Next(_L("class TPtr8 returning TPtr8"));
	TestTDes<TPtr8, TText8, TUint8, TPtr8> uu;
	uu.test_TPtrReturningModifiable();
	test.End();

	test.Next(_L("class TPtr16 returning TPtr16"));
	TestTDes<TPtr16, TText16, TUint16, TPtr16> vv;
	vv.test_TPtrReturningModifiable();
	test.End();

	testSurrogateAwareInterfaces();
	test.End();

	return(KErrNone);
    }
#pragma warning (default: 4702)// Unreachable code
#pragma warning (default: 4701)// Local variable used without being initialized
#pragma warning (default: 4710)// Function not expanded


