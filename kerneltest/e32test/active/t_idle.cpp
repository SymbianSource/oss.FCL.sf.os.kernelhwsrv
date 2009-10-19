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
// e32test\active\t_idle.cpp
// Overview:
// Test the CIdle class. A CIdle and CTimer run on the same active scheduler and
// the test displays how their callback function and RunL respectively are called.
// API Information:
// CIdle 
// Details:
// - Create and install active scheduler.
// - Create and start a timer, add this to the active scheduler, request an event
// after a specified interval. The timer will reschedule itself for a finite
// number of times, then it will cancel the CIdle object and then reschedule
// itself again and eventually it will stop the active scheduler.
// - Create a CIdle active object, start a background task that is encapsulated in
// callback function. The CIdle will keep reschedule itself by always returning
// TRUE from the callback.
// - Output some text from timer's RunL and CIdle's callback to show when they are
// called and how they interleave.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

enum {EIdlePriority=1000,EMoreIdlePriority=800};

class CMyRequestManager : public CActiveScheduler
	{
public:
	virtual void Error(TInt anError) const;
	};

class CMyTimer : public CTimer
	{
public:
	static CMyTimer* New();
	void Start();
	virtual void RunL();
protected:
	CMyTimer(TInt aPriority);
private:
	enum {EMaxCount=10,EStopCount=5,ETimeReq=100000};
	TInt iCount;
	};

LOCAL_D RTest test(_L("T_IDLE"));
LOCAL_D CIdle* MoreIdle;
LOCAL_D TInt TheCount=0;

void CMyRequestManager::Error(TInt anError) const
//
// Called if any Run() method leaves.
//
	{

	test.Panic(anError,_L("CMyRequestManager::Error"));
	}

CMyTimer* CMyTimer::New()
//
// Create a new CMyTimer.
//
	{

	return(new CMyTimer(EIdlePriority));
	}

CMyTimer::CMyTimer(TInt aPriority)
//
// Constructor
//
	: CTimer(aPriority),iCount(0)
	{}

void CMyTimer::Start()
//
// The timer has completed.
//
	{

	TRAPD(r, ConstructL());
	test(r==KErrNone);
	CActiveScheduler::Add(this);
	After(ETimeReq);
	}

void CMyTimer::RunL()
//
// The timer has completed.
//
	{

	iCount++;
	test.Printf(_L("Timer %03d\n"),iCount);
	if (iCount==EStopCount)
		MoreIdle->Cancel();
	if (iCount<EMaxCount)
		After(ETimeReq);
	else
		{
		test.Printf(_L("\n"));
		DoCancel();
		CActiveScheduler::Stop();
		}
	}

TInt IdleResponse(TAny*)
//
//	Respond to the idle callback
//
	{

	test.Printf(_L("\rIdle response number : %6d "),++TheCount);
	return(TRUE);
	}

TInt IdleCancel(TAny*)
//
//	Cancel the other idle object
//
	{

	if (TheCount>9)
		{
		MoreIdle->Cancel();
		return(FALSE);
		}
	return(TRUE);
	}
		
GLDEF_C TInt E32Main()
//
// Test idle objects.
//
    {
	
	test.Title();
	test.Start(_L("Testing idle object cancellation"));
//
	CMyRequestManager* pR=new CMyRequestManager;
	test(pR!=NULL);
	CActiveScheduler::Install(pR);
//
	CMyTimer* pT=CMyTimer::New();
	pT->Start();
	MoreIdle=CIdle::New(EIdlePriority);
	MoreIdle->Start(&IdleResponse);
	CActiveScheduler::Start();	
//
	test.End();
	return(0);
    }

