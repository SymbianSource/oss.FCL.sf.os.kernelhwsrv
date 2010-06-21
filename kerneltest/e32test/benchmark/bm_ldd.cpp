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

#include "k32bm.h"

const TUint8 KMutexOrder = 0xf0;

class DBMLDevice : public DLogicalDevice
	{
public:
	DBMLDevice();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DBMLChannel : public DLogicalChannelBase, public MBMIsr, public MBMInterruptLatencyIsr
	{
public:
	DBMLChannel();
	~DBMLChannel();

	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);

	DBMPChannel* PChannel() { return (DBMPChannel*) iPdd; }

private:
	static const TInt	KBMDfcQThreadPriority;	
	static const TInt	KBMKernelThreadPriority;	

	static void Dfc(TAny*);

	virtual void Isr(TBMTicks now);

	TInt (DBMLChannel::*iRequestInterrupt)();	// Measurement specific RBMChannel::RequestInterrupt() implmentation 
	TInt RequestInterrupt();					// Default iRequestInterrupt() implementation

	TBMTicks (DBMLChannel::*iResult)();			// Measurement specific RBMChannel::Result() implmentation
	TBMTicks Result();							// Default iResult() implementation

	TInt Start(RBMChannel::TMode);

	TInt StartInterruptLatency();
	virtual void InterruptLatencyIsr(TBMTicks latency);

	TInt StartKernelPreemptionLatency();
	static TInt KernelPreemptionLatencyThreadEntry(TAny* ptr);
	void KernelPreemptionLatencyThread();

	TInt StartUserPreemptionLatency();
	TBMTicks UserPreemptionLatencyResult();		// iResult() implementation

	TInt StartNTimerJitter();
	TInt RequestNTimerJitterInterrupt();		// iRequestInterrupt() implementation
	static void NTimerJitterCallBack(TAny*);

	TInt StartTimerStampOverhead();
	TInt RequestTimerStampOverhead();			// iRequestInterrupt() implementation

	TInt SetAbsPriority(TInt aThreadHandle, TInt aNewPrio, TInt* aOldPrio);

	DMutex*			iLock;	// Shall be acquired by anyone who access the object's writable state.

	TBool			iStarted;					// ETrue when a particular sequence of measurements has been started
	TBool			iPendingInterruptRequest;	// ETrue when an interrupt has been requested

	TDynamicDfcQue*	iDfcQ;
	TDfc			iDfc;

	DThread*		iKernelThread;		// the kernel thread created by some benchmarks
	DThread*		iUserThread;		// the user-side thread
	DThread*		iInterruptThread;	// the thread signaled by DFC; if non-NULL either iKernelThread or iUserThread

	NTimer			iNTimer;			// the timer used in "NTimer jitter" benchmark
	TBMTicks		iOneNTimerTick;		// number of high-resolution timer ticks in one NKern tick.
	TInt			iNTimerShotCount;	// used in "NTimer jitter" to distinguish between the first and the second shots 

	TBMTicks		iTime;
	TBMTicks		iTimerPeriod;		// period of high-resolution timer in ticks

	NFastSemaphore*	iKernelThreadExitSemaphore;

	void Lock()
		{
		NKern::ThreadEnterCS();
		Kern::MutexWait(*iLock);
		}
	void Unlock()
		{
		Kern::MutexSignal(*iLock);
		NKern::ThreadLeaveCS();
		}
	
	TBMTicks Delta(TBMTicks aT0, TBMTicks aT1)
		{
		return (aT0 <= aT1) ? (aT1 - aT0) : iTimerPeriod - (aT0 - aT1);
		}
	};

_LIT(KBMLChannelLit, "BMLChannel");

const TInt	DBMLChannel::KBMDfcQThreadPriority = KBMLDDHighPriority;
const TInt	DBMLChannel::KBMKernelThreadPriority = KBMLDDMidPriority;



DECLARE_STANDARD_LDD()
//
// Create a new device
//
	{
	__ASSERT_CRITICAL;
	return new DBMLDevice;
	}

DBMLDevice::DBMLDevice()
//
// Constructor
//
	{
	//iUnitsMask=0;
	iVersion = TVersion(1,0,1);
	iParseMask = KDeviceAllowPhysicalDevice;
	}

TInt DBMLDevice::Install()
//
// Install the device driver.
//
	{
	TInt r = SetName(&KBMLdName);
	return r;
	}

