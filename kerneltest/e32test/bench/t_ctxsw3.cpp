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
// e32test\bench\t_ctxsw3.cpp
// Overview:
// Benchmark the inter-thread context switching. 
// API Information:
// RThread, RRSemaphore.
// Details:
// - Create a semaphore.
// - Create a thread (thread1) with a handle to the Semaphore and 
// priority EPriorityLess. Verify the successful creation.
// - Create another thread (thread2) with a handle to the Semaphore 
// and priority EPriorityMuchLess. Verify the successful creation.
// - thread1 counts the number of times the semaphore count is decremented.
// - thread2 counts the number of times the semaphore count is incremented.
// - Display the number of times the semaphore is signalled per second.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

RTest test(_L("T_CTXSW3"));

LOCAL_D TInt Count;

LOCAL_D RSemaphore StartSem;

LOCAL_C TInt Thread1(TAny* aSemaphore)
	{
	RSemaphore s;
	s.SetHandle((TInt)aSemaphore);
	StartSem.Wait();
	FOREVER
		{
		s.Wait();
		Count++;
		}
	}

LOCAL_C TInt Thread2(TAny* aSemaphore)
	{
	RSemaphore s;
	s.SetHandle((TInt)aSemaphore);
	StartSem.Wait();
	FOREVER
		{
		s.Signal();
		Count++;
		}
	}

const TInt KHeapSize=4096;

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Timing inter-thread context switches..."));

	TInt r=StartSem.CreateLocal(0);
	test(r==KErrNone);

	RSemaphore sem;
	r=sem.CreateLocal(0);
	test(r==KErrNone);
	RThread t1,t2;
	r=t1.Create(_L("Thread1"),Thread1,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)sem.Handle());
	test(r==KErrNone);
	t1.SetPriority(EPriorityLess);
	r=t2.Create(_L("Thread2"),Thread2,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)sem.Handle());
	test(r==KErrNone);
	t2.SetPriority(EPriorityMuchLess);

	TRequestStatus s1,s2;
	t1.Logon(s1);
	t2.Logon(s2);
	t1.Resume();
	t2.Resume();

	Count=0;
	User::After(20000);
	StartSem.Signal(2);
	User::After(2000000);
	TInt count=Count;

	t2.Kill(0);
	User::WaitForRequest(s2);
	t1.Kill(0);
	User::WaitForRequest(s1);
	t2.Close();
	t1.Close();
	sem.Close();

	test.Printf(_L("%d per second\n"),count/2);

	test.End();
	return 0;
	}
