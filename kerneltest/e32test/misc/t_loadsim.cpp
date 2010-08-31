// Copyright (c) 2009-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\t_loadsim.cpp
// 
//


//-------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_loadsim-2705
//! @SYMTestCaseDesc			verifying the behaviour of the load balancer
//! @SYMPREQ					417-52765/417-58889
//! @SYMTestPriority			Critical
//! @SYMTestActions				
//! 1. This test runs a variety of loads on an SMP system. Loads types are:
//!    1.1 Cpu intensive loads
//!    1.2 memory intensive loads (high locality)
//!    1.3 memory intensive loads (low locality)
//!    1.4 memory intensive loads with atomic operations
//!    1.5 cpu intensive loads with some serialization
//! 2. For each test, the load is first run on a single cpu locked thread as a baseline
//!    benchmark. Then the tests are run in the following configurations:
//!    2.1 For n = 1 to 2*Number of cpus do a run with i threads.
//!    2.2 For h = 1 to NumCpus ; For n = h to 2*NumCpus; run with h high priorty threads and 
//!         n standard priority threads, with high priority threads cpu locked.
//!    2.3 For h = 1 to NumCpus ; For n = h to 2*NumCpus; run with h high priorty threads and 
//!         n standard priority threads.
//! @SYMTestExpectedResults 
//!     test passed. TTest is manual:
//! 1. For each test we expect to see that the amount of CPU time obtained by each CPU is 
//!     balanced. That is, all standard priority threads get roughly same amount of CPU time
//!     and all high priority threads get roughly same amount of CPU time and a higher value
//!     than lower priority threads.
//! 2. We also expect the relative efficiency reported by the test between the benchmark
//!    and each test run to be >=95% on average. Values well below this are acceptable in 
//!    test runs involving atomic operations (1.4)

//-------------------------------------------------------------------------------------------

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32base.h>
#include <hal.h>
#include <e32atomics.h>
#include <u32hal.h>
#include <e32svr.h>

//#define	TRACE(x)	x
#define	TRACE(x)

void Panic(TInt aLine)
	{
	User::Panic(_L("T_LOADSIM"),aLine);
	}

#define	assert(x)		((void)((x)||(Panic(__LINE__),0)))

RTest test(_L("T_LOADSIM"));

const TInt KErrCouldNotStart = -99;

volatile TInt STFU = 1;

/******************************************************************************
 * Random Number Generation
 ******************************************************************************/
void LFSR(TUint64& a)
	{
	TInt i;
	for (i=64; i>0; --i)
		{
		TUint64 x = a<<1;
		TUint64 y = x<<1;
		x^=y;
		a = (y>>1) | (x>>63);
		}
	}

// Returns 256*log2(a/2^64)
TInt Log2(TUint64 a)
	{
	const TUint64 KBit63 = UI64LIT(0x8000000000000000);
	TInt n = __e32_find_ms1_64(a);
	a <<= (63-n);
	n -= 64;
	TInt i;
	for (i=0; i<8; ++i)
		{
		a >>= 32;
		a *= a;
		n <<= 1;
		if (a & KBit63)
			{
			++n;
			}
		else
			{
			a <<= 1;
			}
		}
	return n;
	}

TUint32 ExpRV(TUint64 aU, TUint32 aMean, TUint32 aTick)
	{
	TInt n = -Log2(aU);
	TUint64 x = TUint64(n) * TUint64(aMean);
	x *= TUint64(22713);	// 2^15 * ln2
	TUint64 p(aTick);
	p <<= 22;
	x += p;
	p += p;
	x /= p;
	return I64LOW(x);
	}



/******************************************************************************
 * Generic High-Resolution Timing
 ******************************************************************************/
class TTimestamp
	{
public:
	typedef void (*TSampleFunc)(TAny*);
public:
	void Sample();
	void Sample(TSampleFunc aFunc, TAny* aPtr);
	TInt64 operator-(const TTimestamp&) const;
	static void Init();
private:
	TUint32		iF;		// User::FastCounter() value
	TUint32		iN;		// User::NTickCount() value
private:
	static TUint32 FF;	// User::FastCounter() frequency
	static TUint32 NP;	// User::NTickCount() period
	static TBool FU;	// User::FastCounter() counts up
	static TUint32 FWrapM;	// Number of nanokernel ticks for FastCounter() to wrap / 2 * 2^FWrapS
	static TInt FWrapS;	// Shift so that 2^31<=FWrapM<2^32
	};

TUint32	TTimestamp::FF;
TUint32	TTimestamp::NP;
TBool	TTimestamp::FU;
TUint32	TTimestamp::FWrapM;
TInt	TTimestamp::FWrapS;


void TTimestamp::Sample()
	{
	TUint32 n = User::NTickCount();
	do	{
		iN = n;
		iF = User::FastCounter();
		n = User::NTickCount();
		} while (n!=iN);
	}

void TTimestamp::Sample(TSampleFunc aFunc, TAny* aPtr)
	{
	TUint32 n = User::NTickCount();
	do	{
		iN = n;
		(*aFunc)(aPtr);
		iF = User::FastCounter();
		n = User::NTickCount();
		} while (n!=iN);
	}

// return (x*a)/b
TUint64 scale(TUint64 x, TUint32 a, TUint32 b)
	{
	TUint64 mask = KMaxTUint32;
	TUint64 x0 = x & mask;
	TUint64 x1 = x >> 32;
	x0 *= TUint64(a);
	x1 *= TUint64(a);
	x1 += (x0 >> 32);
	x0 &= mask;
	TUint64	q1 = x1 / TUint64(b);
	TUint64 q0 = x1 - q1*TUint64(b);
	q0 <<= 32;
	q0 |= x0;
	q0 /= TUint64(b);
	return (q1<<32)|q0;
	}


// Return difference between a and this in microseconds
TInt64 TTimestamp::operator-(const TTimestamp& a) const
	{
	TInt sign = 1;
	TTimestamp start;
	TTimestamp end;
	if (iN-a.iN >= 0x80000000u)
		{
		sign = -1;
		start = *this;
		end = a;
		}
	else
		{
		start = a;
		end = *this;
		}
	TUint32 fd32 = end.iF - start.iF;
	if (!FU)
		fd32 = ~fd32 + 1u;
	TUint64 nd = TUint64(end.iN) - TUint64(start.iN);
	nd <<= 31;	// 2^31 * difference in NTickCount
	TUint64 x = TUint64(fd32) * TUint64(FWrapM);
	x >>= FWrapS;	// ftick difference * (FWrapM/2^FWrapS) = 2^31 * ntick difference
	nd -= x;	// Should now be a multiple of 2^31N where N=2^32*ftickp/ntickp
				// i.e. should be a multiple of 2^63*ftickp/ntickp

	// FWrapM = 2^(31+FWrapS)*ftickp/ntickp
	// FWrapM << (32-FWrapS) = 2^63*ftickp/ntickp
	TUint64 m = TUint64(FWrapM) << (32-FWrapS);

	nd += (m>>1);
	nd /= m;

	nd = (nd<<32) + TUint64(fd32);	// final result in fast counter ticks
	TInt64 r = scale(nd, 1000000, FF);	// convert to microseconds
	if (sign<0)
		r = -r;
	return r;
	}

void TTimestamp::Init()
	{
	TInt r;
	r = HAL::Get(HAL::ENanoTickPeriod, (TInt&)NP);
	assert(r==KErrNone);
	r = HAL::Get(HAL::EFastCounterFrequency, (TInt&)FF);
	assert(r==KErrNone);
	r = HAL::Get(HAL::EFastCounterCountsUp, (TInt&)FU);
	assert(r==KErrNone);
	TReal fpn = TReal(FF) * TReal(NP) / 1000000.0;	// fast counter ticks per NTick
	TReal fwrap = 2147483648.0 / fpn;	// NTicks between fast counter wraparounds / 2
	TInt exp = 0;
	while (fwrap < 2147483648.0)
		{
		fwrap *= 2.0;
		++exp;
		}
	fwrap += 0.5;
	if (fwrap >= 4294967296.0)
		{
		fwrap *= 0.5;
		--exp;
		}
	FWrapM = (TUint32)fwrap;
	FWrapS = exp;	// NTicks for 2^31 fast ticks = FWrapM/2^FWrapS

	test.Printf(_L("FastCounter frequency  %uHz\n"), FF);
	if (FU)
		test.Printf(_L("FastCounter counts     UP\n"));
	else
		test.Printf(_L("FastCounter counts     DOWN\n"));
	test.Printf(_L("Nanokernel tick period %uus\n"), NP);
	test.Printf(_L("FWrapM                 %08x\n"), FWrapM);
	test.Printf(_L("FWrapS                 %d\n"), FWrapS);
	}

/******************************************************************************
 * CPU Usage Measurement
 ******************************************************************************/
class TThreadCpuUsageSample
	{
public:
	void Sample(RThread aThread);
	TInt64 ElapsedTimeDelta(const TThreadCpuUsageSample& aStart) const;
	TInt64 CpuTimeDelta(const TThreadCpuUsageSample& aStart) const;
private:
	static void SampleThreadCpuTime(TAny* aPtr);
private:
	TTimestamp	iElapsedTime;
	TInt64		iCpuTime;
	RThread		iThread;
	};

void TThreadCpuUsageSample::Sample(RThread aThread)
	{
	iThread = aThread;
	iElapsedTime.Sample(&SampleThreadCpuTime, this);
	}

void TThreadCpuUsageSample::SampleThreadCpuTime(TAny* aPtr)
	{
	TThreadCpuUsageSample& me = *(TThreadCpuUsageSample*)aPtr;
	TTimeIntervalMicroSeconds& rt = *(TTimeIntervalMicroSeconds*)&me.iCpuTime;
	assert(me.iThread.GetCpuTime(rt) == KErrNone);
	}

