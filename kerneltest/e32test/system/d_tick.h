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
// e32test\system\d_tick.h
// 
//

#if !defined(__D_TICK_H__)
#define __D_TICK_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KTickTestLddName,"TickTest");

const TInt KMaxTimers=8;

class TCapsTickTestV01
	{
public:
	TVersion	iVersion;
	};

struct STickTestInfo
	{
	TInt iMinErr;
	TInt iMaxErr;
	TInt iAvgErr;
	TUint iTotalTime;
	TInt iCount;
	TInt iRequestedCount;
	};

struct STimerStartInfo
	{
	TInt iMode;
	TInt iMin;
	TInt iRange;
	TInt iCount;
	TRequestStatus* iStatus;
	};

#define PERIODIC(i,s,iv,c)	\
	STimerStartInfo i; i.iStatus=&s; i.iMin=iv; i.iRange=1; i.iCount=c; i.iMode=EPeriodic
#define NSHOTREL(i,s,m,r,c)	\
	STimerStartInfo i; i.iStatus=&s; i.iMin=m; i.iRange=r; i.iCount=c; i.iMode=EOneShot
#define NSHOTABS(i,s,m,r,c)	\
	STimerStartInfo i; i.iStatus=&s; i.iMin=m; i.iRange=r; i.iCount=c; i.iMode=EAbsolute
#define NSHOTDLY(i,s,p,d,c)	\
	STimerStartInfo i; i.iStatus=&s; i.iMin=p; i.iRange=d; i.iCount=c; i.iMode=ETickDelay

class RTickTest : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControlStart,
		EControlStop,
		EControlGetInfo,
		EControlReadRtc,
		EControlGetTickPeriod,
		};

	enum TMode
		{
		EOneShot,
		EPeriodic,
		EAbsolute,
		ETickDelay,
		};
public:
#ifndef __KERNEL_MODE__
	inline TInt Open()
		{ return DoCreate(KTickTestLddName(),TVersion(0,1,1),KNullUnit,NULL,NULL); }
	inline TInt StartPeriodic(TRequestStatus& aStatus, TInt aId, TInt aInterval, TInt aCount)
		{ aStatus=KRequestPending; PERIODIC(info,aStatus,aInterval,aCount); return DoControl(EControlStart, (TAny*)aId, &info); }
	inline TInt StartNShotRel(TRequestStatus& aStatus, TInt aId, TInt aMin, TInt aRange, TInt aCount)
		{ aStatus=KRequestPending; NSHOTREL(info,aStatus,aMin,aRange,aCount); return DoControl(EControlStart, (TAny*)aId, &info); }
	inline TInt StartNShotAbs(TRequestStatus& aStatus, TInt aId, TInt aMin, TInt aRange, TInt aCount)
		{ aStatus=KRequestPending; NSHOTABS(info,aStatus,aMin,aRange,aCount); return DoControl(EControlStart, (TAny*)aId, &info); }
	inline TInt StartNShotDelay(TRequestStatus& aStatus, TInt aId, TInt aPeriod, TInt aDelay, TInt aCount)
		{ aStatus=KRequestPending; NSHOTDLY(info,aStatus,aPeriod,aDelay,aCount); return DoControl(EControlStart, (TAny*)aId, &info); }
	inline TInt Stop(TInt aId)
		{ return DoControl(EControlStop, (TAny*)aId); }
	inline TInt GetInfo(TInt aId, STickTestInfo& anInfo)
		{ return DoControl(EControlGetInfo, (TAny*)aId, &anInfo); }
	inline TInt ReadRtc(TTime& aSwRtc, TTime& aHwRtc)
		{ return DoControl(EControlReadRtc, &aSwRtc, &aHwRtc); }
	inline TInt TickPeriodUs()
		{ return DoControl(EControlGetTickPeriod); }
#endif
	};

#endif
