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
// e32\euser\epoc\arm\uc_realx.cpp
// 
//

#include "u32std.h"
#include <e32math.h>

GLDEF_C void PanicOverUnderflowDividebyZero(const TInt aErr)
//
// Panics if there is an overflow or underflow or divide by zero
//
	{
//	if (aErr==KErrOverflow)
//		Panic(EMathOverflow);
//	if (aErr==KErrUnderflow)
//		Panic(EMathUnderflow);
//	if (aErr==KErrDivideByZero)
//		Panic(EMathDivideByZero);
//	if (aErr==KErrArgument)
//		Panic(EMathBadOperand);
	User::Panic(_L("MATHX"),aErr);
	}

#ifdef __REALS_MACHINE_CODED__
extern "C" void __math_exception(TInt aErrType) 
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
//		Panic(EMathUnknownError);
		User::Panic(_L("MATHX"),KErrGeneral);
		}

	User::RaiseException(excType);
	}
#endif




#ifdef __EABI_CTORS__
EXPORT_C TRealX::TRealX()
/**
Constructs a default extended precision object.

This sets the value to zero.
*/
	{
	// Be Brutal (this should be as efficient as the naked versions)
        TUint * me = (TUint *)this;
	me[0]=me[1]=me[2]=0;
	}




EXPORT_C TRealX::TRealX(TUint anExp, TUint aMantHi, TUint aMantLo)
/**
Constructs an extended precision object from an explicit exponent and
a 64 bit mantissa.

@param anExp   The exponent 
@param aMantHi The high order 32 bits of the 64 bit mantissa 
@param aMantLo The low order 32 bits of the 64 bit mantissa 
*/
	{
	// Be Brutal (this should be as efficient as the naked versions)
        TUint * me = (TUint *)this;
	me[0]=aMantLo;
	me[1]=aMantHi;
	me[2]=anExp;
	}




EXPORT_C TRealX::TRealX(TInt anInt)
/**
Constructs an extended precision object from a signed integer value.

@param anInt The signed integer value.
*/
	{
	TRealX::operator=(anInt);
	}




EXPORT_C TRealX::TRealX(const TInt64& anInt)
/**
Constructs an extended precision object from a 64 bit integer.

@param anInt A reference to a 64 bit integer. 
*/
	{
	TRealX::operator=(anInt);
	}




EXPORT_C TRealX::TRealX(TUint anInt)
/**
Constructs an extended precision object from an unsigned integer value.

@param anInt The unsigned integer value.
*/
	{
	TRealX::operator=(anInt);
	}




EXPORT_C TRealX::TRealX(TReal32 aReal)__SOFTFP
/**
Constructs an extended precision object from
a single precision floating point number.

@param aReal The single precision floating point value.
*/
	{
	TRealX::operator=(aReal);
	}




EXPORT_C TRealX::TRealX(TReal64 aReal)__SOFTFP
/**
Constructs an extended precision object from
a double precision floating point number.

@param aReal The double precision floating point value.
*/
	{
	TRealX::operator=(aReal);
	}
#endif
