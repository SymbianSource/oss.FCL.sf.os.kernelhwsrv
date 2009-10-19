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
// e32test\system\t_chnot.cpp
// Tests RChangeNotifier class
// Overview:
// Tests RChangeNotifier class
// API Information:
// RChangeNotifier
// Details:
// - Create a RChangeNotifier object and verify the logon status is 
// as expected.
// - Call the Logon and LogonCancel methods, verify results are as
// expected.
// - Test for the correct midnight crossover notifier results in a 
// variety of situations: DST On, DST Off, various time offsets
// and various dates.
// - Test various locale changes and verify that the notifier response
// is as expected.
// - Test the notification of the death of a thread and check that
// results are as expected. Check for normal exit, kill exit, 
// terminate exit and panic exit.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32hal.h>
#include <e32svr.h>
#include <u32hal.h>
#include <e32def.h>
#include <e32def_private.h>

RTest test(_L("T_CHNOT"));

RChangeNotifier notifier;

void TestStat(const TRequestStatus& aStat, TInt aValue)
	{
	if (aStat.Int()!=aValue)
		{
		test.Printf(_L("Got %08x Expected %08x\n"),aStat.Int(),aValue);
		test(0);
		}
	}

void TestCreate()
	{
	notifier.Create();
	TRequestStatus stat;
	notifier.Logon(stat);

	// Expect all except EChangesLowMemory
	TUint expected =
		EChangesLocale |
		EChangesMidnightCrossover |
		EChangesThreadDeath |
		EChangesPowerStatus |
		EChangesSystemTime |
		EChangesFreeMemory |
		EChangesOutOfMemory |
		EChangesThrashLevel;
	
	test(stat==expected);
	}

void TestLogonLogoff()
	{
	TRequestStatus stat;
	notifier.LogonCancel();
	notifier.Logon(stat);
	TestStat(stat,KRequestPending);
	notifier.LogonCancel();
	TestStat(stat,KErrCancel);
	notifier.LogonCancel();
	TestStat(stat,KErrCancel);
	}

void DoTestMidnight()
	{
	TTime time;
	time.HomeTime();
	TDateTime dateTime=time.DateTime();
	dateTime.SetHour(23);
	dateTime.SetMinute(59);
	dateTime.SetSecond(58);
	dateTime.SetMicroSecond(700000);
	time=dateTime;
	TRequestStatus stat;
	TInt r=notifier.Logon(stat);
	test(r==KErrNone);
	TestStat(stat,KRequestPending);
	test(User::SetHomeTime(time)==KErrNone);
	time.HomeTime();
	TDateTime dateTime2=time.DateTime();
	User::WaitForRequest(stat);
	TestStat(stat,EChangesSystemTime);
	r=notifier.Logon(stat);
	test(r==KErrNone);
	User::WaitForRequest(stat);
	time.HomeTime();
	TestStat(stat,EChangesMidnightCrossover);
	dateTime2=time.DateTime();
	test(dateTime2.Second()==0);
	test(dateTime2.Minute()==0);
	test(dateTime2.Hour()==0);
	if (dateTime2.Month()==dateTime.Month())
		test(dateTime2.Day()==dateTime.Day()+1);
	else
		test(dateTime2.Day()==0);
	time=dateTime;
	r=notifier.Logon(stat);
	test(r==KErrNone);
	TestStat(stat,KRequestPending);
	test(User::SetHomeTime(time)==KErrNone);
	User::WaitForRequest(stat);
	TestStat(stat,(EChangesSystemTime|EChangesMidnightCrossover));
	time=dateTime2;
	r=notifier.Logon(stat);
	test(r==KErrNone);
	TestStat(stat,KRequestPending);
	test(User::SetHomeTime(time)==KErrNone);
	User::WaitForRequest(stat);
	TestStat(stat,(EChangesSystemTime|EChangesMidnightCrossover));

	// Check that a change of secure time also triggers notification, even though the user time is unchanged
	r = notifier.Logon(stat);
	test(r == KErrNone);
	TestStat(stat, KRequestPending);
	if ((r = time.HomeTimeSecure()) == KErrNone)
		r = User::SetHomeTimeSecure(time+TTimeIntervalSeconds(60));
	if (r == KErrNone)
		{
		test(User::SetHomeTimeSecure(time) == KErrNone);
		r = EChangesSystemTime;
		}
	else
		{
		RDebug::Printf("WARNING: Secure clock change test skipped because secure time could not be changed!");
		notifier.LogonCancel();
		r = KErrCancel;
		}
	User::WaitForRequest(stat);
	TestStat(stat, r);
	}

