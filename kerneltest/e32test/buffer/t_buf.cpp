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
// e32test\buffer\t_buf.cpp
// Overview:
// Test methods of the TBuf16, TBuf8, TBuf template class.
// API Information:
// TBuf16, TBuf8, TBuf.
// Details :
// - Create some 16 bit modifiable descriptors, 8 bit modifiable descriptors
// of fixed length, Build-independent modifiable descriptors, initialize 
// with different strings and check for
// - Comparison operators,
// - Property access methods,
// - Fill & swap methods,
// - Conversion methods,
// - Comparison methods,
// - Pattern Matching methods,
// - Pattern Locating methods,
// - Copying methods, 
// - Find, FindC, FindF methods,
// - Repeat, Trim, TrimLeft, TrimRight, Insert, Delete, Left,
// Right, Mid methods,
// - Formatting methods,
// - Replace methods are as expected.
// - Construct some descriptors with buffer length, string and buffer reference and
// verify that they are created successfully.
// - Test assignment operators and comparison operators for different descriptors.
// - Initialize some descriptors and check descriptors' maximum length, length and 
// size are as expected. 
// - Check Fill and Swap methods are as expected.
// - Test Fold, Collate, LowerCase, UpperCase methods are as expected.
// - Test Comparison methods are as expected.
// - Test pattern matching for simple string, wild cards with collated comparison. Verify that the 
// return value is KErrNotFound when pattern doesn't match.
// - Check Locating methods by searching character in forward and backward direction and 
// verify the return value is KErrNotFound when unavailable character is searched.
// - Check copying strings and converting those into lower and upper case strings are
// as expected.
// - Check Find methods by searching string and verify the return value is KErrNotFound when 
// unavailable string is searched.
// - Check Repeat, Trim, Insert and Delete methods are as expected.
// - Check the formatting operations are as expected.
// - Check integer to decimal character representation is as expected.
// - Check integer to character representation with different number system is as expected.
// - Check string formatting with variable parameter list is as expected
// - Check Replace method by replacing string at different places in a string is as expected.
// - Check the conversion of real numbers, extended precision real numbers into string
// format is as expected.
// - Check Format and FormatList methods are as expected.
// - Check Format of TReal is as expected.
// - Check the non-leaving and leaving descriptors overflow handlers are as expected. 
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32math.h>
#include <hal.h>
#include <hal_data.h>
#include <hal_data.h>
#include <e32svr.h>
#include <collate.h> 

#ifdef __VC32__
    // Solve compilation problem caused by non-English locale
    #pragma setlocale("english")
#endif

LOCAL_D RTest test(_L("T_BUF"));

#pragma warning(disable: 4127) // disabling warning "conditional expression is constant"
#pragma warning(disable: 4310) // disabling warning "cast truncates constant value"

#undef _TL
#define _TL(a) DESTEMPLATE((S*)RTest::String(sizeof(S),(TText8*)a,(TText16*)L ## a)) 
#undef _TS
#define _TS(a) ((const S*)RTest::String(sizeof(S),(TText8*)a,(TText16*)L ## a)) 

template<class T,class S,class DESTEMPLATE>	
class TestTBuf
	{
public:
	TestTBuf(TInt aLength); // Test class constructor.
	void Test1();   // Tests all functions of the class.
	void Test2();   // Tests all constructors.
	void Test3();	// Tests all assignment operators
	void Test4();	// Tests all comparison operators
	void Test5();	// Tests all property access
	void Test6();	// Tests all fill and swap
	void Test7();	// Tests all conversion 
	void Test8();	// Tests all comparison
	void Test9();	// Tests all matching
	void Test10();	// Tests all locating
	void Test11();	// Tests all Copying
	void Test12();	// Tests all finding
	void Test13();	// Tests all basic like ops
	void Test14();  // Tests all formating
	void Test15();  // Tests all replacing
	void test_TBuf(); // Test all classes
protected:
	void Test1List(T* a,T* b,...);
	void Test14_ReorderedParameterFormatting(TInt aDummyParameter, ...);
private:
	TInt iMaxBufLength;
	};

void TestEq(const TDesC8& a, const TDesC8& b, TInt aLine)
	{
	if (a!=b)
		{
		TBuf<256> buf;
		test.Printf(_L("LINE %d:\n"),aLine);
		buf.Copy(a);
		test.Printf(_L("a=%S\n"),&buf);
		buf.Copy(b);
		test.Printf(_L("b=%S\n"),&buf);
		test(0);
		}
	}

void TestEq(const TDesC16& a, const TDesC16& b, TInt aLine)
	{
	if (a!=b)
		{
		test.Printf(_L("LINE %d:\n"),aLine);
		test.Printf(_L("a=%S\n"),&a);
		test.Printf(_L("b=%S\n"),&b);
		test(0);
		}
	}

#define TESTEQ(a,b)	TestEq((a),(b),__LINE__)

template <class T,class S,class DESTEMPLATE>
GLDEF_C TestTBuf<T,S,DESTEMPLATE>::TestTBuf(TInt aLength)
// Constructor.
	: iMaxBufLength(aLength)
	{}

template <class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::Test1List(T* a,T* b,...)
// Test the format with list functions.
	{
	VA_LIST list;
	VA_START(list,b);
	a->Format(*b,list);
	VA_START(list,b);
	a->AppendFormat(*b,list);
	VA_START(list,b);
	a->Format(_TL("%d"),list);
	VA_START(list,b);
	a->AppendFormat(_TL("%d"),list);
	}

template <class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::Test1()
// Tests all functions of the class.
	{
	test.Start(_L("Constructors"));
	T a;
	T b;
	T c(_TL("AB"));
	T d(c);

// To avoid unused warning
    a.Length();
    b.Length();
    c.Length();
    d.Length();

	test.Next(_L("Copy operators"));
	a=_TL("DE");
	b=c;
	a+=_TL("DE");
	b+=c;

	test.Next(_L("Comparison operators"));
	TInt t=(a<b);
	t=(a<_TL("AB"));
	t=(_TL("AB")<a);
	t=(a<=b);
	t=(a<=_TL("AB"));
	t=(_TL("AB")<=a);
	t=(a>b);
	t=(a>_TL("AB"));
	t=(_TL("AB")>a);
	t=(a>=b);
	t=(a>=_TL("AB"));
	t=(_TL("AB")>=a);
	t=(a==b);
	t=(a==_TL("AB"));
	t=(_TL("AB")==a);
	t=(a!=_TL("AB"));
	t=(_TL("AB")!=a);

	test.Next(_L("Property access"));
	a[0]='A';
	a.Ptr();
	TInt i=a.MaxLength()-a.Length();
	i=a.Size();
	a.Zero();
	a.SetLength(2); 

	test.Next(_L("Fill & swap"));
	a.Fill(' ');
	a.Fill(' ',iMaxBufLength);
	a.FillZ();
	a.FillZ(iMaxBufLength);
	a.Swap(b);

	test.Next(_L("Conversion"));
	a.Fold();
	a.Collate();
	a.LowerCase();
	a.UpperCase();
	a.Capitalize();

	test.Next(_L("Comparison"));
	a.Compare(b);
	a.Compare(_TL("AB"));
	a.CompareF(b);
	a.CompareF(_TL("AB"));
	a.CompareC(b);
	a.CompareC(_TL("AB"));

	test.Next(_L("Matching"));
	a.Match(b);
	a.Match(_TL("AB"));
	a.MatchF(b);
	a.MatchF(_TL("AB"));

	T buf(_TL("abcdef"));
	TInt res = buf.MatchF(_TL("abc*def"));
	test(res == 0);

	a.MatchC(b);
	a.MatchC(_TL("AB"));

	test.Next(_L("Locating"));
	a.Locate('A');
	a.LocateF('A');
	a.LocateReverse('A');
	a.LocateReverseF('A');

	test.Next(_L("Copying"));
	a.Copy(b);
	a.Copy(_TL("AB"));
//	a.Copy(_TL("AB"),1);
	a.CopyF(b);
	a.CopyF(_TL("AB"));
	a.CopyC(b);
	a.CopyC(_TL("AB"));
	a.CopyLC(b);
	a.CopyLC(_TL("AB"));
	a.CopyUC(b);
	a.CopyUC(_TL("AB"));
	a.CopyCP(b);
	a.CopyCP(_TL("AB"));

	test.Next(_L("Finding"));
	a.Find(b);
	a.Find(_TL("AB"));
	a.FindF(b);
	a.FindF(_TL("AB"));
	a.FindC(b);
	a.FindC(_TL("AB"));

	test.Next(_L("Basic like ops"));
	a.Repeat(b);
	a.Repeat(_TL("AB"));
	a.TrimLeft();
	a.TrimRight();
	a.Trim();
	b=_TL("AA");
	a.Insert(0,b);
	a.Delete(0,2);
	b = a.Left(1);
	b = a.Right(1);
	b = a.Mid(0,1);

	test.Next(_L("Formating"));
	a.Justify(_TL("AB"),10,ELeft,' ');
	a.Justify(b,10,ELeft,' ');
	b.Fill('A',2);
	a.Zero();
	a.AppendJustify(_TL("AB"),10,ELeft,' ');
	a.AppendJustify(b,10,ELeft,' ');
	a.AppendJustify(b,10,KDefaultJustifyWidth,ELeft,' ');
	a.AppendJustify(b.Ptr(),10,ELeft,' ');
	
	TInt v1=10;
	a.Num(v1);
	a.AppendNum(v1);
	TInt v2=10;
	a.Num((TUint)v2,EHex);
	a.AppendNum((TUint)v2,EHex);
	a.NumUC((TUint)v2,EHex);
	a.AppendNumUC((TUint)v2,EHex);
	
	//Converts the specified unsigned integer into a fixed width character representation
	//based on the specified number system and copies the conversion into this descriptor,
	//replacing any existing data. The length of this descriptor is set to reflect the new data.
	a.NumFixedWidth(v1,EBinary,4); 
	a.NumFixedWidth(v1,EOctal,3);
	a.NumFixedWidth(v1,EDecimal,2);
	a.NumFixedWidth(v1,EHex,1);
	//When a hexadecimal conversion is specified, hexadecimal characters are in upper case.
	a.NumFixedWidthUC(v1,EBinary,4);
	a.NumFixedWidthUC(v1,EOctal,3);
	a.NumFixedWidthUC(v1,EDecimal,2);
	a.NumFixedWidthUC(v1,EHex,1);
	//Appends the conversion onto the end of this descriptor's data.
	a.AppendNumFixedWidthUC(v1,EBinary,4);
	a.AppendNumFixedWidthUC(v1,EOctal,3);
	a.AppendNumFixedWidthUC(v1,EDecimal,2);
	a.AppendNumFixedWidthUC(v1,EHex,1); 
	
	TReal v3=10.0;
	TRealFormat ff;
	ff.iType=KRealFormatFixed;
	ff.iWidth=10;
	ff.iPlaces=2;
	ff.iPoint='.';
	ff.iTriad=',';
	ff.iTriLen=3;
	a.Num(v3,ff);
	a.AppendNum(v3,ff);
	a.Format(_TL("%d"),12);
	a.AppendFormat(_TL("%d"),12);
	b=_TL("%d");
	a.Format(b,12);
	a.AppendFormat(b,12);
	Test1List(&a,&b,12);

	test.Next(_L("Replacing"));
	a=_TL("AAC");
	b=_TL("B");
	a.Replace(1,1,b);
	test.End();
	}

