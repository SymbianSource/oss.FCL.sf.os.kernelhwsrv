// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\d_mstim.cpp
// LDD for testing millisecond timer
// 
//

#include "plat_priv.h"
#if defined(__MEIG__)
#include <cl7211.h>
#elif defined(__MAWD__)
#include <windermere.h>
#elif defined(__MISA__)
#include <sa1100.h>
#elif defined(__MCOT__)
#include <cotulla.h>
#elif defined(__IS_OMAP1510__) || defined(__IS_OMAP1610__) 
#include <omap.h>
#include <omap_timer.h>
#elif defined(__MI920__) || defined(__NI1136__)
#ifdef __MI920__
#define USE_CM920_FRC
#endif
#ifdef USE_CM920_FRC
#include <iolines.h>
#else
#include <integratorap.h>
#endif
#elif defined(__RVEMUBOARD__)
#include <rvemuboard.h>
#elif defined(__NE1_TB__)
#include <upd35001_timer.h>
#elif defined(__MRAP__)
#include <rap.h>
#endif
#include "d_mstim.h"
#include "../misc/prbs.h"

#if defined(__WINS__)
typedef Int64 TCounter;
typedef Int64 TDelta;
const TDelta KMaxDelta = 0x7fffffffffffffff;
const TDelta KMinDelta = 0x8000000000000000;
#else
typedef TUint TCounter;
typedef TInt TDelta;
const TDelta KMaxDelta = KMaxTInt;
const TDelta KMinDelta = KMinTInt;
#endif

#ifdef __MISA__
inline TCounter TIMER()
	{ return *(volatile TUint*)KHwRwOstOscr; }
#endif
#if defined(__IS_OMAP1510__) || defined(__IS_OMAP1610__) 
inline TCounter TIMER()
	{ return TOmapTimer::Timer3Value(); }
#endif
#ifdef __MCOT__
inline TCounter TIMER()
	{ return *(volatile TUint*)KHwRwOstOscr; }
#endif
#ifdef __MAWD__
inline TCounter TIMER()
	{ return *(volatile TUint*)(KWindBaseAddress+KWindTimer1Value16)&0xffff; }
#endif
#ifdef __MEIG__
inline TCounter TIMER()
{ return *(volatile TUint*)(KEigerBaseAddress+KEigerTimer1Data16)&0xffff;}
#endif
#if defined(__MI920__) || defined(__NI1136__)
inline TCounter TIMER()
#ifdef USE_CM920_FRC
	{ return *(volatile TUint*)(KHwRwCoreClkCounter);}		// 32-bit Core module counter inc's at 24MHz
#else
	{ return *(volatile TUint*)(KHwCounterTimer1+KHoTimerValue)&0xffff;}
#endif
#endif
#if defined(__RVEMUBOARD__)
inline TCounter TIMER()
	{ return *(volatile TUint*)(KHwCounterTimer1+KHoTimerValue)&0xffff;}
#endif
#ifdef __NE1_TB__
inline TCounter TIMER()
	{ return NETimer::Timer(5).iTimerCount; }
#endif
#ifdef __MRAP__
inline TCounter TIMER()
	{ TRap::SetRegister32(1, KRapRegRTC001_TRIGGER);
	return  TRap::Register32(KRapRegRTC001_LONGCOUNT); }
#endif
#if defined(__EPOC32__) && defined(__CPU_X86)
TCounter TIMER();
void SetUpTimerChannel2();
#endif

#ifdef __WINS__
inline TCounter TIMER()
	{
	LARGE_INTEGER c;
	QueryPerformanceCounter(&c);
	return c.QuadPart;
	}
#endif

#if defined(__MISA__) || (defined(USE_CM920_FRC) && (defined(__MI920__) || defined(__NI1136__)))
inline TDelta TimeDelta(TCounter initial, TCounter final)
	{ return final-initial; }				// SA1100 timer counts up
#endif
#if defined(__MCOT__)
inline TDelta TimeDelta(TCounter initial, TCounter final)
	{ return final-initial; }				// Cotulla timer counts up
#endif
#if defined(__MAWD__) || defined(__MEIG__) || (!defined(USE_CM920_FRC) && (defined(__MI920__) || defined(__NI1136__))) 
inline TDelta TimeDelta(TCounter initial, TCounter final)
	{ return (initial-final)&0xffff; }		// Eiger/Windermere/Integrator timer counts down
