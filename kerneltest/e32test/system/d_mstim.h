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
// e32test\system\d_mstim.h
// 
//

#if !defined(__D_MSTIM_H__)
#define __D_MSTIM_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KMsTimerLddName,"MsTim");

class TCapsMsTimV01
	{
public:
	TVersion	iVersion;
	};

struct SMsTimerInfo
	{
	TInt iMin;
	TInt iMax;
	TInt iAvg;
	TInt iCount;
	};

struct SRandomTestInfo
	{
	TInt iMin;
	TInt iMax;
	TInt iXferC;
	TInt iCritC;
	TInt iStartFail;
	TInt iCallBacks;
	TInt iCompletions;
	};

class RMsTim : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControlStartPeriodicInt,
		EControlStartPeriodicDfc,
		EControlStopPeriodic,
		EControlGetInfo,
		EControlBeginRandomTest,
		EControlEndRandomTest,
		EControlGetRandomTestInfo,
		EControlGetIdleTime,
		};

	enum TRequest
		{
		ERequestOneShotInt,
		ERequestOneShotDfc,
		ERequestIntCancel,
		ERequestOneShotIntAgain,
		ERequestOneShotUserDfc,
		ERequestOneShotUserDfcAgain
		};
public:
#ifndef __KERNEL_MODE__
	inline TInt Open()
		{ return DoCreate(KMsTimerLddName(),TVersion(0,1,1),KNullUnit,NULL,NULL); }
	inline TInt StartPeriodicInt(TInt aId, TInt aInterval)
		{ return DoControl(EControlStartPeriodicInt, (TAny*)aId, (TAny*)aInterval); }
	inline TInt StartPeriodicDfc(TInt aId, TInt aInterval)
		{ return DoControl(EControlStartPeriodicDfc, (TAny*)aId, (TAny*)aInterval); }
	inline TInt StopPeriodic(TInt aId)
		{ return DoControl(EControlStopPeriodic, (TAny*)aId); }
	inline TInt GetInfo(TInt aId, SMsTimerInfo& anInfo)
		{ return DoControl(EControlGetInfo, (TAny*)aId, &anInfo); }
	inline TInt BeginRandomTest()
		{ return DoControl(EControlBeginRandomTest); }
	inline TInt EndRandomTest()
		{ return DoControl(EControlEndRandomTest); }
	inline TInt GetRandomTestInfo(SRandomTestInfo& anInfo)
		{ return DoControl(EControlGetRandomTestInfo, &anInfo); }
	inline TInt GetIdleTime()
		{ return DoControl(EControlGetIdleTime); }
	inline void StartOneShotInt(TRequestStatus& aStatus, TInt aId, TInt aInterval)
		{ DoRequest(ERequestOneShotInt, aStatus, (TAny*)aId, (TAny*)aInterval); }
	inline void StartOneShotDfc(TRequestStatus& aStatus, TInt aId, TInt aInterval)
		{ DoRequest(ERequestOneShotDfc, aStatus, (TAny*)aId, (TAny*)aInterval); }
	inline void IntCancel(TRequestStatus& aStatus, TInt aId, TInt aInterval)
		{ DoRequest(ERequestIntCancel, aStatus, (TAny*)aId, (TAny*)aInterval); }
	inline void StartOneShotIntAgain(TRequestStatus& aStatus, TInt aId, TInt aInterval)
		{ DoRequest(ERequestOneShotIntAgain, aStatus, (TAny*)aId, (TAny*)aInterval); }
	inline void StartOneShotUserDfc(TRequestStatus& aStatus, TInt aId, TInt aInterval)
		{ DoRequest(ERequestOneShotUserDfc, aStatus, (TAny*)aId, (TAny*)aInterval); }
	inline void StartOneShotUserDfcAgain(TRequestStatus& aStatus, TInt aId, TInt aInterval)
		{ DoRequest(ERequestOneShotUserDfcAgain, aStatus, (TAny*)aId, (TAny*)aInterval); }
#endif
	};

#endif
