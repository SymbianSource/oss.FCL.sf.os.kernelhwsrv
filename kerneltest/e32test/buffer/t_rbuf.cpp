// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\buffer\t_rbuf.cpp
// Overview:
// Test methods of the RBuf16, RBuf8, RBuf template class.
// API Information:
// RBuf16, RBuf8, RBuf.
// Details:
// For RBuf8, RBuf16 and RBuf objects:
// - Test the Create and CreateMax methods by verifying the return value of
// KErrNone, the initial length and max length. Perform basic write and read
// operations and verify the results.
// - Test the CreateL and CreateMaxL methods by verifying the return value of 
// KErrNone. Also force a heap error and verify return value of KErrNoMemory.
// - Test the Create(const TDesC_& aDesc) and Create(const TDesCX_ aDesc, 
// TInt aMaxLength) methods by verifying the return value of KErrNone. Verify
// initial length, max length and initialisation.
// - Test the CreateL(const TDesC_& aDesc) and CreateMaxL(const TDesCX_ aDesc, 
// TInt aMaxLength) methods by verifying the return value of KErrNone. Also 
// force a heap error and verify return value of KErrNoMemory.
// - Test the Swap method by creating two initialised objects, calling Swap 
// and confirming the results as expected.
// - Test the Assign method by performing an assign from a variety of sources
// and verifying the results are as expected.
// - Test the ReAlloc method in a variety of scenarios that decrease memory, 
// increase memory and zero-length memory. Verify that the results are as
// expected.
// - Test the ReAllocL by verifying the return value of KErrNone. Also force 
// a heap error and verify return value of KErrNoMemory. Verify that the
// object is the same as before the failed ReAllocL call.
// - Test the CleanupClosePushL method via CleanupStack::PopAndDestroy().
// - Force the CleanupClosePushL to leave to check cleanup of RBuf.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32math.h>
#include "u32std.h"

LOCAL_D RTest test(_L("T_RBUF"));

#undef _TS
#define _TS(a) ((const TTEXT*)RTest::String(sizeof(TTEXT),(TText8*)a,(TText16*)L ## a)) 

/**
Tests the following methods. 
 - TInt Create(TInt aMaxLength);
 - TInt CreateMax(TInt aMaxLength);
*/
template<class RBUF>	
LOCAL_C void TestCreate(RBUF*)
{
	RBUF rBuf;
	
	test.Next(_L("Create(TInt aMaxLength) method"));

	test(rBuf.Create(19)==KErrNone);	//Create RBuf as EPtr type
	test(rBuf.Length()==0);
	test(rBuf.MaxLength()==19);
	rBuf.SetLength(2);
	rBuf[1] = 1;						//Try basic write & ...
	test(rBuf[1] == 1);					//... read 
	rBuf.Close();

	test(rBuf.Create(0)==KErrNone);		//Create zero length RBuf as EPtr type
	test(rBuf.Length()==0);
	test(rBuf.MaxLength()==0);
	rBuf.Close();
	
	test.Next(_L("CreateMax(TInt aMaxLength) method"));
	
	test(rBuf.CreateMax(20)==KErrNone);	//Create RBuf as EPtr type
	test(rBuf.Length()==20);
	test(rBuf.MaxLength()==20);
	rBuf[1] = 1;
	test(rBuf[1] == 1);
	rBuf.Close();
}

/**
Tests the following methods. 
 - void CreateL(TInt aMaxLength);
 - void CreateMaxL(TInt aMaxLength);
*/
template<class RBUF>
LOCAL_C void TestCreateLeaving(RBUF*)
{
	RBUF rBuf;

	test.Next(_L("CreateL(TInt aMaxLength) method"));

	TRAPD(ret, rBuf.CreateL(20));	//Create RBuf as EPtr type
	test(KErrNone == ret);
	rBuf.Close();

#if defined(_DEBUG)
	__UHEAP_FAILNEXT(1);			//Set the next alloc to fail
	TRAP(ret, rBuf.CreateL(10));	
	test(KErrNoMemory == ret);		// It fails due to __UHEAP_FAILNEXT(1);
#endif //_DEBUG

	test.Next(_L("CreateMaxL(TInt aMaxLength) method"));

	TRAP(ret, rBuf.CreateMaxL(20));	//Create RBuf as EPtr type
	test(KErrNone == ret);
	rBuf.Close();

#if defined(_DEBUG)
	__UHEAP_FAILNEXT(1);			//Set the next alloc to fail
	TRAP(ret, rBuf.CreateMaxL(10));	
	test(KErrNoMemory == ret);		// It fails due to __UHEAP_FAILNEXT(1);
#endif //_DEBUG
}