#endif
#if defined(__IS_OMAP1510__) || defined(__IS_OMAP1610__)
inline TDelta TimeDelta(TCounter initial, TCounter final)
	{ return (initial-final);}		// OMAP timer counts down
#endif
#if defined(__EPOC32__) && defined(__CPU_X86)
TDelta TimeDelta(TUint initial, TUint final)
	{
	TUint tickdiff=(initial-final)&0xffff;
	TUint msdiff=((final>>16)-(initial>>16))&0xffff;
	msdiff=1193*msdiff-tickdiff;
	msdiff=(msdiff+32768)&~0xffff;
	return msdiff+tickdiff;
	}
#endif
#ifdef __NE1_TB__
inline TDelta TimeDelta(TCounter initial, TCounter final)
	{ return final - initial; }
#endif
#if defined(__MRAP__)
inline TDelta TimeDelta(TCounter initial, TCounter final)
	{ return final-initial; }				// RAP RTC timer counts up
#endif
#ifdef __WINS__
inline TDelta TimeDelta(TCounter initial, TCounter final)
	{ return final-initial; }		// counts up
#endif
#if defined(__RVEMUBOARD__)
inline TDelta TimeDelta(TCounter initial, TCounter final)
	{ return (initial-final)&0xffff; }		// Timer counts down
#endif

const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

const TInt KMaxMsTim=9;
const TInt KMaxMsTimR=9;

TInt TicksToMicroseconds(TDelta aTicks)
	{
#if defined(__MISA__) || defined(__MCOT__)
	Int64 ticks(aTicks);
	ticks*=(1000000);
	ticks+=KHwOscFreqHz/2;		// 3.6864MHz tick
	ticks/=KHwOscFreqHz;
	return (TInt)ticks;
#endif
#if defined(__IS_OMAP1510__) || defined(__IS_OMAP1610__) 
	// Timer runs at 12Mhz/32 = 375kHz. Each tick is 2.66...us which is 16/6us
	aTicks<<=4;		// * 16
	aTicks+=3;	    // rounding to the closest number of us
	return (TInt)(aTicks/6);	// us = (ticks*16+3)/6
#endif
#if defined(__MI920__) || defined(__NI1136__)
#if defined(USE_CM920_FRC)
	Int64 ticks(aTicks);
	ticks*=(1000000);
	ticks+=24000000/2;
	ticks/=24000000;
	return (TInt)ticks;
#else
	aTicks<<=14;	// 1 tick = 32/3 us
	aTicks+=768;	// round
	return (TInt)(aTicks/1536);
#endif
#endif
#if defined(__RVEMUBOARD__)
	return (TInt)(aTicks*256);  // 1 tick = 256 us
#endif
#if defined(__MAWD__) || defined(__MEIG__)
	return aTicks*500;					// 2kHz tick
#endif
#if defined(__NE1_TB__)
	NETimer& T5 = NETimer::Timer(5);
	TUint prescale = __e32_find_ms1_32(T5.iPrescaler & 0x3f);
	TInt f = 66666667 >> prescale;
	TInt64 x = I64LIT(1000000);
	x *= TInt64(aTicks);
	x += TInt64(f>>1);
	x /= TInt64(f);
	return (TInt)x;
#endif
#if defined(__MRAP__)
    // RTC runs with 32.768 kHz -> one tick is 
    const TUint KRTCClockHz = 32768;
    Int64 ticks(aTicks);
    ticks*=(1000000);
	ticks+=KRTCClockHz/2;		// 32.768 kHz tick
	ticks/=KRTCClockHz;
	return (TInt)ticks;
#endif
#if defined(__EPOC32__) && defined(__CPU_X86)
	TInt x = aTicks;
	TInt y = x;
	y -= ((3*x)>>4);	// * 0.D
	y += (aTicks>>12);	// * 0.D00D
	TInt z = (6*x)>>8;	// * 0.06
	y += z;				// * 0.D60D
	y += (x>>9);		// * 0.D68D
	y += (z>>16);		// * 0.D68D6
	y += (z>>20);		// * 0.D68D66
	return y;
#endif
#ifdef __WINS__
	LARGE_INTEGER f;
	QueryPerformanceFrequency(&f);
	aTicks*=1000000;
	aTicks+=f.QuadPart-1;
	aTicks/=f.QuadPart;
	return (TInt)aTicks;
#endif
	}


