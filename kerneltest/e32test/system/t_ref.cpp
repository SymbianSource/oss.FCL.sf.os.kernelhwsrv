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
// e32test\system\t_ref.cpp
// Overview:
// Test the methods of the RRef class.
// API Information:
// RRef
// Details:
// - Check for the existence of all RRef methods.
// - Test and verify the results of the RRef constructors.
// - Test and verify the results of the RRef assignment methods.
// - Test and verify the results of the RRef smart pointer methods.
// - Attempt to allocate all available heap space, verify that it 
// cause a leave.
// - Test and verify the results of the RRef allocation methods.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("T_REF"));

struct SObj
	{
	TInt iInt;
	TBuf<0x100> aBuf;
	};

void Test1()
// All methods
	{

	__UHEAP_MARK;
	SObj anObj;
	anObj.iInt=10;
	anObj.aBuf=_L("Hello World");
	RRef<SObj> aRef1;
	aRef1.Alloc(anObj);
	RRef<SObj> aRef2(aRef1);
	aRef2=aRef1;
	aRef2.AllocL(anObj);
	test(aRef1->iInt==10);
	test(((SObj*)aRef1)->aBuf==_L("Hello World"));
	aRef1.Free();
	aRef2.Free();
	__UHEAP_MARKEND;
	}	

void Test2()
// Constructors
	{
	__UHEAP_MARK;
	__UHEAP_MARK;
	SObj anObj;
	anObj.iInt=10;
	anObj.aBuf=_L("Hello World");
	RRef<SObj> aRef1;
	aRef1.Alloc(anObj);
	RRef<SObj> aRef2(aRef1);
	test(aRef1->iInt==10);
	test(aRef1->aBuf==_L("Hello World"));
	test(aRef2->iInt==10);
	test(aRef2->aBuf==_L("Hello World"));
	aRef1.Free();
	__UHEAP_MARKENDC(1);
	aRef2.Free();
	__UHEAP_MARKEND;
	}

void Test3()
// Assignment methods
	{
	__UHEAP_MARK;
	__UHEAP_MARK;
	SObj anObj;
	anObj.iInt=10;
	anObj.aBuf=_L("Hello World");
	RRef<SObj> aRef1;
	RRef<SObj> aRef2;
	aRef1.Alloc(anObj);
	aRef2=aRef1;
	test(aRef1->iInt==10);
	test(aRef1->aBuf==_L("Hello World"));
	test(aRef2->iInt==10);
	test(aRef2->aBuf==_L("Hello World"));
	aRef1.Free();
	__UHEAP_MARKENDC(1);
	aRef2.Free();
	__UHEAP_MARKEND;
	}

void Test4()
// Smart Pointer methods
	{
	SObj anObj;
	anObj.iInt=10;
	anObj.aBuf=_L("Hello World");
	RRef<SObj> aRef1;
	aRef1.Alloc(anObj);
	test(aRef1->iInt==10);
	test(aRef1->aBuf==_L("Hello World"));
	SObj* pObj=(SObj*)aRef1;
	test(pObj->iInt==10);
	test(pObj->aBuf==_L("Hello World"));
	}

void allocAllL(TInt*& aPtr,RRef<SObj>& aRef1,SObj& anObj)
//
// Alloc all and cause leave.
//
	{

	TInt temp;
	TInt avail=User::Available(temp);
	aPtr=(TInt*)User::AllocL(avail);	
	aRef1.AllocL(anObj);

/*

    Reinstate if some method is found of forcing alloc fail
    This test no longer works due to heap growing functionality of E32 057

    ... so leave with KErrNoMemory to simulate alloc failure
*/
    User::Leave(KErrNoMemory);
	}

void Test5()
//
// Allocation methods
//
	{

	__UHEAP_MARK;
	SObj anObj;
	anObj.iInt=10;
	anObj.aBuf=_L("Hello World");
	RRef<SObj> aRef1;
	aRef1.Alloc(anObj);
	test(aRef1->iInt==10);
	test(aRef1->aBuf==_L("Hello World"));
	aRef1.Alloc(anObj,sizeof(SObj)-245*sizeof(TText));
	test(aRef1->iInt==10);
	test(aRef1->aBuf==_L("Hello World"));
	aRef1.AllocL(anObj);
	test(aRef1->iInt==10);
	test(aRef1->aBuf==_L("Hello World"));
	aRef1.AllocL(anObj,sizeof(SObj)-245*sizeof(TText));
	test(aRef1->iInt==10);
	test(aRef1->aBuf==_L("Hello World"));
	TInt* p=0;
	TRAPD(ret,allocAllL(p,aRef1,anObj))
	test(ret==KErrNoMemory);
	User::Free(p);
	aRef1.AllocL(anObj);
	test(aRef1->iInt==10);
	test(aRef1->aBuf==_L("Hello World"));
	aRef1.Free();
	__UHEAP_MARKEND;
	}

TInt E32Main()
	{

	test.Title();
	test.Start(_L("class RRef"));
	test.Next(_L("Test all methods"));
	Test1();
	test.Next(_L("Constructors"));
	Test2();
	test.Next(_L("Assignment methods"));
	Test3();
	test.Next(_L("Smart pointer methods"));
	Test4();
	test.Next(_L("Allocation methods"));
	Test5();
	test.End();
	return(0);
	}