void SetOffsetForMidnight(TTime time,TTimeIntervalSeconds offset)
	{
	test(User::SetHomeTime(time)==KErrNone);
	User::SetUTCOffset(offset);
// No longer need next line, as we now only get one notification
//	User::After(999999);//So, if time has gone backwards, midnight crossover has been noticed
	TRequestStatus stat;
	notifier.Logon(stat);
	User::WaitForRequest(stat);
	test(stat.Int()&(EChangesSystemTime|EChangesLocale));
	}

void TestMidnightCrossover()
	{
	TTimeIntervalSeconds offset=User::UTCOffset();
	TTime time;
	time.HomeTime();
	test.Start(_L("Normal"));
	DoTestMidnight();
	test.Next(_L("Now  offset 0"));
	SetOffsetForMidnight(time,0);
	DoTestMidnight();
	test.Next(_L("Now  offset +30"));
	SetOffsetForMidnight(time,30);
	DoTestMidnight();
	test.Next(_L("Now  offset -30"));
	SetOffsetForMidnight(time,-30);
	DoTestMidnight();
	test.Next(_L("Now  offset +60"));
	SetOffsetForMidnight(time,60);
	DoTestMidnight();
	test.Next(_L("Now  offset -60"));
	SetOffsetForMidnight(time,-60);
	DoTestMidnight();
	test.Next(_L("Now  offset +120"));
	SetOffsetForMidnight(time,120);
	DoTestMidnight();
	test.Next(_L("Now  offset -120"));
	SetOffsetForMidnight(time,-120);
	DoTestMidnight();
//
	TTime time1998=TDateTime(1998,EFebruary,2,3,4,5,6);
	test.Next(_L("1998 offset 0"));
	SetOffsetForMidnight(time1998,0);
	DoTestMidnight();
	test.Next(_L("1998 offset +30"));
	SetOffsetForMidnight(time1998,30);
	DoTestMidnight();
	test.Next(_L("1998 offset -30"));
	SetOffsetForMidnight(time1998,-30);
	DoTestMidnight();
	test.Next(_L("1998 offset +60"));
	SetOffsetForMidnight(time1998,60);
	DoTestMidnight();
	test.Next(_L("1998 offset -60"));
	SetOffsetForMidnight(time1998,-60);
	DoTestMidnight();
	test.Next(_L("1998 offset +120"));
	SetOffsetForMidnight(time1998,120);
	DoTestMidnight();
	test.Next(_L("1998 offset -120"));
	SetOffsetForMidnight(time1998,-120);
	DoTestMidnight();
//
	TTime time1999=TDateTime(1999,EDecember,30,3,4,5,6);
	test.Next(_L("1999 offset 0"));
	SetOffsetForMidnight(time1999,0);
	DoTestMidnight();
	TTime now;
	now.HomeTime();
	test(now.DateTime().Year()==2000);
	test(now.DateTime().Month()==EJanuary);
	test.Next(_L("1999 offset +30"));
	SetOffsetForMidnight(time1999,30);
	DoTestMidnight();
	now.HomeTime();
	test(now.DateTime().Year()==2000);
	test(now.DateTime().Month()==EJanuary);
	test.Next(_L("1999 offset -30"));
	SetOffsetForMidnight(time1999,-30);
	DoTestMidnight();
	now.HomeTime();
	test(now.DateTime().Year()==2000);
	test(now.DateTime().Month()==EJanuary);
	test.Next(_L("1999 offset +60"));
	SetOffsetForMidnight(time1999,60);
	DoTestMidnight();
	now.HomeTime();
	test(now.DateTime().Year()==2000);
	test(now.DateTime().Month()==EJanuary);
	test.Next(_L("1999 offset -60"));
	SetOffsetForMidnight(time1999,-60);
	DoTestMidnight();
	now.HomeTime();
	test(now.DateTime().Year()==2000);
	test(now.DateTime().Month()==EJanuary);
	test.Next(_L("1999 offset +120"));
	SetOffsetForMidnight(time1999,120);
	DoTestMidnight();
	now.HomeTime();
	test(now.DateTime().Year()==2000);
	test(now.DateTime().Month()==EJanuary);
	test.Next(_L("1999 offset -120"));
	SetOffsetForMidnight(time1999,-120);
	DoTestMidnight();
	now.HomeTime();
	test(now.DateTime().Year()==2000);
	test(now.DateTime().Month()==EJanuary);
//
	TTime time2002=TDateTime(2002,EAugust,30,3,4,5,6);
	test.Next(_L("2002 offset 0"));
	SetOffsetForMidnight(time2002,0);
	DoTestMidnight();
	test.Next(_L("2002 offset +30"));
	SetOffsetForMidnight(time2002,30);
	DoTestMidnight();
	test.Next(_L("2002 offset -30"));
	SetOffsetForMidnight(time2002,-30);
	DoTestMidnight();
	test.Next(_L("2002 offset +60"));
	SetOffsetForMidnight(time2002,60);
	DoTestMidnight();
	test.Next(_L("2002 offset -60"));
	SetOffsetForMidnight(time2002,-60);
	DoTestMidnight();
	test.Next(_L("2002 offset +120"));
	SetOffsetForMidnight(time2002,120);
	DoTestMidnight();
	test.Next(_L("2002 offset -120"));
	SetOffsetForMidnight(time2002,-120);
	DoTestMidnight();
//
	SetOffsetForMidnight(time,offset);
	test.End();
	}

