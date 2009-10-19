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
// e32\euser\maths\um_pow.cpp
// Raise to the power.
// 
//

#include "um_std.h"

#if defined(__USE_VFP_MATH) && !defined(__CPU_HAS_VFP)
#error	__USE_VFP_MATH was defined but not __CPU_HAS_VFP - impossible combination, check variant.mmh 
#endif


#ifndef __USE_VFP_MATH

LOCAL_D const TUint32 ArtanhCoeffs[] =
	{
	0x5C17F0BC,0xB8AA3B29,0x80010000,	// polynomial approximation to (4/ln2)artanh(x)
	0xD01FDDD8,0xF6384EE1,0x7FFF0000,	// for |x| <= (sqr2-1)/(sqr2+1)
	0x7D0DDC69,0x93BB6287,0x7FFF0000,
	0x6564D4F5,0xD30BB153,0x7FFE0000,
	0x1546C858,0xA4258A33,0x7FFE0000,
	0xCCE50DA9,0x864D28DF,0x7FFE0000,
	0x8E1A5DBB,0xE35271A0,0x7FFD0000,
	0xF5A67D92,0xC3A36B08,0x7FFD0000,
	0x62D53E02,0xC4A1FFAC,0x7FFD0000
	};

LOCAL_D const TUint32 TwoToxCoeffs[] =
	{
	0x00000000,0x80000000,0x7FFF0000,	// polynomial approximation to 2^(x/8) for
	0xD1CF79AC,0xB17217F7,0x7FFB0000,	// 0<=x<=1
	0x162CF72B,0xF5FDEFFC,0x7FF60000,
	0x23EC0D04,0xE35846B8,0x7FF10000,
	0xBDB408D7,0x9D955B7E,0x7FEC0000,
	0xFDD8A678,0xAEC3FE73,0x7FE60000,
	0xBD6E3950,0xA184E90A,0x7FE00000,
	0xC1054DA3,0xFFB259D8,0x7FD90000,
	0x70893DE4,0xB8BEDE2F,0x7FD30000
	};

LOCAL_D const TUint32 TwoToNover8[] =
	{
	0xEA8BD6E7,0x8B95C1E3,0x7FFF0000,	// 2^0.125
	0x8DB8A96F,0x9837F051,0x7FFF0000,	// 2^0.250
	0xB15138EA,0xA5FED6A9,0x7FFF0000,	// 2^0.375
	0xF9DE6484,0xB504F333,0x7FFF0000,	// 2^0.500
	0x5506DADD,0xC5672A11,0x7FFF0000,	// 2^0.625
	0xD69D6AF4,0xD744FCCA,0x7FFF0000,	// 2^0.750
	0xDD24392F,0xEAC0C6E7,0x7FFF0000	// 2^0.875
	};

LOCAL_D const TUint32 Sqr2data[] = {0xF9DE6484,0xB504F333,0x7FFF0000};		// sqr2
LOCAL_D const TUint32 Sqr2Invdata[] = {0xF9DE6484,0xB504F333,0x7FFE0000};	// 1/sqr2
LOCAL_D const TUint32 Onedata[] = {0x00000000,0x80000000,0x7FFF0000};		// 1.0

LOCAL_C void Log2(TRealX& y, TRealX& x)
	{
	// Calculate log2(x) and write to y
	// Result to 64-bit precision to allow accurate powers
	// Algorithm:
	//		log2(aSrc)=log2(2^e.m) e=exponent of aSrc, m=mantissa 1<=m<2
	//		log2(aSrc)=e+log2(m)
	//		If e=-1 (0.5<=aSrc<1), let x=aSrc else let x=mantissa(aSrc)
	//		If x>Sqr2, replace x with x/Sqr2
	//		If x<Sqr2/2, replace x with x*Sqr2
	//		Replace x with (x-1)/(x+1)
	//		Use polynomial to calculate artanh(x) for |x| <= (sqr2-1)/(sqr2+1)
	//			( use identity ln(x) = 2artanh((x-1)/(x+1)) )

	const TRealX& Sqr2=*(const TRealX*)Sqr2data;
	const TRealX& Sqr2Inv=*(const TRealX*)Sqr2Invdata;
	const TRealX& One=*(const TRealX*)Onedata;

	TInt n=(x.iExp-0x7FFF)<<1;
	x.iExp=0x7FFF;
	if (n!=-2)
		{
		if (x>Sqr2)
			{
			x*=Sqr2Inv;
			n++;
			}
		}
	else 
		{
		n=0;
		x.iExp=0x7FFE;
		if (x<Sqr2Inv)
			{
			x*=Sqr2;
			n--;
			}
		}
	x=(x-One)/(x+One);	// ln(x)=2artanh((x-1)/(x+1))
	Math::PolyX(y,x*x,8,(const TRealX*)ArtanhCoeffs);
	y*=x;
	y+=TRealX(n);
	if (y.iExp>1)
		y.iExp--;
	else
		y.iExp=0;
	}

