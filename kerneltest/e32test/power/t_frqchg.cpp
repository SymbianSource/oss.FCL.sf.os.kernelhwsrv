// Copyright (c) 2010-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\power\t_frqchg.cpp
//
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32math.h>
#include <e32atomics.h>
#include <hal.h>
#include "d_frqchg.h"
#include <e32svr.h>
#include "u32std.h"

RFrqChg Driver;
RTest test(_L("T_FRQCHG"));

// test will fail if slice is > (expected+KSliceDeltaPercent%of expexted) 
// or < (expected-KSliceDeltaPercent%expected)
const TInt KSliceDeltaPercent = 5;   
// test will fail for global timer based timestamps if interval measured 
// is > (expected+KTimeStampDeltaPercent%of expexted) 
// or < (expected-KTimeStampDeltaPercent%expected)
const TInt KTimeStampDeltaPercent = 5;

TInt RealToRatio(SRatio& aRatio, const TRealX& aReal)
	{
	aRatio.iSpare1 = 0;
	aRatio.iSpare2 = 0;
	if (aReal.iSign || aReal.IsZero() || aReal.IsNaN())
		{
		aRatio.iM = 0;
		aRatio.iX = 0;
		return (aReal.IsZero()) ? KErrNone : KErrNotSupported;
		}
	TRealX rx(aReal);
	TRealX rr(rx);
	rr.iExp -= 32;
	rr.iMantLo = 0;
	rr.iMantHi = 0x80000000u;
	rx += rr;	// rounding
	TInt exp = rx.iExp - 32767 - 31;
	if (exp < -32768)
		{
		aRatio.iM = 0;
		aRatio.iX = 0;
		return KErrUnderflow;
		}
	if (exp > 32767)
		{
		aRatio.iM = 0xffffffffu;
		aRatio.iX = 32767;
		return KErrOverflow;
		}
	aRatio.iM = rx.iMantHi;
	aRatio.iX = (TInt16)exp;
	return KErrNone;
	}

TInt RatioToReal(TRealX& a, const SRatio& aRatio)
	{
	a.iSign = 0;
	a.iFlag = 0;
	a.iMantLo = 0;
	a.iMantHi = aRatio.iM;
	if (!aRatio.iM)
		{
		a.SetZero();
		return KErrNone;
		}
	TInt exp = aRatio.iX + 31 + 32767;
	if (exp > 65534)
		{
		a.SetInfinite(EFalse);
		}
	else
		{
		a.iExp = (TUint16)exp;
		}
	return KErrNone;
	}

TInt RatioSetValue(TRealX& a, TUint32 aInt, TInt aDivisorExp)
	{
	a.Set(TUint(aInt));
	TInt exp = a.iExp;
	exp -= aDivisorExp;
	if (exp<1)
		{
		a.SetZero();
		return KErrUnderflow;
		}
	if (exp>65534)
		{
		a.SetInfinite(EFalse);
		return KErrOverflow;
		}
	a.iExp = (TInt16)exp;
	return KErrNone;
	}

TInt RatioReciprocal(SRatio& aRatio)
	{
	TRealX rx;
	TInt r = RatioToReal(rx, aRatio);
	if (r != KErrNone)
		return r;
	rx = TRealX(1) / rx;
	return RealToRatio(aRatio, rx);
	}

TInt RatioMult(const SRatio& aRatio, TUint32& aInt32)
	{
	TRealX rx;
	TInt r = RatioToReal(rx, aRatio);
	if (r != KErrNone)
		return r;
	r = rx.MultEq(TRealX((TUint)aInt32));
	if (r != KErrNone)
		return r;
	if (rx.IsZero())
		{
		aInt32 = 0;
		return KErrNone;
		}
	rx.AddEq(TRealX(0.5));
	if (rx<TRealX(1))
		{
		aInt32 = 0;
		return KErrUnderflow;
		}
	if (rx.iExp > 32767+31)
		{
		aInt32 = ~0u;
		return KErrOverflow;
		}
	aInt32 = rx.operator TUint();
	return KErrNone;
	}

