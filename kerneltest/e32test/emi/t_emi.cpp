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
// e32test\emi\t_emi.cpp
// 
//

#include <e32test.h>
#include <e32math.h>
#include <e32def.h>
#include <e32def_private.h>
#include "d_emitest.h"

//#define VERBOSE

LOCAL_D RTest test(_L("T_EMI"));

_LIT(KEMITestLddFileName,"D_EMITEST");
const TInt KHeapSize=0x200;

REMITest Ldd;


/*
	TestEventLogging
	
	Tests initialisation of the logging system, ensuring no memory leaks, and then that user events can
	be added and read correctly.
	
	The test tries filling and emptying the buffer from each possible start position.
	Each time it over fills it (Causing offset to rotate) and each time try and take too many.
	The last record should show the event lost flag. Each field is checked for consistency.

*/

void TestEventLogging()
	{		
	TInt no;
	TInt i;
	TInt ii;
	TInt iii;
	TUint time;
	TInt wrapcount = 0;
	TInt changecount = 0;
 
	TUserTaskEventRecord outrec;
	TUserTaskEventRecord rec[5];
	TUserTaskEventRecord blank;

	
//! @SYMTestCaseID t_emi_0
//! @SYMTestType CT
//! @SYMTestCaseDesc Set/Get State
//! @SYMPREQ PREQ898
//! @SYMTestActions Sets the EMI user state randomly 100 times, and reads back the state. 
//! @SYMTestExpectedResults The read back state should always be the same as set.
//! @SYMTestPriority High
//! @SYMTestStatus Defined
	
	test.Next(_L("(Set/Get)State"));
	for (i=0; i<100; i++) 
		{
		no = Math::Random();	
		Ldd.SetState(no);
		test(Ldd.GetState()==no);
		}
	
//! @SYMTestCaseID t_emi_1
//! @SYMTestType CT
//! @SYMTestCaseDesc Log Alloc/Free
//! @SYMPREQ PREQ898
//! @SYMTestActions Allocates different sizes of event log.
//! @SYMTestExpectedResults No memory leak.
//! @SYMTestPriority High
//! @SYMTestStatus Defined
	
	test.Next(_L("TaskEventLogging - Log Alloc/Free"));
	
	__KHEAP_MARK;	
	test(Ldd.TaskEventLogging(EFalse,0,ENone)==KErrNone);
	test(Ldd.TaskEventLogging(EFalse,2,ENone)==KErrNone);
	test(Ldd.TaskEventLogging(EFalse,200,ENone)==KErrNone);
	test(Ldd.TaskEventLogging(EFalse,0,ENone)==KErrNone);
	__KHEAP_MARKEND;

#ifdef _DEBUG
	__KHEAP_MARK;
	__KHEAP_FAILNEXT(1);
	test(Ldd.TaskEventLogging(EFalse,200,ENone)==KErrNoMemory);
	__KHEAP_MARKEND;
#endif


//! @SYMTestCaseID t_emi_2
//! @SYMTestType CT
//! @SYMTestCaseDesc AddTaskEvent/GetTaskEvent
//! @SYMPREQ PREQ898
//! @SYMTestActions Tries filling and emptying the buffer from each possible start position. Each time it over fills it (Causing offset to rotate) and each time try and take too many.
//! @SYMTestExpectedResults The last record of each group should show the event lost flag.  Except for the lost record, the contents should be consistant with when the record was added.
//! @SYMTestPriority High
//! @SYMTestStatus Defined

	test.Next(_L("AddTaskEvent/GetTaskEvent"));
	
	test(!Ldd.GetTaskEvent(outrec));

	test(Ldd.TaskEventLogging(EFalse,5,ENone)==KErrNone);
	
	test(Ldd.AddTaskEvent(blank));
	test(Ldd.GetTaskEvent(outrec));

	time=outrec.iTime;

	test(!Ldd.GetTaskEvent(outrec));
		
	// This next test tries filling and emptying the buffer from each possible start position.
	// Each time it over fills it (Causeing offset to rotate) and each time try and take too many.
	// The last record should show the event lost flag.
		
	// To ensure the time field is samething like you'd expect, let it tick over by 50.
	iii=0;
	do 	
		{
		for (ii=0; ii<5; ii++) 
			{
			test(Ldd.AddTaskEvent(blank)); //will be lost
			for (i=0; i<5; i++)
				{
				rec[i].iType=(TUint8) ((Math::Random() & 255) | 128);
				rec[i].iFlags=0;
				rec[i].iExtra=(TUint16) (Math::Random() & 65535);
				rec[i].iPrevious=&rec[i];
				rec[i].iNext=&rec[ii];
				no = Math::Random();
				Ldd.SetState(no);
				test(Ldd.AddTaskEvent(rec[i]));
				rec[i].iUserState=no;
				}
			
			for (i=0; i<5; i++)
				{			
				test(Ldd.GetTaskEvent(outrec));
#ifdef VERBOSE
				test.Printf(_L("%x : Type %x(%x), Time %x, flag %x, ThdA %x ThdB %x\n"),i,outrec.iType,rec[i].iType,outrec.iTime, outrec.iFlags, outrec.iPrevious,outrec.iNext);
#endif
				if (i==0)
					test((outrec.iFlags & KEMI_EventLost)==KEMI_EventLost);
				else 
					test((outrec.iFlags & KEMI_EventLost)!=KEMI_EventLost);
				
				test(outrec.iType==rec[i].iType);
				test(outrec.iExtra==rec[i].iExtra);
				test(outrec.iPrevious==rec[i].iPrevious);
				test(outrec.iNext==rec[i].iNext);
				test(outrec.iUserState==rec[i].iUserState);
				if (outrec.iTime!=time)
					{
					if (outrec.iTime<time)
						wrapcount++;
					time=outrec.iTime;
					changecount++;
					}
				}
			test(!Ldd.GetTaskEvent(outrec));
			}
#ifdef VERBOSE
		test.Printf(_L("Time = %i\n"),time);
#endif
		iii++;		
		if (changecount<50)
			User::After(1);
		} while ((changecount<50) && (iii<50));
	test(changecount>=50);
	test(wrapcount<2);
	}



