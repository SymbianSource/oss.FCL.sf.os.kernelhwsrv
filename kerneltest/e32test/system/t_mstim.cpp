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
// e32test\system\t_mstim.cpp
// Overview:
// Test millisecond timers
// API Information:
// NTimer
// Details:
// - Create and start a number of periodic timers, verify results are
// as expected.
// - Attempt to start a timer that has already been started. Verify returned
// error results.
// - Start one shot interrupt and one shot DFC timers and verify that the 
// delay time is correct.
// - Start additional one shot interrupt timers with various values and verify
// results are as expected.
// - Calculate and print the elapsed time.
// - Start some timers, display min. max, avg and count information on each.
// Verify results are as expected.
// - Cancel a periodic timer and reuse it in a variety of conditions. Time how
// long it takes for each to complete.
// - Perform some random timer tests and display the results.
// - Check idle time while a variety of one shot timers run. Verify results are
// within the expected range.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32uid.h>
#include "d_mstim.h"

RTest test(_L("T_MSTIM"));
RMsTim mstim;

TBool PauseOnError = 0;
#define GETCH()		(PauseOnError&&test.Getch())

#define TEST(c)		((void)((c)||(test.Printf(_L("Failed at line %d\n"),__LINE__),GETCH(),test(0),0)))
#define CHECK(c)	((void)(((c)==0)||(test.Printf(_L("Error %d at line %d\n"),(c),__LINE__),GETCH(),test(0),0)))
#ifdef __WINS__
#define TESTTIME(v,min,max) test.Printf(_L("Expected range [%d,%d]\n"),min,max+1)
#else
#define TESTTIME(v,min,max) TEST(v>=min && v<max)
#endif

const TPtrC KLddFileName=_L("D_MSTIM.LDD");

void GetInfo(TInt aId)
	{
	SMsTimerInfo info;
	TInt r=mstim.GetInfo(aId,info);
	CHECK(r);
	test.Printf(_L("%1d: min=%-6d max=%-6d avg=%-6d count=%d\n"),aId,info.iMin,info.iMax,info.iAvg,info.iCount);
	}

void GetAllInfo()
	{
	GetInfo(0);
	GetInfo(1);
	GetInfo(2);
	GetInfo(3);
	test.Printf(_L("\n"));
	}

TInt GetOneShotTime(TInt aId)
	{
	SMsTimerInfo info;
	TInt r=mstim.GetInfo(aId,info);
	CHECK(r);
	TEST(info.iCount==1);
	return (info.iMin+500)/1000;
	}

