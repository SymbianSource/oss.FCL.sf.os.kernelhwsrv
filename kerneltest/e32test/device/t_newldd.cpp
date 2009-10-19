// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32def.h>
#include <e32def_private.h>
#include "t_newldd.h"
#include "t_new_classes.h"

RTest test(_L("Testing Operator New"));

TInt RNewLddTest::DoControl(TInt aFunction)
	{
	return RBusLogicalChannel::DoControl(aFunction);
	}

TInt RNewLddTest::Open()
	{
	return DoCreate(KLddName,TVersion(0,1,1),KNullUnit,NULL,NULL);
	}

RNewLddTest lddconn;

TInt TestNew()
	{
	return lddconn.DoControl(RNewLddTest::ENew);
	}
TInt TestPlacementVectorNew()
	{
	return lddconn.DoControl(RNewLddTest::EPlacementVectorNew);
	}
TInt TestVectorNew()
	{
	return lddconn.DoControl(RNewLddTest::EVectorNew);
	}
TInt TestPlacementNew()
	{
	return lddconn.DoControl(RNewLddTest::EPlacementNew);
	}

void UserSideTestNewOOM()
	{
	RDebug::Printf("User-Side: operator new OOM");
	//OOM tests: should not throw
	
	#define TEST_NEW_OOM(CLASS) \
	RDebug::Printf("new " #CLASS);\
		{\
		CLASS* p##CLASS = new CLASS;\
		test_Equal(NULL, p##CLASS);\
		}

	TEST_NEW_OOM(XVeryLargeClassCtorAndDtor);
	TEST_NEW_OOM(XVeryLargeClassCtorOnly);
	TEST_NEW_OOM(XVeryLargeClassDtorOnly);
	TEST_NEW_OOM(XVeryLargeClassNoTors);
	}

void UserSideTestNewConstruction()
	{
	RDebug::Printf("User-Side: operator new non-OOM");
	//Non-OOM:
	

	#define TEST_NEW_CONSTRUCTION(CLASS) \
	RDebug::Printf("new " #CLASS);\
		{\
		CLASS* p##CLASS = new CLASS;\
		test_NotNull(p##CLASS);\
		test_Equal(EConstructed, (p##CLASS)->iState);\
		delete p##CLASS;\
		}

	TEST_NEW_CONSTRUCTION(XCtorAndDtor)
	TEST_NEW_CONSTRUCTION(XCtorOnly)


	#define TEST_NEW(CLASS) \
	RDebug::Printf("new " #CLASS);\
		{\
		CLASS* p##CLASS = new CLASS;\
		test_NotNull(p##CLASS);\
		delete p##CLASS;\
		}
	TEST_NEW(XDtorOnly)
	TEST_NEW(XNoTors)
	}
	
void UserSideTestNew()
	{
	UserSideTestNewOOM();
	UserSideTestNewConstruction();
	}

void TrappedUserSideTestNew()
	{
	RDebug::Printf("TRAPPED User-Side: operator new");
	//OOM tests: should not throw

	#define TEST_NEW_ELEAVE_OOM(CLASS) \
	RDebug::Printf("new(ELeave) " #CLASS);\
		{\
		TRAPD(r, new(ELeave) (CLASS));\
		test_Equal(KErrNoMemory, r);\
		}

	TEST_NEW_ELEAVE_OOM(XVeryLargeClassCtorAndDtor);
	TEST_NEW_ELEAVE_OOM(XVeryLargeClassCtorOnly);
	TEST_NEW_ELEAVE_OOM(XVeryLargeClassDtorOnly);
	TEST_NEW_ELEAVE_OOM(XVeryLargeClassNoTors);



	RDebug::Printf("User-Side: operator new non-OOM");
	//Non-OOM:

	#define TEST_NEW_ELEAVE(CLASS, TEST_CTOR ) \
	RDebug::Printf("new(ELeave) " #CLASS);\
		{\
		CLASS* p##CLASS=NULL;\
		TRAPD(r, p##CLASS = new(ELeave) (CLASS));\
		test_KErrNone(r);\
		volatile TBool testCtor=(TEST_CTOR);\
		if(testCtor)\
			{\
			test_Equal(EConstructed, (p##CLASS)->iState);\
			}\
		delete p##CLASS;\
		}

	TEST_NEW_ELEAVE(XCtorAndDtor, ETrue);
	TEST_NEW_ELEAVE(XCtorOnly, ETrue);
	TEST_NEW_ELEAVE(XDtorOnly, EFalse);
	TEST_NEW_ELEAVE(XNoTors, EFalse);
	}


#define TEST_ARRAY_CONSTRUCTION(ARRAY, LENGTH)\
	{\
	for(TInt i=0; i<(LENGTH); ++i)\
		{\
		test_Equal(EConstructed, ARRAY[i].iState);\
		}\
	}

void UserSideTestVectorNew()
	{
	RDebug::Printf("User-Side:vector operator new");
	RDebug::Printf("OOM tests");

	#define TEST_VEC_NEW_OOM(CLASS) \
	RDebug::Printf("new " #CLASS "[%d]", KOOMArraySize );\
		{\
		CLASS* p##CLASS = new CLASS[KOOMArraySize];\
		test_Equal(NULL, p##CLASS);\
		}

	TEST_VEC_NEW_OOM(XCtorAndDtor);
	TEST_VEC_NEW_OOM(XCtorOnly);
	TEST_VEC_NEW_OOM(XDtorOnly);
	TEST_VEC_NEW_OOM(XNoTors);
	
	RDebug::Printf("non-OOM tests");

	#define TEST_VEC_NEW(CLASS, ARRAY_LENGTH, TEST_CTOR) \
	RDebug::Printf("new " #CLASS "[%d]", ARRAY_LENGTH);\
		{\
		CLASS* p##CLASS = new CLASS[(ARRAY_LENGTH)];\
		test_NotNull(p##CLASS);\
		volatile TBool testCtor=(TEST_CTOR);\
		if(testCtor)\
			{\
			TEST_ARRAY_CONSTRUCTION(p##CLASS, ARRAY_LENGTH);\
			}\
		delete[] p##CLASS;\
		}

	TEST_VEC_NEW(XCtorAndDtor, KTestArrayLength, ETrue);
	TEST_VEC_NEW(XCtorOnly, KTestArrayLength, ETrue);
	TEST_VEC_NEW(XDtorOnly, KTestArrayLength, EFalse);
	TEST_VEC_NEW(XNoTors, KTestArrayLength, EFalse);
	}

void TrappedUserSideTestVectorNew()
	{
	RDebug::Printf("User-Side:vector operator new");
	RDebug::Printf("OOM tests");

	#define TEST_VEC_NEW_ELEAVE_OOM(CLASS) \
	RDebug::Printf("new(ELeave) " #CLASS "[%d]", KOOMArraySize );\
		{\
	   	TRAPD(r, new(ELeave) CLASS[KOOMArraySize];)\
		test_Equal(KErrNoMemory, r);\
		}

	TEST_VEC_NEW_ELEAVE_OOM(XCtorAndDtor);
	TEST_VEC_NEW_ELEAVE_OOM(XCtorOnly);
	TEST_VEC_NEW_ELEAVE_OOM(XDtorOnly);
	TEST_VEC_NEW_ELEAVE_OOM(XNoTors);



	RDebug::Printf("non-OOM tests");
	#define TEST_VEC_NEW_ELEAVE(CLASS, ARRAY_LENGTH, TEST_CTOR) \
	RDebug::Printf("new(ELeave) " #CLASS "[%d]", ARRAY_LENGTH);\
		{\
		CLASS* p##CLASS = NULL;\
		TRAPD(r, p##CLASS = new(ELeave) CLASS[(ARRAY_LENGTH)]);\
		test_KErrNone(r);\
		TBool testCtor=(TEST_CTOR);\
		if(testCtor)\
			{\
			TEST_ARRAY_CONSTRUCTION(p##CLASS, ARRAY_LENGTH);\
			}\
		delete[] p##CLASS;\
		}

	TEST_VEC_NEW_ELEAVE(XCtorAndDtor, KTestArrayLength, ETrue);
	TEST_VEC_NEW_ELEAVE(XCtorOnly, KTestArrayLength, ETrue);
	TEST_VEC_NEW_ELEAVE(XDtorOnly, KTestArrayLength, EFalse);
	TEST_VEC_NEW_ELEAVE(XNoTors, KTestArrayLength, EFalse);
	}

void UserSideTestPlacementNew()
	{
	RDebug::Printf("::UserSideTestPlacementNew");
	
	#define TEST_PLACMENT_NEW(CLASS, POST_CTOR_STATE, POST_DTOR_STATE)\
		{\
		void* someram = User::AllocZ(sizeof(CLASS));\
		test_NotNull(someram);\
		RDebug::Printf("new (someram) " #CLASS);\
		CLASS* p##CLASS = new (someram) CLASS;\
		test_Equal(someram, p##CLASS);\
		test_Equal(POST_CTOR_STATE, p##CLASS->iState);\
		p##CLASS->~CLASS();\
		test_Equal(POST_DTOR_STATE, p##CLASS->iState);\
		User::Free(someram);\
		p##CLASS=NULL;\
		}\

	TEST_PLACMENT_NEW(XCtorAndDtor, EConstructed, EDeconstructed);
	TEST_PLACMENT_NEW(XCtorOnly, EConstructed, EConstructed);
	TEST_PLACMENT_NEW(XDtorOnly, ENull, EDeconstructed);
	TEST_PLACMENT_NEW(XNoTors, ENull, ENull);
	}


void UserSideTestPlacementVectorNew()
	{
	__UHEAP_MARK;

	RDebug::Printf("::UserSideTestPlacementVectorNew");

	#define TEST_VEC_PLACEMENT_NEW(CLASS, ARRAY_LENGTH, POST_CTOR_STATE, POST_DTOR_STATE)\
	RDebug::Printf("new(someram) " #CLASS "[%d]", ARRAY_LENGTH);\
		{\
		void* someram = User::AllocZ(sizeof(CLASS) * ARRAY_LENGTH);\
		test_NotNull(someram);\
		CLASS* p##CLASS = new (someram) CLASS[(ARRAY_LENGTH)];\
		for(TInt i=0; i<(ARRAY_LENGTH); ++i)\
			{\
			test_Equal(POST_CTOR_STATE, p##CLASS[i].iState);\
			p##CLASS[i].~CLASS();\
			test_Equal(POST_DTOR_STATE, p##CLASS[i].iState);\
			}\
		User::Free(someram);\
		}

	TEST_VEC_PLACEMENT_NEW(XCtorAndDtor, KTestArrayLength, EConstructed, EDeconstructed);
	TEST_VEC_PLACEMENT_NEW(XCtorOnly, KTestArrayLength, EConstructed, EConstructed);
	TEST_VEC_PLACEMENT_NEW(XDtorOnly, KTestArrayLength, ENull, EDeconstructed);
	TEST_VEC_PLACEMENT_NEW(XNoTors, KTestArrayLength, ENull, ENull);

	__UHEAP_MARKEND;
	}


TInt E32Main()
	{
	__UHEAP_MARK;

	test.Start(_L("Testing operator new"));
	
	test.Next(_L("Installing LDD"));
	TInt r=User::LoadLogicalDevice(KKInstallLddName);
	test(r==KErrNone || r==KErrAlreadyExists);
	
	__KHEAP_MARK;

	#define TEST_THIS(X) test.Next(_L(#X)); (X)
	
	TEST_THIS(UserSideTestNew());
	TEST_THIS(UserSideTestPlacementNew());

// Workaround for bug in MSVC6/CW compilers. The following test case fails on
// WINS, WINSCW and X86 targets (not X86GCC), where the placement vector new
// operator does not behave as expected, ultimately resulting in heap corruption
// when the parameter to placement array new is passed on the stack.
#if !(defined(__WINS__) || (defined(__VC32__) && (_MSC_VER < 1300)))
	TEST_THIS(UserSideTestPlacementVectorNew());
#else
	test.Next(_L("Emulator and/or VC32 - Skipped: UserSideTestPlacementVectorNew"));
#endif

	TEST_THIS(UserSideTestVectorNew());
	TEST_THIS(TrappedUserSideTestNew());
	TEST_THIS(TrappedUserSideTestVectorNew());
	
	r=lddconn.Open();
	test_KErrNone(r);

	test.Next(_L("Kernel-side:Normal operator new"));
	r = TestNew();
	test_KErrNone(r);
		
	test.Next(_L("Kernel-side:Placement operator new"));
	r = TestPlacementNew();
	test_KErrNone(r);

	test.Next(_L("Kernel-side:Placement Vector operator new"));
	r = TestPlacementVectorNew();
	test_KErrNone(r);

	test.Next(_L("Kernel-side:Vector operator new"));
	r = TestVectorNew();
	test_KErrNone(r);

	r = RTest::CloseHandleAndWaitForDestruction(lddconn);
	test_KErrNone(r);

	test.End();
	test.Close();

	__KHEAP_MARKEND;
	__UHEAP_MARKEND;
	return KErrNone;
	}