void RatioPrint(const char* aTitle, const SRatio& aRatio)
	{
	TPtrC8 t8((const TUint8*)aTitle);
	TBuf<256> t16;
	t16.Copy(t8);
	test.Printf(_L("%S: %08x %04x\n"), &t16, aRatio.iM, TUint16(aRatio.iX));
	}

void RatioPrint2(const char* aTitle, const SRatio& aR1, const SRatio& aR2)
	{
	TPtrC8 t8((const TUint8*)aTitle);
	TBuf<256> t16;
	t16.Copy(t8);
	test.Printf(_L("%S: %08x %04x   %08x %04x\n"), &t16, aR1.iM, TUint16(aR1.iX), aR2.iM, TUint16(aR2.iX));
	}

void TestEqual(const SRatio& aActual, const SRatio& aExpected)
	{
	if (aActual.iM==aExpected.iM && aActual.iX==aExpected.iX)
		return;
	RatioPrint("Actual", aActual);
	RatioPrint("Expected", aExpected);
	test(0);
	}

const TUint32 MultTestIntegers[] =
	{
	0u, 1u, 2u, 3u, 5u, 7u, 11u, 13u, 17u, 19u, 23u, 29u, 31u, 37u, 41u, 43u, 47u,
	50u, 51u, 53u, 59u, 61u, 63u, 67u, 71u, 72u, 81u, 100u, 127u, 133u, 187u, 200u,
	4u, 8u, 16u, 32u, 64u, 128u, 256u, 512u, 1024u, 2048u, 4096u, 8192u, 16384u,
	32768u, 65536u, 131072u, 262144u, 524288u, 1048576u, 2097152u, 4194304u, 8388608u,
	16777216u, 33554432u, 67108864u, 134217728u, 268435456u, 536870912u, 1073741824u,
	2147483648u, 4294967295u,
	9u, 27u, 243u, 729u, 2187u, 6561u, 19683u, 59049u, 177147u, 531441u, 1594323u,
	4782969u, 14348907u, 43046721u, 129140163u, 387420489u, 1162261467u, 3486784401u,
	25u, 125u, 625u, 3125u, 15625u, 78125u, 390625u, 1953125u, 9765625u,
	48828125u, 244140625u, 1220703125u,
	49u, 343u, 2401u, 16807u, 117649u, 823543u, 5764801u, 40353607u, 282475249u, 1977326743u
	};

void Test1M(const SRatio& aRatio)
	{
	SRatio ratio = aRatio;
	const TInt N = sizeof(MultTestIntegers)/sizeof(MultTestIntegers[0]);
	test.Printf(_L("Testing %d integers\n"), N);
	TInt i;
	for (i=0; i<N; ++i)
		{
		TUint32 I = MultTestIntegers[i];
		TUint32 I0 = I;
		TUint32 I1 = I;
		TInt r0 = RatioMult(aRatio, I0);
		TInt r1 = Driver.RatioMult(ratio, I1);
		if (r0!=KErrNone || r1!=KErrNone)
			{
			if (r0!=r1)
				{
				test.Printf(_L("Return code mismatch r0=%d r1=%d (I=%08x I0=%08x I1=%08x)\n"), r0, r1, I, I0, I1);
				test(0);
				}
			}
		else if (I0!=I1)
			{
			test.Printf(_L("Result mismatch I=%08x I0=%08x I1=%08x\n"), I, I0, I1);
			}
		}
	}

void Test1(TUint32 aInt, TInt aDivisorExp)
	{
	TRealX realx;
	SRatio r0x;
	SRatio r0;
	SRatio r1x;
	SRatio r1;
	TInt r;
	test.Printf(_L("Test1 %08x %d\n"), aInt, aDivisorExp);
	r = RatioSetValue(realx, aInt, aDivisorExp);
	test_KErrNone(r);
	r = RealToRatio(r0x, realx);
	test_KErrNone(r);
	r = Driver.RatioSet(r0, aInt, aDivisorExp);
	RatioPrint2("R0X,R0", r0x, r0);
	TestEqual(r0, r0x);
	Test1M(r0);
	r1x = r0x;
	r = RatioReciprocal(r1x);
	test_KErrNone(r);
	r1 = r0;
	r = Driver.RatioReciprocal(r1);
	test_KErrNone(r);
	RatioPrint2("R1X,R1", r1x, r1);
	TestEqual(r1, r1x);
	Test1M(r1);
	}

