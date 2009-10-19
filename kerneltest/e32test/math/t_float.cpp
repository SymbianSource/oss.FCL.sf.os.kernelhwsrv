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
// e32test\math\t_float.cpp
// Data split into files: t_float1.cpp, t_float2.cpp and t_float3.cpp
// Overview:
// Test conversion of real to descriptors, descriptors to real and the 
// Math::Round method.
// API Information:
// TDes8, TDes16, TLex8, TLex16, Math.  
// Details:
// - Test conversion of specified TReal32, TReal64 into TBuf8, TBuf16 
// is as expected.
// - Round some numbers to a specified number of decimal places and check it 
// is as expected.
// - Test conversion of specified TReal32, TReal64 into TBuf8, TBuf16 in
// calculator mode is as expected.
// - Check that errors returned during conversion from TReal32, TReal64 into 
// TBuf8, TBuf16 are as expected.
// - Verify the result when a string is parsed to extract a real number.
// - Test string to real conversion in cases, which are not exact and check
// that results are as expected.
// - Test errors in string to real conversion are as expected.
// - Test that the conversion from string to real and then real to string, verify
// the results are as expected.
// - For specified strings values check parsing of string to extract real value, 
// absolute value, token length is as expected.
// - Check the character representation of specified real numbers are as expected.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include "t_math.h"
#include "t_float.h"

GLREF_D RtoB_TEST testd[];
GLREF_D TUint size_testd;
GLREF_D RtoB_TEST testcalc[];
GLREF_D TUint size_testcalc;
GLREF_D ERR_TEST testerr[];
GLREF_D TUint size_testerr;
GLREF_D DtoR_ERR_TEST testerr2[];
GLREF_D TUint size_testerr2;
GLREF_D DtoR_ERR_TEST testerr3[];
GLREF_D TUint size_testerr3;
GLREF_D CALC_TEST calctest[];
GLREF_D TUint size_calctest;
GLREF_D ROUND_TEST testround[];
GLREF_D TUint size_testround;
GLREF_D DtoR_TEST64 testd2[];
GLREF_D TUint size_testd2;
GLREF_D DtoR_TEST64 testApprox[];
GLREF_D TUint size_testApprox;
GLREF_D DtoR_TEST32 testd32[];
GLREF_D TUint size_testd32;

GLDEF_D RTest test(_L("T_FLOAT"));

LOCAL_C TInt rtob(RtoB_TEST& tst)
    {

    TBuf<0x100> buf;

 	TInt ret=buf.Num(tst.num,tst.format);
	if (ret!=KErrGeneral)
		{
		buf.ZeroTerminate();
		TPtrC result(tst.res);
//		test.Printf(_L("buf=%S\nres=%S\n"),&buf,&result);
		test(buf.Compare(result)==0);
		}
    return(ret);
    }

LOCAL_C void test1()
// 
// Test RtoB using selected numbers
//
    {

  	for (TInt jj=0;jj<TInt(size_testd/sizeof(RtoB_TEST));jj++)
        test(rtob(testd[jj])>=0);
	}

LOCAL_C void test2()
//
// Test Math::Round(_,_,_) for specific numbers
//
    {

    TReal res;
    TRealFormat pf;
	pf.iType=KRealFormatExponent;
	pf.iWidth=22;
	pf.iPlaces=15;
	pf.iPoint='.';
	pf.iTriad=',';
	pf.iTriLen=1;
					
    for (TInt jj=0;jj<TInt(size_testround/sizeof(ROUND_TEST));jj++) 
        {
        test(Math::Round(res,testround[jj].num,testround[jj].plcs)==KErrNone);
        TReal err=(res-testround[jj].res);
        if (res)
            err/=res;
        test(Abs(err)<=1E-15);
        }
    }

LOCAL_C void test3()
// 
// Test RtoB in calculator mode using selected numbers
// Added by AnnW, November 1996
//
    {

  	for (TInt jj=0;jj<TInt(size_testcalc/sizeof(RtoB_TEST));jj++)
        test(rtob(testcalc[jj])>=0);
    }

LOCAL_C void test4()
//
// Tests errors
//
    {

    for (TInt jj=0;jj<TInt(size_testerr/sizeof(ERR_TEST));jj++)
        {
        RtoB_TEST tst=testerr[jj].rtob;
		TInt ret=rtob(tst);
		test(testerr[jj].err==ret);
        }
    }

// End of translated test code from plib.  Now some tests from string o real conversions:

LOCAL_C void test5()
//
// Test string to real conversion with selected numbers
// added by AnnW, December 1996
//
	{
	
	TLex l;
	TReal64 r64;
	TInt jj;
	
	for (jj=0; jj<TInt(size_testd2/sizeof(DtoR_TEST64)); jj++)
		{
		const DtoR_TEST64* tst64=&testd2[jj];
		l=tst64->iLex;
		test(l.Val(r64)==KErrNone);
		test(tst64->iRes==r64);
		}
	
	TReal32 r32;

	for (jj=0; jj<TInt(size_testd32/sizeof(DtoR_TEST32)); jj++)
		{
		const DtoR_TEST32* tst32=&testd32[jj];
		l=tst32->iLex;
		test(l.Val(r32)==KErrNone);
		test(tst32->iRes==r32);
		}
	}	

LOCAL_C void test6()
//
// Test string to real conversion in cases which are not exact
// Added by AnnW, December 1996
//
	{
	
	TReal r64;
	
	for (TInt jj=0; jj<TInt(size_testApprox/sizeof(DtoR_TEST64)); jj++)
		{
		const DtoR_TEST64* tst64=&testApprox[jj];
		TLex l=tst64->iLex;
		test(l.Val(r64)==KErrNone);
		test(tst64->iRes==r64);
		}
	}

