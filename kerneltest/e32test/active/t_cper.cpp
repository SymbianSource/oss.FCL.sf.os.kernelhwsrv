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
// e32test\active\t_cper.cpp
// Overview:
// Test periodic timers. 
// API Information:
// CPeriodic, CHeartbeat
// Details:	
// - Create some CPeriodic timer active objects with different priorities.
// - Start the periodic timers with varying delay time to start generation 
// of first event and different intervals between events
// - Verify the callback functions associated with each periodic are called 
// in order of the time when the event occurred and considering the priority 
// of the periodics.
// - Create heartbeat timer with different priorities
// - Start one heartbeat synchronized at ETwelveOClock 
// - Start two heartbeats synchronized at ETwelveOClock, ESixOClock
// - Start three heartbeats synchronized at ETwelveOClock, ESixOClock, ETwelveOClock
// - Display start time and beat time for each heartbeat timer
// - Check if the heap has been corrupted by all the tests.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// -	The first part of the test (for CPeriodic) will fail if the timers are not completed in order. 
// The test on emulator is very sensitive on the background activities on PC.
// Base Port information:
// 
//

#include <e32base.h>
#include <e32base_private.h>
#include <e32hal.h>
#include <e32test.h>
#include <hal.h>
#include <u32hal.h>
#include <e32svr.h>

LOCAL_D RTest test(_L("T_CPER"));

class myScheduler: public CActiveScheduler
	{
public:	
	virtual void Error(TInt anError) const;
	};

void myScheduler::Error(TInt anError) const
//
// virtual error handler 
//
	{
	test.Panic(anError,_L("myScheduler::Error"));
	}

TInt Array[11];
TTime Times[11];
TInt counter = 0;

CPeriodic* pPer1;
CPeriodic* pPer2;
CPeriodic* pPer3;
CPeriodic* pPer4;
CPeriodic* pPer5;
CPeriodic* pPer6;
CPeriodic* pPer7;

TInt CallBackFn(TAny* Ptr)
//
// Callback function used for all periodics
// On calling Ptr is actually a TInt - the periodic Id
//
	{
	if (counter < 11)
		{
		Array[counter] = (TInt)Ptr;
		Times[counter].HomeTime();
		counter++;
		}
	return(0);
	}

TInt CallBackPanic(TAny* Ptr)
//
// Periodic should never get called
//
	{
	test.Printf(_L("  PERIODIC %d HAS GONE OFF!\n"),(TInt)Ptr);
	test(EFalse);
	return(KErrGeneral);
	}

class myTimer: public CTimer
	{
public:
	myTimer(TInt aPriority);
	virtual void RunL(); 
	};

myTimer::myTimer(TInt aPriority) : CTimer(aPriority)
//
// Constructor - Creates AND ADDS TO MYSCHEDULER
//	
	{
	ConstructL();
	myScheduler::Add(this);
	}

void myTimer::RunL()
//
// The timer stops the scheduler
//
	{
	myScheduler::Stop();
	test.Printf(_L("   Timer has stopped ActiveScheduler\n"));
	}


//
// CHeartbeat test code
//
class CTick : public CBase, public MBeating
	{
public:
	virtual void Beat();
	virtual void Synchronize();
	void Display();
	TInt iTicks;
	TTime iStartTime;
	TTime iTimes[4];
	};
void CTick::Beat()
	{

	test.Printf(_L("Tick\n"));
	iTimes[iTicks].HomeTime();
	if (++iTicks>=4)
		CActiveScheduler::Stop();
	}
void CTick::Synchronize()
	{

	test.Printf(_L("Sync tick to system clock\n"));
	iStartTime.HomeTime();
	iTicks=0;
	}

void PrintTime(const TDesC& aName, const TTime& aTime)
	{
	TDateTime dt(aTime.DateTime());
	test.Printf(_L("%S = %02d:%02d:%02d:%06d\n"),&aName,dt.Hour(),dt.Minute(),dt.Second(),dt.MicroSecond());
	}

void CTick::Display()
	{
	PrintTime(_L("Start time"),iStartTime);
	TInt i;
	for (i=0; i<4; i++)
		{
		TBuf<16> name;
		name.Format(_L("Beat %d"),i);
		PrintTime(name,iTimes[i]);
		}
	}

class CTock : public CTick
	{
public:
	virtual void Beat();
	virtual void Synchronize();
	};

void CTock::Beat()
	{

	iTimes[iTicks++].HomeTime();
	test.Printf(_L("Tock\n"));
	}

void CTock::Synchronize()
	{

	test.Printf(_L("Sync tock to system clock\n"));
	iStartTime.HomeTime();
	iTicks=0;
	}

