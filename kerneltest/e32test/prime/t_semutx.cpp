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
// e32test\prime\t_semutx.cpp
// Tests the RSemaphore, RMutex and RCriticalSection classes
// Overview:
// Tests the RSemaphore, RMutex and RCriticalSection classes
// API Information:
// RSemaphore, RMutex, RCriticalSection
// Details:
// - Test RSemaphore and RMutex with the producer/consumer scenario.
// Create two threads, use signal and wait to coordinate the
// threads. Verify results are as expected.
// - Calculate the time required to create, resume and close a thread.
// - Test RSemaphore::Wait(timeout) in a variety ways and timeout 
// values. Verify results are as expected.
// - Test RMutex via two threads which write to an array. The writing
// and updating of the index is wrapped within a mutex pair. Verify 
// results are as expected.
// - Test RCriticalSection via two threads which write to an array. The 
// writing and updating of the index is wrapped within a critical section
// pair. Verify results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <hal.h>
#include <e32atomics.h>
#include <u32hal.h>
#include <e32svr.h>

const TInt KMaxBufferSize=10;
const TInt KMaxArraySize=10;
const TInt KNumProducerItems=100;

enum {EThread1ID=1,EThread2ID};

RTest test(_L("T_SEMUTX"));
RMutex mutex;
RCriticalSection criticalSn;
TInt thread1Count,thread2Count;
TInt arrayIndex;
TInt array[KMaxArraySize];
TInt consumerArray[KNumProducerItems];
RSemaphore slotAvailable,itemAvailable;
TBool doCpuLocking = EFalse;

// return num of cpus in system
TInt NumCpus()
	{
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0);
	return r;
	}


TInt LockCurrentThreadToCpu0(TBool aCallingIsMainTestThread = EFalse)
	{
	if (aCallingIsMainTestThread) 
		{
		if (NumCpus() > 1) 
			{
			doCpuLocking = ETrue;
			return UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, 0, 0); 
			}
		else
			{
			return KErrNone;
			}
		}
	return UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, 0, 0); 
	}

TInt UnlockCurrentThreadToCpu0(TBool aCallingIsMainTestThread = EFalse)
	{
	if (aCallingIsMainTestThread) 
		{
		if (NumCpus() > 1) 
			{
			doCpuLocking = EFalse;
			return UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, (TAny*) 0xffffffffu, 0); 
			}
		else
			{
			return KErrNone;
			}
		}
	return UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, (TAny*) 0xffffffffu, 0); 
	}


/******************************************************************************
 * Random Number Generation
 ******************************************************************************/
void Random(TUint64& a)
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



/*----------------------------------------------------------------------------*/
class MLock
	{
public:
	enum {EPollable=1, ETimeoutAvail=2, ENestable=4, ELimit1=8, ELooseTimeout=16};
public:
	virtual TInt Flags()=0;
	virtual void Release()=0;
	virtual void Wait()=0;
	virtual void Signal()=0;
	virtual TInt Wait(TInt aTimeout);
	virtual TInt Poll();
	};

TInt MLock::Wait(TInt)
	{ return KErrNotSupported; }
TInt MLock::Poll()
	{ return KErrNotSupported; }

/*----------------------------------------------------------------------------*/
class LockS : public MLock
	{
public:
	LockS();
	virtual TInt Flags();
	virtual void Release();
	virtual void Wait();
	virtual void Signal();
	virtual TInt Wait(TInt aTimeout);
	virtual TInt Poll();
public:
	RSemaphore	iT;
	};

LockS::LockS()
	{ test_KErrNone(iT.CreateLocal(1)); }
TInt LockS::Flags()
	{ return EPollable|ETimeoutAvail; }
void LockS::Release()
	{ iT.Close(); }
void LockS::Wait()
	{ iT.Wait(); }
void LockS::Signal()
	{ iT.Signal(); }
TInt LockS::Wait(TInt aTimeout)
	{ return iT.Wait(aTimeout); }
TInt LockS::Poll()
	{ return iT.Poll(); }

/*----------------------------------------------------------------------------*/
class LockM : public MLock
	{
public:
	LockM();
	virtual TInt Flags();
	virtual void Release();
	virtual void Wait();
	virtual void Signal();
	virtual TInt Wait(TInt aTimeout);
	virtual TInt Poll();
public:
	RMutex		iT;
	};

LockM::LockM()
	{ test_KErrNone(iT.CreateLocal()); }
TInt LockM::Flags()
	{ return EPollable|ETimeoutAvail|ENestable|ELimit1; }
void LockM::Release()
	{ iT.Close(); }
void LockM::Wait()
	{ iT.Wait(); }
void LockM::Signal()
	{ iT.Signal(); }
TInt LockM::Wait(TInt aTimeout)
	{ return iT.Wait(aTimeout); }
TInt LockM::Poll()
	{ return iT.Poll(); }

/*----------------------------------------------------------------------------*/

class LockFL : public MLock
	{
public:
	LockFL();
	virtual TInt Flags();
	virtual void Release();
	virtual void Wait();
	virtual void Signal();
	virtual TInt Wait(TInt aTimeout);
	virtual TInt Poll();
public:
	RFastLock	iT;
	};

LockFL::LockFL()
	{ test_KErrNone(iT.CreateLocal()); }
TInt LockFL::Flags()
	{ return ETimeoutAvail|EPollable|ELimit1|ELooseTimeout; }
void LockFL::Release()
	{ iT.Close(); }
void LockFL::Wait()
	{ iT.Wait(); }
void LockFL::Signal()
	{ iT.Signal(); }
TInt LockFL::Wait(TInt aTimeout)
	{ return iT.Wait(aTimeout); }
TInt LockFL::Poll()
	{ return iT.Poll(); }

/*----------------------------------------------------------------------------*/
class LockCS : public MLock
	{
public:
	LockCS();
	virtual TInt Flags();
	virtual void Release();
	virtual void Wait();
	virtual void Signal();
public:
	RCriticalSection iT;
	};

LockCS::LockCS()
	{ test_KErrNone(iT.CreateLocal()); }
TInt LockCS::Flags()
	{ return ELimit1; }
