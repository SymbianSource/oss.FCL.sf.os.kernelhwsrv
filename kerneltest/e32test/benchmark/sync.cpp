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
#include <e32msgqueue.h>

#include "bm_suite.h"

class Sync : public BMProgram
	{
public :
	Sync() : BMProgram(_L("Synchronization Primitives"))
		{}
	virtual TBMResult* Run(TBMUInt64 aIter, TInt* aCount);

	typedef void (*MeasurementFunc)(TBMResult*, TBMUInt64 aIter, TBool aRemote);
	struct Measurement 
		{
		MeasurementFunc iFunc;
		TPtrC			iName;
		TBool			iRemote;

		Measurement(MeasurementFunc aFunc, const TDesC& aName, TBool aRemote = EFalse) : 
					iFunc(aFunc), iName(aName), iRemote(aRemote) {}
		};

	static TBMResult iResults[];
	static Measurement iMeasurements[];

	static void MutexPassing(TBMResult*, TBMUInt64 aIter, TBool aRemote);
	static void MutexContentionParent(TBMResult* aResult, TBMUInt64 aIter, TBool aRemote);
	static TInt MutexContentionChild(TAny*);
	static void SemaphoreLatencyParent(TBMResult* aResult, TBMUInt64 aIter, TBool aRemote);
	static TInt SemaphoreLatencyChild(TAny*);
	static void ThreadSemaphoreLatencyParent(TBMResult* aResult, TBMUInt64 aIter, TBool aRemote);
	static TInt ThreadSemaphoreLatencyChild(TAny*);
	};

Sync::Measurement Sync::iMeasurements[] =
	{
	Measurement(&Sync::MutexPassing, _L("Mutex Passing Case")),
	Measurement(&Sync::MutexContentionParent, _L("Local Mutex Contention")),
	Measurement(&Sync::MutexContentionParent, _L("Remote Mutex Contention"), ETrue),
	Measurement(&Sync::SemaphoreLatencyParent, _L("Local Semaphore Latency")),
	Measurement(&Sync::SemaphoreLatencyParent, _L("Remote Semaphore Latency"), ETrue),
	Measurement(&Sync::ThreadSemaphoreLatencyParent, _L("Local Thread Semaphore Latency")),
	};
TBMResult Sync::iResults[sizeof(Sync::iMeasurements)/sizeof(Sync::iMeasurements[0])];

static Sync sync;

void Sync::MutexPassing(TBMResult* aResult, TBMUInt64 aIter, TBool)
	{
	RMutex mutex;
	mutex.CreateLocal();

	TBMTimeInterval ti;
	ti.Begin();
	for (TBMUInt64 i = 0; i < aIter; ++i)
		{
		mutex.Wait();
		mutex.Signal();
		}
	TBMTicks t = ti.End();

	mutex.Close();

	aResult->Cumulate(t, aIter);
	}

class MutexContentionArgs : public TBMSpawnArgs
	{
public:
	
	RMutex		iMutexA;
	RMutex		iMutexB;
	RSemaphore	iSem;
	TBMUInt64	iIterationCount;

	MutexContentionArgs(TInt aRemote, TBMUInt64 aIter);

	void ChildOpen();
	void ChildClose();

	void Close();
	};

MutexContentionArgs::MutexContentionArgs(TInt aRemote, TBMUInt64 aIter) :
		TBMSpawnArgs(Sync::MutexContentionChild, KBMPriorityLow, aRemote, sizeof(*this)),
		iIterationCount(aIter)
	{
	TInt r;
	if (aRemote)
		{
		r = iMutexA.CreateGlobal(_L("MutexA"));
		BM_ERROR(r, r == KErrNone);
		r = iMutexB.CreateGlobal(_L("MutexB"));
		BM_ERROR(r, r == KErrNone);
		r = iSem.CreateGlobal(_L("Semaphore"), 0);
		BM_ERROR(r, r == KErrNone);
		}
	else
		{
		r = iMutexA.CreateLocal();
		BM_ERROR(r, r == KErrNone);
		r = iMutexB.CreateLocal();
		BM_ERROR(r, r == KErrNone);
		r = iSem.CreateLocal(0);
		BM_ERROR(r, r == KErrNone);
		}
	}

void MutexContentionArgs::ChildOpen()
	{
	if (iRemote)
		{
		iMutexA.Duplicate(iParent);
		iMutexB.Duplicate(iParent);
		iSem.Duplicate(iParent);
		}
	}

void MutexContentionArgs::ChildClose()
	{
	if (iRemote)
		{
		iMutexA.Close();
		iMutexB.Close();
		iSem.Close();
		}
	}

void MutexContentionArgs::Close()
	{
	iMutexA.Close();
	iMutexB.Close();
	iSem.Close();
	}

