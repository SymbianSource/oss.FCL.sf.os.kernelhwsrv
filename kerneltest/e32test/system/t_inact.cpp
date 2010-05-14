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
// e32test\system\t_inact.cpp
// Test inactivity timers
// 
//

#include <e32test.h>

RTest test(_L("T_INACT"));

class CTimer1 : public CTimer
	{
public:
	static CTimer1* NewL();
	CTimer1();
	void Start();
	virtual void RunL();
	};

class CTimer2 : public CTimer
	{
public:
	static CTimer2* NewL();
	CTimer2();
	void Start();
	virtual void RunL();
	};

class CTimer3 : public CTimer
	{
public:
	static CTimer3* NewL();
	CTimer3();
	void Start();
	virtual void RunL();
public:
	TInt iInactive;
	};

class CRepeatedTimer : public CTimer
	{
public:
	static CRepeatedTimer* NewL(TInt aCount, TInt aPeriod);
	CRepeatedTimer();
	void Start();
	virtual void RunL();
public:
	TInt iCount;
	TInt iPeriod;
	};

CActiveScheduler* ActiveSched;
CTimer1* Timer1;
CTimer2* Timer2;
CTimer3* Timer3;
CRepeatedTimer* RepTimer;

CTimer1::CTimer1()
	: CTimer(EPriorityStandard)
	{
	}

CTimer1* CTimer1::NewL()
	{
	CTimer1* pT=new (ELeave) CTimer1;
	CleanupStack::PushL(pT);
	pT->ConstructL();
	CleanupStack::Pop();
	return pT;
	}

void CTimer1::Start()
	{
	Inactivity(7);
	}

void CTimer1::RunL()
	{
	test.Printf(_L("CTimer1 expired\n"));
	Start();
	}

CTimer2::CTimer2()
	: CTimer(EPriorityStandard)
	{
	}

CTimer2* CTimer2::NewL()
	{
	CTimer2* pT=new (ELeave) CTimer2;
	CleanupStack::PushL(pT);
	pT->ConstructL();
	CleanupStack::Pop();
	return pT;
	}

void CTimer2::Start()
	{
	Inactivity(13);
	}

void CTimer2::RunL()
	{
	test.Printf(_L("CTimer2 expired\n"));
	Start();
	}

CTimer3::CTimer3()
	: CTimer(EPriorityStandard)
	{
	}

CTimer3* CTimer3::NewL()
	{
	CTimer3* pT=new (ELeave) CTimer3;
	CleanupStack::PushL(pT);
	pT->ConstructL();
	CleanupStack::Pop();
	return pT;
	}

void CTimer3::Start()
	{
	iInactive=User::InactivityTime().Int();
	After(500000);
	}

void CTimer3::RunL()
	{
	TInt inactive=User::InactivityTime().Int();
	if (inactive!=iInactive)
		{
		iInactive=inactive;
		test.Printf(_L("%d\n"),inactive);
		}
	After(500000);
	}

CRepeatedTimer::CRepeatedTimer()
	: CTimer(EPriorityStandard)
	{
	}

CRepeatedTimer* CRepeatedTimer::NewL(TInt aCount, TInt aPeriod)
	{
	CRepeatedTimer* pT=new (ELeave) CRepeatedTimer;
	pT->iCount=aCount;
	pT->iPeriod=aPeriod;
	CleanupStack::PushL(pT);
	pT->ConstructL();
	CleanupStack::Pop();
	return pT;
	}

void CRepeatedTimer::Start()
	{
	Inactivity(iPeriod);
	}

void CRepeatedTimer::RunL()
	{
	test.Printf(_L("RepeatTimer expired %d\n"),iCount);
	if (--iCount)
		Start();
	else
		CActiveScheduler::Stop();
	}

void InitialiseL()
	{
	ActiveSched=new (ELeave) CActiveScheduler;
	Timer1=CTimer1::NewL();
	Timer2=CTimer2::NewL();
	Timer3=CTimer3::NewL();
	RepTimer=CRepeatedTimer::NewL(5,10);
	CActiveScheduler::Install(ActiveSched);
	CActiveScheduler::Add(Timer1);
	CActiveScheduler::Add(Timer2);
	CActiveScheduler::Add(Timer3);
	CActiveScheduler::Add(RepTimer);
	Timer1->Start();
	Timer2->Start();
	Timer3->Start();
	RepTimer->Start();
	}

