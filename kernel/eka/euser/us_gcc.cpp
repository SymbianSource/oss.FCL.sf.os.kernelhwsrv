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
// e32\euser\us_gcc.cpp
// 
//

#include "us_std.h"
#include <e32math.h>

extern "C" {

EXPORT_C TInt64 __fixsfdi(TReal32 a1)
//
// Convert float to long long
//
	{
	const TRealX f(a1);
	const TInt64 ret = f;
	return ret;
	}

}	// end of extern "C" declaration

#ifndef __REALS_MACHINE_CODED__
LOCAL_C void MathException(TInt aErrType) 
//
// Decides on type of maths exception according to error and raises exception.
// Added by AnnW, December 1996
//
	{

	TExcType excType=EExcGeneral;

	switch (aErrType)
		{
	case KErrArgument:				// error due to invalid operation
		excType=EExcFloatInvalidOperation;
		break;
	case KErrDivideByZero:
		excType=EExcFloatDivideByZero;
		break;
	case KErrOverflow:
		excType=EExcFloatOverflow;
		break;
	case KErrUnderflow:
		excType=EExcFloatUnderflow;
		break;
/*
	// also errors due to inexact result
	case KErrInexact:		// const not defined yet
		excType=EExcFloatInexact;
		break;
*/
	
	default:
		// Unknown error
		Panic(EMathUnknownError);
		}

	User::RaiseException(excType);
	}

EXPORT_C TReal32 __addsf3(TReal32 a1,TReal32 a2)
//
// Add two floats
//
    {

    TRealX x1=a1;
    TRealX x2=a2;

	TRealX res;
	TReal32 trg;
	x1.Add(res,x2);	// need not check error because no underflow and others will not be lost in conversion
	TInt ret=res.GetTReal(trg);
	if (ret!=KErrNone)
		MathException(ret);

	return(trg);
    }

EXPORT_C TReal64 __adddf3(TReal64 a1,TReal64 a2)
//
// Add two doubles
//
    {

    TRealX x1=a1;
    TRealX x2=a2;

	TRealX res;
	TReal64 trg;
	x1.Add(res,x2);	// need not check error because no underflow and others will not be lost in conversion
	TInt ret=res.GetTReal(trg);
	if (ret!=KErrNone)
		MathException(ret);

	return(trg);
    }

EXPORT_C TReal32 __subsf3(TReal32 a1,TReal32 a2)
//
// Subtract two floats
//
    {

    TRealX x1=a1;
    TRealX x2=a2;

	TRealX res;
	TReal32 trg;
	x1.Sub(res,x2);	// need not check error because no underflow and others will not be lost in conversion
	TInt ret=res.GetTReal(trg);
	if (ret!=KErrNone)
		MathException(ret);

	return(trg);
	}

EXPORT_C TReal64 __subdf3(TReal64 a1,TReal64 a2)
//
// Subtract two doubles
//
    {

    TRealX x1=a1;
    TRealX x2=a2;

	TRealX res;
	TReal64 trg;
	x1.Sub(res,x2);	// need not check error because no underflow and others will not be lost in conversion
	TInt ret=res.GetTReal(trg);
	if (ret!=KErrNone)
		MathException(ret);

	return(trg);    
    }

EXPORT_C TInt __cmpsf3(TReal32 a1,TReal32 a2)
//
// Compare two floats
//
    {

    TRealX x1=a1;
    TRealX x2=a2;
    if (x1<x2)
        return(-1);
    if (x1>x2)
        return(1);
    return(0);
    }

EXPORT_C TInt __cmpdf3(TReal64 a1,TReal64 a2)
//
// Compare two doubles
//
    {

    TRealX x1=a1;
    TRealX x2=a2;
    if (x1<x2)
        return(-1);
    if (x1>x2)
        return(1);
    return(0);
    }

EXPORT_C TInt __eqsf2(TReal32 a1,TReal32 a2)
//
// Compare if two floats are equal
//
    {

    TRealX x1=a1;
    TRealX x2=a2;
    return (x1==x2 ? 0 : 1);
    }

EXPORT_C TInt __eqdf2(TReal64 a1,TReal64 a2)
//
// Compare if two doubles are equal
//
    {

    TRealX x1=a1;
    TRealX x2=a2;
    return (x1==x2 ? 0 : 1);
    }

EXPORT_C TInt __nesf2(TReal32 a1,TReal32 a2)
//
// Compare if two floats are not equal
//
    {

    TRealX x1=a1;
    TRealX x2=a2;
    return(x1!=x2 ? 1 : 0);
    }

EXPORT_C TInt __nedf2(TReal64 a1,TReal64 a2)
//
// Compare if two doubles are not equal
//
    {

    TRealX x1=a1;
    TRealX x2=a2;
    return(x1!=x2 ? 1 : 0);
    }

EXPORT_C TInt __gtsf2(TReal32 a1,TReal32 a2)
//
// Compare if one float is greater than another
//
    {

    TRealX x1=a1;
    TRealX x2=a2;
    return(x1>x2 ? +1 : -1);
    }

EXPORT_C TInt __gtdf2(TReal64 a1,TReal64 a2)
//
// Compare if one double is greater than another
//
    {

    TRealX x1=a1;
    TRealX x2=a2;
    return(x1>x2 ? +1 : -1);
    }

