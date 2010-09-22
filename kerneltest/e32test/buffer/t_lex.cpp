// Copyright (c) 1994-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\buffer\t_lex.cpp
// Overview:
// Test TLex and TLexMark classes.
// API Information:
// TLex, TLexMark.
// Details:
// - For Unicode, non Unicode and build independent variant of TLex class
// - Test that string-parsing methods are present.
// - Test the constructors with no parameter, by a string, with an empty TLex class, 
// non-empty TLex class is as expected.
// - Test assignment operator of TLex, by initializing with TLex reference, string, 
// TBuf reference and check it is as expected.
// - Check that Eos, Inc, Mark, Get, Peek, UnGet, SkipSpace, SkipSpaceAndMark, 
// SkipAndMark, SkipCharacters, TokenLength, MarkedToken methods are as expected.
// - Initialize Lex string, assign different values, Parse to extract signed, 
// unsigned integer of different lengths, using specified radix and verify 
// that the return value is KErrNone when a valid string is parsed, KErrGeneral 
// when invalid string is parsed, KErrOverflow when converted value is greater 
// than the limit. 
// - Refresh the contents with the system's locale settings, separate whole number from 
// it's fractional part, change locale settings then parse a 64-bit floating point 
// number and check that results are as expected.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("T_LEX")); 

struct TLexMark8Dump
	{
	const TUint8* iPtr;
	};

struct TLexMark16Dump
	{
	const TUint16* iPtr;
	};

void TLexMark8::__DbgTest(void *pTLexMark8Dump) const 
	{
	((TLexMark8Dump*)pTLexMark8Dump)->iPtr=iPtr;
	}

void TLexMark16::__DbgTest(void *pTLexMark16Dump) const 
	{
	((TLexMark16Dump*)pTLexMark16Dump)->iPtr=iPtr;
	}

struct TLex8Dump
	{
	const TUint8* iNext;
	const TUint8* iBuf;
	const TUint8* iEnd;
	TLexMark8Dump iMark;
	};

struct TLex16Dump
	{
	const TUint16* iNext;
	const TUint16* iBuf;
	const TUint16* iEnd;
	TLexMark16Dump iMark;
	};


void TLex8::__DbgTest(void* pTLex8Dump) const
	{
	((TLex8Dump*)pTLex8Dump)->iNext=iNext;
	((TLex8Dump*)pTLex8Dump)->iBuf=iBuf;
	((TLex8Dump*)pTLex8Dump)->iEnd=iEnd;
	iMark.__DbgTest(&((TLex8Dump*)pTLex8Dump)->iMark);
	}

void TLex16::__DbgTest(void* pTLex16Dump) const
	{
	((TLex16Dump*)pTLex16Dump)->iNext=iNext;
	((TLex16Dump*)pTLex16Dump)->iBuf=iBuf;
	((TLex16Dump*)pTLex16Dump)->iEnd=iEnd;
	iMark.__DbgTest(&((TLex16Dump*)pTLex16Dump)->iMark);
	}


LOCAL_C	void TestDes(const TUint16* start1, const TUint16* start2, const TUint16* end1, const TUint16* end2)
	{
	TPtrC16 des1(start1, end1-start1);
	TPtrC16 des2(start2, end2-start2);
	test(des1==des2);
	}
LOCAL_C	void TestDes(const TUint8* start1, const TUint8* start2, const TUint8* end1, const TUint8* end2)
	{
	TPtrC8 des1(start1, end1-start1);
	TPtrC8 des2(start2, end2-start2);
	test(des1==des2);
	}
LOCAL_C void TestDes(const TUint16* start, const TUint16* end, const TUint16* string)
	{
	TPtrC16 des1(start, end-start);
	TPtrC16 des2(string);
	test(des1==des2);
	}
LOCAL_C void TestDes(const TUint8* start, const TUint8* end, const TUint8* string)
	{
	TPtrC8 des1(start, end-start);
	TPtrC8 des2(string);
	test(des1==des2);
	}


void _LL(TText8 array[], TText8 string[])
	{

	TInt index=0;
	for(;string[index]; index++)
		array[index]=string[index];
	array[index]='\0';
	}		

void _LL(TText16 array[], TText8 string[])
	{

	TInt index=0;
	for(;string[index]; index++)
		array[index]=string[index];
	array[index]='\0';
	}
  
template<class TDesType, class TLexType, class TLexMarkType, class TBufType, class DumpType, class MarkDumpType, class S>
class TestTLex
	{
public:
	void Test1();
	void Test2();
	void Test3();
	void Test4();
	void Test5();
protected:
	void TestList(TLexType* object, TUint param, ...);
	};


//*********************************
// Test that methods are in the DLL
//*********************************
template<class TDesType, class TLexType, class TLexMarkType, class TBufType, class DumpType, class MarkDumpType, class S>
GLDEF_C void TestTLex<TDesType, TLexType, TLexMarkType, TBufType, DumpType, MarkDumpType, S>::Test1()
	{
	test.Start(_L("Constructors"));
	S String[100];
	_LL(&String[0], (TText8*)"hello");
	TBufType aTBufObject;
	TLexType a;								
	TLexType b(a);						   
	TLexType c(&String[0]);					
	TLexType d(aTBufObject);
    d.Offset(); // Avoids 'unused' warning
	TLexMarkType mark;

	test.Next(_L("Assignment operations"));
	a=b;							
	a=&String[0];					
	a=aTBufObject;					

	test.Next(_L("Assorted"));
	c.Eos();
	c.Mark(mark);
	c.Mark();
	c.Inc();
	c.Inc(2);
	c.Get();
	(S)c.Peek();
	c.UnGet();
	c.UnGetToMark(mark);
	c.UnGetToMark();
	c.SkipSpace();
	c.SkipSpaceAndMark(mark);
	c.SkipSpaceAndMark();
	c.SkipAndMark(2,mark);
	c.SkipCharacters();
	c.TokenLength(mark);
	c.TokenLength();
	aTBufObject=c.MarkedToken(mark);
	aTBufObject=c.MarkedToken();
	aTBufObject=c.NextToken();
	aTBufObject=c.Remainder();
	aTBufObject=c.RemainderFromMark(mark);
	aTBufObject=c.RemainderFromMark();
	c.Offset();
	c.MarkedOffset(mark);
	c.MarkedOffset();

	test.Next(_L("Val"));  
	TInt TI=1;
	c.Val(TI);
	TInt8 TI8='a';
	c.Val(TI8);
	TInt16 TI16=1;
	c.Val(TI16);
	TInt32 TI32=1;
	c.Val(TI32);
	TInt64 TI64=1;
	c.Val(TI64);
	TUint TU=1;
	c.Val(TU);
	TReal32 TR32=1.0F;
	c.Val(TR32);
	c.Val(TR32,'.');
	TReal64 TR64=1.0;
	c.Val(TR64);
	TUint8 TU8='a';
	TUint32 TU32=1;
	TRadix TR=(TRadix)EDecimal;
	TUint16 TU16=1;

	c.Val(TU8, TR);
	c.Val(TU16, TR);  	
	c.BoundedVal(TI32, TU);
	c.BoundedVal(TU32, TR, TU);
	c.BoundedVal(TI64, TR, TU);

	test.Next(_L("Assign"));
	c.Assign(b);
	c.Assign(&String[0]);
	c.Assign(aTBufObject);	

	test.Next(_L("Test Dumps"));
	MarkDumpType mDump;
	mark.__DbgTest(&mDump);

	DumpType dump;
	c.__DbgTest(&dump);
	test.End();
	} 
		

