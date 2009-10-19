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
// e32test\active\t_cactw.cpp
// Overview:
// Test the CActiveSchedulerWait class. 
// API Information:
// CActiveSchedulerWait
// Details:
// - Verify the thread is panicked when one of the following programming errors occurs:
// - the scheduler is started twice. 
// - the scheduler is stopped without starting scheduler.
// - the scheduler is started and then stopped twice.
// - the CanStopNow method of scheduler is called without starting scheduler.
// - Run different combinations of wait, start, async stop, async stop with a callback,
// canstopnow and nested calls to start and async stop operations and verify they run
// correctly
// - Check the heap is not corrupted by all the tests.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32panic.h>

const TInt KPanicThreadRet = 222;
const TInt KHeapSize = 0x1000;
const TInt KMaxActions = 32;
const TInt KNumWaits = 2;

enum TDirective {ENormal,EStartTwice,EStopUnstarted,EStopTwice,ECanStopNotStarted};
enum TActionType {EStartWait, EStopWait, EStopWaitCallBack, EIsWaitStarted, ECanStopWait, EStart, EStop};

struct TAction
	{
	TActionType iType;
	TInt iId;
	};

class TSchedulerTester
	{
public:
	void Test1();
	void Test2();
private:
	void SubTest2(TAction* aActions, TInt aCount, const TDesC& aResult);
	};

class CActionRunner : public CActive
	{
public:
	CActionRunner();
	~CActionRunner();
	void SetActions(TAction* aActions, TInt aCount);
	void Start();
	const TDesC& Trace() const;
private:
	void DoCancel();
	void RunL();
	static TInt CallBack(TAny* aThis);
private:
	TInt iStep;
	TInt iNumActions;
	TAction iActions[KMaxActions];
	CActiveSchedulerWait iWait[KNumWaits];
	TBuf<256> iTrace;
	};


LOCAL_D RTest test(_L("T_CACTW"));
LOCAL_D RSemaphore threadSemaphore;


CActionRunner::CActionRunner() : CActive(EPriorityStandard)
	{
	CActiveScheduler::Add(this);
	}

CActionRunner::~CActionRunner()
	{
	Cancel();
	}

void CActionRunner::SetActions(TAction* aActions, TInt aCount)
	{
	iNumActions = aCount;
	for (TInt ii=0; ii<aCount && ii<KMaxActions; ii++)
		iActions[ii] = aActions[ii];
	}

void CActionRunner::Start()
	{
	TRequestStatus* s = &iStatus;
	SetActive();
	User::RequestComplete(s, KErrNone);
	}

void CActionRunner::DoCancel()
	{
	}

void CActionRunner::RunL()
	{
	Start();
	if (iStep < iNumActions)
		{
		TAction& action = iActions[iStep];
		CActiveSchedulerWait& wait = iWait[action.iId];
		iTrace.AppendFormat(_L("%d%dS"), action.iId, action.iType);
		iStep++;
		switch (action.iType)
			{
			case EStartWait:
				wait.Start();
				break;
			case EStopWait:
				wait.AsyncStop();
				break;
			case EStopWaitCallBack:
				wait.AsyncStop(TCallBack(CallBack, this));
				break;
			case EIsWaitStarted:
				iTrace.AppendFormat(_L("%d"), wait.IsStarted()!=0);
				break;
			case ECanStopWait:
				iTrace.AppendFormat(_L("%d"), wait.CanStopNow()!=0);
				break;
			case EStart:
				CActiveScheduler::Start();
				break;
			case EStop:
				CActiveScheduler::Stop();
				break;
			default:
				break;
			}
		iTrace.AppendFormat(_L("%d%dE"), action.iId, action.iType);
		}
	}

const TDesC& CActionRunner::Trace() const
	{
	return iTrace;
	}

TInt CActionRunner::CallBack(TAny* aThis)
	{
	CActionRunner* self = (CActionRunner*) aThis;
	self->iTrace.Append(_L("C"));
	return 0;
	}


LOCAL_D TInt panicThread(TAny* aDirective)
//
// Test thread which panics
//
	{
	// cause panics in various ways depending upon aDirective
	CActiveScheduler* pManager=new CActiveScheduler;
	CActiveScheduler::Install(pManager);

	CActionRunner* r = new(ELeave)CActionRunner;
	
	switch((TInt)aDirective)
		{
		case ENormal:
			{
			TAction actions[] = {{EStop, 0}};
			r->SetActions(actions, sizeof(actions)/sizeof(*actions));
			break;
			}
		case EStartTwice:
			{
			TAction actions[] = {{EStartWait, 0}, {EStartWait, 0}, {EStop, 0}};
			r->SetActions(actions, sizeof(actions)/sizeof(*actions));
			break;
			}
		case EStopUnstarted:
			{
			TAction actions[] = {{EStopWait, 0}, {EStop, 0}};
			r->SetActions(actions, sizeof(actions)/sizeof(*actions));
			break;
			}
		case EStopTwice:
			{
			TAction actions[] = {{EStartWait, 0}, {EStopWait, 0}, {EStopWait, 0}, {EStop, 0}};
			r->SetActions(actions, sizeof(actions)/sizeof(*actions));
			break;
			}
		case ECanStopNotStarted:
			{
			TAction actions[] = {{ECanStopWait, 0}, {EStop, 0}};
			r->SetActions(actions, sizeof(actions)/sizeof(*actions));
			break;
			}
		default:
			break;
		}

	r->Start();
	CActiveScheduler::Start();
	delete r;
	delete pManager;
	return(KPanicThreadRet);
	}


