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
// e32test\bench\t_r64fnc.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32math.h>

volatile TReal64 Zero(0);
volatile TReal64 One(1);
volatile TReal64 Two(2);
volatile TReal64 Ten(10);
volatile TReal64 Pi(3.14159265358979323846);
volatile TReal64 TenPoint01(10.01);

volatile TReal64 r(0);

TReal64 AntiOptimization[16] = {0.1,	1,	3.14159265358979323846,		10.01,
								2.7,	3,	27.2,	11.23,
								76.1,	9,	56.1,	1/9,
								1/3,	22,	99.7,	42};

GLREF_D volatile TUint Count;

#include <e32btrace.h>

void Step()
	{
	if (++Count & 0xffff)
		return;
	BTrace4(BTrace::ETest1, 0, Count);
	}

TInt TReal64Addition(TAny*)
    {
	Count=0;

    FOREVER
        {
		r=AntiOptimization[Count & 0xf];
		r+=One;
		r+=One;
		r+=One;
		r+=One;
		r+=One;
		r+=One;
		r+=One;
		r+=One;
		r+=One;
		r+=One;
        Step();
        }
    }

TInt TReal64Subtraction(TAny*)
    {
	Count=0;

    FOREVER
        {
		r=AntiOptimization[Count & 0xf];
		r-=Ten;
		r-=Ten;
		r-=Ten;
		r-=Ten;
		r-=Ten;
		r-=Ten;
		r-=Ten;
		r-=Ten;
		r-=Ten;
		r-=Ten;
        Step();
        }
    }

TInt TReal64Multiplication(TAny*)
    {
	Count=0;
    FOREVER
        {
		r=AntiOptimization[Count & 0xf];
		r*=Pi;
		r*=Pi;
		r*=Pi;
		r*=Pi;
		r*=Pi;
		r*=Pi;
		r*=Pi;
		r*=Pi;
		r*=Pi;
		r*=Pi;
        Step();
        }
    }

TInt TReal64Division(TAny*)
    {
	Count=0;
	
    FOREVER
        {
		r=AntiOptimization[Count & 0xf];
		r/=Ten;
		r/=Ten;
		r/=Ten;
		r/=Ten;
		r/=Ten;
		r/=Ten;
		r/=Ten;
		r/=Ten;
		r/=Ten;
		r/=Ten;
        Step();
        }
    }

TInt TRealSqrt(TAny*)
    {
	Count=0;

	TReal r;

    FOREVER
        {
		Math::Sqrt(r,2.0);
		Math::Sqrt(r,3.0);
		Math::Sqrt(r,4.0);
		Math::Sqrt(r,5.0);
		Math::Sqrt(r,3.14159265358979323846);
		Math::Sqrt(r,2.71828182845904524);
		Math::Sqrt(r,0.69314718055994531);
		Math::Sqrt(r,1.414213562373);
		Math::Sqrt(r,1.7320508078);
		Math::Sqrt(r,299792458.0);
        Step();
        }
    }

TInt TRealSin(TAny*)
    {
	Count=0;

	TReal r=7;

    FOREVER
        {
		Math::Sin(r,1.0);
		Math::Sin(r,2.0);
		Math::Sin(r,3.0);
		Math::Sin(r,4.0);
		Math::Sin(r,5.0);
		Math::Sin(r,6.0);
		Math::Sin(r,7.0);
		Math::Sin(r,8.0);
		Math::Sin(r,9.0);
		Math::Sin(r,-1.0);
        Step();
        }
    }

TInt TRealLn(TAny*)
    {
	Count=0;

	TReal r=7;

    FOREVER
        {
		Math::Ln(r,3.141592653589793238);
		Math::Ln(r,2.718281828459045235);
		Math::Ln(r,7.389056098930650227);
		Math::Ln(r,2.0);
		Math::Ln(r,3.0);
		Math::Ln(r,5.0);
		Math::Ln(r,7.0);
		Math::Ln(r,11.0);
		Math::Ln(r,13.0);
		Math::Ln(r,17.0);
        Step();
        }
    }

TInt TRealExp(TAny*)
    {
	Count=0;

	TReal r=7;

    FOREVER
        {
		Math::Exp(r,3.14159265358979323846);
		Math::Exp(r,-3.14159265358979323846);
		Math::Exp(r,0.69314718056);
		Math::Exp(r,-0.69314718056);
		Math::Exp(r,1.0);
		Math::Exp(r,-1.0);
		Math::Exp(r,2.0);
		Math::Exp(r,-2.0);
		Math::Exp(r,11.0);
		Math::Exp(r,-11.0);
        Step();
        }
    }

TInt TRealAsin(TAny*)
    {
	Count=0;

	TReal r=7;

    FOREVER
        {
		Math::ASin(r,0.1);
		Math::ASin(r,0.2);
		Math::ASin(r,0.3);
		Math::ASin(r,0.4);
		Math::ASin(r,0.5);
		Math::ASin(r,0.6);
		Math::ASin(r,0.7);
		Math::ASin(r,0.8);
		Math::ASin(r,0.9);
		Math::ASin(r,-0.9);
        Step();
        }
    }

TInt TRealAtan(TAny*)
    {
	Count=0;

	TReal r=7;

    FOREVER
        {
		Math::ATan(r,0.1);
		Math::ATan(r,0.3);
		Math::ATan(r,0.5);
		Math::ATan(r,0.7);
		Math::ATan(r,0.9);
		Math::ATan(r,1.1);
		Math::ATan(r,1.3);
		Math::ATan(r,1.5);
		Math::ATan(r,1.7);
		Math::ATan(r,2.9);
        Step();
        }
    }

TInt TRealTan(TAny*)
    {
	Count=0;

	TReal r=7;

    FOREVER
        {
		Math::Tan(r,1.0);
		Math::Tan(r,2.0);
		Math::Tan(r,3.0);
		Math::Tan(r,4.0);
		Math::Tan(r,5.0);
		Math::Tan(r,6.0);
		Math::Tan(r,7.0);
		Math::Tan(r,8.0);
		Math::Tan(r,9.0);
		Math::Tan(r,-1.0);
        Step();
        }
    }

TInt TRealPower(TAny*)
    {
	Count=0;

	TReal r=7;

    FOREVER
        {
		Math::Pow(r,2.718281828459045235,2.718281828459045235);
		Math::Pow(r,3.141592653589793238,3.141592653589793238);
		Math::Pow(r,1.414213562373,3.1);
		Math::Pow(r,299792458,0.70710678118);
		Math::Pow(r,1.989e30,0.1);
		Math::Pow(r,0.86602540378,-1.6180334);
		Math::Pow(r,7.0,0.5772156649);
		Math::Pow(r,95.4,1.57079);
		Math::Pow(r,317.9,0.3333333333333333);
		Math::Pow(r,299792458,-2.718281828459045235);
        Step();
        }
    }

