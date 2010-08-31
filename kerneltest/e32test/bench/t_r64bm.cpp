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
// e32test\bench\t_r64bm.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <hal.h>

const TInt KHeapSize=0x2000;
const TInt KAverageOverInSeconds=10;
const TInt KNumberOfCalculationsPerLoop=10;

volatile TUint Count;
#ifdef T_R64BM_WITH_VFP
RTest test(_L("T_VFPBM"));
#else
RTest test(_L("T_R64BM"));
#endif

GLREF_C TInt TReal64Addition(TAny*);
GLREF_C TInt TReal64Subtraction(TAny*);
GLREF_C TInt TReal64Multiplication(TAny*);
GLREF_C TInt TReal64Division(TAny*);
GLREF_C TInt TRealSqrt(TAny*);
GLREF_C TInt TRealSin(TAny*);
GLREF_C TInt TRealTan(TAny*);
GLREF_C TInt TRealLn(TAny*);
GLREF_C TInt TRealExp(TAny*);
GLREF_C TInt TRealAsin(TAny*);
GLREF_C TInt TRealAtan(TAny*);
GLREF_C TInt TRealPower(TAny*);

TInt runTest(TThreadFunction aFunction,const TDesC& aTitle)
    {

    RThread thread;
	TInt r=thread.Create(aTitle,aFunction,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	if(r!=KErrNone)
		{
		test.Printf(_L("Failed to create thread with error %d\n"),r);
		return(r);
		}
    thread.Resume();
    User::After(1000000);
    TUint initial = Count;
    User::After(KAverageOverInSeconds*1000000);
	TUint final = Count;
    TUint64 result = TUint64(final - initial);
	result *= TUint64(KNumberOfCalculationsPerLoop);
	result /= TUint64(KAverageOverInSeconds);
	TUint r32 = (TUint)result;
    thread.Kill(0);
	thread.Close();
    test.Printf(_L("%S executed %d in 1 second\n"),&aTitle,r32);
    return r32;
    }

TInt E32Main()
//
// Benchmark for TRealX functions
//
    {

    test.Title();
#ifdef T_R64BM_WITH_VFP
	TInt supportedModes;
	if (HAL::Get(HALData::EHardwareFloatingPoint, supportedModes) != KErrNone)
		{
		test.Printf(_L("Skipping test as this hardware does not have VFP\n"));
		return(KErrNone);
		}
    test.Start(_L("Benchmarks for TReal64 using VFP"));
#else
    test.Start(_L("Benchmarks for TReal64"));
#endif

	runTest(TReal64Addition,_L("Addition"));
	runTest(TReal64Subtraction,_L("Subtraction"));
	runTest(TReal64Multiplication,_L("Multiplication"));
	runTest(TReal64Division,_L("Division"));
	runTest(TRealSqrt,_L("Square root"));
	runTest(TRealSin,_L("Sine"));
	runTest(TRealTan,_L("Tangent"));
	runTest(TRealLn,_L("Logarithm"));
	runTest(TRealExp,_L("Exponential"));
	runTest(TRealAsin,_L("Arcsine"));
	runTest(TRealAtan,_L("Arctan"));
	runTest(TRealPower,_L("Powers"));

	test.End();
	return(KErrNone);
    }