template <class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::Test2()
// Tests all constructors.
	{
	test.Start(_L("Default"));
	T a;
	test(a.MaxLength()==iMaxBufLength);
	test(a.Length()==0);

	test.Next(_L("By length"));
	T b(iMaxBufLength>>1);
	test(b.MaxLength()==iMaxBufLength);
	test(b.Length()==(iMaxBufLength>>1));

	test.Next(_L("By string"));
	T c(_TL("AB"));
	test(c.MaxLength()==iMaxBufLength);
	test(c.Length()==2);
	test(c[0]=='A');
	test(c[1]=='B');

	test.Next(_L("By buffer reference"));
	T d(c);
	test(d.MaxLength()==iMaxBufLength);
	test(d.Length()==2);
	test(d[0]=='A');
	test(d[1]=='B');
	test.End();
	}

template <class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::Test3()
// Tests all assignment operators
	{
	test.Start(_L("By String"));
	T a;
	a=_TL("AB");
	a+=_TL("CD");
	test(a.Length()==4);
	test(a==_TL("ABCD"));

	test.Next(_L("By buffer"));
	T b;
	b=a;
	b+=a;
	test(b.Length()==8);
	test(b==_TL("ABCDABCD"));
//	
	test.End();
	}

template <class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::Test4()
// Test all comparison operators
	{
	test.Start(_L("By NULL string"));
	T a;
	test(a==_TL(""));		// NULL strings
	test(!(a!=_TL("")));
	test(a<=_TL(""));
	test(a>=_TL(""));
	test(!(a<_TL("")));
	test(!(a>_TL("")));
	test(_TL("")==a);
	test(!(_TL("")!=a));
	test(_TL("")<=a);
	test(_TL("")>=a);
	test(!(_TL("")<a));
	test(!(_TL("")>a));

	test.Next(_L("By string or buffer"));
	a=_TL("abc");
	test(a==_TL("abc"));		// ==
	test(!(a==_TL("xyz")));
	test(!(a==_TL("aa")));
	test(_TL("abc")==a);
	test(!(_TL("xyz")==a));
	test(!(_TL("aa")==a));
	test(a!=_TL("xyz"));		// !=
	test(!(a!=_TL("abc")));
	test(a!=_TL("aa"));
	test(_TL("xyz")!=a);
	test(!(_TL("abc")!=a));
	test(_TL("aa")!=a);
	test(a<_TL("x"));			// <
	test(!(a<_TL("abc")));
	test(!(a<_TL("aa")));
	test(_TL("aa")<a);
	test(!(_TL("abc")<a));
	test(!(_TL("xyz")<a));
	test(a>_TL("aa"));			// >
	test(!(a>_TL("abc")));
	test(!(a>_TL("xyz")));
	test(_TL("xyz")>a);
	test(!(_TL("abc")>a));
	test(!(_TL("aa")>a));
	test(a>=_TL("abc"));		// >=
	test(!(a>=_TL("xyz")));
	test(a>=_TL("aa"));
	test(_TL("abc")>=a);
	test(!(_TL("aaa")>=a));
	test(_TL("xyz")>=a);
	test(a<=_TL("abc"));		// <=
	test(!(a<=_TL("aa")));
	test(a<=_TL("xyz"));
	test(_TL("abc")<=a);
	test(!(_TL("xyz")<=a));
	test(_TL("aa")<=a);

	test.Next(_L("By special characters"));
	a=_TL("!@#$%^&*()");
	test(a==_TL("!@#$%^&*()"));
	test.End();	
	}

template <class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::Test5()
// Test all property access
	{
	test.Start(_L("Length and Size"));
	T a;
	TInt maxLength=a.MaxLength();
    TInt i;
	for (i=0;i<maxLength;i++)
		{
		test(a.Length()==i);	// Length
		test(a.Size()==(TInt)(sizeof(S)*i)); // Size
		a.AppendNum(i%10);
		}
	const T b=a;
	for (i=0;i<maxLength;i++)
		{
		test(a[i]=='0'+(i%10));	// At
		test(b[i]=='0'+(i%10));	// AtConst
		}
	test(a[0]==*a.Ptr());	//Ptr
	a.SetLength(0);	// SetLength
	test(a.Length()==0);
	a.SetLength(maxLength-1);
	test(a.Length()==maxLength-1);
	a.Zero();	// Null
	test(a.Length()==0);
	test.End();
	}

template <class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::Test6()
// Fill and Swap
	{
	test.Start(_L("Fill and Swap"));
	T a,b;
	TChar chr;
	TInt j;
	TInt maxLength=a.MaxLength();
	for (TInt i=0;i<maxLength;i++)
		{
		chr=(i%10)+'0';
		a.SetLength(i);
		a.Fill(chr);	// Fill(TChar)
		b.Swap(a);
		test(b.Length()==i);	// Swap
		j=0;
		while (j<i)
			test(b[j++]=='0'+(i%10));
		b.FillZ();	// FillZ()
		a.Swap(b);
		j=0;
		while (j<i)
			test(a[j++]==0);
		a.Fill(chr,i);	// Fill(TChar,TUint)
		j=0;
		while (j<i)
			test(a[j++]=='0'+(i%10));
		a.FillZ(i);	// FillZ(TUint)
		j=0;
		while (j<i)
			test(a[j++]==0);
		}
	test.End();
	}

template <class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::Test7()
// Conversion operators
	{
	test.Start(_L("Fold, collate ..."));
	T a;
	T b;
	a=_TL("abc AbC");
	b=_TL("ABC ABC");
	a.Fold();
	b.Fold();
	test(a==b);
	a=_TL("abc AbC");
	b=_TL("ABC ABC");
	a.Collate();
	b.Collate();
	test(a==b);
	a.LowerCase();
	test(a==_TL("abc abc"));
	a.Capitalize();
	test(a==_TL("Abc abc"));
	a.UpperCase();
	test(a==_TL("ABC ABC"));
	test.End();
	}

template <class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::Test8()
// Comparison
	{
	test.Start(_L("By string"));
	T a;
	a=_TL("abc AbC");
	test(a.Compare(_TL("abc AbC"))==0);
	test(a.CompareF(_TL("ABC aBc"))==0);
	test(a.Compare(_TL("xyz"))!=0);
	test(a.CompareC(_TL("xyz"))!=0);
	test(a.CompareF(_TL("xyz"))!=0);

	test.Next(_L("By buffer"));
	T b;
	T c;
	a=_TL("abc AbC");
	b=_TL("abc AbC");
	c=_TL("xyz");
	test(a.Compare(b)==0);
	test(a.Compare(c)!=0);
	b=_TL("ABC aBc");
	test(a.CompareC(c)!=0);
	test(a.CompareF(b)==0);
	test(a.CompareF(c)!=0);

	test.End();
	}