void TestLocaleChanges()
	{
	TRequestStatus stat;
	notifier.Logon(stat);
	TestStat(stat,KRequestPending);
	TLocale	locale;
	locale.Set();
	User::WaitForRequest(stat);
	TestStat(stat,EChangesLocale);
	}

void TestOffsetChanges()
	{
	TTimeIntervalSeconds oldOffset = User::UTCOffset();
	User::SetUTCOffset(0);
	
	TRequestStatus stat;
	TTime time;
	time.HomeTime();
	TDateTime dateTime=time.DateTime();
	dateTime.SetHour(23);
	dateTime.SetMinute(30);
	dateTime.SetSecond(0);
	dateTime.SetMicroSecond(0);
	time=dateTime;
	test(User::SetHomeTime(time)==KErrNone);
	notifier.Logon(stat);
	User::WaitForRequest(stat);
	TestStat(stat,(EChangesSystemTime));

	notifier.Logon(stat);
	TestStat(stat,KRequestPending);
	User::SetUTCOffset(3600);
	User::WaitForRequest(stat);
	TestStat(stat,(EChangesSystemTime|EChangesLocale|EChangesMidnightCrossover));
	User::SetUTCOffset(oldOffset);
	notifier.Logon(stat);
	User::WaitForRequest(stat);
	}

const TInt retValue=65432;
const TInt killValue=2081953;
const TInt terminateValue=512123;
const TInt panicValue=1257671;
const TInt KHeapSize=0x200;

TInt ThreadCode(TAny* aReturnImmetiateFlag)
	{
	if(!aReturnImmetiateFlag)
		User::After(60000000); // wait a minute, (effectively forever as far as the test goes).
	return retValue;
	}

