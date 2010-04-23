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
// e32test\math\t_i64.cpp
// Tests TInt64
// Overview:
// Test 64-bit integer functionality.
// API Information:
// TInt64.
// Details:
// - Construct TInt64 with specified range of integer, real, high
// and low values and check constructor, copy constructor are as expected.
// - Test all the operators for range of values and check it is as expected.
// - Check the logical shift of specified number of bits is as expected.
// - Check multiplication of 64 bit integer by the specified 64 bit integer 
// using MulTop, fast multiplication of 64 bit integer by 10.
// - Verify the 64 bit integer divide and mod results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32math.h>

inline TInt __i64multop(TInt64& aX, TInt64& aValue)
			{	Uint64 __lowResult; \
				Uint64 __highResult; \
				Math::UMul64(aX, aValue, __highResult, __lowResult); \
				aX = static_cast<TInt64>(__highResult); \
				return (__lowResult == UI64LIT(0x0000000000000000)) ?	-2 : \
				(__lowResult < UI64LIT(0x8000000000000000)) ?	-1 : \
				(__lowResult == UI64LIT(0x8000000000000000)) ?	0  : \
				/*__lowResult > UI64LIT(0x8000000000000000)*/	1; \
			}

#define	I64MULTOP(x, value) __i64multop(x, (value))


TInt HexMulAdd(TUint8 a1,TUint8 a2,TUint8& answer,TUint8& carry)
	{
	TUint x1= a1>'9' ? a1-'a'+10 : a1-'0';
	TUint x2= a2>'9' ? a2-'a'+10 : a2-'0';
	TUint a= answer>'9' ? answer-'a'+10 : answer-'0';
	TUint c= carry>'9' ? carry-'a'+10 : carry-'0';
	if (x1>15) return (KErrArgument);
	if (x2>15) return (KErrArgument);
	if (a>15) return (KErrArgument);
	if (c>15) return (KErrArgument);
	a+=(x1*x2)+c;
	c=a/16;
	a=a%16;
	a= a>9 ? a-10+'a' : a+'0';
	c= c>9 ? c-10+'a' : c+'0';
	answer=(TUint8)a;
	carry=(TUint8)c;
	return(KErrNone);
	}

TInt HexMul(TDesC8& a1,TDesC8& a2,TDes8& a3)
//
// Infinite precision hex multiplier
//
	{
	TInt l1=a1.Length();
	TInt l2=a2.Length();
	TInt l3=l1+l2;
	if (a3.MaxLength()<l3)
		return(KErrArgument);
	a3.Zero();
	TInt x;
	TInt y;
	TInt z;
	for (x=0;x<l3;x++)
		a3.Append('0');
	for (y=0;y<l2;y++)
		{
		TUint8 carry='0';
		for (z=0;z<l1;z++)
			{
			if(HexMulAdd(a2[l2-y-1],a1[l1-z-1],a3[l3-y-z-1],carry)!=KErrNone)
				return(KErrArgument);
			}
		if(HexMulAdd('0','0',a3[l3-y-l1-1],carry)!=KErrNone)
			return(KErrArgument);
		}
	return(KErrNone);
	}


LOCAL_D RTest test(_L("T_I64"));

LOCAL_C void SlowDivMod(const TInt64& aA, const TInt64& aB, TInt64& aDiv, TInt64& aMod)
//
//	Calculate Division/Remainder using repeated subtraction
//
	{
		TInt negative=0;
		TInt64 t=0;

		if (aA<0)
			{
			negative=1;
			aMod=-aA;
			}
		else
			{
			aMod=aA;
			}
		if (aB<0)
			{
			if (negative)
				negative=0;
			else
				negative=1;
			t=-aB;
			}
		else
			{
			t=aB;
			}
		aDiv=0;

		if ((t & UI64LIT(0xffffffff00000000)) == 0)
			{
			while (aMod >= (t << 31))
				{
				aDiv += static_cast<TUint32>(1U << 31);
				aMod -= t << 31;
				}
			}
		if ((t & UI64LIT(0xffffff0000000000)) == 0)
			{
			while (aMod >= (t << 23))
				{
				aDiv += 1 << 23;
				aMod -= t << 23;
				}
			}
		if ((t & UI64LIT(0xffff000000000000)) == 0)
			{
			while (aMod >= (t << 15))
				{
				aDiv += 1 << 15;
				aMod -= t << 15;
				}
			}
		if ((t & UI64LIT(0xff00000000000000)) == 0)
			{
			while (aMod >= (t << 7))
				{
				aDiv += 1 << 7;
				aMod -= t << 7;
				}
			}
		if ((t & UI64LIT(0xf000000000000000)) == 0)
			{
			while (aMod >= (t << 3))
				{
				aDiv += 1 << 3;
				aMod -= t << 3;
				}
			}
		while (aMod >= t)
			{
			++aDiv;
			aMod -= t;
			}

		if (negative)
			{
			aDiv=-aDiv;
			}

		if (aA < 0)
			{
			aMod =- aMod;	
			}
	}

LOCAL_C void DivModTest(const TInt64& aA, const TInt64& aB)
//
//	Test DivMod against SlowDivMod
//
	{

	if (aB!=0)
		{
		TInt64 n(aA),d(aB);
		TInt64 div=0,mod=0,res=0;
		
		SlowDivMod(n,d,div,mod);
		
		res = n % d;
		n /= d;
		
		test(n==div);
		test(res==mod);
		}
	}


