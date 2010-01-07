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
// f32test\demandpaging\loader\t_pageldrtst.cpp
// Demand Paging Loader Stress Tests
// Demand Paging Loader stress tests attempt to cause as much paging as possible
// whilst putting the system various types of load.
// t_pageldrtst.exe is the root of the tests, it in turn will start copies of 
// itself stored in various types of media (t_pageldrtst_rom.exe for example).
// It also loads DLLs from various media, each DLL containing simple functions 
// that are aligned on page boundaries, so each function call is likely to 
// cause a page fault.
// Usage:
// t_pageldrtst and t_pageldrtst_rom
// Common command lines:
// t_pageldrtst - run the auto test suite
// t_pageldrtst lowmem - run the low memory tests
// t_pageldrtst chunks - run the chunk tests
// t_pageldrtst chunks+ - run the chunk tests (same as used in autotest)
// t_pageldrtst echunks - run the really stressful chunk tests
// t_pageldrtst auto debug - run the autotest but with debug output to the serial port
// t_pageldrtst d_exc - run the d_exc tests
// Arguments:
// single - run the tests in a single thread
// multiple <numThreads> - run the tests in multiple threads where <numThreads>
// auto - dummy param to trick the tests into running the auto test suite with extra params
// fullauto - param to make the tests perform the full automatic stress test
// interleave - force thread interleaving
// prio - each thread reschedules in between each function call, causes lots of context changes
// media - perform media access during the tests, very stressful
// mmc - only use the mmc media for media access to test file caching
// min - min cache size in pages
// max - max cache size in pages
// chunks - simple chunk stress tests
// chunks+ - the chunk auto tests
// echunks - extremem chunks tests
// nochunkdata - don't check the integrity of the data in the chunks
// lowmem - low memory tests
// dll - only load dll's
// exe - only start exe's (t_pagestress)
// self - only start copies of self (t_pageldrtst from various media)
// complete - dll, exe and self.
// rom - only load from ROM
// base - only load the base DLL and exe's (from code)
// mixed - rom and base.
// all_media - load dlls and exes from all media
// debug - switch on debugging information
// silent - no output to the screen or serial port
// noclean - don't delete copied files on exit
// d_exc - run the d_exc tests
// global - load dlls once globally
// thread - load dlls once per thread
// func - load dlls in the test function (default and most stressful)
// forward - patern in which to execute function calls 
// backward - patern in which to execute function calls 			
// random - patern in which to execute function calls 			
// all - patern in which to execute function calls (forward, backward and random)
// inst - for debugging a parameter passed to a spawned exe to give it an id.
// iters <count> - the number of times to loop (a '-' means run forever)
// reaper - test the reaper.
// btrace - test the btrace code.
// defrag - test the ram defrag code.
// stressfree - set the page cache to stress free size and run tests.
// t_pageldrtst causes a large ammount of paging by repeatedly calling 
// functions from multiple DLLs which include 64 functions which have 
// been aligned on page boundaries from multiple threads, whilst causing 
// background paging by spawning copies of itself and t_pagestress.
// The test also endeavours to stress the loader by loading and unloading
// DLLs from multiple threads from various types of media at the same 
// time as stressing the media, testing chunks, the reaper and changing
// thread priorities.
// 002 Load thrashing, test rapid loading and unloading of DLLs from 
// multiple threads (DEF100158)
// 003 Multiple threads loading DLLs in random pattern
// 004 Multiple threads loading EXE, SELF and DLLs in random pattern with
// all media, loaded in thread with prio change
// 005 Multiple threads loading EXE, SELF and DLLs in random pattern with
// all media, loaded globally with prio change
// 006 Multiple threads loading EXE, SELF and DLLs in random pattern with
// all media, loaded in func with process interleaving
// 007 Multiple threads loading EXE, SELF and DLLs in random pattern with
// all media, loaded in func with process interleaving, prio change
// and media access
// 008 Low Memory setup test
// 009 Low Memory, Multiple threads loading EXE, SELF and DLLs in random 
// pattern, loaded in func.
// 010 Low Memory setup test
// 011 Low Memory, Multiple threads loading EXE, SELF and DLLs in random 
// pattern, loaded in func with process interleaving, 
// prio change and media access
// 012 Close test driver
// 013 Chunk tests, Multiple threads loading EXE, SELF and DLLs in random 
// pattern with ROM / ROFS media, loaded in func with prio change 
// 014 Reaper tests with Multiple threads loading EXE, SELF and DLLs in random 
// pattern with all media 
// 015 Reaper tests with Multiple threads loading EXE, SELF and DLLs in random 
// pattern with all media, prio change and process interleaving
// 016 d_exc check test
// 
//

//! @SYMTestCaseID			KBASE-T_PAGELDRTST-0326
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1110
//! @SYMTestCaseDesc		Demand Paging Loader Stress Tests
//! @SYMTestActions			001 Demand Paging loader stress tests...
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented

#include <e32test.h>
#include <e32rom.h>
#include <e32svr.h>
#include <u32hal.h>
#include <f32file.h>
#include <f32dbg.h>
#include <e32msgqueue.h>
#include <e32math.h>
#include <e32btrace.h>
#include <d32btrace.h>
#include <d32locd.h>
#include <hal.h>

#include "t_hash.h"
#include "paging_info.h"

#ifndef TEST_AUTOTEST
#define TEST_RUN_REAPERTEST
#define TEST_RUN_LOWMEMTEST
#define TEST_RUN_DEFRAGTEST
#define TEST_RUN_D_EXCTEST
#define TEST_RUN_CHUNKTEST
#define TEST_RUN_AUTOTEST
RTest test(_L("T_PAGELDRTST"));
#else
#ifdef TEST_RUN_REAPERTEST
RTest test(_L("T_PAGELDRTST_REAPER"));
#endif
#ifdef TEST_RUN_LOWMEMTEST
RTest test(_L("T_PAGELDRTST_LOWMEM"));
#endif
#ifdef TEST_RUN_DEFRAGTEST
RTest test(_L("T_PAGELDRTST_DEFRAG"));
#endif
#ifdef TEST_RUN_D_EXCTEST
RTest test(_L("T_PAGELDRTST_D_EXC"));
#endif
#ifdef TEST_RUN_CHUNKTEST
RTest test(_L("T_PAGELDRTST_CHUNK"));
#endif
#ifdef TEST_RUN_AUTOTEST
RTest test(_L("T_PAGELDRTST_AUTO"));
#endif
#endif //TEST_AUTOTEST

const TInt KMessageBufSize = 80;
typedef TBuf<KMessageBufSize> TMessageBuf;

//#define TEST_SHORT_TEST
//#define TEST_THRASHING_TEST
//#define TEST_ADD_FAT_MEDIA
#define TEST_DONT_RESET_STATS
#define TEST_MINIMAL_STATS
//#define TEST_KERN_HEAP
#define TEST_ADD_FRAGD_MEDIA
#ifdef TEST_ADD_FRAGD_MEDIA
#endif

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
//#define WANT_FS_CACHE_STATS 
#endif

#ifdef __X86__
#define TEST_ON_UNPAGED
#define TEST_NO_DEXC_IN_AUTO
#endif


#include "t_pagestress.h"
#include "t_pageldrtstdll.h"

#include "t_ramstress.h"

TBool		TestDebug					= EFalse;
TBool		TestSilent					= EFalse;
TBool		TestExit					= EFalse;
#define TEST_EXE			0x01
#define TEST_DLL			0x02
#define TEST_SELF			0x04
#define TEST_EXE_SELF		(TEST_EXE | TEST_SELF)
#define TEST_EXE_SELF_DLL	(TEST_EXE | TEST_SELF | TEST_DLL)
TInt		TestLoading				    = TEST_EXE_SELF_DLL;

#define TEST_MEDIA_BASE			(1 << KTestMediaBase)
#define TEST_MEDIA_ROM			(1 << KTestMediaRom)
#define TEST_MEDIA_ROFS			(1 << KTestMediaRofs)
#define TEST_MEDIA_EXT			(1 << KTestMediaExt)
#define TEST_MEDIA_FAT			(1 << KTestMediaFat)
#define TEST_MEDIA_MMC			(1 << KTestMediaMmc)
#define TEST_MEDIA_ROM_BASE		(TEST_MEDIA_ROM | TEST_MEDIA_BASE)
#define TEST_MEDIA_ALL		(TEST_MEDIA_ROM | TEST_MEDIA_BASE | TEST_MEDIA_ROFS | TEST_MEDIA_EXT | TEST_MEDIA_MMC)

typedef enum
{
	KTestMediaBase = 0,
	KTestMediaRom,
	KTestMediaExt,
	KTestMediaRofs,
#ifdef TEST_ADD_FAT_MEDIA
	KTestMediaFat,  // this is the last one that is always present.
#endif
	KTestMediaMmc,
#ifdef TEST_ADD_FRAGD_MEDIA
	KTestMediaNandFrag,
	KTestMediaMmcFrag,
#endif
	KTestMediaCOUNT,
}ETestMediaType;
#ifdef TEST_ADD_FAT_MEDIA
#define TEST_MEDIA_COUNT_HACK   (KTestMediaFat + 1)
#else
#define TEST_MEDIA_COUNT_HACK   (KTestMediaRofs + 1)
#endif

typedef enum
{
	KTestMediaAccessNone = 0,
	KTestMediaAccessBasic,
	KTestMediaAccessMultipleThreads,
	KTestMediaAccessMultiplePattern,
	KTestMediaAccessMixed,
	KTestMediaAccessCOUNT,
}ETestMediaAccess;

TInt		TestWhichMedia			    = TEST_MEDIA_ROM_BASE;
TInt		DriveNumber=-1;   // Parameter - Which drive?  -1 = autodetect.
TBool		TestSingle					= EFalse;
TBool		TestMultiple				= EFalse;
TInt		TestMaxLoops				= 20;
#define TEST_2MEDIA_THREADS		20
#define TEST_ALLMEDIA_THREADS	20
TInt		TestMultipleThreadCount		= TEST_2MEDIA_THREADS;
TInt		TestInstanceId				= 0;
TBool		TestWeAreTheTestBase        = EFalse;
TBool		TestBootedFromMmc			= EFalse;
TBool		TestOnlyFromMmc				= EFalse;
TBool		TestD_Exc					= EFalse;
TBool		TestNoClean					= EFalse;
TBool		TestFullAutoTest			= EFalse;
#define TEST_DLL_GLOBAL		0x01
#define TEST_DLL_THREAD		0x02
#define TEST_DLL_FUNC		0x04
TInt		TestLoadDllHow				= TEST_DLL_FUNC;
TBool		TestIsAutomated				= EFalse;

#define TEST_INTERLEAVE_PRIO		EPriorityMore//EPriorityRealTime //23 // KNandThreadPriority - 1
TBool		TestInterleave				= EFalse;
TFileName	TestNameBuffer;
TBool		TestPrioChange				= EFalse;

volatile TBool		TestStopMedia				= EFalse;
ETestMediaAccess TestMediaAccess        = KTestMediaAccessNone;
#define TEST_NUM_FILES		5

RSemaphore	TestMultiSem;
RMsgQueue<TMessageBuf> TestMsgQueue;

#define TEST_LM_NUM_FREE	0
#define TEST_LM_BLOCKSIZE	1
#define TEST_LM_BLOCKS_FREE	4
TBool		TestLowMem					= EFalse;
TBool		TestingLowMem				= EFalse;
RPageStressTestLdd PagestressLdd;
RRamStressTestLdd  RamstressLdd;

TBool		TestBtrace					= EFalse;
TBool		TestDefrag					= EFalse;
TBool		TestChunks					= EFalse;
TBool		TestChunksPlus				= EFalse;
TBool		TestExtremeChunks			= EFalse;
TBool		TestChunkData				= ETrue;
TBool		TestingChunks				= EFalse;
volatile TBool		TestDefragTestEnd			= EFalse;
TBool		TestingDefrag				= EFalse;
volatile TBool		TestThreadsExit				= EFalse;
TInt		TestPageSize				= 4096;
RChunk		TestChunk;
TInt		TestCommitEnd = 0;
TUint8*		TestChunkBase = NULL;
#define TEST_NUM_PAGES			64
#define TEST_NUM_CHUNK_PAGES	(TEST_NUM_PAGES * 2)
TBool		TestChunkPageState[TEST_NUM_CHUNK_PAGES];

TBool		TestReaper					= EFalse;
TBool		TestingReaper				= EFalse;
TBool		TestingReaperCleaningFiles  = EFalse;
#define TEST_REAPER_ITERS			20
#define TEST_DOT_PERIOD				30
TBool		TestStressFree				= EFalse;
TInt		TestMinCacheSize = 64 * 4096;
TInt		TestMaxCacheSize = 128 * 4096;
TBool		TestIsDemandPaged = ETrue;
#define TEST_MAX_ZONE_THREADS		8
TUint		TestZoneCount = 0;
TInt TickPeriod = 15625;

#define TEST_NONE		0x0
#define TEST_THRASH		0x1
#define TEST_FORWARD	0x2
#define TEST_BACKWARD	0x4
#define TEST_RANDOM		0x8
#define TEST_ALL		(TEST_RANDOM | TEST_BACKWARD | TEST_FORWARD)
TUint32	TestWhichTests				= TEST_ALL;
_LIT(KRomPath, "z:\\sys\\bin\\");
_LIT(KMmcDefaultPath, "d:\\sys\\bin\\");

#define EXISTS(__val) ((__val == KErrNone) ? &KFileExists : &KFileMissing)
_LIT(KSysHash,"?:\\Sys\\Hash\\");
_LIT(KTestBlank, "");
_LIT(KFileExists, "Exists");
_LIT(KFileMissing, "Missing");
_LIT(KMultipleTest, "Multiple");
_LIT(KSingleTest,   "Single  ");
_LIT(KTestExe, "Exe ");
_LIT(KTestDll, "Dll ");
_LIT(KTestSelf, "Self ");
_LIT(KTestBase, "Base ");
_LIT(KTestRom, "ROM ");
_LIT(KTestAll, "All ");
_LIT(KTestGlobal, "Global");
_LIT(KTestThread, "Thread");
_LIT(KTestFunc,  "Func");
_LIT(KTestInter, "Interleave ");
_LIT(KTestPrio, "Prio ");
_LIT(KTestMedia, "Media ");
_LIT(KTestLowMem, "LowMem ");
_LIT(KTestChunking, "Chunks ");
_LIT(KTestEChunking, "EChunks ");
_LIT(KTestChunkingPlus, "Chunks+ ");
_LIT(KTestReaper, "Reaper ");
_LIT(KTestThrash, "Thrash ");
_LIT(KTestForward, "Forward ");
_LIT(KTestBackward, "Backward ");
_LIT(KTestRandom, "Random ");

typedef struct 
	{
	TBool				testFullAutoOnly;
	TInt				testLoading;
	TInt				testWhichMedia;
	TBool				testMultiple;
	TInt				testMaxLoops;
	TInt				testMultipleThreadCount;
	TBool				testLoadDllHow;
	TBool				testInterleave;
	TBool				testPrioChange;
	ETestMediaAccess	testMediaAccess;
	TUint32				testWhichTests;
	TBool				testLowMem;
	TInt				testFreeRam;
	}TTheTests; 

typedef struct
	{
	TInt	ok;
	TInt	fail;
	}TChunkTestPair;

typedef struct
	{
	TChunkTestPair	lock;
	TChunkTestPair	unlock;
	TChunkTestPair	decommit;
	TChunkTestPair	commit;
	TChunkTestPair	check;
	}
TChunkTestStats;

TChunkTestStats	TestChunkStats[TEST_NUM_CHUNK_PAGES];


TPtrC TestPsExeNames[KTestMediaCOUNT] = {	_L("t_pagestress.exe"), 
											_L("t_pagestress_rom.exe"), 
											_L("t_pagestress_ext.exe"), 
											_L("t_pagestress_rofs.exe"), 
#ifdef TEST_ADD_FAT_MEDIA
											_L("t_pagestress_fat.exe"),
#endif
											_L("t_pagestress_mmc.exe"),
#ifdef TEST_ADD_FRAGD_MEDIA
											_L("t_pagestress_nfr.exe"),
											_L("t_pagestress_mfr.exe"),
#endif
											};

TPtrC TestPlExeNames[KTestMediaCOUNT] = {	_L("t_pageldrtst.exe"), 
											_L("t_pageldrtst_rom.exe"), 
											_L("t_pageldrtst_ext.exe"), 
											_L("t_pageldrtst_rofs.exe"), 
#ifdef TEST_ADD_FAT_MEDIA
											_L("t_pageldrtst_fat.exe"),
#endif
											_L("t_pageldrtst_mmc.exe"),
#ifdef TEST_ADD_FRAGD_MEDIA
											_L("t_pageldrtst_nfr.exe"),
											_L("t_pageldrtst_mfr.exe"),
#endif
											};

_LIT(KDllBaseName,   "t_pageldrtst");

TPtrC TestPlExtNames[KTestMediaCOUNT] = {	_L(".dll"),
											_L("_rom.dll"),
											_L("_ext.dll"),
											_L("_rofs.dll"),
#ifdef TEST_ADD_FAT_MEDIA
											_L("_fat.dll"),
#endif
											_L("_mmc.dll"),
#ifdef TEST_ADD_FRAGD_MEDIA
											_L("_nfr.dll"),
											_L("_mfr.dll"),
#endif
											};


TBool TestDllExesExist[KTestMediaCOUNT] = { EFalse, 
											EFalse,
											EFalse,
											EFalse,
#ifdef TEST_ADD_FAT_MEDIA
											EFalse,
#endif
											EFalse,
#ifdef TEST_ADD_FRAGD_MEDIA
											EFalse,
											EFalse,
#endif
											};
#define DBGS_PRINT(__args)\
	if (!TestSilent) test.Printf __args;

#define DBGD_PRINT(__args)\
	if (TestDebug) test.Printf __args;

void SendDebugMessage(RMsgQueue<TMessageBuf> *aMsgQueue = NULL, 
					  TMessageBuf            *aBuffer = NULL,
					  RSemaphore	 		 *aTheSem = NULL)
	{
	for (;;)
		{
		aTheSem->Wait();
		TInt r = aMsgQueue->Send(*aBuffer);
		aTheSem->Signal();
		if (r != KErrOverflow)
			return;
		User::After(0);
		}
	}

#define DEBUG_PRINT(__args)\
if (!TestSilent) \
	{\
	if (aMsgQueue && aBuffer && aTheSem)\
		{\
		aBuffer->Zero();\
		aBuffer->Format __args ;\
		SendDebugMessage(aMsgQueue, aBuffer, aTheSem);\
		}\
	else\
		{\
		test.Printf __args ;\
		}\
	}

#define RUNTEST(__test, __error)\
	if (!TestSilent)\
		test(__test == __error);\
	else\
		__test;

#define RUNTEST1(__test)\
	if (!TestSilent)\
		test(__test);


#define DEBUG_PRINT1(__args)\
if (TestDebug)\
	{\
	DEBUG_PRINT(__args)\
	}

#define DOTEST(__operation, __condition)\
	if (aLowMem) \
		{\
		__operation;\
		while (!__condition)\
			{\
			DBGD_PRINT((_L("Releasing some memory on line %d\n"), __LINE__));\
			if (pTheSem)\
				pTheSem->Wait();\
			PagestressLdd.DoReleaseSomeRam(TEST_LM_BLOCKS_FREE);\
			if (pTheSem)\
				pTheSem->Signal();\
			__operation;\
			}\
		RUNTEST1(__condition);\
		}\
	else\
		{\
		__operation;\
		RUNTEST1(__condition);\
		}

#define DOTEST1(__var, __func, __ok, __fail)\
	if (aLowMem) \
		{\
		__var = __func;\
		while (__var == __fail)\
			{\
			DBGD_PRINT((_L("Releasing some memory on line %d\n"), __LINE__));\
			if (pTheSem)\
				pTheSem->Wait();\
			PagestressLdd.DoReleaseSomeRam(TEST_LM_BLOCKS_FREE);\
			if (pTheSem)\
				pTheSem->Signal();\
			__var = __func;\
			}\
		if (__var != __ok)\
			DBGS_PRINT((_L("Failing on line %d with error %d\n"), __LINE__, __var));\
		RUNTEST1(__var == __ok);\
		}\
	else\
		{\
		__var = __func;\
		RUNTEST1(__var == __ok);\
		}

#define DOLOADALLOC(__numDlls, __pTheLibs, __theSem)\
	if (TestingLowMem)\
		{\
		__pTheLibs = (PageLdrRLibrary *)User::AllocZ(sizeof(PageLdrRLibrary) * __numDlls);\
		while (__pTheLibs == NULL)\
			{\
			DEBUG_PRINT1((_L("Releasing some memory for alloc on line %d\n"), __LINE__));\
			if (__theSem)\
				__theSem->Wait();\
			PagestressLdd.DoReleaseSomeRam(TEST_LM_BLOCKS_FREE);\
			if (__theSem)\
				__theSem->Signal();\
			__pTheLibs = (PageLdrRLibrary *)User::AllocZ(sizeof(PageLdrRLibrary) * __numDlls);\
			}\
		}\
	else\
		{\
		__pTheLibs = (PageLdrRLibrary *)User::AllocZ(sizeof(PageLdrRLibrary) * __numDlls);\
		if (__pTheLibs == NULL)\
			return KErrGeneral;\
		}

#define TEST_NEXT(__args) \
	if (!TestSilent)\
		test.Next __args;

void DoStats();
void CheckFilePresence(TBool aDoFileCopy);
void CleanupFiles(TBool silent);
typedef TInt (*TCallFunction)(TUint32 funcIndex, TInt param1, TInt param2);

class PageLdrRLibrary : public RLibrary
	{
public:
	TInt TestLoadLibrary(const TDesC& aFileName, TInt aThreadIndex, RMsgQueue<TMessageBuf> *aMsgQueue, TMessageBuf *aBuffer, RSemaphore  *aTheSem);
	TInt CloseLibrary();
	
public:	
	TBool				iInUse;
	TUint32				iFuncCount;
	TLibraryFunction	iInitFunc;
	TLibraryFunction	iFunctionCountFunc;
	TCallFunction       iCallFunctionFunc;
	TLibraryFunction	iSetCloseFunc;
	};

TInt PageLdrRLibrary::CloseLibrary()
	{
	if (iInUse)
		{
		if (iSetCloseFunc)
			(iSetCloseFunc)();
		Close();
		iFuncCount = 0;
		iInitFunc = NULL;
		iFunctionCountFunc = NULL;
		iCallFunctionFunc = NULL;
		iSetCloseFunc = NULL;
		iInUse = EFalse;
		}
	return KErrNone;
	}

PageLdrRLibrary		theGlobalLibs[PAGELDRTST_MAX_DLLS * KTestMediaCOUNT];

////////////////////////////////////////////////////////////
// Template functions encapsulating ControlIo magic
//
GLDEF_D template <class C>
GLDEF_C TInt controlIo(RFs &fs, TInt drv, TInt fkn, C &c)
{
    TPtr8 ptrC((TUint8 *)&c, sizeof(C), sizeof(C));

    TInt r = fs.ControlIo(drv, fkn, ptrC);

    return r;
}

//
// FreeRam
//
// Get available free ram.
//

TInt FreeRam()
	{
	// wait for any async cleanup in the supervisor to finish first...
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);

	TMemoryInfoV1Buf meminfo;
	TInt r=UserHal::MemoryInfo(meminfo);
	test (r==KErrNone);
	return meminfo().iFreeRamInBytes;
	}

//
// FindFsNANDDrive
//
// Find the NAND drive
//

