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
// e32test\misc\t_condvar2.cpp
// 
//

#define __E32TEST_EXTENSION__

#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32svr.h>
#include "u32std.h"
#include "../misc/prbs.h"
#include "../mmu/freeram.h"

const TInt KStackSize=0x1000;

RTest test(_L("T_CONDVAR2"));

const TInt KPacketSize = 1024;
const TInt KCrcSize = KPacketSize;
struct SMessage
	{
	TDblQueLink iLink;
	TUint	iSeq;
	TUint	iData[KPacketSize/4];
	TUint	iCrc;
	};

TDblQue<SMessage> Queue(0);
TUint PSeq;
TUint CSeq;
RHeap* SharedHeap;
RMutex Mutex;
RCondVar CV;
TInt NThreads;

class CThread : public CActive
	{
public:
	CThread(TInt aPriority);
	~CThread();
	virtual void RunL();
	virtual void DoCancel();
	virtual void DisplayStats()=0;
	TInt Start();
public:
	static TInt ThreadFunc(TAny* aPtr);
public:
	virtual TInt StartThread()=0;
	virtual TInt RunThread()=0;
public:
	RThread iThread;
	TInt iInnerCount;
	TInt iOuterCount;
	TInt iSuspendCount;
	TInt iExitCount;
	TInt iTerminateCount;
	TInt iCvCloseCount;
	TBool iProducer;
	TInt iId;
	TBool iCritical;
	TAny* iTempAlloc;
	};

class CProducerThread : public CThread
	{
public:
	static void NewL(TInt aId);
	CProducerThread(TInt aId);
	virtual TInt StartThread();
	virtual TInt RunThread();
	virtual void DisplayStats();
	};

class CConsumerThread : public CThread
	{
public:
	static void NewL(TInt aId);
	CConsumerThread(TInt aId);
	virtual TInt StartThread();
	virtual TInt RunThread();
	virtual void DisplayStats();
	};

class CRandomTimer : public CActive
	{
public:
	static void NewL();
	CRandomTimer(TInt aPriority);
	~CRandomTimer();
	virtual void RunL();
	virtual void DoCancel();
	void Start();
public:
	RTimer iTimer;
	TUint iSeed[2];
	TInt iCount;
	TInt iBackOff;
	TInt iZeroHandle;
	CThread* iSuspended;
	};

class CStatsTimer : public CActive
	{
public:
	static void NewL();
	CStatsTimer(TInt aPriority);
	~CStatsTimer();
	virtual void RunL();
	virtual void DoCancel();
	void Start();
public:
	RTimer iTimer;
	TInt iInitFreeRam;
	TInt iMaxDelta;
	TInt iCount;
	};

const TInt KNumProducers=4;
CProducerThread* TheProducers[KNumProducers];
const TInt KNumConsumers=4;
CConsumerThread* TheConsumers[KNumConsumers];
CRandomTimer* TheRandomTimer;

CThread::CThread(TInt aPriority)
	: CActive(aPriority)
	{
	}

CThread::~CThread()
	{
	Cancel();
	iThread.Kill(0);
	iThread.Close();
	}

TInt StartAllThreads()
	{
	TInt i;
	TInt r = CV.CreateLocal();
	if (r!=KErrNone)
		return r;
	for (i=0; i<KNumConsumers; ++i)
		{
		r = TheConsumers[i]->Start();
		if (r!=KErrNone)
			return r;
		}
	for (i=0; i<KNumProducers; ++i)
		{
		r = TheProducers[i]->Start();
		if (r!=KErrNone)
			return r;
		}
	return KErrNone;
	}

void CThread::RunL()
	{
	TExitType exitType = iThread.ExitType();
	TInt exitReason = iThread.ExitReason();
	const TDesC& exitCat = iThread.ExitCategory();
	TBool bad=EFalse;
	if (exitType==EExitKill)
		{
		if (exitReason!=KErrNone && exitReason!=KErrGeneral)
			bad=ETrue;
		}
	else if (exitType==EExitPanic)
		{
		if (exitCat!=_L("KERN-EXEC") || exitReason!=0 || CV.Handle()!=0)
			bad=ETrue;
		else
			++iCvCloseCount;
		}
	if (bad)
		{
		TFullName n(iThread.FullName());
		if (iProducer)
			test.Printf(_L("Thread %S (P%1d) exited %d,%d,%S\n"),&n,iId,exitType,exitReason,&exitCat);
		else
			test.Printf(_L("Thread %S (C%1d) exited %d,%d,%S\n"),&n,iId,exitType,exitReason,&exitCat);
		CActiveScheduler::Stop();
		return;
		}
	CLOSE_AND_WAIT(iThread);
	if (exitType==EExitTerminate)
		++iTerminateCount;
	else if (exitType==EExitKill && exitReason==KErrNone)
		++iExitCount;
	else if (exitType==EExitKill && exitReason==KErrGeneral)
		++iCvCloseCount;
	--NThreads;
	if (iTempAlloc)
		{
		SharedHeap->Free(iTempAlloc);
		iTempAlloc = NULL;
		}
	TInt r;
	if (CV.Handle()==0)
		{
		if (NThreads==0)
			r = StartAllThreads();
		else
			return;
		}
	else
		r=Start();
	if (r!=KErrNone)
		{
		test.Printf(_L("Start thread error %d\n"),r);
		CActiveScheduler::Stop();
		}
	}

