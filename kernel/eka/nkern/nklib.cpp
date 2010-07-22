// Copyright (c) 2010-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkern\nklib.cpp
// 
//

#include <e32atomics.h>
#include <nklib.h>

#ifndef __SRATIO_MACHINE_CODED__
void SRatio::Set(TUint32 aInt, TInt aDivisorExp)
	{
	iSpare1 = 0;
	iSpare2 = 0;
	iM = aInt;
	if (iM)
		{
		TInt ms1 = __e32_find_ms1_32(iM);
		TInt shift = 31 - ms1;
		iM <<= shift;
		iX = (TInt16)(-shift - aDivisorExp);
		}
	else
		iX = 0;
	}

TInt SRatio::Reciprocal()
	{
	if (iM==0)
		return KErrDivideByZero;
	// Calculate 2^32/iM
	TInt exp=0;
	if (iM == 0x80000000u)
		{
		// ratio = 2^(31+iX) so reciprocal = 2^(-31-iX) = 2^(31 + (-62-iX))
		exp = -62-iX;
		}
	else
		{
		// 2^32/iM = 1.xxx
		TUint64 r64 = MAKE_TUINT64(0u-iM,0);
		TUint32 q32 = (TUint32)(r64/TUint64(iM));	// next 32 bits of result
		iM = 0x80000000u | (q32>>1);
		exp = -63-iX;
		if (q32 & 1)
			{
			if (++iM==0)
				iM=0x80000000u, ++exp;
			}
		}
	if (exp < -32768)
		{
		iM = 0;
		iX = 0;
		return KErrUnderflow;
		}
	if (exp > 32767)
		{
		iM = 0xffffffffu;
		iX = 32767;
		return KErrOverflow;
		}
	iX = (TInt16)exp;
	return KErrNone;
	}

TInt SRatio::Mult(TUint32& aInt32)
	{
	TUint64 x = aInt32;
	x *= TUint64(iM);
	if (x==0)
		{
		aInt32 = 0;
		return KErrNone;
		}
	TInt ms1 = __e32_find_ms1_64(x);
	TInt ms1b = ms1 + iX;
	if (ms1b>=32)
		{
		aInt32 = ~0u;
		return KErrOverflow;
		}
	if (ms1b<-1)
		{
		aInt32 = 0;
		return KErrUnderflow;
		}
	TInt shift = ms1b - ms1 + 31;
	if (shift > 0)
		x <<= shift;
	else if (shift < 0)
		x >>= (-shift);
	x += MAKE_TUINT64(0,0x40000000u);
	if (x >> 63)
		{
		aInt32 = ~0u;
		return KErrOverflow;
		}
	aInt32 = (TUint32)(x>>31);
	return aInt32 ? KErrNone : KErrUnderflow;
	}

//TInt SRatio::Mult(TUint64& aInt64)
//	{
//	}
#endif

void SRatioInv::Set(const SRatio* a)
	{
	if (a)
		{
		iR = *a;
		iI = iR;
		iI.Reciprocal();
		}
	else
		{
		iR.Set(1);
		iI.Set(1);
		}
	}



