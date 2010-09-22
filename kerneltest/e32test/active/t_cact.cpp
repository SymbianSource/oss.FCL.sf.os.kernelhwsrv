// Copyright (c) 1995-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\active\t_cact.cpp
// Overview:
// Test the CActiveScheduler class, including scheduling priority, thread 
// panics and cancelling objects attached to the scheduler. 
// API Information:
// CActiveScheduler 
// Details:
// - Verify the thread is panicked when one of the following programming errors occurs:
// - start an uninstalled scheduler
// - install a scheduler twice
// - stop the scheduler twice 
// - stop an uninstalled scheduler
// - set active an active object twice
// - add an active object to an uninstalled scheduler
// - add a NULL pointer to the scheduler
// - add an active object twice
// - thread gets a stray signal
// - make request on an un-added active object
// - Create timers with different priorities
// - Check the timers are scheduled according to expiration time
// - Check the timers are scheduled in order of their priority when they expire at the
// same time
// - Check the timers are scheduled in order of the addition to the scheduler when
// they expire at the same time and they have the same priority
// - Verify that a leave in RunL method not handled by the active object calls the active 
// scheduler's Error method
// - Test a cancelled timer will not be scheduled 
// - Create a thread with its own heap and test that timers are scheduled in the order of
// their expiration time, priority and order of addition to the scheduler
// - Check the heap is not corrupted by all the tests
// - Test replacing the scheduler and also check stack depths
// - Call deprecated dummy functions
// Platforms/Drives/Compatibility:
// All
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32panic.h>

const TInt KmyErrorNum=999;
const TInt KLeaveCode=1234;  
const TInt KgTimerID=4321;
const TInt KCancelCode=1111;
const TInt KTestArraySize=10;
const TInt KPanicThreadRet=2222;
const TInt KHeapSize=0x2000;

enum TActivePriority {eLowest=-100,eLow=-1,eNone=0,eHigh=1,eHighest=100};
enum TDirective {EStartUninstalled,EInstallTwice,EStopTwice,EStopUninstalled,ESetActiveTwice,
				 EAddToUninstalled,EAddNull,EAddTwice,ECreateStray,ERequestUnadded,EStraySignalNoKRequestPending,EStraySignalNoSetActive,ENormal};

class TSecdulerTester
	{
public:
	void Test1();
	void Test2();
	void Test3();
	void Test4();
	void Test5();
	};

class MyManager : public CActiveScheduler
	{
public:
	virtual void Error(TInt anError) const {CActiveScheduler::Halt(anError);}
	};

class myTimer : public CTimer
	{
public:
	myTimer(const TInt aPriority, const TInt anIdentifier):CTimer(aPriority){iIdentifier=anIdentifier;}
	virtual void RunL();
	void Start();		  
	void Setactive() {SetActive();}
	static void SetNum(const TInt aNum) {iNum=aNum; iCount=0;}
private:   
	TInt iIdentifier;
	static TInt iCount;
	static TInt iNum;
	};

class myThreadTimer : public CTimer
	{
public:
	myThreadTimer(const TInt aPriority, const TInt anIdentifier):CTimer(aPriority){iIdentifier=anIdentifier;}
	virtual void RunL();
	static void SetNum(const TInt aNum) {iNum=aNum; iCount=0;}
	void Start();
private:
	TInt iIdentifier;
	static TInt iCount;
	static TInt iNum;
	};

class CreateStray : public CActive
	{
public:
	CreateStray(TInt aPriority);
	void DoCancel() {}
	void RunL() {}
private:
	RTimer iTimer;
	};

class myStray : public CActive
	{
public:
	myStray(TInt aPriority) : CActive(aPriority) {};
	void DoCancel() {}
	void RunL() { CActiveScheduler::Stop(); }
	void Start1();
	void Start2();
	};

void myStray::Start1()
	{
	TRequestStatus* s=&iStatus;
	SetActive();
	RThread().RequestComplete(s,KErrNone);
	}

void myStray::Start2()
	{
	TRequestStatus* s=&iStatus;
	iStatus=KRequestPending;
	User::RequestComplete(s,KErrNone);
	}

TInt myTimer::iCount;
TInt myTimer::iNum;
TInt myThreadTimer::iCount;
TInt myThreadTimer::iNum;