/* Used by 	TestStartStopMonitors() and	TestScheduleEventLogging() */

TBool ShortLivedThreadDone;

LOCAL_C TInt ShortLivedThread(TAny*)
	{
	int i;
	for (i=0; i<50; i++)
		User::After(1);
	ShortLivedThreadDone=ETrue;
	return(KErrNone);
	}


/*
  TestStartStopMonitors
  
  Checks Start monitor was iterated with current thread list on startup.
  It then creates and destroys a thread, and see if it triggered the
  monitor callbacks in the test driver.  The test driver will add
  events to the event log, which are read and checked.

*/

void TestStartStopMonitors()
	{	
	TInt i;
	TInt no;
	TUserTaskEventRecord outrec;
	RThread thread;
	TRequestStatus stat;		
	TAny* myNThread;
	TInt seenIdle;
	TInt seenSigma;

	TAny* idleThread;
	TAny* sigmaThread;

//! @SYMTestCaseID t_emi_5
//! @SYMTestType CT
//! @SYMTestCaseDesc Null/Sigma
//! @SYMPREQ PREQ898
//! @SYMTestActions Get the null and sigma thread pointers.
//! @SYMTestExpectedResults Non-null pointers, which are usable in the following tests.
//! @SYMTestPriority High
//! @SYMTestStatus Defined


	test.Next(_L("Check Null/Sigma"));
	sigmaThread=Ldd.GetSigmaThread();
	test(sigmaThread!=NULL);			
	
	idleThread=Ldd.GetIdleThread();
	test(idleThread!=NULL);	

	sigmaThread=((TInt*) sigmaThread)+1;
	idleThread=((TInt*) idleThread)+1;

//! @SYMTestCaseID t_emi_3
//! @SYMTestType CT
//! @SYMTestCaseDesc Thread start/stop Monitors
//! @SYMPREQ PREQ898
//! @SYMTestActions The start stop monitors are enabled.  The start/stop monitors have been made to add event log entries, to allow there execution to be visible. After all existing threads have be iterated by the start monitor, the log is first filled with random entries for the thread, and then the log is watched as a thread is started and stopped.
//! @SYMTestExpectedResults At least 15 threads should be iterated on activation, and then both monitors should be called for the new thread. The random entries should have changed to refer to the sigma thread.
//! @SYMTestPriority High
//! @SYMTestStatus Defined

	test.Next(_L("Thread start/stop Monitors"));
	for (i=0; i<3; i++)
		{
		test(Ldd.TaskEventLogging(EFalse,50,ENone)==KErrNone);
		test(Ldd.TaskEventLogging(EFalse,50,ENormal)==KErrNone);
	

		// Check Start monitor was iterated with current thread list.
		test.Printf(_L("Check start monitor called for theads. (try %d)\n"),i);
		
		seenIdle=0;
		seenSigma=0;	

		no=0;
		while (Ldd.GetTaskEvent(outrec))
			{
			test(outrec.iType==128);
			if (outrec.iPrevious==sigmaThread)
				{
				test.Printf(_L("Seen Sigma Thread\n"));	
				seenSigma++;
				}
			if (outrec.iPrevious==idleThread)
				{
				test.Printf(_L("Seen Idle Thread\n"));	
				seenIdle++;
				}

			no++;
			}
		test.Printf(_L("Records = %d\n"),no);	
		test(no>=15);
		test(seenSigma==1);
		test(seenIdle==1);
		}
		
		
		
	test.Next(_L("Check monitor behaviour on thead creation/deletion"));

	// To test monitors, we need to create and destroy a thread, and see if it 
	// triggered the callbacks in the test driver.  The test driver will add
	// events to the event log.
	

	// First Check for background noise
	for (i=0; i<50; i++)
		{		
		test(!Ldd.GetTaskEvent(outrec));
		User::After(1);
		}
		
	test.Printf(_L("Create Thread..\n"));
	// Start thread, and let it die
	
	test(thread.Create(_L("ShortLivedThread"),ShortLivedThread,KDefaultStackSize,KHeapSize,KHeapSize,NULL)==KErrNone);
	thread.Logon(stat);
	test(stat==KRequestPending);
	
	myNThread = Ldd.GetNThread(thread);

	// Fill with records - should not survive cleanup
	for (i=0; i<50; i++)
		{
		outrec.iType=(TUint8) (((Math::Random() & 63) + 10) | 128);
		
		outrec.iPrevious= i<5?myNThread:(TAny*) Math::Random();
		outrec.iNext= i>10?myNThread:(TAny*) Math::Random();
	
		test(Ldd.AddTaskEvent(outrec));
		}	
		
	// Let thead run
	thread.Resume();
	User::WaitForRequest(stat);
	thread.Close();
	test(Ldd.TaskEventLogging(EFalse,50,ENone)==KErrNone);
	
	// Did it leave log as expected?
	test.Printf(_L("Check for logging/Clearing\n"));
	i = 0;
	no=0;
	while (Ldd.GetTaskEvent(outrec)) 
		{
		i++;
#ifdef VERBOSE
		test.Printf(_L("%x : Type %x, Time %x,ThdA %x ThdB %x\n"),i,outrec.iType,outrec.iTime, outrec.iPrevious,outrec.iNext);
#endif
		if (outrec.iType==128) 
			{
			test((TInt)outrec.iPrevious==((TInt) myNThread)+4);
			no++;
			}
		if (outrec.iType==129)
			{				
			test(outrec.iPrevious==myNThread);
			no++;
			}
		else
			{
			test(outrec.iPrevious!=myNThread);
			test(outrec.iNext!=myNThread);				
			}
		}
	test (no==2);
	}



