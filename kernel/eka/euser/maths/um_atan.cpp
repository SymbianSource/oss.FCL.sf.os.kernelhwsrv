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
// e32\euser\maths\um_atan.cpp
// Floating point arc tangent
// 
//

#include "um_std.h"

#if defined(__USE_VFP_MATH) && !defined(__CPU_HAS_VFP)
#error	__USE_VFP_MATH was defined but not __CPU_HAS_VFP - impossible combination, check variant.mmh 
#endif

#ifndef __USE_VFP_MATH

LOCAL_D const TUint32 ArctanCoeffs[] =
	{
	0x00000000,0x80000000,0x7FFF0000,	// polynomial approximation to arctan(x)
	0xAA84D6EE,0xAAAAAAAA,0x7FFD0001,	// for -(sqr2-1) <= x <= (sqr2-1)
	0x89C77453,0xCCCCCCCC,0x7FFC0000,
	0xEBC0261C,0x9249247B,0x7FFC0001,
	0x940BC4DB,0xE38E3121,0x7FFB0000,
	0x141C32F1,0xBA2DBF36,0x7FFB0001,
	0xA90615E7,0x9D7C807E,0x7FFB0000,
	0x1C632E93,0x87F6A873,0x7FFB0001,
	0x310FCFFD,0xE8BE5D0A,0x7FFA0000,
	0x92289F15,0xB17B930B,0x7FFA0001,
	0x546FE7CE,0xABDE562D,0x7FF90000
	};

LOCAL_D const TUint32 Sqr2m1data[] = {0xE7799211,0xD413CCCF,0x7FFD0000};		// sqr2-1
LOCAL_D const TUint32 Sqr2p1data[] = {0xFCEF3242,0x9A827999,0x80000000};		// sqr2+1
LOCAL_D const TUint32 Onedata[] = {0x00000000,0x80000000,0x7FFF0000};			// 1.0
LOCAL_D const TUint32 PiBy8data[] = {0x2168C235,0xC90FDAA2,0x7FFD0000};			// pi/8
LOCAL_D const TUint32 PiBy2data[] = {0x2168C235,0xC90FDAA2,0x7FFF0000};			// pi/2
LOCAL_D const TUint32 ThreePiBy8data[] = {0x990E91A8,0x96CBE3F9,0x7FFF0000};	// 3*pi/8

LOCAL_C void Arctan(TRealX& y, TRealX& x)
	{
	// Calculate arctan(x), write result to y
	// Algorithm:
	//		If x>1, replace x with 1/x and subtract result from pi/2
	//			( use identity tan(pi/2-x)=1/tan(x) )
	//		If x>sqr(2)-1, replace x with (x-(sqr(2)-1))/(1-(sqr2-1)x)
	//			( use identity tan(x-a)=(tanx-tana)/(1-tana.tanx)
	//			  where a=pi/8, tan a = sqr2-1
	//			and add pi/8 to result
	//		Use polynomial approximation to calculate arctan(x) for
	//		x in the interval [0,sqr2-1]

	const TRealX& Sqr2m1 = *(const TRealX*)Sqr2m1data;
	const TRealX& Sqr2p1 = *(const TRealX*)Sqr2p1data;
	const TRealX& One = *(const TRealX*)Onedata;
	const TRealX& PiBy8 = *(const TRealX*)PiBy8data;
	const TRealX& PiBy2 = *(const TRealX*)PiBy2data;
	const TRealX& ThreePiBy8 = *(const TRealX*)ThreePiBy8data;

	TInt section=0;
	TInt8 sign=x.iSign;
	x.iSign=0;
	if (x>Sqr2p1)
		{
		x=One/x;
		section=3;
		}
	else if (x>One)
		{
		x=(One-Sqr2m1*x)/(x+Sqr2m1);
		section=2;
		}
	else if (x>Sqr2m1)
		{
		x=(x-Sqr2m1)/(One+Sqr2m1*x);
		section=1;
		}
	Math::PolyX(y,x*x,10,(const TRealX*)ArctanCoeffs);
	y*=x;
	if (section==1)
		y+=PiBy8;
	else if (section==2)
		y=ThreePiBy8-y;
	else if (section==3)
		y=PiBy2-y;
	y.iSign=sign;
	}




EXPORT_C TInt Math::ATan(TReal& aTrg, const TReal& aSrc)
/**
Calculates the principal value of the inverse tangent of a number.

@param aTrg A reference containing the result in radians,
            a value between -pi/2 and +pi/2.
@param aSrc The argument of the arctan function,
            a value between +infinity and +infinity.

@return KErrNone if successful, otherwise another of
        the system-wide error codes. 
*/
	{
	TRealX x;
	TInt r=x.Set(aSrc);
	if (r==KErrNone)
		{
		TRealX y;
		Arctan(y,x);
		return y.GetTReal(aTrg);
		}
	if (r==KErrArgument)
		{
		SetNaN(aTrg);
		return KErrArgument;
		}
	aTrg=KPiBy2;		// arctan(+/- infinity) = +/- pi/2
	if (x.iSign&1)
		aTrg=-aTrg;
	return KErrNone;
	}