LOCAL_C void Test1()
	{
	// Test the constructors

	// TInt64()
	test.Start(_L("Default constructor"));
	TInt64 t1;
	t1 = 0; // to prevent uninitialised warnings
	(void)(t1 > 0); // to prevent unused warnings
	
	// TInt64(TInt aVal)
	test.Next(_L("TInt64(TInt aVal)"));
	TInt64 t2(0);
	test(I64LOW(t2)==0 && I64HIGH(t2)==0);
	TInt64 t3(1);  	 
	test(I64LOW(t3)==1 && I64HIGH(t3)==0);
	TInt64 t4(KMaxTInt32);
	test(I64LOW(t4)==(TUint)KMaxTInt32 && I64HIGH(t4)==0);
	TInt64 t5(-1);
	test(I64INT(t5)==-1);
	test(I64LOW(t5)==KMaxTUint32 && I64HIGH(t5)==KMaxTUint32);
	TInt64 t6(KMinTInt32); 	
	test(I64INT(t6)==KMinTInt32);

	// TInt64(TUint aVal)
	test.Next(_L("TInt64(TUint aVal)"));
	TInt64 t7((TUint)0);
	test(I64LOW(t7)==0 && I64HIGH(t7)==0);
	TInt64 t8((TUint)1);  	 
	test(I64LOW(t8)==1 && I64HIGH(t8)==0);
	TInt64 t9(KMaxTUint32);
	test(I64LOW(t9)==KMaxTUint32 && I64HIGH(t9)==0);

	// TInt64(TUint aHigh,TUint aLow)
	test.Next(_L("TInt64(TUint aHigh,TUint aLow)"));
	TInt64 t10 = MAKE_TINT64(0,0);
	test(I64LOW(t10)==0 && I64HIGH(t10)==0);
	TInt64 t11 = MAKE_TINT64(KMaxTUint32,KMaxTUint32);   // highest value stored === (2**64)-1
	test(I64LOW(t11)==KMaxTUint32 && I64HIGH(t11)==KMaxTUint32);

	// TInt64(TReal aVal)
	test.Next(_L("TInt64(TReal aVal)"));
	TInt64 t12((TInt64)1.0);
	test(I64LOW(t12)==1 && I64HIGH(t12)==0);
	TInt64 t15((TInt64)4.99);
	test(I64LOW(t15)==4 && I64HIGH(t15)==0);
	
	TReal x;
	
	x = -9.223372036854776831e18; // -2^63 - 2^10 (to ensure rounding outside of TInt64 range)
	TInt64 t16((TInt64)x);
	test(t16==KMinTInt64);
	TInt64 t17((TInt64)0.5);
	test(I64LOW(t17)==0 && I64HIGH(t17)==0);
	TInt64 t18((TInt64)0.0);
	test(I64LOW(t18)==0 && I64HIGH(t18)==0);
	TInt64 t19((TInt64)-123325.23411412);
	test(I64LOW(t19)==(TUint)(-123325) && I64HIGH(t19)==0xffffffff);
	TInt64 t20((TInt64)1.0E-1);
	test(I64LOW(t20)==0 && I64HIGH(t20)==0);
	
	// Make variable volatile to protect ourselves from compiler optimisations. Given that the
	// following test is negative with unspecified results, we don't really care if we do not have
	// FPU/compiler parity.
	volatile TReal xout;
	xout = 9.223372036854776831e18; // 2^63 + 2^10 (to ensure rounding outside of TInt64 range)
	TInt64 t21((TInt64)xout);

	// IEEE 754 does not specify the value to be returned when a conversion
	// is performed on a value that is outside the range of the target, only
	// that an invalid operation exception be raised if the io fp exception
	// is not masked.
#if defined(__WINS__) || defined(__X86__)
	// The x86 FPU returns KMin... as the "indefinite number"
	test(t21 == KMinTInt64);
#else
	// The target compiler support libraries return KMax...
	test(t21 == KMaxTInt64);
#endif

	TReal limit=1048576.0*1048576.0*8192.0;		// integers <2^53 in modulus can be represented exactly
	TInt64 t22((TInt64)limit);
	test(I64LOW(t22)==0 && I64HIGH(t22)==0x00200000);
	TInt64 t23((TInt64)(limit-1.0));
	test(I64LOW(t23)==0xffffffff && I64HIGH(t23)==0x001fffff);
	TReal i64limit=limit*1024.0;				// 2^63
	// Make variable volatile to protect ourselves from compiler optimisations. Given that the
	// following test is negative with unspecified results, we don't really care if we do not have
	// FPU/compiler parity.
	volatile TReal i64limitout=i64limit;
	TInt64 t24((TInt64)i64limitout);
	
	// IEEE 754 does not specify the value to be returned when a conversion
	// is performed on a value that is outside the range of the target, only
	// that an invalid operation exception be raised if the io fp exception
	// is not masked.
#if defined(__WINS__) || defined(__X86__)
	// The x86 FPU returns KMin... as the "indefinite number"
	test(t24 == KMinTInt64);
#else
	// The target compiler support libraries return KMax...
	test(t24 == KMaxTInt64);
#endif

	TInt64 t25((TInt64)(i64limit-1024.0));
	test(I64LOW(t25)==0xfffffc00 && I64HIGH(t25)==0x7fffffff);
	TInt64 t26((TInt64)-i64limit);
	test(I64LOW(t26)==0x00000000 && I64HIGH(t26)==0x80000000);
	TInt64 t27((TInt64)(1024.0-i64limit));
	test(I64LOW(t27)==0x00000400 && I64HIGH(t27)==0x80000000);


	TInt i;
	TInt64 l;
	for (i=-99; i<100; i++)
		{
		x=1;
		l=1;
		TReal a(i);
		TInt64 b(i);
		while (Abs(x)<limit)
			{
//			test.Printf(_L("Testing %g\n"),x);
			TInt64 ll((TInt64)x);
			test(ll==l);
			ll=0;
			ll = (TInt64)x;
			test(ll==l);
			x*=a;
			l*=b;
			if (i==1 || i==0 || (i==-1 && l==TInt64(1)))
				break;
			}
		}

	// TInt64::GetTReal
	test.Next(_L("TInt64::GetTReal"));

	// GCC does optimise large portions of the test code out and there can be small
	// differences in the way GCC and the FPU round floating point values.
	// We isolate the following test by giving it its own variables. This should
	// prevent values returned by the FPU from being compared with wrong GCC calculations.
	TInt64 m = MAKE_TINT64(0x7fffffff,0xffffffff);
	TReal xy = I64REAL(m);
	TReal xx = 1048576.0*1048576.0*1048576.0*8.0 - 1.0; // 2^63 - 1
	test(xy == xx);
	//

	l = MAKE_TINT64(0x7fffffff,0xfffffc00);
	x = I64REAL(l);

	test(x == (i64limit - 1024.0));

	l = MAKE_TINT64(0x80000000,0x00000000);
	x = I64REAL(l);

	test(x == -i64limit);

	l = MAKE_TINT64(0x80000000,0x00000400);
	x = I64REAL(l);

	test(x == (1024.0 - i64limit));

	l = MAKE_TINT64(0x00000001,0x00000000);
	x = I64REAL(l);

	test(x == (65536.0 * 65536.0));

	l = MAKE_TINT64(0xffffffff,0x00000000);
	x = I64REAL(l);

	test(x == (-65536.0 * 65536.0));

	for (i=-99; i<100; i++)
		{
		x=1;
		l=1;
		TReal a(i);
		TInt64 b(i);
		while (Abs(x)<limit)
			{
//			test.Printf(_L("Testing %g\n"),x);
			TReal y = I64REAL(l);
			test(y==x);
			x*=a;
			l*=b;
			if (i==1 || i==0 || (i==-1 && l==TInt64(1)))
				break;
			}
		}
 
	// TInt64(const TInt64& aVal)
	test.Next(_L("Copy constructor"));
	TInt64 t13(t10);
	test(I64LOW(t13)==I64LOW(t10) && I64HIGH(t13)==I64HIGH(t10));

	test.Next(_L("Set"));
	t13 = MAKE_TINT64(0, 0);
	test(I64LOW(t13)==0 && I64HIGH(t13)==0);
	test.End();
	}