/**
Tests the following methods. 
 - TInt Create(const TDesC_& aDesc);
 - TInt Create(const TDesC_& aDesc, TInt aMaxLength));
*/
template<class RBUF, class TBUF, class TTEXT>	
LOCAL_C void TestCreateFromDes(RBUF*)
{
	RBUF rBuf;
	TBUF des (_TS("012345"));

	test.Next(_L("Create(const TDesC_& aDesc) method"));

	test(rBuf.Create(des)==KErrNone);					//Create RBuf as EPtr type
	test(rBuf == des);
	rBuf.Close();

	test.Next(_L("Create(const TDesCX_ aDesc, TInt aMaxLength) method"));

	test(rBuf.Create(des, des.Length())==KErrNone);		//Create RBuf as EPtr type
	test(rBuf==des);
	rBuf.Close();

	test(rBuf.Create(des, des.Length()-2)==KErrNone);	//Create RBuf as EPtr type
	test(rBuf.Length()==4);
	test(rBuf.MaxLength()==4);
	test(rBuf[0] == (TTEXT)('0'));
	test(rBuf[3] == (TTEXT)('3'));
	test(rBuf<des);
	rBuf.Close();

	test(rBuf.Create(des, des.Length()+2)==KErrNone);	//Create RBuf as EPtr type
	test(rBuf.Length()==6);
	test(rBuf.MaxLength()==8);
	test(rBuf==des);
	rBuf.Close();
}

/**
Tests the following methods. 
 - void CreateL(const TDesC_& aDesc);
 - void CreateMaxL(const TDesC_& aDesc, TInt aMaxLength);
*/
template<class RBUF, class TBUF, class TTEXT>
LOCAL_C void TestCreateFromDesLeaving(RBUF*)
{
	RBUF rBuf;
	TBUF des (_TS("123456"));

	test.Next(_L("CreateL(const TDesC_& aDesc) method"));

	TRAPD(ret, rBuf.CreateL(des));				//Create RBuf as EPtr type
	test(KErrNone == ret);
	rBuf.Close();

#if defined(_DEBUG)
	__UHEAP_FAILNEXT(1);						//Set the next alloc to fail
	TRAP(ret, rBuf.CreateL(des));	
	test(KErrNoMemory == ret);					// This will fail due to __UHEAP_FAILNEXT(1);
#endif //(_DEBUG)

	test.Next(_L("CreateL(const TDesC_& aDesc, TInt aMaxLength) method"));

	TRAP(ret, rBuf.CreateL(des, des.Length()));	//Create RBuf as EPtr type
	test(KErrNone == ret);
	rBuf.Close();

#if defined(_DEBUG)
	__UHEAP_FAILNEXT(1);						//Set the next alloc to fail
	TRAP(ret, rBuf.CreateL(des, des.Length()));	
	test(KErrNoMemory == ret);					// It fails due to __UHEAP_FAILNEXT(1);
#endif //(_DEBUG)
}

/**
Tests the following methods:
 - TInt Assign(const RBuf_& rBuf);
 - TInt Assign(TUint* aHeapCell, TInt aMaxLength);
 - TInt Assign(TUint* aHeapCell, TInt aLength, TInt aMaxLength);
 - TInt Assign(HBufC& aHBuf);
 - RBuf(HBufC_&) constructor.
*/
template<class RBUF, class TBUF, class TTEXT, class HBUF>	
LOCAL_C void TestAssign(RBUF*)
{
	RBUF rBuf;
	TBUF des (_TS("123456"));
	RBUF rBuf2;

	test.Next(_L("Assign(const RBuf_& aRBuf) method"));

	rBuf2.Create(des);
	rBuf.Assign(rBuf2);
	test(rBuf==rBuf2);
	rBuf.Close();
	
	test.Next(_L("Assign(TUint* aHeapCell, TInt aLength, TInt aMaxLength ) method"));

	TTEXT* heap = (TTEXT*)User::Alloc(24*(TInt)sizeof(TTEXT)); //Allocate 48 bytes for 24 long RBuf16
	rBuf.Assign(heap, 12,24);
	test(rBuf.Length() == 12);		
	test(rBuf.MaxLength() == 24);		
	rBuf.Close();

	heap = NULL;
	rBuf.Assign(heap, 0,0);
	test(rBuf.Length() == 0);		
	test(rBuf.MaxLength() == 0);		
	rBuf.Close();
	
	test.Next(_L("Assign(TUint* aHeapCell, TInt aMaxLength ) method"));
	
	heap = (TTEXT*)User::Alloc(24*(TInt)sizeof(TTEXT)); //Allocate 48 bytes for 24 long RBuf16
	rBuf.Assign(heap, 24);
	test(rBuf.Length() == 0);		
	test(rBuf.MaxLength() == 24);		
	rBuf.Close();

	test.Next(_L("Assign(HBufC_* aHBuf) method"));

	HBUF* hBuf = HBUF::NewMax(11);
	rBuf.Assign(hBuf);			//Create RBuf as EBufCPtr type
	test(rBuf.Length() == 11);
	test(rBuf.MaxLength() >= 11); //There could me more allocated memory - see HBufC8::Des()
	rBuf.Close();

	test.Next(_L("RBuf_(HBufC_* aHBuf) constructor"));

	hBuf = HBUF::NewMax(12);	//Create RBuf as EBufCPtr
	RBUF rBuf3(hBuf);
	test(rBuf3.Length() == 12);
	test(rBuf3.MaxLength() >= 12);
	rBuf3.Close();

	hBuf = HBUF::NewMax(0);
	RBUF rBuf4(hBuf);			//The length of aHBuf is zero
	test(rBuf4.Length() == 0);
	rBuf4.Close();

	hBuf = NULL;				//aHBuf is NULL
	RBUF rBuf5(hBuf);
	test(rBuf5.Length() == 0);
	test(rBuf5.MaxLength() == 0);
	rBuf5.Close();
}

