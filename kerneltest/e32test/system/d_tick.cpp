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
// e32test\system\d_tick.cpp
// LDD for testing tick-based timers
// 
//

#include "platform.h"
#include <assp.h>
#if defined(__MEIG__)
#include <cl7211.h>
#elif defined(__MAWD__)
#include <windermere.h>
#elif defined(__MISA__)
#include <sa1100.h>
#elif defined(__MCOT__)
#include <cotulla.h>
#elif defined(__MI920__) || defined(__NI1136__)
#include <integratorap.h>
#elif defined(__IS_OMAP1510__) || defined(__IS_OMAP1610__) 
#include <omap.h>
#include <omap_timer.h>
#elif defined(__WINS__)
#include "nk_priv.h"
#elif defined(__RVEMUBOARD__)
#include <rvemuboard.h>
#elif defined(__NE1_TB__)
#include <upd35001_timer.h>
#elif defined(__MRAP__)
#include <rap.h>
#endif
#include <kernel/kern_priv.h>
#include "d_tick.h"
#include "../misc/prbs.h"

#if defined(__WINS__)
typedef Int64 TCounter;
typedef Int64 TDelta;
#else
typedef TUint TCounter;
typedef TInt TDelta;
#endif

#if defined(__MISA__)|| defined(__MCOT__)
inline TCounter TIMER()
	{ return *(volatile TUint*)KHwRwOstOscr; }
#endif
#if defined(__IS_OMAP1510__) || defined(__IS_OMAP1610__) 
inline TCounter TIMER()
	{ return TOmapTimer::Timer3Value(); }
#endif
#ifdef __MAWD__
inline TCounter TIMER()
	{ return *(volatile TUint*)(KWindBaseAddress+KWindTimer1Value16)&0xffff; }
#endif
#ifdef __MEIG__
inline TCounter TIMER()
	{ return *(volatile TUint*)(KEigerBaseAddress+KEigerTimer1Data16)&0xffff; }
#endif
#if defined(__MI920__) || defined(__NI1136__)
inline TCounter TIMER()
	{ return *(volatile TUint*)(KHwCounterTimer1+KHoTimerValue)&0xffff;}
#endif
#if defined(__RVEMUBOARD__)
inline TCounter TIMER()
	{ return *(volatile TUint*)(KHwCounterTimer1+KHoTimerValue)&0xffff;}
#endif
#if defined(__NE1_TB__)
inline TCounter TIMER()
	{ return NETimer::Timer(2).iTimerCount; }
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
#if defined(__MRAP__)
inline TDelta TimeDelta(TCounter initial, TCounter final)
	{ return final-initial; }				// RAP RTC timer counts up
inline TInt LongTimeDelta(TCounter initial, TCounter final, TUint, TUint)
	{ return final-initial; }				// RAP RTC timer counts up
#endif

#if defined(__MISA__) || defined(__MCOT__)
inline TDelta TimeDelta(TCounter initial, TCounter final)
	{ return final-initial; }				// SA1100 timer counts up
inline TInt LongTimeDelta(TCounter initial, TCounter final, TUint, TUint)
	{ return final-initial; }				// SA1100 timer counts up
#endif
#if defined(__IS_OMAP1510__) || defined(__IS_OMAP1610__)
inline TDelta TimeDelta(TCounter initial, TCounter final)
	{ return initial-final; }				// OMAP timer counts down
inline TInt LongTimeDelta(TCounter initial, TCounter final, TUint, TUint)
	{ return initial-final; }
#endif
#if defined(__MI920__) || defined(__NI1136__)
inline TDelta TimeDelta(TCounter initial, TCounter final)
	{ return (initial-final)&0xffff; }		// Integrator timer counts down
TInt LongTimeDelta(TCounter initial, TCounter final, TUint init_ms, TUint final_ms)
	{
	TUint r=(initial-final)&0xffff;			// Integrator timer counts down
	TUint ms=final_ms-init_ms;
	ms=2*ms-r;
	ms=(ms+32768)&~0xffff;
	return r+ms;
	}
#endif
#if defined(__RVEMUBOARD__)
inline TDelta TimeDelta(TCounter initial, TCounter final)
	{ return (initial-final)&0xffff; }		// Timer counts down
