// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\realtime\d_latncy.h
// 
//

#if !defined(__D_LATNCY_H__)
#define __D_LATNCY_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

#if defined(__IS_OMAP2420__) || defined(__WAKEUP_3430__)
#include <shared_timer.h>
#endif

#if !defined(__EPOC32__) || !defined(__CPU_X86)
#if !defined(__SMP__)
#define __CAPTURE_EXTRAS
#endif
#endif

_LIT(KLatencyLddName,"Latency");

class TCapsLatencyV01
	{
public:
	TVersion	iVersion;
	};

struct SLatencyResults
	{
	TUint iIntRetAddr;
	TUint iIntTicks;
	TUint iKernThreadTicks;
	TUint iUserThreadTicks;
	TUint iIntSpsr;
	TUint iIntR14;
	};

class RLatency : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControlStart,
		EControlTicksPerMs,
		EControlGetResults,
		};
public:
	inline TInt Open();
	inline void Start();
	inline TUint TicksPerMs();
	inline TInt GetResults(SLatencyResults& aResults);
	};

#ifndef __KERNEL_MODE__
inline TInt RLatency::Open()
	{ return DoCreate(KLatencyLddName,TVersion(1,0,1),KNullUnit,NULL,NULL); }

inline void RLatency::Start()
	{ DoControl(EControlStart); }

inline TUint RLatency::TicksPerMs()
	{ return (TUint)DoControl(EControlTicksPerMs); }

inline TInt RLatency::GetResults(SLatencyResults& aResults)
	{ return DoControl(EControlGetResults,&aResults); }
#endif

#ifdef __KERNEL_MODE__
class DLatencyPowerHandler;
class DLatency : public DLogicalChannelBase
	{
public:
	DLatency();
	~DLatency();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
public:
	TInt StartTimer();
	void StopTimer();
	static void MsCallBack(TAny*);
	static void MsDfc(TAny*);
#if defined( __MISA__) || defined(__MCOT__) || defined(__IS_OMAP2420__) || defined(__WAKEUP_3430__)
	inline TUint Ticks();
#elif defined(__EPOC32__) && defined(__CPU_X86)
	static TUint Ticks();
#else
	inline static TUint Ticks();
#endif

#if defined(__IS_OMAP2420__) || defined(__WAKEUP_3430__)
private:
	void DisableTimer();
	TInt ConfigureTimer();
#endif
public:
	NTimer iMsCallBack;
	TDynamicDfcQue* iRtDfcQ;
	TDfc iMsDfc;
	SLatencyResults iResults;
#if !defined(__SMP__)
	TUint* iIntStackTop;
#endif
	DThread* iClient;
	DLatencyPowerHandler* iPowerHandler;
	TUint8 iOff;
	TUint8 iStarted;
	TUint8 iPad2;
	TUint8 iPad3;
#if defined( __MISA__) || defined(__MCOT__)
	TUint iTickIncrement;
	TUint iTriggerTime;
#endif
#if defined(__IS_OMAP2420__) || defined(__WAKEUP_3430__)
	TUint8 iGPTimerId;
	SOmapTimerInfo iTimerInfo;
	TUint iTimerLoadValue;
#endif
	};
#endif

#endif