/* 

Thread Tracking functions, 
used in	TestScheduleEventLogging() and TestTagMaskDFC()

*/

struct TThreadList
	{
	TAny* iThread;
	TUint iRan;
	};

TThreadList ThreadList[51];
TInt NoThreads=0;

inline TUint LookupThread(TAny* aThread)
	{
	for (TInt i=0; i<NoThreads;i++)
		if (aThread==ThreadList[i].iThread)
			return i;
	if (NoThreads<49)
		{
			ThreadList[NoThreads].iThread=aThread;
			ThreadList[NoThreads].iRan=0;
			NoThreads++;
		
		}
	return NoThreads-1;	
	}
	
void AddThread(TAny* aThread) 
	{
	test(NoThreads<49);
	ThreadList[NoThreads].iThread=aThread;
	ThreadList[NoThreads].iRan=0;
	NoThreads++;
	}


/*
  TestScheduleEventLogging
  
  Checks AfterIdle, Null/Sigma thread lookup, Set/Get Loggable, as well as
  Schedule Event logging.  Select threads are marked as loggable, and starts a
  new thread (that repeatadly sleeps for short times, to generate events).
  The events are then collected and all of the fields checked.  The test
  
*/

void TestScheduleEventLogging()
	{	
	RThread thread;
	TRequestStatus stat;		
	TAny* myNThread;
	TInt i;
	TInt ii;
	TUint time=0;
	TInt wrapcount = 0;
	TInt changecount = 0;
	TUserTaskEventRecord outrec;


//! @SYMTestCaseID t_emi_4
//! @SYMTestType CT
//! @SYMTestCaseDesc AfterIdle
//! @SYMPREQ PREQ898
//! @SYMTestActions Uses AfterIdle.
//! @SYMTestExpectedResults No panic.
//! @SYMTestPriority High
//! @SYMTestStatus Defined


	test.Next(_L("Check AfterIdle"));
	Ldd.AfterIdle(100);
		
	
//! @SYMTestCaseID t_emi_6
//! @SYMTestType CT
//! @SYMTestCaseDesc Set/Get Loggable
//! @SYMPREQ PREQ898
//! @SYMTestActions Get/Set ThreadLoggable are used on multiple threads.
//! @SYMTestExpectedResults The Get function should confirm that the Set function changed the loggable state.
//! @SYMTestPriority High
//! @SYMTestStatus Defined

	test.Next(_L("Check Set/Get Loggable"));
	myNThread=Ldd.GetSigmaThread();
	test(myNThread!=NULL);		
	AddThread(myNThread);
		
	myNThread=Ldd.GetIdleThread();
	test(myNThread!=NULL);		
	AddThread(myNThread);

	
	Ldd.SetThreadLoggable(myNThread,ETrue);
	test(Ldd.GetThreadLoggable(myNThread)!=EFalse);

	Ldd.SetThreadLoggable(myNThread,EFalse);
	test(Ldd.GetThreadLoggable(myNThread)==EFalse);

	Ldd.SetThreadLoggable(myNThread,ETrue);
	test(Ldd.GetThreadLoggable(myNThread)!=EFalse);
		
	
//! @SYMTestCaseID t_emi_7
//! @SYMTestType CT
//! @SYMTestCaseDesc Set/Get VEMSData
//! @SYMPREQ PREQ898
//! @SYMTestActions Get/Set VEMSData are used on a thread 100 times.
//! @SYMTestExpectedResults The Get function should confirm that the Set function set the value correctly.
//! @SYMTestPriority High
//! @SYMTestStatus Defined

	
	test.Next(_L("(Set/Get) VEMSData"));
	
	test.Printf(_L("Spawn Thread\n"));
	ShortLivedThreadDone=EFalse;
	test(thread.Create(_L("ShortLivedThread"),ShortLivedThread,KDefaultStackSize,KHeapSize,KHeapSize,NULL)==KErrNone);
	thread.Logon(stat);
	test(stat==KRequestPending);	
	myNThread = Ldd.GetNThread(thread);
	Ldd.SetThreadLoggable(myNThread,ETrue);
	AddThread(myNThread);
	test(Ldd.GetThreadLoggable(myNThread)!=EFalse);

	for (i=0; i<100; i++) 
		{
		ii = Math::Random();
		Ldd.SetVEMSData(myNThread,(TAny*)ii);
		test((TInt)Ldd.GetVEMSData(myNThread)==ii);
		}

//! @SYMTestCaseID t_emi_8
//! @SYMTestType CT
//! @SYMTestCaseDesc Task Event Logging
//! @SYMPREQ PREQ898
//! @SYMTestActions Task event logging is turned on, with a thread running in the background. Some threads set as loggable. Each event record is inspected, and a count for the seen thread in incremented. User state is incremented on each iteration.
//! @SYMTestExpectedResults Each thread ran at lest 10 times.  Time changed over 45 times, but wrapped no more then once. User state changed over 15, some events where seen to be waiting, thread order is consistent, record type is correct, and at lest 100 records where read before the test finished.
//! @SYMTestPriority High
//! @SYMTestStatus Defined


		
	test.Next(_L("Task Event Logging"));	
	
	Ldd.SetState(0);
	thread.Resume();
	User::After(1);
	test(Ldd.TaskEventLogging(ETrue,50,ENone)==KErrNone);
	i=0;
	ii=0;
	TInt prevNextThread=-1;
	TInt lastThread;
	TInt nextThread;
	TBool seenWaiting=EFalse;
	wrapcount=0;
	changecount=0;
	TUint lastUserState=0;
	TInt userStateChange=0;
	FOREVER
		{
		
		if (!Ldd.GetTaskEvent(outrec))
			{			
			if ((i>100) && ShortLivedThreadDone)
				break;
			Ldd.AfterIdle(5);
			test((ii++)<500); //timeout!
			}
		else
			{
			lastThread=LookupThread(outrec.iPrevious);
			nextThread=LookupThread(outrec.iNext);
#ifdef VERBOSE
			test.Printf(_L("%x: Ty %x Tm %x ThdA %x ThdB %x F %x U %x\n"),
				i,outrec.iType,outrec.iTime, lastThread,nextThread, outrec.iFlags, outrec.iUserState);
#endif
			test(outrec.iType==0);
			
			
			if (outrec.iTime!=time)
				{
				if ((outrec.iTime<time) && (time!=0))
					wrapcount++;
				time=outrec.iTime;
				changecount++;
				}			
			
			if ((outrec.iFlags & KEMI_PrevWaiting)!=0)
				seenWaiting=ETrue;
			
			if (((outrec.iFlags & KEMI_EventLost)==0) && (prevNextThread!=-1))
				test (prevNextThread==lastThread);
			prevNextThread=nextThread;
			ThreadList[nextThread].iRan++;

			test(outrec.iUserState>=lastUserState);
			if (outrec.iUserState!=lastUserState)
				{
				userStateChange++;
				lastUserState=outrec.iUserState;
				}

			i++;
			Ldd.SetState(i);
			}
		if ((i>100) && ShortLivedThreadDone)
			test(Ldd.TaskEventLogging(EFalse,50,ENone)==KErrNone);			
		}

	test.Printf(_L("DTime %x DState %x Wrap %x\n"),changecount,userStateChange, wrapcount);

	test(changecount>=45);
	test(userStateChange>15);
	test(wrapcount<2);
		
	test(seenWaiting);
	
	User::WaitForRequest(stat);
	thread.Close();
	
	for (i=0; i<NoThreads;i++)
		{		
		test.Printf(_L("Thread %x ran %d times.\n"),i,ThreadList[i].iRan);
		test(ThreadList[i].iRan>10);
		}
	}




