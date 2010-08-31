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
// e32test\system\t_cobj.cpp
// Overview:
// Test CObject, CObjectIx, CObjectCon, CObjectConIx classes
// API Information:
// CObject, CObjectIx, CObjectCon, CObjectConIx
// Details:
// - Test the CObjectIx class by adding and removing CObject objects
// in the same order, in reverse order and in random order.
// - Test adding many CObject objects to a CObjectConIx and test that
// the returned handle is as expected.
// - Perform a speed test on CObjectCon for both named and unnamed
// objects.
// - Check that the CObject, CObjectCon, CObjectIx and CObjectConIx methods 
// are in the DLL by calling each one.
// - Test the CObject, CObjectCon, CObjectIx and CObjectConIx methods and
// verify the results are as expected.
// - Test all the objects together: create two containers, find objects by 
// name in a variety of ways, delete objects and verify the results are 
// as expected.
// - Test panic functions by behaving badly
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32svr.h>
#include <e32ver.h>
#include <e32panic.h>
#include "../misc/prbs.h"

struct TCObjectDump
	{
	TInt 		iAccessCount;
	CObject*	iOwner;
	CObjectCon*	iContainer;
	HBufC*		iName;
	};

void CObject::__DbgTest(void* pCObjectDump) const
	{
	((TCObjectDump*)pCObjectDump)->iAccessCount=iAccessCount;
	((TCObjectDump*)pCObjectDump)->iOwner=iOwner;
	((TCObjectDump*)pCObjectDump)->iContainer=iContainer;
	((TCObjectDump*)pCObjectDump)->iName=iName;
	}

LOCAL_D RTest test(_L("T_COBJ"));
LOCAL_D	TName gName;
LOCAL_D	TName gName1;
LOCAL_D	TName gName2;
LOCAL_D	TFullName gFullName;
LOCAL_D	TFullName gFullName1;
LOCAL_D	TFullName gFullName2;

class RTestHeap : public RAllocator
	{
public:
	enum TOp {EOpNone=0, EOpAlloc=1, EOpFree=2, EOpReAlloc=3};

	struct TOpInfo
		{
		TInt	iOp;
		TAny*	iPtrArg;
		TInt	iIntArg;
		TAny*	iResult;
		};
public:
	static RTestHeap* Install();
	void Uninstall();
	TInt GetLastOp(TOpInfo& aInfo);
	virtual TAny* Alloc(TInt aSize);
	virtual void Free(TAny* aPtr);
	virtual TAny* ReAlloc(TAny* aPtr, TInt aSize, TInt aMode);
	virtual TInt AllocLen(const TAny* aCell) const;
	virtual TInt Compress();
	virtual void Reset();
	virtual TInt AllocSize(TInt& aTotalAllocSize) const;
	virtual TInt Available(TInt& aBiggestBlock) const;
	virtual TInt DebugFunction(TInt aFunc, TAny* a1, TAny* a2);
public:
	RAllocator* iA;
	TOpInfo iLastOp;
	};

RTestHeap* RTestHeap::Install()
	{
	RTestHeap* p = new RTestHeap;
	test(p!=0);
	p->iA = &User::Heap();
	p->iLastOp.iOp = EOpNone;
	User::SwitchHeap(p);
	return p;
	}

void RTestHeap::Uninstall()
	{
	User::SwitchHeap(iA);
	delete this;
	}

TInt RTestHeap::GetLastOp(RTestHeap::TOpInfo& aInfo)
	{
	memcpy(&aInfo, &iLastOp, sizeof(TOpInfo));
	iLastOp.iOp = EOpNone;
	return (aInfo.iOp == EOpNone) ? KErrNotFound : KErrNone;
	}

TAny* RTestHeap::Alloc(TInt aSize)
	{
	TAny* p = iA->Alloc(aSize);
	iLastOp.iOp = EOpAlloc;
	iLastOp.iPtrArg = 0;
	iLastOp.iIntArg = aSize;
	iLastOp.iResult = p;
	return p;
	}

void RTestHeap::Free(TAny* aPtr)
	{
	iLastOp.iOp = EOpFree;
	iLastOp.iPtrArg = aPtr;
	iLastOp.iIntArg = 0;
	iLastOp.iResult = 0;
	iA->Free(aPtr);
	}

TAny* RTestHeap::ReAlloc(TAny* aPtr, TInt aSize, TInt aMode=0)
	{
	TAny* p = iA->ReAlloc(aPtr,aSize,aMode);
	iLastOp.iOp = EOpReAlloc;
	iLastOp.iPtrArg = aPtr;
	iLastOp.iIntArg = aSize;
	iLastOp.iResult = p;
	return p;
	}

TInt RTestHeap::AllocLen(const TAny* aCell) const
	{
	TInt l = iA->AllocLen(aCell);
	return l;
	}

TInt RTestHeap::Compress()
	{
	TInt l = iA->Compress();
	return l;
	}

void RTestHeap::Reset()
	{
	iA->Reset();
	}

TInt RTestHeap::AllocSize(TInt& aTotalAllocSize) const
	{
	TInt s;
	TInt r = iA->AllocSize(s);
	aTotalAllocSize = s;
	return r;
	}

TInt RTestHeap::Available(TInt& aBiggestBlock) const
	{
	TInt s;
	TInt r = iA->Available(s);
	aBiggestBlock = s;
	return r;
	}

TInt RTestHeap::DebugFunction(TInt aFunc, TAny* a1=NULL, TAny* a2=NULL)
	{
	TInt r = iA->DebugFunction(aFunc, a1, a2);
	return r;
	}




class TestCObjects
	{
public:
	void Test1(void);
	void Test2(void);
	void Test3(void);
	void Test4(void);
	void Test5(void);
	void Test6(void);
	void Test7(void);
	void Test8(void);
	void Test9(void);
private:
	static void GetObjName(TName& aDest,const CObject& aObj);
	static void GetObjFullName(TFullName& aDest,const CObject& aObj);
	};


