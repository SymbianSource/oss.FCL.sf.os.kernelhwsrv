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
// e32\euser\maths\um_ln.cpp
// Natural log.
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
	0xD02489EE,0xF6384EE1,0x7FFF0000,	// for |x| <= (sqr2-1)/(sqr2+1)
	0x7008CA5F,0x93BB6287,0x7FFF0000,
	0xE32D1D6B,0xD30BB16D,0x7FFE0000,
	0x461D071E,0xA4257CE2,0x7FFE0000,
	0xC3B0EC87,0x8650D459,0x7FFE0000,
	0x53BEC0CD,0xE23137E3,0x7FFD0000,
	0xC523F21B,0xDAF79221,0x7FFD0000
	};

LOCAL_D const TUint32 Ln2By2data[] = {0xD1CF79AC,0xB17217F7,0x7FFD0000};	// (ln2)/2
LOCAL_D const TUint32 Sqr2data[] = {0xF9DE6484,0xB504F333,0x7FFF0000};		// sqr2
LOCAL_D const TUint32 Sqr2Invdata[] = {0xF9DE6484,0xB504F333,0x7FFE0000};	// 1/sqr2
LOCAL_D const TUint32 Onedata[] = {0x00000000,0x80000000,0x7FFF0000};		// 1.0




EXPORT_C TInt Math::Ln(TReal& aTrg, const TReal& aSrc)
/**
Calculates the natural logarithm of a number.

@param aTrg A reference containing the result. 
@param aSrc The number whose natural logarithm is required.

@return KErrNone if successful, otherwise another of
        the system-wide error codes. 
*/
	{
	// Calculate ln(aSrc) and write to aTrg
	// Algorithm:
	//		Calculate log2(aSrc) and multiply by ln2
	//		log2(aSrc)=log2(2^e.m) e=exponent of aSrc, m=mantissa 1<=m<2
	//		log2(aSrc)=e+log2(m)
	//		If e=-1 (0.5<=aSrc<1), let x=aSrc else let x=mantissa(aSrc)
	//		If x>Sqr2, replace x with x/Sqr2
	//		If x<Sqr2/2, replace x with x*Sqr2
	//		Replace x with (x-1)/(x+1)
	//		Use polynomial to calculate artanh(x) for |x| <= (sqr2-1)/(sqr2+1)
	//			( use identity ln(x) = 2artanh((x-1)/(x+1)) )

	TRealX x;
	const TRealX& Ln2By2=*(const TRealX*)Ln2By2data;
	const TRealX& Sqr2=*(const TRealX*)Sqr2data;
	const TRealX& Sqr2Inv=*(const TRealX*)Sqr2Invdata;
	const TRealX& One=*(const TRealX*)Onedata;

	TInt r=x.Set(aSrc);
	if (r==KErrNone)
		{
		if (x.iExp==0)
			{
			SetInfinite(aTrg,1);
			return KErrOverflow;
			}
		if (x.iSign&1)
			{
			SetNaN(aTrg);
			return KErrArgument;
			}
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
		TRealX y;
		PolyX(y,x*x,7,(const TRealX*)ArtanhCoeffs);
		y*=x;
		y+=TRealX(n);
		y*=Ln2By2;
		return y.GetTReal(aTrg);
		}
	if (r==KErrArgument || (r==KErrOverflow && (x.iSign&1)))
		{
		SetNaN(aTrg);
		return KErrArgument;
		}
	SetInfinite(aTrg,0);
	return KErrOverflow;
	}

#else // __USE_VFP_MATH

// definitions come from RVCT math library
extern "C" TReal log(TReal);

EXPORT_C TInt Math::Ln(TReal& aTrg, const TReal& aSrc)
	{
	aTrg = log(aSrc);
	if (Math::IsFinite(aTrg))
		return KErrNone;
	if (Math::IsInfinite(aTrg))
		return KErrOverflow;
	SetNaN(aTrg);
	return KErrArgument;
	}

#endif