/* TagMaskTestThread() - used by TestTagMaskDFC */
LOCAL_C TInt TagMaskTestThread(TAny*)
	{
	TInt i;
	TInt mask=1;
	for (i=0; i<31; i++)
		{		
		User::After(1);
		mask = mask << 1;
		Ldd.SetMask(mask);
		}
	ShortLivedThreadDone=ETrue;
	return(KErrNone);
	}

/*
  TestTagMaskDFC()

  Checks Mask and Tag setting.
  Checks a that a DFC (found in driver) is triggered my an apropriate tag and mask.
*/

void TestTagMaskDFC()
	{	
	RThread thread;
	TRequestStatus stat;		
	TAny* myNThread;
	TUserTaskEventRecord outrec;
	TInt i=0;
	TInt ii=0;
	TInt seen9=0;
	TInt seen18=0;
	TInt seen27=0;	
	TUint no;
	
#ifdef VERBOSE	
	TInt lastThread;
#endif	
	TInt nextThread;


//! @SYMTestCaseID t_emi_9
//! @SYMTestType CT
//! @SYMTestCaseDesc Set/Get Mask
//! @SYMPREQ PREQ898
//! @SYMTestActions Get/Set the mask 100 times.
//! @SYMTestExpectedResults The Get function should confirm that the Set function changed the mask state.
//! @SYMTestPriority High
//! @SYMTestStatus Defined


	test.Next(_L("(Set/Get)Mask"));
	for (i=0; i<100; i++) 
		{
		no = Math::Random();
		Ldd.SetMask(no);
		test(Ldd.GetMask()==no);
		}

//! @SYMTestCaseID t_emi_10
//! @SYMTestType CT
//! @SYMTestCaseDesc Tag/Mask DFC
//! @SYMPREQ PREQ898
//! @SYMTestActions A DFC is set than will change the user state to the results of GetDFCTriggerTag.  The event log is monitored, with a background thread, which has had a tag set.  This thread repeatedly changes the mask.
//! @SYMTestExpectedResults The state should be seen to change once to the correct tag for the thread.  Other factors also checked in t_emi_7 should show the test running correctly.
//! @SYMTestPriority High
//! @SYMTestStatus Defined


	test.Next(_L("Tag/Mask DFC."));
	
	NoThreads=0; // Reset Thread Tracking
	
	myNThread=Ldd.GetSigmaThread();
	test(myNThread!=NULL);		
	AddThread(myNThread);
	
	myNThread=Ldd.GetIdleThread();
	test(myNThread!=NULL);		
	AddThread(myNThread);
	
	Ldd.SetThreadLoggable(myNThread,ETrue);
	test(Ldd.GetThreadLoggable(myNThread)!=EFalse);
		
	Ldd.SetDfc();
	test(Ldd.GetMask()==0);
	
	test.Printf(_L("Spawn Thread\n"));
	ShortLivedThreadDone=EFalse;
	test(thread.Create(_L("TagMaskTestThread"),TagMaskTestThread,KDefaultStackSize,KHeapSize,KHeapSize,NULL)==KErrNone);
	thread.Logon(stat);
	test(stat==KRequestPending);	
	myNThread = Ldd.GetNThread(thread);
	test(Ldd.GetThreadLoggable(myNThread)==EFalse);
	Ldd.SetThreadLoggable(myNThread,ETrue);
	test(Ldd.GetThreadLoggable(myNThread)!=EFalse);
	AddThread(myNThread);
	Ldd.SetThreadTag(myNThread,(1<<9)+(1<<18)+(1<<27));
	test(Ldd.GetThreadTag(myNThread)==(1<<9)+(1<<18)+(1<<27));
	
	Ldd.SetState(0);
	
	test.Printf(_L("Start Event Logging\n"));
	test(Ldd.TaskEventLogging(ETrue,50,ENone)==KErrNone);

	thread.Resume();
	
	FOREVER
		{		
		if (!Ldd.GetTaskEvent(outrec))
			{			
			if ((i>100) && ShortLivedThreadDone)
				break;
			Ldd.AfterIdle(5);
			test((ii++)<500); //timeout!
			}
		else
			{

#ifdef VERBOSE
			lastThread=LookupThread(outrec.iPrevious);
#else
			LookupThread(outrec.iPrevious);
#endif
			nextThread=LookupThread(outrec.iNext);
			
#ifdef VERBOSE
			test.Printf(_L("%x: Ty %x Tm %x ThdA %x ThdB %x F %x U %x\n"),
				i,outrec.iType,outrec.iTime, lastThread,nextThread, outrec.iFlags, outrec.iUserState);
#endif

			ThreadList[nextThread].iRan++;
				
			switch(outrec.iUserState)
				{
				case 0      : break;
				case (1<<9) : ++seen9; break;
				case (1<<18): ++seen18; break;
				case (1<<27): ++seen27; break;
				default: test(0); // bad state
				}
			}
		i++;		
		if ((i>100) && ShortLivedThreadDone)
			test(Ldd.TaskEventLogging(EFalse,50,ENone)==KErrNone);
		
		}
	Ldd.SetMask(0);

	test(seen9);
	test(seen18);
	test(seen27);
	User::WaitForRequest(stat);
	thread.Close();
		
	for (i=0; i<NoThreads;i++)
		{		
		test.Printf(_L("Thread %x ran %d times.\n"),i,ThreadList[i].iRan);
		test(ThreadList[i].iRan>10);
		}
	test(Ldd.TaskEventLogging(EFalse,0,ENone)==KErrNone);	
	}