LOCAL_C TInt TwoTox(TRealX& y, TRealX& x)
	{
	// Calculate 2^x and write result to y. Result to 64 bit precision.
	// Algorithm:
	//		2^x = 2^int(x).2^frac(x)
	//		2^int(x) just adds int(x) to the final result exponent
	//		Reduce frac(x) to the range [0,0.125] (modulo 0.125)
	//		Use polynomial to calculate 2^x for 0<=x<=0.125
	//		Multiply by 2^(n/8) for n=0,1,2,3,4,5,6,7 to give 2^frac(x)

	if (x.iExp)
		x.iExp+=3;
	TInt n=(TInt)x;
	if (n<16384 && n>-16384)
		{
		if (x.iSign&1)
			n--;
		x-=TRealX(n);
		Math::PolyX(y,x,8,(const TRealX*)TwoToxCoeffs);
		y.iExp=TUint16(TInt(y.iExp)+(n>>3));
		n&=7;
		if (n)
			y*= (*(const TRealX*)(TwoToNover8+3*n-3));
		return KErrNone;
		}
	else
		{
		if (n<0)
			{
			y.SetZero();
			return KErrUnderflow;
			}
		else
			{
			y.SetInfinite(0);
			return KErrOverflow;
			}
		}
	}




EXPORT_C TInt Math::Pow(TReal &aTrg,const TReal &aSrc,const TReal &aPower)
/**
Calculates the value of x raised to the power of y.

The behaviour conforms to that specified for pow() in the
ISO C Standard ISO/IEC 9899 (Annex F), although floating-point exceptions
are not supported.

@param aTrg   A reference containing the result.
@param aSrc   The x argument of the function.
@param aPower The y argument of the function.

@return KErrNone if successful;
		KErrOverflow if the result is +/- infinity;
	   	KErrUnderflow if the result is too small to be represented;
		KErrArgument if the result is not a number (NaN).
*/
//
// Evaluates aSrc raised to the power aPower and places the result in aTrg.
// For non-special values algorithm is aTrg=2^(aPower*log2(aSrc))
//
	{
	TRealX x,p;

	TInt ret2=p.Set(aPower);
	// pow(x, +/-0) -> 1 for any x, even a NaN
	if (p.IsZero())
		{
		aTrg=1.0;
		return KErrNone;
		}

	TInt ret1=x.Set(aSrc);
	if (ret1==KErrArgument || ret2==KErrArgument)
		{
		// pow(+1, y) -> 1 for any y, even a NaN
		// XXX First test should not be necessary, but on WINS
		//     aSrc == 1.0 is true when aSrc is NaN.
		if (ret1 != KErrArgument && aSrc == 1.0)
			{
			aTrg=aSrc;
			return KErrNone;
			}
		SetNaN(aTrg);
		return KErrArgument;
		}

	// Infinite power
	if (ret2==KErrOverflow)
		{
		// figure out which of these cases we have:
		//
		// pow(x, -INF) -> +INF for |x| < 1  } flag = 0
		// pow(x, +INF) -> +INF for |x| > 1  }
		// pow(x, -INF) -> +0 for |x| > 1      } flag = 1
		// pow(x, +INF) -> +0 for |x| < 1      }
		//
		// flag = 2 => |x| == 1.0
		//
		TInt flag=2;
		if (Abs(aSrc)>1.0)
			flag=p.iSign&1;
		if (Abs(aSrc)<1.0)
			flag=1-(p.iSign&1);
		if (flag==0)
			{
			SetInfinite(aTrg,0);
			return KErrOverflow;
			}
		if (flag==1)
			{
			SetZero(aTrg,0);
			return KErrNone;
			}
		if (Abs(aSrc)==1.0)
			{
			// pow(-1, +/-INF) -> 1
			aTrg=1.0;
			return KErrNone;
			}
		// This should never happen (i.e. aSrc is NaN, which
		// should be taken care of above)
		SetNaN(aTrg);
		return KErrArgument;
		}

	// Negative Base raised to a power
	TInt odd=1;
	if (x.iSign & 1)
		{
		TReal pint;
		Math::Int(pint,aPower);
		if (aPower-pint) // Checks that if aSrc is less than zero, then aPower is integral
			{
			// pow(-INF, y) -> +0 for y < 0 and not an odd integer
			// pow(-INF, y) -> +INF for y > 0 and not an odd integer
			// Since we're here, aPower is not integral, so can't be odd, either
			if (ret1 == KErrOverflow)
				{
				if (aPower < 0)
					{
					SetZero(aTrg);
					return KErrNone;
					}
				else
					{
					SetInfinite(aTrg,0);
					return KErrOverflow;
					}
				}
			SetNaN(aTrg);
			return KErrArgument;
			}
		TReal powerby2=aPower*0.5;
		Math::Int(pint,powerby2);
		if (powerby2-pint)
			odd=(-1);
		x.iSign=0;
		}

	// Zero or infinity raised to a power
	if (x.IsZero() || ret1==KErrOverflow)
		{
		if (x.IsZero() && p.IsZero())
			{
			aTrg=1.0;
			return KErrNone;
			}
		TInt sign=(odd==-1 ? 1 : 0);
		if ((x.IsZero() && (p.iSign&1)==0) || (ret1==KErrOverflow && (p.iSign&1)))
			{
			SetZero(aTrg,sign);				
			return KErrNone;
			}
		else
			{
			SetInfinite(aTrg,sign);
			return KErrOverflow;
			}
		}

	TRealX y;
	Log2(y,x);
	x=y*p;			// this cannot overflow or underflow
	TInt r=TwoTox(y,x);
	if (odd<0)
		y.iSign=1;
	TInt r2=y.GetTReal(aTrg);
	return (r==KErrNone)?r2:r;
	}