template <class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::Test9()
// Matching (need to test explicit result as error KErrNotFound = KMaxTUint
// 			 and so registers as TRUE. (test parameter is TUint) )
	{
	test.Start(_L("By string"));
	T a;
	a=_TL("abc AbC");
	test(a.Match(_TL("abc AbC"))==0);
	test(a.MatchC(_TL("ABC aBc"))==0);
	test(a.MatchF(_TL("ABC aBc"))==0);
	test(a.Match(_TL("xyz"))==KErrNotFound);
	test(a.MatchC(_TL("xyz"))==KErrNotFound);
	test(a.MatchF(_TL("xyz"))==KErrNotFound);

	test.Next(_L("By buffer"));
	T b;
	T c;
	a=_TL("abc AbC");
	b=_TL("abc AbC");
	c=_TL("xyz");
	test(a.Match(b)==0);
	test(a.Match(c)==KErrNotFound);
	b=_TL("ABC aBc");
	test(a.MatchC(b)==0);
	test(a.MatchC(c)==KErrNotFound);
	test(a.MatchF(b)==0);
	test(a.MatchF(c)==KErrNotFound);

	test.Next(_L("Wildcards"));
	a=_TL("abcxyz");
	test(a.Match(_TL("abc*"))==0);
	test(a.Match(_TL("abw*"))==KErrNotFound);
	a=_TL("abcdefg");
	test(a.Match(_TL("a*fg"))==0);
	test(a.Match(_TL("a*f"))==KErrNotFound);
	test(a.Match(_TL("abc*fgh"))==KErrNotFound);
	a=_TL("abcdef");
	test(a.Match(_TL("abc?ef"))==0);
	test(a.Match(_TL("abc?xf"))==KErrNotFound);

	a=_TL("a(01)");
	test(a.Match(_TL("*(01)"))==1);
	test(a.Match(_TL("?(01)"))==0);
	test(a.Match(_TL("?(*)"))==0);
	test(a.Match(_TL("?(**)"))==0);

	test(a.Match(_TL("?(\?\?)"))==0);
	test(a.Match(_TL("*(*)"))>=0);
	test(a.Match(_TL("*(0?)"))>=0);
	test(a.Match(_TL("a(\?\?)"))==0);
	test(a.Match(_TL("*(\?\?)"))>=0);

	test.Next(_L("wild cards with collated comparison"));
	a = _TL("abcdefghijkl");
	test(a.MatchC(_TL("abc*")) == 0);
	test(a.MatchC(_TL("abc")) == KErrNotFound);
	test(a.MatchC(_TL("xyz")) == KErrNotFound);
	test(a.MatchC(_TL("*def")) == KErrNotFound);
	test(a.MatchC(_TL("*def*")) == 3);
	test(a.MatchC(_TL("*d?f*")) == 3);
	test(a.MatchC(_TL("a*kl")) == 0);
	test(a.MatchC(_TL("*e*?l")) == 4);
	test(a.MatchC(_TL("abc*dEf*")) == 0);
	
	
	T candidate;
	T search;
	
	candidate = _TL("");
	search = _TL("**");
	test(candidate.MatchC(search) == 0);
	
	candidate = _TL("");
	search = _TL("*");
	test(candidate.MatchC(search) == 0);
	
	candidate = _TL("abcd");
	search = _TL("*abc*cd");
	test(candidate.MatchC(search) == KErrNotFound);
   	
	if (sizeof(S) == 2)
		{
		test.Next(_L("Unicode MatchC and FindC treat base+accent as equal to composed character"));
		TPtrC p = _L("te\x302te");
		test(p.MatchC(_L("t\xeate")) == 0);
		test(p.FindC(_L("t\xeate")) == 0);
		}

	test.End();
	}

template <class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::Test10()
// Locating
	{
	T a;
	TChar b;

	test.Start(_L("First Char"));
	b='a';
	a=_TL("axaxa");
	test(a.Locate(b)==0);
	test(a.LocateF(b)==0);
	test(a.LocateReverse(b)==4);
	test(a.LocateReverseF(b)==4);

	test.Next(_L("Middle Char"));
	a=_TL("xaxa");
	test(a.Locate(b)==1);
	test(a.LocateF(b)==1);
	a=_TL("axax");
	test(a.LocateReverse(b)==2);
	test(a.LocateReverseF(b)==2);

	test.Next(_L("Last Char"));
	a=_TL("xxa");
	test(a.Locate(b)==2);
	test(a.LocateF(b)==2);
	a=_TL("axx");
	test(a.LocateReverse(b)==0);
	test(a.LocateReverseF(b)==0);

	test.Next(_L("Test for failure of locate"));
	a=_TL("xxx");
	test(a.Locate(b)==KErrNotFound);
	test(a.LocateF(b)==KErrNotFound);
	test(a.LocateReverse(b)==KErrNotFound);
	test(a.LocateReverseF(b)==KErrNotFound);

	test.End();
	}

template <class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::Test11()
// Copying
	{
	T a, b;
		
	test.Start(_L("By String"));
	a.Copy(_TL("abc"));
	test(a.Length()==3);
	test(a==_TL("abc"));
	a.CopyF(_TL("abc"));
	test(a.Length()==3);
	b.CopyF(_TL("ABC"));
	test(a==b);
	a.CopyLC(_TL("AbC"));
	test(a==_TL("abc"));
	test(a.Length()==3);
	a.CopyC(_TL("abc"));
	b.CopyC(_TL("ABC"));
	test(a==b);
	test(a.Length()==3);
	a.CopyCP(_TL("abc"));
	test(a==_TL("Abc"));
	test(a.Length()==3);
	a.CopyUC(_TL("aBc"));
	test(a==_TL("ABC"));
	test(a.Length()==3);
//	a.Copy(_TL("abc"),3);
//	test(a==_TL("abc"));
//	test(a.Length()==3);
//	a.Copy(_TL("abcd"),3);
//	test(a==_TL("abc"));
//	test(a.Length()==3);

	test.Next(_L("By buffer"));
	b=_TL("abc");
	a.Copy(b);
	test(a==_TL("abc"));
	test(a.Length()==3);
	a=_TL("");
	a.CopyF(b);
	b.CopyF(_TL("ABC"));
	test(a==b);
	test(a.Length()==3);
	a=_TL("");
	b=_TL("AbC");
	a.CopyLC(b);
	test(a==_TL("abc"));
	test(a.Length()==3);
	a=_TL("");
	b=_TL("abC");
	a.CopyC(b);
	b.CopyC(_TL("ABC"));
	test(a==b);
	test(a.Length()==3);
	a=_TL("");
	b=_TL("abC");
	a.CopyCP(b);
	test(a==_TL("Abc"));
	test(a.Length()==3);
	a=_TL("");
	b=_TL("aBc");
	a.CopyUC(b);
	test(a.Length()==3);
	test(a==_TL("ABC"));

	test.End();
	}


template <class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::Test12()
// Finding
	{
	test.Start(_L("By String"));
	T a,b;
	a=_TL("abccef");
	test(a.Find(_TL(""))==0);
	test(a.Find(_TL("abc"))==0);
	test(a.Find(_TL("cce"))==2);
	test(a.Find(_TL("cef"))==3);
	test(a.Find(_TL("efg"))==KErrNotFound);
	test(a.Find(_TL("xxx"))==KErrNotFound);
	test(a.FindF(_TL(""))==0);
	test(a.FindF(_TL("AbC"))==0);
	test(a.FindF(_TL("CcE"))==2);
	test(a.FindF(_TL("CeF"))==3);
	test(a.FindF(_TL("efg"))==KErrNotFound);
	test(a.FindF(_TL("xxx"))==KErrNotFound);
	test(a.FindC(_TL(""))==0);
	test(a.FindC(_TL("aBc"))==0);
	test(a.FindC(_TL("cce"))==2);
	test(a.FindC(_TL("cEf"))==3);
	test(a.FindC(_TL("efg"))==KErrNotFound);
	test(a.FindC(_TL("xxx"))==KErrNotFound);

	test.Next(_L("By buffer"));
	test(a.Find(b)==0);
	test(a.FindF(b)==0);
	test(a.FindC(b)==0);
	b=_TL("xxx");
	test(a.Find(b)==KErrNotFound);
	test(a.FindF(b)==KErrNotFound);
	test(a.FindC(b)==KErrNotFound);
	b=_TL("efg");
	test(a.Find(b)==KErrNotFound);
	test(a.FindF(b)==KErrNotFound);
	test(a.FindC(b)==KErrNotFound);
	b=_TL("abc");
	test(a.Find(b)==0);
	b=_TL("cce");
	test(a.Find(b)==2);
	b=_TL("cef");
	test(a.Find(b)==3);
	b=_TL("AbC");
	test(a.FindF(b)==0);
	b=_TL("CcE");
	test(a.FindF(b)==2);
	b=_TL("CeF");
	test(a.FindF(b)==3);
	b=_TL("aBc");
	test(a.FindC(b)==0);
	b=_TL("cCe");
	test(a.FindC(b)==2);
	b=_TL("cEf");
	test(a.FindC(b)==3);

	test.End();
	}

template <class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::Test13()
// Basic like ops
	{
	test.Start(_L("Repeat, trim, insert and delete"));
	T a,b;
	TInt max=a.MaxLength(); 
	b=_TL("abc");
	a.Repeat(_TL("abc"));
	test(a==_TL(""));
	a.Repeat(b);
	test(a==_TL(""));
	for (TInt j=1;j<max;j++) // ?? Cannot SetLength = MaxLength
		{
		a.SetLength(j);
		a.Repeat(_TL("abc"));
        TInt i;
		for (i=0;i<j;i++)
			test(a[i]==b[i%3]);
		a=_TL("");
		a.SetLength(j);
		a.Repeat(b);
		for (i=0;i<j;i++)
			test(a[i]==b[i%3]);
		a=_TL("");
		}
	a=_TL("\t\n  ab \t\n ");
	a.TrimLeft();
	test(a==_TL("ab \t\n "));
	test(a.Length()==6);
	a=_TL("\t\n  ab \t\n ");
	a.TrimRight();
	test(a==_TL("\t\n  ab"));
	test(a.Length()==6);
	a=_TL(" \t\n ab \t \n");
	a.Trim();
	test(a==_TL("ab"));
	a.Trim();
	test(a==_TL("ab"));
	a=_TL("abc");
	b=_TL("123");
	a.Insert(1,b);
	test(a==_TL("a123bc"));
	test(a.Length()==6);
	b=_TL("");
	a.Insert(4,b);
	test(a==_TL("a123bc"));
	test(a.Length()==6);
	a.Insert(0,b);
	test(a==_TL("a123bc"));
	test(a.Length()==6);
	a.Delete(1,3);
	test(a==_TL("abc"));
	test(a.Length()==3);
	a.Delete(0,3);
	test(a==_TL(""));
	test(a.Length()==0);

	test.Next(_L("TrimAll"));
	a=_TL("");
	a.TrimAll();
	test(a==_TL(""));
	a=_TL(" ");
	a.TrimAll();
	test(a==_TL(""));
	a=_TL("   ");
	a.TrimAll();
	test(a==_TL(""));
	a=_TL("    ab cd  ef    g");
	a.TrimAll();
	test(a==_TL("ab cd ef g"));
	a=_TL("abcdef");
	a.TrimAll();
	test(a==_TL("abcdef"));
	a=_TL("a  b\t cd\t\tef");
	a.TrimAll();
	test(a==_TL("a b\tcd\tef"));
	a=_TL("abcdef \t ghijk");
	a.TrimAll();
	test(a==_TL("abcdef ghijk"));
	a=_TL("abcdef g");
	a.TrimAll();
	test(a==_TL("abcdef g"));
	a=_TL("ab  cd  ef  gh  ij");
	a.TrimAll();
	test(a==_TL("ab cd ef gh ij"));
	a=_TL("a        b          c     defg h     i  jk l     mno pqr stu  vw   xyz");
	a.TrimAll();
	test(a==_TL("a b c defg h i jk l mno pqr stu vw xyz"));

	test.Next(_L("Right, Left and Mid"));
	a=_TL("abcdef");
	b = a.Left(3);
	test(b==_TL("abc"));
	test(b.Length()==3);
	b = a.Right(3);
	test(b==_TL("def"));
	b = a.Mid(2);
	test(b==_TL("cdef"));
	test(b.Length()==4);
	b = a.Left(2);
	test(b==_TL("ab"));
	test(b.Length()==2);
	b = a.Right(2);
	test(b==_TL("ef"));
	b = a.Mid(2,1);
	test(b==_TL("c"));
	test(b.Length()==1);
	b = a.Left(6);
	test(b==_TL("abcdef"));
	test(b.Length()==6);
	b=_TL("");
	b.SetLength(4);
	b = a.Right(6);
	test(b==_TL("abcdef"));
	test(b.Length()==6);
	a = a.Left(6);
	test(a==_TL("abcdef"));
	b=_TL("");
	b = a.Mid(0,6);
	test(b==_TL("abcdef")); 
	test.End();
	}