static TInt FindFsNANDDrive(RFs& aFs)
	{
	TDriveList driveList;
	TDriveInfo driveInfo;
	TInt r=aFs.DriveList(driveList);
    if (r == KErrNone)
		{
		for (TInt drvNum= (DriveNumber<0)?0:DriveNumber; drvNum<KMaxDrives; ++drvNum)
			{
			if(!driveList[drvNum])
				continue;   //-- skip unexisting drive

			if (aFs.Drive(driveInfo, drvNum) == KErrNone)
				{
				if(driveInfo.iMediaAtt&KMediaAttPageable)
					{
					TBool readOnly = driveInfo.iMediaAtt & KMediaAttWriteProtected;		// skip ROFS partitions
					if(!readOnly && (driveInfo.iType != EMediaHardDisk))
						{
						if ((drvNum==DriveNumber) || (DriveNumber<0))		// only test if running on this drive
							{
							return (drvNum);
							}
						}
					}
				}
			}
		}
	return (-1);
	}

//
// FindMMCDriveNumber
// 
// Find the first read write drive.
//

TInt FindMMCDriveNumber(RFs& aFs)
	{
	TDriveInfo driveInfo;
	for (TInt drvNum=0; drvNum<KMaxDrives; ++drvNum)
		{
		TInt r = aFs.Drive(driveInfo, drvNum);
		if (r >= 0)
			{
			if (driveInfo.iType == EMediaHardDisk)
				return (drvNum);
			}
		}
	return -1;
	}


//
// PageLdrRLibrary::TestLoadLibrary
//
// Load a library and initialise information about that library
//

TInt PageLdrRLibrary::TestLoadLibrary(const TDesC&           aFileName,
									  TInt					 aThreadIndex,
									  RMsgQueue<TMessageBuf> *aMsgQueue = NULL, 
									  TMessageBuf           *aBuffer = NULL,
									  RSemaphore			*aTheSem = NULL)
	{
	TInt retVal = KErrNone;
	if (TestingLowMem)
		{
		TBool whinged = EFalse;
		TInt initialFreeRam = 0;
		TInt freeRam = 0;

		while (1)
			{
			initialFreeRam = FreeRam();
			retVal = Load(aFileName);
			freeRam = FreeRam();
			if (retVal == KErrNoMemory)
				{
				if (!whinged && (freeRam > (4 * TestPageSize)))
					{
					whinged = ETrue;
					DEBUG_PRINT1((_L("Load() %d pages %S\n"), (freeRam / TestPageSize), &aFileName));
					if (TestIsDemandPaged)
						{
						SVMCacheInfo  tempPages;
						UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0);

						DEBUG_PRINT1((_L("DPC : min %d max %d curr %d\n"), 
									tempPages.iMinSize, tempPages.iMaxSize, tempPages.iCurrentSize));
						DEBUG_PRINT1((_L("    : maxFree %d freeRam %d\n"),
									tempPages.iMaxFreeSize, FreeRam()));
						}
					}
				DEBUG_PRINT1((_L("Load() releasing some memory for %S (%d)\n"), &aFileName, retVal));
				if (aTheSem)
					aTheSem->Wait();
				PagestressLdd.DoReleaseSomeRam(TEST_LM_BLOCKS_FREE);
				if (aTheSem)
					aTheSem->Signal();
				}
			else
				{
				if (whinged)
					{
					DEBUG_PRINT((_L("Load() Ok %d pages (%d) %S\n"), ((initialFreeRam - freeRam) / TestPageSize), (freeRam / TestPageSize), &aFileName));
					}
				break;
				}
			}
		}
	else
		{
		DEBUG_PRINT1((_L("Loading %S (%d)\n"), &aFileName, aThreadIndex));	 
		retVal = Load(aFileName);
		if (retVal != KErrNone)
			{
			DEBUG_PRINT1((_L("Load failed %S (%d)\n"), &aFileName, aThreadIndex));	 
			if (TestingReaper )
				{
				TInt tempIndex = 0;
				TBool whinged = EFalse;
				while (    (   (retVal == KErrNotFound) 
							|| (retVal == KErrPermissionDenied) 
							|| (retVal == KErrCorrupt) 
							|| (retVal == KErrInUse)) 
						&& (    TestingReaperCleaningFiles
							|| (tempIndex < TEST_REAPER_ITERS)))
					{
					User::After(2000000);
					if (!whinged)
						{
						DEBUG_PRINT((_L("Load() retrying load for %S (%d)\n"), &aFileName, retVal));
						whinged = ETrue;
						}
					retVal = Load(aFileName);
					if (!TestingReaperCleaningFiles)
						{
						tempIndex ++;
						}
					}
				if (retVal != KErrNone)
					{
					DEBUG_PRINT((_L("Load() failing for %S (%d) idx %d\n"), &aFileName, retVal, tempIndex));
					}
				}
			else if (TestingDefrag)
				{
				TInt tempIndex = 0;
				TBool whinged = EFalse;
				while ((retVal == KErrGeneral) && (tempIndex < 10))
					{
					User::After(20000);
					if (!whinged)
						{
						DEBUG_PRINT((_L("Load() retrying load for %S (%d)\n"), &aFileName, retVal));
						whinged = ETrue;
						}
					retVal = Load(aFileName);
					tempIndex ++;
					}
				if (retVal != KErrNone)
					{
					DEBUG_PRINT((_L("Load() failing for %S (%d) idx %d\n"), &aFileName, retVal, tempIndex));
					}
				}
			}
		}
	DEBUG_PRINT1((_L("Loaded %S (%d)\n"), &aFileName, aThreadIndex));	 
	if (retVal == KErrNone)
		{
		iInUse = ETrue;
		iInitFunc = Lookup(PAGELDRTST_FUNC_Init);
		iFunctionCountFunc = Lookup(PAGELDRTST_FUNC_FunctionCount);
		iCallFunctionFunc = (TCallFunction)Lookup(PAGELDRTST_FUNC_CallFunction);
		iSetCloseFunc = Lookup(PAGELDRTST_FUNC_SetClose);
		if (   (iInitFunc != NULL)
			&& (iFunctionCountFunc != NULL)
			&& (iCallFunctionFunc != NULL)
			&& (iSetCloseFunc != NULL))
			{
			retVal = (iInitFunc)();
			if (retVal == KErrNone)
				{
				iFuncCount = (iFunctionCountFunc)();
				if (iFuncCount != 0)
					{
					DEBUG_PRINT1((_L("Loaded ok %S (%d)\n"), &aFileName, aThreadIndex));	 
					return KErrNone;	
					}
				retVal = KErrGeneral;
				DEBUG_PRINT((_L("!!! bad count %S (%d)\n"), &aFileName, aThreadIndex));	 
				}
			else
				{
				DEBUG_PRINT((_L("!!! init failed %S (%d)\n"), &aFileName, aThreadIndex));	 
				retVal = KErrGeneral;
				}
			}
		else
			{
			DEBUG_PRINT((_L("!!! missing %S (%d)\n"), &aFileName, aThreadIndex));	 
			retVal = KErrGeneral;
			}
		}
	else
		{
		DEBUG_PRINT((_L("Load() failed %S %d\n"), &aFileName, retVal));
#ifdef WANT_FS_CACHE_STATS
		RFs			 fs;
		if (KErrNone != fs.Connect())
			{
			DEBUG_PRINT(_L("TestLoadLibrary : Can't connect to the FS\n"));
			}
		else
			{
			TFileCacheStats stats1;
			TInt drvNum = FindMMCDriveNumber(fs); 
			controlIo(fs,drvNum, KControlIoFileCacheStats, stats1);
		
			DEBUG_PRINT((_L("FSC: drv %d %c free %d used %d locked %d\n"),
						drvNum, 'a' + drvNum,
						stats1.iFreeCount,
						stats1.iUsedCount,
						stats1.iLockedSegmentCount));
			DEBUG_PRINT((_L("   : alloc %d lock %d closed %d\n"),
						stats1.iAllocatedSegmentCount,
						stats1.iFileCount,
						stats1.iFilesOnClosedQueue));
			fs.Close();
			}
#endif //WANT_FS_CACHE_STATS 

		if (TestIsDemandPaged)
			{
			SVMCacheInfo  tempPages;
			UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0);

			DEBUG_PRINT((_L("DPC : min %d max %d curr %d\n"), 
						tempPages.iMinSize, tempPages.iMaxSize, tempPages.iCurrentSize));
			DEBUG_PRINT((_L("    : maxFree %d freeRam %d\n"),
						tempPages.iMaxFreeSize, FreeRam()));
			}
		}
	return retVal;
	}

//
// GetNumDlls
//
// Work out how many Dlls we will play with
//
TInt GetNumDlls()
	{
	TInt maxDllIndex;

	switch (TestWhichMedia)
		{
		default:
		case TEST_MEDIA_BASE:	
		case TEST_MEDIA_ROM:
			maxDllIndex = PAGELDRTST_MAX_DLLS;
		break;

		case TEST_MEDIA_ROM_BASE:
			maxDllIndex = PAGELDRTST_MAX_DLLS * 2;
		break;

		case TEST_MEDIA_ALL:
			maxDllIndex = PAGELDRTST_MAX_DLLS * KTestMediaCOUNT;
		break;
		}
	return maxDllIndex;
	}

//
// LoadTheLibs
//
// Open DLLs for use in the tests.
//

TInt LoadTheLibs(PageLdrRLibrary       *aTheLibs,
                 TInt                   aLibCount,
				 TInt				    aThreadIndex, 
                 RMsgQueue<TMessageBuf> *aMsgQueue = NULL, 
				 TMessageBuf           *aBuffer = NULL, 
				 RSemaphore			   *aTheSem = NULL)
	{
	TBuf<128>			nameBuffer;
	TInt				dllIndex = 0;
	TInt				realDllIndex = 0;
	TInt				dllOffset = -1;
	TInt				testWhich;
	RThread				thisThread;

	memset(aTheLibs, 0, sizeof(*aTheLibs) * aLibCount);
	for (dllIndex = 0; dllIndex < aLibCount; dllIndex ++)
		{
		realDllIndex = (dllIndex + aThreadIndex) % PAGELDRTST_MAX_DLLS;
//		realDllIndex = (dllIndex) % PAGELDRTST_MAX_DLLS;
		if (realDllIndex == 0)
			dllOffset ++;

		if ((TestWhichMedia & TEST_MEDIA_ALL) == TEST_MEDIA_ALL)
			testWhich = (dllIndex + dllOffset) % KTestMediaCOUNT;
		else if ((TestWhichMedia & TEST_MEDIA_ALL) == TEST_MEDIA_ROM_BASE)
			testWhich = ((dllIndex + dllOffset) & 1) ? KTestMediaBase : KTestMediaRom;
		else if (TestWhichMedia & TEST_MEDIA_BASE )
			testWhich = KTestMediaBase;
		else
			testWhich = KTestMediaRom;
		
		if (!TestDllExesExist[testWhich])
			testWhich = KTestMediaBase;

		nameBuffer.Format(_L("%S%d%S"), &KDllBaseName, realDllIndex, &TestPlExtNames[testWhich]);
		
		DEBUG_PRINT1((_L("LoadTheLibs[%02d] - loading %S\n"), aThreadIndex, &nameBuffer));
		TInt theErr = aTheLibs[dllIndex].TestLoadLibrary(nameBuffer, aThreadIndex, aMsgQueue, aBuffer, aTheSem);
		if (theErr != KErrNone)
			{
			DEBUG_PRINT((_L("LoadTheLibs[%02d] - fail %S %d\n"), aThreadIndex, &nameBuffer, theErr));
			return KErrGeneral;
			}
		else
			{
			DEBUG_PRINT1((_L("LoadTheLibs[%02d] - loaded %S OK\n"), aThreadIndex, &nameBuffer));
			}
		if (TestThreadsExit)
			{
			DEBUG_PRINT((_L("LoadTheLibs[%02d] - cancelled\n"), aThreadIndex));
			return KErrCancel;
			}
		if (TestPrioChange)
			{
			TThreadPriority originalThreadPriority = thisThread.Priority();
			DEBUG_PRINT1((_L("LoadTheLibs[%02d] before priority change\n"), aThreadIndex));
			thisThread.SetPriority(EPriorityLess);
			User::AfterHighRes(0);
			thisThread.SetPriority(originalThreadPriority);
			DEBUG_PRINT1((_L("LoadTheLibs[%02d] after priority change\n"), aThreadIndex));
			}
		}
	DEBUG_PRINT((_L("LoadTheLibs[%02d] done\n"), aThreadIndex));
	return KErrNone;
	}

//
// CloseTheLibs
//
// Close the DLLs that we have previously opened
//

void CloseTheLibs (PageLdrRLibrary       *aTheLibs,
                   TInt                   aLibCount)
	{
	TInt				dllIndex = 0;
	
	for (dllIndex = 0; dllIndex < aLibCount; dllIndex ++)
		{
		aTheLibs[dllIndex].CloseLibrary();
		}
	memset(aTheLibs, 0, sizeof(*aTheLibs) * aLibCount);
	}

//
// RunThreadForward
//
// Walk through the function pointer array (forwards) calling each function
//

TInt RunThreadForward(TInt				     aThreadIndex, 
					  PageLdrRLibrary		*aTheLibs,
					  TInt					 aMaxDllIndex,
					  RMsgQueue<TMessageBuf> *aMsgQueue = NULL, 
					  TMessageBuf			*aBuffer = NULL, 
					  RSemaphore			*aTheSem = NULL)
	{
	TInt				seed = 1;
	TUint32				index = 0;
	RThread				thisThread;
	PageLdrRLibrary    *pTheLibs = NULL;
	TInt				dllIndex = 0;

	if (TestLoadDllHow == TEST_DLL_FUNC)
		{
		DOLOADALLOC(aMaxDllIndex, pTheLibs, aTheSem);
		if (pTheLibs)
			{
			TInt retVal = LoadTheLibs(pTheLibs, aMaxDllIndex, aThreadIndex, aMsgQueue, aBuffer, aTheSem);
			if (retVal != KErrNone)
				{
				DEBUG_PRINT((_L("Forward[%d] - load fail\n"), aThreadIndex));
				CloseTheLibs (pTheLibs, aMaxDllIndex);
				User::Free(pTheLibs);
				return retVal;
				}
			}
		else
			{
			DEBUG_PRINT((_L("Forward[%d] - alloc fail\n"), aThreadIndex));
			return KErrGeneral;
			}
		}
	else
		{
		pTheLibs = aTheLibs;
		}
	
	for (dllIndex = 0; dllIndex < aMaxDllIndex; dllIndex ++)
		{
		index = 0;
		while (index < pTheLibs[dllIndex].iFuncCount)
			{
			if (TestPrioChange)
				{
				TThreadPriority originalThreadPriority = thisThread.Priority();
				thisThread.SetPriority(EPriorityLess);
				User::AfterHighRes(0);
				thisThread.SetPriority(originalThreadPriority);
				}
			if (pTheLibs[dllIndex].iCallFunctionFunc)
				seed = pTheLibs[dllIndex].iCallFunctionFunc(index, seed, index);
			else
				DEBUG_PRINT((_L("Forward[%d] : dll %d was NULL\n"), aThreadIndex, dllIndex));
			index ++;
			if (TestThreadsExit)
				break;
			}
		if (TestThreadsExit)
			break;
		}
	if (TestLoadDllHow == TEST_DLL_FUNC)
		{
		CloseTheLibs(pTheLibs, aMaxDllIndex);
		User::Free(pTheLibs);
		}
	return KErrNone;
	}

//
// RunThreadBackward
//
// Walk through the function pointer array (backwards) calling each function
//

TInt RunThreadBackward(TInt				      aThreadIndex, 
					   PageLdrRLibrary		 *aTheLibs,
					   TInt					  aMaxDllIndex,
					   RMsgQueue<TMessageBuf> *aMsgQueue = NULL, 
					   TMessageBuf			 *aBuffer = NULL,
					   RSemaphore			 *aTheSem = NULL)
	{
	TInt				seed = 1;
	TUint32				index = 0;
	RThread				thisThread;
	PageLdrRLibrary    *pTheLibs = NULL;
	TInt				dllIndex = 0;

	if (TestLoadDllHow == TEST_DLL_FUNC)
		{
		DOLOADALLOC(aMaxDllIndex, pTheLibs, aTheSem);
		if (pTheLibs)
			{
			TInt retVal = LoadTheLibs(pTheLibs, aMaxDllIndex, aThreadIndex, aMsgQueue, aBuffer, aTheSem);
			if (retVal != KErrNone)
				{
				DEBUG_PRINT((_L("Backward[%d] - load fail\n"), aThreadIndex));
				CloseTheLibs (pTheLibs, aMaxDllIndex);
				User::Free(pTheLibs);
				return retVal;
				}
			}
		else
			{
			DEBUG_PRINT((_L("Backward[%d] - alloc fail\n"), aThreadIndex));
			return KErrGeneral;
			}
		}	
	else
		{
		pTheLibs = aTheLibs;
		}

	for (dllIndex = aMaxDllIndex - 1; dllIndex >= 0; dllIndex --)
		{
		index = pTheLibs[dllIndex].iFuncCount;
		while (index > 0)
			{
			if (TestPrioChange)
				{
				TThreadPriority originalThreadPriority = thisThread.Priority();
				thisThread.SetPriority(EPriorityLess);
				User::AfterHighRes(0);
				thisThread.SetPriority(originalThreadPriority);
				}
			if (pTheLibs[dllIndex].iCallFunctionFunc)
				seed = pTheLibs[dllIndex].iCallFunctionFunc(index, seed, index);
			else
				DEBUG_PRINT((_L("Backward[%d] : dll %d was NULL\n"), aThreadIndex, dllIndex));
			index --;
			if (TestThreadsExit)
				break;
			}
		if (TestThreadsExit)
			break;
		}
	if (TestLoadDllHow == TEST_DLL_FUNC)
		{
		CloseTheLibs(pTheLibs, aMaxDllIndex);
		User::Free(pTheLibs);
		}
	return KErrNone;
	}

//
// RunThreadRandom
//
// Walk through the function pointer array in a random order a number of times calling each function
//

TInt RunThreadRandom(TInt				    aThreadIndex, 
					 PageLdrRLibrary	   *aTheLibs,
					 TInt				    aMaxDllIndex,
					 RMsgQueue<TMessageBuf> *aMsgQueue = NULL, 
					 TMessageBuf		   *aBuffer = NULL,
					 RSemaphore			   *aTheSem = NULL)
	{
	TInt				seed = 1;
	TUint				randNum;
	RThread				thisThread;
	PageLdrRLibrary    *pTheLibs = NULL;
	TUint				dllIndex = 0;
	
	if (TestLoadDllHow == TEST_DLL_FUNC)
		{
		DOLOADALLOC(aMaxDllIndex, pTheLibs, aTheSem);
		if (pTheLibs)
			{
			TInt retVal = LoadTheLibs(pTheLibs, aMaxDllIndex, aThreadIndex, aMsgQueue, aBuffer, aTheSem);
			if (retVal != KErrNone)
				{
				DEBUG_PRINT((_L("Random[%d] - load fail\n"), aThreadIndex));
				CloseTheLibs (pTheLibs, aMaxDllIndex);
				User::Free(pTheLibs);
				return retVal;
				}
			}
		else
			{
			DEBUG_PRINT((_L("Random[%d] - alloc fail\n"), aThreadIndex));
			return KErrGeneral;
			}
		}
	else
		{
		pTheLibs = aTheLibs;
		}

	
	TUint funcCount = (TUint)pTheLibs[0].iFuncCount;
	TInt iterCount = aMaxDllIndex * funcCount;
	
	// reduce the time for auto tests by reducing the number of cycles.
	if (TestIsAutomated)
		iterCount /= 4;

	while (iterCount > 0)
		{
		if (TestPrioChange)
			{
			TThreadPriority originalThreadPriority = thisThread.Priority();
			thisThread.SetPriority(EPriorityLess);
			User::AfterHighRes(0);
			thisThread.SetPriority(originalThreadPriority);
			}
		
		randNum = (TUint)Math::Random();
		dllIndex = randNum % (TUint)aMaxDllIndex;

		randNum %= funcCount;

		if (   (randNum < funcCount)
		    && ((TInt)dllIndex < aMaxDllIndex))
			{
			if (pTheLibs[dllIndex].iCallFunctionFunc)
				{
				seed = pTheLibs[dllIndex].iCallFunctionFunc(randNum, seed, randNum);
				}
			else
				DEBUG_PRINT((_L("Random[%d] : dll %d was NULL\n"), aThreadIndex, dllIndex));
			}
		else
			{
			DEBUG_PRINT((_L("Random[%d] : %d ERROR dllIndex %u rand %u\n"), aThreadIndex, iterCount, dllIndex, randNum));
			}
		
		--iterCount;
		if (TestThreadsExit)
			break;
		}

	if (TestLoadDllHow == TEST_DLL_FUNC)
		{
		CloseTheLibs(pTheLibs, aMaxDllIndex);
		User::Free(pTheLibs);
		}
	return KErrNone;
	}


//
// ThrashThreadLoad
//
// Load and unload the DLLs rapidly to show up a timing window in the kernel.
//

TInt ThrashThreadLoad (TInt				      aThreadIndex, 
					   PageLdrRLibrary		 *aTheLibs,
					   TInt					  aMaxDllIndex,
					   RMsgQueue<TMessageBuf> *aMsgQueue = NULL, 
					   TMessageBuf			 *aBuffer = NULL,
					   RSemaphore			 *aTheSem = NULL)
	{
	if (TestLoadDllHow == TEST_DLL_FUNC)
		{
		PageLdrRLibrary    *pTheLibs = NULL;
		DOLOADALLOC(aMaxDllIndex, pTheLibs, aTheSem);
		if (pTheLibs)
			{
			TInt retVal = LoadTheLibs(pTheLibs, aMaxDllIndex, aThreadIndex, aMsgQueue, aBuffer, aTheSem);
			if (retVal != KErrNone)
				{
				DEBUG_PRINT((_L("Thrash[%d] - load fail\n"), aThreadIndex));
				CloseTheLibs (pTheLibs, aMaxDllIndex);
				User::Free(pTheLibs);
				return retVal;
				}
			}
		else
			{
			DEBUG_PRINT((_L("Thrash[%d] - alloc fail\n"), aThreadIndex));
			return KErrGeneral;
			}

		CloseTheLibs(pTheLibs, aMaxDllIndex);
		User::Free(pTheLibs);
		}
	return KErrNone;
	}


//
// PerformTestThread
//
// This is the function that actually does the work.
// It is complicated a little because test.Printf can only be called from the first thread that calls it 
// so if we are using multiple threads we need to use a message queue to pass the debug info from the
// child threads back to the parent for the parent to then call printf.
//
//