LOCAL_D const TUint32 Pidata[] = {0x2168C235,0xC90FDAA2,0x80000000};
LOCAL_D const TUint32 PiBy4data[] = {0x2168C235,0xC90FDAA2,0x7FFE0000};
LOCAL_D const TUint32 MinusPiBy4data[] = {0x2168C235,0xC90FDAA2,0x7FFE0001};
LOCAL_D const TUint32 ThreePiBy4data[] = {0x990E91A8,0x96CBE3F9,0x80000000};
LOCAL_D const TUint32 MinusThreePiBy4data[] = {0x990E91A8,0x96CBE3F9,0x80000001};
LOCAL_D const TUint32 Zerodata[] = {0x00000000,0x00000000,0x00000000};




EXPORT_C TInt Math::ATan(TReal &aTrg,const TReal &aY,const TReal &aX)
/**
Calculates the angle between the x-axis and a line drawn from the origin
to a point represented by its (x,y) co-ordinates.

The co-ordinates are passed as arguments to the function.
This function returns the same result as arctan(y/x), but:

1. it adds +/-pi to the result, if x is negative

2. it sets the result to +/-pi/2, if x is zero but y is non-zero.

@param aTrg A reference containing the result in radians,
            a value between -pi exclusive and +pi inclusive.
@param aY   The y argument of the arctan(y/x) function. 
@param aX   The x argument of the arctan(y/x) function.

@return KErrNone if successful, otherwise another of
        the system-wide error codes. 
*/
	{
	const TRealX& Zero=*(const TRealX*)Zerodata;
	const TRealX& Pi=*(const TRealX*)Pidata;
	const TRealX& PiBy4=*(const TRealX*)PiBy4data;
	const TRealX& MinusPiBy4=*(const TRealX*)MinusPiBy4data;
	const TRealX& ThreePiBy4=*(const TRealX*)ThreePiBy4data;
	const TRealX& MinusThreePiBy4=*(const TRealX*)MinusThreePiBy4data;

	TRealX x, y;
	TInt rx=x.Set(aX);
	TInt ry=y.Set(aY);
	if (rx!=KErrArgument && ry!=KErrArgument)
		{
		if (x.iExp==0)
			x.iSign=0;
		TRealX q;
		TInt rq=y.Div(q,x);
		if (rq!=KErrArgument)
			{
			TRealX arg;
			Arctan(arg,q);
			if (x<Zero)
				{
				if (y>=Zero)
					arg+=Pi;
				else
					arg-=Pi;
				}
			aTrg=arg;
			return KErrNone;
			}
		if (!x.IsZero())
			{
			// Both x and y must be infinite
			TInt quadrant=((y.iSign & 1)<<1) + (x.iSign&1);
			TRealX arg;
			if (quadrant==0)
				arg=PiBy4;
			else if (quadrant==1)
				arg=ThreePiBy4;
			else if (quadrant==3)
				arg=MinusThreePiBy4;
			else
				arg=MinusPiBy4;
			aTrg=(TReal)arg;
			return KErrNone;
			}
		}
	SetNaN(aTrg);
	return KErrArgument;
	}

#else // __USE_VFP_MATH

LOCAL_D const TUint32 PiBy4data[] = {0x54442D18,0x3FE921FB};
LOCAL_D const TUint32 MinusPiBy4data[] = {0x54442D18,0xBFE921FB};
LOCAL_D const TUint32 ThreePiBy4data[] = {0x7F3321D2,0x4002D97C};
LOCAL_D const TUint32 MinusThreePiBy4data[] = {0x7F3321D2,0xC002D97C};

// definitions come from RVCT math library
extern "C" TReal atan(TReal);
extern "C" TReal atan2(TReal,TReal);

EXPORT_C TInt Math::ATan(TReal& aTrg, const TReal& aSrc)
	{
	aTrg = atan(aSrc);
	if (Math::IsFinite(aTrg))
		return KErrNone;
	SetNaN(aTrg);
	return KErrArgument;
	}

EXPORT_C TInt Math::ATan(TReal &aTrg,const TReal &aY,const TReal &aX)
	{
	aTrg = atan2(aY,aX);
	if (Math::IsFinite(aTrg))
		return KErrNone;
	
	// Return is a NaN, but ARM implementation returns NaN for atan(inf/inf)
	// whereas implementation above returns multiples of pi/4 - fix up here
	SReal64 *pY=(SReal64 *)&aY;
	SReal64 *pX=(SReal64 *)&aX;
	
	if (   pY->msm==0 && pY->lsm==0 && pY->exp==KTReal64SpecialExponent
		&& pX->msm==0 && pX->lsm==0 && pX->exp==KTReal64SpecialExponent)
		{
		TInt quadrant=((pY->sign)<<1) + (pX->sign);
		if (quadrant==0)
			aTrg=*(const TReal*)PiBy4data;
		else if (quadrant==1)
			aTrg=*(const TReal*)ThreePiBy4data;
		else if (quadrant==3)
			aTrg=*(const TReal*)MinusThreePiBy4data;
		else
			aTrg=*(const TReal*)MinusPiBy4data;
		return KErrNone;
		}

	// If we get here then the args weren't inf/inf so one of them must've
	// been a NaN to start with
	SetNaN(aTrg);
	return KErrArgument;
	}

#endif
