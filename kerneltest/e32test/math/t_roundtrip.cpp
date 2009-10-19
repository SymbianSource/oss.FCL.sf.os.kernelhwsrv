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
// e32test\math\t_roundtrip.cpp
// Tests round-trip convertibility of double->string->double
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32math.h>

//#define __ALWAYS_PRINT__

RTest test(_L("T_ROUNDTRIP"));

void PrintRealHex(const char* aTitle, const TReal& aIn)
	{
	volatile TUint32* in = (volatile TUint32*)&aIn;
#ifdef __DOUBLE_WORDS_SWAPPED__
	TUint32 high = in[0];
	TUint32 low = in[1];
#else
	TUint32 high = in[1];
	TUint32 low = in[0];
#endif
	TBuf<256> title;
	if (aTitle)
		title.Copy(TPtrC8((const TUint8*)aTitle));
	test.Printf(_L("%S%08x %08x\n"), &title, high, low);
	}

TInt RoundTrip(TReal& aOut, const TReal& aIn)
	{
	TBuf8<64> text;
	TRealFormat fmt;
	fmt.iType = KRealFormatExponent | KRealInjectiveLimit | KUseSigFigs | KDoNotUseTriads | KAllowThreeDigitExp;
	fmt.iWidth = 32;
	fmt.iPlaces = KIEEEDoubleInjectivePrecision;
	fmt.iPoint = '.';
#ifdef __ALWAYS_PRINT__
	PrintRealHex("Input : ", aIn);
#endif
	TInt r = text.Num(aIn, fmt);
	if (r<0)
		{
		test.Printf(_L("Result %d (Num)\n"), r);
		return r;
		}
#ifdef __ALWAYS_PRINT__
	TBuf16<64> text16;
	text16.Copy(text);
	test.Printf(_L("Text  : %S\n"), &text16);
#endif
	TLex8 lex(text);
	r = lex.Val(aOut);
	if (r < 0)
		{
		test.Printf(_L("Result %d (Val)\n"), r);
		return r;
		}
#ifdef __ALWAYS_PRINT__
	PrintRealHex("Output: ", aOut);
#endif
	volatile TUint32* in = (volatile TUint32*)&aIn;
	volatile TUint32* out = (volatile TUint32*)&aOut;
	if (in[0]!=out[0] || in[1]!=out[1])
		{
		test.Printf(_L("Unsuccessful\n"));
#ifndef __ALWAYS_PRINT__
		PrintRealHex("Input : ", aIn);
		TBuf16<64> text16;
		text16.Copy(text);
		test.Printf(_L("Text  : %S\n"), &text16);
		PrintRealHex("Output: ", aOut);
#endif
		return KErrUnknown;
		}
	return KErrNone;
	}

const TUint64 KMantissaOverflow =	UI64LIT(0x20000000000000);	// 2^53
const TUint64 KMantissaThreshold =	UI64LIT(0x10000000000000);	// 2^52

class R
	{
public:
	enum {EMinExp=0, EMinNormExp=1, EMaxNormExp=2046, EMaxExp=2047};
public:
	R();
	R(const TReal& aIn);
	TReal Value() const;
	TInt Next();
	TInt Prev();
public:
	TUint64	iMant;		//	if iExp>0 2^52<=iMant<2^53 else 0<=iMant<2^52
	TInt	iExp;		//	0 < iExp < 2047
	TInt	iSign;
	};

R::R()
	{
	iMant = 0;
	iExp = 0;
	iSign = 0;
	}

R::R(const TReal& aIn)
	{
	const volatile TUint32* in = (const volatile TUint32*)&aIn;
#ifdef __DOUBLE_WORDS_SWAPPED__
	TUint32 high = in[0];
	TUint32 low = in[1];
#else
	TUint32 high = in[1];
	TUint32 low = in[0];
#endif
	iSign = high >> 31;
	iExp = (high >> 20) & EMaxExp;
	iMant = MAKE_TUINT64(high, low);
	iMant <<= 12;
	iMant >>= 12;
	if (iExp)
		iMant += KMantissaThreshold;
	}