GLDEF_C void TestCObjects::Test1(void)
	{
	// Check CObject methods are in the DLL
	CObject* pCObject=new CObject; 
	CObject* temp=new CObject;

	pCObject->Open();
	pCObject->Close();
	gName=pCObject->Name();
	TPtrC aDesR(_S("a name"));
	pCObject->SetName(&aDesR);
	pCObject->SetNameL(&aDesR);
	gFullName=pCObject->FullName();
	pCObject->Owner();
	pCObject->SetOwner(temp);
	pCObject->Close(); // deletes object when iAccessCount==0
	temp->Close();
	}

GLDEF_C void TestCObjects::Test2(void)
	{
	// Check CObjectCon methods are in the DLL
	CObjectCon* pCObjectCon=CObjectCon::NewL();
	CObject* pCObject=new CObject;
	TInt aFindHandle=0;
	
	pCObjectCon->AddL(pCObject);
	pCObjectCon->At(aFindHandle);
	User::ValidateName(_L("a name"));
	pCObjectCon->CheckUniqueFullName(pCObject, _L("a name"));
	pCObjectCon->FindByName(aFindHandle, _L("a name"), gName); 
	pCObjectCon->FindByFullName(aFindHandle, _L("a Name"), gFullName);
	pCObjectCon->UniqueID();
	pCObjectCon->Count();
	pCObjectCon->Remove(pCObject); 
	pCObject->Close();
	delete pCObjectCon;
	}

GLDEF_C void TestCObjects::Test3(void)
	{	
	// Check the CObjectIx methods are in the DLL		  
	CObjectIx* pCObjectIx=CObjectIx::NewL();
	pCObjectIx->Count();
	test(pCObjectIx->At(0)==NULL);
	delete pCObjectIx;
	pCObjectIx=CObjectIx::NewL();
	test(pCObjectIx->Count()==0);
	CObjectConIx* pCObjectConIx=CObjectConIx::NewL();
	delete pCObjectConIx;
	pCObjectConIx=CObjectConIx::NewL();
	CObjectCon* pCObjectCon=pCObjectConIx->CreateL();
	test(pCObjectCon->Count()==0);
	CObject* pCObject= new CObject;
	test(pCObject!=NULL);

	TInt find;
	TName name;
	TInt r=pCObjectCon->FindByName(find,_L("Name"),name);
	test(r==KErrNotFound);
	TFullName fullName;
	r=pCObjectCon->FindByFullName(find,_L("Full Name"),fullName);
	test(r==KErrNotFound);


	pCObjectCon->AddL(pCObject);
	test(pCObjectCon->Count()==1);
	test(pCObjectIx->Count(pCObject)==0);
	pCObjectIx->AddL(pCObject); 
	test(pCObjectIx->Count(pCObject)==1);
	pCObjectIx->Count();
	pCObjectIx->At(0, 0);
	pCObjectIx->At(0);
//	pCObjectIx->Remove(0);
	delete pCObjectIx;
	delete pCObjectConIx;
//	delete pCObjectCon;
//	pCObject->Close();
	}

GLDEF_C void TestCObjects::Test4(void)
	{
	// Check CObjectConIx methods are in the DLL
	CObjectConIx* pCObjectConIx=CObjectConIx::NewL();
	CObjectCon* pCObjectCon=pCObjectConIx->CreateL();
	pCObjectConIx->Lookup(0);
	pCObjectConIx->Remove(pCObjectCon);
	delete pCObjectConIx;
	}

GLDEF_C void TestCObjects::Test5(void)
	{
	// Test the methods of the CObject class
	TCObjectDump CObjectDump;
	CObject* a=new CObject;
	CObject* b=new CObject;
	a->__DbgTest(&CObjectDump);
	test(CObjectDump.iAccessCount==1 && CObjectDump.iOwner==NULL && CObjectDump.iContainer==NULL);
	a->Open();
	a->__DbgTest(&CObjectDump);
	test(CObjectDump.iAccessCount==2);
	a->Close();
	a->__DbgTest(&CObjectDump);
	test(CObjectDump.iAccessCount==1);
	TPtrC aDesR(_L("aName"));
	a->SetName(&aDesR);
    gName=a->Name();
	test(gName==_L("aName"));
    gFullName=a->FullName();
	test(gFullName==_L("aName"));
	TPtrC aDesR2(_L("Owner"));
	b->SetName(&aDesR2);
	a->SetOwner(b);
	test(a->Owner()==b);
    gFullName=a->FullName();
	test(gFullName==_L("Owner::aName"));
	a->Close();  // Calls the destructor via the call to delete
// To Do: set iContainer to something then call Close() 
	}