TInt64 TThreadCpuUsageSample::ElapsedTimeDelta(const TThreadCpuUsageSample& aStart) const
	{
	return iElapsedTime - aStart.iElapsedTime;
	}

TInt64 TThreadCpuUsageSample::CpuTimeDelta(const TThreadCpuUsageSample& aStart) const
	{
	return iCpuTime - aStart.iCpuTime;
	}

class TCpuUsage
	{
public:
	enum {EMaxCpus=8};
public:
	void Sample();
	TInt64 ElapsedTimeDelta(const TCpuUsage& aStart) const;
	TInt64 CpuTimeDelta(const TCpuUsage& aStart, TInt aCpu) const;
	static TInt N() { return NumberOfCpus; }
public:
	static void Init();
private:
	static void SampleIdleTimes(TAny* aPtr);
private:
	TTimestamp	iElapsedTime;
	TInt64		iIdleTime[EMaxCpus];
private:
	static TInt NumberOfCpus;
	static RThread IdleThread[EMaxCpus];
	};

TInt TCpuUsage::NumberOfCpus = -1;
RThread TCpuUsage::IdleThread[TCpuUsage::EMaxCpus];

void TCpuUsage::Init()
	{
	TTimestamp::Init();

	NumberOfCpus = UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0);
	test.Printf(_L("NumberOfCpus = %d\n"), NumberOfCpus);
	assert(NumberOfCpus > 0);
	assert(NumberOfCpus <= EMaxCpus);

	TTimeIntervalMicroSeconds ms;
	TInt r;
	r = RThread().GetCpuTime(ms);
	if (r != KErrNone)
		{
		test.Printf(_L("RThread::GetCpuTime() returned %d\n"), r);
		test.Printf(_L("This test requires a working RThread::GetCpuTime() to run\n"));
		test(0);
		}

	TFullName kname;
	_LIT(KLitKernelName, "ekern.exe*");
	_LIT(KLitNull, "::Null");
	TFindProcess fp(KLitKernelName);
	test_KErrNone(fp.Next(kname));
	test.Printf(_L("Found kernel process: %S\n"), &kname);
	kname.Append(KLitNull);
	TInt i;
	for (i=0; i<NumberOfCpus; ++i)
		{
		TFullName tname(kname);
		TFullName tname2;
		if (i>0)
			tname.AppendNum(i);
		TFindThread ft(tname);
		test_KErrNone(ft.Next(tname2));
		TInt r = IdleThread[i].Open(ft);
		test_KErrNone(r);
		IdleThread[i].FullName(tname2);
		test.Printf(_L("Found and opened %S\n"), &tname2);
		}
	}

void TCpuUsage::Sample()
	{
	iElapsedTime.Sample(&SampleIdleTimes, this);
	}

void TCpuUsage::SampleIdleTimes(TAny* aPtr)
	{
	TCpuUsage& me = *(TCpuUsage*)aPtr;
	assert(NumberOfCpus > 0);
	TInt i;
	for (i=0; i<NumberOfCpus; ++i)
		assert(IdleThread[i].GetCpuTime((TTimeIntervalMicroSeconds&)me.iIdleTime[i]) == KErrNone);
	}

TInt64 TCpuUsage::ElapsedTimeDelta(const TCpuUsage& aStart) const
	{
	return iElapsedTime - aStart.iElapsedTime;
	}

TInt64 TCpuUsage::CpuTimeDelta(const TCpuUsage& aStart, TInt aCpu) const
	{
	assert(TUint(aCpu) < TUint(EMaxCpus));
	if (aCpu >= NumberOfCpus)
		return 0;
	TInt64 idle_time = iIdleTime[aCpu] - aStart.iIdleTime[aCpu];
	TInt64 elapsed_time = iElapsedTime - aStart.iElapsedTime;
	return elapsed_time - idle_time;
	}

 
 
/******************************************************************************
 * Generic CPU Consumer
 ******************************************************************************/
enum TCpuEaterType
	{
	EEaterStd				=0,		// do CPU-intensive work with few memory references
	EEaterMemoryLocalS		=1,		// do loads of memory references with reasonable locality, shared
	EEaterMemoryNonLocalS	=2,		// do loads of memory references with poor locality, shared
	EEaterMemoryLocalU		=3,		// do loads of memory references with reasonable locality, unshared
	EEaterMemoryNonLocalU	=4,		// do loads of memory references with poor locality, unshared
	EEaterMemoryAtomic		=5,		// do loads of atomic memory references
	EEaterMemoryAtomic2		=6,		// do loads of atomic memory references
	EEaterAmdahl			=7,		// do CPU-intensive work interspersed with serialized sections
	};

class CDefaultCpuEater;

class REaterArray;
class MCpuEater
	{
public:
	MCpuEater();
	virtual ~MCpuEater();
	virtual void Eat(TInt aTime, TUint32* aWorkDone)=0;
	virtual void Calibrate();
	inline TBool IsCalibrated() { return iCalibration!=0; }
protected:
	TUint32 WorkValue(TInt aTime);
	TUint32	iCalibration;	// work value for 2^16 microseconds
	TUint16	iInstance;
	TUint16	iType;

	friend class REaterArray;
	};

MCpuEater::MCpuEater()
	{
	iCalibration = 0;			// uncalibrated
	iInstance = KMaxTUint16;	// dummy value
	iType = KMaxTUint16;		// dummy value
	}

MCpuEater::~MCpuEater()
	{
	}

// Calibration is for 2^KLog2CalibrateTime microseconds
const TInt KLog2CalibrateTime = 13;

TUint32 MCpuEater::WorkValue(TInt aTime)
	{
	if (iCalibration == 0)
		return aTime;
	TUint64 x = TUint64(aTime) * TUint64(iCalibration);
	x >>= (KLog2CalibrateTime + 2);	// Factor of 4 margin for slowdowns
	TUint32 r = I64LOW(x);
	if (I64HIGH(x))
		r = KMaxTUint32;
	if (r == 0)
		return 1;
	if (r > iCalibration)
		return iCalibration;
	return r;
	}

void MCpuEater::Calibrate()
	{
	iCalibration = 0;
	TUint32 work = 1;
	TUint64 used = 1;
	TUint64 threshold = 1;
	threshold <<= KLog2CalibrateTime;
	while (work)
		{
		TThreadCpuUsageSample initial;
		TThreadCpuUsageSample final;
		initial.Sample(RThread());
		Eat(work, 0);
		final.Sample(RThread());
		used = final.CpuTimeDelta(initial);
		if (used >= threshold)
			break;
		work <<= 1;
		}
	assert(work > 0);
	TUint64 c(work);
	c <<= KLog2CalibrateTime;
	c /= used;
	if (I64HIGH(c))
		iCalibration = KMaxTUint32;
	else if (I64LOW(c))
		iCalibration = I64LOW(c);
	else
		iCalibration = 1;
	test.Printf(_L("MCpuEater::Calibrate() %u\n"), iCalibration);
	}


class REaterArray : public RPointerArray<MCpuEater>
	{
public:
	REaterArray();
	void Close();
	MCpuEater* Find(TInt aType, TInt aInstance);
	MCpuEater* FindOrCreateL(TInt aType, TInt aInstance);
private:
	MCpuEater* CreateLC(TInt aType);
private:
	class MDummy : public MCpuEater
		{
	public:
		MDummy(TInt aType, TInt aInstance)
			{ iType=TUint16(aType); iInstance=TUint16(aInstance); }
		virtual ~MDummy()
			{}
		virtual void Eat(TInt, TUint32*)
			{}
		};
private:
	static TBool Identity(const MCpuEater& aL, const MCpuEater& aR);
	static TInt Ordering(const MCpuEater& aL, const MCpuEater& aR);
	};

REaterArray::REaterArray()
	:	RPointerArray<MCpuEater>(8, 2*256)
	{
	}

void REaterArray::Close()
	{
	ResetAndDestroy();
	}

TBool REaterArray::Identity(const MCpuEater& aL, const MCpuEater& aR)
	{
	return (aL.iType==aR.iType && aL.iInstance==aR.iInstance);
	}

TInt REaterArray::Ordering(const MCpuEater& aL, const MCpuEater& aR)
	{
	if (aL.iType > aR.iType)
		return 1;
	if (aL.iType < aR.iType)
		return -1;
	if (aL.iInstance > aR.iInstance)
		return 1;
	if (aL.iInstance < aR.iInstance)
		return -1;
	return 0;
	}

MCpuEater* REaterArray::Find(TInt aType, TInt aInstance)
	{
	MDummy search(aType, aInstance);
	TInt ix = FindInOrder(&search, &Ordering);
	if (ix < 0)
		return 0;
	return (*this)[ix];
	}

MCpuEater* REaterArray::FindOrCreateL(TInt aType, TInt aInstance)
	{
	MCpuEater* p = Find(aType, aInstance);
	if (p)
		return p;
	p = CreateLC(aType);
	p->iType = TUint16(aType);
	p->iInstance = TUint16(aInstance);
	InsertInOrderL(p, &Ordering);
	CleanupStack::Pop();
	return p;
	}

/******************************************************************************
 * Generic zero-drift timed events
 ******************************************************************************/
class CLoadSim;
class MEvent
	{
public:
	MEvent(CLoadSim*, TInt);
	virtual void Start()=0;
	virtual ~MEvent();
	inline TBool Queued() const
		{ return iQueued; }
protected:
	void QueueAt(TUint32 aTime);
	void QueueAfter(TUint32 aInterval);
	void Dequeue();
	virtual TInt Event();
	inline TUint64 Random();
protected:
	TUint8		iId;
	TUint8		iQueued;
	TUint8		iE1;
	TUint8		iE2;
	MEvent*		iChain;
	CLoadSim*	iT;
	TUint32		iNextEventTime;
	friend class CLoadSim;
	};

