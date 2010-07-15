// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include <e32test.h>
#include <e32svr.h>

#include "bm_suite.h"

class Thread : public BMProgram
	{
public :
	Thread() : BMProgram(_L("Threads"))
		{}
	virtual TBMResult* Run(TBMUInt64 aIter, TInt* aCount);

	typedef void (*MeasurementFunc)(TBMResult*, TBMUInt64 aIter);
	struct Measurement 
		{
		MeasurementFunc iFunc;
		TPtrC			iName;

		Measurement(MeasurementFunc aFunc, const TDesC& aName) : 
					iFunc(aFunc), iName(aName) {}
		};

	static TBMResult iResults[];
	static Measurement iMeasurements[];

	static TBMTicks	iChildTime;

	static void Creation(TBMResult*, TBMUInt64 aIter);
	static TInt CreationChild(TAny*);
	static void CreationSuicide(TBMResult*, TBMUInt64 aIter);
	static TInt CreationSuicideChild(TAny*);
	static void Suicide(TBMResult*, TBMUInt64 aIter);
	static TInt SuicideChild(TAny*);
	static void Killing(TBMResult*, TBMUInt64 aIter);
	static TInt KillingChild(TAny*);
	static void SetTls(TBMResult*, TBMUInt64 aIter);
	static void GetTls(TBMResult*, TBMUInt64 aIter);

	void EnableCleanup()
		{
		TInt prio = BMProgram::SetAbsPriority(RThread(), iOrigAbsPriority);
		BMProgram::SetAbsPriority(RThread(), prio);
		}
	};

Thread::Measurement Thread::iMeasurements[] =
	{
	Measurement(&Thread::Creation, _L("Thread Creation Latency")),
	Measurement(&Thread::CreationSuicide, _L("Thread Creation Suicide")),
	Measurement(&Thread::Suicide, _L("Thread Suicide")),
	Measurement(&Thread::Killing, _L("Thread Killing")),
	Measurement(&Thread::SetTls, _L("Setting per-thread data")),
	Measurement(&Thread::GetTls, _L("Getting per-thread data"))
	};
TBMResult Thread::iResults[sizeof(Thread::iMeasurements)/sizeof(Thread::iMeasurements[0])];

TBMTicks Thread::iChildTime;

static Thread prog;

void Thread::Creation(TBMResult* aResult, TBMUInt64 aIter)
	{
	for (TBMUInt64 i = 0; i < aIter; ++i)
		{
		RThread child;
		TRequestStatus st;
		TBMTicks t1;
		::bmTimer.Stamp(&t1);
		TInt r = child.Create(KNullDesC, Thread::CreationChild, 0x2000, NULL, NULL);
		BM_ERROR(r, r == KErrNone);
		child.Logon(st);
		BMProgram::SetAbsPriority(RThread(), KBMPriorityHigh);
		child.Resume();
		User::WaitForRequest(st);
		BM_ERROR(st.Int(), st == KErrNone);
		aResult->Cumulate(TBMTicksDelta(t1, iChildTime));
		CLOSE_AND_WAIT(child);
		prog.EnableCleanup();
		}
	}

TInt Thread::CreationChild(TAny*)
	{
	::bmTimer.Stamp(&iChildTime);
	return KErrNone;
	}

void Thread::CreationSuicide(TBMResult* aResult, TBMUInt64 aIter)
	{
	for (TBMUInt64 i = 0; i < aIter; ++i)
		{
		RThread child;
		TRequestStatus st;
		TBMTicks t1;
		::bmTimer.Stamp(&t1);
		TInt r = child.Create(KNullDesC, Thread::CreationSuicideChild, 0x2000, NULL, NULL);
		BM_ERROR(r, r == KErrNone);
		child.Logon(st);
		BMProgram::SetAbsPriority(RThread(), KBMPriorityLow);
		child.Resume();
		User::WaitForRequest(st);
		BM_ERROR(st.Int(), st == KErrNone);
		TBMTicks t2;
		::bmTimer.Stamp(&t2);
		aResult->Cumulate(TBMTicksDelta(t1, t2));
		CLOSE_AND_WAIT(child);
		prog.EnableCleanup();
		}
	}

