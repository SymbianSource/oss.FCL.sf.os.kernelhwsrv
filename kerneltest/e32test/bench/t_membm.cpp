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
// e32test\bench\t_membm.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32svr.h>

#define ALIGN_UP(x, y) (TInt8*)(_ALIGN_UP((TInt)(x), (y)))
#define ALIGN_DOWN(x, y) (TInt8*)(_ALIGN_DOWN((TInt)(x), (y)))

const TInt KHeapSize=720*1024;
const TInt KAverageOverInMilliseconds=500;
const TInt Len64K = 64*1024;

volatile TInt count;
volatile TInt iters;

TInt8* trg;
TInt8* src;
TInt8* dummy;
TInt8 trgoffset;
TInt8 srcoffset;
TInt len=64*1024;
TInt64 result;

RTest test(_L("T_MEMBM"));

GLREF_C TInt MemBaseline(TAny*);
GLREF_C TInt MemCopy(TAny*);
GLREF_C TInt MemMove(TAny*);
GLREF_C TInt MemFill(TAny*);
GLREF_C TInt MemSwap(TAny*);
GLREF_C TInt WordMove(TAny*);
GLREF_C TInt MemCopyUncached(TAny*);
GLREF_C TInt WordMoveUncached(TAny*);
GLREF_C TInt MemFillUncached(TAny*);
GLREF_C TInt PurgeCache();

TInt nextLen(TInt aTestLength)
	{
	if (len == Len64K)
		return 0;
	if (!aTestLength)
		return Len64K;

	TInt inc = aTestLength;
	for (TInt i = len >> 5 ; i ; i >>= 1)
		inc <<= 1;

	return len + inc;
	}

/**
 * Run benchmarks on the supplied function.
 *
 * @param aFunction			The function to benchmark
 * @param aTestBackwards	Run copy tests with overlapping areas to test backwards copy
 * @param aTestSrcAlign		Run tests for source alignments 0 - 31
 * @param aTestDestAlign	Run tests for destination alignments 0 - 31
 * @param aTestLength		If non-zero, run tests for different lengths starting at aTestLength
 */
TInt64 runTest(TThreadFunction aFunction,const TDesC& aTitle, TBool aTestBackwards, TBool aTestSrcAlign, TBool aTestDestAlign, TInt aTestLength)
    {
	TInt8* buffer = (TInt8*)User::Alloc(640 * 1024 + 32);
	test(buffer != 0);

	test(!(aTestLength && aTestBackwards)); // not supported
	
	TBool goingforward=ETrue;

	FOREVER
		{
		if (aTestLength)
			test.Printf(_L("Running length experiment on %S.  Results follow:\n"), &aTitle);
		
		len = 0;
		while(len = nextLen(aTestLength), len)
			{
			src = ALIGN_UP(buffer, 32);
			if (goingforward)
				trg = src + 512 * 1024;					// blow half a meg
			else
				trg = ALIGN_DOWN(src + len - 1, 32);	// ensure overlapping, doesn't work for length < 32

			if (aTestLength)
				test.Printf(_L("%d, "), len);
			else
				test.Printf(_L("Test array bases trg=0x%08x, src=0x%08x.  Running experiment on %S.  Results follow:\n"), trg, src, &aTitle);
			
			const TInt xsquare = aTestDestAlign ? 32 : 1;
			const TInt ysquare = aTestSrcAlign ? 32 : 1;

			for (srcoffset = 0 ; srcoffset < ysquare ; srcoffset++)
				{
				for (trgoffset = 0 ; trgoffset < xsquare ; trgoffset++)
					{
					RThread thread;
					TInt r=thread.Create(aTitle,aFunction,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
					if(r!=KErrNone)
						{
						test.Printf(_L("Failed to create thread with error %d\n"),r);
						return(r);
						}
					thread.SetPriority(EPriorityLess);
					thread.Resume();
					User::After(50000);  // 50 msec is more than enough in EKA2
					count=0;
					User::After(5000);   // even if the second reset fails, max inaccuracy is 5000 usecs
					count=0;			 // try and reduce the probability of failing to reset
					User::After(KAverageOverInMilliseconds*1000);
					result=count;
					thread.Kill(0);
					CLOSE_AND_WAIT(thread);
					PurgeCache();

					result *= 1000;
					result /= KAverageOverInMilliseconds;

					TInt s = I64INT(result);
					test.Printf(_L("%d, "),s);
					}
				test.Printf(_L("\n"));
				}
			}

		if (aTestBackwards && goingforward)
			goingforward = EFalse;
		else
			break;
		}
	
	User::Free(buffer);

	return(result);
    }

enum TTestType
	{
	ENormalTests,
	EFullTests,
	ELengthTests,
	};

// Return whether we should run the full alignment benchmarks
TTestType ParseCommandLine()
	{
	TBuf<32> args;
	User::CommandLine(args);
	
	if (args == _L("-f"))
		return EFullTests;
	else if (args == _L("-l"))
		return ELengthTests;
	else if (args != KNullDesC)
		{
		test.Printf(_L("usage: t_membm [OPTIONS]\n"));
		test.Printf(_L("  -f  Run full alignment benchmarks\n"));
		test.Printf(_L("  -l  Run memcpy length benchmarks\n"));
		test(EFalse);
		}
	
	return ENormalTests;
	}

TInt E32Main()
//
// Benchmark for Mem functions
//
    {

    test.Title();
    test.Start(_L("Benchmarks for Mem functions"));

	TTestType testType = ParseCommandLine();

	switch (testType)
		{
		case ENormalTests:
		case EFullTests:
			{
			TBool srcAlign = testType == EFullTests;
			
			if (srcAlign)
				test.Printf(_L("Running full alignment benchmarks (may take a long time)\n"));
			else
				test.Printf(_L("Not testing source alignment (run with -f if you want this)\n"));
			
			runTest(MemBaseline,     _L("Processor baseline"),   EFalse, EFalse,   EFalse, 0);
			runTest(MemFill,	     _L("Memory fill"),		     EFalse, EFalse,   ETrue,  0);
			runTest(MemCopy,	     _L("Memory copy"),		     ETrue,  srcAlign, ETrue,  0);
			runTest(MemSwap,	     _L("Memory swap"),		     ETrue,  srcAlign, ETrue,  0);
			}
			break;
		case ELengthTests:
			test.Printf(_L("Running length benchmarks (may take a long time)\n"));
			runTest(MemFill,         _L("Fill length cached"),        EFalse,  EFalse,  EFalse, 1);
			runTest(MemFillUncached, _L("Fill length uncached"),      EFalse,  EFalse,  EFalse, 1);
			runTest(MemCopy,         _L("Copy length cached"),        EFalse,  EFalse,  EFalse, 1);
			runTest(MemCopyUncached, _L("Copy length uncached"),      EFalse,  EFalse,  EFalse, 1);
			runTest(WordMove,        _L("Word move length cached"),   EFalse,  EFalse,  EFalse, 4);
			runTest(WordMoveUncached,_L("Word move length uncached"), EFalse,  EFalse,  EFalse, 4);
			break;
		default:
			test(EFalse);
			break;
		}

    test.End();
	return(KErrNone);
    }