class CLoadSim : public CActive
	{
public:
	static CLoadSim* NewL();
	~CLoadSim();
	inline TInt TimerPeriod() const
		{ return iTimerPeriod; }
	TUint64 Random();
private:
	CLoadSim();
	virtual void RunL();
	virtual void DoCancel();
	void StartTimer();
private:
	RTimer		iTimer;
	TUint64		iSeed;
	MEvent*		iNextEvent;
	TUint32		iIterations;
	TUint32		iLastTrigger;		// Last trigger time in ticks
	TInt		iCarry;
	TInt		iTimerPeriod;		// Timer tick period in microseconds
	TInt		iMaxDelta;
	TUint8		iInRunL;
	TUint8		iTimerRunning;
	TUint8		iTimerInit;
	TUint8		iOffsetInit;
	TUint32		iOffset;
private:
	friend class MEvent;
	};

inline TUint64 MEvent::Random()
	{ return iT->Random(); }

CLoadSim::CLoadSim()
	: CActive(EPriorityStandard)
	{
	iSeed = 0xadf85458;
	assert(HAL::Get(HAL::ENanoTickPeriod, iTimerPeriod)==KErrNone);
	iMaxDelta = KMaxTInt / (2*iTimerPeriod);
	}

CLoadSim::~CLoadSim()
	{
	Cancel();
	iTimer.Close();
	}

CLoadSim* CLoadSim::NewL()
	{
	CLoadSim* p = new (ELeave) CLoadSim();
	CleanupStack::PushL(p);
	User::LeaveIfError(p->iTimer.CreateLocal());
	CleanupStack::Pop();
	return p;
	}

void CLoadSim::DoCancel()
	{
	iTimer.Cancel();
	iTimerRunning = 0;
	}

void CLoadSim::RunL()
	{
	TRACE(RDebug::Printf("!%d\n", iStatus.Int()));
	iTimerRunning = 0;
	iInRunL = 1;
	TUint32 now = iLastTrigger;
	if (iStatus == KErrNone)
		{
		now += iCarry;
		iLastTrigger = now;
		}
	else if (iStatus == KErrArgument)
		{
		now += iCarry;
		}
	else if (iStatus == KErrCancel)
		{
		iLastTrigger += iCarry;	// trigger time was still updated
		}
	iCarry = 0;
	MEvent* e = 0;
	FOREVER
		{
		++iIterations;
		e = iNextEvent;
		if (!e || e->iNextEventTime>now)
			break;
		iNextEvent = e->iChain;
		e->iChain = 0;
		e->iQueued = 0;
		e->Event();
		}
	if (e)
		{
		TInt delta = TInt(e->iNextEventTime - iLastTrigger);
		if (delta > iMaxDelta)
			delta = iMaxDelta;
		if (delta < -iMaxDelta)
			delta = -iMaxDelta;
		iCarry = delta;
		TInt us = delta * iTimerPeriod;
		TRACE(RDebug::Printf("T+%d\n", us));
		iTimer.AgainHighRes(iStatus, us);
		SetActive();
		iTimerRunning = 1;
		}
	iInRunL = 0;
	}

void CLoadSim::StartTimer()
	{
	if (iInRunL)
		return;
	if (iTimerRunning)
		{
		TRACE(RDebug::Printf("TC\n"));
		iTimer.Cancel();	// will cause RunL with KErrCancel which will restart timer
		return;
		}
	TInt delta = TInt(iNextEvent->iNextEventTime - iLastTrigger);
	if (delta > iMaxDelta)
		delta = iMaxDelta;
	if (delta < -iMaxDelta)
		delta = -iMaxDelta;
	iCarry = delta;
	TInt us = delta * iTimerPeriod;
	if (iTimerInit)
		{
		TRACE(RDebug::Printf("sT+%d\n", us));
		iTimer.AgainHighRes(iStatus, us);
		}
	else
		{
		if (!iOffsetInit)
			iOffsetInit=1, iOffset=User::NTickCount();
		TRACE(RDebug::Printf("sT++%d\n", us));
		iTimer.HighRes(iStatus, us);
		iTimerInit = 1;
		}
	SetActive();
	iTimerRunning = 1;
	}

TUint64 CLoadSim::Random()
	{
	LFSR(iSeed);
	TUint32 h = I64HIGH(iSeed);
	TUint32 l = I64LOW(iSeed);
	h *= 0x9e3779b9u;
	l *= 0x9e3779b9u;
	return MAKE_TUINT64(l,h);
	}

MEvent::MEvent(CLoadSim* aT, TInt aId)
	{
	iId = (TUint8)aId;
	iQueued = 0;
	iE1 = 0;
	iE2 = 0;
	iChain = 0;
	iT = aT;
	iNextEventTime = 0;
	}

MEvent::~MEvent()
	{
	if (iT)
		Dequeue();
	}

void MEvent::QueueAt(TUint32 aTime)
	{
	TRACE(RDebug::Printf("Q%d@%u\n", iId, aTime));
	if (iQueued)
		Dequeue();
	MEvent** p = &iT->iNextEvent;
	MEvent* e = iT->iNextEvent;
	for (; e && e->iNextEventTime <= aTime; p=&e->iChain, e=e->iChain)
		{}
	iChain = e;
	*p = this;
	iNextEventTime = aTime;
	iQueued = 1;
	if (iT->iNextEvent==this && !iT->iInRunL)
		iT->StartTimer();
	}

void MEvent::QueueAfter(TUint32 aInterval)
	{
	TRACE(RDebug::Printf("Q%d+%u\n", iId, aInterval));
	TUint32 now = User::NTickCount();
	if (!iT->iTimerInit)
		iT->iOffset=now, iT->iOffsetInit=1;
	QueueAt(now-iT->iOffset+aInterval);
	}

void MEvent::Dequeue()
	{
	TRACE(RDebug::Printf("DQ%d\n", iId));
	if (!iQueued)
		return;
	MEvent* e = iT->iNextEvent;
	for (; e && e->iChain!=this; e=e->iChain)
		{}
	if (e)
		{
		e->iChain = iChain;
		}
	iChain = 0;
	iQueued = 0;
	}

TInt MEvent::Event()
	{
	TRACE(RDebug::Printf("*%d\n", iId));
	return iId;
	}



/******************************************************************************
 * Poisson process simulation
 ******************************************************************************/
class MDiscretePoisson : public MEvent
	{
public:
	MDiscretePoisson(CLoadSim* aT, TInt aId, TUint32 aMicroseconds);
	~MDiscretePoisson();
	virtual void Start();
	virtual TInt Event();
	virtual void PoissonEvent();
public:
	TUint32		iUs;
	TBool		iContinue;
	};

MDiscretePoisson::MDiscretePoisson(CLoadSim* aT, TInt aId, TUint32 aMicroseconds)
	:	MEvent(aT, aId)
	{
	iUs = aMicroseconds;
	iContinue = EFalse;
	}

MDiscretePoisson::~MDiscretePoisson()
	{
	}

void MDiscretePoisson::Start()
	{
	iContinue = ETrue;
	TUint32 gap = ExpRV(Random(), iUs, iT->TimerPeriod());
	TRACE(RDebug::Printf("GG%u\n", gap));
	QueueAt(iNextEventTime + gap);
	}

TInt MDiscretePoisson::Event()
	{
	PoissonEvent();
	if (iContinue)
		Start();
	return MEvent::Event();
	}

void MDiscretePoisson::PoissonEvent()
	{
	}



/******************************************************************************
 * Consume a specified amount of CPU time in either a continuous
 * or 'staccato' fashion (i.e. in irregular intervals punctuated by gaps)
 ******************************************************************************/
class CStaccatoCpuEater : public CActive, public MEvent
	{
public:
	CStaccatoCpuEater(CLoadSim* aT, MCpuEater* aE, TUint32 aGranularity, TUint32 aMeanGap);
	~CStaccatoCpuEater();
	void EatMore(TInt64 aMicroseconds);
	TUint32 WorkDone() const { return iWorkDone; }
	TUint32 Invocations() const { return iInvocations; }
private:
	virtual void RunL();
	virtual void DoCancel();
	virtual void Start();
	virtual TInt Event();
	void StartEating();
private:
	MCpuEater* iE;
	TUint32 iWorkDone;
	TUint32 iInvocations;
	TUint32 iGranularity;
	TUint32 iMeanGap;
	TBool iEating;
	TInt64 iRemainingCpuTime;
	TTimeIntervalMicroSeconds iInitialCpuTime;
	TTimeIntervalMicroSeconds iFinalCpuTime;
	TInt64 iTotalCpuTime;
	};

CStaccatoCpuEater::CStaccatoCpuEater(CLoadSim* aT, MCpuEater* aE, TUint32 aGranularity, TUint32 aMeanGap)
	:	CActive(EPriorityIdle),
		MEvent(aT, 0x53)
	{
	iE = aE;
	iWorkDone = 0;
	iInvocations = 0;
	iGranularity = aGranularity;
	iMeanGap = aMeanGap;
	iEating = EFalse;
	iRemainingCpuTime = 0;
	}

CStaccatoCpuEater::~CStaccatoCpuEater()
	{
	Cancel();
	}

void CStaccatoCpuEater::EatMore(TInt64 aMicroseconds)
	{
	TRACE(RDebug::Printf("E+%08x %08x\n", I64HIGH(aMicroseconds), I64LOW(aMicroseconds)));
	iRemainingCpuTime += aMicroseconds;
	if (!Queued() && !iEating && iRemainingCpuTime>0)
		StartEating();
	}