void CThread::DoCancel()
	{
	iThread.LogonCancel(iStatus);
	}

TInt CThread::Start()
	{
	TInt r;
	FOREVER
		{
		r=StartThread();
		if (r==KErrNone)
			break;
		if (r!=KErrAlreadyExists)
			break;
		User::After(100000);
		}
	if (r==KErrNone)
		{
		iThread.Logon(iStatus);
		SetActive();
		}
	++NThreads;
	return r;
	}

TInt CThread::ThreadFunc(TAny* aPtr)
	{
	return ((CThread*)aPtr)->RunThread();
	}

CConsumerThread::CConsumerThread(TInt aId)
	: CThread(0)
	{
	iId = aId;
	}

void CConsumerThread::NewL(TInt aId)
	{
	CConsumerThread* pT=new (ELeave) CConsumerThread(aId);
	TheConsumers[aId] = pT;
	CActiveScheduler::Add(pT);
	User::LeaveIfError(pT->Start());
	}

TInt CConsumerThread::StartThread()
	{
	TInt r=iThread.Create(KNullDesC(), &ThreadFunc, KStackSize, SharedHeap, this);	// use unnamed thread
	if (r!=KErrNone)
		return r;
	iThread.Resume();
	return KErrNone;
	}

void CConsumerThread::DisplayStats()
	{
	test.Printf(_L("C%1d: I:%9d O:%9d S:%9d T:%9d C:%9d\n"), iId, iInnerCount, iOuterCount, iSuspendCount, iTerminateCount, iCvCloseCount);
	}

TInt CConsumerThread::RunThread()
	{
	Mutex.Wait();
	TInt r = KErrNone;
	FOREVER
		{
		while (Queue.IsEmpty())
			{
			r = CV.Wait(Mutex);
			++iInnerCount;
			if (r!=KErrNone)
				return r;
			}
		++iOuterCount;
		iCritical = ETrue;
		SMessage* m = Queue.First();
		m->iLink.Deque();
		iTempAlloc = m;
		TBool seq_ok = (m->iSeq == CSeq++);
		iCritical = EFalse;
		Mutex.Signal();
		if (!seq_ok)
			return KErrCorrupt;
		TUint16 crc = 0;
		Mem::Crc(crc, m->iData, KCrcSize);
		if (crc != m->iCrc)
			return KErrCorrupt;
		iCritical = ETrue;
		iTempAlloc = NULL;
		User::Free(m);
		iCritical = EFalse;
		Mutex.Wait();
		}
	}

CProducerThread::CProducerThread(TInt aId)
	: CThread(0)
	{
	iId = aId;
	}

void CProducerThread::NewL(TInt aId)
	{
	CProducerThread* pT=new (ELeave) CProducerThread(aId);
	TheProducers[aId] = pT;
	CActiveScheduler::Add(pT);
	User::LeaveIfError(pT->Start());
	}

TInt CProducerThread::StartThread()
	{
	TInt r=iThread.Create(KNullDesC(), &ThreadFunc, KStackSize, SharedHeap, this);	// use unnamed thread
	if (r!=KErrNone)
		return r;
	iThread.Resume();
	return KErrNone;
	}

void CProducerThread::DisplayStats()
	{
	test.Printf(_L("P%1d: I:%9d O:%9d S:%9d T:%9d C:%9d\n"), iId, iInnerCount, iOuterCount, iSuspendCount, iTerminateCount, iCvCloseCount);
	}

TInt CProducerThread::RunThread()
	{
	TUint seed[2];
	seed[0] = User::TickCount();
	seed[1] = 0;
	FOREVER
		{
		iCritical = ETrue;
		SMessage* m = new SMessage;
		iTempAlloc = m;
		iCritical = EFalse;
		TInt i = 0;
		for (; i<KPacketSize/4; ++i)
			m->iData[i] = Random(seed);
		TUint16 crc = 0;
		Mem::Crc(crc, m->iData, KCrcSize);
		m->iCrc = crc;
		Mutex.Wait();
		iCritical = ETrue;
		m->iSeq = PSeq++;
		Queue.AddLast(*m);
		iTempAlloc = NULL;
		iCritical = EFalse;
		CV.Signal();
		Mutex.Signal();
		++iOuterCount;
		if (!(Random(seed)&1))
			User::AfterHighRes(1000);
		}
	}

void CRandomTimer::NewL()
	{
	CRandomTimer* pR=new (ELeave) CRandomTimer(20);
	User::LeaveIfError(pR->iTimer.CreateLocal());
	CActiveScheduler::Add(pR);
	TheRandomTimer=pR;
	pR->Start();
	}