void LockCS::Release()
	{ iT.Close(); }
void LockCS::Wait()
	{ iT.Wait(); }
void LockCS::Signal()
	{ iT.Signal(); }


			 
/*----------------------------------------------------------------------------*/
class LFSR
	{
public:
	LFSR(TInt aBits, TInt aTap2, TInt aTap3=0, TInt aTap4=0);
	~LFSR();
	void Step();
	void Step(TInt aSteps);
	TBool operator==(const LFSR& a) const;
public:
	TUint32* iData;
	TInt iBits;
	TInt iTap2;
	TInt iTap3;
	TInt iTap4;
	TInt iNW;
	TInt iSh1;
	TInt iIx2;
	TInt iSh2;
	TInt iIx3;
	TInt iSh3;
	TInt iIx4;
	TInt iSh4;
	};

LFSR::LFSR(TInt aBits, TInt aTap2, TInt aTap3, TInt aTap4)
	{
	iBits = aBits;
	iTap2 = aTap2;
	iTap3 = aTap3;
	iTap4 = aTap4;
	iNW = (aBits + 31) >> 5;
	iData = (TUint32*)User::AllocZ(iNW*sizeof(TUint32));
	test(iData!=0);
	iData[0] = 1;
	iSh1 = (aBits-1)&31;
	iIx2 = (iTap2-1)>>5;
	iSh2 = (iTap2-1)&31;
	if (iTap3)
		{
		iIx3 = (iTap3-1)>>5;
		iSh3 = (iTap3-1)&31;
		}
	else
		{
		iIx3 = -1;
		iSh3 = 0;
		}
	if (iTap4)
		{
		iIx4 = (iTap4-1)>>5;
		iSh4 = (iTap4-1)&31;
		}
	else
		{
		iIx4 = -1;
		iSh4 = 0;
		}
	}

LFSR::~LFSR()
	{
	User::Free(iData);
	}

void LFSR::Step(TInt aSteps)
	{
	while (aSteps--)
		Step();
	}

void LFSR::Step()
	{
	TUint32 b = iData[iNW-1]>>iSh1;
	b ^= (iData[iIx2]>>iSh2);
	if (iIx3>=0)
		b ^= (iData[iIx3]>>iSh3);
	if (iIx4>=0)
		b ^= (iData[iIx4]>>iSh4);
	b &= 1;
	TInt i;
	for (i=0; i<iNW; ++i)
		{
		TUint32 bb = iData[i] >> 31;
		iData[i] = (iData[i]<<1)|b;
		b = bb;
		}
	iData[iNW-1] &= ((2u<<iSh1)-1u);
	}

TBool LFSR::operator==(const LFSR& a) const
	{
	if (iBits!=a.iBits || iTap2!=a.iTap2 || iTap3!=a.iTap3 || iTap4!=a.iTap4 || iNW!=a.iNW)
		return EFalse;
	if (iData==a.iData)
		return ETrue;
	if (memcompare((const TUint8*)iData, iNW, (const TUint8*)a.iData, a.iNW))
		return EFalse;
	return ETrue;
	}



/*----------------------------------------------------------------------------*/
class CStack
	{
public:	   
	CStack() {iCount=0;};
	void Push(TInt aItem) {iStack[iCount++]=aItem;};
	TInt Pop(void) {return(iStack[--iCount]);};
private:
	TInt iStack[KMaxBufferSize];
	TInt iCount;
	};
CStack stack;


TInt Producer(TAny*)
	{
	for(TInt ii=0;ii<KNumProducerItems;ii++)
		{
		slotAvailable.Wait();
		mutex.Wait();
		stack.Push(ii);
		mutex.Signal();
		itemAvailable.Signal();
		}
	return(KErrNone);
	}

TInt Consumer(TAny*)
	{
	TInt item;
	for(TInt ii=0;ii<KNumProducerItems;ii++)
		{
		itemAvailable.Wait();
		mutex.Wait();
		item=stack.Pop();
		mutex.Signal();
		slotAvailable.Signal();
		consumerArray[item]=item;
		}
	return(KErrNone);
	}

void BusyWait(TInt aMicroseconds)
	{
	TTime begin;
	begin.HomeTime();
	FOREVER
		{
		TTime now;
		now.HomeTime();
		TTimeIntervalMicroSeconds iv=now.MicroSecondsFrom(begin);
		if (iv.Int64()>=TInt64(aMicroseconds))
			return;
		}
	}

TInt MutexThreadEntryPoint1(TAny*)
//
// Mutex test thread 1
//
	{	
	TInt n = NumCpus();

	thread1Count=0;
	TBool running=ETrue;
	do
		{
		mutex.Wait();
		BusyWait(100000);
		if (arrayIndex<KMaxArraySize)
			{
			array[arrayIndex++]=EThread1ID;
			thread1Count++;
			}
		else
			running=EFalse;
		mutex.Signal();

		if (n > 1) 
			{
			// when the mutex is singaled, due to priority balancing, the other
			// thread will be scheduled to run on a CPU other than this one. The delay
			// in getting that thread to run means that this one can manage to re-claim the 
			// mutex before the other thread gets to run. So we add a small delay here 
			User::After(100); 
			}

		} while (running);
	return(KErrNone);
	}

TInt MutexThreadEntryPoint2(TAny*)
//
// Mutex test thread 2
//
	{
	TInt n = NumCpus();

	thread2Count=0;
	TBool running=ETrue;
	do
		{
		mutex.Wait();
		BusyWait(200000);
		if (arrayIndex<KMaxArraySize)
			{
			array[arrayIndex++]=EThread2ID;
			thread2Count++;
			}
		else
			running=EFalse;
		mutex.Signal();

		if (n > 1) 
			{
			// when the mutex is singaled, due to priority balancing, the other
			// thread will be scheduled to run on a CPU other than this one. The delay
			// in getting that thread to run means that this one can manage to re-claim the 
			// mutex before the other thread gets to run. So we add a small delay here 
			User::After(100); 
			}
		

		} while (running);
	return(KErrNone);
	}

