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
// e32test\bench\t_ctxsw.cpp
// Overview:
// Test and benchmark inter-thread context switch.
// API Information:
// RThread, RSemaphore, TRequestStatus.
// Details:
// - Creates a thread with a handle to a Semaphore, check for successful 
// creation and assign priority EPriorityLess.
// - Creates another thread with the handle to the thread already created, 
// check for successful creation and assign priority EPriorityMuchLess.
// - Makes the two threads eligible for execution.
// - First thread counts the number of semaphore requests.
// - Second thread counts the number of times the thread is signalled on 
// completion of request.
// - Terminate the threads, print the count per second.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>

RTest test(_L("T_CTXSW"));

volatile TInt Count1;
volatile TInt Count2;

TInt Thread1(TAny* aSemaphore)
	{
	RSemaphore s;
	s.SetHandle((TInt)aSemaphore);
	User::WaitForAnyRequest();
	s.Signal();
	FOREVER
		{
		User::WaitForAnyRequest();
		Count1++;
		}
	}

TInt Thread2(TAny* aThread)
	{
	TRequestStatus s;
	RThread t;
	t.SetHandle((TInt)aThread);
	FOREVER
		{
		Count2++;
		TRequestStatus* ps=&s;
		t.RequestComplete(ps,0);
		}
	}

const TInt KHeapSize=4096;
void Test1()
	{
	test.Start(_L("Test 1"));

	RSemaphore sem;
	TInt r = sem.CreateLocal(0);
	test_KErrNone(r);
	RThread t1,t2;
	RThread* pt1, *pt2;

	pt1 = NULL;
	pt2 = NULL;

	Count1=Count2=0;
	r=t1.Create(_L("Thread1"),Thread1,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)sem.Handle());
	if (!pt1)
		pt1=&t1;
	else
		test(pt1==&t1);

	test_KErrNone(r);
	t1.SetPriority(EPriorityLess);
	r=t2.Create(_L("Thread2"),Thread2,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)t1.Handle());
		if (!pt2)
		pt2=&t2;
	else
		test(pt2==&t2);
	test_KErrNone(r);
	t2.SetPriority(EPriorityMuchLess);

	TRequestStatus s1,s2;
	t1.Logon(s1);
	t2.Logon(s2);
	t1.Resume();
	t2.Resume();
	sem.Wait();
	Count1=Count2=0;
	User::After(16000000);
	t2.Kill(0);
	User::WaitForRequest(s2);
	t1.Kill(0);
	User::WaitForRequest(s1);

	t2.Close();
	t1.Close();
	test.Printf(_L("Ran %d times in one second.\n"),(Count1+Count2)/16);  // get the value
	sem.Close();

	test.End();
	}

struct SThread3Info
	{
	TRequestStatus		iStatus;
	TRequestStatus		iLogonStatus;
	RThread				iThread;
	volatile TUint32	iCount;
	SThread3Info*		iOther;
	TUint32				iPadding[9];
	};

void Thread3(TAny* aInfo)
	{
	SThread3Info& me = *(SThread3Info*)aInfo;
	SThread3Info& you = *me.iOther;
	FOREVER
		{
		TRequestStatus* ps = &you.iStatus;
		you.iThread.RequestComplete(ps, 0);
		User::WaitForAnyRequest();
		++me.iCount;
		Count2++;
		}
	}

void Test2()
	{
	test.Start(_L("Test 2"));

	SThread3Info info[2];
	TInt r;
	TInt i;
	for (i=0; i<2; ++i)
		{
		info[i].iCount = 0;
		info[i].iOther = &info[1-i];
		r = info[i].iThread.Create(KNullDesC, (TThreadFunction)&Thread3, 0x1000, 0, &info[i]);
		test_KErrNone(r);
		info[i].iThread.Logon(info[i].iLogonStatus);
		test_Equal(KRequestPending, info[i].iLogonStatus.Int());
		}
	info[0].iThread.SetPriority(EPriorityMuchLess);
	info[1].iThread.SetPriority(EPriorityLess);
	info[0].iThread.Resume();
	info[1].iThread.Resume();
	User::AfterHighRes(100000);
	TUint32 ic0 = info[0].iCount;
	TUint32 ic1 = info[1].iCount;
	User::AfterHighRes(10000000);
	info[0].iThread.Kill(0);
	info[1].iThread.Kill(0);
	User::WaitForRequest(info[0].iLogonStatus);
	User::WaitForRequest(info[1].iLogonStatus);
	test_Equal(EExitKill, info[0].iThread.ExitType());
	test_Equal(0, info[0].iThread.ExitReason());
	test_Equal(EExitKill, info[1].iThread.ExitType());
	test_Equal(0, info[1].iThread.ExitReason());

	TUint32 fc0 = info[0].iCount;
	TUint32 fc1 = info[1].iCount;
	test.Printf(_L("T0: Initial %10u Final %10u Diff %10u PerSec %10u\n"), ic0, fc0, (fc0-ic0), (fc0-ic0)/10);
	test.Printf(_L("T1: Initial %10u Final %10u Diff %10u PerSec %10u\n"), ic1, fc1, (fc1-ic1), (fc1-ic1)/10);

	CLOSE_AND_WAIT(info[0].iThread);
	CLOSE_AND_WAIT(info[1].iThread);

	test.End();
	}

TInt E32Main()
	{
	
	test.Title();
	test.Start(_L("Timing inter-thread context switches..."));

	Test1();
	Test2();

	test.End();
	return 0;
	}