/**
Tests the following methods. 
 - TInt ReAlloc(TInt aMaxLength);
*/
template<class RBUF, class TBUF, class TTEXT, class HBUF>	
LOCAL_C void TestReAlloc(RBUF*)
{
	RBUF rBuf;

	TBUF des (_TS("0123456"));


	test.Next(_L("ReAlloc(TInt aMaxLength) method"));

	//reallocate EPtr type - decrease memory
	test(rBuf.Create(des)==KErrNone);					//Create as EPtr
	rBuf.SetLength(3);
	test(rBuf.ReAlloc(3)==KErrNone);					//ReAlloc to EPtr
	test(rBuf.MaxLength()>=3);
	test(rBuf.Length()==3);
	test(rBuf[0] == (TTEXT)('0'));
	test(rBuf[2] == (TTEXT)('2'));
	rBuf.Close();

	//reallocate EPtr type - increase memory
	test(rBuf.Create(des,des.MaxLength())==KErrNone);	//Create as EPtr
	test(rBuf.ReAlloc(15)==KErrNone);					//ReAlloc to EPtr
	test(rBuf.MaxLength()==15);
	test(rBuf.Length()==7);
	test(rBuf[0] == (TTEXT)('0'));
	test(rBuf[6] == (TTEXT)('6'));
	rBuf.Close();


	//reallocate EBufCPtr type - decrease memory
	HBUF* hBuf = HBUF::NewMax(9);
	*hBuf = _TS("012345678");
	rBuf.Assign(hBuf);						//Create as EBufCPtr
	rBuf.SetLength(5);
	test(rBuf.ReAlloc(5)==KErrNone);		//ReAlloc to EBufCPtr
	test(rBuf.MaxLength()>=5);//There could be more allocated memory - see HBufC8::Des()
	test(rBuf.Length()==5);
	test(rBuf[0] == (TTEXT)('0'));
	test(rBuf[4] == (TTEXT)('4'));
	rBuf.Close();

	//reallocate EBufCPtr type - increase memory
	hBuf = HBUF::NewMax(9);
	*hBuf = _TS("012345678");
	rBuf.Assign(hBuf);						//Create as EBufCPtr
	test(rBuf.ReAlloc(15)==KErrNone);		//ReAlloc to EBufCPtr
	test(rBuf.MaxLength()>=15);//There could be more allocated memory - see HBufC8::Des()
	test(rBuf.Length()==9);
	test(rBuf[0] == (TTEXT)('0'));
	test(rBuf[8] == (TTEXT)('8'));
	rBuf.Close();

	//reallocate EPtr type - to zero-length
	test(rBuf.Create(des)==KErrNone);		//Create as EPtr
	rBuf.SetLength(0);
	test(rBuf.ReAlloc(0)==KErrNone);		//ReAlloc to EPtr
	test(rBuf.MaxLength()==0);
	test(rBuf.Length()==0);
	rBuf.Close();

	//reallocate EBufCPtr type to zero-length
	hBuf = HBUF::NewMax(9);
	*hBuf = _TS("012345678");
	rBuf.Assign(hBuf);						//Create as EBufCPtr
	rBuf.SetLength(0);
	test(rBuf.ReAlloc(0)==KErrNone);		//ReAlloc to EPtr
	test(rBuf.MaxLength()==0);
	test(rBuf.Length()==0);
	rBuf.Close();

	//reallocate from zero-length
	rBuf.Create(0);							//Create as EPtr
	test(rBuf.ReAlloc(9)==KErrNone);		//ReAlloc to EPtr
	test(rBuf.MaxLength()==9);
	test(rBuf.Length()==0);
	rBuf.Close();

	//reallocate from zero-length EBufCPtr to EPtr
	struct dummy // make it look like RBuf16
		{
		TInt iLength;
		TInt iMaxLength;
		HBUF* iEBufCPtrType;	//Pointer to buffer data
		};

	// reference rBuf as our dummy.. 
	dummy &drBuf = (dummy&) rBuf;
	rBuf.Assign(HBUF::NewL(0)); 			//Create as EBufCPtr
	test(EBufCPtr == (drBuf.iLength>>KShiftDesType));
	rBuf.Close(); 	// the actual behavior causes memory leaks, so we should close it first.
	test(rBuf.ReAlloc(13)==KErrNone);	// ReAlloc changes it from EBufCPtr to EPtr
	test(EPtr == (drBuf.iLength>>KShiftDesType));
	test(rBuf.MaxLength() == 13);		
	test(rBuf.Length() == 0);		
	rBuf.Close();

	//reallocate from zero-length to zero-length
	rBuf.Create(0);							//Create as EPtr
	test(rBuf.ReAlloc(0)==KErrNone);		//ReAlloc to EPtr
	test(rBuf.Length() == 0);		
	test(rBuf.MaxLength() == 0);		
	rBuf.Close();

}