TInt CriticalSnThreadEntryPoint1(TAny*)
//
// Critical Section test thread 1
//
	{	

	thread1Count=0;
	TBool running=ETrue;
	do
		{
		criticalSn.Wait();
		User::After(100000);
		if (arrayIndex<KMaxArraySize)
			{
			array[arrayIndex++]=EThread1ID;
			thread1Count++;
			}
		else
			running=EFalse;
		criticalSn.Signal();
		} while (running);
	return(KErrNone);
	}

TInt CriticalSnThreadEntryPoint2(TAny*)
//
// Critical Section test thread 2
//
	{

	thread2Count=0;
	TBool running=ETrue;
	do
		{
		criticalSn.Wait();
		User::After(200000);
		if (arrayIndex<KMaxArraySize)
			{
			array[arrayIndex++]=EThread2ID;
			thread2Count++;
			}
		else
			running=EFalse;
		criticalSn.Signal();
		} while (running);
	return(KErrNone);
	}


/*----------------------------------------------------------------------------*/
struct SWaitLock
	{
	enum {EDummy=-2, EPoll=-1, EInfinite=0};

	static TInt WaitLockThread(TAny*);
	void Start(RThread& aT, TThreadPriority aP=EPriorityLess);
	void Wait(RThread& aT, TInt aResult);
	TInt DoTest2(RThread& aT, TInt aTimeout, TInt aResult, TThreadPriority aP=EPriorityLess);
	void Test2();
	void TestSignalled();
	void TestNotSignalled();
	void TestState();


	MLock* iLock;
	TInt iTimeout;
	};

TInt SWaitLock::WaitLockThread(TAny* a)
	{
	
	if (doCpuLocking)
		{
		TInt r = LockCurrentThreadToCpu0();
		if (KErrNone!=r) return r;
		// Rendevous was requested
		RThread::Rendezvous(KErrNone);
		}
	
	SWaitLock& w = *(SWaitLock*)a;
	TInt lfl = w.iLock->Flags();
	TBool limit1 = lfl & MLock::ELimit1;
	TInt r;
	switch (w.iTimeout)
		{
		case EDummy:
			return KErrNone;
		case EPoll:
			r = w.iLock->Poll();
			break;
		case EInfinite:
			w.iLock->Wait();
			r = KErrNone;
			break;
		default:
			r = w.iLock->Wait(w.iTimeout);
			break;
		}
	if (limit1 && r==KErrNone)
		w.iLock->Signal();
	return r;
	}

void SWaitLock::Start(RThread& aT, TThreadPriority aP)
	{
	TRequestStatus st;
	TInt r = aT.Create(KNullDesC, &WaitLockThread, 0x1000, 0x1000, 0x1000, this);
	test_KErrNone(r);
	aT.SetPriority(aP);
	if (doCpuLocking) 
		{
		aT.Rendezvous(st);
		}
	aT.Resume();
	if (doCpuLocking) 
		{
		User::WaitForRequest(st);
		test_KErrNone(st.Int());
		}
	}

void SWaitLock::Wait(RThread& aT, TInt aResult)
	{
	TRequestStatus s;
	aT.Logon(s);
	User::WaitForRequest(s);
	test_Equal(EExitKill, aT.ExitType());
	test_Equal(aResult, aT.ExitReason());
	test_Equal(aResult, s.Int());
	CLOSE_AND_WAIT(aT);
	}

TInt SWaitLock::DoTest2(RThread& aT, TInt aTimeout, TInt aResult, TThreadPriority aP)
	{
	TTime initial;
	TTime final;
	iTimeout = aTimeout;
	initial.HomeTime();
	Start(aT, aP);
	Wait(aT, aResult);
	final.HomeTime();
	TInt elapsed = I64INT(final.Int64()-initial.Int64());
	return elapsed;
	}

void SWaitLock::TestSignalled()
	{
	TInt r = iLock->Poll();
	if (r == KErrNotSupported)
		r = iLock->Wait(1);
	test_KErrNone(r);
	}

void SWaitLock::TestNotSignalled()
	{
	TInt r = iLock->Poll();
	if (r == KErrNotSupported)
		r = iLock->Wait(1);
	test_Equal(KErrTimedOut, r);
	}

void SWaitLock::TestState()
	{
	if (iLock->Flags() & MLock::ELimit1)
		TestSignalled();	// not signalled afterwards
	else
		TestNotSignalled();
	}

