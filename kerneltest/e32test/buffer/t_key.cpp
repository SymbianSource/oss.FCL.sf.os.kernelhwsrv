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
// e32test\buffer\t_key.cpp
// Overview:
// Test arrays keys against flat and segmented arrays of characters (8 and 16 bit) and records.
// API Information:
// TKeyArrayFix, TKeyArrayVar.
// Details:
// - Create flat and segmented array of TText8, TText16, TText. Append some text to the 
// arrays and test the functions of the TKeyArrayFix class using the Find method which 
// calls SetPtr, Set, Compare methods with ECmpNormal8, ECmpFolded8, ECmpCollated8, 
// ECmpNormal16, ECmpFolded16, ECmpCollated16, ECmpNormal, ECmpFolded, ECmpCollated key types.
// - Test the functions of the TKeyArrayVar class using the Find method which calls SetPtr, Set, 
// Compare methods with ECmpNormal8, ECmpFolded8, ECmpCollated8, ECmpNormal16, ECmpFolded16, 
// ECmpCollated16, ECmpNormal, ECmpFolded, ECmpCollated key types.
// - Create flat and segmented array of TText, append some structures with different values.
// - Test the functions of the TKeyArrayFix, TKeyArrayVar classes by searching the values using 
// sequential search technique with ECmpNormal, ECmpTInt32 key type and verifying that it is 
// found at correct position.
// - For TKeyArrayFix and TKeyArrayVar class, create a flat array of TText, append some structures 
// with different numeric values, sort the array, search the values using ECmpTInt, ECmpTUint, 
// ECmpTint8, ECmpTUint8, ECmpTint16, ECmpTUint16, ECmpTint32, ECmpTUint32 key types and verify 
// that values are found in order as expected.
// Platforms/Drives/Compatibility:
// All 
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

#ifdef __VC32__
#pragma warning (disable:4710)	// Function not expanded
#pragma warning (disable:4702)	// Unreachable code
#endif

const TInt KTestGranularity=0x02;

LOCAL_D RTest test(_L("T_KEY"));

template <class T,TInt S>
class TArr
	{
public:
	TArr() {}
	TInt Count() const {return S;}
	T& operator[](TInt anIndex) {return iArr[anIndex];}
	const T& operator[](TInt anIndex) const {return iArr[anIndex];}
private:
	T iArr[S];
	};

//#if defined(_DEBUG)
struct Record
	{
	TBuf<0x10> name;  
	TInt32	age;
	TText8	code;
	};

struct Record2
	{
	TInt	tint;
	TUint	tuint;
	TInt8	tint8;
	TUint8	tuint8;
	TInt16  tint16;
	TUint16 tuint16;
	TInt32  tint32;
	TUint32 tuint32;
	}Rec1, Rec2, Rec3, Rec4;		
		
LOCAL_C void SetRecordData(void)
	{
	Rec1.tint=KMaxTInt;
	Rec2.tint=0;
	Rec3.tint=-KMaxTInt;
	Rec4.tint=KMinTInt;
	Rec1.tint8=KMaxTInt8;
	Rec2.tint8=0;
	Rec3.tint8=-KMaxTInt8;
	Rec4.tint8=(TInt8)KMinTInt8;
	Rec1.tint16=KMaxTInt16;
	Rec2.tint16=0;
	Rec3.tint16=-KMaxTInt16;
	Rec4.tint16=(TInt16)KMinTInt16;
	Rec1.tint32=KMaxTInt32;
	Rec2.tint32=0;
	Rec3.tint32=-KMaxTInt32;
	Rec4.tint32=(TInt32)KMinTInt32;
	Rec1.tuint=0;
	Rec2.tuint=1;
	Rec3.tuint=KMaxTUint-1;
	Rec4.tuint=KMaxTUint;
	Rec1.tuint8=0;
	Rec2.tuint8=1;
	Rec3.tuint8=(TUint8)(KMaxTUint8-1);
	Rec4.tuint8=(TUint8)KMaxTUint8;
	Rec1.tuint16=0;
	Rec2.tuint16=1;
	Rec3.tuint16=(TUint16)(KMaxTUint16-1);
	Rec4.tuint16=(TUint16)KMaxTUint16;
	Rec1.tuint32=0;
	Rec2.tuint32=1;
	Rec3.tuint32=(TUint32)(KMaxTUint32-1);
	Rec4.tuint32=KMaxTUint32;
	}

