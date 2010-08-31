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
// e32test\system\t_ctrap.cpp
// Overview:
// Test the CCleanup, CTrapCleanup and TAutoClose classes
// API Information:
// CCleanup, CTrapCleanup, TAutoClose
// Details:
// - Test cleanup stack reallocation during cleanup.
// - Test cleanup stack modifications during the cleanup operation  
// will cause a panic.
// - Test single-level cleanup of cells, objects, items and a mix: 
// Create a CCleanup object, call a combination of methods, verify 
// the results are as expected and verify the heap has not been 
// corrupted.
// - Test multi-level cleanup of cells, objects, items and a mix: 
// Create a CCleanup object, call a combination of methods, verify 
// the results are as expected and verify the heap has not been 
// corrupted.
// - Test a variety of special case cleanup tasks. Verify that the 
// results are as expected.
// - Test CTrapCleanup cleanup of objects that either exit normally
// or leave. Also test the cleanup of multiple objects that leave.
// Verify results are as expected.
// - Test TAutoClose: create a TAutoClose object, verify that it is 
// closed when it goes out of scope, push it on the cleanup stack,
// verify cleanup results are as expected.
// - Test that the Cleanup stack can go re-entrant.
// - Ensure that the stack is properly balanced with and without
// leaving.
// - Test creating cleanup with CCleanup::NewL in normal
// memory conditions and condition where heap is full (panic)
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32panic.h>
#include <e32debug.h>
#include <e32def.h>
#include <e32def_private.h>

#if defined(_DEBUG)
	const TInt KInitialCount=2;
	const TInt KInitialCountAll=3;
#endif

const TInt KLeaveValue=0x12345678;
const TInt KMaxAlloc=6;	
const TInt KTableSize = 1000;

static const TInt KHeapSize = 0x2000;


enum TWhat {EPop,EPopAndDestroy,EMulti,ENull};

class CTest : public CBase
	{
public:
	void ConstructL();
private:
	TInt iData;
	};
	
class CTest2: public CBase 
	{
public:
	~CTest2();
	};

class CTest3: public CBase 
	{
public:
	~CTest3();
	};

class RItem
	{
public:
	RItem() : iOpen(EFalse) {}
	void Open() {iOpen=ETrue;}
	void Close() {iOpen=EFalse;}
	operator TCleanupItem() {return TCleanupItem(Cleanup,this);}
	TBool IsOpen() const {return(iOpen);}
private:
	static void Cleanup(TAny* aPtr);
private:
	TBool iOpen;
	};

LOCAL_D RTest test(_L("T_CTRAP"));
LOCAL_D TAny* gP1;
LOCAL_D CBufFlat* gP2;


LOCAL_C void ReallocateStackL()
	{
	for(TInt i = 0; i < KMaxAlloc; ++i)
		{
		(void)HBufC::NewLC(4);   //Stack re-allocation will be performed due to the additional objects pushed
									   //into the cleanup stack
		}
	test.Printf(_L("ReallocateStackL(): PopAndDestroy KMaxAlloc pointers\n"));
	CleanupStack::PopAndDestroy(KMaxAlloc);
	}

CTest2::~CTest2()
	{
	TInt err = KErrNoMemory;
	
	test.Printf(_L("~CTest2(): call ReallocateStackL()\n"));
	
	TRAP(err, ReallocateStackL() );
	}

CTest3::~CTest3()
	{
	RDebug::Printf("~CTest3(): Modify Cleanup stack by pushing items");
	
	for(TInt i = 0; i < KMaxAlloc; ++i)
		{
		HBufC::NewLC(4);   //Stack re-allocation will be performed due to the additional objects pushed
									   //into the cleanup stack
		}
	}

LOCAL_C void ModifyStack()
	{
	CTest3* ptr6 = new(ELeave)CTest3;
	CleanupStack::PushL(ptr6);

	RDebug::Printf("ModifyStack(): PopAndDestroy ptr6");
	CleanupStack::PopAndDestroy();
	}

LOCAL_C TInt PanicStackModifiedFn(TAny* /*aNopFn*/)
	{
	__UHEAP_MARK;
	CTrapCleanup* cleanup = CTrapCleanup::New();

	TInt err = KErrNoMemory;

	RDebug::Printf("PanicStackModifiedFn(): call TRAP(err, ModifyStack())");

	if(NULL != cleanup)
		{
		TRAP(err, ModifyStack());
		delete cleanup;
		}
	__UHEAP_MARKEND;
	return err;	
	}

LOCAL_C void PushAndCleanupL()
	{
	CTest2* ptr1 = new(ELeave)CTest2;
	CleanupStack::PushL(ptr1);
	
	CTest2* ptr2 = new(ELeave)CTest2;
	CleanupStack::PushL(ptr2);
	
	CTest2* ptr3 = new(ELeave)CTest2;
	CleanupStack::PushL(ptr3);

	test.Printf(_L("PushAndCleanupL(): PopAndDestroy ptr3, ptr2 and ptr1\n"));
	CleanupStack::PopAndDestroy(3);

	CTest2* ptr4 = new(ELeave)CTest2;
	CleanupStack::PushL(ptr4);
	
	CTest2* ptr5 = new(ELeave)CTest2;
	CleanupStack::PushL(ptr5);

	test.Printf(_L("PushAndCleanupL(): PopAndDestroy ptr5 and ptr4\n"));
	CleanupStack::PopAndDestroy();
	CleanupStack::PopAndDestroy();
	}