LOCAL_D RTest test(_L("T_CACT"));
LOCAL_D myTimer* gTimer; // This is cancelled from within the run method 
LOCAL_D RSemaphore threadSemaphore;
LOCAL_D myTimer* pLowest=NULL;
LOCAL_D myTimer* pLow=NULL;
LOCAL_D myTimer* pNone=NULL;
LOCAL_D myTimer* pHigh=NULL;
LOCAL_D myTimer* pHigh2=NULL;
LOCAL_D myTimer* pHigh3=NULL;
LOCAL_D myTimer* pHighest=NULL;
LOCAL_D TInt order[KTestArraySize]; // When a timer expires its identifier is placed in here
LOCAL_D TInt threadArray[KTestArraySize];

LOCAL_C TInt myThreadEntryPoint(TAny*)
//
// myThread tests 
//
	{

	__UHEAP_MARK;
	RTest test(_L("Separate thread tests"));
	test.Title();
	test.Start(_L("Create semaphore"));
	RTimer t;
	TInt ret=t.CreateLocal();
	test(ret==KErrNone);
//
	test.Next(_L("Install scheduler"));
	MyManager* pManager=new MyManager;
	CActiveScheduler::Install(pManager);
//
	test.Next(_L("Create active objects"));
	myThreadTimer* pLowest=new myThreadTimer(eLowest, eLowest);
	test(pLowest!=NULL);
	myThreadTimer* pLow=new myThreadTimer(eLow, eLow);
	test(pLow!=NULL);
	myThreadTimer* pNone=new myThreadTimer(eNone, eNone);
	test(pNone!=NULL);
	myThreadTimer* pHigh=new myThreadTimer(eHigh, eHigh);
	test(pHigh!=NULL);
	myThreadTimer* pHigh2=new myThreadTimer(eHigh, eHigh+2);
	test(pHigh2!=NULL);
	myThreadTimer* pHigh3=new myThreadTimer(eHigh, eHigh+3);
	test(pHigh3!=NULL);
	myThreadTimer* pHighest=new myThreadTimer(eHighest, eHighest);
	test(pHighest!=NULL);
//
	test.Next(_L("Test low is scheduled before lowest"));
	myThreadTimer::SetNum(2);
	pLowest->Start();
	pLowest->After(0);
	pLow->Start();
	pLow->After(0);
	TRequestStatus s;
	t.After(s,100000);
	test(s==KRequestPending);
	User::WaitForRequest(s);
	CActiveScheduler::Start();
	test(threadArray[0]==eLow && threadArray[1]==eLowest);
//
	test.Next(_L("Test none is scheduled before low"));
	myThreadTimer::SetNum(2);
	pLow->After(0);
	pNone->Start();
	pNone->After(0);
	t.After(s,100000);
	test(s==KRequestPending);
	User::WaitForRequest(s);
	CActiveScheduler::Start();
	test(threadArray[0]==eNone && threadArray[1]==eLow);
//
	test.Next(_L("Test high is scheduled before none"));
	myThreadTimer::SetNum(2);
	pHigh->Start();
	pHigh->After(0);
	pNone->After(0);
	t.After(s, 100000);
	test(s==KRequestPending);
	User::WaitForRequest(s);
	CActiveScheduler::Start();
	test(threadArray[0]==eHigh && threadArray[1]==eNone);
//
	test.Next(_L("Test highest is scheduled before high"));
	myThreadTimer::SetNum(2);
	pHighest->Start();
	pHighest->After(0);
	pHigh->After(0);
	t.After(s, 100000);
	test(s==KRequestPending);
	User::WaitForRequest(s);
	CActiveScheduler::Start();
	test(threadArray[0]==eHighest && threadArray[1]==eHigh);
//
	test.Next(_L("Test objects are scheduled according to priority"));
	myThreadTimer::SetNum(5);
	pLowest->After(0);
	pLow->After(0);
	pNone->After(0);
	pHigh->After(0);
	pHighest->After(0);
	t.After(s, 100000);
	test(s==KRequestPending);
	User::WaitForRequest(s);
	CActiveScheduler::Start();
	test(threadArray[0]==eHighest && threadArray[1]==eHigh && threadArray[2]==eNone && 
		 threadArray[3]==eLow && threadArray[4]==eLowest);
//
	test.Next(_L("Test objects are scheduled according to timer expiry"));
	myThreadTimer::SetNum(5);
	pLowest->After(0);
	pLow->After(500000);
	pNone->After(1000000);
	pHigh->After(1500000);
	pHighest->After(2000000);
	CActiveScheduler::Start();
	test(threadArray[4]==eHighest && threadArray[3]==eHigh && threadArray[2]==eNone && 
		 threadArray[1]==eLow && threadArray[0]==eLowest);
//
	test.Next(_L("Test with some objects having the same priority"));
	myThreadTimer::SetNum(7);
	pLowest->After(0);
	pLow->After(0);
	pNone->After(0);
	pHigh->After(0);
	pHigh2->Start();
	pHigh2->After(0);
	pHigh3->Start();
	pHigh3->After(0);
	pHighest->After(0);
	t.After(s, 100000);
	test(s==KRequestPending);
	User::WaitForRequest(s);
	CActiveScheduler::Start();
	test(threadArray[0]==eHighest && threadArray[1]==eHigh && threadArray[2]==eHigh+2 && 
		 threadArray[3]==eHigh+3 && threadArray[4]==eNone && threadArray[5]==eLow && threadArray[6]==eLowest);
//
	test.Next(_L("Tidying up"));
	delete pManager;
	delete pLowest;
	delete pLow;
	delete pNone;
	delete pHigh;
	delete pHigh2;
	delete pHigh3;
	delete pHighest;
	test.Close();
	__UHEAP_MARKEND;
	threadSemaphore.Signal();
	return(KErrNone);
	}

