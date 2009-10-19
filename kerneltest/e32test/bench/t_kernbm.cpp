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
// e32test\bench\t_kernbm.cpp
// Overview:
// Benchmark the performance of some kernel functions including RTimer
// and RThread. Calculation time required for creation & closing of
// timer objects, suspension of threads, handling kernel messages, and 
// the number of threads created and stopped within a fixed time period.
// API Information:
// RTimer, RThread.
// Details:
// - Calculate and display the time required to create and close 1000 
// thread-relative timer objects.
// - Count and print the time taken to suspend the thread for 100000 times.
// - Count and display the time consumed for 100000 kernel messages.
// - Get and print the number of threads created and stopped within a 
// specified timeframe, verify the ExitType, and ExitReason of the threads.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

#define	TIMECOUNT()	User::NTickCount()
#define TIMEDIFFMS(i,f)	(f-i)

LOCAL_D RTest test(_L("T_KERNBM"));
volatile TInt Count;
volatile TBool Stop;

#ifdef __EPOC32__
extern void DummyKernMsg();
#else
void DummyKernMsg()
	{}
#endif

LOCAL_C TInt ThreadFunction(TAny*)
	{
	User::WaitForAnyRequest();
	return 0;
	}

TInt TestThread(TAny*)
	{
	return 0;
	}

TInt TestThreadCreate(TAny*)
	{
	RThread t;
	t.SetPriority(EPriorityAbsoluteVeryLow);
	while (!Stop)
		{
		t.Create(KNullDesC,TestThread,4096,NULL,NULL);
		t.Resume();
		t.Close();
		++Count;
		}
	return 0;
	}

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Start...\n"));

	RTimer t;
	TInt i;
	TInt r;
	TUint32 c1=TIMECOUNT();
	for (i=0; i<1000; i++)
		{
		r=t.CreateLocal();
		if (r!=KErrNone)
			test(0);
		t.Close();
		}
	TUint32 c2=TIMECOUNT();
	test.Printf(_L("Create and close 1000 timers takes %d ms\n"),TIMEDIFFMS(c1,c2));

	RThread thrd;
	r=thrd.Create(_L("DodgyThread"),ThreadFunction,4096,1024,8192,NULL);
	test(r==KErrNone);
	test.Printf(_L("DodgyThread created\n"));
	thrd.SetPriority(EPriorityMore);
	thrd.Resume();
	test.Printf(_L("DodgyThread resumed\n"));
	c1=TIMECOUNT();
	for (i=0; i<100000; i++)
		{
		thrd.Suspend();
		}
	c2=TIMECOUNT();
	test.Printf(_L("Suspend 100000 times takes %d ms\n"),TIMEDIFFMS(c1,c2));
	thrd.Kill(0);
	thrd.Close();
	test.Printf(_L("DodgyThread killed\n"));

	c1=TIMECOUNT();
	for (i=0; i<100000; i++)
		{
		DummyKernMsg();
		}
	c2=TIMECOUNT();
	test.Printf(_L("100000 kernel messages take %d ms\n"),TIMEDIFFMS(c1,c2));

	test.Next(_L("Create thread test"));
	r=thrd.Create(_L("Creator"),TestThreadCreate,4096,NULL,NULL);
	test(r==KErrNone);
	thrd.SetPriority(EPriorityLess);
	RThread().SetPriority(EPriorityMuchMore);
	TRequestStatus s;
	thrd.Logon(s);
	thrd.Resume();
	Count=0;
	Stop=EFalse;
	c1=TIMECOUNT();
	User::After(1000000);
	Stop=ETrue;
	User::WaitForRequest(s);
	c2=TIMECOUNT();
	test(thrd.ExitType()==EExitKill);
	test(thrd.ExitReason()==0);
	thrd.Close();
	test.Printf(_L("%d threads created and stopped in %dms\n"),Count,TIMEDIFFMS(c1,c2));

	test.End();
	return 0;
	}