LOCAL_C TInt PerformTestThread(TInt					  aThreadIndex, 
							   RMsgQueue<TMessageBuf> *aMsgQueue = NULL, 
							   TMessageBuf			 *aBuffer = NULL,
							   RSemaphore			 *aTheSem = NULL)
	{
	TUint start = User::TickCount();

	TFullName n(RThread().Name());

	DEBUG_PRINT((_L("%S : thread %d Executing %S\n"), &TestNameBuffer, aThreadIndex, &n));
	
	// now select how we do the test...
	TInt	iterIndex;

	PageLdrRLibrary    *pTheLibs = theGlobalLibs;
	TInt				maxDllIndex = GetNumDlls();

	switch (TestLoadDllHow)
		{
		case TEST_DLL_THREAD:
			pTheLibs = NULL;
			DOLOADALLOC(maxDllIndex, pTheLibs, aTheSem);
			if (pTheLibs)
				{
				TInt retVal = LoadTheLibs(pTheLibs, maxDllIndex, aThreadIndex, aMsgQueue, aBuffer, aTheSem);
				if (retVal != KErrNone)
					{
					DEBUG_PRINT((_L("Perform[%d] - load fail\n"), aThreadIndex));
					CloseTheLibs (pTheLibs, maxDllIndex);
					User::Free(pTheLibs);
					return retVal;
					}
				}
			else
				{
				DEBUG_PRINT((_L("Perform[%d] - alloc fail\n"), aThreadIndex));
				return KErrGeneral;
				}
		break;

		case TEST_DLL_GLOBAL:
			pTheLibs = theGlobalLibs;
		break;

		case TEST_DLL_FUNC:
		default:
		// do nowt
		break;
		}

	TInt    retVal = KErrNone;
	if (TEST_ALL == (TestWhichTests & TEST_ALL))
		{
		#define LOCAL_ORDER_INDEX1	6
		#define LOCAL_ORDER_INDEX2	3
		TInt	order[LOCAL_ORDER_INDEX1][LOCAL_ORDER_INDEX2] = {	{TEST_FORWARD, TEST_BACKWARD,TEST_RANDOM},
																	{TEST_FORWARD, TEST_RANDOM,  TEST_BACKWARD},
																	{TEST_BACKWARD,TEST_FORWARD, TEST_RANDOM},
																	{TEST_BACKWARD,TEST_RANDOM,  TEST_FORWARD},
																	{TEST_RANDOM,  TEST_FORWARD, TEST_BACKWARD},
																	{TEST_RANDOM,  TEST_BACKWARD,TEST_FORWARD}};
		TInt	whichOrder = 0;

		for (iterIndex = 0; ; )
			{
			TInt    selOrder = ((aThreadIndex + 1) * (iterIndex + 1)) % LOCAL_ORDER_INDEX1;
			for (whichOrder = 0; whichOrder < LOCAL_ORDER_INDEX2; whichOrder ++)
				{
				switch (order[selOrder][whichOrder])
					{
						case TEST_FORWARD:
						DEBUG_PRINT((_L("%S : %d Iter %d.%d Forward\n"),
							&TestNameBuffer, aThreadIndex, iterIndex, whichOrder));
						retVal = RunThreadForward(aThreadIndex, pTheLibs, maxDllIndex, aMsgQueue, aBuffer, aTheSem);
						break;

						case TEST_BACKWARD:
						DEBUG_PRINT((_L("%S : %d Iter %d.%d Backward\n"),
							&TestNameBuffer, aThreadIndex, iterIndex, whichOrder));
						retVal = RunThreadBackward(aThreadIndex, pTheLibs, maxDllIndex, aMsgQueue, aBuffer, aTheSem);
						break;

						case TEST_RANDOM:
						DEBUG_PRINT((_L("%S : %d Iter %d.%d Random\n"),
							&TestNameBuffer, aThreadIndex, iterIndex, whichOrder));
						retVal = RunThreadRandom(aThreadIndex, pTheLibs, maxDllIndex, aMsgQueue, aBuffer, aTheSem);
						break;
						
						default: // this is really an error.
						break;
					}
				DEBUG_PRINT((_L("%S : %d Iter %d.%d finished %d\n"),
					&TestNameBuffer, aThreadIndex, iterIndex, whichOrder, retVal));
				if ((retVal == KErrCancel) && iterIndex > 0)
					retVal = KErrNone;
				if ((retVal != KErrNone) || TestThreadsExit)
					break;
				}
			if ((retVal != KErrNone) || TestThreadsExit)
				break;
			if (++iterIndex >= TestMaxLoops)
				break;
			User::AfterHighRes(TEST_DOT_PERIOD/3*1000000);
			}
		}
	else
		{
		if (TestWhichTests & TEST_FORWARD)
			{
			for (iterIndex = 0; ; )
				{
				DEBUG_PRINT((_L("%S : %d Iter %d Forward\n"), &TestNameBuffer, aThreadIndex, iterIndex));
				retVal = RunThreadForward(aThreadIndex, pTheLibs, maxDllIndex, aMsgQueue, aBuffer, aTheSem);
				DEBUG_PRINT((_L("%S : %d Iter %d finished %d\n"), &TestNameBuffer, aThreadIndex, iterIndex, retVal));
				if ((retVal == KErrCancel) && iterIndex > 0)
					retVal = KErrNone;
				if ((retVal != KErrNone) || TestThreadsExit)
					break;
				if (++iterIndex >= TestMaxLoops)
					break;
				User::AfterHighRes(TEST_DOT_PERIOD/3*1000000);
				}
			}
			
		if (TestWhichTests & TEST_BACKWARD)
			{
			for (iterIndex = 0; ; )
				{
				DEBUG_PRINT((_L("%S : %d Iter %d Backward\n"), &TestNameBuffer, aThreadIndex, iterIndex));
				retVal = RunThreadBackward(aThreadIndex, pTheLibs, maxDllIndex, aMsgQueue, aBuffer, aTheSem);
				DEBUG_PRINT((_L("%S : %d Iter %d finished %d\n"), &TestNameBuffer, aThreadIndex, iterIndex, retVal));
				if ((retVal == KErrCancel) && iterIndex > 0)
					retVal = KErrNone;
				if ((retVal != KErrNone) || TestThreadsExit)
					break;
				if (++iterIndex >= TestMaxLoops)
					break;
				User::AfterHighRes(TEST_DOT_PERIOD/3*1000000);
				}
			}

		if (TestWhichTests & TEST_RANDOM)
			{
			for (iterIndex = 0; ; )
				{
				DEBUG_PRINT((_L("%S : %d Iter %d Random\n"), &TestNameBuffer, aThreadIndex, iterIndex));
				retVal = RunThreadRandom(aThreadIndex, pTheLibs, maxDllIndex, aMsgQueue, aBuffer, aTheSem);
				DEBUG_PRINT((_L("%S : %d Iter %d finished %d\n"), &TestNameBuffer, aThreadIndex, iterIndex, retVal));
				if ((retVal == KErrCancel) && iterIndex > 0)
					retVal = KErrNone;
				if ((retVal != KErrNone) || TestThreadsExit)
					break;
				if (++iterIndex >= TestMaxLoops)
					break;
				User::AfterHighRes(TEST_DOT_PERIOD/3*1000000);
				}
			}
		
		if (TestWhichTests & TEST_THRASH)
			{
			for (iterIndex = 0; ; )
				{
				DEBUG_PRINT((_L("%S : %d Iter %d Thrash Load\n"), &TestNameBuffer, aThreadIndex, iterIndex));
				retVal = ThrashThreadLoad(aThreadIndex, pTheLibs, maxDllIndex, aMsgQueue, aBuffer, aTheSem);
				DEBUG_PRINT((_L("%S : %d Iter %d finished %d\n"), &TestNameBuffer, aThreadIndex, iterIndex, retVal));
				if ((retVal == KErrCancel) && iterIndex > 0)
					retVal = KErrNone;
				if ((retVal != KErrNone) || TestThreadsExit)
					break;
				if (++iterIndex >= TestMaxLoops)
					break;
				User::AfterHighRes(TEST_DOT_PERIOD/3*1000000);
				}
			}
		}

	if (TestLoadDllHow == TEST_DLL_THREAD)
		{
		CloseTheLibs(pTheLibs, maxDllIndex);
		User::Free(pTheLibs);
		}

	DEBUG_PRINT((_L("%S : thread %d Exit (tick %u)\n"), &TestNameBuffer, aThreadIndex, User::TickCount() - start));
	return retVal;
	}


//
// MultipleTestThread
//
// Thread function, one created for each thread in a multiple thread test.
//

LOCAL_C TInt MultipleTestThread(TAny* aUseTb)
	{
	TInt			ret;
	TMessageBuf		localBuffer;

	if (TestInterleave)	
		{
		RThread				thisThread;
		thisThread.SetPriority((TThreadPriority) TEST_INTERLEAVE_PRIO);
		}

	ret = PerformTestThread((TInt) aUseTb, &TestMsgQueue, &localBuffer, &TestMultiSem);
	if (!TestingChunks)
		{
		if (ret != KErrNone) 
			User::Panic(_L("LOAD"), KErrGeneral);
		}
	return KErrNone;
	}

//
// StartExe
//
// Start an executable.
//

TInt StartExe(RProcess& aTheProcesses, TRequestStatus* aPrStatus, TInt aIndex, TBool aLoadSelf, TBool aLowMem, RSemaphore *pTheSem = NULL)
	{
	TBuf<256>		buffer;
	TInt			testWhich = KTestMediaRom;
	//y_LIT(KTestDebug, "debug");
	_LIT(KTestSilent, "silent");

	if ((TestWhichMedia & TEST_MEDIA_ALL) == TEST_MEDIA_ALL)
		testWhich = aIndex % KTestMediaCOUNT;
	else if ((TestWhichMedia & TEST_MEDIA_ALL) == TEST_MEDIA_ROM_BASE)
		testWhich = (aIndex & 1) ? KTestMediaBase : KTestMediaRom;
	else if (TestWhichMedia & TEST_MEDIA_BASE )
		testWhich = KTestMediaBase;
	else
		testWhich = KTestMediaRom;

	if (!TestDllExesExist[testWhich])
		testWhich = KTestMediaBase;

	buffer.Zero();
	TInt ret;
	if (aLoadSelf)
		{
		buffer.Format(_L("single random dll %S iters %d inst %d"),
			/* TestDebug ? &KTestDebug : */ &KTestSilent, TestMaxLoops, aIndex);
		if (TestExtremeChunks)
			buffer.Append(_L(" echunks"));
		else if (TestChunksPlus)
			buffer.Append(_L(" chunks prio"));
		if (TestChunkData == EFalse)
			buffer.Append(_L(" nochunkdata"));
		DBGS_PRINT((_L("%S : Starting Process %d %S %S\n"),
			&TestNameBuffer, aIndex, &TestPlExeNames[testWhich], &buffer));
		DOTEST1(ret,aTheProcesses.Create(TestPlExeNames[testWhich],buffer),KErrNone, KErrNoMemory);
		}
	else
		{
		buffer.Format(_L("single random %S iters %d inst %d"),
			/* TestDebug ? &KTestDebug : */ &KTestSilent, TestMaxLoops, aIndex);
		DBGS_PRINT((_L("%S : Starting Process %d %S %S\n"),
			&TestNameBuffer, aIndex, &TestPsExeNames[testWhich], &buffer));
		DOTEST1(ret,aTheProcesses.Create(TestPsExeNames[testWhich],buffer),KErrNone, KErrNoMemory);
		}
	if (ret == KErrNone)
		{
		if(aPrStatus)
			{
			aTheProcesses.Logon(*aPrStatus);
			RUNTEST1(*aPrStatus == KRequestPending);	
			}
		aTheProcesses.Resume();
		}
	return ret;
	}

//
// PerformRomAndFileSystemAccessThread
// 
// Access the rom and dump it out to one of the writeable partitions...
// really just to make the media server a little busy during the test.
//
TInt PerformRomAndFileSystemAccessThread(TInt					aThreadId,
										 RMsgQueue<TMessageBuf> *aMsgQueue, 
										 TMessageBuf		   *aBuffer,
										 RSemaphore			   *aTheSem,
										 TBool					aLowMem)
	{
	RThread		 thisThread;
	TUint		 maxBytes = KMaxTUint;
	TInt		 startTime = User::TickCount();
	RSemaphore	*pTheSem = aTheSem;
	RFs fs;
	RFile file;

	if (KErrNone != fs.Connect())
		{
		DEBUG_PRINT(_L("PerformRomAndFileSystemAccessThread : Can't connect to the FS\n"));
		return KErrGeneral;
		}

	// get info about the ROM...
	TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
	TUint8* start;
	TUint8* end;
	if(romHeader->iPageableRomStart)
		{
		start = (TUint8*)romHeader + romHeader->iPageableRomStart;
		end = start + romHeader->iPageableRomSize;
		}
	else
		{
		start = (TUint8*)romHeader;
		end = start + romHeader->iUncompressedSize;
		}
	if (end <= start)
		return KErrGeneral;

	// read all ROM pages in a random order...and write out to file in ROFs
	TInt pageSize = 0;
	UserSvr::HalFunction(EHalGroupKernel,EKernelHalPageSizeInBytes,&pageSize,0);

	TUint size = end - start - pageSize;
	if(size > maxBytes)
		size = maxBytes;

	TUint32 random = 1 + aThreadId;
	TPtrC8  sourceData;
	TUint8* theAddr;
	HBufC8* checkData;

	DOTEST((checkData = HBufC8::New(pageSize + 10)),
	       (checkData != NULL));
	
	if (!checkData)
		{
		DEBUG_PRINT((_L("RomAndFSThread %S : failed to alloc read buffer\n"), &TestNameBuffer));
		}

	TInt		drvNum = (TestBootedFromMmc || TestOnlyFromMmc) ? FindMMCDriveNumber(fs) : FindFsNANDDrive(fs);
	TBuf<32>	filename;
	
	filename.Format(_L("?:\\Pageldrtst%d.tmp"), aThreadId);
	if (drvNum >= 0)
		{
		DEBUG_PRINT((_L("%S : Filename %S\n"), &TestNameBuffer, &filename));
		}
	else
		{
		DEBUG_PRINT((_L("RomAndFSThread : error getting drive num\n")));
		drvNum = 3; //make it 'd' by default.
		}
	filename[0] = 'a' + drvNum;

#ifdef WANT_FS_CACHE_STATS 
	TInt allocatedSegmentCount = 0;
	TInt filesOnClosedQueue = 0;
#endif
	TInt ret;
	while(1)
		{
		for(TInt i = size / (pageSize); i>0; --i)
			{
			DEBUG_PRINT1((_L("%S : Opening the file\n"), &TestNameBuffer));
			DOTEST((ret = file.Replace(fs, filename, EFileWrite)),
				   (KErrNone == ret));

			random = random * 69069 + 1;
			theAddr = (TUint8 *)(start + ((TInt64(random) * TInt64(size - pageSize)) >> 32));
			sourceData.Set(theAddr,pageSize);
			DEBUG_PRINT1((_L("%S : Writing the file\n"), &TestNameBuffer));
			ret = file.Write(sourceData);
			if (ret != KErrNone)
				{
				DEBUG_PRINT((_L("%S : Write returned error %d\n"), &TestNameBuffer, ret));
				}
			DEBUG_PRINT1((_L("%S : Closing the file\n"), &TestNameBuffer));
			file.Close();
			
			if (checkData)
				{
				TPtr8  theBuf = checkData->Des();

#ifdef WANT_FS_CACHE_STATS 
				// Page cache
				TFileCacheStats stats1;
				TFileCacheStats stats2;
				ret = controlIo(fs,drvNum, KControlIoFileCacheStats, stats1);
				if ((ret != KErrNone) && (ret != KErrNotSupported))
					{
					DEBUG_PRINT((_L("%S : KControlIoFileCacheStats 1 failed %d\n"), &TestNameBuffer, ret));
					}

				if (aThreadId & 1)
					{
					// flush closed files queue
					ret = fs.ControlIo(drvNum, KControlIoFlushClosedFiles);
					if (ret != KErrNone)
						{
						DEBUG_PRINT((_L("%S : KControlIoFlushClosedFiles failed %d\n"), &TestNameBuffer, ret));
						}
					}
				else
#endif //WANT_FS_CACHE_STATS 
					{
					// rename file to make sure it has cleared the cache.				
					TBuf<32>	newname;
					newname.Format(_L("d:\\Pageldrtst%d.temp"), aThreadId);
					if (drvNum >= 0)
						{
						newname[0] = 'a' + drvNum;
						}
					fs.Rename(filename, newname);
					filename = newname;
					}
#ifdef WANT_FS_CACHE_STATS 
				ret = controlIo(fs,drvNum, KControlIoFileCacheStats, stats2);
				if (ret != KErrNone && ret != KErrNotSupported)
					{
					DEBUG_PRINT((_L("%S : KControlIoFileCacheStats2 failed %d\n"), &TestNameBuffer, ret));
					}

				allocatedSegmentCount = (allocatedSegmentCount > stats1.iAllocatedSegmentCount) ? allocatedSegmentCount : stats1.iAllocatedSegmentCount;
				filesOnClosedQueue = (filesOnClosedQueue > stats1.iFilesOnClosedQueue) ? filesOnClosedQueue : stats1.iFilesOnClosedQueue;
#endif //WANT_FS_CACHE_STATS 

				DOTEST((ret = file.Open(fs, filename, EFileRead)),
					   (KErrNone == ret));
				// now read back the page that we wrote and compare with the source.
				ret = file.Read(0, theBuf, pageSize);
				if (ret == KErrNone)
					{		
					ret = sourceData.Compare(theBuf);
					if (ret != 0)
						{
						DEBUG_PRINT((_L("%S : read compare error %d\n"), &TestNameBuffer, ret));
						}
					}
				else
					{
					DEBUG_PRINT((_L("%S : failed read compare, error %d\n"), &TestNameBuffer, ret));
					}
				file.Close();
				}
			DEBUG_PRINT1((_L("%S : Deleting the file\n"), &TestNameBuffer));
			ret = fs.Delete(filename);
			if (KErrNone != ret)
				{
				DEBUG_PRINT((_L("%S [%d] Delete %S Failed %d!\n"), &TestNameBuffer, aThreadId, &filename, ret));
				}
		
			if (TestPrioChange)
				{
				TThreadPriority originalThreadPriority = thisThread.Priority();
				DEBUG_PRINT1((_L("%S [%d] media thread before priority change, stop = %d\n"), &TestNameBuffer, aThreadId, TestStopMedia));
				thisThread.SetPriority(EPriorityLess);
				User::AfterHighRes(0);
				thisThread.SetPriority(originalThreadPriority);
				DEBUG_PRINT1((_L("%S [%d] media thread after priority change, stop = %d\n"), &TestNameBuffer, aThreadId, TestStopMedia));
				}
			if (TestStopMedia)
				break;
			}
		if (TestStopMedia)
			break;
		}

#ifdef WANT_FS_CACHE_STATS 
	DEBUG_PRINT((_L("%S : [%d] allocPageCount %d filesClosedQueue %d \n"),&TestNameBuffer, aThreadId,allocatedSegmentCount,filesOnClosedQueue));
#endif //WANT_FS_CACHE_STATS 

	if (checkData)
		{
		delete checkData;
		}
	fs.Close();
	DEBUG_PRINT1((_L("Done in %d ticks\n"), User::TickCount() - startTime));
	return KErrNone;
	}

//
// PerformFileSystemAccessThread
// 
// Access the rom and dump it out to one of the writeable partitions...
// really just to make the media server a little busy during the test.
//
TInt PerformFileSystemAccessThread(TInt					    aThreadId,
								   RMsgQueue<TMessageBuf>   *aMsgQueue, 
								   TMessageBuf	           *aBuffer,
								   RSemaphore			   *aTheSem,
								   TBool					aLowMem)
	{
	RThread		 thisThread;
	TInt		 startTime = User::TickCount();
	RSemaphore	*pTheSem = aTheSem;
	RFs			 fs;
	RFile		 file;
	if (KErrNone != fs.Connect())
		{
		DEBUG_PRINT(_L("PerformFileSystemAccessThread : Can't connect to the FS\n"));
		return KErrGeneral;
		}

	// read all ROM pages in a random order...and write out to file in ROFs
	TInt pageSize = 0;
	UserSvr::HalFunction(EHalGroupKernel,EKernelHalPageSizeInBytes,&pageSize,0);

	HBufC8* checkData;
	HBufC8* sourceData;
	TUint32 random = 1 + aThreadId;
	TInt	dataSize = pageSize + (pageSize / 2);
	
	DOTEST((sourceData = HBufC8::New(dataSize)),
	       (sourceData != NULL));
	if (!sourceData)
		{
		DEBUG_PRINT((_L("RomAndFSThread %S : failed to alloc read buffer\n"), &TestNameBuffer));
		fs.Close();
		return KErrGeneral;
		}

	DOTEST((checkData = HBufC8::New(dataSize)),
	       (checkData != NULL));
	if (!checkData)
		{
		DEBUG_PRINT((_L("RomAndFSThread %S : failed to alloc read buffer\n"), &TestNameBuffer));
		}

	TInt		drvNum = (TestBootedFromMmc || TestOnlyFromMmc) ? FindMMCDriveNumber(fs) : FindFsNANDDrive(fs);
	TBuf<32>	filename;
	
	if (drvNum < 0)
		{
		drvNum = 3; //make it 'd' by default.
		DEBUG_PRINT((_L("FSAccessThread : error getting drive num\n")));
		}

#ifdef WANT_FS_CACHE_STATS 
	TInt allocatedSegmentCount = 0;
	TInt filesOnClosedQueue = 0;
#endif
	TInt fileIndex;
	TInt ret;

	TPtr8  pBuf = sourceData->Des();
	
	while (1)
		{
		TUint32 randomStart = random;
		// write the file
		for (fileIndex = 0; fileIndex < TEST_NUM_FILES; fileIndex ++)
			{
			filename.Format(_L("%c:\\pldrtst%d_%d.tmp"), 'a' + drvNum, aThreadId, fileIndex);

			DEBUG_PRINT1((_L("%S : Opening the file\n"), &TestNameBuffer));

			DOTEST ((ret = file.Replace(fs, filename, EFileWrite)),
				   (KErrNone == ret));

			pBuf.Zero();			
			if (fileIndex & 1)
				{
				TInt fillSize = dataSize / sizeof(TUint32);
				while (fillSize > 0)
					{
					random = random * 69069 + 1;
					pBuf.Append((const TUint8 *) &random, sizeof(random));
					fillSize --;
					}
				}
			else
				{
				pBuf.Fill('x',dataSize);
				}
		

			DEBUG_PRINT1((_L("%S : Writing the file\n"), &TestNameBuffer));
			ret = file.Write(sourceData->Des());
			if (ret != KErrNone)
				{
				DEBUG_PRINT((_L("%S : Write returned error %d\n"), &TestNameBuffer, ret));
				}
			DEBUG_PRINT1((_L("%S : Closing the file\n"), &TestNameBuffer));
			file.Close();
			}

		random = randomStart;
		// check the file
		for (fileIndex = 0; fileIndex < TEST_NUM_FILES; fileIndex ++)
			{
			filename.Format(_L("%c:\\pldrtst%d_%d.tmp"), 'a' + drvNum, aThreadId, fileIndex);
			
			if (checkData)
				{
				TPtr8  theBuf = checkData->Des();

#ifdef WANT_FS_CACHE_STATS 
				// Page cache
				TFileCacheStats stats1;
				TFileCacheStats stats2;
				ret = controlIo(fs,drvNum, KControlIoFileCacheStats, stats1);
				if ((ret != KErrNone) && (ret != KErrNotSupported))
					{
					DEBUG_PRINT((_L("%S : KControlIoFileCacheStats 1 failed %d\n"), &TestNameBuffer, ret));
					}

				if (aThreadId & 1)
					{
					// flush closed files queue
					ret = fs.ControlIo(drvNum, KControlIoFlushClosedFiles);
					if (ret != KErrNone)
						{
						DEBUG_PRINT((_L("%S : KControlIoFlushClosedFiles failed %d\n"), &TestNameBuffer, ret));
						}
					}
				else
#endif //WANT_FS_CACHE_STATS 
					{
					// rename file to make sure it has cleared the cache.				
					TBuf<32>	newname;
					newname.Format(_L("%c:\\pldrtst%d_%d.temp"), 'a' + drvNum, aThreadId, fileIndex);
					fs.Rename(filename, newname);
					filename = newname;
					}
#ifdef WANT_FS_CACHE_STATS 
				ret = controlIo(fs,drvNum, KControlIoFileCacheStats, stats2);
				if (ret != KErrNone && ret != KErrNotSupported)
					{
					DEBUG_PRINT((_L("%S : KControlIoFileCacheStats2 failed %d\n"), &TestNameBuffer, ret));
					}
				allocatedSegmentCount = (allocatedSegmentCount > stats1.iAllocatedSegmentCount) ? allocatedSegmentCount : stats1.iAllocatedSegmentCount;
				filesOnClosedQueue = (filesOnClosedQueue > stats1.iFilesOnClosedQueue) ? filesOnClosedQueue : stats1.iFilesOnClosedQueue;
#endif //WANT_FS_CACHE_STATS 

				DOTEST((ret = file.Open(fs, filename, EFileRead)),
					   (KErrNone == ret));
				// now read back the page that we wrote and compare with the source.
				ret = file.Read(0, theBuf, dataSize);
				if (ret == KErrNone)
					{
					pBuf.Zero();			
					if (fileIndex & 1)
						{
						TInt fillSize = dataSize / sizeof(TUint32);
						while (fillSize > 0)
							{
							random = random * 69069 + 1;
							pBuf.Append((const TUint8 *) &random, sizeof(random));
							fillSize --;
							}
						}
					else
						{
						pBuf.Fill('x',dataSize);
						}

					ret = sourceData->Des().Compare(theBuf);
					if (ret != 0)
						{
						DEBUG_PRINT((_L("%S :compare error %S %d\n"), &TestNameBuffer, &filename, ret));
						}
					}
				else
					{
					DEBUG_PRINT((_L("%S : failed read compare, error %d\n"), &TestNameBuffer, ret));
					}
				file.Close();
				}
			DEBUG_PRINT1((_L("%S : Deleting the file\n"), &TestNameBuffer));
			ret = fs.Delete(filename);
			if (KErrNone != ret)
				{
				DEBUG_PRINT((_L("%S [%d] Delete %S Failed %d!\n"), &TestNameBuffer, aThreadId, &filename, ret));
				}
			if (TestPrioChange)
				{
				TThreadPriority originalThreadPriority = thisThread.Priority();
				thisThread.SetPriority(EPriorityLess);
				User::AfterHighRes(0);
				thisThread.SetPriority(originalThreadPriority);
				}
			if (TestStopMedia)
				break;
			}
		if (TestStopMedia)
			break;
		}
#ifdef WANT_FS_CACHE_STATS 
	DEBUG_PRINT((_L("%S : [%d] allocPageCount %d filesClosedQueue %d \n"),&TestNameBuffer, aThreadId,allocatedSegmentCount,filesOnClosedQueue));
#endif //WANT_FS_CACHE_STATS 

	if (checkData)
		{
		delete checkData;
		}
	delete sourceData;
	fs.Close();
	DEBUG_PRINT1((_L("Done in %d ticks\n"), User::TickCount() - startTime));
	return KErrNone;
	}