template <class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::Test14()
// Formating operations
	{
	test.Start(_L("Justify"));
	T a,b,d;
	TInt aWidth;
	TChar c;
	a=_TL("wxyz");
	b=_TL("abc");
	d=_TL("linearisation");
	const S* pD=_TS("299792458");
	c='x';
	aWidth=KDefaultJustifyWidth; // Left justified, Default width
	a.Justify(b,aWidth,ELeft,c);
	test(a==b);
	test(a.Length()==3);
	a.AppendJustify(b,aWidth,ELeft,c);
	test(a==_TL("abcabc"));
	test(a.Length()==6);
	aWidth=1;	// Width < String length
	a.Justify(b,aWidth,ELeft,c);
	test(a==_TL("a"));
	test(a.Length()==1);
	a.AppendJustify(b,aWidth,ELeft,c);
	test(a==_TL("aa"));
	test(a.Length()==2);
	aWidth=5; // Width > String length
	a.Justify(b,aWidth,ELeft,c);
	test(a==_TL("abcxx"));
	test(a.Length()==5);
	a.AppendJustify(b,aWidth,ELeft,c);
	test(a==_TL("abcxxabcxx"));
	test(a.Length()==10);

	aWidth=KDefaultJustifyWidth; // Right justified, Default width
	a.Justify(b,aWidth,ERight,c);
	test(a==b);
	test(a.Length()==3);
	a.AppendJustify(b,aWidth,ERight,c);
	test(a==_TL("abcabc"));
	test(a.Length()==6);
	aWidth=1; // Right justified, Width < String length
	a.Justify(b,aWidth,ERight,c);
	test(a==_TL("a"));
	test(a.Length()==1);
	a.AppendJustify(b,aWidth,ERight,c);
	test(a==_TL("aa"));
	test(a.Length()==2);
	aWidth=5; // Right justified, width > String length
	a.Justify(b,aWidth,ERight,c);
	test(a==_TL("xxabc"));
	test(a.Length()==5);
	a.AppendJustify(b,aWidth,ERight,c);
	test(a==_TL("xxabcxxabc"));
	test(a.Length()==10);

	aWidth=KDefaultJustifyWidth; // Center justified, Default width
	a.Justify(b,aWidth,ECenter,c);
	test(a==b);
	test(a.Length()==3);
	a.AppendJustify(b,aWidth,ECenter,c);
	test(a==_TL("abcabc"));
	test(a.Length()==6);
	aWidth=1; // Centre justified, width < String length
	a.Justify(b,aWidth,ECenter,c);
	test(a==_TL("a"));
	test(a.Length()==1);
	a.AppendJustify(b,aWidth,ECenter,c);
	test(a==_TL("aa"));
	test(a.Length()==2);
	aWidth=5; // Centre justified, width > String length
	a.Justify(b,aWidth,ECenter,c);
	test(a==_TL("xabcx"));
	test(a.Length()==5);
	a.AppendJustify(b,aWidth,ECenter,c);
	test(a==_TL("xabcxxabcx"));
	test(a.Length()==10);

	test.Next(_L("Num"));
	TInt j=-2147483647-1; 
	a.Num(j);
	test(a==_TL("-2147483648"));
	test(a.Length()==11);
	TUint i=2147483648u;
	a.Num(i);
	test(a==_TL("2147483648"));
	test(a.Length()==10);
	if (a.MaxLength()>31)
		{
		a.Num(i,EBinary);
		test(a==_TL("10000000000000000000000000000000"));
		test(a.Length()==32);
		a=_TL("");
		a.NumUC(i,EBinary);
		test(a==_TL("10000000000000000000000000000000"));
		test(a.Length()==32);
		}
	i=31;
	a.Num(i,EBinary);
	test(a==_TL("11111"));
	test(a.Length()==5);
	a=_TL("");
	a.NumUC(i,EBinary);
	test(a==_TL("11111"));
	test(a.Length()==5);
	i=2147483648u;
	a.Num(i,EOctal);
	test(a==_TL("20000000000"));
	test(a.Length()==11);
	a=_TL("");
	a.NumUC(i,EOctal);
	test(a==_TL("20000000000"));
	test(a.Length()==11);
	a.Num(i,EDecimal);
	test(a==_TL("2147483648"));
	test(a.Length()==10);
	a=_TL("");
	a.NumUC(i,EDecimal);
	test(a==_TL("2147483648"));
	test(a.Length()==10);
	a.Num(i,EHex);
	test(a==_TL("80000000"));
	test(a.Length()==8);
	a=_TL("");
	a.NumUC(i,EHex);
	test(a==_TL("80000000"));
	test(a.Length()==8);
	i=0;
	a.Num(i);
	test(a==_TL("0"));
	test(a.Length()==1);
	a=_TL("abc");
	a.Num(i,EBinary);
	test(a==_TL("0"));
	test(a.Length()==1);
	a=_TL("abc");
	a.NumUC(i,EBinary);
	test(a==_TL("0"));
	test(a.Length()==1);
	a=_TL("abc");
	a.Num(i,EOctal);
	test(a==_TL("0"));
	test(a.Length()==1);
	a=_TL("");
	a.NumUC(i,EOctal);
	test(a==_TL("0"));
	test(a.Length()==1);
	a=_TL("abc");
	a.Num(i,EDecimal);
	test(a==_TL("0"));
	test(a.Length()==1);
	a=_TL("");
	a.NumUC(i,EDecimal);
	test(a==_TL("0"));
	test(a.Length()==1);
	a=_TL("abc");
	a.Num(i,EHex);
	test(a==_TL("0"));
	test(a.Length()==1);
	a=_TL("");
	a.NumUC(i,EHex);
	test(a==_TL("0"));
	test(a.Length()==1);
//	TInt i=a.Num(6.2,format); NOT IMPLEMENTED
	a.AppendNum(j);
	test(a==_TL("0-2147483648"));
	test(a.Length()==12);
	a=_TL("abc");
	i=4294967295u;
	a.AppendNum(i);
	test(a==_TL("abc4294967295"));
	test(a.Length()==13);
	j=2147483647;
	a=_TL("abc");
	a.AppendNum(j);
	test(a==_TL("abc2147483647"));
	test(a.Length()==13);
	a=_TL("a");
	i=180150000;
	if (a.MaxLength()>28)
		{
		a.AppendNum(i,EBinary);
		test(a==_TL("a1010101111001101111011110000"));
		test(a.Length()==29);
		}
	a=_TL("a");
	a.AppendNum(15,EBinary);
	test(a==_TL("a1111"));
	test(a.Length()==5);
	a=_TL("a");
	a.AppendNum(i,EDecimal);
	test(a==_TL("a180150000"));
	test(a.Length()==10);
	a=_TL("a");
	a.AppendNum(i,EOctal);
	test(a==_TL("a1257157360"));
	test(a.Length()==11);
	a=_TL("a");
	a.AppendNumUC(i,EHex);
	test(a==_TL("aABCDEF0"));
	test(a.Length()==8);
//	i=a.AppendNum(6.32, format); NOT IMPLEMENTED

	test.Next(_L("Format"));
	a=_TL("");
	b=_TL("cde");
	a.Format(_TL("%S"),&b);
	test(a==b);
	test(a.Length()==3);
    DESTEMPLATE xyz=_TL("xyzwpq");
	a.Format(_TL("%S"),&xyz);
	test(a==_TL("xyzwpq"));
	test(a.Length()==6);
    DESTEMPLATE cde=_TL("cde");
	a.Format(_TL("ab %-x5S"),&cde);
	test(a==_TL("ab cdexx"));
	test(a.Length()==8);
	a.Format(_TL("ab %=x5S"),&cde);
	test(a==_TL("ab xcdex"));
	test(a.Length()==8);
	a.Format(_TL("ab %+x5S"),&cde);
	test(a==_TL("ab xxcde"));
	test(a.Length()==8);
	a.Format(_TL("ab %5S"),&cde);
	test(a==_TL("ab   cde"));
	test(a.Length()==8);
	a.Format(_TL("ab %-**S"),'x',5,&cde);
	test(a==_TL("ab cdexx"));
	test(a.Length()==8);
	a.Format(_TL("ab %*S"),5,&cde);
	test(a==_TL("ab   cde"));
	test(a.Length()==8);
	a=_TL("xyz");
	a.Format(_TL("ab %-x5S"),&b);
	test(a==_TL("ab cdexx"));
	test(a.Length()==8);
	a=_TL("xyz");
	a.Format(_TL("ab %-**S"),'x',5,&b);
	test(a==_TL("ab cdexx"));
	test(a.Length()==8);
	a=_TL("xyz");
	a.Format(_TL("ab %*S"),5,&b);
	test(a==_TL("ab   cde"));
	test(a.Length()==8);

	DESTEMPLATE fred=_TL("fred");
	a.Format(_TL("%+0*S"),10,&fred);
	test(a==_TL("000000fred"));
	test(a.Length()==10);
	a.Format(_TL("%-0*S"),7,&fred);
	test(a==_TL("fred000"));
	test(a.Length()==7);
	a.Format(_TL("%0*S"),11,&fred);
	test(a==_TL("0000000fred"));
	test(a.Length()==11);
	a.Format(_TL("c=%s"),pD);
	TESTEQ(a,_TL("c=299792458"));
	a.Format(_TL("c=%10.6s"),pD);
	test(a==_TL("c=    299792"));
	a.Format(_TL("c=%*.*s"),5,4,pD);
	test(a==_TL("c= 2997"));
	a.Format(_TL("%S"),&d);
	test(a==_TL("linearisation"));
	a.Format(_TL("%10.6S"),&d);
	test(a==_TL("    linear"));
	a.Format(_TL("%*.*S"),5,4,&d);
	test(a==_TL(" line"));
	a.Format(_TL("%*.*Sed"),10,8,&d);
	test(a==_TL("  linearised"));
	a.Format(_TL("%*.*S"),14,20,&d);
	test(a==_TL(" linearisation"));

	a.Format(_TL("ab %-,5b"),7);
	test(a==_TL("ab 111,,"));
	test(a.Length()==8);
	a.Format(_TL("ab %=,5O"),31);
	test(a==_TL("ab ,37,,"));
	test(a.Length()==8);
	a.Format(_TL("ab %+xlx"),TInt64(171));
	test(a==_TL("ab ab"));
	test(a.Length()==5);
	a.Format(_TL("ab %+xlX %+xlx"),TInt64(171),TInt64(171));
	TESTEQ(a,_TL("ab AB ab"));
	test(a.Length()==8);
	a.Format(_TL("ab %lu"),MAKE_TINT64((TUint)(KMinTInt),0));
	test(a==_TL("ab 9223372036854775808"));
	test(a.Length()==22);
	a.Format(_TL("ab %ld"),MAKE_TINT64((TUint)(KMinTInt),1));
	test(a==_TL("ab -9223372036854775807"));
	test(a.Length()==23);
	a.Format(_TL("ab %ld"),MAKE_TINT64((TUint)(KMinTInt),0));
	test(a==_TL("ab -9223372036854775808"));
	test(a.Length()==23);
	a.Format(_TL("ab %ld"),MAKE_TINT64((TUint)(KMaxTInt),KMaxTUint));
	test(a==_TL("ab 9223372036854775807"));
	test(a.Length()==22);
	a.Format(_TL("ab %ld"),MAKE_TINT64(KMaxTUint,KMaxTUint));
	test(a==_TL("ab -1"));
	test(a.Length()==5);
	a.Format(_TL("ab %lu"),MAKE_TINT64(KMaxTUint,KMaxTUint));
	test(a==_TL("ab 18446744073709551615"));
	test(a.Length()==23);
	a.Format(_TL("ab %ld"),TInt64(0));
	test(a==_TL("ab 0"));
	test(a.Length()==4);
	a.Format(_TL("ab %lb"),TInt64(0));
	test(a==_TL("ab 0"));
	test(a.Length()==4);
	a.Format(_TL("ab %lx"),TInt64(0));
	test(a==_TL("ab 0"));
	test(a.Length()==4);
	a.Format(_TL("ab %lo"),TInt64(0));
	test(a==_TL("ab 0"));
	test(a.Length()==4);
	a.Format(_TL("ab %lu"),TInt64(0));
	test(a==_TL("ab 0"));
	test(a.Length()==4);
	a.Format(_TL("ab %lb"),MAKE_TINT64((TUint)(KMaxTInt),KMaxTUint));
	test(a==_TL("ab 111111111111111111111111111111111111111111111111111111111111111"));
	test(a.Length()==66);
	a.Format(_TL("ab %lb"),MAKE_TINT64(KMaxTUint,KMaxTUint));
	test(a==_TL("ab 1111111111111111111111111111111111111111111111111111111111111111"));
	test(a.Length()==67);
	a.Format(_TL("ab %lx"),MAKE_TINT64((TUint)(KMaxTInt),KMaxTUint));
	test(a==_TL("ab 7fffffffffffffff"));
	test(a.Length()==19);
	a.Format(_TL("ab %lx"),MAKE_TINT64(KMaxTUint,KMaxTUint));
	test(a==_TL("ab ffffffffffffffff"));
	test(a.Length()==19);
	a.Format(_TL("ab %lo"),MAKE_TINT64((TUint)(KMaxTInt),KMaxTUint));
	test(a==_TL("ab 777777777777777777777"));
	test(a.Length()==24);
	// tests which excercise any 8 byte alignment requirement on 64bit integers
	a.Format(_TL("%li%S"),MAKE_TINT64(1,2),&fred);
	test(a==_TL("4294967298fred"));
	a.Format(_TL("%S%li%S"),&fred,MAKE_TINT64(1,2),&fred);
	test(a==_TL("fred4294967298fred"));
	a.Format(_TL("%lu%S"),MAKE_TINT64(1,2),&fred);
	test(a==_TL("4294967298fred"));
	a.Format(_TL("%S%lu%S"),&fred,MAKE_TINT64(1,2),&fred);
	test(a==_TL("fred4294967298fred"));

	a.Format(_TL("ab %U"),233);
	test(a==_TL("ab 233"));
	test(a.Length()==6);
	a.Format(_TL("ab %*d"),5,-131);
	test(a==_TL("ab  -131"));
	test(a.Length()==8);
	a.Format(_TL("ab%c"),'x');
	test(a==_TL("abx"));
	test(a.Length()==3);
	a.Format(_TL("%W"),-131);
	test(*(TInt32*)a.Ptr()==-131);
	a.Format(_TL("%M"),-131);
	test(*(TInt32*)a.Ptr()==2113929215);
	a.Format(_TL("%w"),-131);
	test(*(TInt16*)a.Ptr()==-131);
	a.Format(_TL("%m"),-131);
	test(*(TInt16*)a.Ptr()==32255);
	a=_TL("xyz");
	a.AppendFormat(_TL("ab %+xlx"),TInt64(171));
	test(a==_TL("xyzab ab"));
	test(a.Length()==8);
	a=_TL("xyz");
	a.AppendFormat(_TL("ab %5S"),&b);
	test(a==_TL("xyzab   cde"));
	test(a.Length()==11);
	a=_TL("xyz");
	a.AppendFormat(_TL("%W"),-131);
//	test(*(TInt32*)(a.Ptr()+3)==-131); // Alignment-safe version:
    TInt val;
    Mem::Copy(&val,&a[3],4);
	test(val==-131);
	a=_TL("xyz");
//	a.Append(_TL("abc"),3);
//	test(a==_TL("xyzabc"));

	a.Format(_TL(""));
	test(a==_TL(""));
	a.Format(_TL(""),9,5);
	test(a==_TL(""));
	a.Format(_TL("qwerty"));
	test(a==_TL("qwerty"));
	a.Format(_TL("qwerty"),9,5);
	test(a==_TL("qwerty"));
	a.Format(_TL("%$1$d%$2$d"),9,5);
	test(a==_TL("95"));
	test(a.Length()==2);
	a.Format(_TL("%$2$d%$1$d"),9,5);
	test(a==_TL("59"));
	test(a.Length()==2);
	b=_TL("eb");
	a.Format(_TL("%$1$S%$2$d"),&b,205);
	test(a==_TL("eb205"));
	test(a.Length()==5);
	a.Format(_TL("%$2$d%$1$S"),&b,205);
	test(a==_TL("205eb"));
	test(a.Length()==5);
	b=_TL("ebdb");

// Cannot do this on GCC (X86) because of "Cannot pass objects of non-POD type through '...'. Call will abort at runtime".
#if !(defined(__GCC32__) && defined(__X86__))
	const TAny* const zeroTerminatedString=(sizeof(S)==2)? (const TAny*)_S16(":-)E"): (const TAny*)_S8(":-)E");
	const TInt dummyParameter=0;
#ifdef __ARMCC__
#pragma push
#pragma diag_suppress 1446 
#endif
	Test14_ReorderedParameterFormatting(dummyParameter, 0x20ac, 11, 3, 13.89543, zeroTerminatedString, '!', TInt64(199), 2, &b, 6, 30005, TRealX(0.125), 0x8bdd);
#ifdef __ARMCC__
#pragma pop
#endif
#endif

	test.Next(_L("Print some numbers"));
	TInt64 TI64 = MAKE_TINT64(0x101010u,0x10101010u);
	test.Printf(_L("    %%ld: %ld\n"),TI64);
	test.Printf(_L("    %%lu: %lu\n"),TI64);
	test.Printf(_L("    %%lx: %lx\n"),TI64);
	test.Printf(_L("    %%lb: %lb\n"),TI64);
	test.Printf(_L("    %%lo: %lo\n\n"),TI64);
	TI64 = UI64LIT(0xabcdef12345678);
	test.Printf(_L("    %%ld: %ld\n"),TI64);
	test.Printf(_L("    %%lu: %lu\n"),TI64);
	test.Printf(_L("    %%lx: %lx\n"),TI64);
	test.Printf(_L("    %%lb: %lb\n"),TI64);
	test.Printf(_L("    %%lo: %lo\n\n"),TI64);
	TI64 = UI64LIT(0x7fffffffffffffff);
	test.Printf(_L("    %%ld: %ld\n"),TI64);
	test.Printf(_L("    %%lu: %lu\n"),TI64);
	test.Printf(_L("    %%lx: %lx\n"),TI64);
	test.Printf(_L("    %%lb: %lb\n"),TI64);
	test.Printf(_L("    %%lo: %lo\n\n"),TI64);
	TI64 = UI64LIT(0x8000000000000000);
	test.Printf(_L("    %%ld: %ld\n"),TI64);
	test.Printf(_L("    %%lu: %lu\n"),TI64);
	test.Printf(_L("    %%lx: %lx\n"),TI64);
	test.Printf(_L("    %%lb: %lb\n"),TI64);
	test.Printf(_L("    %%lo: %lo\n\n"),TI64);
	TI64 = UI64LIT(0xffffffffffffffff);
	test.Printf(_L("    %%ld: %ld\n"),TI64);
	test.Printf(_L("    %%lu: %lu\n"),TI64);
	test.Printf(_L("    %%lx: %lx\n"),TI64);
	test.Printf(_L("    %%lb: %lb\n"),TI64);
	test.Printf(_L("    %%lo: %lo\n\n"),TI64);

	test.Next(_L("Regression tests"));
	a.Format(_TL("[%-A4p]"));
	test(a==_TL("[AAAA]"));

	test.End();
	}

