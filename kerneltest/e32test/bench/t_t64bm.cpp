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
// e32test\bench\t_t64bm.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>

const TInt KHeapSize=0x2000;
const TInt KAverageOverInSeconds=10;
const TInt KNumberOfCalculationsPerLoop=100;

volatile TInt64 count;
volatile TInt64 a;
volatile TInt64 b;
volatile TInt64 c;
volatile TInt64 barrier;

RTest test(_L("T_T64BM"));

GLREF_C TInt TInt64Addition(TAny*);
GLREF_C TInt TInt64Subtraction(TAny*);
GLREF_C TInt TInt64Multiplication(TAny*);
GLREF_C TInt TInt64Division(TAny*);

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

	barrier = a;
	if (!barrier)
		barrier = c;
	barrier = b;
	if (!barrier)
		barrier = a;
	barrier = c;
	if (!barrier)
		barrier = b;

	thread.Kill(0);
	thread.Close();
	result*=KNumberOfCalculationsPerLoop;
	result/=KAverageOverInSeconds;
	r=I64INT(result);
    test.Printf(_L("%S executed %d in 1 second\n"),&aTitle,r);
    return(result);
    }

TInt E32Main()
//
// Benchmark for TInt64 functions
//
    {

    test.Title();
    test.Start(_L("Benchmarks for TInt64"));

	runTest(TInt64Addition,_L("Addition"));
	runTest(TInt64Subtraction,_L("Subtraction"));
	runTest(TInt64Multiplication,_L("Multiplication"));
	runTest(TInt64Division,_L("Division"));

	test.End();
	return(KErrNone);
    }

