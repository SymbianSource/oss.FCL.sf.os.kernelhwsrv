// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\paging_info.cpp
// 
//

#include <e32debug.h>
#include <e32svr.h>
#include <u32hal.h>
#include <hal.h>
#include "paging_info.h"


const TUint8* BenchmarkNames[] =
	{
	(const TUint8*)"Read rom page",
	(const TUint8*)"Read code page",
	(const TUint8*)"Decompress",
	(const TUint8*)"Set code page free",
	(const TUint8*)"Set code page old",
	(const TUint8*)"Read media",
	(const TUint8*)"Fixup code page",
	(const TUint8*)"Read data page",
	(const TUint8*)"Write data page",
	(const TUint8*)"Del notify data page",
	(const TUint8*)"Read media data page",
	(const TUint8*)"Write media data page",
	(const TUint8*)"Rejuvenate page",
	};

__ASSERT_COMPILE(sizeof(BenchmarkNames)/sizeof(TUint8*) == EMaxPagingBm);


TInt PagingInfo::ResetConcurrency(TInt aLocDrvNo, TMediaPagingStats aMediaStats)
	{
	TInt r = UserSvr::HalFunction(EHalGroupVM, EVMHalResetConcurrencyInfo, NULL, NULL);
	if (r!=KErrNotSupported && r!=KErrNone)
		return r;
	if(aLocDrvNo>=0)
		r=UserSvr::HalFunction(EHalGroupMedia,EMediaHalResetConcurrencyInfo,(TAny*)aLocDrvNo,(TAny*)aMediaStats);
	if(r==KErrNotSupported)
		r = KErrNone;
	return r;
	}


TInt PagingInfo::PrintConcurrency(TInt aLocDrvNo, TMediaPagingStats aMediaStats)
	{
	SPagingConcurrencyInfo info;
	TInt r = UserSvr::HalFunction(EHalGroupVM, EVMHalGetConcurrencyInfo, &info, NULL);
	if (r!=KErrNotSupported && r!=KErrNone)
		return r;
	if (r == KErrNone)
		{
		RDebug::Printf("Concurrency info:");
		RDebug::Printf("  Max waiting threads == %d", info.iMaxWaitingCount);
		RDebug::Printf("  Max paging threads == %d", info.iMaxPagingCount);
		}
	if(aLocDrvNo>=0)
		{
		if(aMediaStats==EMediaPagingStatsAll || aMediaStats==EMediaPagingStatsRom)
			{
			SMediaROMPagingConcurrencyInfo info;		
			r=UserSvr::HalFunction(EHalGroupMedia,EMediaHalGetROMConcurrencyInfo,(TAny*)aLocDrvNo,&info);
			if (r!=KErrNotSupported && r!=KErrNone)
				return r;
			if(r==KErrNone)
				{
				RDebug::Printf("ROM paging media concurrency stats on drive %d:",aLocDrvNo);
				RDebug::Printf("  Total page-in issued whilst processing other page-ins              : %d",info.iTotalConcurrentReqs);
				RDebug::Printf("  Total page-in issued with at least one queue not empty             : %d",info.iTotalReqIssuedNonEmptyQ);
				RDebug::Printf("  Max pending page-in in the main queue                              : %d",info.iMaxReqsInPending);
				RDebug::Printf("  Max pending page-in in the deferred queue                          : %d",info.iMaxReqsInDeferred);
				RDebug::Printf("  Total page-in first-time deferred during this session              : %d",info.iTotalFirstTimeDeferrals);
				RDebug::Printf("  Total page-in re-deferred during this session                      : %d",info.iTotalReDeferrals);
				RDebug::Printf("  Maximum deferrals of any single page-in                            : %d",info.iMaxDeferrals);
				RDebug::Printf("  Total times the main queue was emptied during asynchronous request : %d",info.iTotalSynchEmptiedMainQ);
				RDebug::Printf("  Total page-in serviced from main queue during asynchronous request : %d",info.iTotalSynchServicedFromMainQ);
				RDebug::Printf("  Total page-in deferred from main queue during asynchronous request : %d",info.iTotalSynchDeferredFromMainQ);
				RDebug::Printf("  Total page-in DFC run with an empty main queue                     : %d",info.iTotalRunDry);
				RDebug::Printf("  Total dry runs of paging DFC avoided                               : %d",info.iTotalDryRunsAvoided);
				}
			}

		if(aMediaStats==EMediaPagingStatsAll || aMediaStats==EMediaPagingStatsCode)
			{
			SMediaCodePagingConcurrencyInfo infoCode;
			r=UserSvr::HalFunction(EHalGroupMedia,EMediaHalGetCodeConcurrencyInfo,(TAny*)aLocDrvNo,&infoCode);
			if(r==KErrNone)
				{
				RDebug::Printf("Media Code Paging concurrency stats on drive %d", aLocDrvNo);
				RDebug::Printf("  Total page-in issued whilst processing other page in requests      : %d", infoCode.iTotalConcurrentReqs);
				RDebug::Printf("  Total page-in issued with at least one queue not empty             : %d", infoCode.iTotalReqIssuedNonEmptyQ);
				RDebug::Printf("  Max pending page-in in the main queue                              : %d", infoCode.iMaxReqsInPending);
				RDebug::Printf("  Max pending page-in requests in the deferred queue                 : %d", infoCode.iMaxReqsInDeferred);
				RDebug::Printf("  Total page-in first-time deferred                                  : %d", infoCode.iTotalFirstTimeDeferrals);
				RDebug::Printf("  Total page-in re-deferred during this session (from deferred queue): %d", infoCode.iTotalReDeferrals);
				RDebug::Printf("  Max number of deferrals of any single page in                      : %d", infoCode.iMaxDeferrals);
				RDebug::Printf("  Total pagein serviced from main queue during asynchronous request  : %d", infoCode.iTotalSynchServicedFromMainQ);
				RDebug::Printf("  Total pagein deferred from main queue during asynchronous request  : %d", infoCode.iTotalSynchDeferredFromMainQ);
				}
			}
		}
	if (r == KErrNotSupported)
		r = KErrNone;
	return r;
	}