void SWaitLock::Test2()
	{
	test.Start(_L("SWaitLock::Test2"));
	RThread t;
	RThread t2;
	TTime initial;
	TTime final;
	TInt elapsed = 0;
	TInt r = 0;
	TInt lfl = iLock->Flags();
	TBool nestable = lfl & MLock::ENestable;
	TBool limit1 = lfl & MLock::ELimit1;
	TBool pollable = lfl & MLock::EPollable;
	TBool to = lfl & MLock::ETimeoutAvail;
	TBool lto = lfl & MLock::ELooseTimeout;

	RThread().SetPriority(EPriorityAbsoluteVeryLow);
	TInt threadcount=0;
	iTimeout = EDummy;
	initial.HomeTime();
	while (elapsed<1000000)
		{
		Start(t, EPriorityMore);
		Wait(t, KErrNone);
		++threadcount;
		final.HomeTime();
		elapsed = I64INT(final.Int64()-initial.Int64());
		}
	RThread().SetPriority(EPriorityNormal);
	test.Printf(_L("%d threads in 1 sec\n"),threadcount);
	TInt overhead = 1000000/threadcount;
	test.Printf(_L("overhead = %dus\n"),overhead);

	iLock->Wait();

	if (to)
		{
		elapsed = DoTest2(t, 1000000, KErrTimedOut);
		test.Printf(_L("Time taken = %dus\n"), elapsed);
		test(elapsed>=900000+overhead && elapsed<1500000+overhead);
		elapsed = DoTest2(t, -99, KErrArgument);
		test.Printf(_L("Time taken = %dus\n"), elapsed);
		}

	if (pollable)
		{
		test.Printf(_L("Testing Poll() function\n"));
		r = iLock->Poll();
		test_Equal((nestable ? KErrNone : KErrTimedOut), r);
		if (nestable)
			{
			iTimeout=EPoll;
			r = iLock->Poll();
			test_KErrNone(r);
			iLock->Signal();
			Start(t, EPriorityMore);
			Wait(t, KErrTimedOut);
			}
		iLock->Signal();
		if (nestable)
			{
			iTimeout=EPoll;
			r = iLock->Poll();
			test_KErrNone(r);
			iLock->Signal();
			Start(t, EPriorityMore);
			Wait(t, KErrTimedOut);
			iLock->Signal();
			Start(t, EPriorityMore);
			Wait(t, KErrNone);
			}
		r = iLock->Poll();
		test_KErrNone(r);
		if (!nestable)
			{
			r = iLock->Poll();
			test_Equal(KErrTimedOut, r);
			iLock->Signal();
			if (!limit1)
				{
				iLock->Signal();
				r = iLock->Poll();
				test_KErrNone(r);
				}
			r = iLock->Poll();
			test_KErrNone(r);
			r = iLock->Poll();
			test_Equal(KErrTimedOut, r);
			}
		elapsed = DoTest2(t, EPoll, KErrTimedOut);
		test.Printf(_L("Time taken = %dus\n"), elapsed);
		test(elapsed<=50000+3*overhead);
		iLock->Signal();
		elapsed = DoTest2(t, EPoll, KErrNone);
		test.Printf(_L("Time taken = %dus\n"), elapsed);
		test(elapsed<=50000+3*overhead);
		TestState();
		iLock->Signal();
		r = LockCurrentThreadToCpu0(ETrue);
		test_KErrNone(r);
		Start(t, EPriorityMuchMore);
		Start(t2, EPriorityMore);
		test_Equal(EExitKill, t2.ExitType());
		test_Equal(EExitKill, t.ExitType());
		Wait(t2, limit1 ? KErrNone : KErrTimedOut);
		Wait(t, KErrNone);
		r = UnlockCurrentThreadToCpu0(ETrue);
		test_KErrNone(r);
		TestState();
		}
	else
		{
		test.Printf(_L("Poll() function not supported\n"));
		}

	if (to)
		{
		iTimeout=2000000;
		initial.HomeTime();
		Start(t);
		User::After(1000000);
		iLock->Signal();
		Wait(t, KErrNone);
		final.HomeTime();
		elapsed = I64INT(final.Int64()-initial.Int64());
		test.Printf(_L("Time taken = %dus\n"), elapsed);
		test(elapsed>=900000+overhead && elapsed<1500000+overhead);
		TestState();

		r = LockCurrentThreadToCpu0(ETrue);
		test_KErrNone(r);

		if (!lto)
			{
			iTimeout=100000;
			Start(t, EPriorityMore);
			t.Suspend();
			iLock->Signal();
			User::After(200000);
			t.Resume();
			Wait(t, KErrTimedOut);
			TestSignalled();

			iTimeout=100000;
			Start(t, EPriorityMore);
			t.Suspend();
			iLock->Signal();
			User::After(50000);
			t.Resume();
			Wait(t, KErrNone);
			TestState();

			iTimeout=100000;
			Start(t, EPriorityMuchMore);
			Start(t2, EPriorityMore);
			t.Suspend();
			iLock->Signal();
			test_Equal(EExitKill, t2.ExitType());
			test_Equal(EExitPending, t.ExitType());
			t.Resume();
			Wait(t, limit1 ? KErrNone : KErrTimedOut);
			Wait(t2, KErrNone);
			TestState();
			}

		iTimeout=1000000;
		initial.HomeTime();
		Start(t2, EPriorityMore);
		Start(t, EPriorityMuchMore);
		iLock->Signal();
		Wait(t, KErrNone);
		final.HomeTime();
		elapsed = I64INT(final.Int64()-initial.Int64());
		test.Printf(_L("Time taken = %dus\n"), elapsed);
		Wait(t2, limit1 ? KErrNone : KErrTimedOut);
		final.HomeTime();
		elapsed = I64INT(final.Int64()-initial.Int64());
		test.Printf(_L("Time taken = %dus\n"), elapsed);
		if (!limit1)
			{
			test(elapsed>=900000+2*overhead && elapsed<1500000+2*overhead);
			}
		TestState();

		iTimeout=1000000;
		initial.HomeTime();
		Start(t2, EPriorityMore);
		Start(t, EPriorityMuchMore);
		Wait(t, KErrTimedOut);
		final.HomeTime();
		elapsed = I64INT(final.Int64()-initial.Int64());
		test.Printf(_L("Time taken = %dus\n"), elapsed);
		Wait(t2, KErrTimedOut);
		final.HomeTime();
		elapsed = I64INT(final.Int64()-initial.Int64());
		test.Printf(_L("Time taken = %dus\n"), elapsed);
		test(elapsed>=900000+2*overhead && elapsed<1500000+2*overhead);

		iTimeout=1000000;
		initial.HomeTime();
		Start(t2, EPriorityMore);
		Start(t, EPriorityMuchMore);
		t.Kill(299792458);
		Wait(t2, KErrTimedOut);
		Wait(t, 299792458);
		final.HomeTime();
		elapsed = I64INT(final.Int64()-initial.Int64());
		test.Printf(_L("Time taken = %dus\n"), elapsed);
		test(elapsed>=900000+2*overhead && elapsed<1500000+2*overhead);

		iTimeout=1000000;
		initial.HomeTime();
		Start(t, EPriorityMore);
		Start(t2, EPriorityMuchMore);
		test_Equal(EExitPending, t.ExitType());
		test_Equal(EExitPending, t2.ExitType());
		iLock->Release();
		test_Equal(EExitKill, t.ExitType());
		test_Equal(EExitKill, t2.ExitType());
		Wait(t2, KErrGeneral);
		Wait(t, KErrGeneral);
		final.HomeTime();
		elapsed = I64INT(final.Int64()-initial.Int64());
		test.Printf(_L("Time taken = %dus\n"), elapsed);
		test(elapsed<=50000+3*overhead);
		r = UnlockCurrentThreadToCpu0(ETrue);
		test_KErrNone(r);
		}
	else
		{
		test.Printf(_L("Timed waits not supported\n"));
		iLock->Release();
		}
	test.End();
	}