void TestThreadDeath()
	{
	test.Start(_L("Normal Exit"));
	RThread	thread;
	TInt r=thread.Create(_L("T_CHNOT ThreadCode"),ThreadCode,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETrue);
	test(r==KErrNone);
	__KHEAP_MARK;
	TRequestStatus threadStat;
	thread.Logon(threadStat);
	TRequestStatus stat;
	notifier.Logon(stat);
	TestStat(stat,KRequestPending);
	thread.Resume();
	User::WaitForRequest(stat);
	TestStat(stat,EChangesThreadDeath);
	test(threadStat==retValue);
	test(thread.ExitReason()==retValue);
	test(thread.ExitType()==EExitKill);
	test(thread.ExitCategory()==_L("Kill"));
	CLOSE_AND_WAIT(thread);
	__KHEAP_MARKEND;

	test.Next(_L("Kill"));
	r=thread.Create(_L("T_CHNOT ThreadCode"),ThreadCode,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	thread.Logon(threadStat);
	notifier.Logon(stat);
	TestStat(stat,KRequestPending);
	thread.Resume();
	thread.Kill(killValue);
	User::WaitForRequest(stat);
	TestStat(stat,EChangesThreadDeath);
	test(threadStat==killValue);
	test(thread.ExitReason()==killValue);
	test(thread.ExitType()==EExitKill);
	test(thread.ExitCategory()==_L("Kill"));
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Terminate"));
	r=thread.Create(_L("T_CHNOT ThreadCode"),ThreadCode,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	thread.Logon(threadStat);
	notifier.Logon(stat);
	TestStat(stat,KRequestPending);
	thread.Resume();
	thread.Terminate(terminateValue);
	User::WaitForRequest(stat);
	TestStat(stat,EChangesThreadDeath);
	test(threadStat==terminateValue);
	test(thread.ExitReason()==terminateValue);
	test(thread.ExitType()==EExitTerminate);
	test(thread.ExitCategory()==_L("Terminate"));
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Panic"));
	r=thread.Create(_L("T_CHNOT ThreadCode"),ThreadCode,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	thread.Logon(threadStat);
	notifier.Logon(stat);
	TestStat(stat,KRequestPending);
	TBool justInTime=User::JustInTime(); 
	User::SetJustInTime(EFalse); 
	thread.Resume();
	thread.Panic(_L("Testing panic"),panicValue);
	User::WaitForRequest(stat);
	User::SetJustInTime(justInTime); 
	TestStat(stat,EChangesThreadDeath);
	test(threadStat==panicValue);
	test(thread.ExitReason()==panicValue);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitCategory()==_L("Testing panic"));
	CLOSE_AND_WAIT(thread);
	test.End();
	}

void TestCloseWhilstPending()
	{
	test_KErrNone(notifier.Create());
	TRequestStatus stat;
	test_KErrNone(notifier.Logon(stat));
	User::WaitForRequest(stat);
	test_KErrNone(notifier.Logon(stat));
	notifier.Close();
	test_Equal(KErrGeneral,stat.Int());
	}

void TestCloseAndCompleteRace()
	{
	RThread().SetPriority(EPriorityRealTime);

	// setup notifier2
	RChangeNotifier notifier2;
	test_KErrNone(notifier2.Create());
	TRequestStatus stat2;
	test_KErrNone(notifier2.Logon(stat2));
	User::WaitForRequest(stat2);
	test_KErrNone(notifier2.Logon(stat2));

	// setup notifier
	test_KErrNone(notifier.Create());
	TRequestStatus stat;
	test_KErrNone(notifier.Logon(stat));
	User::WaitForRequest(stat);
	test_KErrNone(notifier.Logon(stat));

	// create and kill a thread so notifiers get signaled
	RThread	thread;
	test_KErrNone(thread.Create(_L("T_CHNOT ThreadCode"),ThreadCode,KDefaultStackSize,KHeapSize,KHeapSize,NULL));
	thread.Kill(0);

	// wait for notifier2
	User::WaitForRequest(stat2);

	// as this thread is realtime priority, then (on unicore systems) it has preempted
	// kernel supervisor thread after it completed 'notifier2' but before it completed
	// 'notifier'. if we close both notifiers now we trigger a race conidition which
	// previousely caused a null pointer dereference in the kernel...
	notifier.Close();
	notifier2.Close();

	User::WaitForRequest(stat);
	TInt result = stat.Int();

	// expect KErrGeneral from closing notifier, or on SMP probably EChangesThreadDeath as
	// the notifier had time to complete
	const TInt numCpus = UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0);
	if(numCpus==1 || result!=EChangesThreadDeath)
		test_Equal(KErrGeneral,result);

	RThread().SetPriority(EPriorityNormal);
	thread.Close();
	}

TInt E32Main()
	{

	User::After(1000000);//So WINS doesn't give an instant power-status change;
	test.Start(_L("Create"));
	TestCreate();

	test.Next(_L("Close"));
	notifier.Close();

	test.Next(_L("Create"));
	TestCreate();

	test.Next(_L("Logon/Logoff"));
	TestLogonLogoff();

	test.Next(_L("Midnight crossover"));
	TestMidnightCrossover();

	test.Next(_L("Locale changes"));
	TestLocaleChanges();

	test.Next(_L("Offset changes"));
	TestOffsetChanges();

	test.Next(_L("Thread death"));
	TestThreadDeath();

	test.Next(_L("Close"));
	notifier.Close();

	test.Next(_L("Close whilst pending"));
	TestCloseWhilstPending();

	test.Next(_L("Race between Close and complete"));
	TestCloseAndCompleteRace();

	test.End();
	return KErrNone;
	}