CRandomTimer::CRandomTimer(TInt aPriority)
	: CActive(aPriority)
	{
	iSeed[0]=User::TickCount();
	}

CRandomTimer::~CRandomTimer()
	{
	Cancel();
	iTimer.Close();
	}

void CRandomTimer::RunL()
	{
	++iCount;
	FOREVER
		{
		TUint x=Random(iSeed)&511;
		TInt tn=(TInt)(Random(iSeed) % (KNumConsumers + KNumProducers));
		CThread* pT = (tn>=KNumConsumers) ? (CThread*)TheProducers[tn-KNumConsumers] : (CThread*)TheConsumers[tn];
		if (x==511)
			{
			CV.Close();
			if (iSuspended)
				{
				if (iSuspended->iThread.Handle())
					iSuspended->iThread.Resume();
				iSuspended = NULL;
				}
			break;
			}
		if (pT->iThread.Handle()==0)
			{
			++iZeroHandle;
			continue;
			}
		if (x>=500)
			{
			if (pT->iCritical)
				{
				++iBackOff;
				continue;
				}
			pT->iThread.Terminate(0);
			if (iSuspended == pT)
				iSuspended = NULL;
			break;
			}
		if (iSuspended && (x&1))
			{
			if (iSuspended->iThread.Handle())
				iSuspended->iThread.Resume();
			iSuspended = NULL;
			}
		if (!iSuspended && !(x&15))
			{
			iSuspended = pT;
			pT->iThread.Suspend();
			++pT->iSuspendCount;
			}
		TThreadPriority tp;
		if (x>=400)
			tp = EPriorityMuchMore;
		else if (x>=300)
			tp = EPriorityMore;
		else if (x>=200)
			tp = EPriorityNormal;
		else if (x>=100)
			tp = EPriorityLess;
		else
			tp = EPriorityMuchLess;
		pT->iThread.SetPriority(tp);
		break;
		}
	Start();
	}

void CRandomTimer::Start()
	{
	TUint x=Random(iSeed)&15;
	x+=1;
	iTimer.HighRes(iStatus, x*1000);
	SetActive();
	}

void CRandomTimer::DoCancel()
	{
	iTimer.Cancel();
	}

void CStatsTimer::NewL()
	{
	CStatsTimer* pT=new (ELeave) CStatsTimer(-10);
	User::LeaveIfError(pT->iTimer.CreateLocal());
	CActiveScheduler::Add(pT);
	pT->Start();
	}

CStatsTimer::CStatsTimer(TInt aPriority)
	: CActive(aPriority)
	{
	iInitFreeRam = FreeRam();
	}

CStatsTimer::~CStatsTimer()
	{
	Cancel();
	iTimer.Close();
	}

void CStatsTimer::RunL()
	{
	TInt i;
	for (i=0; i<KNumProducers; i++)
		TheProducers[i]->DisplayStats();
	for (i=0; i<KNumConsumers; i++)
		TheConsumers[i]->DisplayStats();
	test.Printf(_L("RndTm: %9d BO: %9d ZH: %9d\n"), TheRandomTimer->iCount, TheRandomTimer->iBackOff, TheRandomTimer->iZeroHandle);
	TInt free_ram = FreeRam();
	TInt delta_ram = iInitFreeRam - free_ram;
	if (delta_ram > iMaxDelta)
		iMaxDelta = delta_ram;
	if (++iCount==10)
		{
		test.Printf(_L("Max RAM delta %dK Free RAM %08x\n"), iMaxDelta/1024, free_ram);
		iCount=0;
		}
	Start();
	}

void CStatsTimer::Start()
	{
	iTimer.After(iStatus, 1000000);
	SetActive();
	}

void CStatsTimer::DoCancel()
	{
	iTimer.Cancel();
	}

_LIT(KSharedHeap, "SharedHeap");
void InitialiseL()
	{
	PSeq = 0;
	CSeq = 0;
	User::LeaveIfError(Mutex.CreateLocal());
	User::LeaveIfError(CV.CreateLocal());
	User::LeaveIfNull(SharedHeap = UserHeap::ChunkHeap(&KSharedHeap, 0x1000, 0x01000000));
	CActiveScheduler* pA=new (ELeave) CActiveScheduler;
	CActiveScheduler::Install(pA);
	TInt id;
	for (id=0; id<KNumConsumers; ++id)
		CConsumerThread::NewL(id);
	for (id=0; id<KNumProducers; ++id)
		CProducerThread::NewL(id);
	CRandomTimer::NewL();
	CStatsTimer::NewL();
	}

GLDEF_C TInt E32Main()
//
// Test timers.
//
	{

	test.Title();

	RThread().SetPriority(EPriorityAbsoluteHigh);

	TRAPD(r,InitialiseL());
	test(r==KErrNone);

	User::SetJustInTime(EFalse);

	CActiveScheduler::Start();

	test(0);

	return(0);
	}