LOCAL_D TInt panicThread(TAny* aDirective)
//
// Test thread which panics
//
	{
	// cause panics in various ways depending upon aDirective
	MyManager* pManager=new MyManager;
	
	switch((TInt)aDirective)
		{
	case EStartUninstalled:
		CActiveScheduler::Start(); // Start an uninstalled active schedler
		break;
	case EInstallTwice:
		CActiveScheduler::Install(pManager); // Install the scheduler twice
		CActiveScheduler::Install(pManager);
		break;
	case EStopTwice:
		CActiveScheduler::Install(pManager); // Stop a scheduler twice
		CActiveScheduler::Stop();
		CActiveScheduler::Stop();
		break;
	case EStopUninstalled:
		CActiveScheduler::Stop(); // Stop an uninstalled active scheduler
		break;
	case ESetActiveTwice:
		{
//
// Set an active object to active twice
//
		myTimer* pTimer=new myTimer(eNone,eNone);
		CActiveScheduler::Install(pManager);
		CActiveScheduler::Add(pTimer);
		pTimer->Setactive();
		pTimer->Setactive();
		}
		break;
	case EAddToUninstalled:
		{
//
// Add an active object to an uninstalled scheduler
//
		myTimer* pTimer=new myTimer(eNone,eNone);
		CActiveScheduler::Add(pTimer); 
		}
		break;
	case EAddNull:
//
// Add Null to the scheduling queue
//
		CActiveScheduler::Install(pManager);  
		CActiveScheduler::Add(NULL);
		break;
	case EAddTwice:
		{
//
// Add the same object twice to the scheduling queue
//
		myTimer* pTimer=new myTimer(eNone,eNone);
		CActiveScheduler::Install(pManager);  
		CActiveScheduler::Add(pTimer);
		CActiveScheduler::Add(pTimer);
		}
		break;
	case ECreateStray:
		{
//
// Create a stray signal
//
		CActiveScheduler::Install(pManager); 
		new CreateStray(1);
		CActiveScheduler::Start();
		}
		break;
	case ERequestUnadded:
		{
//
// Make a request of an active object not added to the scheduling queue
//
		CActiveScheduler::Install(pManager);
		myTimer* pTimer=new myTimer(eNone,eNone);
		pTimer->Setactive();
		}
		break;
	case EStraySignalNoKRequestPending:
		{
		CActiveScheduler::Install(pManager);
		myStray* pStray = new myStray(eNone);
		CActiveScheduler::Add(pStray);
		pStray->Start1();
		CActiveScheduler::Start();
		}
		break;
	case EStraySignalNoSetActive:
		{
		CActiveScheduler::Install(pManager);
		myStray* pStray = new myStray(eNone);
		CActiveScheduler::Add(pStray);
		pStray->Start2();
		CActiveScheduler::Start();
		}
		break;
	case ENormal:
	default:
		break;
		}
	delete pManager;
	return(KPanicThreadRet);
	}