LOCAL_C void Test1_2()
//
//	Test Unary operators -, and +
//
	{
	TInt64 r(0),q(0);

	r=1;
	test(-r==-1);
	test(+r==1);
	r=-100;
	test(-r==100);
	test(+r==-100);
	r = MAKE_TINT64(540423,21344);
	test(-r==(q-r));
	test(+r==r);
	test(+r==MAKE_TINT64(540423,21344));
	r=0;
	test(-r==0);
	test(+r==0);
	}


LOCAL_C void Test2()
	{
	// Test the operators

	// =
	test.Start(_L("="));
	TInt64 r=0,r2=0, a = MAKE_TINT64(12345,54321);
	r=KMaxTInt32;
	test(I64LOW(r)==(TUint)KMaxTInt32 && I64HIGH(r)==0);
	r2=r=KMinTInt32;
	test(I64INT(r)==KMinTInt32);
	test(I64INT(r2)==KMinTInt32);
	r2=r=KMaxTUint32;
	test(I64LOW(r)==KMaxTUint32 && I64HIGH(r)==0);
	test(I64LOW(r2)==KMaxTUint32 && I64HIGH(r2)==0);
	r2=r=a;
	test(I64LOW(r)==I64LOW(a) && I64HIGH(r)==I64HIGH(a));
	test(I64LOW(r2)==I64LOW(a) && I64HIGH(r2)==I64HIGH(a));

	r2=r=(TInt64)((TReal)1.2);
	test(r==1);
	test(r2==1);
	r2=r=(TInt64)((TReal)20.9);
	test(r==20);
	test(r2==20);
	r2=r=(TInt64)((TReal)-100.2);
	test(r==-100);
	test(r2==-100);
	


	// +=, -=, *=, /=, %=, >>=, <<=, >>, <<
	// += 
	test.Next(_L("+="));
	r=-1;
	r+=1;
	test(I64INT(r)==0);

	r+=1;
	test(I64INT(r)==1);

	r=KMaxTUint32;
	r+=1;
	test(I64INT(r)-1==(TInt)KMaxTUint32);

	r=KMinTInt32;
	r+=1;
	test(I64INT(r)==KMinTInt32+1);

	r=0;
	r+=MAKE_TINT64(0,0x80000000u);
	test(r==MAKE_TINT64(0,0x80000000u));

	// -=
	test.Next(_L("-="));						 
	r=-1;
	r-=1;
	test(I64INT(r)==-2);
	r=0;
	r-=1;
	test(I64INT(r)==-1);
	r=1;
	r-=1;
	test(I64INT(r)==0);
	r=KMaxTUint32;
	r+=1;
	r-=1;
	test(I64HIGH(r)==0 && I64LOW(r)==KMaxTUint32);
	test(I64INT(r)==(TInt)KMaxTUint32);
	r-=1;
	test(I64INT(r)==(TInt)(KMaxTUint32-1));


	// *= 
	test.Next(_L("*="));
	r=KMaxTUint32;			// ffffffff * 2 = 1 fffffffe
	r*=2;
	test(I64HIGH(r)==1 && I64LOW(r)==KMaxTUint32-1);

	r=KMaxTUint32-1;
	r*=2;
	r+=2;
	test(I64HIGH(r)==1 && I64LOW(r)==KMaxTUint32-1);

	r=KMaxTUint32;
	r+=1;
	r*=2;
	test(I64HIGH(r)==2 && I64LOW(r)==0);

	TUint PosMinTInt=(TUint)KMinTInt32;
	r=PosMinTInt;
	r*=-1;
	test(I64INT(r)==KMinTInt32);

	r=KMinTInt32;
	r*=-1;
	r-=1;
	test(I64INT(r)==KMaxTInt32);

	r=KMaxTUint32;	   	// ffffffff * ffffffff + (2 * ffffffff) = ffffffff ffffffff
	r*=KMaxTUint32;
	r+=KMaxTUint32;
	r+=KMaxTUint32;
	test(I64LOW(r)==KMaxTUint32 && I64HIGH(r)==KMaxTUint32);

	r=KMaxTUint32;
	r+=1;
	r*=2;
	test(I64LOW(r)==0 && I64HIGH(r)==2);


	// /=
	test.Next(_L("/="));
	r=4;
	r/=2;
	test(I64INT(r)==2);
	r=4;
	r/=-2;
	test(I64INT(r)==-2);
	r=-4;
	r/=1;
	test(I64INT(r)==-4);
	r=-8;

	r/=-2;
	test(I64INT(r)==4);
	r=4;

	r/=4;
	test(I64INT(r)==1);
	r=0;

	r/=4;
	test(I64INT(r)==0);
	r=KMaxTUint32;
	TInt64 z(KMaxTUint32);
	r/=z;
	test(I64INT(r)==1);
	r=KMinTInt32;
	z=KMinTInt32;
	r/=z;
	test(I64INT(r)==1);
	r=KMinTInt32;
	z = MAKE_TINT64(0,(TUint)KMinTInt32);
	r/=z;
	test(I64INT(r)==-1);
	r=KMaxTUint32;			
	r*=2;
	r/=2;
	test(I64HIGH(r)==0 && I64LOW(r)==KMaxTUint32);
   	r=KMaxTUint32;
	a=KMaxTUint32;
	r*=z;
	r/=z;
	test(I64HIGH(r)==0 && I64LOW(r)==KMaxTUint32);
	r = MAKE_TINT64(0,(TUint)KMinTInt32);
	r/=-1;
	test(I64LOW(r)==(TUint)KMinTInt32);
	r=0;
	r/=KMaxTUint32;
	test(I64INT(r)==0);
	r=0;
	r/=KMinTInt32;
	test(I64INT(r)==0);
	r=0;
	TInt64 b = MAKE_TINT64(KMaxTUint32,KMaxTUint32);
	r/=b;
	test(I64INT(r)==0);
	TInt64 c = MAKE_TINT64(KMaxTUint32,KMaxTUint32);	  // -1/Anything == 0
	TInt64* cptr = &c;		// MSVC compiler calculates -1/anything at compile time
	*cptr /= KMaxTUint32;	// and gets it wrong if we don't use a pointer, bizarre.
	test(I64LOW(c)==0 && I64HIGH(c)==0);
	r=-1;
	z=1;
	r/=z;
	test(r==-1);
	r=-r;
	r/=z;
	test(r==1);
	r = MAKE_TINT64(0x01,KMaxTUint32);
	z=2;
	r/=z;
	test(r==TInt64(KMaxTUint32));
	r = MAKE_TINT64(0x01,KMaxTUint32);
	z = MAKE_TINT64(0,KMaxTUint32);
	r/=z;
	test(r==TInt64(2));
	r = MAKE_TINT64(1,0);
	r/=z;
	test(r==TInt64(1));
	r = MAKE_TINT64(6221,5621243);
	z = MAKE_TINT64(3,42011);
	r/=z;
	test(r==2073);
	r=100;
	z=99;
	r/=z;
	test(r==1);
	r = MAKE_TINT64(17,KMaxTUint32);
	z = MAKE_TINT64(0,8);
	test((r/=z)==MAKE_TINT64(0x2, 0x3fffffff));

	// %=
	test.Next(_L("%="));
	r=4;
	r%=4;
	test(I64INT(r)==0);
	r=4;
	r%=1;
	test(I64INT(r)==0);
	r=5;
	r%=3;
	test(I64INT(r)==2);
	r=(-5);
	r%=3;
	test(I64INT(r)==(-2));
	r = MAKE_TINT64(134,KMaxTUint32-10342);
	z = MAKE_TINT64(134,0);
	test((r%=z)==KMaxTUint32-10342);
	r = MAKE_TINT64(134,KMaxTUint32-10342);
	z = MAKE_TINT64(134,KMaxTUint32-10343);
	test((r%=z)==1);
	r = MAKE_TINT64(1363,0xfd432ab0u);
	z = MAKE_TINT64(0,16);
	test((r%=z)==0);
	
	r=-10;
	r%=3;
	test(r==-1);
	r=-10;
	r%=-3;
	test(r==-1);
	r=10;
	r%=3;
	test(r==1);
	r=10;
	r%=-3;
	test(r==1);

	// <<= and >>=
	// <<=
	test.Next(_L("<<="));
	r=1;
	r<<=32;
	test(I64LOW(r)==0 && I64HIGH(r)==1);
	r<<=31;
	test(I64LOW(r)==0 && I64HIGH(r)==0x80000000);

	r=1;
	r<<=31;
	test(I64LOW(r)==0x80000000 && I64HIGH(r)==0);
	r<<=32;
	test(I64LOW(r)==0 && I64HIGH(r)==0x80000000);

	r=1;
	r<<=63;
	test(I64LOW(r)==0 && I64HIGH(r)==0x80000000);

	r=0;
	r<<=32;
	test(I64LOW(r)==0 && I64HIGH(r)==0);

	r=0xC0000000; 	// 1100000..........
	r<<=1;
	test(I64HIGH(r)==1 && I64LOW(r)==0x80000000);	// 100000.......
	r<<=1;
	test(I64HIGH(r)==3 && I64LOW(r)==0);
	r<<=1;
	test(I64HIGH(r)==6 && I64LOW(r)==0);

	r = MAKE_TINT64(0,KMaxTUint32);
	r<<=32;
	test(I64LOW(r)==0 && I64HIGH(r)==KMaxTUint32);

	// >>=
	test.Next(_L(">>="));
	r = MAKE_TINT64(3,0);
	r>>=1;
	test(I64HIGH(r)==1 && I64LOW(r)==0x80000000);
	r>>=1;
	test(I64HIGH(r)==0 && I64LOW(r)==0xC0000000);

	r = MAKE_TINT64(0x80000000,0);  
	r>>=(31);
	test(I64HIGH(r)==KMaxTUint32 && I64LOW(r)==0);
	r>>=(32);
	test(I64HIGH(r)==KMaxTUint32 && I64LOW(r)==KMaxTUint32);

	r = MAKE_TINT64(0x80000000,0);
	r>>=(32);
	test(I64HIGH(r)==KMaxTUint32 && I64LOW(r)==0x80000000);

	r = MAKE_TINT64(0x80000000,0);
	r>>=63;
	test(I64LOW(r)==KMaxTUint32 && I64HIGH(r)==KMaxTUint32);

	r = MAKE_TINT64(KMaxTUint32, 0);
	r>>=32;
	test(I64HIGH(r)==KMaxTUint32 && I64LOW(r)==KMaxTUint32);

	// <<
	test.Next(_L("<<"));
	r=1;
	TInt64 t = r<<32;
	test(I64LOW(t)==0 && I64HIGH(t)==1);
	t = t<<31;
	test(I64LOW(t)==0 && I64HIGH(t)==0x80000000);

	r=1;
	t = r<<31;
	test(I64LOW(t)==0x80000000 && I64HIGH(t)==0);
	t = t<<32;
	test(I64LOW(t)==0 && I64HIGH(t)==0x80000000);

	r=1;
	t = r<<63;
	test(I64LOW(t)==0 && I64HIGH(t)==0x80000000);

	r=0;
	t = r<<32;
	test(I64LOW(t)==0 && I64HIGH(t)==0);

	r=0xC0000000; 	// 1100000..........
	t = r<<1;
	test(I64HIGH(t)==1 && I64LOW(t)==0x80000000);	// 100000.......
	t = t<<1;
	test(I64HIGH(t)==3 && I64LOW(t)==0);
	t = t<<1;
	test(I64HIGH(t)==6 && I64LOW(t)==0);

	r = MAKE_TINT64(0,KMaxTUint32);
	t = r<<32;
	test(I64LOW(t)==0 && I64HIGH(t)==KMaxTUint32);

	// >>
	test.Next(_L(">>"));
	r = MAKE_TINT64(3,0);
	t = r>>1;
	test(I64HIGH(t)==1 && I64LOW(t)==0x80000000);
	t = t>>1;
	test(I64HIGH(t)==0 && I64LOW(t)==0xC0000000);

	r = MAKE_TINT64(0x80000000,0);  
	t = r>>(31);
	test(I64HIGH(t)==KMaxTUint32 && I64LOW(t)==0);
	t = t>>(32);
	test(I64HIGH(t)==KMaxTUint32 && I64LOW(t)==KMaxTUint32);
	t = t>>1;
	test(I64HIGH(t)==KMaxTUint32 && I64LOW(t)==KMaxTUint32);
	t = t>>16;
	test(I64HIGH(t)==KMaxTUint32 && I64LOW(t)==KMaxTUint32);

	r = MAKE_TINT64(0x80000000,0);
	t = r>>(32);
	test(I64HIGH(t)==KMaxTUint32 && I64LOW(t)==0x80000000);

	r = MAKE_TINT64(0x80000000,0);
	t = r>>63;
	test(I64LOW(t)==KMaxTUint32 && I64HIGH(t)==KMaxTUint32);

	r = MAKE_TINT64(KMaxTUint32, 0);
	t = r>>32;
	test(I64HIGH(t)==KMaxTUint32 && I64LOW(t)==KMaxTUint32);

	r = MAKE_TINT64(0x40000000,0);
	t = r>>30;
	test(I64HIGH(t)==1 && I64LOW(t)==0);
	t = r>>31;
	test(I64HIGH(t)==0 && I64LOW(t)==0x80000000);
	t = r>>62;
	test(I64HIGH(t)==0 && I64LOW(t)==1);
	t = r>>63;
	test(I64HIGH(t)==0 && I64LOW(t)==0);

	test.End();
	}