///////////////////////////////////////
// Test calling Convert() with  a list
///////////////////////////////////////
template<class TDesType, class TLexType, class TLexMarkType, class TBufType, class DumpType, class MarkDumpType, class S>
GLDEF_C void TestTLex<TDesType, TLexType, TLexMarkType, TBufType, DumpType, MarkDumpType, S>::TestList(TLexType *object, TUint param, ...)
	{
	VA_LIST l;
	TBufType b;

	VA_START(l, param);
	object->Convert(b, l);
	}


/////////////////////////
// Test the constructors
////////////////////////
template<class TDesType, class TLexType, class TLexMarkType, class TBufType, class DumpType, class MarkDumpType, class S>
GLDEF_C void TestTLex<TDesType, TLexType, TLexMarkType, TBufType, DumpType, MarkDumpType, S>::Test2()
	{
	// Test constructors  NOTE: there's no getters for iMark or iBuf
	DumpType dump, dump2;
	S String[40];

	test.Start(_L("Constructors:"));	// TLexx::TLexx()
	test.Next(_L("By default"));
	TLexType a; 
	a.__DbgTest(&dump);
	test(dump.iBuf==NULL);
 	
	test.Next(_L("By string"));		 	// TLexx::TLexx(const TUintx*)
	_LL(&String[0], (TText8*)"AB");
	TLexType b(&String[0]);
	b.__DbgTest(&dump);
	TestDes(dump.iNext, dump.iEnd, &String[0]);
	TestDes(dump.iBuf, dump.iEnd, &String[0]);
	TestDes(dump.iMark.iPtr, dump.iEnd, &String[0]);

	test.Next(_L("By TLex reference"));	// TLexx::TLexx(const TLexx&)
	// Test with an empty class
	TLexType c, d(c);
	c.__DbgTest(&dump);
	d.__DbgTest(&dump2);
	TestDes(dump.iNext, dump2.iNext, dump.iEnd, dump2.iEnd);
	test(dump.iBuf==NULL);
	test(dump.iBuf==dump2.iBuf); 
	TestDes(dump.iMark.iPtr, dump2.iMark.iPtr, dump.iEnd, dump2.iEnd);

	//Test with a non empty class
	_LL(&String[0], (TText8*)"XYZ");
	TLexType e(&String[0]), f(e);
	e.__DbgTest(&dump);
	f.__DbgTest(&dump2);
	TestDes(dump.iNext, dump.iEnd, &String[0]);
	TestDes(dump.iNext, dump2.iNext, dump.iEnd, dump2.iEnd);
	TestDes(dump.iBuf, dump2.iBuf, dump.iEnd, dump2.iEnd);
	TestDes(dump.iMark.iPtr, dump2.iMark.iPtr, dump.iEnd, dump2.iEnd);
	
	test.Next(_L("By TBuf reference"));	//TLexx::TLexx(const TBufBasex&)
	_LL(&String[0], (TText8*)"Hello");
	TBufType aBuf(&String[0]);
	TLexType g(aBuf);
	g.__DbgTest(&dump);
	TestDes(dump.iNext, dump.iEnd, &String[0]);
	TestDes(dump.iBuf, dump.iEnd, &String[0]);
	TestDes(dump.iMark.iPtr, dump.iEnd, &String[0]);

	test.End();
	}


//*********************************
// Test the = methods
//*********************************
template<class TDesType, class TLexType, class TLexMarkType, class TBufType, class DumpType, class MarkDumpType, class S>
GLDEF_C void TestTLex<TDesType, TLexType, TLexMarkType, TBufType, DumpType, MarkDumpType, S>::Test3()
	{
	DumpType dump, dump2;
	S String[40];

	test.Start(_L("= operators"));
	test.Next(_L("by TLex reference"));	//TLexx::operator=(const TLexx&)
	_LL(&String[0], (TText8*)"MNO");
	TLexType a(&String[0]), b;
	b=a;	
	a.__DbgTest(&dump);
	b.__DbgTest(&dump2);  
	TestDes(dump.iNext, dump2.iNext, dump.iEnd, dump2.iEnd); 
	TestDes(dump.iMark.iPtr, dump2.iMark.iPtr, dump.iEnd, dump2.iEnd);
	TestDes(dump.iBuf, dump2.iBuf, dump.iEnd, dump2.iEnd);

	test.Next(_L("by string"));			//TLexx::operator=(const TUintx*)
	TLexType c;
	_LL(&String[0], (TText8*)" abc");
	c=&String[0];
	c.__DbgTest(&dump);
	TestDes(dump.iNext, dump.iEnd, &String[0]);	
	TestDes(dump.iBuf, dump.iEnd, &String[0]);
	TestDes(dump.iMark.iPtr, dump.iEnd, &String[0]);

	test.Next(_L("by TBuf reference"));	//TLexx::operator=(const TBufx&);
	_LL(&String[0], (TText8*)"PQ R ");
	TLexType d;
	TBufType e(&String[0]);
	d=e;
	d.__DbgTest(&dump);
	TestDes(dump.iNext, dump.iEnd, &String[0]);	
	TestDes(dump.iBuf, dump.iEnd, &String[0]);
	TestDes(dump.iMark.iPtr, dump.iEnd, &String[0]);	
	test.End();	 		 
	}