volatile TBool NoRepeat = EFalse;
void TestPollTimeout()
	{
	SWaitLock w;
	do	{
		test.Printf(_L("TestPollTimeout - RSemaphore\n"));
		LockS ls;
		w.iLock = &ls;
		w.Test2();	// Release()s ls
		} while(NoRepeat);
	do	{
		test.Printf(_L("TestPollTimeout - RMutex\n"));
		LockM lm;
		w.iLock = &lm;
		w.Test2();	// Release()s lm
		} while(NoRepeat);
	do	{
		test.Printf(_L("TestPollTimeout - RFastLock\n"));
		LockFL fl;
		w.iLock = &fl;
		w.Test2();	// Release()s fl
		} while(NoRepeat);
	}


/*----------------------------------------------------------------------------*/
class CMXThreadGrp;

struct SStats
	{
	SStats();
	void Add(TInt aValue);
	void Add(const SStats& aS);
	TInt Count() const {return iN;}
	TInt Min() const;
	TInt Max() const;
	TInt Mean() const;

	TInt64	iSum;
	TInt	iMin;
	TInt	iMax;
	TInt	iN;
	TInt	iSp;
	};

SStats::SStats()
	{
	iSum = 0;
	iMax = KMinTInt;
	iMin = ~iMax;
	iN = 0;
	iSp = 0;
	}

void SStats::Add(TInt aValue)
	{
	TInt64 v = aValue;
	iSum += v;
	++iN;
	if (aValue > iMax)
		iMax = aValue;
	if (aValue < iMin)
		iMin = aValue;
	}

void SStats::Add(const SStats& a)
	{
	iN += a.iN;
	iSum += a.iSum;
	if (a.iMax > iMax)
		iMax = a.iMax;
	if (a.iMin < iMin)
		iMin = a.iMin;
	}

TInt SStats::Min() const
	{return iN ? iMin : 0;}

TInt SStats::Max() const
	{return iN ? iMax : 0;}

TInt SStats::Mean() const
	{
	if (iN==0)
		return 0;
	return (TInt)(iSum/TInt64(iN));
	}

TUint32 ticks_to_us(TUint32 aTicks, TUint32 aF)
	{
	TUint64 x = aTicks;
	TUint64 f = aF;
	x *= TUint64(1000000);
	x += (f>>1);
	x /= f;
	return I64LOW(x);
	}

class CMXThread : public CBase
	{
private:
	CMXThread();
	~CMXThread();
	static CMXThread* New(CMXThreadGrp* aG, TUint32 aId, TUint32 aL, TUint32 aD);
	void Start();
	void Wait();
	TInt Construct(CMXThreadGrp* aG, TUint32 aId, TUint32 aL, TUint32 aD);
	TInt Steps();
	TInt Action();
	TInt Run();
	static TInt ThreadFunc(TAny*);
	void PrintStats();
private:
	TUint64 iSeed;
	RThread	iThread;
	TRequestStatus iExitStatus;
	CMXThreadGrp* iG;
	LFSR* iDummyLfsr;
	TUint32 iId;
	TUint32 iLambda;
	TUint32 iDummySteps;
	TInt iTotalSteps;
	TInt iIterations;
	TInt iPolls;
	TInt iPollFails;
	SStats iStats;
	SStats iTimeoutStats;
private:
	friend class CMXThreadGrp;
	};

class CMXThreadGrp : public CBase
	{
public:
	static CMXThreadGrp* New(MLock* aLock, TInt aNThreads, TUint32 aLambda, TUint32 aDummySteps, TUint32 aTime);
	CMXThreadGrp();
	~CMXThreadGrp();
	TBool Run();
	void PrintStats();
private:
	TInt Construct(MLock* aLock, TInt aNThreads, TUint32 aLambda, TUint32 aDummySteps, TUint32 aTime);
private:
	TInt iNThreads;
	CMXThread** iThreads;
	MLock* iLock;
	LFSR* iLfsr;
	LFSR* iLfsr0;
	TUint32 iNTickPeriod;
	TUint32 iFCF;
	TUint32 iNTicks;
	TInt iTotalSteps;
	TInt iIterations;
	TInt iPolls;
	TInt iPollFails;
	SStats iStats;
	SStats iTimeoutStats;
private:
	friend class CMXThread;
	};

CMXThread::CMXThread()
	{
	iThread.SetHandle(0);
	}

CMXThread::~CMXThread()
	{
	delete iDummyLfsr;
	if (iThread.Handle())
		{
		if (iThread.ExitType() == EExitPending)
			{
			iThread.Kill(0);
			Wait();
			}
		CLOSE_AND_WAIT(iThread);
		}
	}

void CMXThread::PrintStats()
	{
	test.Printf(_L("Thread %d:\n"), iId);
	test.Printf(_L(" ST:%10d IT:%10d P:%10d PF:%10d TO:%10d\n"), iTotalSteps, iIterations, iPolls, iPollFails, iTimeoutStats.Count());
	TUint32 min, max, mean;
	min = ticks_to_us(iStats.Min(), iG->iFCF);
	max = ticks_to_us(iStats.Max(), iG->iFCF);
	mean = ticks_to_us(iStats.Mean(), iG->iFCF);
	test.Printf(_L(" Lock acquire times MIN %10d MAX %10d AVG %10d\n"), min, max, mean);
	min = ticks_to_us(iTimeoutStats.Min(), iG->iFCF);
	max = ticks_to_us(iTimeoutStats.Max(), iG->iFCF);
	mean = ticks_to_us(iTimeoutStats.Mean(), iG->iFCF);
	test.Printf(_L(" Lock timeout times MIN %10d MAX %10d AVG %10d\n"), min, max, mean);
	}