typedef enum {eEight, eSixteen} TMode;
			
template<class KeyType, class ArrayType, class S> // S is TText8, TTExt etc. called S as _TL requires S in e32test.h
class TestTKey
	{
public:
	void Test1(TKeyCmpText, TKeyCmpText, TKeyCmpText);
	void Test2(TKeyCmpText, TKeyCmpText, TKeyCmpText);
	void Test3(void);
	void Test4(void);
	void Test5(void);
	void Test6(void);
	};

template<class KeyType, class ArrayType, class S>
GLDEF_C void TestTKey<KeyType, ArrayType, S>::Test1(TKeyCmpText N, TKeyCmpText F, TKeyCmpText C)
	{
	// This tests the functions of the TKey classes indirectly - mostly using the Find method
	// which calls SetPtr(), Set() (both trivial) and more importantly Compare(), 

	ArrayType* pArray=new ArrayType(KTestGranularity);
	pArray->AppendL(*(const TArr<S,5>*)_TL("aa cc"));
	pArray->AppendL(*(const TArr<S,5>*)_TL("bb bb"));
	pArray->AppendL(*(const TArr<S,5>*)_TL("cc aa"));

	KeyType NormalKey(0,N,5);	
	KeyType NormalKeyOffset(sizeof(S)*3,N,2) ;
	KeyType FoldedKey(0,F,5);
	KeyType FoldedKeyOffset(sizeof(S)*3,F,2);	
	KeyType CollatedKey(0,C,5);
	KeyType CollatedKeyOffset(sizeof(S)*3,C,2);

	TInt pos;
	test(pArray->Find(*(const TArr<S,5>*)_TL("aa cc"), NormalKey, pos)==0);
	test(pos==0);
	test(pArray->Find(*(const TArr<S,5>*)_TL("bb bb"), NormalKey, pos)==0);
	test(pos==1);
	test(pArray->Find(*(const TArr<S,5>*)_TL("cc aa"), NormalKey, pos)==0);
	test(pos==2);	
	test(pArray->Find(*(const TArr<S,5>*)_TL("BB BB"), NormalKey, pos)!=0);


	test(pArray->Find(*(const TArr<S,5>*)_TL("___cc"), NormalKeyOffset, pos)==0);
	test(pos==0);
	test(pArray->Find(*(const TArr<S,5>*)_TL("___bb"), NormalKeyOffset, pos)==0);
	test(pos==1);
	test(pArray->Find(*(const TArr<S,5>*)_TL("___aa"), NormalKeyOffset, pos)==0);
	test(pos==2);
	test(pArray->Find(*(const TArr<S,5>*)_TL("___ax"), NormalKeyOffset, pos)!=0);


	test(pArray->Find(*(const TArr<S,5>*)_TL("aa cc"), FoldedKey, pos)==0);
	test(pos==0);
	test(pArray->Find(*(const TArr<S,5>*)_TL("bb bb"), FoldedKey, pos)==0);
	test(pos==1);
	test(pArray->Find(*(const TArr<S,5>*)_TL("cc aa"), FoldedKey, pos)==0);
	test(pos==2);


	test(pArray->Find(*(const TArr<S,5>*)_TL("___CC"), FoldedKeyOffset, pos)==0);
	test(pos==0);
	test(pArray->Find(*(const TArr<S,5>*)_TL("___bB"), FoldedKeyOffset, pos)==0);
	test(pos==1);
	test(pArray->Find(*(const TArr<S,5>*)_TL("___Aa"), FoldedKeyOffset, pos)==0);
	test(pos==2);
	test(pArray->Find(*(const TArr<S,5>*)_TL("___ax"), FoldedKeyOffset, pos)!=0);


	test(pArray->Find(*(const TArr<S,5>*)_TL("aa cc"), CollatedKey, pos)==0);
	test(pos==0);
	test(pArray->Find(*(const TArr<S,5>*)_TL("bb bb"), CollatedKey, pos)==0);
	test(pos==1);
	test(pArray->Find(*(const TArr<S,5>*)_TL("cc aa"), CollatedKey, pos)==0);
	test(pos==2);


	test(pArray->Find(*(const TArr<S,5>*)_TL("___cc"), CollatedKeyOffset, pos)==0);
	test(pos==0);
	test(pArray->Find(*(const TArr<S,5>*)_TL("___bb"), CollatedKeyOffset, pos)==0);
	test(pos==1);
	test(pArray->Find(*(const TArr<S,5>*)_TL("___aa"), CollatedKeyOffset, pos)==0);
	test(pos==2);
	test(pArray->Find(*(const TArr<S,5>*)_TL("___ax"), CollatedKeyOffset, pos)!=0);

	delete pArray;
	}