void InitTimer()
	{
#ifdef __MAWD__
	// Set up timer 1 as free running 2kHz clock
	TWind::SetBuzzerControl(0);		// disable buzzer
	TWind::SetTimer1Control(KWindTimer1ControlTimerEnable);
	TWind::SetTimer1Load(0);
#endif
#ifdef __MEIG__
	// Set up timer 1 as free running 2kHz clock
	TEiger::ModifyControl21(KEigerControlTimer1PreOrFree|KEigerControlTimer1K512OrK2|
							KEigerControlBuzzerToggle|KEigerControlBuzzerTimer1OrToggle,0);
	TEiger::SetTimer1Data(0);
#endif
#if defined(__MISA__)
	// MISA free running counter is always active - no initialisation required
#endif
#if defined(__IS_OMAP1510__) || defined(__IS_OMAP1610__)
	// Set up Timer3 as a free-running timer at 12Mhz/32 = 375kHz
	TOmapTimer::SetTimer3Ctrl(	TOmapTimer::KHtOSTimer_Cntl_Ar
									| TOmapTimer::KHtOSTimer_Cntl_Free
									| TOmapTimer::KHtOSTimer_Cntl_ClkEnable );
	TOmapTimer::SetTimer3Prescale( TOmapTimer::EPrescaleBy32 );
	// Autoreload 0xFFFFFFFF to effectively wrap from zero back to 0xFFFFFFFF
	TOmapTimer::SetTimer3LoadTim( 0xFFFFFFFF );
	TOmapTimer::StartTimer3();
#endif
#if defined(__MI920__) || defined(__NI1136__)
#if !defined(USE_CM920_FRC)
    TIntegratorAP::SetTimerMode(TIntegratorAP::ECounterTimer1, TIntegratorAP::ETimerModeFreeRunning);
    TIntegratorAP::SetTimerPreScale(TIntegratorAP::ECounterTimer1, TIntegratorAP::ETimerPreScaleDiv256);	// 93.75kHz wrap 699ms
    TIntegratorAP::EnableTimer(TIntegratorAP::ECounterTimer1, TIntegratorAP::EEnable);
#endif
#endif
#if defined(__RVEMUBOARD__)
	// Switch timer 1 to a 1MHz clock in the system controller Ctrl register
	TRvEmuBoard::SetSCCtrl(KTimer1EnSel);

	// Set up timer 1 as free running 3.90625kHz clock
	TRvEmuBoard::SetTimerMode(KHwCounterTimer1, TRvEmuBoard::ETimerModeFreeRunning);
	TRvEmuBoard::SetTimerPreScale(KHwCounterTimer1, TRvEmuBoard::ETimerPreScaleDiv256);// 3.90625kHz wrap 16.777s
	TRvEmuBoard::EnableTimer(KHwCounterTimer1, TRvEmuBoard::EEnable);
#endif
#if defined(__NE1_TB__)
    // set up timer 5
    NETimer& T5 = NETimer::Timer(5);

	T5.iTimerCtrl = 0;						// stop and reset timer 5
	T5.iGTICtrl = 0;						// disable timer 5 capture modes
	__e32_io_completion_barrier();
	T5.iPrescaler = KNETimerPrescaleBy32;	// Timer 5 prescaled by 32 (=2.0833MHz)
	__e32_io_completion_barrier();
	T5.iGTInterruptEnable = 0;
	__e32_io_completion_barrier();
	T5.iGTInterrupt = KNETimerGTIInt_All;
	__e32_io_completion_barrier();
	T5.iTimerCtrl = KNETimerCtrl_CE;		// deassert reset for timer 5, count still stopped
	__e32_io_completion_barrier();
	T5.iTimerReset = 0xffffffffu;			// timer 5 wraps after 2^32 counts
	__e32_io_completion_barrier();
	T5.iTimerCtrl = KNETimerCtrl_CE | KNETimerCtrl_CAE;	// start timer 5
	__e32_io_completion_barrier();		
#endif
#if defined(__EPOC32__) && defined(__CPU_X86)
	// Set up timer channel 2 as free running counter at 14318180/12 Hz
	SetUpTimerChannel2();
#endif
	}