void TestRatios()
	{
	Test1(1,0);
	Test1(3,0);
	Test1(0xb504f334u,32);
	Test1(0xc90fdaa2u,30);
	Test1(10,0);
	Test1(0xcccccccd,35);
	Test1(100,0);
	Test1(0xa3d70a3d,38);
	}

class CircBuf
	{
public:
	static CircBuf* New(TInt aSlots);
	CircBuf();
	~CircBuf();
	TInt TryPut(TUint32 aIn);
	void Reset();
public:
	volatile TUint32* iBufBase;
	TUint32 iSlotCount;
	volatile TUint32 iPutIndex;
	};

CircBuf* CircBuf::New(TInt aSlots)
	{
	test(TUint32(aSlots-1)<65536);
	CircBuf* p = new CircBuf();
	p->iSlotCount = aSlots;
	p->iPutIndex = 0;
	p->iBufBase = (TUint32*)User::Alloc(aSlots*sizeof(TUint32));
	if (!p->iBufBase)
		{
		delete p;
		p = 0;
		}
	__e32_memory_barrier();
	return p;
	}

CircBuf::CircBuf()
	{
	iBufBase = 0;
	}

CircBuf::~CircBuf()
	{
	User::Free((TAny*)iBufBase);
	}

TInt CircBuf::TryPut(TUint32 aIn)
	{
	TUint32 orig = __e32_atomic_tau_rlx32(&iPutIndex, iSlotCount, 0, 1);
	if (orig == iSlotCount)
		return KErrOverflow;
	iBufBase[orig] = aIn;
	return KErrNone;
	}

void CircBuf::Reset()
	{
	__e32_atomic_store_ord32(&iPutIndex, 0);
	}



class CTimesliceTestThread : public CBase
	{
public:
	CTimesliceTestThread();
	~CTimesliceTestThread();
	static CTimesliceTestThread* New(TUint32 aId, TInt aCpu, TInt aSlice, CircBuf* aBuf);
	void Start();
	void Wait();
	TBool Finished();
	TInt Construct(TUint32 aId, TInt aCpu, TInt aSlice, CircBuf* aBuf);
	static TInt ThreadFunc(TAny*);
public:
	RThread	iThread;
	TRequestStatus iExitStatus;
	TUint32 iId;
	CircBuf* iBuf;
	TUint32 iFreq;
	TUint32 iThresh;
	TUint32 iThresh2;
	TInt iCpu;
	TInt iSlice;
	};

CTimesliceTestThread::CTimesliceTestThread()
	{
	iThread.SetHandle(0);
	}