LOCAL_C void testDestructorStackReallocation()
	{
	__UHEAP_MARK;
	CTrapCleanup* cleanup = CTrapCleanup::New();
	
	TInt err = KErrNoMemory;
	
	if(NULL != cleanup)
		{
		TRAP(err, PushAndCleanupL());
		delete cleanup;
		}
	__UHEAP_MARKEND;

	test_KErrNone(err);

	test.Printf(_L("Verify cleanup stack modification during cleanup operation causes EClnStackModified panic\n"));
	
	//
	//To verify the above case a new thread is created which does modify the cleanup stack during cleanup.
	//The exit reason is then checked for the appropriate value(EClnStackModified)
	//

	RThread panicThread;

	TInt r = panicThread.Create(_L("Panic EClnStackModified Thread"), PanicStackModifiedFn, KDefaultStackSize, KHeapSize, KHeapSize, NULL);

	test_KErrNone(r);

	TRequestStatus panicThreadStatus;
	panicThread.Logon(panicThreadStatus);

	//don't want just in time debugging as we trap panics
	TBool justInTime=User::JustInTime(); 
	User::SetJustInTime(EFalse); 

	panicThread.Resume();

	User::WaitForRequest(panicThreadStatus);

	test_Equal(EExitPanic, panicThread.ExitType());
	test_Equal(EClnStackModified, panicThread.ExitReason());

	User::SetJustInTime(justInTime);

	CLOSE_AND_WAIT(panicThread);
	}
	
LOCAL_C void createMultiL()
//
// Create an object on the cleanup list and leave
//
	{

	CBufFlat* pT=CBufFlat::NewL(8);
	User::LeaveIfNull(pT);
	CleanupStack::PushL(pT);
	__UHEAP_CHECK(3);
	User::Leave(KLeaveValue+1);
	}

LOCAL_C void createL(TWhat aWhat,TBool aLeave)
//
// Create objects and then either leave or return.
// Optionally pop them again.
//
	{

	gP1=User::AllocL(0x10);
    test.Printf(_L("createL 1"));
	CleanupStack::PushL(gP1);
    test.Printf(_L("createL 2"));
	__UHEAP_CHECK(1);
    test.Printf(_L("createL 3"));
	gP2=CBufFlat::NewL(8);
    test.Printf(_L("createL 4"));
	User::LeaveIfNull(gP2);
    test.Printf(_L("createL 5"));
	CleanupStack::PushL(gP2);
    test.Printf(_L("createL 6"));
	__UHEAP_CHECK(2);
    test.Printf(_L("createL 7"));
	if (aWhat==EPop)
		{
		test.Printf(_L("createL 8"));
		CleanupStack::Pop();
		test.Printf(_L("createL 9"));
		CleanupStack::Pop(1);
		test.Printf(_L("createL 10"));
		}
	if (aWhat==EPopAndDestroy)
		{
		test.Printf(_L("createL 11"));
		CleanupStack::PopAndDestroy();
		test.Printf(_L("createL 12"));
		CleanupStack::PopAndDestroy(1);
		test.Printf(_L("createL 13"));
		}
	if (aWhat==EMulti)
		{
		test.Printf(_L("createL 14"));
		TRAPD(r,createMultiL())
		test.Printf(_L("createL 15"));
		test(r==(KLeaveValue+1));
		test.Printf(_L("createL 16"));
		__UHEAP_CHECK(2);
		test.Printf(_L("createL 17"));
		}
	if (aLeave)
		{
		test.Printf(_L("createL 18"));
		User::Leave(KLeaveValue);
		}
    test.Printf(_L("createL 19"));
	}

LOCAL_C void createAllL(TBool aLeave)
//
// Call all functions which autmatically put objects on the cleanup list.
//
	{

	__UHEAP_CHECK(KInitialCountAll);
	TLex* pL=new(ELeave) TLex; // ::new, 1 cell
	CleanupStack::PushL(pL);								// Push
	__UHEAP_CHECK(KInitialCountAll+1);
	CTest* pT=new(ELeave) CTest; // CBase::new, 1 cell
	CleanupStack::PushL(pT);								// Push
	__UHEAP_CHECK(KInitialCountAll+2);
	pT->ConstructL(); // 1 more cell						// Push
	__UHEAP_CHECK(KInitialCountAll+3);
	User::AllocLC(0x10); // Test RHeap::AllocLC as well		// Push
	__UHEAP_CHECK(KInitialCountAll+4);
	_L("Hello").AllocLC(); // Test HBufC::NewLC() as well	// Push
	__UHEAP_CHECK(KInitialCountAll+5);
	HBufC* pH=HBufC::NewMaxLC(8);							// Push
	test(pH->Length()==8);
	__UHEAP_CHECK(KInitialCountAll+6);
	if (aLeave)
		User::Leave(KLeaveValue);
	// new behavior for TCleanupTrapHander requires Pushes to the
	// cleanup stack to be balanced by Pops
	CleanupStack::PopAndDestroy(6);
	}