void Sync::MutexContentionParent(TBMResult* aResult, TBMUInt64 aIter, TBool aRemote)
	{
	MutexContentionArgs mc(aRemote, aIter);

	MBMChild* child = sync.SpawnChild(&mc);

	mc.iSem.Wait();

	TBMTimeInterval ti;
	ti.Begin();
	for (TBMUInt64 i = 0; i < aIter; ++i)
		{
		mc.iMutexA.Wait();
		mc.iMutexA.Signal();
		mc.iMutexB.Wait();
		mc.iMutexB.Signal();
		}	
	TBMTicks t = ti.End();

	child->WaitChildExit();
	mc.Close();

	aResult->Cumulate(t/2, aIter);
	}

TInt Sync::MutexContentionChild(TAny* ptr)
	{
	MutexContentionArgs* mc = (MutexContentionArgs*) ptr;
	mc->ChildOpen();

	mc->iMutexA.Wait();
	mc->iSem.Signal();
	for (TBMUInt64 i = 0; i < mc->iIterationCount; ++i)
		{
		mc->iMutexB.Wait();
		mc->iMutexA.Signal();
		mc->iMutexA.Wait();
		mc->iMutexB.Signal();
		}
	mc->iMutexA.Signal();

	mc->ChildClose();
	return KErrNone;
	}

class SemaphoreLatencyArgs : public TBMSpawnArgs
	{
public:
	
	RSemaphore			iSem;
	TBMUInt64			iIterationCount;
	RMsgQueue<TBMTicks>	iSignalTimeQue;

	SemaphoreLatencyArgs(TInt aRemote, TBMUInt64 aIter);

	void ChildOpen();
	void ChildClose();

	TBMTicks SignalTime();
	void ChildSignalTime(TBMTicks);

	void Close();
	};

SemaphoreLatencyArgs::SemaphoreLatencyArgs(TInt aRemote, TBMUInt64 aIter) : 
	TBMSpawnArgs(Sync::SemaphoreLatencyChild, KBMPriorityLow, aRemote, sizeof(*this)),
	iIterationCount(aIter)
	{
	TInt r;
	if (aRemote)
		{
		r = iSem.CreateGlobal(_L("BM Semaphore"), 0);
		BM_ERROR(r, r == KErrNone);
		}
	else
		{
		r = iSem.CreateLocal(0);
		BM_ERROR(r, r == KErrNone);
		}
	r = iSignalTimeQue.CreateGlobal(_L("BM Queue"), 1);
	BM_ERROR(r, r == KErrNone);
	}	

void SemaphoreLatencyArgs::ChildOpen()
	{
	if (iRemote)
		{
		iSem.Duplicate(iParent);
		TInt r = iSignalTimeQue.OpenGlobal(_L("BM Queue"));
		BM_ERROR(r, r == KErrNone);
		}
	}

void SemaphoreLatencyArgs::ChildSignalTime(TBMTicks aTime)
	{
	TInt r = iSignalTimeQue.Send(aTime);
	BM_ERROR(r, r == KErrNone);
	}

TBMTicks SemaphoreLatencyArgs::SignalTime()
	{
	TBMTicks time;
	iSignalTimeQue.ReceiveBlocking(time);
	return time;
	}

void SemaphoreLatencyArgs::ChildClose()
	{
	if (iRemote)
		{
		iSem.Close();
		iSignalTimeQue.Close();
		}
	}

void SemaphoreLatencyArgs::Close()
	{
	iSem.Close();
	iSignalTimeQue.Close();
	}

void Sync::SemaphoreLatencyParent(TBMResult* aResult, TBMUInt64 aIter, TBool aRemote)
	{
	RSemaphore slSync;
	TInt r = slSync.CreateGlobal(_L("slSync"), 0);
	BM_ERROR(r, r == KErrNone);	

	SemaphoreLatencyArgs sl(aRemote, aIter);
	MBMChild* child = sync.SpawnChild(&sl);
	for (TBMUInt64 i = 0; i < aIter; ++i)
		{
		slSync.Signal();
		sl.iSem.Wait();
		TBMTicks now;
		::bmTimer.Stamp(&now);
		aResult->Cumulate(TBMTicksDelta(sl.SignalTime(), now));
		}
	child->WaitChildExit();
	sl.Close();
	slSync.Close();
	}

TInt Sync::SemaphoreLatencyChild(TAny* ptr)
	{
	RSemaphore slSync;
	TInt r = slSync.OpenGlobal(_L("slSync"));
	BM_ERROR(r, r == KErrNone);	

	SemaphoreLatencyArgs* sl = (SemaphoreLatencyArgs*) ptr;
	sl->ChildOpen();
	for (TBMUInt64 i = 0; i < sl->iIterationCount; ++i)
		{
		slSync.Wait();
		TBMTicks sigTime;
		::bmTimer.Stamp(&sigTime);		
		sl->iSem.Signal();
		sl->ChildSignalTime(sigTime);
		}
	sl->ChildClose();
	slSync.Close();
	return KErrNone;
	}

