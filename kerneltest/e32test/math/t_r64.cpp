// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\math\t_r64.cpp
// T_R64.CPP - Test routines for TReal64
// Also note that these tests do not generally include testing of special values.  This is done
// in T_R96 tests and conversions are tested thoroughly, so explicit tests are unnecessary here.
// 
//

#include "t_math.h"
#include "t_vals.h"
#include "t_real64.h"

// Data for tests from T_R64DTA.cpp 
GLREF_D TReal64 addInput[];
GLREF_D TReal64 subInput[];
GLREF_D TReal64 multInput[];
GLREF_D TReal64 divInput[];
GLREF_D TReal64 divDenormalInput[];
GLREF_D TReal64 unaryInput[];
GLREF_D TReal64 incDecInput[];
GLREF_D TInt sizeAdd;
GLREF_D TInt sizeSub;
GLREF_D TInt sizeMult;
GLREF_D TInt sizeDiv;
GLREF_D TInt sizeDivDenormal;
GLREF_D TInt sizeUnary;
GLREF_D TInt sizeIncDec;

#if defined (__WINS__) || defined (__X86__)
// Functions from EMGCC64.CPP
GLREF_C TReal64 __adddf3(TReal64 a1,TReal64 a2);
GLREF_C TReal64 __subdf3(TReal64 a1,TReal64 a2);
GLREF_C TReal64 __muldf3(TReal64 a1,TReal64 a2);
GLREF_C TReal64 __divdf3(TReal64 a1,TReal64 a2);
#endif

// Special numbers for arithmetic tests
GLDEF_D TReal64 endsInFour;
GLDEF_D TReal64 arg1;
GLDEF_D TReal64 arg2;
GLDEF_D TReal64 arg3;
GLDEF_D const TReal64 KMinDenormalTReal64=5E-324;
GLDEF_D const TReal KNaNTReal=TReal(KNaNTReal64);
GLDEF_D const TReal KPosInfTReal=TReal(KPosInfTReal64);;
GLDEF_D const TReal KNegInfTReal=TReal(KNegInfTReal64);;
GLDEF_D const TReal KMinDenormalTReal=5E-324;

enum TOrder
	{
	ELessThan,
	EEqual,
	EGreaterThan
	};

LOCAL_D RTest test(_L("T_R64"));	

#if defined (__WINS__) || defined (__X86__)

LOCAL_C void testConstants()
//
// Checks that constants are defined as expected in WINS, in case low and high words are swapped
//
	{

	test(TRealX(TReal32(KMinTReal32in64))==TRealX(KMinTReal32));
	test(TRealX(TReal32(KMaxTReal32in64))==TRealX(KMaxTReal32));
	}

#endif

LOCAL_C void initSpecialValues()
//
// Initialise special values, etc
//
	{

	SReal64* p64=(SReal64*)&endsInFour;
	p64->sign=0;
	p64->exp=1020;
	p64->msm=0;
	p64->lsm=0x4;

	p64=(SReal64*)&arg1;
	p64->sign=0;
	p64->exp=1075;
	p64->msm=0x20000;
	p64->lsm=0;

	p64=(SReal64*)&arg2;
	p64->sign=0;
	p64->exp=1075;
	p64->msm=0;
	p64->lsm=0x2a;

	p64=(SReal64*)&arg3;
	p64->sign=0;
	p64->exp=970;
	p64->msm=0xc71c7;
	p64->lsm=0x1c71c71c;
	}
	
