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
// e32\euser\maths\um_asin.cpp
// Arc sin.
// 
//

#include "um_std.h"

#if defined(__USE_VFP_MATH) && !defined(__CPU_HAS_VFP)
#error	__USE_VFP_MATH was defined but not __CPU_HAS_VFP - impossible combination, check variant.mmh 
#endif

#ifndef __USE_VFP_MATH

LOCAL_D const TUint32 ArcsinCoeffs[] =
	{
	0x00000000,0x80000000,0x7FFF0000,	// polynomial approximation to arcsin(x)
	0xAA893CD8,0xAAAAAAAA,0x7FFC0000,	// for -0.5<=x<=0.5
	0xD07ED410,0x99999999,0x7FFB0000,
	0xB6C64A72,0xB6DB6D94,0x7FFA0000,
	0xF5527DD4,0xF8E3995C,0x7FF90000,
	0xA87499FB,0xB744B969,0x7FF90000,
	0x2E8953AD,0x8E392B24,0x7FF90000,
	0xFEDBB4E4,0xE3481C4A,0x7FF80000,
	0x4A32ED70,0xC89998E1,0x7FF80000,
	0x848A2B53,0xCCAE4AE5,0x7FF70000,
	0x09C1F387,0xA587A043,0x7FF90000,
	0x722B9041,0x8C9BD20B,0x7FF90001,
	0xC88B75CC,0x850CE779,0x7FFA0000
	};

LOCAL_D const TUint32 Onedata[] = {0x00000000,0x80000000,0x7FFF0000};		// 1.0
LOCAL_D const TUint32 Halfdata[] = {0x00000000,0x80000000,0x7FFE0000};		// 0.5
LOCAL_D const TUint32 Pidata[] = {0x2168C235,0xC90FDAA2,0x80000000};		// pi
LOCAL_D const TUint32 PiBy2data[] = {0x2168C235,0xC90FDAA2,0x7FFF0000};		// pi/2

LOCAL_C TInt CalcArcsinArccos(TReal& aTrg, TRealX& x, TBool aCos)
	{
	//	Calculate arcsin (if aCos false) or arccos (if aCos true) of x
	//	and write result to aTrg.
	//	Algorithm (arcsin):
	//		If x>0.5, replace x with Sqrt((1-x)/2)
	//			( use identity cos(x)=2(cos(x/2))^2-1 )
	//		Use polynomial approximation for arcsin(x), 0<=x<=0.5
	//		If original x>0.5, replace result y with pi/2-2y

	const TRealX One = *(const TRealX*)Onedata;
	const TRealX Half = *(const TRealX*)Halfdata;
	const TRealX Pi = *(const TRealX*)Pidata;
	const TRealX PiBy2 = *(const TRealX*)PiBy2data;

	TInt sign=x.iSign&1;
	x.iSign=0;
	if (x<=One)
		{
		TBool big=(x>Half);
		if (big)
			{
			x=One-x;
			if (x.iExp>1)
				x.iExp--;
			TReal temp;
			Math::Sqrt(temp, (TReal)x);
			x=temp;
			}
		TRealX y;
		Math::PolyX(y,x*x,12,(const TRealX*)ArcsinCoeffs);
		y*=x;
		if (big)
			{
			if (y.iExp)
				y.iExp++;
			if (aCos)
				{
				if (sign)
					y=Pi-y;
				}
			else
				{
				y=PiBy2-y;
				y.iSign=TInt8(sign);
				}
			}
		else
			{
			y.iSign=TInt8(sign);
			if (aCos)
				y=PiBy2-y;
			}
		return y.GetTReal(aTrg);
		}
	return KErrArgument;
	}




EXPORT_C TInt Math::ASin(TReal& aTrg, const TReal& aSrc)
/**
Calculates the principal value of the inverse sine of a number.

@param aTrg A reference containing the result in radians,
            a value between -pi/2 and +pi/2. 
@param aSrc The argument of the arcsin function, a value
            between -1 and +1 inclusive.

@return KErrNone if successful, otherwise another of the system-wide
        error codes.
*/
	{
	TRealX x;
	TInt r=x.Set(aSrc);
	if (r==KErrNone)
		r=CalcArcsinArccos(aTrg,x,EFalse);
	if (r==KErrNone)
		return r;
	SetNaN(aTrg);
	return KErrArgument;
	}




EXPORT_C TInt Math::ACos(TReal& aTrg, const TReal& aSrc)
/**
Calculates the principal value of the inverse cosine of a number.

@param aTrg A reference containing the result in radians,
            a value between 0 and pi. 
@param aSrc The argument of the arccos function, a value
            between -1 and +1 inclusive. 

@return KErrNone if successful, otherwise another of the system-wide
        error codes.
*/
	{
	TRealX x;
	TInt r=x.Set(aSrc);
	if (r==KErrNone)
		r=CalcArcsinArccos(aTrg,x,ETrue);
	if (r==KErrNone)
		return r;
	SetNaN(aTrg);
	return KErrArgument;
	}

#else // __USE_VFP_MATH

// definitions come from RVCT math library
extern "C" TReal asin(TReal);
extern "C" TReal acos(TReal);

EXPORT_C TInt Math::ASin(TReal& aTrg, const TReal& aSrc)
	{
	aTrg = asin(aSrc);
	if (Math::IsFinite(aTrg))
		return KErrNone;
	SetNaN(aTrg);
	return KErrArgument;
	}

EXPORT_C TInt Math::ACos(TReal& aTrg, const TReal& aSrc)
	{
	aTrg = acos(aSrc);
	if (Math::IsFinite(aTrg))
		return KErrNone;
	SetNaN(aTrg);
	return KErrArgument;
	}

#endif