//*********************************
// Test supporting methods
//*********************************
template<class TDesType, class TLexType, class TLexMarkType, class TBufType, class DumpType, class MarkDumpType, class S>
GLDEF_C void TestTLex<TDesType, TLexType, TLexMarkType, TBufType, DumpType, MarkDumpType, S>::Test4()
	{
	S String[40];
	DumpType dump1,dump2;
	MarkDumpType mDump;

	test.Start(_L("Supporting methods"));
	test.Next(_L("Eos()"));
	_LL(&String[0], (TText8*)"GGG");
	TLexType a, b(&String[0]);
	test(a.Eos()==TRUE);
	test(b.Eos()==FALSE);

	test.Next(_L("Inc()"));	   // Inc() increments iNext
	_LL(&String[0], (TText8*)"mno");
	TLexType c(&String[0]);
	c.__DbgTest(&dump1);

	TestDes(dump1.iNext, dump1.iEnd, &String[0]);
	c.Inc();
	test((S)c.Peek()==String[1]);
	c.Inc();
	test((S)c.Peek()==String[2]);
	
	test.Next(_L("Inc(aNumber)"));    // Inc(aNumber) increments iNext
	_LL(&String[0], (TText8*)"mno");
	TLexType c1(&String[0]);
	c1.__DbgTest(&dump1);

	TestDes(dump1.iNext, dump1.iEnd, &String[0]);
	c1.Inc(1);
	test((S)c1.Peek()==String[1]);
	c1.Inc(1);
	test((S)c1.Peek()==String[2]);

	test.Next(_L("Mark()"));		//	Mark() sets iMark=iNext
	_LL(&String[0], (TText8*)"pqr");
	TLexType d(&String[0]);
	d.Inc();
	d.__DbgTest(&dump1);
	d.Mark();
	d.__DbgTest(&dump2);
	TestDes(dump2.iMark.iPtr, dump1.iNext, dump2.iEnd, dump1.iEnd);

	test.Next(_L("Mark(mark)")); //	Mark(aMark) sets aMark=iNext
	_LL(&String[0], (TText8*)"pqr");
	TLexType d1(&String[0]);
	TLexMarkType dm;
	d1.Inc();
	d1.__DbgTest(&dump1);
	d1.Mark(dm);
	dm.__DbgTest(&mDump);
	TestDes(mDump.iPtr, dump1.iNext, dump1.iEnd, dump1.iEnd);


	test.Next(_L("Get()")); 	// Get() is {TChar c(*iNext);Inc(); return(c);}
	_LL(&String[0], (TText8*)"s");
	TLexType e(&String[0]);
	TChar temp=e.Get();
	test(temp=='s');
	e.Inc();
	temp=e.Get();
	test(temp==0);


	test.Next(_L("Peek()"));	// Peek() returns *iNext
	TLexType f;
	test(f.Peek()==0);
	_LL(&String[0], (TText8*)"ab");
	TLexType g(&String[0]);
	test((S)g.Peek()==String[0]);

	test.Next(_L("UnGet()"));	//  UnGet() is effectively if(iNext!=iBuf) iNext--;
	_LL(&String[0], (TText8*)"abc");
	TLexType h(&String[0]);
	h.Inc();
	test((S)h.Peek()==String[1]);
	h.UnGet();
	test((S)h.Peek()==String[0]);

	test.Next(_L("SkipSpace()"));	// SkipSpace() 	is while(Peek.IsSpace()) iNext++;
	_LL(&String[0], (TText8*)"  j  kl");
	TLexType i(&String[0]);
	i.SkipSpace();
	test((S)i.Peek()==String[2]);
	i.Inc();
	i.SkipSpace();
	test((S)i.Peek()==String[5]);

	test.Next(_L("SkipSpaceAndMark()"));	// while(Peek.IsSpace()) iNext++;	iMark=iNext;
	_LL(&String[0], (TText8*)"    aaa");
	TLexType j(&String[0]);
	j.SkipSpaceAndMark();
	j.__DbgTest(&dump1);
	_LL(&String[0], (TText8*)"aaa");
	TestDes(dump1.iNext, dump1.iEnd, &String[0]);
	TestDes(dump1.iMark.iPtr, dump1.iEnd, &String[0]);

	test.Next(_L("SkipSpaceAndMark(aMark)"));	// while(Peek.IsSpace()) iNext++;	iMark=iNext;
	_LL(&String[0], (TText8*)"    aaa");
	TLexType j1(&String[0]);
	TLexMarkType jm;
	j1.SkipSpaceAndMark(jm);
	j1.__DbgTest(&dump1);
	jm.__DbgTest(&mDump);
	_LL(&String[0], (TText8*)"aaa");
	TestDes(dump1.iNext, dump1.iEnd, &String[0]);
	TestDes(mDump.iPtr, dump1.iEnd, &String[0]);
	
	test.Next(_L("SkipAndMark(4, aMark)"));  // Skips number of characters
	_LL(&String[0], (TText8*)"abcdaaa");
	TLexType j2(&String[0]);
	TLexMarkType jmt;
	j2.SkipAndMark(4, jmt);
	j2.__DbgTest(&dump1);
	jmt.__DbgTest(&mDump);
	_LL(&String[0], (TText8*)"aaa");
	TestDes(dump1.iNext, dump1.iEnd, &String[0]);
	TestDes(mDump.iPtr, dump1.iEnd, &String[0]);

	test.Next(_L("SkipCharacters()"));	// Skips non whitespace characters
	_LL(&String[0], (TText8*)"abc   ");
	TLexType k(&String[0]);
	k.SkipCharacters();
	test((S)k.Peek()==String[3]);

	test.Next(_L("TokenLength()"));		// returns iNext-iMark
	_LL(&String[0], (TText8*)"GGG");
	TLexType l(&String[0]);
	test(l.TokenLength()==0);
	l.Inc();
	test(l.TokenLength()==1); 

	test.Next(_L("MarkedToken()"));		// Extract a marked token
	_LL(&String[0], (TText8*)"ABCD");
	TLexType m(&String[0]);
	TBufType Buf;
	TLexMarkType mm;
	m.Inc();
	m.Mark();
	m.Inc();
	m.Mark(mm);
	m.Inc();
	Buf=m.MarkedToken();
	S String2[4];
	_LL(&String2[0], (TText8*)"BC");
	test(TDesType(&String2[0])==Buf); 
	_LL(&String2[0], (TText8*)"C");
	Buf=m.MarkedToken(mm);
	test(TDesType(&String2[0])==Buf); 

	test.End();
	}