void myTimer::RunL()
//
// Handle the timer completion
//
	{

	if (iIdentifier==KLeaveCode)	// Used to test that the request manager error() method is called
		User::Leave(KmyErrorNum);
	if (iIdentifier==KCancelCode) // Used to test cancelling an object
		gTimer->Cancel();
	order[iCount++]=iIdentifier;
	if (iCount>=iNum)
		CActiveScheduler::Stop();
	}

void myTimer::Start()
//
// Start a timer
//
	{

	ConstructL();
	CActiveScheduler::Add(this);
	}

void myThreadTimer::RunL()
//
// Handle timer completion
//
	{
	
	threadArray[iCount++]=iIdentifier;
	if(iCount>=iNum)
		CActiveScheduler::Stop();
	}

void myThreadTimer::Start()
//
// Start a timer
//
	{

	ConstructL();
	CActiveScheduler::Add(this);
	}

CreateStray::CreateStray(TInt aPriority)
//
// Constructor
//
	: CActive(aPriority)
	{

	iTimer.CreateLocal();
	CActiveScheduler::Add(this);
	iTimer.After(iStatus, 1000000);
	}

void TSecdulerTester::Test1()
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
	test.Next(_L("Starting an uninstalled scheduler panics"));
	r=thread.Create(_L("myThread"),panicThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)EStartUninstalled);
	test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitReason()==EReqManagerDoesNotExist);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
//
	test.Next(_L("Installing the scheduler twice panics"));
	r=thread.Create(_L("myThread"),panicThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)EInstallTwice);
	test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitReason()==EReqManagerAlreadyExists);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
//
	test.Next(_L("Stopping the scheduler twice panics"));
	r=thread.Create(_L("myThread"),panicThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)EStopTwice);
	test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitReason()==EReqTooManyStops);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
//
	test.Next(_L("Stopping an uninstalled scheduler panics"));
	r=thread.Create(_L("myThread"),panicThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)EStopUninstalled);
	test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitReason()==EReqManagerDoesNotExist);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
//
	test.Next(_L("Setting an active object to active twice panics"));
	r=thread.Create(_L("myThread"),panicThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ESetActiveTwice);
	test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitReason()==EReqAlreadyActive);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
//
	test.Next(_L("Adding an active object to an uninstalled scheduler panics"));
	r=thread.Create(_L("myThread"),panicThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)EAddToUninstalled);
	test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitReason()==EReqManagerDoesNotExist);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
//
	test.Next(_L("Adding NULL panics"));
	r=thread.Create(_L("myThread"),panicThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)EAddNull);
	test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitReason()==EReqNull);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
//
	test.Next(_L("Adding an active object twice panics"));
	r=thread.Create(_L("myThread"),panicThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)EAddTwice);
	test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitReason()==EReqAlreadyAdded);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
//
	test.Next(_L("A stray signal causes a panic"));
	r=thread.Create(_L("myThread"),panicThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ECreateStray);
	test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitReason()==EReqStrayEvent);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
//
	test.Next(_L("Making a request on an unadded active object panics"));
	r=thread.Create(_L("myThread"),panicThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ERequestUnadded);
	test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitReason()==EActiveNotAdded);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
//
	test.Next(_L("The service provider does not set the status to KRequestPending"));
	r=thread.Create(_L("myThread"),panicThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)EStraySignalNoKRequestPending);
	test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
#ifdef _DEBUG
	//this might fail if we're using a UREL euser
	test(thread.ExitReason()==EReqStrayEvent);
	test(thread.ExitType()==EExitPanic);
#else
	test(thread.ExitCategory().Compare(_L("Kill"))==0);
	test(thread.ExitReason()==KPanicThreadRet);	  
	test(thread.ExitType()==EExitKill);
#endif 
	CLOSE_AND_WAIT(thread);
//
#ifdef _DEBUG
	//this might fail if we're using a UREL euser
	test.Next(_L("The active object does not call SetActive"));
	r=thread.Create(_L("myThread"),panicThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)EStraySignalNoSetActive);
	test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitReason()==EReqStrayEvent);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
