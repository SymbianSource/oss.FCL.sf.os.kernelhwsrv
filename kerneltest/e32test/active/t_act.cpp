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
// e32test\active\t_act.cpp
// Overview:
// Test CTimer based timers and error handling in active objects and 
// active scheduler
// API Information:
// CTimer, CActiveScheduler
// Details:
// - Create and install the active scheduler
// - Create a timer, add it to the active scheduler and start the timer
// - Verify RunL is run when the timer fires off and test dequeuing itself from 
// the active scheduler
// - Test that when a leave in RunL occurs, the active object gets the chance to 
// handle it before the active scheduler
// - Test that leaves in RunL can be handled successfully in the active object
// - Test the active object can propagate the leave to the active scheduler
// - Test that leaves in RunL can be handled successfully in the active scheduler
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32panic.h>

class CMyRequestManager : public CActiveScheduler
	{
public:
	enum TMode
		{
		EExpectError,
		EGenerateException,
		EPanic
	} iMode;
public:
	virtual void Error(TInt anError) const;
	void SetMode(TMode aExpect);

private:
	TBool iExpectError;
	};

class CMyTimer : public CTimer
	{
public:
	static CMyTimer* New();
	void Start();
	virtual void RunL();
	void StartLeave(TBool aHandleLocally, TBool aLeaveInOOM, CMyRequestManager::TMode aMode);
	virtual TInt RunError(TInt aError);
	void StopLeave();
	void StartImbalance();
	virtual ~CMyTimer();
protected:
	CMyTimer(TInt aPriority);
private:
	enum {EMaxCount=10,ETimeReq=100000};
	CBase* iDummy;
	TInt iCount;
	TBool iConstructed;
	TBool iLeave;
	TBool iHandleLocally;
	TBool iLeaveInOOM;
	TBool iImbalance;
	TBool iStopping;
	};

LOCAL_D RTest test(_L("T_ACT"));

void CMyRequestManager::Error(TInt anError) const
//
// Called if any Run() method leaves.
//
	{
	switch (iMode)
		{
		case EExpectError:
			{
			_LIT(KExpectError,"CMyRequestManager::Error handling error %d\n");
			test.Printf(KExpectError,anError);
			Stop();
			return;
			}
		case EGenerateException:
			{

			_LIT(KExpectError,"CMyRequestManager::Error about to generate exception...\n");
			test.Printf(KExpectError,anError);

			__UHEAP_FAILNEXT(1);
			TRAPD(ret,User::Leave(KErrArgument));
			__UHEAP_RESET;
	   		if (ret != KErrArgument) 
	   			{
				_LIT(KDoNotExpectError,"CMyRequestManager::Error unexpected");
				test.Panic(anError,KDoNotExpectError);
	   			}
			Stop();
			return;	
			}
		case EPanic:
		default:
			{
			_LIT(KDoNotExpectError,"CMyRequestManager::Error unexpected");
			test.Panic(anError,KDoNotExpectError);
			}
		}
	}


void CMyRequestManager::SetMode(TMode aMode)
	{
	iMode = aMode;
	}

CMyTimer* CMyTimer::New()
//
// Create a new CMyTimer.
//
	{

	return(new CMyTimer(0));
	}

CMyTimer::CMyTimer(TInt aPriority)
//
// Constructor
//
	: CTimer(aPriority),iCount(0), iImbalance(EFalse), iStopping(EFalse)
	{}

CMyTimer::~CMyTimer()
	{
	delete iDummy;
	}

void CMyTimer::Start()
//
// Start the timer
//
	{
	if (!iConstructed)
		{
		TRAPD(r, ConstructL());
		test(r==KErrNone);
		iConstructed = ETrue;
		iDummy = new(ELeave)CBase();
		}
	CActiveScheduler::Add(this); // Previously caused panic in UREL after Deque()
	SetPriority(1);
	After(ETimeReq);
	}

void CMyTimer::StartLeave(TBool aHandleLocally, TBool aLeaveInOOM, CMyRequestManager::TMode aMode)
//
// Start the timer
//
	{
	CActiveScheduler::Add(this); 
	SetPriority(1);
	After(ETimeReq);
	iLeave = ETrue;
	iHandleLocally = aHandleLocally;
	iLeaveInOOM = aLeaveInOOM;
	//	STATIC_CAST(CMyRequestManager*,CActiveScheduler::Current())->ExpectError(!aHandleLocally);
	STATIC_CAST(CMyRequestManager*,CActiveScheduler::Current())->SetMode(aMode);
	}

void CMyTimer::StopLeave()
	{
	iLeave=EFalse;
	}

void CMyTimer::StartImbalance()
	{
	iImbalance=ETrue;
	}