LOCAL_C void testConvert()
//
// Test conversion from TReal to TReal64
// N.B. Conversions between TRealX and TReal64 are tested by t_r96.cpp 
//
	{

	const TReal input[]=
		{
		KMaxTReal,KMinTReal,-KMaxTReal,-KMinTReal,
		1.7976931348622E+308,-1.7976931348622E+308,2.2250738585073E-308,-2.2250738585073E-308,
		KMaxTReal32inTReal,KMinTReal32inTReal,-KMaxTReal32inTReal,-KMinTReal32inTReal,
		0.0,64.5,-64.5,1.54E+180,-1.54E+180,4.72E-225,-4.72E-225,
		KNegZeroTReal,KPosInfTReal,KNegInfTReal,KMinDenormalTReal,
		1.2345E-318,-2.4682E-321,1.0E-323,-2.0E-308
		};

	const TReal64 expect[]=
		{
		KMaxTReal64,KMinTReal64,-KMaxTReal64,-KMinTReal64,
		1.7976931348622E+308,-1.7976931348622E+308,2.2250738585073E-308,-2.2250738585073E-308,
		KMaxTReal32in64,KMinTReal32in64,-KMaxTReal32in64,-KMinTReal32in64,			
		0.0,64.5,-64.5,1.54E+180,-1.54E+180,4.72E-225,-4.72E-225,
		KNegZeroTReal64,KPosInfTReal64,KNegInfTReal64,KMinDenormalTReal64,
		1.2345E-318,-2.4682E-321,1.0E-323,-2.0E-308
		};		  
	
	TInt size = sizeof(input)/sizeof(TReal);
	TRealX f;

	for (TInt ii=0; ii<size; ii++)
		{
		f=TRealX(TReal64(TRealX(input[ii])));
		test(f==TRealX(expect[ii]));
		}

	// NaN
	TReal a=KNaNTReal;
//	TReal64 b=KNaNTReal64;
	f=TRealX(TReal64(TRealX(a)));
//	test(f!=TRealX(b));
	test(f.IsNaN());
	}

LOCAL_C void testAdd()
//
//	Addition tests
//
	{
	TReal64 f,g,h,y;
	TRealX ff,gg,hh;
	
	for (TInt ii=0; ii<sizeAdd-1; ii++)
		{
		f=addInput[ii];
		g=addInput[ii+1];
		ff=TRealX(f);
		gg=TRealX(g);
		// Test commute
		test(f+g == g+f);
		// Test PC real addition using fp-hardware same as TRealX addition
		h=f+g;
		hh=ff+gg;
		y=(TReal64)hh;	// need to do this since TRealX has too much precision
		test(y==h);
		h=g+f;
		hh=gg+ff;
		y=(TReal64)hh;	// need to do this since TRealX has too much precision
		test(y==h);
		// Test hex-encoded constants for TReal64s generated on PC using fp-hardware same as 
		// TRealX addition
		test(*(TReal64*)&addArray[ii]==f+g);
		test(*(TReal64*)&addArray[ii]==g+f);
		// similarly to tests above ...
		h=g;
		hh=gg;
		h+=f;
		hh+=ff;
		test(h==(TReal64)hh);
		test(*(TReal64*)&addArray[ii]==h);
		//
		h=f;
		hh=ff;
		h+=g;
		hh+=gg;
		test(h==(TReal64)hh);
		test(*(TReal64*)&addArray[ii]==h);
		}
	}

LOCAL_C void testSub()
//
// Subtraction tests - see notes in addition test above
//
	{
	TReal64 f,g,h;
	TRealX ff,gg,hh;
	
	for (TInt ii=0; ii<sizeSub-1; ii++)
		{
		f=subInput[ii];
		g=subInput[ii+1];
		ff=TRealX(f);
		gg=TRealX(g);
		//
		test(f-g == -(g-f));
		//
		test(TRealX(f-g)==ff-gg);
		test(TRealX(g-f)==gg-ff);
		test(*(TReal64*)&subArray[ii]==f-g);
		test(*(TReal64*)&subArray[ii]==-(g-f));
		//
		h=g;
		hh=gg;
		test(TRealX(h-=f)==(hh-=ff));
		test(TRealX(h)==hh);
		test(*(TReal64*)&subArray[ii]==-h);
		//
		h=f;
		hh=ff;
		test(TRealX(h-=g)==(hh-=gg));
		test(TRealX(h)==hh);
		test(*(TReal64*)&subArray[ii]==h);
		}
	}