void CStaccatoCpuEater::RunL()
	{
	TInt time = KMaxTInt;
	if (iRemainingCpuTime < TInt64(KMaxTInt))
		time = I64LOW(iRemainingCpuTime);
	++iInvocations;
	iE->Eat(time, &iWorkDone);
	TTimeIntervalMicroSeconds ms;
	TInt r = RThread().GetCpuTime(ms);
	assert(r==KErrNone);
	if (ms < iFinalCpuTime)
		{
		SetActive();
		TRequestStatus* pS = &iStatus;
		User::RequestComplete(pS, 0);
		return;
		}
	iEating = EFalse;
	TInt64 delta = ms.Int64() - iInitialCpuTime.Int64();
	iRemainingCpuTime -= delta;
	iTotalCpuTime += delta;
	TRACE(RDebug::Printf("D=%8u T=%10u\n",I64LOW(delta),I64LOW(iTotalCpuTime)));
	if (iRemainingCpuTime > 0)
		{
		TUint32 gap = ExpRV(Random(), iMeanGap, iT->TimerPeriod());
		TRACE(RDebug::Printf("G%u\n", gap));
		QueueAfter(gap);
		}
	}

void CStaccatoCpuEater::DoCancel()
	{
	MEvent::Dequeue();
	iEating = EFalse;
	}

void CStaccatoCpuEater::Start()
	{
	}

TInt CStaccatoCpuEater::Event()
	{
	if (!iEating && iRemainingCpuTime>0)
		{
		StartEating();
		}
	return MEvent::Event();
	}

void CStaccatoCpuEater::StartEating()
	{
	iEating = ETrue;
	TInt r = RThread().GetCpuTime(iInitialCpuTime);
	assert(r==KErrNone);
	if (iGranularity)
		{
		TInt howmuch = ExpRV(iT->Random(), iGranularity, 1);
		TRACE(RDebug::Printf("SE+%08x\n", howmuch));
		iFinalCpuTime = iInitialCpuTime.Int64() + TInt64(howmuch);
		}
	else
		iFinalCpuTime = iInitialCpuTime.Int64() + iRemainingCpuTime;	// continuous CPU use
	SetActive();
	TRequestStatus* pS = &iStatus;
	User::RequestComplete(pS, 0);
	}



/******************************************************************************
 * Consume CPU time in a bursty fashion
 ******************************************************************************/
class CBurstyCpuEater : public CStaccatoCpuEater, public MDiscretePoisson
	{
public:
	struct SParams
		{
		MCpuEater*	iE;
		TUint32		iGranularity;
		TUint32		iMeanIntraBurstGap;
		TUint32		iMeanBurstLength;
		TUint32		iMeanInterBurstGap;
		};
public:
	CBurstyCpuEater(CLoadSim* aT, const SParams& aParams);
	~CBurstyCpuEater();
	virtual void Start();
	virtual void PoissonEvent();
public:
	TUint32		iMeanBurstLength;
	TUint32		iMeanInterBurstGap;
	};

CBurstyCpuEater::CBurstyCpuEater(CLoadSim* aT, const SParams& aParams)
	:	CStaccatoCpuEater(aT, aParams.iE, aParams.iGranularity, aParams.iMeanIntraBurstGap),
		MDiscretePoisson(aT, 0x42, aParams.iMeanInterBurstGap)
	{
	iMeanBurstLength = aParams.iMeanBurstLength;
	iMeanInterBurstGap = aParams.iMeanInterBurstGap;
	}

CBurstyCpuEater::~CBurstyCpuEater()
	{
	}

void CBurstyCpuEater::Start()
	{
	if (iMeanInterBurstGap > 0)
		{
		PoissonEvent();
		MDiscretePoisson::Start();
		}
	else
		{
		EatMore(iMeanBurstLength);	// one single burst
		}
	}

void CBurstyCpuEater::PoissonEvent()
	{
	TInt burstLen = ExpRV(CStaccatoCpuEater::Random(), iMeanBurstLength, 1);
	EatMore(burstLen);
	}



/******************************************************************************
 * Stop the active scheduler after a certain time
 ******************************************************************************/
class CTimedStopper : public CActive
	{
public:
	static CTimedStopper* NewL();
	~CTimedStopper();
	void Start(TInt64 aMicroseconds);
private:
	CTimedStopper();
	virtual void RunL();
	virtual void DoCancel();
private:
	RTimer	iTimer;
	};

CTimedStopper::CTimedStopper()
	:	CActive(EPriorityHigh)
	{
	}

CTimedStopper::~CTimedStopper()
	{
	Cancel();
	iTimer.Close();
	}

CTimedStopper* CTimedStopper::NewL()
	{
	CTimedStopper* p = new (ELeave) CTimedStopper();
	CleanupStack::PushL(p);
	User::LeaveIfError(p->iTimer.CreateLocal());
	CleanupStack::Pop();
	return p;
	}

void CTimedStopper::DoCancel()
	{
	iTimer.Cancel();
	}

void CTimedStopper::RunL()
	{
	CActiveScheduler::Stop();
	}

void CTimedStopper::Start(TInt64 aMicroseconds)
	{
	TInt p = (TInt)aMicroseconds;
	iTimer.HighRes(iStatus, p);
	SetActive();
	}





/******************************************************************************
 * Do something CPU intensive to consume CPU time
 ******************************************************************************/
class CDefaultCpuEater : public CBase, public MCpuEater
	{
public:
	CDefaultCpuEater();
	~CDefaultCpuEater();
	virtual void Eat(TInt aTime, TUint32* aWorkDone);
protected:
	TUint64 iX;
	};

CDefaultCpuEater::CDefaultCpuEater()
	{
	iX = 1;
	}

CDefaultCpuEater::~CDefaultCpuEater()
	{
	}

void CDefaultCpuEater::Eat(TInt aTime, TUint32* aWorkDone)
	{
	const TUint64 KMagic = UI64LIT(0x9e3779b97f4a7c15);
	TUint32 work = WorkValue(aTime);
	if (aWorkDone)
		*aWorkDone += work;
	while (work--)
		iX *= KMagic;
	}



/******************************************************************************
 * Do something CPU intensive to consume CPU time, partially serialized
 ******************************************************************************/
class CAmdahlCpuEater : public CDefaultCpuEater
	{
public:
	static CAmdahlCpuEater* NewLC();
	~CAmdahlCpuEater();
	virtual void Eat(TInt aTime, TUint32* aWorkDone);
protected:
	CAmdahlCpuEater();
	void ConstructL();
protected:
	RMutex iMutex;
	TUint32 iFactor;
	};

CAmdahlCpuEater::CAmdahlCpuEater()
	{
	}

CAmdahlCpuEater::~CAmdahlCpuEater()
	{
	iMutex.Close();
	}

CAmdahlCpuEater* CAmdahlCpuEater::NewLC()
	{
	CAmdahlCpuEater* p = new (ELeave) CAmdahlCpuEater();
	CleanupStack::PushL(p);
	p->ConstructL();
	return p;
	}

void CAmdahlCpuEater::ConstructL()
	{
	User::LeaveIfError(iMutex.CreateLocal());
	iFactor = KMaxTUint32 / (4*TCpuUsage::N());
	}

void CAmdahlCpuEater::Eat(TInt aTime, TUint32* aWorkDone)
	{
	TUint64 t(aTime);
	t *= TUint64(iFactor);
	t += TUint64(0x80000000u);
	t >>= 32;
	TInt stime = I64LOW(t);
	if (IsCalibrated())
		{
		iMutex.Wait();
		CDefaultCpuEater::Eat(stime, aWorkDone);
		aTime -= stime;
		iMutex.Signal();
		}
	CDefaultCpuEater::Eat(aTime, aWorkDone);
	}



/******************************************************************************
 * Do something memory intensive to consume CPU time
 ******************************************************************************/
class CMemoryBandwidthEater : public CBase, public MCpuEater
	{
public:
	static CMemoryBandwidthEater* NewLC(TUint32 aSize, TUint32 aRegionSize, TUint32 aRegionOffset);
	~CMemoryBandwidthEater();
	virtual void Calibrate();
protected:
	CMemoryBandwidthEater(TUint32 aSize, TUint32 aRegionSize, TUint32 aRegionOffset);
	void ConstructL();
	virtual void Eat(TInt aTime, TUint32* aWorkDone);
	TAny* At(TUint32 aRegion, TUint32 aIndex);
	TAny* StepWithinRegion(TAny* aInitial, TUint32 aStep);
protected:
	volatile TUint32 iRegionAlloc;
	TUint32 iPageSize;
	TUint32 iSize;				// multiple of page size
	TAny* iData;				// page aligned
	RChunk iChunk;
	TUint8 iLog2RegionSize;		// log2(bytes per region)
	TUint8 iLog2RO;				// log2(offset from region n to n+1 in bytes)
	TUint8 iLog2PageSize;
	TUint8 iRegionBits;			// number of bits to specify region
	TUint32 iNRgn;
	TUint32 iRegionMask;
	TUint32 iLowerIndexMask;
	TUint32 iUpperIndexMask;
	};

TUint32 AtomicClearLS1(volatile TUint32* aMask)
	{
	TUint32 initial = *aMask;
	TUint32 final;
	do	{
		final = initial & (initial-1);
		} while(!__e32_atomic_cas_ord32(aMask, &initial, final));
	return initial;
	}

TInt AtomicAllocBit(volatile TUint32* aMask)
	{
	return __e32_find_ls1_32(AtomicClearLS1(aMask));
	}

TUint32 AtomicFreeBit(volatile TUint32* aMask, TInt aBit)
	{
	return __e32_atomic_ior_ord32(aMask, 1u<<aBit);
	}



CMemoryBandwidthEater* CMemoryBandwidthEater::NewLC(TUint32 aSize, TUint32 aRegionSize, TUint32 aRegionOffset)
	{
	CMemoryBandwidthEater* p = new (ELeave) CMemoryBandwidthEater(aSize, aRegionSize, aRegionOffset);
	CleanupStack::PushL(p);
	p->ConstructL();
	return p;
	}

