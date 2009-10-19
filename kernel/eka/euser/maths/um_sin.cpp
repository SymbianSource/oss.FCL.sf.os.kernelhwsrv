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
// e32\euser\maths\um_sin.cpp
// Floating point sine and cosine functions
// 
//

#include "um_std.h"

#if defined(__USE_VFP_MATH) && !defined(__CPU_HAS_VFP)
#error	__USE_VFP_MATH was defined but not __CPU_HAS_VFP - impossible combination, check variant.mmh 
#endif


#ifndef __USE_VFP_MATH

LOCAL_D const TUint32 SinCoeffs[] =
	{
	0x2168C235,0xC90FDAA2,0x80000000,	// polynomial approximation to sin(pi*x)
	0x2DF200BF,0xA55DE731,0x80010001,	// for |x| <= 0.25
	0xAC273AA1,0xA335E33B,0x80000000,
	0x5AB23F44,0x99696671,0x7FFE0001,
	0xD585EAFE,0xA83C17D9,0x7FFB0000,
	0xA30DE7AD,0xF1802BAC,0x7FF70001,
	0xF57FD821,0xF1F6A1C9,0x7FF30000
	};

LOCAL_D const TUint32 CosCoeffs[] =
	{
	0x00000000,0x80000000,0x7FFF0000,	// polynomial approximation to cos(pi*x)
	0xF22EF286,0x9DE9E64D,0x80010001,	// for |x| <= 0.25
	0xDAD59F90,0x81E0F840,0x80010000,
	0xE4E45144,0xAAE9E3F1,0x7FFF0001,
	0x3232D733,0xF0FA8342,0x7FFC0000,
	0x03E16BB8,0xD368F6A3,0x7FF90001,
	0x712FD084,0xFCE66DE2,0x7FF50000,
	0x9E5353EE,0xD94951B0,0x7FF10001
	};

LOCAL_D const TUint32 PiInvdata[] = {0x4E44152A,0xA2F9836E,0x7FFD0000};			// 1/pi
LOCAL_D const TUint32 Halfdata[] = {0x00000000,0x80000000,0x7FFE0000};			// 0.5
LOCAL_D const TUint32 Onedata[] = {0x00000000,0x80000000,0x7FFF0000};			// 1.0

LOCAL_C TInt CalcSinCos(TReal& aTrg, TRealX& aSrc, TBool aCos)
	{
	// Calculate sin(aSrc) if aCos=false or cos(aSrc) if aCos=true
	// and write result to aTrg.
	// Algorithm:
	//		Divide aSrc by pi and throw away integer part, but change sign
	//			of result if integer part odd. Replace aSrc with remainder.
	//			( use identities	sin(x+n*pi)=(-1)^n*sin(x)
	//								cos(x+n*pi)=(-1)^n*cos(x) )
	//		If aSrc>=0.5 replace aSrc with 1-aSrc, and change sign of result
	//			if cos required.
	//			( use identities sin(pi-x)=sin(x), cos(pi-x)=-cos(x) )
	//		If aSrc>=0.25 replace aSrc with 0.5-aSrc and swap sin and cos
	//			( use identities sin(pi/2-x)=cos(x), cos(pi/2-x)=sin(x) )
	//		Use polynomial approximation to evaluate sin(pi*x) or cos(pi*x)
	//		for |x|<=0.25

	const TRealX& One = *(const TRealX*)Onedata;
	const TRealX& Half = *(const TRealX*)Halfdata;
	const TRealX& PiInv = *(const TRealX*)PiInvdata;

	TRealX y;
	aSrc*=PiInv;
	TInt n=(TInt)aSrc;
	if (n<KMaxTInt && n>KMinTInt)
		{
		aSrc-=TRealX(n);
		TInt sign=0;
		if (!aCos)
			sign=aSrc.iSign & 1;
		sign^=n;
		aSrc.iSign=0;
		if (aSrc.iExp>=0x7FFE)			// if remainder>=pi/2
			{
			aSrc=One-aSrc;
			if (aCos)
				sign^=1;
			}
		if (aSrc.iExp>=0x7FFD)			// if remainder>=pi/4
			{
			aSrc=Half-aSrc;				// take complementary angle
			aCos=!aCos;					// and swap sin and cos
			}
		if (aCos)
			Math::PolyX(y,aSrc*aSrc,7,(const TRealX*)CosCoeffs);
		else
			{
			Math::PolyX(y,aSrc*aSrc,6,(const TRealX*)SinCoeffs);
			y*=aSrc;
			}
		if (sign & 1)
			y=-y;
		return y.GetTReal(aTrg);
		}
	return KErrArgument;
	}




EXPORT_C TInt Math::Sin(TReal& aTrg, const TReal& aSrc)
/**
Calculates the sine of a number.

@param aTrg A reference containing the result. 
@param aSrc The argument of the sin function in radians.

@return KErrNone if successful, otherwise another of
        the system-wide error codes.
*/
	{
	TRealX x;
	TInt r=x.Set(aSrc);
	if (r==KErrNone)
		r=CalcSinCos(aTrg,x,EFalse);
	if (r==KErrNone)
		return r;
	SetNaN(aTrg);
	return KErrArgument;
	}




EXPORT_C TInt Math::Cos(TReal& aTrg, const TReal& aSrc)
/**
Calculates the cosine of a number.

@param aTrg A reference containing the result. 
@param aSrc The argument of the cos function in radians

@return KErrNone if successful, otherwise another of
        the system-wide error codes.
*/
	{
	TRealX x;
	TInt r=x.Set(aSrc);
	if (r==KErrNone)
		r=CalcSinCos(aTrg,x,ETrue);
	if (r==KErrNone)
		return r;
	SetNaN(aTrg);
	return KErrArgument;
	}

#else // __USE_VFP_MATH

// definitions come from RVCT math library
extern "C" TReal sin(TReal);
extern "C" TReal cos(TReal);

EXPORT_C TInt Math::Sin(TReal& aTrg, const TReal& aSrc)
	{
	if (aSrc<KMaxTInt && aSrc>KMinTInt)
		{
		aTrg = sin(aSrc);
		if (Math::IsFinite(aTrg))
			return KErrNone;
		}
	SetNaN(aTrg);
	return KErrArgument;
	}

EXPORT_C TInt Math::Cos(TReal& aTrg, const TReal& aSrc)
	{
	if (aSrc<KMaxTInt && aSrc>KMinTInt)
		{
		aTrg = cos(aSrc);
		if (Math::IsFinite(aTrg))
			return KErrNone;
		}
	SetNaN(aTrg);
	return KErrArgument;
	}

#endif