#endif
//
	test.End();
	}

void TSecdulerTester::Test2()
//
// Test 2
//
	{

	test.Start(_L("Create timer"));
	RTimer t;
	TInt ret=t.CreateLocal();
	test(ret==KErrNone);
	TRequestStatus s;
//
	test.Next(_L("Test low is scheduled before lowest when timing equal"));
	myTimer::SetNum(2);
	pLowest->After(0);
	pLow->After(0);
	t.After(s, 100000);
	test(s==KRequestPending);
	User::WaitForRequest(s);
	CActiveScheduler::Start();
	test(order[0]==eLow && order[1]==eLowest);
//
	test.Next(_L("Test low is not before lowest when it expires later"));
	myTimer::SetNum(2);
	pLowest->After(0);
	pLow->After(1000000);
	CActiveScheduler::Start();
	test(order[0]==eLowest && order[1]==eLow);
//
	test.Next(_L("Test none is scheduled before low"));
	myTimer::SetNum(2);
	pLow->After(0);
	pNone->After(0);
	t.After(s, 100000);
	test(s==KRequestPending);
	User::WaitForRequest(s);
	CActiveScheduler::Start();
	test(order[0]==eNone && order[1]==eLow);
//
	test.Next(_L("Test none is not before low when it expires later"));
	myTimer::SetNum(2);
	pLow->After(0);
	pNone->After(1000000);
	CActiveScheduler::Start();
	test(order[0]==eLow && order[1]==eNone);
//
	test.Next(_L("Test high is scheduled before none"));
	myTimer::SetNum(2);
	pHigh->After(0);
	pNone->After(0);
	t.After(s, 100000);
	test(s==KRequestPending);
	User::WaitForRequest(s);
	CActiveScheduler::Start();
	test(order[0]==eHigh && order[1]==eNone);
//
	test.Next(_L("Test high is not before none when it expires later"));
	myTimer::SetNum(2);
	pHigh->After(1000000);
	pNone->After(0);
	CActiveScheduler::Start();
	test(order[0]==eNone && order[1]==eHigh);
//
	test.Next(_L("Test highest is scheduled before high"));
	myTimer::SetNum(2);
	pHighest->After(0);
	pHigh->After(0);
	t.After(s, 100000);
	test(s==KRequestPending);
	User::WaitForRequest(s);
	CActiveScheduler::Start();
	test(order[0]==eHighest && order[1]==eHigh);
//
	test.Next(_L("Test highest is not before high when it expires later"));
	myTimer::SetNum(2);
	pHighest->After(1000000);
	pHigh->After(0);
	CActiveScheduler::Start();
	test(order[0]==eHigh && order[1]==eHighest);
//			  
	test.Next(_L("Test all objects are scheduled in priority order"));
	myTimer::SetNum(5);
	pLowest->After(0);
	pLow->After(0);
	pNone->After(0);
	pHigh->After(0);
	pHighest->After(0);
	t.After(s, 100000);
	test(s==KRequestPending);
	User::WaitForRequest(s);
	CActiveScheduler::Start();
	test(order[0]==eHighest && order[1]==eHigh && order[2]==eNone && order[3]==eLow && order[4]==eLowest);
//
	test.Next(_L("Test objects are scheduled according to expiry of timers"));
	myTimer::SetNum(5);
	pLowest->After(0);
	pLow->After(500000);
	pNone->After(1000000);
	pHigh->After(1500000);
	pHighest->After(2000000);
	CActiveScheduler::Start();
	test(order[4]==eHighest && order[3]==eHigh && order[2]==eNone && order[1]==eLow && order[0]==eLowest);
//
	test.Next(_L("Test with some objects having the same priority"));
	myTimer::SetNum(7);
	pLowest->After(0);
	pLow->After(0);
	pNone->After(0);
	pHigh->After(0);
	pHigh2->After(0);
	pHigh3->After(0);
	pHighest->After(0);	  
	t.After(s, 100000);
	test(s==KRequestPending);
	User::WaitForRequest(s);
	CActiveScheduler::Start();
	test(order[0]==eHighest && order[1]==eHigh && order[2]==eHigh+2 && order[3]==eHigh+3 && order[4]==eNone && order[5]==eLow && order[6]==eLowest);
//
	test.End();
	}