CMemoryBandwidthEater::CMemoryBandwidthEater(TUint32 aSize, TUint32 aRegionSize, TUint32 aRegionOffset)
	{
	TInt r = HAL::Get(HAL::EMemoryPageSize, (TInt&)iPageSize);
	assert(r==KErrNone);
	iLog2PageSize = (TUint8)__e32_find_ms1_32(iPageSize);
	assert( !(aRegionSize & (aRegionSize-1)) );
	assert( !(aRegionOffset & (aRegionOffset-1)) );
	iLog2RegionSize = (TUint8)__e32_find_ms1_32(aRegionSize);
	iLog2RO = (TUint8)__e32_find_ms1_32(aRegionOffset);
	TUint32 round = (aRegionSize>iPageSize) ? aRegionSize : iPageSize;
	iSize = (aSize + round - 1) &~ (round - 1);
	--iSize;
	iSize |= (iSize>>1);
	iSize |= (iSize>>2);
	iSize |= (iSize>>4);
	iSize |= (iSize>>8);
	iSize |= (iSize>>16);
	++iSize;
	iNRgn = iSize >> iLog2RegionSize;
	if (iNRgn>=32)
		iRegionAlloc = ~0u;
	else
		iRegionAlloc = ~((~0u)<<iNRgn);
	iRegionBits = TUint8(1 + __e32_find_ms1_32(iNRgn-1));
	iLowerIndexMask = ~((~0u)<<iLog2RO);
	iRegionMask = (~((~0u)<<iRegionBits))<<iLog2RO;
	iUpperIndexMask = ((iSize-1)>>(iRegionBits+iLog2RO))<<(iRegionBits+iLog2RO);
	}

CMemoryBandwidthEater::~CMemoryBandwidthEater()
	{
	iChunk.Close();
	}

void CMemoryBandwidthEater::ConstructL()
	{
	TInt mask = (1<<20)-1;
	TInt maxSize = (TInt(iSize)+mask)&~mask;
	User::LeaveIfError(iChunk.CreateLocal(iSize, maxSize, EOwnerThread));
	iData = iChunk.Base();
	}

void CMemoryBandwidthEater::Calibrate()
	{
	MCpuEater::Calibrate();
	MCpuEater::Calibrate();
	}

TAny* CMemoryBandwidthEater::At(TUint32 aRegion, TUint32 aIndex)
	{
	TUint32 offset = aIndex & iLowerIndexMask;
	offset |= (aRegion<<iLog2RO);
	offset |= ((aIndex<<iRegionBits) & iUpperIndexMask);
	return ((TUint8*)iData) + offset;
	}

TAny* CMemoryBandwidthEater::StepWithinRegion(TAny* aInitial, TUint32 aStep)
	{
	TUintPtr offset = TUintPtr(aInitial) - TUintPtr(iData);
	TUintPtr offset2 = offset + (aStep & iLowerIndexMask);
	if ((offset^offset2)&iRegionMask)
		{
		offset2 -= (iLowerIndexMask+1);
		aStep += (iLowerIndexMask+1);
		}
	offset2 += ((aStep<<iRegionBits)&iUpperIndexMask);
	offset2 &= (iSize-1);
	return ((TUint8*)iData) + offset2;
	}

void CMemoryBandwidthEater::Eat(TInt aTime, TUint32* aWorkDone)
	{
	TUint32 work = WorkValue(aTime);
	if (aWorkDone)
		*aWorkDone += work;
	TInt region = AtomicAllocBit(&iRegionAlloc);
	assert(region>=0);
	TUint32 done = 0;
	TUint32 rgnsz = 1u << iLog2RegionSize;
	for (; work; work-=done)
		{
		done = (work>rgnsz) ? rgnsz : work;
		TUint8* p = (TUint8*)At(region,0);
		TUint8 prev = *p;
		TUint32 n = done;
		do	{
			TUint8* q = p;
			p = (TUint8*)StepWithinRegion(p, 31);
			*q = *p;
			} while(--n);
		*p = prev;
		}
	AtomicFreeBit(&iRegionAlloc, region);
	}


/******************************************************************************
 * Do lots of atomic operations to consume CPU time
 ******************************************************************************/
class CAtomicMemoryBandwidthEater : public CMemoryBandwidthEater
	{
public:
	static CAtomicMemoryBandwidthEater* NewLC(TUint32 aSize);
	~CAtomicMemoryBandwidthEater();
protected:
	CAtomicMemoryBandwidthEater(TUint32 aSize);
	void ConstructL();
	virtual void Eat(TInt aTime, TUint32* aWorkDone);
protected:
	volatile TUint32 iX;
	};

CAtomicMemoryBandwidthEater* CAtomicMemoryBandwidthEater::NewLC(TUint32 aSize)
	{
	CAtomicMemoryBandwidthEater* p = new (ELeave) CAtomicMemoryBandwidthEater(aSize);
	CleanupStack::PushL(p);
	p->ConstructL();
	return p;
	}

CAtomicMemoryBandwidthEater::CAtomicMemoryBandwidthEater(TUint32 aSize)
	:	CMemoryBandwidthEater(aSize, aSize, aSize)
	{
	iX = TUint32(this) ^ RThread().Id().operator TUint();
	iX *= 0x9e3779b9u;
	}

CAtomicMemoryBandwidthEater::~CAtomicMemoryBandwidthEater()
	{
	}

void CAtomicMemoryBandwidthEater::ConstructL()
	{
	CMemoryBandwidthEater::ConstructL();
	}

TUint32 AtomicRandom(volatile TUint32* a)
	{
	TUint32 initial = *a;
	TUint32 final;
	do	{
		final = 69069*initial + 41;
		} while(!__e32_atomic_cas_ord32(a, &initial, final));
	return final;
	}

void CAtomicMemoryBandwidthEater::Eat(TInt aTime, TUint32* aWorkDone)
	{
	TUint32 work = WorkValue(aTime);
	if (aWorkDone)
		*aWorkDone += work;
	volatile TUint32* pW = (volatile TUint32*)iData;
	const TUint32 mask = iSize/sizeof(TUint32)-1;
	TUint32 x = AtomicRandom(&iX);
	TUint32 n = work;
	do	{
		TUint32 offset = (x>>2) & mask;
		x = 69069*x+41;
		__e32_atomic_add_rlx32(pW+offset, 1);
		} while(--n);
	}


/******************************************************************************
 *
 ******************************************************************************/
struct SThreadResult
	{
	TUint64		iElapsedTime;
	TUint64		iCpuTime;
	TUint32		iWorkDone;
	TUint32		iInvocations;
	};

struct SThreadParams
	{
	TInt64		iTestTime;

	TInt		iId;
	TUint32		iCpuAffinity;

	TInt		iPriority;
	RSemaphore	iTurnstile;

	SThreadResult* iResult;
	TInt		iGroupId;

	MCpuEater*	iE;
	TUint32		iGranularity;
	TUint32		iMeanIntraBurstGap;
	TUint32		iMeanBurstLength;
	TUint32		iMeanInterBurstGap;
	};

class MThreadCompletion
	{
public:
	virtual void Complete(TBool aOk, SThreadParams* aParams)=0;
	};

class CThreadI : public CBase
	{
public:
	CThreadI();
	~CThreadI();
	static TInt ThreadFunc(TAny* aPtr);
	TInt Run();
	void InitL();
public:
	CTrapCleanup*		iCleanup;
	CActiveScheduler*	iAS;
	CLoadSim*			iL;
	CBurstyCpuEater*	iB;
	CTimedStopper*		iStopper;
	RSemaphore			iTurnstile;
	SThreadParams*		iParams;
	};

CThreadI::CThreadI()
	{
	}

CThreadI::~CThreadI()
	{
	iTurnstile.Close();
	delete iStopper;
	delete iB;
	delete iL;
	delete iAS;
	delete iCleanup;
	}

TInt CThreadI::ThreadFunc(TAny* aPtr)
	{
	CThreadI* p = new CThreadI;
	if (!p)
		return KErrNoMemory;
	p->iParams = (SThreadParams*)aPtr;
	return p->Run();
	}

void CThreadI::InitL()
	{
	iTurnstile = iParams->iTurnstile;
	User::LeaveIfError(iTurnstile.Duplicate(RThread(), EOwnerThread));
	iAS = new (ELeave) CActiveScheduler;
	CActiveScheduler::Install(iAS);
	iL = CLoadSim::NewL();
	CActiveScheduler::Add(iL);
	const CBurstyCpuEater::SParams* params = (const CBurstyCpuEater::SParams*)&iParams->iE;
	iB = new (ELeave) CBurstyCpuEater(iL, *params);
	CActiveScheduler::Add(iB);
	iStopper = CTimedStopper::NewL();
	CActiveScheduler::Add(iStopper);
	memclr(iParams->iResult, sizeof(*iParams->iResult));
	RThread().SetPriority(TThreadPriority(iParams->iPriority));
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, (TAny*)iParams->iCpuAffinity, 0);
	}

TInt CThreadI::Run()
	{
	iCleanup = CTrapCleanup::New();
	if (!iCleanup)
		return KErrNoMemory;
	TRAPD(r,InitL());
	if (r == KErrNone)
		{
		TThreadCpuUsageSample initial;
		TThreadCpuUsageSample final;
		RThread::Rendezvous(KErrNone);
		iTurnstile.Wait();
		iB->Start();
		initial.Sample(RThread());
		iStopper->Start(iParams->iTestTime);
		CActiveScheduler::Start();
		final.Sample(RThread());
		iParams->iResult->iWorkDone = iB->WorkDone();
		iParams->iResult->iInvocations = iB->Invocations();
		iParams->iResult->iElapsedTime = final.ElapsedTimeDelta(initial);
		iParams->iResult->iCpuTime = final.CpuTimeDelta(initial);
		}
	delete this;
	return r;
	}