TInt PagingInfo::ResetEvents()
	{
	return UserSvr::HalFunction(EHalGroupVM,EVMHalResetEventInfo,0,0);
	}


TInt PagingInfo::PrintEvents()
	{
	SVMEventInfo info;
	TPckg<SVMEventInfo> infoBuf(info);
	TInt r = UserSvr::HalFunction(EHalGroupVM,EVMHalGetEventInfo,&infoBuf,0);
	if(r!=KErrNone)
		return r;
	RDebug::Printf("Event info:");
	RDebug::Printf("  Page fault events: %Ld", info.iPageFaultCount);
	RDebug::Printf("  Page in events: %Ld", info.iPageInReadCount);
	return KErrNone;
	}


TInt PagingInfo::ResetBenchmarks(TInt aLocDrvNo, TMediaPagingStats aMediaStats)
	{
	TInt r = KErrNone;
	for (TInt i = 0 ; r == KErrNone && i < EMaxPagingBm ; ++i)
		{
		r = UserSvr::HalFunction(EHalGroupVM, EVMHalResetPagingBenchmark, (TAny*)i, NULL);
		if (r!=KErrNotSupported && r!=KErrNone)
			return r;
		}
	if(aLocDrvNo>=0)
		r=UserSvr::HalFunction(EHalGroupMedia,EMediaHalResetPagingBenchmark,(TAny*)aLocDrvNo,(TAny*)aMediaStats);
	if(r==KErrNotSupported)
		r = KErrNone;
	return r;
	}