GLDEF_C void TestCObjects::Test6(void)
	{
	// Test the methods of CObjectCon
	TCObjectDump dump1, dump2;
	TInt aFindHandle=0;
	CObject	*pObj1=new CObject,
			*pObj2=new CObject,
			*pObj3=new CObject,
			*temp;
	CObjectCon* pCon=CObjectCon::NewL();

	test(User::ValidateName(_L("xxx*xxx"))==KErrBadName);
	test(User::ValidateName(_L("xxx?xxx"))==KErrBadName);
	test(User::ValidateName(_L("xxx:xxx"))==KErrBadName);
	test(User::ValidateName(_L("xxxxxxx"))==KErrNone);

	TPtrC name1(_S("Obj1")), name2(_S("Obj2")), name3(_S("Owner"));
	pObj1->SetName(&name1);
	pObj2->SetName(&name2);
	pObj3->SetName(&name3);

	test(pCon->Count()==0);
	pCon->AddL(pObj1);
	test(pCon->Count()==1);
	pCon->AddL(pObj2);
	test(pCon->Count()==2);

	test((*pCon)[0]==pObj1);
	test((*pCon)[1]==pObj2);

	aFindHandle=0;
	test(pCon->FindByName(aFindHandle, _L("xxx"), gName)==KErrNotFound);
	aFindHandle=0;
	GetObjName(gName1,*pObj1);
	test(pCon->FindByName(aFindHandle, gName1, gName)==KErrNone);
	test(aFindHandle==0);
	aFindHandle=2;
	GetObjName(gName1,*pObj1);
	test(pCon->FindByName(aFindHandle, gName1, gName)==KErrNotFound);
	aFindHandle=0;
	GetObjName(gName1,*pObj2);
	test(pCon->FindByName(aFindHandle, gName1, gName)==KErrNone);
	test(aFindHandle==1);

	aFindHandle=0;
	test(pCon->FindByFullName(aFindHandle, _L("xxx"), gFullName)==KErrNotFound);
	aFindHandle=0;
	GetObjName(gName1,*pObj1);
	test(pCon->FindByFullName(aFindHandle, gName1, gFullName)==KErrNone);
	test(aFindHandle==0);
	GetObjName(gName1,*pObj2);
	test(pCon->FindByFullName(aFindHandle, gName1, gFullName)==KErrNone);
	test(aFindHandle==1);

	pObj1->SetOwner(pObj3);
	pObj2->SetOwner(pObj3);
	aFindHandle=0;
	test(pCon->FindByFullName(aFindHandle, _L("xxx"), gFullName)==KErrNotFound);
	aFindHandle=0;
	GetObjFullName(gFullName1,*pObj1);
	test(pCon->FindByFullName(aFindHandle, gFullName1, gFullName)==KErrNone);
	test(aFindHandle==0);
	GetObjFullName(gFullName1,*pObj2);
	test(pCon->FindByFullName(aFindHandle, gFullName1, gFullName)==KErrNone);
	test(aFindHandle==1);


	test(pCon->CheckUniqueFullName(pObj3, _L("aname"))==KErrNone);
	GetObjName(gName1,*pObj1);
	test(pCon->CheckUniqueFullName(pObj3, gName1)==KErrAlreadyExists);
	pCon->Remove(pObj1);
	test(pCon->CheckUniqueFullName(pObj3, gName1)==KErrNone);
	test(pCon->Count()==1);
	pCon->AddL(pObj3);
	test(pCon->Count()==2);
	aFindHandle=0;
	test(pCon->FindByName(aFindHandle, gName1, gName)==KErrNotFound);
	aFindHandle=0;
	GetObjFullName(gFullName1,*pObj1);
	test(pCon->FindByFullName(aFindHandle, gFullName1, gFullName)==KErrNotFound);
	aFindHandle=0;
	GetObjName(gName1,*pObj2);
	test(pCon->FindByName(aFindHandle, gName1, gName)==KErrNone);
	test(aFindHandle==0);
	GetObjFullName(gFullName1,*pObj2);
	test(pCon->FindByFullName(aFindHandle, gFullName1, gFullName)==KErrNone);
	test(aFindHandle==0);
	GetObjName(gName1,*pObj3);
	test(pCon->FindByName(aFindHandle, gName1, gName)==KErrNone);
	test(aFindHandle==1);
	aFindHandle=0;
	test(pCon->FindByFullName(aFindHandle, _L("Owner"), gFullName)==KErrNone);
	test(aFindHandle==1);


	pObj2->__DbgTest(&dump1);
	temp=pCon->At(0);
	temp->__DbgTest(&dump2);
	test(dump1.iAccessCount==dump2.iAccessCount  && dump1.iOwner==dump2.iOwner && 
			dump1.iName==dump2.iName &&dump1.iContainer==dump2.iContainer); 
	pObj3->__DbgTest(&dump1);
	temp=pCon->At(1);
	temp->__DbgTest(&dump2);
	test(dump1.iAccessCount==dump2.iAccessCount  && dump1.iOwner==dump2.iOwner && 
			dump1.iName==dump2.iName &&dump1.iContainer==dump2.iContainer); 

	pCon->Remove(pObj2);
	pCon->Remove(pObj3);
	test(pCon->Count()==0);
	delete pCon;

	// Test expansion and shrinking
	__UHEAP_MARK;
	pCon=CObjectCon::NewL();
	CObject** obj = new CObject*[2048];
	test(obj!=0);
	TInt i;
	for (i=0; i<2048; ++i)
		{
		obj[i] = new CObject;
		test(obj[i] != 0);
		}

	RTestHeap* h = RTestHeap::Install();

	const TInt xov_values[] = {0,8,12,16,24,32,48,64,96,128,192,256,384,512,768,1024,1536,2048};
	const TInt xov2_values[] = {1472, 960, 704, 448, 320, 192, 128, 96, 64, 48, 32, 24, 16, 12, 8, 6, 0};
	const TInt* xov_ptr = xov_values;
	const TInt* xov2_ptr = xov2_values;
	TInt sz = 0;
	TInt xov = 0;
	TAny* ptr = 0;
	for (i=1; i<=2048; ++i)
		{
		pCon->AddL(obj[i-1]);
		test(pCon->Count()==i);
		RTestHeap::TOpInfo opi;
		TInt r = h->GetLastOp(opi);
		if (i*4 <= sz)
			{
			test(r==KErrNotFound);
			continue;
			}
		test(r==KErrNone);
		test.Printf(_L("Realloc at %d -> %d\n"), i, opi.iIntArg);
		test(i-1 == xov);
		test(opi.iOp == RTestHeap::EOpReAlloc);
		test(opi.iPtrArg == ptr);
		// recalc xov
		xov = *++xov_ptr;
		test(opi.iIntArg == xov*4);
		sz = xov*4;
		test(opi.iResult != 0);
		ptr = opi.iResult;
		}

	xov = *xov2_ptr;
	for (i=2047; i>=0; --i)
		{
		pCon->Remove(obj[i]);
		test(pCon->Count()==i);
		RTestHeap::TOpInfo opi;
		TInt r = h->GetLastOp(opi);
		if (i>xov)
			{
			test(r==KErrNotFound);
			continue;
			}
		test(r==KErrNone);
		test.Printf(_L("Realloc at %d -> %d\n"), i, opi.iIntArg);
		test(i == xov);
		if (i==0)
			{
			test(opi.iOp == RTestHeap::EOpReAlloc || opi.iOp == RTestHeap::EOpFree);
			}
		else
			{
			test(opi.iOp = RTestHeap::EOpReAlloc);
			test(opi.iResult == ptr);
			}
		test(opi.iPtrArg == ptr);
		// recalc xov
		xov = *++xov2_ptr;
		sz = *--xov_ptr;
		test(opi.iIntArg == sz*4);
		}

	delete pCon;
	for (i=0; i<2048; ++i)
		obj[i]->Close();
	delete[] obj;
	h->Uninstall();
	__UHEAP_MARKEND;
	}


