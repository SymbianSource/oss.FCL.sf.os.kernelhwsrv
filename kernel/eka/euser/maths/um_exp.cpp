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
// e32\euser\maths\um_exp.cpp
// Floating point exponentiation
// 
//

#include "um_std.h"

#if defined(__USE_VFP_MATH) && !defined(__CPU_HAS_VFP)
#error	__USE_VFP_MATH was defined but not __CPU_HAS_VFP - impossible combination, check variant.mmh 
#endif

#ifndef __USE_VFP_MATH

LOCAL_D const TUint32 ExpCoeffs[] =
	{
	0x00000000,0x80000000,0x7FFF0000,	// polynomial approximation to 2^(x/8)
	0xD1CF79AC,0xB17217F7,0x7FFB0000,	// for 0<=x<=1
	0x1591EF2B,0xF5FDEFFC,0x7FF60000,
	0x23B940A9,0xE35846B9,0x7FF10000,
	0xDD73C23F,0x9D955ADE,0x7FEC0000,
	0x8728EBE7,0xAEC4616C,0x7FE60000,
	0xAF177130,0xA1646F7D,0x7FE00000,
	0xC44EAC22,0x8542C46E,0x7FDA0000
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

LOCAL_D const TUint32 EightLog2edata[] = {0x5C17F0BC,0xB8AA3B29,0x80020000};	// 8/ln2




EXPORT_C TInt Math::Exp(TReal& aTrg, const TReal& aSrc)
/**
Calculates the value of e to the power of x.

@param aTrg A reference containing the result. 
@param aSrc The power to which e is to be raised. 

@return KErrNone if successful, otherwise another of
        the system-wide error codes. 
*/	
	{
	// Calculate exp(aSrc) and write result to aTrg
	// Algorithm:
	//		Let x=aSrc/ln2 and calculate 2^x
	//		2^x = 2^int(x).2^frac(x)
	//		2^int(x) just adds int(x) to the final result exponent
	//		Reduce frac(x) to the range [0,0.125] (modulo 0.125)
	//		Use polynomial to calculate 2^x for 0<=x<=0.125
	//		Multiply by 2^(n/8) for n=0,1,2,3,4,5,6,7 to give 2^frac(x)

	const TRealX& EightLog2e=*(const TRealX*)EightLog2edata;

	TRealX x;
	TRealX y;
	TInt r=x.Set(aSrc);
	if (r==KErrNone)
		{
		x*=EightLog2e;
		TInt n=(TInt)x;
		if (n<16384 && n>-16384)
			{
			if (x.iSign&1)
				n--;
			x-=TRealX(n);
			PolyX(y,x,7,(const TRealX*)ExpCoeffs);
			y.iExp=TUint16(TInt(y.iExp)+(n>>3));
			n&=7;
			if (n)
				y*= (*(const TRealX*)(TwoToNover8+3*n-3));
			return y.GetTReal(aTrg);
			}
		else
			{
			if (n<0)
				{
				SetZero(aTrg);
				r=KErrUnderflow;
				}
			else
				{
				SetInfinite(aTrg,0);
				r=KErrOverflow;
				}
			return r;
			}
		}
	else
		{
		if (r==KErrArgument)
			SetNaN(aTrg);
		if (r==KErrOverflow)
			{
			if (x.iSign&1)
				{
				SetZero(aTrg);
				r=KErrUnderflow;
				}
			else
				{
				SetInfinite(aTrg,0);
				}
			}
		return r;
		}
	}

#else // __USE_VFP_MATH

// definitions come from RVCT math library
extern "C" TReal exp(TReal);

EXPORT_C TInt Math::Exp(TReal& aTrg, const TReal& aSrc)
	{
	aTrg = exp(aSrc);
	if (Math::IsZero(aTrg))
		return KErrUnderflow;
	if (Math::IsFinite(aTrg))
		return KErrNone;
	if (Math::IsInfinite(aTrg))
		return KErrOverflow;
	SetNaN(aTrg);
	return KErrArgument;
	}

#endif