TInt LongTimeDelta(TCounter initial, TCounter final, TUint init_ms, TUint final_ms)
	{
	TUint r=(initial-final)&0xffff;			// Timer counts down
	TUint ms=final_ms-init_ms;
	ms=2*ms-r;
	ms=(ms+32768)&~0xffff;
	return r+ms;
	}
#endif
#if defined(__NE1_TB__)
inline TDelta TimeDelta(TCounter initial, TCounter final)
	{ return final - initial; }
inline TDelta LongTimeDelta(TCounter initial, TCounter final, TUint, TUint)
	{ return final - initial; }
#endif
#if defined(__MAWD__) || defined(__MEIG__)
inline TDelta TimeDelta(TCounter initial, TCounter final)
	{ return (initial-final)&0xffff; }		// Eiger/Windermere timer counts down
TInt LongTimeDelta(TCounter initial, TCounter final, TUint init_ms, TUint final_ms)
	{
	TUint r=(initial-final)&0xffff;			// Eiger/Windermere timer counts down
	TUint ms=final_ms-init_ms;
	ms=2*ms-r;
	ms=(ms+32768)&~0xffff;
	return r+ms;
	}
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

TInt LongTimeDelta(TUint initial, TUint final, TUint init_ms, TUint final_ms)
	{
	TUint r=(initial-final)&0xffff;			// PC timer counts down
	TUint ms=final_ms-init_ms;
	ms=1193*ms-r;
	ms=(ms+32768)&~0xffff;
	return r+ms;
	}
#endif
#ifdef __WINS__
inline TDelta TimeDelta(TCounter initial, TCounter final)
	{ return final-initial; }		// counts up
inline TDelta LongTimeDelta(TCounter initial, TCounter final, TUint, TUint)
	{ return final-initial; }		// counts up
#endif

const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

const TInt KDaysFrom0ADTo2000AD=730497;		// See US_TIME.CPP to verify this
const TInt KSecondsPerDay=86400;

TUint TicksToMicroseconds(TDelta aTicks)
	{
#if defined(__MISA__) || defined(__MCOT__)
	Int64 ticks(aTicks);
	ticks*=(1000000);
	ticks+=KHwOscFreqHz/2;		// 3.6864MHz tick
	ticks/=KHwOscFreqHz;
	return (TUint)ticks;
#endif
#if defined(__IS_OMAP1510__) || defined(__IS_OMAP1610__) 
	// Timer runs at 12Mhz/32 = 375kHz. Each tick is 2.66...us which is 16/6us
	aTicks<<=4;		// * 16
	aTicks+=3;	    // rounding to the closest number of us
	return (TInt)(aTicks/6);	// us = (ticks*16+3)/6
#endif
#if defined(__MI920__) || defined(__NI1136__)
	aTicks<<=14;	// 1 tick = 32/3 us
	aTicks+=768;	// round
	return (TInt)(aTicks/1536);
#endif
#if defined(__RVEMUBOARD__)
	return (TInt)(aTicks*256);  // 1 tick = 256 us
#endif
#if defined(__NE1_TB__)
	NETimer& T2 = NETimer::Timer(2);
	TUint prescale = __e32_find_ms1_32(T2.iPrescaler & 0x3f);
	TUint f = 66666667 >> prescale;
	TUint64 x = I64LIT(1000000);
	x *= TUint64(aTicks);
	x += TUint64(f>>1);
	x /= TUint64(f);
	return (TUint)x;
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
#if defined(__MAWD__) || defined(__MEIG__)
	return aTicks*500;					// 2kHz tick
#endif
#if defined(__EPOC32__) && defined(__CPU_X86)
	return (aTicks*8381+4190)/10000;
#endif
#ifdef __WINS__
	LARGE_INTEGER f;
	QueryPerformanceFrequency(&f);
	aTicks*=1000000;
	aTicks+=f.QuadPart/2;
	aTicks/=f.QuadPart;
	return (TUint)aTicks;
#endif
	}