LOCAL_C void testMult()
//
//	Multiplication test
//
	{
	TReal64 f,g,h;
	TRealX ff,gg,hh;
	
	for (TInt ii=0; ii<sizeMult-1; ii++)
		{
		f=multInput[ii];
		g=multInput[ii+1];
		ff=TRealX(f);
		gg=TRealX(g);
		//
		test(f*g == g*f);
		//
		test(TRealX(f*g)==ff*gg);
		test(TRealX(g*f)==gg*ff);
		test(*(TReal64*)&multArray[ii]==f*g);
		test(*(TReal64*)&multArray[ii]==g*f);
		//
		h=f;		
		hh=ff;
		test(TRealX(h*=g)==(hh*=gg));
		test(TRealX(h)==hh);
		test(*(TReal64*)&multArray[ii]==h);
		//
		h=g;
		hh=gg;
		test(TRealX(h*=f)==(hh*=ff));
		test(TRealX(h)==hh);
		test(*(TReal64*)&multArray[ii]==h);
		}
	}

LOCAL_C void testDiv()
//
//	Division tests
//
	{
	TReal64 f,g,h;
	TRealX ff,gg,hh;
	TInt count=0;
	
	// Panic (under ARM) - Divide by Zero - run in DEBUG build as a check only
	//f=1.0;
	//g=0.0;
	//f/=g;

	for (TInt ii=0; ii<sizeDiv-1; ii++)
		{
		f=divInput[ii];
		g=divInput[ii+1];
		ff=TRealX(f);
		gg=TRealX(g);
		if (g!=0.0)
			{
			test(TRealX(f/g)==ff/gg);
			test(*(TReal64*)&divArray[count]==f/g);
			//
			h=f;
			hh=ff;
			test(TRealX(h/=g)==(hh/=gg));
			test(TRealX(h)==hh);
			test(*(TReal64*)&divArray[count]==h);
			++count;
			}							   
		if (f!=0.0)
			{
			test(TRealX(g/f)==gg/ff);
			//
			h=g;
			hh=gg;
			test(TRealX(h/=f)==(hh/=ff));
			test(TRealX(h)==hh);
			}
		}

	gg=TRealX(arg2)/TRealX(arg3);

	//Additional test
	f=3.999999999999999;
	g=KMinTReal64;
	ff=TRealX(f);
	gg=TRealX(g);
	test(TRealX(f/g)==ff/gg);
	h=f;
	test(TRealX(h/=g)==ff/gg);
	test(TRealX(h)==ff/gg);
	}

#if defined (__WINS__) || defined (__X86__)

LOCAL_C	 void testArithmeticExceptionRaising()
//
// Test that UP_GCC.CPP raise exceptions correctly by calling functions from EMGCC64.CPP which
// are copies of those in UP_GCC.CPP.  To be used in debugger only.
// Added by AnnW, December 1996
//
	{
	TReal64 f,g,h;

	// Addition - possible errors are argument, overflow, or none
	// NB cannot achieve underflow now denormals in use

	f=KNaNTReal64;
	h=__adddf3(f,f);	// argument

	f=KMaxTReal64;
	h=__adddf3(f,f);	// overflow
	
	f=1.0;
	g=2.0;
	h=__adddf3(f,g);	// none
	test(h==3.0);

	// Subtraction - possible errors are argumnet, overflow or none
	// NB cannot achieve underflow now denormals in use

	f=KNaNTReal64;
	h=__subdf3(f,f);	// argument

	f=KMaxTReal64;
	g=-KMaxTReal64;
	h=__subdf3(f,g);	// overflow

	f=1.0;
	g=2.0;
	h=__subdf3(f,g);	// none
	test(h==-1.0);

	// Multiplication - possible errors are argument, overflow, underflow or none

	f=KNaNTReal64;
	h=__muldf3(f,f);	// argument

	f=KMaxTReal64;
	g=2.0;
	h=__muldf3(f,g);	// overflow

	f=KMinDenormalTReal64;
	g=0.1;
	h=__muldf3(f,g);	// underflow

	f=1.0;
	g=2.0;
	h=__muldf3(f,g);	// none
	test(h==2.0);

	// Division - possible errors are overflow, underflow, divide by zero, argument or none

	f=KMaxTReal64;
	g=0.5;
	h=__divdf3(f,g);	// overflow

	f=KMinDenormalTReal64;
	g=10.0;
	h=__divdf3(f,g);	// underflow

	f=4.0;
	g=0.0;
	h=__divdf3(f,g);	// divide by zero

	f=0.0;
	g=0.0;
	h=__divdf3(f,g);	// argument

	f=1.0;
	g=2.0;
	h=__divdf3(f,g);	// none
	test(h==0.5);
	}