template <class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::Test14_ReorderedParameterFormatting(TInt aDummyParameter, ...)
	{
	VA_LIST parameterList;
	T generated;
	T expected;

	VA_START(parameterList, aDummyParameter);
	generated.FormatList(_TL("\t%-**.*fqq%.3swww%+*5Ldeeee%.*Srrrrr%0*xtttttt%.3Fyyyyyyy%c"), parameterList);
	test(generated.Length()==61);
	expected.Format(_TL("\t13.895%c%c%c%c%cqq:-)www!!199eeeeebrrrrr007535tttttt0.125yyyyyyy"), (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac);
	test(generated.Left(generated.Length()-1)==expected);
	test(generated[generated.Length()-1]==(S)0x8bdd);

	VA_START(parameterList, aDummyParameter);
	generated.FormatList(_TL("\t%$1$-**.*fqq%.3swww%+*5Ldeeee%.*Srrrrr%0*xtttttt%$6$.3Fyyyyyyy%c"), parameterList);
	test(generated.Length()==61);
	expected.Format(_TL("\t13.895%c%c%c%c%cqq:-)www!!199eeeeebrrrrr007535tttttt0.125yyyyyyy"), (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac);
	test(generated.Left(generated.Length()-1)==expected);
	test(generated[generated.Length()-1]==(S)0x8bdd);

	VA_START(parameterList, aDummyParameter);
	generated.FormatList(_TL("\t%$6$.3Fqq%.3swww%+*5Ldeeee%.*Srrrrr%0*xtttttt%$1$-**.*fyyyyyyy%c"), parameterList);
	test(generated.Length()==61);
	expected.Format(_TL("\t0.125qq:-)www!!199eeeeebrrrrr007535tttttt13.895%c%c%c%c%cyyyyyyy"), (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac);
	test(generated.Left(generated.Length()-1)==expected);
	test(generated[generated.Length()-1]==(S)0x8bdd);

	VA_START(parameterList, aDummyParameter);
	generated.FormatList(_TL("\t%-**.*fqq%.3swww%$5$0*xeeee%.*Srrrrr%$3$+*5Ldtttttt%.3Fyyyyyyy%c"), parameterList);
	test(generated.Length()==61);
	expected.Format(_TL("\t13.895%c%c%c%c%cqq:-)www007535eeeeebrrrrr!!199tttttt0.125yyyyyyy"), (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac);
	test(generated.Left(generated.Length()-1)==expected);
	test(generated[generated.Length()-1]==(S)0x8bdd);

	VA_START(parameterList, aDummyParameter);
	generated.FormatList(_TL("\t%-**.*fqq%$4$.*Swww%+*5Ldeeee%$2$.3srrrrr%0*xtttttt%.3Fyyyyyyy%c"), parameterList);
	test(generated.Length()==61);
	expected.Format(_TL("\t13.895%c%c%c%c%cqqebwww!!199eeee:-)rrrrr007535tttttt0.125yyyyyyy"), (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac);
	test(generated.Left(generated.Length()-1)==expected);
	test(generated[generated.Length()-1]==(S)0x8bdd);

	VA_START(parameterList, aDummyParameter);
	generated.FormatList(_TL("\t%-**.*fqq%.3swww%+*5Ldeeee%$7$crrrrr%0*xtttttt%.3Fyyyyyyy%$4$.*S"), parameterList);
	test(generated.Length()==61);
	expected.Format(_TL("\t13.895%c%c%c%c%cqq:-)www!!199eeee"), (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac);
	test(generated.Left(29)==expected);
	test(generated[29]==(S)0x8bdd);
	test(generated.Mid(29+1)==_TL("rrrrr007535tttttt0.125yyyyyyyeb"));

	VA_START(parameterList, aDummyParameter);
	generated.FormatList(_TL("\t%$4$.*Sqq%.3swww%+*5Ldeeee%$6$.3Frrrrr%0*xtttttt%$1$-**.*fyyyyyyy%c"), parameterList);
	test(generated.Length()==61);
	expected.Format(_TL("\tebqq:-)www!!199eeee0.125rrrrr007535tttttt13.895%c%c%c%c%cyyyyyyy"), (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac);
	test(generated.Left(generated.Length()-1)==expected);
	test(generated[generated.Length()-1]==(S)0x8bdd);

	VA_START(parameterList, aDummyParameter);
	generated.FormatList(_TL("\t%$7$cqq%$6$.3Fwww%$5$0*xeeee%.*Srrrrr%$3$+*5Ldtttttt%$2$.3syyyyyyy%$1$-**.*f"), parameterList);
	test(generated.Length()==61);
	test(generated.Left(1)==_TL("\t"));
	test(generated[1]==(S)0x8bdd);
	expected.Format(_TL("qq0.125www007535eeeeebrrrrr!!199tttttt:-)yyyyyyy13.895%c%c%c%c%c"), (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac);
	test(generated.Mid(2)==expected);

	VA_START(parameterList, aDummyParameter);
	generated.FormatList(_TL("\t%$7$cqq%$6$.3Fwww%$5$0*xeeee%$4$.*Srrrrr%$3$+*5Ldtttttt%$2$.3syyyyyyy%$1$-**.*f"), parameterList);
	test(generated.Length()==61);
	test(generated.Left(1)==_TL("\t"));
	test(generated[1]==(S)0x8bdd);
	expected.Format(_TL("qq0.125www007535eeeeebrrrrr!!199tttttt:-)yyyyyyy13.895%c%c%c%c%c"), (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac, (S)0x20ac);
	test(generated.Mid(2)==expected);
	}

template <class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::Test15()
// Replacing
	{
	test.Start(_L("Replace"));
	T a,b;
	test(a.MaxLength()>=9);
	a=_TL("abccccc");
	b=_TL("def");
	a.Replace(3,4,b); // Replace with smaller in middle (insert and delete)
	test(a==_TL("abcdef"));
	a.Replace(1,1,b); // Replace with larger in middle (insert and delete)
	test(a==_TL("adefcdef"));
	a.Replace(0,8,_TL("")); // Replace complete string (delete)
	test(a==_TL(""));
	a.Replace(0,0,b); // Replace at beginning (insert)
	test(a==b);
	a.Replace(3,0,_TL("xyz")); // Replace at end (append)
	test(a==_TL("defxyz"));
	a.Replace(0,0,_TL("")); // Replace nothing at beginning (do nothing)
	test(a==_TL("defxyz"));
	a.Replace(6,0,_TL("")); // Replace nothing at end (do nothing)
	test(a==_TL("defxyz"));
	//a.Replace(6,1,_TL("")); // this will panic - no char after end to replace
	//a.Replace(0,7,_TL("")); // this will panic - aint 7 chars to replace
	test.End();
	}

template<class T,class S,class DESTEMPLATE>
GLDEF_C void TestTBuf<T,S,DESTEMPLATE>::test_TBuf()
// Test the TBuf class.
	{
	test.Start(_L("All operations"));
	Test1();
	test.Next(_L("Constructors"));
	Test2();
	test.Next(_L("Additional tests"));
	Test3();
	test.Next(_L("Comparison operators"));
	Test4();
	test.Next(_L("Property access"));
	Test5();
	test.Next(_L("Fill and swap"));
	Test6();
	test.Next(_L("Conversion operators"));
	Test7();
	test.Next(_L("Comparison"));
	Test8();
	test.Next(_L("Matching"));
	Test9();
	test.Next(_L("Locating"));
	Test10();
	test.Next(_L("Copying"));
	Test11();
	test.Next(_L("Finding"));
	Test12();
	test.Next(_L("Basic like ops"));
	Test13();
	test.Next(_L("Formating"));
	Test14();
	test.Next(_L("Replacing"));
	Test15();
	test.End();
	}

LOCAL_C void testFormat()
	{
	TBuf<0x100> aa;
	aa.Format(_L("x%- 5fx"), 6.2345678);
	test(aa==_L("x6.234568x"));
	aa.Format(_L("x%+ 5fx"), 6.2345678);
	test(aa==_L("x6.234568x"));
	aa.Format(_L("x% 5fx"), 6.2345678);
	test(aa==_L("x6.234568x"));
	aa.Format(_L("x%= 5fx"), 6.2345678);
	test(aa==_L("x6.234568x"));
	aa.Format(_L("x%- 10fx"), 6.2345);
	test(aa==_L("x6.234500  x"));
	aa.Format(_L("x%+ 10fx"), 6.2345);
	test(aa==_L("x  6.234500x"));
	aa.Format(_L("x% 10fx"), 6.2345);
	test(aa==_L("x  6.234500x"));
	aa.Format(_L("x%= 10fx"), 6.2345);
	test(aa==_L("x 6.234500 x"));
	aa.Format(_L("x%10fx"), 12345352342.545);
	test(aa==_L("x12,345,352,342.545000x"));
	aa.Format(_L("x%20.9fx"), 1.0123456789);
	test(aa==_L("x         1.012345679x"));
	aa.Format(_L("x%5.1fx"), 1.99);
	test(aa==_L("x  2.0x"));

// Cannot do these on GCC (X86) because of "Cannot pass objects of non-POD type through '...'. Call will abort at runtime".
#if !(defined(__GCC32__) && defined(__X86__))
#ifdef __ARMCC__
#pragma push
#pragma diag_suppress 1446 
#endif
	aa.Format(_L("x%- 5Fx"), TRealX(6.2345678));
	test(aa==_L("x6.234568x"));
	aa.Format(_L("x%+ 5Fx"), TRealX(6.2345678));
	test(aa==_L("x6.234568x"));
	aa.Format(_L("x% 5Fx"), TRealX(6.2345678));
	test(aa==_L("x6.234568x"));
	aa.Format(_L("x%= 5Fx"), TRealX(6.2345678));
	test(aa==_L("x6.234568x"));
	aa.Format(_L("x%- 10Fx"), TRealX(6.2345));
	test(aa==_L("x6.234500  x"));
	aa.Format(_L("x%+ 10Fx"), TRealX(6.2345));
	test(aa==_L("x  6.234500x"));
	aa.Format(_L("x% 10Fx"), TRealX(6.2345));
	test(aa==_L("x  6.234500x"));
	aa.Format(_L("x%+010Fx"), TRealX(6.2345));
	test(aa==_L("x006.234500x"));
	aa.Format(_L("x%+10Fx"), TRealX(6.2345));
	test(aa==_L("x  6.234500x"));
	aa.Format(_L("x%10Fx"), TRealX(6.2345));
	test(aa==_L("x  6.234500x"));
	aa.Format(_L("x%010Fx"), TRealX(6.2345));
	test(aa==_L("x006.234500x"));
	aa.Format(_L("x%= 10Fx"), TRealX(6.2345));
	test(aa==_L("x 6.234500 x"));
	aa.Format(_L("x%10Fx"), TRealX(12345352342.545));
	test(aa==_L("x12,345,352,342.545000x"));
	aa.Format(_L("x%20.9Fx"), TRealX(1.0123456789));
	test(aa==_L("x         1.012345679x"));
	aa.Format(_L("x%5.1Fx"), TRealX(1.99));
	test(aa==_L("x  2.0x"));
#ifdef __ARMCC__	
#pragma pop
#endif
#endif

	aa.Format(_L("x%- 5ex"), 6.2345678);
	test(aa==_L("x6.234568E+00x"));
	aa.Format(_L("x%+ 5ex"), 6.2345678);
	test(aa==_L("x6.234568E+00x"));
	aa.Format(_L("x% 5ex"), 6.2345678);
	test(aa==_L("x6.234568E+00x"));
	aa.Format(_L("x%= 5ex"), 6.2345678);
	test(aa==_L("x6.234568E+00x"));
	aa.Format(_L("x%- 14ex"), 6.2345);
	test(aa==_L("x6.234500E+00  x"));
	aa.Format(_L("x%+ 14ex"), 6.2345);
	test(aa==_L("x  6.234500E+00x"));
	aa.Format(_L("x% 14ex"), 6.2345);
	test(aa==_L("x  6.234500E+00x"));
	aa.Format(_L("x%= 14ex"), 6.2345);
	test(aa==_L("x 6.234500E+00 x"));
	aa.Format(_L("x%10ex"), 12345352342.545);
	test(aa==_L("x1.234535E+10x"));
	aa.Format(_L("x%20.9ex"), 1.0123456789);
	test(aa==_L("x     1.012345679E+00x"));
	aa.Format(_L("x%5.1ex"), 1.99);
	test(aa==_L("x2.0E+00x"));
	}


class TO8 : public TDes8Overflow
	{
public:
	virtual void Overflow(TDes8 &aDes);
	};

void TO8::Overflow(TDes8 &aDes)
	{
	aDes=_L8("OVERFLOW");
	}

class TO8L : public TDes8Overflow
	{
public:
	virtual void Overflow(TDes8 &aDes);
	};

void TO8L::Overflow(TDes8 &/*aDes*/)
	{
	User::Leave(KErrOverflow);
	}

class TO16 : public TDes16Overflow
	{
public:
	virtual void Overflow(TDes16 &aDes);
	};

void TO16::Overflow(TDes16 &aDes)
	{
	aDes=_L16("OVERFLOW");
	}

class TO16L : public TDes16Overflow
	{
public:
	virtual void Overflow(TDes16 &aDes);
	};

void TO16L::Overflow(TDes16 &/*aDes*/)
	{
	User::Leave(KErrOverflow);
	}

void append8(TDes8 &aBuf, TDes8Overflow *aHandler, TRefByValue<const TDesC8> aFmt, ...)
	{
	VA_LIST list;
	VA_START(list, aFmt);
	aBuf.AppendFormatList(aFmt, list, aHandler);
	}

void append16(TDes16 &aBuf, TDes16Overflow *aHandler, TRefByValue<const TDesC16> aFmt, ...)
	{
	VA_LIST list;
	VA_START(list, aFmt);
	aBuf.AppendFormatList(aFmt, list, aHandler);
	}

void testOverflow()
	{
	test.Start(_L("Test no overflow"));
	TBuf8<16> buf=_L8("A ");
	append8(buf, NULL, _L8("Descriptor"));
	test(buf==_L8("A Descriptor"));

	test.Printf(_L("Use a non-leaving overflow handler\n"));
	test.Next(_L("Force overflow with no conversions"));
    TO8 overflow;
	append8(buf, &overflow, _L8("12345678901234567"));
	test(buf==_L8("OVERFLOW"));

	test.Next(_L("Force overflow with decimal conversion"));
	buf=_L8("A Descriptor");
	append8(buf, &overflow,  _L8("%d"), 12345678);
	test(buf==_L8("OVERFLOW"));

	test.Printf(_L("Use a leaving overflow handler\n"));
	test.Next(_L("AppendFormatList with no overflow"));
	buf=_L8("A Descriptor");
    TO8L overflowLeave;
	TRAPD(r, append8(buf, &overflowLeave, _L8("ONE")));
	test(r==KErrNone);
	test(buf==_L8("A DescriptorONE"));
	test.Next(_L("Force overflow with hexadecimal conversion"));
	buf=_L8("A Descriptor");
	TRAP(r, append8(buf, &overflowLeave, _L8("%08x"), 0));
	test(r==KErrOverflow);
		{
		test.Printf(_L("Repeat tests with TBuf16\n"));
		test.Next(_L("Test no overflow"));
		TBuf16<16> buf=_L16("A ");
		append16(buf, NULL, _L16("Descriptor"));
		test(buf==_L16("A Descriptor"));

		test.Printf(_L("Use a non-leaving overflow handler\n"));
		test.Next(_L("Force overflow with no conversions"));
		TO16 overflow;
		append16(buf, &overflow, _L16("12345678901234567"));
		test(buf==_L16("OVERFLOW"));

		test.Next(_L("Force overflow with decimal conversion"));
		buf=_L16("A Descriptor");
		append16(buf, &overflow,  _L16("%d"), 12345678);
		test(buf==_L16("OVERFLOW"));

		test.Printf(_L("Use a leaving overflow handler\n"));
		test.Next(_L("AppendFormatList with no overflow"));
		buf=_L16("A Descriptor");
		TO16L overflowLeave;
		TRAPD(r, append16(buf, &overflowLeave, _L16("ONE")));
		test(r==KErrNone);
		test(buf==_L16("A DescriptorONE"));
		test.Next(_L("Force overflow with hexadecimal conversion"));
		buf=_L16("A Descriptor");
		TRAP(r, append16(buf, &overflowLeave, _L16("%08x"), 0));
		test(r==KErrOverflow);
		}
	test.End();
	}

void testIgnoreOverflow()
	{
	test.Start(_L("Test no overflow"));
	TBuf8<16> buf=_L8("A ");
	append8(buf, NULL, _L8("Descriptor"));
	test(buf==_L8("A Descriptor"));

	test.Printf(_L("Use a non-leaving overflow handler\n"));
	test.Next(_L("Force overflow with no conversions"));
    TDes8IgnoreOverflow overflow;
	append8(buf, &overflow, _L8("12345678901234567"));
	test(buf==_L8("A Descriptor1234"));

	test.Next(_L("Force overflow with decimal conversion"));
	buf=_L8("A Descriptor");
	append8(buf, &overflow,  _L8("%d"), 123456789);
	test(buf==_L8("A Descriptor"));

	//test.Printf(_L("Repeat tests with TBuf16\n"));
	test.Next(_L("Test no overflow"));
	TBuf16<17> buf2=_L16("A ");
	append16(buf2, NULL, _L16("Descriptor"));
	test(buf2==_L16("A Descriptor"));

	test.Printf(_L("Use a non-leaving overflow handler\n"));
	test.Next(_L("Force overflow with no conversions"));
	TDes16IgnoreOverflow overflow2;
	append16(buf2, &overflow2, _L16("12345678901234567"));
	test(buf2==_L16("A Descriptor12345"));

	test.Next(_L("Force overflow with decimal conversion"));
	buf2=_L16("A Descriptor");
	append16(buf2, &overflow2,  _L16("%d"), 123456789);
	test(buf2==_L16("A Descriptor"));

	test.End();
	}

void testAppendFormatIgnoreOverflow()
	{
	test.Start(_L("Test no overflow"));
	TBuf8<16> buf;
	buf.AppendFormat(_L8("A Descriptor"));	
	test(buf==_L8("A Descriptor"));
	
	test.Next(_L("Force overflow with no conversions"));
    TDes8IgnoreOverflow overflow;
	buf.AppendFormat(_L8("123456789012345679"),&overflow);
	test(buf==_L8("A Descriptor1234"));
	
	test.Next(_L("Force overflow with decimal conversion"));
	buf = _L8("Symbian OS");
	buf.AppendFormat(_L8("%d"), &overflow, 1234567);
	test(buf==_L8("Symbian OS"));
	
	test.Next(_L("Test no overflow"));
	TBuf16<16> buf2;
	buf2.AppendFormat(_L16("A Descriptor"));	
	test(buf2==_L16("A Descriptor"));
	
	test.Next(_L("Force overflow with no conversions"));
    TDes16IgnoreOverflow overflow2;
	buf2.AppendFormat(_L16("123456789012345679"),&overflow2);
	test(buf2==_L16("A Descriptor1234"));
	
	test.Next(_L("Force overflow with decimal conversion"));
	buf2 = _L16("Symbian OS");
	buf2.AppendFormat(_L16("%d"), &overflow2, 1234567);
	test(buf2==_L16("Symbian OS"));

	test.End();


	}

// INC061330  AV28Crit: NTT - TInt TDesC16.FindC -method is giving strange output values 
// This test should pass with "ELangPrcChinese" locale.
void INC061330()
	{
	TLanguage defaultLang = User::Language();

	TInt err = HAL::Set(HAL::ELanguageIndex, ELangPrcChinese);
	test(err == KErrNone);	

	TBuf<50> libraryName;

	libraryName.Format(_L("ELOCL.%02d"), ELangPrcChinese);

	//Reset the locale
	err=UserSvr::ChangeLocale(KNullDesC);
	test(err==KErrNone);

	//Now change to chinese locale
	err = UserSvr::ChangeLocale(libraryName);
	if(err==KErrNotFound)
		{
		test.Printf(_L("TEST CASE NOT RUN BECAUSE ELangPrcChinese LOCALE NOT FOUND!\n"));
		HAL::Set(HAL::ELanguageIndex, defaultLang);
		return;
		}
	test(err == KErrNone);
	
	TLanguage lang = User::Language();
	test(lang == ELangPrcChinese);

	TInt pos;
	_LIT(KBuf, "hello");

	pos = KBuf().FindC(_L("a"));
	test(pos == KErrNotFound);

	pos = KBuf().FindC(_L("zzz"));
	test(pos == KErrNotFound);

	pos = KBuf().FindC(_L("."));
	test(pos == KErrNotFound);

	pos = KBuf().FindC(_L(":"));
	test(pos == KErrNotFound);

	pos = KBuf().FindC(_L("hela"));
	test(pos == KErrNotFound);

	//Reset the locale
	err=UserSvr::ChangeLocale(KNullDesC);
	test(err==KErrNone);
	
	//Now revert to the original default english locale
	libraryName.Format(_L("ELOCL.%02d"), defaultLang);
	test(err == KErrNone);
	err = UserSvr::ChangeLocale(libraryName);
	test(err == KErrNone);
	
	lang = User::Language();
	test(lang == defaultLang);
	}

// Test the surrogate aware version functions of the class. 
GLDEF_C void SurrogateAware1()
    {
    test.Start(_L("Constructors"));
    TBuf16<0x50> a;
    TBuf16<0x50> b;
    TBuf16<0x50> c;
    TBuf16<0x50> d;
    
    a=_L("ABCD");
    b=_L("abcd");
    // Cannot define these on GCC (X86).
	#if !(defined(__GCC32__) && defined(__X86__))
    c=_L("‡·‚„‰ÂÁËÈÍÎÏÌÓÔÒÚÛÙıˆ˘˙˚¸˝");
    d=_L("¿¡¬√ƒ≈«»… ÀÃÕŒœ—“”‘’÷Ÿ⁄€‹›");
    #else
    c=_L("aaaaaaceeeeiiiinooooouuuuy");
    d=_L("AAAAAACEEEEIIIINOOOOOUUUUY");
    #endif
 
    test.Next(_L("Fill2"));
    TInt maxBufLength=a.MaxLength();
    a.Fill2(' ');
    a.Fill2(' ',maxBufLength);
    a.Fill2('z');
    a.Fill2('*',maxBufLength);
    a=c;
    b=d;
    a.Swap(b);
    test(a==d);

    test.Next(_L("Conversion 2"));
    a.Fold2();
    b.Collate2();
    a.UpperCase2();
    a.LowerCase2();
    a.Capitalize2();
    b.Capitalize2();
    
    test.Next(_L("Locating"));
    a=_L("ABCDabcd");
    test(a.Locate('A')==0);
    test(a.LocateF('b')==1);
    test(a.LocateReverse('A')==0);
    test(a.LocateReverse('a')==4);
    test(a.LocateReverse('b')==5);

    test.Next(_L("Copying"));
    a.Copy(b); // Copies a 16 bit descriptor
    a.Copy(_L("AB")); 
    a.Copy(b.Ptr(),3); // Copies aLength characters from the aBuf pointer
    a.CopyF2(c); // Copies and folds
    a.CopyF2(_L("AB"));
    a.CopyC2(d); // Copies and collates
    a.CopyC2(_L("AB"));
    a.CopyLC(d); // Copies and converts the text to lower case
    a.CopyLC(_L("AB"));
    a.CopyUC2(c); // Copies and converts the text to upper case
    a.CopyUC2(_L("AB"));
    a.CopyCP2(b); // Copies and capitalizes the text
    a.CopyCP2(_L("AB"));
    
    test.Next(_L("Finding"));
    a=_L("ABCDabcd");
    b=_L("bc");
    test(a.Find(b)==5);
    test(a.Find(_L("ab"))==4);
    test(a.FindF(b)==1);
    test(a.FindF(_L("ab"))==0);
    test(a.FindC(b)==1);
    test(a.FindC(_L("AB"))==0);
    test(a.FindC(b.Ptr(), b.Length(), 3)==1);
    test(a.FindC(b.Ptr(), b.Length(), 2)==1);
    test(a.FindC(b.Ptr(), b.Length(), 1)==1);
    test(a.FindC(b.Ptr(), b.Length(), 0)==1); 
    test(a.FindF(b.Ptr(), b.Length())==1);
    
    test.Next(_L("Formating"));
    TInt width = 10;
    a.Justify2(_L("AB"),width,ELeft,' ');
    a.Justify2(b,10,ELeft,' ');
    b.Fill2('A',2);
    a.Zero();
    a.AppendJustify2(_L("AB"),width,ELeft,' ');
    a.AppendJustify2(b,width,ELeft,' ');
    a=_L("ABCDE");
    b=_L("abcde");
    a.AppendJustify2(b,width,ELeft,'*');
    a.AppendJustify2(b.Ptr(),width,ELeft,'*');
    // Append and justify with explicit length
    TInt length = 5;
    a.AppendJustify2(b,length,KDefaultJustifyWidth,ELeft,'*');
    a.AppendJustify2(b.Ptr(),length,KDefaultJustifyWidth,ELeft,'*');
    
    TCollationMethod cm = *Mem::CollationMethodByIndex( 0 ); // default collation method
    cm.iFlags |= TCollationMethod::EIgnoreNone;
    TDesC::TPrefix prefix = a.HasPrefixC(a, 0, &cm);
    test(prefix==TDesC16::EIsPrefix);
    test.End();
    }

// Test the surrogate aware versions of conversion operators (test7) 
// 
void SurrogateAware7()
    {
    test.Start(_L("Fold2, Collate2 ..."));
    TBuf16<0x50> a;
    TBuf16<0x50> b;
    a=_L("abc AbC");
    b=_L("ABC ABC");
    a.Fold2();
    b.Fold2();
    test(a==b);
    a=_L("abc AbC");
    b=_L("ABC ABC");
    a.Collate2();
    b.Collate2();
    test(a==b);
    a.UpperCase2();
    test(a==_L("ABC ABC"));
    a.LowerCase2();
    test(a==_L("abc abc"));
    a.Capitalize2();
    test(a==_L("Abc abc"));
    test.End();
    }

#ifndef _DEBUG
#pragma warning( disable : 4702) //Unreachable code
#pragma warning( disable : 4710) //Function not expanded
#endif
GLDEF_C TInt E32Main()
// Test the TBuf type.
    {
	test.Title();

	test.Start(_L("class TBuf16<0x50>"));
	TestTBuf<TBuf16<0x50>,TText16,TPtrC16> c(0x50);
	c.test_TBuf();
	
	test.Next(_L("class TBuf8<0x50>"));
	TestTBuf<TBuf8<0x50>,TText8,TPtrC8> b(0x50);
	b.test_TBuf();
	
	test.Next(_L("class TBuf<0x50>"));
	TestTBuf<TBuf<0x50>,TText,TPtrC> a(0x50);
	a.test_TBuf();

	test.Next(_L("TReal formating"));
	testFormat();

	test.Next(_L("Test overflow handler"));
	testOverflow();

	test.Next(_L("Test ignore overflow handler"));
	testIgnoreOverflow();

	test.Next(_L("Test Format ignore overflow handler"));
	testAppendFormatIgnoreOverflow();

	test.Next(_L("INC061330"));
	INC061330();
	
	test.Next(_L("Surrogate aware version"));
	SurrogateAware1();
	    
	test.Next(_L("Surrogate aware version"));
	SurrogateAware7();

	test.End();

	return(KErrNone);
    }

//#pragma warning( default : 4702)
//#pragma warning( default : 4710)