class DTick;
class TTickTimer
	{
public:
	enum TMode
		{
		EOneShot,
		EPeriodic,
		EAbsolute,
		ETickDelay,
		};
public:
	TTickTimer();
	TInt Start(TInt aMode, TUint aMin, TUint aRange, TInt aCount);
	void Cancel();
	void CompleteClient(TInt aValue);
	static void TickCallBack(TAny* aPtr);
	static void SecondCallBack(TAny* aPtr);
public:
	TTickLink iTickLink;
	TSecondLink iSecondLink;
	TMode iMode;
	TInt iInterval;
	TTimeK iExpiryTime;
	TUint iMin;
	TUint iRange;
	TInt iParam;
	TCounter iStartTime0;
	TUint iStartTime1;
	TCounter iStartTime;
	TInt iMinErr;
	TInt iMaxErr;
	Int64 iTotalErr;
	TInt iCount;
	TInt iRequestedCount;
	TInt iIgnore;
	DTick* iLdd;
	TInt iId;
	TClientRequest* iRequest;
	};

class DTickFactory : public DLogicalDevice
//
// Tick timer LDD factory
//
	{
public:
	DTickFactory();
	virtual TInt Install();						//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel);	//overriding pure virtual
	};

class DTick : public DLogicalChannelBase
//
// Tick timer LDD channel
//
	{
public:
	DTick();
	~DTick();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
public:
	void TimerExpired(TInt anId);
	inline DThread* Client() { return iThread; }
public:
	DThread* iThread;
	TInt iTickPeriodUs;
	TTimeK iYear2000;
	TTickTimer iTickTimer[KMaxTimers];
	TUint iSeed[2];
	};

TTickTimer::TTickTimer()
	:	iMode(EOneShot),
		iInterval(0),
		iExpiryTime(0),
		iMin(0),
		iRange(1),
		iParam(0),
		iStartTime0(0),
		iStartTime1(0),
		iStartTime(0),
		iMinErr(KMaxTInt),
		iMaxErr(KMinTInt),
		iTotalErr(0),
		iCount(0),
		iRequestedCount(0),
		iIgnore(0),
		iLdd(NULL),
		iId(0),
		iRequest(NULL)
	{
	}

void TTickTimer::TickCallBack(TAny* aPtr)
	{
	TCounter timer_val=TIMER();
	TTickTimer& m=*(TTickTimer*)aPtr;
	TDelta time=TimeDelta(m.iStartTime, timer_val);
	TInt time_us=TicksToMicroseconds(time);
	TInt rounded_interval=((m.iInterval+500)/1000)*1000;
	TInt error=time_us-rounded_interval;
	if (!m.iIgnore)
		{
		if (error<m.iMinErr)
			m.iMinErr=error;
		if (error>m.iMaxErr)
			m.iMaxErr=error;
		m.iTotalErr+=error;
		}
	if (m.iIgnore==1 && m.iMode==EPeriodic)
		{
		m.iStartTime0=timer_val;
		m.iStartTime1=NKern::TickCount();
		}
	if ((m.iIgnore && m.iIgnore--) || (++m.iCount<m.iRequestedCount))
		{
		if (m.iMode==EOneShot)
			{
			TUint rnd=Random(m.iLdd->iSeed);
			TUint ticks=(rnd%m.iRange)+m.iMin;
			m.iInterval=ticks*m.iLdd->iTickPeriodUs;
			m.iStartTime=TIMER();
			m.iTickLink.OneShot(m.iInterval, TickCallBack, &m);
			}
		else if (m.iMode==ETickDelay)
			{
			m.iStartTime=TIMER();
			m.iTickLink.OneShot(m.iInterval, TickCallBack, &m);
			NKern::Sleep(m.iRange);
			}
		else
			m.iStartTime=timer_val;
		return;
		}
	m.CompleteClient(KErrNone);
	if (m.iMode==EPeriodic)
		{
		m.iStartTime0=LongTimeDelta(m.iStartTime0, timer_val, m.iStartTime1, NKern::TickCount());
		m.iTickLink.Cancel();
		}
	}