//
// PerformRomAndFileSystemAccess
//
// Thread function, kicks off the file system access.
//

LOCAL_C TInt PerformRomAndFileSystemAccess(TAny* aParam)
	{
	TMessageBuf			localBuffer;
	TInt				threadId = (TInt) aParam;
	TInt				retVal = KErrGeneral;

	if (TestInterleave)	
		{
		RThread				thisThread;
		thisThread.SetPriority((TThreadPriority) TEST_INTERLEAVE_PRIO);
		}

	switch (TestMediaAccess)
		{
		default:
		break;
		
		case KTestMediaAccessBasic:
		case KTestMediaAccessMultipleThreads:
			retVal = PerformRomAndFileSystemAccessThread(threadId, &TestMsgQueue, &localBuffer, &TestMultiSem, TestingLowMem);	
		break;
				
		case KTestMediaAccessMultiplePattern:
			retVal = PerformFileSystemAccessThread(threadId, &TestMsgQueue, &localBuffer, &TestMultiSem, TestingLowMem);
		break;

		case KTestMediaAccessMixed:
			if (threadId < ((TestMultipleThreadCount + 1) / 2))
				retVal = PerformRomAndFileSystemAccessThread(threadId, &TestMsgQueue, &localBuffer, &TestMultiSem, TestingLowMem);	
			else
				retVal = PerformFileSystemAccessThread(threadId, &TestMsgQueue, &localBuffer, &TestMultiSem, TestingLowMem);
		break;
		}
	return retVal;
	}


//
// DisplayTestBanner
// 
// Output a header showing the test parameters.
//

void DisplayTestBanner(TBool aMultiple)
	{
	DBGS_PRINT((_L("%S : what = %S%S%S(0x%x), media = %S%S%S(0x%x)\n"),
				aMultiple ? &KMultipleTest : &KSingleTest,
				TestLoading & TEST_EXE ? &KTestExe : &KTestBlank,
 				TestLoading & TEST_DLL ? &KTestDll : &KTestBlank,
 				TestLoading & TEST_SELF ? &KTestSelf : &KTestBlank,
				TestLoading,
				TestWhichMedia & TEST_MEDIA_BASE ? &KTestBase : &KTestBlank,
				TestWhichMedia & TEST_MEDIA_ROM ? &KTestRom : &KTestBlank,
				(TestWhichMedia & TEST_MEDIA_ALL) == TEST_MEDIA_ALL ? &KTestAll : &KTestBlank,
				TestWhichMedia));
	DBGS_PRINT((_L("         : maxLoops = %d, threads = %d, loadHow = %S (0x%x)\n"),
				TestMaxLoops,
				TestMultipleThreadCount,
				TestLoadDllHow == TEST_DLL_GLOBAL ? &KTestGlobal : TestLoadDllHow == TEST_DLL_THREAD ? &KTestThread : &KTestFunc, TestLoadDllHow));
	DBGS_PRINT((_L("         : options = %S%S%S%S%S%S, which = %S%S%S%S (0x%x)\n"),
				TestInterleave ? &KTestInter : &KTestBlank,
				TestPrioChange ? &KTestPrio: &KTestBlank,
				(TestMediaAccess == KTestMediaAccessNone) ? &KTestBlank : &KTestMedia,
				TestingLowMem ? &KTestLowMem : &KTestBlank, 
				TestExtremeChunks ? &KTestEChunking : TestChunksPlus ? &KTestChunkingPlus : TestingChunks ? &KTestChunking : &KTestBlank, 
				TestingReaper ? &KTestReaper : &KTestBlank, 
				TestWhichTests & TEST_THRASH ? &KTestThrash : &KTestBlank,
				TestWhichTests & TEST_FORWARD ? &KTestForward : &KTestBlank,
				TestWhichTests & TEST_BACKWARD ? &KTestBackward : &KTestBlank,
				TestWhichTests & TEST_RANDOM ? &KTestRandom : &KTestBlank,
				TestWhichTests));
	}

//
// DoSingleTest
// 
// Perform the single thread test, spawning a number of threads.
//

LOCAL_C TInt DoSingleTest(TBool aLowMem = EFalse)
	{
	TUint        start = User::TickCount();
	RSemaphore	*pTheSem = NULL;
	TInt ret = KErrNone;
	DisplayTestBanner(EFalse);

	if (aLowMem)
		{
		DOTEST1(ret,TestMultiSem.CreateLocal(1),KErrNone, KErrNoMemory);
		pTheSem = &TestMultiSem;
		}
	if (TestLoading & TEST_EXE)
		{
		RProcess		theProcess;
		TRequestStatus	status;
		
		if (StartExe(theProcess, &status, 0, EFalse, aLowMem, pTheSem) == KErrNone)
			{
			User::WaitForRequest(status);
			if (theProcess.ExitType() == EExitPanic)
				{
				DBGS_PRINT((_L("%S : Process Panic'd...\n"), &TestNameBuffer));	
				}
			theProcess.Close();
			}
		}

	if (TestLoading & TEST_SELF)
		{
		RProcess		theProcess;
		TRequestStatus	status;
		
		if (StartExe(theProcess, &status, 0, ETrue, aLowMem,pTheSem) == KErrNone)
			{
			User::WaitForRequest(status);
			if (theProcess.ExitType() == EExitPanic)
				{
				DBGS_PRINT((_L("%S : Process Panic'd...\n"), &TestNameBuffer));
				}
			theProcess.Close();
			}
		}

	if (TestLoading	& TEST_DLL)
		{
		TInt maxDlls = GetNumDlls();
		if (TestLoadDllHow == TEST_DLL_GLOBAL)
			{
			TInt retVal = LoadTheLibs(theGlobalLibs, maxDlls, TestInstanceId, NULL, NULL, pTheSem);
			if (retVal != KErrNone)
				{
				DBGS_PRINT((_L("DoSingleTest - unable to load libs\n") ));
				CloseTheLibs (theGlobalLibs, PAGELDRTST_MAX_DLLS);
				if (aLowMem)
					{
					TestMultiSem.Close();
					}
				return KErrGeneral;
				}
			}

		ret = PerformTestThread((TInt) TestInstanceId, NULL, NULL, pTheSem);

		if (TestLoadDllHow == TEST_DLL_GLOBAL)
			{
			CloseTheLibs(theGlobalLibs, maxDlls);
			}
		}
	if (aLowMem)
		{
		TestMultiSem.Close();
		}

	if (!TestSilent)
		{
		TInt end = User::TickCount();
		TInt time = TUint((TUint64)(end-start)*(TUint64)TickPeriod/(TUint64)1000000);
		DBGS_PRINT((_L("\n%S : Single Test : (%u seconds)\n"), &TestNameBuffer, time));
		}

	return ret;
	}

//
// FillPage
//
// Fill a page with test data
//

void FillPage(TUint aOffset)
	{
	if (TestChunkData)
		{
		TUint32* ptr = (TUint32 *)((TUint8 *)TestChunkBase+aOffset);
		TUint32* ptrEnd = (TUint32 *)((TUint8 *)ptr + TestPageSize);
		do 
			{
			*ptr = 0x55000000 + aOffset;
			ptr ++;
			aOffset += 4;
			}
		while(ptr<ptrEnd);
		}
	}

//
// CheckPage
//
// Check a page matches test data....
//

TBool CheckPage(TUint index, TUint aOffset)
	{
	TBool ret = ETrue;
	if (TestChunkData)
		{
		TUint32* ptr = (TUint32 *)((TUint8 *)TestChunkBase+aOffset);
		TUint32* ptrEnd = (TUint32 *)((TUint8 *)ptr + TestPageSize);
		do
			{
			if (*ptr != (0x55000000 + aOffset)) 
				break;
			ptr ++;
			aOffset += 4;
			}
		while(ptr<ptrEnd);
		if (ptr==ptrEnd)
			{
			TestChunkStats[index].check.ok ++;
			}
		else
			{
			TestChunkStats[index].check.fail ++;
			ret = EFalse;
			}
		}
	return ret;
	}

//
// DoSomeChunking
//
// Lock and unlock various pages in a chunk...
//
TUint   TestChunkingIndex = 0;
TUint   TestChunkingIndexFails = 0;

void DoSomeChunking()
	{
	TUint       iters = TEST_NUM_CHUNK_PAGES / 4;
	TBool		lockit = EFalse;
	TBool		decomit = EFalse;
	TUint		index;
	TInt		ret;
	TInt		theOffset;
	
	while (iters)
		{
		TestChunkingIndex = TestChunkingIndex * 69069 + 1;
		index = TUint64((TUint64)TestChunkingIndex*(TUint64)TEST_NUM_CHUNK_PAGES)>>32;
		if (index >= TEST_NUM_CHUNK_PAGES)
			TestChunkingIndexFails ++;

		theOffset = index * TestPageSize;
		if (theOffset < TestCommitEnd)
			{
			if (lockit)
				{
				if (decomit)
					{
					ret = TestChunk.Decommit(theOffset,TestPageSize);
					if (KErrNone == ret)
						TestChunkStats[index].decommit.ok ++;
					else
						TestChunkStats[index].decommit.fail ++;
					ret = TestChunk.Commit(theOffset,TestPageSize);
					if (KErrNone == ret)
						{
						TestChunkStats[index].commit.ok ++;
						FillPage(theOffset);
						TestChunkPageState[index] = ETrue;
						}
					else
						{
						TestChunkStats[index].commit.fail ++;
						TestChunkPageState[index] = EFalse;
						}
					ret = KErrNone;
					}
				else
					{
					ret = TestChunk.Lock(theOffset,TestPageSize);
					if (KErrNone == ret)
						{
						TestChunkStats[index].lock.ok ++;
						if (!CheckPage(index, theOffset))
							FillPage(theOffset);
						TestChunkPageState[index] = ETrue;
						}
					else
						{
						TestChunkStats[index].lock.fail ++;
						TestChunkPageState[index] = EFalse;
						}
					}
				decomit = !decomit;
				}
			else
				{
				if (TestChunkPageState[index])
					{
					// this one should still be locked so the data should be ok.
					if (KErrNone == TestChunk.Lock(theOffset,TestPageSize))
						{				
						TestChunkStats[index].lock.ok ++;
						CheckPage(index, theOffset);
						}
					else
						TestChunkStats[index].lock.fail ++;
					}
				ret = TestChunk.Unlock(theOffset,TestPageSize);
				if (KErrNone == ret)
					TestChunkStats[index].unlock.ok ++;
				else
					TestChunkStats[index].unlock.fail ++;
				TestChunkPageState[index] = EFalse;
				}
			if (KErrNone != ret)			
				{
				// so now we need to commit another page in this pages place.
				ret = TestChunk.Commit(theOffset,TestPageSize);
				if (KErrNone != ret)
					{
					TestChunkStats[index].commit.fail ++;
					//DBGS_PRINT((_L("%S : DoSomeChunking[%03d] index %03d failed to commit a page  %d\n"), &TestNameBuffer, iters, index, ret));
					TestChunkPageState[index] = EFalse;
					}
				else
					{
					TestChunkStats[index].commit.ok ++;
					FillPage(theOffset);
					TestChunkPageState[index] = ETrue;
					}
				}
			lockit = !lockit;
			}
		else
			{
			RDebug::Printf("DoSomeChunking - offset was bad %d / %d", theOffset, TestCommitEnd);
			}
		iters --;
		}
	}

//
// DoMultipleTest
// 
// Perform the multiple thread test, spawning a number of threads.
// It is complicated a little because test.Printf can only be called from the first thread that calls it 
// so if we are using multiple threads we need to use a message queue to pass the debug info from the
// child threads back to the parent for the parent to then call printf.
//

TInt DoMultipleTest(TBool aLowMem = EFalse)
	{
	TInt			 index;
	TUint            start = User::TickCount();
	RThread			*pTheThreads = NULL;
	TInt			*pThreadInUse = NULL;

	RProcess		*pTheProcesses = NULL;
	TInt			*pProcessInUse = NULL;

	RThread			*pMedThreads = NULL;
	TInt			*pMedInUse = NULL;

	TRequestStatus	mediaStatus;
	RThread			mediaThread;
	TInt ret;

	RSemaphore	*pTheSem = NULL;

	DisplayTestBanner(ETrue);
	
	TestThreadsExit = EFalse;

	DOTEST1(ret,TestMultiSem.CreateLocal(1),KErrNone, KErrNoMemory);
	
	pTheSem = &TestMultiSem;
	if (TestLoading & TEST_DLL)
		{
		DOTEST((pTheThreads  = (RThread *)User::AllocZ(sizeof(RThread) * TestMultipleThreadCount)),
		       (pTheThreads != NULL))
		DOTEST((pThreadInUse = (TInt *)User::AllocZ(sizeof(TInt) * TestMultipleThreadCount)),
		       (pThreadInUse != NULL));
		RUNTEST1(pTheThreads && pThreadInUse);
		if (!(pTheThreads && pThreadInUse))
			return KErrGeneral;
		}

	if (TestLoading & TEST_EXE_SELF)
		{
		DOTEST((pTheProcesses = (RProcess *)User::AllocZ(sizeof(RProcess) * TestMultipleThreadCount)),
		       (pTheProcesses != NULL));
		DOTEST((pProcessInUse = (TInt *)User::AllocZ(sizeof(TInt) * TestMultipleThreadCount)),
		       (pProcessInUse != NULL));
		RUNTEST1(pTheProcesses && pProcessInUse);
		if (!(pTheProcesses && pProcessInUse))
			return KErrGeneral;
		}
	
	if (!TestSilent)
		{
		DOTEST1(ret,TestMsgQueue.CreateLocal(TestMultipleThreadCount * 10, EOwnerProcess),KErrNone, KErrNoMemory);
		if (ret != KErrNone)
			return KErrGeneral;
		}

	if (TestMediaAccess != KTestMediaAccessNone)
		{
		if (TestMediaAccess != KTestMediaAccessBasic)
			{
			TestStopMedia = EFalse;
			DOTEST((pMedThreads  = (RThread *)User::AllocZ(sizeof(RThread) * TestMultipleThreadCount)),
				   (pMedThreads != NULL))
			DOTEST((pMedInUse = (TInt *)User::AllocZ(sizeof(TInt) * TestMultipleThreadCount)),
				   (pMedInUse != NULL));
			RUNTEST1(pMedThreads && pMedInUse);
			if (!(pMedThreads && pMedInUse))
				return KErrGeneral;

			for (index = 0; index < TestMultipleThreadCount; index++)
				{
				DBGS_PRINT((_L("%S : Starting Media Thread %d\n"), &TestNameBuffer, index));
				DOTEST1(ret,pMedThreads[index].Create(KTestBlank,PerformRomAndFileSystemAccess,KDefaultStackSize,NULL,(TAny*) index),KErrNone, KErrNoMemory);
				if (ret == KErrNone)
					{
					pMedThreads[index].Resume();
					pMedInUse[index] = 1;
					}
				User::AfterHighRes(0);
				}
			}
		else
			{
			TestStopMedia = EFalse;
			DOTEST1(ret,mediaThread.Create(KTestBlank,PerformRomAndFileSystemAccess,KDefaultStackSize,NULL,(TAny *) 0),KErrNone, KErrNoMemory);
			if (ret == KErrNone)
				{
				mediaThread.Logon(mediaStatus);
				RUNTEST1(mediaStatus == KRequestPending);	
				mediaThread.Resume();
				}
			}
		}

	TInt maxDlls = GetNumDlls();
	if (TestLoadDllHow == TEST_DLL_GLOBAL)
		{
		TInt retVal = LoadTheLibs(theGlobalLibs, maxDlls, 0, NULL, NULL, NULL);
		if (retVal != KErrNone)
			{
			DBGS_PRINT((_L("DoMultipleTest - unable to load libs\n")));
			CloseTheLibs (theGlobalLibs, maxDlls);
			if (!TestSilent)
				{
				TestMsgQueue.Close();
				}
			TestMultiSem.Close();
			return KErrGeneral;
			}
		}

	// make sure we have a priority higher than that of the threads we spawn...
	RThread thisThread;
	TThreadPriority savedThreadPriority = thisThread.Priority();
	const TThreadPriority KMainThreadPriority = EPriorityMuchMore;
	__ASSERT_COMPILE(KMainThreadPriority>TEST_INTERLEAVE_PRIO);
	thisThread.SetPriority(KMainThreadPriority);
	

	for (index = 0; index < TestMultipleThreadCount; index++)
		{
		if (TestLoading & TEST_EXE_SELF)
			{
			if (KErrNone == StartExe(pTheProcesses[index], 0, index + ((TestLoading & TEST_DLL) ? TestMultipleThreadCount : 0), ((TestLoading & TEST_EXE_SELF) == TEST_EXE_SELF) ? (index & 2) : (TestLoading & TEST_SELF), aLowMem, pTheSem))
				{
				User::AfterHighRes(0);
				pProcessInUse[index] = 1;
				}
			}
		
	
		if (TestLoading & TEST_DLL)
			{
			DBGS_PRINT((_L("%S : Starting Thread %d\n"), &TestNameBuffer, index));
			DOTEST1(ret,pTheThreads[index].Create(KTestBlank,MultipleTestThread,KDefaultStackSize,NULL,(TAny*) index),KErrNone, KErrNoMemory);
			if (ret == KErrNone)
				{
				pTheThreads[index].Resume();
				User::AfterHighRes(0);
				pThreadInUse[index] = 1;
				}
			}
		}

	// wait for any child threads to exit and process any debug messages they pass back to the parent.
	TBool		anyUsed = ETrue;
	TMessageBuf	localBuffer;
	
	TInt		processOk = 0;
	TInt		threadOk = 0;
	TInt		processPanic = 0;
	TInt		threadPanic = 0;
	TUint		end = start;
	TUint		now;
	TUint		time;
	TUint		killNext = 0;
	TUint		numDots = 0;
	TUint		maxDots = (10*60)/TEST_DOT_PERIOD;	// No individual test should take longer than 10 minutes!
													// Most have been tuned to take between 2 and 8 minutes.
													// The autotests should not take more than 120 minutes total.

	while(anyUsed)
		{
		TInt threadCount = 0;
		TInt processCount = 0;
		anyUsed = EFalse;

		// check the message queue and call printf if we get a message.
		if (!TestSilent)
			{
			while (KErrNone == TestMsgQueue.Receive(localBuffer))
				{
				DBGS_PRINT((localBuffer));
				}
			}

		// walk through the thread list to check which are still alive.
		for (index = 0; index < TestMultipleThreadCount; index++)
			{
			if (TestLoading & TEST_DLL)
				{
				if (pThreadInUse[index])
					{
					if (pTheThreads[index].ExitType() != EExitPending)
						{
						if (pTheThreads[index].ExitType() == EExitPanic)
							{
							DBGS_PRINT((_L("%S : Thread %d Panic'd after %u ticks \n"),
								&TestNameBuffer, index, User::TickCount() - start));	
							threadPanic ++;
							}
						else
							{
							DBGS_PRINT((_L("%S : Thread %d Exited after %u ticks \n"),
								&TestNameBuffer, index, User::TickCount() - start));	
							threadOk ++;
							}
						pTheThreads[index].Close();
						pThreadInUse[index] = EFalse;
						}
					else
						{
						threadCount += 1;
						anyUsed = ETrue;
						if (TestThreadsExit)
							{
							now = User::TickCount();
							time = TUint((TUint64)(now-end)*(TUint64)TickPeriod/(TUint64)1000000);
							if (time > TEST_DOT_PERIOD)
								{
								DBGS_PRINT((_L("%S : Thread %d still running\n"), &TestNameBuffer, index));	
								}
							time = TUint((TUint64)(now-killNext)*(TUint64)TickPeriod/(TUint64)1000000);
							const TUint killTimeStep = (TEST_DOT_PERIOD+9)/10; // 1/10th of a dot
							if(time>TEST_DOT_PERIOD+killTimeStep)
								{
								killNext += killTimeStep*1000000/TickPeriod;
								DBGS_PRINT((_L("%S : killing Thread %d\n"), &TestNameBuffer, index));	
								pTheThreads[index].Kill(KErrNone);
								pTheThreads[index].Close();
								pThreadInUse[index] = EFalse;
								}
							}
						}
					}
				}
			if (TestLoading & TEST_EXE_SELF)
				{
				if (pProcessInUse[index])
					{
					if (pTheProcesses[index].ExitType() != EExitPending)
						{
						if (pTheProcesses[index].ExitType() == EExitPanic)
							{
							DBGS_PRINT((_L("%S : Process %d Panic'd after %u ticks \n"),
								&TestNameBuffer,
								index + ((TestLoading & TEST_DLL) ? TestMultipleThreadCount : 0),
								User::TickCount() - start));	
							processPanic ++;
							}
						else
							{
							DBGS_PRINT((_L("%S : Process %d Exited after %u ticks \n"),
								&TestNameBuffer,
								index + ((TestLoading & TEST_DLL) ? TestMultipleThreadCount : 0),
								User::TickCount() - start));	
							processOk ++;
							}

						pTheProcesses[index].Close();
						pProcessInUse[index] = EFalse;
						}
					else
						{
						processCount += 1;
						anyUsed = ETrue;
						if (TestThreadsExit)
							{
							now = User::TickCount();
							time = TUint((TUint64)(now-end)*(TUint64)TickPeriod/(TUint64)1000000);
							if (time > TEST_DOT_PERIOD)
								{
								DBGS_PRINT((_L("%S : Process %d still running; killing it.\n"),
									&TestNameBuffer, index));
								pTheProcesses[index].Kill(EExitKill);
								pTheProcesses[index].Close();
								pProcessInUse[index] = EFalse;
								}
							
							}
						}
					}
				}
			}

		now = User::TickCount();
		time = TUint((TUint64)(now-end)*(TUint64)TickPeriod/(TUint64)1000000);

		DBGD_PRINT((_L("%S : %d seconds (%d ticks) %d threads, %d processes still alive\n"),
			&TestNameBuffer, time, now, threadCount, processCount));

		if (time > TEST_DOT_PERIOD)
			{
			DBGS_PRINT((_L(".")));
			numDots ++;
			end += TEST_DOT_PERIOD*1000000/TickPeriod;
			if (TestingReaper)
				{
				TestingReaperCleaningFiles = ETrue;
				CleanupFiles(EFalse);
				CheckFilePresence(ETrue);
				TestingReaperCleaningFiles = EFalse;
				}
			if ((numDots >= maxDots) && (!TestThreadsExit))
				{
				DBGS_PRINT((_L("Taking longer than %d dots...exiting test case."), maxDots));
				TestThreadsExit = ETrue;
				killNext = end;
				}
			}

		if (TestingChunks)
			{
			DoSomeChunking();
			}

#ifdef TEST_THRASHING_TEST
		User::AfterHighRes(1000);
#else
		User::AfterHighRes(TickPeriod);
#endif
		}

	DBGD_PRINT((_L("%S : all test threads presumably gone now\n"), &TestNameBuffer));

	if (TestMediaAccess != KTestMediaAccessNone)
		{
		if (TestMediaAccess != KTestMediaAccessBasic)
			{
			TBool killMedia = EFalse;
			TestStopMedia = ETrue;
			anyUsed = ETrue;
			DBGS_PRINT((_L("%S : Waiting for media threads to exit...\n"), &TestNameBuffer));	
			end = User::TickCount();
			while (anyUsed)
				{
				anyUsed = EFalse;

				// check the message queue and call printf if we get a message.
				if (!TestSilent)
					{
					while (KErrNone == TestMsgQueue.Receive(localBuffer))
						{
						DBGS_PRINT((localBuffer));
						}
					}

				for (index = 0; index < TestMultipleThreadCount; index++)
					{
					if (pMedInUse[index])
						{
						if (pMedThreads[index].ExitType() != EExitPending)
							{
							if (pMedThreads[index].ExitType() == EExitPanic)
								{
								DBGS_PRINT((_L("%S : Media Thread %d Panic'd after %u ticks \n"),
									&TestNameBuffer, index, User::TickCount() - start));	
								threadPanic ++;
								}
							else
								{
								DBGS_PRINT((_L("%S : Media Thread %d Exited after %u ticks \n"),
									&TestNameBuffer, index, User::TickCount() - start));	
								threadOk ++;
								}
							pMedInUse[index] = EFalse;
							}
						else
							{
							anyUsed = ETrue;
							if (killMedia)
								{
								DBGS_PRINT((_L("%S : Media Thread %d still going after %u ticks; killing it!\n"),
									&TestNameBuffer, index, User::TickCount() - start));	
								pMedThreads[index].Kill(EExitKill);
								}
							}
						}
					}
				now = User::TickCount();
				time = TUint((TUint64)(now-end)*(TUint64)TickPeriod/(TUint64)1000000);
				if (time > TEST_DOT_PERIOD)
					{
					DBGS_PRINT((_L(".")));
					end += TEST_DOT_PERIOD*1000000/TickPeriod;
					killMedia = ETrue;
					}

				User::AfterHighRes(50000);

				}
			DBGS_PRINT((_L("%S : Media threads exited...\n"), &TestNameBuffer));	
			User::Free(pMedThreads);
			User::Free(pMedInUse);
			}
		else
			{
			TestStopMedia = ETrue;
			DBGS_PRINT((_L("%S : Waiting for media thread to exit...\n"), &TestNameBuffer));	
			end = User::TickCount();
			while (mediaThread.ExitType() == EExitPending)
				{
				now = User::TickCount();
				time = TUint((TUint64)(now-end)*(TUint64)TickPeriod/(TUint64)1000000);
				if (time > TEST_DOT_PERIOD)
					{
					DBGS_PRINT((_L("%S : Media thread still going after %u seconds; killing it!\n"),
						&TestNameBuffer, time));
					mediaThread.Kill(EExitKill);
					}
				User::AfterHighRes(50000);
				}
			User::WaitForRequest(mediaStatus);
			mediaThread.Close();
			DBGS_PRINT((_L("%S : Media thread exited...\n"), &TestNameBuffer));	
			}
		}

	DBGD_PRINT((_L("%S : all media threads presumably gone now\n"), &TestNameBuffer));

	if (!TestSilent)
		{
		TestMsgQueue.Close();
		}
	TestMultiSem.Close();

	DBGD_PRINT((_L("%S : about to close the libraries\n"), &TestNameBuffer));

	if (TestLoadDllHow == TEST_DLL_GLOBAL)
		{
		CloseTheLibs(theGlobalLibs, maxDlls);
		}

	TestThreadsExit = EFalse;

	DBGD_PRINT((_L("%S : cleaning up\n"), &TestNameBuffer));

	// cleanup the resources and exit.
	if (TestLoading & TEST_EXE_SELF)
		{
		User::Free(pTheProcesses);
		User::Free(pProcessInUse);
		}

	// cleanup the resources and exit.
	if (TestLoading & TEST_DLL)
		{
		User::Free(pTheThreads);
		User::Free(pThreadInUse);
		}

	if (!TestSilent)
		{
		end = User::TickCount();
		time = TUint((TUint64)(end-start)*(TUint64)TickPeriod/(TUint64)1000000);
		DBGS_PRINT((_L("\n%S : Multiple Test : (%u seconds)\n\tThreads panic'd = %d Ok = %d\n\tProcess panic'd = %d Ok = %d\n"), &TestNameBuffer, time, threadPanic, threadOk, processPanic, processOk));
		}

	thisThread.SetPriority(savedThreadPriority);

	return (threadPanic | processPanic) ? KErrGeneral : KErrNone;
	}

