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
// e32test\debug\t_cache.cpp
// Overview:
// Performs cache checking in order to ensure it is running correctly.
// Runs automatically or as manual test. (Type "t_cache help" for details.)
// API Information:
// class Cache
// class L2Cache
// Details:
// -Test1 - Displays cache properties.
// -Test2 - Gets thresholds.
// -Test3 - Sets thresholds.
// -Test4 - Compares different memory mappings of data.
// -Test5 - Compares different memory mappings of code.
// -Test6 - Tests Write Back mode.
// -Test7 - Tests cache maintenance functions
// It can also perform more specified tests if run manually:
// - t_cache info				Test1 only
// - t_cache data <size_hex>	Runs data chunk test (Test 4) with specified chunk size. For example,
// "t_cache data 10000" will run data chunk test against 64KB chunk.
// - t_cache code <size_hex>	Runs code chunk test (Test 5) for specified chink size. For example,
// "t_cache code 10000" will run code chunk test against 64KB chunk.
// - t_cache data+<size_hex>	Runs data chunk test for specified set of chunk sizes. For example,
// "t_cache data+1000" will perform data chunk test for sizes 4K, 8K,... up to 512K.
// - t_cache code+<size_hex>	Runs code chunk test for specified set of sizes. For example,
// "t_cache code+1000" will perform code chunk test for sizes 4K, 8K,... up to 512K.
// - t_cache threshold			Diplays thresholds for all caches on the platform.
// - t_cache threshold T P C H	Sets new values for cache thresholds:
// - "T" specifies the type of cache to whose threshould are to be chenged:
// 1  for Instruction (or Unified) Cache.
// 2  for Data Cache (for ARMv7 and later, this is Point-of-Coherency threshold.
// 4  for XScale AltData Cache.
// 8  for Point-of-Unification Data Cache Threshold for ARMv7 and later platforms.
// 10 for L2 (L210 or XScale L2 cache)
// - "P" "C" & "H" are hex values for purge, clean & flush thresholds.
// For example: "t_cache 1 10000 10000 10000" sets 64KB for all thresholds in Instruction Cache.
// - t_cache usecase			Runs particular use case tests.
// - t_cache help				Displays the list of manual commands.
// Platforms/Drives/Compatibility:
// Hardware - ARM only (Manual). Not supported on emulator.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// d_cache.mmp to be built from the baseport.
// 
//

#include <e32test.h>
#include "d_cache.h"

//------------globals---------------------
LOCAL_D RTest test(_L("T_CACHE"));
_LIT(KPrintCacheInfos,"info");
_LIT(KTestData,"data");
_LIT(KTestCode,"code");
_LIT(KHelp,"help");
_LIT(KThreshold,"threshold");
_LIT(KIncremental,"+");
_LIT(KUseCase,"usecase");

RCacheTestDevice Device;
RCacheTestDevice::TCacheInfo CacheInfo;
TBuf<KCacheDescSize> TempBuff;

const TInt KDefaultDataSize = 0x20000;	
const TInt KDefaultCodeSize = 0x804;	//2K+4. Should be <= TestCodeFuncSize(). Otherwise, code test won't run against rom image.
const TInt KMaxChunkSize 	= 0x80000;	//512KB Incremental tests limit
const TInt KWriteBackTestSizeSize = 0x4000; // Shouldn't go over cache thresholds (where purge becomes flush).

extern void DataSegmetTestFunct(void* aBase, TInt aSize);

/** Loads & opens LDD.*/
void StartDriver()
	{
	TInt r = User::LoadLogicalDevice(KCacheTestDriverName);
	test( r==KErrNone || r==KErrAlreadyExists);
	if((r = Device.Open())!=KErrNone)	
		{
		User::FreeLogicalDevice(KCacheTestDriverName);
		test.Printf(_L("Could not open LDD"));
		test(0);
		}
	}

/** Closes and unloads LDD.*/
void StopDriver()
	{
	Device.Close();
	User::FreeLogicalDevice(KCacheTestDriverName);
	}

/** Get cache info from device driver. This will update CacheInfo global variable.*/
void GetCacheInfo()
	{
	TInt r = Device.GetCacheInfo(CacheInfo);
	test(r==KErrNone);
	}

//---------------------------------------------
//! @SYMTestCaseID			MMU-T_CACHE-01
//! @SYMTestType 			ST
//! @SYMPREQ 				PREQ305
//! @SYMREQ 				REQ5795
//! @SYMTestCaseDesc 		Displays cache properties.
//! @SYMTestExpectedResults KErrNone 
//! @SYMTestPriority 		Low
//! @SYMTestStatus 			Implemented
//---------------------------------------------
void Test1()
	{
	TInt i;
	
	test.Printf(_L("General Info:\n"));
	TPtr ptr = CacheInfo.iDesc.Expand();
	test.Printf(ptr);
	test.Printf(_L("CacheCount:%d, MaxCacheSize:%xH, MemoryRemapping:%d, OuterCache:%d\n"),CacheInfo.iCacheCount, CacheInfo.iMaxCacheSize, CacheInfo.iMemoryRemapping, CacheInfo.iOuterCache);
	test.Printf(_L("DMAMemoryAlignement:%d\n"),CacheInfo.iDmaBufferAlignment);


	test.Printf(_L("Per Level Info:\n"));
	test.Printf(_L("Level\tData\tInstr\tSize(b)\tLine(b)\tWays\tSets\tDescription\n"));

	for (i = 0; i<CacheInfo.iCacheCount; i++)
		{
		TempBuff.SetLength(0);
		RCacheTestDevice::TCacheSingle& cs = CacheInfo.iCache[i];
		TempBuff.Format(_L("%d\t%d\t%d\t%xH\t%xH\t%xH\t%xH\t"), cs.iLevel, cs.iData, cs.iCode, cs.iSize, cs.iLineSize, cs.iWays, cs.iSets);
		ptr = cs.iDesc.Expand();
		TempBuff.Append(ptr);
		TempBuff.Append(_L("\n"));
		test.Printf(TempBuff);
		}
	}

