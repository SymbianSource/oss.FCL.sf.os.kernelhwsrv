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
// e32\euser\maths\um_int.cpp
// Writes the integer part aTrg to aSrc (aSrc is either TReal,TInt16 orTInt32)
// 
//

#include "um_std.h"

#if defined(__USE_VFP_MATH) && !defined(__CPU_HAS_VFP)
#error	__USE_VFP_MATH was defined but not __CPU_HAS_VFP - impossible combination, check variant.mmh 
#endif

#ifndef __USE_VFP_MATH

#ifndef __REALS_MACHINE_CODED__
EXPORT_C TInt Math::Int(TReal &aTrg,const TReal &aSrc)
/**
Calculates the integer part of a number.

The integer part is that before a decimal point.
Truncation is toward zero, so that
int(2.4)=2, int(2)=2, int(-1)=-1, int(-1.4)=-1, int(-1.999)=-1.


@param aTrg A reference containing the result. 
@param aSrc The number whose integer part is required. 

@return KErrNone if successful, otherwise another of
        the system-wide error codes. 
*/
	{
	TRealX f;
	TInt ret=f.Set(aSrc);
	if (ret!=KErrNone)
		{
		aTrg=aSrc;
		return(ret);
		}
	TInt intbits=f.iExp-0x7FFE;	// number of integer bits in mantissa
	if (intbits<=0)
		{
		SetZero(aTrg,f.iSign&1); // no integer part
		return(KErrNone);
		}
	if (intbits>=KMantissaBits)
		{
		aTrg=aSrc; // fractional part is outside range of significance
		return(KErrNone);
		}

	TUint64 mask = ~(UI64LIT(0));
	mask <<= (64 - intbits);

	f.iMantHi &= static_cast<TUint32>(mask >> 32);
	f.iMantLo &= static_cast<TUint32>(mask);

	f.GetTReal(aTrg);
	return(KErrNone);
	}




EXPORT_C TInt Math::Int(TInt16 &aTrg,const TReal &aSrc)
/**
Calculates the integer part of a number.

The integer part is that before a decimal point.
Truncation is toward zero, so that
int(2.4)=2, int(2)=2, int(-1)=-1, int(-1.4)=-1, int(-1.999)=-1.

This function is suitable when the result is known to be small enough
for a 16-bit signed integer.

@param aTrg A reference containing the result. 
@param aSrc The number whose integer part is required.

@return KErrNone if successful, otherwise another of
        the system-wide error codes.
*/
//
// If the integer part of aSrc is in the range -32768 to +32767
// inclusive, write the integer part to the TInt16 at aTrg
// Negative numbers are rounded towards zero.
// If an overflow or underflow occurs, aTrg is set to the max/min value
//
	{
	TRealX f;
	TInt ret=f.Set(aSrc);

	if (ret==KErrArgument)
		{
		aTrg=0;
		return(ret);
		}

	TInt intbits=f.iExp-0x7FFE;	// number of integer bits in mantissa

	if (intbits<=0)
		{
		aTrg=0;
		return(KErrNone);
		}

	if (intbits>16)
		{
		aTrg=(TInt16)((f.iSign&1) ? KMinTInt16 : KMaxTInt16);
		return((f.iSign&1) ? KErrUnderflow : KErrOverflow);
		}

	TUint val = f.iMantHi >> (32 - intbits);

	if ((f.iSign&1)==0 && val>(TUint)KMaxTInt16)
		{
		aTrg=TInt16(KMaxTInt16);
		return(KErrOverflow);
		}

	if ((f.iSign&1) && val>(TUint)(KMaxTInt16+1))
		{
		aTrg=TInt16(KMinTInt16);
		return(KErrUnderflow);
		}

	aTrg = (f.iSign&1) ? (TInt16)(-(TInt)val) : (TInt16)val;

	return(KErrNone);
	} 




EXPORT_C TInt Math::Int(TInt32 &aTrg,const TReal &aSrc)
/**
Calculates the integer part of a number.

The integer part is that before a decimal point.
Truncation is toward zero, so that
int(2.4)=2, int(2)=2, int(-1)=-1, int(-1.4)=-1, int(-1.999)=-1.

This function is suitable when the result is known to be small enough
for a 32-bit signed integer.

@param aTrg A reference containing the result. 
@param aSrc The number whose integer part is required.

@return KErrNone if successful, otherwise another of
        the system-wide error codes.
*/
//													 
// If the integer part of the float is in the range -2147483648 to +2147483647
// inclusive, write the integer part to the TInt32 at aTrg
// Negative numbers are rounded towards zero.
// If an overflow or underflow occurs, aTrg is set to the max/min value
//
	{
	TRealX f;
	TInt ret=f.Set(aSrc);

	if (ret==KErrArgument)
		{
		aTrg=0;
		return(ret);
		}

	TInt intbits=f.iExp-0x7FFE;	// number of integer bits in mantissa

	if (intbits<=0)
		{
		aTrg=0;
		return(KErrNone);
		}

	if (intbits>32)
		{
		aTrg=((f.iSign&1) ? KMinTInt32 : KMaxTInt32);
		return((f.iSign&1) ? KErrUnderflow : KErrOverflow);
		}

	TUint val = f.iMantHi >> (32 - intbits);

	if ((f.iSign&1)==0 && val>(TUint)KMaxTInt32)
		{
		aTrg=KMaxTInt32;
		return(KErrOverflow);
		}

	if ((f.iSign&1) && val>((TUint)KMaxTInt32+1))
		{
		aTrg=KMinTInt32;
		return(KErrUnderflow);
		}

	aTrg=(f.iSign&1) ? -(TInt32)val : val;

	return(KErrNone);
	}

#endif //__REALS_MACHINE_CODED__

#else // __USE_VFP_MATH

// definitions come from RVCT math library
extern "C" TReal modf(TReal,TReal*);

EXPORT_C TInt Math::Int(TReal& aTrg, const TReal& aSrc)
	{
	if (Math::IsNaN(aSrc))
		{
		SetNaN(aTrg);
		return KErrArgument;
		}
	if (Math::IsInfinite(aSrc))
		{
		aTrg=aSrc;
		return KErrOverflow;
		}

	modf(aSrc,&aTrg);
	return KErrNone;
	}

EXPORT_C TInt Math::Int(TInt32& aTrg, const TReal& aSrc)
	{
	TReal aIntPart;
	TInt r = Math::Int(aIntPart,aSrc);
	if (r==KErrArgument)
		{
		aTrg = 0;
		return r;
		}
	if (aIntPart>KMaxTInt32)
		{
		aTrg = KMaxTInt32;
		return KErrOverflow;
		}
	if (aIntPart<KMinTInt32)
		{
		aTrg = KMinTInt32;
		return KErrUnderflow;
		}
	aTrg = aIntPart;
	return KErrNone;
	}

EXPORT_C TInt Math::Int(TInt16& aTrg, const TReal& aSrc)
	{
	TReal aIntPart;
	TInt r = Math::Int(aIntPart,aSrc);
	if (r==KErrArgument)
		{
		aTrg = 0;
		return r;
		}
	if (aIntPart>KMaxTInt16)
		{
		aTrg = KMaxTInt16;
		return KErrOverflow;
		}
	if (aIntPart<KMinTInt16)
		{
		aTrg = KMinTInt16;
		return KErrUnderflow;
		}
	aTrg = aIntPart;
	return KErrNone;
	}

#endif