CTimesliceTestThread::~CTimesliceTestThread()
	{
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

TInt CTimesliceTestThread::Construct(TUint32 aId, TInt aCpu, TInt aSlice, CircBuf* aBuf)
	{
	iId = aId;
	iCpu = aCpu;
	iSlice = aSlice;
	iBuf = aBuf;

	TInt r = HAL::Get(HAL::EFastCounterFrequency, (TInt&)iFreq);
	if (r!=KErrNone)
		return r;
	iThresh = iFreq / 3000;
	if (iThresh < 10)
		iThresh = 10;
	iThresh2 = iFreq;
	TBuf<16> name = _L("TSThrd");
	name.AppendNum(iId);
	r = iThread.Create(name, &ThreadFunc, 0x1000, NULL, this);
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
	return KErrNone;
	}

CTimesliceTestThread* CTimesliceTestThread::New(TUint32 aId, TInt aCpu, TInt aSlice, CircBuf* aBuf)
	{
	CTimesliceTestThread* p = new CTimesliceTestThread;
	if (p)
		{
		TInt r = p->Construct(aId, aCpu, aSlice, aBuf);
		if (r != KErrNone)
			{
			delete p;
			p = 0;
			}
		}
	return p;
	}

void CTimesliceTestThread::Start()
	{
	iThread.Resume();
	}

TBool CTimesliceTestThread::Finished()
	{
	return (KRequestPending!=iExitStatus.Int());
	}

void CTimesliceTestThread::Wait()
	{
	User::WaitForRequest(iExitStatus);
	}

TInt CTimesliceTestThread::ThreadFunc(TAny* aPtr)
	{
	CTimesliceTestThread& a = *(CTimesliceTestThread*)aPtr;
	Driver.SetCurrentThreadCpu(a.iCpu);
	Driver.SetCurrentThreadPriority(63);
	Driver.SetCurrentThreadTimeslice(a.iSlice);
	User::AfterHighRes(100000);
	TUint id = a.iId;
	TUint32 last_interval_begin = User::FastCounter();
	TUint32 last_seen_time = User::FastCounter();
	FOREVER
		{
		TUint32 nfc = User::FastCounter();
		TUint32 delta = nfc - last_seen_time;
		TUint32 interval_length = last_seen_time - last_interval_begin;
		if (delta > a.iThresh || interval_length > a.iThresh2)
			{
			last_interval_begin = nfc;
			TUint32 x = (id<<30) | (interval_length&0x3fffffffu);
			TInt r = a.iBuf->TryPut(x);
			if (r != KErrNone)
				break;
			}
		last_seen_time = nfc;
		}
	return KErrNone;
	}

CircBuf* RunTimesliceTest(TInt aCpu, TInt aSlice, TInt aCount, TInt aInterfere = 0)
 	{
	TUint32 oldaff = 0;
	TUint32 interfereAffinity = 0; 
	TUint tellKernel = 0x80000000u;
	
	CircBuf* buf = CircBuf::New(aCount);
	test(buf != 0);
	CTimesliceTestThread* t0 = CTimesliceTestThread::New(0, aCpu, aSlice, buf);
	test(t0 != 0);
	CTimesliceTestThread* t1 = CTimesliceTestThread::New(1, aCpu, aSlice, buf);
	test(t1 != 0);

	if (aInterfere) 
		{
		if (aInterfere < 0) 
			{
			tellKernel = 0;
			}
		TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0);
		test(r>0);
		interfereAffinity = (0x80000000 | ((0x1<<r)-1)) & ~0x2; // all except core 1
		if (0x80000001 == interfereAffinity) 
			{
			interfereAffinity = 0;   // dual core system (not doing this fails affinity check later)
			}
		
		Driver.SetCurrentThreadCpu(interfereAffinity , &oldaff);   // move away from core 1 (doesn't hurt though not much difference gained)
		Driver.SetCurrentThreadPriority(63);                       // changing prescaler requires running on core 1 so priority needs to 
		}                                                          // match test threads


	t0->Start();
	t1->Start();
	if (aInterfere) 
		{
		TInt prescale = 1;
		while (!t0->Finished() || !t1->Finished()) 
			{
			User::AfterHighRes(23000);
			Driver.SetLocalTimerPrescaler((1u<<1)|tellKernel, prescale);
			prescale++;
			if (prescale >  4) 
				{
				prescale = 0;
				}
			}
		}

	t0->Wait();
	t1->Wait();
	
	delete t0;
	delete t1;
	if (aInterfere) 
		{
		TUint32 aff;
		Driver.SetLocalTimerPrescaler((1u<<1)|0x80000000u, -1);
		RThread().SetPriority(EPriorityNormal);
		Driver.SetCurrentThreadCpu(oldaff,&aff);
		test_Equal(aff,interfereAffinity);
		}
	return buf;
	}

TUint32 ticks_to_us(TUint32 aTicks, TUint32 aF)
	{
	TUint64 x = TUint64(aTicks) * TUint64(1000000);
	TUint64 f64 = aF;
	x += (f64>>1);
	x /= f64;
	return I64LOW(x);
	}