LOCAL_C void testSingleLevelCellsCleanup()
//
// Test single level cells cleanup
//
	{

	test.Start(_L("Creating"));
//
	__UHEAP_MARK;
	CCleanup* pC=CCleanup::New();
	test(pC!=NULL);
//
	__UHEAP_MARK;
//
	test.Next(_L("PopAll when empty"));
	pC->NextLevel();
	pC->PopAll();
	pC->NextLevel();
	pC->PopAndDestroyAll();
	__UHEAP_CHECK(0);
//
	test.Next(_L("Push and pop"));
	TAny* p=User::Alloc(0x10);
	test(p!=NULL);
	__UHEAP_CHECK(1);
	pC->NextLevel();
	pC->PushL(p);
	pC->Pop();
	__UHEAP_CHECK(1);
	User::Free(p);
	__UHEAP_CHECK(0);
	pC->PopAll();
	__UHEAP_CHECK(0);
//
	test.Next(_L("Push and pop N"));
	TAny* p1=User::Alloc(0x10);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	TAny* p2=User::Alloc(0x10);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(p2);
	pC->Pop(2);
	__UHEAP_CHECK(2);
	User::Free(p1);
	User::Free(p2);
	__UHEAP_CHECK(0);
	pC->PopAll();
	__UHEAP_CHECK(0);
//
	test.Next(_L("Push and pop all"));
	p1=User::Alloc(0x10);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	p2=User::Alloc(0x10);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	TAny* p3=User::Alloc(0x10);
	test(p3!=NULL);
	__UHEAP_CHECK(3);
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(p2);
	pC->PushL(p3);
	pC->PopAll();
	__UHEAP_CHECK(3);
	User::Free(p1);
	User::Free(p2);
	User::Free(p3);
	__UHEAP_CHECK(0);
//
	test.Next(_L("Push and pop and destroy"));
	p=User::Alloc(0x10);
	test(p!=NULL);
	__UHEAP_CHECK(1);
	pC->NextLevel();
	pC->PushL(p);
	pC->PopAndDestroy();
	__UHEAP_CHECK(0);
	pC->PopAll();
//
	test.Next(_L("Push and pop and destroy N"));
	p1=User::Alloc(0x10);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	p2=User::Alloc(0x10);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(p2);
	pC->PopAndDestroy(2);
	__UHEAP_CHECK(0);
	pC->PopAll();
	__UHEAP_CHECK(0);
//
	test.Next(_L("Push and pop and destroy all"));
	p1=User::Alloc(0x10);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	p2=User::Alloc(0x10);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	p3=User::Alloc(0x10);
	test(p3!=NULL);
	__UHEAP_CHECK(3);
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(p2);
	pC->PushL(p3);
	pC->PopAndDestroyAll();
	__UHEAP_CHECK(0);
//
	__UHEAP_MARKEND;
//
	delete pC;
	__UHEAP_MARKEND;
//
	test.End();
	}

LOCAL_C void testSingleLevelObjCleanup()
//
// Test single level object cleanup
//
	{

	test.Start(_L("Creating"));
//
	__UHEAP_MARK;
	CCleanup* pC=CCleanup::New();
	test(pC!=NULL);
//
	__UHEAP_MARK;
//
	test.Next(_L("Push and pop"));
	CBufFlat* p=CBufFlat::NewL(8);
	test(p!=NULL);
	__UHEAP_CHECK(1);
	pC->NextLevel();
	pC->PushL(p);
	pC->Pop();
	__UHEAP_CHECK(1);
	User::Free(p);
	__UHEAP_CHECK(0);
	pC->PopAll();
	__UHEAP_CHECK(0);
//
	test.Next(_L("Push and pop N"));
	CBufFlat* p1=CBufFlat::NewL(8);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	CBufFlat* p2=CBufFlat::NewL(8);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(p2);
	pC->Pop(2);
	__UHEAP_CHECK(2);
	User::Free(p1);
	User::Free(p2);
	__UHEAP_CHECK(0);
	pC->PopAll();
	__UHEAP_CHECK(0);
//
	test.Next(_L("Push and pop all"));
	p1=CBufFlat::NewL(8);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	p2=CBufFlat::NewL(8);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	CBufFlat* p3=CBufFlat::NewL(8);
	test(p3!=NULL);
	__UHEAP_CHECK(3);
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(p2);
	pC->PushL(p3);
	pC->PopAll();
	__UHEAP_CHECK(3);
	User::Free(p1);
	User::Free(p2);
	User::Free(p3);
	__UHEAP_CHECK(0);
//
	test.Next(_L("Push and pop and destroy"));
	p=CBufFlat::NewL(8);
	test(p!=NULL);
	__UHEAP_CHECK(1);
	pC->NextLevel();
	pC->PushL(p);
	pC->PopAndDestroy();
	__UHEAP_CHECK(0);
	pC->PopAll();
//
	test.Next(_L("Push and pop and destroy N"));
	p1=CBufFlat::NewL(8);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	p2=CBufFlat::NewL(8);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(p2);
	pC->PopAndDestroy(2);
	__UHEAP_CHECK(0);
	pC->PopAll();
	__UHEAP_CHECK(0);
//
	test.Next(_L("Push and pop and destroy all"));
	p1=CBufFlat::NewL(8);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	p2=CBufFlat::NewL(8);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	p3=CBufFlat::NewL(8);
	test(p3!=NULL);
	__UHEAP_CHECK(3);
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(p2);
	pC->PushL(p3);
	pC->PopAndDestroyAll();
	__UHEAP_CHECK(0);
//
	__UHEAP_MARKEND;
//
	delete pC;
	__UHEAP_MARKEND;
//
	test.End();
	}

LOCAL_C void testSingleLevelItemCleanup()
//
// Test single level object cleanup
//
	{

	test.Start(_L("Creating"));
//
	__UHEAP_MARK;
	CCleanup* pC=CCleanup::New();
	test(pC!=NULL);
//
	__UHEAP_MARK;
//
	test.Next(_L("Push and pop"));
	RItem r;
	r.Open();
	test(r.IsOpen());
	pC->NextLevel();
	pC->PushL(r);
	pC->Pop();
	test(r.IsOpen());
	r.Close();
	test(!r.IsOpen());
	pC->PopAll();
//
	test.Next(_L("Push and pop N"));
	RItem r1;
	r1.Open();
	RItem r2;
	r2.Open();
	pC->NextLevel();
	pC->PushL(r1);
	pC->PushL(r2);
	pC->Pop(2);
	test(r1.IsOpen());
	test(r2.IsOpen());
	r1.Close();
	r2.Close();
	pC->PopAll();
//
	test.Next(_L("Push and pop all"));
	r1.Open();
	r2.Open();
	RItem r3;
	r3.Open();
	pC->NextLevel();
	pC->PushL(r1);
	pC->PushL(r2);
	pC->PushL(r3);
	pC->PopAll();
	test(r1.IsOpen());
	test(r2.IsOpen());
	test(r3.IsOpen());
	r1.Close();
	r2.Close();
	r3.Close();
//
	test.Next(_L("Push and pop and destroy"));
	r.Open();
	pC->NextLevel();
	pC->PushL(r);
	pC->PopAndDestroy();
	test(!r.IsOpen());
	pC->PopAll();
//
	test.Next(_L("Push and pop and destroy N"));
	r1.Open();
	r2.Open();
	pC->NextLevel();
	pC->PushL(r1);
	pC->PushL(r2);
	pC->PopAndDestroy(2);
	test(!r1.IsOpen());
	test(!r2.IsOpen());
	pC->PopAll();
//
	test.Next(_L("Push and pop and destroy all"));
	r1.Open();
	r2.Open();
	r3.Open();
	pC->NextLevel();
	pC->PushL(r1);
	pC->PushL(r2);
	pC->PushL(r3);
	pC->PopAndDestroyAll();
	test(!r1.IsOpen());
	test(!r2.IsOpen());
	test(!r3.IsOpen());
//
	__UHEAP_MARKEND;
//
	delete pC;
	__UHEAP_MARKEND;
//
	test.End();
	}