GLDEF_C void TestCObjects::Test7(void)
	{
	// Test the methods of CObjectIx  

	// Before an object can be added to a ConIx it first has to be added to a container because the
	// CObjectIx::Add method references theObject->iContainer->iName. But theObject->iContainer field
	// is only set (within the CObjectCon::Add method) IF the container has first been created by the
	// CObjectConIx::Create method, otherwise it is NULL and adding it to a CObjectIx will fail.
	CObjectIx		*pIx=CObjectIx::NewL();
	CObjectConIx 	*pConIx=CObjectConIx::NewL();
	CObjectCon 		*pCon1=pConIx->CreateL(),
					*pCon2=pConIx->CreateL();
	CObject 		*pObj1=new CObject, 
					*pObj2=new CObject, 
					*pObj3=new CObject,
					*temp;
	TInt aHandle1, aHandle2, aHandle3;
	TCObjectDump dump1, dump2;	  
	TPtrC name1(_S("Obj1")), name2(_S("Obj2")), name3(_S("Obj3"));

	// Create two containers with Obj1 and Obj2 in the first and Obj3 in the second
	pObj1->SetName(&name1);
	pObj2->SetName(&name2);
	pObj3->SetName(&name3);			   
	pCon1->AddL(pObj1);
	pCon1->AddL(pObj2);
	pCon2->AddL(pObj3);
	aHandle1=pIx->AddL(pObj1);
	aHandle2=pIx->AddL(pObj2);
	aHandle3=pIx->AddL(pObj3); 
	// At(TInt aHandle)
	pObj1->__DbgTest(&dump1);
	temp=pIx->At(aHandle1);	
	temp->__DbgTest(&dump2);
	test(dump1.iAccessCount==dump2.iAccessCount && dump1.iOwner==dump2.iOwner &&
		 dump1.iContainer==dump2.iContainer && dump1.iName==dump2.iName); 
	pObj2->__DbgTest(&dump1);
	temp=pIx->At(aHandle2);
	temp->__DbgTest(&dump2);
	test(dump1.iAccessCount==dump2.iAccessCount && dump1.iOwner==dump2.iOwner &&
		 dump1.iContainer==dump2.iContainer && dump1.iName==dump2.iName); 
	pObj3->__DbgTest(&dump1);
	temp=pIx->At(aHandle3);
	temp->__DbgTest(&dump2);
	test(dump1.iAccessCount==dump2.iAccessCount && dump1.iOwner==dump2.iOwner &&
		 dump1.iContainer==dump2.iContainer && dump1.iName==dump2.iName);
	// At(TInt aHandle, TInt aUniqueID);
	pObj1->__DbgTest(&dump1);
	temp=pIx->At(aHandle1, 1);	
	temp->__DbgTest(&dump2);
	test(dump1.iAccessCount==dump2.iAccessCount && dump1.iOwner==dump2.iOwner &&
		 dump1.iContainer==dump2.iContainer && dump1.iName==dump2.iName); 
	pObj2->__DbgTest(&dump1);
	temp=pIx->At(aHandle2, 1);
	temp->__DbgTest(&dump2);
	test(dump1.iAccessCount==dump2.iAccessCount && dump1.iOwner==dump2.iOwner &&
		 dump1.iContainer==dump2.iContainer && dump1.iName==dump2.iName); 
	pObj3->__DbgTest(&dump1);
	temp=pIx->At(aHandle3, 2);
	temp->__DbgTest(&dump2);
	test(dump1.iAccessCount==dump2.iAccessCount && dump1.iOwner==dump2.iOwner &&
		 dump1.iContainer==dump2.iContainer && dump1.iName==dump2.iName);
	// Remove(Tint aHandle)
	pIx->Remove(aHandle2);
	pObj1->__DbgTest(&dump1);
	temp=pIx->At(aHandle1);
	temp->__DbgTest(&dump2);
	test(dump1.iAccessCount==dump2.iAccessCount && dump1.iOwner==dump2.iOwner &&
		 dump1.iContainer==dump2.iContainer && dump1.iName==dump2.iName); 
	pObj3->__DbgTest(&dump1);
	temp=pIx->At(aHandle3);
	temp->__DbgTest(&dump2);
	test(dump1.iAccessCount==dump2.iAccessCount && dump1.iOwner==dump2.iOwner &&
		 dump1.iContainer==dump2.iContainer && dump1.iName==dump2.iName);
	pIx->Remove(aHandle1);
	pIx->Remove(aHandle3);
	// pIx->Remove(aHandle3); this will cause a crash

	// Removing an object from a CObjectIx calls the objects close method hence in this case
	// deleting it
	//delete pCon1;	// The destructor for a container deletes its contained objects
	//delete pCon2;	
	delete pConIx; // The CObjectConIx destructor deletes its containers
	delete pIx; // The CObjectIx destructor calls close on all its objects
	}