TReal R::Value() const
	{
	TUint32 high = iSign ? 1 : 0;
	high <<= 31;
	high |= (iExp<<20);
	TUint32 mh = I64HIGH(iMant);
	mh <<= 12;
	high |= (mh>>12);
	TUint32 low = I64LOW(iMant);

	union {TReal iReal; TUint32 iX[2];} result;
#ifdef __DOUBLE_WORDS_SWAPPED__
	result.iX[0] = high;
	result.iX[1] = low;
#else
	result.iX[0] = low;
	result.iX[1] = high;
#endif
	return result.iReal;
	}

TInt R::Next()
	{
	if (iExp>0)
		{
		if (++iMant == KMantissaOverflow)
			{
			iMant >>= 1;
			if (++iExp == EMaxExp)
				return KErrOverflow;
			}
		return KErrNone;
		}
	if (++iMant == KMantissaThreshold)
		iExp = 1;
	return KErrNone;
	}

TInt R::Prev()
	{
	if (iExp == EMaxExp)
		{
		if (iMant == KMantissaThreshold)
			{
			--iExp;
			return KErrNone;
			}
		return KErrGeneral;
		}
	if (iExp>0)
		{
		if (--iMant < KMantissaThreshold)
			{
			if (--iExp)
				{
				iMant <<= 1;
				iMant++;
				}
			}
		return KErrNone;
		}
	if (iMant==0)
		return KErrUnderflow;
	--iMant;
	return KErrNone;
	}

void DoTest(R& aR, TInt& aErrorCount)
	{
	TReal out;
	TInt r;
	r = RoundTrip(out, aR.Value());
	if (r==KErrUnknown)
		++aErrorCount;
	R R1(aR);
	R R2(aR);
	if (R1.Next()==KErrNone)
		{
		r = RoundTrip(out, R1.Value());
		if (r==KErrUnknown)
			++aErrorCount;
		}
	if (R2.Prev()==KErrNone)
		{
		r = RoundTrip(out, R2.Value());
		if (r==KErrUnknown)
			++aErrorCount;
		}
	}

void DoTest(TInt aExp, TInt& aErrorCount)
	{
	R x;
	x.iExp = aExp;
	x.iMant = KMantissaThreshold;
	if (aExp==0)
		{
		do	{
			x.iMant >>= 1;
			DoTest(x, aErrorCount);
			} while (x.iMant);
		}
	else
		{
		DoTest(x, aErrorCount);
		}
	}

void DoTestPow10(TInt aPow, TInt& aErrorCount)
	{
	TReal64 r64;
	TInt r = Math::Pow10(r64, aPow);
	if (r<0)
		return;
	R x(r64);
	DoTest(x, aErrorCount);
	}

void DoTestRandom(TInt& aErrorCount)
	{
	static TInt64 randSeed = I64LIT(0x3333333333333333);
	R x;
	x.iExp = Math::Rand(randSeed) & R::EMaxExp;
	x.iMant = ((TUint64)Math::Rand(randSeed) << 32) | (TUint64)Math::Rand(randSeed);
	while (x.iMant > KMantissaThreshold)
		x.iMant >>= 1;
	x.iSign = Math::Rand(randSeed) & 0x1;
	DoTest(x, aErrorCount);
	}

TInt E32Main()
	{
	test.Title();
	test.Start(_L("Testing conversion from double->string->double"));

	TInt exp;
	TInt errors = 0;
	test.Next(_L("Test the conversion of powers of 2"));
	for (exp = 0; exp < 2047; ++exp)
		{
		DoTest(exp, errors);
		}

	test.Next(_L("Test the conversion of powers of 10"));
	for (exp = -325; exp < 325; ++exp)
		{
		DoTestPow10(exp, errors);
		}

	test.Next(_L("Test the conversion of some random numbers"));
	for (exp = 0; exp < 100; ++exp)
		{
		DoTestRandom(errors);
		}

	test_Equal(0, errors);

	test.End();
	return KErrNone;
	}
