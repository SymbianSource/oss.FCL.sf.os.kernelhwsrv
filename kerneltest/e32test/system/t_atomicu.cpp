// Copyright (c) 2003-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\t_atomicu.cpp
// Overview:
// Simple test for class User atomic operations
// API Information:
// User::SafeInc(), User::SafeDec(), User::LockedInc(),
// User::LockedDec()
// Details:
// - Tests SafeInc, SafeDec, LockedInc and LockedDec
// functions in single thread and determines that counts
// match after finished
// - Tests SafeInc, SafeDec, LockedInc and LockedDec
// functions in multithreaded configuration and determines
// that counts match after finished
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>
#include <e32def.h>

LOCAL_D RTest test(_L("T_ATOMICU"));

const TInt KMaxOps=20000;
#define KNumThreads 20
TInt gValue=0;

void TestSafeIncAndSafeDec()
	{
	gValue=0;
	// increasing when 0, should return 0
	test_Equal(0,User::SafeInc(gValue));
	// value also should be 0
	test_Equal(0,gValue);
	
	gValue=1;
	TInt expected=0;
	// gValue should vary only between 1 and 2
	for (TInt i=0; i<KMaxOps; i++)
		{
		expected=User::SafeInc(gValue)+1;
		test_Equal(expected,User::SafeDec(gValue));
		}

	// after running these, it should be 1
	test_Equal(1,gValue);
	
	// should stay zero after decreasing from 1 multiple times
	test_Equal(1,User::SafeDec(gValue));
	test_Equal(0,User::SafeDec(gValue));
	test_Equal(0,User::SafeDec(gValue));
	test_Equal(0,gValue);
	}

void TestLockedIncAndLockedDec()
	{
	gValue=0;
	// increasing when 0, should return 0 as old value
	test_Equal(0,User::LockedInc(gValue));
	// new value should be 1
	test_Equal(1,gValue);

	gValue=-1;

	// gValue should vary only between 1 and 2
	for (TInt i=0; i<KMaxOps; i++)
		{
		test_Equal((User::LockedInc(gValue)+1),User::LockedDec(gValue));
		}

	// after running these, it should be back in -1
	test_Equal(-1,gValue);
	}

TInt MultiThreadSafeIncAndSafeDec_FUNC(TAny*)
	{
	for (TInt i=0; i<KMaxOps; i++)
		{
		User::SafeInc(gValue);
		User::SafeDec(gValue);
		}
	return KErrNone;
	}

TInt MultiThreadLockedIncAndLockedDec_FUNC(TAny*)
	{
	for (TInt i=0; i<KMaxOps; i++)
		{
		User::LockedInc(gValue);
		User::LockedDec(gValue);
		}
	return KErrNone;
	}

void MultiThreadSafeIncAndSafeDec()
	{
	gValue=1; // start value 1
	RThread threads[KNumThreads];
	TRequestStatus stats[KNumThreads];
	TInt i;

	for (i=0;i<KNumThreads;i++)
		{
		test_KErrNone(threads[i].Create(KNullDesC,MultiThreadSafeIncAndSafeDec_FUNC,KDefaultStackSize,NULL,NULL));
		threads[i].Logon(stats[i]);
		}
	
	//lets increase our priority first, so that all the threads start
	RThread().SetPriority(EPriorityMore);

	test.Printf(_L("Resuming threads...\n"));
	for(i=0; i<KNumThreads; i++)
		{
		threads[i].Resume();
		}

	for(i=0; i<KNumThreads; i++)
		{
		User::WaitForRequest(stats[i]);
		test_KErrNone(stats[i].Int());
		CLOSE_AND_WAIT(threads[i]);
		}

	test.Printf(_L("...Threads finished\n"));
	
	// back to normal
	RThread().SetPriority(EPriorityNormal);

	// test that we returned to the startvalue
	test_Equal(1,gValue);
	}

void MultiThreadLockedIncAndLockedDec()
	{
	gValue=-1; // set start value to -1
	RThread threads[KNumThreads];
	TRequestStatus stats[KNumThreads];
	TInt i;

	for (i=0;i<KNumThreads;i++)
		{
		test_KErrNone(threads[i].Create(KNullDesC,MultiThreadLockedIncAndLockedDec_FUNC,KDefaultStackSize,NULL,NULL));
		threads[i].Logon(stats[i]);
		}
	
	//lets increase our priority first, so that all the threads start
	RThread().SetPriority(EPriorityMore);

	test.Printf(_L("Resuming threads...\n"));
	for(i=0; i<KNumThreads; i++)
		{
		threads[i].Resume();
		}

	for(i=0; i<KNumThreads; i++)
		{
		User::WaitForRequest(stats[i]);
		test_KErrNone(stats[i].Int());
		CLOSE_AND_WAIT(threads[i]);
		}

	test.Printf(_L("...Threads finished\n"));
	
	// back to normal
	RThread().SetPriority(EPriorityNormal);

	// test that we returned to the startvalue
	test_Equal(-1,gValue);
	}


TInt E32Main()
	{
	test.Title();
	
	test.Start(_L("Test single thread User::SafeInc and User::SafeDec"));
	TestSafeIncAndSafeDec();
	
	test.Next(_L("Test single thread User::LockedInc and User::LockedDec"));
	TestLockedIncAndLockedDec();

	test.Next(_L("Test multiple thread User::SafeInc and User::SafeDec"));
	MultiThreadSafeIncAndSafeDec();
	
	test.Next(_L("Test multiple thread User::LockedInc and User::LockedDec"));
	MultiThreadLockedIncAndLockedDec();

	test.End();

	return KErrNone;
	}