/*
void SoakTest()

Repeatedly changes the start/exit monitors, while repeatedly starting and stopping threads.
As threads exit, they are checked for consistency.

*/

TBool FinishedSoak=EFalse;
TBool Going=EFalse;
LOCAL_C TInt SoakTest_Thread(TAny*)
	{
	int i;
	Going=ETrue;
	for (i=(Math::Random() & 1); i<2; i++)
			User::AfterHighRes(1000+(Math::Random() & 31));
	return(KErrNone);
	}

TInt MainCount=0;
TInt ThreadGenCount=0;
TInt Maxloops=4100;
TBool Forever=EFalse;
TInt State=1;
TInt GoodCount=0;

LOCAL_C TInt SoakTest_ThreadGenerator(TAny*)
	{
	const TInt NOTHREADS=100;

	RThread threadlist[NOTHREADS];
	TRequestStatus threadlist_stat[NOTHREADS];
	TInt head=0;
	TInt tail=0;
	TBool done=EFalse;
	TInt newHead;
	RThread thisThread = RThread();
	FOREVER
		{
		
		switch (Math::Random() & 63) 
			{
			case 1: thisThread.SetPriority(EPriorityLess);
			break;
			case 2: thisThread.SetPriority(EPriorityNormal);
			break;
			case 3: thisThread.SetPriority(EPriorityMore);
			break;
			}
			
		newHead = head+1;
		if (newHead==NOTHREADS)
			newHead=0;
		if ((newHead!=tail) &&  !done) //not full & not finished
			{
			test(threadlist[head].Create(_L(""),SoakTest_Thread,KDefaultStackSize,NULL,NULL)==KErrNone);
			threadlist[head].Logon(threadlist_stat[head]);
			switch (Math::Random() & 3) 
				{
				case 1: threadlist[head].SetPriority(EPriorityLess);
				break;
				case 2: threadlist[head].SetPriority(EPriorityNormal);
				break;
				case 3: threadlist[head].SetPriority(EPriorityMore);
				break;
				}

			threadlist[head].Resume();
			head=newHead;
			}
		else
			{
			if (head==tail)//empty
				{
				
				if (done)
					break;			
				}

			else
				{				
				User::WaitForRequest(threadlist_stat[tail] );
	
				if (State==1)
					{
					State=(TInt) Ldd.GetVEMSData(Ldd.GetNThread(threadlist[tail]));
					if (State!=1)
						done=ETrue;
					else
						GoodCount++;
					}
				threadlist[tail].Close();
				tail++;
				if (tail==NOTHREADS)
					tail=0;
				}					
			}
		if ((Math::Random() & 63)==1)
				User::AfterHighRes((Math::Random() & 32));
		ThreadGenCount=MainCount;
		if ((ThreadGenCount>=Maxloops) && !Forever)
			done=ETrue;
		}
	FinishedSoak=ETrue;
	return(KErrNone);
	}