/**
Tests the following methods. 
 - TInt ReAllocL(TInt aMaxLength);
*/
template<class RBUF, class TBUF, class TTEXT>	
LOCAL_C void TestReAllocLeaving(RBUF*)
{
	RBUF rBuf;

	TBUF des(_TS("01"));

	test.Next(_L("ReAllocL(TInt aMaxLength) method"));

	test(rBuf.Create(des) ==KErrNone);
	TRAPD(ret, rBuf.ReAllocL(6));	//ReAlloc buffer
	test(KErrNone == ret);

#if defined(_DEBUG)
	__UHEAP_FAILNEXT(1);
	TRAP(ret, rBuf.ReAllocL(100));	//Realloc buffer. This should fail.
	test(KErrNoMemory == ret);
#endif //(_DEBUG)

	test(rBuf.MaxLength()==6);		//Check RBuf is the same as before ... 
	test(rBuf.Length()==2);			//... ReAlloc that failed.
	test(rBuf[0] == (TTEXT)('0'));
	test(rBuf[1] == (TTEXT)('1'));
	rBuf.Close();
}

/**
Tests the following methods. 
 - void Swap(RBuf_& aBuf);
*/
template<class RBUF, class TBUF, class TTEXT>
LOCAL_C void TestSwap(RBUF*)
{
	RBUF rBuf1, rBuf2;
	TBUF des1(_TS("12"));
	TBUF des2 (_TS("345678"));

	test.Next(_L("Swap(RBuf_& aRBuf) method"));

	test(rBuf1.Create(des1) ==KErrNone);
	test(rBuf2.Create(des2) ==KErrNone);

	rBuf1.Swap(rBuf2);

	test(rBuf1==des2);
	test(rBuf2==des1);

	rBuf1.Close();
	rBuf2.Close();
}

/**
Test assignemnt operator.
*/
template<class RBUF, class TBUF, class TBUFC, class TTEXT>
LOCAL_C void TestAssignmentOperator()
{
	test.Next(_L("Assignment operator"));

	TBUF tdes(_TS("Modifiable descriptor"));
	TBUFC tdesc(_TS("Non-modifiable descriptor"));

	RBUF rbuf, rbuf2;
	rbuf.Create(32);
	rbuf2.Create(32);
	rbuf2.Copy(_TS("Buffer descriptor"), 17);

	rbuf = tdesc;	test(rbuf == tdesc);
	rbuf = tdes;		test(rbuf == tdes);
	rbuf = rbuf2;	test(rbuf == rbuf2);

	rbuf2.Close();
	rbuf.Close();
}

/**
Tests the following methods. 
 - void CleanupClosePushL();
*/
template<class RBUF> LOCAL_C void TestCleanupClosePushL(RBUF*)
{
	RBUF rBuf;
	
	test.Next(_L("CleanupClosePushL() method"));
	test(KErrNone == rBuf.Create(10));
	rBuf.CleanupClosePushL();
	CleanupStack::PopAndDestroy();
}

