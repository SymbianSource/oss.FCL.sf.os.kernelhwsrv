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
// e32test\system\t_tick.cpp
// Overview:
// Test tick-based timers
// API Information:
// RTimer
// Details:
// - Create a number of periodic timers, start each and wait for them to 
// complete. Print results.
// - Create a number of random relative timers, start each and wait for 
// them to complete. Print results.
// - Create a number of separate random relative timers, start each and 
// wait for them to complete. Print results.
// - Create a number of random absolute timers, start each and wait for 
// them to complete. Print results.
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

RTest test(_L("T_TICK"));
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
	Start();
	}

void CBackgroundTimer::DoCancel()
	{
	iShort.Cancel();
	iLong.Cancel();
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

TInt ChangeTime(TAny*)
	{
	TTime now;
	now.UniversalTime();
	User::SetUTCTime(now+TTimeIntervalSeconds(1000));
	User::SetUTCTime(now);
	return 1;	// so we run again
	}

GLDEF_C TInt E32Main()
//
// Test tick-based timers
//
    {

	test.Title();

	test.Start(_L("Load test LDD"));
	TInt r=User::LoadLogicalDevice(KLddFileName);
	TEST(r==KErrNone || r==KErrAlreadyExists);
	
	r=ticktest.Open();
	CHECK(r);

	test.Next(_L("Create test objects"));
	TRAP(r, InitialiseL());
	CHECK(r);

	test.Next(_L("Start periodics"));
	TickTest[0]->StartPeriodic(3,1000);
	TickTest[1]->StartPeriodic(5,600);
	TickTest[2]->StartPeriodic(7,400);
	TickTest[3]->StartPeriodic(11,300);
	TickTest[4]->StartPeriodic(13,30);
	TickTest[5]->StartPeriodic(19,30);
	TickTest[6]->StartPeriodic(23,30);
	TickTest[7]->StartPeriodic(37,30);
	ActiveCount=8;
	ErrorCount=0;
	CActiveScheduler::Start();
	PrintInfo();

	test.Next(_L("Start random relative"));
	TickTest[0]->StartNShotRel(1,10,1000);
	TickTest[1]->StartNShotRel(5,25,300);
	TickTest[2]->StartNShotRel(7,93,100);
	TickTest[3]->StartNShotRel(2,2,1000);
	TickTest[4]->StartPeriodic(13,30);
	TickTest[5]->StartPeriodic(19,30);
	TickTest[6]->StartPeriodic(23,30);
	TickTest[7]->StartPeriodic(37,30);
	ActiveCount=8;
	ErrorCount=0;
	CActiveScheduler::Start();
	PrintInfo();

	test.Next(_L("Start separate random relative"));
	TickTest[0]->StartNShotRel(1,10,1000);
	TickTest[1]->StartNShotRel(5,25,300);
	TickTest[2]->StartNShotRel(7,93,100);
	TickTest[3]->StartNShotRel(2,2,1000);
	TickTest[4]->StartNShotRel(1,10,-1000);
	TickTest[5]->StartNShotRel(5,25,-300);
	TickTest[6]->StartNShotRel(7,93,-100);
	TickTest[7]->StartNShotRel(2,2,-1000);
	ActiveCount=8;
	ErrorCount=0;
	CActiveScheduler::Start();
	PrintInfo();

	test.Next(_L("Start random absolute"));
	TickTest[0]->StartNShotAbs(1,10,10);
	TickTest[1]->StartNShotAbs(5,13,3);
	TickTest[2]->StartNShotAbs(1,3,20);
	TickTest[3]->StartNShotAbs(10,1,4);
	TickTest[4]->StartPeriodic(13,30);
	TickTest[5]->StartPeriodic(19,30);
	TickTest[6]->StartPeriodic(23,30);
	TickTest[7]->StartPeriodic(37,30);
	ActiveCount=8;
	ErrorCount=0;
	CActiveScheduler::Start();
	PrintInfo();

	ticktest.Close();
	test.End();
	return(KErrNone);
    }