void TTickTimer::SecondCallBack(TAny* aPtr)
	{
	TTickTimer& m=*(TTickTimer*)aPtr;
	TTimeK now=Kern::SystemTime();
	Int64 error=now-m.iExpiryTime;
	if (error>KMaxTInt)
		error=KMaxTInt;
	if (error<KMinTInt)
		error=KMinTInt;
	if (error<m.iMinErr)
		m.iMinErr=(TInt)error;
	if (error>m.iMaxErr)
		m.iMaxErr=(TInt)error;
	m.iTotalErr+=error;
	if (++m.iCount<m.iRequestedCount)
		{
		TUint rnd=Random(m.iLdd->iSeed);
		TUint secs=(rnd%m.iRange)+m.iMin;
		m.iExpiryTime=now+secs*1000000+999999;
		m.iExpiryTime/=1000000;
		m.iExpiryTime*=1000000;
		m.iSecondLink.At(m.iExpiryTime, SecondCallBack, &m);
		return;
		}
	m.CompleteClient(KErrNone);
	}

TInt TTickTimer::Start(TInt aMode, TUint a1, TUint a2, TInt aCount)
	{
	TInt r=KErrGeneral;
	iMode=(TMode)aMode;
	iMin=a1;
	iRange=a2;
	iMinErr=KMaxTInt;
	iMaxErr=KMinTInt;
	iTotalErr=0;
	iCount=0;
	iRequestedCount=aCount;
	switch (aMode)
		{
		case EOneShot:
			{
			TUint rnd=Random(iLdd->iSeed);
			TUint ticks=(rnd%iRange)+iMin;
			iInterval=ticks*iLdd->iTickPeriodUs;
			iStartTime=TIMER();
			iStartTime0=0;
			iStartTime1=0;
			iTickLink.OneShot(iInterval, TickCallBack, this);
			iIgnore=Min(aCount-1,1);
			r=KErrNone;
			break;
			}
		case EPeriodic:
			{
			iInterval=iMin*iLdd->iTickPeriodUs;
			iStartTime=TIMER();
			iStartTime0=iStartTime;
			iStartTime1=NKern::TickCount();
			iTickLink.Periodic(iInterval, TickCallBack, this);
			iIgnore=Min(aCount-1,1);
			r=KErrNone;
			break;
			}
		case EAbsolute:
			{
			TUint rnd=Random(iLdd->iSeed);
			TUint secs=(rnd%iRange)+iMin;
			TTimeK now=Kern::SystemTime();
			iExpiryTime=now+secs*1000000+999999;
			iExpiryTime/=1000000;
			iExpiryTime*=1000000;
			iSecondLink.At(iExpiryTime, SecondCallBack, this);
			iIgnore=0;
			iStartTime0=0;
			iStartTime1=0;
			r=KErrNone;
			break;
			}
		case ETickDelay:
			{
			iInterval=iMin*iLdd->iTickPeriodUs;
			iStartTime=TIMER();
			iStartTime0=0;
			iStartTime1=0;
			iTickLink.OneShot(iInterval, TickCallBack, this);
			iIgnore=Min(aCount-1,1);
			r=KErrNone;
			break;
			}
		default:
			break;
		}
	return r;
	}

void TTickTimer::CompleteClient(TInt aValue)
	{
	Kern::QueueRequestComplete(iLdd->Client(),iRequest,aValue);
	}

void TTickTimer::Cancel()
	{
	iTickLink.Cancel();
	iSecondLink.Cancel();
	CompleteClient(KErrCancel);
	}

DECLARE_STANDARD_LDD()
	{
    return new DTickFactory;
    }

DTickFactory::DTickFactory()
    {
    iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    //iParseMask=0;//No units, no info, no PDD
    //iUnitsMask=0;//Only one thing
    }

TInt DTickFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DTick on this logical device
//
    {
	aChannel=new DTick;
	return aChannel?KErrNone:KErrNoMemory;
    }

TInt DTickFactory::Install()
//
// Install the LDD - overriding pure virtual
//
    {
    return SetName(&KTickTestLddName);
    }

void DTickFactory::GetCaps(TDes8& aDes) const
//
// Get capabilities - overriding pure virtual
//
    {
    TCapsTickTestV01 b;
    b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
    }