/**
This function will intentionally leave to check cleanup of RBuf.
To be called in debug build only. Otherwise will panic.
*/
template<class RBUF> LOCAL_C void TestRBufCleanupL(RBUF*)
{
	RBUF rBuf;

	test.Next(_L("Test cleanup of RBuf"));
	test(KErrNone == rBuf.Create(10));
	rBuf.CleanupClosePushL();

	__UHEAP_FAILNEXT(1);
	TInt* ptr = (TInt*)User::AllocL(20); //This should leave
	*ptr = 0; //Avoid compiler warning
	User::Panic(_L("Should not reach this line"),0);
}

GLDEF_C TInt E32Main()
    {
	RBuf8* r8=0;
	RBuf16* r16=0;
	RBuf* r=0;

	CTrapCleanup* trapHandler=CTrapCleanup::New();
	test(trapHandler!=NULL);

	test.Title();
	test.Start(_L("Testing RBuf8, RBuf16 & RBuf classes"));

	__UHEAP_MARK;

	test.Start(_L("Testing class RBuf8 ..."));
	TestCreate<RBuf8>(r8);
	TestCreateLeaving<RBuf8>(r8);
	TestCreateFromDes<RBuf8,TBuf8<11>,TText8>(r8);
	TestCreateFromDesLeaving<RBuf8,TBuf8<11>,TText8>(r8);
	TestSwap<RBuf8,TBuf8<11>,TText8>(r8);
	TestAssign<RBuf8,TBuf8<11>,TText8,HBufC8>(r8);
	TestReAlloc<RBuf8,TBuf8<11>,TText8,HBufC8>(r8);
	TestReAllocLeaving<RBuf8,TBuf8<11>,TText8>(r8);
	TestAssignmentOperator<RBuf8,TBuf8<32>,TBufC8<32>,TText8>();
	TRAPD(ret,TestCleanupClosePushL<RBuf8>(r8)); test(ret==KErrNone);
#if defined(_DEBUG)
	TRAP(ret, TestRBufCleanupL<RBuf8>(r8)); test(KErrNoMemory == ret);
#endif //(_DEBUG)
	test.End();

	test.Start(_L("Testing class RBuf16 ..."));
	TestCreate<RBuf16>(r16);
	TestCreateLeaving<RBuf16>(r16);
	TestCreateFromDes<RBuf16,TBuf16<11>,TText16>(r16);
	TestCreateFromDesLeaving<RBuf16,TBuf16<11>,TText16>(r16);
	TestSwap<RBuf16,TBuf16<11>,TText16>(r16);
	TestAssign<RBuf16,TBuf16<11>,TText16,HBufC16>(r16);
	TestReAlloc<RBuf16,TBuf16<11>,TText16,HBufC16>(r16);
	TestReAllocLeaving<RBuf16,TBuf16<11>,TText16>(r16);
	TestAssignmentOperator<RBuf16,TBuf16<32>,TBufC16<32>,TText16>();
	TRAP(ret,TestCleanupClosePushL<RBuf16>(r16)); test(ret==KErrNone);
#if defined(_DEBUG)
	TRAP(ret, TestRBufCleanupL<RBuf16>(r16)); test(KErrNoMemory == ret);
#endif //(_DEBUG)
	test.End();

	test.Start(_L("Testing class RBuf ..."));
	TestCreate<RBuf>(r);
	TestCreateLeaving<RBuf>(r);
	TestCreateFromDes<RBuf,TBuf<11>,TText>(r);
	TestCreateFromDesLeaving<RBuf,TBuf<11>,TText>(r);
	TestSwap<RBuf,TBuf<11>,TText>(r);
	TestAssign<RBuf,TBuf<11>,TText,HBufC>(r);
	TestReAlloc<RBuf,TBuf<11>,TText,HBufC>(r);
	TestReAllocLeaving<RBuf,TBuf<11>,TText>(r);
	TestAssignmentOperator<RBuf,TBuf<32>,TBufC<32>,TText>();
	TRAP(ret,TestCleanupClosePushL<RBuf>(r)); test(ret==KErrNone);
#if defined(_DEBUG)
	TRAP(ret, TestRBufCleanupL<RBuf>(r)); test(KErrNoMemory == ret);
#endif //(_DEBUG)
	test.End();

	__UHEAP_MARKEND;

	test.End();

	delete trapHandler;
	return(KErrNone);
    }