//*********************************
// Test Val()
//*********************************
template<class TDesType, class TLexType, class TLexMarkType, class TBufType, class DumpType, class MarkDumpType, class S>
GLDEF_C void TestTLex<TDesType, TLexType, TLexMarkType, TBufType, DumpType, MarkDumpType, S>::Test5()
	{
	S String[66];
	TInt ret; 
	TLexType Lex;
	test.Start(_L("Val()"));

	//////////////////
	// Test Val(TInt8)
	/////////////////
	test.Next(_L("Val(TInt8)"));
	TInt8 T8;
	_LL(&String[0], (TText8*)"");
	Lex=&String[0];
	test((ret=Lex.Val(T8))==KErrGeneral);

	_LL(&String[0], (TText8*)"abc");
	Lex=&String[0];
	test((ret=Lex.Val(T8))==KErrGeneral);

	_LL(&String[0], (TText8*)"-abc-");
	Lex=&String[0];
	test((ret=Lex.Val(T8))==KErrGeneral);
	
	_LL(&String[0], (TText8*)"+abc+");
	Lex=&String[0];
	test((ret=Lex.Val(T8))==KErrGeneral);

	_LL(&String[0], (TText8*)"0000000123abc");
	Lex=&String[0];
	test((ret=Lex.Val(T8))==KErrNone);
	test(T8==123);

	_LL(&String[0], (TText8*)"000");
	Lex=&String[0];
	test((ret=Lex.Val(T8))==KErrNone);
	test(T8==0);

	_LL(&String[0], (TText8*)"+0");
	Lex=&String[0];
	test((ret=Lex.Val(T8))==KErrNone);
	test(T8==0);

	_LL(&String[0], (TText8*)"-0");
	Lex=&String[0];
	test((ret=Lex.Val(T8))==KErrNone);
	test(T8==0);

	_LL(&String[0], (TText8*)"+1 ");
	Lex=&String[0];
	test((ret=Lex.Val(T8))==KErrNone);
	test(T8==1);

	_LL(&String[0], (TText8*)"-1 ");
	Lex=&String[0];
	test((ret=Lex.Val(T8))==KErrNone);
	test(T8==-1);

	_LL(&String[0], (TText8*)"127");
	Lex=&String[0];
	test((ret=Lex.Val(T8))==KErrNone);
	test(T8==127);

	_LL(&String[0], (TText8*)"128");
	Lex=&String[0];
	test((ret=Lex.Val(T8))==KErrOverflow);

	_LL(&String[0], (TText8*)"-128");
	Lex=&String[0];
	test((ret=Lex.Val(T8))==KErrNone);
	test(T8==-128);

	_LL(&String[0], (TText8*)"-129");
	Lex=&String[0];
	test((ret=Lex.Val(T8))==KErrOverflow);


	///////////////////
	// Test Val(TInt16)
	///////////////////
	test.Next(_L("Val(TInt16)"));
	TInt16 T16;
	_LL(&String[0], (TText8*)"");
	Lex=&String[0];
	test((ret=Lex.Val(T16))==KErrGeneral);

	_LL(&String[0], (TText8*)"32767");
	Lex=&String[0];
	test((ret=Lex.Val(T16))==KErrNone);
	test(T16==32767);

	_LL(&String[0], (TText8*)"32768");
	Lex=&String[0];
	test((ret=Lex.Val(T16))==KErrOverflow);

	_LL(&String[0], (TText8*)"-32768");
	Lex=&String[0];
	test((ret=Lex.Val(T16))==KErrNone);
	test(T16==-32768);

	_LL(&String[0], (TText8*)"-32769");
	Lex=&String[0];
	test((ret=Lex.Val(T16))==KErrOverflow);


	///////////////////
	// Test Val(TInt32)
	///////////////////
	test.Next(_L("Val(TInt32)"));
	TInt32 T32;
	_LL(&String[0], (TText8*)"");
	Lex=&String[0];
	test((ret=Lex.Val(T32))==KErrGeneral);

	_LL(&String[0], (TText8*)"2147483647");
	Lex=&String[0];
	test((ret=Lex.Val(T32))==KErrNone);
	test(T32==2147483647L);

	_LL(&String[0], (TText8*)"2147483648");
	Lex=&String[0];
	test((ret=Lex.Val(T32))==KErrOverflow);

	_LL(&String[0], (TText8*)"-2147483648");
	Lex=&String[0];
	test((ret=Lex.Val(T32))==KErrNone);
	test(T32==-2147483647-1);  // the -1 prevents a (bug?) warning

	_LL(&String[0], (TText8*)"-2147483649");
	Lex=&String[0];
	test((ret=Lex.Val(T32))==KErrOverflow);


	/////////////////
	// Test Val(TInt)
	/////////////////
	test.Next(_L("Val(TInt)"));
	TInt T;
	_LL(&String[0], (TText8*)"");
	Lex=&String[0];
	test((ret=Lex.Val(T))==KErrGeneral);

	_LL(&String[0], (TText8*)"2147483647");
	Lex=&String[0];
	test((ret=Lex.Val(T))==KErrNone);
	test(T==2147483647L);

	_LL(&String[0], (TText8*)"2147483648");
	Lex=&String[0];
	test((ret=Lex.Val(T))==KErrOverflow);

	_LL(&String[0], (TText8*)"-2147483648");
	Lex=&String[0];
	test((ret=Lex.Val(T))==KErrNone);
	test(T==-2147483647-1);  // the -1 prevents a (bug?) warning

	_LL(&String[0], (TText8*)"-2147483649");
	Lex=&String[0];
	test((ret=Lex.Val(T))==KErrOverflow);

	/////////////////
	// Test Val(TInt64)
	/////////////////
	test.Next(_L("Val(TInt64)"));
	TInt64 T64;
	_LL(&String[0], (TText8*)"");
	Lex=&String[0];
	test((ret=Lex.Val(T64))==KErrGeneral);

	_LL(&String[0], (TText8*)"2147483647");
	Lex=&String[0];
	test((ret=Lex.Val(T64))==KErrNone);
	test(T64==TInt(2147483647L));

	_LL(&String[0], (TText8*)"2147483648");
	Lex=&String[0];
	test((ret=Lex.Val(T64))==KErrNone);
	test(T64==MAKE_TINT64(0,0x80000000u));

	_LL(&String[0], (TText8*)"-2147483648");
	Lex=&String[0];
	test((ret=Lex.Val(T64))==KErrNone);
	test(T64==-2147483647-1);  // the -1 prevents a (bug?) warning

	_LL(&String[0], (TText8*)"-2147483649");
	Lex=&String[0];
	test((ret=Lex.Val(T64))==KErrNone);
	test(T64==MAKE_TINT64(0xffffffffu,0x7fffffffu));

	_LL(&String[0], (TText8*)"9223372036854775807");
	Lex=&String[0];
	ret=Lex.Val(T64);
	test.Printf(_L("ret=%d\n"),ret);
	test(ret==KErrNone);
	test.Printf(_L("%lx\n"),T64);
	test(T64==MAKE_TINT64(0x7fffffffu,0xffffffffu));

	_LL(&String[0], (TText8*)"9223372036854775808");
	Lex=&String[0];
	test((ret=Lex.Val(T64))==KErrOverflow);

	_LL(&String[0], (TText8*)"-9223372036854775808");
	Lex=&String[0];
	test((ret=Lex.Val(T64))==KErrNone);
	test(T64==MAKE_TINT64(0x80000000u,0x0));

	_LL(&String[0], (TText8*)"-9223372036854775809");
	Lex=&String[0];
	test((ret=Lex.Val(T64))==KErrOverflow);

	////////////////////					
	// Test Val(TReal32)
	// 32-bit floating point number.
	/////////////////////
	test.Next(_L("Val(TReal32)"));
	TReal32 TR32 = 0;
	
	_LL(&String[0], (TText8*)"92.2337203685477");
	Lex=&String[0];
	test((ret=Lex.Val(TR32))==KErrNone);
				
	_LL(&String[0], (TText8*)"92.2337203685477");
	Lex=&String[0];
	test((ret=Lex.Val(TR32,'.'))==KErrNone);
			
	_LL(&String[0], (TText8*)"0.2337285477");
	Lex=&String[0];
	test((ret=Lex.Val(TR32,'.'))==KErrNone);
		
	_LL(&String[0], (TText8*)"23");
	Lex=&String[0];
	test((ret=Lex.Val(TR32,'.'))==KErrNone);
		
	_LL(&String[0], (TText8*)"0");
	Lex=&String[0];
	test((ret=Lex.Val(TR32,'.'))==KErrNone);
		
	_LL(&String[0], (TText8*)"-324.27890");
	Lex=&String[0];
	test((ret=Lex.Val(TR32,'.'))==KErrNone);
	
	////////////////////
	// Test Val(TReal)
	// 64-bit floating point number. Identical to TReal64.
	///////////////////
	test.Next(_L("Val(TReal)"));
	TReal TR64 = 0;

	_LL(&String[0], (TText8*)"92.2337203685477");
	Lex=&String[0];
	test((ret=Lex.Val(TR64))==KErrNone);
		            
	_LL(&String[0], (TText8*)"92.2337203685477");
	Lex=&String[0];
	test((ret=Lex.Val(TR64,'.'))==KErrNone);
		       
	_LL(&String[0], (TText8*)"0.2337285477");
	Lex=&String[0];
	test((ret=Lex.Val(TR64,'.'))==KErrNone);
		    
	_LL(&String[0], (TText8*)"23");
	Lex=&String[0];
	test((ret=Lex.Val(TR64,'.'))==KErrNone);
		    
	_LL(&String[0], (TText8*)"0");
	Lex=&String[0];
	test((ret=Lex.Val(TR64,'.'))==KErrNone);
		    
	_LL(&String[0], (TText8*)"-324.27890");
	Lex=&String[0];
	test((ret=Lex.Val(TR64,'.'))==KErrNone);
	
	
	///////////////////////////
	// Test Val(TUint8, TRadix)
	///////////////////////////
	test.Next(_L("Val(TUint8, TRadix)"));
	TUint8 TU8;

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TU8, (TRadix)EBinary))==KErrNone);
	test(TU8==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TU8, (TRadix)EBinary))==KErrNone);
	test(TU8==1);

	_LL(&String[0], (TText8*)"11111111");
	Lex=&String[0];
	test((ret=Lex.Val(TU8, (TRadix)EBinary))==KErrNone);
	test(TU8==255);

	_LL(&String[0], (TText8*)"100000000");
	Lex=&String[0];
	test((ret=Lex.Val(TU8, (TRadix)EBinary))==KErrOverflow);

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TU8, (TRadix)EOctal))==KErrNone);
	test(TU8==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TU8, (TRadix)EOctal))==KErrNone);
	test(TU8==1);

	_LL(&String[0], (TText8*)"377");
	Lex=&String[0];
	test((ret=Lex.Val(TU8, (TRadix)EOctal))==KErrNone);
	test(TU8==255);

	_LL(&String[0], (TText8*)"400");
	Lex=&String[0];			
	test((ret=Lex.Val(TU8, (TRadix)EOctal))==KErrOverflow);

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TU8, (TRadix)EDecimal))==KErrNone);
	test(TU8==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TU8, (TRadix)EDecimal))==KErrNone);
	test(TU8==1);

	_LL(&String[0], (TText8*)"255");
	Lex=&String[0];
	test((ret=Lex.Val(TU8, (TRadix)EDecimal))==KErrNone);
	test(TU8==255);

	_LL(&String[0], (TText8*)"256");
	Lex=&String[0];			
	test((ret=Lex.Val(TU8, (TRadix)EDecimal))==KErrOverflow);

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TU8, (TRadix)EHex))==KErrNone);
	test(TU8==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TU8, (TRadix)EHex))==KErrNone);
	test(TU8==1);

	_LL(&String[0], (TText8*)"Ff");
	Lex=&String[0];
	test((ret=Lex.Val(TU8, (TRadix)EHex))==KErrNone);
	test(TU8==255);

	_LL(&String[0], (TText8*)"100");
	Lex=&String[0];			
	test((ret=Lex.Val(TU8, (TRadix)EHex))==KErrOverflow);



	////////////////////////////
	// Test Val(TUint16, TRadix)
	////////////////////////////
	test.Next(_L("Val(TUint16, TRadix)"));
	TUint16 TU16;

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TU16, (TRadix)EBinary))==KErrNone);
	test(TU16==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TU16, (TRadix)EBinary))==KErrNone);
	test(TU16==1);

	_LL(&String[0], (TText8*)"1111111111111111");
	Lex=&String[0];
	test((ret=Lex.Val(TU16, (TRadix)EBinary))==KErrNone);
	test(TU16==65535);

	_LL(&String[0], (TText8*)"10000000000000000");
	Lex=&String[0];
	test((ret=Lex.Val(TU16, (TRadix)EBinary))==KErrOverflow);

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TU16, (TRadix)EOctal))==KErrNone);
	test(TU16==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TU16, (TRadix)EOctal))==KErrNone);
	test(TU16==1);

	_LL(&String[0], (TText8*)"177777");
	Lex=&String[0];
	test((ret=Lex.Val(TU16, (TRadix)EOctal))==KErrNone);
	test(TU16==65535);

	_LL(&String[0], (TText8*)"200000");
	Lex=&String[0];
	test((ret=Lex.Val(TU16, (TRadix)EOctal))==KErrOverflow);

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TU16, (TRadix)EDecimal))==KErrNone);
	test(TU16==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TU16, (TRadix)EDecimal))==KErrNone);
	test(TU16==1);

	_LL(&String[0], (TText8*)"65535");
	Lex=&String[0];
	test((ret=Lex.Val(TU16, (TRadix)EDecimal))==KErrNone);
	test(TU16==65535);

	_LL(&String[0], (TText8*)"65536");
	Lex=&String[0];			
	test((ret=Lex.Val(TU16, (TRadix)EDecimal))==KErrOverflow);

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TU16, (TRadix)EHex))==KErrNone);
	test(TU16==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TU16, (TRadix)EHex))==KErrNone);
	test(TU16==1);

	_LL(&String[0], (TText8*)"ffFf");
	Lex=&String[0];
	test((ret=Lex.Val(TU16, (TRadix)EHex))==KErrNone);
	test(TU16==65535);

	_LL(&String[0], (TText8*)"10000");
	Lex=&String[0];			
	test((ret=Lex.Val(TU16, (TRadix)EHex))==KErrOverflow);



	////////////////////////////
	// Test Val(TUint32, TRadix)
	////////////////////////////
	test.Next(_L("Val(TUint32, TRadix)"));
	TUint32 TU32;

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TU32, (TRadix)EBinary))==KErrNone);
	test(TU32==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TU32, (TRadix)EBinary))==KErrNone);
	test(TU32==1);

	_LL(&String[0], (TText8*)"11111111111111111111111111111111");
	Lex=&String[0];
	test((ret=Lex.Val(TU32, (TRadix)EBinary))==KErrNone);
	test(TU32==4294967295u);

	_LL(&String[0], (TText8*)"100000000000000000000000000000000");
	Lex=&String[0];
	test((ret=Lex.Val(TU32, (TRadix)EBinary))==KErrOverflow);

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TU32, (TRadix)EOctal))==KErrNone);
	test(TU32==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TU32, (TRadix)EOctal))==KErrNone);
	test(TU32==1);

	_LL(&String[0], (TText8*)"37777777777");
	Lex=&String[0];
	test((ret=Lex.Val(TU32, (TRadix)EOctal))==KErrNone);
	test(TU32==4294967295u);

	_LL(&String[0], (TText8*)"40000000000");
	Lex=&String[0];			
	test((ret=Lex.Val(TU32, (TRadix)EOctal))==KErrOverflow);

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TU32, (TRadix)EDecimal))==KErrNone);
	test(TU32==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TU32, (TRadix)EDecimal))==KErrNone);
	test(TU32==1);

	_LL(&String[0], (TText8*)"4294967295");
	Lex=&String[0];
	test((ret=Lex.Val(TU32, (TRadix)EDecimal))==KErrNone);
	test(TU32==4294967295u);

	_LL(&String[0], (TText8*)"4294967296");
	Lex=&String[0];			
	test((ret=Lex.Val(TU32, (TRadix)EDecimal))==KErrOverflow);

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TU32, (TRadix)EHex))==KErrNone);
	test(TU32==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TU32, (TRadix)EHex))==KErrNone);
	test(TU32==1);

	_LL(&String[0], (TText8*)"FFFFFFFF");
	Lex=&String[0];
	test((ret=Lex.Val(TU32, (TRadix)EHex))==KErrNone);
	test(TU32==4294967295u);

	_LL(&String[0], (TText8*)"100000000");
	Lex=&String[0];			
	test((ret=Lex.Val(TU32, (TRadix)EHex))==KErrOverflow);

 	///////////////////////////////////
	// Test Val(TInt64, TRadix)
	///////////////////////////////////
	test.Next(_L("Val(TInt64,TRadix)"));
	TInt64 TI64;

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TI64, (TRadix)EBinary))==KErrNone);
	test(TI64==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TI64, (TRadix)EBinary))==KErrNone);
	test(TI64==1);

	_LL(&String[0], (TText8*)"11111111111111111111111111111111");
	Lex=&String[0];
	test((ret=Lex.Val(TI64, (TRadix)EBinary))==KErrNone);
	test(TI64==TInt64(0xffffffffu));

	_LL(&String[0], (TText8*)"100000000000000000000000000000000");
	Lex=&String[0];
	test((ret=Lex.Val(TI64, (TRadix)EBinary))==KErrNone);
	test(TI64==MAKE_TINT64(0x1,0x0));

 	_LL(&String[0], (TText8*)"1111111111111111111111111111111111111111111111111111111111111111");
	Lex=&String[0];
	test((ret=Lex.Val(TI64, (TRadix)EBinary))==KErrNone);
	test(TI64==MAKE_TINT64(0xffffffffu,0xffffffffu));

	_LL(&String[0], (TText8*)"10000000000000000000000000000000000000000000000000000000000000000");
	Lex=&String[0];
	test((ret=Lex.Val(TI64, (TRadix)EBinary))==KErrOverflow);

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TI64, (TRadix)EOctal))==KErrNone);
	test(TI64==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TI64, (TRadix)EOctal))==KErrNone);
	test(TI64==1);

	_LL(&String[0], (TText8*)"37777777777");
	Lex=&String[0];
	test((ret=Lex.Val(TI64, (TRadix)EOctal))==KErrNone);
	test(TI64==TInt64(0xffffffffu));

	_LL(&String[0], (TText8*)"40000000000");
	Lex=&String[0];			
	test((ret=Lex.Val(TI64, (TRadix)EOctal))==KErrNone);
	test(TI64==MAKE_TINT64(0x1,0x0));

	_LL(&String[0], (TText8*)"1777777777777777777777");
	Lex=&String[0];
	test((ret=Lex.Val(TI64, (TRadix)EOctal))==KErrNone);
	test(TI64==MAKE_TINT64(0xffffffffu,0xffffffffu));

	_LL(&String[0], (TText8*)"2000000000000000000000");
	Lex=&String[0];			
	test((ret=Lex.Val(TI64, (TRadix)EOctal))==KErrOverflow);

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];				
	test((ret=Lex.Val(TI64, (TRadix)EDecimal))==KErrNone);
	test(TI64==0);

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0]; //************** iNext is set to "" by Val
	test((ret=Lex.Val(TI64))==KErrNone);	
	test(TI64==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TI64, (TRadix)EDecimal))==KErrNone);
	test(TI64==1);
	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TI64))==KErrNone);
	test(TI64==1);

	_LL(&String[0], (TText8*)"4294967295");
	Lex=&String[0];
	test((ret=Lex.Val(TI64, (TRadix)EDecimal))==KErrNone);
	test(TI64==TInt64(0xffffffffu));
	_LL(&String[0], (TText8*)"4294967295");
	Lex=&String[0];
	test((ret=Lex.Val(TI64))==KErrNone);
	test(TI64==TInt64(0xffffffffu));

	_LL(&String[0], (TText8*)"4294967296");
	Lex=&String[0];			
	test((ret=Lex.Val(TI64, (TRadix)EDecimal))==KErrNone);
	test(TI64==MAKE_TINT64(1,0)); 
	_LL(&String[0], (TText8*)"4294967296");
	Lex=&String[0];
	test((ret=Lex.Val(TI64))==KErrNone);   
	test(TI64==MAKE_TINT64(1,0)); 

	_LL(&String[0], (TText8*)"18446744073709551615");
	Lex=&String[0];
	test((ret=Lex.Val(TI64, (TRadix)EDecimal))==KErrNone);
	test(TI64==MAKE_TINT64(0xffffffffu,0xffffffffu));
	_LL(&String[0], (TText8*)"18446744073709551616");
	Lex=&String[0];
	test((ret=Lex.Val(TI64))==KErrOverflow);

	_LL(&String[0], (TText8*)"-1");
	Lex=&String[0];			
	test((ret=Lex.Val(TI64, (TRadix)EDecimal))==KErrGeneral);

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TI64, (TRadix)EHex))==KErrNone);
	test(TI64==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TI64, (TRadix)EHex))==KErrNone);
	test(TI64==1);

	_LL(&String[0], (TText8*)"FFFFFFFF");
	Lex=&String[0];
	test((ret=Lex.Val(TI64, (TRadix)EHex))==KErrNone);
	test(TI64==TInt64(0xffffffffu));

	_LL(&String[0], (TText8*)"100000000");
	Lex=&String[0];			
	test((ret=Lex.Val(TI64, (TRadix)EHex))==KErrNone);
	test(TI64==MAKE_TINT64(1,0)); 

	_LL(&String[0], (TText8*)"FFFFFFFFffffffff");
	Lex=&String[0];
	test((ret=Lex.Val(TI64, (TRadix)EHex))==KErrNone);
	test(TI64==MAKE_TINT64(0xffffffffu,0xffffffffu));

	_LL(&String[0], (TText8*)"10000000000000000");
	Lex=&String[0];			
	test((ret=Lex.Val(TI64, (TRadix)EHex))==KErrOverflow);


	///////////////////////////////////
	// Test Val(TUint, TRadix=(TRadix)EDecimal)
	///////////////////////////////////
	test.Next(_L("Val(TUint, TRadix=(TRadix)EDecimal)"));
	TUint TU;

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TU, (TRadix)EBinary))==KErrNone);
	test(TU==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TU, (TRadix)EBinary))==KErrNone);
	test(TU==1);

	_LL(&String[0], (TText8*)"11111111111111111111111111111111");
	Lex=&String[0];
	test((ret=Lex.Val(TU, (TRadix)EBinary))==KErrNone);
	test(TU==4294967295u);

	_LL(&String[0], (TText8*)"100000000000000000000000000000000");
	Lex=&String[0];
	test((ret=Lex.Val(TU, (TRadix)EBinary))==KErrOverflow);

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TU, (TRadix)EOctal))==KErrNone);
	test(TU==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TU, (TRadix)EOctal))==KErrNone);
	test(TU==1);

	_LL(&String[0], (TText8*)"37777777777");
	Lex=&String[0];
	test((ret=Lex.Val(TU, (TRadix)EOctal))==KErrNone);
	test(TU==4294967295u);

	_LL(&String[0], (TText8*)"40000000000");
	Lex=&String[0];			
	test((ret=Lex.Val(TU, (TRadix)EOctal))==KErrOverflow);

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];				
	test((ret=Lex.Val(TU, (TRadix)EDecimal))==KErrNone);
	test(TU==0);

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0]; //************** iNext is set to "" by Val
	test((ret=Lex.Val(TU))==KErrNone);	
	test(TU==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TU, (TRadix)EDecimal))==KErrNone);
	test(TU==1);
	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TU))==KErrNone);
	test(TU==1);

	_LL(&String[0], (TText8*)"4294967295");
	Lex=&String[0];
	test((ret=Lex.Val(TU, (TRadix)EDecimal))==KErrNone);
	test(TU==4294967295u);
	_LL(&String[0], (TText8*)"4294967295");
	Lex=&String[0];
	test((ret=Lex.Val(TU))==KErrNone);
	test(TU==4294967295u);

	_LL(&String[0], (TText8*)"4294967296");
	Lex=&String[0];			
	test((ret=Lex.Val(TU, (TRadix)EDecimal))==KErrOverflow); 
	_LL(&String[0], (TText8*)"4294967296");
	Lex=&String[0];
	test((ret=Lex.Val(TU))==KErrOverflow);   

	_LL(&String[0], (TText8*)"00");
	Lex=&String[0];
	test((ret=Lex.Val(TU, (TRadix)EHex))==KErrNone);
	test(TU==0);

	_LL(&String[0], (TText8*)"01");
	Lex=&String[0];
	test((ret=Lex.Val(TU, (TRadix)EHex))==KErrNone);
	test(TU==1);

	_LL(&String[0], (TText8*)"FFFFFFFF");
	Lex=&String[0];
	test((ret=Lex.Val(TU, (TRadix)EHex))==KErrNone);
	test(TU==4294967295u);

	_LL(&String[0], (TText8*)"100000000");
	Lex=&String[0];			
	test((ret=Lex.Val(TU, (TRadix)EHex))==KErrOverflow);


	/////////////////////////////////
	// Test Val(TInt32, TUint aLimit)
	/////////////////////////////////		  
	// This is called by several of the other Val methods and so has been indirectly tested already
	test.Next(_L("Val(TInt32, TUint aLimit"));

	_LL(&String[0], (TText8*)"1000");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(T32,1000))==KErrNone);
	test(T32==1000);

	_LL(&String[0], (TText8*)"1001");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(T32, 1000))==KErrOverflow);

	_LL(&String[0], (TText8*)"-1000");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(T32, 1000))==KErrNone);
	test(T32==-1000);

	_LL(&String[0], (TText8*)"-1001");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(T32, 1000))==KErrNone);
	test(T32==-1001);

	_LL(&String[0], (TText8*)"-1002");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(T32, 1000))==KErrOverflow);

	_LL(&String[0], (TText8*)"0");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(T32, 0))==KErrNone);
	test(T32==0);
	
	_LL(&String[0], (TText8*)"1");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(T32, 0))==KErrOverflow);	   


	/////////////////////////////////////////////////
	// Test Val(TUInt32, TRadix aRadix, TUint aLimit)
	////////////////////////////////////////////////
	// This is called by several of the other Val methods and so has been indirectly tested already
	test.Next(_L("Val(TUInt32, TRadix, TUint)"));

	// Test bug found during previous testing 
	_LL(&String[0], (TText8*)"10");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TU32, (TRadix)EDecimal, 10))==KErrNone);
	test(TU32==10);

	_LL(&String[0], (TText8*)"11");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TU32, (TRadix)EDecimal, 10))==KErrOverflow);

	_LL(&String[0], (TText8*)"19");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TU32, (TRadix)EDecimal, 10))==KErrOverflow);

	_LL(&String[0], (TText8*)"20");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TU32, (TRadix)EDecimal, 10))==KErrOverflow);

	/////////////////////////////////////////////////
	// Test Val(TInt64, TRadix aRadix, TInt64 aLimit)
	////////////////////////////////////////////////
	test.Next(_L("Val(TInt64, TRadix, TInt64)"));

	_LL(&String[0], (TText8*)"10");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, (TRadix)EDecimal, 10))==KErrNone);
	test(TI64==TInt64(10));

	_LL(&String[0], (TText8*)"11");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, (TRadix)EDecimal, 10))==KErrOverflow);

	_LL(&String[0], (TText8*)"19");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, (TRadix)EDecimal, 10))==KErrOverflow);

	_LL(&String[0], (TText8*)"20");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, (TRadix)EDecimal, 10))==KErrOverflow);

	_LL(&String[0], (TText8*)"34532739886900");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, (TRadix)EDecimal, MAKE_TINT64(0x1f68u,0x47b1af34u)))==KErrNone);
	test(TI64==MAKE_TINT64(0x1f68u,0x47b1af34u));

	_LL(&String[0], (TText8*)"34532739886901");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, (TRadix)EDecimal, MAKE_TINT64(0x1f68u,0x47b1af34u)))==KErrOverflow);

	_LL(&String[0], (TText8*)"74532739886901");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, (TRadix)EDecimal, MAKE_TINT64(0x1f68u,0x47b1af34u)))==KErrOverflow);

	_LL(&String[0], (TText8*)"6901");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, (TRadix)EDecimal, MAKE_TINT64(0x1f68u,0x47b1af34u)))==KErrNone);
	test(TI64==TInt64(6901));

	_LL(&String[0], (TText8*)"1f6847b1af34");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, (TRadix)EHex, MAKE_TINT64(0x1f68u,0x47b1af34u)))==KErrNone);
	test(TI64==MAKE_TINT64(0x1f68u,0x47b1af34u));

	_LL(&String[0], (TText8*)"1f6847b1af35");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, (TRadix)EHex, MAKE_TINT64(0x1f68u,0x47b1af34u)))==KErrOverflow);

	_LL(&String[0], (TText8*)"1f6847b1af340");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, (TRadix)EHex, MAKE_TINT64(0x1f68u,0x47b1af34u)))==KErrOverflow);

	_LL(&String[0], (TText8*)"1e82791aed35");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, (TRadix)EHex, TInt64(0x56fba45u)))==KErrOverflow);

	/////////////////////////////////////////////////
	// Test Val(TInt64, TInt64 aLimit)
	////////////////////////////////////////////////
	test.Next(_L("Val(TInt64, TInt64)"));

	_LL(&String[0], (TText8*)"10");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, 10))==KErrNone);
	test(TI64==TInt64(10));

	_LL(&String[0], (TText8*)"11");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, 10))==KErrOverflow);

	_LL(&String[0], (TText8*)"19");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, 10))==KErrOverflow);

	_LL(&String[0], (TText8*)"20");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, 10))==KErrOverflow);

	_LL(&String[0], (TText8*)"34532739886900");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, MAKE_TINT64(0x1f68u,0x47b1af34u)))==KErrNone);
	test(TI64==MAKE_TINT64(0x1f68u,0x47b1af34u));

	_LL(&String[0], (TText8*)"34532739886901");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, MAKE_TINT64(0x1f68u,0x47b1af34u)))==KErrOverflow);

	_LL(&String[0], (TText8*)"74532739886901");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, MAKE_TINT64(0x1f68u,0x47b1af34u)))==KErrOverflow);

	_LL(&String[0], (TText8*)"6901");
	Lex=&String[0];
	test((ret=Lex.BoundedVal(TI64, MAKE_TINT64(0x1f68u,0x47b1af34u)))==KErrNone);
	test(TI64==TInt64(6901));

	test.End();
	}