//
// DoChunkTests
//
// Allocate a chunk and assign some pages to it...
// Then do a multiple test.
//

void DoChunkTests()
	{
	SVMCacheInfo  tempPages;
	memset(&tempPages, 0, sizeof(tempPages));
	if (TestIsDemandPaged)
		{
		// Shrink the page cache down to the minimum.
		UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0);

		DBGS_PRINT((_L("Start : min %d max %d current %d maxFree %d freeRam %d\n"),
					 tempPages.iMinSize, tempPages.iMaxSize, tempPages.iCurrentSize ,tempPages.iMaxFreeSize, FreeRam()));

		// set the cache small 
		TInt minSize = 16 * TestPageSize;
		TInt maxSize = TEST_NUM_PAGES * TestPageSize;
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minSize,(TAny*)maxSize);
		}

	if (KErrNone != TestChunk.CreateDisconnectedLocal(0,0,TEST_NUM_CHUNK_PAGES *TestPageSize))
		{
		DBGS_PRINT((_L("DoChunkTests - create failed.\n")));
		return;
		}
	TestChunkBase = TestChunk.Base();
	if (TestChunkBase == NULL)
		{
		RDebug::Printf("DoChunkTests - TestChunkBase was NULL");
		TestChunk.Close();
		return;
		}
	TInt retVal = KErrNone;
	TUint index = 0;
	TestCommitEnd = 0;
	memset(TestChunkPageState, 0, sizeof(TestChunkPageState));
	memset(TestChunkStats,0,sizeof(TestChunkStats));
	while(index < TEST_NUM_CHUNK_PAGES)
		{
		retVal = TestChunk.Commit(TestCommitEnd,TestPageSize);
		if (KErrNone != retVal)
			{
			DBGS_PRINT((_L("%S : TestChunk.Commit returned %d for 0x%08x...\n"), &TestNameBuffer, retVal, TestCommitEnd));	
			break;
			}
		TestChunkPageState[index] = ETrue;
		FillPage(TestCommitEnd);
		TestCommitEnd += TestPageSize;
		index ++;
		}
	RUNTEST1(retVal == KErrNone);
	
	// now do some testing....
	TestingChunks = ETrue;
	TestInterleave = EFalse;
	TestPrioChange = ETrue;
	TestMediaAccess = KTestMediaAccessNone;
	// temp
	TestWhichMedia = TEST_MEDIA_ROM_BASE;

	if (TestChunksPlus)
		{
		TestMaxLoops = 1;
		TestMultipleThreadCount	= 40;
		}
	else if (TestExtremeChunks)
		{
		TestMaxLoops = 10;
		TestMultipleThreadCount	= 12;
		}
	else
		{
		TestMaxLoops = 3;
		TestMultipleThreadCount	= 20;
		}
	TestWhichTests = TEST_RANDOM;

	TestLoading = TEST_EXE_SELF_DLL;
	TestLoadDllHow = TEST_DLL_FUNC;
	TestChunkingIndexFails = 0;

	TEST_NEXT((_L("Multiple threads random with chunks.")));
	RUNTEST(DoMultipleTest(), KErrNone);
	
	TestingChunks = EFalse;

	// clean up.
	UserSvr::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0);
	TestChunk.Close();

	if (TestIsDemandPaged)
		{
		// put the cache back to the the original values.
		TInt minSize = tempPages.iMinSize;
		TInt maxSize = tempPages.iMaxSize;

		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minSize,(TAny*)maxSize);

		UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0);

		DBGS_PRINT((_L("Finish : min %d max %d current %d maxFree %d freeRam %d\n"),
					 tempPages.iMinSize, tempPages.iMaxSize, tempPages.iCurrentSize ,tempPages.iMaxFreeSize, FreeRam()));
		}
	TChunkTestStats  stats;

	memset(&stats, 0, sizeof(stats));
	DBGS_PRINT((_L("Stats : (pass/fail) \nindex\t\tlock\t\tunlock\t\tcommit\t\tdecommit\t\tcheck\n")));
	for (index = 0; index < TEST_NUM_CHUNK_PAGES; index ++)
		{
		DBGS_PRINT((_L("%u\t\t%d/%d\t\t%d/%d\t\t%d/%d\t\t%d/%d\t\t%d/%d\n"), 
					index,
					TestChunkStats[index].lock.ok, TestChunkStats[index].lock.fail,
					TestChunkStats[index].unlock.ok, TestChunkStats[index].unlock.fail,
					TestChunkStats[index].commit.ok, TestChunkStats[index].commit.fail,
					TestChunkStats[index].decommit.ok, TestChunkStats[index].decommit.fail,
					TestChunkStats[index].check.ok, TestChunkStats[index].check.fail));

		stats.lock.ok += TestChunkStats[index].lock.ok;
		stats.lock.fail += TestChunkStats[index].lock.fail;
		stats.unlock.ok += TestChunkStats[index].unlock.ok;
		stats.unlock.fail += TestChunkStats[index].unlock.fail;
		stats.decommit.ok += TestChunkStats[index].decommit.ok;
		stats.decommit.fail += TestChunkStats[index].decommit.fail;
		stats.commit.ok += TestChunkStats[index].commit.ok;
		stats.commit.fail += TestChunkStats[index].commit.fail;
		stats.check.ok += TestChunkStats[index].check.ok;
		stats.check.fail += TestChunkStats[index].check.fail;
		}

	DBGS_PRINT((_L("Total Stats (p/f): \n\t lock %d / %d\n\t unlock  %d / %d\n\t commit %d / %d\n\t decommit %d / %d\n\t check %d / %d\n"), 
				stats.lock.ok, stats.lock.fail,
				stats.unlock.ok, stats.unlock.fail,
				stats.commit.ok, stats.commit.fail,
				stats.decommit.ok, stats.decommit.fail,
				stats.check.ok, stats.check.fail));
	DBGS_PRINT((_L("TestChunkingIndexFails %d\n"), TestChunkingIndexFails));

	}

//
// DoReaperTests
//
// Test the reaper by deleting the transient files and re-creating them.
//

void DoReaperTests(void)
	{
	// make sure we have the full complement of files.
	CheckFilePresence(ETrue);
	
	// now do some testing....
	TestInterleave = EFalse;
	TestPrioChange = EFalse;
	TestMediaAccess = KTestMediaAccessNone;
	// temp
	TestWhichMedia = TEST_MEDIA_ALL;
	TestMaxLoops = 3;
	TestMultipleThreadCount	= 12;
	TestWhichTests = TEST_RANDOM;

	TestLoading = TEST_EXE_SELF_DLL;
	TestLoadDllHow = TEST_DLL_FUNC;
	
	TestingReaper = ETrue;

	TEST_NEXT((_L("Reaper tests.")));
	RUNTEST(DoMultipleTest(), KErrNone);
	TestInterleave = ETrue;
	TestPrioChange = ETrue;
	TEST_NEXT((_L("Reaper tests 2.")));
	RUNTEST(DoMultipleTest(), KErrNone);
	
	TestingReaper = EFalse;
	}

//
// DoBtraceTest
//
// Test the paging BTrace function.
//

void DoBtraceTest(void)
	{
#define LE4(a) ((*((a) + 3) << 24) + (*((a) + 2) << 16) + (*((a) + 1) << 8) + *(a))

	RBTrace bTraceHandle;
	
	TInt r = bTraceHandle.Open();
	test(r == KErrNone);
	
	r = bTraceHandle.ResizeBuffer(0x200000); 
	test(r == KErrNone);
	bTraceHandle.SetFilter(BTrace::EPaging, ETrue);

	// Enable trace
	bTraceHandle.Empty();
	bTraceHandle.SetMode(RBTrace::EEnable);
	
	TestLoading             = TEST_EXE_SELF_DLL;
	TestWhichMedia          = TEST_MEDIA_ROM_BASE;
	TestMaxLoops            = 2;
	TestMultipleThreadCount = 10;
	TestLoadDllHow          = TEST_DLL_FUNC;
	TestInterleave          = ETrue;
	TestPrioChange          = ETrue;
	TestMediaAccess         = KTestMediaAccessNone;
	TestWhichTests          = TEST_RANDOM;		
	TestingLowMem			= EFalse;

	RUNTEST(DoMultipleTest(TestingLowMem), KErrNone);

	bTraceHandle.SetMode(0);

	// analyse the btrace logs and display on the serial port.
	TUint8* pDataStart;
	TInt	dataSize;
	TUint8* pTemp;
	TUint8* pThis;
	TUint8* pEnd;
	TBuf<128>	data;
	while (1)
		{
		dataSize = bTraceHandle.GetData(pDataStart);
		if (dataSize <= 0)
			{
			break;
			}
		pEnd = pDataStart + dataSize;
		pTemp = pDataStart;
		while (pTemp < pEnd)
			{
			TUint8	recSize		= pTemp[BTrace::ESizeIndex];
			TUint8	recFlags	= pTemp[BTrace::EFlagsIndex];
			TUint8	recCat		= pTemp[BTrace::ECategoryIndex];
			TUint8	recSub		= pTemp[BTrace::ESubCategoryIndex];
			TUint32 addr[4];
			pThis = pTemp;

			data.Zero();
			// step over the header.
			data.Format(_L("size %d cat %d sub %d flg 0x%02x "), recSize, recCat, recSub, recFlags);
			pTemp += 4; 
					
			if (recFlags & BTrace::EHeader2Present)
				{
				data.AppendFormat(_L("h2 0x%08x "), LE4(pTemp));
				pTemp += 4;
				}
			if (recFlags & BTrace::ETimestampPresent)
				{
				data.AppendFormat(_L("ts 0x%08x "), LE4(pTemp));
				pTemp += 4;
				}
			if (recFlags & BTrace::ETimestamp2Present)
				{
				data.AppendFormat(_L("ts2 0x%08x "), LE4(pTemp));
				pTemp += 4;
				}
			if (recFlags & BTrace::EContextIdPresent)
				{
				data.AppendFormat(_L("cId 0x%08x "), LE4(pTemp));
				pTemp += 4;
				}
			TInt index;
			for (index = 0; index < 4; index ++)
				{
				if (recSize > pTemp - pThis)
					{
					addr[index] = LE4(pTemp);
					pTemp += 4;
					}
				else
					addr[index] = 0;
				}

			switch(recCat)
				{
				case BTrace::EPaging:
					{
					switch (recSub)
						{
						case BTrace::EPagingPageInBegin:
						/**
						- 4 bytes containing the virtual address which was accessed, causing this paging event.
						- 4 bytes containing the virtual address of the instuction which caused this paging event.
						  (The PC value.)
						**/
						test.Printf(_L("PageInBegin    : %S addr 0x%08x inst 0x%08x\n"), &data, addr[0], addr[1]);
						break;

						/**
						- 0 bytes. (No extra data.)
						*/
						case BTrace::EPagingPageInUnneeded:
						test.Printf(_L("PageInUnneeded : %S\n"), &data);
						break;

						/**
						- 4 bytes containing the physical address of the page 'paged in'.
						- 4 bytes containing the virtual address of the page 'paged in'.
						*/
						case BTrace::EPagingPageInROM:
						test.Printf(_L("PageInROM      : %S phys 0x%08x virt 0x%08x\n"), &data, addr[0], addr[1]);
						break;

						/**
						- 4 bytes containing the physical address of the page being 'paged out'.
						- 4 bytes containing the virtual address of the page being 'paged out'.
						*/
						case BTrace::EPagingPageOutROM:
						test.Printf(_L("PageOutROM     : %S phys 0x%08x virt 0x%08x\n"), &data, addr[0], addr[1]);
						break;

						/**
						- 4 bytes containing the physical address of the page being 'paged in'.
						*/
						case BTrace::EPagingPageInFree:
						test.Printf(_L("PageInFree     : %S phys 0x%08x\n"), &data, addr[0]);
						break;

						/**
						- 4 bytes containing the physical address of the page being 'paged out'.
						*/
						case BTrace::EPagingPageOutFree:
						test.Printf(_L("PageOutFree    : %S phys 0x%08x\n"), &data, addr[0]);
						break;

						/**
						- 4 bytes containing the physical address of the page being rejuvenated, (made young).
						- 4 bytes containing the virtual address which was accessed, causing this paging event.
						- 4 bytes containing the virtual address of the instuction which caused this paging event.
						  (The PC value.)
						*/
						case BTrace::EPagingRejuvenate:
						test.Printf(_L("Rejuvenate     : %S phys 0x%08x virt 0x%08x inst 0x%08x\n"), &data, addr[0], addr[1], addr[2]);
						break;

						/**
						- 4 bytes containing the physical address of the page accessed.
						- 4 bytes containing the virtual address which was accessed, causing this paging event.
						- 4 bytes containing the virtual address of the instuction which caused this paging event.
						  (The PC value.)
						*/
						case BTrace::EPagingPageNop:
						test.Printf(_L("PageNop        : %S phys 0x%08x virt 0x%08x inst 0x%08x\n"), &data, addr[0], addr[1], addr[2]);
						break;

						/**
						- 4 bytes containing the physical address of the page being locked.
						- 4 bytes containing the value of the lock count after the paged was locked.
						*/
						case BTrace::EPagingPageLock:
						test.Printf(_L("PageLock       : %S phys 0x%08x lock 0x%08x\n"), &data, addr[0], addr[1]);
						break;

						/**
						- 4 bytes containing the physical address of the page being unlocked.
						- 4 bytes containing the value of the lock count before the paged was unlocked.
						*/
						case BTrace::EPagingPageUnlock:
						test.Printf(_L("PageUnlock     : %S phys 0x%08x lock 0x%08x\n"), &data, addr[0], addr[1]);
						break;
		
						/**
						- 4 bytes containing the physical address of the page being 'paged out'.
						- 4 bytes containing the virtual address of the page being 'paged out'.
						*/
						case BTrace::EPagingPageOutCache:
						test.Printf(_L("PageOutCache   : %S phys 0x%08x virt 0x%08x\n"), &data, addr[0], addr[1]);
						break;
		
						/**
						- 4 bytes containing the physical address of the page 'paged in'.
						- 4 bytes containing the virtual address of the page 'paged in'.
						*/
						case BTrace::EPagingPageInCode:
						test.Printf(_L("PageInCode     : %S phys 0x%08x virt 0x%08x\n"), &data, addr[0], addr[1]);
						break;

						/**
						- 4 bytes containing the physical address of the page being 'paged out'.
						- 4 bytes containing the virtual address of the page being 'paged out'.
						*/
						case BTrace::EPagingPageOutCode:
						test.Printf(_L("PageOutCode    : %S phys 0x%08x virt 0x%08x\n"), &data, addr[0], addr[1]);
						break;
		
						/**
						- 4 bytes containing the physical address of the page 'paged in'.
						- 4 bytes containing the virtual address of the page 'paged in'.
						*/
						case BTrace::EPagingMapCode:
						test.Printf(_L("MapCode        : %S phys 0x%08x virt 0x%08x\n"), &data, addr[0], addr[1]);
						break;
						
						/**
						- 4 bytes containing the physical address of the page being aged, (made old).
						*/
						case BTrace::EPagingAged:
						test.Printf(_L("Aged           : %S phys 0x%08x\n"), &data, addr[0]);
						break;
						}
					}
				break;

				default:
				
				break;
				}
			pTemp = BTrace::NextRecord(pThis);
			}
		bTraceHandle.DataUsed();
		}
	bTraceHandle.Close();
	}

//
// ParseCommandLine 
//
// read the arguments passed from the command line and set global variables to 
// control the tests.
//

