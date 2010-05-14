// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\bench\t_exec.cpp
// 
//

#include <e32test.h>

const TInt KHeapSize=0x2000;


LOCAL_D RTest test(_L("T_EXEC"));
LOCAL_D RTest testSvr(_L("Server"));
LOCAL_D RSemaphore semmy;
LOCAL_D TInt speedCount;

LOCAL_C TInt speedyThreadEntryPoint(TAny*)
//
// The entry point for the speed test thread.
//
	{

	speedCount=0;
	semmy.Signal();
	TUint myChar='a';
	for (TUint i=0;i<0xffffffff;i++)
		{
		User::UpperCase(myChar);
		speedCount++;
		}
	return(KErrNone);
	}

GLDEF_C TInt E32Main()
//
// Test timers.
//
    {

	test.Title();
	test.Start(_L("Creating semaphore"));
	TInt r=semmy.CreateLocal(0);
	test(r==KErrNone);
//
	test.Next(_L("Starting speedy thread"));
	RThread speedy;
	r=speedy.Create(_L("Speedy"),speedyThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	speedy.Resume();
//
	test.Next(_L("Wait for speedy to start"));
	semmy.Wait();
//
	test.Printf(_L("Starting exec speed test...\n"));
	User::After(0);
	TInt b=speedCount;
	User::After(1000000);
	TInt n=speedCount;
	test.Printf(_L("Count = %d exec calls in 1 second\n"),n-b);
//
	test.Next(_L("Kill speedy"));
	speedy.Kill(0x666);
	speedy.Close();
//
	test.End();
	return(0);
    }