void TestNonPositiveTimeout()
	{
	TRequestStatus x1,x2;
	RTimer rt1,rt2;

	test.Next(_L("Test RTimer::Inactivity() with zero timeout"));
	rt1.CreateLocal();
	rt2.CreateLocal();

	rt1.Inactivity(x1, 2);
	User::After(500000);
	rt2.Inactivity(x2, 0);
	User::ResetInactivityTime();

	User::WaitForRequest(x1);
	test(x1==KErrNone);
	User::WaitForRequest(x2);
	test(x2==KErrNone);

	test.Next(_L("Test RTimer::Inactivity() with negative timeout"));
	rt1.Inactivity(x1, -1);
	User::WaitForRequest(x1);
	test(x1==KErrArgument);
	}

GLDEF_C TInt E32Main()
	{
	test.Title();
	__UHEAP_MARK;
	test.Start(_L("Test RTimer::Inactivity"));

	TestNonPositiveTimeout();

	RTimer t1;
	RTimer t2;
	TInt r=t1.CreateLocal();
	test(r==KErrNone);
	r=t2.CreateLocal();
	test(r==KErrNone);
	test.Printf(_L("\nPress a key...\n"));
	test.Getch();
	TRequestStatus s;
	t1.Inactivity(s,5);
	TTime before;
	before.UniversalTime();
	test.Printf(_L("Wait... "));
	User::WaitForRequest(s);
	TTime after;
	after.UniversalTime();
	TTimeIntervalMicroSeconds interval=after.MicroSecondsFrom(before);
	TInt interval_int=I64INT(interval.Int64());
	test.Printf(_L("Timer expired after %d us\n"),interval_int);
	test.Printf(_L("Press a key...\n"));
	test.Getch();
	t1.Inactivity(s,5);
	before.UniversalTime();
	test.Printf(_L("Test changing time"));
	r=User::SetUTCTime(before+TTimeIntervalDays(1));
	test(r==KErrNone);
	test(s==KRequestPending);	// make sure time change doesn't trigger inactivity timers
	r=User::SetUTCTime(before-TTimeIntervalDays(1));
	test(r==KErrNone);
	test(s==KRequestPending);	// make sure time change doesn't trigger inactivity timers
	r=User::SetUTCTime(before);
	test(r==KErrNone);
	test(s==KRequestPending);	// make sure time change doesn't trigger inactivity timers

	TTime secure;
	if ((r = secure.UniversalTimeSecure()) == KErrNone)
		r = User::SetUTCTimeSecure(secure-TTimeIntervalDays(1));
	if (r != KErrNone)
		{
		RDebug::Printf("WARNING: Secure clock change test skipped because secure time could not be changed!");
		}
	else
		{
		User::SetUTCTimeSecure(secure+TTimeIntervalDays(1));
		User::SetUTCTimeSecure(secure);
		test(s == KRequestPending);	// make sure secure time change doesn't trigger inactivity timers
		}

	User::WaitForRequest(s);
	after.UniversalTime();
	interval=after.MicroSecondsFrom(before);
	interval_int=I64INT(interval.Int64());
	test.Printf(_L("Timer expired after %d us\n"),interval_int);
	test.Printf(_L("Press a key...\n"));
	test.Getch();

	TInt inactive=User::InactivityTime().Int();
	TRequestStatus s1;
	TRequestStatus s2;
	t1.Inactivity(s1, 10);
	t2.Inactivity(s2, 15);
	FOREVER
		{
		TInt new_inact=User::InactivityTime().Int();
		if (new_inact!=inactive)
			{
			inactive=new_inact;
			test.Printf(_L("%d\n"),inactive);
			}
		if (s2!=KRequestPending)
			{
			User::WaitForRequest(s2);
			test(s2.Int()==KErrNone);
			test.Printf(_L("Timer 2 expired\n"));
			break;
			}
		if (s1!=KRequestPending)
			{
			User::WaitForRequest(s1);
			test(s1.Int()==KErrNone);
			test.Printf(_L("Timer 1 expired\n"));
			s1=KRequestPending;
			}
		}

	test.Next(_L("Test RTimer::Cancel()"));
	test.Printf(_L("Press a key...\n"));
	test.Getch();
	t1.Inactivity(s1, 300);
	t1.Cancel();
	User::WaitForRequest(s1);
	test(s1==KErrCancel);

	test.Next(_L("Test CTimer::Inactivity()"));
	test.Printf(_L("Press a key...\n"));
	test.Getch();

	CTrapCleanup* pC=CTrapCleanup::New();
	test(pC!=NULL);
	TRAP(r,InitialiseL());
	test(r==KErrNone);

	CActiveScheduler::Start();

	Timer1->Cancel();
	Timer2->Cancel();
	Timer3->Cancel();
	RepTimer->Cancel();

	delete Timer1;
	delete Timer2;
	delete Timer3;
	delete RepTimer;
	delete ActiveSched;
	delete pC;

	test.Printf(_L("Press a key...\n"));
	test.Getch();

	test.End();
	__UHEAP_MARKEND;
	return KErrNone;
	}