TBool ParseCommandLine()
	{
	TBuf<256> args;
	User::CommandLine(args);
	TLex	lex(args);
	TBool	retVal = ETrue;
	
	// initially test for arguments, the parse them, if not apply some sensible defaults.
	TBool	foundArgs = EFalse;	
		
	FOREVER
		{
		TPtrC  token=lex.NextToken();
		if(token.Length()!=0)
			{
			if ((token == _L("help")) || (token == _L("-h")) || (token == _L("-?")))
				{
				DBGS_PRINT((_L("\nUsage: [ single | multiple <numThreads>] [ dll | exe | self | complete ] [func | thread | global ] [ rom | base | mixed | mall ] [reaper] [chunks|echunks|chunks+ {nochunkdata}] [prio] [media] [lowmem] [forward | backward | random | all] [loadGlobal | loadThread | loadFunc] [interleave] [d_exc] [btrace] [defrag] [noclean] [min <pages>] [max <pages>] [stressfree] [iters <iters>]\n'-' indicated infinity.\n\n")));
				test.Getch();
				}
			else  if (token == _L("mmc"))
				{
				TestOnlyFromMmc = ETrue;
				}
			else  if (token == _L("min"))
				{
				TPtrC val=lex.NextToken();
				TLex lexv(val);
				TInt value;
				lexv.Val(value);
				TestMinCacheSize = value * 4096;
				}
			else  if (token == _L("max"))
				{
				TPtrC val=lex.NextToken();
				TLex lexv(val);
				TInt value;
				lexv.Val(value);
				TestMaxCacheSize = value * 4096;
				}
			else  if (token == _L("interleave"))
				{
				TestInterleave = ETrue;
				}
			else if (token == _L("auto"))
				{
				TestFullAutoTest = EFalse;
				retVal = EFalse;
				}
			else if (token == _L("stressfree"))
				{
				TestStressFree = !TestStressFree;
				retVal = EFalse;
				}
			else if (token == _L("fullauto"))
				{
				TestFullAutoTest = ETrue;
				retVal = EFalse;
				}
			else if (token == _L("prio"))
				{
				TestPrioChange = !TestPrioChange;
				}
			else if (token == _L("media"))
				{
				TestMediaAccess = KTestMediaAccessBasic;
				}
			else if (token == _L("reaper"))
				{
				TestReaper = ETrue;
				}
			else if (token == _L("btrace"))
				{
				TestBtrace = ETrue;
				}
			else if (token == _L("defrag"))
				{
				TestDefrag = ETrue;
				}
			else if (token == _L("echunks"))
				{
				TestChunks = ETrue;
				TestExtremeChunks = ETrue;
				}
			else if (token == _L("chunks+"))
				{
				TestChunks = ETrue;
				TestChunksPlus = ETrue;
				}
			else if (token == _L("chunks"))
				{
				TestChunks = ETrue;
				}
			else if (token == _L("nochunkdata"))
				{
				TestChunkData = EFalse;
				}
			else if (token == _L("lowmem"))
				{
				TestLowMem = ETrue;
				}
			else if (token == _L("dll"))
				{
				TestLoading = TEST_DLL;
				}
			else if (token == _L("exe"))
				{
				TestLoading = TEST_EXE;
				}
			else if (token == _L("self"))
				{
				TestLoading = TEST_SELF;
				}
			else if (token == _L("complete"))
				{
				TestLoading |= TEST_EXE_SELF_DLL;
				}
			else if (token == _L("rom"))
				{
				TestWhichMedia = TEST_MEDIA_ROM;
				}
			else if (token == _L("base"))
				{
				TestWhichMedia = TEST_MEDIA_BASE;
				}
			else if (token == _L("mixed"))
				{
				TestWhichMedia |= TEST_MEDIA_ROM_BASE;
				}
			else if (token == _L("all_media"))
				{
				TestWhichMedia |= TEST_MEDIA_ALL;
				}
			else if (token == _L("debug"))
				{
				if (!TestSilent)
					{
					TestDebug = ETrue;
					TestPrioChange = ETrue;
					}
				}
			else if (token == _L("silent"))
				{
				TestSilent = ETrue;
				TestDebug = EFalse;
				}
			else if (token == _L("noclean"))
				{
				TestNoClean = ETrue;
				}
			else if (token == _L("d_exc"))
				{
				TestD_Exc = ETrue;
				}
			else if (token == _L("global"))
				{
				TestLoadDllHow = TEST_DLL_GLOBAL;
				}	
			else if (token == _L("thread"))
				{
				TestLoadDllHow = TEST_DLL_THREAD;
				}	
			else if (token == _L("func"))
				{
				TestLoadDllHow = TEST_DLL_FUNC;
				}	
			else if (token == _L("single"))
				{
				TestSingle = ETrue;
				}
			else if (token == _L("multiple"))
				{
				TPtrC val=lex.NextToken();
				TLex lexv(val);
				TInt value;

				if (lexv.Val(value)==KErrNone)
					{
					if ((value <= 0) || (value > 100))
						{
						TestMultipleThreadCount = 10;
						}
					else
						{
						TestMultipleThreadCount = value;
						}
					}
				else
					{
					DBGS_PRINT((_L("Bad value for thread count '%S' was ignored.\n"), &val));
					retVal = EFalse;
					break;
					}
				TestMultiple = ETrue;
				}
			else if (token == _L("forward"))
				{
				TestWhichTests = TEST_FORWARD;
				}
			else if (token == _L("backward"))
				{
				TestWhichTests = TEST_BACKWARD;
				}
			else if (token == _L("random"))
				{
				TestWhichTests = TEST_RANDOM;
				}
			else if (token == _L("all"))
				{
				TestWhichTests = TEST_ALL;
				}
			else  if (token == _L("inst"))
				{
				TPtrC val=lex.NextToken();
				TLex lexv(val);
				TInt value;

				if (lexv.Val(value)==KErrNone)
					{
					TestInstanceId = value;
					}
				}
			else  if (token == _L("iters"))
				{
				TPtrC val=lex.NextToken();
				TLex lexv(val);
				TInt value;

				if (val==_L("-"))
					{
					TestMaxLoops = KMaxTInt;
					}
				else
					{
					if (lexv.Val(value)==KErrNone)
						{
						TestMaxLoops = value;
						}
					else
						{
						DBGS_PRINT((_L("Bad value for thread count '%S' was ignored.\n"), &val));
						retVal = EFalse;
						break;
						}
					}
				}
			else
				{
				if ((foundArgs == EFalse) && (token.Length() == 1))
					{
					// Single letter argument...only run on 'd'
					if (token.CompareF(_L("d")) == 0)
						{

						TestFullAutoTest = EFalse;
						TestIsAutomated = ETrue;
						break;
						}
					else
						{
						if (!TestSilent)
							{
							test.Title();
							test.Start(_L("Skipping non drive 'd' - Test Exiting."));
							test.End();
							}
						foundArgs = ETrue;
						TestExit = ETrue;
						break;
						}
					}
				DBGS_PRINT((_L("Unknown argument '%S' was ignored.\n"), &token));
				break;
				}
			foundArgs = ETrue;
			}
		else
			{
			break;
			}
		}
	if (!foundArgs)
		{
		retVal = EFalse;
		}
	return retVal;
	}

//
// AreWeTheTestBase
//
// Test whether we are the root of the tests.
//
void AreWeTheTestBase()
	{
	if (!TestSilent)
		{
		TFileName  filename(RProcess().FileName());

		TParse	myParse;
		myParse.Set(filename, NULL, NULL);
		TestNameBuffer.Zero();
		TestNameBuffer.Append(myParse.Name());
		TestNameBuffer.Append(_L(".exe"));

		TestWeAreTheTestBase = !TestNameBuffer.Compare(TestPlExeNames[KTestMediaBase]);

		RFs fs;
		if (KErrNone == fs.Connect())
			{
			TEntry  anEntry;
			TInt retVal = fs.Entry(_L("z:\\test\\mmcdemandpaginge32tests.bat"), anEntry);
			if (retVal == KErrNone)
				{
				TestBootedFromMmc = ETrue;
				}
			else
				{
				TestBootedFromMmc = EFalse;
				}
			fs.Close();
			}
		}
	else
		{
		TestNameBuffer.Zero();
		TestNameBuffer.Append(_L("t_pageldrtst.exe"));
		}
	}
#define  MEDNONE	KTestMediaAccessNone
#define MEDBASIC	KTestMediaAccessBasic
#define MEDMTHRE	KTestMediaAccessMultipleThreads
#define MEDMPATT	KTestMediaAccessMultiplePattern
#define MEDMIX		KTestMediaAccessMixed

TTheTests TheAutoTests[] =
	{// fullOnly,           loading,               media,  multi, loops, threads,         loadHow,  inter,   prio,    media,  whichTests, lowmem, free, testName
#ifdef TEST_SHORT_TEST
		{ EFalse,          TEST_DLL,      TEST_MEDIA_ALL,  ETrue,     2,      24,   TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("DLL Load (ALL Media) Multiple thread all."), },
#else
		{ EFalse,          TEST_DLL,     TEST_MEDIA_BASE,  ETrue,     5,      24,   TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_THRASH, EFalse,    0, }, //_L("DLL Load (ROM) Multiple thread Thrash."), },
		{  ETrue,          TEST_DLL, TEST_MEDIA_ROM_BASE, EFalse,     5,      20,   TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_ALL,    EFalse,    0, }, //_L("DLL Load (ROM/ROFS) Single thread all."), },
		{  ETrue,          TEST_EXE, TEST_MEDIA_ROM_BASE, EFalse,     5,      20,   TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_ALL,    EFalse,    0, }, //_L("Exe Load (ROM/ROFS) Single thread."), },
		{  ETrue,         TEST_SELF, TEST_MEDIA_ROM_BASE, EFalse,     5,      20,   TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_ALL,    EFalse,    0, }, //_L("Self Load (ROM/ROFS) Single thread."), },
		{  ETrue,          TEST_DLL, TEST_MEDIA_ROM_BASE,  ETrue,     5,      20,   TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("DLL Load (ROM/ROFS) Multiple thread all."), },
		{ EFalse,          TEST_DLL,      TEST_MEDIA_ALL,  ETrue,     3,      20,   TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("DLL Load (ALL Media) Multiple thread all."), },
		{  ETrue, TEST_EXE_SELF_DLL,      TEST_MEDIA_ALL,  ETrue,     2,      16,   TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("DLL/EXE/SELF Load (All Media) Multiple threads."), },
		{ EFalse, TEST_EXE_SELF_DLL,      TEST_MEDIA_ALL,  ETrue,     2,      16,   TEST_DLL_FUNC, EFalse,  ETrue,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("DLL/EXE/SELF Load (All Media) Multiple threads with prio."), },
		{ EFalse, TEST_EXE_SELF_DLL,      TEST_MEDIA_ALL,  ETrue,     2,      10,   TEST_DLL_FUNC, EFalse, EFalse, MEDBASIC, TEST_RANDOM, EFalse,    0, }, //_L("DLL/EXE/SELF Load (All Media) Multiple threads with media access."), },
		{ EFalse, TEST_EXE_SELF_DLL,      TEST_MEDIA_ALL,  ETrue,     2,      12,   TEST_DLL_FUNC, EFalse,  ETrue, MEDBASIC, TEST_RANDOM, EFalse,    0, }, //_L("DLL/EXE/SELF Load (All Media) Multiple threads with media access and prio."), },
		{  ETrue, TEST_EXE_SELF_DLL,      TEST_MEDIA_ALL,  ETrue,     2,      16, TEST_DLL_THREAD, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("DLL/EXE/SELF Load thread (All Media) Multiple threads."), },
		{ EFalse, TEST_EXE_SELF_DLL,      TEST_MEDIA_ALL,  ETrue,     2,      16, TEST_DLL_THREAD, EFalse,  ETrue,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("DLL/EXE/SELF Load thread (All Media) Multiple threads with prio."), },
		{  ETrue, TEST_EXE_SELF_DLL,      TEST_MEDIA_ALL,  ETrue,     2,      12, TEST_DLL_THREAD, EFalse,  ETrue, MEDBASIC, TEST_RANDOM, EFalse,    0, }, //_L("DLL/EXE/SELF Load thread (All Media) Multiple threads with media access and prio."), },
		{  ETrue, TEST_EXE_SELF_DLL,      TEST_MEDIA_ALL,  ETrue,     2,      16, TEST_DLL_GLOBAL, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("DLL/EXE/SELF Load global (All Media) Multiple threads."), },
		{ EFalse, TEST_EXE_SELF_DLL,      TEST_MEDIA_ALL,  ETrue,     2,      16, TEST_DLL_GLOBAL, EFalse,  ETrue,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("DLL/EXE/SELF Load global (All Media) Multiple threads with prio."), },
		{  ETrue, TEST_EXE_SELF_DLL,      TEST_MEDIA_ALL,  ETrue,     2,      12, TEST_DLL_GLOBAL, EFalse,  ETrue, MEDBASIC, TEST_RANDOM, EFalse,    0, }, //_L("DLL/EXE/SELF Load global (All Media) Multiple threads with media access and prio."), },
		{ EFalse, TEST_EXE_SELF_DLL,      TEST_MEDIA_ALL,  ETrue,     2,      16,   TEST_DLL_FUNC,  ETrue, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("DLL/EXE/SELF Load (All Media) Multiple threads with interleave."), },
		{  ETrue, TEST_EXE_SELF_DLL,      TEST_MEDIA_ALL,  ETrue,     2,      16,   TEST_DLL_FUNC,  ETrue,  ETrue,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("DLL/EXE/SELF Load (All Media) Multiple threads with interleave, prio."), },

		{  ETrue, TEST_EXE_SELF_DLL,      TEST_MEDIA_ALL,  ETrue,     2,      16,   TEST_DLL_FUNC,  ETrue,  ETrue, MEDBASIC, TEST_RANDOM, EFalse,    0, }, //_L("DLL/EXE/SELF Load (All Media) Multiple threads with interleave, media and prio."), },
		{  ETrue, TEST_EXE_SELF_DLL,      TEST_MEDIA_ALL,  ETrue,     2,      12,   TEST_DLL_FUNC,  ETrue,  ETrue, MEDMTHRE, TEST_RANDOM, EFalse,    0, }, //_L("DLL/EXE/SELF Load (All Media) Multiple threads with interleave, multi media and prio."), },
		{  ETrue, TEST_EXE_SELF_DLL,      TEST_MEDIA_ALL,  ETrue,     2,      12,   TEST_DLL_FUNC,  ETrue,  ETrue, MEDMPATT, TEST_RANDOM, EFalse,    0, }, //_L("DLL/EXE/SELF Load (All Media) Multiple threads with interleave, media and prio."), },
		{  ETrue, TEST_EXE_SELF_DLL,      TEST_MEDIA_ALL,  ETrue,     2,      12,   TEST_DLL_FUNC,  ETrue,  ETrue,   MEDMIX, TEST_RANDOM, EFalse,    0, }, //_L("DLL/EXE/SELF Load (All Media) Multiple threads with interleave, media and prio."), },
		{ EFalse, TEST_EXE_SELF_DLL, TEST_MEDIA_ROM_BASE,  ETrue,     2,      10,   TEST_DLL_FUNC,  ETrue,  ETrue,   MEDMIX, TEST_RANDOM, EFalse,    0, }, //_L("DLL/EXE/SELF Load Multiple threads with interleave, media and prio."), },
#endif // TEST_SHORT_TEST
	};
#define NUM_AUTO_TESTS (TInt)(sizeof(TheAutoTests) / sizeof(TTheTests))

//
// PerformAutoTest
//
// The autotest.
//

void PerformAutoTest(TBool aReduceTime = EFalse)
	{
	TInt        testIndex;
	TTheTests  *pTest = &TheAutoTests[0];
	
	DoStats();

	for (testIndex = 0; testIndex < NUM_AUTO_TESTS; testIndex ++, pTest++)
		{
		if (   (   !TestWeAreTheTestBase 
			    && (   (pTest->testLoadDllHow != TEST_DLL_FUNC)
				    || !pTest->testMultiple))
			|| ((TestFullAutoTest == EFalse) && (pTest->testFullAutoOnly)))
			{
			continue;
			}
		
		TestLoading             = pTest->testLoading;
		TestWhichMedia          = pTest->testWhichMedia;
		TestMaxLoops            = aReduceTime ? 1 : pTest->testMaxLoops;
		TestMultipleThreadCount = aReduceTime ? 10 : pTest->testMultipleThreadCount;
		TestLoadDllHow          = pTest->testLoadDllHow;
		TestInterleave          = pTest->testInterleave;
		TestPrioChange          = pTest->testPrioChange;
		TestMediaAccess         = pTest->testMediaAccess;
		if (aReduceTime && (TestMediaAccess != MEDBASIC) && (TestMediaAccess != MEDNONE))
			{
			continue;
			}
		TestWhichTests          = pTest->testWhichTests;		
		TestingLowMem			= pTest->testLowMem;
		if (!TestSilent)
			{
			test.Next(_L("Auto Test"));
			}
		if (pTest->testMultiple)
			{
			RUNTEST(DoMultipleTest(ETrue), KErrNone);
			}
		else
			{
			RUNTEST(DoSingleTest(ETrue), KErrNone);
			}

		DoStats();

#ifdef TEST_KERN_HEAP
		__KHEAP_MARK;
		__KHEAP_CHECK(0);
		__KHEAP_MARKEND;
#endif
		}
#ifdef TEST_KERN_HEAP
	__KHEAP_MARK;
	__KHEAP_CHECK(0);
	__KHEAP_MARKEND;
#endif
	}

TTheTests TheLowMemTests[] =
	{// fullOnly,           loading,               media,  multi, loops, threads,       loadHow,  inter,   prio,    media,  whichTests, lowmem, free, testName
#ifndef TEST_SHORT_TEST
		{  ETrue, TEST_EXE_SELF_DLL, TEST_MEDIA_ROM_BASE, EFalse,     1,       1, TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("Single thread with Low memory (init)."), },
		{  ETrue, TEST_EXE_SELF_DLL, TEST_MEDIA_ROM_BASE, EFalse,     5,       1, TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM,  ETrue,    0, }, //_L("Single thread with Low memory."), },
		{  ETrue,          TEST_DLL, TEST_MEDIA_ROM_BASE, EFalse,     1,       1, TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("Multiple thread with Low memory (init)."), },
		{  ETrue, TEST_EXE_SELF_DLL, TEST_MEDIA_ROM_BASE,  ETrue,     2,      16, TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM,  ETrue,    0, }, //_L("Multiple thread with Low memory ."), },
		{ EFalse,          TEST_DLL, TEST_MEDIA_ALL,      EFalse,     5,       1, TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("Multiple thread with Low memory and All media(init)."), },
		{ EFalse, TEST_EXE_SELF_DLL, TEST_MEDIA_ALL,       ETrue,     2,      12, TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM,  ETrue,    0, }, //_L("Multiple thread with Low memory and All media."), },
		{  ETrue,          TEST_DLL, TEST_MEDIA_ROM_BASE, EFalse,     1,       1, TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("Multiple thread with Low memory, with starting free ram (init)."), },
		{  ETrue, TEST_EXE_SELF_DLL, TEST_MEDIA_ROM_BASE,  ETrue,     2,      16, TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM,  ETrue,   32, }, //_L("Multiple thread with Low memory, with starting free ram."), },
		{  ETrue,          TEST_DLL, TEST_MEDIA_ROM_BASE, EFalse,     1,      16, TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("Multiple thread with Low memory and prio and media access(init)."), },
		{  ETrue, TEST_EXE_SELF_DLL, TEST_MEDIA_ROM_BASE,  ETrue,     2,      16, TEST_DLL_FUNC, EFalse,  ETrue, MEDBASIC, TEST_RANDOM,  ETrue,    0, }, //_L("Multiple thread with Low memory and prio and media access."), },
		{  ETrue,          TEST_DLL, TEST_MEDIA_ROM_BASE, EFalse,     1,       1, TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("Multiple thread with Low memory interleave, prio and media access(init)."), },
		{  ETrue, TEST_EXE_SELF_DLL, TEST_MEDIA_ROM_BASE,  ETrue,     2,      16, TEST_DLL_FUNC,  ETrue,  ETrue, MEDBASIC, TEST_RANDOM,  ETrue,    0, }, //_L("Multiple thread with Low memory interleave, prio and media access."), },
		{  ETrue,          TEST_DLL, TEST_MEDIA_ROM_BASE, EFalse,     1,       1, TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("Multiple thread with Low memory interleave, media access and All media (init)."), },
		{  ETrue, TEST_EXE_SELF_DLL, TEST_MEDIA_ALL,       ETrue,     2,      16, TEST_DLL_FUNC,  ETrue, EFalse, MEDBASIC, TEST_RANDOM,  ETrue,    0, }, //_L("Multiple thread with Low memory interleave, media access and All media + loading."), },
		{ EFalse,		   TEST_DLL, TEST_MEDIA_ROM_BASE, EFalse,    10,       1, TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("Single thread with Low memory (init)."), },
		{ EFalse,          TEST_DLL, TEST_MEDIA_ROM_BASE, EFalse,     5,       1, TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM,  ETrue,    0, }, //_L("Single thread with Low memory."), },
#endif //TEST_SHORT_TEST
		{ EFalse,          TEST_DLL, TEST_MEDIA_ROM_BASE, EFalse,    10,       1, TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("Multiple thread with Low memory interleave, prio, media access and All media (init)."), },
		{ EFalse, TEST_EXE_SELF_DLL, TEST_MEDIA_ROM_BASE,  ETrue,     2,      16, TEST_DLL_FUNC,  ETrue,  ETrue, MEDBASIC, TEST_RANDOM,  ETrue,    0, }, //_L("Multiple thread with Low memory interleave, prio, media access and All media + loading."), },
		{ EFalse,          TEST_DLL, TEST_MEDIA_ROM_BASE, EFalse,     5,       1, TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("Multiple thread with Low memory interleave, prio, media access and All media (init)."), },
		{ EFalse, TEST_EXE_SELF_DLL, TEST_MEDIA_ROM_BASE,  ETrue,     2,      10, TEST_DLL_FUNC,  ETrue,  ETrue, MEDMTHRE, TEST_RANDOM,  ETrue,    0, }, //_L("Multiple thread with Low memory interleave, prio, multi media access and All media + loading."), },
		{  ETrue,          TEST_DLL, TEST_MEDIA_ROM_BASE, EFalse,     1,       1, TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("Multiple thread with Low memory interleave, prio, media access and All media (init)."), },
		{  ETrue, TEST_EXE_SELF_DLL, TEST_MEDIA_ALL,       ETrue,     2,      16, TEST_DLL_FUNC,  ETrue,  ETrue, MEDBASIC, TEST_RANDOM,  ETrue,    0, }, //_L("Multiple thread with Low memory interleave, prio, media access and All media + loading."), },
		{  ETrue,          TEST_DLL, TEST_MEDIA_ROM_BASE, EFalse,     1,       1, TEST_DLL_FUNC, EFalse, EFalse,  MEDNONE, TEST_RANDOM, EFalse,    0, }, //_L("Multiple thread with Low memory interleave, prio, media access and All media (init)."), },
		{  ETrue, TEST_EXE_SELF_DLL, TEST_MEDIA_ALL,       ETrue,     2,      16, TEST_DLL_FUNC,  ETrue,  ETrue, MEDMTHRE, TEST_RANDOM,  ETrue,    0, }, //_L("Multiple thread with Low memory interleave, prio, multi media access and All media + loading."), },

	};
#define NUM_LOWMEM_TESTS (TInt)(sizeof(TheLowMemTests) / sizeof(TTheTests))

//
// DoLowMemTest
//
// Low Memory Test
//
void DoLowMemTest(TBool aEnableAllMedia = EFalse)
	{
	TInt r = User::LoadLogicalDevice(KPageStressTestLddName);
	RUNTEST1(r==KErrNone || r==KErrAlreadyExists);
	RUNTEST(PagestressLdd.Open(),KErrNone);
	RUNTEST(PagestressLdd.DoSetDebugFlag((TInt)TestDebug),KErrNone);
	
	SVMCacheInfo  tempPages;
	memset(&tempPages, 0, sizeof(tempPages));
	if (TestIsDemandPaged)
		{
		// get the old cache info
		UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0);
		TInt minSize = 8 * 4096;
		TInt maxSize = 256 * 4096;
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minSize,(TAny*)maxSize);
		}

	TInt		testIndex;
	TTheTests  *pTest = &TheLowMemTests[0];
	for (testIndex = 0; testIndex < NUM_LOWMEM_TESTS; testIndex ++, pTest++)
		{
		if (   (!aEnableAllMedia && (pTest->testWhichMedia == TEST_MEDIA_ALL))
		    || ((TestFullAutoTest == EFalse) && (pTest->testFullAutoOnly)))
			{
			continue;
			}

		TestLoading             = pTest->testLoading;
		TestWhichMedia          = pTest->testWhichMedia;
		TestMaxLoops            = pTest->testMaxLoops;
		TestMultipleThreadCount = pTest->testMultipleThreadCount;
		TestLoadDllHow          = pTest->testLoadDllHow;
		TestInterleave          = pTest->testInterleave;
		TestPrioChange          = pTest->testPrioChange;
		TestMediaAccess         = pTest->testMediaAccess;
		TestWhichTests          = pTest->testWhichTests;		
		TestingLowMem			= pTest->testLowMem;
		if (!TestSilent)
			{
			test.Next(_L("Low Memory"));
			}
		if (pTest->testLowMem)
			{
			PagestressLdd.DoConsumeRamSetup(pTest->testFreeRam, TEST_LM_BLOCKSIZE);
			}

		if (pTest->testMultiple)
			{
			RUNTEST(DoMultipleTest(pTest->testLowMem), KErrNone);
			}
		else
			{
			RUNTEST(DoSingleTest(pTest->testLowMem), KErrNone);
			}

		if (pTest->testLowMem)
			{
			PagestressLdd.DoConsumeRamFinish();
			}

		DoStats();
#ifdef TEST_KERN_HEAP
		__KHEAP_MARK;
		__KHEAP_CHECK(0);
		__KHEAP_MARKEND;
#endif
		}

	if (!TestSilent)
		{
		test.Next(_L("Close test driver"));
		}
	PagestressLdd.Close();
	RUNTEST(User::FreeLogicalDevice(KPageStressTestLddName), KErrNone);

	if (TestIsDemandPaged)
		{
		TInt minSize = tempPages.iMinSize;
		TInt maxSize = tempPages.iMaxSize;
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minSize,(TAny*)maxSize);
		}

#ifdef TEST_KERN_HEAP
	__KHEAP_MARK;
	__KHEAP_CHECK(0);
	__KHEAP_MARKEND;
#endif
	TestingLowMem = EFalse;

	}

//
// MultipleDefragThread
//
// Thread function, one created for each zone in a multiple thread test.
//