void testLocale()
	{

	TLocale current;
	current.Refresh();
	TLocale loc;
	loc.Refresh();

	TReal64 v,v8;
	TLex l;
	TLex8 l8;
	TInt r;

	loc.SetDecimalSeparator('.');
	loc.Set();
	l=_L("-12.34");
	l8=_L8("-12.34");
	r=l.Val(v);
	r=l8.Val(v8);
	test(r==KErrNone);
	test(v==-12.34);
	test(v==v8);

	l=_L("-12.34");
	l8=_L8("-12.34");
	r=l.Val(v,'.');
	r=l8.Val(v8,'.');
	test(r==KErrNone);
	test(v==-12.34);
	test(v==v8);
	l=_L("-12.34");
	l8=_L8("-12.34");
	r=l.Val(v,',');
	r=l8.Val(v8,',');
	test(r==KErrNone);
	test(v==-12);
	test(v==v8);

	l=_L("-12,34");
	l8=_L8("-12,34");
	r=l.Val(v);
	r=l8.Val(v8);
	test(r==KErrNone);
	test(v==-12);
	test(v==v8);
	l=_L("-12,34");
	l8=_L8("-12,34");
	r=l.Val(v,'.');
	r=l8.Val(v8,'.');
	test(r==KErrNone);
	test(v==-12);
	test(v==v8);
	l=_L("-12,34");
	l8=_L8("-12,34");
	r=l.Val(v,',');
	r=l8.Val(v8,',');
	test(r==KErrNone);
	test(v==-12.34);
	test(v==v8);

	loc.SetDecimalSeparator(',');
	loc.Set();
	l=_L("-12.34");
	l8=_L8("-12.34");
	r=l.Val(v);
	r=l8.Val(v8);
	test(r==KErrNone);
	test(v==-12);
	test(v==v8);

	l=_L("-12.34");
	l8=_L8("-12.34");
	r=l.Val(v,'.');
	r=l8.Val(v8,'.');
	test(r==KErrNone);
	test(v==-12.34);
	test(v==v8);
	l=_L("-12.34");
	l8=_L8("-12.34");
	r=l.Val(v,',');
	r=l8.Val(v8,',');
	test(r==KErrNone);
	test(v==-12);
	test(v==v8);

	l=_L("-12,34");
	l8=_L8("-12,34");
	r=l.Val(v);
	r=l8.Val(v8);
	test(r==KErrNone);
	test(v==-12.34);
	l=_L("-12,34");
	l8=_L8("-12,34");
	r=l.Val(v,'.');
	r=l8.Val(v8,'.');
	test(r==KErrNone);
	test(v==-12);
	test(v==v8);
	l=_L("-12,34");
	l8=_L8("-12,34");
	r=l.Val(v,',');
	r=l8.Val(v8,',');
	test(r==KErrNone);
	test(v==-12.34);
	test(v==v8);

	loc.Set();
	current.Set();
	}