void DBMLDevice::GetCaps(TDes8&) const
//
// Return the Comm capabilities.
//
	{
	}

TInt DBMLDevice::Create(DLogicalChannelBase*& aChannel)
//
// Create a channel on the device.
//
	{
	__ASSERT_CRITICAL;
	aChannel = new DBMLChannel;
	return aChannel ? KErrNone : KErrNoMemory;
	}

DBMLChannel::DBMLChannel() : 
		iDfc(0, this, 0, 0),
		iNTimer(NULL, this)
	{
	// iDfcQueue = NULL;
	// iStarted = EFalse;
	// iPendingInterruptRequest = EFalse;
	// iKernelThread = NULL;
	}

TInt DBMLChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /* aInfo*/ , const TVersion& aVer)
//
// Create the channel from the passed info.
//
	{
	__ASSERT_CRITICAL;
	if (!Kern::QueryVersionSupported(TVersion(1,0,1),aVer))
		return KErrNotSupported;
	TInt r = Kern::MutexCreate(iLock, KBMLChannelLit, KMutexOrder);
	if (r != KErrNone)
		{
		return r;
		}
	iTimerPeriod = PChannel()->TimerPeriod();
		// Calculate the number of high-resolution timer ticks in one NKern tick
		// deviding the number of high-resolution timer ticks in one second by the
		// number of NKern ticks in one second.
		//
	iOneNTimerTick = PChannel()->TimerNsToTicks(BMSecondsToNs(1))/NKern::TimerTicks(1000);
	return KErrNone;
	}

DBMLChannel::~DBMLChannel()
// Called on a channel close. Note that if the PDD channel create failed 
// then the DoCreate() call will not have been made so don't assume anything 
// about non-ctor initialisation of members.
	{
	if (iLock) 
		iLock->Close(0);

	if (iPendingInterruptRequest)
		{
		PChannel()->CancelInterrupt();
		iDfc.Cancel();
		}

	if (iDfcQ)
		{
		iDfcQ->Destroy();
		}

	if (iKernelThread)
		{
		NFastSemaphore exitSemaphore;
		exitSemaphore.iOwningThread = NKern::CurrentThread();
		iKernelThreadExitSemaphore = &exitSemaphore;
		NKern::ThreadRequestSignal(&iKernelThread->iNThread);
		NKern::FSWait(&exitSemaphore);
		}
	}

void DBMLChannel::Dfc(TAny* ptr)
	{	
	DBMLChannel* lCh = (DBMLChannel*) ptr;
	BM_ASSERT(lCh->iPendingInterruptRequest);
	BM_ASSERT(lCh->iInterruptThread);
	NKern::ThreadRequestSignal(&lCh->iInterruptThread->iNThread);
	lCh->iPendingInterruptRequest = EFalse;
	}

//
// Default DBMLChannel::iRequestInterrupt implementation
//
TInt DBMLChannel::RequestInterrupt()
	{
	if (!iStarted)
		{
		return KErrNotReady;
		}
	if (iPendingInterruptRequest) 
		{
		return KErrInUse;
		}
	iPendingInterruptRequest = ETrue;
	PChannel()->RequestInterrupt();
	return KErrNone;
	}

//
// Default DBMLChannel::iResult implementation
//
TBMTicks DBMLChannel::Result()
	{
	return iTime;
	}

void DBMLChannel::Isr(TBMTicks aNow)
	{
	//
	// Store the ISR entry time and queue a DFC.
	//
	iTime = aNow;
	iDfc.Add();
	}

//
// "INTERRUPT LATENCY"
// 
// SCENARIO:
//
//		A user thread requests an interrupt (RBMChannel::RequestInterrupt()) and waits at User::WaitForAnyRequest()
//		(RBMChannel::Result()).
//		When the interrupt occurs DBMLChannel::InterruptLatencyIsr() stores the interrupt latency 
//		provided by LDD, in DBMLChannel::iTime and queues a DFC (DBMLChannel::iDfc, DBMLChannel::Dfc())
//		which in its turn signals the user thread.
//