LOCAL_C void testSingleLevelMixCleanup()
//
// Test single level mixed cleanup
//
	{

	test.Start(_L("Creating"));
//
	__UHEAP_MARK;
	CCleanup* pC=CCleanup::New();
	test(pC!=NULL);
//
	__UHEAP_MARK;
//
	test.Next(_L("PushO PushC PushI and pop N"));
	CBufFlat* p1=CBufFlat::NewL(8);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	TAny* p2=User::Alloc(0x10);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	RItem r;
	r.Open();
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(p2);
	pC->PushL(r);
	pC->Pop(3);
	__UHEAP_CHECK(2);
	test(r.IsOpen());
	User::Free(p1);
	User::Free(p2);
	r.Close();
	__UHEAP_CHECK(0);
	pC->PopAll();
	__UHEAP_CHECK(0);
//
	test.Next(_L("PushO PushI PushC PushO and pop all"));
	p1=CBufFlat::NewL(8);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	r.Open();
	p2=User::Alloc(0x10);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	CBufFlat* p3=CBufFlat::NewL(8);
	test(p3!=NULL);
	__UHEAP_CHECK(3);
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(r);
	pC->PushL(p2);
	pC->PushL(p3);
	pC->PopAll();
	__UHEAP_CHECK(3);
	test(r.IsOpen());
	User::Free(p1);
	User::Free(p2);
	User::Free(p3);
	r.Close();
	__UHEAP_CHECK(0);
//
	test.Next(_L("PushO PushC PushI and pop and destroy N"));
	p1=CBufFlat::NewL(8);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	p2=User::Alloc(0x10);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	r.Open();
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(p2);
	pC->PushL(r);
	pC->PopAndDestroy(3);
	test(!r.IsOpen());
	__UHEAP_CHECK(0);
	pC->PopAll();
	__UHEAP_CHECK(0);
//
	test.Next(_L("PushO PushI PushC PushO and pop and destroy all"));
	p1=CBufFlat::NewL(8);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	r.Open();
	p2=User::Alloc(0x10);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	p3=CBufFlat::NewL(8);
	test(p3!=NULL);
	__UHEAP_CHECK(3);
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(r);
	pC->PushL(p2);
	pC->PushL(p3);
	pC->PopAndDestroyAll();
	test(!r.IsOpen());
	__UHEAP_CHECK(0);
//
	__UHEAP_MARKEND;
//
	delete pC;
	__UHEAP_MARKEND;
//
	test.End();
	}

LOCAL_C void testMultiLevelCellsCleanup()
//
// Test multi level cells cleanup
//
	{

	test.Start(_L("Creating"));
//
	__UHEAP_MARK;
	CCleanup* pC=CCleanup::New();
	test(pC!=NULL);
//
	__UHEAP_MARK;
//
	test.Next(_L("Nest push push nest push popall popall"));
	TAny* p1=User::Alloc(0x10);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	TAny* p2=User::Alloc(0x10);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	TAny* p3=User::Alloc(0x10);
	test(p3!=NULL);
	__UHEAP_CHECK(3);
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(p2);
	pC->NextLevel();
	pC->PushL(p3);
	pC->PopAll();
	__UHEAP_CHECK(3);
	pC->PopAll();
	__UHEAP_CHECK(3);
	User::Free(p1);
	User::Free(p2);
	User::Free(p3);
	__UHEAP_CHECK(0);
//
	test.Next(_L("Nest push push nest push popallD popallD"));
	p1=User::Alloc(0x10);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	p2=User::Alloc(0x10);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	p3=User::Alloc(0x10);
	test(p3!=NULL);
	__UHEAP_CHECK(3);
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(p2);
	pC->NextLevel();
	pC->PushL(p3);
	pC->PopAndDestroyAll();
	__UHEAP_CHECK(2);
	pC->PopAndDestroyAll();
	__UHEAP_CHECK(0);
//
	__UHEAP_MARKEND;
//
	delete pC;
	__UHEAP_MARKEND;
//
	test.End();
	}