//---------------------------------------------
//! @SYMTestCaseID			MMU-T_CACHE-02
//! @SYMTestType 			ST
//! @SYMPREQ 				PREQ1068
//! @SYMREQ 				REQ5909
//! @SYMTestCaseDesc 		Gets thresholds.
//! @SYMTestActions 		Fetches Cache Thresholds from the driver.
//! @SYMTestExpectedResults KErrNone 
//! @SYMTestPriority 		High
//! @SYMTestStatus 			Implemented
//---------------------------------------------
void Test2()
	{
	RCacheTestDevice::TThresholdInfo info;
	
	test.Printf(_L("Cache: Purge  Clear  Flush\n"));
	
	info.iCacheType=1;
	if (KErrNone == Device.GetThreshold(info))
		test.Printf(_L("Instr:%6x %6x %6x\n"),info.iPurge,info.iClean,info.iFlush);
	
	info.iCacheType=2;
	if (KErrNone == Device.GetThreshold(info))
		test.Printf(_L("Data: %6x %6x %6x\n"),info.iPurge,info.iClean,info.iFlush);
	
	info.iCacheType=4;
	if (KErrNone == Device.GetThreshold(info))
		test.Printf(_L("AltD: %6x %6x %6x\n"),info.iPurge,info.iClean,info.iFlush);
	
	info.iCacheType=8;
	if (KErrNone == Device.GetThreshold(info))
		test.Printf(_L("D_IMB:%6x %6x %6x\n"),info.iPurge,info.iClean,info.iFlush);
	
	info.iCacheType=16;
	if (KErrNone == Device.GetThreshold(info))
		test.Printf(_L("L2:   %6x %6x %6x\n"),info.iPurge,info.iClean,info.iFlush);
	}

//---------------------------------------------
//! @SYMTestCaseID			MMU-T_CACHE-03
//! @SYMTestType 			ST
//! @SYMPREQ 				PREQ1068
//! @SYMREQ 				REQ5909
//! @SYMTestCaseDesc 		Sets thresholds.
//! @SYMTestActions 		Sets new values forr Cache Thresholds. Then, sets back the old values.
//! @SYMTestExpectedResults KErrNone 
//! @SYMTestPriority 		High
//! @SYMTestStatus 			Implemented
//---------------------------------------------
void Test3()
	{
	TInt i, tested=0;
	RCacheTestDevice::TThresholdInfo info[5]; //for 5 types og cache
	TInt returned[5];
	

	//Get the old values
	for (i=0;i<5;i++)
		{
		info[i].iCacheType= 1<<i;
		returned[i] = Device.GetThreshold(info[i]);
		}

	//Double them all
	for (i=0;i<5;i++)
		{
		if (returned[i] != KErrNone) continue; //not a valid cache type for running platform
		tested++;
		info[i].iPurge <<=1;
		info[i].iClean <<=1;
		info[i].iFlush <<=1;
		test(KErrNone==Device.SetThreshold(info[i]));
		}

	//Put back the old values
	for (i=0;i<5;i++)
		{
		if (returned[i] != KErrNone) continue; //not a valid cache type for running platform
		info[i].iPurge >>=1;
		info[i].iClean >>=1;
		info[i].iFlush >>=1;
		test(KErrNone==Device.SetThreshold(info[i]));
		}
	test.Printf(_L(" ... %d caches present & tested\n"), tested);
	}



void DoTest4(RCacheTestDevice::TCacheAttr aCacheAttr, RCacheTestDevice::TChunkTest& aDC, TInt& aTime1, TInt& aTime2)
	{
	aDC.iCacheAttr = aCacheAttr;
	
	aDC.iShared = EFalse;
	test(KErrNone==Device.TestDataChunk(aDC));
	aTime1 = aDC.iTime;

	aDC.iShared = ETrue;
	test(KErrNone==Device.TestDataChunk(aDC));
	aTime2 = aDC.iTime;
	}