void TSecdulerTester::Test3()
//
// Test 3
//
	{

//
// Test that a leave in a Run method calls the requestmanager error() method
//
	myTimer* pTimer=new myTimer(0, KLeaveCode);
	pTimer->Start();
	pTimer->After(100000);
	TRAPD(ret,CActiveScheduler::Start());
	test(ret==KmyErrorNum);		
//
// Test cancelling an object
// Without the cancel the schedule order should be: pTimer1, gTimer, pTimer3
// gTimer is cancelled so should not be scheduled 
//
	myTimer::SetNum(2);
	myTimer* pTimer2=new myTimer(eHighest,KCancelCode);
	myTimer* pTimer3=new myTimer(eLowest,eLowest);
	pTimer2->Start();
	pTimer2->After(100000);
	gTimer->Start();
	gTimer->After(1000000);
	pTimer3->Start();
	pTimer3->After(2000000);
	CActiveScheduler::Start();
	test(order[0]==KCancelCode && order[1]==eLowest);
	delete pTimer;
	delete pTimer2;
	delete pTimer3;
	}

void TSecdulerTester::Test4()
//
// Create a thread with its own scheduler
//
	{	  

	threadSemaphore.CreateLocal(0);
	RThread myThread;
	test(myThread.Create(_L("myThread"),myThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,NULL)==KErrNone);
	myThread.Resume();
	myThread.Close();
	Test2();
	threadSemaphore.Wait();
	User::After(100000);
	}

void TSecdulerTester::Test5()
//
// Replace scheduler and check depth
//
	{
	MyManager* pManager=(MyManager*)CActiveScheduler::Current(); // get current

	test.Next(_L("Test scheduler replace"));
	TInt depthOld = pManager->StackDepth(); // get stack depth
	MyManager* pManager2=new MyManager;
	test(pManager2!=NULL);
	// replace active scheduler and check the old one was pManager
	test(CActiveScheduler::Replace(pManager2)==pManager);
	test(CActiveScheduler::Current()==pManager2);
	
	test.Next(_L("Test stack depth of the new scheduler"));
	TInt depthNew = pManager2->StackDepth(); // get stack depth
	test(depthOld==depthNew); // compare

	delete pManager2;
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
	TSecdulerTester sched;
	sched.Test1();
	MyManager* pManager=new MyManager;
	test(pManager!=NULL);
	CActiveScheduler::Install(pManager);
	test(CActiveScheduler::Current()==pManager);
	MyManager* pManager2=new MyManager;
	test(pManager2!=NULL);
	delete pManager2;
	test(CActiveScheduler::Current()==pManager);
	pLowest=new myTimer(eLowest,eLowest);
	test(pLowest!=NULL);
	pLow=new myTimer(eLow,eLow);
	test(pLow!=NULL);
	pNone=new myTimer(eNone,eNone);
	test(pNone!=NULL);
	pHigh=new myTimer(eHigh,eHigh);
	test(pHigh!=NULL);
	pHigh2=new myTimer(eHigh,eHigh+2);
	test(pHigh2!=NULL);
	pHigh3=new myTimer(eHigh,eHigh+3);
	test(pHigh3!=NULL);
	pHighest=new myTimer(eHighest,eHighest);
	test(pHighest!=NULL);
	pLowest->Start();
	pLow->Start();
	pNone->Start();
	pHigh->Start();
	pHigh2->Start();
	pHigh3->Start();
	pHighest->Start();
//
	test.Next(_L("Test2"));
	sched.Test2();	
	User::Check();
//
	test.Next(_L("Test3"));
	gTimer=new myTimer(eNone, KgTimerID);
	sched.Test3();
//
	test.Next(_L("Test4"));
	sched.Test4();
	delete gTimer;
	User::Check();
//
	test.Next(_L("Test5"));
	sched.Test5();

	delete pManager;
	delete pLowest;
	delete pLow;
	delete pNone;
	delete pHigh;
	delete pHigh2;
	delete pHigh3;
	delete pHighest;
//
	test.End();
	__UHEAP_MARKEND;

	User::SetJustInTime(justInTime);
	return(KErrNone);
    }