#pragma warning( disable : 4705 )	// statement has no effect
GLDEF_C TInt E32Main()
    {

	test.Title(); 

	test.Start(_L("********* TLEX **********"));
#if defined(_UNICODE)
	TestTLex<TPtrC, TLex, TLexMark, TBuf<0x40>, TLex16Dump, TLexMark16Dump, TText> T;
#else
	TestTLex<TPtrC, TLex, TLexMark, TBuf<0x40>, TLex8Dump, TLexMark8Dump, TText> T;
#endif
	test.Next(_L("TText 1"));
	T.Test1();
	test.Next(_L("TText 2"));
	T.Test2();
	test.Next(_L("TText 3"));
	T.Test3();
	test.Next(_L("TText 4"));
	T.Test4();
	test.Next(_L("TText 5"));
	T.Test5();

	TestTLex<TPtrC8, TLex8, TLexMark8, TBuf8<0x40>, TLex8Dump, TLexMark8Dump, TText8> T8;
	test.Next(_L("TText8 1"));
	T8.Test1();
	test.Next(_L("TText8 2"));
	T8.Test2();
	test.Next(_L("TText8 3"));
	T8.Test3();
	test.Next(_L("TText8 4"));
	T8.Test4();
	test.Next(_L("TText8 5"));
	T8.Test5();
  
	TestTLex<TPtrC16, TLex16, TLexMark16, TBuf16<0x40>, TLex16Dump, TLexMark16Dump, TText16> T16;
	test.Next(_L("TText16 1"));
	T16.Test1();
	test.Next(_L("TText16 2"));
	T16.Test2();
	test.Next(_L("TText16 3"));
	T16.Test3();
	test.Next(_L("TText16 4"));
	T16.Test4();
	test.Next(_L("TText16 5"));
	T16.Test5();

	test.Next(_L("Test TLex in different locales"));
	testLocale();

	test.End();
	return(KErrNone);
    }
#pragma warning( default : 4705 )

