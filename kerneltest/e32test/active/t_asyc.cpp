// Copyright (c) 1996-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\active\t_asyc.cpp
// Overview:
// Exercise the priority mechanism of active objects whereby active 
// objects are run in the priority order.
// API Information:
// CAsyncOneShot, CAsyncCallBack, CActiveScheduler.
// Details:
// - Install active scheduler.
// - Create active objects of different priorities and verify their RunL 
// is called in the priority order.
// - Verify that a very low priority active object will not get the chance
// to run if a higher priority object keeps rescheduling itself or a 
// higher priority object stops the active scheduler.
// - Do the same tests again by using CAsyncOneShot derived class
// CAsyncCallBack which has a CallBack function to do the stuff
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("T_ASYC"));

enum {ETopPriority=1000,EMiddlePriority=900,ELatePriority=800};

const TInt KIToldYouSo=666;


//

class CMyActiveScheduler : public CActiveScheduler
	{
public:
	virtual void Error(TInt anError) const;
	};

void CMyActiveScheduler::Error(TInt anError) const
//
// Called if any Run() method leaves.
//
	{

	test.Panic(anError,_L("CMyActiveScheduler::Error"));
	}

// 
// CAsyncOneShot derived class declarations
//

class CMyMultiShot : public CAsyncOneShot
	{
public:
	static CMyMultiShot* NewL(TInt aPriority,const TDesC& aMessage,TInt aCount);
	void RunL();
	CMyMultiShot(TInt aPriority,const TDesC& aMessage,TInt aCount);
private:
	TInt iCountRemaining;
	TPtrC iMessage;
	};

//

class CShouldNeverRun : public CAsyncOneShot
	{
public:
	void RunL();
	static CShouldNeverRun* NewL();
	CShouldNeverRun(TInt aValue);
	};

//

class CStopTheScheduler : public CAsyncOneShot
	{
public:
	CStopTheScheduler(TInt aPriority);
	static CStopTheScheduler* NewL(TInt aPriority);
	void RunL();
	};

//
// CAsyncCallBack derived class declarations
//

class CMyMultiShotACB : public CAsyncCallBack
	{
public:
	static CMyMultiShotACB* NewL(TInt aPriority,const TDesC& aMessage,TInt aCount);
	void RunL(); //needs to be implemented since CAsyncCallBack::RunL is not exported
	static TInt CallBackFunc(TAny* aPtr); // this is called by the CAsyncCallBack::RunL
	CMyMultiShotACB(const TCallBack& aCallBack, TInt aPriority, const TDesC& aMessage, TInt aCount);
private:
	TInt iCountRemaining;
	TPtrC iMessage;
	};

//

class CShouldNeverRunACB : public CAsyncCallBack
	{
public:
	void RunL(); //needs to be implemented since CAsyncCallBack::RunL is not exported
	static TInt CallBackFunc(TAny*); // this is called by the CAsyncCallBack::RunL
	static CShouldNeverRunACB* NewL();
	CShouldNeverRunACB();
	};

//

class CStopTheSchedulerACB : public CAsyncCallBack
	{
public:
	CStopTheSchedulerACB(TInt aPriority);
	static CStopTheSchedulerACB* NewL(TInt aPriority);
	static TInt CallBackFunc(TAny*); // this is called by the CAsyncCallBack::RunL
	void RunL(); //needs to be implemented since CAsyncCallBack::RunL is not exported
	};

//
// CAsyncOneShot implementations
//

CMyMultiShot* CMyMultiShot::NewL(TInt aPriority,const TDesC& aMessage,TInt aCount)
	{
	return new(ELeave)CMyMultiShot(aPriority,aMessage,aCount);
	}

CMyMultiShot::CMyMultiShot(TInt aPriority,const TDesC& aMessage,TInt aCount)
	:CAsyncOneShot(aPriority),iMessage(aMessage)
	{
	iCountRemaining=aCount;
	}

void CMyMultiShot::RunL()
	{
	if (iCountRemaining--)
		{
		test.Printf(_L("%S,%d\n\r"),&iMessage,iCountRemaining);
		Call();
		}
	}

CShouldNeverRun* CShouldNeverRun::NewL()
	{
	return new(ELeave)CShouldNeverRun(KMinTInt);
	}

CShouldNeverRun::CShouldNeverRun(TInt aValue)
	:CAsyncOneShot(aValue)
	{
	}

void CShouldNeverRun::RunL()
	{
	User::Panic(_L("CShouldNeverRun"),KIToldYouSo);
	}

CStopTheScheduler* CStopTheScheduler::NewL(TInt aPriority)
	{
	return new(ELeave)CStopTheScheduler(aPriority);
	}

CStopTheScheduler::CStopTheScheduler(TInt aPriority)
	:CAsyncOneShot(aPriority)
	{
	}

void CStopTheScheduler::RunL()
	{
	CActiveScheduler::Stop();
	}

//
// CAsyncCallBack derived implementations
//

