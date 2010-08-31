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
// e32test\active\t_ctimer.cpp
// Overview:
// Test timer functionality.
// API Information:
// CTimer 
// Details:
// - Create some timer active objects, install an active scheduler, and the
// active objects, request event after an interval and verify that:
// - Absolute timer set with time less than current time return KErrUnderflow.
// - Relative timer set with 0 goes off immediately.
// - Absolute timer set to a time of now returns KErrUnderflow.
// - Absolute timer set with time more than the current time returns KErrNone.
// - Repeated timer is cancelled as expected.
// - Call relative timer's After function without adding it to the active scheduler 
// and check for panic.
// - Call absolute timer's At function without adding it to the active scheduler and 
// check for panic.
// - Call 1s inactivity timer
// - Check if heap has been corrupted by the tests.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32panic.h>

enum TDirective {ERelInvalidTime,ERelNotAdded,EAbNotAdded,EHighResNotAdded};

LOCAL_D RTest test(_L("T_CTIMER"));
LOCAL_D const TInt KRepeatID=999; 	
LOCAL_D const TInt ID1=101;
LOCAL_D const TInt ID2=102;
LOCAL_D const TInt ID3=103;
LOCAL_D TInt A[5]; //When a timer expires its identifier is placed in here

class TestCTimer
	{
public:
	void Test1();
	void Test2();
	void Test3();
	};

class myScheduler : public CActiveScheduler
	{
public:
	virtual void Error(TInt anError)const{User::Leave(anError);}
	};

class myTimer : public CTimer
	{
public:
	myTimer(const TInt aPriority, const TInt anId):CTimer(aPriority){iIdentifier=anId; iCount=0;}
	void RunL(void);	
	void Start(void);
	static void SetNum(const TInt aNum) {iNum=aNum; iTotalCount=0;}
private:
	TInt iIdentifier;
	TInt iCount;
	static TInt iNum;
	static TInt iTotalCount;
	};

// for inactivity test
class myInactTimer : public CTimer
	{
public:
	myInactTimer(const TInt aPriority):CTimer(aPriority){;}
	void RunL(void);	
	void Start(void);
	};

TInt myTimer::iTotalCount;
TInt myTimer::iNum;

void myTimer::RunL(void)
//
// Timer has completed
//
	{

	A[iTotalCount++]=iIdentifier;
	if (iIdentifier==KRepeatID)
		{
		if(++iCount>2)
			Cancel();
		else
			After(100000);
		}
	if (iTotalCount>=iNum)
		CActiveScheduler::Stop();
	}

void myTimer::Start(void)
//
// Start a timer going.
//
	{

	ConstructL();
	CActiveScheduler::Add(this);
	}

void myInactTimer::RunL(void)
//
// Timer has completed
//
	{
	CActiveScheduler::Stop();
	}

void myInactTimer::Start(void)
//
// Start a timer going.
//
	{
	ConstructL();
	CActiveScheduler::Add(this);
	}

LOCAL_D TInt ThreadEntry(TAny* aDirective)
//
// Test thread
//
	{

	myTimer* pTimer=new myTimer(0,ID1);
	TTime time;
	switch((TUint)aDirective)
		{
	case ERelInvalidTime: // Setting a relative timer with a negative time panics 
		CActiveScheduler::Install(new myScheduler);
		pTimer->Start();
		pTimer->After(-100000); 
		break;	
	case ERelNotAdded: 	// Requesting an un-added relative timer panics
		pTimer->After(100000);
		break; 	
	case EAbNotAdded: // Requesting an un-added absolute timer panics
		time.HomeTime(); 
		pTimer->At(time); 
		break;
	case EHighResNotAdded: // Requesting an un-added HighRes timer panics
		pTimer->HighRes(100000);
		break;
		}
	return(KErrNone);
	}