LOCAL_C void test7()
//
// Test errors in string to real conversion
// added by AnnW, December 1996
//
	{

	TReal r64;
	TInt jj;

	for (jj=0; jj<TInt(size_testerr2/sizeof(DtoR_ERR_TEST)); jj++)
		{
		const DtoR_ERR_TEST* errtst64=&testerr2[jj];
		TLex l=errtst64->iLex;
		TInt err=errtst64->iErr;
		test(l.Val(r64)==err);
		}

	TReal32 r32;

	for (jj=0; jj<TInt(size_testerr3/sizeof(DtoR_ERR_TEST)); jj++)
		{
		const DtoR_ERR_TEST* errtst32=&testerr3[jj];
		TLex l=errtst32->iLex;
		TInt err=errtst32->iErr;
		test(l.Val(r32)==err);
		}
	}

LOCAL_C void test8()
//
// Some tests for calc to check that a number entered is printed to same significance when
// returned
// do dtor and rtod test
//
	{

	TLex l;
	TBuf<0x100> buf;
	TInt ret;
	TReal64 num;

	for (TInt jj=0;jj<TInt(size_calctest/sizeof(CALC_TEST));jj++)
		{
		// string to real
		const CALC_TEST tst=calctest[jj];
		l=tst.iLex;
		test(l.Val(num)==KErrNone);
		
		// real to string
 		ret=buf.Num(num,tst.iFormat);
		if (ret!=KErrGeneral)
			{
			buf.ZeroTerminate();
			test(buf.Compare(TPtrC(tst.iRes))==0);
			}
		test(ret>=0);    
		}
	}

// Tag on a few extra tests relating to ascii and unicode

template<class R,class S,class L,class B,class T> 		
class AsciiUnicode
	{
public:
	void Test1();
	};

template<class R, class S,class L,class B,class T>
void AsciiUnicode<R,S,L,B,T>::Test1()
// R == TReal32 or TReal64
// S == TText, TText8 or TText16
// L == TLex, TLex8 or TLex16
// B == TBuf, TBuf8 or TBuf16
// T == TPtr, TPtr8 or TPtr16
	{

	L lex1(_TL("123.456"));
	R real; // TReal32 or TReal64 
	test(lex1.Val(real)==KErrNone);							 
	test(Abs(real-123.456)<1E-5);
	test(lex1.TokenLength()==7);
	test((lex1.Remainder()).Compare(T(_TL(""),0,0))==0);
	test(lex1.Val(real)==KErrGeneral);

	L lex2(_TL("123.456abc"));
	test(lex2.Val(real)==KErrNone);
	test(Abs(real-123.456)<1E-5);
	test(lex2.TokenLength()==7);
	test((lex2.Remainder()).Compare(T(_TL("abc"),3,3))==0);
	test(lex2.Val(real)==KErrGeneral);

	L lex3;
	real=(R)0.5;
	test(lex3.Val(real)==KErrGeneral);
	test(real==((R)0.5));
	test(lex3.TokenLength()==0);

	L lex4(_TL("abc123.45"));
	real=(R)0.5;
	test(lex4.Val(real)==KErrGeneral);
	test(real==((R)0.5));

	L Lex5(_TL("1.2e37"));
	real=(R)0.5;
	test(Lex5.Val(real)==KErrNone);
	
	L Lex6(_TL("1.2e"));
	real=(R)0.5;
	test(Lex6.Val(real)==KErrNone);

	TRealFormat format(20,3); // first param width, 2nd decimals
	real=(R)12345.6789;
	B buf1;
	test(buf1.Num(real,format)==10);
	test(buf1.Compare(T(_TL("12,345.679"),10,10))==0);

	B buf2(_TL("abc"));
	test(buf2.AppendNum(real,format)==13);
	test(buf2.Compare(T(_TL("abc12,345.679"),13,13))==0); 

	B buf3;
	test(buf3.AppendNum(real,format)==10);
	test(buf3.Compare(T(_TL("12,345.679"),10,10))==0);
	}

#ifndef _DEBUG
#pragma warning (disable :4710) //Function not expanded
#pragma warning (disable :4702) //Unreachable code
#endif

GLDEF_C TInt E32Main()
    {
	test.Title();
	test.Start(_L("Test ftol"));
    TReal f=34.567;
    TInt i=TInt(f);
    test(i==34);
	test.Next(_L("Test TBuf8/16::Num() using selected numbers"));
	test1();
	test.Next(_L("Test specific numbers for Math::Round(_,_,_)"));
	test2();
	test.Next(_L("Test TBuf8/16::Num() using selected numbers in calculator mode"));
	test3();
	test.Next(_L("Test errors"));
	test4();
	test.Next(_L("Test TLex8/16::Val using selected values which give exact results"));
	test5();
	test.Next(_L("Test TLex8/16::Val using values which do NOT give exact results"));
	test6();
	test.Next(_L("Test errors"));
	test7();
	test.Next(_L("Test for CALC"));
	test8();
	//
	test.Next(_L("Ascii/Unicode: TText"));
	AsciiUnicode<TReal64,TText,TLex,TBuf<0x20>,TPtr> T1;
	T1.Test1();
	test.Next(_L("Ascii/Unicode: TText8"));
	AsciiUnicode<TReal32,TText8,TLex8,TBuf8<0x20>,TPtr8> T2;
	T2.Test1();
	test.Next(_L("Ascii/Unicode: TText16"));
	AsciiUnicode<TReal32,TText16,TLex16,TBuf16<0x20>,TPtr16> T3;
	T3.Test1();											
	//
	test.End();
	return(KErrNone);
	}
#pragma warning (default :4710)
#pragma warning (default :4702)