#endif

LOCAL_C void testUnary()
//
//	Unary operator tests
//
	{
	TReal64 f;
	TRealX g;

	for (TInt ii=0; ii<sizeUnary-1; ii++)
		{
		f=unaryInput[ii];
		g=TRealX(f);
		test(TRealX(-f)==-g);
		test(TRealX(-f)==TRealX(0-f));
		test(TRealX(+f)==g);
		test(TRealX(+f)==TRealX(0+f));
		test(*(TReal64*)&unaryArray[ii]==-f);
		}
	}

LOCAL_C void testEqualities(const TReal& aA, TOrder aOrder, const TReal& aB)
//
//	Test equality/inequality functions on aA and aB
//	aOrder specifies the operand's relative sizes
//
	{
	//	Tautologies
	test((aA>aA) ==FALSE);
	test((aA<aA) ==FALSE);
	test((aA>=aA)==TRUE);
	test((aA<=aA)==TRUE);
	test((aA==aA)==TRUE);
	test((aA!=aA)==FALSE);

	if (aOrder!=EEqual)
		{
		test((aA==aB)==FALSE);
  		test((aA!=aB)==TRUE);
		}

	if (aOrder==ELessThan)
		{
		test((aA<aB) ==TRUE);
		test((aA<=aB)==TRUE);
		test((aA>aB) ==FALSE);
		test((aA>=aB)==FALSE);
		}

	if (aOrder==EEqual)
		{
		test((aA==aB)==TRUE);
		test((aA!=aB)==FALSE);
		test((aA>=aB)==TRUE);
		test((aA<=aB)==TRUE);
		test((aA>aB)==FALSE);
		test((aA<aB)==FALSE);
		}

	if (aOrder==EGreaterThan)
		{
		test((aA>aB) ==TRUE);
		test((aA>=aB)==TRUE);
		test((aA<aB) ==FALSE);
		test((aA<=aB)==FALSE);
		}
	}
	 
LOCAL_C void testEqualities()
//
//	Test >, <, >=, <=, ==, !=
//
	{
	TInt i, size;
	TReal64 lessThanMax = KMaxTReal64-TReal64(1.0E+294);
	TReal64 greaterThanMin = TReal64(2.225075E-308);
	TReal64 zero(0.0);
	
	TReal64 positive[] =
	{KMinTReal64,5.3824705392348592E-138,1.0,2387501,5.3824705392348592E+138,KMaxTReal64};

	TReal64 large[] =
	{2.0,KMaxTReal64,-lessThanMax,greaterThanMin,-KMinTReal64,10.40584821945060,-10.40584821945058,
	1.244334567201E+105,1.244334567201E+105,-1.3420344230402E-106,132435.97865,5.0E-16,9.6,-8.0}; 
	
	TReal64 small[] =
	{1.0,lessThanMax,-KMaxTReal64,KMinTReal64,-greaterThanMin,10.40584821945058,-10.40584821945060,
	50E-100,1.244334567201E+104,-5.03824705392348592E+58,-132435.97865,-5.1E-16,8.0,-9.6};
	
	TReal64 equal[] =							  // Same as large[]
	{2.0,KMaxTReal64,-lessThanMax,greaterThanMin,-KMinTReal64,10.40584821945060,-10.40584821945058,
	1.244334567201E+105,1.244334567201E+105,-1.3420344230402E-106,132435.97865,5.0E-16,9.6,-8.0}; 

	// Tests with zero

	size = sizeof(positive)/sizeof(TReal64);
	
	test.Start(_L("Zero"));
	testEqualities(zero, EEqual, zero);
	for (i=0; i<size; i++)
		{
		testEqualities(positive[i], EGreaterThan, zero);
		testEqualities(-positive[i], ELessThan, zero);
		testEqualities(zero, ELessThan, positive[i]);
		testEqualities(zero, EGreaterThan, -positive[i]);
		}

	// Test boundary and other numbers
	
	size = sizeof(large)/sizeof(TReal64);
	
	test.Next(_L("Nonzero"));
	for (i=0; i<size; i++)
		{
		testEqualities(large[i], EGreaterThan, small[i]);
		testEqualities(small[i], ELessThan, large[i]);
		testEqualities(large[i], EEqual, equal[i]);
		}

	test.End();
	}