DTick::DTick()
//
// Constructor
//
    {
	TInt i;
	for (i=0; i<KMaxTimers; i++)
		{
		iTickTimer[i].iLdd=this;
		iTickTimer[i].iId=i;
		}
	iSeed[0]=NKern::TickCount();
	iSeed[1]=0;
	iTickPeriodUs=Kern::TickPeriod();

	// Careful with the constants here or GCC will get it wrong
	iYear2000=Int64(KDaysFrom0ADTo2000AD)*Int64(KSecondsPerDay);
	iYear2000*=1000000;

	iThread=&Kern::CurrentThread();
	iThread->Open();
    }

TInt DTick::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
// Create channel
//
    {

    if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
		return KErrNotSupported;

	for (TInt i=0; i<KMaxTimers; i++)
		{
		TInt r = Kern::CreateClientRequest(iTickTimer[i].iRequest);
		if (r != KErrNone)
			return r;
		}
	
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
	// Set up timer 1 as free running 93.75KHz clock
    TIntegratorAP::SetTimerLoad(TIntegratorAP::ECounterTimer1, 0);			// start from 0xffff downwards
    TIntegratorAP::SetTimerMode(TIntegratorAP::ECounterTimer1, TIntegratorAP::ETimerModeFreeRunning);
    TIntegratorAP::SetTimerPreScale(TIntegratorAP::ECounterTimer1, TIntegratorAP::ETimerPreScaleDiv256);	// 93.75kHz wrap 699ms
    TIntegratorAP::EnableTimer(TIntegratorAP::ECounterTimer1, TIntegratorAP::EEnable);
	TIntegratorAP::DisableIrq(TIntegratorAP::EIrqSet0,EIntIdTimer0);		// make sure timer int is disabled
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
	// nothing to do since we use fast counter
#endif
#if defined(__MRAP__)
	// nothing to do here RTC runs with 32.768 kHz
#endif
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
#if defined(__EPOC32__) && defined(__CPU_X86)
	// Set up timer channel 2 as free running counter at 14318180/12 Hz
	SetUpTimerChannel2();
#endif
	return KErrNone;
	}

DTick::~DTick()
//
// Destructor
//
    {
	TInt i;
	for (i=0; i<KMaxTimers; i++)
		{
		iTickTimer[i].Cancel();
		Kern::DestroyClientRequest(iTickTimer[i].iRequest);
		}
	Kern::SafeClose((DObject*&)iThread, NULL);
    }

TInt DTick::Request(TInt aFunction, TAny* a1, TAny* a2)
//
// Runs in context of client thread, system unlocked on entry and exit
//
	{
	NKern::ThreadEnterCS();			// stop thread kills
	TInt r=KErrNone;
	TInt id=(TInt)a1;
	TTickTimer& m=iTickTimer[id];
	switch (aFunction)
		{
		case RTickTest::EControlStart:
			{
			STimerStartInfo info;
			kumemget(&info,a2,sizeof(info));
			r=m.iRequest->SetStatus(info.iStatus);
			if (r==KErrNone)
				r=m.Start(info.iMode, info.iMin, info.iRange, info.iCount);
			break;
			}
		case RTickTest::EControlStop:
			{
			m.Cancel();
			break;
			}
		case RTickTest::EControlGetInfo:
			{
			STickTestInfo info;
			info.iMinErr=m.iMinErr;
			info.iMaxErr=m.iMaxErr;
			info.iCount=m.iCount;
			info.iRequestedCount=m.iRequestedCount;
			Int64 avg=m.iTotalErr/m.iCount;
			info.iAvgErr=(TInt)avg;
			info.iTotalTime=TicksToMicroseconds(m.iStartTime0);
			kumemput(a2,&info,sizeof(info));
			break;
			}
		case RTickTest::EControlReadRtc:
			{
			TInt hwrtc;
			Arch::TheAsic()->SystemTimeInSecondsFrom2000(hwrtc);
			*(TTimeK*)a1=Kern::SystemTime();
			TTimeK hwtimeK=(TTimeK)hwrtc;
			hwtimeK*=1000000;
			hwtimeK+=iYear2000;
			kumemput(a2,&hwtimeK,sizeof(hwtimeK));
			break;
			}
		case RTickTest::EControlGetTickPeriod:
			{
			r=iTickPeriodUs;
			break;
			}
		default:
			r=KErrNotSupported;
			break;
		}
	NKern::ThreadLeaveCS();
	return r;
	}

