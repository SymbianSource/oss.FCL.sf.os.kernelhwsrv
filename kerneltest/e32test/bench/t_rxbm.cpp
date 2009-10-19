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
// e32test\bench\t_rxbm.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>

const TInt KHeapSize=0x2000;
const TInt KAverageOverInSeconds=10;
const TInt KNumberOfCalculationsPerLoop=10;

volatile TInt count;
RTest test(_L("T_RXBM"));

GLREF_C TInt TRealXAddition(TAny*);
GLREF_C TInt TRealXSubtraction(TAny*);
GLREF_C TInt TRealXMultiplication(TAny*);
GLREF_C TInt TRealXDivision(TAny*);

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
    count=0;
    User::After(KAverageOverInSeconds*1000000);
    TInt result=count;
    thread.Kill(0);
	thread.Close();
    test.Printf(_L("%S executed %d in 1 second\n"),&aTitle,result*KNumberOfCalculationsPerLoop/KAverageOverInSeconds);
    return(result);
    }

TInt E32Main()
//
// Benchmark for TRealX functions
//
    {
    test.Title();
    test.Start(_L("Benchmarks for TRealX"));

	runTest(TRealXAddition,_L("Addition"));
	runTest(TRealXSubtraction,_L("Subtraction"));
	runTest(TRealXMultiplication,_L("Multiplication"));
	runTest(TRealXDivision,_L("Division"));

	test.End();
	return(KErrNone);
    }