void TSchedulerTester::Test1()
//
// Test 1
//
	{

//
// Test the panics
//

	RThread thread;
	TRequestStatus stat;
//
	test.Start(_L("First test normal thread termination"));
	TInt r=thread.Create(_L("myThread"),panicThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ENormal);
	test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitCategory().Compare(_L("Kill"))==0);
	test(thread.ExitReason()==KPanicThreadRet);	  
	test(thread.ExitType()==EExitKill);
	CLOSE_AND_WAIT(thread);
//
	test.Next(_L("Two starts panics"));
	r=thread.Create(_L("myThread"),panicThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)EStartTwice);
	test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitReason()==EActiveSchedulerWaitAlreadyStarted);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
//
	test.Next(_L("Stop without start panics"));
	r=thread.Create(_L("myThread"),panicThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)EStopUnstarted);
	test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitReason()==EActiveSchedulerWaitNotStarted);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
//
	test.Next(_L("Start then two stops panics"));
	r=thread.Create(_L("myThread"),panicThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)EStopTwice);
	test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitReason()==EActiveSchedulerWaitNotStarted);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
//
	test.Next(_L("Can stop now, without start panics"));
	r=thread.Create(_L("myThread"),panicThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ECanStopNotStarted);
	test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitReason()==EActiveSchedulerWaitNotStarted);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);

	test.End();
	}