class ThreadSemaphoreLatencyArgs : public TBMSpawnArgs
	{
public:
	
	TBMUInt64			iIterationCount;
	TBMTicks			iSignalTime;
	TRequestStatus		iStatus;
	TRequestStatus*		iStatusPtr;
	RMsgQueue<TBMTicks>	iSignalTimeQue;

	ThreadSemaphoreLatencyArgs(TInt aRemote, TBMUInt64 aIter);

	void ChildOpen();
	void ChildClose();

	TBMTicks SignalTime();
	void ChildSignalTime(TBMTicks);

	void Close();
	};

ThreadSemaphoreLatencyArgs::ThreadSemaphoreLatencyArgs(TInt aRemote, TBMUInt64 aIter) : 
	TBMSpawnArgs(Sync::ThreadSemaphoreLatencyChild, KBMPriorityLow, aRemote, sizeof(*this)),
	iIterationCount(aIter),
	iStatusPtr(&iStatus)

	{
	TInt r = iSignalTimeQue.CreateGlobal(_L("BM Queue"), 1);
	BM_ERROR(r, r == KErrNone);
	}

void ThreadSemaphoreLatencyArgs::ChildOpen()
	{
	if (iRemote)
		{
		TInt r = iSignalTimeQue.OpenGlobal(_L("BM Queue"));
		BM_ERROR(r, r == KErrNone);
		}
	}

void ThreadSemaphoreLatencyArgs::ChildSignalTime(TBMTicks aTime)
	{
	TInt r = iSignalTimeQue.Send(aTime);
	BM_ERROR(r, r == KErrNone);
	}

TBMTicks ThreadSemaphoreLatencyArgs::SignalTime()
	{
	TBMTicks time;
	iSignalTimeQue.ReceiveBlocking(time);
	return time;
	}

void ThreadSemaphoreLatencyArgs::ChildClose()
	{
	if (iRemote)
		{
		iSignalTimeQue.Close();
		}
	}

void ThreadSemaphoreLatencyArgs::Close()
	{
	iSignalTimeQue.Close();
	}

void Sync::ThreadSemaphoreLatencyParent(TBMResult* aResult, TBMUInt64 aIter, TBool aRemote)
	{
	RSemaphore tslSync;
	TInt r = tslSync.CreateGlobal(_L("tslSync"), 0);
	BM_ERROR(r, r == KErrNone);	

	ThreadSemaphoreLatencyArgs sl(aRemote, aIter);
	MBMChild* child = sync.SpawnChild(&sl);
	for (TBMUInt64 i = 0; i < aIter; ++i)
		{
		sl.iStatus = KRequestPending;
		tslSync.Signal();
		User::WaitForRequest(sl.iStatus);
		BM_ASSERT(sl.iStatus == KErrNone);
		TBMTicks now;
		::bmTimer.Stamp(&now);
		aResult->Cumulate(TBMTicksDelta(sl.SignalTime(), now));
		}
	child->WaitChildExit();
	sl.Close();
	tslSync.Close();
	}

TInt Sync::ThreadSemaphoreLatencyChild(TAny* ptr)
	{
	RSemaphore tslSync;
	TInt r = tslSync.OpenGlobal(_L("tslSync"));
	BM_ERROR(r, r == KErrNone);	

	ThreadSemaphoreLatencyArgs* sl = (ThreadSemaphoreLatencyArgs*) ptr;
	sl->ChildOpen();
	for (TBMUInt64 i = 0; i < sl->iIterationCount; ++i)
		{
		tslSync.Wait();
		TRequestStatus* sptr = sl->iStatusPtr;
		TBMTicks sigTime;
		::bmTimer.Stamp(&sigTime);		
		sl->iParent.RequestComplete(sptr, KErrNone);
		sl->ChildSignalTime(sigTime);
		}
	sl->ChildClose();
	tslSync.Close();
	return KErrNone;
	}

						
TBMResult* Sync::Run(TBMUInt64 aIter, TInt* aCount)
	{
	TInt count = sizeof(iResults)/sizeof(iResults[0]);

	for (TInt i = 0; i < count; ++i)
		{
		iResults[i].Reset(iMeasurements[i].iName);
		iMeasurements[i].iFunc(&iResults[i], aIter, iMeasurements[i].iRemote);
		iResults[i].Update();
		}
	
	*aCount = count;
	return iResults;
	}

void AddSync()
	{
	BMProgram* next = bmSuite;
	bmSuite=(BMProgram*)&sync;
	bmSuite->Next()=next;
	}