TInt DBMLChannel::StartInterruptLatency()
	{
	if (iStarted)
		{
		return KErrInUse;
		}
	TInt r = PChannel()->BindInterrupt((MBMInterruptLatencyIsr*) this);
	if (r != KErrNone)
		{
		return r;
		}
		// Use the default iRequestInterrupt() implmentation
	iRequestInterrupt = &DBMLChannel::RequestInterrupt;
		// Use the default iResult() implmentation
	iResult = &DBMLChannel::Result;
	iInterruptThread = &Kern::CurrentThread();
	iStarted = ETrue;
	return KErrNone;
	}

void DBMLChannel::InterruptLatencyIsr(TBMTicks aLatency)
	{
	iTime = aLatency;
	iDfc.Add();
	}

//
// "KERNEL THREAD PREEMPTION LATENCY"
// 
// SCENARIO:
//
//		A new kernel thread is created at the beginning of a sequence of measurements 
//		(DBMLChannel::StartKernelPreemptionLatency()). The kernel thread waits at Kern::WaitForAnyRequest()
//		(DBMLChannel::KernelPreemptionLatencyThread()).
//		The user thread requests an interrupt (RBMChannel::RequestInterrupt()) and waits at User::WaitForAnyRequest()
//		(RBMChannel::Result()).
//		When the interrupt occurs DBMLChannel::Isr() stores the ISR entry time, provided by LDD 
//		in DBMLChannel::iTime and queues a DFC (DBMLChannel::iDfc, DBMLChannel::Dfc()) which, in its turn,
//		signals the kernel thread.
//		The kernel thread, when awaken, calculates the latency as the difference between the ISR entry time 
//		and the current time and finally signals the user thread.
//

TInt DBMLChannel::StartKernelPreemptionLatency()
	{
	if (iStarted)
		{
		return KErrInUse;
		}
	TInt r = PChannel()->BindInterrupt((MBMIsr*) this);
	if (r != KErrNone)
		{
		return r;
		}
	{
	SThreadCreateInfo info;
	info.iType = EThreadSupervisor;
	info.iFunction = DBMLChannel::KernelPreemptionLatencyThreadEntry;
	info.iPtr = this;
	info.iSupervisorStack = NULL;
	info.iSupervisorStackSize = 0;
	info.iInitialThreadPriority = KBMKernelThreadPriority;
	info.iName.Set(KBMLChannelLit);
	info.iTotalSize = sizeof(info);
	r = Kern::ThreadCreate(info);
	if (r != KErrNone)
		{
		return r;
		}
	iKernelThread = (DThread*) info.iHandle;
	}

	iUserThread = &Kern::CurrentThread();
		// Use the default iRequestInterrupt() implmentation
	iRequestInterrupt = &DBMLChannel::RequestInterrupt;
		// Use the default iResult() implmentation
	iResult = &DBMLChannel::Result;
	iInterruptThread = iKernelThread;
	iStarted = ETrue;

	Kern::ThreadResume(*iKernelThread);

	return KErrNone;
	}

TInt DBMLChannel::KernelPreemptionLatencyThreadEntry(TAny* ptr)
	{
	DBMLChannel* lCh = (DBMLChannel*) ptr;
	lCh->KernelPreemptionLatencyThread();
	BM_ASSERT(0);
	return 0;
	}

void DBMLChannel::KernelPreemptionLatencyThread()
	{
	for(;;)
		{
		NKern::WaitForAnyRequest();

		if(iKernelThreadExitSemaphore)
			break;

		TBMTicks now = PChannel()->TimerStamp();
		iTime = Delta(iTime, now);
		BM_ASSERT(iUserThread);
		NKern::ThreadRequestSignal(&iUserThread->iNThread);
		}

	NKern::FSSignal(iKernelThreadExitSemaphore);
	Kern::Exit(0); 
	}


//
// "USER THREAD PREEMPTION LATENCY"
// 
// SCENARIO:
//
//		A user thread requests an interrupt (RBMChannel::RequestInterrupt()) and waits at User::WaitForAnyRequest()
//		(RBMChannel::Result()).
//		When the interrupt occurs DBMLChannel::Isr() stores the ISR entry time provided by LDD, 
//		in DBMLChannel::iTime and queues a DFC (DBMLChannel::iDfc, DBMLChannel::Dfc()) which in its turn
//		signals the user thread.
//		The user thread, when awaken, immediately re-enters in the LDD, and calculates the latency as 
//		the difference between the ISR entry time and the current time.
//

