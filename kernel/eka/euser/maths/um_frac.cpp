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
// e32\euser\maths\um_frac.cpp
// Writes the fractional part of aTrg to aSrc
// 
//

#include "um_std.h"

#if defined(__USE_VFP_MATH) && !defined(__CPU_HAS_VFP)
#error	__USE_VFP_MATH was defined but not __CPU_HAS_VFP - impossible combination, check variant.mmh 
#endif

#ifndef __USE_VFP_MATH

#ifndef __REALS_MACHINE_CODED__
EXPORT_C TInt Math::Frac(TReal &aTrg,const TReal &aSrc)
/**
Calculates the fractional part of a number.

The fractional part is that after a decimal point.
Truncation is toward zero, so that
Frac(2.4)=0.4, Frac(2)=0, Frac(-1)=0, Frac(-1.4)=0.4.

@param aTrg A reference containing the result.
@param aSrc The number whose fractional part is required. 

@return KErrNone if successful, otherwise another of
        the system-wide error codes.
*/
	{
	TRealX f;
	TInt ret=f.Set(aSrc);
	if (ret!=KErrNone)
		{
		if (ret==KErrArgument)
			SetNaN(aTrg);
		if (ret==KErrOverflow)
			SetZero(aTrg,f.iSign&1);
		return(ret);
		}
	TInt intbits=f.iExp-0x7FFE;
	if (intbits<=0)	 // aSrc is already a fraction
		{
		aTrg=aSrc;
		return(KErrNone);
		}
	if (intbits>KMantissaBits)
		{
		SetZero(aTrg,f.iSign&1);
		return(KErrNone);
		}

	// calculate integer part and subtract
	// this means that the subtraction normalises the result
	TRealX g=f;

	TUint64 mask = ~(UI64LIT(0));
	mask <<= (64 - intbits);

	g.iMantHi &= static_cast<TUint32>(mask >> 32);
	g.iMantLo &= static_cast<TUint32>(mask);

	f-=g;
	f.GetTReal(aTrg);
	return(KErrNone);
	}

#endif //__REALS_MACHINE_CODED__

#else // __USE_VFP_MATH

// definitions come from RVCT math library
extern "C" TReal modf(TReal,TReal*);

EXPORT_C TInt Math::Frac(TReal& aTrg, const TReal& aSrc)
	{
	if (Math::IsNaN(aSrc))
		{
		SetNaN(aTrg);
		return KErrArgument;
		}
	if (Math::IsInfinite(aSrc))
		{
		SetZero(aTrg);
		return KErrOverflow;
		}

	TReal temp;
	aTrg = modf(aSrc,&temp);
	return KErrNone;
	}

#endif