/******************************************************************************
 *
 ******************************************************************************/
class CThreadX : public CActive
	{
public:
	static CThreadX* NewL(SThreadParams* aParams, MThreadCompletion* aComplete);
	static CThreadX* NewLC(SThreadParams* aParams, MThreadCompletion* aComplete);
	CThreadX();
	~CThreadX();
	void ConstructL();
	virtual void RunL();
	virtual void DoCancel();
public:
	RThread				iThread;
	RTimer				iTimer;
	SThreadParams*		iParams;
	MThreadCompletion*	iComplete;
	};

CThreadX::CThreadX()
	:	CActive(EPriorityStandard)
	{
	}

CThreadX::~CThreadX()
	{
	Cancel();
	iTimer.Close();
	iThread.Close();
	}

CThreadX* CThreadX::NewL(SThreadParams* aParams, MThreadCompletion* aComplete)
	{
	CThreadX* p = NewLC(aParams, aComplete);
	CleanupStack::Pop();
	return p;
	}

CThreadX* CThreadX::NewLC(SThreadParams* aParams, MThreadCompletion* aComplete)
	{
	CThreadX* p = new (ELeave) CThreadX();
	p->iParams = aParams;
	p->iComplete = aComplete;
	CleanupStack::PushL(p);
	p->ConstructL();
	return p;
	}

const TInt KThreadHeapMin = 0x1000;
const TInt KThreadHeapMax = 0x200000;
void CThreadX::ConstructL()
	{
	CActiveScheduler::Add(this);
	TRequestStatus s0, s1;
	User::LeaveIfError(iTimer.CreateLocal());
	User::LeaveIfError(iThread.Create(KNullDesC, &CThreadI::ThreadFunc, 0x1000, KThreadHeapMin, KThreadHeapMax, iParams));
	iThread.Rendezvous(s1);
	if (s1!=KRequestPending)
		{
		User::WaitForRequest(s1);
		User::Leave(s1.Int());
		}
	iTimer.After(s0, 5*1000*1000);
	iThread.Resume();
	User::WaitForRequest(s0, s1);
	if (s1==KRequestPending)
		{
		iThread.Terminate(KErrCouldNotStart);
		User::WaitForRequest(s1);
		User::Leave(KErrTimedOut);
		}
	iTimer.Cancel();
	User::WaitForRequest(s0);
	if (iThread.ExitType() != EExitPending)
		{
		User::Leave(KErrDied);
		}
	iThread.Logon(iStatus);
	if (iStatus!=KRequestPending)
		{
		User::WaitForRequest(iStatus);
		User::Leave(iStatus.Int());
		}
	SetActive();
	User::LeaveIfError(s1.Int());
	}

void CThreadX::DoCancel()
	{
	iThread.Terminate(KErrCouldNotStart);
	}

void CThreadX::RunL()
	{
	TBool ok = ETrue;
	if (iThread.ExitType() != EExitKill)
		ok = EFalse;
	if (iThread.ExitReason() != KErrNone)
		ok = EFalse;
	if (iComplete)
		iComplete->Complete(ok, iParams);
	}


/******************************************************************************
 *
 ******************************************************************************/
struct STestThreadDesc
	{
	TUint32		iCpuAffinity;
	TInt		iPriority;
	TInt		iGroupId;
	TUint16		iEaterType;
	TUint16		iEaterInstance;
	TUint32		iGranularity;
	TUint32		iMeanIntraBurstGap;
	TUint32		iMeanBurstLength;
	TUint32		iMeanInterBurstGap;

	static STestThreadDesc* ContinuousL(TInt aPri = EPriorityNormal);
	static STestThreadDesc* ContinuousLC(TInt aPri = EPriorityNormal);
	static STestThreadDesc* StaccatoL(TUint32 aGranularity, TUint32 aMeanGap, TInt aPri = EPriorityNormal);
	static STestThreadDesc* StaccatoLC(TUint32 aGranularity, TUint32 aMeanGap, TInt aPri = EPriorityNormal);
	};

STestThreadDesc* STestThreadDesc::ContinuousLC(TInt aPri)
	{
	STestThreadDesc* p = (STestThreadDesc*)User::AllocLC(sizeof(STestThreadDesc));
	p->iCpuAffinity = 0xffffffff;
	p->iPriority = aPri;
	p->iGroupId = 0;
	p->iEaterType = EEaterStd;
	p->iEaterInstance = 0;
	p->iGranularity = 0;
	p->iMeanIntraBurstGap = 0;
	p->iMeanBurstLength = KMaxTInt32;
	p->iMeanInterBurstGap = 0;
	return p;
	}

STestThreadDesc* STestThreadDesc::ContinuousL(TInt aPri)
	{
	STestThreadDesc* p = ContinuousLC(aPri);
	CleanupStack::Pop();
	return p;
	}

STestThreadDesc* STestThreadDesc::StaccatoLC(TUint32 aGranularity, TUint32 aMeanGap, TInt aPri)
	{
	STestThreadDesc* p = (STestThreadDesc*)User::AllocLC(sizeof(STestThreadDesc));
	p->iCpuAffinity = 0xffffffff;
	p->iPriority = aPri;
	p->iGroupId = 0;
	p->iEaterType = EEaterStd;
	p->iEaterInstance = 0;
	p->iGranularity = aGranularity;
	p->iMeanIntraBurstGap = aMeanGap;
	p->iMeanBurstLength = KMaxTInt32;
	p->iMeanInterBurstGap = 0;
	return p;
	}

STestThreadDesc* STestThreadDesc::StaccatoL(TUint32 aGranularity, TUint32 aMeanGap, TInt aPri)
	{
	STestThreadDesc* p = StaccatoLC(aGranularity, aMeanGap, aPri);
	CleanupStack::Pop();
	return p;
	}


class CTest : public CBase, public MThreadCompletion
	{
public:
	struct SStats
		{
		TInt64	iTotalCpu;
		TInt64	iMinCpu;
		TInt64	iMaxCpu;
		TInt64	iTotalWork;
		TInt64	iMinWork;
		TInt64	iMaxWork;
		};
public:
	static CTest* NewL(TInt64 aTestTime, TInt aNumTypes, ...);
	~CTest();
	RPointerArray<SThreadParams>& Threads()
		{ return iP; }
	TInt Execute();
	void PrintResults() const;
	void GetStats(SStats& aStats, TInt aFirstThread=0, TInt aCount=KMaxTInt) const;
	TInt64 TotalCpuAll() const;
private:
	CTest();
	void ConstructL(TInt64 aTestTime, TInt aNumTypes, VA_LIST aList);
	SThreadParams* AddThreadParamsL();
	CThreadX* AddThreadXL(SThreadParams* aParams);

	virtual void Complete(TBool aOk, SThreadParams* aParams);
private:
	RPointerArray<SThreadParams> iP;
	RPointerArray<CThreadX> iTX;
	REaterArray iEaters;
	RSemaphore iTurnstile;
	TInt iCompleteCount;
	TCpuUsage iInitialCpuUsage;
	TCpuUsage iFinalCpuUsage;
	};

CTest::CTest()
	:	iP(32),
		iTX(32)
	{
	}

CTest::~CTest()
	{
	iTX.ResetAndDestroy();

	TInt i;
	TInt c = iP.Count();
	for (i=0; i<c; ++i)
		{
		SThreadParams* p = iP[i];
		iP[i] = 0;
		if (p)
			{
			User::Free(p->iResult);
			User::Free(p);
			}
		}
	iP.Close();
	iEaters.Close();
	iTurnstile.Close();
	}

CTest* CTest::NewL(TInt64 aTestTime, TInt aNumTypes, ...)
	{
	VA_LIST list;
	VA_START(list, aNumTypes);
	CTest* p = new (ELeave) CTest;
	CleanupStack::PushL(p);
	p->ConstructL(aTestTime, aNumTypes, list);
	CleanupStack::Pop();
	return p;
	}

SThreadParams* CTest::AddThreadParamsL()
	{
	SThreadResult* tr = (SThreadResult*)User::AllocLC(sizeof(SThreadResult));
	SThreadParams* tp = (SThreadParams*)User::AllocLC(sizeof(SThreadParams));
	memclr(tr, sizeof(SThreadResult));
	tp->iResult = tr;
	iP.AppendL(tp);
	CleanupStack::Pop(2);
	tp->iTurnstile = iTurnstile;
	return tp;
	}

CThreadX* CTest::AddThreadXL(SThreadParams* aP)
	{
	if (aP->iGranularity==0 && aP->iMeanInterBurstGap==0)
		{
		// continuous use thread
		if (TInt64(aP->iMeanBurstLength) >= aP->iTestTime)
			aP->iMeanBurstLength = I64LOW(aP->iTestTime) + (I64LOW(aP->iTestTime)>>1);
		}
	CThreadX* tx = CThreadX::NewLC(aP, this);
	iTX.AppendL(tx);
	CleanupStack::Pop();
	return tx;
	}

void CTest::Complete(TBool aOk, SThreadParams*)
	{
	if (!aOk || --iCompleteCount==0)
		CActiveScheduler::Stop();
	}


