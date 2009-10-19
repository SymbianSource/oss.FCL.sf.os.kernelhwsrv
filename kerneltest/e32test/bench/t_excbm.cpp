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
// e32test\bench\t_excbm.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32svr.h>

const TInt KHeapSize=132*1024;
const TInt KAverageOverInSeconds=10;
const TInt KNumberOfCalculationsPerLoop=20;

TInt64 count;

RTest test(_L("T_EXCBM"));

GLREF_C TInt SlowExec(TAny*);
GLREF_C TInt FastExec(TAny*);

TInt64 runTest(TThreadFunction aFunction,const TDesC& aTitle)
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
    count=0;
    User::After(KAverageOverInSeconds*1000000);
    TInt64 result=count;
    thread.Kill(0);
	thread.Close();
    result *= KNumberOfCalculationsPerLoop;
    result /= KAverageOverInSeconds;
    TInt r2 = I64INT(result);
    test.Printf(_L("%S executed %d in 1 second\n"),&aTitle,r2);
    return(result);
    }

TInt E32Main()
//
// Benchmark for Exec functions
//
    {

    test.Title();
    test.Start(_L("Benchmarks for Exec functions"));
    runTest(SlowExec,_L("Slow Exec"));
    runTest(FastExec,_L("Fast Exec"));
    test.End();
	return(KErrNone);
    }

