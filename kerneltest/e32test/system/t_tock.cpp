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
// e32test\system\t_tock.cpp
// Overview:
// Test tick-based timers
// API Information:
// RTimer
// Details:
// - Create a number of periodic timers, start each and change the time
// offset via TLocale::SetUniversalTimeOffset(). Print the results.
// - Create a number of periodic timers, start each and change the system
// time. Print the results.
// - Create a number of periodic timers, start each and change the system
// time in a second thread. Print the results.
// - Create a number of periodic timers, start each along with a background
// timer. Print the results.
// - Create a number of periodic timers, start each and change the system
// time and tick delay. Print the results.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32uid.h>
#include "d_tick.h"

RTest test(_L("T_TOCK"));
RTickTest ticktest;
TInt ActiveCount;
TInt ErrorCount;

TBool PauseOnError = 0;
#define GETCH()		(PauseOnError&&test.Getch())

#define TEST(c)		((void)((c)||(test.Printf(_L("Failed at line %d\n"),__LINE__),GETCH(),test(0),0)))
#define CHECK(c)	((void)(((c)==0)||(test.Printf(_L("Error %d at line %d\n"),(c),__LINE__),GETCH(),test(0),0)))

const TPtrC KLddFileName=_L("D_TICK.LDD");

class CTickTest : public CActive
	{
public:
	CTickTest(TInt aPriority, TInt aId, RTickTest aLdd);
	~CTickTest();
	virtual void RunL();
	virtual void DoCancel();
public:
	void StartPeriodic(TInt aInterval, TInt aCount);
	void StartNShotRel(TInt aMin, TInt aRange, TInt aCount);
	void StartNShotAbs(TInt aMin, TInt aRange, TInt aCount);
	void StartNShotDelay(TInt aPeriod, TInt aDelay, TInt aCount);
	void GetInfo(STickTestInfo& aInfo);
public:
	TInt iId;
	TInt iCount;
	TInt iMin;
	TInt iRange;
	TInt64 iErrorAcc;
	RTickTest iLdd;
	STickTestInfo iInfo;
	};

class CBackgroundTimer : public CActive
	{
public:
	static CBackgroundTimer* NewL(TInt aPriority);
	CBackgroundTimer(TInt aPriority);
	~CBackgroundTimer();
	virtual void RunL();
	virtual void DoCancel();
	void Start();
public:
	RTimer iShort;
	RTimer iLong;
	TRequestStatus iLongStatus;
	};

CTickTest* TickTest[KMaxTimers];
CIdle* Idler;
CBackgroundTimer* BackgroundTimer;

CTickTest::CTickTest(TInt aPriority, TInt aId, RTickTest aLdd)
	:	CActive(aPriority),
		iId(aId),
		iLdd(aLdd)
	{
	}

CTickTest::~CTickTest()
	{
	Cancel();
	iLdd.SetHandle(0);
	}

void CTickTest::StartPeriodic(TInt aInterval, TInt aCount)
	{
	TInt r=iLdd.StartPeriodic(iStatus,iId,aInterval,aCount);
	if (r!=KErrNone)
		{
		TRequestStatus* pS=&iStatus;
		User::RequestComplete(pS,r);
		}
	SetActive();
	}

void CTickTest::StartNShotRel(TInt aMin, TInt aRange, TInt aCount)
	{
	TInt c=aCount;
	if (aCount<0)
		{
		iCount=-aCount;
		c=1;
		}
	iMin=aMin;
	iRange=aRange;
	iErrorAcc=0;
	iInfo.iMinErr=KMaxTInt;
	iInfo.iMaxErr=KMinTInt;
	iInfo.iCount=0;
	iInfo.iRequestedCount=iCount;
	iInfo.iTotalTime=0;
	TInt r=iLdd.StartNShotRel(iStatus,iId,aMin,aRange,c);
	if (r!=KErrNone)
		{
		TRequestStatus* pS=&iStatus;
		User::RequestComplete(pS,r);
		}
	SetActive();
	}

void CTickTest::StartNShotAbs(TInt aMin, TInt aRange, TInt aCount)
	{
	TInt r=iLdd.StartNShotAbs(iStatus,iId,aMin,aRange,aCount);
	if (r!=KErrNone)
		{
		TRequestStatus* pS=&iStatus;
		User::RequestComplete(pS,r);
		}
	SetActive();
	}

void CTickTest::StartNShotDelay(TInt aPeriod, TInt aDelay, TInt aCount)
	{
	iCount=0;
	TInt r=iLdd.StartNShotDelay(iStatus,iId,aPeriod,aDelay,aCount);
	if (r!=KErrNone)
		{
		TRequestStatus* pS=&iStatus;
		User::RequestComplete(pS,r);
		}
	SetActive();
	}