LOCAL_C void testMultiLevelObjCleanup()
//
// Test multi level object cleanup
//
	{

	test.Start(_L("Creating"));
//
	__UHEAP_MARK;
	CCleanup* pC=CCleanup::New();
	test(pC!=NULL);
//
	__UHEAP_MARK;
//
	test.Next(_L("Nest push push nest push popall popall"));
	CBufFlat* p1=CBufFlat::NewL(8);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	CBufFlat* p2=CBufFlat::NewL(8);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	CBufFlat* p3=CBufFlat::NewL(8);
	test(p3!=NULL);
	__UHEAP_CHECK(3);
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(p2);
	pC->NextLevel();
	pC->PushL(p3);
	pC->PopAll();
	__UHEAP_CHECK(3);
	pC->PopAll();
	__UHEAP_CHECK(3);
	User::Free(p1);
	User::Free(p2);
	User::Free(p3);
	__UHEAP_CHECK(0);
//
	test.Next(_L("Nest push push nest push popallD popallD"));
	p1=CBufFlat::NewL(8);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	p2=CBufFlat::NewL(8);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	p3=CBufFlat::NewL(8);
	test(p3!=NULL);
	__UHEAP_CHECK(3);
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(p2);
	pC->NextLevel();
	pC->PushL(p3);
	pC->PopAndDestroyAll();
	__UHEAP_CHECK(2);
	pC->PopAndDestroyAll();
	__UHEAP_CHECK(0);
//
	__UHEAP_MARKEND;
//
	delete pC;
	__UHEAP_MARKEND;
//
	test.End();
	}

LOCAL_C void testMultiLevelItemCleanup()
//
// Test multi level item cleanup
//
	{

	test.Start(_L("Creating"));
//
	__UHEAP_MARK;
	CCleanup* pC=CCleanup::New();
	test(pC!=NULL);
//
	__UHEAP_MARK;
//
	test.Next(_L("Nest push push nest push popall popall"));
	RItem r1;
	r1.Open();
	RItem r2;
	r2.Open();
	RItem r3;
	r3.Open();
	pC->NextLevel();
	pC->PushL(r1);
	pC->PushL(r2);
	pC->NextLevel();
	pC->PushL(r3);
	pC->PopAll();
	test(r1.IsOpen());
	test(r2.IsOpen());
	test(r3.IsOpen());
	pC->PopAll();
	test(r1.IsOpen());
	test(r2.IsOpen());
	test(r3.IsOpen());
	r1.Close();
	r2.Close();
	r3.Close();
//
	test.Next(_L("Nest push push nest push popallD popallD"));
	r1.Open();
	r2.Open();
	r3.Open();
	pC->NextLevel();
	pC->PushL(r1);
	pC->PushL(r2);
	pC->NextLevel();
	pC->PushL(r3);
	pC->PopAndDestroyAll();
	test(r1.IsOpen());
	test(r2.IsOpen());
	test(!r3.IsOpen());
	pC->PopAndDestroyAll();
	test(!r1.IsOpen());
	test(!r2.IsOpen());
	test(!r3.IsOpen());
//
	__UHEAP_MARKEND;
//
	delete pC;
	__UHEAP_MARKEND;
//
	test.End();
	}

LOCAL_C void testMultiLevelMixCleanup()
//
// Test multi level mixed cleanup
//
	{

	test.Start(_L("Creating"));
//
	__UHEAP_MARK;
	CCleanup* pC=CCleanup::New();
	test(pC!=NULL);
//
	__UHEAP_MARK;
//
	test.Next(_L("Nest pushO pushC nest pushI popall popall"));
	CBufFlat* p1=CBufFlat::NewL(8);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	TAny* p2=User::Alloc(0x10);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	RItem r3;
	r3.Open();
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(p2);
	pC->NextLevel();
	pC->PushL(r3);
	pC->PopAll();
	__UHEAP_CHECK(2);
	test(r3.IsOpen());
	pC->PopAll();
	__UHEAP_CHECK(2);
	test(r3.IsOpen());
	User::Free(p1);
	User::Free(p2);
	r3.Close();
	__UHEAP_CHECK(0);
//
	test.Next(_L("Nest pushO pushC nest pushI popallD popallD"));
	p1=CBufFlat::NewL(8);
	test(p1!=NULL);
	__UHEAP_CHECK(1);
	p2=User::Alloc(0x10);
	test(p2!=NULL);
	__UHEAP_CHECK(2);
	r3.Open();
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(p2);
	pC->NextLevel();
	pC->PushL(r3);
	pC->PopAndDestroyAll();
	__UHEAP_CHECK(2);
	test(!r3.IsOpen());
	pC->PopAndDestroyAll();
	test(!r3.IsOpen());
	__UHEAP_CHECK(0);
//
	__UHEAP_MARKEND;
//
	delete pC;
	__UHEAP_MARKEND;
//
	test.End();
	}

