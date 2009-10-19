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

#include <kernel/kern_priv.h>
#include "d_newldd.h"
#include "t_newldd.h"

TInt DOperatorNewTest::Request(TInt aReqNo, TAny* /*a1*/, TAny* /*a2*/)
	{
	switch(aReqNo)
		{
		//new
		case RNewLddTest::ENew:
			{
			return TestNew();
			}
		//placement new
		case RNewLddTest::EPlacementVectorNew:
			{
			return TestPlacementVectorNew();
			}
		//vector new
		case RNewLddTest::EVectorNew:
			{
			return TestVectorNew();
			}
		case RNewLddTest::EPlacementNew:
			{
			return TestPlacementNew();
			}
		default:
			break;
		}
	return KErrNone;
	}

TInt DOperatorNewTest::TestNew()
	{
	Kern::Printf("::TestNew()");
	NKern::ThreadEnterCS();

	#define TEST_KERN_NEW_OOM(CLASS)\
		{\
		Kern::Printf("new " #CLASS);\
		CLASS* p##CLASS=NULL;\
		XTRAPD(r,XT_DEFAULT, p##CLASS = new CLASS);\
		if(r!=KErrNone)\
			{\
			NKern::ThreadLeaveCS();\
			return r;\
			}\
		if(p##CLASS)\
			{\
			delete p##CLASS;\
			NKern::ThreadLeaveCS();\
			return KErrGeneral;\
			}\
		}

	//OOM tests: should(must) not throw
	Kern::Printf("OOM Tests;");

	TEST_KERN_NEW_OOM(XVeryLargeClassCtorAndDtor);
	TEST_KERN_NEW_OOM(XVeryLargeClassCtorOnly);
	TEST_KERN_NEW_OOM(XVeryLargeClassDtorOnly);
	TEST_KERN_NEW_OOM(XVeryLargeClassNoTors);

	Kern::Printf("non-OOM Tests;");
	//Non-OOM:
	
	#define TEST_KERN_NEW(CLASS, TEST_CTOR)\
		{\
		Kern::Printf("new " #CLASS);\
		CLASS* p##CLASS=NULL;\
		XTRAPD(r,XT_DEFAULT, p##CLASS = new CLASS);\
		if(r!=KErrNone)\
			{\
			NKern::ThreadLeaveCS();\
			return r;\
			}\
		if(p##CLASS==NULL)\
			{\
			NKern::ThreadLeaveCS();\
			return KErrGeneral;\
			}\
		volatile TBool testCtor=(TEST_CTOR);\
		if(testCtor && (p##CLASS->iState!=EConstructed) )\
			{\
			r=KErrGeneral;\
			}\
		delete p##CLASS;\
		p##CLASS=NULL;\
		if(r!=KErrNone)\
			return r;\
		}

	TEST_KERN_NEW(XCtorAndDtor, ETrue);
	TEST_KERN_NEW(XCtorOnly, ETrue);
	TEST_KERN_NEW(XDtorOnly, EFalse);
	TEST_KERN_NEW(XNoTors, EFalse);
	
	NKern::ThreadLeaveCS();
	return KErrNone;
	}

TInt DOperatorNewTest::TestPlacementNew()
	{
	Kern::Printf("::TestPlacementNew");

	#define TEST_KERN_PLACEMENT_NEW(CLASS, POST_CTOR, POST_DTOR) \
		{\
		Kern::Printf("new(someram) " #CLASS);\
		NKern::ThreadEnterCS();\
		void* someram = Kern::AllocZ(sizeof(CLASS));\
		NKern::ThreadLeaveCS();\
		if(!someram)\
			return KErrNoMemory;\
		\
		CLASS* p##CLASS = new (someram) CLASS;\
		TInt r=KErrNone;\
		if(p##CLASS->iState != POST_CTOR)\
			{\
			r=KErrGeneral;\
			}\
		if(r==KErrNone)\
			{\
			p##CLASS->~CLASS();\
			if(p##CLASS->iState != POST_DTOR)\
				{\
				r=KErrGeneral;\
				}\
			}\
		NKern::ThreadEnterCS();\
		Kern::Free(someram);\
		NKern::ThreadLeaveCS();\
		if(r != KErrNone)\
			return r;\
		}

	TEST_KERN_PLACEMENT_NEW(XCtorAndDtor, EConstructed, EDeconstructed);
	TEST_KERN_PLACEMENT_NEW(XCtorOnly, EConstructed, EConstructed);
	TEST_KERN_PLACEMENT_NEW(XDtorOnly, ENull, EDeconstructed);
	TEST_KERN_PLACEMENT_NEW(XNoTors, ENull, ENull);
	
	return KErrNone;
	}

TInt DOperatorNewTest::TestPlacementVectorNew()
	{
	
	Kern::Printf("::TestPlacementVectorNew");
	
	//for vector placement new, emulator compilers 
	//allocate a cookie at start of buffer.
	//this seems wrong since the cookie is an internal compiller
	//detail which the user should not need to know about
	#if defined(__WINSCW__) || defined(__VC32__)
	Kern::Printf("Not running on emulator. WINSCW and Visual studio\n insert a cookie for placement vector new"); 

	#else

	#define TEST_KERN_PLACEMENT_VECTOR_NEW(CLASS, ARRAY_LENGTH, POST_CTOR, POST_DTOR) \
		{\
		NKern::ThreadEnterCS();\
		void* someram = Kern::AllocZ(sizeof(CLASS) * (ARRAY_LENGTH));\
		NKern::ThreadLeaveCS();\
		if(someram==NULL)\
			return KErrNoMemory;\
		\
		TInt r = KErrNone;\
		Kern::Printf("new (someram) " #CLASS "[%d]", ARRAY_LENGTH);\
		\
		CLASS* p##CLASS = new (someram) CLASS[ARRAY_LENGTH];\
		for(TInt i=0; i<(ARRAY_LENGTH); ++i)\
			{\
			if(p##CLASS[i].iState != POST_CTOR)\
				{\
				r=KErrGeneral;\
				break;\
				}\
			p##CLASS[i].~CLASS();\
			if(p##CLASS[i].iState != POST_DTOR)\
				{\
				r=KErrGeneral;\
				break;\
				}\
			}\
		NKern::ThreadEnterCS();\
		Kern::Free(someram);\
		NKern::ThreadLeaveCS();\
		if(r!=KErrNone)\
			return r;\
		}\
	
	TEST_KERN_PLACEMENT_VECTOR_NEW(XCtorAndDtor, KTestArrayLength, EConstructed, EDeconstructed);
	TEST_KERN_PLACEMENT_VECTOR_NEW(XCtorOnly, KTestArrayLength, EConstructed, EConstructed);
	TEST_KERN_PLACEMENT_VECTOR_NEW(XDtorOnly, KTestArrayLength, ENull, EDeconstructed);
	TEST_KERN_PLACEMENT_VECTOR_NEW(XNoTors, KTestArrayLength, ENull, ENull);

	#endif

	return KErrNone;
	}
TInt DOperatorNewTest::TestVectorNew()
	{
	//OOM testing
	Kern::Printf("::TestVectorNew()");
	Kern::Printf("OOM test");
	
	TInt r=KErrNone;

	#define TEST_KERN_VECTOR_NEW_OOM(CLASS, ARRAY_LENGTH)\
	{\
		Kern::Printf("new " #CLASS "[%d]", ARRAY_LENGTH);\
		CLASS* p##CLASS = NULL;\
		NKern::ThreadEnterCS();\
		XTRAP(r,XT_DEFAULT,p##CLASS = new CLASS[ARRAY_LENGTH]; );\
		if(p##CLASS)\
			{\
			r=KErrGeneral;\
			delete p##CLASS;\
			}\
		NKern::ThreadLeaveCS();\
		if(r!=KErrNone)\
			{\
			return r;\
			}\
	}\

	TEST_KERN_VECTOR_NEW_OOM(XCtorAndDtor, KOOMArraySize);
	TEST_KERN_VECTOR_NEW_OOM(XCtorOnly, KOOMArraySize);
	TEST_KERN_VECTOR_NEW_OOM(XDtorOnly, KOOMArraySize);
	TEST_KERN_VECTOR_NEW_OOM(XNoTors, KOOMArraySize);

	
	//non-OOM:
	Kern::Printf("non-OOM test");

	#define TEST_KERN_VECTOR_NEW(CLASS, ARRAY_LENGTH, TEST_CTOR)\
	{\
		Kern::Printf("new " #CLASS "[%d]", ARRAY_LENGTH);\
		CLASS* p##CLASS = NULL;\
		NKern::ThreadEnterCS();\
		XTRAP(r,XT_DEFAULT,p##CLASS = new CLASS[ARRAY_LENGTH]; );\
		NKern::ThreadLeaveCS();\
		if(p##CLASS == NULL)\
			{\
			return KErrNoMemory;\
			}\
		\
		TBool testCtor=(TEST_CTOR);\
		if(testCtor)\
			{\
			for(TInt i=0; i<(ARRAY_LENGTH); ++i)\
				{\
				if(p##CLASS[i].iState!=	EConstructed)\
					{\
					r=KErrGeneral;\
					break;\
					}\
				}\
			}\
		\
		NKern::ThreadEnterCS();\
		delete[] p##CLASS;\
		NKern::ThreadLeaveCS();\
		p##CLASS=NULL;\
		if(r!=KErrNone)\
			{\
			return r;\
			}\
	}\

	TEST_KERN_VECTOR_NEW(XCtorAndDtor, KTestArrayLength, ETrue);
	TEST_KERN_VECTOR_NEW(XCtorOnly, KTestArrayLength, ETrue);
	TEST_KERN_VECTOR_NEW(XDtorOnly, KTestArrayLength, EFalse);
	TEST_KERN_VECTOR_NEW(XNoTors, KTestArrayLength, EFalse);

	return KErrNone;
	}

TInt DOperatorNewTestFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DTest on this logical device
//
	{
	aChannel=new DOperatorNewTest();
	return aChannel?KErrNone:KErrNoMemory;
	}

TInt DOperatorNewTestFactory::Install()
//
// Install the LDD - overriding pure virtual
//
	{
	iVersion = TVersion(0,1,1);
	return SetName(&KLddName);
	}

void DOperatorNewTestFactory::GetCaps(TDes8& /*aDes*/) const
//
// Get capabilities - overriding pure virtual
//
	{
	//not supported
	}



DECLARE_STANDARD_LDD()
	{
	//create factory here.
	return new DOperatorNewTestFactory;
	}