void SoakTest()
	{
	RThread threadGenerator;
	TRequestStatus stat;
	TInt count=0;
	TInt oldcount=0;
	RThread thisThread = RThread();
	TInt ret;
	TMonitors monitor=EStressSecond;

	test.Next(_L("Repeated monitor switching Test"));
	test(Ldd.TaskEventLogging(EFalse,0,EStressSecond)==KErrNone);

	for (count=1; count<100; count++) 
		{
		
		Going=EFalse;
		test(threadGenerator.Create(_L(""),SoakTest_Thread,KDefaultStackSize,NULL,NULL)==KErrNone);
		threadGenerator.Logon(stat);
		test(stat==KRequestPending);	
		threadGenerator.Resume();
		while (!Going)	User::AfterHighRes(Math::Random() & 15);

		monitor= (monitor==EStressFirst)?EStressSecond:EStressFirst;
		test(Ldd.TaskEventLogging(EFalse,0,monitor)==KErrNone);
		
		if (count & 1)
			{
			monitor= (monitor==EStressFirst)?EStressSecond:EStressFirst;
			test(Ldd.TaskEventLogging(EFalse,0,monitor)==KErrNone);
			}
		User::WaitForRequest(stat);
		ret=(TInt) Ldd.GetVEMSData(Ldd.GetNThread(threadGenerator));
		if (ret!=1)
			test.Printf(_L("Thread %x had VEMs value was %x \n"),count,ret);
		threadGenerator.Close();
		test(ret==1);
		}


	// Full soak test
	
	count=1;
	test.Next(_L("Soak test"));
	test(Ldd.TaskEventLogging(EFalse,0,EStressSecond)==KErrNone);

	// Start threadGenerator
	test(threadGenerator.Create(_L("threadGenerator"),SoakTest_ThreadGenerator,KDefaultStackSize,KHeapSize,KHeapSize,NULL)==KErrNone);
	threadGenerator.Logon(stat);
	test(stat==KRequestPending);	
	threadGenerator.Resume();
	
	for (;!FinishedSoak; MainCount=ThreadGenCount+1) 
		{
			switch (Math::Random() & 63) 
			{
			case 1: thisThread.SetPriority(EPriorityLess);
			break;
			case 2: thisThread.SetPriority(EPriorityNormal);
			break;
			case 3: thisThread.SetPriority(EPriorityMore);
			break;
			}

		// Now repeatadly start and stop the start/exit monitors.
		test(Ldd.TaskEventLogging(EFalse,0,EStressFirst)==KErrNone);
		if ((Math::Random() & 63)==1)
				User::AfterHighRes((Math::Random() & 31));
		test(Ldd.TaskEventLogging(EFalse,0,EStressSecond)==KErrNone);
		if (MainCount>>7 != oldcount)
			{	
			oldcount=MainCount>>7;
			if ((count & 15)==1)
				test.Printf(_L("\nCount: "));			
			test.Printf(_L("%x "),count);
	
			count++;
			}
		if ((Math::Random() & 63)==1)
			User::AfterHighRes((Math::Random() & 31));
		}
		
	test.Printf(_L("\nDone.\n: "));	
	User::WaitForRequest(stat);
	threadGenerator.Close();
	if (State!=1)
		{
		test.Printf(_L("Thread %x VEMs value was %x \n"),GoodCount,State);
		test(0);
		}

	test(Ldd.TaskEventLogging(EFalse,0,ENone)==KErrNone);
	}