LOCAL_C void Test3()
	{
	// Test some more operators

	// unary -
	test.Start(_L("unary -"));
	TInt64 r=0, x(KMinTInt32);
	r=-x;
	test(I64INT(r)==KMinTInt32);

	x = MAKE_TINT64(0,0x80000000);
	r=-x;
	test(I64INT(r)==KMinTInt32);

	// ++
	// post increment
	test.Next(_L("++"));
	x=-1;
	r=x++;
	test(I64INT(r)==-1 && I64INT(x)==0);
	r=x++;
	test(I64INT(r)==0 && I64INT(x)==1);
	r=x++;
	test(I64INT(r)==1 && I64INT(x)==2);

	x=KMinTInt32;
	r=x++;
	test(I64INT(r)==KMinTInt32 && I64INT(x)==KMinTInt32+1);

	x=KMaxTUint32;
	r=x++;
	test(I64INT(r)==(TInt)KMaxTUint32 && I64HIGH(x)==1 && I64LOW(x)==0);
	r=x++;
	test(I64HIGH(r)==1 && I64LOW(r)==0 && I64HIGH(x)==1 && I64LOW(x)==1);

	// pre increment;
	x=-1;
	r=++x;
	test(I64INT(r)==0 && I64INT(x)==0);
	r=++x;
	test(I64INT(r)==1 && I64INT(x)==1);
	r=++x;
	test(I64INT(r)==2 && I64INT(x)==2);

	x=KMinTInt32;
	r=++x;
	test(I64INT(r)==KMinTInt32+1 && I64INT(x)==KMinTInt32+1);

	x=KMaxTUint32;
	r=++x;
	test(I64HIGH(r) && I64HIGH(x)==1 && I64LOW(x)==0);
	r=x++;
	test(I64HIGH(r)==1 && I64LOW(r)==0 && I64HIGH(x)==1 && I64LOW(x)==1);

	
	// --
	test.Next(_L("--"));
	// post decrement	   
	x=1;
	r=x--;
	test(I64INT(r)==1 && I64INT(x)==0);
	r=x--;
	test(I64INT(r)==0 && I64INT(x)==-1);
	r=x--;
	test(I64INT(r)==-1 && I64INT(x)==-2);

	x=KMinTInt32+1;
	r=x--;
	test(I64INT(r)==KMinTInt32+1 && I64INT(x)==KMinTInt32);

	x=KMaxTUint32;
	x+=1;
	r=x--;
	test(I64HIGH(r)==1 && I64LOW(r)==0 && I64HIGH(x)==0 && I64LOW(x)==KMaxTUint32);
	r=x--;
	test(I64HIGH(r)==0 && I64LOW(r)==KMaxTUint32 && I64HIGH(x)==0 && I64LOW(x)==KMaxTUint32-1);

	x = MAKE_TINT64(KMaxTUint32,KMaxTUint32);
	r=x--;
	test(I64HIGH(r)==KMaxTUint32 && I64LOW(r)==KMaxTUint32 && I64HIGH(x)==KMaxTUint32 && I64LOW(x)==KMaxTUint32-1);

	// pre decrement	   
	x=1;
	r=--x;
	test(I64INT(r)==0 && I64INT(x)==0);
	r=--x;
	test(I64INT(r)==-1 && I64INT(x)==-1);
	r=--x;
	test(I64INT(r)==-2 && I64INT(x)==-2);

	x=KMinTInt32+1;
	r=--x;
	test(I64INT(r)==KMinTInt32 && I64INT(x)==KMinTInt32);

	x=KMaxTUint32;
	x+=1;
	r=--x;
	test(I64HIGH(r)==0 && I64LOW(r)==KMaxTUint32 && I64HIGH(x)==0 && I64LOW(x)==KMaxTUint32);
	r=--x;
	test(I64HIGH(r)==0 && I64LOW(r)==KMaxTUint32-1 && I64HIGH(x)==0 && I64LOW(x)==KMaxTUint32-1);

	x = MAKE_TINT64(KMaxTUint32,KMaxTUint32);
	r=--x;
	test(I64HIGH(r)==KMaxTUint32 && I64LOW(r)==KMaxTUint32-1 && I64HIGH(x)==KMaxTUint32 && I64LOW(x)==KMaxTUint32-1);

  

	// Binary +
	test.Next(_L("Binary +"));
	x=KMinTInt32;
	r=x+1;
	test(I64INT(r)==KMinTInt32+1 && I64INT(x)==KMinTInt32);

	x=-1;
	r=x+1;
	test(I64INT(r)==0 && I64INT(x)==-1);
	x=r+1;
	test(I64INT(r)==0 && I64INT(x)==1);

	x=KMaxTUint32;
	r=x+KMaxTUint32;
	test(I64HIGH(r)==1 && I64LOW(r)==KMaxTUint-1);

	x=KMaxTUint32;
	x+=1;
	r=x+(-1);
	test(I64HIGH(r)==0 && I64LOW(r)==KMaxTUint32);

	TInt64 y(0);
	x=KMaxTUint32;
	r=x+y;
	test(I64HIGH(r)==0 && I64LOW(r)==KMaxTUint32);

	y=KMaxTUint32;
	r=x+y;
	test(I64HIGH(r)==1 && I64LOW(r)==KMaxTUint32-1);

	y=0;
	x = MAKE_TINT64(KMaxTUint32,KMaxTUint32);
	r=x+y;
	test(I64HIGH(r)==KMaxTUint32 && I64LOW(r)==KMaxTUint32);

	y=1;
	x = MAKE_TINT64(KMaxTUint32,KMaxTUint32);
	x-=1;
	r=x+y;
	test(I64HIGH(r)==KMaxTUint32 && I64LOW(r)==KMaxTUint32);

	y=0;
	x=KMinTInt32;
	r=x+y;
	test(I64INT(r)==KMinTInt32);

	y=-1;
	x=KMinTInt32;
	x+=1;
	r=x+y;
	test(I64INT(r)==KMinTInt32);
	
	y=-1;
	x=-1;
	r=x+y;
	test(I64INT(r)==-2);

	y=-1;
	x=KMaxTUint32;
	r=x+y;
	test(I64HIGH(r)==0 && I64LOW(r)==KMaxTUint32-1);
	y=-1;
	x+=1;
	r=x+y;
	test(I64HIGH(r)==0 && I64LOW(r)==KMaxTUint32);
	y=-1;
	x = MAKE_TINT64(KMaxTUint32,KMaxTUint32);
	r=x+y;
	test(I64HIGH(r)==KMaxTUint32 && I64LOW(r)==KMaxTUint32-1);

	y=KMinTInt32;
	x=KMaxTUint32;
	r=x+y;
	test(I64HIGH(r)==0 && I64LOW(r)==KMaxTUint32-(TUint)KMinTInt32);
	x = MAKE_TINT64(KMaxTUint32,KMaxTUint32);
	r=x+y;
	test(I64HIGH(r)==KMaxTUint32 && I64LOW(r)==0x7fffffff);

	y=KMinTInt32;
	x=-(KMinTInt32+1);
	x+=1;
	r=x+y;
	test(I64INT(r)==0);


	// Binary -
	test.Next(_L("Binary -"));
	x=KMinTInt32+1;
	r=x-1;
	test(I64INT(r)==KMinTInt32);

	x=2;
	r=x-1;
	test(I64INT(r)==1);
	x=1;
	r=x-1;
	test(I64INT(r)==0);
	x=0;
	r=x-1;
	test(I64INT(r)==-1);

	x=KMaxTUint32;
	r=x-KMaxTUint32;
	test(I64INT(r)==0);

	x=KMaxTUint32;
	x+=1;
	r=x-1;
	test(I64HIGH(r)==0 && I64LOW(r)==KMaxTUint32);

	x=KMaxTUint32;
	r=x-1;
	test(I64HIGH(r)==0 && I64LOW(r)==KMaxTUint32-1);

	x = MAKE_TINT64(KMaxTUint32,KMaxTUint32);
	r=x-1;
	test(I64HIGH(r)==KMaxTUint32 && I64LOW(r)==KMaxTUint32-1);


	y=0;
	x=KMaxTUint32;
	r=x-y;
	test(I64HIGH(r)==0 && I64LOW(r)==KMaxTUint32);

	y=KMaxTUint32;
	r=x-y;
	test(I64INT(r)==0);

	x=KMaxTUint32;
	x+=1;
	y=1;
	r=x-1;
   	test(I64HIGH(r)==0 && I64LOW(r)==KMaxTUint32);
	x-=1;
	r=x-1;
	test(I64HIGH(r)==0 && I64LOW(r)==KMaxTUint32-1);

	y=0;
	x = MAKE_TINT64(KMaxTUint32,KMaxTUint32);
	r=x-y;
	test(I64HIGH(r)==KMaxTUint32 && I64LOW(r)==KMaxTUint32);

	y=1;
	x = MAKE_TINT64(KMaxTUint32,KMaxTUint32);
	r=x-y;
	test(I64HIGH(r)==KMaxTUint32 && I64LOW(r)==KMaxTUint32-1);

	y=0;
	x=KMinTInt32;
	r=x-y;
	test(I64INT(r)==KMinTInt32);

	y=1;
	x=KMinTInt32;
	x+=1;
	r=x-y;
	test(I64INT(r)==KMinTInt32);
	
	y=1;
	x=1;
	r=x-y;
	test(I64INT(r)==0);

	y=-1;
	x=-1;
	r=x-y;
	test(I64INT(r)==0);

	x=0;
	y=KMinTInt32;
	r=x-y;
	test(I64INT(r)==KMinTInt32);

	x=KMinTInt32;
	y=KMinTInt32;
	r=x-y;
	test(I64INT(r)==0);

	test.End();
	}


