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
// e32test\bench\t_kernasmbm.cpp
// 
//

#include "d_kernasmbm.h"
#include <e32std.h>
#include <e32std_private.h>
#include <e32test.h>

/**
 @SYMTestCaseID				KBASE-0046-t_kernasmbm
 @SYMPREQ					PREQ521
 @SYMREQ                  	REQ4891
 @SYMTestCaseDesc 			This is a performance test used to get a relative indication
 							of the speed of assmbler functions in the kernel
 @SYMTestPriority   	 	Medium
 @SYMTestActions  	 		Runs benchmarks for kernel functions
 @SYMTestExpectedResults  	Results are for information - no specific results expected
*/

#define FailIfError(EXPR) \
	{ \
	TInt aErr = (EXPR); \
	if (aErr != KErrNone) \
		{ \
		test.Printf(_L("Return code == %d\n"), aErr); \
		test(EFalse); \
		} \
	}

_LIT(KLddFileName, "D_KERNASMBM.LDD");
RTest test(_L("T_KERNASMBM"));

RKernAsmBmLdd TestLdd;

void InitDataL()
	{
	// Load device driver
	User::LeaveIfError(User::LoadLogicalDevice(KLddFileName));
	User::LeaveIfError(TestLdd.Open());
	}

TInt E32Main()
    {
	CTrapCleanup* trapHandler=CTrapCleanup::New();
	test(trapHandler!=NULL);

    test.Title();
    test.Start(_L("Benchmarks for assmblerised kernel functions\n"));

	TRAPD(err, RunBenchmarkTestsL(TestLdd));
	if (err != KErrNone)
		test.Printf(_L("TestMainL left with %d\n"), err);
	
	TestLdd.Close();
	User::After(10 * 1000);
	FailIfError(User::FreeLogicalDevice(KKernAsmBmLddName));

	FailIfError(err);

	test.End();

	delete trapHandler;
	return(KErrNone);
    }
