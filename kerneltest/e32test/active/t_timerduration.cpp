// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Overview:
// The test measures the duration of the user-side timer services.
// API Information:
// User::After(...)
// User::At(...)
// User::AfterHighRes(...)
// Details:
// - Calls time services a number of times with the same input arguments.
// - Records and prints the minimum and maximum duration of each test case.
// - Tests the duration of User::After and User::AfterHighRes on target.
// Platforms/Drives/Compatibility:
// Emulator and Hardware (Automatic). 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// The test can fail only on target.
// - The duration of Timer::After(aTime) is not within the limits (from <aTime> to <aTime + 1000000/64+2*NanoKarnelTickPeriod>)
// - The duration of Timer::AfterHighRes(aTime) is not within the limits (from <aTime> to <aTime+2*NanoKarnelTickPeriod>)
// Base Port information:
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32test.h>
#include <e32math.h>
#include <e32svr.h>
#include <hal.h>

LOCAL_D RTest test(_L("T_TIMERDURATION"));

// Max number of different time values measured
const TInt KMaxTimeValues = 12;

// number of times measurement taken to average
const TInt KMaxTimeMeasurements = 20;
TInt MaxTimeMeasurements;
TInt TimeRawMS[KMaxTimeMeasurements];//Holds the ROW time in Kernel ticks

TInt* TimeValue;
TInt TimeMin[KMaxTimeValues];
TInt TimeMax[KMaxTimeValues];

RTimer TheTimer;

void After(TInt aTime)
	{
	TRequestStatus s;
	TheTimer.HighRes(s, aTime);
	User::WaitForRequest(s);
	}

TInt Again(TInt aTime)
	{
	TRequestStatus s;
	TheTimer.AgainHighRes(s, aTime);
	User::WaitForRequest(s);
	return s.Int();
	}

void WaitInSteps(TUint aWait, TUint aPeriod, TUint aSteps)
	{
	TUint total_ticks = aWait / aPeriod;
	TUint remain = total_ticks;
	TUint steps = aSteps > remain ? remain : aSteps;
	TUint carry = 0;
	TUint step = 0;
	while (remain)
		{
		TUint stepLength = remain / steps;
		carry += stepLength;
		TUint us = carry * aPeriod;
		TInt r = KErrNone;
		if (step==0)
			After(us);
		else
			r = Again(us);
		if (r==KErrNone)
			carry = 0;
		++step;
		--steps;
		remain -= stepLength;
		}
	}
	
void calcStats(TInt i)
	{
	TimeMin[i]=TimeRawMS[0];
	TimeMax[i]=TimeRawMS[0];
	for (TInt j=1; j<MaxTimeMeasurements; ++j)
			{
			if (TimeMin[i]>TimeRawMS[j]) TimeMin[i]=TimeRawMS[j];
			if (TimeMax[i]<TimeRawMS[j]) TimeMax[i]=TimeRawMS[j];
			}
	}

void printStats()
	{
	test.Printf(_L("  Value     Min      Max\n"));
	for (TInt i=0;i<KMaxTimeValues;++i)
		{
		if (TimeValue[i]<0) break;
		test.Printf(_L("%8d %8d %8d\n"),TimeValue[i],TimeMin[i],TimeMax[i]);
		}
	}

#define __BEFORE_WAIT__ \
	test.Printf(_L("Measuring value(%d measurements at each value):\n"), MaxTimeMeasurements);\
	for (i=0;i<KMaxTimeValues;++i)\
		{\
		if (TimeValue[i]<0) break;\
		test.Printf(_L("%8d microseconds ...\n"),TimeValue[i]);\
		value = TimeValue[i];\
		for (j=0; j<MaxTimeMeasurements; ++j)\
			{\
			User::AfterHighRes((Math::Random()&0xf)*1000);\
	
#define __MEASURE1__ tick1 = User::NTickCount();

#define __MEASURE2__ tick2 = User::NTickCount();

#define __AFTER_WAIT__ \
			TimeRawMS[j]=(tick2-tick1)*tickPeriod;\
			}\
		calcStats(i);\
		}\
	printStats();\