CMyMultiShotACB* CMyMultiShotACB::NewL(TInt aPriority,const TDesC& aMessage,TInt aCount)
	{
	TCallBack myCallBack(CMyMultiShotACB::CallBackFunc);
	return new(ELeave)CMyMultiShotACB(myCallBack, aPriority,aMessage,aCount);
	}

CMyMultiShotACB::CMyMultiShotACB(const TCallBack& aCallBack, TInt aPriority,const TDesC& aMessage,TInt aCount)
	:CAsyncCallBack(aCallBack ,aPriority),iMessage(aMessage)
// this is calling the 2 parameter constructor for CAsyncCallBack
	{
	iCallBack.iPtr=this;
	iCountRemaining=aCount;
	}

TInt CMyMultiShotACB::CallBackFunc(TAny* aPtr)
	{
	CMyMultiShotACB* pMultiShot=(CMyMultiShotACB*)aPtr;

	if (pMultiShot->iCountRemaining--)
		{
		test.Printf(_L("%S,%d\n\r"),&(pMultiShot->iMessage),pMultiShot->iCountRemaining);
		pMultiShot->CallBack();
		}
	return KErrNone;
	}

void CMyMultiShotACB::RunL()
	{
	iCallBack.CallBack();
	}

CShouldNeverRunACB* CShouldNeverRunACB::NewL()
	{
	return new(ELeave)CShouldNeverRunACB;
	}

CShouldNeverRunACB::CShouldNeverRunACB()
	:CAsyncCallBack(KMinTInt)
// this is calling the 1 parameter constructor for CAsyncCallBack
// callback function needs to be set before setting active using ::Set method
	{
	}

TInt CShouldNeverRunACB::CallBackFunc(TAny*)
	{
	User::Panic(_L("CShouldNeverRunACB"),KIToldYouSo);
	return KErrNone;
	}

void CShouldNeverRunACB::RunL()
	{
	iCallBack.CallBack();
	}

CStopTheSchedulerACB* CStopTheSchedulerACB::NewL(TInt aPriority)
	{
	return new(ELeave)CStopTheSchedulerACB(aPriority);
	}

CStopTheSchedulerACB::CStopTheSchedulerACB(TInt aPriority)
	:CAsyncCallBack(aPriority)
// this is calling the 1 parameter constructor for CAsyncCallBack
// callback function needs to be set before setting active using ::Set method
	{
	}

TInt CStopTheSchedulerACB::CallBackFunc(TAny*)
	{
	CActiveScheduler::Stop();
	return KErrNone;
	}

void CStopTheSchedulerACB::RunL()
	{
	iCallBack.CallBack();
	}


GLDEF_C TInt E32Main()
//
// Test idle objects.
//
    {
	test.Title();
	test.Start(_L("Testing CAsyncOneShot derived objects"));
//
	CMyActiveScheduler* pR=new CMyActiveScheduler;
	test(pR!=NULL);
	CActiveScheduler::Install(pR);
//
	CShouldNeverRun* pn=CShouldNeverRun::NewL();
	pn->Call();

	CStopTheScheduler* ps=CStopTheScheduler::NewL(ELatePriority);
	ps->Call();

	CMyMultiShot* multiShot1=CMyMultiShot::NewL(EMiddlePriority,_L("Call Ten times"),10);
	multiShot1->Call();

	CMyMultiShot* multiShot2=CMyMultiShot::NewL(ETopPriority,_L("Call five times"),5);
	multiShot2->Call();

	CActiveScheduler::Start();	

	test.Next(_L("Test DoCancel"));
	// put to same priority as the CStopScheduler
	CShouldNeverRun* pn2=new CShouldNeverRun(ELatePriority);
	
	pn2->Call(); // queue the panic object
	
	ps->Call();  // queue stop scheduler object

	pn2->DoCancel(); //cancel immediately (panic if cancel fails)

	CActiveScheduler::Start();	

// cleanup, call destructors
	delete pn;
	delete pn2;
	delete ps;
	delete multiShot1;
	delete multiShot2;

//
	test.Next(_L("Testing CAsyncCallBack derived objects"));

	CShouldNeverRunACB* pnACB=CShouldNeverRunACB::NewL();
	TCallBack myCB1(CShouldNeverRunACB::CallBackFunc,pn);
	pnACB->Set(myCB1); // set callback func
	pnACB->CallBack(); // set active
		
	CStopTheSchedulerACB* psACB=CStopTheSchedulerACB::NewL(ELatePriority);
	TCallBack myCB2(CStopTheSchedulerACB::CallBackFunc,ps);
	psACB->Set(myCB2); // set callback func
	psACB->CallBack(); // set active

	CMyMultiShotACB* multiShot1ACB=CMyMultiShotACB::NewL(EMiddlePriority,_L("Call Ten times"),10);
	multiShot1ACB->CallBack(); // set active

	CMyMultiShotACB* multiShot2ACB=CMyMultiShotACB::NewL(ETopPriority,_L("Call five times"),5);
	multiShot2ACB->CallBack(); // set active

	CActiveScheduler::Start();	

// cleanup, call destructors
	delete pR;
	delete pnACB;
	delete psACB;
	delete multiShot1ACB;
	delete multiShot2ACB;

	test.End();
	return(0);
    }

