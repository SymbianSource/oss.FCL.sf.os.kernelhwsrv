// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/defrag/perf/t_perf.h
// 
//

#ifndef _DEFRAG_PERF_T_PERF_H_
#define _DEFRAG_PERF_T_PERF_H_

#include "t_testdll.h"
#include "..\d_pagemove.h"

typedef TUint32 DTime_t;
//#define EXTRA_TRACE

#ifdef EXTRA_TRACE
#define TEST_PRINTF(x...) test.Printf(x)
#else
#define TEST_PRINTF(x...)
#endif

#define MAXCHUNK_SIZE (40 * 1024 * 1024)
#define MINCHUNK_SIZE (2 * 1024 * 1024)


extern TInt TestDLLPerformance(TInt aNum); 
class DefragLatency 
{
public:
	DefragLatency()
		{
		iIterations = iCummulative = iResult = 0;
		iMaxTime = (DTime_t)0;
		iMinTime = (DTime_t) -1;
		iCalDelay = 0;
		}
	~DefragLatency() {}

	void CalibrateTimer(RTest& test);

private:
	static inline TUint32 GetFastCounter(void)
		{
		return User::FastCounter();
		}
public:
	
	/** Log the new time diff and update iMaxTime and iMinTime appropriately
	*/
	inline void AddIteration(DTime_t aTimeDiff)
		{
		iTime1 = 0;
		iTime2 = 0;

		if (aTimeDiff > iMaxTime)
			iMaxTime = aTimeDiff;

		if (aTimeDiff < iMinTime)
			iMinTime = aTimeDiff;

		iCummulative += aTimeDiff;
		iIterations++;
		}

	inline void StartTimer(void)
		{
		iTime1 = GetFastCounter();
		}

	inline TUint32 StopTimer(RTest& aTest)
		{
		iTime2 = GetFastCounter();
		TUint32 diff = iTime2 - iTime1;
		if (iTime2 < iTime1)
			{
			aTest.Printf(_L("WARNING - Fast Counter rolled over.  Assuming only once and continuing\n"));
			diff = (KMaxTUint32 - iTime1) + iTime2;
			}
		AddIteration(diff);
		return diff;
		}
						
	DTime_t GetResult(DTime_t& aMax, DTime_t& aMin, DTime_t& aDelay)
		{
		iResult = (iCummulative / iIterations);
		aMax = iMaxTime;
		aMin = iMinTime;
		aDelay = iCalDelay;
		return iResult;
		}


	TInt iFastCounterFreq;
private:
	int iIterations;
	DTime_t iCalDelay;
	DTime_t iTime1, iTime2;
	DTime_t iMaxTime, iMinTime;
	DTime_t iCummulative;
	DTime_t iResult;
};


class DllDefrag
{
public:
	DllDefrag()
		{
		iFunc0Addr = (TInt8 *)(-1);
		iFuncNAddr = (TInt8 *)0;
		iLib = new RLibrary;
		}
	~DllDefrag()
		{
		delete iLib;
		}
	TInt LoadTheLib(TInt aIdx, RTest& aTest);
	TInt TestDLLPerformance(TInt aNum, RPageMove& aPageMove, RTest& aTest);
public:	
	RLibrary *iLib;
	TInt8 *iFunc0Addr;
	TInt8 *iFuncNAddr;
};

#endif // _DEFRAG_PERF_T_PERF_H_