template<class KeyType, class ArrayType, class S>
GLDEF_C void TestTKey<KeyType, ArrayType, S>::Test2(TKeyCmpText N, TKeyCmpText F, TKeyCmpText C)
	{
	// This tests the functions of the TKey classes indirectly - mostly using the Find method
	// which calls SetPtr(), Set() (both trivial) and more importantly Compare(), 

	ArrayType* pArray=new ArrayType(KTestGranularity);
	pArray->AppendL(*(S(*))_TL("aa cc"), 5*sizeof(S));
	pArray->AppendL(*(S(*))_TL("bb bbb"), 6*sizeof(S));
	pArray->AppendL(*(S(*))_TL("cc aaaa"), 7*sizeof(S));

	KeyType NormalKey5(0,N,5);		 
	KeyType NormalKey6(0,N,6);		
	KeyType NormalKey7(0,N,7);
	KeyType NormalKeyOffset(sizeof(S)*3,N,2); 

	KeyType FoldedKey5(0,F,5);
	KeyType FoldedKey6(0,F,6);
	KeyType FoldedKey7(0,F,7);
	KeyType FoldedKeyOffset(sizeof(S)*3,F,2);	

	KeyType CollatedKey5(0,C,5);
	KeyType CollatedKey6(0,C,6);
	KeyType CollatedKey7(0,C,7);
	KeyType CollatedKeyOffset(sizeof(S)*3,C,2);

	TInt pos;
	test(pArray->Find(*(S(*))_TL("aa cc"), NormalKey5, pos)==0);
	test(pos==0);
	test(pArray->Find(*(S(*))_TL("bb bbb"), NormalKey6, pos)==0);
	test(pos==1);
	test(pArray->Find(*(S(*))_TL("cc aaaa"), NormalKey7, pos)==0);
	test(pos==2);	
	test(pArray->Find(*(S(*))_TL("BB BB"), NormalKey5, pos)!=0);


	test(pArray->Find(*(S(*))_TL("___cc"), NormalKeyOffset, pos)==0);
	test(pos==0);
	test(pArray->Find(*(S(*))_TL("___bb"), NormalKeyOffset, pos)==0);
	test(pos==1);
	test(pArray->Find(*(S(*))_TL("___aa"), NormalKeyOffset, pos)==0);
	test(pos==2);  
	test(pArray->Find(*(S(*))_TL("___ax"), NormalKeyOffset, pos)!=0);


	test(pArray->Find(*(S(*))_TL("aa cc"), FoldedKey5, pos)==0);
	test(pos==0);
	test(pArray->Find(*(S(*))_TL("bb bbb"), FoldedKey6, pos)==0);
	test(pos==1);
	test(pArray->Find(*(S(*))_TL("cc aaaa"), FoldedKey7, pos)==0);
	test(pos==2);


	test(pArray->Find(*(S(*))_TL("___CC"), FoldedKeyOffset, pos)==0);
	test(pos==0);
	test(pArray->Find(*(S(*))_TL("___bB"), FoldedKeyOffset, pos)==0);
	test(pos==1);
	test(pArray->Find(*(S(*))_TL("___Aa"), FoldedKeyOffset, pos)==0);
	test(pos==2);
	test(pArray->Find(*(S(*))_TL("___ax"), FoldedKeyOffset, pos)!=0);


	test(pArray->Find(*(S(*))_TL("aa cc"), CollatedKey5, pos)==0);
	test(pos==0);
	test(pArray->Find(*(S(*))_TL("bb bbb"), CollatedKey6, pos)==0);
	test(pos==1);
	test(pArray->Find(*(S(*))_TL("cc aaaa"), CollatedKey7, pos)==0);
	test(pos==2);


	test(pArray->Find(*(S(*))_TL("___cc"), CollatedKeyOffset, pos)==0);
	test(pos==0);
	test(pArray->Find(*(S(*))_TL("___bb"), CollatedKeyOffset, pos)==0);
	test(pos==1);
	test(pArray->Find(*(S(*))_TL("___aa"), CollatedKeyOffset, pos)==0);
	test(pos==2);
	test(pArray->Find(*(S(*))_TL("___ax"), CollatedKeyOffset, pos)!=0);

	delete pArray;
	}