GLDEF_C void TestCObjects::Test8(void)
	{
	// Test all objects together
	CObjectIx		*pIx=CObjectIx::NewL();
	CObjectConIx 	*pConIx=CObjectConIx::NewL();
	CObjectCon 		*pCon1=pConIx->CreateL(),
					*pCon2=pConIx->CreateL();
	CObject 		*pObj1=new CObject, 
					*pObj2=new CObject, 
					*pObj3=new CObject,
					*pObj4=new CObject,
					*pObj5=new CObject,
					*temp;
	TInt tempHandle;
	TPtrC name1(_S("Obj1")), name2(_S("Obj2")), name3(_S("Obj3")), name4(_S("Obj4")), name5(_S("Obj5"));

	// Create two containers with Obj1,Obj2, Obj3 in the first and Obj4 and Obj5 in the second
	// Obj1 owns Obj2 which owns Obj3
	pObj1->SetName(&name1);
	pObj2->SetName(&name2);
	pObj2->SetOwner(pObj1);
	pObj3->SetName(&name3);
	pObj3->SetOwner(pObj2);	
	pObj4->SetName(&name4);		   
	pObj5->SetName(&name5);
	pCon1->AddL(pObj1);
	pCon1->AddL(pObj2);
	pCon1->AddL(pObj3);
	pCon2->AddL(pObj4);
	pCon2->AddL(pObj5);
	pIx->AddL(pObj1);
	pIx->AddL(pObj2);
	pIx->AddL(pObj3); 
	pIx->AddL(pObj4);
	pIx->AddL(pObj5);


	tempHandle=0;
	GetObjName(gName1,*pObj1);
	pCon1->FindByName(tempHandle, gName1, gName);
	temp=pCon1->At(tempHandle);
	GetObjName(gName2,*temp);
	test(gName2==gName1);
	tempHandle=0;
	GetObjFullName(gFullName1,*pObj1);
	pCon1->FindByFullName(tempHandle, gFullName1, gFullName);
	temp=pCon1->At(tempHandle);
	GetObjName(gName2,*temp);
	test(gName2==gName1);

	tempHandle=0;
	GetObjName(gName1,*pObj2);
	pCon1->FindByName(tempHandle, gName1, gName);
	temp=pCon1->At(tempHandle);
	GetObjName(gName2,*temp);
	test(gName2==gName1);
	tempHandle=0;
	GetObjFullName(gFullName1,*pObj2);
	pCon1->FindByFullName(tempHandle, gFullName1, gFullName);
	temp=pCon1->At(tempHandle);
	GetObjName(gName2,*temp);
	test(gName2==gName1);

	tempHandle=0;
	GetObjName(gName1,*pObj3);
	pCon1->FindByName(tempHandle, gName1, gName);
	temp=pCon1->At(tempHandle);
	GetObjName(gName2,*temp);
	test(gName2==gName1);
	tempHandle=0;
	GetObjFullName(gFullName1,*pObj3);
	pCon1->FindByFullName(tempHandle, gFullName1, gFullName);
	temp=pCon1->At(tempHandle);
	GetObjName(gName2,*temp);
	test(gName2==gName1);

	tempHandle=0;
	GetObjName(gName1,*pObj4);
	pCon2->FindByName(tempHandle, gName1, gName);
	temp=pCon2->At(tempHandle);
	GetObjName(gName2,*temp);
	test(gName2==gName1);
	tempHandle=0;
	GetObjFullName(gFullName1,*pObj4);
	pCon2->FindByFullName(tempHandle, gFullName1, gFullName);
	temp=pCon2->At(tempHandle);
	GetObjName(gName2,*temp);
	test(gName1==gName2);

	tempHandle=0;
	GetObjName(gName1,*pObj5);
	pCon2->FindByName(tempHandle, gName1, gName);
	temp=pCon2->At(tempHandle);
    GetObjName(gName2,*temp);
	test(gName2==gName1);
	tempHandle=0;
	GetObjFullName(gFullName1,*pObj5);
	pCon2->FindByFullName(tempHandle, gFullName1, gFullName);
	temp=pCon2->At(tempHandle);
    GetObjName(gName2,*temp);
	test(gName2==gName1);

		
	tempHandle=0;
	test(pCon1->FindByName(tempHandle, _L("*1"), gName)==KErrNone);
	GetObjName(gName1,*pObj1);
	test(gName==gName1);
	tempHandle=0;
	test(pCon1->FindByFullName(tempHandle, _L("*::*"), gFullName)==KErrNone);
	GetObjFullName(gFullName1,*pObj2);
	test(gFullName==gFullName1);
    GetObjName(gName1,*pObj3);
	for(tempHandle=0;gName!=gName1;pCon1->FindByName(tempHandle, _L("????"),gName)) {};
	test(gName==gName1);


	pObj1->Close();  // Deletes it and removes it from its container
	pObj3->Close();
	tempHandle=0;
	test(pCon1->FindByName(tempHandle, _L("Obj1"), gName)==KErrNotFound);
	tempHandle=0;
	test(pCon1->FindByName(tempHandle, _L("Obj3"), gName)==KErrNotFound);
	tempHandle=0;
	GetObjName(gName1,*pObj2);
	test(pCon1->FindByName(tempHandle, gName1, gName)==KErrNone);
	test(gName==gName1);
	tempHandle=0;
	//CObject* x=new CObject;
	//x->SetName(_L("xxxx"));
	//pCon1->FindByFullName(tempHandle, pObj2->FullName(), aFullName);
	// FullName is set to Xxxx::Obj2

	}


TInt PanicCObjectConIndexOutOfRangeFn(TAny* aCObjectCon)
	{	
	CObjectCon* pCObjectCon=(CObjectCon*)aCObjectCon;
	(*pCObjectCon)[1]; // no objects added to the container
	return KErrNone; // should not come here
	}

TInt PanicCObjectConFindIndexOutOfRangeFn(TAny* aCObjectCon)
	{
	CObjectCon* pCObjectCon=(CObjectCon*)aCObjectCon;
	TInt aFindHandle=1;
	pCObjectCon->At(aFindHandle);
	return KErrNone; // should not come here
	}

TInt PanicCObjectConFindBadHandleFn(TAny* aCObjectCon)
	{	
	CObjectCon* pCObjectCon=(CObjectCon*)aCObjectCon;
	TInt aFindHandle=KMaxTInt;
	pCObjectCon->At(aFindHandle);
	return KErrNone; // should not come here
	}

TInt PanicCObjectIxIndexOutOfRangeFn(TAny* aCObjectIx)
	{	
	CObjectIx* pCObjectIx=(CObjectIx*)aCObjectIx;
	(*pCObjectIx)[1]; // no objects added to the container
	return KErrNone; // should not come here
	}
	