void TestCTimer::Test1()
//
// Test timer active objects.
//
	{

	test.Start(_L("Create objects"));
	myTimer* pTimer1=new myTimer(0, ID1);
	myTimer* pTimer2=new myTimer(0, ID2);
	myTimer* pTimer3=new myTimer(0, ID3);
	TTime time;
	pTimer1->Start();
	pTimer2->Start();
	pTimer3->Start();
//
	test.Next(_L("Abs timer with time less than now set KErrUnderflow"));
	myTimer::SetNum(1);
	time.HomeTime();
	pTimer1->At(time+TTimeIntervalSeconds(-1));
	CActiveScheduler::Start();
	test(pTimer1->iStatus==KErrUnderflow);
//
	test.Next(_L("Rel timer with 0 goes off immediately"));
	myTimer::SetNum(2);
	pTimer1->After(0);
	pTimer2->After(1000000);
	CActiveScheduler::Start();
	test(A[0]==ID1 && A[1]==ID2);
//
	test.Next(_L("Abs timer to a time of now sets KErrUnderflow"));
	myTimer::SetNum(1);
	time.UniversalTime();
	pTimer1->AtUTC(time);
	CActiveScheduler::Start();
	test(pTimer1->iStatus==KErrUnderflow);
//
	test.Next(_L("Abs timer set to future"));
	myTimer::SetNum(2);
	pTimer1->After(2000000);
	time.HomeTime();
	pTimer2->At(time+TTimeIntervalSeconds(1));
	CActiveScheduler::Start();
	test(A[0]==ID2 && A[1]==ID1&& pTimer1->iStatus==KErrNone && pTimer2->iStatus==KErrNone);
//  
  	test.Next(_L("Cancel a repeating timer after 3"));
	myTimer::SetNum(4);
	myTimer* pRepeater=new myTimer(0,KRepeatID);
	pRepeater->Start();
	pTimer1->After(1000000);
	pRepeater->After(100000);
	CActiveScheduler::Start();
	test(A[0]==KRepeatID && A[1]==KRepeatID && A[2]==KRepeatID && A[3]==ID1);

//
	test.Next(_L("HighRes timer"));
	myTimer::SetNum(1);
	pTimer1->HighRes(1000000);
	CActiveScheduler::Start();
	test(A[0]==ID1 && pTimer1->iStatus==KErrNone);
//
	test.Next(_L("Inactivity 1s"));
	User::ResetInactivityTime();
	myInactTimer* pInactTimer=new myInactTimer(0);
	pInactTimer->Start();
	test.Printf(_L("inactivity..."));
	pInactTimer->Inactivity(1);
	CActiveScheduler::Start();
	test.Printf(_L("...back"));
	test(pInactTimer->iStatus==KErrNone);
//
	test.Next(_L("Destroy objects"));
	delete pTimer1;
	delete pTimer2;
	delete pTimer3;
	delete pRepeater;
	delete pInactTimer;
//
	test.End();
	}

void TestCTimer::Test2()
//
// Test the panics
//
	{

	RThread thread;
	TRequestStatus stat;
//
// Calling At or After or HighRes of an object not added to the queue panics
//
	test.Start(_L("Queue rel when not added"));
	test(thread.Create(_L("myThread"),ThreadEntry,KDefaultStackSize,0x2000,0x2000,(TAny*)ERelNotAdded)==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitCategory().Compare(_L("E32USER-CBase"))==0);
	test(thread.ExitReason()==ETimNotAdded);				
	test(thread.ExitType()==EExitPanic);				   
	CLOSE_AND_WAIT(thread);
//
	test.Next(_L("Queue abs when not added"));
	test(thread.Create(_L("myThread"),ThreadEntry,KDefaultStackSize,0x2000,0x2000,(TAny*)EAbNotAdded)==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitCategory().Compare(_L("E32USER-CBase"))==0);
	test(thread.ExitReason()==ETimNotAdded);				
	test(thread.ExitType()==EExitPanic);				   	
	CLOSE_AND_WAIT(thread);
//
	test.Next(_L("Queue HighRes when not added"));
	test(thread.Create(_L("myThread"),ThreadEntry,KDefaultStackSize,0x2000,0x2000,(TAny*)EHighResNotAdded)==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitCategory().Compare(_L("E32USER-CBase"))==0);
	test(thread.ExitReason()==ETimNotAdded);				
	test(thread.ExitType()==EExitPanic);				   	
	CLOSE_AND_WAIT(thread);
//
	test.End();
	}

LOCAL_D TInt Test3ThreadEntry(TAny* aArg)
//
// Test thread for RTimer::HighRes
//
	{
	TRequestStatus status;
	RTimer timer;
	TInt r = timer.CreateLocal();
	if (r != KErrNone)
		return r;
	timer.HighRes(status, (TInt) aArg);
	timer.Cancel();
	User::WaitForRequest(status);
	timer.Close();
	if (status != KErrNone && status != KErrCancel)
		return status.Int();	
	return KErrNone;
	}

void TestRTimerHighRes(TInt aInterval, TExitType aExpectedExitType)
	{
	RThread thread;
	TRequestStatus stat;
	test(thread.Create(_L("myThread"),Test3ThreadEntry,KDefaultStackSize,0x2000,0x2000,(TAny*)aInterval)==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitType()==aExpectedExitType);
	if (thread.ExitType()==EExitKill)
		test(thread.ExitReason()==KErrNone);
	CLOSE_AND_WAIT(thread);
	}

void TestCTimer::Test3()
//
// Test RTimer::HighRes argument checking
//
	{
	test.Start(_L("Test RTimer::HighRes argument checking"));
	TestRTimerHighRes(0, EExitKill);
	TestRTimerHighRes(0x7FFFEC79, EExitKill);
	TestRTimerHighRes(0x7FFFFFFF, EExitKill);
	TestRTimerHighRes(0x80000000, EExitPanic);
	test.End();
	}

GLDEF_C TInt E32Main()
//
// Test the CTimer class
//
    {
	// don't want just in time debugging as we trap panics
	TBool justInTime=User::JustInTime(); 
	User::SetJustInTime(EFalse); 

	test.Title();
	__UHEAP_MARK;
	TestCTimer T;
	myScheduler* pScheduler=new myScheduler;
	CActiveScheduler::Install(pScheduler);
	test.Start(_L("Test1"));
	T.Test1();
	test.Next(_L("Test2"));
	T.Test2();
	CActiveScheduler::Install(NULL);
	delete pScheduler;
	T.Test3();
	test.End();
	__UHEAP_MARKEND;

	User::SetJustInTime(justInTime);
	return(KErrNone);
    }