LOCAL_C void Test4()
// still more operators
	{
	// * 
	test.Start(_L("Binary *"));
	TInt64 r(0), x(1), y(0);
	r=x*y;
	test(I64INT(r)==0);

	y=-1;
	r=x*y;
	test(I64INT(r)==-1);

	x=-1;
	r=x*y;
	test(I64INT(r)==1);

	x=KMinTInt32;
	r=x*y;
	test(I64INT(r)==KMinTInt32);

	y=0;
	r=x*y;
	test(I64INT(r)==0);

	y=KMinTInt32;
	r=x*y;
	test(I64LOW(r)==0 && I64HIGH(r)==0x40000000);

	y=KMaxTUint32;
	x=KMaxTUint32;
	r=x*y;
	test(I64LOW(r)==1 && I64HIGH(r)==0xfffffffe);


	// /
	test.Next(_L("Binary /"));
	x=5;
	y=5;
	r=x/y;
	test(I64INT(r)==1);

	y=1;
	r=x/y;
	test(I64INT(r)==5);

	x=-5;
	r=x/y;
	test(I64INT(r)==-5);

	y=-1;
	r=x/y;
	test(I64INT(r)==5);
	
	x=-1;
	r=x/y;
	test(I64INT(r)==1);

	x=0;
	r=x/y;
	test(I64INT(r)==0);

	x=KMinTInt32;
	y=-1;
	r=x/y;
	test(I64INT(r)==KMinTInt32);

	x=KMinTInt32;
	y=KMinTInt32;
	r=x/y;
	test(I64INT(r)==1);

	x=KMaxTUint32;
	y=KMaxTUint32;
	r=x/y;
	test(I64INT(r)==1);

	x = MAKE_TINT64(KMaxTUint32,KMaxTUint32);
	r=x/y;	   
	test(I64INT(r)==0);

	y = MAKE_TINT64(KMaxTUint32,KMaxTUint32);
	r=x/y;
	test(I64INT(r)==1);

	y=KMinTInt32;
	r=x/y;
	test(I64INT(r)==0);
  
    x = MAKE_TINT64(4,257629747);
    y=KMaxTInt;
	r=x/y;
	test(I64LOW(r)==8 && I64HIGH(r)==0);

	x = MAKE_TINT64(3452,533254);
	x=-x;
	x=x/x;
	test(x==1);

  	// %
	test.Next(_L("binary %%"));
	x=2341;
	y=2340;
	test(x%y==1);
	y=2;
	test(x%y==1);
	x = MAKE_TINT64(234893,23494);
	test(x%x==0);
	test(x%y==0);
	x=-x;
	y=10;
	test(x%y==-2);
	test(x%(-y)==-2);
	
					 
	// Lsr
	test.Next(_L("Lsr"));

	r = MAKE_TINT64(3,0);
	I64LSR(r, 1);
	test(I64HIGH(r)==1 && I64LOW(r)==0x80000000);
	I64LSR(r, 1);
	test(I64HIGH(r)==0 && I64LOW(r)==0xC0000000);

	r = MAKE_TINT64(0x80000000,0);  
	I64LSR(r, 31);
	test(I64HIGH(r)==1 && I64LOW(r)==0);
	//test(I64HIGH(r)==KMaxTUint32 && I64LOW(r)==0);
	I64LSR(r, 32);
	//test(I64HIGH(r)==KMaxTUint32 && I64LOW(r)==KMaxTUint32);
	test(I64HIGH(r)==0 && I64LOW(r)==1);

	r = MAKE_TINT64(0x80000000,0);
	I64LSR(r, 32);
	test(I64HIGH(r)==0 && I64LOW(r)==0x80000000);
	//test(I64HIGH(r)==KMaxTUint32 && I64LOW(r)==0x80000000);

	r = MAKE_TINT64(0x80000000,0);
	I64LSR(r, 63);
	test(I64LOW(r)==1 && I64HIGH(r)==0);
	//test(I64LOW(r)==KMaxTUint32 && I64HIGH(r)==KMaxTUint32);

	r = MAKE_TINT64(KMaxTUint32, 0);
	I64LSR(r, 32);
	test(I64HIGH(r)==0 && I64LOW(r)==KMaxTUint32);
	//test(I64HIGH(r)==KMaxTUint32 && I64LOW(r)==KMaxTUint32);


	// Mul10
	test.Next(_L("Mul10"));
	const TInt64 KMaxDiv10= KMaxTInt64 / 10;
	const TInt64 KStep=MAKE_TINT64(0x003dfe03, 0xf7ea23cd);
	for(TInt64 jj=-KMaxDiv10; jj<=KMaxDiv10; jj+=KStep)
		{
		r=jj;
		r *= 10;
		test(r==jj*10);
		r/=10;
		test(r==jj);
		}

	r=KMinTInt32/10;
	r *= 10;
	test(I64INT(r)==(KMinTInt/10)*10);

	r=KMaxTUint32;
	r *= 10;
	test(I64HIGH(r)==9 && I64LOW(r)==0xFFFFFFF6);

	r/=10;
	test(r==MAKE_TINT64(0,KMaxTUint32));


	// DivMod
	test.Next(_L("DivMod"));
	TInt64 seed = MAKE_TINT64(0x0000336a,0xb2001a78);
	for (TInt i=0; i<200; i++)
		{
		TInt o=Math::Rand(seed);
		TInt p=Math::Rand(seed);
		TInt r=Math::Rand(seed);
		TInt q=Math::Rand(seed);

		DivModTest(MAKE_TINT64(0,q), MAKE_TINT64(0,r));
		DivModTest(MAKE_TINT64(r,q), MAKE_TINT64(o,p));
		DivModTest(MAKE_TINT64(p,q), MAKE_TINT64(0,o));
		DivModTest(MAKE_TINT64(0,p), MAKE_TINT64(r,o));
		
		DivModTest(-MAKE_TINT64(0,q), -MAKE_TINT64(0,r));
		DivModTest( MAKE_TINT64(0,q), -MAKE_TINT64(0,r));
		DivModTest(-MAKE_TINT64(0,q),  MAKE_TINT64(0,r));

		DivModTest(-MAKE_TINT64(r,q), -MAKE_TINT64(o,p));
		DivModTest( MAKE_TINT64(r,q), -MAKE_TINT64(o,p));
		DivModTest(-MAKE_TINT64(r,q),  MAKE_TINT64(o,p));

		DivModTest(-MAKE_TINT64(0,p), -MAKE_TINT64(r,o));
 		DivModTest( MAKE_TINT64(0,p), -MAKE_TINT64(r,o));
		DivModTest(-MAKE_TINT64(0,p),  MAKE_TINT64(r,o));
		}

	test.End();
	}