void StartPanicTest(TInt aPanicType)
	{
	CObjectCon* pCObjectCon=CObjectCon::NewL();
	CObjectIx* pCObjectIx=CObjectIx::NewL();
	RThread thread;
	TRequestStatus status;
	TInt r=KErrNone;

	switch (aPanicType)
		{
		case 0:		// this index used for (PanicCObjectIxIndexOutOfRange) CObjectIx index out of range
			r=thread.Create(_L("PanicCObjectIxIndexOutOfRangeThread"),PanicCObjectIxIndexOutOfRangeFn,KDefaultStackSize,NULL,(TAny*)pCObjectIx);
			break;
		case EObjFindBadHandle:			// for testing CObjectCon panic (PanicCObjectConFindBadHandle)
			r=thread.Create(_L("PanicCObjectConFindBadHandleThread"),PanicCObjectConFindBadHandleFn,KDefaultStackSize,NULL,(TAny*)pCObjectCon);
			break;
		case EObjFindIndexOutOfRange:	// for testing CObjectCon panic (PanicCObjectConFindIndexOutOfRange)
			r=thread.Create(_L("PanicCObjectConFindIndexOutOfRangeThread"),PanicCObjectConFindIndexOutOfRangeFn,KDefaultStackSize,NULL,(TAny*)pCObjectCon);
			break;
		case EArrayIndexOutOfRange:		// for testing CObjectCon panic (PanicCObjectConIndexOutOfRange)
			r=thread.Create(_L("PanicCObjectConIndexOutOfRangeThread"),PanicCObjectConIndexOutOfRangeFn,KDefaultStackSize,NULL,(TAny*)pCObjectCon);
			break;
		default:
			break;
		}

	test (r==KErrNone);
	thread.SetPriority(EPriorityMore);
	thread.Logon(status);
	thread.Resume();
	User::WaitForRequest(status);

	test(status.Int() != KErrNone);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitCategory()==_L("E32USER-CBase"));

	switch (aPanicType)
		{
		case 0:		// this index used for (PanicCObjectIxIndexOutOfRange) CObjectIx index out of range
			test(thread.ExitReason()==EArrayIndexOutOfRange);
			break;
		case EObjFindBadHandle:			// for testing CObjectCon panic (PanicCObjectConFindBadHandle)
			test(thread.ExitReason()==EObjFindBadHandle);
			break;
		case EObjFindIndexOutOfRange:	// for testing CObjectCon panic (PanicCObjectConFindIndexOutOfRange)
			test(thread.ExitReason()==EObjFindIndexOutOfRange);
			break;
		case EArrayIndexOutOfRange:		// for testing CObjectCon panic (PanicCObjectConIndexOutOfRange)
			test(thread.ExitReason()==EArrayIndexOutOfRange);
			break;
		default:
			break;
		}
	
	CLOSE_AND_WAIT(thread);
	delete pCObjectCon;
	delete pCObjectIx;
	}


GLDEF_C void TestCObjects::Test9(void)
	{
	// Disable JIT debugging.
	TBool justInTime=User::JustInTime();
	User::SetJustInTime(EFalse);

	test.Next(_L("test PanicCObjectConFindBadHandle"));
	StartPanicTest(EObjFindBadHandle);
	test.Next(_L("test PanicCObjectConFindIndexOutOfRange"));
	StartPanicTest(EObjFindIndexOutOfRange);
	test.Next(_L("test PanicCObjectConIndexOutOfRange"));
	StartPanicTest(EArrayIndexOutOfRange);
	test.Next(_L("test PanicCObjectIxIndexOutOfRange"));
	StartPanicTest(0);

	// Put JIT debugging back to previous status.
	User::SetJustInTime(justInTime);

	test.End();
	}


GLDEF_C void TestCObjects::GetObjName(TName& aDest,const CObject& aObj)
//
// Utility function to reduce stack usage in functions, and so get rid of __chkstk errors
//
	{
	aDest=aObj.Name();
	}

GLDEF_C void TestCObjects::GetObjFullName(TFullName& aDest,const CObject& aObj)
//
// Utility function to reduce stack usage in functions, and so get rid of __chkstk errors
//
	{
	aDest=aObj.FullName();
	}

void testHandles()

	{

	CObjectConIx* myConIx=CObjectConIx::NewL();
	CObjectCon* myCon= myConIx->CreateL();
	CObjectIx* myIx=CObjectIx::NewL();
	test(myCon->UniqueID()!=0);

	for (TInt i=0;i<0x4006; i++)
		{
		CObject* myObj=new CObject;
		myCon->AddL(myObj);
		TInt handle=myIx->AddL(myObj);
		if ((handle&0xffff0000)==0x3fff0000)
			test.Printf(_L("Created biggest handle\n"));
		test (handle!=0);
		myIx->Remove(handle);
		}
	delete myIx;
	delete myConIx;
	}

void testSpeed()

	{

	const TInt numObjects=0x600;
	CObjectConIx* myConIx=CObjectConIx::NewL();
	CObjectCon* myCon= myConIx->CreateL();
//	CObjectIx* myIx=CObjectIx::NewL();
	TTime start;
	TTime end;
	TInt64 diff;
	TInt i;

	start.HomeTime();
	for (i=0;i<numObjects; i++)
		{
		CObject* myObj=new CObject;
		TName name;
		name.Format(_L("CObject%d"),i);
		myObj->SetName(&name);
		myCon->AddL(myObj);
		}
	end.HomeTime();
	diff=(end.Int64()-start.Int64())/100000;
	test.Printf(_L("Time for 0x%08x named objects = %d tenths of secs\n"),numObjects,I64INT(diff));

// If run as a manual test (when enhancements to RTEST arrive)
//	test.Printf(_L("Press a key to continue with these spammy tests\n"));
//	test.Getch();

	const TInt numObjects2=0x600;
//	CObjectConIx* myConIx2=CObjectConIx::NewL();
	CObjectCon* myCon2= myConIx->CreateL();
//	CObjectIx* myIx2=CObjectIx::NewL();

	start.HomeTime();
	for (i=0;i<numObjects2; i++)
		{
		CObject* myObj=new CObject;
		TName name;
		name.Format(_L("CObject%d"),i);
		myCon2->AddL(myObj);
		}
	end.HomeTime();
	diff=(end.Int64()-start.Int64())/100000;
	test.Printf(_L("Time for 0x%08x unnamed objects = %d tenths of secs\n"),numObjects2,I64INT(diff));


// If run as a manual test (when enhancements to RTEST arrive)
//  test.Printf(_L("Press a key to continue with these spammy tests\n"));
//  test.Getch();
	
	//	delete myIx;
//	delete myConIx;
	}