void CMyTimer::RunL()
//
// The timer has completed.
//
	{
	if (iLeave) 
		{
		Deque(); // Test removal from scheduler
		if (iLeaveInOOM)
			{
			__UHEAP_FAILNEXT(1);
			}
		User::Leave(KErrGeneral);
		}
	
	// This switch is used when testing for imbalance in the cleanupstack
	if(iImbalance)
		{
		if(iStopping)
			{
			
			iStopping=EFalse;
			//CleanupStack::PopAndDestroy(iDummy);
			CActiveScheduler::Stop();
			return;
			}
		else 
			{
			// Push something onto the stack, but dont take it off 
			//- deal in CActiveScheduler::DoRunL
			CleanupStack::PushL(iDummy);
			iStopping=ETrue; //Stop the scheduler the next time
			iCount=EMaxCount; 
//			CActiveScheduler::Stop();
			After(ETimeReq);
			return;
			}
		}

	iCount++;
	SetPriority(iCount);
	test.Printf(_L("\r%03d"),iCount);

	if (iCount<EMaxCount)
		After(ETimeReq);
	else
		{
		test.Printf(_L("\n"));
		CActiveScheduler::Stop();
		Deque(); // Test removal from scheduler
		iCount = 0;
		}
	}

TInt CMyTimer::RunError(TInt aError)
//
// Handle leave from RunL
//
	{
	if (iHandleLocally)
		{
		_LIT(KExpectError,"CMyTimer::RunError handling error %d\n");
		test.Printf(KExpectError,aError);
		CActiveScheduler::Stop();
		return KErrNone;
		}
	else 
		return aError; // Let the scheduler handle this error
	}


TInt DoImbalanceTest(TAny* /*aAny*/)
// This function is the first executing fuction of the cleanupstack imbalace
// testing thread - RunLCleanupImbalance
// see ImbalanceTest()
	{
	// Set up cleanup stack & scheduler
	RTest test(_L("Thread:RunLCleanupImbalance - DoImbalanceTest()"));
	test.Start(_L("DoImbalanceTest()"));

	//Create a cleanup stack and scheduler
	CTrapCleanup::New();
	CActiveScheduler* cas = new(ELeave) CActiveScheduler();
	CActiveScheduler::Install(cas);

	// Create a new AO.
	CMyTimer* myTimer = CMyTimer::New();
	myTimer->StopLeave();
	myTimer->StartImbalance(); 
	// Start the AO
	test.Next(_L("Start Imblance Test"));
	myTimer->Start();
	
	test.Next(_L("Start Scheduler"));
	// The following is expected to panic (with EUSER-CBase 90 EClnCheckFailed)
	cas->Start(); 
	
	delete myTimer;
	delete cas;
	test.End();
	return 0;
	}

#ifdef _DEBUG
LOCAL_C void ImbalanceTest()
// this test will test whether the cleanup stack is imbalanced after
// a runL of an Active object.
	{
	TBool imbalanced = ETrue;
	User::SetJustInTime(EFalse);
	RThread t;
	TRequestStatus s;
	test.Next(_L("Create a thread (RunLCleanupImbalance)"));
	TInt r=t.Create(_L("RunLCleanupImbalance"),DoImbalanceTest,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,&imbalanced);
	test(r==KErrNone);
	t.Logon(s);
	test.Next(_L("Resume and wait for panic (E32USER-CBase 90 EClnCheckFailed) due to imbalance"));
	t.Resume();
	User::WaitForRequest(s);
	
	test.Printf(_L("Exit Type %d\r\n"),(TInt)t.ExitType());
	test.Printf(_L("Exit Reason %d\r\n"),(TInt)t.ExitReason());
	
	test(t.ExitReason()==EClnCheckFailed);
	
	CLOSE_AND_WAIT(t);

	}
#endif

GLDEF_C TInt E32Main()
//
// Test timers.
//
    {
 	test.Title();
	test.Start(_L("Creating CActiveScheduler"));
	CMyRequestManager* pR=new CMyRequestManager;
	test(pR!=NULL);
	CActiveScheduler::Install(pR);
//
	test.Next(_L("Testing relative timers"));
	CMyTimer* pT=CMyTimer::New();
	test(pT!=NULL);
//
	test.Next(_L("Start timer"));
	pT->Start();
//
	test.Next(_L("Start CMyRequestManager"));
	CActiveScheduler::Start();
//
	test.Next(_L("Start timer again"));
	pT->Start();
//
	test.Next(_L("Start CMyRequestManager"));
	CActiveScheduler::Start();
//
	test.Next(_L("Start timer, leave in RunL, handle in scheduler"));
	pT->StartLeave(EFalse, EFalse, CMyRequestManager::EExpectError );
//
	test.Next(_L("Start CMyRequestManager"));
	CActiveScheduler::Start();
#ifdef _DEBUG
//
	test.Next(_L("Start timer, leave in RunL, generate nested exception under OOM condition"));
	pT->StartLeave(EFalse, ETrue, CMyRequestManager::EGenerateException);
//
	test.Next(_L("Start CMyRequestManager"));
	CActiveScheduler::Start();
	__UHEAP_RESET;
#endif
	//
	test.Next(_L("Start timer, leave in RunL, handle in object"));
	pT->StartLeave(ETrue, EFalse, CMyRequestManager::EPanic);
//
	test.Next(_L("Start CMyRequestManager"));
	CActiveScheduler::Start();
//
#ifdef _DEBUG
// Test the cleanupstack imbalances
	test.Next(_L("Test : Check Cleanupstack imbalance in RunL, handle(panic) in scheduler"));
	ImbalanceTest();
#endif
//
	test.End();
	return(0); 
    }