void ParseCommandLine()
	{
	TBuf<32> args;
	User::CommandLine(args);
	
	if (args == KNullDesC)
		{
		test.Printf(_L("No soaklength specified, using default.\n"));
		return;
		}
	else if (args == _L("-"))
		{
		test.Printf(_L("Infinate soaklength specified.\n"));

		Forever=ETrue;
		}
	else 
		{
		TLex l(args);
		TInt r = l.Val(Maxloops);
		if (r != KErrNone)
			{
			test.Printf(_L("Bad parameter!\nUsage: t_emi <soaklength>\n       '-' indicates infinity.\n\n"));
			User::After(4000000);
			test(0);
			}
		else
			{
			test.Printf(_L("Soaklength set to 0x%x\n"),Maxloops);
			Maxloops = Maxloops << 7;
			}

		}
	}

/* 
 E32Main - Where it all begins.
*/

GLDEF_C TInt E32Main()
	{
	TInt r;

	test.Title();
	ParseCommandLine();

	test.Start(_L("Loading Device"));

	r=User::LoadLogicalDevice(KEMITestLddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);

	test.Next(_L("Open Logical Channel"));
	r=Ldd.Open();
	test(r==KErrNone);
	
	
	test.Next(_L("Calling LDD"));

	if(Ldd.TaskEventLogging(EFalse,0,ENone)==KErrNotSupported)
		{
		test.Printf(_L("********************\n"));
		test.Printf(_L("*  NO EMI SUPPORT  *\n"));
		test.Printf(_L("********************\n"));
		test.Printf(_L("*  Testing SKIPPED *\n"));
		test.Printf(_L("********************\n"));
		User::After(2000000);  //Time to read it!
		test.Next(_L("Close Logical Channel"));
		Ldd.Close();
		test.End();
		return(0);
		}

	// Start Testing!
	
	TestEventLogging();
	TestStartStopMonitors();
	TestScheduleEventLogging();		
	TestTagMaskDFC();
	SoakTest();

	test.Next(_L("Close Logical Channel"));
	Ldd.Close();
	test.End();

	return(0);
	}