LOCAL_C void testSpecialCaseCleanup()
//
// Test special case cleanup
//
	{

	test.Start(_L("Creating"));
//
	__UHEAP_MARK;
	CCleanup* pC=CCleanup::New();
	test(pC!=NULL);
	__UHEAP_CHECK(KInitialCount);
//
	test.Next(_L("Nest push push push fail"));
	CBufFlat* p1=CBufFlat::NewL(8);
	test(p1!=NULL);
	__UHEAP_CHECK(KInitialCount+1);
	CBufFlat* p2=CBufFlat::NewL(8);
	test(p2!=NULL);
	__UHEAP_CHECK(KInitialCount+2);
	CBufFlat* p3=CBufFlat::NewL(8);
	test(p3!=NULL);
	__UHEAP_CHECK(KInitialCount+3);
	CBufFlat* p4=CBufFlat::NewL(8);
	test(p4!=NULL);
	__UHEAP_CHECK(KInitialCount+4);
	CBufFlat* p5=CBufFlat::NewL(8);
	test(p5!=NULL);
	__UHEAP_CHECK(KInitialCount+5);
	CBufFlat* p6=CBufFlat::NewL(8);
	test(p6!=NULL);
	__UHEAP_CHECK(KInitialCount+6);
	pC->NextLevel();
	pC->PushL(p1);
	pC->PushL(p2);
	pC->PushL(p3);
	pC->PushL(p4);
	pC->PushL(p5);
//
// The granularity is 4 so this should try and grow the array
// since room is always made for a free slot. We set the allocator
// to fail so that we can test that the free slot is re-established
// when we do the cleanup. This test only works in debug mode.
//
	__UHEAP_FAILNEXT(1);
#if defined(_DEBUG)
	TRAPD(r,pC->PushL(p6));
	test(r==KErrNoMemory);
#else
	TRAP_IGNORE(pC->PushL(p6));
#endif
	__UHEAP_CHECK(KInitialCount+6);
	pC->PopAndDestroyAll();
	__UHEAP_CHECK(KInitialCount);
//
	test.Next(_L("Nest push push push push popallD"));
	p1=CBufFlat::NewL(8);
	test(p1!=NULL);
	__UHEAP_CHECK(KInitialCount+1);
	p2=CBufFlat::NewL(8);
	test(p2!=NULL);
	__UHEAP_CHECK(KInitialCount+2);
	p3=CBufFlat::NewL(8);
	test(p3!=NULL);
	__UHEAP_CHECK(KInitialCount+3);
	p4=CBufFlat::NewL(8);
	test(p4!=NULL);
	__UHEAP_CHECK(KInitialCount+4);
	pC->NextLevel();
	pC->PushL(p1);
	pC->NextLevel();
	pC->PushL(p2);
	pC->PushL(p3);
	pC->PushL(p4);
	pC->PopAndDestroyAll();
	__UHEAP_CHECK(KInitialCount+1);
	pC->PopAndDestroyAll();
	__UHEAP_CHECK(KInitialCount);
//
	test.Next(_L("Destroy cleanup object"));
//
	p1=CBufFlat::NewL(8);
	test(p1!=NULL);
	__UHEAP_CHECK(KInitialCount+1);
	p2=CBufFlat::NewL(8);
	test(p2!=NULL);
	__UHEAP_CHECK(KInitialCount+2);
	p3=CBufFlat::NewL(8);
	test(p3!=NULL);
	__UHEAP_CHECK(KInitialCount+3);
	p4=CBufFlat::NewL(8);
	test(p4!=NULL);
	__UHEAP_CHECK(KInitialCount+4);
	pC->NextLevel();
	pC->PushL(p1);
	pC->NextLevel();
	pC->PushL(p2);
	pC->PushL(p3);
	pC->PushL(p4);
	delete pC;
	__UHEAP_CHECK(0);
//
	__UHEAP_MARKEND;
//
	test.End();
	}

LOCAL_C void testUnTrap()
//
// Test cleanup with normal exits
//
	{

	test.Start(_L("Creating"));
//
	__UHEAP_MARK;
	CTrapCleanup* pT=CTrapCleanup::New();
	test(pT!=NULL);
	__UHEAP_CHECK(KInitialCountAll);
//
	__UHEAP_MARK;
//
	test.Next(_L("PushC PushO EPop cleanup empty"));
	TRAPD(r,createL(EPop,EFalse))
	test.Next(_L("PushC PushO EPop cleanup empty 1"));
	test(r==KErrNone);
	test.Next(_L("PushC PushO EPop cleanup empty 2"));
	__UHEAP_CHECK(2);
	test.Next(_L("PushC PushO EPop cleanup empty 3"));
	User::Free(gP1);
	test.Next(_L("PushC PushO EPop cleanup empty 4"));
	delete gP2;
	test.Next(_L("PushC PushO EPop cleanup empty 5"));
	__UHEAP_CHECK(0);
//
	test.Next(_L("PushC PushO EPopAndDestroy cleanup empty"));
	TRAP(r,createL(EPopAndDestroy,EFalse))
	test(r==KErrNone);
	__UHEAP_CHECK(0);
//
/*
// Change of behavior for TCleanupTrapHandler means that the current
// cleanup stack must be empty when UnTrap is called. IE. calls to
// Push should be balanced with a Pop within the same function.
	test.Next(_L("PushC PushO ENull cleanup 2 objects"));
	TRAP(r,createL(ENull,EFalse))
	test(r==KErrNone);
	__UHEAP_CHECK(0);
*/
	__UHEAP_MARKEND;
//
	test.Next(_L("Test all LC functions"));
	TRAP(r,createAllL(EFalse))
	test(r==KErrNone);
	__UHEAP_CHECK(KInitialCountAll);
//
	delete pT;
	__UHEAP_MARKEND;
//
	test.End();
	}

LOCAL_C void testLeave()
//
// Test cleanup with leave exits
//
	{

	test.Start(_L("Creating"));
//
	__UHEAP_MARK;
	CTrapCleanup* pT=CTrapCleanup::New();
	test(pT!=NULL);
	__UHEAP_CHECK(KInitialCountAll);
//
	__UHEAP_MARK;
//
	test.Next(_L("PushC PushO EPop cleanup empty and leave"));
	TRAPD(r,createL(EPop,ETrue))
	test(r==KLeaveValue);
	__UHEAP_CHECK(2);
	User::Free(gP1);
	delete gP2;
	__UHEAP_CHECK(0);
//
	test.Next(_L("PushC PushO EPopAndDestroy cleanup empty and leave"));
	TRAP(r,createL(EPopAndDestroy,ETrue))
	test(r==KLeaveValue);
	__UHEAP_CHECK(0);
//
	test.Next(_L("PushC PushO ENull cleanup 2 objects and leave"));
	TRAP(r,createL(ENull,ETrue))
	test(r==KLeaveValue);
	__UHEAP_CHECK(0);
	__UHEAP_MARKEND;
//
	test.Next(_L("Test all LC functions and leave"));
	TRAP(r,createAllL(ETrue))
	test(r==KLeaveValue);
	__UHEAP_CHECK(KInitialCountAll);
//
	delete pT;
	__UHEAP_MARKEND;
//
	test.End();
	}