template<class KeyType, class ArrayType, class S>
GLDEF_C void TestTKey<KeyType, ArrayType, S>::Test3(void)
	{
	ArrayType* pArray=new ArrayType(KTestGranularity);
	Record rec1, rec2, rec3;

	rec1.name=(_TL("fred"));
	rec1.age=5;
	rec1.code='A';

	rec2.name=(_TL("bill"));
	rec2.age=0x7fffffff;
	rec2.code='Z';

	rec3.name=(_TL("bert"));
	rec3.age=-5;
	rec3.code='X';

	pArray->AppendL(rec1);
	pArray->AppendL(rec2);
	pArray->AppendL(rec3);
	
	TInt pos;	 

	KeyType codekey(_FOFF(Record, code),ECmpNormal,1);
	test(pArray->Find(rec1, codekey, pos)==0);
	test(pos==0);
	test(pArray->Find(rec2, codekey, pos)==0);
	test(pos==1);
	test(pArray->Find(rec3, codekey, pos)==0);
	test(pos==2);

	KeyType agekey(_FOFF(Record, age), ECmpTInt32);
	test(pArray->Find(rec1, agekey, pos)==0);
	test(pos==0);
	test(pArray->Find(rec2, agekey, pos)==0);
	test(pos==1);
	test(pArray->Find(rec3, agekey, pos)==0);
	test(pos==2);

	rec1.age=-50; // march 95 - this isn't allowed , lucky that it works
	test(pArray->Find(rec1, agekey, pos)!=0);
	rec1.age=5;

	pArray->Sort(agekey);
	test(pArray->Find(rec1, agekey, pos)==0);
	test(pos==1);
	test(pArray->Find(rec2, agekey, pos)==0);
	test(pos==2);
	test(pArray->Find(rec3, agekey, pos)==0);
	test(pos==0);

	delete pArray;
	}

template<class KeyType, class ArrayType, class S>
GLDEF_C void TestTKey<KeyType, ArrayType, S>::Test4(void)
	{
	ArrayType* pArray=new ArrayType(KTestGranularity);
	Record rec1, rec2, rec3;
	
	rec1.name=(_TL("fred"));
	rec1.age=5;
	rec1.code='A';
	rec2.name=(_TL("bill"));
	rec2.age=0x7fffffff;
	rec2.code='Z';
	rec3.name=(_TL("bert"));
	rec3.age=-5;
	rec3.code='X';

	pArray->AppendL(rec1, sizeof(Record));
	pArray->AppendL(rec2, sizeof(Record));
	pArray->AppendL(rec3, sizeof(Record));

	TInt pos;
	KeyType codekey(_FOFF(Record, code),ECmpNormal,1);
	test(pArray->Find(rec1, codekey, pos)==0);
	test(pos==0);
	test(pArray->Find(rec2, codekey, pos)==0);
	test(pos==1);
	test(pArray->Find(rec3, codekey, pos)==0);
	test(pos==2);

	KeyType agekey(_FOFF(Record, age), ECmpTInt32);
	test(pArray->Find(rec1, agekey, pos)==0);
	test(pos==0);
	test(pArray->Find(rec2, agekey, pos)==0);
	test(pos==1);
	test(pArray->Find(rec3, agekey, pos)==0);
	test(pos==2);
	rec1.age=-50;					// march 95 - this isn't allowed - lucky to get away with it
	test(pArray->Find(rec1, agekey, pos)!=0);
	rec1.age=5;

	pArray->Sort(agekey);
	test(pArray->Find(rec1, agekey, pos)==0);
	test(pos==1);
	test(pArray->Find(rec2, agekey, pos)==0);
	test(pos==2);
	test(pArray->Find(rec3, agekey, pos)==0);
	test(pos==0);

	delete pArray;
	}