LOCAL_C void testIncDec()
//
//	Test Pre/Post-increment/decrement
//
	{
	TInt ii;
	TReal64 f;
	TRealX g;

	test.Start(_L("Pre-increment"));
	
	for (ii=0; ii<sizeIncDec; ii++)
		{
		f=incDecInput[ii];
		g=TRealX(f);
		test(TRealX(f)==g);
		test(TRealX(++f)==(++g));
		test(*(TReal64*)&preIncArray1[ii]==f);
		test(TRealX(f)==g);
		test(TRealX(++f)==(++g));
		test(*(TReal64*)&preIncArray2[ii]==f);
		test(TRealX(f)==g);
		}
	
	test.Next(_L("Post-increment"));

	for (ii=0; ii<sizeIncDec; ii++)
		{
		f=incDecInput[ii];
		g=TRealX(f);
		test(TRealX(f)==g);
		test(TRealX(f++)==(g++));
		test(*(TReal64*)&postIncArray1[ii]==f);
		test(TRealX(f)==g);
		test(TRealX(f++)==(g++));
		test(*(TReal64*)&postIncArray2[ii]==f);
		test(TRealX(f)==g);
		}
	
	test.Next(_L("Pre-decrement"));

	for (ii=0; ii<sizeIncDec; ii++)
		{
		f=incDecInput[ii];
		g=TRealX(f);
		test(TRealX(f)==g);
		test(TRealX(--f)==(--g));
		test(*(TReal64*)&preDecArray1[ii]==f);
		test(TRealX(f)==g);
		test(TRealX(--f)==(--g));
		test(*(TReal64*)&preDecArray2[ii]==f);
		test(TRealX(f)==g);
		}
	
	test.Next(_L("Post-decrement"));

	for	(ii=0; ii<sizeIncDec; ii++)
		{
		f=incDecInput[ii];
		g=TRealX(f);
		test(TRealX(f)==g);
		test(TRealX(f--)==(g--));
		test(*(TReal64*)&postDecArray1[ii]==f);
		test(TRealX(f)==g);
		test(TRealX(f--)==(g--));
		test(*(TReal64*)&postDecArray2[ii]==f);
		test(TRealX(f)==g);
		}
	test.End();
	}

LOCAL_C void _matherr(TExcType aType)
//
// Dummy function to handle exceptions
//
	{

	test.Printf(_L("_matherr: Exception type %u handled\n"),TUint(aType));
	}


GLDEF_C TInt E32Main()
//
//	Test TReal64
//
    {	  

	test.Title();

#if defined (__X86__)
	TInt16 cw=0;
	_asm fstcw cw;
	test.Printf(_L("control word = 0x%x\n"),cw);
	cw=0x27f;	// WINS value
	_asm fldcw cw;
#endif

	// Set exceptions to be handled
	RThread myThread;
	myThread.SetExceptionHandler(_matherr,KExceptionFpe);

	initSpecialValues();

#if defined (__WINS__) || defined (__X86__)
	test.Start(_L("Checking double words not swapped..."));
	testConstants();
	test.Next(_L("Conversion from TReal to TReal64"));
	testConvert();
#else
	test.Start(_L("Conversion from TReal to TReal64"));
	testConvert();
#endif
	test.Next(_L("Conversion from TReal to TReal64"));
	testConvert();
	test.Next(_L("Addition"));
	testAdd();
	test.Next(_L("Subtraction"));
	testSub();
	test.Next(_L("Multiplication"));
	testMult();
	test.Next(_L("Division"));
	testDiv();
#if defined (__WINS__) || defined (__X86__)
	test.Next(_L("Arithmetic which emulates UP_GCC and raises an exception"));
	testArithmeticExceptionRaising();
#endif
	test.Next(_L("Unary Operations"));
	testUnary();
	test.Next(_L("Equalities and Inequalities"));
	testEqualities();
	test.Next(_L("Increment and Decrement"));
	testIncDec();

	test.End();
	return(KErrNone);
    }