void CTest::ConstructL(TInt64 aTestTime, TInt aNumTypes, VA_LIST aList)
	{
	typedef const STestThreadDesc* TTestThreadDescPtrC;

	User::LeaveIfError(iTurnstile.CreateLocal(0));
	TInt tt;
	TInt tid = 0;
	for (tt=0; tt<aNumTypes; ++tt)
		{
		TInt nThreads = VA_ARG(aList, TInt);
		TInt inc = 0;
		const TTestThreadDescPtrC* ppttd = 0;
		TTestThreadDescPtrC pttd = 0;
		if (nThreads < 0)
			{
			ppttd = VA_ARG(aList, const TTestThreadDescPtrC*);
			nThreads = -nThreads;
			inc = 1;
			}
		else
			{
			pttd = VA_ARG(aList, TTestThreadDescPtrC);
			ppttd = &pttd;
			}
		TInt k;
		for (k=0; k<nThreads; ++k, ++tid)
			{
			const STestThreadDesc& ttd = **ppttd;
			ppttd += inc;
			SThreadParams* tp = AddThreadParamsL();
			tp->iId = tid;
			tp->iTestTime = aTestTime;
			tp->iCpuAffinity = ttd.iCpuAffinity;
			tp->iPriority = ttd.iPriority;
			tp->iGroupId = ttd.iGroupId;
			tp->iE = iEaters.FindOrCreateL(ttd.iEaterType, ttd.iEaterInstance);
			tp->iGranularity = ttd.iGranularity;
			tp->iMeanIntraBurstGap = ttd.iMeanIntraBurstGap;
			tp->iMeanBurstLength = ttd.iMeanBurstLength;
			tp->iMeanInterBurstGap = ttd.iMeanInterBurstGap;
			AddThreadXL(tp);
			}
		}
	}

TInt CTest::Execute()
	{
	iCompleteCount = iP.Count();
	iInitialCpuUsage.Sample();
	iTurnstile.Signal(iCompleteCount);
	CActiveScheduler::Start();
	iFinalCpuUsage.Sample();
	return iCompleteCount ? KErrGeneral : KErrNone;
	}

void CTest::GetStats(SStats& a, TInt aFirstThread, TInt aCount) const
	{
	a.iTotalCpu = 0;
	a.iMinCpu = KMaxTInt64;
	a.iMaxCpu = KMinTInt64;
	a.iTotalWork = 0;
	a.iMinWork = KMaxTInt64;
	a.iMaxWork = KMinTInt64;
	TInt nt = iP.Count();
	if (aFirstThread > nt)
		aFirstThread = nt;
	if (aCount > nt - aFirstThread)
		aCount = nt - aFirstThread;
	TInt i = aFirstThread;
	for (; i<aFirstThread+aCount; ++i)
		{
		SThreadResult* tr = iP[i]->iResult;
		TInt64 cpu = tr->iCpuTime;
		TInt64 work = tr->iWorkDone;
		a.iTotalCpu += cpu;
		a.iTotalWork += work;
		if (cpu < a.iMinCpu)
			a.iMinCpu = cpu;
		if (cpu > a.iMaxCpu)
			a.iMaxCpu = cpu;
		if (work < a.iMinWork)
			a.iMinWork = work;
		if (work > a.iMaxWork)
			a.iMaxWork = work;
		}
	}

TInt64 CTest::TotalCpuAll() const
	{
	TInt i;
	TInt nc = TCpuUsage::N();
	TInt64 totalCpuAll = 0;
	for (i=0; i<nc; ++i)
		{
		TInt64 u = iFinalCpuUsage.CpuTimeDelta(iInitialCpuUsage, i);
		totalCpuAll += u;
		}
	return totalCpuAll;
	}

void CTest::PrintResults() const
	{
	TInt i;
	TInt nt = iP.Count();
	TInt nc = TCpuUsage::N();
	TInt64 totalCpuAll = 0;
	TInt64 totalCpu = 0;
	TInt64 totalWork = 0;
	for (i=0; i<nt; ++i)
		{
		SThreadResult* tr = iP[i]->iResult;
		test.Printf(_L("%2u: E=%10u     C=%10u     I=%10u     W=%10u\n"),
			i, I64LOW(tr->iElapsedTime), I64LOW(tr->iCpuTime), tr->iInvocations, tr->iWorkDone );
		totalCpu += tr->iCpuTime;
		totalWork += TInt64(tr->iWorkDone);
		}
	test.Printf(_L("Total              C=%12Lu                    W=%12Lu\n"), totalCpu, totalWork);
	for (i=0; i<nc; ++i)
		{
		TInt64 u = iFinalCpuUsage.CpuTimeDelta(iInitialCpuUsage, i);
		totalCpuAll += u;
		test.Printf(_L("Cpu%1u: %10u "), i, I64LOW(u));
		}
	test.Printf(_L("\n"));
	test.Printf(_L("Total %12Lu\n"), totalCpuAll);
	}



MCpuEater* REaterArray::CreateLC(TInt aType)
	{
	switch (aType)
		{
		case EEaterStd:
			{
			CDefaultCpuEater* p = new (ELeave) CDefaultCpuEater();
			CleanupStack::PushL(p);
			p->Calibrate();
			return p;
			}
		case EEaterMemoryLocalS:
			{
			CMemoryBandwidthEater* p = CMemoryBandwidthEater::NewLC(0x8000, 0x0800, 0x0800);
			p->Calibrate();
			return p;
			}
		case EEaterMemoryNonLocalS:
			{
			CMemoryBandwidthEater* p = CMemoryBandwidthEater::NewLC(0x100000, 0x10000, 0x4);
			p->Calibrate();
			return p;
			}
		case EEaterMemoryLocalU:
			{
			CMemoryBandwidthEater* p = CMemoryBandwidthEater::NewLC(0x4000, 0x4000, 0x4000);
			p->Calibrate();
			return p;
			}
		case EEaterMemoryNonLocalU:
			{
			CMemoryBandwidthEater* p = CMemoryBandwidthEater::NewLC(0x80000, 0x80000, 0x80000);
			p->Calibrate();
			return p;
			}
		case EEaterMemoryAtomic:
			{
			CAtomicMemoryBandwidthEater* p = CAtomicMemoryBandwidthEater::NewLC(0x1000);
			p->Calibrate();
			return p;
			}
		case EEaterMemoryAtomic2:
			{
			CAtomicMemoryBandwidthEater* p = CAtomicMemoryBandwidthEater::NewLC(0x8000);
			p->Calibrate();
			return p;
			}
		case EEaterAmdahl:
			{
			CAmdahlCpuEater* p = CAmdahlCpuEater::NewLC();
			p->Calibrate();
			return p;
			}
		default:
			User::Leave(KErrNotSupported);
		}
	return 0;
	}



/******************************************************************************
 *
 ******************************************************************************/

void RunBenchmarkL(CTest::SStats& aB, STestThreadDesc* aT, TInt aLength)
	{
	const TInt NC = TCpuUsage::N();
	CTest* p;
	TUint32 saved_aff = aT->iCpuAffinity;
	aT->iCpuAffinity = NC-1;
	p = CTest::NewL(aLength, 1, 1, aT);
	TInt r = p->Execute();
	test_KErrNone(r);
	p->PrintResults();
	p->GetStats(aB);
	delete p;
	aT->iCpuAffinity = saved_aff;
	}

void CompareToBenchmark(const CTest::SStats& aS, const CTest::SStats& aB)
	{
	TReal bCpu = (TReal)aB.iTotalCpu;
	TReal bWork = (TReal)aB.iTotalWork;
	TReal bEff = bWork/bCpu*1000000.0;
	TReal tCpu = (TReal)aS.iTotalCpu;
	TReal tWork = (TReal)aS.iTotalWork;
	TReal tEff = tWork/tCpu*1000000.0;
	TReal mCpu = (TReal)aS.iMinCpu;
	TReal MCpu = (TReal)aS.iMaxCpu;
	TReal mWork = (TReal)aS.iMinWork;
	TReal MWork = (TReal)aS.iMaxWork;
	test.Printf(_L("Total CPU usage %6.1f%% of benchmark\n"), 100.0*tCpu/bCpu);
	test.Printf(_L("Total work done %6.1f%% of benchmark\n"), 100.0*tWork/bWork);
	test.Printf(_L("Max/min ratio   %6.1f%% (CPU) %6.1f%% (Work)\n"), 100.0*MCpu/mCpu, 100.0*MWork/mWork);
	test.Printf(_L("Work/sec bench: %10.1f test: %10.1f  Relative Efficiency %6.1f%%\n"), bEff, tEff, tEff/bEff*100.0);
	}