template<class KeyType, class ArrayType, class S>
GLDEF_C void TestTKey<KeyType, ArrayType, S>::Test5(void)
	{
	// test the numeric enumeration types
	ArrayType* pArray=new ArrayType(KTestGranularity);
	TInt pos;
	
	KeyType TIntKey(_FOFF(Record2, tint), ECmpTInt);
	KeyType TUintKey(_FOFF(Record2, tuint), ECmpTUint);
	KeyType TInt8Key(_FOFF(Record2, tint8), ECmpTInt8);
	KeyType TUint8Key(_FOFF(Record2, tuint8), ECmpTUint8);
	KeyType TInt16Key(_FOFF(Record2, tint16), ECmpTInt16);
	KeyType TUint16Key(_FOFF(Record2, tuint16), ECmpTUint16);
	KeyType TInt32Key(_FOFF(Record2, tint32), ECmpTInt32);
	KeyType TUint32Key(_FOFF(Record2, tuint32), ECmpTUint32);

	SetRecordData();

	pArray->AppendL(Rec1);
	pArray->AppendL(Rec2);
	pArray->AppendL(Rec3);
	pArray->AppendL(Rec4);

	pArray->Sort(TIntKey);
	// order should be 4,3,2,1
	test(pArray->Find(Rec4, TIntKey, pos)==0);
	test(pos==0);
	test(pArray->Find(Rec3, TIntKey, pos)==0);
	test(pos==1);
	test(pArray->Find(Rec2, TIntKey, pos)==0);
	test(pos==2);
	test(pArray->Find(Rec1, TIntKey, pos)==0);
	test(pos==3);

	pArray->Sort(TUintKey);
	// order should be 1,2,3,4
	test(pArray->Find(Rec1, TUintKey, pos)==0);
	test(pos==0);
	test(pArray->Find(Rec2, TUintKey, pos)==0);
	test(pos==1);
	test(pArray->Find(Rec3, TUintKey, pos)==0);
	test(pos==2);
	test(pArray->Find(Rec4, TUintKey, pos)==0);
	test(pos==3);

	pArray->Sort(TInt8Key);
	// order should be 4,3,2,1
	test(pArray->Find(Rec4, TInt8Key, pos)==0);
	test(pos==0);
	test(pArray->Find(Rec3, TInt8Key, pos)==0);
	test(pos==1);
	test(pArray->Find(Rec2, TInt8Key, pos)==0);
	test(pos==2);
	test(pArray->Find(Rec1, TInt8Key, pos)==0);
	test(pos==3);

	pArray->Sort(TUint8Key);
	// order should be 1,2,3,4
	test(pArray->Find(Rec1, TUint8Key, pos)==0);
	test(pos==0);
	test(pArray->Find(Rec2, TUint8Key, pos)==0);
	test(pos==1);
	test(pArray->Find(Rec3, TUint8Key, pos)==0);
	test(pos==2);
	test(pArray->Find(Rec4, TUint8Key, pos)==0);
	test(pos==3);

	pArray->Sort(TInt16Key);
	// order should be 4,3,2,1
	test(pArray->Find(Rec4, TInt16Key, pos)==0);
	test(pos==0);
	test(pArray->Find(Rec3, TInt16Key, pos)==0);
	test(pos==1);
	test(pArray->Find(Rec2, TInt16Key, pos)==0);
	test(pos==2);
	test(pArray->Find(Rec1, TInt16Key, pos)==0);
	test(pos==3);

	pArray->Sort(TUintKey);
	// order should be 1,2,3,4
	test(pArray->Find(Rec1, TUint16Key, pos)==0);
	test(pos==0);
	test(pArray->Find(Rec2, TUint16Key, pos)==0);
	test(pos==1);
	test(pArray->Find(Rec3, TUint16Key, pos)==0);
	test(pos==2);
	test(pArray->Find(Rec4, TUint16Key, pos)==0);
	test(pos==3);

	pArray->Sort(TInt32Key);
	// order should be 4,3,2,1
	test(pArray->Find(Rec4, TInt32Key, pos)==0);
	test(pos==0);
	test(pArray->Find(Rec3, TInt32Key, pos)==0);
	test(pos==1);
	test(pArray->Find(Rec2, TInt32Key, pos)==0);
	test(pos==2);
	test(pArray->Find(Rec1, TInt32Key, pos)==0);
	test(pos==3);

	pArray->Sort(TUint32Key);
	// order should be 1,2,3,4
	test(pArray->Find(Rec1, TUint32Key, pos)==0);
	test(pos==0);
	test(pArray->Find(Rec2, TUint32Key, pos)==0);
	test(pos==1);
	test(pArray->Find(Rec3, TUint32Key, pos)==0);
	test(pos==2);
	test(pArray->Find(Rec4, TUint32Key, pos)==0);
	test(pos==3);

	delete pArray;
	}