const TInt KObjectIndexMask=0x7fff;

//Tests CObjectIx class
class TestCObjectIx
	{
public:
	void StartL(void);
	~TestCObjectIx();
private:
	void Test1(TInt aSize);
	void Test2(TInt aSize);
	void Test3(TInt aSize, TBool aPerformanceTest);
	inline TInt Index(TInt aHandle)	{return(aHandle&KObjectIndexMask);}

private:
	struct COBjAndHandle
		{
		CObject* iObject;
		TInt	 iHandle;
		};

	CObjectConIx* iMyConIx;
	CObjectCon*   iMyCon;
	CObjectIx*    iMyIx;
	TUint iSeed[2];
	COBjAndHandle* iObjAndHandle;	
	};

const TInt KMaxTestObjects = (1<<11);

//Runs all tests (methods) of the class
void TestCObjectIx::StartL(void)
{
	TInt i;
	iMyConIx=CObjectConIx::NewL();
	iMyCon= iMyConIx->CreateL();
	iObjAndHandle = (COBjAndHandle*) User::AllocL(KMaxTestObjects * sizeof(COBjAndHandle));

	test.Title();
	test.Start(_L("Test CObjectIx"));

	test.Next(_L("Add and remove objects in the same order..."));
	TUint32 startTime = User::NTickCount();
	for (i=1;i<2000;i=i+10) Test1(i);
	test.Printf(_L("...done in %d msec\n"), User::NTickCount()-startTime);

	test.Next(_L("Add and remove objects in the reverse order..."));
	startTime = User::NTickCount();
	for (i=1;i<2000;i=i+10) Test2(i);
	test.Printf(_L("...done in %d msec\n"), User::NTickCount()-startTime);

	test.Next(_L("Add and remove objects in random order..."));
	iSeed[0]=User::TickCount();
	test.Printf(_L("The initial seed for the random function is:  S0=%xh S1=%xh\n"), iSeed[0], iSeed[1]);
	for (i=2;i<=KMaxTestObjects/2; i<<=1)
		Test3(i-1, EFalse);

	test.Next(_L("Add and remove objects in random order - performance test..."));
	iSeed[0]=0;
	iSeed[1]=1;
	startTime = User::NTickCount();
	for (i=2;i<=KMaxTestObjects; i<<=1)
		Test3(i-1, ETrue);
	test.Printf(_L("...done in %d msec\n"), User::NTickCount()-startTime);
	test.Printf(_L("CAUTION: Test changed in May 2005. Comparison of timings with those before then are not valid.\n"));
	
	test.End();
}

TestCObjectIx::~TestCObjectIx()
	{
	delete iMyConIx;
	delete [] iObjAndHandle;
	}

//Adds a number of CObjects to CObjectIx, then removes them in the same order.
void TestCObjectIx::Test1(TInt aSize)
{
	TInt i;	
	iMyIx=CObjectIx::NewL();

	for (i=0; i<aSize; i++) //Add
		{
		if( (iObjAndHandle[i].iObject = new CObject) == NULL) User::Leave(KErrNoMemory);
		iMyCon->AddL(iObjAndHandle[i].iObject);
		iObjAndHandle[i].iHandle = iMyIx->AddL(iObjAndHandle[i].iObject);
		}

	for (i=0; i<aSize; i++)		//Remove
		iMyIx->Remove(iObjAndHandle[i].iHandle);

	delete iMyIx;
	iMyIx=0;
}


//Adds a number of CObjects to CObjectIx, then removes them in the reverse order.
void TestCObjectIx::Test2(TInt aSize)
{
	TInt i;	
	iMyIx=CObjectIx::NewL();

	for (i=0; i<aSize; i++)		//Add
		{
		if( (iObjAndHandle[i].iObject = new CObject) == NULL) User::Leave(KErrNoMemory);
		iMyCon->AddL(iObjAndHandle[i].iObject);
		iObjAndHandle[i].iHandle = iMyIx->AddL(iObjAndHandle[i].iObject);
		}

	for (i=aSize-1; i>=aSize; i--)		//Remove
		iMyIx->Remove(iObjAndHandle[i].iHandle);

	delete iMyIx;
	iMyIx=0;
}


//Adds and removes random number of CObjects to/from CObjectIx.
void TestCObjectIx::Test3(TInt aSize, TBool aPerformanceTest)
{
	TInt index, x;
	if(!aPerformanceTest) 
		test.Printf(_L("Testing size %d.The seeds are:  S0=%xh S1=%xh\n"), aSize, iSeed[0], iSeed[1]);
	
	//---Create & init the objects we need
	for (x=0; x<aSize; x++) iObjAndHandle[x].iObject = NULL; //initialize the array
	iMyIx=CObjectIx::NewL();

	for (x = 0; x<100; x++)
		{
		
		//---Add the random number of objects (in random order)---
		TInt toAdd=Random(iSeed)%(aSize-iMyIx->ActiveCount()+1);
		//test.Printf(_L("Adding %d objects\n"), toAdd );
		while (toAdd--)
			{
			index=Random(iSeed)%aSize;
			while (iObjAndHandle[index].iObject) //Find the next NULL pointer 
				{  ++index; if(index>=aSize) index=0; }
			if( (iObjAndHandle[index].iObject = new CObject) == NULL) User::Leave(KErrNoMemory);
			iMyCon->AddL(iObjAndHandle[index].iObject);
			iObjAndHandle[index].iHandle = iMyIx->AddL(iObjAndHandle[index].iObject);
			//test.Printf(_L("%d(%d) "), index,iObjAndHandle[index].iHandle & 0x7fff  );
			}
		//test.Printf(_L("\n"));


		//---Remove the random number of objects (in random order)---
		TInt toRemove=Random(iSeed)%(iMyIx->ActiveCount()+1);
		//test.Printf(_L("Removing %d objects: "), toRemove );
		while (toRemove--)
			{
			index=Random(iSeed)%aSize;
			while (!iObjAndHandle[index].iObject) //Find the next non-NULL pointer 
				{  ++index; if(index>=aSize) index=0; }
			//test.Printf(_L("%d(%d) "), index,iObjAndHandle[index].iHandle & 0x7fff  );
			iMyIx->Remove(iObjAndHandle[index].iHandle);
			iObjAndHandle[index].iObject=NULL;
			}
		//test.Printf(_L("\n"));


		//---Test data consistency---
		if(aPerformanceTest) continue;

		TInt objNum=0;
		for (index=0;index<aSize;index++) 
			{
			if (iObjAndHandle[index].iObject)
				{
				objNum++;
				//Test At(TInt aHandle) method
				test(iObjAndHandle[index].iObject == iMyIx->At(iObjAndHandle[index].iHandle));
				//Test Count(CObject* aObject) method
				test(1==iMyIx->Count(iObjAndHandle[index].iObject));
				//Test At(CObject* aObject) method
				test(iObjAndHandle[index].iHandle==iMyIx->At(iObjAndHandle[index].iObject));
				//Test operator[](TInt index) method
				test(iObjAndHandle[index].iObject==(*iMyIx)[Index(iObjAndHandle[index].iHandle)]);
				}
			}
	
		test (objNum==iMyIx->ActiveCount());
		//test.Printf(_L("%d objects in array\n"), objNum);
		}

	delete iMyIx;
	iMyIx=NULL;
}