void DisplayBuffer(CircBuf* aBuf, TUint32 aSlice )
	{
	TUint32 f;
	TInt r = HAL::Get(HAL::EFastCounterFrequency, (TInt&)f);
	test_KErrNone(r);
	TUint32* p = (TUint32*)aBuf->iBufBase;
	TInt c = aBuf->iSlotCount;
	TInt i;
	TInt lid = -1;
	TUint32 min = ~0u;
	TUint32 max = 0;
	TUint32 totivus = 0;
	TBool firstchg = ETrue;
	for (i=0; i<c; ++i)
		{
		TUint32 x = p[i];
		TUint32 id = x>>30;
		TUint32 iv = (x<<2)>>2;
		TUint32 ivus = ticks_to_us(iv,f);
		if (lid >= 0)
			{
			if (lid == (TInt)id)
				totivus += ivus;
			else
				{
				if (!firstchg)
					{
					if (totivus < min)
						min = totivus;
					if (totivus > max)
						max = totivus;
					}
				else
					firstchg = EFalse;
				totivus = ivus;
				}
			}
		lid = (TInt)id;
		test.Printf(_L("ID: %1d IV: %10d (=%10dus) TIV %10dus\n"), id, iv, ivus, totivus);
		}

	if (aSlice > 0)
		{
		// check timeslices where within acceptable ranges
		TUint32 sliceError = KSliceDeltaPercent*aSlice/100;
		test_Compare(max,<,aSlice+sliceError);  
		test_Compare(min,>,aSlice-sliceError);  
		}
	test.Printf(_L("RANGE %d-%dus (%dus)\n"), min, max, max-min);
	}

void TT()
	{
	test.Printf(_L("Timeslicing test ...\n"));
	CircBuf* b = RunTimesliceTest(1, 50000, 100);
	test.Next(_L("Baseline - expecting normal"));
	DisplayBuffer(b,50000u);
	delete b;

	Driver.SetLocalTimerPrescaler(1u<<1, 1);
	b = RunTimesliceTest(1, 50000, 100);
	test.Next(_L("expecting double"));
	DisplayBuffer(b,100000u);
	delete b;

	Driver.SetLocalTimerPrescaler(1u<<1|0x80000000u, 1);
	test.Next(_L("expecting normal again"));
	b = RunTimesliceTest(1, 50000, 100);
	DisplayBuffer(b,50000u);
	delete b;

	test.Next(_L("expecting half"));
	Driver.SetLocalTimerPrescaler(1u<<1, -1);
	b = RunTimesliceTest(1, 50000, 100);
	DisplayBuffer(b,25000u);
	delete b;

	Driver.SetLocalTimerPrescaler(1u<<1|0x80000000u, -1);
	test.Next(_L("expecting normal again"));
	b = RunTimesliceTest(1, 50000, 100);
	DisplayBuffer(b,50000u);
	delete b;

	b = RunTimesliceTest(1, 50000, 200 ,-1);
	test.Next(_L("expecting random"));
	DisplayBuffer(b,0u);  // timeslices should be fairly random on this run

	b = RunTimesliceTest(1, 50000, 200 ,1);
	test.Next(_L("expecting normal again"));
	DisplayBuffer(b,50000u);
	delete b;
	}

struct SGTRecord
	{
	TUint64 iTSInterval;
	TUint64 iGTInterval;
	};


SGTRecord* RunGTTest(TInt aCount, TInt aWait)
	{
	TUint64 lastgt,lastts,gt,ts;

	SGTRecord* res = new SGTRecord[aCount];
	test(res!=0);


	TInt r = Driver.ReadGlobalTimerAndTimestamp(lastgt,lastts);
	test_Equal(r,KErrNone);

	for (TInt i = 0; i < aCount; i++) 
		{
		User::AfterHighRes(aWait);
		
		TInt r = Driver.ReadGlobalTimerAndTimestamp(gt,ts);
		test_Equal(r,KErrNone);
		res[i].iGTInterval = gt-lastgt;
		lastgt = gt;
		res[i].iTSInterval = ts-lastts;
		lastts = ts;
		}

	return res;
	}