template<class KeyType, class ArrayType, class S>
GLDEF_C void TestTKey<KeyType, ArrayType, S>::Test6(void)
	{
	// test the numeric enumeration types
	ArrayType* pArray=new ArrayType(KTestGranularity);
	TInt pos;
	
	KeyType TIntKey(_FOFF(Record2, tint), ECmpTInt);
	KeyType TUintKey(_FOFF(Record2, tuint), ECmpTUint);
	KeyType TInt8Key(_FOFF(Record2, tint8), ECmpTInt8);
	KeyType TUint8Key(_FOFF(Record2, tuint8), ECmpTUint8);
	KeyType TInt16Key(_FOFF(Record2, tint16), ECmpTInt16);
	KeyType TUint16Key(_FOFF(Record2, tuint16), ECmpTUint16);
	KeyType TInt32Key(_FOFF(Record2, tint32), ECmpTInt32);
	KeyType TUint32Key(_FOFF(Record2, tuint32), ECmpTUint32);

	SetRecordData();

	pArray->AppendL(Rec1, sizeof(Record2));
	pArray->AppendL(Rec2, sizeof(Record2));
	pArray->AppendL(Rec3, sizeof(Record2));
	pArray->AppendL(Rec4, sizeof(Record2));

	pArray->Sort(TIntKey);
	// order should be 4,3,2,1
	test(pArray->Find(Rec4, TIntKey, pos)==0);
	test(pos==0);
	test(pArray->Find(Rec3, TIntKey, pos)==0);
	test(pos==1);
	test(pArray->Find(Rec2, TIntKey, pos)==0);
	test(pos==2);
	test(pArray->Find(Rec1, TIntKey, pos)==0);
	test(pos==3);

	pArray->Sort(TUintKey);
	// order should be 1,2,3,4
	test(pArray->Find(Rec1, TUintKey, pos)==0);
	test(pos==0);
	test(pArray->Find(Rec2, TUintKey, pos)==0);
	test(pos==1);
	test(pArray->Find(Rec3, TUintKey, pos)==0);
	test(pos==2);
	test(pArray->Find(Rec4, TUintKey, pos)==0);
	test(pos==3);

	pArray->Sort(TInt8Key);
	// order should be 4,3,2,1
	test(pArray->Find(Rec4, TInt8Key, pos)==0);
	test(pos==0);
	test(pArray->Find(Rec3, TInt8Key, pos)==0);
	test(pos==1);
	test(pArray->Find(Rec2, TInt8Key, pos)==0);
	test(pos==2);
	test(pArray->Find(Rec1, TInt8Key, pos)==0);
	test(pos==3);

	pArray->Sort(TUint8Key);
	// order should be 1,2,3,4
	test(pArray->Find(Rec1, TUint8Key, pos)==0);
	test(pos==0);
	test(pArray->Find(Rec2, TUint8Key, pos)==0);
	test(pos==1);
	test(pArray->Find(Rec3, TUint8Key, pos)==0);
	test(pos==2);
	test(pArray->Find(Rec4, TUint8Key, pos)==0);
	test(pos==3);

	pArray->Sort(TInt16Key);
	// order should be 4,3,2,1
	test(pArray->Find(Rec4, TInt16Key, pos)==0);
	test(pos==0);
	test(pArray->Find(Rec3, TInt16Key, pos)==0);
	test(pos==1);
	test(pArray->Find(Rec2, TInt16Key, pos)==0);
	test(pos==2);
	test(pArray->Find(Rec1, TInt16Key, pos)==0);
	test(pos==3);

	pArray->Sort(TUintKey);
	// order should be 1,2,3,4
	test(pArray->Find(Rec1, TUint16Key, pos)==0);
	test(pos==0);
	test(pArray->Find(Rec2, TUint16Key, pos)==0);
	test(pos==1);
	test(pArray->Find(Rec3, TUint16Key, pos)==0);
	test(pos==2);
	test(pArray->Find(Rec4, TUint16Key, pos)==0);
	test(pos==3);

	pArray->Sort(TInt32Key);
	// order should be 4,3,2,1
	test(pArray->Find(Rec4, TInt32Key, pos)==0);
	test(pos==0);
	test(pArray->Find(Rec3, TInt32Key, pos)==0);
	test(pos==1);
	test(pArray->Find(Rec2, TInt32Key, pos)==0);
	test(pos==2);
	test(pArray->Find(Rec1, TInt32Key, pos)==0);
	test(pos==3);

	pArray->Sort(TUint32Key);
	// order should be 1,2,3,4
	test(pArray->Find(Rec1, TUint32Key, pos)==0);
	test(pos==0);
	test(pArray->Find(Rec2, TUint32Key, pos)==0);
	test(pos==1);
	test(pArray->Find(Rec3, TUint32Key, pos)==0);
	test(pos==2);
	test(pArray->Find(Rec4, TUint32Key, pos)==0);
	test(pos==3);

	delete pArray;
	}