class CBigTock : public CTick
	{
public:
	virtual void Beat();
	virtual void Synchronize();
	};

void CBigTock::Beat()
	{

	iTimes[iTicks++].HomeTime();
	test.Printf(_L("TOCK!\n"));
	}

void CBigTock::Synchronize()
	{

	test.Printf(_L("Sync TOCK to system clock\n"));
	iStartTime.HomeTime();
	iTicks=0;
	}

void testHeartbeat()
//
// Test CHeartBeat
//
	{

	test.Start(_L("Test CHeartbeat timer"));
	CActiveScheduler *scheduler = new CActiveScheduler;
	CActiveScheduler::Install(scheduler);

	test.Next(_L("Create a beating object synchronised at ETwelveOClock"));
	CTick *tick=new CTick;
	CHeartbeat *pH=NULL;
	TRAPD(r, pH=CHeartbeat::NewL(EPriorityNormal));
	test(r==KErrNone);
	test.Next(_L("Run for 4 beats on the second"));
	pH->Start(ETwelveOClock, tick);
	CActiveScheduler::Start();
	pH->Cancel();
	tick->Display();

	User::After(1000000);
	test.Next(_L("Create another heartbeat synchronised at ESixOClock"));
	CHeartbeat *pH6=CHeartbeat::New(EPriorityNormal);
	CTock *tock=new CTock;
	test.Next(_L("Start both"));
	pH->Start(ETwelveOClock, tick);
	pH6->Start(ESixOClock, tock);
	CActiveScheduler::Start();
	tick->Display();
	tock->Display();

	pH->Cancel();
	pH6->Cancel();
	User::After(1000000);
	test.Next(_L("Create another beating object synchronised at ESixOClock with a higher priority"));
	CHeartbeat *pH2=CHeartbeat::New(EPriorityHigh);
	CBigTock *bigtock=new CBigTock;
	test.Next(_L("Start all"));
	pH->Start(ETwelveOClock, tick);
	pH6->Start(ESixOClock, tock);
	pH2->Start(ESixOClock, bigtock);
	CActiveScheduler::Start();
	pH->Cancel();
	pH2->Cancel();
	pH6->Cancel();
	tick->Display();
	tock->Display();
	bigtock->Display();

	delete pH;
	delete pH2;
	delete pH6;
	delete tock;
	delete tick;
	delete bigtock;
	delete scheduler;
	test.End();
	}

void testLockSpec()
//
// test the operators defined for TTimerLockSpec
//
	{
/*
	test.Start(_L("Test pre fix operator ++"));
	TTimerLockSpec i=ETwelveOClock,k=EOneOClock,l;
	TInt j;
	for (j=0; j<30; j++)
		{
		++k=EOneOClock;
		test(k==EOneOClock);
		k=i;
		l=++i;
		switch (k)
			{
		case EOneOClock:
			test(i==ETwoOClock);
			test(l==ETwoOClock);
			break;
		case ETwoOClock:
			test(i==EThreeOClock);
			test(l==EThreeOClock);
			break;
		case EThreeOClock:
			test(i==EFourOClock);
			test(l==EFourOClock);
			break;
		case EFourOClock:
			test(i==EFiveOClock);
			test(l==EFiveOClock);
			break;
		case EFiveOClock:
			test(i==ESixOClock);
			test(l==ESixOClock);
			break;
		case ESixOClock:
			test(i==ESevenOClock);
			test(l==ESevenOClock);
			break;
		case ESevenOClock:
			test(i==EEightOClock);
			test(l==EEightOClock);
			break;
		case EEightOClock:
			test(i==ENineOClock);
			test(l==ENineOClock);
			break;
		case ENineOClock:
			test(i==ETenOClock);
			test(l==ETenOClock);
			break;
		case ETenOClock:
			test(i==EElevenOClock);
			test(l==EElevenOClock);
			break;
		case EElevenOClock:
			test(i==ETwelveOClock);
			test(l==ETwelveOClock);
			break;
		case ETwelveOClock:
			test(i==EOneOClock);
			test(l==EOneOClock);
			break;
			}
		}

	test.Next(_L("Test post fix operator ++"));
	for (j=0; j<30; j++)
		{
		++k=EOneOClock;
		test(k==EOneOClock);
		k=i;
		l=i++;
		switch (k)
			{
		case EOneOClock:
			test(i==ETwoOClock);
			test(l==k);
			break;
		case ETwoOClock:
			test(i==EThreeOClock);
			test(l==k);
			break;
		case EThreeOClock:
			test(i==EFourOClock);
			test(l==k);
			break;
		case EFourOClock:
			test(i==EFiveOClock);
			test(l==k);
			break;
		case EFiveOClock:
			test(i==ESixOClock);
			test(l==k);
			break;
		case ESixOClock:
			test(i==ESevenOClock);
			test(l==k);
			break;
		case ESevenOClock:
			test(i==EEightOClock);
			test(l==k);
			break;
		case EEightOClock:
			test(i==ENineOClock);
			test(l==k);
			break;
		case ENineOClock:
			test(i==ETenOClock);
			test(l==k);
			break;
		case ETenOClock:
			test(i==EElevenOClock);
			test(l==k);
			break;
		case EElevenOClock:
			test(i==ETwelveOClock);
			test(l==k);
			break;
		case ETwelveOClock:
			test(i==EOneOClock);
			test(l==k);
			break;
			}
		}
	test.End();
*/
	}


