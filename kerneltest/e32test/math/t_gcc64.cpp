// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\math\t_gcc64.cpp
// From UP_GCC.CPP, for emulating UP_GCC in T_R64.CPP tests
// 
//


#include <e32def.h>
#include <e32def_private.h>

#include "t_math.h"

LOCAL_C void MathException(TInt aErrType) 
//
// Decides on type of maths exception according to error and raises exception.
// DOES NOT CONVERT TO SPECIAL IF RETURNS
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
		User::Panic(_L("USER-Math"),EMathUnknownError);
		}

	RThread thread;
	thread.RaiseException(excType);
	}

GLDEF_C TReal64 __adddf3(TReal64 a1,TReal64 a2)
//
// Add two doubles
//
    {

    TRealX x1=a1;
    TRealX x2=a2;

	TRealX res;
	TReal64 trg;
	x1.Add(res,x2);
	TInt ret=res.GetTReal(trg);
	if (ret!=KErrNone)
		MathException(ret);

	return(trg);
    }

GLDEF_C TReal64 __subdf3(TReal64 a1,TReal64 a2)
//
// Subtract two doubles
//
    {

    TRealX x1=a1;
    TRealX x2=a2;

	TRealX res;
	TReal64 trg;
	x1.Sub(res,x2);
	TInt ret=res.GetTReal(trg);
	if (ret!=KErrNone)
		MathException(ret);

	return(trg);    
    }

GLDEF_C TReal64 __muldf3(TReal64 a1,TReal64 a2)
//
// Multiply two doubles
//
    {

    TRealX x1=a1;
    TRealX x2=a2;

	TRealX res;
	TReal64 trg;
	TInt ret=x1.Mult(res,x2);
	if (ret==KErrNone)
		ret=res.GetTReal(trg);
	if (ret!=KErrNone)
		MathException(ret);

	return((TReal64)res);
    }

GLDEF_C TReal64 __divdf3(TReal64 a1,TReal64 a2)
	//
	// Divide two doubles
	//
	{
	
	TRealX x1=a1;
	TRealX x2=a2;

	TRealX res;
	TReal64 trg;
	TInt ret=x1.Div(res,x2);
	if (ret==KErrNone)
		ret=res.GetTReal(trg);
	if (ret!=KErrNone)
		MathException(ret);

	return((TReal64)res);
	}
