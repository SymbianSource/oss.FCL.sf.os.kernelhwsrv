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
// e32\euser\maths\um_mod.cpp
// Writes the remainder of aSrc/aModulus to aTrg
// 
//

#include "um_std.h"

#if defined(__USE_VFP_MATH) && !defined(__CPU_HAS_VFP)
#error	__USE_VFP_MATH was defined but not __CPU_HAS_VFP - impossible combination, check variant.mmh 
#endif

#ifndef __USE_VFP_MATH

EXPORT_C TInt Math::Mod(TReal &aTrg,const TReal &aSrc,const TReal &aModulus)
/**
Calculates the modulo remainder.

This is the value of p mod q, the modulo remainder when dividing p by q.
The result is given by p - q int (p/q):
it has the same sign as p:
thus, 5 mod 3 = 2, -5 mod 3 = -2.
No error is raised if non-integer arguments are passed.

@param aTrg      A reference containing the result.
@param aSrc      The p argument to the mod function.
@param aModulus  The q argument to the mod function.

@return KErrNone if successful, otherwise another of
        the system-wide error codes. 
*/
//
// Floating point modulo arithmetic.
//
	{

	TRealX f1,f2;
	TInt r=f1.Set(aSrc);
	if (r!=KErrNone)
		{
		SetNaN(aTrg);
		return KErrArgument;
		}
	r=f2.Set(aModulus);
	if (r==KErrArgument || f2.IsZero())
		{
		SetNaN(aTrg);
		return KErrArgument;
		}
	if (r==KErrOverflow)
		{
		aTrg=aSrc;
		return KErrNone;
		}
	if ((TInt(f1.iExp)-TInt(f2.iExp))>KMantissaBits)
		{
		SetZero(aTrg);
		return KErrTotalLossOfPrecision;
		}
	f1.ModEq(f2);
	return f1.GetTReal(aTrg);
	}

#else // __USE_VFP_MATH

// definitions come from RVCT math library
extern "C" TReal fmod(TReal,TReal);

EXPORT_C TInt Math::Mod(TReal& aTrg, const TReal& aSrc, const TReal &aModulus)
	{
	SReal64 *pSrc=(SReal64 *)&aSrc;
	SReal64 *pModulus=(SReal64 *)&aModulus;
	
	if (pSrc->exp==0 || pModulus->exp==0 || pSrc->exp==KSpecialExponent || pModulus->exp==KSpecialExponent)
		{
		TRealX f1,f2;
		TInt r=f1.Set(aSrc);
		if (r!=KErrNone)
			{
			SetNaN(aTrg);
			return KErrArgument;
			}
		r=f2.Set(aModulus);
		if (r==KErrArgument || f2.IsZero())
			{
			SetNaN(aTrg);
			return KErrArgument;
			}
		if (r==KErrOverflow)
			{
			aTrg=aSrc;
			return KErrNone;
			}
		if ((TInt(f1.iExp)-TInt(f2.iExp))>KMantissaBits)
			{
			SetZero(aTrg);
			return KErrTotalLossOfPrecision;
			}
		}
	else if ((pSrc->exp - pModulus->exp) > KMantissaBits)
		{
		SetZero(aTrg);
		return KErrTotalLossOfPrecision;
		}

	aTrg = fmod(aSrc,aModulus);
	return KErrNone;
	}

#endif
