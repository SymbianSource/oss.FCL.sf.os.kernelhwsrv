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
// e32test\misc\t_tmout.cpp
// 
//

#include <e32test.h>
#include "d_rndtim.h"

RTest test(_L("T_TMOUT"));

_LIT(KLitThread1Name,"IsrThread");
const TInt KThread1Priority = 56;
_LIT(KLitThread2Name,"WaitThread");
const TInt KThread2Priority = 55;

const TInt KTimeout = 5;	// milliseconds

volatile TUint32 RandomSignalInterval;
volatile TUint32 RandomSignalCount;
volatile TUint32 TotalCount;
volatile TUint32 TimeoutCount;
volatile TUint32 BadCount;
volatile TUint32 Bad0Count;
volatile TUint32 Bad1Count;
volatile TUint32 Calibration;

RThread IsrT;
RThread WaitT;
RSemaphore Sem;

TInt IsrThread(TAny*)
	{
	RThread me;
	RRndTim rt;
	TInt r = rt.Open();
	if (r!=KErrNone)
		return r;
	r = rt.SetPriority(me, KThread1Priority);
	if (r!=KErrNone)
		return r;
	rt.StartTimer();
	Calibration = rt.Calibrate(1024);
	r = rt.SetPriority(WaitT, KThread2Priority);
	if (r!=KErrNone)
		return r;
	WaitT.Resume();
	RThread::Rendezvous(KErrNone);
	FOREVER
		{
		rt.Wait();
		if (RandomSignalCount && !--RandomSignalCount)
			{
			RandomSignalCount = RandomSignalInterval;
			Sem.Signal();
			}
		}
	}

TInt WaitThread(TAny*)
	{
	TUint32 t1, t2;
	FOREVER
		{
		t1 = User::NTickCount();
		TInt r = Sem.Wait(KTimeout*1000);
		t2 = User::NTickCount();
		++TotalCount;
		if (r == KErrTimedOut)
			{
			++TimeoutCount;
			TInt d = (TInt)(t2-t1);
			if (d<KTimeout)
				++BadCount;
			if (d==0)
				++Bad0Count;
			if (d==1)
				++Bad1Count;
			}
		}
	}

GLDEF_C TInt E32Main()
	{
	test.Title();

	test.Start(_L("Load device driver"));
	TInt r = User::LoadLogicalDevice(_L("d_rndtim.ldd"));
	if (r == KErrNotFound)
		{
		test.Printf(_L("Test not supported on this platform\n"));
		test.End();
		return 0;
		}

	test.Next(_L("Create semaphore"));
	r = Sem.CreateLocal(0);
	test(r==KErrNone);

	test.Next(_L("Create ISR thread"));
	r=IsrT.Create(KLitThread1Name, &IsrThread, 0x1000, NULL, NULL);
	test(r==KErrNone);

	test.Next(_L("Create wait thread"));
	r=WaitT.Create(KLitThread2Name, &WaitThread, 0x1000, NULL, NULL);
	test(r==KErrNone);

	TRequestStatus s;
	IsrT.Rendezvous(s);
	test(s==KRequestPending);

	IsrT.Resume();
	User::WaitForRequest(s);
	test(s==KErrNone);
	test(IsrT.ExitType() == EExitPending);
	test(WaitT.ExitType() == EExitPending);

	test.Printf(_L("%d random timers in 1024ms"), Calibration);
	TUint32 interval = (KTimeout * Calibration + 512)/1024;
	test.Printf(_L("Interval %d"), interval);
	RandomSignalInterval = interval;
	RandomSignalCount = interval;

	volatile TInt forever = 1;
	while(forever)
		{
		test.Printf(_L("Total: %8d Timeout: %8d Bad: %4d Bad0: %4d Bad1: %4d\n"), TotalCount, TimeoutCount, BadCount, Bad0Count, Bad1Count);
		User::After(1000000);
		}
	return 0;
	}
