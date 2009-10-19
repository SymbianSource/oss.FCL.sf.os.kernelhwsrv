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
// e32test\bench\t_userasmbm.cpp
// 
//

#include "t_userbm.h"
#include <e32test.h>
#include <e32std.h>
#include <e32std_private.h>

/**
 @SYMTestCaseID				KBASE-0047-t_userasmbm
 @SYMPREQ					PREQ521
 @SYMREQ                  	REQ4891
 @SYMTestCaseDesc 			This is a performance test used to get a relative indication
 							of the speed of assmbler functions in euser
 @SYMTestPriority   	 	Medium
 @SYMTestActions  	 		Runs benchmarks for euser functions
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

RTest test(_L("T_USERASMBM"));

TInt E32Main()
//
// Benchmark for euser assmblerised functions
//
    {
	CTrapCleanup* trapHandler=CTrapCleanup::New();
	test(trapHandler!=NULL);

    test.Title();
    test.Start(_L("Benchmarks for assemblerised euser functions"));

	TUserBenchmarkList bms;
	TRAPD(err, RunBenchmarkTestsL(bms));
	if (err != KErrNone)
		test.Printf(_L("TestMainL left with %d\n"), err);

	FailIfError(err);
	
	test.End();
	
	delete trapHandler;
	return(KErrNone);
    }