TInt PagingInfo::PrintBenchmarks(TInt aLocDrvNo, TMediaPagingStats aMediaStats)
	{
	RDebug::Printf("Paging benchmarks:");
	RDebug::Printf("  Name:                         Count:  Min (us):  Max(us):  Avg(us):");
	
	TInt freq = 0;
	TReal min;
	TReal max;
	TReal avg;
	TInt r = HAL::Get(HAL::EFastCounterFrequency, freq);
	if (r != KErrNone)
		return r;

	TReal mult = 1000000.0 / freq;
	
	for (TInt i = 0 ; i < EMaxPagingBm ; ++i)
		{
		SPagingBenchmarkInfo info;
		r = UserSvr::HalFunction(EHalGroupVM, EVMHalGetPagingBenchmark, (TAny*)i, &info);
		if (r!=KErrNotSupported && r!=KErrNone)
			return r;
		if (r == KErrNone)
			{
			min = 0.0;
			max = 0.0;
			avg = 0.0;
			if (info.iCount != 0)
				{
				min = info.iMinTime * mult;
				max = info.iMaxTime * mult;
				avg = (info.iTotalTime * mult) / info.iCount;
				}
			const TUint8* name = BenchmarkNames[i];
			RDebug::Printf("  %-30s %6d %9.1f %9.1f %9.1f", name, info.iCount, min, max, avg);
			}
		}

	if(aLocDrvNo>=0)
		{
		if(aMediaStats==EMediaPagingStatsAll || aMediaStats==EMediaPagingStatsRom)
			{
			SPagingBenchmarkInfo info;
			r=UserSvr::HalFunction(EHalGroupMedia,EMediaHalGetROMPagingBenchmark,(TAny*)aLocDrvNo,&info);
			if (r!=KErrNotSupported && r!=KErrNone)
				return r;
			if(r==KErrNone)
				{
				if (info.iCount != 0)
					{
					min = info.iMinTime * mult;
					max = info.iMaxTime * mult;
					avg = (info.iTotalTime * mult) / info.iCount;
					RDebug::Printf("ROM paging media benchmarks on drive %d:",aLocDrvNo);
					RDebug::Printf("  %-30s %6d %9.1f %9.1f %9.1f", "Page-in latency", info.iCount, min, max, avg);
					}
				}
			}

		if(aMediaStats==EMediaPagingStatsAll || aMediaStats==EMediaPagingStatsCode)
			{
			SPagingBenchmarkInfo info;
			r=UserSvr::HalFunction(EHalGroupMedia,EMediaHalGetCodePagingBenchmark,(TAny*)aLocDrvNo,&info);
			if (r!=KErrNotSupported && r!=KErrNone)
				return r;
			if(r==KErrNone)
				{
				if (info.iCount != 0)
					{
					min = info.iMinTime * mult;
					max = info.iMaxTime * mult;
					avg = (info.iTotalTime * mult) / info.iCount;
					RDebug::Printf("Code paging media benchmarks on drive %d:",aLocDrvNo);
					RDebug::Printf("  %-30s %6d %9.1f %9.1f %9.1f", "Page-in latency", info.iCount, min, max, avg);
					}
				}
			}

		if(aMediaStats==EMediaPagingStatsAll || aMediaStats==EMediaPagingStatsDataIn)
			{
			SPagingBenchmarkInfo info;
			r=UserSvr::HalFunction(EHalGroupMedia,EMediaHalGetDataInPagingBenchmark,(TAny*)aLocDrvNo,&info);
			if (r!=KErrNotSupported && r!=KErrNone)
				return r;
			if(r==KErrNone)
				{
				if (info.iCount != 0)
					{
					min = info.iMinTime * mult;
					max = info.iMaxTime * mult;
					avg = (info.iTotalTime * mult) / info.iCount;
					RDebug::Printf("Data page-in media benchmarks on drive %d:",aLocDrvNo);
					RDebug::Printf("  %-30s %6d %9.1f %9.1f %9.1f", "Page-in latency", info.iCount, min, max, avg);
					}
				}
			}

		if(aMediaStats==EMediaPagingStatsAll || aMediaStats==EMediaPagingStatsDataOut)
			{
			SPagingBenchmarkInfo info;
			r=UserSvr::HalFunction(EHalGroupMedia,EMediaHalGetDataOutPagingBenchmark,(TAny*)aLocDrvNo,&info);
			if (r!=KErrNotSupported && r!=KErrNone)
				return r;
			if(r==KErrNone)
				{
				if (info.iCount != 0)
					{
					min = info.iMinTime * mult;
					max = info.iMaxTime * mult;
					avg = (info.iTotalTime * mult) / info.iCount;
					RDebug::Printf("Data page-out media benchmarks on drive %d:",aLocDrvNo);
					RDebug::Printf("  %-30s %6d %9.1f %9.1f %9.1f", "Page-in latency", info.iCount, min, max, avg);
					}
				}
			}

		}

	return KErrNone;
	}


TInt PagingInfo::ResetAll(TInt aLocDrvNo, TMediaPagingStats aMediaStats)
	{
	TInt r = ResetEvents();
	if (r == KErrNone || r == KErrNotSupported)
		r = ResetConcurrency(aLocDrvNo,aMediaStats);
	if (r == KErrNone || r == KErrNotSupported)
		r = ResetBenchmarks(aLocDrvNo,aMediaStats);
	if (r == KErrNotSupported)
		r = KErrNone;
	return r;
	}


TInt PagingInfo::PrintAll(TInt aLocDrvNo, TMediaPagingStats aMediaStats)
	{
	TInt r = PrintEvents();
	if (r == KErrNone || r == KErrNotSupported)
		r = PrintConcurrency(aLocDrvNo,aMediaStats);
	if (r == KErrNone || r == KErrNotSupported)
		r = PrintBenchmarks(aLocDrvNo,aMediaStats);
	if (r == KErrNotSupported)
		r = KErrNone;
	return r;
	}