TInt DBMLChannel::StartUserPreemptionLatency()
	{
	if (iStarted)
		{
		return KErrInUse;
		}
	TInt r = PChannel()->BindInterrupt((MBMIsr*) this);
	if (r != KErrNone)
		{
		return r;
		}
		// Default iRequestInterrupt() implmentation
	iRequestInterrupt = &DBMLChannel::RequestInterrupt;
	iResult = &DBMLChannel::UserPreemptionLatencyResult;
	iInterruptThread = &Kern::CurrentThread();
	iStarted = ETrue;
	return KErrNone;
	}

TBMTicks DBMLChannel::UserPreemptionLatencyResult()
	{
	TBMTicks now = PChannel()->TimerStamp();
	return Delta(iTime, now);
	}

//
// "NTimer PERIOD JITTER"
// 
// SCENARIO:
//
//		One measuremnt is done by two consecutive NTimer callbacks. 
//		The first callback stores the current time and the second one calculate the actual period as 
//		the difference between its own current time and the time stored by the first callback.
//		The difference between this actual period and the theoretical period is considered as the jitter.
//

TInt DBMLChannel::StartNTimerJitter()
	{
	if (iStarted)
		{
		return KErrInUse;
		}
	new (&iNTimer) NTimer(&NTimerJitterCallBack, this);
	iRequestInterrupt = &DBMLChannel::RequestNTimerJitterInterrupt;
		// Use the default iResult() implmentation
	iResult = &DBMLChannel::Result;
	iInterruptThread = &Kern::CurrentThread();
	iStarted = ETrue;
	return KErrNone;
	}

TInt DBMLChannel::RequestNTimerJitterInterrupt()
	{
	if (!iStarted)
		{
		return KErrNotReady;
		}
	if (iPendingInterruptRequest) 
		{
		return KErrInUse;
		}
	iPendingInterruptRequest = ETrue;
	iNTimerShotCount = 0;
	iNTimer.OneShot(1);
	return KErrNone;
	}


void DBMLChannel::NTimerJitterCallBack(TAny* ptr)
	{
	DBMLChannel* lCh = (DBMLChannel*) ptr;
	TBMTicks now = lCh->PChannel()->TimerStamp();
	if (lCh->iNTimerShotCount++ == 0)
		{
		//
		// This is the first callback: store the time and request another one.
		//
		lCh->iTime = now;
		lCh->iNTimer.Again(1);
		}
	else 
		{
		//
		// This is the second callback: measure the jitter and schedule a DFC
		// which in its turn will signal the user thread.
		//
		lCh->iTime = lCh->Delta(lCh->iTime, now);
		lCh->iDfc.Add();
		}
	}

//
// "TIMER OVERHEAD"
// 
// SCENARIO:
//		To measure the overhead of the high-precision timer read operation we get
//		two consecutive timestamps through DBMPChannel::TimerStamp() interface.
//		The difference beween this two values is considered as the measured overhead.
//

TInt DBMLChannel::StartTimerStampOverhead()
	{
	if (iStarted)
		{
		return KErrInUse;
		}
	iRequestInterrupt = &DBMLChannel::RequestTimerStampOverhead;
		// Use the default iResult() implmentation
	iResult = &DBMLChannel::Result;
	iInterruptThread = &Kern::CurrentThread();
	iStarted = ETrue;
	return KErrNone;
	}

TInt DBMLChannel::RequestTimerStampOverhead()
	{
	TBMTicks t1 = PChannel()->TimerStamp();
	TBMTicks t2 = PChannel()->TimerStamp();
	iTime = Delta(t1, t2);
	NKern::ThreadRequestSignal(&iInterruptThread->iNThread);
	return KErrNone;
	}
//
// END OF "GETTING TIMER OVERHEAD"
//

//
// The implementation of RBMDriver::SetAbsPriority() call.
//
TInt DBMLChannel::SetAbsPriority(TInt aThreadHandle, TInt aNewPrio, TInt* aOldPrio)
	{
	NKern::LockSystem();
	//
	// Under the system lock find the DThread object and increment its ref-count (i.e Open()) 
	//
	DThread* thr = (DThread*) Kern::ObjectFromHandle(&Kern::CurrentThread(), aThreadHandle, EThread);
	TInt r;
	if (!thr)
		{
		r = EBadHandle;
		}
	else
		{
		r = thr->Open();
		}
	//
	// Now it's safe to release the system lock and to work with the object.
	//
	NKern::ThreadEnterCS();
	NKern::UnlockSystem();
	if (r != KErrNone)
		{
		NKern::ThreadLeaveCS();
		return r;
		}
	*aOldPrio = thr->iDefaultPriority;
	Kern::SetThreadPriority(aNewPrio, thr);
	//
	// Work is done - close the object.
	//	
	thr->Close(NULL);
	NKern::ThreadLeaveCS();
	return KErrNone;
	}

