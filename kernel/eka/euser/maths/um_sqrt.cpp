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
// e32\euser\maths\um_sqrt.cpp
// Square root.
// 
//

#include "um_std.h"

#if defined(__USE_VFP_MATH) && !defined(__CPU_HAS_VFP)
#error	__USE_VFP_MATH was defined but not __CPU_HAS_VFP - impossible combination, check variant.mmh 
#endif


#ifndef __USE_VFP_MATH

#ifndef __REALS_MACHINE_CODED__
LOCAL_D const TUint32 KConstAdata[] = {0x00000000,0xD5A9A805,0x7FFD0000};
LOCAL_D const TUint32 KConstBdata[] = {0x00000000,0x9714B9CB,0x7FFE0000};
LOCAL_D const TUint32 Sqr2Invdata[] = {0xF9DE6484,0xB504F333,0x7FFE0000};		// 1/sqr2




EXPORT_C TInt Math::Sqrt(TReal& aTrg,const TReal &aSrc)
/**
Calculates the square root of a number.

@param aTrg A reference containing the result. 
@param aSrc The number whose square-root is required.

@return KErrNone if successful, otherwise another of
        the system-wide error codes. 
*/
//
// Fast sqrt routine. See Software manual by W.J.Cody & W.Waite Chapter 4.
//
	{
	const TRealX& KConstA=*(const TRealX*)KConstAdata;
	const TRealX& KConstB=*(const TRealX*)KConstBdata;
	const TRealX& Sqr2Inv=*(const TRealX*)Sqr2Invdata;

	TRealX x;
	TInt r=x.Set(aSrc);
	if (x.IsZero())
		{
		aTrg=aSrc;
		return(KErrNone);
		}   
	if (r==KErrArgument || x.iSign&1)
		{
		SetNaN(aTrg);
		return(KErrArgument);
		}
	if (r==KErrOverflow)	// positive infinity
		{
		aTrg=aSrc;
		return(r);
		}
	TInt n=x.iExp-0x7FFE;
	x.iExp=0x7FFE;
	TRealX y=KConstB*x+KConstA;
	y=y+(x/y);
	y.iExp--;
	y=y+(x/y);
	y.iExp--;
	y=y+(x/y);
	y.iExp--;
	if (n&1)
		{
		y*=Sqr2Inv;
		n++;
		}
	y.iExp=TUint16(TInt(y.iExp)+(n>>1));
	return y.GetTReal(aTrg);
	}
#endif

#endif // !__USE_VFP_MATH - VFP version is in assembler
