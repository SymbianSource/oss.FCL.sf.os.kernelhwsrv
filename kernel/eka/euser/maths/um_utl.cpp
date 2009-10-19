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
// e32\euser\maths\um_utl.cpp
// 
//

#include "um_std.h"

#if defined(__USE_VFP_MATH) && !defined(__CPU_HAS_VFP)
#error	__USE_VFP_MATH was defined but not __CPU_HAS_VFP - impossible combination, check variant.mmh 
#endif

GLDEF_C void Panic(TMathPanic aPanic)
//
// Panic the process with USER-Math as the category.
//
	{

	User::Panic(_L("USER-Math"),aPanic);
	}



#ifdef __USE_VFP_MATH
extern "C" void __set_errno(TInt errno)
	{
	// Do nothing, we don't have an errno to set as we use
	// return values instead - return values are recosntructed
	// from the results/inputs, not from this
	}

extern "C" IMPORT_C TReal __ARM_scalbn(TReal,TInt);
extern "C" TReal scalbn(TReal x,TInt n)
	{
	return __ARM_scalbn(x,n);
	}
#endif



#ifndef __REALS_MACHINE_CODED__
EXPORT_C TReal Math::Poly(TReal aX,const SPoly *aPoly)
/**
Evaluates the polynomial:
{a[n]X^n + a[n-1]X^(n-1) + ... + a[2]X^2 + a[1]X^1 + a[0]}.


@param aX    The value of the x-variable 
@param aPoly A pointer to the structure containing the set of coefficients
             in the order: a[0], a[1], ..., a[n-1], a[n].

@return The result of the evaluation.
*/
//
// Evaluate a power series in x for a P_POLY coefficient table.
// Changed to use TRealX throughout the calculation
//
	{

	const TReal *pR=(&aPoly->c[aPoly->num-1]);
	TRealX r(*pR);
	TRealX x(aX);
	while (pR>&aPoly->c[0])
		{
		r*=x;
		r+=*--pR;
		}
	return(TReal(r));
	}




EXPORT_C void Math::PolyX(TRealX& aY, const TRealX& aX, TInt aDegree, const TRealX *aCoeff)
/**
Evaluates the polynomial:
{a[n]X^n + a[n-1]X^(n-1) + ... + a[2]X^2 + a[1]X^1 + a[0]}.

@param aY      A reference containing the result. 
@param aX      The value of the x-variable. 
@param aDegree The degree of the polynomial (the highest power of x
               which is present).
@param aCoeff  A pointer to a contiguous set of TRealX values containing
               the coefficients.
               They must be in the order: a[0], a[1], ..., a[n-1], a[n].
*/
	{
    // Evaluate a polynomial with TRealX argument and TRealX coefficients.
    // Return a TRealX result.

	const TRealX *pC=aCoeff+aDegree;
	aY=*pC;
	while(aDegree--)
		{
		aY*=aX;
		aY+=*--pC;
		}
	}
#endif
