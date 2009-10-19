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
// e32test\active\t_asyc.cpp
// Overview:
// Exercise the priority mechanism of active objects whereby active 
// objects are run in the priority order.
// API Information:
// CAsyncOneShot, CActiveScheduler.
// Details:
// - Install active scheduler.
// - Create active objects of different priorities and verify their RunL 
// is called in the priority order.
// - Verify that a very low priority active object will not get the chance
// to run if a higher priority object keeps rescheduling itself or a 
// higher priority object stops the active scheduler.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

enum {ETopPriority=1000,EMiddlePriority=900,ELatePriority=800};

const TInt KIToldYouSo=666;

class CMyActiveScheduler : public CActiveScheduler
	{
public:
	virtual void Error(TInt anError) const;
	};

LOCAL_D RTest test(_L("T_ASYC"));

void CMyActiveScheduler::Error(TInt anError) const
//
// Called if any Run() method leaves.
//
	{

	test.Panic(anError,_L("CMyActiveScheduler::Error"));
	}

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

class CShouldNeverRun : public CAsyncOneShot
	{
public:
	void RunL();
	static CShouldNeverRun* NewL();
	CShouldNeverRun();
	};

class CStopTheScheduler : public CAsyncOneShot
	{
public:
	CStopTheScheduler(TInt aPriority);
	static CStopTheScheduler* NewL(TInt aPriority);
	void RunL();
	};

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
	return new(ELeave)CShouldNeverRun;
	}

CShouldNeverRun::CShouldNeverRun()
	:CAsyncOneShot(KMinTInt)
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

GLDEF_C TInt E32Main()
//
// Test idle objects.
//
    {
	test.Title();
	test.Start(_L("Testing idle object cancellation"));
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
//
	test.End();
	return(0);
    }

