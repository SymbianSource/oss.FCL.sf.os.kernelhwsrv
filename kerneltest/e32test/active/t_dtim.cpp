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
// e32test\active\t_dtim.cpp
// Overview:
// Test delta timers. 
// API Information:
// CDeltaTimer.
// Details:
// - Create a delta timer and queue a number of timed events with different time intervals.
// The callback functions of the timed events perform various actions: 
// - Requeuing itself
// - Cancelling the previous or next timer
// - Doing nothing
// - Stopping the active scheduler
// - Verifies the timed events are run correctly and orderly
// - The callback for each timed event increments a counter each time they are run
// - The counters are checked to check how many times each callback has been called
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32hal.h>
#include <e32def.h>
#include <e32def_private.h>

RTest test(_L("T_DTIM"));


TInt theResults[10];
TCallBack* theCallBacks;
TDeltaTimerEntry TheTimers[10];
CDeltaTimer* theTimer;

class CMyActiveScheduler : public CActiveScheduler
	{
public:
	void Error(TInt anError)const;
	static CMyActiveScheduler* NewL();
	};

void CMyActiveScheduler::Error(TInt /*anError*/) const
//
//
//
	{
	User::Panic(_L("MYActiveScheduler"),0);
	}

CMyActiveScheduler* CMyActiveScheduler::NewL()
	{
	return new(ELeave) CMyActiveScheduler;
	}

struct TDeltaTimerCallData
	{
	CDeltaTimer* iTimer;
	TDeltaTimerEntry *iTimers;
	TInt iIndex;
	};

TInt print(TAny* anArg)
//
// Null entry. Just prints and increments it's counter.
//
	{
	TInt index=(TInt)anArg;
	theResults[index]++;
	test.Printf(_L("Callback %d\r\n"),index);
	return 0;
	}

TInt cancelNext(TAny * anArg)
//
// Cancels the next entry - assumes it is valid.
//
	{
	TInt index=(TInt)anArg;
	theTimer->Remove(TheTimers[index+1]);
	return print(anArg);
	}

TInt requeue(TAny * anArg)
//
// Note that a requeue will run once every 200 ms until stop is called.
//
	{
	TTimeIntervalMicroSeconds32 tickPeriod;
	UserHal::TickPeriod(tickPeriod);
	const TInt K13Ticks = 13 * tickPeriod.Int();

	TInt index=(TInt)anArg;
	theTimer->Queue(K13Ticks*2/3,TheTimers[index]);
	return print(anArg);
	}

TInt requeuePrevious(TAny * anArg)
//
// Requeue the previous entry (assumes previous entry is valid handle)
//
	{
	TTimeIntervalMicroSeconds32 tickPeriod;
	UserHal::TickPeriod(tickPeriod);
	const TInt K13Ticks = 13 * tickPeriod.Int();

	TInt index=(TInt)anArg - 1;
	theTimer->Queue(K13Ticks,TheTimers[index]);
	return print(anArg);
	}

TInt cancel2ndFollowing(TAny * anArg)
//
// Assumes that 2nd following timer handle is valid.
//
	{
	TInt index=(TInt)anArg;
	theTimer->Remove(TheTimers[index+2]);
	return print(anArg);
	}

TInt stop(TAny * anArg)
//
// Stops the active schduler
//
	{
	TInt index=(TInt)anArg;
	theResults[index]++;
	test.Printf(_L("Callback %d, stopping\r\n"),index);
	CMyActiveScheduler::Stop();

	return 0;
	}

TInt E32Main()
//
//
//
	{

	CMyActiveScheduler* s=NULL;
	TRAPD(ret,s=CMyActiveScheduler::NewL())
	test(ret==KErrNone);

	CActiveScheduler::Install(s);
	test.Title();
	test.Start(_L("Timer"));
	
	__KHEAP_MARK;

	TTimeIntervalMicroSeconds32 tickPeriod;
	UserHal::TickPeriod(tickPeriod);

	TRAP(ret,theTimer=CDeltaTimer::NewL(100, tickPeriod.Int()));
	test(ret==KErrNone);

	Mem::FillZ(theResults,10*sizeof(TInt));

	TCallBack callBacks[10]=
		{
		/* 0 */ TCallBack(print,(TAny*)0),
		/* 1 */ TCallBack(cancelNext,(TAny*)1),
		/* 2 */ TCallBack(print,(TAny*)2),	// Gets cancelled
		/* 3 */ TCallBack(print,(TAny*)3),		// Runs twice
		/* 4 */ TCallBack(requeuePrevious,(TAny*)4),
		/* 5 */ TCallBack(cancel2ndFollowing,(TAny*)5),
		/* 6 */ TCallBack(print,(TAny*)6),
		/* 7 */ TCallBack(cancelNext,(TAny*)7),	// Gets cancelled
		/* 8 */ TCallBack(requeue,(TAny*)8),	// Runs twice, once on the same RunL as the stop
		/* 9 */ TCallBack(stop,(TAny*)9),
		};

	theCallBacks=callBacks;
	for (TInt i=0;i<10;i++)
		TheTimers[i].Set(theCallBacks[i]);

	const TInt K13Ticks = 13 * tickPeriod.Int();

	theTimer->Queue(K13Ticks,TheTimers[0]);
	theTimer->Queue(2*K13Ticks,TheTimers[1]);
	theTimer->Queue(3*K13Ticks,TheTimers[2]);
	theTimer->Queue(4*K13Ticks,TheTimers[3]);
	theTimer->Queue(5*K13Ticks,TheTimers[4]);
	theTimer->Queue(6*K13Ticks,TheTimers[5]);
	theTimer->Queue(7*K13Ticks,TheTimers[6]);
	theTimer->Queue(8*K13Ticks,TheTimers[7]);
	theTimer->Queue(9*K13Ticks,TheTimers[8]);
	theTimer->Queue(10*K13Ticks,TheTimers[9]);

	CActiveScheduler::Start();

	test(theResults[0]==1);
	test(theResults[1]==1);
	test(theResults[2]==0);
	test(theResults[3]==2);
	test(theResults[4]==1);
	test(theResults[5]==1);
	test(theResults[6]==1);
	test(theResults[7]==0);
	test(theResults[8]==2);
	test(theResults[9]==1);

	delete theTimer;

	__KHEAP_MARKEND;

	test.End();
	return 0;
	}