TInt CMXThread::Construct(CMXThreadGrp* aG, TUint32 aId, TUint32 aL, TUint32 aD)
	{
	iG = aG;
	iId = aId;
	iLambda = aL;
	iDummySteps = aD;
	iSeed = iId + 1;
	iDummyLfsr = new LFSR(785,693);
	if (!iDummyLfsr)
		return KErrNoMemory;
	TBuf<16> name = _L("TSThrd");
	name.AppendNum(iId);
	TInt r = iThread.Create(name, &ThreadFunc, 0x1000, NULL, this);
	if (r!=KErrNone)
		return r;
	iThread.Logon(iExitStatus);
	if (iExitStatus != KRequestPending)
		{
		iThread.Kill(0);
		iThread.Close();
		iThread.SetHandle(0);
		return iExitStatus.Int();
		}
	iThread.SetPriority(EPriorityLess);
	return KErrNone;
	}

CMXThread* CMXThread::New(CMXThreadGrp* aG, TUint32 aId, TUint32 aL, TUint32 aD)
	{
	CMXThread* p = new CMXThread;
	if (p)
		{
		TInt r = p->Construct(aG, aId, aL, aD);
		if (r != KErrNone)
			{
			delete p;
			p = 0;
			}
		}
	return p;
	}

void CMXThread::Start()
	{
	iThread.Resume();
	}

void CMXThread::Wait()
	{
	User::WaitForRequest(iExitStatus);
	}

TInt CMXThread::ThreadFunc(TAny* aPtr)
	{
	CMXThread& a = *(CMXThread*)aPtr;
	return a.Run();
	}

TInt CMXThread::Steps()
	{
	Random(iSeed);
	return ExpRV(iSeed, iLambda, 1);
	}

TInt CMXThread::Action()
	{
	Random(iSeed);
	return I64LOW(iSeed)%3;
	}

TInt CMXThread::Run()
	{
	MLock* lock = iG->iLock;
	LFSR* lfsr = iG->iLfsr;
	TInt lfl = lock->Flags();
	TBool pollable = lfl & MLock::EPollable;
	TBool to = lfl & MLock::ETimeoutAvail;
	TUint32 start_time = User::NTickCount();
	TInt r;

	FOREVER
		{
		TUint32 now = User::NTickCount();
		if (now - start_time >= iG->iNTicks)
			break;
		++iIterations;
		iDummyLfsr->Step(iDummySteps);
		TInt action = Action();
		TInt steps = Steps();
		TUint32 initial = User::FastCounter();
		if (action==2 && to)
			{
			r = lock->Wait(1000);
			if (r!=KErrNone)
				{
				TUint32 final = User::FastCounter();
				TInt elapsed = TInt(final - initial);
				iTimeoutStats.Add(elapsed);
				}
			}
		else if (action==1 && pollable)
			{
			++iPolls;
			r = lock->Poll();
			if (r!=KErrNone)
				++iPollFails;
			}
		else
			{
			lock->Wait();
			r = KErrNone;
			}
		if (r == KErrNone)
			{
			TUint32 final = User::FastCounter();
			lfsr->Step(steps);
			lock->Signal();
			TInt elapsed = TInt(final - initial);
			iTotalSteps += steps;
			iStats.Add(elapsed);
			}
		}

	return KErrNone;
	}

CMXThreadGrp* CMXThreadGrp::New(MLock* aLock, TInt aNThreads, TUint32 aLambda, TUint32 aDummySteps, TUint32 aTime)
	{
	CMXThreadGrp* p = new CMXThreadGrp;
	if (p)
		{
		TInt r = p->Construct(aLock, aNThreads, aLambda, aDummySteps, aTime);
		if (r != KErrNone)
			{
			delete p;
			p = 0;
			}
		}
	return p;
	}

CMXThreadGrp::CMXThreadGrp()
	{
	}

TInt CMXThreadGrp::Construct(MLock* aLock, TInt aNThreads, TUint32 aLambda, TUint32 aDummySteps, TUint32 aTime)
	{
	iNThreads = aNThreads;
	iLock = aLock;
	TInt r = HAL::Get(HAL::EFastCounterFrequency, (TInt&)iFCF);
	if (r!=KErrNone)
		return r;
	r = HAL::Get(HAL::ENanoTickPeriod, (TInt&)iNTickPeriod);
	if (r!=KErrNone)
		return r;
	iNTicks = (aTime+iNTickPeriod-1)/iNTickPeriod;
	iLfsr = new LFSR(785,693);
	iLfsr0 = new LFSR(785,693);
	if (!iLfsr || !iLfsr0)
		return KErrNoMemory;
	iThreads = (CMXThread**)User::AllocZ(iNThreads*sizeof(CMXThread*));
	if (!iThreads)
		return KErrNoMemory;
	TInt i;
	for (i=0; i<iNThreads; ++i)
		{
		iThreads[i] = CMXThread::New(this, i, aLambda, aDummySteps);
		if (!iThreads[i])
			return KErrNoMemory;
		}
	return KErrNone;
	}

CMXThreadGrp::~CMXThreadGrp()
	{
	delete iLfsr;
	delete iLfsr0;
	if (iThreads)
		{
		TInt i;
		for (i=0; i<iNThreads; ++i)
			delete iThreads[i];
		}
	User::Free(iThreads);
	}

TBool CMXThreadGrp::Run()
	{
	TInt i;
	test.Printf(_L("Starting test with N=%d L=%d D=%d T=%d\n"), iNThreads, iThreads[0]->iLambda, iThreads[0]->iDummySteps, iNTicks);
	for (i=0; i<iNThreads; ++i)
		iThreads[i]->Start();
	for (i=0; i<iNThreads; ++i)
		iThreads[i]->Wait();
	for (i=0; i<iNThreads; ++i)
		{
		iTotalSteps += iThreads[i]->iTotalSteps;
		iIterations += iThreads[i]->iIterations;
		iPolls += iThreads[i]->iPolls;
		iPollFails += iThreads[i]->iPollFails;
		iStats.Add(iThreads[i]->iStats);
		iTimeoutStats.Add(iThreads[i]->iTimeoutStats);
		}
	test.Printf(_L("Total LFSR steps %d\n"), iTotalSteps);
	iLfsr0->Step(iTotalSteps);
	TBool ok = (*iLfsr == *iLfsr0);
	return ok;
	}