LOCAL_C void testMultiLeave()
//
// Test cleanup with multiple leave exits
//
	{

	test.Start(_L("Creating"));
//
	__UHEAP_MARK;
	CTrapCleanup* pT=CTrapCleanup::New();
	test(pT!=NULL);
//
	__UHEAP_MARK;
//
	test.Next(_L("PushC PushO nest PushO cleanup leave leave"));
	TRAPD(r,createL(EMulti,ETrue))
	test(r==KLeaveValue);
	__UHEAP_CHECK(0);
	__UHEAP_MARKEND;
//
	delete pT;
	__UHEAP_MARKEND;
//
	test.End();
	}

LOCAL_C void addNullItemL()
	{
	CleanupStack::PushL((TAny*)0);
	}

LOCAL_C void addCellL()
	{
	User::AllocLC(4);
	}

LOCAL_C void useCleanupStackL()
	{
	addNullItemL();
	addCellL();
	CleanupStack::PopAndDestroy();
	CleanupStack::Pop();
	}

LOCAL_C void reentrantCleanup(TAny*)
//
// A cleanup operation which uses a trap harness and the cleanup stack
//
	{
	TRAP_IGNORE(useCleanupStackL());
	}

LOCAL_C void addReentrantItemL()
	{
	CleanupStack::PushL(TCleanupItem(reentrantCleanup));
	}

LOCAL_C void addItemsL(TInt aCount)
//
// add number of reentrant items to make stack fail
//
	{
	while (--aCount>=0)
		addReentrantItemL();
#if !defined(_DEBUG)
	User::Leave(KErrNoMemory);	// heap failure not available
#endif
	}

const TInt KInitialStackSize=8;	// from UC_CLN.CPP
const TInt KGrowItems=KInitialStackSize-3;

LOCAL_C void testReentrancyL()
//
// Test the Cleanup stack can go re-entrant
//
	{

	test.Next(_L("PopAndDestroy()"));
	__UHEAP_MARK;
	addNullItemL();
	addCellL();
	addReentrantItemL();
	CleanupStack::PopAndDestroy(2);
	CleanupStack::Pop();
	__UHEAP_MARKEND;
//
	test.Next(_L("cleanup after a leave"));
	addNullItemL();
	TRAPD(r,addReentrantItemL();User::Leave(KLeaveValue);)
	test(r==KLeaveValue);
	CleanupStack::Pop();
//
	test.Next(_L("cleanup after stack failure"));
	// Ensuring stack reallocate fails by placing following cell
	TInt* forceAlloc=(TInt*)User::AllocL(4);
	for (TInt i=0;i<KGrowItems;++i)
		addNullItemL();
	__UHEAP_SETFAIL(RHeap::EDeterministic,1);	// fail everything 
	TRAP(r,addItemsL(1);)	// will leave as stack full and cannot grow
	test(r==KErrNoMemory);
	__UHEAP_RESET;
	CleanupStack::Pop(KGrowItems);
//
	test.Next(_L("multiple re-entrancy & stack failure"));
	__UHEAP_SETFAIL(RHeap::EDeterministic,1);	// fail everything
	TRAP(r,addItemsL(KGrowItems+1););
	test(r==KErrNoMemory);
	__UHEAP_RESET;
	User::Free(forceAlloc);
	}

LOCAL_C void testReentrancy()
//
// Test the Cleanup stack can go re-entrant
//
	{

	test.Start(_L("Creating"));
//
	__UHEAP_MARK;
	CTrapCleanup* pT=CTrapCleanup::New();
	test(pT!=NULL);
//
	TRAPD(r,testReentrancyL());
	test(r==KErrNone);
//
	delete pT;
	__UHEAP_MARKEND;
//
	test.End();
	}

LOCAL_C void testAutoCloseL()
//
// A leaving function for testAutoClose()
//
	{
	test.Next(_L("Create a TAutoClose object"));
	TAutoClose<RTimer> tim;
	tim.iObj.CreateLocal();
	test.Next(_L("Push it on the cleanup stack"));
	tim.PushL();
	test.Next(_L("Leave before object goes out of scope"));
	User::Leave(KErrGeneral);
	tim.Pop();
	}

LOCAL_C void testAutoClose()
//
// Test the TAutoClose class
//
	{

	// Kill the granules
	RTimer s[20];
	TInt i;
	for (i=0; i<20; i++)
		s[i].CreateLocal();
	for (i=0; i<20; i++)
		s[i].Close();

	__KHEAP_MARK;
	test.Start(_L("Make a TAutoClose object"));
		{
		TAutoClose<RTimer> tim;
		tim.iObj.CreateLocal();

		test.Next(_L("Let it fall out of scope"));
		}
	test.Next(_L("Check the object has closed"));
	__KHEAP_CHECK(0);

	TRAP_IGNORE(testAutoCloseL());
	test.Next(_L("Check object has been closed and cleaned up after leave"));
	__KHEAP_MARKEND;
	test.End();
	}

void CTest::ConstructL()
//
// Allocate a cell with CBase::new
//
	{

	TLex* pL=new(ELeave) TLex;
	CleanupStack::PushL(pL);
	}

void RItem::Cleanup(TAny* aPtr)
//
// Invoke the Close member on the RItem at aPtr
//
	{							

	((RItem*)aPtr)->Close();
	}

LOCAL_C TInt getStackPointer()
	{
	static TUint8 there;
	TUint8 here;
	return &here-&there;
	}
LOCAL_C void sheLeavesMeL(TBool sheLeavesMeNot)
	{
	if (!sheLeavesMeNot)
		User::Leave(KErrBadName);	// Montague
	}

// Variables for stack balance test need to be global or clever compiler optimisations
// Can interfere with stack balance calculations.
TInt StackBalanceLoopCounter;
TInt StackBalanceResult=KErrNone;
TInt StackBalanceBefore;
TInt StackBalanceAfter;

