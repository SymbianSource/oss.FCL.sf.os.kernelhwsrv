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
// e32test\bench\t_desbm.cpp
// 
//

#include "t_userbm.h"
#include <e32test.h>
#include <e32std.h>
#include <e32std_private.h>

#define FailIfError(EXPR) \
	{ \
	TInt aErr = (EXPR); \
	if (aErr != KErrNone) \
		{ \
		test.Printf(_L("Return code == %d\n"), aErr); \
		test(EFalse); \
		} \
	}

RTest test(_L("T_DESBM"));

TInt E32Main()
//
// Benchmark for descriptor functions
//
    {
	CTrapCleanup* trapHandler=CTrapCleanup::New();
	test(trapHandler!=NULL);

    test.Title();
    test.Start(_L("Benchmarks for descriptor functions"));

	TUserBenchmarkList bms;
	TRAPD(err, RunBenchmarkTestsL(bms));
	if (err != KErrNone)
		test.Printf(_L("TestMainL left with %d\n"), err);
	
	FailIfError(err);
	
	test.End();
	
	delete trapHandler;
	return(KErrNone);
    }
