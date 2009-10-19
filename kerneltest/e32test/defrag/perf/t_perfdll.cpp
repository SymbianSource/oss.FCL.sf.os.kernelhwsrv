// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/defrag/perf/t_perfdll.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32hal.h>
#include "..\d_pagemove.h"
#include "t_perf.h"
#include "t_testdll.h"

_LIT(KDllbaseName, "t_defragperf");

TPtrC TestPlExtNames = _L(".dll");

const TInt KIterations = 50;

#define MAXDLLS 1


#define MINDLL_PAGES (100 * E_SIZE)

typedef TInt (*TCallFunction)(TUint32 funcIndex, TInt param1, TInt param2);

TInt DllDefrag::LoadTheLib(TInt aIdx, RTest& test)
	{

	TBuf<128>           nameBuffer;
	TUint32             FuncCount;
	TLibraryFunction    InitFunc;
	TLibraryFunction    FunctionCountFunc;
	TCallFunction       CallFunctionFunc;
	TLibraryFunction    SetCloseFunc;
	TLibraryFunction    Function0AddrFunc, FunctionNAddrFunc;
	TInt retVal;

	if (iLib == NULL)
		return KErrNoMemory;


	nameBuffer.Format(_L("%S%d%S"), &KDllbaseName, aIdx, &TestPlExtNames);

	/* Load the DLL */
	TEST_PRINTF(_L("Loading the DLL (%S) ...\n"), &nameBuffer);

	retVal = iLib->Load(nameBuffer);

	if (retVal != KErrNone)
		{
		test.Printf(_L("Load Failed %S (%d)\n"), &nameBuffer, aIdx);
		return KErrGeneral;
		}

	TEST_PRINTF(_L("Loaded %S (%d)\n"), &nameBuffer, aIdx);

	/* Test that the DLL is loaded correctly */
	InitFunc = iLib->Lookup(DEFRAGDLL_FUNC_Init);
	FunctionCountFunc = iLib->Lookup(DEFRAGDLL_FUNC_FunctionCount);
	CallFunctionFunc = (TCallFunction)iLib->Lookup(DEFRAGDLL_FUNC_CallFunction);
	SetCloseFunc = iLib->Lookup(DEFRAGDLL_FUNC_SetClose);
	Function0AddrFunc = iLib->Lookup(DEFRAGDLL_FUNC_func0);
	FunctionNAddrFunc = iLib->Lookup(DEFRAGDLL_FUNC_funcN);

	if (InitFunc == NULL ||
		FunctionCountFunc == NULL ||
		CallFunctionFunc == NULL ||
		SetCloseFunc == NULL)
		{
		return KErrGeneral;
		}

	retVal = (InitFunc)();
	if (retVal != KErrNone)
		return KErrGeneral;

	FuncCount = (FunctionCountFunc)();
	if (FuncCount == 0)
		return KErrGeneral;

	iFunc0Addr = (TInt8 *)(Function0AddrFunc)();
	iFuncNAddr = (TInt8 *)(FunctionNAddrFunc)();
		
	TEST_PRINTF(_L("%S:Func0 = %x FuncN = %x\n"), &nameBuffer, iFunc0Addr, iFuncNAddr);
	
	if (iFunc0Addr == NULL || iFuncNAddr == NULL)
			return KErrGeneral;

	if (iFunc0Addr > iFuncNAddr) 
		return KErrGeneral;

	return KErrNone;
	}

TInt DllDefrag::TestDLLPerformance(TInt aDummy, RPageMove& aPageMove, RTest& test)
{
	DefragLatency timer;
	TInt size = iFuncNAddr - iFunc0Addr;
	const TUint8 *end = (TUint8 *)iFunc0Addr + size; 

	TInt pageSize;
	TInt r = UserHal::PageSizeInBytes(pageSize);
	test_KErrNone(r);
	TEST_PRINTF(_L("size %x"), size);
	TInt pagesToMove = _ALIGN_UP(size, pageSize) / pageSize;
	// Ensure there is at least one page to move
	test_Compare(size, >=, 1);

	timer.CalibrateTimer(test);


	for (TInt j = 0; j < KIterations; j++)
		{
		TUint8* base = (TUint8 *)iFunc0Addr;
				
		timer.StartTimer();
		while (base < end) 
			{
			test_KErrNone(aPageMove.TryMovingUserPage(base));
			base += pageSize;
			}
		timer.StopTimer(test);
		}
	
	DTime_t average, max, min, delay;
	TEST_PRINTF(_L("Finished moving %d dll pages\n"), pagesToMove);
	average = timer.GetResult(max, min, delay);
	test.Printf(_L("Fast counter ticks to move %d dll pages: Av %d Min %d Max %d (Overhead %d)\n"), pagesToMove, average, min, max, delay);
	test.Printf(_L("Average of %d ticks to move one page\n\n"), average / pagesToMove);

	return KErrNone;
	}