//---------------------------------------------
//! @SYMTestCaseID 			MMU-T_CACHE-04
//! @SYMTestType 			ST
//! @SYMPREQ 				PREQ305
//! @SYMREQ 				REQ5795
//! @SYMTestCaseDesc 		Compares different memory mappings of data.
//! @SYMTestActions 		Runs the same performance test against data in the chunks with different cache attributes.
//!							Also, runs the same test against data from user & kernel heaps.
//! @SYMTestExpectedResults KErrNone 
//! @SYMTestPriority 		Low
//! @SYMTestStatus 			Implemented
//---------------------------------------------
void Test4(TInt aSize)
	{
	RCacheTestDevice::TChunkTest dC;
	dC.iSize = aSize;
	dC.iUseCase = 0;  //not used
	dC.iLoops = 0; //not used
	TInt timeNS=0, timeS=0;

	test.Printf(_L("                                  	Time\n"));
	test.Printf(_L("Mem_Type      ActualMapAttr	NotShared	Shared\n"));
	test.Printf(_L("----------------------------------------------\n"));

	DoTest4(RCacheTestDevice::E_FullyBlocking, dC, timeNS, timeS);
	test.Printf(_L("FullyBlocking   %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
	DoTest4(RCacheTestDevice::E_Buffered_NC, dC, timeNS, timeS);
	test.Printf(_L("Buffered_NC     %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
	DoTest4(RCacheTestDevice::E_Buffered_C, dC, timeNS, timeS);
	test.Printf(_L("Buffered_C      %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);


	DoTest4(RCacheTestDevice::E_InnerWT, dC, timeNS, timeS);
	test.Printf(_L("InnerWT         %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
	DoTest4(RCacheTestDevice::E_InnerWBRA, dC, timeNS, timeS);
	test.Printf(_L("InnerWBRA       %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
	DoTest4(RCacheTestDevice::E_InnerWB, dC, timeNS, timeS);
	test.Printf(_L("InnerWB         %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);

	if(CacheInfo.iOuterCache)
		{
		DoTest4(RCacheTestDevice::E_OuterWT, dC, timeNS, timeS);
		test.Printf(_L("OuterWT         %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
		DoTest4(RCacheTestDevice::E_OuterWBRA, dC, timeNS, timeS);
		test.Printf(_L("OuterWBRA       %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
		DoTest4(RCacheTestDevice::E_OuterWB, dC, timeNS, timeS);
		test.Printf(_L("OuterWB         %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);

		DoTest4(RCacheTestDevice::E_InOutWT, dC, timeNS, timeS);
		test.Printf(_L("InOutWT         %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
		DoTest4(RCacheTestDevice::E_InOutWBRA, dC, timeNS, timeS);
		test.Printf(_L("InOutWBRA       %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
		DoTest4(RCacheTestDevice::E_InOutWB, dC, timeNS, timeS);
		test.Printf(_L("InOutWB         %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
		}


	DoTest4(RCacheTestDevice::E_StronglyOrder, dC, timeNS, timeS);
	test.Printf(_L("StronglyOrder   %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
	DoTest4(RCacheTestDevice::E_Device, dC, timeNS, timeS);
	test.Printf(_L("Device          %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
	DoTest4(RCacheTestDevice::E_Normal_Uncached, dC, timeNS, timeS);
	test.Printf(_L("Normal_Uncached %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
	DoTest4(RCacheTestDevice::E_Normal_Cached, dC, timeNS, timeS);
	test.Printf(_L("Normal_Cached   %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);

    DoTest4(RCacheTestDevice::E_KernelInternal4, dC, timeNS, timeS);
    test.Printf(_L("KernelInternal4 %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
    DoTest4(RCacheTestDevice::E_PlatformSpecific5, dC, timeNS, timeS);
    test.Printf(_L("PlatSpecific5   %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
    DoTest4(RCacheTestDevice::E_PlatformSpecific6, dC, timeNS, timeS);
    test.Printf(_L("PlatSpecific6   %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
    DoTest4(RCacheTestDevice::E_PlatformSpecific7, dC, timeNS, timeS);
    test.Printf(_L("PlatSpecific7   %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
	
	if(CacheInfo.iMemoryRemapping)
		{
		DoTest4(RCacheTestDevice::E_InnerWT_Remapped, dC, timeNS, timeS);
		test.Printf(_L("InnerWT_Remap   %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
		DoTest4(RCacheTestDevice::E_InnerWBRA_Remapped, dC, timeNS, timeS);
		test.Printf(_L("InnerWBRA_Remap  %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
		DoTest4(RCacheTestDevice::E_InnerWB_Remapped, dC, timeNS, timeS);
		test.Printf(_L("InnerWB_Remap    %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);

		if(CacheInfo.iOuterCache)
			{
			DoTest4(RCacheTestDevice::E_OuterWT_Remapped, dC, timeNS, timeS);
			test.Printf(_L("OuterWT_Remap    %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
			DoTest4(RCacheTestDevice::E_OuterWBRA_Remapped, dC, timeNS, timeS);
			test.Printf(_L("OuterWBRA_Remap  %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
			DoTest4(RCacheTestDevice::E_OuterWB_Remapped, dC, timeNS, timeS);
			test.Printf(_L("OuterWB_Remap    %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);

			DoTest4(RCacheTestDevice::E_InOutWT_Remapped, dC, timeNS, timeS);
			test.Printf(_L("InOutWT_Remap    %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
			DoTest4(RCacheTestDevice::E_InOutWBRA_Remapped, dC, timeNS, timeS);
			test.Printf(_L("InOutWBRA_Remap  %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
			DoTest4(RCacheTestDevice::E_InOutWB_Remapped, dC, timeNS, timeS);
			test.Printf(_L("InOutWB_Remap    %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
			}
		}

	//Run against kernel heap - allow the test to fail due to OOM
	dC.iCacheAttr = RCacheTestDevice::E_Default;
	TInt r = Device.TestDataChunk(dC);
	if (r==KErrNone)			test.Printf(_L("Kernel Heap     ---------\t%7d\t-------\n"), dC.iTime);
	else if (r==KErrNoMemory)	test.Printf(_L("Kernel Heap     Cannot allocate memory\n"));
	else						test(0);//fail

	//Run against user heap - allow the test to fail due to OOM
	void* buffer = User::Alloc(dC.iSize);
	if (buffer == NULL)
		{
		test.Printf(_L("User Heap      Cannot allocate memory\n"));
		return;
		}
	TInt time = User::NTickCount();
	DataSegmetTestFunct(buffer , dC.iSize);
	time = User::NTickCount() - time;
	User::Free(buffer);
	test.Printf(_L("User Heap       ---------\t%7d\t-------\n"), time);

	}		


void DoTest5(RCacheTestDevice::TCacheAttr aCacheAttr, RCacheTestDevice::TChunkTest& aDC, TInt& aTime1, TInt& aTime2)
	{
	aDC.iCacheAttr = aCacheAttr;
	
	aDC.iShared = EFalse;
	test(KErrNone==Device.TestCodeChunk(aDC));
	aTime1 = aDC.iTime;

	aDC.iShared = ETrue;
	test(KErrNone==Device.TestCodeChunk(aDC));
	aTime2 = aDC.iTime;
	}
//---------------------------------------------
//! @SYMTestCaseID 			MMU-T_CACHE-05
//! @SYMTestType 			ST
//! @SYMPREQ 				PREQ305
//! @SYMREQ 				REQ5795
//! @SYMTestCaseDesc 		Compares different memory mappings of code.
//! @SYMTestActions 		Runs the same performance test against code in chunks with different cache attributes.
//!							Also, runs the same test against code from rom..
//! @SYMTestExpectedResults KErrNone 
//! @SYMTestPriority 		Low
//! @SYMTestStatus 			Implemented
//---------------------------------------------
void Test5(TInt aSize)
	{
	RCacheTestDevice::TChunkTest dC;
	dC.iSize = aSize;
	dC.iUseCase = 0;  //not used
	dC.iLoops = 0; //not used
	TInt timeNS=0, timeS=0;

	test.Printf(_L("                                  	Time\n"));
	test.Printf(_L("Mem_Type   AttemptedMapAttr	NotShared	Shared\n"));
	test.Printf(_L("----------------------------------------------\n"));

	DoTest5(RCacheTestDevice::E_FullyBlocking, dC, timeNS, timeS);
	test.Printf(_L("FullyBlocking   %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
	DoTest5(RCacheTestDevice::E_Buffered_NC, dC, timeNS, timeS);
	test.Printf(_L("Buffered_NC     %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
	DoTest5(RCacheTestDevice::E_Buffered_C, dC, timeNS, timeS);
	test.Printf(_L("Buffered_C      %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);


	DoTest5(RCacheTestDevice::E_InnerWT, dC, timeNS, timeS);
	test.Printf(_L("InnerWT         %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
	DoTest5(RCacheTestDevice::E_InnerWBRA, dC, timeNS, timeS);
	test.Printf(_L("InnerWBRA       %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
	DoTest5(RCacheTestDevice::E_InnerWB, dC, timeNS, timeS);
	test.Printf(_L("InnerWB         %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);

	if(CacheInfo.iOuterCache)
		{
		DoTest5(RCacheTestDevice::E_OuterWT, dC, timeNS, timeS);
		test.Printf(_L("OuterWT         %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
		DoTest5(RCacheTestDevice::E_OuterWBRA, dC, timeNS, timeS);
		test.Printf(_L("OuterWBRA       %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
		DoTest5(RCacheTestDevice::E_OuterWB, dC, timeNS, timeS);
		test.Printf(_L("OuterWB         %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);

		DoTest5(RCacheTestDevice::E_InOutWT, dC, timeNS, timeS);
		test.Printf(_L("InOutWT         %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
		DoTest5(RCacheTestDevice::E_InOutWBRA, dC, timeNS, timeS);
		test.Printf(_L("InOutWBRA       %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
		DoTest5(RCacheTestDevice::E_InOutWB, dC, timeNS, timeS);
		test.Printf(_L("InOutWB         %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
		}


	DoTest5(RCacheTestDevice::E_StronglyOrder, dC, timeNS, timeS);
	test.Printf(_L("StronglyOrder   %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
	DoTest5(RCacheTestDevice::E_Device, dC, timeNS, timeS);
	test.Printf(_L("Device          %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
	DoTest5(RCacheTestDevice::E_Normal_Uncached, dC, timeNS, timeS);
	test.Printf(_L("Normal_Uncached %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
	DoTest5(RCacheTestDevice::E_Normal_Cached, dC, timeNS, timeS);
	test.Printf(_L("Normal_Cached   %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);

	if(CacheInfo.iMemoryRemapping)
		{
		DoTest5(RCacheTestDevice::E_InnerWT_Remapped, dC, timeNS, timeS);
		test.Printf(_L("InnerWT_Remap   %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
		DoTest5(RCacheTestDevice::E_InnerWBRA_Remapped, dC, timeNS, timeS);
		test.Printf(_L("InnerWBRA_Remap  %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
		DoTest5(RCacheTestDevice::E_InnerWB_Remapped, dC, timeNS, timeS);
		test.Printf(_L("InnerWB_Remap    %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);

		if(CacheInfo.iOuterCache)
			{
			DoTest5(RCacheTestDevice::E_OuterWT_Remapped, dC, timeNS, timeS);
			test.Printf(_L("OuterWT_Remap    %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
			DoTest5(RCacheTestDevice::E_OuterWBRA_Remapped, dC, timeNS, timeS);
			test.Printf(_L("OuterWBRA_Remap  %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
			DoTest5(RCacheTestDevice::E_OuterWB_Remapped, dC, timeNS, timeS);
			test.Printf(_L("OuterWB_Remap    %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);

			DoTest5(RCacheTestDevice::E_InOutWT_Remapped, dC, timeNS, timeS);
			test.Printf(_L("InOutWT_Remap    %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
			DoTest5(RCacheTestDevice::E_InOutWBRA_Remapped, dC, timeNS, timeS);
			test.Printf(_L("InOutWBRA_Remap  %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
			DoTest5(RCacheTestDevice::E_InOutWB_Remapped, dC, timeNS, timeS);
			test.Printf(_L("InOutWB_Remap    %08xH\t%7d\t%7d\n"),dC.iActualMapAttr, timeNS, timeS);
			}
		}

	//Run against kernel heap - allow the test to fail due to OOM
		dC.iCacheAttr = RCacheTestDevice::E_Default;
		TInt r = Device.TestCodeChunk(dC);
		if (r==KErrNone)			test.Printf(_L("Run from rom   ---------\t%7d\t-------\n"), dC.iTime);
		else if (r==KErrNoMemory)	test.Printf(_L("Run from rom    Cannot allocate memory\n"));
		else						test(0);//fail

	}



//---------------------------------------------
//! @SYMTestCaseID 			MMU-T_CACHE-06
//! @SYMTestType 			ST
//! @SYMPREQ 				PREQ305
//! @SYMREQ 				REQ5795
//! @SYMTestCaseDesc 		Tests Write Back mode.
//! @SYMTestActions 		The driver allocates write back chunk, write data into it and invalidate (aka purge)
//							the chunk from the cache. Then, it counts the number of bytes of the chunk that
//							reached the physical memory.
//! @SYMTestExpectedResults KErrNone 
//! @SYMTestPriority 		Low
//! @SYMTestStatus 			Implemented
//---------------------------------------------

void Test6()
	{
	test.Printf(_L("Test4: Testing WriteBack cache mode...\n"));
	RCacheTestDevice::TChunkTest dC;


	test.Printf(_L("Cache\tMemType\tChecking\tSize\tBytesInRAM\tVerdict\n"));
	test.Printf(_L("-----\t-------\t--------\t----\t----------\t-------\n"));
//
	dC.iSize = KWriteBackTestSizeSize;
	if(CacheInfo.iMemoryRemapping)	dC.iCacheAttr = RCacheTestDevice::E_InnerWBRA_Remapped;
	else							dC.iCacheAttr = RCacheTestDevice::E_InnerWBRA;
	test(Device.TestWriteBackReadAllocate(dC)==KErrNone);
	if (dC.iSize<KWriteBackTestSizeSize)test.Printf(_L("Inner\tWBRA\tReadAlloc\t%xH\t%xH\tOK\n"), KWriteBackTestSizeSize, dC.iSize);
	else								test.Printf(_L("Inner\tWBRA\tReadAlloc\t%xH\t%xH\tWarning: Didn't pass\n"), KWriteBackTestSizeSize, dC.iSize);
//
	dC.iSize = KWriteBackTestSizeSize;
	if(CacheInfo.iMemoryRemapping)	dC.iCacheAttr = RCacheTestDevice::E_InnerWB_Remapped;
	else							dC.iCacheAttr = RCacheTestDevice::E_InnerWB;
	test(Device.TestWriteBackReadAllocate(dC)==KErrNone);
	if (dC.iSize<KWriteBackTestSizeSize)test.Printf(_L("Inner\tWBWA\tReadAlloc\t%xH\t%xH\tOK\n"), KWriteBackTestSizeSize, dC.iSize);
	else								test.Printf(_L("Inner\tWBWA\tReadAlloc\t%xH\t%xH\tWarning: Didn't pass\n"), KWriteBackTestSizeSize, dC.iSize);
//
	dC.iSize = KWriteBackTestSizeSize;
	if(CacheInfo.iMemoryRemapping)	dC.iCacheAttr = RCacheTestDevice::E_InnerWB_Remapped;
	else							dC.iCacheAttr = RCacheTestDevice::E_InnerWB;
	test(Device.TestWriteBackWriteAllocate(dC)==KErrNone);
	if (dC.iSize<KWriteBackTestSizeSize)test.Printf(_L("Inner\tWBWA\tWriteAlloc\t%xH\t%xH\tOK\n"), KWriteBackTestSizeSize, dC.iSize);
	else								test.Printf(_L("Inner\tWBWA\tWriteAlloc\t%xH\t%xH\tWarning: Didn't pass\n"), KWriteBackTestSizeSize, dC.iSize);
	
	if(!CacheInfo.iOuterCache) return;
		
	dC.iSize = KWriteBackTestSizeSize;
	if(CacheInfo.iMemoryRemapping)	dC.iCacheAttr = RCacheTestDevice::E_OuterWBRA_Remapped;
	else							dC.iCacheAttr = RCacheTestDevice::E_OuterWBRA;
	test(Device.TestWriteBackReadAllocate(dC)==KErrNone);
	if (dC.iSize<KWriteBackTestSizeSize)test.Printf(_L("Outer\tWBRA\tReadAlloc\t%xH\t%xH\tOK\n"), KWriteBackTestSizeSize, dC.iSize);
	else								test.Printf(_L("Outer\tWBRA\tReadAlloc\t%xH\t%xH\tWarning: Didn't pass\n"), KWriteBackTestSizeSize, dC.iSize);
//
	dC.iSize = KWriteBackTestSizeSize;
	if(CacheInfo.iMemoryRemapping)	dC.iCacheAttr = RCacheTestDevice::E_OuterWB_Remapped;
	else							dC.iCacheAttr = RCacheTestDevice::E_OuterWB;
	test(Device.TestWriteBackReadAllocate(dC)==KErrNone);
	if (dC.iSize<KWriteBackTestSizeSize)test.Printf(_L("Outer\tWBWA\tReadAlloc\t%xH\t%xH\tOK\n"), KWriteBackTestSizeSize, dC.iSize);
	else								test.Printf(_L("Outer\tWBWA\tReadAlloc\t%xH\t%xH\tWarning: Didn't pass\n"), KWriteBackTestSizeSize, dC.iSize);
//
	dC.iSize = KWriteBackTestSizeSize;
	if(CacheInfo.iMemoryRemapping)	dC.iCacheAttr = RCacheTestDevice::E_OuterWB_Remapped;
	else							dC.iCacheAttr = RCacheTestDevice::E_OuterWB;
	test(Device.TestWriteBackWriteAllocate(dC)==KErrNone);
	if (dC.iSize<KWriteBackTestSizeSize)test.Printf(_L("Outer\tWBWA\tWriteAlloc\t%xH\t%xH\tOK\n"), KWriteBackTestSizeSize, dC.iSize);
	else								test.Printf(_L("Outer\tWBWA\tWriteAlloc\t%xH\t%xH\tWarning: Didn't pass\n"), KWriteBackTestSizeSize, dC.iSize);
	}



//---------------------------------------------
//! @SYMTestCaseID 			MMU-T_CACHE-07
//! @SYMTestType 			UT
//! @SYMPREQ 				PREQ305
//! @SYMREQ 				REQ5795
//! @SYMTestCaseDesc 		Tests Cache Maintanence Functions.
//! @SYMTestActions 		Does not do any sort of functional test. Just makes sure nothing panics.
//! @SYMTestExpectedResults KErrNone 
//! @SYMTestPriority 		High
//! @SYMTestStatus 			Implemented
//---------------------------------------------
void Test7()
	{
	test(KErrNone== Device.TestL2Maintenance());
	}

// The main function of automatic test.
void AutoTestL()
	{
	test.Start(_L("Test1: Display cache attributes:"));
	test.Printf(_L("Starting Driver...\n"));
	StartDriver();	
	test.Printf(_L("Getting CacheInfo...\n"));
	GetCacheInfo();
	Test1();

	test.Next(_L("Test2:Thresholds (hex)..."));
	Test2();

	test.Next(_L("Test3:Setting Cache Thresholds..."));
	Test3();

	test.Next(_L("Test4: Data chunk test..."));
	test.Printf(_L("chunk size=%xH \n"),KDefaultDataSize);
	Test4(KDefaultDataSize);

	test.Next(_L("Test5: Code chunk test..."));
	test.Printf(_L("chunk size=%xH \n"),KDefaultCodeSize);
	Test5(KDefaultCodeSize);

	test.Next(_L("Test6: Testing WriteBack cache mode..."));
	Test6(); 

	test.Next(_L("Test7: Testing L2 cache maintenance..."));
	Test7();

	StopDriver();	
	test.End();
	}

//////////////////////Manual tests start here///////////////////////////////

// Gets the size of the chunk from the command line
TInt GetSizeFromCommandLine(TBuf<64>& command)
	{
	TUint arg;
	TInt length = command.Length()-5;
	if (length <=0)
		return KErrArgument;
	TPtrC ptr = command.Mid(5,length);
	TLex lex(ptr);
	lex.Val(arg, EHex);
	return arg;
	}

/** Invoked by "t_cache info"*/
void ManualCacheInfo()
	{
	
	test.Start(_L("Cache Info:"));
	StartDriver();	
	GetCacheInfo();
	Test1();
	test.Printf(_L("Press any key...\n"));
	test.Getch();
	StopDriver();
	test.End();		
	return;
	}

/** Invoked by "t_cache code threshold [<C> <P> <C> <F>]"*/
TInt ManualThresholds(TBuf<64>& command)
	{
	TUint arg[4];
	TInt argCurrent=0;
	TInt argStart = 9; //jump over "threshold"
	TInt argEnd;
	TChar c;

	test.Start(_L("Thresholds:"));
	StartDriver();	
	GetCacheInfo();


	// Decode input arguments from the command line
	while (argCurrent<4)
		{
		find_arg_start:
		if (argStart >= command.Length()) break;
		c = command[argStart];
		if (c.IsSpace())
			{
			argStart++;
			goto find_arg_start;
			}
		
		argEnd = argStart+1;
		find_arg_end:
		if (argEnd >= command.Length()) goto get_arg;
		c = command[argEnd];
		if (c.IsSpace()) goto get_arg;
		argEnd++;
		goto find_arg_end;

		get_arg:
		TPtrC ptr = command.Mid(argStart,argEnd-argStart);
		TLex lex(ptr);
		lex.Val(arg[argCurrent++], EHex);
		argStart=argEnd;
		}

	test.Printf(_L("%d argument(s) decoded\n"),argCurrent);
	
	RCacheTestDevice::TThresholdInfo info;

	//If te values are provided in the command line, set thresholds with the given paramaters.
	if (argCurrent == 4)
		{
		test.Printf(_L("Setting thresholds: ...\n"));
		test.Printf(_L("Cache Type:%xh P:%xh C:%xh F:%xh\n"),arg[0], arg[1], arg[2], arg[3]);
		info.iCacheType=arg[0];
		info.iPurge = arg[1];
		info.iClean = arg[2];
		info.iFlush = arg[3];
		TInt r = Device.SetThreshold(info);
		test.Printf(_L("... returned %d\n"),r);
		}

	//Read thresholds from Kernel.
	test.Printf(_L("Reading thresholds(hex)...\n"));
	Test2();

	test.Printf(_L("Press any key...\n"));
	test.Getch();
	StopDriver();
	test.End();		
	return 0;
	}


/** Invoked by "t_cache data <size>"*/
void ManualDataTest(TInt aSize)
	{
	StartDriver();	
	GetCacheInfo();

	Test4(aSize);

	test.Printf(_L("Press any key...\n"));
	test.Getch();
	StopDriver();	
	test.End();
	}

/** Invoked by "t_cache code <size>"*/
void ManualCodeTest(TInt aSize)
	{
	StartDriver();	
	GetCacheInfo();

	Test5(aSize);

	test.Printf(_L("Press any key...\n"));
	test.Getch();
	StopDriver();	
	test.End();
	}

void DoUseCase(TInt aUseCase, TInt aMaxSize, TInt aLoops )
	{
	RCacheTestDevice::TChunkTest dC(aUseCase, 0x1000, aLoops);
	
	TInt time[5];

	test.Printf(_L("size(H)\tloops\tNormal/NC\tNormal/WT\tFullyCached\n"));
	test.Printf(_L("-------\t-----\t---------\t---------\t-----------\n"));

	while (dC.iSize<=aMaxSize)
		{
		dC.iCacheAttr = RCacheTestDevice::E_Normal_Cached;
		test(Device.TestUseCase(dC)==KErrNone);
		time[2]=dC.iTime;

		if(time[2] < 20)
			{dC.iLoops *=2;	continue;} //Time too short. Double the loops and try the same chunk size.

		dC.iCacheAttr = RCacheTestDevice::E_Normal_Uncached;
		test(Device.TestUseCase(dC)==KErrNone);
		time[0]=dC.iTime;

		if(CacheInfo.iMemoryRemapping)	dC.iCacheAttr = RCacheTestDevice::E_InOutWT_Remapped;
		else							dC.iCacheAttr = RCacheTestDevice::E_InOutWT;
		test(Device.TestUseCase(dC)==KErrNone);
		time[1]=dC.iTime;


		test.Printf(_L("%6xH\t%5d\t%9d\t%9d\t%11d\n"),dC.iSize,dC.iLoops,time[0],time[1],time[2]);

		if ((time[2] > 100) && (dC.iLoops >= 8))
			dC.iLoops /=2; //Time too long. Half the loops.

		dC.iSize+=0x1000; // Next chunk size.
		}
	}		

/** Invoked by "t_cache usecase"*/
void ManualUseCase()
	{
	test.Start(_L("Use Case manual tests"));
	StartDriver();	
	GetCacheInfo();

	test.Printf(_L("\nUseCase: Read From Chunk\n"));
	DoUseCase(0,Min(CacheInfo.iMaxCacheSize*4, 0x40000),32);
	
	test.Printf(_L("\nUseCase: Read From Chunk & Read From Heap\n"));
	DoUseCase(1,Min(CacheInfo.iMaxCacheSize*4, 0x40000),32);

	test.Printf(_L("\nUseCase: Write To Chunk\n"));
	DoUseCase(2,Min(CacheInfo.iMaxCacheSize*4, 0x40000),32);
	
	test.Printf(_L("\nUseCase: Write To Chunk & Read From Heap\n"));
	DoUseCase(3,Min(CacheInfo.iMaxCacheSize*4, 0x40000),32);


	test.Printf(_L("Press any key...\n"));
	test.Getch();
	StopDriver();	
	test.End();
	}


// Invoked by "t_cache data+<size_hex>"
void ManualDataTestIncremental(TInt aIncrement)
	{
	TInt time[4];
	TInt r = KErrNone;
	TInt size = aIncrement;
	RCacheTestDevice::TChunkTest dC;

	StartDriver();	
	GetCacheInfo();

	test.Printf(_L("Chunk\t\tTime(KernelTicks):\n"));
	test.Printf(_L("Size(KB)\tUnCached\tInner\tOuter\tIn&Out\n"));

	while(size < KMaxChunkSize)
		{
		dC.iSize = size;

		dC.iCacheAttr = RCacheTestDevice::E_Buffered_C;
		r = Device.TestDataChunk(dC);
		if (r!=KErrNone) break;
		time[0] = dC.iTime;

		if(CacheInfo.iMemoryRemapping)	dC.iCacheAttr = RCacheTestDevice::E_InnerWB_Remapped;
		else							dC.iCacheAttr = RCacheTestDevice::E_InnerWB;
		r = Device.TestDataChunk(dC);
		if (r!=KErrNone) break;
		time[1] = dC.iTime;

		if(CacheInfo.iOuterCache)
			{
			if(CacheInfo.iMemoryRemapping)	dC.iCacheAttr = RCacheTestDevice::E_OuterWB_Remapped;
			else							dC.iCacheAttr = RCacheTestDevice::E_OuterWB;
			r = Device.TestDataChunk(dC);
			if (r!=KErrNone) break;
			time[2] = dC.iTime;

			dC.iCacheAttr = RCacheTestDevice::E_InOutWB;
			r = Device.TestDataChunk(dC);
			if (r!=KErrNone) break;
			time[3] = dC.iTime;
			}
		else 
			{
			time[2]= time[3]=0;
			}
		test.Printf(_L("%d\t%d\t%d\t%d\t%d\n"),size/0x400, time[0],time[1],time[2],time[3]);
		size += aIncrement;
		}

	test.Printf(_L("The test exited with %d\n"), r);
	test.Printf(_L("Press any key...\n"));
	test.Getch();
	StopDriver();	
	test.End();
	}

// Invoked by "t_cache code+<size_hex>"
void ManualCodeTestIncremental(TInt aIncrement)
	{
	TInt time[4];
	TInt r = KErrNone;
	TInt size = aIncrement;
	RCacheTestDevice::TChunkTest dC;

	StartDriver();	
	GetCacheInfo();

	test.Printf(_L("Chunk\t\tTime(KernelTicks):\n"));
	test.Printf(_L("Size(KB)\tUnCached\tInner\tOuter\tIn&Out\n"));

	while(size < KMaxChunkSize)
		{
		TempBuff.SetLength(0);

		dC.iSize = size;

		dC.iCacheAttr = RCacheTestDevice::E_Buffered_C;
		r = Device.TestCodeChunk(dC);
		if (r!=KErrNone) break;
		time[0] = dC.iTime;

		if(CacheInfo.iMemoryRemapping)	dC.iCacheAttr = RCacheTestDevice::E_InnerWB_Remapped;
		else							dC.iCacheAttr = RCacheTestDevice::E_InnerWB;
		r = Device.TestCodeChunk(dC);
		if (r!=KErrNone) break;
		time[1] = dC.iTime;

		if(CacheInfo.iOuterCache)
			{
			if(CacheInfo.iMemoryRemapping)	dC.iCacheAttr = RCacheTestDevice::E_OuterWB_Remapped;
			else							dC.iCacheAttr = RCacheTestDevice::E_OuterWB;
			r = Device.TestCodeChunk(dC);
			if (r!=KErrNone) break;
			time[2] = dC.iTime;
			//
			dC.iCacheAttr = RCacheTestDevice::E_InOutWB;
			r = Device.TestCodeChunk(dC);
			if (r!=KErrNone) break;
			time[3] = dC.iTime;
			}
		else
			{
			time[2]=time[3] = 0;
			}	

		test.Printf(_L("%d\t%d\t%d\t%d\t%d\n"),size/0x400, time[0],time[1],time[2],time[3]);
		size += aIncrement;
		}

	test.Printf(_L("The test exited with %d\n"), r);
	test.Printf(_L("Press any key...\n"));
	test.Getch();
	StopDriver();	
	test.End();
	}

TInt E32Main()
	{

	TBuf<64> c;
	TInt size;
	User::CommandLine(c);
	if (c.FindF(KPrintCacheInfos) >= 0) {ManualCacheInfo();		return 0;}
	if (c.FindF(KUseCase) >= 0)			{ManualUseCase();		return 0;}
	if (c.FindF(KThreshold) >= 0)		{ManualThresholds(c);	return 0;}

	if (c.FindF(KTestData) >= 0)
		{
		test.Start(_L("Data Chunk"));
		size = GetSizeFromCommandLine(c);
		// Always round up the size to 1K boundary (because of DataSegmetTestFunct)
		size +=0x3ff; size &=~0x3ff;
		
		if (c.FindF(KIncremental) >= 0)
			{
			// Invoked by "t_cache data+<size>"
			test.Printf(_L(" size + %xH\n"),size);
			if (size<=0)
					return KErrArgument;
			ManualDataTestIncremental(size);	
			}
		else
			{
			// Invoked by "t_cache data <size>"
			test.Printf(_L("chunk size %xH\n"),size);
			if (size<=0)
					return KErrArgument;
			ManualDataTest(size);	
			}
		return 0;
		}

	if (c.FindF(KTestCode) >= 0)
		{
		test.Start(_L("Code Chunk"));
		size = GetSizeFromCommandLine(c);
		// Always round up the size to 1K boundary
		size +=0x3ff; size &=~0x3ff;
		if (c.FindF(KIncremental) >= 0)
			{
			// Invoked by "t_cache code+<size>"
			test.Printf(_L(" size + %xH\n"),size);
			if (size<=0)
					return KErrArgument;
			ManualCodeTestIncremental(size);	
			}
		else
			{
			// Invoked by "t_cache code <size>"
			test.Printf(_L("chunk size %xH\n"),size);
			if (size<=0)
					return KErrArgument;
			ManualCodeTest(size);	
			}
		return 0;
		}

	if (c.FindF(KHelp) >= 0)
		{
		// Invoked by "t_cache help"
		test.Start(_L("t_cache usage:\n"));
		test.Printf(_L("t_cache info\n"));
		test.Printf(_L("t_cache data <size_hex>\n"));
		test.Printf(_L("t_cache code <size_hex>\n"));
		test.Printf(_L("t_cache data+<size_hex>\n"));
		test.Printf(_L("t_cache code+<size_hex>\n"));
		test.Printf(_L("t_cache usecase\n\n"));
		test.Printf(_L("t_cache threshold [<cache> <purge> <clean> <flush>]\n\n"));
		test.Printf(_L("... where <cache>= 1,2,4,8 or 10\n\n"));
		test.Printf(_L("Press any key...\n"));
		test.Getch();
		test.End();
		return 0;
		}

	
	// auto test starts here
	CTrapCleanup* trap = CTrapCleanup::New();
	if (!trap)
		return KErrNoMemory;
	test.Title();
	__UHEAP_MARK;
	TRAPD(r,AutoTestL());
	__UHEAP_MARKEND;
	delete trap;
	return r;
	}