GLDEF_C TInt E32Main()
    {

	test.Title();	
	test.Start(_L("Fixed key class with a flat array of TText8"));
	typedef CArrayFixFlat<TArr<TText8,5> >  aFixedFlatArrayOfTText8;		
	TestTKey<TKeyArrayFix, aFixedFlatArrayOfTText8, TText8> T1;
	T1.Test1(ECmpNormal8, ECmpFolded8, ECmpCollated8);
  
	test.Next(_L("Fixed key class with a flat array of TText16"));
	typedef CArrayFixFlat<TArr<TText16,5> >  aFixedFlatArrayOfTText16;		
	TestTKey<TKeyArrayFix, aFixedFlatArrayOfTText16, TText16> T2;
	T2.Test1(ECmpNormal16, ECmpFolded16, ECmpCollated16);

	test.Next(_L("Fixed key class with a flat array of TText"));
	typedef CArrayFixFlat<TArr<TText,5> >  aFixedFlatArrayOfTText;		
	TestTKey<TKeyArrayFix, aFixedFlatArrayOfTText, TText> T3;
	T3.Test1(ECmpNormal, ECmpFolded, ECmpCollated);

	test.Next(_L("Fixed key class with a segmented array of TText8"));
	typedef CArrayFixSeg<TArr<TText8,5> >  aFixedSegmentedArrayOfTText8;		
	TestTKey<TKeyArrayFix, aFixedSegmentedArrayOfTText8, TText8> T4;
	T4.Test1(ECmpNormal8, ECmpFolded8, ECmpCollated8);
						   
	test.Next(_L("Fixed key class with a segmented array of TText16"));
	typedef CArrayFixSeg<TArr<TText16,5> >  aFixedSegmentedArrayOfTText16;		
	TestTKey<TKeyArrayFix, aFixedSegmentedArrayOfTText16, TText16> T5;
	T5.Test1(ECmpNormal16, ECmpFolded16, ECmpCollated16);

	test.Next(_L("Fixed key class with a segmented array of TText"));
	typedef CArrayFixSeg<TArr<TText,5> >  aFixedSegmentedArrayOfTText;		
	TestTKey<TKeyArrayFix, aFixedSegmentedArrayOfTText, TText> T6;
	T6.Test1(ECmpNormal, ECmpFolded, ECmpCollated);

	test.Next(_L("Var key with a flat array of TText8"));
	typedef CArrayVarFlat<TText8> aVarFlatArrayOfTText8;
	TestTKey<TKeyArrayVar, aVarFlatArrayOfTText8, TText8> T7;
	T7.Test2(ECmpNormal8, ECmpFolded8, ECmpCollated8);

	test.Next(_L("Var key with a flat array of TText16"));
	typedef CArrayVarFlat<TText16> aVarFlatArrayOfTText16;
	TestTKey<TKeyArrayVar, aVarFlatArrayOfTText16, TText16> T8;
	T8.Test2(ECmpNormal16, ECmpFolded16, ECmpCollated16);

	test.Next(_L("Var key with a flat array of TText"));
	typedef CArrayVarFlat<TText> aVarFlatArrayOfTText;
	TestTKey<TKeyArrayVar, aVarFlatArrayOfTText, TText> T9;
	T9.Test2(ECmpNormal, ECmpFolded, ECmpCollated);

	test.Next(_L("Var key with a segmented array of TText8"));
	typedef CArrayVarSeg<TText8> aVarSegmentedArrayOfTText8;
	TestTKey<TKeyArrayVar, aVarSegmentedArrayOfTText8, TText8> T10;
	T10.Test2(ECmpNormal8, ECmpFolded8, ECmpCollated8);	

	test.Next(_L("Var key with a segmented array of TText16"));
	typedef CArrayVarSeg<TText16> aVarSegmentedArrayOfTText16;
	TestTKey<TKeyArrayVar, aVarSegmentedArrayOfTText16, TText16> T11;
	T11.Test2(ECmpNormal16, ECmpFolded16, ECmpCollated16);

	test.Next(_L("Var key with a segmented array of TText"));
	typedef CArrayVarSeg<TText> aVarSegmentedArrayOfTText;
	TestTKey<TKeyArrayVar, aVarSegmentedArrayOfTText, TText> T12;
	T12.Test2(ECmpNormal, ECmpFolded, ECmpCollated);

	test.Next(_L("Fixed key with a flat array of structs"));
	typedef CArrayFixFlat<Record> aFixedFlatArrayOfRecord;
	TestTKey<TKeyArrayFix, aFixedFlatArrayOfRecord, TText> T13;
	T13.Test3();

	test.Next(_L("Fixed key with a segmented array of structs"));
	typedef CArrayFixSeg<Record> aFixedSegmentedArrayOfRecord;
	TestTKey<TKeyArrayFix, aFixedSegmentedArrayOfRecord, TText> T14;
	T14.Test3();
  
	test.Next(_L("Var key with a flat array of structs"));
	typedef CArrayVarFlat<Record> aVarFlatArrayOfRecord;
	TestTKey<TKeyArrayVar, aVarFlatArrayOfRecord, TText> T15;
	T15.Test4();

	test.Next(_L("Var key with a segmented array of structs"));
	typedef CArrayVarSeg<Record> aVarSegmentedArrayOfRecord;
	TestTKey<TKeyArrayVar, aVarSegmentedArrayOfRecord, TText> T16;
	T16.Test4();

	test.Next(_L("Fixed key with a flat array of structs testing numeric types"));
	typedef CArrayFixFlat<Record2> aFixedFlatArrayOfRecord2;
	TestTKey<TKeyArrayFix, aFixedFlatArrayOfRecord2, TText> T17;
	T17.Test5();   

	test.Next(_L("Var key with a flat array of structs testing numeric types"));
	typedef CArrayVarFlat<Record2> aVarFlatArrayOfRecord2;
	TestTKey<TKeyArrayVar, aVarFlatArrayOfRecord2, TText> T18;
	T18.Test6();
	test.End();
	return(KErrNone);
    }

/*#else
GLDEF_C TInt E32Main()
//
// Test unavailable in release build.
//
    {

	test.Title();	
	test.Start(_L("No tests for release builds"));
	test.End();
	return(0);
    }
#endif

*/

