// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\bench\t_asmbm.cpp
// 
//

#include "t_asmbm.h"
#include <hal.h>
#include <e32test.h>

extern RTest test;

const TReal KDefaultRunLength = 1.0;
const TInt KInitIterations = 3000;

// Length of time to run benchmark for in seconds
TReal RunLength = KDefaultRunLength;

TInt FastCounterFrequency;
TBool FastCounterCountsUp;

/**
 * Calculate the time in seconds corresponding to a fast counter delta value.
 */
TReal TimeDelta(TInt aDelta)
	{
	if (!FastCounterCountsUp)
		aDelta = -aDelta;
	return ((TReal) aDelta + 1) / FastCounterFrequency;
	}

/**
 * Run a benchmark for the specifiec number of iterations and return the total
 * time taken in seconds.
 */
TReal TimeBenchmarkL(MBenchmarkList& aBenchmarks, TInt aIndex, const TBmParams& aParams)
	{
	TInt delta = 0;
	User::LeaveIfError(aBenchmarks.Run(aIndex, aParams, delta));
	User::After(20 * 1000); // hack: wait for kernel thread to exit
	return TimeDelta(delta);
	}

void RunGeneralBenchmarkL(MBenchmarkList& aBenchmarks, TInt aIndex, const TBmInfo& aInfo)
    {
	// Run the benchmark with a small number of iterations and from this result
	// work out how many iterations we need to run it for RunLength seconds.
	// Loop till we get it right.

	TBmParams params;
	params.iSourceAlign = 0;
	params.iDestAlign = 0;
	
	TInt iterations = KInitIterations;
	TReal time;
	for (;;)
		{
		params.iIts = iterations / 10;
		time = TimeBenchmarkL(aBenchmarks, aIndex, params);
		if (time >= RunLength)
			break;
		iterations = (TInt) ((RunLength * 1.2) / (time/(TReal)iterations));
		}
	
	TBuf<64> nameBuf;
	nameBuf.Copy(aInfo.iName);
    test.Printf(_L("%i\t%e\t%S\n"), aIndex, time/iterations * 1000000.0, &nameBuf);
    }

void RunMemoryBenchmarkL(MBenchmarkList& aBenchmarks, TInt aIndex, const TBmInfo& aInfo)
    {
	// Run the benchmark with a small number of iterations and from this result
	// work out how many iterations we need to run it for RunLength seconds.
	// Loop till we get it right.
	
	TBmParams params;
	params.iSourceAlign = 0;
	params.iDestAlign = 0;

	TInt iterations = KInitIterations;
	TReal time;
	for (;;)
		{
		params.iIts = iterations / 10;
		time = TimeBenchmarkL(aBenchmarks, aIndex, params);
		if (time >= RunLength)
			break;
		iterations = (TInt) ((RunLength * 1.2) / (time/(TReal)iterations));
		}

	TBuf<64> nameBuf;
	nameBuf.Copy(aInfo.iName);
	test.Printf(_L("%i\t%S\talignment step == %d\n"), aIndex, &nameBuf, aInfo.iAlignStep);

	for (TInt sourceAlign = 0 ; sourceAlign < 32 ; sourceAlign += aInfo.iAlignStep)
		{
		for (TInt destAlign = 0 ; destAlign < 32 ; destAlign += aInfo.iAlignStep)
			{
			params.iSourceAlign = sourceAlign;
			params.iDestAlign = destAlign;
			time = TimeBenchmarkL(aBenchmarks, aIndex, params);
			test.Printf(_L("%e\t"), time/iterations * 1000000.0);
			}
		test.Printf(_L("\n"));
		}
    }

void RunBenchmarkL(MBenchmarkList& aBenchmarks, TInt aIndex, TUint aCategories)
	{
	TBmInfo info;
	User::LeaveIfError(aBenchmarks.Info(aIndex, info));

	if (!(info.iCategories & aCategories))
		return;

	if (info.iCategories & aCategories & KCategoryMemory)
		RunMemoryBenchmarkL(aBenchmarks, aIndex, info);
	else
		RunGeneralBenchmarkL(aBenchmarks, aIndex, info);
	}

void BadUsage()
	{
	test.Printf(_L("usage: [ OPTIONS ] [ INDEX... ]\n"));
	test.Printf(_L("Options are:\n"));
	test.Printf(_L("  -r TIME  Set the length of time in seconds to run each benchmark for\n"));
	test.Printf(_L("  -m       Run memory alignment benchmarks only\n"));
	test.Printf(_L("  -x       Run extra benchmarks as well as normal ones\n"));
	}

void RunBenchmarkTestsL(MBenchmarkList& aBenchmarks)
	{
	InitDataL();
	
	User::LeaveIfError(HAL::Get(HALData::EFastCounterFrequency, FastCounterFrequency));
	User::LeaveIfError(HAL::Get(HALData::EFastCounterCountsUp, FastCounterCountsUp));

	TInt count = aBenchmarks.Count();
	TBool ok = ETrue;
	TUint categories = KCategoryGeneral;

	RArray<TInt> testsToRun;
	CleanupClosePushL(testsToRun);

	HBufC* buf = HBufC::NewLC(User::CommandLineLength());
	TPtr ptr = buf->Des();
	User::CommandLine(ptr);

	if (ptr != KNullDesC)
		{
		TLex lex(ptr);
		TPtrC16 token;

		while (ok && (token.Set(lex.NextToken()), token != KNullDesC))
			{
			if (token == _L("-r"))
				{
				token.Set(lex.NextToken());
				if (token == KNullDesC ||
					TLex(token).Val(RunLength) != KErrNone ||
					RunLength < 0.0)
					{
					BadUsage();
					ok = EFalse;
					}
				}
			else if (token == _L("-m"))
				{
				categories = KCategoryMemory;
				}
			else if (token == _L("-x"))
				{
				categories = KCategoryGeneral | KCategoryExtra;
				}
			else
				{
				TInt index;
				if (TLex(token).Val(index) != KErrNone)
					{
					BadUsage();
					ok = EFalse;
					}
				else if (index < 0 || index >= count)
					{
					test.Printf(_L("Index out of range: %d\n"), index);
					ok = EFalse;
					}
				else
					{
					testsToRun.AppendL(index);
					}
				}
			}
		}

	CleanupStack::PopAndDestroy(buf);

	if (ok)
		{
		test.Printf(_L("Note that these benchmarks are intended to guide optimisation, and not to\n"));
		test.Printf(_L("provide a meaningful indication of the speed of specific functions\n"));
		test.Printf(_L("\n"));
		if (testsToRun.Count() == 0)
			{
			for (TInt i = 0 ; i < count ; ++i)
				{				
				RunBenchmarkL(aBenchmarks, i, categories);
				}
			}
		else
			{
			for (TInt i = 0 ; i < testsToRun.Count() ; ++i)
				{
				RunBenchmarkL(aBenchmarks, testsToRun[i], categories);
				}
			}
		}
	
	CleanupStack::PopAndDestroy(&testsToRun);
	}
