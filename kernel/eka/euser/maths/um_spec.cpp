// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\maths\um_spec.cpp
// Functions to check for  and set special values
// 
//


#include "um_std.h"




#ifndef __REALS_MACHINE_CODED__
EXPORT_C TBool Math::IsZero(const TReal &aVal)
/**
Determines whether a value is zero.

@param aVal A reference to the value to be checked. 

@return True, if aVal is zero; false, otherwise.
*/
	{

	SReal64 *pS=(SReal64 *)&aVal;

	if (pS->msm==0 && pS->lsm==0 && pS->exp==KTReal64ZeroExponent)
		return TRUE;
	else
		return FALSE;
	}




EXPORT_C TBool Math::IsNaN(const TReal &aVal)
/**
Determines whether a value is not a number.

@param aVal A reference to the value to be checked. 

@return True, if aVal is not a number; false, otherwise.
*/
	{

	SReal64 *pS=(SReal64 *)&aVal;

	if (pS->exp==KTReal64SpecialExponent && (pS->msm|pS->lsm)!=0)
		return TRUE;
	else
		return FALSE;
	}




EXPORT_C TBool Math::IsInfinite(const TReal &aVal)
/**
Determines whether a value is infinite.

@param aVal A reference to the value to be checked.

@return True, if aVal is infinite; false, otherwise.
*/
	{

	SReal64 *pS=(SReal64 *)&aVal;
	
	if (pS->msm==0 && pS->lsm==0 && pS->exp==KTReal64SpecialExponent)
		return TRUE;
	else 
		return FALSE;
	}




EXPORT_C TBool Math::IsFinite(const TReal &aVal)
/**
Determines whether a value is finite.

In this context, a value is finite if it is a valid number and
is not infinite.

@param aVal A reference to the value to be checked.

@return True, if aVal is finite; false, otherwise.
*/
	{

	SReal64 *pS=(SReal64 *)&aVal;
	
	if (pS->exp!=KTReal64SpecialExponent)
		return TRUE;
	else 
		return FALSE;
	}




EXPORT_C void Math::SetZero(TReal &aVal,TInt aSign)
//
// Constructs zeros, assuming default sign is positive
//
	{

	SReal64 *pS=(SReal64 *)&aVal;
	pS->sign=aSign;
	pS->exp=KTReal64ZeroExponent;
	pS->msm=0;
	pS->lsm=0;
	}




EXPORT_C void Math::SetNaN(TReal &aVal)
//
// Constructs NaN (+ve sign for Java)
//
	{

	SReal64 *pS=(SReal64 *)&aVal;
	pS->sign=0;
	pS->exp=KTReal64SpecialExponent;
	pS->msm=0xfffffu;
	pS->lsm=0xffffffffu;
	}




EXPORT_C void Math::SetInfinite(TReal &aVal,TInt aSign)
//
// Constructs infinities
//
	{

	SReal64 *pS=(SReal64 *)&aVal;
	pS->sign=aSign;
	pS->exp=KTReal64SpecialExponent;
	pS->msm=0;
	pS->lsm=0;
	}
#endif