// Split into two functions because x86gcc makes a local stack optimisation for the second
// loop which unbalances the stack frame of the first loop.
LOCAL_C TInt StackBalanceNotLeaving()
	{
	StackBalanceBefore=getStackPointer();
	for (StackBalanceLoopCounter=0; StackBalanceLoopCounter<20;StackBalanceLoopCounter++)
		{
		TRAP(StackBalanceResult,sheLeavesMeL(ETrue));
		}
	StackBalanceAfter=getStackPointer();
	return StackBalanceAfter-StackBalanceBefore;
	}
LOCAL_C TInt StackBalanceLeaving()
	{
	StackBalanceBefore=getStackPointer();
	for (StackBalanceLoopCounter=0; StackBalanceLoopCounter<20;StackBalanceLoopCounter++)
		{
		TRAP(StackBalanceResult,sheLeavesMeL(EFalse));
		}
	StackBalanceAfter=getStackPointer();
	return StackBalanceAfter-StackBalanceBefore;
	}

LOCAL_C void testStackBalance()
//
// Ensure that we get the stack properly balanced
//
	{
	// Not leaving case
	test.Start(_L("Stack balance without Leaving"));
	TInt balance = StackBalanceNotLeaving();
	test.Printf(_L("Stack balance: %d bytes\n"), balance);
	test(balance == 0);

	// Leaving case
	test.Next(_L("Stack balance after Leave"));
	balance = StackBalanceLeaving();
	test.Printf(_L("Stack balance: %d bytes\n"), balance);
	test(balance == 0);
	test.End();
	}

void Inc(TAny* aPtr)
	{
	++(*(TInt*)aPtr);
	}

void testTrapIgnore()
	{
	test.Start(_L("Create cleanup"));
	CCleanup* pC=CCleanup::New();
	test(pC!=NULL);
	TInt count = 0;

	test.Next(_L("TRAP_IGNORE with no leave"));
	TRAP_IGNORE(
		CleanupStack::PushL(TCleanupItem(Inc,&count));
		CleanupStack::Pop();
		);
	test(count==0);

	test.Next(_L("TRAP_IGNORE with leave"));
	TRAP_IGNORE(
		CleanupStack::PushL(TCleanupItem(Inc,&count));
		User::Leave(KErrGeneral);
		);
	test(count==1);

	delete pC;
	test.End();
	}

void testCCleanupNewL()
	{
	// don't want just in time debugging as we trap panics
	TBool justInTime=User::JustInTime(); 
	User::SetJustInTime(EFalse); 

	// no need to test otherwise, since this calls only
	// CCleanup::New and that has been tested.
	test.Start(_L("Create cleanup NewL"));
	CCleanup* pC=CCleanup::NewL();
	test(pC!=NULL);
	delete pC;

	TAny* ptrTable[KTableSize];
	TInt allocSize=sizeof(CCleanup);
	TAny* ptr=0;

	__UHEAP_MARK;

	TInt i=0;
	// first alloc 4Kb bits
	do
		{
		ptr=User::Alloc(0x1000);
		if(ptr!=NULL)
			{
			ptrTable[i]=ptr;
			i++;
			}
		}
		while (ptr!=NULL && i<KTableSize);

	// then eat memory with size of CCleanup object granurality
	do
		{
		ptr=User::Alloc(allocSize);
		if(ptr!=NULL)
			{
			ptrTable[i]=ptr;
			i++;
			}
		}
		while (ptr!=NULL && i<KTableSize);
	
	i--; // last one failed, so lets adjust this to last successfull entry

	TInt r=KErrNone;
	test.Next(_L("Create cleanup NewL while no room in heap"));
	TRAP(r,pC=CCleanup::NewL());
	test_Equal(KErrNoMemory,r);

	for (;i>=0;i--)
		{
		User::Free(ptrTable[i]);
		}
	
	__UHEAP_MARKEND;

	//restore settings
	User::SetJustInTime(justInTime); 

	test.End();
	}

GLDEF_C TInt E32Main()
    {
	test.Title();
	
	test.Start(_L("Test destructor causing stack reallocation"));
	testDestructorStackReallocation();	
	
	test.Next(_L("CCleanup single level tests just alloc cells"));
	testSingleLevelCellsCleanup();

	test.Next(_L("CCleanup single level tests just objects"));
	testSingleLevelObjCleanup();

	test.Next(_L("CCleanup single level tests just items"));
	testSingleLevelItemCleanup();

	test.Next(_L("CCleanup single level tests mixed"));
	testSingleLevelMixCleanup();

	test.Next(_L("CCleanup multi level tests just alloc cells"));
	testMultiLevelCellsCleanup();

	test.Next(_L("CCleanup multi level tests just objects"));
	testMultiLevelObjCleanup();

	test.Next(_L("CCleanup multi level tests just items"));
	testMultiLevelItemCleanup();

	test.Next(_L("CCleanup multi level tests mixed"));
	testMultiLevelMixCleanup();

	test.Next(_L("CCleanup special case test"));
	testSpecialCaseCleanup();

	test.Next(_L("Install trap handler"));
	CTrapCleanup* pT=CTrapCleanup::New();
	test(pT!=NULL);

	test.Next(_L("Untrap handling tests"));
	testUnTrap();

	test.Next(_L("Leave handling tests"));
	testLeave();

	test.Next(_L("Multi level leave handling tests"));
	testMultiLeave();

	test.Next(_L("Test TAutoClose"));
	testAutoClose();

	test.Next(_L("Test Re-entrancy of cleanup stack"));
	testReentrancy();

	test.Next(_L("Test stack safety of TRAP and Leave"));
	testStackBalance();

	test.Next(_L("Test TRAP_IGNORE"));
	testTrapIgnore();

	test.Next(_L("Test CCleanup::NewL"));
	testCCleanupNewL();

	delete pT;

	test.End();
	return(0);
    }