_LIT(KBmDfcQName, "BmDfcQ");

//
// Starts a new sequence of measurements.
//
// Only one sequence can be started for any particular DBMLChannel object during its life. 
// If more than one sequence is required a new DBMLChannel object must be created.
//
TInt DBMLChannel::Start(RBMChannel::TMode aMode)
	{
	TInt r;
	if (iDfcQ == NULL)
		{
		r = Kern::DynamicDfcQCreate(iDfcQ, KBMDfcQThreadPriority, KBmDfcQName);
		if (r != KErrNone)
			return r;

		iDfc.SetDfcQ(iDfcQ);
		iDfc.SetFunction(Dfc);
		}

	switch (aMode)
		{
	case RBMChannel::EInterruptLatency:
		r = StartInterruptLatency();
		break;
	case RBMChannel::EKernelPreemptionLatency:
		r = StartKernelPreemptionLatency();
		break;
	case RBMChannel::EUserPreemptionLatency:
		r = StartUserPreemptionLatency();
		break;
	case RBMChannel::ENTimerJitter:
		r = StartNTimerJitter();
		break;
	case RBMChannel::ETimerStampOverhead:
		r = StartTimerStampOverhead();
		break;
	default:
		r = KErrNotSupported;
		break;
		}

	return r;
	}

//
// Client requests.
//
TInt DBMLChannel::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r = KErrNone;
	switch (aFunction)
		{
		case RBMChannel::EStart:
			{
			RBMChannel::TMode mode = (RBMChannel::TMode) (TInt) a1;
			Lock();
			r = Start(mode);
			Unlock();
			break;
			}
		case RBMChannel::ERequestInterrupt:
			{
			Lock();
			r = (this->*iRequestInterrupt)();
			Unlock();
			break;
			}
		case RBMChannel::EResult:
			{
			//
			// We don't acquire the lock because:
			//	(1) iResult() typically reads iTime which was written BEFORE to signal the current thread 
			//      and therefore BEFORE the current thread comes here. 
			//  (2) we really want if possible (i.e. correct!) to avoid the lock acquisition because it can
			//		increase the measurement overhead in the case when we are in a measured path (e.g. user
			//		preemption latency benchmark).
			//
			TBMTicks ticks = (this->*iResult)();
			umemput(a1, &ticks, sizeof(ticks));
			break;
			}
		//
		// All below requests do not access writable DBMChannel state and therefore do not require the lock
		//
		case RBMChannel::ETimerStamp:
			{
			TBMTicks ticks = PChannel()->TimerStamp();
			umemput(a1, &ticks, sizeof(ticks));
			break;
			}
		case RBMChannel::ETimerPeriod:
			{
			TBMTicks ticks = iTimerPeriod;
			umemput(a1, &ticks, sizeof(ticks));
			break;
			}
		case RBMChannel::ETimerTicksToNs:
			{
			TBMTicks ticks;
			umemget(&ticks, a1, sizeof(ticks));
			TBMNs ns = PChannel()->TimerTicksToNs(ticks);
			umemput(a2, &ns, sizeof(ns));
			break;
			}
		case RBMChannel::ETimerNsToTicks:
			{
			TBMNs ns;
			umemget(&ns, a1, sizeof(ns));
			TBMTicks ticks = PChannel()->TimerNsToTicks(ns);
			umemput(a2, &ticks, sizeof(ticks));
			break;
			}
		case RBMChannel::ESetAbsPriority:
			{
			TInt newPrio;
			TInt oldPrio;
			umemget(&newPrio, a2, sizeof(newPrio));
			r = SetAbsPriority((TInt) a1, newPrio, &oldPrio);
			umemput(a2, &oldPrio, sizeof(oldPrio));
			break;
			}
		default:
			r = KErrNotSupported;
			break;
		}
	return r;
	}