void CTickTest::GetInfo(STickTestInfo& aInfo)
	{
	iLdd.GetInfo(iId,aInfo);
	}

void CTickTest::RunL()
	{
	if (iStatus!=KErrNone)
		{
		test.Printf(_L("Timer %d error %d\n"),iId,iStatus.Int());
		++ErrorCount;
		CActiveScheduler::Stop();
		return;
		}
	if (iCount==0)
		GetInfo(iInfo);
	else
		{
		STickTestInfo info;
		iLdd.GetInfo(iId,info);
		TInt err=info.iMinErr;
		if (err<iInfo.iMinErr)
			iInfo.iMinErr=err;
		if (err>iInfo.iMaxErr)
			iInfo.iMaxErr=err;
		++iInfo.iCount;
		iErrorAcc+=err;
		if (--iCount)
			{
			TInt r=iLdd.StartNShotRel(iStatus,iId,iMin,iRange,1);
			if (r!=KErrNone)
				{
				TRequestStatus* pS=&iStatus;
				User::RequestComplete(pS,r);
				}
			SetActive();
			return;
			}
		iInfo.iAvgErr=I64INT(iErrorAcc/TInt64(iInfo.iCount));
		}
	test.Printf(_L("Timer %d\n"),iId);
	if (!--ActiveCount)
		CActiveScheduler::Stop();
	}

void CTickTest::DoCancel()
	{
	iLdd.Stop(iId);
	}

CBackgroundTimer::CBackgroundTimer(TInt aPriority)
	:	CActive(aPriority)
	{
	}

CBackgroundTimer::~CBackgroundTimer()
	{
	iShort.Close();
	iLong.Close();
	}

void CBackgroundTimer::Start()
	{
	iLong.After(iLongStatus, 100000);
	iShort.After(iStatus, 20000);
	SetActive();
	}

void CBackgroundTimer::RunL()
	{
	iLong.Cancel();
	User::WaitForRequest(iLongStatus);
	Start();
	}

void CBackgroundTimer::DoCancel()
	{
	iShort.Cancel();
	iLong.Cancel();
	User::WaitForRequest(iLongStatus);
	}

CBackgroundTimer* CBackgroundTimer::NewL(TInt aPriority)
	{
	CBackgroundTimer* pB=new (ELeave) CBackgroundTimer(aPriority);
	TInt r=pB->iShort.CreateLocal();
	if (r==KErrNone)
		r=pB->iLong.CreateLocal();
	if (r!=KErrNone)
		{
		delete pB;
		pB=NULL;
		}
	return pB;
	}

void InitialiseL()
	{
	CActiveScheduler* pS=new (ELeave) CActiveScheduler;
	CActiveScheduler::Install(pS);
	TInt i;
	for (i=0; i<KMaxTimers; ++i)
		{
		TickTest[i]=new (ELeave) CTickTest(0,i,ticktest);
		CActiveScheduler::Add(TickTest[i]);
		}
	Idler=CIdle::NewL(-100);
	BackgroundTimer=CBackgroundTimer::NewL(-10);
	CActiveScheduler::Add(BackgroundTimer);
	}

void PrintInfo(TInt aId, STickTestInfo& a)
	{
	test.Printf(_L("%1d: min=%-6d max=%-6d avg=%-6d tot=%-10u count=%d rcount=%d\n"),aId,a.iMinErr,a.iMaxErr,a.iAvgErr,a.iTotalTime,a.iCount,a.iRequestedCount);
	}

void PrintInfo(TInt aId)
	{
	PrintInfo(aId, TickTest[aId]->iInfo);
	}

void PrintInfo()
	{
	TInt i;
	for (i=0; i<KMaxTimers; ++i)
		PrintInfo(i);
	}

TInt ChangeOffset(TAny*)
	{
	User::SetUTCOffset(3600);
	User::SetUTCOffset(0);
	return 1;	// so we run again
	}

TInt ChangeTime(TAny*)
	{
	TTime now;
	now.UniversalTime();
	User::SetUTCTime(now+TTimeIntervalSeconds(1000));
	User::SetUTCTime(now);
	return 1;	// so we run again
	}

TInt ChangeTimeThread(TAny*)
	{
	FOREVER
		{
		User::AfterHighRes(3000);
		TTime now;
		now.UniversalTime();
		User::SetUTCTime(now+TTimeIntervalSeconds(1000));
		User::AfterHighRes(1000);
		User::SetUTCTime(now);
		}
	}