void DisplayGTResults(SGTRecord* aRec, TInt aCount, TUint32 aFreq, TUint64 aExpectedTSInterval, TUint64 aExpectedGTInterval)
	{
	SGTRecord max = { 0ul , 0ul };
	SGTRecord min = { KMaxTUint64 , KMaxTUint64 };
	
	TUint64 errgt = (aExpectedGTInterval*KTimeStampDeltaPercent)/100;
	TUint64 errts = (aExpectedTSInterval*KTimeStampDeltaPercent)/100;

	
	for (TInt i = 0 ; i < aCount; i++) 
		{
		test.Printf(_L("gt interval : %Lu (gtticks) %Lu (us)\n"),
					aRec[i].iGTInterval,
					aRec[i].iTSInterval*1000000u/TUint64(aFreq));
		
		if (max.iTSInterval < aRec[i].iTSInterval) 
			{
			max.iTSInterval = aRec[i].iTSInterval;
			}
		if (max.iGTInterval < aRec[i].iGTInterval) 
			{
			max.iGTInterval = aRec[i].iGTInterval;
			}
		
		if (min.iTSInterval > aRec[i].iTSInterval) 
			{
			min.iTSInterval = aRec[i].iTSInterval;
			}
		if (min.iGTInterval > aRec[i].iGTInterval) 
			{
			min.iGTInterval = aRec[i].iGTInterval;
			}
		}
	
	test.Printf(_L("RANGE Global Timer %Lu-%Lu ticks (%Lu ticks)\n"),
				min.iGTInterval, max.iGTInterval, max.iGTInterval-min.iGTInterval);
	
	test.Printf(_L("RANGE Timestamp %Lu-%Lu us (%Lu us)\n"),
				(1000000u*min.iGTInterval)/TUint64(aFreq), (1000000u*max.iGTInterval)/TUint64(aFreq),
				(1000000u*max.iGTInterval)/TUint64(aFreq) - (1000000u*min.iGTInterval)/TUint64(aFreq));
	
	if (errts) 
		{
		test_Compare(max.iTSInterval,<,aExpectedTSInterval+errts);  
		test_Compare(min.iTSInterval,>,aExpectedTSInterval);  
		}
	
	if (errgt) 
		{
		test_Compare(max.iGTInterval,<,aExpectedGTInterval+errgt);  
		test_Compare(min.iGTInterval,>,aExpectedGTInterval);  
		}
	
	}

void GTT()
	{
	test.Printf(_L("Global timer tests ...\n"));
	TUint64 gt,ts;

	TInt r = Driver.ReadGlobalTimerAndTimestamp(gt,ts);
	if (KErrNotSupported == r ) 
		{
		test.Printf(_L("Global timer not supported in this plaform, skipping GT tests\n"));
		return;
		}

	TUint32 f;
	r = HAL::Get(HAL::EFastCounterFrequency, (TInt&)f);
	test_KErrNone(r);
	TInt wait = 100000; // 100ms
	TInt count = 10;
	
	TUint64 expectedTs = (TUint64(f)*TUint64(wait))/1000000u;
	TUint64 expectedGtOrig = expectedTs;
	
	SGTRecord* rec;
	for (TInt i = 0; i < 10; i++)
		{
		TUint64 expectedGt = expectedGtOrig/(i+1);
		r = Driver.SetGlobalTimerPrescaler(i);
		test_KErrNone(r);
		rec = RunGTTest(count, wait);
		test.Printf(_L("expectedTS %Lu expectedGT %Lu\n"),expectedTs,expectedGt);
		DisplayGTResults(rec,count, f, expectedTs , expectedGt);
		delete rec;
		}

	r = Driver.SetGlobalTimerPrescaler(-1); // back to default
	test_KErrNone(r);
	}

void RunTests()
	{
	TestRatios();
	if (Driver.FrqChgTestPresent()!=KErrNone)
		{
		test.Printf(_L("Frequency Change not supported on this platform\n"));
		return;
		}
	TT();
	GTT();
	}

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Testing"));
	TInt r = User::LoadLogicalDevice(KLddName);
	if (r==KErrNotFound)
		{
		test.Printf(_L("Test not supported on this platform\n"));
		}
	else 
		{
		if (r!=KErrNone)
			{
			test_Equal(KErrAlreadyExists, r);
			}
		r = Driver.Open();
		test_KErrNone(r);
		RunTests();
		Driver.Close();
		}

	test.End();
	r = User::FreeLogicalDevice(KLddName);
	test_KErrNone(r);
	return KErrNone;
	}