GLDEF_C TInt E32Main()
//
// Test millisecond timers
//
    {
//	test.SetLogged(EFalse);
	test.Title();

	test.Start(_L("Load test LDD"));
	TInt r=User::LoadLogicalDevice(KLddFileName);
	TEST(r==KErrNone || r==KErrAlreadyExists);
	
	r=mstim.Open();
	CHECK(r);

	test.Next(_L("Start periodics"));
	TUint init_count=User::NTickCount();
	r=mstim.StartPeriodicInt(0,31);
	CHECK(r);
	r=mstim.StartPeriodicInt(1,32);
	CHECK(r);
	r=mstim.StartPeriodicInt(4,7);
	CHECK(r);
	r=mstim.StartPeriodicInt(5,43);
	CHECK(r);
	r=mstim.StartPeriodicDfc(6,19);
	CHECK(r);
	r=mstim.StartPeriodicDfc(7,71);
	CHECK(r);

	test.Next(_L("Start while started"));
	TRequestStatus s;
	mstim.StartOneShotInt(s,0,100);
	User::WaitForRequest(s);
	TEST(s==KErrInUse);

	test.Next(_L("One shot interrupt"));
	mstim.StartOneShotInt(s,2,100);
	User::WaitForRequest(s);
	TUint fc1=User::NTickCount();
	TEST(s==KErrNone);
	TInt time=GetOneShotTime(2);
	test.Printf(_L("Took %dms\n"),time);
	TESTTIME(time,100,102);

	test.Next(_L("One shot DFC"));
	mstim.StartOneShotDfc(s,3,200);
	User::WaitForRequest(s);
	TUint fc3=User::NTickCount();
	TEST(s==KErrNone);
	time=GetOneShotTime(3);
	test.Printf(_L("Took %dms\n"),time);
	TESTTIME(time,200,202);

	test.Next(_L("One shot interrupt again"));
	TUint fc2=User::NTickCount();
	mstim.StartOneShotIntAgain(s,2,300);
	User::WaitForRequest(s);
	TEST(s==KErrNone);
	TInt time2=GetOneShotTime(2);
	test.Printf(_L("Took %dms, delay %dms\n"),time2,fc2-fc1);
	time2+=TInt(fc2-fc1);
	TESTTIME(time2,295,306);

	test.Next(_L("One shot interrupt again too late"));
	mstim.StartOneShotIntAgain(s,3,10);
	User::WaitForRequest(s);
	TEST(s==KErrArgument);

	test.Next(_L("One shot interrupt again"));
	fc2=User::NTickCount();
	mstim.StartOneShotIntAgain(s,3,300);
	User::WaitForRequest(s);
	TEST(s==KErrNone);
	time=GetOneShotTime(3);
	test.Printf(_L("Took %dms, delay %dms\n"),time,fc2-fc3);
	time+=TInt(fc2-fc3);
	TESTTIME(time,295,306);

	test.Printf(_L("Please wait...\n"));
	User::After(10000000);

	SMsTimerInfo info[8];
	TInt i;
	for (i=0; i<8; i++)
		{
		r=mstim.GetInfo(i,info[i]);
		CHECK(r);
		}

	TUint final_count=User::NTickCount();
	TInt elapsed=TInt(final_count-init_count);
	test.Printf(_L("Elapsed time %dms\n"),elapsed);

	const TInt period[8]={31,32,0,0,7,43,19,71};
	for (i=0; i<8; i++)
		{
		TInt p=period[i];
		if (p==0)
			continue;
		SMsTimerInfo& z=info[i];
		test.Printf(_L("%1d: min=%-6d max=%-6d avg=%-6d count=%d\n"),i,z.iMin,z.iMax,z.iAvg,z.iCount);
		TInt count=elapsed/p;
		TInt cdiff=count-z.iCount;
		TEST(cdiff>=0 && cdiff<=2);
#ifndef __WINS__
		TEST(Abs(z.iMin-1000*p)<1000);
		TEST(Abs(z.iMax-1000*p)<1000);
#endif
		TEST(Abs(z.iAvg-1000*p)<1000);
		}

	test.Next(_L("Cancel periodic"));
	r=mstim.StopPeriodic(7);
	CHECK(r);
	r=mstim.GetInfo(7,info[7]);
	CHECK(r);
	User::After(1000000);
	r=mstim.GetInfo(7,info[6]);
	CHECK(r);
	TEST(info[6].iCount==info[7].iCount);

	test.Next(_L("Reuse cancelled"));
	mstim.StartOneShotInt(s,7,128);
	User::WaitForRequest(s);
	TEST(s==KErrNone);
	time=GetOneShotTime(7);
	test.Printf(_L("Took %dms\n"),time);
	TESTTIME(time,128,130);

	TRequestStatus s2;
	test.Next(_L("Timed Cancel"));
	mstim.StartOneShotInt(s,2,128);
	mstim.IntCancel(s2,2,130);
	User::WaitForRequest(s);
	TEST(s==KErrNone);
	User::WaitForRequest(s2);
	TEST(s2==KErrNone);
	time=GetOneShotTime(2);
	test.Printf(_L("Took %dms\n"),time);
	TESTTIME(time,128,130);
	time=GetOneShotTime(7);
	test.Printf(_L("Cancel Took %dms\n"),time);
	TESTTIME(time,130,132);

	mstim.StartOneShotInt(s,2,128);
	mstim.IntCancel(s2,2,126);
	User::WaitForRequest(s);
	TEST(s==KErrAbort);
	User::WaitForRequest(s2);
	TEST(s2==KErrNone);
	time=GetOneShotTime(7);
	test.Printf(_L("Cancel Took %dms\n"),time);
	TESTTIME(time,126,128);

	test.Next(_L("Reuse cancelled"));
	mstim.StartOneShotInt(s,2,64);
	User::WaitForRequest(s);
	TEST(s==KErrNone);
	time=GetOneShotTime(2);
	test.Printf(_L("Took %dms\n"),time);
	TESTTIME(time,64,66);

#ifdef _DEBUG
	test.Next(_L("Random test"));
	r=mstim.BeginRandomTest();
	CHECK(r);
#endif

	test.Printf(_L("Please wait...\n"));
	User::After(10000000);

#ifdef _DEBUG
	r=mstim.EndRandomTest();
	CHECK(r);
	SRandomTestInfo rInfo;
	r=mstim.GetRandomTestInfo(rInfo);
	test.Printf(_L("min error = %d\n"),rInfo.iMin);
	test.Printf(_L("max error = %d\n"),rInfo.iMax);
	test.Printf(_L("xfer cancel = %d\n"),rInfo.iXferC);
	test.Printf(_L("crit cancel = %d\n"),rInfo.iCritC);
	test.Printf(_L("start fails = %d\n"),rInfo.iStartFail);
	test.Printf(_L("debug calls = %d\n"),rInfo.iCallBacks);
	test.Printf(_L("completions = %d\n"),rInfo.iCompletions);
#endif

	for (i=0; i<8; i++)
		{
		r=mstim.GetInfo(i,info[i]);
		CHECK(r);
		}

	final_count=User::NTickCount();
	elapsed=TInt(final_count-init_count);
	test.Printf(_L("Elapsed time %dms\n"),elapsed);

	const TInt period2[8]={31,32,0,0,7,43,19,0};
	for (i=0; i<8; i++)
		{
		TInt p=period2[i];
		if (p==0)
			continue;
		r=mstim.StopPeriodic(i);
		CHECK(r);
		SMsTimerInfo& z=info[i];
		test.Printf(_L("%1d: min=%-6d max=%-6d avg=%-6d count=%d\n"),i,z.iMin,z.iMax,z.iAvg,z.iCount);
		TInt count=elapsed/p;
		TInt cdiff=count-z.iCount;
		TEST(cdiff>=0 && cdiff<=2);
#ifndef __WINS__
		TEST(Abs(z.iMin-1000*p)<=1000);
		TEST(Abs(z.iMax-1000*p)<=1000);
#endif
		TEST(Abs(z.iAvg-1000*p)<=1000);
		}

	test.Next(_L("Idle time"));
	time=0;
	TInt idle=0;
	while (time<3000)
		{
		idle=mstim.GetIdleTime();
		if (idle>=1000)
			break;
		if (idle<32)
			idle=32;
		User::AfterHighRes(idle*1000);
		time+=idle;
		}
	if (time>=3000)
		test.Printf(_L("Never got long enough idle time\n"));
	else
		{
		mstim.StartOneShotInt(s,0,900);
		TUint fc0=User::NTickCount();
		User::AfterHighRes(20000);
		idle=mstim.GetIdleTime();
		test.Printf(_L("Idle time %dms\n"),idle);
		TESTTIME(idle,860,881);
		mstim.StartOneShotInt(s2,1,200);
		fc1=User::NTickCount();
		User::AfterHighRes(20000);
		idle=mstim.GetIdleTime();
		test.Printf(_L("Idle time %dms\n"),idle);
		TESTTIME(idle,160,181);
		TRequestStatus s3;
		mstim.StartOneShotInt(s3,2,10);
		idle=mstim.GetIdleTime();
		test.Printf(_L("Idle time %dms\n"),idle);
		TEST(idle==0);
		User::WaitForRequest(s3);
		fc2=User::NTickCount();
		idle=mstim.GetIdleTime();
		elapsed=fc2-fc1;
		test.Printf(_L("Idle time %dms elapsed %dms\n"),idle,elapsed);
		TESTTIME(idle,180-elapsed,201-elapsed);
		User::WaitForRequest(s2);
		fc2=User::NTickCount();
		idle=mstim.GetIdleTime();
		elapsed=fc2-fc0;
		test.Printf(_L("Idle time %dms elapsed %dms\n"),idle,elapsed);
		TESTTIME(idle,880-elapsed,900-elapsed);
		User::WaitForRequest(s);
		}

	TUint fc4, fc5;
	test.Next(_L("One shot int "));
	mstim.StartOneShotInt(s,8,100);
	User::WaitForRequest(s);
	TEST(s==KErrNone);
	time=GetOneShotTime(8);
	test.Printf(_L("Took %dms\n"),time);
	TESTTIME(time,100,102);

	test.Next(_L("One shot int "));
	mstim.StartOneShotInt(s,8,300);
	User::WaitForRequest(s);
	fc4=User::NTickCount();
	TEST(s==KErrNone);
	time=GetOneShotTime(8);
	test.Printf(_L("Took %dms\n"),time);
	TESTTIME(time,300,302);

	test.Next(_L("One shot int again"));
	fc5=User::NTickCount();
	mstim.StartOneShotIntAgain(s,8,300);
	User::WaitForRequest(s);
	TEST(s==KErrNone);
	time2=GetOneShotTime(8);
	test.Printf(_L("Took %dms, delay %dms\n"),time2,fc5-fc4);
	time2+=TInt(fc5-fc4);
	TESTTIME(time2,295,306);

	test.Next(_L("One shot with provided Dfc queue"));
	mstim.StartOneShotUserDfc(s,8,100);
	User::WaitForRequest(s);
	TEST(s==KErrNone);
	time=GetOneShotTime(8);
	test.Printf(_L("Took %dms\n"),time);
	TESTTIME(time,100,102);

	test.Next(_L("One shot with provided Dfc queue"));
	mstim.StartOneShotUserDfc(s,8,300);
	User::WaitForRequest(s);
	fc4=User::NTickCount();
	TEST(s==KErrNone);
	time=GetOneShotTime(8);
	test.Printf(_L("Took %dms\n"),time);
	TESTTIME(time,300,302);

	test.Next(_L("One shot with provided Dfc queue again"));
	fc5=User::NTickCount();
	mstim.StartOneShotUserDfcAgain(s,8,300);
	User::WaitForRequest(s);
	TEST(s==KErrNone);
	time2=GetOneShotTime(8);
	test.Printf(_L("Took %dms, delay %dms\n"),time2,fc5-fc4);
	time2+=TInt(fc5-fc4);
	TESTTIME(time2,295,306);

	test.End();
	return(KErrNone);
    }