#else // __USE_VFP_MATH

// definitions come from RVCT math library
extern "C" TReal pow(TReal,TReal);

EXPORT_C TInt Math::Pow(TReal &aTrg,const TReal &aSrc,const TReal &aPower)
	{
	aTrg = pow(aSrc,aPower);
	if (Math::IsZero(aTrg) && !Math::IsZero(aSrc) && !Math::IsInfinite(aSrc) && !Math::IsInfinite(aPower))
		return KErrUnderflow;
	if (Math::IsFinite(aTrg))
		return KErrNone;
	if (Math::IsZero(aPower))	// pow(x, +/-0) -> 1 for any x, even a NaN
		{
		aTrg = 1.0;
		return KErrNone;
		}
	if (Math::IsInfinite(aTrg))
		return KErrOverflow;
	if (aSrc==1.0)				// pow(+1, y) -> 1 for any y, even a NaN
		{
		aTrg=aSrc;
		return KErrNone;
		}
	if (Math::IsInfinite(aPower))
		{
		if (aSrc == -1.0)		// pow(-1, +/-INF) -> 1
			{
			aTrg = 1.0;
			return KErrNone;
			}
		if (((Abs(aSrc) < 1) && (aPower < 0)) ||	// pow(x, -INF) -> +INF for |x| < 1
		    ((Abs(aSrc) > 1) && (aPower > 0)))		// pow(x, +INF) -> +INF for |x| > 1
			{
			SetInfinite(aTrg,0);
			return KErrOverflow;
			}
		}
	// pow(-INF, y) -> +INF for y > 0 and not an odd integer
	if (Math::IsInfinite(aSrc) && (aSrc < 0) && (aPower > 0))
		{
		TBool odd = EFalse;
		TReal pint;
		Math::Int(pint, aPower);
		if (aPower == pint)
			{
			TReal halfPower = aPower * 0.5;
			Math::Int(pint, halfPower);
			if (halfPower != pint)
				odd = ETrue;
			}
		if (odd == EFalse)
			{
			SetInfinite(aTrg,0);
			return KErrOverflow;
			}
		}

	// Otherwise...
	SetNaN(aTrg);
	return KErrArgument;
	}

#endif