GLDEF_C TInt E32Main()
    {
    TInt i,j;
	test.Title();
	test.Start(_L("Timer resolution test"));
	test.SetLogged(ETrue);
	test(TheTimer.CreateLocal()==KErrNone);
	RThread This;
	This.SetPriority(EPriorityRealTime);
	TUint tick1,tick2;
	TInt value, tickPeriod;
	HAL::Get(HAL::ENanoTickPeriod, tickPeriod);
	test.Printf(_L("tickPeriod=%d\n"),tickPeriod);
///////////////////////////////////////////
	test.Next(_L("Calibrate"));
	MaxTimeMeasurements = KMaxTimeMeasurements;
	TInt TimeValues1[KMaxTimeValues]={0,-1};
	TimeValue = &TimeValues1[0];
	__BEFORE_WAIT__
	__MEASURE1__
	__MEASURE2__
	__AFTER_WAIT__
///////////////////////////////////////////
	test.Next(_L("User::After"));
	TInt TimeValues2[KMaxTimeValues]={10000, 40000,80000,160000,320000,-1};
	TimeValue = &TimeValues2[0];
	__BEFORE_WAIT__
	__MEASURE1__
			User::After(value);
	__MEASURE2__
	__AFTER_WAIT__
#if defined(__EPOC32__)
	//Check that User::After calls completed within boundaries
    TInt k;
	for (k = 0; k<KMaxTimeValues; k++)
		{
		if (TimeValue[k] == -1) break;
		test(TimeValue[k] <= TimeMin[k]);
		TInt aTimerResolution = 1000000/64;
		test((TimeValue[k] + aTimerResolution +2*tickPeriod) >= TimeMax[k]);
		}
#endif
///////////////////////////////////////////
	test.Next(_L("User::At"));
	MaxTimeMeasurements = KMaxTimeMeasurements/2;
	TInt TimeValues3[KMaxTimeValues]={950000,1000000,-1};
	TimeValue = &TimeValues3[0];
	TTime time;
	time.Set(_L("20050101:000001.000000"));
	User::SetHomeTime(time);
	__BEFORE_WAIT__
			time.HomeTime();
			time += (TTimeIntervalMicroSeconds32)value;
	__MEASURE1__
			User::At(time);
	__MEASURE2__
	__AFTER_WAIT__
///////////////////////////////////////////
	test.Next(_L("User::AfterHighRes"));
	MaxTimeMeasurements = KMaxTimeMeasurements;
	TInt TimeValues4[KMaxTimeValues]={1000,2000,4000,8000,16000,32000,64000,128000,-1};
	TimeValue = &TimeValues4[0];
	__BEFORE_WAIT__
	__MEASURE1__
			User::AfterHighRes(value);
	__MEASURE2__
	__AFTER_WAIT__
#if defined(__EPOC32__)
	//Check that User::AfterHighRes calls completed within boundaries
	for (k = 0; k<KMaxTimeValues; k++)
		{
		if (TimeValue[k] == -1) break;
		test(TimeValue[k] <= TimeMin[k]);
		test((TimeValue[k] + 2*tickPeriod) >= TimeMax[k]);
		}
#endif
///////////////////////////////////////////
	test.Next(_L("RTimer::AgainHighRes (2 steps)"));
	MaxTimeMeasurements = KMaxTimeMeasurements;
	TInt TimeValues5[KMaxTimeValues]={2000,4000,8000,16000,32000,64000,128000,-1};
	TimeValue = &TimeValues5[0];
	__BEFORE_WAIT__
	__MEASURE1__
			WaitInSteps(value, tickPeriod, 2);
	__MEASURE2__
	__AFTER_WAIT__
#if defined(__EPOC32__)
	//Check that RTimer::AgainHighRes() calls completed within boundaries
	for (k = 0; k<KMaxTimeValues; k++)
		{
		if (TimeValue[k] == -1) break;
		test(TimeValue[k] <= TimeMin[k]);
		test((TimeValue[k] + 2*tickPeriod) >= TimeMax[k]);
		}
#endif
///////////////////////////////////////////
	test.Next(_L("RTimer::AgainHighRes (5 steps)"));
	MaxTimeMeasurements = KMaxTimeMeasurements;
	TInt TimeValues6[KMaxTimeValues]={4000,8000,16000,32000,64000,128000,-1};
	TimeValue = &TimeValues6[0];
	__BEFORE_WAIT__
	__MEASURE1__
			WaitInSteps(value, tickPeriod, 5);
	__MEASURE2__
	__AFTER_WAIT__
#if defined(__EPOC32__)
	//Check that RTimer::AgainHighRes() calls completed within boundaries
	for (k = 0; k<KMaxTimeValues; k++)
		{
		if (TimeValue[k] == -1) break;
		test(TimeValue[k] <= TimeMin[k]);
		test((TimeValue[k] + 2*tickPeriod) >= TimeMax[k]);
		}
#endif
///////////////////////////////////////////
	TheTimer.Close();
	test.End();
	return(KErrNone);
	}