GLDEF_C TInt E32Main()
//
// Test tick-based timers
//
    {

//	test.SetLogged(EFalse);
	test.Title();

	test.Start(_L("Load test LDD"));
	TInt r=User::LoadLogicalDevice(KLddFileName);
	TEST(r==KErrNone || r==KErrAlreadyExists);
	
	r=ticktest.Open();
	CHECK(r);

	test.Next(_L("Create test objects"));
	TRAP(r, InitialiseL());
	CHECK(r);

	test.Next(_L("Periodics with changing home time offset"));
	TickTest[0]->StartPeriodic(3,1000);
	TickTest[1]->StartPeriodic(5,600);
	TickTest[2]->StartPeriodic(7,400);
	TickTest[3]->StartPeriodic(11,300);
	TickTest[4]->StartPeriodic(13,30);
	TickTest[5]->StartPeriodic(19,30);
	TickTest[6]->StartPeriodic(23,30);
	TickTest[7]->StartPeriodic(37,30);
	Idler->Start(TCallBack(ChangeOffset,NULL));
	ActiveCount=8;
	ErrorCount=0;
	CActiveScheduler::Start();
	Idler->Cancel();
	PrintInfo();

	test.Next(_L("Periodics with changing system time"));
	TickTest[0]->StartPeriodic(3,1000);
	TickTest[1]->StartPeriodic(5,600);
	TickTest[2]->StartPeriodic(7,400);
	TickTest[3]->StartPeriodic(11,300);
	TickTest[4]->StartPeriodic(13,30);
	TickTest[5]->StartPeriodic(19,30);
	TickTest[6]->StartPeriodic(23,30);
	TickTest[7]->StartPeriodic(37,30);
	Idler->Start(TCallBack(ChangeTime,NULL));
	ActiveCount=8;
	ErrorCount=0;
	CActiveScheduler::Start();
	Idler->Cancel();
	PrintInfo();

	test.Next(_L("Periodics with changing system time in second thread"));
	RThread t;
	r=t.Create(KNullDesC(),ChangeTimeThread,0x1000,NULL,NULL);
	CHECK(r);
	t.SetPriority(EPriorityMore);
	TickTest[0]->StartPeriodic(3,1000);
	TickTest[1]->StartPeriodic(5,600);
	TickTest[2]->StartPeriodic(7,400);
	TickTest[3]->StartPeriodic(11,300);
	TickTest[4]->StartPeriodic(13,30);
	TickTest[5]->StartPeriodic(19,30);
	TickTest[6]->StartPeriodic(23,30);
	TickTest[7]->StartPeriodic(37,30);
	Idler->Start(TCallBack(ChangeTime,NULL));
	ActiveCount=8;
	ErrorCount=0;
	t.Resume();
	CActiveScheduler::Start();
	Idler->Cancel();
	TRequestStatus s;
	t.Logon(s);
	t.Kill(0);
	User::WaitForRequest(s);
	test(t.ExitType()==EExitKill);
	test(t.ExitReason()==0);
	test(s==0);
	CLOSE_AND_WAIT(t);
	PrintInfo();

	test.Next(_L("Periodics with background timer"));
	TickTest[0]->StartPeriodic(3,1000);
	TickTest[1]->StartPeriodic(5,600);
	TickTest[2]->StartPeriodic(7,400);
	TickTest[3]->StartPeriodic(11,300);
	TickTest[4]->StartPeriodic(13,30);
	TickTest[5]->StartPeriodic(19,30);
	TickTest[6]->StartPeriodic(23,30);
	TickTest[7]->StartPeriodic(37,30);
	BackgroundTimer->Start();
	ActiveCount=8;
	ErrorCount=0;
	CActiveScheduler::Start();
	BackgroundTimer->Cancel();
	PrintInfo();

	test.Next(_L("Periodics with changing system time and tick delay"));
	TickTest[0]->StartPeriodic(3,1000);
	TickTest[1]->StartPeriodic(5,600);
	TickTest[2]->StartPeriodic(7,400);
	TickTest[3]->StartPeriodic(11,300);
	TickTest[4]->StartPeriodic(13,30);
	TickTest[5]->StartPeriodic(19,30);
	TickTest[6]->StartPeriodic(23,30);
	TickTest[7]->StartNShotDelay(17,30,200);
	Idler->Start(TCallBack(ChangeTime,NULL));
	ActiveCount=8;
	ErrorCount=0;
	CActiveScheduler::Start();
	Idler->Cancel();
	PrintInfo();

	ticktest.Close();
	test.End();
	return(KErrNone);
    }