void TSchedulerTester::Test2()
//
// Test 2
//
	{
//
// Test sequencing
	test.Start(_L("Scheduler wait sequencing"));
		{
		test.Next(_L("First a simple stop"));
		TAction a[] = {{EStop, 0}};
		SubTest2(a, sizeof(a)/sizeof(*a), _L("06S06E"));
		}
		{
		test.Next(_L("Simple wait stop"));
		TAction a[] = { {EIsWaitStarted, 0}, {EStartWait, 0}, {EIsWaitStarted, 0}, 
						{ECanStopWait, 0}, {EStopWait, 0}, {EIsWaitStarted, 0}, 
						{EStop, 0}};
		SubTest2(a, sizeof(a)/sizeof(*a), _L("03S003E00S03S103E04S104E01S01E00E03S003E06S06E"));
		}
		{
		test.Next(_L("Properly nested wait"));
		TAction a[] = { {EStartWait, 0}, {EStartWait, 1}, 
						{EStopWait, 1}, {EStopWait, 0}, {EStop, 0}};
		SubTest2(a, sizeof(a)/sizeof(*a), _L("00S10S11S11E10E01S01E00E06S06E"));
		}
		{
		test.Next(_L("Properly nested scheduler"));
		TAction a[] = { {EStart, 0}, {EStart, 1}, 
						{EStop, 1}, {EStop, 0}, {EStop, 0}};
		SubTest2(a, sizeof(a)/sizeof(*a), _L("05S15S16S16E15E06S06E05E06S06E"));
		}
		{
		test.Next(_L("Badly nested wait"));
		TAction a[] = { {EStartWait, 0}, {EStartWait, 1}, 
						{EStopWait, 0}, {EStopWait, 1}, {EStop, 0}};
		SubTest2(a, sizeof(a)/sizeof(*a), _L("00S10S01S01E11S11E10E00E06S06E"));
		}
		{
		test.Next(_L("Badly nested scheduler"));
		TAction a[] = { {EStart, 0}, {EStart, 1}, 
						{EStop, 0}, {EStop, 1}, {EStop, 0}};
		SubTest2(a, sizeof(a)/sizeof(*a), _L("05S15S06S06E15E16S16E05E06S06E"));
		}
		{
		test.Next(_L("Bad mixed nesting 1"));
		TAction a[] = { {EStartWait, 0}, {EStart, 1}, 
						{EStopWait, 0}, {EStop, 1}, {EStop, 0}};
		SubTest2(a, sizeof(a)/sizeof(*a), _L("00S15S01S01E16S16E15E00E06S06E"));
		}
		{
		test.Next(_L("Bad mixed nesting 2"));
		TAction a[] = { {EStart, 0}, {EStartWait, 1}, 
						{EStop, 0}, {EStopWait, 1}, {EStop, 0}};
		SubTest2(a, sizeof(a)/sizeof(*a), _L("05S10S06S06E11S11E10E05E06S06E"));
		}
		{
		test.Next(_L("Properly nested wait callback 1"));
		TAction a[] = { {EStartWait, 0}, {EStartWait, 1}, 
						{EStopWaitCallBack, 1}, {EStopWait, 0}, {EStop, 0}};
		SubTest2(a, sizeof(a)/sizeof(*a), _L("00S10S12S12EC10E01S01E00E06S06E"));
		}
		{
		test.Next(_L("Properly nested wait callback 2"));
		TAction a[] = { {EStartWait, 0}, {EStartWait, 1}, 
						{EStopWait, 1}, {EStopWaitCallBack, 0}, {EStop, 0}};
		SubTest2(a, sizeof(a)/sizeof(*a), _L("00S10S11S11E10E02S02EC00E06S06E"));
		}
		{
		test.Next(_L("Badly nested wait callback 1"));
		TAction a[] = { {EStartWait, 0}, {EStartWait, 1}, 
						{EStopWaitCallBack, 0}, {EStopWait, 1}, {EStop, 0}};
		SubTest2(a, sizeof(a)/sizeof(*a), _L("00S10S02S02E11S11E10EC00E06S06E"));
		}
		{
		test.Next(_L("Badly nested wait callback 2"));
		TAction a[] = { {EStartWait, 0}, {EStartWait, 1}, 
						{EStopWait, 0}, {EStopWaitCallBack, 1}, {EStop, 0}};
		SubTest2(a, sizeof(a)/sizeof(*a), _L("00S10S01S01E12S12EC10E00E06S06E"));
		}
		{
		test.Next(_L("Properly nested wait can stop now"));
		TAction a[] = { {EStartWait, 0}, {EStartWait, 1}, {ECanStopWait, 0}, {ECanStopWait, 1}, 
						{EStopWait, 1}, {ECanStopWait, 0}, {EStopWait, 0}, {EStop, 0}};
		SubTest2(a, sizeof(a)/sizeof(*a), _L("00S10S04S004E14S114E11S11E10E04S104E01S01E00E06S06E"));
		}
		{
		test.Next(_L("Badly nested wait can stop now"));
		TAction a[] = { {EStartWait, 0}, {EStartWait, 1}, {ECanStopWait, 0}, {ECanStopWait, 1}, 
						{EStopWait, 0}, {ECanStopWait, 1}, {EStopWait, 1}, {EStop, 0}};
		SubTest2(a, sizeof(a)/sizeof(*a), _L("00S10S04S004E14S114E01S01E14S114E11S11E10E00E06S06E"));
		}
		{
		test.Next(_L("Properly nested wait is started"));
		TAction a[] = { {EStartWait, 0}, {EStartWait, 1}, {EIsWaitStarted, 0}, {EIsWaitStarted, 1}, 
						{EStopWait, 1}, {EIsWaitStarted, 0}, {EIsWaitStarted, 1}, {EStopWait, 0}, {EStop, 0}};
		SubTest2(a, sizeof(a)/sizeof(*a), _L("00S10S03S103E13S113E11S11E10E03S103E13S013E01S01E00E06S06E"));
		}
		{
		test.Next(_L("Badly nested wait is started"));
		TAction a[] = { {EStartWait, 0}, {EStartWait, 1}, {EIsWaitStarted, 0}, {EIsWaitStarted, 1}, 
						{EStopWait, 0}, {EIsWaitStarted, 0}, {EIsWaitStarted, 1}, {EStopWait, 1}, {EStop, 0}};
		SubTest2(a, sizeof(a)/sizeof(*a), _L("00S10S03S103E13S113E01S01E03S003E13S113E11S11E10E00E06S06E"));
		}
		{
		test.Next(_L("Interleaved badly nested wait with callback"));
		TAction a[] = { {EStartWait, 0}, {EStartWait, 1}, {EStopWaitCallBack, 0}, 
						{EStartWait, 0}, {EStopWaitCallBack, 1}, {EStopWaitCallBack, 0}, 
						{EStop, 0}};
		SubTest2(a, sizeof(a)/sizeof(*a), _L("00S10S02S02E00S12S12E02S02EC00EC10EC00E06S06E"));
		}
	test.End();
	}

void TSchedulerTester::SubTest2(TAction* aActions, TInt aCount, const TDesC& aResult)
	{
	CActiveScheduler* pManager=new CActiveScheduler;
	CActiveScheduler::Install(pManager);

	CActionRunner* r = new(ELeave)CActionRunner;
	r->SetActions(aActions, aCount);
	r->Start();
	CActiveScheduler::Start();
	test(r->Trace() == aResult);

	delete r;
	delete pManager;
	}

GLDEF_C TInt E32Main()
    {

	// don't want just in time debugging as we trap panics
	TBool justInTime=User::JustInTime(); 
	User::SetJustInTime(EFalse); 

	test.Title();
	__UHEAP_MARK;
//
	test.Start(_L("Test1"));
	TSchedulerTester sched;
	sched.Test1();
//
	test.Next(_L("Test2"));
	sched.Test2();
//
	test.End();
	__UHEAP_MARKEND;

	User::SetJustInTime(justInTime);
	return(KErrNone);
    }