GLDEF_C TInt E32Main()
	{
	
	test.Title();
	__UHEAP_MARK;
	test.Start(_L("Create some CPeriodics"));

	myScheduler* pScheduler = new myScheduler;
	myScheduler::Install(pScheduler);

	pPer1 = CPeriodic::New(0);
	pPer2 = CPeriodic::NewL(0);
	pPer3 = CPeriodic::NewL(10);
	pPer4 = CPeriodic::NewL(100);
	pPer5 = CPeriodic::NewL(100);
	pPer6 = CPeriodic::NewL(100);
	pPer7 = CPeriodic::NewL(100);
	myTimer* pTimer = new myTimer(50);

	test.Next(_L("Start them"));

	TCallBack callBack1(CallBackFn,(TAny*)1);
	TCallBack callBack2(CallBackFn,(TAny*)2);
	TCallBack callBack3(CallBackFn,(TAny*)3);
	TCallBack callBack4(CallBackPanic,(TAny*)4);
	TCallBack callBack5(CallBackPanic,(TAny*)5);
	TCallBack callBack6(CallBackPanic,(TAny*)6);
	TCallBack callBack7(CallBackPanic,(TAny*)7);

	TInt p=0;
	HAL::Get(HAL::ESystemTickPeriod, p);

	User::After(p); // ensure tick does not occur while starting all these timers

	pPer1->Start(2*p+1,7*p+1,callBack1);	//After 3 ticks, complete every 8th tick
	pPer2->Start(1,    2*p+1,callBack2);	//After 1 tick , complete every 3rd tick
	pPer3->Start(7*p+1,  p+1,callBack3);	//After 8 ticks, complete every 2nd tick

	pPer4->Start(KMaxTInt,KMaxTInt,callBack4);
	pPer5->Start(60000000,60000000,callBack5);
	pPer6->Start(KMaxTInt/91,KMaxTInt/91,callBack6);
	pPer7->Start(KMaxTInt/91+1,KMaxTInt/91+1,callBack7);
	pTimer->After(20*p-1); // ensure there's enough time for them to fill up the array.
	/*
		Time	per1   per2	  per3
		  1				-
		  2
		  3		 -
		  4				-
		  5
		  6
		  7				-
		  8					   -
		  9
		 10				-	   -
		 11		 -			   
		 12					   -
		 13				-	   
		 14					   -
	*/

	myScheduler::Start();

	TInt i;
	for (i=0; i<counter; ++i)
		{
		test.Printf(_L("   Time: %7d Periodic: %d\n"),static_cast<TUint32>(Times[i].Int64()-Times[0].Int64()),Array[i]);
		}

	test(Array[0]==2);
	test(Array[1]==1);
	test(Array[2]==2);
	test(Array[3]==2);
	test(Array[4]==3);
	TBool normal56 = (Array[5]==3 && Array[6]==2);
	TBool reverse56 = (Array[5]==2 && Array[6]==3);
	if (UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0) > 1)
		{
		// If there are multiple processors the order of 'simultaneous' timers is undefined since
		// the test may get to run as soon as the first timer is completed, instead of only after
		// the timer thread blocks, which would be after both timers completed.
		test(normal56 || reverse56);
		}
	else
		test(normal56);
	test(Array[7]==1);
	test(Array[8]==3);
	test(Array[9]==2);
	test(Array[10]==3);

	test.Next(_L("Destroy them"));

	delete pPer1;
	delete pPer2;
	delete pPer3;
	delete pPer4;
	delete pPer5;
	delete pPer6;
	delete pPer7;
	delete pTimer;
	delete pScheduler;

	test.Next(_L("Test CHeartbeat"));
	testHeartbeat();
	test.Next(_L("Test TTimerLockSpec"));
	testLockSpec();
	__UHEAP_MARKEND;
	test.End();
	return(KErrNone);
	}