LOCAL_C TInt MultipleDefragThread(TAny* aUseTb)
	{
	TInt numZones = 1;
	TInt zoneId = (TInt)aUseTb;

	if (TestZoneCount > TEST_MAX_ZONE_THREADS)
		{
		numZones = TestZoneCount / TEST_MAX_ZONE_THREADS;
		}

	while (1)
		{
		TInt index = 0;
		TInt tempy = 0;
		for (; index < numZones; index ++)
			{
			User::AfterHighRes(TEST_MAX_ZONE_THREADS*TickPeriod/4);
			tempy = zoneId + (TEST_MAX_ZONE_THREADS * index);
			if (tempy < (TInt)TestZoneCount)
				{
				RamstressLdd.DoMovePagesInZone(tempy);
				}
			if (TestDefragTestEnd)
				break;
			}
		if (TestDefragTestEnd)
			break;
		}	
	return KErrNone;
	}

//
// DoDefragAutoTest
//
// Call the auto tests whilst defraging in the background.
//

void DoDefragAutoTest()
	{
	TUint	localZoneCount = TestZoneCount;
	if (TestZoneCount > TEST_MAX_ZONE_THREADS)
		{
		localZoneCount = TEST_MAX_ZONE_THREADS;
		}
	TInt			size =    (sizeof(RThread) * localZoneCount) 
							+ (sizeof(TInt) * localZoneCount);
	TUint8*			pBuf = (TUint8*)User::AllocZ(size);

	test(pBuf != NULL);
	RThread			*pTheThreads = (RThread*)pBuf;
	TInt			*pThreadInUse = (TInt*)(pTheThreads + localZoneCount);
	TInt			 ret;
	TUint			 index;
	for (index = 0; index < localZoneCount; index ++)
		{
		DBGS_PRINT((_L("%S : Starting Defrag Thread %d\n"), &TestNameBuffer, index));
		ret = pTheThreads[index].Create(KTestBlank,MultipleDefragThread,KDefaultStackSize,NULL,(TAny*) index);
		if (ret == KErrNone)
			{
			pTheThreads[index].Resume();
			pThreadInUse[index] = 1;
			}
		else
			{
			DBGS_PRINT((_L("%S : Starting Defrag Thread Failed %d\n"), &TestNameBuffer, index));
			}
		}

	// Do the full auto tests...
	PerformAutoTest(TestIsDemandPaged);

	TestDefragTestEnd = ETrue;
	RamstressLdd.DoSetEndFlag(1);
	TBool	anyUsed = ETrue;

	DBGS_PRINT((_L("%S : Waiting for Defrag Threads to exit...\n"), &TestNameBuffer));	
	TUint killNext = User::TickCount();
	while(anyUsed)
		{
		anyUsed = EFalse;
		
		// walk through the thread list to check which are still alive.
		for (index = 0; index < localZoneCount; index++)
			{
			if (pThreadInUse[index])
				{
				if (pTheThreads[index].ExitType() != EExitPending)
					{
					if (pTheThreads[index].ExitType() == EExitPanic)
						{
						DBGS_PRINT((_L("%S : Defrag Thread %d Panic'd\n"), &TestNameBuffer, index));	
						}
					else
						{
						DBGS_PRINT((_L("%S : Defrag Thread %d Exited\n"), &TestNameBuffer, index));	
						}
					pTheThreads[index].Close();
					pThreadInUse[index] = EFalse;
					}
				else
					{
					anyUsed = ETrue;
					TUint now = User::TickCount();
					TUint time = TUint((TUint64)(now-killNext)*(TUint64)TickPeriod/(TUint64)1000000);
					const TUint killTimeStep = (TEST_DOT_PERIOD+9)/10; // 1/10th of a dot
					if(time>TEST_DOT_PERIOD+killTimeStep)
						{
						killNext += killTimeStep*1000000/TickPeriod;
						DBGS_PRINT((_L("%S : killing Defrag Thread %d\n"), &TestNameBuffer, index));	
						pTheThreads[index].Kill(KErrNone);
						pTheThreads[index].Close();
						pThreadInUse[index] = EFalse;
						}
					}
				}
			}
		User::After(500000);
		}
	DBGS_PRINT((_L("%S : Defrag Threads exited...\n"), &TestNameBuffer));	
	RamstressLdd.DoSetEndFlag(0);
	User::Free(pBuf);
	}

//
// DoDefragTest
//
// Test the ram defrag code.
//

void DoDefragTest(void)
	{
	SVMCacheInfo  tempPages;
	memset(&tempPages, 0, sizeof(tempPages));

	test.Next(_L("Ram Defrag : Get the number of zones"));
	// first get the number of zones
	TInt ret = UserSvr::HalFunction(EHalGroupRam,ERamHalGetZoneCount,&TestZoneCount,0);
	if(ret==KErrNotSupported)
		{
		test.Next(_L("TESTS NOT RUN - Ram Defrag appears to not be supported.\n"));
		return;
		}
	test(ret == KErrNone);
	test(TestZoneCount != 0);
	test.Printf(_L("RAM Zones (count=%u)\n"),TestZoneCount);

	// now get the config of each of the zones.
	TUint						index;
	struct SRamZoneConfig		config;
	struct SRamZoneUtilisation	util;
	test.Next(_L("Ram Defrag : Get info about the zones"));
	for (index = 0; index < TestZoneCount; index ++)
		{
		ret = UserSvr::HalFunction(EHalGroupRam,ERamHalGetZoneConfig,(TAny*)index, (TAny*)&config);
		test(ret == KErrNone);
		test.Printf(_L("config : id=%d index=%d base=0x%08x end=0x%08x pages=%d pref=%d flags=0x%x\n"),
					config.iZoneId,config.iZoneIndex,config.iPhysBase,config.iPhysEnd,config.iPhysPages, 
					config.iPref,config.iFlags);

		ret = UserSvr::HalFunction(EHalGroupRam,ERamHalGetZoneUtilisation,(TAny*)index, (TAny*)&util);
		test(ret == KErrNone);
		test.Printf(_L("usage  : id=%d index=%d pages=%d free=%d unknown=%d fixed=%d move=%d discard=%d other=%d\n"),
					util.iZoneId,util.iZoneIndex,util.iPhysPages,util.iFreePages,
					util.iAllocUnknown,util.iAllocFixed,util.iAllocMovable,util.iAllocDiscardable,util.iAllocOther);
		}
	// Now test for zones out of range.
	test.Next(_L("Ram Defrag : test out of range indexes"));
	ret = UserSvr::HalFunction(EHalGroupRam,ERamHalGetZoneConfig,(TAny*)(TestZoneCount + 1), (TAny*)&config);
	test(ret != KErrNone);
	ret = UserSvr::HalFunction(EHalGroupRam,ERamHalGetZoneUtilisation,(TAny*)(TestZoneCount + 1), (TAny*)&util);
	test(ret != KErrNone);

	ret = UserSvr::HalFunction(EHalGroupRam,ERamHalGetZoneConfig,(TAny*)-1, (TAny*)&config);
	test(ret != KErrNone);
	ret = UserSvr::HalFunction(EHalGroupRam,ERamHalGetZoneUtilisation,(TAny*)-1, (TAny*)&util);
	test(ret != KErrNone);
	test.Next(_L("Ram Defrag : test out of range enums"));
	ret = UserSvr::HalFunction(EHalGroupRam,-1, 0, 0);
	test(ret != KErrNone);
	ret = UserSvr::HalFunction(EHalGroupRam,ERamHalGetZoneUtilisation + 1,0, 0);
	test(ret != KErrNone);

	TInt r = User::LoadLogicalDevice(KRamStressTestLddName);
	RUNTEST1(r==KErrNone || r==KErrAlreadyExists);
	RUNTEST(RamstressLdd.Open(),KErrNone);
	//TestDebug = ETrue;
	RUNTEST(RamstressLdd.DoSetDebugFlag((TInt)TestDebug),KErrNone);

	test.Next(_L("Ram Defrag : set VM cache to stress free..."));

	if (TestIsDemandPaged)
		{
		// get the old cache info
		UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0);

		TInt minSize = 512 * 4096;
		TInt maxSize = 32767 * 4096;
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minSize,(TAny*)maxSize);
		}

	test.Next(_L("Ram Defrag : move all pages in all zone in 1 thread..."));

	for (index = 0; index < TestZoneCount; index ++)
		{
		test.Printf(_L("Ram Defrag : moving pages in zone %u\n"),index);
		ret = RamstressLdd.DoMovePagesInZone(index);
		if (ret != KErrNone)
			{
			test.Printf(_L("Ram Defrag : moving pages in zone failed %u err=%d\n"), index, ret);
			}
		}


	test.Next(_L("Ram Defrag : Get info after test"));
	for (index = 0; index < TestZoneCount; index ++)
		{
		ret = UserSvr::HalFunction(EHalGroupRam,ERamHalGetZoneUtilisation,(TAny*)index, (TAny*)&util);
		test(ret == KErrNone);
		test.Printf(_L("usage  : id=%d index=%d pages=%d free=%d unknown=%d fixed=%d move=%d discard=%d other=%d\n"),
					util.iZoneId,util.iZoneIndex,util.iPhysPages,util.iFreePages,
					util.iAllocUnknown,util.iAllocFixed,util.iAllocMovable,util.iAllocDiscardable,util.iAllocOther);
		}

	test.Next(_L("Ram Defrag : Page moving on multiple threads with auto test running."));

	TestingDefrag = ETrue;
	TestDefragTestEnd = EFalse;

	DoDefragAutoTest();
	TestingDefrag = EFalse;
	/*
	 * End of test cleanup.
	 */

	test.Next(_L("Ram Defrag : reset VM cache back to stressed."));
	if (TestIsDemandPaged)
		{
		TInt minSize = tempPages.iMinSize;
		TInt maxSize = tempPages.iMaxSize;
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minSize,(TAny*)maxSize);
		}
	RamstressLdd.Close();
	test.Next(_L("Ram Defrag : Done"));
	}

//
// PerformExceptionThread
//
// Generate a Panic
//

LOCAL_C TInt PerformExceptionThread(TAny* )
	{
	User::AfterHighRes(1000000);
	// this line will cause a Kern::Exec 0 !!!
	test.Printf(_L("Hello World\n"));

	return KErrNone;
	}

//
// DoExceptionInAnotherThread
//
// Test the d_exc and minkda functionality with faulting processes.
//

void DoExceptionInAnotherThread(void)
	{
	TRequestStatus	theStatus;
	RThread			theThread;
	
	TInt ret = theThread.Create(KTestBlank,PerformExceptionThread,KDefaultStackSize,NULL,NULL);
	test(ret == KErrNone);
	theThread.Logon(theStatus);
	RUNTEST1(theStatus == KRequestPending);	
	theThread.Resume();
	theThread.Close();
	User::WaitForRequest(theStatus);
	}

//
// DoTestD_Exc
//
// Test the d_exc and minkda functionality with faulting processes.
//

TInt DoTestD_Exc()
	{
	if (!TestSilent)
		{
		test.Next(_L("DoTestD_Exc : d_exc check test."));
		}
	DBGS_PRINT((_L("%S : DoTestD_Exc start...\n"), &TestNameBuffer));	
	// first we need to spawn d_exc.exe
	RProcess dexcProcess;
	TInt ret = dexcProcess.Create(_L("d_exc.exe"),_L("-b"));
	RUNTEST1(KErrNone == ret);
	TRequestStatus dexcStatus;
	dexcProcess.Logon(dexcStatus);
	RUNTEST1(dexcStatus == KRequestPending);	
	dexcProcess.Resume();

	DBGS_PRINT((_L("%S : DoTestD_Exc started d_exc.exe\n"), &TestNameBuffer));	

	DoExceptionInAnotherThread();

	DBGS_PRINT((_L("%S : DoTestD_Exc test completed\n"), &TestNameBuffer));	
	// check that d_exc and minkda don't die!
	RUNTEST1(dexcProcess.ExitType() == EExitPending);

	DBGS_PRINT((_L("%S : DoTestD_Exc d_exc still running\n"), &TestNameBuffer));	
	
	// kill off d_exc!
	dexcProcess.Kill(KErrNone);
	dexcProcess.Close();
	User::WaitForRequest(dexcStatus);
	DBGS_PRINT((_L("%S : DoTestD_Exc d_exc killed and exiting\n"), &TestNameBuffer));	
	return KErrNone;
	}

/**
	Get name of the hash file used for an EXE or DLL which has been
	copied to writable media.

	@param	aOrigName		Name of EXE or DLL which has been copied to
							writable media.  This does not have to be
							qualified because only the name and extension
							are used.
	@param	aHashName		On return this is set to the absolute filename
							which should contain the file's hash.  This
							function does not create the file, or its containing
							directory.
 */

static void GetHashFileName(const TDesC& aOrigName, TDes& aHashName)
	{
	aHashName.Copy(KSysHash);
	aHashName[0] = (TUint8) RFs::GetSystemDriveChar();
	const TParsePtrC ppc(aOrigName);
	aHashName.Append(ppc.NameAndExt());
	}

//
// HashFile
// take hash of files require full drive:/path/name.ext
//

void HashFile(const TDesC& aFileName, RFs& aFs)
	{
	CSHA1* sha1 = CSHA1::NewL();
	CleanupStack::PushL(sha1);
	
	TBuf<50> hashfile;
	hashfile = KSysHash;
	hashfile[0] = (TUint8) RFs::GetSystemDriveChar();
	
	TInt r = aFs.MkDirAll(hashfile);
	RUNTEST1(r==KErrNone || r==KErrAlreadyExists);

	RFile fDest;
	r = fDest.Open(aFs, aFileName, EFileRead | EFileStream);
	if (r != KErrNone)
		{
		if (TestingReaper && (r == KErrInUse))
			{
			TBool whinged = EFalse;
			while (r == KErrInUse)
				{
				User::After(2000000);
				if (!whinged)
					{
					DBGS_PRINT((_L("HashFile() retrying Open for %S (%d)\n"), &aFileName, r));
					whinged = ETrue;
					}
				r = fDest.Open(aFs, aFileName, EFileRead | EFileStream);
				}

			}
		else
			{
			DBGS_PRINT((_L("fDest.Open returned %d\n"), r));
			}
		}
	User::LeaveIfError(r);
	CleanupClosePushL(fDest);

	TBool done;
	TBuf8<512> content;
	do
		{
		r = fDest.Read(content);
		if (r!=KErrNone)
			DBGS_PRINT((_L("fDest.Read returned %d\n"), r));	
		User::LeaveIfError(r);
		done = (content.Length() == 0);
		if (! done)
			sha1->Update(content);
		} while (! done);
	CleanupStack::PopAndDestroy(&fDest);

	// write hash to \sys\hash
	TBuf8<SHA1_HASH> hashVal = sha1->Final();

	TFileName fnSrc(aFileName);
	GetHashFileName(aFileName, fnSrc);
	RFile fHash;
	r = fHash.Replace(aFs, fnSrc, EFileWrite | EFileStream);
	if (r != KErrNone)
		DBGS_PRINT((_L("fHash.Replace returned %d\n"), r));
	User::LeaveIfError(r);
	CleanupClosePushL(fHash);
	r = fHash.Write(hashVal);
	if (r != KErrNone)
		DBGS_PRINT((_L("fHash.Write returned %d\n"), r));
	User::LeaveIfError(r);

	CleanupStack::PopAndDestroy(2, sha1);
	}

//
// CopyFileToMMc
//
// Copy a file to the MMC card and create a hash of it.
//

TInt CopyFileToMMc(RFs& aFs,CFileMan* aFileMan, TPtrC aPath, TPtrC  aOldFilename, TPtrC  aNewFilename)
	{
	TInt retVal = aFs.MkDirAll(aPath);
	RUNTEST1(retVal==KErrNone || retVal==KErrAlreadyExists);

	TFileName newPath;
	TFileName oldPath;

	oldPath.Format(_L("%S%S"),&KRomPath, &aOldFilename);
	newPath.Format(_L("%S%S"),&aPath, &aNewFilename);
	DBGD_PRINT((_L("Copying %S to %S\n"), &oldPath, &newPath));
	retVal = aFileMan->Copy(oldPath, newPath, CFileMan::EOverWrite);
	if (retVal == KErrNone)
		{
		retVal = aFileMan->Attribs(newPath, KEntryAttNormal, KEntryAttReadOnly, 0);
		if (retVal != KErrNone)
			{
			DBGS_PRINT((_L("%S :   Attribs failed (%d)\n"), &newPath, retVal));
			}
		TEntry  anEntry;
		retVal = aFs.Entry(newPath, anEntry);
		if (retVal != KErrNone)
			{
			DBGS_PRINT((_L("%S : aFs.Entry failed (%d)\n"), &newPath, retVal));
			}
		TRAPD(r, HashFile(newPath, aFs));
		RUNTEST1(r == KErrNone);
		}
	else
		DBGS_PRINT((_L("Failed to copy file %d\n"), retVal));
	DBGD_PRINT((_L("%S : now %S (%d)\n"), &newPath, EXISTS(retVal), retVal));
	return retVal;
	}

//
// CopyAndFragmentFiles
//
// Copy the test files to a specified location edeavouring to fragment as much as possible.
//

TBool CopyAndFragmentFiles(RFs& aFs,CFileMan* aFileMan, TPtrC aPath, ETestMediaType aMediaType)
	{
	TInt retVal = aFs.MkDirAll(aPath);
	RUNTEST1(retVal==KErrNone || retVal==KErrAlreadyExists);
#define FILECOUNTMAX (PAGELDRTST_MAX_DLLS + 2)
	RFile	theInFiles[FILECOUNTMAX];
	RFile	theOutFiles[FILECOUNTMAX];
	TInt	inFileSize[FILECOUNTMAX];
	TInt	inFilePos[FILECOUNTMAX];
	TBool	fileOk[FILECOUNTMAX];

	TInt	  index;
	TFileName newPath;
	TFileName oldPath;

	for (index = 0; index < FILECOUNTMAX; index ++)
		{
		inFileSize[index] = 0;
		inFilePos[index] = 0;
		fileOk[index] = EFalse;

		if (index < PAGELDRTST_MAX_DLLS)
			{
			oldPath.Format(_L("%S%S%d%S"), &KRomPath, &KDllBaseName, index, &TestPlExtNames[KTestMediaBase]);
			newPath.Format(_L("%S%S%d%S"), &aPath, &KDllBaseName, index, &TestPlExtNames[aMediaType]);
			}
		else if (index < (PAGELDRTST_MAX_DLLS + 1))
			{
			oldPath.Format(_L("%S%S"), &KRomPath, &TestPsExeNames[KTestMediaBase]);
			newPath.Format(_L("%S%S"), &aPath, &TestPsExeNames[aMediaType]);
			}
		else
			{
			oldPath.Format(_L("%S%S"), &KRomPath, &TestPlExeNames[KTestMediaBase]);
			newPath.Format(_L("%S%S"), &aPath, &TestPlExeNames[aMediaType]);
			}

		retVal = theInFiles[index].Open(aFs, oldPath, EFileRead);
		if (retVal != KErrNone)
			{
			DBGS_PRINT((_L("%S : Failed to open for read (%d)\n"), &oldPath, retVal));
			break;
			}
		retVal = theInFiles[index].Size(inFileSize[index]);
		if (retVal != KErrNone)
			{
			theInFiles[index].Close();
			DBGS_PRINT((_L("%S : Failed to get file size (%d)\n"), &newPath, retVal));
			break;
			}
		retVal = theOutFiles[index].Replace(aFs, newPath, EFileWrite);
		if (retVal != KErrNone)
			{
			theInFiles[index].Close();
			DBGS_PRINT((_L("%S : Failed to open for write (%d)\n"), &newPath, retVal));
			break;
			}

		fileOk[index] = ETrue;
		}

	const TInt KBufferSize = 3333;
	TBuf8<KBufferSize> buffer;
	TBool stillGoing;

	do
		{
		stillGoing = EFalse;
		for (index = 0; index < FILECOUNTMAX; index ++)
			{
			if (!fileOk[index])
				break;
			if (inFilePos[index] < inFileSize[index])
				{
				retVal = theInFiles[index].Read(buffer);
				if (retVal != KErrNone)
					{
					DBGS_PRINT((_L("theInFiles[%d] read failed (%d)\n"), index, retVal));
					break;
					}
				retVal = theOutFiles[index].Write(buffer);
				if (retVal != KErrNone)
					{
					DBGS_PRINT((_L("theOutFiles[%d] Write failed (%d)\n"), index, retVal));
					break;
					}
				retVal = theOutFiles[index].Flush();
				if (retVal != KErrNone)
					{
					DBGS_PRINT((_L("theOutFiles[%d] flush failed (%d)\n"), index, retVal));
					break;
					}
				inFilePos[index] += buffer.Length();
				if (inFilePos[index] < inFileSize[index])
					stillGoing = ETrue;
				}
			}
		}
	while (stillGoing);

	TBool allOk = retVal == KErrNone;
	for (index = 0; index < FILECOUNTMAX; index ++)
		{
		if (!fileOk[index])
			{
			allOk = EFalse;
			break;
			}
		theInFiles[index].Close();
		theOutFiles[index].Close();
		if (index < PAGELDRTST_MAX_DLLS)
			{
			newPath.Format(_L("%S%S%d%S"), &aPath, &KDllBaseName, index, &TestPlExtNames[aMediaType]);
			}
		else if (index < (PAGELDRTST_MAX_DLLS + 1))
			{
			newPath.Format(_L("%S%S"), &aPath, &TestPsExeNames[aMediaType]);
			}
		else
			{
			newPath.Format(_L("%S%S"), &aPath, &TestPlExeNames[aMediaType]);
			}

		retVal = aFileMan->Attribs(newPath, KEntryAttNormal, KEntryAttReadOnly, 0);
		if (retVal != KErrNone)
			{
			DBGS_PRINT((_L("%S : Attribs failed (%d)\n"), &newPath, retVal));
			allOk = EFalse;
			}
		TEntry  anEntry;
		retVal = aFs.Entry(newPath, anEntry);
		if (retVal != KErrNone)
			{
			DBGS_PRINT((_L("%S : aFs.Entry failed (%d)\n"), &newPath, retVal));
			allOk = EFalse;
			}
		TRAPD(r, HashFile(newPath, aFs));
		if (r != KErrNone)
			{
			allOk = EFalse;
			}
		DBGD_PRINT((_L("%S : %S!\n"), &newPath, EXISTS(!allOk)));
		}
	return allOk;
	}

//
// CheckFilePresence
//
// Checks all the files required for the test are present and copies some tests to the MMC card
//