// global Dfc Que
TDynamicDfcQue* gDfcQ;

class NTimerQTest
	{
public:
	static inline NTimerQ& Timer()
		{ return *(NTimerQ*)NTimerQ::TimerAddress(); }
	static inline TUint32 MsCount()
		{ return Timer().iMsCount; }
	static inline void Setup(TAny* aPtr)
		{ NTimerQ& m=Timer(); m.iDebugFn=Test; m.iDebugPtr=aPtr; }
	static inline void Stop()
		{ NTimerQ& m=Timer(); m.iDebugFn=NULL; m.iDebugPtr=NULL; }
	static inline TBool XferC()
		{ return Timer().iTransferringCancelled; }
	static inline TBool CritC()
		{ return Timer().iCriticalCancelled; }
	static void Test(TAny* aPtr, TInt aPos);
	};

class DMsTim;

class TMsTim : public NTimer
	{
public:
	enum TMode
		{
		EIntAfter,
		EDfcAfter,
		EIntAgain,
		EDfcAgain,
		EIntCancel,
		EDfcCancel,
		EUserDfcAfter
		};

	enum TModeX
		{
		EIntAgainOnce=7,
		EUserDfcAgainOnce
		};
public:
	TMsTim();
	~TMsTim();
	TInt Create();
	TInt Start(TInt aMode, TInt aInterval, TInt aParam);
	static void MsCallBack(TAny* aPtr);
	static void IDfcFn(TAny* aPtr);
	static void DfcFn(TAny* aPtr);
	void CompleteClient(TInt aValue);
public:
	TMode iMode;
	TInt iInterval;
	TInt iParam;
	TCounter iStartTime;
	TDelta iMin;
	TDelta iMax;
	Int64 iTotal;
	TInt iCount;
	DMsTim* iLdd;
	TInt iId;
	TClientRequest* iRequest;
	TDfc iIDfc;
	TDfc iCompletionDfc;
	};

class TMsTimRand : public NTimer
	{
public:
	TMsTimRand();
#ifdef __SMP__
	~TMsTimRand();
#endif
	TInt Start(TInt aInterval, DMsTim* aLdd, TInt aPos);
	static void MsCallBack(TAny* aPtr);
	void FillWithGarbage(TUint aFillValue);
public:
	TInt iInterval;
	TCounter iStartTime;
	DMsTim* iLdd;
	};

class DMsTimFactory : public DLogicalDevice
//
// Millisecond timer LDD factory
//
	{
public:
	DMsTimFactory();
	~DMsTimFactory();
	virtual TInt Install();						//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel);	//overriding pure virtual
	};