EXPORT_C TInt __gesf2(TReal32 a1,TReal32 a2)
//
// Compare if one float is greater than or equal to another
//
    {

    TRealX x1=a1;
    TRealX x2=a2;
    return(x1>=x2 ? 1 : -1);
    }

EXPORT_C TInt __gedf2(TReal64 a1,TReal64 a2)
//
// Compare if one double is greater than or equal to another
//
    {

    TRealX x1=a1;
    TRealX x2=a2;
    return(x1>=x2 ? 1 : -1);
    }

EXPORT_C TInt __ltsf2(TReal32 a1,TReal32 a2)
//
// Compare if one float is less than another
//
    {

    TRealX x1=a1;
    TRealX x2=a2;
    return(x1<x2 ? -1 : +1);
    }

EXPORT_C TInt __ltdf2(TReal64 a1,TReal64 a2)
//
// Compare if one double is less than another
//
    {

    TRealX x1=a1;
    TRealX x2=a2;
    return(x1<x2 ? -1 : +1);
    }

EXPORT_C TInt __lesf2(TReal32 a1,TReal32 a2)
//
// Compare if one float is less than or equal to another
//
    {

    TRealX x1=a1;
    TRealX x2=a2;
    return(x1<=x2 ? -1 : +1);
    }

EXPORT_C TInt __ledf2(TReal64 a1,TReal64 a2)
//
// Compare if one double is less than or equal to another
//
    {

    TRealX x1=a1;
    TRealX x2=a2;
    return(x1<=x2 ? -1 : +1);
    }

EXPORT_C TReal32 __mulsf3(TReal32 a1,TReal32 a2)
//
// Multiply two floats
//
    {

    TRealX x1=a1;
    TRealX x2=a2;

	TRealX res;
	TReal32 trg;
	TInt ret=x1.Mult(res,x2);
	if (ret==KErrNone)		// must check this because if underflow, error is lost in conversion
		ret=res.GetTReal(trg);
	if (ret!=KErrNone)
		MathException(ret);
	
	return((TReal32)res);
    }

EXPORT_C TReal64 __muldf3(TReal64 a1,TReal64 a2)
//
// Multiply two doubles
//
    {

    TRealX x1=a1;
    TRealX x2=a2;

	TRealX res;
	TReal64 trg;
	TInt ret=x1.Mult(res,x2);
	if (ret==KErrNone)		// must check this because if underflow, error is lost in conversion
		ret=res.GetTReal(trg);
	if (ret!=KErrNone)
		MathException(ret);

	return((TReal64)res);
    }

EXPORT_C TReal32 __divsf3(TReal32 a1,TReal32 a2)
//
// Divide two floats
//
    {

    TRealX x1=a1;
    TRealX x2=a2;

	TRealX res;
	TReal32 trg;
	TInt ret=x1.Div(res,x2);
	if (ret==KErrNone)		// must check this because if underflow, error is lost in conversion
		ret=res.GetTReal(trg);
	if (ret!=KErrNone)
		MathException(ret);

	return((TReal32)res);
    }

EXPORT_C TReal64 __divdf3(TReal64 a1,TReal64 a2)
	//
	// Divide two doubles
	//
	{

	TRealX x1=a1;
	TRealX x2=a2;

	TRealX res;
	TReal64 trg;
	TInt ret=x1.Div(res,x2);
	if (ret==KErrNone)		// must check this because if underflow, error is lost in conversion
		ret=res.GetTReal(trg);
	if (ret!=KErrNone)
		MathException(ret);
	
	return((TReal64)res);
	}

EXPORT_C TReal32 __negsf2(TReal32 a1)
//
// Negate a float
//
    {

    TRealX x1=a1;
    return((TReal32)-x1);
    }

EXPORT_C TReal64 __negdf2(TReal64 a1)
//
// Negate a double
//
    {

    TRealX x1=a1;
    return((TReal64)-x1);
    }

EXPORT_C TReal32 __floatsisf(TInt a1)
//
// Convert int to float
//
    {

    return((TReal32)TRealX(a1));
    }

EXPORT_C TReal64 __floatsidf(TInt a1)
//
// Convert int to double
//
    {

    return((TReal64)TRealX(a1));
    }

EXPORT_C TInt __fixsfsi(TReal32 a1)
//
// Convert float to int
//
    {

    return((TInt)TRealX(a1));
    }

EXPORT_C TInt __fixdfsi(TReal64 a1)
//
// Convert double to int
//
    {

    return((TInt)TRealX(a1));
    }

EXPORT_C TReal64 __extendsfdf2(TReal32 a1)
//
// Convert a float to a double
//
    {

    return((TReal64)TRealX(a1));
    }

EXPORT_C TReal32 __truncdfsf2(TReal64 a1)
//
// Convert a double to a float
// Raises an exception if conversion results in an error
//
    {

	TRealX x1=a1;

	TInt ret;
	TReal32 trg;
	if ((ret=x1.GetTReal(trg))!=KErrNone)
		MathException(ret);

    return(trg);
    }
#endif //__REALS_MACHINE_CODED__