void CheckFilePresence(TBool aDoFileCopy)
	{
	TUint start = User::TickCount();

	RFs fs;
	if (KErrNone != fs.Connect())
		{
		DBGS_PRINT(_L("CheckFilePresence : Can't connect to the FS\n"));
		return ;
		}

	TFileName filename;
	TFileName newFilename;
	TEntry anEntry;
	TInt   index;
	TInt   retVal;
	TInt   dllIndex;

	// now we need to add the MMC files
	TInt drvNum = FindMMCDriveNumber(fs);
	TBuf<32>	mmcPath;
	mmcPath.Format(_L("%S"),&KMmcDefaultPath);
	if (drvNum >= 0)
		mmcPath[0] = 'a' + drvNum;

	TBool	allOk;
	//TInt  indexMax = aDoFileCopy ? KTestMediaMmc : KTestMediaCOUNT; 
	for (index = 0; index < TEST_MEDIA_COUNT_HACK; index ++)
		{
		allOk = ETrue;	
		filename.Format(_L("%S%S"),(index == KTestMediaMmc) ? & mmcPath : &KRomPath, &TestPsExeNames[index]);
		if (KErrNone != fs.Entry(filename, anEntry))
			allOk = EFalse;

		filename.Format(_L("%S%S"),(index == KTestMediaMmc) ? & mmcPath : &KRomPath, &TestPlExeNames[index]);
		if (KErrNone != fs.Entry(filename, anEntry))
			allOk = EFalse;

		for (dllIndex = 0; dllIndex < PAGELDRTST_MAX_DLLS; dllIndex ++)
			{
			filename.Format(_L("%S%S%d%S"), (index == KTestMediaMmc) ? & mmcPath : &KRomPath, &KDllBaseName, dllIndex, &TestPlExtNames[index]);
			if (KErrNone != fs.Entry(filename, anEntry))
				allOk = EFalse;
			}
		TestDllExesExist[index] = allOk;
		DBGS_PRINT((_L("%S : %S!\n"), &TestPsExeNames[index], EXISTS(!TestDllExesExist[index])));
		}
	TInt nandDrvNum = FindFsNANDDrive(fs);
	if (aDoFileCopy && (drvNum >= 0) && (nandDrvNum >= 0))
		{
		CTrapCleanup* cleanupStack = CTrapCleanup::New();
		if(!cleanupStack)
			DBGS_PRINT((_L("Cleanup stack failed\n")));	
		CFileMan* pFileMan = NULL;
		TRAP(retVal, pFileMan = CFileMan::NewL(fs));
	
		// First make a clean copy of the DLLs to the MMC card.
		allOk = ETrue;			
		if (KErrNone != CopyFileToMMc(fs, pFileMan, mmcPath, TestPsExeNames[KTestMediaBase], TestPsExeNames[KTestMediaMmc]))
			allOk = EFalse;
		if (KErrNone != CopyFileToMMc(fs, pFileMan, mmcPath, TestPlExeNames[KTestMediaBase], TestPlExeNames[KTestMediaMmc]))
			allOk = EFalse;
		for (dllIndex = 0; dllIndex < PAGELDRTST_MAX_DLLS; dllIndex ++)
			{
			filename.Format(_L("%S%d%S"), &KDllBaseName, dllIndex, &TestPlExtNames[KTestMediaBase]);
			newFilename.Format(_L("%S%d%S"), &KDllBaseName, dllIndex, &TestPlExtNames[KTestMediaMmc]);
			if (KErrNone != CopyFileToMMc(fs, pFileMan, mmcPath, filename, newFilename))
				allOk = EFalse;
			}
		TestDllExesExist[KTestMediaMmc] = allOk;
		DBGS_PRINT((_L("%S : %S! (Drive %c)\n"), &TestPsExeNames[index], EXISTS(!TestDllExesExist[index]), mmcPath[0]));
#ifdef TEST_ADD_FRAGD_MEDIA
		//now make some fragmented files on the MMC card.
		TestDllExesExist[KTestMediaMmcFrag] = CopyAndFragmentFiles(fs, pFileMan, mmcPath, KTestMediaMmcFrag);
		DBGS_PRINT((_L("%S : %S! (Drive %c)\n"), &TestPsExeNames[KTestMediaMmcFrag], EXISTS(!TestDllExesExist[KTestMediaMmcFrag]), mmcPath[0]));

		//now make some fragmented files on the NAND card.
		if (nandDrvNum >= 0)
			{
			mmcPath[0] = 'a' + nandDrvNum;
			TestDllExesExist[KTestMediaNandFrag] = CopyAndFragmentFiles(fs, pFileMan, mmcPath, KTestMediaNandFrag);
			DBGS_PRINT((_L("%S : %S! (Drive %c)\n"), &TestPsExeNames[KTestMediaNandFrag], EXISTS(!TestDllExesExist[KTestMediaNandFrag]), mmcPath[0]));
			}
		else
			DBGS_PRINT((_L("CheckFilePresence : Failed to get NAND drive number\n")));
#endif // TEST_ADD_FRAGD_MEDIA
		delete pFileMan; pFileMan = NULL;
		delete cleanupStack; cleanupStack = NULL;
		}

	fs.Close();

	TUint end = User::TickCount();
	TUint time = TUint((TUint64)(end-start)*(TUint64)TickPeriod/(TUint64)1000000);
	DBGS_PRINT((_L("CheckFilePresence : %d secs elapsed\n"), time));
	}

//
// DoDeleteFile
//
// Delete a file and remove the hash
//

void DoDeleteFile(CFileMan* aFileMan, TBool aSilent,TFileName& aFileName )
	{
	TFileName hashName;
	RLoader l;
	test(l.Connect() == KErrNone);

	DBGD_PRINT((_L("Deleting %S ...\n"), &aFileName));
	if (!aSilent)
		DBGD_PRINT((_L("Deleting %S\n"), &aFileName));
	TInt retVal = aFileMan->Delete(aFileName);
	if (retVal != KErrNone)
		{
		if (TestingReaper)
			{
			aFileMan->Attribs(aFileName, KEntryAttNormal, KEntryAttReadOnly, 0);
			retVal = l.Delete(aFileName);
			if (retVal != KErrNone)
				{
				DBGS_PRINT((_L("RLoader::Delete %S Failed %d\n"), &aFileName, retVal));
				}
			}
		else
			{
			if (!aSilent)
				DBGS_PRINT((_L("Deleting %S Failed %d\n"), &aFileName, retVal));
			}
		}
	GetHashFileName(aFileName, hashName);
	retVal = aFileMan->Delete(hashName);
	if (retVal != KErrNone)
		{
		if (TestingReaper && (retVal == KErrInUse))
			{
			retVal = l.Delete(hashName);
			if (retVal != KErrNone)
				{
				DBGS_PRINT((_L("RLoader::Delete %S Failed %d\n"), &hashName, retVal));
				}
			}
		else
			{
			if (!aSilent)
				DBGS_PRINT((_L("Deleting %S Failed %d\n"), &hashName, retVal));
			}
		}
	l.Close();
	}

//
// CleanupFiles
//
// Remove any copied files and created directories.
//

void CleanupFiles(TBool silent)
	{
	TUint start = User::TickCount();

	RFs fs;
	if (KErrNone != fs.Connect())
		{
		DBGS_PRINT(_L("CleanupFiles : Can't connect to the FS\n"));
		return ;
		}

	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	if(!cleanupStack)
		if (!silent)
			DBGS_PRINT((_L("Cleanup stack failed\n")));	
	
	CFileMan* pFileMan = NULL;
	TInt retVal;
	TRAP(retVal, pFileMan = CFileMan::NewL(fs));
	
	TFileName newPath;
	TInt index;
	TInt dllIndex;

	TBuf<32>	path;
	path.Format(_L("%S"),&KMmcDefaultPath);
	TInt mmcDrvNum = FindMMCDriveNumber(fs);
	TInt nandDrvNum = FindFsNANDDrive(fs);
	for (index = KTestMediaMmc; index < KTestMediaCOUNT; index ++)
		{
#ifdef TEST_ADD_FRAGD_MEDIA
		if (index == KTestMediaNandFrag)
			{
			if (nandDrvNum < 0)
				continue;
			path[0] = 'a' + nandDrvNum;
			}
		else
			{
			if (mmcDrvNum < 0)
				continue;
			path[0] = 'a' + mmcDrvNum;
			}
#else
		path[0] = 'a' + mmcDrvNum;
#endif
		newPath.Format(_L("%S%S"),&path, &TestPsExeNames[index]);
		DoDeleteFile(pFileMan, silent,  newPath);

		newPath.Format(_L("%S%S"),&path, &TestPlExeNames[index]);
		DoDeleteFile(pFileMan, silent,  newPath);
		
		for (dllIndex = 0; dllIndex < PAGELDRTST_MAX_DLLS; dllIndex ++)
			{
			newPath.Format(_L("%S%S%d%S"), &path, &KDllBaseName, dllIndex, &TestPlExtNames[index]);
			DoDeleteFile(pFileMan, silent,  newPath);
			}
		}
	if (nandDrvNum >= 0)
		{
		path[0] = 'a' + nandDrvNum;
		fs.RmDir(path);
		}
	if (mmcDrvNum >= 0)
		{
		path[0] = 'a' + mmcDrvNum;
		fs.RmDir(path);
		}

	delete pFileMan; pFileMan = NULL;
	delete cleanupStack; cleanupStack = NULL;
	fs.Close();
	TUint end = User::TickCount();
	TUint time = TUint((TUint64)(end-start)*(TUint64)TickPeriod/(TUint64)1000000);
	DBGS_PRINT((_L("CleanupFiles : %d secs elapsed\n"), time));
	}

#ifdef _DEBUG

//
// FindLocalDriveNumber
//
// Find the local drive
//

TInt FindLocalDriveNumber(RFs &aFs, TInt aFsDrvNum)
	{
	RFile file;
	TBuf<256> fileName;	
	fileName.Append((TChar)('A' + aFsDrvNum));
	fileName+=_L(":\\f32-tst\\");
	TInt r=aFs.MkDirAll(fileName);
	TInt locDriveNumber = -1;
	if (r==KErrNone || r== KErrAlreadyExists)
		{
		fileName += _L("tempy.txt");
		r=file.Replace(aFs,fileName,EFileWrite);
		if (r!=KErrNone)
			DBGS_PRINT((_L("FindLocalDriveNumber : Error %d: file '%S' could not be created\n"),r,&fileName));
		RUNTEST1(r==KErrNone);
		r=file.Write(_L8("Flies as big as sparrows indoletly buzzing in the warm air, heavy with the stench of rotting carcasses"));
		if (r!=KErrNone)
			{
			DBGS_PRINT((_L("FindLocalDriveNumber : Error %d: could not write to file %d (%S)\n"),r,aFsDrvNum, &fileName));
			}
		else
			{
			// write caching may be enabled to flush the cache...
			TRequestStatus flushStatus;
			file.Flush(flushStatus);
			User::WaitForRequest(flushStatus);
			// get the block map
			SBlockMapInfo info;
			TInt64 start=0;
			r=file.BlockMap(info, start, -1,ETestDebug);
			if (r==KErrNone || r==KErrCompletion)
				{
				locDriveNumber=info.iLocalDriveNumber;
				DBGD_PRINT((_L("FindLocalDriveNumber : locDriveNumber  %d\n"), locDriveNumber));
				}
			else
				DBGS_PRINT((_L("FindLocalDriveNumber : Error %d: error getting blockmap for drive %d (%S)\n"),r,aFsDrvNum, &fileName));
			}
		aFs.Delete(fileName);
		file.Close();
		}
	else
		DBGS_PRINT((_L("FindLocalDriveNumber : Error %d: error creating dir	\n"),r));
	return locDriveNumber;
	}

//
// ResetConcurrencyStats
//
// Reset the stats
//

void ResetConcurrencyStats(RFs& aFs)
	{
	if(TestBootedFromMmc)
		{
		TInt fsDriveNum = FindMMCDriveNumber(aFs);
		if (fsDriveNum >= 0)
			{
			TInt locDriveNumber = FindLocalDriveNumber(aFs, fsDriveNum);
			if (locDriveNumber >= 0)
				{
				RUNTEST(PagingInfo::ResetConcurrency(locDriveNumber,EMediaPagingStatsRomAndCode),KErrNone);
				}
			else
				DBGS_PRINT((_L("ResetConcurrencyStats MMC : Failed to get locDriveNumber %d (%d)\n"), locDriveNumber, fsDriveNum));
			}
		else
			DBGS_PRINT((_L("ResetConcurrencyStats MMC : Failed to get fsDriveNum %d\n"), fsDriveNum));
		}
	else
		{
		TInt fsDriveNum = FindFsNANDDrive(aFs);
		if (fsDriveNum >= 0)
			{
			TInt locDriveNumber = FindLocalDriveNumber(aFs, fsDriveNum);
			if (locDriveNumber >= 0)
				{
				RUNTEST(PagingInfo::ResetConcurrency(locDriveNumber,EMediaPagingStatsRomAndCode),KErrNone);
				}
			else
				DBGS_PRINT((_L("ResetConcurrencyStats NAND : Failed to get locDriveNumber %d (%d)\n"), locDriveNumber, fsDriveNum));
			}
		else
			DBGS_PRINT((_L("ResetConcurrencyStats NAND : Failed to get fsDriveNum %d\n"), fsDriveNum));
		}
	}

//
// ResetBenchmarks
//
// Reset the stats
//

void ResetBenchmarks(RFs& aFs)
	{
	if(TestBootedFromMmc)
		{
		TInt fsDriveNum = FindMMCDriveNumber(aFs);
		if (fsDriveNum >= 0)
			{
			TInt locDriveNumber = FindLocalDriveNumber(aFs, fsDriveNum);
			if (locDriveNumber >= 0)
				{
				RUNTEST(PagingInfo::ResetBenchmarks(locDriveNumber,EMediaPagingStatsRomAndCode),KErrNone);
				}
			else
				DBGS_PRINT((_L("ResetBenchmarks MMC : Failed to get locDriveNumber %d (%d)\n"), locDriveNumber, fsDriveNum));
			}
		else
			DBGS_PRINT((_L("ResetBenchmarks MMC : Failed to get fsDriveNum %d\n"), fsDriveNum));
		}
	else
		{
		TInt fsDriveNum = FindFsNANDDrive(aFs);
		if (fsDriveNum >= 0)
			{
			TInt locDriveNumber = FindLocalDriveNumber(aFs, fsDriveNum);
			if (locDriveNumber >= 0)
				{
				RUNTEST(PagingInfo::ResetBenchmarks(locDriveNumber,EMediaPagingStatsRomAndCode),KErrNone);
				}
			else
				DBGS_PRINT((_L("ResetBenchmarks NAND : Failed to get locDriveNumber %d (%d)\n"), locDriveNumber, fsDriveNum));
			}
		else
			DBGS_PRINT((_L("ResetBenchmarks NAND : Failed to get fsDriveNum %d\n"), fsDriveNum));
		}
	}

//
// DisplayConcurrencyStats
//
// Display the stats
//

void DisplayConcurrencyStats(RFs& aFs)
	{
	if(TestBootedFromMmc)
		{
		TInt fsDriveNum = FindMMCDriveNumber(aFs);
		if (fsDriveNum >= 0)
			{
			TInt locDriveNumber = FindLocalDriveNumber(aFs, fsDriveNum);
			if (locDriveNumber >= 0)
				{
				DBGS_PRINT((_L("MMC stats\n")));
				RUNTEST1(PagingInfo::PrintConcurrency(locDriveNumber,EMediaPagingStatsRomAndCode)==KErrNone);
				}
			else
				DBGS_PRINT((_L("DisplayConcurrencyStats MMC : Failed to get locDriveNumber %d (%d)\n"), locDriveNumber, fsDriveNum));
			}
		else
			DBGS_PRINT((_L("DisplayConcurrencyStats MMC : Failed to get fsDriveNum %d\n"), fsDriveNum));
		}
	else
		{
		TInt fsDriveNum = FindFsNANDDrive(aFs);
		if (fsDriveNum >= 0)
			{
			TInt locDriveNumber = FindLocalDriveNumber(aFs, fsDriveNum);

			if (locDriveNumber >= 0)
				{
				DBGS_PRINT((_L("NAND stats\n")));
				RUNTEST1(PagingInfo::PrintConcurrency(locDriveNumber,EMediaPagingStatsRomAndCode)==KErrNone);
				}
			else
				DBGS_PRINT((_L("DisplayConcurrencyStats NAND : Failed to get locDriveNumber %d (%d)\n"), locDriveNumber, fsDriveNum));
			}
		else
			DBGS_PRINT((_L("DisplayConcurrencyStats NAND : Failed to get fsDriveNum %d\n"), fsDriveNum));
		}
	}

void DisplayBenchmarks(RFs& aFs)
	{
	if(TestBootedFromMmc)
		{
		TInt fsDriveNum = FindMMCDriveNumber(aFs);
		if (fsDriveNum >= 0)
			{
			TInt locDriveNumber = FindLocalDriveNumber(aFs, fsDriveNum);
			if(locDriveNumber>=0)
				{
				DBGS_PRINT((_L("MMC benchmarks\n")));
				RUNTEST1(PagingInfo::PrintBenchmarks(locDriveNumber,EMediaPagingStatsRomAndCode)==KErrNone);
				}
			else
				DBGS_PRINT((_L("DisplayBenchmarks MMC : Failed to get locDriveNumber %d (%d)\n"), locDriveNumber, fsDriveNum));
			}
		else
			DBGS_PRINT((_L("DisplayBenchmarks MMC : Failed to get fsDriveNum %d\n"), fsDriveNum));
		}
	else
		{
		TInt fsDriveNum = FindFsNANDDrive(aFs);
		if (fsDriveNum >= 0)
			{
			TInt locDriveNumber = FindLocalDriveNumber(aFs, fsDriveNum);
			if(locDriveNumber>=0)
				{
				DBGS_PRINT((_L("NAND benchmarks\n")));
				RUNTEST1(PagingInfo::PrintBenchmarks(locDriveNumber,EMediaPagingStatsRomAndCode)==KErrNone);
				}
			else
				DBGS_PRINT((_L("DisplayBenchmarks NAND : Failed to get locDriveNumber %d (%d)\n"), locDriveNumber, fsDriveNum));
			}
		else
			DBGS_PRINT((_L("DisplayBenchmarks NAND : Failed to get fsDriveNum %d\n"), fsDriveNum));
		}
	}

#endif

void DoStats()
	{
	if (TestIsDemandPaged)
		{
		SVMCacheInfo  tempPages;
		UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0);
		DBGS_PRINT((_L("DPC : min %d max %d curr %d\n"), 
					tempPages.iMinSize, tempPages.iMaxSize, tempPages.iCurrentSize));
		DBGS_PRINT((_L("    : maxFree %d freeRam %d\n"),
					tempPages.iMaxFreeSize, FreeRam()));
		}

#ifdef _DEBUG
	if (TestWeAreTheTestBase && !TestSilent)
		{
		RFs fs;
		if (KErrNone != fs.Connect())
			{
			DBGS_PRINT(_L("ResetConcurrencyStats : Can't connect to the FS\n"));
			return;
			}

#ifndef TEST_MINIMAL_STATS
		DisplayConcurrencyStats(fs);
		DisplayBenchmarks(fs);
#endif
#ifndef TEST_DONT_RESET_STATS
		ResetConcurrencyStats(fs);
		ResetBenchmarks(fs);
#endif
		fs.Close();
		}
#endif
	}


//
// E32Main
//
// Main entry point.
//

TInt E32Main()
	{
#ifndef TEST_ON_UNPAGED
	TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
	if(!romHeader->iPageableRomStart)
		{
		TestIsDemandPaged = EFalse;
		}
#endif
	// Turn off lazy dll unloading
	RLoader l;
	if (l.Connect() == KErrNone)
		{
		l.CancelLazyDllUnload();
		l.Close();
		}
	
	HAL::Get(HAL::ESystemTickPeriod, TickPeriod);

	SVMCacheInfo  tempPages;
	memset(&tempPages, 0, sizeof(tempPages));

	TBool parseResult = ParseCommandLine();

	if (TestExit)
		{
		return KErrNone;
		}

	TUint start = User::TickCount();
	
	AreWeTheTestBase();	

	if (TestIsDemandPaged)
		{
		TInt  minSize = TestMinCacheSize;
		TInt  maxSize = TestMaxCacheSize;

		SVMCacheInfo  tempPages;

		// get the old cache info
		UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0);
		// set the cache to our test value
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minSize,(TAny*)maxSize);
		}
	if (!TestSilent)
		{
		test.Title();
		test.Start(_L("Demand Paging loader stress tests..."));
		test.Printf(_L("%S (%d)\n"), &TestNameBuffer, TestWeAreTheTestBase);
		test.Printf(_L("TestBootedFromMmc %d\n"), TestBootedFromMmc);

		if (TestWeAreTheTestBase)
			CleanupFiles(ETrue);

		CheckFilePresence(TestWeAreTheTestBase);
		}

	if (parseResult)
		{
		if (TestLowMem)
			{
			DoLowMemTest(ETrue);
			}
		if (TestSingle)
			{
			RUNTEST(DoSingleTest(),KErrNone);
			}
		if (TestMultiple)
			{
			RUNTEST(DoMultipleTest(),KErrNone);
			}
		if (TestD_Exc)
			{
			RUNTEST(DoTestD_Exc(),KErrNone);
			}
		if (TestChunks)
			{
			DoChunkTests();
			}
		if (TestReaper)
			{
			DoReaperTests();
			}
		if (TestBtrace)
			{
			DoBtraceTest();
			}
		if (TestDefrag)
			{
			DoDefragTest();
			}
		}
	else
		{
#ifdef _DEBUG
		if (TestWeAreTheTestBase)
			{
			RFs fs;
			if (KErrNone == fs.Connect())
				{
				//fs.SetDebugRegister(KCACHE);
				ResetConcurrencyStats(fs);
				ResetBenchmarks(fs);
				fs.Close();
				}
			}
#endif

		while (1)
			{
			if (TestIsDemandPaged)
				{
#ifdef TEST_RUN_AUTOTEST
				PerformAutoTest();
#endif //TEST_RUN_AUTOTEST

#ifndef	TEST_SHORT_TEST
#ifdef TEST_RUN_LOWMEMTEST
				DoLowMemTest(ETrue);
#endif //TEST_RUN_LOWMEMTEST
#ifdef TEST_RUN_CHUNKTEST
				DoChunkTests();
#endif //TEST_RUN_CHUNKTEST
#ifdef TEST_RUN_REAPERTEST
				DoReaperTests();
#endif //TEST_RUN_REAPERTEST
#endif //TEST_SHORT_TEST
				}

#ifdef TEST_RUN_DEFRAGTEST
			DoDefragTest();
#endif //TEST_RUN_DEFRAGTEST

			if (TestStressFree)
				{
				TInt minSize = 512 * 4096;
				TInt maxSize = 32767 * 4096;
				UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minSize,(TAny*)maxSize);

				test.Printf(_L("%S Stress Free!!\n"), &TestNameBuffer, TestWeAreTheTestBase);
				TestStressFree = EFalse;
				}
			else
				{
				break;
				}
			}

#ifndef TEST_SHORT_TEST
#ifndef TEST_NO_DEXC_IN_AUTO
#ifdef TEST_RUN_D_EXCTEST
		RUNTEST(DoTestD_Exc(),KErrNone);
#endif //TEST_RUN_D_EXCTEST
#endif //TEST_NO_DEXC_IN_AUTO
		if (TestWeAreTheTestBase && TestFullAutoTest && TestIsDemandPaged)
			{
			RProcess		theProcess;
			TRequestStatus	status;

			TInt retVal = theProcess.Create(_L("t_pageldrtst_rom.exe"),_L("fullauto"));
			if (retVal != KErrNotFound)
				{
				RUNTEST1(KErrNone == retVal);
				theProcess.Logon(status);
				RUNTEST1(status == KRequestPending);	
				theProcess.Resume();
#ifdef TEST_THRASHING_TEST
				while (1)
					{
					if (theProcess.ExitType() != EExitPending)
						{
						RUNTEST1(theProcess.ExitType() != EExitPanic);
						break;
						}
					User::AfterHighRes(1);
					}
				User::WaitForRequest(status);
#else
				User::WaitForRequest(status);
				if (theProcess.ExitType() != EExitPending)
					{
					RUNTEST1(theProcess.ExitType() != EExitPanic);
					}
#endif //TEST_THRASHING_TEST
				theProcess.Close();
				}
			}
#endif //TEST_SHORT_TEST
#ifdef _DEBUG
		if (TestWeAreTheTestBase && !TestSilent)
			{
			RFs fs;
			if (KErrNone == fs.Connect())
				{
				DisplayConcurrencyStats(fs);
				DisplayBenchmarks(fs);
				fs.Close();
				}
			}
#endif
		}

	if (TestWeAreTheTestBase && !TestNoClean)
		CleanupFiles(EFalse);

	if (TestIsDemandPaged)
		{
		TInt minSize = tempPages.iMinSize;
		TInt maxSize = tempPages.iMaxSize;
		// put the cache back to the the original values.
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minSize,(TAny*)maxSize);
		}
	if (!TestSilent)
		{
		TUint end = User::TickCount();
		TUint time = TUint((TUint64)(end-start)*(TUint64)TickPeriod/(TUint64)1000000);
		test.Printf(_L("%S : Complete (%u seconds)\n"), &TestNameBuffer, time);	
		test.End();
		}
	return KErrNone;
	}
