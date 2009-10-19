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
// e32\euser\maths\um_log.cpp
// Floating point base 10 logarithm
// 
//

#include "um_std.h"

#if defined(__USE_VFP_MATH) && !defined(__CPU_HAS_VFP)
#error	__USE_VFP_MATH was defined but not __CPU_HAS_VFP - impossible combination, check variant.mmh 
#endif


#ifndef __USE_VFP_MATH

EXPORT_C TInt Math::Log(TReal &aTrg,const TReal &aSrc)
/**
Calculates the logarithm to base 10 of a number.

@param aTrg  A reference containing the result. 
@param aSrc  The number whose logarithm is required. 
 
@return KErrNone if successful, otherwise another of
        the system-wide error codes. 
*/
//
// Base 10 log routine. See Software manual by W.J.Cody & W.Waite
//
	{

	TInt ret=Math::Ln(aTrg,aSrc);
	if (ret!=KErrNone)
		return(ret);    	
	aTrg*=KRln10;
	return(ret);
	}

#else // __USE_VFP_MATH

// definitions come from RVCT math library
extern "C" TReal log10(TReal);

EXPORT_C TInt Math::Log(TReal& aTrg, const TReal& aSrc)
	{
	aTrg = log10(aSrc);
	if (Math::IsFinite(aTrg))
		return KErrNone;
	if (Math::IsInfinite(aTrg))
		return KErrOverflow;
	SetNaN(aTrg);
	return KErrArgument;
	}

#endif