void CMXThreadGrp::PrintStats()
	{
	TInt i;
	for (i=0; i<iNThreads; ++i)
		{
		iThreads[i]->PrintStats();
		}
	test.Printf(_L("TOTALS:\n"));
	test.Printf(_L(" ST:%10d IT:%10d P:%10d PF:%10d TO:%10d\n"), iTotalSteps, iIterations, iPolls, iPollFails, iTimeoutStats.Count());
	TUint32 min, max, mean;
	min = ticks_to_us(iStats.Min(), iFCF);
	max = ticks_to_us(iStats.Max(), iFCF);
	mean = ticks_to_us(iStats.Mean(), iFCF);
	test.Printf(_L(" Lock acquire times MIN %10d MAX %10d AVG %10d\n"), min, max, mean);
	min = ticks_to_us(iTimeoutStats.Min(), iFCF);
	max = ticks_to_us(iTimeoutStats.Max(), iFCF);
	mean = ticks_to_us(iTimeoutStats.Mean(), iFCF);
	test.Printf(_L(" Lock timeout times MIN %10d MAX %10d AVG %10d\n"), min, max, mean);
	}

TUint32 Calibrate()
	{
	TUint32 fcf;
	TInt r = HAL::Get(HAL::EFastCounterFrequency, (TInt&)fcf);
	test_KErrNone(r);
	LFSR* d = new LFSR(785,693);
	test(d!=0);
	TInt steps = 2;
	TUint32 ticks = fcf/10;
	TUint32 elapsed;
	FOREVER
		{
		TUint32 h0 = User::FastCounter();
		d->Step(steps);
		TUint32 h1 = User::FastCounter();
		elapsed = h1 - h0;
		if (elapsed > ticks)
			break;
		steps *= 2;
		}
	delete d;
	test.Printf(_L("%d steps in %d fast ticks\n"), steps, elapsed);
	TUint64 x = elapsed;
	TUint64 s = steps;
	TUint64 y = fcf;
	y /= x;
	s *= y;	// steps per second
	TUint32 res = I64LOW(s);
	test.Printf(_L("%d steps per second\n"), res);
	return res;
	}

void DoTMX(MLock* aLock, TInt aNThreads, TUint32 aLambda, TUint32 aDummySteps, TUint32 aTime, TBool aShouldFail=EFalse)
	{
	CMXThreadGrp* g = CMXThreadGrp::New(aLock, aNThreads, aLambda, aDummySteps, aTime);
	test(g!=0);
	TBool ok = g->Run();
	if (aShouldFail)
		{
		test(!ok);
		}
	else
		{
		test(ok);
		}
	g->PrintStats();
	delete g;
	}

void DoTMX(MLock* aLock, TUint32 aLambda, TUint32 aDummySteps, TUint32 aTime)
	{
	TInt n;
	for (n=1; n<=4; ++n)
		{
		TUint32 l = (n<2) ? aLambda : (aLambda/(n-1));
		DoTMX(aLock, n, l, aDummySteps, aTime);
		}
	aLock->Release();
	}


void TestMutualExclusion()
	{
	TInt ntp;
	TInt r = HAL::Get(HAL::ENanoTickPeriod, ntp);
	test_KErrNone(r);
	test.Printf(_L("Nanokernel tick period = %dus\n"), ntp);
	TUint32 sps = Calibrate();
	TUint32 lambda = sps/2000;
	TUint32 dummy = sps/2000;
	TUint32 time = 5000000;
	do	{
		test.Printf(_L("TestMutualExclusion - RSemaphore\n"));
		LockS ls;
		DoTMX(&ls, lambda, dummy, time);
		} while(NoRepeat);
	do	{
		test.Printf(_L("TestMutualExclusion - RSemaphore init=2\n"));
		LockS ls2;
		ls2.Signal();	// count=2
		DoTMX(&ls2, 4, lambda, dummy, time, ETrue);
		} while(NoRepeat);
	do	{
		test.Printf(_L("TestMutualExclusion - RMutex\n"));
		LockM lm;
		DoTMX(&lm, lambda, dummy, time);
		} while(NoRepeat);
	do	{
		test.Printf(_L("TestMutualExclusion - RFastLock\n"));
		LockFL fl;
		DoTMX(&fl, lambda, dummy, time);
		} while(NoRepeat);
	do	{
		test.Printf(_L("TestMutualExclusion - RCriticalSection\n"));
		LockCS cs;
		DoTMX(&cs, lambda, dummy, time);
		} while(NoRepeat);
	}




/*----------------------------------------------------------------------------*/
void TestSemaphore()
	{
/*********** TO DO ************/
// Check it panics if the count <0

	test.Start(_L("Create"));
	RSemaphore semaphore;
	RThread thread1, thread2;

	semaphore.CreateLocal(0); 	// creates a DPlatSemaphore but casts it to a pointer to a DSemaphore
								// sets semaphore count to the value of the parameter, 
								// adds object to the K::Semaphores container, sets iHandle
								// Local sets DSemaphore.iName to NULL & iOwner to Kern::CurrentProcess()
								// Global sets iName to that passed and iOwner to NULL
								// Adds a record into CObjectIx containing a pointer to the semaphore object
/*	test.Next(_L("Find"));
	fullName=semaphore.FullName();	
	find.Find(fullName);	// sets iMatch to fullName	(misleadingly named method as it doesn't find anything)
	test(find.Next(fullName)== KErrNone);	
*/
	test.Next(_L("Producer/Consumer scenario"));
	// Test Rsemaphore with the producer/consumer scenario	RThread thread1, thread2;
	TRequestStatus stat1, stat2;
	test(mutex.CreateLocal()==KErrNone);
	test(slotAvailable.CreateLocal(KMaxBufferSize)==KErrNone);
	test(itemAvailable.CreateLocal(0)==KErrNone);
	test(thread1.Create(_L("Thread1"),Producer,KDefaultStackSize,0x200,0x200,NULL)==KErrNone);
	test(thread2.Create(_L("Thread2"),Consumer,KDefaultStackSize,0x200,0x200,NULL)==KErrNone);
	thread1.Logon(stat1);
	thread2.Logon(stat2);
	test(stat1==KRequestPending);
	test(stat2==KRequestPending);
	thread1.Resume(); 
	thread2.Resume();
	User::WaitForRequest(stat1);
	User::WaitForRequest(stat2);
	test(stat1==KErrNone);
	test(stat2==KErrNone);
	for(TInt jj=0;jj<KNumProducerItems;jj++)
		test(consumerArray[jj]==jj);		
	
	test.Next(_L("Close"));
	mutex.Close();
	CLOSE_AND_WAIT(thread1);
	CLOSE_AND_WAIT(thread2);
	test.End();
	}