class DMsTim : public DLogicalChannel
//
// Millisecond timer LDD channel
//
	{
public:
	DMsTim();
	~DMsTim();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	TInt DoRequest(TInt aFunction, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	virtual void HandleMsg(TMessageBase* aMsg);
public:
	void TimerExpired(TInt anId);
	inline DThread* Client() { return iThread; }
public:
	DThread* iThread;
	TMsTim iMsTim[KMaxMsTim];
	TMsTimRand iMsTimR[KMaxMsTimR];
	TInt iRandMin;
	TInt iRandMax;
	TInt iXferC;
	TInt iCritC;
	TInt iStartFail;
	TInt iCallBacks;
	TInt iCompletions;
	TUint iSeed[2];
	};

TMsTim::TMsTim()
	:	NTimer(MsCallBack,this),
		iMode(EIntAfter),
		iInterval(0),
		iParam(0),
		iStartTime(0),
		iMin(KMaxDelta),
		iMax(KMinDelta),
		iTotal(0),
		iCount(0),
		iRequest(NULL),
		iIDfc(IDfcFn,this),
		iCompletionDfc(DfcFn,this,gDfcQ,1)
	{
	}

TMsTim::~TMsTim()
	{
	Kern::DestroyClientRequest(iRequest);
#ifdef __SMP__
	NTimer* nt = STATIC_CAST(NTimer*,this);
	new (nt) NTimer(&MsCallBack, this);	// so NTimer destructor doesn't kill us
#endif
	}

TInt TMsTim::Create()
	{
	return Kern::CreateClientRequest(iRequest);
	}

void TMsTim::IDfcFn(TAny* aPtr)
	{
	TMsTim& m=*(TMsTim*)aPtr;
	TInt c = NKern::CurrentContext();
	__NK_ASSERT_ALWAYS(c == NKern::EIDFC);
	__NK_ASSERT_ALWAYS(NKern::KernelLocked(1));
	m.iCompletionDfc.DoEnque();
	}

void TMsTim::DfcFn(TAny* aPtr)
	{
	TMsTim& m=*(TMsTim*)aPtr;
	if (m.iMode==EUserDfcAfter)
		{
		TCounter timer_val=TIMER();
		TDelta time=TimeDelta(m.iStartTime, timer_val);
		++m.iCount;
		if (time<m.iMin)
			m.iMin=time;
		if (time>m.iMax)
			m.iMax=time;
		m.iTotal+=time;
		}
	m.iLdd->TimerExpired(m.iId);
	}

void TestThreadContext()
	{
	TInt c1 = NKern::CurrentContext();
	NKern::Lock();
	TInt c2 = NKern::CurrentContext();
	NKern::Unlock();
	__NK_ASSERT_ALWAYS((c1 == NKern::EThread) && (c2 == NKern::EThread));
	}

void TMsTim::MsCallBack(TAny* aPtr)
	{
	TInt c = NKern::CurrentContext();
	TCounter timer_val=TIMER();
	TMsTim& m=*(TMsTim*)aPtr;
	TDelta time=TimeDelta(m.iStartTime, timer_val);
	if (++m.iCount>0 || (m.iMode!=EIntAgain && m.iMode!=EDfcAgain))
		{
		if (time<m.iMin)
			m.iMin=time;
		if (time>m.iMax)
			m.iMax=time;
		m.iTotal+=time;
		}
	switch (m.iMode)
		{
		case EIntAfter:
			__NK_ASSERT_ALWAYS(c == NKern::EInterrupt);
			m.iIDfc.Add();
			break;
		case EDfcAfter:
			TestThreadContext();
			m.iCompletionDfc.Enque();
			break;
		case EIntAgain:
			__NK_ASSERT_ALWAYS(c == NKern::EInterrupt);
			m.iStartTime=TIMER();
			m.Again(m.iInterval);
			break;
		case EDfcAgain:
			TestThreadContext();
			m.iStartTime=TIMER();
			m.Again(m.iInterval);
			break;
		case EIntCancel:
			__NK_ASSERT_ALWAYS(c == NKern::EInterrupt);
			m.iLdd->iMsTim[m.iParam].Cancel();
			m.iIDfc.Add();
			break;
		case EDfcCancel:
			TestThreadContext();
			m.iLdd->iMsTim[m.iParam].Cancel();
			m.iCompletionDfc.Enque();
			break;
		case EUserDfcAfter:
			__NK_ASSERT_ALWAYS(EFalse);
			break;
		}
	}

TInt TMsTim::Start(TInt aMode, TInt aInterval, TInt aParam)
	{
	TInt r=KErrGeneral;
	TInt c=0;
	TCounter holder=TIMER();		// holds the start value of timer
	switch (aMode)
		{
		case EIntAgain:
			c=-1;
		case EIntAfter:
		case EIntCancel:
			r=OneShot(aInterval);
			break;
		case EDfcAgain:
			c=-1;
		case EDfcAfter:
		case EDfcCancel:
			r=OneShot(aInterval,ETrue);
			break;
		case EIntAgainOnce:
		case EUserDfcAgainOnce:
#ifdef __SMP__
			i8888.iHState2=FALSE;
#else
			iCompleteInDfc=FALSE;
#endif
			r=Again(aInterval);
			if (aMode==EUserDfcAgainOnce)
				aMode=EUserDfcAfter;
			else
				aMode=EIntAfter;
			break;
		case EUserDfcAfter:
			r=OneShot(aInterval, iCompletionDfc);
			break;
		}
	if (r!=KErrNone)
		return r;
	iStartTime=holder;
	iMode=TMode(aMode);
	iInterval=aInterval;
	iParam=aParam;
	iMin=KMaxDelta;
	iMax=KMinDelta;
	iTotal=0;
	iCount=c;
	return KErrNone;
	}

void TMsTim::CompleteClient(TInt aValue)
	{
	Kern::QueueRequestComplete(iLdd->Client(),iRequest,aValue);
	}

TMsTimRand::TMsTimRand()
	:	NTimer(&MsCallBack,this)
	{
	memset(this,0,sizeof(TMsTimRand));
#ifdef __SMP__
	NTimer* nt = STATIC_CAST(NTimer*,this);
	new (nt) NTimer(&MsCallBack,this);
#else
	iFunction=MsCallBack;	// avoid triggering assertion in NTimer::OneShot()
#endif
	}

#ifdef __SMP__
TMsTimRand::~TMsTimRand()
	{
	NTimer* nt = STATIC_CAST(NTimer*,this);
	new (nt) NTimer(&MsCallBack, this);	// so NTimer destructor doesn't kill us
	}
#endif

void TMsTimRand::FillWithGarbage(TUint aFill)
	{
#ifdef __SMP__
	TUint32 f = aFill;
	f |= (f<<8);
	f |= (f<<16);
	iNext = (SDblQueLink*)f;
	iPrev = (SDblQueLink*)f;
	iDfcQ = (TDfcQue*)f;
	iPtr = (TAny*)f;
	iFn = (NEventFn)f;
	iTiedLink.iNext = (SDblQueLink*)f;
	iTiedLink.iPrev = (SDblQueLink*)f;
#else
	memset(this, (TUint8)aFill, 16);
#endif
	}

TInt TMsTimRand::Start(TInt aInterval, DMsTim* aLdd, TInt aPos)
	{
	iLdd=aLdd;
#ifdef __SMP__
	TUint fill=(aPos<<5)|(i8888.iHState1<<2)|3;
#else
	TUint fill=(aPos<<5)|(iState<<2)|3;
	iPad1 = (TUint8)fill;
#endif
	TInt r=OneShot(aInterval,ETrue);
	if (r==KErrNone)
		{
		iPtr=this;
		iInterval=aInterval;
		iStartTime=TIMER();
#ifdef __SMP__
		iFn=MsCallBack;
		i8888.iHState0 = (TUint8)fill;
		if (i8888.iHState1!=EHolding)
			*(TUint*)0xfcd1fcd1=i8888.iHState1;
#else
		iFunction=MsCallBack;
		iUserFlags = (TUint8)fill;
		if (iState!=EHolding)
			*(TUint*)0xfcd1fcd1=iState;
#endif
		}
	return r;
	}

void TMsTimRand::MsCallBack(TAny* aPtr)
	{
	TMsTimRand& m=*(TMsTimRand*)aPtr;
	TCounter time=TIMER();
	TDelta elapsed=TimeDelta(m.iStartTime,time);
	TInt error=TicksToMicroseconds(elapsed)-m.iInterval*1000;
	if (error<m.iLdd->iRandMin)
		m.iLdd->iRandMin=error;
	if (error>m.iLdd->iRandMax)
		m.iLdd->iRandMax=error;
	++m.iLdd->iCompletions;
	m.FillWithGarbage(0xd9);
	}

void NTimerQTest::Test(TAny* aPtr, TInt aPos)
	{
	DMsTim& ldd=*(DMsTim*)aPtr;
	++ldd.iCallBacks;
	if (aPos==7)
		return;
	TUint action=Random(ldd.iSeed)&31;
	TMsTimRand& m=ldd.iMsTimR[action&7];
	if (action<8)
		{
#ifdef __SMP__
		TUint fill=(aPos<<5)|(m.i8888.iHState1<<2)|3;
#else
		TUint fill=(aPos<<5)|(m.iState<<2)|3;
#endif
		m.Cancel();
		m.FillWithGarbage(fill);
		}
	else if (action<16)
		{
		TUint iv=(Random(ldd.iSeed)&31)+32;
		TInt r=m.Start(iv,&ldd,aPos);
		if (r!=KErrNone)
			++ldd.iStartFail;
		}
	if (XferC())
		++ldd.iXferC;
	if (CritC())
		++ldd.iCritC;
	}

DECLARE_STANDARD_LDD()
	{
    return new DMsTimFactory;
    }

DMsTimFactory::DMsTimFactory()
//
// Constructor
//
    {
    iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    //iParseMask=0;//No units, no info, no PDD
    //iUnitsMask=0;//Only one thing
    }

TInt DMsTimFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DMsTim on this logical device
//
    {
	aChannel=new DMsTim;
	return aChannel?KErrNone:KErrNoMemory;
    }

const TInt KDMsTimThreadPriority = 27;
_LIT(KDMsTimThread,"DMsTimThread");

TInt DMsTimFactory::Install()
//
// Install the LDD - overriding pure virtual
//
    {
	// Allocate a kernel thread to run the DFC 
	TInt r = Kern::DynamicDfcQCreate(gDfcQ, KDMsTimThreadPriority, KDMsTimThread);

#ifdef CPU_AFFINITY_ANY
			NKern::ThreadSetCpuAffinity((NThread*)(gDfcQ->iThread), KCpuAffinityAny);			
#endif

	if (r != KErrNone)
		return r; 	

    return SetName(&KMsTimerLddName);
    }

void DMsTimFactory::GetCaps(TDes8& aDes) const
//
// Get capabilities - overriding pure virtual
//
    {
    TCapsMsTimV01 b;
    b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
    }

/**
  Destructor
*/
DMsTimFactory::~DMsTimFactory()
	{
	if (gDfcQ)
		gDfcQ->Destroy();
	}

DMsTim::DMsTim()
//
// Constructor
//
    {
	iThread=&Kern::CurrentThread();
	iThread->Open();
	TInt i;
	for (i=0; i<KMaxMsTim; i++)
		{
		iMsTim[i].iLdd=this;
		iMsTim[i].iId=i;
		}
	for (i=0; i<KMaxMsTimR; i++)
		{
		iMsTimR[i].iLdd=this;
		}
	iSeed[0]=NTimerQTest::MsCount();
	iSeed[1]=0;
    }

TInt DMsTim::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
// Create channel
//
    {
    if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
    	return KErrNotSupported;
	InitTimer();
	SetDfcQ(gDfcQ);
	for (TInt i = 0 ; i < KMaxMsTim ; ++i)
		{
		TInt r = iMsTim[i].Create();
		if (r != KErrNone)
			return r;
		}
	iMsgQ.Receive();
#ifdef __SMP__
	NKern::ThreadSetCpuAffinity(NKern::CurrentThread(), NKern::NumberOfCpus() - 1); // Try and avoid the cpu the test app is running on
#endif
	return KErrNone;
	}

DMsTim::~DMsTim()
//
// Destructor
//
    {
#if defined(__MI920__) || defined(__NI1136__)
#if !defined(USE_CM920_FRC)
	TIntegratorAP::EnableTimer(TIntegratorAP::ECounterTimer1, TIntegratorAP::EDisable);
#endif
#endif
#if defined(__IS_OMAP1510__) || defined(__IS_OMAP1610__) 
	TOmapTimer::SetTimer3Ctrl( 0 );	// disable the timer
#endif
	Kern::SafeClose((DObject*&)iThread, NULL);
    }

void DMsTim::HandleMsg(TMessageBase* aMsg)
	{
	TInt r=KErrNone;
	TThreadMessage& m=*(TThreadMessage*)aMsg;
	TInt id=m.iValue;
	if (id==(TInt)ECloseMsg)
		{
		NTimerQTest::Stop();
		TInt i;
		for (i=0; i<KMaxMsTim; i++)
			{
			iMsTim[i].Cancel();
			iMsTim[i].iCompletionDfc.Cancel();
			iMsTim[i].CompleteClient(KErrCancel);
			}
		for (i=0; i<KMaxMsTimR; i++)
			{
			iMsTimR[i].Cancel();
			iMsTimR[i].FillWithGarbage(0x01);
			}
		m.Complete(KErrNone,EFalse);
		iMsgQ.CompleteAll(KErrServerTerminated);
		return;
		}
	else if (id<0)
		{
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		r=DoRequest(~id,pS,m.Ptr1(),m.Ptr2());
		}
	else
		{
		r=DoControl(id,m.Ptr0(),m.Ptr1());
		}
	m.Complete(r,ETrue);
	}

TInt DMsTim::DoControl(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r=KErrNone;
	TInt id=(TInt)a1;
	TMsTim& m=iMsTim[id];
	TInt interval=(TInt)a2;
	switch (aFunction)
		{
		case RMsTim::EControlStartPeriodicInt:
			r=m.Start(TMsTim::EIntAgain,interval,0);
			break;
		case RMsTim::EControlStartPeriodicDfc:
			r=m.Start(TMsTim::EDfcAgain,interval,0);
			break;
		case RMsTim::EControlStopPeriodic:
			m.Cancel();
			m.iCompletionDfc.Cancel();
			break;
		case RMsTim::EControlGetInfo:
			{
			SMsTimerInfo info;
			info.iCount=m.iCount;
			Int64 avg=m.iTotal/m.iCount;
			info.iAvg=TicksToMicroseconds((TInt)avg);
#ifdef __SMP__
			info.iMin=info.iAvg;
			info.iMax=info.iAvg;
#else
			info.iMin=TicksToMicroseconds(m.iMin);
			info.iMax=TicksToMicroseconds(m.iMax);
#endif

			r=Kern::ThreadRawWrite(iThread,a2,&info,sizeof(info));
			break;
			}
		case RMsTim::EControlBeginRandomTest:
			{
			iRandMin=KMaxTInt;
			iRandMax=KMinTInt;
			iXferC=0;
			iCritC=0;
			iStartFail=0;
			iCallBacks=0;
			iCompletions=0;
			NTimerQTest::Setup(this);
			break;
			}
		case RMsTim::EControlEndRandomTest:
			{
			NTimerQTest::Stop();
			TInt i;
			for (i=0; i<KMaxMsTimR; i++)
				{
				iMsTimR[i].Cancel();
				iMsTimR[i].FillWithGarbage(0x35);
				}
			break;
			}
		case RMsTim::EControlGetRandomTestInfo:
			{
			SRandomTestInfo info;
			info.iMin=iRandMin;
			info.iMax=iRandMax;
			info.iXferC=iXferC;
			info.iCritC=iCritC;
			info.iStartFail=iStartFail;
			info.iCallBacks=iCallBacks;
			info.iCompletions=iCompletions;
			r=Kern::ThreadRawWrite(iThread,a1,&info,sizeof(info));
			break;
			}
		case RMsTim::EControlGetIdleTime:
			{
			TInt irq=NKern::DisableAllInterrupts();
			r=NTimerQ::IdleTime();
			NKern::RestoreInterrupts(irq);
			break;
			}
		default:
			r=KErrNotSupported;
			break;
		}
	return r;
	}

TInt DMsTim::DoRequest(TInt aFunction, TRequestStatus* aStatus, TAny* a1, TAny* a2)
	{
	TInt id=(TInt)a1;
	TMsTim& m=iMsTim[aFunction == RMsTim::ERequestIntCancel ? 7 : id];
	TInt interval=(TInt)a2;
	TInt r=KErrNone;
	switch (aFunction)
		{
		case RMsTim::ERequestOneShotInt:
			r=m.Start(TMsTim::EIntAfter,interval,0);
			break;
		case RMsTim::ERequestOneShotDfc:
			r=m.Start(TMsTim::EDfcAfter,interval,0);
			break;
		case RMsTim::ERequestIntCancel:
			r=m.Start(TMsTim::EIntCancel,interval,id);
			break;
		case RMsTim::ERequestOneShotIntAgain:
			r=m.Start(TMsTim::EIntAgainOnce,interval,0);
			break;
		case RMsTim::ERequestOneShotUserDfc:
			r=m.Start(TMsTim::EUserDfcAfter,interval,0);
			break;
		case RMsTim::ERequestOneShotUserDfcAgain:
			r=m.Start(TMsTim::EUserDfcAgainOnce,interval,0);
			break;
		default:
			r=KErrNotSupported;
			break;
		}
	m.iRequest->SetStatus(aStatus);
	if (r!=KErrNone)
		Kern::QueueRequestComplete(iThread,m.iRequest,r);
	return r;
	}

void DMsTim::TimerExpired(TInt anId)
	{
	TMsTim& m=iMsTim[anId];
	switch (m.iMode)
		{
		case TMsTim::EIntAfter:
		case TMsTim::EDfcAfter:
		case TMsTim::EUserDfcAfter:
			m.CompleteClient(KErrNone);
			break;
		case TMsTim::EIntAgain:
		case TMsTim::EDfcAgain:
			break;
		case TMsTim::EIntCancel:
		case TMsTim::EDfcCancel:
			{
			TMsTim& cancelled=iMsTim[m.iParam];
			cancelled.CompleteClient(KErrAbort);
			m.CompleteClient(KErrNone);
			break;
			}
		}
	}