// Test that things work when the unique ID of the object container grows larger
// than 15 bits
void TestCObjectConIxL(void)
	{
	const TInt KCObjectConLimit = 65535;
	_LIT(KAnyMatch, "*");
	
   	__UHEAP_MARK;
	
	CObjectConIx* conIx = CObjectConIx::NewL();
	CObjectIx* ix = CObjectIx::NewL();
	TInt i;

	test.Next(_L("Test repeated add/remove of object containers"));
	for (i = 0 ; i < KCObjectConLimit * 2 ; ++i)
		{
		CObjectCon* con = conIx->CreateL();
		CObject* obj = new (ELeave) CObject();
		con->AddL(obj);
		
		TInt handle = ix->AddL(obj);
		test(ix->At(handle) == obj);
		test(ix->AtL(handle) == obj);
		test(ix->At(handle, con->UniqueID()) == obj);
		test(ix->AtL(handle, con->UniqueID()) == obj);

		TName name;
		TInt findHandle = 0;
		test(con->FindByName(findHandle, KAnyMatch, name) == KErrNone);
		test(con->AtL(findHandle) == obj);
		test(con->At(findHandle) == obj);
		test(conIx->Lookup(findHandle) == con);
		test(con->FindByName(findHandle, KAnyMatch, name) == KErrNotFound);

		TFullName fullName;
		findHandle = 0;
		test(con->FindByFullName(findHandle, KAnyMatch, fullName) == KErrNone);
		test(con->AtL(findHandle) == obj);
		test(con->At(findHandle) == obj);
		test(conIx->Lookup(findHandle) == con);
		test(con->FindByFullName(findHandle, KAnyMatch, fullName) == KErrNotFound);
		
		ix->Remove(handle);
		conIx->Remove(con);
		}
	
	test.Next(_L("Test adding maximum possible number of object containers"));
	RPointerArray<CObjectCon> cons;
	for (i = 0 ; i < KCObjectConLimit ; ++i)
		{
		CObjectCon* con = conIx->CreateL();
		cons.AppendL(con);
		}
	TRAPD(err, conIx->CreateL());
	test(err == KErrOverflow);
	
	test.Next(_L("Test unique IDs are really unique after ID value has wrapped"));
	for (i = 100 ; i < KCObjectConLimit ; ++i)
		{
		CObjectCon* con = cons[i];
		conIx->Remove(con);		
		}
	for (i = 100 ; i < 200 ; ++i)
		{
		CObjectCon* con = conIx->CreateL();
		cons[i] = con;
		}
	for (i = 0 ; i < 200 ; ++i)
		{
		TName name;
		TInt findHandle = 0;
		CObjectCon* con = cons[i];

		CObject* obj = new (ELeave) CObject();
		con->AddL(obj);

		test(con->FindByName(findHandle, KAnyMatch, name) == KErrNone);
		test(conIx->Lookup(findHandle) == con);

		obj->Close();
		}
	for (i = 0 ; i < 200 ; ++i)
		{
		CObjectCon* con = cons[i];
		conIx->Remove(con);
		}
	cons.Close();

	delete ix;
	delete conIx;
	
   	__UHEAP_MARKEND;
	}

GLDEF_C TInt E32Main()
    {

   	CTrapCleanup* trapHandler=CTrapCleanup::New();
   	test(trapHandler!=NULL);

	test.Title();
	test.Start(_L("TEST METHODS ARE IN THE DLL"));

   	__UHEAP_MARK;

	TestCObjectIx* TCobjectIx = new TestCObjectIx;
   	TRAPD(ret, TCobjectIx->StartL()); 
	test(KErrNone == ret);
	delete TCobjectIx;

   	__UHEAP_MARKEND;

	test.Next(_L("Generate lots of handles"));
	testHandles();

	test.Next(_L("Test CObjectCon is fast enough"));
	testSpeed();

	TestCObjects T;
	test.Next(_L("CObject methods"));
	T.Test1();
	test.Next(_L("CObjectCon methods"));
	T.Test2();
	test.Next(_L("CObjectIx methods"));
	T.Test3();
	test.Next(_L("CObjectConIx methods"));
	T.Test4();
	test.Next(_L("TEST THE METHODS"));
	test.Next(_L("CObject"));
	T.Test5();
	test.Next(_L("CObjectCon"));
	T.Test6();
	test.Next(_L("CObjectIx"));
	T.Test7();

	//////////////////////////////
	// PROPER TESTING STARTS HERE
	//////////////////////////////
	test.Next(_L("All objects"));
	T.Test8();
	
	test.Next(_L("CObjectConIx"));
	TRAPD(err, TestCObjectConIxL());
	if (err != KErrNone)
		test.Printf(_L("TestCObjectConIxL left with %d\n"), err);
	test(err == KErrNone);

	//Test Panics
	test.Start(_L("Test Panic functions"));
	T.Test9();

	test.End();

   	delete trapHandler;
   	return(KErrNone);
	}