void TestMutex2()
	{
	RMutex m;
	test.Start(_L("Create"));
	test(m.CreateLocal()==KErrNone);

	// Test RMutex::IsHeld()
	test.Next(_L("IsHeld ?"));
	test(!m.IsHeld());
	test.Next(_L("Wait"));
	m.Wait();
	test.Next(_L("IsHeld ?"));
	test(m.IsHeld());
	test.Next(_L("Signal"));
	m.Signal();
	test.Next(_L("IsHeld ?"));
	test(!m.IsHeld());

	test.End();
	}

void TestMutex()
	{
	test.Start(_L("Create"));
	test(mutex.CreateLocal()==KErrNone);
	
	test.Next(_L("Threads writing to arrays test"));
//
// Create two threads which write to two arrays. The arrays and indexs
// are global and each thread writes an identifier to the arrays. For
// one array the writing and updating of the index is wrapped in a mutex
// pair. The other array is a control and is not wrapaped within mutex.
// Each thread records the number of instances it "thinks" it wrote to
// each array. For the mutex controlled array the actual instances
// written to the array should always be the same as the threads think.
//
	arrayIndex=0;
	RThread thread1,thread2;	
	test(thread1.Create(_L("Thread1"),MutexThreadEntryPoint1,KDefaultStackSize,0x2000,0x2000,NULL)==KErrNone);
	test(thread2.Create(_L("Thread2"),MutexThreadEntryPoint2,KDefaultStackSize,0x2000,0x2000,NULL)==KErrNone);			 
	TRequestStatus stat1,stat2;
	thread1.Logon(stat1);
	thread2.Logon(stat2);
	test(stat1==KRequestPending);
	test(stat2==KRequestPending);
	thread1.Resume(); 
	thread2.Resume();
	User::WaitForRequest(stat1);
	User::WaitForRequest(stat2);
	test(stat1==KErrNone);
	test(stat2==KErrNone); 
	TInt thread1ActualCount=0; 
	TInt thread2ActualCount=0;
	TInt ii=0;
	while(ii<KMaxArraySize)
		{
		if (array[ii]==EThread1ID)
			thread1ActualCount++;
		if (array[ii]==EThread2ID)
			thread2ActualCount++;
		ii++;
		}
	test.Printf(_L("T1 %d T1ACT %d T2 %d T2ACT %d\n"),thread1Count,thread1ActualCount,thread2Count,thread2ActualCount);
	test(thread1ActualCount==thread1Count);
	test(thread2ActualCount==thread2Count);
	test(thread1Count==thread2Count);
	test(thread1Count==(KMaxArraySize>>1));
	
	test.Next(_L("Close"));
	CLOSE_AND_WAIT(thread1);
	CLOSE_AND_WAIT(thread2);
	mutex.Close();
	test.End();
	}

void TestCriticalSection()
//
//As TestMutex, but for RCriticalSection
//
	{
	
	test.Start(_L("Create"));
	test(criticalSn.CreateLocal()==KErrNone);

/***************** TO DO ***********************

	test.Next(_L("Find"));
//
// Test finding the RCriticalSection
//
	TFindCriticalSection find;
	TFullName fullName;
	fullName=criticalSn.FullName();
	find.Find(fullName);
	test(find.Next(fullName)==KErrNone);
	test(fullName==criticalSn.FullName());

************************************************/

	test.Next(_L("Threads writing to arrays test"));
//
// Create two threads which write to two arrays. The arrays and indexs
// are global and each thread writes an identifier to the arrays. For
// one array the writing and updating of the index is wrapped in a critical
// section pair. The other array is a control and is not wrapaped within
// a critical section. Each thread records the number of instances it
// "thinks" it wrote to each array. For the mutex controlled array the
// actual instances written to the array should always be the same as the
// threads think.
//
	arrayIndex=0;

	RThread thread1,thread2;	
	test(thread1.Create(_L("Thread1"),CriticalSnThreadEntryPoint1,KDefaultStackSize,0x2000,0x2000,NULL)==KErrNone);
	test(thread2.Create(_L("Thread2"),CriticalSnThreadEntryPoint2,KDefaultStackSize,0x2000,0x2000,NULL)==KErrNone);			 
	TRequestStatus stat1,stat2;
	thread1.Logon(stat1);
	thread2.Logon(stat2);
	test(stat1==KRequestPending);
	test(stat2==KRequestPending);
	thread1.Resume(); 
	thread2.Resume();
	User::WaitForRequest(stat1);
	User::WaitForRequest(stat2);
	test(stat1==KErrNone);
	test(stat2==KErrNone); 
	TInt thread1ActualCount=0; 
	TInt thread2ActualCount=0;
	TInt ii=0;
	while(ii<KMaxArraySize)
		{
		if (array[ii]==EThread1ID)
			thread1ActualCount++;
		if (array[ii]==EThread2ID)
			thread2ActualCount++;
		ii++;
		}
	test(thread1ActualCount==thread1Count);
	test(thread2ActualCount==thread2Count);
	test(thread1Count==thread2Count);
	test(thread1Count==(KMaxArraySize>>1));

	test.Next(_L("Close"));
	CLOSE_AND_WAIT(thread1);
	CLOSE_AND_WAIT(thread2);
	criticalSn.Close();
	test.End();
	}


GLDEF_C TInt E32Main()
	{	

	test.Title();
 	__UHEAP_MARK;
	TestMutualExclusion();
	TestPollTimeout();
	test.Start(_L("Test RSemaphore"));
	TestSemaphore();
	test.Next(_L("Test RMutex"));
	TestMutex();
	TestMutex2();
	test.Next(_L("Test RCriticalSection"));
	TestCriticalSection();
	test.End();
	__UHEAP_MARKEND;
	return(KErrNone);
	}