LOCAL_C void Test5()
// still more operators
	{

	// fast multiply by 10
	test.Start(_L("Mul10"));
   	TInt64 r(0);
	r *= 10;
	test(I64INT(r)==0);

	r=-1;
	r *= 10;
	test(I64INT(r)==-10);

	r=KMinTInt32/10;
	r *= 10;
	test(I64INT(r)==KMinTInt32-(KMinTInt32%10));

	r=1;
	r *= 10;
	test(I64INT(r)==10);

	r=KMaxTUint32/10;
	r *= 10;
	test(I64LOW(r)==(KMaxTUint32-(KMaxTUint%10)) && I64HIGH(r)==0);

	r *= 10;
	test(r==TInt64(KMaxTUint32-(KMaxTUint%10))*10);

	r *= 10;
	test(r==TInt64(KMaxTUint32-(KMaxTUint%10))*100);

 	// Comparisons
	test.Next(_L("Comparison operators"));

	// == , !=, <= and >=
	test.Next(_L("==, !=, <= and >="));
	r=KMinTInt32;
	TInt64 x(KMinTInt32);
	TInt64 y(100);
	test(r==x && r!=y && r>=x && r<=x);

	r=-1;
	x=-1;
	test(r==x && r!=y && r>=x && r<=x);

	r=0;
	x=0;
	test(r==x && r!=y && r>=x && r<=x);

	r=1;
	x=1;
	test(r==x && r!=y && r>=x && r<=x);

	r=KMaxTUint32;
	x=KMaxTUint32;
	test(r==x && r!=y && r>=x && r<=x);

	r = MAKE_TINT64(KMaxTUint32,KMaxTUint32);
	x = MAKE_TINT64(KMaxTUint32,KMaxTUint32);
	test(x==r && x!=y && x>=r && x<=r);

	//>=, <=, > and <
	test.Next(_L(">=, <=, > and <"));
	r=KMinTInt32;
	x=KMinTInt32+1;
	test(x>r && x>=r && r<x && r<=x);

	r=1;
	x=-1;
	test(r>x && x<r && r>=x && x<=r);

	r=KMaxTUint32;
	x=KMaxTUint32-1;
	test(r>x && x<r && r>=x && x<=r);

	r+=1;
	x+=1;
	test(r>x && x<r && r>=x && x<=r);

	r = MAKE_TINT64(KMaxTUint32,KMaxTUint32);
	x = MAKE_TINT64(KMaxTUint32,KMaxTUint32-1);
	test(r>x && x<r && r>=x && x<=r);

	r = MAKE_TINT64(0x80000000,0);
	x = MAKE_TINT64(KMaxTInt32,KMaxTUint32);
	test(r<x);
	test(x>r);
	test(x!=r);
	test(r<=x);
	test(x>=r);
	test(r<0 && x>0);

	x = MAKE_TINT64(0x80000000,1);
	test(r<x && x>r && x!=r && r<=x && x>=r);

	r = MAKE_TINT64(KMaxTInt32,KMaxTUint32);
	--r;
	test(r>x && x<r && x!=r && r>=x && x<=r);

	// multiply top bits
	test.Next(_L("MulTop"));
   	r=0;
    x=0;
	I64MULTOP(r, x);
	test(I64INT(r)==0);

	r=1;
    x=1;
	I64MULTOP(r, x);
	test(I64INT(r)==0);

	r = MAKE_TINT64(KMaxTInt,KMaxTUint);
    x=2;
	I64MULTOP(r, x);
	test(I64INT(r)==0);

	r = MAKE_TINT64(KMaxTInt,KMaxTUint);
    x=4;
	I64MULTOP(r, x);
	test(I64INT(r)==1);

    r = MAKE_TINT64(0x80000000,0);
    x = MAKE_TINT64(0x80000000,0);
	I64MULTOP(r, x);
    r>>=32;
	test(I64INT(r)==0x40000000);

    r = MAKE_TINT64(0x18763529,0x93263921);
    x = MAKE_TINT64(0x0abcdef0,0x647239ea);
    TInt64 r2=r;
    TInt64 x2=x;
    I64MULTOP(r, x2);
    I64MULTOP(x, r2);
    test(r==x);

//	TInt64(0xac11b680,0x1e603000) * TInt64(0x014a5c20,0xc9d58740)

	TPtrC8 a4=_L8("ac11b6801e603000");
	TPtrC8 a5=_L8("014a5c20c9d58740");
	TBuf8<64> a6;
	HexMul(a4,a5,a6);

	x = MAKE_TINT64(0x014a5c20,0xc9d58740); 

	r = MAKE_TINT64(0xac11b680,0x1e603000);
	y = MAKE_TINT64(0x0963fbc4,0x415c0000); // Expected result (bottom 64 bits)

	r *= x;

	test(r==y);

	r = MAKE_TINT64(0xac11b680,0x1e603000);
	y = MAKE_TINT64(0x00de0cc1,0xa89d70dc); // Expected result (top 64 bits)
	I64MULTOP(r, x);
	test(r==y);

	test.End();
	}


GLDEF_C TInt E32Main()
    {
	test.Title();
	test.Start(_L("Constructors"));
	Test1();
	test.Next(_L("Unary operators"));
	Test1_2();
	test.Next(_L("Operators 1"));
	Test2();
	test.Next(_L("Operators 2"));
	Test3();
	test.Next(_L("Operators 3"));
	Test4();
	test.Next(_L("Operators 4"));
	Test5();
	test.End();
	return(KErrNone);
    }