TInt Thread::CreationSuicideChild(TAny*)
	{
	return KErrNone;
	}
						
void Thread::Suicide(TBMResult* aResult, TBMUInt64 aIter)
	{
	for (TBMUInt64 i = 0; i < aIter; ++i)
		{
		RThread child;
		TRequestStatus st;
		TInt r = child.Create(KNullDesC, Thread::SuicideChild, 0x2000, NULL, NULL);
		BM_ERROR(r, r == KErrNone);
		child.Logon(st);
		BMProgram::SetAbsPriority(RThread(), KBMPriorityLow);
		child.Resume();
		User::WaitForRequest(st);
		BM_ERROR(st.Int(), st == KErrNone);
		TBMTicks t2;
		::bmTimer.Stamp(&t2);
		aResult->Cumulate(TBMTicksDelta(iChildTime, t2));
		CLOSE_AND_WAIT(child);
		prog.EnableCleanup();
		}
	}

TInt Thread::SuicideChild(TAny*)
	{
	::bmTimer.Stamp(&iChildTime);
	return KErrNone;
	}

void Thread::Killing(TBMResult* aResult, TBMUInt64 aIter)
	{
	for (TBMUInt64 i = 0; i < aIter; ++i)
		{
		RThread child;
		TRequestStatus st;
		TInt r = child.Create(KNullDesC, Thread::KillingChild, 0x2000, NULL, NULL);
		BM_ERROR(r, r == KErrNone);
		child.Logon(st);
		BMProgram::SetAbsPriority(RThread(), KBMPriorityLow);
		TRequestStatus threadRunning;
		child.Rendezvous(threadRunning);
		child.Resume();
		User::WaitForRequest(threadRunning);	// Wait for the thread to run before killing it.
		BMProgram::SetAbsPriority(RThread(), KBMPriorityHigh);
		TBMTicks t1;
		::bmTimer.Stamp(&t1);
		child.Kill(KErrCancel);
		User::WaitForRequest(st);
		BM_ERROR(st.Int(), st == KErrCancel);
		TBMTicks t2;
		::bmTimer.Stamp(&t2);
		aResult->Cumulate(TBMTicksDelta(t1, t2));
		CLOSE_AND_WAIT(child);
		prog.EnableCleanup();
		}
	}

TInt Thread::KillingChild(TAny*)
	{
	RThread::Rendezvous(KErrNone);
	User::WaitForAnyRequest();
	return KErrNone;
	}

#define TLS_KEY ((TInt32) &Thread::SetTls)

void Thread::SetTls(TBMResult* aResult, TBMUInt64 aIter)
	{
	aIter <<= 4;

	TBMTimeInterval ti;
	ti.Begin();
	for (TBMUInt64 i = 0; i < aIter; ++i)
		{
		TInt r = UserSvr::DllSetTls(TLS_KEY, 0);
		BM_ERROR(r, r == KErrNone);
		}
	TBMTicks t = ti.End();
	aResult->Cumulate(t, aIter);

	UserSvr::DllFreeTls(TLS_KEY);
	}

void Thread::GetTls(TBMResult* aResult, TBMUInt64 aIter)
	{
	aIter <<= 4;

	TInt r = UserSvr::DllSetTls(TLS_KEY, 0);
	BM_ERROR(r, r == KErrNone);

	TBMTimeInterval ti;
	ti.Begin();
	for (TBMUInt64 i = 0; i < aIter; ++i)
		{
		UserSvr::DllTls(TLS_KEY);
		}
	TBMTicks t = ti.End();
	aResult->Cumulate(t, aIter);
	
	UserSvr::DllFreeTls(TLS_KEY);
	}

TBMResult* Thread::Run(TBMUInt64 aIter, TInt* aCount)
	{
	TInt count = sizeof(iResults)/sizeof(iResults[0]);

	for (TInt i = 0; i < count; ++i)
		{
		iResults[i].Reset(iMeasurements[i].iName);
		iMeasurements[i].iFunc(&iResults[i], aIter);
		iResults[i].Update();
		}
	
	*aCount = count;
	return iResults;
	}

void AddThread()
	{
	BMProgram* next = bmSuite;
	bmSuite=(BMProgram*)&prog;
	bmSuite->Next()=next;
	}