void ContinuousTestL(TInt aLength, TUint32 aWhich)
	{
	aLength *= 1000;
	const TInt NC = TCpuUsage::N();
	TInt r = 0;
	TInt i = 0;
	CTest::SStats benchmark;
	CTest::SStats st;
	CTest::SStats stm;
	CTest* p = 0;
	TUint et = aWhich >> 28;
	TBool separate = (aWhich & 0x08000000u);
	TInt instance = 0;
	STestThreadDesc* td[2*TCpuUsage::EMaxCpus] = {0};
	STestThreadDesc* tdm[TCpuUsage::EMaxCpus] = {0};
	STestThreadDesc* tdl[TCpuUsage::EMaxCpus] = {0};
	for (i=0; i<2*NC; ++i)
		{
		td[i] = STestThreadDesc::ContinuousLC();
		td[i]->iEaterType = (TUint16)et;
		td[i]->iEaterInstance = (TUint16)(separate ? (instance++) : 0);
		if (i<NC)
			{
			tdm[i] = STestThreadDesc::ContinuousLC(EPriorityMore);
			tdm[i]->iEaterType = (TUint16)et;
			tdm[i]->iEaterInstance = (TUint16)(separate ? (instance++) : 0);
			tdl[i] = STestThreadDesc::ContinuousLC();
			tdl[i]->iCpuAffinity = i;
			tdl[i]->iEaterType = (TUint16)et;
			tdl[i]->iEaterInstance = (TUint16)(separate ? (instance++) : 0);
			}
		}

	test.Printf(_L("\nTesting a single continuous CPU-locked thread\n"));
	RunBenchmarkL(benchmark, tdl[NC-1], aLength);

	TInt n;

	if (aWhich & 1)
		{
		for (n=1; n<=2*NC; ++n)
			{
			test.Printf(_L("\nTesting %d continuous thread(s) ...\n"), n);
			p = CTest::NewL(aLength, 1, -n, td);
			r = p->Execute();
			test_KErrNone(r);
			p->PrintResults();
			p->GetStats(st);
			delete p;
			CompareToBenchmark(st, benchmark);
			}
		}

	TInt h;
	if (aWhich & 2)
		{
		for (h=1; h<NC; ++h)
			{
			for (n=h; n<=2*NC; ++n)
				{
				TInt l = n - h;
				test.Printf(_L("\nTesting %d continuous thread(s) (%d higher priority) CPU-locked...\n"), n, h);
				p = CTest::NewL(aLength, 2, -h, tdl, l, tdl[h]);
				r = p->Execute();
				test_KErrNone(r);
				p->PrintResults();
				p->GetStats(st, h, l);
				p->GetStats(stm, 0, h);
				delete p;
				CompareToBenchmark(stm, benchmark);
				if (l>0)
					CompareToBenchmark(st, benchmark);
				}
			}
		}

	if (aWhich & 4)
		{
		for (h=1; h<NC; ++h)
			{
			for (n=h; n<=2*NC; ++n)
				{
				TInt l = n - h;
				test.Printf(_L("\nTesting %d continuous thread(s) (%d higher priority)...\n"), n, h);
				p = CTest::NewL(aLength, 2, -h, tdm, -l, td);
				r = p->Execute();
				test_KErrNone(r);
				p->PrintResults();
				p->GetStats(st, h, l);
				p->GetStats(stm, 0, h);
				delete p;
				CompareToBenchmark(stm, benchmark);
				if (l>0)
					CompareToBenchmark(st, benchmark);
				}
			}
		}

	CleanupStack::PopAndDestroy(NC*4);
	}


void TestWithOneSpecialL(const TDesC& aTitle, TInt aLength, const CTest::SStats& aB1, const CTest::SStats& aB0, STestThreadDesc* aT1, STestThreadDesc* aT0)
	{
	const TInt NC = TCpuUsage::N();
	CTest::SStats st;
	CTest::SStats sti;
	CTest* p = 0;
	TInt n;
	TInt r;
	for (n=1; n<=2*NC-1; ++n)
		{
		test.Printf(_L("\nTesting %d continuous thread(s) plus %S ...\n"), n, &aTitle);
		p = CTest::NewL(aLength, 2, 1, aT1, n, aT0);
		r = p->Execute();
		test_KErrNone(r);
		p->PrintResults();
		p->GetStats(st, 1, n);
		p->GetStats(sti, 0, 1);
		delete p;
		CompareToBenchmark(sti, aB1);
		CompareToBenchmark(st, aB0);

		test.Printf(_L("\nTesting %d continuous thread(s) plus %Sh ...\n"), n, &aTitle);
		TInt orig_pri = aT1->iPriority;
		aT1->iPriority = EPriorityMore;
		p = CTest::NewL(aLength, 2, 1, aT1, n, aT0);
		r = p->Execute();
		test_KErrNone(r);
		p->PrintResults();
		p->GetStats(st, 1, n);
		p->GetStats(sti, 0, 1);
		delete p;
		CompareToBenchmark(sti, aB1);
		CompareToBenchmark(st, aB0);
		aT1->iPriority = orig_pri;
		}
	}

void ContinuousPlusIntermittentTestL(TInt aLength, TUint32 aWhich)
	{
	aLength *= 1000;
	const TInt NC = TCpuUsage::N();
	TInt i = 0;
	CTest::SStats bmc, bmilc, bmilf, bmihc, bmihf;
	STestThreadDesc* td = STestThreadDesc::ContinuousLC();
	STestThreadDesc* tdl[TCpuUsage::EMaxCpus] = {0};
	STestThreadDesc* tdilc[TCpuUsage::EMaxCpus] = {0};	// light load, coarse grained
	STestThreadDesc* tdilf[TCpuUsage::EMaxCpus] = {0};	// light load, fine grained
	STestThreadDesc* tdihc[TCpuUsage::EMaxCpus] = {0};	// heavy load, coarse grained
	STestThreadDesc* tdihf[TCpuUsage::EMaxCpus] = {0};	// heavy load, fine grained
	for (i=0; i<NC; ++i)
		{
		tdl[i] = STestThreadDesc::ContinuousLC();
		tdl[i]->iCpuAffinity = i;
		tdilc[i] = STestThreadDesc::StaccatoLC(45000, 500000);
		tdilf[i] = STestThreadDesc::StaccatoLC(1000, 50000);
		tdihc[i] = STestThreadDesc::StaccatoLC(400000, 500000);
		tdihf[i] = STestThreadDesc::StaccatoLC(3000, 5000);
		}

	test.Printf(_L("\nTesting a single continuous CPU-locked thread\n"));
	RunBenchmarkL(bmc, tdl[NC-1], aLength);

	if (aWhich & 1)
		{
		test.Printf(_L("\nTesting a single ILC CPU-locked thread\n"));
		RunBenchmarkL(bmilc, tdilc[NC-1], aLength);
		}

	if (aWhich & 2)
		{
		test.Printf(_L("\nTesting a single ILF CPU-locked thread\n"));
		RunBenchmarkL(bmilf, tdilf[NC-1], aLength);
		}

	if (aWhich & 4)
		{
		test.Printf(_L("\nTesting a single IHC CPU-locked thread\n"));
		RunBenchmarkL(bmihc, tdihc[NC-1], aLength);
		}

	if (aWhich & 8)
		{
		test.Printf(_L("\nTesting a single IHF CPU-locked thread\n"));
		RunBenchmarkL(bmihf, tdihf[NC-1], aLength);
		}

	if (aWhich & 1)
		{
		TestWithOneSpecialL(_L("ILC"), aLength, bmilc, bmc, tdilc[0], td);
		}
	if (aWhich & 2)
		{
		TestWithOneSpecialL(_L("ILF"), aLength, bmilf, bmc, tdilf[0], td);
		}
	if (aWhich & 4)
		{
		TestWithOneSpecialL(_L("IHC"), aLength, bmihc, bmc, tdihc[0], td);
		}
	if (aWhich & 8)
		{
		TestWithOneSpecialL(_L("IHF"), aLength, bmihf, bmc, tdihf[0], td);
		}
	CleanupStack::PopAndDestroy(5*NC+1);
	}


TInt E32Main()
	{
	RThread().SetPriority(EPriorityAbsoluteHigh);
	test.Title();
	User::SetCritical(User::ESystemCritical);
	TCpuUsage::Init();
	TInt r = 0;

	CTrapCleanup* cln = CTrapCleanup::New();
	test_NotNull(cln);
	CActiveScheduler* as = new CActiveScheduler;
	test_NotNull(as);
	CActiveScheduler::Install(as);

	test.Printf(_L("\n************************************************************************\n"));
	test.Printf(_L("* Testing with CPU intensive loads...\n"));
	test.Printf(_L("************************************************************************\n"));
	TRAP(r, ContinuousTestL(3000, 1+4));
	test_KErrNone(r);
	TRAP(r, ContinuousTestL(10000, 1+4));
	test_KErrNone(r);
	TRAP(r, ContinuousPlusIntermittentTestL(10000, 15));
	test_KErrNone(r);

	test.Printf(_L("\n************************************************************************\n"));
	test.Printf(_L("* Testing with memory intensive loads, good locality...\n"));
	test.Printf(_L("************************************************************************\n"));
	TRAP(r, ContinuousTestL(3000, 1+4+0x08000000u+(EEaterMemoryLocalU<<28)));
	test_KErrNone(r);
	TRAP(r, ContinuousTestL(10000, 1+4+0x08000000u+(EEaterMemoryLocalU<<28)));
	test_KErrNone(r);

	test.Printf(_L("\n************************************************************************\n"));
	test.Printf(_L("* Testing with memory intensive loads, poor locality...\n"));
	test.Printf(_L("************************************************************************\n"));
	TRAP(r, ContinuousTestL(3000, 1+4+0x08000000u+(EEaterMemoryNonLocalU<<28)));
	test_KErrNone(r);
	TRAP(r, ContinuousTestL(10000, 1+4+0x08000000u+(EEaterMemoryNonLocalU<<28)));
	test_KErrNone(r);

	test.Printf(_L("\n************************************************************************\n"));
	test.Printf(_L("* Testing with memory intensive loads, atomic operations...\n"));
	test.Printf(_L("************************************************************************\n"));
	TRAP(r, ContinuousTestL(3000, 1+4+(EEaterMemoryAtomic<<28)));
	test_KErrNone(r);
	TRAP(r, ContinuousTestL(10000, 1+4+(EEaterMemoryAtomic<<28)));
	test_KErrNone(r);

	test.Printf(_L("\n************************************************************************\n"));
	test.Printf(_L("* Testing with memory intensive loads, atomic operations 2...\n"));
	test.Printf(_L("************************************************************************\n"));
	TRAP(r, ContinuousTestL(3000, 1+4+(EEaterMemoryAtomic2<<28)));
	test_KErrNone(r);
	TRAP(r, ContinuousTestL(10000, 1+4+(EEaterMemoryAtomic2<<28)));
	test_KErrNone(r);

	test.Printf(_L("\n************************************************************************\n"));
	test.Printf(_L("* Testing with CPU intensive loads with some serialization...\n"));
	test.Printf(_L("************************************************************************\n"));
	TRAP(r, ContinuousTestL(3000, 1+4+(EEaterAmdahl<<28)));
	test_KErrNone(r);
	TRAP(r, ContinuousTestL(10000, 1+4+(EEaterAmdahl<<28)));
	test_KErrNone(r);

	delete as;
	delete cln;
	return r;
	}

