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
// e32\euser\maths\um_tan.cpp
// Tangent.
// 
//

#include "um_std.h"

#if defined(__USE_VFP_MATH) && !defined(__CPU_HAS_VFP)
#error	__USE_VFP_MATH was defined but not __CPU_HAS_VFP - impossible combination, check variant.mmh 
#endif


#ifndef __USE_VFP_MATH

LOCAL_D const TUint32 TanCoeffs[] =
	{
	0x2168C235,0xC90FDAA2,0x7FFF0000,	// polynomial approximation to tan((pi/2)*x)
	0x2DF4707D,0xA55DE731,0x7FFF0000,	// for |x|<=0.25
	0xA9A1A71A,0xA335E33B,0x7FFF0000,
	0x0BB9E431,0xA2FFFCDD,0x7FFF0000,
	0x3E523A39,0xA2FA3863,0x7FFF0000,
	0x8A35C401,0xA2F9D38B,0x7FFF0000,
	0x91269411,0xA2F16003,0x7FFF0000,
	0xDA32CC78,0xA3A93B13,0x7FFF0000,
	0x4FB88317,0x9A146197,0x7FFF0000,
	0x0D787ECE,0xE131DEE5,0x7FFF0000
	};

LOCAL_D const TUint32 Onedata[] = {0x00000000,0x80000000,0x7FFF0000};		// 1.0
LOCAL_D const TUint32 Halfdata[] = {0x00000000,0x80000000,0x7FFE0000};		// 0.5
LOCAL_D const TUint32 PiBy2Invdata[] = {0x4E44152A,0xA2F9836E,0x7FFE0000};	// 2/pi




EXPORT_C TInt Math::Tan(TReal& aTrg, const TReal& aSrc)
/**
Calculates the tangent of a number.

@param aTrg A reference containing the result.
@param aSrc The argument of the tan function in radians. 

@return KErrNone if successful, otherwise another of
        the system-wide error codes. 
*/
	{
	// Calculate tan(aSrc) and write result to aTrg.
	// Algorithm:
	//		Let x=aSrc/(pi/2). Throw away integer part, but if integer part odd
	//			then replace final result y with -1/y
	//			( use identities tan(x+n*pi)=tan(x), tan(x+pi/2)=-1/tan(x) )
	//		Replace x with fractional part after division.
	//		If x>=0.5, replace x with 1-x and replace result y with 1/y
	//			( use identity tan(pi/2-x)=1/tan(x) )
	//		If x>=0.25, replace x with 0.5-x and replace result y with (1-y)/(1+y)
	//			( use identity tan(pi/4-x)=(1-tan(x))/(1+tan(x)) )
	//		Use polynomial approximation to calculate tan(pi*x/2) for |x|<=0.25

	const TRealX& One = *(const TRealX*)Onedata;
	const TRealX& Half = *(const TRealX*)Halfdata;
	const TRealX& PiBy2Inv = *(const TRealX*)PiBy2Invdata;

	TRealX x;
	TInt r=x.Set(aSrc);
	if (r==KErrNone)
		{
		TInt8 sign=x.iSign;
		x.iSign=0;
		x*=PiBy2Inv;
		TInt n=(TInt)x;
		if (n<KMaxTInt && n>KMinTInt)
			{
			TInt flags=(n&1)<<1;
			x-=TRealX(n);
			if (x.iExp>=0x7FFE)
				{
				x=One-x;
				flags^=2;
				}
			if (x.iExp>=0x7FFD)
				{
				x=Half-x;
				flags^=1;
				}
			TRealX y;
			PolyX(y,x*x,9,(const TRealX*)TanCoeffs);
			y*=x;
			if (flags==3)
				y=(One+y)/(One-y);
			else if (flags==2)
				y=One/y;
			else if (flags==1)
				y=(One-y)/(One+y);
			y.iSign=TInt8(sign ^ (n&1));
			return y.GetTReal(aTrg);
			}
		}
	SetNaN(aTrg);
	return KErrArgument;
	}

#else // __USE_VFP_MATH

// definitions come from RVCT math library
extern "C" TReal tan(TReal);

EXPORT_C TInt Math::Tan(TReal& aTrg, const TReal& aSrc)
	{
	if (aSrc<KMaxTInt && aSrc>KMinTInt)
		{
		aTrg = tan(aSrc);
		if (Math::IsFinite(aTrg))
			return KErrNone;
		}
	SetNaN(aTrg);
	return KErrArgument;
	}

#endif
